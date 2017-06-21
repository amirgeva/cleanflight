/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * isl.c
 *
 * Author: Amir Geva
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"

#if defined(TELEMETRY) && defined(TELEMETRY_ISL)

#include "common/maths.h"
#include "common/axis.h"
#include "common/color.h"

#include "config/feature.h"
#include "config/parameter_group.h"
#include "config/parameter_group_ids.h"

#include "drivers/system.h"
#include "drivers/time.h"
#include "drivers/sensor.h"
#include "drivers/accgyro/accgyro.h"

#include "fc/config.h"
#include "fc/rc_controls.h"
#include "fc/runtime_config.h"

#include "flight/mixer.h"
#include "flight/pid.h"
#include "flight/imu.h"
#include "flight/failsafe.h"
#include "flight/navigation.h"
#include "flight/altitude.h"

#include "io/serial.h"
#include "io/gimbal.h"
#include "io/gps.h"
#include "io/ledstrip.h"
#include "io/motors.h"

#include "rx/rx.h"

#include "sensors/sensors.h"
#include "sensors/acceleration.h"
#include "sensors/gyro.h"
#include "sensors/barometer.h"
#include "sensors/boardalignment.h"
#include "sensors/battery.h"

#include "telemetry/telemetry.h"
#include "telemetry/isl.h"

// Rate is 100 Hz
#define TELEMETRY_ISL_MAXRATE 100
#define TELEMETRY_ISL_DELAY ((1000 * 1000) / TELEMETRY_ISL_MAXRATE)

extern uint16_t raw_rssi; // defined in rx.c

static serialPort_t *islPort = NULL;
static serialPortConfig_t *portConfig;

static bool islTelemetryEnabled =  false;
static portSharing_e islPortSharing;

#define ISL_BUFFER_SIZE 64

/*
 *		Valid buffer structure (out/in) is:
 *		Byte 0 = 0xFE  \
 *		Byte 1 = 0xED   \  4 bytes of the MAGIC header
 *		Byte 2 = 0xBE   /
 *		Byte 3 = 0xEF  /
 *		Bytes 4-61 = Payload (58 bytes)
 *      Byte 62 = LSB of CRC16 of entire payload 58 bytes
 *      Byte 63 = MSB of CRC16 of entire payload 58 bytes
 */
static const uint8_t MAGIC[] = {0xFE, 0xED, 0xBE, 0xEF};

// Buffer to contain outgoing telemetry messages
static uint8_t islOutBuffer[ISL_BUFFER_SIZE];

// Buffer to accumulate incoming commands
static uint8_t islInBuffer[ISL_BUFFER_SIZE];

// Actual RC channels coming in from radio
static uint16_t rcin[8];

// Override commands for RC channels
static uint16_t rccmd[8];

// Next index in islInBuffer to write incoming bytes.  
// When this reaches ISL_BUFFER_SIZE, the buffer is full and should be processed.
static int     islInBufferCursor = 0;

// Next index to write telemetry data.  Resets to 4, right after the header
static int     islOutBufferCursor = 4;

// Timestamp for last telemetry processing (micros)
static uint32_t lastISLMessage = 0;

// Timeout counter for incoming commands.  See function checkCommandTimeout for details
static int islCommandTimeout = 0;

// Reusing function defined in rx/jetiexbus.c
uint16_t calcCRC16(uint8_t *buf, uint8_t len);

static void addData(uint32_t data, int bytes)
{
	// Prevent accidental buffer overrun
	if ((islOutBufferCursor+bytes) > ISL_BUFFER_SIZE) return;
	
	for(int i=0;i<bytes;++i)
	{
		islOutBuffer[islOutBufferCursor++]=(data&0xFF);
		data=(data>>8);
	}
}

static void islSerialWriteBuffer()
{
	// Clear unused part of the buffer
	while(islOutBufferCursor<(ISL_BUFFER_SIZE-2))
		islOutBuffer[islOutBufferCursor++]=0;
	islOutBufferCursor=(ISL_BUFFER_SIZE-2);
	
	// Calculate CRC of payload (skip 4 header bytes, ignore last 2 crc bytes)
	uint16_t crc=calcCRC16(islOutBuffer+4,ISL_BUFFER_SIZE-6);
	addData(crc,sizeof(uint16_t));
	
	// Write to serial
    for (int i = 0; i < ISL_BUFFER_SIZE; i++)
        serialWrite(islPort, islOutBuffer[i]);
}

typedef struct isl_command_s {
	uint8_t header[4];
	uint16_t rccmd[8];
} isl_command_t;

typedef struct isl_telemetry_s {
	uint8_t  header[4];         //  4 bytes header
	
	uint32_t acc[3];            // 12 bytes  \ 4 byte data types first
	float    gyro[3];           // 12 bytes  / to avoid alignment problems
	uint16_t roll,pitch,yaw;    //  6 bytes
	uint16_t rcin[8];           // 16 bytes
	uint16_t rssi;              //  2 bytes
	uint16_t altitude;          //  2 bytes
	////////////////////// Total = 50 bytes for payload
	uint8_t  ecc[8];
	uint16_t crc;               // 10 bytes total suffix
	                            //
								// Total 4 + 50 + 10 = 64 bytes
} isl_telemetry_t;

// Following line checks that the struct is 64 bytes at compile time.
static const uint8_t static_check_struct_size[sizeof(isl_telemetry_t)==64?1:-1];

static void processISLTelemetry(void)
{
    isl_telemetry_t* t=(isl_telemetry_t*)islOutBuffer;
    for(int i=0;i<4;++i)
        t->header[i]=MAGIC[i];
    t->roll=attitude.values.roll;
    t->pitch=attitude.values.pitch;
    t->yaw=attitude.values.yaw;
    for(int i=0;i<3;++i) 
        t->acc[i]=acc.accSmooth[i];
    for(int i=0;i<3;++i) 
        t->gyro[i]=gyro.gyroADCf[i];
    for(int i=0;i<8;++i) 
        t->rcin[i]=rcin[i];
    t->rssi = raw_rssi;
    t->altitude = 0;
    const uint8_t* payload=islOutBuffer+4;
    // Add simple XOR based error correction code
    for(int i=0;i<8;++i) t->ecc[i]=0;
    for(int i=0;i<50;++i)
        t->ecc[i/8]^=payload[i];
    islOutBufferCursor=sizeof(isl_telemetry_t);
	islSerialWriteBuffer();
}

static void analyzeIncomingData(void)
{
	uint16_t crc=calcCRC16(islInBuffer+4,ISL_BUFFER_SIZE-6);
	if (( crc    &0xFF) != islInBuffer[ISL_BUFFER_SIZE-2]) return; // Invalid CRC
	if (((crc>>8)&0xFF) != islInBuffer[ISL_BUFFER_SIZE-1]) return; // Invalid CRC
	
	// Reset timeout counter, since we just got a valid command
	islCommandTimeout=0;
	
	isl_command_t* c=(isl_command_t*)islInBuffer;
	for(int i=0;i<8;++i)
	{
		// 0xFFFF means no change to existing value
		if (c->rccmd[i]!=0xFFFF)
			rccmd[i]=c->rccmd[i];
	}
}

// This is a safety function.  If no command has arrived in the past second,
// remove all RC override, to avoid cases where a companion computer crashes
// and you cannot resume manual control.
static void checkCommandTimeout(void)
{
	if (++islCommandTimeout>=100) // Function called at 100Hz,  100 * 10ms = 1 second
	{
		islCommandTimeout=0;
		for(int i=0;i<8;++i)
			rccmd[i]=0;
	}
}

static void processIncomingData(void)
{
	checkCommandTimeout();
	uint32_t avail = serialRxBytesWaiting(islPort);
	for(uint32_t i=0;i<avail;++i)
	{
		islInBuffer[islInBufferCursor]=serialRead(islPort);
		if (islInBufferCursor<4)
		{
			if (islInBuffer[islInBufferCursor] == MAGIC[islInBufferCursor])
				++islInBufferCursor;
			else
				islInBufferCursor=0;
		}
		else
		{
			// We're past the header
			if (++islInBufferCursor == ISL_BUFFER_SIZE)
			{
				// Buffer is full, analyze it
				analyzeIncomingData();
				// Reset cursor for next buffer
				islInBufferCursor=0;
			}
		}
	}
}

uint16_t applyRxChannelOverride(int channel, uint16_t sample)
{
	if (!islTelemetryEnabled) return sample;
	if (channel<0 || channel>=8) return sample;
	// Store actual sample for telemetry
	rcin[channel]=sample;
	// If there is override, use that
	if (rccmd[channel]>0) return rccmd[channel];
	// Default is no action
	return sample;
}

void initISLTelemetry(void)
{
    portConfig = findSerialPortConfig(FUNCTION_TELEMETRY_ISL);
    islPortSharing = determinePortSharing(portConfig, FUNCTION_TELEMETRY_ISL);
	for(int i=0;i<8;++i)
	{
		rcin[i]=0;
		rccmd[i]=0;
	}
}

void handleISLTelemetry(void)
{
    if (!islTelemetryEnabled) return;
    if (!islPort) return;

    uint32_t now = micros();
    if ((now - lastISLMessage) >= TELEMETRY_ISL_DELAY) {
		processIncomingData();
        processISLTelemetry();
        lastISLMessage = now;
    }
}

void checkISLTelemetryState(void)
{
    if (portConfig && telemetryCheckRxPortShared(portConfig)) {
        if (!islTelemetryEnabled && telemetrySharedPort != NULL) {
            islPort = telemetrySharedPort;
            islTelemetryEnabled = true;
        }
    } else {
        bool newTelemetryEnabledValue = telemetryDetermineEnabledState(islPortSharing);

        if (newTelemetryEnabledValue == islTelemetryEnabled) return;

        if (newTelemetryEnabledValue)
            configureISLTelemetryPort();
        else
            freeISLTelemetryPort();
    }
}

void freeISLTelemetryPort(void)
{
    closeSerialPort(islPort);
    islPort = NULL;
    islTelemetryEnabled = false;
}

void configureISLTelemetryPort(void)
{
    if (!portConfig) return;

    baudRate_e baudRateIndex = portConfig->telemetry_baudrateIndex;
    if (baudRateIndex == BAUD_AUTO) {
        baudRateIndex = BAUD_115200;
    }

    islPort = openSerialPort(portConfig->identifier, FUNCTION_TELEMETRY_ISL, NULL, 
	                         baudRates[baudRateIndex], MODE_RXTX, SERIAL_NOT_INVERTED);

    if (!islPort) return;

    islTelemetryEnabled = true;
}


#endif

#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>

template<class II>
uint8_t calc_xor(II b, II e, uint8_t v)
{
  for (auto i = b; i != e; ++i)
    v ^= *i;
  return v;
}

typedef union header_t
{
  unsigned magic;
  uint8_t  magic_bytes[4];
} header;

const header MAGIC = { 0xFEEDBEEF };

std::mt19937 engine(1);
std::normal_distribution<double> gaussian;
std::uniform_real_distribution<double> uniform;

#define G gaussian(engine)
#define U uniform(engine)

#pragma pack(1)

uint16_t crc16(const uint8_t* data_p, uint8_t length) {
  uint8_t x;
  uint16_t crc = 0xFFFF;

  while (length--) {
    x = crc >> 8 ^ *data_p++;
    x ^= x >> 4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
  }
  return crc;
}

typedef struct telemetry_t
{
  uint32_t acc[3];            // 12 bytes
  float    gyro[3];           // 12 bytes
  uint16_t rcin[8];           // 16 bytes
  uint16_t roll, pitch, yaw;  //  6 bytes
  uint16_t rssi;              //  2 bytes
  uint16_t reserved;          //  2 bytes
  uint8_t  ecc[8];            //  8 bytes for a total of 58
} telemetry;

typedef struct telemetry_buffer_t
{
  header prefix;
  telemetry payload;
  uint16_t crc;
} telemetry_buffer;

void calculate_crc(telemetry_buffer& b)
{
  b.crc = crc16((const uint8_t*)&b.payload, sizeof(b.payload));
}

void generate(telemetry_buffer& buffer)
{
  buffer.prefix.magic = MAGIC.magic;
  for (int i = 0; i < 3; ++i)
  {
    buffer.payload.acc[i] = uint32_t(fabs(G*1500));
    buffer.payload.gyro[i] = float(G);
  }
  for (int i = 0; i < 8; ++i)
    buffer.payload.rcin[i] = uint16_t(fabs(G*500));
  buffer.payload.roll = uint16_t(fabs(G*500));
  buffer.payload.pitch = uint16_t(fabs(G*500));
  buffer.payload.yaw = uint16_t(fabs(G*500));
  buffer.payload.rssi = uint16_t(fabs(G*500));
  buffer.payload.reserved = 0;
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&buffer.payload);
  int n = sizeof(buffer.payload);
  int j = 0;
  for (int i = 0; i < 8; ++i)
    buffer.payload.ecc[i] = 0;
  for (int i = 0; i < n; ++i)
    buffer.payload.ecc[i / 8] ^= ptr[i];
  calculate_crc(buffer);
}

uint8_t noise(uint8_t b, double noise_thres)
{
  if (U < noise_thres)
  {
    int bit = int(U * 8);
    if (bit >= 8) bit = 7;
    return b ^ (1 << bit);
  }
  return b;
}

class Receiver
{
  uint8_t buffer[64];
  int     cur;

  int successful;
  int failed_crc;
  int thrown_bytes;

  std::string perc(int n)
  {
    double value = (n * 100.0) / ((successful + failed_crc) * 64 + thrown_bytes);
    std::ostringstream os;
    os << "  (" << std::setprecision(3) << value << "%)";
    return os.str();
  }

  bool correct_error(telemetry_buffer* tb)
  {
    uint16_t orig_crc = tb->crc;
    uint8_t* ptr = reinterpret_cast<uint8_t*>(tb);
    int n = sizeof(tb->payload);
    int j = 0;
    for (int i = 0; i < 8; ++i)
    {
      if (j<(n-8))
      {
        uint8_t sum=calc_xor(ptr + j, ptr + j + 8, 0);
        if (sum != tb->payload.ecc[i])
        {
          for (int k = 0; k < 8; ++k)
          {
            uint8_t cand = sum^ptr[j + k] ^ tb->payload.ecc[i];
            std::swap(ptr[j + k], cand);
            calculate_crc(*tb);
            if (tb->crc == orig_crc) return true;
            std::swap(ptr[j + k], cand);
          }
        }
      }
    }
    return false;
  }

  void analyze_buffer()
  {
    telemetry_buffer* tb = reinterpret_cast<telemetry_buffer*>(buffer);
    uint16_t crc=tb->crc;
    calculate_crc(*tb);
    if (crc != tb->crc)
    {
      if (correct_error(tb)) ++successful; else
        ++failed_crc;
    }
    else
      ++successful;
  }
public:
  Receiver() 
  : cur(0)
  , successful(0)
  , failed_crc(0)
  , thrown_bytes(0)
  {}

  double get_success_perc() const
  {
    return (successful*64.0*100.0) / ((successful + failed_crc) * 64 + thrown_bytes);
  }

  void print()
  {
    std::cout << "Succesful buffers: " << successful << perc(successful * 64) << std::endl;
    std::cout << "Failed buffers:    " << failed_crc << perc(failed_crc * 64)<< std::endl;
    std::cout << "Thrown bytes:      " << thrown_bytes << perc(thrown_bytes) << std::endl;
  }

  void add_byte(uint8_t b)
  {
    buffer[cur] = b;
    if (cur < 4)
    {
      for (int i = 0; i <= cur; ++i)
      {
        if (buffer[i] != MAGIC.magic_bytes[i])
        {
          thrown_bytes += (cur+1);
          cur = 0;
          return;
        }
      }
    }
    if (++cur == 64)
    {
      analyze_buffer();
      cur = 0;
    }
  }
};

int main(int argc, char* argv[])
{
  std::cout << "Header size: " << sizeof(header) << std::endl;
  std::cout << "Buffer size: " << sizeof(telemetry_buffer) << std::endl;
  for (double noise_thres = 0.001; noise_thres < 0.05; noise_thres += 0.001)
  {
    Receiver rcv;
    for (int i = 0; i < 10000; ++i)
    {
      telemetry_buffer tb;
      generate(tb);
      const uint8_t* buffer = reinterpret_cast<const uint8_t*>(&tb);
      for (int j = 0; j < 64; ++j)
        rcv.add_byte(noise(buffer[j],noise_thres));
    }
    std::cout << noise_thres << "\t\t" << rcv.get_success_perc() << std::endl;
  }
  //rcv.print();
  return 0;
}

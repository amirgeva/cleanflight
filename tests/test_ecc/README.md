The test included in this directory simulates the transmission and reception of telemetry packets, 
with a varying rate of probability for a bit flip due to noise.

The noise threshold indicates the probability for any byte to have a single bit flipped,
which should be a good approximation of the effects of noise on the UART lines.

A simple test without any error correction when sending packets 
from a NAZE32 Rev6 board to the PC using a UART dongle and simple breadboard dupont wires,
shows a rate of about 30% packet loss due to CRC errors.

A simulation using the code in this directory shows that this is about 0.7% probability 
that any byte has to have one of its bits flipped.

After implementation of the error correction, the results of successful packets receive percentage is as follows:
Noise Threshold   Success Rate %
0.001           99.56
0.002           99.15
0.003           98.62
0.004           98.34
0.005           97.99
0.006           97.62
0.007           97.39
0.008           96.77
0.009           96.59
0.01            96.02
0.011           95.48
0.012           95.23
0.013           94.76
0.014           94.57
0.015           93.96
0.016           93.93
0.017           93.17
0.018           93.45
0.019           92.73
0.02            92.3
0.021           91.2
0.022           91.51
0.023           91.08
0.024           90.93
0.025           90.95
0.026           90.06
0.027           89.17
0.028           88.9
0.029           89.32
0.03            88.91
0.031           88.02
0.032           87.87
0.033           87.53
0.034           87.23
0.035           86.65
0.036           86.28
0.037           86.29
0.038           85.07
0.039           85.37
0.04            84.93
0.041           84.75
0.042           84.45
0.043           83.29
0.044           83.72
0.045           83.37
0.046           83.3
0.047           82.74
0.048           82.31
0.049           81.47

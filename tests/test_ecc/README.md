# Noise Error Correction Analysis

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

| Noise Threshold % | Success Rate % |
| ----------------- | -------------- |
|        0.1        |    99.56       |
|        0.2        |    99.15       |
|        0.3        |    98.62       |
|        0.4        |    98.34       |
|        0.5        |    97.99       |
|        0.6        |    97.62       |
|        0.7        |    97.39       |
|        0.8        |    96.77       |
|        0.9        |    96.59       |
|        1.0        |    96.02       |
|        1.1        |    95.48       |
|        1.2        |    95.23       |
|        1.3        |    94.76       |
|        1.4        |    94.57       |
|        1.5        |    93.96       |
|        1.6        |    93.93       |
|        1.7        |    93.17       |
|        1.8        |    93.45       |
|        1.9        |    92.73       |
|        2.0        |    92.30       |
|        2.1        |    91.20       |
|        2.2        |    91.51       |
|        2.3        |    91.08       |
|        2.4        |    90.93       |
|        2.5        |    90.95       |
|        2.6        |    90.06       |
|        2.7        |    89.17       |
|        2.8        |    88.90       |
|        2.9        |    89.32       |
|        3.0        |    88.91       |
|        3.1        |    88.02       |
|        3.2        |    87.87       |
|        3.3        |    87.53       |
|        3.4        |    87.23       |
|        3.5        |    86.65       |
|        3.6        |    86.28       |
|        3.7        |    86.29       |
|        3.8        |    85.07       |
|        3.9        |    85.37       |
|        4.0        |    84.93       |
|        4.1        |    84.75       |
|        4.2        |    84.45       |
|        4.3        |    83.29       |
|        4.4        |    83.72       |
|        4.5        |    83.37       |
|        4.6        |    83.30       |
|        4.7        |    82.74       |
|        4.8        |    82.31       |
|        4.9        |    81.47       |

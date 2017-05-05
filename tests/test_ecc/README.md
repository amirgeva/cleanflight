# Noise Error Correction Analysis

The test included in this directory simulates the transmission and reception of telemetry packets, 
with a varying rate of probability for a bit flip due to noise.

The noise threshold indicates the probability for any byte to have a single bit flipped,
which should be a good approximation of the effects of noise on the UART lines.

A simple test without any error correction when sending packets 
from a NAZE32 Rev6 board to the PC using a UART dongle and simple breadboard dupont wires,
shows a rate of about 30% packet loss due to CRC errors.

A simulation using the code in this directory shows that this is about 0.6% probability 
that any byte has to have one of its bits flipped.

After implementation of the error correction, the results of successful packets receive percentage is in the table below,
and the 30% loss from before is reduced to 2.6%

| Noise Threshold % | Success Rate % without ECC | Success Rate % with ECC |
| ----------------- | -------------------------- | ----------------------- |
|       0.001       |           94.05            |          99.68          |
|       0.002       |            87.7            |          99.24          |
|       0.003       |           82.63            |          98.81          |
|       0.004       |           78.42            |          98.53          |
|       0.005       |            72.8            |          98.02          |
|       0.006       |           68.17            |          97.4           |
|       0.007       |           63.97            |          96.79          |
|       0.008       |           59.77            |          96.83          |
|       0.009       |           56.35            |          96.57          |
|       0.01        |           52.62            |          96.06          |
|       0.011       |           49.36            |          95.55          |
|       0.012       |           46.26            |          95.46          |
|       0.013       |             43             |          94.89          |
|       0.014       |           40.06            |          94.31          |
|       0.015       |           38.41            |          94.34          |
|       0.016       |           35.86            |          93.35          |
|       0.017       |           32.39            |          92.76          |
|       0.018       |           31.02            |          92.89          |
|       0.019       |           29.65            |          92.51          |
|       0.02        |           27.26            |          91.8           |
|       0.021       |           25.75            |          92.46          |
|       0.022       |            24.2            |          90.77          |
|       0.023       |           23.09            |          90.92          |
|       0.024       |           21.89            |          90.61          |
|       0.025       |           20.05            |          90.61          |
|       0.026       |           17.81            |          90.6           |
|       0.027       |           17.65            |          89.73          |
|       0.028       |           16.55            |          89.42          |
|       0.029       |           14.98            |          89.04          |
|       0.03        |           14.59            |          88.68          |
|       0.031       |           13.13            |          88.15          |
|       0.032       |           12.03            |          87.72          |
|       0.033       |           12.02            |          86.76          |
|       0.034       |           10.77            |          87.84          |
|       0.035       |            9.92            |          87.25          |
|       0.036       |            9.46            |          86.33          |
|       0.037       |            8.7             |          85.76          |
|       0.038       |            8.82            |          85.55          |
|       0.039       |            7.84            |          85.41          |
|       0.04        |            7.53            |          85.5           |
|       0.041       |            6.72            |          84.68          |
|       0.042       |            6.26            |          84.59          |
|       0.043       |            6.04            |          84.11          |
|       0.044       |            5.5             |          83.46          |
|       0.045       |            5.33            |          82.95          |
|       0.046       |            4.64            |          82.77          |
|       0.047       |            4.43            |          82.24          |
|       0.048       |            4.05            |          82.58          |
|       0.049       |            3.81            |          82.26          |
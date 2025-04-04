#pragma once

#define UART2_RX 16
#define UART2_TX 17

#define APA102_CLOCK 18
#define APA102_DATA 4

#define DMX512_MAX 512 // Maximum number of channels on a DMX512 universe.

#define REPORT_TASK_INTERVAL 120 * 1000 // How often to report task status in milliseconds

/*
    Options include:
        9600
        19200
        28800
        38400
        57600
        76800
        115200
        230400
        460800
        576000
        921600
*/

#define NOVANET_BAUD 921600

// LEDC (PWM) Configuration
#define LEDC_FREQ_HZ      5000
#define LEDC_RESOLUTION   8       // 8-bit resolution (0-255)
#define LEDC_FULL_DUTY    255     // Full brightness duty cycle
#define LEDC_DIM_DUTY     3      // 10% brightness duty cycle (255 * 0.10)
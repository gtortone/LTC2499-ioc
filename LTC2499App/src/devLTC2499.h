#ifndef LTC2499_H
#define LTC2499_H

#include <stdint.h>
#include <linux/i2c-dev.h>

/* Single-Ended Channels Configuration */
#define LTC2499_CH0            0xB0
#define LTC2499_CH1            0xB8
#define LTC2499_CH2            0xB1
#define LTC2499_CH3            0xB9
#define LTC2499_CH4            0xB2
#define LTC2499_CH5            0xBA
#define LTC2499_CH6            0xB3
#define LTC2499_CH7            0xBB
#define LTC2499_CH8            0xB4
#define LTC2499_CH9            0xBC
#define LTC2499_CH10           0xB5
#define LTC2499_CH11           0xBD
#define LTC2499_CH12           0xB6
#define LTC2499_CH13           0xBE
#define LTC2499_CH14           0xB7
#define LTC2499_CH15           0xBF

static char chan_se_config[16] = { LTC2499_CH0, LTC2499_CH1, LTC2499_CH2, LTC2499_CH3, LTC2499_CH4, LTC2499_CH5, LTC2499_CH6, LTC2499_CH7, LTC2499_CH8, LTC2499_CH9, \
	LTC2499_CH10, LTC2499_CH11, LTC2499_CH12, LTC2499_CH13, LTC2499_CH14, LTC2499_CH15 };

/* Differential Channels Configuration */
#define LTC2499_P0_N1          0xA0
#define LTC2499_P1_N0          0xA8
#define LTC2499_P2_N3          0xA1
#define LTC2499_P3_N2          0xA9
#define LTC2499_P4_N5          0xA2
#define LTC2499_P5_N4          0xAA
#define LTC2499_P6_N7          0xA3
#define LTC2499_P7_N6          0xAB
#define LTC2499_P8_N9          0xA4
#define LTC2499_P9_N8          0xAC
#define LTC2499_P10_N11        0xA5
#define LTC2499_P11_N10        0xAD
#define LTC2499_P12_N13        0xA6
#define LTC2499_P13_N12        0xAE
#define LTC2499_P14_N15        0xA7
#define LTC2499_P15_N14        0xAF

static char chan_diff_config[16] = { LTC2499_P0_N1, LTC2499_P1_N0, LTC2499_P2_N3, LTC2499_P3_N2, LTC2499_P4_N5, LTC2499_P5_N4, LTC2499_P6_N7, LTC2499_P7_N6, LTC2499_P8_N9, \
	LTC2499_P9_N8, LTC2499_P10_N11, LTC2499_P11_N10, LTC2499_P12_N13, LTC2499_P13_N12, LTC2499_P14_N15, LTC2499_P15_N14 };

/* Mode Configuration for High Speed Family */
#define LTC2499_KEEP_PREVIOUS_MODE              0x80
#define LTC2499_KEEP_PREVIOUS_SPEED_RESOLUTION  0x00
#define LTC2499_SPEED_1X                        0x00
#define LTC2499_SPEED_2X                        0x08
#define LTC2499_INTERNAL_TEMP                   0xC0

/* Select rejection frequency - 50, 55, or 60Hz */
#define LTC2499_R50             0b10010000
#define LTC2499_R50_R60         0b10000000
#define LTC2499_R60             0b10100000

#endif

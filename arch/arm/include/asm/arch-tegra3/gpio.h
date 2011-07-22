/*
 * Copyright (c) 2011, Google Inc. All rights reserved.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _TEGRA_GPIO_H_
#define _TEGRA_GPIO_H_

/*
 * The Tegra 2x GPIO controller has 224 GPIOs arranged in 7 banks of 4 ports,
 * each with 8 GPIOs.
 * The Tegra 3x GPIO controller has 246 GPIOS in 8 banks of 4 ports.
 */
#define TEGRA_GPIO_PORTS 4   /* The number of ports per bank */
#define TEGRA_GPIO_BANKS 8   /* The number of banks */

/* GPIO Controller registers for a single bank */
struct gpio_ctlr_bank {
	uint gpio_config[TEGRA_GPIO_PORTS];
	uint gpio_dir_out[TEGRA_GPIO_PORTS];
	uint gpio_out[TEGRA_GPIO_PORTS];
	uint gpio_in[TEGRA_GPIO_PORTS];
	uint gpio_int_status[TEGRA_GPIO_PORTS];
	uint gpio_int_enable[TEGRA_GPIO_PORTS];
	uint gpio_int_level[TEGRA_GPIO_PORTS];
	uint gpio_int_clear[TEGRA_GPIO_PORTS];
#if defined(CONFIG_TEGRA3)
	uint gpio_masked_config[TEGRA_GPIO_PORTS];
	uint gpio_masked_dir_out[TEGRA_GPIO_PORTS];
	uint gpio_masked_out[TEGRA_GPIO_PORTS];
	uint gpio_masked_in[TEGRA_GPIO_PORTS];
	uint gpio_masked_int_status[TEGRA_GPIO_PORTS];
	uint gpio_masked_int_enable[TEGRA_GPIO_PORTS];
	uint gpio_masked_int_level[TEGRA_GPIO_PORTS];
	uint gpio_masked_int_clear[TEGRA_GPIO_PORTS];
#endif
};

struct gpio_ctlr {
	struct gpio_ctlr_bank gpio_bank[TEGRA_GPIO_BANKS];
};

#define GPIO_BANK(x)	((x) >> 5)
#define GPIO_PORT(x)	(((x) >> 3) & 0x3)
#define GPIO_PORT8(x)	((x) >> 3)
#define GPIO_BIT(x)	((x) & 0x7)

#define GPIO_PA0    0		/* port A (0), pin 0 */
#define GPIO_PA1    1
#define GPIO_PA2    2
#define GPIO_PA3    3
#define GPIO_PA4    4
#define GPIO_PA5    5
#define GPIO_PA6    6
#define GPIO_PA7    7
#define GPIO_PB0    8
#define GPIO_PB1    9
#define GPIO_PB2    10
#define GPIO_PB3    11
#define GPIO_PB4    12
#define GPIO_PB5    13
#define GPIO_PB6    14
#define GPIO_PB7    15
#define GPIO_PC0    16
#define GPIO_PC1    17
#define GPIO_PC2    18
#define GPIO_PC3    19
#define GPIO_PC4    20
#define GPIO_PC5    21
#define GPIO_PC6    22
#define GPIO_PC7    23
#define GPIO_PD0    24
#define GPIO_PD1    25
#define GPIO_PD2    26
#define GPIO_PD3    27
#define GPIO_PD4    28
#define GPIO_PD5    29
#define GPIO_PD6    30
#define GPIO_PD7    31
#define GPIO_PE0    32
#define GPIO_PE1    33
#define GPIO_PE2    34
#define GPIO_PE3    35
#define GPIO_PE4    36
#define GPIO_PE5    37
#define GPIO_PE6    38
#define GPIO_PE7    39
#define GPIO_PF0    40
#define GPIO_PF1    41
#define GPIO_PF2    42
#define GPIO_PF3    43
#define GPIO_PF4    44
#define GPIO_PF5    45
#define GPIO_PF6    46
#define GPIO_PF7    47
#define GPIO_PG0    48
#define GPIO_PG1    49
#define GPIO_PG2    50
#define GPIO_PG3    51
#define GPIO_PG4    52
#define GPIO_PG5    53
#define GPIO_PG6    54
#define GPIO_PG7    55
#define GPIO_PH0    56
#define GPIO_PH1    57
#define GPIO_PH2    58
#define GPIO_PH3    59
#define GPIO_PH4    60
#define GPIO_PH5    61
#define GPIO_PH6    62
#define GPIO_PH7    63
#define GPIO_PI0    64
#define GPIO_PI1    65
#define GPIO_PI2    66
#define GPIO_PI3    67
#define GPIO_PI4    68
#define GPIO_PI5    69
#define GPIO_PI6    70
#define GPIO_PI7    71
#define GPIO_PJ0    72
#define GPIO_PJ1    73
#define GPIO_PJ2    74
#define GPIO_PJ3    75
#define GPIO_PJ4    76
#define GPIO_PJ5    77
#define GPIO_PJ6    78
#define GPIO_PJ7    79
#define GPIO_PK0    80
#define GPIO_PK1    81
#define GPIO_PK2    82
#define GPIO_PK3    83
#define GPIO_PK4    84
#define GPIO_PK5    85
#define GPIO_PK6    86
#define GPIO_PK7    87
#define GPIO_PL0    88
#define GPIO_PL1    89
#define GPIO_PL2    90
#define GPIO_PL3    91
#define GPIO_PL4    92
#define GPIO_PL5    93
#define GPIO_PL6    94
#define GPIO_PL7    95
#define GPIO_PM0    96
#define GPIO_PM1    97
#define GPIO_PM2    98
#define GPIO_PM3    99
#define GPIO_PM4    100
#define GPIO_PM5    101
#define GPIO_PM6    102
#define GPIO_PM7    103
#define GPIO_PN0    104
#define GPIO_PN1    105
#define GPIO_PN2    106
#define GPIO_PN3    107
#define GPIO_PN4    108
#define GPIO_PN5    109
#define GPIO_PN6    110
#define GPIO_PN7    111
#define GPIO_PO0    112
#define GPIO_PO1    113
#define GPIO_PO2    114
#define GPIO_PO3    115
#define GPIO_PO4    116
#define GPIO_PO5    117
#define GPIO_PO6    118
#define GPIO_PO7    119
#define GPIO_PP0    120
#define GPIO_PP1    121
#define GPIO_PP2    122
#define GPIO_PP3    123
#define GPIO_PP4    124
#define GPIO_PP5    125
#define GPIO_PP6    126
#define GPIO_PP7    127
#define GPIO_PQ0    128
#define GPIO_PQ1    129
#define GPIO_PQ2    130
#define GPIO_PQ3    131
#define GPIO_PQ4    132
#define GPIO_PQ5    133
#define GPIO_PQ6    134
#define GPIO_PQ7    135
#define GPIO_PR0    136
#define GPIO_PR1    137
#define GPIO_PR2    138
#define GPIO_PR3    139
#define GPIO_PR4    140
#define GPIO_PR5    141
#define GPIO_PR6    142
#define GPIO_PR7    143
#define GPIO_PS0    144
#define GPIO_PS1    145
#define GPIO_PS2    146
#define GPIO_PS3    147
#define GPIO_PS4    148
#define GPIO_PS5    149
#define GPIO_PS6    150
#define GPIO_PS7    151
#define GPIO_PT0    152
#define GPIO_PT1    153
#define GPIO_PT2    154
#define GPIO_PT3    155
#define GPIO_PT4    156
#define GPIO_PT5    157
#define GPIO_PT6    158
#define GPIO_PT7    159
#define GPIO_PU0    160
#define GPIO_PU1    161
#define GPIO_PU2    162
#define GPIO_PU3    163
#define GPIO_PU4    164
#define GPIO_PU5    165
#define GPIO_PU6    166
#define GPIO_PU7    167
#define GPIO_PV0    168
#define GPIO_PV1    169
#define GPIO_PV2    170
#define GPIO_PV3    171
#define GPIO_PV4    172
#define GPIO_PV5    173
#define GPIO_PV6    174
#define GPIO_PV7    175
#define GPIO_PW0    176
#define GPIO_PW1    177
#define GPIO_PW2    178
#define GPIO_PW3    179
#define GPIO_PW4    180
#define GPIO_PW5    181
#define GPIO_PW6    182
#define GPIO_PW7    183
#define GPIO_PX0    184
#define GPIO_PX1    185
#define GPIO_PX2    186
#define GPIO_PX3    187
#define GPIO_PX4    188
#define GPIO_PX5    189
#define GPIO_PX6    190
#define GPIO_PX7    191
#define GPIO_PY0    192
#define GPIO_PY1    193
#define GPIO_PY2    194
#define GPIO_PY3    195
#define GPIO_PY4    196
#define GPIO_PY5    197
#define GPIO_PY6    198
#define GPIO_PY7    199
#define GPIO_PZ0    200
#define GPIO_PZ1    201
#define GPIO_PZ2    202
#define GPIO_PZ3    203
#define GPIO_PZ4    204
#define GPIO_PZ5    205
#define GPIO_PZ6    206
#define GPIO_PZ7    207
#define GPIO_PAA0   208
#define GPIO_PAA1   209
#define GPIO_PAA2   210
#define GPIO_PAA3   211
#define GPIO_PAA4   212
#define GPIO_PAA5   213
#define GPIO_PAA6   214
#define GPIO_PAA7   215
#define GPIO_PBA0   216
#define GPIO_PBB1   217
#define GPIO_PBB2   218
#define GPIO_PBB3   219
#define GPIO_PBB4   220
#define GPIO_PBB5   221
#define GPIO_PBB6   222
#define GPIO_PBB7   223
#if defined(CONFIG_TEGRA3)
#define GPIO_PCC0   224
#define GPIO_PCC1   225
#define GPIO_PCC2   226
#define GPIO_PCC3   227
#define GPIO_PCC4   228
#define GPIO_PCC5   229
#define GPIO_PCC6   230
#define GPIO_PCC7   231
#define GPIO_PDD0   232
#define GPIO_PDD1   233
#define GPIO_PDD2   234
#define GPIO_PDD3   235
#define GPIO_PDD4   236
#define GPIO_PDD5   237
#define GPIO_PDD6   238
#define GPIO_PDD7   239
#define GPIO_PEE0   240
#define GPIO_PEE1   241
#define GPIO_PEE2   242
#define GPIO_PEE3   243
#define GPIO_PEE4   244
#define GPIO_PEE5   245
#define GPIO_PEE6   246
#define GPIO_PEE7   247
#endif
/*
 * Tegra-specific GPIO API
 */

int gpio_direction_input(int gp);
int gpio_direction_output(int gp, int value);
int gpio_get_value(int gp);
void gpio_set_value(int gp, int value);

#endif	/* TEGRA_GPIO_H_ */

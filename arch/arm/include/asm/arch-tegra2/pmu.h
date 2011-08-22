/*
 *  (C) Copyright 2010,2011
 *  NVIDIA Corporation <www.nvidia.com>
 *
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

#ifndef _PMU_H_
#define _PMU_H_

/*
 * abs() handles unsigned ints, shorts and chars and returns a signed long.
 */
#define abs(x) ({						\
		long ret;					\
		{						\
			typeof((x)) __x = (x);			\
			ret = (__x < 0) ? -__x : __x;		\
		}						\
		ret;						\
	})

#define stp(x, y) ((x < y) ? VDD_TRANSITION_STEP : -VDD_TRANSITION_STEP)

#define DVC_I2C_BUS_NUMBER	0
#define PMU_I2C_ADDRESS		0x34

#define PMU_CORE_VOLTAGE_REG	0x26
#define PMU_CPU_VOLTAGE_REG	0x23
#define PMU_SUPPLY_CONTROL_REG1	0x20
#define PMU_SUPPLY_CONTROL_REG2	0x21
#define VDD_CPU_SUPPLY_CONTROL	0x01
#define VDD_CPU_SUPPLY2_SEL	0x02
#define VDD_CORE_SUPPLY_CONTROL	0x04
#define VDD_CORE_SUPPLY2_SEL	0x08

#define VDD_CORE_NOMINAL_T25	0x17	/* 1.3v */
#define VDD_CPU_NOMINAL_T25	0x10	/* 1.125v */

#define VDD_CORE_NOMINAL_T20	0x16	/* 1.275v */
#define VDD_CPU_NOMINAL_T20	0x0f	/* 1.1v */

#define VDD_RELATION		0x02	/*  50mv */
#define VDD_TRANSITION_STEP	0x06	/* 150mv */
#define VDD_TRANSITION_RATE	0x07

#define MAX_I2C_RETRY	3

int tegra2_pmu_is_voltage_nominal(void);

#endif	/* PMU_H */

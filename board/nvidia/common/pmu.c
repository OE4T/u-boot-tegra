/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010,2011 NVIDIA Corporation <www.nvidia.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/tegra2.h>
#include <asm/arch/sys_proto.h>

#include <asm/arch/pmu.h>
#include <asm/arch/pmc.h>
#include <i2c.h>

struct vdd_settings {
	int	data;
	int	nominal;
};

enum vdd_type {
	core = 0,
	cpu = 1,
};

static struct vdd_settings vdd_current[] = {
	/* vdd core */
	{.data		= 0,
	 .nominal	= 0,
	},

	/* vdd cpu */
	{.data		= 0,
	 .nominal	= 0,
	},
};

static int vdd_init_nominal_table(void)
{
	/* by default, the table has been filled with T25 settings */
	switch (tegra_get_chip_type()) {
	case TEGRA_SOC_T20:
		vdd_current[core].nominal = VDD_CORE_NOMINAL_T20;
		vdd_current[cpu].nominal = VDD_CPU_NOMINAL_T20;
		break;
	case TEGRA_SOC_T25:
		vdd_current[core].nominal = VDD_CORE_NOMINAL_T25;
		vdd_current[cpu].nominal = VDD_CPU_NOMINAL_T25;
		break;
	default:
		/* unknown chip type */
		return -1;
	}
	return 0;
}

#define MAX_I2C_RETRY	3
static int pmu_read(int reg)
{
	int	i;
	uchar	data;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_read(PMU_I2C_ADDRESS, reg, 0, &data, 1))
			return (int)data;

		/* i2c access failed, retry */
		udelay(100);
	}

	return -1;
}

static int pmu_write(int reg, uchar *data, uint len)
{
	int i;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_write(PMU_I2C_ADDRESS, reg, 0, data, len))
			return 0;

		/* i2c access failed, retry */
		udelay(100);
	}

	return -1;
}

/* get current vdd_core and vdd_cpu */
static int tegra2_get_voltage(void)
{
	int	reg;
	int	data1, data2;

	/*
	 * Each vdd has two supply sources, ie, v1 and v2.
	 * The supply control reg1 and reg2 determine the current selection.
	 */
	/* get supply control reg1 and reg2 */
	if ((data1 = pmu_read(PMU_SUPPLY_CONTROL_REG1)) == -1)
		return -1;

	if ((data2 = pmu_read(PMU_SUPPLY_CONTROL_REG2)) == -1)
		return -1;

	reg = PMU_CORE_VOLTAGE_REG;	/* set default to v1 */
	if ((data1 | data2) & VDD_CORE_SUPPLY2_SEL)
		reg++;	/* v2 is selected*/

	/* get vdd_core */
	if ((vdd_current[core].data = pmu_read(reg)) == -1)
		return -1;

	reg = PMU_CPU_VOLTAGE_REG;	/* set default to v1 */
	if ((data1 | data2) & VDD_CPU_SUPPLY2_SEL)
		reg++;	/* v2 is selected*/

	/* get vdd_cpu */
	if ((vdd_current[cpu].data = pmu_read(reg)) == -1)
		return -1;

	return 0;
}

static int tegra2_set_voltage(int reg, int data, int control)
{
	uchar	data_buffer[3];
	uchar	control_bit = (uchar)control;

	/*
	 * only one supply is needed in u-boot. set both v1 and v2 to
	 * same value.
	 *
	 * when both v1 and v2 are set to same value, we just need to set
	 * control1 reg to trigger the supply selection.
	 */
	data_buffer[0] = data_buffer[1] = (uchar)data;
	data_buffer[2] = VDD_TRANSITION_RATE;

	if (!pmu_write(reg, data_buffer, 3) &&		/* v1, v2 and rate */
	    !pmu_write(PMU_SUPPLY_CONTROL_REG1, &control_bit, 1)) /* trigger */
		return 0;
	return -1;
}

int tegra2_pmu_is_voltage_nominal(void)
{
	if ((vdd_current[core].data == vdd_current[core].nominal) &&
	    (vdd_current[cpu].data == vdd_current[cpu].nominal))
		return 1;
	return 0;
}

void calculate_next_voltage(int *data, int nominal, int step)
{
	if (abs(nominal - *data) > VDD_TRANSITION_STEP)
		*data += step;
	else
		*data = nominal;
}

/*
 * It is requird by tegra2 soc that vdd_core must be higher than vdd_cpu
 * with certain range at all time. If current settings doesn't meet this
 * condition, pmu_adjust_voltage just simply returns without setting any
 * voltage.
 */
static int pmu_adjust_voltage(void)
{
	int step_core, step_cpu;
	int adjust_vdd_core_late;

	/*
	 * if vdd_core < vdd_cpu + rel
	 *    skip
	 *
	 * This condition may happen when system reboots due to kernel crash.
	 */
	if (vdd_current[core].data < (vdd_current[cpu].data + VDD_RELATION))
		return -1;

	/*
	 * Since vdd_core and vdd_cpu may both stand at either greater or less
	 * than their nominal voltage, the adjustment may go either directions.
	 *
	 * Make sure vdd_core is always higher than vdd_cpu with certain margin.
	 * So, find out which vdd to adjust first in each step.
	 *
	 * case 1: both vdd_core and vdd_cpu need to move up
	 *              adjust vdd_core before vdd_cpu
	 *
	 * case 2: both vdd_core and vdd_cpu need to move down
	 *              adjust vdd_cpu before vdd_core
	 *
	 * case 3: vdd_core moves down and vdd_cpu moves up
	 *              adjusting either one first is fine.
	 */
	step_core = stp(vdd_current[core].data, vdd_current[core].nominal);
	step_cpu = stp(vdd_current[cpu].data, vdd_current[cpu].nominal);

	/*
	 * Adjust vdd_core and vdd_cpu one step at a time until they reach
	 * their nominal values.
	 */
	while ((vdd_current[core].data != vdd_current[core].nominal) ||
	       (vdd_current[cpu].data != vdd_current[cpu].nominal)) {

		adjust_vdd_core_late = 0;

		/* if vdd_core hasn't reached its nominal value? */
		if (vdd_current[core].data != vdd_current[core].nominal) {

			calculate_next_voltage(&vdd_current[core].data,
					       vdd_current[core].nominal,
					       step_core);

			/*
			 * if case 1 and case 3, set new vdd_core first.
			 * otherwise, hold down until new vdd_cpu is set.
			 */
			if (step_cpu > 0) {
				/* adjust vdd_core first */
				if (tegra2_set_voltage(PMU_CORE_VOLTAGE_REG,
						vdd_current[core].data,
						VDD_CORE_SUPPLY_CONTROL))
					return -1;
			} else
				/* set flag to adjust vdd_core later */
				adjust_vdd_core_late = 1;
		}

		/* if vdd_cpu hasn't reached its nominal value? */
		if (vdd_current[cpu].data != vdd_current[cpu].nominal) {

			calculate_next_voltage(&vdd_current[cpu].data,
					       vdd_current[cpu].nominal,
					       step_cpu);

			/* adjust vdd_cpu */
			if (tegra2_set_voltage(PMU_CPU_VOLTAGE_REG,
					       vdd_current[cpu].data,
					       VDD_CPU_SUPPLY_CONTROL))
				return -1;
		}

		/*
		 * if vdd_core late flag is set
		 */
		if (adjust_vdd_core_late) {
			/* adjust vdd_core */
			if (tegra2_set_voltage(PMU_CORE_VOLTAGE_REG,
					       vdd_current[core].data,
					       VDD_CORE_SUPPLY_CONTROL))
				return -1;
		}
	}
	return 0;
}

int pmu_set_nominal(void)
{
	/* fill in nominal values based on chip type */
	if (vdd_init_nominal_table())
		return -1;

	/* select current i2c bus to DVC */
	i2c_set_bus_num(DVC_I2C_BUS_NUMBER);

	/* get current voltage settings */
	if (tegra2_get_voltage())
		return -1;

	/* if current voltage is already set to nominal, skip */
	if (tegra2_pmu_is_voltage_nominal())
		return 0;

	/* adjust vdd_core and/or vdd_cpu */
	return pmu_adjust_voltage();
}

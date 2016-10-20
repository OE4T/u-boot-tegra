/*
 * (C) Copyright 2013-2016
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <pca953x.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include "../p2571/max77620_init.h"
#include "pinmux-config-m3402-0000.h"

void pin_mux_mmc(void)
{
	/* No SD/eMMC on this board */
}

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	pinmux_clear_tristate_input_clamping();

	gpio_config_table(m3402_0000_gpio_inits,
			  ARRAY_SIZE(m3402_0000_gpio_inits));

	pinmux_config_pingrp_table(m3402_0000_pingrps,
				   ARRAY_SIZE(m3402_0000_pingrps));

	pinmux_config_drvgrp_table(m3402_0000_drvgrps,
				   ARRAY_SIZE(m3402_0000_drvgrps));
}

#ifdef CONFIG_PCI_TEGRA
int tegra_pcie_board_init(void)
{
	/* TBD: provide power to the PEX slot? */
	return 0;
}
#endif /* PCI */

/*
 * There are 2 PCA9539 IO expanders on this board on I2C4:
 * The one at address 0x77 has WDT_CLEAR, WDT_ENABLE,
 *  GPIO_VBUS_EN, LOW_POWER_MODE, THERM_ALERT_OUT, etc.
 * The one at address 0x76 appears to only have debug GPIOS.
 */
#define PCA9539_I2C_BUS		5
#define PCA9539_I2C_ADDR	0x77
#define PCA9539_INP0_REG	(PCA953X_IN << 1)
#define PCA9539_INP1_REG	(PCA953X_IN << 1) + 1
#define PCA9539_OUT0_REG	(PCA953X_OUT << 1)
#define PCA9539_OUT1_REG	(PCA953X_OUT << 1) + 1
#define PCA9539_INV0_REG	(PCA953X_POL << 1)
#define PCA9539_INV1_REG	(PCA953X_POL << 1) + 1
#define PCA9539_CFG0_REG	(PCA953X_CONF << 1)
#define PCA9539_CFG1_REG	(PCA953X_CONF << 1) + 1
#define	P03_BIT			3	/* 3rd bit of 1st reg set, WDT_CLEAR */
#define	P04_BIT			4	/* 3rd bit of 1st reg set, WDT_RESET */
#define	P10_BIT			0	/* 1st bit of 2nd reg set, USB_MUXCTL */
#define	P11_BIT			1	/* 2nd bit of "   "   ", GPIO_VBUS_EN */

int gpio_vbus_init(bool enable)
{
	struct udevice *dev;
	uchar val;
	int ret;

	/*
	 * Enable/disable VBUS for USB3.0 type A connectors.
	 * P11 on PCA9539 GPIO Expander on I2C4 @ 77h controls GPIO_VBEN_EN
	 * that goes to a APL3511 power distribution switch.
	 */

	/* NOTE: Current PCA953x driver isn't sufficient for this (yet) */
	debug("%s: %sable USB VBUS\n", __func__, enable ? "Dis" : "En");

	ret = i2c_get_chip_for_busnum(PCA9539_I2C_BUS, PCA9539_I2C_ADDR, 1,
				      &dev);
	if (ret) {
		printf("Cannot find PCA9539 GPIO expander on I2C bus %d\n",
		       PCA9539_I2C_BUS);
		return -1;
	}

	/* Disable P11 polarity inversion */
	ret = dm_i2c_read(dev, PCA9539_INV1_REG, &val, 1);
	if (ret) {
		printf("i2c_read of GPIO expander reg 5 failed: %d\n", ret);
		return -1;
	}

	val &= ~BIT(P11_BIT);
	val |= (PCA953X_POL_NORMAL << P11_BIT);	/* normal polarity */
	ret = dm_i2c_write(dev, PCA9539_INV1_REG, &val, 1);
	if (ret) {
		printf("i2c_write of GPIO expander reg 5 failed: %d\n", ret);
		return -1;
	}

	/* Set P11 output high or low based on 'enable' arg */
	ret = dm_i2c_read(dev, PCA9539_OUT1_REG, &val, 1);
	if (ret) {
		printf("i2c_read of GPIO expander reg 3 failed: %d\n", ret);
		return -1;
	}

	val &= ~BIT(P11_BIT);
	if (enable)
		val |= (PCA953X_OUT_HIGH << P11_BIT);	/* drive high */
	ret = dm_i2c_write(dev, PCA9539_OUT1_REG, &val, 1);
	if (ret) {
		printf("i2c_write of GPIO expander reg 3 failed: %d\n", ret);
		return -1;
	}

	/* Set P11 (bit 1) config to OUTPUT */
	ret = dm_i2c_read(dev, PCA9539_CFG1_REG, &val, 1);
	if (ret) {
		printf("i2c_read of GPIO expander reg 7 failed: %d\n", ret);
		return -1;
	}

	val &= ~BIT(P11_BIT);
	val |= (PCA953X_DIR_OUT << P11_BIT);	/* P11 = output */
	ret = dm_i2c_write(dev, PCA9539_CFG1_REG, &val, 1);
	if (ret) {
		printf("i2c_write of GPIO expander reg 7 failed: %d\n", ret);
		return -1;
	}

	return 0;
}

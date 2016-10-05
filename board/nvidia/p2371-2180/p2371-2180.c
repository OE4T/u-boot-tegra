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
#include "pinmux-config-p2371-2180.h"

void pin_mux_mmc(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	/* Turn on MAX77620 LDO2 to 3.3V for SD card power */
	debug("%s: Set LDO2 for VDDIO_SDMMC_AP power to 3.3V\n", __func__);
	ret = i2c_get_chip_for_busnum(0, MAX77620_I2C_ADDR_7BIT, 1, &dev);
	if (ret) {
		printf("%s: Cannot find MAX77620 I2C chip\n", __func__);
		return;
	}
	/* 0xF2 for 3.3v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xF2;
	ret = dm_i2c_write(dev, MAX77620_CNFG1_L2_REG, &val, 1);
	if (ret)
		printf("i2c_write 0 0x3c 0x27 failed: %d\n", ret);

	/* Disable LDO4 discharge */
	ret = dm_i2c_read(dev, MAX77620_CNFG2_L4_REG, &val, 1);
	if (ret) {
		printf("i2c_read 0 0x3c 0x2c failed: %d\n", ret);
	} else {
		val &= ~BIT(1); /* ADE */
		ret = dm_i2c_write(dev, MAX77620_CNFG2_L4_REG, &val, 1);
		if (ret)
			printf("i2c_write 0 0x3c 0x2c failed: %d\n", ret);
	}

	/* Set MBLPD */
	ret = dm_i2c_read(dev, MAX77620_CNFGGLBL1_REG, &val, 1);
	if (ret) {
		printf("i2c_write 0 0x3c 0x00 failed: %d\n", ret);
	} else {
		val |= BIT(6); /* MBLPD */
		ret = dm_i2c_write(dev, MAX77620_CNFGGLBL1_REG, &val, 1);
		if (ret)
			printf("i2c_write 0 0x3c 0x00 failed: %d\n", ret);
	}
}

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	pinmux_clear_tristate_input_clamping();

	gpio_config_table(p2371_2180_gpio_inits,
			  ARRAY_SIZE(p2371_2180_gpio_inits));

	pinmux_config_pingrp_table(p2371_2180_pingrps,
				   ARRAY_SIZE(p2371_2180_pingrps));

	pinmux_config_drvgrp_table(p2371_2180_drvgrps,
				   ARRAY_SIZE(p2371_2180_drvgrps));
}

#ifdef CONFIG_PCI_TEGRA
int tegra_pcie_board_init(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	/* Turn on MAX77620 LDO1 to 1.05V for PEX power */
	debug("%s: Set LDO1 for PEX power to 1.05V\n", __func__);
	ret = i2c_get_chip_for_busnum(0, MAX77620_I2C_ADDR_7BIT, 1, &dev);
	if (ret) {
		printf("%s: Cannot find MAX77620 I2C chip\n", __func__);
		return -1;
	}
	/* 0xCA for 1.05v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xCA;
	ret = dm_i2c_write(dev, MAX77620_CNFG1_L1_REG, &val, 1);
	if (ret)
		printf("i2c_write 0 0x3c 0x25 failed: %d\n", ret);

	return 0;
}
#endif /* PCI */

#ifdef CONFIG_NV_BOARD_ID_EEPROM_CAM
/* IO expander */
#define PCA9539_I2C_BUS		2
#define PCA9539_I2C_ADDR	0x77
#define PCA9539_INP0_REG	(PCA953X_IN << 1)
#define PCA9539_INP1_REG	(PCA953X_IN << 1) + 1
#define PCA9539_OUT0_REG	(PCA953X_OUT << 1)
#define PCA9539_OUT1_REG	(PCA953X_OUT << 1) + 1
#define PCA9539_INV0_REG	(PCA953X_POL << 1)
#define PCA9539_INV1_REG	(PCA953X_POL << 1) + 1
#define PCA9539_CFG0_REG	(PCA953X_CONF << 1)
#define PCA9539_CFG1_REG	(PCA953X_CONF << 1) + 1
#define	P11_BIT			1	/* 2nd bit of 2nd reg set */

int camera_board_power_init(bool enable)
{
	struct udevice *dev;
	uchar val;
	int ret;

	/*
	 * Enable/disable 1.8V to camera board EEPROM.
	 * P11 on PCA9539 GPIO Expander on I2C2 @ 77h controls
	 * CAM_VDD_1V8_EN that powers the board ID EEPROM.
	 */

	/* NOTE: Current PCA953x driver isn't sufficient for this (yet) */
	debug("%s: %sable 1.8V to camera board EEPROM\n", __func__,
	      enable ? "Dis" : "En");

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

	/* Delay approx 10 msec for power to settle */
	if (enable)
		mdelay(10);

	return 0;
}
#endif	/* CONFIG_NV_BOARD_ID_EEPROM_CAM */

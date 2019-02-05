/*
 * (C) Copyright 2018
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <libfdt.h>
#include <pca953x.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include "../p2571/max77620_init.h"
#include "pinmux-config-p3450-porg.h"

#define ETH_ALEN 6

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

	gpio_config_table(p3450_porg_gpio_inits,
			  ARRAY_SIZE(p3450_porg_gpio_inits));

	pinmux_config_pingrp_table(p3450_porg_pingrps,
				   ARRAY_SIZE(p3450_porg_pingrps));

	pinmux_config_drvgrp_table(p3450_porg_drvgrps,
				   ARRAY_SIZE(p3450_porg_drvgrps));
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

int set_ethaddr_from_cboot(const void *fdt)
{
	int node, len, err;
	const uchar *mac;

	node = fdt_path_offset(fdt, "/pcie@1003000/pci@2,0/ethernet@0,0");
	if (node < 0)
		return 0;

	debug("PCI ethernet device tree node found\n");

	mac = fdt_getprop(fdt, node, "local-mac-address", &len);
	if (!mac)
		return 0;

	debug("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	err = eth_setenv_enetaddr("ethaddr", mac);
	if (err)
		return 0;

	return 0;
}

int ft_board_setup(void *fdt, bd_t *bd)
{
	uchar mac[ETH_ALEN];
	int node, err;

	node = fdt_path_offset(fdt, "/pcie@1003000/pci@2,0/ethernet@0,0");
	if (node < 0)
		return 0;

	debug("PCI ethernet device tree node found\n");

	if (eth_getenv_enetaddr("ethaddr", mac)) {
		err = fdt_setprop(fdt, node, "mac-address", mac, ETH_ALEN);
		if (err < 0)
			return 0;

		debug("MAC address set: %02x:%02x:%02x:%02x:%02x:%02x\n",
		      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return 0;
}

/*
 * Copyright (c) 2010-2016, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <stdlib.h>
#include <common.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <i2c.h>
#include <asm/arch-tegra/board.h>

struct __attribute__((__packed__)) eeprom_ver_1 {
	uint16_t version;
	uint16_t size;
	uint16_t board_no;
	uint16_t sku;
	uint8_t fab;
	uint8_t rev;
	uint8_t minor_rev;
	uint8_t mem_type;
	uint8_t pwr_cfg;
	uint8_t misc_cfg;
	uint8_t modem_cfg;
	uint8_t touch_cfg;
	uint8_t disp_cfg;
	uint8_t rework_lvl;
	uint16_t sno;
	uint8_t assetf1[30];
	uint8_t wifi_mac[6];
	uint8_t bt_mac[6];
	uint8_t second_wifi_mac[6];
	uint8_t eth_mac[6];
	uint8_t assetf2[15];
	uint8_t reserved[61];
	uint32_t cust_block_sig;
	uint16_t cust_block_len;
	uint16_t cust_type_sig;
	uint16_t cust_ver;
	uint8_t cust_wifi_mac[6];
	uint8_t cust_bt_mac[6];
	uint8_t cust_eth_mac[6];
	uint8_t cust_reserved[77];
	uint8_t checksum;
};

static struct eeprom_ver_1 eeprom_buf;

static uint8_t nv_brdid_eeprom_crc_iter(uint8_t data, uint8_t crc)
{
	uint8_t i;
	uint8_t odd;

	for (i = 0; i < 8; i++) {
		odd = (data ^ crc) & 1;
		crc >>= 1;
		data >>= 1;
		if (odd)
			crc ^= 0x8c;
	}

	return crc;
}

static bool nv_brdid_eeprom_crc_ok(uint8_t *data, uint32_t size)
{
	uint32_t i;
	uint8_t crc = 0;

	for (i = 0; i < size - 1; i++)
		crc = nv_brdid_eeprom_crc_iter(data[i], crc);

	return data[size - 1] == crc;
}

static int nv_brdinfo_eeprom_read(int bus, int chip)
{
	int ret;
	struct udevice *dev;

	ret = i2c_get_chip_for_busnum(bus, chip, 1, &dev);
	if (ret) {
		printf("ERROR: Cannot get ID EEPROM on I2C bus %d chip %02X\n",
		       bus, chip);
		goto print_warning;
	}

	if (dm_i2c_read(dev, 0, (void *)&eeprom_buf, sizeof(eeprom_buf))) {
		printf("ERROR: Cannot read board ID EEPROM\n");
		goto print_warning;
	}

	if (eeprom_buf.version != 1) {
		printf("ERROR: Board ID EEPROM version != 1\n");
		goto print_warning;
	}

	if (!nv_brdid_eeprom_crc_ok((void *)&eeprom_buf, sizeof(eeprom_buf))) {
		printf("ERROR: Board ID EEPROM CRC check failure\n");
		goto print_warning;
	}

	return ret;

print_warning:
	printf("%s: I2C bus %d chip %02X\n", __func__, bus, chip);
	printf("WARNING: failed to read board EEPROM\n");
	return ret;
}

#ifdef CONFIG_NV_BOARD_ID_EEPROM_OF_MAC_ADDRS
static int ft_add_chosen_mac_addrs(void *blob)
{
	char mac_str[6 * 3];
	int ret, chosen, i;
	struct {
		const char *prop;
		uint8_t *data;
	} macs[] = {
		{ "nvidia,wifi-mac", eeprom_buf.cust_wifi_mac },
		{ "nvidia,bluetooth-mac", eeprom_buf.cust_bt_mac },
		{ "nvidia,ethernet-mac", eeprom_buf.cust_eth_mac },
	};
	const uint8_t zero_mac[6] = {0};
	const uint8_t ones_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#define PACK(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
	if (eeprom_buf.cust_block_sig != PACK('N', 'V', 'C', 'B')) {
		printf("ERROR: Board ID EEPROM bad cust_block_sig\n");
		goto print_warning;
	}

	if (eeprom_buf.cust_block_len != 0x001c) {
		printf("ERROR: Board ID EEPROM bad cust_block_len\n");
		goto print_warning;
	}

	if (eeprom_buf.cust_type_sig != PACK('M', '1', 0, 0)) {
		printf("ERROR: Board ID EEPROM bad cust_type_sig\n");
		goto print_warning;
	}
#undef PACK

	if (eeprom_buf.cust_ver != 0) {
		printf("ERROR: Board ID EEPROM bad cust_ver\n");
		goto print_warning;
	}

	chosen = fdt_path_offset(blob, "/chosen");
	if (chosen < 0) {
		chosen = fdt_add_subnode(blob, 0, "/chosen");
		if (chosen < 0) {
			printf("ERROR: cannot add /chosen node to DT\n");
			goto print_warning;
		}
	}

	for (i = 0; i < ARRAY_SIZE(macs); i++) {
		if (!memcmp(macs[i].data, zero_mac, sizeof(zero_mac)))
			continue;
		if (!memcmp(macs[i].data, zero_mac, sizeof(ones_mac)))
			continue;
		snprintf(mac_str, sizeof(mac_str),
			 "%02x:%02x:%02x:%02x:%02x:%02x",
			 macs[i].data[5], macs[i].data[4], macs[i].data[3],
			 macs[i].data[2], macs[i].data[1], macs[i].data[0]);
		ret = fdt_setprop(blob, chosen, macs[i].prop, mac_str,
				  sizeof(mac_str));
		if (ret < 0)
			printf("ERROR: could not set /chosen/%s property\n",
			       macs[i].prop);
	}

	return 0;

print_warning:
	printf("WARNING: not setting /chosen MAC address props\n");
	/* Don't return an error, so the kernel still boots */
	return 0;
}
#endif

#ifdef CONFIG_NV_BOARD_ID_EEPROM_OF_PLUGIN_MANAGER
#define ASSET_F1_BOARD_ID_OFFSET	5
#define ASSET_F1_BOARD_ID_LEN		13
static int ft_add_plugin_manager_ids(void *blob)
{
	int parent, offset;
	char cvm_boardid[ASSET_F1_BOARD_ID_LEN+1] = { 0 };

	parent = 0;
	offset = fdt_path_offset(blob, "/chosen");
	if (offset < 0) {
		offset = fdt_add_subnode(blob, parent, "/chosen");
		if (offset < 0)
			goto print_warning;
	}

	parent = offset;
	offset = fdt_path_offset(blob, "/chosen/plugin-manager");
	if (offset < 0) {
		offset = fdt_add_subnode(blob, parent, "plugin-manager");
		if (offset < 0)
			goto print_warning;
	}

	parent = offset;
	offset = fdt_path_offset(blob, "/chosen/plugin-manager/ids");
	if (offset < 0) {
		offset = fdt_add_subnode(blob, parent, "ids");
		if (offset < 0)
			goto print_warning;
	}

	strncpy(cvm_boardid,
		(const char *)&eeprom_buf.assetf1[ASSET_F1_BOARD_ID_OFFSET],
		ASSET_F1_BOARD_ID_LEN);

	debug("CPU Board-ID from EEPROM Asset Tracker1: %s\n", cvm_boardid);

	fdt_setprop_string(blob, offset, cvm_boardid, "");

	return 0;

print_warning:
	printf("ERROR: DT add node failed %s\n", fdt_strerror(offset));
	/* Don't return an error, so the kernel still boots */
	return 0;
}
#endif

__weak int camera_board_power_init(bool enable)
{
	return 0;
}

int ft_board_setup_eeprom(void *blob)
{
	int ret;

	ret = nv_brdinfo_eeprom_read(EEPROM_I2C_BUS, EEPROM_I2C_ADDRESS);
	/* Board should continue to boot even if EEPROM read fails */
	if (!ret) {
#ifdef CONFIG_NV_BOARD_ID_EEPROM_OF_MAC_ADDRS
		ft_add_chosen_mac_addrs(blob);
#endif
#ifdef CONFIG_NV_BOARD_ID_EEPROM_OF_PLUGIN_MANAGER
		ft_add_plugin_manager_ids(blob);
#endif
	}

#ifdef CONFIG_NV_BOARD_ID_EEPROM_CAM
	/* Enable 1.8V to the camera board EEPROM */
	camera_board_power_init(true);

	ret = nv_brdinfo_eeprom_read(CAM_EEPROM_I2C_BUS, CAM_EEPROM_I2C_ADDR);
	if (!ret)
		ft_add_plugin_manager_ids(blob);

	/* Disable power to the camera board EEPROM */
	camera_board_power_init(false);
#endif

	return 0;
}

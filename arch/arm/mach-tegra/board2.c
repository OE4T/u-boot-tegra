/*
 *  (C) Copyright 2010,2011,2015
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdlib.h>
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <ns16550.h>
#include <linux/compiler.h>
#include <linux/sizes.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#ifdef CONFIG_LCD
#include <asm/arch/display.h>
#endif
#include <asm/arch/funcmux.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/pmu.h>
#ifdef CONFIG_PWM_TEGRA
#include <asm/arch/pwm.h>
#endif
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/board.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch-tegra/sys_proto.h>
#include <asm/arch-tegra/uart.h>
#include <asm/arch-tegra/warmboot.h>
#ifdef CONFIG_TEGRA_CLOCK_SCALING
#include <asm/arch/emc.h>
#endif
#ifdef CONFIG_USB_EHCI_TEGRA
#include <asm/arch-tegra/usb.h>
#include <usb.h>
#endif
#ifdef CONFIG_TEGRA_MMC
#include <asm/arch-tegra/tegra_mmc.h>
#include <asm/arch-tegra/mmc.h>
#endif
#include <asm/arch-tegra/xusb-padctl.h>
#include <power/as3722.h>
#include <i2c.h>
#include <spi.h>
#include <fdt_support.h>
#include "emc.h"
#include "ft_board_info.h"
#include "nvtboot.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_BUILD
/* TODO(sjg@chromium.org): Remove once SPL supports device tree */
U_BOOT_DEVICE(tegra_gpios) = {
	"gpio_tegra"
};
#endif

__weak void pinmux_init(void) {}
__weak void pin_mux_usb(void) {}
__weak void pin_mux_spi(void) {}
__weak void gpio_early_init_uart(void) {}
__weak void pin_mux_display(void) {}
__weak void start_cpu_fan(void) {}

#if defined(CONFIG_TEGRA_NAND)
__weak void pin_mux_nand(void)
{
	funcmux_select(PERIPH_ID_NDFLASH, FUNCMUX_DEFAULT);
}
#endif

/*
 * Routine: power_det_init
 * Description: turn off power detects
 */
static void power_det_init(void)
{
#if defined(CONFIG_TEGRA20)
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	/* turn off power detects */
	writel(0, &pmc->pmc_pwr_det_latch);
	writel(0, &pmc->pmc_pwr_det);
#endif
}

__weak int tegra_board_id(void)
{
	return -1;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	int board_id = tegra_board_id();

	printf("Board: %s", CONFIG_TEGRA_BOARD_STRING);
	if (board_id != -1)
		printf(", ID: %d\n", board_id);
	printf("\n");

	return 0;
}
#endif	/* CONFIG_DISPLAY_BOARDINFO */

__weak int tegra_lcd_pmic_init(int board_it)
{
	return 0;
}

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	__maybe_unused int err;
	__maybe_unused int board_id;

	/* Do clocks and UART first so that printf() works */
	clock_init();
	clock_verify();

#ifdef CONFIG_TEGRA_SPI
	pin_mux_spi();
#endif

#ifdef CONFIG_PWM_TEGRA
	if (pwm_init(gd->fdt_blob))
		debug("%s: Failed to init pwm\n", __func__);
#endif
#ifdef CONFIG_LCD
	pin_mux_display();
	tegra_lcd_check_next_stage(gd->fdt_blob, 0);
#endif
	/* boot param addr */
	gd->bd->bi_boot_params = (NV_PA_SDRAM_BASE + 0x100);

	power_det_init();

#ifdef CONFIG_SYS_I2C_TEGRA
# ifdef CONFIG_TEGRA_PMU
	if (pmu_set_nominal())
		debug("Failed to select nominal voltages\n");
#  ifdef CONFIG_TEGRA_CLOCK_SCALING
	err = board_emc_init();
	if (err)
		debug("Memory controller init failed: %d\n", err);
#  endif
# endif /* CONFIG_TEGRA_PMU */
#ifdef CONFIG_AS3722_POWER
	err = as3722_init(NULL);
	if (err && err != -ENODEV)
		return err;
#endif
#endif /* CONFIG_SYS_I2C_TEGRA */

#ifdef CONFIG_USB_EHCI_TEGRA
	pin_mux_usb();
	usb_process_devicetree(gd->fdt_blob);
#endif

#ifdef CONFIG_LCD
	board_id = tegra_board_id();
	err = tegra_lcd_pmic_init(board_id);
	if (err)
		return err;
	tegra_lcd_check_next_stage(gd->fdt_blob, 0);
#endif

#ifdef CONFIG_TEGRA_NAND
	pin_mux_nand();
#endif

	tegra_xusb_padctl_init(gd->fdt_blob);

#ifdef CONFIG_TEGRA_LP0
	/* save Sdram params to PMC 2, 4, and 24 for WB0 */
	warmboot_save_sdram_params();
#endif

	return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
static void __gpio_early_init(void)
{
}

void gpio_early_init(void) __attribute__((weak, alias("__gpio_early_init")));

int board_early_init_f(void)
{
#ifdef CONFIG_NVTBOOT
	nvtboot_init();
#endif

	/* Do any special system timer/TSC setup */
#if defined(CONFIG_TEGRA_SUPPORT_NON_SECURE)
	if (!tegra_cpu_is_non_secure())
#endif
		arch_timer_init();

	pinmux_init();
	board_init_uart_f();

	/* Initialize periph GPIOs */
	gpio_early_init();
	gpio_early_init_uart();
#ifdef CONFIG_LCD
	tegra_lcd_early_init(gd->fdt_blob);
#endif

	return 0;
}
#endif	/* EARLY_INIT */

int board_late_init(void)
{
#ifdef CONFIG_LCD
	/* Make sure we finish initing the LCD */
	tegra_lcd_check_next_stage(gd->fdt_blob, 1);
#endif

#ifdef CONFIG_TEGRA_LP0
	/* prepare the WB code to LP0 location */
	warmboot_prepare_code(TEGRA_LP0_ADDR, TEGRA_LP0_SIZE);
#endif

#if defined(CONFIG_TEGRA_SUPPORT_NON_SECURE)
	if (tegra_cpu_is_non_secure()) {
		printf("CPU is in NS mode\n");
		setenv("cpu_ns_mode", "1");
	} else {
		setenv("cpu_ns_mode", "");
	}
#endif
	start_cpu_fan();
#if defined(CONFIG_NVTBOOT)
	nvtboot_init_late();
#endif

	return 0;
}

#if defined(CONFIG_TEGRA_MMC)
__weak void pin_mux_mmc(void)
{
}

/* this is a weak define that we are overriding */
int board_mmc_init(bd_t *bd)
{
	debug("%s called\n", __func__);

	/* Enable muxes, etc. for SDMMC controllers */
	pin_mux_mmc();

	debug("%s: init MMC\n", __func__);
	tegra_mmc_init();

	return 0;
}

void pad_init_mmc(struct mmc_host *host)
{
#if defined(CONFIG_TEGRA30)
	enum periph_id id = host->mmc_id;
	u32 val;

	debug("%s: sdmmc address = %08x, id = %d\n", __func__,
		(unsigned int)host->reg, id);

	/* Set the pad drive strength for SDMMC1 or 3 only */
	if (id != PERIPH_ID_SDMMC1 && id != PERIPH_ID_SDMMC3) {
		debug("%s: settings are only valid for SDMMC1/SDMMC3!\n",
			__func__);
		return;
	}

	val = readl(&host->reg->sdmemcmppadctl);
	val &= 0xFFFFFFF0;
	val |= MEMCOMP_PADCTRL_VREF;
	writel(val, &host->reg->sdmemcmppadctl);

	val = readl(&host->reg->autocalcfg);
	val &= 0xFFFF0000;
	val |= AUTO_CAL_PU_OFFSET | AUTO_CAL_PD_OFFSET | AUTO_CAL_ENABLED;
	writel(val, &host->reg->autocalcfg);
#endif	/* T30 */
}
#endif	/* MMC */

int ft_board_info_set(void *blob, const struct ft_board_info *bi,
	const char *nodepath, const char *nodename)
{
	int offset, ret;
	u32 val;

	offset = fdt_path_offset(blob, nodepath);
	if (offset < 0) {
		int chosen = fdt_path_offset(blob, "/chosen");
		offset = fdt_add_subnode(blob, chosen, nodename);
		if (offset < 0) {
			printf("ERROR: add node %s: %s.\n",
			       nodepath, fdt_strerror(offset));
			return offset;
		}
	}

	val = cpu_to_fdt32(bi->id);
	ret = fdt_setprop(blob, offset, "id", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update id property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->sku);
	ret = fdt_setprop(blob, offset, "sku", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update sku property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->fab);
	ret = fdt_setprop(blob, offset, "fab", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update fab property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->major);
	ret = fdt_setprop(blob, offset, "major_revision", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update major_revision property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->minor);
	ret = fdt_setprop(blob, offset, "minor_revision", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update minor_revision property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	return 0;
}

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	int ret;
	struct udevice *dev;

	/* pass board id to kernel */
	serialnr->high = CONFIG_TEGRA_SERIAL_HIGH;
	serialnr->low = CONFIG_TEGRA_SERIAL_LOW;
	/* TODO: use FDT */

	debug("Config file serialnr->high = %08X, ->low = %08X\n",
	      serialnr->high, serialnr->low);

	/*
	 * Check if we can read the EEPROM serial number
	 *  and if so, use it instead.
	 */

#if !defined(CONFIG_SERIAL_EEPROM)
	debug("No serial EEPROM onboard, using default values\n");
	return;
#else
	uchar data_buffer[NUM_SERIAL_ID_BYTES];

	ret = i2c_get_chip_for_busnum(EEPROM_I2C_BUS, EEPROM_I2C_ADDRESS, 1,
				      &dev);
	if (ret) {
		debug("%s: Cannot find EEPROM I2C chip\n", __func__);
		return;
	}

	if (dm_i2c_read(dev, EEPROM_SERIAL_OFFSET, data_buffer,
			NUM_SERIAL_ID_BYTES)) {
		printf("%s: I2C read of bus %d chip %d addr %d failed!\n",
		       __func__, EEPROM_I2C_BUS, EEPROM_I2C_ADDRESS,
		       EEPROM_SERIAL_OFFSET);
	} else {
#ifdef DEBUG
		int i;
		printf("get_board_serial: Got ");
		for (i = 0; i < NUM_SERIAL_ID_BYTES; i++)
			printf("%02X:", data_buffer[i]);
		printf("\n");
#endif
		serialnr->high = data_buffer[2];
		serialnr->high |= (u32)data_buffer[3] << 8;
		serialnr->high |= (u32)data_buffer[0] << 16;
		serialnr->high |= (u32)data_buffer[1] << 24;

		serialnr->low = data_buffer[7];
		serialnr->low |= (u32)data_buffer[6] << 8;
		serialnr->low |= (u32)data_buffer[5] << 16;
		serialnr->low |= (u32)data_buffer[4] << 24;

		debug("SEEPROM serialnr->high = %08X, ->low = %08X\n",
		      serialnr->high, serialnr->low);
	}
#endif	/* SERIAL_EEPROM */
}

#ifdef CONFIG_OF_BOARD_SETUP
int fdt_serial_tag_setup(void *blob, bd_t *bd)
{
	struct tag_serialnr serialnr;
	struct ft_board_info bi;
	int ret;

	get_board_serial(&serialnr);

	bi.id = serialnr.high >> 16;
	bi.sku = serialnr.high & 0xffff;
	bi.fab = serialnr.low >> 24;
	bi.major = (serialnr.low >> 16) & 0xff;
	bi.minor = (serialnr.low >> 8) & 0xff;

	ret = ft_board_info_set(blob, &bi, "/chosen/proc-board", "proc-board");
	if (ret)
		return ret;
	ret = ft_board_info_set(blob, &bi, "/chosen/pmu-board", "pmu-board");
	if (ret)
		return ret;

	return 0;
}
#endif  /* OF_BOARD_SETUP */
#endif	/* SERIAL_TAG */

#ifdef CONFIG_OF_REMOVE_FIRMWARE_NODES
int ft_remove_firmware_nodes(void *blob)
{
	int offset;

	offset = fdt_path_offset(blob, "/firmware");
	if (offset >= 0)
		fdt_del_node(blob, offset);

	offset = fdt_path_offset(blob, "/psci");
	if (offset >= 0)
		fdt_del_node(blob, offset);

	return 0;
}
#endif

#ifdef CONFIG_OF_COPY_NODES
#define fdt_for_each_property(fdt, prop, parent)		\
	for (prop = fdt_first_property_offset(fdt, parent);	\
	     prop >= 0;						\
	     prop = fdt_next_property_offset(fdt, prop))

int fdt_copy_node_content(void *blob_src, int ofs_src, void *blob_dst,
		int ofs_dst, int indent)
{
	int ofs_src_child, ofs_dst_child;

	/*
	 * FIXME: This doesn't remove properties or nodes in the destination
	 * that are not present in the source. For the nodes we care about
	 * right now, this is not an issue.
	 */

	fdt_for_each_property(blob_src, ofs_src_child, ofs_src) {
		const char *prop;
		const char *name;
		int len, ret;

		prop = fdt_getprop_by_offset(blob_src, ofs_src_child, &name,
					     &len);
		debug("%*scopy prop: %s\n", indent, "", name);

		ret = fdt_setprop(blob_dst, ofs_dst, name, prop, len);
		if (ret < 0) {
			printf("Can't create DT prop %s to copy\n", name);
			return ret;
		}
	}

	fdt_for_each_subnode(blob_src, ofs_src_child, ofs_src) {
		const char *name;

		name = fdt_get_name(blob_src, ofs_src_child, NULL);
		debug("%*scopy node: %s\n", indent, "", name);

		ofs_dst_child = fdt_subnode_offset(blob_dst, ofs_dst, name);
		if (ofs_dst_child < 0) {
			debug("%*s(creating it in dst)\n", indent, "");
			ofs_dst_child = fdt_add_subnode(blob_dst, ofs_dst,
							name);
			if (ofs_dst_child < 0) {
				printf("Can't create DT node %s to copy\n", name);
				return ofs_dst_child;
			}
		}

		fdt_copy_node_content(blob_src, ofs_src_child, blob_dst,
				      ofs_dst_child, indent + 2);
	}

	return 0;
}

int ft_copy_node(void *blob_src, void *blob_dst, const char *nodename)
{
	int ofs_dst_mc, ofs_src_mc;
	int ret;

	ofs_dst_mc = fdt_path_offset(blob_dst, nodename);
	if (ofs_dst_mc < 0) {
		ofs_dst_mc = fdt_add_subnode(blob_dst, 0, nodename);
		if (ofs_dst_mc < 0) {
			printf("Can't create DT node %s to copy\n", nodename);
			return ofs_dst_mc;
		}
	} else {
		if (!fdtdec_get_is_enabled(blob_dst, ofs_dst_mc)) {
			printf("DT node %s disabled in dst; skipping copy\n",
			       nodename);
			return 0;
		}
	}

	ofs_src_mc = fdt_path_offset(blob_src, nodename);
	if (ofs_src_mc < 0) {
		printf("DT node %s missing in source; can't copy\n",
		       nodename);
		return 0;
	}

	ret = fdt_copy_node_content(blob_src, ofs_src_mc, blob_dst,
				    ofs_dst_mc, 2);
	if (ret < 0)
		return ret;

	return 0;
}

int ft_copy_nodes(void *blob_dst)
{
	char *src_addr_s;
	ulong src_addr;
	char *node_names, *tmp, *node_name;
	int ret;

	src_addr_s = getenv("fdt_copy_src_addr");
	if (!src_addr_s)
		return 0;
	src_addr = simple_strtoul(src_addr_s, NULL, 16);

	node_names = getenv("fdt_copy_node_names");
	if (!node_names)
		return 0;
	node_names = strdup(node_names);
	if (!node_names) {
		printf("%s:strdup failed", __func__);
		ret = -1;
		goto out;
	}

	tmp = node_names;
	while (true) {
		node_name = strsep(&tmp, ":");
		if (!node_name)
			break;
		debug("node to copy: %s\n", node_name);
		ret = ft_copy_node((void *)src_addr, blob_dst, node_name);
		if (ret < 0) {
			ret = -1;
			goto out;
		}
	}

	ret = 0;

out:
	free(node_names);

	return ret;
}
#endif

#ifdef CONFIG_OF_ADD_CHOSEN_MAC_ADDRS
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

int ft_add_chosen_mac_addrs(void *blob)
{
	struct eeprom_ver_1 eeprom_buf;
	char mac_str[6 * 3];
	int ret, chosen, i;
	struct udevice *dev;
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

	ret = i2c_get_chip_for_busnum(EEPROM_I2C_BUS, EEPROM_I2C_ADDRESS, 1,
				      &dev);
	if (ret) {
		printf("ERROR: Cannot get board ID EEPROM I2C chip\n");
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

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	int ret;

#ifdef CONFIG_OF_REMOVE_FIRMWARE_NODES
	ret = ft_remove_firmware_nodes(blob);
	if (ret)
		return ret;
#endif

	/* Overwrite DT file with right board info properties */
#ifdef CONFIG_SERIAL_TAG
	ret = fdt_serial_tag_setup(blob, bd);
	if (ret)
		return ret;
#endif	/* SERIAL_TAG */

#ifdef CONFIG_OF_COPY_NODES
	ret = ft_copy_nodes(blob);
	if (ret)
		return ret;
#endif

#ifdef CONFIG_NVTBOOT
	ret = ft_nvtboot(blob);
	if (ret)
		return ret;
#endif

#ifdef CONFIG_OF_ADD_CHOSEN_MAC_ADDRS
	ret = ft_add_chosen_mac_addrs(blob);
	if (ret)
		return ret;
#endif

	return 0;
}
#endif  /* OF_BOARD_SETUP */

#ifndef CONFIG_NVTBOOT
/*
 * In some SW environments, a memory carve-out exists to house a secure
 * monitor, a trusted OS, and/or various statically allocated media buffers.
 *
 * This carveout exists at the highest possible address that is within a
 * 32-bit physical address space.
 *
 * This function returns the total size of this carve-out. At present, the
 * returned value is hard-coded for simplicity. In the future, it may be
 * possible to determine the carve-out size:
 * - By querying some run-time information source, such as:
 *   - A structure passed to U-Boot by earlier boot software.
 *   - SoC registers.
 *   - A call into the secure monitor.
 * - In the per-board U-Boot configuration header, based on knowledge of the
 *   SW environment that U-Boot is being built for.
 *
 * For now, we support two configurations in U-Boot:
 * - 32-bit ports without any form of carve-out.
 * - 64 bit ports which are assumed to use a carve-out of a conservatively
 *   hard-coded size.
 */
static ulong carveout_size(void)
{
#ifdef CONFIG_ARM64
	return SZ_512M;
#else
	return 0;
#endif
}

/*
 * Determine the amount of usable RAM below 4GiB, taking into account any
 * carve-out that may be assigned.
 */
static ulong usable_ram_size_below_4g(void)
{
	ulong total_size_below_4g;
	ulong usable_size_below_4g;

	/*
	 * The total size of RAM below 4GiB is the lesser address of:
	 * (a) 2GiB itself (RAM starts at 2GiB, and 4GiB - 2GiB == 2GiB).
	 * (b) The size RAM physically present in the system.
	 */
	if (gd->ram_size < SZ_2G)
		total_size_below_4g = gd->ram_size;
	else
		total_size_below_4g = SZ_2G;

	/* Calculate usable RAM by subtracting out any carve-out size */
	usable_size_below_4g = total_size_below_4g - carveout_size();

	return usable_size_below_4g;
}

/*
 * Represent all available RAM in either one or two banks.
 *
 * The first bank describes any usable RAM below 4GiB.
 * The second bank describes any RAM above 4GiB.
 *
 * This split is driven by the following requirements:
 * - The NVIDIA L4T kernel requires separate entries in the DT /memory/reg
 *   property for memory below and above the 4GiB boundary. The layout of that
 *   DT property is directly driven by the entries in the U-Boot bank array.
 * - The potential existence of a carve-out at the end of RAM below 4GiB can
 *   only be represented using multiple banks.
 *
 * Explicitly removing the carve-out RAM from the bank entries makes the RAM
 * layout a bit more obvious, e.g. when running "bdinfo" at the U-Boot
 * command-line.
 *
 * This does mean that the DT U-Boot passes to the Linux kernel will not
 * include this RAM in /memory/reg at all. An alternative would be to include
 * all RAM in the U-Boot banks (and hence DT), and add a /memreserve/ node
 * into DT to stop the kernel from using the RAM. IIUC, I don't /think/ the
 * Linux kernel will ever need to access any RAM in* the carve-out via a CPU
 * mapping, so either way is acceptable.
 *
 * On 32-bit systems, we never define a bank for RAM above 4GiB, since the
 * start address of that bank cannot be represented in the 32-bit .size
 * field.
 */
void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = usable_ram_size_below_4g();

#ifdef CONFIG_PHYS_64BIT
	if (gd->ram_size > SZ_2G) {
		gd->bd->bi_dram[1].start = 0x100000000;
		gd->bd->bi_dram[1].size = gd->ram_size - SZ_2G;
	} else
#endif
	{
		gd->bd->bi_dram[1].start = 0;
		gd->bd->bi_dram[1].size = 0;
	}
}

/*
 * Most hardware on 64-bit Tegra is still restricted to DMA to the lower
 * 32-bits of the physical address space. Cap the maximum usable RAM area
 * at 4 GiB to avoid DMA buffers from being allocated beyond the 32-bit
 * boundary that most devices can address. Also, don't let U-Boot use any
 * carve-out, as mentioned above.
 *
 * This function is called before dram_init_banksize(), so we can't simply
 * return gd->bd->bi_dram[1].start + gd->bd->bi_dram[1].size.
 */
ulong board_get_usable_ram_top(ulong total_size)
{
	return CONFIG_SYS_SDRAM_BASE + usable_ram_size_below_4g();
}
#endif

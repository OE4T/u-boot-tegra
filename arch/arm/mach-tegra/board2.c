/*
 *  (C) Copyright 2010,2011
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <ns16550.h>
#include <linux/compiler.h>
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
int fdt_serial_tag_setup_one(void *blob, const struct tag_serialnr *serialnr,
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

	val = serialnr->high >> 16;
	val = cpu_to_fdt32(val);
	ret = fdt_setprop(blob, offset, "id", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update id property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = serialnr->high & 0xFFFF;
	val = cpu_to_fdt32(val);
	ret = fdt_setprop(blob, offset, "sku", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update sku property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = serialnr->low >> 24;
	val = cpu_to_fdt32(val);
	ret = fdt_setprop(blob, offset, "fab", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update fab property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = (serialnr->low >> 16) & 0xFF;
	val = cpu_to_fdt32(val);
	ret = fdt_setprop(blob, offset, "major_revision", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update major_revision property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	val = (serialnr->low >> 8) & 0xFF;
	val = cpu_to_fdt32(val);
	ret = fdt_setprop(blob, offset, "minor_revision", &val, sizeof(val));
	if (ret < 0) {
		printf("ERROR: could not update minor_revision property %s.\n",
		       fdt_strerror(ret));
		return ret;
	}

	return 0;
}

int fdt_serial_tag_setup(void *blob, bd_t *bd)
{
	struct tag_serialnr serialnr;
	int ret;

	get_board_serial(&serialnr);

	ret = fdt_serial_tag_setup_one(blob, &serialnr, "/chosen/proc-board",
				       "proc-board");
	if (ret)
		return ret;
	ret = fdt_serial_tag_setup_one(blob, &serialnr, "/chosen/pmu-board",
				       "pmu-board");
	if (ret)
		return ret;

	return 0;
}
#endif  /* OF_BOARD_SETUP */
#endif	/* SERIAL_TAG */

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	int ret;

	/* Overwrite DT file with right board info properties */
#ifdef CONFIG_SERIAL_TAG
	ret = fdt_serial_tag_setup(blob, bd);
	if (ret)
		return ret;
#endif	/* SERIAL_TAG */

	return 0;
}
#endif  /* OF_BOARD_SETUP */

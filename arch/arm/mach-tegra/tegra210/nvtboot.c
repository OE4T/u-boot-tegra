/*
 * nvtboot-related code
 *
 * (C) Copyright 2015 NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdlib.h>
#include <common.h>
#include "nvtboot.h"

DECLARE_GLOBAL_DATA_PTR;

typedef enum {
	/* Invalid entry */
	MEM_LAYOUT_NONE = 0x0,
	/* Primary memory layout */
	MEM_LAYOUT_PRIMARY,
	/* Extended memory layout */
	MEM_LAYOUT_EXTENDED,
	/* VPR carveout information */
	MEM_LAYOUT_VPR,
	/* TSEC carveout information */
	MEM_LAYOUT_TSEC,
	/* SECUREOS carveout information */
	MEM_LAYOUT_SECUREOS,
	/* LP0 carveout information */
	MEM_LAYOUT_LP0,
	/* NCK carveout information */
	MEM_LAYOUT_NCK,
	/* DEBUG carveout information */
	MEM_LAYOUT_DEBUG,
	/* BPMP FW carveout information */
	MEM_LAYOUT_BPMPFW,
	/* RAMDump carveout information */
	MEM_LAYOUT_NVDUMPER,
	/* carveout for GSC1 */
	MEM_LAYOUT_GSC1,
	/* carveout for GSC2 */
	MEM_LAYOUT_GSC2,
	/* carveout for GSC3 */
	MEM_LAYOUT_GSC3,
	/* carveout for GSC4 */
	MEM_LAYOUT_GSC4,
	/* carveout for GSC5 */
	MEM_LAYOUT_GSC5,
	/* Total types of memory layout */
	MEM_LAYOUT_NUM,
} mem_layout;

/**
 * Reason for poweron
 */
typedef enum {
	poweron_source_pmcpor,
	poweron_source_apwdt_rst,
	poweron_source_sensor_rst,
	poweron_source_sw_rst,
	poweron_source_deepsleep_rst,
	poweron_source_aotag_rst,
	poweron_source_onkey,
	poweron_source_resetkey,
	poweron_source_usbhotplug,
	poweron_source_rtcalarm,
	poweron_source_pmuwdt,
	poweron_source_unknown = 0x7fffffff,
} poweron_source;

/**
 * Tracks the base and size of the Carveout
 */
typedef struct {
	/* Base of the carveout */
	 uint64_t base;
	/* Size of the carveout */
	 uint64_t size;
} carveout_info;

/**
 * Stores the information related to memory params
 */
typedef struct {
	/* Size and Base of the carveout */
	carveout_info car_info[MEM_LAYOUT_NUM];
	/* DRAM bottom information */
	uint64_t dram_carveout_bottom;
} mem_info;

/**
 * Stores the information related to memory params
 */
typedef enum {
	/* Specifies the memory type to be undefined */
	dram_memory_type_none = 0,
	/* Specifies the memory type to be LPDDR2 SDRAM */
	dram_memory_type_lpddr2,
	/* Specifies the memory type to be DDR3 SDRAM */
	dram_memory_type_ddr3,
	dram_memory_type_lpddr4,
	dram_memory_type_num,
	dram_memory_type_force32 = 0x7FFFFFF
} dram_memory_type;

/**
 * Defines the BootDevice Type available
 */
typedef enum {
	/* boot device is sdmmc */
	BOOT_DEVICE_SDMMC,
	/* boot device is snor */
	BOOT_DEVICE_SNOR_FLASH,
	/* boot device is spi */
	BOOT_DEVICE_SPI_FLASH,
	/* boot device is sata */
	BOOT_DEVICE_SATA,
	/* boot device is usb3 */
	BOOT_DEVICE_USB3,
	/*reserved boot device */
	BOOT_DEVICE_RESVD,
	/* max number of boot devices */
	BOOT_DEVICE_MAX,
} fuse_boot_device;

/**
 * Device type and instance
 */
typedef struct {
	/* boot of storage device type */
	fuse_boot_device device;
	/* instance of the device */
	uint32_t instance;
} dev_info;

/**
 * Holds the chip's unique ID.
 */
typedef struct {
	/* Bytes 3-0 of the ecid */
	uint32_t ecid_0;
	/* Bytes 7-4 of ecid */
	uint32_t ecid_1;
	/* Bytes 11-8 of ecid */
	uint32_t ecid_2;
	/* Bytes 15-12 of ecid */
	uint32_t ecid_3;
} unique_chip;

/**
 * Defines the board information stored on eeprom
 */
typedef struct {
	/* Holds the version number. */
	uint16_t version;
	/* Holds the size of 2 byte value of \a board_id data that follows. */
	uint16_t size;
	/* Holds the board number. */
	uint16_t board_id;
	/* Holds the SKU number. */
	uint16_t sku;
	/* Holds the FAB number. */
	uint8_t fab;
	/* Holds the part revision. */
	uint8_t revision;
	/* Holds the minor revision level. */
	uint8_t minor_revision;
	/* Holds the memory type. */
	uint8_t mem_type;
	/* Holds the power configuration values. */
	uint8_t power_config;
	/* Holds the SPL reworks, mech. changes. */
	uint8_t misc_config;
	/* Holds the modem bands value. */
	uint8_t modem_bands;
	/* Holds the touch screen configs. */
	uint8_t touch_screen_configs;
	/* Holds the display configs. */
	uint8_t display_configs;
} board_info;

/**
 * Specifies different modes of booting.
 */
typedef enum {
	/* Specifies a default (unset) value. */
	BOOT_MODE_NONE = 0,
	/* Specifies a cold boot */
	BOOT_MODE_COLD,
	/* Specifies the BR entered RCM */
	BOOT_MODE_RECOVERY,
	/* Specifies UART boot (only available internal to NVIDIA) */
	BOOT_MODE_UART,
} boot_mode;

/**
 * Specifies different operating modes.
 */
typedef enum {
	/* Invalid Operating mode */
	OPMODE_NONE = 0,
	/* Preproduction mode */
	OPMODE_PRE_PRODUCTION,
	/* failure analysis mode */
	OPMODE_FAILURE_ANALYSIS,
	/* nvproduction mode */
	OPMODE_NV_PRODUCTION,
	/* non-secure mode */
	OPMODE_ODM_PRODUCTION_NON_SECURE,
	/* sbk mode */
	OPMODE_ODM_PRODUCTION_SECURE_SBK,
	/* pkc mode */
	OPMODE_ODM_PRODUCTION_SECURE_PKC,
} opmode_type;

typedef struct {
	/* Active BCT block */
	uint32_t active_bct_block;
	/* Active BCT sector */
	uint32_t active_bct_sector;
} active_bct_info;

/**
 * Specifies shared structure.
 */
typedef struct {
	/* Revision of common structure */
	uint32_t revision;
	/* CRC of this structure */
	uint32_t crc;
	/* Early Uart Address */
	uint64_t early_uart_addr;
	/* Memory layout Information */
	mem_info params;
	/* Poweron Source */
	poweron_source pwron_src;
	/* Pmic shutdown reason */
	uint8_t pmic_reset_reason;
	/* Information about bootdevice */
	dev_info boot_dev;
	/* Information about storage device */
	dev_info storage_dev;
	/* Unique Chip Id */
	unique_chip uid;
	dram_memory_type dram_type;
	/* Processor Board Details */
	board_info proc_board_details;
	/* Pmu Board Details */
	board_info pmu_board_details;
	/* Display BoardDetails */
	board_info display_board_details;
	/* Size of Bct in bytes */
	uint32_t bct_size;
	/* Size of start Offset of tboot in bytes */
	uint64_t tboot_start_offset_inbytes;
	/* Size of boot device block size in  bytes */
	uint32_t block_size_inbytes;
	/* Size of boot device page size in bytes */
	uint32_t page_size_inbytes;
	/* Boot mode can be cold boot, uart or recovery */
	boot_mode boot_type;
	/* Operating Mode of device */
	opmode_type op_mode;
	/* Size of start Offset of mts preboot in bytes */
	uint64_t mts_start_offset_inbytes;
	/* Info of the bct being used */
	active_bct_info active_bct;
	/* Bootloader dtb load address */
	uint32_t bl_dtb_load_addr;
	/* Kernel dtb load address */
	uint32_t kernel_dtb_load_addr;
	/* Reserved Bytes at the end */
	uint8_t reserved[256];
} boot_arg;

extern boot_arg *nvtboot_boot_x5;
/*
 * This must be in .data not .bss so we can access it before relocation, both
 * due to MMU permissions, and because we don't want to trash any data that's
 * appended to the U-Boot binary (DTB, binary data for flashing, ...) Hence,
 * assign some meaningless value to it.
 */
static boot_arg nvtboot_boot_arg = {0xff};

void nvtboot_init(void)
{
	memcpy(&nvtboot_boot_arg, nvtboot_boot_x5, sizeof(nvtboot_boot_arg));
	/* FIXME: Should validate the structure here (version, CRC) */
}

/*
 * Initialize U-Boot's DRAM bank configuration from the information supplied
 * by nvtboot.
 *
 * nvtboot happens to segment RAM into two banks:
 * MEM_LAYOUT_PRIMARY describes any usable RAM below 4GiB.
 * MEM_LAYOUT_EXTENDED describes any RAM above 4GiB.
 *
 * This split is driven by the following requirements:
 * - The NVIDIA L4T kernel requires separate entries in the DT /memory/reg
 *   property for memory below and above the 4GiB boundary. The layout of that
 *   DT property is directly driven by the entries in the U-Boot bank array.
 * - The potential existence of a carve-out at the end of RAM below 4GiB can
 *   only be represented using multiple banks.
 */
void dram_init_banksize(void)
{
	carveout_info *ci;

	ci = &nvtboot_boot_arg.params.car_info[MEM_LAYOUT_PRIMARY];
	gd->bd->bi_dram[0].start = ci->base;
	gd->bd->bi_dram[0].size = ci->size & ~(SZ_2M - 1);

#ifdef CONFIG_PHYS_64BIT
	ci = &nvtboot_boot_arg.params.car_info[MEM_LAYOUT_EXTENDED];
	gd->bd->bi_dram[1].start = ci->base;
	gd->bd->bi_dram[1].size = ci->size & ~(SZ_2M - 1);
#else
	gd->bd->bi_dram[1].start = 0;
	gd->bd->bi_dram[1].size = 0;
#endif
}

/*
 * Most hardware on 64-bit Tegra is still restricted to DMA to the lower
 * 32-bits of the physical address space. Cap the maximum usable RAM area
 * at 4 GiB to avoid DMA buffers from being allocated beyond the 32-bit
 * boundary that most devices can address. Also, don't let U-Boot use any
 * carve-out, as mentioned above. This conveniently matches the content of
 * the MEM_LAYOUT_PRIMARY data supplied by nvtboot.
 *
 * This function is called before dram_init_banksize(), so we can't simply
 * return gd->bd->bi_dram[1].start + gd->bd->bi_dram[1].size.
 */
ulong board_get_usable_ram_top(ulong total_size)
{
	carveout_info *ci;

	ci = &nvtboot_boot_arg.params.car_info[MEM_LAYOUT_PRIMARY];

	return (ci->base + ci->size) & ~(SZ_2M - 1);
}

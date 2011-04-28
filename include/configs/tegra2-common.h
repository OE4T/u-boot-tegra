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

#ifndef __TEGRA2_COMMON_H
#define __TEGRA2_COMMON_H
#include <asm/sizes.h>

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMCORTEXA9		/* This is an ARM V7 CPU core */
#define CONFIG_TEGRA2			/* in a NVidia Tegra2 core */
#define CONFIG_MACH_TEGRA_GENERIC	/* which is a Tegra generic machine */
#define CONFIG_L2_OFF			/* No L2 cache */

#define CONFIG_ENABLE_CORTEXA9		/* enable CPU (A9 complex) */

#include <asm/arch/tegra2.h>		/* get chip and board defs */

/*
 * Display CPU and Board information
 */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SKIP_RELOCATE_UBOOT
#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs */

/* Environment */
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE			0x20000	/* Total Size Environment */

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(4 << 20)	/* 4MB  */

/*
 * PllX Configuration
 */
#define CONFIG_SYS_CPU_OSC_FREQUENCY	1000000	/* Set CPU clock to 1GHz */

/*
 * NS16550 Configuration
 */
#define V_NS16550_CLK			216000000	/* 216MHz (pllp_out0) */

#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)
#define CONFIG_SYS_NS16550_CLK		V_NS16550_CLK

/*
 * select serial console configuration
 */
#define CONFIG_CONS_INDEX	1

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{4800, 9600, 19200, 38400, 57600,\
					115200}


/*
 * USB Host.
 */
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_TEGRA
#define CONFIG_USB_CONTROLLER_INSTANCES	2

/* Tegra2 requires USB buffers to be aligned to a word boundary */
#define CONFIG_USB_EHCI_DATA_ALIGN	4

/*
 * This parameter affects a TXFILLTUNING field that controls how much data is
 * sent to the latency fifo before it is sent to the wire. Without this
 * parameter, the default (2) causes occasional Data Buffer Errors in OUT
 * packets depending on the buffer address and size.
 */
#define CONFIG_USB_EHCI_TXFIFO_THRESH	10

#define CONFIG_EHCI_IS_TDI
#define CONFIG_USB_STORAGE

#define CONFIG_CMD_USB		/* USB Host support		*/

/* partition types and file systems we want */
#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_EXT2

/* support USB ethernet adapters */
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX

/* include default commands */
#include <config_cmd_default.h>

/* remove unused commands */
#undef CONFIG_CMD_FLASH		/* flinfo, erase, protect */
#undef CONFIG_CMD_FPGA		/* FPGA configuration support */
#undef CONFIG_CMD_IMI
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_NFS		/* NFS support */

/*
 * Ethernet support
 */
#define CONFIG_CMD_NET
#define CONFIG_NET_MULTI
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP

/*
 * BOOTP / TFTP options
 */
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_TFTP_TSIZE

#define CONFIG_IPADDR		10.0.0.2
#define CONFIG_SERVERIP		10.0.0.1
#define CONFIG_BOOTFILE		uImage

/*
 * We decorate the nfsroot name so that multiple users / boards can easily
 * share an NFS server:
 *   user - username, e.g. 'frank'
 *   board - board, e.g. 'seaboard'
 *   serial - serial number, e.g. '1234'
 */
#define CONFIG_ROOTPATH		"/export/nfsroot-${user}-${board}-${serial#}"
#define CONFIG_TFTPPATH		"/tftpboot/uImage-${user}-${board}-${serial#}"

/* turn on command-line edit/hist/auto */
#define CONFIG_CMDLINE_EDITING
#define CONFIG_COMMAND_HISTORY
#define CONFIG_AUTOCOMPLETE

#define CONFIG_SYS_NO_FLASH

/* Environment information */
#define CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	"console=ttyS0,115200n8\0" \
	"smpflag=smp\0" \
	"user=user\0" \
	"serial#=1\0" \
	"tftpserverip=172.22.72.144\0" \
	"nfsserverip=172.22.72.144\0" \
	"extra_bootargs=\0" \
	"platform_extras=mem= " TEGRA2_SYSMEM"\0" \
	"regen_all="\
		"setenv common_bootargs console=${console} " \
		"${platform_extras} noinitrd; " \
		"setenv bootargs ${common_bootargs} ${extra_bootargs} " \
		"${bootdev_bootargs}\0" \
	"regen_net_bootargs=setenv bootdev_bootargs " \
		"dev=/dev/nfs4 rw nfsroot=${nfsserverip}:${rootpath} " \
		"ip=dhcp; " \
		"run regen_all\0" \
	"dhcp_setup=setenv tftppath " CONFIG_TFTPPATH "; " \
		"setenv rootpath " CONFIG_ROOTPATH "; " \
		"setenv autoload n; " \
		"run regen_net_bootargs\0" \
	"dhcp_boot=run dhcp_setup; " \
		"bootp; tftpboot ${loadaddr} ${tftpserverip}:${tftppath}; " \
		"bootm ${loadaddr}\0" \

#define CONFIG_BOOTCOMMAND \
  "usb start; "\
  "if test ${ethact} != \"\"; then "\
    "run dhcp_boot ; " \
  "fi ; " \

#define CONFIG_LOADADDR		0x408000	/* def. location for kernel */
#define CONFIG_BOOTDELAY	2		/* -1 to disable auto boot */

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser */
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT		V_PROMPT
/*
 * Increasing the size of the IO buffer as default nfsargs size is more
 *  than 256 and so it is not possible to edit it
 */
#define CONFIG_SYS_CBSIZE		(256 * 2) /* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		(CONFIG_SYS_CBSIZE)

#define CONFIG_SYS_MEMTEST_START	(TEGRA2_SDRC_CS0 + 0x600000)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x100000)

#define CONFIG_SYS_LOAD_ADDR		(0xA00800)	/* default */
#define CONFIG_SYS_HZ			1000

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKBASE	0x2800000	/* 40MB */
#define CONFIG_STACKSIZE	0x20000		/* 128K regular stack*/

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		TEGRA2_SDRC_CS0
#define PHYS_SDRAM_1_SIZE	0x20000000	/* 512M */

#define CONFIG_SYS_TEXT_BASE	0x00E08000
#define CONFIG_SYS_SDRAM_BASE	PHYS_SDRAM_1

#define CONFIG_SYS_INIT_RAM_ADDR	CONFIG_STACKBASE
#define CONFIG_SYS_INIT_RAM_SIZE	CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_INIT_RAM_ADDR + \
						CONFIG_SYS_INIT_RAM_SIZE - \
						GENERATED_GBL_DATA_SIZE)

#endif /* __TEGRA2_COMMON_H */

/*
 * Copyright (c) 2011 The Chromium OS Authors.
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


/*
 * This file contains convenience functions for decoding useful and
 * enlightening information from FDTs. It is intended to be used by device
 * drivers and board-specific code within U-Boot. It aims to reduce the
 * amount of FDT munging required within U-Boot itself, so that driver code
 * changes to support FDT are minimized.
 */

#include <ns16550.h>
#include <asm/arch/clock.h>

/* A typedef for a physical address. We should move it to a generic place */
#ifdef CONFIG_PHYS_64BIT
typedef u64 addr_t;
#define ADDR_T_NONE (-1ULL)
#define addr_to_cpu(reg) be64_to_cpu(reg)
#else
typedef u32 addr_t;
#define ADDR_T_NONE (-1U)
#define addr_to_cpu(reg) be32_to_cpu(reg)
#endif

/**
 * Compat types that we know about and for which we might have drivers.
 * Each is named COMPAT_<dir>_<filename> where <dir> is the directory
 * within drivers.
 */
enum fdt_compat_id {
	COMPAT_UNKNOWN,
	COMPAT_NVIDIA_SPI_UART_SWITCH,	/* SPI / UART switch */
	COMPAT_SERIAL_NS16550,		/* NS16550 UART */
	COMPAT_NVIDIA_TEGRA250_USB,	/* Tegra 250 USB port */

	COMPAT_COUNT,
};

/* Information obtained about a UART from the FDT */
struct fdt_uart {
	addr_t reg;	/* address of registers in physical memory */
	int id;		/* id or port number (numbered from 0, default -1) */
	int reg_shift;	/* each register is (1 << reg_shift) apart */
	int baudrate;	/* baud rate, will be gd->baudrate if not defined */
	int clock_freq;	/* clock frequency, -1 if not provided */
	int multiplier;	/* divisor multiplier, default 16 */
	int divisor;	/* baud rate divisor, default calculated */
	int enabled;	/* 1 to enable, 0 to disable */
	int interrupt;	/* interrupt line */
	int silent;	/* 1 for silent UART (supresses output by default) */
	enum fdt_compat_id compat; /* our selected driver */
};

/* Information about the spi/uart switch */
struct fdt_spi_uart {
	int gpio;			/* GPIO to control switch */
	NS16550_t regs;			/* Address of UART affected */
	u32 port;			/* Port number of UART affected */
};

enum {
	FDT_GPIO_NONE = 255,	/* an invalid GPIO used to end our list */
	FDT_GPIO_MAX  = 224,	/* maximum value GPIO number on Tegra2 */

	FDT_GPIO_OUTPUT	= 1 << 0,	/* set as output (else input) */
	FDT_GPIO_HIGH	= 1 << 1,	/* set output as high (else low) */
};

/* This is the state of a GPIO pin. For now it only handles output pins */
struct fdt_gpio_state {
	u8 gpio;		/* GPIO number, or FDT_GPIO_NONE if none */
	u8 flags;		/* FDT_GPIO_... flags */
};

/* This tells us whether a fdt_gpio_state record is valid or not */
#define fdt_gpio_isvalid(gpio) ((gpio)->gpio != FDT_GPIO_NONE)

#define FDT_LCD_GPIOS	5

enum {
	FDT_LCD_TIMING_REF_TO_SYNC,
	FDT_LCD_TIMING_SYNC_WIDTH,
	FDT_LCD_TIMING_BACK_PORCH,
	FDT_LCD_TIMING_FRONT_PORCH,

	FDT_LCD_TIMING_COUNT,
};

/* Information about the LCD */
struct fdt_lcd {
	addr_t reg;		/* address of registers in physical memory */
	int width;		/* width in pixels */
	int height;		/* height in pixels */
	int bpp;		/* number of bits per pixel */

	/*
	 * log2 of number of bpp, in general, unless it bpp is 24 in which
	 * case this field holds 24 also! This is a U-Boot thing.
	 */
	int log2_bpp;
	struct fdt_gpio_state gpios[FDT_LCD_GPIOS]; /* state of output GPIOS */
	struct pwfm_ctlr *pwfm;	/* PWM to use for backlight */
	struct disp_ctlr *disp;	/* Display controller to use */
	addr_t frame_buffer;	/* Address of frame buffer */
	unsigned pixel_clock;	/* Pixel clock in Hz */
	int horiz_timing[FDT_LCD_TIMING_COUNT];	/* Horizontal timing */
	int vert_timing[FDT_LCD_TIMING_COUNT];	/* Vertical timing */
};

/* Parameters we need for USB */
enum {
	PARAM_DIVN,			/* PLL FEEDBACK DIVIDer */
	PARAM_DIVM,			/* PLL INPUT DIVIDER */
	PARAM_DIVP,			/* POST DIVIDER (2^N) */
	PARAM_CPCON,			/* BASE PLLC CHARGE Pump setup ctrl */
	PARAM_LFCON,			/* BASE PLLC LOOP FILter setup ctrl */
	PARAM_ENABLE_DELAY_COUNT,	/* PLL-U Enable Delay Count */
	PARAM_STABLE_COUNT,		/* PLL-U STABLE count */
	PARAM_ACTIVE_DELAY_COUNT,	/* PLL-U Active delay count */
	PARAM_XTAL_FREQ_COUNT,		/* PLL-U XTAL frequency count */
	PARAM_DEBOUNCE_A_TIME,		/* 10MS DELAY for BIAS_DEBOUNCE_A */
	PARAM_BIAS_TIME,		/* 20US DELAY AFter bias cell op */

	PARAM_COUNT
};

/* Information about USB */
struct fdt_usb {
	struct usb_ctlr *reg;	/* address of registers in physical memory */
	int params[PARAM_COUNT]; /* timing parameters */
	int host_mode;		/* 1 if port has host mode, else 0 */
	int utmi;		/* 1 if port has external tranceiver, else 0 */
	int enabled;		/* 1 to enable, 0 to disable */
	enum periph_id periph_id;/* peripheral id */
};

/**
 * Return information from the FDT about the console UART. This looks for
 * an alias node called 'console' which must point to a UART. It then reads
 * out the following attributes:
 *
 *	reg
 *	port
 *	clock-frequency
 *	reg-shift (default 2)
 *	baudrate (default 115200)
 *	multiplier (default 16)
 *	divisor (calculated by default)
 *	enabled (default 1)
 *	interrupts (returns first in interrupt list, -1 if none)
 *	silent (default 0)
 *
 * The divisor calculation is performed by calling
 * fdt_decode_uart_calc_divisor() automatically once the information is
 * available. Therefore callers should be able to simply use the ->divisor
 * field as their baud rate divisor.
 *
 * @param blob	FDT blob to use
 * @param uart	struct to use to return information
 * @param default_baudrate  Baud rate to use if none defined in FDT
 * @return 0 on success, -ve on error, in which case uart is unchanged
 */
int fdt_decode_uart_console(const void *blob, struct fdt_uart *uart,
		int default_baudrate);

/**
 * Calculate the divisor to use for the uart. This is done based on the
 * formula (uart clock / (baudrate * multiplier). The divisor is updated
 * in the uart structure ready for use.
 *
 * @param uart	uart structure to examine and update
 */
void fdt_decode_uart_calc_divisor(struct fdt_uart *uart);

/**
 * Find the compat id for a node. This looks at the 'compat' property of
 * the node and looks up the corresponding ftp_compat_id. This is used for
 * determining which driver will implement the decide described by the node.
 */
enum fdt_compat_id fdt_decode_lookup(const void *blob, int node);

/**
 * Find the next node which is compatible with the given id. Pass in a node
 * of 0 the first time.
 *
 * @param blob		FDT blob to use
 * @param node		Node offset to start search after
 * @param id		Compatible ID to look for
 * @return offset of next compatible node, or -FDT_ERR_NOTFOUND if no more
 */
int fdt_decode_next_compatible(const void *blob, int node,
		enum fdt_compat_id id);

/**
 * Find the next numbered alias for a peripheral. This is used to enumerate
 * all the peripherals of a certain type.
 *
 * Do the first call with *upto = 0. Assuming /aliases/<name>0 exists then
 * this function will return a pointer to the node the alias points to, and
 * then update *upto to 1. Next time you call this function, the next node
 * will be returned.
 *
 * All nodes returned will match the compatible ID, as it is assumed that
 * all peripherals use the same driver.
 *
 * @param blob		FDT blob to use
 * @param name		Root name of alias to search for
 * @param id		Compatible ID to look for
 * @return offset of next compatible node, or -FDT_ERR_NOTFOUND if no more
 */
int fdt_decode_next_alias(const void *blob, const char *name,
		enum fdt_compat_id id, int *upto);

/**
 * Returns information from the FDT about the SPI / UART switch on tegra
 * platforms.
 *
 * @param blob		FDT blob to use
 * @param config	structure to use to return information
 * @returns 0 on success, -ve on error, in which case config is unchanged
 */
int fdt_decode_get_spi_switch(const void *blob, struct fdt_spi_uart *config);

/**
 * Set up a GPIO pin according to the provided gpio. This sets it to either
 * input or output. If an output, then the defined value is assigned.
 *
 * @param gpio		GPIO to set up
 */
void fdt_setup_gpio(struct fdt_gpio_state *gpio);

/**
 * Set up GPIO pins according to the list provided.
 *
 * @param gpio_list	List of GPIOs to set up
 */
void fdt_setup_gpios(struct fdt_gpio_state *gpio_list);

/**
 * Converts an FDT GPIO record into a number and returns it.
 *
 * @param gpio		GPIO to check
 * @return gpio number, or -1 if none or not valid
 */
int fdt_get_gpio_num(struct fdt_gpio_state *gpio);

/**
 * Returns information from the FDT about the LCD display. This function reads
 * out the following attributes:
 *
 *	reg		physical address of display peripheral
 *	width		width in pixels
 *	height		height in pixels
 *	bits_per_pixel	put in bpp
 *	log2_bpp	log2 of bpp (so bpp = 2 ^ log2_bpp)
 *	pwfm		PWFM to use
 *	display		Display to use (SOC peripheral)
 *	frame-buffer	Frame buffer address (can be omitted since U-Boot will
 *			normally set this)
 *	pixel_clock	Pixel clock in Hz
 *	horiz_timing	ref_to_sync, sync_width. back_porch, front_porch
 *	vert_timing	ref_to_sync, sync_width. back_porch, front_porch
 *	gpios		list of GPIOs to make as outputs, along with 0/1 value
 *
 * @param blob		FDT blob to use
 * @param config	structure to use to return information
 * @returns 0 on success, -ve on error, in which case config may or may not be
 *			unchanged. If the node is present but expected data is
 *			missing then this will generally return
 *			-FDT_ERR_MISSING.
 */
int fdt_decode_lcd(const void *blob, struct fdt_lcd *config);

/**
 * Returns information from the FDT about the USB port. This function reads
 * out the following attributes:
 *
 *	reg
 *	params
 *	host-mode
 *	utmi
 *	periph-id
 *	enabled
 *
 * The params are read out according to the supplied clock frequency. This is
 * done by looking for a compatible 'nvidia,tegra250-usbparams' node with
 * osc-frequency set to the given value.
 *
 * @param blob		FDT blob to use
 * @param node		Node to read from
 * @param osc_frequency_mhz	Current oscillator frequency
 * @param config	structure to use to return information
 * @returns 0 on success, -ve on error, in which case config may or may not be
 *			unchanged. If the node is present but expected data is
 *			missing then this will generally return
 *			-FDT_ERR_MISSING.
 */
int fdt_decode_usb(const void *blob, int node, unsigned osc_frequency_mhz,
		struct fdt_usb *config);

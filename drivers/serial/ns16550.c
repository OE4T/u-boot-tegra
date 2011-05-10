/*
 * COM1 NS16550 support
 * originally from linux source (arch/powerpc/boot/ns16550.c)
 * modified to use CONFIG_SYS_ISA_MEM and new defines
 */

#include <common.h>
#include <config.h>
#include <common.h>
#include <ns16550.h>
#include <watchdog.h>
#include <linux/types.h>
#include <asm/io.h>
#include "uart-spi-fix.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_LCRVAL UART_LCR_8N1		/* 8 data, 1 stop, no parity */
#define UART_MCRVAL (UART_MCR_DTR | \
		     UART_MCR_RTS)		/* RTS/DTR */
#define UART_FCRVAL (UART_FCR_FIFO_EN |	\
		     UART_FCR_RXSR |	\
		     UART_FCR_TXSR)		/* Clear & enable FIFOs */
#ifdef CONFIG_SYS_NS16550_PORT_MAPPED
#define serial_out(x,y)	outb(x,(ulong)y)
#define serial_in(y)	inb((ulong)y)
#else
#define serial_out(x,y) writeb(x,y)
#define serial_in(y) 	readb(y)
#endif

#ifndef CONFIG_SYS_NS16550_IER
#define CONFIG_SYS_NS16550_IER  0x00
#endif /* CONFIG_SYS_NS16550_IER */

void NS16550_init (NS16550_t com_port, int baud_divisor)
{
	serial_out(CONFIG_SYS_NS16550_IER, &com_port->ier);
#if defined(CONFIG_OMAP) && !defined(CONFIG_OMAP3_ZOOM2)
	serial_out(0x7, &com_port->mdr1);	/* mode select reset TL16C750*/
#endif
	serial_out(UART_LCR_BKSE | UART_LCRVAL, (ulong)&com_port->lcr);
	serial_out(0, &com_port->dll);
	serial_out(0, &com_port->dlm);
	serial_out(UART_LCRVAL, &com_port->lcr);
	serial_out(UART_MCRVAL, &com_port->mcr);
	serial_out(UART_FCRVAL, &com_port->fcr);
	serial_out(UART_LCR_BKSE | UART_LCRVAL, &com_port->lcr);
	serial_out(baud_divisor & 0xff, &com_port->dll);
	serial_out((baud_divisor >> 8) & 0xff, &com_port->dlm);
	serial_out(UART_LCRVAL, &com_port->lcr);
#if defined(CONFIG_OMAP) && !defined(CONFIG_OMAP3_ZOOM2)
#if defined(CONFIG_APTIX)
	serial_out(3, &com_port->mdr1);	/* /13 mode so Aptix 6MHz can hit 115200 */
#else
	serial_out(0, &com_port->mdr1);	/* /16 is proper to hit 115200 with 48MHz */
#endif
#endif /* CONFIG_OMAP */
}

#ifndef CONFIG_NS16550_MIN_FUNCTIONS
void NS16550_reinit (NS16550_t com_port, int baud_divisor)
{
	serial_out(CONFIG_SYS_NS16550_IER, &com_port->ier);
	serial_out(UART_LCR_BKSE | UART_LCRVAL, &com_port->lcr);
	serial_out(0, &com_port->dll);
	serial_out(0, &com_port->dlm);
	serial_out(UART_LCRVAL, &com_port->lcr);
	serial_out(UART_MCRVAL, &com_port->mcr);
	serial_out(UART_FCRVAL, &com_port->fcr);
	serial_out(UART_LCR_BKSE, &com_port->lcr);
	serial_out(baud_divisor & 0xff, &com_port->dll);
	serial_out((baud_divisor >> 8) & 0xff, &com_port->dlm);
	serial_out(UART_LCRVAL, &com_port->lcr);
}

#endif /* CONFIG_NS16550_MIN_FUNCTIONS */

void NS16550_putc(NS16550_t regs, char c)
{
	uart_enable(regs);
	while ((serial_in(&regs->lsr) & UART_LSR_THRE) == 0);
	serial_out(c, &regs->thr);

	/*
	 * Call watchdog_reset() upon newline. This is done here in putc
	 * since the environment code uses a single puts() to print the complete
	 * environment upon "printenv". So we can't put this watchdog call
	 * in puts().
	 */
	if (c == '\n')
		WATCHDOG_RESET();
}

#ifndef CONFIG_NS16550_MIN_FUNCTIONS

static char NS16550_raw_getc(NS16550_t regs)
{
	uart_enable(regs);
	while ((serial_in(&regs->lsr) & UART_LSR_DR) == 0) {
#ifdef CONFIG_USB_TTY
		extern void usbtty_poll(void);
		usbtty_poll();
#endif
		WATCHDOG_RESET();
	}
	return serial_in(&regs->rbr);
}

static int NS16550_raw_tstc(NS16550_t regs)
{
	uart_enable(regs);
	return ((serial_in(&regs->lsr) & UART_LSR_DR) != 0);
}

#ifdef CONFIG_NS16550_BUFFER_READS

#define BUF_SIZE 256
#define NUM_PORTS 4

struct ns16550_priv {
	char buf[BUF_SIZE];
	unsigned int head, tail;
};

static struct ns16550_priv rxstate[NUM_PORTS];

static void enqueue(unsigned int port, char ch)
{
	/* If queue is full, drop the character. */
	if ((rxstate[port].head - rxstate[port].tail - 1) % BUF_SIZE == 0)
		return;

	rxstate[port].buf[rxstate[port].tail] = ch;
	rxstate[port].tail = (rxstate[port].tail + 1) % BUF_SIZE;
}

static int dequeue(unsigned int port, char *ch)
{
	/* Empty queue? */
	if (rxstate[port].head == rxstate[port].tail)
		return 0;

	*ch = rxstate[port].buf[rxstate[port].head];
	rxstate[port].head = (rxstate[port].head + 1) % BUF_SIZE;
	return 1;
}

static void fill_rx_buf(NS16550_t regs, unsigned int port)
{
	uart_enable(regs);
	while ((serial_in(&regs->lsr) & UART_LSR_DR) != 0)
		enqueue(port, serial_in(&regs->rbr));
}

char NS16550_getc(NS16550_t regs, unsigned int port)
{
	char ch;

	if (port >= NUM_PORTS || !(gd->flags & GD_FLG_RELOC))
		return NS16550_raw_getc(regs);

	do  {
#ifdef CONFIG_USB_TTY
		extern void usbtty_poll(void);
		usbtty_poll();
#endif
		fill_rx_buf(regs, port);
		WATCHDOG_RESET();
	} while (!dequeue(port, &ch));

	return ch;
}

int NS16550_tstc(NS16550_t regs, unsigned int port)
{
	if (port >= NUM_PORTS || !(gd->flags & GD_FLG_RELOC))
		return NS16550_raw_tstc(regs);

	fill_rx_buf(regs, port);

	return rxstate[port].head != rxstate[port].tail;
}

#else /* CONFIG_NS16550_BUFFER_READS */

char NS16550_getc(NS16550_t regs, unsigned int port)
{
	return NS16550_raw_getc(regs);
}

int NS16550_tstc(NS16550_t regs, unsigned int port)
{
	return NS16550_raw_tstc(regs);
}

#endif /* CONFIG_NS16550_BUFFER_READS */

/* Clear the UART's RX FIFO */

void NS16550_clear(NS16550_t regs, unsigned port)
{
	/* Reset RX fifo */
	serial_out(UART_FCR_FIFO_EN | UART_FCR_RXSR, &regs->fcr);

	/* Remove any pending characters */
	while (NS16550_raw_tstc(regs))
		NS16550_raw_getc(regs);
}

/* Wait for the UART's output buffer and holding register to drain */

void NS16550_drain(NS16550_t regs, unsigned port)
{
	u32 mask = UART_LSR_TEMT | UART_LSR_THRE;

	/* Wait for the UART to finish sending */
	while ((serial_in(&regs->lsr) & mask) != mask)
		udelay(100);
#ifdef CONFIG_NS16550_BUFFER_READS
	/* Quickly grab any incoming data while we can */
	fill_rx_buf(regs, port);
#endif
}

#endif /* CONFIG_NS16550_MIN_FUNCTIONS */

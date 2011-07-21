/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Copyright (c) 2010-2011 NVIDIA Corporation
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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra_i2c.h>

static unsigned int i2c_bus_num;

struct i2c_bus {
	int			id;
	enum periph_id		periph_id;
	int			pinmux_config;
	struct i2c_control	*control;
	struct i2c_ctlr		*regs;
};

struct i2c_bus i2c_controllers[CONFIG_SYS_MAX_I2C_BUS];

static void i2c_pin_mux_select(struct i2c_bus *i2c_bus, int pinmux_config)
{
	switch (i2c_bus->periph_id) {
	case PERIPH_ID_DVC_I2C:	/* DVC I2C (I2CP) */
		/* there is only one selection, pinmux_config is ignored */
		pinmux_set_func(PINGRP_I2CP, PMUX_FUNC_I2C);
		break;
	case PERIPH_ID_I2C1: /* I2C1 */
		/* support pinmux_config of 1 for now, */
		pinmux_set_func(PINGRP_RM, PMUX_FUNC_I2C);
		break;
	case PERIPH_ID_I2C2: /* I2C2 */
		switch (pinmux_config) {
		case 1:	/* DDC pin group, select I2C2 */
			pinmux_set_func(PINGRP_DDC, PMUX_FUNC_I2C2);
			/* PTA to HDMI */
			pinmux_set_func(PINGRP_PTA, PMUX_FUNC_HDMI);
			break;
		case 2:	/* PTA pin group, select I2C2 */
			pinmux_set_func(PINGRP_PTA, PMUX_FUNC_I2C2);
			/* set DDC_SEL to RSVDx (RSVD2 works for now) */
			pinmux_set_func(PINGRP_DDC, PMUX_FUNC_RSVD2);
			break;
		default:
			printf("bad pinmux_config(%d): pinmux select ignored\n",
			       pinmux_config);
			break;
		}
		break;
	case PERIPH_ID_I2C3: /* I2C3 */
		/* support pinmux_config of 1 for now */
		pinmux_set_func(PINGRP_DTF, PMUX_FUNC_I2C3);
		break;
	default:
		break;
	}
}

/**
 * tristate : 0 - NORMAL
 *	      1 - TRISTATE
 */
static void i2c_pin_mux_tristate(struct i2c_bus *i2c_bus, int pinmux_config,
				 int tristate)
{
	enum pmux_pingrp pin;

	switch (i2c_bus->periph_id) {
	case PERIPH_ID_DVC_I2C:	/* DVC I2C (I2CP) */
		/* there is only one selection, pinmux_config is ignored */
		pin = PINGRP_I2CP;
		break;
	case PERIPH_ID_I2C1:	/* I2C1 */
		/* support pinmux_config of 1 for now */
		pin = PINGRP_RM;
		break;
	case PERIPH_ID_I2C2:	/* I2C2 */
		switch (pinmux_config) {
		case 1:	/* DDC pin group */
			pin = PINGRP_DDC;
			break;
		case 2:	/* PTA pin group */
			pin = PINGRP_PTA;
			break;
		default:
			printf("bad pinmux_config(%d): tristate ignored\n",
			       pinmux_config);
			return;
		}
		break;
	case PERIPH_ID_I2C3:	/* I2C3 */
		/* support pinmux_config of 1 for now */
		pin = PINGRP_DTF;
		break;
	default:
		printf("invalid controller(%d): tristate ignored\n",
		       i2c_bus->id);
		return;
	}

	pinmux_set_tristate(pin, tristate);
}

static void set_packet_mode(struct i2c_bus *i2c_bus)
{
	u32 config;

	config = bf_pack(I2C_CNFG_NEW_MASTER_FSM, 1) |
		 bf_pack(I2C_CNFG_PACKET_MODE, 1);

	if (i2c_bus->periph_id == PERIPH_ID_DVC_I2C) {
		struct dvc_ctlr *dvc = (struct dvc_ctlr *)i2c_bus->regs;

		writel(config, &dvc->cnfg);
	} else {
		writel(config, &i2c_bus->regs->cnfg);
		/*
		 * program I2C_SL_CNFG.NEWSL to ENABLE. This fixes probe
		 * issues, i.e., some slaves may be wrongly detected.
		 */
		bf_writel(I2C_SL_CNFG_NEWSL, 1, &i2c_bus->regs->sl_cnfg);
	}
}

static void i2c_reset_controller(struct i2c_bus *i2c_bus)
{
	/* Reset I2C controller. */
	reset_periph(i2c_bus->periph_id, 1);

	/* re-program config register to packet mode */
	set_packet_mode(i2c_bus);
}

static void i2c_init_controller(struct i2c_bus *i2c_bus, u32 clock_khz)
{
	/* TODO: Fix bug which makes us need to do this */
	clock_start_periph_pll(i2c_bus->periph_id, CLOCK_ID_OSC,
			       clock_khz * 1000 * (8 * 2 - 1));

	/* Reset I2C controller. */
	i2c_reset_controller(i2c_bus);

	/* Configure I2C controller. */
	if (i2c_bus->periph_id == PERIPH_ID_DVC_I2C) {	/* only for DVC I2C */
		struct dvc_ctlr *dvc = (struct dvc_ctlr *)i2c_bus->regs;

		bf_writel(DVC_CTRL_REG3_I2C_HW_SW_PROG, 1, &dvc->ctrl3);
	}

	i2c_pin_mux_select(i2c_bus, i2c_bus->pinmux_config);
	i2c_pin_mux_tristate(i2c_bus, i2c_bus->pinmux_config, 0);
}

static void send_packet_headers(
	struct i2c_bus *i2c_bus,
	struct i2c_trans_info *trans,
	u32 packet_id)
{
	u32 data;

	/* prepare header1: Header size = 0 Protocol = I2C, pktType = 0 */
	data = bf_pack(PKT_HDR1_PROTOCOL, PROTOCOL_TYPE_I2C) |
				bf_pack(PKT_HDR1_PKT_ID, packet_id) |
				bf_pack(PKT_HDR1_CTLR_ID, i2c_bus->id);
	writel(data, &i2c_bus->control->tx_fifo);
	debug("pkt header 1 sent (0x%x)\n", data);

	/* prepare header2 */
	data = bf_pack(PKT_HDR2_PAYLOAD_SIZE, (trans->num_bytes - 1));
	writel(data, &i2c_bus->control->tx_fifo);
	debug("pkt header 2 sent (0x%x)\n", data);

	/* prepare IO specific header: configure the slave address */
	data = bf_pack(PKT_HDR3_SLAVE_ADDR, trans->address);

	/* Enable Read if it is not a write transaction */
	if (!(trans->flags & I2C_IS_WRITE))
		bf_update(PKT_HDR3_READ_MODE, data, 1);

	/* Write I2C specific header */
	writel(data, &i2c_bus->control->tx_fifo);
	debug("pkt header 3 sent (0x%x)\n", data);
}

static int wait_for_tx_fifo_empty(struct i2c_control *control)
{
	u32 count;
	int timeout_us = I2C_TIMEOUT_USEC;

	while (timeout_us >= 0) {
		count = bf_readl(TX_FIFO_EMPTY_CNT, &control->fifo_status);
		if (count == I2C_FIFO_DEPTH)
			return 1;
		udelay(10);
		timeout_us -= 10;
	};

	return 0;
}

static int wait_for_rx_fifo_notempty(struct i2c_control *control)
{
	u32 count;
	int timeout_us = I2C_TIMEOUT_USEC;

	while (timeout_us >= 0) {
		count = bf_readl(RX_FIFO_FULL_CNT, &control->fifo_status);
		if (count)
			return 1;
		udelay(10);
		timeout_us -= 10;
	};

	return 0;
}

static int wait_for_transfer_complete(struct i2c_control *control)
{
	int int_status;
	int timeout_us = I2C_TIMEOUT_USEC;

	while (timeout_us >= 0) {
		int_status = readl(&control->int_status);
		if (bf_unpack(I2C_INT_NO_ACK, int_status))
			return -int_status;
		if (bf_unpack(I2C_INT_ARBITRATION_LOST, int_status))
			return -int_status;
		if (bf_unpack(I2C_INT_PACKET_XFER_COMPLETE, int_status))
			return 0;

		udelay(10);
		timeout_us -= 10;
	};

	return -1;
}

static int send_recv_packets(
	struct i2c_bus *i2c_bus,
	struct i2c_trans_info *trans)
{
	struct i2c_control *control = i2c_bus->control;
	u32 int_status;
	u32 words;
	u8 *dptr;
	u32 local;
	uchar last_bytes;
	int error = 0;
	int is_write = trans->flags & I2C_IS_WRITE;

	/* clear status from previous transaction, XFER_COMPLETE, NOACK, etc. */
	int_status = readl(&control->int_status);
	writel(int_status, &control->int_status);

	send_packet_headers(i2c_bus, trans, 1);

	words = BYTES_TO_WORDS(trans->num_bytes);
	last_bytes = trans->num_bytes & 3;
	dptr = trans->buf;

	while (words) {
		if (is_write) {
			/* deal with word alignment */
			if ((unsigned)dptr & 3) {
				memcpy(&local, dptr, sizeof(u32));
				writel(local, &control->tx_fifo);
				debug("pkt data sent (0x%x)\n", local);
			} else {
				writel(*(u32 *)dptr, &control->tx_fifo);
				debug("pkt data sent (0x%x)\n", *(u32 *)dptr);
			}
			if (!wait_for_tx_fifo_empty(control)) {
				error = -1;
				goto exit;
			}
		} else {
			if (!wait_for_rx_fifo_notempty(control)) {
				error = -1;
				goto exit;
			}
			/*
			 * for the last word, we read into our local buffer,
			 * in case that caller did not provide enough buffer.
			 */
			local = readl(&control->rx_fifo);
			if ((words == 1) && last_bytes)
				memcpy(dptr, (char *)&local, last_bytes);
			else if ((unsigned)dptr & 3)
				memcpy(dptr, &local, sizeof(u32));
			else
				*(u32 *)dptr = local;
			debug("pkt data received (0x%x)\n", local);
		}
		words--;
		dptr += sizeof(u32);
	}

	if (wait_for_transfer_complete(control)) {
		error = -1;
		goto exit;
	}
	return 0;
exit:
	/* error, reset the controller. */
	i2c_reset_controller(i2c_bus);

	return error;
}

static int tegra2_i2c_write_data(u32 addr, u8 *data, u32 len)
{
	int error;
	struct i2c_trans_info trans_info;

	trans_info.address = addr;
	trans_info.buf = data;
	trans_info.flags = I2C_IS_WRITE;
	trans_info.num_bytes = len;
	trans_info.is_10bit_address = 0;

	error = send_recv_packets(&i2c_controllers[i2c_bus_num], &trans_info);
	if (error)
		debug("tegra2_i2c_write_data: Error (%d) !!!\n", error);

	return error;
}

static int tegra2_i2c_read_data(u32 addr, u8 *data, u32 len)
{
	int error;
	struct i2c_trans_info trans_info;

	trans_info.address = addr | 1;
	trans_info.buf = data;
	trans_info.flags = 0;
	trans_info.num_bytes = len;
	trans_info.is_10bit_address = 0;

	error = send_recv_packets(&i2c_controllers[i2c_bus_num], &trans_info);
	if (error)
		debug("tegra2_i2c_read_data: Error (%d) !!!\n", error);

	return error;
}

void i2c_init_board(void)
{
	struct i2c_bus *i2c_bus;
	int i;

	enum periph_id i2c_periph_ids[CONFIG_SYS_MAX_I2C_BUS] = {
		PERIPH_ID_DVC_I2C,
		PERIPH_ID_I2C1,
		PERIPH_ID_I2C2,
		PERIPH_ID_I2C3
	};

	u32 *i2c_bus_base[CONFIG_SYS_MAX_I2C_BUS] = {
		(u32 *)NV_PA_DVC_BASE,
		(u32 *)NV_PA_I2C1_BASE,
		(u32 *)NV_PA_I2C2_BASE,
		(u32 *)NV_PA_I2C3_BASE
	};

	/* pinmux_configs based on the pinmux configuration */
	int pinmux_configs[CONFIG_SYS_MAX_I2C_BUS] = {
		CONFIG_I2CP_PIN_MUX,	/* for I2CP (DVC I2C) */
		CONFIG_I2C1_PIN_MUX,	/* for I2C1 */
		CONFIG_I2C2_PIN_MUX,	/* for I2C2 */
		CONFIG_I2C3_PIN_MUX	/* for I2C3 */
	};

	/* build the i2c_controllers[] for each controller */
	for (i = 0; i < CONFIG_SYS_MAX_I2C_BUS; ++i) {
		i2c_bus = &i2c_controllers[i];
		i2c_bus->id = i;
		i2c_bus->periph_id = i2c_periph_ids[i];
		i2c_bus->pinmux_config = pinmux_configs[i];
		i2c_bus->regs = (struct i2c_ctlr *)i2c_bus_base[i];

		if (i2c_bus->periph_id == PERIPH_ID_DVC_I2C)
			i2c_bus->control =
				&((struct dvc_ctlr *)i2c_bus->regs)->control;
		else
			i2c_bus->control = &i2c_bus->regs->control;

		i2c_init_controller(i2c_bus, I2CSPEED_KHZ);
	}
}

void i2c_init(int speed, int slaveaddr)
{
	debug("i2c_init(speed=%u, slaveaddr=0x%x)\n", speed, slaveaddr);
}

/* i2c write version without the register address */
int i2c_write_data(uchar chip, uchar *buffer, int len)
{
	int rc;

	debug("i2c_write_data: chip=0x%x, len=0x%x\n", chip, len);
	debug("write_data: ");
	/* use rc for counter */
	for (rc = 0; rc < len; ++rc)
		debug(" 0x%02x", buffer[rc]);
	debug("\n");

	rc = tegra2_i2c_write_data(I2C_ADDR_ON_BUS(chip), buffer, len);
	if (rc)
		debug("i2c_write_data(): rc=%d\n", rc);

	return rc;
}

/* i2c read version without the register address */
int i2c_read_data(uchar chip, uchar *buffer, int len)
{
	int rc;

	debug("inside i2c_read_data():\n");
	rc = tegra2_i2c_read_data(I2C_ADDR_ON_BUS(chip), buffer, len);
	if (rc) {
		debug("i2c_read_data(): rc=%d\n", rc);
		return rc;
	}

	debug("i2c_read_data: ");
	/* reuse rc for counter*/
	for (rc = 0; rc < len; ++rc)
		debug(" 0x%02x", buffer[rc]);
	debug("\n");

	return 0;
}

/* Probe to see if a chip is present. */
int i2c_probe(uchar chip)
{
	int rc;
	uchar reg;

	debug("i2c_probe: addr=0x%x\n", chip);
	reg = 0;
	rc = i2c_write_data(chip, &reg, 1);
	if (rc) {
		debug("Error probing 0x%x.\n", chip);
		return 1;
	}
	return 0;
}

static int i2c_addr_ok(const uint addr, const int alen)
{
	if (alen < 0 || alen > sizeof(addr))
		return 0;
	if (alen != sizeof(addr)) {
		uint max_addr = (1 << (8 * alen)) - 1;
		if (addr > max_addr)
			return 0;
	}
	return 1;
}

/* Read bytes */
int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	uint offset;
	int i;

	debug("i2c_read: chip=0x%x, addr=0x%x, len=0x%x\n",
				chip, addr, len);
	if (!i2c_addr_ok(addr, alen)) {
		debug("i2c_read: Bad address %x.%d.\n", addr, alen);
		return 1;
	}
	for (offset = 0; offset < len; offset++) {
		if (alen) {
			uchar data[alen];
			for (i = 0; i < alen; i++) {
				data[alen - i - 1] =
					(addr + offset) >> (8 * i);
			}
			if (i2c_write_data(chip, data, alen)) {
				debug("i2c_read: error sending (0x%x)\n",
					addr);
				return 1;
			}
		}
		if (i2c_read_data(chip, buffer + offset, 1)) {
			debug("i2c_read: error reading (0x%x)\n", addr);
			return 1;
		}
	}

	return 0;
}

/* Write bytes */
int i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	uint offset;
	int i;

	debug("i2c_write: chip=0x%x, addr=0x%x, len=0x%x\n",
				chip, addr, len);
	if (!i2c_addr_ok(addr, alen)) {
		debug("i2c_write: Bad address %x.%d.\n", addr, alen);
		return 1;
	}
	for (offset = 0; offset < len; offset++) {
		uchar data[alen + 1];
		for (i = 0; i < alen; i++)
			data[alen - i - 1] = (addr + offset) >> (8 * i);
		data[alen] = buffer[offset];
		if (i2c_write_data(chip, data, alen + 1)) {
			debug("i2c_write: error sending (0x%x)\n", addr);
			return 1;
		}
	}

	return 0;
}

#if defined(CONFIG_I2C_MULTI_BUS)
/*
 * Functions for multiple I2C bus handling
 */
unsigned int i2c_get_bus_num(void)
{
	return i2c_bus_num;
}

int i2c_set_bus_num(unsigned int bus)
{
	if (bus >= CONFIG_SYS_MAX_I2C_BUS)
		return -1;
	i2c_bus_num = bus;

	return 0;
}
#endif


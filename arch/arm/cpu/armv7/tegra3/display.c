/*
 *  (C) Copyright 2010
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

#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/tegra.h>
#include <asm/arch/display.h>
#include <asm/arch/dc.h>
#include <common.h>
#include "fdt_decode.h"

static void update_window(struct dc_ctlr *dc, struct disp_ctl_win *win)
{
	unsigned h_dda, v_dda;
	unsigned long val;

	bf_writel(WINDOW_A_SELECT, 1, &dc->cmd.disp_win_header);

	writel(win->fmt, &dc->win.color_depth);
	bf_enum_writel(BYTE_SWAP, NOSWAP, &dc->win.byte_swap);

	val = bf_pack(H_POSITION, win->out_x);
	val |= bf_pack(V_POSITION, win->out_y);
	writel(val, &dc->win.pos);

	val = bf_pack(H_SIZE, win->out_w);
	val |= bf_pack(V_SIZE, win->out_h);
	writel(val, &dc->win.size);

	val = bf_pack(H_PRESCALED_SIZE, win->w * win->bpp / 8);
	val |= bf_pack(V_PRESCALED_SIZE, win->h);
	writel(val, &dc->win.prescaled_size);

	writel(0, &dc->win.h_initial_dda);
	writel(0, &dc->win.v_initial_dda);

	h_dda = (win->w * 0x1000) / max(win->out_w - 1, 1);
	v_dda = (win->h * 0x1000) / max(win->out_h - 1, 1);

	val = bf_pack(H_DDA_INC, h_dda);
	val |= bf_pack(V_DDA_INC, v_dda);
	writel(val, &dc->win.dda_increment);

	writel(win->stride, &dc->win.line_stride);
	writel(0, &dc->win.buf_stride);

	val = bf_ones(WIN_ENABLE);
	if (win->bpp < 24)
		val |= bf_ones(COLOR_EXPAND);
	writel(val, &dc->win.win_opt);

	writel((unsigned long)win->phys_addr, &dc->winbuf.start_addr);
	writel(win->x, &dc->winbuf.addr_h_offset);
	writel(win->y, &dc->winbuf.addr_v_offset);

	writel(0xff00, &dc->win.blend_nokey);
	writel(0xff00, &dc->win.blend_1win);

	val = bf_ones(GENERAL_ACT_REQ) | bf_ones(WIN_A_ACT_REQ);
	val |= bf_ones(GENERAL_UPDATE) | bf_ones(WIN_A_UPDATE);
	writel(val, &dc->cmd.state_ctrl);
}

static void write_pair(struct fdt_lcd *config, int item, u32 *reg)
{
	writel(config->horiz_timing[item] |
			(config->vert_timing[item] << 16), reg);
}

static int update_display_mode(struct dc_disp_reg *disp,
		struct fdt_lcd *config)
{
	unsigned long val;
	unsigned long rate;
	unsigned long div;

	writel(0x0, &disp->disp_timing_opt);
	write_pair(config, FDT_LCD_TIMING_REF_TO_SYNC, &disp->ref_to_sync);
	write_pair(config, FDT_LCD_TIMING_SYNC_WIDTH, &disp->sync_width);
	write_pair(config, FDT_LCD_TIMING_BACK_PORCH, &disp->back_porch);
	write_pair(config, FDT_LCD_TIMING_FRONT_PORCH, &disp->front_porch);

	writel(config->width | (config->height << 16), &disp->disp_active);

	val = bf_pack(DE_SELECT, DE_SELECT_ACTIVE);
	val |= bf_enum(DE_CONTROL, NORMAL);
	writel(val, &disp->data_enable_opt);

	val = bf_enum(DATA_FORMAT, DF1P1C);
	val |= bf_enum(DATA_ALIGNMENT, MSB);
	val |= bf_enum(DATA_ORDER, RED_BLUE);
	writel(val, &disp->disp_interface_ctrl);

	/*
	 * The pixel clock divider is in 7.1 format (where the bottom bit
	 * represents 0.5). Here we calculate the divider needed to get from
	 * the display clock (typically 216MHz) to the pixel clock. We round
	 * up or down as requried.
	 */
	rate = clock_get_periph_rate(PERIPH_ID_DISP1, CLOCK_ID_PERIPH);
	div = ((rate * 2 + config->pixel_clock / 2) / config->pixel_clock) - 2;
	debug("Display clock %lu, divider %lu\n", rate, div);

	writel(0x00010001, &disp->shift_clk_opt);

	val = bf_enum(PIXEL_CLK_DIVIDER, PCD1);
	val |= bf_pack(SHIFT_CLK_DIVIDER, div);
	writel(val, &disp->disp_clk_ctrl);

	return 0;
}

/* Start up the display and turn on power to PWMs */
static void basic_init(struct dc_cmd_reg *cmd)
{
	u32 val;

	writel(0x00000100, &cmd->gen_incr_syncpt_ctrl);
	writel(0x0000011a, &cmd->cont_syncpt_vsync);
	writel(0x00000000, &cmd->int_type);
	writel(0x00000000, &cmd->int_polarity);
	writel(0x00000000, &cmd->int_mask);
	writel(0x00000000, &cmd->int_enb);

	val = bf_ones(PW0_ENABLE) | bf_ones(PW1_ENABLE) | bf_ones(PW2_ENABLE);
	val |= bf_ones(PW3_ENABLE) | bf_ones(PW4_ENABLE) | bf_ones(PM0_ENABLE);
	val |= bf_ones(PM1_ENABLE);
	writel(val, &cmd->disp_pow_ctrl);

	bf_enum_writel(CTRL_MODE, C_DISPLAY, &cmd->disp_cmd);
}

static void basic_init_timer(struct dc_disp_reg *disp)
{
	writel(0x00000020, &disp->mem_high_pri);
	writel(0x00000001, &disp->mem_high_pri_timer);
}

static const u32 rgb_enb_tab[PIN_REG_COUNT] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
};

static const u32 rgb_polarity_tab[PIN_REG_COUNT] = {
	0x00000000,
	0x01000000,
	0x00000000,
	0x00000000,
};

static const u32 rgb_data_tab[PIN_REG_COUNT] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
};

static const u32 rgb_sel_tab[PIN_OUTPUT_SEL_COUNT] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00210222,
	0x00002200,
	0x00020000,
};

static void rgb_enable(struct dc_com_reg *com)
{
	int i;

	for (i = 0; i < PIN_REG_COUNT; i++) {
		writel(rgb_enb_tab[i], &com->pin_output_enb[i]);
		writel(rgb_polarity_tab[i], &com->pin_output_polarity[i]);
		writel(rgb_data_tab[i], &com->pin_output_data[i]);
	}

	for (i = 0; i < PIN_OUTPUT_SEL_COUNT; i++)
		writel(rgb_sel_tab[i], &com->pin_output_sel[i]);
}

void setup_window(struct disp_ctl_win *win, struct fdt_lcd *config)
{
	win->x = 0;
	win->y = 0;
	win->w = config->width;
	win->h = config->height;
	win->out_x = 0;
	win->out_y = 0;
	win->out_w = config->width;
	win->out_h = config->height;
	win->phys_addr = config->frame_buffer;
	/* Is this easier to understand than 1 << (x - 3) ? */
	win->stride = config->width * (1 << config->log2_bpp) / 8;
	switch (config->log2_bpp) {
	case 5:
	case 24:
		win->fmt = COLOR_DEPTH_R8G8B8A8;
		win->bpp = 32;
		break;
	case 4:
		win->fmt = COLOR_DEPTH_B5G6R5;
		win->bpp = 16;
		break;

	default:
		printf("Unsupported LCD bit depth");
		return;
	}
}

void tegra2_display_register(struct fdt_lcd *config)
{
	struct disp_ctl_win window;
	struct dc_ctlr *dc = (struct dc_ctlr *)config->reg;

	reset_set_enable(PERIPH_ID_DISP1, 1);
	udelay(20);
	reset_set_enable(PERIPH_ID_DISP1, 0);

	basic_init(&dc->cmd);
	basic_init_timer(&dc->disp);
	rgb_enable(&dc->com);

	if (config->pixel_clock)
		update_display_mode(&dc->disp, config);

	setup_window(&window, config);
	update_window(dc, &window);
}

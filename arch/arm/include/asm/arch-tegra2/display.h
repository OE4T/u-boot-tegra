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

#ifndef __DRIVERS_VIDEO_TEGRA_DC_DC_H
#define __DRIVERS_VIDEO_TEGRA_DC_DC_H

#include <asm/arch/dc.h>

/* This holds information about a window which can be displayed */
struct disp_ctl_win {
	enum win_color_depth_id fmt;	/* Color depth/format */
	unsigned	bpp;		/* Bits per pixel */
	phys_addr_t	phys_addr;	/* Physical address in memory */
	unsigned	x;		/* Horizontal address offset (bytes) */
	unsigned	y;		/* Veritical address offset (bytes) */
	unsigned	w;		/* Width of source window */
	unsigned	h;		/* Height of source window */
	unsigned	stride;		/* Number of bytes per line */
	unsigned	out_x;		/* Left edge of output window (col) */
	unsigned	out_y;		/* Top edge of output window (row) */
	unsigned	out_w;		/* Width of output window in pixels */
	unsigned	out_h;		/* Height of output window in pixels */
};

struct fdt_lcd;

/* Register a new display based on the given configuration */
void tegra2_display_register(struct fdt_lcd *config);

#endif /*__DRIVERS_VIDEO_TEGRA_DC_DC_REG_H*/

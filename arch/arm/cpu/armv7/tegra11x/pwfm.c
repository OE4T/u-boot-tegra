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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* Tegra2 pulse width frequency modulator definitions */

#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/pwfm.h>

void pwfm_setup(struct pwfm_ctlr *pwfm, int enable, int pulse_width,
		int freq_divider)
{
	u32 reg;

	reg = bf_pack(PWFM_ENABLE, enable) |
		bf_pack(PWFM_WIDTH, pulse_width) |
		bf_pack(PWFM_DIVIDER, freq_divider);
	writel(reg, &pwfm->control);
}

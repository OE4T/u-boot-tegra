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

/* This is a single PWFM channel */
struct pwfm_ctlr {
	uint control;		/* Control register */
};

/* PWM_CONTROLLER_PWM_CSR_0/1/2/3_0 */
#define PWFM_ENABLE_RANGE	31:31
#define PWFM_WIDTH_RANGE	30:16
#define PWFM_DIVIDER_RANGE	12:0


/**
 * Program the PWFM with the given parameters.
 *
 * @param enable	1 to enable, 0 to disable
 * @param pulse_width	high pulse width: 0=always low, 1=1/256 pulse high,
 *			n = n/256 pulse high
 * @param freq_divider	frequency divider value
 */
void pwfm_setup(struct pwfm_ctlr *pwfm, int enable, int pulse_width,
		int freq_divider);

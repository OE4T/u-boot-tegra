/*
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TEGRA124_WARMBOOT_AVP_H_
#define _TEGRA124_WARMBOOT_AVP_H_

/* CRC_CCLK_BURST_POLICY_0 0x20 */
#define CCLK_PLLP_BURST_POLICY		0x20004444

/* CRC_CLK_SOURCE_MSELECT_0, 0x3b4 */
#define MSELECT_CLK_DIVISOR_6		6

/* PMC_SCRATCH4 bit 31 defines which Cluster suspends (1 = LP Cluster) */
#define CPU_WAKEUP_CLUSTER		(1 << 31)

/* CPU_SOFTRST_CTRL2_0, 0x388 */
#define CAR2PMC_CPU_ACK_WIDTH_408	408

#endif	/* _TEGRA124_WARMBOOT_AVP_H_ */

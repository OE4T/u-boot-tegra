/*
 * (C) Copyright 2010 - 2013
 * NVIDIA Corporation <www.nvidia.com>
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

#ifndef _FLOW_H_
#define _FLOW_H_

struct flow_ctlr {
	u32 halt_cpu_events;	/* offset 0x00 */
	u32 halt_cop_events;	/* offset 0x04 */
	u32 cpu_csr;		/* offset 0x08 */
	u32 cop_csr;		/* offset 0x0c */
	u32 xrq_events;		/* offset 0x10 */
	u32 halt_cpu1_events;	/* offset 0x14 */
	u32 cpu1_csr;		/* offset 0x18 */
	u32 halt_cpu2_events;	/* offset 0x1c */
	u32 cpu2_csr;		/* offset 0x20 */
	u32 halt_cpu3_events;	/* offset 0x24 */
	u32 cpu3_csr;		/* offset 0x28 */
	u32 cluster_control;	/* offset 0x2c */
	u32 halt_cop1_events;	/* offset 0x30 */
	u32 halt_cop1_csr;	/* offset 0x34 */
	u32 cpu_pwr_csr;	/* offset 0x38 */
	u32 mpid;		/* offset 0x3c */
	u32 ram_repair;		/* offset 0x40 */
};

#endif

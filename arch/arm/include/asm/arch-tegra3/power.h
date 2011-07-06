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

/* Tegra2 power control functions */

#ifndef _POWER_H
#define _POWER_H

/* power partitions as used by APBDEV_PMC_PWRGATE_TOGGLE_0 */
enum power_partition_id {
	POWERP_CPU,
	POWERP_3D,
	POWERP_VIDEO_ENC,
	POWERP_PCI,
	POWERP_VIDEO_DEC,
	POWERP_L2_CACHE,
	POWERP_MPEG_ENC,
};

/* Enable power to a partition */
void power_enable_partition(enum power_partition_id id);

#endif


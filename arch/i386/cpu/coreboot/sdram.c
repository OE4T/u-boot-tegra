/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010,2011
 * Graeme Russ, <graeme.russ@gmail.com>
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
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/u-boot-i386.h>
#include <asm/global_data.h>
#include <asm/ic/coreboot/sysinfo.h>
#include <asm/ic/coreboot/tables.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init_f(void) {
	int i;
	phys_size_t ram_size = 0;
	for (i = 0; i < lib_sysinfo.n_memranges; i++) {
		unsigned long long end = \
			lib_sysinfo.memrange[i].base +
			lib_sysinfo.memrange[i].size;
		if (lib_sysinfo.memrange[i].type == CB_MEM_RAM &&
			end > ram_size) {
			ram_size = end;
		}
	}
	gd->ram_size = ram_size;
	if (ram_size == 0) {
		return -1;
	}
	return 0;
}

int dram_init(void)
{
	return 0;
}

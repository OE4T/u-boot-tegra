/*
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * Command to change the size, address of SDRAM
 */
#include <common.h>
#include <command.h>

extern int dram_reinit (uint32_t start_addr, uint32_t size, int bank_num);

int do_ramconfig (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int j;
    DECLARE_GLOBAL_DATA_PTR;
    if(argc == 1) {
        for(j=0;j<CONFIG_NR_DRAM_BANKS; j++) {
            printf( "SDRAM BANK %d:\n"
                    "Base:\t 0x%08lx\n"
                    "Size:\t 0x%08lx\n",
                    j,
                    gd->bd->bi_dram[j].start,
                    gd->bd->bi_dram[j].size);
        }
        return 0;
    }

    if(argc < 4) {
       printf( "Missing Parameters...\n"
               "Usage: ramconfig <bank> <start_address> <size>\n" );
       return 1;
    } else {
       j = simple_strtoul(argv[1], NULL, 16);
       if(j<CONFIG_NR_DRAM_BANKS) {
           dram_reinit((uint32_t)simple_strtoul(argv[2], NULL, 16),
                       (uint32_t)simple_strtoul(argv[3], NULL, 16),
                       j);
       } else {
           printf( "Invalid bank...must be < %d\n"
                   "Usage: ramconfig <bank> <start_address> <size>\n",
                   CONFIG_NR_DRAM_BANKS);
       }
    }

    return 0;
}

U_BOOT_CMD(
	ramconfig,	4,	1,	do_ramconfig,
	"change sdram start address and size",
	" <bank> <start_address> <size>\n"
    " change SDRAM start address and size. \n "
    " These are passed through ATAG_MEM tag(s) to Linux.\n"
);

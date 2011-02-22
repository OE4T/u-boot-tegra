/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <part.h>

int do_read (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *ep;
	block_dev_desc_t *dev_desc = NULL;
	int dev;
	int part = 0;
	disk_partition_t part_info;
	ulong offset = 0u;
	ulong limit = 0u;
	void *addr;
	uint blk;
	uint cnt;

	if (argc != 6) {
		cmd_usage (cmdtp);
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	if (*ep) {
		if (*ep != ':') {
			printf ("Invalid block device %s\n", argv[2]);
			return 1;
		}
		part = (int)simple_strtoul (++ep, NULL, 16);
	}

	dev_desc = get_dev (argv[1], dev);
	if (dev_desc == NULL) {
		printf ("Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	addr = (void *)simple_strtoul (argv[3], NULL, 16);
	blk = simple_strtoul (argv[4], NULL, 16);
	cnt = simple_strtoul (argv[5], NULL, 16);

	if (part != 0) {
		if (get_partition_info (dev_desc, part, &part_info)) {
			printf ("Cannot find partition %d\n", part);
			return 1;
		}
		offset = part_info.start;
		limit = part_info.size;
	} else {
		/* Largest address not available in block_dev_desc_t. */
		limit = ~0;
	}

	if (cnt + blk > limit) {
		printf ("Read out of range\n");
		return 1;
	}

	if (dev_desc->block_read (dev, offset + blk, cnt, addr) < 0) {
		printf ("Error reading blocks\n");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	read,	6,	0,	do_read,
	"Load binary data from a partition",
	"<interface> <dev[:part]> addr blk# cnt"
);

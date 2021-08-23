// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <spi_flash.h>
#include <mmc.h>
#include <malloc.h>
#include <memalign.h>
#include <part_efi.h>
#include "rp4_support.h"

int find_rp4_data(char *buf, long size)
{
	struct udevice *dev;
	struct mmc *mmc;
	u32 cnt, n;
	int i, curr_device = 0;
	int ret = 0;
	gpt_entry *gpt_pte = NULL;
	bool use_spi_flash = true;	/* Assume QSPI (Nano SKU0/SKU3) */
	long offset = 0x3F8000;		/* 4MB QSPI, which has no GPT to parse */

	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, 512 /*dev_desc->blksz*/);

	printf("Loading XUSB FW blob from RP4 partition ...\n");

	/* Find out if QSPI is present, if not use eMMC (TX1, Nano-SKU2) */
	ret = uclass_first_device_err(UCLASS_SPI_FLASH, &dev);
	if (ret) {
		debug("Can't find SPI flash device, using eMMC ..\n");
		use_spi_flash = false;
	}

	if (use_spi_flash) {
		debug("%s: Reading QSPI, offset 0x%08lX ...\n", __func__,
			offset);
		ret = spi_flash_read_dm(dev, offset, size, buf);
		if (ret)
			return log_msg_ret("Cannot read SPI flash", ret);
	} else {
		debug("%s: Reading mmc %d, parsing GPT ...\n", __func__,
			curr_device);

		mmc = init_mmc_device(curr_device, false);
		if (!mmc)
			return -ENODEV;

		if (find_valid_gpt(mmc_get_blk_desc(mmc), gpt_head, &gpt_pte) != 1) {
			printf("ERROR reading eMMC GPT!\n");
			return -EINVAL;
		}

		for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
			/* Stop at the first non valid PTE */
			if (!is_pte_valid(&gpt_pte[i]))
				break;

			debug("%3d 0x%08llx 0x%08llx %s\n", (i + 1),
				le64_to_cpu(gpt_pte[i].starting_lba),
				le64_to_cpu(gpt_pte[i].ending_lba),
				print_efiname(&gpt_pte[i]));

			if (strcmp("RP4", print_efiname(&gpt_pte[i])) == 0) {
				offset = le64_to_cpu(gpt_pte[i].starting_lba);
				debug("Found RP4 partition @ 0x%8lX\n", offset);
				break;
			}
		}

		/* Remember to free pte */
		free(gpt_pte);

		cnt = size / mmc_get_blk_desc(mmc)->blksz;
		debug("\neMMC read: dev # %d, block # %ld, count %d ... ",
			curr_device, offset, cnt);
		n = blk_dread(mmc_get_blk_desc(mmc), offset, cnt, buf);
		debug("%d blocks read: %s\n", n, (n == cnt) ? "OK" : "ERROR");
	}
again:
	/* Good read, check for NVBLOB sig */
	debug("%s: checking offset %lX\n", __func__, offset);
	if (strcmp(buf, "NVIDIA__BLOB__V2") == 0) {
		debug("%s: Found an NV BLOB! ...\n", __func__);
		if (strcmp(buf+0x58, "XUSB") == 0) {
			debug("%s: Found XUSB sig!\n", __func__);
			if (use_spi_flash) {
				debug("%ld bytes read: %s\n", size, "QSPI");
			} else {
				debug("%d blocks read: %s\n", cnt, "eMMC");
			}

			debug(" XUSB FW @ %p\n", buf);
			goto done;
		}
	}

	if (use_spi_flash) {
		/* read next block, work back thru QSPI (RP4 is near the end) */
		offset -= PROBE_BUF_SIZE;
		if (offset <= 0)
			goto done;

		ret = spi_flash_read_dm(dev, offset, size, buf);
		if (ret)
			return log_msg_ret("Cannot read SPI flash", ret);

		goto again;
	}

done:
	return 0;
}

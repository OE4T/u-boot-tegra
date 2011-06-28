/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <mmc.h>
#include <part.h>
#include <usb.h>
#include <linux/list.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

#define PREFIX			"boot_device: "

/*
 * Total number of storage devices, USB + MMC + reserved.
 * MMC is 2 now. Reserve extra 3 for margin of safety.
 */
#define MAX_DISK_INFO		(USB_MAX_STOR_DEV + 2 + 3)

static uint32_t get_dev_flags(const block_dev_desc_t *dev)
{
	return dev->removable ? VB_DISK_FLAG_REMOVABLE : VB_DISK_FLAG_FIXED;
}

static const char *get_dev_name(const block_dev_desc_t *dev)
{
	/*
	 * See the definitions of IF_TYPE_* in /include/part.h.
	 * 8 is max size of strings, the "UNKNOWN".
	 */
	static const char all_disk_types[IF_TYPE_MAX][8] = {
		"UNKNOWN", "IDE", "SCSI", "ATAPI", "USB",
		"DOC", "MMC", "SD", "SATA"};

	if (dev->if_type >= IF_TYPE_MAX) {
		return all_disk_types[0];
	}
	return all_disk_types[dev->if_type];
}

static void fill_disk_info(const block_dev_desc_t *dev, VbDiskInfo *info_ptr)
{
	info_ptr->handle = (VbExDiskHandle_t)dev;
	info_ptr->bytes_per_lba = dev->blksz;
	info_ptr->lba_count = dev->lba;
	info_ptr->flags = get_dev_flags(dev);
	info_ptr->name = get_dev_name(dev);
}

static int add_disk_info(const block_dev_desc_t *dev, VbDiskInfo *infos,
		uint32_t *count_ptr)
{
	if (*count_ptr >= MAX_DISK_INFO) {
		VbExDebug(PREFIX "Too many storage devices "
				 "registered, > %d.\n", MAX_DISK_INFO);
		return 1;
	}

	fill_disk_info(dev, &infos[*count_ptr]);
	(*count_ptr)++;
	return 0;
}

static void init_usb_storage(void)
{
	/*
	 * We should stop all USB devices first. Otherwise we can't detect any
	 * new devices.
	 */
	usb_stop();

	if (usb_init() >= 0)
		usb_stor_scan(/*mode=*/1);
}

VbError_t VbExDiskGetInfo(VbDiskInfo** infos_ptr, uint32_t* count,
                          uint32_t disk_flags)
{
	VbDiskInfo *infos;
	struct mmc *m;
	block_dev_desc_t *d;
	int i;

	infos = (VbDiskInfo *)VbExMalloc(sizeof(VbDiskInfo) * MAX_DISK_INFO);
	*infos_ptr = infos;
	*count = 0;

	/* Detect all SD/MMC devices. */
	for (i = 0; (m = find_mmc_device(i)) != NULL; i++) {
		uint32_t flags = get_dev_flags(&m->block_dev);

		/* Skip this entry if the flags are not matched. */
		if (!(flags & disk_flags))
			continue;

		/* Skip this entry if it is unable to initialize. */
		if (mmc_init(m)) {
			VbExDebug(PREFIX "Unable to init MMC dev %d.\n", i);
			continue;
		}

		if (add_disk_info(&m->block_dev, infos, count)) {
			/*
			 * If too many storage devices registered,
			 * returns as many disk infos as we could
			 * handle.
			 */
			return 1;
		}
	}

	/* To speed up, skip detecting USB if not require removable devices. */
	if (disk_flags & VB_DISK_FLAG_REMOVABLE) {
		/* Detect all USB storage devices. */
		init_usb_storage();
		for (i = 0; (d = usb_stor_get_dev(i)) != NULL; i++) {
			uint32_t flags = get_dev_flags(d);

			/* Skip this entry if the flags are not matched. */
			if (!(flags & disk_flags))
				continue;

			if (add_disk_info(&m->block_dev, infos, count)) {
				/*
				 * If too many storage devices registered,
				 * returns as many disk infos as we could
				 * handle.
				 */
				return 1;
			}
		}
	}

	if (*count == 0) {
		VbExFree(infos);
		infos = NULL;
	}

	return VBERROR_SUCCESS;
}

VbError_t VbExDiskFreeInfo(VbDiskInfo *infos, VbExDiskHandle_t preserve_handle)
{
	/* We do nothing for preserve_handle as we keep all the devices on. */
	VbExFree(infos);
	return VBERROR_SUCCESS;
}

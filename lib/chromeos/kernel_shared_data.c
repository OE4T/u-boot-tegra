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
#include <gbb_header.h> /* for GoogleBinaryBlockHeader */
#include <chromeos/common.h>
#include <chromeos/kernel_shared_data.h>
#include <chromeos/vboot_nvstorage_helper.h> /* for NVCONTEXT_LBA */
#include <linux/string.h> /* for strcpy */

/* This is used to keep u-boot and kernel in sync */
#define SHARED_MEM_VERSION 1

#define PREFIX "kernel_shared_data: "

enum {
	CHSW_RECOVERY_BUTTON_PRESSED	= 0x002,
	CHSW_DEVELOPER_MODE_ENABLED	= 0x020,
	CHSW_WRITE_PROTECT_DISABLED	= 0x200
};

/* the data blob format */
typedef struct {
	uint32_t	total_size;
	uint8_t		signature[10];
	uint16_t	version;
	uint64_t	nvcxt_lba;
	uint16_t	vbnv[2];
	uint8_t		nvcxt_cache[VBNV_BLOCK_SIZE];
	uint8_t		write_protect_sw;
	uint8_t		recovery_sw;
	uint8_t		developer_sw;
	uint8_t		binf[5];
	uint32_t	chsw;
	uint8_t		hwid[ID_LEN];
	uint8_t		fwid[ID_LEN];
	uint8_t		frid[ID_LEN];
	uint32_t	fmap_base;
	uint8_t		shared_data_body[CONFIG_LENGTH_FMAP];
} __attribute__((packed)) KernelSharedDataType;

/* TODO remove this function when we embedded the blob in fdt */
void *get_last_1mb_of_ram(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	return (void *)(gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS-1].start +
			gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS-1].size - SZ_1M);
}

int cros_ksd_init(void *kernel_shared_data, uint8_t *frid,
		void *fmap_data, uint32_t fmap_size,
		void *gbb_data, uint32_t gbb_size,
		VbNvContext *nvcxt,
		int write_protect_sw, int recovery_sw, int developer_sw)
{
	KernelSharedDataType *sd = (KernelSharedDataType *)kernel_shared_data;
	GoogleBinaryBlockHeader *gbbh = (GoogleBinaryBlockHeader *)gbb_data;

	VBDEBUG(PREFIX "kernel shared data at %p\n", sd);

	memset(sd, '\0', sizeof(*sd));

	strcpy((char *)sd->signature, "CHROMEOS");
	sd->version = SHARED_MEM_VERSION;

	if (recovery_sw)
		sd->chsw |= CHSW_RECOVERY_BUTTON_PRESSED;
	if (developer_sw)
		sd->chsw |= CHSW_DEVELOPER_MODE_ENABLED;
	if (!write_protect_sw)
		sd->chsw |= CHSW_WRITE_PROTECT_DISABLED;

	memcpy(sd->frid, frid, ID_LEN);
	strncpy((char *)sd->hwid, gbb_data + gbbh->hwid_offset, gbbh->hwid_size);

	/* boot reason; always 0 */
	sd->binf[0] = 0;
	/* ec firmware type: 0=read-only, 1=rewritable; default to 0 */
	sd->binf[2] = 0;
	/* recovery reason is default to VBNV_RECOVERY_NOT_REQUESTED */
	sd->binf[4] = VBNV_RECOVERY_NOT_REQUESTED;

	sd->write_protect_sw = write_protect_sw;
	sd->recovery_sw = recovery_sw;
	sd->developer_sw = developer_sw;

	sd->vbnv[0] = 0;
	sd->vbnv[1] = VBNV_BLOCK_SIZE;

	memcpy(sd->shared_data_body, fmap_data, fmap_size);
	sd->fmap_base = (uint32_t)sd->shared_data_body;

	sd->total_size = sizeof(*sd);

	sd->nvcxt_lba = NVCONTEXT_LBA;

	memcpy(sd->nvcxt_cache, nvcxt->raw, VBNV_BLOCK_SIZE);

	return 0;
}

int cros_ksd_set_fwid(void *kernel_shared_data, uint8_t *fwid)
{
	KernelSharedDataType *sd = (KernelSharedDataType *)kernel_shared_data;
	memcpy(sd->fwid, fwid, ID_LEN);
	return 0;
}

int cros_ksd_set_active_main_firmware(void *kernel_shared_data,
		int which, int type)
{
	KernelSharedDataType *sd = (KernelSharedDataType *)kernel_shared_data;
	sd->binf[1] = which;
	sd->binf[3] = type;
	return 0;
}

int cros_ksd_set_recovery_reason(void *kernel_shared_data, uint32_t reason)
{
	KernelSharedDataType *sd = (KernelSharedDataType *)kernel_shared_data;
	sd->binf[4] = reason;
	return 0;
}

void cros_ksd_dump(void *kernel_shared_data)
{
#ifdef VBOOT_DEBUG /* decleare inside ifdef so that compiler doesn't complain */
	KernelSharedDataType *sd = (KernelSharedDataType *)kernel_shared_data;
	int i;

#define _p(format, field) \
	VBDEBUG("cros_ksd_dump: %-20s: " format "\n", #field, sd->field)
	_p("%08x",	total_size);
	_p("\"%s\"",	signature);
	_p("%d",	version);
	_p("%08llx",	nvcxt_lba);
	_p("%08x",	vbnv[0]);
	_p("%08x",	vbnv[1]);
	VBDEBUG("cros_ksd_dump: nvcxt_cache: ");
	for (i = 0; i < VBNV_BLOCK_SIZE; i++)
		VBDEBUG("%02x", sd->nvcxt_cache[i]);
	VBDEBUG("\n");
	_p("%d",	write_protect_sw);
	_p("%d",	recovery_sw);
	_p("%d",	developer_sw);
	_p("%08x",	binf[0]);
	_p("%08x",	binf[1]);
	_p("%08x",	binf[2]);
	_p("%08x",	binf[3]);
	_p("%08x",	binf[4]);
	_p("%08x",	chsw);
	_p("\"%s\"",	hwid);
	_p("\"%s\"",	fwid);
	_p("\"%s\"",	frid);
	_p("%08x",	fmap_base);
#undef _p
#endif
}

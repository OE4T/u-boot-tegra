/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of normal firmware of Chrome OS Verify Boot */

#include <common.h>
#include <command.h>
#include <fat.h>
#include <malloc.h>
#include <mmc.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/load_firmware_helper.h>
#include <chromeos/load_kernel_helper.h>
#include <chromeos/os_storage.h>
#include <chromeos/power_management.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <boot_device.h>
#include <load_kernel_fw.h>
#include <tlcl_stub.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

#define PREFIX "cros_normal_firmware: "

#define DEVICE_TYPE		"mmc"
#define DEVICE_NUMBER		0

/* Return 0 if success, non-zero on error */
int initialize_drive(void)
{
	if (initialize_mmc_device(DEVICE_NUMBER)) {
		debug(PREFIX "mmc %d init fail\n", DEVICE_NUMBER);
		return -1;
	}

	debug(PREFIX "set_bootdev %s %x:0\n", DEVICE_TYPE, DEVICE_NUMBER);
	if (set_bootdev(DEVICE_TYPE, DEVICE_NUMBER, 0)) {
		debug(PREFIX "set_bootdev fail\n");
		return -1;
	}

	return 0;
}

int do_cros_normal_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	int status;
	firmware_storage_t file;
	void *gbb_data = NULL;
	uint64_t gbb_size = 0;
	uint32_t reason = VBNV_RECOVERY_RW_UNSPECIFIED;

	if (TlclStubInit() != TPM_SUCCESS) {
		debug(PREFIX "fail to init tpm\n");
		reboot_to_recovery_mode(VBNV_RECOVERY_RW_TPM_ERROR);
	}

	if (initialize_drive()) {
		debug(PREFIX "error: initialize fixed drive fail\n");
		reboot_to_recovery_mode(VBNV_RECOVERY_RW_NO_OS);
	}

	if (firmware_storage_init(&file) ||
			load_gbb(&file, &gbb_data, &gbb_size)) {
		debug(PREFIX "error: cannot read gbb\n");
		reboot_to_recovery_mode(VBNV_RECOVERY_RO_SHARED_DATA);
	}

	status = load_and_boot_kernel(gbb_data, gbb_size, 0ULL);
	debug(PREFIX "status == %d\n", status);

	if (status == LOAD_KERNEL_REBOOT) {
		debug(PREFIX "internal error: reboot to current mode\n");
		cold_reboot();
	}

	if (status == LOAD_KERNEL_NOT_FOUND)
		reason = VBNV_RECOVERY_RW_NO_OS;
	else if (status == LOAD_KERNEL_INVALID)
		reason = VBNV_RECOVERY_RW_INVALID_OS;
	reboot_to_recovery_mode(reason);

	/* never reach here */
	/* free(gbb_data); */
	/* file.close(file.context); */
	return 1;
}

U_BOOT_CMD(cros_normal_firmware, 1, 1, do_cros_normal_firmware,
		"verified boot normal firmware", NULL);

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
#include <chromeos/hardware_interface.h>
#include <chromeos/os_storage.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <boot_device.h>
#include <load_kernel_fw.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

#define PREFIX "cros_normal_firmware: "

/* defined in common/cmd_source.c */
int source(ulong addr, const char *fit_uname);

/* defined in common/cmd_bootm.c */
int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#define DEVICE_TYPE		"mmc"
#define DEVICE_NAME		"mmcblk" DEVICE_NUMBER_STRING "p"
#define DEVICE_NUMBER_STRING	"0"
#define DEVICE_NUMBER		0
#define DEVICE_PART		0xc

/* Return 0 if success, non-zero on error */
int initialize_drive(void)
{
	if (initialize_mmc_device(DEVICE_NUMBER)) {
		debug(PREFIX "mmc %d init fail\n", DEVICE_NUMBER);
		return -1;
	}

	/* Environment variables used by u-boot scripts in kernel partition */
	setenv("devtype", DEVICE_TYPE);
	setenv("devname", DEVICE_NAME);
	setenv("devnum",  DEVICE_NUMBER_STRING);

	/* TODO move to u-boot-config */
	run_command("setenv console console=ttyS0,115200n8", 0);
	run_command("setenv bootargs ${console} ${platform_extras}", 0);

	debug(PREFIX "set_bootdev %s %x:0\n", DEVICE_TYPE, DEVICE_NUMBER);
	if (set_bootdev(DEVICE_TYPE, DEVICE_NUMBER, 0)) {
		debug(PREFIX "set_bootdev fail\n");
		return -1;
	}

	return 0;
}

/* This function should never return */
void reboot_to_recovery_mode(firmware_storage_t *file, VbNvContext *nvcxt,
		uint32_t reason)
{
	debug(PREFIX "store recovery cookie in recovery field\n");
	if (VbNvSet(nvcxt, VBNV_RECOVERY_REQUEST, reason) ||
			VbNvTeardown(nvcxt) ||
			(nvcxt->raw_changed && write_nvcontext(file, nvcxt))) {
		/* FIXME: bring up a sad face? */
		debug(PREFIX "error: cannot write recovery cookie");
		while (1);
	}

	debug(PREFIX "reboot to recovery mode\n");
	reset_cpu(0);

	debug(PREFIX "error: reset_cpu() returned\n");
	while (1);
}

/* This function should never return */
void boot_kernel(LoadKernelParams *params)
{
	char load_address[32];
	char *argv[2] = { "bootm", load_address };

	debug(PREFIX "boot_kernel\n");
	debug(PREFIX "kernel_buffer:      0x%p\n",
			params->kernel_buffer);
	debug(PREFIX "bootloader_address: 0x%08x\n",
			(int) params->bootloader_address);

	if (source((ulong) params->bootloader_address, NULL)) {
		debug(PREFIX "source fail\n");
		return;
	}

	/*
	 * FIXME: So far bootloader in kernel partition isn't really a
	 * bootloader; instead, it is merely a u-boot scripts that sets kernel
	 * parameters. And therefore we still have to boot kernel to here
	 * by calling do_bootm.
	 */
	sprintf(load_address, "0x%p", params->kernel_buffer);
	debug(PREFIX "run command: %s %s\n", argv[0], argv[1]);
	do_bootm(NULL, 0, sizeof(argv)/sizeof(*argv), argv);

	debug(PREFIX "error: do_bootm() returned\n");
	while (1);
}

int do_cros_normal_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	int status;
	LoadKernelParams params;
	firmware_storage_t file;
	VbNvContext nvcxt;
	void *gbb_data = NULL;
	uint64_t gbb_size = 0;
	uint64_t boot_flags = 0;
	uint32_t reason = VBNV_RECOVERY_RW_UNSPECIFIED;

	/* TODO Continue initializing chipset */

	if (firmware_storage_init(&file)) {
		/*
		 * FIXME: because we can't write to nv context, we can't even
		 * reboot to recovery firmware! Bring up a sad face now?
		 */
		debug(PREFIX "init firmware storage failed\n");
		while (1);
	}

	if (read_nvcontext(&file, &nvcxt)) {
		/*
		 * FIXME: because we don't have a clean state of nvcxt, we
		 * can't reboot to recovery. Bring up a sad face now?
		 */
		debug(PREFIX "fail to read nvcontext\n");
		while (1);
	}

	if (initialize_drive()) {
		debug(PREFIX "error: initialize fixed drive fail\n");
		reboot_to_recovery_mode(&file, &nvcxt,
				VBNV_RECOVERY_RW_NO_OS);
	}

	if (load_gbb(&file, &gbb_data, &gbb_size)) {
		debug(PREFIX "error: cannot read gbb\n");
		reboot_to_recovery_mode(&file, &nvcxt,
				VBNV_RECOVERY_RO_SHARED_DATA);
	}

	if (is_developer_mode_gpio_asserted())
		boot_flags |= BOOT_FLAG_DEVELOPER;

	debug(PREFIX "call load_kernel_wrapper with boot_flags: %d\n",
			(int) boot_flags);
	status = load_kernel_wrapper(&params, gbb_data, gbb_size,
			boot_flags, &nvcxt, NULL);

	if (nvcxt.raw_changed && write_nvcontext(&file, &nvcxt)) {
		/*
		 * FIXME: because we can't write to nv context, we can't even
		 * reboot to recovery firmware! Bring up a sad face now?
		 */
		debug(PREFIX "fail to write nvcontext\n");
		while (1);
        }

	if (status == LOAD_KERNEL_SUCCESS) {
		debug(PREFIX "success: good kernel found on device\n");
		boot_kernel(&params);
	}

	if (status == LOAD_KERNEL_REBOOT) {
		debug(PREFIX "internal error: reboot to current mode\n");
		reset_cpu(0);
		debug(PREFIX "error: reset_cpu() returns\n");
		while (1);
	}

	if (status == LOAD_KERNEL_RECOVERY) {
		debug(PREFIX "internal error: reboot to recovery mode\n");
	} else if (status == LOAD_KERNEL_NOT_FOUND) {
		debug(PREFIX "no kernel found on device\n");
		reason = VBNV_RECOVERY_RW_NO_OS;
	} else if (status == LOAD_KERNEL_INVALID) {
		debug(PREFIX "only invalid kernels found on device\n");
		reason = VBNV_RECOVERY_RW_INVALID_OS;
	} else {
		debug(PREFIX "unknown status from LoadKernel(): %d\n", status);
	}

	reboot_to_recovery_mode(&file, &nvcxt, reason);

	/* never reach here */
	/* free(gbb_data); */
	/* file.close(file.context); */
	return 1;
}

U_BOOT_CMD(cros_normal_firmware, 1, 1, do_cros_normal_firmware,
		"verified boot normal firmware", NULL);

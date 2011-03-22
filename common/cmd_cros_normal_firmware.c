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
#include <chromeos/hardware_interface.h>
#include <chromeos/os_storage.h>

#include <boot_device.h>
#include <load_kernel_fw.h>
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

#define SCRIPT_PATH		"/u-boot/boot.scr.uimg"
#define SCRIPT_LOAD_ADDRESS	0x408000

/* Return 0 if success, non-zero on error */
int initialize_drive(void)
{
	if (initialize_mmc_device(DEVICE_NUMBER)) {
		debug(PREFIX "mmc %d init fail\n", DEVICE_NUMBER);
		return -1;
	}

	/*
	 * Environment variables used by u-boot scripts in EFI partition and
	 * kernel partition
	 */
	setenv("devtype", DEVICE_TYPE);
	setenv("devname", DEVICE_NAME);
	setenv("devnum",  DEVICE_NUMBER_STRING);

	/* To get ${kernelpart} and other stuff from EFI partition. */
	if (fat_fsload(DEVICE_TYPE, DEVICE_NUMBER, DEVICE_PART, SCRIPT_PATH,
				(void*) SCRIPT_LOAD_ADDRESS, 0) < 0) {
		debug(PREFIX "fail to load %s from %s %x:%x to 0x%08x\n",
				SCRIPT_PATH,
				DEVICE_TYPE, DEVICE_NUMBER, DEVICE_PART,
				SCRIPT_LOAD_ADDRESS);
		return -1;
	}
	/* FIXME Should not execute scripts in EFI partition */
	if (source(SCRIPT_LOAD_ADDRESS, NULL)) {
		debug(PREFIX "fail to execute script at 0x%08x\n",
				SCRIPT_LOAD_ADDRESS);
		return -1;
	}

	debug(PREFIX "set_bootdev %s %x:0\n", DEVICE_TYPE, DEVICE_NUMBER);
	if (set_bootdev(DEVICE_TYPE, DEVICE_NUMBER, 0)) {
		debug(PREFIX "set_bootdev fail\n");
		return -1;
	}

	return 0;
}

/* This function should never return */
void reboot_to_recovery_mode(void)
{
	debug(PREFIX "store recovery cookie in recovery field\n");
	/* TODO store recovery cookie in recovery field */

	debug(PREFIX "reboot to recovery mode\n");
	reset_cpu(0);

	debug(PREFIX "error: reboot_to_recovery_mode() fail\n");
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
}

int do_cros_normal_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	int status;
	LoadKernelParams params;
	void *gbb_data = NULL;
	uint64_t gbb_size = 0;
	uint64_t boot_flags = 0;

	/* TODO Continue initializing chipset */

	if (is_recovery_mode_gpio_asserted() ||
			is_recovery_mode_field_containing_cookie()) {
		debug(PREFIX "error: bootstub should have booted "
				"recovery firmware\n");
		reboot_to_recovery_mode();
		return 1;
	}

	if (initialize_drive()) {
		debug(PREFIX "error: initialize fixed drive fail\n");
		reboot_to_recovery_mode();
		return 1;
	}

	if (load_gbb(NULL, &gbb_data, &gbb_size)) {
		debug(PREFIX "error: cannot read gbb\n");
		reboot_to_recovery_mode();
		return 1;
	}

	if (is_developer_mode_gpio_asserted())
		boot_flags |= BOOT_FLAG_DEVELOPER;

	debug(PREFIX "call load_kernel_wrapper with boot_flags: %d\n",
			(int) boot_flags);
	status = load_kernel_wrapper(&params, gbb_data, gbb_size, boot_flags,
			NULL);

	if (status == LOAD_KERNEL_SUCCESS) {
		debug(PREFIX "success: good kernel found on device\n");
		boot_kernel(&params);
		debug(PREFIX "error: boot_kernel fail\n");
		return 1;
	}

	if (status == LOAD_KERNEL_REBOOT) {
		debug(PREFIX "internal error: reboot to current mode\n");
		reset_cpu(0);
		debug(PREFIX "error: reset_cpu(0) fail\n");
		return 1;
	}

	if (status == LOAD_KERNEL_RECOVERY) {
		debug(PREFIX "internal error: reboot to recovery mode\n");
	} else if (status == LOAD_KERNEL_NOT_FOUND) {
		debug(PREFIX "no kernel found on device\n");
	} else if (status == LOAD_KERNEL_INVALID) {
		debug(PREFIX "only invalid kernels found on device\n");
	} else {
		debug(PREFIX "unknown status from LoadKernel(): %d\n", status);
	}

	free(gbb_data);

	reboot_to_recovery_mode();
	return 1;
}

U_BOOT_CMD(cros_normal_firmware, 1, 1, do_cros_normal_firmware,
		"verified boot normal firmware", NULL);

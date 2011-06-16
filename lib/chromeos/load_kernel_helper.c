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
#include <chromeos/common.h>
#include <chromeos/load_kernel_helper.h>
#include <chromeos/os_storage.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <load_kernel_fw.h>

#define PREFIX "load_kernel_helper: "

/* defined in common/cmd_bootm.c */
int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

/**
 * This is a wrapper of LoadKernel, which verifies the kernel image specified
 * by set_bootdev. The caller of this functions must have called set_bootdev
 * first.
 *
 * @param boot_flags are bitwise-or'ed of flags in load_kernel_fw.h
 * @param gbb_data points to a GBB blob
 * @param gbb_size is the size of the GBB blob
 * @param vbshared_data points to VbSharedData blob
 * @param vbshared_size is the size of the VbSharedData blob
 * @param nvcxt points to a VbNvContext object
 * @return LoadKernel's return value
 */
static int load_kernel_wrapper(LoadKernelParams *params, uint64_t boot_flags,
		void *gbb_data, uint32_t gbb_size,
		void *vbshared_data, uint32_t vbshared_size,
		VbNvContext *nvcxt)
{
	int status = LOAD_KERNEL_NOT_FOUND;

	memset(params, '\0', sizeof(*params));

	params->boot_flags = boot_flags;

	params->gbb_data = gbb_data;
	params->gbb_size = gbb_size;

	params->shared_data_blob = vbshared_data;
	params->shared_data_size = vbshared_size;

	params->bytes_per_lba = get_bytes_per_lba();
	params->ending_lba = get_ending_lba();

	params->kernel_buffer = (void*)CONFIG_CHROMEOS_KERNEL_LOADADDR;
	params->kernel_buffer_size = CONFIG_CHROMEOS_KERNEL_BUFSIZE;

	params->nv_context = nvcxt;

	VBDEBUG(PREFIX "call LoadKernel() with parameters...\n");
	VBDEBUG(PREFIX "shared_data_blob:     0x%p\n",
			params->shared_data_blob);
	VBDEBUG(PREFIX "bytes_per_lba:        %d\n",
			(int) params->bytes_per_lba);
	VBDEBUG(PREFIX "ending_lba:           0x%08x\n",
			(int) params->ending_lba);
	VBDEBUG(PREFIX "kernel_buffer:        0x%p\n",
			params->kernel_buffer);
	VBDEBUG(PREFIX "kernel_buffer_size:   0x%08x\n",
			(int) params->kernel_buffer_size);
	VBDEBUG(PREFIX "boot_flags:           0x%08x\n",
			(int) params->boot_flags);

	status = LoadKernel(params);

	VBDEBUG(PREFIX "LoadKernel status: %d\n", status);
	if (status == LOAD_KERNEL_SUCCESS) {
		VBDEBUG(PREFIX "partition_number:   0x%08x\n",
				(int) params->partition_number);
		VBDEBUG(PREFIX "bootloader_address: 0x%08x\n",
				(int) params->bootloader_address);
		VBDEBUG(PREFIX "bootloader_size:    0x%08x\n",
				(int) params->bootloader_size);

		/* TODO(clchiou): deprecated when we fix crosbug:14022 */
		if (params->partition_number == 2) {
			setenv("kernelpart", "2");
			setenv("rootpart", "3");
		} else if (params->partition_number == 4) {
			setenv("kernelpart", "4");
			setenv("rootpart", "5");
		} else {
			VBDEBUG(PREFIX "unknown kernel partition: %d\n",
					(int) params->partition_number);
			status = LOAD_KERNEL_NOT_FOUND;
		}
	}

	return status;
}

/* Maximum kernel command-line size */
#define CROS_CONFIG_SIZE 4096

/* Size of the x86 zeropage table */
#define CROS_PARAMS_SIZE 4096

/**
 * This loads kernel command line from the buffer that holds the loaded kernel
 * image. This function calculates the address of the command line from the
 * bootloader address.
 *
 * @param bootloader_address is the address of the bootloader in the buffer
 * @return 0 if it succeeds; non-zero if it fails
 */
static int load_kernel_config(void *bootloader_address)
{
	char buf[80 + CROS_CONFIG_SIZE];

	strcpy(buf, "setenv bootargs ${bootargs} ");

	/* Use the bootloader address to find the kernel config location. */
	strncat(buf, bootloader_address - CROS_PARAMS_SIZE - CROS_CONFIG_SIZE,
			CROS_CONFIG_SIZE);

	/*
	 * TODO(clchiou): Use run_command instead of setenv because we need
	 * variable substitutions. This shouldn't be the case after we fix
	 * crosbug:14022
	 */
	if (run_command(buf, 0)) {
		VBDEBUG(PREFIX "run_command(%s) fail\n", buf);
		return 1;
	}
	return 0;
}

/* assert(0 <= val && val < 99); sprintf(dst, "%u", val); */
static char *itoa(char *dst, int val)
{
	if (val > 9)
		*dst++ = '0' + val / 10;
	*dst++ = '0' + val % 10;
	return dst;
}

/* copied from x86 bootstub code; sprintf(dst, "%02x", val) */
static void one_byte(char *dst, uint8_t val)
{
	dst[0] = "0123456789abcdef"[(val >> 4) & 0x0F];
	dst[1] = "0123456789abcdef"[val & 0x0F];
}

/* copied from x86 bootstub code; display a GUID in canonical form */
static char *emit_guid(char *dst, uint8_t *guid)
{
	one_byte(dst, guid[3]); dst += 2;
	one_byte(dst, guid[2]); dst += 2;
	one_byte(dst, guid[1]); dst += 2;
	one_byte(dst, guid[0]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[5]); dst += 2;
	one_byte(dst, guid[4]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[7]); dst += 2;
	one_byte(dst, guid[6]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[8]); dst += 2;
	one_byte(dst, guid[9]); dst += 2;
	*dst++ = '-';
	one_byte(dst, guid[10]); dst += 2;
	one_byte(dst, guid[11]); dst += 2;
	one_byte(dst, guid[12]); dst += 2;
	one_byte(dst, guid[13]); dst += 2;
	one_byte(dst, guid[14]); dst += 2;
	one_byte(dst, guid[15]); dst += 2;
	return dst;
}

/**
 * This replaces:
 *   %D -> device number
 *   %P -> partition number
 *   %U -> GUID
 * in kernel command line.
 *
 * For example:
 *   ("root=/dev/sd%D%P", 2, 3)      -> "root=/dev/sdc3"
 *   ("root=/dev/mmcblk%Dp%P", 0, 5) -> "root=/dev/mmcblk0p5".
 *
 * @param src - input string
 * @param devnum - device number of the storage device we will mount
 * @param partnum - partition number of the root file system we will mount
 * @param guid - guid of the kernel partition
 * @param dst - output string; a copy of [src] with special characters replaced
 */
static void update_cmdline(char *src, int devnum, int partnum, uint8_t *guid,
		char *dst)
{
	int c;

	// sanity check on inputs
	if (devnum < 0 || devnum > 25 || partnum < 1 || partnum > 99) {
		VBDEBUG(PREFIX "insane input: %d, %d\n", devnum, partnum);
		devnum = 0;
		partnum = 3;
	}

	while ((c = *src++)) {
		if (c != '%') {
			*dst++ = c;
			continue;
		}

		switch ((c = *src++)) {
		case '\0':
			/* input ends in '%'; is it not well-formed? */
			src--;
			break;
		case 'D':
			/*
			 * TODO: Do we have any better way to know whether %D
			 * is replaced by a letter or digits? So far, this is
			 * done by a rule of thumb that if %D is followed by a
			 * 'p' character, then it is replaced by digits.
			 */
			if (*src == 'p')
				dst = itoa(dst, devnum);
			else
				*dst++ = 'a' + devnum;
			break;
		case 'P':
			dst = itoa(dst, devnum);
			break;
		case 'U':
			dst = emit_guid(dst, guid);
			break;
		default:
			*dst++ = '%';
			*dst++ = c;
			break;
		}
	}

	*dst = '\0';
}

/**
 * This boots kernel specified in [parmas].
 *
 * @param params that specifies where to boot from
 * @return LOAD_KERNEL_INVALID if it fails to boot; otherwise it never returns
 *         to its caller
 */
static int boot_kernel_helper(LoadKernelParams *params)
{
	char *cmdline, cmdline_buf[4096];
	char load_address[32];
	char *argv[2] = { "bootm", load_address };

	VBDEBUG(PREFIX "boot_kernel\n");
	VBDEBUG(PREFIX "kernel_buffer:      0x%p\n",
			params->kernel_buffer);
	VBDEBUG(PREFIX "bootloader_address: 0x%08x\n",
			(int) params->bootloader_address);

	/*
	 * casting bootloader_address of uint64_t type to uint32_t before
	 * further casting it to void* to avoid compiler warning
	 * "cast to pointer from integer of different size"
	 */
	if (load_kernel_config((void*)(uint32_t)params->bootloader_address)) {
		VBDEBUG(PREFIX "error: load kernel config failed\n");
		return LOAD_KERNEL_INVALID;
	}

	if ((cmdline = getenv("bootargs"))) {
		VBDEBUG(PREFIX "cmdline before update: %s\n", cmdline);

		update_cmdline(cmdline, get_device_number(),
				params->partition_number + 1,
				params->partition_guid,
				cmdline_buf);
		setenv("bootargs", cmdline_buf);
		VBDEBUG(PREFIX "cmdline after update:  %s\n",
				getenv("bootargs"));
	} else {
		VBDEBUG(PREFIX "bootargs == NULL\n");
	}

	/*
	 * TODO(clchiou): we probably should minimize calls to other u-boot
	 * commands inside verified boot for security reasons?
	 */
	sprintf(load_address, "0x%p", params->kernel_buffer);
	VBDEBUG(PREFIX "run command: %s %s\n", argv[0], argv[1]);
	do_bootm(NULL, 0, sizeof(argv)/sizeof(*argv), argv);

	/* should never reach here! */
	VBDEBUG(PREFIX "error: do_bootm() returned\n");
	return LOAD_KERNEL_INVALID;
}

int boot_kernel(uint64_t boot_flags,
		void *gbb_data, uint32_t gbb_size,
		void *vbshared_data, uint32_t vbshared_size,
		VbNvContext *nvcxt)
{
	LoadKernelParams params;
	int status;

	status = load_kernel_wrapper(&params, boot_flags, gbb_data, gbb_size,
			vbshared_data, vbshared_size, nvcxt);

	VBDEBUG(PREFIX "load_kernel_wrapper returns %d\n", status);

	/*
	 * Failure of tearing down or writing back VbNvContext is non-fatal. So
	 * we just complain about it, but don't return error code for it.
	 */
	if (VbNvTeardown(nvcxt))
		VBDEBUG(PREFIX "fail to tear down VbNvContext\n");
	else if (nvcxt->raw_changed && write_nvcontext(nvcxt))
		VBDEBUG(PREFIX "fail to write back nvcontext\n");

	if (status == LOAD_KERNEL_SUCCESS)
		return boot_kernel_helper(&params);

	return status;
}

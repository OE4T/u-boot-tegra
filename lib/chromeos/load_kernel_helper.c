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
#include <part.h>
#include <chromeos/common.h>
#include <chromeos/gpio.h>
#include <chromeos/kernel_shared_data.h>
#include <chromeos/load_kernel_helper.h>
#include <chromeos/os_storage.h>
#include <chromeos/vboot_nvstorage_helper.h>

/* TODO For load fmap; remove when not used */
#include <chromeos/firmware_storage.h>

/* TODO For strcpy; remove when not used */
#include <linux/string.h>

/* TODO For GoogleBinaryBlockHeader; remove when not used */
#include <gbb_header.h>

/* TODO remove when not used */
extern uint64_t get_nvcxt_lba(void);

/* defined in common/cmd_bootm.c */
int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#include <load_kernel_fw.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

/* This is used to keep u-boot and kernel in sync */
#define SHARED_MEM_VERSION 1

#undef PREFIX
#define PREFIX "load_kernel_wrapper: "

int load_kernel_wrapper_core(LoadKernelParams *params,
			     void *gbb_data, uint64_t gbb_size,
			     uint64_t boot_flags, VbNvContext *nvcxt,
			     uint8_t *shared_data_blob,
			     int bypass_load_kernel)
{
	/*
	 * TODO(clchiou): Hack for bringing up factory; preserve recovery
	 * reason before LoadKernel destroys it. Remove when not needed.
	 */
	uint32_t reason = 0;
	VbNvGet(nvcxt, VBNV_RECOVERY_REQUEST, &reason);

	int status = LOAD_KERNEL_NOT_FOUND;
	block_dev_desc_t *dev_desc;

	memset(params, '\0', sizeof(*params));

	if (!bypass_load_kernel) {
		dev_desc = get_bootdev();
		if (!dev_desc) {
			VBDEBUG(PREFIX "get_bootdev fail\n");
			goto EXIT;
		}
	}

	params->gbb_data = gbb_data;
	params->gbb_size = gbb_size;

	params->boot_flags = boot_flags;
	params->shared_data_blob = shared_data_blob;
	params->shared_data_size = VB_SHARED_DATA_REC_SIZE;

	params->bytes_per_lba = get_bytes_per_lba();
	params->ending_lba = get_ending_lba();

	params->kernel_buffer = (uint8_t*)CONFIG_CHROMEOS_KERNEL_LOADADDR;
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

	if (!bypass_load_kernel) {
		status = LoadKernel(params);
	} else {
		status = LOAD_KERNEL_SUCCESS;
		params->partition_number = 2;
	}

EXIT:
	VBDEBUG(PREFIX "LoadKernel status: %d\n", status);
	if (status == LOAD_KERNEL_SUCCESS) {
		VBDEBUG(PREFIX "partition_number:   0x%08x\n",
				(int) params->partition_number);
		VBDEBUG(PREFIX "bootloader_address: 0x%08x\n",
				(int) params->bootloader_address);
		VBDEBUG(PREFIX "bootloader_size:    0x%08x\n",
				(int) params->bootloader_size);

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

	/*
	 * TODO(clchiou): This is an urgent hack for bringing up factory. We
	 * fill in data that will be used by kernel at last 1MB space.
	 *
	 * Rewrite this part after the protocol specification between
	 * Chrome OS firmware and kernel is finalized.
	 */
	if (status == LOAD_KERNEL_SUCCESS) {
		KernelSharedDataType *sd = get_kernel_shared_data();

		GoogleBinaryBlockHeader *gbbh =
			(GoogleBinaryBlockHeader*) gbb_data;
		int i;

		VBDEBUG(PREFIX "kernel shared data at %p\n", sd);

		strcpy((char*) sd->signature, "CHROMEOS");
		sd->version = SHARED_MEM_VERSION;

		/*
		 * chsw bit value
		 *   bit 0x00000002 : recovery button pressed
		 *   bit 0x00000020 : developer mode enabled
		 *   bit 0x00000200 : firmware write protect disabled
		 */
		if (params->boot_flags & BOOT_FLAG_RECOVERY)
			sd->chsw |= 0x002;
		if (params->boot_flags & BOOT_FLAG_DEVELOPER)
			sd->chsw |= 0x020;
		if (!is_firmware_write_protect_gpio_asserted())
			sd->chsw |= 0x200; /* write protect is disabled */

		strncpy((char*) sd->hwid,
				gbb_data + gbbh->hwid_offset, gbbh->hwid_size);

		/* boot reason; always 0 */
		sd->binf[0] = 0;

		/* active main firmware; TODO: rewritable B (=2) */
		if (params->boot_flags & BOOT_FLAG_RECOVERY)
			sd->binf[1] = 0;
		else
			sd->binf[1] = 1; /* rewritable A */

		/* active EC firmware; TODO: rewritable (=1) */
		sd->binf[2] = 0;

		/* active firmware type */
		if (params->boot_flags & BOOT_FLAG_RECOVERY)
			sd->binf[3] = 0;
		else if (params->boot_flags & BOOT_FLAG_DEVELOPER)
			sd->binf[3] = 2;
		else
			sd->binf[3] = 1;

		/* recovery reason */
		sd->binf[4] = reason;

		sd->write_protect_sw =
			is_firmware_write_protect_gpio_asserted();
		sd->recovery_sw = is_recovery_mode_gpio_asserted();
		sd->developer_sw = is_developer_mode_gpio_asserted();

		sd->vbnv[0] = 0;
		sd->vbnv[1] = VBNV_BLOCK_SIZE;

		firmware_storage_t file;
		firmware_storage_init(&file);
		firmware_storage_read(&file,
				CONFIG_OFFSET_FMAP, CONFIG_LENGTH_FMAP,
				sd->shared_data_body);
		file.close(file.context);
		sd->fmap_base = (uint32_t)sd->shared_data_body;

		sd->total_size = sizeof(*sd);

		sd->nvcxt_lba = get_nvcxt_lba();

		memcpy(sd->nvcxt_cache,
				params->nv_context->raw, VBNV_BLOCK_SIZE);

		VBDEBUG(PREFIX "version  %08x\n", sd->version);
		VBDEBUG(PREFIX "chsw     %08x\n", sd->chsw);
		for (i = 0; i < 5; i++)
			VBDEBUG(PREFIX "binf[%2d] %08x\n", i, sd->binf[i]);
		VBDEBUG(PREFIX "vbnv[ 0] %08x\n", sd->vbnv[0]);
		VBDEBUG(PREFIX "vbnv[ 1] %08x\n", sd->vbnv[1]);
		VBDEBUG(PREFIX "nvcxt    %08llx\n", sd->nvcxt_lba);
		VBDEBUG(PREFIX "nvcxt_c  ");
		for (i = 0; i < VBNV_BLOCK_SIZE; i++)
			VBDEBUG("%02x", sd->nvcxt_cache[i]);
		putc('\n');
		VBDEBUG(PREFIX "write_protect_sw %d\n", sd->write_protect_sw);
		VBDEBUG(PREFIX "recovery_sw      %d\n", sd->recovery_sw);
		VBDEBUG(PREFIX "developer_sw     %d\n", sd->developer_sw);
		VBDEBUG(PREFIX "hwid     \"%s\"\n", sd->hwid);
		VBDEBUG(PREFIX "fwid     \"%s\"\n", sd->fwid);
		VBDEBUG(PREFIX "frid     \"%s\"\n", sd->frid);
		VBDEBUG(PREFIX "fmap     %08x\n", sd->fmap_base);
	}

	return status;
}

int load_kernel_wrapper(LoadKernelParams *params,
			void *gbb_data, uint64_t gbb_size,
			uint64_t boot_flags, VbNvContext *nvcxt,
			uint8_t *shared_data_blob)
{
	return load_kernel_wrapper_core(params, gbb_data, gbb_size, boot_flags,
					nvcxt, shared_data_blob, 0);
}

/* Maximum kernel command-line size */
#define CROS_CONFIG_SIZE 4096

/* Size of the x86 zeropage table */
#define CROS_PARAMS_SIZE 4096

static int load_kernel_config(uint64_t bootloader_address)
{
	char buf[80 + CROS_CONFIG_SIZE];
	const uint32_t addr = (uint32_t)bootloader_address;

	strcpy(buf, "setenv bootargs ${bootargs} ");

	/* Use the bootloader address to find the kernel config location. */
	strncat(buf, (char*)(addr - CROS_PARAMS_SIZE - CROS_CONFIG_SIZE),
			CROS_CONFIG_SIZE);

	/*
	 * Use run_command instead of setenv because we need variable
	 * substitutions.
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

/*
 * Replace:
 *   %D -> device number
 *   %P -> partition number
 *   %U -> GUID
 *
 * For example:
 *   ("root=/dev/sd%D%P", 2, 3)      -> "root=/dev/sdc3"
 *   ("root=/dev/mmcblk%Dp%P", 0, 5) -> "root=/dev/mmcblk0p5".
 *
 * <cmdline> must have sufficient space for in-place update.
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

static int boot_kernel(LoadKernelParams *params)
{
	char *cmdline, cmdline_buf[4096];
	char load_address[32];
	char *argv[2] = { "bootm", load_address };

	VBDEBUG(PREFIX "boot_kernel\n");
	VBDEBUG(PREFIX "kernel_buffer:      0x%p\n",
			params->kernel_buffer);
	VBDEBUG(PREFIX "bootloader_address: 0x%08x\n",
			(int) params->bootloader_address);

	if (load_kernel_config(params->bootloader_address)) {
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
		VBDEBUG(PREFIX "cmdline after update:  %s\n", getenv("bootargs"));
	} else {
		VBDEBUG(PREFIX "bootargs == NULL\n");
	}

	/*
	 * FIXME: So far bootloader in kernel partition isn't really a
	 * bootloader; instead, it is merely a u-boot scripts that sets kernel
	 * parameters. And therefore we still have to boot kernel to here
	 * by calling do_bootm.
	 */
	sprintf(load_address, "0x%p", params->kernel_buffer);
	VBDEBUG(PREFIX "run command: %s %s\n", argv[0], argv[1]);
	do_bootm(NULL, 0, sizeof(argv)/sizeof(*argv), argv);

	/* should never reach here! */
	VBDEBUG(PREFIX "error: do_bootm() returned\n");
	return LOAD_KERNEL_INVALID;
}

static void prepare_bootargs(void)
{
	/* TODO move to u-boot-config */
	run_command("setenv console console=ttyS0,115200n8", 0);
	run_command("setenv bootargs "
			"${bootargs} ${console} ${platform_extras}", 0);
}

int load_and_boot_kernel(void *gbb_data, uint64_t gbb_size,
		uint64_t boot_flags)
{
	LoadKernelParams params;
	VbNvContext nvcxt;
	int status;

	if (read_nvcontext(&nvcxt)) {
		/*
		 * Even if we can't read nvcxt, we continue anyway because this
		 * is developer firmware
		 */
		VBDEBUG(PREFIX "fail to read nvcontext\n");
	}

	prepare_bootargs();

	status = load_kernel_wrapper(&params, gbb_data, gbb_size,
			boot_flags, &nvcxt, NULL);

	VBDEBUG(PREFIX "load_kernel_wrapper returns %d\n", status);

	if (VbNvTeardown(&nvcxt) ||
			(nvcxt.raw_changed && write_nvcontext(&nvcxt))) {
		/*
		 * Even if we can't read nvcxt, we continue anyway because this
		 * is developer firmware
		 */
		VBDEBUG(PREFIX "fail to write nvcontext\n");
	}

	if (status == LOAD_KERNEL_SUCCESS)
		return boot_kernel(&params);

	return status;
}

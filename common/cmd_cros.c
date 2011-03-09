/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Debug commands for Chrome OS Verify Boot.  Probably not useful to you if
 * you are not developing firmware stuff. */

#include <common.h>
#include <command.h>
#include <chromeos/boot_device_impl.h>
#include <chromeos/hardware_interface.h>
#include <chromeos/fmap.h>
#include <chromeos/gbb_bmpblk.h>

/* Verify Boot interface */
#include <boot_device.h>
#include <gbb_header.h>
#include <load_firmware_fw.h>
#include <load_kernel_fw.h>
#include <vboot_struct.h>

#define USAGE(ret, cmdtp, fmt, ...) do { \
	printf(fmt, ##__VA_ARGS__); \
	cmd_usage(cmdtp); \
	return (ret); \
} while (0)

int do_cros	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_test_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_bootdev	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#ifdef CONFIG_CHROMEOS_BMPBLK
int do_bmpblk	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif /* CONFIG_CHROMEOS_BMPBLK */
int do_fmap	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_load_fw	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_load_k	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_cros_help(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

U_BOOT_CMD(cros, CONFIG_SYS_MAXARGS, 1, do_cros,
	"perform action (try \"cros help\")",
	"[action [args...]]\n    - perform action with arguments"
);

cmd_tbl_t cmd_cros_sub[] = {
	U_BOOT_CMD_MKENT(test_gpio, 0, 1, do_test_gpio,
			"test Chrome OS verified boot GPIOs",
			""),
	U_BOOT_CMD_MKENT(bootdev, 4, 1, do_bootdev,
		"show/set/read/write boot device",
		"[sub-action args...]\n    - perform sub-action\n"
		"\n"
		"Subactions:\n"
		"    - show boot device (when no sub-action)\n"
		"set iface dev [part]\n    - set boot device\n"
		"read addr block count\n    - read from boot device\n"
		"write addr block count\n    - write to boot device"),
#ifdef CONFIG_CHROMEOS_BMPBLK
	U_BOOT_CMD_MKENT(bmpblk, 3, 1, do_bmpblk,
	        "Manipulate GBB BMP block",
	        "display <gbbaddr> <screen#> - display the screen\n"
	        "bmpblk info <gbbaddr> <screen#>    - print the screen info\n"),
#endif /* CONFIG_CHROMEOS_BMPBLK */
	U_BOOT_CMD_MKENT(fmap, 2, 1, do_fmap,
		"Find and print flash map",
		"addr len\n    - Find and print flash map "
		"in memory[addr, addr+len]\n"),
	U_BOOT_CMD_MKENT(load_fw, 3, 1, do_load_fw,
		"Load firmware from memory "
		"(you have to download the image before running this)",
		"addr len shdata\n    - Wrapper of LoadFirmware."
		"Load firmware from [addr, addr+len] and "
		"store shared data at shdata\n"),
	U_BOOT_CMD_MKENT(load_k, 3, 1, do_load_k,
		"Load kernel from the boot device",
		"addr len shdata\n    - Load kernel to [addr, addr+len] and\n"
		"modify shared data at shdata\n"),
	U_BOOT_CMD_MKENT(help, 1, 1, do_cros_help,
		"show this message",
		"[action]")
};

int do_cros(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		USAGE(0, cmdtp, "Missing action\n");

	c = find_cmd_tbl(argv[1], &cmd_cros_sub[0], ARRAY_SIZE(cmd_cros_sub));
	if (!c)
		USAGE(1, cmdtp, "Unrecognized action: %s\n", argv[1]);

	return c->cmd(c, flag, argc - 1, argv + 1);
}

static void sleep(int second)
{
	const ulong start = get_timer(0);
	const ulong delay = second * CONFIG_SYS_HZ;

	while (!ctrlc() && get_timer(start) < delay)
		udelay(100);
}

int do_test_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum { WAIT = 2 }; /* 2 seconds */

	struct {
		const char *gpio_name;
		int (*accessor)(void);
	} testcase[3] = {
		{
			.gpio_name = "write protect",
			.accessor = is_firmware_write_protect_gpio_asserted
		},
		{
			.gpio_name = "recovery mode",
			.accessor = is_recovery_mode_gpio_asserted
		},
		{
			.gpio_name = "developer mode",
			.accessor = is_developer_mode_gpio_asserted
		},
	};
	const char *gpio_name;
	int i;

	for (i = 0; i < sizeof(testcase) / sizeof(testcase[0]); i ++) {
		gpio_name = testcase[i].gpio_name;

		printf("Please enable %s...\n", gpio_name);
		sleep(WAIT);
		if (testcase[i].accessor())
			printf("TEST PASS: %s is enabled\n", gpio_name);
		else
			printf("TEST FAIL: %s is not enabled\n", gpio_name);
		printf("\n");

		printf("Please disable %s...\n", gpio_name);
		sleep(WAIT);
		if (!testcase[i].accessor())
			printf("TEST PASS: %s is disabled\n", gpio_name);
		else
			printf("TEST FAIL: %s is not disabled\n", gpio_name);
		printf("\n");
	}

	return 0;
}

int do_bootdev(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum {
		SET, READ, WRITE
	} opcode;

	block_dev_desc_t *dev_desc;
	int dev, part, retcode;
	uint64_t blk, cnt;
	void *buf;

	if (argc < 2) { /* show boot device information */
		if ((dev_desc = get_bootdev()) == NULL)
			puts("No boot device set\n");
		else {
			printf("offset=0x%lx limit=0x%lx\n", get_offset(),
					get_limit());
			dev_print(dev_desc);
		}
		return 0;
	}

	if (!strcmp(argv[1], "set"))
		opcode = SET;
	else if (!strcmp(argv[1], "read"))
		opcode = READ;
	else if (!strcmp(argv[1], "write"))
		opcode = WRITE;
	else
		USAGE(1, cmdtp, "Unrecognized action: %s\n", argv[1]);

	/* apply De Morgan's laws on
	 * !((argc == 4 && opcode == SET) || argc == 5) */
	if ((argc != 4 || opcode != SET) && argc != 5)
		USAGE(1, cmdtp, "Wrong number of arguments\n");

	if (opcode == SET) {
		dev = (int) simple_strtoul(argv[3], NULL, 16);
		part = (argc < 5) ?
			0 : (int) simple_strtoul(argv[4], NULL, 16);

		printf("Set boot device to %s %d %d\n", argv[2], dev, part);
		if (set_bootdev(argv[2], dev, part)) {
			puts("Set bootdev failed\n");
			return 1;
		}

		return 0;
	}

	/* assert(opcode == READ || opcode == WRITE); */

	buf = (void *) simple_strtoul(argv[2], NULL, 16);
	blk = (uint64_t) simple_strtoul(argv[3], NULL, 16);
	cnt = (uint64_t) simple_strtoul(argv[4], NULL, 16);

	retcode = (opcode == READ) ?
		BootDeviceReadLBA(blk, cnt, buf) :
		BootDeviceWriteLBA(blk, cnt, buf);

	if (retcode)
		USAGE(1, cmdtp, opcode == READ ?
				"Read failed\n" : "Write failed\n");

	return 0;
}

#ifdef CONFIG_CHROMEOS_BMPBLK
int do_bmpblk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
        uint8_t *addr;
        int index;
	int ret;

	if (argc != 4)
		return cmd_usage(cmdtp);

	addr = (uint8_t *)simple_strtoul(argv[2], NULL, 16);
	index = (int)simple_strtoul(argv[3], NULL, 10);

	if (!strcmp(argv[1], "info"))
		return print_screen_info_in_bmpblk(addr, index);

	if (!strcmp(argv[1], "display")) {
		ret = display_screen_in_bmpblk(addr, index);
		switch (ret) {
			case BMPBLK_OK:
				return 0;
			case BMPBLK_UNSUPPORTED_COMPRESSION:
				printf("Error: Compression not supported.\n");
				break;
			case BMPBLK_LZMA_DECOMPRESS_FAILED:
				printf("Error: LZMA decompress failed.\n");
				break;
			case BMPBLK_BMP_DISPLAY_FAILED:
				printf("Error: BMP display failed.\n");
				break;
			default:
				printf("Unknown failure: %d.\n", ret);
				break;
		}

		return 1;
        }

	return cmd_usage(cmdtp);
}
#endif /* CONFIG_CHROMEOS_BMPBLK */

static void _print_fmap(struct fmap *fmap)
{
	static const struct {
		uint16_t flag;
		const char *str;
	} flag2str[] = {
		{ 1 << 0, "static" },
		{ 1 << 1, "compressed" },
		{ 0, NULL },
	};

	int i, j;
	uint16_t flags;

	printf("fmap_signature: 0x%016llx\n",
			(unsigned long long) fmap->signature);
	printf("fmap_ver_major: %d\n", fmap->ver_major);
	printf("fmap_ver_minor: %d\n", fmap->ver_minor);
	printf("fmap_base: 0x%016llx\n", (unsigned long long) fmap->base);
	printf("fmap_size: 0x%04x\n", fmap->size);
	printf("fmap_name: \"%s\"\n", fmap->name);
	printf("fmap_nareas: %d\n", fmap->nareas);

	for (i = 0; i < fmap->nareas; i++) {
		printf("area_offset: 0x%08x\n", fmap->areas[i].offset);
		printf("area_size: 0x%08x\n", fmap->areas[i].size);
		printf("area_name: \"%s\"\n", fmap->areas[i].name);
		printf("area_flags_raw: 0x%02x\n", fmap->areas[i].flags);

		flags = fmap->areas[i].flags;
		if (!flags)
			continue;

		puts("area_flags:");
		for (j = 0; flag2str[j].flag; j++)
			if (flags & flag2str[j].flag)
				printf(" %s", flag2str[j].str);
		putc('\n');
	}
}

int do_fmap(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const uint8_t	*addr;
	size_t		len;
	off_t		offset;
	struct fmap	*fmap;

	if (argc != 3)
		USAGE(1, cmdtp, "Wrong number of arguments\n");

	addr	= (const uint8_t *) simple_strtoul(argv[1], NULL, 16);
	len	= (size_t) simple_strtoul(argv[2], NULL, 16);
	offset	= fmap_find(addr, len);
	if (offset < 0) {
		printf("No map found in addr=0x%08lx len=0x%08x\n",
				(unsigned long) addr, len);
		return 1;
	}
	fmap	= (struct fmap *) (addr + offset);

	_print_fmap(fmap);

	return 0;
}

struct context_t {
	void *begin, *cur, *end;
};

static off_t mem_seek(void *context, off_t offset, enum whence_t whence)
{
	struct context_t *cxt = (struct context_t *) context;
	void *cur;

	if (whence == SEEK_SET)
		cur = cxt->begin + offset;
	else if (whence == SEEK_CUR)
		cur = cxt->cur + offset;
	else if (whence == SEEK_END)
		cur = cxt->end + offset;
	else {
		debug("mem_seek: unknown whence value: %d\n", whence);
		return -1;
	}

	if (cur < cxt->begin) {
		debug("mem_seek: offset underflow: %p < %p\n", cur, cxt->begin);
		return -1;
	}

	if (cur >= cxt->end) {
		debug("mem_seek: offset exceeds size: %p >= %p\n", cur,
				cxt->end);
		return -1;
	}

	cxt->cur = cur;
	return cur - cxt->begin;
}

static ssize_t mem_read(void *context, void *buf, size_t count)
{
	struct context_t *cxt = (struct context_t *) context;

	if (count == 0)
		return 0;

	if (count > cxt->end - cxt->cur)
		count = cxt->end - cxt->cur;

	memcpy(buf, cxt->cur, count);
	cxt->cur += count;

	return count;
}

int do_load_fw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	LoadFirmwareParams params;
        VbNvContext vnc;
	struct context_t context;
	void *block[2];
	uint64_t *psize[2];
	VbKeyBlockHeader *kbh;
	VbFirmwarePreambleHeader *fph;
	int i, status;
	firmware_storage_t f = {
		.seek = mem_seek,
		.read = mem_read,
		.context = (void *) &context
	};

	if (argc != 4)
		USAGE(1, cmdtp, "Wrong number of arguments\n");

	context.begin = (void *) simple_strtoul(argv[1], NULL, 16);
	context.cur = context.begin;
	context.end = context.begin + simple_strtoul(argv[2], NULL, 16);

	GetFirmwareBody_setup(&f, CONFIG_OFFSET_FW_A_DATA,
			CONFIG_OFFSET_FW_B_DATA);

	params.gbb_data = context.begin + CONFIG_OFFSET_GBB;
	params.gbb_size = CONFIG_LENGTH_GBB;

	debug("do_load_fw: params.firmware_root_key_blob:\t%p\n",
			params.firmware_root_key_blob);

	params.verification_block_0 = context.begin + CONFIG_OFFSET_FW_A_KEY;
	params.verification_block_1 = context.begin + CONFIG_OFFSET_FW_B_KEY;

	block[0] = params.verification_block_0;
	block[1] = params.verification_block_1;
	psize[0] =  &params.verification_size_0;
	psize[1] =  &params.verification_size_1;
	for (i = 0; i < 2; i++) {
		kbh = (VbKeyBlockHeader *) block[i];
		fph = (VbFirmwarePreambleHeader *)
			(block[i] + kbh->key_block_size);

		*psize[i] = kbh->key_block_size + fph->preamble_size;

		debug("do_load_fw: params.verification_block_%d:\t%p\n",
				i, block[i]);
		debug("do_load_fw: params.verification_size_%d:\t%08llx\n",
				i, *psize[i]);
	}

	params.shared_data_blob =
		(uint8_t *) simple_strtoul(argv[3], NULL, 16);
	params.shared_data_size = VB_SHARED_DATA_REC_SIZE;

	debug("do_load_fw: params.shared_data_blob:\t%p\n",
			params.shared_data_blob);
	debug("do_load_fw: params.shared_data_size:\t%08llx\n",
			params.shared_data_size);

	params.caller_internal = &f;

	params.boot_flags = 0;

        /* TODO: load vnc.raw from NV storage */
        params.nv_context = &vnc;

	status = LoadFirmware(&params);

	if (vnc.raw_changed) {
		/* TODO: save vnc.raw to NV storage */
        }

	puts("LoadFirmware returns: ");
	switch (status) {
	case LOAD_FIRMWARE_SUCCESS:
		puts("LOAD_FIRMWARE_SUCCESS\n");
		printf("firmware_index: %d\n",
				(int) params.firmware_index);
		break;
	case LOAD_FIRMWARE_RECOVERY:
		puts("LOAD_FIRMWARE_RECOVERY\n");
		break;
	case LOAD_FIRMWARE_REBOOT:
		puts("LOAD_FIRMWARE_REBOOT\n");
		break;
	default:
		printf("%d (unknown value!)\n", status);
		break;
	}

	GetFirmwareBody_dispose(&f);

	return 0;
}

int do_load_k(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	LoadKernelParams par;
        VbNvContext vnc;
	block_dev_desc_t *dev_desc;
	int i, status;

	if (argc != 4)
		USAGE(1, cmdtp, "Wrong number of arguments\n");

	if ((dev_desc = get_bootdev()) == NULL)
		USAGE(1, cmdtp, "No boot device set yet\n");

	par.shared_data_blob =
		(uint8_t *) simple_strtoul(argv[3], NULL, 16);
	par.shared_data_size = VB_SHARED_DATA_REC_SIZE;

	par.bytes_per_lba = (uint64_t) dev_desc->blksz;
	par.ending_lba = (uint64_t) get_limit() - 1;
	par.kernel_buffer = (void *) simple_strtoul(argv[1], NULL, 16);
	par.kernel_buffer_size = (uint64_t) simple_strtoul(argv[2], NULL, 16);
	par.boot_flags = BOOT_FLAG_DEVELOPER | BOOT_FLAG_SKIP_ADDR_CHECK;

	/* TODO: load GBB; see do_load_fw() */
	par.gbb_data = NULL;
	par.gbb_size = 0;

        /* TODO: load vnc.raw from NV storage */
        par.nv_context = &vnc;

	status = LoadKernel(&par);

        if (vnc.raw_changed) {
		/* TODO: save vnc.raw to NV storage */
        }

	switch (status) {
	case LOAD_KERNEL_SUCCESS:
		puts("Success; good kernel found on device\n");
		printf("partition_number: %lld\n",
				par.partition_number);
		printf("bootloader_address: 0x%llx",
				par.bootloader_address);
		printf("bootloader_size: 0x%llx", par.bootloader_size);
		puts("partition_guid:");
		for (i = 0; i < 16; i++)
			printf(" %02x", par.partition_guid[i]);
		putc('\n');
		break;
	case LOAD_KERNEL_NOT_FOUND:
		puts("No kernel found on device\n");
		break;
	case LOAD_KERNEL_INVALID:
		puts("Only invalid kernels found on device\n");
		break;
	case LOAD_KERNEL_RECOVERY:
		puts("Internal error; reboot to recovery mode\n");
		break;
	case LOAD_KERNEL_REBOOT:
		puts("Internal error; reboot to current mode\n");
		break;
	default:
		printf("Unexpected return status from LoadKernel: %d\n",
				status);
		return 1;
	}

	return 0;
}

int do_cros_help(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return _do_help(&cmd_cros_sub[0], ARRAY_SIZE(cmd_cros_sub),
				cmdtp, flag, argc, argv);

	c = find_cmd_tbl(argv[1], &cmd_cros_sub[0], ARRAY_SIZE(cmd_cros_sub));
	if (!c)
		USAGE(1, cmdtp, "Unrecognized action: %s\n", argv[1]);

	cmd_usage(c);
	return 0;
}

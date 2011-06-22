/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * Debug commands for testing basic verified-boot related utilities.
 */

#include <common.h>
#include <command.h>
#include <vboot/firmware_storage.h>
#include <vboot_api.h>

/* We test the regions of GBB. */
#define TEST_FW_START	CONFIG_OFFSET_GBB
#define TEST_FW_LENGTH	CONFIG_LENGTH_GBB

static int do_vboot_test_fwrw(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	int ret = 0;
	firmware_storage_t file;
	uint8_t original_buf[TEST_FW_LENGTH];
	uint8_t target_buf[TEST_FW_LENGTH];
	uint8_t verify_buf[TEST_FW_LENGTH];
	int i;

	/* Fill the target test pattern. */
	for (i = 0; i < TEST_FW_LENGTH; i++)
		target_buf[i] = i & 0xFF;

	/* Open firmware storage device. */
	if (firmware_storage_open_spi(&file)) {
		VbExDebug("Failed to open firmware device!\n");
		return 1;
	}

	if (firmware_storage_read(&file, TEST_FW_START, TEST_FW_LENGTH,
			original_buf)) {
		VbExDebug("Failed to read firmware!\n");
		return 1;
	}

	if (firmware_storage_write(&file, TEST_FW_START, TEST_FW_LENGTH,
			target_buf)) {
		VbExDebug("Failed to write firmware!\n");
		ret = 1;
	} else {
		/* Read back and verify the data. */
		firmware_storage_read(&file, TEST_FW_START, TEST_FW_LENGTH,
				verify_buf);
		if (memcmp(target_buf, verify_buf, TEST_FW_LENGTH) != 0) {
			VbExDebug("Verify failed. The target data wrote "
				  "wrong.\n");
			ret = 1;
		}
	}

	 /* Write the original data back. */
	firmware_storage_write(&file, TEST_FW_START, TEST_FW_LENGTH,
			original_buf);

	if (ret == 0)
		VbExDebug("Read and write firmware test SUCCESS.\n");

	return ret;
}

static int do_vboot_test_all(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	int ret = 0;

	ret |= do_vboot_test_fwrw(cmdtp, flag, argc, argv);

	return ret;
}

static cmd_tbl_t cmd_vboot_test_sub[] = {
	U_BOOT_CMD_MKENT(all, 0, 1, do_vboot_test_all, "", ""),
	U_BOOT_CMD_MKENT(fwrw, 0, 1, do_vboot_test_fwrw, "", ""),
};

static int do_vboot_test(cmd_tbl_t *cmdtp,
		int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vboot_test_sub[0],
			ARRAY_SIZE(cmd_vboot_test_sub));
	if (c)
		return c->cmd(c, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vboot_test, CONFIG_SYS_MAXARGS, 1, do_vboot_test,
	"Perform tests for basic vboot related utilities",
	"all - perform all tests\n"
	"vboot_test fwrw - test the firmware read/write functions\n"
);


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
 * Debug commands for testing Verify Boot Export APIs that are implemented by
 * firmware and exported to vboot_reference. Some of the tests are automatic
 * but some of them are manual.
 */

#include <common.h>
#include <command.h>
#include <vboot_api.h>

static int do_vbexport_test_debug(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char c = 'K';
	const char s[] = "Hello! It's \"Chrome OS\".";
	const int16_t hd = -22222;
	const uint16_t hu = 44444;
	const int32_t ld = -1111111111L;
	const uint32_t lu = 2222222222UL;
	const int64_t lld = -8888888888888888888LL;
	const uint64_t llu = 11111111111111111111ULL;
	VbExDebug("The \"Expect\" and \"Actual\" should be the same...\n");
	VbExDebug("Expect: K 75 Hello! It's \"Chrome OS\".\n");
	VbExDebug("Actual: %c %d %s\n", c, c, s);
	VbExDebug("Expect: -22222 0xa932\n");
	VbExDebug("Actual: %hd 0x%hx\n", hd, hd);
	VbExDebug("Expect: 44444 0xad9c\n");
	VbExDebug("Actual: %hu 0x%hx\n", hu, hu);
	VbExDebug("Expect: -1111111111 0xbdc5ca39\n");
	VbExDebug("Actual: %ld 0x%lx\n", ld, ld);
	VbExDebug("Expect: 2222222222 0x84746b8e\n");
	VbExDebug("Actual: %lu 0x%lx\n", lu, lu);
	VbExDebug("Expect: -8888888888888888888 0x84a452a6a1dc71c8\n");
	VbExDebug("Actual: %lld 0x%llx\n", lld, lld);
	VbExDebug("Expect: 11111111111111111111 0x9a3298afb5ac71c7\n");
	VbExDebug("Actual: %llu 0x%llx\n", llu, llu);
	return 0;
}

static int do_vbexport_test_malloc_size(uint32_t size)
{
	char *mem = VbExMalloc(size);
	int i;
	VbExDebug("Trying to malloc a memory block for %lu bytes...", size);
	if ((uint32_t)mem % CACHE_LINE_SIZE != 0) {
		VbExDebug("\nMemory not algined with a cache line!\n");
		VbExFree(mem);
		return 1;
	}
	for (i = 0; i < size; i++) {
		mem[i] = i % 0x100;
	}
	for (i = 0; i < size; i++) {
		if (mem[i] != i % 0x100) {
			VbExDebug("\nMemory verification failed!\n");
			VbExFree(mem);
			return 1;
		}
	}
	VbExFree(mem);
	VbExDebug(" - SUCCESS\n");
	return 0;
}

static int do_vbexport_test_malloc(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Preforming the malloc/free tests...\n");
	ret |= do_vbexport_test_malloc_size(1);
	ret |= do_vbexport_test_malloc_size(2);
	ret |= do_vbexport_test_malloc_size(4);
	ret |= do_vbexport_test_malloc_size(8);
	ret |= do_vbexport_test_malloc_size(32);
	ret |= do_vbexport_test_malloc_size(1024);
	ret |= do_vbexport_test_malloc_size(4 * 1024);
	ret |= do_vbexport_test_malloc_size(32 * 1024);
	ret |= do_vbexport_test_malloc_size(1 * 1024 * 1024);
	ret |= do_vbexport_test_malloc_size(12345);
	ret |= do_vbexport_test_malloc_size(13579);
	return ret;
}

typedef void (*sleep_handler_t)(uint32_t);

static void sleep_ms_handler(uint32_t msec)
{
	VbExSleepMs(msec);
}

static void beep_handler(uint32_t msec)
{
	VbExBeep(msec, 4000);
}

static int do_vbexport_test_sleep_time(sleep_handler_t handler, uint32_t msec)
{
	uint32_t start, end, delta, expected;
	VbExDebug("System is going to sleep for %lu ms...\n", msec);
	start = VbExGetTimer();
	(*handler)(msec);
	end = VbExGetTimer();
	delta = end - start;
	VbExDebug("From tick %lu to %lu (delta: %lu)", start, end, delta);

	expected = msec * CONFIG_SYS_HZ;
	/* The sleeping time should be accurate to within 10%. */
	if (delta > expected + expected / 10) {
		VbExDebug("\nSleep too long: expected %lu but actaully %lu!\n",
				expected, delta);
		return 1;
	} else if (delta < expected - expected / 10) {
		VbExDebug("\nSleep too short: expected %lu but actaully %lu!\n",
				expected, delta);
		return 1;
	}
	VbExDebug(" - SUCCESS\n");
	return 0;
}

static int do_vbexport_test_sleep(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Preforming the sleep tests...\n");
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 10);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 50);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 100);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 500);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 1000);
	return ret;
}

static int do_vbexport_test_longsleep(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Preforming the long sleep tests...\n");
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 5000);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 10000);
	ret |= do_vbexport_test_sleep_time(&sleep_ms_handler, 50000);
	return ret;
}

static int do_vbexport_test_beep(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	VbExDebug("Preforming the beep tests...\n");
	ret |= do_vbexport_test_sleep_time(&beep_handler, 500);
	return ret;
}

static int do_vbexport_test_all(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	ret |= do_vbexport_test_debug(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_malloc(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_sleep(cmdtp, flag, argc, argv);
	ret |= do_vbexport_test_beep(cmdtp, flag, argc, argv);
	if (!ret)
		VbExDebug("All tests passed!\n");
	return ret;
}

static cmd_tbl_t cmd_vbexport_test_sub[] = {
	U_BOOT_CMD_MKENT(all, 0, 1, do_vbexport_test_all, "", ""),
	U_BOOT_CMD_MKENT(debug, 0, 1, do_vbexport_test_debug, "", ""),
	U_BOOT_CMD_MKENT(malloc, 0, 1, do_vbexport_test_malloc, "", ""),
	U_BOOT_CMD_MKENT(sleep, 0, 1, do_vbexport_test_sleep, "", ""),
	U_BOOT_CMD_MKENT(longsleep, 0, 1, do_vbexport_test_longsleep, "", ""),
	U_BOOT_CMD_MKENT(beep, 0, 1, do_vbexport_test_beep, "", ""),
};

static int do_vbexport_test(
		cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vbexport_test_sub[0],
			ARRAY_SIZE(cmd_vbexport_test_sub));
	if (c)
		return c->cmd(c, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vbexport_test, CONFIG_SYS_MAXARGS, 1, do_vbexport_test,
	"Perform tests for vboot_wrapper",
	"all - perform all tests\n"
	"vbexport_test debug - test the debug function\n"
	"vbexport_test malloc - test the malloc and free functions\n"
	"vbexport_test sleep - test the sleep and timer functions\n"
	"vbexport_test longsleep - test the sleep functions for long delays\n"
	"vbexport_test beep - test the beep functions"
);


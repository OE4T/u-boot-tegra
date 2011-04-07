/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* time - run a command and report its run time */

#include <common.h>
#include <command.h>

static void report_time(unsigned long int cycles)
{
#ifdef CONFIG_SYS_HZ
	unsigned long int minutes, seconds, milliseconds;
	unsigned long int total_seconds, remainder;

	total_seconds = cycles / CONFIG_SYS_HZ;
	remainder = cycles % CONFIG_SYS_HZ;
	minutes = total_seconds / 60;
	seconds = total_seconds % 60;
	/* approximate millisecond value */
	milliseconds = (remainder * 1000 + CONFIG_SYS_HZ / 2) / CONFIG_SYS_HZ;

	printf("time:");
	if (minutes)
		printf(" %lu minutes,", minutes);
	printf(" %lu.%03lu seconds,", seconds, milliseconds);
	printf(" %lu ticks\n", cycles);
#else
	printf("CONFIG_SYS_HZ not defined\n");
	printf("time: %lu ticks\n", cycles);
#endif
}

int do_time(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const int target_argc = argc - 1;
	int retval = 0;
	unsigned long int cycles = 0;
	cmd_tbl_t *target_cmdtp = NULL;

	if (argc == 1) {
		printf("no command provided\n");
		return 1;
	}

	/* parse command */
	target_cmdtp = find_cmd(argv[1]);
	if (!target_cmdtp) {
		printf("command not found: %s\n", argv[1]);
		return 1;
	}

	if (target_argc > target_cmdtp->maxargs) {
		printf("maxarags exceeded: %d > %d\n", target_argc,
				target_cmdtp->maxargs);
		return 1;
	}

	/* run the command and report run time */
	cycles = get_timer_masked();
	retval = target_cmdtp->cmd(target_cmdtp, 0, target_argc, argv + 1);
	cycles = get_timer_masked() - cycles;

	putc('\n');
	report_time(cycles);

	return retval;
}

U_BOOT_CMD(time, CONFIG_SYS_MAXARGS, 0, do_time,
		"run a command and report its run time",
		"command [args...]\n"
		"the return value of time is the return value of "
		"the command it executed, "
		"or non-zero if there is an internal error of time.\n");

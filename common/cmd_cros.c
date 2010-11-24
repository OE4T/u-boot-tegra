/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * Support Chrome OS Verify Boot
 */
#include <common.h>
#include <command.h>
#include <part.h>
#include <chromeos/fmap.h>
#include <boot_device.h>

#define USAGE(ret, cmdtp, fmt, ...) do { \
	printf(fmt, ##__VA_ARGS__); \
	cmd_usage(cmdtp); \
	return (ret); \
} while (0)

block_dev_desc_t *get_bootdev(void);
ulong get_offset(void);
ulong get_limit(void);
int set_bootdev(char *ifname, int dev, int part);

int do_cros	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_bootdev	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_fmap	(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_cros_help(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

U_BOOT_CMD(cros, CONFIG_SYS_MAXARGS, 1, do_cros,
	"perform action (try \"cros help\")",
	"[action [args...]]\n    - perform action with arguments"
);

cmd_tbl_t cmd_cros_sub[] = {
	U_BOOT_CMD_MKENT(bootdev, 4, 1, do_bootdev,
			"show/set/read/write boot device",
			"[sub-action args...]\n    - perform sub-action\n"
			"\n"
			"Subactions:\n"
			"    - show boot device (when no sub-action)\n"
			"set iface dev [part]\n    - set boot device\n"
			"read addr block count\n    - read from boot device\n"
			"write addr block count\n    - write to boot device"),
	U_BOOT_CMD_MKENT(fmap, 2, 1, do_fmap,
			"Find and print flash map",
			"addr len\n    - Find and print flash map "
			"in memory[addr, addr+len]\n"),
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

int do_fmap(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static const struct {
		uint16_t flag;
		const char *str;
	} flag2str[] = {
		{ 1 << 0, "static" },
		{ 1 << 1, "compressed" },
		{ 0, NULL },
	};

	const uint8_t	*addr;
	size_t		len;
	off_t		offset;
	struct fmap	*fmap;
	int		i, j;
	uint16_t	flags;

	if (argc != 3)
		USAGE(1, cmdtp, "Wrong number of arguments\n");

	addr	= (const uint8_t *) simple_strtoul(argv[1], NULL, 16);
	len	= (size_t) simple_strtoul(argv[2], NULL, 16);
	offset	= fmap_find(addr, len);
	if (offset < 0) {
		printf("No map found in addr=0x%08x len=0x%08x\n",
				(unsigned) addr, len);
		return 1;
	}
	fmap	= (struct fmap *) (addr + offset);

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

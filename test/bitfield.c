/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Bitfield test routines
 *
 */

#include <stdio.h>

#include "bitfield.h"


#define FULL_RANGE		31:0
#define HIGH_RANGE		31:30
#define MID_RANGE		23:16
#define LOW_RANGE		2:0
#define HIGH_SINGLE_RANGE	31:31
#define MID_SINGLE_RANGE	16:16
#define LOW_SINGLE_RANGE	0:0

static int test_count = 0;

#ifdef DEBUG
#define assert(x) 	\
	({ test_count++; if (!(x)) printf("Assertion failure '%s' %s line %d\n", \
		#x, __FILE__, __LINE__) })
#define asserteq(x, y) 	\
	({ int _x = x; int _y = y; test_count++; \
		if (_x != _y) \
		printf("Assertion failure at %s:%d: '%s' %#x != '%s' %#x\n", \
			__FILE__, __LINE__, #x, _x, #y, _y); })
#else
#define assert(x) test_count++
#define asserteq(x,y) test_count++
#endif


static void test_low_high(void)
{
	asserteq(BITFIELD_HIGHBIT(FULL_RANGE), 31);
	asserteq(BITFIELD_LOWBIT(FULL_RANGE), 0);
	asserteq(BITFIELD_HIGHBIT(HIGH_RANGE), 31);
	asserteq(BITFIELD_LOWBIT(HIGH_RANGE), 30);
	asserteq(BITFIELD_HIGHBIT(MID_RANGE), 23);
	asserteq(BITFIELD_LOWBIT(MID_RANGE), 16);
	asserteq(BITFIELD_HIGHBIT(LOW_RANGE), 2);
	asserteq(BITFIELD_LOWBIT(LOW_RANGE), 0);
	asserteq(BITFIELD_HIGHBIT(HIGH_SINGLE_RANGE), 31);
	asserteq(BITFIELD_LOWBIT(HIGH_SINGLE_RANGE), 31);
	asserteq(BITFIELD_HIGHBIT(MID_SINGLE_RANGE), 16);
	asserteq(BITFIELD_LOWBIT(MID_SINGLE_RANGE), 16);
	asserteq(BITFIELD_HIGHBIT(LOW_SINGLE_RANGE), 0);
	asserteq(BITFIELD_LOWBIT(LOW_SINGLE_RANGE), 0);
}

static void test_width(void)
{
	asserteq(BITFIELD_WIDTH(FULL_RANGE), 32);
	asserteq(BITFIELD_WIDTH(HIGH_RANGE), 2);
	asserteq(BITFIELD_WIDTH(MID_RANGE), 8);
	asserteq(BITFIELD_WIDTH(LOW_RANGE), 3);
	asserteq(BITFIELD_WIDTH(HIGH_SINGLE_RANGE), 1);
	asserteq(BITFIELD_WIDTH(MID_SINGLE_RANGE), 1);
	asserteq(BITFIELD_WIDTH(LOW_SINGLE_RANGE), 1);
}

static void test_shift(void)
{
	asserteq(bf_shift(FULL), 0);
	asserteq(bf_shift(HIGH), 30);
	asserteq(bf_shift(MID), 16);
	asserteq(bf_shift(LOW), 0);
	asserteq(bf_shift(HIGH_SINGLE), 31);
	asserteq(bf_shift(MID_SINGLE), 16);
	asserteq(bf_shift(LOW_SINGLE), 0);
}

static void test_rawmask(void)
{
	asserteq(bf_rawmask(FULL), 0xffffffffU);
	asserteq(bf_rawmask(HIGH), 0x3);
	asserteq(bf_rawmask(MID), 0xff);
	asserteq(bf_rawmask(LOW), 0x7);
	asserteq(bf_rawmask(HIGH_SINGLE), 0x1);
	asserteq(bf_rawmask(MID_SINGLE), 0x1);
	asserteq(bf_rawmask(LOW_SINGLE), 0x1);
}

static void test_mask(void)
{
	asserteq(bf_mask(FULL), 0xffffffffU);
	asserteq(bf_mask(HIGH), 0xc0000000);
	asserteq(bf_mask(MID), 0x00ff0000);
	asserteq(bf_mask(LOW), 0x7);
	asserteq(bf_mask(HIGH_SINGLE), 0x80000000U);
	asserteq(bf_mask(MID_SINGLE), 0x00010000);
	asserteq(bf_mask(LOW_SINGLE), 0x1);
}

static void test_unpack(void)
{
	asserteq(bf_unpack(FULL, 0), 0);
	asserteq(bf_unpack(FULL, -1U), -1U);
	asserteq(bf_unpack(FULL, 0x12345678), 0x12345678);
	asserteq(bf_unpack(FULL, 0x87654321), 0x87654321);
	asserteq(bf_unpack(FULL, 0xa5a5a5a6), 0xa5a5a5a6);

	asserteq(bf_unpack(HIGH, 0), 0);
	asserteq(bf_unpack(HIGH, -1U), 3);
	asserteq(bf_unpack(HIGH, 0x12345678), 0);
	asserteq(bf_unpack(HIGH, 0x87654321), 2);
	asserteq(bf_unpack(HIGH, 0xa5a5a5a6), 2);

	asserteq(bf_unpack(MID, 0), 0);
	asserteq(bf_unpack(MID, -1U), 0xff);
	asserteq(bf_unpack(MID, 0x12345678), 0x34);
	asserteq(bf_unpack(MID, 0x87654321), 0x65);
	asserteq(bf_unpack(MID, 0xa5a5a5a6), 0xa5);

	asserteq(bf_unpack(LOW, 0), 0);
	asserteq(bf_unpack(LOW, -1U), 7);
	asserteq(bf_unpack(LOW, 0x12345678), 0);
	asserteq(bf_unpack(LOW, 0x87654321), 1);
	asserteq(bf_unpack(LOW, 0xa5a5a5a6), 6);

	asserteq(bf_unpack(HIGH_SINGLE, 0), 0);
	asserteq(bf_unpack(HIGH_SINGLE, -1U), 1);
	asserteq(bf_unpack(HIGH_SINGLE, 0x12345678), 0);
	asserteq(bf_unpack(HIGH_SINGLE, 0x87654321), 1);
	asserteq(bf_unpack(HIGH_SINGLE, 0xa5a5a5a6), 1);

	asserteq(bf_unpack(MID_SINGLE, 0), 0);
	asserteq(bf_unpack(MID_SINGLE, -1U), 1);
	asserteq(bf_unpack(MID_SINGLE, 0x12345678), 0);
	asserteq(bf_unpack(MID_SINGLE, 0x87654321), 1);
	asserteq(bf_unpack(MID_SINGLE, 0xa5a5a5a6), 1);

	asserteq(bf_unpack(LOW_SINGLE, 0), 0);
	asserteq(bf_unpack(LOW_SINGLE, -1U), 1);
	asserteq(bf_unpack(LOW_SINGLE, 0x12345678), 0);
	asserteq(bf_unpack(LOW_SINGLE, 0x87654321), 1);
	asserteq(bf_unpack(LOW_SINGLE, 0xa5a5a5a6), 0);
}

static void test_pack(void)
{
	asserteq(bf_pack(FULL, 0), 0);
	asserteq(bf_pack(FULL, -1U), -1U);
	asserteq(bf_pack(FULL, 0x12345678), 0x12345678);
	asserteq(bf_pack(FULL, 0x87654321), 0x87654321);
	asserteq(bf_pack(FULL, 0xa5a5a5a6), 0xa5a5a5a6);

	asserteq(bf_pack(HIGH, 0), 0);
	asserteq(bf_pack(HIGH, -1U), 0xc0000000);
	asserteq(bf_pack(HIGH, 0x12345678), 0);
	asserteq(bf_pack(HIGH, 0x87654321), 0x40000000);
	asserteq(bf_pack(HIGH, 0xa5a5a5a6), 0x80000000);

	asserteq(bf_pack(MID, 0), 0);
	asserteq(bf_pack(MID, -1U), 0x00ff0000);
	asserteq(bf_pack(MID, 0x12345678), 0x00780000);
	asserteq(bf_pack(MID, 0x87654321), 0x00210000);
	asserteq(bf_pack(MID, 0xa5a5a5a6), 0x00a60000);

	asserteq(bf_pack(LOW, 0), 0);
	asserteq(bf_pack(LOW, -1U), 7);
	asserteq(bf_pack(LOW, 0x12345678), 0);
	asserteq(bf_pack(LOW, 0x87654321), 1);
	asserteq(bf_pack(LOW, 0xa5a5a5a6), 6);

	asserteq(bf_pack(HIGH_SINGLE, 0), 0);
	asserteq(bf_pack(HIGH_SINGLE, -1U), 0x80000000u);
	asserteq(bf_pack(HIGH_SINGLE, 0x12345678), 0);
	asserteq(bf_pack(HIGH_SINGLE, 0x87654321), 0x80000000u);
	asserteq(bf_pack(HIGH_SINGLE, 0xa5a5a5a6), 0x00000000u);

	asserteq(bf_pack(MID_SINGLE, 0), 0);
	asserteq(bf_pack(MID_SINGLE, -1U), 0x00010000);
	asserteq(bf_pack(MID_SINGLE, 0x12345678), 0);
	asserteq(bf_pack(MID_SINGLE, 0x87654321), 0x00010000);
	asserteq(bf_pack(MID_SINGLE, 0xa5a5a5a6), 0x00000000);

	asserteq(bf_pack(LOW_SINGLE, 0), 0);
	asserteq(bf_pack(LOW_SINGLE, -1U), 1);
	asserteq(bf_pack(LOW_SINGLE, 0x12345678), 0);
	asserteq(bf_pack(LOW_SINGLE, 0x87654321), 1);
	asserteq(bf_pack(LOW_SINGLE, 0xa5a5a5a6), 0);
}

static void test_set(void)
{
}

void bf_test(void)
{
	test_low_high();
	test_width();
	test_shift();
	test_rawmask();
	test_unpack();
	test_pack();
}

int main(int argc, char *argv[])
{
	bf_test();
	printf("%d tests run\n", test_count);
	return 0;
}

/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of firmware storage access interface for SPI */

#include <common.h>
#include <spi_flash.h>
#include <vboot/firmware_storage.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

#define PREFIX "firmware_storage_spi: "

#ifndef CONFIG_SF_DEFAULT_SPEED
# define CONFIG_SF_DEFAULT_SPEED	1000000
#endif
#ifndef CONFIG_SF_DEFAULT_MODE
# define CONFIG_SF_DEFAULT_MODE		SPI_MODE_3
#endif

struct context_t {
	struct spi_flash *flash;
	off_t offset;
};

static off_t seek_spi(void *context, off_t offset, enum whence_t whence)
{
	struct context_t *cxt = context;
	off_t next_offset;

	if (whence == SEEK_SET)
		next_offset = offset;
	else if (whence == SEEK_CUR)
		next_offset = cxt->offset + offset;
	else if (whence == SEEK_END)
		next_offset = cxt->flash->size + offset;
	else {
		VbExDebug(PREFIX "unknown whence value: %d\n", whence);
		return -1;
	}

	if (next_offset < 0) {
		VbExDebug(PREFIX "negative offset: %d\n", next_offset);
		return -1;
	}

	if (next_offset > cxt->flash->size) {
		VbExDebug(PREFIX "offset overflow: 0x%08x > 0x%08x\n",
				next_offset, cxt->flash->size);
		return -1;
	}

	cxt->offset = next_offset;
	return cxt->offset;
}

/*
 * Check the right-exclusive range [offset:offset+*count_ptr), and adjust
 * value pointed by <count_ptr> to form a valid range when needed.
 *
 * Return 0 if it is possible to form a valid range. and non-zero if not.
 */
static int border_check(struct spi_flash *flash, off_t offset,
		size_t *count_ptr)
{
	if (offset >= flash->size) {
		VbExDebug(PREFIX "at EOF\n");
		return -1;
	}

	if (offset + *count_ptr > flash->size)
		*count_ptr = flash->size - offset;

	return 0;
}

static ssize_t read_spi(void *context, void *buf, size_t count)
{
	struct context_t *cxt = context;

	if (border_check(cxt->flash, cxt->offset, &count))
		return -1;

	if (cxt->flash->read(cxt->flash, cxt->offset, count, buf)) {
		VbExDebug(PREFIX "SPI read fail\n");
		return -1;
	}

	cxt->offset += count;
	return count;
}

/*
 * FIXME: It is a reasonable assumption that sector size = 4096 bytes.
 * Nevertheless, comparing to coding this magic number here, there should be a
 * better way (maybe rewrite driver interface?) to expose this parameter from
 * eeprom driver.
 */
#define SECTOR_SIZE 0x1000

/*
 * Align the right-exclusive range [*offset_ptr:*offset_ptr+*length_ptr) with
 * SECTOR_SIZE.
 * After alignment adjustment, both offset and length will be multiple of
 * SECTOR_SIZE, and will be larger than or equal to the original range.
 */
static void align_to_sector(size_t *offset_ptr, size_t *length_ptr)
{
	VbExDebug(PREFIX "before adjustment\n");
	VbExDebug(PREFIX "offset: 0x%x\n", *offset_ptr);
	VbExDebug(PREFIX "length: 0x%x\n", *length_ptr);

	/* Adjust if offset is not multiple of SECTOR_SIZE */
	if (*offset_ptr & (SECTOR_SIZE - 1ul)) {
		*offset_ptr &= ~(SECTOR_SIZE - 1ul);
	}

	/* Adjust if length is not multiple of SECTOR_SIZE */
	if (*length_ptr & (SECTOR_SIZE - 1ul)) {
		*length_ptr &= ~(SECTOR_SIZE - 1ul);
		*length_ptr += SECTOR_SIZE;
	}

	VbExDebug(PREFIX "after adjustment\n");
	VbExDebug(PREFIX "offset: 0x%x\n", *offset_ptr);
	VbExDebug(PREFIX "length: 0x%x\n", *length_ptr);
}

static ssize_t write_spi(void *context, const void *buf, size_t count)
{
	struct context_t *cxt = context;
	struct spi_flash *flash = cxt->flash;
	uint8_t static_buf[SECTOR_SIZE];
	uint8_t *backup_buf;
	ssize_t ret = -1;
	size_t offset, length, tmp;
	int status;

	/* We will erase <length> bytes starting from <offset> */
	offset = cxt->offset;
	length = count;
	align_to_sector(&offset, &length);

	tmp = length;
	if (border_check(flash, offset, &tmp))
		return -1;
	if (tmp != length) {
		VbExDebug(PREFIX "cannot erase range [%08x:%08x]: %08x\n",
				offset, offset + length, offset + tmp);
		return -1;
	}

	backup_buf = length > sizeof(static_buf) ? VbExMalloc(length)
						 : static_buf;

	if ((status = flash->read(flash, offset, length, backup_buf))) {
		VbExDebug(PREFIX "cannot backup data: %d\n", status);
		goto EXIT;
	}

	if ((status = flash->erase(flash, offset, length))) {
		VbExDebug(PREFIX "SPI erase fail: %d\n", status);
		goto EXIT;
	}

	VbExDebug(PREFIX "cxt->offset: 0x%08x\n", cxt->offset);
	VbExDebug(PREFIX "offset:      0x%08x\n", offset);

	/* combine data we want to write and backup data */
	memcpy(backup_buf + (cxt->offset - offset), buf, count);

	if (flash->write(flash, offset, length, backup_buf)) {
		VbExDebug(PREFIX "SPI write fail\n");
		goto EXIT;
	}

	cxt->offset += count;
	ret = count;

EXIT:
	if (backup_buf != static_buf)
		VbExFree(backup_buf);

	return ret;
}

static int close_spi(void *context)
{
	struct context_t *cxt = context;

	spi_flash_free(cxt->flash);
	VbExFree(cxt);

	return 0;
}

static int lock_spi(void *context)
{
	/* TODO Implement lock device */
	return 0;
}

int firmware_storage_open_spi(firmware_storage_t *file)
{
	const unsigned int bus = 0;
	const unsigned int cs = 0;
	const unsigned int max_hz = CONFIG_SF_DEFAULT_SPEED;
	const unsigned int spi_mode = CONFIG_SF_DEFAULT_MODE;
	struct context_t *cxt;

	cxt = VbExMalloc(sizeof(*cxt));
	cxt->offset = 0;
	cxt->flash = spi_flash_probe(bus, cs, max_hz, spi_mode);
	if (!cxt->flash) {
		VbExDebug(PREFIX "fail to init SPI flash at %u:%u\n", bus, cs);
		VbExFree(cxt);
		return -1;
	}

	file->seek = seek_spi;
	file->read = read_spi;
	file->write = write_spi;
	file->close = close_spi;
	file->lock_device = lock_spi;
	file->context = (void*) cxt;

	return 0;
}

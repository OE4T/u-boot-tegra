/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_
#define CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_

#include <linux/types.h>
#include <chromeos/firmware_storage.h>

#include <vboot_nvstorage.h>

/*
 * Read/write non-volative storage: return zero if success, non-zero if fail.
 *
 * <nvram> must be at least VBNV_BLOCK_SIZE bytes.
 *
 * See vboot_reference/firmware/include/vboot_nvstorage.h
 */
int read_nvcontext(firmware_storage_t *file, VbNvContext *vnc);
int write_nvcontext(firmware_storage_t *file, VbNvContext *vnc);

#endif /* CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_ */

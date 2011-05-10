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
 * Read/write non-volatile storage: return zero if success, non-zero if fail.
 *
 * <nvram> must be at least VBNV_BLOCK_SIZE bytes.
 *
 * See vboot_reference/firmware/include/vboot_nvstorage.h
 */
int read_nvcontext(VbNvContext *nvcxt);
int write_nvcontext(VbNvContext *nvcxt);

/*
 * Set the recovery request in the non-volatile storage.
 *
 * Return zero if success, non-zero if fail.
 */
int clear_recovery_request(void);

/*
 * Set the recovery request to <reason> and reboot. This function never returns.
 *
 * If <nvcxt> is NULL, this function loads the nvcxt from non-volatile storage.
 */
void reboot_to_recovery_mode(VbNvContext *nvcxt, uint32_t reason);

#endif /* CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_ */

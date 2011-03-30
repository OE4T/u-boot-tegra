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

/* Accessor to non-volatile storage: returns 0 if false, nonzero if true */
int is_debug_reset_mode_field_containing_cookie(void);
int is_recovery_mode_field_containing_cookie(void);
int is_try_firmware_b_field_containing_cookie(void);

#endif /* CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_ */

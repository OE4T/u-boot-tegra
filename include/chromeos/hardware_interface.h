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
 * The minimal hardware interface for Chrome OS verified boot
 */

#ifndef __HARDWARE_INTERFACE_H__
#define __HARDWARE_INTERFACE_H__

int is_s3_resume(void);

/* Accessor to non-volatile storage: returns 0 if false, nonzero if true */
int is_debug_reset_mode_field_containing_cookie(void);
int is_recovery_mode_field_containing_cookie(void);
int is_try_firmware_b_field_containing_cookie(void);

/* GPIO accessor functions: returns 0 if false, nonzero if true */
int is_firmware_write_protect_gpio_asserted(void);
int is_recovery_mode_gpio_asserted(void);
int is_developer_mode_gpio_asserted(void);

/* Returns 0 if success, nonzero if error. */
int lock_down_eeprom(void);

/* TPM driver functions: returns 0 if success, nonzero if error. */
int initialize_tpm(void);
int lock_tpm(void);
int lock_tpm_rewritable_firmware_index(void);

/* Interface for access firmware */
#include <chromeos/firmware_storage.h>

/* Returns 0 if success, nonzero if error. */
int init_caller_internal(caller_internal_t *ci);
int release_caller_internal(caller_internal_t *ci);

#endif /* __HARDWARE_INTERFACE_H__ */

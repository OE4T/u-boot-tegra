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

/* GPIO accessor functions: returns 0 if false, nonzero if true */
int is_firmware_write_protect_gpio_asserted(void);
int is_recovery_mode_gpio_asserted(void);
int is_developer_mode_gpio_asserted(void);

/* TPM driver functions: returns 0 if success, nonzero if error. */
int initialize_tpm(void);
int lock_tpm(void);
int lock_tpm_rewritable_firmware_index(void);

#endif /* __HARDWARE_INTERFACE_H__ */

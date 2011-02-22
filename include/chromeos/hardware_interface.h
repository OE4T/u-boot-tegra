/*
 * Copyright 2011, Google Inc.
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

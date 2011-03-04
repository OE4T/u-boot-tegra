/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board TPM driver functions */

#include <chromeos/hardware_interface.h>

/* TODO Replace dummy functions below with real implementation. */

/* Returns 0 if success, nonzero if error. */
int initialize_tpm(void) { return 0; }
int lock_tpm(void) { return 0; }
int lock_tpm_rewritable_firmware_index(void) { return 0; }

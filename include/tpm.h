/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef TPM_H_
#define TPM_H_

#include <common.h>

int tpm_init(void);
int tpm_send(const uint8_t *pdata, size_t length);
int tpm_receive(uint8_t *pdata, size_t max_length);

#endif /* TPM_H_ */

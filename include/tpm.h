/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef TPM_H_
#define TPM_H_

#include <common.h>

int tis_init(void);
int tis_open(void);
int tis_close(void);
int tis_sendrecv(const uint8_t *sendbuf, size_t send_size, uint8_t *recvbuf,
			size_t *recv_len);

#endif /* TPM_H_ */

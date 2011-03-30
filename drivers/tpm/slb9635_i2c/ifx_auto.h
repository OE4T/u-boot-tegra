/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __tpm_slb9635_i2c_ifx_auto_h__
#define __tpm_slb9635_i2c_ifx_auto_h__

int tpm_init_v03(void);
int tpm_open_v03(void);
int tpm_close_v03(void);
int tpm_sendrecv_v03(const uint8_t *sendbuf, size_t buf_size, uint8_t *recvbuf,
			size_t *recv_len);
int tpm_init_v05(void);
int tpm_open_v05(void);
int tpm_close_v05(void);
int tpm_sendrecv_v05(const uint8_t *sendbuf, size_t buf_size, uint8_t *recvbuf,
			size_t *recv_len);

#endif /* __tpm_slb9635_i2c_ifx_auto_h__ */


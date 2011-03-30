/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <config.h>
#include <common.h>
#include <i2c.h>
#include "tddl.h"

#define TPM_V05_DEFAULT_ADDR (0x20)

#ifdef CONFIG_TPM_SLB9635_I2C_ADDR
#define TPM_V05_ADDR CONFIG_TPM_SLB9635_I2C_ADDR
#else
#define TPM_V05_ADDR TPM_V05_DEFAULT_ADDR
#endif

int tpm_init_v05(void)
{
	return i2c_probe(TPM_V05_ADDR );
}

int tpm_open_v05(void)
{
	if (TDDL_Open(TPM_V05_ADDR ))
		return -1;
	return 0;
}

int tpm_close_v05(void)
{
	if (TDDL_Close())
		return -1;
	return 0;
}

int tpm_sendrecv_v05(const uint8_t *sendbuf, size_t buf_size,
	uint8_t *recvbuf, size_t *recv_len)
{
	if (TDDL_TransmitData((uint8_t*)sendbuf, (uint32_t)buf_size, recvbuf,
		(uint32_t*)recv_len))
		return -1;
	return 0;
}


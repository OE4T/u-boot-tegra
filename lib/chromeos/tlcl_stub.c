/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <config.h>
#include <common.h>
#include <tlcl_stub.h>
#include <tpm.h>

/* A simple TPM library implementation for now */

uint32_t TlclStubInit(void)
{
#ifdef CONFIG_HARDWARE_TPM
	int ret = tis_init();
	if (ret)
		return TPM_E_IOERROR;
	/* tpm-lite lib doesn't call TlclOpenDevice after TlclStubInit */
	ret = tis_open();
	if (ret)
		return TPM_E_IOERROR;
#endif
	return TPM_SUCCESS;
}

uint32_t TlclCloseDevice(void)
{
#ifdef CONFIG_HARDWARE_TPM
	int ret = tis_close();
	if (ret)
		return TPM_E_IOERROR;
#endif
	return TPM_SUCCESS;
}

uint32_t TlclOpenDevice(void)
{
#ifdef CONFIG_HARDWARE_TPM
	int ret = tis_open();
	if (ret)
		return TPM_E_IOERROR;
#endif
	return TPM_SUCCESS;
}

uint32_t TlclStubSendReceive(const uint8_t* request, int request_length,
		uint8_t* response, int max_length)
{
#ifdef CONFIG_HARDWARE_TPM
	int ret;
	size_t recv_len = (size_t) max_length;
	ret= tis_sendrecv(request, request_length, response, &recv_len);
	if (ret)
		return TPM_E_IOERROR;
#endif
	return TPM_SUCCESS;
}


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
	return tpm_init();
#else
	return TPM_SUCCESS;
#endif
}

uint32_t TlclCloseDevice(void)
{
	return TPM_SUCCESS;
}

uint32_t TlclOpenDevice(void)
{
	return TPM_SUCCESS;
}

uint32_t TlclStubSendReceive(const uint8_t* request, int request_length,
		uint8_t* response, int max_length)
{
#ifdef CONFIG_HARDWARE_TPM
	int ret_code;
	ret_code = tpm_send(request, request_length);
	if (ret_code)
		return TPM_E_IOERROR;
	ret_code = tpm_receive(response, max_length);
	if (ret_code)
		return TPM_E_IOERROR;
	return TPM_SUCCESS;
#else
	return TPM_SUCCESS;
#endif
}


/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <config.h>
#include <tpm.h>

/* Import the header files from vboot_reference. */
#include <tss_constants.h>
#include <vboot_api.h>

VbError_t VbExTpmInit(void)
{
/* TODO According to FDT, select the real TPM path or just returning success. */
#ifdef CONFIG_HARDWARE_TPM
	if (tis_init())
		return TPM_E_IOERROR;
	/* tpm_lite lib doesn't call VbExTpmOpene after VbExTpmInit. */
	return VbExTpmOpen();
#endif
	return TPM_SUCCESS;
}

VbError_t VbExTpmClose(void)
{
#ifdef CONFIG_HARDWARE_TPM
	if (tis_close())
		return TPM_E_IOERROR;
#endif
	return TPM_SUCCESS;
}

VbError_t VbExTpmOpen(void)
{
#ifdef CONFIG_HARDWARE_TPM
	if (tis_open())
		return TPM_E_IOERROR;
#endif
	return TPM_SUCCESS;
}

VbError_t VbExTpmSendReceive(const uint8_t* request, uint32_t request_length,
		uint8_t* response, uint32_t* response_length)
{
#ifdef CONFIG_HARDWARE_TPM
	if (tis_sendrecv(request, request_length, response, response_length))
		return TPM_E_IOERROR;
#else
	/* Make a successful response */
	uint8_t successful_response[10] =
			{0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0x0, 0x0, 0x0, 0x0};
	memcpy(response, successful_response, 10);
	*response_length = 10;
#endif
	return TPM_SUCCESS;
}

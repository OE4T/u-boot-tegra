/*
 * Copyright (C) 2011 Infineon Technologies
 *
 * Authors:
 * Peter Huewe <huewe.external@infineon.com>
 *
 * description: Infineon_TPM-I2C - TDDL supporting FW 2010.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Version: 2.1.1
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <exports.h>

#include "tddl.h"
#include "tpm.h"

/* buffer supporting tpm_transmit */
uint8_t tmpbuf[TPM_BUFSIZE];

/* Open connection to TPM */
TDDL_RESULT TDDL_Open(uint32_t dev_addr)
{
	int rc;
	dbg_printf("INFO: initialising TPM\n");
	rc = tpm_open(dev_addr);
	if (rc < 0) {
		switch (rc) {
			case -EBUSY:  return TDDL_E_ALREADY_OPENED;
			case -ENODEV: return TDDL_E_COMPONENT_NOT_FOUND;
		};
		return TDDL_E_FAIL;
	}
	return TDDL_SUCCESS;
}

/* Disconnect from the TPM */
TDDL_RESULT TDDL_Close(void)
{
	tpm_close();
	return TDDL_SUCCESS;
}

/* Send the TPM Application Protocol Data Unit (APDU) to the TPM and
 * return the response APDU */
TDDL_RESULT TDDL_TransmitData(uint8_t *pbTransmitBuf, uint32_t dwTransmitBufLen,
			     uint8_t *pbReceiveBuf, uint32_t *pdwReceiveBufLen)
{
	int len;
	uint32_t i;
	TDDL_RESULT rc = TDDL_E_FAIL;

	dbg_printf("--> xTDDL_TransmitData()\n");

	dbg_printf("pbTransmitBuf = ");
	for (i = 0; i < dwTransmitBufLen; i++)
		dbg_printf("%02x ", pbTransmitBuf[i]);
	dbg_printf("\n");

	/* copy buffer to make sure transmit data may be overwritten */
	memcpy(tmpbuf, pbTransmitBuf, dwTransmitBufLen);

	/* do transmission */
	len = tpm_transmit(tmpbuf, dwTransmitBufLen);

	if (len >= 10) {
		memcpy(pbReceiveBuf, tmpbuf, len);
		dbg_printf("INFO: copied %d bytes into pbReceiveBuf\n", len);
		*pdwReceiveBufLen = len;
		rc = TDDL_SUCCESS;
	} else {
		dbg_printf("ERROR: tpm_transmit() returned %d\n", len);
		*pdwReceiveBufLen = 0;
	}

	dbg_printf("pbReceiveBuf = ");
	for (i = 0; i < (*pdwReceiveBufLen); i++)
		dbg_printf("%02x ", pbReceiveBuf[i]);
	dbg_printf("\n");

	dbg_printf("<-- xTDDL_TransmitData()\n");

	return rc;
}

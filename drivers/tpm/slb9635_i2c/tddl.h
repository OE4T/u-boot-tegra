/*******************************************************************************
**
** FILENAME:      tddl.h
** COPYRIGHT:     Infineon Technologies
** DESCRIPTION:   I2C TPM - TDDL API.
** CREATOR:       Peter Huewe <huewe.external@infineon.com>
** LICENSE:	  GPL
** VERSION:	  2.1.1
*******************************************************************************/
#ifndef _TDDL_H_
#define _TDDL_H_

/* Includes from U-Boot */
#include <linux/types.h>

typedef	uint32_t TDDL_RESULT;

#define TDDL_SUCCESS	0x00000000L
#define TDDL_E_FAIL	0x00000001L
#define TDDL_E_COMPONENT_NOT_FOUND      (TDDL_E_FAIL + 0x00000001L)
#define TDDL_E_ALREADY_OPENED           (TDDL_E_FAIL + 0x00000002L)


/* if dev_addr != 0 - redefines TPM device address */
TDDL_RESULT TDDL_Open(uint32_t dev_addr);

TDDL_RESULT TDDL_Close(void);

TDDL_RESULT TDDL_TransmitData(uint8_t *pbTransmitBuf, uint32_t dwTransmitBufLen,
			uint8_t *pbReceiveBuf, uint32_t *pdwReceiveBufLen);
#endif

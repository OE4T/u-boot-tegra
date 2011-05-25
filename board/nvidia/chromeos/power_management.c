/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board power management function */

#include <common.h>
#include <i2c.h>

#include <chromeos/common.h>
#include <chromeos/power_management.h>

#define PREFIX "cold_reboot: "

#define PMIC_I2C_BUS		0x00
#define PMIC_I2C_DEVICE_ADDRESS	0x34
#define TPS6586X_SUPPLYENE	0x14

/* This function never returns */
void cold_reboot(void)
{
	uint8_t byte;

	if (i2c_set_bus_num(PMIC_I2C_BUS)) {
		VBDEBUG(PREFIX "i2c_set_bus_num fail\n");
		goto FATAL;
	}

	if (i2c_read(PMIC_I2C_DEVICE_ADDRESS, TPS6586X_SUPPLYENE, 1,
				&byte, sizeof(byte))) {
		VBDEBUG(PREFIX "i2c_read fail\n");
		goto FATAL;
	}

	/* Set TPS6586X_SUPPLYENE bit0 to 1 */
	byte |= 1;

	if (i2c_write(PMIC_I2C_DEVICE_ADDRESS, TPS6586X_SUPPLYENE, 1,
				&byte, sizeof(byte))) {
		VBDEBUG(PREFIX "i2c_write fail\n");
		goto FATAL;
	}

	/* The PMIC will reboot the whole system after 10 ms */
	udelay(100);

FATAL:
	/* The final solution of doing a cold reboot */
	printf("Please press cold reboot button\n");
	while (1);
}

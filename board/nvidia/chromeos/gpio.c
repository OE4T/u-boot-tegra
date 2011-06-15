/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board GPIO accessor functions */

#include <common.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tegra2.h>
#include <chromeos/common.h>
#include <chromeos/gpio.h>

#define GPIO_ACTIVE_HIGH	0
#define GPIO_ACTIVE_LOW		1

#define GPIO_ACCESSOR(gpio_number, polarity) \
	gpio_direction_input(gpio_number); \
	return (polarity) ^ gpio_get_value(gpio_number);

int is_firmware_write_protect_gpio_asserted(void)
{
	GPIO_ACCESSOR(GPIO_PH3, GPIO_ACTIVE_HIGH);
}

int is_recovery_mode_gpio_asserted(void)
{
	GPIO_ACCESSOR(GPIO_PH0, GPIO_ACTIVE_LOW);
}

int is_developer_mode_gpio_asserted(void)
{
	GPIO_ACCESSOR(GPIO_PV0, GPIO_ACTIVE_HIGH);
}

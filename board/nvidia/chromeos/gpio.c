/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board GPIO accessor functions */

#include <common.h>
#include "../../common/lcd/gpinit/gpinit.h"

#include <chromeos/hardware_interface.h>

#define GPIO_ACTIVE_HIGH	0
#define GPIO_ACTIVE_LOW		1

#define GPIO_ACCESSOR(gpio_number, polarity) \
	tg2_gpio_direction_input(TEGRA_GPIO_PORT(gpio_number), \
			TEGRA_GPIO_BIT(gpio_number)); \
	return (polarity) ^ tg2_gpio_get_value(TEGRA_GPIO_PORT(gpio_number), \
			TEGRA_GPIO_BIT(gpio_number));

int is_firmware_write_protect_gpio_asserted(void)
{
	GPIO_ACCESSOR(TEGRA_GPIO_PH3, GPIO_ACTIVE_LOW)
}

int is_recovery_mode_gpio_asserted(void)
{
	GPIO_ACCESSOR(TEGRA_GPIO_PH0, GPIO_ACTIVE_LOW)
}

int is_developer_mode_gpio_asserted(void)
{
	GPIO_ACCESSOR(TEGRA_GPIO_PV0, GPIO_ACTIVE_HIGH)
}

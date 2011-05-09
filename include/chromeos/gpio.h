/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* GPIO interface for Chrome OS verified boot */

#ifndef __GPIO_H__
#define __GPIO_H__

/* GPIO accessor functions: returns 0 if false, nonzero if true */
int is_firmware_write_protect_gpio_asserted(void);
int is_recovery_mode_gpio_asserted(void);
int is_developer_mode_gpio_asserted(void);

#endif /* __GPIO_H__ */

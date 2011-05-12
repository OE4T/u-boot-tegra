/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* power management interface for Chrome OS verified boot */

#ifndef CHROMEOS_POWER_MANAGEMENT_H_
#define CHROMEOS_POWER_MANAGEMENT_H_

/* Tell PMIC to cold reboot the whole system */
void cold_reboot(void);

#endif /* CHROMEOS_POWER_MANAGEMENT_H_ */


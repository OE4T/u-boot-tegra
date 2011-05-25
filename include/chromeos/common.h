/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_COMMON_H_
#define CHROMEOS_COMMON_H_

#if defined DEBUG
#define VBDEBUG debug
#elif defined VBOOT_DEBUG
#define VBDEBUG(fmt, args...) printf(fmt ,##args)
#else
#define VBDEBUG(fmt, args...)
#endif

#endif /* CHROMEOS_COMMON_H_ */

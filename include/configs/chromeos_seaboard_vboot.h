/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __configs_chromeos_seaboard_vboot_h__
#define __configs_chromeos_seaboard_vboot_h__

#include <configs/chromeos_seaboard_common.h>

#define CONFIG_VBOOT_WRAPPER
#define CONFIG_CMD_VBEXPORT_TEST

#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY	-1	/* disable auto boot */

#endif /* __configs_chromeos_seaboard_vboot_h__ */

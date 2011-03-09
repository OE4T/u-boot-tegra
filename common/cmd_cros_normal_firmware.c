/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of rewritable firmware of Chrome OS Verify Boot */

#include <common.h>
#include <command.h>
#include <chromeos/boot_device_impl.h>
#include <chromeos/hardware_interface.h>

#include <boot_device.h>
#include <load_kernel_fw.h>
#include <vboot_struct.h>

#define PREFIX "cros_normal_firmware: "

int do_cros_normal_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	return 0;
}

U_BOOT_CMD(cros_normal_firmware, 1, 1, do_cros_normal_firmware,
		"verified boot normal firmware", NULL);

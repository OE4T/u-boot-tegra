/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <chromeos/kernel_shared_data.h>

KernelSharedDataType *get_kernel_shared_data(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	return (KernelSharedDataType *)(
		gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS-1].start +
		gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS-1].size - SZ_1M);
}

void clear_kernel_shared_data(void)
{
	KernelSharedDataType *sd = get_kernel_shared_data();
	memset(sd, '\0', sizeof(*sd));
}

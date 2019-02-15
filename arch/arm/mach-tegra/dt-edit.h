/*
 *  Copyright (C) 2010-2019 NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

void *fdt_copy_get_blob_src_default(void);

int fdt_copy_env_proplist(void *blob_dst);
int fdt_copy_env_nodelist(void *blob_dst);
int fdt_del_env_nodelist(void *blob_dst);
int fdt_del_env_proplist(void *blob_dst);

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 */

struct mmc *init_mmc_device(int dev, bool force_init);
char *print_efiname(gpt_entry *pte);
int is_pte_valid(gpt_entry * pte);
int find_valid_gpt(struct blk_desc *dev_desc, gpt_header *gpt_head, gpt_entry **pgpt_pte);
int find_rp4_data(char *buf, long size);

/* The amount of the rp4 header to probe to obtain what we need */
#define PROBE_BUF_SIZE 32768


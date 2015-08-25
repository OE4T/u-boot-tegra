/*
 * Board info related definitions
 *
 * (C) Copyright 2015 NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _BOARD_INFO_H_
#define _BOARD_INFO_H_

struct ft_board_info {
	u32 id;
	u32 sku;
	u32 fab;
	u32 major;
	u32 minor;
};

int ft_board_info_set(void *blob, const struct ft_board_info *bi,
		      const char *chosen_subnode_name);

#endif

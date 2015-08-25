/*
 *  (C) Copyright 2010-2016
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include "ft_board_info.h"

int ft_board_info_set(void *blob, const struct ft_board_info *bi,
		      const char *chosen_subnode_name)
{
	int chosen, offset, ret;
	u32 val;

	chosen = fdt_path_offset(blob, "/chosen");
	if (!chosen) {
		chosen = fdt_add_subnode(blob, 0, "chosen");
		if (chosen < 0) {
			error("fdt_add_subnode(/, chosen) failed: %s.\n",
			      fdt_strerror(chosen));
			return chosen;
		}
	}

	offset = fdt_add_subnode(blob, chosen, chosen_subnode_name);
	if (offset < 0) {
		error("fdt_add_subnode(/chosen, %s): %s.\n",
			chosen_subnode_name, fdt_strerror(offset));
		return offset;
	}

	val = cpu_to_fdt32(bi->id);
	ret = fdt_setprop(blob, offset, "id", &val, sizeof(val));
	if (ret < 0) {
		error("could not update id property %s.\n",
		      fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->sku);
	ret = fdt_setprop(blob, offset, "sku", &val, sizeof(val));
	if (ret < 0) {
		error("could not update sku property %s.\n",
		      fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->fab);
	ret = fdt_setprop(blob, offset, "fab", &val, sizeof(val));
	if (ret < 0) {
		error("could not update fab property %s.\n",
		      fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->major);
	ret = fdt_setprop(blob, offset, "major_revision", &val, sizeof(val));
	if (ret < 0) {
		error("could not update major_revision property %s.\n",
		      fdt_strerror(ret));
		return ret;
	}

	val = cpu_to_fdt32(bi->minor);
	ret = fdt_setprop(blob, offset, "minor_revision", &val, sizeof(val));
	if (ret < 0) {
		error("could not update minor_revision property %s.\n",
		      fdt_strerror(ret));
		return ret;
	}

	return 0;
}

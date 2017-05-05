/*
 * Copyright (c) 2010-2017, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <asm/arch-tegra/gpu.h>
#include <asm/arch-tegra/board.h>
#ifdef CONFIG_TEGRA210
#include "tegra210/nvtboot.h"
#endif

/*
 * This function is called right before the kernel is booted. "blob" is the
 * device tree that will be passed to the kernel.
 */
int ft_system_setup(void *blob, bd_t *bd)
{
#if !defined(CONFIG_CPU_BL_IS_CBOOT)
	const char *gpu_compats[] = {
#if defined(CONFIG_TEGRA124)
		"nvidia,gk20a",
#endif
#if defined(CONFIG_TEGRA210)
		"nvidia,gm20b",
#endif
	};
	int i, ret;

	/* Enable GPU node if GPU setup has been performed */
	for (i = 0; i < ARRAY_SIZE(gpu_compats); i++) {
		ret = tegra_gpu_enable_node(blob, gpu_compats[i]);
		if (ret)
			return ret;
	}
#ifdef CONFIG_TEGRA210
	ft_nvtboot(blob);
#endif
#endif	/* !CPU_BL_IS_CBOOT */
	return 0;
}

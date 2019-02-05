/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/mmc.h>
#include <asm/arch-tegra/tegra_mmc.h>

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
	return 0;
}

__weak int tegra_board_init(void)
{
	return 0;
}

int board_init(void)
{
	return tegra_board_init();
}

__weak int cboot_init_late(void)
{
	return 0;
}

int board_late_init(void)
{
	return cboot_init_late();
}

void pad_init_mmc(struct mmc_host *host)
{
}

int board_mmc_init(bd_t *bd)
{
	tegra_mmc_init();

	return 0;
}

/*
 * Attempt to use /chosen/nvidia,ether-mac in the cboot DTB to U-Boot's
 * ethaddr environment variable if possible.
 */
int set_ethaddr_from_cboot(const void *fdt)
{
	int ret, node, len;
	const u32 *prop;

	/* Already a valid address in the environment? If so, keep it */
	if (getenv("ethaddr"))
		return 0;

	node = fdt_path_offset(fdt, "/chosen");
	if (node < 0) {
		printf("Can't find /chosen node in cboot DTB\n");
		return node;
	}
	prop = fdt_getprop(fdt, node, "nvidia,ether-mac", &len);
	if (!prop) {
		printf("Can't find nvidia,ether-mac property in cboot DTB\n");
		return -ENOENT;
	}

	ret = setenv("ethaddr", (void *)prop);
	if (ret) {
		printf("Failed to set ethaddr from cboot DTB: %d\n", ret);
		return ret;
	}

	return 0;
}

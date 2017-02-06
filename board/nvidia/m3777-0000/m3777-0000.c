/*
 * Copyright (c) 2017, NVIDIA CORPORATION
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <i2c.h>

/*
 * Both tegra_board_init() and tegra_pcie_board_init() use the weak
 * empty versions for this board. If init is needed, instantiate real
 * versions here.
 */

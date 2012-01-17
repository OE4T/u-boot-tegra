/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <common.h>
#include <tegra-kbc.h>

#ifndef CONFIG_OF_CONTROL

struct tegra_keyboard_config board_keyboard_config = {
	.plain_keycode = CONFIG_TEGRA2_KBC_PLAIN_KEYCODES,
	.shift_keycode = CONFIG_TEGRA2_KBC_SHIFT_KEYCODES,
	.fn_keycode = CONFIG_TEGRA2_KBC_FUNCTION_KEYCODES,
};

#endif

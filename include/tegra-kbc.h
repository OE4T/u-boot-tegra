/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __include_tegra_kbc_h__
#define __include_tegra_kbc_h__

#include <common.h>

enum KEYS {
	KEY_FIRST_MODIFIER = 220,
	KEY_RIGHT_CTRL = KEY_FIRST_MODIFIER,
	KEY_LEFT_CTRL,
	KEY_FN,
	KEY_SHIFT,
};

#define KEY_IS_MODIFIER(key) ((key) >= KEY_FIRST_MODIFIER)

enum {
	KBC_MAX_ROW	= 16,
	KBC_MAX_COL	= 8,
	KBC_KEY_COUNT	= KBC_MAX_ROW * KBC_MAX_COL,
};

struct tegra_keyboard_config {
	/* keycode tables, one for each row/col position */
	u8 plain_keycode[KBC_KEY_COUNT]; /* when no Shift or Fn */
	u8 shift_keycode[KBC_KEY_COUNT]; /* Shift modifier key is pressed */
	u8 fn_keycode[KBC_KEY_COUNT]; /* Fn modifier key is pressed */
	u8 ctrl_keycode[KBC_KEY_COUNT]; /* Ctrl modifier key is pressed */
};

struct kbc_tegra {
	u32 control;
	u32 interrupt;
	u32 row_cfg[4];
	u32 col_cfg[3];
	u32 timeout_cnt;
	u32 init_dly;
	u32 rpt_dly;
	u32 kp_ent[2];
	u32 row_mask[16];
};

#ifdef CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
extern int overwrite_console(void);
#define OVERWRITE_CONSOLE overwrite_console()
#else
#define OVERWRITE_CONSOLE 0
#endif /* CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE */

extern struct tegra_keyboard_config board_keyboard_config;

#endif /* __include_tegra_kbc_h__ */

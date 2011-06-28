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

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

/* Control Sequence Introducer for arrow keys */
#define CSI_0		0x1B	/* Escape */
#define CSI_1		0x5B	/* '[' */

uint32_t VbExKeyboardRead(void)
{
	int c;

	if (!tstc())
		return 0;

	c = getc();

	/* Special handle for up/down/left/right arrow keys. */
	if (c == CSI_0) {
		if (getc() == CSI_1) {
			c = getc();
			switch (c) {
				case 'A': return VB_KEY_UP;
				case 'B': return VB_KEY_DOWN;
				case 'C': return VB_KEY_RIGHT;
				case 'D': return VB_KEY_LEFT;
			}
		}
		/* Filter out any speical key we don't recognize. */
		return 0;
	}

	return c;
}

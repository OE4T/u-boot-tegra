/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_
#define CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_

#include <vboot_nvstorage.h>

/**
 * These read/write VbNvContext from internal storage devcie. Before calling
 * these functions, caller must have initialized the storage device first.
 *
 * @param nvcxt points to a VbNvContext these functions read from or write to
 * @return 0 if it succeeds; non-zero if it fails
 */
int read_nvcontext(VbNvContext *nvcxt);
int write_nvcontext(VbNvContext *nvcxt);

/* The VbNvContext is stored in mbr of internal storage device */
#define NVCONTEXT_LBA 0

#endif /* CHROMEOS_VBOOT_NVSTORAGE_HELPER_H_ */

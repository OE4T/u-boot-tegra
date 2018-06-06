/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _MON_SECTION_DEFAULT_H_
#define _MON_SECTION_DEFAULT_H_

#ifndef mon_data
#define mon_data
#endif

#ifndef mon_rodata
#define mon_rodata
#endif

#ifndef mon_text
#define mon_text
#endif

#ifndef MON_SYM
#define MON_SYM(x) x
#endif

#ifndef MON_SYM_STR
#define MON_SYM_STR(x) #x
#endif

#ifndef MON_STR
#define MON_STR(x) (x)
#endif

#endif

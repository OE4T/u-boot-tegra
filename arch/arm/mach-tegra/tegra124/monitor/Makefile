#
# (C) Copyright 2018
# NVIDIA Corporation <www.nvidia.com>
#
# SPDX-License-Identifier:     GPL-2.0
#

# Make this first.
# It doesn't really matter, but makes the binary layout a bit more obvious.
obj-y += mon_vecs.o

obj-y += mon_cache.o
obj-y += mon_dbg.o
obj-y += mon_entry.o
obj-y += mon_lib.o
obj-y += mon_psci.o
obj-y += mon_sip.o
obj-y += mon_sleep.o
obj-y += mon_smc.o

obj-y += mon_aes.o
obj-y += mon_crypto.o
obj-y += mon_t1x4_warmboot.o
obj-y += mon_warmboot.o
obj-y += mon_warmboot_avp.o

CFLAGS_mon_warmboot_avp.o = \
    -march=armv4t \
    -U__LINUX_ARM_ARCH__ \
    -D__LINUX_ARM_ARCH__=4

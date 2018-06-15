#
# (C) Copyright 2013-2018
# NVIDIA Corporation <www.nvidia.com>
#
# SPDX-License-Identifier:     GPL-2.0+
#

obj-$(CONFIG_SPL_BUILD) += cpu.o

obj-y	+= clock.o
obj-y	+= funcmux.o
obj-y	+= pinmux.o
obj-y	+= pmc.o
obj-y	+= xusb-padctl.o
obj-y	+= ../xusb-padctl-common.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_ARMV7_NONSEC) += psci.o
endif

# The AVP is ARMv4T architecture so we must use special compiler
# flags for any startup files it might use.
CFLAGS_warmboot_avp.o = -march=armv4t -U__LINUX_ARM_ARCH__ \
        -D__LINUX_ARM_ARCH__=4

obj-$(CONFIG_TEGRA_LP0) += crypto.o
obj-$(CONFIG_TEGRA_LP0) += t1x4_warmboot.o
obj-$(CONFIG_TEGRA_LP0) += warmboot.o
obj-$(CONFIG_TEGRA_LP0) += warmboot_avp.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_TARGET_L4T_MONITOR) += mon_boot.o
obj-$(CONFIG_TARGET_L4T_MONITOR) += monitor/
endif

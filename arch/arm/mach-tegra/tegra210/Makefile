# Copyright (c) 2013-2017, NVIDIA CORPORATION.
#
# SPDX-License-Identifier: GPL-2.0
#

obj-y	+= clock.o
obj-y	+= funcmux.o
ifdef CONFIG_CPU_BL_IS_CBOOT
obj-y  += ../nvtboot_ll.o
obj-y  += ../nvtboot_mem.o
obj-y  += ../nvtboot_board.o
else
obj-y	+= nvtboot_ll.o
obj-y	+= nvtboot.o
endif
obj-y	+= pinmux.o
obj-y	+= xusb-padctl.o
obj-y	+= ../ft_board_info.o
obj-y	+= ../xusb-padctl-common.o

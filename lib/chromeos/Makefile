#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Alternatively, this software may be distributed under the terms of the
# GNU General Public License ("GPL") version 2 as published by the Free
# Software Foundation.
#

include $(TOPDIR)/config.mk

LIB	= $(obj)libchromeos.a

COBJS-$(CONFIG_CHROMEOS) += firmware_storage.o
COBJS-$(CONFIG_CHROMEOS) += fmap.o
COBJS-$(CONFIG_CHROMEOS) += load_firmware_fw.o
COBJS-$(CONFIG_CHROMEOS) += os_storage.o
COBJS-$(CONFIG_CHROMEOS) += tlcl_stub.o
COBJS-$(CONFIG_CHROMEOS) += utility.o
COBJS-$(CONFIG_CHROMEOS_BMPBLK) += gbb_bmpblk.o

COBJS	:= $(COBJS-y)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

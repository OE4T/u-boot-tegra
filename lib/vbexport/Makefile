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

LIB	= $(obj)libvbexport.a

COBJS-$(CONFIG_VBOOT_WRAPPER) += boot_device.o
COBJS-$(CONFIG_VBOOT_WRAPPER) += keyboard.o
COBJS-$(CONFIG_VBOOT_WRAPPER) += load_firmware.o
COBJS-$(CONFIG_VBOOT_WRAPPER) += nvstorage.o
COBJS-$(CONFIG_VBOOT_WRAPPER) += tlcl_stub.o
COBJS-$(CONFIG_VBOOT_WRAPPER) += utility.o

COBJS	:= $(COBJS-y)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

include $(TOPDIR)/config.mk

LIB := $(obj)libtpm.a

IFXDIR = slb9635_i2c

$(shell mkdir -p $(obj)$(IFXDIR))

COBJS-$(CONFIG_INFINEON_TPM_I2C) = slb9635_i2c.o \
	$(IFXDIR)/tpm_proto.o \
	$(IFXDIR)/tpm.o \
	$(IFXDIR)/tpm_tis_i2c.o \
	$(IFXDIR)/tddl.o \
	$(IFXDIR)/ifx_v05.o \
	$(IFXDIR)/ifx_v03.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

# EXTRA_CFLAGS += -I. -I$(TOPDIR)/include
# CFLAGS += $(EXTRA_CFLAGS)
# HOSTCFLAGS += $(EXTRA_CFLAGS)
# CPPFLAGS += $(EXTRA_CFLAGS)

all:	$(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

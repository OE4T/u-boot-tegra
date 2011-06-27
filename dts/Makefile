#
# Copyright (c) 2011 The Chromium OS Authors.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundatio; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)libdts.o

# We use DEV_TREE_SRC as the name of the device tree, if defined
# If not, we use CONFIG_DEFAULT_DEVICE_TREE

ifeq ($(DEV_TREE_SRC),)
$(if $(CONFIG_DEFAULT_DEVICE_TREE),,\
$(error Please define CONFIG_DEFAULT_DEVICE_TREE in your board header \
	file or define DEV_TREE_SRC in your environment))
DEVICE_TREE = $(subst ",,$(CONFIG_DEFAULT_DEVICE_TREE))
else
DEVICE_TREE = $(DEV_TREE_SRC)
endif

all:	$(obj).depend $(LIB)

# Use a constant name for this so we can access it from C code.
# objcopy doesn't seem to allow us to set the symbol name independently of
# the filename.
DT_BIN	:= $(obj)dt.dtb

$(DT_BIN): $(TOPDIR)/board/$(BOARDDIR)/$(DEVICE_TREE).dts
	$(DTC) -R 4 -p 0x1000 -O dtb -o ${DT_BIN} $<

# TODO: get arch properly for objcopy line.
$(obj)dt.o: $(DT_BIN)
	cd $(dir ${DT_BIN}) && \
	$(OBJCOPY) -B armv5te -I binary -O elf32-littlearm \
		$(notdir ${DT_BIN}) $@
	rm $(DT_BIN)

OBJS-$(CONFIG_OF_EMBED)	:= dt.o

COBJS	:= $(OBJS-y)

OBJS	:= $(addprefix $(obj),$(COBJS))

binary:	$(DT_BIN)

$(LIB):	$(OBJS) $(DTB)
	$(call cmd_link_o_target, $(OBJS))

clean:
	rm -f $(OBJS) $(LIB)
	rm -f $(DT_BIN)

distclean:	clean

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

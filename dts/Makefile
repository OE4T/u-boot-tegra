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

# Use a constant name for this so we can access it from C code.
# objcopy doesn't seem to allow us to set the symbol name independently of
# the filename.
DT_BIN	:= dt.dtb

# TODO: get arch properly for objcopy line.
dt.o: $(TOPDIR)/board/$(BOARDDIR)/$(DEVICE_TREE).dts
	$(DTC) -R 4 -p 0x1000 -O dtb -o $(DT_BIN) $<
	$(OBJCOPY) -B armv5te -I binary -O elf32-littlearm $(DT_BIN) dt.o
	rm $(DT_BIN)

OBJS	:= dt.o

OBJS	:= $(addprefix $(obj),$(OBJS))

all:	$(obj).depend $(LIB)

$(LIB):	$(OBJS)
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

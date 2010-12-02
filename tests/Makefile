#
# Copyright 2010, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Alternatively, this software may be distributed under the terms of the
# GNU General Public License ("GPL") version 2 as published by the Free
# Software Foundation.
#

ROOT = /build/tegra2_seaboard
TC = armv7a-cros-linux-gnueabi-
CC = $(TC)gcc
LD = $(TC)ld
CFLAGS = -I$(ROOT)/usr/include/ -I$(ROOT)/include/

TARGET = $(ROOT)/tmp/test_uldivmod
TESTDATA = $(ROOT)/tmp/testdata
RUN_TARGET = qemu-arm $(ROOT)/lib/ld-linux.so.3 --library-path $(ROOT)/lib \
	     $(TARGET)
RUN_TEST = $(RUN_TARGET) < $(TESTDATA) | diff -q - $(TESTDATA)

DRIVE_SRC = drive_uldivmod.c
DRIVE_OBJ = $(subst .c,.o,$(DRIVE_SRC))

ASMSRC_PATH	= ../arch/arm/lib
ASM_SRCS	= _uldivmod.S _udivsi3.S _divsi3.S
OBJS = $(DRIVE_OBJ) $(subst .S,.o,$(ASM_SRCS))

test: $(TARGET)
	[ -f "$(TESTDATA)" ] || ./generate_cases_uldivmod $(TESTDATA)
	($(RUN_TEST)) > /dev/null 2>&1 && echo "TEST PASS" || echo "TEST FAIL"

$(TARGET): $(OBJS)
	$(LD) -o $@ $? $(ROOT)/usr/lib/crt?.o -lc

$(DRIVE_OBJ): $(DRIVE_SRC)
	$(CC) -march=armv7-a $(CFLAGS) -c -o $@ $<

%.o: $(ASMSRC_PATH)/%.S
	$(CC) -march=armv7-a -c -o $@ $<

clean:
	rm -f $(TARGET) $(TESTDATA) $(OBJS)

.PHONY: test clean

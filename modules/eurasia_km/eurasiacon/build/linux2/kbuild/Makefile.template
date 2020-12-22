########################################################################### ###
#@Title         Root kernel makefile
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

# This top-level kbuild makefile builds all the Linux kernel modules in the
# DDK. To run kbuild, this makefile is copied to $(TARGET_OUT)/kbuild/Makefile
# and make is invoked in $(TARGET_OUT)/kbuild.

# This makefile doesn't define any kbuild special variables apart from
# ccflags-y and obj-m. The variables for objects are picked up by including
# the kbuild makefile fragments named in $(INTERNAL_KBUILD_MAKEFILES). The
# list of objects that these fragments make is collected in
# $(INTERNAL_KBUILD_OBJECTS) and $(INTERNAL_EXTRA_KBUILD_OBJECTS). These
# variables are set according to the build's $(KERNEL_COMPONENTS) and
# $(EXTRA_PVRSRVKM_COMPONENTS). To add a new kernel module to the build, edit
# these variables in the per-build Makefile.

include $(OUT)/config_kernel.mk

.SECONDARY:

$(OUT)/target/kbuild/external/%.c: $(EXTRA_KBUILD_SOURCE)/%.c
	@if [ ! -e $(dir $@) ]; then mkdir -p $(dir $@); fi
	@if [ ! -h $@ ]; then ln -sf $< $@; fi

$(OUT)/target/kbuild/%.c: $(TOP)/%.c
	@if [ ! -e $(dir $@) ]; then mkdir -p $(dir $@); fi
	@if [ ! -h $@ ]; then ln -sf $< $@; fi

ccflags-y += -D__linux__ -include $(OUT)/config_kernel.h \
 -I$(OUT)/include \
 -I$(TOP)/include4 \
 -I$(TOP)/services4/include \
 -I$(TOP)/services4/system/$(PVR_SYSTEM) \
 -I$(TOP)/services4/system/include \
 -I$(TOP)/services4/srvkm/bridged \
 -I$(TOP)/services4/srvkm/bridged/sgx \
 -I$(TOP)/services4/srvkm/common \
 -I$(TOP)/services4/srvkm/devices/sgx \
 -I$(TOP)/services4/srvkm/env/linux \
 -I$(TOP)/services4/srvkm/include

ifeq ($(PVR_LOCAL_HWDEFS),)
ccflags-y += -I$(TOP)/services4/srvkm/hwdefs
else
ccflags-y += -I$(TOP)/hwdefs
endif

include $(INTERNAL_KBUILD_MAKEFILES)

$(if $(pvrsrvkm-y),,$(error pvrsrvkm-y was empty, which could mean that srvkm is missing from $$(KERNEL_COMPONENTS)))
pvrsrvkm-y += $(foreach _m,$(INTERNAL_EXTRA_KBUILD_OBJECTS:.o=),$($(_m)-y))

obj-m += $(INTERNAL_KBUILD_OBJECTS)

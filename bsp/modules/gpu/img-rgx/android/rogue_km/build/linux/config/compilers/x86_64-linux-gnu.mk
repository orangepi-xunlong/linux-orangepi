########################################################################### ###
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
# 64-bit x86 compiler
ifneq ($(KERNELDIR),)
 ifneq ($(ARCH),i386)
  ifeq ($(shell grep -q "CONFIG_X86_32=y" $(KERNELDIR)/.config && echo 1 || echo 0),1)
   $(warning ******************************************************)
   $(warning Your kernel appears to be configured for 32-bit x86,)
   $(warning but CROSS_COMPILE (or KERNEL_CROSS_COMPILE) points)
   $(warning to a 64-bit compiler.)
   $(warning If you want a 32-bit build, either set CROSS_COMPILE)
   $(warning to point to a 32-bit compiler, or build with ARCH=i386)
   $(warning to force 32-bit mode with your existing compiler.)
   $(warning ******************************************************)
   $(error Invalid CROSS_COMPILE / kernel architecture combination)
  endif # CONFIG_X86_32
 endif # ARCH=i386
endif # KERNELDIR

ifeq ($(ARCH),i386)
 # This is actually a 32-bit build using a native 64-bit compiler
 INCLUDE_I386-LINUX-GNU := true
else
 TARGET_PRIMARY_ARCH := target_x86_64
 ifeq ($(MULTIARCH),1)
  ifeq ($(CROSS_COMPILE_SECONDARY),)
   # The secondary architecture is being built with a native 64-bit compiler
   INCLUDE_I386-LINUX-GNU := true
  endif
 endif
endif

ifeq ($(INCLUDE_I386-LINUX-GNU),true)
 TARGET_FORCE_32BIT := -m32
 include $(compilers)/i386-linux-gnu.mk
endif

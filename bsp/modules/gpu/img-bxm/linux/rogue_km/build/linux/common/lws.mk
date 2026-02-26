########################################################################### ###
#@File
#@Title         Linux window system config options
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@Description   Linux build system LWS config options.
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
ifeq ($(SUPPORT_KMS),1)

 # Common mandatory config options
 $(eval $(call KernelConfigMake,SUPPORT_BUFFER_SYNC,1))
 $(eval $(call BothConfigC,SUPPORT_BUFFER_SYNC,1))

 # Common tunable config options



 # Xorg specific config options
 ifeq ($(WINDOW_SYSTEM),xorg)
 endif

 ifeq ($(WINDOW_SYSTEM),wayland)
 endif

 ifeq ($(WINDOW_SYSTEM),tizen)
  override SUPPORT_TIZEN_PLATFORM := 1
 endif

 # DRI Support library specific config options.

 # If attempting to import non-display surfaces into the display driver
 # causes issues, set PVRDRI_DBM_BUFFER_FROM_FD to zero. For Tizen,
 # PVRDRI_DBM_BUFFER_FROM_FD_PROTECTED defaults to zero.
 ifeq ($(SUPPORT_TIZEN_PLATFORM),1)
  PVRDRI_DBM_BUFFER_FROM_FD ?= 0
 else
  PVRDRI_DBM_BUFFER_FROM_FD ?= 1
 endif

 # EGL_EXTENSION_CONTENT_PROTECTED specific config options.

 # If it is possible to import protected surfaces into the display driver,
 # and this behaviour is desired, set PVRDRI_DBM_BUFFER_FROM_FD_PROTECTED
 # to one. This option is ignored if PVRDRI_DBM_BUFFER_FROM_FD is zero.
 PVRDRI_DBM_BUFFER_FROM_FD_PROTECTED ?= 0

endif

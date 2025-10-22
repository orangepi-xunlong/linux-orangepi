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

RGX_BVNC ?= 4.46.6.62

include ../config/preconfig.mk
include ../config/window_system.mk

PVR_SYSTEM := vz_example
VMM_TYPE := stub
KERNEL_COMPONENTS := srvkm
EXCLUDED_APIS="camerahal composerhal hwperftools memtrackhal opencl opengl opengles1 opengles3 openglsc2 rogue2d scripts sensorhal servicestools thermalhal unittests vulkan"

PVR_DRM_MODESET_DRIVER_NAME := nulldisp

ifeq ($(SUPPORT_GPUVIRT_VALIDATION),1)
# if validation is enabled, disable real virtualization
RGX_NUM_DRIVERS_SUPPORTED := 1
GPUVIRT_VALIDATION_NUM_OS ?= 8
else
# Setting the maximum number of OSIDs supported
RGX_NUM_DRIVERS_SUPPORTED ?= 8
endif

ifeq ($(RGX_BVNC),33.15.11.3)
# Rogue Anthill cores support a maximum of 2 OSIDs in hardware
ifeq ($(RGX_NUM_DRIVERS_SUPPORTED),8)
		RGX_NUM_DRIVERS_SUPPORTED := 2
endif
endif

# (optional) use static vz memory model, defaulting to dynamic mode otherwise
#RGX_VZ_STATIC_CARVEOUT_FW_HEAPS=1

# (optional) enable AutoVz
#SUPPORT_AUTOVZ=1

# (optional) Some GPUs do not have scratch registers for virtualisation use.
# Disable HW register support and fall back on state variables from shared memory
#SUPPORT_AUTOVZ_HW_REGS := 0

# Should be last
include ../config/core.mk
-include ../common/lws.mk
include ../common/3rdparty.mk

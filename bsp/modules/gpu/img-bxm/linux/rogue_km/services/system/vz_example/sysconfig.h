/*************************************************************************/ /*!
@File           sysconfig.h
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "pvrsrv_device.h"
#include "rgxdevice.h"

#if !defined(__SYSCCONFIG_H__)
#define __SYSCCONFIG_H__

#define SYS_RGX_ACTIVE_POWER_LATENCY_MS 100

#define DEFAULT_CLOCK_RATE			IMG_UINT64_C(600000000)

/* fixed IPA Base of the memory carveout reserved for the GPU Firmware Heaps */
#define FW_CARVEOUT_IPA_BASE		IMG_UINT64_C(0x7E000000)

/* fixed IPA Base of the memory carveout reserved for the Firmware's Page Tables */
#define FW_PT_CARVEOUT_IPA_BASE		IMG_UINT64_C(0x8F000000)

/* mock SoC registers */
#define SOC_REGBANK_BASE			IMG_UINT64_C(0xF0000000)
#define SOC_REGBANK_SIZE			IMG_UINT32_C(0x10000)
#define POW_DOMAIN_ENABLE_REG		IMG_UINT32_C(0xA000)
#define POW_DOMAIN_DISABLE_REG		IMG_UINT32_C(0xA008)
#define POW_DOMAIN_STATUS_REG		IMG_UINT32_C(0xA010)

#define POW_DOMAIN_GPU				IMG_UINT32_C(0x1)

/*****************************************************************************
 * system specific data structures
 *****************************************************************************/

#endif /* __SYSCCONFIG_H__ */

/*************************************************************************/ /*!
@File
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#ifndef TEE_DDK_H
#define TEE_DDK_H

#include "img_types.h"
#include "img_defs.h"

#include "rgxlayer.h"

#define POLL_TIMEOUT_NS (1000000000ULL)

/* Entire GPU register io range (all OSIDs + secure register bank) */
#define FPGA_RGX_REG_SIZE   (0x1000000ULL)

/* GPU Virtual Address base of the Firmware heap */
#define FWHEAP_GPU_VA (0xE1C0000000ULL)

typedef struct _PVRSRV_FW_PARAMS_
{
	const void *pvFirmware;
	IMG_UINT32 ui32FirmwareSize;
	const void *pvSignature;
	IMG_UINT32 ui32SignatureSize;
	PVRSRV_FW_BOOT_PARAMS uFWP;
} PVRSRV_FW_PARAMS;

typedef struct _PVRSRV_TD_POWER_PARAMS_
{
	IMG_DEV_PHYADDR sPCAddr;

	/* MIPS-only fields */
	IMG_DEV_PHYADDR sGPURegAddr;
	IMG_DEV_PHYADDR sBootRemapAddr;
	IMG_DEV_PHYADDR sCodeRemapAddr;
	IMG_DEV_PHYADDR sDataRemapAddr;
} PVRSRV_TD_POWER_PARAMS;

typedef struct _PVRSRV_DEVICE_FEATURE_CONFIG_
{
	IMG_UINT64 ui64ErnsBrns;
	IMG_UINT64 ui64Features;
	IMG_UINT32 ui32B;
	IMG_UINT32 ui32V;
	IMG_UINT32 ui32N;
	IMG_UINT32 ui32C;
	IMG_UINT32 ui32FeaturesValues[RGX_FEATURE_WITH_VALUES_MAX_IDX];
	IMG_UINT32 ui32MAXDMCount;
	IMG_UINT32 ui32MAXPowUnitCount;
#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
	IMG_UINT32 ui32MAXRACCount;
#endif
	IMG_UINT32 ui32SLCSizeInBytes;
	IMG_PCHAR  pszBVNCString;
} PVRSRV_DEVICE_FEATURE_CONFIG;

/* structure passed by the REE's FPGA system layer */
typedef struct _SYS_DATA_
{
	IMG_UINT32 ui32SysDataSize;
	IMG_UINT64 ui64GpuRegisterBase;
	IMG_UINT64 ui64FwHeapCpuBase;
	IMG_UINT64 ui64FwHeapGpuBase;
	IMG_UINT64 ui64FwPrivateHeapSize;
	IMG_UINT64 ui64FwTotalHeapSize;
	IMG_UINT64 ui64FwHeapStride;
	IMG_UINT64 ui64FwPageTableHeapCpuBase;
	IMG_UINT64 ui64FwPageTableHeapGpuBase;
	IMG_UINT64 ui64FwPageTableHeapSize;

	PVRSRV_DEVICE_FEATURE_CONFIG sDevFeatureCfg;

	struct platform_device *pdev;

	struct tc_rogue_platform_data *pdata;

	struct resource *registers;
} SYS_DATA;

PVRSRV_ERROR TEE_LoadFirmware(IMG_HANDLE hSysData, PVRSRV_FW_PARAMS *psTDFWParams);
PVRSRV_ERROR TEE_SetPowerParams(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams);
PVRSRV_ERROR TEE_RGXStart(IMG_HANDLE hSysData);
PVRSRV_ERROR TEE_RGXStop(IMG_HANDLE hSysData);

#endif

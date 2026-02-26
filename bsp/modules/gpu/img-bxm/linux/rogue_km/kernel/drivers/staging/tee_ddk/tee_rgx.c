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

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/sched/clock.h>

#include "rgxfwimageutils.h"
#include "rgxdefs_km.h"
#include "rgx_cr_defs_km.h"
#include "rgxlayer.h"
#include "tee_ddk.h"
#include "tee_rgx.h"
#include "tee_fw_premap.h"

extern void *regbank;
extern PVRSRV_DEVICE_FEATURE_CONFIG *psDevFeatureCfg;

__printf(2, 3)
void RGXCommentLog(const void *hPrivate, const IMG_CHAR *pszString, ...)
{
	IMG_CHAR szBuffer[512];
	va_list argList;

	va_start(argList, pszString);
	vsnprintf(szBuffer, sizeof(szBuffer), pszString, argList);
	va_end(argList);

	pr_notice("TEE_DDK: %s\n", szBuffer);
}

__printf(2, 3)
void RGXErrorLog(const void *hPrivate, const IMG_CHAR *pszString, ...)
{
	IMG_CHAR szBuffer[512];
	va_list argList;

	va_start(argList, pszString);
	vsnprintf(szBuffer, sizeof(szBuffer), pszString, argList);
	va_end(argList);

	pr_err("TEE_DDK ERROR: %s\n", szBuffer);
}

void RGXMemCopy(const void *hPrivate,
                void *pvDst,
                void *pvSrc,
                size_t uiSize)
{
	volatile const char *pcSrc = pvSrc;
	volatile char *pcDst = pvDst;

	while (uiSize)
	{
		*pcDst++ = *pcSrc++;
		uiSize--;
	}
}

void RGXMemSet(const void *hPrivate,
               void *pvDst,
               IMG_UINT8 ui8Value,
               size_t uiSize)
{
	volatile char *pcDst = pvDst;

	while (uiSize)
	{
		*pcDst++ = ui8Value;
		uiSize--;
	}
}

IMG_INT32 RGXDeviceGetFeatureValue(const void *hPrivate, IMG_UINT64 ui64Feature)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);

	if (ui64Feature >= RGX_FEATURE_WITH_VALUES_MAX_IDX)
	{
		pr_err("TEE_DDK ERROR: %s(0x%llX): invalid feature number.\n", __func__, ui64Feature);
		return -1;
	}

	if (psDevFeatureCfg->ui32FeaturesValues[ui64Feature] == RGX_FEATURE_VALUE_DISABLED)
	{
		pr_err("TEE_DDK ERROR: %s(0x%llX): feature disabled.\n", __func__, ui64Feature);
		return -1;
	}

	return psDevFeatureCfg->ui32FeaturesValues[ui64Feature];
}

IMG_BOOL RGXDeviceHasFeature(const void *hPrivate, IMG_UINT64 ui64Feature)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);

	return ((psDevFeatureCfg->ui64Features & ui64Feature) != 0);
}

IMG_BOOL RGXDeviceHasErnBrn(const void *hPrivate, IMG_UINT64 ui64ErnsBrns)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);

	return (psDevFeatureCfg->ui64ErnsBrns & ui64ErnsBrns) != 0;
}

IMG_INTERNAL
IMG_UINT32 RGXGetFWCorememSize(const void *hPrivate)
{
	return RGX_FEATURE_META_COREMEM_SIZE * 1024;
}

void RGXWriteReg32(const void *hPrivate,
                   IMG_UINT32 ui32RegAddr,
                   IMG_UINT32 ui32RegValue)
{
	writel((IMG_UINT32)(ui32RegValue), (IMG_BYTE __iomem *)(regbank) + (ui32RegAddr));
}


void RGXWriteReg64(const void *hPrivate,
                   IMG_UINT32 ui32RegAddr,
                   IMG_UINT64 ui64RegValue)
{
	IMG_UINT64 _off  = ui32RegAddr;
	IMG_UINT64 _val  = ui64RegValue;

	writel((IMG_UINT32)((_val) & 0xffffffff), (IMG_BYTE __iomem *)(regbank) + (_off));
	writel((IMG_UINT32)(((IMG_UINT64)(_val) >> 32) & 0xffffffff), (IMG_BYTE __iomem *)(regbank) + (_off) + 4);
}

IMG_UINT32 RGXReadReg32(const void *hPrivate,
                        IMG_UINT32 ui32RegAddr)
{
	return readl((IMG_BYTE __iomem *)(regbank) + (ui32RegAddr));
}

IMG_UINT64 RGXReadReg64(const void *hPrivate,
                        IMG_UINT32 ui32RegAddr)
{
	IMG_UINT64 _off  = ui32RegAddr;

	return (IMG_UINT64)(((IMG_UINT64)(readl((IMG_BYTE __iomem *)(regbank) + (_off) + 4)) << 32) | readl((IMG_BYTE __iomem *)(regbank) + (_off)));
}

IMG_UINT32 RGXReadModifyWriteReg64(const void *hPrivate,
                                   IMG_UINT32 ui32RegAddr,
                                   IMG_UINT64 ui64RegValue,
                                   IMG_UINT64 ui64RegKeepMask)
{
	IMG_UINT64 uiRegValue = RGXReadReg64(NULL, ui32RegAddr);

	ui64RegValue &= ~ui64RegKeepMask;
	uiRegValue &= ui64RegKeepMask;
	RGXWriteReg64(NULL, ui32RegAddr, uiRegValue | ui64RegValue);

	return 0;
}

PVRSRV_ERROR RGXPollReg32(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT32 ui32RegValue,
                          IMG_UINT32 ui32RegMask)
{
	IMG_UINT32 ui32ActualValue = 0xFFFFFFFFU;
	IMG_UINT64 uiOffset, uiStart, uiCurrent;
	IMG_INT32 iNotLastLoop;

	for (uiOffset = 0, uiStart = sched_clock(), uiCurrent = uiStart + 1, iNotLastLoop = 1;
		((uiCurrent - uiStart + uiOffset) < (POLL_TIMEOUT_NS)) || iNotLastLoop--;
		uiCurrent = sched_clock(),
		uiOffset = uiCurrent < uiStart ? IMG_UINT64_MAX - uiStart : uiOffset,
		uiStart = uiCurrent < uiStart ? 0 : uiStart)
	{
		ui32ActualValue = RGXReadReg32(hPrivate, ui32RegAddr) & ui32RegMask;

		if (ui32ActualValue == ui32RegValue)
		{
			return PVRSRV_OK;
		}

		msleep(10);
	}

	return PVRSRV_ERROR_TIMEOUT;
}

PVRSRV_ERROR RGXPollReg64(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT64 ui64RegValue,
                          IMG_UINT64 ui64RegMask)
{
	IMG_UINT64 ui64ActualValue = 0xFFFFFFFFFFFFFFFFU;
	IMG_UINT64 uiOffset, uiStart, uiCurrent;
	IMG_INT32 iNotLastLoop;

	for (uiOffset = 0, uiStart = sched_clock(), uiCurrent = uiStart + 1, iNotLastLoop = 1;
		((uiCurrent - uiStart + uiOffset) < (POLL_TIMEOUT_NS)) || iNotLastLoop--;
		uiCurrent = sched_clock(),
		uiOffset = uiCurrent < uiStart ? IMG_UINT64_MAX - uiStart : uiOffset,
		uiStart = uiCurrent < uiStart ? 0 : uiStart)
	{
		ui64ActualValue = RGXReadReg64(hPrivate, ui32RegAddr) & ui64RegMask;

		if (ui64ActualValue == ui64RegValue)
		{
			return PVRSRV_OK;
		}

		msleep(10);
	}

	return PVRSRV_ERROR_TIMEOUT;
}

void RGXSetPoweredState(const void *hPrivate, IMG_BOOL bPowered)
{
	/* not needed here */
}

void RGXWaitCycles(const void *hPrivate,
                   IMG_UINT32 ui32Cycles,
                   IMG_UINT32 ui32WaitUs)
{
	udelay(ui32WaitUs);
}

void RGXAcquireKernelMMUPC(const void *hPrivate, IMG_DEV_PHYADDR *psPCAddr)
{
	const SYS_DATA *psSysData = hPrivate;

	psPCAddr->uiAddr = psSysData->ui64FwPageTableHeapGpuBase;
}

IMG_BOOL RGXDoFWSlaveBoot(const void *hPrivate)
{
	return false;
}

PVRSRV_ERROR RGXFabricCoherencyTest(const void *hPrivate)
{
	return PVRSRV_OK;
}

IMG_UINT32 RGXGetDeviceSLCBanks(const void *hPrivate)
{
	return RGX_FEATURE_SLC_BANKS;
}

IMG_UINT32 RGXGetDeviceCacheLineSize(const void *hPrivate)
{
	return RGX_FEATURE_SLC_CACHE_LINE_SIZE_BITS;
}

void RGXAcquireBootCodeAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootCodeAddr)
{
	psBootCodeAddr->uiAddr = RGX_FIRMWARE_RAW_HEAP_BASE;
}

void RGXAcquireBootDataAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootDataAddr)
{
}

void *RGXCalculateHostFWDataAddress(const void *hPrivate, void *pvHostFWDataAddr)
{
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	IMG_UINT8 *ui8HostFWDataAddr = (IMG_UINT8*)pvHostFWDataAddr;
	IMG_UINT32 ui32Offset = 0U;

	if (RGXDeviceGetFeatureValue(hPrivate, RGX_FEATURE_HOST_SECURITY_VERSION_IDX) >= 4)
	{
		ui32Offset =
			PVR_ALIGN(RGXGetFWImageSectionAllocSize(hPrivate, RISCV_UNCACHED_CODE),
			          RGXRISCVFW_REMAP_CONFIG_DEVVADDR_ALIGN) +
			PVR_ALIGN(RGXGetFWImageSectionAllocSize(hPrivate, RISCV_CACHED_CODE),
			          RGXRISCVFW_REMAP_CONFIG_DEVVADDR_ALIGN);
	}

	ui8HostFWDataAddr -= ui32Offset;
	return (void*)ui8HostFWDataAddr;
#else
	PVR_UNREFERENCED_PARAMETER(hPrivate);

	return pvHostFWDataAddr;
#endif
}

IMG_BOOL RGXDeviceAckIrq(const void *hPrivate)
{
	/* Clear all interrupts before shutdown. */
	RGXWriteReg32(NULL,
	              RGX_CR_IRQ_OS0_EVENT_STATUS,
	              RGX_CR_IRQ_OS0_EVENT_CLEAR_SOURCE_CLRMSK);

	return true;
}

IMG_UINT64 RGXMMUInitRangeValue(IMG_UINT32 ui32MMURange)
{
	switch (ui32MMURange)
	{
		case 0:
			return 0x6ffffdc000;
		case 3:
			return 0x3ffff80000;
		default:
			return 0;
	}
}




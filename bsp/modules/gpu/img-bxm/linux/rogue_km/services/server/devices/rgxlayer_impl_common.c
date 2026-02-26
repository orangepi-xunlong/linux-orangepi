/*************************************************************************/ /*!
@File
@Title          Common DDK implementation of the Services abstraction layer
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Common DDK implementation of the Services abstraction layer
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

#include "rgxlayer_impl.h"
#include "pdump_km.h"
#include "rgxfwutils.h"
#include "rgxfwimageutils.h"

#if defined(PDUMP)
#if defined(__linux__)
 #include <linux/version.h>

 #if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
  #include <linux/stdarg.h>
 #else
  #include <stdarg.h>
 #endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) */
#else
 #include <stdarg.h>
#endif /* __linux__ */
#endif

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
#define RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr) \
	(((ui32RegAddr) < RGX_HOST_SECURE_REGBANK_OFFSET) ? \
	 ((psDevInfo)->pvRegsBaseKM) : ((psDevInfo)->pvSecureRegsBaseKM))
#else
#define RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr) ((psDevInfo)->pvRegsBaseKM)
#endif

void RGXMemCopy(const void *hPrivate,
                void *pvDst,
                void *pvSrc,
                size_t uiSize)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	OSDeviceMemCopy(pvDst, pvSrc, uiSize);
}

void RGXMemSet(const void *hPrivate,
               void *pvDst,
               IMG_UINT8 ui8Value,
               size_t uiSize)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	OSDeviceMemSet(pvDst, ui8Value, uiSize);
}

void RGXCommentLog(const void *hPrivate,
                   const IMG_CHAR *pszString,
                   ...)
{
#if defined(PDUMP)
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	va_list argList;
	va_start(argList, pszString);

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	PDumpCommentWithFlagsVA(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS, pszString, argList);
	va_end(argList);
#else
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	PVR_UNREFERENCED_PARAMETER(pszString);
#endif
}

void RGXErrorLog(const void *hPrivate,
                 const IMG_CHAR *pszString,
                 ...)
{
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	va_list argList;

	PVR_UNREFERENCED_PARAMETER(hPrivate);

	va_start(argList, pszString);
	vsnprintf(szBuffer, sizeof(szBuffer), pszString, argList);
	va_end(argList);

	PVR_DPF((PVR_DBG_ERROR, "%s", szBuffer));
}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
IMG_UINT32 RGXGetOSPageSize(const void *hPrivate)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	return OSGetPageSize();
}
#endif

IMG_INT32 RGXDeviceGetFeatureValue(const void *hPrivate, IMG_UINT64 ui64Feature)
{
	IMG_INT32 i32Ret = -1;
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVR_ASSERT(hPrivate != NULL);

	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	psDeviceNode = psDevInfo->psDeviceNode;

	if ((psDeviceNode->pfnGetDeviceFeatureValue))
	{
		i32Ret = psDeviceNode->pfnGetDeviceFeatureValue(psDeviceNode, ui64Feature);
	}

	return i32Ret;
}

IMG_BOOL RGXDeviceHasFeature(const void *hPrivate, IMG_UINT64 ui64Feature)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);

	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	return (psDevInfo->sDevFeatureCfg.ui64Features & ui64Feature) != 0;
}

IMG_UINT32 RGXGetFWCorememSize(const void *hPrivate)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32CorememSize = 0;

	PVR_ASSERT(hPrivate != NULL);

	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META_COREMEM_SIZE))
	{
		ui32CorememSize = RGX_GET_FEATURE_VALUE(psDevInfo, META_COREMEM_SIZE);
	}

	return ui32CorememSize;
}

void RGXWriteReg32(const void *hPrivate, IMG_UINT32 ui32RegAddr, IMG_UINT32 ui32RegValue)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

#if defined(PDUMP)
	if (!(psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW))
#endif
	{
		OSWriteUncheckedHWReg32(pvRegsBase, ui32RegAddr, ui32RegValue);
	}

	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	           ui32RegAddr, ui32RegValue, psParams->ui32PdumpFlags);
}

void RGXWriteReg64(const void *hPrivate, IMG_UINT32 ui32RegAddr, IMG_UINT64 ui64RegValue)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

#if defined(PDUMP)
	if (!(psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW))
#endif
	{
		OSWriteUncheckedHWReg64(pvRegsBase, ui32RegAddr, ui64RegValue);
	}

	PDUMPREG64(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	           ui32RegAddr, ui64RegValue, psParams->ui32PdumpFlags);
}

IMG_UINT32 RGXReadReg32(const void *hPrivate, IMG_UINT32 ui32RegAddr)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;
	IMG_UINT32 ui32RegValue;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

#if defined(PDUMP)
	if (psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW)
	{
		ui32RegValue = IMG_UINT32_MAX;
	}
	else
#endif
	{
		ui32RegValue = OSReadUncheckedHWReg32(pvRegsBase, ui32RegAddr);
	}

	PDUMPREGREAD32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	               ui32RegAddr, psParams->ui32PdumpFlags);

	return ui32RegValue;
}

IMG_UINT64 RGXReadReg64(const void *hPrivate, IMG_UINT32 ui32RegAddr)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;
	IMG_UINT64 ui64RegValue;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

#if defined(PDUMP)
	if (psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW)
	{
		ui64RegValue = IMG_UINT64_MAX;
	}
	else
#endif
	{
		ui64RegValue = OSReadUncheckedHWReg64(pvRegsBase, ui32RegAddr);
	}

	PDUMPREGREAD64(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	               ui32RegAddr, PDUMP_FLAGS_CONTINUOUS);

	return ui64RegValue;
}

IMG_UINT32 RGXReadModifyWriteReg64(const void *hPrivate,
                                   IMG_UINT32 ui32RegAddr,
                                   IMG_UINT64 uiRegValueNew,
                                   IMG_UINT64 uiRegKeepMask)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;
#if defined(PDUMP)
	PDUMP_FLAGS_T ui32PDumpFlags = PDUMP_FLAGS_CONTINUOUS;
#endif

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

	/* only use the new values for bits we update according to the keep mask */
	uiRegValueNew &= ~uiRegKeepMask;

#if defined(PDUMP)

	PDUMP_BLKSTART(ui32PDumpFlags);

	/* Store register offset to temp PDump variable */
	PDumpRegRead64ToInternalVar(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	                            ":SYSMEM:$1", ui32RegAddr, ui32PDumpFlags);

	/* Keep the bits set in the mask */
	PDumpWriteVarANDValueOp(psDevInfo->psDeviceNode, ":SYSMEM:$1",
	                        uiRegKeepMask, ui32PDumpFlags);

	/* OR the new values */
	PDumpWriteVarORValueOp(psDevInfo->psDeviceNode, ":SYSMEM:$1",
	                       uiRegValueNew, ui32PDumpFlags);

	/* Do the actual register write */
	PDumpInternalVarToReg64(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	                        ui32RegAddr, ":SYSMEM:$1", ui32PDumpFlags);

	PDUMP_BLKEND(ui32PDumpFlags);

	if (!(psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW))
#endif

	{
		IMG_UINT64 uiRegValue = OSReadUncheckedHWReg64(pvRegsBase, ui32RegAddr);
		uiRegValue &= uiRegKeepMask;
		OSWriteUncheckedHWReg64(pvRegsBase, ui32RegAddr, uiRegValue | uiRegValueNew);
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXPollReg32(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT32 ui32RegValue,
                          IMG_UINT32 ui32RegMask)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

#if defined(PDUMP)
	if (!(psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW))
#endif
	{
		if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
		                         (IMG_UINT32 __iomem *)((IMG_UINT8 __iomem *)pvRegsBase + ui32RegAddr),
		                         ui32RegValue,
		                         ui32RegMask,
		                         POLL_FLAG_LOG_ERROR,
		                         NULL) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXPollReg32: Poll for Reg (0x%x) failed", ui32RegAddr));
			return PVRSRV_ERROR_TIMEOUT;
		}
	}

	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32RegAddr,
	            ui32RegValue,
	            ui32RegMask,
	            psParams->ui32PdumpFlags,
	            PDUMP_POLL_OPERATOR_EQUAL);

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXPollReg64(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT64 ui64RegValue,
                          IMG_UINT64 ui64RegMask)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;

	/* Split lower and upper words */
	IMG_UINT32 ui32UpperValue = (IMG_UINT32) (ui64RegValue >> 32);
	IMG_UINT32 ui32LowerValue = (IMG_UINT32) (ui64RegValue);
	IMG_UINT32 ui32UpperMask = (IMG_UINT32) (ui64RegMask >> 32);
	IMG_UINT32 ui32LowerMask = (IMG_UINT32) (ui64RegMask);

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32RegAddr);

#if defined(PDUMP)
	if (!(psParams->ui32PdumpFlags & PDUMP_FLAGS_NOHW))
#endif
	{
		if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
		                         (IMG_UINT32 __iomem *)((IMG_UINT8 __iomem *)pvRegsBase + ui32RegAddr + 4),
		                         ui32UpperValue,
		                         ui32UpperMask,
		                         POLL_FLAG_LOG_ERROR,
		                         NULL) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXPollReg64: Poll for upper part of Reg (0x%x) failed", ui32RegAddr));
			return PVRSRV_ERROR_TIMEOUT;
		}

		if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
		                         (IMG_UINT32 __iomem *)((IMG_UINT8 __iomem *)pvRegsBase + ui32RegAddr),
		                         ui32LowerValue,
		                         ui32LowerMask,
		                         POLL_FLAG_LOG_ERROR,
		                         NULL) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXPollReg64: Poll for lower part of Reg (0x%x) failed", ui32RegAddr));
			return PVRSRV_ERROR_TIMEOUT;
		}
	}

	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32RegAddr + 4,
	            ui32UpperValue,
	            ui32UpperMask,
	            psParams->ui32PdumpFlags,
	            PDUMP_POLL_OPERATOR_EQUAL);


	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32RegAddr,
	            ui32LowerValue,
	            ui32LowerMask,
	            psParams->ui32PdumpFlags,
	            PDUMP_POLL_OPERATOR_EQUAL);

	return PVRSRV_OK;
}

void RGXSetPoweredState(const void *hPrivate, IMG_BOOL bPowered)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	psDevInfo->bRGXPowered = bPowered;
}

void RGXWaitCycles(const void *hPrivate, IMG_UINT32 ui32Cycles, IMG_UINT32 ui32TimeUs)
{
	__maybe_unused PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;
	OSWaitus(ui32TimeUs);
	PDUMPIDLWITHFLAGS(psDevInfo->psDeviceNode, ui32Cycles, PDUMP_FLAGS_CONTINUOUS);
}

void RGXAcquireKernelMMUPC(const void *hPrivate, IMG_DEV_PHYADDR *psPCAddr)
{
	PVR_ASSERT(hPrivate != NULL);
	*psPCAddr = ((RGX_LAYER_PARAMS*)hPrivate)->sPCAddr;
}

#if defined(PDUMP)
#if !defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
void RGXWriteKernelMMUPC64(const void *hPrivate,
		IMG_UINT32 ui32PCReg,
		IMG_UINT32 ui32PCRegAlignShift,
		IMG_UINT32 ui32PCRegShift,
		IMG_UINT64 ui64PCVal)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	/* Write the cat-base address */
	OSWriteHWReg64(psDevInfo->pvRegsBaseKM, ui32PCReg, ui64PCVal);

	/* Pdump catbase address */
	MMU_PDumpWritePageCatBase(psDevInfo->psKernelMMUCtx,
			RGX_PDUMPREG_NAME,
			ui32PCReg,
			8,
			ui32PCRegAlignShift,
			ui32PCRegShift,
			PDUMP_FLAGS_CONTINUOUS);
}
#endif

void RGXWriteKernelMMUPC32(const void *hPrivate,
                           IMG_UINT32 ui32PCReg,
                           IMG_UINT32 ui32PCRegAlignShift,
                           IMG_UINT32 ui32PCRegShift,
                           IMG_UINT32 ui32PCVal)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	void __iomem *pvRegsBase;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;
	pvRegsBase = RGX_GET_REGS_BASE(psDevInfo, ui32PCReg);

	/* Write the cat-base address */
	OSWriteUncheckedHWReg32(pvRegsBase, ui32PCReg, ui32PCVal);

	/* Pdump catbase address */
	MMU_PDumpWritePageCatBase(psDevInfo->psKernelMMUCtx,
	                          RGX_PDUMPREG_NAME,
	                          ui32PCReg,
	                          4,
	                          ui32PCRegAlignShift,
	                          ui32PCRegShift,
	                          PDUMP_FLAGS_CONTINUOUS);
}
#endif /* defined(PDUMP) */

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
void RGXAcquireGPURegsAddr(const void *hPrivate, IMG_DEV_PHYADDR *psGPURegsAddr)
{
	PVR_ASSERT(hPrivate != NULL);
	*psGPURegsAddr = ((RGX_LAYER_PARAMS*)hPrivate)->sGPURegAddr;
}

#if defined(PDUMP)
void RGXMIPSWrapperConfig(const void *hPrivate,
		IMG_UINT32 ui32RegAddr,
		IMG_UINT64 ui64GPURegsAddr,
		IMG_UINT32 ui32GPURegsAlign,
		IMG_UINT32 ui32BootMode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	PDUMP_FLAGS_T ui32PDumpFlags = PDUMP_FLAGS_CONTINUOUS;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	OSWriteHWReg64(psDevInfo->pvRegsBaseKM,
			ui32RegAddr,
			(ui64GPURegsAddr >> ui32GPURegsAlign) | ui32BootMode);

	PDUMP_BLKSTART(ui32PDumpFlags);

	/* Store register offset to temp PDump variable */
	PDumpRegLabelToInternalVar(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	                           ui32RegAddr, ":SYSMEM:$1", ui32PDumpFlags);

	/* Align register transactions identifier */
	PDumpWriteVarSHRValueOp(psDevInfo->psDeviceNode, ":SYSMEM:$1",
	                        ui32GPURegsAlign, ui32PDumpFlags);

	/* Enable micromips instruction encoding */
	PDumpWriteVarORValueOp(psDevInfo->psDeviceNode, ":SYSMEM:$1",
	                       ui32BootMode, ui32PDumpFlags);

	/* Do the actual register write */
	PDumpInternalVarToReg64(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME,
	                        ui32RegAddr, ":SYSMEM:$1", ui32PDumpFlags);

	PDUMP_BLKEND(ui32PDumpFlags);
}
#endif

void RGXAcquireBootRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psBootRemapAddr)
{
	PVR_ASSERT(hPrivate != NULL);
	*psBootRemapAddr = ((RGX_LAYER_PARAMS*)hPrivate)->sBootRemapAddr;
}

void RGXAcquireCodeRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psCodeRemapAddr)
{
	PVR_ASSERT(hPrivate != NULL);
	*psCodeRemapAddr = ((RGX_LAYER_PARAMS*)hPrivate)->sCodeRemapAddr;
}

void RGXAcquireDataRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psDataRemapAddr)
{
	PVR_ASSERT(hPrivate != NULL);
	*psDataRemapAddr = ((RGX_LAYER_PARAMS*)hPrivate)->sDataRemapAddr;
}

void RGXAcquireTrampolineRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psTrampolineRemapAddr)
{
	PVR_ASSERT(hPrivate != NULL);
	*psTrampolineRemapAddr = ((RGX_LAYER_PARAMS*)hPrivate)->sTrampolineRemapAddr;
}

#if defined(PDUMP)
static inline
void RGXWriteRemapConfig2Reg(void __iomem *pvRegs,
		PMR *psPMR,
		IMG_DEVMEM_OFFSET_T uiLogicalOffset,
		IMG_UINT32 ui32RegAddr,
		IMG_UINT64 ui64PhyAddr,
		IMG_UINT64 ui64PhyMask,
		IMG_UINT64 ui64Settings)
{
	PDUMP_FLAGS_T ui32PDumpFlags = PDUMP_FLAGS_CONTINUOUS;
	PVRSRV_DEVICE_NODE *psDevNode;

	PVR_ASSERT(psPMR != NULL);
	psDevNode = PMR_DeviceNode(psPMR);

	OSWriteHWReg64(pvRegs, ui32RegAddr, (ui64PhyAddr & ui64PhyMask) | ui64Settings);

	PDUMP_BLKSTART(ui32PDumpFlags);

	/* Store memory offset to temp PDump variable */
	PDumpMemLabelToInternalVar64(":SYSMEM:$1", psPMR,
	                             uiLogicalOffset, ui32PDumpFlags);

	/* Keep only the relevant bits of the output physical address */
	PDumpWriteVarANDValueOp(psDevNode, ":SYSMEM:$1", ui64PhyMask, ui32PDumpFlags);

	/* Extra settings for this remapped region */
	PDumpWriteVarORValueOp(psDevNode, ":SYSMEM:$1", ui64Settings, ui32PDumpFlags);

	/* Do the actual register write */
	PDumpInternalVarToReg64(psDevNode, RGX_PDUMPREG_NAME, ui32RegAddr,
	                        ":SYSMEM:$1", ui32PDumpFlags);

	PDUMP_BLKEND(ui32PDumpFlags);
}

void RGXBootRemapConfig(const void *hPrivate,
		IMG_UINT32 ui32Config1RegAddr,
		IMG_UINT64 ui64Config1RegValue,
		IMG_UINT32 ui32Config2RegAddr,
		IMG_UINT64 ui64Config2PhyAddr,
		IMG_UINT64 ui64Config2PhyMask,
		IMG_UINT64 ui64Config2Settings)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32BootRemapMemOffset = RGXGetFWImageSectionOffset(NULL, MIPS_BOOT_CODE);

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	/* Write remap config1 register */
	RGXWriteReg64(hPrivate,
			ui32Config1RegAddr,
			ui64Config1RegValue);

	/* Write remap config2 register */
	RGXWriteRemapConfig2Reg(psDevInfo->pvRegsBaseKM,
			psDevInfo->psRGXFWCodeMemDesc->psImport->hPMR,
			psDevInfo->psRGXFWCodeMemDesc->uiOffset + ui32BootRemapMemOffset,
			ui32Config2RegAddr,
			ui64Config2PhyAddr,
			ui64Config2PhyMask,
			ui64Config2Settings);
}

void RGXCodeRemapConfig(const void *hPrivate,
		IMG_UINT32 ui32Config1RegAddr,
		IMG_UINT64 ui64Config1RegValue,
		IMG_UINT32 ui32Config2RegAddr,
		IMG_UINT64 ui64Config2PhyAddr,
		IMG_UINT64 ui64Config2PhyMask,
		IMG_UINT64 ui64Config2Settings)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32CodeRemapMemOffset = RGXGetFWImageSectionOffset(NULL, MIPS_EXCEPTIONS_CODE);

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	/* Write remap config1 register */
	RGXWriteReg64(hPrivate,
			ui32Config1RegAddr,
			ui64Config1RegValue);

	/* Write remap config2 register */
	RGXWriteRemapConfig2Reg(psDevInfo->pvRegsBaseKM,
			psDevInfo->psRGXFWCodeMemDesc->psImport->hPMR,
			psDevInfo->psRGXFWCodeMemDesc->uiOffset + ui32CodeRemapMemOffset,
			ui32Config2RegAddr,
			ui64Config2PhyAddr,
			ui64Config2PhyMask,
			ui64Config2Settings);
}

void RGXDataRemapConfig(const void *hPrivate,
		IMG_UINT32 ui32Config1RegAddr,
		IMG_UINT64 ui64Config1RegValue,
		IMG_UINT32 ui32Config2RegAddr,
		IMG_UINT64 ui64Config2PhyAddr,
		IMG_UINT64 ui64Config2PhyMask,
		IMG_UINT64 ui64Config2Settings)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32DataRemapMemOffset = RGXGetFWImageSectionOffset(NULL, MIPS_BOOT_DATA);

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	/* Write remap config1 register */
	RGXWriteReg64(hPrivate,
			ui32Config1RegAddr,
			ui64Config1RegValue);

	/* Write remap config2 register */
	RGXWriteRemapConfig2Reg(psDevInfo->pvRegsBaseKM,
			psDevInfo->psRGXFWDataMemDesc->psImport->hPMR,
			psDevInfo->psRGXFWDataMemDesc->uiOffset + ui32DataRemapMemOffset,
			ui32Config2RegAddr,
			ui64Config2PhyAddr,
			ui64Config2PhyMask,
			ui64Config2Settings);
}

void RGXTrampolineRemapConfig(const void *hPrivate,
		IMG_UINT32 ui32Config1RegAddr,
		IMG_UINT64 ui64Config1RegValue,
		IMG_UINT32 ui32Config2RegAddr,
		IMG_UINT64 ui64Config2PhyAddr,
		IMG_UINT64 ui64Config2PhyMask,
		IMG_UINT64 ui64Config2Settings)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	PDUMP_FLAGS_T ui32PDumpFlags = PDUMP_FLAGS_CONTINUOUS;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	/* write the register for real, without PDump */
	OSWriteHWReg64(psDevInfo->pvRegsBaseKM,
			ui32Config1RegAddr,
			ui64Config1RegValue);

	PDUMP_BLKSTART(ui32PDumpFlags);

	/* Store the memory address in a PDump variable */
	PDumpPhysHandleToInternalVar64(psDevInfo->psDeviceNode, ":SYSMEM:$1",
			psDevInfo->psTrampoline->hPdumpPages,
			ui32PDumpFlags);

	/* Keep only the relevant bits of the input physical address */
	PDumpWriteVarANDValueOp(psDevInfo->psDeviceNode, ":SYSMEM:$1",
			~RGX_CR_MIPS_ADDR_REMAP4_CONFIG1_BASE_ADDR_IN_CLRMSK,
			ui32PDumpFlags);

	/* Enable bit */
	PDumpWriteVarORValueOp(psDevInfo->psDeviceNode, ":SYSMEM:$1",
			RGX_CR_MIPS_ADDR_REMAP4_CONFIG1_MODE_ENABLE_EN,
			ui32PDumpFlags);

	/* Do the PDump register write */
	PDumpInternalVarToReg64(psDevInfo->psDeviceNode,
			RGX_PDUMPREG_NAME,
			ui32Config1RegAddr,
			":SYSMEM:$1",
			ui32PDumpFlags);

	PDUMP_BLKEND(ui32PDumpFlags);

	/* this can be written directly */
	RGXWriteReg64(hPrivate,
			ui32Config2RegAddr,
			(ui64Config2PhyAddr & ui64Config2PhyMask) | ui64Config2Settings);
}
#endif /* defined(PDUMP) */
#endif /* defined(RGX_FEATURE_MIPS_BIT_MASK) */

IMG_BOOL RGXDeviceHasErnBrn(const void *hPrivate, IMG_UINT64 ui64ErnsBrns)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	return (psDevInfo->sDevFeatureCfg.ui64ErnsBrns & ui64ErnsBrns) != 0;
}

IMG_UINT32 RGXGetDeviceSLCBanks(const void *hPrivate)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	if (!RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, SLC_BANKS))
	{
		return 0;
	}
	return RGX_GET_FEATURE_VALUE(psDevInfo, SLC_BANKS);
}

IMG_UINT32 RGXGetDeviceCacheLineSize(const void *hPrivate)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	if (!RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, SLC_CACHE_LINE_SIZE_BITS))
	{
		return 0;
	}
	return RGX_GET_FEATURE_VALUE(psDevInfo, SLC_CACHE_LINE_SIZE_BITS);
}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
IMG_UINT32 RGXGetDevicePhysBusWidth(const void *hPrivate)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	if (!RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, PHYS_BUS_WIDTH))
	{
		return 0;
	}
	return RGX_GET_FEATURE_VALUE(psDevInfo, PHYS_BUS_WIDTH);
}

IMG_BOOL RGXDevicePA0IsValid(const void *hPrivate)
{
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	return psDevInfo->sLayerParams.bDevicePA0IsValid;
}
#endif

void RGXAcquireBootCodeAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootCodeAddr)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	*psBootCodeAddr = psDevInfo->sFWCodeDevVAddrBase;
}

void RGXAcquireBootDataAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootDataAddr)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	*psBootDataAddr = psDevInfo->sFWDataDevVAddrBase;
}

void *RGXCalculateHostFWDataAddress(const void *hPrivate, void *pvHostFWDataAddr)
{
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT8 *ui8HostFWDataAddr = (IMG_UINT8*)pvHostFWDataAddr;
	IMG_UINT32 ui32Offset = 0U;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	if (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) >= 4)
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
	RGX_LAYER_PARAMS *psParams;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psParams = (RGX_LAYER_PARAMS*)hPrivate;
	psDevInfo = psParams->psDevInfo;

	return (psDevInfo->pfnRGXAckIrq != NULL) ?
			psDevInfo->pfnRGXAckIrq(psDevInfo) : IMG_TRUE;
}

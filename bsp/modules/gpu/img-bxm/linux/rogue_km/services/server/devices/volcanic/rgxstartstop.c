/*************************************************************************/ /*!
@File
@Title          Device specific start/stop routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific start/stop routines
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

/* The routines implemented here are built on top of an abstraction layer to
 * hide DDK/OS-specific details in case they are used outside of the DDK
 * (e.g. when trusted device is enabled).
 * Any new dependency should be added to rgxlayer.h.
 * Any new code should be built on top of the existing abstraction layer,
 * which should be extended when necessary. */
#include "rgxstartstop.h"



/*
	RGXWriteMetaRegThroughSP
*/
PVRSRV_ERROR RGXWriteMetaRegThroughSP(const void *hPrivate,
                                      IMG_UINT32 ui32RegAddr,
                                      IMG_UINT32 ui32RegValue)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32StateReg, ui32StateReadyFlag;
	IMG_UINT32 ui32CtrlReg, ui32DataReg;

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, META_REGISTER_UNPACKED_ACCESSES))
	{
		/* ensure the meta_registers_unpacked_accesses auto-increment feature is not used */
		BITMASK_UNSET(ui32RegAddr, RGX_CR_META_SP_MSLVCTRL0_AUTOINCR_EN);

		if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) > 1)
		{
			ui32StateReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA;
			ui32StateReadyFlag = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN |
								 RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32CtrlReg = RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_GT1_AND_MRUA;
			ui32DataReg = RGX_CR_META_SP_MSLVDATAT__HOST_SECURITY_GT1_AND_MRUA;
		}
		else
		{
			ui32StateReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA;
			ui32StateReadyFlag = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN |
								 RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32CtrlReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA;
			ui32DataReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA;
		}
	}
	else
#endif
	{
		ui32StateReg = RGX_CR_META_SP_MSLVCTRL1;
		ui32StateReadyFlag = RGX_CR_META_SP_MSLVCTRL1_READY_EN |
							 RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN;
		ui32CtrlReg = RGX_CR_META_SP_MSLVCTRL0;
		ui32DataReg = RGX_CR_META_SP_MSLVDATAT;
	}

	eError = RGXPollReg32(hPrivate, ui32StateReg, ui32StateReadyFlag, ui32StateReadyFlag);

	if (eError == PVRSRV_OK)
	{
		/* Issue a Write */
		RGXWriteReg32(hPrivate, ui32CtrlReg, ui32RegAddr);
		(void) RGXReadReg32(hPrivate, ui32CtrlReg); /* Fence write */
		RGXWriteReg32(hPrivate, ui32DataReg, ui32RegValue);
		(void) RGXReadReg32(hPrivate, ui32DataReg); /* Fence write */
	}

	return eError;
}

/*
	RGXReadMetaRegThroughSP
*/
PVRSRV_ERROR RGXReadMetaRegThroughSP(const void *hPrivate,
                                     IMG_UINT32 ui32RegAddr,
                                     IMG_UINT32* ui32RegValue)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32StateReg, ui32StateReadyFlag;
	IMG_UINT32 ui32CtrlReg, ui32DataReg;

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, META_REGISTER_UNPACKED_ACCESSES))
	{
		/* ensure the meta_registers_unpacked_accesses auto-increment feature is not used */
		BITMASK_UNSET(ui32RegAddr, RGX_CR_META_SP_MSLVCTRL0_AUTOINCR_EN);

		if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) > 1)
		{
			ui32StateReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA;
			ui32StateReadyFlag = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN |
								 RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32CtrlReg = RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_GT1_AND_MRUA;
			ui32DataReg = RGX_CR_META_SP_MSLVDATAX__HOST_SECURITY_GT1_AND_MRUA;
			BITMASK_SET(ui32RegAddr, RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_GT1_AND_MRUA__RD_EN);
		}
		else
		{
			ui32StateReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA;
			ui32StateReadyFlag = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN |
								 RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32CtrlReg = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA;
			ui32DataReg = RGX_CR_META_SP_MSLVDATAX__HOST_SECURITY_EQ1_AND_MRUA;
			BITMASK_SET(ui32RegAddr, RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_EQ1_AND_MRUA__RD_EN);
		}
	}
	else
#endif
	{
		ui32StateReg = RGX_CR_META_SP_MSLVCTRL1;
		ui32StateReadyFlag = RGX_CR_META_SP_MSLVCTRL1_READY_EN |
							 RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN;
		ui32CtrlReg = RGX_CR_META_SP_MSLVCTRL0;
		ui32DataReg = RGX_CR_META_SP_MSLVDATAX;
		BITMASK_SET(ui32RegAddr, RGX_CR_META_SP_MSLVCTRL0_RD_EN);
	}

	/* Wait for Slave Port to be Ready */
	eError = RGXPollReg32(hPrivate,
						  ui32StateReg,
						  ui32StateReadyFlag,
						  ui32StateReadyFlag);
	if (eError == PVRSRV_OK)
	{
		/* Issue a Read */
		RGXWriteReg32(hPrivate, ui32CtrlReg, ui32RegAddr);
		(void) RGXReadReg32(hPrivate, ui32CtrlReg); /* Fence write */

		/* Wait for Slave Port to be Ready */
		eError = RGXPollReg32(hPrivate,
				  ui32StateReg,
				  ui32StateReadyFlag,
				  ui32StateReadyFlag);
		if (eError != PVRSRV_OK) return eError;
	}

#if !defined(NO_HARDWARE)
	*ui32RegValue = RGXReadReg32(hPrivate, ui32DataReg);
#else
	PVR_UNREFERENCED_PARAMETER(ui32DataReg);
	*ui32RegValue = 0xFFFFFFFF;
#endif

	return eError;
}

/*
 * Specific fields for RGX_CR_IDLE must not be polled in pdumps
 * (technical reasons)
 */
#define CR_IDLE_UNSELECTED_MASK ((~RGX_CR_SLC_IDLE_ACE_CONVERTERS_CLRMSK) | \
								 (~RGX_CR_SLC_IDLE_OWDB_CLRMSK) |		\
								 (RGX_CR_SLC_IDLE_FBCDC_ARB_EN))

static PVRSRV_ERROR RGXWriteMetaCoreRegThoughSP(const void *hPrivate,
                                                IMG_UINT32 ui32CoreReg,
                                                IMG_UINT32 ui32Value)
{
	IMG_UINT32 i = 0;

	RGXWriteMetaRegThroughSP(hPrivate, META_CR_TXUXXRXDT_OFFSET, ui32Value);
	RGXWriteMetaRegThroughSP(hPrivate, META_CR_TXUXXRXRQ_OFFSET, ui32CoreReg & ~META_CR_TXUXXRXRQ_RDnWR_BIT);

	do
	{
		RGXReadMetaRegThroughSP(hPrivate, META_CR_TXUXXRXRQ_OFFSET, &ui32Value);
	} while (((ui32Value & META_CR_TXUXXRXRQ_DREADY_BIT) != META_CR_TXUXXRXRQ_DREADY_BIT) && (i++ < 1000));

	if (i == 1000)
	{
		RGXCommentLog(hPrivate, "RGXWriteMetaCoreRegThoughSP: Timeout");
		return PVRSRV_ERROR_TIMEOUT;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR RGXStartFirmware(const void *hPrivate)
{
	PVRSRV_ERROR eError;

	/* Give privilege to debug and slave port */
	RGXWriteMetaRegThroughSP(hPrivate, META_CR_SYSC_JTAG_THREAD, META_CR_SYSC_JTAG_THREAD_PRIV_EN);

	/* Point Meta to the bootloader address, global (uncached) range */
	eError = RGXWriteMetaCoreRegThoughSP(hPrivate,
	                                     PC_ACCESS(0),
	                                     RGXFW_BOOTLDR_META_ADDR | META_MEM_GLOBAL_RANGE_BIT);

	if (eError != PVRSRV_OK)
	{
		RGXCommentLog(hPrivate, "RGXStart: RGX Firmware Slave boot Start failed!");
		return eError;
	}

	/* Enable minim encoding */
	RGXWriteMetaRegThroughSP(hPrivate, META_CR_TXPRIVEXT, META_CR_TXPRIVEXT_MINIM_EN);

	/* Enable Meta thread */
	RGXWriteMetaRegThroughSP(hPrivate, META_CR_T0ENABLE_OFFSET, META_CR_TXENABLE_ENABLE_BIT);

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function      RGXInitMetaProcWrapper

 @Description   Configures the hardware wrapper of the META processor

 @Input         hPrivate  : Implementation specific data

 @Return        void

******************************************************************************/
static void RGXInitMetaProcWrapper(const void *hPrivate)
{
	IMG_UINT64 ui64GartenConfig;

	/* Garten IDLE bit controlled by META */
	ui64GartenConfig = RGX_CR_MTS_GARTEN_WRAPPER_CONFIG_IDLE_CTRL_META;

#if defined(RGX_CR_MTS_GARTEN_WRAPPER_CONFIG__S7_INFRA__FENCE_PC_BASE_SHIFT)
	/* Set the Garten Wrapper BIF Fence address */
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, S7_TOP_INFRASTRUCTURE))
	{
		/* Set PC = 0 for fences */
		ui64GartenConfig &= RGX_CR_MTS_GARTEN_WRAPPER_CONFIG__S7_INFRA__FENCE_PC_BASE_CLRMSK;
		ui64GartenConfig |= (IMG_UINT64)MMU_CONTEXT_MAPPING_FWPRIV
		                    << RGX_CR_MTS_GARTEN_WRAPPER_CONFIG__S7_INFRA__FENCE_PC_BASE_SHIFT;

	}
	else
	{
		/* Set PC = 0 for fences */
		ui64GartenConfig &= RGX_CR_MTS_GARTEN_WRAPPER_CONFIG_FENCE_PC_BASE_CLRMSK;
		ui64GartenConfig |= (IMG_UINT64)MMU_CONTEXT_MAPPING_FWPRIV
		                    << RGX_CR_MTS_GARTEN_WRAPPER_CONFIG_FENCE_PC_BASE_SHIFT;

		/* Set SLC DM=META */
		ui64GartenConfig |= ((IMG_UINT64) RGXFW_SEGMMU_META_BIFDM_ID) << RGX_CR_MTS_GARTEN_WRAPPER_CONFIG_FENCE_DM_SHIFT;
	}
#endif

	RGXCommentLog(hPrivate, "RGXStart: Configure META wrapper");
	RGXWriteReg64(hPrivate, RGX_CR_MTS_GARTEN_WRAPPER_CONFIG, ui64GartenConfig);
}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
/*!
*******************************************************************************

 @Function      RGXInitMipsProcWrapper

 @Description   Configures the hardware wrapper of the MIPS processor

 @Input         hPrivate  : Implementation specific data

 @Return        void

******************************************************************************/
static void RGXInitMipsProcWrapper(const void *hPrivate)
{
	IMG_DEV_PHYADDR sPhyAddr;
	IMG_UINT64 ui64RemapSettings = RGXMIPSFW_BOOT_REMAP_LOG2_SEGMENT_SIZE; /* Same for all remap registers */

	RGXCommentLog(hPrivate, "RGXStart: Configure MIPS wrapper");

	/*
	 * MIPS wrapper (registers transaction ID and ISA mode) setup
	 */

	RGXCommentLog(hPrivate, "RGXStart: Write wrapper config register");

	if (RGXGetDevicePhysBusWidth(hPrivate) > 32)
	{
		RGXWriteReg32(hPrivate,
		              RGX_CR_MIPS_WRAPPER_CONFIG,
		              (RGXMIPSFW_REGISTERS_VIRTUAL_BASE >>
		              RGXMIPSFW_WRAPPER_CONFIG_REGBANK_ADDR_ALIGN) |
		              RGX_CR_MIPS_WRAPPER_CONFIG_BOOT_ISA_MODE_MICROMIPS);
	}
	else
	{
		RGXAcquireGPURegsAddr(hPrivate, &sPhyAddr);

		RGXMIPSWrapperConfig(hPrivate,
		                     RGX_CR_MIPS_WRAPPER_CONFIG,
		                     sPhyAddr.uiAddr,
		                     RGXMIPSFW_WRAPPER_CONFIG_REGBANK_ADDR_ALIGN,
		                     RGX_CR_MIPS_WRAPPER_CONFIG_BOOT_ISA_MODE_MICROMIPS);
	}

	/*
	 * Boot remap setup
	 */

	RGXAcquireBootRemapAddr(hPrivate, &sPhyAddr);

#if defined(SUPPORT_TRUSTED_DEVICE)
	/* Do not mark accesses to a FW code remap region as DRM accesses */
	ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_TRUSTED_CLRMSK;
#endif

	ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_CLRMSK;
#if defined(MIPS_FW_CODE_OSID)
	ui64RemapSettings |= ((IMG_UINT64) MIPS_FW_CODE_OSID) << RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_SHIFT;
#else
	ui64RemapSettings |= ((IMG_UINT64) FW_OSID) << RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_SHIFT;
#endif

	RGXCommentLog(hPrivate, "RGXStart: Write boot remap registers");
	RGXBootRemapConfig(hPrivate,
	                   RGX_CR_MIPS_ADDR_REMAP1_CONFIG1,
	                   RGXMIPSFW_BOOT_REMAP_PHYS_ADDR_IN | RGX_CR_MIPS_ADDR_REMAP1_CONFIG1_MODE_ENABLE_EN,
	                   RGX_CR_MIPS_ADDR_REMAP1_CONFIG2,
	                   sPhyAddr.uiAddr,
	                   ~RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_ADDR_OUT_CLRMSK,
	                   ui64RemapSettings);

#if defined(FIX_HW_BRN_63553_BIT_MASK)
	if (RGX_DEVICE_HAS_BRN(hPrivate, 63553))
	{
		IMG_BOOL bPhysBusAbove32Bit = RGXGetDevicePhysBusWidth(hPrivate) > 32;
		IMG_BOOL bDevicePA0IsValid  = RGXDevicePA0IsValid(hPrivate);

		/* WA always required on 36 bit cores, to avoid continuous unmapped memory accesses to address 0x0 */
		if (bPhysBusAbove32Bit || !bDevicePA0IsValid)
		{
			RGXCodeRemapConfig(hPrivate,
					RGX_CR_MIPS_ADDR_REMAP5_CONFIG1,
					0x0 | RGX_CR_MIPS_ADDR_REMAP5_CONFIG1_MODE_ENABLE_EN,
					RGX_CR_MIPS_ADDR_REMAP5_CONFIG2,
					sPhyAddr.uiAddr,
					~RGX_CR_MIPS_ADDR_REMAP5_CONFIG2_ADDR_OUT_CLRMSK,
					ui64RemapSettings);
		}
	}
#endif

	/*
	 * Data remap setup
	 */

	RGXAcquireDataRemapAddr(hPrivate, &sPhyAddr);

#if defined(SUPPORT_TRUSTED_DEVICE)
	if (RGXGetDevicePhysBusWidth(hPrivate) > 32)
	{
		/* Remapped private data in secure memory */
		ui64RemapSettings |= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_TRUSTED_EN;
	}
	else
	{
		/* Remapped data in non-secure memory */
		ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_TRUSTED_CLRMSK;
	}
#endif

	ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_CLRMSK;
	ui64RemapSettings |= ((IMG_UINT64) FW_OSID) << RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_SHIFT;

	RGXCommentLog(hPrivate, "RGXStart: Write data remap registers");
	RGXDataRemapConfig(hPrivate,
	                   RGX_CR_MIPS_ADDR_REMAP2_CONFIG1,
	                   RGXMIPSFW_DATA_REMAP_PHYS_ADDR_IN | RGX_CR_MIPS_ADDR_REMAP2_CONFIG1_MODE_ENABLE_EN,
	                   RGX_CR_MIPS_ADDR_REMAP2_CONFIG2,
	                   sPhyAddr.uiAddr,
	                   ~RGX_CR_MIPS_ADDR_REMAP2_CONFIG2_ADDR_OUT_CLRMSK,
	                   ui64RemapSettings);

	/*
	 * Code remap setup
	 */

	RGXAcquireCodeRemapAddr(hPrivate, &sPhyAddr);

#if defined(SUPPORT_TRUSTED_DEVICE)
	/* Do not mark accesses to a FW code remap region as DRM accesses */
	ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_TRUSTED_CLRMSK;
#endif

	ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_CLRMSK;
#if defined(MIPS_FW_CODE_OSID)
	ui64RemapSettings |= ((IMG_UINT64) MIPS_FW_CODE_OSID) << RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_SHIFT;
#else
	ui64RemapSettings |= ((IMG_UINT64) FW_OSID) << RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_SHIFT;
#endif

	RGXCommentLog(hPrivate, "RGXStart: Write exceptions remap registers");
	RGXCodeRemapConfig(hPrivate,
	                   RGX_CR_MIPS_ADDR_REMAP3_CONFIG1,
	                   RGXMIPSFW_CODE_REMAP_PHYS_ADDR_IN | RGX_CR_MIPS_ADDR_REMAP3_CONFIG1_MODE_ENABLE_EN,
	                   RGX_CR_MIPS_ADDR_REMAP3_CONFIG2,
	                   sPhyAddr.uiAddr,
	                   ~RGX_CR_MIPS_ADDR_REMAP3_CONFIG2_ADDR_OUT_CLRMSK,
	                   ui64RemapSettings);

	if (RGXGetDevicePhysBusWidth(hPrivate) == 32)
	{
		/*
		 * Trampoline remap setup
		 */

		RGXAcquireTrampolineRemapAddr(hPrivate, &sPhyAddr);
		ui64RemapSettings = RGXMIPSFW_TRAMPOLINE_LOG2_SEGMENT_SIZE;

#if defined(SUPPORT_TRUSTED_DEVICE)
		/* Remapped data in non-secure memory */
		ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_TRUSTED_CLRMSK;
#endif

		ui64RemapSettings &= RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_CLRMSK;
		ui64RemapSettings |= ((IMG_UINT64) FW_OSID) << RGX_CR_MIPS_ADDR_REMAP1_CONFIG2_OS_ID_SHIFT;

		RGXCommentLog(hPrivate, "RGXStart: Write trampoline remap registers");
		RGXTrampolineRemapConfig(hPrivate,
		                         RGX_CR_MIPS_ADDR_REMAP4_CONFIG1,
		                         sPhyAddr.uiAddr | RGX_CR_MIPS_ADDR_REMAP4_CONFIG1_MODE_ENABLE_EN,
		                         RGX_CR_MIPS_ADDR_REMAP4_CONFIG2,
		                         RGXMIPSFW_TRAMPOLINE_TARGET_PHYS_ADDR,
		                         ~RGX_CR_MIPS_ADDR_REMAP4_CONFIG2_ADDR_OUT_CLRMSK,
		                         ui64RemapSettings);
	}

	/* Garten IDLE bit controlled by MIPS */
	RGXCommentLog(hPrivate, "RGXStart: Set GARTEN_IDLE type to MIPS");
	RGXWriteReg64(hPrivate, RGX_CR_MTS_GARTEN_WRAPPER_CONFIG, RGX_CR_MTS_GARTEN_WRAPPER_CONFIG_IDLE_CTRL_META);

	/* Turn on the EJTAG probe (only useful driver live) */
	RGXWriteReg32(hPrivate, RGX_CR_MIPS_DEBUG_CONFIG, 0);
}
#endif


/*!
*******************************************************************************

 @Function      RGXInitRiscvProcWrapper

 @Description   Configures the hardware wrapper of the RISCV processor

 @Input         hPrivate  : Implementation specific data

 @Return        void

******************************************************************************/
static void RGXInitRiscvProcWrapper(const void *hPrivate)
{
	IMG_UINT32 ui32BootCodeRemap = RGXRISCVFW_BOOTLDR_CODE_REMAP;
	IMG_UINT32 ui32BootDataRemap = RGXRISCVFW_BOOTLDR_DATA_REMAP;
	IMG_DEV_VIRTADDR sTmp;

	RGXCommentLog(hPrivate, "RGXStart: Configure RISCV wrapper");

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) < 4)
#endif
	{
		RGXCommentLog(hPrivate, "RGXStart: Write boot code remap");
		RGXAcquireBootCodeAddr(hPrivate, &sTmp);
		RGXWriteReg64(hPrivate,
		              ui32BootCodeRemap,
		              sTmp.uiAddr |
		              (IMG_UINT64) (RGX_FIRMWARE_RAW_HEAP_SIZE >> FWCORE_ADDR_REMAP_CONFIG0_SIZE_ALIGNSHIFT)
		                << RGX_CR_FWCORE_ADDR_REMAP_CONFIG0_SIZE_SHIFT |
		              (IMG_UINT64) MMU_CONTEXT_MAPPING_FWPRIV << FWCORE_ADDR_REMAP_CONFIG0_MMU_CONTEXT_SHIFT |
		              RGX_CR_FWCORE_ADDR_REMAP_CONFIG0_FETCH_EN_EN);

		RGXCommentLog(hPrivate, "RGXStart: Write boot data remap");
		RGXAcquireBootDataAddr(hPrivate, &sTmp);
		RGXWriteReg64(hPrivate,
		              ui32BootDataRemap,
		              sTmp.uiAddr |
		              (IMG_UINT64) (RGX_FIRMWARE_RAW_HEAP_SIZE >> FWCORE_ADDR_REMAP_CONFIG0_SIZE_ALIGNSHIFT)
		                << RGX_CR_FWCORE_ADDR_REMAP_CONFIG0_SIZE_SHIFT |
		              (IMG_UINT64) MMU_CONTEXT_MAPPING_FWPRIV << FWCORE_ADDR_REMAP_CONFIG0_MMU_CONTEXT_SHIFT |
#if defined(SUPPORT_TRUSTED_DEVICE) && defined(RGX_CR_FWCORE_ADDR_REMAP_CONFIG0_TRUSTED_EN)
		              RGX_CR_FWCORE_ADDR_REMAP_CONFIG0_TRUSTED_EN |
#endif
		              RGX_CR_FWCORE_ADDR_REMAP_CONFIG0_LOAD_STORE_EN_EN);
	}

	/* Garten IDLE bit controlled by RISCV */
	RGXCommentLog(hPrivate, "RGXStart: Set GARTEN_IDLE type to RISCV");
	RGXWriteReg64(hPrivate, RGX_CR_MTS_GARTEN_WRAPPER_CONFIG, RGX_CR_MTS_GARTEN_WRAPPER_CONFIG_IDLE_CTRL_META);
}


static void RGXWriteKernelCatBase(const void *hPrivate, IMG_DEV_PHYADDR sPCAddr)
{
	IMG_UINT32 uiPCAddr;

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) > 1)
	{
		IMG_UINT32 ui32CBaseMapCtxReg = RGX_CR_MMU_CBASE_MAPPING_CONTEXT__HOST_SECURITY_GT1_AND_MHPW_LT6_AND_MMU_VER_GEQ4;

		uiPCAddr = (((sPCAddr.uiAddr >> RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_ALIGNSHIFT)
		             << RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_SHIFT)
		            & ~RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_CLRMSK);

		/* Set the mapping context */
		RGXWriteReg32(hPrivate, ui32CBaseMapCtxReg, MMU_CONTEXT_MAPPING_FWPRIV);
		(void)RGXReadReg32(hPrivate, ui32CBaseMapCtxReg); /* Fence write */

		/* Write the cat-base address */
		RGXWriteKernelMMUPC32(hPrivate,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_ALIGNSHIFT,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_SHIFT,
		                      uiPCAddr);

#if (MMU_CONTEXT_MAPPING_FWIF != MMU_CONTEXT_MAPPING_FWPRIV)
		/* Set-up different MMU ID mapping to the same PC used above */
		RGXWriteReg32(hPrivate, ui32CBaseMapCtxReg, MMU_CONTEXT_MAPPING_FWIF);
		(void)RGXReadReg32(hPrivate, ui32CBaseMapCtxReg); /* Fence write */

		RGXWriteKernelMMUPC32(hPrivate,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_ALIGNSHIFT,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_SHIFT,
		                      uiPCAddr);
#endif
	}
#else /* defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX) */
	if (!RGX_DEVICE_HAS_FEATURE(hPrivate, SLC_VIVT))
	{
		/* Write the cat-base address */
		RGXWriteKernelMMUPC64(hPrivate,
		                      BIF_CAT_BASEx(MMU_CONTEXT_MAPPING_FWPRIV),
		                      RGX_CR_BIF_CAT_BASE0_ADDR_ALIGNSHIFT,
		                      RGX_CR_BIF_CAT_BASE0_ADDR_SHIFT,
		                      ((sPCAddr.uiAddr
		                      >> RGX_CR_BIF_CAT_BASE0_ADDR_ALIGNSHIFT)
		                      << RGX_CR_BIF_CAT_BASE0_ADDR_SHIFT)
		                      & ~RGX_CR_BIF_CAT_BASE0_ADDR_CLRMSK);

		if (RGX_DEVICE_HAS_FEATURE(hPrivate, RISCV_FW_PROCESSOR))
		{
			/* Keep catbase registers in sync */
			RGXWriteKernelMMUPC64(hPrivate,
			                      FWCORE_MEM_CAT_BASEx(MMU_CONTEXT_MAPPING_FWPRIV),
			                      RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_ALIGNSHIFT,
			                      RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_SHIFT,
			                      ((sPCAddr.uiAddr
			                      >> RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_ALIGNSHIFT)
			                      << RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_SHIFT)
			                      & ~RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_CLRMSK);
		}

		/*
		 * Trusted Firmware boot
		 */
#if defined(SUPPORT_TRUSTED_DEVICE)
		RGXCommentLog(hPrivate, "RGXWriteKernelCatBase: Trusted Device enabled");
		RGXWriteReg32(hPrivate, RGX_CR_BIF_TRUST, RGX_CR_BIF_TRUST_ENABLE_EN);
#endif
	}
#endif /* defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX) */
	else
	{
		uiPCAddr = (((sPCAddr.uiAddr >> RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_ALIGNSHIFT)
		             << RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_SHIFT)
		            & ~RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_CLRMSK);

		/* Set the mapping context */
		RGXWriteReg32(hPrivate, RGX_CR_MMU_CBASE_MAPPING_CONTEXT, MMU_CONTEXT_MAPPING_FWPRIV);
		(void)RGXReadReg32(hPrivate, RGX_CR_MMU_CBASE_MAPPING_CONTEXT); /* Fence write */

		/* Write the cat-base address */
		RGXWriteKernelMMUPC32(hPrivate,
		                      RGX_CR_MMU_CBASE_MAPPING,
		                      RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_ALIGNSHIFT,
		                      RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_SHIFT,
		                      uiPCAddr);

#if (MMU_CONTEXT_MAPPING_FWIF != MMU_CONTEXT_MAPPING_FWPRIV)
		/* Set-up different MMU ID mapping to the same PC used above */
		RGXWriteReg32(hPrivate, RGX_CR_MMU_CBASE_MAPPING_CONTEXT, MMU_CONTEXT_MAPPING_FWIF);
		(void)RGXReadReg32(hPrivate, RGX_CR_MMU_CBASE_MAPPING_CONTEXT); /* Fence write */

		RGXWriteKernelMMUPC32(hPrivate,
		                      RGX_CR_MMU_CBASE_MAPPING,
		                      RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_ALIGNSHIFT,
		                      RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_SHIFT,
		                      uiPCAddr);
#endif
	}
}

/*!
*******************************************************************************

 @Function      RGXInitBIF

 @Description   Initialise RGX BIF

 @Input         hPrivate : Implementation specific data

 @Return        void

******************************************************************************/
static void RGXInitBIF(const void *hPrivate)
{
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, MIPS))
	{
		/*
		 * Trusted Firmware boot
		 */
#if defined(SUPPORT_TRUSTED_DEVICE)
		RGXCommentLog(hPrivate, "RGXInitBIF: Trusted Device enabled");
		RGXWriteReg32(hPrivate, RGX_CR_BIF_TRUST, RGX_CR_BIF_TRUST_ENABLE_EN);
#endif
	}
	else
#endif /* defined(RGX_FEATURE_MIPS_BIT_MASK) */
	{
		IMG_DEV_PHYADDR sPCAddr;

		/*
		 * Acquire the address of the Kernel Page Catalogue.
		 */
		RGXAcquireKernelMMUPC(hPrivate, &sPCAddr);

		/*
		 * Write the kernel catalogue base.
		 */
		RGXCommentLog(hPrivate, "RGX firmware MMU Page Catalogue");

		RGXWriteKernelCatBase(hPrivate, sPCAddr);
	}
}


#if defined(RGX_FEATURE_MMU_VERSION_MAX_VALUE_IDX)
/**************************************************************************/ /*!
@Function       RGXInitMMURangeRegisters
@Description    Initialises MMU range registers for Non4K pages.
@Input          hPrivate           Implementation specific data
@Return         void
 */ /**************************************************************************/
static void RGXInitMMURangeRegisters(const void *hPrivate)
{
	IMG_UINT32 ui32RegAddr = RGX_CR_MMU_PAGE_SIZE_RANGE_ONE;
	IMG_UINT32 i;

	for (i = 0; i < RGX_MAX_NUM_MMU_PAGE_SIZE_RANGES; ++i, ui32RegAddr += sizeof(IMG_UINT64))
	{
		RGXWriteReg64(hPrivate, ui32RegAddr, RGXMMUInitRangeValue(i));
	}
}
#endif


/*!
*******************************************************************************

 @Function      RGXInitAXIACE

 @Description    Initialise AXI-ACE interface

 @Input         hPrivate : Implementation specific data

 @Return        void

******************************************************************************/
static void RGXInitAXIACE(const void *hPrivate)
{
	IMG_UINT64 ui64RegVal;
	IMG_UINT32 ui32RegAddr;

#if defined(RGX_FEATURE_AXI_ACELITE_BIT_MASK)
	ui32RegAddr = RGX_CR_AXI_ACE_LITE_CONFIGURATION;

	/* Setup AXI-ACE config. Set everything to outer cache */
	ui64RegVal = (3U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_AWDOMAIN_NON_SNOOPING_SHIFT) |
	             (3U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_ARDOMAIN_NON_SNOOPING_SHIFT) |
	             (2U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_ARDOMAIN_CACHE_MAINTENANCE_SHIFT)  |
	             (2U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_AWDOMAIN_COHERENT_SHIFT) |
	             (2U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_ARDOMAIN_COHERENT_SHIFT) |
	             (2U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_AWCACHE_COHERENT_SHIFT) |
	             (2U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_ARCACHE_COHERENT_SHIFT) |
	             (2U << RGX_CR_AXI_ACE_LITE_CONFIGURATION_ARCACHE_CACHE_MAINTENANCE_SHIFT);

#if defined(FIX_HW_BRN_42321_BIT_MASK)
	if (RGX_DEVICE_HAS_BRN(hPrivate, 42321))
	{
		ui64RegVal |= (((IMG_UINT64) 1) << RGX_CR_AXI_ACE_LITE_CONFIGURATION_DISABLE_COHERENT_WRITELINEUNIQUE_SHIFT);
	}
#endif

#if defined(FIX_HW_BRN_68186_BIT_MASK)
	if (RGX_DEVICE_HAS_BRN(hPrivate, 68186))
	{
		/* default value for reg_enable_fence_out is zero. Force to 1 to allow core_clk < mem_clk */
		ui64RegVal |= (IMG_UINT64)1 << RGX_CR_AXI_ACE_LITE_CONFIGURATION_ENABLE_FENCE_OUT_SHIFT;
	}
#endif
#else /* defined(RGX_FEATURE_AXI_ACELITE_BIT_MASK) */
	ui32RegAddr = RGX_CR_ACE_CTRL;

	/**
	 * The below configuration is only applicable for RGX cores supporting
	 * ACE/ACE-lite protocol and connected to ACE coherent interconnect.
	 */

	/**
	 * Configure AxDomain and AxCache for MMU transactions.
	 * AxDomain set to non sharable (0x0).
	 */
	ui64RegVal = RGX_CR_ACE_CTRL_MMU_AWCACHE_WRITE_BACK_WRITE_ALLOCATE |
				 RGX_CR_ACE_CTRL_MMU_ARCACHE_WRITE_BACK_READ_ALLOCATE;

	/**
	 * Configure AxCache for PM/MMU transactions.
	 * Set to same value (i.e WBRWALLOC caching, rgxmmunit.c:RGXDerivePTEProt8)
	 * as non-coherent PTEs
	 */
	ui64RegVal |= (IMG_UINT64_C(0xF)) << RGX_CR_ACE_CTRL_PM_MMU_AXCACHE_SHIFT;

	/**
	 * Configure AxDomain for non MMU transactions.
	 */
	ui64RegVal |= (IMG_UINT64)(RGX_CR_ACE_CTRL_COH_DOMAIN_OUTER_SHAREABLE |
							   RGX_CR_ACE_CTRL_NON_COH_DOMAIN_NON_SHAREABLE);
#endif /* defined(RGX_FEATURE_AXI_ACELITE_BIT_MASK) */

	RGXCommentLog(hPrivate, "Init AXI-ACE interface");
	RGXWriteReg64(hPrivate, ui32RegAddr, ui64RegVal);
}

static void RGXMercerSoftResetSet(const void *hPrivate, IMG_UINT64 ui32MercerFlags)
{
	RGXWriteReg64(hPrivate, RGX_CR_MERCER_SOFT_RESET, ui32MercerFlags & RGX_CR_MERCER_SOFT_RESET_MASKFULL);

	/* Read soft-reset to fence previous write in order to clear the SOCIF pipeline */
	(void) RGXReadReg64(hPrivate, RGX_CR_MERCER_SOFT_RESET);
}

static void RGXSPUSoftResetAssert(const void *hPrivate)
{
	/* Assert Mercer0 */
	RGXMercerSoftResetSet(hPrivate, RGX_CR_MERCER0_SOFT_RESET_SPU_EN);
	/* Assert Mercer1 */
	RGXMercerSoftResetSet(hPrivate, RGX_CR_MERCER0_SOFT_RESET_SPU_EN | RGX_CR_MERCER1_SOFT_RESET_SPU_EN);
	/* Assert Mercer2 */
	RGXMercerSoftResetSet(hPrivate, RGX_CR_MERCER0_SOFT_RESET_SPU_EN | RGX_CR_MERCER1_SOFT_RESET_SPU_EN | RGX_CR_MERCER2_SOFT_RESET_SPU_EN);

	RGXWriteReg32(hPrivate, RGX_CR_SWIFT_SOFT_RESET, RGX_CR_SWIFT_SOFT_RESET_MASKFULL);
	/* Fence the previous write */
	(void) RGXReadReg32(hPrivate, RGX_CR_SWIFT_SOFT_RESET);

	RGXWriteReg32(hPrivate, RGX_CR_TEXAS_SOFT_RESET, RGX_CR_TEXAS_SOFT_RESET_MASKFULL);
	/* Fence the previous write */
	(void) RGXReadReg32(hPrivate, RGX_CR_TEXAS_SOFT_RESET);
}

static void RGXSPUSoftResetDeAssert(const void *hPrivate)
{
	RGXWriteReg32(hPrivate, RGX_CR_TEXAS_SOFT_RESET, 0);
	/* Fence the previous write */
	(void) RGXReadReg32(hPrivate, RGX_CR_TEXAS_SOFT_RESET);


	RGXWriteReg32(hPrivate, RGX_CR_SWIFT_SOFT_RESET, 0);
	/* Fence the previous write */
	(void) RGXReadReg32(hPrivate, RGX_CR_SWIFT_SOFT_RESET);

	/* Deassert Mercer2 */
	RGXMercerSoftResetSet(hPrivate, RGX_CR_MERCER0_SOFT_RESET_SPU_EN | RGX_CR_MERCER1_SOFT_RESET_SPU_EN);
	/* Deassert Mercer1 */
	RGXMercerSoftResetSet(hPrivate, RGX_CR_MERCER0_SOFT_RESET_SPU_EN);
	/* Deassert Mercer0 */
	RGXMercerSoftResetSet(hPrivate, 0);
}

static void RGXResetSequence(const void *hPrivate, const IMG_CHAR *pcRGXFW_PROCESSOR)
{
	/* Set RGX in soft-reset */
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, RISCV_FW_PROCESSOR))
	{
		RGXCommentLog(hPrivate, "RGXStart: soft reset cpu core");
		RGXWriteReg32(hPrivate, RGX_CR_FWCORE_BOOT, 0);
	}

	RGXCommentLog(hPrivate, "RGXStart: soft reset assert step 1");
	RGXSPUSoftResetAssert(hPrivate);

	RGXCommentLog(hPrivate, "RGXStart: soft reset assert step 2");
	RGXWriteReg64(hPrivate, RGX_CR_SOFT_RESET, RGX_SOFT_RESET_JONES_ALL);

	/* Read soft-reset to fence previous write in order to clear the SOCIF pipeline */
	(void) RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);

	RGXWriteReg64(hPrivate, RGX_CR_SOFT_RESET, RGX_SOFT_RESET_JONES_ALL | RGX_SOFT_RESET_EXTRA);

	(void) RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);

	/* Take everything out of reset but the FW processor */
	RGXCommentLog(hPrivate, "RGXStart: soft reset de-assert step 1 excluding %s", pcRGXFW_PROCESSOR);
	RGXWriteReg64(hPrivate, RGX_CR_SOFT_RESET, RGX_SOFT_RESET_EXTRA | RGX_CR_SOFT_RESET_GARTEN_EN);

	(void) RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);

	RGXWriteReg64(hPrivate, RGX_CR_SOFT_RESET, RGX_CR_SOFT_RESET_GARTEN_EN);

	(void) RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);

	RGXCommentLog(hPrivate, "RGXStart: soft reset de-assert step 2 excluding %s", pcRGXFW_PROCESSOR);
	RGXSPUSoftResetDeAssert(hPrivate);

	(void) RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);
}

static void DeassertMetaReset(const void *hPrivate)
{
	/* Need to wait for at least 32 cycles before taking the FW processor out of reset ... */
	RGXWaitCycles(hPrivate, 32, 3);

	RGXWriteReg64(hPrivate, RGX_CR_SOFT_RESET, 0x0);
	(void) RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);

	/* ... and afterwards */
	RGXWaitCycles(hPrivate, 32, 3);
}

static PVRSRV_ERROR InitJonesECCRAM(const void *hPrivate)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32Value;
	IMG_BOOL bMetaFW = RGX_DEVICE_HAS_FEATURE_VALUE(hPrivate, META);
	IMG_UINT32 ui32Mask;

	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, ECC_RAMS) == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	if (bMetaFW)
	{
		/* META must be taken out of reset (without booting) during Coremem initialization. */
		RGXWriteReg32(hPrivate, RGX_CR_META_BOOT, 0);
		DeassertMetaReset(hPrivate);
	}

	/* Clocks must be set to "on" during RAMs initialization. */
	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, PIPELINED_DATAMASTERS_VERSION) > 0)
	{
		RGXWriteReg64(hPrivate, RGX_CR_CLK_CTRL0, RGX_CR_CLK_CTRL0__PIPEDM_GT0__ALL_ON);
	}
	else
	{
		RGXWriteReg64(hPrivate, RGX_CR_CLK_CTRL0, RGX_CR_CLK_CTRL0_ALL_ON);
	}
	RGXWriteReg64(hPrivate, RGX_CR_CLK_CTRL1, RGX_CR_CLK_CTRL1_ALL_ON);
	RGXWriteReg32(hPrivate, RGX_CR_CLK_CTRL2, RGX_CR_CLK_CTRL2_ALL_ON);

	if (bMetaFW)
	{
		RGXWriteMetaRegThroughSP(hPrivate, META_CR_SYSC_JTAG_THREAD, META_CR_SYSC_JTAG_THREAD_PRIV_EN);
		RGXWriteMetaRegThroughSP(hPrivate, META_CR_TXCLKCTRL, META_CR_TXCLKCTRL_ALL_ON);
		RGXReadMetaRegThroughSP(hPrivate, META_CR_TXCLKCTRL, &ui32Value);
	}

	ui32Mask = bMetaFW ?
		RGX_CR_JONES_RAM_INIT_KICK_MASKFULL
		: RGX_CR_JONES_RAM_INIT_KICK_MASKFULL & ~RGX_CR_JONES_RAM_INIT_KICK_GARTEN_EN;

	/* SLC component in CHEST either does not exist or not powered at this point */
	ui32Mask &= RGX_CR_JONES_RAM_INIT_KICK_SLC_CHEST_CLRMSK;

	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, POWER_ISLAND_VERSION) >= 4)
	{
		/* FBCDC has components in CHEST which may not be powered at this point */
		ui32Mask &= RGX_CR_JONES_RAM_INIT_KICK_FBCDC_CLRMSK;
	}

	RGXWriteReg64(hPrivate, RGX_CR_JONES_RAM_INIT_KICK, ui32Mask);
	eError = RGXPollReg64(hPrivate, RGX_CR_JONES_RAM_STATUS, ui32Mask, ui32Mask);

	if (bMetaFW)
	{
		RGXWriteMetaRegThroughSP(hPrivate, META_CR_TXCLKCTRL, META_CR_TXCLKCTRL_ALL_AUTO);
		RGXReadMetaRegThroughSP(hPrivate, META_CR_TXCLKCTRL, &ui32Value);
	}

	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, PIPELINED_DATAMASTERS_VERSION) > 0)
	{
		RGXWriteReg64(hPrivate, RGX_CR_CLK_CTRL0, RGX_CR_CLK_CTRL0__PIPEDM_GT0__ALL_AUTO);
	}
	else
	{
		RGXWriteReg64(hPrivate, RGX_CR_CLK_CTRL0, RGX_CR_CLK_CTRL0_ALL_AUTO);
	}
	RGXWriteReg64(hPrivate, RGX_CR_CLK_CTRL1, RGX_CR_CLK_CTRL1_ALL_AUTO);
	RGXWriteReg32(hPrivate, RGX_CR_CLK_CTRL2, RGX_CR_CLK_CTRL2_ALL_AUTO);

	if (bMetaFW)
	{
		RGXWriteReg64(hPrivate, RGX_CR_SOFT_RESET, RGX_CR_SOFT_RESET_GARTEN_EN);
		RGXReadReg64(hPrivate, RGX_CR_SOFT_RESET);
	}

	return eError;
}


PVRSRV_ERROR RGXStart(const void *hPrivate)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_CHAR *pcRGXFW_PROCESSOR;
	IMG_BOOL bDoFWSlaveBoot = IMG_FALSE;
	IMG_BOOL bMetaFW = IMG_FALSE;

	if (RGX_DEVICE_HAS_FEATURE(hPrivate, RISCV_FW_PROCESSOR))
	{
		pcRGXFW_PROCESSOR = RGXFW_PROCESSOR_RISCV;
	}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	else if (RGX_DEVICE_HAS_FEATURE(hPrivate, MIPS))
	{
		pcRGXFW_PROCESSOR = RGXFW_PROCESSOR_MIPS;
	}
#endif
	else
	{
		pcRGXFW_PROCESSOR = RGXFW_PROCESSOR_META;
		bMetaFW = IMG_TRUE;
		bDoFWSlaveBoot = RGXDoFWSlaveBoot(hPrivate);
	}

	if (RGX_DEVICE_HAS_FEATURE(hPrivate, SYS_BUS_SECURE_RESET))
	{
		/* Disable the default sys_bus_secure protection to perform minimal setup */
		RGXCommentLog(hPrivate, "RGXStart: Disable sys_bus_secure");
		RGXWriteReg32(hPrivate, RGX_CR_SYS_BUS_SECURE, 0);
		(void) RGXReadReg32(hPrivate, RGX_CR_SYS_BUS_SECURE); /* Fence write */
	}

#if defined(RGX_FEATURE_HYPERVISOR_MMU_BIT_MASK)
	/* Only bypass HMMU if the module is present */
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, HYPERVISOR_MMU))
	{
		/* Always set HMMU in bypass mode */
		RGXWriteReg32(hPrivate, RGX_CR_HMMU_BYPASS, RGX_CR_HMMU_BYPASS_MASKFULL);
		(void) RGXReadReg32(hPrivate, RGX_CR_HMMU_BYPASS);
	}
#endif

	if (RGX_DEVICE_HAS_FEATURE(hPrivate, RISCV_DUAL_LOCKSTEP))
	{
		/* Enable the dual LS mode */
		RGXCommentLog(hPrivate, "RGXStart: Enable Dual Core Lock Step");
		RGXWriteReg32(hPrivate, RGX_CR_FIRMWARE_PROCESSOR_LS, (0x3 << RGX_CR_FIRMWARE_PROCESSOR_LS__RDL__ENABLE_SHIFT));
		(void) RGXReadReg32(hPrivate, RGX_CR_FIRMWARE_PROCESSOR_LS);
	}


	/*!
	 * Start FW init sequence
	 */
	RGXResetSequence(hPrivate, pcRGXFW_PROCESSOR);

	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, ECC_RAMS) > 0)
	{
		RGXCommentLog(hPrivate, "RGXStart: Init Jones ECC RAM");
		eError = InitJonesECCRAM(hPrivate);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		if (RGX_DEVICE_HAS_FEATURE(hPrivate, GPU_MULTICORE_SUPPORT))
		{
			/* Set OR reduce for ECC faults to ensure faults are not missed during early boot stages */
			RGXWriteReg32(hPrivate, RGX_CR_MULTICORE_EVENT_REDUCE, RGX_CR_MULTICORE_EVENT_REDUCE_FAULT_FW_EN | RGX_CR_MULTICORE_EVENT_REDUCE_FAULT_GPU_EN);
		}

		/* Route fault events to the host */
		RGXWriteReg32(hPrivate, RGX_CR_EVENT_ENABLE, RGX_CR_EVENT_ENABLE_FAULT_FW_EN);
	}

	if (bMetaFW)
	{
		if (bDoFWSlaveBoot)
		{
			/* Configure META to Slave boot */
			RGXCommentLog(hPrivate, "RGXStart: META Slave boot");
			RGXWriteReg32(hPrivate, RGX_CR_META_BOOT, 0);
		}
		else
		{
			/* Configure META to Master boot */
			RGXCommentLog(hPrivate, "RGXStart: META Master boot");
			RGXWriteReg32(hPrivate, RGX_CR_META_BOOT, RGX_CR_META_BOOT_MODE_EN);
		}
	}

	/*
	 * Initialise Firmware wrapper
	 */
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, RISCV_FW_PROCESSOR))
	{
		RGXInitRiscvProcWrapper(hPrivate);
	}
	else if (bMetaFW)
	{
		RGXInitMetaProcWrapper(hPrivate);
	}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	else
	{
		RGXInitMipsProcWrapper(hPrivate);
	}
#endif

#if defined(RGX_FEATURE_MMU_VERSION_MAX_VALUE_IDX)
	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, MMU_VERSION) >= 4)
	{
		/* initialise the MMU range based config registers for Non4K pages */
		RGXInitMMURangeRegisters(hPrivate);
	}
#endif

#if defined(RGX_FEATURE_AXI_ACELITE_BIT_MASK)
	if (RGX_DEVICE_HAS_FEATURE(hPrivate, AXI_ACELITE))
#endif
	{
		/* We must init the AXI-ACE interface before 1st BIF transaction */
		RGXInitAXIACE(hPrivate);
	}

	/*
	 * Initialise BIF.
	 */
	RGXInitBIF(hPrivate);

	RGXSetPoweredState(hPrivate, IMG_TRUE);

	RGXCommentLog(hPrivate, "RGXStart: Take %s out of reset", pcRGXFW_PROCESSOR);
	DeassertMetaReset(hPrivate);

	if (bMetaFW && bDoFWSlaveBoot)
	{
		eError = RGXFabricCoherencyTest(hPrivate);
		if (eError != PVRSRV_OK) return eError;

		RGXCommentLog(hPrivate, "RGXStart: RGX Firmware Slave boot Start");
		eError = RGXStartFirmware(hPrivate);
		if (eError != PVRSRV_OK) return eError;
	}
	else
	{
		RGXCommentLog(hPrivate, "RGXStart: RGX Firmware Master boot Start");

		if (RGX_DEVICE_HAS_FEATURE(hPrivate, RISCV_FW_PROCESSOR))
		{
			/* Bring Debug Module out of reset */
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
			if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) >= 4)
			{
				RGXWriteReg32(hPrivate, RGX_CR_FWCORE_DMI_DMCONTROL__HOST_SECURITY_GEQ4, RGX_CR_FWCORE_DMI_DMCONTROL__HOST_SECURITY_GEQ4__DMACTIVE_EN);
			}
			else
#endif
			{
				RGXWriteReg32(hPrivate, RGX_CR_FWCORE_DMI_DMCONTROL, RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN);
			}

			/* Boot the FW */
			RGXWriteReg32(hPrivate, RGX_CR_FWCORE_BOOT, 1);
			RGXWaitCycles(hPrivate, 32, 3);
		}
	}

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(SUPPORT_SECURITY_VALIDATION)
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) == 1)
#endif
	{
		RGXCommentLog(hPrivate, "RGXStart: Enable sys_bus_secure");
		RGXWriteReg32(hPrivate, RGX_CR_SYS_BUS_SECURE, RGX_CR_SYS_BUS_SECURE_ENABLE_EN);
		(void) RGXReadReg32(hPrivate, RGX_CR_SYS_BUS_SECURE); /* Fence write */
	}
#endif

	return eError;
}

PVRSRV_ERROR RGXStop(const void *hPrivate)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_BOOL bMetaFW = RGX_DEVICE_HAS_FEATURE_VALUE(hPrivate, META);
	IMG_UINT32 ui32JonesIdleMask = RGX_CR_JONES_IDLE_MASKFULL^(RGX_CR_JONES_IDLE_AXI2IMG_EN|RGX_CR_JONES_IDLE_SI_AXI2IMG_EN);

	RGXDeviceAckIrq(hPrivate);

	/* Set FW power state OFF to disable LISR handler */
	RGXSetPoweredState(hPrivate, IMG_FALSE);


	/* Wait for Sidekick/Jones to signal IDLE except for the Garten Wrapper */
	if (!RGX_DEVICE_HAS_FEATURE_VALUE(hPrivate, RAY_TRACING_ARCH) ||
	    RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, RAY_TRACING_ARCH) < 2)
	{
		ui32JonesIdleMask ^= (RGX_CR_JONES_IDLE_ASC_EN|RGX_CR_JONES_IDLE_RCE_EN);
	}

#if defined(FIX_HW_BRN_74812_BIT_MASK)
	if (RGX_DEVICE_HAS_BRN(hPrivate, 74812))
	{
		/* BRN74812 means the IPP idle signal may not be valid and is therefore ignored... */
		ui32JonesIdleMask &= RGX_CR_JONES_IDLE_IPP_CLRMSK;
	}
#endif

	eError = RGXPollReg32(hPrivate,
	                      RGX_CR_JONES_IDLE,
	                      ui32JonesIdleMask^(RGX_CR_JONES_IDLE_GARTEN_EN|RGX_CR_JONES_IDLE_SOCIF_EN),
	                      ui32JonesIdleMask^(RGX_CR_JONES_IDLE_GARTEN_EN|RGX_CR_JONES_IDLE_SOCIF_EN));

	if (eError != PVRSRV_OK) return eError;


	/* Wait for SLC to signal IDLE */
	eError = RGXPollReg32(hPrivate,
	                      RGX_CR_SLC_IDLE,
	                      RGX_CR_SLC_IDLE_MASKFULL^(CR_IDLE_UNSELECTED_MASK),
	                      RGX_CR_SLC_IDLE_MASKFULL^(CR_IDLE_UNSELECTED_MASK));
	if (eError != PVRSRV_OK) return eError;


	/* Unset MTS DM association with threads */
	RGXWriteReg32(hPrivate,
	              RGX_CR_MTS_INTCTX_THREAD0_DM_ASSOC,
	              RGX_CR_MTS_INTCTX_THREAD0_DM_ASSOC_DM_ASSOC_CLRMSK
	              & RGX_CR_MTS_INTCTX_THREAD0_DM_ASSOC_MASKFULL);
	RGXWriteReg32(hPrivate,
	              RGX_CR_MTS_BGCTX_THREAD0_DM_ASSOC,
	              RGX_CR_MTS_BGCTX_THREAD0_DM_ASSOC_DM_ASSOC_CLRMSK
	              & RGX_CR_MTS_BGCTX_THREAD0_DM_ASSOC_MASKFULL);

	if (bMetaFW)
	{
		RGXWriteReg32(hPrivate,
					  RGX_CR_MTS_INTCTX_THREAD1_DM_ASSOC,
					  RGX_CR_MTS_INTCTX_THREAD1_DM_ASSOC_DM_ASSOC_CLRMSK
					  & RGX_CR_MTS_INTCTX_THREAD1_DM_ASSOC_MASKFULL);
		RGXWriteReg32(hPrivate,
					  RGX_CR_MTS_BGCTX_THREAD1_DM_ASSOC,
					  RGX_CR_MTS_BGCTX_THREAD1_DM_ASSOC_DM_ASSOC_CLRMSK
					  & RGX_CR_MTS_BGCTX_THREAD1_DM_ASSOC_MASKFULL);
	}

#if defined(PDUMP)
	if (bMetaFW)
	{
		/* Disabling threads is only required for pdumps to stop the fw gracefully */

		/* Disable thread 0 */
		eError = RGXWriteMetaRegThroughSP(hPrivate,
		                                  META_CR_T0ENABLE_OFFSET,
		                                  ~META_CR_TXENABLE_ENABLE_BIT);
		if (eError != PVRSRV_OK) return eError;

		/* Disable thread 1 */
		eError = RGXWriteMetaRegThroughSP(hPrivate,
		                                  META_CR_T1ENABLE_OFFSET,
		                                  ~META_CR_TXENABLE_ENABLE_BIT);
		if (eError != PVRSRV_OK) return eError;

		/* Clear down any irq raised by META (done after disabling the FW
		 * threads to avoid a race condition).
		 * This is only really needed for PDumps but we do it anyway driver-live.
		 */
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
		/* Wait for the Slave Port to finish all the transactions */
		if (RGX_DEVICE_HAS_FEATURE(hPrivate, META_REGISTER_UNPACKED_ACCESSES))
		{
			if (RGX_DEVICE_GET_FEATURE_VALUE(hPrivate, HOST_SECURITY_VERSION) > 1)
			{
				RGXWriteReg32(hPrivate, RGX_CR_META_SP_MSLVIRQSTATUS__HOST_SECURITY_GT1_AND_MRUA, 0x0);
				(void)RGXReadReg32(hPrivate, RGX_CR_META_SP_MSLVIRQSTATUS__HOST_SECURITY_GT1_AND_MRUA); /* Fence write */

				eError = RGXPollReg32(hPrivate,
									  RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA,
									  RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN
									  | RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN,
									  RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN
									  | RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN);
			}
			else
			{
				RGXWriteReg32(hPrivate, RGX_CR_META_SP_MSLVIRQSTATUS__HOST_SECURITY_EQ1_AND_MRUA, 0x0);
				(void)RGXReadReg32(hPrivate, RGX_CR_META_SP_MSLVIRQSTATUS__HOST_SECURITY_EQ1_AND_MRUA); /* Fence write */

				eError = RGXPollReg32(hPrivate,
									  RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA,
									  RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN
									  | RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN,
									  RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN
									  | RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN);
			}
		}
		else
#endif
		{
			RGXWriteReg32(hPrivate, RGX_CR_META_SP_MSLVIRQSTATUS, 0x0);
			(void)RGXReadReg32(hPrivate, RGX_CR_META_SP_MSLVIRQSTATUS); /* Fence write */

			eError = RGXPollReg32(hPrivate,
								  RGX_CR_META_SP_MSLVCTRL1,
								  RGX_CR_META_SP_MSLVCTRL1_READY_EN | RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
								  RGX_CR_META_SP_MSLVCTRL1_READY_EN | RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN);
		}

		if (eError != PVRSRV_OK) return eError;
	}
#endif


	eError = RGXPollReg64(hPrivate,
	                      RGX_CR_SLC_STATUS1,
	                      0,
	                      (~RGX_CR_SLC_STATUS1_BUS0_OUTSTANDING_READS_CLRMSK |
	                       ~RGX_CR_SLC_STATUS1_BUS1_OUTSTANDING_READS_CLRMSK |
	                       ~RGX_CR_SLC_STATUS1_BUS0_OUTSTANDING_WRITES_CLRMSK |
	                       ~RGX_CR_SLC_STATUS1_BUS1_OUTSTANDING_WRITES_CLRMSK));
	if (eError != PVRSRV_OK) return eError;

	eError = RGXPollReg64(hPrivate,
	                      RGX_CR_SLC_STATUS2,
	                      0,
	                      (~RGX_CR_SLC_STATUS2_BUS2_OUTSTANDING_READS_CLRMSK |
	                       ~RGX_CR_SLC_STATUS2_BUS3_OUTSTANDING_READS_CLRMSK |
	                       ~RGX_CR_SLC_STATUS2_BUS2_OUTSTANDING_WRITES_CLRMSK |
	                       ~RGX_CR_SLC_STATUS2_BUS3_OUTSTANDING_WRITES_CLRMSK));
	if (eError != PVRSRV_OK) return eError;


	/* Wait for SLC to signal IDLE */
	eError = RGXPollReg32(hPrivate,
	                      RGX_CR_SLC_IDLE,
	                      RGX_CR_SLC_IDLE_MASKFULL^(CR_IDLE_UNSELECTED_MASK),
	                      RGX_CR_SLC_IDLE_MASKFULL^(CR_IDLE_UNSELECTED_MASK));
	if (eError != PVRSRV_OK) return eError;


	/* Wait for Jones to signal IDLE except for the Garten Wrapper */
	eError = RGXPollReg32(hPrivate,
	                      RGX_CR_JONES_IDLE,
	                      ui32JonesIdleMask^(RGX_CR_JONES_IDLE_GARTEN_EN|RGX_CR_JONES_IDLE_SOCIF_EN),
	                      ui32JonesIdleMask^(RGX_CR_JONES_IDLE_GARTEN_EN|RGX_CR_JONES_IDLE_SOCIF_EN));
	if (eError != PVRSRV_OK) return eError;

	if (bMetaFW)
	{
		IMG_UINT32 ui32RegValue;

		eError = RGXReadMetaRegThroughSP(hPrivate,
		                                 META_CR_TxVECINT_BHALT,
		                                 &ui32RegValue);
		if (eError != PVRSRV_OK) return eError;

		if ((ui32RegValue & 0xFFFFFFFFU) == 0x0)
		{
			/* Wait for Sidekick/Jones to signal IDLE including
			 * the Garten Wrapper if there is no debugger attached
			 * (TxVECINT_BHALT = 0x0) */
			eError = RGXPollReg32(hPrivate,
			                      RGX_CR_JONES_IDLE,
			                      ui32JonesIdleMask^RGX_CR_JONES_IDLE_SOCIF_EN,
			                      ui32JonesIdleMask^RGX_CR_JONES_IDLE_SOCIF_EN);
			if (eError != PVRSRV_OK) return eError;
		}
	}
	else
	{
		eError = RGXPollReg32(hPrivate,
		                      RGX_CR_JONES_IDLE,
		                      ui32JonesIdleMask^RGX_CR_JONES_IDLE_SOCIF_EN,
		                      ui32JonesIdleMask^RGX_CR_JONES_IDLE_SOCIF_EN);
		if (eError != PVRSRV_OK) return eError;
	}

	return eError;
}

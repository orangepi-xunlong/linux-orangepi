/*************************************************************************/ /*!
@File
@Title          Services initialisation routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific functions
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

#include "img_defs.h"
#include "srvinit.h"
#include "pvr_debug.h"
#include "osfunc.h"
#include "km_apphint_defs.h"
#include "htbuffer_types.h"

#include "devicemem.h"
#include "devicemem_pdump.h"

#include "rgx_fwif_km.h"
#include "pdump_km.h"

#include "rgxinit_apphints.h"
#include "rgxinit.h"
#include "rgxmulticore.h"

#include "rgx_compat_bvnc.h"

#include "osfunc.h"

#include "rgxdefs_km.h"

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "virt_validation_defs.h"
#endif

#include "rgx_fwif_hwperf.h"
#include "rgx_hwperf_table.h"

#include "fwload.h"
#include "rgxlayer_impl.h"
#include "rgxfwimageutils.h"
#include "rgxfwutils.h"

#include "rgx_hwperf.h"
#include "rgx_bvnc_defs_km.h"

#include "rgxdevice.h"

#include "pvrsrv.h"

#if defined(SUPPORT_TRUSTED_DEVICE)
#include "rgxdevice.h"
#include "pvrsrv_device.h"
#endif

#define	HW_PERF_FILTER_DEFAULT         0x00000000 /* Default to no HWPerf */
#define HW_PERF_FILTER_DEFAULT_ALL_ON  0xFFFFFFFF /* All events */
#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
#define AVAIL_POW_UNITS_MASK_DEFAULT   (PVRSRV_APPHINT_HWVALAVAILABLESPUMASK)
#define AVAIL_RAC_MASK_DEFAULT         (PVRSRV_APPHINT_HWVALAVAILABLERACMASK)
#endif

/* Kernel CCB size */

#if !defined(PVRSRV_RGX_LOG2_KERNEL_CCB_MIN_SIZE)
#define PVRSRV_RGX_LOG2_KERNEL_CCB_MIN_SIZE 4
#endif
#if !defined(PVRSRV_RGX_LOG2_KERNEL_CCB_MAX_SIZE)
#define PVRSRV_RGX_LOG2_KERNEL_CCB_MAX_SIZE 16
#endif

#if PVRSRV_APPHINT_KCCB_SIZE_LOG2 < PVRSRV_RGX_LOG2_KERNEL_CCB_MIN_SIZE
#error PVRSRV_APPHINT_KCCB_SIZE_LOG2 is too low.
#elif PVRSRV_APPHINT_KCCB_SIZE_LOG2 > PVRSRV_RGX_LOG2_KERNEL_CCB_MAX_SIZE
#error PVRSRV_APPHINT_KCCB_SIZE_LOG2 is too high.
#endif


#include "os_apphint.h"

/*
 * _ParseHTBAppHints:
 *
 * Generate necessary references to the globally visible AppHints which are
 * declared in the above #include "km_apphint_defs.h"
 * Without these local references some compiler tool-chains will treat
 * unreferenced declarations as fatal errors. This function duplicates the
 * HTB_specific apphint references which are made in htbserver.c:HTBInit()
 * However, it makes absolutely *NO* use of these hints.
 */
static void
_ParseHTBAppHints(PVRSRV_DEVICE_NODE *psDeviceNode, void *pvAppHintState)
{
	IMG_UINT32 ui32AppHintDefault;
	IMG_UINT32 ui32LogType;
	IMG_UINT32 ui32OpMode;
	IMG_UINT32 ui32BufferSize;

	ui32AppHintDefault = PVRSRV_APPHINT_ENABLEHTBLOGGROUP;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, EnableHTBLogGroup,
	                     &ui32AppHintDefault, &ui32LogType);
	ui32AppHintDefault = PVRSRV_APPHINT_HTBOPERATIONMODE;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, HTBOperationMode,
	                     &ui32AppHintDefault, &ui32OpMode);
	ui32AppHintDefault = PVRSRV_APPHINT_HTBUFFERSIZE;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, HTBufferSizeInKB,
	                     &ui32AppHintDefault, &ui32BufferSize);
}

/*!
*******************************************************************************

 @Function      GetFilterFlags

 @Description   Initialise and return filter flags

 @Input         bFilteringMode : Enable new TPU filtering mode
 @Input         ui32TruncateMode : TPU Truncate mode

 @Return        IMG_UINT32 : Filter flags

******************************************************************************/
static INLINE IMG_UINT32 GetFilterFlags(IMG_BOOL bFilteringMode, IMG_UINT32 ui32TruncateMode)
{
	IMG_UINT32 ui32FilterFlags = 0;

	ui32FilterFlags |= bFilteringMode ? RGXFWIF_FILTCFG_NEW_FILTER_MODE : 0;
	if (ui32TruncateMode == 2)
	{
		ui32FilterFlags |= RGXFWIF_FILTCFG_TRUNCATE_INT;
	}
	else if (ui32TruncateMode == 3)
	{
		ui32FilterFlags |= RGXFWIF_FILTCFG_TRUNCATE_HALF;
	}

	return ui32FilterFlags;
}


/*!
*******************************************************************************

 @Function      InitDeviceFlags

 @Description   Initialise and return device flags

 @Input         psDeviceNode     : Pointer to device node
 @Input         pvAppHintState   : Pointer to apphint state
 @Input         psHints          : Apphints container

 @Return        void

******************************************************************************/
static INLINE void InitDeviceFlags(PVRSRV_DEVICE_NODE *psDeviceNode,
                                   void *pvAppHintState,
                                   RGX_INIT_APPHINTS *psHints)
{
	IMG_UINT32 ui32DeviceFlags = 0;
	IMG_BOOL bAppHintDefault;
	IMG_BOOL bZeroFreelist;
	IMG_BOOL bDisableFEDLogging;


	bAppHintDefault = PVRSRV_APPHINT_ZEROFREELIST;
	OSGetAppHintBOOL(psDeviceNode,  pvAppHintState,  ZeroFreelist,
	                   &bAppHintDefault,         &bZeroFreelist);
	ui32DeviceFlags |= bZeroFreelist ? RGXKM_DEVICE_STATE_ZERO_FREELIST : 0;

	bAppHintDefault = PVRSRV_APPHINT_DISABLEFEDLOGGING;
	OSGetAppHintBOOL(psDeviceNode,    pvAppHintState,    DisableFEDLogging,
	                   &bAppHintDefault,        &bDisableFEDLogging);
	ui32DeviceFlags |= bDisableFEDLogging ? RGXKM_DEVICE_STATE_DISABLE_DW_LOGGING_EN : 0;


#if defined(PVRSRV_ENABLE_CCCB_GROW)
	BITMASK_SET(ui32DeviceFlags, RGXKM_DEVICE_STATE_CCB_GROW_EN);
#endif

	psHints->ui32DeviceFlags = ui32DeviceFlags;
}

/*!
*******************************************************************************

 @Function      GetApphints

 @Description   Read init time apphints and initialise apphints structure

 @Input         psHints : Pointer to apphints container

 @Return        void

******************************************************************************/
static INLINE void GetApphints(PVRSRV_RGXDEV_INFO *psDevInfo, RGX_INIT_APPHINTS *psHints)
{
	void *pvAppHintState = NULL;
	IMG_UINT32 ui32AppHintDefault;
	IMG_BOOL bAppHintDefault;
	IMG_UINT32 ui32ParamTemp;
#if defined(__linux__)
	IMG_UINT64 ui64AppHintDefault;
#endif

	OSCreateAppHintState(&pvAppHintState);

	bAppHintDefault = PVRSRV_APPHINT_ENABLESIGNATURECHECKS;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,        pvAppHintState,    EnableSignatureChecks,
	                     &bAppHintDefault,    &psHints->bEnableSignatureChecks);
	ui32AppHintDefault = PVRSRV_APPHINT_SIGNATURECHECKSBUFSIZE;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    SignatureChecksBufSize,
	                     &ui32AppHintDefault,    &psHints->ui32SignatureChecksBufSize);

	bAppHintDefault = PVRSRV_APPHINT_ASSERTOUTOFMEMORY;
	OSGetAppHintBOOL(psDevInfo->psDeviceNode,    pvAppHintState,    AssertOutOfMemory,
	                   &bAppHintDefault,        &psHints->bAssertOnOutOfMem);
	bAppHintDefault = PVRSRV_APPHINT_ASSERTONHWRTRIGGER;
	OSGetAppHintBOOL(psDevInfo->psDeviceNode,    pvAppHintState,    AssertOnHWRTrigger,
	                   &bAppHintDefault,        &psHints->bAssertOnHWRTrigger);
	bAppHintDefault = PVRSRV_APPHINT_CHECKMLIST;
	OSGetAppHintBOOL(psDevInfo->psDeviceNode,    pvAppHintState,    CheckMList,
	                   &bAppHintDefault,        &psHints->bCheckMlist);
	bAppHintDefault = PVRSRV_APPHINT_DISABLECLOCKGATING;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,          pvAppHintState,    DisableClockGating,
	                   &bAppHintDefault,        &psHints->bDisableClockGating);
	bAppHintDefault = PVRSRV_APPHINT_DISABLEDMOVERLAP;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,          pvAppHintState,    DisableDMOverlap,
	                   &bAppHintDefault,        &psHints->bDisableDMOverlap);
	ui32AppHintDefault = PVRSRV_APPHINT_ENABLEAPM;
	OSGetAppHintUINT32(psDevInfo->psDeviceNode,  pvAppHintState,    EnableAPM,
	                     &ui32AppHintDefault,      &ui32ParamTemp);
	psHints->eRGXActivePMConf = ui32ParamTemp;
	bAppHintDefault = PVRSRV_APPHINT_ENABLECDMKILLINGRANDMODE;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,        pvAppHintState,    EnableCDMKillingRandMode,
	                   &bAppHintDefault,      &psHints->bEnableDMKillRand);
	bAppHintDefault = PVRSRV_APPHINT_ENABLERANDOMCONTEXTSWITCH;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,        pvAppHintState,    EnableRandomContextSwitch,
	                   &bAppHintDefault,      &psHints->bEnableRandomCsw);
	bAppHintDefault = PVRSRV_APPHINT_ENABLESOFTRESETCONTEXTSWITCH;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,        pvAppHintState,    EnableSoftResetContextSwitch,
	                   &bAppHintDefault,      &psHints->bEnableSoftResetCsw);
	ui32AppHintDefault = PVRSRV_APPHINT_ENABLEFWCONTEXTSWITCH;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    EnableFWContextSwitch,
	                     &ui32AppHintDefault,    &psHints->ui32EnableFWContextSwitch);
	ui32AppHintDefault = PVRSRV_APPHINT_ENABLERDPOWERISLAND;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    EnableRDPowerIsland,
	                     &ui32AppHintDefault,    &ui32ParamTemp);
	psHints->eRGXRDPowerIslandConf = ui32ParamTemp;
#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	bAppHintDefault = PVRSRV_APPHINT_ENABLESPUCLOCKGATING;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,          pvAppHintState,    EnableSPUClockGating,
	                   &bAppHintDefault,           &psHints->bSPUClockGating);
#endif
	ui32AppHintDefault = PVRSRV_APPHINT_FIRMWAREPERF;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    FirmwarePerf,
	                     &ui32AppHintDefault,    &ui32ParamTemp);
	psHints->eFirmwarePerf = ui32ParamTemp;
	ui32AppHintDefault = PVRSRV_APPHINT_FWCONTEXTSWITCHPROFILE;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    FWContextSwitchProfile,
	                     &ui32AppHintDefault,    &psHints->ui32FWContextSwitchProfile);

	/*
	 * HWPerf apphints *
	 */
	bAppHintDefault = PVRSRV_APPHINT_HWPERFDISABLECUSTOMCOUNTERFILTER;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,        pvAppHintState,    HWPerfDisableCustomCounterFilter,
	                     &bAppHintDefault,    &psHints->bHWPerfDisableCounterFilter);
	ui32AppHintDefault = PVRSRV_APPHINT_HWPERFHOSTBUFSIZEINKB;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    HWPerfHostBufSizeInKB,
	                     &ui32AppHintDefault,    &psHints->ui32HWPerfHostBufSize);
	ui32AppHintDefault = PVRSRV_APPHINT_HWPERFFWBUFSIZEINKB;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,    HWPerfFWBufSizeInKB,
	                     &ui32AppHintDefault,    &psHints->ui32HWPerfFWBufSize);

	ui32AppHintDefault = PVRSRV_APPHINT_KCCB_SIZE_LOG2;
	OSGetAppHintUINT32(psDevInfo->psDeviceNode,  pvAppHintState,  KernelCCBSizeLog2,
	                     &ui32AppHintDefault,    &psHints->ui32KCCBSizeLog2);

	if (psHints->ui32KCCBSizeLog2 < PVRSRV_RGX_LOG2_KERNEL_CCB_MIN_SIZE)
	{
		PVR_DPF((PVR_DBG_WARNING, "KCCB size %u is too low, setting to %u",
		         psHints->ui32KCCBSizeLog2, PVRSRV_RGX_LOG2_KERNEL_CCB_MIN_SIZE));
		psHints->ui32KCCBSizeLog2 = PVRSRV_RGX_LOG2_KERNEL_CCB_MIN_SIZE;
	}
	else if (psHints->ui32KCCBSizeLog2 > PVRSRV_RGX_LOG2_KERNEL_CCB_MAX_SIZE)
	{
		PVR_DPF((PVR_DBG_WARNING, "KCCB size %u is too high, setting to %u",
		         psHints->ui32KCCBSizeLog2, PVRSRV_RGX_LOG2_KERNEL_CCB_MAX_SIZE));
		psHints->ui32KCCBSizeLog2 = PVRSRV_RGX_LOG2_KERNEL_CCB_MAX_SIZE;
	}


#if defined(PVR_ARCH_VOLCANIC)
	ui32AppHintDefault = PVRSRV_APPHINT_ISPSCHEDULINGLATENCYMODE;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,    pvAppHintState,  ISPSchedulingLatencyMode,
	                     &ui32AppHintDefault,  &psHints->ui32ISPSchedulingLatencyMode);
#endif

#if defined(__linux__)
	/* name changes */
	{
		IMG_UINT64 ui64Tmp;
		bAppHintDefault = PVRSRV_APPHINT_DISABLEPDUMPPANIC;
		OSGetAppHintBOOL(psDevInfo->psDeviceNode,    pvAppHintState,  DisablePDumpPanic,
		                   &bAppHintDefault,           &psHints->bDisablePDP);
		ui64AppHintDefault = PVRSRV_APPHINT_HWPERFFWFILTER;
		OSGetAppHintUINT64(psDevInfo->psDeviceNode,  pvAppHintState,  HWPerfFWFilter,
		                   &ui64AppHintDefault,        &ui64Tmp);
		psHints->ui32HWPerfFilter0 = (IMG_UINT32)(ui64Tmp & 0xffffffffllu);
		psHints->ui32HWPerfFilter1 = (IMG_UINT32)((ui64Tmp >> 32) & 0xffffffffllu);
	}
#endif

	ui32AppHintDefault = PVRSRV_APPHINT_HWPERFHOSTFILTER;
	OSGetAppHintUINT32(psDevInfo->psDeviceNode,      pvAppHintState,    HWPerfHostFilter,
	                     &ui32AppHintDefault,          &psHints->ui32HWPerfHostFilter);
	ui32AppHintDefault = PVRSRV_APPHINT_TIMECORRCLOCK;
	OSGetAppHintUINT32(psDevInfo->psDeviceNode,  pvAppHintState,    TimeCorrClock,
	                         &ui32AppHintDefault,      &psHints->ui32TimeCorrClock);
	ui32AppHintDefault = PVRSRV_APPHINT_HWRDEBUGDUMPLIMIT;
	OSGetAppHintUINT32(psDevInfo->psDeviceNode,      pvAppHintState,    HWRDebugDumpLimit,
	                     &ui32AppHintDefault,          &ui32ParamTemp);
	psHints->ui32HWRDebugDumpLimit = MIN(ui32ParamTemp, RGXFWIF_HWR_DEBUG_DUMP_ALL);

	{
		IMG_BOOL bFilteringMode = IMG_FALSE;
		IMG_UINT32 ui32TruncateMode = 0U;

#if defined(HW_ERN_42290_BIT_MASK) && defined(RGX_FEATURE_TPU_FILTERING_MODE_CONTROL_BIT_MASK)
		if (RGX_IS_ERN_SUPPORTED(psDevInfo, 42290) && RGX_IS_FEATURE_SUPPORTED(psDevInfo, TPU_FILTERING_MODE_CONTROL))
#endif
		{
			bAppHintDefault = PVRSRV_APPHINT_NEWFILTERINGMODE;
			OSGetAppHintBOOL(APPHINT_NO_DEVICE,    pvAppHintState,  NewFilteringMode,
			                   &bAppHintDefault,  &bFilteringMode);
		}

#if defined(HW_ERN_42606_BIT_MASK)
		if (RGX_IS_ERN_SUPPORTED(psDevInfo, 42606))
#endif
		{
			ui32AppHintDefault = PVRSRV_APPHINT_TRUNCATEMODE;
			OSGetAppHintUINT32(APPHINT_NO_DEVICE,  pvAppHintState,  TruncateMode,
			                     &ui32AppHintDefault,  &ui32TruncateMode);
		}

		psHints->ui32FilterFlags = GetFilterFlags(bFilteringMode, ui32TruncateMode);
	}

#if defined(EMULATOR)
#if defined(RGX_FEATURE_AXI_ACELITE_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, AXI_ACELITE))
#endif
	{
		bAppHintDefault = PVRSRV_APPHINT_ENABLETRUSTEDDEVICEACECONFIG;
		OSGetAppHintBOOL(APPHINT_NO_DEVICE,    pvAppHintState,  EnableTrustedDeviceAceConfig,
		                   &bAppHintDefault,     &psHints->bEnableTrustedDeviceAceConfig);
	}
#endif

#if defined(__linux__)
	ui32AppHintDefault = 0;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,  FWContextSwitchCrossDM,
	                     &ui32AppHintDefault,    &psHints->ui32FWContextSwitchCrossDM);
#endif

#if defined(SUPPORT_PHYSMEM_TEST) && !defined(INTEGRITY_OS) && !defined(__QNXNTO__)
	ui32AppHintDefault = PVRSRV_APPHINT_PHYSMEMTESTPASSES;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,      pvAppHintState,  PhysMemTestPasses,
	                     &ui32AppHintDefault,    &psHints->ui32PhysMemTestPasses);
#endif

#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	psHints->ui32AvailablePowUnitsMask = AVAIL_POW_UNITS_MASK_DEFAULT;
	psHints->ui32AvailableRACMask = AVAIL_RAC_MASK_DEFAULT;
#endif /* defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX) */


#if defined(RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX)
	ui32AppHintDefault = PVRSRV_APPHINT_TFBCVERSION;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, TFBCVersionDowngrade,
	 &ui32AppHintDefault, &psHints->ui32TFBCVersion);

	if (ui32AppHintDefault != psHints->ui32TFBCVersion)
	{
		PVR_LOG(("TFBCVersionDowngrade set to %u", psHints->ui32TFBCVersion));
	}

	psHints->bTFBCCompressionControlLossyMinChannel = false;
	psHints->bTFBCCompressionControlYUVFormat = false;
	psHints->ui32TFBCCompressionControlScheme =
	    PVRSRV_APPHINT_TFBCCOMPRESSIONCONTROLSCHEME;
	psHints->ui32TFBCCompressionControlGroup =
	    PVRSRV_APPHINT_TFBCCOMPRESSIONCONTROLGROUP;
#endif /* defined(RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX) */

	ui32AppHintDefault = PVRSRV_APPHINT_DEBUGDUMPFWTLOGTYPE;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,    pvAppHintState,  DebugDumpFWTLogType,
	                     &ui32AppHintDefault,  &psHints->eDebugDumpFWTLogType);
	if ((IMG_UINT32)psHints->eDebugDumpFWTLogType > RGX_FWT_LOGTYPE_PARTIAL)
	{
		psHints->eDebugDumpFWTLogType = RGX_FWT_LOGTYPE_NONE;
		PVR_DPF((PVR_DBG_WARNING, "Invalid value for DebugDumpFWTLogType. Setting to 0 (disabled)."));
	}

	/*
	 * FW logs apphints
	 */
	{
		IMG_UINT32 ui32LogGroup, ui32TraceOrTBI;

		ui32AppHintDefault = PVRSRV_APPHINT_ENABLELOGGROUP;
		OSGetAppHintUINT32(psDevInfo->psDeviceNode, pvAppHintState, EnableLogGroup, &ui32AppHintDefault, &ui32LogGroup);
		ui32AppHintDefault = PVRSRV_APPHINT_FIRMWARELOGTYPE;
		OSGetAppHintUINT32(psDevInfo->psDeviceNode, pvAppHintState, FirmwareLogType, &ui32AppHintDefault, &ui32TraceOrTBI);

		/* Defaulting to TRACE */
		BITMASK_SET(ui32LogGroup, RGXFWIF_LOG_TYPE_TRACE);

#if defined(SUPPORT_TBI_INTERFACE)
		if (ui32TraceOrTBI == 1 /* TBI */)
		{
			if ((ui32LogGroup & RGXFWIF_LOG_TYPE_GROUP_MASK) == 0)
			{
				/* No groups configured - defaulting to MAIN group */
				BITMASK_SET(ui32LogGroup, RGXFWIF_LOG_TYPE_GROUP_MAIN);
			}
			BITMASK_UNSET(ui32LogGroup, RGXFWIF_LOG_TYPE_TRACE);
		}
#endif
		psHints->ui32LogType = ui32LogGroup;
	}

	_ParseHTBAppHints(psDevInfo->psDeviceNode, pvAppHintState);

	InitDeviceFlags(psDevInfo->psDeviceNode, pvAppHintState, psHints);

	OSFreeAppHintState(pvAppHintState);
}


/*!
*******************************************************************************

 @Function      GetFWConfigFlags

 @Description   Initialise and return FW config flags

 @Input         psHints            : Apphints container
 @Input         pui32FWConfigFlags : Pointer to config flags

 @Return        void

******************************************************************************/
static INLINE void GetFWConfigFlags(PVRSRV_DEVICE_NODE *psDeviceNode,
                                    RGX_INIT_APPHINTS *psHints,
                                    IMG_UINT32 *pui32FWConfigFlags,
                                    IMG_UINT32 *pui32FWConfigFlagsExt,
                                    IMG_UINT32 *pui32FwOsCfgFlags)
{
	IMG_UINT32 ui32FWConfigFlags = 0;
	IMG_UINT32 ui32FWConfigFlagsExt = 0;
	IMG_UINT32 ui32FwOsCfgFlags = psHints->ui32FWContextSwitchCrossDM |
	                              (psHints->ui32EnableFWContextSwitch & ~RGXFWIF_INICFG_OS_CTXSWITCH_CLRMSK);

	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
#if defined(RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX)
	IMG_UINT32 ui32FWConfigFlagsSupValExt = 0;
	IMG_UINT32 ui32TFBCVersion = 0U;
#endif

	ui32FWConfigFlags |= psHints->bAssertOnOutOfMem ? RGXFWIF_INICFG_ASSERT_ON_OUTOFMEMORY : 0;
	ui32FWConfigFlags |= psHints->bAssertOnHWRTrigger ? RGXFWIF_INICFG_ASSERT_ON_HWR_TRIGGER : 0;
	ui32FWConfigFlags |= psHints->bCheckMlist ? RGXFWIF_INICFG_CHECK_MLIST_EN : 0;
	ui32FWConfigFlags |= psHints->bDisableClockGating ? RGXFWIF_INICFG_DISABLE_CLKGATING_EN : 0;

#if defined(RGX_FEATURE_PIPELINED_DATAMASTERS_VERSION_MAX_VALUE_IDX)
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, PIPELINED_DATAMASTERS_VERSION)  &&
		(RGX_GET_FEATURE_VALUE(psDevInfo, PIPELINED_DATAMASTERS_VERSION) > 0) &&
		!RGX_IS_FEATURE_SUPPORTED(psDevInfo, ERYX_TOP_INFRASTRUCTURE))
	{
		/* Pipeline DM roadblocks are currently enabled pre-Eryx. */
		ui32FWConfigFlags |= RGXFWIF_INICFG_DM_PIPELINE_ROADBLOCKS_EN;
	}
#endif

	ui32FWConfigFlags |= psHints->bDisableDMOverlap ? RGXFWIF_INICFG_DISABLE_DM_OVERLAP : 0;

	if ((RGX_GET_FEATURE_VALUE(psDevInfo, SLC_SIZE_IN_KILOBYTES) <= 2))
	{
		ui32FWConfigFlags |= RGXFWIF_INICFG_DISABLE_DM_OVERLAP;
	}

	ui32FWConfigFlags |= psHints->bDisablePDP ? RGXFWIF_INICFG_DISABLE_PDP_EN : 0;
	ui32FWConfigFlags |= psHints->bEnableDMKillRand ? RGXFWIF_INICFG_DM_KILL_MODE_RAND_EN : 0;
	ui32FWConfigFlags |= psHints->bEnableRandomCsw ? RGXFWIF_INICFG_CTXSWITCH_MODE_RAND : 0;
	ui32FWConfigFlags |= psHints->bEnableSoftResetCsw ? RGXFWIF_INICFG_CTXSWITCH_SRESET_EN : 0;
	ui32FWConfigFlags |= (psHints->ui32HWPerfFilter0 != 0 || psHints->ui32HWPerfFilter1 != 0) ? RGXFWIF_INICFG_HWPERF_EN : 0;
	ui32FWConfigFlags |= psHints->bHWPerfDisableCounterFilter ? RGXFWIF_INICFG_HWP_DISABLE_FILTER : 0;
	ui32FWConfigFlags |= (psHints->ui32FWContextSwitchProfile << RGXFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT) & RGXFWIF_INICFG_CTXSWITCH_PROFILE_MASK;
#if defined(PVR_ARCH_VOLCANIC)
	ui32FWConfigFlags |= (psHints->ui32ISPSchedulingLatencyMode << RGXFWIF_INICFG_ISPSCHEDMODE_SHIFT) & RGXFWIF_INICFG_ISPSCHEDMODE_MASK;
#endif

	{
		ui32FWConfigFlags |= psDeviceNode->pfnHasFBCDCVersion31(psDeviceNode) ? RGXFWIF_INICFG_FBCDC_V3_1_EN : 0;
	}


#if defined(RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX)

	/* Determine if we need to present a TFBC v1.0, v1.1 or native
	 * behaviour. For V1.0 we need to set the following features:
	 *  TFBCCompressionControlLossyMinChannel = 0x1
	 *  TFBCCompressionControlYUVFormat = 0x1
	 *  TFBCCompressionControlScheme = 0x2
	 *  TFBCCompressionControlGroup = 0x0
	 * For V1.1 we need to set the following:
	 *  TFBCCompressionControlLossyMinChannel = 0x1
	 *  TFBCCompressionControlYUVFormat = 0x0
	 *  TFBCCompressionControlScheme = 0x1
	 *  TFBCCompressionControlGroup = 0 / 1 (depends on LOSSY_37_PERCENT)
	 * The gating for these values depends on whether the GPU supports
	 * RGX_FEATURE_TFBC_VERSION = 20U
	 */

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, TFBC_VERSION))
	{
		ui32TFBCVersion = RGX_GET_FEATURE_VALUE(psDevInfo, TFBC_VERSION);

		if (ui32TFBCVersion >= 20U)
		{
			switch (psHints->ui32TFBCVersion) {
				case 10:	/* TFBC Version 1.0 */
					psHints->bTFBCCompressionControlLossyMinChannel = true;
					psHints->bTFBCCompressionControlYUVFormat = true;
					psHints->ui32TFBCCompressionControlScheme = 2U;
					psHints->ui32TFBCCompressionControlGroup = 0U;

#if defined(DEBUG)
					PVR_LOG(("%s: Setting TFBC Version 1.0, Native v%u",
					         __func__, ui32TFBCVersion));
#endif
					break;

				case 11:	/* TFBC Version 1.1 */
					psHints->bTFBCCompressionControlLossyMinChannel = true;
					psHints->bTFBCCompressionControlYUVFormat = false;
					psHints->ui32TFBCCompressionControlScheme = 1U;
					psHints->ui32TFBCCompressionControlGroup =
					    PVRSRV_APPHINT_TFBCCOMPRESSIONCONTROLGROUP;

#if defined(DEBUG)
					PVR_LOG(("%s: Setting TFBC Version 1.1, Native v%u",
					         __func__, ui32TFBCVersion));
#endif
					break;

				case 0:		/* Leave with whatever the ui32TFBCVersion is */
					break;
				default:	/* Unexpected / unsupported value */
					PVR_DPF((PVR_DBG_WARNING,
					         "%s: Unexpected TFBC Version %u"
					         " Ignoring. Using value %u instead",
					         __func__, psHints->ui32TFBCVersion,
					         ui32TFBCVersion));
					break;
			}

			ui32FWConfigFlagsExt |=
				((((psHints->ui32TFBCCompressionControlGroup  << RGX_CR_TFBC_COMPRESSION_CONTROL_GROUP_CONTROL_SHIFT) &
															    ~RGX_CR_TFBC_COMPRESSION_CONTROL_GROUP_CONTROL_CLRMSK) |
				  ((psHints->ui32TFBCCompressionControlScheme << RGX_CR_TFBC_COMPRESSION_CONTROL_SCHEME_SHIFT) &
															    ~RGX_CR_TFBC_COMPRESSION_CONTROL_SCHEME_CLRMSK) |
				  ((psHints->bTFBCCompressionControlYUVFormat) ? RGX_CR_TFBC_COMPRESSION_CONTROL_YUV10_OVERRIDE_EN : 0) |
				  ((psHints->bTFBCCompressionControlLossyMinChannel) ? RGX_CR_TFBC_COMPRESSION_CONTROL_LOSSY_MIN_CHANNEL_OVERRIDE_EN : 0))
				<< RGXFWIF_INICFG_EXT_TFBC_CONTROL_SHIFT) & RGXFWIF_INICFG_EXT_TFBC_CONTROL_MASK;
			/* Save the CompressionControlGroup for later use by
			 * ->pfnGetTFBCLossyGroup() */
#if defined(RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK)
			psDevInfo->ui32TFBCLossyGroup = psHints->ui32TFBCCompressionControlGroup;
#endif
		}
		else if (ui32TFBCVersion == 11U)
		{
			switch (psHints->ui32TFBCVersion) {
				case 10:	/* TFBC Version 1.0 */
					psHints->bTFBCCompressionControlLossyMinChannel = true;
					psHints->bTFBCCompressionControlYUVFormat = true;
					psHints->ui32TFBCCompressionControlScheme = 2U;
					psHints->ui32TFBCCompressionControlGroup = 0U;

#if defined(DEBUG)
					PVR_LOG(("%s: Setting TFBC Version 1.0, Native v%u",
					         __func__, ui32TFBCVersion));
#endif
					break;

				case 0:		/* Leave with whatever the ui32TFBCVersion is */
					break;

				default:	/* Unexpected / unsupported value */
					PVR_DPF((PVR_DBG_WARNING,
					         "%s: Unexpected TFBC Version %u"
				             " Ignoring. Using value %u instead",
				             __func__, psHints->ui32TFBCVersion,
				             ui32TFBCVersion));
					break;
			}
			ui32FWConfigFlagsExt |=
				((((psHints->ui32TFBCCompressionControlGroup  << RGX_CR_TFBC_COMPRESSION_CONTROL_GROUP_CONTROL_SHIFT) &
															    ~RGX_CR_TFBC_COMPRESSION_CONTROL_GROUP_CONTROL_CLRMSK) |
				  ((psHints->ui32TFBCCompressionControlScheme << RGX_CR_TFBC_COMPRESSION_CONTROL_SCHEME_SHIFT) &
															    ~RGX_CR_TFBC_COMPRESSION_CONTROL_SCHEME_CLRMSK) |
				  ((psHints->bTFBCCompressionControlYUVFormat) ? RGX_CR_TFBC_COMPRESSION_CONTROL_YUV10_OVERRIDE_EN : 0) |
				  ((psHints->bTFBCCompressionControlLossyMinChannel) ? RGX_CR_TFBC_COMPRESSION_CONTROL_LOSSY_MIN_CHANNEL_OVERRIDE_EN : 0))
				<< RGXFWIF_INICFG_EXT_TFBC_CONTROL_SHIFT) & RGXFWIF_INICFG_EXT_TFBC_CONTROL_MASK;
			/* Save the CompressionControlGroup for later use by
			 * ->pfnGetTFBCLossyGroup() */
#if defined(RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK)
			psDevInfo->ui32TFBCLossyGroup = psHints->ui32TFBCCompressionControlGroup;
#endif
		}
		else	/* TFBC v1.0 */
		{
			PVR_UNREFERENCED_PARAMETER(ui32FWConfigFlagsSupValExt);
#if defined(RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK)
			psDevInfo->ui32TFBCLossyGroup = 0;
#endif
			if ((psHints->ui32TFBCVersion != 0U) &&
			    (psHints->ui32TFBCVersion != ui32TFBCVersion))
			{
				PVR_DPF((PVR_DBG_WARNING,
				        "%s: Cannot specify TFBC version %u"
				         " on a version %u GPU core", __func__,
				         psHints->ui32TFBCVersion, ui32TFBCVersion));
			}
		}
	}
#endif /* defined(RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX) */

	*pui32FWConfigFlags    = ui32FWConfigFlags;
	*pui32FWConfigFlagsExt = ui32FWConfigFlagsExt;
	*pui32FwOsCfgFlags     = ui32FwOsCfgFlags;
}


#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
/*!
*******************************************************************************

 @Function      RGXTDProcessFWImage

 @Description   Fetch and send data used by the trusted device to complete
                the FW image setup

 @Input         psDeviceNode : Device node
 @Input         psFWParams : Firmware and parameters used by the FW

 @Return        PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXTDProcessFWImage(PVRSRV_DEVICE_NODE *psDeviceNode,
                                        PVRSRV_FW_PARAMS *psFWParams)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDeviceNode->psDevConfig;
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
#endif
	PVRSRV_ERROR eError;

	if (psDevConfig->pfnTDSendFWImage == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: TDSendFWImage not implemented!", __func__));
		return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		if (psFWParams->uFWP.sMips.ui32FWPageTableNumPages > TD_MAX_NUM_MIPS_PAGETABLE_PAGES)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Number of page table pages %u greater "
					 "than what is allowed by the TD interface (%u), FW might "
					 "not work properly!", __func__,
					 psFWParams->uFWP.sMips.ui32FWPageTableNumPages,
					 TD_MAX_NUM_MIPS_PAGETABLE_PAGES));
		}
	}
#endif

	eError = psDevConfig->pfnTDSendFWImage(psDevConfig->hSysData, psFWParams);

	return eError;
}
#endif

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
/*!
*******************************************************************************

 @Function      RGXAcquireMipsBootldrData

 @Description   Acquire MIPS bootloader data parameters

 @Input         psDeviceNode : Device node
 @Input         puFWParams   : FW boot parameters

 @Return        PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR RGXAcquireMipsBootldrData(PVRSRV_DEVICE_NODE *psDeviceNode,
                                              PVRSRV_FW_BOOT_PARAMS *puFWParams)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO*) psDeviceNode->pvDevice;
	MMU_DEVICEATTRIBS *psFWMMUDevAttrs = psDevInfo->psDeviceNode->psFirmwareMMUDevAttrs;
	IMG_DEV_PHYADDR sAddr;
	IMG_UINT32 ui32PTSize, i;
	PVRSRV_ERROR eError;
	IMG_BOOL bValid;

	/* Rogue Registers physical address */
#if defined(SUPPORT_ALT_REGBASE)
	puFWParams->sMips.sGPURegAddr = psDeviceNode->psDevConfig->sAltRegsGpuPBase;
#else
	PhysHeapCpuPAddrToDevPAddr(psDevInfo->psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_GPU_LOCAL],
	                           1,
	                           &puFWParams->sMips.sGPURegAddr,
	                           &(psDeviceNode->psDevConfig->sRegsCpuPBase));
#endif

	/* MIPS Page Table physical address */
	MMU_AcquireBaseAddr(psDevInfo->psKernelMMUCtx, &sAddr);

	/* MIPS Page Table allocation is contiguous. Pass one or more addresses
	 * to the FW depending on the Page Table size and alignment. */

	ui32PTSize = (psFWMMUDevAttrs->psTopLevelDevVAddrConfig->uiNumEntriesPT)
	             << RGXMIPSFW_LOG2_PTE_ENTRY_SIZE;
	ui32PTSize = PVR_ALIGN(ui32PTSize, 1U << psFWMMUDevAttrs->ui32BaseAlign);

	puFWParams->sMips.ui32FWPageTableLog2PageSize = psFWMMUDevAttrs->ui32BaseAlign;
	puFWParams->sMips.ui32FWPageTableNumPages = ui32PTSize >> psFWMMUDevAttrs->ui32BaseAlign;

	if (puFWParams->sMips.ui32FWPageTableNumPages > 4U)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Page table cannot be mapped by the FW "
		         "(size 0x%x, log2 page size %u, %u pages)",
		         __func__, ui32PTSize, puFWParams->sMips.ui32FWPageTableLog2PageSize,
		         puFWParams->sMips.ui32FWPageTableNumPages));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	/* Confirm page alignment fits in 64-bits */
	if (psFWMMUDevAttrs->ui32BaseAlign > 63)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid page alignment "
		         "(psFWMMUDevAttrs->ui32BaseAlign = %u)",
		         __func__, psFWMMUDevAttrs->ui32BaseAlign));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	for (i = 0; i < puFWParams->sMips.ui32FWPageTableNumPages; i++)
	{
		puFWParams->sMips.asFWPageTableAddr[i].uiAddr =
		    sAddr.uiAddr + i * (1ULL << psFWMMUDevAttrs->ui32BaseAlign);
	}

	/* MIPS Stack Pointer Physical Address */
	eError = RGXGetPhyAddr(psDevInfo->psRGXFWDataMemDesc->psImport->hPMR,
	                       &puFWParams->sMips.sFWStackAddr,
	                       RGXGetFWImageSectionOffset(NULL, MIPS_STACK),
	                       OSGetPageShift(),
	                       1,
	                       &bValid);

	return eError;
}
#endif

/*!
*******************************************************************************

 @Function      InitFirmware

 @Description   Allocate, initialise and pdump Firmware code and data memory

 @Input         psDeviceNode : Device Node

 @Return        PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR InitFirmware(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	OS_FW_IMAGE       *psRGXFW = NULL;
	PVRSRV_FW_PARAMS  sFWParams = {0};

	/* FW code memory */
	IMG_DEVMEM_SIZE_T uiFWCodeAllocSize;
	void              *pvFWCodeHostAddr;

	/* FW data memory */
	IMG_DEVMEM_SIZE_T uiFWDataAllocSize;
	void              *pvFWDataHostAddr;

	/* FW coremem code memory */
	IMG_DEVMEM_SIZE_T uiFWCorememCodeAllocSize;
	void              *pvFWCorememCodeHostAddr = NULL;

	/* FW coremem data memory */
	IMG_DEVMEM_SIZE_T uiFWCorememDataAllocSize;
	void              *pvFWCorememDataHostAddr = NULL;

	PVRSRV_FW_BOOT_PARAMS uFWParams;
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	IMG_BOOL bUseSecureFWData =
								RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META) ||
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	                            (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) &&
	                             RGX_GET_FEATURE_VALUE(psDevInfo, PHYS_BUS_WIDTH) > 32) ||
#endif
	                            RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR);
#endif

	/*
	 * Get pointer to Firmware image
	 */
	eError = RGXLoadAndGetFWData(psDeviceNode, &psRGXFW);

	if (eError != PVRSRV_OK)
	{
		/* Error or confirmation message generated in RGXLoadAndGetFWData */
		goto fw_load_fail;
	}

	/*
	 * Get pointer and size
	 */
	sFWParams.pvFirmware       = OSFirmwareData(psRGXFW);
	sFWParams.ui32FirmwareSize = OSFirmwareSize(psRGXFW);

	/*
	 * Allow it to be pre-processed by the platform hook
	 */
	if (psDeviceNode->psDevConfig->pfnPrepareFWImage != NULL)
	{
		eError = psDeviceNode->psDevConfig->pfnPrepareFWImage(
				psDeviceNode->psDevConfig->hSysData, &sFWParams);
		if (eError != PVRSRV_OK)
			goto cleanup_initfw;
	}

	/*
	 * Allocate Firmware memory
	 */

	eError = RGXGetFWImageAllocSize(&psDevInfo->sLayerParams,
	                                sFWParams.pvFirmware,
	                                sFWParams.ui32FirmwareSize,
	                                &uiFWCodeAllocSize,
	                                &uiFWDataAllocSize,
	                                &uiFWCorememCodeAllocSize,
	                                &uiFWCorememDataAllocSize,
	                                &psDevInfo->sFWInfoHeader);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: RGXGetFWImageAllocSize failed",
		        __func__));
		goto cleanup_initfw;
	}

	/*
	 * Initiate FW compatibility check for Native and Host.
	 * Guest compatibility check must be done after FW boot.
	 */
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = PVRSRVDevInitCompatCheck(psDeviceNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed compatibility check for device %p (%s)",
					 __func__, psDeviceNode, PVRSRVGetErrorString(eError)));
			goto cleanup_initfw;
		}
	}

	psDevInfo->ui32FWCodeSizeInBytes = uiFWCodeAllocSize;

#if defined(SUPPORT_TRUSTED_DEVICE)
    PVR_DPF((PVR_DBG_WARNING,
            "%s: META DMA not available, disabling core memory code/data",
            __func__));
    uiFWCorememCodeAllocSize = 0;
    uiFWCorememDataAllocSize = 0;
#endif

	psDevInfo->ui32FWCorememCodeSizeInBytes = uiFWCorememCodeAllocSize;

	eError = RGXInitAllocFWImgMem(psDeviceNode,
	                              uiFWCodeAllocSize,
	                              uiFWDataAllocSize,
	                              uiFWCorememCodeAllocSize,
	                              uiFWCorememDataAllocSize);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: RGXInitAllocFWImgMem failed (%d)",
		        __func__,
		        eError));
		goto cleanup_initfw;
	}

	/*
	 * Acquire pointers to Firmware allocations
	 */

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (bUseSecureFWData)
	{
		/* We can't get a pointer to a secure FW allocation from within the DDK */
		pvFWCodeHostAddr = NULL;
	}
	else
#endif
	{
		eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWCodeMemDesc, &pvFWCodeHostAddr);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", cleanup_initfw);
	}

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (bUseSecureFWData)
	{
		/* We can't get a pointer to a secure FW allocation from within the DDK */
		pvFWDataHostAddr = NULL;
	}
	else
#endif
	{
		eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWDataMemDesc, &pvFWDataHostAddr);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", release_code);
	}

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (bUseSecureFWData)
	{
		/* We can't get a pointer to a secure FW allocation from within the DDK */
		pvFWCorememCodeHostAddr = NULL;
	}
	else
#endif
	if (uiFWCorememCodeAllocSize != 0)
	{
		eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWCorememCodeMemDesc, &pvFWCorememCodeHostAddr);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", release_data);
	}

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (bUseSecureFWData)
	{
		pvFWCorememDataHostAddr = NULL;
	}
	else
#endif
	if (uiFWCorememDataAllocSize != 0)
	{
		eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWIfCorememDataStoreMemDesc, &pvFWCorememDataHostAddr);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", release_corememcode);
	}

	/*
	 * Prepare FW boot parameters
	 */
	OSCachedMemSet(&uFWParams, 0, sizeof(PVRSRV_FW_BOOT_PARAMS));

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
	{
		uFWParams.sMeta.sFWCodeDevVAddr = psDevInfo->sFWCodeDevVAddrBase;
		uFWParams.sMeta.sFWDataDevVAddr = psDevInfo->sFWDataDevVAddrBase;
		uFWParams.sMeta.sFWCorememCodeDevVAddr = psDevInfo->sFWCorememCodeDevVAddrBase;
		uFWParams.sMeta.sFWCorememCodeFWAddr = psDevInfo->sFWCorememCodeFWAddr;
		uFWParams.sMeta.uiFWCorememCodeSize = uiFWCorememCodeAllocSize;
		uFWParams.sMeta.sFWCorememDataDevVAddr = psDevInfo->sFWCorememDataStoreDevVAddrBase;
		uFWParams.sMeta.sFWCorememDataFWAddr = psDevInfo->sFWCorememDataStoreFWAddr;
#if defined(RGXFW_META_SUPPORT_2ND_THREAD)
		uFWParams.sMeta.ui32NumThreads = 2;
#else
		uFWParams.sMeta.ui32NumThreads = 1;
#endif
	}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		eError = RGXAcquireMipsBootldrData(psDeviceNode, &uFWParams);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: RGXAcquireMipsBootldrData failed (%d)",
					 __func__, eError));
			goto release_fw_allocations;
		}
	}
#endif
	else
	{
		uFWParams.sRISCV.sFWCodeDevVAddr = psDevInfo->sFWCodeDevVAddrBase;
		uFWParams.sRISCV.sFWDataDevVAddr = psDevInfo->sFWDataDevVAddrBase;

		uFWParams.sRISCV.sFWCorememCodeDevVAddr = psDevInfo->sFWCorememCodeDevVAddrBase;
		uFWParams.sRISCV.sFWCorememCodeFWAddr   = psDevInfo->sFWCorememCodeFWAddr;
		uFWParams.sRISCV.uiFWCorememCodeSize    = uiFWCorememCodeAllocSize;

		uFWParams.sRISCV.sFWCorememDataDevVAddr = psDevInfo->sFWCorememDataStoreDevVAddrBase;
		uFWParams.sRISCV.sFWCorememDataFWAddr   = psDevInfo->sFWCorememDataStoreFWAddr;
		uFWParams.sRISCV.uiFWCorememDataSize    = uiFWCorememDataAllocSize;
	}


	/*
	 * Process the Firmware image and setup code and data segments.
	 *
	 * When the trusted device is enabled and the FW code lives
	 * in secure memory we will only setup the data segments here,
	 * while the code segments will be loaded to secure memory
	 * by the trusted device.
	 */
	if (!psDeviceNode->bAutoVzFwIsUp)
	{
		eError = RGXProcessFWImage(&psDevInfo->sLayerParams,
								   sFWParams.pvFirmware,
								   pvFWCodeHostAddr,
								   pvFWDataHostAddr,
								   pvFWCorememCodeHostAddr,
								   pvFWCorememDataHostAddr,
								   &uFWParams);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: RGXProcessFWImage failed (%d)",
					 __func__, eError));
			goto release_fw_allocations;
		}
#if !defined(SUPPORT_TRUSTED_DEVICE) || defined(NO_HARDWARE) || defined(SUPPORT_SECURITY_VALIDATION)
		RGXFwSharedMemCacheOpExec(pvFWCodeHostAddr,
		                          sizeof(psDevInfo->psRGXFWCodeMemDesc->uiAllocSize),
		                          PVRSRV_CACHE_OP_FLUSH);
		if (uiFWCorememCodeAllocSize)
		{
			RGXFwSharedMemCacheOpExec(pvFWCorememCodeHostAddr,
			                          sizeof(psDevInfo->psRGXFWCorememCodeMemDesc->uiAllocSize),
			                          PVRSRV_CACHE_OP_FLUSH);
		}

		RGXFwSharedMemCacheOpExec(pvFWDataHostAddr,
		                          sizeof(psDevInfo->psRGXFWDataMemDesc->uiAllocSize),
		                          PVRSRV_CACHE_OP_FLUSH);
		if (uiFWCorememDataAllocSize)
		{
			RGXFwSharedMemCacheOpExec(pvFWCorememDataHostAddr,
			                          sizeof(psDevInfo->psRGXFWIfCorememDataStoreMemDesc->uiAllocSize),
			                          PVRSRV_CACHE_OP_FLUSH);
		}
#endif
	}

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	sFWParams.uFWP = uFWParams;
	RGXTDProcessFWImage(psDeviceNode, &sFWParams);
#endif


	/*
	 * PDump Firmware allocations
	 */

#if !defined(SUPPORT_TRUSTED_DEVICE) || defined(NO_HARDWARE) || defined(SUPPORT_SECURITY_VALIDATION)
	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Dump firmware code image");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWCodeMemDesc,
	                   0,
	                   uiFWCodeAllocSize,
	                   PDUMP_FLAGS_CONTINUOUS);
#endif

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (!bUseSecureFWData)
#endif
	{
		PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
		                      "Dump firmware data image");
		DevmemPDumpLoadMem(psDevInfo->psRGXFWDataMemDesc,
		                   0,
		                   uiFWDataAllocSize,
		                   PDUMP_FLAGS_CONTINUOUS);
	}

#if !defined(SUPPORT_TRUSTED_DEVICE) || defined(NO_HARDWARE) || defined(SUPPORT_SECURITY_VALIDATION)
	if (uiFWCorememCodeAllocSize != 0)
	{
		PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
		                      "Dump firmware coremem code image");
		DevmemPDumpLoadMem(psDevInfo->psRGXFWCorememCodeMemDesc,
						   0,
						   uiFWCorememCodeAllocSize,
						   PDUMP_FLAGS_CONTINUOUS);
	}
#endif

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (!bUseSecureFWData && uiFWCorememDataAllocSize)
#else
	if (uiFWCorememDataAllocSize != 0)
#endif
	{
		PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
		                      "Dump firmware coremem data store image");
		DevmemPDumpLoadMem(psDevInfo->psRGXFWIfCorememDataStoreMemDesc,
						   0,
						   uiFWCorememDataAllocSize,
						   PDUMP_FLAGS_CONTINUOUS);
	}

	/*
	 * Release Firmware allocations and clean up
	 */
release_fw_allocations:
#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (!bUseSecureFWData && uiFWCorememDataAllocSize)
#else
	if (uiFWCorememDataAllocSize != 0)
#endif
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfCorememDataStoreMemDesc);
	}

release_corememcode:
#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (!bUseSecureFWData && (uiFWCorememCodeAllocSize != 0))
#else
	if (uiFWCorememCodeAllocSize != 0)
#endif
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWCorememCodeMemDesc);
	}

release_data:
#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (!bUseSecureFWData)
#endif
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWDataMemDesc);
	}

release_code:
#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (!bUseSecureFWData)
#endif
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWCodeMemDesc);
	}
cleanup_initfw:
	OSUnloadFirmware(psRGXFW);
fw_load_fail:

	return eError;
}


#if defined(PDUMP)
/*!
*******************************************************************************

 @Function      InitialiseHWPerfCounters

 @Description   Initialisation of hardware performance counters and dumping
                them out to pdump, so that they can be modified at a later
                point.

 @Input         pvDevice
 @Input         psHWPerfDataMemDesc
 @Input         psHWPerfInitDataInt

 @Return        void

******************************************************************************/

static void InitialiseHWPerfCounters(PVRSRV_DEVICE_NODE *psDeviceNode,
                                     void *pvDevice,
                                     DEVMEM_MEMDESC *psHWPerfDataMemDesc,
                                     RGXFWIF_HWPERF_CTL *psHWPerfInitDataInt)
{
	RGXFWIF_HWPERF_CTL_BLK *psHWPerfInitBlkData;
#if defined(HWPERF_UNIFIED)
	RGXFWIF_HWPERF_DA_BLK *psHWPerfInitDABlkData;
#endif
	IMG_UINT32 ui32CntBlkModelLen;
	const RGXFW_HWPERF_CNTBLK_TYPE_MODEL *asCntBlkTypeModel;
	const RGXFW_HWPERF_CNTBLK_TYPE_MODEL* psBlkTypeDesc;
	IMG_UINT32 ui32BlockID, ui32BlkCfgIdx, ui32CounterIdx;
	RGX_HWPERF_CNTBLK_RT_INFO sCntBlkRtInfo;

	ui32CntBlkModelLen = RGXGetHWPerfBlockConfig(&asCntBlkTypeModel);

	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "HWPerf Counter Config starts here.");

	for (ui32BlkCfgIdx = 0; ui32BlkCfgIdx < ui32CntBlkModelLen; ui32BlkCfgIdx++)
	{
		__maybe_unused IMG_UINT32 uiUnit;
		__maybe_unused IMG_BOOL bDirect;

		/* Exit early if this core does not have any of these counter blocks
		 * due to core type/BVNC features.... */
		psBlkTypeDesc = &asCntBlkTypeModel[ui32BlkCfgIdx];
		if (psBlkTypeDesc->pfnIsBlkPresent(psBlkTypeDesc, pvDevice, &sCntBlkRtInfo) == IMG_FALSE)
		{
			continue;
		}

		/* Program all counters in one block so those already on may
		 * be configured off and vice-versa. */
		for (ui32BlockID = psBlkTypeDesc->ui32CntBlkIdBase;
			 ui32BlockID < psBlkTypeDesc->ui32CntBlkIdBase+sCntBlkRtInfo.ui32NumUnits;
			 ui32BlockID++)
		{
			PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "Unit %d Block : %s",
			                      ui32BlockID-psBlkTypeDesc->ui32CntBlkIdBase,
			                      psBlkTypeDesc->pszBlockNameComment);

			/* Get the block configure store to update from the global store of
			 * block configuration. This is used to remember the configuration
			 * between configurations and core power on in APM.
			 * For HWPERF_UNIFIED layout we will have a different
			 * structure type to decode the HWPerf block. This is indicated by
			 * the RGX_CNTBLK_ID_DA_MASK bit being set in the block-ID value. */

			bDirect = (psBlkTypeDesc->ui32IndirectReg == 0U);
			uiUnit = ui32BlockID - psBlkTypeDesc->ui32CntBlkIdBase;

#if defined(HWPERF_UNIFIED)
			if ((ui32BlockID & RGX_CNTBLK_ID_DA_MASK) == RGX_CNTBLK_ID_DA_MASK)
			{
				psHWPerfInitDABlkData = rgxfw_hwperf_get_da_block_ctl(ui32BlockID, psHWPerfInitDataInt);

				PVR_ASSERT(psHWPerfInitDABlkData);

				psHWPerfInitDABlkData->eBlockID = ui32BlockID;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "eBlockID: The Block ID for the layout block. See RGX_HWPERF_CNTBLK_ID for further information.");
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitDABlkData->eBlockID) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitDABlkData->eBlockID,
							PDUMP_FLAGS_CONTINUOUS);

				psHWPerfInitDABlkData->uiEnabled = 0U;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "uiEnabled: Set to 0x1 if the block needs to be enabled during playback.");
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitDABlkData->uiEnabled) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitDABlkData->uiEnabled,
							PDUMP_FLAGS_CONTINUOUS);

				psHWPerfInitDABlkData->uiNumCounters = 0U;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "uiNumCounters (X): Specifies the number of valid counters"
			                      " [0..%d] which follow.", RGX_CNTBLK_COUNTERS_MAX);
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitDABlkData->uiNumCounters) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitDABlkData->uiNumCounters,
							PDUMP_FLAGS_CONTINUOUS);

				for (ui32CounterIdx = 0; ui32CounterIdx < RGX_CNTBLK_COUNTERS_MAX; ui32CounterIdx++)
				{
					psHWPerfInitDABlkData->aui32Counters[ui32CounterIdx] = IMG_UINT32_C(0x00000000);

					if (bDirect)
					{
						PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
					                      "%s_COUNTER_%d",
					                      psBlkTypeDesc->pszBlockNameComment,
					                      ui32CounterIdx);
					}
					else
					{
						PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
					                      "%s%d_COUNTER_%d",
					                      psBlkTypeDesc->pszBlockNameComment,
					                      uiUnit, ui32CounterIdx);
					}

					DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitDABlkData->aui32Counters[ui32CounterIdx]) - (size_t)(psHWPerfInitDataInt),
					        psHWPerfInitDABlkData->aui32Counters[ui32CounterIdx],
							PDUMP_FLAGS_CONTINUOUS);
				}
			}
			else
#endif	/* defined(HWPERF_UNIFIED) */
			{
				psHWPerfInitBlkData = rgxfw_hwperf_get_block_ctl(ui32BlockID, psHWPerfInitDataInt);
				/* Assert to check for HWPerf block mis-configuration */
				PVR_ASSERT(psHWPerfInitBlkData);

				psHWPerfInitBlkData->ui32Valid = IMG_TRUE;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "bValid: This specifies if the layout block is valid for the given BVNC.");
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitBlkData->ui32Valid) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitBlkData->ui32Valid,
							PDUMP_FLAGS_CONTINUOUS);

				psHWPerfInitBlkData->ui32Enabled = IMG_FALSE;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "bEnabled: Set to 0x1 if the block needs to be enabled during playback.");
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitBlkData->ui32Enabled) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitBlkData->ui32Enabled,
							PDUMP_FLAGS_CONTINUOUS);

				psHWPerfInitBlkData->eBlockID = ui32BlockID;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "eBlockID: The Block ID for the layout block. See RGX_HWPERF_CNTBLK_ID for further information.");
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitBlkData->eBlockID) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitBlkData->eBlockID,
							PDUMP_FLAGS_CONTINUOUS);

				psHWPerfInitBlkData->uiCounterMask = 0x00;
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "uiCounterMask: Bitmask for selecting the counters that need to be configured. (Bit 0 - counter0, bit 1 - counter1 and so on.)");
				DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitBlkData->uiCounterMask) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitBlkData->uiCounterMask,
							PDUMP_FLAGS_CONTINUOUS);

				for (ui32CounterIdx = RGX_CNTBLK_COUNTER0_ID; ui32CounterIdx < psBlkTypeDesc->ui8NumCounters; ui32CounterIdx++)
				{
					psHWPerfInitBlkData->aui64CounterCfg[ui32CounterIdx] = IMG_UINT64_C(0x0000000000000000);

					PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
				                      "%s_COUNTER_%d", psBlkTypeDesc->pszBlockNameComment, ui32CounterIdx);
					DevmemPDumpLoadMemValue64(psHWPerfDataMemDesc,
							(size_t)&(psHWPerfInitBlkData->aui64CounterCfg[ui32CounterIdx]) - (size_t)(psHWPerfInitDataInt),
							psHWPerfInitBlkData->aui64CounterCfg[ui32CounterIdx],
							PDUMP_FLAGS_CONTINUOUS);

				}
			}
		}
	}
}
/*!
*******************************************************************************

 @Function      InitialiseCustomCounters

 @Description   Initialisation of custom counters and dumping them out to
                pdump, so that they can be modified at a later point.

 @Input         psHWPerfDataMemDesc

 @Return        void

******************************************************************************/

static void InitialiseCustomCounters(PVRSRV_DEVICE_NODE *psDeviceNode,
                                     DEVMEM_MEMDESC *psHWPerfDataMemDesc)
{
	IMG_UINT32 ui32CustomBlock, ui32CounterID;

	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "ui32SelectedCountersBlockMask - The Bitmask of the custom counters that are to be selected");
	DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
						offsetof(RGXFWIF_HWPERF_CTL, ui32SelectedCountersBlockMask),
						0,
						PDUMP_FLAGS_CONTINUOUS);

	for (ui32CustomBlock = 0; ui32CustomBlock < RGX_HWPERF_MAX_CUSTOM_BLKS; ui32CustomBlock++)
	{
		/*
		 * Some compilers cannot cope with the use of offsetof() below - the specific problem being the use of
		 * a non-const variable in the expression, which it needs to be const. Typical compiler error produced is
		 * "expression must have a constant value".
		 */
		const IMG_DEVMEM_OFFSET_T uiOffsetOfCustomBlockSelectedCounters
		= (IMG_DEVMEM_OFFSET_T)(uintptr_t)&(((RGXFWIF_HWPERF_CTL *)0)->SelCntr[ui32CustomBlock].ui32NumSelectedCounters);

		PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
		                      "ui32NumSelectedCounters - The Number of counters selected for this Custom Block: %d",ui32CustomBlock );
		DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
					uiOffsetOfCustomBlockSelectedCounters,
					0,
					PDUMP_FLAGS_CONTINUOUS);

		for (ui32CounterID = 0; ui32CounterID < RGX_HWPERF_MAX_CUSTOM_CNTRS; ui32CounterID++ )
		{
			const IMG_DEVMEM_OFFSET_T uiOffsetOfCustomBlockSelectedCounterIDs
			= (IMG_DEVMEM_OFFSET_T)(uintptr_t)&(((RGXFWIF_HWPERF_CTL *)0)->SelCntr[ui32CustomBlock].aui32SelectedCountersIDs[ui32CounterID]);

			PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
			                      "CUSTOMBLK_%d_COUNTERID_%d",ui32CustomBlock, ui32CounterID);
			DevmemPDumpLoadMemValue32(psHWPerfDataMemDesc,
					uiOffsetOfCustomBlockSelectedCounterIDs,
					0,
					PDUMP_FLAGS_CONTINUOUS);
		}
	}
}

/*!
*******************************************************************************

 @Function      InitialiseAllCounters

 @Description   Initialise HWPerf and custom counters

 @Input         psDeviceNode : Device Node

 @Return        PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR InitialiseAllCounters(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;
	RGXFWIF_HWPERF_CTL *psHWPerfInitData;
	PVRSRV_ERROR eError;

	eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWIfHWPerfCountersMemDesc, (void **)&psHWPerfInitData);
	PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireCpuVirtAddr", failHWPerfCountersMemDescAqCpuVirt);

	InitialiseHWPerfCounters(psDeviceNode, psDevInfo, psDevInfo->psRGXFWIfHWPerfCountersMemDesc, psHWPerfInitData);
	InitialiseCustomCounters(psDeviceNode, psDevInfo->psRGXFWIfHWPerfCountersMemDesc);
	RGXFwSharedMemCacheOpPtr(psHWPerfInitData, FLUSH);

failHWPerfCountersMemDescAqCpuVirt:
	DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfHWPerfCountersMemDesc);

	return eError;
}
#endif /* PDUMP */

#if defined(SUPPORT_TRUSTED_DEVICE)
static PVRSRV_ERROR RGXValidateTDHeap(PVRSRV_DEVICE_NODE *psDeviceNode,
									  PVRSRV_PHYS_HEAP ePhysHeap,
									  PHYS_HEAP_USAGE_FLAGS ui32RequiredFlags)
{
	PHYS_HEAP *psHeap = psDeviceNode->apsPhysHeap[ePhysHeap];
	PHYS_HEAP_USAGE_FLAGS ui32HeapFlags = PhysHeapGetFlags(psHeap);
	PHYS_HEAP_USAGE_FLAGS ui32InvalidFlags = ~(PHYS_HEAP_USAGE_FW_PRIV_DATA | PHYS_HEAP_USAGE_FW_CODE
											   | PHYS_HEAP_USAGE_GPU_SECURE | PHYS_HEAP_USAGE_FW_PRIVATE);

	PVR_LOG_RETURN_IF_FALSE_VA((ui32HeapFlags & ui32RequiredFlags) != 0,
							   PVRSRV_ERROR_NOT_SUPPORTED,
							   "TD heap is missing required flags. flags: 0x%x / required:0x%x",
							   ui32HeapFlags,
							   ui32RequiredFlags);

	PVR_LOG_RETURN_IF_FALSE_VA((ui32HeapFlags & ui32InvalidFlags) == 0,
							   PVRSRV_ERROR_NOT_SUPPORTED,
							   "TD heap uses invalid flags. flags: 0x%x / invalid:0x%x",
							   ui32HeapFlags,
							   ui32InvalidFlags);

	return PVRSRV_OK;
}

static PVRSRV_ERROR RGXValidateTDHeaps(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = RGXValidateTDHeap(psDeviceNode, PVRSRV_PHYS_HEAP_FW_PRIV_DATA, PHYS_HEAP_USAGE_FW_PRIV_DATA);
		PVR_LOG_RETURN_IF_ERROR(eError, "RGXValidateTDHeap:FW_PRIV_DATA");

		eError = RGXValidateTDHeap(psDeviceNode, PVRSRV_PHYS_HEAP_FW_CODE, PHYS_HEAP_USAGE_FW_CODE);
		PVR_LOG_RETURN_IF_ERROR(eError, "RGXValidateTDHeap:FW_CODE");
	}

	eError = RGXValidateTDHeap(psDeviceNode, PVRSRV_PHYS_HEAP_GPU_SECURE, PHYS_HEAP_USAGE_GPU_SECURE);
	PVR_LOG_RETURN_IF_ERROR(eError, "RGXValidateTDHeap:GPU_SECURE");

	return PVRSRV_OK;
}
#endif

/*!
*******************************************************************************

 @Function      RGXInit

 @Description   RGX Initialisation

 @Input         psDeviceNode

 @Return        PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RGXInit(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;

	/* Services initialisation parameters */
	RGX_INIT_APPHINTS sApphints = {0};
	IMG_UINT32 ui32FWConfigFlags, ui32FWConfigFlagsExt, ui32FwOsCfgFlags;

	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;

	PDUMPCOMMENT(psDeviceNode, "RGX Initialisation Part 1");

	PDUMPCOMMENT(psDeviceNode, "Device Name: %s",
	             psDeviceNode->psDevConfig->pszName);
	PDUMPCOMMENT(psDeviceNode, "Device ID: %u (%d)",
	             psDeviceNode->sDevId.ui32InternalID,
	             psDeviceNode->sDevId.i32KernelDeviceID);

	if (psDeviceNode->psDevConfig->pszVersion)
	{
		PDUMPCOMMENT(psDeviceNode, "Device Version: %s",
		             psDeviceNode->psDevConfig->pszVersion);
	}

	/* pdump info about the core */
	PDUMPCOMMENT(psDeviceNode,
	             "RGX Version Information (KM): %d.%d.%d.%d",
	             psDevInfo->sDevFeatureCfg.ui32B,
	             psDevInfo->sDevFeatureCfg.ui32V,
	             psDevInfo->sDevFeatureCfg.ui32N,
	             psDevInfo->sDevFeatureCfg.ui32C);

#if defined(SUPPORT_TRUSTED_DEVICE)
	eError = RGXValidateTDHeaps(psDeviceNode);
	PVR_LOG_RETURN_IF_ERROR(eError, "RGXValidateTDHeaps");
#endif

#if defined(SUPPORT_AUTOVZ)
	if (PVRSRV_VZ_MODE_IS(HOST, DEVNODE, psDeviceNode))
	{
		/* The RGX_CR_MTS_DM4_INTERRUPT_ENABLE register is always set by the firmware during initialisation
		 * and it provides a good method of determining if the firmware has been booted previously */
		psDeviceNode->bAutoVzFwIsUp = (OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MTS_DM4_INTERRUPT_ENABLE) != 0);

		PVR_LOG(("AutoVz startup check: firmware is %s;",
				(psDeviceNode->bAutoVzFwIsUp) ? "already running" : "powered down"));
		PVR_LOG(("AutoVz allow GPU powerdown is %s:",
				(psDeviceNode->bAutoVzAllowGPUPowerdown) ? "enabled" : "disabled"));
	}
	else if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Guest assumes the firmware is always available */
		psDeviceNode->bAutoVzFwIsUp = IMG_TRUE;
		psDeviceNode->bAutoVzAllowGPUPowerdown = IMG_FALSE;
	}
	else
#endif
	{
		/* Firmware does not follow the AutoVz life-cycle */
		psDeviceNode->bAutoVzFwIsUp = IMG_FALSE;
	}

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) || (psDeviceNode->bAutoVzFwIsUp))
	{
		/* set the device power state here as the regular power
		 * callbacks will not be executed on this driver */
		psDevInfo->bRGXPowered = IMG_TRUE;
	}

	/* Set which HW Safety Events will be handled by the driver */
	psDevInfo->ui32HostSafetyEventMask |= RGX_IS_FEATURE_SUPPORTED(psDevInfo, WATCHDOG_TIMER) ?
										  RGX_CR_SAFETY_EVENT_STATUS__ROGUEXE__WATCHDOG_TIMEOUT_EN : 0;
	psDevInfo->ui32HostSafetyEventMask |= (RGX_DEVICE_HAS_FEATURE_VALUE(&psDevInfo->sLayerParams, ECC_RAMS)
										   && (RGX_DEVICE_GET_FEATURE_VALUE(&psDevInfo->sLayerParams, ECC_RAMS) > 0)) ?
										  RGX_CR_SAFETY_EVENT_STATUS__ROGUEXE__FAULT_FW_EN : 0;

#if defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Register defs revision: %d", RGX_CR_DEFS_KM_REVISION);
#endif

	/* Services initialisation parameters */
	GetApphints(psDevInfo, &sApphints);

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#if defined(EMULATOR)
	if ((sApphints.bEnableTrustedDeviceAceConfig) &&
		(RGX_IS_FEATURE_SUPPORTED(psDevInfo, AXI_ACELITE)))
	{
		SetTrustedDeviceAceEnabled(psDeviceNode->psDevConfig->hSysData);
	}
#endif
#endif

	eError = RGXInitCreateFWKernelMemoryContext(psDeviceNode);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create FW kernel memory context (%u)",
		         __func__, eError));
		goto cleanup;
	}

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = InitFirmware(psDeviceNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: InitFirmware failed (%d)",
					 __func__, eError));
			goto cleanup;
		}
	}

	/*
	 * Setup Firmware initialisation data
	 */

	GetFWConfigFlags(psDeviceNode, &sApphints, &ui32FWConfigFlags, &ui32FWConfigFlagsExt, &ui32FwOsCfgFlags);

	eError = RGXInitFirmware(psDeviceNode,
	                         &sApphints,
	                         ui32FWConfigFlags,
	                         ui32FWConfigFlagsExt,
	                         ui32FwOsCfgFlags);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: RGXInitFirmware failed (%d)",
				 __func__, eError));
		goto cleanup;
	}

#if defined(PDUMP)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = InitialiseAllCounters(psDeviceNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: InitialiseAllCounters failed (%d)",
					 __func__, eError));
			goto cleanup;
		}
	}
#endif

	/*
	 * Perform second stage of RGX initialisation
	 */
	eError = RGXInitDevPart2(psDeviceNode,
	                         &sApphints);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: RGXInitDevPart2 failed (%d)",
				 __func__, eError));
		goto cleanup;
	}


	eError = PVRSRV_OK;

cleanup:
	return eError;
}

/******************************************************************************
 End of file (rgxsrvinit.c)
******************************************************************************/

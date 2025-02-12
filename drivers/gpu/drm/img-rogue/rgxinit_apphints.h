/*************************************************************************/ /*!
@File
@Title          Services device initialisation settings
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device initialisation settings
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

#if !defined(RGXINIT_APPHINTS_H)
#define RGXINIT_APPHINTS_H

#include "img_defs.h"

#include "rgx_fwif_km.h"
#include "rgxdefs_km.h"
#include "rgxdevice.h"

/*
 * Container for all the apphints used by this module
 */
typedef struct _RGX_INIT_APPHINTS_
{
	IMG_BOOL   bEnableSignatureChecks;
	IMG_UINT32 ui32SignatureChecksBufSize;

	IMG_BOOL   bAssertOnOutOfMem;
	IMG_BOOL   bAssertOnHWRTrigger;
#if defined(RGX_FEATURE_TFBC_VERSION_MAX_VALUE_IDX)
	IMG_UINT32 ui32TFBCVersion;
	IMG_UINT32 ui32TFBCCompressionControlGroup;
	IMG_UINT32 ui32TFBCCompressionControlScheme;
	IMG_BOOL   bTFBCCompressionControlYUVFormat;
	IMG_BOOL   bTFBCCompressionControlLossyMinChannel;
#endif
	IMG_BOOL   bCheckMlist;
	IMG_BOOL   bDisableClockGating;
	IMG_BOOL   bDisableDMOverlap;
	IMG_BOOL   bDisablePDP;
	IMG_BOOL   bEnableDMKillRand;
	IMG_BOOL   bEnableRandomCsw;
	IMG_BOOL   bEnableSoftResetCsw;
	IMG_BOOL   bHWPerfDisableCounterFilter;
	IMG_UINT32 ui32DeviceFlags;
	IMG_UINT32 ui32FilterFlags;
	IMG_UINT32 ui32EnableFWContextSwitch;
	IMG_UINT32 ui32FWContextSwitchProfile;
	IMG_UINT32 ui32HWPerfFWBufSize;
	IMG_UINT32 ui32HWPerfHostBufSize;
	IMG_UINT32 ui32HWPerfFilter0;
	IMG_UINT32 ui32HWPerfFilter1;
	IMG_UINT32 ui32HWPerfHostFilter;
	IMG_UINT32 ui32TimeCorrClock;
	IMG_UINT32 ui32HWRDebugDumpLimit;
	IMG_UINT32 ui32LogType;
	IMG_UINT32 ui32KCCBSizeLog2;
#if defined(PVR_ARCH_VOLCANIC)
	IMG_UINT32 ui32ISPSchedulingLatencyMode;
#endif
	FW_PERF_CONF eFirmwarePerf;
	RGX_ACTIVEPM_CONF eRGXActivePMConf;
	RGX_RD_POWER_ISLAND_CONF eRGXRDPowerIslandConf;
#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	IMG_UINT32 ui32AvailablePowUnitsMask;
	IMG_UINT32 ui32AvailableRACMask;
	IMG_BOOL bSPUClockGating;
#endif
	IMG_BOOL   bEnableTrustedDeviceAceConfig;
	IMG_UINT32 ui32FWContextSwitchCrossDM;
#if defined(SUPPORT_PHYSMEM_TEST) && !defined(INTEGRITY_OS) && !defined(__QNXNTO__)
	IMG_UINT32 ui32PhysMemTestPasses;
#endif
	RGX_FWT_LOGTYPE eDebugDumpFWTLogType;
#if defined(SUPPORT_ICS)
	IMG_UINT32 ui32EnableIdleCycleStealing;
	IMG_UINT32 ui32FDTI;
	IMG_UINT32 ui32ICSThreshold;
	IMG_BOOL   bTestModeOn;
#endif
} RGX_INIT_APPHINTS;

#endif /* RGXINIT_APPHINTS_H */

/******************************************************************************
 End of file (rgxinit_apphints.h)
******************************************************************************/

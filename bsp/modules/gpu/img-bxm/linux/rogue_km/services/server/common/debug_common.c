/*************************************************************************/ /*!
@File
@Title          Debug Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Creates common debug info entries.
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

#if !defined(__linux__)
#include <errno.h>
#endif /* #if !defined(__linux__) */

#include "debug_common.h"
#include "pvrsrv.h"
#include "di_server.h"
#include "lists.h"
#include "pvrversion.h"
#include "rgx_options.h"
#include "allocmem.h"
#include "rgxfwutils.h"
#include "rgxfwriscv.h"
#include "osfunc.h"
#if defined(SUPPORT_RGX) && defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
#include "rgxfwdbg.h"
#endif

#ifdef SUPPORT_RGX
#include "rgxdevice.h"
#include "rgxdebug_common.h"
#include "rgxinit.h"
#include "rgxmmudefs_km.h"
#endif

static DI_ENTRY *gpsVersionDIEntry;
static DI_ENTRY *gpsStatusDIEntry;

#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
static DI_ENTRY *gpsDebugLevelDIEntry;
#endif /* defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON) */

#if defined(SUPPORT_RGX) && defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
struct DI_VZ_DATA {
	PVRSRV_DEVICE_NODE *psDevNode;
	IMG_UINT32 ui32DriverID;
};
#endif

static void _DumpDebugDIPrintfWrapper(void *pvDumpDebugFile, const IMG_CHAR *pszFormat, ...)
{
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	va_list ArgList;

	OSSNPrintf(szBuffer, PVR_MAX_DEBUG_MESSAGE_LEN, "%s\n", pszFormat);

	va_start(ArgList, pszFormat);
	DIVPrintf(pvDumpDebugFile, szBuffer, ArgList);
	va_end(ArgList);
}

/*************************************************************************/ /*!
 Version DebugFS entry
*/ /**************************************************************************/

static void *_DebugVersionCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode,
                                          va_list va)
{
	IMG_UINT64 *pui64CurrentPosition = va_arg(va, IMG_UINT64 *);
	IMG_UINT64 ui64Position = va_arg(va, IMG_UINT64);
	IMG_UINT64 ui64CurrentPosition = *pui64CurrentPosition;

	(*pui64CurrentPosition)++;

	return (ui64CurrentPosition == ui64Position) ? psDevNode : NULL;
}

static void *_VersionDIStart(OSDI_IMPL_ENTRY *psEntry, IMG_UINT64 *pui64Pos)
{
	PVRSRV_DATA *psPVRSRVData = DIGetPrivData(psEntry);
	IMG_UINT64 uiCurrentPosition = 1;
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVR_UNREFERENCED_PARAMETER(psEntry);

	if (psPVRSRVData == NULL) {
		PVR_DPF((PVR_DBG_ERROR, "psPVRSRVData = NULL"));
		return NULL;
	}

	if (*pui64Pos == 0)
	{
		return DI_START_TOKEN;
	}

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
	                                      _DebugVersionCompare_AnyVaCb,
	                                      &uiCurrentPosition,
	                                      *pui64Pos);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	return psDeviceNode;
}

static void _VersionDIStop(OSDI_IMPL_ENTRY *psEntry, void *pvPriv)
{
	PVR_UNREFERENCED_PARAMETER(psEntry);
	PVR_UNREFERENCED_PARAMETER(pvPriv);
}

static void *_VersionDINext(OSDI_IMPL_ENTRY *psEntry,void *pvPriv,
                            IMG_UINT64 *pui64Pos)
{
	PVRSRV_DATA *psPVRSRVData = DIGetPrivData(psEntry);
	IMG_UINT64 uiCurrentPosition = 1;
	PVRSRV_DEVICE_NODE *psDeviceNode;

	(*pui64Pos)++;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
	                                      _DebugVersionCompare_AnyVaCb,
	                                      &uiCurrentPosition,
	                                      *pui64Pos);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	return psDeviceNode;
}

#define DI_PRINT_VERSION_FMTSPEC \
		"%s Version: %u.%u @ %u (%s) build options: 0x%08x %s\n"
#define STR_DEBUG   "debug"
#define STR_RELEASE "release"

#if defined(DEBUG)
#define BUILD_OPT_LEN 80

static inline void _AppendOptionStr(IMG_CHAR pszBuildOptions[], const IMG_CHAR* str, OSDI_IMPL_ENTRY *psEntry, IMG_UINT32* pui32BuildOptionLen)
{
	IMG_UINT32 ui32BuildOptionLen = *pui32BuildOptionLen;
	const IMG_UINT32 strLen = OSStringLength(str);
	const IMG_UINT32 optStrLen = sizeof(IMG_CHAR) * (BUILD_OPT_LEN-1);

	if ((ui32BuildOptionLen + strLen) > optStrLen)
	{
		pszBuildOptions[ui32BuildOptionLen] = '\0';
		DIPrintf(psEntry, "%s\n", pszBuildOptions);
		ui32BuildOptionLen = 0;
	}
	if (strLen < optStrLen)
	{
		OSStringSafeCopy(pszBuildOptions+ui32BuildOptionLen, str, strLen);
		ui32BuildOptionLen += strLen - 1;
	}
	*pui32BuildOptionLen = ui32BuildOptionLen;
}
#endif /* DEBUG || SUPPORT_VALIDATION */

static int _VersionDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvPriv)
{
	PVRSRV_DATA *psPVRSRVData = DIGetPrivData(psEntry);

	if (pvPriv == DI_START_TOKEN)
	{
		if (psPVRSRVData->sDriverInfo.bIsNoMatch)
		{
			const BUILD_INFO *psBuildInfo;

			psBuildInfo = &psPVRSRVData->sDriverInfo.sUMBuildInfo;
			DIPrintf(psEntry, DI_PRINT_VERSION_FMTSPEC,
			         "UM Driver",
			         PVRVERSION_UNPACK_MAJ(psBuildInfo->ui32BuildVersion),
			         PVRVERSION_UNPACK_MIN(psBuildInfo->ui32BuildVersion),
			         psBuildInfo->ui32BuildRevision,
			         (psBuildInfo->ui32BuildType == BUILD_TYPE_DEBUG) ?
			                 STR_DEBUG : STR_RELEASE,
			         psBuildInfo->ui32BuildOptions,
			         PVR_BUILD_DIR);

			psBuildInfo = &psPVRSRVData->sDriverInfo.sKMBuildInfo;
			DIPrintf(psEntry, DI_PRINT_VERSION_FMTSPEC,
			         "KM Driver (" PVR_ARCH_NAME ")",
			         PVRVERSION_UNPACK_MAJ(psBuildInfo->ui32BuildVersion),
			         PVRVERSION_UNPACK_MIN(psBuildInfo->ui32BuildVersion),
			         psBuildInfo->ui32BuildRevision,
			         (psBuildInfo->ui32BuildType == BUILD_TYPE_DEBUG) ?
			                 STR_DEBUG : STR_RELEASE,
			         psBuildInfo->ui32BuildOptions,
			         PVR_BUILD_DIR);
		}
		else
		{
			/* bIsNoMatch is `false` in one of the following cases:
			 * - UM & KM version parameters actually match.
			 * - A comparison between UM & KM has not been made yet, because no
			 *   client ever connected.
			 *
			 * In both cases, available (KM) version info is the best output we
			 * can provide.
			 */
			DIPrintf(psEntry, "Driver Version: %s (%s) (%s) build options: "
			         "0x%08lx %s\n", PVRVERSION_STRING, PVR_ARCH_NAME,
			         PVR_BUILD_TYPE, RGX_BUILD_OPTIONS_KM, PVR_BUILD_DIR);
		}
	}
	else if (pvPriv != NULL)
	{
		PVRSRV_DEVICE_NODE *psDevNode = (PVRSRV_DEVICE_NODE *) pvPriv;
		PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;
#ifdef SUPPORT_RGX
		PVRSRV_RGXDEV_INFO *psDevInfo = psDevNode->pvDevice;
#if defined(DEBUG)
		IMG_CHAR pszBuildOptions[BUILD_OPT_LEN];
		IMG_UINT32 ui32BuildOptionLen = 0;
		static const char* aszOptions[] = RGX_BUILD_OPTIONS_LIST;
		int i = 0;
#endif
#endif /* SUPPORT_RGX */
		IMG_BOOL bFwVersionInfoPrinted = IMG_FALSE;

		DIPrintf(psEntry, "\nDevice Name: %s\n", psDevConfig->pszName);
		DIPrintf(psEntry, "Device ID: %u:%d\n", psDevNode->sDevId.ui32InternalID,
		                                        psDevNode->sDevId.i32KernelDeviceID);

		if (psDevConfig->pszVersion)
		{
			DIPrintf(psEntry, "Device Version: %s\n",
			          psDevConfig->pszVersion);
		}

		if (psDevNode->pfnDeviceVersionString)
		{
			IMG_CHAR *pszVerStr;

			if (psDevNode->pfnDeviceVersionString(psDevNode,
			                                      &pszVerStr) == PVRSRV_OK)
			{
				DIPrintf(psEntry, "%s\n", pszVerStr);

				OSFreeMem(pszVerStr);
			}
		}

		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDevNode))
		{
#ifdef SUPPORT_RGX
			/* print device's firmware version info */
			if (psDevInfo->psRGXFWIfOsInitMemDesc != NULL)
			{
				/* psDevInfo->psRGXFWIfOsInitMemDesc should be permanently mapped */
				if (psDevInfo->psRGXFWIfOsInit != NULL)
				{
					RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfOsInit->sRGXCompChecks,
				                               INVALIDATE);
					if (psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated)
					{
						const RGXFWIF_COMPCHECKS *psRGXCompChecks =
						        &psDevInfo->psRGXFWIfOsInit->sRGXCompChecks;
						IMG_UINT32 ui32DDKVer = psRGXCompChecks->ui32DDKVersion;

						DIPrintf(psEntry, DI_PRINT_VERSION_FMTSPEC,
						         "Firmware",
						         PVRVERSION_UNPACK_MAJ(ui32DDKVer),
						         PVRVERSION_UNPACK_MIN(ui32DDKVer),
						         psRGXCompChecks->ui32DDKBuild,
						         ((psRGXCompChecks->ui32BuildOptions &
						          OPTIONS_DEBUG_EN) ? STR_DEBUG : STR_RELEASE),
						         psRGXCompChecks->ui32BuildOptions,
						         PVR_BUILD_DIR);
						bFwVersionInfoPrinted = IMG_TRUE;

#if defined(DEBUG)
						DIPrintf(psEntry, "Firmware Build Options:\n");

						for (i = 0; i < ARRAY_SIZE(aszOptions); i++)
						{
							if ((psRGXCompChecks->ui32BuildOptions & 1<<i))
							{
								_AppendOptionStr(pszBuildOptions, aszOptions[i], psEntry, &ui32BuildOptionLen);
							}
						}

						if (ui32BuildOptionLen != 0)
						{
							DIPrintf(psEntry, "%s", pszBuildOptions);
						}
						DIPrintf(psEntry, "\n");
#endif
					}
				}
				else
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Error acquiring CPU virtual "
					        "address of FWInitMemDesc", __func__));
				}
			}
#endif /* SUPPORT_RGX */
		}
		else
		{
#ifdef SUPPORT_RGX
			RGX_FW_INFO_HEADER	*psFWInfoHeader = &psDevInfo->sFWInfoHeader;
			DIPrintf(psEntry, DI_PRINT_VERSION_FMTSPEC,
					 "Firmware",
					 psFWInfoHeader->ui16PVRVersionMajor,
					 psFWInfoHeader->ui16PVRVersionMinor,
					 psFWInfoHeader->ui32PVRVersionBuild,
					 ((psFWInfoHeader->ui32Flags &
					 OPTIONS_DEBUG_EN) ? STR_DEBUG : STR_RELEASE),
					 psFWInfoHeader->ui32Flags,
					 PVR_BUILD_DIR);

			bFwVersionInfoPrinted = IMG_TRUE;
#if defined(DEBUG)
			DIPrintf(psEntry, "Firmware Build Options:\n");

			for (i = 0; i < ARRAY_SIZE(aszOptions); i++)
			{
				if ((psFWInfoHeader->ui32Flags & 1<<i))
				{
					_AppendOptionStr(pszBuildOptions, aszOptions[i], psEntry, &ui32BuildOptionLen);
				}
			}

			if (ui32BuildOptionLen != 0)
			{
				DIPrintf(psEntry, "%s", pszBuildOptions);
			}
			DIPrintf(psEntry, "\n");
#endif

#endif /* SUPPORT_RGX */
		}

		if (!bFwVersionInfoPrinted)
		{
			DIPrintf(psEntry, "Firmware Version: Info unavailable %s\n",
#ifdef NO_HARDWARE
			         "on NoHW driver"
#else /* NO_HARDWARE */
			         "(Is INIT complete?)"
#endif /* NO_HARDWARE */
			         );
		}
	}

	return 0;
}

#if defined(SUPPORT_RGX) && defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)

/*************************************************************************/ /*!
 Power data DebugFS entry
*/ /**************************************************************************/

static PVRSRV_ERROR SendPowerCounterCommand(PVRSRV_DEVICE_NODE* psDeviceNode,
                                            RGXFWIF_COUNTER_DUMP_REQUEST eRequestType,
                                            IMG_UINT32 *pui32kCCBCommandSlot)
{
	PVRSRV_ERROR eError;

	RGXFWIF_KCCB_CMD sCounterDumpCmd;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	sCounterDumpCmd.eCmdType = RGXFWIF_KCCB_CMD_COUNTER_DUMP;
	sCounterDumpCmd.uCmdData.sCounterDumpConfigData.eCounterDumpRequest = eRequestType;

	eError = RGXScheduleCommandAndGetKCCBSlot(psDeviceNode->pvDevice,
				RGXFWIF_DM_GP,
				&sCounterDumpCmd,
				PDUMP_FLAGS_CONTINUOUS,
				pui32kCCBCommandSlot);
	PVR_LOG_IF_ERROR(eError, "RGXScheduleCommandAndGetKCCBSlot");

	return eError;
}

static int _DebugPowerDataDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	IMG_UINT32 ui32kCCBCommandSlot;
	int eError = 0;

	PVR_UNREFERENCED_PARAMETER(pvData);

	if (psDeviceNode->eDevState != PVRSRV_DEVICE_STATE_ACTIVE)
	{
		PVR_DPF((PVR_DBG_ERROR, "Device not initialised when "
				 "power counter data was requested!"));
		return -EIO;
	}

	OSLockAcquire(psDevInfo->hCounterDumpingLock);

	eError = SendPowerCounterCommand(psDeviceNode,
									 RGXFWIF_PWR_COUNTER_DUMP_SAMPLE,
									 &ui32kCCBCommandSlot);

	if (eError != PVRSRV_OK)
	{
		OSLockRelease(psDevInfo->hCounterDumpingLock);
		return -EIO;
	}

	/* Wait for FW complete completion */
	eError = RGXWaitForKCCBSlotUpdate(psDevInfo,
									  ui32kCCBCommandSlot,
									  PDUMP_FLAGS_CONTINUOUS);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_ERROR(eError, "RGXWaitForKCCBSlotUpdate");
		OSLockRelease(psDevInfo->hCounterDumpingLock);
		return -EIO;
	}

	/* Read back the buffer */
	{
		IMG_UINT32* pui32PowerBuffer;
		IMG_UINT32 ui32NumOfRegs, ui32SamplePeriod, ui32NumOfCores;
		IMG_UINT32 i, j;

		if (!psDevInfo->psCounterBufferMemDesc)
		{
			PVR_DPF((PVR_DBG_ERROR, "Counter buffer not allocated!"));
			return -EINVAL;
		}

		eError = DevmemAcquireCpuVirtAddr(psDevInfo->psCounterBufferMemDesc,
										  (void**)&pui32PowerBuffer);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "DevmemAcquireCpuVirtAddr");
			OSLockRelease(psDevInfo->hCounterDumpingLock);
			return -EIO;
		}

		RGXFwSharedMemCacheOpExec(pui32PowerBuffer, PAGE_SIZE, PVRSRV_CACHE_OP_INVALIDATE);

		ui32NumOfRegs = *pui32PowerBuffer++;
		ui32SamplePeriod = *pui32PowerBuffer++;
		ui32NumOfCores = *pui32PowerBuffer++;
		PVR_DPF((PVR_DBG_MESSAGE, "Number of power counters: %u.", ui32NumOfRegs));

		if (ui32NumOfCores == 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "No GPU cores enabled!"));
			eError = -EINVAL;
		}

		if (ui32NumOfRegs && ui32NumOfCores)
		{
			DIPrintf(psEntry, "Power counter data for device\n");
			DIPrintf(psEntry, "Sample period: 0x%08x\n", ui32SamplePeriod);

			for (i = 0; i < ui32NumOfRegs; i++)
			{
				IMG_UINT32 ui32High, ui32Low;
				IMG_UINT32 ui32RegOffset = *pui32PowerBuffer++;
				IMG_UINT32 ui32NumOfInstances = *pui32PowerBuffer++;

				PVR_ASSERT(ui32NumOfInstances);

				DIPrintf(psEntry, "0x%08x:", ui32RegOffset);

				for (j = 0; j < ui32NumOfInstances * ui32NumOfCores; j++)
				{
					ui32Low = *pui32PowerBuffer++;
#if defined(RGX_FEATURE_CATURIX_XTP_TOP_INFRASTRUCTURE_BIT_MASK)
					if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, CATURIX_XTP_TOP_INFRASTRUCTURE))
					{
						/* Power counters have 32-bit range */
						DIPrintf(psEntry, " 0x%08x", ui32Low);
					}
					else
#endif
					{
						/* Power counters have 64-bit range */
						ui32High = *pui32PowerBuffer++;

						DIPrintf(psEntry, " 0x%016" IMG_UINT64_FMTSPECx,
								 (IMG_UINT64) ui32Low | (IMG_UINT64) ui32High << 32);
					}
				}

				DIPrintf(psEntry, "\n");
			}
		}

		DevmemReleaseCpuVirtAddr(psDevInfo->psCounterBufferMemDesc);
	}

	OSLockRelease(psDevInfo->hCounterDumpingLock);

	return eError;
}

static IMG_INT64 PowerDataSet(const IMG_CHAR __user *pcBuffer,
                              IMG_UINT64 ui64Count, IMG_UINT64 *pui64Pos,
                              void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE*)pvData;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_COUNTER_DUMP_REQUEST eRequest;
	IMG_UINT32 ui32kCCBCommandSlot;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(pui64Pos != NULL && *pui64Pos == 0, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count >= 1, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);

	if (psDeviceNode->eDevState != PVRSRV_DEVICE_STATE_ACTIVE)
	{
		PVR_DPF((PVR_DBG_ERROR, "Device not initialised when "
				 "power counter data was requested!"));
		return -EIO;
	}

	if (pcBuffer[0] == '1')
	{
		eRequest = RGXFWIF_PWR_COUNTER_DUMP_START;
	}
	else if (pcBuffer[0] == '0')
	{
		eRequest = RGXFWIF_PWR_COUNTER_DUMP_STOP;
	}
	else
	{
		return -EINVAL;
	}

	OSLockAcquire(psDevInfo->hCounterDumpingLock);

	SendPowerCounterCommand(psDeviceNode,
	                        eRequest,
	                        &ui32kCCBCommandSlot);

	OSLockRelease(psDevInfo->hCounterDumpingLock);

	*pui64Pos += ui64Count;
	return ui64Count;
}

#endif /* defined(SUPPORT_RGX) && defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS) */

/*************************************************************************/ /*!
 Status DebugFS entry
*/ /**************************************************************************/

static void *_DebugStatusCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode,
										 va_list va)
{
	IMG_UINT64 *pui64CurrentPosition = va_arg(va, IMG_UINT64 *);
	IMG_UINT64 ui64Position = va_arg(va, IMG_UINT64);
	IMG_UINT64 ui64CurrentPosition = *pui64CurrentPosition;

	(*pui64CurrentPosition)++;

	return (ui64CurrentPosition == ui64Position) ? psDevNode : NULL;
}

static void *_DebugStatusDIStart(OSDI_IMPL_ENTRY *psEntry, IMG_UINT64 *pui64Pos)
{
	PVRSRV_DATA *psPVRSRVData = DIGetPrivData(psEntry);
	IMG_UINT64 uiCurrentPosition = 1;
	PVRSRV_DEVICE_NODE *psDeviceNode;

	if (*pui64Pos == 0)
	{
		return DI_START_TOKEN;
	}

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode =  List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugStatusCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *pui64Pos);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	return psDeviceNode;
}

static void _DebugStatusDIStop(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psEntry);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *_DebugStatusDINext(OSDI_IMPL_ENTRY *psEntry,
								 void *pvData,
								 IMG_UINT64 *pui64Pos)
{
	PVRSRV_DATA *psPVRSRVData = DIGetPrivData(psEntry);
	IMG_UINT64 uiCurrentPosition = 1;
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVR_UNREFERENCED_PARAMETER(pvData);

	(*pui64Pos)++;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode =  List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugStatusCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *pui64Pos);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

#ifdef SUPPORT_RGX
	if (psDeviceNode && !PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

		if (psDevInfo && psDevInfo->pfnGetGpuUtilStats)
		{
			PVRSRV_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
			PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus = OSAtomicRead(&psDeviceNode->eHealthStatus);

			if (eHealthStatus == PVRSRV_DEVICE_HEALTH_STATUS_OK)
			{
				PVRSRV_ERROR eError;
#if defined(EMULATOR) || defined(VIRTUAL_PLATFORM)
				static IMG_BOOL bFirstTime = IMG_TRUE;
#endif

				OSLockAcquire(psDevInfo->hGpuUtilStatsLock);

				eError = psDevInfo->pfnGetGpuUtilStats(psDeviceNode,
													   psDebugInfo->hGpuUtilUserDebugFS,
													   &psDevInfo->sGpuUtilStats);

				OSLockRelease(psDevInfo->hGpuUtilStatsLock);

				if (eError != PVRSRV_OK)
				{
#if defined(EMULATOR) || defined(VIRTUAL_PLATFORM)
					if (bFirstTime)
					{
						bFirstTime = IMG_FALSE;
#endif	/* defined(EMULATOR) || defined(VIRTUAL_PLATFORM) */
					PVR_DPF((PVR_DBG_ERROR,
					        "%s: Failed to get GPU statistics (%s)",
					        __func__, PVRSRVGetErrorString(eError)));
#if defined(EMULATOR) || defined(VIRTUAL_PLATFORM)
					}
#endif	/* defined(EMULATOR) || defined(VIRTUAL_PLATFORM) */
				}
			}
		}
	}
#endif

	return psDeviceNode;
}

static int _DebugStatusDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	if (pvData == DI_START_TOKEN)
	{
		PVRSRV_DATA *psPVRSRVData = DIGetPrivData(psEntry);

		if (psPVRSRVData != NULL)
		{
			switch (psPVRSRVData->eServicesState)
			{
				case PVRSRV_SERVICES_STATE_OK:
					DIPrintf(psEntry, "Driver Status:   OK\n");
					break;
				case PVRSRV_SERVICES_STATE_BAD:
					DIPrintf(psEntry, "Driver Status:   BAD\n");
					break;
				case PVRSRV_SERVICES_STATE_UNDEFINED:
					DIPrintf(psEntry, "Driver Status:   UNDEFINED\n");
					break;
				default:
					DIPrintf(psEntry, "Driver Status:   UNKNOWN (%d)\n",
					         psPVRSRVData->eServicesState);
					break;
			}
		}
	}
	else if (pvData != NULL)
	{
		PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
		IMG_CHAR           *pszStatus = "";
		IMG_CHAR           *pszReason = "";
		PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus;
		PVRSRV_DEVICE_HEALTH_REASON eHealthReason;

		DIPrintf(psEntry, "\nDevice ID: %u:%d\n", psDeviceNode->sDevId.ui32InternalID,
		                                          psDeviceNode->sDevId.i32KernelDeviceID);

		/* Update the health status now if possible... */
		if (psDeviceNode->pfnUpdateHealthStatus)
		{
			psDeviceNode->pfnUpdateHealthStatus(psDeviceNode, IMG_FALSE);
		}
		eHealthStatus = OSAtomicRead(&psDeviceNode->eHealthStatus);
		eHealthReason = OSAtomicRead(&psDeviceNode->eHealthReason);

		switch (eHealthStatus)
		{
			case PVRSRV_DEVICE_HEALTH_STATUS_OK:  pszStatus = "OK";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_NOT_RESPONDING:  pszStatus = "NOT RESPONDING";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_DEAD:  pszStatus = "DEAD";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_FAULT:  pszStatus = "FAULT";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_UNDEFINED:  pszStatus = "UNDEFINED";  break;
			default:  pszStatus = "UNKNOWN";  break;
		}

		switch (eHealthReason)
		{
			case PVRSRV_DEVICE_HEALTH_REASON_NONE:  pszReason = "";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_ASSERTED:  pszReason = " (Asserted)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_POLL_FAILING:  pszReason = " (Poll failing)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_TIMEOUTS:  pszReason = " (Global Event Object timeouts rising)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_QUEUE_CORRUPT:  pszReason = " (KCCB offset invalid)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_QUEUE_STALLED:  pszReason = " (KCCB stalled)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_IDLING:  pszReason = " (Idling)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_RESTARTING:  pszReason = " (Restarting)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_MISSING_INTERRUPTS:  pszReason = " (Missing interrupts)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_PCI_ERROR:  pszReason = " (PCI error)";  break;
			default:  pszReason = " (Unknown reason)";  break;
		}

		DIPrintf(psEntry, "Firmware Status: %s%s\n", pszStatus, pszReason);
		if (PVRSRV_ERROR_LIMIT_REACHED)
		{
			DIPrintf(psEntry, "Server Errors:   %d+\n", IMG_UINT32_MAX);
		}
		else
		{
			DIPrintf(psEntry, "Server Errors:   %d\n", PVRSRV_KM_ERRORS);
		}


		/* Write other useful stats to aid the test cycle... */
		if (psDeviceNode->pvDevice != NULL)
		{
#ifdef SUPPORT_RGX
			PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
			const RGXFWIF_HWRINFOBUF *psHWRInfoBuf;
			const RGXFWIF_SYSDATA *psFwSysData;

			RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfHWRInfoBufCtl, INVALIDATE);
			psHWRInfoBuf = psDevInfo->psRGXFWIfHWRInfoBufCtl;

			RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData, INVALIDATE);
			psFwSysData = psDevInfo->psRGXFWIfFwSysData;

#ifdef PVRSRV_DEBUG_LISR_EXECUTION
			/* Show the detected #LISR, #MISR scheduled calls */
			DIPrintf(psEntry, "RGX #LISR: %" IMG_UINT64_FMTSPEC "\n", psDeviceNode->ui64nLISR);
			DIPrintf(psEntry, "RGX #MISR: %" IMG_UINT64_FMTSPEC "\n", psDeviceNode->ui64nMISR);
#endif /* PVRSRV_DEBUG_LISR_EXECUTION */

			/* Calculate the number of HWR events in total across all the DMs... */
			if (psHWRInfoBuf != NULL)
			{
				IMG_UINT32 ui32HWREventCount = 0;
				IMG_UINT32 ui32CRREventCount = 0;
				IMG_UINT32 ui32DMIndex;

				for (ui32DMIndex = 0; ui32DMIndex < RGXFWIF_DM_MAX; ui32DMIndex++)
				{
					ui32HWREventCount += psHWRInfoBuf->aui32HwrDmLockedUpCount[ui32DMIndex];
					ui32CRREventCount += psHWRInfoBuf->aui32HwrDmOverranCount[ui32DMIndex];
				}

				DIPrintf(psEntry, "HWR Event Count: %d\n", ui32HWREventCount);
				DIPrintf(psEntry, "CRR Event Count: %d\n", ui32CRREventCount);
#ifdef PVRSRV_STALLED_CCB_ACTION
				/* Write the number of Sync Lockup Recovery (SLR) events... */
				RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwOsData->ui32ForcedUpdatesRequested,
				                           INVALIDATE);
				DIPrintf(psEntry, "SLR Event Count: %d\n", psDevInfo->psRGXFWIfFwOsData->ui32ForcedUpdatesRequested);
#endif /* PVRSRV_STALLED_CCB_ACTION */
			}

			/* Show error counts */
			DIPrintf(psEntry, "WGP Error Count: %d\n", psDevInfo->sErrorCounts.ui32WGPErrorCount);
			DIPrintf(psEntry, "TRP Error Count: %d\n", psDevInfo->sErrorCounts.ui32TRPErrorCount);

			/*
			 * Guest drivers do not support the following functionality:
			 *	- Perform actual on-chip fw tracing.
			 *	- Collect actual on-chip GPU utilization stats.
			 *	- Perform actual on-chip GPU power/dvfs management.
			 *	- As a result no more information can be provided.
			 */
			if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
			{
				if (psFwSysData != NULL)
				{
					DIPrintf(psEntry, "FWF Event Count: %d\n", psFwSysData->ui32FWFaults);
				}

				/* Write the number of APM events... */
				DIPrintf(psEntry, "APM Event Count: %d\n", psDevInfo->ui32ActivePMReqTotal);

				/* Write the current GPU Utilisation values... */
				if (psDevInfo->pfnGetGpuUtilStats &&
					eHealthStatus == PVRSRV_DEVICE_HEALTH_STATUS_OK)
				{
					PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
					RGXFWIF_GPU_UTIL_STATS *psGpuUtilStats = &psDevInfo->sGpuUtilStats;

					OSLockAcquire(psDevInfo->hGpuUtilStatsLock);

					if ((IMG_UINT32)psGpuUtilStats->ui64GpuStatCumulative)
					{
						const IMG_CHAR *apszDmNames[RGXFWIF_DM_MAX] = {"GP", "TDM", "GEOM", "3D", "CDM", "RAY", "GEOM2", "GEOM3", "GEOM4"};
						IMG_UINT64 util;
						IMG_UINT32 rem;
						IMG_UINT32 ui32DriverID;
						RGXFWIF_DM eDM;
						IMG_INT    iDM_Util = 0;

						if (!(RGX_IS_FEATURE_SUPPORTED(psDevInfo, FASTRENDER_DM)))
						{
							apszDmNames[RGXFWIF_DM_TDM] = "2D";
						}

						util = 100 * psGpuUtilStats->ui64GpuStatActive;
						util = OSDivide64(util, (IMG_UINT32)psGpuUtilStats->ui64GpuStatCumulative, &rem);

						DIPrintf(psEntry, "GPU Utilisation: %u%%\n", (IMG_UINT32)util);

						DIPrintf(psEntry, "DM Utilisation:");

						FOREACH_SUPPORTED_DRIVER(ui32DriverID)
						{
							DIPrintf(psEntry, "  VM%u", ui32DriverID);
						}

						DIPrintf(psEntry, "\n");

						for (eDM = RGXFWIF_DM_TDM; eDM < psDevInfo->sDevFeatureCfg.ui32MAXDMCount; eDM++,iDM_Util++)
						{
							DIPrintf(psEntry, "        %5s: ", apszDmNames[eDM]);

							FOREACH_SUPPORTED_DRIVER(ui32DriverID)
							{
								IMG_UINT32 uiDivisor = (IMG_UINT32)psGpuUtilStats->aaui64DMOSStatCumulative[iDM_Util][ui32DriverID];

								if (uiDivisor == 0U)
								{
									DIPrintf(psEntry, "   - ");
									continue;
								}

								util = 100 * psGpuUtilStats->aaui64DMOSStatActive[iDM_Util][ui32DriverID];
								util = OSDivide64(util, uiDivisor, &rem);

								DIPrintf(psEntry, "%3u%% ", (IMG_UINT32)util);
							}


							DIPrintf(psEntry, "\n");
						}
					}
					else
					{
						DIPrintf(psEntry, "GPU Utilisation: -\n");
					}

					OSLockRelease(psDevInfo->hGpuUtilStatsLock);

				}
			}
#endif /* SUPPORT_RGX */
		}
	}

	return 0;
}

#if defined(DEBUG)
static IMG_INT64 DebugStatusSet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
                                IMG_UINT64 *pui64Pos, void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(pui64Pos != NULL && *pui64Pos == 0, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count >= 1, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[0] == 'k' || pcBuffer[0] == 'K', -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);

	psPVRSRVData->eServicesState = PVRSRV_SERVICES_STATE_BAD;

	*pui64Pos += ui64Count;
	return ui64Count;
}
#endif

const IMG_CHAR *PVRSRVGetDebugDevStateString(PVRSRV_DEVICE_STATE eDevState)
{
	static const char *const _pszDeviceStateStrings[] = {
	#define X(_name) #_name,
		PVRSRV_DEVICE_STATE_LIST
	#undef X
	};

	if (eDevState < 0 || eDevState >= PVRSRV_DEVICE_STATE_LAST)
	{
		return "Undefined";
	}

	return _pszDeviceStateStrings[eDevState];
}


const IMG_CHAR *PVRSRVGetDebugHealthStatusString(PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus)
{
	static const char *const _pszDeviceHealthStatusStrings[] = {
	#define X(_name) #_name,
		PVRSRV_DEVICE_HEALTH_STATUS_LIST
	#undef X
	};

	if (eHealthStatus < 0 || eHealthStatus >= PVRSRV_DEVICE_HEALTH_STATUS_LAST)
	{
		return "Undefined";
	}

	return _pszDeviceHealthStatusStrings[eHealthStatus];
}


const IMG_CHAR *PVRSRVGetDebugHealthReasonString(PVRSRV_DEVICE_HEALTH_REASON eHealthReason)
{
	static const char *const _pszDeviceHealthReasonStrings[] = {
	#define X(_name) #_name,
		PVRSRV_DEVICE_HEALTH_REASON_LIST
	#undef X
	};

	if (eHealthReason < 0 || eHealthReason >= PVRSRV_DEVICE_HEALTH_REASON_LAST)
	{
		return "Undefined";
	}

	return _pszDeviceHealthReasonStrings[eHealthReason];
}

/*************************************************************************/ /*!
 Dump Debug DebugFS entry
*/ /**************************************************************************/

static int _DebugDumpDebugDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = DIGetPrivData(psEntry);

	PVR_UNREFERENCED_PARAMETER(pvData);

	if (psDeviceNode->pvDevice != NULL)
	{
		PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX,
		                   _DumpDebugDIPrintfWrapper, psEntry);
	}

	return 0;
}

#ifdef SUPPORT_RGX

/*************************************************************************/ /*!
 Firmware Trace DebugFS entry
*/ /**************************************************************************/

static int _DebugFWTraceDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(pvData);

	if (psDevInfo != NULL)
	{
		RGXDumpFirmwareTrace(_DumpDebugDIPrintfWrapper, psEntry, psDevInfo);
	}

	return 0;
}

/*************************************************************************/ /*!
 Firmware Translated Page Tables DebugFS entry
*/ /**************************************************************************/

#if !defined(SUPPORT_TRUSTED_DEVICE) || defined(SUPPORT_SECURITY_VALIDATION)
static int _FirmwareMappingsDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32FwVA;
	IMG_UINT32 ui32FwPageSize;
	IMG_UINT32 ui32DriverID;

	PVR_UNREFERENCED_PARAMETER(pvData);

	psDeviceNode = DIGetPrivData(psEntry);

	if ((psDeviceNode == NULL) ||
	    (psDeviceNode->pvDevice == NULL) ||
	    (((PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice)->psKernelMMUCtx == NULL))
	{
		/* The Kernel MMU context containing the Firmware mappings is not initialised */
		return 0;
	}

	psDevInfo = psDeviceNode->pvDevice;

	DIPrintf(psEntry, "+-----------------+------------------------+------------------------+--------------+\n"
					  "|    Firmware     |           CPU          |         Device         |      PTE     |\n"
					  "| Virtual Address |    Physical Address    |    Physical Address    |     Flags    |\n"
					  "+-----------------+------------------------+------------------------+              +\n");

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		DIPrintf(psEntry, "|                                               RI/XI = Read / Execution Inhibit   |\n"
		                  "|                                               C     = Cache Coherent             |\n"
		                  "|                                               D     = Dirty Page Table Entry     |\n"
		                  "|                                               V     = Valid Page Table Entry     |\n"
		                  "|                                               G     = Global Page Table Entry    |\n"
		                  "+-----------------+------------------------+------------------------+--------------+\n");

		/* MIPS uses the same page size as the OS */
		ui32FwPageSize = OSGetPageSize();
	}
	else
#endif
	{
		DIPrintf(psEntry, "|                                               P     = Pending Page Table Entry   |\n"
		                  "|                                               PM    = Parameter Manager Source   |\n"
		                  "|                                               B     = Bypass SLC                 |\n"
		                  "|                                               C     = Cache Coherent             |\n"
		                  "|                                               RW/RO = Device Access Rights       |\n"
		                  "|                                               V     = Valid Page Table Entry     |\n"
		                  "+-----------------+------------------------+------------------------+--------------+\n");

		ui32FwPageSize = BIT(RGX_MMUCTRL_PAGE_4KB_RANGE_SHIFT);
	}

	FOREACH_SUPPORTED_DRIVER(ui32DriverID)
	{
		IMG_UINT32 ui32FwHeapBase = (IMG_UINT32) ((RGX_FIRMWARE_RAW_HEAP_BASE +
		                             (ui32DriverID * RGX_FIRMWARE_RAW_HEAP_SIZE)) & UINT_MAX);
		IMG_UINT32 ui32FwHeapEnd  = ui32FwHeapBase + (IMG_UINT32) (RGX_FIRMWARE_RAW_HEAP_SIZE & UINT_MAX);

		DIPrintf(psEntry, "|                                       OS ID %u                                    |\n"
						  "+-----------------+------------------------+------------------------+--------------+\n", ui32DriverID);

		for (ui32FwVA = ui32FwHeapBase;
		     ui32FwVA < ui32FwHeapEnd;
		     ui32FwVA += ui32FwPageSize)
		{
			PVRSRV_ERROR eError;
			IMG_UINT64 ui64PTE = 0U;
			IMG_CPU_PHYADDR sCpuPA = {0U};
			IMG_DEV_PHYADDR sDevPA = {0U};

			eError = RGXGetFwMapping(psDevInfo, ui32FwVA, &sCpuPA, &sDevPA, &ui64PTE);

			if (eError == PVRSRV_OK)
			{
				RGXDocumentFwMapping(psDevInfo, _DumpDebugDIPrintfWrapper, psEntry,
				                     ui32FwVA, sCpuPA, sDevPA, ui64PTE);
			}
			else if (eError != PVRSRV_ERROR_DEVICEMEM_NO_MAPPING)
			{
				PVR_LOG_ERROR(eError, "RGXGetFwMapping");
				return -EIO;
			}
		}

		DIPrintf(psEntry, "+-----------------+------------------------+------------------------+--------------+\n");

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		if (PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode))
		{
			break;
		}
#endif
	}

	return 0;
}
#endif

#ifdef SUPPORT_FIRMWARE_GCOV

static void *_FirmwareGcovDIStart(OSDI_IMPL_ENTRY *psEntry, IMG_UINT64 *pui64Pos)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (psDevInfo != NULL)
	{
		if (psDevInfo->psFirmwareGcovBufferMemDesc != NULL)
		{
			void *pvCpuVirtAddr;
			DevmemAcquireCpuVirtAddr(psDevInfo->psFirmwareGcovBufferMemDesc, &pvCpuVirtAddr);
			return *pui64Pos ? NULL : pvCpuVirtAddr;
		}
	}

	return NULL;
}

static void _FirmwareGcovDIStop(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(pvData);

	if (psDevInfo != NULL)
	{
		if (psDevInfo->psFirmwareGcovBufferMemDesc != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psFirmwareGcovBufferMemDesc);
		}
	}
}

static void *_FirmwareGcovDINext(OSDI_IMPL_ENTRY *psEntry,
								  void *pvData,
								  IMG_UINT64 *pui64Pos)
{
	PVR_UNREFERENCED_PARAMETER(psEntry);
	PVR_UNREFERENCED_PARAMETER(pvData);
	PVR_UNREFERENCED_PARAMETER(pui64Pos);
	return NULL;
}

static int _FirmwareGcovDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (psDevInfo != NULL)
	{
		DIWrite(psEntry, pvData, psDevInfo->ui32FirmwareGcovSize);
	}
	return 0;
}

#endif /* SUPPORT_FIRMWARE_GCOV */


#if  defined(SUPPORT_RISCV_GDB)
#define RISCV_DMI_SIZE  (8U)

static IMG_INT64 _RiscvDmiRead(IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
                               IMG_UINT64 *pui64Pos, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
	PVRSRV_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;

	ui64Count = MIN(RISCV_DMI_SIZE, ui64Count);
	memcpy(pcBuffer, &psDebugInfo->ui64RiscvDmi, ui64Count);

	return ui64Count;
}

static IMG_INT64 _RiscvDmiWrite(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
                                IMG_UINT64 *pui64Pos, void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;

	if (psDevInfo == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: devinfo is NULL", __func__));
		return 0;
	}

	ui64Count -= 1; /* Drop `\0` */
	ui64Count = MIN(RISCV_DMI_SIZE, ui64Count);

	memcpy(&psDebugInfo->ui64RiscvDmi, pcBuffer, ui64Count);

	RGXRiscvDmiOp(psDevInfo, &psDebugInfo->ui64RiscvDmi);

	return ui64Count;
}
#endif

#endif /* SUPPORT_RGX */


#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)

/*************************************************************************/ /*!
 Debug level DebugFS entry
*/ /**************************************************************************/

static int DebugLevelDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	DIPrintf(psEntry, "%u\n", OSDebugLevel());

	return 0;
}

#ifndef __GNUC__
static int __builtin_ffsl(long int x)
{
	for (size_t i = 0; i < sizeof(x) * 8; i++)
	{
		if (x & (1 << i))
		{
			return i + 1;
		}
	}
	return 0;
}
#endif /* __GNUC__ */

static IMG_INT64 DebugLevelSet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
                               IMG_UINT64 *pui64Pos, void *pvData)
{
	const IMG_UINT uiMaxBufferSize = 6;
	IMG_UINT32 ui32Level;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(pui64Pos != NULL && *pui64Pos == 0, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count > 0 && ui64Count < uiMaxBufferSize, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);

	if (sscanf(pcBuffer, "%u", &ui32Level) == 0)
	{
		return -EINVAL;
	}

	OSSetDebugLevel(ui32Level & ((1 << __builtin_ffsl(DBGPRIV_LAST)) - 1));

	*pui64Pos += ui64Count;
	return ui64Count;
}
#endif /* defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON) */

#if defined(SUPPORT_RGX) && defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
static int VZPriorityDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	DI_VZ_DATA *psVZDriverData = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg;
	IMG_UINT32 ui32DriverID;

	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	psDevInfo = psVZDriverData->psDevNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, -EIO);

	psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVR_RETURN_IF_FALSE(psRuntimeCfg != NULL, -EIO);

	ui32DriverID = psVZDriverData->ui32DriverID;
	PVR_RETURN_IF_FALSE(ui32DriverID < (RGXFW_HOST_DRIVER_ID + RGX_NUM_DRIVERS_SUPPORTED),
	                    -EINVAL);

	RGXFwSharedMemCacheOpValue(psRuntimeCfg->ai32DriverPriority[ui32DriverID], INVALIDATE);
	DIPrintf(psEntry, "%u\n", psRuntimeCfg->ai32DriverPriority[ui32DriverID]);

	return 0;
}

static IMG_INT64 VZPrioritySet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
                               IMG_UINT64 *pui64Pos, void *pvData)
{
	const DI_VZ_DATA *psVZDriverData = (const DI_VZ_DATA*)pvData;
	const IMG_UINT32 uiMaxBufferSize = 12;
	IMG_UINT32 ui32Priority;
	PVRSRV_ERROR eError;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count > 0 && ui64Count < uiMaxBufferSize, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	if (OSStringToUINT32(pcBuffer, 10, &ui32Priority) != PVRSRV_OK)
	{
		return -EINVAL;
	}

	eError = PVRSRVRGXFWDebugSetDriverPriorityKM(NULL, psVZDriverData->psDevNode,
	                                             psVZDriverData->ui32DriverID, ui32Priority);
	if (eError != PVRSRV_OK)
	{
		return -EIO;
	}

	*pui64Pos += ui64Count;
	return ui64Count;
}

static int VZTimeSliceIntervalDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	DI_VZ_DATA *psVZDriverData = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg;

	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	psDevInfo = psVZDriverData->psDevNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, -EIO);

	psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVR_RETURN_IF_FALSE(psRuntimeCfg != NULL, -EIO);

	DIPrintf(psEntry, "%u ms (0: disable)\n", psRuntimeCfg->ui32TSIntervalMs);

	return 0;
}

static IMG_INT64 VZTimeSliceIntervalSet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
										IMG_UINT64 *pui64Pos, void *pvData)
{
	const DI_VZ_DATA *psVZDriverData = (const DI_VZ_DATA*)pvData;
	const IMG_UINT32 uiMaxBufferSize = 12;
	IMG_UINT32 ui32TSIntervalMs;
	PVRSRV_ERROR eError;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count > 0 && ui64Count < uiMaxBufferSize, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	if (OSStringToUINT32(pcBuffer, 10, &ui32TSIntervalMs) != PVRSRV_OK)
	{
		return -EINVAL;
	}

	eError = PVRSRVRGXFWDebugSetDriverTimeSliceIntervalKM(NULL, psVZDriverData->psDevNode,
														  ui32TSIntervalMs);
	if (eError != PVRSRV_OK)
	{
		return -EIO;
	}

	*pui64Pos += ui64Count;
	return ui64Count;
}

static int VZTimeSliceDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	DI_VZ_DATA *psVZDriverData = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg;
	IMG_UINT32 ui32DriverID;

	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	psDevInfo = psVZDriverData->psDevNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, -EIO);

	psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVR_RETURN_IF_FALSE(psRuntimeCfg != NULL, -EIO);

	ui32DriverID = psVZDriverData->ui32DriverID;
	PVR_RETURN_IF_FALSE(ui32DriverID < (RGXFW_HOST_DRIVER_ID + RGX_NUM_DRIVERS_SUPPORTED),
						-EINVAL);

	DIPrintf(psEntry, "%u (0: auto; 1%% to 100%%)\n", psRuntimeCfg->aui32TSPercentage[ui32DriverID]);

	return 0;
}

static IMG_INT64 VZTimeSliceSet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
								IMG_UINT64 *pui64Pos, void *pvData)
{
	const DI_VZ_DATA *psVZDriverData = (const DI_VZ_DATA*)pvData;
	const IMG_UINT32 uiMaxBufferSize = 12;
	IMG_UINT32 ui32TSPercentage;
	PVRSRV_ERROR eError;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count > 0 && ui64Count < uiMaxBufferSize, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	if (OSStringToUINT32(pcBuffer, 10, &ui32TSPercentage) != PVRSRV_OK)
	{
		return -EINVAL;
	}

	eError = PVRSRVRGXFWDebugSetDriverTimeSliceKM(NULL, psVZDriverData->psDevNode,
												  psVZDriverData->ui32DriverID, ui32TSPercentage);
	if (eError != PVRSRV_OK)
	{
		return -EIO;
	}

	*pui64Pos += ui64Count;
	return ui64Count;
}

static int VZIsolationGroupDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	DI_VZ_DATA *psVZDriverData = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg;
	IMG_UINT32 ui32DriverID;

	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	psDevInfo = psVZDriverData->psDevNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, -EIO);

	psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVR_RETURN_IF_FALSE(psRuntimeCfg != NULL, -EIO);

	ui32DriverID = psVZDriverData->ui32DriverID;
	PVR_RETURN_IF_FALSE(ui32DriverID < (RGXFW_HOST_DRIVER_ID + RGX_NUM_DRIVERS_SUPPORTED),
	                    -EINVAL);

	RGXFwSharedMemCacheOpValue(psRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID], INVALIDATE);
	DIPrintf(psEntry, "%u\n", psRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID]);

	return 0;
}

static IMG_INT64 VZIsolationGroupSet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
                                     IMG_UINT64 *pui64Pos, void *pvData)
{
	const DI_VZ_DATA *psVZDriverData = (const DI_VZ_DATA*)pvData;
	const IMG_UINT32 uiMaxBufferSize = 12;
	IMG_UINT32 ui32IsolationGroup;
	PVRSRV_ERROR eError;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count > 0 && ui64Count < uiMaxBufferSize, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	if (OSStringToUINT32(pcBuffer, 10, &ui32IsolationGroup) != PVRSRV_OK)
	{
		return -EINVAL;
	}

	eError = PVRSRVRGXFWDebugSetDriverIsolationGroupKM(NULL, psVZDriverData->psDevNode,
	                                                   psVZDriverData->ui32DriverID, ui32IsolationGroup);
	if (eError != PVRSRV_OK)
	{
		return -EIO;
	}

	*pui64Pos += ui64Count;
	return ui64Count;
}

static int VZConnectionCooldownPeriodDIShow(OSDI_IMPL_ENTRY *psEntry, void *pvData)
{
	DI_VZ_DATA *psVZDriverData = DIGetPrivData(psEntry);
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg;

	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	psDevInfo = psVZDriverData->psDevNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, -EIO);

	psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVR_RETURN_IF_FALSE(psRuntimeCfg != NULL, -EIO);

	DIPrintf(psEntry, "%u sec\n", psRuntimeCfg->ui32VzConnectionCooldownPeriodInSec);

	return 0;
}

static IMG_INT64 VZConnectionCooldownPeriodSet(const IMG_CHAR *pcBuffer, IMG_UINT64 ui64Count,
										IMG_UINT64 *pui64Pos, void *pvData)
{
	const DI_VZ_DATA *psVZDriverData = (const DI_VZ_DATA*)pvData;
	const IMG_UINT32 uiMaxBufferSize = 12;
	IMG_UINT32 ui32VzConnectionCooldownPeriodInSec;
	PVRSRV_ERROR eError;

	PVR_RETURN_IF_FALSE(pcBuffer != NULL, -EIO);
	PVR_RETURN_IF_FALSE(ui64Count > 0 && ui64Count < uiMaxBufferSize, -EINVAL);
	PVR_RETURN_IF_FALSE(pcBuffer[ui64Count - 1] == '\0', -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData != NULL, -EINVAL);
	PVR_RETURN_IF_FALSE(psVZDriverData->psDevNode != NULL, -ENXIO);

	if (OSStringToUINT32(pcBuffer, 10, &ui32VzConnectionCooldownPeriodInSec) != PVRSRV_OK)
	{
		return -EINVAL;
	}

	eError = PVRSRVRGXFWDebugSetVzConnectionCooldownPeriodInSecKM(NULL, psVZDriverData->psDevNode,
														  ui32VzConnectionCooldownPeriodInSec);
	if (eError != PVRSRV_OK)
	{
		return -EIO;
	}

	*pui64Pos += ui64Count;
	return ui64Count;
}

#endif

PVRSRV_ERROR DebugCommonInitDriver(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_ERROR eError;

	PVR_ASSERT(psPVRSRVData != NULL);

	/*
	 * The DebugFS entries are designed to work in a single device system but
	 * this function will be called multiple times in a multi-device system.
	 * Return an error in this case.
	 */
	if (gpsVersionDIEntry)
	{
		return -EEXIST;
	}

	{
		DI_ITERATOR_CB sIterator = {
			.pfnStart = _VersionDIStart,
			.pfnStop = _VersionDIStop,
			.pfnNext = _VersionDINext,
			.pfnShow = _VersionDIShow
		};

		eError = DICreateEntry("version", NULL, &sIterator, psPVRSRVData,
		                       DI_ENTRY_TYPE_GENERIC, &gpsVersionDIEntry);
		PVR_GOTO_IF_ERROR(eError, return_error_);
	}

	{
		DI_ITERATOR_CB sIterator = {
			.pfnStart = _DebugStatusDIStart,
			.pfnStop = _DebugStatusDIStop,
			.pfnNext = _DebugStatusDINext,
			.pfnShow = _DebugStatusDIShow,
#if defined(DEBUG)
			.pfnWrite = DebugStatusSet,
			//'K' expected + Null terminator
#endif
			.ui32WriteLenMax= ((1U)+1U)
		};
		eError = DICreateEntry("status", NULL, &sIterator, psPVRSRVData,
		                       DI_ENTRY_TYPE_GENERIC, &gpsStatusDIEntry);
		PVR_GOTO_IF_ERROR(eError, return_error_);
	}


#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
	{
		DI_ITERATOR_CB sIterator = {
			.pfnShow = DebugLevelDIShow,
			.pfnWrite = DebugLevelSet,
			//Max value of 255(3 char) + Null terminator
			.ui32WriteLenMax =((3U)+1U)
		};
		eError = DICreateEntry("debug_level", NULL, &sIterator, NULL,
		                       DI_ENTRY_TYPE_GENERIC, &gpsDebugLevelDIEntry);
		PVR_GOTO_IF_ERROR(eError, return_error_);
	}
#endif /* defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON) */

	return PVRSRV_OK;

return_error_:
	DebugCommonDeInitDriver();

	return eError;
}

void DebugCommonDeInitDriver(void)
{
#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
	if (gpsDebugLevelDIEntry != NULL)
	{
		DIDestroyEntry(gpsDebugLevelDIEntry);
	}
#endif /* defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON) */


	if (gpsStatusDIEntry != NULL)
	{
		DIDestroyEntry(gpsStatusDIEntry);
	}

	if (gpsVersionDIEntry != NULL)
	{
		DIDestroyEntry(gpsVersionDIEntry);
	}
}

PVRSRV_ERROR DebugCommonInitDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
	PVRSRV_ERROR eError;
	IMG_CHAR pszDeviceId[sizeof("gpu4294967296")];
	__maybe_unused PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	OSSNPrintf(pszDeviceId, sizeof(pszDeviceId), "gpu%02d",
	           psDeviceNode->sDevId.ui32InternalID);
	eError = DICreateGroup(pszDeviceId, NULL, &psDebugInfo->psGroup);
	PVR_GOTO_IF_ERROR(eError, return_error_);

#if defined(SUPPORT_RGX) && !defined(NO_HARDWARE)
	eError = SORgxGpuUtilStatsRegister(&psDebugInfo->hGpuUtilUserDebugFS);
	PVR_GOTO_IF_ERROR(eError, return_error_);
#endif

	{
		DI_ITERATOR_CB sIterator = {.pfnShow = _DebugDumpDebugDIShow};
		eError = DICreateEntry("debug_dump", psDebugInfo->psGroup, &sIterator,
		                       psDeviceNode, DI_ENTRY_TYPE_GENERIC,
		                       &psDebugInfo->psDumpDebugEntry);
		PVR_GOTO_IF_ERROR(eError, return_error_);
	}

#ifdef SUPPORT_RGX
	if (! PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		{
			DI_ITERATOR_CB sIterator = {.pfnShow = _DebugFWTraceDIShow};
			eError = DICreateEntry("firmware_trace", psDebugInfo->psGroup, &sIterator,
			                       psDeviceNode, DI_ENTRY_TYPE_GENERIC,
			                       &psDebugInfo->psFWTraceEntry);
			PVR_GOTO_IF_ERROR(eError, return_error_);
		}

#ifdef SUPPORT_FIRMWARE_GCOV
		{
			DI_ITERATOR_CB sIterator = {
				.pfnStart = _FirmwareGcovDIStart,
				.pfnStop = _FirmwareGcovDIStop,
				.pfnNext = _FirmwareGcovDINext,
				.pfnShow = _FirmwareGcovDIShow
			};

			eError = DICreateEntry("firmware_gcov", psDebugInfo->psGroup, &sIterator,
			                       psDeviceNode, DI_ENTRY_TYPE_GENERIC,
			                       &psDebugInfo->psFWGCOVEntry);
			PVR_GOTO_IF_ERROR(eError, return_error_);
		}
#endif /* SUPPORT_FIRMWARE_GCOV */

#if !defined(SUPPORT_TRUSTED_DEVICE) || defined(SUPPORT_SECURITY_VALIDATION)
		{
			DI_ITERATOR_CB sIterator = {.pfnShow = _FirmwareMappingsDIShow};
			eError = DICreateEntry("firmware_mappings", psDebugInfo->psGroup, &sIterator,
			                       psDeviceNode, DI_ENTRY_TYPE_GENERIC,
			                       &psDebugInfo->psFWMappingsEntry);
			PVR_GOTO_IF_ERROR(eError, return_error_);
		}
#endif

#if  defined(SUPPORT_RISCV_GDB)
		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
		{
			DI_ITERATOR_CB sIterator = {
				.pfnRead = _RiscvDmiRead,
				.pfnWrite = _RiscvDmiWrite,
				.ui32WriteLenMax = ((RISCV_DMI_SIZE)+1U)
			};
			eError = DICreateEntry("riscv_dmi", psDebugInfo->psGroup, &sIterator, psDeviceNode,
			                       DI_ENTRY_TYPE_RANDOM_ACCESS, &psDebugInfo->psRiscvDmiDIEntry);
			PVR_GOTO_IF_ERROR(eError, return_error_);
			psDebugInfo->ui64RiscvDmi = 0ULL;
		}
#endif /* SUPPORT_VALIDATION || SUPPORT_RISCV_GDB */

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		if (PVRSRV_VZ_MODE_IS(HOST, DEVNODE, psDeviceNode))
		{
			eError = DICreateGroup("vz", psDebugInfo->psGroup, &psDebugInfo->psVZGroup);
			PVR_GOTO_IF_ERROR(eError, return_error_);

			{
				IMG_UINT32 ui32DriverID;

				DI_ITERATOR_CB sPriorityIterator = {
					.pfnShow = VZPriorityDIShow,
					.pfnWrite = VZPrioritySet,
					//Max value of UINT_MAX (10 chars) + Null terminator
					.ui32WriteLenMax = sizeof("4294967295")
				};

				DI_ITERATOR_CB sTimeSliceIntervalIterator = {
					.pfnShow = VZTimeSliceIntervalDIShow,
					.pfnWrite = VZTimeSliceIntervalSet,
					//Max value of UINT_MAX (10 chars) + Null terminator
					.ui32WriteLenMax = sizeof("4294967295")
				};

				DI_ITERATOR_CB sTimeSliceIterator = {
					.pfnShow = VZTimeSliceDIShow,
					.pfnWrite = VZTimeSliceSet,
					//Max value of UINT_MAX (10 chars) + Null terminator
					.ui32WriteLenMax = sizeof("4294967295")
				};

				DI_ITERATOR_CB sIsolationGroupIterator = {
					.pfnShow = VZIsolationGroupDIShow,
					.pfnWrite = VZIsolationGroupSet,
					//Max value of UINT_MAX (10 chars) + Null terminator
					.ui32WriteLenMax = sizeof("4294967295")
				};

				DI_ITERATOR_CB sVzConnectionCooldownPeriodIterator = {
					.pfnShow = VZConnectionCooldownPeriodDIShow,
					.pfnWrite = VZConnectionCooldownPeriodSet,
					//Max value of UINT_MAX (10 chars) + Null terminator
					.ui32WriteLenMax = sizeof("4294967295")
				};

				FOREACH_SUPPORTED_DRIVER(ui32DriverID)
				{
					IMG_CHAR szDriverID[4];
					OSSNPrintf(szDriverID, 4, "%u", ui32DriverID);

					eError = DICreateGroup(szDriverID, psDebugInfo->psVZGroup, &psDebugInfo->apsVZDriverGroups[ui32DriverID]);
					PVR_GOTO_IF_ERROR(eError, return_error_);

					psDebugInfo->apsVZDriverData[ui32DriverID] = OSAllocMem(sizeof(PVRSRV_DEVICE_DEBUG_INFO));
					PVR_GOTO_IF_NOMEM(psDebugInfo->apsVZDriverData[ui32DriverID], eError, return_error_);

					psDebugInfo->apsVZDriverData[ui32DriverID]->psDevNode = psDeviceNode;
					psDebugInfo->apsVZDriverData[ui32DriverID]->ui32DriverID = ui32DriverID;

					eError = DICreateEntry("priority", psDebugInfo->apsVZDriverGroups[ui32DriverID],
					                       &sPriorityIterator, psDebugInfo->apsVZDriverData[ui32DriverID], DI_ENTRY_TYPE_GENERIC,
					                       &psDebugInfo->apsVZDriverPriorityDIEntries[ui32DriverID]);
					PVR_GOTO_IF_ERROR(eError, return_error_);

					eError = DICreateEntry("time_slice", psDebugInfo->apsVZDriverGroups[ui32DriverID],
					                       &sTimeSliceIterator, psDebugInfo->apsVZDriverData[ui32DriverID], DI_ENTRY_TYPE_GENERIC,
					                       &psDebugInfo->apsVZDriverTimeSliceDIEntries[ui32DriverID]);
					PVR_GOTO_IF_ERROR(eError, return_error_);

					if (ui32DriverID == RGXFW_HOST_DRIVER_ID)
					{
						eError = DICreateEntry("time_slice_interval", psDebugInfo->apsVZDriverGroups[ui32DriverID],
											   &sTimeSliceIntervalIterator, psDebugInfo->apsVZDriverData[ui32DriverID], DI_ENTRY_TYPE_GENERIC,
											   &psDebugInfo->psVZDriverTimeSliceIntervalDIEntry);
						PVR_GOTO_IF_ERROR(eError, return_error_);

						eError = DICreateEntry("vz_connection_cooldown_period", psDebugInfo->apsVZDriverGroups[ui32DriverID],
											   &sVzConnectionCooldownPeriodIterator, psDebugInfo->apsVZDriverData[ui32DriverID], DI_ENTRY_TYPE_GENERIC,
											   &psDebugInfo->psVZDriverConnectionCooldownPeriodDIEntry);
						PVR_GOTO_IF_ERROR(eError, return_error_);
					}

					eError = DICreateEntry("isolation_group", psDebugInfo->apsVZDriverGroups[ui32DriverID],
					                       &sIsolationGroupIterator, psDebugInfo->apsVZDriverData[ui32DriverID], DI_ENTRY_TYPE_GENERIC,
					                       &psDebugInfo->apsVZDriverIsolationGroupDIEntries[ui32DriverID]);
					PVR_GOTO_IF_ERROR(eError, return_error_);
				}
			}
		}
#endif /* defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1 */
	}

#ifdef SUPPORT_POWER_SAMPLING_VIA_DEBUGFS
	{
		DI_ITERATOR_CB sIterator = {
			.pfnShow = _DebugPowerDataDIShow,
			.pfnWrite = PowerDataSet,
			//Expects '0' or '1' plus Null terminator
			.ui32WriteLenMax = ((1U)+1U)
		};
		eError = DICreateEntry("power_data", psDebugInfo->psGroup, &sIterator, psDeviceNode,
		                       DI_ENTRY_TYPE_GENERIC, &psDebugInfo->psPowerDataEntry);
		PVR_GOTO_IF_ERROR(eError, return_error_);
	}
#endif /* SUPPORT_POWER_SAMPLING_VIA_DEBUGFS */
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	{
		DI_ITERATOR_CB sIterator = {
			.pfnShow = PVRSRVPowerStatsPrintElements,
		};
		eError = DICreateEntry("power_timing_stats", psDebugInfo->psGroup, &sIterator, psDeviceNode,
		                       DI_ENTRY_TYPE_GENERIC, &psDebugInfo->psPowerTimingStatsEntry);
		PVR_GOTO_IF_ERROR(eError, return_error_);
	}
#endif
#endif /* SUPPORT_RGX */

	return PVRSRV_OK;

return_error_:
	DebugCommonDeInitDevice(psDeviceNode);

	return eError;
}

void DebugCommonDeInitDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
	__maybe_unused PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	if (psDebugInfo->psPowerTimingStatsEntry != NULL)
	{
		DIDestroyEntry(psDebugInfo->psPowerTimingStatsEntry);
		psDebugInfo->psPowerTimingStatsEntry = NULL;
	}
#endif

#ifdef SUPPORT_POWER_SAMPLING_VIA_DEBUGFS
	if (psDebugInfo->psPowerDataEntry != NULL)
	{
		DIDestroyEntry(psDebugInfo->psPowerDataEntry);
		psDebugInfo->psPowerDataEntry = NULL;
	}
#endif /* SUPPORT_POWER_SAMPLING_VIA_DEBUGFS */


#ifdef SUPPORT_RGX
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (PVRSRV_VZ_MODE_IS(HOST, DEVNODE, psDeviceNode))
	{
		IMG_UINT32 ui32DriverID;

		FOREACH_SUPPORTED_DRIVER(ui32DriverID)
		{
			if (psDebugInfo->apsVZDriverIsolationGroupDIEntries[ui32DriverID] != NULL)
			{
				DIDestroyEntry(psDebugInfo->apsVZDriverIsolationGroupDIEntries[ui32DriverID]);
				psDebugInfo->apsVZDriverIsolationGroupDIEntries[ui32DriverID] = NULL;
			}

			if (psDebugInfo->apsVZDriverPriorityDIEntries[ui32DriverID] != NULL)
			{
				DIDestroyEntry(psDebugInfo->apsVZDriverPriorityDIEntries[ui32DriverID]);
				psDebugInfo->apsVZDriverPriorityDIEntries[ui32DriverID] = NULL;
			}

			if (psDebugInfo->apsVZDriverTimeSliceDIEntries[ui32DriverID] != NULL)
			{
				DIDestroyEntry(psDebugInfo->apsVZDriverTimeSliceDIEntries[ui32DriverID]);
				psDebugInfo->apsVZDriverTimeSliceDIEntries[ui32DriverID] = NULL;
			}

			if (ui32DriverID == RGXFW_HOST_DRIVER_ID)
			{
				if (psDebugInfo->psVZDriverTimeSliceIntervalDIEntry != NULL)
				{
					DIDestroyEntry(psDebugInfo->psVZDriverTimeSliceIntervalDIEntry);
					psDebugInfo->psVZDriverTimeSliceIntervalDIEntry = NULL;
				}

				if (psDebugInfo->psVZDriverConnectionCooldownPeriodDIEntry != NULL)
				{
					DIDestroyEntry(psDebugInfo->psVZDriverConnectionCooldownPeriodDIEntry);
					psDebugInfo->psVZDriverConnectionCooldownPeriodDIEntry = NULL;
				}
			}

			if (psDebugInfo->apsVZDriverData[ui32DriverID] != NULL)
			{
				OSFreeMem(psDebugInfo->apsVZDriverData[ui32DriverID]);
				psDebugInfo->apsVZDriverData[ui32DriverID] = NULL;
			}

			if (psDebugInfo->apsVZDriverGroups[ui32DriverID] != NULL)
			{
				DIDestroyGroup(psDebugInfo->apsVZDriverGroups[ui32DriverID]);
				psDebugInfo->apsVZDriverGroups[ui32DriverID] = NULL;
			}
		}

		if (psDebugInfo->psVZGroup != NULL)
		{
			DIDestroyGroup(psDebugInfo->psVZGroup);
			psDebugInfo->psVZGroup = NULL;
		}
	}
#endif

	if (psDebugInfo->psFWTraceEntry != NULL)
	{
		DIDestroyEntry(psDebugInfo->psFWTraceEntry);
		psDebugInfo->psFWTraceEntry = NULL;
	}

#ifdef SUPPORT_FIRMWARE_GCOV
	if (psDebugInfo->psFWGCOVEntry != NULL)
	{
		DIDestroyEntry(psDebugInfo->psFWGCOVEntry);
		psDebugInfo->psFWGCOVEntry = NULL;
	}
#endif

	if (psDebugInfo->psFWMappingsEntry != NULL)
	{
		DIDestroyEntry(psDebugInfo->psFWMappingsEntry);
		psDebugInfo->psFWMappingsEntry = NULL;
	}

#if  defined(SUPPORT_RISCV_GDB)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR) &&
		(psDebugInfo->psRiscvDmiDIEntry != NULL))
	{
		DIDestroyEntry(psDebugInfo->psRiscvDmiDIEntry);
		psDebugInfo->psRiscvDmiDIEntry = NULL;
	}
#endif
#endif /* SUPPORT_RGX */

	if (psDebugInfo->psDumpDebugEntry != NULL)
	{
		DIDestroyEntry(psDebugInfo->psDumpDebugEntry);
		psDebugInfo->psDumpDebugEntry = NULL;
	}

#if defined(SUPPORT_RGX) && !defined(NO_HARDWARE)
	if (psDebugInfo->hGpuUtilUserDebugFS != NULL)
	{
		SORgxGpuUtilStatsUnregister(psDebugInfo->hGpuUtilUserDebugFS);
		psDebugInfo->hGpuUtilUserDebugFS = NULL;
	}
#endif /* defined(SUPPORT_RGX) && !defined(NO_HARDWARE) */

	if (psDebugInfo->psGroup != NULL)
	{
		DIDestroyGroup(psDebugInfo->psGroup);
		psDebugInfo->psGroup = NULL;
	}
}

/*
	Appends flags strings to a null-terminated string buffer
*/
void DebugCommonFlagStrings(IMG_CHAR *psDesc,
							IMG_UINT32 ui32DescSize,
							const IMG_FLAGS2DESC *psConvTable,
							IMG_UINT32 ui32TableSize,
							IMG_UINT32 ui32Flags)
{
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0; ui32Idx < ui32TableSize; ui32Idx++)
	{
		if (BITMASK_HAS(ui32Flags, psConvTable[ui32Idx].uiFlag))
		{
			OSStringLCat(psDesc, psConvTable[ui32Idx].pszLabel, ui32DescSize);
		}
	}
}

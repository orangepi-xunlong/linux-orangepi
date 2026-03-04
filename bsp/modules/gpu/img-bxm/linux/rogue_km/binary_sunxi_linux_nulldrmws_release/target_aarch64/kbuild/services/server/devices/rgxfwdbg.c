/*************************************************************************/ /*!
@File
@Title          Debugging and miscellaneous functions server implementation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Kernel services functions for debugging and other
                miscellaneous functionality.
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

#include "pvrsrv.h"
#include "pvr_debug.h"
#include "rgxfwdbg.h"
#include "rgxfwutils.h"
#include "rgxta3d.h"
#include "pdump_km.h"
#include "mmu_common.h"
#include "devicemem_server.h"
#include "osfunc.h"
#include "vmm_pvz_server.h"
#include "vz_vm.h"
#if defined(PDUMP)
#include "devicemem_pdump.h"
#endif

PVRSRV_ERROR
PVRSRVRGXFWDebugQueryFWLogKM(
	const CONNECTION_DATA *psConnection,
	const PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32 *pui32RGXFWLogType)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	if (!psDeviceNode || !pui32RGXFWLogType)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_IMPLEMENTED);
	psDevInfo = psDeviceNode->pvDevice;

	if (!psDevInfo || !psDevInfo->psRGXFWIfTraceBufCtl)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType, INVALIDATE);
	*pui32RGXFWLogType = psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType;
	return PVRSRV_OK;
}


PVRSRV_ERROR
PVRSRVRGXFWDebugSetFWLogKM(
	const CONNECTION_DATA * psConnection,
	const PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32RGXFWLogType)
{
	RGXFWIF_KCCB_CMD sLogTypeUpdateCmd;
	PVRSRV_DEV_POWER_STATE ePowerState;
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_RGXDEV_INFO* psDevInfo;
	IMG_UINT32 ui32OldRGXFWLogTpe;
	IMG_UINT32 ui32kCCBCommandSlot;
	IMG_BOOL bWaitForFwUpdate = IMG_FALSE;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	psDevInfo = psDeviceNode->pvDevice;
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType, INVALIDATE);
	ui32OldRGXFWLogTpe = psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType;

	/* check log type is valid */
	if (ui32RGXFWLogType & ~RGXFWIF_LOG_TYPE_MASK)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	OSLockAcquire(psDevInfo->hRGXFWIfBufInitLock);

	/* set the new log type and ensure the new log type is written to memory
	 * before requesting the FW to read it
	 */
	psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType = ui32RGXFWLogType;
	OSMemoryBarrier(&psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType, FLUSH);

	/* Allocate firmware trace buffer resource(s) if not already done */
	if (RGXTraceBufferIsInitRequired(psDevInfo))
	{
		eError = RGXTraceBufferInitOnDemandResources(psDevInfo, RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS);
	}
#if defined(SUPPORT_TBI_INTERFACE)
	/* Check if LogType is TBI then allocate resource on demand and copy
	 * SFs to it
	 */
	else if (RGXTBIBufferIsInitRequired(psDevInfo))
	{
		eError = RGXTBIBufferInitOnDemandResources(psDevInfo);
	}

	/* TBI buffer address will be 0 if not initialised */
	sLogTypeUpdateCmd.uCmdData.sTBIBuffer = psDevInfo->sRGXFWIfTBIBuffer;
#else
	sLogTypeUpdateCmd.uCmdData.sTBIBuffer.ui32Addr = 0;
#endif

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to allocate resource on-demand. Reverting to old value",
		         __func__));
		psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType = ui32OldRGXFWLogTpe;
		OSMemoryBarrier(&psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType);
		RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType, FLUSH);

		OSLockRelease(psDevInfo->hRGXFWIfBufInitLock);

		return eError;
	}

	OSLockRelease(psDevInfo->hRGXFWIfBufInitLock);

	eError = PVRSRVPowerLock((PPVRSRV_DEVICE_NODE) psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to acquire power lock (%u)",
		         __func__,
		         eError));
		return eError;
	}

	eError = PVRSRVGetDevicePowerState((PPVRSRV_DEVICE_NODE) psDeviceNode, &ePowerState);

	if ((eError == PVRSRV_OK) && (ePowerState != PVRSRV_DEV_POWER_STATE_OFF))
	{
		/* Ask the FW to update its cached version of logType value */
		sLogTypeUpdateCmd.eCmdType = RGXFWIF_KCCB_CMD_LOGTYPE_UPDATE;

		eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
											  &sLogTypeUpdateCmd,
											  PDUMP_FLAGS_CONTINUOUS,
											  &ui32kCCBCommandSlot);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSendCommandAndGetKCCBSlot", unlock);
		bWaitForFwUpdate = IMG_TRUE;
	}

unlock:
	PVRSRVPowerUnlock( (PPVRSRV_DEVICE_NODE) psDeviceNode);
	if (bWaitForFwUpdate)
	{
		/* Wait for the LogType value to be updated in FW */
		eError = RGXWaitForKCCBSlotUpdate(psDevInfo, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
		PVR_LOG_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate");
	}
	return eError;
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetHCSDeadlineKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32HCSDeadlineMS)
{
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	PVR_UNREFERENCED_PARAMETER(psConnection);

	return RGXFWSetHCSDeadline(psDevInfo, ui32HCSDeadlineMS);
}

PVRSRV_ERROR
PVRSRVRGXFWDebugMapGuestHeapKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32 ui32DriverID,
	IMG_UINT64 ui64GuestHeapBase)
{
#if defined(ENABLE_PVRDEBUG_PRIVILEGED_CMDS)
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32DeviceID = psDeviceNode->sDevId.ui32InternalID;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	if (PVRSRV_VZ_MODE_IS(HOST, DEVNODE, psDeviceNode))
	{
		if (ui64GuestHeapBase == IMG_UINT64_MAX)
		{
			/* unmap heap and set DriverID to offline */
			eError = PvzServerUnmapDevPhysHeap(ui32DriverID, ui32DeviceID);
			PVR_LOG_RETURN_IF_ERROR(eError, "PvzServerUnmapDevPhysHeap()");
			eError = PvzServerOnVmOffline(ui32DriverID, ui32DeviceID);
		}
		else
		{
			/* set DriverID online if necessary and map firmware heap */
			if (!IsVmOnline(ui32DriverID, ui32DeviceID))
			{
				eError = PvzServerOnVmOnline(ui32DriverID, ui32DeviceID);
				PVR_LOG_RETURN_IF_ERROR(eError, "PvzServerOnVmOnline()");
			}

			eError = PvzServerMapDevPhysHeap(ui32DriverID, ui32DeviceID, RGX_FIRMWARE_RAW_HEAP_SIZE, ui64GuestHeapBase);
		}
	}
	else
	{
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		PVR_DPF((PVR_DBG_ERROR, " %s: Driver must run in Host mode to support Guest Mapping operations\n", __func__));
	}

	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(ui32DriverID);
	PVR_UNREFERENCED_PARAMETER(ui64GuestHeapBase);

	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetDriverTimeSliceIntervalKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32TSIntervalMs)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_KCCB_CMD sVzTSIntervalCmd = { 0 };

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	if (psDevInfo->psRGXFWIfRuntimeCfg == NULL)
	{
		return PVRSRV_ERROR_NOT_INITIALISED;
	}

	sVzTSIntervalCmd.eCmdType = RGXFWIF_KCCB_CMD_VZ_DRV_TIME_SLICE_INTERVAL;
	psDevInfo->psRGXFWIfRuntimeCfg->ui32TSIntervalMs = ui32TSIntervalMs;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->ui32TSIntervalMs);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the timeslice interval inside RGXFWIfRuntimeCfg");
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, ui32TSIntervalMs),
							  ui32TSIntervalMs,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
									RGXFWIF_DM_GP,
									&sVzTSIntervalCmd,
									PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetDriverTimeSliceKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32DriverID,
	IMG_UINT32  ui32TSPercentage)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RGXFWIF_KCCB_CMD sVzTimeSliceCmd = { 0 };
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg;
	IMG_INT32 ui32TSPercentageMax = 0;
	IMG_UINT32 ui32DriverIDLoop;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	if (ui32DriverID >= RGX_NUM_DRIVERS_SUPPORTED)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = psDeviceNode->pvDevice;
	PVR_RETURN_IF_FALSE(psDevInfo != NULL, PVRSRV_ERROR_NOT_INITIALISED);

	psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVR_RETURN_IF_FALSE(psRuntimeCfg != NULL, PVRSRV_ERROR_NOT_INITIALISED);

	/*
	 * Each time slice is a number between 0 -> 100.
	 * Use '0' to disable time slice based CSW for the driver.
	 */
	 /* Check if the sum exceeds PVRSRV_VZ_TIME_SLICE_MAX */
	if (ui32TSPercentage)
	{
		PVR_RETURN_IF_FALSE(ui32TSPercentage <= PVRSRV_VZ_TIME_SLICE_MAX, PVRSRV_ERROR_INVALID_PARAMS);

		FOREACH_SUPPORTED_DRIVER(ui32DriverIDLoop)
		{
			if (ui32DriverID != ui32DriverIDLoop)
			{
				ui32TSPercentageMax += psRuntimeCfg->aui32TSPercentage[ui32DriverIDLoop];
			}
			else
			{
				ui32TSPercentageMax += ui32TSPercentage;
			}

			PVR_RETURN_IF_FALSE(ui32TSPercentageMax <= PVRSRV_VZ_TIME_SLICE_MAX, PVRSRV_ERROR_INVALID_PARAMS);
		}
	}

	sVzTimeSliceCmd.eCmdType = RGXFWIF_KCCB_CMD_VZ_DRV_TIME_SLICE;
	psDevInfo->psRGXFWIfRuntimeCfg->aui32TSPercentage[ui32DriverID] = ui32TSPercentage;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->aui32TSPercentage[ui32DriverID]);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->aui32TSPercentage[ui32DriverID], FLUSH);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the timeslice of DriverID %u inside RGXFWIfRuntimeCfg", ui32DriverID);
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, aui32TSPercentage) + (ui32DriverID * sizeof(ui32TSPercentage)),
							  ui32TSPercentage,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
									RGXFWIF_DM_GP,
									&sVzTimeSliceCmd,
									PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetDriverPriorityKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32DriverID,
	IMG_INT32   i32DriverPriority)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_KCCB_CMD sVzPriorityCmd = { 0 };

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	if (psDevInfo->psRGXFWIfRuntimeCfg == NULL)
	{
		return PVRSRV_ERROR_NOT_INITIALISED;
	}

	if (ui32DriverID >= RGX_NUM_DRIVERS_SUPPORTED)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if ((i32DriverPriority & ~RGXFW_VZ_PRIORITY_MASK) != 0)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sVzPriorityCmd.eCmdType = RGXFWIF_KCCB_CMD_VZ_DRV_ARRAY_CHANGE;
	psDevInfo->psRGXFWIfRuntimeCfg->ai32DriverPriority[ui32DriverID] = i32DriverPriority;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->ai32DriverPriority[ui32DriverID]);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->ai32DriverPriority[ui32DriverID], FLUSH);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the priority of DriverID %u inside RGXFWIfRuntimeCfg", ui32DriverID);
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, ai32DriverPriority) + (ui32DriverID * sizeof(i32DriverPriority)),
							  i32DriverPriority,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
									RGXFWIF_DM_GP,
									&sVzPriorityCmd,
									PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetDriverIsolationGroupKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32DriverID,
	IMG_UINT32  ui32DriverIsolationGroup)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_KCCB_CMD sVzIsolationGroupCmd = { 0 };

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	if (psDevInfo->psRGXFWIfRuntimeCfg == NULL)
	{
		return PVRSRV_ERROR_NOT_INITIALISED;
	}

	if (ui32DriverID >= RGX_NUM_DRIVERS_SUPPORTED)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sVzIsolationGroupCmd.eCmdType = RGXFWIF_KCCB_CMD_VZ_DRV_ARRAY_CHANGE;
	psDevInfo->psRGXFWIfRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID] = ui32DriverIsolationGroup;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID]);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID], FLUSH);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the isolation group of DriverID%u inside RGXFWIfRuntimeCfg", ui32DriverID);
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, aui32DriverIsolationGroup) + (ui32DriverID * sizeof(ui32DriverIsolationGroup)),
							  ui32DriverIsolationGroup,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
				RGXFWIF_DM_GP,
				&sVzIsolationGroupCmd,
				PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetOSNewOnlineStateKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32DriverID,
	IMG_UINT32  ui32OSNewState)
{
#if defined(ENABLE_PVRDEBUG_PRIVILEGED_CMDS)
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_OS_STATE_CHANGE eState;
	PVR_UNREFERENCED_PARAMETER(psConnection);

	if (ui32DriverID >= RGX_NUM_DRIVERS_SUPPORTED)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eState = (ui32OSNewState) ? (RGXFWIF_OS_ONLINE) : (RGXFWIF_OS_OFFLINE);
	return RGXFWSetFwOsState(psDevInfo, ui32DriverID, eState);
#else
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(ui32DriverID);
	PVR_UNREFERENCED_PARAMETER(ui32OSNewState);

	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

PVRSRV_ERROR
PVRSRVRGXFWDebugPHRConfigureKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32 ui32PHRMode)
{
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	PVR_UNREFERENCED_PARAMETER(psConnection);

	return RGXFWConfigPHR(psDevInfo,
	                      ui32PHRMode);
}

PVRSRV_ERROR
PVRSRVRGXFWDebugWdgConfigureKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32 ui32WdgPeriodUs)
{
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	PVR_UNREFERENCED_PARAMETER(psConnection);

	return RGXFWConfigWdg(psDevInfo,
	                      ui32WdgPeriodUs);
}

PVRSRV_ERROR
PVRSRVRGXFWDebugDumpFreelistPageListKM(
	CONNECTION_DATA * psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	DLLIST_NODE *psNode, *psNext;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	if (dllist_is_empty(&psDevInfo->sFreeListHead))
	{
		return PVRSRV_OK;
	}

	PVR_LOG(("---------------[ Begin Freelist Page List Dump ]------------------"));

	OSLockAcquire(psDevInfo->hLockFreeList);
	dllist_foreach_node(&psDevInfo->sFreeListHead, psNode, psNext)
	{
		RGX_FREELIST *psFreeList = IMG_CONTAINER_OF(psNode, RGX_FREELIST, sNode);
		RGXDumpFreeListPageList(psFreeList);
	}
	OSLockRelease(psDevInfo->hLockFreeList);

	PVR_LOG(("----------------[ End Freelist Page List Dump ]-------------------"));

	return PVRSRV_OK;

}

PVRSRV_ERROR
PVRSRVRGXFWDebugInjectFaultKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(ENABLE_PVRDEBUG_PRIVILEGED_CMDS)
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	PVR_UNREFERENCED_PARAMETER(psConnection);

	return RGXFWInjectFault(psDevInfo);
#else
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

PVRSRV_ERROR
PVRSRVRGXFWDebugPowerOffKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);

#if defined(ENABLE_PVRDEBUG_PRIVILEGED_CMDS)
#if defined(SUPPORT_AUTOVZ)
	psDeviceNode->bAutoVzFwIsUp = IMG_FALSE;
#endif

	return PVRSRVSetDeviceSystemPowerState(psDeviceNode,
					       PVRSRV_SYS_POWER_STATE_OFF,
					       PVRSRV_POWER_FLAGS_NONE);
#else
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

PVRSRV_ERROR
PVRSRVRGXFWDebugPowerOnKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(ENABLE_PVRDEBUG_PRIVILEGED_CMDS)
	PVR_UNREFERENCED_PARAMETER(psConnection);

	return PVRSRVSetDeviceSystemPowerState(psDeviceNode,
					       PVRSRV_SYS_POWER_STATE_ON,
					       PVRSRV_POWER_FLAGS_NONE);
#else
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

PVRSRV_ERROR
PVRSRVRGXFWDebugSetVzConnectionCooldownPeriodInSecKM(
	CONNECTION_DATA *psConnection,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32  ui32VzConnectionCooldownPeriodInSec)
{
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
	PVR_UNREFERENCED_PARAMETER(psConnection);

	return RGXFWSetVzConnectionCooldownPeriod(psDevInfo, ui32VzConnectionCooldownPeriodInSec);
}

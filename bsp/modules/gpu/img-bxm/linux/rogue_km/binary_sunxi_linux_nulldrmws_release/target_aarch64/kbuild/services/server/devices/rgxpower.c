/*************************************************************************/ /*!
@File
@Title          Device specific power routines
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

#if defined(__linux__)
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

#include "rgxpower.h"
#include "rgxinit.h"
#include "rgx_fwif_km.h"
#include "rgxfwutils.h"
#include "rgxfwriscv.h"
#include "pdump_km.h"
#include "pvr_debug.h"
#include "osfunc.h"
#include "rgxdebug_common.h"
#include "devicemem.h"
#include "devicemem_pdump.h"
#include "rgxtimecorr.h"
#include "devicemem_utils.h"
#include "htbserver.h"
#include "rgxstartstop.h"
#include "rgxfwimageutils.h"
#include "sync.h"
#include "rgxdefs_km.h"

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#endif
#if defined(SUPPORT_LINUX_DVFS)
#include "pvr_dvfs_device.h"
#endif

#if defined(PVRSRV_ANDROID_TRACE_GPU_FREQ)
#include "pvr_gpufreq.h"
#endif /* defined(PVRSRV_ANDROID_TRACE_GPU_FREQ) */

#if defined(SUPPORT_PDVFS) && (PDVFS_COM == PDVFS_COM_HOST)
#include "rgxpdvfs.h"
#endif

static PVRSRV_ERROR RGXFWNotifyHostTimeout(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXFWIF_KCCB_CMD sCmd;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32CmdKCCBSlot;

	/* Send the Timeout notification to the FW */
	sCmd.eCmdType = RGXFWIF_KCCB_CMD_POW;
	sCmd.uCmdData.sPowData.ePowType = RGXFWIF_POW_FORCED_IDLE_REQ;
	sCmd.uCmdData.sPowData.uPowerReqData.ePowRequestType = RGXFWIF_POWER_HOST_TIMEOUT;

	eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
	                                      &sCmd,
	                                      PDUMP_FLAGS_NONE,
	                                      &ui32CmdKCCBSlot);

	return eError;
}

static void _RGXUpdateGPUUtilStats(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXFWIF_GPU_UTIL_FW *psUtilFW;
	IMG_UINT64 ui64LastPeriod;
	IMG_UINT64 ui64LastState;
	IMG_UINT64 ui64LastReducedState;
	IMG_UINT64 ui64LastTime;
	IMG_UINT64 ui64TimeNow;
	IMG_UINT32 ui32DriverID;
	IMG_UINT64 ui64DMOSStatsCounter;

	psUtilFW = psDevInfo->psRGXFWIfGpuUtilFW;
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfGpuUtilFW, INVALIDATE);

	OSLockAcquire(psDevInfo->hGPUUtilLock);

	ui64TimeNow = RGXFWIF_GPU_UTIL_GET_TIME(RGXTimeCorrGetClockns64(psDevInfo->psDeviceNode));

	/* Update counters to account for the time since the last update */
	ui64LastState  = RGXFWIF_GPU_UTIL_GET_STATE(psUtilFW->ui64GpuLastWord);
	ui64LastTime   = RGXFWIF_GPU_UTIL_GET_TIME(psUtilFW->ui64GpuLastWord);
	ui64LastPeriod = RGXFWIF_GPU_UTIL_GET_PERIOD(ui64TimeNow, ui64LastTime);
	psUtilFW->aui64GpuStatsCounters[ui64LastState] += ui64LastPeriod;

	/* Update state and time of the latest update */
	psUtilFW->ui64GpuLastWord = RGXFWIF_GPU_UTIL_MAKE_WORD(ui64TimeNow, ui64LastState);

	/* convert last period into the same units as used by fw */
	ui64TimeNow  = ui64TimeNow >> RGXFWIF_DM_OS_TIMESTAMP_SHIFT;

	FOREACH_SUPPORTED_DRIVER(ui32DriverID)
	{
		RGXFWIF_GPU_STATS *psStats = &psUtilFW->sStats[ui32DriverID];
		RGXFWIF_DM eDM;

		for (eDM = 0; eDM < RGXFWIF_GPU_UTIL_DM_MAX; eDM++)
		{
			ui64LastState  = (IMG_UINT64)RGXFWIF_GPU_UTIL_GET_STATE32(psStats->aui32DMOSLastWord[eDM]);
			ui64LastTime   = (IMG_UINT64)RGXFWIF_GPU_UTIL_GET_TIME32(psStats->aui32DMOSLastWord[eDM]) +
			                 ((IMG_UINT64)psStats->aui32DMOSLastWordWrap[eDM] << 32);
			ui64LastPeriod = RGXFWIF_GPU_UTIL_GET_PERIOD(ui64TimeNow, ui64LastTime);
			/* for states statistics per DM per driver we only care about the time in Active state,
			so we "combine" other states (Idle and Blocked) together */
			ui64LastReducedState = (ui64LastState == RGXFWIF_GPU_UTIL_STATE_ACTIVE) ?
			                       RGXFWIF_GPU_UTIL_STATE_ACTIVE : RGXFWIF_GPU_UTIL_STATE_INACTIVE;
			ui64DMOSStatsCounter = (IMG_UINT64)psStats->aaui32DMOSStatsCounters[eDM][ui64LastReducedState] + ui64LastPeriod;
			psStats->aaui32DMOSStatsCounters[eDM][ui64LastReducedState] = (IMG_UINT32)(ui64DMOSStatsCounter & IMG_UINT32_MAX);
			if (ui64DMOSStatsCounter > IMG_UINT32_MAX)
			{
				psStats->aaui32DMOSCountersWrap[eDM][ui64LastReducedState] += (IMG_UINT32)(ui64DMOSStatsCounter >> 32);
			}

			/* Update state and time of the latest update */
			psStats->aui32DMOSLastWord[eDM] = RGXFWIF_GPU_UTIL_MAKE_WORD32((ui64TimeNow & (IMG_UINT64)IMG_UINT32_MAX), ui64LastState);
			if (ui64TimeNow > IMG_UINT32_MAX)
			{
				if (psStats->aui32DMOSLastWordWrap[eDM] != (IMG_UINT32)(ui64TimeNow >> 32))
				{
					psStats->aui32DMOSLastWordWrap[eDM] = (IMG_UINT32)(ui64TimeNow >> 32);
				}
			}
		}
	}
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfGpuUtilFW, FLUSH);

	OSLockRelease(psDevInfo->hGPUUtilLock);
}

static INLINE PVRSRV_ERROR RGXDoStop(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	if (psDeviceNode->psDevConfig->pfnTDRGXStop == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "RGXPrePowerState: TDRGXStop not implemented!"));
		return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}

	psDevInfo->bRGXPowered = IMG_FALSE;
	eError = psDeviceNode->psDevConfig->pfnTDRGXStop(psDeviceNode->psDevConfig->hSysData);
#else
	eError = RGXStop(&psDevInfo->sLayerParams);
#endif

	return eError;
}

/*************************************************************************/ /*!
@Function       RGXSendPowerOffKick
@Description    Send a KCCB kick to power off the GPU FW. This function will wait
                for completion of the command before exiting.

@Input          psDeviceNode    The device node struct associated with the GPU.
@Input          bForce          A boolean indicating if the power off command
                                should be forced.

@Return         Failure code if the virtual address is invalid.
*/ /**************************************************************************/
static PVRSRV_ERROR RGXSendPowerOffKick(PVRSRV_DEVICE_NODE *psDeviceNode,
                                        IMG_BOOL bForce)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	RGXFWIF_KCCB_CMD sPowCmd;
	IMG_UINT32 ui32CmdKCCBSlot;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_ASSERT(psDeviceNode != NULL);
	PVR_ASSERT(psDeviceNode->pvDevice != NULL);

	psDevInfo = psDeviceNode->pvDevice;

	/* Send the Power off request to the FW */
	sPowCmd.eCmdType = RGXFWIF_KCCB_CMD_POW;
	sPowCmd.uCmdData.sPowData.ePowType = RGXFWIF_POW_OFF_REQ;
	sPowCmd.uCmdData.sPowData.uPowerReqData.bForced = bForce;

	eError = SyncPrimSet(psDevInfo->psPowSyncPrim, 0);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set Power sync prim",
		         __func__));
		return eError;
	}

	eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
	                                      &sPowCmd,
	                                      PDUMP_FLAGS_NONE,
	                                      &ui32CmdKCCBSlot);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to send Power off request",
		         __func__));
		return eError;
	}

	/* Wait for the firmware to complete processing. It cannot use PVRSRVWaitForValueKM as it relies
	 * on the EventObject which is signalled in this MISR */
	return RGXPollForGPCommandCompletion(psDeviceNode,
	                                     psDevInfo->psPowSyncPrim->pui32LinAddr,
	                                     0x1, 0xFFFFFFFF);
}

/*************************************************************************/ /*!
@Function       RGXFinalisePowerOff
@Description    Finalises the GPU power transition to off.

@Input          psDeviceNode    The device node struct associated with the GPU.

@Return         Result code indicating the success or reason for failure.
*/ /**************************************************************************/
static PVRSRV_ERROR RGXFinalisePowerOff(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	PVRSRV_ERROR eError = PVRSRV_OK;

#if !defined(NO_HARDWARE) && !defined(SUPPORT_SYNC_IRQ)
	IMG_UINT32 ui32idx;
#endif

	PVR_ASSERT(psDeviceNode != NULL);
	PVR_ASSERT(psDeviceNode->pvDevice != NULL);

	psDevInfo = psDeviceNode->pvDevice;

#if !defined(NO_HARDWARE)
	/* Driver takes the VZ Fw-KM connection down, preventing the
		* firmware from submitting further interrupts */
	KM_SET_OS_CONNECTION(OFFLINE, psDevInfo);
	KM_CONNECTION_CACHEOP(Os, FLUSH);

#if defined(SUPPORT_SYNC_IRQ)
	/* Wait for the pending IRQ handlers to complete. */
	OSSyncIRQ(psDeviceNode->psDevConfig->ui32IRQ);
#else
#if defined(RGX_FW_IRQ_OS_COUNTERS)
	ui32idx = RGXFW_HOST_DRIVER_ID;
#else
	for_each_irq_cnt(ui32idx)
#endif /* RGX_FW_IRQ_OS_COUNTERS */
	{
		IMG_UINT32 ui32IrqCnt;

		get_irq_cnt_val(ui32IrqCnt, ui32idx, psDevInfo);

		/* Wait for the pending FW processor to host interrupts to come back. */
		eError = PVRSRVPollForValueKM(psDeviceNode,
		                              (IMG_UINT32 __iomem *)&psDevInfo->aui32SampleIRQCount[ui32idx],
		                              ui32IrqCnt,
		                              0xffffffff,
		                              POLL_FLAG_LOG_ERROR | POLL_FLAG_DEBUG_DUMP,
		                              NULL);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Wait for pending interrupts failed (DevID %u)." MSG_IRQ_CNT_TYPE " %u Host: %u, FW: %u",
			         __func__,
			         psDeviceNode->sDevId.ui32InternalID,
			         ui32idx,
			         psDevInfo->aui32SampleIRQCount[ui32idx],
			         ui32IrqCnt));

			RGX_WaitForInterruptsTimeout(psDevInfo);
#if !defined(RGX_FW_IRQ_OS_COUNTERS)
			break;
#endif
		}
	}
#endif /* SUPPORT_SYNC_IRQ */
#endif /* NO_HARDWARE */

	/* Update GPU frequency and timer correlation related data */
	RGXTimeCorrEnd(psDeviceNode, RGXTIMECORR_EVENT_POWER);

	/* Update GPU state counters */
	_RGXUpdateGPUUtilStats(psDevInfo);

#if defined(SUPPORT_LINUX_DVFS)
	eError = SuspendDVFS(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to suspend DVFS", __func__));
		return eError;
	}
#endif

	eError = RGXDoStop(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		/* Power down failures are treated as successful since the power was removed but logged. */
		PVR_DPF((PVR_DBG_WARNING, "%s: RGXDoStop failed (%s)",
		         __func__, PVRSRVGetErrorString(eError)));
		psDevInfo->ui32ActivePMReqNonIdle++;
		eError = PVRSRV_OK;
	}

	return eError;
}

/*************************************************************************/ /*!
@Function       RGXPrePowerState
@Description    Initial step for setting power state, to be followed by
                RGXPostPowerState.

@Input          psDeviceNode        The device node struct associated with the GPU.
@Input          eNewPowerState      The power state the GPU is to transition to.
@Input          eCurrentPowerState  The current power state of the GPU.
@Input          ePwrFlags           Flags indicating the behaviour of the transition.

@Return         Result code indicating the success or reason for failure.
*/ /**************************************************************************/
PVRSRV_ERROR RGXPrePowerState(PVRSRV_DEVICE_NODE *psDeviceNode,
                              PVRSRV_DEV_POWER_STATE eNewPowerState,
                              PVRSRV_DEV_POWER_STATE eCurrentPowerState,
                              PVRSRV_POWER_FLAGS ePwrFlags)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;
	const RGXFWIF_SYSDATA *psFwSysData;
#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	IMG_BOOL              bDeviceOk;
#endif
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_ASSERT(psDevInfo != NULL);

	psFwSysData = psDevInfo->psRGXFWIfFwSysData;

	if ((eNewPowerState == eCurrentPowerState) ||
	    (eNewPowerState == PVRSRV_DEV_POWER_STATE_ON))
	{
		return PVRSRV_OK;
	}

#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	bDeviceOk = (OSAtomicRead(&psDeviceNode->eHealthStatus) == PVRSRV_DEVICE_HEALTH_STATUS_OK);
	if (bDeviceOk)
#endif
	{
		IMG_BOOL bForce = IMG_FALSE;

		if (BITMASK_HAS(ePwrFlags, PVRSRV_POWER_FLAGS_FORCED))
		{
			bForce = IMG_TRUE;
		}

		eError = RGXSendPowerOffKick(psDeviceNode, bForce);
		if (eError == PVRSRV_ERROR_TIMEOUT)
		{
			/* timeout waiting for the FW to ack the request: return timeout */
			PVR_DPF((PVR_DBG_WARNING,
			         "%s: Timeout waiting for powoff ack from the FW",
			         __func__));
			return eError;
		}
		else if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Error waiting for powoff ack from the FW (%s)",
			         __func__, PVRSRVGetErrorString(eError)));
			return PVRSRV_ERROR_DEVICE_POWER_CHANGE_FAILURE;
		}
	}

	/* Check the Power state after the answer */
	RGXFwSharedMemCacheOpValue(psFwSysData->ePowState, INVALIDATE);
	if ((psFwSysData->ePowState != RGXFWIF_POW_OFF)
#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	    && (bDeviceOk)
#endif
		)
	{
		if (BITMASK_HAS(ePwrFlags, PVRSRV_POWER_FLAGS_FORCED))
		{	/* It is an error for a forced request to be denied */
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Failure to power off during a forced power off. FW: %d",
			         __func__, psFwSysData->ePowState));
		}

		/* the sync was updated but the pow state isn't off -> the FW denied the transition */
		return PVRSRV_ERROR_DEVICE_POWER_CHANGE_DENIED;
	}

	return RGXFinalisePowerOff(psDeviceNode);
}

#if defined(SUPPORT_AUTOVZ)
static PVRSRV_ERROR _RGXWaitForGuestsToDisconnect(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR eError = PVRSRV_ERROR_TIMEOUT;
	IMG_UINT32 ui32FwTimeout = (20 * SECONDS_TO_MICROSECONDS);

	LOOP_UNTIL_TIMEOUT_US(ui32FwTimeout)
	{
		IMG_UINT32 ui32DriverID;
		IMG_BOOL bGuestOnline = IMG_FALSE;

		RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->asOsRuntimeFlagsMirror,
		                           INVALIDATE);

		for (ui32DriverID = RGXFW_GUEST_DRIVER_ID_START;
			 ui32DriverID < RGX_NUM_DRIVERS_SUPPORTED; ui32DriverID++)
		{
			RGXFWIF_CONNECTION_FW_STATE eGuestState = (RGXFWIF_CONNECTION_FW_STATE)
					psDevInfo->psRGXFWIfFwSysData->asOsRuntimeFlagsMirror[ui32DriverID].bfOsState;

			if ((eGuestState == RGXFW_CONNECTION_FW_ACTIVE) ||
				(eGuestState == RGXFW_CONNECTION_FW_GRACEFUL_OFFLOAD) ||
				(eGuestState == RGXFW_CONNECTION_FW_FORCED_OFFLOAD))
			{
				bGuestOnline = IMG_TRUE;
				PVR_DPF((PVR_DBG_WARNING, "%s: Guest OS %u still online.", __func__, ui32DriverID));
			}
		}

		if (!bGuestOnline)
		{
			/* Allow Guests to finish reading Connection state registers before disconnecting. */
			OSSleepms(100);

			PVR_DPF((PVR_DBG_WARNING, "%s: All Guest connections are down. "
									  "Host can power down the GPU.", __func__));
			eError = PVRSRV_OK;
			break;
		}
		else
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Waiting for Guests to disconnect "
									  "before powering down GPU.", __func__));

			if (PVRSRVPwrLockIsLockedByMe(psDeviceNode))
			{
				/* Don't wait with the power lock held as this prevents the vz
				 * watchdog thread from keeping the fw-km connection alive. */
				PVRSRVPowerUnlock(psDeviceNode);
			}
		}

		OSSleepms(10);
	} END_LOOP_UNTIL_TIMEOUT_US();

	if (!PVRSRVPwrLockIsLockedByMe(psDeviceNode))
	{
		/* Take back power lock after waiting for Guests */
		eError = PVRSRVPowerLock(psDeviceNode);
	}

	return eError;
}
#endif /* defined(SUPPORT_AUTOVZ) */

/*
	RGXVzPrePowerState
*/
PVRSRV_ERROR RGXVzPrePowerState(PVRSRV_DEVICE_NODE		*psDeviceNode,
                                PVRSRV_DEV_POWER_STATE	eNewPowerState,
                                PVRSRV_DEV_POWER_STATE	eCurrentPowerState,
                                PVRSRV_POWER_FLAGS		ePwrFlags)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_LOG_RETURN_IF_FALSE((eNewPowerState != eCurrentPowerState), "no power change", eError);

	if (eNewPowerState != PVRSRV_DEV_POWER_STATE_ON)
	{
		/* powering down */
#if defined(SUPPORT_AUTOVZ)
		if (PVRSRV_VZ_MODE_IS(HOST, DEVNODE, psDeviceNode) && (!psDeviceNode->bAutoVzFwIsUp || psDeviceNode->bAutoVzAllowGPUPowerdown))
		{
			if (psDeviceNode->bAutoVzFwIsUp)
			{
				/* bAutoVzAllowGPUPowerdown must be TRUE here and
				 * bAutoVzFwIsUp=TRUE indicates that this is a powerdown event
				 * so send requests to the FW to disconnect all guest connections
				 * before GPU is powered down. */
				eError = RGXDisconnectAllGuests(psDeviceNode);
				PVR_LOG_RETURN_IF_ERROR(eError, "RGXDisconnectAllGuests");
			}

			/* The Host must ensure all Guest drivers have disconnected from the GPU before powering it down.
			 * Guest drivers regularly access hardware registers during runtime. If an attempt is made to
			 * access a GPU register while the GPU is down, the SoC might lock up. */
			eError = _RGXWaitForGuestsToDisconnect(psDeviceNode);
			PVR_LOG_RETURN_IF_ERROR(eError, "_RGXWaitForGuestsToDisconnect");

			if (psDeviceNode->bAutoVzAllowGPUPowerdown)
			{
				psDeviceNode->bAutoVzFwIsUp = IMG_FALSE;
			}

			/* Temporarily restore all power callbacks used by the driver to fully power down the GPU.
			 * Under AutoVz, power transitions requests (e.g. on driver deinitialisation and unloading)
			 * are generally ignored and the GPU power state is unaffected. Special power requests like
			 * those triggered by Suspend/Resume calls must reinstate the callbacks when needed. */
			PVRSRVSetPowerCallbacks(psDeviceNode, psDeviceNode->psPowerDev,
									&RGXVzPrePowerState, &RGXVzPostPowerState,
									psDeviceNode->psDevConfig->pfnPrePowerState,
									psDeviceNode->psDevConfig->pfnPostPowerState,
									&RGXForcedIdleRequest, &RGXCancelForcedIdleRequest);
		}
		else
		{
			PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

			KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
			KM_CONNECTION_CACHEOP(Os, INVALIDATE);

			if (KM_FW_CONNECTION_IS(ACTIVE, psDevInfo) &&
				KM_OS_CONNECTION_IS(ACTIVE, psDevInfo))
			{
				PVRSRV_ERROR eError = RGXFWSetFwOsState(psDevInfo, 0, RGXFWIF_OS_OFFLINE);
				PVR_LOG_RETURN_IF_ERROR(eError, "RGXFWSetFwOsState");
			}
		}
#endif
		PVR_DPF((PVR_DBG_WARNING, "%s: %s driver powering down: bAutoVzFwIsUp = %s",
								__func__, PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode)? "GUEST" : "HOST",
								psDeviceNode->bAutoVzFwIsUp ? "TRUE" : "FALSE"));
	}
	else if (eCurrentPowerState != PVRSRV_DEV_POWER_STATE_ON)
	{
		/* powering up */
		PVR_DPF((PVR_DBG_WARNING, "%s: %s driver powering up: bAutoVzFwIsUp = %s",
								__func__, PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode)? "GUEST" : "HOST",
								psDeviceNode->bAutoVzFwIsUp ? "TRUE" : "FALSE"));

	}

	if (!(PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) || (psDeviceNode->bAutoVzFwIsUp)))
	{
		/* call regular device power function */
		eError = RGXPrePowerState(psDeviceNode, eNewPowerState, eCurrentPowerState, ePwrFlags);
	}

	return eError;
}

#if defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
static PVRSRV_ERROR RGXVzWaitFirmwareReady(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
	if (!KM_FW_CONNECTION_IS(READY, psDevInfo))
	{
		PVR_LOG(("%s: Firmware Connection is not in Ready state. Waiting for Firmware ...", __func__));
	}

	LOOP_UNTIL_TIMEOUT_US(RGX_VZ_CONNECTION_TIMEOUT_US)
	{
		KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
		if (KM_FW_CONNECTION_IS(READY, psDevInfo))
		{
			PVR_LOG(("%s: Firmware Connection is Ready. Initialisation proceeding.", __func__));
			break;
		}
		else
		{
			OSSleepms(10);
		}
	} END_LOOP_UNTIL_TIMEOUT_US();

	KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
	if (!KM_FW_CONNECTION_IS(READY, psDevInfo))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Timed out waiting for the Firmware to enter Ready state.", __func__));
		return PVRSRV_ERROR_TIMEOUT;
	}

	return PVRSRV_OK;
}
#endif

/*
	RGXVzPostPowerState
*/
PVRSRV_ERROR RGXVzPostPowerState(PVRSRV_DEVICE_NODE		*psDeviceNode,
                                 PVRSRV_DEV_POWER_STATE	eNewPowerState,
                                 PVRSRV_DEV_POWER_STATE	eCurrentPowerState,
                                 PVRSRV_POWER_FLAGS		ePwrFlags)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PVR_LOG_RETURN_IF_FALSE((eNewPowerState != eCurrentPowerState), "no power change", eError);

	if (!(PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) || (psDeviceNode->bAutoVzFwIsUp)))
	{
		if (eCurrentPowerState != PVRSRV_DEV_POWER_STATE_ON)
		{
			KM_SET_OS_CONNECTION(READY, psDevInfo);
			KM_CONNECTION_CACHEOP(Os, FLUSH);
		}
		/* call regular device power function */
		eError = RGXPostPowerState(psDeviceNode, eNewPowerState, eCurrentPowerState, ePwrFlags);
	}
	else
	{
		KM_SET_OS_CONNECTION(OFFLINE, psDevInfo);
		KM_CONNECTION_CACHEOP(Os, FLUSH);
	}

	if (eNewPowerState != PVRSRV_DEV_POWER_STATE_ON)
	{
		/* powering down */
		if (psDeviceNode->bAutoVzFwIsUp)
		{
			PVR_LOG(("%s: AutoVz Fw active, power not changed", __func__));
			return eError;
		}
		PVR_DPF((PVR_DBG_WARNING, "%s: %s driver powering down: bAutoVzFwIsUp = %s",
								__func__, PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode)? "GUEST" : "HOST",
								psDeviceNode->bAutoVzFwIsUp ? "TRUE" : "FALSE"));

#if !defined(SUPPORT_AUTOVZ_HW_REGS)
		/* The connection states must be reset on a GPU power cycle. If the states are kept
		 * in hardware scratch registers, they will be cleared on power down. When using shared
		 * memory the connection data must be explicitly cleared by the driver. */
		OSCachedMemSetWMB(psDevInfo->psRGXFWIfConnectionCtl, 0, sizeof(RGXFWIF_CONNECTION_CTL));
		RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfConnectionCtl, FLUSH);
#endif /* defined(SUPPORT_AUTOVZ) && !defined(SUPPORT_AUTOVZ_HW_REGS) */

		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) || (psDeviceNode->bAutoVzFwIsUp))
		{
#if defined(SUPPORT_AUTOVZ)
			/* AutoVz Guests attempting to suspend have updated their connections earlier in RGXVzPrePowerState.
			 * Skip this redundant register write, as the Host could have powered down the GPU by now. */
			if (psDeviceNode->bAutoVzFwIsUp)
#endif
			{
				/* Take the VZ connection down to prevent firmware from submitting further interrupts */
				KM_SET_OS_CONNECTION(OFFLINE, psDevInfo);
				KM_CONNECTION_CACHEOP(Os, FLUSH);
			}
			/* Power transition callbacks were not executed, update RGXPowered flag here */
			psDevInfo->bRGXPowered = IMG_FALSE;
		}
	}
	else if (eCurrentPowerState != PVRSRV_DEV_POWER_STATE_ON)
	{
		/* powering up */
		IMG_UINT32 ui32FwTimeout = (3 * SECONDS_TO_MICROSECONDS);
		volatile IMG_BOOL *pbUpdatedFlag;
		RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated,
		                           INVALIDATE);
		pbUpdatedFlag = &psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated;

		PVR_DPF((PVR_DBG_WARNING, "%s: %s driver powering up: bAutoVzFwIsUp = %s",
								__func__, PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode)? "GUEST" : "HOST",
								psDeviceNode->bAutoVzFwIsUp ? "TRUE" : "FALSE"));
		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			/* Guests don't execute the power transition callbacks, so update their RGXPowered flag here */
			psDevInfo->bRGXPowered = IMG_TRUE;

#if defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
			/* Guest drivers expect the firmware to have set its end of the
			 * connection to Ready state by now. */
			eError = RGXVzWaitFirmwareReady(psDevInfo);
			PVR_LOG_RETURN_IF_ERROR(eError, "RGXVzWaitFirmwareReady()");
#endif /* RGX_VZ_STATIC_CARVEOUT_FW_HEAPS */

			OSWriteDeviceMem32WithWMB(pbUpdatedFlag, IMG_FALSE);

			/* Kick an initial dummy command to make the firmware initialise all
			 * its internal guest OS data structures and compatibility information.
			 * Use the lower-level RGXSendCommand() for the job, to make
			 * sure only 1 KCCB command is issued to the firmware.
			 * The default RGXFWHealthCheckCmd() prefaces each HealthCheck command with
			 * a pre-kick cache command which can interfere with the FW-KM init handshake. */
			{
				RGXFWIF_KCCB_CMD sCmpKCCBCmd;
				sCmpKCCBCmd.eCmdType = RGXFWIF_KCCB_CMD_HEALTH_CHECK;

				KM_SET_OS_CONNECTION(READY, psDevInfo);
				KM_CONNECTION_CACHEOP(Os, FLUSH);

				eError = RGXSendCommand(psDevInfo, &sCmpKCCBCmd, PDUMP_FLAGS_CONTINUOUS);
				PVR_LOG_RETURN_IF_ERROR(eError, "RGXSendCommand()");
			}
		}
		else
		{
#if defined(SUPPORT_AUTOVZ)
			/* Disable power callbacks that should not be run on virtualised drivers after the GPU
			 * is fully initialised: system layer pre/post functions and driver idle requests.
			 * The original device RGX Pre/Post functions are called from this Vz wrapper. */
			PVRSRVSetPowerCallbacks(psDeviceNode, psDeviceNode->psPowerDev,
									&RGXVzPrePowerState, &RGXVzPostPowerState,
									NULL, NULL, NULL, NULL);

			/* AutoVz Host driver reconnecting to running Firmware */
			if (psDeviceNode->bAutoVzFwIsUp)
			{
				/* Firmware already running, send a KCCB command to establish the new connection */
				RGXFWIF_KCCB_CMD sCmpKCCBCmd;
				sCmpKCCBCmd.eCmdType = RGXFWIF_KCCB_CMD_HEALTH_CHECK;

				eError = RGXVzWaitFirmwareReady(psDevInfo);
				PVR_LOG_RETURN_IF_ERROR(eError, "RGXVzWaitFirmwareReady()");

				KM_SET_OS_CONNECTION(READY, psDevInfo);
				KM_CONNECTION_CACHEOP(Os, FLUSH);

				eError = RGXSendCommand(psDevInfo, &sCmpKCCBCmd, PDUMP_FLAGS_CONTINUOUS);
				PVR_LOG_RETURN_IF_ERROR(eError, "RGXSendCommand()");
			}
			else
			{
				/* During first-time boot the flag is set here, while subsequent reboots will already
				 * have set it earlier in RGXInit. Set to true from this point on. */
				psDeviceNode->bAutoVzFwIsUp = IMG_TRUE;
			}
#endif
		}

		/* Wait for the firmware to accept and enable the connection with this OS by setting its state to Active */
		LOOP_UNTIL_TIMEOUT_US(RGX_VZ_CONNECTION_TIMEOUT_US)
		{
			KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
			if (KM_FW_CONNECTION_IS(ACTIVE, psDevInfo))
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Firmware Connection is Active. Initialisation proceeding.", __func__));
				break;
			}
			else
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Firmware Connection is not in Active state. Waiting for Firmware ...", __func__));
				OSSleepms(10);
			}
		} END_LOOP_UNTIL_TIMEOUT_US();

		KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
		if (!KM_FW_CONNECTION_IS(ACTIVE, psDevInfo))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Timed out waiting for the Firmware to enter Active state.", __func__));
			return PVRSRV_ERROR_TIMEOUT;
		}

		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			/* poll on the Firmware supplying the compatibility data */
			LOOP_UNTIL_TIMEOUT_US(ui32FwTimeout)
			{
				if (*pbUpdatedFlag)
				{
					break;
				}
				OSSleepms(10);
			} END_LOOP_UNTIL_TIMEOUT_US();

			PVR_LOG_RETURN_IF_FALSE(*pbUpdatedFlag, "Firmware does not respond with compatibility data.", PVRSRV_ERROR_TIMEOUT);
		}

		KM_SET_OS_CONNECTION(ACTIVE, psDevInfo);
		KM_CONNECTION_CACHEOP(Os, FLUSH);
	}

	return PVRSRV_OK;
}

#if defined(TRACK_FW_BOOT)
static INLINE void RGXCheckFWBootStage(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	FW_BOOT_STAGE eStage;

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
	{
		/* Boot stage temporarily stored to the register below */
		eStage = OSReadHWReg32(psDevInfo->pvRegsBaseKM,
		                       RGX_FW_BOOT_STAGE_REGISTER);
	}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		IMG_BYTE *pbBootData;

		if (PVRSRV_OK != DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWDataMemDesc,
		                                          (void**)&pbBootData))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Could not acquire pointer to FW boot stage", __func__));
			eStage = FW_BOOT_STAGE_NOT_AVAILABLE;
		}
		else
		{
			pbBootData += RGXGetFWImageSectionOffset(NULL, MIPS_BOOT_DATA);

			eStage = *(FW_BOOT_STAGE*)&pbBootData[RGXMIPSFW_BOOT_STAGE_OFFSET];

			if (eStage == FW_BOOT_STAGE_TLB_INIT_FAILURE)
			{
				RGXMIPSFW_BOOT_DATA *psBootData =
					(RGXMIPSFW_BOOT_DATA*) (pbBootData + RGXMIPSFW_BOOTLDR_CONF_OFFSET);

				PVR_LOG(("MIPS TLB could not be initialised. Boot data info:"
						 " num PT pages %u, log2 PT page size %u, PT page addresses"
						 " %"IMG_UINT64_FMTSPECx " %"IMG_UINT64_FMTSPECx
						 " %"IMG_UINT64_FMTSPECx " %"IMG_UINT64_FMTSPECx,
						 psBootData->ui32PTNumPages,
						 psBootData->ui32PTLog2PageSize,
						 psBootData->aui64PTPhyAddr[0U],
						 psBootData->aui64PTPhyAddr[1U],
						 psBootData->aui64PTPhyAddr[2U],
						 psBootData->aui64PTPhyAddr[3U]));
			}

			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWDataMemDesc);
		}
	}
#endif
	else
	{
		eStage = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_SCRATCH14);
	}

	PVR_LOG(("%s: FW reached boot stage %i/%i.",
	         __func__, eStage, FW_BOOT_INIT_DONE));
}
#endif

static INLINE PVRSRV_ERROR RGXDoStart(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDeviceNode->psDevConfig;

	if (psDevConfig->pfnTDRGXStart == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "RGXPostPowerState: TDRGXStart not implemented!"));
		return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}

	eError = psDevConfig->pfnTDRGXStart(psDevConfig->hSysData);

	if (eError == PVRSRV_OK)
	{
		psDevInfo->bRGXPowered = IMG_TRUE;
	}
#else
	eError = RGXStart(&psDevInfo->sLayerParams);
#endif

	return eError;
}

#if defined(SUPPORT_GPUVIRT_VALIDATION_MTS)
/*
 * To validate the MTS unit we do the following:
 *  - Immediately after firmware loading for each OSID
 *    - Write the OSid to a memory location shared with FW
 *    - Kick the register of that OSid
 *         (Uncounted, DM 0)
 *    - FW clears the memory location if OSid matches
 *    - Host checks that memory location is cleared
 *
 *  See firmware/rgxfw_bg.c
 */
static PVRSRV_ERROR RGXVirtualisationPowerupSidebandTest(PVRSRV_DEVICE_NODE	 *psDeviceNode,
														 RGXFWIF_SYSINIT *psFwSysInit,
														 PVRSRV_RGXDEV_INFO	 *psDevInfo)
{
	IMG_UINT32 ui32ScheduleRegister;
	IMG_UINT32 ui32OSid;
	IMG_UINT32 ui32KickType;
	IMG_UINT32 ui32OsRegBanksMapped = (psDeviceNode->psDevConfig->ui32RegsSize / RGX_VIRTUALISATION_REG_SIZE_PER_OS);

	/* Nothing to do if the device does not support GPU_VIRTUALISATION */
	if (!PVRSRV_IS_FEATURE_SUPPORTED(psDeviceNode, GPU_VIRTUALISATION))
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "Testing per-os kick registers:"));

	ui32OsRegBanksMapped = MIN(ui32OsRegBanksMapped, GPUVIRT_VALIDATION_NUM_OS);

	if (ui32OsRegBanksMapped != RGXFW_MAX_NUM_OSIDS)
	{
		PVR_DPF((PVR_DBG_WARNING, "The register bank mapped into kernel VA does not cover all OS' registers:"));
		PVR_DPF((PVR_DBG_WARNING, "Maximum OS count = %d / Per-os register banks mapped = %d", RGXFW_MAX_NUM_OSIDS, ui32OsRegBanksMapped));
		PVR_DPF((PVR_DBG_WARNING, "Only first %d MTS registers will be tested", ui32OsRegBanksMapped));
	}

	ui32KickType = RGX_CR_MTS_SCHEDULE_DM_DM0 | RGX_CR_MTS_SCHEDULE_TASK_NON_COUNTED;

	for (ui32OSid = 0; ui32OSid < ui32OsRegBanksMapped; ui32OSid++)
	{
		/* set Test field */
		psFwSysInit->ui32OSKickTest = (ui32OSid << RGXFWIF_KICK_TEST_OSID_SHIFT) | RGXFWIF_KICK_TEST_ENABLED_BIT;

#if defined(PDUMP)
		DevmemPDumpLoadMem(psDevInfo->psRGXFWIfSysInitMemDesc,
						   offsetof(RGXFWIF_SYSINIT, ui32OSKickTest),
						   sizeof(psFwSysInit->ui32OSKickTest),
						   PDUMP_FLAGS_CONTINUOUS);
#endif

		/* Force a read-back to memory to avoid posted writes on certain buses */
		OSWriteMemoryBarrier(&psFwSysInit->ui32OSKickTest);
		RGXFwSharedMemCacheOpValue(psFwSysInit->ui32OSKickTest, FLUSH);

		/* kick register */
		ui32ScheduleRegister = RGX_CR_MTS_SCHEDULE + (ui32OSid * RGX_VIRTUALISATION_REG_SIZE_PER_OS);
		PVR_DPF((PVR_DBG_MESSAGE, "  Testing OS: %u, Kick Reg: %X",
				 ui32OSid,
				 ui32ScheduleRegister));
		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, ui32ScheduleRegister, ui32KickType);

#if defined(PDUMP)
		PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS, "VZ sideband test, kicking MTS register %u", ui32OSid);

		PDUMPREG32(psDeviceNode, RGX_PDUMPREG_NAME,
				ui32ScheduleRegister, ui32KickType, PDUMP_FLAGS_CONTINUOUS);

		DevmemPDumpDevmemPol32(psDevInfo->psRGXFWIfSysInitMemDesc,
							   offsetof(RGXFWIF_SYSINIT, ui32OSKickTest),
							   0,
							   0xFFFFFFFF,
							   PDUMP_POLL_OPERATOR_EQUAL,
							   PDUMP_FLAGS_CONTINUOUS);
#endif

#if !defined(NO_HARDWARE)
		OSMemoryBarrier((IMG_BYTE*) psDevInfo->pvRegsBaseKM + ui32ScheduleRegister);

		/* Wait test enable bit to be unset */
		if (PVRSRVPollForValueKM(psDeviceNode,
								 (volatile IMG_UINT32 __iomem *)&psFwSysInit->ui32OSKickTest,
								 0,
								 RGXFWIF_KICK_TEST_ENABLED_BIT,
								 POLL_FLAG_LOG_ERROR | POLL_FLAG_DEBUG_DUMP,
								 RGXFwSharedMemCacheOpExecPfn) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Testing OS %u kick register failed: firmware did not clear test location (contents: 0x%X)",
					 ui32OSid,
					 psFwSysInit->ui32OSKickTest));

			return PVRSRV_ERROR_TIMEOUT;
		}

		/* Check that the value is what we expect */
		if (psFwSysInit->ui32OSKickTest != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "Testing OS %u kick register failed: firmware wrote 0x%X to test location",
					 ui32OSid,
					 psFwSysInit->ui32OSKickTest));
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		PVR_DPF((PVR_DBG_MESSAGE, "    PASS"));
#endif
	}

	PVR_LOG(("MTS passed sideband tests"));
	return PVRSRV_OK;
}
#endif /* defined(SUPPORT_GPUVIRT_VALIDATION_MTS) */


/*
	RGXPostPowerState
*/
PVRSRV_ERROR RGXPostPowerState(PVRSRV_DEVICE_NODE		*psDeviceNode,
                               PVRSRV_DEV_POWER_STATE	eNewPowerState,
                               PVRSRV_DEV_POWER_STATE	eCurrentPowerState,
                               PVRSRV_POWER_FLAGS		ePwrFlags)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(ePwrFlags);

	if ((eNewPowerState == eCurrentPowerState) ||
	    (eCurrentPowerState != PVRSRV_DEV_POWER_STATE_OFF))
	{
		PDUMPCOMMENT(psDeviceNode,
		             "RGXPostPowerState: Current state: %d, New state: %d",
		             eCurrentPowerState, eNewPowerState);

		return PVRSRV_OK;
	}

	/* Update timer correlation related data */
	RGXTimeCorrBegin(psDeviceNode, RGXTIMECORR_EVENT_POWER);

	/* Update GPU state counters */
	_RGXUpdateGPUUtilStats(psDevInfo);

	eError = RGXDoStart(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXDoStart", fail);

	OSMemoryBarrier(NULL);


	/*
		* Check whether the FW has started by polling on bFirmwareStarted flag
		*/
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfSysInit->bFirmwareStarted,
	                           INVALIDATE);
	if (PVRSRVPollForValueKM(psDeviceNode,
	                         (IMG_UINT32 __iomem *)&psDevInfo->psRGXFWIfSysInit->bFirmwareStarted,
	                         IMG_TRUE,
	                         0xFFFFFFFF,
	                         POLL_FLAG_LOG_ERROR | POLL_FLAG_DEBUG_DUMP,
	                         RGXFwSharedMemCacheOpExecPfn) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "RGXPostPowerState: Polling for 'FW started' flag failed."));
		eError = PVRSRV_ERROR_TIMEOUT;

#if defined(TRACK_FW_BOOT)
		RGXCheckFWBootStage(psDevInfo);
#endif

		/*
		* When bFirmwareStarted fails some info may be gained by doing the following
		* debug dump but unfortunately it could be potentially dangerous if the reason
		* for not booting is the GPU power is not ON. However, if we have reached this
		* point the System Layer has returned without errors, we assume the GPU power
		* is indeed ON.
		*/
		RGXDumpRGXDebugSummary(NULL, NULL, psDeviceNode->pvDevice, IMG_TRUE);
		RGXDumpRGXRegisters(NULL, NULL, psDeviceNode->pvDevice);

		PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVPollForValueKM(bFirmwareStarted)", fail);
	}

#if defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS, "Wait for the Firmware to start.");
	eError = DevmemPDumpDevmemPol32(psDevInfo->psRGXFWIfSysInitMemDesc,
	                                offsetof(RGXFWIF_SYSINIT, bFirmwareStarted),
	                                IMG_TRUE,
	                                0xFFFFFFFFU,
	                                PDUMP_POLL_OPERATOR_EQUAL,
	                                PDUMP_FLAGS_CONTINUOUS);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "RGXPostPowerState: problem pdumping POL for psRGXFWIfSysInitMemDesc (%d)",
		         eError));
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemPDumpDevmemPol32", fail);
	}

#endif /* defined(PDUMP) */

#if defined(SUPPORT_GPUVIRT_VALIDATION_MTS)
	eError = RGXVirtualisationPowerupSidebandTest(psDeviceNode, psDevInfo->psRGXFWIfSysInit, psDevInfo);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXVirtualisationPowerupSidebandTest", fail);
#endif


#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfSysInit->ui32FirmwareStartedTimeStamp,
	                           INVALIDATE);
	PVRSRVSetFirmwareStartTime(psDeviceNode->psPowerDev,
	                           psDevInfo->psRGXFWIfSysInit->ui32FirmwareStartedTimeStamp);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfSysInit->ui32FirmwareStartedTimeStamp,
	                           FLUSH);
#endif

	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfSysInit->ui32MarkerVal,
	                           INVALIDATE);
	HTBSyncPartitionMarker(psDevInfo->psRGXFWIfSysInit->ui32MarkerVal);

#if defined(SUPPORT_LINUX_DVFS)
	eError = ResumeDVFS(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "ResumeDVFS", fail);
#endif

	PDUMPCOMMENT(psDeviceNode,
	             "RGXPostPowerState: Current state: %d, New state: %d",
	             eCurrentPowerState, eNewPowerState);

	return eError;

fail:
	psDevInfo->bRGXPowered = IMG_FALSE;

	return eError;
}

/*
	RGXPreClockSpeedChange
*/
PVRSRV_ERROR RGXPreClockSpeedChange(PVRSRV_DEVICE_NODE		*psDeviceNode,
                                    PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	const PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	__maybe_unused const RGX_DATA *psRGXData = (RGX_DATA*)psDeviceNode->psDevConfig->hDevData;
	const RGXFWIF_SYSDATA *psFwSysData = psDevInfo->psRGXFWIfFwSysData;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	PVR_DPF((PVR_DBG_MESSAGE, "RGXPreClockSpeedChange: RGX clock speed was %uHz",
			psRGXData->psRGXTimingInfo->ui32CoreClockSpeed));

	RGXFwSharedMemCacheOpValue(psFwSysData->ePowState,
	                           INVALIDATE);

	if ((eCurrentPowerState != PVRSRV_DEV_POWER_STATE_OFF) &&
	    (psFwSysData->ePowState != RGXFWIF_POW_OFF))
	{
		/* Update GPU frequency and timer correlation related data */
		RGXTimeCorrEnd(psDeviceNode, RGXTIMECORR_EVENT_DVFS);
	}

	return eError;
}

/*
	RGXPostClockSpeedChange
*/
PVRSRV_ERROR RGXPostClockSpeedChange(PVRSRV_DEVICE_NODE		*psDeviceNode,
                                     PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	const PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	const RGX_DATA *psRGXData = (RGX_DATA*)psDeviceNode->psDevConfig->hDevData;
	const RGXFWIF_SYSDATA *psFwSysData = psDevInfo->psRGXFWIfFwSysData;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32NewClockSpeed = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	/* Update runtime configuration with the new value */
	OSWriteDeviceMem32WithWMB(&psDevInfo->psRGXFWIfRuntimeCfg->ui32CoreClockSpeed,
	                          ui32NewClockSpeed);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->ui32CoreClockSpeed, FLUSH);

	RGXFwSharedMemCacheOpValue(psFwSysData->ePowState,
	                           INVALIDATE);
	if ((eCurrentPowerState != PVRSRV_DEV_POWER_STATE_OFF) &&
	    (psFwSysData->ePowState != RGXFWIF_POW_OFF))
	{
		RGXFWIF_KCCB_CMD sCOREClkSpeedChangeCmd;
		IMG_UINT32 ui32CmdKCCBSlot;

		RGXTimeCorrBegin(psDeviceNode, RGXTIMECORR_EVENT_DVFS);

		sCOREClkSpeedChangeCmd.eCmdType = RGXFWIF_KCCB_CMD_CORECLKSPEEDCHANGE;
		sCOREClkSpeedChangeCmd.uCmdData.sCoreClkSpeedChangeData.ui32NewClockSpeed = ui32NewClockSpeed;

		PDUMPCOMMENT(psDeviceNode, "Scheduling CORE clock speed change command");

		PDUMPPOWCMDSTART(psDeviceNode);
		eError = RGXSendCommandAndGetKCCBSlot(psDeviceNode->pvDevice,
		                                      &sCOREClkSpeedChangeCmd,
		                                      PDUMP_FLAGS_NONE,
		                                      &ui32CmdKCCBSlot);
		PDUMPPOWCMDEND(psDeviceNode);

		if (eError != PVRSRV_OK)
		{
			PDUMPCOMMENT(psDeviceNode, "Scheduling CORE clock speed change command failed");
			PVR_DPF((PVR_DBG_ERROR, "RGXPostClockSpeedChange: Scheduling KCCB command failed. Error:%u", eError));
			return eError;
		}

#if defined(PVRSRV_ANDROID_TRACE_GPU_FREQ)
		GpuTraceFrequency(psDeviceNode->sDevId.ui32InternalID,
				psRGXData->psRGXTimingInfo->ui32CoreClockSpeed);
#endif /* defined(PVRSRV_ANDROID_TRACE_GPU_FREQ) */

		PVR_DPF((PVR_DBG_MESSAGE, "RGXPostClockSpeedChange: RGX clock speed changed to %uHz",
				psRGXData->psRGXTimingInfo->ui32CoreClockSpeed));
	}

	return eError;
}

/*!
******************************************************************************

 @Function	RGXPowUnitsChange

 @Description Change power units state

 @Input	   psDeviceNode : RGX Device Node
 @Input	   ui32PowUnits : On Rogue: Number of DUSTs to make transition to.
                          On Volcanic: Mask containing power state of SPUs.
                          Each bit corresponds to an SPU and value must be non-zero.

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR RGXPowUnitsChange(PVRSRV_DEVICE_NODE *psDeviceNode,
                               IMG_UINT32 ui32PowUnits)

{
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR		eError;
	RGXFWIF_KCCB_CMD	sPowUnitsChange;
	IMG_UINT32			ui32AvailablePowUnits;
	IMG_UINT32			ui32CmdKCCBSlot;
	RGXFWIF_RUNTIME_CFG *psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

#if defined(PVR_ARCH_VOLCANIC)
	ui32AvailablePowUnits = psDevInfo->ui32AvailablePowUnitsMask;

	/**
	 * Validate the input. At-least one PU must be powered on and all requested
	 * PU's must be a subset of full PU mask.
	 */
	if ((ui32PowUnits == 0) || (ui32PowUnits & ~ui32AvailablePowUnits))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Invalid Power Units mask requested (0x%X). Value should be non-zero and sub-set of 0x%X mask",
				__func__,
				ui32PowUnits,
				ui32AvailablePowUnits));
		return PVRSRV_ERROR_INVALID_SPU_MASK;
	}
#else
	ui32AvailablePowUnits = psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount;

	if (ui32PowUnits > ui32AvailablePowUnits)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Invalid number of DUSTs (%u) while expecting value within <0,%u>",
				__func__,
				ui32PowUnits,
				ui32AvailablePowUnits));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
#endif

	psRuntimeCfg->ui32PowUnitsState = ui32PowUnits;
	OSWriteMemoryBarrier(&psRuntimeCfg->ui32PowUnitsState);
	RGXFwSharedMemCacheOpValue(psRuntimeCfg->ui32PowUnitsState, FLUSH);

#if !defined(NO_HARDWARE)
	{
		const RGXFWIF_SYSDATA *psFwSysData = psDevInfo->psRGXFWIfFwSysData;
		RGXFwSharedMemCacheOpValue(psFwSysData->ePowState,
		                           INVALIDATE);

		if (psFwSysData->ePowState == RGXFWIF_POW_OFF)
		{
			return PVRSRV_OK;
		}

		if (psFwSysData->ePowState != RGXFWIF_POW_FORCED_IDLE)
		{
			eError = PVRSRV_ERROR_DEVICE_POWER_CHANGE_DENIED;
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Powered units state can not be changed when not IDLE",
					 __func__));
			return eError;
		}
	}
#endif

	eError = SyncPrimSet(psDevInfo->psPowSyncPrim, 0);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set Power sync prim",
				__func__));
		return eError;
	}

	sPowUnitsChange.eCmdType = RGXFWIF_KCCB_CMD_POW;
	sPowUnitsChange.uCmdData.sPowData.ePowType = RGXFWIF_POW_NUM_UNITS_CHANGE;
	sPowUnitsChange.uCmdData.sPowData.uPowerReqData.ui32PowUnits = ui32PowUnits;
#if defined(RGX_FEATURE_POWER_ISLAND_VERSION_MAX_VALUE_IDX)
	sPowUnitsChange.uCmdData.sPowData.uPowerReqData.ui32RACUnits = 0;

	if (RGX_GET_FEATURE_VALUE(psDevInfo, POWER_ISLAND_VERSION) >= 3)
	{
		sPowUnitsChange.uCmdData.sPowData.uPowerReqData.ui32RACUnits =
			(1U << psDevInfo->sDevFeatureCfg.ui32MAXRACCount) - 1;
	}
#endif

	PDUMPCOMMENT(psDeviceNode,
	             "Scheduling command to change power units state to 0x%X",
	             ui32PowUnits);
	eError = RGXSendCommandAndGetKCCBSlot(psDeviceNode->pvDevice,
	                                      &sPowUnitsChange,
	                                      PDUMP_FLAGS_NONE,
	                                      &ui32CmdKCCBSlot);

	if (eError != PVRSRV_OK)
	{
		PDUMPCOMMENT(psDeviceNode,
		             "Scheduling command to change power units state. Error:%u",
		             eError);
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Scheduling KCCB to change power units state. Error:%u",
				 __func__, eError));
		return eError;
	}

	/* Wait for the firmware to answer. */
	eError = RGXPollForGPCommandCompletion(psDeviceNode,
	                              psDevInfo->psPowSyncPrim->pui32LinAddr,
								  0x1, 0xFFFFFFFF);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Timeout waiting for idle request", __func__));
		return eError;
	}

#if defined(PDUMP)
	PDUMPCOMMENT(psDeviceNode,
	             "%s: Poll for Kernel SyncPrim [0x%p] on DM %d",
	             __func__, psDevInfo->psPowSyncPrim->pui32LinAddr,
	             RGXFWIF_DM_GP);

	SyncPrimPDumpPol(psDevInfo->psPowSyncPrim,
	                 1,
	                 0xffffffff,
	                 PDUMP_POLL_OPERATOR_EQUAL,
	                 0);
#endif

	return PVRSRV_OK;
}

/*
 @Function	RGXAPMLatencyChange
*/
PVRSRV_ERROR RGXAPMLatencyChange(PVRSRV_DEVICE_NODE	*psDeviceNode,
                                 IMG_UINT32			ui32ActivePMLatencyms,
                                 IMG_BOOL			bActivePMLatencyPersistant)
{
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR		eError;
	RGXFWIF_RUNTIME_CFG	*psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	IMG_UINT32			ui32CmdKCCBSlot;
	PVRSRV_DEV_POWER_STATE	ePowerState;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	if (psRuntimeCfg == NULL)
	{
		return PVRSRV_ERROR_NOT_INITIALISED;
	}

	/* Update runtime configuration with the new values and ensure the
	 * new APM latency is written to memory before requesting the FW to
	 * read it
	 */
	psRuntimeCfg->ui32ActivePMLatencyms = ui32ActivePMLatencyms;
	psRuntimeCfg->bActivePMLatencyPersistant = bActivePMLatencyPersistant;
	OSWriteMemoryBarrier(&psRuntimeCfg->bActivePMLatencyPersistant);
	RGXFwSharedMemCacheOpValue(psRuntimeCfg->ui32ActivePMLatencyms, FLUSH);
	RGXFwSharedMemCacheOpValue(psRuntimeCfg->bActivePMLatencyPersistant, FLUSH);

	eError = PVRSRVPowerLock(psDeviceNode);
	if (eError == PVRSRV_ERROR_SYSTEM_STATE_POWERED_OFF)
	{
		/* Power is off, APM latency will be read on next firmware boot */
		return PVRSRV_OK;
	}
	else if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to acquire power lock (%u)",
		         __func__, eError));
		return eError;
	}

	eError = PVRSRVGetDevicePowerState(psDeviceNode, &ePowerState);

	if ((eError == PVRSRV_OK) && (ePowerState != PVRSRV_DEV_POWER_STATE_OFF))
	{
		RGXFWIF_KCCB_CMD	sActivePMLatencyChange;
		sActivePMLatencyChange.eCmdType = RGXFWIF_KCCB_CMD_POW;
		sActivePMLatencyChange.uCmdData.sPowData.ePowType = RGXFWIF_POW_APM_LATENCY_CHANGE;

		PDUMPCOMMENT(psDeviceNode,
		             "Scheduling command to change APM latency to %u",
		             ui32ActivePMLatencyms);
		eError = RGXSendCommandAndGetKCCBSlot(psDeviceNode->pvDevice,
		                                      &sActivePMLatencyChange,
		                                      PDUMP_FLAGS_NONE,
		                                      &ui32CmdKCCBSlot);

		if (eError != PVRSRV_OK)
		{
			PDUMPCOMMENT(psDeviceNode,
			             "Scheduling command to change APM latency failed. Error:%u",
			             eError);
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Scheduling KCCB to change APM latency failed. Error:%u",
			         __func__, eError));
			goto ErrorExit;
		}
	}

ErrorExit:
	PVRSRVPowerUnlock(psDeviceNode);

	return eError;
}

/*
	RGXActivePowerRequest
*/
PVRSRV_ERROR RGXActivePowerRequest(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	const RGXFWIF_SYSDATA *psFwSysData = psDevInfo->psRGXFWIfFwSysData;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);


	psDevInfo->ui32ActivePMReqTotal++;

	/* Powerlock to avoid further requests from racing with the FW hand-shake
	 * from now on (previous kicks to this point are detected by the FW)
	 * PVRSRVPowerLock is replaced with PVRSRVPowerTryLock to avoid
	 * potential dead lock between PDumpWriteLock and PowerLock
	 * during 'DriverLive + PDUMP=1 + EnableAPM=1'.
	 */
	eError = PVRSRVPowerTryLock(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		if (eError != PVRSRV_ERROR_RETRY)
		{
			PVR_LOG_ERROR(eError, "PVRSRVPowerTryLock");
		}
		else
		{
			psDevInfo->ui32ActivePMReqRetry++;
		}
		goto _RGXActivePowerRequest_PowerLock_failed;
	}

	RGXFwSharedMemCacheOpValue(psFwSysData->ePowState,
	                           INVALIDATE);

	/* Check again for IDLE once we have the power lock */
	if (psFwSysData->ePowState == RGXFWIF_POW_IDLE)
	{
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
		PVRSRVSetFirmwareHandshakeIdleTime(psDeviceNode->psPowerDev,
		                                   RGXReadHWTimerReg(psDevInfo)-psFwSysData->ui64StartIdleTime);
#endif

		PDUMPPOWCMDSTART(psDeviceNode);
		eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
		                                     PVRSRV_DEV_POWER_STATE_OFF,
		                                     PVRSRV_POWER_FLAGS_NONE);
		PDUMPPOWCMDEND(psDeviceNode);

		if (eError == PVRSRV_OK)
		{
			psDevInfo->ui32ActivePMReqOk++;
		}
		else if (eError == PVRSRV_ERROR_DEVICE_POWER_CHANGE_DENIED)
		{
			psDevInfo->ui32ActivePMReqDenied++;
		}
	}
	else
	{
		psDevInfo->ui32ActivePMReqNonIdle++;
	}

	PVRSRVPowerUnlock(psDeviceNode);

_RGXActivePowerRequest_PowerLock_failed:

	return eError;
}
/*
	RGXForcedIdleRequest
*/

#define RGX_FORCED_IDLE_RETRY_COUNT 10

PVRSRV_ERROR RGXForcedIdleRequest(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_BOOL bDeviceOffPermitted)
{
	PVRSRV_RGXDEV_INFO    *psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_KCCB_CMD      sPowCmd;
	PVRSRV_ERROR          eError;
	IMG_UINT32            ui32RetryCount = 0;
	IMG_UINT32            ui32CmdKCCBSlot;
#if !defined(NO_HARDWARE)
	const RGXFWIF_SYSDATA *psFwSysData;
#endif
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

#if !defined(NO_HARDWARE)
	psFwSysData = psDevInfo->psRGXFWIfFwSysData;
	RGXFwSharedMemCacheOpValue(psFwSysData->ePowState,
	                           INVALIDATE);

	/* Firmware already forced idle */
	if (psFwSysData->ePowState == RGXFWIF_POW_FORCED_IDLE)
	{
		return PVRSRV_OK;
	}

	/* Firmware is not powered. Sometimes this is permitted, for instance we were forcing idle to power down. */
	if (psFwSysData->ePowState == RGXFWIF_POW_OFF)
	{
		PVR_DPF((PVR_DBG_WARNING, "Firmware is powered OFF (bDeviceOffPermitted = %s)",
				 bDeviceOffPermitted ? "Yes" : "No"));
		return (bDeviceOffPermitted) ? PVRSRV_OK : PVRSRV_ERROR_DEVICE_IDLE_REQUEST_DENIED;
	}
#endif

	eError = SyncPrimSet(psDevInfo->psPowSyncPrim, 0);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set Power sync prim",
				__func__));
		return eError;
	}
	sPowCmd.eCmdType = RGXFWIF_KCCB_CMD_POW;
	sPowCmd.uCmdData.sPowData.ePowType = RGXFWIF_POW_FORCED_IDLE_REQ;
	sPowCmd.uCmdData.sPowData.uPowerReqData.ePowRequestType = RGXFWIF_POWER_FORCE_IDLE;

	PDUMPCOMMENT(psDeviceNode,
	             "RGXForcedIdleRequest: Sending forced idle command");

	/* Send one forced IDLE command to GP */
	eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
	                                      &sPowCmd,
	                                      PDUMP_FLAGS_NONE,
	                                      &ui32CmdKCCBSlot);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to send idle request", __func__));
		return eError;
	}

	/* Wait for GPU to finish current workload */
	do {
		eError = RGXPollForGPCommandCompletion(psDeviceNode,
		                              psDevInfo->psPowSyncPrim->pui32LinAddr,
									  0x1, 0xFFFFFFFF);
		if ((eError == PVRSRV_OK) || (ui32RetryCount == RGX_FORCED_IDLE_RETRY_COUNT))
		{
			break;
		}
		ui32RetryCount++;
		PVR_DPF((PVR_DBG_WARNING,
				"%s: Request timeout. Retry %d of %d",
				 __func__, ui32RetryCount, RGX_FORCED_IDLE_RETRY_COUNT));
	} while (IMG_TRUE);

	if (eError != PVRSRV_OK)
	{
		RGXFWNotifyHostTimeout(psDevInfo);
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Idle request failed. Firmware potentially left in forced idle state",
				 __func__));
		return eError;
	}

#if defined(PDUMP)
	PDUMPCOMMENT(psDeviceNode,
	             "RGXForcedIdleRequest: Poll for Kernel SyncPrim [0x%p] on DM %d",
	             psDevInfo->psPowSyncPrim->pui32LinAddr, RGXFWIF_DM_GP);

	SyncPrimPDumpPol(psDevInfo->psPowSyncPrim,
	                 1,
	                 0xffffffff,
	                 PDUMP_POLL_OPERATOR_EQUAL,
	                 0);
#endif

#if !defined(NO_HARDWARE)
	/* Check the firmware state for idleness */
	RGXFwSharedMemCacheOpValue(psFwSysData->ePowState,
	                           INVALIDATE);
	if (psFwSysData->ePowState != RGXFWIF_POW_FORCED_IDLE)
	{
		PVR_DPF((PVR_DBG_WARNING, "FW power state (%u) is not RGXFWIF_POW_FORCED_IDLE (%u)",
				 psFwSysData->ePowState, RGXFWIF_POW_FORCED_IDLE));
		return PVRSRV_ERROR_DEVICE_IDLE_REQUEST_DENIED;
	}
#endif

	return PVRSRV_OK;
}

/*
	RGXCancelForcedIdleRequest
*/
PVRSRV_ERROR RGXCancelForcedIdleRequest(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_KCCB_CMD	sPowCmd;
	PVRSRV_ERROR		eError = PVRSRV_OK;
	IMG_UINT32			ui32CmdKCCBSlot;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	eError = SyncPrimSet(psDevInfo->psPowSyncPrim, 0);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to set Power sync prim",
				__func__));
		goto ErrorExit;
	}

	/* Send the IDLE request to the FW */
	sPowCmd.eCmdType = RGXFWIF_KCCB_CMD_POW;
	sPowCmd.uCmdData.sPowData.ePowType = RGXFWIF_POW_FORCED_IDLE_REQ;
	sPowCmd.uCmdData.sPowData.uPowerReqData.ePowRequestType = RGXFWIF_POWER_CANCEL_FORCED_IDLE;

	PDUMPCOMMENT(psDeviceNode,
	             "RGXForcedIdleRequest: Sending cancel forced idle command");

	/* Send cancel forced IDLE command to GP */
	eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
	                                      &sPowCmd,
	                                      PDUMP_FLAGS_NONE,
	                                      &ui32CmdKCCBSlot);

	if (eError != PVRSRV_OK)
	{
		PDUMPCOMMENT(psDeviceNode,
		             "RGXCancelForcedIdleRequest: Failed to send cancel IDLE request for DM%d",
		             RGXFWIF_DM_GP);
		goto ErrorExit;
	}

	/* Wait for the firmware to answer. */
	eError = RGXPollForGPCommandCompletion(psDeviceNode,
	                              psDevInfo->psPowSyncPrim->pui32LinAddr,
								  1, 0xFFFFFFFF);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Timeout waiting for cancel idle request", __func__));
		goto ErrorExit;
	}

#if defined(PDUMP)
	PDUMPCOMMENT(psDeviceNode,
	             "RGXCancelForcedIdleRequest: Poll for Kernel SyncPrim [0x%p] on DM %d",
	             psDevInfo->psPowSyncPrim->pui32LinAddr, RGXFWIF_DM_GP);

	SyncPrimPDumpPol(psDevInfo->psPowSyncPrim,
	                 1,
	                 0xffffffff,
	                 PDUMP_POLL_OPERATOR_EQUAL,
	                 0);
#endif

	return eError;

ErrorExit:
	PVR_DPF((PVR_DBG_ERROR, "%s: Firmware potentially left in forced idle state", __func__));
	return eError;
}

#if defined(SUPPORT_FW_CORE_CLK_RATE_CHANGE_NOTIFY)
#if defined(SUPPORT_PDVFS) && (PDVFS_COM == PDVFS_COM_HOST)
/*************************************************************************/ /*!
@Function       PDVFSProcessCoreClkChangeRequest
@Description    Processes a core clock rate change request.
@Input          psDevInfo            A pointer to PVRSRV_RGXDEV_INFO.
@Input          ui32CoreClockRate    New core clock rate.
@Return         PVRSRV_ERROR.
*/ /**************************************************************************/
PVRSRV_ERROR RGXProcessCoreClkChangeRequest(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32CoreClockRate)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDevInfo->psDeviceNode->psDevConfig;
	IMG_DVFS_DEVICE_CFG *psDVFSDeviceCfg = &psDevConfig->sDVFS.sDVFSDeviceCfg;
	RGX_TIMING_INFORMATION *psRGXTimingInfo = ((RGX_DATA*)(psDevConfig->hDevData))->psRGXTimingInfo;
	IMG_UINT32 ui32CoreClockRateCurrent = psRGXTimingInfo->ui32CoreClockSpeed;
	const IMG_OPP *psOpp = NULL;
	IMG_UINT32 ui32Index;
	PVRSRV_ERROR eError;

	if (!_PDVFSEnabled())
	{
		/* No error message to avoid excessive messages */
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "Core clock rate = %u", ui32CoreClockRate));

	/* Find the matching OPP (Exact). */
	for (ui32Index = 0; ui32Index < psDVFSDeviceCfg->ui32OPPTableSize; ui32Index++)
	{
		if (ui32CoreClockRate == psDVFSDeviceCfg->pasOPPTable[ui32Index].ui32Freq)
		{
			psOpp = &psDVFSDeviceCfg->pasOPPTable[ui32Index];
			break;
		}
	}

	if (! psOpp)
	{
		PVR_DPF((PVR_DBG_ERROR, "Frequency not present in OPP table - %u", ui32CoreClockRate));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = PVRSRVDevicePreClockSpeedChange(psDevInfo->psDeviceNode, psDVFSDeviceCfg->bIdleReq, NULL);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDevicePreClockSpeedChange failed"));
		return eError;
	}

	psRGXTimingInfo->ui32CoreClockSpeed = ui32CoreClockRate;

	/* Increasing frequency, change voltage first */
	if (ui32CoreClockRate > ui32CoreClockRateCurrent)
	{
		psDVFSDeviceCfg->pfnSetVoltage(psDevConfig->hSysData, psOpp->ui32Volt);
	}

	psDVFSDeviceCfg->pfnSetFrequency(psDevConfig->hSysData, ui32CoreClockRate);

	/* Decreasing frequency, change frequency first */
	if (ui32CoreClockRate < ui32CoreClockRateCurrent)
	{
		psDVFSDeviceCfg->pfnSetVoltage(psDevConfig->hSysData, psOpp->ui32Volt);
	}

	PVRSRVDevicePostClockSpeedChange(psDevInfo->psDeviceNode, psDVFSDeviceCfg->bIdleReq, NULL);

	return PVRSRV_OK;
}
#else
/*************************************************************************/ /*!
@Function       PDVFSProcessCoreClkChangeNotification
@Description    Processes a core clock rate change notification.
@Input          psDevInfo            A pointer to PVRSRV_RGXDEV_INFO.
@Input          ui32CoreClockRate    New core clock rate.
@Return         PVRSRV_ERROR.
*/ /**************************************************************************/
PVRSRV_ERROR RGXProcessCoreClkChangeNotification(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32CoreClockRate)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDevInfo->psDeviceNode->psDevConfig;
	RGX_TIMING_INFORMATION *psRGXTimingInfo = ((RGX_DATA*)(psDevConfig->hDevData))->psRGXTimingInfo;
	PVRSRV_DEV_POWER_STATE ePowerState;
	PVRSRV_ERROR eError;

	eError = PVRSRVPowerLock(psDevInfo->psDeviceNode);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to acquire lock (%s)",
				 __func__, PVRSRVGetErrorString(eError)));
		return eError;
	}

	eError = PVRSRVGetDevicePowerState(psDevInfo->psDeviceNode, &ePowerState);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to acquire power state (%s)",
				 __func__, PVRSRVGetErrorString(eError)));
		PVRSRVPowerUnlock(psDevInfo->psDeviceNode);
		return eError;
	}

	/* Guest drivers do not initialize psRGXFWIfFwSysData */
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->ePowState,
	                           INVALIDATE);
	if ((ePowerState != PVRSRV_DEV_POWER_STATE_OFF)
	    && ((psDevInfo->psRGXFWIfFwSysData == NULL) || (psDevInfo->psRGXFWIfFwSysData->ePowState != RGXFWIF_POW_OFF)))
	{
		/* Update GPU frequency and timer correlation related data */
		RGXTimeCorrEnd(psDevInfo->psDeviceNode, RGXTIMECORR_EVENT_DVFS);
		psRGXTimingInfo->ui32CoreClockSpeed = ui32CoreClockRate;
		RGXTimeCorrBegin(psDevInfo->psDeviceNode, RGXTIMECORR_EVENT_DVFS);
	}
	else
	{
		psRGXTimingInfo->ui32CoreClockSpeed = ui32CoreClockRate;
	}

	PVRSRVPowerUnlock(psDevInfo->psDeviceNode);

	return PVRSRV_OK;
}
#endif
#endif /* SUPPORT_FW_CORE_CLK_RATE_CHANGE_NOTIFY */



/******************************************************************************
 End of file (rgxpower.c)
******************************************************************************/

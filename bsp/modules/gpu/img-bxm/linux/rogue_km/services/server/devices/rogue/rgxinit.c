/*************************************************************************/ /*!
@File
@Title          Device specific initialisation routines
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

#include "img_defs.h"
#include "pvr_notifier.h"
#include "pvrsrv.h"
#include "pvrsrv_bridge_init.h"
#include "rgx_bridge_init.h"
#include "syscommon.h"
#include "rgx_heaps_server.h"
#include "rgxheapconfig.h"
#include "rgxpower.h"
#include "tlstream.h"
#include "pvrsrv_tlstreams.h"
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
#include "pvr_ricommon.h"
#endif

#include "rgxinit.h"
#include "rgxbvnc.h"
#include "rgxmulticore.h"

#include "pdump_km.h"
#include "handle.h"
#include "allocmem.h"
#include "devicemem.h"
#include "devicemem_pdump.h"
#include "rgxmem.h"
#include "sync_internal.h"
#include "pvrsrv_apphint.h"
#include "os_apphint.h"
#include "rgxfwdbg.h"
#include "info_page.h"

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
#include "rgxfwimageutils.h"
#endif
#include "rgxutils.h"
#include "rgxfwutils.h"
#include "rgx_fwif_km.h"

#include "rgxmmuinit.h"
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
#include "rgxmipsmmuinit.h"
#include "physmem.h"
#endif
#include "devicemem_utils.h"
#include "devicemem_server.h"
#include "physmem_osmem.h"
#include "physmem_lma.h"

#include "rgxdebug_common.h"
#include "rgxhwperf.h"
#include "htbserver.h"

#include "rgx_options.h"
#include "pvrversion.h"

#include "rgx_compat_bvnc.h"

#include "rgxta3d.h"
#include "rgxtimecorr.h"
#include "rgxshader.h"

#if defined(PDUMP)
#include "rgxstartstop.h"
#endif

#include "rgx_fwif_alignchecks.h"
#include "vmm_pvz_client.h"

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
#include "rgxworkest.h"
#endif

#if defined(SUPPORT_PDVFS)
#include "rgxpdvfs.h"
#endif


#if defined(PDUMP) && defined(SUPPORT_SECURITY_VALIDATION)
#include "pdump_physmem.h"
#endif

static PVRSRV_ERROR RGXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);
static PVRSRV_ERROR RGXDevVersionString(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_CHAR **ppszVersionString);
static PVRSRV_ERROR RGXDevClockSpeed(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_PUINT32 pui32RGXClockSpeed);
static PVRSRV_ERROR RGXSoftReset(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_UINT64 ui64ResetValue1, IMG_UINT64 ui64ResetValue2);
static PVRSRV_ERROR RGXPhysMemDeviceHeapsInit(PVRSRV_DEVICE_NODE *psDeviceNode);
static void DevPart2DeInitRGX(PVRSRV_DEVICE_NODE *psDeviceNode);

#if defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1)
static PVRSRV_ERROR RGXInitFwRawHeap(PVRSRV_RGXDEV_INFO *psDevInfo, DEVMEM_HEAP_BLUEPRINT *psDevMemHeap, IMG_UINT32 ui32DriverID);
static void RGXDeInitFwRawHeap(DEVMEM_HEAP_BLUEPRINT *psDevMemHeap);
#endif /* defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1) */

/* Services internal heap identification used in this file only */
#define RGX_FIRMWARE_MAIN_HEAP_IDENT   "FwMain"   /*!< RGX Main Firmware Heap identifier */
#define RGX_FIRMWARE_CONFIG_HEAP_IDENT "FwConfig" /*!< RGX Config firmware Heap identifier */

#define RGX_MMU_PAGE_SIZE_4KB   (   4 * 1024)
#define RGX_MMU_PAGE_SIZE_16KB  (  16 * 1024)
#define RGX_MMU_PAGE_SIZE_64KB  (  64 * 1024)
#define RGX_MMU_PAGE_SIZE_256KB ( 256 * 1024)
#define RGX_MMU_PAGE_SIZE_1MB   (1024 * 1024)
#define RGX_MMU_PAGE_SIZE_2MB   (2048 * 1024)
#define RGX_MMU_PAGE_SIZE_MIN RGX_MMU_PAGE_SIZE_4KB
#define RGX_MMU_PAGE_SIZE_MAX RGX_MMU_PAGE_SIZE_2MB

#define VAR(x) #x

static void RGXDeInitHeaps(DEVICE_MEMORY_INFO *psDevMemoryInfo, PVRSRV_DEVICE_NODE *psDeviceNode);

#if !defined(NO_HARDWARE)
/*************************************************************************/ /*!
@Function       SampleIRQCount
@Description    Utility function taking snapshots of RGX FW interrupt count.
@Input          psDevInfo    Device Info structure

@Return         IMG_BOOL     Returns IMG_TRUE if RGX FW IRQ is not equal to
                             sampled RGX FW IRQ count for any RGX FW thread.
 */ /**************************************************************************/
static INLINE IMG_BOOL SampleIRQCount(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_BOOL bReturnVal = IMG_FALSE;
	volatile IMG_UINT32 *pui32SampleIrqCount = psDevInfo->aui32SampleIRQCount;
	IMG_UINT32 ui32IrqCnt;

#if defined(RGX_FW_IRQ_OS_COUNTERS)
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		bReturnVal = IMG_TRUE;
	}
	else
	{
		get_irq_cnt_val(ui32IrqCnt, RGXFW_HOST_DRIVER_ID, psDevInfo);

		if (ui32IrqCnt != pui32SampleIrqCount[RGXFW_THREAD_0])
		{
			pui32SampleIrqCount[RGXFW_THREAD_0] = ui32IrqCnt;
			bReturnVal = IMG_TRUE;
		}
	}
#else
	IMG_UINT32 ui32TID;

	for_each_irq_cnt(ui32TID)
	{
		get_irq_cnt_val(ui32IrqCnt, ui32TID, psDevInfo);

		/* treat unhandled interrupts here to align host count with fw count */
		if (pui32SampleIrqCount[ui32TID] != ui32IrqCnt)
		{
			pui32SampleIrqCount[ui32TID] = ui32IrqCnt;
			bReturnVal = IMG_TRUE;
		}
	}
#endif

	return bReturnVal;
}

/*************************************************************************/ /*!
@Function       RGXHostSafetyEvents
@Description    Returns the event status masked to keep only the safety
                events handled by the Host
@Input          psDevInfo    Device Info structure
@Return         IMG_UINT32   Status of Host-handled safety events
 */ /**************************************************************************/
static INLINE IMG_UINT32 RGXHostSafetyEvents(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo) || (psDevInfo->ui32HostSafetyEventMask == 0))
	{
		return 0;
	}
	else
	{
		IMG_UINT32 ui32SafetyEventStatus = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_SAFETY_EVENT_STATUS__ROGUEXE);
		return (ui32SafetyEventStatus & psDevInfo->ui32HostSafetyEventMask);
	}
}

/*************************************************************************/ /*!
@Function       RGXSafetyEventCheck
@Description    Clears the Event Status register and checks if any of the
                safety events need Host handling
@Input          psDevInfo    Device Info structure
@Return         IMG_BOOL     Are there any safety events for Host to handle ?
 */ /**************************************************************************/
static INLINE IMG_BOOL RGXSafetyEventCheck(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_BOOL bSafetyEvent = IMG_FALSE;

	if (psDevInfo->ui32HostSafetyEventMask != 0)
	{
		IMG_UINT32 ui32EventStatus = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_EVENT_STATUS);

		if (BIT_ISSET(ui32EventStatus, RGX_CR_EVENT_STATUS__ROGUEXE__SAFETY_SHIFT))
		{
			/* clear the safety event */
			OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_EVENT_CLEAR, RGX_CR_EVENT_CLEAR__ROGUEXE__SAFETY_EN);

			/* report if there is anything for the Host to handle */
			bSafetyEvent = (RGXHostSafetyEvents(psDevInfo) != 0);
		}
	}

	return bSafetyEvent;
}

/*************************************************************************/ /*!
@Function       RGXSafetyEventHandler
@Description    Handles the Safety Events that the Host is responsible for
@Input          psDevInfo    Device Info structure
 */ /**************************************************************************/
static void RGXSafetyEventHandler(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = psDevInfo->psDeviceNode;
	PVRSRV_ERROR eError;

	eError = PVRSRVPowerLock(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to acquire PowerLock (device: %p, error: %s)",
				 __func__, psDeviceNode, PVRSRVGetErrorString(eError)));
		return;
	}

	if (psDevInfo->bRGXPowered)
	{
		RGX_CONTEXT_RESET_REASON eResetReason = RGX_CONTEXT_RESET_REASON_NONE;
		IMG_UINT32 ui32HostSafetyStatus = RGXHostSafetyEvents(psDevInfo);

		if (ui32HostSafetyStatus != 0)
		{
			/* clear the safety bus events handled by the Host */
			OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_SAFETY_EVENT_CLEAR__ROGUEXE, ui32HostSafetyStatus);

			if (BIT_ISSET(ui32HostSafetyStatus, RGX_CR_SAFETY_EVENT_STATUS__ROGUEXE__FAULT_FW_SHIFT))
			{
				IMG_UINT32 ui32FaultFlag;
				IMG_UINT32 ui32FaultFW = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_FAULT_FW_STATUS);
				IMG_UINT32 ui32CorrectedBitOffset = RGX_CR_FAULT_FW_STATUS_CPU_CORRECT_SHIFT -
													RGX_CR_FAULT_FW_STATUS_CPU_DETECT_SHIFT;

				PVR_DPF((PVR_DBG_ERROR, "%s: Firmware safety fault status: 0x%X", __func__, ui32FaultFW));

				for (ui32FaultFlag = 0; ui32FaultFlag < ui32CorrectedBitOffset; ui32FaultFlag++)
				{
					if (BIT_ISSET(ui32FaultFW, ui32FaultFlag))
					{
						PVR_DPF((PVR_DBG_ERROR, "%s: Firmware safety hardware fault detected (0x%lX).",
							 __func__, BIT(ui32FaultFlag)));
						eResetReason = RGX_CONTEXT_RESET_REASON_FW_ECC_ERR;
					}
					else if BIT_ISSET(ui32FaultFW, ui32FaultFlag + ui32CorrectedBitOffset)
					{
						PVR_DPF((PVR_DBG_WARNING, "%s: Firmware safety hardware fault corrected.(0x%lX).",
							 __func__, BIT(ui32FaultFlag)));

						/* Only report this if we haven't detected a more serious error */
						if (eResetReason != RGX_CONTEXT_RESET_REASON_FW_ECC_ERR)
						{
							eResetReason = RGX_CONTEXT_RESET_REASON_FW_ECC_OK;
						}
					}
				}

				OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_FAULT_FW_CLEAR, ui32FaultFW);
			}

			if (BIT_ISSET(ui32HostSafetyStatus, RGX_CR_SAFETY_EVENT_STATUS__ROGUEXE__WATCHDOG_TIMEOUT_SHIFT))
			{
				volatile RGXFWIF_POW_STATE ePowState;

				RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->ePowState,
										   INVALIDATE);
				ePowState = psDevInfo->psRGXFWIfFwSysData->ePowState;

				if (ePowState != RGXFWIF_POW_OFF)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Safety Watchdog Trigger !", __func__));

					/* Only report this if we haven't detected a more serious error */
					if (eResetReason != RGX_CONTEXT_RESET_REASON_FW_ECC_ERR)
					{
						eResetReason = RGX_CONTEXT_RESET_REASON_FW_WATCHDOG;
					}
				}
			}

			/* Notify client and system layer of any error */
			if (eResetReason != RGX_CONTEXT_RESET_REASON_NONE)
			{
				PVRSRV_DEVICE_NODE *psDevNode = psDevInfo->psDeviceNode;
				PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;

				/* Client notification of device error will be achieved by
				 * clients calling UM function RGXGetLastDeviceError() */
				psDevInfo->eLastDeviceError = eResetReason;

				/* Notify system layer of any error */
				if (psDevConfig->pfnSysDevErrorNotify)
				{
					PVRSRV_ROBUSTNESS_NOTIFY_DATA sErrorData = {0};

					sErrorData.eResetReason = eResetReason;

					psDevConfig->pfnSysDevErrorNotify(psDevConfig,
													  &sErrorData);
				}
			}
		}
	}

	PVRSRVPowerUnlock(psDeviceNode);
}

static IMG_BOOL _WaitForInterruptsTimeoutCheck(PVRSRV_RGXDEV_INFO *psDevInfo)
{
#if defined(PVRSRV_DEBUG_LISR_EXECUTION)
	PVRSRV_DEVICE_NODE *psDeviceNode = psDevInfo->psDeviceNode;
	IMG_UINT32 ui32idx;
#endif

	RGXDEBUG_PRINT_IRQ_COUNT(psDevInfo);

#if defined(PVRSRV_DEBUG_LISR_EXECUTION)
	PVR_DPF((PVR_DBG_ERROR,
	        "Last RGX_LISRHandler State (DevID %u): 0x%08X Clock: %" IMG_UINT64_FMTSPEC,
			 psDeviceNode->sDevId.ui32InternalID,
			 psDeviceNode->sLISRExecutionInfo.ui32Status,
			 psDeviceNode->sLISRExecutionInfo.ui64Clockns));

	for_each_irq_cnt(ui32idx)
	{
		PVR_DPF((PVR_DBG_ERROR,
				MSG_IRQ_CNT_TYPE " %u: InterruptCountSnapshot: 0x%X",
				ui32idx, psDeviceNode->sLISRExecutionInfo.aui32InterruptCountSnapshot[ui32idx]));
	}
#else
	PVR_DPF((PVR_DBG_ERROR, "No further information available. Please enable PVRSRV_DEBUG_LISR_EXECUTION"));
#endif

	return SampleIRQCount(psDevInfo);
}

void RGX_WaitForInterruptsTimeout(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_BOOL bScheduleMISR;

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		bScheduleMISR = IMG_TRUE;
	}
	else
	{
		bScheduleMISR = _WaitForInterruptsTimeoutCheck(psDevInfo);
	}

	if (bScheduleMISR)
	{
		OSScheduleMISR(psDevInfo->pvMISRData);

		if (psDevInfo->pvAPMISRData != NULL)
		{
			OSScheduleMISR(psDevInfo->pvAPMISRData);
		}
	}
}

static inline IMG_BOOL RGXAckHwIrq(PVRSRV_RGXDEV_INFO *psDevInfo,
								   IMG_UINT32 ui32IRQStatusReg,
								   IMG_UINT32 ui32IRQStatusEventMsk,
								   IMG_UINT32 ui32IRQClearReg,
								   IMG_UINT32 ui32IRQClearMask)
{
	IMG_UINT32 ui32IRQStatus = OSReadHWReg32(psDevInfo->pvRegsBaseKM, ui32IRQStatusReg);

	/* clear only the pending bit of the thread that triggered this interrupt */
	ui32IRQClearMask &= ui32IRQStatus;

	if (ui32IRQStatus & ui32IRQStatusEventMsk)
	{
		/* acknowledge and clear the interrupt */
		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, ui32IRQClearReg, ui32IRQClearMask);
		return IMG_TRUE;
	}
	else
	{
		/* spurious interrupt */
		return IMG_FALSE;
	}
}

static __maybe_unused IMG_BOOL RGXAckIrqMETA(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	return RGXAckHwIrq(psDevInfo,
					   RGX_CR_META_SP_MSLVIRQSTATUS,
					   RGX_CR_META_SP_MSLVIRQSTATUS_TRIGVECT2_EN,
					   RGX_CR_META_SP_MSLVIRQSTATUS,
					   RGX_CR_META_SP_MSLVIRQSTATUS_TRIGVECT2_CLRMSK);
}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
static IMG_BOOL RGXAckIrqMIPS(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	return RGXAckHwIrq(psDevInfo,
					   RGX_CR_MIPS_WRAPPER_IRQ_STATUS,
					   RGX_CR_MIPS_WRAPPER_IRQ_STATUS_EVENT_EN,
					   RGX_CR_MIPS_WRAPPER_IRQ_CLEAR,
					   RGX_CR_MIPS_WRAPPER_IRQ_CLEAR_EVENT_EN);
}
#endif

static IMG_BOOL RGXAckIrqDedicated(PVRSRV_RGXDEV_INFO *psDevInfo)
{
		/* status & clearing registers are available on both Host and Guests
		 * and are agnostic of the Fw CPU type. Due to the remappings done by
		 * the 2nd stage device MMU, all drivers assume they are accessing
		 * register bank 0 */
	return RGXAckHwIrq(psDevInfo,
					   RGX_CR_IRQ_OS0_EVENT_STATUS,
					   ~RGX_CR_IRQ_OS0_EVENT_STATUS_SOURCE_CLRMSK,
					   RGX_CR_IRQ_OS0_EVENT_CLEAR,
					   ~RGX_CR_IRQ_OS0_EVENT_CLEAR_SOURCE_CLRMSK);
}

static PVRSRV_ERROR RGXSetAckIrq(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	if ((RGX_IS_FEATURE_SUPPORTED(psDevInfo, IRQ_PER_OS)) && (!PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo)))
	{
		psDevInfo->pfnRGXAckIrq = RGXAckIrqDedicated;
	}
	else if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		psDevInfo->pfnRGXAckIrq = NULL;
	}
	else
	{
		/* native and host drivers must clear the unique GPU physical interrupt */
		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
		{
			psDevInfo->pfnRGXAckIrq = RGXAckIrqMIPS;
		}
		else if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
		{
			psDevInfo->pfnRGXAckIrq = RGXAckIrqMETA;
		}
		else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
		{
			psDevInfo->pfnRGXAckIrq = RGXAckIrqDedicated;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: GPU IRQ clearing mechanism not implemented "
									"for this architecture.", __func__));
			return PVRSRV_ERROR_NOT_IMPLEMENTED;
		}
	}

#if defined(RGX_IRQ_HYPERV_HANDLER)
	/* The hypervisor receives and acknowledges the GPU irq, then it injects an
	 * irq only in the recipient OS. The KM driver doesn't handle the GPU irq line */
	psDevInfo->pfnRGXAckIrq = NULL;
#endif

	return PVRSRV_OK;
}

static IMG_BOOL RGX_LISRHandler(void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = pvData;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	IMG_BOOL bIrqAcknowledged = IMG_FALSE;

#if defined(PVRSRV_DEBUG_LISR_EXECUTION)
	IMG_UINT32 ui32idx, ui32IrqCnt;

	for_each_irq_cnt(ui32idx)
	{
		get_irq_cnt_val(ui32IrqCnt, ui32idx, psDevInfo);
		UPDATE_LISR_DBG_SNAPSHOT(ui32idx, ui32IrqCnt);
	}

	UPDATE_LISR_DBG_STATUS(RGX_LISR_INIT);
	UPDATE_LISR_DBG_TIMESTAMP();
#endif

	UPDATE_LISR_DBG_COUNTER();

	if (psDevInfo->bRGXPowered)
	{
		IMG_BOOL bSafetyEvent = RGXSafetyEventCheck(psDevInfo);

		if ((psDevInfo->pfnRGXAckIrq == NULL) || psDevInfo->pfnRGXAckIrq(psDevInfo) || bSafetyEvent)
		{
			bIrqAcknowledged = IMG_TRUE;

			if (bSafetyEvent || SampleIRQCount(psDevInfo))
			{
				UPDATE_LISR_DBG_STATUS(RGX_LISR_PROCESSED);
				UPDATE_MISR_DBG_COUNTER();

				OSScheduleMISR(psDevInfo->pvMISRData);

#if defined(SUPPORT_AUTOVZ)
				RGXUpdateAutoVzWdgToken(psDevInfo);
#endif
				if (psDevInfo->pvAPMISRData != NULL)
				{
					OSScheduleMISR(psDevInfo->pvAPMISRData);
				}
			}
			else
			{
				UPDATE_LISR_DBG_STATUS(RGX_LISR_FW_IRQ_COUNTER_NOT_UPDATED);
			}
		}
		else
		{
			UPDATE_LISR_DBG_STATUS(RGX_LISR_NOT_TRIGGERED_BY_HW);
		}
	}
	else
	{
#if defined(SUPPORT_AUTOVZ)
		/* AutoVz drivers rebooting while the firmware is active must acknowledge
		 * and clear the hw IRQ line before the RGXInit() has finished. */
		if ((psDevInfo->pfnRGXAckIrq != NULL) &&
			 psDevInfo->pfnRGXAckIrq(psDevInfo))
		{
			bIrqAcknowledged = IMG_TRUE;
		}
		else
#endif
		{
			UPDATE_LISR_DBG_STATUS(RGX_LISR_DEVICE_NOT_POWERED);
		}
	}

	return bIrqAcknowledged;
}

static void RGX_MISR_ProcessKCCBDeferredList(PVRSRV_DEVICE_NODE	*psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	OS_SPINLOCK_FLAGS uiFlags;

	/* First check whether there are pending commands in Deferred KCCB List */
	OSSpinLockAcquire(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);
	if (dllist_is_empty(&psDevInfo->sKCCBDeferredCommandsListHead))
	{
		OSSpinLockRelease(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);
		return;
	}
	OSSpinLockRelease(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);

	/* Powerlock to avoid further Power transition requests
	   while KCCB deferred list is being processed */
	eError = PVRSRVPowerLock(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to acquire PowerLock (device: %p, error: %s)",
				 __func__, psDeviceNode, PVRSRVGetErrorString(eError)));
		return;
	}

	/* Try to send deferred KCCB commands Do not Poll from here*/
	eError = RGXSendCommandsFromDeferredList(psDevInfo, IMG_FALSE);

	PVRSRVPowerUnlock(psDeviceNode);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_MESSAGE,
				 "%s could not flush Deferred KCCB list, KCCB is full.",
				 __func__));
	}
}

static void RGX_MISRHandler_CheckFWActivePowerState(void *psDevice)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = psDevice;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	const RGXFWIF_SYSDATA *psFwSysData;
	PVRSRV_ERROR eError = PVRSRV_OK;

	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->ePowState,
	                           INVALIDATE);
	psFwSysData = psDevInfo->psRGXFWIfFwSysData;

	if (psFwSysData->ePowState == RGXFWIF_POW_ON || psFwSysData->ePowState == RGXFWIF_POW_IDLE)
	{
		RGX_MISR_ProcessKCCBDeferredList(psDeviceNode);
	}

	if (psFwSysData->ePowState == RGXFWIF_POW_IDLE)
	{
		/* The FW is IDLE and therefore could be shut down */
		eError = RGXActivePowerRequest(psDeviceNode);

		if ((eError != PVRSRV_OK) && (eError != PVRSRV_ERROR_DEVICE_POWER_CHANGE_DENIED))
		{
			if (eError != PVRSRV_ERROR_RETRY)
			{
				PVR_DPF((PVR_DBG_WARNING,
					"%s: Failed RGXActivePowerRequest call (device: %p) with %s",
					__func__, psDeviceNode, PVRSRVGetErrorString(eError)));
				PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
			}
			else
			{
				/* Re-schedule the power down request as it was deferred. */
				OSScheduleMISR(psDevInfo->pvAPMISRData);
			}
		}
	}

}

/* Shorter defines to keep the code a bit shorter */
#define GPU_IDLE       RGXFWIF_GPU_UTIL_STATE_IDLE
#define GPU_ACTIVE     RGXFWIF_GPU_UTIL_STATE_ACTIVE
#define GPU_BLOCKED    RGXFWIF_GPU_UTIL_STATE_BLOCKED
#define GPU_INACTIVE   RGXFWIF_GPU_UTIL_STATE_INACTIVE
#define MAX_ITERATIONS 64
#define MAX_DIFF_TIME_NS (300000ULL)
#define MAX_DIFF_DM_TIME_NS (MAX_DIFF_TIME_NS >> RGXFWIF_DM_OS_TIMESTAMP_SHIFT)

static PVRSRV_ERROR RGXGetGpuUtilStats(PVRSRV_DEVICE_NODE *psDeviceNode,
                                       IMG_HANDLE hGpuUtilUser,
                                       RGXFWIF_GPU_UTIL_STATS *psReturnStats)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	RGXFWIF_GPU_STATS sStats;
	RGXFWIF_GPU_UTIL_STATS *psAggregateStats;
	IMG_UINT64 (*paaui64DMOSTmpCounters)[RGX_NUM_DRIVERS_SUPPORTED][RGXFWIF_GPU_UTIL_REDUCED_STATES_NUM];
	IMG_UINT64 (*paui64DMOSTmpLastWord)[RGX_NUM_DRIVERS_SUPPORTED];
	IMG_UINT64 (*paui64DMOSTmpLastState)[RGX_NUM_DRIVERS_SUPPORTED];
	IMG_UINT64 (*paui64DMOSTmpLastPeriod)[RGX_NUM_DRIVERS_SUPPORTED];
	IMG_UINT64 (*paui64DMOSTmpLastTime)[RGX_NUM_DRIVERS_SUPPORTED];
	IMG_UINT64 ui64TimeNow;
	IMG_UINT64 ui64TimeNowShifted;
	IMG_UINT32 ui32Attempts;
	IMG_UINT32 ui32Remainder;
	IMG_UINT32 ui32DriverID;
	IMG_UINT32 ui32MaxDMCount;
	RGXFWIF_DM eDM;

	/***** (1) Initialise return stats *****/

	psReturnStats->bValid = IMG_FALSE;

	if (hGpuUtilUser == NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	psAggregateStats = hGpuUtilUser;

	/* decrease by 1 to account for excluding GP DM from the statics */;
	ui32MaxDMCount =  psDevInfo->sDevFeatureCfg.ui32MAXDMCount-1;

	/* Reset temporary counters used in the attempts loop */
	paaui64DMOSTmpCounters  = &psAggregateStats->sTempGpuStats.aaaui64DMOSTmpCounters[0];
	paui64DMOSTmpLastWord   = &psAggregateStats->sTempGpuStats.aaui64DMOSTmpLastWord[0];
	paui64DMOSTmpLastState  = &psAggregateStats->sTempGpuStats.aaui64DMOSTmpLastState[0];
	paui64DMOSTmpLastPeriod = &psAggregateStats->sTempGpuStats.aaui64DMOSTmpLastPeriod[0];
	paui64DMOSTmpLastTime   = &psAggregateStats->sTempGpuStats.aaui64DMOSTmpLastTime[0];

	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfGpuUtilFW, INVALIDATE);

	/* Try to acquire GPU utilisation counters and repeat if the FW is in the middle of an update */
	for (ui32Attempts = 0; ui32Attempts < 4; ui32Attempts++)
	{
		IMG_UINT64 aui64GpuTmpCounters[RGXFWIF_GPU_UTIL_STATE_NUM] = {0};
		IMG_UINT64 ui64GpuLastPeriod = 0, ui64GpuLastWord = 0, ui64GpuLastState = 0, ui64GpuLastTime = 0;

		/***** (2) Get latest data from shared area *****/

		FOREACH_SUPPORTED_DRIVER(ui32DriverID)
		{
			IMG_UINT64 aui64StatsCountersNew[RGXFWIF_GPU_UTIL_STATE_NUM];
			IMG_UINT64 ui64GpuLastWordNew;
			RGXFWIF_GPU_STATS sStatsNew;
			IMG_UINT32 i = 0;

			ui64GpuLastWord = 0;
			ui64GpuLastState = 0;

			OSLockAcquire(psDevInfo->hGPUUtilLock);

			/* Copy data from device memory */
			memcpy(&sStatsNew, &psDevInfo->psRGXFWIfGpuUtilFW->sStats[ui32DriverID], sizeof(sStats));
			memcpy(&ui64GpuLastWordNew, &psDevInfo->psRGXFWIfGpuUtilFW->ui64GpuLastWord, sizeof(ui64GpuLastWord));
			memcpy(aui64StatsCountersNew, psDevInfo->psRGXFWIfGpuUtilFW->aui64GpuStatsCounters, sizeof(aui64StatsCountersNew));

			/*
			 * First attempt at detecting if the FW is in the middle of an update.
			 * This should also help if the FW is in the middle of a 64 bit variable update.
			 * This loop must be fast. Faster than FW updates the stats.
			 */
			for (i = 0; i < MAX_ITERATIONS; i++)
			{
				IMG_UINT32 j,k;
				IMG_BOOL bRetry = IMG_FALSE;

				if (i > 0)
				{
					/* On retry keep previous data */
					ui64GpuLastWordNew = ui64GpuLastWord;
					memcpy(aui64StatsCountersNew, aui64GpuTmpCounters, sizeof(aui64StatsCountersNew));
					memcpy(&sStatsNew, &sStats, sizeof(sStatsNew));
				}

				/* Copy data from device memory */
				memcpy(&sStats, &psDevInfo->psRGXFWIfGpuUtilFW->sStats[ui32DriverID], sizeof(sStats));
				memcpy(&ui64GpuLastWord, &psDevInfo->psRGXFWIfGpuUtilFW->ui64GpuLastWord, sizeof(ui64GpuLastWord));
				memcpy(aui64GpuTmpCounters, psDevInfo->psRGXFWIfGpuUtilFW->aui64GpuStatsCounters, sizeof(aui64GpuTmpCounters));

				/* Check for abnormal time difference between reads */
				if (RGXFWIF_GPU_UTIL_GET_TIME(ui64GpuLastWord) - RGXFWIF_GPU_UTIL_GET_TIME(ui64GpuLastWordNew) > MAX_DIFF_TIME_NS)
				{
					bRetry = IMG_TRUE;
					continue;
				}

				for (j = 0; j < RGXFWIF_GPU_UTIL_STATE_NUM; j++)
				{
					/* Check for abnormal time difference between reads */
					if (aui64GpuTmpCounters[j] - aui64StatsCountersNew[j] > MAX_DIFF_TIME_NS)
					{
						bRetry = IMG_TRUE;
						break;
					}
				}

				if (bRetry)
				{
					continue;
				}

				/* Check for DM counters wrapped or
				   abnormal time difference between reads.
				   The DM time is shifted by RGXFWIF_DM_OS_TIMESTAMP_SHIFT */
				for (j = 0; j < RGXFWIF_GPU_UTIL_DM_MAX; j++)
				{
					if (sStats.aui32DMOSLastWordWrap[j] != sStatsNew.aui32DMOSLastWordWrap[j] ||
						RGXFWIF_GPU_UTIL_GET_TIME32(sStats.aui32DMOSLastWord[j]) - RGXFWIF_GPU_UTIL_GET_TIME32(sStatsNew.aui32DMOSLastWord[j]) > MAX_DIFF_DM_TIME_NS)
					{
						bRetry = IMG_TRUE;
						break;
					}

					for (k = 0; k < RGXFWIF_GPU_UTIL_REDUCED_STATES_NUM; k++)
					{
						if (sStats.aaui32DMOSCountersWrap[j][k] != sStatsNew.aaui32DMOSCountersWrap[j][k] ||
							sStats.aaui32DMOSStatsCounters[j][k] - sStatsNew.aaui32DMOSStatsCounters[j][k] > MAX_DIFF_DM_TIME_NS)
						{
							bRetry = IMG_TRUE;
							break;
						}

					}

					if (bRetry)
					{
						break;
					}
				}

				if (!bRetry)
				{
					/* Stats are good*/
					break;
				}
			}

			OSLockRelease(psDevInfo->hGPUUtilLock);

			ui64GpuLastState = RGXFWIF_GPU_UTIL_GET_STATE(ui64GpuLastWord);

			if (i == MAX_ITERATIONS)
			{
				PVR_DPF((PVR_DBG_WARNING,
						 "RGXGetGpuUtilStats could not get reliable data after trying %u times", i));

				return PVRSRV_ERROR_TIMEOUT;
			}

			for (eDM = 0; eDM < ui32MaxDMCount; eDM++)
			{
				paui64DMOSTmpLastWord[eDM][ui32DriverID]  =
					((IMG_UINT64)sStats.aui32DMOSLastWordWrap[eDM] << 32) + sStats.aui32DMOSLastWord[eDM];
				paui64DMOSTmpLastState[eDM][ui32DriverID] = RGXFWIF_GPU_UTIL_GET_STATE(paui64DMOSTmpLastWord[eDM][ui32DriverID]);
				if (paui64DMOSTmpLastState[eDM][ui32DriverID] != GPU_ACTIVE)
				{
					paui64DMOSTmpLastState[eDM][ui32DriverID] = GPU_INACTIVE;
				}
				paaui64DMOSTmpCounters[eDM][ui32DriverID][GPU_INACTIVE] = (IMG_UINT64)sStats.aaui32DMOSStatsCounters[eDM][GPU_INACTIVE] +
					((IMG_UINT64)sStats.aaui32DMOSCountersWrap[eDM][GPU_INACTIVE] << 32);
				paaui64DMOSTmpCounters[eDM][ui32DriverID][GPU_ACTIVE]  = (IMG_UINT64)sStats.aaui32DMOSStatsCounters[eDM][GPU_ACTIVE] +
					((IMG_UINT64)sStats.aaui32DMOSCountersWrap[eDM][GPU_ACTIVE] << 32);
			}

		} /* FOREACH_SUPPORTED_DRIVER(ui32DriverID) */


		/***** (3) Compute return stats *****/

		/* Update temp counters to account for the time since the last update to the shared ones */
		OSMemoryBarrier(NULL); /* Ensure the current time is read after the loop above */
		ui64TimeNow    = RGXFWIF_GPU_UTIL_GET_TIME(RGXTimeCorrGetClockns64(psDeviceNode));

		ui64GpuLastTime   = RGXFWIF_GPU_UTIL_GET_TIME(ui64GpuLastWord);
		ui64GpuLastPeriod = RGXFWIF_GPU_UTIL_GET_PERIOD(ui64TimeNow, ui64GpuLastTime);
		aui64GpuTmpCounters[ui64GpuLastState] += ui64GpuLastPeriod;

		/* Get statistics for a user since its last request */
		psReturnStats->ui64GpuStatIdle = RGXFWIF_GPU_UTIL_GET_PERIOD(aui64GpuTmpCounters[GPU_IDLE],
		                                                             psAggregateStats->ui64GpuStatIdle);
		psReturnStats->ui64GpuStatActive = RGXFWIF_GPU_UTIL_GET_PERIOD(aui64GpuTmpCounters[GPU_ACTIVE],
		                                                               psAggregateStats->ui64GpuStatActive);
		psReturnStats->ui64GpuStatBlocked = RGXFWIF_GPU_UTIL_GET_PERIOD(aui64GpuTmpCounters[GPU_BLOCKED],
		                                                                psAggregateStats->ui64GpuStatBlocked);
		psReturnStats->ui64GpuStatCumulative = psReturnStats->ui64GpuStatIdle +
		                                       psReturnStats->ui64GpuStatActive +
		                                       psReturnStats->ui64GpuStatBlocked;

		/* convert time into the same units as used by fw */
		ui64TimeNowShifted  = ui64TimeNow >> RGXFWIF_DM_OS_TIMESTAMP_SHIFT;
		for (eDM = 0; eDM < ui32MaxDMCount; eDM++)
		{
			FOREACH_SUPPORTED_DRIVER(ui32DriverID)
			{
				paui64DMOSTmpLastTime[eDM][ui32DriverID]   = RGXFWIF_GPU_UTIL_GET_TIME(paui64DMOSTmpLastWord[eDM][ui32DriverID]);
				paui64DMOSTmpLastPeriod[eDM][ui32DriverID] = RGXFWIF_GPU_UTIL_GET_PERIOD(ui64TimeNowShifted , paui64DMOSTmpLastTime[eDM][ui32DriverID]);
				paaui64DMOSTmpCounters[eDM][ui32DriverID][paui64DMOSTmpLastState[eDM][ui32DriverID]] += paui64DMOSTmpLastPeriod[eDM][ui32DriverID];
				/* Get statistics for a user since its last request */
				psReturnStats->aaui64DMOSStatInactive[eDM][ui32DriverID] = RGXFWIF_GPU_UTIL_GET_PERIOD(paaui64DMOSTmpCounters[eDM][ui32DriverID][GPU_INACTIVE],
				                                                             psAggregateStats->aaui64DMOSStatInactive[eDM][ui32DriverID]);
				psReturnStats->aaui64DMOSStatActive[eDM][ui32DriverID] = RGXFWIF_GPU_UTIL_GET_PERIOD(paaui64DMOSTmpCounters[eDM][ui32DriverID][GPU_ACTIVE],
				                                                               psAggregateStats->aaui64DMOSStatActive[eDM][ui32DriverID]);
				psReturnStats->aaui64DMOSStatCumulative[eDM][ui32DriverID] = psReturnStats->aaui64DMOSStatInactive[eDM][ui32DriverID] +
				                                       psReturnStats->aaui64DMOSStatActive[eDM][ui32DriverID];
			}
		}

		if (psAggregateStats->ui64TimeStamp != 0)
		{
			IMG_UINT64 ui64TimeSinceLastCall = ui64TimeNow - psAggregateStats->ui64TimeStamp;
			/* We expect to return at least 75% of the time since the last call in GPU stats */
			IMG_UINT64 ui64MinReturnedStats = ui64TimeSinceLastCall - (ui64TimeSinceLastCall / 4);

			/*
			 * If the returned stats are substantially lower than the time since
			 * the last call, then the Host might have read a partial update from the FW.
			 * If this happens, try sampling the shared counters again.
			 */
			if (psReturnStats->ui64GpuStatCumulative < ui64MinReturnedStats)
			{
				PVR_DPF((PVR_DBG_MESSAGE,
				         "%s: Return stats (%" IMG_UINT64_FMTSPEC ") too low "
				         "(call period %" IMG_UINT64_FMTSPEC ")",
				         __func__, psReturnStats->ui64GpuStatCumulative, ui64TimeSinceLastCall));
				PVR_DPF((PVR_DBG_MESSAGE, "%s: Attempt #%u has failed, trying again",
				         __func__, ui32Attempts));
				continue;
			}
		}

		break;
	}

	/***** (4) Update aggregate stats for the current user *****/

	psAggregateStats->ui64GpuStatIdle    += psReturnStats->ui64GpuStatIdle;
	psAggregateStats->ui64GpuStatActive  += psReturnStats->ui64GpuStatActive;
	psAggregateStats->ui64GpuStatBlocked += psReturnStats->ui64GpuStatBlocked;
	psAggregateStats->ui64TimeStamp       = ui64TimeNow;

	for (eDM = 0; eDM < ui32MaxDMCount; eDM++)
	{
		FOREACH_SUPPORTED_DRIVER(ui32DriverID)
		{
			psAggregateStats->aaui64DMOSStatInactive[eDM][ui32DriverID]    += psReturnStats->aaui64DMOSStatInactive[eDM][ui32DriverID];
			psAggregateStats->aaui64DMOSStatActive[eDM][ui32DriverID]  += psReturnStats->aaui64DMOSStatActive[eDM][ui32DriverID];
		}
	}

	/***** (5) Convert return stats to microseconds *****/

	psReturnStats->ui64GpuStatIdle       = OSDivide64(psReturnStats->ui64GpuStatIdle, 1000, &ui32Remainder);
	psReturnStats->ui64GpuStatActive     = OSDivide64(psReturnStats->ui64GpuStatActive, 1000, &ui32Remainder);
	psReturnStats->ui64GpuStatBlocked    = OSDivide64(psReturnStats->ui64GpuStatBlocked, 1000, &ui32Remainder);
	psReturnStats->ui64GpuStatCumulative = OSDivide64(psReturnStats->ui64GpuStatCumulative, 1000, &ui32Remainder);

	/* Check that the return stats make sense */
	if (psReturnStats->ui64GpuStatCumulative == 0)
	{
		/* We can enter here only if allocating the temporary stats
		 * buffers failed, or all the RGXFWIF_GPU_UTIL_GET_PERIOD
		 * returned 0. The latter could happen if the GPU frequency value
		 * is not well calibrated and the FW is updating the GPU state
		 * while the Host is reading it.
		 * When such an event happens frequently, timers or the aggregate
		 * stats might not be accurate...
		 */
#if defined(VIRTUAL_PLATFORM)
		/* To avoid spamming the console logging system on emulated devices,
		 * we special-case so that we will only produce a single message per
		 * driver invocation. This should reduce the time spent logging
		 * information which is not relevant for very slow timers found in
		 * VP device configurations
		 */
		static IMG_BOOL bFirstTime = IMG_TRUE;

		if (bFirstTime)
		{
			bFirstTime = IMG_FALSE;
#endif
		PVR_DPF((PVR_DBG_WARNING, "RGXGetGpuUtilStats could not get reliable data."));
#if defined(VIRTUAL_PLATFORM)
		}
#endif	/* defined(VIRTUAL_PLATFORM) */
		return PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
	}

	psReturnStats->bValid = IMG_TRUE;

	return PVRSRV_OK;
}

PVRSRV_ERROR SORgxGpuUtilStatsRegister(IMG_HANDLE *phGpuUtilUser)
{
	RGXFWIF_GPU_UTIL_STATS *psAggregateStats;

	/* NoStats used since this may be called outside of the register/de-register
	 * process calls which track memory use. */
	psAggregateStats = OSAllocZMemNoStats(sizeof(RGXFWIF_GPU_UTIL_STATS));
	if (psAggregateStats == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	*phGpuUtilUser = psAggregateStats;

	return PVRSRV_OK;
}

PVRSRV_ERROR SORgxGpuUtilStatsUnregister(IMG_HANDLE hGpuUtilUser)
{
	RGXFWIF_GPU_UTIL_STATS *psAggregateStats;

	if (hGpuUtilUser == NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psAggregateStats = hGpuUtilUser;
	OSFreeMemNoStats(psAggregateStats);

	return PVRSRV_OK;
}

/*
	RGX MISR Handler
*/
static void RGX_MISRHandler_Main (void *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = pvData;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	/* Give the HWPerf service a chance to transfer some data from the FW
	 * buffer to the host driver transport layer buffer.
	 */
	RGXHWPerfDataStoreCB(psDeviceNode);

	/* Inform other services devices that we have finished an operation */
	PVRSRVNotifyCommandCompletion(psDeviceNode);

#if defined(SUPPORT_PDVFS) && defined(RGXFW_META_SUPPORT_2ND_THREAD)
	/* Normally, firmware CCB only exists for the primary FW thread unless PDVFS
	   is running on the second[ary] FW thread, here we process said CCB */
	RGXPDVFSCheckCoreClkRateChange(psDeviceNode->pvDevice);
#endif

	/* Only execute SafetyEventHandler if RGX_FEATURE_SAFETY_EVENT is on */
	if (PVRSRV_GET_DEVICE_FEATURE_VALUE(psDeviceNode, ECC_RAMS) > 0 ||
		PVRSRV_IS_FEATURE_SUPPORTED(psDeviceNode, WATCHDOG_TIMER))
	{
		/* Handle Safety events if necessary */
		RGXSafetyEventHandler(psDeviceNode->pvDevice);
	}

	/* Signal the global event object */
	PVRSRVSignalDriverWideEO();

	/* Process the Firmware CCB for pending commands */
	RGXCheckFirmwareCCB(psDeviceNode->pvDevice);

	/* Calibrate the GPU frequency and recorrelate Host and GPU timers (done every few seconds) */
	RGXTimeCorrRestartPeriodic(psDeviceNode);

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Process Workload Estimation Specific commands from the FW */
		WorkEstCheckFirmwareCCB(psDeviceNode->pvDevice);
	}
#endif

	if (psDevInfo->pvAPMISRData == NULL)
	{
		RGX_MISR_ProcessKCCBDeferredList(psDeviceNode);
	}
}
#endif /* !defined(NO_HARDWARE) */


#if defined(RGX_FEATURE_MIPS_BIT_MASK) && defined(PDUMP)
static PVRSRV_ERROR RGXPDumpBootldrData(PVRSRV_DEVICE_NODE *psDeviceNode,
		PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PMR *psFWDataPMR;
	RGXMIPSFW_BOOT_DATA *psBootData;
	IMG_DEV_PHYADDR sTmpAddr;
	IMG_UINT32 ui32BootConfOffset, ui32ParamOffset, i;
	PVRSRV_ERROR eError;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	psFWDataPMR = (PMR *)(psDevInfo->psRGXFWDataMemDesc->psImport->hPMR);
	ui32BootConfOffset = RGXGetFWImageSectionOffset(NULL, MIPS_BOOT_DATA);
	ui32BootConfOffset += RGXMIPSFW_BOOTLDR_CONF_OFFSET;

	/* The physical addresses used by a pdump player will be different
	 * than the ones we have put in the MIPS bootloader configuration data.
	 * We have to tell the pdump player to replace the original values with the real ones.
	 */
	PDUMPCOMMENT(psDeviceNode, "Pass new boot parameters to the FW");

	/* Rogue Registers physical address */
	ui32ParamOffset = ui32BootConfOffset + offsetof(RGXMIPSFW_BOOT_DATA, ui64RegBase);

	eError = PDumpRegLabelToMem64(RGX_PDUMPREG_NAME,
			0x0,
			psFWDataPMR,
			ui32ParamOffset,
			PDUMP_FLAGS_CONTINUOUS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "RGXPDumpBootldrData: Dump of Rogue registers phy address failed (%u)", eError));
		return eError;
	}

	/* Page Table physical Address */
	eError = MMU_AcquireBaseAddr(psDevInfo->psKernelMMUCtx, &sTmpAddr);
	if (eError !=  PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "RGXBootldrDataInit: MMU_AcquireBaseAddr failed (%u)",
		         eError));
		return eError;
	}

	eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWDataMemDesc,
									 (void **)&psBootData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to acquire pointer to FW data (%s)",
				__func__, PVRSRVGetErrorString(eError)));
		return eError;
	}

	psBootData = IMG_OFFSET_ADDR(psBootData, ui32BootConfOffset);

	for (i = 0; i < psBootData->ui32PTNumPages; i++)
	{
		ui32ParamOffset = ui32BootConfOffset +
			offsetof(RGXMIPSFW_BOOT_DATA, aui64PTPhyAddr[0])
			+ i * sizeof(psBootData->aui64PTPhyAddr[0]);

		eError = PDumpPTBaseObjectToMem64(psDeviceNode->psFirmwareMMUDevAttrs->pszMMUPxPDumpMemSpaceName,
				psFWDataPMR,
				0,
				ui32ParamOffset,
				PDUMP_FLAGS_CONTINUOUS,
				MMU_LEVEL_1,
				sTmpAddr.uiAddr,
				i << psBootData->ui32PTLog2PageSize);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXPDumpBootldrData: Dump of page tables phy address failed (%u)", eError));
			return eError;
		}
	}

	DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWDataMemDesc);

	/* Stack physical address */
	ui32ParamOffset = ui32BootConfOffset + offsetof(RGXMIPSFW_BOOT_DATA, ui64StackPhyAddr);

	eError = PDumpMemLabelToMem64(psFWDataPMR,
			psFWDataPMR,
			RGXGetFWImageSectionOffset(NULL, MIPS_STACK),
			ui32ParamOffset,
			PDUMP_FLAGS_CONTINUOUS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "RGXPDumpBootldrData: Dump of stack phy address failed (%u)", eError));
		return eError;
	}

	return eError;
}
#endif /* PDUMP */

static PVRSRV_ERROR RGXSetPowerParams(PVRSRV_RGXDEV_INFO   *psDevInfo,
                                      PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_DEV_PHYADDR sKernelMMUCtxPCAddr;
	IMG_BOOL bPremappedFw;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVCFG, psDevConfig, PVRSRV_OK);

	/* Save information used on power transitions for later
	 * (when RGXStart and RGXStop are executed)
	 */
#if defined(PDUMP)
	psDevInfo->sLayerParams.ui32PdumpFlags = PDUMP_FLAGS_CONTINUOUS;
#endif

#if defined(SUPPORT_TRUSTED_DEVICE) && defined(RGX_PREMAP_FW_HEAPS)
	/* Rogue drivers with security support and premapped fw heaps
	 * always have their fw heap premapped by the TEE */
	bPremappedFw = IMG_TRUE;
#else
	/* If AutoVz firmware is up at this stage, the driver initialised it
	 * during a previous life-cycle. The firmware's memory is already pre-mapped
	 * and the MMU page tables reside in the predetermined memory carveout.
	 * The Kernel MMU Context created in this life-cycle is a dummy structure
	 * that is not used for mapping.
	 * To program the Device's BIF with the correct PC address, use the base
	 * address of the carveout reserved for MMU mappings as Kernel MMU PC Address */
	bPremappedFw = psDevInfo->psDeviceNode->bAutoVzFwIsUp;
#endif

	if (bPremappedFw)
	{
		IMG_DEV_PHYADDR sDevPAddr;
		PHYS_HEAP *psFwPageTableHeap = psDevInfo->psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PREMAP_PT];

		PVR_LOG_RETURN_IF_FALSE((NULL != psFwPageTableHeap),
								"Firmware Page Table heap not defined.",
								PVRSRV_ERROR_INVALID_HEAP);

		PhysHeapGetDevPAddr(psFwPageTableHeap, &sDevPAddr);
		sKernelMMUCtxPCAddr.uiAddr = sDevPAddr.uiAddr;
		psDevInfo->sLayerParams.sPCAddr = sKernelMMUCtxPCAddr;
	}
	else if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META) ||
	    RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
	{
		eError = MMU_AcquireBaseAddr(psDevInfo->psKernelMMUCtx,
		                             &sKernelMMUCtxPCAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXSetPowerParams: Failed to acquire Kernel MMU Ctx page catalog"));
			return eError;
		}

		psDevInfo->sLayerParams.sPCAddr = sKernelMMUCtxPCAddr;
	}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	else
	{
		PMR *psFWCodePMR = (PMR *)(psDevInfo->psRGXFWCodeMemDesc->psImport->hPMR);
		PMR *psFWDataPMR = (PMR *)(psDevInfo->psRGXFWDataMemDesc->psImport->hPMR);
		IMG_DEV_PHYADDR sPhyAddr;
		IMG_BOOL bValid;

#if defined(SUPPORT_ALT_REGBASE)
		psDevInfo->sLayerParams.sGPURegAddr = psDevConfig->sAltRegsGpuPBase;
#else
		/* The physical address of the GPU registers needs to be translated
		 * in case we are in a LMA scenario
		 */
		PhysHeapCpuPAddrToDevPAddr(psDevInfo->psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_GPU_LOCAL],
				1,
				&sPhyAddr,
				&(psDevConfig->sRegsCpuPBase));

		psDevInfo->sLayerParams.sGPURegAddr = sPhyAddr;
#endif

		/* Register bank must be aligned to 512KB (as per the core integration) to
		 * prevent the FW accessing incorrect registers */
		if ((psDevInfo->sLayerParams.sGPURegAddr.uiAddr & 0x7FFFFU) != 0U)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXSetPowerParams: Register bank must be aligned to 512KB, but current address (0x%016"IMG_UINT64_FMTSPECX") is not",
						psDevInfo->sLayerParams.sGPURegAddr.uiAddr));
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		eError = RGXGetPhyAddr(psFWCodePMR,
				&sPhyAddr,
				RGXGetFWImageSectionOffset(NULL, MIPS_BOOT_CODE),
				OSGetPageShift(), /* FW will be using the same page size as the OS */
				1,
				&bValid);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXSetPowerParams: Failed to acquire FW boot/NMI code address"));
			return eError;
		}

		psDevInfo->sLayerParams.sBootRemapAddr = sPhyAddr;

		eError = RGXGetPhyAddr(psFWDataPMR,
				&sPhyAddr,
				RGXGetFWImageSectionOffset(NULL, MIPS_BOOT_DATA),
				OSGetPageShift(),
				1,
				&bValid);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXSetPowerParams: Failed to acquire FW boot/NMI data address"));
			return eError;
		}

		psDevInfo->sLayerParams.sDataRemapAddr = sPhyAddr;

		eError = RGXGetPhyAddr(psFWCodePMR,
				&sPhyAddr,
				RGXGetFWImageSectionOffset(NULL, MIPS_EXCEPTIONS_CODE),
				OSGetPageShift(),
				1,
				&bValid);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "RGXSetPowerParams: Failed to acquire FW exceptions address"));
			return eError;
		}

		psDevInfo->sLayerParams.sCodeRemapAddr = sPhyAddr;

		psDevInfo->sLayerParams.sTrampolineRemapAddr.uiAddr = psDevInfo->psTrampoline->sPhysAddr.uiAddr;

		psDevInfo->sLayerParams.bDevicePA0IsValid = psDevConfig->bDevicePA0IsValid;
	}
#endif

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(NO_HARDWARE) && !defined(SUPPORT_SECURITY_VALIDATION)
	/* Send information used on power transitions to the trusted device as
	 * in this setup the driver cannot start/stop the GPU and perform resets
	 */
	if (psDevConfig->pfnTDSetPowerParams)
	{
		PVRSRV_TD_POWER_PARAMS sTDPowerParams;

		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META) ||
		    RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
		{
			sTDPowerParams.sPCAddr = psDevInfo->sLayerParams.sPCAddr;
		}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
		else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
		{
			sTDPowerParams.sGPURegAddr    = psDevInfo->sLayerParams.sGPURegAddr;
			sTDPowerParams.sBootRemapAddr = psDevInfo->sLayerParams.sBootRemapAddr;
			sTDPowerParams.sCodeRemapAddr = psDevInfo->sLayerParams.sCodeRemapAddr;
			sTDPowerParams.sDataRemapAddr = psDevInfo->sLayerParams.sDataRemapAddr;
		}
#endif

		eError = psDevConfig->pfnTDSetPowerParams(psDevConfig->hSysData,
												  &sTDPowerParams);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "RGXSetPowerParams: TDSetPowerParams not implemented!"));
		eError = PVRSRV_ERROR_NOT_IMPLEMENTED;
	}
#endif

	return eError;
}

#if defined(RGX_FEATURE_AXI_ACE_BIT_MASK)
/*
	RGXSystemGetFabricCoherency
*/
PVRSRV_ERROR RGXSystemGetFabricCoherency(PVRSRV_DEVICE_CONFIG *psDevConfig,
										 IMG_CPU_PHYADDR sRegsCpuPBase,
										 IMG_UINT32 ui32RegsSize,
										 PVRSRV_DEVICE_FABRIC_TYPE *peDevFabricType,
										 PVRSRV_DEVICE_SNOOP_MODE *peCacheSnoopingMode)
{
	IMG_CHAR *aszLabels[] = {"none", "acelite", "fullace", "unknown"};
	PVRSRV_DEVICE_SNOOP_MODE eAppHintCacheSnoopingMode;
	PVRSRV_DEVICE_SNOOP_MODE eDeviceCacheSnoopingMode;
	IMG_UINT32 ui32AppHintFabricCoherency;
	IMG_UINT32 ui32DeviceFabricCoherency;
	void *pvAppHintState = NULL;
	IMG_UINT32 ui32AppHintDefault;
#if !defined(NO_HARDWARE)
	void *pvRegsBaseKM;
	IMG_BOOL bPowerDown = IMG_TRUE;
	PVRSRV_ERROR eError;
#endif

	if (!sRegsCpuPBase.uiAddr || !ui32RegsSize)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "RGXSystemGetFabricCoherency: Invalid RGX register base/size parameters"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if !defined(NO_HARDWARE)
	pvRegsBaseKM = OSMapPhysToLin(sRegsCpuPBase, ui32RegsSize, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (!pvRegsBaseKM)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "RGXSystemGetFabricCoherency: Failed to create RGX register mapping"));
		return PVRSRV_ERROR_BAD_MAPPING;
	}

	bPowerDown = ! PVRSRVIsSystemPowered(psDevConfig->psDevNode);

	/* Power-up the device as required to read the registers */
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVCFG, psDevConfig) && bPowerDown)
	{
		eError = PVRSRVSetSystemPowerState(psDevConfig, PVRSRV_SYS_POWER_STATE_ON);
		PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState ON");
	}

	/* AXI support within the SoC, bitfield COHERENCY_SUPPORT [1 .. 0]
		value NO_COHERENCY        0x0 {SoC does not support any form of Coherency}
		value ACE_LITE_COHERENCY  0x1 {SoC supports ACE-Lite or I/O Coherency}
		value FULL_ACE_COHERENCY  0x2 {SoC supports full ACE or 2-Way Coherency} */
	ui32DeviceFabricCoherency = OSReadHWReg32((void __iomem *)pvRegsBaseKM, RGX_CR_SOC_AXI);
	PVR_LOG(("AXI fabric coherency (RGX_CR_SOC_AXI): 0x%x", ui32DeviceFabricCoherency));
#if defined(DEBUG)
	if (ui32DeviceFabricCoherency & ~((IMG_UINT32)RGX_CR_SOC_AXI_MASKFULL))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid RGX_CR_SOC_AXI value.", __func__));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
#endif
	ui32DeviceFabricCoherency &= ~((IMG_UINT32)RGX_CR_SOC_AXI_COHERENCY_SUPPORT_CLRMSK);
	ui32DeviceFabricCoherency >>= RGX_CR_SOC_AXI_COHERENCY_SUPPORT_SHIFT;

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVCFG, psDevConfig) && bPowerDown)
	{
		eError = PVRSRVSetSystemPowerState(psDevConfig, PVRSRV_SYS_POWER_STATE_OFF);
		PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState OFF");
	}

	/* UnMap Regs */
	OSUnMapPhysToLin(pvRegsBaseKM, ui32RegsSize);

	switch (ui32DeviceFabricCoherency)
	{
	case RGX_CR_SOC_AXI_COHERENCY_SUPPORT_FULL_ACE_COHERENCY:
		eDeviceCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_CROSS;
		*peDevFabricType = PVRSRV_DEVICE_FABRIC_FULLACE;
		break;

	case RGX_CR_SOC_AXI_COHERENCY_SUPPORT_ACE_LITE_COHERENCY:
		eDeviceCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_CPU_ONLY;
		*peDevFabricType = PVRSRV_DEVICE_FABRIC_ACELITE;
		break;

	case RGX_CR_SOC_AXI_COHERENCY_SUPPORT_NO_COHERENCY:
	default:
		eDeviceCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;
		*peDevFabricType = PVRSRV_DEVICE_FABRIC_NONE;
		break;
	}
#else /* !defined(NO_HARDWARE) */
    *peDevFabricType = PVRSRV_DEVICE_FABRIC_ACELITE;
    eDeviceCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_CPU_ONLY;
    ui32DeviceFabricCoherency = RGX_CR_SOC_AXI_COHERENCY_SUPPORT_ACE_LITE_COHERENCY;
#endif /* !defined(NO_HARDWARE) */

	OSCreateAppHintState(&pvAppHintState);
	ui32AppHintDefault = RGX_CR_SOC_AXI_COHERENCY_SUPPORT_FULL_ACE_COHERENCY;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, FabricCoherencyOverride,
						 &ui32AppHintDefault, &ui32AppHintFabricCoherency);
	OSFreeAppHintState(pvAppHintState);

#if defined(SUPPORT_SECURITY_VALIDATION)
	/* Temporarily disable coherency */
	ui32AppHintFabricCoherency = RGX_CR_SOC_AXI_COHERENCY_SUPPORT_NO_COHERENCY;
#endif

	/* Suppress invalid AppHint value */
	switch (ui32AppHintFabricCoherency)
	{
	case RGX_CR_SOC_AXI_COHERENCY_SUPPORT_NO_COHERENCY:
		eAppHintCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;
		break;

	case RGX_CR_SOC_AXI_COHERENCY_SUPPORT_ACE_LITE_COHERENCY:
		eAppHintCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_CPU_ONLY;
		break;

	case RGX_CR_SOC_AXI_COHERENCY_SUPPORT_FULL_ACE_COHERENCY:
		eAppHintCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_CROSS;
		break;

	default:
		PVR_DPF((PVR_DBG_ERROR,
				"Invalid FabricCoherencyOverride AppHint %d, ignoring",
				ui32AppHintFabricCoherency));
		eAppHintCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_CROSS;
		ui32AppHintFabricCoherency = RGX_CR_SOC_AXI_COHERENCY_SUPPORT_FULL_ACE_COHERENCY;
		break;
	}

	if (ui32AppHintFabricCoherency < ui32DeviceFabricCoherency)
	{
		PVR_LOG(("Downgrading device fabric coherency from %s to %s",
				aszLabels[ui32DeviceFabricCoherency],
				aszLabels[ui32AppHintFabricCoherency]));
		eDeviceCacheSnoopingMode = eAppHintCacheSnoopingMode;
	}
	else if (ui32AppHintFabricCoherency > ui32DeviceFabricCoherency)
	{
		PVR_DPF((PVR_DBG_WARNING,
				"Cannot upgrade device fabric coherency from %s to %s, not supported by device!",
				aszLabels[ui32DeviceFabricCoherency],
				aszLabels[ui32AppHintFabricCoherency]));

		/* Override requested-for app-hint with actual app-hint value being used */
		ui32AppHintFabricCoherency = ui32DeviceFabricCoherency;
	}

	*peCacheSnoopingMode = eDeviceCacheSnoopingMode;
	return PVRSRV_OK;
}
#endif

/*
	RGXSystemHasFBCDCVersion31
*/
static IMG_BOOL RGXSystemHasFBCDCVersion31(PVRSRV_DEVICE_NODE *psDeviceNode)
{

#if defined(HW_ERN_66622_BIT_MASK)
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (RGX_IS_ERN_SUPPORTED(psDevInfo, 66622))
#endif
	{
		{
			if (psDeviceNode->psDevConfig->bHasFBCDCVersion31)
			{
				return IMG_TRUE;
			}
		}
	}
#if defined(HW_ERN_66622_BIT_MASK)
	else
	{

#if !defined(NO_HARDWARE)
		if (psDeviceNode->psDevConfig->bHasFBCDCVersion31)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: System uses FBCDC3.1 but GPU doesn't support it!",
			         __func__));
		}
#endif
	}
#endif /* defined(HW_ERN_66622_BIT_MASK) */

	return IMG_FALSE;
}

/*
	RGXGetTFBCLossyGroup
*/
static IMG_UINT32 RGXGetTFBCLossyGroup(PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK)
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	return psDevInfo->ui32TFBCLossyGroup;
#else
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	return 0;
#endif
}

/*
	RGXDevMMUAttributes
*/
static MMU_DEVICEATTRIBS *RGXDevMMUAttributes(PVRSRV_DEVICE_NODE *psDeviceNode,
                                              IMG_BOOL bKernelFWMemoryCtx)
{
	MMU_DEVICEATTRIBS *psMMUDevAttrs = NULL;

	if (psDeviceNode->pfnCheckDeviceFeature)
	{
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
		if (PVRSRV_IS_FEATURE_SUPPORTED(psDeviceNode, MIPS))
		{
			psMMUDevAttrs = bKernelFWMemoryCtx ?
			                psDeviceNode->psFirmwareMMUDevAttrs :
			                psDeviceNode->psMMUDevAttrs;
		}
		else
#endif
		{
			PVR_UNREFERENCED_PARAMETER(bKernelFWMemoryCtx);
			psMMUDevAttrs = psDeviceNode->psMMUDevAttrs;
		}
	}

	return psMMUDevAttrs;
}

/*
	RGXDevSnoopMode
*/
static PVRSRV_DEVICE_SNOOP_MODE RGXDevSnoopMode(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(psDeviceNode != NULL);
	PVR_ASSERT(psDeviceNode->pvDevice != NULL);

	psDevInfo = (PVRSRV_RGXDEV_INFO *) psDeviceNode->pvDevice;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, AXI_ACELITE))
	{
		return PVRSRV_DEVICE_SNOOP_CPU_ONLY;
	}

	return PVRSRV_DEVICE_SNOOP_NONE;
}

/*
 * RGXInitDevPart2
 */
PVRSRV_ERROR RGXInitDevPart2(PVRSRV_DEVICE_NODE	*psDeviceNode,
                             RGX_INIT_APPHINTS	*psApphints)
{
	PVRSRV_ERROR			eError;
	PVRSRV_RGXDEV_INFO		*psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_DEV_POWER_STATE	eDefaultPowerState = PVRSRV_DEV_POWER_STATE_ON;
	PVRSRV_DEVICE_CONFIG	*psDevConfig = psDeviceNode->psDevConfig;
#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	IMG_UINT32			ui32AllPowUnitsMask = (1 << psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount) - 1;
	IMG_UINT32			ui32AllRACMask = (1 << psDevInfo->sDevFeatureCfg.ui32MAXRACCount) - 1;
#endif

	PDUMPCOMMENT(psDeviceNode, "RGX Initialisation Part 2");

#if defined(RGX_FEATURE_MIPS_BIT_MASK) && defined(PDUMP)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		RGXPDumpBootldrData(psDeviceNode, psDevInfo);
	}
#endif
#if defined(TIMING) || defined(DEBUG)
	OSUserModeAccessToPerfCountersEn();
#endif

	/* Initialise Device Flags */
	psDevInfo->ui32DeviceFlags = 0;
	RGXSetDeviceFlags(psDevInfo, psApphints->ui32DeviceFlags, IMG_TRUE);

	/* Allocate DVFS Table (needs to be allocated before GPU trace events
	 *  component is initialised because there is a dependency between them) */
	psDevInfo->psGpuDVFSTable = OSAllocZMem(sizeof(*(psDevInfo->psGpuDVFSTable)));
	PVR_LOG_GOTO_IF_NOMEM(psDevInfo->psGpuDVFSTable, eError, ErrorExit);

	if (psDevInfo->ui32HWPerfHostFilter == 0)
	{
		RGXHWPerfHostSetEventFilter(psDevInfo, psApphints->ui32HWPerfHostFilter);
	}

	/* If HWPerf enabled allocate all resources for the host side buffer. */
	if (psDevInfo->ui32HWPerfHostFilter != 0)
	{
		if (RGXHWPerfHostInitOnDemandResources(psDevInfo) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_WARNING, "HWPerfHost buffer on demand"
			        " initialisation failed."));
		}
	}

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Initialise work estimation lock */
		eError = OSLockCreate(&psDevInfo->hWorkEstLock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(WorkEstLock)", ErrorExit);
	}
#endif

	/* Initialise lists of ZSBuffers */
	eError = OSLockCreate(&psDevInfo->hLockZSBuffer);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(LockZSBuffer)", ErrorExit);
	dllist_init(&psDevInfo->sZSBufferHead);
	psDevInfo->ui32ZSBufferCurrID = 1;

	/* Initialise lists of growable Freelists */
	eError = OSLockCreate(&psDevInfo->hLockFreeList);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(LockFreeList)", ErrorExit);
	dllist_init(&psDevInfo->sFreeListHead);
	psDevInfo->ui32FreelistCurrID = 1;

	eError = OSLockCreate(&psDevInfo->hDebugFaultInfoLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(DebugFaultInfoLock)", ErrorExit);

	if (GetInfoPageDebugFlagsKM() & DEBUG_FEATURE_PAGE_FAULT_DEBUG_ENABLED)
	{
		eError = OSLockCreate(&psDevInfo->hMMUCtxUnregLock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(MMUCtxUnregLock)", ErrorExit);
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		eError = OSLockCreate(&psDevInfo->hNMILock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(NMILock)", ErrorExit);
	}
#endif

	/* Setup GPU utilisation stats update callback */
	eError = OSLockCreate(&psDevInfo->hGPUUtilLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(GPUUtilLock)", ErrorExit);
#if !defined(NO_HARDWARE)
	psDevInfo->pfnGetGpuUtilStats = RGXGetGpuUtilStats;
#endif

	eDefaultPowerState = PVRSRV_DEV_POWER_STATE_ON;
	psDevInfo->eActivePMConf = psApphints->eRGXActivePMConf;

#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	/* Validate the SPU mask and initialize to number of SPUs to power up */
	if ((psApphints->ui32AvailablePowUnitsMask & ui32AllPowUnitsMask) == 0)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s:Invalid SPU mask (All=0x%X, Non Fused=0x%X). At-least one SPU must to be powered up.",
		         __func__,
		         ui32AllPowUnitsMask,
		         psApphints->ui32AvailablePowUnitsMask));
		PVR_LOG_GOTO_WITH_ERROR("ui32AvailablePowUnitsMask", eError, PVRSRV_ERROR_INVALID_SPU_MASK, ErrorExit);
	}

	psDevInfo->ui32AvailablePowUnitsMask = psApphints->ui32AvailablePowUnitsMask & ui32AllPowUnitsMask;

	psDevInfo->ui32AvailableRACMask = psApphints->ui32AvailableRACMask & ui32AllRACMask;
#endif

#if !defined(NO_HARDWARE)
	/* set-up the Active Power Mgmt callback */
	{
		RGX_DATA *psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;
		IMG_BOOL bSysEnableAPM = psRGXData->psRGXTimingInfo->bEnableActivePM;
		IMG_BOOL bEnableAPM = ((psApphints->eRGXActivePMConf == RGX_ACTIVEPM_DEFAULT) && bSysEnableAPM) ||
							   (psApphints->eRGXActivePMConf == RGX_ACTIVEPM_FORCE_ON);

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1) && defined(SUPPORT_AUTOVZ)
		/* The AutoVz driver enables a virtualisation watchdog not compatible with APM */
		if (bEnableAPM && (!PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode)))
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Active Power Management disabled in AutoVz mode", __func__));
			bEnableAPM = IMG_FALSE;
		}

		PVR_ASSERT(bEnableAPM == IMG_FALSE);
#endif

		if (bEnableAPM)
		{
			eError = OSInstallMISR(&psDevInfo->pvAPMISRData,
					RGX_MISRHandler_CheckFWActivePowerState,
					psDeviceNode,
					"RGX_CheckFWActivePower");
			PVR_LOG_GOTO_IF_ERROR(eError, "OSInstallMISR(APMISR)", ErrorExit);

			/* Prevent the device being woken up before there is something to do. */
			eDefaultPowerState = PVRSRV_DEV_POWER_STATE_OFF;
		}
	}
#endif

	psDevInfo->eDebugDumpFWTLogType = psApphints->eDebugDumpFWTLogType;

	PVRSRVAppHintRegisterHandlersUINT32(APPHINT_ID_EnableAPM,
	                                    RGXQueryAPMState,
	                                    RGXSetAPMState,
	                                    psDeviceNode,
	                                    NULL);

	RGXTimeCorrInitAppHintCallbacks(psDeviceNode);

	/* Register the device with the power manager */
	eError = PVRSRVRegisterPowerDevice(psDeviceNode,
	                                   (PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode)) ? &RGXPrePowerState : &RGXVzPrePowerState,
	                                   (PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode)) ? &RGXPostPowerState : &RGXVzPostPowerState,
	                                   psDevConfig->pfnPrePowerState, psDevConfig->pfnPostPowerState,
	                                   &RGXPreClockSpeedChange, &RGXPostClockSpeedChange,
	                                   &RGXForcedIdleRequest, &RGXCancelForcedIdleRequest,
	                                   &RGXPowUnitsChange,
	                                   (IMG_HANDLE)psDeviceNode,
	                                   PVRSRV_DEV_POWER_STATE_OFF,
	                                   eDefaultPowerState);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVRegisterPowerDevice", ErrorExit);

	eError = RGXSetPowerParams(psDevInfo, psDevConfig);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetPowerParams", ErrorExit);


#if defined(PDUMP)
#if defined(NO_HARDWARE)
	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_DEINIT, "Wait for the FW to signal idle");

	/* Kick the FW once, in case it still needs to detect and set the idle state */
	PDUMPREG32(psDeviceNode, RGX_PDUMPREG_NAME,
			   RGX_CR_MTS_SCHEDULE,
			   RGXFWIF_DM_GP & ~RGX_CR_MTS_SCHEDULE_DM_CLRMSK,
			   PDUMP_FLAGS_CONTINUOUS | PDUMP_FLAGS_DEINIT);

	eError = DevmemPDumpDevmemPol32(psDevInfo->psRGXFWIfFwSysDataMemDesc,
	                                offsetof(RGXFWIF_SYSDATA, ePowState),
	                                RGXFWIF_POW_IDLE,
	                                0xFFFFFFFFU,
	                                PDUMP_POLL_OPERATOR_EQUAL,
	                                PDUMP_FLAGS_CONTINUOUS | PDUMP_FLAGS_DEINIT);
	PVR_LOG_GOTO_IF_ERROR(eError, "DevmemPDumpDevmemPol32", ErrorExit);
#endif

	/* Run RGXStop with the correct PDump flags to feed the last-frame deinit buffer */
	PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_DEINIT,
	                      "RGX deinitialisation commands");

	psDevInfo->sLayerParams.ui32PdumpFlags |= PDUMP_FLAGS_DEINIT | PDUMP_FLAGS_NOHW;

	if (! PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = RGXStop(&psDevInfo->sLayerParams);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXStop", ErrorExit);
	}

	psDevInfo->sLayerParams.ui32PdumpFlags &= ~(PDUMP_FLAGS_DEINIT | PDUMP_FLAGS_NOHW);
#endif

#if !defined(NO_HARDWARE)
	eError = RGXInstallProcessQueuesMISR(&psDevInfo->hProcessQueuesMISR, psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXInstallProcessQueuesMISR", ErrorExit);

	/* Register RGX to receive notifies when other devices complete some work */
	PVRSRVRegisterCmdCompleteNotify(&psDeviceNode->hCmdCompNotify, &RGXScheduleProcessQueuesKM, psDeviceNode);

	/* Register the interrupt handlers */
	eError = OSInstallMISR(&psDevInfo->pvMISRData,
			RGX_MISRHandler_Main,
			psDeviceNode,
			"RGX_Main");
	PVR_LOG_GOTO_IF_ERROR(eError, "OSInstallMISR(MISR)", ErrorExit);

	/* Register appropriate mechanism for clearing hw interrupts */
	eError = RGXSetAckIrq(psDevInfo);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetAckIrq", ErrorExit);

	eError = SysInstallDeviceLISR(psDevConfig->hSysData,
								  psDevConfig->ui32IRQ,
								  PVRSRV_MODNAME,
								  RGX_LISRHandler,
								  psDeviceNode,
								  &psDevInfo->pvLISRData);
	PVR_LOG_GOTO_IF_ERROR(eError, "SysInstallDeviceLISR", ErrorExit);
#endif /* !defined(NO_HARDWARE) */

#if defined(PDUMP)
    if (PVRSRVSystemSnoopingOfCPUCache(psDevConfig))
    {
        PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
                              "System has CPU cache snooping");
    }
    else
    {
        PDUMPCOMMENTWITHFLAGS(psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
                              "System has NO cache snooping");
    }
#endif

#if defined(RGX_FEATURE_COMPUTE_ONLY_BIT_MASK)
	if (!RGX_IS_FEATURE_SUPPORTED(psDevInfo, COMPUTE_ONLY))
#endif
	{
		eError = PVRSRVTQLoadShaders(psDeviceNode);
		PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVTQLoadShaders", ErrorExit);
	}

#if defined(SUPPORT_SECURE_ALLOC_KM)
	eError = OSAllocateSecBuf(psDeviceNode, RGXFWIF_KM_GENERAL_HEAP_RESERVED_SIZE, "SharedSecMem", &psDevInfo->psGenHeapSecMem);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSAllocateSecBuf", ErrorExit);
#endif

	psDevInfo->bDevInit2Done = IMG_TRUE;

	return PVRSRV_OK;

ErrorExit:
	DevPart2DeInitRGX(psDeviceNode);

	return eError;
}

#define VZ_RGX_FW_FILENAME_SUFFIX ".vz"
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
#define RGX_64K_FW_FILENAME_SUFFIX ".64k"
#define RGX_FW_FILENAME_MAX_SIZE   ((sizeof(RGX_FW_FILENAME)+ \
			RGX_BVNC_STR_SIZE_MAX+sizeof(VZ_RGX_FW_FILENAME_SUFFIX) + sizeof(RGX_64K_FW_FILENAME_SUFFIX)))
#else
#define RGX_FW_FILENAME_MAX_SIZE   ((sizeof(RGX_FW_FILENAME)+ \
			RGX_BVNC_STR_SIZE_MAX+sizeof(VZ_RGX_FW_FILENAME_SUFFIX)))
#endif

static void _GetFWFileName(PVRSRV_DEVICE_NODE *psDeviceNode,
		IMG_CHAR *pszFWFilenameStr,
		IMG_CHAR *pszFWpFilenameStr)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	const IMG_CHAR * const pszFWFilenameSuffix =
			PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode) ? "" : VZ_RGX_FW_FILENAME_SUFFIX;

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	const IMG_CHAR * const pszFWFilenameSuffix2 =
			((OSGetPageSize() == RGX_MMU_PAGE_SIZE_64KB) &&
			 RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
			? RGX_64K_FW_FILENAME_SUFFIX : "";
#else
	const IMG_CHAR * const pszFWFilenameSuffix2 = "";
#endif

	OSSNPrintf(pszFWFilenameStr, RGX_FW_FILENAME_MAX_SIZE,
			RGX_FW_FILENAME "." RGX_BVNC_STR_FMTSPEC "%s%s",
			psDevInfo->sDevFeatureCfg.ui32B, psDevInfo->sDevFeatureCfg.ui32V,
			psDevInfo->sDevFeatureCfg.ui32N, psDevInfo->sDevFeatureCfg.ui32C,
			pszFWFilenameSuffix, pszFWFilenameSuffix2);

	OSSNPrintf(pszFWpFilenameStr, RGX_FW_FILENAME_MAX_SIZE,
			RGX_FW_FILENAME "." RGX_BVNC_STRP_FMTSPEC "%s%s",
			psDevInfo->sDevFeatureCfg.ui32B, psDevInfo->sDevFeatureCfg.ui32V,
			psDevInfo->sDevFeatureCfg.ui32N, psDevInfo->sDevFeatureCfg.ui32C,
			pszFWFilenameSuffix, pszFWFilenameSuffix2);
}

PVRSRV_ERROR RGXLoadAndGetFWData(PVRSRV_DEVICE_NODE *psDeviceNode,
                                 OS_FW_IMAGE **ppsRGXFW)
{
	IMG_CHAR aszFWFilenameStr[RGX_FW_FILENAME_MAX_SIZE];
	IMG_CHAR aszFWpFilenameStr[RGX_FW_FILENAME_MAX_SIZE];
	IMG_CHAR *pszLoadedFwStr;
	PVRSRV_ERROR eErr;

	/* Prepare the image filenames to use in the following code */
	_GetFWFileName(psDeviceNode, aszFWFilenameStr, aszFWpFilenameStr);

	/* Get pointer to Firmware image */
	pszLoadedFwStr = aszFWFilenameStr;
	eErr = OSLoadFirmware(psDeviceNode, pszLoadedFwStr, OS_FW_VERIFY_FUNCTION, ppsRGXFW);
	if (eErr == PVRSRV_ERROR_NOT_FOUND)
	{
		pszLoadedFwStr = aszFWpFilenameStr;
		eErr = OSLoadFirmware(psDeviceNode, pszLoadedFwStr, OS_FW_VERIFY_FUNCTION, ppsRGXFW);
		if (eErr == PVRSRV_ERROR_NOT_FOUND)
		{
			pszLoadedFwStr = RGX_FW_FILENAME;
			eErr = OSLoadFirmware(psDeviceNode, pszLoadedFwStr, OS_FW_VERIFY_FUNCTION, ppsRGXFW);
			if (eErr == PVRSRV_ERROR_NOT_FOUND)
			{
				PVR_DPF((PVR_DBG_FATAL, "All RGX Firmware image loads failed for '%s' (%s)",
						aszFWFilenameStr, PVRSRVGetErrorString(eErr)));
			}
		}
	}

	if (eErr == PVRSRV_OK)
	{
		PVR_LOG(("RGX Firmware image '%s' loaded", pszLoadedFwStr));
	}

	return eErr;
}

#if defined(PDUMP)
PVRSRV_ERROR RGXInitHWPerfCounters(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	return PVRSRV_OK;
}
#endif

PVRSRV_ERROR RGXInitCreateFWKernelMemoryContext(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/* set up fw memory contexts */
	PVRSRV_RGXDEV_INFO   *psDevInfo = psDeviceNode->pvDevice;
	__maybe_unused PVRSRV_DEVICE_CONFIG *psDevConfig = psDeviceNode->psDevConfig;
	PVRSRV_ERROR eError;

#if defined(RGX_PREMAP_FW_HEAPS) || defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	IMG_BOOL  bNativeFwUMAHeap = PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode) &&
	                             (PhysHeapGetType(psDeviceNode->apsPhysHeap[FIRST_PHYSHEAP_MAPPED_TO_FW_MAIN_DEVMEM]) == PHYS_HEAP_TYPE_UMA);
#endif

#if defined(RGX_PREMAP_FW_HEAPS)
	PHYS_HEAP *psDefaultPhysHeap = psDeviceNode->psMMUPhysHeap;

	if ((!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode)) && (!psDeviceNode->bAutoVzFwIsUp) && (!bNativeFwUMAHeap))
	{
		PHYS_HEAP *psFwPageTableHeap =
				psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PREMAP_PT];

		PVR_LOG_GOTO_IF_INVALID_PARAM((psFwPageTableHeap != NULL),
		                              eError, failed_to_create_ctx);

		/* Temporarily swap the MMU and default GPU physheap to allow the page
		 * tables of all memory mapped by the FwKernel context to be placed
		 * in a dedicated memory carveout. This should allow the firmware mappings to
		 * persist after a Host kernel crash or driver reset. */
		psDeviceNode->psMMUPhysHeap = psFwPageTableHeap;
	}
#endif

#if defined(RGX_FEATURE_AXI_ACE_BIT_MASK)
	/* Set the device fabric coherency before FW context creation */
	eError = RGXSystemGetFabricCoherency(psDevConfig,
										 psDevConfig->sRegsCpuPBase,
										 psDevConfig->ui32RegsSize,
										 &psDeviceNode->eDevFabricType,
										 &psDevConfig->eCacheSnoopingMode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed RGXSystemGetFabricCoherency (%u)",
		         __func__,
		         eError));
		goto failed_to_create_ctx;
	}
#endif

	/* Register callbacks for creation of device memory contexts */
	psDeviceNode->pfnRegisterMemoryContext = RGXRegisterMemoryContext;
	psDeviceNode->pfnUnregisterMemoryContext = RGXUnregisterMemoryContext;

	RGXFwSharedMemCheckSnoopMode(psDevConfig);

	/* Create the memory context for the firmware. */
	eError = DevmemCreateContext(psDeviceNode, DEVMEM_HEAPCFG_FORFW,
	                             &psDevInfo->psKernelDevmemCtx);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed DevmemCreateContext (%u)",
		         __func__,
		         eError));
		goto failed_to_create_ctx;
	}

	eError = DevmemFindHeapByName(psDevInfo->psKernelDevmemCtx, RGX_FIRMWARE_MAIN_HEAP_IDENT,
	                              &psDevInfo->psFirmwareMainHeap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed DevmemFindHeapByName (%u)",
		         __func__,
		         eError));
		goto failed_to_find_heap;
	}

	eError = DevmemFindHeapByName(psDevInfo->psKernelDevmemCtx, RGX_FIRMWARE_CONFIG_HEAP_IDENT,
	                              &psDevInfo->psFirmwareConfigHeap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed DevmemFindHeapByName (%u)",
		         __func__,
		         eError));
		goto failed_to_find_heap;
	}

#if (defined(RGX_PREMAP_FW_HEAPS)) || (defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1))
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		IMG_UINT32 ui32DriverID;

		FOREACH_DRIVER_RAW_HEAP(ui32DriverID, DEVNODE, psDeviceNode)
		{
			IMG_CHAR szHeapName[RA_MAX_NAME_LENGTH];

			OSSNPrintf(szHeapName, sizeof(szHeapName), RGX_FIRMWARE_GUEST_RAW_HEAP_IDENT, ui32DriverID);
			eError = DevmemFindHeapByName(psDevInfo->psKernelDevmemCtx, szHeapName,
										  &psDevInfo->psPremappedFwRawHeap[ui32DriverID]);
			PVR_LOG_GOTO_IF_ERROR(eError, "DevmemFindHeapByName", failed_to_find_heap);
		}
	}
#endif

#if defined(RGX_PREMAP_FW_HEAPS) || defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && !bNativeFwUMAHeap)
	{
		IMG_DEV_PHYADDR sPhysHeapBase;
		IMG_UINT32 ui32DriverID;
		void *pvAppHintState = NULL;
		IMG_UINT64 ui64DefaultHeapStride;
		IMG_UINT64 ui64GuestHeapDevBaseStride;

		OSCreateAppHintState(&pvAppHintState);
		ui64DefaultHeapStride = PVRSRV_APPHINT_GUESTFWHEAPSTRIDE;
		OSGetAppHintUINT64(APPHINT_NO_DEVICE,
							pvAppHintState,
							GuestFWHeapStride,
							&ui64DefaultHeapStride,
							&ui64GuestHeapDevBaseStride);
		OSFreeAppHintState(pvAppHintState);
		pvAppHintState = NULL;

		eError = PhysHeapGetDevPAddr(psDeviceNode->apsPhysHeap[FIRST_PHYSHEAP_MAPPED_TO_FW_MAIN_DEVMEM], &sPhysHeapBase);
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapGetDevPAddr", failed_to_find_heap);

		FOREACH_DRIVER_RAW_HEAP(ui32DriverID, DEVNODE, psDeviceNode)
		{
			IMG_DEV_PHYADDR sRawFwHeapBase = {sPhysHeapBase.uiAddr + (ui32DriverID * ui64GuestHeapDevBaseStride)};

			eError = RGXFwRawHeapAllocMap(psDeviceNode,
										  ui32DriverID,
										  sRawFwHeapBase,
										  RGX_FIRMWARE_RAW_HEAP_SIZE);
			if (eError != PVRSRV_OK)
			{
				for (; ui32DriverID > RGX_FIRST_RAW_HEAP_DRIVER_ID; ui32DriverID--)
				{
					RGXFwRawHeapUnmapFree(psDeviceNode, ui32DriverID);
				}
				PVR_LOG_GOTO_IF_ERROR(eError, "RGXFwRawHeapAllocMap", failed_to_find_heap);
			}
		}

#if defined(RGX_PREMAP_FW_HEAPS)
		/* restore default Px setup */
		psDeviceNode->psMMUPhysHeap = psDefaultPhysHeap;
#endif
	}
#endif /* defined(RGX_PREMAP_FW_HEAPS) || defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) */

#if !defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	/* On setups with dynamically mapped Guest heaps, the Guest makes
	 * a PVZ call to the Host to request the mapping during init. */
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = PvzClientMapDevPhysHeap(psDevConfig);
		PVR_LOG_GOTO_IF_ERROR(eError, "PvzClientMapDevPhysHeap", failed_to_find_heap);
	}
#endif /* !defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) */

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		DevmemHeapSetPremapStatus(psDevInfo->psFirmwareMainHeap, IMG_TRUE);
		DevmemHeapSetPremapStatus(psDevInfo->psFirmwareConfigHeap, IMG_TRUE);
	}

	return eError;

failed_to_find_heap:
	/*
	 * Clear the mem context create callbacks before destroying the RGX firmware
	 * context to avoid a spurious callback.
	 */
	psDeviceNode->pfnRegisterMemoryContext = NULL;
	psDeviceNode->pfnUnregisterMemoryContext = NULL;
	DevmemDestroyContext(psDevInfo->psKernelDevmemCtx);
	psDevInfo->psKernelDevmemCtx = NULL;
failed_to_create_ctx:
	return eError;
}

void RGXDeInitDestroyFWKernelMemoryContext(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR       eError;
#if defined(RGX_PREMAP_FW_HEAPS)
	PHYS_HEAP *psDefaultPhysHeap = psDeviceNode->psMMUPhysHeap;
#endif

#if defined(RGX_PREMAP_FW_HEAPS) || defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
#if defined(RGX_PREMAP_FW_HEAPS)
		psDeviceNode->psMMUPhysHeap =
				psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PREMAP_PT];

		if (!psDeviceNode->bAutoVzFwIsUp)
#endif
		{
			IMG_UINT32 ui32DriverID;

			FOREACH_DRIVER_RAW_HEAP(ui32DriverID, DEVNODE, psDeviceNode)
			{
				RGXFwRawHeapUnmapFree(psDeviceNode, ui32DriverID);
			}
		}
	}
#endif /* defined(RGX_PREMAP_FW_HEAPS) || defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) */

#if !defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		(void) PvzClientUnmapDevPhysHeap(psDeviceNode->psDevConfig);

		if (psDevInfo->psFirmwareMainHeap)
		{
			DevmemHeapSetPremapStatus(psDevInfo->psFirmwareMainHeap, IMG_FALSE);
		}
		if (psDevInfo->psFirmwareConfigHeap)
		{
			DevmemHeapSetPremapStatus(psDevInfo->psFirmwareConfigHeap, IMG_FALSE);
		}
	}
#endif

	/*
	 * Clear the mem context create callbacks before destroying the RGX firmware
	 * context to avoid a spurious callback.
	 */
	psDeviceNode->pfnRegisterMemoryContext = NULL;
	psDeviceNode->pfnUnregisterMemoryContext = NULL;

	if (psDevInfo->psKernelDevmemCtx)
	{
		eError = DevmemDestroyContext(psDevInfo->psKernelDevmemCtx);
		PVR_ASSERT(eError == PVRSRV_OK);
	}

#if defined(RGX_PREMAP_FW_HEAPS)
	psDeviceNode->psMMUPhysHeap = psDefaultPhysHeap;
#endif
}

static PVRSRV_ERROR RGXAlignmentCheck(PVRSRV_DEVICE_NODE *psDevNode,
                                      IMG_UINT32 ui32AlignChecksSizeUM,
                                      IMG_UINT32 aui32AlignChecksUM[])
{
	static const IMG_UINT32 aui32AlignChecksKM[] = {RGXFW_ALIGN_CHECKS_INIT_KM};
	IMG_UINT32 ui32UMChecksOffset = ARRAY_SIZE(aui32AlignChecksKM) + 1;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDevNode->pvDevice;
	IMG_UINT32 i, *paui32FWAlignChecks;
	PVRSRV_ERROR eError = PVRSRV_OK;

	/* Skip the alignment check if the driver is guest
	   since there is no firmware to check against */
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDevNode, eError);

	if (psDevInfo->psRGXFWAlignChecksMemDesc == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: FW Alignment Check Mem Descriptor is NULL",
		         __func__));
		return PVRSRV_ERROR_ALIGNMENT_ARRAY_NOT_AVAILABLE;
	}

	eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWAlignChecksMemDesc,
	                                  (void **) &paui32FWAlignChecks);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to acquire kernel address for alignment checks (%u)",
		         __func__,
		         eError));
		return eError;
	}

	paui32FWAlignChecks += ui32UMChecksOffset;
	/* Invalidate the size value, check the next region size (UM) and invalidate */
	RGXFwSharedMemCacheOpPtr(paui32FWAlignChecks, INVALIDATE);
	if (*paui32FWAlignChecks++ != ui32AlignChecksSizeUM)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Mismatching sizes of RGXFW_ALIGN_CHECKS_INIT"
		         " array between UM(%d) and FW(%d)",
		         __func__,
		         ui32AlignChecksSizeUM,
		         *paui32FWAlignChecks));
		eError = PVRSRV_ERROR_INVALID_ALIGNMENT;
		goto return_;
	}

	RGXFwSharedMemCacheOpExec(paui32FWAlignChecks,
	                          ui32AlignChecksSizeUM * sizeof(IMG_UINT32),
	                          PVRSRV_CACHE_OP_INVALIDATE);

	for (i = 0; i < ui32AlignChecksSizeUM; i++)
	{
		if (aui32AlignChecksUM[i] != paui32FWAlignChecks[i])
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: size/offset mismatch in RGXFW_ALIGN_CHECKS_INIT[%d]"
					" between UM(%d) and FW(%d)",
					__func__, i, aui32AlignChecksUM[i], paui32FWAlignChecks[i]));
			eError = PVRSRV_ERROR_INVALID_ALIGNMENT;
		}
	}

	if (eError == PVRSRV_ERROR_INVALID_ALIGNMENT)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Check for FW/KM structure"
				" alignment failed.", __func__));
	}

return_:

	DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWAlignChecksMemDesc);

	return eError;
}

static
PVRSRV_ERROR RGXAllocateFWMemoryRegion(PVRSRV_DEVICE_NODE *psDeviceNode,
                                       IMG_DEVMEM_SIZE_T ui32Size,
                                       PVRSRV_MEMALLOCFLAGS_T uiMemAllocFlags,
                                       const IMG_PCHAR pszText,
                                       DEVMEM_MEMDESC **ppsMemDescPtr)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_DEVMEM_LOG2ALIGN_T uiLog2Align = OSGetPageShift();
#if defined(RGX_FEATURE_MIPS_BIT_MASK) && defined(SUPPORT_MIPS_CONTIGUOUS_FW_MEMORY)
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		uiLog2Align = RGXMIPSFW_LOG2_PAGE_SIZE_64K;
	}
#else
	PVR_UNREFERENCED_PARAMETER(uiLog2Align);
#endif

	uiMemAllocFlags = (uiMemAllocFlags |
					   PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC |
					   PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC) &
	                   RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp);

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(SUPPORT_SECURITY_VALIDATION)
	uiMemAllocFlags &= PVRSRV_MEMALLOCFLAGS_TDFWMASK;
#endif

	PDUMPCOMMENT(psDeviceNode, "Allocate FW %s memory", pszText);

	eError = DevmemFwAllocateExportable(psDeviceNode,
										ui32Size,
										1ULL << uiLog2Align,
										uiMemAllocFlags,
										pszText,
										ppsMemDescPtr);

	return eError;
}

/*!
 *******************************************************************************

 @Function	RGXDevInitCompatCheck_KMBuildOptions_FWAgainstDriver

 @Description

 Validate the FW build options against KM driver build options (KM build options only)

 Following check is redundant, because next check checks the same bits.
 Redundancy occurs because if client-server are build-compatible and client-firmware are
 build-compatible then server-firmware are build-compatible as well.

 This check is left for clarity in error messages if any incompatibility occurs.

 @Input psDevInfo - device info

 @Return   PVRSRV_ERROR - depending on mismatch found

 ******************************************************************************/
static PVRSRV_ERROR RGXDevInitCompatCheck_KMBuildOptions_FWAgainstDriver(PVRSRV_RGXDEV_INFO *psDevInfo)
{

	IMG_UINT32			ui32BuildOptions, ui32BuildOptionsFWKMPart, ui32BuildOptionsMismatch;
	RGX_FW_INFO_HEADER	*psFWInfoHeader = NULL;
	RGXFWIF_OSINIT		*psFwOsInit = NULL;
	IMG_UINT8               ui8FwOsCount;

	if (psDevInfo == NULL)
		return PVRSRV_ERROR_INVALID_PARAMS;

	ui32BuildOptions = (RGX_BUILD_OPTIONS_KM & RGX_BUILD_OPTIONS_MASK_FW);

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		psFwOsInit = psDevInfo->psRGXFWIfOsInit;
		if (psFwOsInit == NULL)
			return PVRSRV_ERROR_INVALID_PARAMS;

		ui32BuildOptionsFWKMPart = psFwOsInit->sRGXCompChecks.ui32BuildOptions & RGX_BUILD_OPTIONS_MASK_FW;

		ui8FwOsCount = psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.sInitOptions.ui8OsCountSupport;
		if (ui8FwOsCount != RGX_NUM_DRIVERS_SUPPORTED)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Mismatch between the number of Operating Systems supported by KM driver (%d) and FW (%d)",
					__func__, RGX_NUM_DRIVERS_SUPPORTED, ui8FwOsCount));
			return PVRSRV_ERROR_BUILD_OPTIONS_MISMATCH;
		}
	}
	else
	{
		psFWInfoHeader = &psDevInfo->sFWInfoHeader;
		ui32BuildOptionsFWKMPart = psFWInfoHeader->ui32Flags & RGX_BUILD_OPTIONS_MASK_FW;

		if (PVRSRV_VZ_MODE_IS(HOST, DEVINFO, psDevInfo) && BITMASK_HAS(psFWInfoHeader->ui32Flags, OPTIONS_NUM_DRIVERS_SUPPORTED_CHECK_EN))
		{
			ui8FwOsCount = (psFWInfoHeader->ui32Flags & OPTIONS_NUM_DRIVERS_SUPPORTED_MASK) >> OPTIONS_NUM_DRIVERS_SUPPORTED_SHIFT;
			ui8FwOsCount++;
			if (ui8FwOsCount != RGX_NUM_DRIVERS_SUPPORTED)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Mismatch between the number of Operating Systems supported by KM driver (%d) and FW (%d)",
					__func__, RGX_NUM_DRIVERS_SUPPORTED, ui8FwOsCount));
				return PVRSRV_ERROR_BUILD_OPTIONS_MISMATCH;
			}
		}
	}

	/* Check if the FW is missing support for any features required by the driver */
	if (~ui32BuildOptionsFWKMPart & ui32BuildOptions)
	{
		ui32BuildOptionsMismatch = ui32BuildOptions ^ ui32BuildOptionsFWKMPart;
#if !defined(PVRSRV_STRICT_COMPAT_CHECK)
		/*Mask non-critical options out as we do support combining them in UM & KM */
		ui32BuildOptionsMismatch &= FW_OPTIONS_STRICT;
#endif
		if ((ui32BuildOptions & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) RGXDevInitCompatCheck: Mismatch in Firmware and KM driver build options; "
					 "extra options present in the KM driver: (0x%x). Please check rgx_options.h",
					 ui32BuildOptions & ui32BuildOptionsMismatch));
			return PVRSRV_ERROR_BUILD_OPTIONS_MISMATCH;
		}

		if ((ui32BuildOptionsFWKMPart & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) RGXDevInitCompatCheck: Mismatch in Firmware-side and KM driver build options; "
					 "extra options present in Firmware: (0x%x). Please check rgx_options.h",
					 ui32BuildOptionsFWKMPart & ui32BuildOptionsMismatch ));
			return PVRSRV_ERROR_BUILD_OPTIONS_MISMATCH;
		}
		PVR_DPF((PVR_DBG_WARNING, "RGXDevInitCompatCheck: Firmware and KM driver build options differ."));
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "RGXDevInitCompatCheck: Firmware and KM driver build options match. [ OK ]"));
	}

	return PVRSRV_OK;
}

/*!
 *******************************************************************************

 @Function	RGXDevInitCompatCheck_DDKVersion_FWAgainstDriver

 @Description

 Validate FW DDK version against driver DDK version

 @Input psDevInfo - device info

 @Return   PVRSRV_ERROR - depending on mismatch found

 ******************************************************************************/
static PVRSRV_ERROR RGXDevInitCompatCheck_DDKVersion_FWAgainstDriver(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_UINT32			ui32KMDDKVersion;
	IMG_UINT32			ui32FWDDKVersion;
	PVRSRV_ERROR		eError;
	RGX_FW_INFO_HEADER	*psFWInfoHeader = NULL;
	RGXFWIF_OSINIT		*psFwOsInit = NULL;

	if (psDevInfo == NULL)
		return PVRSRV_ERROR_INVALID_PARAMS;

	ui32KMDDKVersion = PVRVERSION_PACK(PVRVERSION_MAJ, PVRVERSION_MIN);

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		psFwOsInit = psDevInfo->psRGXFWIfOsInit;
		if (psFwOsInit == NULL)
			return PVRSRV_ERROR_INVALID_PARAMS;

		ui32FWDDKVersion = psFwOsInit->sRGXCompChecks.ui32DDKVersion;
	}
	else
	{
		psFWInfoHeader = &psDevInfo->sFWInfoHeader;

		ui32FWDDKVersion = PVRVERSION_PACK(psFWInfoHeader->ui16PVRVersionMajor, psFWInfoHeader->ui16PVRVersionMinor);
	}

	if (ui32FWDDKVersion != ui32KMDDKVersion)
	{
		PVR_LOG(("(FAIL) RGXDevInitCompatCheck: Incompatible driver DDK version (%u.%u) / Firmware DDK version (%u.%u).",
				 PVRVERSION_MAJ, PVRVERSION_MIN,
				 PVRVERSION_UNPACK_MAJ(ui32FWDDKVersion),
				 PVRVERSION_UNPACK_MIN(ui32FWDDKVersion)));
		eError = PVRSRV_ERROR_DDK_VERSION_MISMATCH;
		PVR_DBG_BREAK;
		return eError;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "RGXDevInitCompatCheck: driver DDK version (%u.%u) and Firmware DDK version (%u.%u) match. [ OK ]",
				 PVRVERSION_MAJ, PVRVERSION_MIN,
				 PVRVERSION_MAJ, PVRVERSION_MIN));
	}

	return PVRSRV_OK;
}

/*!
 *******************************************************************************

 @Function	RGXDevInitCompatCheck_DDKBuild_FWAgainstDriver

 @Description

 Validate FW DDK build against driver DDK build

 @Input psDevInfo - device info

 @Return   PVRSRV_ERROR - depending on mismatch found

 ******************************************************************************/
static PVRSRV_ERROR RGXDevInitCompatCheck_DDKBuild_FWAgainstDriver(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	IMG_UINT32			ui32KMDDKBuild;
	IMG_UINT32			ui32FWDDKBuild;
	RGX_FW_INFO_HEADER	*psFWInfoHeader = NULL;
	RGXFWIF_OSINIT		*psFwOsInit = NULL;

	ui32KMDDKBuild = PVRVERSION_BUILD;

	if (psDevInfo == NULL)
		return PVRSRV_ERROR_INVALID_PARAMS;

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		psFwOsInit = psDevInfo->psRGXFWIfOsInit;
		if (psFwOsInit == NULL)
			return PVRSRV_ERROR_INVALID_PARAMS;

		ui32FWDDKBuild = psFwOsInit->sRGXCompChecks.ui32DDKBuild;
	}
	else
	{
		psFWInfoHeader = &psDevInfo->sFWInfoHeader;
		ui32FWDDKBuild = psFWInfoHeader->ui32PVRVersionBuild;
	}

	if (ui32FWDDKBuild != ui32KMDDKBuild)
	{
		PVR_LOG(("(WARN) RGXDevInitCompatCheck: Different driver DDK build version (%d) / Firmware DDK build version (%d).",
				 ui32KMDDKBuild, ui32FWDDKBuild));
#if defined(PVRSRV_STRICT_COMPAT_CHECK)
		eError = PVRSRV_ERROR_DDK_BUILD_MISMATCH;
		PVR_DBG_BREAK;
		return eError;
#endif
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "RGXDevInitCompatCheck: driver DDK build version (%d) and Firmware DDK build version (%d) match. [ OK ]",
				 ui32KMDDKBuild, ui32FWDDKBuild));
	}
	return eError;
}

/*!
 *******************************************************************************

 @Function	RGXDevInitCompatCheck_BVNC_FWAgainstDriver

 @Description

 Validate FW BVNC against driver BVNC

 @Input psDevInfo - device info

 @Return   PVRSRV_ERROR - depending on mismatch found

 ******************************************************************************/
static PVRSRV_ERROR RGXDevInitCompatCheck_BVNC_FWAgainstDriver(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;

	RGX_FW_INFO_HEADER	*psFWInfoHeader;
	IMG_UINT64			ui64KMBVNC;

	if (psDevInfo == NULL)
		return PVRSRV_ERROR_INVALID_PARAMS;

	psFWInfoHeader = &psDevInfo->sFWInfoHeader;

	ui64KMBVNC = rgx_bvnc_pack(psDevInfo->sDevFeatureCfg.ui32B,
							   psDevInfo->sDevFeatureCfg.ui32V,
							   psDevInfo->sDevFeatureCfg.ui32N,
							   psDevInfo->sDevFeatureCfg.ui32C);

	if (ui64KMBVNC != psFWInfoHeader->ui64BVNC)
	{
		PVR_LOG(("(FAIL) RGXDevInitCompatCheck: Mismatch in KM driver BVNC (%u.%u.%u.%u) and Firmware BVNC (%u.%u.%u.%u)",
				 RGX_BVNC_PACKED_EXTR_B(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_V(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_N(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_C(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_B(psFWInfoHeader->ui64BVNC),
				 RGX_BVNC_PACKED_EXTR_V(psFWInfoHeader->ui64BVNC),
				 RGX_BVNC_PACKED_EXTR_N(psFWInfoHeader->ui64BVNC),
				 RGX_BVNC_PACKED_EXTR_C(psFWInfoHeader->ui64BVNC)));

		eError = PVRSRV_ERROR_BVNC_MISMATCH;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "RGXDevInitCompatCheck: KM driver BVNC (%d.%d.%d.%d) and FW BVNC (%d.%d.%d.%d) match. [ OK ]",
				 RGX_BVNC_PACKED_EXTR_B(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_V(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_N(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_C(ui64KMBVNC),
				 RGX_BVNC_PACKED_EXTR_B(psFWInfoHeader->ui64BVNC),
				 RGX_BVNC_PACKED_EXTR_V(psFWInfoHeader->ui64BVNC),
				 RGX_BVNC_PACKED_EXTR_N(psFWInfoHeader->ui64BVNC),
				 RGX_BVNC_PACKED_EXTR_C(psFWInfoHeader->ui64BVNC)));

		eError = PVRSRV_OK;
	}

	return eError;
}

/*!
*******************************************************************************

 @Function	RGXDevInitCompatCheck

 @Description

 Check compatibility of host driver and firmware (DDK and build options)
 for RGX devices at services/device initialisation

 @Input psDeviceNode - device node

 @Return   PVRSRV_ERROR - depending on mismatch found

 ******************************************************************************/
static PVRSRV_ERROR RGXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR		eError;
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
#if !defined(NO_HARDWARE)
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		IMG_UINT32			ui32FwTimeout = MAX_HW_TIME_US;

		LOOP_UNTIL_TIMEOUT_US(ui32FwTimeout)
		{
			RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated,
		                               INVALIDATE);
			if (*((volatile IMG_BOOL *)&psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated))
			{
				/* No need to wait if the FW has already updated the values */
				break;
			}
			OSWaitus(ui32FwTimeout/WAIT_TRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();

		/* Flush covers this instance and the reads in the functions below */
		RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfOsInit->sRGXCompChecks,
	                               INVALIDATE);
		if (!*((volatile IMG_BOOL *)&psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated))
		{
			eError = PVRSRV_ERROR_TIMEOUT;
			PVR_DPF((PVR_DBG_ERROR, "%s: GPU Firmware not responding: failed to supply compatibility info (%u)",
					__func__, eError));

			PVR_DPF((PVR_DBG_ERROR, "%s: Potential causes: firmware not initialised or the current Guest driver's "
									"OsConfig initialisation data was not accepted by the firmware", __func__));
			goto chk_exit;
		}
	}
#endif /* defined(NO_HARDWARE) */

	eError = RGXDevInitCompatCheck_KMBuildOptions_FWAgainstDriver(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		goto chk_exit;
	}

	eError = RGXDevInitCompatCheck_DDKVersion_FWAgainstDriver(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		goto chk_exit;
	}

	eError = RGXDevInitCompatCheck_DDKBuild_FWAgainstDriver(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		goto chk_exit;
	}

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = RGXDevInitCompatCheck_BVNC_FWAgainstDriver(psDevInfo);
		if (eError != PVRSRV_OK)
		{
			goto chk_exit;
		}
	}

	eError = PVRSRV_OK;
chk_exit:

	return eError;
}

static void _RGXSoftResetToggle(PVRSRV_RGXDEV_INFO *psDevInfo,
                                IMG_UINT64  ui64ResetValue1,
                                IMG_UINT64  ui64ResetValue2)
{
	OSWriteHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_SOFT_RESET, ui64ResetValue1);

	/* Read soft-reset to fence previous write in order to clear the SOCIF pipeline */
	(void) OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_SOFT_RESET);
}

/**************************************************************************/ /*!
@Function       RGXSoftReset
@Description    Resets some modules of the RGX device
@Input          psDeviceNode		Device node
@Input          ui64ResetValue1 A mask for which each bit set corresponds
                                to a module to reset (via the SOFT_RESET
                                register).
@Input          ui64ResetValue2 A mask for which each bit set corresponds
                                to a module to reset (via the SOFT_RESET2
                                register).
@Return         PVRSRV_ERROR
*/ /***************************************************************************/
static PVRSRV_ERROR RGXSoftReset(PVRSRV_DEVICE_NODE *psDeviceNode,
                                 IMG_UINT64  ui64ResetValue1,
                                 IMG_UINT64  ui64ResetValue2)
{
	PVRSRV_RGXDEV_INFO        *psDevInfo;
	IMG_BOOL	bSoftReset = IMG_FALSE;
	IMG_UINT64	ui64SoftResetMask = 0;

	PVR_ASSERT(psDeviceNode != NULL);
	PVR_ASSERT(psDeviceNode->pvDevice != NULL);
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	psDevInfo = psDeviceNode->pvDevice;

#if defined(RGX_CR_SOFT_RESET__PBE2_XE__MASKFULL)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, PBE2_IN_XE))
	{
		ui64SoftResetMask = RGX_CR_SOFT_RESET__PBE2_XE__MASKFULL;
	}
	else
#endif
	{
		ui64SoftResetMask = RGX_CR_SOFT_RESET_MASKFULL;
	}

	if (((ui64ResetValue1 & ui64SoftResetMask) != ui64ResetValue1) || bSoftReset)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Set in soft-reset */
	_RGXSoftResetToggle(psDevInfo, ui64ResetValue1, ui64ResetValue2);

	/* Take the modules out of reset... */
	_RGXSoftResetToggle(psDevInfo, 0, 0);

	return PVRSRV_OK;
}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
static const RGX_MIPS_ADDRESS_TRAMPOLINE sNullTrampoline;

static void RGXFreeTrampoline(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	DevPhysMemFree(psDeviceNode,
#if defined(PDUMP)
			psDevInfo->psTrampoline->hPdumpPages,
#endif
			&psDevInfo->psTrampoline->sPages);

	if (psDevInfo->psTrampoline != &sNullTrampoline)
	{
		OSFreeMem(psDevInfo->psTrampoline);
	}
	psDevInfo->psTrampoline = (RGX_MIPS_ADDRESS_TRAMPOLINE *)&sNullTrampoline;
}

#define RANGES_OVERLAP(x,y,size) (x < (y+size) && y < (x+size))
#define TRAMPOLINE_ALLOC_MAX_RETRIES (3)

static PVRSRV_ERROR RGXAllocTrampoline(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;
	IMG_INT32 i, j;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	RGX_MIPS_ADDRESS_TRAMPOLINE *pasTrampoline[TRAMPOLINE_ALLOC_MAX_RETRIES];

	PDUMPCOMMENT(psDeviceNode, "Allocate pages for trampoline");

	/* Retry the allocation of the trampoline block (16KB), retaining any
	 * previous allocations overlapping  with the target range until we get an
	 * allocation that doesn't overlap with the target range.
	 * Any allocation like this will require a maximum of 3 tries as we are
	 * allocating a physical contiguous block of memory, not individual pages.
	 * Free the unused allocations at the end only after the desired range
	 * is obtained to prevent the alloc function from returning the same bad
	 * range repeatedly.
	 */
	for (i = 0; i < TRAMPOLINE_ALLOC_MAX_RETRIES; i++)
	{
		pasTrampoline[i] = OSAllocMem(sizeof(RGX_MIPS_ADDRESS_TRAMPOLINE));
		eError = DevPhysMemAlloc(psDeviceNode,
				RGXMIPSFW_TRAMPOLINE_SIZE,
				RGXMIPSFW_TRAMPOLINE_LOG2_SEGMENT_SIZE,
				0,         // (init) u8Value
				IMG_FALSE, // bInitPage,
#if defined(PDUMP)
				psDeviceNode->psFirmwareMMUDevAttrs->pszMMUPxPDumpMemSpaceName,
				"TrampolineRegion",
				&pasTrampoline[i]->hPdumpPages,
#endif
				PVR_SYS_ALLOC_PID,
				&pasTrampoline[i]->sPages,
				&pasTrampoline[i]->sPhysAddr);
		if (PVRSRV_OK != eError)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s failed (%u)",
			         __func__,
			         eError));
			goto fail;
		}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
		/* Set the persistent uiOSid value so that we free from the correct
		 * base arena when unloading the driver and freeing the trampoline.
		 */
		pasTrampoline[i]->sPages.uiOSid = 0;	/* Firmware global arena */
#endif

		if (!RANGES_OVERLAP(pasTrampoline[i]->sPhysAddr.uiAddr,
				RGXMIPSFW_TRAMPOLINE_TARGET_PHYS_ADDR,
				RGXMIPSFW_TRAMPOLINE_SIZE))
		{
			break;
		}
	}
	if (TRAMPOLINE_ALLOC_MAX_RETRIES == i)
	{
		/* Failed to find a physical allocation after 3 attempts */
		eError = PVRSRV_ERROR_FAILED_TO_ALLOC_PAGES;
		PVR_DPF((PVR_DBG_ERROR,
				"%s failed to allocate non-overlapping pages (%u)",
				__func__, eError));
		/* Fall through, clean up and return error. */
	}
	else
	{
		/* Remember the last physical block allocated, it will not be freed */
		psDevInfo->psTrampoline = pasTrampoline[i];
	}

fail:
	/* free all unused allocations */
	for (j = 0; j < i; j++)
	{
		DevPhysMemFree(psDeviceNode,
#if defined(PDUMP)
				pasTrampoline[j]->hPdumpPages,
#endif
				&pasTrampoline[j]->sPages);
		OSFreeMem(pasTrampoline[j]);
	}

	return eError;
}

#undef RANGES_OVERLAP
#endif

PVRSRV_ERROR RGXInitAllocFWImgMem(PVRSRV_DEVICE_NODE   *psDeviceNode,
                                  IMG_DEVMEM_SIZE_T    uiFWCodeLen,
                                  IMG_DEVMEM_SIZE_T    uiFWDataLen,
                                  IMG_DEVMEM_SIZE_T    uiFWCorememCodeLen,
                                  IMG_DEVMEM_SIZE_T    uiFWCorememDataLen)
{
	PVRSRV_MEMALLOCFLAGS_T uiMemAllocFlags;
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR        eError;
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	IMG_DEVMEM_SIZE_T	uiDummyLen;
	DEVMEM_MEMDESC		*psDummyMemDesc = NULL;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) &&
		(RGX_GET_FEATURE_VALUE(psDevInfo, PHYS_BUS_WIDTH) == 32))
	{
		eError = RGXAllocTrampoline(psDeviceNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"Failed to allocate trampoline region (%u)",
					eError));
			goto failTrampolineMemDescAlloc;
		}
	}
#endif

	/*
	 * Set up Allocation for FW code section
	 */
	uiMemAllocFlags = RGX_FWCODEDATA_ALLOCFLAGS |
	                  PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_CODE);
	eError = RGXAllocateFWMemoryRegion(psDeviceNode,
	                                   uiFWCodeLen,
	                                   uiMemAllocFlags,
	                                   "FwExCodeRegion",
	                                   &psDevInfo->psRGXFWCodeMemDesc);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "Failed to allocate fw code mem (%u)",
		         eError));
		goto failFWCodeMemDescAlloc;
	}

	eError = DevmemAcquireDevVirtAddr(psDevInfo->psRGXFWCodeMemDesc,
	                                  &psDevInfo->sFWCodeDevVAddrBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "Failed to acquire devVAddr for fw code mem (%u)",
		         eError));
		goto failFWCodeMemDescAqDevVirt;
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (!(RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) || (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))))
#endif
	{
		/*
		 * The FW code must be the first allocation in the firmware heap, otherwise
		 * the bootloader will not work (the FW will not be able to find the bootloader).
		 */
		PVR_ASSERT(psDevInfo->sFWCodeDevVAddrBase.uiAddr == RGX_FIRMWARE_RAW_HEAP_BASE);
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		/*
		 * Allocate Dummy Pages so that Data segment allocation gets the same
		 * device virtual address as specified in MIPS firmware linker script
		 */
		uiDummyLen = RGXGetFWImageSectionMaxSize(NULL, MIPS_CODE) +
				RGXGetFWImageSectionMaxSize(NULL, MIPS_EXCEPTIONS_CODE) +
				RGXGetFWImageSectionMaxSize(NULL, MIPS_BOOT_CODE) -
				uiFWCodeLen; /* code actual size */

		if (uiDummyLen > 0)
		{
			eError = DevmemFwAllocateExportable(psDeviceNode,
					uiDummyLen,
					OSGetPageSize(),
					uiMemAllocFlags,
					"FwExDummyPages",
					&psDummyMemDesc);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,
				         "Failed to allocate fw dummy mem (%u)",
				         eError));
				goto failDummyMemDescAlloc;
			}
		}
	}
#endif

	/*
	 * Set up Allocation for FW data section
	 */
	eError = RGXAllocateFWMemoryRegion(psDeviceNode,
	                                   uiFWDataLen,
	                                   RGX_FWCODEDATA_ALLOCFLAGS |
	                                   PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_PRIV_DATA),
	                                   "FwExDataRegion",
	                                   &psDevInfo->psRGXFWDataMemDesc);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "Failed to allocate fw data mem (%u)",
		         eError));
		goto failFWDataMemDescAlloc;
	}

	eError = DevmemAcquireDevVirtAddr(psDevInfo->psRGXFWDataMemDesc,
	                                  &psDevInfo->sFWDataDevVAddrBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "Failed to acquire devVAddr for fw data mem (%u)",
		         eError));
		goto failFWDataMemDescAqDevVirt;
	}

	if (uiFWCorememCodeLen != 0)
	{
		/*
		 * Set up Allocation for FW coremem code section
		 */
		uiMemAllocFlags = (RGX_FWCODEDATA_ALLOCFLAGS |
		                   PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_CODE)) &
		                   ~PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE;
		eError = RGXAllocateFWMemoryRegion(psDeviceNode,
		                                   uiFWCorememCodeLen,
		                                   uiMemAllocFlags,
		                                   "FwExCorememCodeRegion",
		                                   &psDevInfo->psRGXFWCorememCodeMemDesc);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "Failed to allocate fw coremem code mem, size: %"  IMG_INT64_FMTSPECd ", flags: %" PVRSRV_MEMALLOCFLAGS_FMTSPEC " (%u)",
			         uiFWCorememCodeLen, uiMemAllocFlags, eError));
			goto failFWCorememCodeMemDescAlloc;
		}

		eError = DevmemAcquireDevVirtAddr(psDevInfo->psRGXFWCorememCodeMemDesc,
		                                  &psDevInfo->sFWCorememCodeDevVAddrBase);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "Failed to acquire devVAddr for fw coremem mem code (%u)",
			         eError));
			goto failFWCorememCodeMemDescAqDevVirt;
		}

		eError = RGXSetFirmwareAddress(&psDevInfo->sFWCorememCodeFWAddr,
		                      psDevInfo->psRGXFWCorememCodeMemDesc,
		                      0, RFW_FWADDR_NOREF_FLAG);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:1", failFWCorememCodeMemDescFwAddr);
	}
	else
	{
		psDevInfo->sFWCorememCodeDevVAddrBase.uiAddr = 0;
		psDevInfo->sFWCorememCodeFWAddr.ui32Addr = 0;
	}

	if (uiFWCorememDataLen != 0)
	{
		/*
		 * Set up Allocation for FW coremem data section
		 */
		uiMemAllocFlags = RGX_FWCODEDATA_ALLOCFLAGS |
		                  PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_PRIV_DATA);
		eError = RGXAllocateFWMemoryRegion(psDeviceNode,
				uiFWCorememDataLen,
				uiMemAllocFlags,
				"FwExCorememDataRegion",
				&psDevInfo->psRGXFWIfCorememDataStoreMemDesc);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "Failed to allocate fw coremem data mem, "
			         "size: %"  IMG_INT64_FMTSPECd ", flags: %" PVRSRV_MEMALLOCFLAGS_FMTSPEC " (%u)",
			         uiFWCorememDataLen,
			         uiMemAllocFlags,
			         eError));
			goto failFWCorememDataMemDescAlloc;
		}

		eError = DevmemAcquireDevVirtAddr(psDevInfo->psRGXFWIfCorememDataStoreMemDesc,
				&psDevInfo->sFWCorememDataStoreDevVAddrBase);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "Failed to acquire devVAddr for fw coremem mem data (%u)",
			         eError));
			goto failFWCorememDataMemDescAqDevVirt;
		}

		eError = RGXSetFirmwareAddress(&psDevInfo->sFWCorememDataStoreFWAddr,
				psDevInfo->psRGXFWIfCorememDataStoreMemDesc,
				0, RFW_FWADDR_NOREF_FLAG);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:2", failFWCorememDataMemDescFwAddr);
	}
	else
	{
		psDevInfo->sFWCorememDataStoreDevVAddrBase.uiAddr = 0;
		psDevInfo->sFWCorememDataStoreFWAddr.ui32Addr = 0;
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	/* Free Dummy Pages */
	if (psDummyMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDummyMemDesc);
	}
#endif

	return PVRSRV_OK;

failFWCorememDataMemDescFwAddr:
failFWCorememDataMemDescAqDevVirt:
	if (uiFWCorememDataLen != 0)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfCorememDataStoreMemDesc);
		psDevInfo->psRGXFWIfCorememDataStoreMemDesc = NULL;
	}
failFWCorememDataMemDescAlloc:
failFWCorememCodeMemDescFwAddr:
failFWCorememCodeMemDescAqDevVirt:
	if (uiFWCorememCodeLen != 0)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWCorememCodeMemDesc);
		psDevInfo->psRGXFWCorememCodeMemDesc = NULL;
	}
failFWCorememCodeMemDescAlloc:
failFWDataMemDescAqDevVirt:
	DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWDataMemDesc);
	psDevInfo->psRGXFWDataMemDesc = NULL;
failFWDataMemDescAlloc:
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (psDummyMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDummyMemDesc);
	}
failDummyMemDescAlloc:
#endif
failFWCodeMemDescAqDevVirt:
	DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWCodeMemDesc);
	psDevInfo->psRGXFWCodeMemDesc = NULL;
failFWCodeMemDescAlloc:
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) &&
		(RGX_GET_FEATURE_VALUE(psDevInfo, PHYS_BUS_WIDTH) == 32))
	{
		RGXFreeTrampoline(psDeviceNode);
	}
failTrampolineMemDescAlloc:
#endif
	return eError;
}

/*
	AppHint parameter interface
 */
static
PVRSRV_ERROR RGXFWTraceQueryFilter(const PVRSRV_DEVICE_NODE *psDeviceNode,
                                   const void *psPrivate,
                                   IMG_UINT32 *pui32Value)
{
	PVRSRV_ERROR eResult;

	eResult = PVRSRVRGXFWDebugQueryFWLogKM(NULL, psDeviceNode, pui32Value);
	*pui32Value &= RGXFWIF_LOG_TYPE_GROUP_MASK;
	return eResult;
}

static
PVRSRV_ERROR RGXFWTraceQueryLogType(const PVRSRV_DEVICE_NODE *psDeviceNode,
                                   const void *psPrivate,
                                   IMG_UINT32 *pui32Value)
{
	PVRSRV_ERROR eResult;

	eResult = PVRSRVRGXFWDebugQueryFWLogKM(NULL, psDeviceNode, pui32Value);
	if (PVRSRV_OK == eResult)
	{
		if (*pui32Value & RGXFWIF_LOG_TYPE_TRACE)
		{
			*pui32Value = 0; /* Trace */
		}
		else
		{
			*pui32Value = 1; /* TBI */
		}
	}
	return eResult;
}

static
PVRSRV_ERROR RGXFWTraceSetFilter(const PVRSRV_DEVICE_NODE *psDeviceNode,
                                 const void *psPrivate,
                                 IMG_UINT32 ui32Value)
{
	PVRSRV_ERROR eResult;
	IMG_UINT32 ui32RGXFWLogType;

	eResult = RGXFWTraceQueryLogType(psDeviceNode, NULL, &ui32RGXFWLogType);
	if (PVRSRV_OK == eResult)
	{
		if (0 == ui32RGXFWLogType)
		{
			BITMASK_SET(ui32Value, RGXFWIF_LOG_TYPE_TRACE);
		}
		eResult = PVRSRVRGXFWDebugSetFWLogKM(NULL, psDeviceNode, ui32Value);
	}
	return eResult;
}

static
PVRSRV_ERROR RGXFWTraceSetLogType(const PVRSRV_DEVICE_NODE *psDeviceNode,
                                  const void *psPrivate,
                                  IMG_UINT32 ui32Value)
{
	PVRSRV_ERROR eResult;
	IMG_UINT32 ui32RGXFWLogType = ui32Value;

	eResult = RGXFWTraceQueryFilter(psDeviceNode, NULL, &ui32RGXFWLogType);
	if (PVRSRV_OK != eResult)
	{
		return eResult;
	}

	/* 0 - trace, 1 - tbi */
	if (0 == ui32Value)
	{
		BITMASK_SET(ui32RGXFWLogType, RGXFWIF_LOG_TYPE_TRACE);
	}
#if defined(SUPPORT_TBI_INTERFACE)
	else if (1 == ui32Value)
	{
		BITMASK_UNSET(ui32RGXFWLogType, RGXFWIF_LOG_TYPE_TRACE);
	}
#endif
	else
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Invalid parameter %u specified to set FW log type AppHint.",
		         __func__, ui32Value));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eResult = PVRSRVRGXFWDebugSetFWLogKM(NULL, psDeviceNode, ui32RGXFWLogType);
	return eResult;
}

#if defined(DEBUG)
static
PVRSRV_ERROR RGXQueryFWPoisonOnFree(const PVRSRV_DEVICE_NODE *psDeviceNode,
									const void *psPrivate,
									IMG_BOOL *pbValue)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *) psDeviceNode->pvDevice;

	*pbValue = (PVRSRV_MEMALLOCFLAG_POISON_ON_FREE == psDevInfo->uiFWPoisonOnFreeFlag)
		? IMG_TRUE
		: IMG_FALSE;
	return PVRSRV_OK;
}

static
PVRSRV_ERROR RGXSetFWPoisonOnFree(const PVRSRV_DEVICE_NODE *psDeviceNode,
									const void *psPrivate,
									IMG_BOOL bValue)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *) psDeviceNode->pvDevice;
	psDevInfo->uiFWPoisonOnFreeFlag = bValue
			? PVRSRV_MEMALLOCFLAG_POISON_ON_FREE
			: 0ULL;

	return PVRSRV_OK;
}
#endif

/*
 * RGXInitFirmware
 */
PVRSRV_ERROR
RGXInitFirmware(PVRSRV_DEVICE_NODE       *psDeviceNode,
                RGX_INIT_APPHINTS        *psApphints,
                IMG_UINT32               ui32ConfigFlags,
                IMG_UINT32               ui32ConfigFlagsExt,
                IMG_UINT32               ui32FwOsCfgFlags)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;
#if defined(DEBUG)
	void *pvAppHintState = NULL;
	IMG_BOOL bAppHintDefault;
	IMG_BOOL bEnableFWPoisonOnFree = IMG_FALSE;
#endif

	eError = RGXSetupFirmware(psDeviceNode,
	                          psApphints,
	                          ui32ConfigFlags,
	                          ui32ConfigFlagsExt,
	                          ui32FwOsCfgFlags);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "PVRSRVRGXInitFirmwareKM: RGXSetupFirmware failed (%u)",
		         eError));
		goto failed_init_firmware;
	}

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PVRSRVAppHintRegisterHandlersUINT32(APPHINT_ID_EnableLogGroup,
		                                    RGXFWTraceQueryFilter,
		                                    RGXFWTraceSetFilter,
		                                    psDeviceNode,
		                                    NULL);
		PVRSRVAppHintRegisterHandlersUINT32(APPHINT_ID_FirmwareLogType,
		                                    RGXFWTraceQueryLogType,
		                                    RGXFWTraceSetLogType,
		                                    psDeviceNode,
		                                    NULL);
	}

#if defined(DEBUG)
	OSCreateAppHintState(&pvAppHintState);

	bAppHintDefault = PVRSRV_APPHINT_ENABLEFWPOISONONFREE;
	OSGetAppHintBOOL(psDeviceNode,
			pvAppHintState,
			EnableFWPoisonOnFree,
			&bAppHintDefault,
			&bEnableFWPoisonOnFree);

	OSFreeAppHintState(pvAppHintState);

	PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_EnableFWPoisonOnFree,
	                                   RGXQueryFWPoisonOnFree,
	                                   RGXSetFWPoisonOnFree,
	                                   psDeviceNode,
	                                   NULL);

	psDevInfo->uiFWPoisonOnFreeFlag = bEnableFWPoisonOnFree
			? PVRSRV_MEMALLOCFLAG_POISON_ON_FREE
			: 0ULL;
#else
	psDevInfo->uiFWPoisonOnFreeFlag = 0ULL;
#endif

	psDevInfo->ui32ClockSource = PVRSRV_APPHINT_TIMECORRCLOCK;
	psDevInfo->ui32LastClockSource = PVRSRV_APPHINT_TIMECORRCLOCK;

	return PVRSRV_OK;

failed_init_firmware:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

/* See device.h for function declaration */
static PVRSRV_ERROR RGXAllocUFOBlock(PVRSRV_DEVICE_NODE *psDeviceNode,
									 IMG_UINT32 ui32RequestedSize,
									 DEVMEM_MEMDESC **psMemDesc,
									 IMG_UINT32 *puiSyncPrimVAddr,
									 IMG_UINT32 *puiSyncPrimBlockSize)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	PVRSRV_ERROR eError;
	RGXFWIF_DEV_VIRTADDR pFirmwareAddr;
	IMG_DEVMEM_ALIGN_T uiUFOBlockAlign = MAX(sizeof(IMG_UINT32), sizeof(SYNC_CHECKPOINT_FW_OBJ));
	IMG_DEVMEM_SIZE_T uiUFOBlockSize = PVR_ALIGN(ui32RequestedSize, uiUFOBlockAlign);

	psDevInfo = psDeviceNode->pvDevice;

	/* Size and align are 'expanded' because we request an Exportalign allocation */
	eError = DevmemExportalignAdjustSizeAndAlign(DevmemGetHeapLog2PageSize(psDevInfo->psFirmwareMainHeap),
	                                             &uiUFOBlockSize,
	                                             &uiUFOBlockAlign);

	if (eError != PVRSRV_OK)
	{
		goto e0;
	}

	eError = DevmemFwAllocateExportable(psDeviceNode,
										uiUFOBlockSize,
										uiUFOBlockAlign,
										PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN) |
										PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
										PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
										PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC |
										PVRSRV_MEMALLOCFLAG_GPU_READABLE |
										PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
										PVRSRV_MEMALLOCFLAG_CPU_READABLE |
										PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
										PVRSRV_MEMALLOCFLAG_UNCACHED,
										"FwExUFOBlock",
										psMemDesc);
	if (eError != PVRSRV_OK)
	{
		goto e0;
	}

	eError = RGXSetFirmwareAddress(&pFirmwareAddr, *psMemDesc, 0, RFW_FWADDR_FLAG_NONE);
	PVR_GOTO_IF_ERROR(eError, e1);

	*puiSyncPrimVAddr = pFirmwareAddr.ui32Addr;
	*puiSyncPrimBlockSize = TRUNCATE_64BITS_TO_32BITS(uiUFOBlockSize);

	return PVRSRV_OK;

e1:
	DevmemFwUnmapAndFree(psDevInfo, *psMemDesc);
e0:
	return eError;
}

/* See device.h for function declaration */
static void RGXFreeUFOBlock(PVRSRV_DEVICE_NODE *psDeviceNode,
							DEVMEM_MEMDESC *psMemDesc)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	RGXUnsetFirmwareAddress(psMemDesc);
	DevmemFwUnmapAndFree(psDevInfo, psMemDesc);
}

static void DevPart2DeInitRGX(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;

	psDevInfo->bDevInit2Done = IMG_FALSE;

#if defined(SUPPORT_SECURE_ALLOC_KM)
	if (psDevInfo->psGenHeapSecMem != NULL)
	{
		OSFreeSecBuf(psDevInfo->psGenHeapSecMem);
	}
#endif

#if defined(RGX_FEATURE_COMPUTE_ONLY_BIT_MASK)
	if (!RGX_IS_FEATURE_SUPPORTED(psDevInfo, COMPUTE_ONLY))
#endif
	{
		if ((psDevInfo->hTQUSCSharedMem != NULL) &&
		    (psDevInfo->hTQCLISharedMem != NULL))
		{
			PVRSRVTQUnloadShaders(psDeviceNode);
		}
	}

#if !defined(NO_HARDWARE)
	if (psDevInfo->pvLISRData != NULL)
	{
		(void) SysUninstallDeviceLISR(psDevInfo->pvLISRData);
	}
	if (psDevInfo->pvMISRData != NULL)
	{
		(void) OSUninstallMISR(psDevInfo->pvMISRData);
	}
	if (psDevInfo->hProcessQueuesMISR != NULL)
	{
		(void) OSUninstallMISR(psDevInfo->hProcessQueuesMISR);
	}
	if (psDevInfo->pvAPMISRData != NULL)
	{
		(void) OSUninstallMISR(psDevInfo->pvAPMISRData);
	}
	if (psDeviceNode->hCmdCompNotify != NULL)
	{
		/* Cancel notifications to this device */
		PVRSRVUnregisterCmdCompleteNotify(psDeviceNode->hCmdCompNotify);
		psDeviceNode->hCmdCompNotify = NULL;
	}
#endif /* !NO_HARDWARE */

	/* Remove the device from the power manager */
	PVRSRVRemovePowerDevice(psDeviceNode);

	psDevInfo->pfnGetGpuUtilStats = NULL;
	if (psDevInfo->hGPUUtilLock != NULL)
	{
		OSLockDestroy(psDevInfo->hGPUUtilLock);
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) &&
		(psDevInfo->hNMILock != NULL))
	{
		OSLockDestroy(psDevInfo->hNMILock);
	}
#endif

	if ((GetInfoPageDebugFlagsKM() & DEBUG_FEATURE_PAGE_FAULT_DEBUG_ENABLED) &&
		(psDevInfo->hMMUCtxUnregLock != NULL))
	{
		OSLockDestroy(psDevInfo->hMMUCtxUnregLock);
	}

	if (psDevInfo->hDebugFaultInfoLock != NULL)
	{
		OSLockDestroy(psDevInfo->hDebugFaultInfoLock);
	}

	/* De-init Freelists/ZBuffers... */
	if (psDevInfo->hLockFreeList != NULL)
	{
		OSLockDestroy(psDevInfo->hLockFreeList);
	}

	if (psDevInfo->hLockZSBuffer != NULL)
	{
		OSLockDestroy(psDevInfo->hLockZSBuffer);
	}

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* De-init work estimation lock */
		if (psDevInfo->hWorkEstLock != NULL)
		{
			OSLockDestroy(psDevInfo->hWorkEstLock);
		}
	}
#endif

	/* Free DVFS Table */
	if (psDevInfo->psGpuDVFSTable != NULL)
	{
		OSFreeMem(psDevInfo->psGpuDVFSTable);
		psDevInfo->psGpuDVFSTable = NULL;
	}
}

/*
	DevDeInitRGX
 */
PVRSRV_ERROR DevDeInitRGX(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO		*psDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;
	PVRSRV_ERROR			eError;
	DEVICE_MEMORY_INFO		*psDevMemoryInfo;

	if (!psDevInfo)
	{
		/* Can happen if DevInitRGX failed */
		PVR_DPF((PVR_DBG_ERROR, "DevDeInitRGX: Null DevInfo"));
		return PVRSRV_OK;
	}

	if (psDevInfo->psRGXFWIfOsInit)
	{
		KM_SET_OS_CONNECTION(OFFLINE, psDevInfo);
		KM_CONNECTION_CACHEOP(Os, FLUSH);
	}

	RGXUnregisterBridges(psDevInfo);

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
	OSLockDestroy(psDevInfo->hCounterDumpingLock);
#endif

	/* Unregister debug request notifiers first as they could depend on anything. */

	RGXDebugDeinit(psDevInfo);

	/* De-initialise in reverse order, so stage 2 init is undone first. */
	if (psDevInfo->bDevInit2Done)
	{
		DevPart2DeInitRGX(psDeviceNode);
	}

	/* Unregister MMU related stuff */
	eError = RGXMMUInit_Unregister(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "DevDeInitRGX: Failed RGXMMUInit_Unregister (0x%x)",
		         eError));
	}

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		/* Unregister MMU related stuff */
		eError = RGXMipsMMUInit_Unregister(psDeviceNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "DevDeInitRGX: Failed RGXMipsMMUInit_Unregister (0x%x)",
			         eError));
		}
	}
#endif

	/* UnMap Regs */
	if (psDevInfo->pvRegsBaseKM != NULL)
	{
#if !defined(NO_HARDWARE)
		OSUnMapPhysToLin((void __force *) psDevInfo->pvRegsBaseKM,
						 psDevInfo->ui32RegSize);
#endif /* !NO_HARDWARE */
		psDevInfo->pvRegsBaseKM = NULL;
	}

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (psDevInfo->pvSecureRegsBaseKM != NULL)
	{
#if !defined(NO_HARDWARE)
		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, HOST_SECURITY_VERSION) &&
			(RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1))
		{
			/* undo the VA offset performed in RGXRegisterDevice() to allow the allocation to be unmapped */
			psDevInfo->pvSecureRegsBaseKM = (void __iomem *)((uintptr_t)psDevInfo->pvSecureRegsBaseKM + RGX_HOST_SECURE_REGBANK_OFFSET);
			OSUnMapPhysToLin((void __force *) psDevInfo->pvSecureRegsBaseKM, RGX_HOST_SECURE_REGBANK_SIZE);
		}
#endif /* !NO_HARDWARE */
		psDevInfo->pvSecureRegsBaseKM = NULL;
	}
#endif

#if 0 /* not required at this time */
	if (psDevInfo->hTimer)
	{
		eError = OSRemoveTimer(psDevInfo->hTimer);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "DevDeInitRGX: Failed to remove timer"));
			return eError;
		}
		psDevInfo->hTimer = NULL;
	}
#endif

	psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;

	RGXDeInitHeaps(psDevMemoryInfo, psDeviceNode);

	if (psDevInfo->psRGXFWCodeMemDesc)
	{
		/* Free fw code */
		PDUMPCOMMENT(psDeviceNode, "Freeing FW code memory");
		DevmemReleaseDevVirtAddr(psDevInfo->psRGXFWCodeMemDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWCodeMemDesc);
		psDevInfo->psRGXFWCodeMemDesc = NULL;
	}
#if !defined(NO_HARDWARE)
	else if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PVR_DPF((PVR_DBG_WARNING, "No firmware code memory to free"));
	}
#endif	/* !defined(NO_HARDWARE) */

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) &&
		(RGX_GET_FEATURE_VALUE(psDevInfo, PHYS_BUS_WIDTH) == 32))
	{
		if (psDevInfo->psTrampoline->sPages.u.pvHandle)
		{
			/* Free trampoline region */
			PDUMPCOMMENT(psDeviceNode, "Freeing trampoline memory");
			RGXFreeTrampoline(psDeviceNode);
		}
	}
#endif

	if (psDevInfo->psRGXFWDataMemDesc)
	{
		/* Free fw data */
		PDUMPCOMMENT(psDeviceNode, "Freeing FW data memory");
		DevmemReleaseDevVirtAddr(psDevInfo->psRGXFWDataMemDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWDataMemDesc);
		psDevInfo->psRGXFWDataMemDesc = NULL;
	}
#if !defined(NO_HARDWARE)
	else if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PVR_DPF((PVR_DBG_WARNING, "No firmware data memory to free"));
	}
#endif	/* !defined(NO_HARDWARE) */

	if (psDevInfo->psRGXFWCorememCodeMemDesc)
	{
		/* Free fw core mem code */
		PDUMPCOMMENT(psDeviceNode, "Freeing FW coremem code memory");
		DevmemReleaseDevVirtAddr(psDevInfo->psRGXFWCorememCodeMemDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWCorememCodeMemDesc);
		psDevInfo->psRGXFWCorememCodeMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWIfCorememDataStoreMemDesc)
	{
		/* Free fw core mem data */
		PDUMPCOMMENT(psDeviceNode, "Freeing FW coremem data store memory");
		DevmemReleaseDevVirtAddr(psDevInfo->psRGXFWIfCorememDataStoreMemDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfCorememDataStoreMemDesc);
		psDevInfo->psRGXFWIfCorememDataStoreMemDesc = NULL;
	}

	/*
	   Free the firmware allocations.
	 */
	RGXFreeFirmware(psDevInfo);


	RGXDeInitMultiCoreInfo(psDeviceNode);

	/* De-initialise non-device specific (TL) users of RGX device memory */
	{
		IMG_UINT32 i;
		for (i = 0; i < RGX_HWPERF_L2_STREAM_LAST; i++)
		{
			RGXHWPerfDeinitL2Stream(psDevInfo, i);
		}

		RGXHWPerfDeinit(psDevInfo);
	}

	RGXDeInitDestroyFWKernelMemoryContext(psDeviceNode);

	RGXHWPerfHostDeInit(psDevInfo);
	eError = HTBDeInit();
	PVR_LOG_IF_ERROR(eError, "HTBDeInit");

	OSLockDestroy(psDevInfo->hGpuUtilStatsLock);

	/* destroy the stalled CCB locks */
	OSLockDestroy(psDevInfo->hCCBRecoveryLock);
	OSLockDestroy(psDevInfo->hCCBStallCheckLock);

	/* destroy the context list locks */
	OSLockDestroy(psDevInfo->sRegConfig.hLock);
	OSLockDestroy(psDevInfo->hBPLock);
	OSLockDestroy(psDevInfo->hRGXFWIfBufInitLock);
	OSWRLockDestroy(psDevInfo->hRenderCtxListLock);
	OSWRLockDestroy(psDevInfo->hComputeCtxListLock);
	OSWRLockDestroy(psDevInfo->hTransferCtxListLock);
	OSWRLockDestroy(psDevInfo->hTDMCtxListLock);
	OSWRLockDestroy(psDevInfo->hKickSyncCtxListLock);
	OSWRLockDestroy(psDevInfo->hMemoryCtxListLock);
	OSSpinLockDestroy(psDevInfo->hLockKCCBDeferredCommandsList);
	OSWRLockDestroy(psDevInfo->hCommonCtxtListLock);

	/* Free device BVNC string */
	if (NULL != psDevInfo->sDevFeatureCfg.pszBVNCString)
	{
		OSFreeMem(psDevInfo->sDevFeatureCfg.pszBVNCString);
	}


	/* DeAllocate devinfo */
	OSFreeMem(psDevInfo);

	psDeviceNode->pvDevice = NULL;

	return PVRSRV_OK;
}

#if defined(PDUMP)
static
PVRSRV_ERROR RGXResetPDump(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)(psDeviceNode->pvDevice);

	psDevInfo->bDumpedKCCBCtlAlready = IMG_FALSE;

	return PVRSRV_OK;
}
#endif /* PDUMP */

/* Takes a log2 page size parameter and calculates a suitable page size
 * for the RGX heaps. Returns 0 if parameter is wrong.*/
IMG_UINT32 RGXHeapDerivePageSize(IMG_UINT32 uiLog2PageSize)
{
	IMG_BOOL bFound = IMG_FALSE;
	IMG_UINT32 ui32PageSizeMask = RGXGetValidHeapPageSizeMask();

	/* OS page shift must be at least RGX_HEAP_4KB_PAGE_SHIFT,
	 * max RGX_HEAP_2MB_PAGE_SHIFT, non-zero and a power of two*/
	if (uiLog2PageSize == 0U ||
	    (uiLog2PageSize < RGX_HEAP_4KB_PAGE_SHIFT) ||
	    (uiLog2PageSize > RGX_HEAP_2MB_PAGE_SHIFT))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Provided incompatible log2 page size %u",
				__func__,
				uiLog2PageSize));
		PVR_ASSERT(0);
		return 0;
	}

	do
	{
		if ((IMG_PAGE2BYTES32(uiLog2PageSize) & ui32PageSizeMask) == 0)
		{
			/* We have to fall back to a smaller device
			 * page size than given page size because there
			 * is no exact match for any supported size. */
			uiLog2PageSize -= 1U;
		}
		else
		{
			/* All good, RGX page size equals given page size
			 * => use it as default for heaps */
			bFound = IMG_TRUE;
		}
	} while (!bFound);

	return uiLog2PageSize;
}

/* First 16-bits define possible types */
#define HEAP_INST_VALUE_MASK     (0xFFFF)
#define HEAP_INST_DEFAULT_VALUE  (1U)  /* Used to show either the heap is always instantiated by default (pfn = NULL)
	                                      OR
	                                      that this is the default configuration of the heap with an Alternative BRN */
#define HEAP_INST_BRN_DEP_VALUE  (2U)  /* The inclusion of this heap is dependent on the brn being present */
#define HEAP_INST_FEAT_DEP_VALUE (3U)  /* The inclusion of this heap is dependent on the feature being present */
#define HEAP_INST_BRN_ALT_VALUE  (4U)  /* This entry is a possible alternative to the default determined by a BRN */
#define HEAP_INST_FEAT_ALT_VALUE (5U)  /* The entry is a possible alternative to the default determined by a Feature define */

/* Latter 16-bits define other flags we may need */
#define HEAP_INST_NON4K_FLAG     (1 << 16U) /* This is a possible NON4K Entry and we should use the device
                                               NON4K size when instantiating */

typedef struct RGX_HEAP_INFO_TAG RGX_HEAP_INFO; // Forward declaration
typedef IMG_BOOL (*PFN_IS_PRESENT)(PVRSRV_RGXDEV_INFO*, const RGX_HEAP_INFO*);
typedef void (*PFN_HEAP_DYNAMIC)(PVRSRV_DEVICE_NODE*, RGX_HEAP_INFO*);

struct RGX_HEAP_INFO_TAG
{
	IMG_CHAR           *pszName;
	IMG_UINT64         ui64HeapBase;
	IMG_DEVMEM_SIZE_T  uiHeapLength;
	IMG_DEVMEM_SIZE_T  uiHeapReservedRegionLength;
	IMG_UINT32         ui32Log2ImportAlignment;
	PFN_IS_PRESENT     pfnIsHeapPresent;
	PFN_HEAP_DYNAMIC   pfnDynamicBaseSize; /* May modify the psHeapInfo's base & length. May be NULL.
	                                          Only called once if the heap is present otherwise never. */
	PFN_HEAP_INIT      pfnInit;
	PFN_HEAP_DEINIT    pfnDeInit;
	IMG_UINT32         ui32HeapInstanceFlags;
};
/* RGX_GENERAL_HEAP_RESERVED_TOTAL_BYTES is the total amount of reserved space, to be specified in gasRGXHeapLayoutApp[] */
#define RGX_GENERAL_HEAP_RESERVED_TOTAL_BYTES	(RGX_HEAP_UM_GENERAL_RESERVED_SIZE + RGXFWIF_KM_GENERAL_HEAP_RESERVED_SIZE)

/* Private heap data struct. */
typedef struct RGX_HEAP_DATA_TAG
{
	DEVMEMINT_RESERVATION  *psMemReservation;
} RGX_HEAP_DATA;

static PVRSRV_ERROR HeapInit(PVRSRV_DEVICE_NODE *psDeviceNode,
							 DEVMEMINT_HEAP *psDevmemHeap,
							 PMR *psPMR,
							 IMG_DEVMEM_SIZE_T uiSize,
							 IMG_UINT64 ui64Offset,
							 IMG_BOOL bWriteAble,
							 IMG_HANDLE *phPrivData)
{
	RGX_HEAP_DATA *psHeapData;
	IMG_DEV_VIRTADDR sCarveOutAddr;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDeviceNode, "psDeviceNode");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevmemHeap, "psDevmemHeap");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psPMR, "psPMR");
	PVR_LOG_RETURN_IF_INVALID_PARAM(phPrivData, "phPrivData");

	psHeapData = OSAllocMem(sizeof(*psHeapData));
	PVR_LOG_RETURN_IF_NOMEM(psHeapData, "psHeapData");

	/* Map the per device secure mem PMR allocation to the general devmem heap carveout. */
	sCarveOutAddr = DevmemIntHeapGetBaseAddr(psDevmemHeap);
	sCarveOutAddr.uiAddr += ui64Offset;

	eError = DevmemIntReserveRange(psDevmemHeap,
									sCarveOutAddr,
									uiSize,
									PVRSRV_MEMALLOCFLAG_GPU_READABLE |
									(bWriteAble ? PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE : 0),
									&psHeapData->psMemReservation);
	PVR_GOTO_IF_ERROR(eError, ErrorFreeHeapData);

	eError = DevmemIntMapPMR(psHeapData->psMemReservation, psPMR);
	PVR_GOTO_IF_ERROR(eError, ErrorUnreserve);

	*phPrivData = (IMG_HANDLE)psHeapData;

	return PVRSRV_OK;

ErrorUnreserve:
	DevmemIntUnreserveRange(psHeapData->psMemReservation);
ErrorFreeHeapData:
	OSFreeMem(psHeapData);

	return eError;
}

/* Deinit callback function for general heap. */
static void HeapDeInit(IMG_HANDLE hPrivData)
{
	RGX_HEAP_DATA *psHeapData = (RGX_HEAP_DATA*)hPrivData;

	PVR_ASSERT(hPrivData);

	DevmemIntUnmapPMR(psHeapData->psMemReservation);
	DevmemIntUnreserveRange(psHeapData->psMemReservation);

	OSFreeMem(psHeapData);
}

/* Init callback function for general heap. */
static PVRSRV_ERROR USCHeapInit(PVRSRV_DEVICE_NODE *psDeviceNode,
                                    DEVMEMINT_HEAP *psDevmemHeap,
                                    IMG_HANDLE *phPrivData)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_BOOL bWriteAble = IMG_FALSE;
	IMG_DEVMEM_SIZE_T uiSize;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDeviceNode, "psDeviceNode");

	psDevInfo = psDeviceNode->pvDevice;

	uiSize = PMR_PhysicalSize(psDevInfo->hTQUSCSharedMem);

	eError = HeapInit(psDeviceNode,
					  psDevmemHeap,
					  psDevInfo->hTQUSCSharedMem,
					  uiSize,
					  RGX_HEAP_KM_USC_RESERVED_REGION_OFFSET,
					  bWriteAble,
					  phPrivData);

	return eError;
}

#if defined(SUPPORT_SECURE_ALLOC_KM)
/* Init callback function for general heap. */
static PVRSRV_ERROR GeneralHeapInit(PVRSRV_DEVICE_NODE *psDeviceNode,
                                    DEVMEMINT_HEAP *psDevmemHeap,
                                    IMG_HANDLE *phPrivData)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_BOOL bWriteAble = IMG_TRUE;
	PVRSRV_ERROR eError;

	psDevInfo = psDeviceNode->pvDevice;

	eError = HeapInit(psDeviceNode,
					  psDevmemHeap,
					  psDevInfo->psGenHeapSecMem,
					  RGXFWIF_KM_GENERAL_HEAP_RESERVED_SIZE,
					  RGX_HEAP_KM_GENERAL_RESERVED_REGION_OFFSET,
					  bWriteAble,
					  phPrivData);

	return eError;
}

#define GeneralHeapDeInit HeapDeInit
#else
/* Callbacks not used */
#define GeneralHeapInit NULL
#define GeneralHeapDeInit NULL
#endif

static void SVMHeapDynamic(PVRSRV_DEVICE_NODE *psDeviceNode,
                           RGX_HEAP_INFO *psHeapInfo)
{
	IMG_UINT64 ui64OSPageSize = OSGetPageSize();
#if defined(FIX_HW_BRN_65273_BIT_MASK)
	PVRSRV_RGXDEV_INFO* psDevInfo = psDeviceNode->pvDevice;
#endif /* defined(FIX_HW_BRN_65273_BIT_MASK) */

	/* Ensures the SVM heap has the correct alignment & size for any OS page size.
	 *
	 * The SVM heap's base must be the smallest possible address mappable by UM.
	 * This is 32KB unless the page size is larger than 32KB. [1]
	 * If the page size > 32KB, raise the SVM heap base to the next page boundary.
	 * Also reduce the length to ensure it's still page aligned and doesn't go
	 * into another heap.
	 *
	 * [1]: https://chromium.googlesource.com/chromium/src/+/fe24932ee14aa93e1fe4d3e7003b9362591a54d4/docs/security/faq.md#why-aren_t-null-pointer-dereferences-considered-security-bugs
	 */
	IMG_UINT64 ui64Base = PVR_ALIGN(psHeapInfo->ui64HeapBase, ui64OSPageSize);
	IMG_UINT64 ui64BaseDiff = ui64Base - psHeapInfo->ui64HeapBase;
	psHeapInfo->ui64HeapBase = ui64Base;
	if (psHeapInfo->uiHeapLength >= ui64BaseDiff)
	    psHeapInfo->uiHeapLength -= ui64BaseDiff;
	if (psHeapInfo->uiHeapReservedRegionLength >= ui64BaseDiff)
	    psHeapInfo->uiHeapReservedRegionLength -= ui64BaseDiff;

	/* The device shared-virtual-memory heap address-space size is stored on the device for
	   faster look-up without having to walk the device heap configuration structures during
	   client device connection  (i.e. this size is relative to a zero-based offset) */
#if defined(FIX_HW_BRN_65273_BIT_MASK)
	if (RGX_IS_BRN_SUPPORTED(psDevInfo, 65273))
	{
		psDeviceNode->ui64GeneralSVMHeapTopVA = 0;
	}
	else
#endif /* defined(FIX_HW_BRN_65273_BIT_MASK) */
	{
		psDeviceNode->ui64GeneralSVMHeapTopVA = psHeapInfo->ui64HeapBase + psHeapInfo->uiHeapLength;
	}
}

/* Feature Present function prototypes */

#if defined(FIX_HW_BRN_65273_BIT_MASK)
static IMG_BOOL BRN65273IsPresent(PVRSRV_RGXDEV_INFO *psDevInfo, const RGX_HEAP_INFO *pksHeapInfo)
{
	if (RGX_IS_BRN_SUPPORTED(psDevInfo, 65273))
	{
		return (((pksHeapInfo->ui32HeapInstanceFlags & HEAP_INST_VALUE_MASK) == HEAP_INST_BRN_ALT_VALUE) ||
		        ((pksHeapInfo->ui32HeapInstanceFlags & HEAP_INST_VALUE_MASK) == HEAP_INST_BRN_DEP_VALUE)) ?
		        IMG_TRUE : IMG_FALSE;
	}
	else
	{
		return ((pksHeapInfo->ui32HeapInstanceFlags & HEAP_INST_VALUE_MASK) == HEAP_INST_DEFAULT_VALUE) ? IMG_TRUE : IMG_FALSE;
	}
}
#endif

#if defined(FIX_HW_BRN_63142_BIT_MASK)
static IMG_BOOL BRN63142IsPresent(PVRSRV_RGXDEV_INFO *psDevInfo, const RGX_HEAP_INFO *pksHeapInfo)
{
	PVR_UNREFERENCED_PARAMETER(pksHeapInfo);

	if (RGX_IS_BRN_SUPPORTED(psDevInfo, 63142))
	{
		PVR_ASSERT((pksHeapInfo->ui64HeapBase & IMG_UINT64_C(0x3FFFFFFFF)) +
		            pksHeapInfo->uiHeapLength == IMG_UINT64_C(0x400000000));

		return IMG_TRUE;
	}

	return IMG_FALSE;
}
#endif

static IMG_BOOL FBCDescriptorIsPresent(PVRSRV_RGXDEV_INFO *psDevInfo, const RGX_HEAP_INFO *pksHeapInfo)
{
	PVR_UNREFERENCED_PARAMETER(pksHeapInfo);

	if (RGX_GET_FEATURE_VALUE(psDevInfo, FBC_MAX_DEFAULT_DESCRIPTORS))
	{
		return IMG_TRUE;
	}

	return IMG_FALSE;
}

static IMG_BOOL FBCLargeDescriptorIsPresent(PVRSRV_RGXDEV_INFO *psDevInfo, const RGX_HEAP_INFO *pksHeapInfo)
{
	PVR_UNREFERENCED_PARAMETER(pksHeapInfo);

	if (RGX_GET_FEATURE_VALUE(psDevInfo, FBC_MAX_LARGE_DESCRIPTORS))
	{
		return IMG_TRUE;
	}

	return IMG_FALSE;
}

static IMG_BOOL TextureStateIsPresent(PVRSRV_RGXDEV_INFO *psDevInfo, const RGX_HEAP_INFO *pksHeapInfo)
{
	PVR_UNREFERENCED_PARAMETER(pksHeapInfo);
#if defined(RGX_FEATURE_BINDLESS_IMAGE_AND_TEXTURE_STATE_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, BINDLESS_IMAGE_AND_TEXTURE_STATE))
	{
		return IMG_TRUE;
	}
#else
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
#endif
	return IMG_FALSE;
}

/* FW Feature Present function prototypes */

#if defined(FIX_HW_BRN_65101_BIT_MASK)
static IMG_BOOL FWBRN65101IsPresent(PVRSRV_RGXDEV_INFO *psDevInfo, const RGX_HEAP_INFO *pksHeapInfo)
{
	/* Used to determine the correct table row to instantiate as a heap by checking
	 * the Heap size and base at run time VS the current table instance
	 */
	IMG_UINT64 ui64MainSubHeapSize;

	/* MIPS Firmware must reserve some space in its Host/Native heap for GPU memory mappings */
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) && (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo)))
	{
		if (RGX_IS_BRN_SUPPORTED(psDevInfo, 65101))
		{
			ui64MainSubHeapSize = RGX_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_BRN65101;
		}
		else
		{
			ui64MainSubHeapSize = RGX_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_NORMAL;
		}
	}
	else
	{
		ui64MainSubHeapSize = RGX_FIRMWARE_DEFAULT_MAIN_HEAP_SIZE;
	}

	/* Determine if we should include this entry based upon previous checks */
	return (pksHeapInfo->uiHeapLength == ui64MainSubHeapSize &&
	        pksHeapInfo->ui64HeapBase == RGX_FIRMWARE_MAIN_HEAP_BASE) ?
	        IMG_TRUE : IMG_FALSE;
}
#endif

static IMG_BOOL FWVZConfigPresent(PVRSRV_RGXDEV_INFO* psDevInfo, const RGX_HEAP_INFO* pksHeapInfo)
{
	/* Used to determine the correct table row to instantiate as a heap by checking
	 * the Heap base at run time VS the current table instance
	 */

	/* Determine if we should include this entry based upon previous checks */
	return (pksHeapInfo->ui64HeapBase == RGX_FIRMWARE_CONFIG_HEAP_BASE) ? IMG_TRUE : IMG_FALSE;
}

/* Blueprint array. note: not all heaps are available to clients*/

static RGX_HEAP_INFO gasRGXHeapLayoutApp[] =
{
	/* Name                             HeapBase                                 HeapLength                               HeapReservedRegionLength                     Log2ImportAlignment pfnIsHeapPresent             pfnDynamicBaseSize pfnInit           pfnDeInit          HeapInstanceFlags   */
	{RGX_GENERAL_SVM_HEAP_IDENT,        RGX_GENERAL_SVM_HEAP_BASE,               RGX_GENERAL_SVM_HEAP_SIZE,               0,                                           0,                  NULL,                        SVMHeapDynamic,    NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_GENERAL_HEAP_IDENT,            RGX_GENERAL_HEAP_BASE,                   RGX_GENERAL_HEAP_SIZE,                   RGX_GENERAL_HEAP_RESERVED_TOTAL_BYTES,       0,                  BRN65273IsPresent,           NULL,              GeneralHeapInit,  GeneralHeapDeInit, HEAP_INST_DEFAULT_VALUE },
	{RGX_GENERAL_HEAP_IDENT,            RGX_GENERAL_BRN_65273_HEAP_BASE,         RGX_GENERAL_BRN_65273_HEAP_SIZE,         RGX_GENERAL_HEAP_RESERVED_TOTAL_BYTES,       0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_ALT_VALUE },
	{RGX_GENERAL_NON4K_HEAP_IDENT,      RGX_GENERAL_NON4K_HEAP_BASE,             RGX_GENERAL_NON4K_HEAP_SIZE,             0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE | HEAP_INST_NON4K_FLAG },
	{RGX_GENERAL_NON4K_HEAP_IDENT,      RGX_GENERAL_NON4K_BRN_65273_HEAP_BASE,   RGX_GENERAL_NON4K_BRN_65273_HEAP_SIZE,   0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_ALT_VALUE | HEAP_INST_NON4K_FLAG },
	{RGX_PDSCODEDATA_HEAP_IDENT,        RGX_PDSCODEDATA_HEAP_BASE,               RGX_PDSCODEDATA_HEAP_SIZE,               RGX_HEAP_PDS_RESERVED_TOTAL_SIZE,            0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_PDSCODEDATA_HEAP_IDENT,        RGX_PDSCODEDATA_BRN_65273_HEAP_BASE,     RGX_PDSCODEDATA_BRN_65273_HEAP_SIZE,     RGX_HEAP_PDS_RESERVED_TOTAL_SIZE,            0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_ALT_VALUE },
	{RGX_RGNHDR_BRN_63142_HEAP_IDENT,   RGX_RGNHDR_BRN_63142_HEAP_BASE,          RGX_RGNHDR_BRN_63142_HEAP_SIZE,          0,                                           0,                  BRN63142IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_DEP_VALUE },
	{RGX_USCCODE_HEAP_IDENT,            RGX_USCCODE_HEAP_BASE,                   RGX_USCCODE_HEAP_SIZE,                   RGX_HEAP_USC_RESERVED_TOTAL_SIZE,            0,                  BRN65273IsPresent,           NULL,              USCHeapInit,      HeapDeInit,        HEAP_INST_DEFAULT_VALUE },
	{RGX_USCCODE_HEAP_IDENT,            RGX_USCCODE_BRN_65273_HEAP_BASE,         RGX_USCCODE_BRN_65273_HEAP_SIZE,         RGX_HEAP_USC_RESERVED_TOTAL_SIZE,            0,                  BRN65273IsPresent,           NULL,              USCHeapInit,      HeapDeInit,        HEAP_INST_BRN_ALT_VALUE },
	{RGX_TQ3DPARAMETERS_HEAP_IDENT,     RGX_TQ3DPARAMETERS_HEAP_BASE,            RGX_TQ3DPARAMETERS_HEAP_SIZE,            0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_TQ3DPARAMETERS_HEAP_IDENT,     RGX_TQ3DPARAMETERS_BRN_65273_HEAP_BASE,  RGX_TQ3DPARAMETERS_BRN_65273_HEAP_SIZE,  0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_ALT_VALUE },
	{RGX_VK_CAPT_REPLAY_HEAP_IDENT,     RGX_VK_CAPT_REPLAY_HEAP_BASE,            RGX_VK_CAPT_REPLAY_HEAP_SIZE,            0,                                           0,                  NULL,                        NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_FBCDC_HEAP_IDENT,              RGX_FBCDC_HEAP_BASE,                     RGX_FBCDC_HEAP_SIZE,                     0,                                           0,                  FBCDescriptorIsPresent,      NULL,              NULL,             NULL,              HEAP_INST_FEAT_DEP_VALUE},
	{RGX_FBCDC_LARGE_HEAP_IDENT,        RGX_FBCDC_LARGE_HEAP_BASE,               RGX_FBCDC_LARGE_HEAP_SIZE,               0,                                           0,                  FBCLargeDescriptorIsPresent, NULL,              NULL,             NULL,              HEAP_INST_FEAT_DEP_VALUE},
	{RGX_CMP_MISSION_RMW_HEAP_IDENT,    RGX_CMP_MISSION_RMW_HEAP_BASE,           RGX_CMP_MISSION_RMW_HEAP_SIZE,           0,                                           0,                  NULL,                        NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_CMP_SAFETY_RMW_HEAP_IDENT,     RGX_CMP_SAFETY_RMW_HEAP_BASE,            RGX_CMP_SAFETY_RMW_HEAP_SIZE,            0,                                           0,                  NULL,                        NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_TEXTURE_STATE_HEAP_IDENT,      RGX_TEXTURE_STATE_HEAP_BASE,             RGX_TEXTURE_STATE_HEAP_SIZE,             0,                                           0,                  TextureStateIsPresent,       NULL,              NULL,             NULL,              HEAP_INST_FEAT_DEP_VALUE},
	{RGX_VISIBILITY_TEST_HEAP_IDENT,    RGX_VISIBILITY_TEST_HEAP_BASE,           RGX_VISIBILITY_TEST_HEAP_SIZE,           0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_DEFAULT_VALUE },
	{RGX_VISIBILITY_TEST_HEAP_IDENT,    RGX_VISIBILITY_TEST_BRN_65273_HEAP_BASE, RGX_VISIBILITY_TEST_BRN_65273_HEAP_SIZE, 0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_ALT_VALUE },
	{RGX_MMU_INIA_BRN_65273_HEAP_IDENT, RGX_MMU_INIA_BRN_65273_HEAP_BASE,        RGX_MMU_INIA_BRN_65273_HEAP_SIZE,        0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_DEP_VALUE },
	{RGX_MMU_INIB_BRN_65273_HEAP_IDENT, RGX_MMU_INIB_BRN_65273_HEAP_BASE,        RGX_MMU_INIB_BRN_65273_HEAP_SIZE,        0,                                           0,                  BRN65273IsPresent,           NULL,              NULL,             NULL,              HEAP_INST_BRN_DEP_VALUE },
};

static RGX_HEAP_INFO gasRGXHeapLayoutFW[] =
{
	/* Name                          HeapBase                        HeapLength                                      HeapReservedRegionLength Log2ImportAlignment pfnIsHeapPresent     pfnDynamicBaseSize pfnInit pfnDeInit HeapInstanceFlags*/
	{RGX_FIRMWARE_MAIN_HEAP_IDENT,   RGX_FIRMWARE_MAIN_HEAP_BASE,    RGX_FIRMWARE_DEFAULT_MAIN_HEAP_SIZE,            0,                       0,                  FWBRN65101IsPresent, NULL,              NULL,   NULL,      HEAP_INST_DEFAULT_VALUE},
	{RGX_FIRMWARE_MAIN_HEAP_IDENT,   RGX_FIRMWARE_MAIN_HEAP_BASE,    RGX_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_NORMAL,   0,                       0,                  FWBRN65101IsPresent, NULL,              NULL,   NULL,      HEAP_INST_DEFAULT_VALUE},
	{RGX_FIRMWARE_MAIN_HEAP_IDENT,   RGX_FIRMWARE_MAIN_HEAP_BASE,    RGX_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_BRN65101, 0,                       0,                  FWBRN65101IsPresent, NULL,              NULL,   NULL,      HEAP_INST_BRN_ALT_VALUE},
	{RGX_FIRMWARE_CONFIG_HEAP_IDENT, RGX_FIRMWARE_CONFIG_HEAP_BASE,  RGX_FIRMWARE_CONFIG_HEAP_SIZE,                  0,                       0,                  FWVZConfigPresent,   NULL,              NULL,   NULL,      HEAP_INST_DEFAULT_VALUE},
};

static INLINE IMG_BOOL IsFwHeapLayout(const RGX_HEAP_INFO *psHeapInfo)
{
	return psHeapInfo->pszName[0] == 'F' &&
	       psHeapInfo->pszName[1] == 'w' ? IMG_TRUE : IMG_FALSE;
}

static INLINE void CheckHeapAlignment(const RGX_HEAP_INFO *psHeapInfo,
                                      const PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_UINT64 uiAlignment = RGX_HEAP_BASE_SIZE_ALIGN - 1;

	if (IsFwHeapLayout(psHeapInfo) ||
	    RGX_IS_BRN_SUPPORTED(psDevInfo, 65273))
	{
		/*
		 * 1) Main heap starts at 2MB offset 0xEC10000000UL
		 * 2) Config Sub heap is created at the end of the main heap making the entire unit start and end at 2MB offset aligned,
		 *    these 2 heaps will always have the same page size
		 *    There are no other heaps in between these two heaps, there is no risk for another devmem heap be created between them.
		 */
		return;
	}

	/* General SVM Heap must be placed below 2MiB boundary so we need to adjust
	 * the validity condition. This is because an OS might return virtual
	 * addresses below 2MiB threshold. By default (based on testing on Linux)
	 * this is around 32KiB. */
	if (OSStringNCompare(psHeapInfo->pszName, RGX_GENERAL_SVM_HEAP_IDENT,
	                     sizeof(RGX_GENERAL_SVM_HEAP_IDENT)) == 0)
	{
		uiAlignment = RGX_GENERAL_SVM_BASE_SIZE_ALIGNMENT - 1;
	}

	/* All UM accessible heap bases should be aligned to 2MB */
	if (psHeapInfo->ui64HeapBase & uiAlignment)
	{
		PVR_ASSERT(!"Heap Base not aligned to RGX_HEAP_BASE_SIZE_ALIGN");
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Invalid Heap \"%s\" Base: "
		         "%"IMG_UINT64_FMTSPEC")",
		         __func__,
		         psHeapInfo->pszName,
		         psHeapInfo->ui64HeapBase));
		PVR_DPF((PVR_DBG_ERROR,
		         "Heap Base (0x%"IMG_UINT64_FMTSPECX") should always be aligned to "
		         "RGX_HEAP_BASE_ALIGN (0x%" IMG_UINT64_FMTSPECX ")",
		         psHeapInfo->ui64HeapBase,
		         uiAlignment + 1));
	}

	/* All UM accessible heaps should also be size aligned to 2MB */
	if (psHeapInfo->uiHeapLength & uiAlignment)
	{
		PVR_ASSERT(!"Heap Size not aligned to RGX_HEAP_BASE_SIZE_ALIGN");
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Invalid Heap \"%s\" Size: "
		         "%"IMG_UINT64_FMTSPEC")",
		         __func__,
		         psHeapInfo->pszName,
		         psHeapInfo->uiHeapLength));
		PVR_DPF((PVR_DBG_ERROR,
		         "Heap Size (0x%"IMG_UINT64_FMTSPECX") should always be aligned to "
		         "RGX_HEAP_BASE_SIZE_ALIGN (0x%" IMG_UINT64_FMTSPECX ")",
		         psHeapInfo->uiHeapLength,
		         uiAlignment + 1));
	}
}

/* Generic counting method. */
static void _CountRequiredHeaps(PVRSRV_RGXDEV_INFO  *psDevInfo,
	                            const RGX_HEAP_INFO  pksHeapInfo[],
	                            IMG_UINT32           ui32HeapListSize,
	                            IMG_UINT32*          ui32HeapCount)
{
	IMG_UINT32 i;

	/* Loop over rows in the heap data array using callback to decide if we
	 * should include the heap
	 */
	for (i = 0; i < ui32HeapListSize; i++)
	{
		const RGX_HEAP_INFO *psHeapInfo = &pksHeapInfo[i];

		if (psHeapInfo->pfnIsHeapPresent)
		{
			if (!psHeapInfo->pfnIsHeapPresent(psDevInfo, psHeapInfo))
			{
				/* We don't need to create this heap */
				continue;
			}
		}

		(*ui32HeapCount)++;
	}
}
/* Generic heap instantiator */
static void _InstantiateRequiredHeaps(PVRSRV_RGXDEV_INFO     *psDevInfo,
                                      RGX_HEAP_INFO           psHeapInfos[],
                                      IMG_UINT32              ui32HeapListSize,
                                      const IMG_UINT32        ui32Log2RgxDefaultPageShift,
                                      DEVMEM_HEAP_BLUEPRINT **psDeviceMemoryHeapCursor)
{
	IMG_UINT32 i;

#if defined DEBUG
	IMG_UINT32 ui32heapListCnt;
	bool bHeapPageSizeMisMatch = false;

	/*
	 * To ensure all heaps within a 2MB region have the same page sizes
	 */
	for (ui32heapListCnt = 0; ui32heapListCnt < (ui32HeapListSize-1); ui32heapListCnt++)
	{
		const RGX_HEAP_INFO *psHeapInfo1 = &psHeapInfos[ui32heapListCnt];
		const RGX_HEAP_INFO *psHeapInfo2 = &psHeapInfos[ui32heapListCnt+1];

		if (((psHeapInfo1->uiHeapLength) & (RGX_HEAP_BASE_SIZE_ALIGN - 1)) &&
			((psHeapInfo1->ui64HeapBase + psHeapInfo1->uiHeapLength) & (RGX_HEAP_BASE_SIZE_ALIGN - 1)) ==
			((psHeapInfo2->ui64HeapBase) & (RGX_HEAP_BASE_SIZE_ALIGN - 1)))
		{
			if (psHeapInfo1->ui32HeapInstanceFlags & HEAP_INST_NON4K_FLAG)
			{
				if (!(psHeapInfo2->ui32HeapInstanceFlags & HEAP_INST_NON4K_FLAG))
				{
					bHeapPageSizeMisMatch = true;
				}
			}
			else if (psHeapInfo2->ui32HeapInstanceFlags & HEAP_INST_NON4K_FLAG)
			{
				bHeapPageSizeMisMatch = true;
			}
		}
		if (bHeapPageSizeMisMatch)
		{
			PVR_ASSERT(!"Two Heap with Different Page Size allocated in the same PD space(2MB)");
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Invalid Heaps 1) \"%s\" and 2) \"%s\"",
					__func__,
					psHeapInfo1->pszName,
					psHeapInfo2->pszName));
		}
	}
#endif

	/* We now have a list of the heaps to include and so we should loop over this
	 * list and instantiate.
	 */
	for (i = 0; i < ui32HeapListSize; i++)
	{
		IMG_UINT32 ui32Log2DataPageSize = 0;
		RGX_HEAP_INFO *psHeapInfo = &psHeapInfos[i];

		if (psHeapInfo->pfnIsHeapPresent)
		{
			if (!psHeapInfo->pfnIsHeapPresent(psDevInfo, psHeapInfo))
			{
				/* We don't need to create this heap */
				continue;
			}
		}

		if (psHeapInfo->pfnDynamicBaseSize != NULL)
		{
			psHeapInfo->pfnDynamicBaseSize(psDevInfo->psDeviceNode, psHeapInfo);
		}

		if (psHeapInfo->ui32HeapInstanceFlags & HEAP_INST_NON4K_FLAG)
		{
			ui32Log2DataPageSize = psDevInfo->psDeviceNode->ui32Non4KPageSizeLog2;
		}
		else
		{
			ui32Log2DataPageSize = ui32Log2RgxDefaultPageShift;
		}

		CheckHeapAlignment(psHeapInfo, psDevInfo);

		HeapCfgBlueprintInit(psHeapInfo->pszName,
		                     psHeapInfo->ui64HeapBase,
		                     psHeapInfo->uiHeapLength,
		                     psHeapInfo->uiHeapReservedRegionLength,
		                     ui32Log2DataPageSize,
		                     psHeapInfo->ui32Log2ImportAlignment,
		                     psHeapInfo->pfnInit,
		                     psHeapInfo->pfnDeInit,
		                     *psDeviceMemoryHeapCursor);

		(*psDeviceMemoryHeapCursor)++;
	}
}

static PVRSRV_ERROR RGXInitHeaps(PVRSRV_RGXDEV_INFO *psDevInfo,
	                             DEVICE_MEMORY_INFO *psNewMemoryInfo)
{
	PVRSRV_ERROR eError;
	DEVMEM_HEAP_BLUEPRINT *psDeviceMemoryHeapCursor;

	IMG_UINT32 ui32AppHeapListSize = ARRAY_SIZE(gasRGXHeapLayoutApp);
	IMG_UINT32 ui32FWHeapListSize = ARRAY_SIZE(gasRGXHeapLayoutFW);
	IMG_UINT32 ui32CountedHeapSize;

	IMG_UINT32 ui32AppHeapCount = 0;
	IMG_UINT32 ui32FWHeapCount = 0;

	IMG_UINT32 ui32Log2DefaultPageShift = RGXHeapDerivePageSize(OSGetPageShift());

	if (ui32Log2DefaultPageShift == 0)
	{
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e0;
	}

#if defined(FIX_HW_BRN_71317_BIT_MASK)
	if (RGX_IS_BRN_SUPPORTED(psDevInfo, 71317))
	{
		if (ui32Log2DefaultPageShift == RGX_HEAP_2MB_PAGE_SHIFT
		   || ui32Log2DefaultPageShift == RGX_HEAP_1MB_PAGE_SHIFT)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "OS page size too large for device virtual heaps. "
			         "Maximum page size supported is 256KB when BRN71317 is present."));
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto e0;
		}
	}
#endif

	/* Count heaps required for the app heaps */
	_CountRequiredHeaps(psDevInfo,
		                gasRGXHeapLayoutApp,
		                ui32AppHeapListSize,
		                &ui32AppHeapCount);

	/* Count heaps required for the FW heaps */
	_CountRequiredHeaps(psDevInfo,
		                gasRGXHeapLayoutFW,
		                ui32FWHeapListSize,
		                &ui32FWHeapCount);

	ui32CountedHeapSize = (ui32AppHeapCount + ui32FWHeapCount + RGX_NUM_DRIVERS_SUPPORTED);

	psNewMemoryInfo->psDeviceMemoryHeap = OSAllocMem(sizeof(DEVMEM_HEAP_BLUEPRINT) * ui32CountedHeapSize);
	PVR_LOG_GOTO_IF_NOMEM(psNewMemoryInfo->psDeviceMemoryHeap, eError, e0);

	/* Initialise the heaps */
	psDeviceMemoryHeapCursor = psNewMemoryInfo->psDeviceMemoryHeap;

	/* Instantiate App Heaps */
	_InstantiateRequiredHeaps(psDevInfo,
	                          gasRGXHeapLayoutApp,
	                          ui32AppHeapListSize,
	                          ui32Log2DefaultPageShift,
	                          &psDeviceMemoryHeapCursor);

	/* Instantiate FW Heaps */
	_InstantiateRequiredHeaps(psDevInfo,
	                          gasRGXHeapLayoutFW,
	                          ui32FWHeapListSize,
	                          ui32Log2DefaultPageShift,
	                          &psDeviceMemoryHeapCursor);

	/* set the heap count */
	psNewMemoryInfo->ui32HeapCount = (IMG_UINT32)(psDeviceMemoryHeapCursor - psNewMemoryInfo->psDeviceMemoryHeap);

	/* Check we have allocated the correct # of heaps, minus any VZ heaps as these
	 * have not been created at this point
	 */
	PVR_ASSERT(psNewMemoryInfo->ui32HeapCount == (ui32CountedHeapSize - RGX_NUM_DRIVERS_SUPPORTED));

	/*
	   In the new heap setup, we initialise 2 configurations:
		1 - One will be for the firmware only (index 1 in array)
			a. This primarily has the firmware heap in it.
			b. It also has additional guest OSID firmware heap(s)
				- Only if the number of support firmware OSID > 1
		2 - Others shall be for clients only (index 0 in array)
			a. This has all the other client heaps in it.
	 */
	psNewMemoryInfo->uiNumHeapConfigs = 2;
	psNewMemoryInfo->psDeviceMemoryHeapConfigArray = OSAllocMem(sizeof(DEVMEM_HEAP_CONFIG) * psNewMemoryInfo->uiNumHeapConfigs);
	PVR_LOG_GOTO_IF_NOMEM(psNewMemoryInfo->psDeviceMemoryHeapConfigArray, eError, e1);

	psNewMemoryInfo->psDeviceMemoryHeapConfigArray[0].pszName = "Default Heap Configuration";
	psNewMemoryInfo->psDeviceMemoryHeapConfigArray[0].uiNumHeaps = psNewMemoryInfo->ui32HeapCount - RGX_FIRMWARE_NUMBER_OF_FW_HEAPS;
	psNewMemoryInfo->psDeviceMemoryHeapConfigArray[0].psHeapBlueprintArray = psNewMemoryInfo->psDeviceMemoryHeap;

	psNewMemoryInfo->psDeviceMemoryHeapConfigArray[1].pszName = "Firmware Heap Configuration";
	psNewMemoryInfo->psDeviceMemoryHeapConfigArray[1].uiNumHeaps = RGX_FIRMWARE_NUMBER_OF_FW_HEAPS;
	psNewMemoryInfo->psDeviceMemoryHeapConfigArray[1].psHeapBlueprintArray = psDeviceMemoryHeapCursor - RGX_FIRMWARE_NUMBER_OF_FW_HEAPS;

#if defined(RGX_FEATURE_MMU_VERSION_MAX_VALUE_IDX)
	if (RGX_GET_FEATURE_VALUE(psDevInfo, MMU_VERSION) >= 4)
	{
		IMG_UINT32 i;
		const IMG_UINT32 ui32GeneralNon4KHeapPageSize = (1 << psDevInfo->psDeviceNode->ui32Non4KPageSizeLog2);
		const IMG_UINT32 ui32RgxDefaultPageSize = (1 << RGXHeapDerivePageSize(OSGetPageShift()));

		/*
		 * Initialise all MMU Page Size Range Config register to the default page size
		 * used by the OS, leaving the address range 0;
		 */
		for (i = 0; i < ARRAY_SIZE(psDevInfo->aui64MMUPageSizeRangeValue); ++i)
		{
			psDevInfo->aui64MMUPageSizeRangeValue[i] =
					RGXMMUInit_GetConfigRangeValue(ui32RgxDefaultPageSize,
												   0,
												   (1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT));
		}


		/* set the last MMU config range covering the entire virtual memory to the OS's page size */
		psDevInfo->aui64MMUPageSizeRangeValue[RGX_MAX_NUM_MMU_PAGE_SIZE_RANGES - 1] =
				RGXMMUInit_GetConfigRangeValue(ui32RgxDefaultPageSize, 0, (1ULL << 40));

		/*
		 * If the Non4K heap has a different page size than the OS's page size
		 * (used as default for all other heaps), configure one MMU config range
		 * for the Non4K heap
		 */
		if (ui32GeneralNon4KHeapPageSize != ui32RgxDefaultPageSize)
		{
			psDevInfo->aui64MMUPageSizeRangeValue[0] =
					RGXMMUInit_GetConfigRangeValue(ui32GeneralNon4KHeapPageSize,
												   RGX_GENERAL_NON4K_HEAP_BASE,
												   RGX_GENERAL_NON4K_HEAP_SIZE);
		}
	}
#endif

#if defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		IMG_UINT32 ui32DriverID;

		/* Create additional raw firmware heaps */
		FOREACH_DRIVER_RAW_HEAP(ui32DriverID, DEVINFO, psDevInfo)
		{
			eError = RGXInitFwRawHeap(psDevInfo, psDeviceMemoryHeapCursor, ui32DriverID);
			if (eError != PVRSRV_OK)
			{
				/* if any allocation fails, free previously allocated heaps and abandon initialisation */
				for (; ui32DriverID > RGX_FIRST_RAW_HEAP_DRIVER_ID; ui32DriverID--)
				{
					RGXDeInitFwRawHeap(psDeviceMemoryHeapCursor);
					psDeviceMemoryHeapCursor--;
				}
				goto e1;
			}

			/* Append additional firmware heaps to host driver firmware context heap configuration */
			psNewMemoryInfo->psDeviceMemoryHeapConfigArray[1].uiNumHeaps += 1;

			/* advance to the next heap */
			psDeviceMemoryHeapCursor++;
		}
	}
#endif /* defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1) */

	return PVRSRV_OK;
e1:
	OSFreeMem(psNewMemoryInfo->psDeviceMemoryHeap);
e0:
	return eError;
}

static void RGXDeInitHeaps(DEVICE_MEMORY_INFO *psDevMemoryInfo, PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		IMG_UINT32 ui32DriverID;
		DEVMEM_HEAP_BLUEPRINT *psDeviceMemoryHeapCursor = psDevMemoryInfo->psDeviceMemoryHeap;

		/* Delete all guest firmware heaps */
		FOREACH_DRIVER_RAW_HEAP(ui32DriverID, DEVNODE, psDeviceNode)
		{
			RGXDeInitFwRawHeap(psDeviceMemoryHeapCursor);
			psDeviceMemoryHeapCursor++;
		}
	}
#else
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
#endif /* defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1) */

	OSFreeMem(psDevMemoryInfo->psDeviceMemoryHeapConfigArray);
	OSFreeMem(psDevMemoryInfo->psDeviceMemoryHeap);
}

static PVRSRV_ERROR RGXInitSharedFwPhysHeaps(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PHYS_HEAP_CONFIG *psSysHeapCfg = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																PHYS_HEAP_USAGE_FW_SHARED);

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	/* VZ heap validation */
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PVR_LOG_RETURN_IF_FALSE(psSysHeapCfg != NULL,
								"FW Main heap is required for VZ Guest.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);
	}
#endif

	if (psSysHeapCfg != NULL)
	{
		/* Check FW_SHARED for multiple usage flags. Because FW_SHARED is divided
		   into subheaps, shared usage with other heaps is not allowed.  */
		PVR_LOG_RETURN_IF_FALSE(psSysHeapCfg->ui32UsageFlags == PHYS_HEAP_USAGE_FW_SHARED,
								"FW_SHARED phys heap config not specified with more than one usage."
								"FW_SHARED heap must be exclusively used as FW_SHARED.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);
	}

	if (psSysHeapCfg == NULL)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Firmware physical heap not set", __func__));
		/* Nothing to do. Default to the physheap fallback option */
	}
	else if (psSysHeapCfg->eType == PHYS_HEAP_TYPE_UMA)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Firmware physical heap uses OS System memory (UMA)", __func__));

		eError = PhysHeapCreateHeapFromConfig(psDeviceNode,
											  psSysHeapCfg,
											  &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_MAIN]);
		PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeap");

		psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_CONFIG] = psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_MAIN];
	}
	else /* PHYS_HEAP_TYPE_LMA or PHYS_HEAP_TYPE_DMA */
	{
		PHYS_HEAP_CONFIG sFwMainHeapCfg, sFwCfgHeapCfg;

		PVR_DPF((PVR_DBG_MESSAGE, "%s: Firmware physical heap uses local memory managed by the driver (LMA)", __func__));


		/* Subheap layout: Main + (optional MIPS reserved range) + Config */
		sFwMainHeapCfg = *psSysHeapCfg;
		PVR_ASSERT(sFwMainHeapCfg.eType == PHYS_HEAP_TYPE_LMA ||
		           sFwMainHeapCfg.eType == PHYS_HEAP_TYPE_DMA);

		/* Reserve space for the Config heap */
		sFwMainHeapCfg.uConfig.sLMA.uiSize -= RGX_FIRMWARE_CONFIG_HEAP_SIZE;

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
		{
			PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;

			/* MIPS Firmware must reserve some space in its Host/Native heap for GPU memory mappings */
			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) && (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode)))
			{
#if defined(FIX_HW_BRN_65101_BIT_MASK)
				if (RGX_IS_BRN_SUPPORTED(psDevInfo, 65101))
				{
					sFwMainHeapCfg.uConfig.sLMA.uiSize -= RGX_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_BRN65101;
				}
				else
#endif
				{
					sFwMainHeapCfg.uConfig.sLMA.uiSize -= RGX_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_NORMAL;
				}
			}
		}
#endif

		eError = PhysmemCreateHeapLMA(psDeviceNode,
		                              RGXPhysHeapGetLMAPolicy(sFwMainHeapCfg.ui32UsageFlags, psDeviceNode),
		                              &sFwMainHeapCfg,
		                              "Fw Main subheap",
		                              &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_MAIN]);
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemCreateHeapLMA:MAIN", ErrorDeinit);

		sFwCfgHeapCfg = *psSysHeapCfg;
		PVR_ASSERT(sFwCfgHeapCfg.eType == PHYS_HEAP_TYPE_LMA ||
		           sFwCfgHeapCfg.eType == PHYS_HEAP_TYPE_DMA);

		sFwCfgHeapCfg.uConfig.sLMA.sStartAddr.uiAddr += psSysHeapCfg->uConfig.sLMA.uiSize - RGX_FIRMWARE_CONFIG_HEAP_SIZE;
		sFwCfgHeapCfg.uConfig.sLMA.sCardBase.uiAddr += psSysHeapCfg->uConfig.sLMA.uiSize - RGX_FIRMWARE_CONFIG_HEAP_SIZE;

		sFwCfgHeapCfg.uConfig.sLMA.uiSize = RGX_FIRMWARE_CONFIG_HEAP_SIZE;

		eError = PhysmemCreateHeapLMA(psDeviceNode,
		                              RGXPhysHeapGetLMAPolicy(sFwCfgHeapCfg.ui32UsageFlags, psDeviceNode),
		                              &sFwCfgHeapCfg,
		                              "Fw Cfg subheap",
		                              &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_CONFIG]);
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemCreateHeapLMA:CFG", ErrorDeinit);
	}

	/* Acquire FW heaps */
	eError = PhysHeapAcquireByID(PVRSRV_PHYS_HEAP_FW_MAIN, psDeviceNode,
								 &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_MAIN]);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapAcquire:FW_MAIN", ErrorDeinit);

	eError = PhysHeapAcquireByID(PVRSRV_PHYS_HEAP_FW_CONFIG, psDeviceNode,
								 &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_CONFIG]);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapAcquire:FW_CONFIG", ErrorDeinit);

	return eError;

ErrorDeinit:
	PVR_ASSERT(IMG_FALSE);

	return eError;
}

static PVRSRV_ERROR RGXInitPrivateFwPhysHeaps(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PHYS_HEAP_CONFIG *psFwCodeHeapCfg = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																 PHYS_HEAP_USAGE_FW_CODE);
	PHYS_HEAP_CONFIG *psFwDataHeapCfg = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																 PHYS_HEAP_USAGE_FW_PRIV_DATA);
	PHYS_HEAP_CONFIG *psFwPrivateHeapCfg = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																 PHYS_HEAP_USAGE_FW_PRIVATE);
	PHYS_HEAP_CONFIG sFwPrivateTempCfg;

	if (psFwPrivateHeapCfg != NULL)
	{
		PVR_LOG_RETURN_IF_FALSE((psFwCodeHeapCfg == NULL) && (psFwDataHeapCfg == NULL),
								"FW_PRIVATE and the FW_CODE & FW_PRIV_DATA usage flags "
								"achieve the same goal and are mutually exclusive.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);

		/* Fw code and data are both allocated from this unified heap */
		sFwPrivateTempCfg = *psFwPrivateHeapCfg;
		sFwPrivateTempCfg.ui32UsageFlags = PHYS_HEAP_USAGE_FW_CODE | PHYS_HEAP_USAGE_FW_PRIV_DATA;

		psFwCodeHeapCfg = &sFwPrivateTempCfg;
		psFwDataHeapCfg = &sFwPrivateTempCfg;
	}

	if ((psFwCodeHeapCfg == NULL) || (psFwDataHeapCfg == NULL))
	{
		if (psFwCodeHeapCfg != psFwDataHeapCfg)
		{
			/* Private Firmware code and data heaps must be either both defined
			 * or both undefined. There is no point in isolating one but not
			 * the other.*/
			eError = PVRSRV_ERROR_PHYSHEAP_CONFIG;
			PVR_LOG_GOTO_IF_ERROR(eError, "PrivateFwPhysHeap check", ErrorDeinit);
		}
		else
		{
			/* No dedicated heaps, default to the physheap fallback option */
		}
	}
	else if (psFwCodeHeapCfg == psFwDataHeapCfg)
	{
		if (psFwCodeHeapCfg->ui32UsageFlags ==
			(PHYS_HEAP_USAGE_FW_CODE | PHYS_HEAP_USAGE_FW_PRIV_DATA))
		{
			/* Fw code and private data allocations come from the same system heap
			 * Instantiate one physheap and share it between them. */

			eError = PhysHeapCreateHeapFromConfig(psDeviceNode,
												  psFwCodeHeapCfg,
												  NULL);
			PVR_LOG_RETURN_IF_ERROR(eError, "PhysHeapCreateHeapFromConfig");
		}
		else
		{
			/* Not an exclusive heap, can be used for other purposes (e.g. secure buffers).
			 * Expect the PVR layer to have already created a heap for the other uses. */
		}
	}
	else
	{
		/*
		 * Separating private Firmware code and data is allowed for backwards compatibility
		 * purposes. New platforms should use the unified FW_PRIVATE heap instead.
		 *
		 * Early security implementations on Rogue cores required separate FW_PRIV_DATA
		 * and FW_CODE heaps, as access permissions to Firmware were granted differently
		 * based on the transaction types (code or data).
		 */
		PVR_LOG_RETURN_IF_FALSE((psFwCodeHeapCfg->ui32UsageFlags == PHYS_HEAP_USAGE_FW_CODE) &&
								(psFwDataHeapCfg->ui32UsageFlags == PHYS_HEAP_USAGE_FW_PRIV_DATA),
								"Dedicated private heaps for Fw code and "
								"data must have one usage flag exclusively.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);

		/* Dedicated Fw code heap */
		eError = PhysHeapCreateHeapFromConfig(psDeviceNode,
											  psFwCodeHeapCfg,
											  NULL);
		PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeap");

		/* Dedicated Fw private data heap */
		eError = PhysHeapCreateHeapFromConfig(psDeviceNode,
											  psFwDataHeapCfg,
											  NULL);
		PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemCreateHeap");
	}

#if defined(RGX_PREMAP_FW_HEAPS) && defined(SUPPORT_TRUSTED_DEVICE)
	/* When premapping distinct private and shared Firmware phys heaps
	 * inside the same virtual devmem heap, their sizes must add up to
	 * the fixed RGX_FIRMWARE_RAW_HEAP_SIZE for the premapping to work */
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PHYS_HEAP_CONFIG *psFwSharedHeapCfg = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																		PHYS_HEAP_USAGE_FW_SHARED);
		IMG_UINT64 ui64FwCodeHeapSize = PhysHeapConfigGetSize(psFwCodeHeapCfg);
		IMG_UINT64 ui64FwDataHeapSize = PhysHeapConfigGetSize(psFwDataHeapCfg);
		IMG_UINT64 ui64FwSharedHeapSize = PhysHeapConfigGetSize(psFwSharedHeapCfg);
		IMG_UINT64 ui64FwPrivateHeapSize;

		PVR_LOG_GOTO_IF_FALSE((psFwCodeHeapCfg != NULL) && (psFwDataHeapCfg != NULL),
							  "Security support requires Fw code and data memory be"
							  " separate from the heap shared with the kernel driver.", FailDeinit);

		if (psFwCodeHeapCfg != psFwDataHeapCfg)
		{
			/* Private Firmware allocations come from 2 different heaps */
			ui64FwPrivateHeapSize = ui64FwCodeHeapSize + ui64FwDataHeapSize;
		}
		else
		{
			/* Private Firmware allocations come from a single heap */
			ui64FwPrivateHeapSize = ui64FwCodeHeapSize;
		}

				PVR_LOG_GOTO_IF_FALSE((ui64FwSharedHeapSize +
									   ui64FwPrivateHeapSize) ==
									   RGX_FIRMWARE_RAW_HEAP_SIZE,
									   "Invalid firmware physical heap size.", FailDeinit);
	}
#endif

	eError = PhysHeapAcquireByID(PVRSRV_PHYS_HEAP_FW_CODE, psDeviceNode,
								 &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_CODE]);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapAcquire:FW_CODE", ErrorDeinit);

	eError = PhysHeapAcquireByID(PVRSRV_PHYS_HEAP_FW_PRIV_DATA, psDeviceNode,
								 &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PRIV_DATA]);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapAcquire:FW_DATA", ErrorDeinit);

	return eError;

#if defined(RGX_PREMAP_FW_HEAPS) && defined(SUPPORT_TRUSTED_DEVICE)
FailDeinit:
    eError = PVRSRV_ERROR_INVALID_PARAMS;
#endif
ErrorDeinit:
	PVR_ASSERT(IMG_FALSE);

	return eError;
}

static PVRSRV_ERROR RGXInitFwPageTableHeap(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

#if defined(RGX_PREMAP_FW_HEAPS)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		PHYS_HEAP_CONFIG *psFwPageTableHeapCfg = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																		  PHYS_HEAP_USAGE_FW_PREMAP_PT);

		PVR_LOG_RETURN_IF_FALSE((psFwPageTableHeapCfg != NULL),
								"The Firmware Page Table phys heap config not found.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);


		PVR_LOG_RETURN_IF_FALSE((psFwPageTableHeapCfg->ui32UsageFlags == PHYS_HEAP_USAGE_FW_PREMAP_PT),
								"The Firmware Page Table heap must be used exclusively for this purpose",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);

		PVR_LOG_RETURN_IF_FALSE((psFwPageTableHeapCfg->eType == PHYS_HEAP_TYPE_LMA) ||
								(psFwPageTableHeapCfg->eType == PHYS_HEAP_TYPE_DMA),
								"The Firmware Page Table heap must be LMA or DMA memory.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);

		PVR_LOG_RETURN_IF_FALSE((psFwPageTableHeapCfg->uConfig.sLMA.uiSize >= RGX_FIRMWARE_MAX_PAGETABLE_SIZE),
								"The Firmware Page Table heap must be able to hold the maximum "
								"number of pagetables needed to cover the Firmware's VA space.",
								PVRSRV_ERROR_PHYSHEAP_CONFIG);

		eError = PhysHeapCreateHeapFromConfig(psDeviceNode,
											  psFwPageTableHeapCfg,
											  &psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PREMAP_PT]);
		PVR_LOG_RETURN_IF_ERROR(eError, "PhysHeapCreateHeapFromConfig:FwPageTableHeap");

		eError = PhysHeapAcquire(psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PREMAP_PT]);
		PVR_LOG_RETURN_IF_ERROR(eError, "PhysHeapAcquire:FwPageTableHeap");
	}
#else
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
#endif /* defined(RGX_PREMAP_FW_HEAPS) */

	return eError;
}

static PVRSRV_ERROR RGXPhysMemDeviceHeapsInit(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	eError = RGXInitFwPageTableHeap(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXInitFwPageTableHeap", ErrorDeinit);
	eError = RGXInitSharedFwPhysHeaps(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXInitSharedFwPhysHeaps", ErrorDeinit);
	eError = RGXInitPrivateFwPhysHeaps(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXInitPrivateFwPhysHeaps", ErrorDeinit);

ErrorDeinit:
	return eError;
}

/*************************************************************************/ /*!
@Function       RGXDeviceFWMainHeapMemCheck
@Description    Checks the free memory in FW Main PhysHeap of a device to ensure
                there is enough for a connection to be made.

@Input          psDeviceNode    The device of the FW Main PhysHeap to be checked.

@Return         On success PVRSRV_OK, else a PVRSRV_ERROR code.
*/ /**************************************************************************/
static PVRSRV_ERROR RGXDeviceFWMainHeapMemCheck(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PHYS_HEAP *psFWMainPhysHeap;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDeviceNode, "psDeviceNode");

	psFWMainPhysHeap = psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_MAIN];
	if (psFWMainPhysHeap == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to get device's FW Main PhysHeap"));
		return PVRSRV_ERROR_INVALID_HEAP;
	}

	if (PhysHeapGetType(psFWMainPhysHeap) == PHYS_HEAP_TYPE_LMA)
	{
		const IMG_UINT32 ui32MinMemInKBs = RGX_FW_PHYSHEAP_MINMEM_ON_CONNECTION;
		IMG_UINT64 ui64FreePhysHeapMem;

		eError = PhysHeapFreeMemCheck(psFWMainPhysHeap,
		                              KB2B(ui32MinMemInKBs),
		                              &ui64FreePhysHeapMem);

		if (eError == PVRSRV_ERROR_INSUFFICIENT_PHYS_HEAP_MEMORY)
		{
			PVR_DPF((PVR_DBG_ERROR, "FW_MAIN PhysHeap contains less than the "
				"minimum free space required to acquire a connection. "
				"Free space: %"IMG_UINT64_FMTSPEC"KB "
				"Minimum required: %uKB",
				B2KB(ui64FreePhysHeapMem),
				ui32MinMemInKBs));
		}
	}

	return eError;
}

PVRSRV_ERROR RGXGetNon4KHeapPageShift(const void *hPrivate, IMG_UINT32 *pui32Log2Non4KPgShift)
{
	IMG_UINT32 ui32GeneralNon4KHeapPageSize;
	IMG_UINT32 uiLog2OSPageShift = OSGetPageShift();

	/* We support Non4K pages only on platforms with 4KB pages. On all platforms
	 * where OS pages are larger than 4KB we must ensure the non4K device memory
	 * heap matches the page size used in all other device memory heaps, which
	 * is the OS page size, see RGXHeapDerivePageSize. */
	if (uiLog2OSPageShift > RGX_HEAP_4KB_PAGE_SHIFT)
	{
		*pui32Log2Non4KPgShift = RGXHeapDerivePageSize(uiLog2OSPageShift);
	}
	else
	{
		void *pvAppHintState = NULL;
		IMG_UINT32 ui32AppHintDefault = PVRSRV_APPHINT_GENERALNON4KHEAPPAGESIZE;
		IMG_UINT32 ui32Log2Non4KPgShift;

		/* Get the page size for the dummy page from the NON4K heap apphint */
		OSCreateAppHintState(&pvAppHintState);
		OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState,
		                   GeneralNon4KHeapPageSize, &ui32AppHintDefault,
		                   &ui32GeneralNon4KHeapPageSize);
		OSFreeAppHintState(pvAppHintState);

		/* Validate the specified parameter to be one of the supported values */
		ui32Log2Non4KPgShift = RGXHeapDerivePageSize(ExactLog2(ui32GeneralNon4KHeapPageSize));
		if (ui32AppHintDefault != ui32GeneralNon4KHeapPageSize)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Invalid Non4K Page-size, default=%u, requested=%u,"
			         " actual=%u, pageshift 0x%x",
			         __func__, ui32AppHintDefault, ui32GeneralNon4KHeapPageSize,
			         (1U << ui32Log2Non4KPgShift), ui32Log2Non4KPgShift));
		}

		if (ui32Log2Non4KPgShift == 0U)
		{
			return PVRSRV_ERROR_INVALID_NON4K_HEAP_PAGESIZE;
		}

		*pui32Log2Non4KPgShift = ui32Log2Non4KPgShift;
	}

#if defined(FIX_HW_BRN_71317_BIT_MASK)
	if (RGX_DEVICE_HAS_BRN(hPrivate, 71317))
	{
		if (*pui32Log2Non4KPgShift == RGX_HEAP_2MB_PAGE_SHIFT
		   || *pui32Log2Non4KPgShift == RGX_HEAP_1MB_PAGE_SHIFT)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "Page sizes of 2MB or 1MB cause page faults."));
			return PVRSRV_ERROR_INVALID_NON4K_HEAP_PAGESIZE;
		}
	}
#else
	PVR_UNREFERENCED_PARAMETER(hPrivate);
#endif

	/* Check the Non4k page size is at least the size of the OS page size
	 * or larger. The Non4k page size also has to be a multiple of the OS page
	 * size but since we have the log2 value from the apphint we know powers of 2
	 * will always be multiples. If the Non4k page size is less than OS page size
	 * we notify and upgrade the size.
	 */
	if (*pui32Log2Non4KPgShift < uiLog2OSPageShift)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "Non4K page size smaller than OS page size, upgrading to "
		                          "match OS page size."));
		*pui32Log2Non4KPgShift = uiLog2OSPageShift;
	}

	return PVRSRV_OK;
}

/* RGXRegisterDevice
 *
 * WARNING!
 *
 * No PDUMP statements are allowed until device initialisation starts.
 */
PVRSRV_ERROR RGXRegisterDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;
	DEVICE_MEMORY_INFO *psDevMemoryInfo;
	PVRSRV_RGXDEV_INFO	*psDevInfo;
	void *pvAppHintState = NULL;
	IMG_UINT32 ui32AppHintDefault = HWPERF_HOST_TL_STREAM_SIZE_DEFAULT, ui32HWPerfHostBufSizeKB;

	OSCreateAppHintState(&pvAppHintState);
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, HWPerfHostBufSizeInKB,
	                     &ui32AppHintDefault, &ui32HWPerfHostBufSizeKB);
	OSFreeAppHintState(pvAppHintState);
	pvAppHintState = NULL;

	/*********************
	 * Device node setup *
	 *********************/
	/* Setup static data and callbacks on the device agnostic device node */
#if defined(PDUMP)
	psDeviceNode->sDevId.pszPDumpRegName	= RGX_PDUMPREG_NAME;
	psDeviceNode->sDevId.pszPDumpDevName	= PhysHeapPDumpMemspaceName(psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_GPU_LOCAL]);
	psDeviceNode->pfnPDumpInitDevice = &RGXResetPDump;
#endif /* PDUMP */

	OSAtomicWrite(&psDeviceNode->eHealthStatus, PVRSRV_DEVICE_HEALTH_STATUS_OK);
	OSAtomicWrite(&psDeviceNode->eHealthReason, PVRSRV_DEVICE_HEALTH_REASON_NONE);

	/* Configure MMU specific stuff */
	RGXMMUInit_Register(psDeviceNode);

	psDeviceNode->pfnInvalFBSCTable = NULL;

	psDeviceNode->pfnValidateOrTweakPhysAddrs = NULL;

	/* Callback for getting the MMU device attributes */
	psDeviceNode->pfnGetMMUDeviceAttributes = RGXDevMMUAttributes;
	psDeviceNode->pfnGetDeviceSnoopMode = RGXDevSnoopMode;
	psDeviceNode->pfnMMUCacheInvalidate = RGXMMUCacheInvalidate;
	psDeviceNode->pfnMMUCacheInvalidateKick = RGXMMUCacheInvalidateKick;
#if defined(RGX_BRN71422_TARGET_HARDWARE_PHYSICAL_ADDR)
	psDeviceNode->pfnMMUTopLevelPxWorkarounds = RGXMapBRN71422TargetPhysicalAddress;
#else
	psDeviceNode->pfnMMUTopLevelPxWorkarounds = NULL;
#endif
	/* pfnMMUTweakProtFlags is set later on once BVNC features are setup */
	psDeviceNode->pfnMMUTweakProtFlags = NULL;

	psDeviceNode->pfnInitDeviceCompatCheck	= &RGXDevInitCompatCheck;

	/* Register callbacks for creation of device memory contexts */
	psDeviceNode->pfnRegisterMemoryContext = RGXRegisterMemoryContext;
	psDeviceNode->pfnUnregisterMemoryContext = RGXUnregisterMemoryContext;

	/* Register callbacks for Unified Fence Objects */
	psDeviceNode->pfnAllocUFOBlock = RGXAllocUFOBlock;
	psDeviceNode->pfnFreeUFOBlock = RGXFreeUFOBlock;

	/* Register callback for checking the device's health */
	psDeviceNode->pfnUpdateHealthStatus = PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) ? NULL : RGXUpdateHealthStatus;

#if defined(SUPPORT_AUTOVZ)
	/* Register callback for updating the virtualization watchdog */
	psDeviceNode->pfnUpdateAutoVzWatchdog = RGXUpdateAutoVzWatchdog;
#endif

	/* Register method to service the FW HWPerf buffer */
	psDeviceNode->pfnServiceHWPerf = RGXHWPerfDataStoreCB;

	/* Register callback for getting the device version information string */
	psDeviceNode->pfnDeviceVersionString = RGXDevVersionString;

	/* Register callback for getting the device clock speed */
	psDeviceNode->pfnDeviceClockSpeed = RGXDevClockSpeed;

	/* Register callback for soft resetting some device modules */
	psDeviceNode->pfnSoftReset = RGXSoftReset;

	/* Register callback for resetting the HWR logs */
	psDeviceNode->pfnResetHWRLogs = RGXResetHWRLogs;

	/* Register callback for resetting the HWR logs */
	psDeviceNode->pfnVerifyBVNC = RGXVerifyBVNC;

	/* Register callback for checking alignment of UM structures */
	psDeviceNode->pfnAlignmentCheck = RGXAlignmentCheck;

	/*Register callback for checking the supported features and getting the
	 * corresponding values */
	psDeviceNode->pfnCheckDeviceFeature = RGXBvncCheckFeatureSupported;
	psDeviceNode->pfnGetDeviceFeatureValue = RGXBvncGetSupportedFeatureValue;

	/* Callback for checking if system layer supports FBC 3.1 */
	psDeviceNode->pfnHasFBCDCVersion31 = RGXSystemHasFBCDCVersion31;

	/* Callback for getting TFBC configuration */
	psDeviceNode->pfnGetTFBCLossyGroup = RGXGetTFBCLossyGroup;

	/* Register callback for initialising device-specific physical memory heaps */
	psDeviceNode->pfnPhysMemDeviceHeapsInit = RGXPhysMemDeviceHeapsInit;

	/* Register callback for checking a device's FW Main physical heap for sufficient free memory */
	psDeviceNode->pfnCheckForSufficientFWPhysMem = RGXDeviceFWMainHeapMemCheck;

	/* Register callback for determining the appropriate LMA allocation policy for a phys heap */
	psDeviceNode->pfnPhysHeapGetLMAPolicy = RGXPhysHeapGetLMAPolicy;

	/*********************
	 * Device info setup *
	 *********************/
	/* Allocate device control block */
	psDevInfo = OSAllocZMem(sizeof(*psDevInfo));
	if (psDevInfo == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "DevInitRGXPart1 : Failed to alloc memory for DevInfo"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* initialise the layer parameters needed for early hw feature checks */
	psDevInfo->sLayerParams.psDevInfo = psDevInfo;
	psDevInfo->sLayerParams.psDevConfig = psDeviceNode->psDevConfig;

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	/* Default psTrampoline to point to null struct */
	psDevInfo->psTrampoline = (RGX_MIPS_ADDRESS_TRAMPOLINE *)&sNullTrampoline;
#endif

	/* create locks for the context lists stored in the DevInfo structure.
	 * these lists are modified on context create/destroy and read by the
	 * watchdog thread
	 */

	eError = OSWRLockCreate(&(psDevInfo->hRenderCtxListLock));
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create render context list lock", __func__));
		goto e0;
	}

	eError = OSWRLockCreate(&(psDevInfo->hComputeCtxListLock));
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create compute context list lock", __func__));
		goto e1;
	}

	eError = OSWRLockCreate(&(psDevInfo->hTransferCtxListLock));
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create transfer context list lock", __func__));
		goto e2;
	}

	eError = OSWRLockCreate(&(psDevInfo->hTDMCtxListLock));
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create TDM context list lock", __func__));
		goto e3;
	}

	eError = OSWRLockCreate(&(psDevInfo->hKickSyncCtxListLock));
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create kick sync context list lock", __func__));
		goto e4;
	}

	eError = OSWRLockCreate(&(psDevInfo->hMemoryCtxListLock));
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create memory context list lock", __func__));
		goto e5;
	}

	eError = OSSpinLockCreate(&psDevInfo->hLockKCCBDeferredCommandsList);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to KCCB deferred commands list lock", __func__));
		goto e6;
	}
	dllist_init(&(psDevInfo->sKCCBDeferredCommandsListHead));

	dllist_init(&(psDevInfo->sRenderCtxtListHead));
	dllist_init(&(psDevInfo->sComputeCtxtListHead));
	dllist_init(&(psDevInfo->sTransferCtxtListHead));
	dllist_init(&(psDevInfo->sTDMCtxtListHead));
	dllist_init(&(psDevInfo->sKickSyncCtxtListHead));

	dllist_init(&(psDevInfo->sCommonCtxtListHead));
	psDevInfo->ui32CommonCtxtCurrentID = 1;


	eError = OSWRLockCreate(&psDevInfo->hCommonCtxtListLock);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create common context list lock", __func__));
		goto e7;
	}

	eError = OSLockCreate(&psDevInfo->sRegConfig.hLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create register configuration lock", __func__));
		goto e8;
	}

	eError = OSLockCreate(&psDevInfo->hBPLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create lock for break points", __func__));
		goto e9;
	}

	eError = OSLockCreate(&psDevInfo->hRGXFWIfBufInitLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create lock for trace buffers", __func__));
		goto e10;
	}

	eError = OSLockCreate(&psDevInfo->hCCBStallCheckLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create stalled CCB checking lock", __func__));
		goto e11;
	}
	eError = OSLockCreate(&psDevInfo->hCCBRecoveryLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create stalled CCB recovery lock", __func__));
		goto e12;
	}

	eError = OSLockCreate(&psDevInfo->hGpuUtilStatsLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create GPU stats lock", __func__));
		goto e13;
	}

	dllist_init(&psDevInfo->sMemoryContextList);

	/* initialise ui32SLRHoldoffCounter */
	if (RGX_INITIAL_SLR_HOLDOFF_PERIOD_MS > DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT)
	{
		psDevInfo->ui32SLRHoldoffCounter = RGX_INITIAL_SLR_HOLDOFF_PERIOD_MS / DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT;
	}
	else
	{
		psDevInfo->ui32SLRHoldoffCounter = 0;
	}

	/* Setup static data and callbacks on the device specific device info */
	psDevInfo->psDeviceNode		= psDeviceNode;

	psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
	psDevInfo->pvDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

	/*
	 * Map RGX Registers
	 */
	psDevInfo->ui32RegSize = psDeviceNode->psDevConfig->ui32RegsSize;
	psDevInfo->sRegsPhysBase = psDeviceNode->psDevConfig->sRegsCpuPBase;

#if !defined(NO_HARDWARE)
	psDevInfo->pvRegsBaseKM = (void __iomem *) OSMapPhysToLin(psDeviceNode->psDevConfig->sRegsCpuPBase,
			psDeviceNode->psDevConfig->ui32RegsSize,
			PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);

	if (psDevInfo->pvRegsBaseKM == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to create RGX register mapping",
		         __func__));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto e14;
	}
#endif /* !NO_HARDWARE */

	psDeviceNode->pvDevice = psDevInfo;

	eError = RGXBvncInitialiseConfiguration(psDeviceNode);
	if (PVRSRV_OK != eError)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Unsupported HW device detected by driver",
		         __func__));
		goto e15;
	}

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	/*
	 * We must now setup the SECURITY mappings if supported. We cannot
	 * check on the features until we have reached here as the BVNC is
	 * not setup before now.
	 */
#if !defined(NO_HARDWARE)
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, HOST_SECURITY_VERSION) &&
		(RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1))
	{
		IMG_CPU_PHYADDR sHostSecureRegBankBase = {psDeviceNode->psDevConfig->sRegsCpuPBase.uiAddr + RGX_HOST_SECURE_REGBANK_OFFSET};

		psDevInfo->pvSecureRegsBaseKM = (void __iomem *) OSMapPhysToLin(sHostSecureRegBankBase,
																		RGX_HOST_SECURE_REGBANK_SIZE,
																		PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);

		if (psDevInfo->pvSecureRegsBaseKM == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "PVRSRVRGXInitDevPart2KM: Failed to create RGX secure register mapping"));
			eError = PVRSRV_ERROR_BAD_MAPPING;
			goto e15;
		}

		/*
		 * The secure register bank is mapped into the CPU VA space starting from
		 * the base of the normal register bank + an offset of RGX_HOST_SECURE_REGBAK_OFFSET.
		 * The hardware register addresses are all indexed from the base of the regular register bank.
		 * For the RegBankBase+RegOffset computation to still be accurate for host-secure registers,
		 * we need to compensate for offsets of registers in the secure bank
		 */
		psDevInfo->pvSecureRegsBaseKM = (void __iomem *)((uintptr_t)psDevInfo->pvSecureRegsBaseKM - RGX_HOST_SECURE_REGBANK_OFFSET);
	}
	else
	{
		psDevInfo->pvSecureRegsBaseKM = psDevInfo->pvRegsBaseKM;
	}
#endif /* !NO_HARDWARE */
#endif /* defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX) */

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	psDeviceNode->pfnMMUTweakProtFlags = (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS)) ?
			RGXMMUTweakProtFlags : NULL;
#endif

	eError = RGXGetNon4KHeapPageShift(&psDevInfo->sLayerParams,
	                                &psDeviceNode->ui32Non4KPageSizeLog2);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXGetNon4KHeapPageSize", e16);

	eError = RGXInitHeaps(psDevInfo, psDevMemoryInfo);
	if (eError != PVRSRV_OK)
	{
		goto e16;
	}

	eError = RGXHWPerfInit(psDevInfo);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXHWPerfInit", e16);

	eError = RGXHWPerfHostInit(psDeviceNode->pvDevice, ui32HWPerfHostBufSizeKB);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXHWPerfHostInit", ErrorDeInitHWPerfFw);


	/* Register callback for dumping debug info */
	eError = RGXDebugInit(psDevInfo);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXDebugInit", e17);

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	/* Register callback for fw mmu init */
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		psDeviceNode->pfnFwMMUInit = RGXMipsMMUInit_Register;
	}
#endif

	if (NULL != psDeviceNode->psDevConfig->pfnSysDevFeatureDepInit)
	{
		psDeviceNode->psDevConfig->pfnSysDevFeatureDepInit(psDeviceNode->psDevConfig,
				psDevInfo->sDevFeatureCfg.ui64Features);
	}

	/* Initialise the device dependent bridges */
	eError = RGXRegisterBridges(psDevInfo);
	PVR_LOG_IF_ERROR(eError, "RGXRegisterBridges");

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
	eError = OSLockCreate(&psDevInfo->hCounterDumpingLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create lock for counter sampling.", __func__));
		goto ErrorDeInitDeviceDepBridge;
	}
#endif

	/* Initialise error counters */
	memset(&psDevInfo->sErrorCounts, 0, sizeof(PVRSRV_RGXDEV_ERROR_COUNTS));

	return PVRSRV_OK;

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
ErrorDeInitDeviceDepBridge:
	RGXUnregisterBridges(psDevInfo);
#endif

e17:
	RGXHWPerfHostDeInit(psDevInfo);
ErrorDeInitHWPerfFw:
	RGXHWPerfDeinit(psDevInfo);
e16:
#if !defined(NO_HARDWARE)
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (psDevInfo->pvSecureRegsBaseKM != NULL)
	{
		/* Adjust pvSecureRegsBaseKM if device has SECURITY_VERSION > 1 */
		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, HOST_SECURITY_VERSION) &&
		    (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1))
		{
			/* Undo the VA offset adjustment to unmap correct VAddr */
			psDevInfo->pvSecureRegsBaseKM = (void __iomem *)((uintptr_t)psDevInfo->pvSecureRegsBaseKM + RGX_HOST_SECURE_REGBANK_OFFSET);
			OSUnMapPhysToLin((void __force *) psDevInfo->pvSecureRegsBaseKM,
			                 RGX_HOST_SECURE_REGBANK_SIZE);
		}
	}
#endif
#endif /* !NO_HARDWARE */
e15:
#if !defined(NO_HARDWARE)
	if (psDevInfo->pvRegsBaseKM != NULL)
	{
		OSUnMapPhysToLin((void __force *) psDevInfo->pvRegsBaseKM,
		                 psDevInfo->ui32RegSize);
	}
e14:
#endif /* !NO_HARDWARE */
	OSLockDestroy(psDevInfo->hGpuUtilStatsLock);
e13:
	OSLockDestroy(psDevInfo->hCCBRecoveryLock);
e12:
	OSLockDestroy(psDevInfo->hCCBStallCheckLock);
e11:
	OSLockDestroy(psDevInfo->hRGXFWIfBufInitLock);
e10:
	OSLockDestroy(psDevInfo->hBPLock);
e9:
	OSLockDestroy(psDevInfo->sRegConfig.hLock);
e8:
	OSWRLockDestroy(psDevInfo->hCommonCtxtListLock);
e7:
	OSSpinLockDestroy(psDevInfo->hLockKCCBDeferredCommandsList);
e6:
	OSWRLockDestroy(psDevInfo->hMemoryCtxListLock);
e5:
	OSWRLockDestroy(psDevInfo->hKickSyncCtxListLock);
e4:
	OSWRLockDestroy(psDevInfo->hTDMCtxListLock);
e3:
	OSWRLockDestroy(psDevInfo->hTransferCtxListLock);
e2:
	OSWRLockDestroy(psDevInfo->hComputeCtxListLock);
e1:
	OSWRLockDestroy(psDevInfo->hRenderCtxListLock);
e0:
	OSFreeMem(psDevInfo);

	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

IMG_PCHAR RGXDevBVNCString(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_PCHAR psz = psDevInfo->sDevFeatureCfg.pszBVNCString;
	if (NULL == psz)
	{
		IMG_CHAR pszBVNCInfo[RGX_HWPERF_MAX_BVNC_LEN];
		size_t uiBVNCStringSize;
		size_t uiStringLength;

		uiStringLength = OSSNPrintf(pszBVNCInfo, RGX_HWPERF_MAX_BVNC_LEN, "%d.%d.%d.%d",
				psDevInfo->sDevFeatureCfg.ui32B,
				psDevInfo->sDevFeatureCfg.ui32V,
				psDevInfo->sDevFeatureCfg.ui32N,
				psDevInfo->sDevFeatureCfg.ui32C);
		PVR_ASSERT(uiStringLength < RGX_HWPERF_MAX_BVNC_LEN);

		uiBVNCStringSize = (uiStringLength + 1) * sizeof(IMG_CHAR);
		psz = OSAllocMem(uiBVNCStringSize);
		if (NULL != psz)
		{
			OSCachedMemCopy(psz, pszBVNCInfo, uiBVNCStringSize);
			psDevInfo->sDevFeatureCfg.pszBVNCString = psz;
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE,
					"%s: Allocating memory for BVNC Info string failed",
					__func__));
		}
	}

	return psz;
}

/*************************************************************************/ /*!
@Function       RGXDevVersionString
@Description    Gets the version string for the given device node and returns
                a pointer to it in ppszVersionString. It is then the
                responsibility of the caller to free this memory.
@Input          psDeviceNode            Device node from which to obtain the
                                        version string
@Output	        ppszVersionString	Contains the version string upon return
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
static PVRSRV_ERROR RGXDevVersionString(PVRSRV_DEVICE_NODE *psDeviceNode,
                                        IMG_CHAR **ppszVersionString)
{
#if defined(NO_HARDWARE) || defined(EMULATOR)
	const IMG_CHAR szFormatString[] = "GPU variant BVNC: %s (SW)";
#else
	const IMG_CHAR szFormatString[] = "GPU variant BVNC: %s (HW)";
#endif
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_PCHAR pszBVNC;
	size_t uiStringLength;

	if (psDeviceNode == NULL || ppszVersionString == NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;
	pszBVNC = RGXDevBVNCString(psDevInfo);

	if (NULL == pszBVNC)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	uiStringLength = OSStringLength(pszBVNC);
	uiStringLength += (sizeof(szFormatString) - 2); /* sizeof includes the null, -2 for "%s" */
	*ppszVersionString = OSAllocMem(uiStringLength * sizeof(IMG_CHAR));
	if (*ppszVersionString == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	OSSNPrintf(*ppszVersionString, uiStringLength, szFormatString,
		pszBVNC);

	return PVRSRV_OK;
}

/**************************************************************************/ /*!
@Function       RGXDevClockSpeed
@Description    Gets the clock speed for the given device node and returns
                it in pui32RGXClockSpeed.
@Input          psDeviceNode		Device node
@Output         pui32RGXClockSpeed  Variable for storing the clock speed
@Return         PVRSRV_ERROR
*/ /***************************************************************************/
static PVRSRV_ERROR RGXDevClockSpeed(PVRSRV_DEVICE_NODE *psDeviceNode,
                                     IMG_PUINT32  pui32RGXClockSpeed)
{
	RGX_DATA *psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;

	/* get clock speed */
	*pui32RGXClockSpeed = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;

	return PVRSRV_OK;
}

#if defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1)
/*!
 *******************************************************************************

 @Function		RGXInitFwRawHeap

 @Description	Called to perform additional initialisation
 ******************************************************************************/
static PVRSRV_ERROR RGXInitFwRawHeap(PVRSRV_RGXDEV_INFO *psDevInfo, DEVMEM_HEAP_BLUEPRINT *psDevMemHeap, IMG_UINT32 ui32DriverID)
{
	IMG_UINT32 uiStringLength;
	IMG_UINT32 uiStringLengthMax = 32;

	IMG_UINT32 ui32Log2RgxDefaultPageShift = RGXHeapDerivePageSize(OSGetPageShift());

	PVR_RETURN_IF_FALSE(ui32Log2RgxDefaultPageShift != 0, PVRSRV_ERROR_INVALID_PARAMS);

#if defined(FIX_HW_BRN_71317_BIT_MASK)
	if (RGX_IS_BRN_SUPPORTED(psDevInfo, 71317))
	{
		if (ui32Log2RgxDefaultPageShift == RGX_HEAP_2MB_PAGE_SHIFT
		   || ui32Log2RgxDefaultPageShift == RGX_HEAP_1MB_PAGE_SHIFT)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "OS page size too large for device virtual heaps. "
			         "Maximum page size supported is 256KB when BRN71317 is present."));
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}
#endif

	uiStringLength = MIN(sizeof(RGX_FIRMWARE_GUEST_RAW_HEAP_IDENT), uiStringLengthMax + 1);

	/* Start by allocating memory for this DriverID heap identification string */
	psDevMemHeap->pszName = OSAllocMem(uiStringLength * sizeof(IMG_CHAR));
	if (psDevMemHeap->pszName == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Append the DriverID number to the RGX_FIRMWARE_GUEST_RAW_HEAP_IDENT string */
	OSSNPrintf((IMG_CHAR *)psDevMemHeap->pszName, uiStringLength, RGX_FIRMWARE_GUEST_RAW_HEAP_IDENT, ui32DriverID);

	/* Use the common blueprint template support function to initialise the heap */
	HeapCfgBlueprintInit(psDevMemHeap->pszName,
		                 RGX_FIRMWARE_RAW_HEAP_BASE + (ui32DriverID * RGX_FIRMWARE_RAW_HEAP_SIZE),
		                 RGX_FIRMWARE_RAW_HEAP_SIZE,
		                 0,
		                 ui32Log2RgxDefaultPageShift,
		                 0,
		                 NULL,
		                 NULL,
		                 psDevMemHeap);

	return PVRSRV_OK;
}

/*!
 *******************************************************************************

 @Function		RGXDeInitFwRawHeap

 @Description	Called to perform additional deinitialisation
 ******************************************************************************/
static void RGXDeInitFwRawHeap(DEVMEM_HEAP_BLUEPRINT *psDevMemHeap)
{
	IMG_UINT64 uiBase = RGX_FIRMWARE_RAW_HEAP_BASE + RGX_FIRMWARE_RAW_HEAP_SIZE;
	IMG_UINT64 uiSpan = uiBase + ((RGX_NUM_DRIVERS_SUPPORTED - 1) * RGX_FIRMWARE_RAW_HEAP_SIZE);

	/* Safe to do as the guest firmware heaps are last in the list */
	if (psDevMemHeap->sHeapBaseAddr.uiAddr >= uiBase &&
	    psDevMemHeap->sHeapBaseAddr.uiAddr < uiSpan)
	{
		void *pszName = (void*)psDevMemHeap->pszName;
		OSFreeMem(pszName);
	}
}
#endif /* defined(RGX_PREMAP_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED > 1) */

/******************************************************************************
 End of file (rgxinit.c)
******************************************************************************/

/*************************************************************************/ /*!
@File
@Title          RGX firmware common context utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware common context utility routines
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

#include "rgxfwcmnctx.h"
#include "rgxfwutils.h"
#include "devicemem_pdump.h"
#if defined(__linux__) && defined(PVRSRV_TRACE_ROGUE_EVENTS)
#include "rogue_trace_events.h"
#endif

#if defined(__linux__) && defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
#include "linux/cred.h"
#include "linux/pid.h"
#endif
/*
 * Maximum length of time a DM can run for before the DM will be marked
 * as out-of-time. CDM has an increased value due to longer running kernels.
 *
 * These deadlines are increased on FPGA, EMU and VP due to the slower
 * execution time of these platforms. PDUMPS are also included since they
 * are often run on EMU, FPGA or in CSim.
 */
#if defined(FPGA) || defined(EMULATOR) || defined(VIRTUAL_PLATFORM) || defined(PDUMP)
#define RGXFWIF_MAX_WORKLOAD_DEADLINE_MS     (480000)
#define RGXFWIF_MAX_CDM_WORKLOAD_DEADLINE_MS (10800000)
#else
#define RGXFWIF_MAX_WORKLOAD_DEADLINE_MS     (40000)
#define RGXFWIF_MAX_CDM_WORKLOAD_DEADLINE_MS (600000)
#endif
#define RGXFWIF_MAX_RDM_WORKLOAD_DEADLINE_MS (36000000)

struct _RGX_SERVER_COMMON_CONTEXT_ {
	PVRSRV_RGXDEV_INFO *psDevInfo;
	DEVMEM_MEMDESC *psFWCommonContextMemDesc;
	PRGXFWIF_FWCOMMONCONTEXT sFWCommonContextFWAddr;
	SERVER_MMU_CONTEXT *psServerMMUContext;
	DEVMEM_MEMDESC *psFWMemContextMemDesc;
	DEVMEM_MEMDESC *psFWFrameworkMemDesc;
	DEVMEM_MEMDESC *psContextStateMemDesc;
	RGX_CLIENT_CCB *psClientCCB;
	DEVMEM_MEMDESC *psClientCCBMemDesc;
	DEVMEM_MEMDESC *psClientCCBCtrlMemDesc;
	IMG_BOOL bCommonContextMemProvided;
	IMG_UINT32 ui32ContextID;
	DLLIST_NODE sListNode;
	RGX_CONTEXT_RESET_REASON eLastResetReason;
	IMG_UINT32 ui32LastResetJobRef;
	IMG_INT32 i32Priority;
	RGX_CCB_REQUESTOR_TYPE eRequestor;
};

#if defined(__linux__) && defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
static IMG_UINT32 _GetUID(IMG_PID pid)
{
	struct task_struct *psTask;
	struct pid *psPid;

	psPid = find_get_pid((pid_t)pid);
	if (!psPid)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to lookup PID %u.",
		                        __func__, pid));
		return 0;
	}

	psTask = get_pid_task(psPid, PIDTYPE_PID);
	if (!psTask)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get pid task for PID %u.",
		                        __func__, pid));
	}
	put_pid(psPid);

	return psTask ? from_kuid(&init_user_ns, psTask->cred->uid) : 0;
}
#endif

/*************************************************************************/ /*!
@Function       _CheckPriority
@Description    Check if priority is allowed for requestor type
@Input          psDevInfo    pointer to DevInfo struct
@Input          i32Priority Requested priority
@Input          eRequestor   Requestor type specifying data master
@Return         PVRSRV_ERROR PVRSRV_OK on success
*/ /**************************************************************************/
static PVRSRV_ERROR _CheckPriority(PVRSRV_RGXDEV_INFO *psDevInfo,
								   IMG_INT32 i32Priority,
								   RGX_CCB_REQUESTOR_TYPE eRequestor)
{
	/* Only contexts from a single PID allowed with real time priority (highest priority) */
	if (i32Priority == RGX_CTX_PRIORITY_REALTIME)
	{
		DLLIST_NODE *psNode, *psNext;

		dllist_foreach_node(&psDevInfo->sCommonCtxtListHead, psNode, psNext)
		{
			RGX_SERVER_COMMON_CONTEXT *psThisContext =
				IMG_CONTAINER_OF(psNode, RGX_SERVER_COMMON_CONTEXT, sListNode);

			if (psThisContext->i32Priority == RGX_CTX_PRIORITY_REALTIME &&
				psThisContext->eRequestor == eRequestor &&
				RGXGetPIDFromServerMMUContext(psThisContext->psServerMMUContext) != OSGetCurrentClientProcessIDKM())
			{
				PVR_LOG(("Only one process can have contexts with real time priority"));
				return PVRSRV_ERROR_INVALID_PARAMS;
			}
		}
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR FWCommonContextAllocate(CONNECTION_DATA *psConnection,
									 PVRSRV_DEVICE_NODE *psDeviceNode,
									 RGX_CCB_REQUESTOR_TYPE eRGXCCBRequestor,
									 RGXFWIF_DM eDM,
									 SERVER_MMU_CONTEXT *psServerMMUContext,
									 DEVMEM_MEMDESC *psAllocatedMemDesc,
									 IMG_UINT32 ui32AllocatedOffset,
									 DEVMEM_MEMDESC *psFWMemContextMemDesc,
									 DEVMEM_MEMDESC *psContextStateMemDesc,
									 IMG_UINT32 ui32CCBAllocSizeLog2,
									 IMG_UINT32 ui32CCBMaxAllocSizeLog2,
									 IMG_UINT32 ui32ContextFlags,
									 IMG_INT32 i32Priority,
									 IMG_UINT32 ui32MaxDeadlineMS,
									 IMG_UINT64 ui64RobustnessAddress,
									 RGX_COMMON_CONTEXT_INFO *psInfo,
									 RGX_SERVER_COMMON_CONTEXT **ppsServerCommonContext)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	RGX_SERVER_COMMON_CONTEXT *psServerCommonContext;
	RGXFWIF_FWCOMMONCONTEXT sFWCommonContext = {{0}};
	IMG_UINT32 ui32FWCommonContextOffset;
	IMG_UINT8 *pui8Ptr;
	PVRSRV_ERROR eError;

	/*
	 * Allocate all the resources that are required
	 */
	psServerCommonContext = OSAllocMem(sizeof(*psServerCommonContext));
	if (psServerCommonContext == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_alloc;
	}

	psServerCommonContext->psDevInfo = psDevInfo;
	psServerCommonContext->psServerMMUContext = psServerMMUContext;

	if (psAllocatedMemDesc)
	{
		PDUMPCOMMENT(psDeviceNode,
					 "Using existing MemDesc for Rogue firmware %s context (offset = %d)",
					 aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT],
					 ui32AllocatedOffset);
		ui32FWCommonContextOffset = ui32AllocatedOffset;
		psServerCommonContext->psFWCommonContextMemDesc = psAllocatedMemDesc;
		psServerCommonContext->bCommonContextMemProvided = IMG_TRUE;
	}
	else
	{
		/* Allocate device memory for the firmware context */
		PDUMPCOMMENT(psDeviceNode,
					 "Allocate Rogue firmware %s context", aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT]);
		eError = DevmemFwAllocate(psDevInfo,
								sizeof(sFWCommonContext),
								RGX_FWCOMCTX_ALLOCFLAGS,
								"FwContext",
								&psServerCommonContext->psFWCommonContextMemDesc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Failed to allocate firmware %s context (%s)",
			         __func__,
			         aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT],
			         PVRSRVGetErrorString(eError)));
			goto fail_contextalloc;
		}
		ui32FWCommonContextOffset = 0;
		psServerCommonContext->bCommonContextMemProvided = IMG_FALSE;
	}

	/* Record this context so we can refer to it if the FW needs to tell us it was reset. */
	psServerCommonContext->eLastResetReason    = RGX_CONTEXT_RESET_REASON_NONE;
	psServerCommonContext->ui32LastResetJobRef = 0;
	psServerCommonContext->ui32ContextID       = psDevInfo->ui32CommonCtxtCurrentID++;

	/*
	 * Temporarily map the firmware context to the kernel and initialise it
	 */
	eError = DevmemAcquireCpuVirtAddr(psServerCommonContext->psFWCommonContextMemDesc,
	                                  (void **)&pui8Ptr);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to map firmware %s context to CPU (%s)",
		         __func__,
		         aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT],
		         PVRSRVGetErrorString(eError)));
		goto fail_cpuvirtacquire;
	}

	/* Allocate the client CCB */
	eError = RGXCreateCCB(psDevInfo,
						  ui32CCBAllocSizeLog2,
						  ui32CCBMaxAllocSizeLog2,
						  ui32ContextFlags,
						  psConnection,
						  eRGXCCBRequestor,
						  psServerCommonContext,
						  &psServerCommonContext->psClientCCB,
						  &psServerCommonContext->psClientCCBMemDesc,
						  &psServerCommonContext->psClientCCBCtrlMemDesc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: failed to create CCB for %s context (%s)",
		         __func__,
		         aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT],
		         PVRSRVGetErrorString(eError)));
		goto fail_allocateccb;
	}

	sFWCommonContext.eDM = eDM;
	BITMASK_SET(sFWCommonContext.ui32CompatFlags, RGXFWIF_CONTEXT_COMPAT_FLAGS_HAS_DEFER_COUNT);

	/* Set the firmware CCB device addresses in the firmware common context */
	eError = RGXSetFirmwareAddress(&sFWCommonContext.psCCB,
						  psServerCommonContext->psClientCCBMemDesc,
						  0, RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:1", fail_cccbfwaddr);

	eError = RGXSetFirmwareAddress(&sFWCommonContext.psCCBCtl,
						  psServerCommonContext->psClientCCBCtrlMemDesc,
						  0, RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:2", fail_cccbctrlfwaddr);

#if defined(RGX_FEATURE_META_DMA_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, META_DMA))
	{
		RGXSetMetaDMAAddress(&sFWCommonContext.sCCBMetaDMAAddr,
							 psServerCommonContext->psClientCCBMemDesc,
							 &sFWCommonContext.psCCB,
							 0);
	}
#endif

	/* Set the memory context device address */
	psServerCommonContext->psFWMemContextMemDesc = psFWMemContextMemDesc;
	eError = RGXSetFirmwareAddress(&sFWCommonContext.psFWMemContext,
						  psFWMemContextMemDesc,
						  0, RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:3", fail_fwmemctxfwaddr);

	/* Set the framework register updates address */
	psServerCommonContext->psFWFrameworkMemDesc = psInfo->psFWFrameworkMemDesc;
	if (psInfo->psFWFrameworkMemDesc != NULL)
	{
		eError = RGXSetFirmwareAddress(&sFWCommonContext.psRFCmd,
				psInfo->psFWFrameworkMemDesc,
				0, RFW_FWADDR_FLAG_NONE);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:4", fail_fwframeworkfwaddr);
	}
	else
	{
		/* This should never be touched in this contexts without a framework
		 * memdesc, but ensure it is zero so we see crashes if it is.
		 */
		sFWCommonContext.psRFCmd.ui32Addr = 0;
	}

	eError = _CheckPriority(psDevInfo, i32Priority, eRGXCCBRequestor);
	PVR_LOG_GOTO_IF_ERROR(eError, "_CheckPriority", fail_checkpriority);

	psServerCommonContext->i32Priority = i32Priority;
	psServerCommonContext->eRequestor = eRGXCCBRequestor;

	sFWCommonContext.i32Priority = i32Priority;
	sFWCommonContext.ui32PrioritySeqNum = 0;

	if (eDM == RGXFWIF_DM_CDM)
	{
		sFWCommonContext.ui32MaxDeadlineMS = MIN(ui32MaxDeadlineMS, RGXFWIF_MAX_CDM_WORKLOAD_DEADLINE_MS);
	}
	else if (eDM == RGXFWIF_DM_RAY)
	{
		sFWCommonContext.ui32MaxDeadlineMS = MIN(ui32MaxDeadlineMS, RGXFWIF_MAX_RDM_WORKLOAD_DEADLINE_MS);
	}
	else
	{
		sFWCommonContext.ui32MaxDeadlineMS = MIN(ui32MaxDeadlineMS, RGXFWIF_MAX_WORKLOAD_DEADLINE_MS);
	}

	sFWCommonContext.ui64RobustnessAddress = ui64RobustnessAddress;

	/* Store a references to Server Common Context and PID for notifications back from the FW. */
	sFWCommonContext.ui32ServerCommonContextID = psServerCommonContext->ui32ContextID;
	sFWCommonContext.ui32PID                   = OSGetCurrentClientProcessIDKM();
#if defined(__linux__) && defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
	sFWCommonContext.ui32UID                   = _GetUID(sFWCommonContext.ui32PID);
#endif
	OSStringSafeCopy(sFWCommonContext.szProcName, psConnection->pszProcName, RGXFW_PROCESS_NAME_LEN);

	/* Set the firmware GPU context state buffer */
	psServerCommonContext->psContextStateMemDesc = psContextStateMemDesc;
	if (psContextStateMemDesc)
	{
		eError = RGXSetFirmwareAddress(&sFWCommonContext.psContextState,
							  psContextStateMemDesc,
							  0,
							  RFW_FWADDR_FLAG_NONE);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:5", fail_ctxstatefwaddr);
	}

	OSCachedMemCopy(IMG_OFFSET_ADDR(pui8Ptr, ui32FWCommonContextOffset), &sFWCommonContext, sizeof(sFWCommonContext));
	RGXFwSharedMemCacheOpExec(IMG_OFFSET_ADDR(pui8Ptr, ui32FWCommonContextOffset),
	                          sizeof(sFWCommonContext),
	                          PVRSRV_CACHE_OP_FLUSH);

	/*
	 * Dump the created context
	 */
	PDUMPCOMMENT(psDeviceNode,
				 "Dump %s context", aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT]);
	DevmemPDumpLoadMem(psServerCommonContext->psFWCommonContextMemDesc,
					   ui32FWCommonContextOffset,
					   sizeof(sFWCommonContext),
					   PDUMP_FLAGS_CONTINUOUS);

	/* We've finished the setup so release the CPU mapping */
	DevmemReleaseCpuVirtAddr(psServerCommonContext->psFWCommonContextMemDesc);

	/* Map this allocation into the FW */
	eError = RGXSetFirmwareAddress(&psServerCommonContext->sFWCommonContextFWAddr,
						  psServerCommonContext->psFWCommonContextMemDesc,
						  ui32FWCommonContextOffset,
						  RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:6", fail_fwcommonctxfwaddr);

#if defined(__linux__) && defined(PVRSRV_TRACE_ROGUE_EVENTS)
	{
		IMG_UINT32 ui32FWAddr;
		switch (eDM) {
			case RGXFWIF_DM_GEOM:
				ui32FWAddr = (IMG_UINT32) ((uintptr_t) IMG_CONTAINER_OF((void *) ((uintptr_t)
						psServerCommonContext->sFWCommonContextFWAddr.ui32Addr), RGXFWIF_FWRENDERCONTEXT, sTAContext));
				break;
			case RGXFWIF_DM_3D:
				ui32FWAddr = (IMG_UINT32) ((uintptr_t) IMG_CONTAINER_OF((void *) ((uintptr_t)
						psServerCommonContext->sFWCommonContextFWAddr.ui32Addr), RGXFWIF_FWRENDERCONTEXT, s3DContext));
				break;
			default:
				ui32FWAddr = psServerCommonContext->sFWCommonContextFWAddr.ui32Addr;
				break;
		}

		trace_rogue_create_fw_context(OSGetCurrentClientProcessNameKM(),
									  aszCCBRequestors[eRGXCCBRequestor][REQ_PDUMP_COMMENT],
									  psDeviceNode->sDevId.ui32InternalID,
									  ui32FWAddr);
	}
#endif

	/*Add the node to the list when finalised */
	OSWRLockAcquireWrite(psDevInfo->hCommonCtxtListLock);
	dllist_add_to_tail(&(psDevInfo->sCommonCtxtListHead), &(psServerCommonContext->sListNode));
	OSWRLockReleaseWrite(psDevInfo->hCommonCtxtListLock);

	*ppsServerCommonContext = psServerCommonContext;
	return PVRSRV_OK;

fail_fwcommonctxfwaddr:
	if (psContextStateMemDesc)
	{
		RGXUnsetFirmwareAddress(psContextStateMemDesc);
	}
fail_ctxstatefwaddr:
fail_checkpriority:
	if (psInfo->psFWFrameworkMemDesc != NULL)
	{
		RGXUnsetFirmwareAddress(psInfo->psFWFrameworkMemDesc);
	}
fail_fwframeworkfwaddr:
	RGXUnsetFirmwareAddress(psFWMemContextMemDesc);
fail_fwmemctxfwaddr:
	RGXUnsetFirmwareAddress(psServerCommonContext->psClientCCBCtrlMemDesc);
fail_cccbctrlfwaddr:
	RGXUnsetFirmwareAddress(psServerCommonContext->psClientCCBMemDesc);
fail_cccbfwaddr:
	RGXDestroyCCB(psDevInfo, psServerCommonContext->psClientCCB);
fail_allocateccb:
	DevmemReleaseCpuVirtAddr(psServerCommonContext->psFWCommonContextMemDesc);
fail_cpuvirtacquire:
	if (!psServerCommonContext->bCommonContextMemProvided)
	{
		DevmemFwUnmapAndFree(psDevInfo, psServerCommonContext->psFWCommonContextMemDesc);
		psServerCommonContext->psFWCommonContextMemDesc = NULL;
	}
fail_contextalloc:
	OSFreeMem(psServerCommonContext);
fail_alloc:
	return eError;
}

void FWCommonContextFree(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext)
{

	OSWRLockAcquireWrite(psServerCommonContext->psDevInfo->hCommonCtxtListLock);
	/* Remove the context from the list of all contexts. */
	dllist_remove_node(&psServerCommonContext->sListNode);
	OSWRLockReleaseWrite(psServerCommonContext->psDevInfo->hCommonCtxtListLock);

	/*
		Unmap the context itself and then all its resources
	*/

	/* Unmap the FW common context */
	RGXUnsetFirmwareAddress(psServerCommonContext->psFWCommonContextMemDesc);
	/* Umap context state buffer (if there was one) */
	if (psServerCommonContext->psContextStateMemDesc)
	{
		RGXUnsetFirmwareAddress(psServerCommonContext->psContextStateMemDesc);
	}
	/* Unmap the framework buffer */
	if (psServerCommonContext->psFWFrameworkMemDesc != NULL)
	{
		RGXUnsetFirmwareAddress(psServerCommonContext->psFWFrameworkMemDesc);
	}
	/* Unmap client CCB and CCB control */
	RGXUnsetFirmwareAddress(psServerCommonContext->psClientCCBCtrlMemDesc);
	RGXUnsetFirmwareAddress(psServerCommonContext->psClientCCBMemDesc);
	/* Unmap the memory context */
	RGXUnsetFirmwareAddress(psServerCommonContext->psFWMemContextMemDesc);

	/* Destroy the client CCB */
	RGXDestroyCCB(psServerCommonContext->psDevInfo, psServerCommonContext->psClientCCB);


	/* Free the FW common context (if there was one) */
	if (!psServerCommonContext->bCommonContextMemProvided)
	{
		DevmemFwUnmapAndFree(psServerCommonContext->psDevInfo,
						psServerCommonContext->psFWCommonContextMemDesc);
		psServerCommonContext->psFWCommonContextMemDesc = NULL;
	}
	/* Free the hosts representation of the common context */
	OSFreeMem(psServerCommonContext);
}

PRGXFWIF_FWCOMMONCONTEXT FWCommonContextGetFWAddress(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext)
{
	return psServerCommonContext->sFWCommonContextFWAddr;
}

RGX_CLIENT_CCB *FWCommonContextGetClientCCB(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext)
{
	return psServerCommonContext->psClientCCB;
}

SERVER_MMU_CONTEXT *FWCommonContextGetServerMMUCtx(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext)
{
	return psServerCommonContext->psServerMMUContext;
}

RGX_CONTEXT_RESET_REASON FWCommonContextGetLastResetReason(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext,
                                                               IMG_UINT32 *pui32LastResetJobRef)
{
	RGX_CONTEXT_RESET_REASON eLastResetReason;

	PVR_ASSERT(psServerCommonContext != NULL);
	PVR_ASSERT(pui32LastResetJobRef != NULL);

	/* Take the most recent reason & job ref and reset for next time... */
	eLastResetReason      = psServerCommonContext->eLastResetReason;
	*pui32LastResetJobRef = psServerCommonContext->ui32LastResetJobRef;
	psServerCommonContext->eLastResetReason = RGX_CONTEXT_RESET_REASON_NONE;
	psServerCommonContext->ui32LastResetJobRef = 0;

	if (eLastResetReason == RGX_CONTEXT_RESET_REASON_HARD_CONTEXT_SWITCH)
	{
		PVR_DPF((PVR_DBG_WARNING,
		         "A Hard Context Switch was triggered on the GPU to ensure Quality of Service."));
	}

	return eLastResetReason;
}

PVRSRV_RGXDEV_INFO* FWCommonContextGetRGXDevInfo(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext)
{
	return psServerCommonContext->psDevInfo;
}

PVRSRV_ERROR RGXGetFWCommonContextAddrFromServerMMUCtx(PVRSRV_RGXDEV_INFO *psDevInfo,
													   SERVER_MMU_CONTEXT *psServerMMUContext,
													   PRGXFWIF_FWCOMMONCONTEXT *psFWCommonContextFWAddr)
{
	DLLIST_NODE *psNode, *psNext;
	dllist_foreach_node(&psDevInfo->sCommonCtxtListHead, psNode, psNext)
	{
		RGX_SERVER_COMMON_CONTEXT *psThisContext =
			IMG_CONTAINER_OF(psNode, RGX_SERVER_COMMON_CONTEXT, sListNode);

		if (psThisContext->psServerMMUContext == psServerMMUContext)
		{
			psFWCommonContextFWAddr->ui32Addr = psThisContext->sFWCommonContextFWAddr.ui32Addr;
			return PVRSRV_OK;
		}
	}
	return PVRSRV_ERROR_INVALID_PARAMS;
}

PRGXFWIF_FWCOMMONCONTEXT RGXGetFWCommonContextAddrFromServerCommonCtx(PVRSRV_RGXDEV_INFO *psDevInfo,
													                  DLLIST_NODE *psNode)
{
	RGX_SERVER_COMMON_CONTEXT *psThisContext =
			IMG_CONTAINER_OF(psNode, RGX_SERVER_COMMON_CONTEXT, sListNode);

	return FWCommonContextGetFWAddress(psThisContext);
}

PVRSRV_ERROR FWCommonContextSetFlags(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext,
                                     IMG_UINT32 ui32ContextFlags)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (BITMASK_ANY(ui32ContextFlags, ~RGX_CONTEXT_FLAGS_WRITEABLE_MASK))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Context flag(s) invalid or not writeable (%d)",
		         __func__, ui32ContextFlags));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
	}
	else
	{
		RGXSetCCBFlags(psServerCommonContext->psClientCCB,
		               ui32ContextFlags);
	}

	return eError;
}

PVRSRV_ERROR ContextSetPriority(RGX_SERVER_COMMON_CONTEXT *psContext,
								CONNECTION_DATA *psConnection,
								PVRSRV_RGXDEV_INFO *psDevInfo,
								IMG_INT32 i32Priority,
								RGXFWIF_DM eDM)
{
	IMG_UINT32				ui32CmdSize;
	IMG_UINT8				*pui8CmdPtr;
	RGXFWIF_KCCB_CMD		sPriorityCmd = { 0 };
	RGXFWIF_CCB_CMD_HEADER	*psCmdHeader;
	RGXFWIF_CMD_PRIORITY	*psCmd;
	PVRSRV_ERROR			eError;
	RGX_CLIENT_CCB *psClientCCB = FWCommonContextGetClientCCB(psContext);

	PVR_UNREFERENCED_PARAMETER(psConnection);

	eError = _CheckPriority(psDevInfo, i32Priority, psContext->eRequestor);
	PVR_LOG_GOTO_IF_ERROR(eError, "_CheckPriority", fail_checkpriority);

	/*
		Get space for command
	*/
	ui32CmdSize = RGX_CCB_FWALLOC_ALIGN(sizeof(RGXFWIF_CCB_CMD_HEADER) + sizeof(RGXFWIF_CMD_PRIORITY));

	eError = RGXAcquireCCB(psClientCCB,
						   ui32CmdSize,
						   (void **) &pui8CmdPtr,
						   PDUMP_FLAGS_CONTINUOUS);
	if (eError != PVRSRV_OK)
	{
		if (eError != PVRSRV_ERROR_RETRY)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to acquire space for client CCB", __func__));
		}
		goto fail_ccbacquire;
	}

	/*
		Write the command header and command
	*/
	psCmdHeader = IMG_OFFSET_ADDR(pui8CmdPtr, 0);
	psCmdHeader->eCmdType = RGXFWIF_CCB_CMD_TYPE_PRIORITY;
	psCmdHeader->ui32CmdSize = RGX_CCB_FWALLOC_ALIGN(sizeof(RGXFWIF_CMD_PRIORITY));
	pui8CmdPtr += sizeof(*psCmdHeader);

	psCmd = IMG_OFFSET_ADDR(pui8CmdPtr, 0);
	psCmd->i32Priority = i32Priority;
	pui8CmdPtr += sizeof(*psCmd);

	/*
		We should reserve space in the kernel CCB here and fill in the command
		directly.
		This is so if there isn't space in the kernel CCB we can return with
		retry back to services client before we take any operations
	*/

	/*
		Submit the command
	*/
	RGXReleaseCCB(psClientCCB,
				  ui32CmdSize,
				  PDUMP_FLAGS_CONTINUOUS);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to release space in client CCB", __func__));
		return eError;
	}

	/* Construct the priority command. */
	sPriorityCmd.eCmdType = RGXFWIF_KCCB_CMD_KICK;
	sPriorityCmd.uCmdData.sCmdKickData.psContext = FWCommonContextGetFWAddress(psContext);
	sPriorityCmd.uCmdData.sCmdKickData.ui32CWoffUpdate = RGXGetHostWriteOffsetCCB(psClientCCB);
	sPriorityCmd.uCmdData.sCmdKickData.ui32CWrapMaskUpdate = RGXGetWrapMaskCCB(psClientCCB);
	sPriorityCmd.uCmdData.sCmdKickData.ui32NumCleanupCtl = 0;

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	sPriorityCmd.uCmdData.sCmdKickData.ui32WorkEstCmdHeaderOffset = 0;
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
									eDM,
									&sPriorityCmd,
									PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to submit set priority command with error (%u)",
				__func__,
				eError));
		goto fail_cmdacquire;
	}

	psContext->i32Priority = i32Priority;

	return PVRSRV_OK;

fail_ccbacquire:
fail_checkpriority:
fail_cmdacquire:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

PVRSRV_ERROR CheckStalledClientCommonContext(RGX_SERVER_COMMON_CONTEXT *psCurrentServerCommonContext, RGX_KICK_TYPE_DM eKickTypeDM)
{
	if (psCurrentServerCommonContext == NULL)
	{
		/* the context has already been freed so there is nothing to do here */
		return PVRSRV_OK;
	}

	return CheckForStalledCCB(psCurrentServerCommonContext->psDevInfo->psDeviceNode,
	                          psCurrentServerCommonContext->psClientCCB,
	                          eKickTypeDM);
}

void DumpFWCommonContextInfo(RGX_SERVER_COMMON_CONTEXT *psCurrentServerCommonContext,
                             DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                             void *pvDumpDebugFile,
                             IMG_UINT32 ui32VerbLevel)
{
	if (psCurrentServerCommonContext == NULL)
	{
		/* the context has already been freed so there is nothing to do here */
		return;
	}

	if (DD_VERB_LVL_ENABLED(ui32VerbLevel, DEBUG_REQUEST_VERBOSITY_HIGH))
	{
		/* If high verbosity requested, dump whole CCB */
		DumpCCB(psCurrentServerCommonContext->psDevInfo,
		        psCurrentServerCommonContext->sFWCommonContextFWAddr,
		        psCurrentServerCommonContext->psClientCCB,
		        pfnDumpDebugPrintf,
		        pvDumpDebugFile);
	}
	else
	{
		/* Otherwise, only dump first command in the CCB */
		DumpFirstCCBCmd(psCurrentServerCommonContext->sFWCommonContextFWAddr,
		                      psCurrentServerCommonContext->psClientCCB,
		                      pfnDumpDebugPrintf,
		                      pvDumpDebugFile);
	}
}

void FWCommonContextListSetLastResetReason(PVRSRV_RGXDEV_INFO *psDevInfo,
                                           IMG_UINT32 *pui32ErrorPid,
                                           const RGXFWIF_FWCCB_CMD_CONTEXT_RESET_DATA *psCmdContextResetNotification)
{
	DLLIST_NODE *psNode, *psNext;
	RGX_SERVER_COMMON_CONTEXT *psServerCommonContext = NULL;
	IMG_UINT32 ui32ErrorPid = 0;

	OSWRLockAcquireRead(psDevInfo->hCommonCtxtListLock);

	dllist_foreach_node(&psDevInfo->sCommonCtxtListHead, psNode, psNext)
	{
		RGX_SERVER_COMMON_CONTEXT *psThisContext =
		    IMG_CONTAINER_OF(psNode, RGX_SERVER_COMMON_CONTEXT, sListNode);

		/* If the notification applies to all contexts update reset info
		* for all contexts, otherwise only do so for the appropriate ID.
		*/
		if (psCmdContextResetNotification->ui32Flags & RGXFWIF_FWCCB_CMD_CONTEXT_RESET_FLAG_ALL_CTXS)
		{
			/* Notification applies to all contexts */
			psThisContext->eLastResetReason    = psCmdContextResetNotification->eResetReason;
			psThisContext->ui32LastResetJobRef = psCmdContextResetNotification->ui32ResetJobRef;
		}
		else
		{
			/* Notification applies to one context only */
			if (psThisContext->ui32ContextID == psCmdContextResetNotification->ui32ServerCommonContextID)
			{
				psServerCommonContext = psThisContext;
				psServerCommonContext->eLastResetReason    = psCmdContextResetNotification->eResetReason;
				psServerCommonContext->ui32LastResetJobRef = psCmdContextResetNotification->ui32ResetJobRef;
				ui32ErrorPid = RGXGetPIDFromServerMMUContext(psServerCommonContext->psServerMMUContext);
				break;
			}
		}
	}

	OSWRLockReleaseRead(psDevInfo->hCommonCtxtListLock);

	if (psCmdContextResetNotification->ui32Flags & RGXFWIF_FWCCB_CMD_CONTEXT_RESET_FLAG_ALL_CTXS)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: All contexts reset (Reason=%d, JobRef=0x%08x)",
				__func__,
				(IMG_UINT32)(psCmdContextResetNotification->eResetReason),
				psCmdContextResetNotification->ui32ResetJobRef));
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Context 0x%p reset (ID=0x%08x, Reason=%d, JobRef=0x%08x)",
				__func__,
				psServerCommonContext,
				psCmdContextResetNotification->ui32ServerCommonContextID,
				(IMG_UINT32)(psCmdContextResetNotification->eResetReason),
				psCmdContextResetNotification->ui32ResetJobRef));
	}

	if (pui32ErrorPid)
	{
		*pui32ErrorPid = ui32ErrorPid;
	}
}

/******************************************************************************
 End of file (rgxfwcmnctx.c)
******************************************************************************/

/*************************************************************************/ /*!
@Title          Services kernel module internal header file
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

#ifndef SRVKM_H
#define SRVKM_H

#include "servicesint.h"

#if defined(__cplusplus)
extern "C" {
#endif

	/**	Use PVR_DPF() unless message is necessary in release build
	 */
	#ifdef PVR_DISABLE_LOGGING
	#define PVR_LOG(X)
	#else
	/* PRQA S 3410 1 */ /* this macro requires no brackets in order to work */
	#define PVR_LOG(X)			PVRSRVReleasePrintf X;
	#endif

	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVReleasePrintf(const IMG_CHAR *pszFormat, ...) IMG_FORMAT_PRINTF(1, 2);

	IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcessConnect(IMG_UINT32	ui32PID, IMG_UINT32 ui32Flags);
	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVProcessDisconnect(IMG_UINT32	ui32PID);

	IMG_IMPORT IMG_VOID PVRSRVScheduleDevicesKM(IMG_VOID);

#if defined(SUPPORT_PVRSRV_DEVICE_CLASS)
	IMG_VOID IMG_CALLCONV PVRSRVSetDCState(IMG_UINT32 ui32State);
#endif

	PVRSRV_ERROR IMG_CALLCONV PVRSRVSaveRestoreLiveSegments(IMG_HANDLE hArena, IMG_PBYTE pbyBuffer, IMG_SIZE_T *puiBufSize, IMG_BOOL bSave);

	IMG_VOID PVRSRVScheduleDeviceCallbacks(IMG_VOID);

	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDumpSyncs(IMG_BOOL bActiveOnly);

#define SYNC_OP_CLASS_MASK			0x0000ffffUL
#define SYNC_OP_CLASS_SHIFT			0
#define SYNC_OP_CLASS_MODOBJ		(1<<0)
#define SYNC_OP_CLASS_QUEUE			(1<<1)
#define SYNC_OP_CLASS_KICKTA		(1<<2)
#define SYNC_OP_CLASS_TQ_3D			(1<<3)
#define SYNC_OP_CLASS_TQ_2D			(1<<4)

#define SYNC_OP_TYPE_MASK			0x00f0000UL
#define SYNC_OP_TYPE_SHIFT			16
#define SYNC_OP_TYPE_READOP			(1<<0)
#define SYNC_OP_TYPE_WRITEOP		(1<<1)
#define SYNC_OP_TYPE_READOP2		(1<<2)

#define SYNC_OP_HAS_DATA			0x80000000UL
#define SYNC_OP_TAKE				0x40000000UL
#define SYNC_OP_ROLLBACK			0x20000000UL

#ifdef INLINE_IS_PRAGMA
#pragma inline(SyncTakeWriteOp)
#endif
static INLINE
IMG_UINT32 SyncTakeWriteOp(PVRSRV_KERNEL_SYNC_INFO *psKernelSync, IMG_UINT32 ui32OpType)
{
#if defined(SUPPORT_PER_SYNC_DEBUG)
	IMG_UINT32 ui32Index = psKernelSync->ui32HistoryIndex;

	/* Record a history of all the classes of operation taken on this sync */
	psKernelSync->ui32OperationMask |= (ui32OpType & SYNC_OP_CLASS_MASK) >> SYNC_OP_CLASS_SHIFT;

	/* Add this operation to the history buffer */
	psKernelSync->aui32OpInfo[ui32Index] = SYNC_OP_HAS_DATA | ui32OpType | (SYNC_OP_TYPE_WRITEOP << SYNC_OP_TYPE_SHIFT) | SYNC_OP_TAKE;
	psKernelSync->aui32ReadOpSample[ui32Index] = psKernelSync->psSyncData->ui32ReadOpsPending;
	psKernelSync->aui32WriteOpSample[ui32Index] = psKernelSync->psSyncData->ui32WriteOpsPending;
	psKernelSync->aui32ReadOp2Sample[ui32Index] = psKernelSync->psSyncData->ui32ReadOps2Pending;
	psKernelSync->ui32HistoryIndex++;
	psKernelSync->ui32HistoryIndex = psKernelSync->ui32HistoryIndex % PER_SYNC_HISTORY;
#endif
	PVR_UNREFERENCED_PARAMETER(ui32OpType);
	return psKernelSync->psSyncData->ui32WriteOpsPending++;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SyncTakeReadOp)
#endif
static INLINE
IMG_UINT32 SyncTakeReadOp(PVRSRV_KERNEL_SYNC_INFO *psKernelSync, IMG_UINT32 ui32OpType)
{
#if defined(SUPPORT_PER_SYNC_DEBUG)
	IMG_UINT32 ui32Index = psKernelSync->ui32HistoryIndex;

	/* Record a history of all the classes of operation taken on this sync */
	psKernelSync->ui32OperationMask |= (ui32OpType & SYNC_OP_CLASS_MASK) >> SYNC_OP_CLASS_SHIFT;

	/* Add this operation to the history buffer */
	psKernelSync->aui32OpInfo[ui32Index] = SYNC_OP_HAS_DATA | ui32OpType | (SYNC_OP_TYPE_READOP << SYNC_OP_TYPE_SHIFT) | SYNC_OP_TAKE;
	psKernelSync->aui32ReadOpSample[ui32Index] = psKernelSync->psSyncData->ui32ReadOpsPending;
	psKernelSync->aui32WriteOpSample[ui32Index] = psKernelSync->psSyncData->ui32WriteOpsPending;
	psKernelSync->aui32ReadOp2Sample[ui32Index] = psKernelSync->psSyncData->ui32ReadOps2Pending;
	psKernelSync->ui32HistoryIndex++;
	psKernelSync->ui32HistoryIndex = psKernelSync->ui32HistoryIndex % PER_SYNC_HISTORY;
#endif
	PVR_UNREFERENCED_PARAMETER(ui32OpType);
	return psKernelSync->psSyncData->ui32ReadOpsPending++;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SyncTakeReadOp2)
#endif
static INLINE
IMG_UINT32 SyncTakeReadOp2(PVRSRV_KERNEL_SYNC_INFO *psKernelSync, IMG_UINT32 ui32OpType)
{
#if defined(SUPPORT_PER_SYNC_DEBUG)
	IMG_UINT32 ui32Index = psKernelSync->ui32HistoryIndex;

	/* Record a history of all the classes of operation taken on this sync */
	psKernelSync->ui32OperationMask |= (ui32OpType & SYNC_OP_CLASS_MASK) >> SYNC_OP_CLASS_SHIFT;

	/* Add this operation to the history buffer */
	psKernelSync->aui32OpInfo[ui32Index] = SYNC_OP_HAS_DATA | ui32OpType | (SYNC_OP_TYPE_READOP2 << SYNC_OP_TYPE_SHIFT) | SYNC_OP_TAKE;
	psKernelSync->aui32ReadOpSample[ui32Index] = psKernelSync->psSyncData->ui32ReadOpsPending;
	psKernelSync->aui32WriteOpSample[ui32Index] = psKernelSync->psSyncData->ui32WriteOpsPending;
	psKernelSync->aui32ReadOp2Sample[ui32Index] = psKernelSync->psSyncData->ui32ReadOps2Pending;
	psKernelSync->ui32HistoryIndex++;
	psKernelSync->ui32HistoryIndex = psKernelSync->ui32HistoryIndex % PER_SYNC_HISTORY;
#endif
	PVR_UNREFERENCED_PARAMETER(ui32OpType);
	return psKernelSync->psSyncData->ui32ReadOps2Pending++;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SyncRollBackWriteOp)
#endif
static INLINE
IMG_UINT32 SyncRollBackWriteOp(PVRSRV_KERNEL_SYNC_INFO *psKernelSync, IMG_UINT32 ui32OpType)
{
#if defined(SUPPORT_PER_SYNC_DEBUG)
	IMG_UINT32 ui32Index = psKernelSync->ui32HistoryIndex;

	/* Record a history of all the classes of operation taken on this sync */
	psKernelSync->ui32OperationMask |= (ui32OpType & SYNC_OP_CLASS_MASK) >> SYNC_OP_CLASS_SHIFT;

	/* Add this operation to the history buffer */
	psKernelSync->aui32OpInfo[ui32Index] = SYNC_OP_HAS_DATA | ui32OpType | (SYNC_OP_TYPE_WRITEOP << SYNC_OP_TYPE_SHIFT) | SYNC_OP_ROLLBACK;
	psKernelSync->aui32ReadOpSample[ui32Index] = psKernelSync->psSyncData->ui32ReadOpsPending;
	psKernelSync->aui32WriteOpSample[ui32Index] = psKernelSync->psSyncData->ui32WriteOpsPending;
	psKernelSync->aui32ReadOp2Sample[ui32Index] = psKernelSync->psSyncData->ui32ReadOps2Pending;
	psKernelSync->ui32HistoryIndex++;
	psKernelSync->ui32HistoryIndex = psKernelSync->ui32HistoryIndex % PER_SYNC_HISTORY;
#endif
	PVR_UNREFERENCED_PARAMETER(ui32OpType);
	return psKernelSync->psSyncData->ui32WriteOpsPending--;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SyncRollBackReadOp)
#endif
static INLINE
IMG_UINT32 SyncRollBackReadOp(PVRSRV_KERNEL_SYNC_INFO *psKernelSync, IMG_UINT32 ui32OpType)
{
#if defined(SUPPORT_PER_SYNC_DEBUG)
	IMG_UINT32 ui32Index = psKernelSync->ui32HistoryIndex;

	/* Record a history of all the classes of operation taken on this sync */
	psKernelSync->ui32OperationMask |= (ui32OpType & SYNC_OP_CLASS_MASK) >> SYNC_OP_CLASS_SHIFT;

	/* Add this operation to the history buffer */
	psKernelSync->aui32OpInfo[ui32Index] = SYNC_OP_HAS_DATA | ui32OpType | (SYNC_OP_TYPE_READOP << SYNC_OP_TYPE_SHIFT) | SYNC_OP_ROLLBACK;
	psKernelSync->aui32ReadOpSample[ui32Index] = psKernelSync->psSyncData->ui32ReadOpsPending;
	psKernelSync->aui32WriteOpSample[ui32Index] = psKernelSync->psSyncData->ui32WriteOpsPending;
	psKernelSync->aui32ReadOp2Sample[ui32Index] = psKernelSync->psSyncData->ui32ReadOps2Pending;
	psKernelSync->ui32HistoryIndex++;
	psKernelSync->ui32HistoryIndex = psKernelSync->ui32HistoryIndex % PER_SYNC_HISTORY;
#endif
	PVR_UNREFERENCED_PARAMETER(ui32OpType);
	return psKernelSync->psSyncData->ui32ReadOpsPending--;
}



#if defined (__cplusplus)
}
#endif

/******************
HIGHER LEVEL MACROS
*******************/

/*----------------------------------------------------------------------------
Repeats the body of the loop for a certain minimum time, or until the body
exits by its own means (break, return, goto, etc.)

Example of usage:

LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
{
	if(psQueueInfo->ui32ReadOffset == psQueueInfo->ui32WriteOffset)
	{
		bTimeout = IMG_FALSE;
		break;
	}
	
	OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
} END_LOOP_UNTIL_TIMEOUT();

-----------------------------------------------------------------------------*/

/*	uiNotLastLoop will remain at 1 until the timeout has expired, at which time		
 * 	it will be decremented and the loop executed one final time. This is necessary
 *	when preemption is enabled. 
 */
/* PRQA S 3411,3431 12 */ /* critical format, leave alone */
#define LOOP_UNTIL_TIMEOUT(TIMEOUT) \
{\
	IMG_UINT32 uiOffset, uiStart, uiCurrent; \
	IMG_INT32 iNotLastLoop;					 \
	for(uiOffset = 0, uiStart = OSClockus(), uiCurrent = uiStart + 1, iNotLastLoop = 1;\
		((uiCurrent - uiStart + uiOffset) < (TIMEOUT)) || iNotLastLoop--;				\
		uiCurrent = OSClockus(),													\
		uiOffset = uiCurrent < uiStart ? IMG_UINT32_MAX - uiStart : uiOffset,		\
		uiStart = uiCurrent < uiStart ? 0 : uiStart)

#define END_LOOP_UNTIL_TIMEOUT() \
}

/*!
 ******************************************************************************

 @Function		PVRSRVGetErrorStringKM

 @Description	Returns a text string relating to the PVRSRV_ERROR enum.

 ******************************************************************************/
IMG_IMPORT
const IMG_CHAR *PVRSRVGetErrorStringKM(PVRSRV_ERROR eError);

#endif /* SRVKM_H */

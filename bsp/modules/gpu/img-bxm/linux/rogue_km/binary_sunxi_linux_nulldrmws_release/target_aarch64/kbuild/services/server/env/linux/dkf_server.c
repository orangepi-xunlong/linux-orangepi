/*************************************************************************/ /*!
@File         dkf_server.c
@Title        DRM Key Framework support routines.
@Copyright    Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include "dkf_server.h"
#include "img_types.h"
#include "device.h"
#include "pvrsrv_error.h"
#include "dkp_impl.h"
#include "osfunc.h"
#include "pvr_debug.h"

#if defined(SUPPORT_LINUX_FDINFO)
typedef struct DKF_REGISTERED_DKP_TAG
{
	IMG_HANDLE              hPrivHandle;    /*!< Private data - can be NULL */
	const IMG_CHAR          *pszDKPName;    /*!< DKP provider name */
	IMG_PID                 pid;            /*!< Process-ID - can be 0 */
	IMG_UINT32              uiReserved1;    /*!< Reserved field - padding */
	DKP_PFN_SHOW            *psDKPShowPfn;  /*!< DKP Callback function */
	DLLIST_NODE             sDKFEntryNode;  /*!< List of DKP entries */

	DKP_CONNECTION_FLAGS    ui32Filter;     /*!< The types of connection to output to */
} DKF_REGISTERED_DKP;

/* Global sentinel to track all allocated DKP callback key/value pairs */
typedef struct DKF_TAG
{
	POS_LOCK            hDKFListLock;   /*!< Lock for accessing sDKFListNode */
	IMG_UINT32          ui32NumEntries; /*!< Number of registered DKP entries */
	IMG_UINT32          uiReserved1;    /*!< Reserved field - padding */
	DLLIST_NODE         sDKFListNode;   /*!< Head of the DKF_REGISTERED_DKP linked list */
	DKF_VPRINTF_FUNC    *pfnPrint;  /*!< Default printf-style output fn */
	void                *pvPrintArg1; /*!< First arg for pfnPrint */
} DKF;

static DKF *gpsDKF;

static_assert(DKF_CONNECTION_FLAG_INVALID == DKP_CONNECTION_FLAG_INVALID, "DKF and DKP INVALID connection flags do not match");
static_assert(DKF_CONNECTION_FLAG_SYNC == DKP_CONNECTION_FLAG_SYNC, "DKF and DKP SYNC connection flags do not match");
static_assert(DKF_CONNECTION_FLAG_SERVICES == DKP_CONNECTION_FLAG_SERVICES, "DKF and DKP SERVICES connection flags do not match");

PVRSRV_ERROR PVRDKFInit(void)
{
	DKF *psDKF;
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;

	if (gpsDKF != NULL)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: gpsDKF = %p, NULL expected",
		        __func__, gpsDKF));

		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s called", __func__));

	psDKF = OSAllocZMemNoStats(sizeof(*psDKF));
	PVR_GOTO_IF_NOMEM(psDKF, eError, Error);

	dllist_init(&psDKF->sDKFListNode);
	eError = OSLockCreate(&psDKF->hDKFListLock);
	PVR_GOTO_IF_ERROR(eError, ErrorFree);

	gpsDKF = psDKF;

	return PVRSRV_OK;

ErrorFree:
	OSFreeMemNoStats(psDKF);
	/* fallthrough */

Error:
	PVR_DPF((PVR_DBG_ERROR, "%s: %s", __func__, PVRSRVGetErrorString(eError)));
	return eError;
}


void PVRDKFDeInit(void)
{
	IMG_UINT32 uiNumFreed = 0U;

	PVR_DPF((PVR_DBG_MESSAGE, "%s called", __func__));

	if (gpsDKF == NULL)
	{
		return;
	}

	/* Ensure we leave no data allocated. The DKP instances should have
	 * been cleaned up by their module deInit processing. Handle badly
	 * behaved clients here.
	 */
	if (gpsDKF->ui32NumEntries > 0)
	{
		DLLIST_NODE *psThis, *psNext;

		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Have %u un-freed allocations remaining", __func__,
		         gpsDKF->ui32NumEntries));

		OSLockAcquire(gpsDKF->hDKFListLock);

		dllist_foreach_node(&gpsDKF->sDKFListNode, psThis, psNext)
		{
			DKF_REGISTERED_DKP *psEntry = IMG_CONTAINER_OF(psThis,
			                                             DKF_REGISTERED_DKP,
			                                             sDKFEntryNode);

			dllist_remove_node(&psEntry->sDKFEntryNode);

			uiNumFreed++;

			OSFreeMemNoStats(psEntry);
		}

		if (uiNumFreed != gpsDKF->ui32NumEntries)
		{
			PVR_DPF((PVR_DBG_ERROR, "Could only free %u out of %u",
			         uiNumFreed, gpsDKF->ui32NumEntries));
		}

		dllist_remove_node(&gpsDKF->sDKFListNode);

		OSLockRelease(gpsDKF->hDKFListLock);
	}

	OSLockDestroy(gpsDKF->hDKFListLock);

	OSFreeMemNoStats(gpsDKF);

	gpsDKF = NULL;
}

void PVRDKFTraverse(DKF_VPRINTF_FUNC *pfnPrint,
                    void *pvArg,
                    struct _PVRSRV_DEVICE_NODE_ *psDevNode,
                    IMG_PID pid,
                    DKF_CONNECTION_FLAGS ui32ConnectionType)
{
	PDLLIST_NODE pNext, pNode;

	PVR_ASSERT(gpsDKF != NULL);
	PVR_ASSERT(pfnPrint != NULL);
	PVR_ASSERT(ui32ConnectionType != DKF_CONNECTION_FLAG_INVALID);

	OSLockAcquire(gpsDKF->hDKFListLock);

	gpsDKF->pfnPrint = pfnPrint;
	gpsDKF->pvPrintArg1 = pvArg;

	if (dllist_is_empty(&gpsDKF->sDKFListNode))
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: No DKPs registered", __func__));
	}
	else
	{
		dllist_foreach_node(&gpsDKF->sDKFListNode, pNode, pNext)
		{
			DKF_REGISTERED_DKP *psEntry = IMG_CONTAINER_OF(pNode,
			                                               DKF_REGISTERED_DKP,
			                                               sDKFEntryNode);

			if (psEntry->psDKPShowPfn != NULL &&
			    BITMASK_ANY(psEntry->ui32Filter, ui32ConnectionType))
			{
				psEntry->psDKPShowPfn(psDevNode, pid,
				                      psEntry->hPrivHandle);
			}
		}
	}

	OSLockRelease(gpsDKF->hDKFListLock);
}

/*
 * Wrapper function to display data using preconfigured DKF-specific output
 * function.
 */
void PVRDKPOutput(IMG_HANDLE hPrivData, const char *fmt, ...)
{
	DKF_REGISTERED_DKP *psDKFEntry = (DKF_REGISTERED_DKP *)hPrivData;
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	va_list Arglist;
	DLLIST_NODE *pNode, *pNext;
	IMG_BOOL bFound = IMG_FALSE;

	if (psDKFEntry == NULL || gpsDKF == NULL)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: NULL DKF entry found (%p, %p)",
		        __func__, psDKFEntry, gpsDKF));
		return;
	}

	PVR_ASSERT(gpsDKF->pfnPrint != NULL);

	/* validate that this is a legitimate output function reference */

	dllist_foreach_node(&gpsDKF->sDKFListNode, pNode, pNext)
	{
		DKF_REGISTERED_DKP *psEntry = IMG_CONTAINER_OF(pNode,
		                                               DKF_REGISTERED_DKP,
		                                               sDKFEntryNode);

		if (psEntry == psDKFEntry)
		{
			bFound = IMG_TRUE;
			break;
		}
	}

	if (!bFound)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Handle %p not found.", __func__,
		         hPrivData));
		return;
	}

	OSSNPrintf(szBuffer, PVR_MAX_DEBUG_MESSAGE_LEN, "%s", fmt);

	va_start(Arglist, fmt);
	gpsDKF->pfnPrint(gpsDKF->pvPrintArg1, szBuffer, &Arglist);
	va_end(Arglist);
}

PVRSRV_ERROR PVRSRVRegisterDKP(IMG_HANDLE hPrivData,
                               const char *pszDKPName,
                               DKP_PFN_SHOW *psShowPfn,
                               DKP_CONNECTION_FLAGS ui32Filter,
                               PPVRDKF_DKP_HANDLE phDkpHandle)
{
	DKF_REGISTERED_DKP *psDKFEntry; /* New entry for this DKP */
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;

	/* Check for a NULL argument. Nothing to allocate if we are not provided
	 * a location to store the DkpHandle reference.
	 */
	if (phDkpHandle == NULL || ui32Filter == DKF_CONNECTION_FLAG_INVALID)
	{

#if defined(DEBUG)
		PVR_DPF((PVR_DBG_WARNING, "%s(%p, %s, %p, %p) Called", __func__,
		         hPrivData, pszDKPName, psShowPfn,
		         phDkpHandle));
#endif

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDKFEntry = OSAllocZMemNoStats(sizeof(*psDKFEntry));
	PVR_GOTO_IF_NOMEM(psDKFEntry, eError, Error);

	OSLockAcquire(gpsDKF->hDKFListLock);

	psDKFEntry->hPrivHandle = hPrivData;
	psDKFEntry->pszDKPName = pszDKPName;
	psDKFEntry->psDKPShowPfn = psShowPfn;
	psDKFEntry->ui32Filter = ui32Filter;
	dllist_init(&psDKFEntry->sDKFEntryNode);

	/*
	 * Append the new entry to the end of the gpsDKF list-head. Return a
	 * reference to the new entry to the caller.
	 */
	dllist_add_to_tail(&gpsDKF->sDKFListNode, &psDKFEntry->sDKFEntryNode);

	gpsDKF->ui32NumEntries++;

	OSLockRelease(gpsDKF->hDKFListLock);

	*phDkpHandle = psDKFEntry;

	return PVRSRV_OK;


Error:
	PVR_DPF((PVR_DBG_ERROR, "%s: Error: '%s'", __func__,
	         PVRSRVGetErrorString(eError)));

	return eError;
}

PVRSRV_ERROR PVRSRVUnRegisterDKP(IMG_HANDLE hPrivData, PVRDKF_DKP_HANDLE hDkpHandle)
{
	DKF_REGISTERED_DKP *psDKFEntry = (DKF_REGISTERED_DKP *)hDkpHandle;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psDKFEntry)
	{
#if defined(DEBUG)
		if (psDKFEntry->hPrivHandle == hPrivData)
		{
			PVR_DPF((PVR_DBG_VERBOSE, "%s: Matched %p private handle",
			         __func__, hDkpHandle));
		}
		else
		{
			PVR_DPF((PVR_DBG_VERBOSE,
			         "%s: Did not find match (%p. vs %p), freeing anyway",
			         __func__, hPrivData, psDKFEntry->hPrivHandle));
		}
#endif	/* DEBUG */

		OSLockAcquire(gpsDKF->hDKFListLock);

		dllist_remove_node(&psDKFEntry->sDKFEntryNode);

		gpsDKF->ui32NumEntries--;

		OSLockRelease(gpsDKF->hDKFListLock);

		OSFreeMemNoStats(psDKFEntry);

	}
	else
	{
		eError = PVRSRV_ERROR_INVALID_PARAMS;
	}

	return eError;
}
#else

/* Stub routines for earlier kernel versions */

PVRSRV_ERROR PVRDKFInit(void)
{
	return PVRSRV_OK;
}

void PVRDKFDeInit(void)
{
}

void PVRDKFTraverse(DKF_VPRINTF_FUNC *pfnPrint,
                    void *pvArg,
                    struct _PVRSRV_DEVICE_NODE_ *psDevNode,
                    IMG_PID pid,
                    IMG_UINT32 ui32ConnectionType)
{
	PVR_UNREFERENCED_PARAMETER(psDevNode);
	PVR_UNREFERENCED_PARAMETER(pid);
}

void PVRDKPOutput(IMG_HANDLE hPrivData, const char *fmt, ...)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	PVR_UNREFERENCED_PARAMETER(fmt);
}

PVRSRV_ERROR PVRSRVRegisterDKP(IMG_HANDLE hPrivData, const char *pszDKPName,
                               DKP_PFN_SHOW *psShowPfn,
                               DKP_CONNECTION_FLAGS ui32Filter,
                               PPVRDKF_DKP_HANDLE phDkpHandle)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	PVR_UNREFERENCED_PARAMETER(pszDKPName);
	PVR_UNREFERENCED_PARAMETER(psShowPfn);
	PVR_UNREFERENCED_PARAMETER(phDkpHandle);

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVUnRegisterDKP(IMG_HANDLE hPrivData, PVRDKF_DKP_HANDLE hDkpHandle)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	PVR_UNREFERENCED_PARAMETER(hDkpHandle);

	return PVRSRV_OK;
}
#endif	/* SUPPORT_LINUX_FDINFO */

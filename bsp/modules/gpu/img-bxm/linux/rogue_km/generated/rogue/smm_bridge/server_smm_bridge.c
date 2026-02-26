/*******************************************************************************
@File
@Title          Server bridge for smm
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements the server side of the bridge for smm
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
*******************************************************************************/

#include <linux/uaccess.h>

#include "img_defs.h"

#include "pmr.h"
#include "secure_export.h"

#include "common_smm_bridge.h"

#include "allocmem.h"
#include "pvr_debug.h"
#include "connection_server.h"
#include "pvr_bridge.h"
#if defined(SUPPORT_RGX)
#include "rgx_bridge.h"
#endif
#include "srvcore.h"
#include "handle.h"

#include <linux/slab.h>

/* ***************************************************************************
 * Server-side bridge entry points
 */

static IMG_INT
PVRSRVBridgePMRSecureExportPMR(IMG_UINT32 ui32DispatchTableEntry,
			       IMG_UINT8 * psPMRSecureExportPMRIN_UI8,
			       IMG_UINT8 * psPMRSecureExportPMROUT_UI8,
			       CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_PMRSECUREEXPORTPMR *psPMRSecureExportPMRIN =
	    (PVRSRV_BRIDGE_IN_PMRSECUREEXPORTPMR *) IMG_OFFSET_ADDR(psPMRSecureExportPMRIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_PMRSECUREEXPORTPMR *psPMRSecureExportPMROUT =
	    (PVRSRV_BRIDGE_OUT_PMRSECUREEXPORTPMR *) IMG_OFFSET_ADDR(psPMRSecureExportPMROUT_UI8,
								     0);

	IMG_HANDLE hPMR = psPMRSecureExportPMRIN->hPMR;
	PMR *psPMRInt = NULL;
	PMR *psPMROutInt = NULL;
	CONNECTION_DATA *psSecureConnection;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psPMRSecureExportPMROUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psPMRInt,
				       hPMR, PVRSRV_HANDLE_TYPE_PHYSMEM_PMR, IMG_TRUE);
	if (unlikely(psPMRSecureExportPMROUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto PMRSecureExportPMR_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psPMRSecureExportPMROUT->eError =
	    PMRSecureExportPMR(psConnection, OSGetDevNode(psConnection),
			       psPMRInt,
			       &psPMRSecureExportPMROUT->Export, &psPMROutInt, &psSecureConnection);
	/* Exit early if bridged call fails */
	if (unlikely(psPMRSecureExportPMROUT->eError != PVRSRV_OK))
	{
		goto PMRSecureExportPMR_exit;
	}

PMRSecureExportPMR_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psPMRInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hPMR, PVRSRV_HANDLE_TYPE_PHYSMEM_PMR);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psPMRSecureExportPMROUT->eError != PVRSRV_OK)
	{
		if (psPMROutInt)
		{
			LockHandle(KERNEL_HANDLE_BASE);
			PMRSecureUnexportPMR(psPMROutInt);
			UnlockHandle(KERNEL_HANDLE_BASE);
		}
	}

	return 0;
}

static IMG_INT
PVRSRVBridgePMRSecureUnexportPMR(IMG_UINT32 ui32DispatchTableEntry,
				 IMG_UINT8 * psPMRSecureUnexportPMRIN_UI8,
				 IMG_UINT8 * psPMRSecureUnexportPMROUT_UI8,
				 CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_PMRSECUREUNEXPORTPMR *psPMRSecureUnexportPMRIN =
	    (PVRSRV_BRIDGE_IN_PMRSECUREUNEXPORTPMR *) IMG_OFFSET_ADDR(psPMRSecureUnexportPMRIN_UI8,
								      0);
	PVRSRV_BRIDGE_OUT_PMRSECUREUNEXPORTPMR *psPMRSecureUnexportPMROUT =
	    (PVRSRV_BRIDGE_OUT_PMRSECUREUNEXPORTPMR *)
	    IMG_OFFSET_ADDR(psPMRSecureUnexportPMROUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psPMRSecureUnexportPMROUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psPMRSecureUnexportPMRIN->hPMR,
					      PVRSRV_HANDLE_TYPE_PHYSMEM_PMR_SECURE_EXPORT);
	if (unlikely((psPMRSecureUnexportPMROUT->eError != PVRSRV_OK) &&
		     (psPMRSecureUnexportPMROUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psPMRSecureUnexportPMROUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s",
			 __func__, PVRSRVGetErrorString(psPMRSecureUnexportPMROUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto PMRSecureUnexportPMR_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

PMRSecureUnexportPMR_exit:

	return 0;
}

static PVRSRV_ERROR _PMRSecureImportPMRpsPMRIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = PMRUnrefPMR((PMR *) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgePMRSecureImportPMR(IMG_UINT32 ui32DispatchTableEntry,
			       IMG_UINT8 * psPMRSecureImportPMRIN_UI8,
			       IMG_UINT8 * psPMRSecureImportPMROUT_UI8,
			       CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_PMRSECUREIMPORTPMR *psPMRSecureImportPMRIN =
	    (PVRSRV_BRIDGE_IN_PMRSECUREIMPORTPMR *) IMG_OFFSET_ADDR(psPMRSecureImportPMRIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_PMRSECUREIMPORTPMR *psPMRSecureImportPMROUT =
	    (PVRSRV_BRIDGE_OUT_PMRSECUREIMPORTPMR *) IMG_OFFSET_ADDR(psPMRSecureImportPMROUT_UI8,
								     0);

	PMR *psPMRInt = NULL;

	psPMRSecureImportPMROUT->eError =
	    PMRSecureImportPMR(psConnection, OSGetDevNode(psConnection),
			       psPMRSecureImportPMRIN->Export,
			       &psPMRInt,
			       &psPMRSecureImportPMROUT->uiSize, &psPMRSecureImportPMROUT->uiAlign);
	/* Exit early if bridged call fails */
	if (unlikely(psPMRSecureImportPMROUT->eError != PVRSRV_OK))
	{
		goto PMRSecureImportPMR_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psPMRSecureImportPMROUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
								    &psPMRSecureImportPMROUT->hPMR,
								    (void *)psPMRInt,
								    PVRSRV_HANDLE_TYPE_PHYSMEM_PMR,
								    PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
								    (PFN_HANDLE_RELEASE) &
								    _PMRSecureImportPMRpsPMRIntRelease);
	if (unlikely(psPMRSecureImportPMROUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto PMRSecureImportPMR_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

PMRSecureImportPMR_exit:

	if (psPMRSecureImportPMROUT->eError != PVRSRV_OK)
	{
		if (psPMRInt)
		{
			LockHandle(KERNEL_HANDLE_BASE);
			PMRUnrefPMR(psPMRInt);
			UnlockHandle(KERNEL_HANDLE_BASE);
		}
	}

	return 0;
}

/* ***************************************************************************
 * Server bridge dispatch related glue
 */

PVRSRV_ERROR InitSMMBridge(void);
void DeinitSMMBridge(void);

/*
 * Register all SMM functions with services
 */
PVRSRV_ERROR InitSMMBridge(void)
{

	SetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREEXPORTPMR,
			      PVRSRVBridgePMRSecureExportPMR, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_PMRSECUREEXPORTPMR),
			      sizeof(PVRSRV_BRIDGE_OUT_PMRSECUREEXPORTPMR));

	SetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREUNEXPORTPMR,
			      PVRSRVBridgePMRSecureUnexportPMR, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_PMRSECUREUNEXPORTPMR),
			      sizeof(PVRSRV_BRIDGE_OUT_PMRSECUREUNEXPORTPMR));

	SetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREIMPORTPMR,
			      PVRSRVBridgePMRSecureImportPMR, NULL,
			      sizeof(PVRSRV_BRIDGE_IN_PMRSECUREIMPORTPMR),
			      sizeof(PVRSRV_BRIDGE_OUT_PMRSECUREIMPORTPMR));

	return PVRSRV_OK;
}

/*
 * Unregister all smm functions with services
 */
void DeinitSMMBridge(void)
{

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREEXPORTPMR);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREUNEXPORTPMR);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREIMPORTPMR);

}

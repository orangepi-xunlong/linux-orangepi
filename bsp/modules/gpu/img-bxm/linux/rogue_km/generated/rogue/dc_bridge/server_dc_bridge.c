/*******************************************************************************
@File
@Title          Server bridge for dc
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements the server side of the bridge for dc
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

#include "dc_server.h"

#include "common_dc_bridge.h"

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

#include "lock.h"

/* ***************************************************************************
 * Server-side bridge entry points
 */

static IMG_INT
PVRSRVBridgeDCDevicesQueryCount(IMG_UINT32 ui32DispatchTableEntry,
				IMG_UINT8 * psDCDevicesQueryCountIN_UI8,
				IMG_UINT8 * psDCDevicesQueryCountOUT_UI8,
				CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDEVICESQUERYCOUNT *psDCDevicesQueryCountIN =
	    (PVRSRV_BRIDGE_IN_DCDEVICESQUERYCOUNT *) IMG_OFFSET_ADDR(psDCDevicesQueryCountIN_UI8,
								     0);
	PVRSRV_BRIDGE_OUT_DCDEVICESQUERYCOUNT *psDCDevicesQueryCountOUT =
	    (PVRSRV_BRIDGE_OUT_DCDEVICESQUERYCOUNT *) IMG_OFFSET_ADDR(psDCDevicesQueryCountOUT_UI8,
								      0);

	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDCDevicesQueryCountIN);

	psDCDevicesQueryCountOUT->eError =
	    DCDevicesQueryCount(&psDCDevicesQueryCountOUT->ui32DeviceCount);

	return 0;
}

static_assert(DC_MAX_DEVICE_COUNT <= IMG_UINT32_MAX,
	      "DC_MAX_DEVICE_COUNT must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCDevicesEnumerate(IMG_UINT32 ui32DispatchTableEntry,
			       IMG_UINT8 * psDCDevicesEnumerateIN_UI8,
			       IMG_UINT8 * psDCDevicesEnumerateOUT_UI8,
			       CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDEVICESENUMERATE *psDCDevicesEnumerateIN =
	    (PVRSRV_BRIDGE_IN_DCDEVICESENUMERATE *) IMG_OFFSET_ADDR(psDCDevicesEnumerateIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDEVICESENUMERATE *psDCDevicesEnumerateOUT =
	    (PVRSRV_BRIDGE_OUT_DCDEVICESENUMERATE *) IMG_OFFSET_ADDR(psDCDevicesEnumerateOUT_UI8,
								     0);

	IMG_UINT32 *pui32DeviceIndexInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCDevicesEnumerateIN->ui32DeviceArraySize * sizeof(IMG_UINT32)) + 0;

	if (psDCDevicesEnumerateIN->ui32DeviceArraySize > DC_MAX_DEVICE_COUNT)
	{
		psDCDevicesEnumerateOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCDevicesEnumerate_exit;
	}

	psDCDevicesEnumerateOUT->pui32DeviceIndex = psDCDevicesEnumerateIN->pui32DeviceIndex;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCDevicesEnumerateOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCDevicesEnumerate_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCDevicesEnumerateIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psDCDevicesEnumerateIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCDevicesEnumerateOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCDevicesEnumerate_exit;
			}
		}
	}

	if (psDCDevicesEnumerateIN->ui32DeviceArraySize != 0)
	{
		pui32DeviceIndexInt =
		    (IMG_UINT32 *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCDevicesEnumerateIN->ui32DeviceArraySize * sizeof(IMG_UINT32);
	}

	psDCDevicesEnumerateOUT->eError =
	    DCDevicesEnumerate(psConnection, OSGetDevNode(psConnection),
			       psDCDevicesEnumerateIN->ui32DeviceArraySize,
			       &psDCDevicesEnumerateOUT->ui32DeviceCount, pui32DeviceIndexInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCDevicesEnumerateOUT->eError != PVRSRV_OK))
	{
		goto DCDevicesEnumerate_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((pui32DeviceIndexInt) &&
	    ((psDCDevicesEnumerateOUT->ui32DeviceCount * sizeof(IMG_UINT32)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL, (void __user *)psDCDevicesEnumerateOUT->pui32DeviceIndex,
		      pui32DeviceIndexInt,
		      (psDCDevicesEnumerateOUT->ui32DeviceCount * sizeof(IMG_UINT32))) !=
		     PVRSRV_OK))
		{
			psDCDevicesEnumerateOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDevicesEnumerate_exit;
		}
	}

DCDevicesEnumerate_exit:

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCDevicesEnumerateOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static PVRSRV_ERROR _DCDeviceAcquirepsDeviceIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCDeviceRelease((DC_DEVICE *) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgeDCDeviceAcquire(IMG_UINT32 ui32DispatchTableEntry,
			    IMG_UINT8 * psDCDeviceAcquireIN_UI8,
			    IMG_UINT8 * psDCDeviceAcquireOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDEVICEACQUIRE *psDCDeviceAcquireIN =
	    (PVRSRV_BRIDGE_IN_DCDEVICEACQUIRE *) IMG_OFFSET_ADDR(psDCDeviceAcquireIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDEVICEACQUIRE *psDCDeviceAcquireOUT =
	    (PVRSRV_BRIDGE_OUT_DCDEVICEACQUIRE *) IMG_OFFSET_ADDR(psDCDeviceAcquireOUT_UI8, 0);

	DC_DEVICE *psDeviceInt = NULL;

	psDCDeviceAcquireOUT->eError =
	    DCDeviceAcquire(psConnection, OSGetDevNode(psConnection),
			    psDCDeviceAcquireIN->ui32DeviceIndex, &psDeviceInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCDeviceAcquireOUT->eError != PVRSRV_OK))
	{
		goto DCDeviceAcquire_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psDCDeviceAcquireOUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
								 &psDCDeviceAcquireOUT->hDevice,
								 (void *)psDeviceInt,
								 PVRSRV_HANDLE_TYPE_DC_DEVICE,
								 PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
								 (PFN_HANDLE_RELEASE) &
								 _DCDeviceAcquirepsDeviceIntRelease);
	if (unlikely(psDCDeviceAcquireOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCDeviceAcquire_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

DCDeviceAcquire_exit:

	if (psDCDeviceAcquireOUT->eError != PVRSRV_OK)
	{
		if (psDeviceInt)
		{
			DCDeviceRelease(psDeviceInt);
		}
	}

	return 0;
}

static IMG_INT
PVRSRVBridgeDCDeviceRelease(IMG_UINT32 ui32DispatchTableEntry,
			    IMG_UINT8 * psDCDeviceReleaseIN_UI8,
			    IMG_UINT8 * psDCDeviceReleaseOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDEVICERELEASE *psDCDeviceReleaseIN =
	    (PVRSRV_BRIDGE_IN_DCDEVICERELEASE *) IMG_OFFSET_ADDR(psDCDeviceReleaseIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDEVICERELEASE *psDCDeviceReleaseOUT =
	    (PVRSRV_BRIDGE_OUT_DCDEVICERELEASE *) IMG_OFFSET_ADDR(psDCDeviceReleaseOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psDCDeviceReleaseOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psDCDeviceReleaseIN->hDevice,
					      PVRSRV_HANDLE_TYPE_DC_DEVICE);
	if (unlikely((psDCDeviceReleaseOUT->eError != PVRSRV_OK) &&
		     (psDCDeviceReleaseOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psDCDeviceReleaseOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s", __func__, PVRSRVGetErrorString(psDCDeviceReleaseOUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto DCDeviceRelease_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

DCDeviceRelease_exit:

	return 0;
}

static IMG_INT
PVRSRVBridgeDCGetInfo(IMG_UINT32 ui32DispatchTableEntry,
		      IMG_UINT8 * psDCGetInfoIN_UI8,
		      IMG_UINT8 * psDCGetInfoOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCGETINFO *psDCGetInfoIN =
	    (PVRSRV_BRIDGE_IN_DCGETINFO *) IMG_OFFSET_ADDR(psDCGetInfoIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCGETINFO *psDCGetInfoOUT =
	    (PVRSRV_BRIDGE_OUT_DCGETINFO *) IMG_OFFSET_ADDR(psDCGetInfoOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCGetInfoIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCGetInfoOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCGetInfoOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCGetInfo_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCGetInfoOUT->eError = DCGetInfo(psDeviceInt, &psDCGetInfoOUT->sDisplayInfo);

DCGetInfo_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	return 0;
}

static IMG_INT
PVRSRVBridgeDCPanelQueryCount(IMG_UINT32 ui32DispatchTableEntry,
			      IMG_UINT8 * psDCPanelQueryCountIN_UI8,
			      IMG_UINT8 * psDCPanelQueryCountOUT_UI8,
			      CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCPANELQUERYCOUNT *psDCPanelQueryCountIN =
	    (PVRSRV_BRIDGE_IN_DCPANELQUERYCOUNT *) IMG_OFFSET_ADDR(psDCPanelQueryCountIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCPANELQUERYCOUNT *psDCPanelQueryCountOUT =
	    (PVRSRV_BRIDGE_OUT_DCPANELQUERYCOUNT *) IMG_OFFSET_ADDR(psDCPanelQueryCountOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCPanelQueryCountIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCPanelQueryCountOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCPanelQueryCountOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCPanelQueryCount_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCPanelQueryCountOUT->eError =
	    DCPanelQueryCount(psDeviceInt, &psDCPanelQueryCountOUT->ui32NumPanels);

DCPanelQueryCount_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	return 0;
}

static_assert(DC_MAX_PANEL_COUNT <= IMG_UINT32_MAX,
	      "DC_MAX_PANEL_COUNT must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCPanelQuery(IMG_UINT32 ui32DispatchTableEntry,
			 IMG_UINT8 * psDCPanelQueryIN_UI8,
			 IMG_UINT8 * psDCPanelQueryOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCPANELQUERY *psDCPanelQueryIN =
	    (PVRSRV_BRIDGE_IN_DCPANELQUERY *) IMG_OFFSET_ADDR(psDCPanelQueryIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCPANELQUERY *psDCPanelQueryOUT =
	    (PVRSRV_BRIDGE_OUT_DCPANELQUERY *) IMG_OFFSET_ADDR(psDCPanelQueryOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCPanelQueryIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;
	PVRSRV_PANEL_INFO *psPanelInfoInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCPanelQueryIN->ui32PanelsArraySize * sizeof(PVRSRV_PANEL_INFO)) + 0;

	if (psDCPanelQueryIN->ui32PanelsArraySize > DC_MAX_PANEL_COUNT)
	{
		psDCPanelQueryOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCPanelQuery_exit;
	}

	psDCPanelQueryOUT->psPanelInfo = psDCPanelQueryIN->psPanelInfo;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCPanelQueryOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCPanelQuery_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCPanelQueryIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psDCPanelQueryIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCPanelQueryOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCPanelQuery_exit;
			}
		}
	}

	if (psDCPanelQueryIN->ui32PanelsArraySize != 0)
	{
		psPanelInfoInt =
		    (PVRSRV_PANEL_INFO *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCPanelQueryIN->ui32PanelsArraySize * sizeof(PVRSRV_PANEL_INFO);
	}

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCPanelQueryOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCPanelQueryOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCPanelQuery_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCPanelQueryOUT->eError =
	    DCPanelQuery(psDeviceInt,
			 psDCPanelQueryIN->ui32PanelsArraySize,
			 &psDCPanelQueryOUT->ui32NumPanels, psPanelInfoInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCPanelQueryOUT->eError != PVRSRV_OK))
	{
		goto DCPanelQuery_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((psPanelInfoInt) &&
	    ((psDCPanelQueryOUT->ui32NumPanels * sizeof(PVRSRV_PANEL_INFO)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL, (void __user *)psDCPanelQueryOUT->psPanelInfo, psPanelInfoInt,
		      (psDCPanelQueryOUT->ui32NumPanels * sizeof(PVRSRV_PANEL_INFO))) != PVRSRV_OK))
		{
			psDCPanelQueryOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCPanelQuery_exit;
		}
	}

DCPanelQuery_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCPanelQueryOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static_assert(DC_MAX_FORMATS <= IMG_UINT32_MAX,
	      "DC_MAX_FORMATS must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCFormatQuery(IMG_UINT32 ui32DispatchTableEntry,
			  IMG_UINT8 * psDCFormatQueryIN_UI8,
			  IMG_UINT8 * psDCFormatQueryOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCFORMATQUERY *psDCFormatQueryIN =
	    (PVRSRV_BRIDGE_IN_DCFORMATQUERY *) IMG_OFFSET_ADDR(psDCFormatQueryIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCFORMATQUERY *psDCFormatQueryOUT =
	    (PVRSRV_BRIDGE_OUT_DCFORMATQUERY *) IMG_OFFSET_ADDR(psDCFormatQueryOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCFormatQueryIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;
	PVRSRV_SURFACE_FORMAT *psFormatInt = NULL;
	IMG_UINT32 *pui32SupportedInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCFormatQueryIN->ui32NumFormats * sizeof(PVRSRV_SURFACE_FORMAT)) +
	    ((IMG_UINT64) psDCFormatQueryIN->ui32NumFormats * sizeof(IMG_UINT32)) + 0;

	if (unlikely(psDCFormatQueryIN->ui32NumFormats > DC_MAX_FORMATS))
	{
		psDCFormatQueryOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCFormatQuery_exit;
	}

	psDCFormatQueryOUT->pui32Supported = psDCFormatQueryIN->pui32Supported;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCFormatQueryOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCFormatQuery_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCFormatQueryIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psDCFormatQueryIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCFormatQueryOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCFormatQuery_exit;
			}
		}
	}

	if (psDCFormatQueryIN->ui32NumFormats != 0)
	{
		psFormatInt =
		    (PVRSRV_SURFACE_FORMAT *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCFormatQueryIN->ui32NumFormats * sizeof(PVRSRV_SURFACE_FORMAT);
	}

	/* Copy the data over */
	if (psDCFormatQueryIN->ui32NumFormats * sizeof(PVRSRV_SURFACE_FORMAT) > 0)
	{
		if (OSCopyFromUser
		    (NULL, psFormatInt, (const void __user *)psDCFormatQueryIN->psFormat,
		     psDCFormatQueryIN->ui32NumFormats * sizeof(PVRSRV_SURFACE_FORMAT)) !=
		    PVRSRV_OK)
		{
			psDCFormatQueryOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCFormatQuery_exit;
		}
	}
	if (psDCFormatQueryIN->ui32NumFormats != 0)
	{
		pui32SupportedInt =
		    (IMG_UINT32 *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCFormatQueryIN->ui32NumFormats * sizeof(IMG_UINT32);
	}

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCFormatQueryOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCFormatQueryOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCFormatQuery_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCFormatQueryOUT->eError =
	    DCFormatQuery(psDeviceInt,
			  psDCFormatQueryIN->ui32NumFormats, psFormatInt, pui32SupportedInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCFormatQueryOUT->eError != PVRSRV_OK))
	{
		goto DCFormatQuery_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((pui32SupportedInt) && ((psDCFormatQueryIN->ui32NumFormats * sizeof(IMG_UINT32)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL, (void __user *)psDCFormatQueryOUT->pui32Supported, pui32SupportedInt,
		      (psDCFormatQueryIN->ui32NumFormats * sizeof(IMG_UINT32))) != PVRSRV_OK))
		{
			psDCFormatQueryOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCFormatQuery_exit;
		}
	}

DCFormatQuery_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCFormatQueryOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static_assert(DC_MAX_DIMENSIONS <= IMG_UINT32_MAX,
	      "DC_MAX_DIMENSIONS must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCDimQuery(IMG_UINT32 ui32DispatchTableEntry,
		       IMG_UINT8 * psDCDimQueryIN_UI8,
		       IMG_UINT8 * psDCDimQueryOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDIMQUERY *psDCDimQueryIN =
	    (PVRSRV_BRIDGE_IN_DCDIMQUERY *) IMG_OFFSET_ADDR(psDCDimQueryIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDIMQUERY *psDCDimQueryOUT =
	    (PVRSRV_BRIDGE_OUT_DCDIMQUERY *) IMG_OFFSET_ADDR(psDCDimQueryOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCDimQueryIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;
	PVRSRV_SURFACE_DIMS *psDimInt = NULL;
	IMG_UINT32 *pui32SupportedInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCDimQueryIN->ui32NumDims * sizeof(PVRSRV_SURFACE_DIMS)) +
	    ((IMG_UINT64) psDCDimQueryIN->ui32NumDims * sizeof(IMG_UINT32)) + 0;

	if (unlikely(psDCDimQueryIN->ui32NumDims > DC_MAX_DIMENSIONS))
	{
		psDCDimQueryOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCDimQuery_exit;
	}

	psDCDimQueryOUT->pui32Supported = psDCDimQueryIN->pui32Supported;

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCDimQueryOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCDimQuery_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCDimQueryIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psDCDimQueryIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCDimQueryOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCDimQuery_exit;
			}
		}
	}

	if (psDCDimQueryIN->ui32NumDims != 0)
	{
		psDimInt =
		    (PVRSRV_SURFACE_DIMS *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCDimQueryIN->ui32NumDims * sizeof(PVRSRV_SURFACE_DIMS);
	}

	/* Copy the data over */
	if (psDCDimQueryIN->ui32NumDims * sizeof(PVRSRV_SURFACE_DIMS) > 0)
	{
		if (OSCopyFromUser
		    (NULL, psDimInt, (const void __user *)psDCDimQueryIN->psDim,
		     psDCDimQueryIN->ui32NumDims * sizeof(PVRSRV_SURFACE_DIMS)) != PVRSRV_OK)
		{
			psDCDimQueryOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDimQuery_exit;
		}
	}
	if (psDCDimQueryIN->ui32NumDims != 0)
	{
		pui32SupportedInt =
		    (IMG_UINT32 *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCDimQueryIN->ui32NumDims * sizeof(IMG_UINT32);
	}

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCDimQueryOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCDimQueryOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCDimQuery_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCDimQueryOUT->eError =
	    DCDimQuery(psDeviceInt, psDCDimQueryIN->ui32NumDims, psDimInt, pui32SupportedInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCDimQueryOUT->eError != PVRSRV_OK))
	{
		goto DCDimQuery_exit;
	}

	/* If dest ptr is non-null and we have data to copy */
	if ((pui32SupportedInt) && ((psDCDimQueryIN->ui32NumDims * sizeof(IMG_UINT32)) > 0))
	{
		if (unlikely
		    (OSCopyToUser
		     (NULL, (void __user *)psDCDimQueryOUT->pui32Supported, pui32SupportedInt,
		      (psDCDimQueryIN->ui32NumDims * sizeof(IMG_UINT32))) != PVRSRV_OK))
		{
			psDCDimQueryOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDimQuery_exit;
		}
	}

DCDimQuery_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCDimQueryOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static IMG_INT
PVRSRVBridgeDCSetBlank(IMG_UINT32 ui32DispatchTableEntry,
		       IMG_UINT8 * psDCSetBlankIN_UI8,
		       IMG_UINT8 * psDCSetBlankOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCSETBLANK *psDCSetBlankIN =
	    (PVRSRV_BRIDGE_IN_DCSETBLANK *) IMG_OFFSET_ADDR(psDCSetBlankIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCSETBLANK *psDCSetBlankOUT =
	    (PVRSRV_BRIDGE_OUT_DCSETBLANK *) IMG_OFFSET_ADDR(psDCSetBlankOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCSetBlankIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCSetBlankOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCSetBlankOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCSetBlank_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCSetBlankOUT->eError = DCSetBlank(psDeviceInt, psDCSetBlankIN->bEnabled);

DCSetBlank_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	return 0;
}

static IMG_INT
PVRSRVBridgeDCSetVSyncReporting(IMG_UINT32 ui32DispatchTableEntry,
				IMG_UINT8 * psDCSetVSyncReportingIN_UI8,
				IMG_UINT8 * psDCSetVSyncReportingOUT_UI8,
				CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCSETVSYNCREPORTING *psDCSetVSyncReportingIN =
	    (PVRSRV_BRIDGE_IN_DCSETVSYNCREPORTING *) IMG_OFFSET_ADDR(psDCSetVSyncReportingIN_UI8,
								     0);
	PVRSRV_BRIDGE_OUT_DCSETVSYNCREPORTING *psDCSetVSyncReportingOUT =
	    (PVRSRV_BRIDGE_OUT_DCSETVSYNCREPORTING *) IMG_OFFSET_ADDR(psDCSetVSyncReportingOUT_UI8,
								      0);

	IMG_HANDLE hDevice = psDCSetVSyncReportingIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCSetVSyncReportingOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCSetVSyncReportingOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCSetVSyncReporting_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCSetVSyncReportingOUT->eError =
	    DCSetVSyncReporting(psDeviceInt, psDCSetVSyncReportingIN->bEnabled);

DCSetVSyncReporting_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	return 0;
}

static IMG_INT
PVRSRVBridgeDCLastVSyncQuery(IMG_UINT32 ui32DispatchTableEntry,
			     IMG_UINT8 * psDCLastVSyncQueryIN_UI8,
			     IMG_UINT8 * psDCLastVSyncQueryOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCLASTVSYNCQUERY *psDCLastVSyncQueryIN =
	    (PVRSRV_BRIDGE_IN_DCLASTVSYNCQUERY *) IMG_OFFSET_ADDR(psDCLastVSyncQueryIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCLASTVSYNCQUERY *psDCLastVSyncQueryOUT =
	    (PVRSRV_BRIDGE_OUT_DCLASTVSYNCQUERY *) IMG_OFFSET_ADDR(psDCLastVSyncQueryOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCLastVSyncQueryIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCLastVSyncQueryOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCLastVSyncQueryOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCLastVSyncQuery_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCLastVSyncQueryOUT->eError =
	    DCLastVSyncQuery(psDeviceInt, &psDCLastVSyncQueryOUT->i64Timestamp);

DCLastVSyncQuery_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	return 0;
}

static PVRSRV_ERROR _DCSystemBufferAcquirepsBufferIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCSystemBufferRelease((DC_BUFFER *) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgeDCSystemBufferAcquire(IMG_UINT32 ui32DispatchTableEntry,
				  IMG_UINT8 * psDCSystemBufferAcquireIN_UI8,
				  IMG_UINT8 * psDCSystemBufferAcquireOUT_UI8,
				  CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCSYSTEMBUFFERACQUIRE *psDCSystemBufferAcquireIN =
	    (PVRSRV_BRIDGE_IN_DCSYSTEMBUFFERACQUIRE *)
	    IMG_OFFSET_ADDR(psDCSystemBufferAcquireIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCSYSTEMBUFFERACQUIRE *psDCSystemBufferAcquireOUT =
	    (PVRSRV_BRIDGE_OUT_DCSYSTEMBUFFERACQUIRE *)
	    IMG_OFFSET_ADDR(psDCSystemBufferAcquireOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCSystemBufferAcquireIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;
	DC_BUFFER *psBufferInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCSystemBufferAcquireOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCSystemBufferAcquireOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCSystemBufferAcquire_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCSystemBufferAcquireOUT->eError =
	    DCSystemBufferAcquire(psDeviceInt,
				  &psDCSystemBufferAcquireOUT->ui32Stride, &psBufferInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCSystemBufferAcquireOUT->eError != PVRSRV_OK))
	{
		goto DCSystemBufferAcquire_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psDCSystemBufferAcquireOUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
								       &psDCSystemBufferAcquireOUT->
								       hBuffer, (void *)psBufferInt,
								       PVRSRV_HANDLE_TYPE_DC_BUFFER,
								       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
								       (PFN_HANDLE_RELEASE) &
								       _DCSystemBufferAcquirepsBufferIntRelease);
	if (unlikely(psDCSystemBufferAcquireOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCSystemBufferAcquire_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

DCSystemBufferAcquire_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psDCSystemBufferAcquireOUT->eError != PVRSRV_OK)
	{
		if (psBufferInt)
		{
			LockHandle(KERNEL_HANDLE_BASE);
			DCSystemBufferRelease(psBufferInt);
			UnlockHandle(KERNEL_HANDLE_BASE);
		}
	}

	return 0;
}

static IMG_INT
PVRSRVBridgeDCSystemBufferRelease(IMG_UINT32 ui32DispatchTableEntry,
				  IMG_UINT8 * psDCSystemBufferReleaseIN_UI8,
				  IMG_UINT8 * psDCSystemBufferReleaseOUT_UI8,
				  CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCSYSTEMBUFFERRELEASE *psDCSystemBufferReleaseIN =
	    (PVRSRV_BRIDGE_IN_DCSYSTEMBUFFERRELEASE *)
	    IMG_OFFSET_ADDR(psDCSystemBufferReleaseIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCSYSTEMBUFFERRELEASE *psDCSystemBufferReleaseOUT =
	    (PVRSRV_BRIDGE_OUT_DCSYSTEMBUFFERRELEASE *)
	    IMG_OFFSET_ADDR(psDCSystemBufferReleaseOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psDCSystemBufferReleaseOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psDCSystemBufferReleaseIN->hBuffer,
					      PVRSRV_HANDLE_TYPE_DC_BUFFER);
	if (unlikely((psDCSystemBufferReleaseOUT->eError != PVRSRV_OK) &&
		     (psDCSystemBufferReleaseOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psDCSystemBufferReleaseOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s",
			 __func__, PVRSRVGetErrorString(psDCSystemBufferReleaseOUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto DCSystemBufferRelease_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

DCSystemBufferRelease_exit:

	return 0;
}

static PVRSRV_ERROR _DCDisplayContextCreatepsDisplayContextIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCDisplayContextDestroy((DC_DISPLAY_CONTEXT *) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgeDCDisplayContextCreate(IMG_UINT32 ui32DispatchTableEntry,
				   IMG_UINT8 * psDCDisplayContextCreateIN_UI8,
				   IMG_UINT8 * psDCDisplayContextCreateOUT_UI8,
				   CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCREATE *psDCDisplayContextCreateIN =
	    (PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCREATE *)
	    IMG_OFFSET_ADDR(psDCDisplayContextCreateIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCREATE *psDCDisplayContextCreateOUT =
	    (PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCREATE *)
	    IMG_OFFSET_ADDR(psDCDisplayContextCreateOUT_UI8, 0);

	IMG_HANDLE hDevice = psDCDisplayContextCreateIN->hDevice;
	DC_DEVICE *psDeviceInt = NULL;
	DC_DISPLAY_CONTEXT *psDisplayContextInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCDisplayContextCreateOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDeviceInt,
				       hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE, IMG_TRUE);
	if (unlikely(psDCDisplayContextCreateOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCDisplayContextCreate_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCDisplayContextCreateOUT->eError =
	    DCDisplayContextCreate(psDeviceInt, &psDisplayContextInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCDisplayContextCreateOUT->eError != PVRSRV_OK))
	{
		goto DCDisplayContextCreate_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psDCDisplayContextCreateOUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
									&psDCDisplayContextCreateOUT->
									hDisplayContext,
									(void *)psDisplayContextInt,
									PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT,
									PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
									(PFN_HANDLE_RELEASE) &
									_DCDisplayContextCreatepsDisplayContextIntRelease);
	if (unlikely(psDCDisplayContextCreateOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCDisplayContextCreate_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

DCDisplayContextCreate_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDeviceInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDevice, PVRSRV_HANDLE_TYPE_DC_DEVICE);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psDCDisplayContextCreateOUT->eError != PVRSRV_OK)
	{
		if (psDisplayContextInt)
		{
			DCDisplayContextDestroy(psDisplayContextInt);
		}
	}

	return 0;
}

static_assert(DC_MAX_PIPE_COUNT <= IMG_UINT32_MAX,
	      "DC_MAX_PIPE_COUNT must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCDisplayContextConfigureCheck(IMG_UINT32 ui32DispatchTableEntry,
					   IMG_UINT8 * psDCDisplayContextConfigureCheckIN_UI8,
					   IMG_UINT8 * psDCDisplayContextConfigureCheckOUT_UI8,
					   CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCONFIGURECHECK *psDCDisplayContextConfigureCheckIN =
	    (PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCONFIGURECHECK *)
	    IMG_OFFSET_ADDR(psDCDisplayContextConfigureCheckIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCONFIGURECHECK *psDCDisplayContextConfigureCheckOUT =
	    (PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCONFIGURECHECK *)
	    IMG_OFFSET_ADDR(psDCDisplayContextConfigureCheckOUT_UI8, 0);

	IMG_HANDLE hDisplayContext = psDCDisplayContextConfigureCheckIN->hDisplayContext;
	DC_DISPLAY_CONTEXT *psDisplayContextInt = NULL;
	PVRSRV_SURFACE_CONFIG_INFO *psSurfInfoInt = NULL;
	DC_BUFFER **psBuffersInt = NULL;
	IMG_HANDLE *hBuffersInt2 = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCDisplayContextConfigureCheckIN->ui32PipeCount *
	     sizeof(PVRSRV_SURFACE_CONFIG_INFO)) +
	    ((IMG_UINT64) psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(DC_BUFFER *)) +
	    ((IMG_UINT64) psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(IMG_HANDLE)) +
	    0;

	if (unlikely(psDCDisplayContextConfigureCheckIN->ui32PipeCount > DC_MAX_PIPE_COUNT))
	{
		psDCDisplayContextConfigureCheckOUT->eError =
		    PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCDisplayContextConfigureCheck_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCDisplayContextConfigureCheckOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCDisplayContextConfigureCheck_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCDisplayContextConfigureCheckIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer =
			    (IMG_BYTE *) (void *)psDCDisplayContextConfigureCheckIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCDisplayContextConfigureCheckOUT->eError =
				    PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCDisplayContextConfigureCheck_exit;
			}
		}
	}

	if (psDCDisplayContextConfigureCheckIN->ui32PipeCount != 0)
	{
		psSurfInfoInt =
		    (PVRSRV_SURFACE_CONFIG_INFO *) IMG_OFFSET_ADDR(pArrayArgsBuffer,
								   ui32NextOffset);
		ui32NextOffset +=
		    psDCDisplayContextConfigureCheckIN->ui32PipeCount *
		    sizeof(PVRSRV_SURFACE_CONFIG_INFO);
	}

	/* Copy the data over */
	if (psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(PVRSRV_SURFACE_CONFIG_INFO) >
	    0)
	{
		if (OSCopyFromUser
		    (NULL, psSurfInfoInt,
		     (const void __user *)psDCDisplayContextConfigureCheckIN->psSurfInfo,
		     psDCDisplayContextConfigureCheckIN->ui32PipeCount *
		     sizeof(PVRSRV_SURFACE_CONFIG_INFO)) != PVRSRV_OK)
		{
			psDCDisplayContextConfigureCheckOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDisplayContextConfigureCheck_exit;
		}
	}
	if (psDCDisplayContextConfigureCheckIN->ui32PipeCount != 0)
	{
		psBuffersInt = (DC_BUFFER **) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		OSCachedMemSet(psBuffersInt, 0,
			       psDCDisplayContextConfigureCheckIN->ui32PipeCount *
			       sizeof(DC_BUFFER *));
		ui32NextOffset +=
		    psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(DC_BUFFER *);
		hBuffersInt2 = (IMG_HANDLE *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset +=
		    psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(IMG_HANDLE);
	}

	/* Copy the data over */
	if (psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(IMG_HANDLE) > 0)
	{
		if (OSCopyFromUser
		    (NULL, hBuffersInt2,
		     (const void __user *)psDCDisplayContextConfigureCheckIN->phBuffers,
		     psDCDisplayContextConfigureCheckIN->ui32PipeCount * sizeof(IMG_HANDLE)) !=
		    PVRSRV_OK)
		{
			psDCDisplayContextConfigureCheckOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDisplayContextConfigureCheck_exit;
		}
	}

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCDisplayContextConfigureCheckOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDisplayContextInt,
				       hDisplayContext,
				       PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT, IMG_TRUE);
	if (unlikely(psDCDisplayContextConfigureCheckOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCDisplayContextConfigureCheck_exit;
	}

	{
		IMG_UINT32 i;

		for (i = 0; i < psDCDisplayContextConfigureCheckIN->ui32PipeCount; i++)
		{
			/* Look up the address from the handle */
			psDCDisplayContextConfigureCheckOUT->eError =
			    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
						       (void **)&psBuffersInt[i],
						       hBuffersInt2[i],
						       PVRSRV_HANDLE_TYPE_DC_BUFFER, IMG_TRUE);
			if (unlikely(psDCDisplayContextConfigureCheckOUT->eError != PVRSRV_OK))
			{
				UnlockHandle(psConnection->psHandleBase);
				goto DCDisplayContextConfigureCheck_exit;
			}
		}
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCDisplayContextConfigureCheckOUT->eError =
	    DCDisplayContextConfigureCheck(psDisplayContextInt,
					   psDCDisplayContextConfigureCheckIN->ui32PipeCount,
					   psSurfInfoInt, psBuffersInt);

DCDisplayContextConfigureCheck_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDisplayContextInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDisplayContext, PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT);
	}

	if (hBuffersInt2)
	{
		IMG_UINT32 i;

		for (i = 0; i < psDCDisplayContextConfigureCheckIN->ui32PipeCount; i++)
		{

			/* Unreference the previously looked up handle */
			if (psBuffersInt && psBuffersInt[i])
			{
				PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
							    hBuffersInt2[i],
							    PVRSRV_HANDLE_TYPE_DC_BUFFER);
			}
		}
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCDisplayContextConfigureCheckOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static IMG_INT
PVRSRVBridgeDCDisplayContextDestroy(IMG_UINT32 ui32DispatchTableEntry,
				    IMG_UINT8 * psDCDisplayContextDestroyIN_UI8,
				    IMG_UINT8 * psDCDisplayContextDestroyOUT_UI8,
				    CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTDESTROY *psDCDisplayContextDestroyIN =
	    (PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTDESTROY *)
	    IMG_OFFSET_ADDR(psDCDisplayContextDestroyIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTDESTROY *psDCDisplayContextDestroyOUT =
	    (PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTDESTROY *)
	    IMG_OFFSET_ADDR(psDCDisplayContextDestroyOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psDCDisplayContextDestroyOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psDCDisplayContextDestroyIN->
					      hDisplayContext,
					      PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT);
	if (unlikely
	    ((psDCDisplayContextDestroyOUT->eError != PVRSRV_OK)
	     && (psDCDisplayContextDestroyOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL)
	     && (psDCDisplayContextDestroyOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s",
			 __func__, PVRSRVGetErrorString(psDCDisplayContextDestroyOUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto DCDisplayContextDestroy_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

DCDisplayContextDestroy_exit:

	return 0;
}

static PVRSRV_ERROR _DCBufferAllocpsBufferIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCBufferFree((DC_BUFFER *) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgeDCBufferAlloc(IMG_UINT32 ui32DispatchTableEntry,
			  IMG_UINT8 * psDCBufferAllocIN_UI8,
			  IMG_UINT8 * psDCBufferAllocOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERALLOC *psDCBufferAllocIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERALLOC *) IMG_OFFSET_ADDR(psDCBufferAllocIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERALLOC *psDCBufferAllocOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERALLOC *) IMG_OFFSET_ADDR(psDCBufferAllocOUT_UI8, 0);

	IMG_HANDLE hDisplayContext = psDCBufferAllocIN->hDisplayContext;
	DC_DISPLAY_CONTEXT *psDisplayContextInt = NULL;
	DC_BUFFER *psBufferInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCBufferAllocOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDisplayContextInt,
				       hDisplayContext,
				       PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT, IMG_TRUE);
	if (unlikely(psDCBufferAllocOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferAlloc_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCBufferAllocOUT->eError =
	    DCBufferAlloc(psDisplayContextInt,
			  &psDCBufferAllocIN->sSurfInfo,
			  &psDCBufferAllocOUT->ui32Stride, &psBufferInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCBufferAllocOUT->eError != PVRSRV_OK))
	{
		goto DCBufferAlloc_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psDCBufferAllocOUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
							       &psDCBufferAllocOUT->hBuffer,
							       (void *)psBufferInt,
							       PVRSRV_HANDLE_TYPE_DC_BUFFER,
							       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
							       (PFN_HANDLE_RELEASE) &
							       _DCBufferAllocpsBufferIntRelease);
	if (unlikely(psDCBufferAllocOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferAlloc_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

DCBufferAlloc_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDisplayContextInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDisplayContext, PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psDCBufferAllocOUT->eError != PVRSRV_OK)
	{
		if (psBufferInt)
		{
			LockHandle(KERNEL_HANDLE_BASE);
			DCBufferFree(psBufferInt);
			UnlockHandle(KERNEL_HANDLE_BASE);
		}
	}

	return 0;
}

static PVRSRV_ERROR _DCBufferImportpsBufferIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCBufferFree((DC_BUFFER *) pvData);
	return eError;
}

static_assert(DC_MAX_PLANES <= IMG_UINT32_MAX,
	      "DC_MAX_PLANES must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCBufferImport(IMG_UINT32 ui32DispatchTableEntry,
			   IMG_UINT8 * psDCBufferImportIN_UI8,
			   IMG_UINT8 * psDCBufferImportOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERIMPORT *psDCBufferImportIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERIMPORT *) IMG_OFFSET_ADDR(psDCBufferImportIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERIMPORT *psDCBufferImportOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERIMPORT *) IMG_OFFSET_ADDR(psDCBufferImportOUT_UI8, 0);

	IMG_HANDLE hDisplayContext = psDCBufferImportIN->hDisplayContext;
	DC_DISPLAY_CONTEXT *psDisplayContextInt = NULL;
	PMR **psImportInt = NULL;
	IMG_HANDLE *hImportInt2 = NULL;
	DC_BUFFER *psBufferInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCBufferImportIN->ui32NumPlanes * sizeof(PMR *)) +
	    ((IMG_UINT64) psDCBufferImportIN->ui32NumPlanes * sizeof(IMG_HANDLE)) + 0;

	if (unlikely(psDCBufferImportIN->ui32NumPlanes > DC_MAX_PLANES))
	{
		psDCBufferImportOUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCBufferImport_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCBufferImportOUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCBufferImport_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCBufferImportIN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer = (IMG_BYTE *) (void *)psDCBufferImportIN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCBufferImportOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCBufferImport_exit;
			}
		}
	}

	if (psDCBufferImportIN->ui32NumPlanes != 0)
	{
		psImportInt = (PMR **) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		OSCachedMemSet(psImportInt, 0, psDCBufferImportIN->ui32NumPlanes * sizeof(PMR *));
		ui32NextOffset += psDCBufferImportIN->ui32NumPlanes * sizeof(PMR *);
		hImportInt2 = (IMG_HANDLE *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset += psDCBufferImportIN->ui32NumPlanes * sizeof(IMG_HANDLE);
	}

	/* Copy the data over */
	if (psDCBufferImportIN->ui32NumPlanes * sizeof(IMG_HANDLE) > 0)
	{
		if (OSCopyFromUser
		    (NULL, hImportInt2, (const void __user *)psDCBufferImportIN->phImport,
		     psDCBufferImportIN->ui32NumPlanes * sizeof(IMG_HANDLE)) != PVRSRV_OK)
		{
			psDCBufferImportOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCBufferImport_exit;
		}
	}

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCBufferImportOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDisplayContextInt,
				       hDisplayContext,
				       PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT, IMG_TRUE);
	if (unlikely(psDCBufferImportOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferImport_exit;
	}

	{
		IMG_UINT32 i;

		for (i = 0; i < psDCBufferImportIN->ui32NumPlanes; i++)
		{
			/* Look up the address from the handle */
			psDCBufferImportOUT->eError =
			    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
						       (void **)&psImportInt[i],
						       hImportInt2[i],
						       PVRSRV_HANDLE_TYPE_PHYSMEM_PMR, IMG_TRUE);
			if (unlikely(psDCBufferImportOUT->eError != PVRSRV_OK))
			{
				UnlockHandle(psConnection->psHandleBase);
				goto DCBufferImport_exit;
			}
		}
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCBufferImportOUT->eError =
	    DCBufferImport(psDisplayContextInt,
			   psDCBufferImportIN->ui32NumPlanes,
			   psImportInt, &psDCBufferImportIN->sSurfAttrib, &psBufferInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCBufferImportOUT->eError != PVRSRV_OK))
	{
		goto DCBufferImport_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psDCBufferImportOUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
								&psDCBufferImportOUT->hBuffer,
								(void *)psBufferInt,
								PVRSRV_HANDLE_TYPE_DC_BUFFER,
								PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
								(PFN_HANDLE_RELEASE) &
								_DCBufferImportpsBufferIntRelease);
	if (unlikely(psDCBufferImportOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferImport_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

DCBufferImport_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDisplayContextInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDisplayContext, PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT);
	}

	if (hImportInt2)
	{
		IMG_UINT32 i;

		for (i = 0; i < psDCBufferImportIN->ui32NumPlanes; i++)
		{

			/* Unreference the previously looked up handle */
			if (psImportInt && psImportInt[i])
			{
				PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
							    hImportInt2[i],
							    PVRSRV_HANDLE_TYPE_PHYSMEM_PMR);
			}
		}
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psDCBufferImportOUT->eError != PVRSRV_OK)
	{
		if (psBufferInt)
		{
			LockHandle(KERNEL_HANDLE_BASE);
			DCBufferFree(psBufferInt);
			UnlockHandle(KERNEL_HANDLE_BASE);
		}
	}

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCBufferImportOUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

static IMG_INT
PVRSRVBridgeDCBufferFree(IMG_UINT32 ui32DispatchTableEntry,
			 IMG_UINT8 * psDCBufferFreeIN_UI8,
			 IMG_UINT8 * psDCBufferFreeOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERFREE *psDCBufferFreeIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERFREE *) IMG_OFFSET_ADDR(psDCBufferFreeIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERFREE *psDCBufferFreeOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERFREE *) IMG_OFFSET_ADDR(psDCBufferFreeOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psDCBufferFreeOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psDCBufferFreeIN->hBuffer,
					      PVRSRV_HANDLE_TYPE_DC_BUFFER);
	if (unlikely((psDCBufferFreeOUT->eError != PVRSRV_OK) &&
		     (psDCBufferFreeOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psDCBufferFreeOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s", __func__, PVRSRVGetErrorString(psDCBufferFreeOUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferFree_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

DCBufferFree_exit:

	return 0;
}

static IMG_INT
PVRSRVBridgeDCBufferUnimport(IMG_UINT32 ui32DispatchTableEntry,
			     IMG_UINT8 * psDCBufferUnimportIN_UI8,
			     IMG_UINT8 * psDCBufferUnimportOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERUNIMPORT *psDCBufferUnimportIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERUNIMPORT *) IMG_OFFSET_ADDR(psDCBufferUnimportIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERUNIMPORT *psDCBufferUnimportOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERUNIMPORT *) IMG_OFFSET_ADDR(psDCBufferUnimportOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psDCBufferUnimportOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psDCBufferUnimportIN->hBuffer,
					      PVRSRV_HANDLE_TYPE_DC_BUFFER);
	if (unlikely((psDCBufferUnimportOUT->eError != PVRSRV_OK) &&
		     (psDCBufferUnimportOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psDCBufferUnimportOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s", __func__, PVRSRVGetErrorString(psDCBufferUnimportOUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferUnimport_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

DCBufferUnimport_exit:

	return 0;
}

static PVRSRV_ERROR _DCBufferPinhPinHandleIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCBufferUnpin((DC_PIN_HANDLE) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgeDCBufferPin(IMG_UINT32 ui32DispatchTableEntry,
			IMG_UINT8 * psDCBufferPinIN_UI8,
			IMG_UINT8 * psDCBufferPinOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERPIN *psDCBufferPinIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERPIN *) IMG_OFFSET_ADDR(psDCBufferPinIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERPIN *psDCBufferPinOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERPIN *) IMG_OFFSET_ADDR(psDCBufferPinOUT_UI8, 0);

	IMG_HANDLE hBuffer = psDCBufferPinIN->hBuffer;
	DC_BUFFER *psBufferInt = NULL;
	DC_PIN_HANDLE hPinHandleInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCBufferPinOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psBufferInt,
				       hBuffer, PVRSRV_HANDLE_TYPE_DC_BUFFER, IMG_TRUE);
	if (unlikely(psDCBufferPinOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferPin_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCBufferPinOUT->eError = DCBufferPin(psBufferInt, &hPinHandleInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCBufferPinOUT->eError != PVRSRV_OK))
	{
		goto DCBufferPin_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psHandleBase);

	psDCBufferPinOUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,
							     &psDCBufferPinOUT->hPinHandle,
							     (void *)hPinHandleInt,
							     PVRSRV_HANDLE_TYPE_DC_PIN_HANDLE,
							     PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
							     (PFN_HANDLE_RELEASE) &
							     _DCBufferPinhPinHandleIntRelease);
	if (unlikely(psDCBufferPinOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferPin_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psHandleBase);

DCBufferPin_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psBufferInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hBuffer, PVRSRV_HANDLE_TYPE_DC_BUFFER);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psDCBufferPinOUT->eError != PVRSRV_OK)
	{
		if (hPinHandleInt)
		{
			DCBufferUnpin(hPinHandleInt);
		}
	}

	return 0;
}

static IMG_INT
PVRSRVBridgeDCBufferUnpin(IMG_UINT32 ui32DispatchTableEntry,
			  IMG_UINT8 * psDCBufferUnpinIN_UI8,
			  IMG_UINT8 * psDCBufferUnpinOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERUNPIN *psDCBufferUnpinIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERUNPIN *) IMG_OFFSET_ADDR(psDCBufferUnpinIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERUNPIN *psDCBufferUnpinOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERUNPIN *) IMG_OFFSET_ADDR(psDCBufferUnpinOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psHandleBase);

	psDCBufferUnpinOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psHandleBase,
					      (IMG_HANDLE) psDCBufferUnpinIN->hPinHandle,
					      PVRSRV_HANDLE_TYPE_DC_PIN_HANDLE);
	if (unlikely((psDCBufferUnpinOUT->eError != PVRSRV_OK) &&
		     (psDCBufferUnpinOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psDCBufferUnpinOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s", __func__, PVRSRVGetErrorString(psDCBufferUnpinOUT->eError)));
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferUnpin_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psHandleBase);

DCBufferUnpin_exit:

	return 0;
}

static PVRSRV_ERROR _DCBufferAcquirepsExtMemIntRelease(void *pvData)
{
	PVRSRV_ERROR eError;
	eError = DCBufferRelease((PMR *) pvData);
	return eError;
}

static IMG_INT
PVRSRVBridgeDCBufferAcquire(IMG_UINT32 ui32DispatchTableEntry,
			    IMG_UINT8 * psDCBufferAcquireIN_UI8,
			    IMG_UINT8 * psDCBufferAcquireOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERACQUIRE *psDCBufferAcquireIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERACQUIRE *) IMG_OFFSET_ADDR(psDCBufferAcquireIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERACQUIRE *psDCBufferAcquireOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERACQUIRE *) IMG_OFFSET_ADDR(psDCBufferAcquireOUT_UI8, 0);

	IMG_HANDLE hBuffer = psDCBufferAcquireIN->hBuffer;
	DC_BUFFER *psBufferInt = NULL;
	PMR *psExtMemInt = NULL;

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCBufferAcquireOUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psBufferInt,
				       hBuffer, PVRSRV_HANDLE_TYPE_DC_BUFFER, IMG_TRUE);
	if (unlikely(psDCBufferAcquireOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCBufferAcquire_exit;
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCBufferAcquireOUT->eError = DCBufferAcquire(psBufferInt, &psExtMemInt);
	/* Exit early if bridged call fails */
	if (unlikely(psDCBufferAcquireOUT->eError != PVRSRV_OK))
	{
		goto DCBufferAcquire_exit;
	}

	/* Lock over handle creation. */
	LockHandle(psConnection->psProcessHandleBase->psHandleBase);

	psDCBufferAcquireOUT->eError =
	    PVRSRVAllocHandleUnlocked(psConnection->psProcessHandleBase->psHandleBase,
				      &psDCBufferAcquireOUT->hExtMem, (void *)psExtMemInt,
				      PVRSRV_HANDLE_TYPE_DEVMEM_MEM_IMPORT,
				      PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
				      (PFN_HANDLE_RELEASE) & _DCBufferAcquirepsExtMemIntRelease);
	if (unlikely(psDCBufferAcquireOUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psProcessHandleBase->psHandleBase);
		goto DCBufferAcquire_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle(psConnection->psProcessHandleBase->psHandleBase);

DCBufferAcquire_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psBufferInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hBuffer, PVRSRV_HANDLE_TYPE_DC_BUFFER);
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	if (psDCBufferAcquireOUT->eError != PVRSRV_OK)
	{
		if (psExtMemInt)
		{
			LockHandle(KERNEL_HANDLE_BASE);
			DCBufferRelease(psExtMemInt);
			UnlockHandle(KERNEL_HANDLE_BASE);
		}
	}

	return 0;
}

static IMG_INT
PVRSRVBridgeDCBufferRelease(IMG_UINT32 ui32DispatchTableEntry,
			    IMG_UINT8 * psDCBufferReleaseIN_UI8,
			    IMG_UINT8 * psDCBufferReleaseOUT_UI8, CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCBUFFERRELEASE *psDCBufferReleaseIN =
	    (PVRSRV_BRIDGE_IN_DCBUFFERRELEASE *) IMG_OFFSET_ADDR(psDCBufferReleaseIN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCBUFFERRELEASE *psDCBufferReleaseOUT =
	    (PVRSRV_BRIDGE_OUT_DCBUFFERRELEASE *) IMG_OFFSET_ADDR(psDCBufferReleaseOUT_UI8, 0);

	/* Lock over handle destruction. */
	LockHandle(psConnection->psProcessHandleBase->psHandleBase);

	psDCBufferReleaseOUT->eError =
	    PVRSRVDestroyHandleStagedUnlocked(psConnection->psProcessHandleBase->psHandleBase,
					      (IMG_HANDLE) psDCBufferReleaseIN->hExtMem,
					      PVRSRV_HANDLE_TYPE_DEVMEM_MEM_IMPORT);
	if (unlikely((psDCBufferReleaseOUT->eError != PVRSRV_OK) &&
		     (psDCBufferReleaseOUT->eError != PVRSRV_ERROR_KERNEL_CCB_FULL) &&
		     (psDCBufferReleaseOUT->eError != PVRSRV_ERROR_RETRY)))
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: %s", __func__, PVRSRVGetErrorString(psDCBufferReleaseOUT->eError)));
		UnlockHandle(psConnection->psProcessHandleBase->psHandleBase);
		goto DCBufferRelease_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle(psConnection->psProcessHandleBase->psHandleBase);

DCBufferRelease_exit:

	return 0;
}

static_assert(DC_MAX_PIPE_COUNT <= IMG_UINT32_MAX,
	      "DC_MAX_PIPE_COUNT must not be larger than IMG_UINT32_MAX");

static IMG_INT
PVRSRVBridgeDCDisplayContextConfigure2(IMG_UINT32 ui32DispatchTableEntry,
				       IMG_UINT8 * psDCDisplayContextConfigure2IN_UI8,
				       IMG_UINT8 * psDCDisplayContextConfigure2OUT_UI8,
				       CONNECTION_DATA * psConnection)
{
	PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCONFIGURE2 *psDCDisplayContextConfigure2IN =
	    (PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCONFIGURE2 *)
	    IMG_OFFSET_ADDR(psDCDisplayContextConfigure2IN_UI8, 0);
	PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCONFIGURE2 *psDCDisplayContextConfigure2OUT =
	    (PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCONFIGURE2 *)
	    IMG_OFFSET_ADDR(psDCDisplayContextConfigure2OUT_UI8, 0);

	IMG_HANDLE hDisplayContext = psDCDisplayContextConfigure2IN->hDisplayContext;
	DC_DISPLAY_CONTEXT *psDisplayContextInt = NULL;
	PVRSRV_SURFACE_CONFIG_INFO *psSurfInfoInt = NULL;
	DC_BUFFER **psBuffersInt = NULL;
	IMG_HANDLE *hBuffersInt2 = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE *pArrayArgsBuffer = NULL;
	IMG_BOOL bHaveEnoughSpace = IMG_FALSE;

	IMG_UINT32 ui32BufferSize = 0;
	IMG_UINT64 ui64BufferSize =
	    ((IMG_UINT64) psDCDisplayContextConfigure2IN->ui32PipeCount *
	     sizeof(PVRSRV_SURFACE_CONFIG_INFO)) +
	    ((IMG_UINT64) psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(DC_BUFFER *)) +
	    ((IMG_UINT64) psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(IMG_HANDLE)) + 0;

	if (unlikely(psDCDisplayContextConfigure2IN->ui32PipeCount > DC_MAX_PIPE_COUNT))
	{
		psDCDisplayContextConfigure2OUT->eError = PVRSRV_ERROR_BRIDGE_ARRAY_SIZE_TOO_BIG;
		goto DCDisplayContextConfigure2_exit;
	}

	if (ui64BufferSize > IMG_UINT32_MAX)
	{
		psDCDisplayContextConfigure2OUT->eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
		goto DCDisplayContextConfigure2_exit;
	}

	ui32BufferSize = (IMG_UINT32) ui64BufferSize;

	if (ui32BufferSize != 0)
	{
		/* Try to use remainder of input buffer for copies if possible, word-aligned for safety. */
		IMG_UINT32 ui32InBufferOffset =
		    PVR_ALIGN(sizeof(*psDCDisplayContextConfigure2IN), sizeof(unsigned long));
		IMG_UINT32 ui32InBufferExcessSize =
		    ui32InBufferOffset >=
		    PVRSRV_MAX_BRIDGE_IN_SIZE ? 0 : PVRSRV_MAX_BRIDGE_IN_SIZE - ui32InBufferOffset;

		bHaveEnoughSpace = ui32BufferSize <= ui32InBufferExcessSize;
		if (bHaveEnoughSpace)
		{
			IMG_BYTE *pInputBuffer =
			    (IMG_BYTE *) (void *)psDCDisplayContextConfigure2IN;

			pArrayArgsBuffer = &pInputBuffer[ui32InBufferOffset];
		}
		else
		{
			pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

			if (!pArrayArgsBuffer)
			{
				psDCDisplayContextConfigure2OUT->eError =
				    PVRSRV_ERROR_OUT_OF_MEMORY;
				goto DCDisplayContextConfigure2_exit;
			}
		}
	}

	if (psDCDisplayContextConfigure2IN->ui32PipeCount != 0)
	{
		psSurfInfoInt =
		    (PVRSRV_SURFACE_CONFIG_INFO *) IMG_OFFSET_ADDR(pArrayArgsBuffer,
								   ui32NextOffset);
		ui32NextOffset +=
		    psDCDisplayContextConfigure2IN->ui32PipeCount *
		    sizeof(PVRSRV_SURFACE_CONFIG_INFO);
	}

	/* Copy the data over */
	if (psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(PVRSRV_SURFACE_CONFIG_INFO) > 0)
	{
		if (OSCopyFromUser
		    (NULL, psSurfInfoInt,
		     (const void __user *)psDCDisplayContextConfigure2IN->psSurfInfo,
		     psDCDisplayContextConfigure2IN->ui32PipeCount *
		     sizeof(PVRSRV_SURFACE_CONFIG_INFO)) != PVRSRV_OK)
		{
			psDCDisplayContextConfigure2OUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDisplayContextConfigure2_exit;
		}
	}
	if (psDCDisplayContextConfigure2IN->ui32PipeCount != 0)
	{
		psBuffersInt = (DC_BUFFER **) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		OSCachedMemSet(psBuffersInt, 0,
			       psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(DC_BUFFER *));
		ui32NextOffset +=
		    psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(DC_BUFFER *);
		hBuffersInt2 = (IMG_HANDLE *) IMG_OFFSET_ADDR(pArrayArgsBuffer, ui32NextOffset);
		ui32NextOffset +=
		    psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(IMG_HANDLE);
	}

	/* Copy the data over */
	if (psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(IMG_HANDLE) > 0)
	{
		if (OSCopyFromUser
		    (NULL, hBuffersInt2,
		     (const void __user *)psDCDisplayContextConfigure2IN->phBuffers,
		     psDCDisplayContextConfigure2IN->ui32PipeCount * sizeof(IMG_HANDLE)) !=
		    PVRSRV_OK)
		{
			psDCDisplayContextConfigure2OUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

			goto DCDisplayContextConfigure2_exit;
		}
	}

	/* Lock over handle lookup. */
	LockHandle(psConnection->psHandleBase);

	/* Look up the address from the handle */
	psDCDisplayContextConfigure2OUT->eError =
	    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
				       (void **)&psDisplayContextInt,
				       hDisplayContext,
				       PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT, IMG_TRUE);
	if (unlikely(psDCDisplayContextConfigure2OUT->eError != PVRSRV_OK))
	{
		UnlockHandle(psConnection->psHandleBase);
		goto DCDisplayContextConfigure2_exit;
	}

	{
		IMG_UINT32 i;

		for (i = 0; i < psDCDisplayContextConfigure2IN->ui32PipeCount; i++)
		{
			/* Look up the address from the handle */
			psDCDisplayContextConfigure2OUT->eError =
			    PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
						       (void **)&psBuffersInt[i],
						       hBuffersInt2[i],
						       PVRSRV_HANDLE_TYPE_DC_BUFFER, IMG_TRUE);
			if (unlikely(psDCDisplayContextConfigure2OUT->eError != PVRSRV_OK))
			{
				UnlockHandle(psConnection->psHandleBase);
				goto DCDisplayContextConfigure2_exit;
			}
		}
	}
	/* Release now we have looked up handles. */
	UnlockHandle(psConnection->psHandleBase);

	psDCDisplayContextConfigure2OUT->eError =
	    DCDisplayContextConfigure(psDisplayContextInt,
				      psDCDisplayContextConfigure2IN->ui32PipeCount,
				      psSurfInfoInt,
				      psBuffersInt,
				      psDCDisplayContextConfigure2IN->ui32DisplayPeriod,
				      psDCDisplayContextConfigure2IN->ui32MaxDepth,
				      psDCDisplayContextConfigure2IN->hAcquireFence,
				      psDCDisplayContextConfigure2IN->hReleaseFenceTimeline,
				      &psDCDisplayContextConfigure2OUT->hReleaseFence);

DCDisplayContextConfigure2_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle(psConnection->psHandleBase);

	/* Unreference the previously looked up handle */
	if (psDisplayContextInt)
	{
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					    hDisplayContext, PVRSRV_HANDLE_TYPE_DC_DISPLAY_CONTEXT);
	}

	if (hBuffersInt2)
	{
		IMG_UINT32 i;

		for (i = 0; i < psDCDisplayContextConfigure2IN->ui32PipeCount; i++)
		{

			/* Unreference the previously looked up handle */
			if (psBuffersInt && psBuffersInt[i])
			{
				PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
							    hBuffersInt2[i],
							    PVRSRV_HANDLE_TYPE_DC_BUFFER);
			}
		}
	}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle(psConnection->psHandleBase);

	/* Allocated space should be equal to the last updated offset */
#ifdef PVRSRV_NEED_PVR_ASSERT
	if (psDCDisplayContextConfigure2OUT->eError == PVRSRV_OK)
		PVR_ASSERT(ui32BufferSize == ui32NextOffset);
#endif /* PVRSRV_NEED_PVR_ASSERT */

	if (!bHaveEnoughSpace && pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);

	return 0;
}

/* ***************************************************************************
 * Server bridge dispatch related glue
 */

static POS_LOCK pDCBridgeLock;

PVRSRV_ERROR InitDCBridge(void);
void DeinitDCBridge(void);

/*
 * Register all DC functions with services
 */
PVRSRV_ERROR InitDCBridge(void)
{
	PVR_LOG_RETURN_IF_ERROR(OSLockCreate(&pDCBridgeLock), "OSLockCreate");

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICESQUERYCOUNT,
			      PVRSRVBridgeDCDevicesQueryCount, pDCBridgeLock, 0,
			      sizeof(PVRSRV_BRIDGE_OUT_DCDEVICESQUERYCOUNT));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICESENUMERATE,
			      PVRSRVBridgeDCDevicesEnumerate, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDEVICESENUMERATE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDEVICESENUMERATE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICEACQUIRE,
			      PVRSRVBridgeDCDeviceAcquire, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDEVICEACQUIRE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDEVICEACQUIRE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICERELEASE,
			      PVRSRVBridgeDCDeviceRelease, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDEVICERELEASE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDEVICERELEASE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCGETINFO, PVRSRVBridgeDCGetInfo,
			      pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCGETINFO),
			      sizeof(PVRSRV_BRIDGE_OUT_DCGETINFO));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCPANELQUERYCOUNT,
			      PVRSRVBridgeDCPanelQueryCount, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCPANELQUERYCOUNT),
			      sizeof(PVRSRV_BRIDGE_OUT_DCPANELQUERYCOUNT));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCPANELQUERY,
			      PVRSRVBridgeDCPanelQuery, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCPANELQUERY),
			      sizeof(PVRSRV_BRIDGE_OUT_DCPANELQUERY));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCFORMATQUERY,
			      PVRSRVBridgeDCFormatQuery, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCFORMATQUERY),
			      sizeof(PVRSRV_BRIDGE_OUT_DCFORMATQUERY));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDIMQUERY, PVRSRVBridgeDCDimQuery,
			      pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDIMQUERY),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDIMQUERY));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSETBLANK, PVRSRVBridgeDCSetBlank,
			      pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCSETBLANK),
			      sizeof(PVRSRV_BRIDGE_OUT_DCSETBLANK));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSETVSYNCREPORTING,
			      PVRSRVBridgeDCSetVSyncReporting, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCSETVSYNCREPORTING),
			      sizeof(PVRSRV_BRIDGE_OUT_DCSETVSYNCREPORTING));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCLASTVSYNCQUERY,
			      PVRSRVBridgeDCLastVSyncQuery, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCLASTVSYNCQUERY),
			      sizeof(PVRSRV_BRIDGE_OUT_DCLASTVSYNCQUERY));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSYSTEMBUFFERACQUIRE,
			      PVRSRVBridgeDCSystemBufferAcquire, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCSYSTEMBUFFERACQUIRE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCSYSTEMBUFFERACQUIRE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSYSTEMBUFFERRELEASE,
			      PVRSRVBridgeDCSystemBufferRelease, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCSYSTEMBUFFERRELEASE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCSYSTEMBUFFERRELEASE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTCREATE,
			      PVRSRVBridgeDCDisplayContextCreate, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCREATE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCREATE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTCONFIGURECHECK,
			      PVRSRVBridgeDCDisplayContextConfigureCheck, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCONFIGURECHECK),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCONFIGURECHECK));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTDESTROY,
			      PVRSRVBridgeDCDisplayContextDestroy, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTDESTROY),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTDESTROY));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERALLOC,
			      PVRSRVBridgeDCBufferAlloc, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERALLOC),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERALLOC));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERIMPORT,
			      PVRSRVBridgeDCBufferImport, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERIMPORT),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERIMPORT));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERFREE,
			      PVRSRVBridgeDCBufferFree, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERFREE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERFREE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERUNIMPORT,
			      PVRSRVBridgeDCBufferUnimport, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERUNIMPORT),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERUNIMPORT));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERPIN,
			      PVRSRVBridgeDCBufferPin, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERPIN),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERPIN));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERUNPIN,
			      PVRSRVBridgeDCBufferUnpin, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERUNPIN),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERUNPIN));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERACQUIRE,
			      PVRSRVBridgeDCBufferAcquire, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERACQUIRE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERACQUIRE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERRELEASE,
			      PVRSRVBridgeDCBufferRelease, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCBUFFERRELEASE),
			      sizeof(PVRSRV_BRIDGE_OUT_DCBUFFERRELEASE));

	SetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTCONFIGURE2,
			      PVRSRVBridgeDCDisplayContextConfigure2, pDCBridgeLock,
			      sizeof(PVRSRV_BRIDGE_IN_DCDISPLAYCONTEXTCONFIGURE2),
			      sizeof(PVRSRV_BRIDGE_OUT_DCDISPLAYCONTEXTCONFIGURE2));

	return PVRSRV_OK;
}

/*
 * Unregister all dc functions with services
 */
void DeinitDCBridge(void)
{
	OSLockDestroy(pDCBridgeLock);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICESQUERYCOUNT);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICESENUMERATE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICEACQUIRE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDEVICERELEASE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCGETINFO);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCPANELQUERYCOUNT);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCPANELQUERY);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCFORMATQUERY);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDIMQUERY);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSETBLANK);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSETVSYNCREPORTING);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCLASTVSYNCQUERY);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSYSTEMBUFFERACQUIRE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCSYSTEMBUFFERRELEASE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTCREATE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTCONFIGURECHECK);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTDESTROY);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERALLOC);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERIMPORT);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERFREE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERUNIMPORT);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERPIN);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERUNPIN);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERACQUIRE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCBUFFERRELEASE);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_DC, PVRSRV_BRIDGE_DC_DCDISPLAYCONTEXTCONFIGURE2);

}

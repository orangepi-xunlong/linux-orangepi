/*************************************************************************/ /*!
@File           vmm_type_vzfpga.c
@Title          VZFPGA manager type
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    VM manager implementation to support vzfpga platform
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
#include "img_types.h"
#include "img_defs.h"
#include "pvrsrv_error.h"
#include "rgxheapconfig.h"
#include "rgxdevice.h"

#include "vz_vm.h"
#include "vmm_impl.h"
#include "vmm_pvz_server.h"

static PVRSRV_ERROR
GetDriverIDFromHeapBase(IMG_UINT64 ui64Addr, IMG_UINT32 *pui32DriverID)
{
	PVRSRV_ERROR eErr = PVRSRV_OK;
	IMG_DEV_PHYADDR sHostFwHeapBaseDevPA = {0};
	PHYS_HEAP *psHostFwHeap = NULL;
	PVRSRV_DEVICE_NODE *psHostDevNode = PVRSRVGetDeviceInstance(0);

	PVR_LOG_RETURN_IF_FALSE((psHostDevNode != NULL),
							"Host Device Node not initialised.",
							PVRSRV_ERROR_NO_DEVICENODE_FOUND);

	psHostFwHeap = psHostDevNode->apsPhysHeap[FIRST_PHYSHEAP_MAPPED_TO_FW_MAIN_DEVMEM];
	PVR_LOG_RETURN_IF_FALSE((psHostFwHeap != NULL),
							"Host Fw heap not initialised.",
							PVRSRV_ERROR_PHYSHEAP_ID_INVALID);

	eErr = PhysHeapGetDevPAddr(psHostFwHeap, &sHostFwHeapBaseDevPA);
	PVR_LOG_RETURN_IF_ERROR(eErr, "PhysHeapGetDevPAddr");

	*pui32DriverID = (ui64Addr - sHostFwHeapBaseDevPA.uiAddr) / PVRSRV_APPHINT_GUESTFWHEAPSTRIDE;
	PVR_LOG_RETURN_IF_FALSE((*pui32DriverID >= RGXFW_GUEST_DRIVER_ID_START) &&
							(*pui32DriverID < RGX_NUM_DRIVERS_SUPPORTED),
							"Invalid Guest DriverID",
							PVRSRV_ERROR_INVALID_PVZ_OSID);

	return PVRSRV_OK;
}

static PVRSRV_ERROR
VZFPGAMapDevPhysHeap(IMG_UINT64 ui64Size,
					 IMG_UINT64 ui64Addr)
{
	PVRSRV_ERROR eErr = PVRSRV_OK;
	IMG_UINT32 ui32GuestDriverID;

	eErr = GetDriverIDFromHeapBase(ui64Addr, &ui32GuestDriverID);
	PVR_LOG_RETURN_IF_ERROR(eErr, "GetDriverIDFromHeapBase");

	eErr = PvzServerOnVmOnline(ui32GuestDriverID, 0);
	PVR_LOG_RETURN_IF_ERROR(eErr, "PvzServerOnVmOnline");

	eErr = PvzServerMapDevPhysHeap(ui32GuestDriverID, 0, ui64Size, ui64Addr);
	PVR_LOG_RETURN_IF_ERROR(eErr, "PvzServerMapDevPhysHeap");

	return eErr;
}

static PVRSRV_ERROR
VZFPGAUnmapDevPhysHeap(void)
{
	IMG_UINT32 ui32ID;

	/* During shutdown, the Guests will be deinitialised in reverse order */
	for (ui32ID=(RGX_NUM_DRIVERS_SUPPORTED-1);
		 ui32ID >= RGXFW_HOST_DRIVER_ID; ui32ID--)
	{
		if (IsVmOnline(ui32ID, 0))
		{
			PvzServerUnmapDevPhysHeap(ui32ID, 0);
			PvzServerOnVmOffline(ui32ID, 0);
			break;
		}
	}

	return PVRSRV_OK;
}

static VMM_PVZ_CONNECTION gsVZFPGAPvz =
{
	.sClientFuncTab = {
		/* pfnMapDevPhysHeap */
		&VZFPGAMapDevPhysHeap,

		/* pfnUnmapDevPhysHeap */
		&VZFPGAUnmapDevPhysHeap
	},

	.sServerFuncTab = {
		/* pfnMapDevPhysHeap */
		&PvzServerMapDevPhysHeap,

		/* pfnUnmapDevPhysHeap */
		&PvzServerUnmapDevPhysHeap
	},

	.sVmmFuncTab = {
		/* pfnOnVmOnline */
		&PvzServerOnVmOnline,

		/* pfnOnVmOffline */
		&PvzServerOnVmOffline,

		/* pfnVMMConfigure */
		&PvzServerVMMConfigure
	}
};

PVRSRV_ERROR VMMCreatePvzConnection(VMM_PVZ_CONNECTION **psPvzConnection,
									PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_LOG_RETURN_IF_FALSE((NULL != psPvzConnection), "VMMCreatePvzConnection", PVRSRV_ERROR_INVALID_PARAMS);
	*psPvzConnection = &gsVZFPGAPvz;
	return PVRSRV_OK;
}

void VMMDestroyPvzConnection(VMM_PVZ_CONNECTION *psPvzConnection,
							 PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_LOG_IF_FALSE((NULL != psPvzConnection), "VMMDestroyPvzConnection");
}

/******************************************************************************
 End of file (vmm_type_vzfpga.c)
******************************************************************************/

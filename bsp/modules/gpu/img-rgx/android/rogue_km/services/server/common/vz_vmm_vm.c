/*************************************************************************/ /*!
@File			vz_vmm_vm.c
@Title          System virtualization VM support APIs
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System virtualization VM support functions
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
#include "osfunc.h"
#include "pvrsrv.h"
#include "img_defs.h"
#include "img_types.h"
#include "pvrsrv.h"
#include "pvrsrv_error.h"
#include "vz_vm.h"
#include "rgxfwutils.h"

bool IsVmOnline(IMG_UINT32 ui32OSID, IMG_UINT32 ui32DevID)
{
	PVRSRV_DEVICE_NODE *psDevNode = PVRSRVGetDeviceInstanceByOSId(ui32DevID);

	return BIT_ISSET(psDevNode->ui32VmState, ui32OSID);
}

PVRSRV_ERROR PvzOnVmOnline(IMG_UINT32 ui32OSID, IMG_UINT32 ui32DevID)
{
#if defined(RGX_NUM_OS_SUPPORTED) && (RGX_NUM_OS_SUPPORTED == 1)
	PVRSRV_ERROR       eError          = PVRSRV_ERROR_INVALID_PARAMS;
#else
	PVRSRV_ERROR       eError          = PVRSRV_OK;
	PVRSRV_DEVICE_NODE *psDevNode;

	psDevNode = PVRSRVGetDeviceInstanceByOSId(ui32DevID);

	if (BIT_ISSET(psDevNode->ui32VmState, ui32OSID))
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: OSID %u on Device %u is already enabled.",
				 __func__, ui32OSID, ui32DevID));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e0;
	}

	if (psDevNode->eDevState == PVRSRV_DEVICE_STATE_INIT)
	{
		/* Firmware not initialized yet, do it here */
		eError = PVRSRVCommonDeviceInitialise(psDevNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: failed to initialize firmware (%s)",
					 __func__, PVRSRVGetErrorString(eError)));
			goto e0;
		}
	}

	eError = RGXFWHealthCheckCmd(psDevNode->pvDevice);
	if (eError != PVRSRV_OK)
	{
		goto e0;
	}

	BIT_SET(psDevNode->ui32VmState, ui32OSID);

#if defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
	/* Everything is ready for the firmware to start interacting with this OS */
	eError = RGXFWSetFwOsState(psDevNode->pvDevice, ui32OSID, RGXFWIF_OS_ONLINE);
#endif

e0:
#endif
	return eError;
}

PVRSRV_ERROR PvzOnVmOffline(IMG_UINT32 ui32OSID, IMG_UINT32 ui32DevID)
{
#if defined(RGX_NUM_OS_SUPPORTED) && (RGX_NUM_OS_SUPPORTED == 1)
	PVRSRV_ERROR       eError          = PVRSRV_ERROR_INVALID_PARAMS;
#else
	PVRSRV_ERROR      eError          = PVRSRV_OK;
	PVRSRV_DEVICE_NODE *psDevNode;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	psDevNode = PVRSRVGetDeviceInstanceByOSId(ui32DevID);

	if (!BIT_ISSET(psDevNode->ui32VmState, ui32OSID))
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: OSID %u on Device %u is already disabled.",
				 __func__, ui32OSID, ui32DevID));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e0;
	}

	psDevInfo = psDevNode->pvDevice;

	eError = RGXFWSetFwOsState(psDevInfo, ui32OSID, RGXFWIF_OS_OFFLINE);
	if (eError == PVRSRV_OK)
	{
		BIT_UNSET(psDevNode->ui32VmState, ui32OSID);
	}

e0:
#endif
	return eError;
}

PVRSRV_ERROR PvzVMMConfigure(VMM_CONF_PARAM eVMMParamType,
							 IMG_UINT32 ui32ParamValue,
							 IMG_UINT32 ui32DevID)
{
#if defined(SUPPORT_RGX)
	PVRSRV_DEVICE_NODE *psDevNode = PVRSRVGetDeviceInstanceByOSId(ui32DevID);
	PVRSRV_RGXDEV_INFO *psDevInfo = psDevNode->pvDevice;
	PVRSRV_ERROR eError;

	switch (eVMMParamType)
	{
		case VMM_CONF_PRIO_OSID0:
		case VMM_CONF_PRIO_OSID1:
		case VMM_CONF_PRIO_OSID2:
		case VMM_CONF_PRIO_OSID3:
		case VMM_CONF_PRIO_OSID4:
		case VMM_CONF_PRIO_OSID5:
		case VMM_CONF_PRIO_OSID6:
		case VMM_CONF_PRIO_OSID7:
		{
			IMG_UINT32 ui32OSID = eVMMParamType;
			IMG_UINT32 ui32Prio = ui32ParamValue;

			if (ui32OSID < RGX_NUM_OS_SUPPORTED)
			{
				eError = RGXFWChangeOSidPriority(psDevInfo, ui32OSID, ui32Prio);
			}
			else
			{
				eError = PVRSRV_ERROR_INVALID_PARAMS;
			}
			break;
		}
		case VMM_CONF_HCS_DEADLINE:
		{
			IMG_UINT32 ui32HCSDeadline = ui32ParamValue;
			eError = RGXFWSetHCSDeadline(psDevInfo, ui32HCSDeadline);
			break;
		}
		default:
		{
			eError = PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	return eError;
#else
	PVR_UNREFERENCED_PARAMETER(eVMMParamType);
	PVR_UNREFERENCED_PARAMETER(ui32ParamValue);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);

	return PVRSRV_ERROR_INVALID_PARAMS;
#endif
}

/******************************************************************************
 End of file (vz_vmm_vm.c)
******************************************************************************/

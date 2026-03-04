/*************************************************************************/ /*!
@File
@Title          PVR device dependent bridge Init/Deinit Module (kernel side)
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements device dependent PVR Bridge init/deinit code
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

#include "img_types.h"
#include "img_defs.h"
#include "rgx_bridge_init.h"
#include "rgxdevice.h"

PVRSRV_ERROR InitRGXTQ2Bridge(void);
void DeinitRGXTQ2Bridge(void);
PVRSRV_ERROR InitRGXCMPBridge(void);
void DeinitRGXCMPBridge(void);
#if defined(SUPPORT_RGXRAY_BRIDGE)
PVRSRV_ERROR InitRGXRAYBridge(void);
void DeinitRGXRAYBridge(void);
#endif

/* Reference counts for device-conditional
 * bridges. This ensures that bridges remain
 * valid while there are still devices using
 * them.
 */
static ATOMIC_T i32RGXCMPBridgeRefCt;
static ATOMIC_T i32RGXTQ2BridgeRefCt;
#if defined(SUPPORT_RGXRAY_BRIDGE)
static ATOMIC_T i32RGXRayBridgeRefCt;
#endif
static IMG_BOOL bAtomicsInitialised = IMG_FALSE;

void RGXBridgeDriverInit(void)
{
	if (!bAtomicsInitialised)
	{
		bAtomicsInitialised = IMG_TRUE;
		OSAtomicWrite(&i32RGXCMPBridgeRefCt, 0);
		OSAtomicWrite(&i32RGXTQ2BridgeRefCt, 0);
#if defined(SUPPORT_RGXRAY_BRIDGE)
		OSAtomicWrite(&i32RGXRayBridgeRefCt, 0);
#endif
	}
}

PVRSRV_ERROR RGXRegisterBridges(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR eError;

	if (!bAtomicsInitialised)
	{
		eError = PVRSRV_ERROR_NOT_INITIALISED;
		PVR_LOG_RETURN_IF_ERROR(eError, "RGXBridgeRefCts");
	}

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, COMPUTE))
	{
		if (OSAtomicIncrement(&i32RGXCMPBridgeRefCt) == 1)
		{
			eError = InitRGXCMPBridge();
			if (eError != PVRSRV_OK)
			{
				OSAtomicDecrement(&i32RGXCMPBridgeRefCt);
			}
			PVR_LOG_RETURN_IF_ERROR(eError, "InitRGXCMPBridge");
		}
	}

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, FASTRENDER_DM))
	{
		if (OSAtomicIncrement(&i32RGXTQ2BridgeRefCt) == 1)
		{
			eError = InitRGXTQ2Bridge();
			if (eError != PVRSRV_OK)
			{
				OSAtomicDecrement(&i32RGXTQ2BridgeRefCt);
			}
			PVR_LOG_RETURN_IF_ERROR(eError, "InitRGXTQ2Bridge");
		}
	}

#if defined(SUPPORT_RGXRAY_BRIDGE)
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, RAY_TRACING_ARCH) &&
		RGX_GET_FEATURE_VALUE(psDevInfo, RAY_TRACING_ARCH) > 0)
	{
		if (OSAtomicIncrement(&i32RGXRayBridgeRefCt) == 1)
		{
			eError = InitRGXRAYBridge();
			if (eError != PVRSRV_OK)
			{
				OSAtomicDecrement(&i32RGXRayBridgeRefCt);
			}
			PVR_LOG_RETURN_IF_ERROR(eError, "InitRGXRAYBridge");
		}
	}
#endif

	return PVRSRV_OK;
}

void RGXUnregisterBridges(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, COMPUTE))
	{
		if (OSAtomicDecrement(&i32RGXCMPBridgeRefCt) == 0)
		{
			DeinitRGXCMPBridge();
		}
	}

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, FASTRENDER_DM))
	{
		if (OSAtomicDecrement(&i32RGXTQ2BridgeRefCt) == 0)
		{
			DeinitRGXTQ2Bridge();
		}
	}

#if defined(SUPPORT_RGXRAY_BRIDGE)
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, RAY_TRACING_ARCH) &&
		RGX_GET_FEATURE_VALUE(psDevInfo, RAY_TRACING_ARCH) > 0)
	{
		if (OSAtomicDecrement(&i32RGXRayBridgeRefCt) == 0)
		{
			DeinitRGXRAYBridge();
		}
	}
#endif
}

/*************************************************************************/ /*!
@File           rgxmulticore.c
@Title          Functions related to multicore devices
@Codingstyle    IMG
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Kernel mode workload estimation functionality.
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

#include "rgxdevice.h"
#include "rgxdefs_km.h"
#include "pdump_km.h"
#include "rgxmulticore.h"
#include "multicore_defs.h"
#include "allocmem.h"
#include "pvr_debug.h"
#include "rgxfwmemctx.h"

/*
 * check that register defines match our hardcoded definitions.
 * Rogue has these, volcanic does not.
 */
#if ((RGX_MULTICORE_CAPABILITY_FRAGMENT_EN != RGX_CR_MULTICORE_GPU_CAPABILITY_FRAGMENT_EN) || \
     (RGX_MULTICORE_CAPABILITY_GEOMETRY_EN != RGX_CR_MULTICORE_GPU_CAPABILITY_GEOMETRY_EN) || \
     (RGX_MULTICORE_CAPABILITY_COMPUTE_EN  != RGX_CR_MULTICORE_GPU_CAPABILITY_COMPUTE_EN) || \
     (RGX_MULTICORE_CAPABILITY_PRIMARY_EN  != RGX_CR_MULTICORE_GPU_CAPABILITY_PRIMARY_EN) || \
     (RGX_MULTICORE_ID_CLRMSK              != RGX_CR_MULTICORE_GPU_ID_CLRMSK))
#error "Rogue definitions for RGX_CR_MULTICORE_GPU register have changed"
#endif


static PVRSRV_ERROR RGXGetMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode,
                                        IMG_UINT32 ui32CapsSize,
                                        IMG_UINT32 *pui32NumCores,
                                        IMG_UINT64 *pui64Caps);


/*
 * RGXInitMultiCoreInfo:
 * Return multicore information to clients.
 * Return not supported on cores without multicore.
 */
static PVRSRV_ERROR RGXGetMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode,
                                 IMG_UINT32 ui32CapsSize,
                                 IMG_UINT32 *pui32NumCores,
                                 IMG_UINT64 *pui64Caps)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psDevInfo->ui32MultiCoreNumCores == 0)
	{
		/* MULTICORE not supported on this device */
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
	}
	else
	{
		*pui32NumCores = psDevInfo->ui32MultiCoreNumCores;
		if (ui32CapsSize > 0)
		{
			if (ui32CapsSize < psDevInfo->ui32MultiCoreNumCores)
			{
				PVR_DPF((PVR_DBG_ERROR, "Multicore caps buffer too small"));
				eError = PVRSRV_ERROR_BRIDGE_BUFFER_TOO_SMALL;
			}
			else
			{
				IMG_UINT32 i;

				for (i = 0; i < psDevInfo->ui32MultiCoreNumCores; ++i)
				{
					pui64Caps[i] = psDevInfo->pui64MultiCoreCapabilities[i];
				}
			}
		}
	}

	return eError;
}



/*
 * RGXInitMultiCoreInfo:
 * Read multicore HW registers and fill in data structure for clients.
 * Return not supported on cores without multicore.
 */
PVRSRV_ERROR RGXInitMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psDeviceNode->pfnGetMultiCoreInfo != NULL)
	{
		/* we only set this up once */
		return PVRSRV_OK;
	}

	/* defaults for non-multicore devices */
	psDevInfo->ui32MultiCoreNumCores = 0;
	psDevInfo->ui32MultiCorePrimaryId = (IMG_UINT32)(-1);
	psDevInfo->pui64MultiCoreCapabilities = NULL;
	psDeviceNode->pfnGetMultiCoreInfo = NULL;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
	{
		IMG_BOOL bPowerWasDown;
		IMG_UINT32 ui32MulticoreRegBankOffset = (1 << RGX_GET_FEATURE_VALUE(psDevInfo, XPU_MAX_REGBANKS_ADDR_WIDTH));
		IMG_UINT32 ui32MulticoreGPUReg = RGX_CR_MULTICORE_GPU;
		IMG_UINT32 ui32NumCores;
		IMG_UINT32 i;

#if defined(RGX_HOST_SECURE_REGBANK_OFFSET) && defined(XPU_MAX_REGBANKS_ADDR_WIDTH)
		/* Ensure the HOST_SECURITY reg bank definitions are correct */
		if ((RGX_HOST_SECURE_REGBANK_OFFSET + RGX_HOST_SECURE_REGBANK_SIZE) != ui32MulticoreRegBankOffset)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Register bank definitions for HOST_SECURITY don't match core's configuration.", __func__));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
#endif
		bPowerWasDown = ! PVRSRVIsSystemPowered(psDeviceNode);

		/* Power-up the device as required to read the registers */
		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && bPowerWasDown)
		{
			PVRSRVPowerLock(psDeviceNode);
			eError = PVRSRVSetSystemPowerState(psDeviceNode->psDevConfig, PVRSRV_SYS_POWER_STATE_ON);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVSetSystemPowerState ON failed (%u)", __func__, eError));
				PVRSRVPowerUnlock(psDeviceNode);
				return eError;
			}
		}

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			IMG_UINT32 ui32FwTimeout = MAX_HW_TIME_US;

			LOOP_UNTIL_TIMEOUT_US(ui32FwTimeout)
			{
				RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfOsInit->sRGXMulticoreInfo.ui32NumCores,
		                               INVALIDATE);
				if (*((volatile IMG_UINT32*)&psDevInfo->psRGXFWIfOsInit->sRGXMulticoreInfo.ui32NumCores))
				{
					/* No need to wait if the FW has already updated the values */
					break;
				}
				OSWaitus(ui32FwTimeout/WAIT_TRY_COUNT);
			} END_LOOP_UNTIL_TIMEOUT_US();

			if (*((volatile IMG_UINT32*)&psDevInfo->psRGXFWIfOsInit->sRGXMulticoreInfo.ui32NumCores) == 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "Multicore info not available for guest"));
				return PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
			}

			ui32NumCores = psDevInfo->psRGXFWIfOsInit->sRGXMulticoreInfo.ui32NumCores;

			PVR_LOG(("RGX Guest Device initialised with %u %s",
					 ui32NumCores, (ui32NumCores == 1U) ? "core" : "cores"));
		}
		else
#endif
		{
			ui32NumCores = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE_SYSTEM);
		}
#if !defined(NO_HARDWARE)
		/* check that the number of cores reported is in-bounds */
		if (ui32NumCores > (RGX_CR_MULTICORE_SYSTEM_MASKFULL >> RGX_CR_MULTICORE_SYSTEM_GPU_COUNT_SHIFT))
		{
			PVR_DPF((PVR_DBG_ERROR, "invalid return (%u) read from MULTICORE_SYSTEM", ui32NumCores));
			return PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
		}
#else
		/* for nohw set to max so clients can allocate enough memory for all pdump runs on any config */
		ui32NumCores = RGX_MULTICORE_MAX_NOHW_CORES;
#endif
		PVR_DPF((PVR_DBG_MESSAGE, "Multicore system has %u cores", ui32NumCores));
		PDUMPCOMMENT(psDeviceNode, "RGX Multicore has %d cores\n", ui32NumCores);

		/* allocate storage for capabilities */
		psDevInfo->pui64MultiCoreCapabilities = OSAllocMem(ui32NumCores * sizeof(psDevInfo->pui64MultiCoreCapabilities[0]));
		PVR_LOG_GOTO_IF_NOMEM(psDevInfo->pui64MultiCoreCapabilities, eError, err);

		psDevInfo->ui32MultiCoreNumCores = ui32NumCores;

		for (i = 0; i < ui32NumCores; ++i)
		{
	#if !defined(NO_HARDWARE)
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
			if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
			{
				psDevInfo->pui64MultiCoreCapabilities[i] = psDevInfo->psRGXFWIfOsInit->sRGXMulticoreInfo.aui64MultiCoreCapabilities[i];
			}
			else
#endif
			{
				IMG_BOOL bMulticoreRegsMapped = (psDeviceNode->psDevConfig->ui32RegsSize > ui32MulticoreGPUReg);
				void __iomem *pvCoreRegBase;
				IMG_INT32 ui32MulticoreRegOffset;

				if (bMulticoreRegsMapped)
				{
					pvCoreRegBase = psDevInfo->pvRegsBaseKM;
					ui32MulticoreRegOffset = ui32MulticoreGPUReg;
				}
				else
				{
					/* the register bank of this core is not mapped */
					IMG_CPU_PHYADDR sMultiCoreRegsBase = psDeviceNode->psDevConfig->sRegsCpuPBase;

					sMultiCoreRegsBase.uiAddr += i*ui32MulticoreRegBankOffset;
					pvCoreRegBase = (void __iomem *) OSMapPhysToLin(sMultiCoreRegsBase, psDeviceNode->psDevConfig->ui32RegsSize, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
					PVR_LOG_GOTO_IF_NOMEM(pvCoreRegBase, eError, err);

					/* adjust the register offset to point inside the newly mapped range */
					ui32MulticoreRegOffset = RGX_CR_MULTICORE_GPU;
				}

				psDevInfo->pui64MultiCoreCapabilities[i] =
							OSReadHWReg64(pvCoreRegBase, ui32MulticoreRegOffset) & RGX_CR_MULTICORE_GPU_MASKFULL;

				if (!bMulticoreRegsMapped)
				{
					OSUnMapPhysToLin((void __force *) pvCoreRegBase, psDeviceNode->psDevConfig->ui32RegsSize);
				}
			}
	#else
			/* emulation for what we think caps are */
			psDevInfo->pui64MultiCoreCapabilities[i] =
							   i | ((i == 0) ? (RGX_MULTICORE_CAPABILITY_PRIMARY_EN
											  | RGX_MULTICORE_CAPABILITY_GEOMETRY_EN) : 0)
							   | RGX_MULTICORE_CAPABILITY_COMPUTE_EN
							   | RGX_MULTICORE_CAPABILITY_FRAGMENT_EN;
	#endif
			PVR_DPF((PVR_DBG_MESSAGE, "Core %d has capabilities value 0x%x", i, (IMG_UINT32)psDevInfo->pui64MultiCoreCapabilities[i] ));
			PDUMPCOMMENT(psDeviceNode, "\tCore %d has caps 0x%08x\n", i,
			             (IMG_UINT32)psDevInfo->pui64MultiCoreCapabilities[i]);

			if (psDevInfo->pui64MultiCoreCapabilities[i] & RGX_CR_MULTICORE_GPU_CAPABILITY_PRIMARY_EN)
			{
				psDevInfo->ui32MultiCorePrimaryId = (psDevInfo->pui64MultiCoreCapabilities[i]
														& ~RGX_CR_MULTICORE_GPU_ID_CLRMSK)
														>> RGX_CR_MULTICORE_GPU_ID_SHIFT;
			}

			ui32MulticoreGPUReg += ui32MulticoreRegBankOffset;
		}

        /* revert power state to what it was on entry to this function */
		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && bPowerWasDown)
		{
			eError = PVRSRVSetSystemPowerState(psDeviceNode->psDevConfig, PVRSRV_SYS_POWER_STATE_OFF);
			PVRSRVPowerUnlock(psDeviceNode);
			PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState OFF");
		}

		/* Register callback to return info about multicore setup to client bridge */
		psDeviceNode->pfnGetMultiCoreInfo = RGXGetMultiCoreInfo;
	}
	else
	{
		/* MULTICORE not supported on this device */
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
	}

	return eError;

err:
	RGXDeInitMultiCoreInfo(psDeviceNode);
	return eError;
}


/*
 * RGXDeinitMultiCoreInfo:
 * Release resources and clear the MultiCore values in the DeviceNode.
 */
void RGXDeInitMultiCoreInfo(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (psDevInfo->pui64MultiCoreCapabilities != NULL)
	{
		OSFreeMem(psDevInfo->pui64MultiCoreCapabilities);
		psDevInfo->pui64MultiCoreCapabilities = NULL;
		psDevInfo->ui32MultiCoreNumCores = 0;
		psDevInfo->ui32MultiCorePrimaryId = (IMG_UINT32)(-1);
	}
	psDeviceNode->pfnGetMultiCoreInfo = NULL;
}

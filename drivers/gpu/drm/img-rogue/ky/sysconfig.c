/*************************************************************************/ /*!
@File
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
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

#include <linux/platform_device.h>

#include "interrupt_support.h"
#include "pvrsrv_device.h"
#include "syscommon.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#include "ky_init.h"
#include "vz_vmm_pvz.h"

static RGX_TIMING_INFORMATION	gsRGXTimingInfo;
static RGX_DATA					gsRGXData;
static PVRSRV_DEVICE_CONFIG		gsDevices[1];
static PHYS_HEAP_FUNCTIONS		gsPhysHeapFuncs;
static PHYS_HEAP_CONFIG			gsPhysHeapConfig[3];

/*
	CPU to Device physical address translation
*/
static
void UMAPhysHeapCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
								   IMG_UINT32 ui32NumOfAddr,
								   IMG_DEV_PHYADDR *psDevPAddr,
								   IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = phys_cpu2gpu(psCpuPAddr[0].uiAddr);
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = phys_cpu2gpu(psCpuPAddr[ui32Idx].uiAddr);
		}
	}
}

/*
	Device to CPU physical address translation
*/
static
void UMAPhysHeapDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
								   IMG_UINT32 ui32NumOfAddr,
								   IMG_CPU_PHYADDR *psCpuPAddr,
								   IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = phys_gpu2cpu(psDevPAddr[0].uiAddr);
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = phys_gpu2cpu(psDevPAddr[ui32Idx].uiAddr);
		}
	}
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	IMG_UINT32 ui32NextPhysHeapID = 0;
	int iIrq;
	struct resource *psDevMemRes = NULL;
	struct platform_device *psDev;

	psDev = to_platform_device((struct device *)pvOSDevice);

	if (gsDevices[0].pvOSDevice)
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	/*
	 * Setup information about physical memory heap(s) we have
	 */
	gsPhysHeapFuncs.pfnCpuPAddrToDevPAddr = UMAPhysHeapCpuPAddrToDevPAddr;
	gsPhysHeapFuncs.pfnDevPAddrToCpuPAddr = UMAPhysHeapDevPAddrToCpuPAddr;

	gsPhysHeapConfig[0].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	gsPhysHeapConfig[0].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[0].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[0].uConfig.sUMA.hPrivData = NULL;
	gsPhysHeapConfig[0].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;
	ui32NextPhysHeapID += 1;

	/*
	 * Setup RGX specific timing data
	 */
	gsRGXTimingInfo.ui32CoreClockSpeed        = RGX_ST_CORE_CLOCK_SPEED;
	gsRGXTimingInfo.bEnableActivePM           = IMG_FALSE;
	gsRGXTimingInfo.bEnableRDPowIsland        = IMG_TRUE;
	gsRGXTimingInfo.ui32ActivePMLatencyms     = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/*
	 * Setup RGX specific data
	 */
	gsRGXData.psRGXTimingInfo = &gsRGXTimingInfo;

	/*
	 * Setup RGX device
	 */
	gsDevices[0].pvOSDevice             = pvOSDevice;
	gsDevices[0].pszName                = "ky";

	/* Device setup information */

	psDevMemRes = platform_get_resource(psDev, IORESOURCE_MEM, 0);
	if (psDevMemRes)
	{
		gsDevices[0].sRegsCpuPBase.uiAddr = psDevMemRes->start;
		gsDevices[0].ui32RegsSize         = (unsigned int)(psDevMemRes->end - psDevMemRes->start);
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: platform_get_resource failed", __func__));
		gsDevices[0].sRegsCpuPBase.uiAddr = ST_GPU_PBASE;
		gsDevices[0].ui32RegsSize         = ST_GPU_SIZE;
	}

	iIrq = platform_get_irq(psDev, 0);
	if (iIrq >= 0)
	{
		gsDevices[0].ui32IRQ  = (IMG_UINT32) iIrq;
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: platform_get_irq failed (%d)", __func__, -iIrq));
		gsDevices[0].ui32IRQ = ST_IRQ_GPU;
	}

	gsDevices[0].eCacheSnoopingMode     = PVRSRV_DEVICE_SNOOP_EMULATED;

	/* Device's physical heaps */
	gsDevices[0].pasPhysHeaps           = &gsPhysHeapConfig[0];
	gsDevices[0].ui32PhysHeapCount      = ui32NextPhysHeapID;
	gsDevices[0].eDefaultHeap = PVRSRV_PHYS_HEAP_GPU_LOCAL;

	/* No power management on ST system */
	gsDevices[0].pfnPrePowerState       = STPrePowerState;
	gsDevices[0].pfnPostPowerState      = STPostPowerState;

	/* No clock frequency either */
	gsDevices[0].pfnClockFreqGet        = NULL;

	//gsDevices[0].pfnCheckMemAllocSize   = NULL;

	gsDevices[0].hDevData               = &gsRGXData;

	gsDevices[0].bHasFBCDCVersion31 = IMG_FALSE;
	gsDevices[0].bDevicePA0IsValid  = IMG_FALSE;

	/* device error notify callback function */
	gsDevices[0].pfnSysDevErrorNotify = NULL;

	/* TODO: ST Init */
	gsDevices[0].hSysData = (IMG_HANDLE)RgxStInit(&gsDevices[0]);
	if (!gsDevices[0].hSysData)
	{
		gsDevices[0].pvOSDevice = NULL;
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	/* Virtualization support services needs to know which heap ID corresponds to FW */
	gsPhysHeapConfig[ui32NextPhysHeapID].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	gsPhysHeapConfig[ui32NextPhysHeapID].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[ui32NextPhysHeapID].uConfig.sUMA.psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[ui32NextPhysHeapID].uConfig.sUMA.hPrivData = NULL;
	gsPhysHeapConfig[ui32NextPhysHeapID].ui32UsageFlags = PHYS_HEAP_USAGE_FW_SHARED;
	gsDevices[0].ui32PhysHeapCount = ++ui32NextPhysHeapID;

	*ppsDevConfig = &gsDevices[0];
	return PVRSRV_OK;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);

	/* TODO:ST UnInit */
	RgxStUnInit(psDevConfig->hSysData);
	psDevConfig->hSysData = NULL;

#if defined(SUPPORT_ION)
	IonDeinit();
#endif

	psDevConfig->pvOSDevice = NULL;
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
								  IMG_UINT32 ui32IRQ,
								  const IMG_CHAR *pszName,
								  PFN_LISR pfnLISR,
								  void *pvData,
								  IMG_HANDLE *phLISRData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);
	return OSInstallSystemLISR(phLISRData, ui32IRQ, pszName, pfnLISR, pvData,
							   SYS_IRQ_FLAG_TRIGGER_DEFAULT);
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return OSUninstallSystemLISR(hLISRData);
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
						  DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
						  void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	PVR_UNREFERENCED_PARAMETER(pvDumpDebugFile);

	return PVRSRV_OK;
}

/*
 * Convert gpu physical address to cpu physical address:
 * ky_x1: gpu(0-2g) --> cpu(0-2g);
 *               gpu(2-xg) --> cpu(2g-(x+2)g); note: x>=2g
 *                                                                  CPU
 * 0x04 8000 0000                                         +-->  +----------+
 *                                                        |     |          |
 *                  DEVICE                 DRAM           |     |          |
 * 0x04 0000 0000+---------+  <------+  +----------+  +---+     |          |
 *               |         |            |          |            |          |
 *               |         |            |          |            |    DDR   |
 *               |         |            |          |            |          |
 *               |         |            |          |            |          |
 *               |         |            |          |            |          |
 * 0x01 0000 0000|         |            |          |      +-->  +----------+
 *               |         |            |          |      |     |          |
 *               |         |            |          |      |     |    IO    |
 * 0x00 8000 0000+---------+  <------+  +----------+  +---+-->  +----------+
 *               |         |            |          |            |          |
 *               |         |            |          |            |    DDR   |
 * 0x00 0000 0000+---------+  <------+  +----------+  +------>  +----------+
 *
 */
unsigned long phys_gpu2cpu(unsigned long phys_addr)
{
#ifdef CONFIG_SOC_KY_X1
	if (phys_addr >= 0x80000000UL) {
		phys_addr += 0x80000000UL;
	}
#endif

	return phys_addr;
}

/*
 * Convert cpu physical address to gpu physical address:
 * ky_x1: cpu(0-2g) --> gpu(0-2g);
 *               cpu(4-xg) --> gpu(2-(x-2)g); note: x>=4g
 *                                                                  CPU
 * 0x04 8000 0000                                         +-->  +----------+
 *                                                        |     |          |
 *                  DEVICE                 DRAM           |     |          |
 * 0x04 0000 0000+---------+  <------+  +----------+  +---+     |          |
 *               |         |            |          |            |          |
 *               |         |            |          |            |    DDR   |
 *               |         |            |          |            |          |
 *               |         |            |          |            |          |
 *               |         |            |          |            |          |
 * 0x01 0000 0000|         |            |          |      +-->  +----------+
 *               |         |            |          |      |     |          |
 *               |         |            |          |      |     |    IO    |
 * 0x00 8000 0000+---------+  <------+  +----------+  +---+-->  +----------+
 *               |         |            |          |            |          |
 *               |         |            |          |            |    DDR   |
 * 0x00 0000 0000+---------+  <------+  +----------+  +------>  +----------+
 *
 */
unsigned long phys_cpu2gpu(unsigned long phys_addr)
{
#ifdef CONFIG_SOC_KY_X1
	if (phys_addr >= 0x100000000UL) {
		phys_addr -= 0x80000000UL;
	}
#endif

	return phys_addr;
}
/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/

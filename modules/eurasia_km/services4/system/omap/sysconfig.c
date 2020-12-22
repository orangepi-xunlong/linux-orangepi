/*************************************************************************/ /*!
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

#include "sysconfig.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include "ocpdefs.h"

/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

static SYS_SPECIFIC_DATA gsSysSpecificData;
SYS_SPECIFIC_DATA *gpsSysSpecificData;

/* SGX structures */
static IMG_UINT32			gui32SGXDeviceID;
static SGX_DEVICE_MAP		gsSGXDeviceMap;
static PVRSRV_DEVICE_NODE	*gpsSGXDevNode;


#if defined(NO_HARDWARE) || defined(SGX_OCP_REGS_ENABLED)
static IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;
#endif

#if defined(PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO)
extern struct platform_device *gpsPVRLDMDev;
#endif

IMG_UINT32 PVRSRV_BridgeDispatchKM(IMG_UINT32	Ioctl,
								   IMG_BYTE		*pInBuf,
								   IMG_UINT32	InBufLen,
								   IMG_BYTE		*pOutBuf,
								   IMG_UINT32	OutBufLen,
								   IMG_UINT32	*pdwBytesTransferred);

#if defined(SGX_OCP_REGS_ENABLED)

static IMG_CPU_VIRTADDR gpvOCPRegsLinAddr;

static PVRSRV_ERROR EnableSGXClocksWrap(SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError = EnableSGXClocks(psSysData);

#if !defined(SGX_OCP_NO_INT_BYPASS)
	if(eError == PVRSRV_OK)
	{
		OSWriteHWReg(gpvOCPRegsLinAddr, EUR_CR_OCP_SYSCONFIG, 0x14);
		OSWriteHWReg(gpvOCPRegsLinAddr, EUR_CR_OCP_DEBUG_CONFIG, EUR_CR_OCP_DEBUG_CONFIG_THALIA_INT_BYPASS_MASK);
	}
#endif
	return eError;
}

#else /* defined(SGX_OCP_REGS_ENABLED) */

static INLINE PVRSRV_ERROR EnableSGXClocksWrap(SYS_DATA *psSysData)
{
	return EnableSGXClocks(psSysData);
}

#endif /* defined(SGX_OCP_REGS_ENABLED) */

static INLINE PVRSRV_ERROR EnableSystemClocksWrap(SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError = EnableSystemClocks(psSysData);

#if !defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	if(eError == PVRSRV_OK)
	{
		/*
		 * The SGX Clocks are enabled separately if active power
		 * management is enabled.
		 */
		eError = EnableSGXClocksWrap(psSysData);
		if (eError != PVRSRV_OK)
		{
			DisableSystemClocks(psSysData);
		}
	}
#endif

	return eError;
}

/*!
******************************************************************************

 @Function	SysLocateDevices

 @Description	Specifies devices in the systems memory map

 @Input		psSysData - sys data

 @Return	PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
#if defined(NO_HARDWARE)
	PVRSRV_ERROR eError;
	IMG_CPU_PHYADDR sCpuPAddr;
#else
#if defined(PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO)
	struct resource *dev_res;
	int dev_irq;
#endif
#endif

	PVR_UNREFERENCED_PARAMETER(psSysData);

	/* SGX Device: */
	gsSGXDeviceMap.ui32Flags = 0x0;
	
#if defined(NO_HARDWARE)
	/* 
	 * For no hardware, allocate some contiguous memory for the
	 * register block.
	 */

	/* Registers */
	gsSGXDeviceMap.ui32RegsSize = SYS_OMAP_SGX_REGS_SIZE;

	eError = OSBaseAllocContigMemory(gsSGXDeviceMap.ui32RegsSize,
									 &gsSGXRegsCPUVAddr,
									 &sCpuPAddr);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	gsSGXDeviceMap.sRegsCpuPBase = sCpuPAddr;
	gsSGXDeviceMap.sRegsSysPBase = SysCpuPAddrToSysPAddr(gsSGXDeviceMap.sRegsCpuPBase);
#if defined(__linux__)
	/* Indicate the registers are already mapped */
	gsSGXDeviceMap.pvRegsCpuVBase = gsSGXRegsCPUVAddr;
#else
	/*
	 * FIXME: Could we just use the virtual address returned by
	 * OSBaseAllocContigMemory?
	 */
	gsSGXDeviceMap.pvRegsCpuVBase = IMG_NULL;
#endif

	OSMemSet(gsSGXRegsCPUVAddr, 0, gsSGXDeviceMap.ui32RegsSize);

	/*
		device interrupt IRQ
		Note: no interrupts available on no hardware system
	*/
	gsSGXDeviceMap.ui32IRQ = 0;

#else /* defined(NO_HARDWARE) */
#if defined(PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO)
	/* get the resource and IRQ through platform resource API */
	dev_res = platform_get_resource(gpsPVRLDMDev, IORESOURCE_MEM, 0);
	if (dev_res == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: platform_get_resource failed", __FUNCTION__));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	dev_irq = platform_get_irq(gpsPVRLDMDev, 0);
	if (dev_irq < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: platform_get_irq failed (%d)", __FUNCTION__, -dev_irq));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = dev_res->start;
	gsSGXDeviceMap.sRegsCpuPBase =
		SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	PVR_TRACE(("SGX register base: 0x%lx", (unsigned long)gsSGXDeviceMap.sRegsCpuPBase.uiAddr));

#if defined(SGX544) && defined(SGX_FEATURE_MP)
	/* FIXME: Workaround due to HWMOD change. Otherwise this region is too small. */
	gsSGXDeviceMap.ui32RegsSize = SYS_OMAP_SGX_REGS_SIZE;
#else
	gsSGXDeviceMap.ui32RegsSize = (unsigned int)(dev_res->end - dev_res->start);
#endif
	PVR_TRACE(("SGX register size: %d",gsSGXDeviceMap.ui32RegsSize));

	gsSGXDeviceMap.ui32IRQ = dev_irq;
	PVR_TRACE(("SGX IRQ: %d", gsSGXDeviceMap.ui32IRQ));
#else	/* defined(PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO) */
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = SYS_OMAP_SGX_REGS_SYS_PHYS_BASE;
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SYS_OMAP_SGX_REGS_SIZE;

	gsSGXDeviceMap.ui32IRQ = SYS_OMAP_SGX_IRQ;

#endif	/* defined(PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO) */
#if defined(SGX_OCP_REGS_ENABLED)
	gsSGXRegsCPUVAddr = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
	gsSGXDeviceMap.ui32RegsSize,
											 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
											 IMG_NULL);

	if (gsSGXRegsCPUVAddr == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Failed to map SGX registers"));
		return PVRSRV_ERROR_BAD_MAPPING;
	}

	/* Indicate the registers are already mapped */
	gsSGXDeviceMap.pvRegsCpuVBase = gsSGXRegsCPUVAddr;
	gpvOCPRegsLinAddr = gsSGXRegsCPUVAddr;
#endif
#endif /* defined(NO_HARDWARE) */

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = "SGXMEM";
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

	/* add other devices here: */


	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysCreateVersionString

 @Description Read the version string 

 @Return   IMG_CHAR *  : Version string

******************************************************************************/
static IMG_CHAR *SysCreateVersionString(void)
{
	static IMG_CHAR aszVersionString[100];
	IMG_UINT32 ui32MaxStrLen;
	SYS_DATA	*psSysData;
	IMG_UINT32	ui32SGXRevision;
	IMG_INT32	i32Count;

	SysAcquireData(&psSysData);

	ui32SGXRevision = SGX_CORE_REV;
	ui32MaxStrLen = 99;

	i32Count = OSSNPrintf(aszVersionString, ui32MaxStrLen + 1,
			"SGX revision = %u",
			(IMG_UINT)(ui32SGXRevision));
	if(i32Count == -1)
	{
		return IMG_NULL;
	}

	return aszVersionString;
}


/*!
******************************************************************************

 @Function	SysInitialise
 
 @Description Initialises kernel services at 'driver load' time
 
 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_UINT32			i;
	PVRSRV_ERROR 		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
#if !defined(PVR_NO_OMAP_TIMER)
	IMG_CPU_PHYADDR		TimerRegPhysBase;
#endif
#if !defined(SGX_DYNAMIC_TIMING_INFO)
	SGX_TIMING_INFORMATION*	psTimingInfo;
#endif
	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	gpsSysSpecificData =  &gsSysSpecificData;
	OSMemSet(gpsSysSpecificData, 0, sizeof(SYS_SPECIFIC_DATA));

	gpsSysData->pvSysSpecificData = gpsSysSpecificData;

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_ENVDATA);

	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;

	/* init device ID's */
	for(i=0; i<SYS_DEVICE_COUNT; i++)
	{
		gpsSysData->sDeviceID[i].uiID = i;
		gpsSysData->sDeviceID[i].bInUse = IMG_FALSE;
	}

	gpsSysData->psDeviceNodeList = IMG_NULL;
	gpsSysData->psQueueList = IMG_NULL;

	eError = SysInitialiseCommon(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed in SysInitialiseCommon"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

#if !defined(SGX_DYNAMIC_TIMING_INFO)
	/* Set up timing information*/
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ; 
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else	
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS; 
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ; 
#endif

	/*
		Setup the Source Clock Divider value
	*/
	gpsSysSpecificData->ui32SrcClockDiv = 3;

	/*
		Locate the devices within the system, specifying 
		the physical addresses of each devices components 
		(regs, mem, ports etc.)
	*/
	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to locate devices"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV);

	eError = SysPMRuntimeRegister();
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register with OSPM!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_PM_RUNTIME);

	eError = SysDvfsInitialize(gpsSysSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialize DVFS"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_DVFS_INIT);

	/*
		Register devices with the system
		This also sets up their memory maps/heaps
	*/
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_REGDEV);

	/*
		Once all devices are registered, specify the backing store
		and, if required, customise the memory heap config
	*/	
	psDeviceNode = gpsSysData->psDeviceNodeList;
	while(psDeviceNode)
	{
		/* perform any OEM SOC address space customisations here */
		switch(psDeviceNode->sDevId.eDeviceType)
		{
			case PVRSRV_DEVICE_TYPE_SGX:
			{
				DEVICE_MEMORY_INFO *psDevMemoryInfo;
				DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;

				/* 
					specify the backing store to use for the devices MMU PT/PDs
					- the PT/PDs are always UMA in this system
				*/
				psDeviceNode->psLocalDevMemArena = IMG_NULL;

				/* useful pointers */
				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

				/* specify the backing store for all SGX heaps */
				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
					psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
				}

				gpsSGXDevNode = psDeviceNode;
				gsSysSpecificData.psSGXDevNode = psDeviceNode;

				break;
			}
			default:
				PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to find SGX device node!"));
				return PVRSRV_ERROR_INIT_FAILURE;
		}

		/* advance to next device */
		psDeviceNode = psDeviceNode->psNext;
	}

	eError = EnableSystemClocksWrap(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to Enable system clocks (%d)", eError));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS);
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	eError = EnableSGXClocksWrap(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to Enable SGX clocks (%d)", eError));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
#endif	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */

	eError = PVRSRVInitialiseDevice(gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_INITDEV);

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	/* SGX defaults to D3 power state */
	DisableSGXClocks(gpsSysData);
#endif	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */

#if !defined(PVR_NO_OMAP_TIMER)
#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
	TimerRegPhysBase = gsSysSpecificData.sTimerRegPhysBase;
#else
	TimerRegPhysBase.uiAddr = SYS_OMAP_GP11TIMER_REGS_SYS_PHYS_BASE;
#endif
	gpsSysData->pvSOCTimerRegisterKM = IMG_NULL;
	gpsSysData->hSOCTimerRegisterOSMemHandle = 0;
	if (TimerRegPhysBase.uiAddr != 0)
	{
		OSReservePhys(TimerRegPhysBase,
				  4,
				  PVRSRV_HAP_MULTI_PROCESS|PVRSRV_HAP_UNCACHED,
				  IMG_NULL,
				  (IMG_VOID **)&gpsSysData->pvSOCTimerRegisterKM,
				  &gpsSysData->hSOCTimerRegisterOSMemHandle);
	}
#endif /* !defined(PVR_NO_OMAP_TIMER) */


	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysFinalise
 
 @Description Final part of initialisation at 'driver load' time
 
 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	eError = EnableSGXClocksWrap(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to Enable SGX clocks (%d)", eError));
		return eError;
	}
#endif	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to install MISR"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_MISR);

#if defined(SYS_USING_INTERRUPTS)
	/* install a Device ISR */
	eError = OSInstallDeviceLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ, "SGX ISR", gpsSGXDevNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to install ISR"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR);
#if !defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	SysEnableSGXInterrupts(gpsSysData);
#endif
#endif /* defined(SYS_USING_INTERRUPTS) */
#if defined(__linux__) || defined(__QNXNTO__)
	/* Create a human readable version string for this system */
	gpsSysData->pszVersionString = SysCreateVersionString();
	if (!gpsSysData->pszVersionString)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to create a system version string"));
	}
	else
	{
		PVR_TRACE(("SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}
#endif

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	/* SGX defaults to D3 power state */
	DisableSGXClocks(gpsSysData);
#endif	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */

	gpsSysSpecificData->bSGXInitComplete = IMG_TRUE;

	return eError;
}


/*!
******************************************************************************

 @Function	SysDeinitialise

 @Description	De-initialises kernel services at 'driver unload' time

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	if(gpsSysData->pvSOCTimerRegisterKM)
	{
		OSUnReservePhys(gpsSysData->pvSOCTimerRegisterKM,
						4,
						PVRSRV_HAP_MULTI_PROCESS|PVRSRV_HAP_UNCACHED,
						gpsSysData->hSOCTimerRegisterOSMemHandle);
	}


#if defined(SYS_USING_INTERRUPTS)
	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR))
	{
		eError = OSUninstallDeviceLISR(gpsSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallDeviceLISR failed"));
			return eError;
		}
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_MISR))
	{
		eError = OSUninstallMISR(gpsSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_INITDEV))
	{
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
		PVR_ASSERT(SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS));
		/* Reenable SGX clocks whilst SGX is being deinitialised. */
		eError = EnableSGXClocksWrap(gpsSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: EnableSGXClocks failed"));
			return eError;
		}
#endif	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */

		/* Deinitialise SGX */
		eError = PVRSRVDeinitialiseDevice(gui32SGXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}

	/* Disable system clocks. Must happen after last access to hardware */
	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS))
	{
		DisableSystemClocks(gpsSysData);
	}

	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_DVFS_INIT))
	{
		eError = SysDvfsDeinitialize(gpsSysSpecificData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: Failed to de-init DVFS"));
			gpsSysData = IMG_NULL;
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_PM_RUNTIME))
	{
		eError = SysPMRuntimeUnregister();
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: Failed to unregister with OSPM!"));
			gpsSysData = IMG_NULL;
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_ENVDATA))
	{	
		eError = OSDeInitEnvData(gpsSysData->pvEnvSpecificData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
			return eError;
		}
	}

	SysDeinitialiseCommon(gpsSysData);

#if defined(NO_HARDWARE) || defined(SGX_OCP_REGS_ENABLED)
	if(gsSGXRegsCPUVAddr != IMG_NULL)
	{
#if defined(NO_HARDWARE)
		/* Free hardware resources. */
		OSBaseFreeContigMemory(SYS_OMAP_SGX_REGS_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);
#else
#if defined(SGX_OCP_REGS_ENABLED)
		OSUnMapPhysToLin(gsSGXRegsCPUVAddr,
		gsSGXDeviceMap.ui32RegsSize,
												 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
												 IMG_NULL);

		gpvOCPRegsLinAddr = IMG_NULL;
#endif
#endif	/* defined(NO_HARDWARE) */
		gsSGXRegsCPUVAddr = IMG_NULL;
		gsSGXDeviceMap.pvRegsCpuVBase = gsSGXRegsCPUVAddr;
	}
#endif	/* defined(NO_HARDWARE) || defined(SGX_OCP_REGS_ENABLED) */

	
	gpsSysSpecificData->ui32SysSpecificData = 0;
	gpsSysSpecificData->bSGXInitComplete = IMG_FALSE;

	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		SysGetDeviceMemoryMap

 @Description	returns a device address map for the specified device

 @Input			eDeviceType - device type
 @Input			ppvDeviceMap - void ptr to receive device specific info.

 @Return		PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE	eDeviceType,
								   IMG_VOID				**ppvDeviceMap)
{

	switch(eDeviceType)
	{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			/* just return a pointer to the structure */
			*ppvDeviceMap = (IMG_VOID*)&gsSGXDeviceMap;

			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR,"SysGetDeviceMemoryMap: unsupported device type"));
		}
	}
	return PVRSRV_OK;
}


/*!
******************************************************************************
 @Function        SysCpuPAddrToDevPAddr

 @Description     Compute a device physical address from a cpu physical
                  address. Relevant when

 @Input           cpu_paddr - cpu physical address.
 @Input           eDeviceType - device type required if DevPAddr
                                address spaces vary across devices
                                in the same system
 @Return         device physical address.

******************************************************************************/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE	eDeviceType,
									  IMG_CPU_PHYADDR		CpuPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = CpuPAddr.uiAddr;
	
	return DevPAddr;
}

/*!
******************************************************************************
 @Function        SysSysPAddrToCpuPAddr

 @Description     Compute a cpu physical address from a system physical
                  address.

 @Input           sys_paddr - system physical address.
 @Return          cpu physical address.

******************************************************************************/
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to
	   sys address 0, ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

/*!
******************************************************************************
 @Function        SysCpuPAddrToSysPAddr

 @Description     Compute a system physical address from a cpu physical
	            address.

 @Input           cpu_paddr - cpu physical address.
 @Return          device physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to
	   sys address 0, ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}


/*!
******************************************************************************
 @Function        SysSysPAddrToDevPAddr

 @Description     Compute a device physical address from a system physical
	            address.

 @Input           SysPAddr - system physical address.
 @Input           eDeviceType - device type required if DevPAddr 
				address spaces vary across devices 
				in the same system

 @Return        Device physical address.

******************************************************************************/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = SysPAddr.uiAddr;

	return DevPAddr;
}


/*!
******************************************************************************
 @Function        SysDevPAddrToSysPAddr

 @Description     Compute a device physical address from a system physical
	            address.

 @Input           DevPAddr - device physical address.
 @Input           eDeviceType - device type required if DevPAddr 
		  address spaces vary across devices 
		  in the same system

 @Return        System physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr(PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
	IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for UMA system we assume DevP == SysP */
	SysPAddr.uiAddr = DevPAddr.uiAddr;

	return SysPAddr;
}


/*****************************************************************************
 @Function        SysRegisterExternalDevice

 @Description     Called when a 3rd party device registers with services

 @Input           psDeviceNode - the new device node.

 @Return        IMG_VOID
*****************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*****************************************************************************
 @Function        SysRemoveExternalDevice

 @Description     Called when a 3rd party device unregisters from services

 @Input           psDeviceNode - the device node being removed.

 @Return        IMG_VOID
*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}

/*!
******************************************************************************
 @Function        SysGetInterruptSource

 @Description     Returns System specific information about the device(s) that
				generated the interrupt in the system

 @Input           psSysData
 @Input           psDeviceNode

 @Return        System specific information indicating which device(s) 
				generated the interrupt

******************************************************************************/
IMG_UINT32 SysGetInterruptSource(SYS_DATA			*psSysData,
								 PVRSRV_DEVICE_NODE	*psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
#if defined(NO_HARDWARE)
	/* no interrupts in no_hw system just return all bits */
	return 0xFFFFFFFF;
#else
	/* Not a shared irq, so we know this is an interrupt for this device */
	return psDeviceNode->ui32SOCInterruptBit;
#endif
}


/*!
******************************************************************************
 @Function        SysClearInterrupts

 @Description     Clears specified system interrupts

 @Input           psSysData
 @Input           ui32ClearBits

 @Return        IMG_VOID

******************************************************************************/
IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);
	PVR_UNREFERENCED_PARAMETER(psSysData);
#if !defined(NO_HARDWARE)
#if defined(SGX_OCP_NO_INT_BYPASS)
	OSWriteHWReg(gpvOCPRegsLinAddr, EUR_CR_OCP_IRQSTATUS_2, 0x1);
#endif
	/* Flush posted writes */
	OSReadHWReg(((PVRSRV_SGXDEV_INFO *)gpsSGXDevNode->pvDevice)->pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR);
#endif	/* defined(NO_HARDWARE) */
}

#if defined(SGX_OCP_NO_INT_BYPASS)
/*!
******************************************************************************
 @Function        SysEnableSGXInterrupts

 @Description     Enables SGX interrupts

 @Input           psSysData

 @Return        IMG_VOID

******************************************************************************/
IMG_VOID SysEnableSGXInterrupts(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *)psSysData->pvSysSpecificData;
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_LISR) && !SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
	{
		OSWriteHWReg(gpvOCPRegsLinAddr, EUR_CR_OCP_IRQSTATUS_2, 0x1);
		OSWriteHWReg(gpvOCPRegsLinAddr, EUR_CR_OCP_IRQENABLE_SET_2, 0x1);
		SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
	}
}

/*!
******************************************************************************
 @Function        SysDisableSGXInterrupts

 @Description     Disables SGX interrupts

 @Input           psSysData

 @Return        IMG_VOID

******************************************************************************/
IMG_VOID SysDisableSGXInterrupts(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *)psSysData->pvSysSpecificData;

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
	{
		OSWriteHWReg(gpvOCPRegsLinAddr, EUR_CR_OCP_IRQENABLE_CLR_2, 0x1);
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
	}
}
#endif	/* defined(SGX_OCP_NO_INT_BYPASS) */

/*!
******************************************************************************

 @Function	SysSystemPrePowerState

 @Description	Perform system-level processing required before a power transition

 @Input		eNewPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (eNewPowerState == PVRSRV_SYS_POWER_STATE_D3)
	{
		PVR_TRACE(("SysSystemPrePowerState: Entering state D3"));

#if defined(SYS_USING_INTERRUPTS)
		if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR))
		{
#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
			IMG_BOOL bWrapped = WrapSystemPowerChange(&gsSysSpecificData);
#endif
			eError = OSUninstallDeviceLISR(gpsSysData);
#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
			if (bWrapped)
			{
				UnwrapSystemPowerChange(&gsSysSpecificData);
			}
#endif
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPrePowerState: OSUninstallDeviceLISR failed (%d)", eError));
				return eError;
			}
			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR);
		}
#endif

		if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS))
		{
			DisableSystemClocks(gpsSysData);

			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS);
		}
	}

	return eError;
}


/*!
******************************************************************************

 @Function	SysSystemPostPowerState

 @Description	Perform system-level processing required after a power transition

 @Input		eNewPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (eNewPowerState == PVRSRV_SYS_POWER_STATE_D0)
	{
		PVR_TRACE(("SysSystemPostPowerState: Entering state D0"));

		if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS))
		{
			eError = EnableSystemClocksWrap(gpsSysData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: EnableSystemClocksWrap failed (%d)", eError));
				return eError;
			}
			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS);
		}

#if defined(SYS_USING_INTERRUPTS)
		if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR))
		{
#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
			IMG_BOOL bWrapped = WrapSystemPowerChange(&gsSysSpecificData);
#endif

			eError = OSInstallDeviceLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ, "SGX ISR", gpsSGXDevNode);
#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
			if (bWrapped)
			{
				UnwrapSystemPowerChange(&gsSysSpecificData);
			}
#endif
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: OSInstallDeviceLISR failed to install ISR (%d)", eError));
				return eError;
			}
			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
		}
#endif
	}
	return eError;
}


/*!
******************************************************************************

 @Function	SysDevicePrePowerState

 @Description	Perform system level processing required before a device power
 				transition

 @Input		ui32DeviceIndex :
 @Input		eNewPowerState :
 @Input		eCurrentPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32				ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	if (ui32DeviceIndex != gui32SGXDeviceID)
	{
		return PVRSRV_OK;
	}

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SysDevicePrePowerState: SGX Entering state D3"));
		DisableSGXClocks(gpsSysData);
	}
#else	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	PVR_UNREFERENCED_PARAMETER(eNewPowerState );
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePostPowerState

 @Description	Perform system level processing required after a device power
 				transition

 @Input		ui32DeviceIndex :
 @Input		eNewPowerState :
 @Input		eCurrentPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32				ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(eNewPowerState);

	if (ui32DeviceIndex != gui32SGXDeviceID)
	{
		return eError;
	}

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	if (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SysDevicePostPowerState: SGX Leaving state D3"));
		eError = EnableSGXClocksWrap(gpsSysData);
	}
#else	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);
#endif	/* SUPPORT_ACTIVE_POWER_MANAGEMENT */

	return eError;
}

#if defined(SYS_SUPPORTS_SGX_IDLE_CALLBACK)

IMG_VOID SysSGXIdleTransition(IMG_BOOL bSGXIdle)
{
	PVR_DPF((PVR_DBG_MESSAGE, "SysSGXIdleTransition switch to %u", bSGXIdle));
}

#endif /* defined(SYS_SUPPORTS_SGX_IDLE_CALLBACK) */

/*****************************************************************************
 @Function        SysOEMFunction

 @Description     marshalling function for custom OEM functions

 @Input           ui32ID  - function ID
 @Input           pvIn - in data
 @Output          pvOut - out data

 @Return        PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32	ulInSize,
								IMG_VOID	*pvOut,
								IMG_UINT32	ulOutSize)
{
	PVR_UNREFERENCED_PARAMETER(ui32ID);
	PVR_UNREFERENCED_PARAMETER(pvIn);
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvOut);
	PVR_UNREFERENCED_PARAMETER(ulOutSize);

#if !defined(__QNXNTO__)
	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
		(ulOutSize == sizeof(PVRSRV_DC_OEM_JTABLE)))
	{
		PVRSRV_DC_OEM_JTABLE *psOEMJTable = (PVRSRV_DC_OEM_JTABLE*) pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
		return PVRSRV_OK;
	}
#endif

	return PVRSRV_ERROR_INVALID_PARAMS;
}
/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/

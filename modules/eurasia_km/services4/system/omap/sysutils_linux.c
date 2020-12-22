/*************************************************************************/ /*!
@Title          System dependent utilities
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides system-specific functions
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
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#if defined(SYS_OMAP_HAS_DVFS_FRAMEWORK)
#include <linux/opp.h>
#endif

#if defined(SUPPORT_DRI_DRM_PLUGIN)
#include <drm/drmP.h>
#include <drm/drm.h>

#include <linux/omap_gpu.h>

#include "pvr_drm.h"
#endif

#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

#if defined(SUPPORT_OMAP3430_SGXFCLK_96M)
#define SGX_PARENT_CLOCK "cm_96m_fck"
#else
#define SGX_PARENT_CLOCK "core_ck"
#endif

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
extern struct platform_device *gpsPVRLDMDev;
#endif

static PVRSRV_ERROR PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData, IMG_BOOL bTryLock)
{
	if (!in_interrupt())
	{
		if (bTryLock)
		{
			int locked = mutex_trylock(&psSysSpecData->sPowerLock);
			if (locked == 0)
			{
				return PVRSRV_ERROR_RETRY;
			}
		}
		else
		{
			mutex_lock(&psSysSpecData->sPowerLock);
		}
	}

	return PVRSRV_OK;
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
	{
		mutex_unlock(&psSysSpecData->sPowerLock);
	}
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	PowerLockUnwrap(psSysData->pvSysSpecificData);
}

/*
 * This function should be called to unwrap the Services power lock, prior
 * to calling any function that might sleep.
 * This function shouldn't be called prior to calling EnableSystemClocks
 * or DisableSystemClocks, as those functions perform their own power lock
 * unwrapping.
 * If the function returns IMG_TRUE, UnwrapSystemPowerChange must be
 * called to rewrap the power lock, prior to returning to Services.
 */
IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return IMG_TRUE;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
}

/*
 * Return SGX timining information to caller.
 */
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
#if !defined(NO_HARDWARE)
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);
#endif
#if defined(SYS_OMAP_HAS_DVFS_FRAMEWORK)
	psTimingInfo->ui32CoreClockSpeed =
		gpsSysSpecificData->pui32SGXFreqList[gpsSysSpecificData->ui32SGXFreqListIndex];
#else /* defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
#endif
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

/*!
******************************************************************************

 @Function  EnableSGXClocks

 @Description Enable SGX clocks

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already enabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
#if defined(SYS_OMAP_HAS_DVFS_FRAMEWORK)
	{
		struct gpu_platform_data *pdata;
		IMG_UINT32 max_freq_index;
		int res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;
		max_freq_index = psSysSpecData->ui32SGXFreqListSize - 2;

		/*
		 * Request maximum frequency from DVFS layer if not already set. DVFS may
		 * report busy if early in initialization, but all other errors are
		 * considered serious. Upon any error we proceed assuming our safe frequency
		 * value to be in use as indicated by the "unknown" index.
		 */
		if (psSysSpecData->ui32SGXFreqListIndex != max_freq_index)
		{
			PVR_ASSERT(pdata->device_scale != IMG_NULL);
			res = pdata->device_scale(&gpsPVRLDMDev->dev,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0))
									  &gpsPVRLDMDev->dev,
#endif
									  psSysSpecData->pui32SGXFreqList[max_freq_index]);
			if (res == 0)
			{
				psSysSpecData->ui32SGXFreqListIndex = max_freq_index;
			}
			else if (res == -EBUSY)
			{
				PVR_DPF((PVR_DBG_WARNING, "EnableSGXClocks: Unable to scale SGX frequency (EBUSY)"));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
			else if (res < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Unable to scale SGX frequency (%d)", res));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
		}
	}
#endif /* defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */
	{
		/*
		 * pm_runtime_get_sync returns 1 after the module has
		 * been reloaded.
		 */
		int res = pm_runtime_get_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: pm_runtime_get_sync failed (%d)", -res));
			return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
		}
	}
#endif /* defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI) */

	SysEnableSGXInterrupts(psSysData);

	/* Indicate that the SGX clocks are enabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function  DisableSGXClocks

 @Description Disable SGX clocks.

 @Return   none

******************************************************************************/
IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already disabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	{
		int res = pm_runtime_put_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: pm_runtime_put_sync failed (%d)", -res));
		}
	}
#if defined(SYS_OMAP_HAS_DVFS_FRAMEWORK)
	{
		struct gpu_platform_data *pdata;
		int res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;

		/*
		 * Request minimum frequency (list index 0) from DVFS layer if not already
		 * set. DVFS may report busy if early in initialization, but all other errors
		 * are considered serious. Upon any error we proceed assuming our safe frequency
		 * value to be in use as indicated by the "unknown" index.
		 */
		if (psSysSpecData->ui32SGXFreqListIndex != 0)
		{
			PVR_ASSERT(pdata->device_scale != IMG_NULL);
			res = pdata->device_scale(&gpsPVRLDMDev->dev,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0))
									  &gpsPVRLDMDev->dev,
#endif
									  psSysSpecData->pui32SGXFreqList[0]);
			if (res == 0)
			{
				psSysSpecData->ui32SGXFreqListIndex = 0;
			}
			else if (res == -EBUSY)
			{
				PVR_DPF((PVR_DBG_WARNING, "DisableSGXClocks: Unable to scale SGX frequency (EBUSY)"));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
			else if (res < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: Unable to scale SGX frequency (%d)", res));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
		}
	}
#endif /* defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */
#endif /* defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI) */

	/* Indicate that the SGX clocks are disabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
}

#if (defined(DEBUG) || defined(TIMING)) && !defined(PVR_NO_OMAP_TIMER)
#if defined(PVR_OMAP_USE_DM_TIMER_API)
#define	GPTIMER_TO_USE 11
/*!
******************************************************************************

 @Function  AcquireGPTimer

 @Description Acquire a GP timer

 @Return   PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_ASSERT(psSysSpecData->psGPTimer == NULL);

	/*
	 * This code has problems on module reload for OMAP5 running Linux
	 * 3.4.10, due to omap2_dm_timer_set_src (called by
	 * omap_dm_timer_request_specific), being unable to set the parent
	 * clock to OMAP_TIMER_SRC_32_KHZ.
	 * Not calling omap_dm_timer_set_source doesn't help.
	 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)) || !defined(MODULE)
	/*
	 * This code could try requesting registers 9, 10, and 11,
	 * stopping at the first succesful request.  We'll stick with
	 * 11 for now, as it avoids having to hard code yet more
	 * physical addresses into the code.
	 */
	psSysSpecData->psGPTimer = omap_dm_timer_request_specific(GPTIMER_TO_USE);
	if (psSysSpecData->psGPTimer == NULL)
	{
	
		PVR_DPF((PVR_DBG_WARNING, "%s: omap_dm_timer_request_specific failed", __FUNCTION__));
		return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
	}

	omap_dm_timer_set_source(psSysSpecData->psGPTimer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_enable(psSysSpecData->psGPTimer);

	/* Set autoreload, and start value of 0 */
	omap_dm_timer_set_load_start(psSysSpecData->psGPTimer, 1, 0);

	omap_dm_timer_start(psSysSpecData->psGPTimer);

	/*
	 * The DM timer API doesn't have a mechanism for obtaining the
	 * physical address of the counter register.
	 */
	psSysSpecData->sTimerRegPhysBase.uiAddr = SYS_OMAP_GP11TIMER_REGS_SYS_PHYS_BASE;
#else	/* (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0)) || !defined(MODULE) */
	(void)psSysSpecData;
#endif	/* (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0)) || !defined(MODULE) */

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function  ReleaseGPTimer

 @Description Release a GP timer

 @Return   PVRSRV_ERROR

******************************************************************************/
static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (psSysSpecData->psGPTimer != NULL)
	{
		/* Always returns 0 */	
		(void) omap_dm_timer_stop(psSysSpecData->psGPTimer);

		omap_dm_timer_disable(psSysSpecData->psGPTimer);

		omap_dm_timer_free(psSysSpecData->psGPTimer);

		psSysSpecData->sTimerRegPhysBase.uiAddr = 0;

		psSysSpecData->psGPTimer = NULL;
	}

}
#else	/* PVR_OMAP_USE_DM_TIMER_API */
/*!
******************************************************************************

 @Function  AcquireGPTimer

 @Description Acquire a GP timer

 @Return   PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
#if defined(PVR_OMAP4_TIMING_PRCM)
	struct clk *psCLK;
	IMG_INT res;
	struct clk *sys_ck;
	IMG_INT rate;
#endif
	PVRSRV_ERROR eError;

	IMG_CPU_PHYADDR sTimerRegPhysBase;
	IMG_HANDLE hTimerEnable;
	IMG_UINT32 *pui32TimerEnable;

	PVR_ASSERT(psSysSpecData->sTimerRegPhysBase.uiAddr == 0);

#if defined(PVR_OMAP4_TIMING_PRCM)
	/* assert our dependence on the GPTIMER11 module */
	psCLK = clk_get(NULL, "gpt11_fck");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 functional clock"));
		goto ExitError;
	}
	psSysSpecData->psGPT11_FCK = psCLK;

	psCLK = clk_get(NULL, "gpt11_ick");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 interface clock"));
		goto ExitError;
	}
	psSysSpecData->psGPT11_ICK = psCLK;

	sys_ck = clk_get(NULL, "sys_clkin_ck");
	if (IS_ERR(sys_ck))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get System clock"));
		goto ExitError;
	}

	if(clk_get_parent(psSysSpecData->psGPT11_FCK) != sys_ck)
	{
		PVR_TRACE(("Setting GPTIMER11 parent to System Clock"));
		res = clk_set_parent(psSysSpecData->psGPT11_FCK, sys_ck);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't set GPTIMER11 parent clock (%d)", res));
		goto ExitError;
		}
	}

	rate = clk_get_rate(psSysSpecData->psGPT11_FCK);
	PVR_TRACE(("GPTIMER11 clock is %dMHz", HZ_TO_MHZ(rate)));

	res = clk_enable(psSysSpecData->psGPT11_FCK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 functional clock (%d)", res));
		goto ExitError;
	}

	res = clk_enable(psSysSpecData->psGPT11_ICK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 interface clock (%d)", res));
		goto ExitDisableGPT11FCK;
	}
#endif	/* defined(PVR_OMAP4_TIMING_PRCM) */

	/* Set the timer to non-posted mode */
	sTimerRegPhysBase.uiAddr = SYS_OMAP_GP11TIMER_TSICR_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(sTimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	if(!(*pui32TimerEnable & 4))
	{
		PVR_TRACE(("Setting GPTIMER11 mode to posted (currently is non-posted)"));

		/* Set posted mode */
		*pui32TimerEnable |= 4;
	}

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

	/* Enable the timer */
	sTimerRegPhysBase.uiAddr = SYS_OMAP_GP11TIMER_ENABLE_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(sTimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	/* Enable and set autoreload on overflow */
	*pui32TimerEnable = 3;

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

	psSysSpecData->sTimerRegPhysBase = sTimerRegPhysBase;

	eError = PVRSRV_OK;

	goto Exit;

ExitDisableGPT11ICK:
#if defined(PVR_OMAP4_TIMING_PRCM)
	clk_disable(psSysSpecData->psGPT11_ICK);
ExitDisableGPT11FCK:
	clk_disable(psSysSpecData->psGPT11_FCK);
ExitError:
#endif	/* defined(PVR_OMAP4_TIMING_PRCM) */
	eError = PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
Exit:
	return eError;
}

/*!
******************************************************************************

 @Function  ReleaseGPTimer

 @Description Release a GP timer

 @Return   PVRSRV_ERROR

******************************************************************************/
static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_HANDLE hTimerDisable;
	IMG_UINT32 *pui32TimerDisable;

	if (psSysSpecData->sTimerRegPhysBase.uiAddr == 0)
	{
		return;
	}

	/* Disable the timer */
	pui32TimerDisable = OSMapPhysToLin(psSysSpecData->sTimerRegPhysBase,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				&hTimerDisable);

	if (pui32TimerDisable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "DisableSystemClocks: OSMapPhysToLin failed"));
	}
	else
	{
		*pui32TimerDisable = 0;

		OSUnMapPhysToLin(pui32TimerDisable,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				hTimerDisable);
	}

	psSysSpecData->sTimerRegPhysBase.uiAddr = 0;

#if defined(PVR_OMAP4_TIMING_PRCM)
	clk_disable(psSysSpecData->psGPT11_ICK);

	clk_disable(psSysSpecData->psGPT11_FCK);
#endif	/* defined(PVR_OMAP4_TIMING_PRCM) */
}
#endif	/* PVR_OMAP_USE_DM_TIMER_API */
#else	/* (DEBUG || TIMING) && !PVR_NO_OMAP_TIMER */
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);

	return PVRSRV_OK;
}
static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);
}
#endif /* (DEBUG || TIMING) && !PVR_NO_OMAP_TIMER */

/*!
******************************************************************************

 @Function  EnableSystemClocks

 @Description Setup up the clocks for the graphics device to work.

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));

	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}

	return AcquireGPTimer(psSysSpecData);
}

/*!
******************************************************************************

 @Function  DisableSystemClocks

 @Description Disable the graphics clocks.

 @Return  none

******************************************************************************/
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	/*
	 * Always disable the SGX clocks when the system clocks are disabled.
	 * This saves having to make an explicit call to DisableSGXClocks if
	 * active power management is enabled.
	 */
	DisableSGXClocks(psSysData);

	ReleaseGPTimer(psSysSpecData);
}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_enable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_disable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
#if !defined(SYS_OMAP_HAS_DVFS_FRAMEWORK)
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
#else /* !defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */
	IMG_UINT32 i, *freq_list;
	IMG_INT32 opp_count;
	unsigned long freq;
	struct opp *opp;

	/*
	 * We query and store the list of SGX frequencies just this once under the
	 * assumption that they are unchanging, e.g. no disabling of high frequency
	 * option for thermal management. This is currently valid for 4430 and 4460.
	 */
	rcu_read_lock();
	opp_count = opp_get_opp_count(&gpsPVRLDMDev->dev);
	if (opp_count < 1)
	{
		rcu_read_unlock();
		PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not retrieve opp count"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	/*
	 * Allocate the frequency list with a slot for each available frequency plus
	 * one additional slot to hold a designated frequency value to assume when in
	 * an unknown frequency state.
	 */
	freq_list = kmalloc((opp_count + 1) * sizeof(IMG_UINT32), GFP_ATOMIC);
	if (!freq_list)
	{
		rcu_read_unlock();
		PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not allocate frequency list"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/*
	 * Fill in frequency list from lowest to highest then finally the "unknown"
	 * frequency value. We use the highest available frequency as our assumed value
	 * when in an unknown state, because it is safer for APM and hardware recovery
	 * timers to be longer than intended rather than shorter.
	 */
	freq = 0;
	for (i = 0; i < opp_count; i++)
	{
		opp = opp_find_freq_ceil(&gpsPVRLDMDev->dev, &freq);
		if (IS_ERR_OR_NULL(opp))
		{
			rcu_read_unlock();
			PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not retrieve opp level %d", i));
			kfree(freq_list);
			return PVRSRV_ERROR_NOT_SUPPORTED;
		}
		freq_list[i] = (IMG_UINT32)freq;
		freq++;
	}
	rcu_read_unlock();
	freq_list[opp_count] = freq_list[opp_count - 1];

	psSysSpecificData->ui32SGXFreqListSize = opp_count + 1;
	psSysSpecificData->pui32SGXFreqList = freq_list;

	/* Start in unknown state - no frequency request to DVFS yet made */
	psSysSpecificData->ui32SGXFreqListIndex = opp_count;
#endif /* !defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
#if !defined(SYS_OMAP_HAS_DVFS_FRAMEWORK)
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
#else /* !defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */
	/*
	 * We assume this function is only called if SysDvfsInitialize() was
	 * completed successfully before.
	 *
	 * The DVFS interface does not allow us to actually unregister as a
	 * user of SGX, so we do the next best thing which is to lower our
	 * required frequency to the minimum if not already set. DVFS may
	 * report busy if early in initialization, but all other errors are
	 * considered serious.
	 */
	if (psSysSpecificData->ui32SGXFreqListIndex != 0)
	{
		struct gpu_platform_data *pdata;
		IMG_INT32 res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;

		PVR_ASSERT(pdata->device_scale != IMG_NULL);
		res = pdata->device_scale(&gpsPVRLDMDev->dev,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0))
								  &gpsPVRLDMDev->dev,
#endif
								  psSysSpecificData->pui32SGXFreqList[0]);
		if (res == -EBUSY)
		{
			PVR_DPF((PVR_DBG_WARNING, "SysDvfsDeinitialize: Unable to scale SGX frequency (EBUSY)"));
		}
		else if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "SysDvfsDeinitialize: Unable to scale SGX frequency (%d)", res));
		}

		psSysSpecificData->ui32SGXFreqListIndex = 0;
	}

	kfree(psSysSpecificData->pui32SGXFreqList);
	psSysSpecificData->pui32SGXFreqList = 0;
	psSysSpecificData->ui32SGXFreqListSize = 0;
#endif /* !defined(SYS_OMAP_HAS_DVFS_FRAMEWORK) */

	return PVRSRV_OK;
}

#if defined(SUPPORT_DRI_DRM_PLUGIN)
static struct omap_gpu_plugin sOMAPGPUPlugin;

#define	SYS_DRM_SET_PLUGIN_FIELD(d, s, f) (d)->f = (s)->f
int
SysDRMRegisterPlugin(PVRSRV_DRM_PLUGIN *psDRMPlugin)
{
	int iRes;

	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, name);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, open);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, load);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, unload);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, release);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, mmap);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, ioctls);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, num_ioctls);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, ioctl_start);

	iRes = omap_gpu_register_plugin(&sOMAPGPUPlugin);
	if (iRes != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: omap_gpu_register_plugin failed (%d)", __FUNCTION__, iRes));
	}

	return iRes;
}

void
SysDRMUnregisterPlugin(PVRSRV_DRM_PLUGIN *psDRMPlugin)
{
	int iRes = omap_gpu_unregister_plugin(&sOMAPGPUPlugin);
	if (iRes != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: omap_gpu_unregister_plugin failed (%d)", __FUNCTION__, iRes));
	}
}
#endif

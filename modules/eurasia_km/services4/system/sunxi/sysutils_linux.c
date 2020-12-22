/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk-private.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/sunxi-smc.h>
#ifdef CONFIG_CPU_BUDGET_THERMAL
#include <linux/cpu_budget_cooling.h>
#endif /* CONFIG_CPU_BUDGET_THERMAL */

#include "services_headers.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

static struct clk *gpu_pll_clk         = NULL;
static struct clk *gpu_core_clk        = NULL;
static struct clk *gpu_mem_clk         = NULL;
static struct clk *gpu_hyd_clk         = NULL;
static struct regulator *sgx_regulator = NULL;
static u32    dvfs_level               = 4;
static u32    normal_level             = 4;
static u32    dvfs_enable              = 1;
static u32    dvfs_max_level           = 8;
static struct mutex  dvfs_lock;
extern struct platform_device *gpsPVRLDMDev;

typedef struct
{
    u32 volt;
    u32 freq;
} volttable_t;

static volttable_t vf_table[9] =
{
    {700, 252},
    {740, 288},
    {800, 456},
    {840, 504},
    {900, 624},
    {940, 648},
    {1000,672},
    {1040,696},
    {1100,744},
};

static int SetGpuVol(u32 vf_level)
{
	if(regulator_set_voltage(sgx_regulator, vf_table[vf_level].volt*1000, vf_table[vf_level].volt*1000) != 0)
    {
		PVR_DPF((PVR_DBG_ERROR, "Failed to set gpu power voltage!"));
		return -1;
    }
	/* delay for gpu voltage stability */
	udelay(20);
	return 0;
}

static int SetGpuFreq(u32 vf_level)
{
	if(clk_set_rate(gpu_pll_clk, vf_table[vf_level].freq*1000*1000))
    {
		PVR_DPF((PVR_DBG_ERROR, "Failed to set gpu frequency!"));
		return -1;
    }
	/* delay for gpu pll stability */
	udelay(100);
	return 0;
}

static void SGXDvfsChange(u32 vf_level)
{
	PVRSRV_ERROR err;
	err = PVRSRVDevicePreClockSpeedChange(0, IMG_TRUE, NULL);
	if(err == PVRSRV_OK)
	{
		if(vf_level < dvfs_level)
		{
			/* if changing to a lower level, reduce frequency firstly and then reduce voltage */
			if(SetGpuFreq(vf_level))
			{
				goto post;
			}
			if(SetGpuVol(vf_level))
			{
				SetGpuFreq(dvfs_level);
				goto post;
			}
		}
		else if(vf_level > dvfs_level)
		{
			/* if changing to a higher level, reduce voltage firstly and then reduce frequency */
			if(SetGpuVol(vf_level))
			{
				goto post;
			}
			if(SetGpuFreq(vf_level))
			{
				SetGpuVol(dvfs_level);
				goto post;
			}
		}
		else
		{
			/* here means the level to be is the same as dvfs_level, just do noting */
			goto post;
		}
		
		dvfs_level = vf_level;
	post:
		PVRSRVDevicePostClockSpeedChange(0, IMG_TRUE, NULL);		
	}
}

#ifdef CONFIG_CPU_BUDGET_THERMAL
static int sgx_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
	
	if(dvfs_enable == 0)
	{
		goto out;
	}
	
	mutex_lock(&dvfs_lock);
	if(mode == BUDGET_GPU_THROTTLE && dvfs_level > 0)
    {
        SGXDvfsChange(dvfs_level-1);
    }
	else if(cmd && (*(int *)cmd) == 0 && dvfs_level != normal_level)
	{
        SGXDvfsChange(normal_level);
    }
	mutex_unlock(&dvfs_lock);
	
out:
    return retval;
}
static struct notifier_block sgx_throttle_notifier = {
.notifier_call = sgx_throttle_notifier_call,
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

static ssize_t android_dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 cnt = 0,bufercnt = 0,ret = 0;
    char s[2];
    ret = sprintf(buf + bufercnt, "  vf_level voltage frequency\n");
    while(cnt <= dvfs_max_level)
    {
        if(cnt == dvfs_level)
        {
           sprintf(s, "->");
        }
		else
		{
            sprintf(s, "  ");
        }
        bufercnt += ret;
        ret = sprintf(buf+bufercnt, "%s   %1d     %4dmV   %3dMHz\n", s, cnt, vf_table[cnt].volt, vf_table[cnt].freq);
        cnt++;
    }

    return bufercnt+ret;
}

static ssize_t android_dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{	
    int err;
    unsigned long vf_level;
	
	if(dvfs_enable == 0)
	{
		goto out;
	}
	
    err = strict_strtoul(buf, 10, &vf_level);
    if (err || vf_level > dvfs_max_level)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		return err;
	}
	mutex_lock(&dvfs_lock);
	SGXDvfsChange((u32)vf_level);
	mutex_unlock(&dvfs_lock);

out:	
	return count;
}

static ssize_t normal_dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 cnt = 0,bufercnt = 0,ret = 0;
    char s[2];
    ret = sprintf(buf + bufercnt, "  vf_level voltage frequency\n");
    while(cnt <= dvfs_max_level)
    {
        if(cnt == normal_level)
        {
           sprintf(s, "->");
        }
		else
		{
            sprintf(s, "  ");
        }
        bufercnt += ret;
        ret = sprintf(buf+bufercnt, "%s   %1d     %4dmV   %3dMHz\n", s, cnt, vf_table[cnt].volt, vf_table[cnt].freq);
        cnt++;
    }

	return bufercnt+ret;
}

static ssize_t normal_dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long vf_level;	
    err = strict_strtoul(buf, 10, &vf_level);
    if (err || vf_level > dvfs_max_level)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		return err;
	}
	normal_level = (u32)vf_level;
	
	mutex_lock(&dvfs_lock);
	SGXDvfsChange(normal_level);
	mutex_unlock(&dvfs_lock);
	
	return count;
}

static ssize_t enable_dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 ret = 0;
	ret = sprintf(buf, "%d\n", dvfs_enable);
	return ret;
}

static ssize_t enable_dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long tmp;
	err = strict_strtoul(buf, 10, &tmp);
	if (err)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		return err;
	}
	
	dvfs_enable = (tmp == 0 ? 0 : 1);
	
	return count;
}

static DEVICE_ATTR(android, S_IRUGO|S_IWUSR|S_IWGRP, android_dvfs_show, android_dvfs_store);

static DEVICE_ATTR(normal, S_IRUGO|S_IWUSR|S_IWGRP, normal_dvfs_show, normal_dvfs_store);

static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP, enable_dvfs_show, enable_dvfs_store);

static struct attribute *gpu_attributes[] =
{
	&dev_attr_android.attr,
    &dev_attr_normal.attr,
    &dev_attr_enable.attr,
    NULL
};

 struct attribute_group gpu_attribute_group = {
  .name = "dvfs",
  .attrs = gpu_attributes
};

static void ParseSysconfigFex(void)
{
    char vftbl_name[11] = {0};
    u32 cnt, i=0, numberoftable, tmp;
    script_item_u val;
    script_item_value_type_e type;
    numberoftable = sizeof(vf_table)/sizeof(volttable_t);
	
	type = script_get_item("gpu_dvfs_table", "G_dvfs_enable", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT == type)
    {
        dvfs_enable = (val.val == 0 ? 0 : 1);
    }	
	
    type = script_get_item("gpu_dvfs_table", "G_LV_count", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        goto out;
    }
    cnt = val.val;
    if (cnt <= 0 || cnt > numberoftable)
    {
        goto out;
    }
    for(i=0;i<cnt;i++)
    {
		/* get frequency config from sysconfig.fex */
        sprintf(vftbl_name, "G_LV%d_freq",i);
        type = script_get_item("gpu_dvfs_table", vftbl_name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
        {
			PVR_DPF((PVR_DBG_WARNING, "Faile to get %s from sysconfig!", vftbl_name));
            goto out;
        }
		
		if(val.val < 0 || val.val > vf_table[numberoftable-1].freq)
		{
			goto out;
		}
		
		tmp = val.val;
		
		/* get voltage config from sysconfig.fex */
        sprintf(vftbl_name, "G_LV%d_volt",i);
        type = script_get_item("gpu_dvfs_table", vftbl_name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
        {
			PVR_DPF((PVR_DBG_WARNING, "Faile to get %s from sysconfig!", vftbl_name));
            goto out;
        }
		
		if(val.val < 0 || val.val > vf_table[numberoftable-1].volt)
		{
			goto out;
		}
		
		/* change the value of the frequency of vf_table */
        vf_table[i].freq = tmp;
		
		/* change the value of the voltage of vf_table */
        vf_table[i].volt = val.val;
    }
	
out:
	if(i == 0)
	{
		dvfs_max_level = i;
	}
	else
	{
    	dvfs_max_level = i - 1;
	}
	
	if(dvfs_level > dvfs_max_level)
	{
		dvfs_level = dvfs_max_level;
	}
	
	type = script_get_item("gpu_dvfs_table", "G_normal_level", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        return;
    }
	if(val.val >= 0 || val.val <= dvfs_max_level)
	{
		normal_level = val.val;
		dvfs_level = normal_level;
	}
}

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
	psTimingInfo->ui32CoreClockSpeed = (IMG_UINT32)clk_get_rate(gpu_core_clk);
	
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
PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData, IMG_BOOL bNoDev)
{
	if(gpu_core_clk->enable_count == 0)
	{
		if (clk_prepare_enable(gpu_pll_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu pll clock!"));
		}
		if (clk_prepare_enable(gpu_core_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu core clock!"));
		}
		if (clk_prepare_enable(gpu_mem_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu mem clock!"));
		}
		if (clk_prepare_enable(gpu_hyd_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu hyd clock!"));
		}
	}
	
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
	if(gpu_core_clk->enable_count == 1)
	{
		clk_disable_unprepare(gpu_hyd_clk);
		clk_disable_unprepare(gpu_mem_clk);	
		clk_disable_unprepare(gpu_core_clk);
		clk_disable_unprepare(gpu_pll_clk);
	}
}

/*!
******************************************************************************

 @Function  EnableSystemClocks

 @Description Setup up the clocks for the graphics device to work.

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		sgx_regulator = regulator_get(NULL,"vdd-gpu");
		if (IS_ERR(sgx_regulator))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to get sgx regulator!"));
		}
        
		ParseSysconfigFex();
		
		/* Set up PLL and clock parents */	
		gpu_pll_clk = clk_get(NULL,PLL_GPU_CLK);
		if (!gpu_pll_clk || IS_ERR(gpu_pll_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu pll handle!"));
		}
		gpu_core_clk = clk_get(NULL, GPUCORE_CLK);
		if (!gpu_core_clk || IS_ERR(gpu_core_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu core clock handle!"));
		}
		gpu_mem_clk = clk_get(NULL, GPUMEM_CLK);
		if (!gpu_mem_clk || IS_ERR(gpu_mem_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu mem clock handle!"));
		}
		gpu_hyd_clk = clk_get(NULL, GPUHYD_CLK);
		if (!gpu_hyd_clk || IS_ERR(gpu_hyd_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu hyd clock handle!"));
		}
		
		if (clk_set_parent(gpu_core_clk, gpu_pll_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to set the parent of gpu core clock!"));
		}
		if (clk_set_parent(gpu_mem_clk, gpu_pll_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to set the parent of gpu mem clock!"));
		}
		if (clk_set_parent(gpu_hyd_clk, gpu_pll_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to set the parent of gpu hyd clock!"));
		}
		
		/* set the frequency of gpu pll */
		SetGpuFreq(dvfs_level);
	
		mutex_init(&psSysSpecData->sPowerLock);
		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
		
		mutex_init(&dvfs_lock);
		
	#ifdef CONFIG_CPU_BUDGET_THERMAL
		register_budget_cooling_notifier(&sgx_throttle_notifier);
	#endif /* CONFIG_CPU_BUDGET_THERMAL */

		sysfs_create_group(&gpsPVRLDMDev->dev.kobj, &gpu_attribute_group);
	}

	/* enable gpu power */
	if (regulator_enable(sgx_regulator))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu power!"));
	}
	
	/* delay for gpu power stability */
	mdelay(2);
	
	/* set gpu power off gating invalid */
	sunxi_smc_writel(0, SUNXI_R_PRCM_VBASE + 0x118);
	
	if(sunxi_periph_reset_deassert(gpu_hyd_clk))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to release gpu reset!"));
	}
	
	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function  DisableSystemClocks

 @Description Disable the graphics clocks.

 @Return  none

******************************************************************************/
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	if(sunxi_periph_reset_assert(gpu_hyd_clk))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to pull down gpu reset!"));
	}

	/* set gpu power off gating valid */
	sunxi_smc_writel(1, SUNXI_R_PRCM_VBASE + 0x118);
	
	/* disable gpu power */
	if (regulator_is_enabled(sgx_regulator))
	{
		if (regulator_disable(sgx_regulator))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to disable gpu power!"));
		}
	}
}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}


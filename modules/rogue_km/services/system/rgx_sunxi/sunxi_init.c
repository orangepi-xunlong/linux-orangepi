#if defined(SUPPORT_ION)
#include "ion_sys.h"
#endif /* defined(SUPPORT_ION) */

#include <linux/hardirq.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/io.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk/sunxi.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include "power.h"
#include "sunxi_init.h"

#ifdef CONFIG_CPU_BUDGET_THERMAL
#include <linux/cpu_budget_cooling.h>
static int Is_powernow = 0;
#endif /* CONFIG_CPU_BUDGET_THERMAL */

#define AXI_CLK_FREQ 320
#define GPU_CTRL "gpuctrl"
#define LEVEL_COUNT sizeof(vf_table)/(sizeof(vf_table[0][0])*2)

static struct clk *gpu_core_clk        = NULL;
static struct clk *gpu_mem_clk         = NULL;
static struct clk *gpu_axi_clk         = NULL;
static struct clk *gpu_pll_clk         = NULL;
static struct clk *gpu_ctrl_clk        = NULL;
static struct regulator *rgx_regulator = NULL;
static char *regulator_id			= NULL;
static IMG_UINT32 min_vf_level_val     = 4;
static IMG_UINT32 max_vf_level_val     = 6;

static IMG_UINT32 vf_table[7][2] =
{
	{ 700,  48},
	{ 800, 120},
	{ 800, 240},
	{ 900, 320},
	{ 900, 384},
	{1000, 480},
	{1100, 528},
};

long int GetConfigFreq(IMG_VOID)
{
    return vf_table[min_vf_level_val][1]*1000*1000;
}

IMG_UINT32 AwClockFreqGet(IMG_HANDLE hSysData)
{
	return (IMG_UINT32)clk_get_rate(gpu_core_clk);
}

static IMG_VOID AssertGpuResetSignal(IMG_VOID)
{
	if(sunxi_periph_reset_assert(gpu_core_clk))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to pull down gpu reset!"));
	}
	if(sunxi_periph_reset_assert(gpu_ctrl_clk))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to pull down gpu control reset!"));
	}
}

static IMG_VOID DeAssertGpuResetSignal(IMG_VOID)
{
	if(sunxi_periph_reset_deassert(gpu_ctrl_clk))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to release gpu control reset!"));
	}
	if(sunxi_periph_reset_deassert(gpu_core_clk))
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to release gpu reset!"));
	}
}

static IMG_VOID RgxEnableClock(IMG_VOID)
{
	if(gpu_core_clk->enable_count == 0)
	{	
		if(clk_prepare_enable(gpu_pll_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable pll9 clock!"));
		}
		if(clk_prepare_enable(gpu_core_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable core clock!"));
		}
		if(clk_prepare_enable(gpu_mem_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable mem clock!"));
		}
		if(clk_prepare_enable(gpu_axi_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable axi clock!"));
		}
		if(clk_prepare_enable(gpu_ctrl_clk))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable ctrl clock!"));
		}
	}
}

static IMG_VOID RgxDisableClock(IMG_VOID)
{				
	if(gpu_core_clk->enable_count == 1)
	{
		clk_disable_unprepare(gpu_ctrl_clk);		
		clk_disable_unprepare(gpu_axi_clk);
		clk_disable_unprepare(gpu_mem_clk);	
		clk_disable_unprepare(gpu_core_clk);
		clk_disable_unprepare(gpu_pll_clk);
	}
}

static IMG_VOID RgxEnablePower(IMG_VOID)
{
	if(!regulator_is_enabled(rgx_regulator))
	{
		regulator_enable(rgx_regulator); 		
	}
}

static IMG_VOID RgxDisablePower(IMG_VOID)
{
	if(regulator_is_enabled(rgx_regulator))
	{
		regulator_disable(rgx_regulator); 		
	}
}

static IMG_VOID SetGpuVol(int vf_level)
{
	if(regulator_set_voltage(rgx_regulator, vf_table[vf_level][0]*1000, vf_table[max_vf_level_val][0]*1000) != 0)
    {
		PVR_DPF((PVR_DBG_ERROR, "Failed to set gpu power voltage!"));
    }
	/* delay for gpu voltage stability */
	udelay(20);
}

static IMG_VOID SetClkVal(const char clk_name[], int freq)
{
	struct clk *clk = NULL;
	
	if(!strcmp(clk_name, "pll"))
	{
		clk = gpu_pll_clk;
	}
	else if(!strcmp(clk_name, "core"))
	{
		clk = gpu_core_clk;
	}
	else if(!strcmp(clk_name, "mem"))
	{
		clk = gpu_mem_clk;
	}
	else
	{
		clk = gpu_axi_clk;
	}
	
	if(clk_set_rate(clk, freq*1000*1000))
    {
		clk = NULL;
		return;
    }

	if(clk == gpu_pll_clk)
	{
		/* delay for gpu pll stability */
		udelay(100);
	}
	
	clk = NULL;
}

static IMG_VOID RgxDvfsChange(int vf_level, int up_flag)
{
	PVRSRV_ERROR err;
	err = PVRSRVDevicePreClockSpeedChange(0, IMG_TRUE, NULL);
	if(err == PVRSRV_OK)
	{
		if(up_flag == 1)
		{
			SetGpuVol(vf_level);
			SetClkVal("pll", vf_table[vf_level][1]);
		}
		else
		{
			SetClkVal("pll", vf_table[vf_level][1]);
			SetGpuVol(vf_level);
		}
		PVRSRVDevicePostClockSpeedChange(0, IMG_TRUE, NULL);
	}
}

static IMG_VOID ParseFexPara(IMG_VOID)
{
    script_item_u regulator_id_fex, min_vf_level, max_vf_level;
	if(SCIRPT_ITEM_VALUE_TYPE_STR == script_get_item("rgx_para", "regulator_id", &regulator_id_fex))
    {              
        regulator_id = regulator_id_fex.str;
    }
	
    if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("rgx_para", "min_vf_level", &min_vf_level))
    {              
        if(min_vf_level.val >= 0 && min_vf_level.val < LEVEL_COUNT)
		{
			min_vf_level_val = min_vf_level.val;
		}
    }
	else
	{
		goto err_out2;
	}
	
    if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("rgx_para", "max_vf_level", &max_vf_level))
    {              
		if(max_vf_level.val >= min_vf_level_val && max_vf_level.val < LEVEL_COUNT)
		{
			max_vf_level_val = max_vf_level.val;
		}
    }
	else
	{	
		goto err_out1;
	}
	
	return;

err_out1:
	min_vf_level_val = 4;
err_out2:
	regulator_id = "axp22_dcdc2";
	return;
}

PVRSRV_ERROR AwPrePowerState(PVRSRV_DEV_POWER_STATE eNewPowerState, PVRSRV_DEV_POWER_STATE eCurrentPowerState, IMG_BOOL bForced)
{
	if(eNewPowerState == PVRSRV_DEV_POWER_STATE_ON)
	{
		RgxEnableClock();
	}
	return PVRSRV_OK;
}

PVRSRV_ERROR AwPostPowerState(PVRSRV_DEV_POWER_STATE eNewPowerState, PVRSRV_DEV_POWER_STATE eCurrentPowerState, IMG_BOOL bForced)
{
	if(eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF)
	{
		RgxDisableClock();
	}	
	return PVRSRV_OK;
}

PVRSRV_ERROR AwSysPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	if(eNewPowerState == PVRSRV_SYS_POWER_STATE_ON)
	{
		RgxEnablePower();
	
		mdelay(2);
	
		/* set external isolation invalid */
		writel(0, SUNXI_R_PRCM_VBASE + GPU_PWROFF_GATING);
	
		DeAssertGpuResetSignal();
		
		RgxEnableClock();
		
		/* set delay for internal power stability */
		writel(0x100, SUNXI_GPU_CTRL_VBASE + 0x18);
	}
	
	return PVRSRV_OK;
}

PVRSRV_ERROR AwSysPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	if(eNewPowerState == PVRSRV_SYS_POWER_STATE_OFF)
	{
		RgxDisableClock();
		
		AssertGpuResetSignal();
	
		/* set external isolation valid */
		writel(1, SUNXI_R_PRCM_VBASE + GPU_PWROFF_GATING);
	
		RgxDisablePower();
	}
	
	return PVRSRV_OK;
}

#ifdef CONFIG_CPU_BUDGET_THERMAL
static int rgx_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
	if(mode == BUDGET_GPU_THROTTLE)
    {
		if(Is_powernow)
		{
			RgxDvfsChange(min_vf_level_val, 0);
			Is_powernow = 0;
		}
    }
    else
	{
        if(cmd && (*(int *)cmd) == 1 && !Is_powernow)
		{
			RgxDvfsChange(max_vf_level_val, 1);
            Is_powernow = 1;
        }
		else if(cmd && (*(int *)cmd) == 0 && Is_powernow)
		{
			RgxDvfsChange(min_vf_level_val, 0);
            Is_powernow = 0;
        }
    }
	
	return retval;
}

static struct notifier_block rgx_throttle_notifier = {
.notifier_call = rgx_throttle_notifier_call,
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

IMG_VOID RgxSunxiInit(IMG_VOID)
{	
	ParseFexPara();
	
	rgx_regulator = regulator_get(NULL, regulator_id);
	if (IS_ERR(rgx_regulator)) 
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to get rgx regulator!"));
        rgx_regulator = NULL;
		return;
	}
	
	gpu_core_clk = clk_get(NULL, GPUCORE_CLK);
	gpu_mem_clk  = clk_get(NULL, GPUMEM_CLK);
	gpu_axi_clk  = clk_get(NULL, GPUAXI_CLK);
	gpu_pll_clk  = clk_get(NULL, PLL9_CLK);
	gpu_ctrl_clk = clk_get(NULL, GPU_CTRL);
	
	SetGpuVol(min_vf_level_val);
		
	SetClkVal("pll", vf_table[min_vf_level_val][1]);
	SetClkVal("core", vf_table[min_vf_level_val][1]);
	SetClkVal("mem", vf_table[min_vf_level_val][1]);
	SetClkVal("axi", AXI_CLK_FREQ);
	
	(void) AwSysPrePowerState(PVRSRV_SYS_POWER_STATE_ON);
	
#ifdef CONFIG_CPU_BUDGET_THERMAL
	register_budget_cooling_notifier(&rgx_throttle_notifier);
#endif /* CONFIG_CPU_BUDGET_THERMAL */
}

#if defined(SUPPORT_ION)
struct ion_device *g_psIonDev;
extern struct ion_device *idev;

PVRSRV_ERROR IonInit(void *phPrivateData)
{
	g_psIonDev    = idev;
	return PVRSRV_OK;
}

struct ion_device *IonDevAcquire(IMG_VOID)
{
	return g_psIonDev;
}

IMG_VOID IonDevRelease(struct ion_device *psIonDev)
{
	/* Nothing to do, sanity check the pointer we're passed back */
	PVR_ASSERT(psIonDev == g_psIonDev);
}

IMG_UINT32 IonPhysHeapID(IMG_VOID)
{
	return 0;
}

IMG_VOID IonDeinit(IMG_VOID)
{
	g_psIonDev = NULL;
}
#endif /* defined(SUPPORT_ION) */

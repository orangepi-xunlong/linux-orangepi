#if defined(SUPPORT_ION)
#include "ion_sys.h"
#endif /* defined(SUPPORT_ION) */

#if defined(SUPPORT_PDVFS)
#include "rgxpdvfs.h"
#endif

#include <linux/clkdev.h>
#include <linux/hardirq.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "power.h"
#include "ky_init.h"
#include "pvrsrv_device.h"
#include "syscommon.h"
#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>
#include "rgxdevice.h"
#include "sysconfig.h"

static struct st_context *g_platform = NULL;

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
void stSetFrequency(IMG_HANDLE hSysData, IMG_UINT32 ui32Frequency)
{
	int ret = 0;
	unsigned int old_freq;

	if (NULL == g_platform)
		panic("oops");

	old_freq = clk_get_rate(g_platform->gpu_clk);
	mutex_lock(&g_platform->set_power_state);
	if (g_platform->bEnablePd)
		ret = clk_set_rate(g_platform->gpu_clk, ui32Frequency);
	mutex_unlock(&g_platform->set_power_state);
	PVR_DPF((PVR_DBG_VERBOSE, "gpu clock change frequency: %d",ui32Frequency));
	if (ret) {
		PVR_DPF((PVR_DBG_ERROR, "failed to set gpu clock rate: %d", ret));
		return;
	}
}

void stSetVoltage(IMG_HANDLE hSysData, IMG_UINT32 ui32Volt)
{
	if (NULL == g_platform)
		panic("oops");
	PVR_DPF((PVR_DBG_VERBOSE, "entry %s", __func__));
}
#endif

#if defined(CONFIG_DEVFREQ_THERMAL) && defined(SUPPORT_LINUX_DVFS)

#define FALLBACK_STATIC_TEMPERATURE 55000

static u32 dynamic_coefficient;
static u32 static_coefficient;
static s32 ts[4];
static struct thermal_zone_device *gpu_tz;

static unsigned long model_static_power(struct devfreq *df, unsigned long voltage)
{
	int temperature;
	unsigned long temp;
	unsigned long temp_squared, temp_cubed, temp_scaling_factor;
	const unsigned long voltage_cubed = (voltage * voltage * voltage) >> 10;

	if (gpu_tz) {
		int ret;

		ret = gpu_tz->ops->get_temp(gpu_tz, &temperature);
		if (ret) {
			pr_warn_ratelimited("Error reading temperature for gpu thermal zone: %d\n",
					ret);
			temperature = FALLBACK_STATIC_TEMPERATURE;
		}
	} else {
		temperature = FALLBACK_STATIC_TEMPERATURE;
	}

	/* Calculate the temperature scaling factor. To be applied to the
	 * voltage scaled power.
	 */
	temp = temperature / 1000;
	temp_squared = temp * temp;
	temp_cubed = temp_squared * temp;
	temp_scaling_factor =
			(ts[3] * temp_cubed)
			+ (ts[2] * temp_squared)
			+ (ts[1] * temp)
			+ ts[0];

	return (((static_coefficient * voltage_cubed) >> 20)
			* temp_scaling_factor)
				/ 1000000;
}

static unsigned long model_dynamic_power(struct devfreq *df, unsigned long freq,
		unsigned long voltage)
{
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 *
	 * This function calculates the dynamic power after this formula:
	 * Pdyn (mW) = c (mW/(MHz*mV*mV)) * v (mV) * v (mV) * f (MHz)
	 */
	const unsigned long v2 = (voltage * voltage) / 1000; /* m*(V*V) */
	const unsigned long f_mhz = freq / 1000000; /* MHz */

	return (dynamic_coefficient * v2 * f_mhz) / 1000000; /* mW */
}

struct devfreq_cooling_power ky_power_model_simple_ops = {
	.get_static_power = model_static_power,
	.get_dynamic_power = model_dynamic_power,
};

int ky_power_model_simple_init(struct device *dev)
{
	struct device_node *power_model_node;
	const char *tz_name;

	power_model_node = of_get_child_by_name(dev->of_node,
			"power_model");
	if (!power_model_node) {
		dev_err(dev, "could not find power_model node\n");
		return -ENODEV;
	}
	if (!of_device_is_compatible(power_model_node,
			"img,pvr-simple-power-model")) {
		dev_err(dev, "power_model incompatible with simple power model\n");
		return -ENODEV;
	}

	if (of_property_read_string(power_model_node, "thermal-zone",
			&tz_name)) {
		dev_err(dev, "ts in power_model not available\n");
		return -EINVAL;
	}

	gpu_tz = thermal_zone_get_zone_by_name(tz_name);
	if (IS_ERR(gpu_tz)) {
		pr_warn_ratelimited("Error getting gpu thermal zone (%ld), not yet ready?\n",
				PTR_ERR(gpu_tz));
		gpu_tz = NULL;

		return -EPROBE_DEFER;
	}

	if (of_property_read_u32(power_model_node, "dynamic-coefficient",
			&dynamic_coefficient)) {
		dev_err(dev, "dynamic-coefficient in power_model not available\n");
		return -EINVAL;
	}
	if (of_property_read_u32(power_model_node, "static-coefficient",
			&static_coefficient)) {
		dev_err(dev, "static-coefficient in power_model not available\n");
		return -EINVAL;
	}
	if (of_property_read_u32_array(power_model_node, "ts", (u32 *)ts, 4)) {
		dev_err(dev, "ts in power_model not available\n");
		return -EINVAL;
	}

	return 0;
}
#endif
static void RgxEnableClock(struct st_context *platform)
{
	if (!platform->gpu_clk)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: gpu_clk is null", __func__));
		return;
	}

	if (!platform->gpu_active) {
		//clk_prepare_enable(platform->gpu_clk);
		reset_control_deassert(platform->gpu_reset);
		platform->gpu_active = IMG_TRUE;
		PVR_DPF((PVR_DBG_VERBOSE, "gpu clock on gpu_active:%d", platform->gpu_active));
	} else {
		PVR_DPF((PVR_DBG_VERBOSE, "gpu clock already on!"));
	}
}

static void RgxDisableClock(struct st_context *platform)
{

	if (!platform->gpu_clk) {
		PVR_DPF((PVR_DBG_ERROR, "%s: gpu_clk is null", __func__));
		return;
	}

	if (platform->gpu_active) {
		reset_control_assert(platform->gpu_reset);
		//clk_disable_unprepare(platform->gpu_clk);
		platform->gpu_active = IMG_FALSE;
		PVR_DPF((PVR_DBG_VERBOSE, "gpu clock off gpu_active:%d", platform->gpu_active));
	} else {
		PVR_DPF((PVR_DBG_VERBOSE, "gpu clock already off!"));
	}
}

static void RgxEnablePower(struct st_context *platform)
{
	struct device *dev = (struct device *)platform->dev_config->pvOSDevice;
	if (!platform->bEnablePd) {
		pm_runtime_get_sync(dev);
		platform->bEnablePd = IMG_TRUE;
		PVR_DPF((PVR_DBG_VERBOSE, "gpu power on bEnablePd:%d", platform->bEnablePd));
	} else {
		PVR_DPF((PVR_DBG_VERBOSE, "gpu already power on!"));
	}
}

static void RgxDisablePower(struct st_context *platform)
{
	struct device *dev = (struct device *)platform->dev_config->pvOSDevice;
	if (platform->bEnablePd) {
		pm_runtime_put(dev);
		platform->bEnablePd = IMG_FALSE;
		PVR_DPF((PVR_DBG_VERBOSE, "gpu power off bEnablePd:%d", platform->bEnablePd));
	} else {
		PVR_DPF((PVR_DBG_VERBOSE, "gpu already power off!"));
	}
}

void RgxResume(struct st_context *platform)
{
	PVR_DPF((PVR_DBG_VERBOSE, "%s", __func__));
	RgxEnablePower(platform);
	RgxEnableClock(platform);
 }

void RgxSuspend(struct st_context *platform)
{
	PVR_DPF((PVR_DBG_VERBOSE, "%s", __func__));
	RgxDisableClock(platform);
	RgxDisablePower(platform);
}

PVRSRV_ERROR STPrePowerState(IMG_HANDLE hSysData,
							 PVRSRV_SYS_POWER_STATE eNewPowerState,
							 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
							 PVRSRV_POWER_FLAGS ePwrFlags)
{
	struct st_context *platform = (struct st_context *)hSysData;

	PVR_DPF((PVR_DBG_VERBOSE, "STPrePowerState new:%d, curr:%d", eNewPowerState, eCurrentPowerState));
	mutex_lock(&platform->set_power_state);
	if ((PVRSRV_SYS_POWER_STATE_ON == eNewPowerState) &&
		(PVRSRV_SYS_POWER_STATE_OFF == eCurrentPowerState))
		RgxResume(platform);
	mutex_unlock(&platform->set_power_state);
	return PVRSRV_OK;
}

PVRSRV_ERROR STPostPowerState(IMG_HANDLE hSysData,
							  PVRSRV_SYS_POWER_STATE eNewPowerState,
							  PVRSRV_SYS_POWER_STATE eCurrentPowerState,
							  PVRSRV_POWER_FLAGS ePwrFlags)
{
	struct st_context *platform = (struct st_context *)hSysData;

	PVR_DPF((PVR_DBG_VERBOSE, "STPostPowerState new:%d, curr:%d", eNewPowerState, eCurrentPowerState));
	mutex_lock(&platform->set_power_state);
	if ((PVRSRV_SYS_POWER_STATE_OFF == eNewPowerState) &&
		(PVRSRV_SYS_POWER_STATE_ON == eCurrentPowerState))
		RgxSuspend(platform);
	mutex_unlock(&platform->set_power_state);
	return PVRSRV_OK;
}

void RgxStUnInit(struct st_context *platform)
{
	struct device *dev = (struct device *)platform->dev_config->pvOSDevice;

	RgxSuspend(platform);

	if (platform->gpu_clk) {
		devm_clk_put(dev, platform->gpu_clk);
		platform->gpu_clk = NULL;
	}

	pm_runtime_disable(dev);
	devm_kfree(dev, platform);
}

struct st_context *RgxStInit(PVRSRV_DEVICE_CONFIG* psDevConfig)
{
	struct device *dev = (struct device *)psDevConfig->pvOSDevice;
	struct st_context *platform;
	RGX_DATA* psRGXData = (RGX_DATA*)psDevConfig->hDevData;

	platform = devm_kzalloc(dev, sizeof(struct st_context), GFP_KERNEL);
	if (NULL == platform) {
		PVR_DPF((PVR_DBG_ERROR, "RgxRkInit: Failed to kzalloc rk_context"));
		return NULL;
	}
	g_platform = platform;
	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;

	platform->dev_config = psDevConfig;
	platform->gpu_active = IMG_FALSE;
#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
	psDevConfig->sDVFS.sDVFSDeviceCfg.bIdleReq = IMG_TRUE;
	psDevConfig->sDVFS.sDVFSDeviceCfg.pfnSetFrequency = stSetFrequency;
	psDevConfig->sDVFS.sDVFSDeviceCfg.pfnSetVoltage = stSetVoltage;
	psDevConfig->sDVFS.sDVFSDeviceCfg.ui32PollMs = 300;
	psDevConfig->sDVFS.sDVFSGovernorCfg.ui32UpThreshold = 90;
	psDevConfig->sDVFS.sDVFSGovernorCfg.ui32DownDifferential = 10;
#if defined(CONFIG_DEVFREQ_THERMAL) && defined(SUPPORT_LINUX_DVFS)
	psDevConfig->sDVFS.sDVFSDeviceCfg.psPowerOps = &ky_power_model_simple_ops;
	if (ky_power_model_simple_init(dev)) {
		PVR_DPF((PVR_DBG_ERROR, "RgxStInit: ky_power_model_simple_init fail"));
		goto fail;
	}
#endif
#endif
	platform->bEnablePd = IMG_FALSE;
	pm_runtime_enable(dev);

	platform->gpu_reset = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR_OR_NULL(platform->gpu_reset)) {
		PVR_DPF((PVR_DBG_ERROR, "RgxStInit: Failed to find gpu reset source"));
		goto fail;
	}

	platform->gpu_clk = devm_clk_get(dev, "gpu_clk");
	if (IS_ERR_OR_NULL(platform->gpu_clk)) {
		PVR_DPF((PVR_DBG_ERROR, "RgxStInit: Failed to find gpu clk source"));
		goto fail;
	}

	reset_control_deassert(platform->gpu_reset);

	clk_prepare_enable(platform->gpu_clk);
	clk_set_rate(platform->gpu_clk, RGX_ST_CORE_CLOCK_SPEED);

	if (psRGXData && psRGXData->psRGXTimingInfo)
	{
		psRGXData->psRGXTimingInfo->ui32CoreClockSpeed = clk_get_rate(platform->gpu_clk);
	}

	reset_control_assert(platform->gpu_reset);

	mutex_init(&platform->set_power_state);

	return platform;

fail:
	devm_kfree(dev, platform);
	return NULL;
}

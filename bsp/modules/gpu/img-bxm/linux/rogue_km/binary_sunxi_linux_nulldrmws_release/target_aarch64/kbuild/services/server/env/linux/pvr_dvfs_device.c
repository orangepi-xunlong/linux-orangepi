/*************************************************************************/ /*!
@File           pvr_dvfs_device.c
@Title          PowerVR devfreq device implementation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Linux DVFS module setup
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

#if !defined(NO_HARDWARE)

#include <linux/devfreq.h>
#if defined(CONFIG_DEVFREQ_THERMAL)
#include <linux/devfreq_cooling.h>
#endif
#include <linux/version.h>
#include <linux/device.h>
#include <drm/drm.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#else
#include <drm/drmP.h>
#endif

#include "power.h"
#include "pvrsrv.h"
#include "pvrsrv_device.h"

#include "rgxdevice.h"
#include "rgxinit.h"
#include "sofunc_rgx.h"

#include "syscommon.h"

#include "pvr_dvfs.h"
#include "pvr_dvfs_device.h"
#include "pvr_dvfs_common.h"
#if defined(SUPPORT_PVR_DVFS_GOVERNOR)

#if !defined(CONFIG_PM_DEVFREQ)
#error "PVR DVFS governor requires kernel support for devfreq (CONFIG_PM_DEVFREQ = 1)"
#endif
#include "linux/governor.h"
#include "pvr_dvfs_governor.h"

#if defined(CONFIG_PM_DEVFREQ_EVENT)
#include "linux/devfreq-event.h"
#include "pvr_dvfs_events.h"
#endif
#endif /* SUPPORT_PVR_DVFS_GOVERNOR */

#include "kernel_compatibility.h"


/* Default constants for PVR-Balanced (PVR) governor */
#define PVR_UPTHRESHOLD			(90)
#define PVR_DOWNDIFFERENTIAL	(5)

static int _device_get_devid(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	int deviceId;

	if (ddev->render)
		deviceId = ddev->render->index;
	else /* when render node is NULL, fallback to primary node */
		deviceId = ddev->primary->index;

	return deviceId;
}

static IMG_INT32 devfreq_target(struct device *dev, unsigned long *requested_freq, IMG_UINT32 flags)
{
	int deviceId = _device_get_devid(dev);
	PVRSRV_DEVICE_NODE *psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(deviceId);
	RGX_DATA		*psRGXData = NULL;
	IMG_DVFS_DEVICE		*psDVFSDevice = NULL;
	IMG_DVFS_DEVICE_CFG	*psDVFSDeviceCfg = NULL;
	RGX_TIMING_INFORMATION	*psRGXTimingInfo = NULL;
	IMG_UINT32		ui32Freq, ui32CurFreq, ui32Volt;
	struct dev_pm_opp *opp;

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return -ENODEV;
	}

	psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;
	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;

	/* Check the RGX device is initialised */
	if (!psRGXData)
	{
		return -ENODATA;
	}

	psRGXTimingInfo = psRGXData->psRGXTimingInfo;
	if (psDVFSDevice->eState != PVR_DVFS_STATE_READY)
	{
		*requested_freq = psRGXTimingInfo->ui32CoreClockSpeed;
		return 0;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	rcu_read_lock();
#endif

	opp = devfreq_recommended_opp(dev, requested_freq, flags);
	if (IS_ERR(opp)) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
		rcu_read_unlock();
#endif
		PVR_DPF((PVR_DBG_ERROR, "Invalid OPP"));
		return PTR_ERR(opp);
	}

	ui32Freq = dev_pm_opp_get_freq(opp);
	ui32Volt = dev_pm_opp_get_voltage(opp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	rcu_read_unlock();
#else
	dev_pm_opp_put(opp);
#endif

	ui32CurFreq = psRGXTimingInfo->ui32CoreClockSpeed;

	if (ui32CurFreq == ui32Freq)
	{
		return 0;
	}

	if (PVRSRV_OK != PVRSRVDevicePreClockSpeedChange(psDeviceNode,
													 psDVFSDeviceCfg->bIdleReq,
													 NULL))
	{
		dev_err(dev, "PVRSRVDevicePreClockSpeedChange failed\n");
		return -EPERM;
	}

	/* Increasing frequency, change voltage first */
	if (ui32Freq > ui32CurFreq)
	{
		psDVFSDeviceCfg->pfnSetVoltage(psDeviceNode->psDevConfig->hSysData, ui32Volt);
	}

	psDVFSDeviceCfg->pfnSetFrequency(psDeviceNode->psDevConfig->hSysData, ui32Freq);

	/* Decreasing frequency, change frequency first */
	if (ui32Freq < ui32CurFreq)
	{
		psDVFSDeviceCfg->pfnSetVoltage(psDeviceNode->psDevConfig->hSysData, ui32Volt);
	}

	psRGXTimingInfo->ui32CoreClockSpeed = ui32Freq;

	PVRSRVDevicePostClockSpeedChange(psDeviceNode, psDVFSDeviceCfg->bIdleReq,
									 NULL);

	return 0;
}

static int devfreq_get_dev_status(struct device *dev, struct devfreq_dev_status *stat)
{
	int                      deviceId = _device_get_devid(dev);
	PVRSRV_DEVICE_NODE      *psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(deviceId);
	PVRSRV_RGXDEV_INFO      *psDevInfo = NULL;
	IMG_DVFS_DEVICE         *psDVFSDevice = NULL;
	RGX_DATA                *psRGXData = NULL;
	RGX_TIMING_INFORMATION  *psRGXTimingInfo = NULL;
	RGXFWIF_GPU_UTIL_STATS  *psGpuUtilStats = NULL;
	PVRSRV_ERROR             eError;
#if defined(CONFIG_PM_DEVFREQ_EVENT) && defined(SUPPORT_PVR_DVFS_GOVERNOR)
	struct pvr_profiling_dev_status *pvr_stat = stat->private_data;
	int err;
#endif

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return -ENODEV;
	}

	psDevInfo = psDeviceNode->pvDevice;
	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;

	/* Check the RGX device is initialised */
	if (!psDevInfo || !psRGXData)
	{
		return -ENODATA;
	}

	psRGXTimingInfo = psRGXData->psRGXTimingInfo;
	stat->current_frequency = psRGXTimingInfo->ui32CoreClockSpeed;

	if (psDevInfo->pfnGetGpuUtilStats == NULL)
	{
		/* Not yet ready. So set times to something sensible. */
		stat->busy_time = 0;
		stat->total_time = 0;
		return 0;
	}

	psGpuUtilStats = kzalloc(sizeof(*psGpuUtilStats), GFP_KERNEL);

	if (!psGpuUtilStats)
	{
		return -ENOMEM;
	}

	eError = psDevInfo->pfnGetGpuUtilStats(psDeviceNode,
						psDVFSDevice->hGpuUtilUserDVFS,
						psGpuUtilStats);

	if (eError != PVRSRV_OK)
	{
		kfree(psGpuUtilStats);
		return -EAGAIN;
	}

	stat->busy_time = psGpuUtilStats->ui64GpuStatActive;
	stat->total_time = psGpuUtilStats->ui64GpuStatCumulative;

	kfree(psGpuUtilStats);

#if defined(CONFIG_PM_DEVFREQ_EVENT) && defined(SUPPORT_PVR_DVFS_GOVERNOR)
	err = pvr_get_dev_status_get_events(psDVFSDevice->psProfilingDevice, pvr_stat);
	if (err)
		return err;
#endif

	return 0;
}

static IMG_INT32 devfreq_cur_freq(struct device *dev, unsigned long *freq)
{
	int deviceId = _device_get_devid(dev);
	PVRSRV_DEVICE_NODE *psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(deviceId);
	RGX_DATA *psRGXData = NULL;

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return -ENODEV;
	}

	psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;

	/* Check the RGX device is initialised */
	if (!psRGXData)
	{
		return -ENODATA;
	}

	*freq = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;

	return 0;
}

static struct devfreq_dev_profile img_devfreq_dev_profile =
{
	.polling_ms	        = 10,
	.target             = devfreq_target,
	.get_dev_status     = devfreq_get_dev_status,
	.get_cur_freq       = devfreq_cur_freq,
};

#if defined(SUPPORT_PVR_DVFS_GOVERNOR)

/* DEVFREQ governor name */
#define DEVFREQ_GOV_PVR_BALANCED	"pvr_balanced"

static unsigned long _get_max_freq(struct devfreq *devfreq_dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	return devfreq_dev->scaling_max_freq;
#else
	return devfreq_dev->max_freq;
#endif
}

/*
 * Calculate the utilisation percentage of X cycles as a proportion
 * of Y cycles
 */
static inline
unsigned long UTILISATION_PC(unsigned long X, unsigned long Y)
{
	PVR_ASSERT(Y > 0);

/*
	if (X < UINT_MAX / 100UL)
		return min(100UL, X * 100UL / Y);
	else
		return min(100UL, X / (Y / 100UL));
*/
	/* Prevent overflow */
	if (X < UINT32_MAX / 100U)
		return (X * 100U / Y);
	else
		return (X / (Y / 100U));
}

static int pvr_governor_get_target(struct devfreq *devfreq_dev,
								   unsigned long *freq)
{
	struct devfreq_dev_status *stat;
	struct pvr_profiling_dev_status *pvr_stat;
	IMG_DVFS_GOVERNOR_CFG *data = devfreq_dev->data;
	unsigned long long a, b;
	unsigned int pvr_upthreshold = PVR_UPTHRESHOLD;
	unsigned int pvr_downdifferential = PVR_DOWNDIFFERENTIAL;
	unsigned long ui32Util, ui32UtilDM, ui32UtilBus;
	int iGeomEventID, iFragEventID, iCompEventID, iSLCReadEventID, iSLCWriteEventID;
	int iSLCReadEventID2, iSLCWriteEventID2;
	int err;

	if (!devfreq_dev)
	{
		return -ENODEV;
	}

	/*
	 *	Governor main function for setting target frequency, based on
	 *	metrics for
		- utilisation
		- memory bus bandwidth
		- ALU counters etc.
	 */

	err = devfreq_update_stats(devfreq_dev);
	if (err)
	{
		dev_warn(&devfreq_dev->dev, "get_dev_status: error (%d)", err);
		return err;
	}

	stat = &devfreq_dev->last_status;
	pvr_stat = stat->private_data;
	if (!pvr_stat)
	{
		return -EINVAL;
	}

	iGeomEventID = pvr_find_event_by_name("geom-cycle");
	iFragEventID = pvr_find_event_by_name("3d-cycle");
	iCompEventID = pvr_find_event_by_name("comp-cycle");
	if (iGeomEventID < 0 || iFragEventID < 0 || iCompEventID < 0)
	{
		return -EINVAL;
	}

	if (pvr_stat->event_data[iGeomEventID].total_count == 0 ||
	    pvr_stat->event_data[iFragEventID].total_count == 0 ||
	    pvr_stat->event_data[iCompEventID].total_count == 0)
	{
		/* no error to avoid verbose kernel logs before the firmware/GPU
		 * is available. */
		return 0;
	}

	/* GEOM utilisation */
	if (iGeomEventID >= 0)
	{
		ui32Util =
			UTILISATION_PC(pvr_stat->event_data[iGeomEventID].load_count,
						   pvr_stat->event_data[iGeomEventID].total_count);
		ui32UtilDM = max(ui32UtilDM, ui32Util);
	}

	/* 3D utilisation */
	if (iFragEventID >= 0)
	{
		ui32Util =
			UTILISATION_PC(pvr_stat->event_data[iFragEventID].load_count,
						   pvr_stat->event_data[iFragEventID].total_count);
		ui32UtilDM = max(ui32UtilDM, ui32Util);
	}

	/* Compute utilisation */
	if (iCompEventID >= 0)
	{
		ui32Util =
			UTILISATION_PC(pvr_stat->event_data[iCompEventID].load_count,
						   pvr_stat->event_data[iCompEventID].total_count);
		ui32UtilDM = max(ui32UtilDM, ui32Util);
	}

	if (ui32UtilDM > 1000U)
	{
		/* Illegal value, keep current frequency */
		*freq = stat->current_frequency;
		return 0;
	}

	iSLCReadEventID =   pvr_find_event_by_name("slc-read");
	iSLCWriteEventID =  pvr_find_event_by_name("slc-write");
	iSLCReadEventID2 =  pvr_find_event_by_name("slc-read-2");
	iSLCWriteEventID2 = pvr_find_event_by_name("slc-write-2");
	if (iSLCReadEventID2 >= 0 && iSLCWriteEventID2 >= 0)
	{
		/* Use XE-series memory bus counters */

		ui32UtilBus = 0;

		while (iSLCWriteEventID < ARRAY_SIZE(g_pvr_governor_events))
		{
			ui32Util =
				UTILISATION_PC(pvr_stat->event_data[iSLCReadEventID].load_count,
							   pvr_stat->event_data[iSLCReadEventID].total_count);
			ui32UtilBus = ui32UtilBus + ui32Util;

			ui32Util =
				UTILISATION_PC(pvr_stat->event_data[iSLCWriteEventID].load_count,
							   pvr_stat->event_data[iSLCWriteEventID].total_count);
			ui32UtilBus = ui32UtilBus + ui32Util;
			iSLCReadEventID += 2;
			iSLCWriteEventID += 2;
		}
	}
	else if (iSLCReadEventID >= 0 && iSLCWriteEventID >= 0)
	{
		/* Use DXT-series memory bus counters */

		ui32Util =
			UTILISATION_PC(pvr_stat->event_data[iSLCReadEventID].load_count / data->uiNumMembus,
						   pvr_stat->event_data[iSLCReadEventID].total_count);
		ui32UtilBus = ui32Util;

		ui32Util =
			UTILISATION_PC(pvr_stat->event_data[iSLCWriteEventID].load_count / data->uiNumMembus,
						   pvr_stat->event_data[iSLCWriteEventID].total_count);
		ui32UtilBus = ui32UtilBus + ui32Util;
	}

	if (ui32UtilBus > 1000U)
	{
		/* Illegal value, use current frequency */
		*freq = stat->current_frequency;
		return 0;
	}

#if defined(DEBUG)
	iSLCReadEventID =  pvr_find_event_by_name("slc-read");
	iSLCWriteEventID = pvr_find_event_by_name("slc-write");
	dev_info(&devfreq_dev->dev, "Geom: %10lu, 3D: %10lu, Comp: %10lu, SLC: %10lu cycles. DM %lu pc, Bus %lu pc.",
			pvr_stat->event_data[iGeomEventID].load_count,
			pvr_stat->event_data[iFragEventID].load_count,
			pvr_stat->event_data[iCompEventID].load_count,
			pvr_stat->event_data[iSLCReadEventID].load_count + pvr_stat->event_data[iSLCWriteEventID].load_count,
			ui32UtilDM,
			ui32UtilBus);
#endif

	if (ui32UtilDM > 2 * ui32UtilBus) {
		// DM pipeline limited
		ui32Util = ui32UtilDM;
	}
	else if (ui32UtilBus > 2 * ui32UtilDM) {
		// Mem bandwidth limited
		ui32Util = ui32UtilBus;
	}
	else {
		// Average utilisation to smooth random fluctuations
		ui32Util = (ui32UtilDM + ui32UtilBus) / 2;
	}

	/* Set MAX if it's busy enough */
	if (ui32Util > pvr_upthreshold)
	{
		*freq = _get_max_freq(devfreq_dev);
		return 0;
	}

	/* Set MAX if we do not know the initial frequency */
	if (stat->current_frequency == 0) {
		*freq = _get_max_freq(devfreq_dev);
		return 0;
	}

	a = (unsigned long long) ui32Util * stat->current_frequency;
	b = div_u64(a, (pvr_upthreshold - pvr_downdifferential / 2));
	*freq = (unsigned long) b;
	return 0;
}

static void pvr_governor_suspend(struct devfreq *devfreq_dev)
{
	/* Nothing to do */
}

static void pvr_governor_resume(struct devfreq *devfreq_dev)
{
	int deviceId;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DVFS_DEVICE		*psDVFSDevice = NULL;
	struct pvr_profiling_device *psProfilingDevice;

	if (!devfreq_dev || !dev_get_drvdata(&devfreq_dev->dev))
		return;

	deviceId = _device_get_devid(&devfreq_dev->dev);
	psDeviceNode = PVRSRVGetDeviceInstanceByKernelDevID(deviceId);

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return;
	}

	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psProfilingDevice = psDVFSDevice->psProfilingDevice;

	/* Reset the profiling events */
	pvr_governor_reset_events(psProfilingDevice);
}

static int pvr_governor_event_handler(struct devfreq *devfreq_dev,
									  unsigned int event, void *data)
{
	if (!devfreq_dev)
	{
		pr_err("%s: devfreq_dev not ready.\n", __func__);
		return -ENODEV;
	}

	/*
	 * We cannot take the deviceId here, as the DRM device
	 * may not be initialised. Null pointer in
	 * struct drm_device *ddev = dev_get_drvdata(dev)
	 */

	switch (event) {
	case DEVFREQ_GOV_START:
		dev_info(&devfreq_dev->dev,"GOV_START event.\n");
		devfreq_monitor_start(devfreq_dev);
		break;

	case DEVFREQ_GOV_STOP:
		dev_info(&devfreq_dev->dev,"GOV_STOP event.\n");
		devfreq_monitor_stop(devfreq_dev);
		break;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
	case DEVFREQ_GOV_UPDATE_INTERVAL:
		dev_info(&devfreq_dev->dev,"GOV_UPDATE_INTERVAL event.\n");
		devfreq_update_interval(devfreq_dev, (unsigned int *)data);
		break;

#else
	case DEVFREQ_GOV_INTERVAL:
		dev_info(&devfreq_dev->dev,"GOV_INTERVAL event.\n");
		devfreq_interval_update(devfreq_dev, (unsigned int *)data);
		break;
#endif

	case DEVFREQ_GOV_SUSPEND:
		dev_info(&devfreq_dev->dev,"GOV_SUSPEND event.\n");
		pvr_governor_suspend(devfreq_dev);
		devfreq_monitor_suspend(devfreq_dev);
		break;

	case DEVFREQ_GOV_RESUME:
		dev_info(&devfreq_dev->dev,"GOV_RESUME event.\n");
		devfreq_monitor_resume(devfreq_dev);
		pvr_governor_resume(devfreq_dev);
		break;

	default:
		printk("Unknown event.\n");
		break;
	}

	return 0;
}

static struct devfreq_governor pvr_balanced_governor = {
	.name = DEVFREQ_GOV_PVR_BALANCED,
	.get_target_freq = pvr_governor_get_target,
	.event_handler   = pvr_governor_event_handler,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0))
	.attrs = DEVFREQ_GOV_ATTR_POLLING_INTERVAL
		| DEVFREQ_GOV_ATTR_TIMER,
#else
	.immutable = true,
#endif
};

int pvr_governor_init(void)
{
	int ret;

	ret = devfreq_add_governor(&pvr_balanced_governor);
	if (ret)
	{
		pr_err("%s: failed to install governor %d\n", __func__, ret);
	}

	return ret;
}

void pvr_governor_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&pvr_balanced_governor);
	if (ret)
	{
		pr_err("Failed to remove governor (%u)\n", ret);
	}
}

#if defined(CONFIG_PM_DEVFREQ_EVENT)

static int pvr_get_dev_status_get_events(struct pvr_profiling_device *pvr_prof_dev,
										 struct pvr_profiling_dev_status *pvr_stat)
{
	struct devfreq_event_data event_data;
	int i, ret = 0;

	for (i = 0; i < pvr_prof_dev->num_events; i++)
	{
		if (!pvr_prof_dev->edev[i])
			continue;

		ret = devfreq_event_get_event(pvr_prof_dev->edev[i], &event_data);
		if (ret < 0)
			return ret;

		pvr_stat->event_data[i].load_count = event_data.load_count;
		pvr_stat->event_data[i].total_count = event_data.total_count;
	}

	return ret;
}

static int pvr_governor_reset_events(struct pvr_profiling_device *pvr_prof_dev)
{
	int i, ret = 0;

	for (i = 0; i < pvr_prof_dev->num_events; i++)
	{
		if (!pvr_prof_dev->edev[i])
			continue;

		ret = devfreq_event_set_event(pvr_prof_dev->edev[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int pvr_events_enable(struct devfreq_event_dev *edev)
{
	/* Enable counters */

	dev_dbg(&edev->dev, "%s (Enable)\n", edev->desc->name);

	return 0;
}

static int pvr_events_disable(struct devfreq_event_dev *edev)
{
	/* Disable counters */

	dev_dbg(&edev->dev, "%s (Disable)\n", edev->desc->name);

	return 0;
}

static int pvr_find_event_by_name(const char *name)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(g_pvr_governor_events); idx++)
		if (strcmp(name, g_pvr_governor_events[idx].name) == 0)
			return idx;

	return -EINVAL;
}

static int pvr_governor_set_event(struct devfreq_event_dev *edev)
{
	int id = pvr_find_event_by_name(edev->desc->name);
	if (id < 0)
		return -EINVAL;

	/* clear the last value */
	g_pvr_governor_events[id].last_value = 0;

	return 0;
}

static int pvr_governor_get_event(struct devfreq_event_dev *edev,
				struct devfreq_event_data *edata)
{
	struct pvr_profiling_device *info = devfreq_event_get_drvdata(edev);
	volatile unsigned long perf_ctr;
	int id = pvr_find_event_by_name(edev->desc->name);
	if (id < 0)
		return -EINVAL;

	/*
	 * Read performance counter
	 */
	perf_ctr = OSReadHWReg32(info->pvRegsBaseKM, g_pvr_governor_events[id].cntr);

	/* Calculate deltas */
	if (perf_ctr >= g_pvr_governor_events[id].last_value)
	{
		edata->load_count = perf_ctr - g_pvr_governor_events[id].last_value;
	}
	else
	{
		edata->load_count = UINT32_MAX - g_pvr_governor_events[id].last_value + perf_ctr + 1;
	}

	g_pvr_governor_events[id].last_value = perf_ctr;

	/*
	 * Calc the utilisation as a fraction of the total cycles
	 */
	if (edev->desc->event_type & PVR_EVENT_OP_PERF_CNTR)
	{
		edata->total_count = info->stat->event_data[0].load_count;

		/*
		 * Timer has 256-cycle granularity
		 * Cycle counters have 1-cycle or 256-cycle granularity depending on GPU variant
		 * SLC read/write counters have 1 event granularity
		 */
		if ((g_pvr_governor_events[0].shift > 0) && (g_pvr_governor_events[id].shift == 0))
		{
			int timer_shift = g_pvr_governor_events[0].shift;
			if (edata->total_count < (UINT32_MAX >> timer_shift))
			{
				edata->total_count <<= timer_shift;	/*!< convert total to cycles */
			}
			else
			{
				/* avoid overflow but some loss of precision will occur */
				edata->load_count >>= timer_shift;	/*!< convert load to ticks */
			}
		}
	}
	else
	{
		/* timer runs continuously, effective load is 100% */
		edata->total_count = edata->load_count;
	}

	return 0;
}

static const struct devfreq_event_ops pvr_governor_event_ops = {
	.enable = pvr_events_enable,
	.disable = pvr_events_disable,
	.set_event = pvr_governor_set_event,
	.get_event = pvr_governor_get_event,
};

int pvr_events_register(struct device *dev, PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_DVFS_DEVICE *psDVFSDevice;
	struct pvr_profiling_device *pvr_prof_dev;
	struct devfreq_event_dev **edev;
	struct devfreq_event_desc *desc;
	int size, err;
	int idx;

	if (!psDeviceNode)
		return -ENODEV;

	pvr_prof_dev = devm_kzalloc(dev, sizeof(*pvr_prof_dev), GFP_KERNEL);
	if (!pvr_prof_dev)
		return -ENOMEM;

	pvr_prof_dev->dev = dev;
	pvr_prof_dev->num_events = ARRAY_SIZE(g_pvr_governor_events);

	/* Alloc the event descriptor table */
	desc = devm_kcalloc(dev, pvr_prof_dev->num_events, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	for (idx = 0; idx < pvr_prof_dev->num_events; idx++)
	{
		if (!g_pvr_governor_events[idx].name)
			continue;

		desc[idx].name = g_pvr_governor_events[idx].name;
		desc[idx].event_type = (idx < PERF_CNTR_OFF) ? PVR_EVENT_OP_TIMER : PVR_EVENT_OP_PERF_CNTR;
		desc[idx].event_type |= PVR_EVENT_OP_PERF_VER;
		desc[idx].driver_data = pvr_prof_dev;
		desc[idx].ops = &pvr_governor_event_ops;
	}
	pvr_prof_dev->desc = desc;

	size = sizeof(struct devfreq_event_dev *) * pvr_prof_dev->num_events;
	pvr_prof_dev->edev = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!pvr_prof_dev->edev)
		return -ENOMEM;

	/* Populate the events */
	for (idx = 0, edev = pvr_prof_dev->edev; idx < pvr_prof_dev->num_events; idx++)
	{
		edev[idx] = devm_devfreq_event_add_edev(dev, &desc[idx]);
		if (IS_ERR(edev[idx]))
		{
			dev_err(dev, "failed to add devfreq-event device (idx=%d)\n", idx);
			return PTR_ERR(edev[idx]);
		}

		/* enable */
		err = devfreq_event_enable_edev(edev[idx]);
		if (err)
		{
			dev_err(dev, "failed to enable devfreq-event device (idx=%d)\n", idx);
			return PTR_ERR(edev[idx]);
		}

		dev_info(dev, "%s: new PVR devfreq-event device registered %s (%s)\n",
			__func__, dev_name(dev), desc[idx].name);
	}

	/* Allocate the profiling stats, passed to governor */
	pvr_prof_dev->stat = devm_kzalloc(dev, sizeof(struct pvr_profiling_dev_status), GFP_KERNEL);
	if (!pvr_prof_dev->stat)
		return -ENOMEM;

	/* Map the reg bank */
	pvr_prof_dev->reg_size = psDeviceNode->psDevConfig->ui32RegsSize;
	pvr_prof_dev->pvRegsBaseKM = (void __iomem *) OSMapPhysToLin(psDeviceNode->psDevConfig->sRegsCpuPBase,
			psDeviceNode->psDevConfig->ui32RegsSize,
			PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (!pvr_prof_dev->pvRegsBaseKM)
	{
		dev_err(dev, "failed to map register bank.");
		return -ENODEV;
	}

	dev_info(dev, "Mapped regbank at %p, size 0x%x Bytes.\n", pvr_prof_dev->pvRegsBaseKM,
			pvr_prof_dev->reg_size);

	/* Update the DVFS device */
	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psDVFSDevice->psProfilingDevice = pvr_prof_dev;

	return 0;
}

void pvr_events_unregister(struct device *dev, IMG_DVFS_DEVICE *psDVFSDevice)
{
	struct pvr_profiling_device *pvr_prof_dev = psDVFSDevice->psProfilingDevice;
	struct devfreq_event_dev **edev;
	int idx, err;

	/* Remove the events */
	for (idx = 0, edev = pvr_prof_dev->edev; idx < pvr_prof_dev->num_events; idx++)
	{
		err = devfreq_event_disable_edev(edev[idx]);
		if (err)
		{
			dev_warn(dev, "failed to disable devfreq-event device (idx=%d)\n", idx);
		}
		devm_devfreq_event_remove_edev(dev, edev[idx]);
	}

	/* Unmap the register bank */
	if (pvr_prof_dev != NULL && pvr_prof_dev->pvRegsBaseKM != NULL)
	{
		OSUnMapPhysToLin((void __force *) pvr_prof_dev->pvRegsBaseKM,
		                 pvr_prof_dev->reg_size);
	}

	/* devfreq_event resources are managed by the kernel */
}
#endif /* CONFIG_PM_DEVFREQ_EVENT */

#endif /* SUPPORT_PVR_DVFS_GOVERNOR */

static int FillOPPTable(struct device *dev, PVRSRV_DEVICE_NODE *psDeviceNode)
{
	const IMG_OPP *iopp;
	int i, err = 0;
	IMG_DVFS_DEVICE_CFG *psDVFSDeviceCfg = NULL;

	/* Check the device exists */
	if (!dev || !psDeviceNode)
	{
		return -ENODEV;
	}

	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
	if (!psDVFSDeviceCfg->pasOPPTable)
	{
		dev_err(dev, "No DVFS OPP table provided in system layer and no device tree support.");
		return -ENODATA;
	}

	for (i = 0, iopp = psDVFSDeviceCfg->pasOPPTable;
	     i < psDVFSDeviceCfg->ui32OPPTableSize;
	     i++, iopp++)
	{
		err = dev_pm_opp_add(dev, iopp->ui32Freq, iopp->ui32Volt);
		if (err) {
			dev_err(dev, "Could not add OPP entry, %d\n", err);
			return err;
		}
	}

	return 0;
}


#if defined(CONFIG_DEVFREQ_THERMAL)
static int RegisterCoolingDevice(struct device *dev,
								 IMG_DVFS_DEVICE *psDVFSDevice,
								 struct devfreq_cooling_power *powerOps)
{
	struct device_node *of_node;
	int err = 0;

	if (!psDVFSDevice)
	{
		return -EINVAL;
	}

	if (!powerOps)
	{
		dev_info(dev, "Cooling: power ops not registered, not enabling cooling");
		return 0;
	}

	of_node = of_node_get(dev->of_node);

	psDVFSDevice->psDevfreqCoolingDevice = of_devfreq_cooling_register_power(
		of_node, psDVFSDevice->psDevFreq, powerOps);

	if (IS_ERR(psDVFSDevice->psDevfreqCoolingDevice))
	{
		err = PTR_ERR(psDVFSDevice->psDevfreqCoolingDevice);
		dev_err(dev, "Failed to register as devfreq cooling device %d", err);
	}

	of_node_put(of_node);

	return err;
}
#endif

#define TO_IMG_ERR(err) ((err == -EPROBE_DEFER) ? PVRSRV_ERROR_PROBE_DEFER : PVRSRV_ERROR_INIT_FAILURE)

PVRSRV_ERROR InitDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_DVFS_DEVICE        *psDVFSDevice = NULL;
	IMG_DVFS_DEVICE_CFG    *psDVFSDeviceCfg = NULL;
	struct device          *psDev;
	PVRSRV_ERROR            eError;
	int                     err;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

#if !defined(CONFIG_PM_OPP)
	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif

	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVR_ASSERT(psDeviceNode->psDevConfig);

	if (psDeviceNode->psDevConfig->sDVFS.sDVFSDevice.eState == PVR_DVFS_STATE_INIT_PENDING)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "DVFS initialise pending for device node %p",
				 psDeviceNode));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
	psDeviceNode->psDevConfig->sDVFS.sDVFSDevice.eState = PVR_DVFS_STATE_INIT_PENDING;

#if defined(SUPPORT_SOC_TIMER)
	if (! psDeviceNode->psDevConfig->pfnSoCTimerRead)
	{
		PVR_DPF((PVR_DBG_ERROR, "System layer SoC timer callback not implemented"));
		//return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}
#endif

	eError = SORgxGpuUtilStatsRegister(&psDVFSDevice->hGpuUtilUserDVFS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to register to the GPU utilisation stats, %d", eError));
		return eError;
	}

	err = dev_pm_opp_of_add_table(psDev);
	if (err == 0)
	{
		psDVFSDeviceCfg->bDTConfig = IMG_TRUE;
	}
	else
	{
		/*
		 * If there are no device tree or system layer provided operating points
		 * then return an error
		 */
		if ((err == -ENOTSUPP || err == -ENODEV) && psDVFSDeviceCfg->pasOPPTable)
		{
			err = FillOPPTable(psDev, psDeviceNode);
			if (err != 0 && err != -ENODATA)
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to fill OPP table with data, %d", err));
				eError = PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
				goto err_exit;
			}
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to init opp table from devicetree, %d", err));
			eError = PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
			goto err_exit;
		}
	}


#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
#if defined(CONFIG_PM_DEVFREQ_EVENT)
	err = pvr_events_register(psDev, psDeviceNode);
	if (err != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to init PVR profiling events, %d", err));
		goto err_exit;
	}
#endif

	err = pvr_governor_init();
	if (err != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to init PVR governor, %d", err));
		goto err_exit;
	}
	psDVFSDevice->bGovernorReady = true;
#endif

	PVR_ASSERT(psDev);
	PVR_ASSERT(psDeviceNode);

	PVR_TRACE(("PVR DVFS init pending: dev = %p, PVR device = %p",
			   psDev, psDeviceNode));

	return PVRSRV_OK;

err_exit:
	DeinitDVFS(psDeviceNode);
	return eError;
}

PVRSRV_ERROR RegisterDVFSDevice(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_DVFS_DEVICE        *psDVFSDevice = NULL;
	IMG_DVFS_DEVICE_CFG    *psDVFSDeviceCfg = NULL;
	IMG_DVFS_GOVERNOR_CFG  *psDVFSGovernorCfg = NULL;
	RGX_TIMING_INFORMATION *psRGXTimingInfo = NULL;
	struct device          *psDev;
	struct pvr_opp_freq_table	pvr_freq_table = {0};
	unsigned long           min_freq = 0, max_freq = 0, min_volt = 0;
	PVRSRV_ERROR            eError;
	int                     err;

	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (psDeviceNode->psDevConfig->sDVFS.sDVFSDevice.eState != PVR_DVFS_STATE_INIT_PENDING)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "DVFS initialise not yet pending for device node %p",
				 psDeviceNode));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
	psDVFSGovernorCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSGovernorCfg;
	psRGXTimingInfo = ((RGX_DATA *)psDeviceNode->psDevConfig->hDevData)->psRGXTimingInfo;
	psDeviceNode->psDevConfig->sDVFS.sDVFSDevice.eState = PVR_DVFS_STATE_READY;

	err = GetOPPValues(psDev, &min_freq, &min_volt, &max_freq, &pvr_freq_table);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to read OPP points, %d", err));
		eError = TO_IMG_ERR(err);
		goto err_exit;
	}

	img_devfreq_dev_profile.freq_table = pvr_freq_table.freq_table;
	img_devfreq_dev_profile.max_state = pvr_freq_table.num_levels;
	img_devfreq_dev_profile.initial_freq = min_freq;
	img_devfreq_dev_profile.polling_ms = psDVFSDeviceCfg->ui32PollMs;

	psRGXTimingInfo->ui32CoreClockSpeed = min_freq;

	psDVFSDeviceCfg->pfnSetFrequency(psDeviceNode->psDevConfig->hSysData, min_freq);
	psDVFSDeviceCfg->pfnSetVoltage(psDeviceNode->psDevConfig->hSysData, min_volt);

#if !defined(SUPPORT_PVR_DVFS_GOVERNOR)
	/* Use the Linux 'simple_ondemand' governor */
	psDVFSDevice->data.upthreshold = psDVFSGovernorCfg->ui32UpThreshold;
	psDVFSDevice->data.downdifferential = psDVFSGovernorCfg->ui32DownDifferential;
#endif

#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
	psDVFSDevice->data.ui32UpThreshold = psDVFSGovernorCfg->ui32UpThreshold;
	psDVFSDevice->data.ui32DownDifferential = psDVFSGovernorCfg->ui32DownDifferential;

#if defined(SUPPORT_RGX)
	{
		PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO*) psDeviceNode->pvDevice;
		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_MEMBUS))
		{
			psDVFSDevice->data.uiNumMembus = RGX_GET_FEATURE_VALUE(psDevInfo, NUM_MEMBUS);
		}
		else
		{
			psDVFSDevice->data.uiNumMembus = 1;
		}
	}
#endif
#endif

	psDVFSDevice->psDevFreq = devm_devfreq_add_device(psDev,
													  &img_devfreq_dev_profile,
#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
													  "pvr_balanced",
													  &psDVFSDevice->data);
#else
													  "simple_ondemand",
													  &psDVFSDevice->data);
#endif

	if (IS_ERR(psDVFSDevice->psDevFreq))
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "Failed to add as devfreq device %p, %ld",
				 psDVFSDevice->psDevFreq,
				 PTR_ERR(psDVFSDevice->psDevFreq)));
		eError = TO_IMG_ERR(PTR_ERR(psDVFSDevice->psDevFreq));
		goto err_exit;
	}
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 50)))
	/* Handle Linux kernel bug where a NULL return can occur. */
	if (psDVFSDevice->psDevFreq == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "Failed to add as devfreq device %p, NULL return",
				 psDVFSDevice->psDevFreq));
		eError = TO_IMG_ERR(-EINVAL);
		goto err_exit;
	}
#endif

	eError = SuspendDVFS(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInit: Failed to suspend DVFS"));
		goto err_exit;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	psDVFSDevice->psDevFreq->scaling_min_freq = min_freq;
	psDVFSDevice->psDevFreq->scaling_max_freq = max_freq;
#else
	psDVFSDevice->psDevFreq->min_freq = min_freq;
	psDVFSDevice->psDevFreq->max_freq = max_freq;
#endif
#if defined(CONFIG_PM_DEVFREQ_EVENT) && defined(SUPPORT_PVR_DVFS_GOVERNOR)
	psDVFSDevice->psDevFreq->last_status.private_data =
		(void *)psDVFSDevice->psProfilingDevice->stat;
#endif

	err = devfreq_register_opp_notifier(psDev, psDVFSDevice->psDevFreq);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to register opp notifier, %d", err));
		eError = TO_IMG_ERR(err);
		goto err_exit;
	}

#if defined(CONFIG_DEVFREQ_THERMAL)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		err = RegisterCoolingDevice(psDev, psDVFSDevice, psDVFSDeviceCfg->psPowerOps);
		if (err)
		{
			eError = TO_IMG_ERR(err);
			goto err_exit;
		}
	}
#endif

	PVR_TRACE(("PVR DVFS activated: %lu-%lu Hz, Period: %ums",
			   min_freq,
			   max_freq,
			   psDVFSDeviceCfg->ui32PollMs));

	return PVRSRV_OK;

err_exit:
	UnregisterDVFSDevice(psDeviceNode);
	return eError;
}

void UnregisterDVFSDevice(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_DVFS_DEVICE *psDVFSDevice = NULL;
	struct device *psDev = NULL;
	IMG_INT32 i32Error;

	/* Check the device exists */
	if (!psDeviceNode)
	{
		return;
	}

	PVRSRV_VZ_RETN_IF_MODE(GUEST, DEVNODE, psDeviceNode);

	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psDev = psDeviceNode->psDevConfig->pvOSDevice;

	if (! psDVFSDevice)
	{
		return;
	}

#if defined(CONFIG_DEVFREQ_THERMAL)
	if (!IS_ERR_OR_NULL(psDVFSDevice->psDevfreqCoolingDevice))
	{
		devfreq_cooling_unregister(psDVFSDevice->psDevfreqCoolingDevice);
		psDVFSDevice->psDevfreqCoolingDevice = NULL;
	}
#endif

	if (psDVFSDevice->psDevFreq)
	{
		i32Error = devfreq_unregister_opp_notifier(psDev, psDVFSDevice->psDevFreq);
		if (i32Error < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to unregister OPP notifier"));
		}

		devm_devfreq_remove_device(psDev, psDVFSDevice->psDevFreq);
		psDVFSDevice->psDevFreq = NULL;
	}

	psDVFSDevice->eState = PVR_DVFS_STATE_DEINIT;
}

void DeinitDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_DVFS_DEVICE *psDVFSDevice = NULL;
	struct device *psDev = NULL;

	/* Check the device exists */
	if (!psDeviceNode)
	{
		return;
	}

	PVRSRV_VZ_RETN_IF_MODE(GUEST, DEVNODE, psDeviceNode);

	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	psDev = psDeviceNode->psDevConfig->pvOSDevice;

#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
	if (psDVFSDevice->psProfilingDevice)
	{
		pvr_events_unregister(psDev, psDVFSDevice);
	}
	if (psDVFSDevice->bGovernorReady)
	{
		pvr_governor_exit();
		psDVFSDevice->bGovernorReady = false;
	}
#endif

	/*
	 * Remove OPP entries for this device; both static entries from
	 * the device tree and dynamic entries.
	 */
	dev_pm_opp_of_remove_table(psDev);

	SORgxGpuUtilStatsUnregister(psDVFSDevice->hGpuUtilUserDVFS);
	psDVFSDevice->hGpuUtilUserDVFS = NULL;
	psDVFSDevice->eState = PVR_DVFS_STATE_NONE;
}

PVRSRV_ERROR SuspendDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_DVFS_DEVICE	*psDVFSDevice = NULL;
#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
	int err;
#endif

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;
	if (psDVFSDevice->eState == PVR_DVFS_STATE_DEINIT)
	{
		/* Device is shutting down, nothing to do. */
		return PVRSRV_OK;
	}
	psDVFSDevice->eState = PVR_DVFS_STATE_OFF;
#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
	/* Communicate power suspend to devfreq framework */
	err = devfreq_suspend_device(psDVFSDevice->psDevFreq);
	if (err < 0)
	{
		PVR_DPF((PVR_DBG_WARNING, "Failed to suspend DVFS (%d)", err));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR ResumeDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
	IMG_DVFS_DEVICE	*psDVFSDevice = NULL;
#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
	int err;
#endif

	/* Check the device is registered */
	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	psDVFSDevice = &psDeviceNode->psDevConfig->sDVFS.sDVFSDevice;

	/* Not supported in GuestOS drivers */
	psDVFSDevice->eState = PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) ? PVR_DVFS_STATE_NONE : PVR_DVFS_STATE_READY;
#if defined(SUPPORT_PVR_DVFS_GOVERNOR)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Communicate power resume to devfreq framework */
		err = devfreq_resume_device(psDVFSDevice->psDevFreq);
		if (err < 0)
		{
			PVR_DPF((PVR_DBG_WARNING, "Failed to resume DVFS (%d)", err));
			return PVRSRV_ERROR_INVALID_DEVICE;
		}
	}
#endif

	return PVRSRV_OK;
}


#endif /* !NO_HARDWARE */

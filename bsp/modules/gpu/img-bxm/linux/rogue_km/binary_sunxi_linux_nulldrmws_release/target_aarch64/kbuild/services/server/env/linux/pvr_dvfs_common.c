/*************************************************************************/ /*!
@File           pvr_dvfs_common.c
@Title          PowerVR devfreq device common utilities
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Linux DVFS and PDVFS shared code
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

#include <linux/version.h>
#include <linux/device.h>
#include <drm/drm.h>
#if defined(CONFIG_PM_OPP)
#include <linux/pm_opp.h>
#endif

#include "pvrsrv.h"

/*
 * Common DVFS support code shared between SUPPORT_LINUX_DVFS and
 * SUPPORT_PDVFS, primarily for OPP table support.
 *
 * Note that PDVFS implements the Linux/OS devfreq module in
 * the firmware, so no devfreq API calls should be used here.
 */
#include "pvr_dvfs.h"
#include "pvr_dvfs_common.h"

#include "kernel_compatibility.h"


/*************************************************************************/ /*!
@Function       GetOPPValues

@Description    Common code to store the OPP points in the freq_table,
                for use with the IMG DVFS/devfreq implementation, or with
                the Proactive DVFS.
                Requires CONFIG_PM_OPP support in the kernel.

@Input          dev        OS Device node
@Output         min_freq   Min clock freq (Hz)
@Output         min_volt   Min voltage (V)
@Output         max_freq   Max clock freq (Hz)
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
#if defined(CONFIG_PM_OPP)
int GetOPPValues(struct device *dev,
                 unsigned long *min_freq,
                 unsigned long *min_volt,
                 unsigned long *max_freq,
                 struct pvr_opp_freq_table *pvr_freq_table)
{
	struct dev_pm_opp *opp;
	int count, i, err = 0;
	unsigned long freq;

	unsigned long *freq_table;

	count = dev_pm_opp_get_opp_count(dev);
	if (count <= 0)
	{
		dev_err(dev, "Could not fetch OPP count, %d\n", count);
		return -EINVAL;
	}

	dev_info(dev, "Found %d OPP points.\n", count);

	freq_table = devm_kcalloc(dev, count, sizeof(*freq_table), GFP_ATOMIC);
	if (! freq_table)
	{
		return -ENOMEM;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	/* Start RCU read-side critical section to map frequency to OPP */
	rcu_read_lock();
#endif

	/* Iterate over OPP table; Iteration 0 finds "opp w/ freq >= 0 Hz". */
	freq = 0;
	opp = dev_pm_opp_find_freq_ceil(dev, &freq);
	if (IS_ERR(opp))
	{
		err = PTR_ERR(opp);
		dev_err(dev, "Couldn't find lowest frequency, %d\n", err);
		goto exit;
	}

	*min_volt = dev_pm_opp_get_voltage(opp);
	*max_freq = *min_freq = freq_table[0] = freq;
	dev_info(dev, "opp[%d/%d]: (%lu Hz, %lu uV)\n", 1, count, freq, *min_volt);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
	dev_pm_opp_put(opp);
#endif

	/* Iteration i > 0 finds "opp w/ freq >= (opp[i-1].freq + 1)". */
	for (i = 1; i < count; i++)
	{
		freq++;
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp))
		{
			err = PTR_ERR(opp);
			dev_err(dev, "Couldn't find %dth frequency, %d\n", i, err);
			goto exit;
		}

		freq_table[i] = freq;
		*max_freq = freq;
		dev_info(dev,
				 "opp[%d/%d]: (%lu Hz, %lu uV)\n",
				  i + 1,
				  count,
				  freq,
				  dev_pm_opp_get_voltage(opp));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
		dev_pm_opp_put(opp);
#endif
	}

exit:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	rcu_read_unlock();
#endif

	if (!err)
	{
		pvr_freq_table->freq_table = freq_table;
		pvr_freq_table->num_levels = count;
	}
	else
	{
		devm_kfree(dev, freq_table);
	}

	return err;
}
#endif

/*************************************************************************/ /*!
@Function       DVFSCopyOPPTable

@Description    Copies the OPP points (voltage/frequency) to the target
                firmware structure RGXFWIF_OPP_INFO which must be pre-allocated.
                The source values are expected to be read into the kernel
                Power Manager from the platform's Device Tree.
                Requires CONFIG_OF and CONFIG_PM_OPP support in the kernel.

@Input          psDeviceNode       Device node
@Input          psOPPInfo          Target OPP data buffer
@Input          ui32MaxOPPLevels   Maximum number of OPP levels allowed in buffer.
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
#if defined(SUPPORT_FW_OPP_TABLE) && defined(CONFIG_OF) && defined(CONFIG_PM_OPP)
PVRSRV_ERROR DVFSCopyOPPTable(PPVRSRV_DEVICE_NODE psDeviceNode,
							  RGXFWIF_OPP_INFO   *psOPPInfo,
							  IMG_UINT32          ui32MaxOPPLevels)
{
	PVRSRV_ERROR            eError = PVRSRV_OK;
	struct device          *psDev = NULL;
	OPP_LEVEL              *psOPPValue;
	struct dev_pm_opp      *opp;
	struct pvr_opp_freq_table pvr_freq_table = {0};
	unsigned long           min_freq = 0, max_freq = 0, min_volt = 0;
	unsigned int            i, err;
	unsigned long *freq_table;

	if (!psDeviceNode || !psOPPInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid device or argument", __func__));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	psDev = psDeviceNode->psDevConfig->pvOSDevice;

	err = GetOPPValues(psDev, &min_freq, &min_volt, &max_freq, &pvr_freq_table);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: DVFS OPP table not initialised.", __func__));
		return PVRSRV_ERROR_NOT_INITIALISED;
	}

	freq_table = pvr_freq_table.freq_table;

	if (pvr_freq_table.num_levels > ui32MaxOPPLevels)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Too many OPP levels (%u), max (%u).", __func__,
		         pvr_freq_table.num_levels, ui32MaxOPPLevels));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto exit;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	/* Start RCU read-side critical section to map frequency to OPP */
	rcu_read_lock();
#endif

	/* Loop over the OPP/frequency levels */
	psOPPValue = &psOPPInfo->asOPPValues[0];
	for (i=0; i<pvr_freq_table.num_levels; i++)
	{
		psOPPValue->ui32Freq = freq_table[i];
		opp = dev_pm_opp_find_freq_exact(psDev, freq_table[i], IMG_TRUE);
		if (IS_ERR(opp))
		{
			err = PTR_ERR(opp);
			dev_err(psDev, "Couldn't find %dth frequency, %d\n", i, err);
			eError = PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
			goto exit;
		}
		psOPPValue->ui32Volt = dev_pm_opp_get_voltage(opp);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
		dev_pm_opp_put(opp);
#endif
		psOPPValue++;
	}

	PVR_DPF((PVR_DBG_WARNING, "%s: Copied %u OPP points to the FW processor table.", __func__,
	         pvr_freq_table.num_levels));
	psOPPInfo->ui32MaxOPPPoint = pvr_freq_table.num_levels - 1;
exit:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	rcu_read_unlock();
#endif

	/* Free memory allocated in GetOPPValues */
	devm_kfree(psDev, freq_table);

	return eError;
}
#endif

/*************************************************************************/ /*!
@Function       InitPDVFS

@Description    Initialise the device for Proactive DVFS support.
                Prepares the OPP table from the devicetree, if enabled.

@Input          psDeviceNode       Device node
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
#if defined(SUPPORT_PDVFS)
PVRSRV_ERROR InitPDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
#if !(defined(CONFIG_PM_OPP) && defined(CONFIG_OF))
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return PVRSRV_OK;
#else
	IMG_DVFS_DEVICE_CFG    *psDVFSDeviceCfg = NULL;
	struct device          *psDev;
	int                     err;

	if (!psDeviceNode)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVR_ASSERT(psDeviceNode->psDevConfig);

	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;

	/* Setup the OPP table from the device tree for Proactive DVFS. */
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
		if (psDVFSDeviceCfg->pasOPPTable)
		{
			psDVFSDeviceCfg->bDTConfig = IMG_FALSE;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "No system or device tree opp points found, %d", err));
			return PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
		}
	}
	return PVRSRV_OK;
#endif
}

/*************************************************************************/ /*!
@Function       DeinitPDVFS

@Description    De-Initialise the device for Proactive DVFS support.

@Input          psDeviceNode       Device node
@Return			PVRSRV_ERROR
*/ /**************************************************************************/
void DeinitPDVFS(PPVRSRV_DEVICE_NODE psDeviceNode)
{
#if !(defined(CONFIG_PM_OPP) && defined(CONFIG_OF))
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
#else
	IMG_DVFS_DEVICE_CFG *psDVFSDeviceCfg = NULL;
	struct device *psDev = NULL;

	/* Check the device exists */
	if (!psDeviceNode)
	{
		return;
	}

	psDev = psDeviceNode->psDevConfig->pvOSDevice;
	psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;

	if (psDVFSDeviceCfg->bDTConfig)
	{
		/*
		 * Remove OPP entries for this device; only static entries from
		 * the device tree are present.
		 */
		dev_pm_opp_of_remove_table(psDev);
	}
#endif
}
#endif /* SUPPORT_PDVFS */

#endif /* !NO_HARDWARE */

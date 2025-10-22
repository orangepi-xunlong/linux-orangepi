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

#if defined(__linux__)

#include <linux/pci.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

/* For MODULE_LICENSE */
#include "pvrmodule.h"

#include "dc_osfuncs.h"
#include "dc_pdp.h"

#if defined(ENABLE_PLATO_HDMI)
/* APIs needed from plato_hdmi */
EXPORT_SYMBOL(PDPInitialize);
#endif

#define DCPDP_WIDTH_MIN			(640)
#define DCPDP_WIDTH_MAX			(1920)
#define DCPDP_HEIGHT_MIN		(480)
#define DCPDP_HEIGHT_MAX		(1080)
#define DCPDP_WIDTH_DEFAULT		(640)
#define DCPDP_HEIGHT_DEFAULT	(480)

#define DCPDP_DEBUGFS_DISPLAY_ENABLED   "display_enabled"
#define DCPDP_DEBUGFS_NUM_DEVICES       "num_devices"

#if defined(DCPDP_WIDTH) && !defined(DCPDP_HEIGHT)
#error ERROR: DCPDP_WIDTH defined but DCPDP_HEIGHT not defined
#elif !defined(DCPDP_WIDTH) && defined(DCPDP_HEIGHT)
#error ERROR: DCPDP_HEIGHT defined but DCPDP_WIDTH not defined
#elif !defined(DCPDP_WIDTH) && !defined(DCPDP_HEIGHT)
#define DCPDP_WIDTH			DCPDP_WIDTH_DEFAULT
#define DCPDP_HEIGHT		DCPDP_HEIGHT_DEFAULT
#elif (DCPDP_WIDTH > DCPDP_WIDTH_MAX)
#error ERROR: DCPDP_WIDTH too large (max: 1920)
#elif (DCPDP_WIDTH < DCPDP_WIDTH_MIN)
#error ERROR: DCPDP_WIDTH too small (min: 640)
#elif (DCPDP_HEIGHT > DCPDP_HEIGHT_MAX)
#error ERROR: DCPDP_HEIGHT too large (max: 1080)
#elif (DCPDP_HEIGHT < DCPDP_HEIGHT_MIN)
#error ERROR: DCPDP_HEIGHT too small (max: 480)
#endif

static DLLIST_NODE g_sDeviceDataListHead;    /* List head for devices supported
                                                by this driver. */
static IMG_UINT32 g_uiNumPDPDevs;            /* Number of devices found. */

static struct dentry    *g_psDebugFSEntryDir; /* Top-level 'dc_pdp2' entry */
static struct dentry    *g_psNumDevicesEntry; /* 'num_devices' entry */
static struct dentry    *g_psDispEnabledEntry; /* 'display_enabled' entry */

/* PDP module parameters */
DCPDP_MODULE_PARAMETERS sModuleParams =
{
	.ui32PDPEnabled = 1,
	.ui32PDPWidth   = DCPDP_WIDTH,
	.ui32PDPHeight  = DCPDP_HEIGHT
};
module_param_named(mem_en, sModuleParams.ui32PDPEnabled, uint, S_IRUGO | S_IWUSR);
module_param_named(width,  sModuleParams.ui32PDPWidth,   uint, S_IRUGO | S_IWUSR);
module_param_named(height, sModuleParams.ui32PDPHeight,  uint, S_IRUGO | S_IWUSR);

const DCPDP_MODULE_PARAMETERS *DCPDPGetModuleParameters(void)
{
	return &sModuleParams;
}

static int DisplayEnabledOpen(struct inode *psINode, struct file *psFile)
{
	psFile->private_data = psINode->i_private;

	return 0;
}

static ssize_t NumDevicesRead(struct file *psFile,
				  char __user *psUserBuffer,
				  size_t uiCount,
				  loff_t *puiPosition)
{
	char aszBuffer[] = "XX\n";
	loff_t uiPosition = *puiPosition;
	size_t uiBufferSize = ARRAY_SIZE(aszBuffer);
	size_t uiLocCount;

	PVR_UNREFERENCED_PARAMETER(psFile);

	if (uiPosition < 0)
	{
		return -EINVAL;
	}
	else if ((uiPosition >= (loff_t)uiBufferSize) || (uiCount == 0U))
	{
		return 0;
	}

	(void) DC_OSSNPrintf(aszBuffer, sizeof(aszBuffer), "%u", g_uiNumPDPDevs);

	if (uiCount > (uiBufferSize - (size_t)uiPosition))
	{
		uiLocCount = uiBufferSize - (size_t)uiPosition;
	}
	else
	{
		uiLocCount = uiCount;
	}

	if (copy_to_user(psUserBuffer, &aszBuffer[uiPosition], uiLocCount) != 0)
	{
		return -EFAULT;
	}

	*puiPosition = uiPosition + (loff_t)uiLocCount;

	return (ssize_t)uiLocCount;
}

static ssize_t DisplayEnabledRead(struct file *psFile,
				  char __user *psUserBuffer,
				  size_t uiCount,
				  loff_t *puiPosition)
{
	DCPDP_DEVICE *psDeviceData = (DCPDP_DEVICE *)psFile->private_data;
	IMG_BOOL bPDPEnabled;
	loff_t uiPosition = *puiPosition;
	char pszBuffer[] = "N\n";
	size_t uiBufferSize = ARRAY_SIZE(pszBuffer);
	size_t uiLocCount;

	if (uiPosition < 0)
	{
		return -EINVAL;
	}
	else if ((uiPosition >= (loff_t)uiBufferSize) || (uiCount == 0U))
	{
		return 0;
	}

	/* Handle the top-level 'meta' entry which reports the values of all of
	 * the underlying unit(s) using logical AND of all values.
	 */
	if (psDeviceData == NULL)
	{
		/* Iterate across all present units and construct the top-level value.
		 * IMG_TRUE => all devices are enabled.
		 * IMG_FALSE => at least one device is disabled.
		 */
		DLLIST_NODE *psNode, *psNext;

		bPDPEnabled = IMG_TRUE;
		dllist_foreach_node(&g_sDeviceDataListHead, psNode, psNext)
		{
			psDeviceData = IMG_CONTAINER_OF(psNode, DCPDP_DEVICE, sListNode);

			bPDPEnabled &= psDeviceData->bPDPEnabled;
		}
	}
	else
	{
		bPDPEnabled = psDeviceData->bPDPEnabled;
	}

	if (bPDPEnabled == IMG_TRUE)
	{
		pszBuffer[0] = 'Y';
	}

	if (uiCount > (uiBufferSize - (size_t)uiPosition))
	{
		uiLocCount = uiBufferSize - (size_t)uiPosition;
	}
	else
	{
		uiLocCount = uiCount;
	}

	if (copy_to_user(psUserBuffer, &pszBuffer[uiPosition], uiLocCount) != 0)
	{
		return -EFAULT;
	}

	*puiPosition = uiPosition + (loff_t)uiLocCount;

	return (ssize_t)uiLocCount;
}

static ssize_t DisplayEnabledWrite(struct file *psFile,
				   const char __user *psUserBuffer,
				   size_t uiCount,
				   loff_t *puiPosition)
{
	DCPDP_DEVICE *psDeviceData = (DCPDP_DEVICE *)(psFile->private_data);
	char pszBuffer[3];
	bool bPDPEnabled;
	size_t uiBufferSize = ARRAY_SIZE(pszBuffer);
	size_t uiLocCount;

	uiLocCount = min(uiCount, (uiBufferSize - 1U));

	if (copy_from_user(pszBuffer, psUserBuffer, uiLocCount) != 0)
	{
		return -EFAULT;
	}

	pszBuffer[uiLocCount] = '\0';

	if (kstrtobool(pszBuffer, &bPDPEnabled) == 0)
	{
		/* If we have no device-specific data we iterate over all units */
		if (psDeviceData == NULL)
		{
			DLLIST_NODE *psNode, *psNext;
			dllist_foreach_node(&g_sDeviceDataListHead, psNode, psNext)
			{
				psDeviceData = IMG_CONTAINER_OF(psNode, DCPDP_DEVICE, sListNode);
				psDeviceData->bPDPEnabled = bPDPEnabled ? IMG_TRUE : IMG_FALSE;

#if defined(PDP_DEBUG)
				printk(KERN_INFO DRVNAME " - %s: Setting display %u %s\n", __func__,
				       psDeviceData->ui32Instance, bPDPEnabled ? "Enabled" : "Disabled");
#endif

				DCPDPEnableMemoryRequest(psDeviceData, psDeviceData->bPDPEnabled);
			}
		}
		else
		{
			psDeviceData->bPDPEnabled = bPDPEnabled ? IMG_TRUE : IMG_FALSE;

#if defined(PDP_DEBUG)
			printk(KERN_INFO DRVNAME " - %s: Setting display %u %s\n", __func__,
			       psDeviceData->ui32Instance, bPDPEnabled ? "Enabled" : "Disabled");
#endif

			DCPDPEnableMemoryRequest(psDeviceData, psDeviceData->bPDPEnabled);
		}
	}

	return (ssize_t)uiLocCount;
}

#if defined(PDP_DEBUG)
static long DisplayEnabledIoctl(struct file *file, unsigned int ioctl, unsigned long argp)
{
	PDPDebugCtrl();
	return 0;
}
#endif /* PDP_DEBUG */

static const struct file_operations gsDisplayEnabledFileOps =
{
	.owner = THIS_MODULE,
	.open = DisplayEnabledOpen,
	.read = DisplayEnabledRead,
	.write = DisplayEnabledWrite,
	.llseek = default_llseek,
#if defined(PDP_DEBUG)
	.unlocked_ioctl = DisplayEnabledIoctl
#endif /* PDP_DEBUG */
};

static const struct file_operations gsNumDevicesFileOps =
{
	.owner = THIS_MODULE,
	.open = DisplayEnabledOpen,
	.read = NumDevicesRead,
	.llseek = default_llseek
};

PVRSRV_ERROR PDPInitialize(VIDEO_PARAMS * pVideoParams, IMG_UINT32 uiInstance)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	DLLIST_NODE *psNode, *psNext;

	/* Iterate over all devices and initialise them. May need to be changed to
	 * support different video params per device ... TBD
	 */
	dllist_foreach_node(&g_sDeviceDataListHead, psNode, psNext)
	{
		DCPDP_DEVICE *psDeviceData = IMG_CONTAINER_OF(psNode, DCPDP_DEVICE, sListNode);
		if (psDeviceData->ui32Instance == uiInstance)
		{
			(void) printk(KERN_INFO DRVNAME " - %s: Matched Device %u\n",
			              __func__, psDeviceData->ui32Instance);

			if (psDeviceData->bPreInitDone == IMG_FALSE)
			{
				(void) printk(KERN_ERR DRVNAME
				              "-%s: Called before PreInit is complete\n",
				              __func__);
				return -ENODEV;
			}

			psDeviceData->videoParams = *pVideoParams;

			eError = DCPDPStart(psDeviceData);
			if (eError != PVRSRV_OK)
			{
				(void) printk(KERN_ERR DRVNAME
				              " - %s: Failed to initialise device (%d)\n",
				              __func__, eError);
				return -ENODEV;
			}

			/* Enable / Disable display according to default modparam value */
			if (sModuleParams.ui32PDPEnabled == 1)
			{
				if (!psDeviceData->bPDPEnabled)
				{
#if defined(PDP_DEBUG)
					printk(KERN_WARNING DRVNAME " - %s: Enabling display %u\n",
					       __func__, psDeviceData->ui32Instance);
#endif
					psDeviceData->bPDPEnabled = IMG_TRUE;
					DCPDPEnableMemoryRequest(psDeviceData, IMG_TRUE);
				}
			}
			else
			{
				if (psDeviceData->bPDPEnabled)
				{
#if defined(PDP_DEBUG)
					printk(KERN_WARNING DRVNAME " - %s: Disabling display %u\n",
					       __func__, psDeviceData->ui32Instance);
#endif
					psDeviceData->bPDPEnabled = IMG_FALSE;
					DCPDPEnableMemoryRequest(psDeviceData, IMG_FALSE);
				}
			}
		}
	}

	return eError;
}

static struct pci_dev *dc_pdp_getdevice(struct pci_dev *fromDev)
{
	struct pci_dev *psPCIDev;

	psPCIDev = pci_get_device(DCPDP2_VENDOR_ID_PLATO,
	                          DCPDP2_DEVICE_ID_PCI_PLATO,
	                          fromDev);

	return psPCIDev;
}

static int __init dc_pdp_init(void)
{
	DCPDP_DEVICE *psDevicePriv;
	struct pci_dev *psPCIDev, *fromDev = NULL;
	PVRSRV_ERROR eError;
	int error;
	IMG_CHAR aszDevName[4];	/* Unit number of device "0" .. "N" */

	/* Iterate over all PCI instances */
	dllist_init(&g_sDeviceDataListHead);
	while ((psPCIDev = dc_pdp_getdevice(fromDev)) != NULL)
	{
		fromDev = psPCIDev;

		error = pci_enable_device(psPCIDev);
		if (error != 0)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to enable PCI device (%d)\n", __func__, error);
			continue;  /* Go to next potential instance. Drop reference */
		}

		eError = DCPDPInit(&psPCIDev->dev, &psDevicePriv, g_uiNumPDPDevs);
		if (eError != PVRSRV_OK)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to initialise device (%d)\n", __func__, eError);
			continue;  /* Go to next potential instance. Drop reference */
		}

		dllist_init(&psDevicePriv->sListNode);

		/* To prevent possible problems with system suspend/resume, we don't
		   keep the device enabled, but rely on the fact that the Rogue driver
		   will have done a pci_enable_device. */
		pci_disable_device(psPCIDev);

		/* We need to create a debugFS top-level entry for the dc_pdp2 device
		 * once we've found the first PCI device. This is used as the root
		 * debugFS directory for device-specific entries and holds the number
		 * of devices found entry in 'num_devices'. Only do this on the first
		 * iteration of the discovery loop.
		 * Note: The creation of debugFS style entries may be disabled due to
		 *       OS configuration. If so, we will continue with the driver
		 *       being initialised but there will not be a way of observing
		 *       the values provided by the debugFS entries.
		 */
		if (g_uiNumPDPDevs == 0U)
		{
			/* Top-level directory entry. All devices have a sub-directory */
			g_psDebugFSEntryDir = debugfs_create_dir(DRVNAME, NULL);
			if (IS_ERR_OR_NULL(g_psDebugFSEntryDir))
			{
				printk(KERN_WARNING DRVNAME
				       " - %s: Failed to create '%s' debugfs root directory "
				       "(debugfs entries won't be available)\n", __func__,
				       DRVNAME);
				g_psDebugFSEntryDir = NULL;
			}
			else
			{
				/* Create the 'num_devices' top-level file. This will report the
				 * current value of g_uiNumPDPDevs. Varies from 1 .. N.
				 */
				g_psNumDevicesEntry = debugfs_create_file(DCPDP_DEBUGFS_NUM_DEVICES,
				                                          S_IFREG | S_IRUGO,
				                                          g_psDebugFSEntryDir,
				                                          NULL,	/* Top device */
				                                          &gsNumDevicesFileOps);

				if (IS_ERR_OR_NULL(g_psNumDevicesEntry))
				{
					printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s'"
					       " debugfs file "
					       "(debugfs entries won't be available)\n", __func__,
					       DCPDP_DEBUGFS_NUM_DEVICES);
					g_psNumDevicesEntry = NULL;
				}
			}

			/* Create top-level 'display_enabled' file. This reports the
			 * combined status of all device display_enabled settings and sets
			 * all devices to the same status.
			 */
			g_psDispEnabledEntry = debugfs_create_file(DCPDP_DEBUGFS_DISPLAY_ENABLED,
			                                              S_IFREG | S_IRUGO | S_IWUSR,
			                                              g_psDebugFSEntryDir,
			                                              NULL,	/* Top device */
			                                              &gsDisplayEnabledFileOps);
			if (IS_ERR_OR_NULL(g_psDispEnabledEntry))
			{
				printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s'"
				       " debugfs file "
				       "(debugfs entries won't be available)\n", __func__,
				       DCPDP_DEBUGFS_DISPLAY_ENABLED);
				g_psDispEnabledEntry = NULL;
			}
		}

		/* Create device-specific directory name. This is the unit / instance
		 * number of this device (i.e., the current value of g_uiNumPDPDevs as
		 * this is incremented for each device we enumerate). */
		if (DC_OSSNPrintf(aszDevName, sizeof(aszDevName), "%u", g_uiNumPDPDevs) >= sizeof(aszDevName))
		{
			printk(KERN_WARNING DRVNAME " - %s: DC_OSSNPrintf of '%u' failed"
			       " Returned '%s'\n",
			       __func__, g_uiNumPDPDevs, aszDevName);
		};

		if (!IS_ERR_OR_NULL(g_psDebugFSEntryDir))
		{
			psDevicePriv->psDebugFSEntryDir = debugfs_create_dir(aszDevName, g_psDebugFSEntryDir);
			if (IS_ERR_OR_NULL(psDevicePriv->psDebugFSEntryDir))
			{
				printk(KERN_WARNING DRVNAME
				       " - %s: Failed to create '%s/%s' debugfs device directory "
				       "(debugfs entries won't be available)\n", __func__, DRVNAME,
				       aszDevName);
				psDevicePriv->psDebugFSEntryDir = NULL;
			}
			else
			{

				psDevicePriv->psDisplayEnabledEntry = debugfs_create_file(DCPDP_DEBUGFS_DISPLAY_ENABLED,
				                                                          S_IFREG | S_IRUGO | S_IWUSR,
				                                                          psDevicePriv->psDebugFSEntryDir,
				                                                          psDevicePriv,	/* This device */
				                                                          &gsDisplayEnabledFileOps);
				if (IS_ERR_OR_NULL(psDevicePriv->psDisplayEnabledEntry))
				{
					printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s' debugfs entry\n",
					         __func__, DCPDP_DEBUGFS_DISPLAY_ENABLED);
					         psDevicePriv->psDisplayEnabledEntry = NULL;
				}
			}
		}

#if defined(PDP_DEBUG)
		printk(KERN_INFO DRVNAME " - %s: Initialised instance %u\n",
		       __func__, g_uiNumPDPDevs);
#endif

		psDevicePriv->ui32Instance = g_uiNumPDPDevs++;
		psDevicePriv->bPreInitDone = IMG_TRUE;
		/* Add to the list of known devices */
		dllist_add_to_tail(&g_sDeviceDataListHead, &psDevicePriv->sListNode);
	}

#if defined(PDP_DEBUG)
	printk(KERN_INFO DRVNAME " - %s: Found %u devices\n", __func__, g_uiNumPDPDevs);
#endif
	return (g_uiNumPDPDevs > 0) ? 0 : -ENODEV;
}

static void __exit dc_pdp_deinit(void)
{
	DCPDP_DEVICE *psDevicePriv;
	DLLIST_NODE *psNode, *psNext;

	dllist_foreach_node(&g_sDeviceDataListHead, psNode, psNext)
	{
		struct pci_dev *psPCIDev;
		psDevicePriv = IMG_CONTAINER_OF(psNode, DCPDP_DEVICE, sListNode);

		if (psDevicePriv->psDebugFSEntryDir)
		{
			if (psDevicePriv->psDisplayEnabledEntry)
			{
				debugfs_remove(psDevicePriv->psDisplayEnabledEntry);
				psDevicePriv->psDisplayEnabledEntry = NULL;
			}

			debugfs_remove(psDevicePriv->psDebugFSEntryDir);
			psDevicePriv->psDebugFSEntryDir = NULL;
		}

#if defined(PDP_DEBUG)
		printk(KERN_WARNING DRVNAME " - %s: DeInit'ing instance %u\n",
		       __func__, psDevicePriv->ui32Instance);
#endif
		dllist_remove_node(&psDevicePriv->sListNode);
		DCPDPDeInit(psDevicePriv, (void *)&psPCIDev);

		g_uiNumPDPDevs--;
	}

	/* Now clean up the top-level debugfs entries */
	if (g_psNumDevicesEntry)
	{
		debugfs_remove(g_psNumDevicesEntry);
		g_psNumDevicesEntry = NULL;
	}

	if (g_psDispEnabledEntry)
	{
		debugfs_remove(g_psDispEnabledEntry);
		g_psDispEnabledEntry = NULL;
	}

	if (g_psDebugFSEntryDir)
	{
		debugfs_remove(g_psDebugFSEntryDir);
		g_psDebugFSEntryDir = NULL;
	}

	if (!dllist_is_empty(&g_sDeviceDataListHead))
	{
		printk(KERN_WARNING DRVNAME " - %s: Still have devices initialised\n",
		       __func__);
	}
}

#if defined(PLATO_DISPLAY_PDUMP)

void PDP_OSWriteHWReg32(void *pvLinRegBaseAddr, IMG_UINT32 ui32Offset, IMG_UINT32 ui32Value)
{
	OSWriteHWReg32(pvLinRegBaseAddr, ui32Offset, ui32Value);
	if (pvLinRegBaseAddr == g_psDeviceData->pvPDPRegCpuVAddr)
	{
		plato_pdump_reg32(pvLinRegBaseAddr, ui32Offset, ui32Value, DRVNAME);
	}
	else if (pvLinRegBaseAddr == g_psDeviceData->pvPDPBifRegCpuVAddr)
	{
		plato_pdump_reg32(pvLinRegBaseAddr, ui32Offset, ui32Value, "pdp_bif");
	}
}
#endif

module_init(dc_pdp_init);
module_exit(dc_pdp_deinit);

#endif /* defined(__linux__) */

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
#include "dc_osfuncs.h"
#include "hdmi.h"

/* For MODULE_LICENSE */
#include "pvrmodule.h"

#define HDMI_DEBUGFS_DISPLAY_ENABLED "display_enabled"

/* Callbacks for PDP<->HDMI interface */
extern PVRSRV_ERROR PDPInitialize(VIDEO_PARAMS * pVideoParams, IMG_UINT32 uiInstance);

/* Helpers for getting DDR/GPU/PLL clock speed */
extern IMG_UINT32 SysGetPlatoMemClockSpeed(void);
extern IMG_UINT32 SysGetPlatoCoreClockSpeed(void);
extern IMG_UINT32 SysGetPlatoPLLClockSpeed(IMG_UINT32 ui32ClockSpeed);

static DLLIST_NODE g_sDeviceDataListHead;    /* List of discovered devices */
static IMG_UINT32  g_uiNumHDMIDevs;          /* Number of discovered devices */

static struct dentry	*g_psDebugFSEntryDir;/* debugFS top-level directory */

/* HDMI module parameters */
HDMI_MODULE_PARAMETERS sModuleParams =
{
	.ui32HDMIEnabled = 1,
};

module_param_named(mem_en, sModuleParams.ui32HDMIEnabled, uint, S_IRUGO | S_IWUSR);

static int HdmiDebugFsOpen(struct inode *psINode, struct file *psFile)
{
	psFile->private_data = psINode->i_private;

	return 0;
}

static ssize_t HdmiDebugFsRead(struct file *psFile,
				  char __user *psUserBuffer,
				  size_t uiCount,
				  loff_t *puiPosition)
{
	HDMI_DEVICE *psDevice = (HDMI_DEVICE *)psFile->private_data;
	IMG_BOOL bHDMIEnabled = psDevice->bHDMIEnabled;
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

	if (bHDMIEnabled == IMG_TRUE)
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

static ssize_t HdmiDebugFsWrite(struct file *psFile,
				   const char __user *psUserBuffer,
				   size_t uiCount,
				   loff_t *puiPosition)
{
	HDMI_DEVICE *psDevice = (HDMI_DEVICE *)psFile->private_data;
	char pszBuffer[3];
	bool bHDMIEnabled;
	size_t uiBufferSize = ARRAY_SIZE(pszBuffer);
	size_t uiLocCount;

	uiLocCount = min(uiCount, (uiBufferSize - 1U));

	if (copy_from_user(pszBuffer, psUserBuffer, uiLocCount) != 0)
	{
		return -EFAULT;
	}

	pszBuffer[uiLocCount] = '\0';

	if (kstrtobool(pszBuffer, &bHDMIEnabled) == 0)
	{
		psDevice->bHDMIEnabled = bHDMIEnabled ? IMG_TRUE : IMG_FALSE;
	}

	return (ssize_t)uiLocCount;
}

static const struct file_operations gsDisplayEnabledFileOps =
{
	.owner = THIS_MODULE,
	.open = HdmiDebugFsOpen,
	.read = HdmiDebugFsRead,
	.write = HdmiDebugFsWrite,
	.llseek = default_llseek,
};

static struct pci_dev * hdmi_getdevice(struct pci_dev *psFromDev)
{
	struct pci_dev *psPCIDev;

	psPCIDev = pci_get_device(SYS_RGX_DEV_VENDOR_ID,
	                          SYS_RGX_DEV_DEVICE_ID,
	                          psFromDev);
#if defined(HDMI_DEBUG)
	printk(KERN_INFO DRVNAME " - %s: %x, %x, device %p fromDev %p\n",
	       __func__,
	       SYS_RGX_DEV_VENDOR_ID, SYS_RGX_DEV_DEVICE_ID, psPCIDev, psFromDev);
#endif

	if (psPCIDev != psFromDev)
	{
		return psPCIDev;
	}
	else
	{
#if defined(HDMI_DEBUG)
		if (psPCIDev != NULL)
		{
			printk(KERN_WARNING DRVNAME " - %s: Duplicate entry - terminating scan.\n",
		         __func__);
		}
#endif
		return NULL;
	}
}

static int __init hdmi_init(void)
{
	struct pci_dev *psPCIDev, *fromDev = NULL;
	HDMI_DEVICE *psDeviceData;
	PVRSRV_ERROR eError;
	int error;
	IMG_CHAR aszDevName[4];    /* Location for unit name ("0" .. "N") */
	static IMG_BOOL bFirstTime = IMG_TRUE;

	#if !defined(HDMI_CONTROLLER)
	return 0;
	#endif

	/* Iterate over all PCI instances */
	dllist_init(&g_sDeviceDataListHead);
	while ((psPCIDev = hdmi_getdevice(fromDev)) != NULL)
	{
		fromDev = psPCIDev;
#if defined(HDMI_DEBUG)
		printk(KERN_INFO DRVNAME " - %s: Initializing unit %u, parent %p, dev_data %p\n",
		        __func__,
		       g_uiNumHDMIDevs, psPCIDev->dev.parent,
		       psPCIDev->dev.driver_data);
#endif

		error = pci_enable_device(psPCIDev);
		if (error != 0)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to enable PCI device (%d)\n", __func__, error);
			continue;	/* Continue scan and drop device reference */
		}

		eError = HDMIDrvInit(&psPCIDev->dev, &psDeviceData);
		if (eError != PVRSRV_OK)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to initialise device (%d)\n", __func__, eError);
			continue;	/* Continue scan and drop device reference */
		}
		dllist_init(&psDeviceData->sListNode);

		/* To prevent possible problems with system suspend/resume, we don't
		   keep the device enabled, but rely on the fact that the Rogue driver
		   will have done a pci_enable_device. */
		pci_disable_device(psPCIDev);

		/* Create top-level debugFS entries on first iteration only */
		if (bFirstTime == IMG_TRUE)
		{
			bFirstTime = IMG_FALSE;

			g_psDebugFSEntryDir = debugfs_create_dir(DRVNAME, NULL);
			if (IS_ERR_OR_NULL(g_psDebugFSEntryDir))
			{
				printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s' debugfs root directory "
				       "(debugfs entries won't be available)\n", __func__, DRVNAME);
				g_psDebugFSEntryDir = NULL;
			}

		}

		if (!IS_ERR_OR_NULL(g_psDebugFSEntryDir))
		{
			(void) DC_OSSNPrintf(aszDevName, sizeof(aszDevName), "%d", g_uiNumHDMIDevs);

			psDeviceData->psDebugFSEntryDir = debugfs_create_dir(aszDevName, g_psDebugFSEntryDir);
			if (IS_ERR_OR_NULL(psDeviceData->psDebugFSEntryDir))
			{
				printk(KERN_WARNING DRVNAME
				       " - %s: Failed to create '%s/%s' debugfs root directory "
				       "(debugfs entries won't be available)\n",
				       __func__, DRVNAME, aszDevName);
				psDeviceData->psDebugFSEntryDir = NULL;
			}
			psDeviceData->psDisplayEnabledEntry = debugfs_create_file(
			                         HDMI_DEBUGFS_DISPLAY_ENABLED,
			                         S_IFREG | S_IRUGO | S_IWUSR,
			                         psDeviceData->psDebugFSEntryDir,
			                         psDeviceData,
			                         &gsDisplayEnabledFileOps);
			if (IS_ERR_OR_NULL(psDeviceData->psDisplayEnabledEntry))
			{
				printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s/%s' debugfs entry\n",
				       __func__, aszDevName, HDMI_DEBUGFS_DISPLAY_ENABLED);
				psDeviceData->psDisplayEnabledEntry = NULL;
			}
		}

		/* Initialise the bHDMIEnabled value from the modparam setting */
		psDeviceData->bHDMIEnabled = (sModuleParams.ui32HDMIEnabled == 1U) ? IMG_TRUE : IMG_FALSE;

		psDeviceData->pfnPDPInitialize = PDPInitialize;

		psDeviceData->ui32CoreClockSpeed = SysGetPlatoCoreClockSpeed();
		psDeviceData->ui32PLLClockSpeed  = SysGetPlatoPLLClockSpeed(psDeviceData->ui32CoreClockSpeed);

		psDeviceData->ui32Instance = g_uiNumHDMIDevs++;
		dllist_add_to_tail(&g_sDeviceDataListHead, &psDeviceData->sListNode);
	}

#if defined(HDMI_DEBUG)
	printk(KERN_INFO DRVNAME " - %s: Discovered %u HDMI instances\n",
	       __func__, g_uiNumHDMIDevs);
#endif

	return (g_uiNumHDMIDevs > 0U) ? 0 : -ENODEV;
}

/*
 Clear all registered devices from the system
 */
static void __exit hdmi_deinit(void)
{

	DLLIST_NODE *psNode, *psNext;
	HDMI_DEVICE *psDeviceData;

#if defined(HDMI_DEBUG)
	printk(KERN_INFO DRVNAME " - %s: De-Init Entry %u Devices\n", __func__,
	       g_uiNumHDMIDevs);
#endif

	dllist_foreach_node(&g_sDeviceDataListHead, psNode, psNext)
	{
		psDeviceData = IMG_CONTAINER_OF(psNode, HDMI_DEVICE, sListNode);

		if (psDeviceData->psDisplayEnabledEntry)
		{
			debugfs_remove(psDeviceData->psDisplayEnabledEntry);
		}

#if defined(HDMI_DEBUG)
		printk(KERN_INFO DRVNAME " - %s: De-init unit %u\n", __func__,
		       psDeviceData->ui32Instance);
#endif

		if (psDeviceData->psDebugFSEntryDir)
		{
			debugfs_remove(psDeviceData->psDebugFSEntryDir);
			psDeviceData->psDebugFSEntryDir = NULL;
		}

		dllist_remove_node(&psDeviceData->sListNode);
		HDMIDrvDeInit(psDeviceData);
		g_uiNumHDMIDevs--;
	}

	if (g_psDebugFSEntryDir)
	{
		debugfs_remove(g_psDebugFSEntryDir);
		g_psDebugFSEntryDir = NULL;
	}
}

module_init(hdmi_init);
module_exit(hdmi_deinit);

#endif /* defined(__linux__) */

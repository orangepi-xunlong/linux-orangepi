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
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#if defined(DCPDP_REGISTER_DRIVER)
#include "pdp_common.h"
#include "pdp_regs.h"
#include "tc_drv.h"
#endif

/* For MODULE_LICENSE */
#include "pvrmodule.h"

#include "dc_pdp.h"
#include "img_defs.h"

#define DCPDP_WIDTH_MIN			(640)
#define DCPDP_WIDTH_MAX			(1280)
#define DCPDP_HEIGHT_MIN		(480)
#define DCPDP_HEIGHT_MAX		(1024)

#define DCPDP_DEBUGFS_DISPLAY_ENABLED	"display_enabled"

#if defined(DCPDP_WIDTH) && !defined(DCPDP_HEIGHT)
#error ERROR: DCPDP_WIDTH defined but DCPDP_HEIGHT not defined
#elif !defined(DCPDP_WIDTH) && defined(DCPDP_HEIGHT)
#error ERROR: DCPDP_HEIGHT defined but DCPDP_WIDTH not defined
#elif !defined(DCPDP_WIDTH) && !defined(DCPDP_HEIGHT)
#define DCPDP_WIDTH			DCPDP_WIDTH_MAX
#define DCPDP_HEIGHT			DCPDP_HEIGHT_MAX
#elif (DCPDP_WIDTH > DCPDP_WIDTH_MAX)
#error ERROR: DCPDP_WIDTH too large (max: 1280)
#elif (DCPDP_WIDTH < DCPDP_WIDTH_MIN)
#error ERROR: DCPDP_WIDTH too small (min: 640)
#elif (DCPDP_HEIGHT > DCPDP_HEIGHT_MAX)
#error ERROR: DCPDP_HEIGHT too large (max: 1024)
#elif (DCPDP_HEIGHT < DCPDP_HEIGHT_MIN)
#error ERROR: DCPDP_HEIGHT too small (max: 480)
#endif

struct DCPDP_DEVICE_PRIV_TAG
{
	struct device		*psDev;
	DCPDP_DEVICE		*psPDPDevice;

	IMG_HANDLE		hServicesConnection;
	DC_SERVICES_FUNCS	sServicesFuncs;

	IMG_CPU_PHYADDR		sPDPRegCpuPAddr;
	IMG_UINT32		ui32PDPRegSize;
	IMG_CPU_PHYADDR		sPLLRegCpuPAddr;
	IMG_UINT32		ui32PLLRegSize;

	struct dentry		*psDebugFSEntryDir;
	struct dentry		*psDisplayEnabledEntry;

#if defined(DCPDP_REGISTER_DRIVER)
	PFN_LISR		pfnLISR;
	void			*pvLISRData;
#else
	IMG_HANDLE		hLISRData;
#endif
};

#if !defined(DCPDP_REGISTER_DRIVER)
static DCPDP_DEVICE_PRIV *g_psDevicePriv;
#endif

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

IMG_CPU_PHYADDR DCPDPGetAddrRangeStart(DCPDP_DEVICE_PRIV *psDevicePriv,
				       DCPDP_ADDRESS_RANGE eRange)
{
	switch (eRange)
	{
		default:
			DC_ASSERT(!"Unsupported address range");
			__fallthrough;
		case DCPDP_ADDRESS_RANGE_PDP:
			return psDevicePriv->sPDPRegCpuPAddr;
		case DCPDP_ADDRESS_RANGE_PLL:
			return psDevicePriv->sPLLRegCpuPAddr;
	}
}

IMG_UINT32 DCPDPGetAddrRangeSize(DCPDP_DEVICE_PRIV *psDevicePriv,
				 DCPDP_ADDRESS_RANGE eRange)
{
	switch (eRange)
	{
		default:
			DC_ASSERT(!"Unsupported address range");
			__fallthrough;
		case DCPDP_ADDRESS_RANGE_PDP:
			return psDevicePriv->ui32PDPRegSize;
		case DCPDP_ADDRESS_RANGE_PLL:
			return psDevicePriv->ui32PLLRegSize;
	}
}

#if defined(DCPDP_REGISTER_DRIVER)
static void DCPDPLISRHandlerWrapper(void *pvData)
{
	DCPDP_DEVICE_PRIV *psDevicePriv = pvData;

	if (psDevicePriv->pfnLISR != NULL)
	{
		(void) psDevicePriv->pfnLISR(psDevicePriv->pvLISRData);
	}
}
#endif

PVRSRV_ERROR DCPDPInstallDeviceLISR(DCPDP_DEVICE_PRIV *psDevicePriv,
				    PFN_LISR pfnLISR, void *pvData)
{
#if defined(DCPDP_REGISTER_DRIVER)
	struct device *psParentDev = psDevicePriv->psDev->parent;
	int iErr;

	if (psDevicePriv->pfnLISR != NULL)
	{
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}

	psDevicePriv->pfnLISR = pfnLISR;
	psDevicePriv->pvLISRData = pvData;

	iErr = tc_set_interrupt_handler(psParentDev,
					TC_INTERRUPT_PDP,
					DCPDPLISRHandlerWrapper,
					psDevicePriv);
	if (iErr)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to install interrupt handler (err=%d)\n",
		       __func__, iErr);
		return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
	}

	iErr = tc_enable_interrupt(psParentDev, TC_INTERRUPT_PDP);
	if (iErr)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to enable interrupts (err=%d)\n",
		       __func__, iErr);
		tc_set_interrupt_handler(psParentDev, TC_INTERRUPT_PDP, NULL, NULL);
		psDevicePriv->pfnLISR = NULL;
		psDevicePriv->pvLISRData = NULL;
		return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
	}

	return PVRSRV_OK;
#else
	if (psDevicePriv->hLISRData != NULL)
	{
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}

	return psDevicePriv->sServicesFuncs.pfnSysInstallDeviceLISR(psDevicePriv->psDev,
								    DCPDP_INTERRUPT_ID,
								    DRVNAME,
								    pfnLISR,
								    pvData,
								    &psDevicePriv->hLISRData);
#endif
}

PVRSRV_ERROR DCPDPUninstallDeviceLISR(DCPDP_DEVICE_PRIV *psDevicePriv)
{
#if defined(DCPDP_REGISTER_DRIVER)
	struct device *psParentDev = psDevicePriv->psDev->parent;
	int iErr;

	if (psDevicePriv->pfnLISR == NULL)
	{
		return PVRSRV_ERROR_ISR_NOT_INSTALLED;
	}

	tc_disable_interrupt(psParentDev, TC_INTERRUPT_PDP);

	iErr = tc_set_interrupt_handler(psParentDev, TC_INTERRUPT_PDP, NULL, NULL);
	if (iErr)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to uninstall interrupt handler (err=%d)\n",
		       __func__, iErr);
		return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
	}

	psDevicePriv->pfnLISR = NULL;
	psDevicePriv->pvLISRData = NULL;

	return PVRSRV_OK;
#else
	PVRSRV_ERROR eError;

	if (psDevicePriv->hLISRData == NULL)
	{
		return PVRSRV_ERROR_ISR_NOT_INSTALLED;
	}

	eError = PVRSRVSystemUninstallDeviceLISR(psDevicePriv->hLISRData);
	if (eError == PVRSRV_OK)
	{
		psDevicePriv->hLISRData = NULL;
	}

	return eError;
#endif
}

static int DisplayEnabledOpen(struct inode *psINode, struct file *psFile)
{
	psFile->private_data = psINode->i_private;

	return 0;
}

static ssize_t DisplayEnabledRead(struct file *psFile,
				  char __user *psUserBuffer,
				  size_t uiCount,
				  loff_t *puiPosition)
{
	loff_t uiPosition = *puiPosition;
	char pszBuffer[] = "N\n";
	size_t uiBufferSize = ARRAY_SIZE(pszBuffer);
	int iErr;

	if (uiPosition < 0)
	{
		return -EINVAL;
	}
	else if (uiPosition >= uiBufferSize || uiCount == 0)
	{
		return 0;
	}

	if (sModuleParams.ui32PDPEnabled)
	{
		pszBuffer[0] = 'Y';
	}

	if (uiCount > uiBufferSize - uiPosition)
	{
		uiCount = uiBufferSize - uiPosition;
	}

	iErr = copy_to_user(psUserBuffer, &pszBuffer[uiPosition], uiCount);
	if (iErr)
	{
		return -EFAULT;
	}

	*puiPosition = uiPosition + uiCount;

	return uiCount;
}

static ssize_t DisplayEnabledWrite(struct file *psFile,
				   const char __user *psUserBuffer,
				   size_t uiCount,
				   loff_t *puiPosition)
{
	DCPDP_DEVICE_PRIV *psDevicePriv = psFile->private_data;
	char pszBuffer[3];
	bool bPDPEnabled;
	int iErr;

	uiCount = min(uiCount, ARRAY_SIZE(pszBuffer) - 1);

	iErr = copy_from_user(pszBuffer, psUserBuffer, uiCount);
	if (iErr)
	{
		return -EFAULT;
	}

	pszBuffer[uiCount] = '\0';

	if (kstrtobool(pszBuffer, &bPDPEnabled) == 0)
	{
		sModuleParams.ui32PDPEnabled = bPDPEnabled ? 1 : 0;

		DCPDPEnableMemoryRequest(psDevicePriv->psPDPDevice, bPDPEnabled);
	}

	return uiCount;
}

static const struct file_operations gsDisplayEnabledFileOps =
{
	.owner = THIS_MODULE,
	.open = DisplayEnabledOpen,
	.read = DisplayEnabledRead,
	.write = DisplayEnabledWrite,
	.llseek = default_llseek,
};

static void DCPDPDebugFSInit(DCPDP_DEVICE_PRIV *psDevicePriv)
{
	psDevicePriv->psDebugFSEntryDir = debugfs_create_dir(DRVNAME, NULL);
	if (IS_ERR_OR_NULL(psDevicePriv->psDebugFSEntryDir))
	{
		printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s' debugfs root directory "
		       "(debugfs entries won't be available)\n", __func__, DRVNAME);
		psDevicePriv->psDebugFSEntryDir = NULL;
		return;
	}

	psDevicePriv->psDisplayEnabledEntry =
		debugfs_create_file(DCPDP_DEBUGFS_DISPLAY_ENABLED,
				    S_IFREG | S_IRUGO | S_IWUSR,
				    psDevicePriv->psDebugFSEntryDir,
				    psDevicePriv,
				    &gsDisplayEnabledFileOps);
	if (IS_ERR_OR_NULL(psDevicePriv->psDisplayEnabledEntry))
	{
		printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s' debugfs entry\n",
		       __func__, DCPDP_DEBUGFS_DISPLAY_ENABLED);
		psDevicePriv->psDisplayEnabledEntry = NULL;
		return;
	}
}

static void DCPDPDebugFSDeInit(DCPDP_DEVICE_PRIV *psDevicePriv)
{
	if (psDevicePriv->psDisplayEnabledEntry != NULL)
	{
		debugfs_remove(psDevicePriv->psDisplayEnabledEntry);
		psDevicePriv->psDisplayEnabledEntry = NULL;
	}

	if (psDevicePriv->psDebugFSEntryDir != NULL)
	{
		debugfs_remove(psDevicePriv->psDebugFSEntryDir);
		psDevicePriv->psDebugFSEntryDir = NULL;
	}
}


static PVRSRV_ERROR DCPDPServicesInit(DCPDP_DEVICE_PRIV *psDevicePriv)
{
	PVRSRV_ERROR eError;

	eError = DC_OSPVRServicesConnectionOpen(&psDevicePriv->hServicesConnection);
	if (eError != PVRSRV_OK)
	{
		printk(KERN_WARNING DRVNAME " - %s: Failed to open connection to PVR Services (%d)\n",
		       __func__, eError);
		return eError;
	}

	eError = DC_OSPVRServicesSetupFuncs(psDevicePriv->hServicesConnection,
					    &psDevicePriv->sServicesFuncs);
	if (eError != PVRSRV_OK)
	{
		printk(KERN_WARNING DRVNAME " - %s: Failed to setup PVR Services function table (%d)\n",
		       __func__, eError);
		goto ErrorServicesConnectionClose;
	}

	return PVRSRV_OK;

ErrorServicesConnectionClose:
	DC_OSPVRServicesConnectionClose(psDevicePriv->hServicesConnection);
	psDevicePriv->hServicesConnection = NULL;

	return eError;
}

static void DCPDPServicesDeInit(DCPDP_DEVICE_PRIV *psDevicePriv)
{
	DC_OSMemSet(&psDevicePriv->sServicesFuncs, 0,
		    sizeof(psDevicePriv->sServicesFuncs));

	DC_OSPVRServicesConnectionClose(psDevicePriv->hServicesConnection);
	psDevicePriv->hServicesConnection = NULL;
}

static int DCPDPPCIDevicePrivInit(DCPDP_DEVICE_PRIV *psDevicePriv,
				  struct pci_dev *psPCIDev)
{
	IMG_UINT32 ui32RegBaseAddr = DC_OSAddrRangeStart(&psPCIDev->dev,
							 DCPDP_REG_PCI_BASENUM);

	psDevicePriv->sPDPRegCpuPAddr.uiAddr = ui32RegBaseAddr + DCPDP_PCI_PDP_REG_OFFSET;
	psDevicePriv->ui32PDPRegSize = DCPDP_PCI_PDP_REG_SIZE;

	psDevicePriv->sPLLRegCpuPAddr.uiAddr = ui32RegBaseAddr + DCPDP_PCI_PLL_REG_OFFSET;
	psDevicePriv->ui32PLLRegSize = DCPDP_PCI_PLL_REG_SIZE;

	return 0;
}

static int DCPDPPlatformDevicePrivInit(DCPDP_DEVICE_PRIV *psDevicePriv,
				       struct platform_device *psPlatDev)
{
	struct resource *psRegs;

	psRegs = platform_get_resource_byname(psPlatDev, IORESOURCE_MEM, "pdp-regs");
	if (psRegs == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to get PDP registers\n",
			   __func__);
		return -ENXIO;
	}

	psDevicePriv->sPDPRegCpuPAddr.uiAddr =
		IMG_CAST_TO_CPUPHYADDR_UINT(psRegs->start);
	psDevicePriv->ui32PDPRegSize = (IMG_UINT32) resource_size(psRegs);

	psRegs = platform_get_resource_byname(psPlatDev, IORESOURCE_MEM, "pll-regs");
	if (psRegs == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to get PLL registers\n",
			   __func__);
		return -ENXIO;
	}

	psDevicePriv->sPLLRegCpuPAddr.uiAddr =
		IMG_CAST_TO_CPUPHYADDR_UINT(psRegs->start);
	psDevicePriv->ui32PLLRegSize = (IMG_UINT32) resource_size(psRegs);

	return 0;
}

static DCPDP_DEVICE_PRIV *DCPDPDevicePrivCreate(struct device *psDev)
{
	DCPDP_DEVICE_PRIV *psDevicePriv;
	PVRSRV_ERROR eError;
	int iRet;

	psDevicePriv = DC_OSCallocMem(sizeof(*psDevicePriv));
	if (psDevicePriv == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to allocate device private data\n",
			   __func__);
		iRet = -ENOMEM;
		goto ErrorReturn;
	}

	psDevicePriv->psDev = psDev;

	eError = DCPDPServicesInit(psDevicePriv);
	if (eError != PVRSRV_OK)
	{
		iRet = -ENODEV;
		goto ErrorFreeDevicePriv;
	}

	if (dev_is_pci(psDev))
	{
		iRet = DCPDPPCIDevicePrivInit(psDevicePriv, to_pci_dev(psDev));
	}
	else
	{
		iRet = DCPDPPlatformDevicePrivInit(psDevicePriv,
						   to_platform_device(psDev));
	}

	if (iRet != 0)
	{
		goto ErrorServicesDeInit;
	}

	eError = DCPDPInit(psDevicePriv,
			   &psDevicePriv->sServicesFuncs,
			   &psDevicePriv->psPDPDevice);
	if (eError != PVRSRV_OK)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to initialise device (%d)\n",
			   __func__, eError);
		iRet = -ENODEV;
		goto ErrorServicesDeInit;
	}

	DCPDPDebugFSInit(psDevicePriv);

	return psDevicePriv;

ErrorServicesDeInit:
	DCPDPServicesDeInit(psDevicePriv);
ErrorFreeDevicePriv:
	DC_OSFreeMem(psDevicePriv);
ErrorReturn:
	return ERR_PTR(iRet);
}

static void DCPDPDevicePrivDestroy(DCPDP_DEVICE_PRIV *psDevicePriv)
{
	DCPDPDebugFSDeInit(psDevicePriv);
	DCPDPDeInit(psDevicePriv->psPDPDevice, NULL);
	DCPDPServicesDeInit(psDevicePriv);
	DC_OSFreeMem(psDevicePriv);
}

#if defined(DCPDP_REGISTER_DRIVER)

static int DCPDPProbe(struct platform_device *psPlatDev)
{
	DCPDP_DEVICE_PRIV *psDevicePriv;
	int iRet;

	iRet = tc_enable(psPlatDev->dev.parent);
	if (iRet)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to enable device (err=%d)\n",
			   __func__, iRet);
		return iRet;
	}

	psDevicePriv = DCPDPDevicePrivCreate(&psPlatDev->dev);
	if (IS_ERR(psDevicePriv))
	{
		tc_disable(psPlatDev->dev.parent);
		return PTR_ERR(psDevicePriv);
	}

	platform_set_drvdata(psPlatDev, psDevicePriv);

	return 0;
}

static int DCPDPRemove(struct platform_device *psPlatDev)
{
	DCPDP_DEVICE_PRIV *psDevicePriv;

	psDevicePriv = platform_get_drvdata(psPlatDev);
	if (WARN_ON(psDevicePriv == NULL))
	{
		return -ENODEV;
	}

	platform_set_drvdata(psPlatDev, NULL);

	DCPDPDevicePrivDestroy(psDevicePriv);
	tc_disable(psPlatDev->dev.parent);

	return 0;
}

static void DCPDPShutdown(struct platform_device *psPlatDev)
{
	PVR_UNREFERENCED_PARAMETER(psPlatDev);
}

static struct platform_device_id DCPDPPlatformDeviceIDTable[] =
{
	{ .name = APOLLO_DEVICE_NAME_PDP, .driver_data = PDP_VERSION_APOLLO },
	{ },
};

static struct platform_driver DCPDPPlatformDriver =
{
	.probe		= DCPDPProbe,
	.remove		= DCPDPRemove,
	.shutdown	= DCPDPShutdown,
	.driver		=
	{
		.owner  = THIS_MODULE,
		.name	= DRVNAME,
	},
	.id_table	= DCPDPPlatformDeviceIDTable,
};

module_platform_driver(DCPDPPlatformDriver);

#else /* !defined(DCPDP_REGISTER_DRIVER) */

static int __init dc_pdp_init(void)
{
	DCPDP_DEVICE_PRIV *psDevicePriv;
	struct pci_dev *psPCIDev;
	int error;

	psPCIDev = pci_get_device(DCPDP_VENDOR_ID_POWERVR, DCPDP_DEVICE_ID_PCI_APOLLO_FPGA, NULL);
	if (psPCIDev == NULL)
	{
		psPCIDev = pci_get_device(DCPDP_VENDOR_ID_POWERVR, DCPDP_DEVICE_ID_PCIE_APOLLO_FPGA, NULL);
		if (psPCIDev == NULL)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to get PCI device\n", __func__);
			return -ENODEV;
		}
	}

	error = pci_enable_device(psPCIDev);
	if (error != 0)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to enable PCI device (%d)\n", __func__, error);
		return -ENODEV;
	}

	psDevicePriv = DCPDPDevicePrivCreate(&psPCIDev->dev);

	/* To prevent possible problems with system suspend/resume, we don't
	   keep the device enabled, but rely on the fact that the Rogue driver
	   will have done a pci_enable_device. */
	pci_disable_device(psPCIDev);

	if (IS_ERR(psDevicePriv))
	{
		return PTR_ERR(psDevicePriv);
	}

	g_psDevicePriv = psDevicePriv;

	return 0;
}

static void __exit dc_pdp_deinit(void)
{
	if (g_psDevicePriv)
	{
		DCPDPDevicePrivDestroy(g_psDevicePriv);
		g_psDevicePriv = NULL;
	}
}

module_init(dc_pdp_init);
module_exit(dc_pdp_deinit);

#endif /* defined(DCPDP_REGISTER_DRIVER) */

#endif /* defined(__linux__) */

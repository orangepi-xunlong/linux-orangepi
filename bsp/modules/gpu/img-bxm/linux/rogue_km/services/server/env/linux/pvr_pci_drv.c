/*
 * @File
 * @Title       PowerVR DRM PCI driver
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
#include <drm/drm_drv.h>
#include <drm/drm_print.h>
#else
#include <drm/drmP.h>
#endif

#include <linux/module.h>
#include <linux/pci.h>

#include "module_common.h"
#include "pvr_drv.h"
#include "pvrmodule.h"
#include "sysinfo.h"

/* This header must always be included last */
#include "kernel_compatibility.h"

MODULE_IMPORT_NS(DMA_BUF);

static struct drm_driver pvr_drm_pci_driver;

static int pvr_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct drm_device *ddev;
	int ret;

	DRM_DEBUG_DRIVER("device %p\n", &pdev->dev);

	ddev = drm_dev_alloc(&pvr_drm_pci_driver, &pdev->dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	if (IS_ERR(ddev))
		return PTR_ERR(ddev);
#else
	if (!ddev)
		return -ENOMEM;
#endif

	ret = pci_enable_device(pdev);
	if (ret)
		goto err_drm_dev_put;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0))
	ddev->pdev = pdev;
#endif

	/*
	 * drm_get_pci_dev calls sets the drvdata at this point, to ddev.
	 * We set the drvdata in the load callback, so there is no need
	 * to do it again here. The platform driver equivalent of
	 * drm_get_pci_dev, drm_platform_init, doesn't set the drvdata,
	 * which is why it is done in the load callback.
	 *
	 * The load callback, called from drm_dev_register, is deprecated,
	 * because of potential race conditions. Calling the function here,
	 * before calling drm_dev_register, avoids those potential races.
	 */
	BUG_ON(pvr_drm_pci_driver.load != NULL);
	ret = pvr_drm_load(ddev, 0);
	if (ret)
		goto err_pci_dev_disable;

	ret = drm_dev_register(ddev, 0);
	if (ret)
		goto err_drm_dev_unload;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	DRM_INFO("Initialized %s %d.%d.%d %s for %s on minor %d\n",
		pvr_drm_pci_driver.name,
		pvr_drm_pci_driver.major,
		pvr_drm_pci_driver.minor,
		pvr_drm_pci_driver.patchlevel,
		pvr_drm_pci_driver.date,
		pci_name(pdev),
		ddev->primary->index);
#endif
	return 0;

err_drm_dev_unload:
	pvr_drm_unload(ddev);
err_pci_dev_disable:
	pci_disable_device(pdev);
err_drm_dev_put:
	drm_dev_put(ddev);
	return ret;
}

static void pvr_remove(struct pci_dev *pdev)
{
	struct drm_device *ddev = pci_get_drvdata(pdev);

	DRM_DEBUG_DRIVER("device %p\n", &pdev->dev);

	drm_dev_unregister(ddev);

	/* The unload callback, called from drm_dev_unregister, is
	 * deprecated. Call the unload function directly.
	 */
	BUG_ON(pvr_drm_pci_driver.unload != NULL);
	pvr_drm_unload(ddev);

	pci_disable_device(pdev);

	drm_dev_put(ddev);
}

static void pvr_shutdown(struct pci_dev *pdev)
{
	struct drm_device *ddev = pci_get_drvdata(pdev);

	DRM_DEBUG_DRIVER("device %p\n", &pdev->dev);

	PVRSRVDeviceShutdown(ddev);
}

static const struct pci_device_id pvr_pci_ids[] = {
	{ PCI_DEVICE(SYS_RGX_DEV_VENDOR_ID, SYS_RGX_DEV_DEVICE_ID) },
#if defined(SYS_RGX_DEV1_DEVICE_ID)
	{ PCI_DEVICE(SYS_RGX_DEV_VENDOR_ID, SYS_RGX_DEV1_DEVICE_ID) },
#endif
#if defined(SYS_RGX_DEV_FROST_VENDOR_ID)
	{ PCI_DEVICE(SYS_RGX_DEV_FROST_VENDOR_ID, SYS_RGX_DEV_FROST_DEVICE_ID) },
#endif
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, pvr_pci_ids);

static struct pci_driver pvr_pci_driver = {
	.name		= DRVNAME,
	.driver.pm	= &pvr_pm_ops,
	.id_table	= pvr_pci_ids,
	.probe		= pvr_probe,
	.remove		= pvr_remove,
	.shutdown	= pvr_shutdown,
};

static int __init pvr_init(void)
{
	int err;

	DRM_DEBUG_DRIVER("\n");

	pvr_drm_pci_driver = pvr_drm_generic_driver;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
	pvr_drm_pci_driver.set_busid = drm_pci_set_busid;
#endif

	err = PVRSRVDriverInit();
	if (err)
		return err;

	return pci_register_driver(&pvr_pci_driver);
}

static void __exit pvr_exit(void)
{
	DRM_DEBUG_DRIVER("\n");

	pci_unregister_driver(&pvr_pci_driver);
	PVRSRVDriverDeinit();

	DRM_DEBUG_DRIVER("done\n");
}

module_init(pvr_init);
module_exit(pvr_exit);

// SPDX-License-Identifier: GPL-2.0+

#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/iopoll.h>

#include "../host/xhci-plat.h"
#include "core.h"
#include "gadget-export.h"
#include "drd.h"

static int set_phy_power_on(struct cdns *cdns)
{
	int ret;

	ret = phy_power_on(cdns->usb2_phy);
	if (ret)
		return ret;

	ret = phy_power_on(cdns->usb3_phy);
	if (ret)
		phy_power_off(cdns->usb2_phy);

	return ret;
}

static void set_phy_power_off(struct cdns *cdns)
{
	phy_power_off(cdns->usb3_phy);
	phy_power_off(cdns->usb2_phy);
}

static void devm_phy_release(struct device *dev, void *res)
{
	struct phy *phy = *(struct phy **)res;

	phy_put(dev, phy);
}

struct phy *devm_phy_optional_ref_get(struct device *dev,
			const char *string)
{
	struct phy **ptr, *phy;
	struct fwnode_handle *fwnode;
	struct device *rdev;

	fwnode = fwnode_find_reference(dev_fwnode(dev), string, 0);
	if (IS_ERR_OR_NULL(fwnode))
		return NULL;

	ptr = devres_alloc(devm_phy_release, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	rdev = get_dev_from_fwnode(fwnode_get_parent(fwnode));
	if (IS_ERR_OR_NULL(rdev))
		return NULL;
	phy = phy_get(rdev, fwnode_get_name(fwnode));
	if (!IS_ERR_OR_NULL(phy)) {
		*ptr = phy;
		devres_add(dev, ptr);
	} else {
		devres_free(ptr);
	}
	put_device(rdev);

	return phy;
}

static int cdnsp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource	*res;
	struct cdns *cdns;
	void __iomem *regs;
	int ret;

	cdns = devm_kzalloc(dev, sizeof(*cdns), GFP_KERNEL);
	if (!cdns)
		return -ENOMEM;

	cdns->dev = dev;
	cdns->pdata = dev_get_platdata(dev);

	platform_set_drvdata(pdev, cdns);

	ret = platform_get_irq_byname(pdev, "host");
	if (ret < 0) {
		dev_err(dev, "couldn't get host irq\n");
		return ret;
	}

	cdns->xhci_res[0].start = ret;
	cdns->xhci_res[0].end = ret;
	cdns->xhci_res[0].flags = IORESOURCE_IRQ | irq_get_trigger_type(ret);
	cdns->xhci_res[0].name = "host";

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "xhci");
	if (!res) {
		dev_err(dev, "couldn't get xhci resource\n");
		return -ENXIO;
	}

	cdns->xhci_res[1] = *res;

	cdns->dev_irq = platform_get_irq_byname(pdev, "peripheral");

	if (cdns->dev_irq < 0)
		return cdns->dev_irq;

	regs = devm_platform_ioremap_resource_byname(pdev, "dev");
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	cdns->dev_regs	= regs;

	cdns->otg_irq = platform_get_irq_byname(pdev, "otg");
	if (cdns->otg_irq < 0)
		return cdns->otg_irq;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "otg");
	if (!res) {
		dev_err(dev, "couldn't get otg resource\n");
		return -ENXIO;
	}

	cdns->otg_res = *res;

	cdns->wakeup_irq = platform_get_irq_byname_optional(pdev, "wakeup");
	if (cdns->wakeup_irq == -EPROBE_DEFER)
		return cdns->wakeup_irq;

	if (cdns->wakeup_irq < 0) {
		dev_dbg(dev, "couldn't get wakeup irq\n");
		cdns->wakeup_irq = 0x0;
	}
	cdns->usb2_phy = devm_phy_optional_get(dev, "cdnsp,usb2-phy");
	if (IS_ERR(cdns->usb2_phy)) {
		dev_err(dev, "couldn't get usb2_phy\n");
		return PTR_ERR(cdns->usb2_phy);
	}

	ret = phy_init(cdns->usb2_phy);
	if (ret)
		return ret;

	cdns->usb3_phy = devm_phy_optional_get(dev, "cdnsp,usb3-phy");
	if (IS_ERR_OR_NULL(cdns->usb3_phy))
		cdns->usb3_phy =
			devm_phy_optional_ref_get(dev, "cdnsp,usb3-phy");
	if (IS_ERR(cdns->usb3_phy))
		return PTR_ERR(cdns->usb3_phy);

	ret = phy_init(cdns->usb3_phy);
	if (ret)
		goto err_phy3_init;

	ret = set_phy_power_on(cdns);

	if (ret)
		goto err_phy_power_on;

	cdns->gadget_init = cdnsp_gadget_init;

	ret = cdns_init(cdns);

	if (ret)
		goto err_cdns_init;

	device_set_wakeup_capable(dev, true);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	if (!(cdns->pdata && (cdns->pdata->quirks & CDNS3_DEFAULT_PM_RUNTIME_ALLOW)))
		pm_runtime_forbid(dev);

	/*
	 * The controller needs less time between bus and controller suspend,
	 * and we also needs a small delay to avoid frequently entering low
	 * power mode.
	 */
	pm_runtime_set_autosuspend_delay(dev, 20);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_use_autosuspend(dev);

	return 0;

err_cdns_init:
	set_phy_power_off(cdns);
err_phy_power_on:
	phy_exit(cdns->usb3_phy);
err_phy3_init:
	phy_exit(cdns->usb2_phy);

	return ret;
}

static int cdnsp_remove(struct platform_device *pdev)
{
	struct cdns *cdns = platform_get_drvdata(pdev);
	struct device *dev = cdns->dev;

	pm_runtime_get_sync(dev);
	pm_runtime_disable(dev);
	pm_runtime_put_noidle(dev);
	cdns_remove(cdns);
	set_phy_power_off(cdns);
	phy_exit(cdns->usb2_phy);
	phy_exit(cdns->usb3_phy);
	return 0;
}
#ifdef CONFIG_PM

static int cdnsp_set_platform_suspend(struct device *dev,
				      bool suspend, bool wakeup)
{
	struct cdns *cdns = dev_get_drvdata(dev);
	int ret = 0;

	if (cdns->pdata && cdns->pdata->platform_suspend)
		ret = cdns->pdata->platform_suspend(dev, suspend, wakeup);

	return ret;
}

static int cdnsp_controller_suspend(struct device *dev, pm_message_t msg)
{
	struct cdns *cdns = dev_get_drvdata(dev);
	bool wakeup;
	unsigned long flags;

	if (cdns->in_lpm)
		return 0;

	if (PMSG_IS_AUTO(msg))
		wakeup = true;
	else
		wakeup = device_may_wakeup(dev);

	cdnsp_set_platform_suspend(cdns->dev, true, wakeup);
	set_phy_power_off(cdns);
	spin_lock_irqsave(&cdns->lock, flags);
	cdns->in_lpm = true;
	spin_unlock_irqrestore(&cdns->lock, flags);
	dev_dbg(cdns->dev, "%s ends\n", __func__);

	return 0;
}

static int cdnsp_controller_resume(struct device *dev, pm_message_t msg)
{
	struct cdns *cdns = dev_get_drvdata(dev);
	int ret;
	unsigned long flags;

	if (!cdns->in_lpm)
		return 0;

	ret = set_phy_power_on(cdns);
	if (ret)
		return ret;

	cdnsp_set_platform_suspend(cdns->dev, false, false);

	spin_lock_irqsave(&cdns->lock, flags);
	cdns_resume(cdns, !PMSG_IS_AUTO(msg));
	cdns->in_lpm = false;
	spin_unlock_irqrestore(&cdns->lock, flags);

	return ret;
}

static int cdnsp_plat_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct cdns *cdns = dev_get_drvdata(dev);
	struct platform_device *xhci_dev;
	struct usb_hcd  *hcd;
	struct xhci_hcd *xhci;
	u32	command = 0;
	u32	result;

	if (cdns->role == USB_ROLE_HOST) {
		xhci_dev = cdns->xhci_device;
		hcd = dev_get_drvdata(&xhci_dev->dev);
		xhci = hcd_to_xhci(hcd);

		/* XHCI irq and Wakeup irq are the same interrupt,set Run/Stop bit,
		 * Otherwise, can not receive interrupt after entering runtime suspend.
		 */
		command = readl(&xhci->op_regs->command);
		command |= CMD_RUN;
		writel(command, &xhci->op_regs->command);
		readl_poll_timeout_atomic(&xhci->op_regs->status, result,
			(result & STS_HALT) == 0 || result == U32_MAX ,
			1, 250 * 1000);
		if (result == U32_MAX) {
			dev_err(dev, "set controller run timeout\n");
		}
	}
	ret = cdnsp_controller_suspend(dev, PMSG_AUTO_SUSPEND);

	return ret;
}

static int cdnsp_plat_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct cdns *cdns = dev_get_drvdata(dev);
	struct platform_device *xhci_dev;
	struct usb_hcd  *hcd;
	struct xhci_hcd *xhci;

	ret = cdnsp_controller_resume(dev, PMSG_AUTO_RESUME);

	if (cdns->role == USB_ROLE_HOST) {
		xhci_dev = cdns->xhci_device;
		hcd = dev_get_drvdata(&xhci_dev->dev);
		xhci = hcd_to_xhci(hcd);
		set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
		if (xhci->shared_hcd)
			set_bit(HCD_FLAG_HW_ACCESSIBLE, &xhci->shared_hcd->flags);
		if (cdns->wakeup_pending) {
			enable_irq(cdns->wakeup_irq);
			cdns->wakeup_pending = false;
		}
	}
	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int cdnsp_plat_suspend(struct device *dev)
{
	struct cdns *cdns = dev_get_drvdata(dev);
	int ret;

	cdns_suspend(cdns);

	ret = cdnsp_controller_suspend(dev, PMSG_SUSPEND);
	if (ret)
		return ret;

	if (cdns->role == USB_ROLE_HOST) {
		if (device_may_wakeup(dev) && cdns->wakeup_irq) {
			disable_irq(cdns->wakeup_irq);
			enable_irq_wake(cdns->wakeup_irq);
			dev_info(cdns->dev, "dis irq,enable wake\n");
		}
	}

	return ret;
}

static void cdnsp_cdnsp_plat_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cdns *cdns = dev_get_drvdata(dev);
	int ret;
	mutex_lock(&cdns->role_mutex);
	cdns_suspend(cdns);

	/* suspend the usb controller and config it to D3. */
	ret = cdnsp_controller_suspend(dev, PMSG_SUSPEND);
	if (ret)
		return;
	cdns->d3 = true;
	mutex_unlock(&cdns->role_mutex);

	if (!device_may_wakeup(dev)) {
		phy_exit(cdns->usb3_phy);
		phy_exit(cdns->usb2_phy);
	}
	/* if the user configures the usb device as a wake-up device, then enable_irq_wake
	 *  will enable the corresponding bit for USB to wake up the system.
	 */
	if (cdns->role == USB_ROLE_HOST) {
		if (device_may_wakeup(dev) && cdns->wakeup_irq) {
			disable_irq(cdns->wakeup_irq);
			enable_irq_wake(cdns->wakeup_irq);
			dev_info(cdns->dev, "dis irq,enable wake\n");
		}
	}
}

static int cdnsp_plat_resume(struct device *dev)
{
	int ret;
	struct cdns *cdns = dev_get_drvdata(dev);

	ret = cdnsp_controller_resume(dev, PMSG_RESUME);
	if (cdns->role == USB_ROLE_HOST) {
		if (device_may_wakeup(dev) && cdns->wakeup_irq) {
			disable_irq_wake(cdns->wakeup_irq);
			enable_irq(cdns->wakeup_irq);
			dev_info(cdns->dev, "dis wake,enable irq\n");
		}
	}

	return ret;
}

static int cdnsp_plat_restore(struct device *dev)
{
	struct cdns *cdns = dev_get_drvdata(dev);
	int ret = 0;

	dev_dbg(dev, "at func %s\n", __func__);

	/* restore for both host and device */
	if (cdns->roles[cdns->role]->restore)
		ret = cdns->roles[cdns->role]->restore(cdns);

	return ret;
}

static int cdnsp_plat_freeze(struct device *dev)
{
	dev_dbg(dev, "at func %s\n", __func__);
	return 0;
}

static int cdnsp_plat_thaw(struct device *dev)
{
	dev_dbg(dev, "at func %s\n", __func__);
	return 0;
}

#endif /* CONFIG_PM_SLEEP */
#endif /* CONFIG_PM */

static const struct dev_pm_ops cdnsp_pm_ops = {
        .suspend = cdnsp_plat_suspend,
        .resume = cdnsp_plat_resume,
        .restore = cdnsp_plat_restore,
        .freeze = cdnsp_plat_freeze,
        .thaw = cdnsp_plat_thaw,
	.runtime_resume = cdnsp_plat_runtime_resume,
	.runtime_suspend = cdnsp_plat_runtime_suspend,
};

#ifdef CONFIG_OF
static const struct of_device_id of_cdnsp_match[] = {
	{ .compatible = "cdns,usbssp" },
	{ },
};
MODULE_DEVICE_TABLE(of, of_cdnsp_match);
#endif

static const struct acpi_device_id acpi_cdnsp_match[] = {
	{ "CIXH2031" },
	{},
};
MODULE_DEVICE_TABLE(acpi, acpi_cdnsp_match);

static struct platform_driver cdnsp_driver = {
	.probe		= cdnsp_probe,
	.remove 	= cdnsp_remove,
	.driver 	= {
		.name	= "cdns-usbssp",
		.of_match_table = of_match_ptr(of_cdnsp_match),
		.acpi_match_table = ACPI_PTR(acpi_cdnsp_match),
		.pm = &cdnsp_pm_ops,
	},
	/* some users need to wake up the system through a USB device after the system shutting
	 * down, add shutdown callback to suspend the usb controller and config it to D3, and
	 * enable_irq_wake will enable the corresponding bit for USB to wake up the system.
	 */
	.shutdown = cdnsp_cdnsp_plat_shutdown,
};

module_platform_driver(cdnsp_driver);

MODULE_AUTHOR("Chao Zeng <Chao.Zeng@cixtech.com>");
MODULE_AUTHOR("Matthew MA <Matthew.Ma@cixtech.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform: cdnsp");
MODULE_DESCRIPTION("Cdnsp common platform driver");

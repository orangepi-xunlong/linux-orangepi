// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * EHCI Driver for Ky x1 SoCs
 *
 * Copyright (c) 2023 Ky Inc.
 */

#include <linux/reset.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/usb/otg.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>
#include <linux/of_address.h>
#include <linux/platform_data/x1_ci_usb.h>
#include <dt-bindings/usb/x1_ci_usb.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_wakeirq.h>

#define CAPLENGTH_MASK				(0xff)

#define USB_CDWS_WAKE_STATUS		(1 << 28)
#define USB_ID_WAKE_STATUS			(1 << 27)
#define USB_VBUS_WAKE_STATUS		(1 << 26)
#define USB_LINS1_WAKE_STATUS		(1 << 25)
#define USB_LINS0_WAKE_STATUS		(1 << 24)

#define USB_VBUS_DRV				(1 << 21)

#define USB_CDWS_WAKE_CLEAR			(1 << 20)
#define USB_ID_WAKE_CLEAR			(1 << 19)
#define USB_VBUS_WAKE_CLEAR			(1 << 18)
#define USB_LINS1_WAKE_CLEAR		(1 << 17)
#define USB_LINS0_WAKE_CLEAR		(1 << 16)

#define USB_WAKEUP_INT_MASK			(1 << 15)
#define USB_CDWS_WAKE_MASK			(1 << 12)
#define USB_ID_WAKE_MASK			(1 << 11)
#define USB_VBUS_WAKE_MASK			(1 << 10)
#define USB_LINS1_WAKE_MASK			(1 <<  9)
#define USB_LINS0_WAKE_MASK			(1 <<  8)


struct ehci_hcd_mv {
	struct usb_hcd *hcd;
	struct usb_phy *phy;
	struct device  *dev;

	/* Which mode does this ehci running OTG/Host ? */
	int mode;

	void __iomem *cap_regs;
	void __iomem *op_regs;
	void __iomem *wakeup_reg;
	int	irq;
	struct usb_phy *otg;

	struct mv_usb_platform_data *pdata;

	struct clk *clk;
	struct reset_control *reset;
	struct regulator *vbus_otg;

	bool reset_on_resume;
};

static void mv_ehci_enable_wakeup_irqs(struct ehci_hcd_mv *ehci_mv)
{
	u32 reg;
	reg = readl(ehci_mv->wakeup_reg);
	reg |= (USB_LINS0_WAKE_MASK | USB_LINS1_WAKE_MASK);
	writel(reg, ehci_mv->wakeup_reg);
}

static void mv_ehci_disable_wakeup_irqs(struct ehci_hcd_mv *ehci_mv)
{
	u32 reg;
	reg = readl(ehci_mv->wakeup_reg);
	reg &= ~(USB_LINS0_WAKE_MASK | USB_LINS1_WAKE_MASK);
	writel(reg, ehci_mv->wakeup_reg);
}

static void mv_ehci_clear_wakeup_irqs(struct ehci_hcd_mv *ehci_mv)
{
	u32 reg;
	reg = readl(ehci_mv->wakeup_reg);
	reg |= (USB_LINS0_WAKE_CLEAR | USB_LINS1_WAKE_CLEAR);
	writel(reg, ehci_mv->wakeup_reg);
}

static irqreturn_t mv_ehci_wakeup_interrupt(int irq, void *_ehci_mv)
{
	struct ehci_hcd_mv *ehci_mv = _ehci_mv;
	u32 reg;
	reg = readl(ehci_mv->wakeup_reg);
	dev_dbg(ehci_mv->dev, "wakeup_reg: 0x%x\n", reg);

	mv_ehci_disable_wakeup_irqs(ehci_mv);
	mv_ehci_clear_wakeup_irqs(ehci_mv);

	return IRQ_HANDLED;
}

static int mv_ehci_setvbus(struct ehci_hcd_mv *ehci_mv, bool enable)
{
	u32 reg;
	reg = readl(ehci_mv->wakeup_reg);
	if (enable)
		reg |= USB_VBUS_DRV;
	else
		reg &= ~USB_VBUS_DRV;
	writel(reg, ehci_mv->wakeup_reg);
	return 0;
}

static int mv_ehci_enable(struct ehci_hcd_mv *ehci_mv)
{
	int ret;

	ret = clk_prepare_enable(ehci_mv->clk);
	if (ret){
		dev_err(ehci_mv->dev, "Failed to enable clock\n");
		return ret;
	}

	ret = reset_control_deassert(ehci_mv->reset);
	if (ret){
		dev_err(ehci_mv->dev, "Failed to deassert reset control\n");
		goto err_clk;
	}

	ret = usb_phy_init(ehci_mv->phy);
	if (ret) {
		dev_err(ehci_mv->dev, "Failed to init phy\n");
		goto err_reset;
	}

	return 0;

err_reset:
	reset_control_assert(ehci_mv->reset);
err_clk:
	clk_disable_unprepare(ehci_mv->clk);
	return ret;
}

static void mv_ehci_disable(struct ehci_hcd_mv *ehci_mv)
{
	usb_phy_shutdown(ehci_mv->phy);
	reset_control_assert(ehci_mv->reset);
	clk_disable_unprepare(ehci_mv->clk);
}

static int mv_ehci_reset(struct usb_hcd *hcd)
{
	struct device *dev = hcd->self.controller;
	struct ehci_hcd_mv *ehci_mv = dev_get_drvdata(dev);
	int retval;

	if (ehci_mv == NULL) {
		dev_err(dev, "Can not find private ehci data\n");
		return -ENODEV;
	}

	hcd->has_tt = 1;

	retval = ehci_setup(hcd);
	if (retval)
		dev_err(dev, "ehci_setup failed %d\n", retval);

	return retval;
}

static const struct hc_driver mv_ehci_hc_driver = {
	.description = hcd_name,
	.product_desc = "Ky EHCI",
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2 | HCD_BH | HCD_DMA,

	/*
	 * basic lifecycle operations
	 */
	.reset = mv_ehci_reset,
	.start = ehci_run,
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,
	.endpoint_reset = ehci_endpoint_reset,
	.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ehci_hub_status_data,
	.hub_control =		ehci_hub_control,
	.bus_suspend =		ehci_bus_suspend,
	.bus_resume =		ehci_bus_resume,
	.relinquish_port =	ehci_relinquish_port,
	.port_handed_over =	ehci_port_handed_over,
	.get_resuming_ports =	ehci_get_resuming_ports,

	/*
	 * device support
	 */
	.free_dev =		ehci_remove_device,
};

static int mv_ehci_dt_parse(struct platform_device *pdev,
			struct mv_usb_platform_data *pdata)
{
	struct device_node *np = pdev->dev.of_node;

	if (of_property_read_string(np,
			"ky,ehci-name", &((pdev->dev).init_name)))
		return -EINVAL;

	if (of_property_read_u32(np, "ky,udc-mode", &(pdata->mode)))
		return -EINVAL;

	if (of_property_read_u32(np, "ky,dev-id", &(pdata->id)))
		pdata->id = PXA_USB_DEV_OTG;

	of_property_read_u32(np, "ky,extern-attr", &(pdata->extern_attr));
	pdata->otg_force_a_bus_req = of_property_read_bool(np,
					"ky,otg-force-a-bus-req");
	pdata->disable_otg_clock_gating = of_property_read_bool(np,
						"ky,disable-otg-clock-gating");

	return 0;
}

static int mv_ehci_probe(struct platform_device *pdev)
{
	struct mv_usb_platform_data *pdata;
	struct device *dev = &pdev->dev;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	struct ehci_hcd_mv *ehci_mv;
	struct resource *r;
	struct device_link *link;
	int retval = -ENODEV;
	bool wakeup_source;
	u32 offset;

	dev_dbg(&pdev->dev, "mv_ehci_probe: Enter ... \n");
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(&pdev->dev, "failed to allocate memory for platform_data\n");
		return -ENODEV;
	}
	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	mv_ehci_dt_parse(pdev, pdata);
	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we have dma capability bindings this can go away.
	 */
	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	if (!dev->coherent_dma_mask)
		dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (usb_disabled())
		return -ENODEV;

	hcd = usb_create_hcd(&mv_ehci_hc_driver, &pdev->dev, dev_name(dev));
	if (!hcd)
		return -ENOMEM;

	ehci_mv = devm_kzalloc(&pdev->dev, sizeof(*ehci_mv), GFP_KERNEL);
	if (ehci_mv == NULL) {
		dev_err(&pdev->dev, "cannot allocate ehci_hcd_mv\n");
		retval = -ENOMEM;
		goto err_put_hcd;
	}

	platform_set_drvdata(pdev, ehci_mv);
	ehci_mv->pdata = pdata;
	ehci_mv->hcd = hcd;
	ehci_mv->dev = dev;
	ehci_mv->reset_on_resume = of_property_read_bool(pdev->dev.of_node,
		"ky,reset-on-resume");

	ehci_mv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(ehci_mv->clk)) {
		dev_err(&pdev->dev, "error getting clock\n");
		retval = PTR_ERR(ehci_mv->clk);
		goto err_clear_drvdata;
	}

	ehci_mv->reset = devm_reset_control_array_get_optional_shared(&pdev->dev);
	if (IS_ERR(ehci_mv->reset)) {
		dev_err(&pdev->dev, "failed to get reset control\n");
		retval = PTR_ERR(ehci_mv->reset);
		goto err_clear_drvdata;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&pdev->dev, "no I/O memory resource defined\n");
		retval = -ENODEV;
		goto err_clear_drvdata;
	}

	ehci_mv->cap_regs = devm_ioremap(&pdev->dev, r->start,
					 resource_size(r));
	if (ehci_mv->cap_regs == NULL) {
		dev_err(&pdev->dev, "failed to map I/O memory\n");
		retval = -EFAULT;
		goto err_clear_drvdata;
	}

	ehci_mv->phy = devm_usb_get_phy_by_phandle(&pdev->dev, "usb-phy", 0);
	if (IS_ERR_OR_NULL(ehci_mv->phy)) {
		retval = PTR_ERR(ehci_mv->phy);
		if (retval != -EPROBE_DEFER && retval != -ENODEV)
			dev_err(&pdev->dev, "failed to get the outer phy\n");
		else {
			kfree(hcd->bandwidth_mutex);
			kfree(hcd);
			return -EPROBE_DEFER;
		}
		goto err_clear_drvdata;
	}

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
	device_enable_async_suspend(&pdev->dev);

	retval = mv_ehci_enable(ehci_mv);
	if (retval) {
		dev_err(&pdev->dev, "enable ehci error: %d\n", retval);
		goto err_clear_drvdata;
	}

	offset = readl(ehci_mv->cap_regs) & CAPLENGTH_MASK;
	ehci_mv->op_regs =
		(void __iomem *) ((unsigned long) ehci_mv->cap_regs + offset);
	hcd->rsrc_start = r->start;
	hcd->rsrc_len = resource_size(r);
	hcd->regs = ehci_mv->op_regs;

	hcd->irq = platform_get_irq(pdev, 0);
	if (!hcd->irq) {
		dev_err(&pdev->dev, "Cannot get irq.");
		retval = -ENODEV;
		goto err_disable_clk_rst;
	}

	ehci = hcd_to_ehci(hcd);
	ehci->caps = (struct ehci_caps *) ehci_mv->cap_regs;

	ehci_mv->mode = pdata->mode;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!r) {
		dev_err(dev, "missing wakeup base resource\n");
		retval = -ENODEV;
		goto err_disable_clk_rst;
	}

	ehci_mv->wakeup_reg = devm_ioremap(&pdev->dev, r->start, resource_size(r));
	if (!ehci_mv->wakeup_reg) {
		dev_err(dev, " wakeup reg ioremap failed\n");
		retval = -ENODEV;
		goto err_disable_clk_rst;
	}

	if (ehci_mv->mode == MV_USB_MODE_OTG) {
		dev_info(dev, "mode: MV_USB_MODE_OTG ...\n");
		ehci_mv->otg = devm_usb_get_phy_by_phandle(&pdev->dev, "usb-otg", 0);
		if (IS_ERR(ehci_mv->otg)) {
			retval = PTR_ERR(ehci_mv->otg);
			if (retval == -ENXIO)
				dev_info(&pdev->dev, "MV_USB_MODE_OTG "
						"must have CONFIG_USB_PHY enabled\n");
			else if (retval != -EPROBE_DEFER)
				dev_err(&pdev->dev,
						"unable to find transceiver\n");
			goto err_disable_clk_rst;
		}
		/* devm_usb_get_phy_by_phandle() doesn't create device link while
		 * normal phy does. Add a link here to ensure correct pm order. */
		link = device_link_add(dev, ehci_mv->otg->dev,
				       DL_FLAG_STATELESS);
		if (!link) {
			dev_err(dev, "failed to create device link to %s\n",
				dev_name(ehci_mv->otg->dev));
			retval = -EINVAL;
			goto err_disable_clk_rst;
		}
		retval = otg_set_host(ehci_mv->otg->otg, &hcd->self);
		if (retval < 0) {
			dev_err(&pdev->dev,
				"unable to register with transceiver\n");
			retval = -ENODEV;
			goto err_disable_clk_rst;
		}
		/* otg will enable clock before use as host */
		mv_ehci_disable(ehci_mv);
	} else {
		retval = mv_ehci_setvbus(ehci_mv, 1);
		if (retval)
			goto err_disable_clk_rst;

		retval = usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
		if (retval) {
			dev_err(&pdev->dev,
				"failed to add hcd with err %d\n", retval);
			goto err_set_vbus;
		}
		device_wakeup_enable(hcd->self.controller);
	}

	ehci_mv->irq = platform_get_irq(pdev, 1);
	if (!hcd->irq) {
		dev_err(&pdev->dev, "Cannot get wake irq.");
		retval = -ENODEV;
		goto err_set_vbus;
	}

	retval = devm_request_irq(dev, ehci_mv->irq, mv_ehci_wakeup_interrupt,
				  IRQF_NO_SUSPEND | IRQF_SHARED, "usb-wakeup",
				  ehci_mv);
	if (retval) {
		dev_err(dev, "failed to request IRQ #%d --> %d\n",
				ehci_mv->irq, retval);
		goto err_set_vbus;
	}

	wakeup_source = of_property_read_bool(dev->of_node, "wakeup-source");
	if (wakeup_source) {
		device_init_wakeup(dev, true);
		dev_pm_set_wake_irq(dev, ehci_mv->irq);
	}

	dev_dbg(&pdev->dev,
		 "successful find EHCI device with regs 0x%p irq %d"
		 " working in %s mode\n", hcd->regs, hcd->irq,
		 ehci_mv->mode == MV_USB_MODE_OTG ? "OTG" : "Host");

	return 0;

err_set_vbus:
	if (!IS_ERR_OR_NULL(ehci_mv->otg))
		otg_set_host(ehci_mv->otg->otg, NULL);
	if (ehci_mv->mode == MV_USB_MODE_HOST) {
		usb_remove_hcd(hcd);
		mv_ehci_setvbus(ehci_mv, 0);
	}
err_disable_clk_rst:
	mv_ehci_disable(ehci_mv);
err_clear_drvdata:
	platform_set_drvdata(pdev, NULL);
err_put_hcd:
	usb_put_hcd(hcd);

	return retval;
}

static int mv_ehci_remove(struct platform_device *pdev)
{
	struct ehci_hcd_mv *ehci_mv = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = ehci_mv->hcd;
	bool do_wakeup = device_may_wakeup(&pdev->dev);

	if (do_wakeup) {
		mv_ehci_disable_wakeup_irqs(ehci_mv);
		dev_pm_clear_wake_irq(ehci_mv->dev);
		device_init_wakeup(ehci_mv->dev, false);
	}

	if (hcd->rh_registered)
		usb_remove_hcd(hcd);

	if (!IS_ERR_OR_NULL(ehci_mv->otg))
		otg_set_host(ehci_mv->otg->otg, NULL);

	if (ehci_mv->mode == MV_USB_MODE_HOST) {
		mv_ehci_setvbus(ehci_mv, 0);
	}

	mv_ehci_disable(ehci_mv);
	platform_set_drvdata(pdev, NULL);
	usb_put_hcd(hcd);

	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	return 0;
}

MODULE_ALIAS("mv-ehci");

static const struct of_device_id mv_ehci_dt_match[] = {
	{.compatible = "ky,mv-ehci"},
	{},
};
MODULE_DEVICE_TABLE(of, mv_ehci_dt_match);

static void mv_ehci_shutdown(struct platform_device *pdev)
{
	struct ehci_hcd_mv *ehci_mv = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = ehci_mv->hcd;

	if (!hcd->rh_registered)
		return;

	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);
}

#ifdef CONFIG_PM_SLEEP
static int mv_ehci_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ehci_hcd_mv *ehci_mv = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = ehci_mv->hcd;
	bool do_wakeup = device_may_wakeup(dev);
	int ret;

	/* OTG is not working in host mode thus hcd is stopped */
	if (hcd->state == HC_STATE_HALT)
		return 0;

	ret = ehci_suspend(hcd, do_wakeup);
	if (ret)
		return ret;

	/* OTG driver will handle these for us */
	if (ehci_mv->mode == MV_USB_MODE_OTG)
		goto disable_done;

	usb_phy_shutdown(ehci_mv->phy);

	if (ehci_mv->reset_on_resume) {
		ret = reset_control_assert(ehci_mv->reset);
		if (ret)
			return ret;
		dev_info(dev, "Will reset controller and phy on resume\n");
	}

	clk_disable_unprepare(ehci_mv->clk);
	dev_dbg(dev, "pm suspend: disable clks and phy\n");

disable_done:
	if (do_wakeup) {
		mv_ehci_clear_wakeup_irqs(ehci_mv);
		mv_ehci_enable_wakeup_irqs(ehci_mv);
	}
	return ret;
}

static int mv_ehci_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ehci_hcd_mv *ehci_mv = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = ehci_mv->hcd;
	int ret;

	if (hcd->state == HC_STATE_HALT)
		return 0;

	if (ehci_mv->mode == MV_USB_MODE_OTG)
		goto enable_done;

	ret = clk_prepare_enable(ehci_mv->clk);
	if (ret){
		dev_err(dev, "Failed to enable clock");
		return ret;
	}

	if (ehci_mv->reset_on_resume) {
		dev_info(dev, "Resetting controller and phy\n");
		ret = reset_control_deassert(ehci_mv->reset);
		if (ret)
			return ret;
	}

	ret = usb_phy_init(ehci_mv->phy);
	if (ret) {
		dev_err(dev, "Failed to init phy\n");
		clk_disable_unprepare(ehci_mv->clk);
		return ret;
	}

enable_done:
	dev_dbg(dev, "pm resume: do EHCI resume\n");
	ehci_resume(hcd, ehci_mv->reset_on_resume);
	return 0;
}
#else
#define mv_ehci_suspend	NULL
#define mv_ehci_resume	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct dev_pm_ops mv_ehci_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mv_ehci_suspend, mv_ehci_resume)
};

static struct platform_driver ehci_x1_driver = {
	.probe = mv_ehci_probe,
	.remove = mv_ehci_remove,
	.shutdown = mv_ehci_shutdown,
	.driver = {
		   .name = "mv-ehci",
		   .of_match_table = of_match_ptr(mv_ehci_dt_match),
		   .bus = &platform_bus_type,
#ifdef CONFIG_PM
		   .pm	= &mv_ehci_pm_ops,
#endif
		   },
};

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * OTG support for Ky x1 SoCs
 *
 * Copyright (c) 2023 Ky Inc.
 */
#include <linux/irqreturn.h>
#include <linux/reset.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>
#include <linux/platform_data/x1_ci_usb.h>
#include <linux/of_address.h>
#include <dt-bindings/usb/x1_ci_usb.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/extcon.h>
#include "phy-x1-ci-otg.h"


#define MAX_RETRY_TIMES 60
#define RETRY_SLEEP_MS 1000

#define PMU_SD_ROT_WAKE_CLR_VBUS_DRV (0x1 << 21)

static const char driver_name[] = "x1-ci-otg";

static const char *const state_string[] = {
	[MV_OTG_ROLE_UNDEFINED] = "undefined",
	[MV_OTG_ROLE_DEVICE_IDLE] = "idle",
	[MV_OTG_ROLE_DEVICE_ACTIVE] = "device",
	[MV_OTG_ROLE_HOST_ACTIVE] = "host",
};

static int mv_otg_set_vbus(struct usb_otg *otg, bool on)
{
	struct mv_otg *mvotg = container_of(otg->usb_phy, struct mv_otg, phy);
	uint32_t temp;
	int ret = 0;

	dev_dbg(&mvotg->pdev->dev, "%s  on= %d ... \n", __func__, on);

	if (mvotg->vbus_otg) {
		if (on)
			ret = regulator_enable(mvotg->vbus_otg);
		else
			ret = regulator_disable(mvotg->vbus_otg);

		if (ret)
			dev_err(&mvotg->pdev->dev, "regulator set %d fail\n", on);
		return ret;
	}

	if (on) {
		temp = readl(mvotg->wakeup_reg);
		writel(PMU_SD_ROT_WAKE_CLR_VBUS_DRV | temp, mvotg->wakeup_reg);
	} else {
		temp = readl(mvotg->wakeup_reg);
		temp &= ~PMU_SD_ROT_WAKE_CLR_VBUS_DRV;
		writel(temp, mvotg->wakeup_reg);
	}

	gpiod_set_value(mvotg->vbus_gpio, on);

	return 0;
}

static int mv_otg_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	pr_debug("%s ... \n", __func__);
	otg->host = host;
	return 0;
}

static int mv_otg_set_peripheral(struct usb_otg *otg, struct usb_gadget *gadget)
{
	pr_debug("%s ... \n", __func__);
	otg->gadget = gadget;
	return 0;
}

static void mv_otg_run_state_machine(struct mv_otg *mvotg, unsigned long delay)
{
	dev_dbg(&mvotg->pdev->dev, "mv_otg_run_state_machine ... \n");
	dev_dbg(&mvotg->pdev->dev, "transceiver is updated\n");
	if (!mvotg->qwork)
		return;

	queue_delayed_work(mvotg->qwork, &mvotg->work, delay);
}

static int mv_otg_reset(struct mv_otg *mvotg)
{
	unsigned int loops;
	u32 tmp;

	dev_dbg(&mvotg->pdev->dev, "mv_otg_reset \n");
	/* Stop the controller */
	tmp = readl(&mvotg->op_regs->usbcmd);
	tmp &= ~USBCMD_RUN_STOP;
	writel(tmp, &mvotg->op_regs->usbcmd);

	/* Reset the controller to get default values */
	writel(USBCMD_CTRL_RESET, &mvotg->op_regs->usbcmd);

	loops = 500;
	while (readl(&mvotg->op_regs->usbcmd) & USBCMD_CTRL_RESET) {
		if (loops == 0) {
			dev_err(&mvotg->pdev->dev,
				"Wait for RESET completed TIMEOUT\n");
			return -ETIMEDOUT;
		}
		loops--;
		udelay(20);
	}

	writel(0x0, &mvotg->op_regs->usbintr);
	tmp = readl(&mvotg->op_regs->usbsts);
	writel(tmp, &mvotg->op_regs->usbsts);

	return 0;
}

static void mv_otg_start_host(struct mv_otg *mvotg, int on)
{
	struct usb_otg *otg = mvotg->phy.otg;
	struct usb_hcd *hcd;

	dev_dbg(&mvotg->pdev->dev, "%s ...\n", __func__);
	if (!otg->host) {
		int retry = 0;
		while (retry < MAX_RETRY_TIMES) {
			retry++;
			msleep(RETRY_SLEEP_MS);
			if (otg->host)
				break;
		}

		if (!otg->host) {
			dev_err(&mvotg->pdev->dev, "otg->host is not set!\n");
			return;
		}
	}

	dev_info(&mvotg->pdev->dev, "%s host\n", on ? "start" : "stop");

	hcd = bus_to_hcd(otg->host);

	if (on) {
		usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
		device_wakeup_enable(hcd->self.controller);
	} else {
		usb_remove_hcd(hcd);
	}
}

static void mv_otg_start_peripheral(struct mv_otg *mvotg, int on)
{
	struct usb_otg *otg = mvotg->phy.otg;

	dev_dbg(&mvotg->pdev->dev, "%s ...\n", __func__);
	if (!otg->gadget) {
		int retry = 0;
		while (retry < MAX_RETRY_TIMES) {
			retry++;
			msleep(RETRY_SLEEP_MS);
			if (otg->gadget)
				break;
		}

		if (!otg->gadget) {
			dev_err(&mvotg->pdev->dev, "otg->gadget is not set!\n");
			return;
		}
	}

	dev_info(&mvotg->pdev->dev, "gadget %s\n", on ? "on" : "off");

	if (on)
		usb_gadget_vbus_connect(otg->gadget);
	else
		usb_gadget_vbus_disconnect(otg->gadget);
}

static void otg_clock_enable(struct mv_otg *mvotg)
{
	clk_enable(mvotg->clk);
}

static void otg_reset_assert(struct mv_otg *mvotg)
{
	reset_control_assert(mvotg->reset);
}

static void otg_reset_deassert(struct mv_otg *mvotg)
{
	reset_control_deassert(mvotg->reset);
}

static void otg_clock_disable(struct mv_otg *mvotg)
{
	clk_disable(mvotg->clk);
}

static int mv_otg_enable_internal(struct mv_otg *mvotg)
{
	int retval = 0;

	dev_dbg(&mvotg->pdev->dev,
		"mv_otg_enable_internal: mvotg->active= %d \n", mvotg->active);
	if (mvotg->active)
		return 0;

	dev_dbg(&mvotg->pdev->dev,
		"otg enabled, will enable clk, release rst\n");

	otg_clock_enable(mvotg);
	otg_reset_deassert(mvotg);
	retval = usb_phy_init(mvotg->outer_phy);
	if (retval) {
		dev_err(&mvotg->pdev->dev, "failed to initialize phy %d\n",
			retval);
		otg_clock_disable(mvotg);
		return retval;
	}

	mvotg->active = 1;
	return 0;
}

static void mv_otg_disable_internal(struct mv_otg *mvotg)
{
	dev_dbg(&mvotg->pdev->dev,
		"mv_otg_disable_internal: mvotg->active= %d ... \n",
		mvotg->active);
	if (mvotg->active) {
		dev_dbg(&mvotg->pdev->dev, "otg disabled\n");
		usb_phy_shutdown(mvotg->outer_phy);
		otg_reset_assert(mvotg);
		otg_clock_disable(mvotg);
		mvotg->active = 0;
	}
}

static void mv_otg_update_inputs(struct mv_otg *mvotg)
{
	int id, vbus;

	if (mvotg->role_sw)
		return;

	vbus = extcon_get_state(mvotg->extcon, EXTCON_USB);

	switch (mvotg->dr_mode) {
	case USB_DR_MODE_HOST:
		id = 0;
		break;
	case USB_DR_MODE_PERIPHERAL:
		id = 1;
		vbus = 1;
		break;
	case USB_DR_MODE_OTG:
		id = !extcon_get_state(mvotg->extcon,EXTCON_USB_HOST);
		break;
	default:
		dev_warn(&mvotg->pdev->dev, "invalid dr_mode %d\n",
			 mvotg->dr_mode);
		id = 1;
		break;
	}

	dev_dbg(&mvotg->pdev->dev, "id %d, vbus %d\n", id, vbus);

	if (!id) {
		mvotg->desired_otg_role = MV_OTG_ROLE_HOST_ACTIVE;
	} else {
		if (vbus)
			mvotg->desired_otg_role = MV_OTG_ROLE_DEVICE_ACTIVE;
		else
			mvotg->desired_otg_role = MV_OTG_ROLE_DEVICE_IDLE;
	}
}

static void mv_otg_work(struct work_struct *work)
{
	struct mv_otg *mvotg;
	struct usb_phy *phy;
	struct usb_otg *otg;
	unsigned long flags;
	u32 current_otg_role, desired_otg_role;

	mvotg = container_of(to_delayed_work(work), struct mv_otg, work);

	/* work queue is single thread, or we need spin_lock to protect */
	phy = &mvotg->phy;
	otg = mvotg->phy.otg;

	spin_lock_irqsave(&mvotg->lock, flags);

	/* Update input from extcon/dr_mode if no role_sw registered,
	 * role switch will update desired_otg_role directly.
	 */
	mv_otg_update_inputs(mvotg);
	current_otg_role = mvotg->current_otg_role;
	desired_otg_role = mvotg->desired_otg_role;
	spin_unlock_irqrestore(&mvotg->lock, flags);

	dev_dbg(&mvotg->pdev->dev,
		"otg role change: current role: %s, desired role: %s\n",
		state_string[current_otg_role],
		state_string[desired_otg_role]);

	if (current_otg_role == desired_otg_role)
		return;

	if (current_otg_role == MV_OTG_ROLE_HOST_ACTIVE) {
		/* exit host and turn off the controller */
		mv_otg_set_vbus(otg, 0);
		mv_otg_start_host(mvotg, 0);
		mv_otg_reset(mvotg);
		mv_otg_disable_internal(mvotg);
	} else if (current_otg_role == MV_OTG_ROLE_DEVICE_ACTIVE) {
		mv_otg_start_peripheral(mvotg, 0);
	}

	/* start new role */
	switch (desired_otg_role) {
	case MV_OTG_ROLE_HOST_ACTIVE:
		mv_otg_enable_internal(mvotg);
		mv_otg_reset(mvotg);
		mv_otg_start_host(mvotg, 1);
		mv_otg_set_vbus(otg, 1);
		break;
	case MV_OTG_ROLE_DEVICE_ACTIVE:
		/* tell udc vbus is on, let udc decide clock gating */
		mv_otg_start_peripheral(mvotg, 1);
		break;
	case MV_OTG_ROLE_DEVICE_IDLE:
		/* tell udc vbus connector is gone */
		mv_otg_start_peripheral(mvotg, 0);
		break;
	default:
		break;
	}

	spin_lock_irqsave(&mvotg->lock, flags);
	mvotg->current_otg_role = desired_otg_role;
	spin_unlock_irqrestore(&mvotg->lock, flags);
}

static int mv_otg_vbus_notifier_callback(struct notifier_block *nb,
					 unsigned long val, void *v)
{
	struct usb_phy *mvotg_phy = container_of(nb, struct usb_phy, vbus_nb);
	struct mv_otg *mvotg = container_of(mvotg_phy, struct mv_otg, phy);

	mv_otg_run_state_machine(mvotg, 0);

	return 0;
}

static int mv_otg_id_notifier_callback(struct notifier_block *nb,
				       unsigned long val, void *v)
{
	struct usb_phy *mvotg_phy = container_of(nb, struct usb_phy, id_nb);
	struct mv_otg *mvotg = container_of(mvotg_phy, struct mv_otg, phy);

	mv_otg_run_state_machine(mvotg, 0);

	return 0;
}

static ssize_t get_dr_mode(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct mv_otg *mvotg = dev_get_drvdata(dev);
	unsigned long flags;
	enum usb_dr_mode dr_mode;
	char *state;

	spin_lock_irqsave(&mvotg->lock, flags);
	dr_mode = mvotg->dr_mode;
	spin_unlock_irqrestore(&mvotg->lock, flags);

	if (mvotg->dr_mode == USB_DR_MODE_OTG)
		state = "otg";
	else if (mvotg->dr_mode == USB_DR_MODE_HOST)
		state = "host";
	else if (mvotg->dr_mode == USB_DR_MODE_PERIPHERAL)
		state = "device";
	else
		state = "UNKNOWN";

	return sprintf(buf, "%s\n", state);
}

static ssize_t set_dr_mode(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct mv_otg *mvotg = dev_get_drvdata(dev);
	unsigned long flags;
	char *usage = "Usage: $echo otg/host/device to force set mode, "
		      "only available when no usb-role-switch exist";
	char buff[16], *b;
	enum usb_dr_mode mode;

	if (!mvotg->role_sw)
		goto err;

	strncpy(buff, buf, sizeof(buff));
	b = strim(buff);
	dev_info(dev, "OTG state is %s\n", state_string[mvotg->phy.otg->state]);
	if (!strcmp(b, "otg")) {
		mode = USB_DR_MODE_OTG;
	} else if (!strcmp(b, "host")) {
		mode = USB_DR_MODE_HOST;
	} else if (!strcmp(b, "device") || !strcmp(b, "peripheral")) {
		mode = USB_DR_MODE_PERIPHERAL;
	} else {
		goto err;
	}

	spin_lock_irqsave(&mvotg->lock, flags);
	mvotg->dr_mode = mode;
	spin_unlock_irqrestore(&mvotg->lock, flags);

	mv_otg_run_state_machine(mvotg, 0);

	return count;

err:
	dev_err(dev, "%s\n", usage);
	return -EINVAL;
}
static DEVICE_ATTR(dr_mode, S_IRUGO | S_IWUSR, get_dr_mode, set_dr_mode);

static int mv_otg_remove(struct platform_device *pdev)
{
	struct mv_otg *mvotg = platform_get_drvdata(pdev);

	device_init_wakeup(&pdev->dev, 0);
	sysfs_remove_file(&mvotg->pdev->dev.kobj, &dev_attr_dr_mode.attr);
	if (mvotg->qwork) {
		flush_workqueue(mvotg->qwork);
		destroy_workqueue(mvotg->qwork);
	}

	mv_otg_disable_internal(mvotg);

	clk_unprepare(mvotg->clk);

	usb_remove_phy(&mvotg->phy);

	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	return 0;
}

static int mv_otg_dt_parse(struct platform_device *pdev,
			   struct mv_usb_platform_data *pdata)
{
	struct device_node *np = pdev->dev.of_node;

	if (of_property_read_string(np, "ky,otg-name",
				    &((pdev->dev).init_name)))
		return -EINVAL;

	if (of_property_read_u32(np, "ky,udc-mode", &(pdata->mode)))
		return -EINVAL;

	if (of_property_read_u32(np, "ky,dev-id", &(pdata->id)))
		pdata->id = PXA_USB_DEV_OTG;

	of_property_read_u32(np, "ky,extern-attr", &(pdata->extern_attr));
	pdata->otg_force_a_bus_req =
		of_property_read_bool(np, "ky,otg-force-a-bus-req");
	pdata->disable_otg_clock_gating =
		of_property_read_bool(np, "ky,disable-otg-clock-gating");

	return 0;
}

static int mv_otg_usb_role_switch_set(struct usb_role_switch *sw,
				      enum usb_role role)
{
	struct mv_otg *mvotg = usb_role_switch_get_drvdata(sw);
	unsigned long flags;
	u32 mode = 0;

	switch (role) {
	case USB_ROLE_HOST:
		mode = MV_OTG_ROLE_HOST_ACTIVE;
		break;
	case USB_ROLE_DEVICE:
		mode = MV_OTG_ROLE_DEVICE_ACTIVE;
		break;
	default:
		if (mvotg->role_switch_default_mode == USB_DR_MODE_PERIPHERAL)
			mode = MV_OTG_ROLE_DEVICE_ACTIVE;
		else
			mode = MV_OTG_ROLE_HOST_ACTIVE;
		break;
	}

	spin_lock_irqsave(&mvotg->lock, flags);
	mvotg->desired_otg_role = mode;
	spin_unlock_irqrestore(&mvotg->lock, flags);

	dev_info(&mvotg->pdev->dev, "role switch set to: %s\n",
		 state_string[mode]);

	mv_otg_run_state_machine(mvotg, 0);

	return 0;
}

static enum usb_role mv_otg_usb_role_switch_get(struct usb_role_switch *sw)
{
	struct mv_otg *mvotg = usb_role_switch_get_drvdata(sw);
	unsigned long flags;
	static enum usb_role role;
	u32 mode;

	spin_lock_irqsave(&mvotg->lock, flags);
	mode = mvotg->current_otg_role;
	spin_unlock_irqrestore(&mvotg->lock, flags);

	switch (mode) {
	case MV_OTG_ROLE_HOST_ACTIVE:
		role = USB_ROLE_HOST;
		break;
	case MV_OTG_ROLE_DEVICE_ACTIVE:
		role = USB_ROLE_DEVICE;
		break;
	default:
		role = USB_ROLE_NONE;
		break;
	}
	return role;
}

static int mv_otg_setup_role_switch(struct mv_otg *mvotg)
{
	struct usb_role_switch_desc mv_otg_role_switch = { NULL };
	struct device *dev = &mvotg->pdev->dev;
	int ret;

	mvotg->role_switch_default_mode = usb_get_role_switch_default_mode(dev);
	if (mvotg->role_switch_default_mode == USB_DR_MODE_UNKNOWN) {
		mvotg->role_switch_default_mode = USB_DR_MODE_PERIPHERAL;
	}
	if (mvotg->role_switch_default_mode == USB_DR_MODE_PERIPHERAL)
		mvotg->desired_otg_role = MV_OTG_ROLE_DEVICE_ACTIVE;
	else
		mvotg->desired_otg_role = MV_OTG_ROLE_HOST_ACTIVE;

	mv_otg_role_switch.fwnode = dev_fwnode(dev);
	mv_otg_role_switch.set = mv_otg_usb_role_switch_set;
	mv_otg_role_switch.get = mv_otg_usb_role_switch_get;
	mv_otg_role_switch.allow_userspace_control =
		device_property_read_bool(dev, "role-switch-user-control");
	mv_otg_role_switch.driver_data = mvotg;
	mvotg->role_sw = usb_role_switch_register(dev, &mv_otg_role_switch);
	if (IS_ERR(mvotg->role_sw))
		return PTR_ERR(mvotg->role_sw);

	if (dev->of_node) {
		/* populate connector entry */
		pr_info("mvotg populate connector\n");
		ret = devm_of_platform_populate(dev);

		if (ret) {
			usb_role_switch_unregister(mvotg->role_sw);
			mvotg->role_sw = NULL;
			dev_err(dev,
				"mvotg platform devices creation failed: %i\n",
				ret);
			return ret;
		}
	}

	return 0;
}

static int mv_otg_probe(struct platform_device *pdev)
{
	struct mv_usb_platform_data *pdata;
	struct mv_otg *mvotg;
	struct usb_otg *otg;
	struct resource *r;
	int retval = 0;
	struct device_node *np = pdev->dev.of_node;

	dev_info(&pdev->dev, "x1 otg probe enter ...\n");
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "failed to allocate platform data\n");
		return -ENODEV;
	}
	mv_otg_dt_parse(pdev, pdata);
	mvotg = devm_kzalloc(&pdev->dev, sizeof(*mvotg), GFP_KERNEL);
	if (!mvotg) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		return -ENOMEM;
	}

	otg = devm_kzalloc(&pdev->dev, sizeof(*otg), GFP_KERNEL);
	if (!otg)
		return -ENOMEM;

	platform_set_drvdata(pdev, mvotg);

	mvotg->pdev = pdev;
	mvotg->pdata = pdata;

	mvotg->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(mvotg->clk))
		return PTR_ERR(mvotg->clk);
	clk_prepare(mvotg->clk);

	mvotg->reset = devm_reset_control_get_shared(&pdev->dev, NULL);
	if (IS_ERR(mvotg->reset))
		return PTR_ERR(mvotg->reset);

	mvotg->qwork = create_singlethread_workqueue("mv_otg_queue");
	if (!mvotg->qwork) {
		dev_dbg(&pdev->dev, "cannot create workqueue for OTG\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&mvotg->work, mv_otg_work);

	mvotg->dr_mode = usb_get_role_switch_default_mode(&pdev->dev);
	if (mvotg->dr_mode == USB_DR_MODE_UNKNOWN)
		mvotg->dr_mode = USB_DR_MODE_OTG;

	/* OTG common part */
	mvotg->pdev = pdev;
	mvotg->phy.dev = &pdev->dev;
	mvotg->phy.type = USB_PHY_TYPE_USB2;
	mvotg->phy.otg = otg;
	mvotg->phy.label = driver_name;
	mvotg->dr_mode = USB_DR_MODE_OTG;

	otg->usb_phy = &mvotg->phy;
	otg->state = OTG_STATE_UNDEFINED;
	otg->set_host = mv_otg_set_host;
	otg->set_peripheral = mv_otg_set_peripheral;
	otg->set_vbus = mv_otg_set_vbus;

	r = platform_get_resource(mvotg->pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no I/O memory resource defined\n");
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	mvotg->cap_regs = devm_ioremap(&pdev->dev, r->start, resource_size(r));
	if (mvotg->cap_regs == NULL) {
		dev_err(&pdev->dev, "failed to map I/O memory\n");
		retval = -EFAULT;
		goto err_destroy_workqueue;
	}

	r = platform_get_resource(mvotg->pdev, IORESOURCE_MEM, 1);
	if (r == NULL) {
		dev_err(&pdev->dev, "no apmu base memory resource defined\n");
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	mvotg->wakeup_reg = devm_ioremap(&pdev->dev, r->start, resource_size(r));
	if (mvotg->wakeup_reg == NULL) {
		dev_err(&pdev->dev, "failed to map apmu base memory\n");
		retval = -EFAULT;
		goto err_destroy_workqueue;
	}

	mvotg->outer_phy =
		devm_usb_get_phy_by_phandle(&pdev->dev, "usb-phy", 0);
	if (IS_ERR_OR_NULL(mvotg->outer_phy)) {
		retval = PTR_ERR(mvotg->outer_phy);
		if (retval != -EPROBE_DEFER)
			dev_err(&pdev->dev, "can not find outer phy\n");
		goto err_destroy_workqueue;
	}

	if (of_property_read_bool(np, "extcon")) {
		dev_info(&pdev->dev, "support extcon detect ...\n");
		mvotg->extcon = extcon_get_edev_by_phandle(&pdev->dev, 0);
		if (IS_ERR(mvotg->extcon)) {
			dev_err(&pdev->dev, "couldn't get extcon device\n");
			retval = -EPROBE_DEFER;
			goto err_destroy_workqueue;
		}
		dev_info(&pdev->dev, "extcon_dev name: %s \n",
			 extcon_get_edev_name(mvotg->extcon));
		/*extcon notifier register will be completed in usb_add_phy_dev*/
		mvotg->phy.vbus_nb.notifier_call = mv_otg_vbus_notifier_callback;
		mvotg->phy.id_nb.notifier_call = mv_otg_id_notifier_callback;
	}

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
	device_enable_async_suspend(&pdev->dev);

	mvotg->vbus_gpio =
		devm_gpiod_get_optional(&pdev->dev, "vbus", GPIOD_OUT_LOW);
	if (IS_ERR(mvotg->vbus_gpio)) {
		dev_err(&pdev->dev, "can not find vbus gpio default state\n");
		goto err_destroy_workqueue;
	}

	mvotg->vbus_otg = devm_regulator_get_optional(&pdev->dev, "vbus");
	if (PTR_ERR(mvotg->vbus_otg) == -ENODEV) {
		mvotg->vbus_otg = NULL;
	} else if (IS_ERR(mvotg->vbus_otg)) {
		dev_err(&pdev->dev, "can not get vbus regulator\n");
		goto err_destroy_workqueue;
	}

	/* we will acces controller register, so enable the udc controller */
	retval = mv_otg_enable_internal(mvotg);
	if (retval) {
		dev_err(&pdev->dev, "mv otg enable error %d\n", retval);
		goto err_destroy_workqueue;
	}

	mvotg->op_regs =
		(struct mv_otg_regs __iomem *)((unsigned long)mvotg->cap_regs +
						   (readl(mvotg->cap_regs) &
						CAPLENGTH_MASK));

	mv_otg_reset(mvotg);

	retval = usb_add_phy_dev(&mvotg->phy);
	if (retval < 0) {
		dev_err(&pdev->dev, "can't register transceiver, %d\n", retval);
		goto err_disable_clk;
	}

	spin_lock_init(&mvotg->lock);

	/* Init the first state, turn off everything! */
	gpiod_set_value(mvotg->vbus_gpio, 0);
	mv_otg_disable_internal(mvotg);
	mvotg->current_otg_role = MV_OTG_ROLE_UNDEFINED;

	if (!mvotg->extcon &&
	    device_property_read_bool(&pdev->dev, "usb-role-switch")) {
		mv_otg_setup_role_switch(mvotg);
		mvotg->role_switch_default_mode =
			usb_get_role_switch_default_mode(&pdev->dev);
		if (mvotg->role_switch_default_mode == USB_DR_MODE_UNKNOWN)
			mvotg->role_switch_default_mode =
				USB_DR_MODE_PERIPHERAL;
	}

	mvotg->host_remote_wakeup =
		!device_property_read_bool(&pdev->dev, "ky,reset-on-resume");

	mv_otg_run_state_machine(mvotg, msecs_to_jiffies(200));

	dev_info(&pdev->dev, "successful probe OTG device.\n");

	retval = sysfs_create_file(&mvotg->pdev->dev.kobj,
				   &dev_attr_dr_mode.attr);
	if (retval < 0) {
		dev_dbg(&pdev->dev, "Can't register sysfs attr otg_mode: %d\n",
			retval);
		goto err_remove_otg_phy;
	}

	return 0;

err_remove_otg_phy:
	usb_remove_phy(&mvotg->phy);
err_disable_clk:
	mv_otg_disable_internal(mvotg);
err_destroy_workqueue:
	flush_workqueue(mvotg->qwork);
	destroy_workqueue(mvotg->qwork);

	return retval;
}

#ifdef CONFIG_PM
static int mv_otg_suspend(struct device *dev)
{
	struct mv_otg *mvotg = platform_get_drvdata(to_platform_device(dev));

	/* clk is enabled when we are at host state */
	if (mvotg->current_otg_role == MV_OTG_ROLE_HOST_ACTIVE) {
		if (mvotg->host_remote_wakeup && mvotg->active) {
			usb_phy_set_suspend(mvotg->outer_phy, true);
			otg_clock_disable(mvotg);
			mvotg->active = 0;
		} else {
			mv_otg_disable_internal(mvotg);
		}
	} else if (mvotg->current_otg_role == MV_OTG_ROLE_DEVICE_ACTIVE) {
		mv_otg_start_peripheral(mvotg, false);
	}

	return 0;
}

static int mv_otg_resume(struct device *dev)
{
	struct mv_otg *mvotg = platform_get_drvdata(to_platform_device(dev));

	if (mvotg->current_otg_role == MV_OTG_ROLE_HOST_ACTIVE) {
		if (mvotg->host_remote_wakeup && !mvotg->active) {
			otg_clock_enable(mvotg);
			usb_phy_set_suspend(mvotg->outer_phy, false);
			mvotg->active = 1;
		} else {
			mv_otg_enable_internal(mvotg);
		}
	} else if (mvotg->current_otg_role == MV_OTG_ROLE_DEVICE_ACTIVE) {
		mv_otg_start_peripheral(mvotg, true);
	}

	mv_otg_run_state_machine(mvotg, 0);

	return 0;
}

static const struct dev_pm_ops mv_otg_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mv_otg_suspend, mv_otg_resume)
};

#endif

static const struct of_device_id mv_otg_dt_match[] = {
	{ .compatible = "ky,mv-otg" },
	{},
};
MODULE_DEVICE_TABLE(of, mv_udc_dt_match);

static struct platform_driver mv_otg_driver = {
	.probe = mv_otg_probe,
	.remove = mv_otg_remove,
	.driver = {
		   .of_match_table = of_match_ptr(mv_otg_dt_match),
		   .name = driver_name,
#ifdef CONFIG_PM
		   .pm = &mv_otg_pm_ops,
#endif
	},
};
module_platform_driver(mv_otg_driver);

MODULE_DESCRIPTION("Ky X1-x OTG driver");
MODULE_LICENSE("GPL");

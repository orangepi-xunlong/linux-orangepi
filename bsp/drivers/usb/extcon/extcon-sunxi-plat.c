// SPDX-License-Identifier: GPL-2.0+
/*
 * Allwinner USB2.0 HOST External Connector Platform Driver
 *
 * Copyright (c) 2024 Allwinnertech Co., Ltd.
 * Author: kanghoupeng <kanghoupeng@allwinnertech.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/extcon.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>

enum sunxi_extcon_vbus_id_status {
	SUNXI_EXTCON_ID_FLOAT,
	SUNXI_EXTCON_ID_GROUND,
	SUNXI_EXTCON_VBUS_OFF,
	SUNXI_EXTCON_VBUS_VALID,
};
struct extcon_sunxi_plat {
	struct device		*dev;

	__u32			usbc_no;	/* usb controller number */
	struct extcon_dev	*edev;
	struct notifier_block	vbus_nb;
	struct notifier_block	id_nb;

	struct regulator	*vbus;
	struct gpio_desc	*bcten_gpiod;

	atomic_t		hibernated;
	struct work_struct	resume_work;
	enum sunxi_extcon_vbus_id_status status;
	enum sunxi_extcon_vbus_id_status desired_status;
};

/* NOTE: Supports 4 sets of USB2.0 HOST Controller Interfaces */
#define  HCI0_USBC_NO			(0)
#define  HCI1_USBC_NO			(1)
#define  HCI2_USBC_NO			(2)
#define  HCI3_USBC_NO			(3)

#define  KEY_USBC0_NAME			"usbc0"
#define  KEY_USBC1_NAME			"usbc1"
#define  KEY_USBC2_NAME			"usbc2"
#define  KEY_USBC3_NAME			"usbc3"

extern int sunxi_usb_disable_ehci(__u32 usbc_no);
extern int sunxi_usb_enable_ehci(__u32 usbc_no);
extern int sunxi_usb_disable_ohci(__u32 usbc_no);
extern int sunxi_usb_enable_ohci(__u32 usbc_no);

static void insmod_host_driver(struct extcon_sunxi_plat *extcon)
{
	dev_info(extcon->dev, "insmod_host_driver\n\n");

#if IS_ENABLED(CONFIG_USB_EHCI_HCD_SUNXI)
	sunxi_usb_enable_ehci(extcon->usbc_no);
#endif

#if IS_ENABLED(CONFIG_USB_OHCI_HCD_SUNXI)
	sunxi_usb_enable_ohci(extcon->usbc_no);
#endif
}

static void rmmod_host_driver(struct extcon_sunxi_plat *extcon)
{
	dev_info(extcon->dev, "rmmod_host_driver\n\n");

#if IS_ENABLED(CONFIG_USB_EHCI_HCD_SUNXI)
	sunxi_usb_disable_ehci(extcon->usbc_no);
#endif

#if IS_ENABLED(CONFIG_USB_OHCI_HCD_SUNXI)
	sunxi_usb_disable_ohci(extcon->usbc_no);
#endif
}

static void sunxi_extcon_set_mailbox(struct extcon_sunxi_plat *extcon,
				   enum sunxi_extcon_vbus_id_status status)
{
	int	ret;

	dev_dbg(extcon->dev, "status %d\n", status);

	extcon->desired_status = status;

	if ((extcon->status == status) || atomic_read(&extcon->hibernated))
		return;

	switch (status) {
	case SUNXI_EXTCON_ID_GROUND:
		gpiod_set_value(extcon->bcten_gpiod, 1);
		if (extcon->vbus) {
			ret = regulator_enable(extcon->vbus);
			if (ret) {
				dev_err(extcon->dev, "regulator enable failed\n");
				return;
			}
		}
		insmod_host_driver(extcon);
		break;

	case SUNXI_EXTCON_VBUS_VALID:
		break;

	case SUNXI_EXTCON_ID_FLOAT:
		if (extcon->vbus && regulator_is_enabled(extcon->vbus))
			regulator_disable(extcon->vbus);
		rmmod_host_driver(extcon);
		gpiod_set_value(extcon->bcten_gpiod, 0);
		break;

	case SUNXI_EXTCON_VBUS_OFF:
		break;

	default:
		dev_WARN(extcon->dev, "invalid state\n");
	}
	extcon->status = status;
}

static void sunxi_extcon_resume_work(struct work_struct *work)
{
	struct extcon_sunxi_plat *extcon = container_of(work, struct extcon_sunxi_plat, resume_work);

	if (extcon->desired_status != extcon->status) {
		dev_dbg(extcon->dev, "invalid state\n");

		sunxi_extcon_set_mailbox(extcon, extcon->desired_status);
	}
}

static int sunxi_extcon_id_notifier(struct notifier_block *nb,
	unsigned long event, void *ptr)
{
	struct extcon_sunxi_plat *extcon = container_of(nb, struct extcon_sunxi_plat, id_nb);

	dev_dbg(extcon->dev, "id event %lu\n", event);

	if (event)
		sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_ID_GROUND);
	else
		sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_ID_FLOAT);

	return NOTIFY_DONE;
}

static int sunxi_extcon_vbus_notifier(struct notifier_block *nb,
	unsigned long event, void *ptr)
{
	struct extcon_sunxi_plat *extcon = container_of(nb, struct extcon_sunxi_plat, vbus_nb);

	dev_dbg(extcon->dev, "vbus event %lu\n", event);

	if (event)
		sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_VBUS_VALID);
	else
		sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_VBUS_OFF);

	return NOTIFY_DONE;
}

static int extcon_sunxi_plat_register(struct extcon_sunxi_plat *extcon)
{
	int			ret;
	struct device_node	*node = extcon->dev->of_node;
	struct extcon_dev	*edev;

	if (of_property_read_bool(node, "extcon")) {
		edev = extcon_get_edev_by_phandle(extcon->dev, 0);
		if (IS_ERR_OR_NULL(edev)) {
			dev_vdbg(extcon->dev, "couldn't get extcon device\n");
			return -EPROBE_DEFER;
		}

		extcon->vbus_nb.notifier_call = sunxi_extcon_vbus_notifier;
		ret = devm_extcon_register_notifier(extcon->dev, edev,
						EXTCON_USB, &extcon->vbus_nb);
		if (ret < 0)
			dev_vdbg(extcon->dev, "failed to register notifier for USB\n");

		extcon->id_nb.notifier_call = sunxi_extcon_id_notifier;
		ret = devm_extcon_register_notifier(extcon->dev, edev,
						EXTCON_USB_HOST, &extcon->id_nb);
		if (ret < 0)
			dev_vdbg(extcon->dev, "failed to register notifier for USB-HOST\n");

		if (extcon_get_state(edev, EXTCON_USB) == true)
			sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_VBUS_VALID);
		else
			sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_VBUS_OFF);

		if (extcon_get_state(edev, EXTCON_USB_HOST) == true)
			sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_ID_GROUND);
		else
			sunxi_extcon_set_mailbox(extcon, SUNXI_EXTCON_ID_FLOAT);

		extcon->edev = edev;
	}

	return 0;
}

static int extcon_sunxi_parse_dt(struct device *dev, struct device_node *np,
				 struct extcon_sunxi_plat *extcon)
{
	if (of_node_name_eq(np, KEY_USBC0_NAME)) {
		extcon->usbc_no = HCI0_USBC_NO;
	} else if (of_node_name_eq(np, KEY_USBC1_NAME)) {
		extcon->usbc_no = HCI1_USBC_NO;
	} else if (of_node_name_eq(np, KEY_USBC2_NAME)) {
		extcon->usbc_no = HCI2_USBC_NO;
	} else if (of_node_name_eq(np, KEY_USBC3_NAME)) {
		extcon->usbc_no = HCI3_USBC_NO;
	} else {
		pr_err("unsupported node name %s\n", of_node_full_name(np));
		return -EINVAL;
	}

	return 0;
}

static int extcon_sunxi_plat_probe(struct platform_device *pdev)
{
	struct device_node	*node = pdev->dev.of_node;
	struct extcon_sunxi_plat	*extcon;
	struct device		*dev = &pdev->dev;
	struct regulator	*vbus = NULL;
	int			ret;

	if (!node) {
		dev_err(dev, "device node not found\n");
		return -EINVAL;
	}

	extcon = devm_kzalloc(dev, sizeof(*extcon), GFP_KERNEL);
	if (!extcon)
		return -ENOMEM;

	platform_set_drvdata(pdev, extcon);
	extcon->dev = dev;

	atomic_set(&extcon->hibernated, 0);
	INIT_WORK(&extcon->resume_work, sunxi_extcon_resume_work);

	if (of_property_read_bool(node, "vbus-supply")) {
		vbus = devm_regulator_get_optional(dev, "vbus");
		if (IS_ERR(vbus)) {
			dev_err(dev, "vbus init failed\n");
			return PTR_ERR(vbus);
		}
	}
	extcon->vbus = vbus;
	extcon->bcten_gpiod = devm_gpiod_get_optional(dev, "bcten", GPIOD_OUT_LOW);
	if (IS_ERR(extcon->bcten_gpiod))
		return dev_err_probe(dev, PTR_ERR(extcon->bcten_gpiod),
				     "unable to acquire bcten gpio\n");

	ret = extcon_sunxi_parse_dt(dev, node, extcon);
	if (ret) {
		dev_err(dev, "failed to parse dts of extcon\\n");
		return ret;
	}

	ret = extcon_sunxi_plat_register(extcon);
	if (ret) {
		dev_err(dev, "failed to register extcon extcon\n");
		return ret;
	}

	return 0;
}

static void __extcon_sunxi_teardown(struct extcon_sunxi_plat *extcon)
{
	int ret;

	if (extcon->vbus && regulator_is_enabled(extcon->vbus)) {
		ret = regulator_disable(extcon->vbus);
		if (ret)
			dev_err(extcon->dev, "force disable vbus failed\n");
	}
	gpiod_set_value(extcon->bcten_gpiod, 0);
}

static int extcon_sunxi_plat_remove(struct platform_device *pdev)
{
	struct extcon_sunxi_plat	*extcon = platform_get_drvdata(pdev);

	__extcon_sunxi_teardown(extcon);

	return 0;
}

static void extcon_sunxi_plat_shutdown(struct platform_device *pdev)
{
	struct extcon_sunxi_plat	*extcon = platform_get_drvdata(pdev);

	__extcon_sunxi_teardown(extcon);
}

static int __maybe_unused extcon_sunxi_plat_suspend(struct device *dev)
{
	struct extcon_sunxi_plat	*extcon = dev_get_drvdata(dev);

	dev_dbg(extcon->dev, "%s %d\n", __func__, __LINE__);

	return 0;
}

static int __maybe_unused extcon_sunxi_plat_resume(struct device *dev)
{
	struct extcon_sunxi_plat	*extcon = dev_get_drvdata(dev);

	dev_dbg(extcon->dev, "%s %d\n", __func__, __LINE__);

	return 0;
}

static int extcon_sunxi_plat_prepare(struct device *dev)
{
	struct extcon_sunxi_plat *extcon = dev_get_drvdata(dev);

	dev_dbg(extcon->dev, "%s %d\n", __func__, __LINE__);

	atomic_set(&extcon->hibernated, 1);
	cancel_work_sync(&extcon->resume_work);

	return 0;
}

static void extcon_sunxi_plat_complete(struct device *dev)
{
	struct extcon_sunxi_plat *extcon = dev_get_drvdata(dev);

	dev_dbg(extcon->dev, "%s %d\n", __func__, __LINE__);

	atomic_set(&extcon->hibernated, 0);
	schedule_work(&extcon->resume_work);
}

static const struct dev_pm_ops sunxi_extcon_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(extcon_sunxi_plat_suspend, extcon_sunxi_plat_resume)
	.prepare	= extcon_sunxi_plat_prepare,
	.complete	= extcon_sunxi_plat_complete,
};

static const struct of_device_id of_extcon_sunxi_match[] = {
	{ .compatible = "allwinner,sunxi-plat-extcon", },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_extcon_sunxi_match);

static struct platform_driver extcon_sunxi_plat_driver = {
	.probe		= extcon_sunxi_plat_probe,
	.remove		= extcon_sunxi_plat_remove,
	.shutdown	= extcon_sunxi_plat_shutdown,
	.driver		= {
		.name	= "extcon-sunxi-plat",
		.of_match_table = of_extcon_sunxi_match,
		.pm = &sunxi_extcon_dev_pm_ops,
	},
};

static int __init sunxi_extcon_init(void)
{
	return platform_driver_register(&extcon_sunxi_plat_driver);
}

static void __exit sunxi_extcon_exit(void)
{
	return platform_driver_unregister(&extcon_sunxi_plat_driver);
}

late_initcall(sunxi_extcon_init);
module_exit(sunxi_extcon_exit);

MODULE_ALIAS("platform:sunxi-plat-extcon");
MODULE_DESCRIPTION("Allwinner USB2.0 HOST Glue Layer Driver");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.2");

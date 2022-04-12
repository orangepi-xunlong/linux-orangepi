// SPDX-License-Identifier: GPL-2.0-only
/*
 * Type-C Virtual PD Extcon driver
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 * Copyright (c) 2019 Radxa Limited
 * Copyright (c) 2019 Amarula Solutions(India)
 */

#include <linux/extcon-provider.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

static const unsigned int vpd_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_DISP_DP,
	EXTCON_NONE,
};

enum vpd_data_role {
	DR_NONE,
	DR_HOST,
	DR_DEVICE,
	DR_DISPLAY_PORT,
};

enum vpd_polarity {
	POLARITY_NORMAL,
	POLARITY_FLIP,
};

enum vpd_usb_ss {
	USB_SS_USB2,
	USB_SS_USB3,
};

struct vpd_extcon {
	struct device *dev;
	struct extcon_dev *extcon;
	struct gpio_desc *det_gpio;

	u8 polarity;
	u8 usb_ss;
	enum vpd_data_role data_role;

	int irq;
	bool enable_irq;
	struct work_struct work;
	struct delayed_work irq_work;
};

static void vpd_extcon_irq_work(struct work_struct *work)
{
	struct vpd_extcon *vpd = container_of(work, struct vpd_extcon, irq_work.work);
	bool host_connected = false, device_connected = false, dp_connected = false;
	union extcon_property_value property;
	int det;

	det = vpd->det_gpio ? gpiod_get_raw_value(vpd->det_gpio) : 0;
	if (det) {
		device_connected = (vpd->data_role == DR_DEVICE) ? true : false;
		host_connected = (vpd->data_role == DR_HOST) ? true : false;
		dp_connected = (vpd->data_role == DR_DISPLAY_PORT) ? true : false;
	}

	extcon_set_state(vpd->extcon, EXTCON_USB, host_connected);
	extcon_set_state(vpd->extcon, EXTCON_USB_HOST, device_connected);
	extcon_set_state(vpd->extcon, EXTCON_DISP_DP, dp_connected);

	property.intval = vpd->polarity;
	extcon_set_property(vpd->extcon, EXTCON_USB,
			    EXTCON_PROP_USB_TYPEC_POLARITY, property);
	extcon_set_property(vpd->extcon, EXTCON_USB_HOST,
			    EXTCON_PROP_USB_TYPEC_POLARITY, property);
	extcon_set_property(vpd->extcon, EXTCON_DISP_DP,
			    EXTCON_PROP_USB_TYPEC_POLARITY, property);

	property.intval = vpd->usb_ss;
	extcon_set_property(vpd->extcon, EXTCON_USB,
			    EXTCON_PROP_USB_SS, property);
	extcon_set_property(vpd->extcon, EXTCON_USB_HOST,
			    EXTCON_PROP_USB_SS, property);
	extcon_set_property(vpd->extcon, EXTCON_DISP_DP,
			    EXTCON_PROP_USB_SS, property);

	extcon_sync(vpd->extcon, EXTCON_USB);
	extcon_sync(vpd->extcon, EXTCON_USB_HOST);
	extcon_sync(vpd->extcon, EXTCON_DISP_DP);
}

static irqreturn_t vpd_extcon_irq_handler(int irq, void *dev_id)
{
	struct vpd_extcon *vpd = dev_id;

	schedule_delayed_work(&vpd->irq_work, msecs_to_jiffies(10));

	return IRQ_HANDLED;
}

static enum vpd_data_role vpd_extcon_data_role(struct vpd_extcon *vpd)
{
	const char *const data_roles[] = {
		[DR_NONE]		= "NONE",
		[DR_HOST]		= "host",
		[DR_DEVICE]		= "device",
		[DR_DISPLAY_PORT]	= "display-port",
	};
	struct device *dev = vpd->dev;
	int ret;
	const char *dr;

	ret = device_property_read_string(dev, "vpd-data-role", &dr);
	if (ret < 0)
		return DR_NONE;

	ret = match_string(data_roles, ARRAY_SIZE(data_roles), dr);

	return (ret < 0) ? DR_NONE : ret;
}

static int vpd_extcon_parse_dts(struct vpd_extcon *vpd)
{
	struct device *dev = vpd->dev;
	bool val = false;
	int ret;

	val = device_property_read_bool(dev, "vpd-polarity");
	if (val)
		vpd->polarity = POLARITY_FLIP;
	else
		vpd->polarity = POLARITY_NORMAL;

	val = device_property_read_bool(dev, "vpd-super-speed");
	if (val)
		vpd->usb_ss = USB_SS_USB3;
	else
		vpd->usb_ss = USB_SS_USB2;

	vpd->data_role = vpd_extcon_data_role(vpd);

	vpd->det_gpio = devm_gpiod_get_optional(dev, "det", GPIOD_OUT_LOW);
	if (IS_ERR(vpd->det_gpio)) {
		ret = PTR_ERR(vpd->det_gpio);
		dev_warn(dev, "failed to get det gpio: %d\n", ret);
		return ret;
	}

	vpd->irq = gpiod_to_irq(vpd->det_gpio);
	if (vpd->irq < 0) {
		dev_err(dev, "failed to get irq for gpio: %d\n", vpd->irq);
		return vpd->irq;
	}

	ret = devm_request_threaded_irq(dev, vpd->irq, NULL,
					vpd_extcon_irq_handler,
					IRQF_TRIGGER_FALLING |
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					NULL, vpd);
	if (ret)
		dev_err(dev, "failed to request gpio irq\n");

	return ret;
}

static int vpd_extcon_probe(struct platform_device *pdev)
{
	struct vpd_extcon *vpd;
	struct device *dev = &pdev->dev;
	int ret;

	vpd = devm_kzalloc(dev, sizeof(*vpd), GFP_KERNEL);
	if (!vpd)
		return -ENOMEM;

	vpd->dev = dev;
	ret = vpd_extcon_parse_dts(vpd);
	if (ret)
		return ret;

	INIT_DELAYED_WORK(&vpd->irq_work, vpd_extcon_irq_work);

	vpd->extcon = devm_extcon_dev_allocate(dev, vpd_cable);
	if (IS_ERR(vpd->extcon)) {
		dev_err(dev, "allocat extcon failed\n");
		return PTR_ERR(vpd->extcon);
	}

	ret = devm_extcon_dev_register(dev, vpd->extcon);
	if (ret) {
		dev_err(dev, "register extcon failed: %d\n", ret);
		return ret;
	}

	extcon_set_property_capability(vpd->extcon, EXTCON_USB,
				       EXTCON_PROP_USB_VBUS);
	extcon_set_property_capability(vpd->extcon, EXTCON_USB_HOST,
				       EXTCON_PROP_USB_VBUS);

	extcon_set_property_capability(vpd->extcon, EXTCON_USB,
				       EXTCON_PROP_USB_TYPEC_POLARITY);
	extcon_set_property_capability(vpd->extcon, EXTCON_USB_HOST,
				       EXTCON_PROP_USB_TYPEC_POLARITY);
	extcon_set_property_capability(vpd->extcon, EXTCON_USB,
				       EXTCON_PROP_USB_SS);
	extcon_set_property_capability(vpd->extcon, EXTCON_USB_HOST,
				       EXTCON_PROP_USB_SS);

	extcon_set_property_capability(vpd->extcon, EXTCON_DISP_DP,
				       EXTCON_PROP_USB_SS);
	extcon_set_property_capability(vpd->extcon, EXTCON_DISP_DP,
				       EXTCON_PROP_USB_TYPEC_POLARITY);

	platform_set_drvdata(pdev, vpd);

	vpd_extcon_irq_work(&vpd->irq_work.work);

	return 0;
}

static int vpd_extcon_remove(struct platform_device *pdev)
{
	struct vpd_extcon *vpd = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&vpd->irq_work);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int vpd_extcon_suspend(struct device *dev)
{
	struct vpd_extcon *vpd = dev_get_drvdata(dev);

	if (!vpd->enable_irq) {
		disable_irq_nosync(vpd->irq);
		vpd->enable_irq = true;
	}

	return 0;
}

static int vpd_extcon_resume(struct device *dev)
{
	struct vpd_extcon *vpd = dev_get_drvdata(dev);

	if (vpd->enable_irq) {
		enable_irq(vpd->irq);
		vpd->enable_irq = false;
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(vpd_extcon_pm_ops,
			 vpd_extcon_suspend, vpd_extcon_resume);

static const struct of_device_id vpd_extcon_dt_match[] = {
	{ .compatible = "linux,extcon-usbc-virtual-pd", },
	{ /* sentinel */ }
};

static struct platform_driver vpd_extcon_driver = {
	.probe		= vpd_extcon_probe,
	.remove		= vpd_extcon_remove,
	.driver		= {
		.name	= "extcon-usbc-virtual-pd",
		.pm	= &vpd_extcon_pm_ops,
		.of_match_table = vpd_extcon_dt_match,
	},
};

module_platform_driver(vpd_extcon_driver);

MODULE_AUTHOR("Jagan Teki <jagan@amarulasolutions.com>");
MODULE_DESCRIPTION("Type-C Virtual PD extcon driver");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for mux/switch based on external phy for sunxi
 * tcpm framework.
 *
 * Copyright (c) 2023-2024 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/of_gpio.h>
#include <linux/phy/phy.h>
#include <dt-bindings/phy/phy.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/power_supply.h>

struct sunxi_phy_switcher {
	struct mutex lock; /* protects the cached conf register */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
	struct typec_switch *sw;
	struct typec_mux *mux;
#else
	struct typec_switch_dev *sw;
	struct typec_mux_dev *mux;
#endif
	struct device *dev;
	struct phy *dp_phy;
	struct phy *usb_phy;
	struct extcon_dev *extcon;

	/* for external usb power supply current limit */
	struct power_supply *psy;
	u32 current_limit_ad;

	/* for pd runtime machine delay */
	u32 dp_configure_delay;

	int hpd_gpio;
	int auxp_gpio;
	int auxn_gpio;

	bool hpd_status;
	bool hpd_status_pre;

	enum typec_orientation orientation;
	unsigned long mode;

	/* *
	 * NOTE:
	 * External PHY mode:
	 * bit[1:0] - orientation,
	 * bit[5:2] - mode
	 */
	unsigned long phy_mode;
};

static const u32 switcher_cable[] = {
	EXTCON_DISP_DP,
	EXTCON_USB,
	EXTCON_NONE,
};


enum mux_hpd_state {
	MUX_HOUTPLUG_OUT = 0,
	MUX_HOUTPLUG_IN,
};

enum aux_pull_state {
	AUX_PULL_DOWN = 0,
	AUX_PULL_UP = 1,
};

static const char * const typec_orientation_name[] = {
	[TYPEC_ORIENTATION_NONE]		= "UNKNOW",
	[TYPEC_ORIENTATION_NORMAL]		= "NORMAL",
	[TYPEC_ORIENTATION_REVERSE]		= "REVERSE",
};

static const char * const typec_mode_name[] = {
	[TYPEC_STATE_SAFE]			= "STATE_SAFE",
	[TYPEC_STATE_USB]			= "STATE_USB",
	[TYPEC_DP_STATE_C]			= "STATE_DP_C",
	[TYPEC_DP_STATE_D]			= "STATE_DP_D",
	[TYPEC_DP_STATE_E]			= "STATE_DP_E",
	[TYPEC_DP_STATE_F]			= "STATE_DP_F",
};

static ssize_t
orientation_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_phy_switcher *phy_switcher = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", typec_orientation_name[phy_switcher->orientation]);
}

static ssize_t
mux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_phy_switcher *phy_switcher = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", typec_mode_name[phy_switcher->mode]);
}

static ssize_t
hotplug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_phy_switcher *phy_switcher = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n",
			    phy_switcher->hpd_status ? "Plugin" : "Plugout");
}

static DEVICE_ATTR_RO(orientation);
static DEVICE_ATTR_RO(mux);
static DEVICE_ATTR_RO(hotplug);

static struct attribute *phy_switcher_attrs[] = {
	&dev_attr_orientation.attr,
	&dev_attr_mux.attr,
	&dev_attr_hotplug.attr,
	NULL,
};

static const struct attribute_group phy_switcher_attr_group = {
	.name = "debug",
	.attrs = phy_switcher_attrs,
};


static const struct attribute_group *phy_switcher_groups[] = {
	&phy_switcher_attr_group,
	NULL,
};


static void phy_switcher_notify_hotplug_in(struct sunxi_phy_switcher *switcher)
{
	if (gpio_is_valid(switcher->hpd_gpio))
		gpio_set_value(switcher->hpd_gpio, MUX_HOUTPLUG_IN);
	else
		pr_warn("hotplug gpio for phy mux not config yet\n");

	if (switcher->extcon)
		extcon_set_state_sync(switcher->extcon, EXTCON_DISP_DP, 1);

	if (switcher->dp_phy) {
		phy_set_mode_ext(switcher->dp_phy, PHY_MODE_DP, switcher->phy_mode);
	}
}

static void phy_switcher_notify_hotplug_out(struct sunxi_phy_switcher *switcher)
{
	if (gpio_is_valid(switcher->hpd_gpio))
		gpio_set_value(switcher->hpd_gpio, MUX_HOUTPLUG_OUT);
	else
		pr_warn("hotplug gpio for phy mux not config yet\n");

	if (switcher->extcon)
		extcon_set_state_sync(switcher->extcon, EXTCON_DISP_DP, 0);

	if (switcher->dp_phy) {
		phy_set_mode_ext(switcher->dp_phy, PHY_MODE_DP, switcher->phy_mode);
	}
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
static int sunxi_phy_sw_set(struct typec_switch *sw,
			      enum typec_orientation orientation)
#else
static int sunxi_phy_sw_set(struct typec_switch_dev *sw,
			      enum typec_orientation orientation)
#endif
{
	struct sunxi_phy_switcher *phy_switcher = typec_switch_get_drvdata(sw);

	mutex_lock(&phy_switcher->lock);

	switch (orientation) {
	case TYPEC_ORIENTATION_NONE:
		/* reset default aux pull up/down */
		if (gpio_is_valid(phy_switcher->auxp_gpio))
			gpio_set_value(phy_switcher->auxp_gpio, AUX_PULL_DOWN);

		if (gpio_is_valid(phy_switcher->auxn_gpio))
			gpio_set_value(phy_switcher->auxn_gpio, AUX_PULL_UP);
		break;
	case TYPEC_ORIENTATION_NORMAL:
		/* set default aux pull up/down */
		if (gpio_is_valid(phy_switcher->auxp_gpio))
			gpio_set_value(phy_switcher->auxp_gpio, AUX_PULL_DOWN);

		if (gpio_is_valid(phy_switcher->auxn_gpio))
			gpio_set_value(phy_switcher->auxn_gpio, AUX_PULL_UP);
		break;
	case TYPEC_ORIENTATION_REVERSE:
		/* invert aux pull up/down */
		if (gpio_is_valid(phy_switcher->auxp_gpio))
			gpio_set_value(phy_switcher->auxp_gpio, AUX_PULL_UP);

		if (gpio_is_valid(phy_switcher->auxn_gpio))
			gpio_set_value(phy_switcher->auxn_gpio, AUX_PULL_DOWN);
		break;
	}
	phy_switcher->phy_mode = orientation | (phy_switcher->mode << 2) \
				 | (phy_switcher->hpd_status ? (MUX_HOUTPLUG_IN << 6) : (MUX_HOUTPLUG_OUT << 6));

	dev_info(phy_switcher->dev, "orientation:%d, set sw conf %s -> %s\n",
		 orientation, typec_orientation_name[phy_switcher->orientation],
		 typec_orientation_name[orientation]);

	if (phy_switcher->orientation != orientation) {
		/* phy mode is useless because modes are determined in dts already,
		* we only consider submode*/
		if (phy_switcher->usb_phy)
			phy_set_mode_ext(phy_switcher->usb_phy, PHY_MODE_USB_OTG, phy_switcher->phy_mode);

		/* invert aux lane */
		if (phy_switcher->dp_phy)
			phy_set_mode_ext(phy_switcher->dp_phy, PHY_MODE_DP, phy_switcher->phy_mode);

		phy_switcher->orientation = orientation;
	}

	mutex_unlock(&phy_switcher->lock);

	return 0;
}

static int
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
sunxi_phy_mux_set(struct typec_mux *mux, struct typec_mux_state *state)
#else
sunxi_phy_mux_set(struct typec_mux_dev *mux, struct typec_mux_state *state)
#endif
{
	struct sunxi_phy_switcher *phy_switcher = typec_mux_get_drvdata(mux);
	bool current_limit_restore = false;
	bool dp_mode = false;
	union power_supply_propval val = {0};
	struct typec_displayport_data *data = state->data;

	mutex_lock(&phy_switcher->lock);

	switch (state->mode) {
	case TYPEC_STATE_SAFE:
		phy_switcher->hpd_status = false;
		break;
	case TYPEC_STATE_USB:
		phy_switcher->hpd_status = false;
		break;
	case TYPEC_DP_STATE_C:
	case TYPEC_DP_STATE_E:
	case TYPEC_DP_STATE_D:
	case TYPEC_DP_STATE_F:
		dp_mode = true;
		break;
	default:
		break;
	}

	/* only alt dp mode has this data */
	if (data) {
		/*
		 * delay some time before trigger a DP_CMD_CONFIGURE vdm,
		 * to ensure GET_SINK_CAPABILITY FINISH, otherwise would make
		 * DP_CMD_CONFIGURE block because of  vdm_state change from
		 * SNK_READY to GET_SINK_CAP
		 */
		if ((data->conf & DP_CONF_SIGNALING_DP) &&
		    (state->mode == TYPEC_STATE_SAFE) &&
		    (phy_switcher->dp_configure_delay)) {
			mdelay(phy_switcher->dp_configure_delay);
		}

		/* altmode displayport plugin */
		if ((data->conf & DP_CONF_SIGNALING_DP) && (data->status & DP_STATUS_HPD_STATE))
			phy_switcher->hpd_status = true;
		else
			phy_switcher->hpd_status = false;
	}

	phy_switcher->phy_mode = phy_switcher->orientation | (state->mode << 2) \
				 | (phy_switcher->hpd_status ? (MUX_HOUTPLUG_IN << 6) : (MUX_HOUTPLUG_OUT << 6));

	dev_info(phy_switcher->dev, "conf:0x%x status:0x%x mode:%ld, set mux state %s -> %s\n",
		 data ? data->conf : 0, data ? data->status : 0, state->mode,
		 typec_mode_name[phy_switcher->mode], typec_mode_name[state->mode]);

	if (phy_switcher->mode != state->mode) {
		/* typec plug out from DP altmode, reset current limit */
		if ((phy_switcher->mode == TYPEC_DP_STATE_C ||
			phy_switcher->mode == TYPEC_DP_STATE_D ||
			phy_switcher->mode == TYPEC_DP_STATE_E ||
			phy_switcher->mode == TYPEC_DP_STATE_F) &&
			state->mode == TYPEC_STATE_SAFE) {

			current_limit_restore = true;
		}

		if (phy_switcher->psy) {
			if (dp_mode == true) {
				/*
				 * limit input current to 500mA to compatible most panel,
				 * avoid overload lead to displayport's usb power's drop
				 */
				val.intval = 500; /* mA */
				power_supply_set_property(phy_switcher->psy,
					POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &val);
			} else if (current_limit_restore == true) {
				val.intval = phy_switcher->current_limit_ad; /* mA */
				power_supply_set_property(phy_switcher->psy,
					POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &val);
			}
		}

		/* phy mode is useless because modes are determined in dts already,
		* we only consider submode*/
		if (dp_mode) {
			if (phy_switcher->dp_phy)
				phy_set_mode_ext(phy_switcher->dp_phy, PHY_MODE_DP, phy_switcher->phy_mode);
		} else {
			if (phy_switcher->usb_phy)
				phy_set_mode_ext(phy_switcher->usb_phy, PHY_MODE_USB_OTG, phy_switcher->phy_mode);
		}

		phy_switcher->mode = state->mode;
	}

	if (phy_switcher->hpd_status != phy_switcher->hpd_status_pre) {
		if (phy_switcher->hpd_status)
			phy_switcher_notify_hotplug_in(phy_switcher);
		else
			phy_switcher_notify_hotplug_out(phy_switcher);
	}

	phy_switcher->hpd_status_pre = phy_switcher->hpd_status;


	mutex_unlock(&phy_switcher->lock);

	return 0;
}

static int sunxi_phy_switcher_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct typec_switch_desc sw_desc = { };
	struct typec_mux_desc mux_desc = { };
	struct sunxi_phy_switcher *phy_switcher;
	struct device_node *node = dev->of_node;
	struct power_supply *psy = NULL;
	struct device_node *np = NULL;
	int ret;
	char gpio_name[20];

	phy_switcher = devm_kzalloc(dev, sizeof(*phy_switcher), GFP_KERNEL);
	if (!phy_switcher)
		return -ENOMEM;

	mutex_init(&phy_switcher->lock);

	phy_switcher->dev = dev;
	dev_set_drvdata(dev, phy_switcher);

	sprintf(gpio_name, "hotplug");
	phy_switcher->hpd_gpio = of_get_named_gpio(node, gpio_name, 0);
	if (!gpio_is_valid(phy_switcher->hpd_gpio)) {
		pr_warn("get hotplug pin for phy mux failed, hotplug may be useless!\n");
	} else {
		/* init hotplug state to plugout */
		devm_gpio_request(dev, phy_switcher->hpd_gpio, gpio_name);
		gpio_direction_output(phy_switcher->hpd_gpio, MUX_HOUTPLUG_OUT);
	}

	/* default manaual external pull up/down, auxp pull down and auxn pull up*/
	sprintf(gpio_name, "aux_p");
	phy_switcher->auxp_gpio = of_get_named_gpio(node, gpio_name, 0);
	if (!gpio_is_valid(phy_switcher->auxp_gpio)) {
		pr_warn("get aux_p pull external up/down pin for phy mux failed, may be useless!\n");
	} else {
		/* init hotplug state to plugout */
		devm_gpio_request(dev, phy_switcher->auxp_gpio, gpio_name);
		gpio_direction_output(phy_switcher->auxp_gpio, AUX_PULL_DOWN);
	}

	sprintf(gpio_name, "aux_n");
	phy_switcher->auxn_gpio = of_get_named_gpio(node, gpio_name, 0);
	if (!gpio_is_valid(phy_switcher->auxn_gpio)) {
		pr_warn("get aux_n pull external up/down pin for phy mux failed, may be useless!\n");
	} else {
		/* init hotplug state to plugout */
		devm_gpio_request(dev, phy_switcher->auxn_gpio, gpio_name);
		gpio_direction_output(phy_switcher->auxn_gpio, AUX_PULL_UP);
	}


	phy_switcher->dp_phy = devm_phy_get(dev, "dp-phy");
	if (IS_ERR_OR_NULL(phy_switcher->dp_phy)) {
		dev_err(dev, "fail to get dp phy for mux, driver probe fail!\n");
		return PTR_ERR(phy_switcher->dp_phy);
	}

	phy_switcher->usb_phy = devm_phy_get(dev, "usb-phy");
	if (IS_ERR_OR_NULL(phy_switcher->usb_phy)) {
		dev_warn(dev, "fail to get usb phy for mux, driver probe fail!\n");
		return PTR_ERR(phy_switcher->dp_phy);
	}

	phy_switcher->extcon = devm_extcon_dev_allocate(phy_switcher->dev, switcher_cable);
	if (IS_ERR_OR_NULL(phy_switcher->extcon)) {
		dev_err(dev, "devm_extcon_dev_allocate fail\n");
		return PTR_ERR(phy_switcher->extcon);
	}
	devm_extcon_dev_register(phy_switcher->dev, phy_switcher->extcon);

	if (of_find_property(node, "usb_psy", NULL)) {
		psy = devm_power_supply_get_by_phandle(phy_switcher->dev, "usb_psy");
		if (IS_ERR_OR_NULL(psy)) {
			phy_switcher->psy = NULL;
			dev_err(phy_switcher->dev, "usb_psy is not found, maybe useless!\n");
		} else {
			phy_switcher->psy = psy;
			np = of_parse_phandle(node, "usb_psy", 0);
			if (np)
				of_property_read_u32(np, "pmu_usbad_cur", &phy_switcher->current_limit_ad);
			else
				phy_switcher->current_limit_ad = 1500;
		}
	}

	/* parse delay time before DP_CMD_CONFIGURE */
	ret = of_property_read_u32(node, "dp_configure_delay", &phy_switcher->dp_configure_delay);
	if (ret)
		phy_switcher->dp_configure_delay = 0;

	sw_desc.drvdata = phy_switcher;
	sw_desc.fwnode = dev->fwnode;
	sw_desc.set = sunxi_phy_sw_set;

	phy_switcher->sw = typec_switch_register(dev, &sw_desc);
	if (IS_ERR(phy_switcher->sw)) {
		dev_err(dev, "Error registering typec switch: %ld\n",
			PTR_ERR(phy_switcher->sw));
		return PTR_ERR(phy_switcher->sw);
	}

	mux_desc.drvdata = phy_switcher;
	mux_desc.fwnode = dev->fwnode;
	mux_desc.set = sunxi_phy_mux_set;

	phy_switcher->mux = typec_mux_register(dev, &mux_desc);
	if (IS_ERR(phy_switcher->mux)) {
		typec_switch_unregister(phy_switcher->sw);
		dev_err(dev, "Error registering typec mux: %ld\n",
			PTR_ERR(phy_switcher->mux));
		return PTR_ERR(phy_switcher->mux);
	}

	phy_switcher->hpd_status = false;
	phy_switcher->hpd_status_pre = false;
	phy_switcher->mode = TYPEC_STATE_USB;
	phy_switcher->orientation = TYPEC_ORIENTATION_NONE;

	dev_info(dev, "probe success\n");

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int sunxi_phy_switcher_remove(struct platform_device *pdev)
#else
static void sunxi_phy_switcher_remove(struct platform_device *pdev)
#endif
{
	struct sunxi_phy_switcher *phy_switcher = dev_get_drvdata(&pdev->dev);

	typec_mux_unregister(phy_switcher->mux);
	typec_switch_unregister(phy_switcher->sw);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	return 0;
#endif
}

static const struct of_device_id sunxi_phy_switcher_match[] = {
	{ .compatible = "allwinner,sunxi-phy-switcher", },
};

static struct platform_driver sunxi_phy_switcher_driver = {
	.probe = sunxi_phy_switcher_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	.remove = sunxi_phy_switcher_remove,
#else
	.remove_new = sunxi_phy_switcher_remove,
#endif
	.driver = {
		.name = "switcher",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_phy_switcher_match,
		.dev_groups = phy_switcher_groups,
	},
};

module_platform_driver(sunxi_phy_switcher_driver);

MODULE_ALIAS("platform:sunxi-phy-switch-driver");
MODULE_DESCRIPTION("Allwinner Type-C switch driver base on phy");
MODULE_AUTHOR("huangyongxing<huangyongxing@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");

// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, version 2.
//
//	This program is distributed in the hope that it will be useful, but
//	WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//	General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------
#include <drm/drm_atomic_helper.h>
#include <drm/drm_device.h>
//#include <drm/display/drm_dp_helper.h>
#include <drm/drm_of.h>
//#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <asm/types.h>
#include <linux/component.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_dptx.h"
#include "trilin_phy.h"
#include "cix_edp_panel.h"
#include <linux/pinctrl/consumer.h>

struct trilin_dptx_pdata
{
	int eDP;
};

struct trilin_dptx_cix_dev {
	struct device *dev;
	struct drm_encoder encoder;
	struct trilin_dptx *dptx;
	struct trilin_dpsub dpsub;
	struct gpio_desc *pdb_gpiod;
	bool dp_to_hdmi;
};

/* defined in dptx bridge driver */
static struct trilin_dptx_pdata cix_sky1_pdata = {
	/* TODO */
};

static const struct of_device_id trilin_dptx_dt_ids[] = {
	{
		.compatible = "cix,sky1-dptx",
		.data = &cix_sky1_pdata,
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, trilin_dptx_dt_ids);

static const struct acpi_device_id trilin_dptx_acpi_ids[] = {
	{
		.id = "CIXH502F",
		.driver_data = (kernel_ulong_t)&cix_sky1_pdata,
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, trilin_dptx_acpi_ids);

static uint32_t drm_acpi_crtc_port_mask(struct drm_device *dev,
			    struct fwnode_handle *port)
{
	unsigned int index = 0;
	struct drm_crtc *tmp;

	drm_for_each_crtc(tmp, dev) {
		if ((struct fwnode_handle *)tmp->port == port) {
			pr_info("cix_virtual.drm_acpi_crtc_port_mask, port=%s\n",
				port->ops->get_name(port));
			return 1 << index;
		}

		index++;
	}

	return 0;
}

static uint32_t drm_acpi_find_possible_crtcs(struct drm_device *dev,
				    struct fwnode_handle *port)
{
	struct fwnode_handle *remote_port, *ep;
	uint32_t possible_crtcs = 0;

	fwnode_graph_for_each_endpoint(port, ep) {
		remote_port = fwnode_graph_get_remote_port(ep);
		if (!remote_port) {
			fwnode_handle_put(ep);
			return 0;
		}

		possible_crtcs |= drm_acpi_crtc_port_mask(dev, remote_port);

		fwnode_handle_put(remote_port);
	}

	return possible_crtcs;
}

static int trilin_dptx_cix_bind(struct device *comp, struct device *master,
				void *master_data)
{
	void *np;
	const void *match;
	struct drm_device *drm = master_data;
	struct trilin_dptx_pdata *pdata;
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(comp);
	struct drm_encoder *encoder;
	struct trilin_dpsub *dpsub;
	int ret = 0;

	cix_dptx->dev = comp;
	encoder = &cix_dptx->encoder;
	if (has_acpi_companion(comp)) {
		np = comp->fwnode;
		encoder->possible_crtcs = drm_acpi_find_possible_crtcs(drm, np);
	} else {
		np = comp->of_node;
		encoder->possible_crtcs = drm_of_find_possible_crtcs(drm, np);
	}

	if (!np)
		return -ENODEV;

	if (has_acpi_companion(comp)) {
		match = acpi_device_get_match_data(comp);
		pdata = (struct trilin_dptx_pdata *)(((struct acpi_device_id *)match)->driver_data);
	} else {
		match = of_match_node(trilin_dptx_dt_ids, np);
		pdata = (struct trilin_dptx_pdata *)(((struct of_device_id *)match)->data);
	}

	if (unlikely(!match))
		return -ENODEV;

	if (!encoder->possible_crtcs)
		return -EPROBE_DEFER;

	dpsub = &cix_dptx->dpsub;
	dpsub->dev = comp;

	ret = trilin_dp_probe(dpsub, drm);
	if (ret)
		return ret;

	ret = trilin_dp_drm_init(dpsub);
	if (ret)
		return ret;

	dpsub->link = device_link_add(drm->dev, comp, DL_FLAG_STATELESS);

	return 0;
}

static void trilin_dptx_cix_unbind(struct device *comp, struct device *master,
				   void *master_data)
{
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(comp);
	struct trilin_dpsub *dpsub;

	dpsub = &cix_dptx->dpsub;

	trilin_dp_hdcp_uninit(dpsub);

	trilin_dp_remove(dpsub);

	device_link_del(dpsub->link);
}

static const struct component_ops trilin_dptx_cix_ops = {
	.bind	= trilin_dptx_cix_bind,
	.unbind	= trilin_dptx_cix_unbind,
};

static int trilin_dptx_cix_probe(struct platform_device *pdev)
{
	struct trilin_dptx_cix_dev *dptx_dev;
	const char *str_prop;
	struct device *dev;
	int ret;

	dev = &pdev->dev;
	dptx_dev = devm_kzalloc(&pdev->dev, sizeof(*dptx_dev), GFP_KERNEL);
	if (!dptx_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, dptx_dev);

	ret = of_property_read_string(dev->of_node, "dp_to_hdmi", &str_prop);
	if (ret >= 0) {
		if (strcmp("yes",str_prop) == 0) {
			dptx_dev->pdb_gpiod = devm_gpiod_get_optional(&pdev->dev, "pdb", GPIOD_OUT_HIGH);
			if (IS_ERR(dptx_dev->pdb_gpiod)) {
				dev_dbg(&pdev->dev, "failed to pdb gpio ...\n");
			} else {
				msleep(10);
			}
		        dptx_dev->dp_to_hdmi = true;
		} else {
		        dptx_dev->dp_to_hdmi = false;
		}
	}

	return component_add(&pdev->dev, &trilin_dptx_cix_ops);
}

static int trilin_dptx_cix_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &trilin_dptx_cix_ops);

	return 0;
}

#ifdef CONFIG_PM
static int trilin_dptx_pm_suspend(struct device *dev)
{
	/* TODO */
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(dev);
	struct trilin_dpsub *dpsub = &cix_dptx->dpsub;
	struct trilin_dp *dp = dpsub->dp;

	if (cix_dptx->dp_to_hdmi) {
		if (!IS_ERR(cix_dptx->pdb_gpiod)) {
			gpiod_set_value(cix_dptx->pdb_gpiod, 0);
		}

		pinctrl_pm_select_sleep_state(dev);
	}

	DP_INFO("enter!");
	if (dp)
		return trilin_dp_pm_prepare(dp);
	return 0;
}

static int trilin_dptx_pm_resume(struct device *dev)
{
	/* TODO */
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(dev);
	struct trilin_dpsub *dpsub = &cix_dptx->dpsub;
	struct trilin_dp *dp = dpsub->dp;

	if (cix_dptx->dp_to_hdmi) {
		if (!IS_ERR(cix_dptx->pdb_gpiod)) {
			gpiod_set_value(cix_dptx->pdb_gpiod, 1);
			msleep(10);
		}

		pinctrl_pm_select_default_state(dev);
	}

	DP_INFO("enter!");
	if (dp)
		return trilin_dp_pm_complete(dp);
	return 0;
}

static const struct dev_pm_ops trilin_dptx_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(trilin_dptx_pm_suspend,
			trilin_dptx_pm_resume)
	//SET_RUNTIME_PM_OPS(trilin_dptx_runtime_suspend,
	//		trilin_dptx_runtime_resume, NULL)
	//.prepare = trilin_dptx_pm_prepare,
	//.complete = trilin_dptx_pm_complete,
};
#endif

static struct platform_driver trilin_dp_driver = {
	.probe  = trilin_dptx_cix_probe,
	.remove = trilin_dptx_cix_remove,
	.driver = {
		.name = "trilin-dptx-cix",
		.of_match_table = trilin_dptx_dt_ids,
		.acpi_match_table = ACPI_PTR(trilin_dptx_acpi_ids),
	#ifdef CONFIG_PM
		.pm = &trilin_dptx_pm_ops,
	#endif
	},
};

static int __init trilin_dp_driver_init(void)
{
       int err;

       err = cix_edp_panel_init();
       if (err < 0)
               return err;

       err = platform_driver_register(&trilin_dp_driver);
       if (err < 0)
               return err;

       return 0;
}
module_init(trilin_dp_driver_init);

static void trilin_dp_driver_exit(void)
{
       platform_driver_unregister(&trilin_dp_driver);
       cix_edp_panel_exit();
}
module_exit(trilin_dp_driver_exit);

MODULE_AUTHOR("Fei Mao <fei.mao@cixtech.com>");
MODULE_DESCRIPTION("Cix Platforms DP Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:trilin-dptx-cix");

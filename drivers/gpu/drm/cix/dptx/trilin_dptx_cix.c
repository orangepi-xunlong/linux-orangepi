// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//	Copyright 2024 Cix Technology Group Co., Ltd.
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
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_drv.h>

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
#include <linux/of_device.h>
#include <linux/module.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_dptx.h"
#include "trilin_phy.h"
#include "cix_edp_panel.h"

struct trilin_dptx_pdata {
	int eDP;
};

struct trilin_dptx_cix_dev {
	struct device *dev;
	struct drm_encoder encoder;
	struct trilin_dptx *dptx;
	struct trilin_dpsub dpsub;
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

	fwnode_graph_for_each_endpoint (port, ep) {
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

	if (!encoder->possible_crtcs)
		return -EPROBE_DEFER;

	if (!np)
		return -ENODEV;

	if (has_acpi_companion(comp)) {
		match = acpi_device_get_match_data(comp);
		pdata = (struct trilin_dptx_pdata *)(((struct acpi_device_id *)
							      match)
							     ->driver_data);
	} else {
		match = of_match_node(trilin_dptx_dt_ids, np);
		pdata = (struct trilin_dptx_pdata
				 *)(((struct of_device_id *)match)->data);
	}

	if (unlikely(!match))
		return -ENODEV;

	dpsub = &cix_dptx->dpsub;
	dpsub->dev = comp;

	ret = trilin_dp_probe(dpsub, drm);
	if (ret)
		return ret;

	ret = trilin_dp_drm_init(dpsub);
	if (ret)
		return ret;

	// dpsub->link = device_link_add(drm->dev, comp, DL_FLAG_STATELESS |
	// 				DL_FLAG_PM_RUNTIME | DL_FLAG_RPM_ACTIVE);

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
	.bind = trilin_dptx_cix_bind,
	.unbind = trilin_dptx_cix_unbind,
};

struct linlondp_drv {
	void *mdev;
	void *kms;
};

static int trilin_dptx_cix_probe(struct platform_device *pdev)
{
	struct trilin_dptx_cix_dev *dptx_dev;

	dptx_dev = devm_kzalloc(&pdev->dev, sizeof(*dptx_dev), GFP_KERNEL);
	if (!dptx_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, dptx_dev);

#if !IS_ENABLED(CONFIG_DRM_CIX_COMPONENT_BIND_BYPASSED)
	return component_add(&pdev->dev, &trilin_dptx_cix_ops);
#else
	struct device_node *ports_node, *port_node;
	int i = 0, ret = 0, j = 0;
	struct device_node *remote_node;
	struct platform_device *dpu_pdev;
	struct device *master_dpu_dev_0 = NULL;
	struct device *master_dpu_dev_1 = NULL;
	struct drm_device *drm;

	ports_node = of_get_child_by_name(pdev->dev.of_node, "ports");

	if (!ports_node) {
		dev_err(&pdev->dev, "Device A has no 'ports' node");
		return -ENOMEM;
	}

	for_each_child_of_node(ports_node, port_node) {
		struct device_node *ep_node = of_get_next_child(port_node, NULL);

		if (!ep_node)
			continue;

		struct device_node *remote_ep = of_parse_phandle(ep_node, "remote-endpoint", 0);

		of_node_put(ep_node);
		if (!remote_ep)
			continue;

		struct device_node *port_b = of_get_parent(remote_ep);

		if (!port_b) {
			dev_err(&pdev->dev, "No port parent for endpoint");
			of_node_put(remote_ep);
			continue;
		}

		struct device_node *pipeline_b = of_get_parent(port_b);

		of_node_put(port_b);
		if (!pipeline_b) {
			dev_err(&pdev->dev, "No pipeline parent for port");
			of_node_put(remote_ep);
			continue;
		}

		struct device_node *dev_b_node = of_get_parent(pipeline_b);

		of_node_put(pipeline_b);
		if (!dev_b_node) {
			dev_err(&pdev->dev, "No device_b node for pipeline");
			of_node_put(remote_ep);
			continue;
		}

		dev_dbg(&pdev->dev, "Found Device : %s", dev_b_node->full_name);

		dpu_pdev = of_find_device_by_node(dev_b_node);
		of_node_put(dev_b_node);
		of_node_put(remote_ep);

		struct linlondp_drv *drv_data = platform_get_drvdata(dpu_pdev);

		while (!drv_data) {
			msleep(20);
			if (j++ > 100) {
				dev_err(&pdev->dev, "Timeout: Wait too long (%d) for linlondp ko ready", 20*j);
				return -ETIMEDOUT;
			}
			drv_data = platform_get_drvdata(dpu_pdev);
		}

		dev_dbg(&pdev->dev, "Wait %d ms for drm data ready.", 20*j);

		if (master_dpu_dev_0 == &dpu_pdev->dev
			|| master_dpu_dev_1 == &dpu_pdev->dev) {
			dev_dbg(&pdev->dev, "Ignore the second master dpu");
			continue;
		}

		if (i == 0)
			master_dpu_dev_0 = &dpu_pdev->dev;
		else if (i == 1)
			master_dpu_dev_1 = &dpu_pdev->dev;

		drm = drv_data->kms;
		dptx_dev->dpsub.drm[i++] = drm;
	}

	of_node_put(ports_node);

	ret = trilin_dptx_cix_bind(&pdev->dev, master_dpu_dev_0, dptx_dev->dpsub.drm[0]);
	if (ret < 0) {
		dev_err(&pdev->dev, "trilin_dptx_cix_bind failed...");
		return ret;
	}

	for (j = 0; j < i; j++) {
		drm_mode_config_reset(dptx_dev->dpsub.drm[j]);
		drm_kms_helper_poll_init(dptx_dev->dpsub.drm[j]);
		ret = drm_dev_register(dptx_dev->dpsub.drm[j], 0);
		dev_dbg(&pdev->dev, "register drm%d %s", j, !ret ? "success" : "failed");
	}
	return 0;
#endif
}

static int trilin_dptx_cix_remove(struct platform_device *pdev)
{
#if !IS_ENABLED(CONFIG_DRM_CIX_COMPONENT_BIND_BYPASSED)
	component_del(&pdev->dev, &trilin_dptx_cix_ops);
#else
	trilin_dptx_cix_unbind(&pdev->dev, NULL, NULL);
#endif
	return 0;
}

#ifdef CONFIG_PM
static int trilin_dptx_pm_prepare(struct device *dev)
{
	/* TODO */
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(dev);
	struct trilin_dpsub *dpsub = &cix_dptx->dpsub;
	struct trilin_dp *dp = dpsub->dp;

	if (dp)
		return trilin_dp_pm_prepare(dp);
	return 0;
}


static int trilin_dptx_pm_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int trilin_dptx_pm_resume_early(struct device *dev)
{
	/* TODO */
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(dev);
	struct trilin_dpsub *dpsub = &cix_dptx->dpsub;
	struct trilin_dp *dp = dpsub->dp;

	if (dp)
		return trilin_dp_pm_resume_early(dp);
	return 0;
}

static int trilin_dptx_pm_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static void trilin_dptx_pm_complete(struct device *dev)
{
	/* TODO */
	struct trilin_dptx_cix_dev *cix_dptx = dev_get_drvdata(dev);
	struct trilin_dpsub *dpsub = &cix_dptx->dpsub;
	struct trilin_dp *dp = dpsub->dp;

	trilin_dp_pm_complete(dp);
}

static const struct dev_pm_ops trilin_dptx_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(trilin_dptx_pm_suspend, trilin_dptx_pm_resume)
	//SET_RUNTIME_PM_OPS(trilin_dptx_runtime_suspend,
	//		trilin_dptx_runtime_resume, NULL)
	.prepare = trilin_dptx_pm_prepare,
	.complete = trilin_dptx_pm_complete,
	.resume_early = trilin_dptx_pm_resume_early,
};
#endif

static void trilin_dptx_cix_shutdown(struct platform_device *pdev)
{
	trilin_dptx_pm_suspend(&pdev->dev);
}

static struct platform_driver trilin_dp_driver = {
	.probe  = trilin_dptx_cix_probe,
	.remove = trilin_dptx_cix_remove,
	.shutdown = trilin_dptx_cix_shutdown,
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
#if IS_ENABLED(CONFIG_DRM_CIX_COMPONENT_BIND_BYPASSED)
MODULE_SOFTDEP("pre: linlon_dp");
#endif
MODULE_ALIAS("platform:trilin-dptx-cix");

// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/pm_runtime.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_module.h>
#include <drm/drm_of.h>
#include <drm/drm_fbdev_generic.h>
#include "linlondp_dev.h"
#include "linlondp_kms.h"

static bool enable_fb = true;
module_param_named(enable_fb, enable_fb, bool, 0644);
MODULE_PARM_DESC(enable_fb, "Enable/Disable drm framebuffer support");

struct linlondp_drv {
	struct linlondp_dev *mdev;
	struct linlondp_kms_dev *kms;
};

struct linlondp_dev *dev_to_mdev(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);

	return mdrv ? mdrv->mdev : NULL;
}

static void linlondp_unbind(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);

	if (!mdrv)
		return;

	linlondp_kms_detach(mdrv->kms);

	if (pm_runtime_enabled(dev))
		pm_runtime_disable(dev);
	else
		linlondp_dev_suspend(mdrv->mdev);

	linlondp_dev_destroy(mdrv->mdev);

	dev_set_drvdata(dev, NULL);
	devm_kfree(dev, mdrv);
}

static int linlondp_bind(struct device *dev)
{
	struct linlondp_drv *mdrv;
	int err;

	mdrv = devm_kzalloc(dev, sizeof(*mdrv), GFP_KERNEL);
	if (!mdrv)
		return -ENOMEM;

	pm_runtime_enable(dev);
	mdrv->mdev = linlondp_dev_create(dev);
	if (IS_ERR(mdrv->mdev)) {
		err = PTR_ERR(mdrv->mdev);
		goto free_mdrv;
	}

	if (!pm_runtime_enabled(dev))
		linlondp_dev_resume(mdrv->mdev);

	mdrv->kms = linlondp_kms_attach(mdrv->mdev);
	if (IS_ERR(mdrv->kms)) {
		err = PTR_ERR(mdrv->kms);
		goto destroy_mdev;
	}

	dev_set_drvdata(dev, mdrv);
	if (enable_fb)
		drm_fbdev_generic_setup(&mdrv->kms->base, 32);

	if (mdrv->mdev->enabled_by_gop)
		pm_runtime_set_active(dev);

	return 0;

destroy_mdev:
	if (pm_runtime_enabled(dev))
		pm_runtime_disable(dev);
	else
		linlondp_dev_suspend(mdrv->mdev);

	linlondp_dev_destroy(mdrv->mdev);

free_mdrv:
	devm_kfree(dev, mdrv);
	return err;
}

static const struct component_master_ops linlondp_master_ops = {
	.bind = linlondp_bind,
	.unbind = linlondp_unbind,
};

static int compare_of(struct device *dev, void *data)
{
	int ret;

	if (has_acpi_companion(dev)) {
		ret = dev->fwnode == data;
	} else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		ret = component_compare_of(dev, data);
#else
		ret = dev->of_node == data;
#endif
	}

	return ret;
}

static void drm_release_fwnode(struct device *dev, void *data)
{
	fwnode_handle_put(data);
}

static void linlondp_add_acpi_slave(struct device *master,
				    struct component_match **match,
				    struct fwnode_handle *np, u32 port,
				    u32 endpoint)
{
	struct fwnode_handle *remote;

	remote = fwnode_graph_get_remote_node(np, port, endpoint);

	if (remote) {
		pr_info("%s. remote.name=%s\n", __func__, dev_name(remote->dev));
		fwnode_handle_get(remote);
		component_match_add_release(master, match, drm_release_fwnode,
					    compare_of, remote);
		fwnode_handle_put(remote);
	}
}

static void linlondp_add_slave(struct device *master,
			       struct component_match **match,
			       struct device_node *np, u32 port, u32 endpoint)
{
	struct device_node *remote;

	remote = of_graph_get_remote_node(np, port, endpoint);
	if (remote) {
		drm_of_component_match_add(master, match, compare_of, remote);
		of_node_put(remote);
	}
}

static int linlondp_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct component_match *match = NULL;
	struct fwnode_handle *acpi_child;
	struct device_node *of_child;
	const char *tmp_name = NULL;

	pr_info("%s enter. dev.name=%s\n", __func__, dev_name(dev));
	pr_info("linlondp enable fb is %d", enable_fb);
#if !IS_ENABLED(CONFIG_DRM_CIX_COMPONENT_BIND_BYPASSED)
	if (has_acpi_companion(dev)) {
		pr_info("%s via acpi.\n", __func__);
		fwnode_for_each_child_node(dev->fwnode, acpi_child) {
			tmp_name = acpi_child->ops->get_name(acpi_child);
			if (strncmp(tmp_name, "pipeline", 8))
				continue;

			/* add connector */
			pr_info("%s enter to add connector.\n", __func__);
			linlondp_add_acpi_slave(dev, &match, acpi_child,
						LINLONDP_OF_PORT_OUTPUT, 0);
			linlondp_add_acpi_slave(dev, &match, acpi_child,
						LINLONDP_OF_PORT_OUTPUT, 1);

			dev_pm_set_driver_flags(&pdev->dev, DPM_FLAG_NO_DIRECT_COMPLETE);
		}
	} else {
		pr_info("%s via dt.\n", __func__);
		for_each_available_child_of_node(dev->of_node, of_child) {
			if (of_node_cmp(of_child->name, "pipeline") != 0)
				continue;

			/* add connector */
			linlondp_add_slave(dev, &match, of_child,
					   LINLONDP_OF_PORT_OUTPUT, 0);
			linlondp_add_slave(dev, &match, of_child,
					   LINLONDP_OF_PORT_OUTPUT, 1);
		}
	}
	pr_info("%s end. match=%p\n", __func__, match);
	return component_master_add_with_match(dev, &linlondp_master_ops,
					       match);
#else
	return linlondp_bind(dev);
#endif
}

static int linlondp_platform_remove(struct platform_device *pdev)
{
#if !IS_ENABLED(CONFIG_DRM_CIX_COMPONENT_BIND_BYPASSED)
	component_master_del(&pdev->dev, &linlondp_master_ops);
#else
	linlondp_unbind(&pdev->dev);
#endif
	return 0;
}

static const struct of_device_id linlondp_of_match[] = {
	{
		.compatible = "armchina,linlon-d8",
		.data = dp_identify,
	},
	{
		.compatible = "armchina,linlon-d6",
		.data = dp_identify,
	},
	{
		.compatible = "armchina,linlon-d2",
		.data = dp_identify,
	},
	{},
};

MODULE_DEVICE_TABLE(of, linlondp_of_match);

static const struct acpi_device_id linlondp_acpi_match[] = {
	{
		.id = "CIXH5010",
		.driver_data = (kernel_ulong_t)dp_identify,
	},
	{},
};

MODULE_DEVICE_TABLE(acpi, linlondp_acpi_match);

static int __maybe_unused linlondp_rt_pm_suspend(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);

	if (mdrv)
		return linlondp_dev_suspend(mdrv->mdev);
	else
		return 0;
}

static int __maybe_unused linlondp_rt_pm_resume(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);

	if (mdrv)
		return linlondp_dev_resume(mdrv->mdev);
	else
		return 0;
}

static int __maybe_unused linlondp_pm_suspend(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);
	int res;

	dev_info(dev, "%s\n", __func__);

	if (!mdrv) {
		dev_info(dev, "%s, mdrv is null\n", __func__);
		return 0;
	}

	res = drm_mode_config_helper_suspend(&mdrv->kms->base);

	if (!pm_runtime_status_suspended(dev))
		linlondp_dev_suspend(mdrv->mdev);

	mdrv->mdev->enabled_by_gop = 0;

	return res;
}

static int __maybe_unused linlondp_pm_prepare(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	return 0;
}

static int __maybe_unused linlondp_pm_resume(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	if (!mdrv) {
		dev_info(dev, "%s, mdrv is null\n", __func__);
		return 0;
	}

	if (!pm_runtime_status_suspended(dev))
		linlondp_dev_resume(mdrv->mdev);

	return drm_mode_config_helper_resume(&mdrv->kms->base);
}

static int __maybe_unused linlondp_pm_restore(struct device *dev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(dev);

	of_property_read_u32(dev->of_node, "enabled_by_gop",
			     (u32 *)&mdrv->mdev->enabled_by_gop);

	if (!pm_runtime_status_suspended(dev))
		linlondp_dev_resume(mdrv->mdev);

	return drm_mode_config_helper_resume(&mdrv->kms->base);
}

static void linlondp_platform_shutdown(struct platform_device *pdev)
{
	struct linlondp_drv *mdrv = dev_get_drvdata(&pdev->dev);

	linlondp_pm_suspend(&pdev->dev);

	if (mdrv)
		mdrv->mdev->shutdown = true;
}

static const struct dev_pm_ops linlondp_pm_ops = {
	.suspend = linlondp_pm_suspend,
	.resume = linlondp_pm_resume,
	.prepare = linlondp_pm_prepare,
	.restore = linlondp_pm_restore,
	.freeze = linlondp_pm_suspend,
	.thaw = linlondp_pm_resume,
	SET_RUNTIME_PM_OPS(linlondp_rt_pm_suspend, linlondp_rt_pm_resume, NULL)
};

static struct platform_driver linlondp_platform_driver = {
	.probe = linlondp_platform_probe,
	.remove = linlondp_platform_remove,
	.shutdown = linlondp_platform_shutdown,
	.driver = {
		.name = "linlondp",
		.of_match_table = linlondp_of_match,
		.acpi_match_table = ACPI_PTR(linlondp_acpi_match),
		.pm = &linlondp_pm_ops,
	},
};

drm_module_platform_driver(linlondp_platform_driver);

MODULE_DESCRIPTION("Linlondp KMS driver");
MODULE_AUTHOR("ARMChina");
MODULE_LICENSE("GPL v2");

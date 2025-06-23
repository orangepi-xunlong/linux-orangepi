// SPDX-License-Identifier: GPL-2.0
/*
 * V4L2 Capture CSI Subdev for Cix sky SOC
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/videodev2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <linux/iopoll.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mipi-dphy.h>

#include "cdns-dphy-rx.h"
#include "csi_common.h"

#define CIX_MIPI_DPHY_RX_DRIVER_NAME	"cix-mipi-dphy"
#define CIX_MIPI_DPHY_RX_SUBDEV_NAME	CIX_MIPI_DPHY_RX_DRIVER_NAME
#define CIX_MIPI_DPHY_MAX_LANE 4

enum cix_dphy_pads {
	CIX_DPHY_PAD_SINK,
	CIX_DPHY_PAD_SOURCE,
	CIX_DPHY_PAD_MAX,
};

struct dphy_rx *v4l2_subdev_to_dphy_rx(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct dphy_rx, subdev);
}

static int cix_dphy_rx_get_sensor_data_rate(struct v4l2_subdev *sd)
{

	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);

	struct v4l2_subdev *sensor_sd = dphy->source_subdev;
	struct v4l2_ctrl *link_freq;
	struct v4l2_querymenu qm = { .id = V4L2_CID_LINK_FREQ, };
	int ret;

	if (!sensor_sd) {
		v4l2_warn(sd, "sensor subdev not register\n");
		return -EINVAL;
	}

	link_freq = v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_LINK_FREQ);
	if (!link_freq) {
		v4l2_warn(sd, "No pixel rate control in subdev\n");
		return -EPIPE;
	}

	qm.index = v4l2_ctrl_g_ctrl(link_freq);
	ret = v4l2_querymenu(sensor_sd->ctrl_handler, &qm);
	if (ret < 0) {
		v4l2_err(sd, "Failed to get menu item\n");
		return ret;
	}

	if (!qm.value) {
		v4l2_err(sd, "Invalid link_freq\n");
		return -EINVAL;
	}
	/********************TODO*********************************
	****************Convert frequency to Million**************
	*************Phy data_rate = LT7911 x 2*******************/
	dphy->data_rate = qm.value * 2;
	dphy->data_rate_mbps = dphy->data_rate/1000/1000;
	v4l2_info(sd, "dphy%d, data_rate_mbps %d\n",
		  dphy->id, dphy->data_rate_mbps);

	return 0;
}

static int cix_dphy_rx_get_sensor_fmt(struct dphy_rx *dphy)
{
	struct v4l2_mbus_framefmt *mf = &dphy->format;
	struct v4l2_subdev *remote_sd;
	struct v4l2_subdev_format src_fmt;
	int ret;

	/* Get remote source pad */
	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	memset(&src_fmt, 0, sizeof(src_fmt));
	src_fmt.pad = dphy->source_pad;
	src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(remote_sd, pad, get_fmt, NULL, &src_fmt);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		return -EINVAL;

	/* Update input frame size and formate  */
	memcpy(mf, &src_fmt.format, sizeof(struct v4l2_mbus_framefmt));

	dev_dbg(dphy->dev, "width=%d, height=%d, fmt.code=0x%x\n",
		mf->width, mf->height, mf->code);

	dev_dbg(dphy->dev, "format.reserved[0]=0x%x, format.reserved[1]=%x, \n",
			src_fmt.format.reserved[0], src_fmt.format.reserved[1]);

	return 0;
}

/* mipi csi2 subdev media entity operations */
static int mipi_dphy_rx_link_setup(struct media_entity *entity,
				const struct media_pad *local,
				const struct media_pad *remote, u32 flags)
{
	if (local->flags & MEDIA_PAD_FL_SOURCE) {

	} else if (local->flags & MEDIA_PAD_FL_SINK) {

	}

	return 0;
}

static const struct media_entity_operations mipi_dphy_rx_sd_media_ops = {
	.link_setup = mipi_dphy_rx_link_setup,
};

static int mipi_dphy_rx_s_power(struct v4l2_subdev *sd, int on)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_subdev *remote_sd;

	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	return v4l2_subdev_call(remote_sd, core, s_power, on);
}

static int mipi_dphy_rx_g_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_subdev *remote_sd;

	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	return v4l2_subdev_call(remote_sd, video, g_frame_interval, interval);
}

static int mipi_dphy_rx_s_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_subdev *remote_sd;

	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	return v4l2_subdev_call(remote_sd, video, s_frame_interval, interval);
}

static int mipi_dphy_rx_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	const struct dphy_hw_drv_data * hw_drv = NULL;

	dev_info(dphy->dev, "%s: %d, dphy:\n", __func__, enable);

	hw_drv = dphy->dphy_hw->drv_data;
	if (!hw_drv) {
		dev_info(dphy->dev,"dphy hardware attach failed \n");
	}

	if (enable) {
		pm_runtime_get_sync(dphy->dev);
		hw_drv->stream_on(dphy,dphy->id,dphy->data_rate);
	}
	else {
		hw_drv->stream_off(dphy,dphy->id);
		pm_runtime_put(dphy->dev);
	}

	return ret;
}

static int mipi_dphy_rx_enum_framesizes(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *cfg,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_subdev *remote_sd;

	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	return v4l2_subdev_call(remote_sd, pad, enum_frame_size, NULL, fse);
}

static int mipi_dphy_rx_enum_frame_interval(struct v4l2_subdev *sd,
					 struct v4l2_subdev_state *cfg,
					 struct v4l2_subdev_frame_interval_enum *fie)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_subdev *remote_sd;

	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	return v4l2_subdev_call(remote_sd, pad, enum_frame_interval, NULL, fie);
}

static int mipi_dphy_rx_get_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_format *fmt)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_mbus_framefmt *mf = &fmt->format;
	int ret;

	cix_dphy_rx_get_sensor_fmt(dphy);
	memcpy(mf, &dphy->format, sizeof(struct v4l2_mbus_framefmt));

	ret = cix_dphy_rx_get_sensor_data_rate(sd);
	if (ret < 0)
		return ret;
	mf->reserved[0] = dphy->data_rate_mbps;

	dev_info(dphy->dev, "format.reserved[0]=0x%x, format.reserved[1]=%x, \n",
			mf->reserved[0], mf->reserved[1]);
	return 0;
}

static int mipi_dphy_rx_set_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_format *fmt)
{
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(sd);
	struct v4l2_subdev *remote_sd;
	int ret;

	remote_sd = dphy->source_subdev;
	if (!remote_sd)
		return -EINVAL;

	fmt->pad = dphy->source_pad;
	ret = v4l2_subdev_call(remote_sd, pad, set_fmt, NULL, fmt);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		return ret;

	return 0;
}

static struct v4l2_subdev_pad_ops mipi_dphy_rx_pad_ops = {
	.enum_frame_size = mipi_dphy_rx_enum_framesizes,
	.enum_frame_interval = mipi_dphy_rx_enum_frame_interval,
	.get_fmt = mipi_dphy_rx_get_fmt,
	.set_fmt = mipi_dphy_rx_set_fmt,
};

static struct v4l2_subdev_core_ops mipi_dphy_rx_core_ops = {
	.s_power = mipi_dphy_rx_s_power,
};

static struct v4l2_subdev_video_ops mipi_dphy_rx_video_ops = {
	.g_frame_interval = mipi_dphy_rx_g_frame_interval,
	.s_frame_interval = mipi_dphy_rx_s_frame_interval,
	.s_stream	  = mipi_dphy_rx_s_stream,
};

static struct v4l2_subdev_ops mipi_dphy_rx_subdev_ops = {
	.core = &mipi_dphy_rx_core_ops,
	.video = &mipi_dphy_rx_video_ops,
	.pad = &mipi_dphy_rx_pad_ops,
};

static int mipi_dphy_rx_async_bound(struct v4l2_async_notifier *notifier,
			      struct v4l2_subdev *s_subdev,
			      struct v4l2_async_subdev *asd)
{
	struct v4l2_subdev *subdev = notifier->sd;
	struct dphy_rx *dphy = v4l2_subdev_to_dphy_rx(subdev);

	dev_info(dphy->dev, "dphy bound enter\n");

	/*find remote entity source pad */
	dphy->source_pad = media_entity_get_fwnode_pad(&s_subdev->entity,
							 s_subdev->fwnode,
							 MEDIA_PAD_FL_SOURCE);
	if (dphy->source_pad < 0) {
		dev_err(dphy->dev, "Couldn't find output pad for subdev %s\n",
			s_subdev->name);
		return dphy->source_pad;
	}

	dphy->source_subdev = s_subdev;
	dev_info(dphy->dev, "dphy bound source name %s index %d sink name %s index %d \n",
			s_subdev->name,dphy->source_pad,dphy->subdev.name,CIX_DPHY_PAD_SINK);

	return media_create_pad_link(&dphy->source_subdev->entity,
				     dphy->source_pad,
				     &dphy->subdev.entity,CIX_DPHY_PAD_SINK,
				     MEDIA_LNK_FL_ENABLED |
				     MEDIA_LNK_FL_IMMUTABLE);
}

static int dphy_parse_endpoint(struct device *dev,
                               struct v4l2_fwnode_endpoint *vep,
                               struct v4l2_async_subdev *asd)
{
	dev_info(dev, "dphy parse the endpoints\n");

	if (vep->base.port != 0) {
		dev_info(dev, "dphy do not need remote endpoints port %d id %d \n",vep->base.port,vep->base.id);
		return -ENOTCONN;
	}

	return 0;
}

static const struct v4l2_async_notifier_operations mipi_dphy_rx_notifier_ops = {
	.bound		= mipi_dphy_rx_async_bound,
};

static int dphy_media_init(struct dphy_rx *dphy)
{
	int ret;

	/* Create our media pads */
	dphy->pads[CIX_DPHY_PAD_SINK].flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	dphy->pads[CIX_DPHY_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE | MEDIA_PAD_FL_MUST_CONNECT;
	dphy->subdev.entity.function = MEDIA_ENT_F_VID_IF_BRIDGE;
	dphy->subdev.entity.ops = &mipi_dphy_rx_sd_media_ops;

	ret = media_entity_pads_init(&dphy->subdev.entity, CIX_DPHY_PAD_MAX,dphy->pads);

	return ret;
}

static struct cdns_dphy_rx * csi2_dphy_hw_attach(struct dphy_rx *dphy)
{
	struct platform_device *plat_dev;
	struct fwnode_handle *np;
	struct cdns_dphy_rx *dphy_hw;
	struct device *dev = dphy->dev;
	struct device *tdev = NULL;
	np = fwnode_find_reference(dev->fwnode, "cix,hw", 0);
	if (IS_ERR(np) || !fwnode_device_is_available(np)) {
		dev_err(dphy->dev,"failed to get dphy%d hw node\n", dphy->id);
		return NULL;
	}

	tdev = bus_find_device_by_fwnode(&platform_bus_type, np);
	plat_dev = tdev ? to_platform_device(tdev) : NULL;

	fwnode_handle_put(np);
	if (!plat_dev) {
		dev_err(dphy->dev,"failed to get dphy%d hw from node\n",dphy->id);
		return NULL;
	}

	dphy_hw = platform_get_drvdata(plat_dev);
	if (!dphy_hw) {
		dev_err(dphy->dev,"failed attach dphy%d hw\n",dphy->id);
		return NULL;
	}

	dev_err(dphy->dev,"attach dphy%d hardware success \n", dphy->id);

	return dphy_hw;
}

static int mipi_dphy_rx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dphy_rx *dphy;
	struct v4l2_subdev *sd;
	int ret;

	dev_info(dev, "mipi-dphy probe enter\n");

	dphy = devm_kzalloc(dev, sizeof(*dphy), GFP_KERNEL);
	if (!dphy)
		return -ENOMEM;

	dphy->dev = dev;
	dphy->pdev = pdev;

	dphy->dphy_hw = csi2_dphy_hw_attach(dphy);
	if (!dphy->dphy_hw) {
		dev_info(dev,"csi_dphy hardware attach failed\n");
		return -1;
	}

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u8(dev, CIX_MIPI_DPHY_OF_NODE_NAME, &dphy->id);
	} else {
		ret = dphy->id = of_alias_get_id(dev->of_node, CIX_MIPI_DPHY_OF_NODE_NAME);
	}
	if ((ret < 0) || (dphy->id >= CIX_MIPI_DPHY_RX_MAX_DEVS)) {
		dev_err(dev, "Invalid driver data or device id (%d)\n",
			dphy->id);
		return -EINVAL;
	}

	dev_info(dev,"virtual dphy id %d \n",dphy->id);

	/*init the subdev*/
	sd = &dphy->subdev;
	sd->dev = &pdev->dev;

	v4l2_subdev_init(sd, &mipi_dphy_rx_subdev_ops);
	v4l2_set_subdevdata(sd, dphy);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "%s.%d",
			CIX_MIPI_DPHY_RX_SUBDEV_NAME, dphy->id);
	/*init the media*/
	dphy_media_init(dphy);

	/*notifier & async subdev init*/
	v4l2_async_nf_init(&dphy->notifier);
	ret = v4l2_async_nf_parse_fwnode_endpoints(dphy->dev, &dphy->notifier,
			sizeof(struct v4l2_async_subdev),dphy_parse_endpoint);
	if (ret < 0) {
		dev_err(dev, "mipi-dphy async notifier parse endpoints failed \n");
		return ret;
	}

	dphy->notifier.ops = &mipi_dphy_rx_notifier_ops;
	ret = v4l2_async_subdev_nf_register(sd, &dphy->notifier);
	if (ret) {
		dev_err(dev, "mipi-dphy async register notifier failed \n");
		v4l2_async_nf_cleanup(&dphy->notifier);
		return ret;
	}

	ret = v4l2_async_register_subdev(sd);
	platform_set_drvdata(pdev, dphy);

	pm_runtime_enable(dev);

	dev_info(dev, "mipi-dphy probe exit %s \n",ret == 0 ? "success":"failed");

	return ret;
}

static int mipi_dphy_rx_remove(struct platform_device *pdev)
{
	struct dphy_rx *dphy = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &dphy->subdev;
	struct device *dev = &pdev->dev;

	dev_info(dev,"mipi_dphy_rx remove enter\n");

	media_entity_cleanup(&dphy->subdev.entity);
	v4l2_async_nf_unregister(&dphy->notifier);
	v4l2_async_nf_cleanup(&dphy->notifier);
	v4l2_async_unregister_subdev(sd);

	dev_info(dev,"mipi_dphy_rx remove exit \n");

	return 0;
}

static int mipi_dphy_dev_rpm_suspend(struct device *dev)
{
	struct dphy_rx *dphy = dev_get_drvdata(dev);
	const struct dphy_hw_drv_data * hw_drv = NULL;

	hw_drv = dphy->dphy_hw->drv_data;
	if (!hw_drv) {
		dev_info(dphy->dev,"dphy hardware attach failed \n");
	}

	hw_drv->dphy_hw_suspend(dphy);

	return 0;
}

static int mipi_dphy_dev_rpm_resume(struct device *dev)
{
	struct dphy_rx *dphy = dev_get_drvdata(dev);
	const struct dphy_hw_drv_data * hw_drv = NULL;

	hw_drv = dphy->dphy_hw->drv_data;
	if (!hw_drv) {
		dev_info(dphy->dev,"dphy hardware attach failed \n");
		return -1;
	}

	hw_drv->dphy_hw_resume(dphy);

	return 0;
}

static int mipi_dphy_dev_suspend(struct device *dev)
{
	dev_info(dev, "%s enter!\n", __func__);

	return pm_runtime_force_suspend(dev);
}

static int mipi_dphy_dev_resume(struct device *dev)
{
	struct dphy_rx *dphy = dev_get_drvdata(dev);
	const struct dphy_hw_drv_data * hw_drv = NULL;

	hw_drv = dphy->dphy_hw->drv_data;
	if (!hw_drv) {
		dev_info(dphy->dev,"dphy hardware attach failed \n");
		return -1;
	}

	dev_info(dev, "%s enter!\n", __func__);

	pm_runtime_force_resume(dev);

	hw_drv->stream_on(dphy,dphy->id,dphy->data_rate);

	return 0;
}

static const struct dev_pm_ops mipi_dphy_dev_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(mipi_dphy_dev_suspend, mipi_dphy_dev_resume)
#endif
#ifdef CONFIG_PM
	SET_RUNTIME_PM_OPS(mipi_dphy_dev_rpm_suspend, mipi_dphy_dev_rpm_resume, NULL)
#endif
};

static const struct of_device_id mipi_dphy_rx_of_match[] = {
	{ .compatible = "cix,cix-mipi-dphy-rx", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mipi_dphy_rx_of_match);

static const struct acpi_device_id mipi_dphy_rx_acpi_match[] = {
	{ .id = "CIXH302B", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, mipi_dphy_rx_acpi_match);

static struct platform_driver mipi_dphy_rx_driver = {
	.driver = {
		.name = CIX_MIPI_DPHY_RX_DRIVER_NAME,
		.of_match_table = mipi_dphy_rx_of_match,
		.acpi_match_table = ACPI_PTR(mipi_dphy_rx_acpi_match),
		.pm = &mipi_dphy_dev_pm_ops,
	},
	.probe = mipi_dphy_rx_probe,
	.remove = mipi_dphy_rx_remove,
};

module_platform_driver(mipi_dphy_rx_driver);

MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("Cix MIPI DPHY RX driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_MIPI_DPHY_RX_DRIVER_NAME);

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
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/videodev2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/iopoll.h>

#include "csi_common.h"

#define CIX_MIPI_CSI2_DRIVER_NAME	"cix-mipi-csi2"
#define CIX_MIPI_CSI2_SUBDEV_NAME	CIX_MIPI_CSI2_DRIVER_NAME
#define CIX_MIPI_OF_NODE_NAME		"cix-csi2-virt"
#define CIX_MIPI_MAX_DEVS			(32)

#define DATA_PATH_WIDTH (8 * 4)

enum cix_csi2_pads {
	CIX_MIPI_CSI2_PAD_SINK,
	CIX_MIPI_CSI2_PAD_SOURCE,
	CIX_MIPI_CSI2_PAD_MAX,
};

static struct media_pad *
cix_csi2_get_remote_sensor_pad(struct csi2rx_priv *csi2dev)
{
	struct v4l2_subdev *subdev = &csi2dev->subdev;
	struct media_pad *sink_pad, *source_pad;
	int i;

	source_pad = NULL;
	for (i = 0; i < subdev->entity.num_pads; i++) {
		sink_pad = &subdev->entity.pads[i];

		if (sink_pad->flags & MEDIA_PAD_FL_SINK) {
			source_pad = media_pad_remote_pad_first(sink_pad);
			if (source_pad)
				/* return first pad point in the loop  */
				return source_pad;
		}
	}

	if (i == subdev->entity.num_pads)
		v4l2_err(&csi2dev->subdev, "%s, No remote pad found!\n",
			 __func__);

	return NULL;
}
static struct v4l2_subdev *cix_get_remote_subdev(struct csi2rx_priv *csi2dev,
						 const char *const label)
{
	struct media_pad *source_pad;
	struct v4l2_subdev *sen_sd;

	/* Get remote source pad */
	source_pad = cix_csi2_get_remote_sensor_pad(csi2dev);
	if (!source_pad) {
		v4l2_err(&csi2dev->subdev, "%s, No remote pad found!\n", label);
		return NULL;
	}

	/* Get remote source pad subdev */
	sen_sd = media_entity_to_v4l2_subdev(source_pad->entity);
	if (!sen_sd) {
		v4l2_err(&csi2dev->subdev, "%s, No remote subdev found!\n",
			 label);
		return NULL;
	}

	return sen_sd;
}

static int cix_csi2_get_sensor_fmt(struct csi2rx_priv *csi2rx)
{
	struct v4l2_mbus_framefmt *mf = &csi2rx->format;
	struct v4l2_subdev *sen_sd;
	struct v4l2_subdev_format src_fmt;
	struct media_pad *source_pad;
	int ret;

	/* Get remote source pad */
	source_pad = cix_csi2_get_remote_sensor_pad(csi2rx);
	if (!source_pad) {
		v4l2_err(&csi2rx->subdev, "%s, No remote pad found!\n",
			 __func__);
		return -EINVAL;
	}

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	memset(&src_fmt, 0, sizeof(src_fmt));
	src_fmt.pad = source_pad->index;
	src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(sen_sd, pad, get_fmt, NULL, &src_fmt);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		return -EINVAL;

	/* Update input frame size and formate  */
	memcpy(mf, &src_fmt.format, sizeof(struct v4l2_mbus_framefmt));

	dev_dbg(csi2rx->dev, "width=%d, height=%d, fmt.code=0x%x\n", mf->width,
		mf->height, mf->code);
	return 0;
}

static int cix_csi2_cal_clk_freq(struct csi2rx_priv *csi2rx)
{
	/*
	 *calibrate the frequency of csi
	 *internal datapath width 32-bits
	 *sys_clk = Input Data rate（Mbytes/s）/datapath width(Bytes)
	 *= Input Data rate（Mbites/s）/8/4
	 */

	csi2rx->sys_clk_freq =
		(csi2rx->data_rate_Mbit * csi2rx->num_lanes) / DATA_PATH_WIDTH;

	return 0;
}

static int mipi_csi2_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	return 0;
}

static int mipi_csi2_link_setup(struct media_entity *entity,
				const struct media_pad *local,
				const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations mipi_csi2_sd_media_ops = {
	.link_setup = mipi_csi2_link_setup,
};

/*
 * V4L2 subdev operations
 */

static int mipi_csi2_s_power(struct v4l2_subdev *sd, int on)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	if (on) {
		dev_info(csi2rx->dev, "mipi-csi2 pm get sync\n");
		pm_runtime_get_sync(csi2rx->dev);
	} else {
		dev_info(csi2rx->dev, "mipi-csi2 pm put sync\n");
		pm_runtime_put(csi2rx->dev);
	}

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, core, s_power, on);
}

static int
mipi_csi2_g_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_frame_interval *interval)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, video, g_frame_interval, interval);
}

static int
mipi_csi2_s_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_frame_interval *interval)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, video, s_frame_interval, interval);
}

static int mipi_csi2_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct mipi_csi2_hw_drv_data *hw_drv;
	int stream_id = 0;
	int ret = 0;

	hw_drv = (struct mipi_csi2_hw_drv_data *)csi2rx->mipi_csi2_hw->drv_data;
	if (!hw_drv) {
		dev_info(csi2rx->dev, "mipi csi2 hardware handler is NULL\n");
		return -EINVAL;
	}

	/*here we just user mipi-csi2 0\2\4\6 stream id 0/1/2/3*/
	stream_id = csi2rx->id%CSI2RX_STREAMS_MAX;

	csi2rx->hw_stream_id = stream_id;

	if (enable) {
		/*here fix,stream id equal vc id*/
		if (stream_id == 2) {
			/*force use virtual channel1*/
			hw_drv->stream_start(csi2rx->mipi_csi2_hw,stream_id,1);
			csi2rx->virtual_chan_id = 1;
		} else {
			hw_drv->stream_start(csi2rx->mipi_csi2_hw,stream_id,stream_id);
			csi2rx->virtual_chan_id = stream_id;
		}

		csi2rx->stream_on = 1;
	} else {
		hw_drv->stream_stop(csi2rx->mipi_csi2_hw,stream_id);
		csi2rx->stream_on = 0;
	}

	return ret;
}

static int mipi_csi2_enum_framesizes(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *cfg,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, pad, enum_frame_size, NULL, fse);
}

static int
mipi_csi2_enum_frame_interval(struct v4l2_subdev *sd,
			      struct v4l2_subdev_state *cfg,
			      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);

	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, pad, enum_frame_interval, NULL, fie);
}

static int mipi_csi2_get_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_format *fmt)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_mbus_framefmt *mf = &fmt->format;

	cix_csi2_get_sensor_fmt(csi2rx);

	memcpy(mf, &csi2rx->format, sizeof(struct v4l2_mbus_framefmt));
	/* Source/Sink pads crop rectangle size */

	csi2rx->data_rate_Mbit = mf->reserved[0];
	dev_info(csi2rx->dev, " data_rate_mbyte  =0x%x,\n", mf->reserved[0]);
	cix_csi2_cal_clk_freq(csi2rx);

	dev_info(csi2rx->dev,
		 " format.reserved[0]=0x%x, format.reserved[1]=%x,\n",
		 mf->reserved[0], mf->reserved[1]);

	mf->reserved[0] = csi2rx->sys_clk_freq;

	dev_info(csi2rx->dev,
		 " format.reserved[0]=0x%x, format.reserved[1]=%x,\n",
		 mf->reserved[0], mf->reserved[1]);

	return 0;
}

static int mipi_csi2_async_bound(struct v4l2_async_notifier *notifier,
				 struct v4l2_subdev *s_subdev,
				 struct v4l2_async_connection *asd)
{
	struct v4l2_subdev *subdev = notifier->sd;
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(subdev);

	dev_info(csi2rx->dev, "mipi sync bound enter\n");

	/*find remote entity source pad */
	csi2rx->source_pad = media_entity_get_fwnode_pad(
		&s_subdev->entity, s_subdev->fwnode, MEDIA_PAD_FL_SOURCE);

	if (csi2rx->source_pad < 0) {
		dev_err(csi2rx->dev, "Couldn't find output pad for subdev %s\n",
			s_subdev->name);
		return -1;
	}

	csi2rx->source_subdev = s_subdev;
	dev_info(csi2rx->dev,
		 "mipi bound source subdev %s index %d sink name %s index %d\n",
		 csi2rx->source_subdev->name, csi2rx->source_pad,
		 csi2rx->subdev.name, CIX_MIPI_CSI2_PAD_SINK);

	return media_create_pad_link(
		&csi2rx->source_subdev->entity, csi2rx->source_pad,
		&csi2rx->subdev.entity, CIX_MIPI_CSI2_PAD_SINK,
		MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
}

static int mipi_csi2_parse_endpoint(struct device *dev,
				    struct v4l2_fwnode_endpoint *vep,
				    v4l2_async_subdev *asd)
{
	dev_info(dev, "mipi parse the endpoints port %d\n", vep->base.port);

	if (vep->base.port != 0) {
		dev_info(dev,
			 "mipi do not need remote  endpoints port %d id %d\n",
			 vep->base.port, vep->base.id);
		return -ENOTCONN;
	}

	return 0;
}

#ifdef CONFIG_PM
static int mipi_csi2_dev_rpm_suspend(struct device *dev)
{
	struct csi2rx_priv *csi2rx = dev_get_drvdata(dev);
	struct mipi_csi2_hw_drv_data *hw_drv;

	hw_drv = (struct mipi_csi2_hw_drv_data *)csi2rx->mipi_csi2_hw->drv_data;
	if (!hw_drv) {
		dev_info(dev, "mipi csi2 hardware suspend failed\n");
		return -EINVAL;
	}

	if (csi2rx->stream_on == 1) {
		if (hw_drv->mipi_csi2_irq_enable)
			hw_drv->mipi_csi2_irq_enable(csi2rx->mipi_csi2_hw,0);

		if (hw_drv->stream_stop)
			hw_drv->stream_stop(csi2rx->mipi_csi2_hw,csi2rx->hw_stream_id);
	}

	if (hw_drv->hw_suspend)
		hw_drv->hw_suspend(csi2rx->mipi_csi2_hw);

	return 0;
}

static int mipi_csi2_dev_rpm_resume(struct device *dev)
{
	struct csi2rx_priv *csi2rx = dev_get_drvdata(dev);
	struct mipi_csi2_hw_drv_data *hw_drv;

	hw_drv = (struct mipi_csi2_hw_drv_data *)csi2rx->mipi_csi2_hw->drv_data;
	if (!hw_drv) {
		dev_info(dev, "mipi csi2 hardware resume failed\n");
		return -EINVAL;
	}

	hw_drv->hw_resume(csi2rx->mipi_csi2_hw);

	if (csi2rx->stream_on == 1)
		if (hw_drv->stream_start)
			hw_drv->stream_start(csi2rx->mipi_csi2_hw,
					csi2rx->hw_stream_id,csi2rx->virtual_chan_id);

	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP

static int mipi_csi2_dev_suspend(struct device *dev)
{
	pm_runtime_force_suspend(dev);
	return 0;
}

static int mipi_csi2_dev_resume(struct device *dev)
{
	pm_runtime_force_resume(dev);
	return 0;
}
#endif

static const struct dev_pm_ops mipi_csi2_dev_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(mipi_csi2_dev_suspend, mipi_csi2_dev_resume)
#endif
#ifdef CONFIG_PM
	SET_RUNTIME_PM_OPS(mipi_csi2_dev_rpm_suspend,
				   mipi_csi2_dev_rpm_resume, NULL)
#endif
};

static const struct v4l2_subdev_internal_ops mipi_csi2_sd_internal_ops = {
	.open = mipi_csi2_open,
};

static const struct v4l2_subdev_pad_ops mipi_csi2_pad_ops = {
	.enum_frame_size = mipi_csi2_enum_framesizes,
	.enum_frame_interval = mipi_csi2_enum_frame_interval,
	.get_fmt = mipi_csi2_get_fmt,
};

static const struct v4l2_subdev_core_ops mipi_csi2_core_ops = {
	.s_power = mipi_csi2_s_power,
};

static const struct v4l2_subdev_video_ops mipi_csi2_video_ops = {
	.g_frame_interval = mipi_csi2_g_frame_interval,
	.s_stream = mipi_csi2_s_stream,
	.s_frame_interval = mipi_csi2_s_frame_interval,
};

static const struct v4l2_subdev_ops mipi_csi2_subdev_ops = {
	.core = &mipi_csi2_core_ops,
	.video = &mipi_csi2_video_ops,
	.pad = &mipi_csi2_pad_ops,
};

static const struct v4l2_async_notifier_operations mipi_csi2_notifier_ops = {
	.bound = mipi_csi2_async_bound,
};

static int mipi_csi2_media_init(struct csi2rx_priv *csi2rx)
{
	int ret;

	/* Create our media pads */
	csi2rx->pads[CIX_MIPI_CSI2_PAD_SINK].flags = MEDIA_PAD_FL_SINK |
						     MEDIA_PAD_FL_MUST_CONNECT;
	csi2rx->pads[CIX_MIPI_CSI2_PAD_SOURCE].flags =
		MEDIA_PAD_FL_SOURCE | MEDIA_PAD_FL_MUST_CONNECT;
	csi2rx->subdev.entity.ops = &mipi_csi2_sd_media_ops;
	ret = media_entity_pads_init(&csi2rx->subdev.entity,
				     CIX_MIPI_CSI2_PAD_MAX, csi2rx->pads);

	return ret;
}

static int mipi_csi2_parse(struct csi2rx_priv *csi2rx)
{
	struct device *dev = csi2rx->dev;
	int ret = -1;

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u8(dev, CIX_MIPI_OF_NODE_NAME,
				&csi2rx->id);
	} else {
		ret = csi2rx->id =
			of_alias_get_id(dev->of_node, CIX_MIPI_OF_NODE_NAME);
	}

	if ((ret < 0) || (csi2rx->id >= CIX_MIPI_MAX_DEVS)) {
		dev_err(dev, "invalid mipi device id (%d)\n", csi2rx->id);
		return -EINVAL;
	}

	return 0;
}

static struct mipi_csi2_hw *mipi_csi2_hw_attach(struct csi2rx_priv *csi2rx)
{
	struct platform_device *plat_dev;
	struct fwnode_handle *np;
	struct mipi_csi2_hw *mipi_csi2_hw;
	struct device *dev = csi2rx->dev;
	struct device *tdev = NULL;

	np = fwnode_find_reference(dev->fwnode, "cix,hw", 0);
	if (IS_ERR(np) || !fwnode_device_is_available(np)) {
		dev_err(dev, "failed to get virtual csi%d hw node\n", csi2rx->id);
		return NULL;
	}

	tdev = bus_find_device_by_fwnode(&platform_bus_type, np);
	plat_dev = tdev ? to_platform_device(tdev) : NULL;

	fwnode_handle_put(np);

	if (!plat_dev) {
		dev_err(dev, "failed to get mipi csi2%d hw from node\n",
				csi2rx->id);
		return NULL;
	}

	mipi_csi2_hw = platform_get_drvdata(plat_dev);
	if (!mipi_csi2_hw) {
		dev_err(dev, "failed attach dphy%d hw\n", csi2rx->id);
		return NULL;
	}

	dev_info(dev, "mipi-csi2 %d attach hardware success\n", csi2rx->id);

	return mipi_csi2_hw;
}

static int mipi_csi2_probe(struct platform_device *pdev)
{
	struct csi2rx_priv *csi2rx = NULL;
	struct device *dev = &pdev->dev;
	struct v4l2_subdev *sd;
	int ret = -1;

	dev_info(dev, "mipi-csi2 probe enter\n");

	csi2rx = devm_kzalloc(dev, sizeof(*csi2rx), GFP_KERNEL);
	if (!csi2rx)
		return -ENOMEM;

	csi2rx->dev = dev;
	csi2rx->pdev = pdev;
	csi2rx->mipi_csi2_hw = mipi_csi2_hw_attach(csi2rx);

	/*parse the dts or ACPI table*/
	mipi_csi2_parse(csi2rx);

	mutex_init(&csi2rx->lock);

	spin_lock_init(&csi2rx->slock);
	/*init the subdev*/
	sd = &csi2rx->subdev;
	sd->dev = &pdev->dev;
	sd->owner = THIS_MODULE;
	v4l2_subdev_init(sd, &mipi_csi2_subdev_ops);
	v4l2_set_subdevdata(sd, csi2rx);
	snprintf(sd->name, sizeof(sd->name), "%s.%d", CIX_MIPI_CSI2_SUBDEV_NAME,
		 csi2rx->id);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	/* Create our media pads */
	mipi_csi2_media_init(csi2rx);

	/*notifier & async subdev init*/
	v4l2_async_subdev_nf_init(&csi2rx->notifier, sd);

	ret = v4l2_async_nf_parse_fwnode_endpoints(
		dev, &csi2rx->notifier, sizeof(v4l2_async_subdev),
		mipi_csi2_parse_endpoint);
	if (ret < 0) {
		dev_err(dev,
			"mipi-csi async notifier parse endpoints failed\n");
		return ret;
	}

	csi2rx->notifier.ops = &mipi_csi2_notifier_ops;
	ret = v4l2_async_nf_register(&csi2rx->notifier);
	if (ret) {
		dev_err(dev, "mipi-csi async register notifier failed\n");
		v4l2_async_nf_cleanup(&csi2rx->notifier);
		return ret;
	}

	ret = v4l2_async_register_subdev(sd);

	platform_set_drvdata(pdev, csi2rx);

	pm_runtime_enable(dev);

	dev_info(dev, "mipi-csi2 probe exit %s\n",
		 ret == 0 ? "success" : "failed");

	return ret;
}

static int mipi_csi2_remove(struct platform_device *pdev)
{
	struct csi2rx_priv *csi2rx = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &csi2rx->subdev;

	media_entity_cleanup(&csi2rx->subdev.entity);
	v4l2_async_nf_unregister(&csi2rx->notifier);
	v4l2_async_nf_cleanup(&csi2rx->notifier);
	v4l2_async_unregister_subdev(sd);

	pm_runtime_disable(csi2rx->dev);

	return 0;
}

static const struct of_device_id mipi_csi2_of_match[] = {
	{ .compatible = "cix,cix-mipi-csi2"},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mipi_csi2_of_match);

static struct platform_driver mipi_csi2_driver = {
	.driver = {
		.name = CIX_MIPI_CSI2_DRIVER_NAME,
		.of_match_table = mipi_csi2_of_match,
		.pm = &mipi_csi2_dev_pm_ops,
	},
	.probe = mipi_csi2_probe,
	.remove = mipi_csi2_remove,
};

module_platform_driver(mipi_csi2_driver);

MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("Cix MIPI CSI2 driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_MIPI_CSI2_DRIVER_NAME);

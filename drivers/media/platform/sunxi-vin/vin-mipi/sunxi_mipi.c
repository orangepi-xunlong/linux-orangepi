
/*
 ******************************************************************************
 *
 * sunxi_mipi.c
 *
 * Hawkview ISP - sunxi_mipi.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Zhao Wei  	2015/02/27	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "bsp_mipi_csi.h"
#include "sunxi_mipi.h"
#include "../platform/platform_cfg.h"
#define MIPI_MODULE_NAME "vin_mipi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

static LIST_HEAD(mipi_drv_list);

static struct mipi_fmt_cfg sunxi_mipi_formats[] = {
	{
		.field[0] = V4L2_FIELD_NONE,
		.packet_fmt[0] = MIPI_RAW10,
		.vc[0] = 0,
	}
};

static char *clk_name[MIPI_CLK_NUM] = {
	"mipi_csi_clk",
	"mipi_dphy_clk",
	"mipi_csi_clk_src",
	"mipi_dphy_clk_src",
};

static int __mipi_clk_get(struct mipi_dev *dev)
{
#ifdef VIN_CLK
	int i;
	int clk_index[MIPI_CLK_NUM];
	struct device_node *np = dev->pdev->dev.of_node;
	of_property_read_u32_array(np, "clocks-index", clk_index, MIPI_CLK_NUM);
	for (i = 0; i < MIPI_CLK_NUM; i++) {
		if (clk_index[i] != NOCLK) {
			dev->clock[i] = of_clk_get(np, clk_index[i]);
			if (IS_ERR_OR_NULL(dev->clock[i]))
				vin_warn("Get clk Index:%d, Name:%s is NULL!\n",
				     (int)clk_index[i], clk_name[i]);
			vin_log(VIN_LOG_MIPI, "Get clk Name:%s\n",
				dev->clock[i]->name);
		}
	}
#endif
	return 0;
}

static int __mipi_dphy_clk_set(struct mipi_dev *dev, unsigned long freq)
{
#ifdef VIN_CLK
	if (dev->clock[MIPI_DPHY_CLK] && dev->clock[MIPI_DPHY_CLK_SRC]) {
		if (clk_set_parent
		    (dev->clock[MIPI_DPHY_CLK],
		     dev->clock[MIPI_DPHY_CLK_SRC])) {
			vin_err("set vfe dphy clock source failed \n");
			return -1;
		}
		if (clk_set_rate(dev->clock[MIPI_DPHY_CLK], freq)) {
			vin_err("set mipi%d dphy clock error\n", dev->id);
			return -1;
		}
	} else {
		vin_warn("vfe dphy clock is null\n");
		return -1;
	}
#endif
	return 0;
}

static int __mipi_clk_enable(struct mipi_dev *dev)
{
	int ret = 0;
#ifdef VIN_CLK
	int i;
	for (i = 0; i < MIPI_CSI_CLK_SRC; i++) {
		if (dev->clock[i]) {
			if (clk_prepare_enable(dev->clock[i])) {
				vin_err("%s enable error\n", clk_name[i]);
				ret = -1;
			}
		} else {
			vin_log(VIN_LOG_MIPI, "%s is null\n", clk_name[i]);
			ret = -1;
		}
	}
#endif
	return ret;
}

static void __mipi_clk_disable(struct mipi_dev *dev)
{
#ifdef VIN_CLK
	int i;
	for (i = 0; i < MIPI_CSI_CLK_SRC; i++) {
		if (dev->clock[i])
			clk_disable_unprepare(dev->clock[i]);
		else
			vin_log(VIN_LOG_MIPI, "%s is null\n", clk_name[i]);
	}
#endif
}

static void __mipi_clk_release(struct mipi_dev *dev)
{
#ifdef VIN_CLK
	int i;
	for (i = 0; i < MIPI_CLK_NUM; i++) {
		if (dev->clock[i])
			clk_put(dev->clock[i]);
		else
			vin_log(VIN_LOG_MIPI, "%s is null\n", clk_name[i]);
	}
#endif
}

static enum pkt_fmt get_pkt_fmt(enum v4l2_mbus_pixelcode code)
{
	switch (code) {
	case V4L2_MBUS_FMT_RGB565_2X8_BE:
		return MIPI_RGB565;
	case V4L2_MBUS_FMT_UYVY8_1X16:
		return MIPI_YUV422;
	case V4L2_MBUS_FMT_YUYV10_2X10:
		return MIPI_YUV422_10;
	case V4L2_MBUS_FMT_SBGGR8_1X8:
	case V4L2_MBUS_FMT_SGBRG8_1X8:
	case V4L2_MBUS_FMT_SGRBG8_1X8:
	case V4L2_MBUS_FMT_SRGGB8_1X8:
		return MIPI_RAW8;
	case V4L2_MBUS_FMT_SBGGR10_1X10:
	case V4L2_MBUS_FMT_SGBRG10_1X10:
	case V4L2_MBUS_FMT_SGRBG10_1X10:
	case V4L2_MBUS_FMT_SRGGB10_1X10:
		return MIPI_RAW10;
	case V4L2_MBUS_FMT_SBGGR12_1X12:
	case V4L2_MBUS_FMT_SGBRG12_1X12:
	case V4L2_MBUS_FMT_SGRBG12_1X12:
	case V4L2_MBUS_FMT_SRGGB12_1X12:
		return MIPI_RAW12;
	default:
		return MIPI_RAW8;
	}
}

static int sunxi_mipi_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	if (mipi->use_cnt > 1)
		return 0;
	if (enable) {
		__mipi_dphy_clk_set(mipi, DPHY_CLK);
		__mipi_clk_enable(mipi);
	} else {
		__mipi_clk_disable(mipi);
	}
	usleep_range(10000, 12000);
	return 0;
}
static int sunxi_mipi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &mipi->format;

	int i;
	if (mipi->use_cnt > 1)
		return 0;

	mipi->mipi_para.bps = mf->reserved[0];
	mipi->mipi_para.auto_check_bps = 0;
	mipi->mipi_para.dphy_freq = DPHY_CLK;

	for (i = 0; i < mipi->mipi_para.total_rx_ch; i++) {
		mipi->mipi_fmt->packet_fmt[i] = get_pkt_fmt(mf->code);
		mipi->mipi_fmt->field[i] = mf->field;
		mipi->mipi_fmt->vc[i] = i;
	}

	if (enable) {
		bsp_mipi_csi_dphy_init(mipi->id);
		bsp_mipi_csi_set_para(mipi->id, &mipi->mipi_para);
		bsp_mipi_csi_set_fmt(mipi->id, mipi->mipi_para.total_rx_ch,
				     mipi->mipi_fmt);

		/*for dphy clock async*/
		bsp_mipi_csi_dphy_disable(mipi->id);
		bsp_mipi_csi_dphy_enable(mipi->id);
		bsp_mipi_csi_protocol_enable(mipi->id);
	} else {
		bsp_mipi_csi_dphy_disable(mipi->id);
		bsp_mipi_csi_protocol_disable(mipi->id);
		bsp_mipi_csi_dphy_exit(mipi->id);
	}

	vin_print("%s %d*%d %x %d\n", __func__, mf->width, mf->height,
		mf->code, mf->field);

	return 0;
}

static int sunxi_mipi_enum_mbus_code(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static struct v4l2_mbus_framefmt *__mipi_get_format(
		struct mipi_dev *mipi, struct v4l2_subdev_fh *fh,
		enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_format(fh, 0) : NULL;

	return &mipi->format;
}

static int sunxi_mipi_subdev_get_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = __mipi_get_format(mipi, fh, fmt->which);
	if (!mf)
		return -EINVAL;

	mutex_lock(&mipi->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&mipi->subdev_lock);
	return 0;
}

static struct mipi_fmt_cfg *__mipi_find_format(
	struct v4l2_mbus_framefmt *mf)
{
#if 0
	int i;
	for (i = 0; i < ARRAY_SIZE(sunxi_mipi_formats); i++)
		if (mf->code == sunxi_mipi_formats[i].code)
			return &sunxi_mipi_formats[i];
#endif
	return NULL;
}

static struct mipi_fmt_cfg *__mipi_try_format(struct v4l2_mbus_framefmt *mf)
{
	struct mipi_fmt_cfg *mipi_fmt;

	mipi_fmt = __mipi_find_format(mf);
	if (mipi_fmt == NULL)
		mipi_fmt = &sunxi_mipi_formats[0];

	/*mf->code = mipi_fmt->code;*/

	return mipi_fmt;
}

static int sunxi_mipi_subdev_set_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct mipi_fmt_cfg *mipi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = __mipi_get_format(mipi, fh, fmt->which);

	if (fmt->pad == MIPI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&mipi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&mipi->subdev_lock);
		}
		return 0;
	}

	mipi_fmt = __mipi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&mipi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			mipi->mipi_fmt = mipi_fmt;
		mutex_unlock(&mipi->subdev_lock);
	}

	return 0;
}

int sunxi_mipi_addr_init(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}
static int sunxi_mipi_subdev_cropcap(struct v4l2_subdev *sd,
				     struct v4l2_cropcap *a)
{
	return 0;
}

static int sunxi_mipi_s_mbus_config(struct v4l2_subdev *sd,
				    const struct v4l2_mbus_config *cfg)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2) {
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE))
			mipi->mipi_para.lane_num = 4;
		else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE))
			mipi->mipi_para.lane_num = 3;
		else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE))
			mipi->mipi_para.lane_num = 2;
		else
			mipi->mipi_para.lane_num = 1;

		mipi->mipi_para.total_rx_ch = 0;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0)) {
			mipi->mipi_para.total_rx_ch++;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1)) {
			mipi->mipi_para.total_rx_ch++;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2)) {
			mipi->mipi_para.total_rx_ch++;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3)) {
			mipi->mipi_para.total_rx_ch++;
		}
	}

	return 0;
}

static const struct v4l2_subdev_core_ops sunxi_mipi_core_ops = {
	.s_power = sunxi_mipi_subdev_s_power,
	.init = sunxi_mipi_addr_init,
};

static const struct v4l2_subdev_video_ops sunxi_mipi_subdev_video_ops = {
	.s_stream = sunxi_mipi_subdev_s_stream,
	.cropcap = sunxi_mipi_subdev_cropcap,
	.s_mbus_config = sunxi_mipi_s_mbus_config,
};

static const struct v4l2_subdev_pad_ops sunxi_mipi_subdev_pad_ops = {
	.enum_mbus_code = sunxi_mipi_enum_mbus_code,
	.get_fmt = sunxi_mipi_subdev_get_fmt,
	.set_fmt = sunxi_mipi_subdev_set_fmt,
};

static struct v4l2_subdev_ops sunxi_mipi_subdev_ops = {
	.core = &sunxi_mipi_core_ops,
	.video = &sunxi_mipi_subdev_video_ops,
	.pad = &sunxi_mipi_subdev_pad_ops,
};

static int sunxi_mipi_subdev_init(struct mipi_dev *mipi)
{
	struct v4l2_subdev *sd = &mipi->subdev;
	int ret;
	v4l2_subdev_init(sd, &sunxi_mipi_subdev_ops);
	sd->grp_id = VIN_GRP_ID_MIPI;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_mipi.%u", mipi->id);
	v4l2_set_subdevdata(sd, mipi);

	/*sd->entity->ops = &isp_media_ops;*/
	mipi->mipi_pads[MIPI_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	mipi->mipi_pads[MIPI_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;

	ret = media_entity_init(&sd->entity, MIPI_PAD_NUM, mipi->mipi_pads, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int mipi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mipi_dev *mipi = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("MIPI failed to get of node\n");
		return -ENODEV;
	}

	mipi = kzalloc(sizeof(struct mipi_dev), GFP_KERNEL);
	if (!mipi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	pdev->id = of_alias_get_id(np, "mipi");
	if (pdev->id < 0) {
		vin_err("MIPI failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	mipi->base = of_iomap(np, 0);
	if (!mipi->base) {
		ret = -EIO;
		goto freedev;
	}
	mipi->id = pdev->id;
	mipi->pdev = pdev;

	spin_lock_init(&mipi->slock);
	init_waitqueue_head(&mipi->wait);

	ret = bsp_mipi_csi_set_base_addr(mipi->id, (unsigned long)mipi->base);
	ret =
	    bsp_mipi_dphy_set_base_addr(mipi->id,
					(unsigned long)mipi->base + 0x1000);
	if (ret < 0)
		goto ehwinit;

	if (__mipi_clk_get(mipi)) {
		vin_err("mipi clock get failed!\n");
		return -ENXIO;
	}

	list_add_tail(&mipi->mipi_list, &mipi_drv_list);
	sunxi_mipi_subdev_init(mipi);

	platform_set_drvdata(pdev, mipi);
	vin_print("mipi %d probe end!\n", mipi->id);
	return 0;

ehwinit:
	iounmap(mipi->base);
freedev:
	kfree(mipi);
ekzalloc:
	vin_print("mipi probe err!\n");
	return ret;
}

static int mipi_remove(struct platform_device *pdev)
{
	struct mipi_dev *mipi = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &mipi->subdev;
	platform_set_drvdata(pdev, NULL);
	v4l2_set_subdevdata(sd, NULL);
	if (mipi->base)
		iounmap(mipi->base);
	__mipi_clk_release(mipi);
	list_del(&mipi->mipi_list);
	kfree(mipi);
	return 0;
}

static const struct of_device_id sunxi_mipi_match[] = {
	{.compatible = "allwinner,sunxi-mipi",},
	{},
};

static struct platform_driver mipi_platform_driver = {
	.probe = mipi_probe,
	.remove = mipi_remove,
	.driver = {
		   .name = MIPI_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_mipi_match,
		   }
};

struct v4l2_subdev *sunxi_mipi_get_subdev(int id)
{
	struct mipi_dev *mipi;
	list_for_each_entry(mipi, &mipi_drv_list, mipi_list) {
		if (mipi->id == id) {
			mipi->use_cnt++;
			return &mipi->subdev;
		}
	}
	return NULL;
}

int sunxi_mipi_platform_register(void)
{
	return platform_driver_register(&mipi_platform_driver);
}

void sunxi_mipi_platform_unregister(void)
{
	platform_driver_unregister(&mipi_platform_driver);
	vin_print("mipi_exit end\n");
}

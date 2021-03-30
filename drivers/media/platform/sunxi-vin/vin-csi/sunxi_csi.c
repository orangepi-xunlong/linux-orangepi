
/*
 ******************************************************************************
 *
 * sunxi_csi.c
 *
 * Hawkview ISP - sunxi_csi.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/02/27	ISP Tuning Tools Support
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
#include <media/sunxi_camera_v2.h>

#include "bsp_csi.h"
#include "sunxi_csi.h"
#include "../platform/platform_cfg.h"

#define CSI_MODULE_NAME "vin_csi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

static LIST_HEAD(csi_drv_list);

#define CSI_MAX_PIX_WIDTH		0xffff
#define CSI_MAX_PIX_HEIGHT		0xffff

#define CSI_CFG_FMT_YUV422_8BIT	(0x1e << 2)
#define CSI_CFG_FMT_YVU422_8BIT	(0x1e << 2)
#define CSI_CFG_FMT_UYV422_8BIT	(0x1e << 2)
#define CSI_CFG_FMT_VYU422_8BIT	(0x1e << 2)
#define CSI_CFG_FMT_RAW8	(0x2a << 2)
#define CSI_CFG_FMT_RAW10	(0x2b << 2)
#define CSI_CFG_FMT_RAW12	(0x2c << 2)

static struct csi_pix_format sunxi_csi_formats[] = {
	{
		.code = V4L2_MBUS_FMT_YUYV8_2X8,
		.fmt_reg = CSI_CFG_FMT_YUV422_8BIT,
	}, {
		.code = V4L2_MBUS_FMT_YVYU8_2X8,
		.fmt_reg = CSI_CFG_FMT_YVU422_8BIT,
	}, {
		.code = V4L2_MBUS_FMT_UYVY8_2X8,
		.fmt_reg = CSI_CFG_FMT_UYV422_8BIT,
	}, {
		.code = V4L2_MBUS_FMT_VYUY8_2X8,
		.fmt_reg = CSI_CFG_FMT_VYU422_8BIT,
	}, {
		.code = V4L2_MBUS_FMT_SGRBG8_1X8,
		.fmt_reg = CSI_CFG_FMT_RAW8,
	}, {
		.code = V4L2_MBUS_FMT_SBGGR8_1X8,
		.fmt_reg = CSI_CFG_FMT_RAW8,
	}, {
		.code = V4L2_MBUS_FMT_SGRBG10_1X10,
		.fmt_reg = CSI_CFG_FMT_RAW10,
	}, {
		.code = V4L2_MBUS_FMT_SGRBG12_1X12,
		.fmt_reg = CSI_CFG_FMT_RAW12,
	}
};

static char *clk_name[CSI_CLK_NUM] = {
	"csi_core_clk",
	"csi_master_clk",
	"csi_misc_clk",
	"csi_core_clk_src",
	"csi_master_clk_24M_src",
	"csi_master_clk_pll_src",
};

static int __csi_clk_get(struct csi_dev *dev)
{
#ifdef VIN_CLK
	int i;
	int clk_index[CSI_CLK_NUM];
	struct device_node *np = dev->pdev->dev.of_node;

	of_property_read_u32_array(np, "clocks-index", clk_index, CSI_CLK_NUM);
	for (i = 0; i < CSI_CLK_NUM; i++) {
		if (clk_index[i] != NOCLK) {
			dev->clock[i] = of_clk_get(np, clk_index[i]);
			if (IS_ERR_OR_NULL(dev->clock[i]))
				vin_warn
				    ("Get clk Index:%d , Name:%s is NULL!\n",
				     (int)clk_index[i], clk_name[i]);
			vin_log(VIN_LOG_CSI, "Get clk Name:%s\n", dev->clock[i]->name);
		}
	}

	if (dev->clock[CSI_CORE_CLK] && dev->clock[CSI_CORE_CLK_SRC]) {
		if (clk_set_parent
		    (dev->clock[CSI_CORE_CLK], dev->clock[CSI_CORE_CLK_SRC])) {
			vin_err
			    ("sclk src Name:%s, csi core clock set parent failed \n",
			     dev->clock[CSI_CORE_CLK_SRC]->name);
			return -1;
		}
		if (clk_set_rate(dev->clock[CSI_CORE_CLK], CSI_CORE_CLK_RATE)) {
			vin_err("set core clock rate error\n");
			return -1;
		}
	} else {
		vin_err("csi core clock is null\n");
		return -1;
	}
	vin_log(VIN_LOG_CSI, "csi core clk = %ld\n",
		clk_get_rate(dev->clock[CSI_CORE_CLK]));
#endif
	return 0;
}

static int __csi_clk_enable(struct csi_dev *dev)
{
	int ret = 0;
#ifdef VIN_CLK
	int i;
	for (i = 0; i < CSI_CORE_CLK_SRC; i++) {
		if (CSI_MASTER_CLK != i) {
			if (dev->clock[i]) {
				if (clk_prepare_enable(dev->clock[i])) {
					vin_err("%s enable error\n",
						clk_name[i]);
					ret = -1;
				}
			} else {
				vin_log(VIN_LOG_CSI, "%s is null\n", clk_name[i]);
				ret = -1;
			}
		}
	}
#else
	void __iomem *clk_base;
	clk_base = ioremap(0x01c20000, 0x200);
	if (!clk_base) {
		printk("clk_base directly write pin config EIO\n");
		return -EIO;
	}
	writel(0xffffffff, (clk_base + 0x64));
	writel(0xffffffff, (clk_base + 0x2c4));
	writel(0x0000000f, (clk_base + 0x100));
	writel(0x80000000, (clk_base + 0x130));	/*open misc clk gate*/
	writel(0x80018000, (clk_base + 0x134));	/*set sclk src pll_periph0 and mclk src clk_hosc*/
#endif
	return ret;
}

static void __csi_clk_disable(struct csi_dev *dev)
{
#ifdef VIN_CLK
	int i;
	for (i = 0; i < CSI_CORE_CLK_SRC; i++) {
		if (CSI_MASTER_CLK != i) {
			if (dev->clock[i])
				clk_disable_unprepare(dev->clock[i]);
			else
				vin_log(VIN_LOG_CSI, "%s is null\n", clk_name[i]);
		}
	}
#endif
}

static void __csi_clk_release(struct csi_dev *dev)
{
#ifdef VIN_CLK
	int i;
	for (i = 0; i < CSI_CLK_NUM; i++) {
		if (dev->clock[i])
			clk_put(dev->clock[i]);
		else
			vin_log(VIN_LOG_CSI, "%s is null\n", clk_name[i]);
	}
#endif
}

static void __csi_reset_enable(struct csi_dev *dev)
{
#ifdef VIN_CLK
	sunxi_periph_reset_assert(dev->clock[CSI_CORE_CLK]);
#endif
}

static void __csi_reset_disable(struct csi_dev *dev)
{
#ifdef VIN_CLK
	sunxi_periph_reset_deassert(dev->clock[CSI_CORE_CLK]);
#endif
}

static int __csi_pin_config(struct csi_dev *dev, int enable)
{
#ifdef VIN_GPIO
	char pinctrl_names[10] = "";
	if (!IS_ERR_OR_NULL(dev->pctrl)) {
		devm_pinctrl_put(dev->pctrl);
	}
	if (1 == enable) {
		strcpy(pinctrl_names, "default");
	} else {
		strcpy(pinctrl_names, "sleep");
	}
	dev->pctrl = devm_pinctrl_get_select(&dev->pdev->dev, pinctrl_names);
	if (IS_ERR_OR_NULL(dev->pctrl)) {
		vin_err("csi%d request pinctrl handle failed!\n", dev->id);
		return -EINVAL;
	}
	usleep_range(5000, 6000);
#else
	void __iomem *gpio_base;
	vin_print("directly write pin config @ FPGA\n");

	gpio_base = ioremap(GPIO_REGS_VBASE, 0x120);
	if (!gpio_base) {
		printk("gpio_base directly write pin config EIO\n");
		return -EIO;
	}
#ifdef FPGA_PIN /*Direct write for pin of FPGA*/
	writel(0x33333333, (gpio_base + 0x90));
	writel(0x33333333, (gpio_base + 0x94));
	writel(0x03333333, (gpio_base + 0x98));
#else /*Direct write for pin of IC*/
	writel(0x22222222, (gpio_base + 0x90));
	writel(0x10222222, (gpio_base + 0x94));
	writel(0x11111111, (gpio_base + 0x98));
#endif
#endif
	return 0;
}

static int __csi_pin_release(struct csi_dev *dev)
{
#ifdef VIN_GPIO
	devm_pinctrl_put(dev->pctrl);
#endif
	return 0;
}

void sunxi_csi_set_output_fmt(struct v4l2_subdev *sd, __u32 pixelformat)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	csi->frame_info.pix_ch_fmt[0] = pixelformat;
}

static int __csi_set_fmt_hw(struct csi_dev *csi)
{
	struct v4l2_mbus_framefmt *mf = &csi->format;
	int i, ret;

	for (i = 0; i < csi->bus_info.ch_total_num; i++)
		csi->bus_info.bus_ch_fmt[i] = mf->code;
	for (i = 0; i < csi->bus_info.ch_total_num; i++) {
		csi->frame_info.pix_ch_fmt[i] = csi->frame_info.pix_ch_fmt[0];
		csi->frame_info.ch_field[i] = mf->field;
		csi->frame_info.ch_size[i].width = mf->width;
		csi->frame_info.ch_size[i].height = mf->height;
		csi->frame_info.ch_offset[i].hoff = 0;
		csi->frame_info.ch_offset[i].voff = 0;
	}
	csi->frame_info.arrange = csi->arrange;

	ret = bsp_csi_set_fmt(csi->id, &csi->bus_info, &csi->frame_info);
	if (ret < 0) {
		vin_err("bsp_csi_set_fmt error at %s!\n", __func__);
		return -1;
	}
	ret = bsp_csi_set_size(csi->id, &csi->bus_info, &csi->frame_info);
	if (ret < 0) {
		vin_err("bsp_csi_set_size error at %s!\n", __func__);
		return -1;
	}

	return 0;
}

static int sunxi_csi_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (enable) {
		__csi_clk_enable(csi);
		__csi_reset_disable(csi);
	} else {
		__csi_clk_disable(csi);
		__csi_reset_enable(csi);
	}
	__csi_pin_config(csi, enable);
	return 0;
}
static int sunxi_csi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &csi->format;

	if (enable) {
		bsp_csi_enable(csi->id);
		bsp_csi_disable(csi->id);
		bsp_csi_enable(csi->id);
		__csi_set_fmt_hw(csi);

		vin_log(VIN_LOG_CSI, "enable csi int in s_stream on\n");
		bsp_csi_int_clear_status(csi->id, csi->cur_ch,
					 CSI_INT_ALL);
		bsp_csi_int_enable(csi->id, csi->cur_ch,
				   CSI_INT_CAPTURE_DONE | CSI_INT_FRAME_DONE |
				   CSI_INT_BUF_0_OVERFLOW |
				   CSI_INT_BUF_1_OVERFLOW |
				   CSI_INT_BUF_2_OVERFLOW |
				   CSI_INT_HBLANK_OVERFLOW);

		if (csi->capture_mode == V4L2_MODE_IMAGE)
			bsp_csi_cap_start(csi->id, csi->bus_info.ch_total_num,
					  CSI_SCAP);
		else
			bsp_csi_cap_start(csi->id, csi->bus_info.ch_total_num,
					  CSI_VCAP);
	} else {
		vin_log(VIN_LOG_CSI, "disable csi int in s_stream off\n");
		bsp_csi_int_disable(csi->id, csi->cur_ch, CSI_INT_ALL);
		bsp_csi_int_clear_status(csi->id, csi->cur_ch,
					 CSI_INT_ALL);

		if (csi->capture_mode == V4L2_MODE_IMAGE)
			bsp_csi_cap_stop(csi->id, csi->bus_info.ch_total_num, CSI_SCAP);
		else
			bsp_csi_cap_stop(csi->id, csi->bus_info.ch_total_num, CSI_VCAP);
		bsp_csi_disable(csi->id);
	}
	vin_print("%s on = %d, %d*%d %x %d\n", __func__, enable,
		mf->width, mf->height, mf->code, mf->field);

	return 0;
}
static int sunxi_csi_subdev_s_parm(struct v4l2_subdev *sd,
				   struct v4l2_streamparm *param)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	csi->capture_mode = param->parm.capture.capturemode;
	return 0;
}

static int sunxi_csi_enum_mbus_code(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static struct csi_pix_format *__csi_find_format(
	struct v4l2_mbus_framefmt *mf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_csi_formats); i++)
		if (mf->code == sunxi_csi_formats[i].code)
			return &sunxi_csi_formats[i];
	return NULL;
}

static struct csi_pix_format *__csi_try_format(
	struct v4l2_mbus_framefmt *mf)
{
	struct csi_pix_format *csi_fmt;

	csi_fmt = __csi_find_format(mf);
	if (csi_fmt == NULL)
		csi_fmt = &sunxi_csi_formats[0];

	mf->code = csi_fmt->code;
	v4l_bound_align_image(&mf->width, 1, CSI_MAX_PIX_WIDTH,
			      csi_fmt->pix_width_alignment,
			      &mf->height, 1, CSI_MAX_PIX_HEIGHT, 1,
			      0);
	return csi_fmt;
}

static struct v4l2_mbus_framefmt *__csi_get_format(
		struct csi_dev *csi, struct v4l2_subdev_fh *fh,
		enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_format(fh, 0) : NULL;

	return &csi->format;
}

static int sunxi_csi_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct csi_pix_format *csi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = __csi_get_format(csi, fh, fmt->which);

	if (fmt->pad == CSI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&csi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&csi->subdev_lock);
		}
		return 0;
	}
	csi_fmt = __csi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&csi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			csi->csi_fmt = csi_fmt;
		mutex_unlock(&csi->subdev_lock);
	}

	return 0;
}

static int sunxi_csi_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_fh *fh,
				    struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = __csi_get_format(csi, fh, fmt->which);
	if (!mf)
		return -EINVAL;

	mutex_lock(&csi->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&csi->subdev_lock);
	return 0;
}

int sunxi_csi_addr_init(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}
static int sunxi_csi_subdev_cropcap(struct v4l2_subdev *sd,
				    struct v4l2_cropcap *a)
{
	return 0;
}

static int sunxi_csi_s_mbus_config(struct v4l2_subdev *sd,
				   const struct v4l2_mbus_config *cfg)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2) {
		csi->bus_info.bus_if = V4L2_MBUS_CSI2;
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0)) {
			csi->bus_info.ch_total_num++;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1)) {
			csi->bus_info.ch_total_num++;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2)) {
			csi->bus_info.ch_total_num++;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3)) {
			csi->bus_info.ch_total_num++;
		}
		vin_print("csi->bus_info.ch_total_num = %d\n",
			  csi->bus_info.ch_total_num);
	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
		csi->bus_info.bus_if = V4L2_MBUS_PARALLEL;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_MASTER)) {
			if (IS_FLAG(cfg->flags, V4L2_MBUS_HSYNC_ACTIVE_HIGH)) {
				csi->bus_info.bus_tmg.href_pol = ACTIVE_HIGH;
			} else {
				csi->bus_info.bus_tmg.href_pol = ACTIVE_LOW;
			}
			if (IS_FLAG(cfg->flags, V4L2_MBUS_VSYNC_ACTIVE_HIGH)) {
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_HIGH;
			} else {
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_LOW;
			}
			if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING)) {
				csi->bus_info.bus_tmg.pclk_sample = RISING;
			} else {
				csi->bus_info.bus_tmg.pclk_sample = FALLING;
			}
			if (IS_FLAG(cfg->flags, V4L2_MBUS_FIELD_EVEN_HIGH)) {
				csi->bus_info.bus_tmg.field_even_pol =
				    ACTIVE_HIGH;
			} else {
				csi->bus_info.bus_tmg.field_even_pol =
				    ACTIVE_LOW;
			}
		} else {
			vin_err("Do not support MBUS SLAVE! cfg->type = %d\n",
				cfg->type);
			return -1;
		}
	} else if (cfg->type == V4L2_MBUS_BT656) {
		csi->bus_info.bus_if = V4L2_MBUS_BT656;
	}

	return 0;
}

static long sunxi_csi_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
				   void *arg)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	int ret = 0;

	switch (cmd) {
	/*CSI internal IOCTL*/
	case VIDIOC_SUNXI_CSI_SET_CORE_CLK:
		mutex_lock(&csi->subdev_lock);

		ret =
		    clk_set_rate(csi->clock[CSI_CORE_CLK],
				 *(unsigned long *)arg);
		vin_print
		    ("Set csi core clk = %ld, after Set csi core clk = %ld \n",
		     *(unsigned long *)arg,
		     clk_get_rate(csi->clock[CSI_CORE_CLK]));
		mutex_unlock(&csi->subdev_lock);
		break;
	case VIDIOC_SUNXI_CSI_SET_M_CLK:
		break;
	/*CSI external IOCTL*/
	case VIDIOC_VIN_CSI_ONOFF:
		if (*(unsigned long *)arg)
			csi_enable(csi->id);
		else
			csi_disable(csi->id);
		break;
	case VIDIOC_VIN_CSI_IF_CFG:
		csi_if_cfg(csi->id, (struct csi_if_cfg *)arg);
		break;
	case VIDIOC_VIN_CSI_TIMING_CFG:
		csi_timing_cfg(csi->id, (struct csi_timing_cfg *)arg);
		break;
	case VIDIOC_VIN_CSI_FMT_CFG: {
		struct vin_csi_fmt_cfg *fc = (struct vin_csi_fmt_cfg *)arg;
		csi_fmt_cfg(csi->id, fc->ch, &fc->fmt);
		csi_set_size(csi->id, fc->ch, fc->so.length_h, fc->so.length_v,
				fc->so.length_y, fc->so.length_c);
		csi_set_offset(csi->id, fc->ch, fc->so.start_h, fc->so.start_v);
		break;
	}

	case VIDIOC_VIN_CSI_BUF_ADDR_CFG: {
		struct vin_csi_buf_cfg *bc = (struct vin_csi_buf_cfg *)arg;
		if (bc->set)
			csi_set_buffer_address(csi->id, bc->ch,
				bc->buf.buf_sel, bc->buf.addr);
		else
			bc->buf.addr = csi_get_buffer_address(csi->id, bc->ch,
						bc->buf.buf_sel);
		break;
	}

	case VIDIOC_VIN_CSI_CAP_MODE_CFG: {
		struct vin_csi_cap_mode *cm = (struct vin_csi_cap_mode *)arg;
		if (cm->on)
			csi_capture_start(csi->id, cm->total_ch, cm->mode);
		else
			csi_capture_stop(csi->id, cm->total_ch, cm->mode);
		break;
	}

	case VIDIOC_VIN_CSI_CAP_STATUS: {
		struct vin_csi_cap_status *cs = (struct vin_csi_cap_status *)arg;
		csi_capture_get_status(csi->id, cs->ch, &cs->status);
		break;
	}

	case VIDIOC_VIN_CSI_INT_CFG: {
		struct vin_csi_int_cfg *ic = (struct vin_csi_int_cfg *)arg;
		if (ic->en)
			csi_int_enable(csi->id, ic->ch, ic->sel);
		else
			csi_int_disable(csi->id, ic->ch, ic->sel);
		break;
	}

	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops sunxi_csi_core_ops = {
	.s_power = sunxi_csi_subdev_s_power,
	.init = sunxi_csi_addr_init,
	.ioctl = sunxi_csi_subdev_ioctl,
};

static const struct v4l2_subdev_video_ops sunxi_csi_subdev_video_ops = {
	.s_stream = sunxi_csi_subdev_s_stream,
	.cropcap = sunxi_csi_subdev_cropcap,
	.s_mbus_config = sunxi_csi_s_mbus_config,
	.s_parm = sunxi_csi_subdev_s_parm,
};

static const struct v4l2_subdev_pad_ops sunxi_csi_subdev_pad_ops = {
	.enum_mbus_code = sunxi_csi_enum_mbus_code,
	.get_fmt = sunxi_csi_subdev_get_fmt,
	.set_fmt = sunxi_csi_subdev_set_fmt,
};

static struct v4l2_subdev_ops sunxi_csi_subdev_ops = {
	.core = &sunxi_csi_core_ops,
	.video = &sunxi_csi_subdev_video_ops,
	.pad = &sunxi_csi_subdev_pad_ops,
};

static int sunxi_csi_subdev_init(struct csi_dev *csi)
{
	struct v4l2_subdev *sd = &csi->subdev;
	int ret;
	mutex_init(&csi->subdev_lock);
	csi->arrange.row = 1;
	csi->arrange.column = 1;
	csi->bus_info.ch_total_num = 1;
	v4l2_subdev_init(sd, &sunxi_csi_subdev_ops);
	sd->grp_id = VIN_GRP_ID_CSI;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_csi.%u", csi->id);
	v4l2_set_subdevdata(sd, csi);

	csi->csi_pads[CSI_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	csi->csi_pads[CSI_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;

	ret = media_entity_init(&sd->entity, CSI_PAD_NUM, csi->csi_pads, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int csi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct csi_dev *csi = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("CSI failed to get of node\n");
		return -ENODEV;
	}
	csi = kzalloc(sizeof(struct csi_dev), GFP_KERNEL);
	if (!csi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	pdev->id = of_alias_get_id(np, "csi");
	if (pdev->id < 0) {
		vin_err("CSI failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}
	csi->id = pdev->id;
	csi->pdev = pdev;
	csi->cur_ch = 0;

	/*just for test because the csi1 is virtual node*/
	csi->base = of_iomap(np, 0);
	if (!csi->base) {
		ret = -EIO;
		goto freedev;
	}

	ret = bsp_csi_set_base_addr(csi->id, (unsigned long)csi->base);
	if (ret < 0)
		goto ehwinit;

	if (__csi_clk_get(csi)) {
		vin_err("csi clock get failed!\n");
		return -ENXIO;
	}
	spin_lock_init(&csi->slock);
	init_waitqueue_head(&csi->wait);
	list_add_tail(&csi->csi_list, &csi_drv_list);
	sunxi_csi_subdev_init(csi);

	platform_set_drvdata(pdev, csi);
	vin_print("csi%d probe end!\n", csi->id);

	return 0;

ehwinit:
	iounmap(csi->base);
freedev:
	kfree(csi);
ekzalloc:
	vin_print("csi probe err!\n");
	return ret;
}

static int csi_remove(struct platform_device *pdev)
{
	struct csi_dev *csi = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &csi->subdev;

	platform_set_drvdata(pdev, NULL);
	v4l2_set_subdevdata(sd, NULL);
	/*just for test because the csi1 is virtual node*/
	if (pdev->id == 0) {
		__csi_pin_release(csi);
		__csi_clk_release(csi);
	}
	mutex_destroy(&csi->subdev_lock);
	if (csi->base)
		iounmap(csi->base);
	list_del(&csi->csi_list);
	kfree(csi);
	return 0;
}

static const struct of_device_id sunxi_csi_match[] = {
	{.compatible = "allwinner,sunxi-csi",},
	{},
};

static struct platform_driver csi_platform_driver = {
	.probe = csi_probe,
	.remove = csi_remove,
	.driver = {
		   .name = CSI_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_csi_match,
		   }
};

void sunxi_csi_dump_regs(struct v4l2_subdev *sd)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	int i = 0;
	printk("Vfe dump CSI regs :\n");
	for (i = 0; i < 0xb0; i = i + 4) {
		if (i % 0x10 == 0)
			printk("0x%08x:    ", i);
		printk("0x%08x, ", readl(csi->base + i));
		if (i % 0x10 == 0xc)
			printk("\n");
	}
}

struct v4l2_subdev *sunxi_csi_get_subdev(int id)
{
	struct csi_dev *csi;
	list_for_each_entry(csi, &csi_drv_list, csi_list) {
		if (csi->id == id) {
			csi->use_cnt++;
			return &csi->subdev;
		}
	}
	return NULL;
}

int sunxi_csi_platform_register(void)
{
	return platform_driver_register(&csi_platform_driver);
}

void sunxi_csi_platform_unregister(void)
{
	platform_driver_unregister(&csi_platform_driver);
	vin_print("csi_exit end\n");
}

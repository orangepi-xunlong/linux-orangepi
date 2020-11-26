/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/sunxi_isp.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include "../platform/platform_cfg.h"
#include "sunxi_isp.h"

#include "../vin-csi/sunxi_csi.h"
#include "../vin-vipp/sunxi_scaler.h"

#include "../vin-video/vin_core.h"
#include "../utility/vin_io.h"
#include "../vin-stat/vin_ispstat.h"
#include "isp_default_tbl.h"

#define ISP_MODULE_NAME "vin_isp"

extern unsigned int isp_reparse_flag;

static LIST_HEAD(isp_drv_list);

#define LARGE_IMAGE_OFF			32

#define MIN_IN_WIDTH			192
#define MIN_IN_HEIGHT			128
#define MAX_IN_WIDTH			4224
#define MAX_IN_HEIGHT			4224

#define ISP_STAT_EN 0

#if defined CONFIG_ARCH_SUN8IW15P1
#define ISP_3D_BUFF 0 /*3d buffer req switch*/
#else
#define ISP_3D_BUFF 1
#endif

static struct isp_pix_fmt sunxi_isp_formats[] = {
	{
		.name = "RAW8 (BGGR)",
		.fourcc = V4L2_PIX_FMT_SBGGR8,
		.depth = {8},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.infmt = ISP_BGGR,
	}, {
		.name = "RAW8 (GBRG)",
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.depth = {8},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.infmt = ISP_GBRG,
	}, {
		.name = "RAW8 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG8,
		.depth = {8},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.infmt = ISP_GRBG,
	}, {
		.name = "RAW8 (RGGB)",
		.fourcc = V4L2_PIX_FMT_SRGGB8,
		.depth = {8},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.infmt = ISP_RGGB,
	}, {
		.name = "RAW10 (BGGR)",
		.fourcc = V4L2_PIX_FMT_SBGGR10,
		.depth = {10},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.infmt = ISP_BGGR,
	}, {
		.name = "RAW10 (GBRG)",
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.depth = {10},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.infmt = ISP_GBRG,
	}, {
		.name = "RAW10 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG10,
		.depth = {10},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.infmt = ISP_GRBG,
	}, {
		.name = "RAW10 (RGGB)",
		.fourcc = V4L2_PIX_FMT_SRGGB10,
		.depth = {10},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.infmt = ISP_RGGB,
	}, {
		.name = "RAW12 (BGGR)",
		.fourcc = V4L2_PIX_FMT_SBGGR12,
		.depth = {12},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.infmt = ISP_BGGR,
	}, {
		.name = "RAW12 (GBRG)",
		.fourcc = V4L2_PIX_FMT_SGBRG12,
		.depth = {12},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.infmt = ISP_GBRG,
	}, {
		.name = "RAW12 (GRBG)",
		.fourcc = V4L2_PIX_FMT_SGRBG12,
		.depth = {12},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.infmt = ISP_GRBG,
	}, {
		.name = "RAW12 (RGGB)",
		.fourcc = V4L2_PIX_FMT_SRGGB12,
		.depth = {12},
		.color = 0,
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.infmt = ISP_RGGB,
	},
};

#if ISP_3D_BUFF
static int isp_3d_pingpong_alloc(struct isp_dev *isp)
{
	int ret = 0;

	isp->d3d_pingpong[0].size = isp->mf.width * isp->mf.height * 4;
	isp->d3d_pingpong[1].size = isp->mf.width * isp->mf.height * 4;

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[0]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf0 requset add failed!\n");
		return -ENOMEM;
	}

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[1]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf1 requset add failed!\n");
		return -ENOMEM;
	}
	return ret;

}
static void isp_3d_pingpong_free(struct isp_dev *isp)
{
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[0]);
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[1]);
}

static int isp_3d_pingpong_update(struct isp_dev *isp)
{
	dma_addr_t addr;
	struct vin_mm tmp;

	tmp = isp->d3d_pingpong[0];
	isp->d3d_pingpong[0] = isp->d3d_pingpong[1];
	isp->d3d_pingpong[1] = tmp;

	addr = (dma_addr_t)isp->d3d_pingpong[0].dma_addr;
	writel(addr >> 2, isp->isp_load.vir_addr + 0xc8);
	addr = (dma_addr_t)isp->d3d_pingpong[1].dma_addr;
	writel(addr >> 2, isp->isp_load.vir_addr + 0xcc);

	return 0;
}
#endif

static int isp_wdr_pingpong_alloc(struct isp_dev *isp)
{
	int ret = 0;

	isp->wdr_pingpong[0].size = isp->mf.width * isp->mf.height * 2;
	isp->wdr_pingpong[1].size = isp->mf.width * isp->mf.height * 2;

	ret = os_mem_alloc(&isp->pdev->dev, &isp->wdr_pingpong[0]);
	if (ret < 0) {
		vin_err("isp wdr pingpong buf0 requset add failed!\n");
		return -ENOMEM;
	}

	ret = os_mem_alloc(&isp->pdev->dev, &isp->wdr_pingpong[1]);
	if (ret < 0) {
		vin_err("isp wdr pingpong buf1 requset add failed!\n");
		return -ENOMEM;
	}
	return ret;

}
static void isp_wdr_pingpong_free(struct isp_dev *isp)
{
	os_mem_free(&isp->pdev->dev, &isp->wdr_pingpong[0]);
	os_mem_free(&isp->pdev->dev, &isp->wdr_pingpong[1]);
}

static int isp_wdr_pingpong_set(struct isp_dev *isp)
{
	dma_addr_t addr;

	addr = (dma_addr_t)isp->wdr_pingpong[0].dma_addr;
	writel(addr >> 2, isp->isp_load.vir_addr + 0xa0);
	addr = (dma_addr_t)isp->wdr_pingpong[1].dma_addr;
	writel(addr >> 2, isp->isp_load.vir_addr + 0xa4);

	return 0;
}


static int sunxi_isp_subdev_set_selection(struct v4l2_subdev *sd,
			    struct v4l2_subdev_pad_config *cfg,
			    struct v4l2_subdev_selection *sel)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	isp->isp_ob.ob_valid.width = cfg->try_crop.width;
	isp->isp_ob.ob_valid.height = cfg->try_crop.height;
	isp->isp_ob.ob_start.hor = cfg->try_crop.left;
	isp->isp_ob.ob_start.ver = cfg->try_crop.top;
	isp->isp_ob.ob_black.width = cfg->try_crop.width + cfg->try_crop.left;
	isp->isp_ob.ob_black.height = cfg->try_crop.height + cfg->try_crop.top;
	return 0;
}

static int sunxi_isp_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (!isp->use_isp)
		return 0;

	return 0;
}

static int sunxi_isp_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &isp->mf;
	struct mbus_framefmt_res *res = (void *)mf->reserved;
	struct v4l2_event event;
#if ISP_STAT_EN
	struct vin_isp_h3a_config config;
#endif
	void *wdr_tbl = isp->isp_lut_tbl.vir_addr + ISP_WDR_GAMMA_FE_MEM_OFS;
	unsigned int load_val;

	if (!isp->use_isp)
		return 0;

	switch (res->res_pix_fmt) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
		vin_log(VIN_LOG_FMT, "%s output fmt is raw, return directly\n", __func__);
		if (isp->isp_dbg.debug_en) {
			bsp_isp_debug_output_cfg(isp->id, 1, isp->isp_dbg.debug_sel);
			break;
		} else {
			return 0;
		}
	default:
		break;
	}

	isp->wdr_mode = res->res_wdr_mode;

	if (enable) {
		isp->h3a_stat.frame_number = 0;
		if (isp->load_flag)
			memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_REG_SIZE);
#if ISP_STAT_EN
		config.buf_size = ISP_STAT_TOTAL_SIZE;
		vin_isp_stat_config(&isp->h3a_stat, &config);
		vin_isp_stat_enable(&isp->h3a_stat, 1);
#endif
		if (isp->large_image == 0) {
#if ISP_3D_BUFF
			if (isp_3d_pingpong_alloc(isp))
				return -ENOMEM;
			isp_3d_pingpong_update(isp);
#endif
		}
		if (isp->wdr_mode != ISP_NORMAL_MODE) {
			if (isp_wdr_pingpong_alloc(isp)) {
#if ISP_3D_BUFF
				isp_3d_pingpong_free(isp);
#endif
				return -ENOMEM;
			}
			isp_wdr_pingpong_set(isp);
			memcpy(wdr_tbl, wdr_dol_tbl, ISP_WDR_GAMMA_FE_MEM_SIZE + ISP_WDR_GAMMA_BE_MEM_SIZE);
		}
		bsp_isp_enable(isp->id, 1);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_irq_enable(isp->id, FINISH_INT_EN | PARA_LOAD_INT_EN | SRC0_FIFO_INT_EN
				     | FRAME_ERROR_INT_EN | FRAME_LOST_INT_EN);

		load_val = bsp_isp_load_update_flag(isp->id);
		if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
			load_val = load_val | WDR_UPDATE;
			bsp_isp_module_enable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_DOL_WDR_MODE);
			bsp_isp_ch_enable(isp->id, ISP_CH1, 1);
		} else if (isp->wdr_mode == ISP_COMANDING_MODE) {
			load_val = load_val | WDR_UPDATE;
			bsp_isp_module_enable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_COMANDING_MODE);
		} else {
			load_val = load_val & ~WDR_UPDATE;
			bsp_isp_module_disable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_NORMAL_MODE);
		}
		if (isp->large_image == 2)
			bsp_isp_module_disable(isp->id, PLTM_EN | TDNF_EN | AE_EN | AWB_EN | AF_EN | HIST_EN);
		bsp_isp_update_table(isp->id, load_val);

		bsp_isp_module_enable(isp->id, SRC0_EN);
		bsp_isp_set_input_fmt(isp->id, isp->isp_fmt->infmt);
		bsp_isp_set_size(isp->id, &isp->isp_ob);
		/*sunxi_isp_dump_regs(sd);*/
		bsp_isp_set_para_ready(isp->id, PARA_READY);
		bsp_isp_set_last_blank_cycle(isp->id, 5);
		bsp_isp_set_speed_mode(isp->id, 3);
		bsp_isp_ch_enable(isp->id, ISP_CH0, 1);
		bsp_isp_capture_start(isp->id);
	} else {
#if ISP_STAT_EN
		vin_isp_stat_enable(&isp->h3a_stat, 0);
#endif
		memset(&event, 0, sizeof(event));
		event.type = V4L2_EVENT_VIN_ISP_OFF;
		event.id = 0;
		v4l2_event_queue(isp->subdev.devnode, &event);
		bsp_isp_capture_stop(isp->id);
		bsp_isp_module_disable(isp->id, SRC0_EN);
		bsp_isp_ch_enable(isp->id, ISP_CH0, 0);
		bsp_isp_irq_disable(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_enable(isp->id, 0);
		if (isp->large_image == 0)
#if ISP_3D_BUFF
			isp_3d_pingpong_free(isp);
#endif
		if (isp->wdr_mode != ISP_NORMAL_MODE) {
			bsp_isp_ch_enable(isp->id, ISP_CH1, 0);
			isp_wdr_pingpong_free(isp);
		}
		isp->f1_after_librun = 0;
	}

	vin_log(VIN_LOG_FMT, "isp%d %s, %d*%d hoff: %d voff: %d code: %x field: %d\n",
		isp->id, enable ? "stream on" : "stream off",
		isp->isp_ob.ob_valid.width, isp->isp_ob.ob_valid.height,
		isp->isp_ob.ob_start.hor, isp->isp_ob.ob_start.ver,
		mf->code, mf->field);

	return 0;
}

static struct isp_pix_fmt *__isp_find_format(const u32 *pixelformat,
							const u32 *mbus_code,
							int index)
{
	struct isp_pix_fmt *fmt, *def_fmt = NULL;
	unsigned int i;
	int id = 0;

	if (index >= (int)ARRAY_SIZE(sunxi_isp_formats))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(sunxi_isp_formats); ++i) {
		fmt = &sunxi_isp_formats[i];
		if (pixelformat && fmt->fourcc == *pixelformat)
			return fmt;
		if (mbus_code && fmt->mbus_code == *mbus_code)
			return fmt;
		if (index == id)
			def_fmt = fmt;
		id++;
	}
	return def_fmt;
}

static struct v4l2_mbus_framefmt *__isp_get_format(struct isp_dev *isp,
					struct v4l2_subdev_pad_config *cfg,
					enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return cfg ? &cfg->try_fmt : NULL;
	return &isp->mf;
}

static struct isp_pix_fmt *__isp_try_format(struct isp_dev *isp,
					struct v4l2_mbus_framefmt *mf)
{
	struct isp_pix_fmt *isp_fmt;
	struct isp_size_settings *ob = &isp->isp_ob;

	isp_fmt = __isp_find_format(NULL, &mf->code, 0);
	if (isp_fmt == NULL)
		isp_fmt = &sunxi_isp_formats[0];

	ob->ob_black.width = mf->width;
	ob->ob_black.height = mf->height;

	if (!isp->large_image) {
		if (isp->id == 1) {
			mf->width = clamp_t(u32, mf->width, MIN_IN_WIDTH, 3264);
			mf->height = clamp_t(u32, mf->height, MIN_IN_HEIGHT, 3264);
		} else {
			mf->width = clamp_t(u32, mf->width, MIN_IN_WIDTH, 4224);
			mf->height = clamp_t(u32, mf->height, MIN_IN_HEIGHT, 4224);
		}
	}

	ob->ob_valid.width = mf->width;
	ob->ob_valid.height = mf->height;
	ob->ob_start.hor = (ob->ob_black.width - ob->ob_valid.width) / 2;
	ob->ob_start.ver = (ob->ob_black.height - ob->ob_valid.height) / 2;

	if (isp->large_image == 2) {
		isp->left_right = 0;
		isp->isp_ob.ob_valid.width = mf->width / 2 + LARGE_IMAGE_OFF;
	}

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	return isp_fmt;
}

static int sunxi_isp_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	isp->capture_mode = cp->capturemode;
	isp->large_image = cp->reserved[2];

	return 0;
}

static int sunxi_isp_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = isp->capture_mode;

	return 0;
}

static int sunxi_isp_enum_mbus_code(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_mbus_code_enum *code)
{
	const struct isp_pix_fmt *fmt;

	fmt = __isp_find_format(NULL, NULL, code->index);
	if (!fmt)
		return -EINVAL;
	code->code = fmt->mbus_code;
	return 0;
}

static int sunxi_isp_enum_frame_size(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (fse->index != 0)
		return -EINVAL;

	fse->min_width = MIN_IN_WIDTH;
	fse->min_height = MIN_IN_HEIGHT;
	if (isp->id == 1) {
		fse->max_width = 3264;
		fse->max_height = 3264;
	} else {
		fse->max_width = MAX_IN_WIDTH;
		fse->max_height = MAX_IN_HEIGHT;
	}
	return 0;
}

static int sunxi_isp_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = __isp_get_format(isp, cfg, fmt->which);
	if (!mf)
		return -EINVAL;

	mutex_lock(&isp->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&isp->subdev_lock);
	return 0;
}

static int sunxi_isp_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct isp_pix_fmt *isp_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = __isp_get_format(isp, cfg, fmt->which);

	if (fmt->pad == ISP_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&isp->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&isp->subdev_lock);
		}
		return 0;
	}
	isp_fmt = __isp_try_format(isp, &fmt->format);
	if (mf) {
		mutex_lock(&isp->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			isp->isp_fmt = isp_fmt;
		mutex_unlock(&isp->subdev_lock);
	}

	return 0;

}

int sunxi_isp_subdev_init(struct v4l2_subdev *sd, u32 val)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (!isp->use_isp)
		return 0;

	if (val && isp->use_cnt++ > 0)
		return 0;
	else if (!val && (isp->use_cnt == 0 || --isp->use_cnt > 0))
		return 0;

	vin_log(VIN_LOG_ISP, "isp%d %s use_cnt = %d.\n", isp->id,
		val ? "init" : "uninit", isp->use_cnt);
	if (val) {
		if (!isp->have_init) {
			memcpy(isp->isp_load.vir_addr, &isp_default_reg[0], 0x400);
			memset(&isp->load_shadow[0], 0, ISP_LOAD_REG_SIZE);
			isp->load_flag = 0;
			isp->have_init = 1;
		}
		isp->h3a_stat.buf[0].empty = 1;
		isp->h3a_stat.buf[0].dma_addr = isp->isp_stat.dma_addr;
		isp->h3a_stat.buf[0].virt_addr = isp->isp_stat.vir_addr;
		bsp_isp_set_statistics_addr(isp->id, (dma_addr_t)isp->isp_stat.dma_addr);
		bsp_isp_set_load_addr(isp->id, (unsigned long)isp->isp_load.dma_addr);
		bsp_isp_set_saved_addr(isp->id, (unsigned long)isp->isp_save.dma_addr);
		bsp_isp_set_table_addr(isp->id, LENS_GAMMA_TABLE, (unsigned long)(isp->isp_tbl.isp_lsc_tbl_dma_addr));
		bsp_isp_set_table_addr(isp->id, DRC_TABLE, (unsigned long)(isp->isp_tbl.isp_drc_tbl_dma_addr));

		INIT_WORK(&isp->isp_isr_bh_task, isp_isr_bh_handle);

		bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
	} else {
		flush_work(&isp->isp_isr_bh_task);
	}
	return 0;
}

static int __isp_set_load_reg(struct v4l2_subdev *sd, struct isp_table_reg_map *reg)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (!isp->use_isp)
		return 0;

	isp->load_flag = 1;
	return copy_from_user(&isp->load_shadow[0], reg->addr, reg->size);
}

static int __isp_set_table1_map(struct v4l2_subdev *sd, struct isp_table_reg_map *tbl)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret;

	if (!isp->use_isp)
		return 0;

	if (isp->isp_lut_tbl.vir_addr == NULL) {
		vin_err("isp->isp_lut_tbl.vir_addr is NULL!\n");
		return -ENOMEM;
	}
	ret = copy_from_user(isp->isp_lut_tbl.vir_addr, tbl->addr, tbl->size);
	if (ret < 0) {
		vin_err("copy table mapping1 from usr error!\n");
		return ret;
	}

	return 0;
}

static int __isp_set_table2_map(struct v4l2_subdev *sd, struct isp_table_reg_map *tbl)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret;

	if (!isp->use_isp)
		return 0;

	if (isp->isp_drc_tbl.vir_addr == NULL) {
		vin_err("isp->isp_drc_tbl.vir_addr is NULL!\n");
		return -ENOMEM;
	}
	ret = copy_from_user(isp->isp_drc_tbl.vir_addr, tbl->addr, tbl->size);
	if (ret < 0) {
		vin_err("copy table mapping2 from usr error!\n");
		return ret;
	}
	return 0;
}

static long sunxi_isp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
				   void *arg)
{
	int ret = 0;

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG:
		ret = __isp_set_load_reg(sd, (struct isp_table_reg_map *)arg);
		break;
	case VIDIOC_VIN_ISP_TABLE1_MAP:
		ret = __isp_set_table1_map(sd, (struct isp_table_reg_map *)arg);
		break;
	case VIDIOC_VIN_ISP_TABLE2_MAP:
		ret = __isp_set_table2_map(sd, (struct isp_table_reg_map *)arg);
		break;
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

#ifdef CONFIG_COMPAT

struct isp_table_reg_map32 {
	compat_caddr_t addr;
	unsigned int size;
};

#define VIDIOC_VIN_ISP_LOAD_REG32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 70, struct isp_table_reg_map32)

#define VIDIOC_VIN_ISP_TABLE1_MAP32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 71, struct isp_table_reg_map32)

#define VIDIOC_VIN_ISP_TABLE2_MAP32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 72, struct isp_table_reg_map32)

static int get_isp_table_reg_map32(struct isp_table_reg_map *kp,
			      struct isp_table_reg_map32 __user *up)
{
	u32 tmp;

	if (!access_ok(VERIFY_READ, up, sizeof(struct isp_table_reg_map32)) ||
	    get_user(kp->size, &up->size) || get_user(tmp, &up->addr))
		return -EFAULT;
	kp->addr = compat_ptr(tmp);
	return 0;
}

static int put_isp_table_reg_map32(struct isp_table_reg_map *kp,
			      struct isp_table_reg_map32 __user *up)
{
	u32 tmp = (u32) ((unsigned long)kp->addr);

	if (!access_ok(VERIFY_WRITE, up, sizeof(struct isp_table_reg_map32)) ||
	    put_user(kp->size, &up->size) || put_user(tmp, &up->addr))
		return -EFAULT;
	return 0;
}

static long isp_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg)
{
	union {
		struct isp_table_reg_map isd;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	vin_log(VIN_LOG_ISP, "%s cmd is %d\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG32:
		cmd = VIDIOC_VIN_ISP_LOAD_REG;
		break;
	case VIDIOC_VIN_ISP_TABLE1_MAP32:
		cmd = VIDIOC_VIN_ISP_TABLE1_MAP;
		break;
	case VIDIOC_VIN_ISP_TABLE2_MAP32:
		cmd = VIDIOC_VIN_ISP_TABLE2_MAP;
		break;
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG:
	case VIDIOC_VIN_ISP_TABLE1_MAP:
	case VIDIOC_VIN_ISP_TABLE2_MAP:
		err = get_isp_table_reg_map32(&karg.isd, up);
		compatible_arg = 0;
		break;
	}

	if (err)
		return err;

	if (compatible_arg)
		err = sunxi_isp_subdev_ioctl(sd, cmd, up);
	else {
		mm_segment_t old_fs = get_fs();

		set_fs(KERNEL_DS);
		err = sunxi_isp_subdev_ioctl(sd, cmd, &karg);
		set_fs(old_fs);
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG:
	case VIDIOC_VIN_ISP_TABLE1_MAP:
	case VIDIOC_VIN_ISP_TABLE2_MAP:
		err = put_isp_table_reg_map32(&karg.isd, up);
		break;
	}

	return err;
}
#endif

/*
 * must reset all the pipeline through isp.
 */
static void __sunxi_isp_reset(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	int i = 0;

	vin_log(VIN_LOG_ISP, "isp%d reset!!!\n", isp->id);

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;

		if (vind->vinc[i]->isp_sel == isp->id) {
			vinc = vind->vinc[i];

			csic_prs_capture_stop(vinc->csi_sel);
			csic_prs_disable(vinc->csi_sel);

			if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_FBC)
				csic_fbc_disable(vinc->vipp_sel);
			else
				csic_dma_disable(vinc->vipp_sel);
			vipp_disable(vinc->vipp_sel);
			vipp_top_clk_en(vinc->vipp_sel, 0);
		}
	}
	bsp_isp_enable(isp->id, 0);
	bsp_isp_capture_stop(isp->id);

	/*****************start*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;

		if (vind->vinc[i]->isp_sel == isp->id) {
			vinc = vind->vinc[i];

			vipp_top_clk_en(vinc->vipp_sel, 1);
			vipp_enable(vinc->vipp_sel);
			if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_FBC)
				csic_fbc_enable(vinc->vipp_sel);
			else
				csic_dma_enable(vinc->vipp_sel);
		}
	}

	bsp_isp_enable(isp->id, 1);
	bsp_isp_capture_start(isp->id);

	csic_prs_enable(vinc->csi_sel);
	csic_prs_capture_start(vinc->csi_sel, 1, &mode);
}

static void sunxi_isp_stat_parse(struct isp_dev *isp)
{
	void *va = NULL;

	if (isp->h3a_stat.active_buf == NULL) {
		vin_log(VIN_LOG_ISP, "stat active buf is NULL, please enable\n");
		return;
	}
	va = isp->h3a_stat.active_buf->virt_addr;
	isp->stat_buf->hist_buf = (void *) (va);
	isp->stat_buf->ae_buf =  (void *) (va + ISP_STAT_AE_MEM_OFS);
	isp->stat_buf->af_buf = (void *) (va + ISP_STAT_AF_MEM_OFS);
	isp->stat_buf->afs_buf = (void *) (va + ISP_STAT_AFS_MEM_OFS);
	isp->stat_buf->awb_buf = (void *) (va + ISP_STAT_AWB_MEM_OFS);
	isp->stat_buf->pltm_buf = (void *) (va + ISP_STAT_PLTM_LST_MEM_OFS);
}

void sunxi_isp_vsync_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_event event;

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_VSYNC;
	event.id = 0;
	v4l2_event_queue(isp->subdev.devnode, &event);
}

void sunxi_isp_frame_sync_isr(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_event event;

	vin_isp_stat_isr_frame_sync(&isp->h3a_stat);

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_FRAME_SYNC;
	event.id = 0;
	v4l2_event_queue(isp->subdev.devnode, &event);

	vin_isp_stat_isr(&isp->h3a_stat);
	/* you should enable the isp stat buf first,
	 * when you want to get the stat buf separate.
	 * user can get stat buf from external ioctl.
	 */
	sunxi_isp_stat_parse(isp);
}

int sunxi_isp_subscribe_event(struct v4l2_subdev *sd,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}

int sunxi_isp_unsubscribe_event(struct v4l2_subdev *sd,
				    struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	return v4l2_event_unsubscribe(fh, sub);
}

static const struct v4l2_subdev_core_ops sunxi_isp_subdev_core_ops = {
	.s_power = sunxi_isp_subdev_s_power,
	.init = sunxi_isp_subdev_init,
	.ioctl = sunxi_isp_subdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = isp_compat_ioctl32,
#endif
	.subscribe_event = sunxi_isp_subscribe_event,
	.unsubscribe_event = sunxi_isp_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops sunxi_isp_subdev_video_ops = {
	.s_parm = sunxi_isp_s_parm,
	.g_parm = sunxi_isp_g_parm,
	.s_stream = sunxi_isp_subdev_s_stream,
};

static const struct v4l2_subdev_pad_ops sunxi_isp_subdev_pad_ops = {
	.enum_mbus_code = sunxi_isp_enum_mbus_code,
	.enum_frame_size = sunxi_isp_enum_frame_size,
	.set_selection = sunxi_isp_subdev_set_selection,
	.get_fmt = sunxi_isp_subdev_get_fmt,
	.set_fmt = sunxi_isp_subdev_set_fmt,
};

static struct v4l2_subdev_ops sunxi_isp_subdev_ops = {
	.core = &sunxi_isp_subdev_core_ops,
	.video = &sunxi_isp_subdev_video_ops,
	.pad = &sunxi_isp_subdev_pad_ops,
};

static int __sunxi_isp_ctrl(struct isp_dev *isp, struct v4l2_ctrl *ctrl)
{
	int ret = 0;

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_HUE:
	case V4L2_CID_AUTO_WHITE_BALANCE:
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTOGAIN:
	case V4L2_CID_GAIN:
	case V4L2_CID_POWER_LINE_FREQUENCY:
	case V4L2_CID_HUE_AUTO:
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
	case V4L2_CID_SHARPNESS:
	case V4L2_CID_CHROMA_AGC:
	case V4L2_CID_COLORFX:
	case V4L2_CID_AUTOBRIGHTNESS:
	case V4L2_CID_BAND_STOP_FILTER:
	case V4L2_CID_ILLUMINATORS_1:
	case V4L2_CID_ILLUMINATORS_2:
	case V4L2_CID_EXPOSURE_AUTO:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
	case V4L2_CID_FOCUS_ABSOLUTE:
	case V4L2_CID_FOCUS_RELATIVE:
	case V4L2_CID_FOCUS_AUTO:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
	case V4L2_CID_WIDE_DYNAMIC_RANGE:
	case V4L2_CID_IMAGE_STABILIZATION:
	case V4L2_CID_ISO_SENSITIVITY:
	case V4L2_CID_ISO_SENSITIVITY_AUTO:
	case V4L2_CID_EXPOSURE_METERING:
	case V4L2_CID_SCENE_MODE:
	case V4L2_CID_3A_LOCK:
	case V4L2_CID_AUTO_FOCUS_START:
	case V4L2_CID_AUTO_FOCUS_STOP:
	case V4L2_CID_AUTO_FOCUS_RANGE:
	case V4L2_CID_FLASH_LED_MODE:
	case V4L2_CID_AUTO_FOCUS_INIT:
	case V4L2_CID_AUTO_FOCUS_RELEASE:
		break;
	default:
		break;
	}
	return ret;
}

#define ctrl_to_sunxi_isp(ctrl) \
	container_of(ctrl->handler, struct isp_dev, ctrls.handler)

static int sunxi_isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct isp_dev *isp = ctrl_to_sunxi_isp(ctrl);
	unsigned long flags;
	int ret;

	vin_log(VIN_LOG_ISP, "%s, val = %d, cur.val = %d\n",
		v4l2_ctrl_get_name(ctrl->id), ctrl->val, ctrl->cur.val);
	spin_lock_irqsave(&isp->slock, flags);
	ret = __sunxi_isp_ctrl(isp, ctrl);
	spin_unlock_irqrestore(&isp->slock, flags);

	return ret;
}

static int sunxi_isp_try_ctrl(struct v4l2_ctrl *ctrl)
{
	/*
	 * to cheat control framework, because of  when ctrl->cur.val == ctrl->val
	 * s_ctrl would not be called
	 */
	if ((ctrl->minimum == 0) && (ctrl->maximum == 1)) {
		if (ctrl->val)
			ctrl->cur.val = 0;
		else
			ctrl->cur.val = 1;
	} else {
		if (ctrl->val == ctrl->maximum)
			ctrl->cur.val = ctrl->val - 1;
		else
			ctrl->cur.val = ctrl->val + 1;
	}

	/*
	 * to cheat control framework, because of  when ctrl->flags is
	 * V4L2_CTRL_FLAG_VOLATILE, s_ctrl would not be called
	 */
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_GAIN:
		if (ctrl->val != ctrl->cur.val)
			ctrl->flags &= ~V4L2_CTRL_FLAG_VOLATILE;
		break;
	default:
		break;
	}
	return 0;
}

static const struct v4l2_ctrl_ops sunxi_isp_ctrl_ops = {
	.s_ctrl = sunxi_isp_s_ctrl,
	.try_ctrl = sunxi_isp_try_ctrl,
};

static const struct v4l2_ctrl_config ae_win_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config af_win_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config wb_gain_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_R_GAIN,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_GR_GAIN,
		.name = "GR GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_GB_GAIN,
		.name = "GB GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_B_GAIN,
		.name = "B GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 1024,
		.step = 1,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config custom_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_HOR_VISUAL_ANGLE,
		.name = "Horizontal Visual Angle",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 360,
		.step = 1,
		.def = 60,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_VER_VISUAL_ANGLE,
		.name = "Vertical Visual Angle",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 360,
		.step = 1,
		.def = 60,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_FOCUS_LENGTH,
		.name = "Focus Length",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1000,
		.step = 1,
		.def = 280,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_INIT,
		.name = "AutoFocus Initial",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_RELEASE,
		.name = "AutoFocus Release",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	},
};
static const s64 iso_qmenu[] = {
	100, 200, 400, 800, 1600, 3200, 6400,
};
static const s64 exp_bias_qmenu[] = {
	-4, -3, -2, -1, 0, 1, 2, 3, 4,
};

int __isp_init_subdev(struct isp_dev *isp)
{
	struct v4l2_ctrl_handler *handler = &isp->ctrls.handler;
	struct v4l2_subdev *sd = &isp->subdev;
	struct sunxi_isp_ctrls *ctrls = &isp->ctrls;
	struct v4l2_ctrl *ctrl;
	int i, ret;

	mutex_init(&isp->subdev_lock);
	v4l2_subdev_init(sd, &sunxi_isp_subdev_ops);
	sd->grp_id = VIN_GRP_ID_ISP;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_isp.%u", isp->id);
	v4l2_set_subdevdata(sd, isp);

	v4l2_ctrl_handler_init(handler, 38 + ARRAY_SIZE(ae_win_ctrls)
		+ ARRAY_SIZE(af_win_ctrls) + ARRAY_SIZE(wb_gain_ctrls)
		+ ARRAY_SIZE(custom_ctrls));

	for (i = 0; i < ARRAY_SIZE(wb_gain_ctrls); i++)
		ctrls->wb_gain[i] = v4l2_ctrl_new_custom(handler,
						&wb_gain_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(wb_gain_ctrls), &ctrls->wb_gain[0]);

	for (i = 0; i < ARRAY_SIZE(ae_win_ctrls); i++)
		ctrls->ae_win[i] = v4l2_ctrl_new_custom(handler,
						&ae_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(ae_win_ctrls), &ctrls->ae_win[0]);

	for (i = 0; i < ARRAY_SIZE(af_win_ctrls); i++)
		ctrls->af_win[i] = v4l2_ctrl_new_custom(handler,
						&af_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(af_win_ctrls), &ctrls->af_win[0]);

	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_BRIGHTNESS, -128, 128, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_CONTRAST, -128, 128, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_SATURATION, -256, 512, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_HUE, -180, 180, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE, 1, 65536 * 16, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTOGAIN, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_GAIN, 16, 6000 * 16, 1, 16);

	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_POWER_LINE_FREQUENCY,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_HUE_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops,
			  V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2800, 10000, 1, 6500);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_SHARPNESS, -32, 32, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_CHROMA_AGC, 0, 1, 1, 1);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_COLORFX,
			       V4L2_COLORFX_SET_CBCR, 0, V4L2_COLORFX_NONE);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTOBRIGHTNESS, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_BAND_STOP_FILTER, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_ILLUMINATORS_1, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_ILLUMINATORS_2, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE_AUTO,
			       V4L2_EXPOSURE_APERTURE_PRIORITY, 0,
			       V4L2_EXPOSURE_AUTO);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE_ABSOLUTE, 1, 30 * 1000000, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FOCUS_ABSOLUTE, 0, 127, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FOCUS_RELATIVE, -127, 127, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FOCUS_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_int_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_EXPOSURE_BIAS,
			       ARRAY_SIZE(exp_bias_qmenu) - 1,
			       ARRAY_SIZE(exp_bias_qmenu) / 2, exp_bias_qmenu);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			       V4L2_WHITE_BALANCE_SHADE, 0,
			       V4L2_WHITE_BALANCE_AUTO);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_IMAGE_STABILIZATION, 0, 1, 1, 0);
	v4l2_ctrl_new_int_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_ISO_SENSITIVITY,
			       ARRAY_SIZE(iso_qmenu) - 1,
			       ARRAY_SIZE(iso_qmenu) / 2 - 1, iso_qmenu);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_ISO_SENSITIVITY_AUTO,
			       V4L2_ISO_SENSITIVITY_AUTO, 0,
			       V4L2_ISO_SENSITIVITY_AUTO);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_EXPOSURE_METERING,
			       V4L2_EXPOSURE_METERING_MATRIX, 0,
			       V4L2_EXPOSURE_METERING_AVERAGE);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_SCENE_MODE,
			       V4L2_SCENE_MODE_TEXT, 0, V4L2_SCENE_MODE_NONE);
	ctrl = v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_3A_LOCK, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_START, 0, 0, 0, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_STOP, 0, 0, 0, 0);
	ctrl = v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_STATUS, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_RANGE,
			       V4L2_AUTO_FOCUS_RANGE_INFINITY, 0,
			       V4L2_AUTO_FOCUS_RANGE_AUTO);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FLASH_LED_MODE,
			       V4L2_FLASH_LED_MODE_RED_EYE, 0,
			       V4L2_FLASH_LED_MODE_NONE);

	for (i = 0; i < ARRAY_SIZE(custom_ctrls); i++)
		v4l2_ctrl_new_custom(handler, &custom_ctrls[i], NULL);

	if (handler->error)
		return handler->error;

	/*sd->entity->ops = &isp_media_ops;*/
	isp->isp_pads[ISP_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	isp->isp_pads[ISP_PAD_SOURCE_ST].flags = MEDIA_PAD_FL_SOURCE;
	isp->isp_pads[ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;

	ret = media_entity_pads_init(&sd->entity, ISP_PAD_NUM, isp->isp_pads);
	if (ret < 0)
		return ret;

	sd->ctrl_handler = handler;
	/*sd->internal_ops = &sunxi_isp_sd_internal_ops;*/
	return 0;
}

static int isp_resource_alloc(struct isp_dev *isp)
{
	struct isp_table_addr *tbl = &isp->isp_tbl;
	void *va, *dma;
	int ret = 0;

	isp->isp_stat.size = ISP_STAT_TOTAL_SIZE + ISP_LOAD_REG_SIZE + ISP_SAVED_REG_SIZE
			+ ISP_TABLE_MAPPING1_SIZE + ISP_TABLE_MAPPING2_SIZE;
	ret = os_mem_alloc(&isp->pdev->dev, &isp->isp_stat);
	if (ret < 0) {
		vin_err("isp statistic buf requset failed!\n");
		return -ENOMEM;
	}
	isp->isp_load.dma_addr = isp->isp_stat.dma_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_load.vir_addr = isp->isp_stat.vir_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_save.dma_addr = isp->isp_load.dma_addr + ISP_LOAD_REG_SIZE;
	isp->isp_save.vir_addr = isp->isp_load.vir_addr + ISP_LOAD_REG_SIZE;
	isp->isp_lut_tbl.dma_addr = isp->isp_save.dma_addr + ISP_SAVED_REG_SIZE;
	isp->isp_lut_tbl.vir_addr = isp->isp_save.vir_addr + ISP_SAVED_REG_SIZE;
	isp->isp_drc_tbl.dma_addr = isp->isp_lut_tbl.dma_addr + ISP_TABLE_MAPPING1_SIZE;
	isp->isp_drc_tbl.vir_addr = isp->isp_lut_tbl.vir_addr + ISP_TABLE_MAPPING1_SIZE;

	va = isp->isp_lut_tbl.vir_addr;
	dma = isp->isp_lut_tbl.dma_addr;
	tbl->isp_lsc_tbl_dma_addr = (void *)(dma + ISP_LSC_MEM_OFS);
	tbl->isp_lsc_tbl_vaddr = (void *)(va + ISP_LSC_MEM_OFS);
	tbl->isp_gamma_tbl_dma_addr = (void *)(dma + ISP_GAMMA_MEM_OFS);
	tbl->isp_gamma_tbl_vaddr = (void *)(va + ISP_GAMMA_MEM_OFS);
	tbl->isp_linear_tbl_dma_addr = (void *)(dma + ISP_LINEAR_MEM_OFS);
	tbl->isp_linear_tbl_vaddr = (void *)(va + ISP_LINEAR_MEM_OFS);

	va = isp->isp_drc_tbl.vir_addr;
	dma = isp->isp_drc_tbl.dma_addr;

	tbl->isp_drc_tbl_dma_addr = (void *)(dma + ISP_DRC_MEM_OFS);
	tbl->isp_drc_tbl_vaddr = (void *)(va + ISP_DRC_MEM_OFS);

	return ret;

}
static void isp_resource_free(struct isp_dev *isp)
{
	os_mem_free(&isp->pdev->dev, &isp->isp_stat);
}

void isp_isr_bh_handle(struct work_struct *work)
{

}

static irqreturn_t isp_isr(int irq, void *priv)
{
	struct isp_dev *isp = (struct isp_dev *)priv;
	unsigned int load_val;
	unsigned long flags;

	if (!isp->use_isp)
		return 0;

	vin_log(VIN_LOG_ISP, "isp%d interrupt, status is 0x%x!!!\n", isp->id,
		bsp_isp_get_irq_status(isp->id, ISP_IRQ_STATUS_ALL));

	spin_lock_irqsave(&isp->slock, flags);

	if (bsp_isp_get_irq_status(isp->id, SRC0_FIFO_OF_PD)) {
		vin_err("isp%d source0 fifo overflow\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, SRC0_FIFO_OF_PD);
		if (bsp_isp_get_irq_status(isp->id, CIN_FIFO_OF_PD)) {
			vin_err("isp%d Cin0 fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, CIN_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, DPC_FIFO_OF_PD)) {
			vin_err("isp%d DPC fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, DPC_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D2D_FIFO_OF_PD)) {
			vin_err("isp%d D2D fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D2D_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, BIS_FIFO_OF_PD)) {
			vin_err("isp%d BIS fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, BIS_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, CNR_FIFO_OF_PD)) {
			vin_err("isp%d CNR fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, CNR_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, PLTM_FIFO_OF_PD)) {
			vin_err("isp%d PLTM fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, PLTM_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_WRITE_FIFO_OF_PD)) {
			vin_err("isp%d D3D cmp write to DDR fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_WRITE_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_READ_FIFO_OF_PD)) {
			vin_err("isp%d D3D umcmp read from DDR fifo empty\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_READ_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_WT2CMP_FIFO_OF_PD)) {
			vin_err("isp%d D3D write to cmp fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_WT2CMP_FIFO_OF_PD);
		}

		if (bsp_isp_get_irq_status(isp->id, WDR_WRITE_FIFO_OF_PD)) {
			vin_err("isp%d WDR cmp write to DDR fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, WDR_WRITE_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, WDR_READ_FIFO_OF_PD)) {
			vin_err("isp%d WDR umcmp read from DDR fifo empty\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, WDR_READ_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, WDR_WT2CMP_FIFO_OF_PD)) {
			vin_err("isp%d WDR write to cmp fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, WDR_WT2CMP_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_HB_PD)) {
			vin_err("isp%d Hblanking is not enough for D3D\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_HB_PD);
		}
		/*isp reset*/
		__sunxi_isp_reset(isp);
	}

	if (bsp_isp_get_irq_status(isp->id, HB_SHROT_PD)) {
		vin_err("isp%d Hblanking is short (less than 96 cycles)\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, HB_SHROT_PD);
	}

	if (bsp_isp_get_irq_status(isp->id, FRAME_ERROR_PD)) {
		vin_err("isp%d frame error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, FRAME_ERROR_PD);
		__sunxi_isp_reset(isp);
	}

	if (bsp_isp_get_irq_status(isp->id, FRAME_LOST_PD)) {
		vin_err("isp%d frame lost\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, FRAME_LOST_PD);
		__sunxi_isp_reset(isp);
	}

	if (bsp_isp_get_irq_status(isp->id, PARA_LOAD_PD)) {
		bsp_isp_clr_irq_status(isp->id, PARA_LOAD_PD);
		bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
		if (isp->load_flag)
			memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_REG_SIZE);

		load_val = bsp_isp_load_update_flag(isp->id);
		if (isp->wdr_mode != ISP_NORMAL_MODE)
			isp_wdr_pingpong_set(isp);
		else {
			vin_log(VIN_LOG_ISP, "please close wdr in normal mode!!\n");
			load_val = load_val & ~WDR_UPDATE;
			bsp_isp_module_disable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_NORMAL_MODE);
		}
		if (isp->large_image == 2) {
			bsp_isp_module_disable(isp->id, PLTM_EN | TDNF_EN | AE_EN | AWB_EN | AF_EN | HIST_EN);
			if (isp->left_right == 1) {
				isp->left_right = 0;
				isp->isp_ob.ob_start.hor -= isp->isp_ob.ob_valid.width - LARGE_IMAGE_OFF * 2;
			} else {
				isp->left_right = 1;
				isp->isp_ob.ob_start.hor += isp->isp_ob.ob_valid.width - LARGE_IMAGE_OFF * 2;
			}
		}
		bsp_isp_set_size(isp->id, &isp->isp_ob);
		bsp_isp_update_table(isp->id, (unsigned short)load_val);
#if ISP_3D_BUFF
		isp_3d_pingpong_update(isp);
#endif
		bsp_isp_set_para_ready(isp->id, PARA_READY);
	}
	spin_unlock_irqrestore(&isp->slock, flags);

	if (bsp_isp_get_irq_status(isp->id, FINISH_PD)) {
		bsp_isp_clr_irq_status(isp->id, FINISH_PD);
		vin_log(VIN_LOG_ISP, "call tasklet schedule!\n");
		schedule_work(&isp->isp_isr_bh_task);
		if (!isp->f1_after_librun) {
			sunxi_isp_frame_sync_isr(&isp->subdev);
			if (isp->h3a_stat.stata_en_flag)
				isp->f1_after_librun = 1;
		} else {
			if (isp->load_flag)
				sunxi_isp_frame_sync_isr(&isp->subdev);
		}
		isp->load_flag = 0;
	}

	return IRQ_HANDLED;
}

static int isp_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct isp_dev *isp = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("ISP failed to get of node\n");
		return -ENODEV;
	}

	isp = kzalloc(sizeof(struct isp_dev), GFP_KERNEL);
	if (!isp) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	isp->stat_buf = kzalloc(sizeof(struct isp_stat_to_user), GFP_KERNEL);
	if (!isp->stat_buf) {
		vin_err("request stat_buf struct failed!\n");
		return -ENOMEM;
	}

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("ISP failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	isp->id = pdev->id;
	isp->pdev = pdev;

	isp->base = of_iomap(np, 0);
	if (!isp->base) {
		isp->is_empty = 1;
	} else {
		isp->is_empty = 0;
		/*get irq resource */
		isp->irq = irq_of_parse_and_map(np, 0);
		if (isp->irq <= 0) {
			vin_err("failed to get ISP IRQ resource\n");
			goto unmap;
		}

		ret = request_irq(isp->irq, isp_isr, IRQF_SHARED, isp->pdev->name, isp);
		if (ret) {
			vin_err("isp%d request irq failed\n", isp->id);
			goto unmap;
		}
		if (isp_resource_alloc(isp) < 0) {
			ret = -ENOMEM;
			goto freeirq;
		}
		bsp_isp_map_reg_addr(isp->id, (unsigned long)isp->base);
		bsp_isp_map_load_dram_addr(isp->id, (unsigned long)isp->isp_load.vir_addr);
	}

	list_add_tail(&isp->isp_list, &isp_drv_list);
	__isp_init_subdev(isp);

	spin_lock_init(&isp->slock);

	ret = vin_isp_h3a_init(isp);
	if (ret < 0) {
		vin_err("VIN H3A initialization failed\n");
			goto free_res;
	}

	platform_set_drvdata(pdev, isp);
	vin_log(VIN_LOG_ISP, "isp%d probe end!\n", isp->id);
	return 0;
free_res:
	isp_resource_free(isp);
freeirq:
	if (!isp->is_empty)
		free_irq(isp->irq, isp);
unmap:
	if (!isp->is_empty)
		iounmap(isp->base);
	else
		kfree(isp->base);
	list_del(&isp->isp_list);
freedev:
	kfree(isp);
ekzalloc:
	vin_err("isp probe err!\n");
	return ret;
}

static int isp_remove(struct platform_device *pdev)
{
	struct isp_dev *isp = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &isp->subdev;

	platform_set_drvdata(pdev, NULL);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	v4l2_set_subdevdata(sd, NULL);

	if (!isp->is_empty) {
		isp_resource_free(isp);
		free_irq(isp->irq, isp);
		if (isp->base)
			iounmap(isp->base);
	}
	list_del(&isp->isp_list);
	kfree(isp->stat_buf);
	vin_isp_h3a_cleanup(isp);
	kfree(isp);
	return 0;
}

static const struct of_device_id sunxi_isp_match[] = {
	{.compatible = "allwinner,sunxi-isp",},
	{},
};

static struct platform_driver isp_platform_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	.driver = {
		   .name = ISP_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_isp_match,
		   },
};

void sunxi_isp_sensor_type(struct v4l2_subdev *sd, int use_isp)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	isp->use_isp = use_isp;
	if (isp->is_empty)
		isp->use_isp = 0;
}

void sunxi_isp_dump_regs(struct v4l2_subdev *sd)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int i = 0;

	pr_info("vin dump ISP regs :\n");
	for (i = 0; i < 0x40; i = i + 4) {
		if (i % 0x10 == 0)
			pr_info("0x%08x:  ", i);
		pr_info("0x%08x, ", readl(isp->base + i));
		if (i % 0x10 == 0xc)
			pr_info("\n");
	}
	for (i = 0x40; i < 0x240; i = i + 4) {
		if (i % 0x10 == 0)
			pr_info("0x%08x:  ", i);
		pr_info("0x%08x, ", readl(isp->isp_load.vir_addr + i));
		if (i % 0x10 == 0xc)
			pr_info("\n");
	}
}

void sunxi_isp_debug(struct v4l2_subdev *sd, struct isp_debug_mode *isp_debug)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	isp->isp_dbg = *isp_debug;
}

struct v4l2_subdev *sunxi_isp_get_subdev(int id)
{
	struct isp_dev *isp;

	list_for_each_entry(isp, &isp_drv_list, isp_list) {
		if (isp->id == id)
			return &isp->subdev;
	}
	return NULL;
}

struct v4l2_subdev *sunxi_stat_get_subdev(int id)
{
	struct isp_dev *isp;

	list_for_each_entry(isp, &isp_drv_list, isp_list) {
		if (isp->id == id)
			return &isp->h3a_stat.sd;
	}
	return NULL;
}

int sunxi_isp_platform_register(void)
{
	return platform_driver_register(&isp_platform_driver);
}

void sunxi_isp_platform_unregister(void)
{
	platform_driver_unregister(&isp_platform_driver);
	vin_log(VIN_LOG_ISP, "isp_exit end\n");
}

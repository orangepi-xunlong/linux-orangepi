// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for KY X1 IPE MODULE
 *
 * Copyright(C) 2023 KY Micro Limited.
 */
#define DEBUG			/* for pr_debug() */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dev.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-dma-sg.h>
#include <linux/media-bus-format.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "ccic_drv.h"
#include "ccic_hwreg.h"
#include "csiphy.h"
#include "ccic_vdev.h"

#ifdef CONFIG_ARCH_ZYNQMP
#include "dptc_drv.h"
#include "dptc_pll_setting.h"
#endif

#define X1_CCIC_DRV_NAME "x1ccic"

#define CAM_ALIGN(a, b)		({ \
								unsigned int ___tmp1 = (a); \
								unsigned int ___tmp2 = (b); \
								unsigned int ___tmp3 = ___tmp1 % ___tmp2; \
								___tmp1 /= ___tmp2; \
								if (___tmp3) \
									___tmp1++; \
								___tmp1 *= ___tmp2; \
								___tmp1; \
							})
static LIST_HEAD(ccic_devices);
static DEFINE_MUTEX(list_lock);
static void ccic_dma_bh_handler(struct ccic_dma_work_struct *ccic_dma_work);

static void ccic_irqmask(struct ccic_ctrl *ctrl, int on)
{
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;
	u32 mask_val = ccic_dev->interrupt_mask_value;

	if (on) {
		ccic_reg_write(ccic_dev, REG_IRQSTAT, mask_val);
		ccic_reg_set_bit(ccic_dev, REG_IRQMASK, mask_val);
	} else {
		ccic_reg_clear_bit(ccic_dev, REG_IRQMASK, mask_val);
	}
}

#ifndef CONFIG_ARCH_ZYNQMP
static int ccic_config_csi2(struct ccic_dev *ccic_dev, struct mipi_csi2 *csi,
			    int enable)
{
	unsigned int dphy5_val = 0;
	unsigned int ctrl0_val = 0;
	int lanes = csi->dphy_desc.nr_lane;

	if (!ccic_dev->csiphy)
		return -EINVAL;

	if (enable) {
		csiphy_start(ccic_dev->csiphy, csi);
		dphy5_val = CSI2_DPHY5_LANE_ENA(lanes);
		dphy5_val = dphy5_val | (dphy5_val << CSI2_DPHY5_LANE_RESC_ENA_SHIFT);
		ctrl0_val = ccic_reg_read(ccic_dev, REG_CSI2_CTRL0);
		ctrl0_val &= ~(CSI2_C0_LANE_NUM_MASK);
		ctrl0_val |= CSI2_C0_LANE_NUM(lanes);
		ctrl0_val |= CSI2_C0_ENABLE;
		ctrl0_val &= ~(CSI2_C0_VLEN_MASK);
		ctrl0_val |= CSI2_C0_VLEN;
		ccic_reg_write(ccic_dev, REG_CSI2_DPHY5, dphy5_val);
		ccic_reg_write(ccic_dev, REG_CSI2_CTRL0, ctrl0_val);
	} else {
		csiphy_stop(ccic_dev->csiphy);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);	//csi off
	}

	return 0;
}

#else /* connected to DPTC daughter board */
/*ipe_fpga_rstn_adr*/
#define REG_CSI2_FPGA_RSTN	(0x1fc)
static int ccic_config_csi2(struct ccic_dev *ccic_dev,
			    struct mipi_csi2 *csi, int enable)
{
	unsigned int ctrl0_val = 0;
	int lanes = 1;
	unsigned int sensor_width = 0, sensor_height = 0;

	if (!enable)
		goto out;

	ctrl0_val = ccic_reg_read(ccic_dev, REG_CSI2_CTRL0);
	ctrl0_val &= ~(CSI2_C0_LANE_NUM_MASK);
	ctrl0_val |= CSI2_C0_LANE_NUM(lanes);
	ctrl0_val |= CSI2_C0_ENABLE;
	ctrl0_val &= ~(CSI2_C0_VLEN_MASK);
	ctrl0_val |= CSI2_C0_VLEN;
	ccic_reg_write(ccic_dev, REG_CSI2_CTRL0, ctrl0_val);

out:
	if (!enable) {
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);	//csi off
		DPTC_func3_close();
	} else {
		DPTC_func3_open();
		/* csi_ccic_fpga_reset:bit[5:0] = 00 */
		ccic_reg_write_mask(ccic_dev, REG_CSI2_FPGA_RSTN, 0, 0x3f);
		/* udelay(10); */
		dptc_csi_reg_setting(16, sensor_width, sensor_height, lanes);
		/*csi_ccic_fpga_release: bit[5:0] = b11 */
		ccic_reg_write_mask(ccic_dev, REG_CSI2_FPGA_RSTN, 0x3f, 0x3f);
		/* udelay(10); */
	}

	return 0;
}
#endif

static int ccic_config_csi2_dphy(struct ccic_ctrl *ctrl,
				 struct mipi_csi2 *csi, int enable)
{
	int ret = 0;
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	ret = ccic_config_csi2(ccic_dev, csi, enable);
	if (ret)
		dev_err(ccic_dev->dev, "csi2 config failed\n");

	return ret;
}

static int ccic_config_csi2_vc_dt(struct ccic_ctrl *ctrl, int md, u8 vc0, u8 vc1, u8 dt0, u8 dt1)
{
	int ret = 0;
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	switch (md) {
	case CCIC_CSI2VC_NM:	/* Normal mode */
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_VCCTRL, CSI2_VCCTRL_DT_ENABLE);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_DT_FLT, CSI2_DT_FLT0_EN);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_DT_FLT, CSI2_DT_FLT1_EN);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL,
				    CSI2_VCCTRL_MD_NORMAL, CSI2_VCCTRL_MD_MASK);
		break;
	case CCIC_CSI2VC_VC:	/* Virtual Channel mode */
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_VCCTRL, CSI2_VCCTRL_DT_ENABLE);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_DT_FLT, CSI2_DT_FLT0_EN);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_DT_FLT, CSI2_DT_FLT1_EN);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL,
				    CSI2_VCCTRL_MD_VC, CSI2_VCCTRL_MD_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL, vc0 << 14,
				    CSI2_VCCTRL_VC0_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL, vc1 << 22,
				    CSI2_VCCTRL_VC1_MASK);
		break;
	case CCIC_CSI2VC_DT:	/* TODO: Data-Type Interleaving */
		//ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL,
		//		    CSI2_VCCTRL_MD_DT, CSI2_VCCTRL_MD_MASK);
		pr_err("csi2 vc mode %d todo\n", md);
		ret = -EINVAL;
		break;
	case CCIC_CSI2VC_VCDT:
		ccic_reg_set_bit(ccic_dev, REG_CSI2_VCCTRL, CSI2_VCCTRL_DT_ENABLE);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_DT_FLT, CSI2_DT_FLT0_EN);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_DT_FLT, CSI2_DT_FLT2_EN);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL, vc0 << 14,
				    CSI2_VCCTRL_VC0_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL, vc1 << 22,
				    CSI2_VCCTRL_VC1_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_DT_FLT, dt0 << CSI2_DT_FLT0_SHIFT,
				    CSI2_DT_FLT0_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_DT_FLT, dt1 << CSI2_DT_FLT2_SHIFT,
				    CSI2_DT_FLT2_MASK);
		break;
	default:
		dev_err(ccic_dev->dev, "invalid csi2 vc mode %d\n", md);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * ccic_config_idi_mux - DPCM/Repack Mux Select
 *
 * @ctrl: ccic controller
 * @mux: ipe mux path
 *
 * Return: 0 on success, error code otherwise.
 */
static int ccic_config_idi_mux(struct ccic_ctrl *ctrl, int mux)
{
	int ret = 0;
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	/*
	 * IPE1-->
	 * 0x0 Select "local" CSI2 main output
	 * 0x1 Select "IPE2" CSI2 VC/DT output
	 * 0x2 Select "IPE2" CSI2 main output
	 * 0x3 Select "IPE3" CSI2 VC/DT output
	 *
	 * IPE3-->
	 * 0x0 Select "local" CSI2 main output
	 * 0x1 Select "IPE2" CSI2 VC/DT output
	 * 0x2 Select "IPE2" CSI2 main output
	 * 0x3 Select "IPE1" CSI2 VC/DT output
	 */
	switch (mux) {
	case CCIC_IDI_MUX_LOCAL_MAIN:
		ccic_reg_write_mask(ccic_dev, REG_CSI2_CTRL2,
				    CSI2_C2_MUX_SEL_LOCAL_MAIN, CSI2_C2_MUX_SEL_MASK);
		break;
	case CCIC_IDI_MUX_IPE2_VCDT:
		ccic_reg_write_mask(ccic_dev, REG_CSI2_CTRL2,
				    CSI2_C2_MUX_SEL_IPE2_VCDT, CSI2_C2_MUX_SEL_MASK);
		break;
	case CCIC_IDI_MUX_IPE2_MAIN:
		ccic_reg_write_mask(ccic_dev, REG_CSI2_CTRL2,
				    CSI2_C2_MUX_SEL_IPE2_MAIN, CSI2_C2_MUX_SEL_MASK);
		break;
	case CCIC_IDI_MUX_REMOTE_VCDT:
		ccic_reg_write_mask(ccic_dev, REG_CSI2_CTRL2,
				    CSI2_C2_MUX_SEL_REMOTE_VCDT, CSI2_C2_MUX_SEL_MASK);
		break;
	default:
		dev_err(ccic_dev->dev, "invalid idi mux %d\n", mux);
		ret = -EINVAL;
	}

	return ret;
}

static int ccic_config_idi_sel(struct ccic_ctrl *ctrl, int sel)
{
	int ret = 0;
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	switch (sel) {
	case CCIC_IDI_SEL_NONE:
		/* ccic_reg_clear_bit(ccic_dev, REG_IDI_CTRL, IDI_RELEASE_RESET); */
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_RST);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_ENA);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_DPCM_ENA);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_VCCTRL, CSI2_VCCTRL_MD_VC);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_CTRL2,
				    CSI2_C2_MUX_SEL_LOCAL_MAIN, CSI2_C2_MUX_SEL_MASK);
		break;
	case CCIC_IDI_SEL_REPACK:
		ccic_reg_write_mask(ccic_dev, REG_IDI_CTRL, IDI_SEL_DPCM_REPACK,
				    IDI_SEL_MASK);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_IDI_MUX_SEL_DPCM);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_RST);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_ENA);
		break;
	case CCIC_IDI_SEL_DPCM:
		ccic_reg_write_mask(ccic_dev, REG_IDI_CTRL,
				    IDI_SEL_DPCM_REPACK, IDI_SEL_MASK);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_IDI_MUX_SEL_DPCM);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_DPCM_ENA);
		break;
	case CCIC_IDI_SEL_PARALLEL:
		ccic_reg_write_mask(ccic_dev, REG_IDI_CTRL,
				    IDI_SEL_PARALLEL, IDI_SEL_MASK);
		break;
	default:
		dev_err(ccic_dev->dev, "IDI source is error %d\n", sel);
		ret = -EINVAL;
	}

	return ret;
}

static int __maybe_unused ccic_enable_csi2idi(struct ccic_ctrl *ctrl)
{
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	ccic_csi2idi_reset(ccic_dev, 0);

	return 0;
}

static int __maybe_unused ccic_disable_csi2idi(struct ccic_ctrl *ctrl)
{
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	ccic_csi2idi_reset(ccic_dev, 1);

	return 0;
}

#define ISP_BUS_CLK_FREQ (307200000)
static int axi_set_clock_rates(struct clk *clock)
{
	long rate;
	int ret;

	rate = clk_round_rate(clock, ISP_BUS_CLK_FREQ);
	if (rate < 0) {
		pr_err("axi clk round rate failed: %ld\n", rate);
		return -EINVAL;
	}

	ret = clk_set_rate(clock, rate);
	if (ret < 0) {
		pr_err("axi clk set rate failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int ccic_dma_clk_enable(struct ccic_dma *dma, int on)
{
	struct ccic_dev *ccic_dev = dma->ccic_dev;
	struct device *dev = &ccic_dev->pdev->dev;
	int ret;

	if (on) {
		ret = pm_runtime_get_sync(dev);
		if (ret < 0)
			return ret;

		ret = clk_prepare_enable(ccic_dev->axi_clk);
		if (ret < 0) {
			pm_runtime_put_sync(dev);
			return ret;
		}
		reset_control_deassert(ccic_dev->isp_ci_reset);

		ret = axi_set_clock_rates(ccic_dev->axi_clk);
		if (ret < 0) {
			pm_runtime_put_sync(dev);
			return ret;
		}
		reset_control_deassert(ccic_dev->isp_ci_reset);
	} else {
		clk_disable_unprepare(ccic_dev->axi_clk);
		reset_control_assert(ccic_dev->isp_ci_reset);
		pm_runtime_put_sync(dev);
	}

	return 0;
}

static int ccic_dma_enable(struct ccic_dma *dma_dev, int enable)
{
	struct ccic_dev *ccic_dev = dma_dev->ccic_dev;

	if (enable) {
		//ccic_reg_set_bit(ccic_dev, REG_IRQMASK, FRAMEIRQS);
		ccic_dma_set_burst(ccic_dev);
		/* 0x3c: enable ccic dma */
		ccic_reg_set_bit(ccic_dev, REG_CTRL0, BIT(0));
		ccic_reg_set_bit(ccic_dev, 0x40, BIT(31) | BIT(26));
		ccic_reg_clear_bit(ccic_dev, 0x40, BIT(25));
	} else {
		//ccic_reg_clear_bit(ccic_dev, REG_IRQMASK, FRAMEIRQS);
		/* 0x3c: disable ccic dma */
		ccic_reg_clear_bit(ccic_dev, REG_CTRL0, BIT(0));
	}

	return 0;
}

static int ccic_dma_set_fmt(struct ccic_dma *dma_dev,
							unsigned int width,
							unsigned int height,
							unsigned int pix_fmt)
{
	struct ccic_dev *ccic_dev = dma_dev->ccic_dev;
	struct device *dev = ccic_dev->dev;
	unsigned int data_fmt = C0_DF_BAYER, imgsz_w = 0, imgsz_h = 0;
	unsigned int stride_y = 0, stride_uv = 0;

	switch (pix_fmt) {
	case MEDIA_BUS_FMT_UYVY8_2X8:
		data_fmt = C0_DF_BAYER;
		imgsz_w = width;
		stride_y = 0;
		imgsz_h = height;
		stride_uv = 0;
		break;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		data_fmt = C0_DF_BAYER;
		imgsz_w = width;
		stride_y = 0;//CAM_ALIGN(imgsz_w, 8);
		imgsz_h = height;
		stride_uv = 0;
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		data_fmt = C0_DF_BAYER;
		imgsz_w = width * 5 / 4;
		stride_y = 0;//CAM_ALIGN(imgsz_w, 8);
		imgsz_h = height;
		stride_uv = 0;
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		data_fmt = C0_DF_BAYER;
		imgsz_w = width * 3 / 2;
		stride_y = 0;//CAM_ALIGN(imgsz_w, 8);
		imgsz_h = height;
		stride_uv = 0;
		break;
	default:
		pr_err("%s failed: invalid pixfmt %d\n", __func__, pix_fmt);
		return -1;
	}

	dev_info(dev, "stride_y=0x%x, width=%u\n", stride_y, width);
	ccic_reg_write(ccic_dev, REG_IMGPITCH, stride_uv << 16 | stride_y);
	ccic_reg_write(ccic_dev, REG_IMGSIZE, imgsz_h << 16 | imgsz_w);
	ccic_reg_write(ccic_dev, REG_IMGOFFSET, 0x0);
	ccic_reg_write_mask(ccic_dev, REG_CTRL0, data_fmt, C0_DF_MASK);
	/* Make sure it knows we want to use hsync/vsync. */
	ccic_reg_write_mask(ccic_dev, REG_CTRL0, C0_SIF_HVSYNC, C0_SIFM_MASK);
	/* Need set following bit for auto-recovery */
	ccic_reg_set_bit(ccic_dev, REG_CTRL0, C0_EOFFLUSH);

	return 0;
}

static int ccic_dma_set_addr(struct ccic_dma *dma_dev,
							unsigned long addr_y,
							unsigned long addr_u,
							unsigned long addr_v)
{
	struct ccic_dev *ccic_dev = dma_dev->ccic_dev;

	ccic_reg_write(ccic_dev, 0x00, (u32)(addr_y & 0xffffffff));
	ccic_reg_write(ccic_dev, 0x0c, (u32)(addr_u & 0xffffffff));
	ccic_reg_write(ccic_dev, 0x18, (u32)(addr_v & 0xffffffff));

	return 0;
}

static int ccic_dma_shadow_ready(struct ccic_dma *dma_dev)
{
	struct ccic_dev *ccic_dev = dma_dev->ccic_dev;
	ccic_reg_set_bit(ccic_dev, REG_CTRL1, C1_SHADOW_RDY);

	return 0;
}

static int ccic_dma_src_select(struct ccic_dma *dma_dev, int src, unsigned int main_ccic_id)
{
	struct ccic_dev *ccic_dev = dma_dev->ccic_dev;
	return ccic_dma_src_sel(ccic_dev, src, main_ccic_id);
}

static void ccic_dma_dump_regs(struct ccic_dma *dma_dev)
{
	unsigned int reg_val = 0;
	struct ccic_dev *ccic_dev = dma_dev->ccic_dev;

	reg_val = ccic_reg_read(ccic_dev, 0x30);
	printk(KERN_INFO "ccic%d [0x30]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x28);
	printk(KERN_INFO "ccic%d [0x28]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x2c);
	printk(KERN_INFO "ccic%d [0x2c]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x24);
	printk(KERN_INFO "ccic%d [0x24]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x34);
	printk(KERN_INFO "ccic%d [0x34]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x38);
	printk(KERN_INFO "ccic%d [0x38]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x3c);
	printk(KERN_INFO "ccic%d [0x3c]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x40);
	printk(KERN_INFO "ccic%d [0x40]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x44);
	printk(KERN_INFO "ccic%d [0x44]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x48);
	printk(KERN_INFO "ccic%d [0x48]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x310);
	printk(KERN_INFO "ccic%d [0x310]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x60);
	printk(KERN_INFO "ccic%d [0x60]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x23c);
	printk(KERN_INFO "ccic%d [0x23c]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x128);
	printk(KERN_INFO "ccic%d [0x128]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x12c);
	printk(KERN_INFO "ccic%d [0x12c]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x134);
	printk(KERN_INFO "ccic%d [0x134]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x138);
	printk(KERN_INFO "ccic%d [0x138]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x100);
	printk(KERN_INFO "ccic%d [0x100]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x140);
	printk(KERN_INFO "ccic%d [0x140]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x144);
	printk(KERN_INFO "ccic%d [0x144]=0x%08x\n", ccic_dev->index, reg_val);
	reg_val = ccic_reg_read(ccic_dev, 0x124);
	printk(KERN_INFO "ccic%d [0x124]=0x%08x\n", ccic_dev->index, reg_val);
}
static struct ccic_dma_ops ccic_dma_ops = {
	.set_fmt = ccic_dma_set_fmt,
	.shadow_ready = ccic_dma_shadow_ready,
	.set_addr = ccic_dma_set_addr,
	.ccic_enable = ccic_dma_enable,
	.clk_enable = ccic_dma_clk_enable,
	.src_sel = ccic_dma_src_select,
	.dump_regs = ccic_dma_dump_regs,
};

/*
 * TBD: calculate the clk rate dynamically based on
 * fps, resolution and other arguments.
 */
static int ccic_clk_set_rate(struct ccic_ctrl *ctrl_dev, int mode)
{
	unsigned long clk_val;
	struct ccic_dev *ccic_dev = ctrl_dev->ccic_dev;

#if defined(CONFIG_KY_ZEBU)
	clk_val = clk_round_rate(ccic_dev->csi_clk, 1248000000);
#else
	clk_val = clk_round_rate(ccic_dev->csi_clk, 624000000);
#endif

	clk_set_rate(ccic_dev->csi_clk, clk_val);
	pr_debug("cam clk[csi_func]: %ld\n", clk_val);

	if (mode == SC2_MODE_ISP) {
#if defined(CONFIG_KY_ZEBU)
		clk_val = clk_round_rate(ccic_dev->clk4x, 1248000000);
#else
		clk_val = clk_round_rate(ccic_dev->csi_clk, 624000000);
#endif

		clk_set_rate(ccic_dev->clk4x, clk_val);
		pr_debug("cam clk[ccic4x_func]: %ld\n", clk_val);
	}

	return 0;
}

static int ccic_clk_enable(struct ccic_ctrl *ctrl, int en)
{
	int ret = 0;
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;

	if (en) {
		ret = pm_runtime_get_sync(&ccic_dev->pdev->dev);
		if (ret < 0) {
			pr_err("rpm get failed\n");
			return ret;
		}
		pm_stay_awake(&ccic_dev->pdev->dev);

		clk_prepare_enable(ccic_dev->dpu_clk);
		reset_control_deassert(ccic_dev->mclk_reset);

		//clk_prepare_enable(ccic_dev->ahb_clk);
		reset_control_deassert(ccic_dev->ahb_reset);
		clk_prepare_enable(ccic_dev->clk4x);
		reset_control_deassert(ccic_dev->ccic_4x_reset);
		clk_prepare_enable(ccic_dev->csi_clk);
		reset_control_deassert(ccic_dev->csi_reset);

		ret = ccic_clk_set_rate(ctrl, SC2_MODE_ISP);
		if (ret < 0) {
			pm_runtime_put_sync(&ccic_dev->pdev->dev);
			return ret;
		}

	} else {
		clk_disable_unprepare(ccic_dev->csi_clk);
		reset_control_assert(ccic_dev->csi_reset);

		clk_disable_unprepare(ccic_dev->clk4x);
		reset_control_assert(ccic_dev->ccic_4x_reset);

		clk_disable_unprepare(ccic_dev->dpu_clk);
		reset_control_assert(ccic_dev->mclk_reset);

		//clk_disable_unprepare(ccic_dev->ahb_clk);
		reset_control_assert(ccic_dev->ahb_reset);
		pm_relax(&ccic_dev->pdev->dev);
		pm_runtime_put_sync(&ccic_dev->pdev->dev);
	}

	pr_debug("ccic%d clock %s", ccic_dev->index, en ? "enabled" : "disabled");

	return ret;
}

static int ccic_config_csi2_mbus(struct ccic_ctrl *ctrl, int md, u8 vc0, u8 vc1, u8 dt0, u8 dt1,
			  int lanes)
{
	int ret;
	struct ccic_dev *ccic_dev = ctrl->ccic_dev;
	struct mipi_csi2 csi2para;

	ret = ccic_config_csi2_vc_dt(ctrl, md, vc0, vc1, dt0, dt1);
	if (ret)
		return ret;

	csi2para.calc_dphy = 0;
	csi2para.dphy[0] = 0x00000001;
	csi2para.dphy[1] = 0xa2848888;
	csi2para.dphy[2] = 0x0000201a;	//0x0000201a: 1.0G ~ 1.5G
	csi2para.dphy[3] = 0x000000ff;
	csi2para.dphy[4] = 0x1001;
	csi2para.dphy_desc.nr_lane = lanes;
	ret = ccic_config_csi2_dphy(ctrl, &csi2para, !!lanes);
	if (ret)
		return ret;

	pr_debug("ccic%d csi2 %s", ccic_dev->index, lanes ? "enabled" : "disabled");

	return ret;
}

static int ccic_config_csi2idi_mux(struct ccic_ctrl *ctrl, int chnl, int idi, int en)
{
	struct ccic_dev *csi2idi = NULL;
	struct ccic_dev *tmp;
	int csi2idi_idx;

	if (idi == CCIC_CSI2IDI0) {
		csi2idi_idx = 0;
	} else if (idi == CCIC_CSI2IDI1) {
		csi2idi_idx = 2;
	} else {
		pr_err("%s: invalid idi index %d\n", __func__, idi);
		return -EINVAL;
	}

	list_for_each_entry(tmp, &ccic_devices, list) {
		if (tmp->index == csi2idi_idx) {
			csi2idi = tmp;
			break;
		}
	}

	if (!csi2idi) {
		pr_err("%s: ccic%d not found\n", __func__, csi2idi_idx);
		return -ENODEV;
	}

	if (!en) {
		ccic_csi2idi_reset(csi2idi, 1);
		return 0;
	}

	ccic_config_idi_sel(csi2idi->ctrl, CCIC_IDI_SEL_REPACK);
	if (ctrl->ccic_dev->index == csi2idi->index) {
		ccic_config_idi_mux(csi2idi->ctrl, CCIC_IDI_MUX_LOCAL_MAIN);
	} else if (ctrl->ccic_dev->index == 1) {
		if (chnl == CCIC_CSI2VC_MAIN)
			ccic_config_idi_mux(csi2idi->ctrl, CCIC_IDI_MUX_IPE2_MAIN);
		else
			ccic_config_idi_mux(csi2idi->ctrl, CCIC_IDI_MUX_IPE2_VCDT);
	} else {
		ccic_config_idi_mux(csi2idi->ctrl, CCIC_IDI_MUX_REMOTE_VCDT);
	}

	ccic_csi2idi_reset(csi2idi, 0);
	return 0;
}

static int ccic_reset_csi2idi(struct ccic_ctrl *ctrl, int idi, int rst)
{
	struct ccic_dev *csi2idi = NULL;
	struct ccic_dev *tmp;
	int csi2idi_idx;

	if (idi == 0) {
		csi2idi_idx = 0;
	} else if (idi == 1) {
		csi2idi_idx = 2;
	} else {
		pr_err("%s: invalid idi index %d\n", __func__, idi);
		return -EINVAL;
	}

	list_for_each_entry(tmp, &ccic_devices, list) {
		if (tmp->index == csi2idi_idx) {
			csi2idi = tmp;
			break;
		}
	}

	if (!csi2idi) {
		pr_err("%s: ccic%d not found\n", __func__, csi2idi_idx);
		return -ENODEV;
	}

	ccic_csi2idi_reset(csi2idi, rst);
	return 0;
}

static struct ccic_ctrl_ops ccic_ctrl_ops = {
	.irq_mask = ccic_irqmask,
	.clk_enable = ccic_clk_enable,
	.config_csi2_mbus = ccic_config_csi2_mbus,
	.config_csi2idi_mux = ccic_config_csi2idi_mux,
	.reset_csi2idi = ccic_reset_csi2idi,
};

static int ccic_init_clk(struct ccic_dev *dev)
{
#ifdef CONFIG_ARCH_KY
	dev->axi_clk = devm_clk_get(&dev->pdev->dev, "isp_axi");
	if (IS_ERR(dev->axi_clk))
		return PTR_ERR(dev->axi_clk);
/*
	dev->ahb_clk = devm_clk_get(&dev->pdev->dev, "isp_ahb");
	if (IS_ERR(dev->ahb_clk))
		return PTR_ERR(dev->ahb_clk);
*/
	dev->ahb_reset = devm_reset_control_get_optional_shared(&dev->pdev->dev, "isp_ahb_reset");
	if (IS_ERR_OR_NULL(dev->ahb_reset))
		return PTR_ERR(dev->ahb_reset);

	dev->csi_reset = devm_reset_control_get_optional_shared(&dev->pdev->dev, "csi_reset");
	if (IS_ERR_OR_NULL(dev->csi_reset))
		return PTR_ERR(dev->csi_reset);

	dev->ccic_4x_reset = devm_reset_control_get_optional_shared(&dev->pdev->dev, "ccic_4x_reset");
	if (IS_ERR_OR_NULL(dev->ccic_4x_reset))
		return PTR_ERR(dev->ccic_4x_reset);

	dev->isp_ci_reset = devm_reset_control_get_optional_shared(&dev->pdev->dev, "isp_ci_reset");
	if (IS_ERR_OR_NULL(dev->isp_ci_reset))
		return PTR_ERR(dev->isp_ci_reset);

	dev->mclk_reset = devm_reset_control_get_optional_shared(&dev->pdev->dev, "mclk_reset");
	if (IS_ERR_OR_NULL(dev->mclk_reset))
		return PTR_ERR(dev->mclk_reset);

	dev->csi_clk = devm_clk_get(&dev->pdev->dev, "csi_func");
	if (IS_ERR(dev->csi_clk))
		return PTR_ERR(dev->csi_clk);
	dev->dpu_clk = devm_clk_get(&dev->pdev->dev, "dpu_mclk");
	if (IS_ERR(dev->dpu_clk))
		return PTR_ERR(dev->dpu_clk);

	dev->clk4x = devm_clk_get(&dev->pdev->dev, "ccic_func");
	return PTR_ERR_OR_ZERO(dev->clk4x);
#else
	return 0;
#endif
}

static int ccic_device_register(struct ccic_dev *ccic_dev)
{
	struct ccic_dev *other;

	mutex_lock(&list_lock);
	list_for_each_entry(other, &ccic_devices, list) {
		if (other->index == ccic_dev->index) {
			dev_warn(ccic_dev->dev, "ccic%d already registered\n",
				 ccic_dev->index);
			mutex_unlock(&list_lock);
			return -EBUSY;
		}
	}

	list_add_tail(&ccic_dev->list, &ccic_devices);
	mutex_unlock(&list_lock);
	return 0;
}

static int ccic_device_unregister(struct ccic_dev *ccic_dev)
{
	mutex_lock(&list_lock);
	list_del(&ccic_dev->list);
	mutex_unlock(&list_lock);
	return 0;
}

int ccic_dphy_hssettle_set(unsigned int ccic_id, unsigned int dphy_freg)
{
	u32 reg_settle = 0x00002b00;
	struct ccic_dev *ccic_dev = NULL;
	struct ccic_dev *tmp;

	if (dphy_freg < 80)	//dphy_clock uint: MHZ
		return -EINVAL;
	// RX_Tsettle > TX_HSprepare; reg uint: (1 / (dphy_clock / 2))
	reg_settle =
	    (HS_PREP_ZERO_MIN + HS_PREP_MAX) * dphy_freg / (2 * 2 * 1000) + (6 / 2);
	reg_settle = reg_settle << CSI2_DPHY3_HS_SETTLE_SHIFT;

	mutex_lock(&list_lock);
	list_for_each_entry(tmp, &ccic_devices, list) {
		if (tmp->index == ccic_id) {
			ccic_dev = tmp;
			break;
		}
	}
	if (!ccic_dev) {
		pr_err("ccic%d not found", ccic_id);
		return -ENODEV;
	}
	ccic_reg_write(ccic_dev, REG_CSI2_DPHY3, reg_settle);
	mutex_unlock(&list_lock);

	return 0;
}

EXPORT_SYMBOL_GPL(ccic_dphy_hssettle_set);

int ccic_ctrl_get(struct ccic_ctrl **ctrl_host, int id,
		  irqreturn_t(*handler) (struct ccic_ctrl *, u32))
{
	struct ccic_dev *ccic_dev = NULL;
	struct ccic_dev *tmp;
	struct ccic_ctrl *ctrl = NULL;

	list_for_each_entry(tmp, &ccic_devices, list) {
		if (tmp->index == id) {
			ccic_dev = tmp;
			break;
		}
	}
	if (!ccic_dev) {
		pr_err("ccic%d not found", id);
		return -ENODEV;
	}

	ctrl = ccic_dev->ctrl;
	ctrl->handler = handler;
	*ctrl_host = ctrl;
	pr_debug("acquire ccic%d ctrl dev succeed\n", id);

	return 0;
}

EXPORT_SYMBOL(ccic_ctrl_get);

int ccic_dma_get(struct ccic_dma **ccic_dma, int id)
{
	struct ccic_dev *ccic_dev = NULL;
	struct ccic_dev *tmp = NULL;
	struct ccic_dma *dma = NULL;

	list_for_each_entry(tmp, &ccic_devices, list) {
		if (tmp->index == id) {
			ccic_dev = tmp;
			break;
		}
	}
	if (!ccic_dev) {
		pr_err("ccic%d not found", id);
		return -ENODEV;
	}

	dma = ccic_dev->dma;
	*ccic_dma = dma;
	pr_debug("acquire ccic%d dma dev succeed\n", id);

	return 0;
}
EXPORT_SYMBOL(ccic_dma_get);
static void ipe_error_irq_handler(struct ccic_dev *ccic, u32 ipestatus, u32 csi2status)
{
	static DEFINE_RATELIMIT_STATE(rs, 5 * HZ, 20);

	if (__ratelimit(&rs)) {
		pr_err("CCIC%d: interrupt status 0x%08x, csi2 status 0x%08x\n",
		       ccic->index, ipestatus, csi2status);
	} else {
		/* aovid soft lockup due to high frequency interrupt */
		ccic_reg_clear_bit(ccic, REG_IRQMASK, CSI2PHYERRS);
		pr_err("CCIC%d: too many interrupt errors, mask\n", ccic->index);
	}
}

static int ccic_put_dma_work(struct ccic_dma_context *dma_ctx,
                            struct ccic_dma_work_struct *ccic_dma_work)
{
    unsigned long flags = 0;

    spin_lock_irqsave(&dma_ctx->slock, flags);
    list_del_init(&ccic_dma_work->busy_list_entry);
    list_add(&ccic_dma_work->idle_list_entry, &dma_ctx->dma_work_idle_list);
    spin_unlock_irqrestore(&dma_ctx->slock, flags);

    return 0;
}

static int ccic_get_dma_work(struct ccic_dma_context *dma_ctx,
							struct ccic_dma_work_struct **ccic_dma_work)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&dma_ctx->slock, flags);
	*ccic_dma_work = list_first_entry_or_null(&dma_ctx->dma_work_idle_list, struct ccic_dma_work_struct, idle_list_entry);
	if (NULL == *ccic_dma_work) {
		spin_unlock_irqrestore(&dma_ctx->slock, flags);
		return -1;
	}
	list_del_init(&((*ccic_dma_work)->idle_list_entry));
	list_add(&((*ccic_dma_work)->busy_list_entry), &dma_ctx->dma_work_busy_list);
	spin_unlock_irqrestore(&dma_ctx->slock, flags);

	return 0;
}

static void ccic_dma_bh_handler(struct ccic_dma_work_struct *ccic_dma_work)
{
	struct spm_ccic_vnode *ac_vnode = ccic_dma_work->ac_vnode;
	struct device *dev = ac_vnode->ccic_dev->dev;
	struct ccic_dma_context *dma_ctx = &ac_vnode->dma_ctx;
	struct spm_ccic_vbuffer *n = NULL, *pos = NULL;
	//unsigned int irq_status = ccic_dma_work->irq_status;
	LIST_HEAD(export_list);
	unsigned long flags = 0;

	//spin_lock(&ac_vnode->waitq_head.lock);
	spin_lock_irqsave(&ac_vnode->waitq_head.lock, flags);
	ac_vnode->in_tasklet = 1;
	if (ac_vnode->in_streamoff || !ac_vnode->is_streaming) {
		wake_up_locked(&ac_vnode->waitq_head);
		spin_unlock(&ac_vnode->waitq_head.lock);
		goto dma_tasklet_finish;
	}
	wake_up_locked(&ac_vnode->waitq_head);
//	spin_unlock(&ac_vnode->waitq_head.lock);
	spin_unlock_irqrestore(&ac_vnode->waitq_head.lock, flags);

	spin_lock_irqsave(&ac_vnode->slock, flags);
	list_for_each_entry_safe(pos, n, &ac_vnode->busy_list, list_entry) {
		if (pos->flags & (AC_BUF_FLAG_HW_ERR | AC_BUF_FLAG_SW_ERR | AC_BUF_FLAG_DONE_TOUCH)) {
			list_del_init(&(pos->list_entry));
			atomic_dec(&ac_vnode->busy_buf_cnt);
			list_add_tail(&(pos->list_entry), &export_list);
		}
	}
	spin_unlock_irqrestore(&ac_vnode->slock, flags);
	list_for_each_entry_safe(pos, n, &export_list, list_entry) {
		if (!(pos->flags & AC_BUF_FLAG_SOF_TOUCH)) {
			dev_warn(dev, "%s export buf index=%u frameid=%u without sof touch\n", ac_vnode->name, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence);
		}
		if (pos->flags & AC_BUF_FLAG_HW_ERR) {
			//pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_ERROR_HW;
			dev_warn(dev, "%s export buf index=%u frameid=%u with hw error\n", ac_vnode->name, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence);
			spm_cvdev_export_ccic_vbuffer(pos, 1);
			ac_vnode->hw_err_frm++;
		} else if (pos->flags & AC_BUF_FLAG_SW_ERR) {
			//pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_ERROR_SW;
			dev_warn(dev, "%s export buf index=%u frameid=%u with sw error\n", ac_vnode->name, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence);
			spm_cvdev_export_ccic_vbuffer(pos, 1);
			ac_vnode->sw_err_frm++;
		} else if (pos->flags & AC_BUF_FLAG_DONE_TOUCH) {
			spm_cvdev_export_ccic_vbuffer(pos, 0);
			ac_vnode->ok_frm++;
		}
	}
dma_tasklet_finish:
	if (ac_vnode) {
		spin_lock_irqsave(&ac_vnode->waitq_head.lock, flags);
		//spin_lock(&ac_vnode->waitq_head.lock);
		ac_vnode->in_tasklet = 0;
		wake_up_locked(&ac_vnode->waitq_head);
		//spin_unlock(&ac_vnode->waitq_head.lock);
		spin_unlock_irqrestore(&ac_vnode->waitq_head.lock, flags);
	}
	ccic_put_dma_work(dma_ctx, ccic_dma_work);
}

static void ccic_dma_tasklet_handler(unsigned long param)
{
	struct ccic_dma_work_struct *ccic_dma_work = (struct ccic_dma_work_struct*)param;
	ccic_dma_bh_handler(ccic_dma_work);
}
static irqreturn_t x1_ccic_isr(int irq, void *data)
{
	struct ccic_dev *ccic_dev = data;
	struct spm_ccic_vnode *ac_vnode = (struct spm_ccic_vnode*)ccic_dev->vnode;
	struct ccic_dma_context *dma_ctx = &ac_vnode->dma_ctx;
	struct spm_ccic_vbuffer *pos = NULL, *ac_vb = NULL;
	struct ccic_dma_work_struct *ccic_dma_work = NULL;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	struct device *dev = ac_vnode->ccic_dev->dev;
	uint32_t irqs = 0, csi2status = 0, tmp = 0;
	int ret = 0;

	irqs = ccic_reg_read(ccic_dev, REG_IRQSTAT);
	if (!(irqs & ~IRQ_IDI_PRO_LINE))
		return IRQ_NONE;

	csi2status = ccic_reg_read(ccic_dev, 0x108);
	ccic_reg_write(ccic_dev, REG_IRQSTAT, irqs & ~IRQ_IDI_PRO_LINE);

	if (irqs & CSI2PHYERRS)
		ipe_error_irq_handler(ccic_dev, irqs, csi2status);

	if (irqs & IRQ_DMA_PRO_LINE)
		pr_debug("CCIC%d: IRQ_DMA_PRO_LINE\n", ccic_dev->index);

	//if (irqs & IRQ_IDI_PRO_LINE)
	//	pr_debug("CCIC%d: IRQ_IDI_PRO_LINE\n", ccic_dev->index);

	if (irqs & IRQ_CSI2IDI_FLUSH)
		pr_debug("CCIC%d: IRQ_CSI2IDI_FLUSH\n", ccic_dev->index);

	if (irqs & IRQ_CSI2IDI_HBLK2HSYNC)
		pr_debug("CCIC%d: IRQ_CSI2IDI_HBLK2HSYNC\n", ccic_dev->index);

	if (irqs & IRQ_DPHY_RX_CLKULPS_ACTIVE)
		pr_debug("CCIC%d: IRQ_DPHY_RX_CLKULPS_ACTIVE\n", ccic_dev->index);

	if (irqs & IRQ_DPHY_RX_CLKULPS)
		pr_debug("CCIC%d: IRQ_DPHY_RX_CLKULPS\n", ccic_dev->index);

	if (irqs & IRQ_DPHY_LN_ULPS_ACTIVE)
		pr_debug("CCIC%d: IRQ_DPHY_LN_ULPS_ACTIVE\n", ccic_dev->index);

	//if (irqs & IRQ_DMA_SOF) {
	//	dev_dbg(dev, "CCIC%d: IRQ_DMA_SOF\n", ccic_dev->index);
	//}
	if (irqs & IRQ_DMA_SOF || irqs & IRQ_SHADOW_NOT_RDY) {
		ac_vnode->frame_id++;
		ac_vnode->total_frm++;
	}
	spin_lock(&ac_vnode->waitq_head.lock);
	ac_vnode->in_irq = 1;
	if (ac_vnode->in_streamoff) {
		ac_vnode->in_irq = 0;
		wake_up_locked(&ac_vnode->waitq_head);
		spin_unlock(&ac_vnode->waitq_head.lock);
		return IRQ_HANDLED;
	}
	spin_unlock(&ac_vnode->waitq_head.lock);
	if (ac_vnode->is_streaming && (irqs & FRAMEIRQS)) {
		//if (irqs & IRQ_CSI_SOF) {
		if (irqs & IRQ_DMA_SOF || irqs & IRQ_SHADOW_NOT_RDY) {
			spm_cvdev_dq_idle_vbuffer(ac_vnode, &ac_vb);
			if (ac_vb) {
				spm_cvdev_q_busy_vbuffer(ac_vnode, ac_vb);
				ccic_update_dma_addr(ac_vnode, ac_vb, 0);
				ccic_dma->ops->shadow_ready(ccic_dma);
			}
		}
		tmp = irqs;
		spin_lock(&(ac_vnode->slock));
		list_for_each_entry(pos, &(ac_vnode->busy_list), list_entry) {
			if (!tmp)
				break;
			if (tmp & (IRQ_DMA_OVERFLOW | IRQ_DMA_NOT_DONE)) {
				if (!(pos->flags & AC_BUF_FLAG_SOF_TOUCH)) {
					dev_info(dev, "CCIC%d: dma err(0x%08x) without sof, drop it\n", ccic_dev->index, tmp);
					tmp &= ~(IRQ_DMA_OVERFLOW | IRQ_DMA_NOT_DONE);
				} else if (!(pos->flags & AC_BUF_FLAG_HW_ERR)) {
					pos->flags |= AC_BUF_FLAG_HW_ERR;
					dev_info(dev, "CCIC%d: dma err(0x%08x)\n", ccic_dev->index, tmp);
					tmp &= ~(IRQ_DMA_OVERFLOW | IRQ_DMA_NOT_DONE);
				}
			}
			if (tmp & IRQ_DMA_EOF) {
				if (!(pos->flags & AC_BUF_FLAG_SOF_TOUCH)) {
					dev_info(dev, "CCIC%d: dma done without sof, drop it\n", ccic_dev->index);
					tmp &= ~IRQ_DMA_EOF;
				} else if (!(pos->flags & AC_BUF_FLAG_DONE_TOUCH)) {
					pos->flags |= AC_BUF_FLAG_DONE_TOUCH;
					pos->vb2_v4l2_buf.sequence = ac_vnode->frame_id - 1;
					pos->vb2_v4l2_buf.vb2_buf.timestamp = ktime_get_ns();
					tmp &= ~IRQ_DMA_EOF;
					//dev_info(dev, "CCIC%d: dma done\n", ccic_dev->index);
				}
			}
			if (tmp & IRQ_DMA_SOF) {
				if (pos->flags & AC_BUF_FLAG_SOF_TOUCH) {
					if (!(pos->flags & (AC_BUF_FLAG_DONE_TOUCH | AC_BUF_FLAG_HW_ERR | AC_BUF_FLAG_SW_ERR))) {
						dev_warn(dev, "CCIC%d: next sof arrived without dma done or err\n", ccic_dev->index);
						pos->flags |= AC_BUF_FLAG_SW_ERR;
					}
				} else {
					pos->flags |= (AC_BUF_FLAG_SOF_TOUCH | AC_BUF_FLAG_TIMESTAMPED);
					tmp &= ~IRQ_DMA_SOF;
				}
			}
		}
		spin_unlock(&(ac_vnode->slock));
		ret = ccic_get_dma_work(dma_ctx, &ccic_dma_work);
		if (ret) {
			dev_warn(dev, "CCIC%d: dma work idle list was null\n", ccic_dev->index);
		} else {
			ccic_dma_work->irq_status = irqs;
			tasklet_schedule(&(ccic_dma_work->dma_tasklet));
		}
	}
	spin_lock(&ac_vnode->waitq_head.lock);
	ac_vnode->in_irq = 0;
	wake_up_locked(&ac_vnode->waitq_head);
	spin_unlock(&ac_vnode->waitq_head.lock);
	return IRQ_HANDLED;
}

static int x1_ccic_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ccic_dev *ccic_dev;
	struct ccic_ctrl *ccic_ctrl;
	struct ccic_dma *ccic_dma;
	struct device *dev = &pdev->dev;
	char buf[32];
	int ret;

	ret = of_property_read_u32(np, "cell-index", &pdev->id);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get alias id, errno %d\n", ret);
		return ret;
	}

	ccic_dev = devm_kzalloc(&pdev->dev, sizeof(*ccic_dev), GFP_KERNEL);
	if (!ccic_dev) {
		dev_err(&pdev->dev, "camera: Could not allocate ccic dev\n");
		return -ENOMEM;
	}

	ccic_ctrl = devm_kzalloc(&pdev->dev, sizeof(*ccic_ctrl), GFP_KERNEL);
	if (!ccic_ctrl) {
		dev_err(&pdev->dev, "camera: Could not allocate ctrl dev\n");
		return -ENOMEM;
	}

	ccic_dma = devm_kzalloc(&pdev->dev, sizeof(*ccic_dma), GFP_KERNEL);
	if (!ccic_dma) {
		dev_err(&pdev->dev, "camera: Could not allocate dma dev\n");
		return -ENOMEM;
	}

	/* get mem */
	ccic_dev->mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ccic-regs");
	if (!ccic_dev->mem) {
		dev_err(&pdev->dev, "no mem resource");
		return -ENODEV;
	}
	ccic_dev->base = devm_ioremap(&pdev->dev, ccic_dev->mem->start,
				      resource_size(ccic_dev->mem));
	if (IS_ERR(ccic_dev->base)) {
		dev_err(&pdev->dev, "fail to remap iomem\n");
		return PTR_ERR(ccic_dev->base);
	}

	/* get irqs */
	ccic_dev->irq = platform_get_irq_byname(pdev, "ipe-irq");
	if (ccic_dev->irq < 0) {
		dev_err(&pdev->dev, "no irq resource");
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, ccic_dev->irq, x1_ccic_isr,
			       IRQF_SHARED, X1_CCIC_DRV_NAME, ccic_dev);
	if (ret) {
		dev_err(&pdev->dev, "fail to request irq\n");
		return ret;
	}

	/* ccic device and ctrl init */
	ccic_ctrl->ccic_dev = ccic_dev;
	ccic_ctrl->index = pdev->id;
	ccic_ctrl->ops = &ccic_ctrl_ops;
	atomic_set(&ccic_ctrl->usr_cnt, 0);

	ccic_dma->ccic_dev = ccic_dev;
	ccic_dma->ops = &ccic_dma_ops;

	ccic_dev->csiphy = csiphy_lookup_by_phandle(&pdev->dev, "ky,csiphy");
	if (!ccic_dev->csiphy) {
		dev_err(&pdev->dev, "fail to acquire csiphy\n");
		return -EPROBE_DEFER;
	}

	ccic_dev->index = pdev->id;
	ccic_dev->pdev = pdev;
	ccic_dev->dev = &pdev->dev;
	ccic_dev->ctrl = ccic_ctrl;
	ccic_dev->dma = ccic_dma;
	ccic_dev->interrupt_mask_value = CSI2PHYERRS | FRAMEIRQS;
	dev_set_drvdata(dev, ccic_dev);

	/* enable runtime pm */
	pm_runtime_enable(&pdev->dev);

	ccic_init_clk(ccic_dev);

	ccic_device_register(ccic_dev);

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(33));
	ret = v4l2_device_register(&pdev->dev, &ccic_dev->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register v4l2 dev\n");
		return ret;
	}
	snprintf(buf, 32, "CCIC%d", ccic_ctrl->index);
	ccic_dev->vnode = spm_cvdev_create_vnode(buf, ccic_dev->index,
											&ccic_dev->v4l2_dev,
											&pdev->dev,
											ccic_dev,
											ccic_dma_tasklet_handler,
											0);
	if (NULL == ccic_dev->vnode) {
		dev_err(&pdev->dev, "failed to create ccic vnode\n");
		return -EPROBE_DEFER;
	}
	pr_info("%s probed in %s", dev_name(&pdev->dev), __func__);

	return ret;
}

static int x1_ccic_remove(struct platform_device *pdev)
{
	struct ccic_dev *ccic_dev;
	struct ccic_dma *dma;

	ccic_dev = dev_get_drvdata(&pdev->dev);
	dma = ccic_dev->dma;

	spm_cvdev_destroy_vnode((struct spm_ccic_vnode*)ccic_dev->vnode);
	v4l2_device_unregister(&ccic_dev->v4l2_dev);
	ccic_device_unregister(ccic_dev);

	/* disable runtime pm */
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static const struct of_device_id x1_ccic_dt_match[] = {
	{.compatible = "zynq,x1-ccic",.data = NULL },
	{.compatible = "ky,x1ccic",.data = NULL },
	{ },
};

MODULE_DEVICE_TABLE(of, x1_ccic_dt_match);

static struct platform_driver x1_ccic_driver = {
	.driver = {
		.name = X1_CCIC_DRV_NAME,
		.of_match_table = of_match_ptr(x1_ccic_dt_match),
	},
	.probe = x1_ccic_probe,
	.remove = x1_ccic_remove,
};

static int __init x1_ccic_driver_init(void)
{
	int ret;

	ret = ccic_csiphy_register();
	if (ret < 0)
		return ret;

	ret = platform_driver_register(&x1_ccic_driver);
	if (ret < 0)
		ccic_csiphy_unregister();

	return ret;
}

static void __exit x1_ccic_driver_exit(void)
{
	platform_driver_unregister(&x1_ccic_driver);
	ccic_csiphy_unregister();
}

module_init(x1_ccic_driver_init);
module_exit(x1_ccic_driver_exit);
/* module_platform_driver(x1_ccic_driver); */

MODULE_DESCRIPTION("X1 CCIC Driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0
/*
 * KY ccic driver
 *
 * Copyright(C) 2023 KY Micro Limited.
 */

#include <media/v4l2-dev.h>
#include "ccic_drv.h"
#include "ccic_hwreg.h"

int ccic_csi2_config_dphy(struct ccic_dev *ccic_dev, int lanes, int enable)
{
	unsigned int dphy2_val = 0xa2848888;
	unsigned int dphy3_val = 0x00001500;
	/* unsigned int dphy4_val = 0x00000000; */
	unsigned int dphy5_val = 0x000000ff;	/* 4lanes */
	unsigned int dphy6_val = 0x1001;

	if (!enable) {
		ccic_reg_write(ccic_dev, REG_CSI2_DPHY5, 0x00);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_DPHY1, CSI2_DHPY1_ANA_PU);	/* analog power off */
		return 0;
	}

	if (lanes < 1 || lanes > 4)
		return -EINVAL;

	dphy5_val = CSI2_DPHY5_LANE_ENA(lanes);
	dphy5_val = dphy5_val | (dphy5_val << CSI2_DPHY5_LANE_RESC_ENA_SHIFT);

	ccic_reg_write(ccic_dev, REG_CSI2_DPHY2, dphy2_val);
	ccic_reg_write(ccic_dev, REG_CSI2_DPHY3, dphy3_val);
	/* ccic_reg_write(ccic_dev, REG_CSI2_DPHY4, dphy4_val); */
	ccic_reg_write(ccic_dev, REG_CSI2_DPHY5, dphy5_val);
	ccic_reg_write(ccic_dev, REG_CSI2_DPHY6, dphy6_val);

	/* analog power on */
	ccic_reg_set_bit(ccic_dev, REG_CSI2_DPHY1, CSI2_DHPY1_ANA_PU);

	return 0;
}

int ccic_csi2_lanes_enable(struct ccic_dev *ccic_dev, int lanes)
{
	unsigned int ctrl0_val = 0;

	if (lanes < 0 || lanes > 4)
		return -EINVAL;

	if (!lanes) {		/* Disable MIPI CSI2 Interface */
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);	//csi off
		return 0;
	}

	ctrl0_val = ccic_reg_read(ccic_dev, REG_CSI2_CTRL0);
	ctrl0_val &= ~(CSI2_C0_LANE_NUM_MASK);
	ctrl0_val |= CSI2_C0_LANE_NUM(lanes);
	ctrl0_val |= CSI2_C0_ENABLE;
	ctrl0_val &= ~(CSI2_C0_VLEN_MASK);
	ctrl0_val |= CSI2_C0_VLEN;

	ccic_reg_write(ccic_dev, REG_CSI2_CTRL0, ctrl0_val);

	return 0;
}
#if 0
int ccic_csi2_vc_ctrl(struct ccic_dev *ccic_dev, int md, u8 vc0, u8 vc1)
{
	int ret = 0;

	switch (md) {
	case CCIC_CSI2VC_NM:	/* Normal mode */
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL,
				    CSI2_VCCTRL_MD_NORMAL, CSI2_VCCTRL_MD_MASK);
		break;
	case CCIC_CSI2VC_VC:	/* Virtual Channel mode */
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL,
				    CSI2_VCCTRL_MD_VC, CSI2_VCCTRL_MD_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL, vc0 << 14,
				    CSI2_VCCTRL_VC0_MASK);
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL, vc1 << 22,
				    CSI2_VCCTRL_VC1_MASK);
		break;
	case CCIC_CSI2VC_DT:	/* TODO: Data-Type Interleaving */
		ccic_reg_write_mask(ccic_dev, REG_CSI2_VCCTRL,
				    CSI2_VCCTRL_MD_DT, CSI2_VCCTRL_MD_MASK);
		pr_err("csi2 vc mode %d todo\n", md);
		break;
	default:
		pr_err("%s: invalid csi2 vc mode %d\n", __func__, md);
		ret = -EINVAL;
	}

	return ret;
}
#endif
int ccic_dma_src_sel(struct ccic_dev *ccic_dev, int sel, unsigned int main_ccic_id)
{
	switch (sel) {
	case CCIC_DMA_SEL_LOCAL_MAIN:
	case CCIC_DMA_SEL_LOCAL_VCDT:
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0,
				   CSI2_C0_EXT_TIM_ENA);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
		/* FIXME: no need */
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
		break;
	case CCIC_DMA_SEL_REMOTE_VCDT:
		if (ccic_dev->index == 0) {
			if (main_ccic_id == 2) {
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_EXT_TIM_ENA);
				ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
			} else {
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_EXT_TIM_ENA);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
			}
		} else if (ccic_dev->index == 1) {
			if (main_ccic_id == 2) {
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_EXT_TIM_ENA);
				ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
			} else {
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_EXT_TIM_ENA);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
			}
		} else {
			if (main_ccic_id == 0) {
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_EXT_TIM_ENA);
				ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
			} else {
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_EXT_TIM_ENA);
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_VCDT_SEL);
				/* When EXT_TIM_ENA is enabled, this field must be enabled too. */
				ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL0, CSI2_C0_ENABLE);
			}
		}
		break;
	case CCIC_DMA_SEL_REMOTE_MAIN:
	default:
		return -EINVAL;
	}

	return 0;
}

int ccic_dma_set_out_format(struct ccic_dev *ccic_dev, u32 pixfmt, u32 width,
			    u32 height)
{
	u16 pitch_y, pitch_uv, imgsz_h, imgsz_w;
	u32 data_fmt;

	switch (pixfmt) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
		pitch_y = width;
		pitch_uv = 0;
		imgsz_w = pitch_y;
		imgsz_h = height;
		data_fmt = C0_DF_BAYER;
		break;
	case V4L2_PIX_FMT_SBGGR10P:
	case V4L2_PIX_FMT_SGBRG10P:
	case V4L2_PIX_FMT_SGRBG10P:
	case V4L2_PIX_FMT_SRGGB10P:
		pitch_y = width * 5 / 4;
		pitch_uv = 0;
		imgsz_w = pitch_y;
		imgsz_h = height;
		data_fmt = C0_DF_BAYER;
		break;
	case V4L2_PIX_FMT_SBGGR12P:
	case V4L2_PIX_FMT_SGBRG12P:
	case V4L2_PIX_FMT_SGRBG12P:
	case V4L2_PIX_FMT_SRGGB12P:
		pitch_y = width * 3 / 2;
		pitch_uv = 0;
		imgsz_w = pitch_y;
		imgsz_h = height;
		data_fmt = C0_DF_BAYER;
		break;
	default:
		pr_err("%s failed: invalid pixfmt %d\n", __func__, pixfmt);
		return -1;
	}

	ccic_reg_write(ccic_dev, REG_IMGPITCH, pitch_uv << 16 | pitch_y);
	ccic_reg_write(ccic_dev, REG_IMGSIZE, imgsz_h << 16 | imgsz_w);
	ccic_reg_write(ccic_dev, REG_IMGOFFSET, 0x0);

	ccic_reg_write_mask(ccic_dev, REG_CTRL0, data_fmt, C0_DF_MASK);
	/* Make sure it knows we want to use hsync/vsync. */
	ccic_reg_write_mask(ccic_dev, REG_CTRL0, C0_SIF_HVSYNC, C0_SIFM_MASK);
	/* Need set following bit for auto-recovery */
	ccic_reg_set_bit(ccic_dev, REG_CTRL0, C0_EOFFLUSH);

	return 0;
}

int ccic_dma_set_burst(struct ccic_dev *ccic_dev)
{
	u32 dma_burst;

	/* setup the DMA burst */
	switch (ccic_dev->dma_burst) {
	case 128:
		dma_burst = C1_DMAB128;
		break;
	case 256:
		dma_burst = C1_DMAB256;
		break;
	default:
		dma_burst = C1_DMAB64;
		break;
	}
	ccic_reg_write_mask(ccic_dev, REG_CTRL1, dma_burst, C1_DMAB_MASK);
	ccic_reg_set_bit(ccic_dev, REG_CTRL1, C1_DMAB_LENSEL);

	/* ccic_reg_set_bit(ccic_dev, REG_CTRL2, C2_LGCY_LNNUM); */
	/* ccic_reg_set_bit(ccic_dev, REG_CTRL2, C2_LGCY_HBLANK); */
	return 0;
}

__maybe_unused static void ccic_dma_enable(struct ccic_dev *ccic_dev, int en)
{
	if (en) {
		ccic_reg_set_bit(ccic_dev, REG_IRQMASK, FRAMEIRQS);
		/* 0x3c: enable ccic dma */
		ccic_reg_set_bit(ccic_dev, REG_CTRL0, BIT(0));
	} else {
		ccic_reg_clear_bit(ccic_dev, REG_IRQMASK, FRAMEIRQS);
		/* 0x3c: disable ccic dma */
		ccic_reg_clear_bit(ccic_dev, REG_CTRL0, BIT(0));
	}
}

int ccic_csi2idi_src_sel(struct ccic_dev *ccic_dev, int sel)
{
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
		ccic_reg_write_mask(ccic_dev, REG_IDI_CTRL, IDI_SEL_DPCM_REPACK, IDI_SEL_MASK);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_IDI_MUX_SEL_DPCM);
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_RST);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_ENA);
		break;
	case CCIC_IDI_SEL_DPCM:
		ccic_reg_write_mask(ccic_dev, REG_IDI_CTRL, IDI_SEL_DPCM_REPACK, IDI_SEL_MASK);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_IDI_MUX_SEL_DPCM);
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_DPCM_ENA);
		break;
	case CCIC_IDI_SEL_PARALLEL:
		ccic_reg_write_mask(ccic_dev, REG_IDI_CTRL, IDI_SEL_PARALLEL, IDI_SEL_MASK);
		break;
	default:
		pr_err("%s: IDI source is error %d\n", __func__, sel);
		return -EINVAL;
	}

	return 0;
}

void ccic_csi2idi_reset(struct ccic_dev *ccic_dev, int reset)
{
	if (reset) {
		ccic_reg_clear_bit(ccic_dev, REG_IDI_CTRL, IDI_RELEASE_RESET);
		/* assert reset to Repack module */
		ccic_reg_set_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_RST);
	} else {
		ccic_reg_set_bit(ccic_dev, REG_IDI_CTRL, IDI_RELEASE_RESET);
		/* Deassert reset to Repack module */
		ccic_reg_clear_bit(ccic_dev, REG_CSI2_CTRL2, CSI2_C2_REPACK_RST);
	}
}

/* dump register in order to debug */
void ccic_hw_dump_regs(struct ccic_dev *ccic_dev)
{
	unsigned int ret;

	pr_info("CCIC%d regs dump:\n", ccic_dev->index);
	/*
	 * CCIC IRQ REG
	 */
	ret = ccic_reg_read(ccic_dev, REG_IRQSTAT);
	pr_info("CCIC: REG_IRQSTAT[0x%02x] is 0x%08x\n", REG_IRQSTAT, ret);
	ret = ccic_reg_read(ccic_dev, REG_IRQSTATRAW);
	pr_info("CCIC: REG_IRQSTATRAW[0x%02x] is 0x%08x\n", REG_IRQSTATRAW, ret);
	ret = ccic_reg_read(ccic_dev, REG_IRQMASK);
	pr_info("CCIC: REG_IRQMASK[0x%02x] is 0x%08x\n\n", REG_IRQMASK, ret);

	/*
	 * CCIC IMG REG
	 */
	ret = ccic_reg_read(ccic_dev, REG_IMGPITCH);
	pr_info("CCIC: REG_IMGPITCH[0x%02x] is 0x%08x\n", REG_IMGPITCH, ret);
	ret = ccic_reg_read(ccic_dev, REG_IMGSIZE);
	pr_info("CCIC: REG_IMGSIZE[0x%02x] is 0x%08x\n", REG_IMGSIZE, ret);
	ret = ccic_reg_read(ccic_dev, REG_IMGOFFSET);
	pr_info("CCIC: REG_IMGOFFSET[0x%02x] is 0x%08x\n\n", REG_IMGOFFSET, ret);

	/*
	 * CCIC CTRL REG
	 */
	ret = ccic_reg_read(ccic_dev, REG_CTRL0);
	pr_info("CCIC: REG_CTRL0[0x%02x] is 0x%08x\n", REG_CTRL0, ret);
	ret = ccic_reg_read(ccic_dev, REG_CTRL1);
	pr_info("CCIC: REG_CTRL1[0x%02x] is 0x%08x\n", REG_CTRL1, ret);
	ret = ccic_reg_read(ccic_dev, REG_CTRL2);
	pr_info("CCIC: REG_CTRL2[0x%02x] is 0x%08x\n", REG_CTRL2, ret);
	ret = ccic_reg_read(ccic_dev, REG_CTRL3);
	pr_info("CCIC: REG_CTRL3[0x%02x] is 0x%08x\n", REG_CTRL3, ret);
	ret = ccic_reg_read(ccic_dev, REG_IDI_CTRL);
	pr_info("CCIC: REG_IDI_CTRL[0x%02x] is 0x%08x\n\n", REG_IDI_CTRL, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_VCCTRL);
	pr_info("CCIC: REG_CSI2_VCCTRL[0x%02x] is 0x%08x\n\n", REG_CSI2_VCCTRL, ret);
	ret = ccic_reg_read(ccic_dev, REG_LNNUM);
	pr_info("CCIC: REG_LNNUM[0x%02x] is 0x%08x\n", REG_LNNUM, ret);
	ret = ccic_reg_read(ccic_dev, REG_FRAME_CNT);
	pr_info("CCIC: REG_FRAME_CNT[0x%02x] is 0x%08x\n", REG_FRAME_CNT, ret);

	/*
	 * CCIC CSI2 REG
	 */
	ret = ccic_reg_read(ccic_dev, REG_CSI2_DPHY1);
	pr_info("CCIC: REG_CSI2_DPHY1[0x%02x] is 0x%08x\n", REG_CSI2_DPHY1, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_DPHY2);
	pr_info("CCIC: REG_CSI2_DPHY2[0x%02x] is 0x%08x\n", REG_CSI2_DPHY2, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_DPHY3);
	pr_info("CCIC: REG_CSI2_DPHY3[0x%02x] is 0x%08x\n", REG_CSI2_DPHY3, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_DPHY4);
	pr_info("CCIC: REG_CSI2_DPHY4[0x%02x] is 0x%08x\n", REG_CSI2_DPHY4, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_DPHY5);
	pr_info("CCIC: REG_CSI2_DPHY5[0x%02x] is 0x%08x\n", REG_CSI2_DPHY5, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_DPHY6);
	pr_info("CCIC: REG_CSI2_DPHY6[0x%02x] is 0x%08x\n", REG_CSI2_DPHY6, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_CTRL0);
	pr_info("CCIC: REG_CSI2_CTRL0[0x%02x] is 0x%08x\n\n", REG_CSI2_CTRL0, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_CTRL2);
	pr_info("CCIC: REG_CSI2_CTRL2[0x%02x] is 0x%08x\n\n", REG_CSI2_CTRL2, ret);
	ret = ccic_reg_read(ccic_dev, REG_CSI2_CTRL3);
	pr_info("CCIC: REG_CSI2_CTRL3[0x%02x] is 0x%08x\n\n", REG_CSI2_CTRL3, ret);

	/*
	 * CCIC YUV REG
	 */
	ret = ccic_reg_read(ccic_dev, REG_Y0BAR);
	pr_info("CCIC: REG_Y0BAR[0x%02x] 0x%08x\n", REG_Y0BAR, ret);
	ret = ccic_reg_read(ccic_dev, REG_U0BAR);
	pr_info("CCIC: REG_U0BAR[0x%02x] 0x%08x\n", REG_U0BAR, ret);
	ret = ccic_reg_read(ccic_dev, REG_V0BAR);
	pr_info("CCIC: REG_V0BAR[0x%02x] 0x%08x\n\n", REG_V0BAR, ret);

#if 0
	/*
	 * CCIC APMU REG
	 */
	ret = __raw_readl(get_apmu_base_va() + REG_CLK_CCIC_RES);
	pr_info("CCIC: APMU_CCIC_RES[0x%02x] is 0x%08x\n", REG_CLK_CCIC_RES, ret);
	ret = __raw_readl(get_apmu_base_va() + REG_CLK_CCIC2_RES);
	pr_info("CCIC: APMU_CCIC2_RES[0x%02x] is 0x%08x\n", REG_CLK_CCIC2_RES, ret);
#endif
}

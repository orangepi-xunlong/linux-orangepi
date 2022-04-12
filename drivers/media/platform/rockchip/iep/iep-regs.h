/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Rockchip Image Enhancement Processor (IEP) driver
 *
 * Copyright (C) 2020 Alex Bee <knaerzche@gmail.com>
 *
 */

#ifndef __IEP_REGS_H__
#define __IEP_REGS_H__

/* IEP Registers addresses */
#define IEP_CONFIG0			0x000 /* Configuration register0 */
#define IEP_VOP_DIRECT_PATH			BIT(0)
#define IEP_DEIN_HIGH_FREQ_SHFT			1
#define IEP_DEIN_HIGH_FREQ_MASK			(0x7f << IEP_DEIN_HIGH_FREQ_SHFT)
#define IEP_DEIN_MODE_SHFT			8
#define IEP_DEIN_MODE_MASK			(7 << IEP_DEIN_MODE_SHFT)
#define IEP_DEIN_HIGH_FREQ_EN			BIT(11)
#define IEP_DEIN_EDGE_INTPOL_EN			BIT(12)
#define IEP_YUV_DENOISE_EN			BIT(13)
#define IEP_YUV_ENHNC_EN			BIT(14)
#define IEP_DEIN_EDGE_INTPOL_SMTH_EN		BIT(15)
#define IEP_RGB_CLR_ENHNC_EN			BIT(16)
#define IEP_RGB_CNTRST_ENHNC_EN			BIT(17)
#define IEP_RGB_ENHNC_MODE_BYPASS		(0 << 18)
#define IEP_RGB_ENHNC_MODE_DNS			BIT(18)
#define IEP_RGB_ENHNC_MODE_DTL			(2 << 18)
#define IEP_RGB_ENHNC_MODE_EDG			(3 << 18)
#define IEP_RGB_ENHNC_MODE_MASK			(3 << 18)
#define IEP_RGB_CNTRST_ENHNC_DDE_FRST		BIT(20)
#define IEP_DEIN_EDGE_INTPOL_RADIUS_SHFT	21
#define IEP_DEIN_EDGE_INTPOL_RADIUS_MASK	(3 << IEP_DEIN_EDGE_INTPOL_RADIUS_SHFT)
#define IEP_DEIN_EDGE_INTPOL_SELECT		BIT(23)

#define IEP_CONFIG1			0x004 /* Configuration register1 */
#define IEP_SRC_FMT_SHFT			0
#define IEP_SRC_FMT_MASK			(3 << IEP_SRC_FMT_SHFT)
#define IEP_SRC_RGB_SWP_SHFT			2
#define IEP_SRC_RGB_SWP_MASK			(2 << IEP_SRC_RGB_SWP_SHFT)
#define IEP_SRC_YUV_SWP_SHFT			4
#define IEP_SRC_YUV_SWP_MASK			(3 << IEP_SRC_YUV_SWP_SHFT)
#define IEP_DST_FMT_SHFT			8
#define IEP_DST_FMT_MASK			(3 << IEP_DST_FMT_SHFT)
#define IEP_DST_RGB_SWP_SHFT			10
#define IEP_DST_RGB_SWP_MASK			(2 << IEP_DST_RGB_SWP_SHFT)
#define IEP_DST_YUV_SWP_SHFT			12
#define IEP_DST_YUV_SWP_MASK			(3 << IEP_DST_YUV_SWP_SHFT)
#define IEP_DTH_UP_EN				BIT(14)
#define IEP_DTH_DWN_EN				BIT(15)
#define IEP_YUV2RGB_COE_BT601_1			(0 << 16)
#define IEP_YUV2RGB_COE_BT601_F			BIT(16)
#define IEP_YUV2RGB_COE_BT709_1			(2 << 16)
#define IEP_YUV2RGB_COE_BT709_F			(3 << 16)
#define IEP_YUV2RGB_COE_MASK			(3 << 16)
#define IEP_RGB2YUV_COE_BT601_1			(0 << 18)
#define IEP_RGB2YUV_COE_BT601_F			BIT(18)
#define IEP_RGB2YUV_COE_BT709_1			(2 << 18)
#define IEP_RGB2YUV_COE_BT709_F			(3 << 18)
#define IEP_RGB2YUV_COE_MASK			(3 << 18)
#define IEP_YUV2RGB_EN				BIT(20)
#define IEP_RGB2YUV_EN				BIT(21)
#define IEP_YUV2RGB_CLIP_EN			BIT(22)
#define IEP_RGB2YUV_CLIP_EN			BIT(23)
#define IEP_GLB_ALPHA_SHFT			24
#define IEP_GLB_ALPHA_MASK			(0x7f << IEP_GLB_ALPHA_SHFT)

#define IEP_STATUS			0x008 /* Status register */
#define IEP_STATUS_YUV_DNS			BIT(0)
#define IEP_STATUS_SCL				BIT(1)
#define IEP_STATUS_DIL				BIT(2)
#define IEP_STATUS_DDE				BIT(3)
#define IEP_STATUS_DMA_WR_YUV			BIT(4)
#define IEP_STATUS_DMA_RE_YUV			BIT(5)
#define IEP_STATUS_DMA_WR_RGB			BIT(6)
#define IEP_STATUS_DMA_RE_RGB			BIT(7)
#define IEP_STATUS_VOP_DIRECT_PATH		BIT(8)
#define IEP_STATUS_DMA_IA_WR_YUV		BIT(16)
#define IEP_STATUS_DMA_IA_RE_YUV		BIT(17)
#define IEP_STATUS_DMA_IA_WR_RGB		BIT(18)
#define IEP_STATUS_DMA_IA_RE_RGB		BIT(19)

#define IEP_INT				0x00c /* Interrupt register*/
#define IEP_INT_FRAME_DONE			BIT(0) /* Frame process done interrupt */
#define IEP_INT_FRAME_DONE_EN			BIT(8) /* Frame process done interrupt enable */
#define IEP_INT_FRAME_DONE_CLR			BIT(16) /* Frame process done interrupt clear */

#define IEP_FRM_START			0x010 /* Frame start */
#define IEP_SRST			0x014 /* Soft reset */
#define IEP_CONFIG_DONE			0x018 /* Configuration done */
#define IEP_FRM_CNT			0x01c /* Frame counter */

#define IEP_VIR_IMG_WIDTH		0x020 /* Image virtual width */
#define IEP_IMG_SCL_FCT			0x024 /* Scaling factor */
#define IEP_SRC_IMG_SIZE		0x028 /* src image width/height */
#define IEP_DST_IMG_SIZE		0x02c /* dst image width/height */
#define IEP_DST_IMG_WIDTH_TILE0		0x030 /* dst image tile0 width */
#define IEP_DST_IMG_WIDTH_TILE1		0x034 /* dst image tile1 width */
#define IEP_DST_IMG_WIDTH_TILE2		0x038 /* dst image tile2 width */
#define IEP_DST_IMG_WIDTH_TILE3		0x03c /* dst image tile3 width */

#define IEP_ENH_YUV_CNFG_0		0x040 /* Brightness, contrast, saturation adjustment */
#define IEP_YUV_BRIGHTNESS_SHFT			0
#define IEP_YUV_BRIGHTNESS_MASK			(0x3f << IEP_YUV_BRIGHTNESS_SHFT)
#define IEP_YUV_CONTRAST_SHFT			8
#define IEP_YUV_CONTRAST_MASK			(0xff << IEP_YUV_CONTRAST_SHFT)
#define IEP_YUV_SATURATION_SHFT			16
#define IEP_YUV_SATURATION_MASK			(0x1ff << IEP_YUV_SATURATION_SHFT)

#define IEP_ENH_YUV_CNFG_1		0x044 /* Hue configuration */
#define IEP_YUV_COS_HUE_SHFT			0
#define IEP_YUV_COS_HUE_MASK			(0xff << IEP_YUV_COS_HUE_SHFT)
#define IEP_YUV_SIN_HUE_SHFT			8
#define IEP_YUV_SIN_HUE_MASK			(0xff << IEP_YUV_SIN_HUE_SHFT)

#define IEP_ENH_YUV_CNFG_2		0x048 /* Color bar configuration */
#define IEP_YUV_COLOR_BAR_Y_SHFT		0
#define IEP_YUV_COLOR_BAR_Y_MASK		(0xff << IEP_YUV_COLOR_BAR_Y_SHFT)
#define IEP_YUV_COLOR_BAR_U_SHFT		8
#define IEP_YUV_COLOR_BAR_U_MASK		(0xff << IEP_YUV_COLOR_BAR_U_SHFT)
#define IEP_YUV_COLOR_BAR_V_SHFT		16
#define IEP_YUV_COLOR_BAR_V_MASK		(0xff << IEP_YUV_COLOR_BAR_V_SHFT)
#define IEP_YUV_VIDEO_MODE_SHFT			24
#define IEP_YUV_VIDEO_MODE_MASK			(3 << IEP_YUV_VIDEO_MODE_SHFT)

#define IEP_ENH_RGB_CNFG		0x04c /* RGB enhancement configuration */
#define IEP_ENH_RGB_C_COE		0x050 /* RGB color enhancement coefficient */

#define IEP_RAW_CONFIG0			0x058 /* Raw configuration register0 */
#define IEP_RAW_CONFIG1			0x05c /* Raw configuration register1 */
#define IEP_RAW_VIR_IMG_WIDTH		0x060 /* Raw image virtual width */
#define IEP_RAW_IMG_SCL_FCT		0x064 /* Raw scaling factor */
#define IEP_RAW_SRC_IMG_SIZE		0x068 /* Raw src image width/height  */
#define IEP_RAW_DST_IMG_SIZE		0x06c /* Raw src image width/height  */
#define IEP_RAW_ENH_YUV_CNFG_0		0x070 /* Raw brightness,contrast,saturation adjustment */
#define IEP_RAW_ENH_YUV_CNFG_1		0x074 /* Raw hue configuration */
#define IEP_RAW_ENH_YUV_CNFG_2		0x078 /* Raw color bar configuration */
#define IEP_RAW_ENH_RGB_CNFG		0x07c /* Raw RGB enhancement configuration */

#define IEP_SRC_ADDR_Y_RGB		0x080 /* Start addr. of src image 0 (Y/RGB) */
#define IEP_SRC_ADDR_CBCR		0x084 /* Start addr. of src image 0 (Cb/Cr) */
#define IEP_SRC_ADDR_CR			0x088 /* Start addr. of src image 0 (Cr) */
#define IEP_SRC_ADDR_Y1			0x08c /* Start addr. of src image 1 (Y) */
#define IEP_SRC_ADDR_CBCR1		0x090 /* Start addr. of src image 1 (Cb/Cr) */
#define IEP_SRC_ADDR_CR1		0x094 /* Start addr. of src image 1 (Cr) */
#define IEP_SRC_ADDR_Y_ITEMP		0x098 /* Start addr. of src image(Y int part) */
#define IEP_SRC_ADDR_CBCR_ITEMP		0x09c /* Start addr. of src image(CBCR int part) */
#define IEP_SRC_ADDR_CR_ITEMP		0x0a0 /* Start addr. of src image(CR int part) */
#define IEP_SRC_ADDR_Y_FTEMP		0x0a4 /* Start addr. of src image(Y frac part) */
#define IEP_SRC_ADDR_CBCR_FTEMP		0x0a8 /* Start addr. of src image(CBCR frac part) */
#define IEP_SRC_ADDR_CR_FTEMP		0x0ac /* Start addr. of src image(CR frac part) */

#define IEP_DST_ADDR_Y_RGB		0x0b0 /* Start addr. of dst image 0 (Y/RGB) */
#define IEP_DST_ADDR_CBCR		0x0b4 /* Start addr. of dst image 0 (Cb/Cr) */
#define IEP_DST_ADDR_CR			0x0b8 /* Start addr. of dst image 0 (Cr) */
#define IEP_DST_ADDR_Y1			0x0bc /* Start addr. of dst image 1 (Y) */
#define IEP_DST_ADDR_CBCR1		0x0c0 /* Start addr. of dst image 1 (Cb/Cr) */
#define IEP_DST_ADDR_CR1		0x0c4 /* Start addr. of dst image 1 (Cr) */
#define IEP_DST_ADDR_Y_ITEMP		0x0c8 /* Start addr. of dst image(Y int part) */
#define IEP_DST_ADDR_CBCR_ITEMP		0x0cc /* Start addr. of dst image(CBCR int part)*/
#define IEP_DST_ADDR_CR_ITEMP		0x0d0 /* Start addr. of dst image(CR int part) */
#define IEP_DST_ADDR_Y_FTEMP		0x0d4 /* Start addr. of dst image(Y frac part) */
#define IEP_DST_ADDR_CBCR_FTEMP		0x0d8 /* Start addr. of dst image(CBCR frac part) */
#define IEP_DST_ADDR_CR_FTEMP		0x0dc /* Start addr. of dst image(CR frac part)*/

#define IEP_DEIN_MTN_TAB0		0x0e0 /* Deinterlace motion table0 */
#define IEP_DEIN_MTN_TAB1		0x0e4 /* Deinterlace motion table1 */
#define IEP_DEIN_MTN_TAB2		0x0e8 /* Deinterlace motion table2 */
#define IEP_DEIN_MTN_TAB3		0x0ec /* Deinterlace motion table3 */
#define IEP_DEIN_MTN_TAB4		0x0f0 /* Deinterlace motion table4 */
#define IEP_DEIN_MTN_TAB5		0x0f4 /* Deinterlace motion table5 */
#define IEP_DEIN_MTN_TAB6		0x0f8 /* Deinterlace motion table6 */
#define IEP_DEIN_MTN_TAB7		0x0fc /* Deinterlace motion table7 */

#define IEP_ENH_CG_TAB			0x100 /* Contrast and gamma enhancement table */
#define IEP_ENH_DDE_COE0		0x400 /* Denoise,detail and edge enhancement coefficient */
#define IEP_ENH_DDE_COE1		0x500 /* Denoise,detail and edge enhancement coefficient1 */

#define IEP_INT_MASK			(IEP_INT_FRAME_DONE)

/* IEP colorformats */
#define IEP_COLOR_FMT_XRGB		0U
#define IEP_COLOR_FMT_RGB565		1U
#define IEP_COLOR_FMT_YUV422		2U
#define IEP_COLOR_FMT_YUV420		3U

/* IEP YUV color swaps */
#define IEP_YUV_SWP_SP_UV		0U
#define IEP_YUV_SWP_SP_VU		1U
#define IEP_YUV_SWP_P			2U

/* IEP XRGB color swaps */
#define XRGB_SWP_XRGB			0U
#define XRGB_SWP_XBGR			1U
#define XRGB_SWP_BGRX			2U

/* IEP RGB565 color swaps */
#define RGB565_SWP_RGB			0U
#define RGB565_SWP_BGR			1U

#define FMT_IS_YUV(fmt) (fmt == IEP_COLOR_FMT_XRGB || fmt == IEP_COLOR_FMT_RGB565 ? 0 : 1)

#define IEP_IMG_SIZE(w, h) (((w - 1) & 0x1fff) << 0 | \
			    ((h - 1) & 0x1fff) << 16)

#define IEP_VIR_WIDTH(src_w, dst_w) (((src_w / 4) & 0x1fff) << 0 | \
				     ((dst_w / 4) & 0x1fff) << 16)

#define IEP_Y_STRIDE(w, h) (w * h)
#define IEP_UV_STRIDE(w, h, fac) (w * h + w * h / fac)

#define IEP_SRC_FMT_SWP_MASK(f) (FMT_IS_YUV(f) ? IEP_SRC_YUV_SWP_MASK : IEP_SRC_RGB_SWP_MASK)
#define IEP_DST_FMT_SWP_MASK(f) (FMT_IS_YUV(f) ? IEP_DST_YUV_SWP_MASK : IEP_DST_RGB_SWP_MASK)

#define IEP_SRC_FMT(f, swp) (f << IEP_SRC_FMT_SHFT | \
			     (swp << (FMT_IS_YUV(f) ? IEP_SRC_YUV_SWP_SHFT : IEP_SRC_RGB_SWP_SHFT)))
#define IEP_DST_FMT(f, swp) (f << IEP_DST_FMT_SHFT | \
			     (swp << (FMT_IS_YUV(f) ? IEP_DST_YUV_SWP_SHFT : IEP_DST_RGB_SWP_SHFT)))

/* IEP DEINTERLACE MODES */
#define IEP_DEIN_MODE_YUV		0U
#define IEP_DEIN_MODE_I4O2		1U
#define IEP_DEIN_MODE_I4O1B		2U
#define IEP_DEIN_MODE_I4O1T		3U
#define IEP_DEIN_MODE_I2O1B		4U
#define IEP_DEIN_MODE_I2O1T		5U
#define IEP_DEIN_MODE_BYPASS		6U

#define IEP_DEIN_IN_FIELDS_2		2U
#define IEP_DEIN_IN_FIELDS_4		4U

#define IEP_DEIN_OUT_FRAMES_1		1U
#define IEP_DEIN_OUT_FRAMES_2		2U

/* values taken from BSP driver */
static const u32 default_dein_motion_tbl[][2] = {
	{ IEP_DEIN_MTN_TAB0, 0x40404040 },
	{ IEP_DEIN_MTN_TAB1, 0x3c3e3f3f },
	{ IEP_DEIN_MTN_TAB2, 0x3336393b },
	{ IEP_DEIN_MTN_TAB3, 0x272a2d31 },
	{ IEP_DEIN_MTN_TAB4, 0x181c2023 },
	{ IEP_DEIN_MTN_TAB5, 0x0c0e1215 },
	{ IEP_DEIN_MTN_TAB6, 0x03040609 },
	{ IEP_DEIN_MTN_TAB7, 0x00000001 },

};

#define IEP_DEIN_IN_IMG0_Y(bff) (bff ? IEP_SRC_ADDR_Y_RGB : IEP_SRC_ADDR_Y1)
#define IEP_DEIN_IN_IMG0_CBCR(bff) (bff ? IEP_SRC_ADDR_CBCR : IEP_SRC_ADDR_CBCR1)
#define IEP_DEIN_IN_IMG0_CR(bff) (bff ? IEP_SRC_ADDR_CR : IEP_SRC_ADDR_CR1)
#define IEP_DEIN_IN_IMG1_Y(bff) (IEP_DEIN_IN_IMG0_Y(!bff))
#define IEP_DEIN_IN_IMG1_CBCR(bff) (IEP_DEIN_IN_IMG0_CBCR(!bff))
#define IEP_DEIN_IN_IMG1_CR(bff) (IEP_DEIN_IN_IMG0_CR(!bff))

#define IEP_DEIN_OUT_IMG0_Y(bff) (bff ? IEP_DST_ADDR_Y1 : IEP_DST_ADDR_Y_RGB)
#define IEP_DEIN_OUT_IMG0_CBCR(bff) (bff ? IEP_DST_ADDR_CBCR1 : IEP_DST_ADDR_CBCR)
#define IEP_DEIN_OUT_IMG0_CR(bff) (bff ? IEP_DST_ADDR_CR1 : IEP_DST_ADDR_CR)
#define IEP_DEIN_OUT_IMG1_Y(bff) (IEP_DEIN_OUT_IMG0_Y(!bff))
#define IEP_DEIN_OUT_IMG1_CBCR(bff) (IEP_DEIN_OUT_IMG0_CBCR(!bff))
#define IEP_DEIN_OUT_IMG1_CR(bff) (IEP_DEIN_OUT_IMG0_CR(!bff))

#define IEP_DEIN_MODE(m) (m << IEP_DEIN_MODE_SHFT)

#define IEP_DEIN_IN_MODE_FIELDS(m) ((m == IEP_DEIN_MODE_I4O1T || m == IEP_DEIN_MODE_I4O1B \
				     || m == IEP_DEIN_MODE_I4O2) \
				    ? IEP_DEIN_IN_FIELDS_4 : IEP_DEIN_IN_FIELDS_2)

#define IEP_DEIN_OUT_MODE_FRAMES(m) (m == IEP_DEIN_MODE_I4O2 \
				     ? IEP_DEIN_OUT_FRAMES_2 : IEP_DEIN_OUT_FRAMES_1)

#define IEP_DEIN_OUT_MODE_1FRM_TOP_FIELD(m) (m == IEP_DEIN_MODE_I4O1T || IEP_DEIN_MODE_I2O1T \
					      ? 1 : 0)

#define IEP_DEIN_EDGE_INTPOL_RADIUS(r) (r << IEP_DEIN_EDGE_INTPOL_RADIUS_SHFT)

#define IEP_DEIN_HIGH_FREQ(f) (f << IEP_DEIN_HIGH_FREQ_SHFT)

/* YUV Enhance video modes */
#define VIDEO_MODE_BLACK_SCREEN		0U
#define VIDEO_MODE_BLUE_SCREEN		1U
#define VIDEO_MODE_COLOR_BARS		2U
#define VIDEO_MODE_NORMAL_VIDEO		3U

#define YUV_VIDEO_MODE(m) ((m << IEP_YUV_VIDEO_MODE_SHFT) & IEP_YUV_VIDEO_MODE_MASK)
#define YUV_BRIGHTNESS(v) ((v << IEP_YUV_BRIGHTNESS_SHFT) & IEP_YUV_BRIGHTNESS_MASK)
#define YUV_CONTRAST(v) ((v << IEP_YUV_CONTRAST_SHFT) & IEP_YUV_CONTRAST_MASK)
#define YUV_SATURATION(v) ((v << IEP_YUV_SATURATION_SHFT) & IEP_YUV_SATURATION_MASK)
#define YUV_COS_HUE(v) ((v << IEP_YUV_COS_HUE_SHFT) & IEP_YUV_COS_HUE_MASK)
#define YUV_SIN_HUE(v) ((v << IEP_YUV_SIN_HUE_SHFT) & IEP_YUV_SIN_HUE_MASK)

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :  display engine 35x fcm basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_enhance.h"
#include "de_fcm_type.h"
#include "de_rtmx.h"

#define HUE_BIN_LEN	28
#define SAT_BIN_LEN	13
#define LUM_BIN_LEN	13
#define HBH_HUE_MASK	0x7fff
#define SBH_HUE_MASK	0x1fff
#define VBH_HUE_MASK	0x7ff
#define SAT_MASK	0xff
#define LUM_MASK	0xff

#define FCM_PARA_NUM (12)
#define FCM_SUM_NUM (28)
enum { FCM_PARA_REG_BLK = 0,
       FCM_ANGLE_REG_BLK,
       FCM_HUE_REG_BLK,
       FCM_SAT_REG_BLK,
       FCM_LUM_REG_BLK,
       FCM_CSC_REG_BLK,
       FCM_REG_BLK_NUM,
};

struct de_fcm_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FCM_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_fcm_private *priv, u32 blk_id,
			      u32 dirty);
};

static enum enhance_init_state g_init_state;
static struct de_fcm_private fcm_priv[DE_NUM][VI_CHN_NUM];
static inline struct fcm_reg *get_fcm_reg(struct de_fcm_private *priv)
{
	return (struct fcm_reg *)(priv->reg_blks[FCM_PARA_REG_BLK].vir_addr);
}

static void fcm_set_block_dirty(struct de_fcm_private *priv, u32 blk_id,
				u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void fcm_set_rcq_head_dirty(struct de_fcm_private *priv, u32 blk_id,
				   u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_fcm_init(u32 disp, u32 chn, uintptr_t reg_base, u8 __iomem **phy_addr,
		u8 **vir_addr, u32 *size)
{
	struct de_fcm_private *priv = &fcm_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_FCM_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_FCM_REG_MEM_SIZE;

	priv->reg_blk_num = FCM_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[FCM_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x20;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[FCM_ANGLE_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x20;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x20;
	reg_blk->size = 0x100;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x20);

	reg_blk = &(priv->reg_blks[FCM_HUE_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x120;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x120;
	reg_blk->size = 0x1e0;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x120);

	reg_blk = &(priv->reg_blks[FCM_SAT_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x300;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x300;
	reg_blk->size = 0x1200;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x300);

	reg_blk = &(priv->reg_blks[FCM_LUM_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x1500;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x1500;
	reg_blk->size = 0x1b00;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x1500);

	reg_blk = &(priv->reg_blks[FCM_CSC_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x3000;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x3000;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x3000);

	*phy_addr += DE_FCM_REG_MEM_SIZE;
	*vir_addr += DE_FCM_REG_MEM_SIZE;
	*size -= DE_FCM_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = fcm_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = fcm_set_block_dirty;

	g_init_state = ENHANCE_INVALID;
	return 0;
}

s32 de_fcm_exit(u32 disp, u32 chn) { return 0; }

s32 de_fcm_get_reg_blocks(u32 disp, u32 chn, struct de_reg_block **blks,
			  u32 *blk_num)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WARN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_fcm_set_size(u32 disp, u32 chn, u32 width, u32 height)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcm_set_window(u32 disp, u32 chn, u32 win_enable,
		      struct de_rect_o window)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);
	u32 fcm_en;

	/* if (g_init_state >= ENHANCE_TIGERLCD_ON) { */
		/* return 0; */
	/* } */
	fcm_en = reg->ctl.bits.fcm_en;
	reg->ctl.bits.window_en = fcm_en ? win_enable : 0;
	if (win_enable) {
		reg->win0.bits.win_left = window.x;
		reg->win1.bits.win_right = window.x + window.w - 1;
		reg->win0.bits.win_top = window.y;
		reg->win1.bits.win_bot = window.y + window.h - 1;
	}
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

/*static s32 de_fcm_apply(u32 disp, u32 chn, u32 fcm_en)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

    reg->ctl.bits.fcm_en = fcm_en;
    priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	if (fcm_en == 1) {
	} else {
	}

	return 0;
}*/

s32 de_fcm_info2para(u32 disp, u32 chn, u32 fmt, u32 dev_type,
		     struct fcm_config_data *para, u32 bypass)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);
	s32 sgain;
	u32 en;
	int i = 0;
	static u8 FCM_SAT_GAIN[FCM_PARA_NUM][2] = {
	    /* lcd/hdmi */
	    {-4, -4}, /* 00 gain for yuv */
	    {-3, -4}, /* 01              */
	    {-2, -4}, /* 02              */
	    {-1, -4}, /* 03              */
	    {0, 0},   /* 04              */
	    {1, 1},   /* 05              */
	    {2, 2},   /* 06              */
	    {3, 3},   /* 07              */
	    {4, 4},   /* 08              */
	    {5, 5},   /* 09              */
	    {6, 6},   /* 10 gain for yuv */
	    {0, 0},   /* 11 gain for rgb */
	};

	static u8 FCM_SAT_SUM[FCM_SUM_NUM] = {
	    /*   0,  1,  2,  3   4   5   6   7   8   9 */
	    20, 20, 20, 20, 22, 23, 24, 24, 26, 26, 26, 26, 26, 28,
	    28, 28, 28, 26, 26, 26, 26, 24, 24, 24, 24, 22, 22, 20,
	};
	para->level = para->level % FCM_PARA_NUM;
	if (fmt == 1)
		sgain = FCM_SAT_GAIN[FCM_PARA_NUM - 1][dev_type];
	else
		sgain = FCM_SAT_GAIN[para->level][dev_type];

	en = (((fmt == 0) && (sgain == 0)) || (bypass != 0)) ? 0 : 1;

	reg->ctl.bits.fcm_en = en;
	if (!en) {
		priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);
		return 0;
	}
	for (i = 0; i < FCM_SUM_NUM; i++) {
		reg->sbh_hue_lut[i].bits.sbh_hue_lut_high =
		    reg->sbh_hue_lut[i].bits.sbh_hue_lut_high +
		    sgain * FCM_SAT_SUM[i];
		reg->sbh_hue_lut[i].bits.sbh_hue_lut_low =
		    reg->sbh_hue_lut[i].bits.sbh_hue_lut_low +
		    sgain * FCM_SAT_SUM[i];
	}
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);
	return 0;
}

s32 de_fcm_enable(u32 disp, u32 chn, u32 en)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	reg->ctl.bits.fcm_en = en;
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcm_init_para(u32 disp, u32 chn)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	if (g_init_state == ENHANCE_TIGERLCD_ON) {
		/* sram need refresh, normal reg refreshed by system*/
		priv->set_blk_dirty(priv, FCM_ANGLE_REG_BLK, 1);
		priv->set_blk_dirty(priv, FCM_HUE_REG_BLK, 1);
		priv->set_blk_dirty(priv, FCM_SAT_REG_BLK, 1);
		priv->set_blk_dirty(priv, FCM_LUM_REG_BLK, 1);
		return 0;
	}
	//reg->ctl.dwval = 0x80000001;
	reg->ctl.dwval = 0x0;
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

static void fcm_set_csc_coeff(struct fcm_reg *reg,
	u32 *icsc_coeff, u32 *ocsc_coeff)
{
	u32 dwval0, dwval1, dwval2;
/*
	dwval0 = (icsc_coeff[12] >= 0) ? (icsc_coeff[12] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[12] & 0x3ff));
	dwval1 = (icsc_coeff[13] >= 0) ? (icsc_coeff[13] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[13] & 0x3ff));
	dwval2 = (icsc_coeff[14] >= 0) ? (icsc_coeff[14] & 0x3ff) :
		(0x400 - (u32)(icsc_coeff[14] & 0x3ff));
*/
	dwval0 = ((icsc_coeff[12] & 0x80000000) ? (u32)(-(s32)icsc_coeff[12]) : icsc_coeff[12]) & 0x3ff;
	dwval1 = ((icsc_coeff[13] & 0x80000000) ? (u32)(-(s32)icsc_coeff[13]) : icsc_coeff[13]) & 0x3ff;
	dwval2 = ((icsc_coeff[14] & 0x80000000) ? (u32)(-(s32)icsc_coeff[14]) : icsc_coeff[14]) & 0x3ff;

	reg->csc0_d0.dwval = dwval0;
	reg->csc0_d1.dwval = dwval1;
	reg->csc0_d2.dwval = dwval2;
	reg->csc0_c00.dwval = icsc_coeff[0];
	reg->csc0_c01.dwval = icsc_coeff[1];
	reg->csc0_c02.dwval = icsc_coeff[2];
	reg->csc0_c03.dwval = icsc_coeff[3];
	reg->csc0_c10.dwval = icsc_coeff[4];
	reg->csc0_c11.dwval = icsc_coeff[5];
	reg->csc0_c12.dwval = icsc_coeff[6];
	reg->csc0_c13.dwval = icsc_coeff[7];
	reg->csc0_c20.dwval = icsc_coeff[8];
	reg->csc0_c21.dwval = icsc_coeff[9];
	reg->csc0_c22.dwval = icsc_coeff[10];
	reg->csc0_c23.dwval = icsc_coeff[11];

/*
	dwval0 = (ocsc_coeff[12] >= 0) ? (ocsc_coeff[12] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[12] & 0x3ff));
	dwval1 = (ocsc_coeff[13] >= 0) ? (ocsc_coeff[13] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[13] & 0x3ff));
	dwval2 = (ocsc_coeff[14] >= 0) ? (ocsc_coeff[14] & 0x3ff) :
		(0x400 - (u32)(ocsc_coeff[14] & 0x3ff));
*/

	dwval0 = ((ocsc_coeff[12] & 0x80000000) ? (u32)(-(s32)ocsc_coeff[12]) : ocsc_coeff[12]) & 0x3ff;
	dwval1 = ((ocsc_coeff[13] & 0x80000000) ? (u32)(-(s32)ocsc_coeff[13]) : ocsc_coeff[13]) & 0x3ff;
	dwval2 = ((ocsc_coeff[14] & 0x80000000) ? (u32)(-(s32)ocsc_coeff[14]) : ocsc_coeff[14]) & 0x3ff;

	reg->csc1_d0.dwval = dwval0;
	reg->csc1_d1.dwval = dwval1;
	reg->csc1_d2.dwval = dwval2;
	reg->csc1_c00.dwval = ocsc_coeff[0];
	reg->csc1_c01.dwval = ocsc_coeff[1];
	reg->csc1_c02.dwval = ocsc_coeff[2];
	reg->csc1_c03.dwval = ocsc_coeff[3];
	reg->csc1_c10.dwval = ocsc_coeff[4];
	reg->csc1_c11.dwval = ocsc_coeff[5];
	reg->csc1_c12.dwval = ocsc_coeff[6];
	reg->csc1_c13.dwval = ocsc_coeff[7];
	reg->csc1_c20.dwval = ocsc_coeff[8];
	reg->csc1_c21.dwval = ocsc_coeff[9];
	reg->csc1_c22.dwval = ocsc_coeff[10];
	reg->csc1_c23.dwval = ocsc_coeff[11];
}

int de_fcm_set_csc(u32 disp, u32 chn, struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_csc_info icsc_out, ocsc_in;
	u32 *icsc_coeff, *ocsc_coeff;
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	icsc_out.eotf = in_info->eotf;
	memcpy((void *)&ocsc_in, out_info, sizeof(ocsc_in));

	if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		icsc_out.px_fmt_space = DE_FORMAT_SPACE_RGB;
		icsc_out.color_space = in_info->color_space;
		icsc_out.color_range = in_info->color_range;
		ocsc_in.color_range = DE_COLOR_RANGE_0_255;
		ocsc_in.px_fmt_space = DE_FORMAT_SPACE_RGB;
	} else if (in_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		icsc_out.px_fmt_space = DE_FORMAT_SPACE_RGB;
		icsc_out.color_range = DE_COLOR_RANGE_0_255;
		if ((in_info->color_space != DE_COLOR_SPACE_BT2020NC)
				&& (in_info->color_space != DE_COLOR_SPACE_BT2020C))
					icsc_out.color_space = DE_COLOR_SPACE_BT709;
		else
				icsc_out.color_space = in_info->color_space;
		ocsc_in.color_range = DE_COLOR_RANGE_0_255;
		ocsc_in.px_fmt_space = DE_FORMAT_SPACE_RGB;
	} else {
			DE_WARN("px_fmt_space %d no support",
					in_info->px_fmt_space);
			return -1;
	}

	de_csc_coeff_calc(in_info, &icsc_out, &icsc_coeff);
	de_csc_coeff_calc(&ocsc_in, out_info, &ocsc_coeff);
	fcm_set_csc_coeff(reg, icsc_coeff, ocsc_coeff);
	priv->set_blk_dirty(priv, FCM_CSC_REG_BLK, 1);
	return 1;
}

static void de_fcm_set_hue_lut(struct fcm_reg *reg, unsigned int *h, unsigned int *s, unsigned int *v)
{
	int i;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		reg->hbh_hue_lut[i].bits.hbh_hue_lut_low = h[i] & HBH_HUE_MASK;
		reg->sbh_hue_lut[i].bits.sbh_hue_lut_low = s[i] & SBH_HUE_MASK;
		reg->vbh_hue_lut[i].bits.vbh_hue_lut_low = v[i] & VBH_HUE_MASK;
		if (i == HUE_BIN_LEN - 1) {
			reg->hbh_hue_lut[i].bits.hbh_hue_lut_high = h[0] & HBH_HUE_MASK;
			reg->sbh_hue_lut[i].bits.sbh_hue_lut_high = s[0] & SBH_HUE_MASK;
			reg->vbh_hue_lut[i].bits.vbh_hue_lut_high = v[0] & VBH_HUE_MASK;
		} else {
			reg->hbh_hue_lut[i].bits.hbh_hue_lut_high = h[i+1] & HBH_HUE_MASK;
			reg->sbh_hue_lut[i].bits.sbh_hue_lut_high = s[i+1] & SBH_HUE_MASK;
			reg->vbh_hue_lut[i].bits.vbh_hue_lut_high = v[i+1] & VBH_HUE_MASK;
		}
	}
}

static void de_fcm_set_sat_lut(struct fcm_reg *reg, unsigned int *h, unsigned int *s, unsigned int *y)
{
	int i, j, m, n;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		for (j = 0; j < SAT_BIN_LEN; j++) {
			n = j;
			m = i;

			reg->hbh_sat_lut[i * SAT_BIN_LEN + j].bits.hbh_sat_lut0 = h[i * SAT_BIN_LEN + j] & SAT_MASK;
			reg->sbh_sat_lut[i * SAT_BIN_LEN + j].bits.sbh_sat_lut0 = s[i * SAT_BIN_LEN + j] & SAT_MASK;
			reg->vbh_sat_lut[i * SAT_BIN_LEN + j].bits.vbh_sat_lut0 = y[i * SAT_BIN_LEN + j] & SAT_MASK;

			if (j == SAT_BIN_LEN - 1)
				n = j - 1;
			if (i == HUE_BIN_LEN - 1)
				m = -1;

			reg->hbh_sat_lut[i * SAT_BIN_LEN + j].bits.hbh_sat_lut1 = h[i * SAT_BIN_LEN + n + 1] & SAT_MASK;
			reg->sbh_sat_lut[i * SAT_BIN_LEN + j].bits.sbh_sat_lut1 = s[i * SAT_BIN_LEN + n + 1] & SAT_MASK;
			reg->vbh_sat_lut[i * SAT_BIN_LEN + j].bits.vbh_sat_lut1 = y[i * SAT_BIN_LEN + n + 1] & SAT_MASK;

			reg->hbh_sat_lut[i * SAT_BIN_LEN + j].bits.hbh_sat_lut2 = h[(m + 1) * SAT_BIN_LEN + j] & SAT_MASK;
			reg->sbh_sat_lut[i * SAT_BIN_LEN + j].bits.sbh_sat_lut2 = s[(m + 1) * SAT_BIN_LEN + j] & SAT_MASK;
			reg->vbh_sat_lut[i * SAT_BIN_LEN + j].bits.vbh_sat_lut2 = y[(m + 1) * SAT_BIN_LEN + j] & SAT_MASK;

			reg->hbh_sat_lut[i * SAT_BIN_LEN + j].bits.hbh_sat_lut3 = h[(m + 1) * SAT_BIN_LEN + n + 1] & SAT_MASK;
			reg->sbh_sat_lut[i * SAT_BIN_LEN + j].bits.sbh_sat_lut3 = s[(m + 1) * SAT_BIN_LEN + n + 1] & SAT_MASK;
			reg->vbh_sat_lut[i * SAT_BIN_LEN + j].bits.vbh_sat_lut3 = y[(m + 1) * SAT_BIN_LEN + n + 1] & SAT_MASK;
		}
	}
}

static void de_fcm_set_lum_lut(struct fcm_reg *reg, unsigned int *h, unsigned int *s, unsigned int *y)
{
	int i, j, m, n;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		for (j = 0; j < LUM_BIN_LEN; j++) {
			n = j;
			m = i;

			reg->hbh_lum_lut[i * LUM_BIN_LEN + j].bits.hbh_lum_lut0 = h[i * LUM_BIN_LEN + j] & LUM_MASK;
			reg->sbh_lum_lut[i * LUM_BIN_LEN + j].bits.sbh_lum_lut0 = s[i * LUM_BIN_LEN + j] & LUM_MASK;
			reg->vbh_lum_lut[i * LUM_BIN_LEN + j].bits.vbh_lum_lut0 = y[i * LUM_BIN_LEN + j] & LUM_MASK;

			if (j == LUM_BIN_LEN - 1)
				n = j - 1;
			if (i == HUE_BIN_LEN - 1)
				m = -1;

			reg->hbh_lum_lut[i * LUM_BIN_LEN + j].bits.hbh_lum_lut1 = h[i * LUM_BIN_LEN + n + 1] & LUM_MASK;
			reg->sbh_lum_lut[i * LUM_BIN_LEN + j].bits.sbh_lum_lut1 = s[i * LUM_BIN_LEN + n + 1] & LUM_MASK;
			reg->vbh_lum_lut[i * LUM_BIN_LEN + j].bits.vbh_lum_lut1 = y[i * LUM_BIN_LEN + n + 1] & LUM_MASK;

			reg->hbh_lum_lut[i * LUM_BIN_LEN + j].bits.hbh_lum_lut2 = h[(m + 1) * LUM_BIN_LEN + j] & LUM_MASK;
			reg->sbh_lum_lut[i * LUM_BIN_LEN + j].bits.sbh_lum_lut2 = s[(m + 1) * LUM_BIN_LEN + j] & LUM_MASK;
			reg->vbh_lum_lut[i * LUM_BIN_LEN + j].bits.vbh_lum_lut2 = y[(m + 1) * LUM_BIN_LEN + j] & LUM_MASK;

			reg->hbh_lum_lut[i * LUM_BIN_LEN + j].bits.hbh_lum_lut3 = h[(m + 1) * LUM_BIN_LEN + n + 1] & LUM_MASK;
			reg->sbh_lum_lut[i * LUM_BIN_LEN + j].bits.sbh_lum_lut3 = s[(m + 1) * LUM_BIN_LEN + n + 1] & LUM_MASK;
			reg->vbh_lum_lut[i * LUM_BIN_LEN + j].bits.vbh_lum_lut3 = y[(m + 1) * LUM_BIN_LEN + n + 1] & LUM_MASK;
		}
	}
}

int de_fcm_set_lut(unsigned int sel, fcm_hardware_data_t *data, unsigned int update)
{
	int i = 0;

	DE_INFO("set data sel=%d\n", sel);
	if (!data) {
		DE_WARN("data NULL sel=%d\n", sel);
		return -1;
	}
	for (i = 0; i < VI_CHN_NUM; i++) {
		if (de_feat_is_support_vep_by_chn(sel, i)) {
			struct de_fcm_private *priv = &(fcm_priv[sel][i]);
			struct fcm_reg *reg = get_fcm_reg(priv);

			de_fcm_set_hue_lut(reg, data->hbh_hue, data->sbh_hue, data->ybh_hue);
			de_fcm_set_sat_lut(reg, data->hbh_sat, data->sbh_sat, data->ybh_sat);
			de_fcm_set_lum_lut(reg, data->hbh_lum, data->sbh_lum, data->ybh_lum);

			memcpy(reg->angle_hue_lut, data->angle_hue, sizeof(s32) * 28);
			memcpy(reg->angle_sat_lut, data->angle_sat, sizeof(s32) * 13);
			memcpy(reg->angle_lum_lut, data->angle_lum, sizeof(s32) * 13);

			if (update) {
				reg->ctl.bits.fcm_en = 1;
				reg->csc_ctl.dwval = 1;
				g_init_state = ENHANCE_TIGERLCD_ON;
				priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);
				priv->set_blk_dirty(priv, FCM_ANGLE_REG_BLK, 1);
				priv->set_blk_dirty(priv, FCM_HUE_REG_BLK, 1);
				priv->set_blk_dirty(priv, FCM_SAT_REG_BLK, 1);
				priv->set_blk_dirty(priv, FCM_LUM_REG_BLK, 1);
				priv->set_blk_dirty(priv, FCM_CSC_REG_BLK, 1);
			} else {
				priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 0);
				priv->set_blk_dirty(priv, FCM_ANGLE_REG_BLK, 0);
				priv->set_blk_dirty(priv, FCM_HUE_REG_BLK, 0);
				priv->set_blk_dirty(priv, FCM_SAT_REG_BLK, 0);
				priv->set_blk_dirty(priv, FCM_LUM_REG_BLK, 0);
				priv->set_blk_dirty(priv, FCM_CSC_REG_BLK, 0);
			}
		}
	}
	return 0;
}

int de_fcm_get_lut(unsigned int sel, fcm_hardware_data_t *data)
{
	struct de_fcm_private *priv = &(fcm_priv[sel][0]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	DE_INFO("get data sel=%d\n", sel);
	if (!data) {
		DE_WARN("data NULL sel=%d\n", sel);
		return -1;
	}
	memcpy(data->hbh_hue, reg->hbh_hue_lut, sizeof(s32) * 28);
	memcpy(data->sbh_hue, reg->sbh_hue_lut, sizeof(s32) * 28);
	memcpy(data->ybh_hue, reg->vbh_hue_lut, sizeof(s32) * 28);

	memcpy(data->angle_hue, reg->angle_hue_lut, sizeof(s32) * 28);
	memcpy(data->angle_sat, reg->angle_sat_lut, sizeof(s32) * 13);
	memcpy(data->angle_lum, reg->angle_lum_lut, sizeof(s32) * 13);

	memcpy(data->hbh_sat, reg->hbh_sat_lut, sizeof(s32) * 364);
	memcpy(data->sbh_sat, reg->sbh_sat_lut, sizeof(s32) * 364);
	memcpy(data->ybh_sat, reg->vbh_sat_lut, sizeof(s32) * 364);

	memcpy(data->hbh_lum, reg->hbh_lum_lut, sizeof(s32) * 364);
	memcpy(data->sbh_lum, reg->sbh_lum_lut, sizeof(s32) * 364);
	memcpy(data->ybh_lum, reg->vbh_lum_lut, sizeof(s32) * 364);

	return 0;
}

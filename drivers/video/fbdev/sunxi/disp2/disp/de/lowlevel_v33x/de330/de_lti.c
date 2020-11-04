/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *  File name   :       de_lti.c
 *
 *  Description :       display engine 3.0 LTI basic function definition
 *
 *  History     :       2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#include "de_lti_type.h"
#include "de_enhance.h"

#define LTI_PARA_NUM (12)

enum {
	LTI_LTI_REG_BLK = 0,
	LTI_CTI_REG_BLK,
	LTI_REG_BLK_NUM,
};

struct de_lti_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[LTI_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_lti_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_lti_private lti_priv[DE_NUM][VI_CHN_NUM];


static inline struct lti_reg *get_lti_reg(struct de_lti_private *priv)
{
	return (struct lti_reg *)(priv->reg_blks[LTI_LTI_REG_BLK].vir_addr);
}

static inline struct lti_reg *get_cti_reg(struct de_lti_private *priv)
{
	return (struct lti_reg *)(priv->reg_blks[LTI_CTI_REG_BLK].vir_addr);
}

static void lti_set_block_dirty(
	struct de_lti_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void lti_set_rcq_head_dirty(
	struct de_lti_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_lti_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_lti_private *priv = &lti_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_LTI_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_LTI_REG_MEM_SIZE;

	priv->reg_blk_num = LTI_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[LTI_LTI_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x44;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[LTI_CTI_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x100;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x100;
	reg_blk->size = 0x44;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x100);

	*phy_addr += DE_LTI_REG_MEM_SIZE;
	*vir_addr += DE_LTI_REG_MEM_SIZE;
	*size -= DE_LTI_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = lti_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = lti_set_block_dirty;

	return 0;
}

s32 de_lti_exit(u32 disp, u32 chn)
{
	return 0;
}

s32 de_lti_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_lti_private *priv = &(lti_priv[disp][chn]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WRN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_lti_enable(u32 disp, u32 chn, u32 en)
{
	struct de_lti_private *priv = &lti_priv[disp][chn];
	struct lti_reg *lti = get_lti_reg(priv);
	struct lti_reg *cti = get_cti_reg(priv);

	lti->ctrl.bits.en = en;
	cti->ctrl.bits.en = en;
	priv->set_blk_dirty(priv, LTI_LTI_REG_BLK, 1);
	priv->set_blk_dirty(priv, LTI_CTI_REG_BLK, 1);

	return 0;
}

s32 de_lti_set_size(u32 disp, u32 chn, u32 width,
			  u32 height)
{
	struct de_lti_private *priv = &lti_priv[disp][chn];
	struct lti_reg *lti = get_lti_reg(priv);
	struct lti_reg *cti = get_cti_reg(priv);

	lti->size.dwval = (height - 1) << 16 | (width - 1);
	cti->size.dwval = (height - 1) << 16 | (width - 1);
	priv->set_blk_dirty(priv, LTI_LTI_REG_BLK, 1);
	priv->set_blk_dirty(priv, LTI_CTI_REG_BLK, 1);
	return 0;
}

s32 de_lti_set_window(u32 disp, u32 chn,
			    u32 win_enable, struct de_rect_o window)
{
	struct de_lti_private *priv = &lti_priv[disp][chn];
	struct lti_reg *lti = get_lti_reg(priv);
	struct lti_reg *cti = get_cti_reg(priv);

	lti->ctrl.bits.win_en = win_enable;

	if (win_enable) {
		lti->win0.dwval = window.y << 16 | window.x;
		lti->win1.dwval =
			  (window.y + window.h - 1) << 16 |
			  (window.x + window.w - 1);
	}
	priv->set_blk_dirty(priv, LTI_LTI_REG_BLK, 1);

	cti->ctrl.bits.win_en = win_enable;

	if (win_enable) {
		cti->win0.dwval = window.y << 16 | window.x;
		cti->win1.dwval =
			  (window.y + window.h - 1) << 16 |
			  (window.x + window.w - 1);
	}
	priv->set_blk_dirty(priv, LTI_CTI_REG_BLK, 1);
	return 0;
}

s32 de_lti_init_para(u32 disp, u32 chn)
{
	struct de_lti_private *priv = &lti_priv[disp][chn];
	struct lti_reg *lti = get_lti_reg(priv);
	struct lti_reg *cti = get_cti_reg(priv);

	/* lti */
	lti->ctrl.dwval = 0 << 16 | 1 << 8 | 0;

	lti->corth.dwval = 4;
	lti->diff.dwval = 4 << 16 | 32;
	lti->edge_gain.dwval = 1;
	lti->os_con.dwval = 1 << 28 | 40 << 16 | 0;

	lti->win_range.dwval = 2;
	lti->elvel_th.dwval = 16;

	priv->set_blk_dirty(priv, LTI_LTI_REG_BLK, 1);

	/* cti */
	cti->ctrl.dwval = 0 << 16 | 1 << 8 | 0;

	cti->corth.dwval = 2;
	cti->diff.dwval = 4 << 16 | 2;
	cti->edge_gain.dwval = 1;
	cti->os_con.dwval = 1 << 28 | 40 << 16 | 0;

	cti->win_range.dwval = 2;
	cti->elvel_th.dwval = 32;

	priv->set_blk_dirty(priv, LTI_CTI_REG_BLK, 1);

	return 0;
}

static s32 disp_coef(s32 insz, s32 outsz, s32 *coef)
{
	s32 i;
	s32 ratio;
	s32 win_disp = 0;

	if (insz != 0) {
		ratio = (outsz + insz / 2) / insz;
		ratio = (ratio > 7) ? 7 : ratio;
	} else
		ratio = 1;

	for (i = 0; i < 8; i++)
		coef[i] = 0;

	if (ratio < 3) {
		coef[0] = 64;
		coef[1] = 32;
		coef[2] = -16;
		coef[3] = -32;
		coef[4] = -16;
		win_disp = 0;
	} else if (ratio == 3 || ratio == 4) {
		coef[0] = 64;
		coef[ratio] = -32;
		win_disp = 1;
	} else if (ratio >= 5) {
		coef[0] = 64;
		coef[ratio] = -32;
		win_disp = 2;
	}

	return win_disp;
}

s32 de_lti_info2para(u32 disp, u32 chn,
			   u32 fmt, u32 dev_type,
			   struct __lti_config_data *para,
			   u32 bypass)
{
	static s32 lti_para[LTI_PARA_NUM][2] = {
		/* lcd / hdmi */
		{0,  0}, /* 00 gain for yuv */
		{1,  1}, /* 01			*/
		{2,  1}, /* 02			*/
		{3,  2}, /* 03			*/
		{4,  2}, /* 04			*/
		{5,  3}, /* 05			*/
		{6,  3}, /* 06			*/
		{7,  4}, /* 07			*/
		{8,  4}, /* 08			*/
		{9,  5}, /* 09			*/
		{10, 5}, /* 10 gain for yuv */
		{2,  1}, /* 11 gain for rgb */
	};

	struct de_lti_private *priv = &lti_priv[disp][chn];
	struct lti_reg *lti = get_lti_reg(priv);
	struct lti_reg *cti = get_cti_reg(priv);
	s32 gain, en, win_disp;
	s32 coef[8];

	/* parameters */
	en = (((fmt == 0) && (para->level == 0)) || (bypass != 0)) ? 0 : 1;
	para->mod_en = en; /* return enable info */
	if (en == 0) {
		/* if level=0, module will be disabled,
		* no need to set para register
		*/
		lti->ctrl.bits.en = en;
		cti->ctrl.bits.en = en;
		priv->set_blk_dirty(priv, LTI_LTI_REG_BLK, 1);
		priv->set_blk_dirty(priv, LTI_CTI_REG_BLK, 1);
		return 0;
	}

	if (fmt == 1) /* rgb */
		gain = lti_para[LTI_PARA_NUM - 1][dev_type];
	else
		gain = lti_para[para->level][dev_type];

	win_disp = disp_coef(para->inw, para->outw, coef);

	/* lti */
	lti->ctrl.bits.en = en;
	lti->gain.dwval = gain;

	lti->coef0.dwval = coef[1] << 16 | (coef[0] & 0xffff);
	lti->coef1.dwval = coef[3] << 16 | (coef[2] & 0xffff);
	lti->coef2.dwval = coef[7] << 24 |
	    (coef[6] & 0xff) << 16 | (coef[5] & 0xff) << 8 | (coef[4] & 0xff);

	lti->win_range.dwval = win_disp << 8 | 2;

	priv->set_blk_dirty(priv, LTI_LTI_REG_BLK, 1);

	/* cti */
	cti->ctrl.bits.en = en;
	cti->gain.dwval = gain;
	cti->coef0.dwval = coef[1] << 16 | (coef[0] & 0xffff);
	cti->coef1.dwval = coef[3] << 16 | (coef[2] & 0xffff);
	cti->coef2.dwval = coef[7] << 24 |
	    (coef[6] & 0xff) << 16 | (coef[5] & 0xff) << 8 | (coef[4] & 0xff);

	cti->win_range.dwval = win_disp << 8 | 2;

	priv->set_blk_dirty(priv, LTI_CTI_REG_BLK, 1);
	return 0;
}

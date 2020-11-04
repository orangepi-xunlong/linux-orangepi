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
 *  File name   :   de_bls.c
 *
 *  Description :   display engine 3.0 bls basic function definition
 *
 *  History     :   2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#include "de_bls_type.h"
#include "de_enhance.h"

enum {
	BLS_CTL_REG_BLK = 0,
	BLS_REG_BLK_NUM,
};

struct de_bls_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[BLS_REG_BLK_NUM];

	void (*set_blk_dirty)(struct de_bls_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_bls_private bls_priv[DE_NUM][VI_CHN_NUM];


static inline struct bls_reg *get_bls_reg(struct de_bls_private *priv)
{
	return (struct bls_reg *)(priv->reg_blks[0].vir_addr);
}

static void bls_set_block_dirty(
	struct de_bls_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void bls_set_rcq_head_dirty(
	struct de_bls_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_bls_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_bls_private *priv = &bls_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_BLS_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_BLS_REG_MEM_SIZE;

	priv->reg_blk_num = BLS_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[BLS_CTL_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x40;
	reg_blk->reg_addr = (u8 __iomem *)base;

	*phy_addr += DE_BLS_REG_MEM_SIZE;
	*vir_addr += DE_BLS_REG_MEM_SIZE;
	*size -= DE_BLS_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = bls_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = bls_set_block_dirty;

	return 0;
}

s32 de_bls_exit(u32 disp, u32 chn)
{
	return 0;
}

s32 de_bls_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_bls_private *priv = &(bls_priv[disp][chn]);
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

s32 de_bls_enable(u32 disp, u32 chn, u32 en)
{
	struct de_bls_private *priv = &(bls_priv[disp][chn]);
	struct bls_reg *reg = get_bls_reg(priv);

	__inf("disp=%d, chn=%d, en=%d\n", disp, chn, en);
	reg->ctrl.bits.en = en;
	priv->set_blk_dirty(priv, BLS_CTL_REG_BLK, 1);
	return 0;
}

s32 de_bls_set_size(u32 disp, u32 chn, u32 width,
			   u32 height)
{
	struct de_bls_private *priv = &(bls_priv[disp][chn]);
	struct bls_reg *reg = get_bls_reg(priv);

	reg->size.dwval = (height - 1) << 16 | (width - 1);
	priv->set_blk_dirty(priv, BLS_CTL_REG_BLK, 1);
	return 0;
}

s32 de_bls_set_window(u32 disp, u32 chn,
			     u32 win_enable, struct de_rect_o window)
{
	struct de_bls_private *priv = &(bls_priv[disp][chn]);
	struct bls_reg *reg = get_bls_reg(priv);

	reg->ctrl.bits.win_en = win_enable;
	if (win_enable) {
		reg->win0.dwval = window.y << 16 | window.x;
		reg->win1.dwval =
			(window.y + window.h - 1) << 16 |
			(window.x + window.w - 1);
	}

	priv->set_blk_dirty(priv, BLS_CTL_REG_BLK, 1);
	return 0;
}

s32 de_bls_init_para(u32 disp, u32 chn)
{
	struct de_bls_private *priv = &(bls_priv[disp][chn]);
	struct bls_reg *reg = get_bls_reg(priv);

	u32 i, j, tmpval, gain;

	u32 uatten_lut[32] = {
		6, 7, 8, 8, 9, 10, 10, 11,
		12, 12, 13, 14, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 10, 9, 9, 8, 8, 7, 7
	};

	u32 gain_lut[16] = {
		1, 1, 1, 2,
		2, 3, 3, 4,
		6, 8, 10, 15,
		20, 25, 25, 25
	};

	memset(reg, 0, sizeof(struct bls_reg));
	reg->bls_pos.dwval = 86 << 16 | 118;
	for (i = 0; i < 4; i++) {
		tmpval = 0;

		for (j = 0; j < 8; j++)
			tmpval = (tmpval << 4) +
			(uatten_lut[i * 8 + 7 - j] & 0xF);

		reg->bls_attlut[i].dwval = tmpval;
	}

	gain = 1; /* 5 */
	for (i = 0; i < 4; i++) {
		tmpval = 0;

		for (j = 0; j < 4; j++)
			tmpval = (tmpval << 8) +
			((gain_lut[i * 4 + 3 - j] * gain) & 0xFF);

		reg->bls_gainlut[i].dwval = tmpval;
	}

	priv->set_blk_dirty(priv, BLS_CTL_REG_BLK, 1);
	return 0;
}

s32 de_bls_info2para(u32 disp, u32 chn,
			   u32 fmt, u32 dev_type,
			   struct __bls_config_data *para,
			   u32 bypass)
{
	if ((fmt == 1) || (bypass != 0))
		para->mod_en = 0;
	else
		para->mod_en = (para->level > 4) ? 1 : 0;
		/* return enable info */

	de_bls_enable(disp, chn, para->mod_en);

	return 0;
}

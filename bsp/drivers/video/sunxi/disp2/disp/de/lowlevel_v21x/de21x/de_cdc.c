/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include "de_cdc_type.h"
#include "de_cdc_table.h"

#include "../../include.h"
#include "de_feat.h"
#include "de_top.h"

#include "de_cdc.h"
#include "de_enhance.h"

enum {
	DE_SDR = 0,
	DE_WCG,
	DE_HDR10,
	DE_HLG,
};

enum {
	CDC_REG_BLK_CTL = 0,
	CDC_REG_BLK_LUT0,
	CDC_REG_BLK_LUT1,
	CDC_REG_BLK_LUT2,
	CDC_REG_BLK_LUT3,
	CDC_REG_BLK_LUT4,
	CDC_REG_BLK_LUT5,
	CDC_REG_BLK_LUT6,
	CDC_REG_BLK_LUT7,
	CDC_REG_BLK_CURVE,
	CDC_REG_BLK_NUM,
};

enum gtm_status {
	/* module only alloc mem*/
	CDC_GTM_INVAID = 0x1 << 0,
	/* module has inited pri para */
	CDC_GTM_INITED = 0x1 << 1,
};

struct de_cdc_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[CDC_REG_BLK_NUM];
	enum de_format_space in_px_fmt_space;
	enum gtm_status gtm_status;
	u32 convert_type;
	u8 has_csc;

	void (*set_blk_dirty)(struct de_cdc_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_cdc_private cdc_priv[GLB_MAX_CDC_NUM];
static u8 cdc_priv_alloced[GLB_MAX_CDC_NUM];
static u8 tigerlcd_writing;

static struct de_cdc_private *cdc_alloc_private(void)
{
	u32 i = 0;

	for (; i < GLB_MAX_CDC_NUM; i++) {
		if (cdc_priv_alloced[i] == 0) {
			cdc_priv_alloced[i] = 1;
			return &cdc_priv[i];
		}
	}
	DE_WARN("not enough cdc priv to alloc!\n");
	return NULL;
}

static s32 cdc_free_private(struct de_cdc_private *priv)
{
	u32 i = 0;

	for (; i < GLB_MAX_CDC_NUM; i++) {
		if (&cdc_priv[i] == priv) {
			memset((void *)priv, 0, sizeof(*priv));
			cdc_priv_alloced[i] = 0;
			return 0;
		}
	}
	DE_WARN("free failed: priv=%p, cdc_priv=%p!\n",
		priv, cdc_priv);
	return -1;
}

static inline struct cdc_reg *get_cdc_reg(struct de_cdc_private *priv)
{
	return (struct cdc_reg *)(priv->reg_blks[0].vir_addr);
}

static void cdc_set_block_dirty(
	struct de_cdc_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void cdc_set_rcq_head_dirty(
	struct de_cdc_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}
static u32 cdc_get_wcg_type(struct de_csc_info *info)
{
	if (info->color_space == DE_COLOR_SPACE_BT2020NC
		|| info->color_space == DE_COLOR_SPACE_BT2020C) {
		if (info->eotf == DE_EOTF_SMPTE2084)
			return DE_HDR10;
		else if (info->eotf == DE_EOTF_ARIB_STD_B67)
			return DE_HLG;
		else
			return DE_WCG;
	} else {
		return DE_SDR;
	}
}

static u32 cdc_get_convert_type(u32 in_type, u32 out_type)
{
	u32 convert_type = DE_TFC_SDR2SDR;

	if (in_type == DE_SDR) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_SDR2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_SDR2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_SDR2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_SDR2HLG;
	} else if (in_type == DE_WCG) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_WCG2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_WCG2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_WCG2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_WCG2HLG;
	} else if (in_type == DE_HDR10) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HDR102SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_HDR102WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_HDR102HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_HDR102HLG;
	} else if (in_type == DE_HLG) {
		if (out_type == DE_SDR)
			convert_type = DE_TFC_HLG2SDR;
		else if (out_type == DE_WCG)
			convert_type = DE_TFC_HLG2WCG;
		else if (out_type == DE_HDR10)
			convert_type = DE_TFC_HLG2HDR10;
		else if (out_type == DE_HLG)
			convert_type = DE_TFC_HLG2HLG;
	}

	return convert_type;
}

static u32 cdc_check_bypass(struct de_csc_info *in_info, u32 convert_type)
{
	if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		switch (convert_type) {
		case DE_TFC_SDR2SDR:
		case DE_TFC_WCG2WCG:
		case DE_TFC_HDR102HDR10:
		case DE_TFC_HLG2HLG:
			return 1;
		case DE_TFC_SDR2WCG:
		case DE_TFC_SDR2HDR10:
			return 0;
		default:
			DE_WARN("conversion type no support %d", convert_type);
			return 1;
		}
	} else if (in_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		switch (convert_type) {
		case DE_TFC_SDR2SDR:
		case DE_TFC_WCG2WCG:
		case DE_TFC_HDR102HDR10:
		case DE_TFC_HLG2HLG:
			return 1;
		case DE_TFC_SDR2WCG:
		case DE_TFC_SDR2HDR10:
		case DE_TFC_WCG2SDR:
		case DE_TFC_WCG2HDR10:
		case DE_TFC_HDR102SDR:
		case DE_TFC_HDR102WCG:
		case DE_TFC_HLG2SDR:
		case DE_TFC_HLG2WCG:
		case DE_TFC_HLG2HDR10:
			return 0;
		default:
			DE_WARN("conversion type no support %d", convert_type);
			return 1;
		}
	} else {
		DE_WARN("px_fmt_space %d no support", in_info->px_fmt_space);
		return 1;
	}
}

static void cdc_set_csc_coeff(struct cdc_reg *reg,
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

	reg->in_d[0].dwval = dwval0;
	reg->in_d[1].dwval = dwval1;
	reg->in_d[2].dwval = dwval2;
	reg->in_c0[0].dwval = icsc_coeff[0];
	reg->in_c0[1].dwval = icsc_coeff[1];
	reg->in_c0[2].dwval = icsc_coeff[2];
	reg->in_c03.dwval = icsc_coeff[3];
	reg->in_c1[0].dwval = icsc_coeff[4];
	reg->in_c1[1].dwval = icsc_coeff[5];
	reg->in_c1[2].dwval = icsc_coeff[6];
	reg->in_c13.dwval = icsc_coeff[7];
	reg->in_c2[0].dwval = icsc_coeff[8];
	reg->in_c2[1].dwval = icsc_coeff[9];
	reg->in_c2[2].dwval = icsc_coeff[10];
	reg->in_c23.dwval = icsc_coeff[11];

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

	reg->out_d[0].dwval = dwval0;
	reg->out_d[1].dwval = dwval1;
	reg->out_d[2].dwval = dwval2;
	reg->out_c0[0].dwval = ocsc_coeff[0];
	reg->out_c0[1].dwval = ocsc_coeff[1];
	reg->out_c0[2].dwval = ocsc_coeff[2];
	reg->out_c03.dwval = ocsc_coeff[3];
	reg->out_c1[0].dwval = ocsc_coeff[4];
	reg->out_c1[1].dwval = ocsc_coeff[5];
	reg->out_c1[2].dwval = ocsc_coeff[6];
	reg->out_c13.dwval = ocsc_coeff[7];
	reg->out_c2[0].dwval = ocsc_coeff[8];
	reg->out_c2[1].dwval = ocsc_coeff[9];
	reg->out_c2[2].dwval = ocsc_coeff[10];
	reg->out_c23.dwval = ocsc_coeff[11];
}

static void cdc_set_lut(struct cdc_reg *reg, u32 **lut_ptr)
{
	aw_memcpy_toio((void *)reg->lut0, (void *)lut_ptr[0], sizeof(reg->lut0));
	aw_memcpy_toio((void *)reg->lut1, (void *)lut_ptr[1], sizeof(reg->lut1));
	aw_memcpy_toio((void *)reg->lut2, (void *)lut_ptr[2], sizeof(reg->lut2));
	aw_memcpy_toio((void *)reg->lut3, (void *)lut_ptr[3], sizeof(reg->lut3));
	aw_memcpy_toio((void *)reg->lut4, (void *)lut_ptr[4], sizeof(reg->lut4));
	aw_memcpy_toio((void *)reg->lut5, (void *)lut_ptr[5], sizeof(reg->lut5));
	aw_memcpy_toio((void *)reg->lut6, (void *)lut_ptr[6], sizeof(reg->lut6));
	aw_memcpy_toio((void *)reg->lut7, (void *)lut_ptr[7], sizeof(reg->lut7));
}

s32 de_cdc_set_para(void *cdc_hdl,
	struct de_csc_info *in_info, struct de_csc_info *out_info)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct cdc_reg *reg = get_cdc_reg(priv);

	u32 in_type, out_type;
	u32 convert_type;
	u32 bypass = 0;

	in_type = cdc_get_wcg_type(in_info);
	out_type = cdc_get_wcg_type(out_info);
	convert_type = cdc_get_convert_type(in_type, out_type);
	bypass = cdc_check_bypass(in_info, convert_type);

	if (!bypass) {
		u32 *lut_ptr[DE_CDC_LUT_NUM];
		u32 i;

		reg->ctl.dwval = 1;
		reg->cdc_size.bits.cdc_width = out_info->width - 1;
		reg->cdc_size.bits.cdc_height = out_info->height - 1;
		if (convert_type == DE_TFC_HDR102SDR) {
			if (!(priv->gtm_status & CDC_GTM_INITED)) {
				/*
				   when tigerlcd is written once, the parameters of tigerlcd are
				   used by default, and the default value is not written manually.
				   After each disable, tasklet set rcq dirty will re-refresh the
				   default value of the register to the value written by tigerlcd.
				*/
				if (tigerlcd_writing != 10) {
					reg->dynamic_mode.bits.dynamic_mode_en = 1;
					reg->gtm_hl_th.bits.hl_th = 0x2ec;
					reg->gtm_hl_ratio.bits.th_hig_pct = 0x19;
					reg->gtm_hl_ratio.bits.th_low_pct = 0x0d;
					reg->gtm_src_nit_range.bits.src_nit_hig = 0x5dc;
					reg->gtm_src_nit_range.bits.src_nit_low = 0x3e8;
				}
				reg->gtm_src_max_nit.bits.src_max_nit = 0x3e8;
				reg->gtm_src_max_nit.bits.max_nit_idx = 0xf;
				reg->gtm_gamut_fration.bits.fraction_bit = 0xa;
				reg->gtm_gamut_coef[0].dwval = 0x000006a2;
				reg->gtm_gamut_coef[1].dwval = 0x0000fda7;
				reg->gtm_gamut_coef[2].dwval = 0x0000ffb6;
				reg->gtm_gamut_coef[3].dwval = 0x0000ff81;
				reg->gtm_gamut_coef[4].dwval = 0x00000486;
				reg->gtm_gamut_coef[5].dwval = 0x0000fff8;
				reg->gtm_gamut_coef[6].dwval = 0x0000ffee;
				reg->gtm_gamut_coef[7].dwval = 0x0000ff9a;
				reg->gtm_gamut_coef[8].dwval = 0x00000478;

				for (i = 0; i < 1024; i++) {
					reg->curve[i].bits.hig_curve_val = GTM_CURVE_HIG[i];
					reg->curve[i].bits.low_curve_val = GTM_CURVE_LOW[i];
				}
				priv->set_blk_dirty(priv, CDC_REG_BLK_CURVE, 1);
				priv->gtm_status |= CDC_GTM_INITED;
			}
			reg->ctl.bits.gtm_en = 1;
		} else {
			priv->gtm_status &= !CDC_GTM_INITED;
			reg->ctl.bits.gtm_en = 0;
		}
		priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);

		if (priv->has_csc) {
			struct de_csc_info icsc_out, ocsc_in;
			u32 *icsc_coeff, *ocsc_coeff;

			icsc_out.eotf = in_info->eotf;
			memcpy((void *)&ocsc_in, out_info, sizeof(ocsc_in));
			if (convert_type == DE_TFC_HDR102SDR) {
				if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
					icsc_out.px_fmt_space = DE_FORMAT_SPACE_RGB;
					icsc_out.color_space = in_info->color_space;
					icsc_out.color_range = in_info->color_range;
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
			} else {
				if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
					icsc_out.px_fmt_space = DE_FORMAT_SPACE_RGB;
					icsc_out.color_space = in_info->color_space;
					icsc_out.color_range = in_info->color_range;
				} else if (in_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
					icsc_out.px_fmt_space = DE_FORMAT_SPACE_YUV;
					icsc_out.color_range = DE_COLOR_RANGE_16_235;
					if ((in_info->color_space != DE_COLOR_SPACE_BT2020NC)
						&& (in_info->color_space != DE_COLOR_SPACE_BT2020C))
						icsc_out.color_space = DE_COLOR_SPACE_BT709;
					else
						icsc_out.color_space = in_info->color_space;
					ocsc_in.color_range = DE_COLOR_RANGE_16_235;
				} else {
					DE_WARN("px_fmt_space %d no support",
						   in_info->px_fmt_space);
					return -1;
				}
			}

			de_csc_coeff_calc(in_info, &icsc_out, &icsc_coeff);
			de_csc_coeff_calc(&ocsc_in, out_info, &ocsc_coeff);
			cdc_set_csc_coeff(reg, icsc_coeff, ocsc_coeff);
			priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);
		} else {
			out_info->px_fmt_space = in_info->px_fmt_space;
			out_info->eotf = in_info->eotf;
		}

		if ((convert_type == priv->convert_type)
			&& (in_info->px_fmt_space == priv->in_px_fmt_space))
			return 0;

		priv->convert_type = convert_type;
		priv->in_px_fmt_space = in_info->px_fmt_space;
		if (in_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
			for (i = 0; i < DE_CDC_LUT_NUM; i++) {
				lut_ptr[i] = cdc_lut_ptr_r[convert_type][i];
				priv->set_blk_dirty(priv, CDC_REG_BLK_LUT0 + i, 1);
			}
		} else {
			for (i = 0; i < DE_CDC_LUT_NUM; i++) {
				lut_ptr[i] = cdc_lut_ptr_y[convert_type][i];
				priv->set_blk_dirty(priv, CDC_REG_BLK_LUT0 + i, 1);
			}
		}
		cdc_set_lut(reg, (u32 **)lut_ptr);
	} else {
		de_cdc_disable(cdc_hdl);
		memcpy((void *)out_info, (void *)in_info, sizeof(*out_info));
	}

	return bypass;
}

s32 de_cdc_disable(void *cdc_hdl)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct cdc_reg *reg = get_cdc_reg(priv);

	if (reg->ctl.bits.en != 0) {
		reg->ctl.dwval = 0;
		priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);
		priv->convert_type = DE_TFC_INIT;
		priv->in_px_fmt_space = 0xFF;
		priv->gtm_status &= !CDC_GTM_INITED;
	}
	return 0;
}

void *de_cdc_create(u8 __iomem *reg_base,
	u32 rcq_used, u8 has_csc)
{
	struct de_cdc_private *priv;
	struct de_reg_mem_info *reg_mem_info;
	struct de_reg_block *block;

	priv = cdc_alloc_private();
	if (priv == NULL)
		return NULL;

	reg_mem_info = &priv->reg_mem_info;
	reg_mem_info->size = sizeof(struct cdc_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (NULL == reg_mem_info->vir_addr) {
		DE_WARN("alloc for cdc reg mm fail! size=0x%x\n",
		     reg_mem_info->size);
		cdc_free_private(priv);
		return NULL;
	}

	block = &(priv->reg_blks[CDC_REG_BLK_CTL]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = has_csc ? 0x140 : 0x04;
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT0]);
	block->phy_addr = reg_mem_info->phy_addr + 0x1000;
	block->vir_addr = reg_mem_info->vir_addr + 0x1000;
	block->size = 729 * 4;
	block->reg_addr = reg_base + 0x1000;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT1]);
	block->phy_addr = reg_mem_info->phy_addr + 0x1C00;
	block->vir_addr = reg_mem_info->vir_addr + 0x1C00;
	block->size = 648 * 4;
	block->reg_addr = reg_base + 0x1C00;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT2]);
	block->phy_addr = reg_mem_info->phy_addr + 0x2800;
	block->vir_addr = reg_mem_info->vir_addr + 0x2800;
	block->size = 648 * 4;
	block->reg_addr = reg_base + 0x2800;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT3]);
	block->phy_addr = reg_mem_info->phy_addr + 0x3400;
	block->vir_addr = reg_mem_info->vir_addr + 0x3400;
	block->size = 576 * 4;
	block->reg_addr = reg_base + 0x3400;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT4]);
	block->phy_addr = reg_mem_info->phy_addr + 0x4000;
	block->vir_addr = reg_mem_info->vir_addr + 0x4000;
	block->size = 648 * 4;
	block->reg_addr = reg_base + 0x4000;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT5]);
	block->phy_addr = reg_mem_info->phy_addr + 0x4C00;
	block->vir_addr = reg_mem_info->vir_addr + 0x4C00;
	block->size = 576 * 4;
	block->reg_addr = reg_base + 0x4C00;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT6]);
	block->phy_addr = reg_mem_info->phy_addr + 0x5800;
	block->vir_addr = reg_mem_info->vir_addr + 0x5800;
	block->size = 576 * 4;
	block->reg_addr = reg_base + 0x5800;

	block = &(priv->reg_blks[CDC_REG_BLK_LUT7]);
	block->phy_addr = reg_mem_info->phy_addr + 0x6400;
	block->vir_addr = reg_mem_info->vir_addr + 0x6400;
	block->size = 512 * 4;
	block->reg_addr = reg_base + 0x6400;

	block = &(priv->reg_blks[CDC_REG_BLK_CURVE]);
	block->phy_addr = reg_mem_info->phy_addr + 0x7000;
	block->vir_addr = reg_mem_info->vir_addr + 0x7000;
	block->size = 1024 * 4;
	block->reg_addr = reg_base + 0x7000;

	priv->reg_blk_num = CDC_REG_BLK_NUM;

	priv->has_csc = has_csc;
	priv->convert_type = DE_TFC_INIT;
	priv->in_px_fmt_space = 0xFF;
	priv->gtm_status = CDC_GTM_INVAID;
	if (rcq_used)
		priv->set_blk_dirty = cdc_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = cdc_set_block_dirty;

	return priv;
}

s32 de_cdc_destroy(void *cdc_hdl)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct de_reg_mem_info *reg_mem_info = &priv->reg_mem_info;

	if (reg_mem_info->vir_addr != NULL)
		de_top_reg_memory_free(reg_mem_info->vir_addr,
			reg_mem_info->phy_addr, reg_mem_info->size);
	cdc_free_private(priv);

	return 0;
}

s32 de_cdc_get_reg_blks(void *cdc_hdl,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_cdc_private *priv = (struct de_cdc_private *)cdc_hdl;
	struct de_reg_block *blk_begin, *blk_end;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num)
		*blk_num = priv->reg_blk_num;
	else
		DE_WARN("should not happen\n");
	blk_begin = priv->reg_blks;
	blk_end = blk_begin + CDC_REG_BLK_NUM;
	for (; blk_begin != blk_end; ++blk_begin) {
		if (blk_begin->size != 0)
			*blks++ = blk_begin;
	}

	return 0;
}

int de_gtm_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data)
{
	u32 i = 0;
	struct de_cdc_private *priv = NULL;
	struct cdc_reg *reg = NULL;
	hdr_module_param_t *para = NULL;
	DE_INFO("sel=%d, cmd=%d, subcmd=%d, data=%px\n", sel, cmd, subcmd, data);
	para = (hdr_module_param_t *)data;

	if (para == NULL) {
		DE_WARN("para NULL\n");
		return -1;
	}

	for (i = 0; i < GLB_MAX_CDC_NUM; i++) {
		if (cdc_priv_alloced[i] == 0) {
			continue;
		}
		priv = &cdc_priv[i];
		reg = get_cdc_reg(priv);

		if (subcmd == 16) { /* read */
			para->value[0] = reg->ctl.bits.gtm_en;
			para->value[0] = reg->dynamic_mode.bits.dynamic_mode_en;
			para->value[1] = reg->gtm_hl_th.bits.hl_th;
			para->value[2] = reg->gtm_hl_ratio.bits.th_hig_pct;
			para->value[3] = reg->gtm_hl_ratio.bits.th_low_pct;
			para->value[4] = reg->gtm_src_nit_range.bits.src_nit_hig;
			para->value[5] = reg->gtm_src_nit_range.bits.src_nit_low;
		} else { /* write */
			tigerlcd_writing = 10;
			reg->dynamic_mode.bits.dynamic_mode_en = para->value[0];
			reg->gtm_hl_th.bits.hl_th = para->value[1];
			reg->gtm_hl_ratio.bits.th_hig_pct = para->value[2];
			reg->gtm_hl_ratio.bits.th_low_pct = para->value[3];
			reg->gtm_src_nit_range.bits.src_nit_hig = para->value[4];
			reg->gtm_src_nit_range.bits.src_nit_low = para->value[5];
			reg->gtm_src_max_nit.bits.src_max_nit = 0x3e8;
			reg->gtm_src_max_nit.bits.max_nit_idx = 0xf;
			reg->gtm_gamut_fration.bits.fraction_bit = 0xa;
			reg->gtm_gamut_coef[0].dwval = 0x000006a2;
			reg->gtm_gamut_coef[1].dwval = 0x0000fda7;
			reg->gtm_gamut_coef[2].dwval = 0x0000ffb6;
			reg->gtm_gamut_coef[3].dwval = 0x0000ff81;
			reg->gtm_gamut_coef[4].dwval = 0x00000486;
			reg->gtm_gamut_coef[5].dwval = 0x0000fff8;
			reg->gtm_gamut_coef[6].dwval = 0x0000ffee;
			reg->gtm_gamut_coef[7].dwval = 0x0000ff9a;
			reg->gtm_gamut_coef[8].dwval = 0x00000478;
			priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);
		}
	}
	return 0;
}

static int sw_highlight_ratio(u32 hw_hl, u32 width, u32 height, int th_low_pct,
						int th_high_pct, int dynamic_mode)
{
	int ratio;
	int th_low, th_high;
	s64 tmp;
	int len;

	len = width * height;
	th_low = th_low_pct * len;
	th_high = th_high_pct * len;

	if (dynamic_mode == 0) {
		ratio = 0;
	} else {
		if (hw_hl > th_low && hw_hl < th_high) {
			tmp = (s64)(hw_hl - th_low) * 255;
			ratio = (int)(tmp / (th_high - th_low));
		} else if (hw_hl >= th_high) {
			ratio = 255;
		} else {
			ratio = 0;
		}
	}
	return ratio;
}

static int clip_nit_gen(int ratio, int src_nit_low, int src_nit_high,
						int dynamic_mode)
{
	int clipnit = 0;

	if (dynamic_mode == 0) {
		clipnit = src_nit_low;
	} else {
		clipnit = ((255 - ratio) * src_nit_low + ratio * src_nit_high) >> 8;
	}

	return clipnit;
}

#define SRC_MIN_NIT 700
#define SRC_MAX_NIT 5000
#define SRC_NIT_NORM_STEP 20

static int clip_nit_idx(int clipnit)
{
	int clipnit_idx = 0;
	clipnit = clipnit > SRC_MIN_NIT ? clipnit : SRC_MIN_NIT;
	clipnit = clipnit < SRC_MAX_NIT ? clipnit : SRC_MAX_NIT;

	clipnit_idx = (int) ((clipnit - SRC_MIN_NIT) / SRC_NIT_NORM_STEP);
	return clipnit_idx;
}

s32 de_cdc_tasklet(u32 disp, u32 chn, u32 frame_cnt)
{
	u32 i = 0;
	struct de_cdc_private *priv = NULL;
	struct cdc_reg *reg = NULL;
	int ratio = 0;
	int clipnit = 0;
	int clipidx = 0;

	if (disp != 0 || chn != 0) {
		return 0;
	}

	for (i = 0; i < GLB_MAX_CDC_NUM; i++) {
		if (cdc_priv_alloced[i] == 0) {
			continue;
		}
		priv = &cdc_priv[i];
		reg = get_cdc_reg(priv);

		if (reg->ctl.bits.en == 0 && reg->dynamic_mode.bits.dynamic_mode_en == 0) {
			continue;
		}
#ifdef SUPPORT_AHB_READ
		reg->gtm_hl_val.dwval = readl(priv->reg_blks[0].reg_addr + 0x108);
#else
		reg->gtm_hl_val.dwval = 0;
#endif
		ratio = sw_highlight_ratio(reg->gtm_hl_val.bits.hl_val,
								   reg->cdc_size.bits.cdc_width,
								   reg->cdc_size.bits.cdc_height,
								   reg->gtm_hl_ratio.bits.th_low_pct,
								   reg->gtm_hl_ratio.bits.th_hig_pct,
								   reg->dynamic_mode.bits.dynamic_mode_en);
		clipnit = clip_nit_gen(ratio,
							   reg->gtm_src_nit_range.bits.src_nit_low,
							   reg->gtm_src_nit_range.bits.src_nit_hig,
							   reg->dynamic_mode.bits.dynamic_mode_en);

		clipidx = clip_nit_idx(clipnit);
		reg->gtm_hl_val.bits.hl_val = 0;
		reg->gtm_hl_ratio.bits.hl_ratio = ratio;
		reg->gtm_src_max_nit.bits.src_max_nit = clipnit;
		reg->gtm_src_max_nit.bits.max_nit_idx = clipidx;

		priv->set_blk_dirty(priv, CDC_REG_BLK_CTL, 1);
	}

	return 0;
}

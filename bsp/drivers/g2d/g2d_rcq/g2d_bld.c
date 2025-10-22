/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#include "g2d_bld.h"

__s32 rgb2Ycbcr_601[48] = {
	/* full to limit */
	/* (0.2568, 0.5041, 0.0979, 16) * 1024
	 * (-0.1482, -0.291, 0.4392, 128) * 1024
	 * (0.4392, -0.3678, -0.0714, 128) * 1024
	 * */
	0x107, 0x204, 0x064, 0x4000, 0xFFFFFF68, 0xFFFFFED6, 0x01c2, 0x20000,
	0x01c2, 0xFFFFFE87, 0xFFFFFFB7, 0x20000,
	/* limit to limit */
	0x132, 0x259, 0x075, 0x0, 0xFFFFFF4F, 0xFFFFFEA5, 0x020c, 0x20000,
	0x020c, 0xFFFFFE49, 0xFFFFFFAB, 0x20000,
	/* full to full */
	0x132, 0x0259, 0x075, 0x0, 0xFFFFFF53, 0xFFFFFEAD, 0x0200, 0x20000,
	0x0200, 0xFFFFFE53, 0xFFFFFFAD, 0x20000,
	/* limit to full */
	0x0165, 0x02BC, 0x088, 0xFFFFF570, 0xFFFFFF37, 0xFFFFFE75, 0x0254, 0x20000,
	0x0254, 0xFFFFFE0D, 0xFFFFFF9F, 0x20000,
};
__s32 Ycbcr2rgb_601[48] = {
	/* full to limit */
	0x04a8, 0x0, 0x0662, 0xFFFC8480, 0x04a8, 0xFFFFFE6F, 0xFFFFFCC0,
	0x21E00, 0x04a8, 0x0812, 0x0, 0xFFFBAC80,
	/* limit to limit */
	0x0400, 0x0, 0x057c, 0xFFFD0200, 0x0400, 0xFFFFFEA7, 0xFFFFFD35, 0x1D200,
	0x0400, 0x06EE, 0x0, 0xFFFC4900,
	/* full to full */
	0x0400, 0x0, 0x059c, 0xFFFD31FF, 0x0400, 0xFFFFFE9F, 0xFFFFFD24, 0x21D80,
	0x0400, 0x0717, 0x0, 0xFFFC747F,
	/* limit to full */
	0x036F, 0x0, 0x04D1, 0xFFFDA090, 0x36F, 0xFFFFFED1, 0xFFFFFD8C, 0x1DA90,
	0x036F, 0x0616, 0x0, 0xFFFCFE10,
};
__s32 rgb2Ycbcr_709[48] = {
	/* full to limit */
	0x0bb, 0x0275, 0x03f, 0x4000, 0xFFFFFF99, 0xFFFFFEA5, 0x01c2, 0x20000,
	0x01c2, 0xFFFFFE67, 0xFFFFFFD7, 0x20000,
	/* limit to limit */
	0x0DA, 0x02DC, 0x04A, 0x0, 0xFFFFFF88, 0xFFFFFE6C, 0x020c, 0x20000,
	0x020c, 0xFFFFFE24, 0xFFFFFFD0, 0x20000,
	/* full to full */
	0x0DA, 0x02DC, 0x04a, 0x0, 0xFFFFFF8B, 0xFFFFFE75, 0x0200, 0x20000,
	0x0200, 0xFFFFFE2F, 0xFFFFFFD1, 0x20000,
	/* limit to full */
	0x0FD, 0x0355, 0x056, 0xFFFFF580, 0xFFFFFF77, 0xFFFFFE34, 0x0254, 0x20010,
	0x0254, 0xFFFFFDE3, 0xFFFFFFC9, 0x20000,
};

__s32 Ycbcr2rgb_709[48] = {
	/* full to limit */
	0x04a8, 0x0, 0x072c, 0xFFFC1F80, 0x04a8, 0xFFFFFF26, 0xFFFFFDDE,
	0x13380, 0x04a8, 0x0873, 0, 0xFFFB7C00,
	/* limit to limit */
	0x0400, 0x0, 0x0629, 0xFFFCAB80, 0x0400, 0xFFFFFF44, 0xFFFFE2B, 0x10880,
	0x0400, 0x0742, 0x0, 0xFFFC1F00,
	/* full to full */
	0x0400, 0x0, 0x64D, 0xFFFC9980, 0x0400, 0xFFFFFF40, 0xFFFFFE21, 0x10F80,
	0x0400, 0x076C, 0x0, 0xFFFC0A00,
	/* limit to full */
	0x36f, 0x0, 0x0569, 0xFFFD5490, 0x36f, 0xFFFFFF5B, 0xFFFFFE64, 0x12990,
	0x36f, 0x0660, 0x0, 0xFFFCD910,
};

/*
 * sel: 0-->pipe0 1-->pipe1 other:error
 */
__s32 bld_in_set(struct blender_submodule *p_bld, __u32 sel, g2d_rect rect,
		    int premul)
{
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	if (sel == 0) {
		p_reg->bld_en_ctrl.bits.p0_en = 1;
		/* we best use p0 as bottom layer */
		p_reg->bld_en_ctrl.bits.p0_fcen = 1;
		if (premul)
			p_reg->premulti_ctrl.bits.p0_alpha_mode = 1;
	} else if (sel == 1) {
		p_reg->bld_en_ctrl.bits.p1_en = 1;
		if (premul)
			p_reg->premulti_ctrl.bits.p1_alpha_mode = 1;
	} else
		goto OUT;

	p_reg->mem_size[sel].bits.width = rect.w - 1;
	p_reg->mem_size[sel].bits.height = rect.h - 1;

	p_reg->mem_coor[sel].bits.xcoor = rect.x <= 0 ? 0 : rect.x - 1;
	p_reg->mem_coor[sel].bits.ycoor = rect.y <= 0 ? 0 : rect.y - 1;

	ret = 0;

	p_bld->set_block_dirty(p_bld, 0, 1);

OUT:
	return ret;
}

/**
 * set colorkey para.
 */
__s32 bld_ck_para_set(struct blender_submodule *p_bld, g2d_ck *para, __u32 flag)
{
	__u32 tmp = 0x0;
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg || !para)
		goto OUT;

	if (para->match_rule)
		tmp = 0x7;

	p_reg->color_key_cfg.dwval = tmp;
	p_reg->color_key_max.dwval = para->max_color & 0x00ffffff;
	p_reg->color_key_min.dwval = para->min_color & 0x00ffffff;

	if (flag & G2D_CK_SRC) {
		p_reg->color_key.bits.key0_en = 1;
		p_reg->color_key.bits.key0_match_dir = 0;
	} else if (flag & G2D_CK_DST) {
		p_reg->color_key.bits.key0_en = 1;
		p_reg->color_key.bits.key0_match_dir = 1;
	}

	p_bld->set_block_dirty(p_bld, 0, 1);
OUT:
	return ret;
}

/**
 * background color set
 */
__s32 bld_bk_set(struct blender_submodule *p_bld, __u32 color)
{
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	p_reg->bld_backgroud_color = color & 0xffffffff;
	ret = 0;
	p_bld->set_block_dirty(p_bld, 0, 1);
OUT:
	return ret;
}

__s32 bld_out_setting(struct blender_submodule *p_bld, g2d_image_enh *p_image)
{
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	if (p_image->bpremul)
		p_reg->out_color.bits.premul_en = 1;
	else
		p_reg->out_color.bits.premul_en = 0;
	p_reg->out_size.bits.width =
	    p_image->clip_rect.w == 0 ? 0 : p_image->clip_rect.w - 1;
	p_reg->out_size.bits.height =
	    p_image->clip_rect.h == 0 ? 0 : p_image->clip_rect.h - 1;
	p_bld->set_block_dirty(p_bld, 0, 1);
	ret = 0;
OUT:
	return ret;
}

__s32 bld_set_rop_ctrl(struct blender_submodule *p_bld, __u32 value)
{
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	p_reg->rop_ctrl.dwval = value;
	p_reg->ch3_index0.dwval = 0x41000;
	ret = 0;
	p_bld->set_block_dirty(p_bld, 0, 1);
OUT:
	return ret;
}

/**
 * set the bld color space based on the format
 * if the format is UI, then set the bld in RGB color space
 * if the format is Video, then set the bld in YUV color space
 */
__s32 bld_cs_set(struct blender_submodule *p_bld, __u32 format)
{
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	if (format <= G2D_FORMAT_BGRA1010102) {
		p_reg->out_color.bits.alpha_mode = 0;
	} else if (format <= G2D_FORMAT_YUV411_PLANAR) {
		p_reg->out_color.bits.alpha_mode = 1;
	} else
		goto OUT;

	p_bld->set_block_dirty(p_bld, 0, 1);
	ret = 0;
OUT:
	return ret;
}

/**
 * @csc_no: CSC ID, G2D support three CSC,
 * -1 will return to indicate inappropriate CSC number.
 * @csc_sel: CSC format, G2D support the ITU-R 601. ITU-R 709. standard trans-
 *  form between RGB and YUV colorspace.
 */
__s32 bld_csc_reg_set(struct blender_submodule *p_bld, __u32 csc_no,
		g2d_csc_sel csc_sel, enum color_range src_cr, enum color_range dst_cr)
{
	void *csc_base_addr;
	__s32 ret = -1;

	struct g2d_mixer_bld_reg *p_reg = NULL;
	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	p_reg->bld_fill_color[0] = 0x00108080;
	p_reg->bld_fill_color[1] = 0x00108080;

	switch (csc_no) {
	case 0:
		csc_base_addr = &p_reg->csc0_coeff0_reg0;
		p_reg->cs_ctrl.bits.cs0_en = 1;
		break;
	case 1:
		csc_base_addr = &p_reg->csc1_coeff0_reg0;
		p_reg->cs_ctrl.bits.cs1_en = 1;
		break;
	case 2:
		csc_base_addr = &p_reg->csc2_coeff0_reg0;
		p_reg->cs_ctrl.bits.cs2_en = 1;
		break;
	default:
		goto OUT;
	}
	switch (csc_sel) {
	case G2D_RGB2YUV_709:
		if (src_cr == COLOR_RANGE_0_255) {
			if (dst_cr == COLOR_RANGE_0_255)
				/* full to full */
				memcpy(csc_base_addr, (rgb2Ycbcr_709 + 0x18), 12 * sizeof(unsigned int));
			else
				/* full to limit */
				memcpy(csc_base_addr, rgb2Ycbcr_709, 12 * sizeof(unsigned int));
		} else {
			if (dst_cr == COLOR_RANGE_0_255)
				/* limit to full */
				memcpy(csc_base_addr, (rgb2Ycbcr_709 + 0x24), 12 * sizeof(unsigned int));
			else
				/* limit to limit */
				memcpy(csc_base_addr, (rgb2Ycbcr_709 + 0xc), 12 * sizeof(unsigned int));
		}
		break;
	case G2D_YUV2RGB_709:
		if (src_cr == COLOR_RANGE_0_255) {
			if (dst_cr == COLOR_RANGE_0_255)
				/* full to full */
				memcpy(csc_base_addr, (Ycbcr2rgb_709 + 0x18), 12 * sizeof(unsigned int));
			else
				/* full to limit */
				memcpy(csc_base_addr, Ycbcr2rgb_709, 12 * sizeof(unsigned int));
		} else {
			if (dst_cr == COLOR_RANGE_0_255)
				/* limit to full */
				memcpy(csc_base_addr, (Ycbcr2rgb_709 + 0x24), 12 * sizeof(unsigned int));
			else
				/* limit to limit */
				memcpy(csc_base_addr, (Ycbcr2rgb_709 + 0xc), 12 * sizeof(unsigned int));
		}
		break;
	case G2D_RGB2YUV_601:
		if (src_cr == COLOR_RANGE_0_255) {
			if (dst_cr == COLOR_RANGE_0_255)
				/* full to full */
				memcpy(csc_base_addr, (rgb2Ycbcr_601 + 0x18), 12 * sizeof(unsigned int));
			else
				/* full to limit */
				memcpy(csc_base_addr, rgb2Ycbcr_601, 12 * sizeof(unsigned int));
		} else {
			if (dst_cr == COLOR_RANGE_0_255)
				/* limit to full */
				memcpy(csc_base_addr, (rgb2Ycbcr_601 + 0x24), 12 * sizeof(unsigned int));
			else
				/* limit to limit */
				memcpy(csc_base_addr, (rgb2Ycbcr_601 + 0xc), 12 * sizeof(unsigned int));
		}
		break;
	case G2D_YUV2RGB_601:
		if (src_cr == COLOR_RANGE_0_255) {
			if (dst_cr == COLOR_RANGE_0_255)
				/* full to full */
				memcpy(csc_base_addr, (Ycbcr2rgb_601 + 0x18), 12 * sizeof(unsigned int));
			else
				/* full to limit */
				memcpy(csc_base_addr, Ycbcr2rgb_601, 12 * sizeof(unsigned int));
		} else {
			if (dst_cr == COLOR_RANGE_0_255)
				/* limit to full */
				memcpy(csc_base_addr, (Ycbcr2rgb_601 + 0x24), 12 * sizeof(unsigned int));
			else
				/* limit to limit */
				memcpy(csc_base_addr, (Ycbcr2rgb_601 + 0xc), 12 * sizeof(unsigned int));
		}
		break;
	default:
		G2D_WARN("No implement standard:%d!\n", csc_sel);
		goto OUT;
	}

	p_bld->set_block_dirty(p_bld, 0, 1);
	ret = 0;
OUT:
	return ret;
}

__s32 bld_porter_duff(struct blender_submodule *p_bld, __u32 cmd)
{
	struct g2d_mixer_bld_reg *p_reg = NULL;
	__s32 ret = -1;

	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	switch (cmd) {
	case G2D_BLD_CLEAR:
		p_reg->bld_ctrl.dwval = 0x00000000;
		break;
	case G2D_BLD_COPY:
		p_reg->bld_ctrl.dwval = 0x00010001;
		break;
	case G2D_BLD_DST:
		p_reg->bld_ctrl.dwval = 0x01000100;
		break;
	case G2D_BLD_SRCOVER:
		p_reg->bld_ctrl.dwval = 0x03010301;
		break;
	case G2D_BLD_DSTOVER:
		p_reg->bld_ctrl.dwval = 0x01030103;
		break;
	case G2D_BLD_SRCIN:
		p_reg->bld_ctrl.dwval = 0x00020002;
		break;
	case G2D_BLD_DSTIN:
		p_reg->bld_ctrl.dwval = 0x02000200;
		break;
	case G2D_BLD_SRCOUT:
		p_reg->bld_ctrl.dwval = 0x00030003;
		break;
	case G2D_BLD_DSTOUT:
		p_reg->bld_ctrl.dwval = 0x03000300;
		break;
	case G2D_BLD_SRCATOP:
		p_reg->bld_ctrl.dwval = 0x03020302;
		break;
	case G2D_BLD_DSTATOP:
		p_reg->bld_ctrl.dwval = 0x02030203;
		break;
	case G2D_BLD_XOR:
		p_reg->bld_ctrl.dwval = 0x03030303;
		break;
	default:
		p_reg->bld_ctrl.dwval = 0x03010301;
	}
	p_bld->set_block_dirty(p_bld, 0, 1);
	ret = 0;
OUT:
	return ret;
}

int bld_rcq_setup(struct blender_submodule *p_bld, u8 __iomem *base,
		  struct g2d_rcq_mem_info *p_rcq_info)
{
	u8 __iomem *reg_base = base + G2D_BLD;
	int ret = -1;

	if (!p_bld) {
		G2D_WARN("Null pointer!\n");
		goto OUT;
	}

	p_bld->reg_info->size = sizeof(struct g2d_mixer_bld_reg);
	p_bld->reg_info->vir_addr = (u8 *)g2d_top_reg_memory_alloc(
	    p_bld->reg_info->size, (void *)&(p_bld->reg_info->phy_addr),
	    p_rcq_info);

	if (!p_bld->reg_info->vir_addr) {
		G2D_WARN("Malloc blender reg rcq memory fail!\n");
		goto OUT;
	}

	p_bld->reg_blks->vir_addr = p_bld->reg_info->vir_addr;
	p_bld->reg_blks->phy_addr = p_bld->reg_info->phy_addr;
	p_bld->reg_blks->size = p_bld->reg_info->size;
	p_bld->reg_blks->reg_addr = reg_base;
	ret = 0;
OUT:
	return ret;
}

/**
 * ROP2 cmd register set
 * Index0 is selected
 * dst mapping ch0'
 * src mapping ch1'
 */
__s32 bld_rop2_set(struct blender_submodule *p_bld, __u32 rop_cmd)
{
	__s32 ret = -1;
	struct g2d_mixer_bld_reg *p_reg = p_bld->get_reg(p_bld);

	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	if (rop_cmd == G2D_BLT_BLACKNESS) {
		/* blackness */
		/* tmpue = 0x1<<18; */
		p_reg->ch3_index0.dwval = 0x40000;
	} else if (rop_cmd == G2D_BLT_NOTMERGEPEN) {
		/* ~(dst | src) */
		/* tmpue = (0x1<<6) | (0x1<<10) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41440;
	} else if (rop_cmd == G2D_BLT_MASKNOTPEN) {
		/* ~src&dst */
		/* tmpue = (0x1<<4) | (0x0<<10) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41010;
	} else if (rop_cmd == G2D_BLT_NOTCOPYPEN) {
		/* ~src */
		/* tmpue = (0x1<<4) | (0x2<<6) | (0x2<<11) |
		 * (0x1<<18) | (0x1<<17);
		 */
		p_reg->ch3_index0.dwval = 0x61090;
	} else if (rop_cmd == G2D_BLT_MASKPENNOT) {
		/* src&~dst */
		/* tmpue = (0x1<<3) | (0x0<<10) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41008;
	} else if (rop_cmd == G2D_BLT_NOT) {
		/* ~dst */
		/* tmpue = (0x1<<3) | (0x2<<6) | (0x2<<11) |
		 * (0x1<<18) | (0x1<<16);
		 */
		p_reg->ch3_index0.dwval = 0x51088;
	} else if (rop_cmd == G2D_BLT_XORPEN) {
		/* src xor dst */
		/* tmpue = (0x2<<6) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41080;
	} else if (rop_cmd == G2D_BLT_NOTMASKPEN) {
		/* ~(src & dst) */
		/* tmpue = (0x0<<6) | (0x1<<10) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41400;
	} else if (rop_cmd == G2D_BLT_MASKPEN) {
		/* src&dst */
		/* tmpue = (0x0<<6) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41000;
	} else if (rop_cmd == G2D_BLT_NOTXORPEN) {
		/* ~(src xor dst) */
		/* tmpue = (0x2<<6) | (0x1<<10) | (0x2<<11) | (0x1<<18); */
		p_reg->ch3_index0.dwval = 0x41480;
	} else if (rop_cmd == G2D_BLT_NOP) {
		/* dst */
		/* tmpue = (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<16); */
		p_reg->ch3_index0.dwval = 0x51080;
	} else if (rop_cmd == G2D_BLT_MERGENOTPEN) {
		/* ~dst or src */
		/* tmpue = (0x1<<3)| (0x1<<6) | (0x2<<11) | (0x1<<18) */
		/* write_wvalue(ROP_INDEX0, 0x40A20); */
		p_reg->ch3_index0.dwval = 0x41048;
	} else if (rop_cmd == G2D_BLT_COPYPEN) {
		/* src */
		/* tmpue = (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<17); */
		p_reg->ch3_index0.dwval = 0x61080;
	} else if (rop_cmd == G2D_BLT_MERGEPENNOT) {
		/* src or ~dst */
		/* tmpue =  (0x1<<3)| (0x1<<6) | (0x2<<11) | (0x1<<18) */
		p_reg->ch3_index0.dwval = 0x41048;
	} else if (rop_cmd == G2D_BLT_MERGEPEN) {
		/* src or dst */
		/* tmpue = (0x1<<6) | (0x1<<18) | (0x2<<11); */
		p_reg->ch3_index0.dwval = 0x41040;
	} else if (rop_cmd == G2D_BLT_WHITENESS) {
		/* whiteness */
		/* tmpue = (0x1<<18) | (0x1<<15); */
		p_reg->ch3_index0.dwval = 0x48000;
	} else
		goto OUT;

	p_reg->ch3_index0.bits.index0node0 = 2; /* TODO:different with source */
	p_bld->set_block_dirty(p_bld, 0, 1);

	ret = 0;
OUT:
	return ret;
}

/**
 * ROP3 cmd register set
 * dst mapping ch0'
 * src mapping ch1'
 * ptn mapping ch2'
 * -1 return meaning that the operate is not supported by now
 */
__s32 bld_rop3_set(struct blender_submodule *p_bld, __u32 sel, __u32 rop3_cmd)
{
	__s32 ret = -1;
	union g2d_mixer_rop_ch3_index0 *p_addr = NULL;
	struct g2d_mixer_bld_reg *p_reg = p_bld->get_reg(p_bld);

	p_reg = p_bld->get_reg(p_bld);
	if (!p_reg)
		goto OUT;

	if (sel == 0)
		p_addr = &p_reg->ch3_index0;
	else if (sel == 1)
		p_addr = &p_reg->ch3_index1;
	else
		goto OUT;

	if (rop3_cmd == G2D_ROP3_BLACKNESS) {
		/* blackness */
		/* 0x1<<18; */
		p_addr->dwval = 0x40000;
	} else if (rop3_cmd == G2D_ROP3_NOTSRCERASE) {
		/* (~src) AND (~dst) */
		/* (0x1<<3) | (0x1<<4) | (0x1<<18) | (0x2<<11); */
		p_addr->dwval = 0x41018;
	} else if (rop3_cmd == G2D_ROP3_NOTSRCCOPY) {

		/* ~src */
		/* (0x1<<4) | (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<16); */
		p_addr->dwval = 0x51090;
	} else if (rop3_cmd == G2D_ROP3_SRCERASE) {
		/* src AND ~dst */
		/* (0x1<<3) | (0x0<<6) | (0x2<<11) | (0x1<<18); */
		p_addr->dwval = 0x41008;
	} else if (rop3_cmd == G2D_ROP3_DSTINVERT) {
		/* ~dst */
		/* (0x1<<3) | (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<17); */
		p_addr->dwval = 0x61088;
	} else if (rop3_cmd == G2D_ROP3_PATINVERT) {
		/* ptn XOR dst */
		/* (0x2<<6) | (0x2<<11) | (0x1<<17) */
		p_addr->dwval = 0x21080;
	} else if (rop3_cmd == G2D_ROP3_SRCINVERT) {
		/* src XOR dst */
		/* (0x2<<6) | (0x2<<11) | (0x1<<18); */
		p_addr->dwval = 0x41080;
	} else if (rop3_cmd == G2D_ROP3_SRCAND) {
		/* src AND dst */
		/* (0x0<<6) | (0x2<<11) | (0x1<<18); */
		p_addr->dwval = 0x41000;
	} else if (rop3_cmd == G2D_ROP3_MERGEPAINT) {
		/* ~src OR dst */
		/* (0x1<<4) | (0x1<<6) | (0x2<<11) | (0x1<<18); */
		p_addr->dwval = 0x41050;
	} else if (rop3_cmd == G2D_ROP3_MERGECOPY) {
		/* src AND pattern */
		/* (0x2<<6) | (0x1<<16) */
		p_addr->dwval = 0x10080;
	} else if (rop3_cmd == G2D_ROP3_SRCCOPY) {
		/* src */
		/* (0x2<<6) | (0x2<<11) | (0x1<<18) | (0x1<<16); */
		p_addr->dwval = 0x51080;
	} else if (rop3_cmd == G2D_ROP3_SRCPAINT) {
		/* src OR dst */
		/* (0x1<<6) | (0x2<<11) | (0x1<<18); */
		p_addr->dwval = 0x41040;
	} else if (rop3_cmd == G2D_ROP3_PATCOPY) {
		/* ptn */
		/* (0x1<<16) | (0x1<<17) | (0x2)<<11 */
		p_addr->dwval = 0x31000;
	} else if (rop3_cmd == G2D_ROP3_PATPAINT) {
		/* DPSnoo */
		/* (0x1<<3) | (0x1<<6) | (0x1<<11) */
		p_addr->dwval = 0x848;
	} else if (rop3_cmd == G2D_ROP3_WHITENESS) {
		/* whiteness */
		p_addr->dwval = 0x48000;
	} else
		goto OUT;

	p_bld->set_block_dirty(p_bld, 0, 1);

	ret = 0;
OUT:
	return ret;
}

static __u32 bld_get_reg_block_num(struct blender_submodule *p_bld)
{
	if (p_bld)
		return p_bld->reg_blk_num;
	return 0;
}

static __s32 bld_get_reg_block(struct blender_submodule *p_bld,
			    struct g2d_reg_block **blks)
{
	__s32 i = 0, ret = -1;

	if (p_bld) {
		for (i = 0; i < p_bld->reg_blk_num; ++i)
			blks[i] = p_bld->reg_blks + i;
	}

	return ret;
}

static struct g2d_mixer_bld_reg *bld_get_reg(struct blender_submodule *p_bld)
{
#if G2D_MIXER_RCQ_USED == 1
	return (struct g2d_mixer_bld_reg *)(p_bld->reg_blks
					    ->vir_addr);
#else
	return (struct g2d_mixer_bld_reg *)(p_bld->reg_blks
					    ->reg_addr);
#endif
	return NULL;
}

static void bld_set_block_dirty(struct blender_submodule *p_bld, __u32 blk_id, __u32 dirty)
{
#if G2D_MIXER_RCQ_USED == 1
	if (p_bld && p_bld->reg_blks->rcq_hd)
		p_bld->reg_blks->rcq_hd->dirty.bits.dirty = dirty;
	else
		G2D_WARN("Null pointer!\n");
#else

	if (p_bld)
		p_bld->reg_blks->dirty = dirty;
	else
		G2D_WARN("Null pointer!\n");
#endif
}

static __u32 bld_get_rcq_mem_size(struct blender_submodule *p_bld)
{
	return G2D_RCQ_BYTE_ALIGN(sizeof(struct g2d_mixer_bld_reg));
}

static __s32 bld_destory(struct blender_submodule *p_bld)
{
	__s32 ret = -1;

	if (p_bld) {
		kfree(p_bld->reg_blks);
		p_bld->reg_blks = NULL;

		kfree(p_bld->reg_info);
		p_bld->reg_info = NULL;
		kfree(p_bld);
		ret = 0;
	}

	return ret;
}

struct blender_submodule *
g2d_bld_submodule_setup(struct g2d_mixer_frame *p_frame)
{
	struct blender_submodule *p_bld = NULL;

	p_bld = kmalloc(sizeof(*p_bld), GFP_KERNEL | __GFP_ZERO);

	if (!p_bld) {
		G2D_WARN("Kmalloc wb submodule fail!\n");
		return NULL;
	}

	p_bld->rcq_setup = bld_rcq_setup;
	p_bld->reg_blk_num = 1;
	p_bld->get_reg_block_num = bld_get_reg_block_num;
	p_bld->get_reg_block = bld_get_reg_block;
	p_bld->get_reg = bld_get_reg;
	p_bld->set_block_dirty = bld_set_block_dirty;
	p_bld->get_rcq_mem_size = bld_get_rcq_mem_size;
	p_bld->destory = bld_destory;

	p_bld->reg_blks =
	    kmalloc(sizeof(*(p_bld->reg_blks)) * p_bld->reg_blk_num,
		    GFP_KERNEL | __GFP_ZERO);
	p_bld->reg_info =
	    kmalloc(sizeof(*(p_bld->reg_info)), GFP_KERNEL | __GFP_ZERO);

	if (!p_bld->reg_blks || !p_bld->reg_info) {
		G2D_WARN("Kmalloc wb reg info fail!\n");
		goto FREE_WB;
	}


	return p_bld;
FREE_WB:
	kfree(p_bld->reg_blks);
	kfree(p_bld->reg_info);
	kfree(p_bld);

	return NULL;
}

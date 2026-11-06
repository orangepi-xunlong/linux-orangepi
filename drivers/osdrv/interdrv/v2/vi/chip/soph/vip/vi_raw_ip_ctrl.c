#include <vip/vi_drv.h>

/*******************************************************************************
 *	RAW IPs config
 ******************************************************************************/
static void _bnr_init(struct isp_ctx *ctx, enum isp_blk_id_t blk_id)
{
	uintptr_t bnr = ctx->phys_regs[blk_id];
	u8 intensity_sel[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	u8 weight_lut[256] = {
		31, 16, 8,  4,	2,  1,	1,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	};
	u16 i = 0;

	ISP_WO_BITS(bnr, reg_isp_bnr_t, index_clr, bnr_index_clr, 1);
	for (i = 0; i < ARRAY_SIZE(intensity_sel); ++i)
		ISP_WR_REG(bnr, reg_isp_bnr_t, intensity_sel, intensity_sel[i]);
	for (i = 0; i < ARRAY_SIZE(weight_lut); ++i)
		ISP_WR_REG(bnr, reg_isp_bnr_t, weight_lut, weight_lut[i]);

	ISP_WO_BITS(bnr, reg_isp_bnr_t, shadow_rd_sel,
		    shadow_rd_sel, 1);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, out_sel,
		    bnr_out_sel, ISP_BNR_OUT_BYPASS);

	ISP_WO_BITS(bnr, reg_isp_bnr_t, strength_mode,
		    bnr_strength_mode, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_intra_0,
		    bnr_weight_intra_0, 6);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_intra_1,
		    bnr_weight_intra_1, 6);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_intra_2,
		    bnr_weight_intra_2, 6);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_norm_1,
		    bnr_weight_norm_1, 7);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_norm_2,
		    bnr_weight_norm_2, 5);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, neighbor_max,
		    bnr_flag_neighbor_max, 1);

	ISP_WO_BITS(bnr, reg_isp_bnr_t, res_k_smooth,
		    bnr_res_ratio_k_smooth, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, res_k_texture,
		    bnr_res_ratio_k_texture, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, var_th, bnr_var_th, 128);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_sm, bnr_weight_smooth, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_v, bnr_weight_v, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_h, bnr_weight_h, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_d45, bnr_weight_d45, 0);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, weight_d135, bnr_weight_d135, 0);

	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_slope_b,
		    bnr_ns_slope_b, 135);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_slope_gb,
		    bnr_ns_slope_gb, 106);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_slope_gr,
		    bnr_ns_slope_gr, 106);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_slope_r,
		    bnr_ns_slope_r, 127);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset0_b,
		    bnr_ns_low_offset_b, 177);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset0_gb,
		    bnr_ns_low_offset_gb, 169);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset0_gr,
		    bnr_ns_low_offset_gr, 169);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset0_r,
		    bnr_ns_low_offset_r, 182);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset1_b,
		    bnr_ns_high_offset_b, 1023);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset1_gb,
		    bnr_ns_high_offset_gb, 1023);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset1_gr,
		    bnr_ns_high_offset_gr, 1023);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_offset1_r,
		    bnr_ns_high_offset_r, 1023);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_luma_th_b,
		    bnr_ns_luma_th_b, 160);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_luma_th_gb,
		    bnr_ns_luma_th_gb, 160);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_luma_th_gr,
		    bnr_ns_luma_th_gr, 160);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_luma_th_r,
		    bnr_ns_luma_th_r, 160);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_gain, bnr_ns_gain, 0);
}

void ispblk_bnr_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, enum isp_bnr_out_e out_sel,
			bool lsc_en, u8 ns_gain, u8 str)
{
	uintptr_t bnr = ctx->phys_regs[blk_id];

	// Init once tuning
	_bnr_init(ctx, blk_id);

	ISP_WO_BITS(bnr, reg_isp_bnr_t, out_sel, bnr_out_sel, out_sel);

	//ISP_WO_BITS(bnr, reg_isp_bnr_t, hsize, bnr_hsize, ctx->img_width);
	//ISP_WO_BITS(bnr, reg_isp_bnr_t, vsize, bnr_vsize, ctx->img_height);
	//ISP_WO_BITS(bnr, reg_isp_bnr_t, ns_gain, bnr_ns_gain, ns_gain);
	ISP_WO_BITS(bnr, reg_isp_bnr_t, strength_mode, bnr_strength_mode, str);
}

void ispblk_cfa_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id)
{
	uintptr_t cfa = ctx->phys_regs[blk_id];
	union reg_isp_cfa_00 reg_0;

	reg_0.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_00);
	reg_0.bits.cfa_shdw_sel = 1;
	reg_0.bits.cfa_enable	= 1;
	//reg_0.bits.cfa_fcr_enable = 1;
	//reg_0.bits.cfa_moire_enable = 1;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_00, reg_0.raw);
}

void ispblk_lsc_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en)
{
	uintptr_t lsc = ctx->phys_regs[blk_id];

	int width = ctx->img_width;
	int height = ctx->img_height;
	int mesh_num = 37;
	int InnerBlkX = mesh_num - 1 - 2;
	int InnerBlkY = mesh_num - 1 - 2;
	int mesh_x_coord_unit = (InnerBlkX * (1 << 15)) / width;
	int mesh_y_coord_unit = (InnerBlkY * (1 << 15)) / height;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;
	u32 reg_lsc_ystep = mesh_y_coord_unit + 1;

	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int image_h_in_mesh_unit = height * reg_lsc_ystep;
	int OuterBlkX = InnerBlkX + 2;
	int OuterBlkY = InnerBlkY + 2;
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << 15) - image_w_in_mesh_unit) / 2;
	u32 reg_lsc_imgy0 = (OuterBlkY * (1 << 15) - image_h_in_mesh_unit) / 2;

	union reg_isp_lsc_interpolation inter_p;

	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_bld, lsc_bldratio_enable, en);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_bld, lsc_bldratio, 0x100);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_dmi_heightm1, 0xdd);

	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_xstep, reg_lsc_xstep);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_ystep, reg_lsc_ystep);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_imgx0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_imgy0, reg_lsc_imgy0);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_initx0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_inity0, reg_lsc_imgy0);

	//Tuning
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_strength, 0xfff);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_gain_base, 0x0);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_gain_3p9_0_4p8_1, 0);
	//ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_renormalize_enable, 1);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_gain_bicubic_0_bilinear_1, 1);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_boundary_interpolation_mode, 1);

	inter_p.raw = ISP_RD_REG(lsc, reg_isp_lsc_t, interpolation);
	inter_p.bits.lsc_boundary_interpolation_lf_range = 0x3;
	inter_p.bits.lsc_boundary_interpolation_up_range = 0x4;
	inter_p.bits.lsc_boundary_interpolation_rt_range = 0x1f;
	inter_p.bits.lsc_boundary_interpolation_dn_range = 0x1c;
	ISP_WR_REG(lsc, reg_isp_lsc_t, interpolation, inter_p.raw);

	ISP_WR_BITS(lsc, reg_isp_lsc_t, dmi_enable, dmi_enable, en);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_enable, en);
}

void ispblk_aehist_reset(struct isp_ctx *ctx, int blk_id, enum sop_isp_raw raw_num)
{
#if 0
	uintptr_t sts = ctx->phys_regs[blk_id];

	ISP_WR_REG(sts, reg_isp_ae_hist_t, ae_hist_grace_reset, 1);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, ae_hist_grace_reset, 0);
#endif
}

void ispblk_aehist_config(struct isp_ctx *ctx, int blk_id, bool enable)
{
	uintptr_t sts = ctx->phys_regs[blk_id];
	u8 num_x = 34, num_y = 30;
	u8 sub_window_w = 0, sub_window_h = 0;
	union reg_isp_ae_hist_sts_ae0_hist_enable ae_enable;

	ae_enable.raw = ISP_RD_REG(sts, reg_isp_ae_hist_t, sts_ae0_hist_enable);
	ae_enable.bits.sts_ae0_hist_enable	= enable;
	ae_enable.bits.ae0_gain_enable	= enable;
	ae_enable.bits.hist0_enable	= enable;
	ae_enable.bits.hist0_gain_enable = enable;
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae0_hist_enable, ae_enable.raw);
	ISP_WR_BITS(sts, reg_isp_ae_hist_t, dmi_enable, dmi_enable, enable);
	if (!enable)
		return;

	sub_window_w = ctx->img_width / num_x;
	sub_window_h = ctx->img_height / num_y;

	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_numxm1, num_x - 1);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_numym1, num_y - 1);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_width, sub_window_w);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_height, sub_window_h);
}

void ispblk_gms_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_GMS];

	u8 gap_x = 10, gap_y = 10;
	u16 start_x = 0, start_y = 0;
	// section size must be even, and size % 4 should be 2
	u16 x_section_size = 62, y_section_size = 62;
#if 0
	x_section_size = (x_section_size > ((ctx->img_width - start_x - gap_x * 2) / 3)) ?
				((ctx->img_width - start_x - gap_x * 2) / 3) : x_section_size;
	y_section_size = (y_section_size > ((ctx->img_height - start_y - gap_y * 2) / 3)) ?
				((ctx->img_height - start_y - gap_y * 2) / 3)  : y_section_size;
#endif
	ISP_WR_BITS(sts, reg_isp_gms_t, gms_enable, gms_enable, enable);
	ISP_WR_BITS(sts, reg_isp_gms_t, gms_enable, out_shiftbit, 0);
	ISP_WR_BITS(sts, reg_isp_gms_t, dmi_enable, dmi_enable, enable);
	ISP_WR_REG(sts, reg_isp_gms_t, gms_start_x, start_x);
	ISP_WR_REG(sts, reg_isp_gms_t, gms_start_y, start_y);
	ISP_WR_REG(sts, reg_isp_gms_t, gms_x_sizem1, x_section_size - 1);
	ISP_WR_REG(sts, reg_isp_gms_t, gms_y_sizem1, y_section_size - 1);
	ISP_WR_REG(sts, reg_isp_gms_t, gms_x_gap, gap_x);
	ISP_WR_REG(sts, reg_isp_gms_t, gms_y_gap, gap_y);
}

void ispblk_lmap_chg_size(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, const enum sop_isp_fe_chn_num chn_num)
{
}

void ispblk_lmap_config(struct isp_ctx *ctx, int map_id, bool en)
{
	uintptr_t map = ctx->phys_regs[map_id];
	union reg_isp_lmap_lmp_0 reg0;

	reg0.raw = ISP_RD_REG(map, reg_isp_lmap_t, lmp_0);
	reg0.bits.lmap_enable = en;
	reg0.bits.lmap_y_mode = 0;
	ISP_WR_REG(map, reg_isp_lmap_t, lmp_0, reg0.raw);
}

void ispblk_rgbcac_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, u8 test_case)
{
	uintptr_t rgbcac = ctx->phys_regs[blk_id];

	ISP_WR_BITS(rgbcac, reg_isp_rgbcac_t, rgbcac_ctrl, rgbcac_enable, en);

	if (test_case == 1) {
		ISP_WR_BITS(rgbcac, reg_isp_rgbcac_t, rgbcac_purple_th, rgbcac_purple_th_le, 0xff);
		ISP_WR_BITS(rgbcac, reg_isp_rgbcac_t, rgbcac_purple_th, rgbcac_correct_strength_le, 0xff);
	}
}

void ispblk_lcac_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, u8 test_case)
{
	uintptr_t lcac = ctx->phys_regs[blk_id];

	ISP_WR_BITS(lcac, reg_isp_lcac_t, reg00, lcac_enable, en);

	if (test_case == 1) {
		union reg_isp_lcac_reg08 reg8;
		union reg_isp_lcac_reg0c regc;

		reg8.raw = ISP_RD_REG(lcac, reg_isp_lcac_t, reg08);
		reg8.bits.lcac_lti_str_r2_le	= 64;
		reg8.bits.lcac_lti_str_b2_le	= 64;
		reg8.bits.lcac_lti_wgt_r_le	= 0;
		reg8.bits.lcac_lti_wgt_b_le	= 0;
		ISP_WR_REG(lcac, reg_isp_lcac_t, reg08, reg8.raw);

		regc.raw = ISP_RD_REG(lcac, reg_isp_lcac_t, reg0c);
		regc.bits.lcac_lti_str_r2_se	= 64;
		regc.bits.lcac_lti_str_b2_se	= 64;
		regc.bits.lcac_lti_wgt_r_se	= 0;
		regc.bits.lcac_lti_wgt_b_se	= 0;
		ISP_WR_REG(lcac, reg_isp_lcac_t, reg0c, regc.raw);
	}
}


#include <vip/vi_drv.h>

/*******************************************************************************
 *	FE IPs config
 ******************************************************************************/
static void _patgen_config_timing(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	u16 pat_height = (ctx->is_hdr_on) ?
				(ctx->isp_pipe_cfg[raw_num].csibdg_height * 2 - 1) :
				(ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_v_size, vmde_str, 0x00);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_v_size, vmde_stp, pat_height);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_h_size, hmde_str, 0x00);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_h_size, hmde_stp,
							ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_v_size, vfde_str, 0x10);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_v_size, vfde_stp, 0x10 + pat_height);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_h_size, hfde_str, 0x10);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_h_size, hfde_stp,
							0x10 + ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_hsync_ctrl, hs_str, 4);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_hsync_ctrl, hs_stp, 5);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_vsync_ctrl, vs_str, 4);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_vsync_ctrl, vs_stp, 5);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, vtt, 0x17FF);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, htt, 0x17FF);
	} else {
		if (raw_num == ISP_PRERAW0) { //raw_num0 4k 20fps
			ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, vtt, 0x15FF);
			ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, htt, 0x1FFF);
		} else { //raw_num3 4k 20fps
			ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, vtt, 0x14FF);
			ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, htt, 0x15FF);
		}
	}
}

static void _patgen_config_pat(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, gra_inv, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, auto_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, dith_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, snow_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, fix_mc, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, dith_md, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, bayer_id, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_prd, 0);
	if (raw_num == ISP_PRERAW0)
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 0x7);
	else
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, raw_num);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_r, 0xfff);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_g, 0xfff);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_1, pat_b, 0xfff);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_r, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_g, 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_1, fde_b, 2);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_r, 0x457);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_g, 0x8ae);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_1, mde_b, 0xd05);
}
#ifdef PORTING_TEST
void ispblk_patgen_config_pat(struct isp_ctx *ctx, enum sop_isp_raw raw_num, u8 test_case)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, gra_inv, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, auto_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, dith_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, snow_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, fix_mc, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, dith_md, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, bayer_id, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_prd, 0);

	if (raw_num == ISP_PRERAW0)
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 0x7);
	else
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 1);

	if (test_case == 0) { //white
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 0x0);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_r, 0xfff);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_g, 0xfff);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_1, pat_b, 0xfff);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_r, 0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_g, 1);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_1, fde_b, 2);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_r, 0x457);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_g, 0x8ae);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_1, mde_b, 0xd05);
	} else if (test_case == 1) { //black
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 0x0);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_r, 0x0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_g, 0x0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_1, pat_b, 0x0);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_r, 0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_g, 0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_1, fde_b, 0);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_r, 0x0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_g, 0x0);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_1, mde_b, 0x0);
	} else if (test_case == 3) { // to test ca lite
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 0xf);
	}
}
#endif

void ispblk_csidbg_dma_wr_en(
	struct isp_ctx *ctx,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num,
	const u8 en)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	switch (chn_num) {
	case ISP_FE_CH0:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_wr_enable, en);
		break;
	case ISP_FE_CH1:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch1_dma_wr_enable, en);
		break;
	case ISP_FE_CH2:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch2_dma_wr_enable, en);
		break;
	case ISP_FE_CH3:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch3_dma_wr_enable, en);
		break;
	default:
		break;
	}
}

void ispblk_csibdg_wdma_crop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, struct vi_rect crop, u8 en)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_crop_en, st_ch0_crop_en, en);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_horz_crop, st_ch0_horz_crop_start, crop.x);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_horz_crop, st_ch0_horz_crop_end, crop.x + crop.w - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_vert_crop, st_ch0_vert_crop_start, crop.y);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_vert_crop, st_ch0_vert_crop_end, crop.y + crop.h - 1);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_crop_en, st_ch1_crop_en, en);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_horz_crop, st_ch1_horz_crop_start, crop.x);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_horz_crop, st_ch1_horz_crop_end, crop.x + crop.w - 1);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_vert_crop, st_ch1_vert_crop_start, crop.y);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_vert_crop, st_ch1_vert_crop_end, crop.y + crop.h - 1);
	}
}

void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool en)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	struct vi_rect crop, crop_se;

	crop.x = (ctx->isp_pipe_cfg[raw_num].crop.x == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop.x;
	crop.y = (ctx->isp_pipe_cfg[raw_num].crop.y == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop.y;
	crop.w = (ctx->isp_pipe_cfg[raw_num].crop.x + ctx->isp_pipe_cfg[raw_num].crop.w) - 1;
	crop.h = (ctx->isp_pipe_cfg[raw_num].crop.y + ctx->isp_pipe_cfg[raw_num].crop.h) - 1;

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_crop_en, ch0_crop_en, en);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_horz_crop, ch0_horz_crop_start, crop.x);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_horz_crop, ch0_horz_crop_end, crop.w);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_vert_crop, ch0_vert_crop_start, crop.y);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_vert_crop, ch0_vert_crop_end, crop.h);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		crop_se.x = (ctx->isp_pipe_cfg[raw_num].crop_se.x == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop_se.x;
		crop_se.y = (ctx->isp_pipe_cfg[raw_num].crop_se.y == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop_se.y;
		crop_se.w = (ctx->isp_pipe_cfg[raw_num].crop_se.x + ctx->isp_pipe_cfg[raw_num].crop_se.w) - 1;
		crop_se.h = (ctx->isp_pipe_cfg[raw_num].crop_se.y + ctx->isp_pipe_cfg[raw_num].crop_se.h) - 1;

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_crop_en, ch1_crop_en, en);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_horz_crop, ch1_horz_crop_start, crop.x);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_horz_crop, ch1_horz_crop_end, crop.w);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_vert_crop, ch1_vert_crop_start, crop.y);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_vert_crop, ch1_vert_crop_end, crop.h);
	}
}

int ispblk_csibdg_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	u8 csi_mode = 0;
	union reg_isp_csi_bdg_top_ctrl top_ctrl;
	union reg_isp_csi_bdg_interrupt_ctrl int_ctrl;

	top_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
	top_ctrl.bits.reset_mode	= 0;
	top_ctrl.bits.abort_mode	= 0;
	top_ctrl.bits.csi_in_format	= 0;
	top_ctrl.bits.ch_num		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;

	if (_is_be_post_online(ctx)) { //fe->dram->be->post
		top_ctrl.bits.ch0_dma_wr_enable = 1;
		top_ctrl.bits.ch1_dma_wr_enable = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	} else {
		top_ctrl.bits.ch0_dma_wr_enable = 0;
		top_ctrl.bits.ch1_dma_wr_enable = 0;
	}
	// ToDo stagger sensor
	//ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch2_dma_wr_enable, ctx->is_offline_be);
	//ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch3_dma_wr_enable, ctx->is_offline_be);

	if (ctx->isp_pipe_cfg[raw_num].is_patgen_en) {
		csi_mode = 3;
		top_ctrl.bits.pxl_data_sel = 1;
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, pat_en, 1);

		_patgen_config_timing(ctx, raw_num);
		_patgen_config_pat(ctx, raw_num);
	} else {
		top_ctrl.bits.pxl_data_sel = 0;

		csi_mode = (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) ? 2 : 1;
	}

	top_ctrl.bits.csi_mode	= csi_mode;
	if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
		top_ctrl.bits.vs_mode	= 0;
	else
		top_ctrl.bits.vs_mode	= ctx->isp_pipe_cfg[raw_num].is_stagger_vsync;
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, top_ctrl.raw);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	if (ctx->is_dpcm_on) {
		union reg_isp_csi_bdg_dma_dpcm_mode dpcm;

		dpcm.bits.dma_st_dpcm_mode = 0x7; //12->6 mode
		dpcm.bits.dpcm_xstr = 8191;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_dma_dpcm_mode, dpcm.raw);
	} else {
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_dma_dpcm_mode, 0);
	}

	int_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_interrupt_ctrl);
	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		int_ctrl.bits.ch0_vs_int_en		= 0;
		int_ctrl.bits.ch1_vs_int_en		= 0;
		int_ctrl.bits.ch2_vs_int_en		= 0;
		int_ctrl.bits.ch3_vs_int_en		= 0;
		int_ctrl.bits.ch0_trig_int_en		= 1;
		int_ctrl.bits.ch1_trig_int_en		= 1;
		int_ctrl.bits.ch2_trig_int_en		= 1;
		int_ctrl.bits.ch3_trig_int_en		= 1;
	} else {
		int_ctrl.bits.ch0_vs_int_en		= 1;
		int_ctrl.bits.ch1_vs_int_en		= 1;
		int_ctrl.bits.ch2_vs_int_en		= 1;
		int_ctrl.bits.ch3_vs_int_en		= 1;
		int_ctrl.bits.ch0_trig_int_en		= 0;
		int_ctrl.bits.ch1_trig_int_en		= 0;
		int_ctrl.bits.ch2_trig_int_en		= 0;
		int_ctrl.bits.ch3_trig_int_en		= 0;
	}
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_interrupt_ctrl, int_ctrl.raw);

	return 0;
}

void ispblk_csibdg_lite_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	union reg_isp_csi_bdg_lite_bdg_top_ctrl csibdg_ctrl;
	union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_1 csibdg_intr_ctrl_1;
	u8 chn_num = ctx->isp_pipe_cfg[raw_num].mux_mode;

	vi_pr(VI_DBG, "csibdg_lite[%d], chn_num[%d], inf_mode[%d]\n",
	      raw_num, chn_num, ctx->isp_pipe_cfg[raw_num].inf_mode);

	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, interrupt_status_0, 0xFFFFFFFF);
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, interrupt_status_1, 0xFFFFFFFF);

	csibdg_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl);
	csibdg_ctrl.bits.csi_mode		= 0;
	csibdg_ctrl.bits.ch_num			= chn_num;
	csibdg_ctrl.bits.y_only			= 0;
	csibdg_ctrl.bits.reset_mode		= 0;
	csibdg_ctrl.bits.abort_mode		= 0;
	csibdg_ctrl.bits.ch0_dma_wr_enable	= true;
	csibdg_ctrl.bits.ch1_dma_wr_enable	= (chn_num > 0) ? true : false;
	csibdg_ctrl.bits.ch2_dma_wr_enable	= (chn_num > 1) ? true : false;
	csibdg_ctrl.bits.ch3_dma_wr_enable	= (chn_num > 2) ? true : false;
	if (ctx->isp_pipe_cfg[raw_num].is_422_to_420) {
		csibdg_ctrl.bits.ch0_dma_420_wr_enable	= true;
		csibdg_ctrl.bits.ch1_dma_420_wr_enable	= (chn_num > 0) ? true : false;
	} else {
		csibdg_ctrl.bits.ch0_dma_420_wr_enable	= 0;
		csibdg_ctrl.bits.ch1_dma_420_wr_enable	= 0;
	}
	csibdg_ctrl.bits.ch2_dma_420_wr_enable	= 0;
	csibdg_ctrl.bits.ch3_dma_420_wr_enable	= 0;

	csibdg_intr_ctrl_1.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_interrupt_ctrl_1);
	csibdg_intr_ctrl_1.bits.line_intp_en		= 0;
	csibdg_intr_ctrl_1.bits.fifo_overflow_int_en	= 1;
	csibdg_intr_ctrl_1.bits.dma_error_intp_en	= 1;
	csibdg_intr_ctrl_1.bits.drop_mode		= 0;
	csibdg_intr_ctrl_1.bits.avg_mode		= (ctx->isp_pipe_cfg[raw_num].is_422_to_420) ? true : false;

	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl, csibdg_ctrl.raw);
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_interrupt_ctrl_0, 0xFFFFFFFF);
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_interrupt_ctrl_1, csibdg_intr_ctrl_1.raw);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch0_size, ch0_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch0_size, ch0_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch1_size, ch1_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch1_size, ch1_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch2_size, ch2_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch2_size, ch2_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch3_size, ch3_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch3_size, ch3_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);
}

void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	union reg_isp_csi_bdg_top_ctrl csibdg_ctrl;
	u8 chn_num = ctx->isp_pipe_cfg[raw_num].mux_mode;

	/*
	 * MIPI--->MUX use FE0 or FE1
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 * BT----->MUX use csibdg lite
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 */
	vi_pr(VI_DBG, "csibdg_yuv[%d], chn_num[%d], inf_mode[%d]\n",
	      raw_num, chn_num, ctx->isp_pipe_cfg[raw_num].inf_mode);

	if (chn_num == 0 || (ctx->isp_pipe_cfg[raw_num].inf_mode >= VI_MODE_MIPI_YUV420_NORMAL &&
	    ctx->isp_pipe_cfg[raw_num].inf_mode <= VI_MODE_MIPI_YUV422)) {
		csibdg_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
		csibdg_ctrl.bits.reset_mode		= 0;
		csibdg_ctrl.bits.abort_mode		= 0;
		csibdg_ctrl.bits.csi_mode		= 1;
		csibdg_ctrl.bits.csi_in_format		= 1;
		csibdg_ctrl.bits.csi_in_yuv_format	= 0;
		csibdg_ctrl.bits.y_only			= 0;
		csibdg_ctrl.bits.yuv2bay_enable		= (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) ?
								ctx->is_3dnr_on : 0;
		csibdg_ctrl.bits.ch_num			= chn_num;
		csibdg_ctrl.bits.multi_ch_frame_sync_en	= 0;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, csibdg_ctrl.raw);

	} else if (chn_num > 0 && ctx->isp_pipe_cfg[raw_num].inf_mode >= VI_MODE_BT656 &&
		ctx->isp_pipe_cfg[raw_num].inf_mode <= VI_MODE_BT1120_INTERLEAVED) {
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0_LITE];
	}

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, vs_mode,
			ctx->isp_pipe_cfg[raw_num].is_stagger_vsync);

	if (!_is_all_online(ctx)) // sensor->fe->dram
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_wr_enable, true);
	else // sensor->fe->yuvtop
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_wr_enable, false);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch1_dma_wr_enable, (chn_num > 0) ? true : false);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch2_dma_wr_enable, (chn_num > 1) ? true : false);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch3_dma_wr_enable, (chn_num > 2) ? true : false);

	if (ctx->isp_pipe_cfg[raw_num].is_422_to_420) {
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_420_wr_enable, true);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch1_dma_420_wr_enable,
									(chn_num > 0) ? true : false);
		//only chn0 support avg mode
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_dma_dpcm_mode, avg_mode, true);
	}

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_widthm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_heightm1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);
}

void ispblk_rgbmap_dma_mode(struct isp_ctx *ctx, u32 dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	//1: SW mode: config by SW 0: HW mode: auto config by HW
	sys_ctrl.raw = ISP_RD_REG(dmab, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= 0x1;
	sys_ctrl.bits.seglen_sel	= 0x1;
	sys_ctrl.bits.segnum_sel	= 0x1;
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);
}

int ispblk_rgbmap_config(struct isp_ctx *ctx, int map_id, bool en)
{
	int id = rgbmap_find_hwid(map_id);
	uintptr_t map;

	if (id < 0)
		return -EINVAL;

	map = ctx->phys_regs[id];
	ISP_WR_BITS(map, reg_isp_rgbmap_t, rgbmap_0, rgbmap_enable, en);

	return 0;
}

void ispblk_tnr_rgbmap_chg(struct isp_ctx *ctx, enum sop_isp_raw raw_num, const u8 chn_num)
{
	uintptr_t preraw_fe;
	u32 fe_id, dma_id;

	if (g_rgbmap_chg_pre[raw_num][chn_num] == false)
		return;

	fe_id = fe_find_hwid(raw_num);
	dma_id = rgbmap_dma_find_hwid(raw_num,
			(chn_num == ISP_FE_CH0) ? ISP_RAW_PATH_LE : ISP_RAW_PATH_SE);
	preraw_fe = ctx->phys_regs[fe_id];

	if (chn_num == ISP_FE_CH0) {
		ISP_WR_BITS(preraw_fe, reg_pre_raw_fe_t, le_rgbmap_grid_number,
					le_rgbmp_h_grid_size, g_w_bit[raw_num]);
		ISP_WR_BITS(preraw_fe, reg_pre_raw_fe_t, le_rgbmap_grid_number,
					le_rgbmp_v_grid_size, g_h_bit[raw_num]);

		ispblk_rgbmap_dma_config(ctx, raw_num, dma_id);
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			dma_id = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_LE);
			ispblk_rgbmap_dma_config(ctx, raw_num + 1, dma_id);
		}
	} else {
		ISP_WR_BITS(preraw_fe, reg_pre_raw_fe_t, se_rgbmap_grid_number,
					se_rgbmp_h_grid_size, g_w_bit[raw_num]);
		ISP_WR_BITS(preraw_fe, reg_pre_raw_fe_t, se_rgbmap_grid_number,
					se_rgbmp_v_grid_size, g_h_bit[raw_num]);

		ispblk_rgbmap_dma_config(ctx, raw_num, dma_id);
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			dma_id = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_SE);
			ispblk_rgbmap_dma_config(ctx, raw_num + 1, dma_id);
		}
	}

	g_rgbmap_chg_pre[raw_num][chn_num] = false;
}

struct isp_grid_s_info ispblk_rgbmap_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	int id = fe_find_hwid(raw_num);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	struct isp_grid_s_info ret;

	ret.w_bit = ISP_RD_BITS(preraw_fe, reg_pre_raw_fe_t, le_rgbmap_grid_number, le_rgbmp_h_grid_size);
	ret.h_bit = ISP_RD_BITS(preraw_fe, reg_pre_raw_fe_t, le_rgbmap_grid_number, le_rgbmp_v_grid_size);

	return ret;
}

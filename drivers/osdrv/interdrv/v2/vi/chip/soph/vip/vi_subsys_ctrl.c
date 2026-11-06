#include <vip/vi_drv.h>

/*******************************************************************************
 *	Subsys config
 ******************************************************************************/
void ispblk_splt_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool enable)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_SPLT];
	union reg_isp_line_spliter_sel_ctrl sel_ctrl;
	union reg_isp_line_spliter_dma_ctrl dma_ctrl;
	u32 width = ctx->isp_pipe_cfg[raw_num].max_width;
	u32 height = ctx->isp_pipe_cfg[raw_num].max_height;
	u32 width_l, width_r;

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, enable, line_spliter_enable, enable);

	if (!enable) {
		return;
	}

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_fe0, img_in_w_m1, width - 1);
	ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_fe0, img_in_h_m1, height - 1);

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		width_l = ctx->isp_pipe_cfg[raw_num].csibdg_width;
		width_r = width - width_l;
		// left
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_width_bld, bld_w_str, 0);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_width_bld, bld_w_end, width_l - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_bld, img_width_b, width_l - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_bld, img_height_b, height - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe0, frame_widthm1_fe0, width_l - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe0, frame_heightm1_fe0, height - 1);
		// right
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_fe1, img_width_fe1, width_r - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_fe1, img_height_fe1, height - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_width_nbld, nbld_w_str, width_l);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_width_nbld, nbld_w_end, width - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_nbld, img_width_nb, width_r - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_nbld, img_height_nb, height - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe1, frame_widthm1_fe1, width_r - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe1, frame_heightm1_fe1, height - 1);
	} else {
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_width_bld, bld_w_str, 0);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_width_bld, bld_w_end, width - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_bld, img_width_b, width - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, img_size_bld, img_height_b, height - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe0, frame_widthm1_fe0, width - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe0, frame_heightm1_fe0, height - 1);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe1, frame_widthm1_fe1, 0);
		ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_size_fe1, frame_heightm1_fe1, 0);
	}

	sel_ctrl.raw = 0;
	dma_ctrl.raw = 0;
	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {
		sel_ctrl.bits.fe0_sel		 = 0;
		sel_ctrl.bits.fe1_sel		 = 0;
		sel_ctrl.bits.fe0_rdma_sel	 = 1;
		sel_ctrl.bits.fe1_rdma_sel	 = ctx->isp_pipe_cfg[raw_num].is_tile;

		dma_ctrl.bits.rdma_l_enable_fe0	 = 1;
		dma_ctrl.bits.rdma_s_enable_fe0	 = ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
						ctx->isp_pipe_cfg[raw_num].is_yuv_sensor;
		dma_ctrl.bits.rdma_l_enable_fe1	 = ctx->isp_pipe_cfg[raw_num].is_tile;
		dma_ctrl.bits.rdma_s_enable_fe1	 = ctx->isp_pipe_cfg[raw_num].is_tile &&
						(ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
						ctx->isp_pipe_cfg[raw_num].is_yuv_sensor);
	} else if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap) {
		sel_ctrl.bits.fe0_sel		 = 0;
		sel_ctrl.bits.fe1_sel		 = ctx->isp_pipe_cfg[raw_num].is_tile;
		sel_ctrl.bits.fe0_rdma_sel	 = 1;
		sel_ctrl.bits.fe1_rdma_sel	 = ctx->isp_pipe_cfg[raw_num].is_tile;

		dma_ctrl.bits.fe0_wdma_l_enable	 = 1;
		dma_ctrl.bits.fe0_wdma_s_enable	 = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		dma_ctrl.bits.fe1_wdma_l_enable	 = ctx->isp_pipe_cfg[raw_num].is_tile;
		dma_ctrl.bits.fe1_wdma_s_enable	 = ctx->isp_pipe_cfg[raw_num].is_tile &&
						   ctx->isp_pipe_cfg[raw_num].is_hdr_on;

		dma_ctrl.bits.rdma_l_enable_fe0	 = 1;
		dma_ctrl.bits.rdma_s_enable_fe0	 = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		dma_ctrl.bits.rdma_l_enable_fe1	 = ctx->isp_pipe_cfg[raw_num].is_tile;
		dma_ctrl.bits.rdma_s_enable_fe1	 = ctx->isp_pipe_cfg[raw_num].is_tile &&
						   ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	} else {
		sel_ctrl.bits.fe0_sel		 = 0;
		sel_ctrl.bits.fe1_sel		 = ctx->isp_pipe_cfg[raw_num].is_tile;
		sel_ctrl.bits.fe0_rdma_sel	 = 0;
		sel_ctrl.bits.fe1_rdma_sel	 = 0;
	}
	ISP_WR_REG(ba, reg_isp_line_spliter_t, sel_ctrl, sel_ctrl.raw);
	ISP_WR_REG(ba, reg_isp_line_spliter_t, dma_ctrl, dma_ctrl.raw);

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, yuv_mode, yuv_format_fe0,
			ctx->isp_pipe_cfg[raw_num].is_yuv_sensor);
	ISP_WR_BITS(ba, reg_isp_line_spliter_t, yuv_mode, yuv_format_fe1,
			ctx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, hdr_ctrl, hdr_fe0,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_BITS(ba, reg_isp_line_spliter_t, hdr_ctrl, hdr_fe1,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, lese_arbiter_ctrl, ch_num_fe0,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_BITS(ba, reg_isp_line_spliter_t, lese_arbiter_ctrl, ch_num_fe1,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, vs_sw_ctrl, fe0_vs_sel,
			ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe);
	ISP_WR_BITS(ba, reg_isp_line_spliter_t, vs_sw_ctrl, fe1_vs_sel,
			ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_vld_ctrl, vs_mode_fe0,
			ctx->isp_pipe_cfg[raw_num].is_stagger_vsync);
	ISP_WR_BITS(ba, reg_isp_line_spliter_t, frame_vld_ctrl, vs_mode_fe1, 0);
}

void ispblk_splt_wdma_ctrl_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool enable)
{
	uintptr_t wdma = ctx->phys_regs[blk_id];
	union reg_pre_wdma_ctrl wdma_ctrl;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	if (blk_id == ISP_BLK_ID_SPLT_FE1_WDMA)
		raw_num = ISP_PRERAW1;

	wdma_ctrl.raw = ISP_RD_REG(wdma, reg_pre_wdma_ctrl_t, pre_wdma_ctrl);
	wdma_ctrl.bits.wdmi_en_le = enable;
	wdma_ctrl.bits.wdmi_en_se = enable && ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	ISP_WR_REG(wdma, reg_pre_wdma_ctrl_t, pre_wdma_ctrl, wdma_ctrl.raw);

	// just ai_isp need write to dram, and always no used compress mode.
	ISP_WR_BITS(wdma, reg_pre_wdma_ctrl_t, pre_raw_be_rdmi_dpcm, dpcm_mode, 0);
	ISP_WR_BITS(wdma, reg_pre_wdma_ctrl_t, pre_raw_be_rdmi_dpcm, dpcm_xstr, 0);

	vi_pr(VI_DBG, "splt wdma enable = %d\n", enable);
}

void ispblk_splt_rdma_ctrl_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool enable)
{
	uintptr_t rdma = ctx->phys_regs[blk_id];
	union reg_raw_rdma_ctrl_config ctrl_config;
	union reg_raw_rdma_ctrl_rdma_size rdma_size;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u32 width, height;

	if ((blk_id == ISP_BLK_ID_SPLT_FE1_RDMA_LE) || (blk_id == ISP_BLK_ID_SPLT_FE1_RDMA_SE))
		raw_num = ISP_PRERAW1;

	width = ctx->isp_pipe_cfg[raw_num].csibdg_width;
	height = ctx->isp_pipe_cfg[raw_num].csibdg_height;

	ctrl_config.raw = 0;
	ctrl_config.bits.le_rdma_en = enable;
	// ctrl_config.bits.se_rdma_en = enable; // unused
	ISP_WR_REG(rdma, reg_raw_rdma_ctrl_t, config, ctrl_config.raw);

	rdma_size.raw = 0;
	rdma_size.bits.rdmi_widthm1 = width - 1;
	rdma_size.bits.rdmi_heightm1 = height - 1;
	ISP_WR_REG(rdma, reg_raw_rdma_ctrl_t, rdma_size, rdma_size.raw);

	if (ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_fe) {
		if (ctx->is_dpcm_on) {
			ISP_WR_BITS(rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_mode, 7);
			ISP_WR_BITS(rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_xstr, 8191);
		} else {
			ISP_WR_BITS(rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_mode, 0);
			ISP_WR_BITS(rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_xstr, 0);
		}
	} else if (ctx->isp_pipe_cfg[ISP_PRERAW0].raw_ai_isp_ap) {
		ISP_WR_BITS(rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_mode, 0);
		ISP_WR_BITS(rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_xstr, 0);
	}
}

void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	int id = fe_find_hwid(raw_num);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	u32 width = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 height = ctx->isp_pipe_cfg[raw_num].crop.h;
	union reg_pre_raw_fe_pre_raw_ctrl raw_ctrl;
	union reg_pre_raw_fe_pre_raw_frame_size  frm_size;
	union reg_pre_raw_fe_le_rgbmap_grid_number  rgbmap_le;
	union reg_pre_raw_fe_se_rgbmap_grid_number  rgbmap_se;

	frm_size.raw = rgbmap_le.raw = rgbmap_se.raw = 0;

	raw_ctrl.raw = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_ctrl);
	raw_ctrl.bits.bayer_type_le = ctx->rgb_color_mode[raw_num];
	raw_ctrl.bits.bayer_type_se = ctx->rgb_color_mode[raw_num];
	raw_ctrl.bits.post_blc_bayer_type_le = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	raw_ctrl.bits.post_blc_bayer_type_se = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	raw_ctrl.bits.rgbir_en = ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	ISP_WR_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_ctrl, raw_ctrl.raw);

	frm_size.bits.frame_widthm1 = width - 1;
	frm_size.bits.frame_heightm1 = height - 1;
	ISP_WR_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_frame_size, frm_size.raw);

	rgbmap_le.bits.le_rgbmp_h_grid_size = g_w_bit[raw_num];
	rgbmap_le.bits.le_rgbmp_v_grid_size = g_h_bit[raw_num];
	rgbmap_se.bits.se_rgbmp_h_grid_size = g_w_bit[raw_num];
	rgbmap_se.bits.se_rgbmp_v_grid_size = g_h_bit[raw_num];
#if 0 //only grid size need to program in lmap/rgbmap hw mode
	w_grid_num = UPPER(width, g_w_bit[raw_num]) - 1;
	h_grid_num = UPPER(height, g_h_bit[raw_num]) - 1;

	rgbmap_le.bits.le_rgbmp_h_grid_numm1 = w_grid_num;
	rgbmap_le.bits.le_rgbmp_v_grid_numm1 = h_grid_num;
	rgbmap_se.bits.se_rgbmp_h_grid_numm1 = w_grid_num;
	rgbmap_se.bits.se_rgbmp_v_grid_numm1 = h_grid_num;
#endif
	ISP_WR_REG(preraw_fe, reg_pre_raw_fe_t, le_rgbmap_grid_number, rgbmap_le.raw);
	ISP_WR_REG(preraw_fe, reg_pre_raw_fe_t, se_rgbmap_grid_number, rgbmap_se.raw);
}

void ispblk_preraw_vi_sel_config(struct isp_ctx *ctx)
{
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	union reg_pre_raw_vi_sel_1 vi_sel_1;

	vi_sel_1.raw = 0;
	vi_sel_1.bits.frame_widthm1 = ctx->img_width - 1;
	vi_sel_1.bits.frame_heightm1 = ctx->img_height - 1;
	ISP_WR_REG(vi_sel, reg_pre_raw_vi_sel_t, reg_1, vi_sel_1.raw);

	if (_is_be_post_online(ctx) && ctx->is_dpcm_on) { // dram->be
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 7);
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 8191);
	} else {
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 0);
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 0);
	}
}

void ispblk_pre_wdma_ctrl_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t pre_wdma = ctx->phys_regs[ISP_BLK_ID_PRE_WDMA];
	union reg_pre_wdma_ctrl wdma_ctrl;

	wdma_ctrl.raw = ISP_RD_REG(pre_wdma, reg_pre_wdma_ctrl_t, pre_wdma_ctrl);
	wdma_ctrl.bits.wdmi_en_le = ctx->is_offline_postraw;
	wdma_ctrl.bits.wdmi_en_se = (ctx->is_offline_postraw && ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_REG(pre_wdma, reg_pre_wdma_ctrl_t, pre_wdma_ctrl, wdma_ctrl.raw);

	// NOTE: for be->dram, 'pre_raw_be_rdmi_dpcm' naming is misleading
	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on) { // be->dram
			ISP_WR_BITS(pre_wdma, reg_pre_wdma_ctrl_t, pre_raw_be_rdmi_dpcm, dpcm_mode, 7);
			// 1 if dpcm_mode 7; 0 if dpcm_mode 5
			ISP_WR_BITS(pre_wdma, reg_pre_wdma_ctrl_t, pre_wdma_ctrl, dma_wr_msb, 1);
			ISP_WR_BITS(pre_wdma, reg_pre_wdma_ctrl_t, pre_raw_be_rdmi_dpcm, dpcm_xstr, 8191);
		} else {
			ISP_WR_BITS(pre_wdma, reg_pre_wdma_ctrl_t, pre_raw_be_rdmi_dpcm, dpcm_mode, 0);
			ISP_WR_BITS(pre_wdma, reg_pre_wdma_ctrl_t, pre_raw_be_rdmi_dpcm, dpcm_xstr, 0);
		}
	}
}

void ispblk_preraw_be_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	union reg_pre_raw_be_top_ctrl top_ctrl;
	union reg_pre_raw_be_img_size_le img_size;

	top_ctrl.raw = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, top_ctrl);
	top_ctrl.bits.bayer_type_le	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.bayer_type_se	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.rgbir_en		= ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	top_ctrl.bits.ch_num		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	top_ctrl.bits.post_rgbir_bayer_type_le	= bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	top_ctrl.bits.post_rgbir_bayer_type_se	= bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	ISP_WR_REG(preraw_be, reg_pre_raw_be_t, top_ctrl, top_ctrl.raw);

	img_size.raw = 0;
	img_size.bits.frame_widthm1 = ctx->img_width - 1;
	img_size.bits.frame_heightm1 = ctx->img_height - 1;
	ISP_WR_REG(preraw_be, reg_pre_raw_be_t, img_size_le, img_size.raw);

	ISP_WO_BITS(preraw_be, reg_pre_raw_be_t, up_pq_en, up_pq_en, 1);
	ISP_WR_BITS(preraw_be, reg_pre_raw_be_t, debug_enable, debug_en, 1);
}

void ispblk_raw_rdma_ctrl_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num,
					enum isp_blk_id_t blk_id, bool enable)
{
	uintptr_t raw_rdma = ctx->phys_regs[blk_id];
	union reg_raw_rdma_ctrl_config ctrl_config;
	union reg_raw_rdma_ctrl_rdma_size rdma_size;

	ctrl_config.raw = 0;
	ctrl_config.bits.le_rdma_en = enable;
	// ctrl_config.bits.se_rdma_en = enable; // unused
	ISP_WR_REG(raw_rdma, reg_raw_rdma_ctrl_t, config, ctrl_config.raw);

	rdma_size.raw = 0;
	rdma_size.bits.rdmi_widthm1 = ctx->img_width - 1;
	rdma_size.bits.rdmi_heightm1 = ctx->img_height - 1;
	ISP_WR_REG(raw_rdma, reg_raw_rdma_ctrl_t, rdma_size, rdma_size.raw);

	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on &&
			!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //dram->post
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_mode, 7);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_xstr, 8191);
		} else {
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_mode, 0);
			ISP_WR_BITS(raw_rdma, reg_raw_rdma_ctrl_t, dpcm_mode, dpcm_xstr, 0);
		}
	}
}

void ispblk_rawtop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	union reg_raw_top_raw_2 raw_2;
	union reg_raw_top_raw_bayer_type_topleft raw_bayer_type_topleft;
	union reg_raw_top_rdmi_enable rdmi_enable;
	union reg_raw_top_le_lmap_grid_number   le_lmap_size;
	union reg_raw_top_se_lmap_grid_number   se_lmap_size;

	raw_2.raw = 0;
	raw_2.bits.img_widthm1 = ctx->img_width - 1;
	raw_2.bits.img_heightm1 = ctx->img_height - 1;
	ISP_WR_REG(rawtop, reg_raw_top_t, raw_2, raw_2.raw);

	raw_bayer_type_topleft.raw = 0;
	raw_bayer_type_topleft.bits.bayer_type_topleft = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	raw_bayer_type_topleft.bits.rgbir_enable = ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	ISP_WR_REG(rawtop, reg_raw_top_t, raw_bayer_type_topleft, raw_bayer_type_topleft.raw);

	rdmi_enable.raw = ISP_RD_REG(rawtop, reg_raw_top_t, rdmi_enable);
	rdmi_enable.bits.ch_num		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;

	ISP_WR_REG(rawtop, reg_raw_top_t, rdmi_enable, rdmi_enable.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
		ISP_WO_BITS(rawtop, reg_raw_top_t, ctrl, ls_crop_dst_sel, 1);
		/*only action at raw_top output pattern at yuvdata in raw_top*/
		// ISP_WO_BITS(rawtop, reg_raw_top_t, raw_4, yuv_in_mode, 1);
	} else {
		ISP_WO_BITS(rawtop, reg_raw_top_t, ctrl, ls_crop_dst_sel, 0);
		ISP_WO_BITS(rawtop, reg_raw_top_t, raw_4, yuv_in_mode, 0);
	}
#if 0
	if (_is_fe_be_online(ctx)) { // fe->be->dram->post single sensor frame_base/slice buffer
		union REG_RAW_TOP_RDMA_SIZE rdma_size;

		ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, rdmi_en, 1);
		rdma_size.raw = 0;
		rdma_size.bits.rdmi_widthm1 = ctx->img_width - 1;
		rdma_size.bits.rdmi_heightm1 = ctx->img_height - 1;
		ISP_WR_REG(rawtop, reg_raw_top_t, rdma_size, rdma_size.raw);
	} else { //fe->dram->be->post (2/3 sensors) or fe->be->post (onthefly)
		ISP_WR_BITS(rawtop, reg_raw_top_t, rdmi_enable, rdmi_en, 0);
	}
#endif
	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on) { //dram->post
			ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_mode, 7);
			ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_xstr, 8191);
		} else {
			ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_mode, 0);
			ISP_WR_BITS(rawtop, reg_raw_top_t, dpcm_mode, dpcm_xstr, 0);
		}
	}

	le_lmap_size.raw = ISP_RD_REG(rawtop, reg_raw_top_t, le_lmap_grid_number);
	le_lmap_size.bits.le_lmp_h_grid_size = g_lmp_cfg[raw_num].post_w_bit;
	le_lmap_size.bits.le_lmp_v_grid_size = g_lmp_cfg[raw_num].post_h_bit;
	ISP_WR_REG(rawtop, reg_raw_top_t, le_lmap_grid_number, le_lmap_size.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		se_lmap_size.raw = ISP_RD_REG(rawtop, reg_raw_top_t, se_lmap_grid_number);
		se_lmap_size.bits.se_lmp_h_grid_size = g_lmp_cfg[raw_num].post_w_bit;
		se_lmap_size.bits.se_lmp_v_grid_size = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(rawtop, reg_raw_top_t, se_lmap_grid_number, se_lmap_size.raw);
	}
}

void ispblk_rgbtop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	union reg_isp_rgb_top_0 reg_0;
	union reg_isp_rgb_top_9 reg_9;

	reg_0.raw = reg_9.raw = 0;

	reg_0.raw = ISP_RD_REG(rgbtop, reg_isp_rgb_top_t, reg_0);
	reg_0.bits.rgbtop_bayer_type = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	reg_0.bits.rgbtop_rgbir_enable = ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	ISP_WR_REG(rgbtop, reg_isp_rgb_top_t, reg_0, reg_0.raw);

	reg_9.bits.rgbtop_imgw_m1 = ctx->img_width - 1;
	reg_9.bits.rgbtop_imgh_m1 = ctx->img_height - 1;
	ISP_WR_REG(rgbtop, reg_isp_rgb_top_t, reg_9, reg_9.raw);
	ISP_WR_BITS(rgbtop, reg_isp_rgb_top_t, dbg_ip_s_vld, ip_dbg_en, 1);

	// for debug, this set [patgen1, pg_enable] = [0x1]
}

void ispblk_yuvtop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	union reg_yuv_top_imgw_m1 imgw_m1;
	union reg_yuv_top_ai_isp_rdma_ctrl ai_isp_ctrl;
	u32 width = ctx->img_width;
	u32 height = ctx->img_height;

	imgw_m1.raw = 0;
	ai_isp_ctrl.raw = 0;

	imgw_m1.bits.yuv_top_imgw_m1 = width - 1;
	imgw_m1.bits.yuv_top_imgh_m1 = height - 1;
	ISP_WR_REG(yuvtop, reg_yuv_top_t, imgw_m1, imgw_m1.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp) {
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_img_size_y, ai_isp_img_width_crop_0, width - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_img_size_y, ai_isp_img_height_crop_0, height - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_w_crop_y, ai_isp_crop_w_str_0, 0);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_w_crop_y, ai_isp_crop_w_end_0, width - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_h_crop_y, ai_isp_crop_h_str_0, 0);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_h_crop_y, ai_isp_crop_h_end_0, height - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_img_size_uv, ai_isp_img_width_crop_1, (width >> 1) - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_img_size_uv, ai_isp_img_height_crop_1, height - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_w_crop_uv, ai_isp_crop_w_str_1, 0);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_w_crop_uv, ai_isp_crop_w_end_1, (width >> 1) - 1);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_h_crop_uv, ai_isp_crop_h_str_1, 0);
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, ai_isp_h_crop_uv, ai_isp_crop_h_end_1, height - 1);

		ai_isp_ctrl.bits.ai_isp_rdma_enable = ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp_rdy ? 0x7 : 0x0;
		ai_isp_ctrl.bits.ai_isp_crop_enable = false;
		ai_isp_ctrl.bits.ai_isp_enable      = true;
		ai_isp_ctrl.bits.ai_isp_mask        = true;
		ISP_WR_REG(yuvtop, reg_yuv_top_t, ai_isp_rdma_ctrl, ai_isp_ctrl.raw);
	} else {
		// to do
	}

	if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		//bypass_v = 1 -> 422P online to scaler
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, yuv_ctrl, bypass_v, 1);
	} else {
		ISP_WR_BITS(yuvtop, reg_yuv_top_t, yuv_ctrl, bypass_v,
					!ctx->isp_pipe_cfg[raw_num].is_offline_scaler);
	}
	// for debug, this set [patgen1, pg_enable] = [0x1]
}

void ispblk_isptop_config(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	u8 pre_fe_trig_by_hw[ISP_PRERAW_MAX] = { 0 };
	u8 pre_be_trig_by_hw = 0;
	u8 post_trig_by_hw = 0;
	u8 first_raw_num = vi_get_first_raw_num(ctx);
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	union reg_isp_top_int_event0_en ev0_en;
	union reg_isp_top_int_event1_en ev1_en;
	union reg_isp_top_int_event2_en ev2_en;
	union reg_isp_top_int_event0_en_fe345 ev0_en_fe345;
	union reg_isp_top_int_event1_en_fe345 ev1_en_fe345;
	union reg_isp_top_int_event2_en_fe345 ev2_en_fe345;
	union reg_isp_top_ctrl_mode_sel0 trig_sel0;
	union reg_isp_top_ctrl_mode_sel1 trig_sel1;
	union reg_isp_top_ctrl_mode_sel0_fe345 trig_sel0_fe345;
	union reg_isp_top_ctrl_mode_sel1_fe345 trig_sel1_fe345;
	union reg_isp_top_scenarios_ctrl scene_ctrl;

	ev0_en.raw = ev1_en.raw = ev2_en.raw = 0;
	ev0_en_fe345.raw = ev1_en_fe345.raw = ev2_en_fe345.raw = 0;
	trig_sel0.raw = trig_sel1.raw = 0;
	trig_sel0_fe345.raw = trig_sel1_fe345.raw = 0;
	scene_ctrl.raw = 0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) { //RAW replay
			pre_fe_trig_by_hw[raw_num] = 0x0;
		} else if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
			if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				pre_fe_trig_by_hw[raw_num] = 0x1;
			else
				pre_fe_trig_by_hw[raw_num] = 0x3;
		} else { //YUV sensor
			switch (ctx->isp_pipe_cfg[raw_num].mux_mode) {
			case VI_WORK_MODE_1MULTIPLEX:
				pre_fe_trig_by_hw[raw_num] = 0x1;
				break;
			case VI_WORK_MODE_2MULTIPLEX:
				pre_fe_trig_by_hw[raw_num] = 0x3;
				break;
			case VI_WORK_MODE_3MULTIPLEX:
				pre_fe_trig_by_hw[raw_num] = 0x7;
				break;
			case VI_WORK_MODE_4MULTIPLEX:
				pre_fe_trig_by_hw[raw_num] = 0xF;
				break;
			default:
				break;
			}
		}
	}

	if (ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be)
		pre_be_trig_by_hw = 0x0;
	else if (ctx->is_offline_be)
		pre_be_trig_by_hw = 0x0;
	else { //be online, on the fly mode or fe_A->be
		if (ctx->isp_pipe_cfg[first_raw_num].is_yuv_sensor)
			pre_be_trig_by_hw = 0x0;
		else { //Single RGB sensor
			if (ctx->is_hdr_on)
				pre_be_trig_by_hw = 0x3;
			else
				pre_be_trig_by_hw = 0x1;
		}
	}

	// fly mode or single sensor and slice buffer mode on. post trigger by vsync
	if ((ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be == 0) &&
		(_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)))
		post_trig_by_hw = 0x1;
	else //trigger by SW
		post_trig_by_hw = 0x0;

	//line_spliter intr ctrl, splt_ai_isp enable, otherwise disable
	if (ctx->isp_pipe_cfg[first_raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT ||
	    ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_fe) {
		ev0_en_fe345.bits.line_spliter_dma_done_enable_fe0	= 0xF;
		ev0_en_fe345.bits.line_spliter_dma_done_enable_fe1	= 0xF;
		ev2_en.bits.frame_err_line_spliter_enable_fe0		= 0x1;
		ev2_en.bits.frame_err_line_spliter_enable_fe1		= 0x1;
	} else {
		ev0_en_fe345.bits.line_spliter_dma_done_enable_fe0	= 0;
		ev0_en_fe345.bits.line_spliter_dma_done_enable_fe1	= 0;
		ev2_en.bits.frame_err_line_spliter_enable_fe0		= 0;
		ev2_en.bits.frame_err_line_spliter_enable_fe1		= 0;
	}

	//pre_fe0
	ev0_en.bits.frame_done_enable_fe0	= 0xF;
	ev2_en.bits.frame_start_enable_fe0	= 0xF;
	trig_sel0.bits.trig_str_sel_fe0		= pre_fe_trig_by_hw[ISP_PRERAW0];
	trig_sel0.bits.shaw_up_sel_fe0		= pre_fe_trig_by_hw[ISP_PRERAW0];
	trig_sel1.bits.pq_up_sel_fe0		= pre_fe_trig_by_hw[ISP_PRERAW0];

	//pre_fe1
	ev0_en.bits.frame_done_enable_fe1	= 0xF;
	ev2_en.bits.frame_start_enable_fe1	= 0xF;
	trig_sel0.bits.trig_str_sel_fe1		= pre_fe_trig_by_hw[ISP_PRERAW1];
	trig_sel0.bits.shaw_up_sel_fe1		= pre_fe_trig_by_hw[ISP_PRERAW1];
	trig_sel1.bits.pq_up_sel_fe1		= pre_fe_trig_by_hw[ISP_PRERAW1];

	//pre_fe2
	ev0_en.bits.frame_done_enable_fe2	= 0x3;
	ev2_en.bits.frame_start_enable_fe2	= 0x3;
	trig_sel0.bits.trig_str_sel_fe2		= pre_fe_trig_by_hw[ISP_PRERAW2];
	trig_sel0.bits.shaw_up_sel_fe2		= pre_fe_trig_by_hw[ISP_PRERAW2];
	trig_sel1.bits.pq_up_sel_fe2		= pre_fe_trig_by_hw[ISP_PRERAW2];

	//pre_fe3
	ev0_en_fe345.bits.frame_done_enable_fe3	 = 0x3;
	ev2_en_fe345.bits.frame_start_enable_fe3 = 0x3;
	trig_sel0_fe345.bits.trig_str_sel_fe3	 = pre_fe_trig_by_hw[ISP_PRERAW3];
	trig_sel0_fe345.bits.shaw_up_sel_fe3	 = pre_fe_trig_by_hw[ISP_PRERAW3];
	trig_sel1_fe345.bits.pq_up_sel_fe3	 = pre_fe_trig_by_hw[ISP_PRERAW3];

	//pre_fe4
	ev0_en_fe345.bits.frame_done_enable_fe4	 = 0x3;
	ev2_en_fe345.bits.frame_start_enable_fe4 = 0x3;
	trig_sel0_fe345.bits.trig_str_sel_fe4	 = pre_fe_trig_by_hw[ISP_PRERAW4];
	trig_sel0_fe345.bits.shaw_up_sel_fe4	 = pre_fe_trig_by_hw[ISP_PRERAW4];
	trig_sel1_fe345.bits.pq_up_sel_fe4	 = pre_fe_trig_by_hw[ISP_PRERAW4];

	//pre_fe5
	ev0_en_fe345.bits.frame_done_enable_fe5	 = 0x3;
	ev2_en_fe345.bits.frame_start_enable_fe5 = 0x3;
	trig_sel0_fe345.bits.trig_str_sel_fe5	 = pre_fe_trig_by_hw[ISP_PRERAW5];
	trig_sel0_fe345.bits.shaw_up_sel_fe5	 = pre_fe_trig_by_hw[ISP_PRERAW5];
	trig_sel1_fe345.bits.pq_up_sel_fe5	 = pre_fe_trig_by_hw[ISP_PRERAW5];

	//pre_be
	ev0_en.bits.frame_done_enable_be	= 0x3;
	trig_sel0.bits.trig_str_sel_be		= pre_be_trig_by_hw;
	trig_sel0.bits.shaw_up_sel_be		= pre_be_trig_by_hw;
	trig_sel1.bits.pq_up_sel_be		= pre_be_trig_by_hw;

	//postraw
	ev0_en.bits.frame_done_enable_post	= 0x1;
	ev0_en.bits.shaw_done_enable_post	= 0x1;
	trig_sel0.bits.trig_str_sel_raw		= post_trig_by_hw;
	trig_sel0.bits.shaw_up_sel_raw		= post_trig_by_hw;
	trig_sel1.bits.pq_up_sel_raw		= post_trig_by_hw;
	trig_sel0.bits.trig_str_sel_post	= post_trig_by_hw;
	trig_sel0.bits.shaw_up_sel_post		= post_trig_by_hw;
	trig_sel1.bits.pq_up_sel_post		= post_trig_by_hw;

	//csibdg_lite
	ev2_en.bits.int_bdg0_lite_enable	= 0x1;
	ev2_en.bits.int_bdg1_lite_enable	= 0x1;
	ev2_en.bits.cmdq_int_enable		= 0x1;

	//err int
	ev2_en.bits.frame_err_enable		= 0x1;
	ev2_en.bits.int_dma_err_enable		= 0x1;

	//scenario_ctrl mode
	//single sensor
	scene_ctrl.raw = ISP_RD_REG(isptopb, reg_isp_top_t, scenarios_ctrl);
	scene_ctrl.bits.pre2be_l_enable		= !ctx->is_offline_be;
	scene_ctrl.bits.pre2be_s_enable		= !ctx->is_offline_be && ctx->is_hdr_on;
	scene_ctrl.bits.pre2yuv_422_enable	= 0;
	//multi sensors
	scene_ctrl.bits.be2raw_l_enable		= !ctx->is_offline_postraw;
	scene_ctrl.bits.be2raw_s_enable		= !ctx->is_offline_postraw && ctx->is_hdr_on;
	// multi sensors or raw replay
	scene_ctrl.bits.be_rdma_l_enable	= ctx->is_offline_be ||
						  ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be;
	scene_ctrl.bits.be_rdma_s_enable	= (ctx->is_offline_be ||
						  ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be) &&
						  ctx->is_hdr_on;
	// onthefly -> 0 or single sensor
	scene_ctrl.bits.be_wdma_l_enable	= ctx->is_offline_postraw;
	scene_ctrl.bits.be_wdma_s_enable	= ctx->is_offline_postraw && ctx->is_hdr_on;
	scene_ctrl.bits.af_raw0yuv1		= 0; //0 from be/1 from post
	scene_ctrl.bits.rgbmp_online_l_enable	= _is_all_online(ctx); //onthefly -> 1
	//single sensor hdr and slice buffer mode on
	scene_ctrl.bits.rgbmp_online_s_enable	= 0;
	scene_ctrl.bits.raw2yuv_422_enable	= 0;

#if defined(__CV180X__)
	scene_ctrl.bits.HDR_ENABLE		= ctx->is_hdr_on;
#else
	//in order for linearmode use guideweight
	scene_ctrl.bits.hdr_enable		= !_is_all_online(ctx) && ctx->is_hdr_on;
#endif
	// to verify ip, turn off hw lut of rgbgamma, ynr, and cnr.
	scene_ctrl.bits.hw_auto_enable		= 0;
	// set the position of the beginning of YUV suggested by HW
	scene_ctrl.bits.dci_rgb0yuv1		= 0;
	scene_ctrl.bits.be_src_sel		= first_raw_num;
	scene_ctrl.bits.dst2sc			= !ctx->isp_pipe_cfg[first_raw_num].is_offline_scaler;

#if defined(__CV186X__)
	//SW PATCH: To succeed in the next run, these IPs need to be reset.
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x6FF);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x0);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst_fe345, 0x7);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst_fe345, 0x0);
#endif

	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0_fe345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1_fe345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_fe345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0_en, ev0_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1_en, ev1_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en, ev2_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0_en_fe345, ev0_en_fe345.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1_en_fe345, ev1_en_fe345.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en_fe345, ev2_en_fe345.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0, trig_sel0.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel1, trig_sel1.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0_fe345, trig_sel0_fe345.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel1_fe345, trig_sel1_fe345.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, scenarios_ctrl, scene_ctrl.raw);

	ISP_WR_BITS(isptopb, reg_isp_top_t, dummy, dbus_sel, 4);
	//ISP_WR_REG(isptopb, reg_isp_top_t, reg_1c, 7);
}

#ifdef PORTING_TEST

void ispblk_isptop_fpga_config(struct isp_ctx *ctx, u16 test_case)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_scenarios_ctrl scene_ctrl;

	scene_ctrl.raw = ISP_RD_REG(isptopb, reg_isp_top_t, scenarios_ctrl);
	// to verify IP, turn off HW LUT of rgbgamma, ynr, and cnr.
	if (test_case == 0) {
		scene_ctrl.bits.hw_auto_enable		= 0;
		scene_ctrl.bits.hw_auto_iso		= 0;
	} else if (test_case == 1) {
		scene_ctrl.bits.hw_auto_enable		= 1;
		scene_ctrl.bits.hw_auto_iso		= 0;
	} else if (test_case == 2) { //auto_iso
		scene_ctrl.bits.hw_auto_enable		= 1;
		scene_ctrl.bits.hw_auto_iso		= 2;
	} else if (test_case == 0xff) { //store default config
		scene_ctrl.bits.hw_auto_enable		= 1;
		scene_ctrl.bits.hw_auto_iso		= 0;
	}

	ISP_WR_REG(isptopb, reg_isp_top_t, scenarios_ctrl, scene_ctrl.raw);
}

#endif

void isp_intr_set_mask(struct isp_ctx *ctx)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	ISP_WR_REG(isp_top, reg_isp_top_t, int_event0_en, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event1_en, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event2_en, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event0_en_fe345, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event1_en_fe345, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event2_en_fe345, 0);
}

void isp_reset(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_ctrl_mode_sel0 mode_sel0;
	union reg_isp_top_ctrl_mode_sel0_fe345 mode_sel0_fe345;

	// disable interrupt
	isp_intr_set_mask(ctx);

	// switch back to hw trig.
	mode_sel0.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0);
	mode_sel0.bits.trig_str_sel_fe0  = 0xf;
	mode_sel0.bits.trig_str_sel_fe1  = 0xf;
	mode_sel0.bits.trig_str_sel_fe2  = 0x3;
	mode_sel0.bits.trig_str_sel_be   = 0x3;
	mode_sel0.bits.trig_str_sel_raw  = 0x1;
	mode_sel0.bits.trig_str_sel_post = 0x1;
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0, mode_sel0.raw);

	mode_sel0_fe345.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0_fe345);
	mode_sel0_fe345.bits.trig_str_sel_fe3 = 0x3;
	mode_sel0_fe345.bits.trig_str_sel_fe4 = 0x3;
	mode_sel0_fe345.bits.trig_str_sel_fe5 = 0x3;
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0_fe345, mode_sel0_fe345.raw);

	// reset
	//AXI_RST first, then reset other
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x80);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x6FF);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst_fe345, 0x7);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x80);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x0);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst_fe345, 0x0);

	// clear intr
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0_fe345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1_fe345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_fe345, 0xFFFFFFFF);
}

/*******************************************************************************
 *	Subsys Debug Infomation
 ******************************************************************************/
struct _csibdg_dbg_i ispblk_csibdg_chn_dbg(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	enum sop_isp_fe_chn_num chn_num)
{
	int id = -1;
	uintptr_t ba = 0;
	struct _csibdg_dbg_i data;

	if (ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
		id = csibdg_find_hwid(raw_num);
		ba = ctx->phys_regs[id];

		switch (chn_num) {
		case ISP_FE_CH0:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_3);
			break;
		case ISP_FE_CH1:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_3);
			break;
		case ISP_FE_CH2:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_3);
			break;
		case ISP_FE_CH3:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_3);
			break;
		default:
			break;
		}
	} else {
		id = csibdg_find_hwid(raw_num);
		ba = ctx->phys_regs[id];

		switch (chn_num) {
		case ISP_FE_CH0:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_3);
			break;
		case ISP_FE_CH1:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_3);
			break;
		case ISP_FE_CH2:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_3);
			break;
		case ISP_FE_CH3:
			data.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_0);
			data.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_1);
			data.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_2);
			data.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_3);
			break;
		default:
			break;
		}

	}

	return data;
}

struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	int id = fe_find_hwid(raw_num);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	struct _fe_dbg_i data;

	data.fe_idle_sts = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_debug_state);
	data.fe_done_sts = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, fe_idle_info);

	return data;
}

struct _be_dbg_i ispblk_be_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	struct _be_dbg_i data;

	data.be_done_sts = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, be_info);
	data.be_dma_idle_sts = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, be_dma_idle_info);

	return data;
}

struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	struct _post_dbg_i data;

	data.top_sts_0 = ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle);
	data.top_sts_1 = ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle_1);

	return data;
}

struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t wdma_com_2 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE2];
	uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
	uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];
	uintptr_t rdma_com_1 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE1];
	struct _dma_dbg_i data;

	data.wdma_0_err_sts = ISP_RD_REG(wdma_com_0, reg_wdma_core_t, norm_status0);
	data.wdma_0_idle = ISP_RD_REG(wdma_com_0, reg_wdma_core_t, norm_status1);

	data.wdma_1_err_sts = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, norm_status0);
	data.wdma_1_idle = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, norm_status1);

	data.wdma_2_err_sts = ISP_RD_REG(wdma_com_2, reg_wdma_core_t, norm_status0);
	data.wdma_2_idle = ISP_RD_REG(wdma_com_2, reg_wdma_core_t, norm_status1);

	data.wdma_3_err_sts = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, norm_status0);
	data.wdma_3_idle = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, norm_status1);

	data.rdma_0_err_sts = ISP_RD_REG(rdma_com_0, reg_rdma_core_t, norm_status0);
	data.rdma_0_idle = ISP_RD_REG(rdma_com_0, reg_rdma_core_t, norm_status1);

	data.rdma_1_err_sts = ISP_RD_REG(rdma_com_1, reg_rdma_core_t, norm_status0);
	data.rdma_1_idle = ISP_RD_REG(rdma_com_1, reg_rdma_core_t, norm_status1);
	return data;
}

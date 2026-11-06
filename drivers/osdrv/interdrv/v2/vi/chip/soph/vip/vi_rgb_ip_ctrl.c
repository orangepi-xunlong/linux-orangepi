#include <vip/vi_drv.h>
#include <ion.h>
#include <cmdq.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_MAX];

#if defined(__CV180X__)
#define LTM_DARK_TONE_LUT_SIZE   0x100
#define LTM_BRIGHT_TONE_LUT_SIZE 0x100
#define LTM_GLOBAL_LUT_SIZE      0x100
#else
#define LTM_DARK_TONE_LUT_SIZE   0x100
#define LTM_BRIGHT_TONE_LUT_SIZE 0x200
#define LTM_GLOBAL_LUT_SIZE      0x300
#endif

/*******************************************************************************
 *	RGB IPs config
 ******************************************************************************/

void ispblk_ccm_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, struct isp_ccm_cfg *cfg)
{
	uintptr_t ccm = ctx->phys_regs[blk_id];

	ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_shdw_sel, 1);
	ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_enable, en);

	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_22, cfg->coef[2][2]);
}

void ispblk_fusion_hdr_cfg(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_FUSION];

	if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(fusion, reg_fusion_t, fs_ctrl_0, fs_enable, false);
		ISP_WR_BITS(fusion, reg_fusion_t, fs_se_gain, fs_out_sel, ISP_FS_OUT_LONG);
	} else
		ISP_WR_BITS(fusion, reg_fusion_t, fs_ctrl_0, fs_enable, true);
}

void ispblk_fusion_config(struct isp_ctx *ctx, bool enable, bool mc_enable, enum isp_fs_out_e out_sel)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_FUSION];
	union reg_fusion_fs_ctrl_0 reg_ctrl;
	union reg_fusion_fs_se_gain reg_se_gain;

	reg_ctrl.raw = ISP_RD_REG(fusion, reg_fusion_t, fs_ctrl_0);
	reg_ctrl.bits.fs_enable = enable;
	reg_ctrl.bits.fs_mc_enable = mc_enable;
	reg_ctrl.bits.fs_s_max = 65535;
	ISP_WR_REG(fusion, reg_fusion_t, fs_ctrl_0, reg_ctrl.raw);

	reg_se_gain.raw = ISP_RD_REG(fusion, reg_fusion_t, fs_se_gain);
	reg_se_gain.bits.fs_ls_gain = 64;
	reg_se_gain.bits.fs_out_sel = out_sel;
	ISP_WR_REG(fusion, reg_fusion_t, fs_se_gain, reg_se_gain.raw);

	ISP_WR_BITS(fusion, reg_fusion_t, fs_luma_thd, fs_luma_thd_l, 2048);
	ISP_WR_BITS(fusion, reg_fusion_t, fs_luma_thd, fs_luma_thd_h, 2048);
	ISP_WR_BITS(fusion, reg_fusion_t, fs_wgt, fs_wgt_max, 128);
	ISP_WR_BITS(fusion, reg_fusion_t, fs_wgt, fs_wgt_min, 128);
	ISP_WR_REG(fusion, reg_fusion_t, fs_wgt_slope, 0);
}

void ispblk_hist_v_config(struct isp_ctx *ctx, bool en, u8 test_case)
{
	uintptr_t hist_v = ctx->phys_regs[ISP_BLK_ID_HIST_EDGE_V];

	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_enable, en);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_luma_mode, en);

	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, hist_edge_v_offsetx, hist_edge_v_offsetx, 0);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, hist_edge_v_offsety, hist_edge_v_offsety, 0);

	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, dmi_enable, dmi_enable, 1);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, dmi_enable, force_dma_disable, 0);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, bypass, force_clk_enable, 1);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, bypass, bypass, 0);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, sw_ctl, tile_nm, 0);

	// when test_case == 0
	//   en == 1, case_luma
	//   en == 0, case_disable
	if (test_case == 1) {
		/* case_0 and case_all_ff */
		ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_enable, 1);
		ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_luma_mode, 0);
	} else if (test_case == 2) {
		/* case_offx64_offy32 */
		ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_enable, 1);
		ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_luma_mode, 0);
		ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, hist_edge_v_offsetx, hist_edge_v_offsetx, 64);
		ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, hist_edge_v_offsety, hist_edge_v_offsety, 32);
	}
}

void ispblk_ltm_d_lut(struct isp_ctx *ctx, u8 sel, u16 *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_LTM];
	union reg_ltm_h34 reg_34;
	union reg_ltm_h3c reg_3c;
	u16 i = 0;
	u32 val = 0;

	ISP_WR_REG(ltm, reg_ltm_t, reg_h44, data[LTM_DARK_TONE_LUT_SIZE]);

	reg_34.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h34);
	//reg_34.bits.LUT_DBG_RSEL = sel;
	reg_34.bits.lut_prog_en_dark = 1;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h34, reg_34.raw);

	reg_3c.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h3c);
	reg_3c.bits.lut_wsel = sel;
	reg_3c.bits.lut_wstaddr = 0;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h3c, reg_3c.raw);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr_trig_1t, 1);
	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wdata_trig_1t, 1);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr, 0);
	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr_trig_1t, 1);

	for (i = 0; i < LTM_DARK_TONE_LUT_SIZE; i += 2) {
		val = (data[i] | (data[i + 1] << 16));
		ISP_WR_REG(ltm, reg_ltm_t, reg_h38, val);
		ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wdata_trig_1t, 1);
	}

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h34, lut_prog_en_dark, 0);
}

void ispblk_ltm_b_lut(struct isp_ctx *ctx, u8 sel, u16 *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_LTM];
	union reg_ltm_h34 reg_34;
	union reg_ltm_h3c reg_3c;
	u16 i = 0;
	u32 val = 0;

	ISP_WR_REG(ltm, reg_ltm_t, reg_h40, data[LTM_BRIGHT_TONE_LUT_SIZE]);

	reg_34.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h34);
	//reg_34.bits.lut_dbg_rsel = sel;
	reg_34.bits.lut_prog_en_bright = 1;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h34, reg_34.raw);

	reg_3c.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h3c);
	reg_3c.bits.lut_wsel = sel;
	reg_3c.bits.lut_wstaddr = 0;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h3c, reg_3c.raw);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr_trig_1t, 1);
	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wdata_trig_1t, 1);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr, 0);
	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr_trig_1t, 1);

	for (i = 0; i < LTM_BRIGHT_TONE_LUT_SIZE; i += 2) {
		val = (data[i] | (data[i + 1] << 16));
		ISP_WR_REG(ltm, reg_ltm_t, reg_h38, val);
		ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wdata_trig_1t, 1);
	}

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h34, lut_prog_en_bright, 0);
}

void ispblk_ltm_g_lut(struct isp_ctx *ctx, u8 sel, u16 *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_LTM];
	union reg_ltm_h34 reg_34;
	union reg_ltm_h3c reg_3c;
	u16 i = 0;
	u32 val = 0;

	ISP_WR_REG(ltm, reg_ltm_t, reg_h48, data[LTM_GLOBAL_LUT_SIZE]);

	reg_34.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h34);
	//reg_34.bits.lut_dbg_rsel = sel;
	reg_34.bits.lut_prog_en_global = 1;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h34, reg_34.raw);

	reg_3c.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h3c);
	reg_3c.bits.lut_wsel = sel;
	reg_3c.bits.lut_wstaddr = 0;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h3c, reg_3c.raw);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr_trig_1t, 1);
	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wdata_trig_1t, 1);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr, 0);
	ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wstaddr_trig_1t, 1);

	for (i = 0; i < LTM_GLOBAL_LUT_SIZE; i += 2) {
		val = (data[i] | (data[i + 1] << 16));
		ISP_WR_REG(ltm, reg_ltm_t, reg_h38, val);
		ISP_WR_BITS(ltm, reg_ltm_t, reg_h3c, lut_wdata_trig_1t, 1);
	}

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h34, lut_prog_en_global, 0);
}

void ispblk_ltm_config(struct isp_ctx *ctx, u8 ltm_en, u8 dehn_en, u8 behn_en, u8 ee_en)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_LTM];
	union reg_ltm_h00 reg_00;
	union reg_ltm_h8c reg_8c;

	reg_00.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h00);
	reg_00.bits.ltm_enable          = ltm_en;
	reg_00.bits.ltm_dark_enh_enable = dehn_en;
	reg_00.bits.ltm_brit_enh_enable = behn_en;
	reg_00.bits.force_dma_disable   = ((!dehn_en) | (!behn_en << 1));
	ISP_WR_REG(ltm, reg_ltm_t, reg_h00, reg_00.raw);

	ISP_WR_BITS(ltm, reg_ltm_t, reg_h64, ltm_ee_enable, ee_en);

	reg_8c.raw = ISP_RD_REG(ltm, reg_ltm_t, reg_h8c);
	reg_8c.bits.lmap_w_bit = g_lmp_cfg[ISP_PRERAW0].post_w_bit;
	reg_8c.bits.lmap_h_bit = g_lmp_cfg[ISP_PRERAW0].post_h_bit;
	ISP_WR_REG(ltm, reg_ltm_t, reg_h8c, reg_8c.raw);
}

void ispblk_ygamma_config(struct isp_ctx *ctx, bool en,
				u8 sel, u16 *data, u8 inv, u8 test_case)
{
	uintptr_t ygamma = ctx->phys_regs[ISP_BLK_ID_YGAMMA];
	int16_t i;
	bool lut_check_pass;
	union reg_ygamma_gamma_prog_data reg_data;
	union reg_ygamma_gamma_prog_ctrl prog_ctrl;
	union reg_ygamma_gamma_mem_sw_raddr sw_raddr;
	union reg_ygamma_gamma_mem_sw_rdata sw_rdata;

	prog_ctrl.raw = ISP_RD_REG(ygamma, reg_ygamma_t, gamma_prog_ctrl);
	prog_ctrl.bits.gamma_wsel    = sel;
	prog_ctrl.bits.gamma_prog_en = 1;
	//prog_ctrl.bits.gamma_prog_1to3_en = 1;
	ISP_WR_REG(ygamma, reg_ygamma_t, gamma_prog_ctrl, prog_ctrl.raw);

	ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_st_addr, gamma_st_addr, 0);
	ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_st_addr, gamma_st_w, 1);

	if (inv) {
		for (i = 255; i >= 0; i -= 2) {
			reg_data.raw = 0;
			reg_data.bits.gamma_data_e = data[i];
			reg_data.bits.gamma_data_o = data[i + 1];
			ISP_WR_REG(ygamma, reg_ygamma_t, gamma_prog_data, reg_data.raw);
			ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_ctrl, gamma_w, 1);
		}

		// set max to 0
		ISP_WR_REG(ygamma, reg_ygamma_t, gamma_prog_max, 0);
	} else {
		for (i = 0; i < 256; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.gamma_data_e = data[i];
			reg_data.bits.gamma_data_o = data[i + 1];
			ISP_WR_REG(ygamma, reg_ygamma_t, gamma_prog_data, reg_data.raw);
			ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_ctrl, gamma_w, 1);
		}
	}

	ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_ctrl, gamma_rsel, sel);
	ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_ctrl, gamma_prog_en, 0);

	// sw read mem0/mem1, check value
	if (test_case == 1) {
		lut_check_pass = true;
		ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_ctrl, gamma_prog_en, 1);
		sw_raddr.raw = ISP_RD_REG(ygamma, reg_ygamma_t, gamma_sw_raddr);
		for (i = 0; i < 256; i++) {
			sw_raddr.bits.gamma_sw_r_mem_sel = sel;
			sw_raddr.bits.gamma_sw_raddr = i;
			ISP_WR_REG(ygamma, reg_ygamma_t, gamma_sw_raddr, sw_raddr.raw);
			ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_sw_rdata, gamma_sw_r, 1);

			sw_rdata.raw = ISP_RD_REG(ygamma, reg_ygamma_t, gamma_sw_rdata);
			if ((sw_rdata.raw & 0xFFFF) != data[i]) {
				lut_check_pass = false;
				vi_pr(VI_DBG, "Ygamma LUT Check failed, lut[%d] = %d, should be %d\n",
					i, (sw_rdata.raw & 0xFFFF), data[i]);
				break;
			}
		}
		ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_prog_ctrl, gamma_prog_en, 0);

		if (lut_check_pass)
			vi_pr(VI_WARN, "Ygamma LUT Check passed\n");
	}
}

void ispblk_ygamma_enable(struct isp_ctx *ctx, bool enable)
{
	uintptr_t ygamma = ctx->phys_regs[ISP_BLK_ID_YGAMMA];

	ISP_WR_BITS(ygamma, reg_ygamma_t, gamma_ctrl, ygamma_enable, enable);
}

void ispblk_gamma_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data, u8 inv)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	int16_t i;
	union reg_isp_gamma_prog_data reg_data;
	union reg_isp_gamma_prog_ctrl prog_ctrl;

	prog_ctrl.raw = ISP_RD_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	prog_ctrl.bits.gamma_wsel    = sel;
	prog_ctrl.bits.gamma_prog_en = 1;
	prog_ctrl.bits.gamma_prog_1to3_en = 1;
	ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl, prog_ctrl.raw);

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_addr, 0);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_w, 1);

	if (inv) {
		for (i = 255; i >= 0; i -= 2) {
			reg_data.raw = 0;
			reg_data.bits.gamma_data_e = data[i];
			reg_data.bits.gamma_data_o = data[i + 1];
			reg_data.bits.gamma_w = 1;
			ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_data, reg_data.raw);
		}

		// set max to 0
		ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_max, 0);
	} else {
		for (i = 0; i < 256; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.gamma_data_e = data[i];
			reg_data.bits.gamma_data_o = data[i + 1];
			reg_data.bits.gamma_w = 1;
			ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_data, reg_data.raw);
		}
	}

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, sel);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_ctrl, gamma_enable, enable);
}

void ispblk_dhz_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DEHAZE];

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_bypass, dehaze_enable, en);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_bypass, dehaze_skin_lut_enable, 1);
}

/**
 * ispblk_rgbdither_config - setup rgb dither.
 *
 * @param ctx: global settings
 * @param en: rgb dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en,
			    bool histidx_en, bool fmnum_en)
{
	uintptr_t rgbdither = ctx->phys_regs[ISP_BLK_ID_RGB_DITHER];
	union reg_isp_rgb_dither_rgb_dither reg;

	reg.raw = 0;
	reg.bits.rgb_dither_enable = en;
	reg.bits.rgb_dither_mod_en = mod_en;
	reg.bits.rgb_dither_histidx_en = histidx_en;
	reg.bits.rgb_dither_fmnum_en = fmnum_en;
	reg.bits.rgb_dither_shdw_sel = 1;
	reg.bits.crop_widthm1 = ctx->img_width - 1;
	reg.bits.crop_heightm1 = ctx->img_height - 1;

	ISP_WR_REG(rgbdither, reg_isp_rgb_dither_t, rgb_dither, reg.raw);
}

void ispblk_clut_cmdq_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, bool en,
			int16_t *r_lut, int16_t *g_lut, int16_t *b_lut)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	u16 r_idx, g_idx, b_idx;
	union reg_isp_clut_ctrl      ctrl;
	union reg_isp_clut_prog_addr prog_addr;
	union reg_isp_clut_prog_data prog_data;
	u32 clut_phy_reg = ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CLUT;
	u32 idx = 0;
	u16 cmd_idx = 0;

	union cmdq_set *cmd_start = (union cmdq_set *)ctx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr;

	base_ion_cache_invalidate(ctx->isp_pipe_cfg[raw_num].cmdq_buf.phy_addr,
				  ctx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr,
				  ctx->isp_pipe_cfg[raw_num].cmdq_buf.buf_size);

	ctrl.raw = ISP_RD_REG(clut, reg_isp_clut_t, clut_ctrl);
	ctrl.bits.prog_en = 1;

	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 clut_phy_reg + _OFST(reg_isp_clut_t, clut_ctrl),
			 ctrl.raw);

	for (b_idx = 0; b_idx < 17; b_idx++) {
		for (g_idx = 0; g_idx < 17; g_idx++) {
			for (r_idx = 0; r_idx < 17; r_idx++) {
				idx = b_idx * 289 + g_idx * 17 + r_idx;

				prog_addr.raw = 0;
				prog_addr.bits.sram_r_idx = r_idx;
				prog_addr.bits.sram_g_idx = g_idx;
				prog_addr.bits.sram_b_idx = b_idx;
				cmdq_set_package(&cmd_start[cmd_idx++].reg,
						 clut_phy_reg + _OFST(reg_isp_clut_t, clut_prog_addr),
						 prog_addr.raw);

				prog_data.raw = 0;
				prog_data.bits.sram_wdata = b_lut[idx] + (g_lut[idx] << 10) + (r_lut[idx] << 20);
				prog_data.bits.sram_wr = 1;
				cmdq_set_package(&cmd_start[cmd_idx++].reg,
						 clut_phy_reg + _OFST(reg_isp_clut_t, clut_prog_data),
						 prog_data.raw);
			}
		}
	}

	ctrl.bits.clut_enable = en;
	ctrl.bits.prog_en = 0;
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			clut_phy_reg + _OFST(reg_isp_clut_t, clut_ctrl),
			ctrl.raw);

	base_ion_cache_flush(ctx->isp_pipe_cfg[raw_num].cmdq_buf.phy_addr,
				ctx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr,
				ctx->isp_pipe_cfg[raw_num].cmdq_buf.buf_size);

	ctx->isp_pipe_cfg[raw_num].cmdq_buf.cmd_idx = cmd_idx;
}

void ispblk_clut_config(struct isp_ctx *ctx, bool en,
				int16_t *r_lut, int16_t *g_lut, int16_t *b_lut)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	u16 r_idx, g_idx, b_idx;
	union reg_isp_clut_ctrl      ctrl;
	union reg_isp_clut_prog_addr prog_addr;
	union reg_isp_clut_prog_data prog_data;
	u32 idx = 0;

	ctrl.raw = ISP_RD_REG(clut, reg_isp_clut_t, clut_ctrl);
	ctrl.bits.prog_en = 1;
	ISP_WR_REG(clut, reg_isp_clut_t, clut_ctrl, ctrl.raw);

	for (b_idx = 0; b_idx < 17; b_idx++) {
		for (g_idx = 0; g_idx < 17; g_idx++) {
			for (r_idx = 0; r_idx < 17; r_idx++) {
				idx = b_idx * 289 + g_idx * 17 + r_idx;

				prog_addr.raw = 0;
				prog_addr.bits.sram_r_idx = r_idx;
				prog_addr.bits.sram_g_idx = g_idx;
				prog_addr.bits.sram_b_idx = b_idx;
				ISP_WR_REG(clut, reg_isp_clut_t, clut_prog_addr, prog_addr.raw);

				prog_data.raw		  = 0;
				prog_data.bits.sram_wdata = b_lut[idx] + (g_lut[idx] << 10) + (r_lut[idx] << 20);
				prog_data.bits.sram_wr	  = 1;
				ISP_WR_REG(clut, reg_isp_clut_t, clut_prog_data, prog_data.raw);
			}
		}
	}

	ctrl.bits.clut_enable = en;
	ctrl.bits.prog_en = 0;
	ISP_WR_REG(clut, reg_isp_clut_t, clut_ctrl, ctrl.raw);
}

void ispblk_csc_config(struct isp_ctx *ctx)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];

	ISP_WR_BITS(csc, reg_isp_csc_t, reg_0, csc_enable, 1);
}

static void _manr_init(struct isp_ctx *ctx)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	union reg_isp_mmap_04 reg_04;
	union reg_isp_mmap_08 reg_08;
	union reg_isp_mmap_38 reg_38;

	u16 data[] = {
		264,  436,  264,   60,	262,  436,  266,   60,	260,  435,  268,   61,	258,  435,  270,   61,
		255,  434,  272,   63,	253,  434,  274,   63,	251,  433,  275,   65,	249,  433,  277,   65,
		246,  432,  279,   67,	244,  432,  281,   67,	242,  431,  283,   68,	240,  431,  285,   68,
		237,  430,  286,   71,	235,  429,  288,   72,	233,  429,  290,   72,	231,  428,  292,   73,
		229,  427,  294,   74,	227,  427,  296,   74,	224,  426,  297,   77,	222,  425,  299,   78,
		220,  424,  301,   79,	218,  424,  303,   79,	216,  423,  305,   80,	214,  422,  306,   82,
		212,  421,  308,   83,	210,  420,  310,   84,	208,  419,  312,   85,	206,  419,  313,   86,
		204,  418,  315,   87,	202,  417,  317,   88,	199,  416,  319,   90,	197,  415,  321,   91,
		195,  414,  322,   93,	194,  413,  324,   93,	192,  412,  326,   94,	190,  411,  328,   95,
		188,  410,  329,   97,	186,  409,  331,   98,	184,  408,  333,   99,	182,  407,  334,  101,
		180,  405,  336,  103,	178,  404,  338,  104,	176,  403,  340,  105,	174,  402,  341,  107,
		172,  401,  343,  108,	171,  400,  345,  108,	169,  398,  346,  111,	167,  397,  348,  112,
		165,  396,  349,  114,	163,  395,  351,  115,	161,  393,  353,  117,	160,  392,  354,  118,
		158,  391,  356,  119,	156,  390,  358,  120,	154,  388,  359,  123,	153,  387,  361,  123,
		151,  386,  362,  125,	149,  384,  364,  127,	148,  383,  365,  128,	146,  381,  367,  130,
		144,  380,  368,  132,	143,  379,  370,  132,	141,  377,  371,  135,	139,  376,  373,  136,
		138,  374,  374,  138,
	};

	u8 i = 0;

	reg_04.bits.mmap_0_lpf_00 = 3;
	reg_04.bits.mmap_0_lpf_01 = 4;
	reg_04.bits.mmap_0_lpf_02 = 3;
	reg_04.bits.mmap_0_lpf_10 = 4;
	reg_04.bits.mmap_0_lpf_11 = 4;
	reg_04.bits.mmap_0_lpf_12 = 4;
	reg_04.bits.mmap_0_lpf_20 = 3;
	reg_04.bits.mmap_0_lpf_21 = 4;
	reg_04.bits.mmap_0_lpf_22 = 3;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_04, reg_04.raw);

	reg_08.bits.mmap_0_map_coring = 0;
	reg_08.bits.mmap_0_map_gain   = 8;
	reg_08.bits.mmap_0_map_thd_l  = 64; /* for imx327 tuning */
	reg_08.bits.mmap_0_map_thd_h  = 255;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_08, reg_08.raw);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_0c,
		    mmap_0_luma_adapt_lut_in_0, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_0c,
		    mmap_0_luma_adapt_lut_in_1, 600);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_10,
		    mmap_0_luma_adapt_lut_in_2, 1500);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_10,
		    mmap_0_luma_adapt_lut_in_3, 2500);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_14,
		    mmap_0_luma_adapt_lut_out_0, 63);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_14,
		    mmap_0_luma_adapt_lut_out_1, 48);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_14,
		    mmap_0_luma_adapt_lut_out_2, 8);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_14,
		    mmap_0_luma_adapt_lut_out_3, 2);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_18,
		    mmap_0_luma_adapt_lut_slope_0, -27);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_18,
		    mmap_0_luma_adapt_lut_slope_1, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_1c,
		    mmap_0_luma_adapt_lut_slope_2, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_1c, mmap_0_map_dshift_bit, 5);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_20, mmap_0_iir_prtct_lut_in_0, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_20, mmap_0_iir_prtct_lut_in_1, 45);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_20, mmap_0_iir_prtct_lut_in_2, 90);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_20, mmap_0_iir_prtct_lut_in_3, 255);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_24, mmap_0_iir_prtct_lut_out_0, 6);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_24, mmap_0_iir_prtct_lut_out_1, 10);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_24, mmap_0_iir_prtct_lut_out_2, 9);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_24, mmap_0_iir_prtct_lut_out_3, 2);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_28,
		    mmap_0_iir_prtct_lut_slope_0, 12);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_28,
		    mmap_0_iir_prtct_lut_slope_1, -4);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_2c,
		    mmap_0_iir_prtct_lut_slope_2, -4);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_70, mmap_0_gain_ratio_r, 4096);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_70, mmap_0_gain_ratio_g, 4096);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_74, mmap_0_gain_ratio_b, 4096);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_78, mmap_0_ns_slope_r, 5);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_78, mmap_0_ns_slope_g, 4);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_7c, mmap_0_ns_slope_b, 6);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_80, mmap_0_ns_luma_th0_r, 16);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_80, mmap_0_ns_luma_th0_g, 16);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_84, mmap_0_ns_luma_th0_b, 16);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_84, mmap_0_ns_low_offset_r, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_88, mmap_0_ns_low_offset_g, 2);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_88, mmap_0_ns_low_offset_b, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_8c, mmap_0_ns_high_offset_r, 724);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_8c, mmap_0_ns_high_offset_g, 724);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_90, mmap_0_ns_high_offset_b, 724);

	reg_38.bits.mmap_1_map_coring = 0;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_38, reg_38.raw);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_a0, mmap_1_gain_ratio_r, 4096);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_a0, mmap_1_gain_ratio_g, 4096);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_a4, mmap_1_gain_ratio_b, 4096);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_a8, mmap_1_ns_slope_r, 5 * 4);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_a8, mmap_1_ns_slope_g, 4 * 4);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_ac, mmap_1_ns_slope_b, 6 * 4);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_b0, mmap_1_ns_luma_th0_r, 16);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_b0, mmap_1_ns_luma_th0_g, 16);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_b4, mmap_1_ns_luma_th0_b, 16);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_b4, mmap_1_ns_low_offset_r, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_b8, mmap_1_ns_low_offset_g, 2);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_b8, mmap_1_ns_low_offset_b, 0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_bc, mmap_1_ns_high_offset_r, 724);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_bc, mmap_1_ns_high_offset_g, 724);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_c0, mmap_1_ns_high_offset_b, 724);

	for (i = 0; i < ARRAY_SIZE(data) / 4; ++i) {
		u64 val = 0;

		ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, sram_wen, 0);
		ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, sram_wadd, i);

		val = ((u64)data[i * 4] | (u64)data[i * 4 + 1] << 13 |
			(u64)data[i * 4 + 2] << 26 | (u64)data[i * 4 + 3] << 39);
		ISP_WR_REG(manr, reg_isp_mmap_t, reg_64, val & 0xffffffff);
		ISP_WR_REG(manr, reg_isp_mmap_t, reg_68, val >> 32);

		ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, sram_wen, 1);
	}

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, sram_wen, 0);
}

void ispblk_tnr_post_chg(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	int w = ctx->isp_pipe_cfg[raw_num].is_tile ?
			ctx->img_width :
			ctx->isp_pipe_cfg[raw_num].crop.w;
	int h = ctx->isp_pipe_cfg[raw_num].crop.h;
	int grid_size = (1 << ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit);

	union reg_isp_mmap_60 reg_60;
	union reg_isp_mmap_30 reg_30;
	union reg_isp_mmap_d0 reg_d0;
	union reg_isp_mmap_d4 reg_d4;
	union reg_isp_mmap_d8 reg_d8;

	reg_60.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_60);
	reg_60.bits.rgbmap_w_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;
	reg_60.bits.rgbmap_h_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_60, reg_60.raw);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_04, wh_sw_mode, 1);

	reg_30.raw = 0;
	reg_30.bits.img_widthm1_sw	= ((((w + grid_size - 1) / grid_size) * 6 + 47) / 48 * 8 * grid_size - 1);
	reg_30.bits.img_heightm1_sw	= h - 1;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_30, reg_30.raw);

	reg_d0.raw = 0;
	reg_d0.bits.crop_enable_scalar		= 1;
	reg_d0.bits.img_width_crop_scalar	= reg_30.bits.img_widthm1_sw;
	reg_d0.bits.img_height_crop_scalar	= reg_30.bits.img_heightm1_sw;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_d0, reg_d0.raw);

	reg_d4.raw = 0;
	reg_d4.bits.crop_w_str_scalar		= 0;
	reg_d4.bits.crop_w_end_scalar		= w - 1;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_d4, reg_d4.raw);

	reg_d8.raw = 0;
	reg_d8.bits.crop_h_str_scalar		= 0;
	reg_d8.bits.crop_h_end_scalar		= h - 1;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_d8, reg_d8.raw);

	ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R);
	ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R);
		ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R);
	}
}

void ispblk_manr_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	u8 dma_disable;
	union reg_isp_mmap_00 reg_00;
	u8 raw_num = vi_get_first_raw_num(ctx);

	if (!en) {
		reg_00.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_00);
		reg_00.bits.mmap_0_enable = 0;
		reg_00.bits.mmap_1_enable = 0;
		reg_00.bits.bypass = 1;
		ISP_WR_REG(manr, reg_isp_mmap_t, reg_00, reg_00.raw);

		ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, force_dma_disable, 0xff);
		return;
	}

	//Init once for tuning
	_manr_init(ctx);

	// [0]: prv_l
	// [1]: prv_s
	// [2]: cur_l
	// [3]: cur_s
	// [4]: iir_r
	// [5]: ai_isp
	// [6]: iir_w
	// [7]: no use
	// reg_dma_enable = ~reg_force_dma_disable
	if (_is_all_online(ctx)) //all online mode
		dma_disable = 0xaf;
	else { // fe->be->dram->post or fe->dram->be->post
		dma_disable = (ctx->is_hdr_on) ? 0xa0 : 0xaa;
	}

	reg_00.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_00);
	reg_00.bits.mmap_0_enable = 1;
	reg_00.bits.mmap_1_enable = (ctx->is_hdr_on) ? 1 : 0;
	reg_00.bits.bypass = 0;
	reg_00.bits.reg_2_tap_en = 1;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_00, reg_00.raw);

	ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = g_w_bit[raw_num];
	ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = g_h_bit[raw_num];

	ispblk_tnr_post_chg(ctx, raw_num);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_6c, force_dma_disable, dma_disable);
}

void ispblk_mmap_dma_mode(struct isp_ctx *ctx, u32 dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	//1: SW mode: config by SW 0: HW mode: auto config by HW
	sys_ctrl.raw = ISP_RD_REG(dmab, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= 0x1;
	sys_ctrl.bits.seglen_sel	= 0x0;
	sys_ctrl.bits.segnum_sel	= 0x0;
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);
}

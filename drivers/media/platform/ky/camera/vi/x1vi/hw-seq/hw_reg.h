// SPDX-License-Identifier: GPL-2.0
/*
 * hw_reg.h - definition of isp hw registers
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_ISP_HW_REG_H_
#define _KY_ISP_HW_REG_H_

#include <asm/io.h>
//#include <soc/spm/plat.h>
#ifdef CAM_MODULE_TAG
#undef CAM_MODULE_TAG
#endif
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>
#define read32(a)		readl((volatile void __iomem *)(a))
#define write32(a, v)		writel((v), (volatile void __iomem *)(a))


#define TOP_REG_OFFSET(n)	((n) * 4)
#define TOP_REG_ADDR(base, n)	((base) + TOP_REG_OFFSET((n)))
#define TOP_REG(n)		(TOP_REG_ADDR(sc_block->base_addr, (n)))

union isp_top_reg_0 {
	struct {
		unsigned int m_nwidth						:	13;
		unsigned int rsvd0						:	3;
		unsigned int m_nheight						:	13;
		unsigned int rsvd1						:	3;
	}field;
	unsigned int value;
};

union isp_top_reg_1 {
	struct {
		unsigned int m_nglobalblacklevel			:	9;
		unsigned int m_ncfapattern				:	2;
		unsigned int idi_reg_latch_trig_pip			:	1;
		unsigned int m_cfg_ready				:	1;
		unsigned int rsvd1					:	3;
		unsigned int m_nglobalblccompensategain			:	13;
		unsigned int rsvd2					:	3;
	}field;
	unsigned int value;
};

union isp_top_reg_2 {
	struct {
		unsigned int m_nbinningwidth				:	13;
		unsigned int rsvd0					:	3;
		unsigned int m_nbinningheight				:	13;
		unsigned int rsvd1					:	1;
		unsigned int m_nratio_binning				:	2;
	}field;
	unsigned int value;
};

union isp_top_reg_3 {
	struct {
		unsigned int idi_line_depth					:	6;
		unsigned int rsvd0						:	2;
		unsigned int idi_fifo_depth					:	13;
		unsigned int rsvd1						:	11;
	}field;
	unsigned int value;
};

union isp_top_reg_4 {
	struct {
		unsigned int idi_pix_depth					:	13;
		unsigned int rsvd0						:	3;
		unsigned int idi_fifo_line_th					:	6;
		unsigned int rsvd1						:	10;
	}field;
	unsigned int value;
};

union isp_top_reg_13 {
	struct {
		unsigned int idi_src_sel					:	10;
		unsigned int idi_online_ena					:	1;
		unsigned int idi_offline_ena					:	1;
		unsigned int idi_rdp_ena					:	1;
		unsigned int rsvd						:	19;
	}field;
	unsigned int value;
};

union isp_top_reg_14 {
	struct {
		unsigned int idi_insert_dummy_line			:	8;
		unsigned int idi_gap_value				:	10;
		unsigned int idi_fifo_pix_th				:	13;
		unsigned int gap_hardware_mode				:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_15 {
	struct {
		unsigned int tpg_en					:	1;
		unsigned int sensor_timing_en				:	1;
		unsigned int rolling_en					:	1;
		unsigned int tpg_select					:	1;
		unsigned int rsvd					:	4;
		unsigned int hblank					:	12;
		unsigned int dummy_line					:	10;
		unsigned int rsvd1					:	2;
	}field;
	unsigned int value;
};

union isp_top_reg_16 {
	struct {
		unsigned int vblank					:	21;
		unsigned int rsvd					:	3;
		unsigned int valid_blank				:	8;
	}field;
	unsigned int value;
};

union isp_top_reg_17 {
	struct {
		unsigned int idi_wdma_ch0_src_sel			:	8;
		unsigned int idi_ch0_img_wr_width_byte			:	15;
		unsigned int idi_ch0_wr_14bit_en			:	1;
		unsigned int idi_ch0_wr_burst_len_sel			:	3;
		unsigned int rsvd					:	4;
		unsigned int rdp0_cfg_ready				:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_18 {
	struct {
		unsigned int idi_ch0_img_wr_width_pix			:	16;
		unsigned int idi_ch0_img_wr_height			:	16;
	}field;
	unsigned int value;
};

union isp_top_reg_19 {
	struct {
		unsigned int idi_wdma_ch1_src_sel			:	8;
		unsigned int idi_ch1_img_wr_width_byte			:	15;
		unsigned int idi_ch1_wr_14bit_en			:	1;
		unsigned int idi_ch1_wr_burst_len_sel			:	3;
		unsigned int rsvd					:	4;
		unsigned int rdp1_cfg_ready				:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_20 {
	struct {
		unsigned int idi_ch1_img_wr_width_pix			:	16;
		unsigned int idi_ch1_img_wr_height			:	16;
	}field;
	unsigned int value;
};

union isp_top_reg_21 {
	struct {
		unsigned int idi_ch0_img_rd_width_byte			:	16;
		unsigned int idi_ch0_dma_sync_fifo_th			:	8;
		unsigned int idi_ch0_rd_fifo_depth			:	7;
		unsigned int idi_ch0_rd_burst_len_sel			:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_22 {
	struct {
		unsigned int idi_ch0_img_rd_width_pix		:	14;
		unsigned int idi_ch0_img_rd_raw8_type		:	1;
		unsigned int idi_ch0_img_rd_raw10_type		:	1;
		unsigned int idi_ch0_img_rd_height		:	14;
		unsigned int idi_ch0_img_rd_raw1214_type	:	1;
		unsigned int rsvd				:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_23 {
	struct {
		unsigned int idi_ch1_img_rd_width_byte		:	16;
		unsigned int idi_ch1_dma_sync_fifo_th		:	8;
		unsigned int idi_ch1_rd_fifo_depth		:	7;
		unsigned int idi_ch1_rd_burst_len_sel		:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_24 {
	struct {
		unsigned int idi_ch1_img_rd_width_pix		:	14;
		unsigned int idi_ch1_img_rd_raw8_type		:	1;
		unsigned int idi_ch1_img_rd_raw10_type		:	1;
		unsigned int idi_ch1_img_rd_height		:	14;
		unsigned int idi_ch1_img_rd_raw1214_type	:	1;
		unsigned int rsvd				:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_25 {
	struct {
		unsigned int offline_hdr_ena			:	1;
		unsigned int pip0_vsync_pass_through		:	1;
		unsigned int pip1_vsync_pass_through		:	1;
		unsigned int rsvd				:	29;
	}field;
	unsigned int value;
};

union isp_top_reg_26 {
	struct {
		unsigned int pip0_vsync2href_dly_cnt		:	11;
		unsigned int rsvd0				:	5;
		unsigned int pip1_vsync2href_dly_cnt		:	11;
		unsigned int rsvd1				:	5;
	}field;
	unsigned int value;
};

union isp_top_reg_75 {
	struct {
		unsigned int wbc_clk_auto_ena				:	1;
		unsigned int rdns_clk_auto_ena				:	1;
		unsigned int afc_clk_auto_ena				:	1;
		unsigned int wbm_clk_auto_ena				:	1;
		unsigned int bpc_clk_auto_ena				:	1;
		unsigned int binning_clk_auto_ena			:	1;
		unsigned int ltm_clk_auto_ena				:	1;
		unsigned int aem_clk_auto_ena				:	1;
		unsigned int eis_clk_auto_ena				:	1;
		unsigned int hdr_clk_auto_ena				:	1;
		unsigned int cmc_clk_auto_ena				:	1;
		unsigned int idi_clk_auto_ena				:	1;
		unsigned int demosaic_clk_auto_ena			:	1;
		unsigned int gtm_clk_auto_ena				:	1;
		unsigned int lsc_clk_auto_ena				:	1;
		unsigned int lscm_clk_auto_ena				:	1;
		unsigned int nlc_clk_auto_ena				:	1;
		unsigned int rgb4gain_clk_auto_ena			:	1;
		unsigned int top_reg_clk_auto_ena			:	1;
		unsigned int debug_clk_en				:	1;
		unsigned int auto_gate_clk_ctrl				:	12;
	}field;
	unsigned int value;
};

union isp_top_reg_76 {
	struct {
		unsigned int idi_online_hdr_ena				:	1;
		unsigned int idi_offline_hdr_ena			:	1;
		unsigned int idi_mix_hdr_ena				:	1;
		unsigned int idi_rgb_ir_mode				:	1;
		unsigned int outstanding_read_en			:	1;
		unsigned int idi_sony_hdr_mode				:	1;
		unsigned int idi_low_pr_mode				:	1;
		unsigned int mix_hdr_rdp_ena				:	1;
		unsigned int idi_mix_hdr_line				:	10;
		unsigned int rsvd1					:	2;
		unsigned int idi_ddr_wr_line_cnt			:	5;
		unsigned int send_speed_ctrl				:	5;
		unsigned int rsvd2					:	1;
		unsigned int pip1_gap_hardware_mode			:	1;
	}field;
	unsigned int value;
};

union isp_top_reg_77 {
	struct {
		unsigned int idi_pip0_gap_value				:	16;
		unsigned int idi_pip1_gap_value				:	16;
	}field;
	unsigned int value;
};

union isp_top_reg_78 {
	struct {
		unsigned int idi_pip01_gap_value			:	16;
		unsigned int idi_dma_timeout				:	16;
	}field;
	unsigned int value;
};

union isp_top_reg_82 {
	struct {
		unsigned int idi_pip0_rd_speed_end_cnt			:	20;
		unsigned int idi_ch0_rd_bst_len_sel			:	6;
		unsigned int rsvd					:	6;
	}field;
	unsigned int value;
};

union isp_top_reg_83 {
	struct {
		unsigned int idi_pip1_rd_speed_end_cnt			:	20;
		unsigned int idi_ch1_rd_bst_len_sel			:	5;
		unsigned int rsvd					:	7;
	}field;
	unsigned int value;
};

union isp_top_reg_86 {
	struct {
		unsigned int global_reset					:	1;
		unsigned int rsvd1						:	7;
		unsigned int global_reset_cyc					:	8;
		unsigned int rsvd2						:	16;
	}field;
	unsigned int value;
};

union isp_top_reg_93 {
	struct {
		unsigned int m_nwidth_offset				:	13;
		unsigned int rsvd0					:	3;
		unsigned int m_nheight_offset				:	13;
		unsigned int rsvd1					:	3;
	}field;
	unsigned int value;
};

union isp_top_reg_94 {
	struct {
		unsigned int m_ncrop_width					:	13;
		unsigned int rsvd0						:	3;
		unsigned int m_ncrop_height					:	13;
		unsigned int rsvd1						:	3;
	}field;
	unsigned int value;
};

//dma regs
union dma_reg_38 {
	struct {
		unsigned int ch0_wr_pitch0					:	16;
		unsigned int ch0_wr_pitch1					:	16;
	}field;
	unsigned int value;
};

union dma_reg_52 {
	struct {
		unsigned int ch0_wr_fifo_depth					:	8;
		unsigned int ch0_wr_offset					:	8;
		unsigned int ch0_wr_src_sel					:	4;
		unsigned int ch0_wr_weight					:	3;
		unsigned int rsvd0						:	1;
		unsigned int ch0_fifo_div_mode					:	4;
		unsigned int rsvd1						:	3;
		unsigned int ch0_wr_ready					:	1;
	}field;
	unsigned int value;
};

union dma_reg_68 {
	struct {
		unsigned int wr_burst_length					:	8;
		unsigned int rsvd						:	24;
	}field;
	unsigned int value;
};

union dma_reg_69 {
	struct {
		unsigned int ch0_wr_start					:	1;
		unsigned int ch0_wr_done					:	1;
		unsigned int ch0_wr_err						:	1;
		unsigned int ch1_wr_start					:	1;
		unsigned int ch1_wr_done					:	1;
		unsigned int ch1_wr_err						:	1;
		unsigned int ch2_wr_start					:	1;
		unsigned int ch2_wr_done					:	1;
		unsigned int ch2_wr_err						:	1;
		unsigned int ch3_wr_start					:	1;
		unsigned int ch3_wr_done					:	1;
		unsigned int ch3_wr_err						:	1;
		unsigned int ch4_wr_start					:	1;
		unsigned int ch4_wr_done					:	1;
		unsigned int ch4_wr_err						:	1;
		unsigned int ch5_wr_start					:	1;
		unsigned int ch5_wr_done					:	1;
		unsigned int ch5_wr_err						:	1;
		unsigned int ch6_wr_start					:	1;
		unsigned int ch6_wr_done					:	1;
		unsigned int ch6_wr_err						:	1;
		unsigned int ch7_wr_start					:	1;
		unsigned int ch7_wr_done					:	1;
		unsigned int ch7_wr_err						:	1;
		unsigned int ch8_wr_start					:	1;
		unsigned int ch8_wr_done					:	1;
		unsigned int ch8_wr_err						:	1;
		unsigned int ch9_wr_start					:	1;
		unsigned int ch9_wr_done					:	1;
		unsigned int ch9_wr_err						:	1;
		unsigned int ch10_wr_start					:	1;
		unsigned int ch10_wr_done					:	1;
	}field;
	unsigned int value;
};

union dma_reg_70 {
	struct {
		unsigned int ch10_wr_err					:	1;
		unsigned int ch11_wr_start					:	1;
		unsigned int ch11_wr_done					:	1;
		unsigned int ch11_wr_err					:	1;
		unsigned int ch12_wr_start					:	1;
		unsigned int ch12_wr_done					:	1;
		unsigned int ch12_wr_err					:	1;
		unsigned int ch13_wr_start					:	1;
		unsigned int ch13_wr_done					:	1;
		unsigned int ch13_wr_err					:	1;
		unsigned int ch14_p0_wr_start					:	1;
		unsigned int ch14_p0_wr_done					:	1;
		unsigned int ch14_p0_wr_err					:	1;
		unsigned int ch14_p1_wr_start					:	1;
		unsigned int ch14_p1_wr_done					:	1;
		unsigned int ch14_p1_wr_err					:	1;
		unsigned int ch15_wr_start					:	1;
		unsigned int ch15_wr_done					:	1;
		unsigned int ch15_wr_err					:	1;
		unsigned int ch0_rd_start					:	1;
		unsigned int ch0_rd_done					:	1;
		unsigned int ch0_rd_err						:	1;
		unsigned int ch1_rd_start					:	1;
		unsigned int ch1_rd_done					:	1;
		unsigned int ch1_rd_err						:	1;
		unsigned int ch2_rd_start					:	1;
		unsigned int ch2_rd_done					:	1;
		unsigned int ch2_rd_err						:	1;
		unsigned int rsvd						:	4;
	}field;
	unsigned int value;
};

union dma_reg_79 {
	struct {
		unsigned int ch0_rd_pitch					:	16;
		unsigned int ch0_rd_weight					:	3;
		unsigned int rsvd						:	12;
		unsigned int ch0_rd_trigger					:	1;
	}field;
	unsigned int value;
};

union dma_reg_82 {
	struct {
		unsigned int dmac_postwr_en					:	16;
		unsigned int dmac_arqos						:	4;
		unsigned int dmac_awqos						:	4;
		unsigned int dmac_arb_mode					:	2;
		unsigned int dmac_max_req_num					:	3;
		unsigned int dmac_axi_sec					:	1;
		unsigned int dmac_rst_req					:	1;
		unsigned int dmac_rst_n_pwr					:	1;
	}field;
	unsigned int value;
};

union dma_reg_83 {
	struct {
		unsigned int cfg_dmac_wr_int_clr			:	1;
		unsigned int cfg_dmac_rd_int_clr			:	1;
		unsigned int fbc_enc0_clk_auto_ena			:	1;
		unsigned int fbc_enc1_clk_auto_ena			:	1;
		unsigned int dma_master_ctrl_clk_auto_ena		:	1;
		unsigned int dmac_top_cfg_clk_auto_ena			:	1;
		unsigned int dma_err_sel				:	1;
		unsigned int rsvd					:	19;
		unsigned int dma_overrun_recover_en			:	1;
		unsigned int dma_overlap_recover_en			:	1;
		unsigned int afbc0_enable				:	1;
		unsigned int afbc1_enable				:	1;
		unsigned int rawdump0_enable				:	1;
		unsigned int rawdump1_enable				:	1;
	}field;
	unsigned int value;
};

union dma_reg_86 {
	struct {
		unsigned int mmu_start_p0					:	1;
		unsigned int mmu_start_p1					:	1;
		unsigned int rsvd0						:	6;
		unsigned int sw_mmu_shadow_en_p0				:	1;
		unsigned int sw_mmu_shadow_en_p1				:	1;
		unsigned int rsvd1						:	22;
	}field;
	unsigned int value;
};

//postpipe regs
union pp_reg_2 {
	struct {
		unsigned int fmt0_m_nFormat				:	3;
		unsigned int fmt0_m_bCompress				:	1;
		unsigned int fmt0_m_bSwitchUVFlag			:	1;
		unsigned int fmt0_m_bCompressDithering			:	1;
		unsigned int fmt0_m_bConvertDithering			:	1;
		unsigned int fmt0_m_bSwitchYCFlag			:	1;
		unsigned int rsvd					:	24;
	}field;
	unsigned int value;
};

union pp_reg_14 {
	struct {
		unsigned int dma_mux_ctrl_0					:	1;
		unsigned int dma_mux_ctrl_1					:	1;
		unsigned int dma_mux_ctrl_2					:	1;
		unsigned int dma_mux_ctrl_3					:	1;
		unsigned int dma_mux_ctrl_4					:	1;
		unsigned int dma_mux_ctrl_5					:	1;
		unsigned int dma_mux_ctrl_6					:	2;
		unsigned int dma_mux_ctrl_7					:	2;
		unsigned int rsvd						:	22;
	}field;
	unsigned int value;
};

union pp_reg_8 {
	struct {
		unsigned int dwt0_time_limit					:	16;
		unsigned int dwt0_src_sel					:	3;
		unsigned int dwt0_mode_sel					:	1;
		unsigned int dwt0_ena						:	1;
		unsigned int rsvd						:	11;
	}field;
	unsigned int value;
};
//scaler regs
union scl_reg_0 {
	struct {
		unsigned int m_nintrimEb					:	1;
		unsigned int m_nouttrimEb					:	1;
		unsigned int m_nscalerEb					:	1;
		unsigned int m_nblock_en					:	1;
		unsigned int rsvd0						:	4;
		unsigned int m_nintrimStartX					:	13;
		unsigned int rsvd1						:	11;
	}field;
	unsigned int value;
};

union scl_reg_11 {
	struct {
		unsigned int pipe_sel						:	1;
		unsigned int rsvd						:	31;
	}field;
	unsigned int value;
};
/*
union scl_reg_12 {
	struct {
		unsigned int m_nintrim_out_width			:	13;
		unsigned int rsvd0							:	3;
		unsigned int m_nintrim_out_height			:	13;
		unsigned int rsvd1							:	3;
	}field;
	unsigned int value;
};

union scl_reg_13 {
	struct {
		unsigned int m_nouttrim_out_width			:	13;
		unsigned int rsvd0							:	3;
		unsigned int m_nouttrim_out_height			:	13;
		unsigned int rsvd1							:	3;
	}field;
	unsigned int value;
};
*/
//isp afbc
#define ISP_AFBC_HEADER_BASE_ADDR_LOW	(0x00)
#define ISP_AFBC_HEADER_BASE_ADDR_HIGH	(0x04)
#define ISP_AFBC_PAYLOAD_BASE_ADDR_LOW	(0x08)
#define ISP_AFBC_PAYLOAD_BASE_ADDR_HIGH	(0x0c)
#define ISP_AFBC_Bbox_coor_x			(0x10)
#define ISP_AFBC_Bbox_coor_y			(0x14)
#define ISP_AFBC_Y_BUF_BASE_ADDR		(0x18)
#define ISP_AFBC_Y_BUF_PITCH			(0x1c)
#define ISP_AFBC_UV_BUF_BASE_ADDR		(0x20)
#define ISP_AFBC_UV_BUF_PITCH			(0x24)
#define ISP_AFBC_Y_BUF_SIZE			(0x28)
#define ISP_AFBC_UV_BUF_SIZE			(0x2c)
#define ISP_AFBC_REG_SHADOW_CTRL		(0x30)
#define ISP_AFBC_IRQ_MASK			(0x34)
#define ISP_AFBC_IRQ_CLEAR			(0x38)
#define ISP_AFBC_DMA_CTRL0			(0x3c)
#define ISP_AFBC_ENC_MODE			(0x40)
#define ISP_AFBC_DMAC_LENGTH			(0x44)
#define ISP_AFBC_IRQ_STATUS			(0x48)
union afbc_reg_irq {
	struct {
		unsigned int dma_wr_err						:	16;
		unsigned int dma_wr_eof						:	1;
		unsigned int cfg_update_done					:	1;
		unsigned int rsvd						:	14;
	}field;
	unsigned int value;
};

union afbc_reg_bbox_coor_x {
	struct {
		unsigned int bbox_start_x					:	16;
		unsigned int bbox_end_x						:	16;
	}field;
	unsigned int value;
};

union afbc_reg_bbox_coor_y {
	struct {
		unsigned int bbox_start_y					:	16;
		unsigned int bbox_end_y						:	16;
	}field;
	unsigned int value;
};

union afbc_reg_y_buf_size {
	struct {
		unsigned int y_buf_size_x					:	16;
		unsigned int y_buf_size_y					:	16;
	}field;
	unsigned int value;
};

union afbc_reg_uv_buf_size {
	struct {
		unsigned int uv_buf_size_x					:	16;
		unsigned int uv_buf_size_y					:	16;
	}field;
	unsigned int value;
};

union afbc_reg_shadow_ctrl {
	struct {
		unsigned int direct_swap					:	1;
		unsigned int pending_swap					:	1;
		unsigned int rsvd						:	30;
	}field;
	unsigned int value;
};
#endif

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name:vi_reg_blocks.h
 * Description:HW register description
 */

#ifndef _VI_REG_BLOCKS_H_
#define _VI_REG_BLOCKS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           module definition            */
/******************************************/
struct reg_pre_raw_be_t {
	union reg_pre_raw_be_top_ctrl           top_ctrl;
	union reg_pre_raw_be_up_pq_en           up_pq_en;
	union reg_pre_raw_be_img_size_le        img_size_le;
	uint32_t                                _resv_0xc[1];
	union reg_pre_raw_be_pre_raw_dummy      pre_raw_dummy;
	union reg_pre_raw_be_debug_info         be_info;
	union reg_pre_raw_be_dma_idle_info      be_dma_idle_info;
	union reg_pre_raw_be_ip_idle_info       be_ip_idle_info;
	union reg_pre_raw_be_line_balance_ctrl  line_balance_ctrl;
	union reg_pre_raw_be_debug_enable       debug_enable;
	union reg_pre_raw_be_tvalid_status      tvalid_status;
	union reg_pre_raw_be_tready_status      tready_status;
	union reg_pre_raw_be_patgen1            patgen1;
	union reg_pre_raw_be_patgen2            patgen2;
	union reg_pre_raw_be_patgen3            patgen3;
	union reg_pre_raw_be_patgen4            patgen4;
	union reg_pre_raw_be_chksum_enable      chksum_enable;
	union reg_pre_raw_be_chksum             chksum;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_dma_ctl_t {
	union reg_isp_dma_ctl_sys_control       sys_control;
	union reg_isp_dma_ctl_base_addr         base_addr;
	union reg_isp_dma_ctl_dma_seglen        dma_seglen;
	union reg_isp_dma_ctl_dma_stride        dma_stride;
	union reg_isp_dma_ctl_dma_segnum        dma_segnum;
	union reg_isp_dma_ctl_dma_status        dma_status;
	union reg_isp_dma_ctl_dma_slicesize     dma_slicesize;
	union reg_isp_dma_ctl_dma_dummy         dma_dummy;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_ae_hist_t {
	union reg_isp_ae_hist_ae_hist_status    ae_hist_status;
	union reg_isp_ae_hist_ae_hist_grace_reset  ae_hist_grace_reset;
	union reg_isp_ae_hist_ae_hist_monitor   ae_hist_monitor;
	union reg_isp_ae_hist_ae_hist_bypass    ae_hist_bypass;
	union reg_isp_ae_hist_ae_kickoff        ae_kickoff;
	union reg_isp_ae_hist_sts_ae0_hist_enable  sts_ae0_hist_enable;
	union reg_isp_ae_hist_sts_ae_offsetx    sts_ae_offsetx;
	union reg_isp_ae_hist_sts_ae_offsety    sts_ae_offsety;
	union reg_isp_ae_hist_sts_ae_numxm1     sts_ae_numxm1;
	union reg_isp_ae_hist_sts_ae_numym1     sts_ae_numym1;
	union reg_isp_ae_hist_sts_ae_width      sts_ae_width;
	union reg_isp_ae_hist_sts_ae_height     sts_ae_height;
	union reg_isp_ae_hist_sts_ae_sts_div    sts_ae_sts_div;
	union reg_isp_ae_hist_sts_hist_mode     sts_hist_mode;
	uint32_t                                _resv_0x38[1];
	union reg_isp_ae_hist_ae_hist_monitor_select  ae_hist_monitor_select;
	union reg_isp_ae_hist_ae_hist_location  ae_hist_location;
	uint32_t                                _resv_0x44[1];
	union reg_isp_ae_hist_sts_ir_ae_offsetx  sts_ir_ae_offsetx;
	union reg_isp_ae_hist_sts_ir_ae_offsety  sts_ir_ae_offsety;
	union reg_isp_ae_hist_sts_ir_ae_numxm1  sts_ir_ae_numxm1;
	union reg_isp_ae_hist_sts_ir_ae_numym1  sts_ir_ae_numym1;
	union reg_isp_ae_hist_sts_ir_ae_width   sts_ir_ae_width;
	union reg_isp_ae_hist_sts_ir_ae_height  sts_ir_ae_height;
	union reg_isp_ae_hist_sts_ir_ae_sts_div  sts_ir_ae_sts_div;
	uint32_t                                _resv_0x64[1];
	union reg_isp_ae_hist_ae_hist_bayer_starting  ae_hist_bayer_starting;
	union reg_isp_ae_hist_ae_hist_dummy     ae_hist_dummy;
	union reg_isp_ae_hist_ae_hist_checksum  ae_hist_checksum;
	union reg_isp_ae_hist_wbg_4             wbg_4;
	union reg_isp_ae_hist_wbg_5             wbg_5;
	union reg_isp_ae_hist_wbg_6             wbg_6;
	union reg_isp_ae_hist_wbg_7             wbg_7;
	uint32_t                                _resv_0x84[7];
	union reg_isp_ae_hist_dmi_enable        dmi_enable;
	uint32_t                                _resv_0xa4[3];
	union reg_isp_ae_hist_ae_face0_location  ae_face0_location;
	union reg_isp_ae_hist_ae_face1_location  ae_face1_location;
	union reg_isp_ae_hist_ae_face2_location  ae_face2_location;
	union reg_isp_ae_hist_ae_face3_location  ae_face3_location;
	union reg_isp_ae_hist_ae_face0_size     ae_face0_size;
	union reg_isp_ae_hist_ae_face1_size     ae_face1_size;
	union reg_isp_ae_hist_ae_face2_size     ae_face2_size;
	union reg_isp_ae_hist_ae_face3_size     ae_face3_size;
	union reg_isp_ae_hist_ir_ae_face0_location  ir_ae_face0_location;
	union reg_isp_ae_hist_ir_ae_face1_location  ir_ae_face1_location;
	union reg_isp_ae_hist_ir_ae_face2_location  ir_ae_face2_location;
	union reg_isp_ae_hist_ir_ae_face3_location  ir_ae_face3_location;
	union reg_isp_ae_hist_ir_ae_face0_size  ir_ae_face0_size;
	union reg_isp_ae_hist_ir_ae_face1_size  ir_ae_face1_size;
	union reg_isp_ae_hist_ir_ae_face2_size  ir_ae_face2_size;
	union reg_isp_ae_hist_ir_ae_face3_size  ir_ae_face3_size;
	union reg_isp_ae_hist_ae_face0_enable   ae_face0_enable;
	union reg_isp_ae_hist_ae_face0_sts_div  ae_face0_sts_div;
	union reg_isp_ae_hist_ae_face1_sts_div  ae_face1_sts_div;
	union reg_isp_ae_hist_ae_face2_sts_div  ae_face2_sts_div;
	union reg_isp_ae_hist_ae_face3_sts_div  ae_face3_sts_div;
	union reg_isp_ae_hist_sts_enable        sts_enable;
	union reg_isp_ae_hist_ae_algo_enable    ae_algo_enable;
	union reg_isp_ae_hist_ae_hist_low       ae_hist_low;
	union reg_isp_ae_hist_ae_hist_high      ae_hist_high;
	union reg_isp_ae_hist_ae_top            ae_top;
	union reg_isp_ae_hist_ae_bot            ae_bot;
	union reg_isp_ae_hist_ae_overexp_thr    ae_overexp_thr;
	union reg_isp_ae_hist_ae_num_gapline    ae_num_gapline;
	uint32_t                                _resv_0x124[23];
	union reg_isp_ae_hist_ae_simple2a_result_luma  ae_simple2a_result_luma;
	union reg_isp_ae_hist_ae_simple2a_result_rgain  ae_simple2a_result_rgain;
	union reg_isp_ae_hist_ae_simple2a_result_bgain  ae_simple2a_result_bgain;
	uint32_t                                _resv_0x18c[29];
	union reg_isp_ae_hist_ae_wgt_00         ae_wgt_00;
	union reg_isp_ae_hist_ae_wgt_01         ae_wgt_01;
	union reg_isp_ae_hist_ae_wgt_02         ae_wgt_02;
	union reg_isp_ae_hist_ae_wgt_03         ae_wgt_03;
	union reg_isp_ae_hist_ae_wgt_04         ae_wgt_04;
	union reg_isp_ae_hist_ae_wgt_05         ae_wgt_05;
	union reg_isp_ae_hist_ae_wgt_06         ae_wgt_06;
	union reg_isp_ae_hist_ae_wgt_07         ae_wgt_07;
	union reg_isp_ae_hist_ae_wgt_08         ae_wgt_08;
	union reg_isp_ae_hist_ae_wgt_09         ae_wgt_09;
	union reg_isp_ae_hist_ae_wgt_10         ae_wgt_10;
	union reg_isp_ae_hist_ae_wgt_11         ae_wgt_11;
	union reg_isp_ae_hist_ae_wgt_12         ae_wgt_12;
	union reg_isp_ae_hist_ae_wgt_13         ae_wgt_13;
	union reg_isp_ae_hist_ae_wgt_14         ae_wgt_14;
	union reg_isp_ae_hist_ae_wgt_15         ae_wgt_15;
	union reg_isp_ae_hist_ae_wgt_16         ae_wgt_16;
	union reg_isp_ae_hist_ae_wgt_17         ae_wgt_17;
	union reg_isp_ae_hist_ae_wgt_18         ae_wgt_18;
	union reg_isp_ae_hist_ae_wgt_19         ae_wgt_19;
	union reg_isp_ae_hist_ae_wgt_20         ae_wgt_20;
	union reg_isp_ae_hist_ae_wgt_21         ae_wgt_21;
	union reg_isp_ae_hist_ae_wgt_22         ae_wgt_22;
	union reg_isp_ae_hist_ae_wgt_23         ae_wgt_23;
	union reg_isp_ae_hist_ae_wgt_24         ae_wgt_24;
	union reg_isp_ae_hist_ae_wgt_25         ae_wgt_25;
	union reg_isp_ae_hist_ae_wgt_26         ae_wgt_26;
	union reg_isp_ae_hist_ae_wgt_27         ae_wgt_27;
	union reg_isp_ae_hist_ae_wgt_28         ae_wgt_28;
	union reg_isp_ae_hist_ae_wgt_29         ae_wgt_29;
	union reg_isp_ae_hist_ae_wgt_30         ae_wgt_30;
	union reg_isp_ae_hist_ae_wgt_31         ae_wgt_31;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_gms_t {
	union reg_isp_gms_status                gms_status;
	union reg_isp_gms_grace_reset           gms_grace_reset;
	union reg_isp_gms_monitor               gms_monitor;
	union reg_isp_gms_enable                gms_enable;
	uint32_t                                _resv_0x10[1];
	union reg_isp_gms_flow                  gms_flow;
	union reg_isp_gms_start_x               gms_start_x;
	union reg_isp_gms_start_y               gms_start_y;
	union reg_isp_gms_location              gms_location;
	uint32_t                                _resv_0x24[1];
	union reg_isp_gms_x_sizem1              gms_x_sizem1;
	union reg_isp_gms_y_sizem1              gms_y_sizem1;
	union reg_isp_gms_x_gap                 gms_x_gap;
	union reg_isp_gms_y_gap                 gms_y_gap;
	union reg_isp_gms_dummy                 gms_dummy;
	uint32_t                                _resv_0x3c[1];
	union reg_isp_gms_mem_sw_mode           gms_sw_mode;
	union reg_isp_gms_mem_sw_raddr          gms_sw_raddr;
	union reg_isp_gms_mem_sw_rdata          gms_sw_rdata;
	union reg_isp_gms_monitor_select        gms_monitor_select;
	uint32_t                                _resv_0x50[20];
	union reg_isp_gms_dmi_enable            dmi_enable;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_af_t {
	union reg_isp_af_status                 status;
	union reg_isp_af_grace_reset            grace_reset;
	union reg_isp_af_monitor                monitor;
	union reg_isp_af_bypass                 bypass;
	union reg_isp_af_kickoff                kickoff;
	union reg_isp_af_enables                enables;
	union reg_isp_af_offset_x               offset_x;
	union reg_isp_af_mxn_image_width_m1     mxn_image_width_m1;
	union reg_isp_af_block_width            block_width;
	union reg_isp_af_block_height           block_height;
	union reg_isp_af_block_num_x            block_num_x;
	union reg_isp_af_block_num_y            block_num_y;
	uint32_t                                _resv_0x30[1];
	union reg_isp_af_hor_low_pass_value_shift  hor_low_pass_value_shift;
	union reg_isp_af_corning_offset_horizontal_0  offset_horizontal_0;
	union reg_isp_af_corning_offset_horizontal_1  offset_horizontal_1;
	union reg_isp_af_corning_offset_vertical  offset_vertical;
	union reg_isp_af_high_y_thre            high_y_thre;
	union reg_isp_af_low_pass_horizon       low_pass_horizon;
	union reg_isp_af_location               location;
	union reg_isp_af_high_pass_horizon_0    high_pass_horizon_0;
	union reg_isp_af_high_pass_horizon_1    high_pass_horizon_1;
	union reg_isp_af_high_pass_vertical_0   high_pass_vertical_0;
	union reg_isp_af_mem_sw_mode            sw_mode;
	union reg_isp_af_monitor_select         monitor_select;
	uint32_t                                _resv_0x64[2];
	union reg_isp_af_image_width            image_width;
	union reg_isp_af_dummy                  dummy;
	union reg_isp_af_mem_sw_raddr           sw_raddr;
	union reg_isp_af_mem_sw_rdata           sw_rdata;
	union reg_isp_af_mxn_border             mxn_border;
	union reg_isp_af_th_low                 th_low;
	union reg_isp_af_gain_low               gain_low;
	union reg_isp_af_slop_low               slop_low;
	uint32_t                                _resv_0x8c[5];
	union reg_isp_af_dmi_enable             dmi_enable;
	uint32_t                                _resv_0xa4[45];
	union reg_isp_af_square_enable          square_enable;
	uint32_t                                _resv_0x15c[2];
	union reg_isp_af_outshift               outshift;
	uint32_t                                _resv_0x168[1];
	union reg_isp_af_num_gapline            num_gapline;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_pre_raw_fe_t {
	union reg_pre_raw_fe_pre_raw_ctrl       pre_raw_ctrl;
	union reg_pre_raw_fe_pre_raw_frame_size  pre_raw_frame_size;
	uint32_t                                _resv_0x8[2];
	union reg_pre_raw_fe_le_rgbmap_grid_number  le_rgbmap_grid_number;
	union reg_pre_raw_fe_se_rgbmap_grid_number  se_rgbmap_grid_number;
	uint32_t                                _resv_0x18[2];
	union reg_pre_raw_fe_pre_raw_post_no_rspd_cyc  pre_raw_post_no_rspd_cyc;
	union reg_pre_raw_fe_pre_raw_post_rgbmap_no_rspd_cyc  pre_raw_post_rgbmap_no_rspd_cyc;
	union reg_pre_raw_fe_pre_raw_frame_vld  pre_raw_frame_vld;
	union reg_pre_raw_fe_pre_raw_debug_state  pre_raw_debug_state;
	union reg_pre_raw_fe_pre_raw_dummy      pre_raw_dummy;
	union reg_pre_raw_fe_pre_raw_debug_info  pre_raw_info;
	uint32_t                                _resv_0x38[6];
	union reg_pre_raw_fe_idle_info          fe_idle_info;
	uint32_t                                _resv_0x54[3];
	union reg_pre_raw_fe_check_sum          fe_check_sum;
	union reg_pre_raw_fe_check_sum_value    fe_check_sum_value;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_csi_bdg_dvp_t {
	union reg_isp_csi_bdg_dvp_bdg_top_ctrl  csi_bdg_top_ctrl;
	union reg_isp_csi_bdg_dvp_bdg_interrupt_ctrl  csi_bdg_interrupt_ctrl;
	union reg_isp_csi_bdg_dvp_bdg_dma_dpcm_mode  csi_bdg_dma_dpcm_mode;
	union reg_isp_csi_bdg_dvp_bdg_dma_ld_dpcm_mode  csi_bdg_dma_ld_dpcm_mode;
	union reg_isp_csi_bdg_dvp_ch0_size      ch0_size;
	uint32_t                                _resv_0x14[3];
	union reg_isp_csi_bdg_dvp_ch0_crop_en   ch0_crop_en;
	union reg_isp_csi_bdg_dvp_ch0_horz_crop  ch0_horz_crop;
	union reg_isp_csi_bdg_dvp_ch0_vert_crop  ch0_vert_crop;
	union reg_isp_csi_bdg_dvp_ch0_blc_sum   ch0_blc_sum;
	uint32_t                                _resv_0x30[12];
	union reg_isp_csi_bdg_dvp_pat_gen_ctrl  csi_pat_gen_ctrl;
	union reg_isp_csi_bdg_dvp_pat_idx_ctrl  csi_pat_idx_ctrl;
	union reg_isp_csi_bdg_dvp_pat_color_0   csi_pat_color_0;
	union reg_isp_csi_bdg_dvp_pat_color_1   csi_pat_color_1;
	union reg_isp_csi_bdg_dvp_background_color_0  csi_background_color_0;
	union reg_isp_csi_bdg_dvp_background_color_1  csi_background_color_1;
	union reg_isp_csi_bdg_dvp_fix_color_0   csi_fix_color_0;
	union reg_isp_csi_bdg_dvp_fix_color_1   csi_fix_color_1;
	union reg_isp_csi_bdg_dvp_mde_v_size    csi_mde_v_size;
	union reg_isp_csi_bdg_dvp_mde_h_size    csi_mde_h_size;
	union reg_isp_csi_bdg_dvp_fde_v_size    csi_fde_v_size;
	union reg_isp_csi_bdg_dvp_fde_h_size    csi_fde_h_size;
	union reg_isp_csi_bdg_dvp_hsync_ctrl    csi_hsync_ctrl;
	union reg_isp_csi_bdg_dvp_vsync_ctrl    csi_vsync_ctrl;
	union reg_isp_csi_bdg_dvp_tgen_tt_size  csi_tgen_tt_size;
	union reg_isp_csi_bdg_dvp_line_intp_height_0  line_intp_height_0;
	union reg_isp_csi_bdg_dvp_ch0_debug_0   ch0_debug_0;
	union reg_isp_csi_bdg_dvp_ch0_debug_1   ch0_debug_1;
	union reg_isp_csi_bdg_dvp_ch0_debug_2   ch0_debug_2;
	union reg_isp_csi_bdg_dvp_ch0_debug_3   ch0_debug_3;
	uint32_t                                _resv_0xb0[12];
	union reg_isp_csi_bdg_dvp_interrupt_status_0  interrupt_status_0;
	union reg_isp_csi_bdg_dvp_interrupt_status_1  interrupt_status_1;
	union reg_isp_csi_bdg_dvp_bdg_debug     bdg_debug;
	union reg_isp_csi_bdg_dvp_out_vsync_line_delay  csi_out_vsync_line_delay;
	union reg_isp_csi_bdg_dvp_wr_urgent_ctrl  csi_wr_urgent_ctrl;
	union reg_isp_csi_bdg_dvp_rd_urgent_ctrl  csi_rd_urgent_ctrl;
	union reg_isp_csi_bdg_dvp_dummy         csi_dummy;
	uint32_t                                _resv_0xfc[1];
	union reg_isp_csi_bdg_dvp_slice_line_intp_height_0  slice_line_intp_height_0;
	uint32_t                                _resv_0x104[3];
	union reg_isp_csi_bdg_dvp_wdma_ch0_crop_en  wdma_ch0_crop_en;
	union reg_isp_csi_bdg_dvp_wdma_ch0_horz_crop  wdma_ch0_horz_crop;
	union reg_isp_csi_bdg_dvp_wdma_ch0_vert_crop  wdma_ch0_vert_crop;
	uint32_t                                _resv_0x11c[13];
	union reg_isp_csi_bdg_dvp_trig_dly_control_0  trig_dly_control_0;
	union reg_isp_csi_bdg_dvp_trig_dly_control_1  trig_dly_control_1;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_crop_t {
	union reg_crop_0                        reg_0;
	union reg_crop_1                        reg_1;
	union reg_crop_2                        reg_2;
	union reg_crop_3                        reg_3;
	union reg_crop_dummy                    dummy;
	union reg_crop_debug                    debug;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_blc_t {
	union reg_isp_blc_0                     blc_0;
	union reg_isp_blc_1                     blc_1;
	union reg_isp_blc_2                     blc_2;
	union reg_isp_blc_3                     blc_3;
	union reg_isp_blc_4                     blc_4;
	union reg_isp_blc_5                     blc_5;
	union reg_isp_blc_6                     blc_6;
	union reg_isp_blc_7                     blc_7;
	union reg_isp_blc_8                     blc_8;
	uint32_t                                _resv_0x24[1];
	union reg_isp_blc_dummy                 blc_dummy;
	uint32_t                                _resv_0x2c[1];
	union reg_isp_blc_location              blc_location;
	union reg_isp_blc_9                     blc_9;
	union reg_isp_blc_a                     blc_a;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_lmap_t {
	union reg_isp_lmap_lmp_0                lmp_0;
	union reg_isp_lmap_lmp_1                lmp_1;
	union reg_isp_lmap_lmp_2                lmp_2;
	union reg_isp_lmap_lmp_debug_0          lmp_debug_0;
	union reg_isp_lmap_lmp_debug_1          lmp_debug_1;
	union reg_isp_lmap_dummy                dummy;
	union reg_isp_lmap_lmp_debug_2          lmp_debug_2;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_lmap_lmp_3                lmp_3;
	union reg_isp_lmap_lmp_4                lmp_4;
	union reg_isp_lmap_lmp_set_sel          lmp_set_sel;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_rgbmap_t {
	union reg_isp_rgbmap_0                  rgbmap_0;
	union reg_isp_rgbmap_1                  rgbmap_1;
	union reg_isp_rgbmap_debug_0            rgbmap_debug_0;
	union reg_isp_rgbmap_debug_1            rgbmap_debug_1;
	union reg_isp_rgbmap_dummy              dummy;
	union reg_isp_rgbmap_2                  rgbmap_2;
	union reg_isp_rgbmap_debug_2            rgbmap_debug_2;
	union reg_isp_rgbmap_3                  rgbmap_3;
	union reg_isp_rgbmap_set_sel            rgbmap_set_sel;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_wbg_t {
	union reg_isp_wbg_0                     wbg_0;
	union reg_isp_wbg_1                     wbg_1;
	union reg_isp_wbg_2                     wbg_2;
	uint32_t                                _resv_0xc[1];
	union reg_isp_wbg_4                     wbg_4;
	union reg_isp_wbg_5                     wbg_5;
	union reg_isp_wbg_6                     wbg_6;
	union reg_isp_wbg_7                     wbg_7;
	uint32_t                                _resv_0x20[1];
	union reg_isp_wbg_img_bayerid           img_bayerid;
	union reg_isp_wbg_dummy                 wbg_dummy;
	uint32_t                                _resv_0x2c[1];
	union reg_isp_wbg_location              wbg_location;
	union reg_isp_wbg_34                    wbg_34;
	union reg_isp_wbg_38                    wbg_38;
	union reg_isp_wbg_3c                    wbg_3c;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_dpc_t {
	union reg_isp_dpc_0                     dpc_0;
	union reg_isp_dpc_1                     dpc_1;
	union reg_isp_dpc_2                     dpc_2;
	union reg_isp_dpc_3                     dpc_3;
	union reg_isp_dpc_4                     dpc_4;
	union reg_isp_dpc_5                     dpc_5;
	union reg_isp_dpc_6                     dpc_6;
	union reg_isp_dpc_7                     dpc_7;
	union reg_isp_dpc_8                     dpc_8;
	union reg_isp_dpc_9                     dpc_9;
	union reg_isp_dpc_10                    dpc_10;
	union reg_isp_dpc_11                    dpc_11;
	union reg_isp_dpc_12                    dpc_12;
	union reg_isp_dpc_13                    dpc_13;
	union reg_isp_dpc_14                    dpc_14;
	union reg_isp_dpc_15                    dpc_15;
	union reg_isp_dpc_16                    dpc_16;
	union reg_isp_dpc_17                    dpc_17;
	union reg_isp_dpc_18                    dpc_18;
	union reg_isp_dpc_19                    dpc_19;
	union reg_isp_dpc_mem_w0                dpc_mem_w0;
	union reg_isp_dpc_window                dpc_window;
	union reg_isp_dpc_mem_st_addr           dpc_mem_st_addr;
	uint32_t                                _resv_0x5c[1];
	union reg_isp_dpc_checksum              dpc_checksum;
	union reg_isp_dpc_int                   dpc_int;
	uint32_t                                _resv_0x68[2];
	union reg_isp_dpc_20                    dpc_20;
	union reg_isp_dpc_21                    dpc_21;
	union reg_isp_dpc_22                    dpc_22;
	union reg_isp_dpc_23                    dpc_23;
	union reg_isp_dpc_24                    dpc_24;
	union reg_isp_dpc_25                    dpc_25;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_raw_top_t {
	union reg_raw_top_raw_0                 raw_0;
	union reg_raw_top_read_sel              read_sel;
	union reg_raw_top_raw_1                 raw_1;
	uint32_t                                _resv_0xc[1];
	union reg_raw_top_ctrl                  ctrl;
	union reg_raw_top_up_pq_en              up_pq_en;
	union reg_raw_top_raw_2                 raw_2;
	union reg_raw_top_dummy                 dummy;
	union reg_raw_top_raw_4                 raw_4;
	union reg_raw_top_status                status;
	union reg_raw_top_debug                 debug;
	union reg_raw_top_debug_select          debug_select;
	union reg_raw_top_raw_bayer_type_topleft  raw_bayer_type_topleft;
	union reg_raw_top_rdmi_enable           rdmi_enable;
	union reg_raw_top_rdma_size             rdma_size;
	union reg_raw_top_dpcm_mode             dpcm_mode;
	union reg_raw_top_stvalid_status        stvalid_status;
	union reg_raw_top_stready_status        stready_status;
	union reg_raw_top_patgen1               patgen1;
	union reg_raw_top_patgen2               patgen2;
	union reg_raw_top_patgen3               patgen3;
	union reg_raw_top_patgen4               patgen4;
	union reg_raw_top_ro_idle               ro_idle;
	union reg_raw_top_ro_done               ro_done;
	union reg_raw_top_dma_idle              dma_idle;
	uint32_t                                _resv_0x64[1];
	union reg_raw_top_le_lmap_grid_number   le_lmap_grid_number;
	union reg_raw_top_se_lmap_grid_number   se_lmap_grid_number;
	union reg_raw_top_checksum_0            checksum_0;
	union reg_raw_top_checksum_1            checksum_1;
	union reg_raw_top_patgen5               patgen5;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_rgbcac_t {
	union reg_isp_rgbcac_ctrl               rgbcac_ctrl;
	union reg_isp_rgbcac_purple_th          rgbcac_purple_th;
	union reg_isp_rgbcac_purple_cbcr        rgbcac_purple_cbcr;
	union reg_isp_rgbcac_purple_cbcr2       rgbcac_purple_cbcr2;
	union reg_isp_rgbcac_purple_cbcr3       rgbcac_purple_cbcr3;
	union reg_isp_rgbcac_green_cbcr         rgbcac_green_cbcr;
	union reg_isp_rgbcac_edge_coring        rgbcac_edge_coring;
	union reg_isp_rgbcac_depurple_str_ratio_min  rgbcac_depurple_str_ratio_min;
	union reg_isp_rgbcac_depurple_str_ratio_max  rgbcac_depurple_str_ratio_max;
	union reg_isp_rgbcac_edge_wgt_lut0      rgbcac_edge_wgt_lut0;
	union reg_isp_rgbcac_edge_wgt_lut1      rgbcac_edge_wgt_lut1;
	union reg_isp_rgbcac_edge_wgt_lut2      rgbcac_edge_wgt_lut2;
	union reg_isp_rgbcac_edge_wgt_lut3      rgbcac_edge_wgt_lut3;
	union reg_isp_rgbcac_edge_wgt_lut4      rgbcac_edge_wgt_lut4;
	union reg_isp_rgbcac_luma               rgbcac_luma;
	union reg_isp_rgbcac_luma_blend         rgbcac_luma_blend;
	union reg_isp_rgbcac_luma_filter0       rgbcac_luma_filter0;
	union reg_isp_rgbcac_luma_filter1       rgbcac_luma_filter1;
	union reg_isp_rgbcac_var_filter0        rgbcac_var_filter0;
	union reg_isp_rgbcac_var_filter1        rgbcac_var_filter1;
	union reg_isp_rgbcac_chroma_filter0     rgbcac_chroma_filter0;
	union reg_isp_rgbcac_chroma_filter1     rgbcac_chroma_filter1;
	union reg_isp_rgbcac_cbcr_str           rgbcac_cbcr_str;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_bnr_t {
	union reg_isp_bnr_shadow_rd_sel         shadow_rd_sel;
	union reg_isp_bnr_out_sel               out_sel;
	union reg_isp_bnr_index_clr             index_clr;
	uint32_t                                _resv_0xc[61];
	union reg_isp_bnr_ns_luma_th_r          ns_luma_th_r;
	union reg_isp_bnr_ns_slope_r            ns_slope_r;
	union reg_isp_bnr_ns_offset0_r          ns_offset0_r;
	union reg_isp_bnr_ns_offset1_r          ns_offset1_r;
	union reg_isp_bnr_ns_luma_th_gr         ns_luma_th_gr;
	union reg_isp_bnr_ns_slope_gr           ns_slope_gr;
	union reg_isp_bnr_ns_offset0_gr         ns_offset0_gr;
	union reg_isp_bnr_ns_offset1_gr         ns_offset1_gr;
	union reg_isp_bnr_ns_luma_th_gb         ns_luma_th_gb;
	union reg_isp_bnr_ns_slope_gb           ns_slope_gb;
	union reg_isp_bnr_ns_offset0_gb         ns_offset0_gb;
	union reg_isp_bnr_ns_offset1_gb         ns_offset1_gb;
	union reg_isp_bnr_ns_luma_th_b          ns_luma_th_b;
	union reg_isp_bnr_ns_slope_b            ns_slope_b;
	union reg_isp_bnr_ns_offset0_b          ns_offset0_b;
	union reg_isp_bnr_ns_offset1_b          ns_offset1_b;
	union reg_isp_bnr_ns_gain               ns_gain;
	union reg_isp_bnr_strength_mode         strength_mode;
	union reg_isp_bnr_intensity_sel         intensity_sel;
	uint32_t                                _resv_0x14c[45];
	union reg_isp_bnr_weight_intra_0        weight_intra_0;
	union reg_isp_bnr_weight_intra_1        weight_intra_1;
	union reg_isp_bnr_weight_intra_2        weight_intra_2;
	uint32_t                                _resv_0x20c[1];
	union reg_isp_bnr_weight_norm_1         weight_norm_1;
	union reg_isp_bnr_weight_norm_2         weight_norm_2;
	uint32_t                                _resv_0x218[3];
	union reg_isp_bnr_var_th                var_th;
	union reg_isp_bnr_weight_lut            weight_lut;
	union reg_isp_bnr_weight_sm             weight_sm;
	union reg_isp_bnr_weight_v              weight_v;
	union reg_isp_bnr_weight_h              weight_h;
	union reg_isp_bnr_weight_d45            weight_d45;
	union reg_isp_bnr_weight_d135           weight_d135;
	union reg_isp_bnr_neighbor_max          neighbor_max;
	uint32_t                                _resv_0x244[3];
	union reg_isp_bnr_res_k_smooth          res_k_smooth;
	union reg_isp_bnr_res_k_texture         res_k_texture;
	union reg_isp_bnr_res_max               res_max;
	uint32_t                                _resv_0x25c[872];
	union reg_isp_bnr_dummy                 dummy;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_ca_t {
	union reg_ca_00                         reg_00;
	union reg_ca_04                         reg_04;
	union reg_ca_08                         reg_08;
	union reg_ca_0c                         reg_0c;
	union reg_ca_10                         reg_10;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_ccm_t {
	union reg_isp_ccm_00                    ccm_00;
	union reg_isp_ccm_01                    ccm_01;
	union reg_isp_ccm_02                    ccm_02;
	union reg_isp_ccm_10                    ccm_10;
	union reg_isp_ccm_11                    ccm_11;
	union reg_isp_ccm_12                    ccm_12;
	union reg_isp_ccm_20                    ccm_20;
	union reg_isp_ccm_21                    ccm_21;
	union reg_isp_ccm_22                    ccm_22;
	union reg_isp_ccm_ctrl                  ccm_ctrl;
	union reg_isp_ccm_dbg                   ccm_dbg;
	uint32_t                                _resv_0x2c[1];
	union reg_isp_ccm_dmy0                  dmy0;
	union reg_isp_ccm_dmy1                  dmy1;
	union reg_isp_ccm_dmy_r                 dmy_r;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_mmap_t {
	union reg_isp_mmap_00                   reg_00;
	union reg_isp_mmap_04                   reg_04;
	union reg_isp_mmap_08                   reg_08;
	union reg_isp_mmap_0c                   reg_0c;
	union reg_isp_mmap_10                   reg_10;
	union reg_isp_mmap_14                   reg_14;
	union reg_isp_mmap_18                   reg_18;
	union reg_isp_mmap_1c                   reg_1c;
	union reg_isp_mmap_20                   reg_20;
	union reg_isp_mmap_24                   reg_24;
	union reg_isp_mmap_28                   reg_28;
	union reg_isp_mmap_2c                   reg_2c;
	union reg_isp_mmap_30                   reg_30;
	union reg_isp_mmap_34                   reg_34;
	union reg_isp_mmap_38                   reg_38;
	union reg_isp_mmap_3c                   reg_3c;
	union reg_isp_mmap_40                   reg_40;
	union reg_isp_mmap_44                   reg_44;
	uint32_t                                _resv_0x48[1];
	union reg_isp_mmap_4c                   reg_4c;
	union reg_isp_mmap_50                   reg_50;
	union reg_isp_mmap_54                   reg_54;
	union reg_isp_mmap_58                   reg_58;
	union reg_isp_mmap_5c                   reg_5c;
	union reg_isp_mmap_60                   reg_60;
	union reg_isp_mmap_64                   reg_64;
	union reg_isp_mmap_68                   reg_68;
	union reg_isp_mmap_6c                   reg_6c;
	union reg_isp_mmap_70                   reg_70;
	union reg_isp_mmap_74                   reg_74;
	union reg_isp_mmap_78                   reg_78;
	union reg_isp_mmap_7c                   reg_7c;
	union reg_isp_mmap_80                   reg_80;
	union reg_isp_mmap_84                   reg_84;
	union reg_isp_mmap_88                   reg_88;
	union reg_isp_mmap_8c                   reg_8c;
	union reg_isp_mmap_90                   reg_90;
	uint32_t                                _resv_0x94[3];
	union reg_isp_mmap_a0                   reg_a0;
	union reg_isp_mmap_a4                   reg_a4;
	union reg_isp_mmap_a8                   reg_a8;
	union reg_isp_mmap_ac                   reg_ac;
	union reg_isp_mmap_b0                   reg_b0;
	union reg_isp_mmap_b4                   reg_b4;
	union reg_isp_mmap_b8                   reg_b8;
	union reg_isp_mmap_bc                   reg_bc;
	union reg_isp_mmap_c0                   reg_c0;
	union reg_isp_mmap_c4                   reg_c4;
	union reg_isp_mmap_c8                   reg_c8;
	union reg_isp_mmap_cc                   reg_cc;
	union reg_isp_mmap_d0                   reg_d0;
	union reg_isp_mmap_d4                   reg_d4;
	union reg_isp_mmap_d8                   reg_d8;
	union reg_isp_mmap_dc                   reg_dc;
	union reg_isp_mmap_e0                   reg_e0;
	union reg_isp_mmap_e4                   reg_e4;
	union reg_isp_mmap_e8                   reg_e8;
	union reg_isp_mmap_ec                   reg_ec;
	union reg_isp_mmap_f0                   reg_f0;
	union reg_isp_mmap_f4                   reg_f4;
	union reg_isp_mmap_f8                   reg_f8;
	union reg_isp_mmap_fc                   reg_fc;
	union reg_isp_mmap_100                  reg_100;
	union reg_isp_mmap_104                  reg_104;
	union reg_isp_mmap_108                  reg_108;
	union reg_isp_mmap_10c                  reg_10c;
	union reg_isp_mmap_110                  reg_110;
	union reg_isp_mmap_114                  reg_114;
	union reg_isp_mmap_118                  reg_118;
	union reg_isp_mmap_11c                  reg_11c;
	union reg_isp_mmap_120                  reg_120;
	union reg_isp_mmap_124                  reg_124;
	union reg_isp_mmap_128                  reg_128;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_gamma_t {
	union reg_isp_gamma_ctrl                gamma_ctrl;
	union reg_isp_gamma_prog_ctrl           gamma_prog_ctrl;
	union reg_isp_gamma_prog_st_addr        gamma_prog_st_addr;
	union reg_isp_gamma_prog_data           gamma_prog_data;
	union reg_isp_gamma_prog_max            gamma_prog_max;
	union reg_isp_gamma_mem_sw_raddr        gamma_sw_raddr;
	union reg_isp_gamma_mem_sw_rdata        gamma_sw_rdata;
	union reg_isp_gamma_mem_sw_rdata_bg     gamma_sw_rdata_bg;
	union reg_isp_gamma_dbg                 gamma_dbg;
	union reg_isp_gamma_dmy0                gamma_dmy0;
	union reg_isp_gamma_dmy1                gamma_dmy1;
	union reg_isp_gamma_dmy_r               gamma_dmy_r;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_clut_t {
	union reg_isp_clut_ctrl                 clut_ctrl;
	union reg_isp_clut_prog_addr            clut_prog_addr;
	union reg_isp_clut_prog_data            clut_prog_data;
	union reg_isp_clut_prog_rdata           clut_prog_rdata;
	uint32_t                                _resv_0x10[4];
	union reg_isp_clut_dbg                  clut_dbg;
	union reg_isp_clut_dmy0                 clut_dmy0;
	union reg_isp_clut_dmy1                 clut_dmy1;
	union reg_isp_clut_dmy_r                clut_dmy_r;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_dehaze_t {
	union reg_isp_dehaze_dhz_smooth         dhz_smooth;
	union reg_isp_dehaze_dhz_skin           dhz_skin;
	union reg_isp_dehaze_dhz_wgt            dhz_wgt;
	uint32_t                                _resv_0xc[2];
	union reg_isp_dehaze_dhz_bypass         dhz_bypass;
	union reg_isp_dehaze_0                  reg_0;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_dehaze_1                  reg_1;
	union reg_isp_dehaze_2                  reg_2;
	union reg_isp_dehaze_28                 reg_28;
	union reg_isp_dehaze_2c                 reg_2c;
	union reg_isp_dehaze_3                  reg_3;
	union reg_isp_dehaze_5                  reg_5;
	union reg_isp_dehaze_6                  reg_6;
	union reg_isp_dehaze_7                  reg_7;
	union reg_isp_dehaze_8                  reg_8;
	uint32_t                                _resv_0x44[3];
	union reg_isp_dehaze_9                  reg_9;
	union reg_isp_dehaze_10                 reg_10;
	union reg_isp_dehaze_11                 reg_11;
	union reg_isp_dehaze_12                 reg_12;
	union reg_isp_dehaze_17                 reg_17;
	union reg_isp_dehaze_18                 reg_18;
	union reg_isp_dehaze_19                 reg_19;
	union reg_isp_dehaze_20                 reg_20;
	union reg_isp_dehaze_25                 reg_25;
	union reg_isp_dehaze_26                 reg_26;
	union reg_isp_dehaze_tmap_00            tmap_00;
	union reg_isp_dehaze_tmap_01            tmap_01;
	union reg_isp_dehaze_tmap_02            tmap_02;
	union reg_isp_dehaze_tmap_03            tmap_03;
	union reg_isp_dehaze_tmap_04            tmap_04;
	union reg_isp_dehaze_tmap_05            tmap_05;
	union reg_isp_dehaze_tmap_06            tmap_06;
	union reg_isp_dehaze_tmap_07            tmap_07;
	union reg_isp_dehaze_tmap_08            tmap_08;
	union reg_isp_dehaze_tmap_09            tmap_09;
	union reg_isp_dehaze_tmap_10            tmap_10;
	union reg_isp_dehaze_tmap_11            tmap_11;
	union reg_isp_dehaze_tmap_12            tmap_12;
	union reg_isp_dehaze_tmap_13            tmap_13;
	union reg_isp_dehaze_tmap_14            tmap_14;
	union reg_isp_dehaze_tmap_15            tmap_15;
	union reg_isp_dehaze_tmap_16            tmap_16;
	union reg_isp_dehaze_tmap_17            tmap_17;
	union reg_isp_dehaze_tmap_18            tmap_18;
	union reg_isp_dehaze_tmap_19            tmap_19;
	union reg_isp_dehaze_tmap_20            tmap_20;
	union reg_isp_dehaze_tmap_21            tmap_21;
	union reg_isp_dehaze_tmap_22            tmap_22;
	union reg_isp_dehaze_tmap_23            tmap_23;
	union reg_isp_dehaze_tmap_24            tmap_24;
	union reg_isp_dehaze_tmap_25            tmap_25;
	union reg_isp_dehaze_tmap_26            tmap_26;
	union reg_isp_dehaze_tmap_27            tmap_27;
	union reg_isp_dehaze_tmap_28            tmap_28;
	union reg_isp_dehaze_tmap_29            tmap_29;
	union reg_isp_dehaze_tmap_30            tmap_30;
	union reg_isp_dehaze_tmap_31            tmap_31;
	union reg_isp_dehaze_tmap_32            tmap_32;
	union reg_isp_dehaze_o_dc_th_up_mode    o_dc_th_up_mode;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_csc_t {
	union reg_isp_csc_0                     reg_0;
	union reg_isp_csc_1                     reg_1;
	union reg_isp_csc_2                     reg_2;
	union reg_isp_csc_3                     reg_3;
	union reg_isp_csc_4                     reg_4;
	union reg_isp_csc_5                     reg_5;
	union reg_isp_csc_6                     reg_6;
	union reg_isp_csc_7                     reg_7;
	union reg_isp_csc_8                     reg_8;
	union reg_isp_csc_9                     reg_9;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_rgb_dither_t {
	union reg_isp_rgb_dither_rgb_dither     rgb_dither;
	union reg_isp_rgb_dither_rgb_dither_debug0  rgb_dither_debug0;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_dci_t {
	union reg_isp_dci_status                dci_status;
	union reg_isp_dci_grace_reset           dci_grace_reset;
	union reg_isp_dci_monitor               dci_monitor;
	union reg_isp_dci_enable                dci_enable;
	union reg_isp_dci_map_enable            dci_map_enable;
	union reg_isp_dci_flow                  dci_flow;
	union reg_isp_dci_demo_mode             dci_demo_mode;
	union reg_isp_dci_monitor_select        dci_monitor_select;
	union reg_isp_dci_location              dci_location;
	uint32_t                                _resv_0x24[1];
	union reg_isp_dci_prog_data             dci_prog_data;
	union reg_isp_dci_prog_ctrl             dci_prog_ctrl;
	union reg_isp_dci_prog_max              dci_prog_max;
	union reg_isp_dci_ctrl                  dci_ctrl;
	union reg_isp_dci_mem_sw_mode           dci_sw_mode;
	union reg_isp_dci_mem_raddr             dci_mem_raddr;
	union reg_isp_dci_mem_rdata             dci_mem_rdata;
	union reg_isp_dci_debug                 dci_debug;
	union reg_isp_dci_dummy                 dci_dummy;
	union reg_isp_dci_img_widthm1           img_widthm1;
	union reg_isp_dci_lut_order_select      dci_lut_order_select;
	union reg_isp_dci_roi_start             dci_roi_start;
	union reg_isp_dci_roi_geo               dci_roi_geo;
	uint32_t                                _resv_0x5c[1];
	union reg_isp_dci_uv_gain_max           dci_uv_gain_max;
	uint32_t                                _resv_0x64[7];
	union reg_isp_dci_map_dbg               dci_map_dbg;
	uint32_t                                _resv_0x84[1];
	union reg_isp_dci_bayer_starting        dci_bayer_starting;
	uint32_t                                _resv_0x8c[5];
	union reg_isp_dci_dmi_enable            dmi_enable;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_hist_edge_v_t {
	union reg_isp_hist_edge_v_status        status;
	union reg_isp_hist_edge_v_sw_ctl        sw_ctl;
	union reg_isp_hist_edge_v_bypass        bypass;
	union reg_isp_hist_edge_v_ip_config     ip_config;
	union reg_isp_hist_edge_v_hist_edge_v_offsetx  hist_edge_v_offsetx;
	union reg_isp_hist_edge_v_hist_edge_v_offsety  hist_edge_v_offsety;
	union reg_isp_hist_edge_v_monitor       monitor;
	union reg_isp_hist_edge_v_monitor_select  monitor_select;
	union reg_isp_hist_edge_v_location      location;
	union reg_isp_hist_edge_v_dummy         dummy;
	union reg_isp_hist_edge_v_dmi_enable    dmi_enable;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_fusion_t {
	union reg_fusion_fs_ctrl_0              fs_ctrl_0;
	uint32_t                                _resv_0x4[1];
	union reg_fusion_fs_se_gain             fs_se_gain;
	union reg_fusion_fs_luma_thd            fs_luma_thd;
	union reg_fusion_fs_wgt                 fs_wgt;
	union reg_fusion_fs_wgt_slope           fs_wgt_slope;
	union reg_fusion_fs_shdw_read_sel       fs_shdw_read_sel;
	uint32_t                                _resv_0x1c[1];
	union reg_fusion_fs_motion_lut_in       fs_motion_lut_in;
	union reg_fusion_fs_motion_lut_out_0    fs_motion_lut_out_0;
	union reg_fusion_fs_motion_lut_out_1    fs_motion_lut_out_1;
	union reg_fusion_fs_motion_lut_slope_0  fs_motion_lut_slope_0;
	union reg_fusion_fs_motion_lut_slope_1  fs_motion_lut_slope_1;
	union reg_fusion_fs_ctrl_1              fs_ctrl_1;
	uint32_t                                _resv_0x38[6];
	union reg_fusion_fs_calib_ctrl_0        fs_calib_ctrl_0;
	union reg_fusion_fs_calib_ctrl_1        fs_calib_ctrl_1;
	union reg_fusion_fs_se_fix_offset_0     fs_se_fix_offset_0;
	union reg_fusion_fs_se_fix_offset_1     fs_se_fix_offset_1;
	union reg_fusion_fs_se_fix_offset_2     fs_se_fix_offset_2;
	union reg_fusion_fs_calib_out_0         fs_calib_out_0;
	union reg_fusion_fs_calib_out_1         fs_calib_out_1;
	union reg_fusion_fs_calib_out_2         fs_calib_out_2;
	union reg_fusion_fs_calib_out_3         fs_calib_out_3;
	union reg_fusion_fs_lmap_dark_thd       fs_lmap_dark_thd;
	union reg_fusion_fs_lmap_dark_wgt       fs_lmap_dark_wgt;
	union reg_fusion_fs_lmap_dark_wgt_slope  fs_lmap_dark_wgt_slope;
	union reg_fusion_fs_lmap_brit_thd       fs_lmap_brit_thd;
	union reg_fusion_fs_lmap_brit_wgt       fs_lmap_brit_wgt;
	union reg_fusion_fs_lmap_brit_wgt_slope  fs_lmap_brit_wgt_slope;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_ltm_t {
	union reg_ltm_h00                       reg_h00;
	union reg_ltm_h04                       reg_h04;
	union reg_ltm_h08                       reg_h08;
	union reg_ltm_h0c                       reg_h0c;
	uint32_t                                _resv_0x10[1];
	union reg_ltm_h14                       reg_h14;
	union reg_ltm_h18                       reg_h18;
	union reg_ltm_h1c                       reg_h1c;
	union reg_ltm_h20                       reg_h20;
	union reg_ltm_h24                       reg_h24;
	union reg_ltm_h28                       reg_h28;
	union reg_ltm_h2c                       reg_h2c;
	union reg_ltm_h30                       reg_h30;
	union reg_ltm_h34                       reg_h34;
	union reg_ltm_h38                       reg_h38;
	union reg_ltm_h3c                       reg_h3c;
	union reg_ltm_h40                       reg_h40;
	union reg_ltm_h44                       reg_h44;
	union reg_ltm_h48                       reg_h48;
	union reg_ltm_h4c                       reg_h4c;
	union reg_ltm_h50                       reg_h50;
	union reg_ltm_h54                       reg_h54;
	union reg_ltm_h58                       reg_h58;
	union reg_ltm_h5c                       reg_h5c;
	union reg_ltm_h60                       reg_h60;
	union reg_ltm_h64                       reg_h64;
	union reg_ltm_h68                       reg_h68;
	union reg_ltm_h6c                       reg_h6c;
	union reg_ltm_h70                       reg_h70;
	union reg_ltm_h74                       reg_h74;
	union reg_ltm_h78                       reg_h78;
	union reg_ltm_h7c                       reg_h7c;
	union reg_ltm_h80                       reg_h80;
	union reg_ltm_h84                       reg_h84;
	union reg_ltm_h88                       reg_h88;
	union reg_ltm_h8c                       reg_h8c;
	union reg_ltm_h90                       reg_h90;
	union reg_ltm_h94                       reg_h94;
	union reg_ltm_h98                       reg_h98;
	union reg_ltm_h9c                       reg_h9c;
	union reg_ltm_ha0                       reg_ha0;
	union reg_ltm_ha4                       reg_ha4;
	union reg_ltm_ha8                       reg_ha8;
	union reg_ltm_hac                       reg_hac;
	union reg_ltm_hb0                       reg_hb0;
	union reg_ltm_hb4                       reg_hb4;
	union reg_ltm_hb8                       reg_hb8;
	union reg_ltm_hbc                       reg_hbc;
	union reg_ltm_hc0                       reg_hc0;
	union reg_ltm_hc4                       reg_hc4;
	union reg_ltm_hc8                       reg_hc8;
	union reg_ltm_hcc                       reg_hcc;
	union reg_ltm_hd0                       reg_hd0;
	union reg_ltm_hd4                       reg_hd4;
	union reg_ltm_hd8                       reg_hd8;
	union reg_ltm_hdc                       reg_hdc;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_ca_lite_t {
	union reg_ca_lite_00                    reg_00;
	union reg_ca_lite_04                    reg_04;
	union reg_ca_lite_08                    reg_08;
	union reg_ca_lite_0c                    reg_0c;
	union reg_ca_lite_10                    reg_10;
	union reg_ca_lite_14                    reg_14;
	union reg_ca_lite_18                    reg_18;
	union reg_ca_lite_1c                    reg_1c;
	union reg_ca_lite_20                    reg_20;
	union reg_ca_lite_24                    reg_24;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_444_422_t {
	union reg_isp_444_422_0                 reg_0;
	union reg_isp_444_422_1                 reg_1;
	union reg_isp_444_422_2                 reg_2;
	union reg_isp_444_422_3                 reg_3;
	union reg_isp_444_422_4                 reg_4;
	union reg_isp_444_422_5                 reg_5;
	union reg_isp_444_422_6                 reg_6;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_444_422_8                 reg_8;
	union reg_isp_444_422_9                 reg_9;
	union reg_isp_444_422_10                reg_10;
	union reg_isp_444_422_11                reg_11;
	union reg_isp_444_422_12                reg_12;
	union reg_isp_444_422_13                reg_13;
	union reg_isp_444_422_14                reg_14;
	union reg_isp_444_422_15                reg_15;
	union reg_isp_444_422_16                reg_16;
	union reg_isp_444_422_17                reg_17;
	union reg_isp_444_422_18                reg_18;
	union reg_isp_444_422_19                reg_19;
	union reg_isp_444_422_20                reg_20;
	union reg_isp_444_422_21                reg_21;
	union reg_isp_444_422_22                reg_22;
	union reg_isp_444_422_23                reg_23;
	union reg_isp_444_422_24                reg_24;
	union reg_isp_444_422_25                reg_25;
	union reg_isp_444_422_26                reg_26;
	union reg_isp_444_422_27                reg_27;
	union reg_isp_444_422_28                reg_28;
	union reg_isp_444_422_29                reg_29;
	union reg_isp_444_422_30                reg_30;
	union reg_isp_444_422_31                reg_31;
	union reg_isp_444_422_80                reg_80;
	union reg_isp_444_422_84                reg_84;
	union reg_isp_444_422_88                reg_88;
	union reg_isp_444_422_8c                reg_8c;
	union reg_isp_444_422_90                reg_90;
	union reg_isp_444_422_94                reg_94;
	union reg_isp_444_422_98                reg_98;
	union reg_isp_444_422_9c                reg_9c;
	union reg_isp_444_422_a0                reg_a0;
	union reg_isp_444_422_a4                reg_a4;
	union reg_isp_444_422_a8                reg_a8;
	union reg_isp_444_422_ac                reg_ac;
	union reg_isp_444_422_b0                reg_b0;
	union reg_isp_444_422_b4                reg_b4;
	union reg_isp_444_422_b8                reg_b8;
	union reg_isp_444_422_bc                reg_bc;
	union reg_isp_444_422_c0                reg_c0;
	union reg_isp_444_422_c4                reg_c4;
	union reg_isp_444_422_c8                reg_c8;
	union reg_isp_444_422_cc                reg_cc;
	union reg_isp_444_422_d0                reg_d0;
	union reg_isp_444_422_d4                reg_d4;
	union reg_isp_444_422_d8                reg_d8;
	union reg_isp_444_422_dc                reg_dc;
	union reg_isp_444_422_e0                reg_e0;
	union reg_isp_444_422_e4                reg_e4;
	union reg_isp_444_422_e8                reg_e8;
	union reg_isp_444_422_ec                reg_ec;
	union reg_isp_444_422_f0                reg_f0;
	union reg_isp_444_422_f4                reg_f4;
	union reg_isp_444_422_f8                reg_f8;
	union reg_isp_444_422_fc                reg_fc;
	union reg_isp_444_422_100               reg_100;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_fbce_t {
	union reg_fbce_00                       reg_00;
	uint32_t                                _resv_0x4[3];
	union reg_fbce_10                       reg_10;
	union reg_fbce_14                       reg_14;
	union reg_fbce_18                       reg_18;
	union reg_fbce_1c                       reg_1c;
	union reg_fbce_20                       reg_20;
	union reg_fbce_24                       reg_24;
	union reg_fbce_28                       reg_28;
	union reg_fbce_2c                       reg_2c;
	union reg_fbce_30                       reg_30;
	union reg_fbce_34                       reg_34;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_fbcd_t {
	union reg_fbcd_00                       reg_00;
	uint32_t                                _resv_0x4[2];
	union reg_fbcd_0c                       reg_0c;
	union reg_fbcd_10                       reg_10;
	union reg_fbcd_14                       reg_14;
	union reg_fbcd_18                       reg_18;
	uint32_t                                _resv_0x1c[1];
	union reg_fbcd_20                       reg_20;
	union reg_fbcd_24                       reg_24;
	union reg_fbcd_28                       reg_28;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_yuv_dither_t {
	union reg_isp_yuv_dither_y_dither       y_dither;
	union reg_isp_yuv_dither_uv_dither      uv_dither;
	union reg_isp_yuv_dither_debug_00       debug_00;
	union reg_isp_yuv_dither_debug_01       debug_01;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_ynr_t {
	union reg_isp_ynr_shadow_rd_sel         shadow_rd_sel;
	union reg_isp_ynr_out_sel               out_sel;
	union reg_isp_ynr_index_clr             index_clr;
	union reg_isp_ynr_ns0_luma_th_00        ns0_luma_th_00;
	union reg_isp_ynr_ns0_luma_th_01        ns0_luma_th_01;
	union reg_isp_ynr_ns0_luma_th_02        ns0_luma_th_02;
	union reg_isp_ynr_ns0_luma_th_03        ns0_luma_th_03;
	union reg_isp_ynr_ns0_luma_th_04        ns0_luma_th_04;
	union reg_isp_ynr_ns0_luma_th_05        ns0_luma_th_05;
	union reg_isp_ynr_ns0_slope_00          ns0_slope_00;
	union reg_isp_ynr_ns0_slope_01          ns0_slope_01;
	union reg_isp_ynr_ns0_slope_02          ns0_slope_02;
	union reg_isp_ynr_ns0_slope_03          ns0_slope_03;
	union reg_isp_ynr_ns0_slope_04          ns0_slope_04;
	union reg_isp_ynr_ns0_offset_00         ns0_offset_00;
	union reg_isp_ynr_ns0_offset_01         ns0_offset_01;
	union reg_isp_ynr_ns0_offset_02         ns0_offset_02;
	union reg_isp_ynr_ns0_offset_03         ns0_offset_03;
	union reg_isp_ynr_ns0_offset_04         ns0_offset_04;
	union reg_isp_ynr_ns0_offset_05         ns0_offset_05;
	union reg_isp_ynr_ns1_luma_th_00        ns1_luma_th_00;
	union reg_isp_ynr_ns1_luma_th_01        ns1_luma_th_01;
	union reg_isp_ynr_ns1_luma_th_02        ns1_luma_th_02;
	union reg_isp_ynr_ns1_luma_th_03        ns1_luma_th_03;
	union reg_isp_ynr_ns1_luma_th_04        ns1_luma_th_04;
	union reg_isp_ynr_ns1_luma_th_05        ns1_luma_th_05;
	union reg_isp_ynr_ns1_slope_00          ns1_slope_00;
	union reg_isp_ynr_ns1_slope_01          ns1_slope_01;
	union reg_isp_ynr_ns1_slope_02          ns1_slope_02;
	union reg_isp_ynr_ns1_slope_03          ns1_slope_03;
	union reg_isp_ynr_ns1_slope_04          ns1_slope_04;
	union reg_isp_ynr_ns1_offset_00         ns1_offset_00;
	union reg_isp_ynr_ns1_offset_01         ns1_offset_01;
	union reg_isp_ynr_ns1_offset_02         ns1_offset_02;
	union reg_isp_ynr_ns1_offset_03         ns1_offset_03;
	union reg_isp_ynr_ns1_offset_04         ns1_offset_04;
	union reg_isp_ynr_ns1_offset_05         ns1_offset_05;
	union reg_isp_ynr_ns_gain               ns_gain;
	union reg_isp_ynr_motion_lut_00         motion_lut_00;
	union reg_isp_ynr_motion_lut_01         motion_lut_01;
	union reg_isp_ynr_motion_lut_02         motion_lut_02;
	union reg_isp_ynr_motion_lut_03         motion_lut_03;
	union reg_isp_ynr_motion_lut_04         motion_lut_04;
	union reg_isp_ynr_motion_lut_05         motion_lut_05;
	union reg_isp_ynr_motion_lut_06         motion_lut_06;
	union reg_isp_ynr_motion_lut_07         motion_lut_07;
	union reg_isp_ynr_motion_lut_08         motion_lut_08;
	union reg_isp_ynr_motion_lut_09         motion_lut_09;
	union reg_isp_ynr_motion_lut_10         motion_lut_10;
	union reg_isp_ynr_motion_lut_11         motion_lut_11;
	union reg_isp_ynr_motion_lut_12         motion_lut_12;
	union reg_isp_ynr_motion_lut_13         motion_lut_13;
	union reg_isp_ynr_motion_lut_14         motion_lut_14;
	union reg_isp_ynr_motion_lut_15         motion_lut_15;
	union reg_isp_ynr_weight_intra_0        weight_intra_0;
	union reg_isp_ynr_weight_intra_1        weight_intra_1;
	union reg_isp_ynr_weight_intra_2        weight_intra_2;
	union reg_isp_ynr_weight_norm_1         weight_norm_1;
	union reg_isp_ynr_weight_norm_2         weight_norm_2;
	union reg_isp_ynr_alpha_gain            alpha_gain;
	union reg_isp_ynr_var_th                var_th;
	union reg_isp_ynr_weight_sm             weight_sm;
	union reg_isp_ynr_weight_v              weight_v;
	union reg_isp_ynr_weight_h              weight_h;
	union reg_isp_ynr_weight_d45            weight_d45;
	union reg_isp_ynr_weight_d135           weight_d135;
	union reg_isp_ynr_neighbor_max          neighbor_max;
	union reg_isp_ynr_res_k_smooth          res_k_smooth;
	union reg_isp_ynr_res_k_texture         res_k_texture;
	union reg_isp_ynr_filter_mode_en        filter_mode_en;
	union reg_isp_ynr_filter_mode_alpha     filter_mode_alpha;
	union reg_isp_ynr_res_mot_lut_00        res_mot_lut_00;
	union reg_isp_ynr_res_mot_lut_01        res_mot_lut_01;
	union reg_isp_ynr_res_mot_lut_02        res_mot_lut_02;
	union reg_isp_ynr_res_mot_lut_03        res_mot_lut_03;
	union reg_isp_ynr_res_mot_lut_04        res_mot_lut_04;
	union reg_isp_ynr_res_mot_lut_05        res_mot_lut_05;
	union reg_isp_ynr_res_mot_lut_06        res_mot_lut_06;
	union reg_isp_ynr_res_mot_lut_07        res_mot_lut_07;
	union reg_isp_ynr_res_mot_lut_08        res_mot_lut_08;
	union reg_isp_ynr_res_mot_lut_09        res_mot_lut_09;
	union reg_isp_ynr_res_mot_lut_10        res_mot_lut_10;
	union reg_isp_ynr_res_mot_lut_11        res_mot_lut_11;
	union reg_isp_ynr_res_mot_lut_12        res_mot_lut_12;
	union reg_isp_ynr_res_mot_lut_13        res_mot_lut_13;
	union reg_isp_ynr_res_mot_lut_14        res_mot_lut_14;
	union reg_isp_ynr_res_mot_lut_15        res_mot_lut_15;
	union reg_isp_ynr_res_max               res_max;
	union reg_isp_ynr_res_motion_max        res_motion_max;
	union reg_isp_ynr_motion_ns_clip_max    motion_ns_clip_max;
	uint32_t                                _resv_0x168[38];
	union reg_isp_ynr_weight_lut            weight_lut;
	uint32_t                                _resv_0x204[894];
	union reg_isp_ynr_dummy                 dummy;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_cnr_t {
	union reg_isp_cnr_enable                cnr_enable;
	union reg_isp_cnr_strength_mode         cnr_strength_mode;
	union reg_isp_cnr_purple_th             cnr_purple_th;
	union reg_isp_cnr_purple_cb             cnr_purple_cb;
	union reg_isp_cnr_green_cb              cnr_green_cb;
	union reg_isp_cnr_weight_lut_inter_cnr_00  weight_lut_inter_cnr_00;
	union reg_isp_cnr_weight_lut_inter_cnr_04  weight_lut_inter_cnr_04;
	union reg_isp_cnr_weight_lut_inter_cnr_08  weight_lut_inter_cnr_08;
	union reg_isp_cnr_weight_lut_inter_cnr_12  weight_lut_inter_cnr_12;
	uint32_t                                _resv_0x24[2];
	union reg_isp_cnr_motion_lut_0          cnr_motion_lut_0;
	union reg_isp_cnr_motion_lut_4          cnr_motion_lut_4;
	union reg_isp_cnr_motion_lut_8          cnr_motion_lut_8;
	union reg_isp_cnr_motion_lut_12         cnr_motion_lut_12;
	union reg_isp_cnr_purple_cb2            cnr_purple_cb2;
	union reg_isp_cnr_mask                  cnr_mask;
	union reg_isp_cnr_dummy                 cnr_dummy;
	union reg_isp_cnr_edge_scale            cnr_edge_scale;
	union reg_isp_cnr_edge_ratio_speed      cnr_edge_ratio_speed;
	union reg_isp_cnr_depurple_weight_th    cnr_depurple_weight_th;
	union reg_isp_cnr_coring_motion_lut_0   cnr_coring_motion_lut_0;
	union reg_isp_cnr_coring_motion_lut_4   cnr_coring_motion_lut_4;
	union reg_isp_cnr_coring_motion_lut_8   cnr_coring_motion_lut_8;
	union reg_isp_cnr_coring_motion_lut_12  cnr_coring_motion_lut_12;
	union reg_isp_cnr_edge_scale_lut_0      cnr_edge_scale_lut_0;
	union reg_isp_cnr_edge_scale_lut_4      cnr_edge_scale_lut_4;
	union reg_isp_cnr_edge_scale_lut_8      cnr_edge_scale_lut_8;
	union reg_isp_cnr_edge_scale_lut_12     cnr_edge_scale_lut_12;
	union reg_isp_cnr_edge_scale_lut_16     cnr_edge_scale_lut_16;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_ycurv_t {
	union reg_isp_ycurv_ycur_ctrl           ycur_ctrl;
	union reg_isp_ycurv_ycur_prog_ctrl      ycur_prog_ctrl;
	union reg_isp_ycurv_ycur_prog_st_addr   ycur_prog_st_addr;
	union reg_isp_ycurv_ycur_prog_data      ycur_prog_data;
	union reg_isp_ycurv_ycur_prog_max       ycur_prog_max;
	union reg_isp_ycurv_ycur_mem_sw_mode    ycur_sw_mode;
	union reg_isp_ycurv_ycur_mem_sw_rdata   ycur_sw_rdata;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_ycurv_ycur_dbg            ycur_dbg;
	union reg_isp_ycurv_ycur_dmy0           ycur_dmy0;
	union reg_isp_ycurv_ycur_dmy1           ycur_dmy1;
	union reg_isp_ycurv_ycur_dmy_r          ycur_dmy_r;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_top_t {
	union reg_isp_top_int_event0            int_event0;
	union reg_isp_top_int_event1            int_event1;
	union reg_isp_top_int_event2            int_event2;
	union reg_isp_top_error_sts             error_sts;
	union reg_isp_top_int_event0_en         int_event0_en;
	union reg_isp_top_int_event1_en         int_event1_en;
	union reg_isp_top_int_event2_en         int_event2_en;
	uint32_t                                _resv_0x1c[1];
	union reg_isp_top_sw_ctrl_0             sw_ctrl_0;
	union reg_isp_top_sw_ctrl_1             sw_ctrl_1;
	union reg_isp_top_ctrl_mode_sel0        ctrl_mode_sel0;
	union reg_isp_top_ctrl_mode_sel1        ctrl_mode_sel1;
	union reg_isp_top_scenarios_ctrl        scenarios_ctrl;
	union reg_isp_top_sw_rst                sw_rst;
	union reg_isp_top_blk_idle              blk_idle;
	union reg_isp_top_blk_idle_enable       blk_idle_enable;
	union reg_isp_top_dbus0                 dbus0;
	union reg_isp_top_dbus1                 dbus1;
	union reg_isp_top_dbus2                 dbus2;
	union reg_isp_top_dbus3                 dbus3;
	union reg_isp_top_force_int             force_int;
	union reg_isp_top_dummy                 dummy;
	union reg_isp_top_ip_enable0            ip_enable0;
	union reg_isp_top_ip_enable1            ip_enable1;
	union reg_isp_top_ip_enable2            ip_enable2;
	uint32_t                                _resv_0x64[1];
	union reg_isp_top_cmdq_ctrl             cmdq_ctrl;
	union reg_isp_top_cmdq_trig             cmdq_trig;
	union reg_isp_top_trig_cnt              trig_cnt;
	union reg_isp_top_svn_version           svn_version;
	union reg_isp_top_timestamp             timestamp;
	uint32_t                                _resv_0x7c[1];
	union reg_isp_top_sclie_enable          sclie_enable;
	union reg_isp_top_w_slice_thresh_main   w_slice_thresh_main;
	union reg_isp_top_w_slice_thresh_sub_curr  w_slice_thresh_sub_curr;
	union reg_isp_top_w_slice_thresh_sub_prv  w_slice_thresh_sub_prv;
	union reg_isp_top_r_slice_thresh_main   r_slice_thresh_main;
	union reg_isp_top_r_slice_thresh_sub_curr  r_slice_thresh_sub_curr;
	union reg_isp_top_r_slice_thresh_sub_prv  r_slice_thresh_sub_prv;
	union reg_isp_top_raw_frame_valid       raw_frame_valid;
	union reg_isp_top_first_frame           first_frame;
	union reg_isp_top_int_event0_fe345      int_event0_fe345;
	union reg_isp_top_int_event1_fe345      int_event1_fe345;
	union reg_isp_top_int_event2_fe345      int_event2_fe345;
	union reg_isp_top_error_sts_fe345       error_sts_fe345;
	union reg_isp_top_int_event0_en_fe345   int_event0_en_fe345;
	union reg_isp_top_int_event1_en_fe345   int_event1_en_fe345;
	union reg_isp_top_int_event2_en_fe345   int_event2_en_fe345;
	union reg_isp_top_sw_ctrl_0_fe345       sw_ctrl_0_fe345;
	union reg_isp_top_sw_ctrl_1_fe345       sw_ctrl_1_fe345;
	union reg_isp_top_ctrl_mode_sel0_fe345  ctrl_mode_sel0_fe345;
	union reg_isp_top_ctrl_mode_sel1_fe345  ctrl_mode_sel1_fe345;
	union reg_isp_top_sw_rst_fe345          sw_rst_fe345;
	union reg_isp_top_blk_idle_1            blk_idle_1;
	union reg_isp_top_blk_idle_enable_fe345  blk_idle_enable_fe345;
	union reg_isp_top_ip_enable_fe345       ip_enable_fe345;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_wdma_core_t {
	union reg_wdma_core_shadow_rd_sel       shadow_rd_sel;
	union reg_wdma_core_ip_disable          ip_disable;
	union reg_wdma_core_disable_seglen      disable_seglen;
	union reg_wdma_core_up_ring_base        up_ring_base;
	union reg_wdma_core_norm_status0        norm_status0;
	union reg_wdma_core_norm_status1        norm_status1;
	union reg_wdma_core_ring_buffer_en      ring_buffer_en;
	uint32_t                                _resv_0x1c[1];
	union reg_wdma_core_norm_perf           norm_perf;
	uint32_t                                _resv_0x24[1];
	union reg_wdma_core_ring_patch_enable   ring_patch_enable;
	union reg_wdma_core_set_ring_base       set_ring_base;
	union reg_wdma_core_ring_base_addr_l    ring_base_addr_l;
	union reg_wdma_core_ring_base_addr_h    ring_base_addr_h;
	uint32_t                                _resv_0x38[18];
	union reg_wdma_core_ring_buffer_size0   ring_buffer_size0;
	union reg_wdma_core_ring_buffer_size1   ring_buffer_size1;
	union reg_wdma_core_ring_buffer_size2   ring_buffer_size2;
	union reg_wdma_core_ring_buffer_size3   ring_buffer_size3;
	union reg_wdma_core_ring_buffer_size4   ring_buffer_size4;
	union reg_wdma_core_ring_buffer_size5   ring_buffer_size5;
	union reg_wdma_core_ring_buffer_size6   ring_buffer_size6;
	union reg_wdma_core_ring_buffer_size7   ring_buffer_size7;
	union reg_wdma_core_ring_buffer_size8   ring_buffer_size8;
	union reg_wdma_core_ring_buffer_size9   ring_buffer_size9;
	union reg_wdma_core_ring_buffer_size10  ring_buffer_size10;
	union reg_wdma_core_ring_buffer_size11  ring_buffer_size11;
	union reg_wdma_core_ring_buffer_size12  ring_buffer_size12;
	union reg_wdma_core_ring_buffer_size13  ring_buffer_size13;
	union reg_wdma_core_ring_buffer_size14  ring_buffer_size14;
	union reg_wdma_core_ring_buffer_size15  ring_buffer_size15;
	union reg_wdma_core_ring_buffer_size16  ring_buffer_size16;
	union reg_wdma_core_ring_buffer_size17  ring_buffer_size17;
	union reg_wdma_core_ring_buffer_size18  ring_buffer_size18;
	union reg_wdma_core_ring_buffer_size19  ring_buffer_size19;
	union reg_wdma_core_ring_buffer_size20  ring_buffer_size20;
	union reg_wdma_core_ring_buffer_size21  ring_buffer_size21;
	union reg_wdma_core_ring_buffer_size22  ring_buffer_size22;
	union reg_wdma_core_ring_buffer_size23  ring_buffer_size23;
	union reg_wdma_core_ring_buffer_size24  ring_buffer_size24;
	union reg_wdma_core_ring_buffer_size25  ring_buffer_size25;
	union reg_wdma_core_ring_buffer_size26  ring_buffer_size26;
	union reg_wdma_core_ring_buffer_size27  ring_buffer_size27;
	union reg_wdma_core_ring_buffer_size28  ring_buffer_size28;
	union reg_wdma_core_ring_buffer_size29  ring_buffer_size29;
	union reg_wdma_core_ring_buffer_size30  ring_buffer_size30;
	union reg_wdma_core_ring_buffer_size31  ring_buffer_size31;
	union reg_wdma_core_next_dma_addr_sts0  next_dma_addr_sts0;
	union reg_wdma_core_next_dma_addr_sts1  next_dma_addr_sts1;
	union reg_wdma_core_next_dma_addr_sts2  next_dma_addr_sts2;
	union reg_wdma_core_next_dma_addr_sts3  next_dma_addr_sts3;
	union reg_wdma_core_next_dma_addr_sts4  next_dma_addr_sts4;
	union reg_wdma_core_next_dma_addr_sts5  next_dma_addr_sts5;
	union reg_wdma_core_next_dma_addr_sts6  next_dma_addr_sts6;
	union reg_wdma_core_next_dma_addr_sts7  next_dma_addr_sts7;
	union reg_wdma_core_next_dma_addr_sts8  next_dma_addr_sts8;
	union reg_wdma_core_next_dma_addr_sts9  next_dma_addr_sts9;
	union reg_wdma_core_next_dma_addr_sts10  next_dma_addr_sts10;
	union reg_wdma_core_next_dma_addr_sts11  next_dma_addr_sts11;
	union reg_wdma_core_next_dma_addr_sts12  next_dma_addr_sts12;
	union reg_wdma_core_next_dma_addr_sts13  next_dma_addr_sts13;
	union reg_wdma_core_next_dma_addr_sts14  next_dma_addr_sts14;
	union reg_wdma_core_next_dma_addr_sts15  next_dma_addr_sts15;
	union reg_wdma_core_next_dma_addr_sts16  next_dma_addr_sts16;
	union reg_wdma_core_next_dma_addr_sts17  next_dma_addr_sts17;
	union reg_wdma_core_next_dma_addr_sts18  next_dma_addr_sts18;
	union reg_wdma_core_next_dma_addr_sts19  next_dma_addr_sts19;
	union reg_wdma_core_next_dma_addr_sts20  next_dma_addr_sts20;
	union reg_wdma_core_next_dma_addr_sts21  next_dma_addr_sts21;
	union reg_wdma_core_next_dma_addr_sts22  next_dma_addr_sts22;
	union reg_wdma_core_next_dma_addr_sts23  next_dma_addr_sts23;
	union reg_wdma_core_next_dma_addr_sts24  next_dma_addr_sts24;
	union reg_wdma_core_next_dma_addr_sts25  next_dma_addr_sts25;
	union reg_wdma_core_next_dma_addr_sts26  next_dma_addr_sts26;
	union reg_wdma_core_next_dma_addr_sts27  next_dma_addr_sts27;
	union reg_wdma_core_next_dma_addr_sts28  next_dma_addr_sts28;
	union reg_wdma_core_next_dma_addr_sts29  next_dma_addr_sts29;
	union reg_wdma_core_next_dma_addr_sts30  next_dma_addr_sts30;
	union reg_wdma_core_next_dma_addr_sts31  next_dma_addr_sts31;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_rdma_core_t {
	union reg_rdma_core_shadow_rd_sel       shadow_rd_sel;
	union reg_rdma_core_ip_disable          ip_disable;
	uint32_t                                _resv_0x8[1];
	union reg_rdma_core_up_ring_base        up_ring_base;
	union reg_rdma_core_norm_status0        norm_status0;
	union reg_rdma_core_norm_status1        norm_status1;
	union reg_rdma_core_ring_buffer_en      ring_buffer_en;
	uint32_t                                _resv_0x1c[1];
	union reg_rdma_core_norm_perf           norm_perf;
	union reg_rdma_core_ar_priority_sel     ar_priority_sel;
	union reg_rdma_core_ring_patch_enable   ring_patch_enable;
	union reg_rdma_core_set_ring_base       set_ring_base;
	union reg_rdma_core_ring_base_addr_l    ring_base_addr_l;
	union reg_rdma_core_ring_base_addr_h    ring_base_addr_h;
	uint32_t                                _resv_0x38[18];
	union reg_rdma_core_ring_buffer_size0   ring_buffer_size0;
	union reg_rdma_core_ring_buffer_size1   ring_buffer_size1;
	union reg_rdma_core_ring_buffer_size2   ring_buffer_size2;
	union reg_rdma_core_ring_buffer_size3   ring_buffer_size3;
	union reg_rdma_core_ring_buffer_size4   ring_buffer_size4;
	union reg_rdma_core_ring_buffer_size5   ring_buffer_size5;
	union reg_rdma_core_ring_buffer_size6   ring_buffer_size6;
	union reg_rdma_core_ring_buffer_size7   ring_buffer_size7;
	union reg_rdma_core_ring_buffer_size8   ring_buffer_size8;
	union reg_rdma_core_ring_buffer_size9   ring_buffer_size9;
	union reg_rdma_core_ring_buffer_size10  ring_buffer_size10;
	union reg_rdma_core_ring_buffer_size11  ring_buffer_size11;
	union reg_rdma_core_ring_buffer_size12  ring_buffer_size12;
	union reg_rdma_core_ring_buffer_size13  ring_buffer_size13;
	union reg_rdma_core_ring_buffer_size14  ring_buffer_size14;
	union reg_rdma_core_ring_buffer_size15  ring_buffer_size15;
	union reg_rdma_core_ring_buffer_size16  ring_buffer_size16;
	union reg_rdma_core_ring_buffer_size17  ring_buffer_size17;
	union reg_rdma_core_ring_buffer_size18  ring_buffer_size18;
	union reg_rdma_core_ring_buffer_size19  ring_buffer_size19;
	union reg_rdma_core_ring_buffer_size20  ring_buffer_size20;
	union reg_rdma_core_ring_buffer_size21  ring_buffer_size21;
	union reg_rdma_core_ring_buffer_size22  ring_buffer_size22;
	union reg_rdma_core_ring_buffer_size23  ring_buffer_size23;
	union reg_rdma_core_ring_buffer_size24  ring_buffer_size24;
	union reg_rdma_core_ring_buffer_size25  ring_buffer_size25;
	union reg_rdma_core_ring_buffer_size26  ring_buffer_size26;
	union reg_rdma_core_ring_buffer_size27  ring_buffer_size27;
	union reg_rdma_core_ring_buffer_size28  ring_buffer_size28;
	union reg_rdma_core_ring_buffer_size29  ring_buffer_size29;
	union reg_rdma_core_ring_buffer_size30  ring_buffer_size30;
	union reg_rdma_core_ring_buffer_size31  ring_buffer_size31;
	union reg_rdma_core_next_dma_addr_sts0  next_dma_addr_sts0;
	union reg_rdma_core_next_dma_addr_sts1  next_dma_addr_sts1;
	union reg_rdma_core_next_dma_addr_sts2  next_dma_addr_sts2;
	union reg_rdma_core_next_dma_addr_sts3  next_dma_addr_sts3;
	union reg_rdma_core_next_dma_addr_sts4  next_dma_addr_sts4;
	union reg_rdma_core_next_dma_addr_sts5  next_dma_addr_sts5;
	union reg_rdma_core_next_dma_addr_sts6  next_dma_addr_sts6;
	union reg_rdma_core_next_dma_addr_sts7  next_dma_addr_sts7;
	union reg_rdma_core_next_dma_addr_sts8  next_dma_addr_sts8;
	union reg_rdma_core_next_dma_addr_sts9  next_dma_addr_sts9;
	union reg_rdma_core_next_dma_addr_sts10  next_dma_addr_sts10;
	union reg_rdma_core_next_dma_addr_sts11  next_dma_addr_sts11;
	union reg_rdma_core_next_dma_addr_sts12  next_dma_addr_sts12;
	union reg_rdma_core_next_dma_addr_sts13  next_dma_addr_sts13;
	union reg_rdma_core_next_dma_addr_sts14  next_dma_addr_sts14;
	union reg_rdma_core_next_dma_addr_sts15  next_dma_addr_sts15;
	union reg_rdma_core_next_dma_addr_sts16  next_dma_addr_sts16;
	union reg_rdma_core_next_dma_addr_sts17  next_dma_addr_sts17;
	union reg_rdma_core_next_dma_addr_sts18  next_dma_addr_sts18;
	union reg_rdma_core_next_dma_addr_sts19  next_dma_addr_sts19;
	union reg_rdma_core_next_dma_addr_sts20  next_dma_addr_sts20;
	union reg_rdma_core_next_dma_addr_sts21  next_dma_addr_sts21;
	union reg_rdma_core_next_dma_addr_sts22  next_dma_addr_sts22;
	union reg_rdma_core_next_dma_addr_sts23  next_dma_addr_sts23;
	union reg_rdma_core_next_dma_addr_sts24  next_dma_addr_sts24;
	union reg_rdma_core_next_dma_addr_sts25  next_dma_addr_sts25;
	union reg_rdma_core_next_dma_addr_sts26  next_dma_addr_sts26;
	union reg_rdma_core_next_dma_addr_sts27  next_dma_addr_sts27;
	union reg_rdma_core_next_dma_addr_sts28  next_dma_addr_sts28;
	union reg_rdma_core_next_dma_addr_sts29  next_dma_addr_sts29;
	union reg_rdma_core_next_dma_addr_sts30  next_dma_addr_sts30;
	union reg_rdma_core_next_dma_addr_sts31  next_dma_addr_sts31;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_csi_bdg_lite_t {
	union reg_isp_csi_bdg_lite_bdg_top_ctrl  csi_bdg_top_ctrl;
	union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_0  csi_bdg_interrupt_ctrl_0;
	union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_1  csi_bdg_interrupt_ctrl_1;
	union reg_isp_csi_bdg_lite_frame_vld    frame_vld;
	union reg_isp_csi_bdg_lite_ch0_size     ch0_size;
	union reg_isp_csi_bdg_lite_ch1_size     ch1_size;
	union reg_isp_csi_bdg_lite_ch2_size     ch2_size;
	union reg_isp_csi_bdg_lite_ch3_size     ch3_size;
	union reg_isp_csi_bdg_lite_ch0_crop_en  ch0_crop_en;
	union reg_isp_csi_bdg_lite_ch0_horz_crop  ch0_horz_crop;
	union reg_isp_csi_bdg_lite_ch0_vert_crop  ch0_vert_crop;
	uint32_t                                _resv_0x2c[1];
	union reg_isp_csi_bdg_lite_ch1_crop_en  ch1_crop_en;
	union reg_isp_csi_bdg_lite_ch1_horz_crop  ch1_horz_crop;
	union reg_isp_csi_bdg_lite_ch1_vert_crop  ch1_vert_crop;
	uint32_t                                _resv_0x3c[1];
	union reg_isp_csi_bdg_lite_ch2_crop_en  ch2_crop_en;
	union reg_isp_csi_bdg_lite_ch2_horz_crop  ch2_horz_crop;
	union reg_isp_csi_bdg_lite_ch2_vert_crop  ch2_vert_crop;
	uint32_t                                _resv_0x4c[1];
	union reg_isp_csi_bdg_lite_ch3_crop_en  ch3_crop_en;
	union reg_isp_csi_bdg_lite_ch3_horz_crop  ch3_horz_crop;
	union reg_isp_csi_bdg_lite_ch3_vert_crop  ch3_vert_crop;
	uint32_t                                _resv_0x5c[16];
	union reg_isp_csi_bdg_lite_line_intp_height  line_intp_height;
	union reg_isp_csi_bdg_lite_ch0_debug_0  ch0_debug_0;
	union reg_isp_csi_bdg_lite_ch0_debug_1  ch0_debug_1;
	union reg_isp_csi_bdg_lite_ch0_debug_2  ch0_debug_2;
	union reg_isp_csi_bdg_lite_ch0_debug_3  ch0_debug_3;
	union reg_isp_csi_bdg_lite_ch1_debug_0  ch1_debug_0;
	union reg_isp_csi_bdg_lite_ch1_debug_1  ch1_debug_1;
	union reg_isp_csi_bdg_lite_ch1_debug_2  ch1_debug_2;
	union reg_isp_csi_bdg_lite_ch1_debug_3  ch1_debug_3;
	union reg_isp_csi_bdg_lite_ch2_debug_0  ch2_debug_0;
	union reg_isp_csi_bdg_lite_ch2_debug_1  ch2_debug_1;
	union reg_isp_csi_bdg_lite_ch2_debug_2  ch2_debug_2;
	union reg_isp_csi_bdg_lite_ch2_debug_3  ch2_debug_3;
	union reg_isp_csi_bdg_lite_ch3_debug_0  ch3_debug_0;
	union reg_isp_csi_bdg_lite_ch3_debug_1  ch3_debug_1;
	union reg_isp_csi_bdg_lite_ch3_debug_2  ch3_debug_2;
	union reg_isp_csi_bdg_lite_ch3_debug_3  ch3_debug_3;
	union reg_isp_csi_bdg_lite_interrupt_status_0  interrupt_status_0;
	union reg_isp_csi_bdg_lite_interrupt_status_1  interrupt_status_1;
	union reg_isp_csi_bdg_lite_bdg_debug    bdg_debug;
	uint32_t                                _resv_0xec[1];
	union reg_isp_csi_bdg_lite_wr_urgent_ctrl  csi_wr_urgent_ctrl;
	union reg_isp_csi_bdg_lite_rd_urgent_ctrl  csi_rd_urgent_ctrl;
	union reg_isp_csi_bdg_lite_dummy        csi_dummy;
	uint32_t                                _resv_0xfc[21];
	union reg_isp_csi_bdg_lite_trig_dly_control_0  trig_dly_control_0;
	union reg_isp_csi_bdg_lite_trig_dly_control_1  trig_dly_control_1;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_raw_rdma_ctrl_t {
	union reg_raw_rdma_ctrl_read_sel        read_sel;
	union reg_raw_rdma_ctrl_config          config;
	union reg_raw_rdma_ctrl_rdma_size       rdma_size;
	union reg_raw_rdma_ctrl_dpcm_mode       dpcm_mode;
	union reg_raw_rdma_ctrl_rdma_enable     rdma_enable;
	union reg_raw_rdma_ctrl_rdma_crop       rdma_crop;
	union reg_raw_rdma_ctrl_rdma_crop_h     rdma_crop_h;
	union reg_raw_rdma_ctrl_rdma_crop_w     rdma_crop_w;
	union reg_raw_rdma_ctrl_pass            pass;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_ldci_t {
	union reg_isp_ldci_enable               ldci_enable;
	union reg_isp_ldci_strength             ldci_strength;
	union reg_isp_ldci_luma_wgt_max         ldci_luma_wgt_max;
	union reg_isp_ldci_idx_iir_alpha        ldci_idx_iir_alpha;
	union reg_isp_ldci_edge_scale           ldci_edge_scale;
	union reg_isp_ldci_edge_clamp           ldci_edge_clamp;
	union reg_isp_ldci_idx_filter_norm      ldci_idx_filter_norm;
	union reg_isp_ldci_tone_curve_idx_00    ldci_tone_curve_idx_00;
	uint32_t                                _resv_0x20[3];
	union reg_isp_ldci_blk_size_x           ldci_blk_size_x;
	union reg_isp_ldci_blk_size_x1          ldci_blk_size_x1;
	union reg_isp_ldci_subblk_size_x        ldci_subblk_size_x;
	union reg_isp_ldci_subblk_size_x1       ldci_subblk_size_x1;
	union reg_isp_ldci_interp_norm_lr       ldci_interp_norm_lr;
	union reg_isp_ldci_sub_interp_norm_lr   ldci_sub_interp_norm_lr;
	union reg_isp_ldci_mean_norm_x          ldci_mean_norm_x;
	union reg_isp_ldci_var_norm_y           ldci_var_norm_y;
	union reg_isp_ldci_uv_gain_max          ldci_uv_gain_max;
	union reg_isp_ldci_img_widthm1          ldci_img_widthm1;
	uint32_t                                _resv_0x54[11];
	union reg_isp_ldci_status               ldci_status;
	union reg_isp_ldci_grace_reset          ldci_grace_reset;
	union reg_isp_ldci_monitor              ldci_monitor;
	union reg_isp_ldci_flow                 ldci_flow;
	union reg_isp_ldci_monitor_select       ldci_monitor_select;
	union reg_isp_ldci_location             ldci_location;
	union reg_isp_ldci_debug                ldci_debug;
	union reg_isp_ldci_dummy                ldci_dummy;
	union reg_isp_ldci_dmi_enable           dmi_enable;
	uint32_t                                _resv_0xa4[1];
	union reg_isp_ldci_dci_bayer_starting   dci_bayer_starting;
	uint32_t                                _resv_0xac[1];
	union reg_isp_ldci_idx_filter_lut_00    ldci_idx_filter_lut_00;
	union reg_isp_ldci_idx_filter_lut_02    ldci_idx_filter_lut_02;
	union reg_isp_ldci_idx_filter_lut_04    ldci_idx_filter_lut_04;
	union reg_isp_ldci_idx_filter_lut_06    ldci_idx_filter_lut_06;
	union reg_isp_ldci_idx_filter_lut_08    ldci_idx_filter_lut_08;
	union reg_isp_ldci_idx_filter_lut_10    ldci_idx_filter_lut_10;
	union reg_isp_ldci_idx_filter_lut_12    ldci_idx_filter_lut_12;
	union reg_isp_ldci_idx_filter_lut_14    ldci_idx_filter_lut_14;
	union reg_isp_ldci_interp_norm_lr1      ldci_interp_norm_lr1;
	union reg_isp_ldci_sub_interp_norm_lr1  ldci_sub_interp_norm_lr1;
	union reg_isp_ldci_tone_curve_lut_00_00  ldci_tone_curve_lut_00_00;
	union reg_isp_ldci_tone_curve_lut_00_02  ldci_tone_curve_lut_00_02;
	union reg_isp_ldci_tone_curve_lut_00_04  ldci_tone_curve_lut_00_04;
	union reg_isp_ldci_tone_curve_lut_00_06  ldci_tone_curve_lut_00_06;
	union reg_isp_ldci_tone_curve_lut_00_08  ldci_tone_curve_lut_00_08;
	union reg_isp_ldci_tone_curve_lut_00_10  ldci_tone_curve_lut_00_10;
	union reg_isp_ldci_tone_curve_lut_00_12  ldci_tone_curve_lut_00_12;
	union reg_isp_ldci_tone_curve_lut_00_14  ldci_tone_curve_lut_00_14;
	union reg_isp_ldci_tone_curve_lut_01_00  ldci_tone_curve_lut_01_00;
	union reg_isp_ldci_tone_curve_lut_01_02  ldci_tone_curve_lut_01_02;
	union reg_isp_ldci_tone_curve_lut_01_04  ldci_tone_curve_lut_01_04;
	union reg_isp_ldci_tone_curve_lut_01_06  ldci_tone_curve_lut_01_06;
	union reg_isp_ldci_tone_curve_lut_01_08  ldci_tone_curve_lut_01_08;
	union reg_isp_ldci_tone_curve_lut_01_10  ldci_tone_curve_lut_01_10;
	union reg_isp_ldci_tone_curve_lut_01_12  ldci_tone_curve_lut_01_12;
	union reg_isp_ldci_tone_curve_lut_01_14  ldci_tone_curve_lut_01_14;
	union reg_isp_ldci_tone_curve_lut_02_00  ldci_tone_curve_lut_02_00;
	union reg_isp_ldci_tone_curve_lut_02_02  ldci_tone_curve_lut_02_02;
	union reg_isp_ldci_tone_curve_lut_02_04  ldci_tone_curve_lut_02_04;
	union reg_isp_ldci_tone_curve_lut_02_06  ldci_tone_curve_lut_02_06;
	union reg_isp_ldci_tone_curve_lut_02_08  ldci_tone_curve_lut_02_08;
	union reg_isp_ldci_tone_curve_lut_02_10  ldci_tone_curve_lut_02_10;
	union reg_isp_ldci_tone_curve_lut_02_12  ldci_tone_curve_lut_02_12;
	union reg_isp_ldci_tone_curve_lut_02_14  ldci_tone_curve_lut_02_14;
	union reg_isp_ldci_tone_curve_lut_03_00  ldci_tone_curve_lut_03_00;
	union reg_isp_ldci_tone_curve_lut_03_02  ldci_tone_curve_lut_03_02;
	union reg_isp_ldci_tone_curve_lut_03_04  ldci_tone_curve_lut_03_04;
	union reg_isp_ldci_tone_curve_lut_03_06  ldci_tone_curve_lut_03_06;
	union reg_isp_ldci_tone_curve_lut_03_08  ldci_tone_curve_lut_03_08;
	union reg_isp_ldci_tone_curve_lut_03_10  ldci_tone_curve_lut_03_10;
	union reg_isp_ldci_tone_curve_lut_03_12  ldci_tone_curve_lut_03_12;
	union reg_isp_ldci_tone_curve_lut_03_14  ldci_tone_curve_lut_03_14;
	union reg_isp_ldci_tone_curve_lut_04_00  ldci_tone_curve_lut_04_00;
	union reg_isp_ldci_tone_curve_lut_04_02  ldci_tone_curve_lut_04_02;
	union reg_isp_ldci_tone_curve_lut_04_04  ldci_tone_curve_lut_04_04;
	union reg_isp_ldci_tone_curve_lut_04_06  ldci_tone_curve_lut_04_06;
	union reg_isp_ldci_tone_curve_lut_04_08  ldci_tone_curve_lut_04_08;
	union reg_isp_ldci_tone_curve_lut_04_10  ldci_tone_curve_lut_04_10;
	union reg_isp_ldci_tone_curve_lut_04_12  ldci_tone_curve_lut_04_12;
	union reg_isp_ldci_tone_curve_lut_04_14  ldci_tone_curve_lut_04_14;
	union reg_isp_ldci_tone_curve_lut_05_00  ldci_tone_curve_lut_05_00;
	union reg_isp_ldci_tone_curve_lut_05_02  ldci_tone_curve_lut_05_02;
	union reg_isp_ldci_tone_curve_lut_05_04  ldci_tone_curve_lut_05_04;
	union reg_isp_ldci_tone_curve_lut_05_06  ldci_tone_curve_lut_05_06;
	union reg_isp_ldci_tone_curve_lut_05_08  ldci_tone_curve_lut_05_08;
	union reg_isp_ldci_tone_curve_lut_05_10  ldci_tone_curve_lut_05_10;
	union reg_isp_ldci_tone_curve_lut_05_12  ldci_tone_curve_lut_05_12;
	union reg_isp_ldci_tone_curve_lut_05_14  ldci_tone_curve_lut_05_14;
	union reg_isp_ldci_tone_curve_lut_06_00  ldci_tone_curve_lut_06_00;
	union reg_isp_ldci_tone_curve_lut_06_02  ldci_tone_curve_lut_06_02;
	union reg_isp_ldci_tone_curve_lut_06_04  ldci_tone_curve_lut_06_04;
	union reg_isp_ldci_tone_curve_lut_06_06  ldci_tone_curve_lut_06_06;
	union reg_isp_ldci_tone_curve_lut_06_08  ldci_tone_curve_lut_06_08;
	union reg_isp_ldci_tone_curve_lut_06_10  ldci_tone_curve_lut_06_10;
	union reg_isp_ldci_tone_curve_lut_06_12  ldci_tone_curve_lut_06_12;
	union reg_isp_ldci_tone_curve_lut_06_14  ldci_tone_curve_lut_06_14;
	union reg_isp_ldci_tone_curve_lut_07_00  ldci_tone_curve_lut_07_00;
	union reg_isp_ldci_tone_curve_lut_07_02  ldci_tone_curve_lut_07_02;
	union reg_isp_ldci_tone_curve_lut_07_04  ldci_tone_curve_lut_07_04;
	union reg_isp_ldci_tone_curve_lut_07_06  ldci_tone_curve_lut_07_06;
	union reg_isp_ldci_tone_curve_lut_07_08  ldci_tone_curve_lut_07_08;
	union reg_isp_ldci_tone_curve_lut_07_10  ldci_tone_curve_lut_07_10;
	union reg_isp_ldci_tone_curve_lut_07_12  ldci_tone_curve_lut_07_12;
	union reg_isp_ldci_tone_curve_lut_07_14  ldci_tone_curve_lut_07_14;
	union reg_isp_ldci_tone_curve_lut_p_00  ldci_tone_curve_lut_p_00;
	union reg_isp_ldci_tone_curve_lut_p_02  ldci_tone_curve_lut_p_02;
	union reg_isp_ldci_tone_curve_lut_p_04  ldci_tone_curve_lut_p_04;
	union reg_isp_ldci_tone_curve_lut_p_06  ldci_tone_curve_lut_p_06;
	union reg_isp_ldci_tone_curve_lut_p_08  ldci_tone_curve_lut_p_08;
	union reg_isp_ldci_tone_curve_lut_p_10  ldci_tone_curve_lut_p_10;
	union reg_isp_ldci_tone_curve_lut_p_12  ldci_tone_curve_lut_p_12;
	union reg_isp_ldci_tone_curve_lut_p_14  ldci_tone_curve_lut_p_14;
	uint32_t                                _resv_0x1f8[2];
	union reg_isp_ldci_luma_wgt_lut_00      ldci_luma_wgt_lut_00;
	union reg_isp_ldci_luma_wgt_lut_04      ldci_luma_wgt_lut_04;
	union reg_isp_ldci_luma_wgt_lut_08      ldci_luma_wgt_lut_08;
	union reg_isp_ldci_luma_wgt_lut_12      ldci_luma_wgt_lut_12;
	union reg_isp_ldci_luma_wgt_lut_16      ldci_luma_wgt_lut_16;
	union reg_isp_ldci_luma_wgt_lut_20      ldci_luma_wgt_lut_20;
	union reg_isp_ldci_luma_wgt_lut_24      ldci_luma_wgt_lut_24;
	union reg_isp_ldci_luma_wgt_lut_28      ldci_luma_wgt_lut_28;
	union reg_isp_ldci_luma_wgt_lut_32      ldci_luma_wgt_lut_32;
	uint32_t                                _resv_0x224[3];
	union reg_isp_ldci_var_filter_lut_00    ldci_var_filter_lut_00;
	union reg_isp_ldci_var_filter_lut_02    ldci_var_filter_lut_02;
	union reg_isp_ldci_var_filter_lut_04    ldci_var_filter_lut_04;
	union reg_isp_ldci_block_crop_size      ldci_block_crop_size;
	union reg_isp_ldci_block_x_crop_size    ldci_block_x_crop_size;
	union reg_isp_ldci_block_y_crop_size    ldci_block_y_crop_size;
	union reg_isp_ldci_block_crop_enable    ldci_block_crop_enable;
	union reg_isp_ldci_block_x_offset       ldci_block_x_offset;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_rgb_top_t {
	union reg_isp_rgb_top_0                 reg_0;
	union reg_isp_rgb_top_1                 reg_1;
	union reg_isp_rgb_top_2                 reg_2;
	union reg_isp_rgb_top_3                 reg_3;
	union reg_isp_rgb_top_4                 reg_4;
	union reg_isp_rgb_top_5                 reg_5;
	union reg_isp_rgb_top_6                 reg_6;
	union reg_isp_rgb_top_7                 reg_7;
	union reg_isp_rgb_top_8                 reg_8;
	union reg_isp_rgb_top_9                 reg_9;
	uint32_t                                _resv_0x28[2];
	union reg_isp_rgb_top_10                reg_10;
	union reg_isp_rgb_top_11                reg_11;
	union reg_isp_rgb_top_12                reg_12;
	union reg_isp_rgb_top_13                reg_13;
	union reg_isp_rgb_top_14                reg_14;
	uint32_t                                _resv_0x44[3];
	union reg_isp_rgb_top_dbg_ip_s_vld      dbg_ip_s_vld;
	union reg_isp_rgb_top_dbg_ip_s_rdy      dbg_ip_s_rdy;
	union reg_isp_rgb_top_dbg_dmi_vld       dbg_dmi_vld;
	union reg_isp_rgb_top_dbg_dmi_rdy       dbg_dmi_rdy;
	union reg_isp_rgb_top_patgen1           patgen1;
	union reg_isp_rgb_top_patgen2           patgen2;
	union reg_isp_rgb_top_patgen3           patgen3;
	union reg_isp_rgb_top_patgen4           patgen4;
	union reg_isp_rgb_top_chk_sum           chk_sum;
	union reg_isp_rgb_top_dma_idle          dma_idle;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_pre_wdma_ctrl_t {
	union reg_pre_wdma_ctrl                 pre_wdma_ctrl;
	union reg_pre_wdma_ctrl_pre_raw_be_rdmi_dpcm  pre_raw_be_rdmi_dpcm;
	union reg_pre_wdma_ctrl_dummy           dummy;
	uint32_t                                _resv_0xc[12];
	union reg_pre_wdma_ctrl_debug_info      info;
	uint32_t                                _resv_0x40[12];
	union reg_pre_wdma_ctrl_wdma_enable     wdma_enable;
	union reg_pre_wdma_ctrl_wdma_insert     wdma_insert;
	union reg_pre_wdma_ctrl_wdma_insert_h   wdma_insert_h;
	union reg_pre_wdma_ctrl_wdma_insert_w   wdma_insert_w;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_ee_t {
	union reg_isp_ee_00                     reg_00;
	union reg_isp_ee_04                     reg_04;
	uint32_t                                _resv_0x8[1];
	union reg_isp_ee_0c                     reg_0c;
	union reg_isp_ee_10                     reg_10;
	uint32_t                                _resv_0x14[36];
	union reg_isp_ee_a4                     reg_a4;
	union reg_isp_ee_a8                     reg_a8;
	union reg_isp_ee_ac                     reg_ac;
	union reg_isp_ee_b0                     reg_b0;
	union reg_isp_ee_b4                     reg_b4;
	union reg_isp_ee_b8                     reg_b8;
	union reg_isp_ee_bc                     reg_bc;
	union reg_isp_ee_c0                     reg_c0;
	union reg_isp_ee_c4                     reg_c4;
	union reg_isp_ee_c8                     reg_c8;
	union reg_isp_ee_hcc                    reg_hcc;
	union reg_isp_ee_hd0                    reg_hd0;
	uint32_t                                _resv_0xd4[23];
	union reg_isp_ee_130                    reg_130;
	union reg_isp_ee_134                    reg_134;
	union reg_isp_ee_138                    reg_138;
	union reg_isp_ee_13c                    reg_13c;
	union reg_isp_ee_140                    reg_140;
	union reg_isp_ee_144                    reg_144;
	union reg_isp_ee_148                    reg_148;
	union reg_isp_ee_14c                    reg_14c;
	union reg_isp_ee_150                    reg_150;
	union reg_isp_ee_154                    reg_154;
	union reg_isp_ee_158                    reg_158;
	union reg_isp_ee_15c                    reg_15c;
	union reg_isp_ee_160                    reg_160;
	union reg_isp_ee_164                    reg_164;
	union reg_isp_ee_168                    reg_168;
	union reg_isp_ee_16c                    reg_16c;
	union reg_isp_ee_170                    reg_170;
	union reg_isp_ee_174                    reg_174;
	union reg_isp_ee_178                    reg_178;
	union reg_isp_ee_17c                    reg_17c;
	union reg_isp_ee_180                    reg_180;
	union reg_isp_ee_184                    reg_184;
	union reg_isp_ee_188                    reg_188;
	union reg_isp_ee_18c                    reg_18c;
	union reg_isp_ee_190                    reg_190;
	union reg_isp_ee_194                    reg_194;
	union reg_isp_ee_198                    reg_198;
	union reg_isp_ee_19c                    reg_19c;
	union reg_isp_ee_1a0                    reg_1a0;
	union reg_isp_ee_1a4                    reg_1a4;
	union reg_isp_ee_1a8                    reg_1a8;
	uint32_t                                _resv_0x1ac[6];
	union reg_isp_ee_1c4                    reg_1c4;
	union reg_isp_ee_1c8                    reg_1c8;
	union reg_isp_ee_1cc                    reg_1cc;
	union reg_isp_ee_1d0                    reg_1d0;
	union reg_isp_ee_1d4                    reg_1d4;
	union reg_isp_ee_1d8                    reg_1d8;
	union reg_isp_ee_1dc                    reg_1dc;
	union reg_isp_ee_1e0                    reg_1e0;
	union reg_isp_ee_1e4                    reg_1e4;
	union reg_isp_ee_1e8                    reg_1e8;
	union reg_isp_ee_1ec                    reg_1ec;
	union reg_isp_ee_1f0                    reg_1f0;
	union reg_isp_ee_1f4                    reg_1f4;
	union reg_isp_ee_1f8                    reg_1f8;
	union reg_isp_ee_1fc                    reg_1fc;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_ygamma_t {
	union reg_ygamma_gamma_ctrl             gamma_ctrl;
	union reg_ygamma_gamma_prog_ctrl        gamma_prog_ctrl;
	union reg_ygamma_gamma_prog_st_addr     gamma_prog_st_addr;
	union reg_ygamma_gamma_prog_data        gamma_prog_data;
	union reg_ygamma_gamma_prog_max         gamma_prog_max;
	union reg_ygamma_gamma_mem_sw_raddr     gamma_sw_raddr;
	union reg_ygamma_gamma_mem_sw_rdata     gamma_sw_rdata;
	union reg_ygamma_gamma_mem_sw_rdata_bg  gamma_sw_rdata_bg;
	union reg_ygamma_gamma_dbg              gamma_dbg;
	union reg_ygamma_gamma_dmy0             gamma_dmy0;
	union reg_ygamma_gamma_dmy1             gamma_dmy1;
	union reg_ygamma_gamma_dmy_r            gamma_dmy_r;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_preyee_t {
	union reg_isp_preyee_00                 reg_00;
	union reg_isp_preyee_04                 reg_04;
	uint32_t                                _resv_0x8[1];
	union reg_isp_preyee_0c                 reg_0c;
	union reg_isp_preyee_10                 reg_10;
	uint32_t                                _resv_0x14[36];
	union reg_isp_preyee_a4                 reg_a4;
	union reg_isp_preyee_a8                 reg_a8;
	union reg_isp_preyee_ac                 reg_ac;
	union reg_isp_preyee_b0                 reg_b0;
	union reg_isp_preyee_b4                 reg_b4;
	union reg_isp_preyee_b8                 reg_b8;
	union reg_isp_preyee_bc                 reg_bc;
	union reg_isp_preyee_c0                 reg_c0;
	union reg_isp_preyee_c4                 reg_c4;
	union reg_isp_preyee_c8                 reg_c8;
	union reg_isp_preyee_hcc                reg_hcc;
	union reg_isp_preyee_hd0                reg_hd0;
	uint32_t                                _resv_0xd4[23];
	union reg_isp_preyee_130                reg_130;
	union reg_isp_preyee_134                reg_134;
	union reg_isp_preyee_138                reg_138;
	union reg_isp_preyee_13c                reg_13c;
	union reg_isp_preyee_140                reg_140;
	union reg_isp_preyee_144                reg_144;
	union reg_isp_preyee_148                reg_148;
	union reg_isp_preyee_14c                reg_14c;
	union reg_isp_preyee_150                reg_150;
	union reg_isp_preyee_154                reg_154;
	union reg_isp_preyee_158                reg_158;
	union reg_isp_preyee_15c                reg_15c;
	union reg_isp_preyee_160                reg_160;
	union reg_isp_preyee_164                reg_164;
	union reg_isp_preyee_168                reg_168;
	union reg_isp_preyee_16c                reg_16c;
	union reg_isp_preyee_170                reg_170;
	union reg_isp_preyee_174                reg_174;
	union reg_isp_preyee_178                reg_178;
	union reg_isp_preyee_17c                reg_17c;
	union reg_isp_preyee_180                reg_180;
	union reg_isp_preyee_184                reg_184;
	union reg_isp_preyee_188                reg_188;
	union reg_isp_preyee_18c                reg_18c;
	union reg_isp_preyee_190                reg_190;
	union reg_isp_preyee_194                reg_194;
	union reg_isp_preyee_198                reg_198;
	union reg_isp_preyee_19c                reg_19c;
	union reg_isp_preyee_1a0                reg_1a0;
	union reg_isp_preyee_1a4                reg_1a4;
	union reg_isp_preyee_1a8                reg_1a8;
	uint32_t                                _resv_0x1ac[6];
	union reg_isp_preyee_1c4                reg_1c4;
	union reg_isp_preyee_1c8                reg_1c8;
	union reg_isp_preyee_1cc                reg_1cc;
	union reg_isp_preyee_1d0                reg_1d0;
	union reg_isp_preyee_1d4                reg_1d4;
	union reg_isp_preyee_1d8                reg_1d8;
	union reg_isp_preyee_1dc                reg_1dc;
	union reg_isp_preyee_1e0                reg_1e0;
	union reg_isp_preyee_1e4                reg_1e4;
	union reg_isp_preyee_1e8                reg_1e8;
	union reg_isp_preyee_1ec                reg_1ec;
	union reg_isp_preyee_1f0                reg_1f0;
	uint32_t                                _resv_0x1f4[2];
	union reg_isp_preyee_1fc                reg_1fc;
	union reg_isp_preyee_200                reg_200;
	union reg_isp_preyee_204                reg_204;
	union reg_isp_preyee_208                reg_208;
	union reg_isp_preyee_20c                reg_20c;
	union reg_isp_preyee_210                reg_210;
	union reg_isp_preyee_214                reg_214;
	union reg_isp_preyee_218                reg_218;
	union reg_isp_preyee_21c                reg_21c;
	union reg_isp_preyee_220                reg_220;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_csi_bdg_t {
	union reg_isp_csi_bdg_top_ctrl          csi_bdg_top_ctrl;
	union reg_isp_csi_bdg_interrupt_ctrl    csi_bdg_interrupt_ctrl;
	union reg_isp_csi_bdg_dma_dpcm_mode     csi_bdg_dma_dpcm_mode;
	union reg_isp_csi_bdg_dma_ld_dpcm_mode  csi_bdg_dma_ld_dpcm_mode;
	union reg_isp_csi_bdg_ch0_size          ch0_size;
	union reg_isp_csi_bdg_ch1_size          ch1_size;
	union reg_isp_csi_bdg_ch2_size          ch2_size;
	union reg_isp_csi_bdg_ch3_size          ch3_size;
	union reg_isp_csi_bdg_ch0_crop_en       ch0_crop_en;
	union reg_isp_csi_bdg_ch0_horz_crop     ch0_horz_crop;
	union reg_isp_csi_bdg_ch0_vert_crop     ch0_vert_crop;
	union reg_isp_csi_bdg_ch0_blc_sum       ch0_blc_sum;
	union reg_isp_csi_bdg_ch1_crop_en       ch1_crop_en;
	union reg_isp_csi_bdg_ch1_horz_crop     ch1_horz_crop;
	union reg_isp_csi_bdg_ch1_vert_crop     ch1_vert_crop;
	union reg_isp_csi_bdg_ch1_blc_sum       ch1_blc_sum;
	union reg_isp_csi_bdg_ch2_crop_en       ch2_crop_en;
	union reg_isp_csi_bdg_ch2_horz_crop     ch2_horz_crop;
	union reg_isp_csi_bdg_ch2_vert_crop     ch2_vert_crop;
	union reg_isp_csi_bdg_ch2_blc_sum       ch2_blc_sum;
	union reg_isp_csi_bdg_ch3_crop_en       ch3_crop_en;
	union reg_isp_csi_bdg_ch3_horz_crop     ch3_horz_crop;
	union reg_isp_csi_bdg_ch3_vert_crop     ch3_vert_crop;
	union reg_isp_csi_bdg_ch3_blc_sum       ch3_blc_sum;
	union reg_isp_csi_bdg_pat_gen_ctrl      csi_pat_gen_ctrl;
	union reg_isp_csi_bdg_pat_idx_ctrl      csi_pat_idx_ctrl;
	union reg_isp_csi_bdg_pat_color_0       csi_pat_color_0;
	union reg_isp_csi_bdg_pat_color_1       csi_pat_color_1;
	union reg_isp_csi_bdg_background_color_0  csi_background_color_0;
	union reg_isp_csi_bdg_background_color_1  csi_background_color_1;
	union reg_isp_csi_bdg_fix_color_0       csi_fix_color_0;
	union reg_isp_csi_bdg_fix_color_1       csi_fix_color_1;
	union reg_isp_csi_bdg_mde_v_size        csi_mde_v_size;
	union reg_isp_csi_bdg_mde_h_size        csi_mde_h_size;
	union reg_isp_csi_bdg_fde_v_size        csi_fde_v_size;
	union reg_isp_csi_bdg_fde_h_size        csi_fde_h_size;
	union reg_isp_csi_bdg_hsync_ctrl        csi_hsync_ctrl;
	union reg_isp_csi_bdg_vsync_ctrl        csi_vsync_ctrl;
	union reg_isp_csi_bdg_tgen_tt_size      csi_tgen_tt_size;
	union reg_isp_csi_bdg_line_intp_height_0  line_intp_height_0;
	union reg_isp_csi_bdg_ch0_debug_0       ch0_debug_0;
	union reg_isp_csi_bdg_ch0_debug_1       ch0_debug_1;
	union reg_isp_csi_bdg_ch0_debug_2       ch0_debug_2;
	union reg_isp_csi_bdg_ch0_debug_3       ch0_debug_3;
	union reg_isp_csi_bdg_ch1_debug_0       ch1_debug_0;
	union reg_isp_csi_bdg_ch1_debug_1       ch1_debug_1;
	union reg_isp_csi_bdg_ch1_debug_2       ch1_debug_2;
	union reg_isp_csi_bdg_ch1_debug_3       ch1_debug_3;
	union reg_isp_csi_bdg_ch2_debug_0       ch2_debug_0;
	union reg_isp_csi_bdg_ch2_debug_1       ch2_debug_1;
	union reg_isp_csi_bdg_ch2_debug_2       ch2_debug_2;
	union reg_isp_csi_bdg_ch2_debug_3       ch2_debug_3;
	union reg_isp_csi_bdg_ch3_debug_0       ch3_debug_0;
	union reg_isp_csi_bdg_ch3_debug_1       ch3_debug_1;
	union reg_isp_csi_bdg_ch3_debug_2       ch3_debug_2;
	union reg_isp_csi_bdg_ch3_debug_3       ch3_debug_3;
	union reg_isp_csi_bdg_interrupt_status_0  interrupt_status_0;
	union reg_isp_csi_bdg_interrupt_status_1  interrupt_status_1;
	union reg_isp_csi_bdg_debug             bdg_debug;
	union reg_isp_csi_bdg_out_vsync_line_delay  csi_out_vsync_line_delay;
	union reg_isp_csi_bdg_wr_urgent_ctrl    csi_wr_urgent_ctrl;
	union reg_isp_csi_bdg_rd_urgent_ctrl    csi_rd_urgent_ctrl;
	union reg_isp_csi_bdg_dummy             csi_dummy;
	union reg_isp_csi_bdg_line_intp_height_1  line_intp_height_1;
	union reg_isp_csi_bdg_slice_line_intp_height_0  slice_line_intp_height_0;
	union reg_isp_csi_bdg_slice_line_intp_height_1  slice_line_intp_height_1;
	uint32_t                                _resv_0x108[2];
	union reg_isp_csi_bdg_wdma_ch0_crop_en  wdma_ch0_crop_en;
	union reg_isp_csi_bdg_wdma_ch0_horz_crop  wdma_ch0_horz_crop;
	union reg_isp_csi_bdg_wdma_ch0_vert_crop  wdma_ch0_vert_crop;
	uint32_t                                _resv_0x11c[1];
	union reg_isp_csi_bdg_wdma_ch1_crop_en  wdma_ch1_crop_en;
	union reg_isp_csi_bdg_wdma_ch1_horz_crop  wdma_ch1_horz_crop;
	union reg_isp_csi_bdg_wdma_ch1_vert_crop  wdma_ch1_vert_crop;
	uint32_t                                _resv_0x12c[1];
	union reg_isp_csi_bdg_wdma_ch2_crop_en  wdma_ch2_crop_en;
	union reg_isp_csi_bdg_wdma_ch2_horz_crop  wdma_ch2_horz_crop;
	union reg_isp_csi_bdg_wdma_ch2_vert_crop  wdma_ch2_vert_crop;
	uint32_t                                _resv_0x13c[1];
	union reg_isp_csi_bdg_wdma_ch3_crop_en  wdma_ch3_crop_en;
	union reg_isp_csi_bdg_wdma_ch3_horz_crop  wdma_ch3_horz_crop;
	union reg_isp_csi_bdg_wdma_ch3_vert_crop  wdma_ch3_vert_crop;
	uint32_t                                _resv_0x14c[1];
	union reg_isp_csi_bdg_trig_dly_control_0  trig_dly_control_0;
	union reg_isp_csi_bdg_trig_dly_control_1  trig_dly_control_1;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_lcac_t {
	union reg_isp_lcac_reg00                reg00;
	union reg_isp_lcac_reg04                reg04;
	union reg_isp_lcac_reg08                reg08;
	union reg_isp_lcac_reg0c                reg0c;
	union reg_isp_lcac_reg10                reg10;
	union reg_isp_lcac_reg14                reg14;
	union reg_isp_lcac_reg18                reg18;
	union reg_isp_lcac_reg1c                reg1c;
	union reg_isp_lcac_reg20                reg20;
	union reg_isp_lcac_reg24                reg24;
	union reg_isp_lcac_reg28                reg28;
	union reg_isp_lcac_reg2c                reg2c;
	union reg_isp_lcac_reg30                reg30;
	union reg_isp_lcac_reg34                reg34;
	union reg_isp_lcac_reg38                reg38;
	union reg_isp_lcac_reg3c                reg3c;
	union reg_isp_lcac_reg40                reg40;
	union reg_isp_lcac_reg44                reg44;
	union reg_isp_lcac_reg48                reg48;
	union reg_isp_lcac_reg4c                reg4c;
	union reg_isp_lcac_reg50                reg50;
	union reg_isp_lcac_reg54                reg54;
	union reg_isp_lcac_reg58                reg58;
	union reg_isp_lcac_reg5c                reg5c;
	union reg_isp_lcac_reg60                reg60;
	union reg_isp_lcac_reg64                reg64;
	union reg_isp_lcac_reg68                reg68;
	union reg_isp_lcac_reg6c                reg6c;
	union reg_isp_lcac_reg70                reg70;
	union reg_isp_lcac_reg74                reg74;
	union reg_isp_lcac_reg78                reg78;
	union reg_isp_lcac_reg7c                reg7c;
	union reg_isp_lcac_reg80                reg80;
	union reg_isp_lcac_reg84                reg84;
	union reg_isp_lcac_reg88                reg88;
	union reg_isp_lcac_reg8c                reg8c;
	union reg_isp_lcac_reg90                reg90;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_cfa_t {
	union reg_isp_cfa_00                    reg_00;
	union reg_isp_cfa_04                    reg_04;
	uint32_t                                _resv_0x8[1];
	union reg_isp_cfa_0c                    reg_0c;
	union reg_isp_cfa_10                    reg_10;
	union reg_isp_cfa_14                    reg_14;
	union reg_isp_cfa_18                    reg_18;
	union reg_isp_cfa_1c                    reg_1c;
	union reg_isp_cfa_20                    reg_20;
	uint32_t                                _resv_0x24[3];
	union reg_isp_cfa_30                    reg_30;
	union reg_isp_cfa_34                    reg_34;
	union reg_isp_cfa_38                    reg_38;
	union reg_isp_cfa_3c                    reg_3c;
	union reg_isp_cfa_40                    reg_40;
	union reg_isp_cfa_44                    reg_44;
	union reg_isp_cfa_48                    reg_48;
	union reg_isp_cfa_4c                    reg_4c;
	uint32_t                                _resv_0x50[8];
	union reg_isp_cfa_70                    reg_70;
	union reg_isp_cfa_74                    reg_74;
	union reg_isp_cfa_78                    reg_78;
	union reg_isp_cfa_7c                    reg_7c;
	union reg_isp_cfa_80                    reg_80;
	uint32_t                                _resv_0x84[3];
	union reg_isp_cfa_90                    reg_90;
	union reg_isp_cfa_94                    reg_94;
	union reg_isp_cfa_98                    reg_98;
	union reg_isp_cfa_9c                    reg_9c;
	union reg_isp_cfa_a0                    reg_a0;
	union reg_isp_cfa_a4                    reg_a4;
	union reg_isp_cfa_a8                    reg_a8;
	uint32_t                                _resv_0xac[25];
	union reg_isp_cfa_110                   reg_110;
	uint32_t                                _resv_0x114[3];
	union reg_isp_cfa_120                   reg_120;
	union reg_isp_cfa_124                   reg_124;
	union reg_isp_cfa_128                   reg_128;
	union reg_isp_cfa_12c                   reg_12c;
	union reg_isp_cfa_130                   reg_130;
	union reg_isp_cfa_134                   reg_134;
	union reg_isp_cfa_138                   reg_138;
	union reg_isp_cfa_13c                   reg_13c;
	union reg_isp_cfa_140                   reg_140;
	union reg_isp_cfa_144                   reg_144;
	union reg_isp_cfa_148                   reg_148;
	union reg_isp_cfa_14c                   reg_14c;
	union reg_isp_cfa_150                   reg_150;
	union reg_isp_cfa_154                   reg_154;
	union reg_isp_cfa_158                   reg_158;
	union reg_isp_cfa_15c                   reg_15c;
	union reg_isp_cfa_160                   reg_160;
	union reg_isp_cfa_164                   reg_164;
	union reg_isp_cfa_168                   reg_168;
	union reg_isp_cfa_16c                   reg_16c;
	union reg_isp_cfa_170                   reg_170;
	union reg_isp_cfa_174                   reg_174;
	union reg_isp_cfa_178                   reg_178;
	union reg_isp_cfa_17c                   reg_17c;
	union reg_isp_cfa_180                   reg_180;
	union reg_isp_cfa_184                   reg_184;
	union reg_isp_cfa_188                   reg_188;
	union reg_isp_cfa_18c                   reg_18c;
	union reg_isp_cfa_190                   reg_190;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_pre_raw_vi_sel_t {
	union reg_pre_raw_vi_sel_0              reg_0;
	union reg_pre_raw_vi_sel_1              reg_1;
	uint32_t                                _resv_0x8[2];
	union reg_pre_raw_vi_sel_2              reg_2;
	union reg_pre_raw_vi_sel_3              reg_3;
	uint32_t                                _resv_0x18[2];
	union reg_pre_raw_vi_sel_4              reg_4;
	union reg_pre_raw_vi_sel_5              reg_5;
	union reg_pre_raw_vi_sel_6              reg_6;
	union reg_pre_raw_vi_sel_7              reg_7;
	union reg_pre_raw_vi_sel_8              reg_8;
	union reg_pre_raw_vi_sel_9              reg_9;
	union reg_pre_raw_vi_sel_10             reg_10;
	union reg_pre_raw_vi_sel_11             reg_11;
	union reg_pre_raw_vi_sel_12             reg_12;
	union reg_pre_raw_vi_sel_13             reg_13;
	union reg_pre_raw_vi_sel_14             reg_14;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_yuv_top_t {
	union reg_yuv_top_yuv_0                 yuv_0;
	uint32_t                                _resv_0x4[1];
	union reg_yuv_top_yuv_2                 yuv_2;
	union reg_yuv_top_yuv_3                 yuv_3;
	union reg_yuv_top_yuv_debug_0           yuv_debug_0;
	union reg_yuv_top_yuv_4                 yuv_4;
	union reg_yuv_top_yuv_debug_state       yuv_debug_state;
	uint32_t                                _resv_0x1c[1];
	union reg_yuv_top_yuv_5                 yuv_5;
	uint32_t                                _resv_0x24[15];
	union reg_yuv_top_yuv_ctrl              yuv_ctrl;
	union reg_yuv_top_imgw_m1               imgw_m1;
	uint32_t                                _resv_0x68[1];
	union reg_yuv_top_stvalid_status        stvalid_status;
	union reg_yuv_top_stready_status        stready_status;
	union reg_yuv_top_patgen1               patgen1;
	union reg_yuv_top_patgen2               patgen2;
	union reg_yuv_top_patgen3               patgen3;
	union reg_yuv_top_patgen4               patgen4;
	union reg_yuv_top_check_sum             check_sum;
	union reg_yuv_top_ai_isp_rdma_ctrl      ai_isp_rdma_ctrl;
	union reg_yuv_top_ai_isp_img_size_y     ai_isp_img_size_y;
	union reg_yuv_top_ai_isp_w_crop_y       ai_isp_w_crop_y;
	union reg_yuv_top_ai_isp_h_crop_y       ai_isp_h_crop_y;
	union reg_yuv_top_ai_isp_img_size_uv    ai_isp_img_size_uv;
	union reg_yuv_top_ai_isp_w_crop_uv      ai_isp_w_crop_uv;
	union reg_yuv_top_ai_isp_h_crop_uv      ai_isp_h_crop_uv;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_lsc_t {
	union reg_isp_lsc_status                lsc_status;
	union reg_isp_lsc_grace_reset           lsc_grace_reset;
	union reg_isp_lsc_monitor               lsc_monitor;
	union reg_isp_lsc_enable                lsc_enable;
	union reg_isp_lsc_kickoff               lsc_kickoff;
	union reg_isp_lsc_strength              lsc_strength;
	union reg_isp_lsc_img_bayerid           img_bayerid;
	union reg_isp_lsc_monitor_select        lsc_monitor_select;
	uint32_t                                _resv_0x20[2];
	union reg_isp_lsc_dmi_widthm1           lsc_dmi_widthm1;
	union reg_isp_lsc_dmi_heightm1          lsc_dmi_heightm1;
	uint32_t                                _resv_0x30[3];
	union reg_isp_lsc_gain_base             lsc_gain_base;
	union reg_isp_lsc_xstep                 lsc_xstep;
	union reg_isp_lsc_ystep                 lsc_ystep;
	union reg_isp_lsc_imgx0                 lsc_imgx0;
	union reg_isp_lsc_imgy0                 lsc_imgy0;
	uint32_t                                _resv_0x50[2];
	union reg_isp_lsc_initx0                lsc_initx0;
	union reg_isp_lsc_inity0                lsc_inity0;
	union reg_isp_lsc_kernel_table_write    lsc_kernel_table_write;
	union reg_isp_lsc_kernel_table_data     lsc_kernel_table_data;
	union reg_isp_lsc_kernel_table_ctrl     lsc_kernel_table_ctrl;
	union reg_isp_lsc_dummy                 lsc_dummy;
	union reg_isp_lsc_location              lsc_location;
	union reg_isp_lsc_1st_runhit            lsc_1st_runhit;
	union reg_isp_lsc_compare_value         lsc_compare_value;
	uint32_t                                _resv_0x7c[1];
	union reg_isp_lsc_mem_sw_mode           lsc_sw_mode;
	union reg_isp_lsc_mem_sw_raddr          lsc_sw_raddr;
	uint32_t                                _resv_0x88[1];
	union reg_isp_lsc_mem_sw_rdata          lsc_sw_rdata;
	union reg_isp_lsc_interpolation         interpolation;
	uint32_t                                _resv_0x94[3];
	union reg_isp_lsc_dmi_enable            dmi_enable;
	union reg_isp_lsc_bld                   lsc_bld;
	union reg_isp_lsc_intp_gain_max         lsc_intp_gain_max;
	union reg_isp_lsc_intp_gain_min         lsc_intp_gain_min;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_rgbir_t {
	union reg_isp_rgbir_ctrl                rgbir_ctrl;
	union reg_isp_rgbir_gain_offset_1       rgbir_gain_offset_1;
	union reg_isp_rgbir_gain_offset_2       rgbir_gain_offset_2;
	union reg_isp_rgbir_gain_offset_3       rgbir_gain_offset_3;
	union reg_isp_rgbir_gain_offset_4       rgbir_gain_offset_4;
	union reg_isp_rgbir_shdw_read_sel       rgbir_shdw_read_sel;
	union reg_isp_rgbir_comp_gain           rgbir_comp_gain;
	union reg_isp_rgbir_wdma_ctl            rbgir_wdma_ctl;
};

/******************************************/
/*           module definition            */
/******************************************/
struct reg_isp_line_spliter_t {
	union reg_isp_line_spliter_img_size_fe0     img_size_fe0;
	union reg_isp_line_spliter_img_width_nbld   img_width_nbld;
	union reg_isp_line_spliter_img_width_bld    img_width_bld;
	union reg_isp_line_spliter_img_size_nbld    img_size_nbld;
	union reg_isp_line_spliter_img_size_bld     img_size_bld;
	union reg_isp_line_spliter_img_size_fe1     img_size_fe1;
	union reg_isp_line_spliter_frame_size_fe0   frame_size_fe0;
	union reg_isp_line_spliter_frame_size_fe1   frame_size_fe1;
	union reg_isp_line_spliter_sel_ctrl         sel_ctrl;
	union reg_isp_line_spliter_dma_ctrl         dma_ctrl;
	union reg_isp_line_spliter_yuv_mode         yuv_mode;
	union reg_isp_line_spliter_enable           enable;
	union reg_isp_line_spliter_lese_arbiter_ctrl  lese_arbiter_ctrl;
	union reg_isp_line_spliter_vs_sw_ctrl       vs_sw_ctrl;
	union reg_isp_line_spliter_hdr_ctrl         hdr_ctrl;
	union reg_isp_line_spliter_frame_vld_ctrl   frame_vld_ctrl;
	union reg_isp_line_spliter_pol_ctrl         pol_ctrl;
	union reg_isp_line_spliter_status           status;
};

#ifdef __cplusplus
}
#endif

#endif /* _VI_REG_BLOCKS_H_ */

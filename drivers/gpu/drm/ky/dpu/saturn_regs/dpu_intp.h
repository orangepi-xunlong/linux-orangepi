// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef DPU_INTP_REG_H
#define DPU_INTP_REG_H


#define DPU_INT_CMDLIST_CH_FRM_CFG_DONE_MASK	(0x3fff)
#define DPU_INT_CMDLIST_CH_FRM_CFG_DONE		(DPU_INT_CMDLIST_CH_FRM_CFG_DONE_MASK << 12)
#define DPU_INT_WB_OVFLOW_MASK			(0x3)
#define DPU_INT_WB_OVFLOW			(DPU_INT_WB_OVFLOW_MASK << 10)
#define DPU_INT_FRM_TIMING_UNFLOW		BIT(9)
#define DPU_INT_WB_DONE_MASK			(0x3)
#define DPU_INT_WB_DONE				(DPU_INT_WB_DONE_MASK << 7)
#define DPU_INT_CURVE_DONE			BIT(6)
#define DPU_INT_HIST_DONE			BIT(5)
#define DPU_INT_CFG_RDY_CLR			BIT(4)
#define DPU_INT_FRM_TIMING_CFG_LINE		BIT(3)
#define DPU_INT_FRM_TIMING_CFG_EOF		BIT(2)
#define DPU_INT_FRM_TIMING_EOF			BIT(1)
#define DPU_INT_FRM_TIMING_VSYNC		BIT(0)

#define DPU_REST_INT_BITS			(DPU_INT_FRM_TIMING_CFG_EOF | \
						 DPU_INT_FRM_TIMING_CFG_LINE | \
						 DPU_INT_HIST_DONE | \
						 DPU_INT_CURVE_DONE | \
						 DPU_INT_CMDLIST_CH_FRM_CFG_DONE)

typedef union
{
	struct
	{
	//REGISTER dpu_int_reg_0
	UINT32 onl0_nml_frm_timing_vsync_int_msk          : 1 ;
	UINT32 onl0_nml_frm_timing_eof_int_msk            : 1 ;
	UINT32 onl0_nml_frm_timing_cfg_eof_int_msk        : 1 ;
	UINT32 onl0_nml_frm_timing_cfg_line_int_msk       : 1 ;
	UINT32 onl0_nml_cfg_rdy_clr_int_msk               : 1 ;
	UINT32 onl0_nml_hist_done_int_msk                 : 1 ;
	UINT32 onl0_nml_curve_done_int_msk                : 1 ;
	UINT32 onl0_nml_wb_done_int_msk                   : 2 ;
	UINT32 onl0_nml_frm_timing_unflow_int_msk         : 1 ;
	UINT32 onl0_nml_wb_ovflow_int_msk                 : 2 ;
	UINT32 onl0_nml_cmdlist_ch_frm_cfg_done_int_msk   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_1
	UINT32 onl0_nml_dma_dbg_int_msk                   : 16;
	UINT32 onl0_nml_outctl_dbg_int_msk                : 1 ;
	UINT32 onl0_nml_ctl_dbg_int_msk                   : 1 ;
	UINT32 onl0_nml_cmdlist_dbg_int_msk               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_2
	UINT32 onl1_nml_frm_timing_vsync_int_msk          : 1 ;
	UINT32 onl1_nml_frm_timing_eof_int_msk            : 1 ;
	UINT32 onl1_nml_frm_timing_cfg_eof_int_msk        : 1 ;
	UINT32 onl1_nml_frm_timing_cfg_line_int_msk       : 1 ;
	UINT32 onl1_nml_cfg_rdy_clr_int_msk               : 1 ;
	UINT32 onl1_nml_hist_done_int_msk                 : 1 ;
	UINT32 onl1_nml_curve_done_int_msk                : 1 ;
	UINT32 onl1_nml_wb_done_int_msk                   : 2 ;
	UINT32 onl1_nml_frm_timing_unflow_int_msk         : 1 ;
	UINT32 onl1_nml_wb_ovflow_int_msk                 : 2 ;
	UINT32 onl1_nml_cmdlist_ch_frm_cfg_done_int_msk   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_3
	UINT32 onl1_nml_dma_dbg_int_msk                   : 16;
	UINT32 onl1_nml_outctl_dbg_int_msk                : 1 ;
	UINT32 onl1_nml_ctl_dbg_int_msk                   : 1 ;
	UINT32 onl1_nml_cmdlist_dbg_int_msk               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_4
	UINT32 onl2_nml_frm_timing_vsync_int_msk          : 1 ;
	UINT32 onl2_nml_frm_timing_eof_int_msk            : 1 ;
	UINT32 onl2_nml_frm_timing_cfg_eof_int_msk        : 1 ;
	UINT32 onl2_nml_frm_timing_cfg_line_int_msk       : 1 ;
	UINT32 onl2_nml_cfg_rdy_clr_int_msk               : 1 ;
	UINT32 onl2_nml_hist_done_int_msk                 : 1 ;
	UINT32 onl2_nml_curve_done_int_msk                : 1 ;
	UINT32 onl2_nml_wb_done_int_msk                   : 2 ;
	UINT32 onl2_nml_frm_timing_unflow_int_msk         : 1 ;
	UINT32 onl2_nml_wb_ovflow_int_msk                 : 2 ;
	UINT32 onl2_nml_cmdlist_ch_frm_cfg_done_int_msk   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_5
	UINT32 onl2_nml_dma_dbg_int_msk                   : 16;
	UINT32 onl2_nml_outctl_dbg_int_msk                : 1 ;
	UINT32 onl2_nml_ctl_dbg_int_msk                   : 1 ;
	UINT32 onl2_nml_cmdlist_dbg_int_msk               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_6
	UINT32 offl0_cfg_rdy_clr_int_msk                  : 1 ;
	UINT32 offl0_wb_frm_done_int_msk                  : 2 ;
	UINT32 offl0_wb_slice_done_int_msk                : 2 ;
	UINT32 offl0_cmdlist_ch_frm_cfg_done_int_msk      : 14;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_7
	UINT32 offl0_nml_dma_dbg_int_msk                  : 16;
	UINT32 offl0_nml_ctl_dbg_int_msk                  : 1 ;
	UINT32 offl0_nml_cmdlist_dbg_int_msk              : 1 ;
	UINT32                                            : 14;


	//REGISTER dpu_int_reg_8
	UINT32 offl1_cfg_rdy_clr_int_msk                  : 1 ;
	UINT32 offl1_wb_frm_done_int_msk                  : 2 ;
	UINT32 offl1_wb_slice_done_int_msk                : 2 ;
	UINT32 offl1_cmdlist_ch_frm_cfg_done_int_msk      : 14;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_9
	UINT32 offl1_nml_dma_dbg_int_msk                  : 16;
	UINT32 offl1_nml_ctl_dbg_int_msk                  : 1 ;
	UINT32 offl1_nml_cmdlist_dbg_int_msk              : 1 ;
	UINT32                                            : 14;


	//REGISTER dpu_int_reg_10
	UINT32 onl0_nml_frm_timing_vsync_int_sts          : 1 ;
	UINT32 onl0_nml_frm_timing_eof_int_sts            : 1 ;
	UINT32 onl0_nml_frm_timing_cfg_eof_int_sts        : 1 ;
	UINT32 onl0_nml_frm_timing_cfg_line_int_sts       : 1 ;
	UINT32 onl0_nml_cfg_rdy_clr_int_sts               : 1 ;
	UINT32 onl0_nml_hist_done_int_sts                 : 1 ;
	UINT32 onl0_nml_curve_done_int_sts                : 1 ;
	UINT32 onl0_nml_wb_done_int_sts                   : 2 ;
	UINT32 onl0_nml_frm_timing_unflow_int_sts         : 1 ;
	UINT32 onl0_nml_wb_ovflow_int_sts                 : 2 ;
	UINT32 onl0_nml_cmdlist_ch_frm_cfg_done_int_sts   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_11
	UINT32 onl0_nml_dma_dbg_int_sts                   : 16;
	UINT32 onl0_nml_outctl_dbg_int_sts                : 1 ;
	UINT32 onl0_nml_ctl_dbg_int_sts                   : 1 ;
	UINT32 onl0_nml_cmdlist_dbg_int_sts               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_12
	UINT32 onl1_nml_frm_timing_vsync_int_sts          : 1 ;
	UINT32 onl1_nml_frm_timing_eof_int_sts            : 1 ;
	UINT32 onl1_nml_frm_timing_cfg_eof_int_sts        : 1 ;
	UINT32 onl1_nml_frm_timing_cfg_line_int_sts       : 1 ;
	UINT32 onl1_nml_cfg_rdy_clr_int_sts               : 1 ;
	UINT32 onl1_nml_hist_done_int_sts                 : 1 ;
	UINT32 onl1_nml_curve_done_int_sts                : 1 ;
	UINT32 onl1_nml_wb_done_int_sts                   : 2 ;
	UINT32 onl1_nml_frm_timing_unflow_int_sts         : 1 ;
	UINT32 onl1_nml_wb_ovflow_int_sts                 : 2 ;
	UINT32 onl1_nml_cmdlist_ch_frm_cfg_done_int_sts   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_13
	UINT32 onl1_nml_dma_dbg_int_sts                   : 16;
	UINT32 onl1_nml_outctl_dbg_int_sts                : 1 ;
	UINT32 onl1_nml_ctl_dbg_int_sts                   : 1 ;
	UINT32 onl1_nml_cmdlist_dbg_int_sts               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_14
	UINT32 onl2_nml_frm_timing_vsync_int_sts          : 1 ;
	UINT32 onl2_nml_frm_timing_eof_int_sts            : 1 ;
	UINT32 onl2_nml_frm_timing_cfg_eof_int_sts        : 1 ;
	UINT32 onl2_nml_frm_timing_cfg_line_int_sts       : 1 ;
	UINT32 onl2_nml_cfg_rdy_clr_int_sts               : 1 ;
	UINT32 onl2_nml_hist_done_int_sts                 : 1 ;
	UINT32 onl2_nml_curve_done_int_sts                : 1 ;
	UINT32 onl2_nml_wb_done_int_sts                   : 2 ;
	UINT32 onl2_nml_frm_timing_unflow_int_sts         : 1 ;
	UINT32 onl2_nml_wb_ovflow_int_sts                 : 2 ;
	UINT32 onl2_nml_cmdlist_ch_frm_cfg_done_int_sts   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_15
	UINT32 onl2_nml_dma_dbg_int_sts                   : 16;
	UINT32 onl2_nml_outctl_dbg_int_sts                : 1 ;
	UINT32 onl2_nml_ctl_dbg_int_sts                   : 1 ;
	UINT32 onl2_nml_cmdlist_dbg_int_sts               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_16
	UINT32 offl0_cfg_rdy_clr_int_sts                  : 1 ;
	UINT32 offl0_wb_frm_done_int_sts                  : 2 ;
	UINT32 offl0_wb_slice_done_int_sts                : 2 ;
	UINT32 offl0_cmdlist_ch_frm_cfg_done_int_sts      : 14;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_17
	UINT32 offl0_nml_dma_dbg_int_sts                  : 16;
	UINT32 offl0_nml_ctl_dbg_int_sts                  : 1 ;
	UINT32 offl0_nml_cmdlist_dbg_int_sts              : 1 ;
	UINT32                                            : 14;


	//REGISTER dpu_int_reg_18
	UINT32 offl1_cfg_rdy_clr_int_sts                  : 1 ;
	UINT32 offl1_wb_frm_done_int_sts                  : 2 ;
	UINT32 offl1_wb_slice_done_int_sts                : 2 ;
	UINT32 offl1_cmdlist_ch_frm_cfg_done_int_sts      : 14;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_19
	UINT32 offl1_nml_dma_dbg_int_sts                  : 16;
	UINT32 offl1_nml_ctl_dbg_int_sts                  : 1 ;
	UINT32 offl1_nml_cmdlist_dbg_int_sts              : 1 ;
	UINT32                                            : 14;


	//REGISTER dpu_int_reg_20
	UINT32 onl0_nml_frm_timing_vsync_int_raw          : 1 ;
	UINT32 onl0_nml_frm_timing_eof_int_raw            : 1 ;
	UINT32 onl0_nml_frm_timing_cfg_eof_int_raw        : 1 ;
	UINT32 onl0_nml_frm_timing_cfg_line_int_raw       : 1 ;
	UINT32 onl0_nml_cfg_rdy_clr_int_raw               : 1 ;
	UINT32 onl0_nml_hist_done_int_raw                 : 1 ;
	UINT32 onl0_nml_curve_done_int_raw                : 1 ;
	UINT32 onl0_nml_wb_done_int_raw                   : 2 ;
	UINT32 onl0_nml_frm_timing_unflow_int_raw         : 1 ;
	UINT32 onl0_nml_wb_ovflow_int_raw                 : 2 ;
	UINT32 onl0_nml_cmdlist_ch_frm_cfg_done_int_raw   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_21
	UINT32 onl0_nml_dma_dbg_int_raw                   : 16;
	UINT32 onl0_nml_outctl_dbg_int_raw                : 1 ;
	UINT32 onl0_nml_ctl_dbg_int_raw                   : 1 ;
	UINT32 onl0_nml_cmdlist_dbg_int_raw               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_22
	UINT32 onl1_nml_frm_timing_vsync_int_raw          : 1 ;
	UINT32 onl1_nml_frm_timing_eof_int_raw            : 1 ;
	UINT32 onl1_nml_frm_timing_cfg_eof_int_raw        : 1 ;
	UINT32 onl1_nml_frm_timing_cfg_line_int_raw       : 1 ;
	UINT32 onl1_nml_cfg_rdy_clr_int_raw               : 1 ;
	UINT32 onl1_nml_hist_done_int_raw                 : 1 ;
	UINT32 onl1_nml_curve_done_int_raw                : 1 ;
	UINT32 onl1_nml_wb_done_int_raw                   : 2 ;
	UINT32 onl1_nml_frm_timing_unflow_int_raw         : 1 ;
	UINT32 onl1_nml_wb_ovflow_int_raw                 : 2 ;
	UINT32 onl1_nml_cmdlist_ch_frm_cfg_done_int_raw   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_23
	UINT32 onl1_nml_dma_dbg_int_raw                   : 16;
	UINT32 onl1_nml_outctl_dbg_int_raw                : 1 ;
	UINT32 onl1_nml_ctl_dbg_int_raw                   : 1 ;
	UINT32 onl1_nml_cmdlist_dbg_int_raw               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_24
	UINT32 onl2_nml_frm_timing_vsync_int_raw          : 1 ;
	UINT32 onl2_nml_frm_timing_eof_int_raw            : 1 ;
	UINT32 onl2_nml_frm_timing_cfg_eof_int_raw        : 1 ;
	UINT32 onl2_nml_frm_timing_cfg_line_int_raw       : 1 ;
	UINT32 onl2_nml_cfg_rdy_clr_int_raw               : 1 ;
	UINT32 onl2_nml_hist_done_int_raw                 : 1 ;
	UINT32 onl2_nml_curve_done_int_raw                : 1 ;
	UINT32 onl2_nml_wb_done_int_raw                   : 2 ;
	UINT32 onl2_nml_frm_timing_unflow_int_raw         : 1 ;
	UINT32 onl2_nml_wb_ovflow_int_raw                 : 2 ;
	UINT32 onl2_nml_cmdlist_ch_frm_cfg_done_int_raw   : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_25
	UINT32 onl2_nml_dma_dbg_int_raw                   : 16;
	UINT32 onl2_nml_outctl_dbg_int_raw                : 1 ;
	UINT32 onl2_nml_ctl_dbg_int_raw                   : 1 ;
	UINT32 onl2_nml_cmdlist_dbg_int_raw               : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_26
	UINT32 offl0_cfg_rdy_clr_int_raw                  : 1 ;
	UINT32 offl0_wb_frm_done_int_raw                  : 2 ;
	UINT32 offl0_wb_slice_done_int_raw                : 2 ;
	UINT32 offl0_cmdlist_ch_frm_cfg_done_int_raw      : 14;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_27
	UINT32 offl0_nml_dma_dbg_int_raw                  : 16;
	UINT32 offl0_nml_ctl_dbg_int_raw                  : 1 ;
	UINT32 offl0_nml_cmdlist_dbg_int_raw              : 1 ;
	UINT32                                            : 14;


	//REGISTER dpu_int_reg_28
	UINT32 offl1_cfg_rdy_clr_int_raw                  : 1 ;
	UINT32 offl1_wb_frm_done_int_raw                  : 2 ;
	UINT32 offl1_wb_slice_done_int_raw                : 2 ;
	UINT32 offl1_cmdlist_ch_frm_cfg_done_int_raw      : 14;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_29
	UINT32 offl1_nml_dma_dbg_int_raw                  : 16;
	UINT32 offl1_nml_ctl_dbg_int_raw                  : 1 ;
	UINT32 offl1_nml_cmdlist_dbg_int_raw              : 1 ;
	UINT32                                            : 14;


	//REGISTER dpu_int_reg_30
	struct
	{
	UINT32                                            : 32;
	}dpu_int_reg_30[10];

	//REGISTER dpu_int_reg_40
	UINT32 onl0_secu_frm_timing_vsync_int_msk         : 1 ;
	UINT32 onl0_secu_frm_timing_eof_int_msk           : 1 ;
	UINT32 onl0_secu_frm_timing_cfg_eof_int_msk       : 1 ;
	UINT32 onl0_secu_frm_timing_cfg_line_int_msk      : 1 ;
	UINT32 onl0_secu_cfg_rdy_clr_int_msk              : 1 ;
	UINT32 onl0_secu_hist_done_int_msk                : 1 ;
	UINT32 onl0_secu_curve_done_int_msk               : 1 ;
	UINT32 onl0_secu_wb_done_int_msk                  : 2 ;
	UINT32 onl0_secu_frm_timing_unflow_int_msk        : 1 ;
	UINT32 onl0_secu_wb_ovflow_int_msk                : 2 ;
	UINT32 onl0_secu_cmdlist_ch_frm_cfg_done_int_msk  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_41
	UINT32 onl0_secu_dma_dbg_int_msk                  : 16;
	UINT32 onl0_secu_outctl_dbg_int_msk               : 1 ;
	UINT32 onl0_secu_ctl_dbg_int_msk                  : 1 ;
	UINT32 onl0_secu_cmdlist_dbg_int_msk              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_42
	UINT32 onl1_secu_frm_timing_vsync_int_msk         : 1 ;
	UINT32 onl1_secu_frm_timing_eof_int_msk           : 1 ;
	UINT32 onl1_secu_frm_timing_cfg_eof_int_msk       : 1 ;
	UINT32 onl1_secu_frm_timing_cfg_line_int_msk      : 1 ;
	UINT32 onl1_secu_cfg_rdy_clr_int_msk              : 1 ;
	UINT32 onl1_secu_hist_done_int_msk                : 1 ;
	UINT32 onl1_secu_curve_done_int_msk               : 1 ;
	UINT32 onl1_secu_wb_done_int_msk                  : 2 ;
	UINT32 onl1_secu_frm_timing_unflow_int_msk        : 1 ;
	UINT32 onl1_secu_wb_ovflow_int_msk                : 2 ;
	UINT32 onl1_secu_cmdlist_ch_frm_cfg_done_int_msk  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_43
	UINT32 onl1_secu_dma_dbg_int_msk                  : 16;
	UINT32 onl1_secu_outctl_dbg_int_msk               : 1 ;
	UINT32 onl1_secu_ctl_dbg_int_msk                  : 1 ;
	UINT32 onl1_secu_cmdlist_dbg_int_msk              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_44
	UINT32 onl2_secu_frm_timing_vsync_int_msk         : 1 ;
	UINT32 onl2_secu_frm_timing_eof_int_msk           : 1 ;
	UINT32 onl2_secu_frm_timing_cfg_eof_int_msk       : 1 ;
	UINT32 onl2_secu_frm_timing_cfg_line_int_msk      : 1 ;
	UINT32 onl2_secu_cfg_rdy_clr_int_msk              : 1 ;
	UINT32 onl2_secu_hist_done_int_msk                : 1 ;
	UINT32 onl2_secu_curve_done_int_msk               : 1 ;
	UINT32 onl2_secu_wb_done_int_msk                  : 2 ;
	UINT32 onl2_secu_frm_timing_unflow_int_msk        : 1 ;
	UINT32 onl2_secu_wb_ovflow_int_msk                : 2 ;
	UINT32 onl2_secu_cmdlist_ch_frm_cfg_done_int_msk  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_45
	UINT32 onl2_secu_dma_dbg_int_msk                  : 16;
	UINT32 onl2_secu_outctl_dbg_int_msk               : 1 ;
	UINT32 onl2_secu_ctl_dbg_int_msk                  : 1 ;
	UINT32 onl2_secu_cmdlist_dbg_int_msk              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_46
	UINT32 onl0_secu_frm_timing_vsync_int_sts         : 1 ;
	UINT32 onl0_secu_frm_timing_eof_int_sts           : 1 ;
	UINT32 onl0_secu_frm_timing_cfg_eof_int_sts       : 1 ;
	UINT32 onl0_secu_frm_timing_cfg_line_int_sts      : 1 ;
	UINT32 onl0_secu_cfg_rdy_clr_int_sts              : 1 ;
	UINT32 onl0_secu_hist_done_int_sts                : 1 ;
	UINT32 onl0_secu_curve_done_int_sts               : 1 ;
	UINT32 onl0_secu_wb_done_int_sts                  : 2 ;
	UINT32 onl0_secu_frm_timing_unflow_int_sts        : 1 ;
	UINT32 onl0_secu_wb_ovflow_int_sts                : 2 ;
	UINT32 onl0_secu_cmdlist_ch_frm_cfg_done_int_sts  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_47
	UINT32 onl0_secu_dma_dbg_int_sts                  : 16;
	UINT32 onl0_secu_outctl_dbg_int_sts               : 1 ;
	UINT32 onl0_secu_ctl_dbg_int_sts                  : 1 ;
	UINT32 onl0_secu_cmdlist_dbg_int_sts              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_48
	UINT32 onl1_secu_frm_timing_vsync_int_sts         : 1 ;
	UINT32 onl1_secu_frm_timing_eof_int_sts           : 1 ;
	UINT32 onl1_secu_frm_timing_cfg_eof_int_sts       : 1 ;
	UINT32 onl1_secu_frm_timing_cfg_line_int_sts      : 1 ;
	UINT32 onl1_secu_cfg_rdy_clr_int_sts              : 1 ;
	UINT32 onl1_secu_hist_done_int_sts                : 1 ;
	UINT32 onl1_secu_curve_done_int_sts               : 1 ;
	UINT32 onl1_secu_wb_done_int_sts                  : 2 ;
	UINT32 onl1_secu_frm_timing_unflow_int_sts        : 1 ;
	UINT32 onl1_secu_wb_ovflow_int_sts                : 2 ;
	UINT32 onl1_secu_cmdlist_ch_frm_cfg_done_int_sts  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_49
	UINT32 onl1_secu_dma_dbg_int_sts                  : 16;
	UINT32 onl1_secu_outctl_dbg_int_sts               : 1 ;
	UINT32 onl1_secu_ctl_dbg_int_sts                  : 1 ;
	UINT32 onl1_secu_cmdlist_dbg_int_sts              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_50
	UINT32 onl2_secu_frm_timing_vsync_int_sts         : 1 ;
	UINT32 onl2_secu_frm_timing_eof_int_sts           : 1 ;
	UINT32 onl2_secu_frm_timing_cfg_eof_int_sts       : 1 ;
	UINT32 onl2_secu_frm_timing_cfg_line_int_sts      : 1 ;
	UINT32 onl2_secu_cfg_rdy_clr_int_sts              : 1 ;
	UINT32 onl2_secu_hist_done_int_sts                : 1 ;
	UINT32 onl2_secu_curve_done_int_sts               : 1 ;
	UINT32 onl2_secu_wb_done_int_sts                  : 2 ;
	UINT32 onl2_secu_frm_timing_unflow_int_sts        : 1 ;
	UINT32 onl2_secu_wb_ovflow_int_sts                : 2 ;
	UINT32 onl2_secu_cmdlist_ch_frm_cfg_done_int_sts  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_51
	UINT32 onl2_secu_dma_dbg_int_sts                  : 16;
	UINT32 onl2_secu_outctl_dbg_int_sts               : 1 ;
	UINT32 onl2_secu_ctl_dbg_int_sts                  : 1 ;
	UINT32 onl2_secu_cmdlist_dbg_int_sts              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_52
	UINT32 onl0_secu_frm_timing_vsync_int_raw         : 1 ;
	UINT32 onl0_secu_frm_timing_eof_int_raw           : 1 ;
	UINT32 onl0_secu_frm_timing_cfg_eof_int_raw       : 1 ;
	UINT32 onl0_secu_frm_timing_cfg_line_int_raw      : 1 ;
	UINT32 onl0_secu_cfg_rdy_clr_int_raw              : 1 ;
	UINT32 onl0_secu_hist_done_int_raw                : 1 ;
	UINT32 onl0_secu_curve_done_int_raw               : 1 ;
	UINT32 onl0_secu_wb_done_int_raw                  : 2 ;
	UINT32 onl0_secu_frm_timing_unflow_int_raw        : 1 ;
	UINT32 onl0_secu_wb_ovflow_int_raw                : 2 ;
	UINT32 onl0_secu_cmdlist_ch_frm_cfg_done_int_raw  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_53
	UINT32 onl0_secu_dma_dbg_int_raw                  : 16;
	UINT32 onl0_secu_outctl_dbg_int_raw               : 1 ;
	UINT32 onl0_secu_ctl_dbg_int_raw                  : 1 ;
	UINT32 onl0_secu_cmdlist_dbg_int_raw              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_54
	UINT32 onl1_secu_frm_timing_vsync_int_raw         : 1 ;
	UINT32 onl1_secu_frm_timing_eof_int_raw           : 1 ;
	UINT32 onl1_secu_frm_timing_cfg_eof_int_raw       : 1 ;
	UINT32 onl1_secu_frm_timing_cfg_line_int_raw      : 1 ;
	UINT32 onl1_secu_cfg_rdy_clr_int_raw              : 1 ;
	UINT32 onl1_secu_hist_done_int_raw                : 1 ;
	UINT32 onl1_secu_curve_done_int_raw               : 1 ;
	UINT32 onl1_secu_wb_done_int_raw                  : 2 ;
	UINT32 onl1_secu_frm_timing_unflow_int_raw        : 1 ;
	UINT32 onl1_secu_wb_ovflow_int_raw                : 2 ;
	UINT32 onl1_secu_cmdlist_ch_frm_cfg_done_int_raw  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_55
	UINT32 onl1_secu_dma_dbg_int_raw                  : 16;
	UINT32 onl1_secu_outctl_dbg_int_raw               : 1 ;
	UINT32 onl1_secu_ctl_dbg_int_raw                  : 1 ;
	UINT32 onl1_secu_cmdlist_dbg_int_raw              : 1 ;
	UINT32                                            : 13;


	//REGISTER dpu_int_reg_56
	UINT32 onl2_secu_frm_timing_vsync_int_raw         : 1 ;
	UINT32 onl2_secu_frm_timing_eof_int_raw           : 1 ;
	UINT32 onl2_secu_frm_timing_cfg_eof_int_raw       : 1 ;
	UINT32 onl2_secu_frm_timing_cfg_line_int_raw      : 1 ;
	UINT32 onl2_secu_cfg_rdy_clr_int_raw              : 1 ;
	UINT32 onl2_secu_hist_done_int_raw                : 1 ;
	UINT32 onl2_secu_curve_done_int_raw               : 1 ;
	UINT32 onl2_secu_wb_done_int_raw                  : 2 ;
	UINT32 onl2_secu_frm_timing_unflow_int_raw        : 1 ;
	UINT32 onl2_secu_wb_ovflow_int_raw                : 2 ;
	UINT32 onl2_secu_cmdlist_ch_frm_cfg_done_int_raw  : 14;
	UINT32                                            : 6 ;


	//REGISTER dpu_int_reg_57
	UINT32 onl2_secu_dma_dbg_int_raw                  : 16;
	UINT32 onl2_secu_outctl_dbg_int_raw               : 1 ;
	UINT32 onl2_secu_ctl_dbg_int_raw                  : 1 ;
	UINT32 onl2_secu_cmdlist_dbg_int_raw              : 1 ;
	UINT32                                            : 13;


	} b;

	struct
	{
		UINT32 dpu_int_reg_0;
		UINT32 dpu_int_reg_1;
		UINT32 dpu_int_reg_2;
		UINT32 dpu_int_reg_3;
		UINT32 dpu_int_reg_4;
		UINT32 dpu_int_reg_5;
		UINT32 dpu_int_reg_6;
		UINT32 dpu_int_reg_7;
		UINT32 dpu_int_reg_8;
		UINT32 dpu_int_reg_9;
		UINT32 dpu_int_reg_10;
		UINT32 dpu_int_reg_11;
		UINT32 dpu_int_reg_12;
		UINT32 dpu_int_reg_13;
		UINT32 dpu_int_reg_14;
		UINT32 dpu_int_reg_15;
		UINT32 dpu_int_reg_16;
		UINT32 dpu_int_reg_17;
		UINT32 dpu_int_reg_18;
		UINT32 dpu_int_reg_19;
		UINT32 dpu_int_reg_20;
		UINT32 dpu_int_reg_21;
		UINT32 dpu_int_reg_22;
		UINT32 dpu_int_reg_23;
		UINT32 dpu_int_reg_24;
		UINT32 dpu_int_reg_25;
		UINT32 dpu_int_reg_26;
		UINT32 dpu_int_reg_27;
		UINT32 dpu_int_reg_28;
		UINT32 dpu_int_reg_29;
		UINT32 dpu_int_reg_30;
		UINT32 dpu_int_reg_31;
		UINT32 dpu_int_reg_32;
		UINT32 dpu_int_reg_33;
		UINT32 dpu_int_reg_34;
		UINT32 dpu_int_reg_35;
		UINT32 dpu_int_reg_36;
		UINT32 dpu_int_reg_37;
		UINT32 dpu_int_reg_38;
		UINT32 dpu_int_reg_39;
		UINT32 dpu_int_reg_40;
		UINT32 dpu_int_reg_41;
		UINT32 dpu_int_reg_42;
		UINT32 dpu_int_reg_43;
		UINT32 dpu_int_reg_44;
		UINT32 dpu_int_reg_45;
		UINT32 dpu_int_reg_46;
		UINT32 dpu_int_reg_47;
		UINT32 dpu_int_reg_48;
		UINT32 dpu_int_reg_49;
		UINT32 dpu_int_reg_50;
		UINT32 dpu_int_reg_51;
		UINT32 dpu_int_reg_52;
		UINT32 dpu_int_reg_53;
		UINT32 dpu_int_reg_54;
		UINT32 dpu_int_reg_55;
		UINT32 dpu_int_reg_56;
		UINT32 dpu_int_reg_57;
	} v;

}DPU_INTP_REG;
#endif

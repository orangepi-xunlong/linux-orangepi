// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef DPU_CTL_REG_H
#define DPU_CTL_REG_H

typedef union
{
	struct
	{
	//REGISTER dpu_ctl_reg_0
	UINT32 ctl0_nml_rch_en              : 12;
	UINT32 ctl0_nml_scl_en              : 4 ;
	UINT32 ctl0_nml_wb_en               : 2 ;
	UINT32 ctl0_nml_outctl_en           : 1 ;
	UINT32                              : 13;


	//REGISTER dpu_ctl_reg_1
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_2
	UINT32 ctl0_nml_cmd_updt_en         : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_3
	UINT32 ctl0_nml_cfg_rdy             : 1 ;
	UINT32 ctl0_sw_clr                  : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_4
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_5
	UINT32 ctl0_secu_rch_en             : 12;
	UINT32 ctl0_secu_scl_en             : 4 ;
	UINT32 ctl0_secu_wb_en              : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_6
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_7
	UINT32 ctl0_secu_cmd_updt_en        : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_8
	UINT32 ctl0_secu_cfg_rdy            : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_9
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_10
	UINT32 ctl0_video_mod               : 1 ;
	UINT32 ctl0_dbg_mod                 : 1 ;
	UINT32 ctl0_timing_inter0           : 4 ;
	UINT32                              : 2 ;
	UINT32 ctl0_timing_inter1           : 4 ;
	UINT32                              : 20;


	//REGISTER dpu_ctl_reg_11
	UINT32 ctl0_sw_start                : 1 ;
	UINT32 ctl0_dbg_unflow_clr          : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_12
	UINT32 ctl1_nml_rch_en              : 12;
	UINT32 ctl1_nml_scl_en              : 4 ;
	UINT32 ctl1_nml_wb_en               : 2 ;
	UINT32 ctl1_nml_outctl_en           : 1 ;
	UINT32                              : 13;


	//REGISTER dpu_ctl_reg_13
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_14
	UINT32 ctl1_nml_cmd_updt_en         : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_15
	UINT32 ctl1_nml_cfg_rdy             : 1 ;
	UINT32 ctl1_sw_clr                  : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_16
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_17
	UINT32 ctl1_secu_rch_en             : 12;
	UINT32 ctl1_secu_scl_en             : 4 ;
	UINT32 ctl1_secu_wb_en              : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_18
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_19
	UINT32 ctl1_secu_cmd_updt_en        : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_20
	UINT32 ctl1_secu_cfg_rdy            : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_21
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_22
	UINT32 ctl1_video_mod               : 1 ;
	UINT32 ctl1_dbg_mod                 : 1 ;
	UINT32 ctl1_timing_inter0           : 4 ;
	UINT32                              : 2 ;
	UINT32 ctl1_timing_inter1           : 4 ;
	UINT32                              : 20;


	//REGISTER dpu_ctl_reg_23
	UINT32 ctl1_sw_start                : 1 ;
	UINT32 ctl1_dbg_unflow_clr          : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_24
	UINT32 ctl2_nml_rch_en              : 12;
	UINT32 ctl2_nml_scl_en              : 4 ;
	UINT32 ctl2_nml_wb_en               : 2 ;
	UINT32 ctl2_nml_outctl_en           : 1 ;
	UINT32                              : 13;


	//REGISTER dpu_ctl_reg_25
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_26
	UINT32 ctl2_nml_cmd_updt_en         : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_27
	UINT32 ctl2_nml_cfg_rdy             : 1 ;
	UINT32 ctl2_sw_clr                  : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_28
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_29
	UINT32 ctl2_secu_rch_en             : 12;
	UINT32 ctl2_secu_scl_en             : 4 ;
	UINT32 ctl2_secu_wb_en              : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_30
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_31
	UINT32 ctl2_secu_cmd_updt_en        : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_32
	UINT32 ctl2_secu_cfg_rdy            : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_33
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_34
	UINT32 ctl2_video_mod               : 1 ;
	UINT32 ctl2_dbg_mod                 : 1 ;
	UINT32 ctl2_timing_inter0           : 4 ;
	UINT32                              : 2 ;
	UINT32 ctl2_timing_inter1           : 4 ;
	UINT32                              : 20;


	//REGISTER dpu_ctl_reg_35
	UINT32 ctl2_sw_start                : 1 ;
	UINT32 ctl2_dbg_unflow_clr          : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_36
	UINT32 ctl3_nml_rch_en              : 12;
	UINT32 ctl3_nml_scl_en              : 4 ;
	UINT32 ctl3_nml_wb_en               : 2 ;
	UINT32 ctl3_nml_outctl_en           : 1 ;
	UINT32                              : 13;


	//REGISTER dpu_ctl_reg_37
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_38
	UINT32 ctl3_nml_cmd_updt_en         : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_39
	UINT32 ctl3_nml_cfg_rdy             : 1 ;
	UINT32 ctl3_sw_clr                  : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_40
	UINT32 ctl3_video_mod               : 1 ;
	UINT32 ctl3_dbg_mod                 : 1 ;
	UINT32 ctl3_timing_inter0           : 4 ;
	UINT32                              : 2 ;
	UINT32 ctl3_timing_inter1           : 4 ;
	UINT32                              : 20;


	//REGISTER dpu_ctl_reg_41
	UINT32 ctl3_sw_start                : 1 ;
	UINT32 ctl3_dbg_unflow_clr          : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_42
	UINT32 ctl4_nml_rch_en              : 12;
	UINT32 ctl4_nml_scl_en              : 4 ;
	UINT32 ctl4_nml_wb_en               : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_43
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_44
	UINT32 ctl4_nml_cmd_updt_en         : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_45
	UINT32 ctl4_nml_cfg_rdy             : 1 ;
	UINT32 ctl4_sw_clr                  : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_46
	UINT32                              : 32;


	//REGISTER dpu_ctl_reg_47
	UINT32 ctl4_timing_inter0           : 4 ;
	UINT32 ctl4_timing_inter1           : 4 ;
	UINT32                              : 24;


	//REGISTER dpu_ctl_reg_48
	UINT32 ctl_nml_scl0_layer_id        : 4 ;
	UINT32 ctl_nml_scl0_layer_right     : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_49
	UINT32 ctl_nml_scl1_layer_id        : 4 ;
	UINT32 ctl_nml_scl1_layer_right     : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_50
	UINT32 ctl_nml_scl2_layer_id        : 4 ;
	UINT32 ctl_nml_scl2_layer_right     : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_51
	UINT32 ctl_nml_scl3_layer_id        : 4 ;
	UINT32 ctl_nml_scl3_layer_right     : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_52
	UINT32 ctl_secu_scl0_layer_id       : 4 ;
	UINT32 ctl_secu_scl0_layer_right    : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_53
	UINT32 ctl_secu_scl1_layer_id       : 4 ;
	UINT32 ctl_secu_scl1_layer_right    : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_54
	UINT32 ctl_secu_scl2_layer_id       : 4 ;
	UINT32 ctl_secu_scl2_layer_right    : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_55
	UINT32 ctl_secu_scl3_layer_id       : 4 ;
	UINT32 ctl_secu_scl3_layer_right    : 1 ;
	UINT32                              : 27;


	//REGISTER dpu_ctl_reg_56
	UINT32 outctl_secu                  : 3 ;
	UINT32 cmps_secu                    : 3 ;
	UINT32 prc_curve_secu               : 1 ;
	UINT32                              : 25;


	//REGISTER dpu_ctl_reg_57
	UINT32 ctl_rd_shadow                : 1 ;
	UINT32                              : 31;


	//REGISTER dpu_ctl_reg_58
	UINT32 rch_conflict_ints            : 12;
	UINT32 scl_conflict_ints            : 4 ;
	UINT32 wb_timeout_ints              : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_59
	UINT32 rch_conflict_ints_msk        : 12;
	UINT32 scl_conflict_int_msk         : 4 ;
	UINT32 wb_timeout_int_msk           : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_60
	UINT32 rch_conflict_int_raw         : 12;
	UINT32 scl_conflict_int_raw         : 4 ;
	UINT32 wb_timeout_int_raw           : 2 ;
	UINT32                              : 14;


	//REGISTER dpu_ctl_reg_61
	UINT32 ctl_nml_reuse_scl0_en        : 1 ;
	UINT32 ctl_nml_cmps_scl0_en         : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_62
	UINT32 ctl_nml_reuse_scl1_en        : 1 ;
	UINT32 ctl_nml_cmps_scl1_en         : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_63
	UINT32 ctl_nml_reuse_scl2_en        : 1 ;
	UINT32 ctl_nml_cmps_scl2_en         : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_64
	UINT32 ctl_nml_reuse_scl3_en        : 1 ;
	UINT32 ctl_nml_cmps_scl3_en         : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_65
	UINT32 ctl_secu_reuse_scl0_en       : 1 ;
	UINT32 ctl_secu_cmps_scl0_en        : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_66
	UINT32 ctl_secu_reuse_scl1_en       : 1 ;
	UINT32 ctl_secu_cmps_scl1_en        : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_67
	UINT32 ctl_secu_reuse_scl2_en       : 1 ;
	UINT32 ctl_secu_cmps_scl2_en        : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_68
	UINT32 ctl_secu_reuse_scl3_en       : 1 ;
	UINT32 ctl_secu_cmps_scl3_en        : 1 ;
	UINT32                              : 30;


	//REGISTER dpu_ctl_reg_69
	struct
	{
	UINT32 ctl_nml_cmdlist_rch_en       : 1 ;
	UINT32                              : 31;
	}dpu_ctl_reg_69[12];

	//REGISTER dpu_ctl_reg_81
	struct
	{
	UINT32 ctl_nml_cmdlist_wb_en        : 1 ;
	UINT32                              : 31;
	}dpu_ctl_reg_81[2];

	//REGISTER dpu_ctl_reg_83
	UINT32 ctl_nml_cmdlist_rch_cfg_rdy  : 12;
	UINT32 ctl_nml_cmdlist_wb_cfg_rdy   : 2 ;
	UINT32                              : 18;


	//REGISTER dpu_ctl_reg_84
	UINT32 ctl_wb0_sel_id               : 5 ;
	UINT32 ctl_wb0_sel_right            : 1 ;
	UINT32                              : 26;


	//REGISTER dpu_ctl_reg_85
	UINT32 ctl_wb1_sel_id               : 5 ;
	UINT32 ctl_wb1_sel_right            : 1 ;
	UINT32                              : 26;


	//REGISTER dpu_ctl_reg_86
	UINT32 ctl_rdma_act                 : 12;
	UINT32 ctl_scl_act                  : 4 ;
	UINT32 ctl_layer_act                : 12;
	UINT32                              : 4 ;


	//REGISTER dpu_ctl_reg_87
	UINT32 ctl_cmps_outctl_act          : 3 ;
	UINT32                              : 5 ;
	UINT32 ctl_wb_act                   : 2 ;
	UINT32                              : 6 ;
	UINT32 ctl_wb_slice_cnt             : 10;
	UINT32                              : 6 ;


	//REGISTER dpu_ctl_reg_88
	UINT32 cmdlist_rch_act              : 12;
	UINT32 cmdlist_wb_act               : 2 ;
	UINT32                              : 2 ;
	UINT32 scene_ctl_dbg0               : 15;
	UINT32                              : 1 ;


	//REGISTER dpu_ctl_reg_89
	UINT32 scene_ctl_dbg1               : 15;
	UINT32                              : 1 ;
	UINT32 scene_ctl_dbg2               : 15;
	UINT32                              : 1 ;


	//REGISTER dpu_ctl_reg_90
	UINT32 scene_ctl_dbg3               : 15;
	UINT32                              : 1 ;
	UINT32 scene_ctl_dbg4               : 15;
	UINT32                              : 1 ;


	//REGISTER dpu_ctl_reg_91
	UINT32 rdma_clr_req_aclk            : 12;
	UINT32 wb_clr_req_aclk              : 2 ;
	UINT32                              : 2 ;
	UINT32 rdma_clr_ack_aclk            : 12;
	UINT32 wb_clr_ack_aclk              : 2 ;
	UINT32                              : 2 ;


	//REGISTER dpu_ctl_reg_92
	UINT32 outctl_clr_req_aclk          : 3 ;
	UINT32                              : 1 ;
	UINT32 outctl_clr_ack_aclk          : 3 ;
	UINT32                              : 1 ;
	UINT32 wb_conflict_hld              : 2 ;
	UINT32                              : 22;


	//REGISTER dpu_ctl_reg_93
	UINT32 cmdlist_clr_req_aclk         : 14;
	UINT32                              : 2 ;
	UINT32 cmdlist_clr_ack_aclk         : 14;
	UINT32                              : 2 ;


	};

	INT32 value32[94];

}DPU_CTL_REG;

#endif


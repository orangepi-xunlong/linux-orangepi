// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef WB_TOP_X_REG_H
#define WB_TOP_X_REG_H

typedef union
{
	struct
	{
	//REGISTER wb_top_reg_0
	UINT32 wb_in_crop_en            : 1 ;
	UINT32 m_nwb_proc_en            : 1 ;
	UINT32 m_nmatrix_en             : 1 ;
	UINT32 m_noetf_en               : 1 ;
	UINT32 wb_en                    : 1 ;
	UINT32 wb_scl_en                : 1 ;
	UINT32                          : 2 ;
	UINT32 wb_rot_mode              : 3 ;
	UINT32 wb_wdma_fbc_en           : 1 ;
	UINT32 rsvd_unuse               : 1 ;
	UINT32                          : 3 ;
	UINT32 wb_out_format            : 5 ;
	UINT32                          : 11;


	//REGISTER wb_top_reg_1
	UINT32 wb_in_height             : 16;
	UINT32 wb_in_width              : 16;


	//REGISTER wb_top_reg_2
	UINT32 wb_out_ori_height        : 16;
	UINT32 wb_out_ori_width         : 16;


	//REGISTER wb_top_reg_3
	UINT32 wb_out_crop_ltopx        : 16;
	UINT32 wb_out_crop_ltopy        : 16;


	//REGISTER wb_top_reg_4
	UINT32 wb_out_crop_height       : 16;
	UINT32 wb_out_crop_width        : 16;


	//REGISTER wb_top_reg_5
	UINT32 wb_wdma_base_addr0_low   : 32;


	//REGISTER wb_top_reg_6
	UINT32 wb_wdma_base_addr0_high  : 2 ;
	UINT32                          : 30;


	//REGISTER wb_top_reg_7
	UINT32 wb_wdma_base_addr1_low   : 32;


	//REGISTER wb_top_reg_8
	UINT32 wb_wdma_base_addr1_high  : 2 ;
	UINT32                          : 6 ;
	UINT32 wb_wdma_stride           : 16;
	UINT32 wb_wdma_outstanding_num  : 5 ;
	UINT32                          : 3 ;


	//REGISTER wb_top_reg_9
	UINT32 wb_wdma_burst_len        : 5 ;
	UINT32 wb_wdma_axi_port         : 2 ;
	UINT32                          : 5 ;
	UINT32 wb_wdma_cache            : 4 ;
	UINT32 wb_wdma_region           : 4 ;
	UINT32 wb_wdma_qos              : 4 ;
	UINT32                          : 8 ;


	//REGISTER wb_top_reg_10
	UINT32 copy_mode_enable         : 1 ;
	UINT32 default_color_enable     : 1 ;
	UINT32 yuv_transform_en         : 1 ;
	UINT32 fbc_split_en             : 1 ;
	UINT32 fbc_tile_hd_mode_en      : 1 ;
	UINT32                          : 27;


	//REGISTER wb_top_reg_11
	UINT32 fmt_cvt_A0               : 16;
	UINT32 fmt_cvt_A1               : 16;


	//REGISTER wb_top_reg_12
	UINT32 fmt_cvt_A2               : 16;
	UINT32 fmt_cvt_narrow_mode      : 1 ;
	UINT32                          : 15;


	//REGISTER wb_top_reg_13
	UINT32 fmt_cvt_matrix00         : 14;
	UINT32                          : 2 ;
	UINT32 fmt_cvt_matrix01         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_14
	UINT32 fmt_cvt_matrix02         : 14;
	UINT32                          : 2 ;
	UINT32 fmt_cvt_matrix03         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_15
	UINT32 fmt_cvt_matrix10         : 14;
	UINT32                          : 2 ;
	UINT32 fmt_cvt_matrix11         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_16
	UINT32 fmt_cvt_matrix12         : 14;
	UINT32                          : 2 ;
	UINT32 fmt_cvt_matrix13         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_17
	UINT32 fmt_cvt_matrix20         : 14;
	UINT32                          : 2 ;
	UINT32 fmt_cvt_matrix21         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_18
	UINT32 fmt_cvt_matrix22         : 14;
	UINT32                          : 2 ;
	UINT32 fmt_cvt_matrix23         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_19
	UINT32 wb_dither_en             : 1 ;
	UINT32 wb_dither_auto_temp_en   : 1 ;
	UINT32 wb_dither_rot_mode       : 2 ;
	UINT32 wb_dither_mode           : 1 ;
	UINT32                          : 3 ;
	UINT32 wb_dither_out_dpth0      : 4 ;
	UINT32 wb_dither_out_dpth1      : 4 ;
	UINT32 wb_dither_out_dpth2      : 4 ;
	UINT32                          : 4 ;
	UINT32 wb_dither_tmp_value      : 8 ;


	//REGISTER wb_top_reg_20
	UINT32 wb_dither_pattern_bits   : 3 ;
	UINT32                          : 29;


	//REGISTER wb_top_reg_21
	UINT32 wb_dither_bayer_map0     : 8 ;
	UINT32 wb_dither_bayer_map1     : 8 ;
	UINT32 wb_dither_bayer_map2     : 8 ;
	UINT32 wb_dither_bayer_map3     : 8 ;


	//REGISTER wb_top_reg_22
	UINT32 wb_dither_bayer_map4     : 8 ;
	UINT32 wb_dither_bayer_map5     : 8 ;
	UINT32 wb_dither_bayer_map6     : 8 ;
	UINT32 wb_dither_bayer_map7     : 8 ;


	//REGISTER wb_top_reg_23
	UINT32 wb_dither_bayer_map8     : 8 ;
	UINT32 wb_dither_bayer_map9     : 8 ;
	UINT32 wb_dither_bayer_map10    : 8 ;
	UINT32 wb_dither_bayer_map11    : 8 ;


	//REGISTER wb_top_reg_24
	UINT32 wb_dither_bayer_map12    : 8 ;
	UINT32 wb_dither_bayer_map13    : 8 ;
	UINT32 wb_dither_bayer_map14    : 8 ;
	UINT32 wb_dither_bayer_map15    : 8 ;


	//REGISTER wb_top_reg_25
	UINT32 wb_dither_bayer_map16    : 8 ;
	UINT32 wb_dither_bayer_map17    : 8 ;
	UINT32 wb_dither_bayer_map18    : 8 ;
	UINT32 wb_dither_bayer_map19    : 8 ;


	//REGISTER wb_top_reg_26
	UINT32 wb_dither_bayer_map20    : 8 ;
	UINT32 wb_dither_bayer_map21    : 8 ;
	UINT32 wb_dither_bayer_map22    : 8 ;
	UINT32 wb_dither_bayer_map23    : 8 ;


	//REGISTER wb_top_reg_27
	UINT32 wb_dither_bayer_map24    : 8 ;
	UINT32 wb_dither_bayer_map25    : 8 ;
	UINT32 wb_dither_bayer_map26    : 8 ;
	UINT32 wb_dither_bayer_map27    : 8 ;


	//REGISTER wb_top_reg_28
	UINT32 wb_dither_bayer_map28    : 8 ;
	UINT32 wb_dither_bayer_map29    : 8 ;
	UINT32 wb_dither_bayer_map30    : 8 ;
	UINT32 wb_dither_bayer_map31    : 8 ;


	//REGISTER wb_top_reg_29
	UINT32 wb_dither_bayer_map32    : 8 ;
	UINT32 wb_dither_bayer_map33    : 8 ;
	UINT32 wb_dither_bayer_map34    : 8 ;
	UINT32 wb_dither_bayer_map35    : 8 ;


	//REGISTER wb_top_reg_30
	UINT32 wb_dither_bayer_map36    : 8 ;
	UINT32 wb_dither_bayer_map37    : 8 ;
	UINT32 wb_dither_bayer_map38    : 8 ;
	UINT32 wb_dither_bayer_map39    : 8 ;


	//REGISTER wb_top_reg_31
	UINT32 wb_dither_bayer_map40    : 8 ;
	UINT32 wb_dither_bayer_map41    : 8 ;
	UINT32 wb_dither_bayer_map42    : 8 ;
	UINT32 wb_dither_bayer_map43    : 8 ;


	//REGISTER wb_top_reg_32
	UINT32 wb_dither_bayer_map44    : 8 ;
	UINT32 wb_dither_bayer_map45    : 8 ;
	UINT32 wb_dither_bayer_map46    : 8 ;
	UINT32 wb_dither_bayer_map47    : 8 ;


	//REGISTER wb_top_reg_33
	UINT32 wb_dither_bayer_map48    : 8 ;
	UINT32 wb_dither_bayer_map49    : 8 ;
	UINT32 wb_dither_bayer_map50    : 8 ;
	UINT32 wb_dither_bayer_map51    : 8 ;


	//REGISTER wb_top_reg_34
	UINT32 wb_dither_bayer_map52    : 8 ;
	UINT32 wb_dither_bayer_map53    : 8 ;
	UINT32 wb_dither_bayer_map54    : 8 ;
	UINT32 wb_dither_bayer_map55    : 8 ;


	//REGISTER wb_top_reg_35
	UINT32 wb_dither_bayer_map56    : 8 ;
	UINT32 wb_dither_bayer_map57    : 8 ;
	UINT32 wb_dither_bayer_map58    : 8 ;
	UINT32 wb_dither_bayer_map59    : 8 ;


	//REGISTER wb_top_reg_36
	UINT32 wb_dither_bayer_map60    : 8 ;
	UINT32 wb_dither_bayer_map61    : 8 ;
	UINT32 wb_dither_bayer_map62    : 8 ;
	UINT32 wb_dither_bayer_map63    : 8 ;


	//REGISTER wb_top_reg_37
	UINT32 wb_in_crop_ltopx         : 16;
	UINT32 wb_in_crop_ltopy         : 16;


	//REGISTER wb_top_reg_38
	UINT32 wb_in_crop_rbotx         : 16;
	UINT32 wb_in_crop_rboty         : 16;


	//REGISTER wb_top_reg_39
	UINT32 m_noetf_mode             : 3 ;
	UINT32                          : 5 ;
	UINT32 m_noetf_max              : 12;
	UINT32                          : 12;


	//REGISTER wb_top_reg_40
	UINT32 m_pmatrix_table0         : 14;
	UINT32                          : 2 ;
	UINT32 m_pmatrix_table1         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_41
	UINT32 m_pmatrix_table2         : 14;
	UINT32                          : 2 ;
	UINT32 m_pmatrix_table3         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_42
	UINT32 m_pmatrix_table4         : 14;
	UINT32                          : 2 ;
	UINT32 m_pmatrix_table5         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_43
	UINT32 m_pmatrix_table6         : 14;
	UINT32                          : 2 ;
	UINT32 m_pmatrix_table7         : 14;
	UINT32                          : 2 ;


	//REGISTER wb_top_reg_44
	UINT32 m_pmatrix_table8         : 14;
	UINT32                          : 18;


	//REGISTER wb_top_reg_45
	UINT32 m_pmatrix_offset0        : 25;
	UINT32                          : 7 ;


	//REGISTER wb_top_reg_46
	UINT32 m_pmatrix_offset1        : 25;
	UINT32                          : 7 ;


	//REGISTER wb_top_reg_47
	UINT32 m_pmatrix_offset2        : 25;
	UINT32                          : 7 ;


	//REGISTER wb_top_reg_48
	UINT32 dbug_bus0                : 32;


	//REGISTER wb_top_reg_49
	UINT32 dbug_bus1                : 32;


	//REGISTER wb_top_reg_50
	UINT32 dbug_bus2                : 32;


	//REGISTER wb_top_reg_51
	UINT32 dbug_bus3                : 32;


	//REGISTER wb_top_reg_52
	UINT32 dbug_bus4                : 32;


	//REGISTER wb_top_reg_53
	UINT32 dbug_bus5                : 32;


	//REGISTER wb_top_reg_54
	UINT32 wb_wdma_nsaid            : 4 ;
	UINT32                          : 28;


	};

	INT32 value32[55];

}WB_TOP_REG;

#endif


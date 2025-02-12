// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef OUTCTRL_PROC_X_REG_H
#define OUTCTRL_PROC_X_REG_H

typedef union
{
	struct
	{
	//REGISTER Post_proc_reg_0
	UINT32 m_npost_proc_en               : 1 ;
	UINT32 m_ngain_to_full_en            : 1 ;
	UINT32 m_nmatrix_en                  : 1 ;
	UINT32 m_nendmatrix_en               : 1 ;
	UINT32 m_nfront_tmootf_en            : 1 ;
	UINT32 m_nend_tmootf_en              : 1 ;
	UINT32 m_neotf_en                    : 1 ;
	UINT32 m_noetf_en                    : 1 ;
	UINT32                               : 24;


	//REGISTER Post_proc_reg_1
	UINT32 m_neotf_mode                  : 3 ;
	UINT32                               : 29;


	//REGISTER Post_proc_reg_2
	UINT32 m_noetf_mode                  : 3 ;
	UINT32                               : 5 ;
	UINT32 m_noetf_max                   : 12;
	UINT32                               : 12;


	//REGISTER Post_proc_reg_3
	UINT32 m_pfront_tmootf_gain_table0   : 16;
	UINT32 m_pfront_tmootf_gain_table1   : 16;


	//REGISTER Post_proc_reg_4
	UINT32 m_pfront_tmootf_gain_table2   : 16;
	UINT32 m_pfront_tmootf_gain_table3   : 16;


	//REGISTER Post_proc_reg_5
	UINT32 m_pfront_tmootf_gain_table4   : 16;
	UINT32 m_pfront_tmootf_gain_table5   : 16;


	//REGISTER Post_proc_reg_6
	UINT32 m_pfront_tmootf_gain_table6   : 16;
	UINT32 m_pfront_tmootf_gain_table7   : 16;


	//REGISTER Post_proc_reg_7
	UINT32 m_pfront_tmootf_gain_table8   : 16;
	UINT32 m_pfront_tmootf_gain_table9   : 16;


	//REGISTER Post_proc_reg_8
	UINT32 m_pfront_tmootf_gain_table10  : 16;
	UINT32 m_pfront_tmootf_gain_table11  : 16;


	//REGISTER Post_proc_reg_9
	UINT32 m_pfront_tmootf_gain_table12  : 16;
	UINT32 m_pfront_tmootf_gain_table13  : 16;


	//REGISTER Post_proc_reg_10
	UINT32 m_pfront_tmootf_gain_table14  : 16;
	UINT32 m_pfront_tmootf_gain_table15  : 16;


	//REGISTER Post_proc_reg_11
	UINT32 m_pfront_tmootf_gain_table16  : 16;
	UINT32 m_pfront_tmootf_gain_table17  : 16;


	//REGISTER Post_proc_reg_12
	UINT32 m_pfront_tmootf_gain_table18  : 16;
	UINT32 m_pfront_tmootf_gain_table19  : 16;


	//REGISTER Post_proc_reg_13
	UINT32 m_pfront_tmootf_gain_table20  : 16;
	UINT32 m_pfront_tmootf_gain_table21  : 16;


	//REGISTER Post_proc_reg_14
	UINT32 m_pfront_tmootf_gain_table22  : 16;
	UINT32 m_pfront_tmootf_gain_table23  : 16;


	//REGISTER Post_proc_reg_15
	UINT32 m_pfront_tmootf_gain_table24  : 16;
	UINT32 m_pfront_tmootf_gain_table25  : 16;


	//REGISTER Post_proc_reg_16
	UINT32 m_pfront_tmootf_gain_table26  : 16;
	UINT32 m_pfront_tmootf_gain_table27  : 16;


	//REGISTER Post_proc_reg_17
	UINT32 m_pfront_tmootf_gain_table28  : 16;
	UINT32 m_pfront_tmootf_gain_table29  : 16;


	//REGISTER Post_proc_reg_18
	UINT32 m_pfront_tmootf_gain_table30  : 16;
	UINT32 m_pfront_tmootf_gain_table31  : 16;


	//REGISTER Post_proc_reg_19
	UINT32 m_pfront_tmootf_gain_table32  : 16;
	UINT32 m_pfront_tmootf_gain_table33  : 16;


	//REGISTER Post_proc_reg_20
	UINT32 m_pfront_tmootf_gain_table34  : 16;
	UINT32 m_pfront_tmootf_gain_table35  : 16;


	//REGISTER Post_proc_reg_21
	UINT32 m_pfront_tmootf_gain_table36  : 16;
	UINT32 m_pfront_tmootf_gain_table37  : 16;


	//REGISTER Post_proc_reg_22
	UINT32 m_pfront_tmootf_gain_table38  : 16;
	UINT32 m_pfront_tmootf_gain_table39  : 16;


	//REGISTER Post_proc_reg_23
	UINT32 m_pfront_tmootf_gain_table40  : 16;
	UINT32 m_pfront_tmootf_gain_table41  : 16;


	//REGISTER Post_proc_reg_24
	UINT32 m_pfront_tmootf_gain_table42  : 16;
	UINT32 m_pfront_tmootf_gain_table43  : 16;


	//REGISTER Post_proc_reg_25
	UINT32 m_pfront_tmootf_gain_table44  : 16;
	UINT32 m_pfront_tmootf_gain_table45  : 16;


	//REGISTER Post_proc_reg_26
	UINT32 m_pfront_tmootf_gain_table46  : 16;
	UINT32 m_pfront_tmootf_gain_table47  : 16;


	//REGISTER Post_proc_reg_27
	UINT32 m_pfront_tmootf_gain_table48  : 16;
	UINT32 m_pfront_tmootf_gain_table49  : 16;


	//REGISTER Post_proc_reg_28
	UINT32 m_pfront_tmootf_gain_table50  : 16;
	UINT32 m_pfront_tmootf_gain_table51  : 16;


	//REGISTER Post_proc_reg_29
	UINT32 m_pfront_tmootf_gain_table52  : 16;
	UINT32 m_pfront_tmootf_gain_table53  : 16;


	//REGISTER Post_proc_reg_30
	UINT32 m_pfront_tmootf_gain_table54  : 16;
	UINT32 m_pfront_tmootf_gain_table55  : 16;


	//REGISTER Post_proc_reg_31
	UINT32 m_pfront_tmootf_gain_table56  : 16;
	UINT32 m_pfront_tmootf_gain_table57  : 16;


	//REGISTER Post_proc_reg_32
	UINT32 m_pfront_tmootf_gain_table58  : 16;
	UINT32 m_pfront_tmootf_gain_table59  : 16;


	//REGISTER Post_proc_reg_33
	UINT32 m_pfront_tmootf_gain_table60  : 16;
	UINT32 m_pfront_tmootf_gain_table61  : 16;


	//REGISTER Post_proc_reg_34
	UINT32 m_pfront_tmootf_gain_table62  : 16;
	UINT32 m_pfront_tmootf_gain_table63  : 16;


	//REGISTER Post_proc_reg_35
	UINT32 m_pfront_tmootf_gain_table64  : 16;
	UINT32 m_nfront_tmootf_shift_bits    : 5 ;
	UINT32 m_nfront_tmootf_rgb_mode      : 2 ;
	UINT32                               : 9 ;


	//REGISTER Post_proc_reg_36
	UINT32 m_pend_tmootf_gain_table0     : 16;
	UINT32 m_pend_tmootf_gain_table1     : 16;


	//REGISTER Post_proc_reg_37
	UINT32 m_pend_tmootf_gain_table2     : 16;
	UINT32 m_pend_tmootf_gain_table3     : 16;


	//REGISTER Post_proc_reg_38
	UINT32 m_pend_tmootf_gain_table4     : 16;
	UINT32 m_pend_tmootf_gain_table5     : 16;


	//REGISTER Post_proc_reg_39
	UINT32 m_pend_tmootf_gain_table6     : 16;
	UINT32 m_pend_tmootf_gain_table7     : 16;


	//REGISTER Post_proc_reg_40
	UINT32 m_pend_tmootf_gain_table8     : 16;
	UINT32 m_pend_tmootf_gain_table9     : 16;


	//REGISTER Post_proc_reg_41
	UINT32 m_pend_tmootf_gain_table10    : 16;
	UINT32 m_pend_tmootf_gain_table11    : 16;


	//REGISTER Post_proc_reg_42
	UINT32 m_pend_tmootf_gain_table12    : 16;
	UINT32 m_pend_tmootf_gain_table13    : 16;


	//REGISTER Post_proc_reg_43
	UINT32 m_pend_tmootf_gain_table14    : 16;
	UINT32 m_pend_tmootf_gain_table15    : 16;


	//REGISTER Post_proc_reg_44
	UINT32 m_pend_tmootf_gain_table16    : 16;
	UINT32 m_pend_tmootf_gain_table17    : 16;


	//REGISTER Post_proc_reg_45
	UINT32 m_pend_tmootf_gain_table18    : 16;
	UINT32 m_pend_tmootf_gain_table19    : 16;


	//REGISTER Post_proc_reg_46
	UINT32 m_pend_tmootf_gain_table20    : 16;
	UINT32 m_pend_tmootf_gain_table21    : 16;


	//REGISTER Post_proc_reg_47
	UINT32 m_pend_tmootf_gain_table22    : 16;
	UINT32 m_pend_tmootf_gain_table23    : 16;


	//REGISTER Post_proc_reg_48
	UINT32 m_pend_tmootf_gain_table24    : 16;
	UINT32 m_pend_tmootf_gain_table25    : 16;


	//REGISTER Post_proc_reg_49
	UINT32 m_pend_tmootf_gain_table26    : 16;
	UINT32 m_pend_tmootf_gain_table27    : 16;


	//REGISTER Post_proc_reg_50
	UINT32 m_pend_tmootf_gain_table28    : 16;
	UINT32 m_pend_tmootf_gain_table29    : 16;


	//REGISTER Post_proc_reg_51
	UINT32 m_pend_tmootf_gain_table30    : 16;
	UINT32 m_pend_tmootf_gain_table31    : 16;


	//REGISTER Post_proc_reg_52
	UINT32 m_pend_tmootf_gain_table32    : 16;
	UINT32 m_pend_tmootf_gain_table33    : 16;


	//REGISTER Post_proc_reg_53
	UINT32 m_pend_tmootf_gain_table34    : 16;
	UINT32 m_pend_tmootf_gain_table35    : 16;


	//REGISTER Post_proc_reg_54
	UINT32 m_pend_tmootf_gain_table36    : 16;
	UINT32 m_pend_tmootf_gain_table37    : 16;


	//REGISTER Post_proc_reg_55
	UINT32 m_pend_tmootf_gain_table38    : 16;
	UINT32 m_pend_tmootf_gain_table39    : 16;


	//REGISTER Post_proc_reg_56
	UINT32 m_pend_tmootf_gain_table40    : 16;
	UINT32 m_pend_tmootf_gain_table41    : 16;


	//REGISTER Post_proc_reg_57
	UINT32 m_pend_tmootf_gain_table42    : 16;
	UINT32 m_pend_tmootf_gain_table43    : 16;


	//REGISTER Post_proc_reg_58
	UINT32 m_pend_tmootf_gain_table44    : 16;
	UINT32 m_pend_tmootf_gain_table45    : 16;


	//REGISTER Post_proc_reg_59
	UINT32 m_pend_tmootf_gain_table46    : 16;
	UINT32 m_pend_tmootf_gain_table47    : 16;


	//REGISTER Post_proc_reg_60
	UINT32 m_pend_tmootf_gain_table48    : 16;
	UINT32 m_pend_tmootf_gain_table49    : 16;


	//REGISTER Post_proc_reg_61
	UINT32 m_pend_tmootf_gain_table50    : 16;
	UINT32 m_pend_tmootf_gain_table51    : 16;


	//REGISTER Post_proc_reg_62
	UINT32 m_pend_tmootf_gain_table52    : 16;
	UINT32 m_pend_tmootf_gain_table53    : 16;


	//REGISTER Post_proc_reg_63
	UINT32 m_pend_tmootf_gain_table54    : 16;
	UINT32 m_pend_tmootf_gain_table55    : 16;


	//REGISTER post_proc_reg_64
	UINT32 m_pend_tmootf_gain_table56    : 16;
	UINT32 m_pend_tmootf_gain_table57    : 16;


	//REGISTER Post_proc_reg_65
	UINT32 m_pend_tmootf_gain_table58    : 16;
	UINT32 m_pend_tmootf_gain_table59    : 16;


	//REGISTER Post_proc_reg_66
	UINT32 m_pend_tmootf_gain_table60    : 16;
	UINT32 m_pend_tmootf_gain_table61    : 16;


	//REGISTER Post_proc_reg_67
	UINT32 m_pend_tmootf_gain_table62    : 16;
	UINT32 m_pend_tmootf_gain_table63    : 16;


	//REGISTER Post_proc_reg_68
	UINT32 m_pend_tmootf_gain_table64    : 16;
	UINT32 m_nend_tmootf_shift_bits      : 5 ;
	UINT32 m_nend_tmootf_rgb_mode        : 2 ;
	UINT32                               : 9 ;


	//REGISTER Post_proc_reg_69
	UINT32 m_pmatrix_table0              : 14;
	UINT32                               : 2 ;
	UINT32 m_pmatrix_table1              : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_70
	UINT32 m_pmatrix_table2              : 14;
	UINT32                               : 2 ;
	UINT32 m_pmatrix_table3              : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_71
	UINT32 m_pmatrix_table4              : 14;
	UINT32                               : 2 ;
	UINT32 m_pmatrix_table5              : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_72
	UINT32 m_pmatrix_table6              : 14;
	UINT32                               : 2 ;
	UINT32 m_pmatrix_table7              : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_73
	UINT32 m_pmatrix_table8              : 14;
	UINT32                               : 18;


	//REGISTER Post_proc_reg_74
	UINT32 m_pmatrix_offset0             : 25;
	UINT32                               : 7 ;


	//REGISTER Post_proc_reg_75
	UINT32 m_pmatrix_offset1             : 25;
	UINT32                               : 7 ;


	//REGISTER Post_proc_reg_76
	UINT32 m_pmatrix_offset2             : 25;
	UINT32                               : 7 ;


	//REGISTER Post_proc_reg_77
	UINT32 m_ngain_to_full               : 16;
	UINT32                               : 16;


	//REGISTER Post_proc_reg_78
	UINT32 m_pendmatrix_table0           : 14;
	UINT32                               : 2 ;
	UINT32 m_pendmatrix_table1           : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_79
	UINT32 m_pendmatrix_table2           : 14;
	UINT32                               : 2 ;
	UINT32 m_pendmatrix_table3           : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_80
	UINT32 m_pendmatrix_table4           : 14;
	UINT32                               : 2 ;
	UINT32 m_pendmatrix_table5           : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_81
	UINT32 m_pendmatrix_table6           : 14;
	UINT32                               : 2 ;
	UINT32 m_pendmatrix_table7           : 14;
	UINT32                               : 2 ;


	//REGISTER Post_proc_reg_82
	UINT32 m_pendmatrix_table8           : 14;
	UINT32                               : 18;


	//REGISTER Post_proc_reg_83
	UINT32 m_pendmatrix_offset0          : 13;
	UINT32                               : 19;


	//REGISTER Post_proc_reg_84
	UINT32 m_pendmatrix_offset1          : 13;
	UINT32                               : 19;


	//REGISTER Post_proc_reg_85
	UINT32 m_pendmatrix_offset2          : 13;
	UINT32                               : 19;


	};

	INT32 value32[86];

}OUTCTRL_PROC_X_REG;


#endif

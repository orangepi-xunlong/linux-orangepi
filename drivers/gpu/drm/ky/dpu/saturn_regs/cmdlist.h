// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef CMDLIST_REG_H
#define CMDLIST_REG_H

typedef union
{
	struct
	{
	//REGISTER cmdlist_reg_0
	struct
	{
	UINT32                                 : 4 ;
	UINT32 cmdlist_ch_start_addrl          : 28;
	}cmdlist_reg_0[14];

	//REGISTER cmdlist_reg_14
	struct
	{
	UINT32 cmdlist_ch_start_addrh          : 2 ;
	UINT32                                 : 6 ;
	UINT32 cmdlist_ch_y                    : 16;
	UINT32                                 : 8 ;
	}cmdlist_reg_14[14];

	//REGISTER cmdlist_reg_28
	UINT32 cmdlist_burst_len               : 5 ;
	UINT32 axi_port_sel                    : 2 ;
	UINT32 onl_arb_ratio                   : 3 ;
	UINT32                                 : 22;


	//REGISTER cmdlist_reg_29
	UINT32 cmdlist_clr                     : 1 ;
	UINT32                                 : 31;


	//REGISTER cmdlist_reg_30
	UINT32                                 : 16;
	UINT32 cmdlist_clr_timeout_th          : 16;


	//REGISTER cmdlist_reg_31
	UINT32                                 : 4 ;
	UINT32 cmdlist_ch_cfg_timeout_int_msk  : 14;
	UINT32 cmdlist_ch_clr_timeout_int_msk  : 14;


	//REGISTER cmdlist_reg_32
	UINT32                                 : 32;


	//REGISTER cmdlist_reg_33
	UINT32                                 : 4 ;
	UINT32 cmdlist_ch_cfg_timeout_ints     : 14;
	UINT32 cmdlist_ch_clr_timeout_ints     : 14;


	//REGISTER cmdlist_reg_34
	UINT32                                 : 32;


	//REGISTER cmdlist_reg_35
	UINT32                                 : 4 ;
	UINT32 cmdlist_ch_cfg_timeout_int_raw  : 14;
	UINT32 cmdlist_ch_clr_timeout_int_raw  : 14;


	//REGISTER cmdlist_reg_36
	struct
	{
	UINT32 cmdlist_ch_dbg0                 : 11;
	UINT32                                 : 5 ;
	UINT32 cmdlist_ch_dbg1                 : 11;
	UINT32                                 : 5 ;
	}cmdlist_reg_36[7];

	//REGISTER cmdlist_reg_43
	UINT32 cmdlist_dbg                     : 6 ;
	UINT32                                 : 26;


	};

	INT32 value32[44];

}CMDLIST_REG;

#define CMDLIST_ADDRL_ALIGN_BITS		(4) //From cmdlist_reg_0[] in CMDLIST_REG
#define CMDLIST_ADDRL_ALIGN_MASK		((u32)(~(BIT(CMDLIST_ADDRL_ALIGN_BITS) - 1)))
#endif

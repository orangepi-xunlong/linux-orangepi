// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef DPU_CRG_REG_H
#define DPU_CRG_REG_H

typedef union
{
	struct
	{
	//REGISTER crg_reg_0
	UINT32 mclk_wb_auto_en        : 1 ;
	UINT32 aclk_wb_auto_en        : 1 ;
	UINT32 aclk_rdma_auto_en      : 1 ;
	UINT32 mclk_layer_auto_en     : 1 ;
	UINT32 mclk_scl_auto_en       : 1 ;
	UINT32 mclk_cmps_auto_en      : 1 ;
	UINT32 mclk_outctl_auto_en    : 1 ;
	UINT32 dscclk_outctl_auto_en  : 1 ;
	UINT32 pixclk_outctl_auto_en  : 1 ;
	UINT32 aclk_outctl_auto_en    : 1 ;
	UINT32 aclk_cmdlist_auto_en   : 1 ;
	UINT32 pclk_cmdlist_auto_en   : 1 ;
	UINT32 aclk_dma_top_sw_en     : 1 ;
	UINT32                        : 19;


	//REGISTER crg_reg_1
	UINT32 crg_dma_auto_en        : 5 ;
	UINT32                        : 11;
	UINT32 crg_wb_auto_en         : 6 ;
	UINT32                        : 10;


	//REGISTER crg_reg_2
	UINT32 crg_scl_auto_en        : 2 ;
	UINT32                        : 6 ;
	UINT32 crg_pre_auto_en        : 17;
	UINT32                        : 7 ;


	//REGISTER crg_reg_3
	UINT32 crg_outctl_auto_en     : 18;
	UINT32                        : 14;


	//REGISTER crg_reg_4
	UINT32 crg_ctl_auto_en        : 2 ;
	UINT32                        : 14;
	UINT32 crg_cmdlist_auto_en    : 2 ;
	UINT32                        : 14;


	};

	INT32 value32[5];

}DPU_CRG_REG;

#endif


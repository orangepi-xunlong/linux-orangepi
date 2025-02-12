// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef DPU_TOP_REG_H
#define DPU_TOP_REG_H

typedef union
{
	struct
	{
	//REGISTER dpu_top_reg_0
	UINT32 Minor_number                 : 8 ;
	UINT32 Major_number                 : 8 ;
	UINT32 Product_ID                   : 16;


	//REGISTER dpu_top_reg_1
	struct
	{
	UINT32                              : 32;
	}dpu_top_reg_1[213];

	//REGISTER dpu_top_reg_214
	UINT32 dma0_to_layer0_valid         : 1 ;
	UINT32 layer0_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer1_valid         : 1 ;
	UINT32 layer1_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer2_valid         : 1 ;
	UINT32 layer2_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer3_valid         : 1 ;
	UINT32 layer3_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer4_valid         : 1 ;
	UINT32 layer4_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer5_valid         : 1 ;
	UINT32 layer5_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer6_valid         : 1 ;
	UINT32 layer6_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer7_valid         : 1 ;
	UINT32 layer7_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer8_valid         : 1 ;
	UINT32 layer8_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer9_valid         : 1 ;
	UINT32 layer9_to_dma0_ready         : 1 ;
	UINT32 dma0_to_layer10_valid        : 1 ;
	UINT32 layer10_to_dma0_ready        : 1 ;
	UINT32 dma0_to_layer11_valid        : 1 ;
	UINT32 layer11_to_dma0_ready        : 1 ;
	UINT32                              : 8 ;


	//REGISTER dpu_top_reg_215
	UINT32 dma1_to_layer0_valid         : 1 ;
	UINT32 layer0_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer1_valid         : 1 ;
	UINT32 layer1_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer2_valid         : 1 ;
	UINT32 layer2_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer3_valid         : 1 ;
	UINT32 layer3_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer4_valid         : 1 ;
	UINT32 layer4_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer5_valid         : 1 ;
	UINT32 layer5_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer6_valid         : 1 ;
	UINT32 layer6_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer7_valid         : 1 ;
	UINT32 layer7_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer8_valid         : 1 ;
	UINT32 layer8_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer9_valid         : 1 ;
	UINT32 layer9_to_dma1_ready         : 1 ;
	UINT32 dma1_to_layer10_valid        : 1 ;
	UINT32 layer10_to_dma1_ready        : 1 ;
	UINT32 dma1_to_layer11_valid        : 1 ;
	UINT32 layer11_to_dma1_ready        : 1 ;
	UINT32                              : 8 ;


	//REGISTER dpu_top_reg_216
	UINT32 core0_onl_pre_to_post_valid  : 1 ;
	UINT32 core0_onl_post_to_pre_ready  : 1 ;
	UINT32 core1_onl_pre_to_post_valid  : 1 ;
	UINT32 core1_onl_post_to_pre_ready  : 1 ;
	UINT32 core2_cmb_pre_to_post_valid  : 1 ;
	UINT32 core2_cmb_post_to_pre_ready  : 1 ;
	UINT32 core3_cmb_pre_to_post_valid  : 1 ;
	UINT32 core3_cmb_post_to_pre_ready  : 1 ;
	UINT32                              : 8 ;
	UINT32 prepipe_wb0_valid            : 1 ;
	UINT32 wb0_prepipe_ready            : 1 ;
	UINT32 prepipe_wb1_valid            : 1 ;
	UINT32 wb1_prepipe_ready            : 1 ;
	UINT32                              : 12;


	//REGISTER dpu_top_reg_217
	UINT32 prepipe_to_scale0_valid      : 1 ;
	UINT32 scale0_to_prepipe_ready      : 1 ;
	UINT32 prepipe_to_scale1_valid      : 1 ;
	UINT32 scale1_to_prepipe_ready      : 1 ;
	UINT32 scale0_to_prepipe_valid      : 1 ;
	UINT32 prepipe_to_scale0_ready      : 1 ;
	UINT32 scale1_to_prepipe_valid      : 1 ;
	UINT32 prepipe_to_scale1_ready      : 1 ;
	UINT32 postpipe_to_scale0_valid     : 1 ;
	UINT32 scale0_to_postpipe_ready     : 1 ;
	UINT32 postpipe_to_scale1_valid     : 1 ;
	UINT32 scale1_to_postpipe_ready     : 1 ;
	UINT32 scale0_to_post_valid         : 1 ;
	UINT32 postpipe_to_scale0_ready     : 1 ;
	UINT32 scale1_to_postpipe_valid     : 1 ;
	UINT32 postpipe_to_scale1_ready     : 1 ;
	UINT32                              : 16;


	};

	INT32 value32[218];

}DPU_TOP_REG;

#endif


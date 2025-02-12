// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef RDMA_PATH_X_REG_H
#define RDMA_PATH_X_REG_H

typedef union
{
	struct
	{
	//REGISTER LAYER_CTRL
	UINT32 layer_mode                  : 2 ;
	UINT32 outstanding_num             : 5 ;
	UINT32 req_conti_num               : 5 ;
	UINT32 layer_cmpsr_id              : 3 ;
	UINT32 axi_port_sel                : 2 ;
	UINT32 rdma_burst_len              : 8 ;
	UINT32 is_two_layers               : 1 ;
	UINT32 is_offline                  : 1 ;
	UINT32 hdr_osd_num                 : 4 ;
	UINT32                             : 1 ;


	//REGISTER COMPSR_Y_OFST
	UINT32 compsr_y_offset0            : 16;
	UINT32                             : 16;


	//REGISTER LEFT_SCL_RATIO_V
	UINT32                             : 32;


	//REGISTER LEFT_INIT_PHASE_V_LOW
	UINT32                             : 32;


	//REGISTER LEFT_INIT_PHASE_V_HIGH
	UINT32                             : 32;


	//REGISTER RIGHT_SCL_RATIO_V
	UINT32                             : 32;


	//REGISTER RIGHT_INIT_PHASE_V_LOW
	UINT32                             : 32;


	//REGISTER RIGHT_INIT_PHASE_V_HIGH
	UINT32                             : 32;


	//REGISTER LEFT_BASE_ADDR0_LOW
	UINT32 base_addr0_low_ly0          : 32;


	//REGISTER LEFT_BASE_ADDR0_HIGH
	UINT32 base_addr0_high_ly0         : 2 ;
	UINT32                             : 30;


	//REGISTER LEFT_BASE_ADDR1_LOW
	UINT32 base_addr1_low_ly0          : 32;


	//REGISTER LEFT_BASE_ADDR1_HIGH
	UINT32 base_addr1_high_ly0         : 2 ;
	UINT32                             : 30;


	//REGISTER LEFT_BASE_ADDR2_LOW
	UINT32 base_addr2_low_ly0          : 32;


	//REGISTER LEFT_BASE_ADDR2_HIGH
	UINT32 base_addr2_high_ly0         : 2 ;
	UINT32                             : 30;


	//REGISTER LEFT_RDMA_STRIDE0
	UINT32 rdma_stride0_layer0         : 16;
	UINT32 rdma_stride1_layer0         : 16;


	//REGISTER LEFT_IMG_SIZE
	UINT32 img_width_ly0               : 16;
	UINT32 img_height_ly0              : 16;


	//REGISTER LEFT_CROP_POS_START
	UINT32 bbox_start_x_ly0            : 16;
	UINT32 bbox_start_y_ly0            : 16;


	//REGISTER LEFT_CROP_POS_END
	UINT32 bbox_end_x_ly0              : 16;
	UINT32 bbox_end_y_ly0              : 16;


	//REGISTER RIGHT_BASE_ADDR0_LOW
	UINT32                             : 32;


	//REGISTER RIGHT_BASE_ADDR0_HIGH
	UINT32                             : 32;


	//REGISTER RIGHT_BASE_ADDR1_LOW
	UINT32                             : 32;


	//REGISTER RIGHT_BASE_ADDR1_HIGH
	UINT32                             : 32;


	//REGISTER RIGHT_BASE_ADDR2_LOW
	UINT32                             : 32;


	//REGISTER RIGHT_BASE_ADDR2_HIGH
	UINT32                             : 32;


	//REGISTER RIGHT_RDMA_STRIDE0
	UINT32                             : 32;


	//REGISTER RIGHT_IMG_SIZE
	UINT32                             : 32;


	//REGISTER RIGHT_CROP_POS_START
	UINT32                             : 32;


	//REGISTER RIGHT_CROP_POS_END
	UINT32                             : 32;


	//REGISTER ROT_MODE
	UINT32 pixel_format                : 6 ;
	UINT32 uv_swap                     : 1 ;
	UINT32 rot_mode_ly0                : 3 ;
	UINT32 rot_mode_ly1                : 3 ;
	UINT32                             : 19;


	//REGISTER AFBC_CFG
	UINT32 fbc_split_mode              : 1 ;
	UINT32 fbc_yuv_transform           : 1 ;
	UINT32 fbc_sb_layout               : 1 ;
	UINT32 fbc_tile_type               : 1 ;
	UINT32                             : 28;


	//REGISTER FBC_MEM_SIZE
	UINT32 fbc_mem_size                : 16;
	UINT32 fbc_mem_base_addr           : 12;
	UINT32 fbc_mem_map                 : 1 ;
	UINT32 dec_line_num_sw             : 1 ;
	UINT32 sw_dec_line_nnum            : 2 ;


	//REGISTER LAYER_NSAID
	UINT32 nsaid                       : 4 ;
	UINT32                             : 28;


	//REGISTER CSC_MATRIX0
	UINT32 csc_matrix00                : 14;
	UINT32 csc_matrix01                : 14;
	UINT32                             : 4 ;


	//REGISTER CSC_MATRIX1
	UINT32 csc_matrix02                : 14;
	UINT32 csc_matrix03                : 14;
	UINT32                             : 4 ;


	//REGISTER CSC_MATRIX2
	UINT32 csc_matrix10                : 14;
	UINT32 csc_matrix11                : 14;
	UINT32                             : 4 ;


	//REGISTER CSC_MATRIX3
	UINT32 csc_matrix12                : 14;
	UINT32 csc_matrix13                : 14;
	UINT32                             : 4 ;


	//REGISTER CSC_MATRIX4
	UINT32 csc_matrix20                : 14;
	UINT32 csc_matrix21                : 14;
	UINT32                             : 4 ;


	//REGISTER CSC_MATRIX5
	UINT32 csc_matrix22                : 14;
	UINT32 csc_matrix23                : 14;
	UINT32                             : 4 ;


	//REGISTER LEFT_ALPHA01
	UINT32 alpha0_ly0                  : 12;
	UINT32                             : 4 ;
	UINT32 alpha1_ly0                  : 12;
	UINT32                             : 4 ;


	//REGISTER LEFT_ALPHA23
	UINT32 alpha2_ly0                  : 12;
	UINT32                             : 4 ;
	UINT32 alpha3_ly0                  : 12;
	UINT32                             : 4 ;


	//REGISTER RIGHT_ALPHA01
	UINT32                             : 32;


	//REGISTER RIGHT_ALPHA23
	UINT32                             : 32;


	//REGISTER DBG_IRQ_RAW
	UINT32 tlc_miss_irq_raw            : 1 ;
	UINT32 tbu_size_err_irq_raw        : 1 ;
	UINT32 mmu_rdma_timeout_raw        : 1 ;
	UINT32 mmu_rdma_rsp_decerr_raw     : 1 ;
	UINT32 mmu_rdma_rsp_slverr_raw     : 1 ;
	UINT32 mmu_rdma_rsp_exok_raw       : 1 ;
	UINT32 rdma_timeout_irq_raw        : 1 ;
	UINT32 rdma_rsp_decerr_raw         : 1 ;
	UINT32 rdma_rsp_slverr_raw         : 1 ;
	UINT32 rdma_rsp_exok_raw           : 1 ;
	UINT32 dma_mem_err_hw_raw          : 1 ;
	UINT32 rdma_eof_ly0_raw            : 1 ;
	UINT32 rdma_eof_ly1_raw            : 1 ;
	UINT32 rdma_output_eof_ly0_raw     : 1 ;
	UINT32 rdma_output_eof_ly1_raw     : 1 ;
	UINT32 fbcdec_eof_ly0_raw          : 1 ;
	UINT32 fbcdec_eof_ly1_raw          : 1 ;
	UINT32 fbcdec_err_ly0_raw          : 1 ;
	UINT32 fbcdec_err_ly1_raw          : 1 ;
	UINT32 rdma_all_out_eof_raw        : 1 ;
	UINT32 rmda_err_sw_raw             : 1 ;
	UINT32 mem_conflict_raw            : 1 ;
	UINT32 rdma_sof_raw                : 1 ;
	UINT32 rdma_va_mismatch_raw        : 1 ;
	UINT32                             : 8 ;


	//REGISTER DBG_IRQ_MASK
	UINT32 tlc_miss_irq_mask           : 1 ;
	UINT32 tbu_size_err_irq_mask       : 1 ;
	UINT32 mmu_rdma_timeout_mask       : 1 ;
	UINT32 mmu_rdma_rsp_decerr_mask    : 1 ;
	UINT32 mmu_rdma_rsp_slverr_mask    : 1 ;
	UINT32 mmu_rdma_rsp_exok_mask      : 1 ;
	UINT32 rdma_timeout_irq_mask       : 1 ;
	UINT32 rdma_rsp_decerr_mask        : 1 ;
	UINT32 rdma_rsp_slverr_mask        : 1 ;
	UINT32 rdma_rsp_exok_mask          : 1 ;
	UINT32 dma_mem_err_hw_mask         : 1 ;
	UINT32 rdma_eof_ly0_mask           : 1 ;
	UINT32 rdma_eof_ly1_mask           : 1 ;
	UINT32 rdma_output_eof_ly0_mask    : 1 ;
	UINT32 rdma_output_eof_ly1_mask    : 1 ;
	UINT32 fbcdec_eof_ly0_mask         : 1 ;
	UINT32 fbcdec_eof_ly1_mask         : 1 ;
	UINT32 fbcdec_err_ly0_mask         : 1 ;
	UINT32 fbcdec_err_ly1_mask         : 1 ;
	UINT32 rdma_all_out_eof_mask       : 1 ;
	UINT32 rdma_err_sw_mask            : 1 ;
	UINT32 mem_conflict_mask           : 1 ;
	UINT32 rdma_sof_mask               : 1 ;
	UINT32 rdma_va_mismatch_mask       : 1 ;
	UINT32                             : 8 ;


	//REGISTER DBG_IRQ_STATUS
	UINT32 tlc_miss_irq_status         : 1 ;
	UINT32 tbu_size_err_irq_status     : 1 ;
	UINT32 mmu_rdma_timeout_status     : 1 ;
	UINT32 mmu_rdma_rsp_decerr_status  : 1 ;
	UINT32 mmu_rdma_rsp_slverr_status  : 1 ;
	UINT32 mmu_rdma_rsp_exok_status    : 1 ;
	UINT32 rdma_timeout_irq_status     : 1 ;
	UINT32 rdma_rsp_decerr_status      : 1 ;
	UINT32 rdma_rsp_slverr_status      : 1 ;
	UINT32 rdma_rsp_exok_status        : 1 ;
	UINT32 dma_mem_err_status          : 1 ;
	UINT32 rdma_eof_ly0_status         : 1 ;
	UINT32 rdma_eof_ly1_status         : 1 ;
	UINT32 rdma_output_eof_ly0_status  : 1 ;
	UINT32 rdma_output_eof_ly1_status  : 1 ;
	UINT32 fbcdec_eof_ly0_status       : 1 ;
	UINT32 fbcdec_eof_ly1_status       : 1 ;
	UINT32 fbcdec_err_ly0_status       : 1 ;
	UINT32 fbcdec_err_ly1_status       : 1 ;
	UINT32 rdma_all_out_eof_status     : 1 ;
	UINT32 rdma_mem_err_sw             : 1 ;
	UINT32 mem_conflict_status         : 1 ;
	UINT32 rdma_eof_status             : 1 ;
	UINT32 rdma_va_mismatch_status     : 1 ;
	UINT32                             : 8 ;


	//REGISTER PREFETCH_OSD_CTRL
	UINT32 prefetch_osd_num            : 5 ;
	UINT32 prefetch_hdr_osd_num        : 4 ;
	UINT32 osd_sub_step                : 2 ;
	UINT32 osd_num_min                 : 5 ;
	UINT32 hdr_osd_num_min             : 4 ;
	UINT32 pp_buf_switch_num           : 3 ;
	UINT32                             : 9 ;
	} b;

	struct
	{
	//REGISTER LAYER_CTRL
	UINT32 LAYER_CTRL;


	//REGISTER COMPSR_Y_OFST
	UINT32 COMPSR_Y_OFST;


	//REGISTER LEFT_SCL_RATIO_V
	UINT32 LEFT_SCL_RATIO_V;


	//REGISTER LEFT_INIT_PHASE_V_LOW
	UINT32 LEFT_INIT_PHASE_V_LOW;


	//REGISTER LEFT_INIT_PHASE_V_HIGH
	UINT32 LEFT_INIT_PHASE_V_HIGH;


	//REGISTER RIGHT_SCL_RATIO_V
	UINT32 RIGHT_SCL_RATIO_V;


	//REGISTER RIGHT_INIT_PHASE_V_LOW
	UINT32 RIGHT_INIT_PHASE_V_LOW;


	//REGISTER RIGHT_INIT_PHASE_V_HIGH
	UINT32 RIGHT_INIT_PHASE_V_HIGH;


	//REGISTER LEFT_BASE_ADDR0_LOW
	UINT32 LEFT_BASE_ADDR0_LOW;


	//REGISTER LEFT_BASE_ADDR0_HIGH
	UINT32 LEFT_BASE_ADDR0_HIGH;


	//REGISTER LEFT_BASE_ADDR1_LOW
	UINT32 LEFT_BASE_ADDR1_LOW;


	//REGISTER LEFT_BASE_ADDR1_HIGH
	UINT32 LEFT_BASE_ADDR1_HIGH;


	//REGISTER LEFT_BASE_ADDR2_LOW
	UINT32 LEFT_BASE_ADDR2_LOW;


	//REGISTER LEFT_BASE_ADDR2_HIGH
	UINT32 LEFT_BASE_ADDR2_HIGH;


	//REGISTER LEFT_RDMA_STRIDE0
	UINT32 LEFT_RDMA_STRIDE0;


	//REGISTER LEFT_IMG_SIZE
	UINT32 LEFT_IMG_SIZE;


	//REGISTER LEFT_CROP_POS_START
	UINT32 LEFT_CROP_POS_START;


	//REGISTER LEFT_CROP_POS_END
	UINT32 LEFT_CROP_POS_END;


	//REGISTER RIGHT_BASE_ADDR0_LOW
	UINT32 RIGHT_BASE_ADDR0_LOW;


	//REGISTER RIGHT_BASE_ADDR0_HIGH
	UINT32 RIGHT_BASE_ADDR0_HIGH;


	//REGISTER RIGHT_BASE_ADDR1_LOW
	UINT32 RIGHT_BASE_ADDR1_LOW;


	//REGISTER RIGHT_BASE_ADDR1_HIGH
	UINT32 RIGHT_BASE_ADDR1_HIGH;


	//REGISTER RIGHT_BASE_ADDR2_LOW
	UINT32 RIGHT_BASE_ADDR2_LOW;


	//REGISTER RIGHT_BASE_ADDR2_HIGH
	UINT32 RIGHT_BASE_ADDR2_HIGH;


	//REGISTER RIGHT_RDMA_STRIDE0
	UINT32 RIGHT_RDMA_STRIDE0;


	//REGISTER RIGHT_IMG_SIZE
	UINT32 RIGHT_IMG_SIZE;


	//REGISTER RIGHT_CROP_POS_START
	UINT32 RIGHT_CROP_POS_START;


	//REGISTER RIGHT_CROP_POS_END
	UINT32 RIGHT_CROP_POS_END;


	//REGISTER ROT_MODE
	UINT32 ROT_MODE;


	//REGISTER AFBC_CFG
	UINT32 AFBC_CFG;


	//REGISTER FBC_MEM_SIZE
	UINT32 FBC_MEM_SIZE;


	//REGISTER LAYER_NSAID
	UINT32 LAYER_NSAID;


	//REGISTER CSC_MATRIX0
	UINT32 CSC_MATRIX0;


	//REGISTER CSC_MATRIX1
	UINT32 CSC_MATRIX1;


	//REGISTER CSC_MATRIX2
	UINT32 CSC_MATRIX2;


	//REGISTER CSC_MATRIX3
	UINT32 CSC_MATRIX3;


	//REGISTER CSC_MATRIX4
	UINT32 CSC_MATRIX4;


	//REGISTER CSC_MATRIX5
	UINT32 CSC_MATRIX5;


	//REGISTER LEFT_ALPHA01
	UINT32 LEFT_ALPHA01;


	//REGISTER LEFT_ALPHA23
	UINT32 LEFT_ALPHA23;


	//REGISTER RIGHT_ALPHA01
	UINT32 RIGHT_ALPHA01;


	//REGISTER RIGHT_ALPHA23
	UINT32 RIGHT_ALPHA23;


	//REGISTER DBG_IRQ_RAW
	UINT32 DBG_IRQ_RAW;


	//REGISTER DBG_IRQ_MASK
	UINT32 DBG_IRQ_MASK;


	//REGISTER DBG_IRQ_STATUS
	UINT32 DBG_IRQ_STATUS;


	//PREFETCH_OSD_CTRL
	UINT32 PREFETCH_OSD_CTRL;
	} v;


//	INT32 value32[46];

}RDMA_PATH_X_REG;

#endif


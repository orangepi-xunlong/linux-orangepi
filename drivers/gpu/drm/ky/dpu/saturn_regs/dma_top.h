// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef DMA_TOP_REG_H
#define DMA_TOP_REG_H

typedef union
{
	struct
	{
	//REGISTER DBG_EN
	UINT32 dbg_en                            : 1 ;
	UINT32                                   : 31;


	//REGISTER DMA_ARB_OPTION
	UINT32 img_rr_ratio                      : 8 ;
	UINT32                                   : 4 ;
	UINT32 round_robin_mode                  : 1 ;
	UINT32                                   : 3 ;
	UINT32 pixel_num_th                      : 6 ;
	UINT32                                   : 10;


	//REGISTER DMA_TIMEOUT_NUM
	UINT32 rdma_timeout_limit                : 16;
	UINT32 wdma_timeout_limit                : 16;


	//REGISTER dmac0_ctrl
	UINT32 dmac0_rstn_pwr                    : 1 ;
	UINT32 dmac0_rst_req                     : 1 ;
	UINT32                                   : 1 ;
	UINT32 dmac0_burst_length                : 3 ;
	UINT32 dmac0_arcache                     : 4 ;
	UINT32 dmac0_awcache                     : 4 ;
	UINT32 dmac0_arregion                    : 4 ;
	UINT32 dmac0_awregion                    : 4 ;
	UINT32                                   : 10;


	//REGISTER dmac1_ctrl
	UINT32 dmac1_rstn_pwr                    : 1 ;
	UINT32 dmac1_rst_req                     : 1 ;
	UINT32                                   : 1 ;
	UINT32 dmac1_burst_length                : 3 ;
	UINT32 dmac1_arcache                     : 4 ;
	UINT32 dmac1_awcache                     : 4 ;
	UINT32 dmac1_arregion                    : 4 ;
	UINT32 dmac1_awregion                    : 4 ;
	UINT32                                   : 10;


	//REGISTER dmac2_ctrl
	UINT32 dmac2_rstn_pwr                    : 1 ;
	UINT32 dmac2_rst_req                     : 1 ;
	UINT32                                   : 1 ;
	UINT32 dmac2_burst_length                : 3 ;
	UINT32 dmac2_arcache                     : 4 ;
	UINT32 dmac2_awcache                     : 4 ;
	UINT32 dmac2_arregion                    : 4 ;
	UINT32 dmac2_awregion                    : 4 ;
	UINT32                                   : 10;


	//REGISTER dmac3_ctrl
	UINT32 dmac3_rstn_pwr                    : 1 ;
	UINT32 dmac3_rst_req                     : 1 ;
	UINT32                                   : 1 ;
	UINT32 dmac3_burst_length                : 3 ;
	UINT32 dmac3_arcache                     : 4 ;
	UINT32 dmac3_awcache                     : 4 ;
	UINT32 dmac3_arregion                    : 4 ;
	UINT32 dmac3_awregion                    : 4 ;
	UINT32                                   : 10;


	//REGISTER DMA_QOS
	UINT32 online_rqos                       : 4 ;
	UINT32 offline_rqos                      : 4 ;
	UINT32 online_wqos                       : 4 ;
	UINT32 offline_wqos                      : 4 ;
	UINT32 cmdlist_rqos                      : 4 ;
	UINT32                                   : 12;


	//REGISTER DMAC0_OUTS_NUM
	UINT32 dmac0_rd_outs_num                 : 8 ;
	UINT32 dmac0_wr_outs_num                 : 8 ;
	UINT32                                   : 16;


	//REGISTER DMAC1_OUTS_NUM
	UINT32 dmac1_rd_outs_num                 : 8 ;
	UINT32 dmac1_wr_outs_num                 : 8 ;
	UINT32                                   : 16;


	//REGISTER DMAC2_OUTS_NUM
	UINT32 dmac2_rd_outs_num                 : 8 ;
	UINT32 dmac2_wr_outs_num                 : 8 ;
	UINT32                                   : 16;


	//REGISTER DMAC3_OUTS_NUM
	UINT32 dmac3_rd_outs_num                 : 8 ;
	UINT32 dmac3_wr_outs_num                 : 8 ;
	UINT32                                   : 16;


	//REGISTER CMDLIST0_IRQ_RAW
	UINT32                                   : 6 ;
	UINT32 cmdlist0_rdma_timeout_irq_raw     : 1 ;
	UINT32 cmdlist0_rdma_rsp_decerr_raw      : 1 ;
	UINT32 cmdlist0_rdma_rsp_slverr_raw      : 1 ;
	UINT32 cmdlist0_rdma_rsp_exok_raw        : 1 ;
	UINT32 cmdlist0_va_mismatch_raw          : 1 ;
	UINT32                                   : 21;


	//REGISTER WB0_IRQ_RAW
	UINT32 wb0_tlb_miss_irq_raw              : 1 ;
	UINT32 wb0_tbu_size_err_irq_raw          : 1 ;
	UINT32 wb0_mmu_rdma_timeout_raw          : 1 ;
	UINT32 wb0_mmu_rdma_rsp_decerr_raw       : 1 ;
	UINT32 wb0_mmu_rdma_rsp_slverr_raw       : 1 ;
	UINT32 wb0_mmu_rdma_rsp_exok_raw         : 1 ;
	UINT32 wb0_wdma_timeout_irq_raw          : 1 ;
	UINT32 wb0_wdma_rsp_decerr_raw           : 1 ;
	UINT32 wb0_wdma_rsp_slverr_raw           : 1 ;
	UINT32 wb0_wdma_rsp_exok_raw             : 1 ;
	UINT32 wb0_va_mismatch_raw               : 1 ;
	UINT32                                   : 21;


	//REGISTER WB1_IRQ_RAW
	UINT32 wb1_tlb_miss_irq_raw              : 1 ;
	UINT32 wb1_tbu_size_err_irq_raw          : 1 ;
	UINT32 wb1_mmu_rdma_timeout_raw          : 1 ;
	UINT32 wb1_mmu_rdma_rsp_decerr_raw       : 1 ;
	UINT32 wb1_mmu_rdma_rsp_slverr_raw       : 1 ;
	UINT32 wb1_mmu_rdma_rsp_exok_raw         : 1 ;
	UINT32 wb1_wdma_timeout_irq_raw          : 1 ;
	UINT32 wb1_wdma_rsp_decerr_raw           : 1 ;
	UINT32 wb1_wdma_rsp_slverr_raw           : 1 ;
	UINT32 wb1_wdma_rsp_exok_raw             : 1 ;
	UINT32 wb1_va_mismatch_raw               : 1 ;
	UINT32                                   : 21;


	//REGISTER CMDLIST0_IRQ_MASK
	UINT32                                   : 6 ;
	UINT32 cmdlist0_rdma_timeout_irq_mask    : 1 ;
	UINT32 cmdlist0_rdma_rsp_decerr_mask     : 1 ;
	UINT32 cmdlist0_rdma_rsp_slverr_mask     : 1 ;
	UINT32 cmdlist0_rdma_rsp_exok_mask       : 1 ;
	UINT32 cmdlist0_va_mismatch_mask         : 1 ;
	UINT32                                   : 21;


	//REGISTER WB0_IRQ_MASK
	UINT32 wb0_tlb_miss_irq_mask             : 1 ;
	UINT32 wb0_tbu_size_err_irq_mask         : 1 ;
	UINT32 wb0_mmu_rdma_timeout_mask         : 1 ;
	UINT32 wb0_mmu_rdma_rsp_decerr_mask      : 1 ;
	UINT32 wb0_mmu_rdma_rsp_slverr_mask      : 1 ;
	UINT32 wb0_mmu_rdma_rsp_exok_mask        : 1 ;
	UINT32 wb0_wdma_timeout_irq_mask         : 1 ;
	UINT32 wb0_wdma_rsp_decerr_mask          : 1 ;
	UINT32 wb0_wdma_rsp_slverr_mask          : 1 ;
	UINT32 wb0_wdma_rsp_exok_mask            : 1 ;
	UINT32 wb0_va_mismatch_mask              : 1 ;
	UINT32                                   : 21;


	//REGISTER WB1_IRQ_MASK
	UINT32 wb1_tlb_miss_irq_mask             : 1 ;
	UINT32 wb1_tbu_size_err_irq_mask         : 1 ;
	UINT32 wb1_mmu_rdma_timeout_mask         : 1 ;
	UINT32 wb1_mmu_rdma_rsp_decerr_mask      : 1 ;
	UINT32 wb1_mmu_rdma_rsp_slverr_mask      : 1 ;
	UINT32 wb1_mmu_rdma_rsp_exok_mask        : 1 ;
	UINT32 wb1_wdma_timeout_irq_mask         : 1 ;
	UINT32 wb1_wdma_rsp_decerr_mask          : 1 ;
	UINT32 wb1_wdma_rsp_slverr_mask          : 1 ;
	UINT32 wb1_wdma_rsp_exok_mask            : 1 ;
	UINT32 wb1_va_mismatch_mask              : 1 ;
	UINT32                                   : 21;


	//REGISTER CMDLIST0_IRQ_STATUS
	UINT32                                   : 6 ;
	UINT32 cmdlist0_rdma_timeout_irq_status  : 1 ;
	UINT32 cmdlist0_rdma_rsp_decerr_status   : 1 ;
	UINT32 cmdlist0_rdma_rsp_slverr_status   : 1 ;
	UINT32 cmdlist0_rdma_rsp_exok_status     : 1 ;
	UINT32 cmdlist0_va_mistach_status        : 1 ;
	UINT32                                   : 21;


	//REGISTER WB0_IRQ_STATUS
	UINT32 wb0_tlb_miss_irq_status           : 1 ;
	UINT32 wb0_tbu_size_err_irq_status       : 1 ;
	UINT32 wb0_mmu_rdma_timeout_status       : 1 ;
	UINT32 wb0_mmu_rdma_rsp_decerr_status    : 1 ;
	UINT32 wb0_mmu_rdma_rsp_slverr_status    : 1 ;
	UINT32 wb0_mmu_rdma_rsp_exok_status      : 1 ;
	UINT32 wb0_wdma_timeout_irq_status       : 1 ;
	UINT32 wb0_wdma_rsp_decerr_status        : 1 ;
	UINT32 wb0_wdma_rsp_slverr_status        : 1 ;
	UINT32 wb0_wdma_rsp_exok_status          : 1 ;
	UINT32 wb0_va_mismatch_status            : 1 ;
	UINT32                                   : 21;


	//REGISTER WB1_IRQ_STATUS
	UINT32 wb1_tlb_miss_irq_status           : 1 ;
	UINT32 wb1_tbu_size_err_irq_status       : 1 ;
	UINT32 wb1_mmu_rdma_timeout_status       : 1 ;
	UINT32 wb1_mmu_rdma_rsp_decerr_status    : 1 ;
	UINT32 wb1_mmu_rdma_rsp_slverr_status    : 1 ;
	UINT32 wb1_mmu_rdma_rsp_exok_status      : 1 ;
	UINT32 wb1_wdma_timeout_irq_status       : 1 ;
	UINT32 wb1_wdma_rsp_decerr_status        : 1 ;
	UINT32 wb1_wdma_rsp_slverr_status        : 1 ;
	UINT32 wb1_wdma_rsp_exok_staus           : 1 ;
	UINT32 wb1_va_mismatch_status            : 1 ;
	UINT32                                   : 21;


	//REGISTER ARB_DEBUG_INFO0
	UINT32 arb_debug_info_axi0               : 32;


	//REGISTER ARB_DEBUG_INFO1
	UINT32 arb_debug_info_axi1               : 32;


	//REGISTER ARB_DEBUG_INFO2
	UINT32 arb_debug_info_axi2               : 32;


	//REGISTER ARB_DEBUG_INFO3
	UINT32 arb_debug_info_axi3               : 32;


	};

	INT32 value32[25];

}DMA_TOP_REG;

#endif


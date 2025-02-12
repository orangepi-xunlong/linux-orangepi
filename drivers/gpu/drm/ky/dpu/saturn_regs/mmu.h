// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef MMU_REG_H
#define MMU_REG_H

typedef union
{
	struct
	{
	struct
	{
	//REGISTER TBU_Timelimit
	UINT32 rdma_timelimit         : 16;
	UINT32 mmu_cg_en              : 1 ;
	UINT32                        : 15;


	//REGISTER TBU_AXI_PORT_SEL
	UINT32 axi_port_sel0          : 2 ;
	UINT32 axi_port_sel1          : 2 ;
	UINT32 axi_port_sel2          : 2 ;
	UINT32 axi_port_sel3          : 2 ;
	UINT32                        : 24;


	//REGISTER TLB_Miss_stat
	UINT32 tlb_miss_num_clr       : 1 ;
	UINT32 tlb_miss_num_sel       : 7 ;
	UINT32                        : 8 ;
	UINT32 tlb_miss_num           : 16;


	//REGISTER MMU_Dmac0_Reg
	UINT32                        : 4 ;
	UINT32 cfg_dmac0_arcache      : 4 ;
	UINT32 cfg_dmac0_arregion     : 4 ;
	UINT32 cfg_dmac0_aruser       : 4 ;
	UINT32 cfg_dmac0_rd_outs_num  : 8 ;
	UINT32 cfg_dmac0_axi_burst    : 3 ;
	UINT32                        : 5 ;


	//REGISTER MMU_Dmac1_Reg
	UINT32                        : 4 ;
	UINT32 cfg_dmac1_arcache      : 4 ;
	UINT32 cfg_dmac1_arregion     : 4 ;
	UINT32 cfg_dmac1_aruser       : 4 ;
	UINT32 cfg_dmac1_rd_outs_num  : 8 ;
	UINT32 cfg_dmac1_axi_burst    : 3 ;
	UINT32                        : 5 ;


	//REGISTER MMU_Dmac2_Reg
	UINT32                        : 4 ;
	UINT32 cfg_dmac2_arcache      : 4 ;
	UINT32 cfg_dmac2_arregion     : 4 ;
	UINT32 cfg_dmac2_aruser       : 4 ;
	UINT32 cfg_dmac2_rd_outs_num  : 8 ;
	UINT32 cfg_dmac2_axi_burst    : 3 ;
	UINT32                        : 5 ;


	//REGISTER MMU_Dmac3_Reg
	UINT32                        : 4 ;
	UINT32 cfg_dmac3_arcache      : 4 ;
	UINT32 cfg_dmac3_arregion     : 4 ;
	UINT32 cfg_dmac3_aruser       : 4 ;
	UINT32 cfg_dmac3_rd_outs_num  : 8 ;
	UINT32 cfg_dmac3_axi_burst    : 3 ;
	UINT32                        : 5 ;


	//REGISTER MMU_axi0_ar_debug_Reg
	UINT32 axi0_ar_debug          : 32;


	//REGISTER MMU_axi0_aw_debug_Reg
	UINT32 axi0_aw_debug          : 32;


	//REGISTER MMU_axi1_ar_debug_Reg
	UINT32 axi1_ar_debug          : 32;


	//REGISTER MMU_axi1_aw_debug_Reg
	UINT32 axi1_aw_debug          : 32;


	//REGISTER MMU_axi2_ar_debug_Reg
	UINT32 axi2_ar_debug          : 32;


	//REGISTER MMU_axi2_aw_debug_Reg
	UINT32 axi2_aw_debug          : 32;


	//REGISTER MMU_axi3_ar_debug_Reg
	UINT32 axi3_ar_debug          : 32;


	//REGISTER MMU_axi3_aw_debug_Reg
	UINT32 axi3_aw_debug          : 32;


	//REGISTER TLB_CMD_NUM
	UINT32 tlb_cmd_num_sel        : 7 ;
	UINT32                        : 9 ;
	UINT32 tlb_cmd_num            : 16;


	//REGISTER TLB_CMD_NUM_TOTAL
	UINT32 tlb_cmd_num_total      : 32;


	//REGISTER TLB_WAIT_CYCLE
	UINT32 tlb_wait_cycle_sel     : 7 ;
	UINT32                        : 9 ;
	UINT32 tlb_wait_cycle         : 16;


	//REGISTER TLB_WAIT_CYCLE_TOTAL
	UINT32 tlb_wait_cycle_total   : 32;


	//REGISTER TLB_MISS_NUM_TOTAL
	UINT32 tlb_miss_num_total     : 32;


	struct {
		UINT32                    : 32;
	} reserve[44];

	struct
	{
	//REGISTER TBU_Ctrl
	UINT32 tbu_en               : 1 ;
	UINT32 tbu_fbc_mode         : 1 ;
	UINT32 tbu_plane_num        : 2 ;
	UINT32 tbu_burst_limit_en   : 1 ;
	UINT32 tlb_fetch_active_en  : 1 ;
	UINT32                      : 2 ;
	UINT32 tbu_qos              : 4 ;
	UINT32                      : 20;


	//REGISTER TBU_Base_Addr0_Low
	UINT32 tbu_base_addr0_low   : 32;


	//REGISTER TBU_Base_Addr0_High
	UINT32 tbu_base_addr0_high  : 2 ;
	UINT32                      : 30;


	//REGISTER TBU_Base_Addr1_Low
	UINT32 tbu_base_addr1_low   : 32;


	//REGISTER TBU_Base_Addr1_High
	UINT32 tbu_base_addr1_high  : 2 ;
	UINT32                      : 30;


	//REGISTER TBU_Base_Addr2_Low
	UINT32 tbu_base_addr2_low   : 32;


	//REGISTER TBU_Base_Addr2_High
	UINT32 tbu_base_addr2_high  : 2 ;
	UINT32                      : 30;


	//REGISTER TBU_VA0
	UINT32 tbu_va0              : 22;
	UINT32                      : 10;


	//REGISTER TBU_VA1
	UINT32 tbu_va1              : 22;
	UINT32                      : 10;


	//REGISTER TBU_VA2
	UINT32 tbu_va2              : 22;
	UINT32                      : 10;


	//REGISTER TBU_SIZE0
	UINT32 tbu_size0            : 16;
	UINT32                      : 16;


	//REGISTER TBU_SIZE1
	UINT32 tbu_size1            : 16;
	UINT32                      : 16;


	//REGISTER TBU_SIZE2
	UINT32 tbu_size2            : 16;
	UINT32                      : 16;

	struct {
		UINT32                  : 32;
	} reserve[3];

	}TBU[9];

	};

	INT32 value32[208];
	} b;

	struct
	{
	//REGISTER TBU_Timelimit
	UINT32 TBU_Timelimit;


	//REGISTER TBU_AXI_PORT_SEL
	UINT32 TBU_AXI_PORT_SEL;


	//REGISTER TLB_Miss_stat
	UINT32 TLB_Miss_stat;


	//REGISTER MMU_Dmac0_Reg
	UINT32 MMU_Dmac0_Reg;


	//REGISTER MMU_Dmac1_Reg
	UINT32 MMU_Dmac1_Reg;


	//REGISTER MMU_Dmac2_Reg
	UINT32 MMU_Dmac2_Reg;


	//REGISTER MMU_Dmac3_Reg
	UINT32 MMU_Dmac3_Reg;

	//REGISTER MMU_axi0_ar_debug_Reg
	UINT32 MMU_axi0_ar_debug_Reg;


	//REGISTER MMU_axi0_aw_debug_Reg
	UINT32 MMU_axi0_aw_debug_Reg;


	//REGISTER MMU_axi1_ar_debug_Reg
	UINT32 MMU_axi1_ar_debug_Reg;


	//REGISTER MMU_axi1_aw_debug_Reg
	UINT32 MMU_axi1_aw_debug_Reg;


	//REGISTER MMU_axi2_ar_debug_Reg
	UINT32 axi2_ar_debug;


	//REGISTER MMU_axi2_aw_debug_Reg
	UINT32 MMU_axi2_aw_debug_Reg;


	//REGISTER MMU_axi3_ar_debug_Reg
	UINT32 MMU_axi3_ar_debug_Reg;


	//REGISTER MMU_axi3_aw_debug_Reg
	UINT32 axi3_aw_debug;


	//REGISTER TLB_CMD_NUM
	UINT32 TLB_CMD_NUM;


	//REGISTER TLB_CMD_NUM_TOTAL
	UINT32 TLB_CMD_NUM_TOTAL;


	//REGISTER TLB_WAIT_CYCLE
	UINT32 TLB_WAIT_CYCLE;


	//REGISTER TLB_WAIT_CYCLE_TOTAL
	UINT32 TLB_WAIT_CYCLE_TOTAL;


	//REGISTER TLB_MISS_NUM_TOTAL
	UINT32 TLB_MISS_NUM_TOTAL;

	//Reserved
	struct {
		UINT32 RESERVED;
	} reserve[44];

	struct
	{
	//REGISTER TBU_Ctrl
	UINT32	TBU_Ctrl;


	//REGISTER TBU_Base_Addr0_Low
	UINT32 TBU_Base_Addr0_Low;


	//REGISTER TBU_Base_Addr0_High
	UINT32 TBU_Base_Addr0_High;


	//REGISTER TBU_Base_Addr1_Low
	UINT32 TBU_Base_Addr1_Low;


	//REGISTER TBU_Base_Addr1_High
	UINT32 TBU_Base_Addr1_High;


	//REGISTER TBU_Base_Addr2_Low
	UINT32 TBU_Base_Addr2_Low;


	//REGISTER TBU_Base_Addr2_High
	UINT32 TBU_Base_Addr2_High;


	//REGISTER TBU_VA0
	UINT32 TBU_VA0;


	UINT32 TBU_VA1;


	//REGISTER TBU_VA2
	UINT32 TBU_VA2;


	//REGISTER TBU_SIZE0
	UINT32 TBU_SIZE0;


	//REGISTER TBU_SIZE1
	UINT32 TBU_SIZE1;


	//REGISTER TBU_SIZE2
	UINT32 TBU_SIZE2;

	struct {
		UINT32                  : 32;
	} reserve[3];

	}TBU[9];

	} v;

}MMU_REG;

#endif


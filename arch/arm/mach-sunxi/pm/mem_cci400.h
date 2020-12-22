/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2011-2015, gq.yang China
*                                             All Rights Reserved
*
* File    : mem_cci400.h
* By      : gq.yang
* Version : v1.0
* Date    : 2012-11-31 15:23
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_CCI400_H__
#define __MEM_CCI400_H__
#include "pm.h"

//slave interface 0	
typedef struct slave_interface_reg_list{
	__u32 snoop_ctrl_reg			;	//0x9x000, x:[1-5]
	__u32 share_override_reg		;	//0x9x004
	__u32 reserved0[62]			;	//0x9x008-0x9x100,	size = 62
	__u32 rd_channel_qos_val_reg		;	//0x9x100
	__u32 wr_channel_qos_val_reg		;	//0x9x104
	__u32 reserved1				;	//0x9x108
	__u32 qos_ctrl_reg			;	//0x9x10c
	__u32 max_ot_reg			;	//0x9x110
	__u32 reserved2[7]			;	//0x9x114-0x9x130,	size = 7
	__u32 regu_target_reg			;	//0x9x130
	__u32 qos_regu_scale_factor_reg		;	//0x9x134
	__u32 qos_range_reg			;	//0x9x138
	__u32 reserved3[945]			;	//0x9x13c - 0x9(x+1)000, size = 945
} slave_interface_reg_list_t;

//Cycle counter register
typedef struct cycle_cnter_reg_list{
	__u32 reserved0				;	//0x99000
	__u32 cycle_counter_reg			;	//0x99004
	__u32 count_ctrl_reg			;	//0x99008
	__u32 overflow_flag_status_reg		;	//0x9900c
	__u32 reserved1[1020]			;	//0x99010 - 0x9a000, size: 1020
} cycle_cnter_reg_list_t;
	
//perf counter 0
typedef struct perf_cnter_reg_list{
	__u32 event_select_reg			;	//0x9x000, x:[a-d]
	__u32 event_count_reg			;	//0x9x004
	__u32 count_ctrl_reg			;	//0x9x008
	__u32 overflow_flag_status		;	//0x9x00c
	__u32 reserved0[1020]			;	//0x9x010 - 0x9b000, size: 1020
} perf_cnter_reg_list_t;

//total size: 64K bytes size.
typedef struct cci_reg_list{	
	__u32 ctrl_override_reg 			;	//0x90000
	__u32 spec_ctrl_reg 				;	//0x90004
	__u32 secure_acc_reg 				;	//0x90008
	__u32 status_reg				;	//0x9000c; read
	__u32 imprecise_err_reg				;	//0x90010
	__u32 reserved0[59]				;	//0x90014  - 0x9100, size = 59 words.
	__u32 pmcr_reg					;	//0x90100
	__u32 reserved1[947]				;	//0x90104  - 0x90fcc, size = 947 words.
	__u32 component_peripheral_reg[12] 		;	//0x90fd0 - 0x90ffc, size = 12 words.
	slave_interface_reg_list_t s_inf[5]		;	//0x91000 - 0x95ffc;
	__u32 reserved2[3072] 				;	//0x96000 - 0x99000
	cycle_cnter_reg_list_t c_cnter			;	//0x99000 - 0x9a000
	perf_cnter_reg_list_t  p_cnter[4]      		;	//0x9a000 - 0x9e000
	__u32 reserved3[2048]				;	//0x9e000 - 0x9ffff; 2K * 4 = 8K size.    
} cci_reg_list_t;	

#define CCI_REG_BACKUP_LENGTH		(82)
struct cci400_state{
	cci_reg_list_t		*cci_reg;
	__u32			cci_reg_backup[CCI_REG_BACKUP_LENGTH];	
};

__s32 mem_cci400_save(struct cci400_state *cci_state);
__s32 mem_cci400_restore(struct cci400_state *cci_state);


#endif  //__MEM_CCI400_H__


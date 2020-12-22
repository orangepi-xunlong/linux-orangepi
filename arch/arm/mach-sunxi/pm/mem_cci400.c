/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_cci400.c
* By      : gq.yang
* Version : v1.0
* Date    : 2012-11-3 20:13
* Descript: interrupt for platform mem
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "pm_i.h"

#ifdef CONFIG_ARCH_SUN9IW1P1
/*
*********************************************************************************************************
*                                       MEM cci400 SAVE
*
* Description: mem cci400_state save.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_cci400_save(struct cci400_state *cci_state)
{
	int i = 0;
	cci_state->cci_reg 		= 	(cci_reg_list_t *)IO_ADDRESS(SUNXI_CCI400_PBASE);
	cci_state->cci_reg_backup[0 ]	= 	cci_state->cci_reg->ctrl_override_reg            	;
	cci_state->cci_reg_backup[1 ]	=       cci_state->cci_reg->spec_ctrl_reg 		        ;
	cci_state->cci_reg_backup[2 ]	=       cci_state->cci_reg->secure_acc_reg 		        ;
	cci_state->cci_reg_backup[3 ]	=       cci_state->cci_reg->status_reg		                ;
	cci_state->cci_reg_backup[4 ]	=       cci_state->cci_reg->imprecise_err_reg                    ;
	cci_state->cci_reg_backup[5 ]	=	cci_state->cci_reg->pmcr_reg                             ;
	cci_state->cci_reg_backup[6 ]	=	cci_state->cci_reg->component_peripheral_reg[0 ]         ;
	cci_state->cci_reg_backup[7 ]	=	cci_state->cci_reg->component_peripheral_reg[1 ]         ;
	cci_state->cci_reg_backup[8 ]	=	cci_state->cci_reg->component_peripheral_reg[2 ]         ;
	cci_state->cci_reg_backup[9 ]	=	cci_state->cci_reg->component_peripheral_reg[3 ]         ;
	cci_state->cci_reg_backup[10]	=	cci_state->cci_reg->component_peripheral_reg[4 ]         ;
	cci_state->cci_reg_backup[11]	=	cci_state->cci_reg->component_peripheral_reg[5 ]         ;
	cci_state->cci_reg_backup[12]	=	cci_state->cci_reg->component_peripheral_reg[6 ]         ;
	cci_state->cci_reg_backup[13]	=	cci_state->cci_reg->component_peripheral_reg[7 ]         ;
	cci_state->cci_reg_backup[14]	=	cci_state->cci_reg->component_peripheral_reg[8 ]         ;
	cci_state->cci_reg_backup[15]	=	cci_state->cci_reg->component_peripheral_reg[9 ]         ;
	cci_state->cci_reg_backup[16]	=	cci_state->cci_reg->component_peripheral_reg[10]         ;
	cci_state->cci_reg_backup[17]	=	cci_state->cci_reg->component_peripheral_reg[11]         ;	
	cci_state->cci_reg_backup[18]	=	cci_state->cci_reg->s_inf[0].snoop_ctrl_reg		;	
	cci_state->cci_reg_backup[19]	=	cci_state->cci_reg->s_inf[0].share_override_reg		;        
	cci_state->cci_reg_backup[20]	=	cci_state->cci_reg->s_inf[0].rd_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[21]	=	cci_state->cci_reg->s_inf[0].wr_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[22]	=	cci_state->cci_reg->s_inf[0].qos_ctrl_reg		;	
	cci_state->cci_reg_backup[23]	=	cci_state->cci_reg->s_inf[0].max_ot_reg			;        
	cci_state->cci_reg_backup[24]	=	cci_state->cci_reg->s_inf[0].regu_target_reg		;	
	cci_state->cci_reg_backup[25]	=	cci_state->cci_reg->s_inf[0].qos_regu_scale_factor_reg	;        
	cci_state->cci_reg_backup[26]	=	cci_state->cci_reg->s_inf[0].qos_range_reg		;	
	cci_state->cci_reg_backup[27]	=	cci_state->cci_reg->s_inf[1].snoop_ctrl_reg		;	
	cci_state->cci_reg_backup[28]	=	cci_state->cci_reg->s_inf[1].share_override_reg		;        
	cci_state->cci_reg_backup[29]	=	cci_state->cci_reg->s_inf[1].rd_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[30]	=	cci_state->cci_reg->s_inf[1].wr_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[31]	=	cci_state->cci_reg->s_inf[1].qos_ctrl_reg		;	
	cci_state->cci_reg_backup[32]	=	cci_state->cci_reg->s_inf[1].max_ot_reg			;        
	cci_state->cci_reg_backup[33]	=	cci_state->cci_reg->s_inf[1].regu_target_reg		;	
	cci_state->cci_reg_backup[34]	=	cci_state->cci_reg->s_inf[1].qos_regu_scale_factor_reg	;        
	cci_state->cci_reg_backup[35]	=	cci_state->cci_reg->s_inf[1].qos_range_reg		;	
	cci_state->cci_reg_backup[36]	=	cci_state->cci_reg->s_inf[2].snoop_ctrl_reg		;	
	cci_state->cci_reg_backup[37]	=	cci_state->cci_reg->s_inf[2].share_override_reg		;        
	cci_state->cci_reg_backup[38]	=	cci_state->cci_reg->s_inf[2].rd_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[39]	=	cci_state->cci_reg->s_inf[2].wr_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[40]	=	cci_state->cci_reg->s_inf[2].qos_ctrl_reg		;	
	cci_state->cci_reg_backup[41]	=	cci_state->cci_reg->s_inf[2].max_ot_reg			;        
	cci_state->cci_reg_backup[42]	=	cci_state->cci_reg->s_inf[2].regu_target_reg		;	
	cci_state->cci_reg_backup[43]	=	cci_state->cci_reg->s_inf[2].qos_regu_scale_factor_reg	;        
	cci_state->cci_reg_backup[44]	=	cci_state->cci_reg->s_inf[2].qos_range_reg		;	
	cci_state->cci_reg_backup[45]	=	cci_state->cci_reg->s_inf[3].snoop_ctrl_reg		;	
	cci_state->cci_reg_backup[46]	=	cci_state->cci_reg->s_inf[3].share_override_reg		;        
	cci_state->cci_reg_backup[47]	=	cci_state->cci_reg->s_inf[3].rd_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[48]	=	cci_state->cci_reg->s_inf[3].wr_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[49]	=	cci_state->cci_reg->s_inf[3].qos_ctrl_reg		;	
	cci_state->cci_reg_backup[50]	=	cci_state->cci_reg->s_inf[3].max_ot_reg			;        
	cci_state->cci_reg_backup[51]	=	cci_state->cci_reg->s_inf[3].regu_target_reg		;	
	cci_state->cci_reg_backup[52]	=	cci_state->cci_reg->s_inf[3].qos_regu_scale_factor_reg	;        
	cci_state->cci_reg_backup[53]	=	cci_state->cci_reg->s_inf[3].qos_range_reg		;	
	cci_state->cci_reg_backup[54]	=	cci_state->cci_reg->s_inf[4].snoop_ctrl_reg		;	
	cci_state->cci_reg_backup[55]	=	cci_state->cci_reg->s_inf[4].share_override_reg		;        
	cci_state->cci_reg_backup[56]	=	cci_state->cci_reg->s_inf[4].rd_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[57]	=	cci_state->cci_reg->s_inf[4].wr_channel_qos_val_reg	;	
	cci_state->cci_reg_backup[58]	=	cci_state->cci_reg->s_inf[4].qos_ctrl_reg		;	
	cci_state->cci_reg_backup[59]	=	cci_state->cci_reg->s_inf[4].max_ot_reg			;        
	cci_state->cci_reg_backup[60]	=	cci_state->cci_reg->s_inf[4].regu_target_reg		;	
	cci_state->cci_reg_backup[61]	=	cci_state->cci_reg->s_inf[4].qos_regu_scale_factor_reg	;        
	cci_state->cci_reg_backup[62]	=	cci_state->cci_reg->s_inf[4].qos_range_reg		;	
	cci_state->cci_reg_backup[63]	=	cci_state->cci_reg->c_cnter.cycle_counter_reg	        ;
	cci_state->cci_reg_backup[64]	=	cci_state->cci_reg->c_cnter.count_ctrl_reg		;
	cci_state->cci_reg_backup[65]	=	cci_state->cci_reg->c_cnter.overflow_flag_status_reg     ;
	cci_state->cci_reg_backup[66]	=	cci_state->cci_reg->p_cnter[0].event_select_reg	        ;
	cci_state->cci_reg_backup[67]	=	cci_state->cci_reg->p_cnter[0].event_count_reg		;
	cci_state->cci_reg_backup[68]	=	cci_state->cci_reg->p_cnter[0].count_ctrl_reg		;
	cci_state->cci_reg_backup[69]	=	cci_state->cci_reg->p_cnter[0].overflow_flag_status	;
	cci_state->cci_reg_backup[70]	=	cci_state->cci_reg->p_cnter[1].event_select_reg	        ;
	cci_state->cci_reg_backup[71]	=	cci_state->cci_reg->p_cnter[1].event_count_reg		;
	cci_state->cci_reg_backup[72]	=	cci_state->cci_reg->p_cnter[1].count_ctrl_reg		;
	cci_state->cci_reg_backup[73]	=	cci_state->cci_reg->p_cnter[1].overflow_flag_status	;
	cci_state->cci_reg_backup[74]	=	cci_state->cci_reg->p_cnter[2].event_select_reg	        ;
	cci_state->cci_reg_backup[75]	=	cci_state->cci_reg->p_cnter[2].event_count_reg		;
	cci_state->cci_reg_backup[76]	=	cci_state->cci_reg->p_cnter[2].count_ctrl_reg		;
	cci_state->cci_reg_backup[77]	=	cci_state->cci_reg->p_cnter[2].overflow_flag_status	;
	cci_state->cci_reg_backup[78]	=	cci_state->cci_reg->p_cnter[3].event_select_reg	        ;
	cci_state->cci_reg_backup[79]	=	cci_state->cci_reg->p_cnter[3].event_count_reg		;
	cci_state->cci_reg_backup[80]	=	cci_state->cci_reg->p_cnter[3].count_ctrl_reg		;
	cci_state->cci_reg_backup[81]	=	cci_state->cci_reg->p_cnter[3].overflow_flag_status	;
                                 
	if(debug_mask&PM_STANDBY_PRINT_CCI400_REG){
		for(i=0; i<CCI_REG_BACKUP_LENGTH; i++){     
			printk("cci_state->cci_reg_backup[%d] = 0x%x .\n", i, cci_state->cci_reg_backup[i]);
		}	
	}	                         
	
	return 0;
}

/*
*********************************************************************************************************
*                                       MEM cci400 restore
*
* Description: mem cci400 restore.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_cci400_restore(struct cci400_state *cci_state)
{
	cci_state->cci_reg->ctrl_override_reg            	=	cci_state->cci_reg_backup[0 ];
	cci_state->cci_reg->spec_ctrl_reg 		        =	cci_state->cci_reg_backup[1 ];
	cci_state->cci_reg->secure_acc_reg 		        =	cci_state->cci_reg_backup[2 ];
	//cci_state->cci_reg->status_reg		                =	cci_state->cci_reg_backup[3 ];
	//cci_state->cci_reg->imprecise_err_reg                  =	cci_state->cci_reg_backup[4 ];
	cci_state->cci_reg->pmcr_reg                             =	cci_state->cci_reg_backup[5 ];    
	cci_state->cci_reg->component_peripheral_reg[0 ]		=	cci_state->cci_reg_backup[6 ];
	cci_state->cci_reg->component_peripheral_reg[1 ]		=	cci_state->cci_reg_backup[7 ];
	cci_state->cci_reg->component_peripheral_reg[2 ]		=	cci_state->cci_reg_backup[8 ];
	cci_state->cci_reg->component_peripheral_reg[3 ]		=	cci_state->cci_reg_backup[9 ];
	cci_state->cci_reg->component_peripheral_reg[4 ]		=	cci_state->cci_reg_backup[10];
	cci_state->cci_reg->component_peripheral_reg[5 ]		=	cci_state->cci_reg_backup[11];
	cci_state->cci_reg->component_peripheral_reg[6 ]		=	cci_state->cci_reg_backup[12];
	cci_state->cci_reg->component_peripheral_reg[7 ]		=	cci_state->cci_reg_backup[13];
	cci_state->cci_reg->component_peripheral_reg[8 ]		=	cci_state->cci_reg_backup[14];
	cci_state->cci_reg->component_peripheral_reg[9 ]		=	cci_state->cci_reg_backup[15];
	cci_state->cci_reg->component_peripheral_reg[10]		=	cci_state->cci_reg_backup[16];
	cci_state->cci_reg->component_peripheral_reg[11]		=	cci_state->cci_reg_backup[17];
	cci_state->cci_reg->s_inf[0].snoop_ctrl_reg		=	cci_state->cci_reg_backup[18];
	cci_state->cci_reg->s_inf[0].share_override_reg		=	cci_state->cci_reg_backup[19];
	cci_state->cci_reg->s_inf[0].rd_channel_qos_val_reg	=	cci_state->cci_reg_backup[20];
	cci_state->cci_reg->s_inf[0].wr_channel_qos_val_reg	=	cci_state->cci_reg_backup[21];
	cci_state->cci_reg->s_inf[0].qos_ctrl_reg		=	cci_state->cci_reg_backup[22];
	cci_state->cci_reg->s_inf[0].max_ot_reg			=	cci_state->cci_reg_backup[23];
	cci_state->cci_reg->s_inf[0].regu_target_reg		=	cci_state->cci_reg_backup[24];
	cci_state->cci_reg->s_inf[0].qos_regu_scale_factor_reg	=	cci_state->cci_reg_backup[25];
	cci_state->cci_reg->s_inf[0].qos_range_reg		=	cci_state->cci_reg_backup[26];
	cci_state->cci_reg->s_inf[1].snoop_ctrl_reg		=	cci_state->cci_reg_backup[27];
	cci_state->cci_reg->s_inf[1].share_override_reg		=	cci_state->cci_reg_backup[28];
	cci_state->cci_reg->s_inf[1].rd_channel_qos_val_reg	=	cci_state->cci_reg_backup[29];
	cci_state->cci_reg->s_inf[1].wr_channel_qos_val_reg	=	cci_state->cci_reg_backup[30];
	cci_state->cci_reg->s_inf[1].qos_ctrl_reg		=	cci_state->cci_reg_backup[31];
	cci_state->cci_reg->s_inf[1].max_ot_reg			=	cci_state->cci_reg_backup[32];
	cci_state->cci_reg->s_inf[1].regu_target_reg		=	cci_state->cci_reg_backup[33];
	cci_state->cci_reg->s_inf[1].qos_regu_scale_factor_reg	=	cci_state->cci_reg_backup[34];
	cci_state->cci_reg->s_inf[1].qos_range_reg		=	cci_state->cci_reg_backup[35];
	cci_state->cci_reg->s_inf[2].snoop_ctrl_reg		=	cci_state->cci_reg_backup[36];
	cci_state->cci_reg->s_inf[2].share_override_reg		=	cci_state->cci_reg_backup[37];
	cci_state->cci_reg->s_inf[2].rd_channel_qos_val_reg	=	cci_state->cci_reg_backup[38];
	cci_state->cci_reg->s_inf[2].wr_channel_qos_val_reg	=	cci_state->cci_reg_backup[39];
	cci_state->cci_reg->s_inf[2].qos_ctrl_reg		=	cci_state->cci_reg_backup[40];
	cci_state->cci_reg->s_inf[2].max_ot_reg			=	cci_state->cci_reg_backup[41];
	cci_state->cci_reg->s_inf[2].regu_target_reg		=	cci_state->cci_reg_backup[42];
	cci_state->cci_reg->s_inf[2].qos_regu_scale_factor_reg	=	cci_state->cci_reg_backup[43];
	cci_state->cci_reg->s_inf[2].qos_range_reg		=	cci_state->cci_reg_backup[44];
	cci_state->cci_reg->s_inf[3].snoop_ctrl_reg		=	cci_state->cci_reg_backup[45];
	cci_state->cci_reg->s_inf[3].share_override_reg		=	cci_state->cci_reg_backup[46];
	cci_state->cci_reg->s_inf[3].rd_channel_qos_val_reg	=	cci_state->cci_reg_backup[47];
	cci_state->cci_reg->s_inf[3].wr_channel_qos_val_reg	=	cci_state->cci_reg_backup[48];
	cci_state->cci_reg->s_inf[3].qos_ctrl_reg		=	cci_state->cci_reg_backup[49];
	cci_state->cci_reg->s_inf[3].max_ot_reg			=	cci_state->cci_reg_backup[50];
	cci_state->cci_reg->s_inf[3].regu_target_reg		=	cci_state->cci_reg_backup[51];
	cci_state->cci_reg->s_inf[3].qos_regu_scale_factor_reg	=	cci_state->cci_reg_backup[52];
	cci_state->cci_reg->s_inf[3].qos_range_reg		=	cci_state->cci_reg_backup[53];
	cci_state->cci_reg->s_inf[4].snoop_ctrl_reg		=	cci_state->cci_reg_backup[54];
	cci_state->cci_reg->s_inf[4].share_override_reg		=	cci_state->cci_reg_backup[55];
	cci_state->cci_reg->s_inf[4].rd_channel_qos_val_reg	=	cci_state->cci_reg_backup[56];
	cci_state->cci_reg->s_inf[4].wr_channel_qos_val_reg	=	cci_state->cci_reg_backup[57];
	cci_state->cci_reg->s_inf[4].qos_ctrl_reg		=	cci_state->cci_reg_backup[58];
	cci_state->cci_reg->s_inf[4].max_ot_reg			=	cci_state->cci_reg_backup[59];
	cci_state->cci_reg->s_inf[4].regu_target_reg		=	cci_state->cci_reg_backup[60];
	cci_state->cci_reg->s_inf[4].qos_regu_scale_factor_reg	=	cci_state->cci_reg_backup[61];
	cci_state->cci_reg->s_inf[4].qos_range_reg		=	cci_state->cci_reg_backup[62];
	cci_state->cci_reg->c_cnter.cycle_counter_reg	        =	cci_state->cci_reg_backup[63];
	cci_state->cci_reg->c_cnter.count_ctrl_reg		=	cci_state->cci_reg_backup[64];
	//cci_state->cci_reg->c_cnter.overflow_flag_status_reg   =	cci_state->cci_reg_backup[65];
	cci_state->cci_reg->p_cnter[0].event_select_reg	        =	cci_state->cci_reg_backup[66];
	cci_state->cci_reg->p_cnter[0].event_count_reg		=	cci_state->cci_reg_backup[67];
	cci_state->cci_reg->p_cnter[0].count_ctrl_reg		=	cci_state->cci_reg_backup[68];
	//cci_state->cci_reg->p_cnter[0].overflow_flag_status	=	cci_state->cci_reg_backup[69];
	cci_state->cci_reg->p_cnter[1].event_select_reg	        =	cci_state->cci_reg_backup[70];
        cci_state->cci_reg->p_cnter[1].event_count_reg		=	cci_state->cci_reg_backup[71];
        cci_state->cci_reg->p_cnter[1].count_ctrl_reg		=	cci_state->cci_reg_backup[72];
        //cci_state->cci_reg->p_cnter[1].overflow_flag_status	=	cci_state->cci_reg_backup[73];
        cci_state->cci_reg->p_cnter[2].event_select_reg	        =	cci_state->cci_reg_backup[74];
        cci_state->cci_reg->p_cnter[2].event_count_reg		=	cci_state->cci_reg_backup[75];
        cci_state->cci_reg->p_cnter[2].count_ctrl_reg		=	cci_state->cci_reg_backup[76];
        //cci_state->cci_reg->p_cnter[2].overflow_flag_status	=	cci_state->cci_reg_backup[77];
        cci_state->cci_reg->p_cnter[3].event_select_reg	        =	cci_state->cci_reg_backup[78];
        cci_state->cci_reg->p_cnter[3].event_count_reg		=	cci_state->cci_reg_backup[79];
        cci_state->cci_reg->p_cnter[3].count_ctrl_reg		=	cci_state->cci_reg_backup[80];
        //cci_state->cci_reg->p_cnter[3].overflow_flag_status	=	cci_state->cci_reg_backup[81];

	__asm__ __volatile__ ("dsb" : : : "memory");
	while(cci_state->cci_reg->status_reg&0x01)
		;
		                                             
	if(debug_mask&PM_STANDBY_PRINT_CCI400_REG){
		mem_cci400_save(cci_state);
	}
	                                                                                                  
  	return 0;                                                                                 
}       
#endif

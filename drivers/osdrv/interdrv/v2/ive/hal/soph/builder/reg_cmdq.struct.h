// $Module: cmdq $
// $RegisterBank Version: V 1.0.00 $
// $Author: Test 123 $
// $Date: Sun, 05 Dec 2021 04:15:49 PM $
//

#ifndef __REG_CMDQ_STRUCT_H__
#define __REG_CMDQ_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*CMDQ INT event;*/
		uint32_t reg_cmdq_int:1;
		/*CMDQ end interrupt event when CMDQ decode is finish;*/
		uint32_t reg_cmdq_end:1;
		/*CMDQ wait interrupt event when wait for flag;*/
		uint32_t reg_cmdq_wait:1;
		/*isp apb error from ISP slave;*/
		uint32_t reg_isp_pslverr:1;
		/*Task end interrupt event when task is finish;*/
		uint32_t reg_task_end:1;
	};
	uint32_t val;
} cmdq_int_event_c;
typedef union {
	struct {
		/*CMDQ INT event interrupt enable;*/
		uint32_t reg_cmdq_int_en:1;
		/*CMDQ end interupt enable;*/
		uint32_t reg_cmdq_end_en:1;
		/*CMDQ wait interrupt enable when wait for flag;*/
		uint32_t reg_cmdq_wait_en:1;
		/*isp pslverr interrupt enable;*/
		uint32_t reg_isp_pslverr_en:1;
		/*Task end interrupt enable;*/
		uint32_t reg_task_end_en:1;
	};
	uint32_t val;
} cmdq_int_en_c;
typedef union {
	struct {
		/*DMA start address;*/
		uint32_t reg_dma_addr_l;
	};
	uint32_t val;
} cmdq_dma_addr_l_c;
typedef union {
	struct {
		/*DMA start address;*/
		uint32_t reg_dma_addr_h:8;
	};
	uint32_t val;
} cmdq_dma_addr_h_c;
typedef union {
	struct {
		/*DMA transfer count in bytes;*/
		uint32_t reg_dma_cnt:32;
	};
	uint32_t val;
} cmdq_dma_cnt_c;
typedef union {
	struct {
		/*DMA reserved bit;*/
		uint32_t reg_dma_rsv:1;
		/*Enable ADMA mode;*/
		uint32_t reg_adma_en:1;
		/*Enable Task Management mode;*/
		uint32_t reg_task_en:1;
	};
	uint32_t val;
} cmdq_dma_config_c;
typedef union {
	struct {
		/*Max burst leng for AXI;*/
		uint32_t reg_max_burst_len:8;
		/*Outstanding enable;*/
		uint32_t reg_ot_enable:1;
		/*Overwrite axi config by SW;*/
		uint32_t reg_sw_overwrite:1;
	};
	uint32_t val;
} cmdq_axi_config_c;
typedef union {
	struct {
		/*Job start;*/
		uint32_t reg_job_start:1;
		/*Restart next cmd;*/
		uint32_t reg_cmd_restart:1;
		/*restart cmd by
		1 : hw
		0 : sw;*/
		uint32_t reg_restart_hw_mod:1;
		/*enable cmdq_idle for power saving
		1 : enable
		0 : disable;*/
		uint32_t reg_cmdq_idle_en:1;
	};
	uint32_t val;
} cmdq_job_ctl_c;
typedef union {
	struct {
		/*APB Base address;*/
		uint32_t reg_base_addr:16;
		/*APB pprot[2:0];*/
		uint32_t reg_apb_pprot:3;
	};
	uint32_t val;
} cmdq_apb_para_c;
typedef union {
	struct {
		/*CMDQ debug bus 0;*/
		uint32_t reg_debus0:32;
	};
	uint32_t val;
} cmdq_debug_bus0_c;
typedef union {
	struct {
		/*CMDQ debug bus 1;*/
		uint32_t reg_debus1:32;
	};
	uint32_t val;
} cmdq_debug_bus1_c;
typedef union {
	struct {
		/*CMDQ debug bus 2;*/
		uint32_t reg_debus2:32;
	};
	uint32_t val;
} cmdq_debug_bus2_c;
typedef union {
	struct {
		/*CMDQ debug bus 3;*/
		uint32_t reg_debus3:32;
	};
	uint32_t val;
} cmdq_debug_bus3_c;
typedef union {
	struct {
		/*CMDQ debug bus selection;*/
		uint32_t reg_debus_sel:2;
	};
	uint32_t val;
} cmdq_debug_bus_sel_c;
typedef union {
	struct {
		/*Dummy register;*/
		uint32_t reg_dummy:32;
	};
	uint32_t val;
} cmdq_dummy_c;
typedef union {
	struct {
		/*task done status;*/
		uint32_t reg_task_done:8;
	};
	uint32_t val;
} cmdq_task_done_sts_c;
typedef union {
	struct {
		/*DMA start address for task 0;*/
		uint32_t reg_dma_addr_tsk0;
	};
	uint32_t val;
} cmdq_dma_addr_tsk0_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 0;*/
		uint32_t reg_dma_cnt_tsk0:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk0_c;
typedef union {
	struct {
		/*DMA start address for task 1;*/
		uint32_t reg_dma_addr_tsk1;
	};
	uint32_t val;
} cmdq_dma_addr_tsk1_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 1;*/
		uint32_t reg_dma_cnt_tsk1:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk1_c;
typedef union {
	struct {
		/*DMA start address for task 2;*/
		uint32_t reg_dma_addr_tsk2;
	};
	uint32_t val;
} cmdq_dma_addr_tsk2_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 2;*/
		uint32_t reg_dma_cnt_tsk2:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk2_c;
typedef union {
	struct {
		/*DMA start address for task 3;*/
		uint32_t reg_dma_addr_tsk3;
	};
	uint32_t val;
} cmdq_dma_addr_tsk3_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 3;*/
		uint32_t reg_dma_cnt_tsk3:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk3_c;
typedef union {
	struct {
		/*DMA start address for task 4;*/
		uint32_t reg_dma_addr_tsk4;
	};
	uint32_t val;
} cmdq_dma_addr_tsk4_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 4;*/
		uint32_t reg_dma_cnt_tsk4:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk4_c;
typedef union {
	struct {
		/*DMA start address for task 5;*/
		uint32_t reg_dma_addr_tsk5;
	};
	uint32_t val;
} cmdq_dma_addr_tsk5_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 5;*/
		uint32_t reg_dma_cnt_tsk5:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk5_c;
typedef union {
	struct {
		/*DMA start address for task 6;*/
		uint32_t reg_dma_addr_tsk6;
	};
	uint32_t val;
} cmdq_dma_addr_tsk6_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 6;*/
		uint32_t reg_dma_cnt_tsk6:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk6_c;
typedef union {
	struct {
		/*DMA start address for task 7;*/
		uint32_t reg_dma_addr_tsk7;
	};
	uint32_t val;
} cmdq_dma_addr_tsk7_c;
typedef union {
	struct {
		/*DMA transfer count in bytes for task 7;*/
		uint32_t reg_dma_cnt_tsk7:32;
	};
	uint32_t val;
} cmdq_dma_cnt_tsk7_c;
typedef struct {
	volatile cmdq_int_event_c int_event;
	volatile cmdq_int_en_c int_en;
	volatile cmdq_dma_addr_l_c dma_addr_l;
	volatile cmdq_dma_addr_h_c dma_addr_h;
	volatile cmdq_dma_cnt_c dma_cnt;
	volatile cmdq_dma_config_c dma_config;
	volatile cmdq_axi_config_c axi_config;
	volatile cmdq_job_ctl_c job_ctl;
	volatile uint32_t _apb_para_0; // 0x20
	volatile cmdq_apb_para_c apb_para;
	volatile cmdq_debug_bus0_c debug_bus0;
	volatile cmdq_debug_bus1_c debug_bus1;
	volatile cmdq_debug_bus2_c debug_bus2;
	volatile cmdq_debug_bus3_c debug_bus3;
	volatile cmdq_debug_bus_sel_c debug_bus_sel;
	volatile cmdq_dummy_c dummy;
	volatile cmdq_task_done_sts_c task_done_sts;
	volatile uint32_t _dma_addr_tsk0_0; // 0x44
	volatile uint32_t _dma_addr_tsk0_1; // 0x48
	volatile uint32_t _dma_addr_tsk0_2; // 0x4C
	volatile uint32_t _dma_addr_tsk0_3; // 0x50
	volatile uint32_t _dma_addr_tsk0_4; // 0x54
	volatile uint32_t _dma_addr_tsk0_5; // 0x58
	volatile uint32_t _dma_addr_tsk0_6; // 0x5C
	volatile uint32_t _dma_addr_tsk0_7; // 0x60
	volatile uint32_t _dma_addr_tsk0_8; // 0x64
	volatile uint32_t _dma_addr_tsk0_9; // 0x68
	volatile uint32_t _dma_addr_tsk0_10; // 0x6C
	volatile uint32_t _dma_addr_tsk0_11; // 0x70
	volatile uint32_t _dma_addr_tsk0_12; // 0x74
	volatile uint32_t _dma_addr_tsk0_13; // 0x78
	volatile uint32_t _dma_addr_tsk0_14; // 0x7C
	volatile cmdq_dma_addr_tsk0_c dma_addr_tsk0;
	volatile uint32_t _dma_cnt_tsk0_0; // 0x84
	volatile cmdq_dma_cnt_tsk0_c dma_cnt_tsk0;
	volatile uint32_t _dma_addr_tsk1_0; // 0x8C
	volatile cmdq_dma_addr_tsk1_c dma_addr_tsk1;
	volatile uint32_t _dma_cnt_tsk1_0; // 0x94
	volatile cmdq_dma_cnt_tsk1_c dma_cnt_tsk1;
	volatile uint32_t _dma_addr_tsk2_0; // 0x9C
	volatile cmdq_dma_addr_tsk2_c dma_addr_tsk2;
	volatile uint32_t _dma_cnt_tsk2_0; // 0xA4
	volatile cmdq_dma_cnt_tsk2_c dma_cnt_tsk2;
	volatile uint32_t _dma_addr_tsk3_0; // 0xAC
	volatile cmdq_dma_addr_tsk3_c dma_addr_tsk3;
	volatile uint32_t _dma_cnt_tsk3_0; // 0xB4
	volatile cmdq_dma_cnt_tsk3_c dma_cnt_tsk3;
	volatile uint32_t _dma_addr_tsk4_0; // 0xBC
	volatile cmdq_dma_addr_tsk4_c dma_addr_tsk4;
	volatile uint32_t _dma_cnt_tsk4_0; // 0xC4
	volatile cmdq_dma_cnt_tsk4_c dma_cnt_tsk4;
	volatile uint32_t _dma_addr_tsk5_0; // 0xCC
	volatile cmdq_dma_addr_tsk5_c dma_addr_tsk5;
	volatile uint32_t _dma_cnt_tsk5_0; // 0xD4
	volatile cmdq_dma_cnt_tsk5_c dma_cnt_tsk5;
	volatile uint32_t _dma_addr_tsk6_0; // 0xDC
	volatile cmdq_dma_addr_tsk6_c dma_addr_tsk6;
	volatile uint32_t _dma_cnt_tsk6_0; // 0xE4
	volatile cmdq_dma_cnt_tsk6_c dma_cnt_tsk6;
	volatile uint32_t _dma_addr_tsk7_0; // 0xEC
	volatile cmdq_dma_addr_tsk7_c dma_addr_tsk7;
	volatile uint32_t _dma_cnt_tsk7_0; // 0xF4
	volatile cmdq_dma_cnt_tsk7_c dma_cnt_tsk7;
} cmdq_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void cmdq_dump_ini(FILE* fp, cmdq_c* p) {
	fprintf(fp, "reg_cmdq_int = 0x%x\n",p->int_event.reg_cmdq_int);
	fprintf(fp, "reg_cmdq_end = 0x%x\n",p->int_event.reg_cmdq_end);
	fprintf(fp, "reg_cmdq_wait = 0x%x\n",p->int_event.reg_cmdq_wait);
	fprintf(fp, "reg_isp_pslverr = 0x%x\n",p->int_event.reg_isp_pslverr);
	fprintf(fp, "reg_task_end = 0x%x\n",p->int_event.reg_task_end);
	fprintf(fp, "reg_cmdq_int_en = 0x%x\n",p->int_en.reg_cmdq_int_en);
	fprintf(fp, "reg_cmdq_end_en = 0x%x\n",p->int_en.reg_cmdq_end_en);
	fprintf(fp, "reg_cmdq_wait_en = 0x%x\n",p->int_en.reg_cmdq_wait_en);
	fprintf(fp, "reg_isp_pslverr_en = 0x%x\n",p->int_en.reg_isp_pslverr_en);
	fprintf(fp, "reg_task_end_en = 0x%x\n",p->int_en.reg_task_end_en);
	fprintf(fp, "reg_dma_addr_l = 0x%x\n",p->dma_addr_l.reg_dma_addr_l);
	fprintf(fp, "reg_dma_cnt = 0x%x\n",p->dma_cnt.reg_dma_cnt);
	fprintf(fp, "reg_dma_rsv = 0x%x\n",p->dma_config.reg_dma_rsv);
	fprintf(fp, "reg_adma_en = 0x%x\n",p->dma_config.reg_adma_en);
	fprintf(fp, "reg_task_en = 0x%x\n",p->dma_config.reg_task_en);
	fprintf(fp, "reg_max_burst_len = 0x%x\n",p->axi_config.reg_max_burst_len);
	fprintf(fp, "reg_ot_enable = 0x%x\n",p->axi_config.reg_ot_enable);
	fprintf(fp, "reg_sw_overwrite = 0x%x\n",p->axi_config.reg_sw_overwrite);
	fprintf(fp, "reg_job_start = 0x%x\n",p->job_ctl.reg_job_start);
	fprintf(fp, "reg_cmd_restart = 0x%x\n",p->job_ctl.reg_cmd_restart);
	fprintf(fp, "reg_restart_hw_mod = 0x%x\n",p->job_ctl.reg_restart_hw_mod);
	fprintf(fp, "reg_cmdq_idle_en = 0x%x\n",p->job_ctl.reg_cmdq_idle_en);
	fprintf(fp, "reg_base_addr = 0x%x\n",p->apb_para.reg_base_addr);
	fprintf(fp, "reg_apb_pprot = 0x%x\n",p->apb_para.reg_apb_pprot);
	fprintf(fp, "reg_debus0 = 0x%x\n",p->debug_bus0.reg_debus0);
	fprintf(fp, "reg_debus1 = 0x%x\n",p->debug_bus1.reg_debus1);
	fprintf(fp, "reg_debus2 = 0x%x\n",p->debug_bus2.reg_debus2);
	fprintf(fp, "reg_debus3 = 0x%x\n",p->debug_bus3.reg_debus3);
	fprintf(fp, "reg_debus_sel = 0x%x\n",p->debug_bus_sel.reg_debus_sel);
	fprintf(fp, "reg_dummy = 0x%x\n",p->dummy.reg_dummy);
	fprintf(fp, "reg_task_done = 0x%x\n",p->task_done_sts.reg_task_done);
	fprintf(fp, "reg_dma_addr_tsk0 = 0x%x\n",p->dma_addr_tsk0.reg_dma_addr_tsk0);
	fprintf(fp, "reg_dma_cnt_tsk0 = 0x%x\n",p->dma_cnt_tsk0.reg_dma_cnt_tsk0);
	fprintf(fp, "reg_dma_addr_tsk1 = 0x%x\n",p->dma_addr_tsk1.reg_dma_addr_tsk1);
	fprintf(fp, "reg_dma_cnt_tsk1 = 0x%x\n",p->dma_cnt_tsk1.reg_dma_cnt_tsk1);
	fprintf(fp, "reg_dma_addr_tsk2 = 0x%x\n",p->dma_addr_tsk2.reg_dma_addr_tsk2);
	fprintf(fp, "reg_dma_cnt_tsk2 = 0x%x\n",p->dma_cnt_tsk2.reg_dma_cnt_tsk2);
	fprintf(fp, "reg_dma_addr_tsk3 = 0x%x\n",p->dma_addr_tsk3.reg_dma_addr_tsk3);
	fprintf(fp, "reg_dma_cnt_tsk3 = 0x%x\n",p->dma_cnt_tsk3.reg_dma_cnt_tsk3);
	fprintf(fp, "reg_dma_addr_tsk4 = 0x%x\n",p->dma_addr_tsk4.reg_dma_addr_tsk4);
	fprintf(fp, "reg_dma_cnt_tsk4 = 0x%x\n",p->dma_cnt_tsk4.reg_dma_cnt_tsk4);
	fprintf(fp, "reg_dma_addr_tsk5 = 0x%x\n",p->dma_addr_tsk5.reg_dma_addr_tsk5);
	fprintf(fp, "reg_dma_cnt_tsk5 = 0x%x\n",p->dma_cnt_tsk5.reg_dma_cnt_tsk5);
	fprintf(fp, "reg_dma_addr_tsk6 = 0x%x\n",p->dma_addr_tsk6.reg_dma_addr_tsk6);
	fprintf(fp, "reg_dma_cnt_tsk6 = 0x%x\n",p->dma_cnt_tsk6.reg_dma_cnt_tsk6);
	fprintf(fp, "reg_dma_addr_tsk7 = 0x%x\n",p->dma_addr_tsk7.reg_dma_addr_tsk7);
	fprintf(fp, "reg_dma_cnt_tsk7 = 0x%x\n",p->dma_cnt_tsk7.reg_dma_cnt_tsk7);

}
static void cmdq_print(cmdq_c* p) {
    fprintf(stderr, "cmdq\n");
	fprintf(stderr, "\tINT_EVENT.reg_cmdq_int = 0x%x\n", p->int_event.reg_cmdq_int);
	fprintf(stderr, "\tINT_EVENT.reg_cmdq_end = 0x%x\n", p->int_event.reg_cmdq_end);
	fprintf(stderr, "\tINT_EVENT.reg_cmdq_wait = 0x%x\n", p->int_event.reg_cmdq_wait);
	fprintf(stderr, "\tINT_EVENT.reg_isp_pslverr = 0x%x\n", p->int_event.reg_isp_pslverr);
	fprintf(stderr, "\tINT_EVENT.reg_task_end = 0x%x\n", p->int_event.reg_task_end);
	fprintf(stderr, "\tINT_EN.reg_cmdq_int_en = 0x%x\n", p->int_en.reg_cmdq_int_en);
	fprintf(stderr, "\tINT_EN.reg_cmdq_end_en = 0x%x\n", p->int_en.reg_cmdq_end_en);
	fprintf(stderr, "\tINT_EN.reg_cmdq_wait_en = 0x%x\n", p->int_en.reg_cmdq_wait_en);
	fprintf(stderr, "\tINT_EN.reg_isp_pslverr_en = 0x%x\n", p->int_en.reg_isp_pslverr_en);
	fprintf(stderr, "\tINT_EN.reg_task_end_en = 0x%x\n", p->int_en.reg_task_end_en);
	fprintf(stderr, "\tDMA_ADDR.reg_dma_addr_l = 0x%x\n", p->dma_addr_l.reg_dma_addr_l);
	fprintf(stderr, "\tDMA_CNT.reg_dma_cnt = 0x%x\n", p->dma_cnt.reg_dma_cnt);
	fprintf(stderr, "\tDMA_CONFIG.reg_dma_rsv = 0x%x\n", p->dma_config.reg_dma_rsv);
	fprintf(stderr, "\tDMA_CONFIG.reg_adma_en = 0x%x\n", p->dma_config.reg_adma_en);
	fprintf(stderr, "\tDMA_CONFIG.reg_task_en = 0x%x\n", p->dma_config.reg_task_en);
	fprintf(stderr, "\tAXI_CONFIG.reg_max_burst_len = 0x%x\n", p->axi_config.reg_max_burst_len);
	fprintf(stderr, "\tAXI_CONFIG.reg_ot_enable = 0x%x\n", p->axi_config.reg_ot_enable);
	fprintf(stderr, "\tAXI_CONFIG.reg_sw_overwrite = 0x%x\n", p->axi_config.reg_sw_overwrite);
	fprintf(stderr, "\tJOB_CTL.reg_job_start = 0x%x\n", p->job_ctl.reg_job_start);
	fprintf(stderr, "\tJOB_CTL.reg_cmd_restart = 0x%x\n", p->job_ctl.reg_cmd_restart);
	fprintf(stderr, "\tJOB_CTL.reg_restart_hw_mod = 0x%x\n", p->job_ctl.reg_restart_hw_mod);
	fprintf(stderr, "\tJOB_CTL.reg_cmdq_idle_en = 0x%x\n", p->job_ctl.reg_cmdq_idle_en);
	fprintf(stderr, "\tAPB_PARA.reg_base_addr = 0x%x\n", p->apb_para.reg_base_addr);
	fprintf(stderr, "\tAPB_PARA.reg_apb_pprot = 0x%x\n", p->apb_para.reg_apb_pprot);
	fprintf(stderr, "\tDEBUG_BUS0.reg_debus0 = 0x%x\n", p->debug_bus0.reg_debus0);
	fprintf(stderr, "\tDEBUG_BUS1.reg_debus1 = 0x%x\n", p->debug_bus1.reg_debus1);
	fprintf(stderr, "\tDEBUG_BUS2.reg_debus2 = 0x%x\n", p->debug_bus2.reg_debus2);
	fprintf(stderr, "\tDEBUG_BUS3.reg_debus3 = 0x%x\n", p->debug_bus3.reg_debus3);
	fprintf(stderr, "\tDEBUG_BUS_SEL.reg_debus_sel = 0x%x\n", p->debug_bus_sel.reg_debus_sel);
	fprintf(stderr, "\tDUMMY.reg_dummy = 0x%x\n", p->dummy.reg_dummy);
	fprintf(stderr, "\tTASK_DONE_STS.reg_task_done = 0x%x\n", p->task_done_sts.reg_task_done);
	fprintf(stderr, "\tDMA_ADDR_TSK0.reg_dma_addr_tsk0 = 0x%x\n", p->dma_addr_tsk0.reg_dma_addr_tsk0);
	fprintf(stderr, "\tDMA_CNT_TSK0.reg_dma_cnt_tsk0 = 0x%x\n", p->dma_cnt_tsk0.reg_dma_cnt_tsk0);
	fprintf(stderr, "\tDMA_ADDR_TSK1.reg_dma_addr_tsk1 = 0x%x\n", p->dma_addr_tsk1.reg_dma_addr_tsk1);
	fprintf(stderr, "\tDMA_CNT_TSK1.reg_dma_cnt_tsk1 = 0x%x\n", p->dma_cnt_tsk1.reg_dma_cnt_tsk1);
	fprintf(stderr, "\tDMA_ADDR_TSK2.reg_dma_addr_tsk2 = 0x%x\n", p->dma_addr_tsk2.reg_dma_addr_tsk2);
	fprintf(stderr, "\tDMA_CNT_TSK2.reg_dma_cnt_tsk2 = 0x%x\n", p->dma_cnt_tsk2.reg_dma_cnt_tsk2);
	fprintf(stderr, "\tDMA_ADDR_TSK3.reg_dma_addr_tsk3 = 0x%x\n", p->dma_addr_tsk3.reg_dma_addr_tsk3);
	fprintf(stderr, "\tDMA_CNT_TSK3.reg_dma_cnt_tsk3 = 0x%x\n", p->dma_cnt_tsk3.reg_dma_cnt_tsk3);
	fprintf(stderr, "\tDMA_ADDR_TSK4.reg_dma_addr_tsk4 = 0x%x\n", p->dma_addr_tsk4.reg_dma_addr_tsk4);
	fprintf(stderr, "\tDMA_CNT_TSK4.reg_dma_cnt_tsk4 = 0x%x\n", p->dma_cnt_tsk4.reg_dma_cnt_tsk4);
	fprintf(stderr, "\tDMA_ADDR_TSK5.reg_dma_addr_tsk5 = 0x%x\n", p->dma_addr_tsk5.reg_dma_addr_tsk5);
	fprintf(stderr, "\tDMA_CNT_TSK5.reg_dma_cnt_tsk5 = 0x%x\n", p->dma_cnt_tsk5.reg_dma_cnt_tsk5);
	fprintf(stderr, "\tDMA_ADDR_TSK6.reg_dma_addr_tsk6 = 0x%x\n", p->dma_addr_tsk6.reg_dma_addr_tsk6);
	fprintf(stderr, "\tDMA_CNT_TSK6.reg_dma_cnt_tsk6 = 0x%x\n", p->dma_cnt_tsk6.reg_dma_cnt_tsk6);
	fprintf(stderr, "\tDMA_ADDR_TSK7.reg_dma_addr_tsk7 = 0x%x\n", p->dma_addr_tsk7.reg_dma_addr_tsk7);
	fprintf(stderr, "\tDMA_CNT_TSK7.reg_dma_cnt_tsk7 = 0x%x\n", p->dma_cnt_tsk7.reg_dma_cnt_tsk7);

}
#pragma GCC diagnostic pop
#define DEFINE_CMDQ_C(X) \
	 cmdq_c X = \
{\
	{	/* int_event.reg_cmdq_int = */0x0,\
	/*.int_event.reg_cmdq_end = */0x0,\
	/*.int_event.reg_cmdq_wait = */0x0,\
	/*.int_event.reg_isp_pslverr = */0x0,\
	/*.int_event.reg_task_end = */0x0,\
	},\
	{	/*.int_en.reg_cmdq_int_en = */0x0,\
	/*.int_en.reg_cmdq_end_en = */0x0,\
	/*.int_en.reg_cmdq_wait_en = */0x0,\
	/*.int_en.reg_isp_pslverr_en = */0x0,\
	/*.int_en.reg_task_end_en = */0x0,\
	},\
	{	/*.DMA_ADDR.reg_dma_addr_l = */0x0,\
	},\
	{	/*.dma_cnt.reg_dma_cnt = */0x0,\
	},\
	{	/*.dma_config.reg_dma_rsv = */0x0,\
	/*.dma_config.reg_adma_en = */0x0,\
	/*.dma_config.reg_task_en = */0x0,\
	},\
	{	/*.axi_config.reg_max_burst_len = */0x3,\
	/*.axi_config.reg_ot_enable = */0x1,\
	/*.axi_config.reg_sw_overwrite = */0x0,\
	},\
	{	/*.job_ctl.reg_job_start = */0x0,\
	/*.job_ctl.reg_cmd_restart = */0x0,\
	/*.job_ctl.reg_restart_hw_mod = */0x0,\
	/*.job_ctl.reg_cmdq_idle_en = */0x0,\
	},\
	{	/*.apb_para.reg_base_addr = */0x0,\
	/*.apb_para.reg_apb_pprot = */0x0,\
	},\
	{	/*.debug_bus0.reg_debus0 = */0x0,\
	},\
	{	/*.debug_bus1.reg_debus1 = */0x0,\
	},\
	{	/*.debug_bus2.reg_debus2 = */0x0,\
	},\
	{	/*.debug_bus3.reg_debus3 = */0x0,\
	},\
	{	/*.debug_bus_sel.reg_debus_sel = */0x0,\
	},\
	{	/*.dummy.reg_dummy = */0xffff,\
	},\
	{	/*.task_done_sts.reg_task_done = */0x0,\
	},\
	{	/*.dma_addr_tsk0.reg_dma_addr_tsk0 = */0x0,\
	},\
	{	/*.dma_cnt_tsk0.reg_dma_cnt_tsk0 = */0x0,\
	},\
	{	/*.dma_addr_tsk1.reg_dma_addr_tsk1 = */0x0,\
	},\
	{	/*.dma_cnt_tsk1.reg_dma_cnt_tsk1 = */0x0,\
	},\
	{	/*.dma_addr_tsk2.reg_dma_addr_tsk2 = */0x0,\
	},\
	{	/*.dma_cnt_tsk2.reg_dma_cnt_tsk2 = */0x0,\
	},\
	{	/*.dma_addr_tsk3.reg_dma_addr_tsk3 = */0x0,\
	},\
	{	/*.dma_cnt_tsk3.reg_dma_cnt_tsk3 = */0x0,\
	},\
	{	/*.dma_addr_tsk4.reg_dma_addr_tsk4 = */0x0,\
	},\
	{	/*.dma_cnt_tsk4.reg_dma_cnt_tsk4 = */0x0,\
	},\
	{	/*.dma_addr_tsk5.reg_dma_addr_tsk5 = */0x0,\
	},\
	{	/*.dma_cnt_tsk5.reg_dma_cnt_tsk5 = */0x0,\
	},\
	{	/*.dma_addr_tsk6.reg_dma_addr_tsk6 = */0x0,\
	},\
	{	/*.dma_cnt_tsk6.reg_dma_cnt_tsk6 = */0x0,\
	},\
	{	/*.dma_addr_tsk7.reg_dma_addr_tsk7 = */0x0,\
	},\
	{	/*.dma_cnt_tsk7.reg_dma_cnt_tsk7 = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_CMDQ_C \
{\
	.int_event.reg_cmdq_int = 0x0,\
	.int_event.reg_cmdq_end = 0x0,\
	.int_event.reg_cmdq_wait = 0x0,\
	.int_event.reg_isp_pslverr = 0x0,\
	.int_event.reg_task_end = 0x0,\
	.int_en.reg_cmdq_int_en = 0x0,\
	.int_en.reg_cmdq_end_en = 0x0,\
	.int_en.reg_cmdq_wait_en = 0x0,\
	.int_en.reg_isp_pslverr_en = 0x0,\
	.int_en.reg_task_end_en = 0x0,\
	.dma_addr_l.reg_dma_addr_l = 0x0,\
	.dma_addr_h.reg_dma_addr_h = 0x0,\
	.dma_cnt.reg_dma_cnt = 0x0,\
	.dma_config.reg_dma_rsv = 0x0,\
	.dma_config.reg_adma_en = 0x0,\
	.dma_config.reg_task_en = 0x0,\
	.axi_config.reg_max_burst_len = 0x3,\
	.axi_config.reg_ot_enable = 0x1,\
	.axi_config.reg_sw_overwrite = 0x0,\
	.job_ctl.reg_job_start = 0x0,\
	.job_ctl.reg_cmd_restart = 0x0,\
	.job_ctl.reg_restart_hw_mod = 0x0,\
	.job_ctl.reg_cmdq_idle_en = 0x0,\
	.apb_para.reg_base_addr = 0x0,\
	.apb_para.reg_apb_pprot = 0x0,\
	.debug_bus0.reg_debus0 = 0x0,\
	.debug_bus1.reg_debus1 = 0x0,\
	.debug_bus2.reg_debus2 = 0x0,\
	.debug_bus3.reg_debus3 = 0x0,\
	.debug_bus_sel.reg_debus_sel = 0x0,\
	.dummy.reg_dummy = 0xffff,\
	.task_done_sts.reg_task_done = 0x0,\
	.dma_addr_tsk0.reg_dma_addr_tsk0 = 0x0,\
	.dma_cnt_tsk0.reg_dma_cnt_tsk0 = 0x0,\
	.dma_addr_tsk1.reg_dma_addr_tsk1 = 0x0,\
	.dma_cnt_tsk1.reg_dma_cnt_tsk1 = 0x0,\
	.dma_addr_tsk2.reg_dma_addr_tsk2 = 0x0,\
	.dma_cnt_tsk2.reg_dma_cnt_tsk2 = 0x0,\
	.dma_addr_tsk3.reg_dma_addr_tsk3 = 0x0,\
	.dma_cnt_tsk3.reg_dma_cnt_tsk3 = 0x0,\
	.dma_addr_tsk4.reg_dma_addr_tsk4 = 0x0,\
	.dma_cnt_tsk4.reg_dma_cnt_tsk4 = 0x0,\
	.dma_addr_tsk5.reg_dma_addr_tsk5 = 0x0,\
	.dma_cnt_tsk5.reg_dma_cnt_tsk5 = 0x0,\
	.dma_addr_tsk6.reg_dma_addr_tsk6 = 0x0,\
	.dma_cnt_tsk6.reg_dma_cnt_tsk6 = 0x0,\
	.dma_addr_tsk7.reg_dma_addr_tsk7 = 0x0,\
	.dma_cnt_tsk7.reg_dma_cnt_tsk7 = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_CMDQ_STRUCT_H__

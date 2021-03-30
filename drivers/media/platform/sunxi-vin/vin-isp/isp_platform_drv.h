
/*
 ******************************************************************************
 *
 * isp_platform_drv.h
 *
 * Hawkview ISP - isp_platform_drv.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/20	      Second Version
 *
 ******************************************************************************
 */
#ifndef _ISP_PLATFORM_DRV_H_
#define _ISP_PLATFORM_DRV_H_
#include <linux/string.h>
#include "bsp_isp_comm.h"

enum isp_channel_real {
	SUB_CH_REAL = 0,
	MAIN_CH_REAL = 1,
	ROT_CH_REAL = 2,
	ISP_MAX_CH_NUM_REAL,
};

struct isp_bsp_fun_array {

	void (*map_reg_addr) (unsigned long, unsigned long);
	void (*map_load_dram_addr) (unsigned long, unsigned long);
	void (*map_saved_dram_addr) (unsigned long, unsigned long);
	void (*isp_set_interface) (unsigned long, enum isp_src_interface, enum isp_src);
	void (*isp_enable) (unsigned long, int);
	void (*isp_ch_enable) (unsigned long, int, int);
	void (*isp_wdr_ch_seq) (unsigned long, int);
	void (*isp_set_para_ready) (unsigned long, enum ready_flag);
	unsigned int (*isp_get_para_ready) (unsigned long);
	void (*isp_capture_start) (unsigned long, int);
	void (*isp_capture_stop) (unsigned long, int);
	void (*isp_irq_enable) (unsigned long, unsigned int);
	void (*isp_irq_disable) (unsigned long, unsigned int);
	unsigned int (*isp_get_irq_status) (unsigned long, unsigned int);
	void (*isp_clr_irq_status) (unsigned long, unsigned int);
	void (*isp_debug_output_cfg) (unsigned long, int, int);
	int (*isp_int_get_enable) (unsigned long);
	void (*isp_set_line_int_num) (unsigned long, unsigned int);
	void (*isp_set_rot_of_line_num) (unsigned long, unsigned int);
	void (*isp_set_load_addr) (unsigned long, unsigned long);
	void (*isp_set_saved_addr) (unsigned long, unsigned long);
	void (*isp_set_table_addr) (unsigned long, enum isp_input_tables, unsigned long);
	void (*isp_set_statistics_addr) (unsigned long, unsigned long);
	void (*isp_channel_enable) (unsigned long, enum isp_channel);
	void (*isp_channel_disable) (unsigned long, enum isp_channel);
	void (*isp_reg_test) (unsigned long);
	void (*isp_print_reg_saved) (unsigned long);
	void (*isp_update_table) (unsigned long, unsigned short);
	void (*isp_set_output_speed) (unsigned long, enum isp_output_speed);
	unsigned int (*isp_get_isp_ver)(unsigned long, unsigned int *, unsigned int *);

};

struct isp_platform_drv {
	int platform_id;
	struct isp_bsp_fun_array *fun_array;
};

int isp_platform_register(struct isp_platform_drv *isp_drv);

int isp_platform_init(unsigned int platform_id);

struct isp_platform_drv *isp_get_driver(void);


#endif	/*_ISP_PLATFORM_DRV_H_*/

/*
 *****************************************************************************
 *
 * bsp_isp.h
 *
 * Hawkview ISP - bsp_isp.h module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/12/25	    First Version
 *
 ******************************************************************************
 */
#ifndef __BSP__ISP__H
#define __BSP__ISP__H

#include "../utility/bsp_common.h"
#include "bsp_isp_comm.h"

#define MAX_ISP_SRC_CH_NUM 3

enum isp_src_ch_mode {
	ISP_SINGLE_CH,
	ISP_DUAL_CH,
};

struct isp_frame_info {
	unsigned int byte_size;
	struct isp_size pixel_size[ISP_MAX_CH_NUM];
	unsigned int scale_ratio;
	enum bus_pixeltype bus_pixel_type;
	unsigned int pixel_fmt[ISP_MAX_CH_NUM];
};

struct isp_init_para {
	enum isp_src_ch_mode isp_src_ch_mode;
	unsigned int isp_src_ch_en[MAX_ISP_SRC_CH_NUM];
};

struct isp_table_addr {
	void *isp_def_lut_tbl_vaddr;
	void *isp_def_lut_tbl_paddr;
	void *isp_def_lut_tbl_dma_addr;
	void *isp_lsc_tbl_vaddr;
	void *isp_lsc_tbl_paddr;
	void *isp_lsc_tbl_dma_addr;
	void *isp_gamma_tbl_vaddr;
	void *isp_gamma_tbl_paddr;
	void *isp_gamma_tbl_dma_addr;
	void *isp_linear_tbl_vaddr;
	void *isp_linear_tbl_paddr;
	void *isp_linear_tbl_dma_addr;

	void *isp_drc_tbl_vaddr;
	void *isp_drc_tbl_paddr;
	void *isp_drc_tbl_dma_addr;
	void *isp_disc_tbl_vaddr;
	void *isp_disc_tbl_paddr;
	void *isp_disc_tbl_dma_addr;
};

void bsp_isp_enable(unsigned long id);
void bsp_isp_disable(unsigned long id);

void bsp_isp_channel_enable(unsigned long id, enum isp_channel ch);
void bsp_isp_channel_disable(unsigned long id, enum isp_channel ch);

void bsp_isp_video_capture_start(unsigned long id);
void bsp_isp_video_capture_stop(unsigned long id);
void bsp_isp_image_capture_start(unsigned long id);
void bsp_isp_image_capture_stop(unsigned long id);
unsigned int bsp_isp_get_para_ready(unsigned long id);
void bsp_isp_set_para_ready(unsigned long id);
void bsp_isp_clr_para_ready(unsigned long id);

void bsp_isp_irq_enable(unsigned long id, unsigned int irq_flag);
void bsp_isp_irq_disable(unsigned long id, unsigned int irq_flag);
unsigned int bsp_isp_get_irq_status(unsigned long id, unsigned int irq);
int bsp_isp_int_get_enable(unsigned long id);

void bsp_isp_clr_irq_status(unsigned long id, unsigned int irq);
void bsp_isp_set_statistics_addr(unsigned long id, unsigned int addr);

void bsp_isp_set_flip(unsigned long id, enum isp_channel ch, enum enable_flag on_off);
void bsp_isp_set_mirror(unsigned long id, enum isp_channel ch, enum enable_flag on_off);

void bsp_isp_set_input_fmt(unsigned long id, enum isp_input_fmt fmt, enum isp_input_seq seq_t);

void bsp_isp_set_output_fmt(unsigned long id, enum isp_output_fmt isp_fmt,
			    enum isp_output_seq seq_t, enum isp_channel ch);
int min_scale_w_shift(int x_ratio, int y_ratio);
void bsp_isp_set_ob_zone(unsigned long id, struct isp_size *black, struct isp_size *valid,
			 struct coor *xy, enum isp_src obc_valid_src);


void bsp_isp_set_base_addr(unsigned long id, unsigned long vaddr);
void bsp_isp_set_dma_load_addr(unsigned long id, unsigned long dma_addr);
void bsp_isp_set_dma_saved_addr(unsigned long id, unsigned long dma_addr);
void bsp_isp_set_map_load_addr(unsigned long id, unsigned long vaddr);
void bsp_isp_set_map_saved_addr(unsigned long id, unsigned long vaddr);
void bsp_isp_update_lut_lens_gamma_table(unsigned long id, struct isp_table_addr *tbl_addr);
void bsp_isp_update_drc_table(unsigned long id, struct isp_table_addr *tbl_addr);
void bsp_isp_init_platform(unsigned int platform_id);
void bsp_isp_init(unsigned long id, struct isp_init_para *para);
void bsp_isp_exit(unsigned long id);

void bsp_isp_module_enable(unsigned long id, unsigned int module);
void bsp_isp_module_disable(unsigned long id, unsigned int module);

#endif

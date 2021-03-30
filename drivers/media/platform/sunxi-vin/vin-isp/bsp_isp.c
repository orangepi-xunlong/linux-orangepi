
/*
 ******************************************************************************
 *
 * bsp_isp.c
 *
 * Hawkview ISP - bsp_isp.c module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/11/07	    First Version
 *
 ******************************************************************************
 */

#include <linux/string.h>
#include <linux/kernel.h>

#include "bsp_isp.h"
#include "isp_platform_drv.h"

#include "bsp_isp_comm.h"
#include "bsp_isp_algo.h"

#define  Q16_1_1                  ((1 << 16) >> 0)
#define  Q16_1_2                  ((1 << 16) >> 1)
#define  Q16_1_4                  ((1 << 16) >> 2)
static int isp_platform_id;
struct isp_bsp_fun_array *fun_array_curr;

void bsp_isp_enable(unsigned long id)
{
	fun_array_curr->isp_enable(id, 1);
}

void bsp_isp_disable(unsigned long id)
{
	fun_array_curr->isp_enable(id, 0);
}

void bsp_isp_channel_enable(unsigned long id, enum isp_channel ch)
{
	fun_array_curr->isp_ch_enable(id, ch, 1);
}

void bsp_isp_channel_disable(unsigned long id, enum isp_channel ch)
{
	fun_array_curr->isp_ch_enable(id, ch, 0);
}
void bsp_isp_video_capture_start(unsigned long id)
{

	fun_array_curr->isp_capture_start(id, VCAP_EN);
}

void bsp_isp_video_capture_stop(unsigned long id)
{

	fun_array_curr->isp_capture_stop(id, VCAP_EN);
}

void bsp_isp_image_capture_start(unsigned long id)
{

	fun_array_curr->isp_capture_start(id, SCAP_EN);
}

void bsp_isp_image_capture_stop(unsigned long id)
{

	fun_array_curr->isp_capture_stop(id, SCAP_EN);
}

unsigned int bsp_isp_get_para_ready(unsigned long id)
{
	return fun_array_curr->isp_get_para_ready(id);
}


void bsp_isp_set_para_ready(unsigned long id)
{
	fun_array_curr->isp_set_para_ready(id, PARA_READY);
}
void bsp_isp_clr_para_ready(unsigned long id)
{
	fun_array_curr->isp_set_para_ready(id, PARA_NOT_READY);
}

void bsp_isp_irq_enable(unsigned long id, unsigned int irq_flag)
{
	fun_array_curr->isp_irq_enable(id, irq_flag);
}
void bsp_isp_irq_disable(unsigned long id, unsigned int irq_flag)
{
	fun_array_curr->isp_irq_disable(id, irq_flag);
}

unsigned int bsp_isp_get_irq_status(unsigned long id, unsigned int irq)
{
	return fun_array_curr->isp_get_irq_status(id, irq);
}


void bsp_isp_clr_irq_status(unsigned long id, unsigned int irq)
{
	fun_array_curr->isp_clr_irq_status(id, irq);
}

int bsp_isp_int_get_enable(unsigned long id)
{
	return fun_array_curr->isp_int_get_enable(id);
}


void bsp_isp_set_statistics_addr(unsigned long id, unsigned int addr)
{
	fun_array_curr->isp_set_statistics_addr(id, addr);
}

void bsp_isp_set_base_addr(unsigned long id, unsigned long vaddr)
{
	fun_array_curr->map_reg_addr(id, vaddr);
}

void bsp_isp_set_dma_load_addr(unsigned long id, unsigned long dma_addr)
{
	fun_array_curr->isp_set_load_addr(id, dma_addr);
}

void bsp_isp_set_dma_saved_addr(unsigned long id, unsigned long dma_addr)
{
	fun_array_curr->isp_set_saved_addr(id, dma_addr);
}

void bsp_isp_set_map_load_addr(unsigned long id, unsigned long vaddr)
{
	fun_array_curr->map_load_dram_addr(id, vaddr);
}

void bsp_isp_set_map_saved_addr(unsigned long id, unsigned long vaddr)
{
	fun_array_curr->map_saved_dram_addr(id, vaddr);
}

void bsp_isp_update_lut_lens_gamma_table(unsigned long id, struct isp_table_addr *tbl_addr)
{
	fun_array_curr->isp_set_table_addr(id, LUT_LENS_GAMMA_TABLE,
		(unsigned long)(tbl_addr->isp_def_lut_tbl_dma_addr));
}

void bsp_isp_update_drc_table(unsigned long id, struct isp_table_addr *tbl_addr)
{
	fun_array_curr->isp_set_table_addr(id, DRC_TABLE,
		(unsigned long)(tbl_addr->isp_drc_tbl_dma_addr));
}

void bsp_isp_init(unsigned long id, struct isp_init_para *para)
{
	unsigned int i, j;

	enum isp_src_interface isp_src_sel[] = {ISP_CSI0, ISP_CSI1, ISP_CSI2};

	enum isp_src isp_src_ch[] = {ISP_SRC0, ISP_SRC1};

	j = 0;

	for (i = 0; i < MAX_ISP_SRC_CH_NUM; i++) {

		if (para->isp_src_ch_en[i] == 1) {
			if (fun_array_curr->isp_set_interface)
				fun_array_curr->isp_set_interface(id,
							isp_src_sel[i],
							isp_src_ch[j]);
			j++;
			if (para->isp_src_ch_mode == ISP_SINGLE_CH) {
				if (j == 1)
					break;
			} else if (para->isp_src_ch_mode == ISP_DUAL_CH) {
				if (j == 2)
					break;
			}
		}
	}
}


void bsp_isp_exit(unsigned long id)
{
	bsp_isp_disable(id);
	bsp_isp_irq_disable(id, ISP_IRQ_EN_ALL);
	bsp_isp_video_capture_stop(id);
}


void bsp_isp_print_reg_saved(unsigned long id)
{
	fun_array_curr->isp_print_reg_saved(id);
}

void bsp_isp_init_platform(unsigned int platform_id)
{
	struct isp_platform_drv *isp_platform;


	isp_platform_id = platform_id;
	isp_platform_init(platform_id);
	isp_platform = isp_get_driver();
	fun_array_curr = isp_platform->fun_array;
}

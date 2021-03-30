
/*
 ******************************************************************************
 *
 * sun8iw12p1_isp_reg_cfg.c
 *
 * Hawkview ISP - sun8iw12p1_isp_reg_cfg.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/30	      Second Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>
#include "../isp_platform_drv.h"
#include "sun8iw12p1_isp_reg.h"
#include "sun8iw12p1_isp_reg_cfg.h"


#include <linux/kernel.h>

#define  REG_FUNCTION_LOG do { \
		printk("%s, line: %d\n", __FUNCTION__, __LINE__); \
	} while (0)

#define ABS(x)	((x) > 0 ? (x) : -(x))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define SUN8IW12P1_ISP_MAX_NUM 2

/*
 *
 *  Load ISP register variables
 *
 */
struct sun8iw12p1_isp_reg {
	SUN8IW12P1_ISP_FE_CFG_REG_t *sun8iw12p1_isp_fe_cfg;
	SUN8IW12P1_ISP_FE_CTRL_REG_t *sun8iw12p1_isp_fe_ctrl;
	SUN8IW12P1_ISP_FE_INT_EN_REG_t *sun8iw12p1_isp_fe_int_en;
	SUN8IW12P1_ISP_FE_INT_STA_REG_t *sun8iw12p1_isp_fe_int_sta;
	SUN8IW12P1_ISP_DBG_OUTPUT_REG_t *sun8iw12p1_isp_dbg_output;
	SUN8IW12P1_ISP_LINE_INT_NUM_REG_t *sun8iw12p1_isp_line_int_num;
	SUN8IW12P1_ISP_ROT_OF_CFG_REG_t *sun8iw12p1_isp_rot_of_cfg;
	SUN8IW12P1_ISP_REG_LOAD_ADDR_REG_t *sun8iw12p1_isp_reg_load_addr;
	SUN8IW12P1_ISP_REG_SAVED_ADDR_REG_t *sun8iw12p1_isp_reg_saved_addr;
	SUN8IW12P1_ISP_LUT_LENS_GAMMA_ADDR_REG_t *sun8iw12p1_isp_lut_lens_gamma_addr;
	SUN8IW12P1_ISP_DRC_ADDR_REG_t *sun8iw12p1_isp_drc_addr;
	SUN8IW12P1_ISP_STATISTICS_ADDR_REG_t *sun8iw12p1_isp_statistics_addr;
	SUN8IW12P1_ISP_VER_CFG_REG_t *sun8iw12p1_isp_ver_cfg;
	SUN8IW12P1_ISP_SRAM_RW_OFFSET_REG_t *sun8iw12p1_isp_sram_rw_offset;
	SUN8IW12P1_ISP_SRAM_RW_DATA_REG_t *sun8iw12p1_isp_sram_rw_data;
};

struct sun8iw12p1_isp_reg sun8iw12p1_isp_regs[SUN8IW12P1_ISP_MAX_NUM];

/*
 * Register Address
 */

void sun8iw12p1_map_reg_addr(unsigned long id, unsigned long isp_reg_base)
{
	printk("map isp_reg_base = 0x%lx\n", isp_reg_base);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg = (SUN8IW12P1_ISP_FE_CFG_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_FE_CFG_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl = (SUN8IW12P1_ISP_FE_CTRL_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_FE_CTRL_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_en = (SUN8IW12P1_ISP_FE_INT_EN_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_FE_INT_EN_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_sta = (SUN8IW12P1_ISP_FE_INT_STA_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_FE_INT_STA_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_dbg_output = (SUN8IW12P1_ISP_DBG_OUTPUT_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_DBG_OUTPUT_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_line_int_num = (SUN8IW12P1_ISP_LINE_INT_NUM_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_LINE_INT_NUM_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_rot_of_cfg = (SUN8IW12P1_ISP_ROT_OF_CFG_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_ROT_OF_CFG_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_reg_load_addr = (SUN8IW12P1_ISP_REG_LOAD_ADDR_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_REG_LOAD_ADDR_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_reg_saved_addr = (SUN8IW12P1_ISP_REG_SAVED_ADDR_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_REG_SAVED_ADDR_REG_OFF);

	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_lut_lens_gamma_addr = (SUN8IW12P1_ISP_LUT_LENS_GAMMA_ADDR_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_LUT_LENS_GAMMA_ADDR_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_drc_addr = (SUN8IW12P1_ISP_DRC_ADDR_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_DRC_ADDR_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_statistics_addr = (SUN8IW12P1_ISP_STATISTICS_ADDR_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_STATISTICS_ADDR_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_ver_cfg = (SUN8IW12P1_ISP_VER_CFG_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_VER_CFG_REG_OFF);

	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_sram_rw_offset = (SUN8IW12P1_ISP_SRAM_RW_OFFSET_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_SRAM_RW_OFFSET_REG_OFF);
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_sram_rw_data = (SUN8IW12P1_ISP_SRAM_RW_DATA_REG_t *) (isp_reg_base + SUN8IW12P1_ISP_SRAM_RW_DATA_REG_OFF);
}

/*
 * Load DRAM Register Address
 */

void sun8iw12p1_map_load_dram_addr(unsigned long id, unsigned long isp_load_dram_base)
{

}

/*
 * Saved DRAM Register Address
 */
void sun8iw12p1_map_saved_dram_addr(unsigned long id, unsigned long isp_saved_dram_base)
{

}

void sun8iw12p1_isp_enable(unsigned long id, int enable)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.isp_enable = enable;
}

void sun8iw12p1_isp_ch_enable(unsigned long id, int ch, int enable)
{
	switch (ch) {
	case 0:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.isp_ch0_en = enable;
		break;
	case 1:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.isp_ch1_en = enable;
		break;
	case 2:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.isp_ch2_en = enable;
		break;
	case 3:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.isp_ch3_en = enable;
		break;
	default:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.isp_ch0_en = enable;
		break;
	}
}

void sun8iw12p1_isp_wdr_ch_seq(unsigned long id, int seq)
{
	switch (seq) {
	case 0:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.wdr_ch_seq = 0;
		break;
	case 1:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.wdr_ch_seq = 1;
		break;
	default:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_cfg->bits.wdr_ch_seq = 0;
		break;
	}
}
void sun8iw12p1_isp_set_para_ready(unsigned long id, enum ready_flag ready)
{
	if (ready == PARA_READY)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.para_ready = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.para_ready = 0;
}

unsigned int sun8iw12p1_isp_get_para_ready(unsigned long id)
{
	return sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.para_ready;
}

void sun8iw12p1_isp_update_table(unsigned long id, unsigned short table_update)
{

	if (table_update & LINEAR_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.linear_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.linear_update = 0;

	if (table_update & LENS_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.lens_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.lens_update = 0;

	if (table_update & GAMMA_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.gamma_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.gamma_update = 0;

	if (table_update & DRC_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.drc_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.drc_update = 0;

	if (table_update & DISC_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.disc_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.disc_update = 0;

	if (table_update & SATU_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.satu_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.satu_update = 0;

	if (table_update & WDR_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.wdr_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.wdr_update = 0;

	if (table_update & TDNF_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.tdnf_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.tdnf_update = 0;

	if (table_update & PLTM_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.pltm_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.pltm_update = 0;

	if (table_update & CEM_UPDATE)
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.cem_update = 1;
	else
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.cem_update = 0;
}
void sun8iw12p1_isp_capture_start(unsigned long id, int vcap_read_start)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.cap_en = 1;
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.vcap_read_start = vcap_read_start;
}

void sun8iw12p1_isp_capture_stop(unsigned long id, int vcap_read_start)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.cap_en = 0;
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_ctrl->bits.vcap_read_start = vcap_read_start;
}

void sun8iw12p1_isp_irq_enable(unsigned long id, unsigned int irq_flag)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_en->dwval |= irq_flag;
}

void sun8iw12p1_isp_irq_disable(unsigned long id, unsigned int irq_flag)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_en->dwval &= ~irq_flag;
}

int sun8iw12p1_isp_int_get_enable(unsigned long id)
{
	return sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_en->dwval;
}

unsigned int sun8iw12p1_isp_get_irq_status(unsigned long id, unsigned int irq_flag)
{
	return sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_sta->dwval & irq_flag;
}

void sun8iw12p1_isp_clr_irq_status(unsigned long id, unsigned int irq_flag)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_fe_int_sta->dwval |= irq_flag;
}

void sun8iw12p1_isp_debug_output_cfg(unsigned long id, int enable, int output_sel)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_dbg_output->bits.debug_en = enable;
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_dbg_output->bits.debug_sel = output_sel;
}

void sun8iw12p1_isp_set_line_int_num(unsigned long id, unsigned int line_num)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_line_int_num->bits.line_int_num = line_num;
}
void sun8iw12p1_isp_set_last_blank_cycle(unsigned long id, unsigned int last_blank_cycle)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_line_int_num->bits.last_blank_cycle = last_blank_cycle;
}

void sun8iw12p1_isp_set_rot_of_line_num(unsigned long id, unsigned int speed_mode)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_rot_of_cfg->bits.speed_mode = speed_mode;
}

void sun8iw12p1_isp_set_load_addr(unsigned long id, unsigned long addr)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_reg_load_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
}

void sun8iw12p1_isp_set_saved_addr(unsigned long id, unsigned long addr)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_reg_saved_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
}

void sun8iw12p1_isp_set_table_addr(unsigned long id, enum isp_input_tables table,
				      unsigned long addr)
{
	switch (table) {
	case LUT_LENS_GAMMA_TABLE:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_lut_lens_gamma_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
		break;
	case DRC_TABLE:
		sun8iw12p1_isp_regs[id].sun8iw12p1_isp_drc_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
		break;
	default:
		break;
	}
}

void sun8iw12p1_isp_set_statistics_addr(unsigned long id, unsigned long addr)
{
	sun8iw12p1_isp_regs[id].sun8iw12p1_isp_statistics_addr->dwval = addr >> ISP_ADDR_BIT_R_SHIFT;
}

unsigned int sun8iw12p1_isp_get_isp_ver(unsigned long id, unsigned int *major, unsigned int *minor)
{
	*major = sun8iw12p1_isp_regs[id].sun8iw12p1_isp_ver_cfg->bits.major_ver;
	*minor = sun8iw12p1_isp_regs[id].sun8iw12p1_isp_ver_cfg->bits.minor_ver;
	return sun8iw12p1_isp_regs[id].sun8iw12p1_isp_ver_cfg->dwval;
}


static struct isp_bsp_fun_array sun8iw12p1_fun_array = {
	.map_reg_addr =	sun8iw12p1_map_reg_addr,
	.map_load_dram_addr = sun8iw12p1_map_load_dram_addr,
	.map_saved_dram_addr = sun8iw12p1_map_saved_dram_addr,
	.isp_set_interface = NULL,
	.isp_enable = sun8iw12p1_isp_enable,
	.isp_ch_enable = sun8iw12p1_isp_ch_enable,
	.isp_wdr_ch_seq = sun8iw12p1_isp_wdr_ch_seq,
	.isp_set_para_ready = sun8iw12p1_isp_set_para_ready,
	.isp_get_para_ready = sun8iw12p1_isp_get_para_ready,
	.isp_capture_start = sun8iw12p1_isp_capture_start,
	.isp_capture_stop = sun8iw12p1_isp_capture_stop,
	.isp_irq_enable = sun8iw12p1_isp_irq_enable,
	.isp_irq_disable = sun8iw12p1_isp_irq_disable,
	.isp_get_irq_status = sun8iw12p1_isp_get_irq_status,
	.isp_clr_irq_status = sun8iw12p1_isp_clr_irq_status,
	.isp_debug_output_cfg = sun8iw12p1_isp_debug_output_cfg,
	.isp_int_get_enable = sun8iw12p1_isp_int_get_enable,
	.isp_set_line_int_num =	sun8iw12p1_isp_set_line_int_num,
	.isp_set_rot_of_line_num = sun8iw12p1_isp_set_rot_of_line_num,
	.isp_set_load_addr = sun8iw12p1_isp_set_load_addr,
	.isp_set_saved_addr = sun8iw12p1_isp_set_saved_addr,
	.isp_set_table_addr = sun8iw12p1_isp_set_table_addr,
	.isp_set_statistics_addr = sun8iw12p1_isp_set_statistics_addr,
	.isp_update_table = sun8iw12p1_isp_update_table,
	.isp_set_output_speed = NULL,
	.isp_get_isp_ver = sun8iw12p1_isp_get_isp_ver,
};

struct isp_platform_drv sun8iw12p1_isp_drv = {
	.platform_id = ISP_PLATFORM_SUN8IW7P1,
	.fun_array = &sun8iw12p1_fun_array,
};

int sun8iw12p1_isp_platform_register(void)
{
	return isp_platform_register(&sun8iw12p1_isp_drv);
}



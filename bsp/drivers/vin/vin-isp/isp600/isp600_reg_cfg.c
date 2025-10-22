/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * isp600_reg_cfg.c
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 *
 * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/io.h>
#include <linux/string.h>
#include "isp600_reg.h"
#include "isp600_reg_cfg.h"

#define ISP600_MAX_NUM 4
//#define USE_DEF_PARA

#define addr_base_offset 0x4

int isp_virtual_find_ch[ISP600_MAX_NUM] = {
	0, 1, 2, 3,
};

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
int isp_virtual_find_logic[ISP600_MAX_NUM + 3] = {
       0, 0, 0, 0, 4, 5, 6,
};

int isp_virtual_find_sel[ISP600_MAX_NUM + 3] = {
       0, 0, 0, 0, 1, 2, 3,
};

int isp_ch_find[ISP600_MAX_NUM + 3] = {
       0, 1, 2, 3, 0, 0, 0,
};
#else
int isp_virtual_find_logic[ISP600_MAX_NUM] = {
       0, 0, 0, 0,
};
#endif


/*
 *  Load ISP register variables
 */

struct isp600_reg {
	ISP_TOP_CFG0_REG_t *isp_top_cfg;
	ISP_VER_CFG_REG_t *isp_ver_cfg;
	ISP_MAX_SIZE_REG_t *isp_max_size;
	ISP_MODULE_FET_REG_t *isp_module_fet;

	ISP_AHB_CFG0_REG_t *isp_update_ctrl;
	unsigned int *isp_load_addr;
	ISP_INT_BYPASS_REG_t *isp_int_bypass;
	ISP_INT_STATUS_REG_t *isp_int_status;
	ISP_INT_STATUS0_REG_t *isp_inter_status0;
	ISP_INT_STATUS1_REG_t *isp_inter_status1;
	ISP_INT_STATUS2_REG_t *isp_inter_status2;
	unsigned int *isp_save_addr;

	ISP_AHB_CFG0_REG_t *isp_update_flag;
	ISP_GLOBAL_CFG0_REG_t *isp_global_cfg0;
	ISP_GLOBAL_CFG1_REG_t *isp_global_cfg1;
	ISP_LBC_TIME_CYCLE_REG_t *isp_lbc_time_cycle;
	unsigned int *isp_save_load_addr;
	ISP_INPUT_SIZE_REG_t *isp_input_size;
	ISP_VALID_SIZE_REG_t *isp_valid_size;
	ISP_VALID_START_REG_t *isp_valid_start;
	ISP_MODULE_BYPASS0_REG_t *isp_module_bypass0;
	ISP_MODULE_BYPASS1_REG_t *isp_module_bypass1;
	ISP_MODULE_MODE0_REG_t *isp_module_mode0;
	unsigned int *isp_d3d_bayer_addr;
	unsigned int *isp_d3d_k0_addr;
	unsigned int *isp_d3d_k1_addr;
	unsigned int *isp_d3d_status_addr;
	ISP_VIN_CFG0_REG_t *isp_vin_cfg0;
	ISP_CH0_EXPAND_CFG0_REG_t *isp_ch0_expand_cfg0;
	ISP_CH1_EXPAND_CFG0_REG_t *isp_ch1_expand_cfg0;
	ISP_CH2_EXPAND_CFG0_REG_t *isp_ch2_expand_cfg0;
	ISP_D3D_LBC_CFG0_REG_t *isp_d3d_lbc_cfg0;
	ISP_D3D_LBC_CFG1_REG_t *isp_d3d_lbc_cfg1;
	ISP_D3D_LBC_CFG2_REG_t *isp_d3d_lbc_cfg2;
	ISP_D3D_LBC_CFG3_REG_t *isp_d3d_lbc_cfg3;
	ISP_D3D_LBC_CFG4_REG_t *isp_d3d_lbc_cfg4;
	ISP_D3D_LBC_CFG5_REG_t *isp_d3d_lbc_cfg5;
	ISP_D3D_CFG0_REG_t *isp_d3d_cfg0;

	ISP_RANDOM_SEED_REG_t *isp_random_seed;

	ISP_SRAM_CTRL_REG_t *isp_sram_ctrl;
};

struct isp600_reg isp_regs[ISP600_MAX_NUM];

void bsp_isp_map_reg_addr(unsigned long id, unsigned long base)
{
	base += isp_virtual_find_ch[id] * addr_base_offset;

	isp_regs[id].isp_top_cfg = (ISP_TOP_CFG0_REG_t *) (base + ISP_TOP_CFG0_REG);
	isp_regs[id].isp_ver_cfg = (ISP_VER_CFG_REG_t *) (base + ISP_VER_CFG_REG);
	isp_regs[id].isp_max_size = (ISP_MAX_SIZE_REG_t *) (base + ISP_MAX_WIDTH_REG);
	isp_regs[id].isp_module_fet = (ISP_MODULE_FET_REG_t *) (base + ISP_MODULE_FET_REG);


	isp_regs[id].isp_update_ctrl = (ISP_AHB_CFG0_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_AHB_CFG0_REG);
	isp_regs[id].isp_load_addr = (unsigned int *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_LOAD_ADDR_REG);
	isp_regs[id].isp_int_bypass = (ISP_INT_BYPASS_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_INT_BYPASS_REG);
	isp_regs[id].isp_int_status = (ISP_INT_STATUS_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_INT_STATUS_REG);
	isp_regs[id].isp_inter_status0 = (ISP_INT_STATUS0_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_INTER_STATUS0_REG);
	isp_regs[id].isp_inter_status1 = (ISP_INT_STATUS1_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_INTER_STATUS1_REG);
	isp_regs[id].isp_inter_status2 = (ISP_INT_STATUS2_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_INTER_STATUS2_REG);
	isp_regs[id].isp_save_addr = (unsigned int *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_SAVE_ADDR_REG);
#if !defined CONFIG_ARCH_SUN55IW3
	isp_regs[id].isp_save_load_addr = (unsigned int *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_SAVE_LOAD_ADDR_REG);
#endif

#ifdef USE_DEF_PARA
	isp_regs[id].isp_global_cfg0 = (ISP_GLOBAL_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_GLOBAL_CFG0_REG);
	isp_regs[id].isp_global_cfg1 = (ISP_GLOBAL_CFG1_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_GLOBAL_CFG1_REG);
	isp_regs[id].isp_lbc_time_cycle = (ISP_LBC_TIME_CYCLE_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_LBC_TIME_CYCLE_REG);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	isp_regs[id].isp_save_load_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_SAVE_LOAD_ADDR_REG);
#endif
	isp_regs[id].isp_input_size = (ISP_INPUT_SIZE_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_INPUT_SIZE_REG);
	isp_regs[id].isp_valid_size = (ISP_VALID_SIZE_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_VALID_SIZE_REG);
	isp_regs[id].isp_valid_start = (ISP_VALID_START_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_VALID_START_REG);
	isp_regs[id].isp_module_bypass0 = (ISP_MODULE_BYPASS0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_MODULE_BYPASS0_REG);
	isp_regs[id].isp_module_bypass1 = (ISP_MODULE_BYPASS1_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_MODULE_BYPASS1_REG);
	isp_regs[id].isp_module_mode0 = (ISP_MODULE_MODE0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_MODULE_MODE0_REG);
	isp_regs[id].isp_d3d_bayer_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_BAYER_ADDR_REG);
	isp_regs[id].isp_d3d_k0_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_K0_ADDR_REG);
	isp_regs[id].isp_d3d_k1_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_K1_ADDR_REG);
	isp_regs[id].isp_d3d_status_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_STATUS_ADDR_REG);
	isp_regs[id].isp_vin_cfg0 = (ISP_VIN_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_VIN_CFG0_REG);
	isp_regs[id].isp_ch0_expand_cfg0 = (ISP_CH0_EXPAND_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_CH0_EXPAND_CFG0_REG);
	isp_regs[id].isp_ch1_expand_cfg0 = (ISP_CH1_EXPAND_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_CH1_EXPAND_CFG0_REG);
	isp_regs[id].isp_ch2_expand_cfg0 = (ISP_CH2_EXPAND_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_CH2_EXPAND_CFG0_REG);
	isp_regs[id].isp_d3d_lbc_cfg0 = (ISP_D3D_LBC_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG0_REG);
	isp_regs[id].isp_d3d_lbc_cfg1 = (ISP_D3D_LBC_CFG1_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG1_REG);
	isp_regs[id].isp_d3d_lbc_cfg2 = (ISP_D3D_LBC_CFG2_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG2_REG);
	isp_regs[id].isp_d3d_lbc_cfg3 = (ISP_D3D_LBC_CFG3_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG3_REG);
	isp_regs[id].isp_d3d_lbc_cfg4 = (ISP_D3D_LBC_CFG4_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG4_REG);
	isp_regs[id].isp_d3d_lbc_cfg5 = (ISP_D3D_LBC_CFG5_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG5_REG);
#endif
}

/*
 * Load DRAM Register Address
 */
void bsp_isp_map_load_dram_addr(unsigned long id, unsigned long base)
{
#ifndef USE_DEF_PARA
	isp_regs[id].isp_update_flag = (ISP_AHB_CFG0_REG_t *) (base + ISP_AHB_REG_OFFSET + id * ISP_AHB_AMONG_OFFSET + ISP_AHB_CFG0_REG);
	isp_regs[id].isp_global_cfg0 = (ISP_GLOBAL_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_GLOBAL_CFG0_REG);
	isp_regs[id].isp_global_cfg1 = (ISP_GLOBAL_CFG1_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_GLOBAL_CFG1_REG);
	isp_regs[id].isp_lbc_time_cycle = (ISP_LBC_TIME_CYCLE_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_LBC_TIME_CYCLE_REG);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	isp_regs[id].isp_save_load_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_SAVE_LOAD_ADDR_REG);
#endif
	isp_regs[id].isp_input_size = (ISP_INPUT_SIZE_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_INPUT_SIZE_REG);
	isp_regs[id].isp_valid_size = (ISP_VALID_SIZE_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_VALID_SIZE_REG);
	isp_regs[id].isp_valid_start = (ISP_VALID_START_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_VALID_START_REG);
	isp_regs[id].isp_module_bypass0 = (ISP_MODULE_BYPASS0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_MODULE_BYPASS0_REG);
	isp_regs[id].isp_module_bypass1 = (ISP_MODULE_BYPASS1_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_MODULE_BYPASS1_REG);
	isp_regs[id].isp_module_mode0 = (ISP_MODULE_MODE0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_MODULE_MODE0_REG);
	isp_regs[id].isp_d3d_bayer_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_BAYER_ADDR_REG);
	isp_regs[id].isp_d3d_k0_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_K0_ADDR_REG);
	isp_regs[id].isp_d3d_k1_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_K1_ADDR_REG);
	isp_regs[id].isp_d3d_status_addr = (unsigned int *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_STATUS_ADDR_REG);
	isp_regs[id].isp_vin_cfg0 = (ISP_VIN_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_VIN_CFG0_REG);
	isp_regs[id].isp_ch0_expand_cfg0 = (ISP_CH0_EXPAND_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_CH0_EXPAND_CFG0_REG);
	isp_regs[id].isp_ch1_expand_cfg0 = (ISP_CH1_EXPAND_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_CH1_EXPAND_CFG0_REG);
	isp_regs[id].isp_ch2_expand_cfg0 = (ISP_CH2_EXPAND_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_CH2_EXPAND_CFG0_REG);
	isp_regs[id].isp_d3d_lbc_cfg0 = (ISP_D3D_LBC_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG0_REG);
	isp_regs[id].isp_d3d_lbc_cfg1 = (ISP_D3D_LBC_CFG1_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG1_REG);
	isp_regs[id].isp_d3d_lbc_cfg2 = (ISP_D3D_LBC_CFG2_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG2_REG);
	isp_regs[id].isp_d3d_lbc_cfg3 = (ISP_D3D_LBC_CFG3_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG3_REG);
	isp_regs[id].isp_d3d_lbc_cfg4 = (ISP_D3D_LBC_CFG4_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG4_REG);
	isp_regs[id].isp_d3d_lbc_cfg5 = (ISP_D3D_LBC_CFG5_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_LBC_CFG5_REG);
	isp_regs[id].isp_d3d_cfg0 = (ISP_D3D_CFG0_REG_t *) (base + ISP_LOAD_REG_OFFSET + ISP_D3D_CFG0_REG);
#endif
}

/*
 * save Load DRAM Register Address
 */
void bsp_isp_map_save_load_dram_addr(unsigned long id, unsigned long base)
{
	isp_regs[id].isp_random_seed = (ISP_RANDOM_SEED_REG_t *) (base + ISP_RANDOM_SEED_REG);
}

/*
 * syscfg Register Address
 */
void bsp_isp_map_syscfg_addr(unsigned long id, unsigned long base)
{
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	isp_regs[id].isp_sram_ctrl = (ISP_SRAM_CTRL_REG_t *) (base + 0x4);
#endif
}

void bsp_isp_sram_boot_mode_ctrl(unsigned long id, unsigned int mode)
{
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	isp_regs[id].isp_sram_ctrl->bits.sram_bmode_ctrl = mode;
#endif
}

/*******isp top control register which we can write directly to register*********/

void bsp_isp_enable(unsigned long id, unsigned int en)
{
	isp_regs[id].isp_top_cfg->bits.isp_enable = en;
}

void bsp_isp_mode(unsigned long id, unsigned int mode)
{
	isp_regs[id].isp_top_cfg->bits.isp_mode = mode;
}

void bsp_isp_top_capture_start(unsigned long id)
{
	isp_regs[id].isp_top_cfg->bits.isp_top_cap_en = 1;
}

void bsp_isp_top_capture_stop(unsigned long id)
{
	isp_regs[id].isp_top_cfg->bits.isp_top_cap_en = 0;
}

void bsp_isp_ver_read_en(unsigned long id, unsigned int en)
{
	isp_regs[id].isp_top_cfg->bits.isp_ver_rd_en = en;
}

void bsp_isp_set_sram_clear(unsigned long id, unsigned int en)
{
	isp_regs[id].isp_top_cfg->bits.sram_clear = en;
}

void bsp_isp_set_clk_back_door(unsigned long id, unsigned int en)
{
	isp_regs[id].isp_top_cfg->bits.md_clk_back_door = en;
	isp_regs[id].isp_top_cfg->bits.d2d_clk_back_door = en;
}

unsigned int bsp_isp_get_isp_ver(unsigned long id, unsigned int *major, unsigned int *minor)
{
	*major = isp_regs[id].isp_ver_cfg->bits.big_ver;
	*minor = isp_regs[id].isp_ver_cfg->bits.small_ver;
	return isp_regs[id].isp_ver_cfg->dwval;
}

unsigned int bsp_isp_get_max_width(unsigned long id)
{
	return isp_regs[id].isp_max_size->bits.max_width;
}

/*******ispx ahb control register which we can write directly to register*********/

void bsp_isp_capture_start(unsigned long id)
{
	isp_regs[id].isp_update_ctrl->bits.cap_en = 1;
}

void bsp_isp_capture_stop(unsigned long id)
{
	isp_regs[id].isp_update_ctrl->bits.cap_en = 0;
}

void bsp_isp_set_para_ready(unsigned long id, int ready)
{
#ifndef USE_DEF_PARA
	isp_regs[id].isp_update_ctrl->bits.para_ready = ready;
#endif
}

void bsp_isp_update_table(unsigned long id, unsigned int table_update)
{
	isp_regs[id].isp_update_ctrl->bits.rsc_update = !!(table_update & LENS_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.gamma_update = !!(table_update & GAMMA_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.drc_update = !!(table_update & DRC_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.d3d_update = !!(table_update & D3D_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.pltm_update = !!(table_update & PLTM_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.cem_update = !!(table_update & CEM_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.msc_update = !!(table_update & MSC_UPDATE);
	isp_regs[id].isp_update_ctrl->bits.nr_msc_update = !!(table_update & NR_MSC_UPDATE);
}

void bsp_isp_set_load_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_load_addr);
}

void bsp_isp_irq_enable(unsigned long id, unsigned int irq_flag)
{
	isp_regs[id].isp_int_bypass->dwval |= irq_flag;
}

void bsp_isp_irq_disable(unsigned long id, unsigned int irq_flag)
{
	isp_regs[id].isp_int_bypass->dwval &= ~irq_flag;
}

unsigned int bsp_isp_get_irq_status(unsigned long id, unsigned int flag)
{
	return isp_regs[id].isp_int_status->dwval & flag;
}

void bsp_isp_clr_irq_status(unsigned long id, unsigned int flag)
{
	isp_regs[id].isp_int_status->dwval = flag;
}

unsigned int bsp_isp_get_internal_status0(unsigned long id, unsigned int flag)
{
	return isp_regs[id].isp_inter_status0->dwval & flag;
}

void bsp_isp_clr_internal_status0(unsigned long id, unsigned int flag)
{
	isp_regs[id].isp_inter_status0->dwval = flag;
}

unsigned int bsp_isp_get_internal_status1(unsigned long id, unsigned int flag)
{
	return isp_regs[id].isp_inter_status1->dwval & flag;
}

unsigned int bsp_isp_get_isp_cur_id(unsigned long id)
{
	return (isp_regs[id].isp_inter_status1->dwval & ISP_CUR_ID_MASK) >> ISP_CUR_ID;
}

unsigned int bsp_isp_get_internal_status2(unsigned long id, unsigned int flag)
{
	return isp_regs[id].isp_inter_status2->dwval & flag;
}

void bsp_isp_clr_internal_status2(unsigned long id, unsigned int flag)
{
	isp_regs[id].isp_inter_status2->dwval = flag;
}

void bsp_isp_set_saved_addr(unsigned long id, unsigned long addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_save_addr);
}

void bsp_isp_set_statistics_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_save_addr);
}

void bsp_isp_set_save_load_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_save_load_addr);
}
/*******isp load register which we should write to ddr first*********/

unsigned int bsp_isp_load_update_flag(unsigned long id)
{
#ifndef USE_DEF_PARA
	return isp_regs[id].isp_update_flag->dwval;
#else
	return 0;
#endif
}

void bsp_isp_set_input_fmt(unsigned long id, unsigned int fmt)
{
	isp_regs[id].isp_global_cfg0->bits.input_fmt = fmt;
	isp_regs[id].isp_random_seed->bits.input_fmt = fmt;
}

void bsp_isp_set_byr_max_bit(unsigned long id, int bit)
{
	isp_regs[id].isp_global_cfg0->bits.byr_max_bit = bit;
}

void bsp_isp_set_byr_act_bit(unsigned long id, int bit)
{
	isp_regs[id].isp_global_cfg0->bits.byr_act_bit = bit;
}

void bsp_isp_set_last_blank_cycle(unsigned long id, unsigned int blank)
{
	isp_regs[id].isp_global_cfg1->bits.last_blank_cycle = blank;
}

void bsp_isp_set_speed_mode(unsigned long id, unsigned int speed)
{
	isp_regs[id].isp_global_cfg1->bits.speed_mode = speed;
}

void bsp_isp_set_line_int_num(unsigned long id, unsigned int line_num)
{
	isp_regs[id].isp_global_cfg1->bits.line_int_num = line_num;
}

void bsp_isp_debug_output_cfg(unsigned long id, int enable, int output_sel)
{
	isp_regs[id].isp_global_cfg0->bits.dbg_out_en = enable;
	isp_regs[id].isp_global_cfg1->bits.dbg_out_sel = output_sel;
}

void bsp_isp_set_size(unsigned long id, struct isp_size_settings *size)
{
	isp_regs[id].isp_input_size->bits.input_width = size->ob_black.width;
	isp_regs[id].isp_input_size->bits.input_height = size->ob_black.height;
	isp_regs[id].isp_valid_size->bits.valid_width = size->ob_valid.width;
	isp_regs[id].isp_valid_size->bits.valid_height = size->ob_valid.height;
	isp_regs[id].isp_valid_start->bits.valid_hor_start = size->ob_start.hor;
	isp_regs[id].isp_valid_start->bits.valid_ver_start = size->ob_start.ver;
}

void bsp_isp_module0_enable(unsigned long id, unsigned int module_flag)
{
	isp_regs[id].isp_module_bypass0->dwval |= module_flag;
}

void bsp_isp_module0_disable(unsigned long id, unsigned int module_flag)
{
	isp_regs[id].isp_module_bypass0->dwval &= ~module_flag;
}

void bsp_isp_module_enable(unsigned long id, unsigned int module_flag)
{
	isp_regs[id].isp_module_bypass1->dwval |= module_flag;
}

void bsp_isp_module_disable(unsigned long id, unsigned int module_flag)
{
	isp_regs[id].isp_module_bypass1->dwval &= ~module_flag;
}

void bsp_isp_d3d_ref_frm_mode(unsigned long id, unsigned int d3d_ref)
{
	isp_regs[id].isp_module_mode0->bits.d3d_ref_frm_mode = d3d_ref;
}

void bsp_isp_set_d3d_bayer_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_d3d_bayer_addr);
}

void bsp_isp_set_d3d_k0_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_d3d_k0_addr);
}

void bsp_isp_set_d3d_k1_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_d3d_k1_addr);
}

void bsp_isp_set_d3d_status_addr(unsigned long id, dma_addr_t addr)
{
	writel(addr >> ISP_ADDR_BIT_R_SHIFT, isp_regs[id].isp_d3d_status_addr);
}

void bsp_isp_set_ch_input_bit(unsigned long id, int ch, int bit)
{
	if (ch == 0)
		isp_regs[id].isp_ch0_expand_cfg0->bits.input_bit = bit;
	else if (ch == 1)
		isp_regs[id].isp_ch1_expand_cfg0->bits.input_bit = bit;
	else if (ch == 2)
		isp_regs[id].isp_ch2_expand_cfg0->bits.input_bit = bit;
}

void bsp_isp_set_ch_output_bit(unsigned long id, int ch, int bit)
{
	if (ch == 0)
		isp_regs[id].isp_ch0_expand_cfg0->bits.output_bit = bit;
	else if (ch == 1)
		isp_regs[id].isp_ch1_expand_cfg0->bits.output_bit = bit;
	else if (ch == 2)
		isp_regs[id].isp_ch2_expand_cfg0->bits.output_bit = bit;
}
void bsp_isp_clr_d3d_rec_en(unsigned long id)
{
	isp_regs[id].isp_d3d_cfg0->bits.d3d_rec_en = 0;
}

void bsp_isp_set_d3d_lbc_cfg(unsigned long id, struct isp_lbc_cfg d3d_lbc)
{
	isp_regs[id].isp_d3d_lbc_cfg0->bits.mb_min_bit = d3d_lbc.mb_min_bit;
	isp_regs[id].isp_d3d_lbc_cfg0->bits.mb_num = d3d_lbc.mb_num;

	isp_regs[id].isp_d3d_lbc_cfg1->bits.mb_min_bits_std0 = d3d_lbc.mb_min_bits_std[0];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.mb_min_bits_std1 = d3d_lbc.mb_min_bits_std[1];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.mb_min_bits_std2 = d3d_lbc.mb_min_bits_std[2];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.mb_min_bits_std3 = d3d_lbc.mb_min_bits_std[3];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.start_qp_std0 = d3d_lbc.start_qp_std[0];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.start_qp_std1 = d3d_lbc.start_qp_std[1];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.start_qp_std2 = d3d_lbc.start_qp_std[2];
	isp_regs[id].isp_d3d_lbc_cfg1->bits.start_qp_std3 = d3d_lbc.start_qp_std[3];

	isp_regs[id].isp_d3d_lbc_cfg3->bits.log_en = d3d_lbc.log_en;
	isp_regs[id].isp_d3d_lbc_cfg3->bits.is_lossy = d3d_lbc.is_lossy;

	isp_regs[id].isp_d3d_lbc_cfg4->bits.line_tar_bit = d3d_lbc.line_tar_bits;
	isp_regs[id].isp_d3d_lbc_cfg4->bits.line_max_bit = d3d_lbc.line_max_bits;

	isp_regs[id].isp_d3d_lbc_cfg5->bits.line_stride = d3d_lbc.line_stride;
}

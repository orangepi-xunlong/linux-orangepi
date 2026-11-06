/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: ive_platform.c
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "sys.h"
#include "ion.h"
#include "vi_sys.h"
#include "ive_platform.h"
#include "ive_debug.h"

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/signal.h>
#endif
#define IVE_TOP_16_8_8(val, a, b, c) ((val) | (a << 16) | (b << 8) | (c))
#define TIMEOUT_MS (1)


char IMG_FMT[16][32] = {
	"YUV420 planar",
	"YUV422 planar",
	"RGB888 planar",
	"RGB packed {R,G,B}",
	"RGB packed {B,G,R}",
	"Y only",
	"reserved",
	"reserved",
	"NV12",
	"NV21",
	"YUV422-SP1",
	"YUV422-SP2",
	"YUV2-1 {U,Y,V,Y}",
	"YUV2-2 {V,Y,U,Y}",
	"YUV2-3 {Y,U,Y,V}",
	"YUV2-4 {Y,V,Y,U}"
};

#define IVE_CMDQ 0  //A2 to do

struct _IVE_IP_BLOCK_S IVE_BLK_BA[IVE_DEV_MAX];
struct _IVE_DEBUG_INFO_S g_debug_info = {
	.addr[RDMA_IMG_IN] = {"rdma_img_in", 0, 0, 0},
	.addr[RDMA_IMG1] = {"rdma_img1", 0, 0, 0},
	.addr[RDMA_EIGVAL] = {"rdma_eigval", 0, 0, 0},
	.addr[RDMA_RADFG] = {"rdma_gradfg", 0, 0, 0},
	.addr[RDMA_MM_FACTOR] = {"rdma_gmm_factor", 0, 0, 0},
	.addr[RDMA_MM_MOD] = {"rdma_gmm_mod", 0, 0, 0},
	.addr[RDMA_GMODEL_0] = {"rdma_bgmodel_0", 0, 0, 0},
	.addr[RDMA_GMODEL_1] = {"rdma_bgmodel_1", 0, 0, 0},
	.addr[RDMA_GFLAG] = {"rdma_fgflag", 0, 0, 0},
	.addr[RDMA_DMA] = {"rdma_dma", 0, 0, 0},
	.addr[WDMA_DMA] = {"wdma_dma", 0, 0, 0},
	.addr[WDMA_ODMA] = {"wdma_odma", 0, 0, 0},
	.addr[WDMA_Y] = {"wdma_y", 0, 0, 0},
	.addr[WDMA_C] = {"wdma_c", 0, 0, 0},
	.addr[WDMA_HIST] = {"wdma_hist", 0, 0, 0},
	.addr[WDMA_INTEG] = {"wdma_integ", 0, 0, 0},
	.addr[WDMA_SAD] = {"wdma_sad", 0, 0, 0},
	.addr[WDMA_SAD_THR] = {"wdma_sad_thr", 0, 0, 0},
	.addr[WDMA_GMM_MATCH] = {"wdma_gmm_match", 0, 0, 0},
	.addr[WDMA_GMM_MOD] = {"wdma_gmm_mod", 0, 0, 0},
	.addr[WDMA_CHG] = {"wdma_chg", 0, 0, 0},
	.addr[WDMA_BGMODEL_0] = {"wdma_bgmodel_0", 0, 0, 0},
	.addr[WDMA_BGMODEL_1] = {"wdma_bgmodel_1", 0, 0, 0},
	.addr[WDMA_FG] = {"wdma_fg", 0, 0, 0},
};

ive_dst_mem_info_s *g_pDst;
u16 g_u16MaxEig;
u32 g_u32SizeS8C2;
ive_st_candi_corner_ctrl_s *g_pStCandiCornerCtrl;
uintptr_t g_phy_shift[IVE_DEV_MAX];
bool g_dump_reg_info;
bool g_dump_image_info;
bool g_dump_dma_info;

s32 d_num = 0;  //only for debug device num

ive_top_c ive_top_c_l = _DEFINE_IVE_TOP_C;
ive_filterop_c ive_filterop_c_l = _DEFINE_IVE_FILTEROP_C;

static ive_top_c *init_ive_top_c(void)
{
	ive_top_c *top_c = kzalloc(sizeof(ive_top_c), GFP_ATOMIC);

	if (top_c == NULL) {
		//pr_err("ive_top_c init failed\n");
		return NULL;
	}

	memcpy(top_c, &ive_top_c_l, sizeof(ive_top_c));

	return top_c;
}

static ive_filterop_c *init_ive_filterop_c(void)
{
	ive_filterop_c *filterop_c = kzalloc(sizeof(ive_filterop_c), GFP_ATOMIC);

	if (filterop_c == NULL) {
		//pr_err("ive_filterop_c init failed\n");
		return NULL;
	}

	memcpy(filterop_c, &ive_filterop_c_l, sizeof(ive_filterop_c));

	return filterop_c;

}

// static void ive_dma_printk(ive_dma_c *p)
// {
// 	pr_info("ive_dma\n");
// 	pr_info("\tREG_0.reg_ive_dma_enable = 0x%x\n", p->reg_0.reg_ive_dma_enable);
// 	pr_info("\tREG_0.reg_shdw_sel = 0x%x\n", p->reg_0.reg_shdw_sel);
// 	pr_info("\tREG_0.reg_softrst = 0x%x\n", p->reg_0.reg_softrst);
// 	pr_info("\tREG_0.reg_ive_dma_mode = 0x%x\n", p->reg_0.reg_ive_dma_mode);
// 	pr_info("\tREG_0.reg_force_clk_enable = 0x%x\n", p->reg_0.reg_force_clk_enable);
// 	pr_info("\tREG_0.reg_force_rdma_disable = 0x%x\n", p->reg_0.reg_force_rdma_disable);
// 	pr_info("\tREG_0.reg_force_wdma_disable = 0x%x\n", p->reg_0.reg_force_wdma_disable);
// 	pr_info("\tREG_1.reg_ive_dma_src_stride = 0x%x\n", p->reg_1.reg_ive_dma_src_stride);
// 	pr_info("\tREG_1.reg_ive_dma_dst_stride = 0x%x\n", p->reg_1.reg_ive_dma_dst_stride);
// 	pr_info("\tREG_2.reg_ive_dma_src_mem_addr = 0x%x\n", p->reg_2.reg_ive_dma_src_mem_addr);
// 	pr_info("\tREG_3.reg_ive_dma_dst_mem_addr = 0x%x\n", p->reg_3.reg_ive_dma_dst_mem_addr);
// 	pr_info("\tREG_4.reg_ive_dma_horsegsize = 0x%x\n", p->reg_4.reg_ive_dma_horsegsize);
// 	pr_info("\tREG_4.reg_ive_dma_elemsize = 0x%x\n", p->reg_4.reg_ive_dma_elemsize);
// 	pr_info("\tREG_4.reg_ive_dma_versegrow = 0x%x\n", p->reg_4.reg_ive_dma_versegrow);
// 	pr_info("\tREG_5.reg_ive_dma_u64_val[0] = 0x%x\n", p->reg_5.reg_ive_dma_u64_val[0]);
// 	pr_info("\tREG_5.reg_ive_dma_u64_val[1] = 0x%x\n", p->reg_5.reg_ive_dma_u64_val[1]);
// }

static void ive_filterop_printk(ive_filterop_c *p)
{
	pr_info("ive_filterop\n");
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->reg_1.reg_softrst);
	pr_info("\tREG_1.reg_softrst_wdma_y = 0x%x\n", p->reg_1.reg_softrst_wdma_y);
	pr_info("\tREG_1.reg_softrst_wdma_c = 0x%x\n", p->reg_1.reg_softrst_wdma_c);
	pr_info("\tREG_1.reg_softrst_rdma_gradfg = 0x%x\n", p->reg_1.reg_softrst_rdma_gradfg);
	pr_info("\tREG_1.reg_softrst_op1 = 0x%x\n", p->reg_1.reg_softrst_op1);
	pr_info("\tREG_1.reg_softrst_filter3ch = 0x%x\n", p->reg_1.reg_softrst_filter3ch);
	pr_info("\tREG_1.reg_softrst_st = 0x%x\n", p->reg_1.reg_softrst_st);
	pr_info("\tREG_1.reg_softrst_odma = 0x%x\n", p->reg_1.reg_softrst_odma);
	pr_info("\tREG_H04.reg_gradfg_bggrad_rdma_en = 0x%x\n", p->reg_h04.reg_gradfg_bggrad_rdma_en);
	pr_info("\tREG_H04.reg_gradfg_bggrad_uv_swap = 0x%x\n", p->reg_h04.reg_gradfg_bggrad_uv_swap);
	pr_info("\tREG_2.reg_shdw_sel = 0x%x\n", p->reg_2.reg_shdw_sel);
	pr_info("\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->reg_3.reg_ctrl_dmy1);
	pr_info("\treg_h10.reg_filterop_mode = 0x%x\n", p->reg_h10.reg_filterop_mode);
	pr_info("\treg_h14.reg_filterop_op1_cmd = 0x%x\n", p->reg_h14.reg_filterop_op1_cmd);
	pr_info("\treg_h14.reg_filterop_sw_ovw_op = 0x%x\n", p->reg_h14.reg_filterop_sw_ovw_op);
	pr_info("\treg_h14.reg_filterop_3ch_en = 0x%x\n", p->reg_h14.reg_filterop_3ch_en);
	pr_info("\treg_h14.reg_op_y_wdma_en = 0x%x\n", p->reg_h14.reg_op_y_wdma_en);
	pr_info("\treg_h14.reg_op_c_wdma_en = 0x%x\n", p->reg_h14.reg_op_c_wdma_en);
	pr_info("\treg_h14.reg_op_y_wdma_w1b_en = 0x%x\n", p->reg_h14.reg_op_y_wdma_w1b_en);
	pr_info("\treg_h14.reg_op_c_wdma_w1b_en = 0x%x\n", p->reg_h14.reg_op_c_wdma_w1b_en);
	pr_info("\tREG_4.reg_filterop_h_coef00 = 0x%x\n", p->reg_4.reg_filterop_h_coef00);
	pr_info("\tREG_4.reg_filterop_h_coef01 = 0x%x\n", p->reg_4.reg_filterop_h_coef01);
	pr_info("\tREG_4.reg_filterop_h_coef02 = 0x%x\n", p->reg_4.reg_filterop_h_coef02);
	pr_info("\tREG_4.reg_filterop_h_coef03 = 0x%x\n", p->reg_4.reg_filterop_h_coef03);
	pr_info("\tREG_5.reg_filterop_h_coef04 = 0x%x\n", p->reg_5.reg_filterop_h_coef04);
	pr_info("\tREG_5.reg_filterop_h_coef10 = 0x%x\n", p->reg_5.reg_filterop_h_coef10);
	pr_info("\tREG_5.reg_filterop_h_coef11 = 0x%x\n", p->reg_5.reg_filterop_h_coef11);
	pr_info("\tREG_5.reg_filterop_h_coef12 = 0x%x\n", p->reg_5.reg_filterop_h_coef12);
	pr_info("\tREG_6.reg_filterop_h_coef13 = 0x%x\n", p->reg_6.reg_filterop_h_coef13);
	pr_info("\tREG_6.reg_filterop_h_coef14 = 0x%x\n", p->reg_6.reg_filterop_h_coef14);
	pr_info("\tREG_6.reg_filterop_h_coef20 = 0x%x\n", p->reg_6.reg_filterop_h_coef20);
	pr_info("\tREG_6.reg_filterop_h_coef21 = 0x%x\n", p->reg_6.reg_filterop_h_coef21);
	pr_info("\tREG_7.reg_filterop_h_coef22 = 0x%x\n", p->reg_7.reg_filterop_h_coef22);
	pr_info("\tREG_7.reg_filterop_h_coef23 = 0x%x\n", p->reg_7.reg_filterop_h_coef23);
	pr_info("\tREG_7.reg_filterop_h_coef24 = 0x%x\n", p->reg_7.reg_filterop_h_coef24);
	pr_info("\tREG_7.reg_filterop_h_coef30 = 0x%x\n", p->reg_7.reg_filterop_h_coef30);
	pr_info("\tREG_8.reg_filterop_h_coef31 = 0x%x\n", p->reg_8.reg_filterop_h_coef31);
	pr_info("\tREG_8.reg_filterop_h_coef32 = 0x%x\n", p->reg_8.reg_filterop_h_coef32);
	pr_info("\tREG_8.reg_filterop_h_coef33 = 0x%x\n", p->reg_8.reg_filterop_h_coef33);
	pr_info("\tREG_8.reg_filterop_h_coef34 = 0x%x\n", p->reg_8.reg_filterop_h_coef34);
	pr_info("\tREG_9.reg_filterop_h_coef40 = 0x%x\n", p->reg_9.reg_filterop_h_coef40);
	pr_info("\tREG_9.reg_filterop_h_coef41 = 0x%x\n", p->reg_9.reg_filterop_h_coef41);
	pr_info("\tREG_9.reg_filterop_h_coef42 = 0x%x\n", p->reg_9.reg_filterop_h_coef42);
	pr_info("\tREG_9.reg_filterop_h_coef43 = 0x%x\n", p->reg_9.reg_filterop_h_coef43);
	pr_info("\tREG_10.reg_filterop_h_coef44 = 0x%x\n", p->reg_10.reg_filterop_h_coef44);
	pr_info("\tREG_10.reg_filterop_h_norm = 0x%x\n", p->reg_10.reg_filterop_h_norm);
	pr_info("\tREG_11.reg_filterop_v_coef00 = 0x%x\n", p->reg_11.reg_filterop_v_coef00);
	pr_info("\tREG_11.reg_filterop_v_coef01 = 0x%x\n", p->reg_11.reg_filterop_v_coef01);
	pr_info("\tREG_11.reg_filterop_v_coef02 = 0x%x\n", p->reg_11.reg_filterop_v_coef02);
	pr_info("\tREG_11.reg_filterop_v_coef03 = 0x%x\n", p->reg_11.reg_filterop_v_coef03);
	pr_info("\tREG_12.reg_filterop_v_coef04 = 0x%x\n", p->reg_12.reg_filterop_v_coef04);
	pr_info("\tREG_12.reg_filterop_v_coef10 = 0x%x\n", p->reg_12.reg_filterop_v_coef10);
	pr_info("\tREG_12.reg_filterop_v_coef11 = 0x%x\n", p->reg_12.reg_filterop_v_coef11);
	pr_info("\tREG_12.reg_filterop_v_coef12 = 0x%x\n", p->reg_12.reg_filterop_v_coef12);
	pr_info("\tREG_13.reg_filterop_v_coef13 = 0x%x\n", p->reg_13.reg_filterop_v_coef13);
	pr_info("\tREG_13.reg_filterop_v_coef14 = 0x%x\n", p->reg_13.reg_filterop_v_coef14);
	pr_info("\tREG_13.reg_filterop_v_coef20 = 0x%x\n", p->reg_13.reg_filterop_v_coef20);
	pr_info("\tREG_13.reg_filterop_v_coef21 = 0x%x\n", p->reg_13.reg_filterop_v_coef21);
	pr_info("\tREG_14.reg_filterop_v_coef22 = 0x%x\n", p->reg_14.reg_filterop_v_coef22);
	pr_info("\tREG_14.reg_filterop_v_coef23 = 0x%x\n", p->reg_14.reg_filterop_v_coef23);
	pr_info("\tREG_14.reg_filterop_v_coef24 = 0x%x\n", p->reg_14.reg_filterop_v_coef24);
	pr_info("\tREG_14.reg_filterop_v_coef30 = 0x%x\n", p->reg_14.reg_filterop_v_coef30);
	pr_info("\tREG_15.reg_filterop_v_coef31 = 0x%x\n", p->reg_15.reg_filterop_v_coef31);
	pr_info("\tREG_15.reg_filterop_v_coef32 = 0x%x\n", p->reg_15.reg_filterop_v_coef32);
	pr_info("\tREG_15.reg_filterop_v_coef33 = 0x%x\n", p->reg_15.reg_filterop_v_coef33);
	pr_info("\tREG_15.reg_filterop_v_coef34 = 0x%x\n", p->reg_15.reg_filterop_v_coef34);
	pr_info("\tREG_16.reg_filterop_v_coef40 = 0x%x\n", p->reg_16.reg_filterop_v_coef40);
	pr_info("\tREG_16.reg_filterop_v_coef41 = 0x%x\n", p->reg_16.reg_filterop_v_coef41);
	pr_info("\tREG_16.reg_filterop_v_coef42 = 0x%x\n", p->reg_16.reg_filterop_v_coef42);
	pr_info("\tREG_16.reg_filterop_v_coef43 = 0x%x\n", p->reg_16.reg_filterop_v_coef43);
	pr_info("\tREG_17.reg_filterop_v_coef44 = 0x%x\n", p->reg_17.reg_filterop_v_coef44);
	pr_info("\tREG_17.reg_filterop_v_norm = 0x%x\n", p->reg_17.reg_filterop_v_norm);
	pr_info("\tREG_18.reg_filterop_mode_trans = 0x%x\n", p->reg_18.reg_filterop_mode_trans);
	pr_info("\tREG_18.reg_filterop_order_enmode = 0x%x\n", p->reg_18.reg_filterop_order_enmode);
	pr_info("\tREG_18.reg_filterop_mag_thr = 0x%x\n", p->reg_18.reg_filterop_mag_thr);
	pr_info("\tREG_19.reg_filterop_bernsen_win5x5_en = 0x%x\n", p->reg_19.reg_filterop_bernsen_win5x5_en);
	pr_info("\tREG_19.reg_filterop_bernsen_mode = 0x%x\n", p->reg_19.reg_filterop_bernsen_mode);
	pr_info("\tREG_19.reg_filterop_bernsen_thr = 0x%x\n", p->reg_19.reg_filterop_bernsen_thr);
	pr_info("\tREG_19.reg_filterop_u8ContrastThreshold = 0x%x\n", p->reg_19.reg_filterop_u8ContrastThreshold);
	pr_info("\tREG_20.reg_filterop_lbp_u8bit_thr = 0x%x\n", p->reg_20.reg_filterop_lbp_u8bit_thr);
	pr_info("\tREG_20.reg_filterop_lbp_s8bit_thr = 0x%x\n", p->reg_20.reg_filterop_lbp_s8bit_thr);
	pr_info("\tREG_20.reg_filterop_lbp_enmode = 0x%x\n", p->reg_20.reg_filterop_lbp_enmode);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef00 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef00);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef01 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef01);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef02 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef02);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef03 = 0x%x\n", p->reg_21.reg_filterop_op2_erodila_coef03);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef04 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef04);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef10 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef10);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef11 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef11);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef12 = 0x%x\n", p->reg_22.reg_filterop_op2_erodila_coef12);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef13 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef13);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef14 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef14);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef20 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef20);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef21 = 0x%x\n", p->reg_23.reg_filterop_op2_erodila_coef21);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef22 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef22);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef23 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef23);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef24 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef24);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef30 = 0x%x\n", p->reg_24.reg_filterop_op2_erodila_coef30);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef31 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef31);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef32 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef32);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef33 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef33);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef34 = 0x%x\n", p->reg_25.reg_filterop_op2_erodila_coef34);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef40 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef40);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef41 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef41);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef42 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef42);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef43 = 0x%x\n", p->reg_26.reg_filterop_op2_erodila_coef43);
	pr_info("\tREG_27.reg_filterop_op2_erodila_coef44 = 0x%x\n", p->reg_27.reg_filterop_op2_erodila_coef44);
	pr_info("\tREG_28.reg_filterop_op2_erodila_en = 0x%x\n", p->reg_28.reg_filterop_op2_erodila_en);
	pr_info("\tREG_CSC_DBG_COEFF.reg_csc_dbg_en = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_en);
	pr_info("\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel);
	pr_info("\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff);
	pr_info("\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x);
	pr_info("\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y);
	pr_info("\tREG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = 0x%x\n", p->REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data);
	pr_info("\tREG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = 0x%x\n", p->REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data);
	pr_info("\treg_33.reg_filterop_op2_gradfg_en = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_en);
	pr_info("\treg_33.reg_filterop_op2_gradfg_softrst = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_softrst);
	pr_info("\treg_33.reg_filterop_op2_gradfg_enmode = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_enmode);
	pr_info("\treg_33.reg_filterop_op2_gradfg_edwdark = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_edwdark);
	pr_info("\treg_33.reg_filterop_op2_gradfg_edwfactor = 0x%x\n", p->reg_33.reg_filterop_op2_gradfg_edwfactor);
	pr_info("\treg_34.reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_crlcoefthr);
	pr_info("\treg_34.reg_filterop_op2_gradfg_magcrlthr = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_magcrlthr);
	pr_info("\treg_34.reg_filterop_op2_gradfg_minmagdiff = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_minmagdiff);
	pr_info("\treg_34.reg_filterop_op2_gradfg_noiseval = 0x%x\n", p->reg_34.reg_filterop_op2_gradfg_noiseval);
	pr_info("\treg_110.reg_filterop_map_enmode = 0x%x\n", p->reg_110.reg_filterop_map_enmode);
	pr_info("\treg_110.reg_filterop_norm_out_ctrl = 0x%x\n", p->reg_110.reg_filterop_norm_out_ctrl);
	pr_info("\treg_110.reg_filterop_magang_out_ctrl = 0x%x\n", p->reg_110.reg_filterop_magang_out_ctrl);
	pr_info("\tODMA_REG_00.reg_dma_blen = 0x%x\n", p->odma_reg_00.reg_dma_blen);
	pr_info("\tODMA_REG_00.reg_dma_en = 0x%x\n", p->odma_reg_00.reg_dma_en);
	pr_info("\tODMA_REG_00.reg_fmt_sel = 0x%x\n", p->odma_reg_00.reg_fmt_sel);
	pr_info("\tODMA_REG_00.reg_sc_odma_hflip = 0x%x\n", p->odma_reg_00.reg_sc_odma_hflip);
	pr_info("\tODMA_REG_00.reg_sc_odma_vflip = 0x%x\n", p->odma_reg_00.reg_sc_odma_vflip);
	pr_info("\tODMA_REG_00.reg_sc_422_avg = 0x%x\n", p->odma_reg_00.reg_sc_422_avg);
	pr_info("\tODMA_REG_00.reg_sc_420_avg = 0x%x\n", p->odma_reg_00.reg_sc_420_avg);
	pr_info("\tODMA_REG_00.reg_c_round_mode = 0x%x\n", p->odma_reg_00.reg_c_round_mode);
	pr_info("\tODMA_REG_00.reg_bf16_en = 0x%x\n", p->odma_reg_00.reg_bf16_en);
	pr_info("\tODMA_REG_01.reg_dma_y_base_low_part = 0x%x\n", p->odma_reg_01.reg_dma_y_base_low_part);
	pr_info("\tODMA_REG_02.reg_dma_y_base_high_part = 0x%x\n", p->odma_reg_02.reg_dma_y_base_high_part);
	pr_info("\tODMA_REG_03.reg_dma_u_base_low_part = 0x%x\n", p->odma_reg_03.reg_dma_u_base_low_part);
	pr_info("\tODMA_REG_04.reg_dma_u_base_high_part = 0x%x\n", p->odma_reg_04.reg_dma_u_base_high_part);
	pr_info("\tODMA_REG_05.reg_dma_v_base_low_part = 0x%x\n", p->odma_reg_05.reg_dma_v_base_low_part);
	pr_info("\tODMA_REG_06.reg_dma_v_base_high_part = 0x%x\n", p->odma_reg_06.reg_dma_v_base_high_part);
	pr_info("\tODMA_REG_07.reg_dma_y_pitch = 0x%x\n", p->odma_reg_07.reg_dma_y_pitch);
	pr_info("\tODMA_REG_08.reg_dma_c_pitch = 0x%x\n", p->odma_reg_08.reg_dma_c_pitch);
	pr_info("\tODMA_REG_09.reg_dma_x_str = 0x%x\n", p->odma_reg_09.reg_dma_x_str);
	pr_info("\tODMA_REG_10.reg_dma_y_str = 0x%x\n", p->odma_reg_10.reg_dma_y_str);
	pr_info("\tODMA_REG_11.reg_dma_wd = 0x%x\n", p->odma_reg_11.reg_dma_wd);
	pr_info("\tODMA_REG_12.reg_dma_ht = 0x%x\n", p->odma_reg_12.reg_dma_ht);
	pr_info("\tODMA_REG_13.reg_dma_debug = 0x%x\n", p->odma_reg_13.reg_dma_debug);
	pr_info("\tODMA_REG_14.reg_dma_int_line_target = 0x%x\n", p->odma_reg_14.reg_dma_int_line_target);
	pr_info("\tODMA_REG_14.reg_dma_int_line_target_sel = 0x%x\n", p->odma_reg_14.reg_dma_int_line_target_sel);
	pr_info("\tODMA_REG_15.reg_dma_int_cycle_line_target = 0x%x\n", p->odma_reg_15.reg_dma_int_cycle_line_target);
	pr_info("\tODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x%x\n",
							p->odma_reg_15.reg_dma_int_cycle_line_target_sel);
	pr_info("\tODMA_REG_16.reg_dma_latch_line_cnt = 0x%x\n", p->odma_reg_16.reg_dma_latch_line_cnt);
	pr_info("\tODMA_REG_16.reg_dma_latched_line_cnt = 0x%x\n", p->odma_reg_16.reg_dma_latched_line_cnt);
	pr_info("\tODMA_REG_17.axi_active = 0x%x\n", p->odma_reg_17.axi_active);
	pr_info("\tODMA_REG_17.axi_y_active = 0x%x\n", p->odma_reg_17.axi_y_active);
	pr_info("\tODMA_REG_17.axi_u_active = 0x%x\n", p->odma_reg_17.axi_u_active);
	pr_info("\tODMA_REG_17.axi_v_active = 0x%x\n", p->odma_reg_17.axi_v_active);
	pr_info("\tODMA_REG_17.y_buf_full = 0x%x\n", p->odma_reg_17.y_buf_full);
	pr_info("\tODMA_REG_17.y_buf_empty = 0x%x\n", p->odma_reg_17.y_buf_empty);
	pr_info("\tODMA_REG_17.u_buf_full = 0x%x\n", p->odma_reg_17.u_buf_full);
	pr_info("\tODMA_REG_17.u_buf_empty = 0x%x\n", p->odma_reg_17.u_buf_empty);
	pr_info("\tODMA_REG_17.v_buf_full = 0x%x\n", p->odma_reg_17.v_buf_full);
	pr_info("\tODMA_REG_17.v_buf_empty = 0x%x\n", p->odma_reg_17.v_buf_empty);
	pr_info("\tODMA_REG_17.line_target_hit = 0x%x\n", p->odma_reg_17.line_target_hit);
	pr_info("\tODMA_REG_17.cycle_line_target_hit = 0x%x\n", p->odma_reg_17.cycle_line_target_hit);
	pr_info("\tODMA_REG_17.axi_cmd_cs = 0x%x\n", p->odma_reg_17.axi_cmd_cs);
	pr_info("\tODMA_REG_17.y_line_cnt = 0x%x\n", p->odma_reg_17.y_line_cnt);
	pr_info("\tREG_CANNY_0.reg_canny_lowthr = 0x%x\n", p->reg_canny_0.reg_canny_lowthr);
	pr_info("\tREG_CANNY_0.reg_canny_hithr = 0x%x\n", p->reg_canny_0.reg_canny_hithr);
	pr_info("\tREG_CANNY_1.reg_canny_en = 0x%x\n", p->reg_canny_1.reg_canny_en);
	pr_info("\tREG_CANNY_1.reg_canny_strong_point_cnt_en = 0x%x\n", p->reg_canny_1.reg_canny_strong_point_cnt_en);
	pr_info("\tREG_CANNY_1.reg_canny_non_or_weak = 0x%x\n", p->reg_canny_1.reg_canny_non_or_weak);
	pr_info("\tREG_CANNY_1.reg_canny_strong_point_cnt = 0x%x\n", p->reg_canny_1.reg_canny_strong_point_cnt);
	pr_info("\tREG_CANNY_2.reg_canny_eof = 0x%x\n", p->reg_canny_2.reg_canny_eof);
	pr_info("\tREG_CANNY_3.reg_canny_basel = 0x%x\n", p->reg_canny_3.reg_canny_basel);
	pr_info("\tREG_CANNY_4.reg_canny_baseh = 0x%x\n", p->reg_canny_4.reg_canny_baseh);
	pr_info("\tREG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x%x\n", p->reg_st_candi_0.reg_st_candi_corner_bypass);
	pr_info("\tREG_ST_CANDI_0.reg_st_candi_corner_switch_src = 0x%x\n",
							p->reg_st_candi_0.reg_st_candi_corner_switch_src);
	pr_info("\tREG_ST_EIGVAL_0.reg_st_eigval_max_eigval = 0x%x\n", p->reg_st_eigval_0.reg_st_eigval_max_eigval);
	pr_info("\tREG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0x%x\n", p->reg_st_eigval_0.reg_st_eigval_tile_num);
	pr_info("\tREG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 0x%x\n", p->reg_st_eigval_1.reg_sw_clr_max_eigval);
	pr_info("\treg_h190.reg_filterop_op2_csc_tab_sw_0 = 0x%x\n", p->reg_h190.reg_filterop_op2_csc_tab_sw_0);
	pr_info("\treg_h190.reg_filterop_op2_csc_tab_sw_1 = 0x%x\n", p->reg_h190.reg_filterop_op2_csc_tab_sw_1);
	pr_info("\treg_h194.reg_filterop_op2_csc_tab_sw_update = 0x%x\n",
								p->reg_h194.reg_filterop_op2_csc_tab_sw_update);
	pr_info("\treg_h194.reg_filterop_op2_csc_coeff_sw_update = 0x%x\n",
								p->reg_h194.reg_filterop_op2_csc_coeff_sw_update);
	pr_info("\tREG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n",
								p->reg_csc_coeff_0.reg_filterop_op2_csc_coeff_sw_00);
	pr_info("\tREG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n",
								p->reg_csc_coeff_1.reg_filterop_op2_csc_coeff_sw_01);
	pr_info("\tREG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n",
								p->reg_csc_coeff_2.reg_filterop_op2_csc_coeff_sw_02);
	pr_info("\tREG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n",
								p->reg_csc_coeff_3.reg_filterop_op2_csc_coeff_sw_03);
	pr_info("\tREG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n",
								p->reg_csc_coeff_4.reg_filterop_op2_csc_coeff_sw_04);
	pr_info("\tREG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n",
								p->reg_csc_coeff_5.reg_filterop_op2_csc_coeff_sw_05);
	pr_info("\tREG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n",
								p->reg_csc_coeff_6.reg_filterop_op2_csc_coeff_sw_06);
	pr_info("\tREG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n",
								p->reg_csc_coeff_7.reg_filterop_op2_csc_coeff_sw_07);
	pr_info("\tREG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n",
								p->reg_csc_coeff_8.reg_filterop_op2_csc_coeff_sw_08);
	pr_info("\tREG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n",
								p->reg_csc_coeff_9.reg_filterop_op2_csc_coeff_sw_09);
	pr_info("\tREG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n",
								p->reg_csc_coeff_a.reg_filterop_op2_csc_coeff_sw_10);
	pr_info("\tREG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n",
								p->reg_csc_coeff_b.reg_filterop_op2_csc_coeff_sw_11);
	pr_info("\treg_h1c8.reg_filterop_op2_csc_enmode = 0x%x\n", p->reg_h1c8.reg_filterop_op2_csc_enmode);
	pr_info("\treg_h1c8.reg_filterop_op2_csc_enable = 0x%x\n", p->reg_h1c8.reg_filterop_op2_csc_enable);
	pr_info("\tREG_cropy_s.reg_crop_y_start_x = 0x%x\n", p->reg_cropy_s.reg_crop_y_start_x);
	pr_info("\tREG_cropy_s.reg_crop_y_end_x = 0x%x\n", p->reg_cropy_s.reg_crop_y_end_x);
	pr_info("\tREG_cropy_e.reg_crop_y_start_y = 0x%x\n", p->reg_cropy_e.reg_crop_y_start_y);
	pr_info("\tREG_cropy_e.reg_crop_y_end_y = 0x%x\n", p->reg_cropy_e.reg_crop_y_end_y);
	pr_info("\tREG_cropy_ctl.reg_crop_y_enable = 0x%x\n", p->reg_cropy_ctl.reg_crop_y_enable);
	pr_info("\tREG_cropc_s.reg_crop_c_start_x = 0x%x\n", p->reg_cropc_s.reg_crop_c_start_x);
	pr_info("\tREG_cropc_s.reg_crop_c_end_x = 0x%x\n", p->reg_cropc_s.reg_crop_c_end_x);
	pr_info("\tREG_cropc_e.reg_crop_c_start_y = 0x%x\n", p->reg_cropc_e.reg_crop_c_start_y);
	pr_info("\tREG_cropc_e.reg_crop_c_end_y = 0x%x\n", p->reg_cropc_e.reg_crop_c_end_y);
	pr_info("\tREG_cropc_ctl.reg_crop_c_enable = 0x%x\n", p->reg_cropc_ctl.reg_crop_c_enable);
	pr_info("\tREG_crop_odma_s.reg_crop_odma_start_x = 0x%x\n", p->reg_crop_odma_s.reg_crop_odma_start_x);
	pr_info("\tREG_crop_odma_s.reg_crop_odma_end_x = 0x%x\n", p->reg_crop_odma_s.reg_crop_odma_end_x);
	pr_info("\tREG_crop_odma_e.reg_crop_odma_start_y = 0x%x\n", p->reg_crop_odma_e.reg_crop_odma_start_y);
	pr_info("\tREG_crop_odma_e.reg_crop_odma_end_y = 0x%x\n", p->reg_crop_odma_e.reg_crop_odma_end_y);
	pr_info("\tREG_crop_odma_ctl.reg_crop_odma_enable = 0x%x\n", p->reg_crop_odma_ctl.reg_crop_odma_enable);
}

static void ive_gmm_printk(ive_gmm_c *p)
{
	pr_info("ive_gmm\n");
	pr_info("\treg_gmm_0.reg_gmm_learn_rate = 0x%x\n",
		p->reg_gmm_0.reg_gmm_learn_rate);
	pr_info("\treg_gmm_0.reg_gmm_bg_ratio = 0x%x\n",
		p->reg_gmm_0.reg_gmm_bg_ratio);
	pr_info("\treg_gmm_1.reg_gmm_var_thr = 0x%x\n",
		p->reg_gmm_1.reg_gmm_var_thr);
	pr_info("\treg_gmm_2.reg_gmm_noise_var = 0x%x\n",
		p->reg_gmm_2.reg_gmm_noise_var);
	pr_info("\treg_gmm_3.reg_gmm_max_var = 0x%x\n",
		p->reg_gmm_3.reg_gmm_max_var);
	pr_info("\treg_gmm_4.reg_gmm_min_var = 0x%x\n",
		p->reg_gmm_4.reg_gmm_min_var);
	pr_info("\treg_gmm_5.reg_gmm_init_weight = 0x%x\n",
		p->reg_gmm_5.reg_gmm_init_weight);
	pr_info("\treg_gmm_6.reg_gmm_detect_shadow = 0x%x\n",
		p->reg_gmm_6.reg_gmm_detect_shadow);
	pr_info("\treg_gmm_6.reg_gmm_shadow_thr = 0x%x\n",
		p->reg_gmm_6.reg_gmm_shadow_thr);
	pr_info("\treg_gmm_6.reg_gmm_sns_factor = 0x%x\n",
		p->reg_gmm_6.reg_gmm_sns_factor);
	pr_info("\treg_gmm_7.reg_gmm2_life_update_factor = 0x%x\n",
		p->reg_gmm_7.reg_gmm2_life_update_factor);
	pr_info("\treg_gmm_7.reg_gmm2_var_rate = 0x%x\n",
		p->reg_gmm_7.reg_gmm2_var_rate);
	pr_info("\treg_gmm_8.reg_gmm2_freq_redu_factor = 0x%x\n",
		p->reg_gmm_8.reg_gmm2_freq_redu_factor);
	pr_info("\treg_gmm_8.reg_gmm2_max_var = 0x%x\n",
		p->reg_gmm_8.reg_gmm2_max_var);
	pr_info("\treg_gmm_9.reg_gmm2_min_var = 0x%x\n",
		p->reg_gmm_9.reg_gmm2_min_var);
	pr_info("\treg_gmm_9.reg_gmm2_freq_add_factor = 0x%x\n",
		p->reg_gmm_9.reg_gmm2_freq_add_factor);
	pr_info("\treg_gmm_10.reg_gmm2_freq_init = 0x%x\n",
		p->reg_gmm_10.reg_gmm2_freq_init);
	pr_info("\treg_gmm_10.reg_gmm2_freq_thr = 0x%x\n",
		p->reg_gmm_10.reg_gmm2_freq_thr);
	pr_info("\treg_gmm_11.reg_gmm2_life_thr = 0x%x\n",
		p->reg_gmm_11.reg_gmm2_life_thr);
	pr_info("\treg_gmm_11.reg_gmm2_sns_factor = 0x%x\n",
		p->reg_gmm_11.reg_gmm2_sns_factor);
	pr_info("\treg_gmm_12.reg_gmm2_factor = 0x%x\n",
		p->reg_gmm_12.reg_gmm2_factor);
	pr_info("\treg_gmm_12.reg_gmm2_life_update_factor_mode = 0x%x\n",
		p->reg_gmm_12.reg_gmm2_life_update_factor_mode);
	pr_info("\treg_gmm_12.reg_gmm2_sns_factor_mode = 0x%x\n",
		p->reg_gmm_12.reg_gmm2_sns_factor_mode);
	pr_info("\treg_gmm_13.reg_gmm_gmm2_enable = 0x%x\n",
		p->reg_gmm_13.reg_gmm_gmm2_enable);
	pr_info("\treg_gmm_13.reg_gmm_gmm2_yonly = 0x%x\n",
		p->reg_gmm_13.reg_gmm_gmm2_yonly);
	pr_info("\treg_gmm_13.reg_gmm_gmm2_shdw_sel = 0x%x\n",
		p->reg_gmm_13.reg_gmm_gmm2_shdw_sel);
	pr_info("\treg_gmm_13.reg_force_clk_enable = 0x%x\n",
		p->reg_gmm_13.reg_force_clk_enable);
	pr_info("\treg_gmm_13.reg_gmm_gmm2_model_num = 0x%x\n",
		p->reg_gmm_13.reg_gmm_gmm2_model_num);
	pr_info("\treg_gmm_13.reg_prob_model_sel = 0x%x\n",
		p->reg_gmm_13.reg_prob_model_sel);
	pr_info("\treg_gmm_13.reg_prob_byte_sel = 0x%x\n",
		p->reg_gmm_13.reg_prob_byte_sel);
	pr_info("\treg_gmm_13.reg_prob_bg_sel = 0x%x\n",
		p->reg_gmm_13.reg_prob_bg_sel);
	pr_info("\treg_gmm_13.reg_prob_en = 0x%x\n", p->reg_gmm_13.reg_prob_en);
	pr_info("\treg_gmm_14.reg_prob_line = 0x%x\n",
		p->reg_gmm_14.reg_prob_line);
	pr_info("\treg_gmm_14.reg_prob_pix = 0x%x\n",
		p->reg_gmm_14.reg_prob_pix);
	pr_info("\treg_gmm_14.reg_prob_byte_data = 0x%x\n",
		p->reg_gmm_14.reg_prob_byte_data);
}

// static void ive_map_printk(ive_map_c *p)
// {
// 	pr_info("ive_map\n");
// 	pr_info("\tREG_0.reg_softrst = 0x%x\n", p->reg_0.reg_softrst);
// 	pr_info("\tREG_0.reg_ip_enable = 0x%x\n", p->reg_0.reg_ip_enable);
// 	pr_info("\tREG_0.reg_ck_enable = 0x%x\n", p->reg_0.reg_ck_enable);
// 	pr_info("\tREG_0.reg_shdw_sel = 0x%x\n", p->reg_0.reg_shdw_sel);
// 	pr_info("\tREG_0.reg_prog_hdk_dis = 0x%x\n", p->reg_0.reg_prog_hdk_dis);
// 	pr_info("\tREG_1.reg_lut_prog_en = 0x%x\n", p->reg_1.reg_lut_prog_en);
// 	pr_info("\tREG_1.reg_lut_wsel = 0x%x\n", p->reg_1.reg_lut_wsel);
// 	pr_info("\tREG_1.reg_lut_rsel = 0x%x\n", p->reg_1.reg_lut_rsel);
// 	pr_info("\tREG_1.reg_sw_lut_rsel = 0x%x\n", p->reg_1.reg_sw_lut_rsel);
// 	pr_info("\tREG_2.reg_lut_st_addr = 0x%x\n", p->reg_2.reg_lut_st_addr);
// 	pr_info("\tREG_2.reg_lut_st_w1t = 0x%x\n", p->reg_2.reg_lut_st_w1t);
// 	pr_info("\tREG_3.reg_lut_wdata = 0x%x\n", p->reg_3.reg_lut_wdata);
// 	pr_info("\tREG_3.reg_lut_w1t = 0x%x\n", p->reg_3.reg_lut_w1t);
// 	pr_info("\tREG_4.reg_sw_lut_raddr = 0x%x\n", p->reg_4.reg_sw_lut_raddr);
// 	pr_info("\tREG_4.reg_sw_lut_r_w1t = 0x%x\n", p->reg_4.reg_sw_lut_r_w1t);
// 	pr_info("\tREG_5.reg_sw_lut_rdata = 0x%x\n", p->reg_5.reg_sw_lut_rdata);
// }

static void ive_match_bg_printk(ive_match_bg_c *p)
{
	pr_info("ive_match_bg\n");
	pr_info("\tREG_00.reg_matchbg_en = 0x%x\n", p->reg_00.reg_matchbg_en);
	pr_info("\tREG_00.reg_matchbg_bypass_model = 0x%x\n",
		p->reg_00.reg_matchbg_bypass_model);
	pr_info("\tREG_00.reg_matchbg_softrst = 0x%x\n",
		p->reg_00.reg_matchbg_softrst);
	pr_info("\tREG_04.reg_matchbg_curfrmnum = 0x%x\n",
		p->reg_04.reg_matchbg_curfrmnum);
	pr_info("\tREG_08.reg_matchbg_prefrmnum = 0x%x\n",
		p->reg_08.reg_matchbg_prefrmnum);
	pr_info("\tREG_0C.reg_matchbg_timethr = 0x%x\n",
		p->reg_0c.reg_matchbg_timethr);
	pr_info("\tREG_0C.reg_matchbg_diffthrcrlcoef = 0x%x\n",
		p->reg_0c.reg_matchbg_diffthrcrlcoef);
	pr_info("\tREG_0C.reg_matchbg_diffmaxthr = 0x%x\n",
		p->reg_0c.reg_matchbg_diffmaxthr);
	pr_info("\tREG_0C.reg_matchbg_diffminthr = 0x%x\n",
		p->reg_0c.reg_matchbg_diffminthr);
	pr_info("\tREG_0C.reg_matchbg_diffthrinc = 0x%x\n",
		p->reg_0c.reg_matchbg_diffthrinc);
	pr_info("\tREG_0C.reg_matchbg_fastlearnrate = 0x%x\n",
		p->reg_0c.reg_matchbg_fastlearnrate);
	pr_info("\tREG_0C.reg_matchbg_detchgregion = 0x%x\n",
		p->reg_0c.reg_matchbg_detchgregion);
	pr_info("\tREG_10.reg_matchbg_stat_pixnum = 0x%x\n",
		p->reg_10.reg_matchbg_stat_pixnum);
	pr_info("\tREG_14.reg_matchbg_stat_sumlum = 0x%x\n",
		p->reg_14.reg_matchbg_stat_sumlum);
}

// static void ive_sad_printk(ive_sad_c *p)
// {
// 	pr_info("ive_sad\n");
// 	pr_info("\treg_sad_00.reg_sad_enmode = 0x%x\n", p->reg_sad_00.reg_sad_enmode);
// 	pr_info("\treg_sad_00.reg_sad_out_ctrl = 0x%x\n", p->reg_sad_00.reg_sad_out_ctrl);
// 	pr_info("\treg_sad_00.reg_sad_u16bit_thr = 0x%x\n", p->reg_sad_00.reg_sad_u16bit_thr);
// 	pr_info("\treg_sad_00.reg_sad_shdw_sel = 0x%x\n", p->reg_sad_00.reg_sad_shdw_sel);
// 	pr_info("\treg_sad_01.reg_sad_u8bit_max = 0x%x\n", p->reg_sad_01.reg_sad_u8bit_max);
// 	pr_info("\treg_sad_01.reg_sad_u8bit_min = 0x%x\n", p->reg_sad_01.reg_sad_u8bit_min);
// 	pr_info("\treg_sad_02.reg_sad_enable = 0x%x\n", p->reg_sad_02.reg_sad_enable);
// 	pr_info("\treg_sad_03.reg_force_clk_enable = 0x%x\n", p->reg_sad_03.reg_force_clk_enable);
// 	pr_info("\treg_sad_04.reg_prob_grid_v = 0x%x\n", p->reg_sad_04.reg_prob_grid_v);
// 	pr_info("\treg_sad_04.reg_prob_grid_h = 0x%x\n", p->reg_sad_04.reg_prob_grid_h);
// 	pr_info("\treg_sad_04.reg_prob_pix_v = 0x%x\n", p->reg_sad_04.reg_prob_pix_v);
// 	pr_info("\treg_sad_04.reg_prob_pix_h = 0x%x\n", p->reg_sad_04.reg_prob_pix_h);
// 	pr_info("\treg_sad_05.reg_prob_prev_sum = 0x%x\n", p->reg_sad_05.reg_prob_prev_sum);
// 	pr_info("\treg_sad_05.reg_prob_curr_pix_0 = 0x%x\n", p->reg_sad_05.reg_prob_curr_pix_0);
// 	pr_info("\treg_sad_05.reg_prob_curr_pix_1 = 0x%x\n", p->reg_sad_05.reg_prob_curr_pix_1);
// 	pr_info("\treg_sad_06.reg_prob_en = 0x%x\n", p->reg_sad_06.reg_prob_en);
// }

static void ive_top_printk(ive_top_c *p)
{
	pr_info("ive_top\n");
	pr_info("\tREG_0.reg_img_in_uv_swap = 0x%x\n", p->reg_0.reg_img_in_uv_swap);
	pr_info("\tREG_0.reg_img_1_uv_swap = 0x%x\n", p->reg_0.reg_img_1_uv_swap);
	pr_info("\tREG_0.reg_rdma_eigval_uv_swap = 0x%x\n", p->reg_0.reg_rdma_eigval_uv_swap);
	pr_info("\tREG_0.reg_trig_cnt = 0x%x\n", p->reg_0.reg_trig_cnt);
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->reg_1.reg_softrst);
	pr_info("\tREG_1.reg_shdw_sel = 0x%x\n", p->reg_1.reg_shdw_sel);
	pr_info("\tREG_1.reg_fmt_vld_fg = 0x%x\n", p->reg_1.reg_fmt_vld_fg);
	pr_info("\tREG_1.reg_fmt_vld_ccl = 0x%x\n", p->reg_1.reg_fmt_vld_ccl);
	pr_info("\tREG_1.reg_fmt_vld_dmaf = 0x%x\n", p->reg_1.reg_fmt_vld_dmaf);
	pr_info("\tREG_1.reg_fmt_vld_lk = 0x%x\n", p->reg_1.reg_fmt_vld_lk);
	pr_info("\tREG_1.reg_cmdq_tsk_trig = 0x%x\n", p->reg_1.reg_cmdq_tsk_trig);
	pr_info("\tREG_1.reg_cmdq_tsk_sel = 0x%x\n", p->reg_1.reg_cmdq_tsk_sel);
	pr_info("\tREG_1.reg_cmdq_tsk_en = 0x%x\n", p->reg_1.reg_cmdq_tsk_en);
	pr_info("\tREG_1.reg_dma_abort = 0x%x\n", p->reg_1.reg_dma_abort);
	pr_info("\tREG_1.reg_wdma_abort_done = 0x%x\n", p->reg_1.reg_wdma_abort_done);
	pr_info("\tREG_1.reg_rdma_abort_done = 0x%x\n", p->reg_1.reg_rdma_abort_done);
	pr_info("\tREG_1.reg_img_in_axi_idle = 0x%x\n", p->reg_1.reg_img_in_axi_idle);
	pr_info("\tREG_1.reg_odma_axi_idle = 0x%x\n", p->reg_1.reg_odma_axi_idle);
	pr_info("\tREG_2.reg_img_widthm1 = 0x%x\n", p->reg_2.reg_img_widthm1);
	pr_info("\tREG_2.reg_img_heightm1 = 0x%x\n", p->reg_2.reg_img_heightm1);
	pr_info("\tREG_3.reg_imgmux_img0_sel = 0x%x\n", p->reg_3.reg_imgmux_img0_sel);
	pr_info("\tREG_3.reg_mapmux_rdma_sel = 0x%x\n", p->reg_3.reg_mapmux_rdma_sel);
	pr_info("\tREG_3.reg_ive_rdma_img1_en = 0x%x\n", p->reg_3.reg_ive_rdma_img1_en);
	pr_info("\tREG_3.reg_ive_rdma_img1_mod_u8 = 0x%x\n", p->reg_3.reg_ive_rdma_img1_mod_u8);
	pr_info("\tREG_3.reg_ive_rdma_eigval_en = 0x%x\n", p->reg_3.reg_ive_rdma_eigval_en);
	pr_info("\tREG_3.reg_muxsel_gradfg = 0x%x\n", p->reg_3.reg_muxsel_gradfg);
	pr_info("\tREG_3.reg_dma_share_mux_selgmm = 0x%x\n", p->reg_3.reg_dma_share_mux_selgmm);
	pr_info("\treg_h10.reg_img_in_top_enable = 0x%x\n", p->reg_h10.reg_img_in_top_enable);
	pr_info("\treg_h10.reg_resize_top_enable = 0x%x\n", p->reg_h10.reg_resize_top_enable);
	pr_info("\treg_h10.reg_gmm_top_enable = 0x%x\n", p->reg_h10.reg_gmm_top_enable);
	pr_info("\treg_h10.reg_csc_top_enable = 0x%x\n", p->reg_h10.reg_csc_top_enable);
	pr_info("\treg_h10.reg_rdma_img1_top_enable = 0x%x\n", p->reg_h10.reg_rdma_img1_top_enable);
	pr_info("\treg_h10.reg_bgm_top_enable = 0x%x\n", p->reg_h10.reg_bgm_top_enable);
	pr_info("\treg_h10.reg_bgu_top_enable = 0x%x\n", p->reg_h10.reg_bgu_top_enable);
	pr_info("\treg_h10.reg_r2y4_top_enable = 0x%x\n", p->reg_h10.reg_r2y4_top_enable);
	pr_info("\treg_h10.reg_map_top_enable = 0x%x\n", p->reg_h10.reg_map_top_enable);
	pr_info("\treg_h10.reg_rdma_eigval_top_enable = 0x%x\n", p->reg_h10.reg_rdma_eigval_top_enable);
	pr_info("\treg_h10.reg_thresh_top_enable = 0x%x\n", p->reg_h10.reg_thresh_top_enable);
	pr_info("\treg_h10.reg_hist_top_enable = 0x%x\n", p->reg_h10.reg_hist_top_enable);
	pr_info("\treg_h10.reg_intg_top_enable = 0x%x\n", p->reg_h10.reg_intg_top_enable);
	pr_info("\treg_h10.reg_ncc_top_enable = 0x%x\n", p->reg_h10.reg_ncc_top_enable);
	pr_info("\treg_h10.reg_sad_top_enable = 0x%x\n", p->reg_h10.reg_sad_top_enable);
	pr_info("\treg_h10.reg_filterop_top_enable = 0x%x\n", p->reg_h10.reg_filterop_top_enable);
	pr_info("\treg_h10.reg_dmaf_top_enable = 0x%x\n", p->reg_h10.reg_dmaf_top_enable);
	pr_info("\treg_h10.reg_ccl_top_enable = 0x%x\n", p->reg_h10.reg_ccl_top_enable);
	pr_info("\treg_h10.reg_lk_top_enable = 0x%x\n", p->reg_h10.reg_lk_top_enable);
	pr_info("\tREG_11.reg_csc_tab_sw_0 = 0x%x\n", p->reg_11.reg_csc_tab_sw_0);
	pr_info("\tREG_11.reg_csc_tab_sw_1 = 0x%x\n", p->reg_11.reg_csc_tab_sw_1);
	pr_info("\tREG_12.reg_csc_tab_sw_update = 0x%x\n", p->reg_12.reg_csc_tab_sw_update);
	pr_info("\tREG_12.reg_csc_coeff_sw_update = 0x%x\n", p->reg_12.reg_csc_coeff_sw_update);
	pr_info("\tREG_CSC_COEFF_0.reg_csc_coeff_sw_00 = 0x%x\n", p->reg_csc_coeff_0.reg_csc_coeff_sw_00);
	pr_info("\tREG_CSC_COEFF_1.reg_csc_coeff_sw_01 = 0x%x\n", p->reg_csc_coeff_1.reg_csc_coeff_sw_01);
	pr_info("\tREG_CSC_COEFF_2.reg_csc_coeff_sw_02 = 0x%x\n", p->reg_csc_coeff_2.reg_csc_coeff_sw_02);
	pr_info("\tREG_CSC_COEFF_3.reg_csc_coeff_sw_03 = 0x%x\n", p->reg_csc_coeff_3.reg_csc_coeff_sw_03);
	pr_info("\tREG_CSC_COEFF_4.reg_csc_coeff_sw_04 = 0x%x\n", p->reg_csc_coeff_4.reg_csc_coeff_sw_04);
	pr_info("\tREG_CSC_COEFF_5.reg_csc_coeff_sw_05 = 0x%x\n", p->reg_csc_coeff_5.reg_csc_coeff_sw_05);
	pr_info("\tREG_CSC_COEFF_6.reg_csc_coeff_sw_06 = 0x%x\n", p->reg_csc_coeff_6.reg_csc_coeff_sw_06);
	pr_info("\tREG_CSC_COEFF_7.reg_csc_coeff_sw_07 = 0x%x\n", p->reg_csc_coeff_7.reg_csc_coeff_sw_07);
	pr_info("\tREG_CSC_COEFF_8.reg_csc_coeff_sw_08 = 0x%x\n", p->reg_csc_coeff_8.reg_csc_coeff_sw_08);
	pr_info("\tREG_CSC_COEFF_9.reg_csc_coeff_sw_09 = 0x%x\n", p->reg_csc_coeff_9.reg_csc_coeff_sw_09);
	pr_info("\tREG_CSC_COEFF_A.reg_csc_coeff_sw_10 = 0x%x\n", p->reg_csc_coeff_a.reg_csc_coeff_sw_10);
	pr_info("\tREG_CSC_COEFF_B.reg_csc_coeff_sw_11 = 0x%x\n", p->reg_csc_coeff_b.reg_csc_coeff_sw_11);
	pr_info("\tREG_14.reg_csc_enmode = 0x%x\n", p->reg_14.reg_csc_enmode);
	pr_info("\tREG_14.reg_csc_enable = 0x%x\n", p->reg_14.reg_csc_enable);
	pr_info("\tREG_15.reg_lbp_u8bit_thr = 0x%x\n", p->reg_15.reg_lbp_u8bit_thr);
	pr_info("\tREG_15.reg_lbp_s8bit_thr = 0x%x\n", p->reg_15.reg_lbp_s8bit_thr);
	pr_info("\tREG_15.reg_lbp_enmode = 0x%x\n", p->reg_15.reg_lbp_enmode);
	pr_info("\treg_h54.reg_ive_dma_idle = 0x%x\n", p->reg_h54.reg_ive_dma_idle);
	pr_info("\treg_h58.reg_ive_gmm_dma_idle = 0x%x\n", p->reg_h58.reg_ive_gmm_dma_idle);
	pr_info("\tREG_16.reg_dbg_en = 0x%x\n", p->reg_16.reg_dbg_en);
	pr_info("\tREG_16.reg_dbg_sel = 0x%x\n", p->reg_16.reg_dbg_sel);
	pr_info("\treg_h64.reg_dbg_col = 0x%x\n", p->reg_h64.reg_dbg_col);
	pr_info("\treg_h64.reg_dbg_row = 0x%x\n", p->reg_h64.reg_dbg_row);
	pr_info("\treg_h68.reg_dbg_status = 0x%x\n", p->reg_h68.reg_dbg_status);
	pr_info("\treg_h6c.reg_dbg_pix = 0x%x\n", p->reg_h6c.reg_dbg_pix);
	pr_info("\treg_h6c.reg_dbg_line = 0x%x\n", p->reg_h6c.reg_dbg_line);
	pr_info("\treg_h70.reg_dbg_data = 0x%x\n", p->reg_h70.reg_dbg_data);
	pr_info("\treg_h74.reg_dbg_perfmt = 0x%x\n", p->reg_h74.reg_dbg_perfmt);
	pr_info("\treg_h74.reg_dbg_fmt = 0x%x\n", p->reg_h74.reg_dbg_fmt);
	pr_info("\tREG_20.reg_frame2op_op_mode = 0x%x\n", p->reg_20.reg_frame2op_op_mode);
	pr_info("\tREG_20.reg_frame2op_sub_mode = 0x%x\n", p->reg_20.reg_frame2op_sub_mode);
	pr_info("\tREG_20.reg_frame2op_sub_change_order = 0x%x\n", p->reg_20.reg_frame2op_sub_change_order);
	pr_info("\tREG_20.reg_frame2op_add_mode_rounding = 0x%x\n", p->reg_20.reg_frame2op_add_mode_rounding);
	pr_info("\tREG_20.reg_frame2op_add_mode_clipping = 0x%x\n", p->reg_20.reg_frame2op_add_mode_clipping);
	pr_info("\tREG_20.reg_frame2op_sub_switch_src = 0x%x\n", p->reg_20.reg_frame2op_sub_switch_src);
	pr_info("\tREG_21.reg_fram2op_x_u0q16 = 0x%x\n", p->reg_21.reg_fram2op_x_u0q16);
	pr_info("\tREG_21.reg_fram2op_y_u0q16 = 0x%x\n", p->reg_21.reg_fram2op_y_u0q16);
	pr_info("\treg_h80.reg_frame2op_fg_op_mode = 0x%x\n", p->reg_h80.reg_frame2op_fg_op_mode);
	pr_info("\treg_h80.reg_frame2op_fg_sub_mode = 0x%x\n", p->reg_h80.reg_frame2op_fg_sub_mode);
	pr_info("\treg_h80.reg_frame2op_fg_sub_change_order = 0x%x\n", p->reg_h80.reg_frame2op_fg_sub_change_order);
	pr_info("\treg_h80.reg_frame2op_fg_add_mode_rounding = 0x%x\n", p->reg_h80.reg_frame2op_fg_add_mode_rounding);
	pr_info("\treg_h80.reg_frame2op_fg_add_mode_clipping = 0x%x\n", p->reg_h80.reg_frame2op_fg_add_mode_clipping);
	pr_info("\treg_h80.reg_frame2op_fg_sub_switch_src = 0x%x\n", p->reg_h80.reg_frame2op_fg_sub_switch_src);
	pr_info("\tREG_84.reg_fram2op_fg_x_u0q16 = 0x%x\n", p->reg_84.reg_fram2op_fg_x_u0q16);
	pr_info("\tREG_84.reg_fram2op_fg_y_u0q16 = 0x%x\n", p->reg_84.reg_fram2op_fg_y_u0q16);
	pr_info("\tREG_90.reg_frame_done_img_in = 0x%x\n", p->reg_90.reg_frame_done_img_in);
	pr_info("\tREG_90.reg_frame_done_rdma_img1 = 0x%x\n", p->reg_90.reg_frame_done_rdma_img1);
	pr_info("\tREG_90.reg_frame_done_rdma_eigval = 0x%x\n", p->reg_90.reg_frame_done_rdma_eigval);
	pr_info("\tREG_90.reg_frame_done_resize = 0x%x\n", p->reg_90.reg_frame_done_resize);
	pr_info("\tREG_90.reg_frame_done_gmm = 0x%x\n", p->reg_90.reg_frame_done_gmm);
	pr_info("\tREG_90.reg_frame_done_csc = 0x%x\n", p->reg_90.reg_frame_done_csc);
	pr_info("\tREG_90.reg_frame_done_hist = 0x%x\n", p->reg_90.reg_frame_done_hist);
	pr_info("\tREG_90.reg_frame_done_intg = 0x%x\n", p->reg_90.reg_frame_done_intg);
	pr_info("\tREG_90.reg_frame_done_sad = 0x%x\n", p->reg_90.reg_frame_done_sad);
	pr_info("\tREG_90.reg_frame_done_ncc = 0x%x\n", p->reg_90.reg_frame_done_ncc);
	pr_info("\tREG_90.reg_frame_done_bgm = 0x%x\n", p->reg_90.reg_frame_done_bgm);
	pr_info("\tREG_90.reg_frame_done_bgu = 0x%x\n", p->reg_90.reg_frame_done_bgu);
	pr_info("\tREG_90.reg_frame_done_r2y4 = 0x%x\n", p->reg_90.reg_frame_done_r2y4);
	pr_info("\tREG_90.reg_frame_done_frame2op_bg = 0x%x\n", p->reg_90.reg_frame_done_frame2op_bg);
	pr_info("\tREG_90.reg_frame_done_frame2op_fg = 0x%x\n", p->reg_90.reg_frame_done_frame2op_fg);
	pr_info("\tREG_90.reg_frame_done_map = 0x%x\n", p->reg_90.reg_frame_done_map);
	pr_info("\tREG_90.reg_frame_done_thresh16ro8 = 0x%x\n", p->reg_90.reg_frame_done_thresh16ro8);
	pr_info("\tREG_90.reg_frame_done_thresh = 0x%x\n", p->reg_90.reg_frame_done_thresh);
	pr_info("\tREG_90.reg_frame_done_filterop_odma = 0x%x\n", p->reg_90.reg_frame_done_filterop_odma);
	pr_info("\tREG_90.reg_frame_done_filterop_wdma_y = 0x%x\n", p->reg_90.reg_frame_done_filterop_wdma_y);
	pr_info("\tREG_90.reg_frame_done_filterop_wdma_c = 0x%x\n", p->reg_90.reg_frame_done_filterop_wdma_c);
	pr_info("\tREG_90.reg_frame_done_dmaf = 0x%x\n", p->reg_90.reg_frame_done_dmaf);
	pr_info("\tREG_90.reg_frame_done_ccl = 0x%x\n", p->reg_90.reg_frame_done_ccl);
	pr_info("\tREG_90.reg_frame_done_lk = 0x%x\n", p->reg_90.reg_frame_done_lk);
	pr_info("\tREG_90.reg_frame_done_filterop_wdma_yc = 0x%x\n", p->reg_90.reg_frame_done_filterop_wdma_yc);
	pr_info("\tREG_94.reg_intr_en_hist = 0x%x\n", p->reg_94.reg_intr_en_hist);
	pr_info("\tREG_94.reg_intr_en_intg = 0x%x\n", p->reg_94.reg_intr_en_intg);
	pr_info("\tREG_94.reg_intr_en_sad = 0x%x\n", p->reg_94.reg_intr_en_sad);
	pr_info("\tREG_94.reg_intr_en_ncc = 0x%x\n", p->reg_94.reg_intr_en_ncc);
	pr_info("\tREG_94.reg_intr_en_filterop_odma = 0x%x\n", p->reg_94.reg_intr_en_filterop_odma);
	pr_info("\tREG_94.reg_intr_en_filterop_wdma_y = 0x%x\n", p->reg_94.reg_intr_en_filterop_wdma_y);
	pr_info("\tREG_94.reg_intr_en_filterop_wdma_c = 0x%x\n", p->reg_94.reg_intr_en_filterop_wdma_c);
	pr_info("\tREG_94.reg_intr_en_dmaf = 0x%x\n", p->reg_94.reg_intr_en_dmaf);
	pr_info("\tREG_94.reg_intr_en_ccl = 0x%x\n", p->reg_94.reg_intr_en_ccl);
	pr_info("\tREG_94.reg_intr_en_lk = 0x%x\n", p->reg_94.reg_intr_en_lk);
	pr_info("\tREG_94.reg_intr_en_filterop_wdma_yc = 0x%x\n", p->reg_94.reg_intr_en_filterop_wdma_yc);
	pr_info("\tREG_98.reg_intr_status_hist = 0x%x\n", p->reg_98.reg_intr_status_hist);
	pr_info("\tREG_98.reg_intr_status_intg = 0x%x\n", p->reg_98.reg_intr_status_intg);
	pr_info("\tREG_98.reg_intr_status_sad = 0x%x\n", p->reg_98.reg_intr_status_sad);
	pr_info("\tREG_98.reg_intr_status_ncc = 0x%x\n", p->reg_98.reg_intr_status_ncc);
	pr_info("\tREG_98.reg_intr_status_filterop_odma = 0x%x\n", p->reg_98.reg_intr_status_filterop_odma);
	pr_info("\tREG_98.reg_intr_status_filterop_wdma_y = 0x%x\n", p->reg_98.reg_intr_status_filterop_wdma_y);
	pr_info("\tREG_98.reg_intr_status_filterop_wdma_c = 0x%x\n", p->reg_98.reg_intr_status_filterop_wdma_c);
	pr_info("\tREG_98.reg_intr_status_dmaf = 0x%x\n", p->reg_98.reg_intr_status_dmaf);
	pr_info("\tREG_98.reg_intr_status_ccl = 0x%x\n", p->reg_98.reg_intr_status_ccl);
	pr_info("\tREG_98.reg_intr_status_lk = 0x%x\n", p->reg_98.reg_intr_status_lk);
	pr_info("\tREG_98.reg_intr_status_filterop_wdma_yc = 0x%x\n", p->reg_98.reg_intr_status_filterop_wdma_yc);
	pr_info("\tREG_RS_SRC_SIZE.reg_resize_src_wd = 0x%x\n", p->reg_rs_src_size.reg_resize_src_wd);
	pr_info("\tREG_RS_SRC_SIZE.reg_resize_src_ht = 0x%x\n", p->reg_rs_src_size.reg_resize_src_ht);
	pr_info("\tREG_RS_DST_SIZE.reg_resize_dst_wd = 0x%x\n", p->reg_rs_dst_size.reg_resize_dst_wd);
	pr_info("\tREG_RS_DST_SIZE.reg_resize_dst_ht = 0x%x\n", p->reg_rs_dst_size.reg_resize_dst_ht);
	pr_info("\tREG_RS_H_SC.reg_resize_h_sc_fac = 0x%x\n", p->reg_rs_h_sc.reg_resize_h_sc_fac);
	pr_info("\tREG_RS_V_SC.reg_resize_v_sc_fac = 0x%x\n", p->reg_rs_v_sc.reg_resize_v_sc_fac);
	pr_info("\tREG_RS_PH_INI.reg_resize_h_ini_ph = 0x%x\n", p->reg_rs_ph_ini.reg_resize_h_ini_ph);
	pr_info("\tREG_RS_PH_INI.reg_resize_v_ini_ph = 0x%x\n", p->reg_rs_ph_ini.reg_resize_v_ini_ph);
	pr_info("\tREG_RS_NOR.reg_resize_h_nor = 0x%x\n", p->reg_rs_nor.reg_resize_h_nor);
	pr_info("\tREG_RS_NOR.reg_resize_v_nor = 0x%x\n", p->reg_rs_nor.reg_resize_v_nor);
	pr_info("\tREG_RS_CTRL.reg_resize_ip_en = 0x%x\n", p->reg_rs_ctrl.reg_resize_ip_en);
	pr_info("\tREG_RS_CTRL.reg_resize_dbg_en = 0x%x\n", p->reg_rs_ctrl.reg_resize_dbg_en);
	pr_info("\tREG_RS_CTRL.reg_resize_area_fast = 0x%x\n", p->reg_rs_ctrl.reg_resize_area_fast);
	pr_info("\tREG_RS_CTRL.reg_resize_blnr_mode = 0x%x\n", p->reg_rs_ctrl.reg_resize_blnr_mode);
	pr_info("\tREG_RS_dBG_H1.reg_resize_sc_dbg_h1 = 0x%x\n", p->reg_rs_dbg_h1.reg_resize_sc_dbg_h1);
	pr_info("\tREG_RS_dBG_H2.reg_resize_sc_dbg_h2 = 0x%x\n", p->reg_rs_dbg_h2.reg_resize_sc_dbg_h2);
	pr_info("\tREG_RS_dBG_V1.reg_resize_sc_dbg_v1 = 0x%x\n", p->reg_rs_dbg_v1.reg_resize_sc_dbg_v1);
	pr_info("\tREG_RS_dBG_V2.reg_resize_sc_dbg_v2 = 0x%x\n", p->reg_rs_dbg_v2.reg_resize_sc_dbg_v2);
	pr_info("\treg_h130.reg_thresh_top_mod = 0x%x\n", p->reg_h130.reg_thresh_top_mod);
	pr_info("\treg_h130.reg_thresh_thresh_en = 0x%x\n", p->reg_h130.reg_thresh_thresh_en);
	pr_info("\treg_h130.reg_thresh_softrst = 0x%x\n", p->reg_h130.reg_thresh_softrst);
	pr_info("\treg_h134.reg_thresh_16to8_mod = 0x%x\n", p->reg_h134.reg_thresh_16to8_mod);
	pr_info("\treg_h134.reg_thresh_16to8_s8bias = 0x%x\n", p->reg_h134.reg_thresh_16to8_s8bias);
	pr_info("\treg_h134.reg_thresh_16to8_u8Num_div_u16Den = 0x%x\n", p->reg_h134.reg_thresh_16to8_u8Num_div_u16Den);
	pr_info("\treg_h138.reg_thresh_st_16to8_en = 0x%x\n", p->reg_h138.reg_thresh_st_16to8_en);
	pr_info("\treg_h138.reg_thresh_st_16to8_u8Numerator = 0x%x\n", p->reg_h138.reg_thresh_st_16to8_u8Numerator);
	pr_info("\treg_h138.reg_thresh_st_16to8_maxeigval = 0x%x\n", p->reg_h138.reg_thresh_st_16to8_maxeigval);
	pr_info("\treg_h13c.reg_thresh_s16_enmode = 0x%x\n", p->reg_h13c.reg_thresh_s16_enmode);
	pr_info("\treg_h13c.reg_thresh_s16_u8bit_min = 0x%x\n", p->reg_h13c.reg_thresh_s16_u8bit_min);
	pr_info("\treg_h13c.reg_thresh_s16_u8bit_mid = 0x%x\n", p->reg_h13c.reg_thresh_s16_u8bit_mid);
	pr_info("\treg_h13c.reg_thresh_s16_u8bit_max = 0x%x\n", p->reg_h13c.reg_thresh_s16_u8bit_max);
	pr_info("\treg_h140.reg_thresh_s16_bit_thr_l = 0x%x\n", p->reg_h140.reg_thresh_s16_bit_thr_l);
	pr_info("\treg_h140.reg_thresh_s16_bit_thr_h = 0x%x\n", p->reg_h140.reg_thresh_s16_bit_thr_h);
	pr_info("\treg_h144.reg_thresh_u16_enmode = 0x%x\n", p->reg_h144.reg_thresh_u16_enmode);
	pr_info("\treg_h144.reg_thresh_u16_u8bit_min = 0x%x\n", p->reg_h144.reg_thresh_u16_u8bit_min);
	pr_info("\treg_h144.reg_thresh_u16_u8bit_mid = 0x%x\n", p->reg_h144.reg_thresh_u16_u8bit_mid);
	pr_info("\treg_h144.reg_thresh_u16_u8bit_max = 0x%x\n", p->reg_h144.reg_thresh_u16_u8bit_max);
	pr_info("\treg_h148.reg_thresh_u16_bit_thr_l = 0x%x\n", p->reg_h148.reg_thresh_u16_bit_thr_l);
	pr_info("\treg_h148.reg_thresh_u16_bit_thr_h = 0x%x\n", p->reg_h148.reg_thresh_u16_bit_thr_h);
	pr_info("\treg_h14c.reg_thresh_u8bit_thr_l = 0x%x\n", p->reg_h14c.reg_thresh_u8bit_thr_l);
	pr_info("\treg_h14c.reg_thresh_u8bit_thr_h = 0x%x\n", p->reg_h14c.reg_thresh_u8bit_thr_h);
	pr_info("\treg_h14c.reg_thresh_enmode = 0x%x\n", p->reg_h14c.reg_thresh_enmode);
	pr_info("\treg_h150.reg_thresh_u8bit_min = 0x%x\n", p->reg_h150.reg_thresh_u8bit_min);
	pr_info("\treg_h150.reg_thresh_u8bit_mid = 0x%x\n", p->reg_h150.reg_thresh_u8bit_mid);
	pr_info("\treg_h150.reg_thresh_u8bit_max = 0x%x\n", p->reg_h150.reg_thresh_u8bit_max);
	pr_info("\treg_h160.reg_ncc_nemerator_l = 0x%x\n", p->reg_h160.reg_ncc_nemerator_l);
	pr_info("\treg_h164.reg_ncc_nemerator_m = 0x%x\n", p->reg_h164.reg_ncc_nemerator_m);
	pr_info("\treg_h168.reg_ncc_quadsum0_l = 0x%x\n", p->reg_h168.reg_ncc_quadsum0_l);
	pr_info("\treg_h16C.reg_ncc_quadsum0_m = 0x%x\n", p->reg_h16C.reg_ncc_quadsum0_m);
	pr_info("\treg_h170.reg_ncc_quadsum1_l = 0x%x\n", p->reg_h170.reg_ncc_quadsum1_l);
	pr_info("\treg_h174.reg_ncc_quadsum1_m = 0x%x\n", p->reg_h174.reg_ncc_quadsum1_m);
	pr_info("\tREG_R2Y4_11.reg_csc_r2y4_tab_sw_0 = 0x%x\n", p->reg_r2y4_11.reg_csc_r2y4_tab_sw_0);
	pr_info("\tREG_R2Y4_11.reg_csc_r2y4_tab_sw_1 = 0x%x\n", p->reg_r2y4_11.reg_csc_r2y4_tab_sw_1);
	pr_info("\tREG_R2Y4_12.reg_csc_r2y4_tab_sw_update = 0x%x\n", p->reg_r2y4_12.reg_csc_r2y4_tab_sw_update);
	pr_info("\tREG_R2Y4_12.reg_csc_r2y4_coeff_sw_update = 0x%x\n", p->reg_r2y4_12.reg_csc_r2y4_coeff_sw_update);
	pr_info("\treg_r2y4_coeff_0.reg_csc_r2y4_coeff_sw_00 = 0x%x\n", p->reg_r2y4_coeff_0.reg_csc_r2y4_coeff_sw_00);
	pr_info("\treg_r2y4_coeff_1.reg_csc_r2y4_coeff_sw_01 = 0x%x\n", p->reg_r2y4_coeff_1.reg_csc_r2y4_coeff_sw_01);
	pr_info("\treg_r2y4_coeff_2.reg_csc_r2y4_coeff_sw_02 = 0x%x\n", p->reg_r2y4_coeff_2.reg_csc_r2y4_coeff_sw_02);
	pr_info("\treg_r2y4_coeff_3.reg_csc_r2y4_coeff_sw_03 = 0x%x\n", p->reg_r2y4_coeff_3.reg_csc_r2y4_coeff_sw_03);
	pr_info("\treg_r2y4_coeff_4.reg_csc_r2y4_coeff_sw_04 = 0x%x\n", p->reg_r2y4_coeff_4.reg_csc_r2y4_coeff_sw_04);
	pr_info("\treg_r2y4_coeff_5.reg_csc_r2y4_coeff_sw_05 = 0x%x\n", p->reg_r2y4_coeff_5.reg_csc_r2y4_coeff_sw_05);
	pr_info("\treg_r2y4_coeff_6.reg_csc_r2y4_coeff_sw_06 = 0x%x\n", p->reg_r2y4_coeff_6.reg_csc_r2y4_coeff_sw_06);
	pr_info("\treg_r2y4_coeff_7.reg_csc_r2y4_coeff_sw_07 = 0x%x\n", p->reg_r2y4_coeff_7.reg_csc_r2y4_coeff_sw_07);
	pr_info("\treg_r2y4_coeff_8.reg_csc_r2y4_coeff_sw_08 = 0x%x\n", p->reg_r2y4_coeff_8.reg_csc_r2y4_coeff_sw_08);
	pr_info("\treg_r2y4_coeff_9.reg_csc_r2y4_coeff_sw_09 = 0x%x\n", p->reg_r2y4_coeff_9.reg_csc_r2y4_coeff_sw_09);
	pr_info("\treg_r2y4_coeff_A.reg_csc_r2y4_coeff_sw_10 = 0x%x\n", p->reg_r2y4_coeff_A.reg_csc_r2y4_coeff_sw_10);
	pr_info("\treg_r2y4_coeff_B.reg_csc_r2y4_coeff_sw_11 = 0x%x\n", p->reg_r2y4_coeff_B.reg_csc_r2y4_coeff_sw_11);
	pr_info("\tREG_R2Y4_14.reg_csc_r2y4_enmode = 0x%x\n", p->reg_r2y4_14.reg_csc_r2y4_enmode);
	pr_info("\tREG_R2Y4_14.reg_csc_r2y4_enable = 0x%x\n", p->reg_r2y4_14.reg_csc_r2y4_enable);
}

static void ive_update_bg_printk(ive_update_bg_c *p)
{
	pr_info("ive_update_bg\n");
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->reg_1.reg_softrst);
	pr_info("\tREG_H04.reg_enable = 0x%x\n", p->reg_h04.reg_enable);
	pr_info("\tREG_H04.reg_ck_en = 0x%x\n", p->reg_h04.reg_ck_en);
	pr_info("\tREG_H04.reg_updatebg_byp_model = 0x%x\n", p->reg_h04.reg_updatebg_byp_model);
	pr_info("\tREG_2.reg_shdw_sel = 0x%x\n", p->reg_2.reg_shdw_sel);
	pr_info("\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->reg_3.reg_ctrl_dmy1);
	pr_info("\treg_ctrl0.reg_u32CurFrmNum = 0x%x\n", p->reg_ctrl0.reg_u32CurFrmNum);
	pr_info("\treg_ctrl1.reg_u32PreChkTime = 0x%x\n", p->reg_ctrl1.reg_u32PreChkTime);
	pr_info("\treg_ctrl2.reg_u32FrmChkPeriod = 0x%x\n", p->reg_ctrl2.reg_u32FrmChkPeriod);
	pr_info("\treg_ctrl2.reg_u32InitMinTime = 0x%x\n", p->reg_ctrl2.reg_u32InitMinTime);
	pr_info("\treg_ctrl3.reg_u32StyBgMinBlendTime = 0x%x\n", p->reg_ctrl3.reg_u32StyBgMinBlendTime);
	pr_info("\treg_ctrl3.reg_u32StyBgMaxBlendTime = 0x%x\n", p->reg_ctrl3.reg_u32StyBgMaxBlendTime);
	pr_info("\treg_ctrl4.reg_u32DynBgMinBlendTime = 0x%x\n", p->reg_ctrl4.reg_u32DynBgMinBlendTime);
	pr_info("\treg_ctrl4.reg_u32StaticDetMinTime = 0x%x\n", p->reg_ctrl4.reg_u32StaticDetMinTime);
	pr_info("\treg_ctrl5.reg_u16FgMaxFadeTime = 0x%x\n", p->reg_ctrl5.reg_u16FgMaxFadeTime);
	pr_info("\treg_ctrl5.reg_u16BgMaxFadeTime = 0x%x\n", p->reg_ctrl5.reg_u16BgMaxFadeTime);
	pr_info("\treg_ctrl5.reg_u8StyBgAccTimeRateThr = 0x%x\n", p->reg_ctrl5.reg_u8StyBgAccTimeRateThr);
	pr_info("\treg_ctrl5.reg_u8ChgBgAccTimeRateThr = 0x%x\n", p->reg_ctrl5.reg_u8ChgBgAccTimeRateThr);
	pr_info("\treg_ctrl6.reg_u8DynBgAccTimeThr = 0x%x\n", p->reg_ctrl6.reg_u8DynBgAccTimeThr);
	pr_info("\treg_ctrl6.reg_u8BgEffStaRateThr = 0x%x\n", p->reg_ctrl6.reg_u8BgEffStaRateThr);
	pr_info("\treg_ctrl6.reg_u8DynBgDepth = 0x%x\n", p->reg_ctrl6.reg_u8DynBgDepth);
	pr_info("\treg_ctrl6.reg_u8AcceBgLearn = 0x%x\n", p->reg_ctrl6.reg_u8AcceBgLearn);
	pr_info("\treg_ctrl6.reg_u8DetChgRegion = 0x%x\n", p->reg_ctrl6.reg_u8DetChgRegion);
	pr_info("\treg_ctrl7.reg_stat_pixnum = 0x%x\n", p->reg_ctrl7.reg_stat_pixnum);
	pr_info("\treg_ctrl8.reg_stat_sumlum = 0x%x\n", p->reg_ctrl8.reg_stat_sumlum);
	pr_info("\treg_crop_s.reg_crop_start_x = 0x%x\n", p->reg_crop_s.reg_crop_start_x);
	pr_info("\treg_crop_s.reg_crop_end_x = 0x%x\n", p->reg_crop_s.reg_crop_end_x);
	pr_info("\treg_crop_e.reg_crop_start_y = 0x%x\n", p->reg_crop_e.reg_crop_start_y);
	pr_info("\treg_crop_e.reg_crop_end_y = 0x%x\n", p->reg_crop_e.reg_crop_end_y);
	pr_info("\treg_crop_ctl.reg_crop_enable = 0x%x\n", p->reg_crop_ctl.reg_crop_enable);
}

// static void ive_ncc_printk(ive_ncc_c *p)
// {
// 	pr_info("ive_ncc\n");
// 	pr_info("\treg_ncc_00.reg_numerator_l = 0x%x\n", p->reg_ncc_00.reg_numerator_l);
// 	pr_info("\treg_ncc_01.reg_numerator_h = 0x%x\n", p->reg_ncc_01.reg_numerator_h);
// 	pr_info("\treg_ncc_02.reg_quadsum0_l = 0x%x\n", p->reg_ncc_02.reg_quadsum0_l);
// 	pr_info("\treg_ncc_03.reg_quadsum0_h = 0x%x\n", p->reg_ncc_03.reg_quadsum0_h);
// 	pr_info("\treg_ncc_04.reg_quadsum1_l = 0x%x\n", p->reg_ncc_04.reg_quadsum1_l);
// 	pr_info("\treg_ncc_05.reg_quadsum1_h = 0x%x\n", p->reg_ncc_05.reg_quadsum1_h);
// 	pr_info("\treg_ncc_06.reg_crop_enable = 0x%x\n", p->reg_ncc_06.reg_crop_enable);
// 	pr_info("\treg_ncc_07.reg_crop_start_x = 0x%x\n", p->reg_ncc_07.reg_crop_start_x);
// 	pr_info("\treg_ncc_08.reg_crop_start_y = 0x%x\n", p->reg_ncc_08.reg_crop_start_y);
// 	pr_info("\treg_ncc_09.reg_crop_end_x = 0x%x\n", p->reg_ncc_09.reg_crop_end_x);
// 	pr_info("\treg_ncc_10.reg_crop_end_y = 0x%x\n", p->reg_ncc_10.reg_crop_end_y);
// 	pr_info("\treg_ncc_11.reg_shdw_sel = 0x%x\n", p->reg_ncc_11.reg_shdw_sel);
// }

static void isp_dma_ctl_printk(isp_dma_ctl_c *p)
{
	pr_info("isp_dma_ctl\n");
	pr_info("\tSYS_CONTROL.reg_qos_sel = 0x%x\n",
		p->sys_control.reg_qos_sel);
	pr_info("\tSYS_CONTROL.reg_sw_qos = 0x%x\n", p->sys_control.reg_sw_qos);
	pr_info("\tSYS_CONTROL.reg_baseh = 0x%x\n", p->sys_control.reg_baseh);
	pr_info("\tSYS_CONTROL.reg_base_sel = 0x%x\n",
		p->sys_control.reg_base_sel);
	pr_info("\tSYS_CONTROL.reg_stride_sel = 0x%x\n",
		p->sys_control.reg_stride_sel);
	pr_info("\tSYS_CONTROL.reg_seglen_sel = 0x%x\n",
		p->sys_control.reg_seglen_sel);
	pr_info("\tSYS_CONTROL.reg_segnum_sel = 0x%x\n",
		p->sys_control.reg_segnum_sel);
	pr_info("\tSYS_CONTROL.reg_slice_enable = 0x%x\n",
		p->sys_control.reg_slice_enable);
	pr_info("\tSYS_CONTROL.reg_dbg_sel = 0x%x\n",
		p->sys_control.reg_dbg_sel);
	pr_info("\tBASE_ADDR.reg_basel = 0x%x\n", p->base_addr.reg_basel);
	pr_info("\tDMA_SEGLEN.reg_seglen = 0x%x\n", p->dma_seglen.reg_seglen);
	pr_info("\tDMA_STRIDE.reg_stride = 0x%x\n", p->dma_stride.reg_stride);
	pr_info("\tDMA_SEGNUM.reg_segnum = 0x%x\n", p->dma_segnum.reg_segnum);
	pr_info("\tDMA_STATUS.reg_status = 0x%x\n", p->dma_status.reg_status);
	pr_info("\tDMA_SLICESIZE.reg_slice_size = 0x%x\n",
		p->dma_slicesize.reg_slice_size);
	pr_info("\tDMA_DUMMY.reg_dummy = 0x%x\n", p->dma_dummy.reg_dummy);
}

static void img_in_printk(img_in_c *p)
{
	pr_info("img_in\n");
	pr_info("\tREG_00.reg_src_sel = 0x%x\n", p->reg_00.reg_src_sel);
	pr_info("\tREG_00.reg_fmt_sel = 0x%x\n", p->reg_00.reg_fmt_sel);
	pr_info("\tREG_00.reg_burst_ln = 0x%x\n", p->reg_00.reg_burst_ln);
	pr_info("\tREG_00.reg_img_csc_en = 0x%x\n", p->reg_00.reg_img_csc_en);
	pr_info("\tREG_00.reg_auto_csc_en = 0x%x\n", p->reg_00.reg_auto_csc_en);
	pr_info("\tREG_00.reg_64b_align = 0x%x\n", p->reg_00.reg_64b_align);
	pr_info("\tREG_00.reg_force_clk_enable = 0x%x\n",
		p->reg_00.reg_force_clk_enable);
	pr_info("\tREG_01.reg_src_x_str = 0x%x\n", p->reg_01.reg_src_x_str);
	pr_info("\tREG_01.reg_src_y_str = 0x%x\n", p->reg_01.reg_src_y_str);
	pr_info("\tREG_02.reg_src_wd = 0x%x\n", p->reg_02.reg_src_wd);
	pr_info("\tREG_02.reg_src_ht = 0x%x\n", p->reg_02.reg_src_ht);
	pr_info("\tREG_03.reg_src_y_pitch = 0x%x\n", p->reg_03.reg_src_y_pitch);
	pr_info("\tREG_04.reg_src_c_pitch = 0x%x\n", p->reg_04.reg_src_c_pitch);
	pr_info("\tREG_05.reg_sw_force_up = 0x%x\n", p->reg_05.reg_sw_force_up);
	pr_info("\tREG_05.reg_sw_mask_up = 0x%x\n", p->reg_05.reg_sw_mask_up);
	pr_info("\tREG_05.reg_shrd_sel = 0x%x\n", p->reg_05.reg_shrd_sel);
	pr_info("\tREG_06.reg_dummy_ro = 0x%x\n", p->reg_06.reg_dummy_ro);
	pr_info("\tREG_07.reg_dummy_0 = 0x%x\n", p->reg_07.reg_dummy_0);
	pr_info("\tREG_08.reg_dummy_1 = 0x%x\n", p->reg_08.reg_dummy_1);
	pr_info("\tREG_Y_BASE_0.reg_src_y_base_b0 = 0x%x\n",
		p->reg_y_base_0.reg_src_y_base_b0);
	pr_info("\tREG_Y_BASE_1.reg_src_y_base_b1 = 0x%x\n",
		p->reg_y_base_1.reg_src_y_base_b1);
	pr_info("\tREG_U_BASE_0.reg_src_u_base_b0 = 0x%x\n",
		p->reg_u_base_0.reg_src_u_base_b0);
	pr_info("\tREG_U_BASE_1.reg_src_u_base_b1 = 0x%x\n",
		p->reg_u_base_1.reg_src_u_base_b1);
	pr_info("\tREG_V_BASE_0.reg_src_v_base_b0 = 0x%x\n",
		p->reg_v_base_0.reg_src_v_base_b0);
	pr_info("\tREG_V_BASE_1.reg_src_v_base_b1 = 0x%x\n",
		p->reg_v_base_1.reg_src_v_base_b1);
	pr_info("\tREG_040.reg_c00 = 0x%x\n", p->reg_040.reg_c00);
	pr_info("\tREG_040.reg_c01 = 0x%x\n", p->reg_040.reg_c01);
	pr_info("\tREG_044.reg_c02 = 0x%x\n", p->reg_044.reg_c02);
	pr_info("\tREG_048.reg_c10 = 0x%x\n", p->reg_048.reg_c10);
	pr_info("\tREG_048.reg_c11 = 0x%x\n", p->reg_048.reg_c11);
	pr_info("\tREG_04C.reg_c12 = 0x%x\n", p->reg_04c.reg_c12);
	pr_info("\tREG_050.reg_c20 = 0x%x\n", p->reg_050.reg_c20);
	pr_info("\tREG_050.reg_c21 = 0x%x\n", p->reg_050.reg_c21);
	pr_info("\tREG_054.reg_c22 = 0x%x\n", p->reg_054.reg_c22);
	pr_info("\tREG_058.reg_sub_0 = 0x%x\n", p->reg_058.reg_sub_0);
	pr_info("\tREG_058.reg_sub_1 = 0x%x\n", p->reg_058.reg_sub_1);
	pr_info("\tREG_058.reg_sub_2 = 0x%x\n", p->reg_058.reg_sub_2);
	pr_info("\tREG_05C.reg_add_0 = 0x%x\n", p->reg_05c.reg_add_0);
	pr_info("\tREG_05C.reg_add_1 = 0x%x\n", p->reg_05c.reg_add_1);
	pr_info("\tREG_05C.reg_add_2 = 0x%x\n", p->reg_05c.reg_add_2);
	pr_info("\tREG_060.reg_fifo_rd_th_y = 0x%x\n",
		p->reg_060.reg_fifo_rd_th_y);
	pr_info("\tREG_060.reg_fifo_pr_th_y = 0x%x\n",
		p->reg_060.reg_fifo_pr_th_y);
	pr_info("\tREG_060.reg_fifo_rd_th_c = 0x%x\n",
		p->reg_060.reg_fifo_rd_th_c);
	pr_info("\tREG_060.reg_fifo_pr_th_c = 0x%x\n",
		p->reg_060.reg_fifo_pr_th_c);
	pr_info("\tREG_064.reg_os_max = 0x%x\n", p->reg_064.reg_os_max);
	pr_info("\tREG_068.reg_err_fwr_y = 0x%x\n", p->reg_068.reg_err_fwr_y);
	pr_info("\tREG_068.reg_err_fwr_u = 0x%x\n", p->reg_068.reg_err_fwr_u);
	pr_info("\tREG_068.reg_err_fwr_v = 0x%x\n", p->reg_068.reg_err_fwr_v);
	pr_info("\tREG_068.reg_clr_fwr_w1t = 0x%x\n",
		p->reg_068.reg_clr_fwr_w1t);
	pr_info("\tREG_068.reg_err_erd_y = 0x%x\n", p->reg_068.reg_err_erd_y);
	pr_info("\tREG_068.reg_err_erd_u = 0x%x\n", p->reg_068.reg_err_erd_u);
	pr_info("\tREG_068.reg_err_erd_v = 0x%x\n", p->reg_068.reg_err_erd_v);
	pr_info("\tREG_068.reg_clr_erd_w1t = 0x%x\n",
		p->reg_068.reg_clr_erd_w1t);
	pr_info("\tREG_068.reg_lb_full_y = 0x%x\n", p->reg_068.reg_lb_full_y);
	pr_info("\tREG_068.reg_lb_full_u = 0x%x\n", p->reg_068.reg_lb_full_u);
	pr_info("\tREG_068.reg_lb_full_v = 0x%x\n", p->reg_068.reg_lb_full_v);
	pr_info("\tREG_068.reg_lb_empty_y = 0x%x\n", p->reg_068.reg_lb_empty_y);
	pr_info("\tREG_068.reg_lb_empty_u = 0x%x\n", p->reg_068.reg_lb_empty_u);
	pr_info("\tREG_068.reg_lb_empty_v = 0x%x\n", p->reg_068.reg_lb_empty_v);
	pr_info("\tREG_068.reg_ip_idle = 0x%x\n", p->reg_068.reg_ip_idle);
	pr_info("\tREG_068.reg_ip_int = 0x%x\n", p->reg_068.reg_ip_int);
	pr_info("\tREG_068.reg_ip_clr_w1t = 0x%x\n", p->reg_068.reg_ip_clr_w1t);
	pr_info("\tREG_068.reg_clr_int_w1t = 0x%x\n",
		p->reg_068.reg_clr_int_w1t);
	pr_info("\tREG_AXI_ST.reg_axi_idle = 0x%x\n",
		p->reg_axi_st.reg_axi_idle);
	pr_info("\tREG_AXI_ST.reg_axi_status = 0x%x\n",
		p->reg_axi_st.reg_axi_status);
	pr_info("\tREG_BW_LIMIT.reg_bwl_win = 0x%x\n",
		p->reg_bw_limit.reg_bwl_win);
	pr_info("\tREG_BW_LIMIT.reg_bwl_vld = 0x%x\n",
		p->reg_bw_limit.reg_bwl_vld);
	pr_info("\tREG_BW_LIMIT.reg_bwl_en = 0x%x\n",
		p->reg_bw_limit.reg_bwl_en);
	pr_info("\tREG_CATCH.reg_catch_mode = 0x%x\n",
		p->reg_catch.reg_catch_mode);
	pr_info("\tREG_CATCH.reg_dma_urgent_en = 0x%x\n",
		p->reg_catch.reg_dma_urgent_en);
	pr_info("\tREG_CATCH.reg_qos_sel_rr = 0x%x\n",
		p->reg_catch.reg_qos_sel_rr);
	pr_info("\tREG_CATCH.reg_catch_act_y = 0x%x\n",
		p->reg_catch.reg_catch_act_y);
	pr_info("\tREG_CATCH.reg_catch_act_u = 0x%x\n",
		p->reg_catch.reg_catch_act_u);
	pr_info("\tREG_CATCH.reg_catch_act_v = 0x%x\n",
		p->reg_catch.reg_catch_act_v);
	pr_info("\tREG_CATCH.reg_catch_fail_y = 0x%x\n",
		p->reg_catch.reg_catch_fail_y);
	pr_info("\tREG_CATCH.reg_catch_fail_u = 0x%x\n",
		p->reg_catch.reg_catch_fail_u);
	pr_info("\tREG_CATCH.reg_catch_fail_v = 0x%x\n",
		p->reg_catch.reg_catch_fail_v);
	pr_info("\tREG_CHK_CTRL.reg_chksum_dat_out = 0x%x\n",
		p->reg_chk_ctrl.reg_chksum_dat_out);
	pr_info("\tREG_CHK_CTRL.reg_checksum_en = 0x%x\n",
		p->reg_chk_ctrl.reg_checksum_en);
	pr_info("\tCHKSUM_AXI_RD.reg_chksum_axi_rd = 0x%x\n",
		p->chksum_axi_rd.reg_chksum_axi_rd);
	pr_info("\tSB_REG_CTRL.reg_sb_mode = 0x%x\n",
		p->sb_reg_ctrl.reg_sb_mode);
	pr_info("\tSB_REG_CTRL.reg_sb_size = 0x%x\n",
		p->sb_reg_ctrl.reg_sb_size);
	pr_info("\tSB_REG_CTRL.reg_sb_nb = 0x%x\n", p->sb_reg_ctrl.reg_sb_nb);
	pr_info("\tSB_REG_CTRL.reg_sb_sw_rptr = 0x%x\n",
		p->sb_reg_ctrl.reg_sb_sw_rptr);
	pr_info("\tSB_REG_CTRL.reg_sb_set_str = 0x%x\n",
		p->sb_reg_ctrl.reg_sb_set_str);
	pr_info("\tSB_REG_CTRL.reg_sb_sw_clr = 0x%x\n",
		p->sb_reg_ctrl.reg_sb_sw_clr);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_rptr_ro = 0x%x\n",
		p->sb_reg_c_stat.reg_u_sb_rptr_ro);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_full = 0x%x\n",
		p->sb_reg_c_stat.reg_u_sb_full);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_empty = 0x%x\n",
		p->sb_reg_c_stat.reg_u_sb_empty);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_dptr_ro = 0x%x\n",
		p->sb_reg_c_stat.reg_u_sb_dptr_ro);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_rptr_ro = 0x%x\n",
		p->sb_reg_c_stat.reg_v_sb_rptr_ro);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_full = 0x%x\n",
		p->sb_reg_c_stat.reg_v_sb_full);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_empty = 0x%x\n",
		p->sb_reg_c_stat.reg_v_sb_empty);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_dptr_ro = 0x%x\n",
		p->sb_reg_c_stat.reg_v_sb_dptr_ro);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_rptr_ro = 0x%x\n",
		p->sb_reg_y_stat.reg_y_sb_rptr_ro);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_full = 0x%x\n",
		p->sb_reg_y_stat.reg_y_sb_full);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_empty = 0x%x\n",
		p->sb_reg_y_stat.reg_y_sb_empty);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_dptr_ro = 0x%x\n",
		p->sb_reg_y_stat.reg_y_sb_dptr_ro);
	pr_info("\tSB_REG_Y_STAT.reg_sb_empty = 0x%x\n",
		p->sb_reg_y_stat.reg_sb_empty);
}

#if IVE_CMDQ
static void cmdq_printk(cmdq_c *p)
{
	pr_info("cmdq\n");
	pr_info("\tINT_EVENT.reg_cmdq_int = 0x%x\n", p->int_event.reg_cmdq_int);
	pr_info("\tINT_EVENT.reg_cmdq_end = 0x%x\n", p->int_event.reg_cmdq_end);
	pr_info("\tINT_EVENT.reg_cmdq_wait = 0x%x\n", p->int_event.reg_cmdq_wait);
	pr_info("\tINT_EVENT.reg_isp_pslverr = 0x%x\n", p->int_event.reg_isp_pslverr);
	pr_info("\tINT_EVENT.reg_task_end = 0x%x\n", p->int_event.reg_task_end);
	pr_info("\tINT_EN.reg_cmdq_int_en = 0x%x\n", p->int_en.reg_cmdq_int_en);
	pr_info("\tINT_EN.reg_cmdq_end_en = 0x%x\n", p->int_en.reg_cmdq_end_en);
	pr_info("\tINT_EN.reg_cmdq_wait_en = 0x%x\n", p->int_en.reg_cmdq_wait_en);
	pr_info("\tINT_EN.reg_isp_pslverr_en = 0x%x\n", p->int_en.reg_isp_pslverr_en);
	pr_info("\tINT_EN.reg_task_end_en = 0x%x\n", p->int_en.reg_task_end_en);
	pr_info("\tDMA_ADDR.reg_dma_addr_l = 0x%x\n", p->dma_addr_l.reg_dma_addr_l);
	pr_info("\tDMA_CNT.reg_dma_cnt = 0x%x\n", p->dma_cnt.reg_dma_cnt);
	pr_info("\tDMA_CONFIG.reg_dma_rsv = 0x%x\n", p->dma_config.reg_dma_rsv);
	pr_info("\tDMA_CONFIG.reg_adma_en = 0x%x\n", p->dma_config.reg_adma_en);
	pr_info("\tDMA_CONFIG.reg_task_en = 0x%x\n", p->dma_config.reg_task_en);
	pr_info("\tAXI_CONFIG.reg_max_burst_len = 0x%x\n", p->axi_config.reg_max_burst_len);
	pr_info("\tAXI_CONFIG.reg_ot_enable = 0x%x\n", p->axi_config.reg_ot_enable);
	pr_info("\tAXI_CONFIG.reg_sw_overwrite = 0x%x\n", p->axi_config.reg_sw_overwrite);
	pr_info("\tJOB_CTL.reg_job_start = 0x%x\n", p->job_ctl.reg_job_start);
	pr_info("\tJOB_CTL.reg_cmd_restart = 0x%x\n", p->job_ctl.reg_cmd_restart);
	pr_info("\tJOB_CTL.reg_restart_hw_mod = 0x%x\n", p->job_ctl.reg_restart_hw_mod);
	pr_info("\tJOB_CTL.reg_cmdq_idle_en = 0x%x\n", p->job_ctl.reg_cmdq_idle_en);
	pr_info("\tAPB_PARA.reg_base_addr = 0x%x\n", p->apb_para.reg_base_addr);
	pr_info("\tAPB_PARA.reg_apb_pprot = 0x%x\n", p->apb_para.reg_apb_pprot);
	pr_info("\tDEBUG_BUS0.reg_debus0 = 0x%x\n", p->debug_bus0.reg_debus0);
	pr_info("\tDEBUG_BUS1.reg_debus1 = 0x%x\n", p->debug_bus1.reg_debus1);
	pr_info("\tDEBUG_BUS2.reg_debus2 = 0x%x\n", p->debug_bus2.reg_debus2);
	pr_info("\tDEBUG_BUS3.reg_debus3 = 0x%x\n", p->debug_bus3.reg_debus3);
	pr_info("\tDEBUG_BUS_SEL.reg_debus_sel = 0x%x\n", p->debug_bus_sel.reg_debus_sel);
	pr_info("\tDUMMY.reg_dummy = 0x%x\n", p->dummy.reg_dummy);
	pr_info("\tTASK_DONE_STS.reg_task_done = 0x%x\n", p->task_done_sts.reg_task_done);
	pr_info("\tDMA_ADDR_TSK0.reg_dma_addr_tsk0 = 0x%x\n", p->dma_addr_tsk0.reg_dma_addr_tsk0);
	pr_info("\tDMA_CNT_TSK0.reg_dma_cnt_tsk0 = 0x%x\n", p->dma_cnt_tsk0.reg_dma_cnt_tsk0);
	pr_info("\tDMA_ADDR_TSK1.reg_dma_addr_tsk1 = 0x%x\n", p->dma_addr_tsk1.reg_dma_addr_tsk1);
	pr_info("\tDMA_CNT_TSK1.reg_dma_cnt_tsk1 = 0x%x\n", p->dma_cnt_tsk1.reg_dma_cnt_tsk1);
	pr_info("\tDMA_ADDR_TSK2.reg_dma_addr_tsk2 = 0x%x\n", p->dma_addr_tsk2.reg_dma_addr_tsk2);
	pr_info("\tDMA_CNT_TSK2.reg_dma_cnt_tsk2 = 0x%x\n", p->dma_cnt_tsk2.reg_dma_cnt_tsk2);
	pr_info("\tDMA_ADDR_TSK3.reg_dma_addr_tsk3 = 0x%x\n", p->dma_addr_tsk3.reg_dma_addr_tsk3);
	pr_info("\tDMA_CNT_TSK3.reg_dma_cnt_tsk3 = 0x%x\n", p->dma_cnt_tsk3.reg_dma_cnt_tsk3);
	pr_info("\tDMA_ADDR_TSK4.reg_dma_addr_tsk4 = 0x%x\n", p->dma_addr_tsk4.reg_dma_addr_tsk4);
	pr_info("\tDMA_CNT_TSK4.reg_dma_cnt_tsk4 = 0x%x\n", p->dma_cnt_tsk4.reg_dma_cnt_tsk4);
	pr_info("\tDMA_ADDR_TSK5.reg_dma_addr_tsk5 = 0x%x\n", p->dma_addr_tsk5.reg_dma_addr_tsk5);
	pr_info("\tDMA_CNT_TSK5.reg_dma_cnt_tsk5 = 0x%x\n", p->dma_cnt_tsk5.reg_dma_cnt_tsk5);
	pr_info("\tDMA_ADDR_TSK6.reg_dma_addr_tsk6 = 0x%x\n", p->dma_addr_tsk6.reg_dma_addr_tsk6);
	pr_info("\tDMA_CNT_TSK6.reg_dma_cnt_tsk6 = 0x%x\n", p->dma_cnt_tsk6.reg_dma_cnt_tsk6);
	pr_info("\tDMA_ADDR_TSK7.reg_dma_addr_tsk7 = 0x%x\n", p->dma_addr_tsk7.reg_dma_addr_tsk7);
	pr_info("\tDMA_CNT_TSK7.reg_dma_cnt_tsk7 = 0x%x\n", p->dma_cnt_tsk7.reg_dma_cnt_tsk7);
}
#endif

uint32_t width_align(const uint32_t width, const uint32_t align)
{
	uint32_t stride = (uint32_t)(width / align) * align;
	if (stride < width) {
		stride += align;
	}
	return stride;
}

//#define ENUM_TYPE_CASE(x) (#x+15)
u32 get_channel_count(ive_image_type_e type)
{
	switch (type) {
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		return 1;
	case IVE_IMAGE_TYPE_YUV422SP:
	case IVE_IMAGE_TYPE_YUV422P:
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
	case IVE_IMAGE_TYPE_S8C2_PLANAR:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_BF16C1:
		return 2;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		return 3;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
	case IVE_IMAGE_TYPE_FP32C1:
		return 4;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		return 8;
	default:
		return 0;
	}
}

void dump_ive_image(char *name, ive_image_s *img)
{
	s32 i = 0;

	pr_info("Image %s\n", name);
	if (img != NULL) {
		pr_info("\tType: %#x\n", img->type);
		pr_info("\tWidth: %d\n", img->width);
		pr_info("\tHeight: %d\n", img->height);
		for (i = 0; i < get_channel_count(img->type); i++) {
			if (img->phy_addr[i] != 0 &&
				img->phy_addr[i] != 0xffffffff) {
				pr_info("\tPhy %d Addr: 0x%08llx\n", i,
					img->phy_addr[i] & 0xffffffff);
				pr_info("\tStride %d: %d\n", i, img->stride[i]);
			}
		}
	} else {
		pr_info("\tNULL\n");
	}
}

void dump_ive_data(char *name, ive_data_s *data)
{
	pr_info("Data %s\n", name);
	if (data != NULL) {
		pr_info("\tWidth: %d\n", data->width);
		pr_info("\tHeight: %d\n", data->height);
		if (data->phy_addr != 0 && data->phy_addr != 0xffffffff) {
			pr_info("\tPhy Addr: 0x%08llx\n", data->phy_addr & 0xffffffff);
			pr_info("\tStride: %d\n", data->stride);
		}
	} else {
		pr_info("\tNULL\n");
	}
}

void dump_ive_mem(char *name, ive_mem_info_s *mem)
{
	pr_info("Mem %s\n", name);
	if (mem != NULL) {
		pr_info("\tSize: %d\n", mem->size);
		if (mem->phy_addr != 0 && mem->phy_addr != 0xffffffff) {
			pr_info("\tPhy Addr: 0x%08llx\n", mem->phy_addr & 0xffffffff);
		}
	} else {
		pr_info("\tNULL\n");
	}
}

s32 get_img_fmt_sel(ive_image_type_e type)
{
	s32 r = FAILURE;

	switch (type) {
	case IVE_IMAGE_TYPE_U8C1:
		r = 5;
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		r = 2;
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		r = 3;
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		r = 9;
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		r = 0xb;
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		r = 5;
		break;
	case IVE_IMAGE_TYPE_YUV420P:
		r = 0;
		break;
	case IVE_IMAGE_TYPE_YUV422P:
		r = 1;
		break;
	default:
		// TODO: check YUV422SP set to 0xa
		pr_err("not support src type\n");
		break;
	}
	return r;
}

static void ive_dump_top_reg_state(void)
{
	ive_top_c top;
	pr_info("IVE_BLK_BA_IVE_TOP 0x%p\n",
		IVE_BLK_BA[d_num].IVE_TOP - g_phy_shift[d_num]);
	memcpy(&top, (void *)(IVE_BLK_BA[d_num].IVE_TOP), sizeof(ive_top_c));
	ive_top_printk(&top);
}

static void ive_dump_filterop_reg_state(void)
{
	ive_filterop_c filterop;
	pr_info("IVE_BLK_BA_FILTEROP 0x%p\n",
		IVE_BLK_BA[d_num].FILTEROP - g_phy_shift[d_num]);
	memcpy(&filterop, (void *)(IVE_BLK_BA[d_num].FILTEROP),
		   sizeof(ive_filterop_c));
	ive_filterop_printk(&filterop);

}

static void ive_dump_img_reg_state(void)
{
	img_in_c img_in;
	isp_dma_ctl_c isp_dma;

	//src1
	pr_info("IVE_BLK_BA_IMG_IN 0x%p\n",
		IVE_BLK_BA[d_num].IMG_IN - g_phy_shift[d_num]);
	memcpy(&img_in, (void *)(IVE_BLK_BA[d_num].IMG_IN), sizeof(img_in_c));
	img_in_printk(&img_in);

	//dst1
	pr_info("IVE_BLK_BA_FILTEROP_WDMA_Y 0x%p\n",
		IVE_BLK_BA[d_num].FILTEROP_WDMA_Y - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].FILTEROP_WDMA_Y),
		   sizeof(isp_dma_ctl_c));
	isp_dma_ctl_printk(&isp_dma);

	//src2
	pr_info("IVE_BLK_BA_RDMA_IMG1 0x%p\n",
		IVE_BLK_BA[d_num].RDMA_IMG1 - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].RDMA_IMG1),
		   sizeof(isp_dma_ctl_c));
	isp_dma_ctl_printk(&isp_dma);

	//dst2
	pr_info("IVE_BLK_BA_FILTEROP_WDMA_C 0x%p\n",
		IVE_BLK_BA[d_num].FILTEROP_WDMA_C - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].FILTEROP_WDMA_C),
		   sizeof(isp_dma_ctl_c));
	isp_dma_ctl_printk(&isp_dma);

	//src3
	pr_info("IVE_BLK_BA_DMAF_RDMA 0x%p\n",
		IVE_BLK_BA[d_num].DMAF_RDMA - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].DMAF_RDMA),
		   sizeof(isp_dma_ctl_c));
	isp_dma_ctl_printk(&isp_dma);

	//dst3
	pr_info("IVE_BLK_BA_DMAF_WDMA 0x%p\n",
		IVE_BLK_BA[d_num].DMAF_WDMA - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].DMAF_WDMA),
		   sizeof(isp_dma_ctl_c));
	isp_dma_ctl_printk(&isp_dma);
}

s32 ive_dump_reg_state(bool bDump)
{
	// isp_dma_ctl_c isp_dma;
	ive_gmm_c gmm;
	ive_match_bg_c bgmodel;
	ive_update_bg_c upmodel;
	// ive_map_c map;
	// ive_dma_c dma;
	// ive_ncc_c ncc;
	// ive_sad_c sad;


	if (g_dump_reg_info || bDump) {
		//top
		ive_dump_top_reg_state();

		//filterop
		ive_dump_filterop_reg_state();

		//print src and dst
		ive_dump_img_reg_state();

		//SAD SAD_WDMA
		// pr_info("IVE_BLK_BA_SAD_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].SAD_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].SAD_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// //SAD WDMA_THR
		// pr_info("IVE_BLK_BA_SAD_WDMA_THR 0x%p\n",
		// 	IVE_BLK_BA[d_num].SAD_WDMA_THR - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].SAD_WDMA_THR),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// //RDMA_EIGVAL
		// pr_info("IVE_BLK_RDMA_EIGVAL 0x%p\n",
		// 	IVE_BLK_BA[d_num].RDMA_EIGVAL - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].RDMA_EIGVAL),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// //MAP
		// pr_info("IVE_BLK_BA_MAP 0x%p\n", IVE_BLK_BA[d_num].MAP - g_phy_shift[d_num]);
		// memcpy(&map, (void *)(IVE_BLK_BA[d_num].MAP), sizeof(ive_map_c));
		// ive_map_printk(&map);

		// //DMAF
		// pr_info("IVE_BLK_BA_DMAF 0x%p\n",
		// 	IVE_BLK_BA[d_num].DMAF - g_phy_shift[d_num]);
		// memcpy(&dma, (void *)(IVE_BLK_BA[d_num].DMAF), sizeof(ive_dma_c));
		// ive_dma_printk(&dma);

		// //NCC
		// pr_info("IVE_BLK_BA_NCC 0x%p\n", IVE_BLK_BA[d_num].NCC - g_phy_shift[d_num]);
		// memcpy(&ncc, (void *)(IVE_BLK_BA[d_num].NCC), sizeof(ive_ncc_c));
		// ive_ncc_printk(&ncc);

		// //SAD
		// pr_info("IVE_BLK_BA_SAD 0x%p\n", IVE_BLK_BA[d_num].SAD - g_phy_shift[d_num]);
		// memcpy(&sad, (void *)(IVE_BLK_BA[d_num].SAD), sizeof(ive_sad_c));
		// ive_sad_printk(&sad);

		// //GMM_MATCH_WDMA
		// pr_info("IVE_BLK_BA_GMM_MATCH_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MATCH_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MATCH_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_FACTOR_RDMA
		// pr_info("IVE_BLK_BA_GMM_FACTOR_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_FACTOR_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_FACTOR_RDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_0
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_0 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_0 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_0),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_1
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_1 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_1 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_1),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_2
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_2 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_2 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_2),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_3
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_3 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_3 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_3),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_4
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_4 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_4 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_4),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_0
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_0 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_0 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_0),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_1
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_1 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_1 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_1),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_2
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_2 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_2 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_2),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_3
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_3 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_3 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_3),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_4
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_4 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_4 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_4),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);
		//GMM
		pr_info("IVE_BLK_BA_GMM 0x%p\n", IVE_BLK_BA[d_num].GMM - g_phy_shift[d_num]);
		memcpy(&gmm, (void *)(IVE_BLK_BA[d_num].GMM), sizeof(ive_gmm_c));
		ive_gmm_printk(&gmm);

		pr_info("IVE_BLK_BA_MATCH_BGMODEL 0x%p\n",
			IVE_BLK_BA[d_num].BG_MATCH_IVE_MATCH_BG - g_phy_shift[d_num]);
		memcpy(&bgmodel, (void *)(IVE_BLK_BA[d_num].BG_MATCH_IVE_MATCH_BG),
			   sizeof(ive_match_bg_c));
		ive_match_bg_printk(&bgmodel);

		pr_info("IVE_BLK_BA_UPDATE_BGMODEL 0x%p\n",
			IVE_BLK_BA[d_num].BG_UPDATE_UPDATE_BG - g_phy_shift[d_num]);
		memcpy(&upmodel, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_UPDATE_BG),
			   sizeof(ive_update_bg_c));
		ive_update_bg_printk(&upmodel);

		// pr_info("BG_MATCH_BGMODEL_0_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_0_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_0_RDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_MATCH_BGMODEL_1_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_1_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_1_RDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_MATCH_FGFLAG_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_FGFLAG_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_FGFLAG_RDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_UPDATE_BGMODEL_0_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_0_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_0_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_UPDATE_BGMODEL_1_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_1_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_1_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_UPDATE_FG_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_FG_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_FG_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_MATCH_DIFFFG_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_DIFFFG_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_DIFFFG_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("IVE_BLK_ID_BG_UPDATE 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_CHG_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_CHG_WDMA),
		// 	   sizeof(isp_dma_ctl_c));
		// isp_dma_ctl_printk(&isp_dma);

	}
	return SUCCESS;
}

void ive_reset_reg(s32 select, ive_top_c *Top, s32 dev_id)
{
	s32 i = 0;
	s32 size = 0;
	uint32_t *array;

	if (select == 1) {
		size = sizeof(ive_top_c) / sizeof(uint32_t);
		array = (uint32_t *)Top;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IVE_TOP + sizeof(uint32_t) * i));
		}

	} else if (select == 2) {
		//DEFINE_IVE_FILTEROP_C(FilterOp);
		ive_filterop_c FilterOp = _DEFINE_IVE_FILTEROP_C;

		size = sizeof(ive_filterop_c) / sizeof(uint32_t);
		array = (uint32_t *)&FilterOp;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].FILTEROP + sizeof(uint32_t) * i));
		}
	} else if (select == 3) {
		//DEFINE_img_in_c(ImageIn);
		img_in_c ImageIn = _DEFINE_img_in_c;

		size = sizeof(img_in_c) / sizeof(uint32_t);
		array = (uint32_t *)&ImageIn;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IMG_IN + sizeof(uint32_t) * i));
		}
	} else if (select == 4) {
		//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
		ive_match_bg_c ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
		//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
		ive_update_bg_c ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;

		size = sizeof(ive_match_bg_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_match_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ive_update_bg_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_update_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
					  sizeof(uint32_t) * i));
		}
	} else {
		//DEFINE_IVE_FILTEROP_C(FilterOp);
		ive_filterop_c FilterOp = _DEFINE_IVE_FILTEROP_C;
		//DEFINE_img_in_c(ImageIn);
		img_in_c ImageIn = _DEFINE_img_in_c;
		//DEFINE_IVE_DMA_C(DMA);
		ive_dma_c DMA = _DEFINE_IVE_DMA_C;
		//DEFINE_isp_dma_ctl_c(DMActl);
		isp_dma_ctl_c DMActl = _DEFINE_isp_dma_ctl_c;
		//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
		ive_match_bg_c ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
		//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
		ive_update_bg_c ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;
		//DEFINE_IVE_HIST_C(ive_hist_c);
		ive_hist_c ive_hist_c = _DEFINE_IVE_HIST_C;
		//DEFINE_IVE_MAP_C(ive_map_c);
		ive_map_c ive_map_c = _DEFINE_IVE_MAP_C;
		//DEFINE_IVE_INTG_C(ive_intg_c);
		ive_intg_c ive_intg_c = _DEFINE_IVE_INTG_C;
		//DEFINE_IVE_SAD_C(ive_sad_c);
		ive_sad_c ive_sad_c = _DEFINE_IVE_SAD_C;
		//DEFINE_IVE_NCC_C(ive_ncc_c);
		ive_ncc_c ive_ncc_c = _DEFINE_IVE_NCC_C;

		size = sizeof(ive_top_c) / sizeof(uint32_t);
		array = (uint32_t *)Top;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IVE_TOP + sizeof(uint32_t) * i));
		}
		size = sizeof(ive_filterop_c) / sizeof(uint32_t);
		array = (uint32_t *)&FilterOp;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].FILTEROP + sizeof(uint32_t) * i));
		}
		Top->reg_h10.reg_img_in_top_enable = 1;
		//img_in_printk((img_in_c *)(IVE_BLK_BA.IMG_IN));
		writel(Top->reg_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		size = sizeof(img_in_c) / sizeof(uint32_t);
		array = (uint32_t *)&ImageIn;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IMG_IN + sizeof(uint32_t) * i));
		}
		//img_in_printk((img_in_c *)(IVE_BLK_BA.IMG_IN));
		Top->reg_h10.reg_img_in_top_enable = 0;
		writel(Top->reg_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		size = sizeof(ive_dma_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMA;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF + sizeof(uint32_t) * i));
		}
		size = sizeof(ive_match_bg_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_match_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ive_update_bg_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_update_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ive_hist_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_hist_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].HIST + sizeof(uint32_t) * i));
		}
		size = sizeof(ive_map_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_map_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].MAP + sizeof(uint32_t) * i));
		}
		size = sizeof(ive_intg_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_intg_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].INTG + sizeof(uint32_t) * i));
		}
		size = sizeof(ive_sad_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_sad_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].SAD + sizeof(uint32_t) * i));
		}
		size = sizeof(ive_ncc_c) / sizeof(uint32_t);
		array = (uint32_t *)&ive_ncc_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].NCC + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF_RDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].HIST_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].INTG_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].SAD_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].SAD_WDMA_THR +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_1 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_2 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_3 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_4 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_1 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_2 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_3 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(isp_dma_ctl_c) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_4 +
					  sizeof(uint32_t) * i));
		}
	}
}

s32 clear_framedone(s32 status, s32 log, s32 dev_id)
{
	if (log)
		pr_info("framedone [%x]\n", status);
	if (status) {
		// ive_top_c.reg_90.val = status;
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90));
	}
	return status;
}

s32 clear_interrupt_status(s32 status, bool enlog, s32 dev_id)
{
	if (enlog)
		pr_info("interrupt status [%x]\n", status);
	if (status) {
		//level-trigger
		//ive_top_c->reg_98.val = status;
		//ive_top_c->reg_98.val = status;
#if defined(__CV186X__)
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98));
#else
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98));
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98));
#endif
	}
	return status;
}

inline s32 ive_go(struct ive_device *ndev, ive_top_c *ive_top_c,
			  bool instant, s32 done_mask, s32 optype, s32 dev_id)
{
	s32 cnt = 0;
	long leavetime = 0;
	s32 ret = SUCCESS;
	s32 int_mask = 0;
	ive_filterop_reg_h10_c reg_h10;
	ive_filterop_reg_h14_c reg_h14;
	ive_filterop_reg_28_c  reg_28;

	//for (i = 0; i < IVE_DEV_MAX; i++) {
	//	spin_lock(&ndev->core[i].dev_lock);
	//	if (atomic_read(&ndev->core[i].dev_state) == IVE_DEV_STATE_RUNNING) {
	//		dev_id = i;
	//	}
	//	spin_unlock(&ndev->core[dev_id].dev_lock);

		ndev->core[dev_id].cur_optype = optype;
		if (done_mask == 0)
			done_mask = IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK;

		if (optype == MOD_HIST) {
			int_mask = IVE_TOP_REG_INTR_STATUS_HIST_MASK;
			ive_top_c->reg_94.reg_intr_en_hist = !instant;
		} else if (optype == MOD_NCC) {
			int_mask = IVE_TOP_REG_INTR_STATUS_NCC_MASK;
			ive_top_c->reg_94.reg_intr_en_ncc = !instant;
		} else if (optype == MOD_INTEG) {
			int_mask = IVE_TOP_REG_INTR_STATUS_INTG_MASK;
			ive_top_c->reg_94.reg_intr_en_intg = !instant;
		} else if (optype == MOD_DMA) {
			int_mask = IVE_TOP_REG_INTR_STATUS_DMAF_MASK;
			ive_top_c->reg_94.reg_intr_en_dmaf = !instant;
		} else if (optype == MOD_GRADFG) {
			int_mask = IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_Y_MASK;
			ive_top_c->reg_94.reg_intr_en_filterop_wdma_y = !instant;
		} else if (optype == MOD_SAD) {
			int_mask = IVE_TOP_REG_INTR_STATUS_SAD_MASK;
			ive_top_c->reg_94.reg_intr_en_sad = !instant;
		} else if (optype == MOD_CCL){
			int_mask = IVE_TOP_REG_INTR_STATUS_CCL_MASK;
			ive_top_c->reg_94.reg_intr_en_ccl = !instant;
		} else {
			int_mask = IVE_TOP_REG_INTR_STATUS_FILTEROP_ODMA_MASK |
				   IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_Y_MASK |
				   IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_C_MASK;
			ive_top_c->reg_94.reg_intr_en_filterop_odma = !instant;
			ive_top_c->reg_94.reg_intr_en_filterop_wdma_y = !instant;
			ive_top_c->reg_94.reg_intr_en_filterop_wdma_c = !instant;
		}
		writel(ive_top_c->reg_94.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_94));

		reg_h10.val = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10);
		reg_h14.val = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14);
		reg_28.val = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28);

		g_debug_info.op[0].op_en = reg_h14.reg_filterop_sw_ovw_op;
		g_debug_info.op[0].op_sel = reg_h14.reg_filterop_op1_cmd;
		g_debug_info.op[1].op_en = reg_28.reg_filterop_op2_erodila_en;
		g_debug_info.op[1].op_sel = reg_h10.reg_filterop_mode;

		//if (optype != MOD_INTEG && optype != MOD_DMA && optype != MOD_GRADFG && optype != MOD_SAD){
		clear_framedone(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90), 0, dev_id);
		clear_interrupt_status(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98), 0, dev_id);
		//}
		// GoGoGo
		ive_dump_reg_state(false);
		//ive_dump_hw_flow();
		if (optype == MOD_DMA) {
			ive_top_c->reg_1.reg_fmt_vld_dmaf = 1;
		} else if (optype == MOD_CCL){
			ive_top_c->reg_1.reg_fmt_vld_fg = 1;
			ive_top_c->reg_1.reg_fmt_vld_ccl = 1;
		} else {
			ive_top_c->reg_1.reg_fmt_vld_fg = 1;
		}

		writel(ive_top_c->reg_1.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_1));
		ive_dump_reg_state(false);
		start_vld_time(optype, &ndev->core[dev_id]);
		if (instant) {
			TRACE_IVE(IVE_DBG_INFO, "instant is true\n");
			while (((readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90) &
				done_mask) != done_mask) && cnt < 10000) {
				usleep_range(10, 20);
				cnt++;
			}
			stop_vld_time(optype, ndev->core[dev_id].tile_num, &ndev->core[dev_id]);
			complete(&ndev->core[dev_id].frame_done);
			if (cnt >= 10000) {
				TRACE_IVE(IVE_DBG_ERR, "frame done timeout! tile = %d\n", ndev->core[dev_id].tile_num);
				ret = FAILURE;
			}
		} else {
				TRACE_IVE(IVE_DBG_INFO, "instant is false\n");
			    leavetime = wait_for_completion_timeout(
				&ndev->core[dev_id].frame_done, msecs_to_jiffies(10 * TIMEOUT_MS));

			reinit_completion(&ndev->core[dev_id].frame_done);
			if (leavetime <= 0) {
				TRACE_IVE(IVE_DBG_ERR, "frame done timeout! tile = %d\n", ndev->core[dev_id].tile_num);
				ret = FAILURE;
			}
		}
		if (ndev->core[dev_id].tile_num == ndev->core[dev_id].total_tile) {
			complete(&ndev->core[dev_id].op_done);
		}
	//}

	return ret;
}

s32 emit_bgm_tile(
	struct ive_device *ndev, bool enWdma_y, bool enOdma, s32 optype,
	ive_top_c *ive_top_c, ive_filterop_c *ive_filterop_c,
	img_in_c *img_in_c_f, ive_gmm_c *ive_gmm_c_f, isp_dma_ctl_c *wdma_y_ctl_c,
	u32 Dst0Stride, u32 width, u32 height,
	u32 dstEnType, u32 srcEnType, bool instant,
	ive_bg_stat_data_s *stat, isp_dma_ctl_c *gmm_mod_rdma_ctl_c[5],
	isp_dma_ctl_c *gmm_mod_wdma_ctl_c[5],
	isp_dma_ctl_c *gmm_match_wdma_ctl_c,
	isp_dma_ctl_c *gmm_factor_rdma_ctl_c, isp_dma_ctl_c *fgflag_rdma_ctl_c,
	isp_dma_ctl_c *bgmodel_0_rdma_ctl_c, isp_dma_ctl_c *difffg_wdma_ctl_c,
	isp_dma_ctl_c *bgmodel_1_rdma_ctl_c, isp_dma_ctl_c *fg_wdma_ctl_c,
	isp_dma_ctl_c *bgmodel_0_wdma_ctl_c,
	isp_dma_ctl_c *bgmodel_1_wdma_ctl_c, isp_dma_ctl_c *chg_wdma_ctl_c,
	ive_update_bg_c *ive_update_bg_c_f, s32 dev_id)
{
	u32 rdma_val = 0x201;
	s32 done_mask = 0;
	s32 tileNum = 1 + (width - 1) / 480;
	s32 remain_width = width;
	s32 tileLen[6] = { 0 };
	s32 segLen[6] = { 0 };
	s32 inOffset[6] = { 0 };
	s32 outOffset[6] = { 0 };
	s32 cropstart[6] = { 0 };
	s32 cropend[6] = { 0 };
	s32 i, round, n;
	//DEFINE_img_in_c(_img_in_c);
	img_in_c _img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_GMM_C(_ive_gmm_c);
	ive_gmm_c _ive_gmm_c = _DEFINE_IVE_GMM_C;
	//DEFINE_isp_dma_ctl_c(_wdma_y_ctl_c);
	isp_dma_ctl_c _wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_gmm_match_wdma_ctl_c);
	isp_dma_ctl_c _gmm_match_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_gmm_factor_rdma_ctl_c);
	isp_dma_ctl_c _gmm_factor_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_fgflag_rdma_ctl_c);
	isp_dma_ctl_c _fgflag_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_bgmodel_0_rdma_ctl_c);
	isp_dma_ctl_c _bgmodel_0_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_difffg_wdma_ctl_c);
	isp_dma_ctl_c _difffg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_bgmodel_1_rdma_ctl_c);
	isp_dma_ctl_c _bgmodel_1_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_fg_wdma_ctl_c);
	isp_dma_ctl_c _fg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_bgmodel_0_wdma_ctl_c);
	isp_dma_ctl_c _bgmodel_0_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_bgmodel_1_wdma_ctl_c);
	isp_dma_ctl_c _bgmodel_1_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_chg_wdma_ctl_c);
	isp_dma_ctl_c _chg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_UPDATE_BG_C(_ive_update_bg_c);
	ive_update_bg_c _ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;

	if (img_in_c_f == NULL) {
		img_in_c_f = &_img_in_c;
	}
	if (ive_gmm_c_f == NULL) {
		ive_gmm_c_f = &_ive_gmm_c;
	}
	if (wdma_y_ctl_c == NULL) {
		wdma_y_ctl_c = &_wdma_y_ctl_c;
	}
	if (gmm_match_wdma_ctl_c == NULL) {
		gmm_match_wdma_ctl_c = &_gmm_match_wdma_ctl_c;
	}
	if (gmm_factor_rdma_ctl_c == NULL) {
		gmm_factor_rdma_ctl_c = &_gmm_factor_rdma_ctl_c;
	}
	if (fgflag_rdma_ctl_c == NULL) {
		fgflag_rdma_ctl_c = &_fgflag_rdma_ctl_c;
	}
	if (bgmodel_0_rdma_ctl_c == NULL) {
		bgmodel_0_rdma_ctl_c = &_bgmodel_0_rdma_ctl_c;
	}
	if (difffg_wdma_ctl_c == NULL) {
		difffg_wdma_ctl_c = &_difffg_wdma_ctl_c;
	}
	if (bgmodel_1_rdma_ctl_c == NULL) {
		bgmodel_1_rdma_ctl_c = &_bgmodel_1_rdma_ctl_c;
	}
	if (fg_wdma_ctl_c == NULL) {
		fg_wdma_ctl_c = &_fg_wdma_ctl_c;
	}
	if (bgmodel_0_wdma_ctl_c == NULL) {
		bgmodel_0_wdma_ctl_c = &_bgmodel_0_wdma_ctl_c;
	}
	if (bgmodel_1_wdma_ctl_c == NULL) {
		bgmodel_1_wdma_ctl_c = &_bgmodel_1_wdma_ctl_c;
	}
	if (chg_wdma_ctl_c == NULL) {
		chg_wdma_ctl_c = &_chg_wdma_ctl_c;
	}
	if (ive_update_bg_c_f == NULL) {
		ive_update_bg_c_f = &_ive_update_bg_c;
	}

	if (tileNum > 6)
		return FAILURE;

	//for (dev_id = 0; dev_id < IVE_DEV_MAX; dev_id++) {
		if (atomic_read(&ndev->core[dev_id].dev_state) == IVE_CORE_STATE_RUNNING) {
			ndev->core[dev_id].total_tile = tileNum - 1;
			//Calcuate tileLen array
			for (n = 0; n < tileNum && remain_width > 0; n++) {
				if (remain_width > 480) {
					tileLen[n] = 480;
					remain_width -= 480;
					TRACE_IVE(IVE_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n, tileLen[n], tileLen[n]);
				} else {
					//in case last tile too short
					if (remain_width < 64) {
						tileLen[n] = remain_width + 64;
						tileLen[0] -= 64;
					} else {
						tileLen[n] = remain_width;
					}
					TRACE_IVE(IVE_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n,
							 tileLen[n], tileLen[n]);
					remain_width = 0;
				}
			}

			//Inference other array based on tileLen
			for (n = 0; n < tileNum; n++) {
				cropstart[n] = 0;
				cropend[n] = tileLen[n] - 1;
				segLen[n] = tileLen[n];
				outOffset[n] = (n == 0) ? 0 : segLen[n - 1];
				inOffset[n] = outOffset[n];
				TRACE_IVE(IVE_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n, tileLen[n], tileLen[n]);
				TRACE_IVE(IVE_DBG_INFO, "cropstart[%d]=%d;\ncropend[%d]=%d;\nsegLen[%d]=%d;\noutOffset[%d]=%d;\ninOffset[%d]=%d;\n",
								n,cropstart[n],n,cropend[n],n,segLen[n],n,outOffset[n],n,inOffset[n]);

			}


			bgmodel_0_rdma_ctl_c->sys_control.reg_stride_sel = 1;
			bgmodel_0_rdma_ctl_c->dma_stride.reg_stride = 16 * width_align(width, 16);
			bgmodel_1_rdma_ctl_c->sys_control.reg_stride_sel = 1;
			bgmodel_1_rdma_ctl_c->dma_stride.reg_stride = 8 * width_align(width, 16);
			fgflag_rdma_ctl_c->sys_control.reg_stride_sel = 1;
			fgflag_rdma_ctl_c->dma_stride.reg_stride = Dst0Stride;
			difffg_wdma_ctl_c->sys_control.reg_stride_sel = 1;
			difffg_wdma_ctl_c->dma_stride.reg_stride = 2 * width_align(width, 16);

			bgmodel_0_wdma_ctl_c->sys_control.reg_stride_sel = 1;
			bgmodel_0_wdma_ctl_c->dma_stride.reg_stride = 16 * width_align(width, 16);
			bgmodel_1_wdma_ctl_c->sys_control.reg_stride_sel = 1;
			bgmodel_1_wdma_ctl_c->dma_stride.reg_stride = 8 * width_align(width, 16);
			fg_wdma_ctl_c->sys_control.reg_stride_sel = 1;
			fg_wdma_ctl_c->dma_stride.reg_stride = Dst0Stride;
			chg_wdma_ctl_c->sys_control.reg_stride_sel = 1;
			chg_wdma_ctl_c->dma_stride.reg_stride = 4 * Dst0Stride;

			rdma_val = readl(IVE_BLK_BA[dev_id].RDMA);
			// Increase the number of times to read dram
			// reduce read/write switch, bit 16 =1, bit[15:8] = 'd4
			writel(0x10401, IVE_BLK_BA[dev_id].RDMA);

			writel(bgmodel_0_rdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_0_rdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(bgmodel_1_rdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_rdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(fgflag_rdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fgflag_rdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(difffg_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(difffg_wdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_DMA_STRIDE);

			writel(bgmodel_0_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_0_wdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(bgmodel_1_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_wdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(fg_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fg_wdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(chg_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(chg_wdma_ctl_c->dma_stride.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_DMA_STRIDE);

			if (optype == MOD_BGM || optype == MOD_BGU) {
				stat->pix_num = 0;
				stat->sum_lum = 0;
			}
			if (optype == MOD_BGM) {
				ive_update_bg_c_f->reg_crop_ctl.reg_crop_enable = 0;
				writel(ive_update_bg_c_f->reg_crop_ctl.val,
					   IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
						   IVE_UPDATE_BG_REG_CROP_CTL);
				enWdma_y = false;
				enOdma = false;
			} else if (optype == MOD_BGU) {
				if ((readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
					   IVE_UPDATE_BG_REG_CROP_S) &
					 IVE_UPDATE_BG_REG_CROP_END_X_MASK) > 0) {
					ive_update_bg_c_f->reg_crop_ctl.reg_crop_enable = 1;
					writel(ive_update_bg_c_f->reg_crop_ctl.val,
						   IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							   IVE_UPDATE_BG_REG_CROP_CTL);
					if (enWdma_y) {
						ive_filterop_c->reg_cropy_s.reg_crop_y_end_x =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_X) >>
							 IVE_UPDATE_BG_REG_CROP_END_X_OFFSET) &
							0xffff;
						ive_filterop_c->reg_cropy_e.reg_crop_y_end_y =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_Y) >>
							 IVE_UPDATE_BG_REG_CROP_END_Y_OFFSET) &
							0xffff;
						ive_filterop_c->reg_cropy_ctl.reg_crop_y_enable =
							0;
						writel(ive_filterop_c->reg_cropy_s.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROPY_S);
						writel(ive_filterop_c->reg_cropy_e.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROPY_E);
						writel(ive_filterop_c->reg_cropy_ctl.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROPY_CTL);
					}
					if (enOdma) {
						ive_filterop_c->reg_crop_odma_s
							.reg_crop_odma_start_x = 0;
						ive_filterop_c->reg_crop_odma_s
							.reg_crop_odma_end_x =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_X) >>
							 IVE_UPDATE_BG_REG_CROP_END_X_OFFSET) &
							0xffff;
						ive_filterop_c->reg_crop_odma_e
							.reg_crop_odma_start_y = 0;
						ive_filterop_c->reg_crop_odma_e
							.reg_crop_odma_end_y =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_Y) >>
							 IVE_UPDATE_BG_REG_CROP_END_Y_OFFSET) &
							0xffff;
						ive_filterop_c->reg_crop_odma_ctl
							.reg_crop_odma_enable = 0;
						writel(ive_filterop_c->reg_crop_odma_s.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROP_ODMA_S);
						writel(ive_filterop_c->reg_crop_odma_e.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROP_ODMA_E);
						writel(ive_filterop_c->reg_crop_odma_ctl.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROP_ODMA_CTL);
					}
				} else {
					ive_update_bg_c_f->reg_crop_ctl.reg_crop_enable = 0;
					ive_filterop_c->reg_cropy_ctl.reg_crop_y_enable = 0;
					ive_filterop_c->reg_crop_odma_ctl.reg_crop_odma_enable =
						0;
					writel(ive_update_bg_c_f->reg_crop_ctl.val,
						   IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							   IVE_UPDATE_BG_REG_CROP_CTL);
					writel(ive_filterop_c->reg_cropy_ctl.val,
						   IVE_BLK_BA[dev_id].FILTEROP +
							   IVE_FILTEROP_REG_CROPY_CTL);
					writel(ive_filterop_c->reg_crop_odma_ctl.val,
						   IVE_BLK_BA[dev_id].FILTEROP +
							   IVE_FILTEROP_REG_CROP_ODMA_CTL);
				}
				// ive_update_bg_print(ive_update_bg_c_f);
			}

			for (round = 0; round < tileNum; round++) {
				ndev->core[dev_id].tile_num = round;
				ive_top_c->reg_2.reg_img_widthm1 = tileLen[round] - 1;
				img_in_c_f->reg_02.reg_src_wd = tileLen[round] - 1;
				img_in_c_f->reg_y_base_0.reg_src_y_base_b0 += inOffset[round];
				writel(ive_top_c->reg_2.val,
					   IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_2);
				writel(img_in_c_f->reg_02.val, IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02);
				writel(img_in_c_f->reg_y_base_0.val,
					   IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_0);
				if (enWdma_y) {
					wdma_y_ctl_c->base_addr.reg_basel += outOffset[round];
					wdma_y_ctl_c->sys_control.reg_stride_sel = 1;
					wdma_y_ctl_c->sys_control.reg_seglen_sel = 0;
					wdma_y_ctl_c->dma_seglen.reg_seglen = 0;
					wdma_y_ctl_c->dma_stride.reg_stride = Dst0Stride;

					ive_filterop_c->reg_cropy_s.reg_crop_y_start_x =
						cropstart[round];
					ive_filterop_c->reg_cropy_s.reg_crop_y_end_x =
						cropend[round];
					ive_filterop_c->reg_cropy_e.reg_crop_y_start_y = 0;
					ive_filterop_c->reg_cropy_e.reg_crop_y_end_y =
						height - 1;
					ive_filterop_c->reg_cropy_ctl.reg_crop_y_enable = 0;

					writel(wdma_y_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_BASE_ADDR));
					writel(wdma_y_ctl_c->sys_control.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(wdma_y_ctl_c->dma_seglen.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(wdma_y_ctl_c->dma_stride.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_STRIDE));
					writel(ive_filterop_c->reg_cropy_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_S));
					writel(ive_filterop_c->reg_cropy_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_E));
					writel(ive_filterop_c->reg_cropy_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_CTL));
				}
				if (enOdma) {
					ive_filterop_c->odma_reg_11.reg_dma_wd =
						segLen[round] - 1;
					writel(ive_filterop_c->odma_reg_11.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_11));
					switch (dstEnType) {
					case IVE_IMAGE_TYPE_YUV420P:
					case IVE_IMAGE_TYPE_YUV422P:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_03
							.reg_dma_u_base_low_part +=
							outOffset[round] / 2;
						ive_filterop_c->odma_reg_05
							.reg_dma_v_base_low_part +=
							outOffset[round] / 2;
						break;
					case IVE_IMAGE_TYPE_YUV420SP: // NV21
					case IVE_IMAGE_TYPE_YUV422SP:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						img_in_c_f->reg_u_base_0.reg_src_u_base_b0 +=
							inOffset[round];
						img_in_c_f->reg_00.reg_auto_csc_en = 0;
						writel(img_in_c_f->reg_u_base_0.val,
							   (IVE_BLK_BA[dev_id].IMG_IN +
							IMG_IN_REG_U_BASE_0));
						writel(img_in_c_f->reg_00.val,
							   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));
						break;
					case IVE_IMAGE_TYPE_U8C3_PLANAR:
						ive_filterop_c->odma_reg_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_05
							.reg_dma_v_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C1:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C3_PACKAGE:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round] * 3;
						img_in_c_f->reg_y_base_0.reg_src_y_base_b0 +=
							inOffset[round] * 2;
						writel(img_in_c_f->reg_y_base_0.val,
							   (IVE_BLK_BA[dev_id].IMG_IN +
							IMG_IN_REG_Y_BASE_0));
						break;
					default:
						break;
					}
					writel(ive_filterop_c->odma_reg_01.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_01));
					writel(ive_filterop_c->odma_reg_03.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_03));
					writel(ive_filterop_c->odma_reg_05.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_05));

					if (ive_update_bg_c_f->reg_crop_ctl.reg_crop_enable) {
						ive_filterop_c->odma_reg_11.reg_dma_wd =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_X) >>
							 IVE_UPDATE_BG_REG_CROP_END_X_OFFSET) &
							0xffff;
						writel(ive_filterop_c->odma_reg_11.val,
							   (IVE_BLK_BA[dev_id].FILTEROP +
							IVE_FILTEROP_ODMA_REG_11));
					}
				}

				if (optype == MOD_GMM || optype == MOD_GMM2) {
					ndev->cur_optype = optype;
					for (i = 0; i < 5; i++) {
						u32 u32ModelSize =
							(srcEnType == IVE_IMAGE_TYPE_U8C1) ? 8 :
												 12;

						if (i < ive_gmm_c_f->reg_gmm_13
								.reg_gmm_gmm2_model_num) {
							gmm_mod_rdma_ctl_c[i]
								->base_addr.reg_basel +=
								u32ModelSize * inOffset[round];
							gmm_mod_rdma_ctl_c[i]
								->dma_stride.reg_stride =
								u32ModelSize * width_align(width, 16);
							gmm_mod_rdma_ctl_c[i]
								->sys_control.reg_stride_sel = 1;

							gmm_mod_wdma_ctl_c[i]
								->base_addr.reg_basel +=
								u32ModelSize *
								outOffset[round]; //!?
							gmm_mod_wdma_ctl_c[i]
								->dma_stride.reg_stride =
								u32ModelSize * width_align(width, 16);
							gmm_mod_wdma_ctl_c[i]
								->sys_control.reg_stride_sel = 1;

							writel(gmm_mod_rdma_ctl_c[i]
									   ->base_addr.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_BASE_ADDR));
							writel(gmm_mod_rdma_ctl_c[i]
									   ->dma_stride.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_DMA_STRIDE));
							writel(gmm_mod_rdma_ctl_c[i]
									   ->sys_control.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_SYS_CONTROL));
							writel(gmm_mod_wdma_ctl_c[i]
									   ->base_addr.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_BASE_ADDR));
							writel(gmm_mod_wdma_ctl_c[i]
									   ->dma_stride.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_DMA_STRIDE));
							writel(gmm_mod_wdma_ctl_c[i]
									   ->sys_control.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_SYS_CONTROL));
						}
					}
					if (ive_gmm_c_f->reg_gmm_13.reg_gmm_gmm2_enable > 1) {
						gmm_factor_rdma_ctl_c->base_addr.reg_basel +=
							inOffset[round] * 2;
						gmm_factor_rdma_ctl_c->sys_control
							.reg_stride_sel = 1;
						//
						// gmm_factor_rdma_ctl_c->dma_stride.reg_stride =
						// 	width * 2;
						gmm_match_wdma_ctl_c->base_addr.reg_basel +=
							outOffset[round];
						//
						// gmm_match_wdma_ctl_c->dma_stride.reg_stride =
						// 	width;
						gmm_match_wdma_ctl_c->sys_control
							.reg_stride_sel = 1;
						writel(gmm_factor_rdma_ctl_c->base_addr.val,
							   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
							ISP_DMA_CTL_BASE_ADDR));
						writel(gmm_factor_rdma_ctl_c->sys_control.val,
							   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
							ISP_DMA_CTL_SYS_CONTROL));
						writel(gmm_factor_rdma_ctl_c->dma_stride.val,
							   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
							ISP_DMA_CTL_DMA_STRIDE));
						writel(gmm_match_wdma_ctl_c->base_addr.val,
							   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
							ISP_DMA_CTL_BASE_ADDR));
						writel(gmm_match_wdma_ctl_c->dma_stride.val,
							   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
							ISP_DMA_CTL_DMA_STRIDE));
						writel(gmm_match_wdma_ctl_c->sys_control.val,
							   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
							ISP_DMA_CTL_SYS_CONTROL));
					}
				} else if (optype == MOD_BGM || optype == MOD_BGU) {
					bgmodel_0_rdma_ctl_c->base_addr.reg_basel +=
						inOffset[round] * 16;
					bgmodel_1_rdma_ctl_c->base_addr.reg_basel +=
						inOffset[round] * 8;
					fgflag_rdma_ctl_c->base_addr.reg_basel +=
						inOffset[round];

					bgmodel_0_wdma_ctl_c->base_addr.reg_basel +=
						outOffset[round] * 16;
					bgmodel_1_wdma_ctl_c->base_addr.reg_basel +=
						outOffset[round] * 8;
					fg_wdma_ctl_c->base_addr.reg_basel += outOffset[round];
					writel(bgmodel_0_rdma_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(bgmodel_1_rdma_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(fgflag_rdma_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(bgmodel_0_wdma_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(bgmodel_1_wdma_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(fg_wdma_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA +
						ISP_DMA_CTL_BASE_ADDR));

					if (optype == MOD_BGM) {
						difffg_wdma_ctl_c->base_addr.reg_basel +=
							outOffset[round] * 2;
						writel(difffg_wdma_ctl_c->base_addr.val,
							   (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA +
							ISP_DMA_CTL_BASE_ADDR));
					}
					if (optype == MOD_BGU) {
						chg_wdma_ctl_c->base_addr.reg_basel +=
							outOffset[round] * 4;
						writel(chg_wdma_ctl_c->base_addr.val,
							   (IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA +
							ISP_DMA_CTL_BASE_ADDR));
					}
				}

				if (enOdma) {
					done_mask |= IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK;
				}
				if (optype == MOD_BGM) {
					done_mask |= IVE_TOP_REG_FRAME_DONE_BGM_MASK;
				} else if (optype == MOD_BGU) {
					done_mask |= IVE_TOP_REG_FRAME_DONE_BGU_MASK;
				} else if (optype == MOD_GMM || optype == MOD_GMM2) {
					done_mask |= IVE_TOP_REG_FRAME_DONE_GMM_MASK;
				}

				ive_go(ndev, ive_top_c, instant, done_mask, optype, dev_id);
				// clear after a tile
				clear_framedone(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90), 0, dev_id);
				clear_interrupt_status(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98), 0, dev_id);


				if (optype == MOD_BGM) {
					stat->pix_num +=
						readl(IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
							  IVE_MATCH_BG_REG_10); //9c2fe24
					stat->sum_lum +=
						readl(IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
							  IVE_MATCH_BG_REG_14); //9c2fe24
				} else if (optype == MOD_BGU) {
					stat->pix_num +=
						readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							  IVE_UPDATE_BG_REG_CTRL7); //9c2fe24
					stat->sum_lum +=
						readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							  IVE_UPDATE_BG_REG_CTRL8); //9c2fe24
				}
			}

			if (optype == MOD_GMM || optype == MOD_GMM2) {
				_ive_reset(ndev, 0, dev_id);
			}
			// add read times, for reduce r/w switch.
			writel(rdma_val, IVE_BLK_BA[dev_id].RDMA);

			if (optype == MOD_GMM || optype == MOD_GMM2) {
				for (i = 0; i < ive_gmm_c_f->reg_gmm_13.reg_gmm_gmm2_model_num;
					 i++) {
					gmm_mod_rdma_ctl_c[i]->sys_control.reg_stride_sel = 0;
					gmm_mod_wdma_ctl_c[i]->sys_control.reg_stride_sel = 0;
					writel(gmm_mod_wdma_ctl_c[i]->sys_control.val,
						   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(gmm_mod_rdma_ctl_c[i]->sys_control.val,
						   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
						ISP_DMA_CTL_SYS_CONTROL));
				}
			}
			gmm_factor_rdma_ctl_c->sys_control.reg_stride_sel = 0;
			gmm_match_wdma_ctl_c->sys_control.reg_stride_sel = 0;
			bgmodel_0_rdma_ctl_c->sys_control.reg_stride_sel = 0;
			bgmodel_1_rdma_ctl_c->sys_control.reg_stride_sel = 0;
			fgflag_rdma_ctl_c->sys_control.reg_stride_sel = 0;
			difffg_wdma_ctl_c->sys_control.reg_stride_sel = 0;
			bgmodel_0_wdma_ctl_c->sys_control.reg_stride_sel = 0;
			bgmodel_1_wdma_ctl_c->sys_control.reg_stride_sel = 0;
			fg_wdma_ctl_c->sys_control.reg_stride_sel = 0;
			chg_wdma_ctl_c->sys_control.reg_stride_sel = 0;
			wdma_y_ctl_c->sys_control.reg_stride_sel = 0;
			ive_gmm_c_f->reg_gmm_13.reg_gmm_gmm2_enable = 0;

			writel(gmm_factor_rdma_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_SYS_CONTROL));
			writel(gmm_match_wdma_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_SYS_CONTROL));
			writel(bgmodel_0_rdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_rdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fgflag_rdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(difffg_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_0_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fg_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(chg_wdma_ctl_c->sys_control.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(wdma_y_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));

			writel(ive_gmm_c_f->reg_gmm_13.val,
				   (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		}
	//}

	return SUCCESS;
}

s32 emit_tile(struct ive_device *ndev, ive_top_c *ive_top_c,
		 ive_filterop_c *ive_filterop_c, img_in_c *img_in_c_f,
		 isp_dma_ctl_c *wdma_y_ctl_c, isp_dma_ctl_c *rdma_img1_ctl_c,
		 isp_dma_ctl_c *wdma_c_ctl_c, isp_dma_ctl_c *rdma_eigval_ctl_c,
		 ive_image_s *src1, ive_image_s *src2, ive_image_s *src3,
		 ive_image_s *dst1, ive_image_s *dst2, bool enWdma_y,
		 s32 y_unit, bool enWdma_c, s32 c_unit, bool enOdma,
		 s32 optype, bool instant, s32 dev_id)
{
	/*
	 * Tile x2 Width: 496~ 928 = 480*n-32*(n-1)
	 * Tile x3 Width: 944~1376
	 * Tile x4 Width:1392~1824
	 * Tile x5 Width:1840~2272
	 */
	s32 img1_unit;
	s32 n = 0, round = 0, done_mask = 0;

	s32 tileNum = (src1->width - 32 + 447) / 448;
	s32 remain_width = src1->width + 32 * (tileNum - 1);
	s32 tileLen[6] = { 0 };
	s32 segLen[6] = { 0 };
	s32 inOffset[6] = { 0 };
	s32 outOffset[6] = { 0 };
	s32 cropstart[6] = { 0 };
	s32 cropend[6] = { 0 };
	bool isCanny = (optype == MOD_CANNY);
	ive_map_c *ive_map_c_f;
	isp_dma_ctl_c *rdma_gradfg_ctl_c;
	//DEFINE_img_in_c(_img_in_c);
	img_in_c _img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_MAP_C(_ive_map_c);
	ive_map_c _ive_map_c = _DEFINE_IVE_MAP_C;
	//DEFINE_isp_dma_ctl_c(_wdma_y_ctl_c);
	isp_dma_ctl_c _wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_wdma_c_ctl_c);
	isp_dma_ctl_c _wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_rdma_img1_ctl_c);
	isp_dma_ctl_c _rdma_img1_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_rdma_eigval_ctl_c);
	isp_dma_ctl_c _rdma_eigval_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(_rdma_gradfg_ctl_c);
	isp_dma_ctl_c _rdma_gradfg_ctl_c = _DEFINE_isp_dma_ctl_c;

	ive_map_c_f = &_ive_map_c;
	rdma_gradfg_ctl_c = &_rdma_gradfg_ctl_c;

	if (tileNum > 6)
		return FAILURE;

	//for (dev_id = 0; dev_id < IVE_DEV_MAX; dev_id++) {
		if (atomic_read(&ndev->core[dev_id].dev_state) == IVE_CORE_STATE_RUNNING) {
			ndev->core[dev_id].total_tile = tileNum - 1;

			if (img_in_c_f == NULL) {
				img_in_c_f = &_img_in_c;
			}

			if (wdma_y_ctl_c == NULL) {
				wdma_y_ctl_c = &_wdma_y_ctl_c;
			}

			if (rdma_img1_ctl_c == NULL) {
				rdma_img1_ctl_c = &_rdma_img1_ctl_c;
			}

			if (wdma_c_ctl_c == NULL) {
				wdma_c_ctl_c = &_wdma_c_ctl_c;
			}

			if (rdma_eigval_ctl_c == NULL) {
				rdma_eigval_ctl_c = &_rdma_eigval_ctl_c;
			}

			//Calcuate tileLen array
			for (n = 0; n < tileNum; n++) {
				if (remain_width > 480) {
					tileLen[n] = 480;
					remain_width -= 480;
					TRACE_IVE(IVE_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n, tileLen[n], tileLen[n]);
				} else {
					//in case last tile too short
					if (remain_width < 64) {
						tileLen[n] = 128;
						tileLen[0] -= (128 - remain_width);
					} else {
						tileLen[n] = remain_width;
					}
					TRACE_IVE(IVE_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n,
							 tileLen[n], tileLen[n]);
					break;
				}
			}

			//Inference other array based on tileLen
			for (n = 0; n < tileNum; n++) {
				s32 nFirst_tile = (n == 0) ? 0 : 1;
				s32 nLast_tile = (n == tileNum - 1) ? 0 : 1;

				TRACE_IVE(IVE_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n, tileLen[n], tileLen[n]);
				cropstart[n] = 16 * nFirst_tile;
				cropend[n] = tileLen[n] - 16 * nLast_tile - 1;
				segLen[n] = tileLen[n] - 16 * nFirst_tile - 16 * nLast_tile;
				outOffset[n] = (n == 0) ? 0 : segLen[n - 1];
				inOffset[n] = (n == 1) ? outOffset[n] - 16 : outOffset[n];
				TRACE_IVE(IVE_DBG_INFO, "cropstart[%d]=%d;\ncropend[%d]=%d;\nsegLen[%d]=%d;\noutOffset[%d]=%d;\ninOffset[%d]=%d;\n",
								n,cropstart[n],n,cropend[n],n,segLen[n],n,outOffset[n],n,inOffset[n]);
			}

			if (optype == MOD_STBOX) {
				ive_filterop_c->reg_st_eigval_0.reg_st_eigval_tile_num =
					tileNum - 1;
				writel(ive_filterop_c->reg_st_eigval_0.val,
					   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_0));
			}

			for (round = 0; round < tileNum; round++) {
				ndev->core[dev_id].tile_num = round;
				ive_top_c->reg_2.reg_img_widthm1 = tileLen[round] - 1;
				img_in_c_f->reg_02.reg_src_wd = tileLen[round] - 1;
				if (src1->type == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
					img_in_c_f->reg_y_base_0.reg_src_y_base_b0 += inOffset[round] * 3;
				} else if (src1->type == IVE_IMAGE_TYPE_YUV420SP) {
					img_in_c_f->reg_y_base_0.reg_src_y_base_b0 += inOffset[round];
					img_in_c_f->reg_u_base_0.reg_src_u_base_b0 += inOffset[round];
				} else {
					img_in_c_f->reg_y_base_0.reg_src_y_base_b0 += inOffset[round];

				}
				writel(ive_top_c->reg_2.val,
					   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_2));
				writel(img_in_c_f->reg_02.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02));
				writel(img_in_c_f->reg_y_base_0.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_0));
				writel(img_in_c_f->reg_u_base_0.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
				writel(img_in_c_f->reg_v_base_0.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
				if (enWdma_y) {
					wdma_y_ctl_c->base_addr.reg_basel +=
						y_unit * outOffset[round];
					wdma_y_ctl_c->sys_control.reg_stride_sel = 1;
					wdma_y_ctl_c->sys_control.reg_seglen_sel = 1;
					wdma_y_ctl_c->sys_control.reg_segnum_sel = 1;
					wdma_y_ctl_c->dma_segnum.reg_segnum = src1->height;
					wdma_y_ctl_c->dma_seglen.reg_seglen =
						segLen[round] * y_unit;
					// wdma_y_ctl_c->dma_stride.reg_stride =
					// 		dst1->stride[0] * y_unit;
					wdma_y_ctl_c->dma_stride.reg_stride =
							dst1->stride[0];

						// wdma_y_ctl_c->dma_stride.reg_stride =
						//	(optype == MOD_STBOX) ?
						//		dst1->stride[0] * y_unit :
						//		dst1->stride[0];

					ive_filterop_c->reg_cropy_s.reg_crop_y_start_x =
						cropstart[round];
					ive_filterop_c->reg_cropy_s.reg_crop_y_end_x =
						cropend[round];
					ive_filterop_c->reg_cropy_e.reg_crop_y_start_y = 0;
					ive_filterop_c->reg_cropy_e.reg_crop_y_end_y =
						src1->height - 1;
					ive_filterop_c->reg_cropy_ctl.reg_crop_y_enable = 1;

					writel(wdma_y_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_BASE_ADDR));
					writel(wdma_y_ctl_c->sys_control.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(wdma_y_ctl_c->dma_segnum.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(wdma_y_ctl_c->dma_seglen.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(wdma_y_ctl_c->dma_stride.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_STRIDE));

					writel(ive_filterop_c->reg_cropy_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_E));
					writel(ive_filterop_c->reg_cropy_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_S));
					writel(ive_filterop_c->reg_cropy_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_CTL));
				}
				if (enWdma_c) {
					if (isCanny) {
						//It is only for example, still have no good idea to caculate this...
						wdma_c_ctl_c->base_addr.reg_basel +=
							(round == 0) ? 0 : 0x12000;
						wdma_c_ctl_c->sys_control.reg_stride_sel = 0;
						wdma_c_ctl_c->sys_control.reg_seglen_sel = 0;
						wdma_c_ctl_c->sys_control.reg_segnum_sel = 0;
						wdma_c_ctl_c->dma_segnum.reg_segnum =
							src1->height;
						wdma_c_ctl_c->dma_seglen.reg_seglen =
							segLen[round] * c_unit;
						wdma_c_ctl_c->dma_stride.reg_stride = 0;
					} else {
						wdma_c_ctl_c->base_addr.reg_basel +=
							c_unit * outOffset[round];
						wdma_c_ctl_c->sys_control.reg_stride_sel = 1;
						wdma_c_ctl_c->sys_control.reg_seglen_sel = 1;
						wdma_c_ctl_c->sys_control.reg_segnum_sel = 1;
						wdma_c_ctl_c->dma_segnum.reg_segnum =
							src1->height;
						wdma_c_ctl_c->dma_seglen.reg_seglen =
							segLen[round] * c_unit;
						// wdma_c_ctl_c->dma_stride.reg_stride =
						//	dst1->stride[0] * c_unit;
						if (optype == MOD_MAP) {
							wdma_c_ctl_c->dma_stride.reg_stride =
								dst1->stride[0]; // * c_unit;
						} else {
							wdma_c_ctl_c->dma_stride.reg_stride =
								dst2->stride[0]; // * c_unit;

						}
					}
					ive_filterop_c->reg_cropc_s.reg_crop_c_start_x =
						cropstart[round];
					ive_filterop_c->reg_cropc_s.reg_crop_c_end_x =
						cropend[round];
					ive_filterop_c->reg_cropc_e.reg_crop_c_start_y = 0;
					ive_filterop_c->reg_cropc_e.reg_crop_c_end_y =
						src1->height - 1;
					ive_filterop_c->reg_cropc_ctl.reg_crop_c_enable = 1;
					writel(wdma_c_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_BASE_ADDR));
					writel(wdma_c_ctl_c->sys_control.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(wdma_c_ctl_c->dma_segnum.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(wdma_c_ctl_c->dma_seglen.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(wdma_c_ctl_c->dma_stride.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_DMA_STRIDE));
					writel(ive_filterop_c->reg_cropc_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPC_S));
					writel(ive_filterop_c->reg_cropc_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPC_E));
					writel(ive_filterop_c->reg_cropc_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPC_CTL));
				}
				if (enOdma) {
					ive_filterop_c->odma_reg_11.reg_dma_wd =
						segLen[round] - 1;
					writel(ive_filterop_c->odma_reg_11.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_11));

					switch (dst1->type) {
					case IVE_IMAGE_TYPE_YUV420P:
					case IVE_IMAGE_TYPE_YUV422P:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_03
							.reg_dma_u_base_low_part +=
							outOffset[round] / 2;
						ive_filterop_c->odma_reg_05
							.reg_dma_v_base_low_part +=
							outOffset[round] / 2;
						break;
					case IVE_IMAGE_TYPE_YUV420SP: // NV21
					case IVE_IMAGE_TYPE_YUV422SP:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						img_in_c_f->reg_u_base_0.reg_src_u_base_b0 +=
							inOffset[round];
						img_in_c_f->reg_00.reg_auto_csc_en = 0;
						writel(img_in_c_f->reg_u_base_0.val,
							   (IVE_BLK_BA[dev_id].IMG_IN +
							IMG_IN_REG_U_BASE_0));
						writel(img_in_c_f->reg_00.val,
							   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));
						break;
					case IVE_IMAGE_TYPE_U8C3_PLANAR:
						ive_filterop_c->odma_reg_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_05
							.reg_dma_v_base_low_part +=
							outOffset[round];
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C1:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C3_PACKAGE:
						ive_filterop_c->odma_reg_01
							.reg_dma_y_base_low_part +=
							outOffset[round] * 3;
						// img_in_c->reg_y_base_0.reg_src_y_base_b0 +=
						// 	inOffset[round];
						// writel(img_in_c->reg_y_base_0.val,
						// 	   (IVE_BLK_BA[dev_id].IMG_IN +
						// 	IMG_IN_REG_Y_BASE_0));
						break;
					default:
						break;
					}
					ive_filterop_c->reg_crop_odma_s.reg_crop_odma_start_x =
						cropstart[round];
					ive_filterop_c->reg_crop_odma_s.reg_crop_odma_end_x =
						cropend[round];
					ive_filterop_c->reg_crop_odma_e.reg_crop_odma_start_y =
						0;
					ive_filterop_c->reg_crop_odma_e.reg_crop_odma_end_y =
						src1->height - 1;
					ive_filterop_c->reg_crop_odma_ctl.reg_crop_odma_enable =
						1;

					writel(ive_filterop_c->odma_reg_01.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_01));
					writel(ive_filterop_c->odma_reg_03.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_03));
					writel(ive_filterop_c->odma_reg_05.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_05));
					writel(ive_filterop_c->reg_crop_odma_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROP_ODMA_S));
					writel(ive_filterop_c->reg_crop_odma_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROP_ODMA_E));
					writel(ive_filterop_c->reg_crop_odma_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROP_ODMA_CTL));
				}
				if ((readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3) &
					 IVE_TOP_REG_IVE_RDMA_IMG1_EN_MASK)) {
					img1_unit = (optype == MOD_GRADFG) ? 2 : 1;
					rdma_img1_ctl_c->base_addr.reg_basel +=
						inOffset[round] * img1_unit;
					rdma_img1_ctl_c->sys_control.reg_stride_sel = 1;
					rdma_img1_ctl_c->sys_control.reg_seglen_sel = 1;
					rdma_img1_ctl_c->sys_control.reg_segnum_sel = 1;
					rdma_img1_ctl_c->dma_segnum.reg_segnum =
						src2->height;
					rdma_img1_ctl_c->dma_seglen.reg_seglen =
						tileLen[round] * img1_unit;
					rdma_img1_ctl_c->dma_stride.reg_stride =
						src2->stride[0]; // * img1_unit;
						// src2->stride[0] * img1_unit;
					writel(rdma_img1_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR));
					writel(rdma_img1_ctl_c->sys_control.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(rdma_img1_ctl_c->dma_segnum.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGNUM));
					writel(rdma_img1_ctl_c->dma_seglen.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGLEN));
					writel(rdma_img1_ctl_c->dma_stride.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_STRIDE));
				}
				if ((readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3) &
					 IVE_TOP_REG_IVE_RDMA_EIGVAL_EN_MASK)) {
					rdma_eigval_ctl_c->base_addr.reg_basel +=
						inOffset[round] * 2;
					rdma_eigval_ctl_c->sys_control.reg_stride_sel = 1;
					rdma_eigval_ctl_c->sys_control.reg_seglen_sel = 1;
					rdma_eigval_ctl_c->sys_control.reg_segnum_sel = 1;
					rdma_eigval_ctl_c->dma_segnum.reg_segnum =
						src1->height;
					rdma_eigval_ctl_c->dma_seglen.reg_seglen =
						tileLen[round] * 2;
					rdma_eigval_ctl_c->dma_stride.reg_stride =
						src1->stride[0];

					writel(rdma_eigval_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_BASE_ADDR));
					writel(rdma_eigval_ctl_c->sys_control.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(rdma_eigval_ctl_c->dma_segnum.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(rdma_eigval_ctl_c->dma_seglen.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(rdma_eigval_ctl_c->dma_stride.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_DMA_STRIDE));
				}
				if ((readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04) &
					 IVE_FILTEROP_REG_GRADFG_BGGRAD_RDMA_EN_MASK)) {
					rdma_gradfg_ctl_c->base_addr.reg_basel +=
						inOffset[round] * 2;
					rdma_gradfg_ctl_c->sys_control.reg_stride_sel = 1;
					rdma_gradfg_ctl_c->sys_control.reg_seglen_sel = 1;
					rdma_gradfg_ctl_c->sys_control.reg_segnum_sel = 1;
					rdma_gradfg_ctl_c->dma_segnum.reg_segnum =
						src3->height;
					rdma_gradfg_ctl_c->dma_seglen.reg_seglen =
						tileLen[round] * 2;
					rdma_gradfg_ctl_c->dma_stride.reg_stride =
						src3->stride[0];

					writel(rdma_gradfg_ctl_c->base_addr.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(rdma_gradfg_ctl_c->sys_control.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(rdma_gradfg_ctl_c->dma_segnum.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(rdma_gradfg_ctl_c->dma_seglen.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(rdma_gradfg_ctl_c->dma_stride.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_DMA_STRIDE));
				}

				done_mask =
					(enWdma_y ?
						 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK :
						 0) |
					(enWdma_c ?
						 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK &&
							 !isCanny :
						 0) |
					(enOdma ? IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK :
						  0);

				ive_go(ndev, ive_top_c, instant, done_mask, optype, dev_id);

				// clear after a tile
				clear_framedone(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90), 0, dev_id);
				clear_interrupt_status(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98), 0, dev_id);

			}

			img_in_c_f->reg_068.reg_ip_clr_w1t = 1;
			writel(img_in_c_f->reg_068.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
			// ive_filterop_c->reg_1.val = 0xff;
			//udelay(3);
			wdma_y_ctl_c->base_addr.reg_basel = 0;
			wdma_y_ctl_c->sys_control.reg_base_sel = 0;
			wdma_y_ctl_c->sys_control.reg_stride_sel = 0;
			wdma_y_ctl_c->sys_control.reg_seglen_sel = 0;
			wdma_y_ctl_c->sys_control.reg_segnum_sel = 0;
			wdma_y_ctl_c->dma_segnum.reg_segnum = 0;
			wdma_y_ctl_c->dma_seglen.reg_seglen = 0;
			wdma_y_ctl_c->dma_stride.reg_stride = 0;
			wdma_c_ctl_c->base_addr.reg_basel = 0;
			wdma_c_ctl_c->sys_control.reg_base_sel = 0;
			wdma_c_ctl_c->sys_control.reg_stride_sel = 0;
			wdma_c_ctl_c->sys_control.reg_seglen_sel = 0;
			wdma_c_ctl_c->sys_control.reg_segnum_sel = 0;
			wdma_c_ctl_c->dma_segnum.reg_segnum = 0;
			wdma_c_ctl_c->dma_seglen.reg_seglen = 0;
			wdma_c_ctl_c->dma_stride.reg_stride = 0;
			ive_filterop_c->reg_cropy_s.reg_crop_y_start_x = 0;
			ive_filterop_c->reg_cropy_s.reg_crop_y_end_x = 0;
			ive_filterop_c->reg_cropy_e.reg_crop_y_start_y = 0;
			ive_filterop_c->reg_cropy_e.reg_crop_y_end_y = 0;
			ive_filterop_c->reg_cropy_ctl.reg_crop_y_enable = 0;
			ive_filterop_c->reg_cropc_s.reg_crop_c_start_x = 0;
			ive_filterop_c->reg_cropc_s.reg_crop_c_end_x = 0;
			ive_filterop_c->reg_cropc_e.reg_crop_c_start_y = 0;
			ive_filterop_c->reg_cropc_e.reg_crop_c_end_y = 0;
			ive_filterop_c->reg_cropc_ctl.reg_crop_c_enable = 0;
			ive_filterop_c->reg_crop_odma_s.reg_crop_odma_start_x = 0;
			ive_filterop_c->reg_crop_odma_s.reg_crop_odma_end_x = 0;
			ive_filterop_c->reg_crop_odma_e.reg_crop_odma_start_y = 0;
			ive_filterop_c->reg_crop_odma_e.reg_crop_odma_end_y = 0;
			ive_filterop_c->reg_crop_odma_ctl.reg_crop_odma_enable = 0;
			ive_filterop_c->odma_reg_01.reg_dma_y_base_low_part = 0;
			ive_filterop_c->odma_reg_02.reg_dma_y_base_high_part = 0;
			ive_filterop_c->odma_reg_03.reg_dma_u_base_low_part = 0;
			ive_filterop_c->odma_reg_04.reg_dma_u_base_high_part = 0;
			ive_filterop_c->odma_reg_05.reg_dma_v_base_low_part = 0;
			ive_filterop_c->odma_reg_06.reg_dma_v_base_high_part = 0;
			ive_filterop_c->odma_reg_07.reg_dma_y_pitch = 0;
			ive_filterop_c->odma_reg_08.reg_dma_c_pitch = 0;
			rdma_eigval_ctl_c->base_addr.reg_basel = 0;
			rdma_eigval_ctl_c->sys_control.reg_base_sel = 0;
			rdma_eigval_ctl_c->sys_control.reg_stride_sel = 0;
			rdma_eigval_ctl_c->sys_control.reg_seglen_sel = 0;
			rdma_eigval_ctl_c->sys_control.reg_segnum_sel = 0;
			rdma_eigval_ctl_c->dma_segnum.reg_segnum = 0;
			rdma_eigval_ctl_c->dma_seglen.reg_seglen = 0;
			rdma_eigval_ctl_c->dma_stride.reg_stride = 0;
			rdma_img1_ctl_c->base_addr.reg_basel = 0;
			rdma_img1_ctl_c->sys_control.reg_base_sel = 0;
			rdma_img1_ctl_c->sys_control.reg_stride_sel = 0;
			rdma_img1_ctl_c->sys_control.reg_seglen_sel = 0;
			rdma_img1_ctl_c->sys_control.reg_segnum_sel = 0;
			rdma_img1_ctl_c->dma_segnum.reg_segnum = 0;
			rdma_img1_ctl_c->dma_seglen.reg_seglen = 0;
			rdma_img1_ctl_c->dma_stride.reg_stride = 0;
			img_in_c_f->reg_068.reg_ip_clr_w1t = 0;
			ive_map_c_f->reg_0.reg_ip_enable = 0;
			ive_filterop_c->reg_canny_1.reg_canny_en = 0;
			ive_filterop_c->reg_h14.reg_op_y_wdma_en = 0;
			ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
			writel(wdma_y_ctl_c->base_addr.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_BASE_ADDR));
			writel(wdma_y_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));
			writel(wdma_y_ctl_c->dma_segnum.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGNUM));
			writel(wdma_y_ctl_c->dma_seglen.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGLEN));
			writel(wdma_y_ctl_c->dma_stride.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_STRIDE));
			writel(wdma_c_ctl_c->base_addr.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_BASE_ADDR));
			writel(wdma_c_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_SYS_CONTROL));
			writel(wdma_c_ctl_c->dma_segnum.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_SEGNUM));
			writel(wdma_c_ctl_c->dma_seglen.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_SEGLEN));
			writel(wdma_c_ctl_c->dma_stride.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_STRIDE));
			writel(ive_filterop_c->reg_cropy_s.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPY_S));
			writel(ive_filterop_c->reg_cropy_e.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPY_E));
			writel(ive_filterop_c->reg_cropy_ctl.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPY_CTL));
			writel(ive_filterop_c->reg_cropc_s.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPC_S));
			writel(ive_filterop_c->reg_cropc_e.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPC_E));
			writel(ive_filterop_c->reg_cropc_ctl.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPC_CTL));
			writel(ive_filterop_c->reg_crop_odma_s.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROP_ODMA_S));
			writel(ive_filterop_c->reg_crop_odma_e.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROP_ODMA_E));
			writel(ive_filterop_c->reg_crop_odma_ctl.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROP_ODMA_CTL));
			writel(ive_filterop_c->odma_reg_01.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_01));
			writel(ive_filterop_c->odma_reg_02.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_02));
			writel(ive_filterop_c->odma_reg_03.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_03));
			writel(ive_filterop_c->odma_reg_04.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_04));
			writel(ive_filterop_c->odma_reg_05.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_05));
			writel(ive_filterop_c->odma_reg_06.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_06));
			writel(ive_filterop_c->odma_reg_07.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_07));
			writel(ive_filterop_c->odma_reg_08.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_08));
			writel(rdma_eigval_ctl_c->base_addr.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_BASE_ADDR));
			writel(rdma_eigval_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_SYS_CONTROL));
			writel(rdma_eigval_ctl_c->dma_segnum.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_SEGNUM));
			writel(rdma_eigval_ctl_c->dma_seglen.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_SEGLEN));
			writel(rdma_eigval_ctl_c->dma_stride.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_STRIDE));
			writel(rdma_img1_ctl_c->base_addr.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR));
			writel(rdma_img1_ctl_c->sys_control.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_SYS_CONTROL));
			writel(rdma_img1_ctl_c->dma_segnum.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGNUM));
			writel(rdma_img1_ctl_c->dma_seglen.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGLEN));
			writel(rdma_img1_ctl_c->dma_stride.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_STRIDE));
			writel(img_in_c_f->reg_068.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
			writel(ive_map_c_f->reg_0.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_0));
			writel(ive_filterop_c->reg_canny_1.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_1));
			writel(ive_filterop_c->reg_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
			if (optype == MOD_GRADFG) {
				rdma_gradfg_ctl_c->base_addr.reg_basel = 0;
				rdma_gradfg_ctl_c->sys_control.reg_base_sel = 0;
				rdma_gradfg_ctl_c->sys_control.reg_stride_sel = 0;
				rdma_gradfg_ctl_c->sys_control.reg_seglen_sel = 0;
				rdma_gradfg_ctl_c->sys_control.reg_segnum_sel = 0;
				rdma_gradfg_ctl_c->dma_segnum.reg_segnum = 0;
				rdma_gradfg_ctl_c->dma_seglen.reg_seglen = 0;
				rdma_gradfg_ctl_c->dma_stride.reg_stride = 0;
				ive_filterop_c->reg_33.reg_filterop_op2_gradfg_en = 0;
				ive_filterop_c->reg_h04.reg_gradfg_bggrad_rdma_en = 0;
				ive_top_c->reg_3.reg_muxsel_gradfg = 0;

				writel(rdma_gradfg_ctl_c->base_addr.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_BASE_ADDR));
				writel(rdma_gradfg_ctl_c->sys_control.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_SYS_CONTROL));
				writel(rdma_gradfg_ctl_c->dma_segnum.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGNUM));
				writel(rdma_gradfg_ctl_c->dma_seglen.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGLEN));
				writel(rdma_gradfg_ctl_c->dma_stride.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_STRIDE));
				writel(ive_filterop_c->reg_33.val,
					   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_33));
				writel(ive_filterop_c->reg_h04.val,
					   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
				writel(ive_top_c->reg_3.val,
					   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
			}
		}
	//}
	return SUCCESS;
}

void ive_set_int(ive_top_c *ive_top_c, bool isEnable, s32 dev_id)
{
	ive_top_c->reg_94.reg_intr_en_hist = isEnable;
	ive_top_c->reg_94.reg_intr_en_intg = isEnable;
	ive_top_c->reg_94.reg_intr_en_sad = isEnable;
	ive_top_c->reg_94.reg_intr_en_ncc = isEnable;
	ive_top_c->reg_94.reg_intr_en_filterop_odma = isEnable;
	ive_top_c->reg_94.reg_intr_en_filterop_wdma_y = isEnable;
	ive_top_c->reg_94.reg_intr_en_filterop_wdma_c = isEnable;
	ive_top_c->reg_94.reg_intr_en_dmaf = isEnable;
	ive_top_c->reg_94.reg_intr_en_ccl = isEnable;
	ive_top_c->reg_94.reg_intr_en_lk = isEnable;
	writel(ive_top_c->reg_94.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_94));
}

void ive_reset(struct ive_device *ndev, ive_top_c *ive_top_c, s32 dev_id)
{
	s32 i = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;

#if 0
	reinit_completion(&ndev->frame_done);
	reinit_completion(&ndev->op_done);
	ndev->total_tile = 0;
	ndev->tile_num = 0;
#endif
	//for (dev_id = 0; dev_id < IVE_DEV_MAX; dev_id ++) {
		if (atomic_read(&ndev->core[dev_id].dev_state) == IVE_CORE_STATE_RUNNING) {
			reinit_completion(&ndev->core[dev_id].frame_done);
			reinit_completion(&ndev->core[dev_id].op_done);
			ndev->core[dev_id].total_tile = 0;
			ndev->core[dev_id].tile_num = 0;

			ive_reset_reg(1, ive_top_c, dev_id);
			ive_reset_reg(2, ive_top_c, dev_id);
			ive_reset_reg(3, ive_top_c, dev_id);
			ive_reset_reg(4, ive_top_c, dev_id);
			// disable
			ive_set_int(ive_top_c, 0, dev_id);
			ive_top_c->reg_3.reg_imgmux_img0_sel = 1;
			ive_top_c->reg_3.reg_ive_rdma_img1_en = 0;
			ive_top_c->reg_3.reg_mapmux_rdma_sel = 0;
			ive_top_c->reg_3.reg_ive_rdma_eigval_en = 0;
			// default disable it
			ive_top_c->reg_3.reg_dma_share_mux_selgmm = 0;
			ive_top_c->reg_h10.reg_img_in_top_enable = 0;
			ive_top_c->reg_h10.reg_resize_top_enable = 0;
			ive_top_c->reg_h10.reg_gmm_top_enable = 0;
			ive_top_c->reg_h10.reg_csc_top_enable = 0;
			ive_top_c->reg_h10.reg_rdma_img1_top_enable = 0;
			ive_top_c->reg_h10.reg_bgm_top_enable = 0;
			ive_top_c->reg_h10.reg_bgu_top_enable = 0;
			ive_top_c->reg_h10.reg_r2y4_top_enable = 0;
			ive_top_c->reg_h10.reg_map_top_enable = 0;
			ive_top_c->reg_h10.reg_rdma_eigval_top_enable = 0;
			ive_top_c->reg_h10.reg_thresh_top_enable = 0;
			ive_top_c->reg_h10.reg_hist_top_enable = 0;
			ive_top_c->reg_h10.reg_intg_top_enable = 0;
			ive_top_c->reg_h10.reg_ncc_top_enable = 0;
			ive_top_c->reg_h10.reg_sad_top_enable = 0;
			ive_top_c->reg_h10.reg_filterop_top_enable = 0;
			ive_top_c->reg_h10.reg_dmaf_top_enable = 0;
			ive_top_c->reg_h10.reg_ccl_top_enable = 0;
			ive_top_c->reg_h10.reg_lk_top_enable = 0;
			writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
			writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

			ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
			ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
			ive_filterop_c.odma_reg_00.reg_dma_en = 0;
			writel(ive_filterop_c.reg_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
			writel(ive_filterop_c.odma_reg_00.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

			g_debug_info.src_w = 0;
			g_debug_info.src_h = 0;
			for (i = 0; i < DMA_ALL; i++) {
				g_debug_info.addr[i].addr_en = 0;
				g_debug_info.addr[i].addr_l = 0;
				g_debug_info.addr[i].addr_h = 0;
			}
			for (i = 0; i < 2; i++) {
				g_debug_info.op[i].op_en = 0;
				g_debug_info.op[i].op_sel = 0;
			}
		}
	//}
}

s32 ive_get_mod_u8(s32 tpye)
{
	switch (tpye) {
	case IVE_IMAGE_TYPE_U8C1:
		return 1;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		return 0;
	}
	return -1;
}

void ive_set_wh(ive_top_c *top, u32 w, u32 h, char *name, s32 dev_id)
{
	top->reg_2.reg_img_heightm1 = h - 1;
	top->reg_2.reg_img_widthm1 = w - 1;
	writel(top->reg_2.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_2));

	strcpy(g_debug_info.op_name, name);
	g_debug_info.src_w = top->reg_2.reg_img_widthm1;
	g_debug_info.src_h = top->reg_2.reg_img_heightm1;
}

s32 set_img_dst1(ive_dst_image_s *dst_img, isp_dma_ctl_c *wdma_y_ctl_c, s32 dev_id)
{
	// s32 swMode = 0;
	s32 swMode = 0;
	//DEFINE_isp_dma_ctl_c(_wdma_y_ctl_c);
	isp_dma_ctl_c _wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;

	if (wdma_y_ctl_c == NULL) {
		wdma_y_ctl_c = &_wdma_y_ctl_c;
	}
	if (dst_img == NULL) {
		wdma_y_ctl_c->base_addr.reg_basel = 0;
		wdma_y_ctl_c->sys_control.reg_baseh = 0;
		wdma_y_ctl_c->sys_control.reg_base_sel = 0;
		writel(wdma_y_ctl_c->base_addr.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_BASE_ADDR));
		writel(wdma_y_ctl_c->sys_control.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));

		return SUCCESS;
	}
	wdma_y_ctl_c->base_addr.reg_basel = dst_img->phy_addr[0] & 0xffffffff;
	writel(wdma_y_ctl_c->base_addr.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_BASE_ADDR));

	wdma_y_ctl_c->sys_control.reg_baseh =
		(dst_img->phy_addr[0] >> 32) & 0xffffffff;
	wdma_y_ctl_c->sys_control.reg_stride_sel = swMode;
	wdma_y_ctl_c->sys_control.reg_seglen_sel = swMode;
	wdma_y_ctl_c->sys_control.reg_segnum_sel = swMode;

	if (swMode) {
		// set height
		wdma_y_ctl_c->dma_segnum.reg_segnum = dst_img->height;
		writel(wdma_y_ctl_c->dma_segnum.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGNUM));
		// set width
		wdma_y_ctl_c->dma_seglen.reg_seglen = dst_img->width;
		writel(wdma_y_ctl_c->dma_seglen.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGLEN));
		// set stride
		wdma_y_ctl_c->dma_stride.reg_stride = dst_img->stride[0];
		writel(wdma_y_ctl_c->dma_stride.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_STRIDE));
		// FIXME: set U8/S8
	} else {
		// hw mode, no need to set height/width/stride
		// wdma_y_ctl_c->dma_stride.reg_stride = dst_img->stride[0];
		// writel(wdma_y_ctl_c->dma_stride.val,
		// 	  (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_STRIDE));
	}
	wdma_y_ctl_c->sys_control.reg_stride_sel = 0;
	wdma_y_ctl_c->sys_control.reg_base_sel = 1; // sw specify address
	writel(wdma_y_ctl_c->sys_control.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));
	if (g_dump_dma_info == TRUE) {
		pr_info("Dst1 address: 0x%08x %08x\n",
			wdma_y_ctl_c->sys_control.reg_baseh,
			wdma_y_ctl_c->base_addr.reg_basel);
	}

	g_debug_info.addr[WDMA_Y].addr_en = wdma_y_ctl_c->sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_Y].addr_l = wdma_y_ctl_c->base_addr.reg_basel;
	g_debug_info.addr[WDMA_Y].addr_h = wdma_y_ctl_c->sys_control.reg_baseh & 0xff;

	return SUCCESS;
}

s32 set_img_dst2(ive_dst_image_s *dst_img, isp_dma_ctl_c *wdma_c_ctl_c, s32 dev_id)
{
	//DEFINE_isp_dma_ctl_c(_wdma_c_ctl_c);
	isp_dma_ctl_c _wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;

	if (wdma_c_ctl_c == NULL) {
		wdma_c_ctl_c = &_wdma_c_ctl_c;
	}

	if (dst_img != NULL) {
		wdma_c_ctl_c->base_addr.reg_basel =
			dst_img->phy_addr[0] & 0xffffffff;
		wdma_c_ctl_c->sys_control.reg_baseh =
			(dst_img->phy_addr[0] >> 32) & 0xffffffff;
		// need to set stride ??
		// wdma_c_ctl_c->dma_stride.reg_stride = dst_img->stride[0];
		writel(wdma_c_ctl_c->dma_stride.val,
			(IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_STRIDE));
		wdma_c_ctl_c->sys_control.reg_stride_sel = 0;
		wdma_c_ctl_c->sys_control.reg_base_sel = 1;
		if (g_dump_dma_info == TRUE) {
			pr_info("Dst2 address: 0x%08x %08x\n",
				wdma_c_ctl_c->sys_control.reg_baseh,
				wdma_c_ctl_c->base_addr.reg_basel);
		}
	} else {
		wdma_c_ctl_c->base_addr.reg_basel = 0;
		wdma_c_ctl_c->sys_control.reg_baseh = 0;
		wdma_c_ctl_c->sys_control.reg_base_sel = 0;
	}
	writel(wdma_c_ctl_c->base_addr.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_c_ctl_c->sys_control.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_SYS_CONTROL));

	g_debug_info.addr[WDMA_C].addr_en = wdma_c_ctl_c->sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_C].addr_l = wdma_c_ctl_c->base_addr.reg_basel;
	g_debug_info.addr[WDMA_C].addr_h = wdma_c_ctl_c->sys_control.reg_baseh & 0xff;
	return SUCCESS;
}

s32 set_img_src1(ive_src_image_s *src_img, img_in_c *img_in_c, ive_top_c *ive_top_c, s32 dev_id)
{
	//test2:reset image in ip before input
	img_in_c->reg_068.reg_ip_clr_w1t = 1;
    writel(img_in_c->reg_068.val,(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
    udelay(10);
    img_in_c->reg_068.reg_ip_clr_w1t = 0;
    writel(img_in_c->reg_068.val,(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));


	ive_top_c->reg_h10.reg_img_in_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	// Burst len unit is byte, 8 means 128bit
	img_in_c->reg_00.reg_burst_ln = 8;
	img_in_c->reg_00.reg_src_sel = 2; // 2 for others: DRMA
	img_in_c->reg_00.reg_fmt_sel = get_img_fmt_sel(src_img->type);
	writel(img_in_c->reg_00.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

	// set outstanding to 16
	img_in_c->reg_064.reg_os_max = 0xf;
	writel(img_in_c->reg_064.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_064));

	img_in_c->reg_03.reg_src_y_pitch = src_img->stride[0]; //0x160 ok
	writel(img_in_c->reg_03.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_03));
	// NOTICE: need to set 1 to valify settings immediantly
	// img_in_c->reg_05.reg_shrd_sel = 0;
	img_in_c->reg_05.reg_shrd_sel = 1;
	writel(img_in_c->reg_05.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_05));
	img_in_c->reg_02.reg_src_wd = src_img->width - 1;
	img_in_c->reg_02.reg_src_ht = src_img->height - 1;
	// img_in_c->reg_02.val = (src_img->height<<16) | src_img->width;
	writel(img_in_c->reg_02.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02));



	img_in_c->reg_y_base_0.reg_src_y_base_b0 =
		(src_img->phy_addr[0] & 0xffffffff);
	writel(img_in_c->reg_y_base_0.val,
		   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_0));
	img_in_c->reg_y_base_1.reg_src_y_base_b1 =
		(src_img->phy_addr[0] >> 32 & 0xffffffff);
	writel(img_in_c->reg_y_base_1.val,
		   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_1));
	switch (src_img->type) {
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U8C1:
		break;
	// api already stride[0] * 3
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		img_in_c->reg_03.reg_src_y_pitch = src_img->stride[0];
		writel(img_in_c->reg_03.val,
				(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_03));
		img_in_c->reg_04.reg_src_c_pitch = 0;
		writel(img_in_c->reg_04.val,
				(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->reg_u_base_0.reg_src_u_base_b0 = 0;
		writel(img_in_c->reg_u_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->reg_u_base_1.reg_src_u_base_b1 = 0;
		writel(img_in_c->reg_u_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->reg_v_base_0.reg_src_v_base_b0 = 0;
		writel(img_in_c->reg_v_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->reg_v_base_1.reg_src_v_base_b1 = 0;
		writel(img_in_c->reg_v_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		img_in_c->reg_04.reg_src_c_pitch = src_img->stride[0];
		writel(img_in_c->reg_04.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->reg_u_base_0.reg_src_u_base_b0 =
			(src_img->phy_addr[1] & 0xffffffff);
		writel(img_in_c->reg_u_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->reg_u_base_1.reg_src_u_base_b1 =
			((src_img->phy_addr[1] >> 32) & 0xffffffff);
		writel(img_in_c->reg_u_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->reg_v_base_0.reg_src_v_base_b0 =
			(src_img->phy_addr[2] & 0xffffffff);
		writel(img_in_c->reg_v_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->reg_v_base_1.reg_src_v_base_b1 =
			((src_img->phy_addr[2] >> 32) & 0xffffffff);
		writel(img_in_c->reg_v_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;

	case IVE_IMAGE_TYPE_YUV420SP:
		img_in_c->reg_04.reg_src_c_pitch = src_img->stride[1];
		writel(img_in_c->reg_04.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->reg_u_base_0.reg_src_u_base_b0 =
			(src_img->phy_addr[1] & 0xffffffff);
		writel(img_in_c->reg_u_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->reg_u_base_1.reg_src_u_base_b1 =
			((src_img->phy_addr[1] >> 32) & 0xffffffff);
		writel(img_in_c->reg_u_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->reg_v_base_0.reg_src_v_base_b0 =
			((src_img->phy_addr[1] + 1) & 0xffffffff);
		writel(img_in_c->reg_v_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->reg_v_base_1.reg_src_v_base_b1 =
			(((src_img->phy_addr[1] + 1) >> 32) & 0xffffffff);
		writel(img_in_c->reg_v_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		img_in_c->reg_04.reg_src_c_pitch = src_img->stride[0];
		writel(img_in_c->reg_04.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->reg_u_base_0.reg_src_u_base_b0 =
			(src_img->phy_addr[1] & 0xffffffff);
		writel(img_in_c->reg_u_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->reg_u_base_1.reg_src_u_base_b1 =
			((src_img->phy_addr[1] >> 32) & 0xffffffff);
		writel(img_in_c->reg_u_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->reg_v_base_0.reg_src_v_base_b0 =
			((src_img->phy_addr[1] + 1) & 0xffffffff);
		writel(img_in_c->reg_v_base_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->reg_v_base_1.reg_src_v_base_b1 =
			(((src_img->phy_addr[1] + 1) >> 32) & 0xffffffff);
		writel(img_in_c->reg_v_base_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;
	default:
		pr_err("[IVE] not support src type\n");
		return FAILURE;
	}
	if (g_dump_dma_info == TRUE) {
		pr_info("Src1 img_in_c address y: 0x%08x %08x\n",
				img_in_c->reg_y_base_1.val, img_in_c->reg_y_base_0.val);
	}
	g_debug_info.src_fmt = img_in_c->reg_00.reg_fmt_sel;
	g_debug_info.addr[RDMA_IMG_IN].addr_en = true;
	g_debug_info.addr[RDMA_IMG_IN].addr_l = img_in_c->reg_y_base_0.reg_src_y_base_b0;
	g_debug_info.addr[RDMA_IMG_IN].addr_h = img_in_c->reg_y_base_1.reg_src_y_base_b1 & 0xff;
	return SUCCESS;
}

s32 set_img_src2(ive_src_image_s *src_img, isp_dma_ctl_c *rdma_img1_ctl_c, s32 dev_id)
{
	//DEFINE_isp_dma_ctl_c(_rdma_img1_ctl_c);
	isp_dma_ctl_c _rdma_img1_ctl_c = _DEFINE_isp_dma_ctl_c;

	if (rdma_img1_ctl_c == NULL) {
		rdma_img1_ctl_c = &_rdma_img1_ctl_c;
	}
	rdma_img1_ctl_c->base_addr.reg_basel =
		src_img->phy_addr[0] & 0xffffffff;
	writel(rdma_img1_ctl_c->base_addr.val,
		   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR));

	rdma_img1_ctl_c->sys_control.reg_baseh =
		(src_img->phy_addr[0] >> 32) & 0xffffffff;
	//rdma_img1_ctl_c->dma_stride.reg_stride = src_img->stride[0];
	//rdma_img1_ctl_c->sys_control.reg_stride_sel = 1;
	rdma_img1_ctl_c->sys_control.reg_base_sel = 1;
	writel(rdma_img1_ctl_c->dma_stride.val,
		   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_STRIDE));
	writel(rdma_img1_ctl_c->sys_control.val,
		   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_SYS_CONTROL));
	if (g_dump_dma_info == TRUE) {
		pr_info("Src2 address: 0x%08x %08x\n",
			rdma_img1_ctl_c->sys_control.reg_baseh,
			rdma_img1_ctl_c->base_addr.reg_basel);
	}
	g_debug_info.addr[RDMA_IMG1].addr_en = rdma_img1_ctl_c->sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_IMG1].addr_l = rdma_img1_ctl_c->base_addr.reg_basel;
	g_debug_info.addr[RDMA_IMG1].addr_h = rdma_img1_ctl_c->sys_control.reg_baseh & 0xff;
	return SUCCESS;
}

s32 set_isp_rdma(s32 optype, isp_rdma_c *rdma_c, s32 dev_id)
{
	isp_rdma_c _rdma_c = _DEFINE_ISP_RDMA_C;
	if (rdma_c == NULL) {
		rdma_c = &_rdma_c;
	}

	if (optype == MOD_GMM || optype == MOD_GMM2 || optype == MOD_BGM || optype == MOD_BGU) {
		// set ostanding to 8
		rdma_c->shadow_rd_sel.reg_max_ostd = 0x8;
		rdma_c->shadow_rd_sel.reg_ostd_sw_en = 1;
	}
	else {
		// set ostanding to 16
		rdma_c->shadow_rd_sel.reg_max_ostd = 0x10;
		rdma_c->shadow_rd_sel.reg_ostd_sw_en = 1;
	}
	writel(rdma_c->shadow_rd_sel.val,
		   (IVE_BLK_BA[dev_id].RDMA + ISP_RDMA_SHADOW_RD_SEL));

	return SUCCESS;
}


s32 set_rdma_eigval(ive_src_image_s *pstSrc, isp_dma_ctl_c *rdma_eigval_ctl_c, s32 dev_id)
{
	//DEFINE_isp_dma_ctl_c(_rdma_eigval_ctl_c);
	isp_dma_ctl_c _rdma_eigval_ctl_c = _DEFINE_isp_dma_ctl_c;

	if (rdma_eigval_ctl_c == NULL) {
		rdma_eigval_ctl_c = &_rdma_eigval_ctl_c;
	}

	rdma_eigval_ctl_c->base_addr.reg_basel =
		pstSrc->phy_addr[0] & 0xffffffff;
	rdma_eigval_ctl_c->sys_control.reg_baseh =
		(pstSrc->phy_addr[0] >> 32) & 0xffffffff;
	rdma_eigval_ctl_c->dma_stride.reg_stride = pstSrc->stride[0];
	writel(rdma_eigval_ctl_c->dma_stride.val,
		   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_STRIDE));
	rdma_eigval_ctl_c->sys_control.reg_base_sel = 1;

	rdma_eigval_ctl_c->sys_control.reg_stride_sel = 1;
	rdma_eigval_ctl_c->sys_control.reg_seglen_sel = 0;
	rdma_eigval_ctl_c->sys_control.reg_segnum_sel = 0;
	writel(rdma_eigval_ctl_c->base_addr.val,
		   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_eigval_ctl_c->sys_control.val,
		   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_SYS_CONTROL));
	if (g_dump_dma_info == TRUE) {
		pr_info("RdmaEigval address: 0x%08x %08x\n",
			rdma_eigval_ctl_c->sys_control.reg_baseh,
			rdma_eigval_ctl_c->base_addr.reg_basel);
	}
	g_debug_info.addr[RDMA_EIGVAL].addr_en = rdma_eigval_ctl_c->sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_EIGVAL].addr_l = rdma_eigval_ctl_c->base_addr.reg_basel;
	g_debug_info.addr[RDMA_EIGVAL].addr_h = rdma_eigval_ctl_c->sys_control.reg_baseh & 0xff;
	return SUCCESS;
}

s32 set_odma(ive_src_image_s *dst_img, ive_filterop_c *ive_filterop_c, s32 w,
		s32 h, s32 dev_id)
{
	ive_filterop_c->odma_reg_00.reg_fmt_sel = get_img_fmt_sel(dst_img->type);
	writel(ive_filterop_c->odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	switch (dst_img->type) {
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV422P:
		ive_filterop_c->odma_reg_03.reg_dma_u_base_low_part =
			dst_img->phy_addr[1] & 0xffffffff;
		ive_filterop_c->odma_reg_04.reg_dma_u_base_high_part =
			(dst_img->phy_addr[1] >> 32) & 0xffffffff;
		ive_filterop_c->odma_reg_05.reg_dma_v_base_low_part =
			(dst_img->phy_addr[2]) & 0xffffffff;
		ive_filterop_c->odma_reg_06.reg_dma_v_base_high_part =
			((dst_img->phy_addr[2]) >> 32) & 0xffffffff;
		ive_filterop_c->odma_reg_08.reg_dma_c_pitch =
			dst_img->stride[1];
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		// NV21
	case IVE_IMAGE_TYPE_YUV422SP:
		ive_filterop_c->odma_reg_03.reg_dma_u_base_low_part =
			dst_img->phy_addr[1] & 0xffffffff;
		ive_filterop_c->odma_reg_04.reg_dma_u_base_high_part =
			(dst_img->phy_addr[1] >> 32) & 0xffffffff;
		ive_filterop_c->odma_reg_05.reg_dma_v_base_low_part =
			(dst_img->phy_addr[1] + 1) & 0xffffffff;
		ive_filterop_c->odma_reg_06.reg_dma_v_base_high_part =
			((dst_img->phy_addr[1] + 1) >> 32) & 0xffffffff;
		ive_filterop_c->odma_reg_08.reg_dma_c_pitch =
			dst_img->stride[0];
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		ive_filterop_c->odma_reg_03.reg_dma_u_base_low_part =
			dst_img->phy_addr[1] & 0xffffffff;
		ive_filterop_c->odma_reg_04.reg_dma_u_base_high_part =
			(dst_img->phy_addr[1] >> 32) & 0xffffffff;
		ive_filterop_c->odma_reg_05.reg_dma_v_base_low_part =
			(dst_img->phy_addr[2]) & 0xffffffff;
		ive_filterop_c->odma_reg_06.reg_dma_v_base_high_part =
			((dst_img->phy_addr[2]) >> 32) & 0xffffffff;
		ive_filterop_c->odma_reg_08.reg_dma_c_pitch =
			dst_img->stride[0];
		break;
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		ive_filterop_c->odma_reg_03.reg_dma_u_base_low_part = 0;
		ive_filterop_c->odma_reg_04.reg_dma_u_base_high_part = 0;
		ive_filterop_c->odma_reg_05.reg_dma_v_base_low_part = 0;
		ive_filterop_c->odma_reg_06.reg_dma_v_base_high_part = 0;
		ive_filterop_c->odma_reg_08.reg_dma_c_pitch = 0;
		break;
	default:
		pr_err("[IVE] not support dstEnType");
		return FAILURE;
	}
	writel(ive_filterop_c->odma_reg_03.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_03));
	writel(ive_filterop_c->odma_reg_04.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_04));
	writel(ive_filterop_c->odma_reg_05.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_05));
	writel(ive_filterop_c->odma_reg_06.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_06));
	writel(ive_filterop_c->odma_reg_08.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_08));
	// odma for filter3ch + csc
	ive_filterop_c->odma_reg_01.reg_dma_y_base_low_part =
		dst_img->phy_addr[0] & 0xffffffff;
	writel(ive_filterop_c->odma_reg_01.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_01));
	ive_filterop_c->odma_reg_02.reg_dma_y_base_high_part =
		(dst_img->phy_addr[0] >> 32) & 0xffffffff;
	writel(ive_filterop_c->odma_reg_02.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_02));

	ive_filterop_c->odma_reg_07.reg_dma_y_pitch = dst_img->stride[0];
	writel(ive_filterop_c->odma_reg_07.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_07));
	// api already * 3, we do not need to do this
	/*
	 * if (dst_img->type == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
	 *	ive_filterop_c->odma_reg_07.reg_dma_y_pitch = 3 * dst_img->stride[0];
	 *}
	 */
	ive_filterop_c->odma_reg_11.reg_dma_wd = w - 1;
	ive_filterop_c->odma_reg_12.reg_dma_ht = h - 1;
	writel(ive_filterop_c->odma_reg_11.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_11));
	writel(ive_filterop_c->odma_reg_12.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_12));
	// trigger odma
	ive_filterop_c->odma_reg_00.reg_dma_blen = 1;
	ive_filterop_c->odma_reg_00.reg_dma_en = 1;
	writel(ive_filterop_c->odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	// enable it
	/*ive_filterop_c->reg_h14.reg_op_y_wdma_en = 1;
	 * ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	 * writel(ive_filterop_c->reg_h14.val,
	 *     (IVE_BLK_BA.FILTEROP + IVE_FILTEROP_REG_H14));
	 */
	if (g_dump_dma_info == TRUE) {
		pr_info("Odma address: 0x%08x %08x (%d %d, s=%d)\n",
				ive_filterop_c->odma_reg_02.val,
				ive_filterop_c->odma_reg_01.val, w, h,
				dst_img->stride[0]);
	}
	g_debug_info.dst_fmt = ive_filterop_c->odma_reg_00.reg_fmt_sel;
	g_debug_info.addr[WDMA_ODMA].addr_en = true;
	g_debug_info.addr[WDMA_ODMA].addr_l = ive_filterop_c->odma_reg_01.reg_dma_y_base_low_part;
	g_debug_info.addr[WDMA_ODMA].addr_h = ive_filterop_c->odma_reg_02.reg_dma_y_base_high_part & 0xff;
	return SUCCESS;
}

s32 ive_base_op(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			bool instant, s32 op, void *ctrl, s32 dev_id)
{
	s32 ret = 0;
	s32 mode = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(rdma_img1_ctl_c);
	isp_dma_ctl_c rdma_img1_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		dump_ive_image("pstDst", pstDst);
	}

		if (op == MOD_AND) {
			ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "And", dev_id);
		} else if (op == MOD_OR) {
			ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "Or", dev_id);
		} else if (op == MOD_XOR) {
			ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "Xor", dev_id);
		} else if (op == MOD_ADD) {
			ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "Add", dev_id);
		} else if (op == MOD_SUB) {
			ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "Sub", dev_id);
		} else {
			ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "BaseError", dev_id);
		}

		// setting
		ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
		writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 0;
		writel(ive_top_c->reg_r2y4_14.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

		if (op == MOD_ADD && ctrl != NULL) { //0:and, 1:or, 2:xor, 3:add, 4:sub
			ive_top_c->reg_20.reg_frame2op_add_mode_rounding = 1;
			ive_top_c->reg_20.reg_frame2op_add_mode_clipping = 1;
			//convirt float to unsigned short
			ive_top_c->reg_21.reg_fram2op_x_u0q16 =
				((ive_add_ctrl_s *)ctrl)->x; // 0xffff
			ive_top_c->reg_21.reg_fram2op_y_u0q16 =
				((ive_add_ctrl_s *)ctrl)->y; // 0xffff
			writel(ive_top_c->reg_21.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_21));
		} else if (op == MOD_SUB && ctrl != NULL) {
			ive_top_c->reg_20.reg_frame2op_sub_change_order = 0;
			ive_top_c->reg_20.reg_frame2op_sub_switch_src = 0;
			ive_top_c->reg_20.reg_frame2op_sub_mode =
				((ive_sub_ctrl_s *)ctrl)->mode;
		}
		if (op == MOD_ADD) {
			ive_top_c->reg_20.reg_frame2op_op_mode = 3;
		} else if (op == MOD_AND) {
			ive_top_c->reg_20.reg_frame2op_op_mode = 0;
		} else if (op == MOD_OR) {
			ive_top_c->reg_20.reg_frame2op_op_mode = 1;
		} else if (op == MOD_SUB) {
			ive_top_c->reg_20.reg_frame2op_op_mode = 4;
		} else if (op == MOD_XOR) {
			ive_top_c->reg_20.reg_frame2op_op_mode = 2;
		}
		writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));

		set_isp_rdma(op, NULL, dev_id);
		if (set_img_src1(pstSrc1, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
			kfree(ive_top_c);
			return FAILURE;
		}

		set_img_src2(pstSrc2, &rdma_img1_ctl_c, dev_id);

		set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

		mode = ive_get_mod_u8(pstSrc1->type);
		if (mode == -1) {
			pr_err("[IVE] not support src type");
			kfree(ive_top_c);
			return FAILURE;
		}
		ive_top_c->reg_3.reg_ive_rdma_img1_mod_u8 = mode;
		// TODO: need to set vld?
		ive_top_c->reg_3.reg_imgmux_img0_sel = 0;
		ive_top_c->reg_3.reg_ive_rdma_img1_en = 1;
		writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		ive_filterop_c.reg_h10.reg_filterop_mode = 2;
		writel(ive_filterop_c.reg_h10.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));

		ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0; // sw_ovw; bypass op1
		ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		ive_filterop_c.reg_28.reg_filterop_op2_erodila_en = 0; // bypass op2
		writel(ive_filterop_c.reg_28.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

		ive_top_c->reg_h10.reg_filterop_top_enable = 1;
		ive_top_c->reg_h10.reg_rdma_img1_top_enable = 1;
		writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

		if (pstSrc1->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
					&wdma_y_ctl_c, &rdma_img1_ctl_c, NULL, NULL,
					pstSrc1, pstSrc2, NULL, pstDst, NULL, true, 1,
					false, 1, false, op, instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}

	ive_go(ndev, ive_top_c, instant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, op, dev_id);

	kfree(ive_top_c);
	return SUCCESS;
}

s32 _ive_reset(struct ive_device *ndev, s32 select, s32 dev_id)
{
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c ive_top_c = _DEFINE_IVE_TOP_C;

	//udelay(3);
	vi_sys_reg_write_mask(VI_SYS_REG_RST_IVE_TOP0,
				VI_SYS_REG_RST_IVE_TOP0_MASK,
				0x1 << VI_SYS_REG_RST_IVE_TOP0_OFFSET);
	//udelay(3);
	vi_sys_reg_write_mask(VI_SYS_REG_RST_IVE_TOP0,
				VI_SYS_REG_RST_IVE_TOP0_MASK,
				0x0 << VI_SYS_REG_RST_IVE_TOP0_OFFSET);

	ive_set_int(&ive_top_c, 0, dev_id);

	return SUCCESS;
}

s32 ive_dump_hw_flow(void)
{
	s32 i = 0;
	ive_top_reg_h10_c TOP_REG_H10;
	ive_top_reg_3_c  TOP_REG_3;
	ive_filterop_odma_reg_00_c odma_reg_00;
	ive_filterop_reg_h14_c reg_h14;

	TOP_REG_H10.val = readl(IVE_BLK_BA[d_num].IVE_TOP + IVE_TOP_REG_H10);
	TOP_REG_3.val = readl(IVE_BLK_BA[d_num].IVE_TOP + IVE_TOP_REG_3);
	odma_reg_00.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_ODMA_REG_00);
	reg_h14.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_H14);
	pr_info("[%s]\n", g_debug_info.op_name);
	pr_info("Input Width/Height: %d/%d\n", g_debug_info.src_w, g_debug_info.src_h);
	if (g_debug_info.addr[RDMA_IMG_IN].addr_en)
		pr_info("Input Img Format: %s\n", IMG_FMT[g_debug_info.src_fmt]);
	if (g_debug_info.addr[WDMA_ODMA].addr_en)
		pr_info("Output Img Format: %s\n", IMG_FMT[g_debug_info.dst_fmt]);
	pr_info("Mode Op1 (sw_ovw_op/op1_cmd): %d/%d\n", g_debug_info.op[0].op_en, g_debug_info.op[0].op_sel);
	pr_info("Mode Op2 (op2_erodila_en/mode): %d/%d\n", g_debug_info.op[1].op_en, g_debug_info.op[1].op_sel);
	pr_info("Address Info:\n");
	for (i = 0; i < DMA_ALL; i++) {
		if (g_debug_info.addr[i].addr_en) {
			pr_info(" %s -> 0x%08x 0x%08x\n", g_debug_info.addr[i].addr_name,
			g_debug_info.addr[i].addr_h, g_debug_info.addr[i].addr_l);
		}
	}
	pr_info("Top Enable:\n");
	if (TOP_REG_H10.reg_img_in_top_enable)
		pr_info(" img_in");
	if (TOP_REG_H10.reg_resize_top_enable)
		pr_info(" resize");
	if (TOP_REG_H10.reg_gmm_top_enable)
		pr_info(" gmm");
	if (TOP_REG_H10.reg_csc_top_enable)
		pr_info(" csc");
	if (TOP_REG_H10.reg_rdma_img1_top_enable)
		pr_info(" rdma_img1");
	if (TOP_REG_H10.reg_bgm_top_enable)
		pr_info(" bgm");
	if (TOP_REG_H10.reg_bgu_top_enable)
		pr_info(" bgu");
	if (TOP_REG_H10.reg_r2y4_top_enable)
		pr_info(" r2y4");
	if (TOP_REG_H10.reg_rdma_eigval_top_enable)
		pr_info(" rdma_eigval");
	if (TOP_REG_H10.reg_thresh_top_enable)
		pr_info(" thresh");
	if (TOP_REG_H10.reg_hist_top_enable)
		pr_info(" hist");
	if (TOP_REG_H10.reg_intg_top_enable)
		pr_info(" integ");
	if (TOP_REG_H10.reg_map_top_enable)
		pr_info(" map");
	if (TOP_REG_H10.reg_ncc_top_enable)
		pr_info(" ncc");
	if (TOP_REG_H10.reg_sad_top_enable)
		pr_info(" sad");
	if (TOP_REG_H10.reg_filterop_top_enable) {
		if (odma_reg_00.reg_dma_en)
			pr_info(" filterop: odma");
		if (reg_h14.reg_op_y_wdma_en)
			pr_info(" filterop: y");
		if (reg_h14.reg_op_c_wdma_en)
			pr_info(" filterop: c");
	}
	if (TOP_REG_H10.reg_dmaf_top_enable)
		pr_info(" dmaf");
	if (TOP_REG_H10.reg_ccl_top_enable)
		pr_info(" ccl");
	if (TOP_REG_H10.reg_lk_top_enable)
		pr_info(" lk");
	pr_info("Flow Select:\n");
	pr_info(" imgmux_img0_sel: %d\n", TOP_REG_3.reg_imgmux_img0_sel);
	pr_info(" mapmux_rdma_sel: %d\n", TOP_REG_3.reg_mapmux_rdma_sel);
	pr_info(" muxsel_gradfg: %d\n", TOP_REG_3.reg_muxsel_gradfg);
	pr_info(" dma_share_mux_selgmm: %d\n", TOP_REG_3.reg_dma_share_mux_selgmm);
	return SUCCESS;
}

s32 ive_dump_op1_op2_info(void)
{
	ive_filterop_c ive_filterop_c;
	ive_filterop_c.reg_h10.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_H10);
	ive_filterop_c.reg_h14.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_H14);
	ive_filterop_c.reg_28.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_28);

	pr_info("Ive mode: %d/%d %d/%d\n", ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op,
		ive_filterop_c.reg_h14.reg_filterop_op1_cmd, ive_filterop_c.reg_28.reg_filterop_op2_erodila_en,
		ive_filterop_c.reg_h10.reg_filterop_mode);
	pr_info("\top1: %s\n", ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op?"Enable":"Disable");
	if (ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op) {
		pr_info("\talgo: %d\n", ive_filterop_c.reg_h14.reg_filterop_op1_cmd);
		pr_info("\t 0:OP1_BYP\n\t 1:OP1_FILTER\n\t 2:OP1_DILA\n\t 3:OP1_ERO\n");
		pr_info("\t 4:OP1_ORDERF\n\t 5:OP1_BERN\n\t 6:OP1_LBP\n\t 7:OP1_NORMG\n");
		pr_info("\t 8:OP1_MAG\n\t 9:OP1_SOBEL\n\t 10:OP1_STCANDI\n\t 11:OP1_MAP\n");
	}
	pr_info("\top2: %s\n", ive_filterop_c.reg_28.reg_filterop_op2_erodila_en?"Enable":"Disable");
	if (ive_filterop_c.reg_28.reg_filterop_op2_erodila_en) {
		pr_info("\talgo: %d\n", ive_filterop_c.reg_h10.reg_filterop_mode);
		pr_info("\t 0:MOD_BYP\n\t 1:MOD_FILTER3CH\n\t 2:MOD_DILA\n\t 3:MOD_ERO\n");
		pr_info("\t 4:MOD_CANNY\n\t 5:MOD_STBOX\n\t 6:MOD_GRADFG\n\t 7:MOD_MAG\n");
		pr_info("\t 8:MOD_NORMG\n\t 9:MOD_SOBEL\n\t 10:MOD_STCANDI\n\t 11:MOD_MAP\n");
	}
	return SUCCESS;
}

s32 ive_query(struct ive_device *ndev, bool *pbFinish,
			  bool bBlock, s32 dev_id)
{
	long leavetime = 0;
	*pbFinish = FAILURE;
	if (!bBlock) {
		leavetime = wait_for_completion_timeout(
			&ndev->core[dev_id].op_done, msecs_to_jiffies(1 * TIMEOUT_MS));
		if (leavetime == 0) {
			TRACE_IVE(IVE_DBG_INFO, "[IVE] stop by timeout\n");
			return SUCCESS;
		} else if (leavetime < 0) {
			TRACE_IVE(IVE_DBG_INFO, "[IVE] stop by interrupted\n");
			return SUCCESS;
		}
	} else {
		wait_for_completion(&ndev->core[dev_id].op_done);
	}

	*pbFinish = TRUE;
	return SUCCESS;
}

s32 ive_test(struct ive_device *ndev, char *addr, u16 *w,
			 u16 *h)
{
	return SUCCESS;
}

s32 ive_set_dma_dump(bool enable)
{
	g_dump_dma_info = enable;
	return SUCCESS;
}

s32 ive_set_reg_dump(bool enable)
{
	g_dump_reg_info = enable;
	return SUCCESS;
}

s32 ive_set_img_dump(bool enable)
{
	g_dump_image_info = enable;
	return SUCCESS;
}

s32 assign_ive_block_addr(void __iomem *ive_phy_base, int dev_id)
{
#ifdef DEBUG
	s32 i = 0;
	uintptr_t *array = NULL;
#endif
	g_dump_reg_info = false;
	g_dump_image_info = false;
	g_dump_dma_info = false;

	//reg_val = vip_sys_reg_read(0xc8);
	if (!ive_phy_base)
		return FAILURE;

#if defined(__CV186X__)
	if(dev_id == 0) {
		g_phy_shift[dev_id] = (uintptr_t)ive_phy_base - (uintptr_t)IVE_TOP_PHY_REG_BASE_0;
	}
	if(dev_id == 1) {
		g_phy_shift[dev_id] = (uintptr_t)ive_phy_base - (uintptr_t)IVE_TOP_PHY_REG_BASE_1;
	}
#else
	g_phy_shift = (uintptr_t)ive_phy_base - (uintptr_t)IVE_TOP_PHY_REG_BASE;

#endif

		IVE_BLK_BA[dev_id].IVE_TOP =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_IVE_TOP); //(0X00000000)
		IVE_BLK_BA[dev_id].IMG_IN = (void __iomem *)(ive_phy_base +
							 IVE_BLK_BA_IMG_IN); //(0X00000400)
		IVE_BLK_BA[dev_id].RDMA_IMG1 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_RDMA_IMG1); //(0X00000500)
		IVE_BLK_BA[dev_id].MAP =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_MAP); //(0X00000600)
		IVE_BLK_BA[dev_id].HIST =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_HIST); //(0X00000700)
		IVE_BLK_BA[dev_id].HIST_WDMA = (void __iomem *)(ive_phy_base + IVE_BLK_BA_HIST +
							0x40); //(0X00000740)
		IVE_BLK_BA[dev_id].INTG =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_INTG); //(0X00000800)
		IVE_BLK_BA[dev_id].INTG_WDMA = (void __iomem *)(ive_phy_base + IVE_BLK_BA_INTG +
							0x40); //(0X00000840)
		IVE_BLK_BA[dev_id].NCC =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_NCC); //(0X00000900)
		IVE_BLK_BA[dev_id].SAD =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_SAD); //(0X00000A00)
		IVE_BLK_BA[dev_id].SAD_WDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_SAD_WDMA); //(0X00000A80)
		IVE_BLK_BA[dev_id].SAD_WDMA_THR =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_SAD_WDMA_THR); //(0X00000B00)
		IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_RDMA_0); //(0X00001000)
		IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_1 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_RDMA_1); //(0X00001040)
		IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_2 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_RDMA_2); //(0X00001080)
		IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_3 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_RDMA_3); //(0X000010C0)
		IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_4 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_RDMA_4); //(0X00001100)
		IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_WDMA_0); //(0X00001140)
		IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_1 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_WDMA_1); //(0X00001180)
		IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_2 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_WDMA_2); //(0X000011C0)
		IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_3 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_WDMA_3); //(0X00001200)
		IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_4 =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MODEL_WDMA_4); //(0X00001240)
		IVE_BLK_BA[dev_id].GMM_MATCH_WDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_GMM_MATCH_WDMA); //(0X00001280)
		IVE_BLK_BA[dev_id].GMM =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_GMM); //(0X000012C0)
		IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_GMM_MODEL_RDMA_0 +
					 0x300); //(0X00001300)
		IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_BG_MATCH); //(0X00001400)
		IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_MATCH +
					 0x20); //(0X00001420)
		IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_MATCH +
					 0x40); //(0X00001440)
		IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_MATCH +
					 0x60); //(0X00001460)
		IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_MATCH +
					 0x80); //(0X00001480)
		IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_BG_UPDATE); //(0X00001600)
		IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_UPDATE +
					 0x40); //(0X00001640)
		IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_UPDATE +
					 0x80); //(0X00001680)
		IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_UPDATE +
					 0xc0); //(0X000016c0)
		IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_BG_UPDATE +
					 0x100); //(0X00001700)
		IVE_BLK_BA[dev_id].FILTEROP_RDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_FILTEROP); //(0X00002000)
		IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_FILTEROP +
					 0x40); //(0X00002040)
		IVE_BLK_BA[dev_id].FILTEROP_WDMA_C =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_FILTEROP +
					 0x80); //(0X00002080)
		IVE_BLK_BA[dev_id].FILTEROP =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_FILTEROP +
					 0x200); //(0X00002200)
		IVE_BLK_BA[dev_id].CCL =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_CCL); //(0X00002400)
		IVE_BLK_BA[dev_id].CCL_SRC_RDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_CCL_SRC_RDMA); //(0X00002440)
		IVE_BLK_BA[dev_id].CCL_DST_WDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_CCL_DST_WDMA); //(0X00002480)
		IVE_BLK_BA[dev_id].CCL_REGION_WDMA =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_CCL_REGION_WDMA); //(0X000024C0)
		IVE_BLK_BA[dev_id].CCL_SRC_RDMA_RELABEL =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_CCL +
					 0x100); //(0X000024C0)
		IVE_BLK_BA[dev_id].CCL_DST_WDMA_RELABEL =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_CCL +
					 0x140); //(0X000024C0)
		IVE_BLK_BA[dev_id].DMAF =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_DMAF); //(0X00002600)
		IVE_BLK_BA[dev_id].DMAF_WDMA = (void __iomem *)(ive_phy_base + IVE_BLK_BA_DMAF +
							0x40); //(0X00002640)
		IVE_BLK_BA[dev_id].DMAF_RDMA = (void __iomem *)(ive_phy_base + IVE_BLK_BA_DMAF +
							0x80); //(0X00002680)
		IVE_BLK_BA[dev_id].LK =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_LK); //(0X00002700)
		IVE_BLK_BA[dev_id].RDMA_EIGVAL =
			(void __iomem *)(ive_phy_base +
					 IVE_BLK_BA_RDMA_EIGVAL); //(0X00002800)
		IVE_BLK_BA[dev_id].WDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_WDMA); //(0X00002900)
		IVE_BLK_BA[dev_id].RDMA =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_RDMA); //(0X00002A00)
		IVE_BLK_BA[dev_id].CMDQ =
			(void __iomem *)(ive_phy_base + IVE_BLK_BA_CMDQ); //(0X00003000)
#ifdef DEBUG
		TRACE_IVE(IVE_DBG_INFO, "[IVE] assign_block_addr, Virt Addr : 0x%08x, Phy Addr : 0x%08x\n",
			ive_phy_base, ive_phy_base - g_phy_shift[dev_id]);
		TRACE_IVE(IVE_DBG_INFO, "Dump IVE Register\n");
		array = (uintptr_t *)&IVE_BLK_BA[dev_id];
		for (i = 0; i < 53; i++) {
			TRACE_IVE(IVE_DBG_INFO, "%s : 0x%x\n", IveRegister[i], array[i] - g_phy_shift[dev_id]);
		}
#endif
		return SUCCESS;
	}


void cmdq_set_package(struct cmdq_set_reg *set, u32 addr, u32 data)
{
	set->data = data;
	set->addr = addr >> 2;
	set->byte_mask = 0xf;
	set->action = CMDQ_SET_REG;
}

void cmdq_engine(cmdq_c *ive_cmdq_c, uintptr_t tbl_addr, u16 apb_base,
		bool is_hw_restart, bool is_adma, u16 cnt, s32 dev_id)
{
	// adma or cmdq_set tbl addr
	ive_cmdq_c->dma_addr_l.reg_dma_addr_l = (tbl_addr & 0xFFFFFFFF);
	ive_cmdq_c->dma_addr_h.reg_dma_addr_h = (uint32_t)((uint64_t)tbl_addr >> 32);

	writel(ive_cmdq_c->dma_addr_l.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_ADDR));
	writel(ive_cmdq_c->dma_addr_h.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_ADDR + 4));

	if (!is_adma) {
		ive_cmdq_c->dma_cnt.reg_dma_cnt = cnt<<3;
		ive_cmdq_c->dma_config.reg_adma_en = 0;
		writel(ive_cmdq_c->dma_cnt.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_CNT));
	} else {
		ive_cmdq_c->dma_config.reg_adma_en = 1;
	}
	writel(ive_cmdq_c->dma_config.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_CONFIG));
	ive_cmdq_c->apb_para.reg_base_addr = apb_base;
	writel(ive_cmdq_c->apb_para.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_APB_PARA));
	// job start
	ive_cmdq_c->job_ctl.reg_restart_hw_mod = is_hw_restart;
	ive_cmdq_c->job_ctl.reg_job_start = 1;

	writel(ive_cmdq_c->job_ctl.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_JOB_CTL));
}

void cmdq_adma_package(struct cmdq_adma *item, u64 addr, u32 size,
		bool is_link, bool is_end)
{
	item->addr = addr;
	item->size = size;
	item->flags_end = is_end ? 1 : 0;
	item->flags_link = is_link ? 1 : 0;
}

s32 ive_cmdq(struct ive_device *ndev, s32 dev_id)
{
	return SUCCESS;  //A2 to do

#if IVE_CMDQ
	s32 n = 0;
	s32 loop = 20000;
	s32 test_case = 0;
	s32 tbl_idx = 0;
	u32 u32CoeffTbl[12] = {
		0x12345678,
		0x90abcd00,
		0x02040608,
		0xef000078,
		0x10305070,
		0x0a0b0c0d,
		0x01234568,
		0x00abcded,
		0x12345678,
		0x90abcd00,
		0x02040608,
		0xef000078
	};


	void *u64Cmdq_set = vmalloc(16*2 + 8 * 16);
	bool cmd_end_bit = 0;
	struct cmdq_adma *adma = (struct cmdq_adma *)(u64Cmdq_set);
	union cmdq_set *set = (union cmdq_set *)(u64Cmdq_set + 0x20);
	union cmdq_set *set_2 = (union cmdq_set *)(u64Cmdq_set + 0x40);
	u8 size = 4;
	//DEFINE_CMDQ_C(ive_cmdq_c);
	cmdq_c ive_cmdq_c = _DEFINE_CMDQ_C;

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_CMDQ\n");
	cmdq_printk(&ive_cmdq_c);

	ive_cmdq_c.int_en.reg_cmdq_int_en = 1;
	ive_cmdq_c.int_en.reg_cmdq_end_en = 1;
	ive_cmdq_c.int_en.reg_cmdq_wait_en = 1;

	writel(ive_cmdq_c.int_en.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_INT_EN));

	memset(u64Cmdq_set, 0, 16 * 2 + 8 * 16);
	cmdq_set_package(&set[0].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x2240, u32CoeffTbl[tbl_idx++]);
	cmdq_set_package(&set[1].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x2244, u32CoeffTbl[tbl_idx++]);
	cmdq_set_package(&set[2].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x2248, u32CoeffTbl[tbl_idx++]);
	cmdq_set_package(&set[3].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x224c, u32CoeffTbl[tbl_idx++]);
	set[3].reg.intr_int = 1;
	set[3].reg.intr_end = 1;
	set[3].reg.intr_last = 1;

	cmdq_set_package(&set_2[0].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x2220, u32CoeffTbl[tbl_idx++]);
	cmdq_set_package(&set_2[1].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x2224, u32CoeffTbl[tbl_idx++]);
	cmdq_set_package(&set_2[2].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x2228, u32CoeffTbl[tbl_idx++]);
	cmdq_set_package(&set_2[3].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
		 + 0x222c, u32CoeffTbl[tbl_idx++]);
	set_2[3].reg.intr_int = 1;
	set_2[3].reg.intr_end = 1;
	set_2[3].reg.intr_last = 1;

	for (test_case = 0; test_case < 3; test_case++) {
		loop = 20000;
		n = 0;
		if (test_case == 2) {
			memset(u64Cmdq_set, 0, 16 * 2 + 8 * 16);
			tbl_idx = 3;
			cmdq_set_package(&set[0].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x2240, u32CoeffTbl[tbl_idx++]);
			cmdq_set_package(&set[1].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x2244, u32CoeffTbl[tbl_idx++]);
			cmdq_set_package(&set[2].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x2248, u32CoeffTbl[tbl_idx++]);
			cmdq_set_package(&set[3].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x224c, u32CoeffTbl[tbl_idx++]);
			set[3].reg.intr_int = 1;
			set[3].reg.intr_end = 1;
			set[3].reg.intr_last = 1;

			cmdq_set_package(&set_2[0].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x2220, u32CoeffTbl[tbl_idx++]);
			cmdq_set_package(&set_2[1].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x2224, u32CoeffTbl[tbl_idx++]);
			cmdq_set_package(&set_2[2].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x2228, u32CoeffTbl[tbl_idx++]);
			cmdq_set_package(&set_2[3].reg, (uintptr_t)IVE_TOP_PHY_REG_BASE
				 + 0x222c, u32CoeffTbl[tbl_idx++]);
			set_2[3].reg.intr_int = 1;
			set_2[3].reg.intr_end = 0;
			set_2[3].reg.intr_last = 1;
		}

		switch (test_case) {
		case 0:
			cmdq_engine(&ive_cmdq_c, (uintptr_t)set,
				(uintptr_t)IVE_TOP_PHY_REG_BASE >> 22, false, false, size);
			break;
		case 1:
			cmdq_engine(&ive_cmdq_c, (uintptr_t)set_2,
				(uintptr_t)IVE_TOP_PHY_REG_BASE >> 22, false, false, size);
			break;
		case 2:
			n = 3;
			cmdq_adma_package(adma, (uintptr_t)(set_2), 8*size,
				false, false);
			cmdq_adma_package(adma+1, (uintptr_t)(set), 8*size,
				false, true);
			cmdq_engine(&ive_cmdq_c, (uintptr_t)adma,
				(uintptr_t)IVE_TOP_PHY_REG_BASE >> 22, false, true, 0);
			break;
		default:
			cmdq_adma_package(adma, (uintptr_t)(set_2), 8*size,
				false, false);
			cmdq_adma_package(adma+1, (uintptr_t)(set), 8*size,
				false, true);
			cmdq_engine(&ive_cmdq_c, (uintptr_t)adma,
				(uintptr_t)IVE_TOP_PHY_REG_BASE >> 22, false, true, 0);
			break;
		}

		// busy wait, for ONLY one channel output
		while (!cmd_end_bit && loop--) {
			cmd_end_bit = (readl(IVE_BLK_BA[dev_id].CMDQ + CMDQ_INT_EVENT) &
				 CMDQ_REG_CMDQ_END_MASK) >> CMDQ_REG_CMDQ_END_OFFSET;
		}

		if (loop < 0)
			pr_info("cant wait for reg_cmdq_end, loop %d\n", loop);
		if (readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_11) == u32CoeffTbl[n++] &&
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_12) == u32CoeffTbl[n++] &&
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_13) == u32CoeffTbl[n++] &&
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_14) == u32CoeffTbl[n++]) {
			pr_info("test_case %d PASS1\n", test_case);
		}
		if (readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4) == u32CoeffTbl[n++] &&
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5) == u32CoeffTbl[n++] &&
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6) == u32CoeffTbl[n++] &&
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7) == u32CoeffTbl[n++]) {
			pr_info("test_case %d PASS2\n", test_case);
		}
		pr_info("0x20: 0x%08X\t0x%08X\t0x%08X\t0x%08X\n"
			"0x40: 0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_11),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_12),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_13),
			readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_14));
	}
	return SUCCESS;
#endif
}

s32 ive_dma(struct ive_device *ndev, ive_data_s *pstSrc,
			ive_data_s *pstDst, ive_dma_ctrl_s *pstDmaCtrl,
			bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	s32 i = 0;
	s32 size = 0;
	uint32_t *array;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_DMA_C(ive_dma_c);
	ive_dma_c ive_dma_c_f = _DEFINE_IVE_DMA_C;
	//DEFINE_IVE_DMA_C(DMA);
	ive_dma_c DMA = _DEFINE_IVE_DMA_C;
	//DEFINE_isp_dma_ctl_c(wdma_dma_ctl_c);
	isp_dma_ctl_c wdma_dma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(rdma_dma_ctl_c);
	isp_dma_ctl_c rdma_dma_ctl_c = _DEFINE_isp_dma_ctl_c;

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_DMA\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_data("pstSrc", pstSrc);
		dump_ive_data("pstDst", pstDst);
	}

	if (pstDst == NULL) {
		if (pstDmaCtrl->mode == IVE_DMA_MODE_SET_3BYTE ||
			pstDmaCtrl->mode == IVE_DMA_MODE_SET_8BYTE) {
			pr_err("[IVE] not supper DMA mode of 3BYTE and 8BYTE\n");
			return FAILURE;
		}
		pstDst = pstSrc;
	}
	ive_set_wh(&ive_top_c, pstSrc->width, pstSrc->height, "DMA", dev_id);
	ive_top_c.reg_h10.reg_dmaf_top_enable = 1;
	writel(ive_top_c.reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	ive_dma_c_f.reg_0.reg_ive_dma_enable = 1;
	ive_dma_c_f.reg_0.reg_force_clk_enable = 1;
	ive_dma_c_f.reg_0.reg_ive_dma_mode = pstDmaCtrl->mode;
	writel(ive_dma_c_f.reg_0.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_0));
	ive_dma_c_f.reg_4.reg_ive_dma_horsegsize = pstDmaCtrl->hor_seg_size;
	ive_dma_c_f.reg_4.reg_ive_dma_versegrow = pstDmaCtrl->ver_seg_rows;
	ive_dma_c_f.reg_4.reg_ive_dma_elemsize = pstDmaCtrl->elem_size;
	writel(ive_dma_c_f.reg_4.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_4));
	ive_dma_c_f.reg_1.reg_ive_dma_src_stride = (u16)pstSrc->stride;
	ive_dma_c_f.reg_1.reg_ive_dma_dst_stride = (u16)pstDst->stride;
	writel(ive_dma_c_f.reg_1.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_1));

	ive_dma_c_f.reg_5.reg_ive_dma_u64_val[0] =
		(u32)(pstDmaCtrl->val & 0xffffffff);
	ive_dma_c_f.reg_5.reg_ive_dma_u64_val[1] =
		(u32)((pstDmaCtrl->val >> 32) & 0xffffffff);
	writel(ive_dma_c_f.reg_5.val[0], (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_5));
	writel(ive_dma_c_f.reg_5.val[1],
		   (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_5 + 0x04));

	//Use sw mode for now
	if (0) {
		rdma_dma_ctl_c.sys_control.reg_seglen_sel = 1;
		rdma_dma_ctl_c.sys_control.reg_segnum_sel = 1;
		rdma_dma_ctl_c.sys_control.reg_stride_sel = 1;
		rdma_dma_ctl_c.dma_segnum.reg_segnum = pstSrc->height;
		rdma_dma_ctl_c.dma_seglen.reg_seglen = pstSrc->width;
		rdma_dma_ctl_c.dma_stride.reg_stride = pstSrc->stride;

		wdma_dma_ctl_c.sys_control.reg_seglen_sel = 1;
		wdma_dma_ctl_c.sys_control.reg_segnum_sel = 1;
		wdma_dma_ctl_c.sys_control.reg_stride_sel = 1;
		wdma_dma_ctl_c.dma_segnum.reg_segnum = pstDst->height;
		wdma_dma_ctl_c.dma_seglen.reg_seglen = pstDst->width;
		wdma_dma_ctl_c.dma_stride.reg_stride = pstDst->stride;
	} else { // hw mode has issue since r12611
		rdma_dma_ctl_c.sys_control.reg_seglen_sel = 0;
		rdma_dma_ctl_c.sys_control.reg_segnum_sel = 0;
		rdma_dma_ctl_c.sys_control.reg_stride_sel = 0;
		rdma_dma_ctl_c.dma_segnum.reg_segnum = 0;
		rdma_dma_ctl_c.dma_seglen.reg_seglen = 0;
		rdma_dma_ctl_c.dma_stride.reg_stride = 0;

		wdma_dma_ctl_c.sys_control.reg_seglen_sel = 0;
		wdma_dma_ctl_c.sys_control.reg_segnum_sel = 0;
		wdma_dma_ctl_c.sys_control.reg_stride_sel = 0;
		wdma_dma_ctl_c.dma_segnum.reg_segnum = 0;
		wdma_dma_ctl_c.dma_seglen.reg_seglen = 0;
		wdma_dma_ctl_c.dma_stride.reg_stride = 0;
	}
	writel(rdma_dma_ctl_c.dma_segnum.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_SEGNUM));
	writel(rdma_dma_ctl_c.dma_seglen.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_SEGLEN));
	writel(rdma_dma_ctl_c.dma_stride.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(wdma_dma_ctl_c.dma_segnum.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_SEGNUM));
	writel(wdma_dma_ctl_c.dma_seglen.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_SEGLEN));
	writel(wdma_dma_ctl_c.dma_stride.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_STRIDE));

	ive_top_c.reg_1.reg_fmt_vld_fg = 1;
	writel(ive_top_c.reg_1.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_1));

	if (ive_dma_c_f.reg_0.reg_ive_dma_mode == IVE_DMA_MODE_SET_3BYTE ||
		ive_dma_c_f.reg_0.reg_ive_dma_mode == IVE_DMA_MODE_SET_8BYTE) {
		rdma_dma_ctl_c.base_addr.reg_basel = 0;
		rdma_dma_ctl_c.sys_control.reg_baseh = 0;
		rdma_dma_ctl_c.sys_control.reg_base_sel = 0;
		rdma_dma_ctl_c.dma_stride.reg_stride = 0;
		rdma_dma_ctl_c.sys_control.reg_stride_sel = 0;
	} else {
		rdma_dma_ctl_c.base_addr.reg_basel =
			pstSrc->phy_addr & 0xffffffff;
		rdma_dma_ctl_c.sys_control.reg_baseh =
			((u64)pstSrc->phy_addr >> 32) & 0xffffffff;
		rdma_dma_ctl_c.dma_stride.reg_stride = pstSrc->stride;
		rdma_dma_ctl_c.sys_control.reg_stride_sel = 1;
		rdma_dma_ctl_c.sys_control.reg_base_sel = 1;
	}
	writel(rdma_dma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_dma_ctl_c.dma_stride.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(rdma_dma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_SYS_CONTROL));

	wdma_dma_ctl_c.base_addr.reg_basel = pstDst->phy_addr & 0xffffffff;
	wdma_dma_ctl_c.sys_control.reg_baseh =
		((u64)pstDst->phy_addr >> 32) & 0xffffffff;
	wdma_dma_ctl_c.dma_stride.reg_stride = pstDst->stride;
	wdma_dma_ctl_c.sys_control.reg_stride_sel = 1;
	wdma_dma_ctl_c.sys_control.reg_base_sel = 1;
	writel(wdma_dma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_dma_ctl_c.dma_stride.val,
			(IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(wdma_dma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (g_dump_dma_info == TRUE) {
		pr_info("Src address: 0x%08x %08x\n",
			rdma_dma_ctl_c.sys_control.reg_baseh,
			rdma_dma_ctl_c.base_addr.reg_basel);
		pr_info("Dst address: 0x%08x %08x\n",
			wdma_dma_ctl_c.sys_control.reg_baseh,
			wdma_dma_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[RDMA_DMA].addr_en = rdma_dma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_DMA].addr_l = rdma_dma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_DMA].addr_h = rdma_dma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_DMA].addr_en = wdma_dma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_DMA].addr_l = wdma_dma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_DMA].addr_h = wdma_dma_ctl_c.sys_control.reg_baseh & 0xff;

	ret = ive_go(ndev, &ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_DMAF_MASK, MOD_DMA, dev_id);

	// add in version 9c2fe24
	ive_dma_c_f.reg_0.reg_ive_dma_enable = 0;
	writel(ive_dma_c_f.reg_0.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_0));
	rdma_dma_ctl_c.sys_control.reg_base_sel = 0;
	rdma_dma_ctl_c.base_addr.reg_basel = 0;
	rdma_dma_ctl_c.sys_control.reg_baseh = 0;
	wdma_dma_ctl_c.sys_control.reg_base_sel = 0;
	wdma_dma_ctl_c.base_addr.reg_basel = 0;
	wdma_dma_ctl_c.sys_control.reg_baseh = 0;
	writel(rdma_dma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_dma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(wdma_dma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_dma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	size = sizeof(ive_dma_c) / sizeof(uint32_t);
	array = (uint32_t *)&DMA;
	for (i = 0; i < size; i++) {
		writel(array[i],
				(IVE_BLK_BA[dev_id].DMAF + sizeof(uint32_t) * i));
	}
	return ret;
}

s32 ive_add(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			ive_add_ctrl_s *pstAddCtrl, bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "[IVE] MPI_IVE_Add\n");
	return ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, instant, MOD_ADD,
				   (void *)pstAddCtrl, dev_id); // 3 add 2 = 0  #2
}

s32 ive_and(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "[IVE] MPI_IVE_And\n");
	return ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, instant, MOD_AND,
				   NULL, dev_id); // 3 and 2 = 0  #2
}

s32 ive_or(struct ive_device *ndev, ive_src_image_s *pstSrc1,
		   ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
		   bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "[IVE] MPI_IVE_Or\n");
	return ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, instant, MOD_OR,
				   NULL, dev_id); // 3 or 2 = 2  #3
}

s32 ive_sub(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			ive_sub_ctrl_s *pstSubCtrl, bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "[IVE] MPI_IVE_Sub\n");
	return ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, instant, MOD_SUB,
				   (void *)pstSubCtrl, dev_id); // 3 - 2 = 2  #1
}

s32 ive_xor(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "[IVE] MPI_IVE_Xor\n");
	return ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, instant, MOD_XOR,
				   NULL, dev_id); //3 xor 2 = 2  #1
}

s32 ive_thresh(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst, ive_thresh_ctrl_s *pstThrCtrl,
			   bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "[IVE] MPI_IVE_Thresh\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}

	// top
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "Threshold", dev_id);

	ive_top_c->reg_h14c.reg_thresh_u8bit_thr_l = pstThrCtrl->low_thr;
	ive_top_c->reg_h14c.reg_thresh_u8bit_thr_h = pstThrCtrl->high_thr;
	ive_top_c->reg_h14c.reg_thresh_enmode = pstThrCtrl->mode;
	writel(ive_top_c->reg_h14c.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H14C));
	ive_top_c->reg_h150.reg_thresh_u8bit_min = pstThrCtrl->min_val;
	ive_top_c->reg_h150.reg_thresh_u8bit_mid = pstThrCtrl->mid_val;
	ive_top_c->reg_h150.reg_thresh_u8bit_max = pstThrCtrl->max_val;
	writel(ive_top_c->reg_h150.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H150));
	ive_top_c->reg_h130.reg_thresh_thresh_en = 1;
	ive_top_c->reg_h130.reg_thresh_top_mod = 0;
	writel(ive_top_c->reg_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));

	set_isp_rdma(MOD_THRESH, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	ive_top_c->reg_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	ive_top_c->reg_h130.reg_thresh_thresh_en = 1;
	writel(ive_top_c->reg_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));

	//bypass filterop...
	ive_filterop_c.reg_h10.reg_filterop_mode = 2;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.reg_28.reg_filterop_op2_erodila_en = 0;
	writel(ive_filterop_c.reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_THRESH,
				instant, dev_id);

		kfree(ive_top_c);
		return ret;
	}

	ret = ive_go(ndev, ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_THRESH, dev_id);

	kfree(ive_top_c);

	return ret;
}

s32 erode_dilate_op(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstDst, u8 *mask,
			bool instant, s32 op, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	if (op == MOD_DILA) {
		ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "Dilate", dev_id);
		ive_filterop_c.reg_h10.reg_filterop_mode = 2; // op2 erode or dilate
	} else if (op == MOD_ERO) {
		ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "Erode", dev_id);
		ive_filterop_c.reg_h10.reg_filterop_mode = 3; // op2 erode or dilate
	}
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0; // op1 bypass
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1; // op1 en
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.reg_28.reg_filterop_op2_erodila_en = 1; // op2 en
	writel(ive_filterop_c.reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	if (ive_filterop_c.reg_28.reg_filterop_op2_erodila_en == 0) {
		ive_filterop_c.reg_4.reg_filterop_h_coef00 = mask[0];
		ive_filterop_c.reg_4.reg_filterop_h_coef01 = mask[1];
		ive_filterop_c.reg_4.reg_filterop_h_coef02 = mask[2];
		ive_filterop_c.reg_4.reg_filterop_h_coef03 = mask[3];
		writel(ive_filterop_c.reg_4.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
		ive_filterop_c.reg_5.reg_filterop_h_coef04 = mask[4];
		ive_filterop_c.reg_5.reg_filterop_h_coef10 = mask[5];
		ive_filterop_c.reg_5.reg_filterop_h_coef11 = mask[6];
		ive_filterop_c.reg_5.reg_filterop_h_coef12 = mask[7];
		writel(ive_filterop_c.reg_5.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
		ive_filterop_c.reg_6.reg_filterop_h_coef13 = mask[8];
		ive_filterop_c.reg_6.reg_filterop_h_coef14 = mask[9];
		ive_filterop_c.reg_6.reg_filterop_h_coef20 = mask[10];
		ive_filterop_c.reg_6.reg_filterop_h_coef21 = mask[11];
		writel(ive_filterop_c.reg_6.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
		ive_filterop_c.reg_7.reg_filterop_h_coef22 = mask[12];
		ive_filterop_c.reg_7.reg_filterop_h_coef23 = mask[13];
		ive_filterop_c.reg_7.reg_filterop_h_coef24 = mask[14];
		ive_filterop_c.reg_7.reg_filterop_h_coef30 = mask[15];
		writel(ive_filterop_c.reg_7.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
		ive_filterop_c.reg_8.reg_filterop_h_coef31 = mask[16];
		ive_filterop_c.reg_8.reg_filterop_h_coef32 = mask[17];
		ive_filterop_c.reg_8.reg_filterop_h_coef33 = mask[18];
		ive_filterop_c.reg_8.reg_filterop_h_coef34 = mask[19];
		writel(ive_filterop_c.reg_8.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
		ive_filterop_c.reg_9.reg_filterop_h_coef40 = mask[20];
		ive_filterop_c.reg_9.reg_filterop_h_coef41 = mask[21];
		ive_filterop_c.reg_9.reg_filterop_h_coef42 = mask[22];
		ive_filterop_c.reg_9.reg_filterop_h_coef43 = mask[23];
		writel(ive_filterop_c.reg_9.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
		ive_filterop_c.reg_10.reg_filterop_h_coef44 = mask[24];
		writel(ive_filterop_c.reg_10.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));
	} else {
		ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef00 =
			mask[0];
		ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef01 =
			mask[1];
		ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef02 =
			mask[2];
		ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef03 =
			mask[3];
		writel(ive_filterop_c.reg_21.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_21));
		ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef04 =
			mask[4];
		ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef10 =
			mask[5];
		ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef11 =
			mask[6];
		ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef12 =
			mask[7];
		writel(ive_filterop_c.reg_22.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_22));
		ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef13 =
			mask[8];
		ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef14 =
			mask[9];
		ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef20 =
			mask[10];
		ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef21 =
			mask[11];
		writel(ive_filterop_c.reg_23.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_23));
		ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef22 =
			mask[12];
		ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef23 =
			mask[13];
		ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef24 =
			mask[14];
		ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef30 =
			mask[15];
		writel(ive_filterop_c.reg_24.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_24));
		ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef31 =
			mask[16];
		ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef32 =
			mask[17];
		ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef33 =
			mask[18];
		ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef34 =
			mask[19];
		writel(ive_filterop_c.reg_25.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_25));
		ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef40 =
			mask[20];
		ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef41 =
			mask[21];
		ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef42 =
			mask[22];
		ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef43 =
			mask[23];
		writel(ive_filterop_c.reg_26.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_26));
		ive_filterop_c.reg_27.reg_filterop_op2_erodila_coef44 =
			mask[24];
		writel(ive_filterop_c.reg_27.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_27));
	}

	set_isp_rdma(op, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	// FIXME: check default is 0
	ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 0;
	writel(ive_top_c->reg_r2y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, op,
				instant, dev_id);

		kfree(ive_top_c);
		return ret;
	}

	ret = ive_go(ndev, ive_top_c, instant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, op, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 ive_erode(struct ive_device *ndev, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstDst, ive_erode_ctrl_s *pstErodeCtrl,
			  bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Erode\n");
	return erode_dilate_op(ndev, pstSrc, pstDst, pstErodeCtrl->mask,
				   instant, 3, dev_id);
}

s32 ive_dilate(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst,
			   ive_dilate_ctrl_s *pstDilateCtrl, bool instant, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Dilate\n");
	return erode_dilate_op(ndev, pstSrc, pstDst, pstDilateCtrl->mask,
				   instant, 2, dev_id);
}

s32
ive_frame_diff_motion(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstDst,
			ive_frame_diff_motion_ctrl_s *ctrl, bool instant, s32 dev_id)
{
	s32 mode = 0;
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(rdma_img1_ctl_c);
	isp_dma_ctl_c rdma_img1_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "IVE_FrameDiffMotion\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "FrameDiffMotion", dev_id);

	// 1 sub
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 0;
	writel(ive_top_c->reg_r2y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

	ive_top_c->reg_20.reg_frame2op_sub_change_order = 0;
	ive_top_c->reg_20.reg_frame2op_sub_switch_src = 0;
	ive_top_c->reg_20.reg_frame2op_sub_mode = ctrl->sub_mode;

	ive_top_c->reg_20.reg_frame2op_op_mode = 4;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));

	// 2 thresh
	ive_top_c->reg_h14c.reg_thresh_u8bit_thr_l = ctrl->thr_low;
	ive_top_c->reg_h14c.reg_thresh_u8bit_thr_h = ctrl->thr_high;
	ive_top_c->reg_h14c.reg_thresh_enmode = ctrl->thr_mode;
	writel(ive_top_c->reg_h14c.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H14C));
	ive_top_c->reg_h150.reg_thresh_u8bit_min = ctrl->thr_min_val;
	ive_top_c->reg_h150.reg_thresh_u8bit_mid = ctrl->thr_mid_val;
	ive_top_c->reg_h150.reg_thresh_u8bit_max = ctrl->thr_max_val;
	writel(ive_top_c->reg_h150.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H150));

	// 3 Erode Dilate
	ive_top_c->reg_h130.reg_thresh_thresh_en = 1;
	ive_top_c->reg_h130.reg_thresh_top_mod = 0;
	writel(ive_top_c->reg_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	// NOTICE: need to first set it
	ive_top_c->reg_h10.reg_img_in_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ive_filterop_c.reg_h10.reg_filterop_mode = 2;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 3;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.reg_28.reg_filterop_op2_erodila_en = 1;
	writel(ive_filterop_c.reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));
	// 3 Erode
	ive_filterop_c.reg_4.reg_filterop_h_coef00 = ctrl->erode_mask[0];
	ive_filterop_c.reg_4.reg_filterop_h_coef01 = ctrl->erode_mask[1];
	ive_filterop_c.reg_4.reg_filterop_h_coef02 = ctrl->erode_mask[2];
	ive_filterop_c.reg_4.reg_filterop_h_coef03 = ctrl->erode_mask[3];
	writel(ive_filterop_c.reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	ive_filterop_c.reg_5.reg_filterop_h_coef04 = ctrl->erode_mask[4];
	ive_filterop_c.reg_5.reg_filterop_h_coef10 = ctrl->erode_mask[5];
	ive_filterop_c.reg_5.reg_filterop_h_coef11 = ctrl->erode_mask[6];
	ive_filterop_c.reg_5.reg_filterop_h_coef12 = ctrl->erode_mask[7];
	writel(ive_filterop_c.reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	ive_filterop_c.reg_6.reg_filterop_h_coef13 = ctrl->erode_mask[8];
	ive_filterop_c.reg_6.reg_filterop_h_coef14 = ctrl->erode_mask[9];
	ive_filterop_c.reg_6.reg_filterop_h_coef20 = ctrl->erode_mask[10];
	ive_filterop_c.reg_6.reg_filterop_h_coef21 = ctrl->erode_mask[11];
	writel(ive_filterop_c.reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	ive_filterop_c.reg_7.reg_filterop_h_coef22 = ctrl->erode_mask[12];
	ive_filterop_c.reg_7.reg_filterop_h_coef23 = ctrl->erode_mask[13];
	ive_filterop_c.reg_7.reg_filterop_h_coef24 = ctrl->erode_mask[14];
	ive_filterop_c.reg_7.reg_filterop_h_coef30 = ctrl->erode_mask[15];
	writel(ive_filterop_c.reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	ive_filterop_c.reg_8.reg_filterop_h_coef31 = ctrl->erode_mask[16];
	ive_filterop_c.reg_8.reg_filterop_h_coef32 = ctrl->erode_mask[17];
	ive_filterop_c.reg_8.reg_filterop_h_coef33 = ctrl->erode_mask[18];
	ive_filterop_c.reg_8.reg_filterop_h_coef34 = ctrl->erode_mask[19];
	writel(ive_filterop_c.reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	ive_filterop_c.reg_9.reg_filterop_h_coef40 = ctrl->erode_mask[20];
	ive_filterop_c.reg_9.reg_filterop_h_coef41 = ctrl->erode_mask[21];
	ive_filterop_c.reg_9.reg_filterop_h_coef42 = ctrl->erode_mask[22];
	ive_filterop_c.reg_9.reg_filterop_h_coef43 = ctrl->erode_mask[23];
	writel(ive_filterop_c.reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	ive_filterop_c.reg_10.reg_filterop_h_coef44 = ctrl->erode_mask[24];
	writel(ive_filterop_c.reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef00 =
		ctrl->dilate_mask[0];
	ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef01 =
		ctrl->dilate_mask[1];
	ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef02 =
		ctrl->dilate_mask[2];
	ive_filterop_c.reg_21.reg_filterop_op2_erodila_coef03 =
		ctrl->dilate_mask[3];
	writel(ive_filterop_c.reg_21.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_21));
	ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef04 =
		ctrl->dilate_mask[4];
	ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef10 =
		ctrl->dilate_mask[5];
	ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef11 =
		ctrl->dilate_mask[6];
	ive_filterop_c.reg_22.reg_filterop_op2_erodila_coef12 =
		ctrl->dilate_mask[7];
	writel(ive_filterop_c.reg_22.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_22));
	ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef13 =
		ctrl->dilate_mask[8];
	ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef14 =
		ctrl->dilate_mask[9];
	ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef20 =
		ctrl->dilate_mask[10];
	ive_filterop_c.reg_23.reg_filterop_op2_erodila_coef21 =
		ctrl->dilate_mask[11];
	writel(ive_filterop_c.reg_23.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_23));
	ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef22 =
		ctrl->dilate_mask[12];
	ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef23 =
		ctrl->dilate_mask[13];
	ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef24 =
		ctrl->dilate_mask[14];
	ive_filterop_c.reg_24.reg_filterop_op2_erodila_coef30 =
		ctrl->dilate_mask[15];
	writel(ive_filterop_c.reg_24.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_24));
	ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef31 =
		ctrl->dilate_mask[16];
	ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef32 =
		ctrl->dilate_mask[17];
	ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef33 =
		ctrl->dilate_mask[18];
	ive_filterop_c.reg_25.reg_filterop_op2_erodila_coef34 =
		ctrl->dilate_mask[19];
	writel(ive_filterop_c.reg_25.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_25));
	ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef40 =
		ctrl->dilate_mask[20];
	ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef41 =
		ctrl->dilate_mask[21];
	ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef42 =
		ctrl->dilate_mask[22];
	ive_filterop_c.reg_26.reg_filterop_op2_erodila_coef43 =
		ctrl->dilate_mask[23];
	writel(ive_filterop_c.reg_26.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_26));
	ive_filterop_c.reg_27.reg_filterop_op2_erodila_coef44 =
		ctrl->dilate_mask[24];
	writel(ive_filterop_c.reg_27.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_27));

	set_isp_rdma(MOD_BYP, NULL, dev_id);
	if (set_img_src1(pstSrc1, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}
	set_img_src2(pstSrc2, &rdma_img1_ctl_c, dev_id);
	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	mode = ive_get_mod_u8(pstSrc1->type);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		kfree(ive_top_c);
		return FAILURE;
	}
	ive_top_c->reg_3.reg_ive_rdma_img1_mod_u8 = mode;

	ive_top_c->reg_3.reg_imgmux_img0_sel = 0;
	ive_top_c->reg_3.reg_ive_rdma_img1_en = 1;
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	ive_top_c->reg_h10.reg_rdma_img1_top_enable = 1;
	ive_top_c->reg_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	if (pstSrc1->width > 480) {
		ret =  emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, &rdma_img1_ctl_c, NULL, NULL,
				pstSrc1, pstSrc2, NULL, pstDst, NULL, true, 1,
				false, 1, false, MOD_MD, instant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = ive_go(ndev, ive_top_c, instant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_MD, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 gmm_gmm2_op(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstBg, ive_src_image_s *_pstModel,
			ive_dst_image_s *pstFg, ive_top_c *ive_top_c,
			ive_filterop_c *ive_filterop_c, img_in_c *img_in_c,
			ive_gmm_c *ive_gmm_c, isp_dma_ctl_c *gmm_match_wdma_ctl_c,
			isp_dma_ctl_c *gmm_factor_rdma_ctl_c, int op, bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	s32 i = 0;
	s32 num = 0;
	u64 dst0addr;
	u32 u32ModelSize;
	isp_dma_ctl_c *gmm_mod_rdma_ctl_c[5];
	isp_dma_ctl_c *gmm_mod_wdma_ctl_c[5];
	//DEFINE_isp_dma_ctl_c(gmm_mod_rdma_0);
	isp_dma_ctl_c gmm_mod_rdma_0 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_rdma_1);
	isp_dma_ctl_c gmm_mod_rdma_1 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_rdma_2);
	isp_dma_ctl_c gmm_mod_rdma_2 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_rdma_3);
	isp_dma_ctl_c gmm_mod_rdma_3 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_rdma_4);
	isp_dma_ctl_c gmm_mod_rdma_4 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_wdma_0);
	isp_dma_ctl_c gmm_mod_wdma_0 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_wdma_1);
	isp_dma_ctl_c gmm_mod_wdma_1 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_wdma_2);
	isp_dma_ctl_c gmm_mod_wdma_2 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_wdma_3);
	isp_dma_ctl_c gmm_mod_wdma_3 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_mod_wdma_4);
	isp_dma_ctl_c gmm_mod_wdma_4 = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;

	gmm_mod_rdma_ctl_c[0] = &gmm_mod_rdma_0; // IVE_BLK_BA_GMM_MODEL_RDMA_0;
	gmm_mod_rdma_ctl_c[1] = &gmm_mod_rdma_1; // IVE_BLK_BA_GMM_MODEL_RDMA_1;
	gmm_mod_rdma_ctl_c[2] = &gmm_mod_rdma_2; // IVE_BLK_BA_GMM_MODEL_RDMA_2;
	gmm_mod_rdma_ctl_c[3] = &gmm_mod_rdma_3; // IVE_BLK_BA_GMM_MODEL_RDMA_3;
	gmm_mod_rdma_ctl_c[4] = &gmm_mod_rdma_4; // IVE_BLK_BA_GMM_MODEL_RDMA_4;
	gmm_mod_wdma_ctl_c[0] = &gmm_mod_wdma_0; // IVE_BLK_BA_GMM_MODEL_WDMA_0;
	gmm_mod_wdma_ctl_c[1] = &gmm_mod_wdma_1; // IVE_BLK_BA_GMM_MODEL_WDMA_1;
	gmm_mod_wdma_ctl_c[2] = &gmm_mod_wdma_2; // IVE_BLK_BA_GMM_MODEL_WDMA_2;
	gmm_mod_wdma_ctl_c[3] = &gmm_mod_wdma_3; // IVE_BLK_BA_GMM_MODEL_WDMA_3;
	gmm_mod_wdma_ctl_c[4] = &gmm_mod_wdma_4; // IVE_BLK_BA_GMM_MODEL_WDMA_4;
	//ive_src_image_s src[5];

	//base_ion_alloc(&src[0].phy_addr[0], (void **)&src[i].vir_addr[0],
	//	"ive_mesh", pstSrc[0].width * pstSrc[i].height * 8 * 5 + 0x4000, false);

	u32ModelSize = (pstSrc->type == IVE_IMAGE_TYPE_U8C1) ? 8 : 12;
	for (i = 0; i < 5; i++) {
		//if( i == 0 ){
		//	dst0addr = (((src[0].phy_addr[0] & 0xffffffff) / 0x4000) + 1) * 0x4000;
		//} else {
		//	u64 tmp = pstSrc->width * pstSrc->height * u32ModelSize * i;
		//	u64 tmpaddr = (((src[0].phy_addr[0] & 0xffffffff) / 0x4000) + 1) * 0x4000 + tmp;
		//	dst0addr = (((tmpaddr & 0xffffffff) / 0x4000) + 1) * 0x4000 + 0x800 * i;
		//}
		dst0addr =
			(u64)_pstModel->phy_addr[0] +
			pstSrc->width * pstSrc->height * u32ModelSize * i;
		num = (readl(IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13) &
			   IVE_GMM_REG_GMM_GMM2_MODEL_NUM_MASK) >>
			  IVE_GMM_REG_GMM_GMM2_MODEL_NUM_OFFSET;

		if (i < num) {
			gmm_mod_rdma_ctl_c[i]->base_addr.reg_basel =
				dst0addr & 0xffffffff;
			gmm_mod_rdma_ctl_c[i]->sys_control.reg_baseh =
				(dst0addr >> 32) & 0xffffffff;
			/*pr_err("R/W address : 0x%08x%08x\n",
				gmm_mod_rdma_ctl_c[i]->sys_control.reg_baseh,
				gmm_mod_rdma_ctl_c[i]->base_addr.reg_basel);*/
			gmm_mod_rdma_ctl_c[i]->sys_control.reg_base_sel = 1;
			writel(gmm_mod_rdma_ctl_c[i]->base_addr.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_rdma_ctl_c[i]->sys_control.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
			gmm_mod_wdma_ctl_c[i]->base_addr.reg_basel =
				dst0addr & 0xffffffff;
			gmm_mod_wdma_ctl_c[i]->sys_control.reg_baseh =
				(dst0addr >> 32) & 0xffffffff;
			gmm_mod_wdma_ctl_c[i]->sys_control.reg_base_sel = 1;
			writel(gmm_mod_wdma_ctl_c[i]->base_addr.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_wdma_ctl_c[i]->sys_control.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
		} else {
			gmm_mod_rdma_ctl_c[i]->base_addr.reg_basel = 0;
			gmm_mod_rdma_ctl_c[i]->sys_control.reg_baseh = 0;
			gmm_mod_rdma_ctl_c[i]->sys_control.reg_base_sel = 0;
			writel(gmm_mod_rdma_ctl_c[i]->base_addr.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_rdma_ctl_c[i]->sys_control.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
			gmm_mod_wdma_ctl_c[i]->base_addr.reg_basel = 0;
			gmm_mod_wdma_ctl_c[i]->sys_control.reg_baseh = 0;
			gmm_mod_wdma_ctl_c[i]->sys_control.reg_base_sel = 0;
			writel(gmm_mod_wdma_ctl_c[i]->base_addr.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_wdma_ctl_c[i]->sys_control.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
		}
		if (g_dump_dma_info == TRUE) {
			pr_info("[%d]Src GMM address rdma: 0x%08x %08x\n", i,
				gmm_mod_rdma_ctl_c[i]->sys_control.reg_baseh,
				gmm_mod_rdma_ctl_c[i]->base_addr.reg_basel);
			pr_info("Dst GMM address wdma : 0x%08x %08x\n",
				gmm_mod_wdma_ctl_c[i]->sys_control.reg_baseh,
				gmm_mod_wdma_ctl_c[i]->base_addr.reg_basel);
		}
		if (i == 0) {
			g_debug_info.addr[RDMA_MM_MOD].addr_en = gmm_mod_rdma_ctl_c[0]->sys_control.reg_base_sel;
			g_debug_info.addr[RDMA_MM_MOD].addr_l = gmm_mod_rdma_ctl_c[0]->base_addr.reg_basel;
			g_debug_info.addr[RDMA_MM_MOD].addr_h = gmm_mod_rdma_ctl_c[0]->sys_control.reg_baseh & 0xff;

			g_debug_info.addr[WDMA_GMM_MOD].addr_en = gmm_mod_wdma_ctl_c[0]->sys_control.reg_base_sel;
			g_debug_info.addr[WDMA_GMM_MOD].addr_l = gmm_mod_wdma_ctl_c[0]->base_addr.reg_basel;
			g_debug_info.addr[WDMA_GMM_MOD].addr_h = gmm_mod_wdma_ctl_c[0]->sys_control.reg_baseh & 0xff;
		}
	}

	set_isp_rdma(op, NULL, dev_id);
	if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != SUCCESS) {
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 5;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	//bypass filterop...
	ive_filterop_c->reg_h10.reg_filterop_mode = 2;
	ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 0; //sw_ovw; bypass op1
	ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 1;
	ive_filterop_c->reg_28.reg_filterop_op2_erodila_en = 0; //bypass op2
	writel(ive_filterop_c->reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	ive_top_c->reg_3.reg_dma_share_mux_selgmm = 1;
	ive_top_c->reg_h10.reg_gmm_top_enable = 1;
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstFg, &wdma_y_ctl_c, dev_id);
	set_odma(pstBg, ive_filterop_c, pstSrc->width, pstSrc->height, dev_id);

	ive_filterop_c->reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	return emit_bgm_tile(ndev, true, true, op, ive_top_c, ive_filterop_c,
			   img_in_c, ive_gmm_c, &wdma_y_ctl_c,
			   pstBg->stride[0], pstSrc->width,
			   pstSrc->height, pstBg->type, pstSrc->type,
			   instant, NULL, gmm_mod_rdma_ctl_c,
			   gmm_mod_wdma_ctl_c, gmm_match_wdma_ctl_c,
			   gmm_factor_rdma_ctl_c, NULL, NULL, NULL, NULL, NULL,
			   NULL, NULL, NULL, NULL, dev_id);

	//for (i = 0; i < 5; i++) {
	//	if(src[i].phy_addr[0] != NULL)
	//		base_ion_free(src[i].phy_addr[0]);
	//}
	//return ret;

	// NOTICE: last one to trigger it
	ret = ive_go(ndev, ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK |
				 IVE_TOP_REG_FRAME_DONE_GMM_MASK, op, dev_id);

	ive_gmm_c->reg_gmm_13.reg_gmm_gmm2_enable = 0; //9c2fe24
	writel(ive_gmm_c->reg_gmm_13.val,
		   (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
	return ret;
}

s32 ive_gmm(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstFg, ive_dst_image_s *pstBg,
			ive_mem_info_s *pstModel, ive_gmm_ctrl_s *pstGmmCtrl,
			bool instant,s32 dev_id)
{
	s32 ret = 0;
	ive_src_image_s _pstModel;
	ive_gmm_reg_gmm_0_c reg_gmm_0;
	ive_gmm_reg_gmm_6_c reg_gmm_6;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	ive_gmm_c ive_gmm_c = _DEFINE_IVE_GMM_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_GMM\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstFg", pstFg);
		dump_ive_image("pstBg", pstBg);
		dump_ive_mem("pstModel", pstModel);
	}
	if (pstSrc->type != IVE_IMAGE_TYPE_U8C1 &&
		pstSrc->type != IVE_IMAGE_TYPE_U8C3_PACKAGE &&
		pstSrc->type != IVE_IMAGE_TYPE_U8C3_PLANAR) {
		pr_err("pstSrc->type cannot be (%d)\n", pstSrc->type);
		kfree(ive_top_c);
		return FAILURE;
	}
	// top
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "GMM", dev_id);

	// setting
	reg_gmm_0.reg_gmm_learn_rate = pstGmmCtrl->learn_rate;
	reg_gmm_0.reg_gmm_bg_ratio = pstGmmCtrl->bg_ratio;
	reg_gmm_6.val = ive_gmm_c.reg_gmm_6.val;
	ive_gmm_c.reg_gmm_0.val = reg_gmm_0.val;
	ive_gmm_c.reg_gmm_1.reg_gmm_var_thr = pstGmmCtrl->var_thr;
	ive_gmm_c.reg_gmm_2.reg_gmm_noise_var = pstGmmCtrl->noise_var;
	ive_gmm_c.reg_gmm_3.reg_gmm_max_var = pstGmmCtrl->max_var;
	ive_gmm_c.reg_gmm_4.reg_gmm_min_var = pstGmmCtrl->min_var;
	ive_gmm_c.reg_gmm_5.reg_gmm_init_weight = pstGmmCtrl->init_weight;
	writel(ive_gmm_c.reg_gmm_0.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_0));
	writel(ive_gmm_c.reg_gmm_1.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_1));
	writel(ive_gmm_c.reg_gmm_2.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_2));
	writel(ive_gmm_c.reg_gmm_3.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_3));
	writel(ive_gmm_c.reg_gmm_4.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_4));
	writel(ive_gmm_c.reg_gmm_5.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_5));
	reg_gmm_6.reg_gmm_detect_shadow = 0; // enDetectShadow
	reg_gmm_6.reg_gmm_shadow_thr = 0; // u0q8ShadowThr
	reg_gmm_6.reg_gmm_sns_factor = 8; // u8SnsFactor, as from GMM2 sample
	ive_gmm_c.reg_gmm_6.val = reg_gmm_6.val;
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_model_num = pstGmmCtrl->model_num;
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly =
		(pstSrc->type == IVE_IMAGE_TYPE_U8C1) ? 1 : 0;
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_enable = 1;

	writel(ive_gmm_c.reg_gmm_6.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_6));
	writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

	_pstModel.phy_addr[0] = pstModel->phy_addr;
	//_pstModel.vir_addr[0] = pstModel->vir_addr;

	ret = gmm_gmm2_op(ndev, pstSrc, pstBg, &_pstModel, pstFg, ive_top_c,
			   &ive_filterop_c, &img_in_c, &ive_gmm_c, NULL, NULL, MOD_GMM,
			   instant, dev_id);
	kfree(ive_top_c);
	return ret;

}

s32 ive_gmm2(struct ive_device *ndev, ive_src_image_s *pstSrc,
			 ive_src_image_s *pstFactor, ive_dst_image_s *pstFg,
			 ive_dst_image_s *pstBg, ive_dst_image_s *pstMatchModelInfo,
			 ive_mem_info_s *pstModel, ive_gmm2_ctrl_s *pstGmm2Ctrl,
			 bool instant, s32 dev_id)
{
	ive_src_image_s _pstModel;
	u64 dst2addr, src2addr;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	ive_gmm_c ive_gmm_c = _DEFINE_IVE_GMM_C;
	//DEFINE_isp_dma_ctl_c(gmm_match_wdma_ctl_c);
	isp_dma_ctl_c gmm_match_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(gmm_factor_rdma_ctl_c);
	isp_dma_ctl_c gmm_factor_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}


	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_GMM2\n");
	ive_reset(ndev, ive_top_c, dev_id);
	/*pstSrc->stride[0] = 640;
	pstFactor->stride[0] = 640*2;
	pstFg->stride[0] = 640;
	pstBg->stride[0] = 640;
	pstMatchModelInfo->stride[0] = 640;*/
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstFactor", pstFactor);
		dump_ive_image("pstFg", pstFg);
		dump_ive_image("pstBg", pstBg);
		dump_ive_image("pstMatchModelInfo", pstMatchModelInfo);
		dump_ive_mem("pstModel", pstModel);
	}

	if (pstSrc->type != IVE_IMAGE_TYPE_U8C1 &&
		pstSrc->type != IVE_IMAGE_TYPE_U8C3_PACKAGE &&
		pstSrc->type != IVE_IMAGE_TYPE_U8C3_PLANAR) {
		pr_err("pstSrc->type cannot be (%d)\n", pstSrc->type);
		kfree(ive_top_c);
		return FAILURE;
	}

	// top
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "GMM2", dev_id);

	// setting
	ive_gmm_c.reg_gmm_7.reg_gmm2_life_update_factor =
		pstGmm2Ctrl->glb_life_update_factor;
	ive_gmm_c.reg_gmm_7.reg_gmm2_var_rate = pstGmm2Ctrl->var_rate;
	writel(ive_gmm_c.reg_gmm_7.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_7));
	ive_gmm_c.reg_gmm_8.reg_gmm2_freq_redu_factor =
		pstGmm2Ctrl->freq_redu_factor;
	ive_gmm_c.reg_gmm_8.reg_gmm2_max_var = pstGmm2Ctrl->max_var;
	writel(ive_gmm_c.reg_gmm_8.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_8));
	ive_gmm_c.reg_gmm_9.reg_gmm2_min_var = pstGmm2Ctrl->min_var;
	ive_gmm_c.reg_gmm_9.reg_gmm2_freq_add_factor =
		pstGmm2Ctrl->freq_add_factor;
	writel(ive_gmm_c.reg_gmm_9.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_9));
	ive_gmm_c.reg_gmm_10.reg_gmm2_freq_init = pstGmm2Ctrl->freq_init_val;
	ive_gmm_c.reg_gmm_10.reg_gmm2_freq_thr = pstGmm2Ctrl->freq_thr;
	writel(ive_gmm_c.reg_gmm_10.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_10));
	ive_gmm_c.reg_gmm_11.reg_gmm2_life_thr = pstGmm2Ctrl->life_thr;
	ive_gmm_c.reg_gmm_11.reg_gmm2_sns_factor = pstGmm2Ctrl->glb_sns_factor;
	writel(ive_gmm_c.reg_gmm_11.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_11));
	//iveReg->ive_gmm_c->.reg_gmm2_factor = 0;
	ive_gmm_c.reg_gmm_12.reg_gmm2_life_update_factor_mode =
		pstGmm2Ctrl->life_update_factor_mode;
	ive_gmm_c.reg_gmm_12.reg_gmm2_sns_factor_mode =
		pstGmm2Ctrl->sns_factor_mode;
	writel(ive_gmm_c.reg_gmm_12.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_12));
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly =
		(pstSrc->type == IVE_IMAGE_TYPE_U8C1) ? 1 : 0;
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_model_num = pstGmm2Ctrl->model_num;
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_enable = 2;
	writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

	_pstModel.phy_addr[0] = pstModel->phy_addr;
	/*
	 *S0 pstSrc
	 *S1 pstModel
	 *S2 pstFactor
	 *D0 pstBg
	 *D1 pstFg
	 *D2 pstMatchModelInfo
	 */
	dst2addr = pstMatchModelInfo->phy_addr[0];
	src2addr = pstFactor->phy_addr[0];

	if (dst2addr) {
		gmm_match_wdma_ctl_c.base_addr.reg_basel =
			dst2addr & 0xffffffff;
		gmm_match_wdma_ctl_c.sys_control.reg_baseh =
			(dst2addr >> 32) & 0xffffffff;
		gmm_match_wdma_ctl_c.sys_control.reg_base_sel = 1;
		gmm_match_wdma_ctl_c.dma_stride.reg_stride = pstMatchModelInfo->stride[0];
	} else {
		gmm_match_wdma_ctl_c.sys_control.reg_base_sel = 0;
		gmm_match_wdma_ctl_c.base_addr.reg_basel = 0;
		gmm_match_wdma_ctl_c.sys_control.reg_baseh = 0;
		gmm_match_wdma_ctl_c.dma_stride.reg_stride = 0;
	}
	writel(gmm_match_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(gmm_match_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(gmm_match_wdma_ctl_c.dma_stride.val,
			(IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_DMA_STRIDE));
	if (g_dump_dma_info == TRUE) {
		pr_info("GMM WDMA address: 0x%08x %08x\n",
			gmm_match_wdma_ctl_c.sys_control.reg_baseh,
			gmm_match_wdma_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[WDMA_GMM_MATCH].addr_en = gmm_match_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_GMM_MATCH].addr_l = gmm_match_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_GMM_MATCH].addr_h = gmm_match_wdma_ctl_c.sys_control.reg_baseh & 0xff;
	if (src2addr) {
		gmm_factor_rdma_ctl_c.base_addr.reg_basel =
			src2addr & 0xffffffff;
		gmm_factor_rdma_ctl_c.sys_control.reg_baseh =
			(src2addr >> 32) & 0xffffffff;
		gmm_factor_rdma_ctl_c.sys_control.reg_base_sel = 1;
		gmm_factor_rdma_ctl_c.dma_stride.reg_stride = pstFactor->stride[0];
	} else {
		gmm_factor_rdma_ctl_c.sys_control.reg_base_sel = 0;
		gmm_factor_rdma_ctl_c.base_addr.reg_basel = 0;
		gmm_factor_rdma_ctl_c.sys_control.reg_baseh = 0;
		gmm_factor_rdma_ctl_c.dma_stride.reg_stride = 0;
	}
	writel(gmm_factor_rdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(gmm_factor_rdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(gmm_factor_rdma_ctl_c.dma_stride.val,
			(IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_DMA_STRIDE));

	if (g_dump_dma_info == TRUE) {
		pr_info("GMM FACTOR RDMA address: 0x%08x %08x\n",
			gmm_factor_rdma_ctl_c.sys_control.reg_baseh,
			gmm_factor_rdma_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[RDMA_MM_FACTOR].addr_en = gmm_factor_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_MM_FACTOR].addr_l = gmm_factor_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_MM_FACTOR].addr_h = gmm_factor_rdma_ctl_c.sys_control.reg_baseh & 0xff;
	gmm_gmm2_op(ndev, pstSrc, pstBg, &_pstModel, pstFg, ive_top_c,
			&ive_filterop_c, &img_in_c, &ive_gmm_c,
			&gmm_match_wdma_ctl_c, &gmm_factor_rdma_ctl_c, MOD_GMM2, instant, dev_id);

	kfree(ive_top_c);
	return SUCCESS;
}

s32 ive_match_bg_model(struct ive_device *ndev,
				 ive_src_image_s *pstCurImg, ive_data_s *pstBgModel,
				 ive_image_s *pstFgFlag, ive_dst_image_s *pstDiffFg,
				 ive_bg_stat_data_s *pstStatData,
				 ive_match_bg_model_ctrl_s *pstMatchBgModelCtrl,
				 bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	s32 i = 0;
	u64 dst0addr;
	ive_src_image_s _pstFgFlag;
	ive_src_image_s _pstBgModel;
	u64 src2addr, src3addr, src2addr_1;
	// ive_bg_stat_data_s stat;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
	ive_match_bg_c ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
	//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
	ive_update_bg_c ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;
	//DEFINE_isp_dma_ctl_c(fgflag_rdma_ctl_c);
	isp_dma_ctl_c fgflag_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_0_rdma_ctl_c);
	isp_dma_ctl_c bgmodel_0_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_1_rdma_ctl_c);
	isp_dma_ctl_c bgmodel_1_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(difffg_wdma_ctl_c);
	isp_dma_ctl_c difffg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(fg_wdma_ctl_c);
	isp_dma_ctl_c fg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_0_wdma_ctl_c);
	isp_dma_ctl_c bgmodel_0_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_1_wdma_ctl_c);
	isp_dma_ctl_c bgmodel_1_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	ive_top_c *ive_top_c = NULL;
	ive_filterop_c *ive_filterop_c = NULL;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c = init_ive_filterop_c();
	if (!ive_filterop_c) {
		pr_err("ive_filterop_c init failed\n");
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_MatchBgModel\n");

	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstCurImg", pstCurImg);
		dump_ive_data("pstBgModel", pstBgModel);
		dump_ive_image("pstFgFlag", pstFgFlag);
		dump_ive_image("pstDiffFg", pstDiffFg);
		// dump_ive_mem("pstStatData", pstStatData);
	}

	ive_reset(ndev, ive_top_c, dev_id);
	// top
	ive_set_wh(ive_top_c, pstCurImg->width, pstCurImg->height, "MatchBgModel", dev_id);

	ive_match_bg_c.reg_00.reg_matchbg_softrst = 0;
	writel(ive_match_bg_c.reg_00.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	//+0x80
	ive_match_bg_c.reg_04.reg_matchbg_curfrmnum =
		pstMatchBgModelCtrl->cur_frm_num;
	ive_match_bg_c.reg_08.reg_matchbg_prefrmnum =
		pstMatchBgModelCtrl->pre_frm_num;
	writel(ive_match_bg_c.reg_04.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_04));
	writel(ive_match_bg_c.reg_08.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_08));
	ive_match_bg_c.reg_0c.reg_matchbg_timethr =
		pstMatchBgModelCtrl->time_thr;
	ive_match_bg_c.reg_0c.reg_matchbg_diffthrcrlcoef =
		pstMatchBgModelCtrl->diff_thr_crl_coef;
	ive_match_bg_c.reg_0c.reg_matchbg_diffmaxthr =
		pstMatchBgModelCtrl->diff_max_thr;
	ive_match_bg_c.reg_0c.reg_matchbg_diffminthr =
		pstMatchBgModelCtrl->diff_min_thr;
	ive_match_bg_c.reg_0c.reg_matchbg_diffthrinc =
		pstMatchBgModelCtrl->diff_thr_inc;
	ive_match_bg_c.reg_0c.reg_matchbg_fastlearnrate =
		pstMatchBgModelCtrl->fast_learn_rate;
	ive_match_bg_c.reg_0c.reg_matchbg_detchgregion =
		pstMatchBgModelCtrl->det_chg_region;
	writel(ive_match_bg_c.reg_0c.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_0C));
	// TODO: set dma
	set_isp_rdma(MOD_BGM, NULL, dev_id);
	for (i = 0; i < 3; i++) {
		_pstFgFlag.phy_addr[i] = pstFgFlag->phy_addr[i];
		//_pstFgFlag.vir_addr[i] = pstFgFlag->vir_addr[i];
		_pstFgFlag.stride[i] = pstFgFlag->stride[i];
		_pstBgModel.phy_addr[i] = pstBgModel->phy_addr;
		//_pstBgModel.vir_addr[i] = pstBgModel->vir_addr;
		_pstBgModel.stride[i] = pstBgModel->stride;
	}
	/*
	 *S0 pstCurImg
	 *S1 _pstBgModel
	 *S2 _pstFgFlag
	 *D0 pstDiffFg
	 *D1 pstFrmDiffFg
	 *D2
	 */
	src2addr = _pstBgModel.phy_addr[0];
	src2addr_1 =
		src2addr + (pstCurImg->width * pstCurImg->height * 16);
	src3addr = _pstFgFlag.phy_addr[0];

	if (src2addr) {
		bgmodel_0_rdma_ctl_c.base_addr.reg_basel =
			src2addr & 0xffffffff;
		bgmodel_0_rdma_ctl_c.sys_control.reg_baseh =
			(src2addr >> 32) & 0xffffffff;
		bgmodel_0_rdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		bgmodel_0_rdma_ctl_c.sys_control.reg_base_sel = 0;
		bgmodel_0_rdma_ctl_c.base_addr.reg_basel = 0;
		bgmodel_0_rdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(bgmodel_0_rdma_ctl_c.base_addr.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
		ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_rdma_ctl_c.sys_control.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
		ISP_DMA_CTL_SYS_CONTROL));
	if (src2addr_1) {
		bgmodel_1_rdma_ctl_c.base_addr.reg_basel =
			src2addr_1 & 0xffffffff;
		bgmodel_1_rdma_ctl_c.sys_control.reg_baseh =
			(src2addr_1 >> 32) & 0xffffffff;
		bgmodel_1_rdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		bgmodel_1_rdma_ctl_c.sys_control.reg_base_sel = 0;
		bgmodel_1_rdma_ctl_c.base_addr.reg_basel = 0;
		bgmodel_1_rdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(bgmodel_1_rdma_ctl_c.base_addr.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
		ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_rdma_ctl_c.sys_control.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
		ISP_DMA_CTL_SYS_CONTROL));
	if (src3addr) {
		fgflag_rdma_ctl_c.base_addr.reg_basel = src3addr & 0xffffffff;
		fgflag_rdma_ctl_c.sys_control.reg_baseh =
			(src3addr >> 32) & 0xffffffff;
		fgflag_rdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		fgflag_rdma_ctl_c.sys_control.reg_base_sel = 0;
		fgflag_rdma_ctl_c.base_addr.reg_basel = 0;
		fgflag_rdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(fgflag_rdma_ctl_c.base_addr.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
		ISP_DMA_CTL_BASE_ADDR));
	writel(fgflag_rdma_ctl_c.sys_control.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
		ISP_DMA_CTL_SYS_CONTROL));


	if (set_img_src1(pstCurImg, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return FAILURE;
	}

	bgmodel_0_wdma_ctl_c.base_addr.reg_basel = src2addr & 0xffffffff;
	bgmodel_0_wdma_ctl_c.sys_control.reg_baseh =
		(src2addr >> 32) & 0xffffffff;
	bgmodel_0_wdma_ctl_c.sys_control.reg_base_sel = 1;
	writel(bgmodel_0_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	bgmodel_1_wdma_ctl_c.base_addr.reg_basel = src2addr_1 & 0xffffffff;
	bgmodel_1_wdma_ctl_c.sys_control.reg_baseh =
		(src2addr_1 >> 32) & 0xffffffff;
	bgmodel_1_wdma_ctl_c.sys_control.reg_base_sel = 1;
	writel(bgmodel_1_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	fg_wdma_ctl_c.base_addr.reg_basel = src3addr & 0xffffffff;
	fg_wdma_ctl_c.sys_control.reg_baseh = (src3addr >> 32) & 0xffffffff;
	fg_wdma_ctl_c.sys_control.reg_base_sel = 1;
	writel(fg_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(fg_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	////////////////////////
	ive_match_bg_c.reg_00.reg_matchbg_en = 1;
	ive_match_bg_c.reg_00.reg_matchbg_bypass_model = 0;
	ive_update_bg_c.reg_h04.reg_enable = 0;
	ive_update_bg_c.reg_h04.reg_updatebg_byp_model = 1;
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 5;

	ive_top_c->reg_h10.reg_filterop_top_enable = 0;
	ive_top_c->reg_h10.reg_bgm_top_enable = 1;
	ive_top_c->reg_h10.reg_bgu_top_enable = 1;
	writel(ive_match_bg_c.reg_00.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	writel(ive_update_bg_c.reg_h04.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	///////////////////////////
	dst0addr = pstDiffFg->phy_addr[0];

	if (dst0addr) {
		difffg_wdma_ctl_c.base_addr.reg_basel = dst0addr & 0xffffffff;
		difffg_wdma_ctl_c.sys_control.reg_baseh =
			(dst0addr >> 32) & 0xffffffff;
		difffg_wdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		difffg_wdma_ctl_c.sys_control.reg_base_sel = 0;
		difffg_wdma_ctl_c.base_addr.reg_basel = 0;
		difffg_wdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(difffg_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(difffg_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (g_dump_dma_info == TRUE) {
		pr_info("BG_UPDATE_BGMODEL_0_WDMA address: 0x%08x %08x\n",
			bgmodel_0_wdma_ctl_c.sys_control.reg_baseh,
			bgmodel_0_wdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_UPDATE_BGMODEL_1_WDMA address: 0x%08x %08x\n",
			bgmodel_1_wdma_ctl_c.sys_control.reg_baseh,
			bgmodel_1_wdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_UPDATE_FG_WDMA address: 0x%08x %08x\n",
			fg_wdma_ctl_c.sys_control.reg_baseh,
			fg_wdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_MATCH_BGMODEL_0_RDMA address: 0x%08x %08x\n",
			bgmodel_0_rdma_ctl_c.sys_control.reg_baseh,
			bgmodel_0_rdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_MATCH_BGMODEL_1_RDMA address: 0x%08x %08x\n",
			bgmodel_1_rdma_ctl_c.sys_control.reg_baseh,
			bgmodel_1_rdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_MATCH_FGFLAG_RDMA address: 0x%08x %08x\n",
			fgflag_rdma_ctl_c.sys_control.reg_baseh,
			fgflag_rdma_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[WDMA_BGMODEL_0].addr_en = bgmodel_0_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_l = bgmodel_0_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_h = bgmodel_0_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_BGMODEL_1].addr_en = bgmodel_1_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_l = bgmodel_1_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_h = bgmodel_1_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_FG].addr_en = fg_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_FG].addr_l = fg_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_FG].addr_h = fg_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_0].addr_en = bgmodel_0_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_l = bgmodel_0_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_h = bgmodel_0_rdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_1].addr_en = bgmodel_1_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_l = bgmodel_1_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_h = bgmodel_1_rdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GFLAG].addr_en = fgflag_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_GFLAG].addr_l = fgflag_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_GFLAG].addr_h = fgflag_rdma_ctl_c.sys_control.reg_baseh & 0xff;

	ive_filterop_c->odma_reg_00.reg_dma_en = 0;
	ive_filterop_c->reg_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	// ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 0;
	// ive_filterop_c->reg_h14.reg_filterop_3ch_en = 0;
	writel(ive_filterop_c->odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	ret = emit_bgm_tile(ndev, false, false, MOD_BGM, ive_top_c,
			  ive_filterop_c, &img_in_c, NULL, NULL,
			  pstCurImg->stride[0], pstCurImg->width,
			  pstCurImg->height, -1, pstCurImg->type, instant,
			  pstStatData, NULL, NULL, NULL, NULL, &fgflag_rdma_ctl_c,
			  &bgmodel_0_rdma_ctl_c, &difffg_wdma_ctl_c,
			  &bgmodel_1_rdma_ctl_c, &fg_wdma_ctl_c,
			  &bgmodel_0_wdma_ctl_c, &bgmodel_1_wdma_ctl_c, NULL,
			  &ive_update_bg_c, dev_id);


	kfree(ive_top_c);
	kfree(ive_filterop_c);
	return ret;
#if 0
	ret = ive_go(ndev, ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_BGM_MASK, MOD_BGM);

	stat.pix_num = readl(IVE_BLK_BA.BG_MATCH_IVE_MATCH_BG +
				   IVE_MATCH_BG_REG_10); //9c2fe24
	stat.sum_lum = readl(IVE_BLK_BA.BG_MATCH_IVE_MATCH_BG +
				   IVE_MATCH_BG_REG_14); //9c2fe24
	kfree(ive_top_c);
	kfree(ive_filterop_c);
	return ret;
#endif
}

s32 ive_update_bg_model(struct ive_device *ndev,
				  ive_src_image_s *pstCurImg,
				  ive_data_s *pstBgModel, ive_image_s *pstFgFlag,
				  ive_dst_image_s *pstBgImg,
				  ive_dst_image_s *pstChgSta,
				  ive_bg_stat_data_s *pstStatData,
				  ive_update_bg_model_ctrl_s *pstUpdateBgModelCtrl,
				  bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	s32 i;
	ive_dst_image_s _pstFgFlag;
	ive_dst_image_s _pstBgModel;
	u64 src2addr, src3addr, src2addr_1;
	// ive_bg_stat_data_s stat;
	ive_update_bg_reg_ctrl3_c reg_ctrl3;
	ive_update_bg_reg_ctrl5_c reg_ctrl5;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(fg_wdma_ctl_c);
	isp_dma_ctl_c fg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_0_wdma_ctl_c);
	isp_dma_ctl_c bgmodel_0_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_1_wdma_ctl_c);
	isp_dma_ctl_c bgmodel_1_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(fgflag_rdma_ctl_c);
	isp_dma_ctl_c fgflag_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_0_rdma_ctl_c);
	isp_dma_ctl_c bgmodel_0_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(bgmodel_1_rdma_ctl_c);
	isp_dma_ctl_c bgmodel_1_rdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(difffg_wdma_ctl_c);
	isp_dma_ctl_c difffg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(chg_wdma_ctl_c);
	isp_dma_ctl_c chg_wdma_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
	ive_match_bg_c ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
	//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
	ive_update_bg_c ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;
	ive_top_c *ive_top_c = NULL;
	ive_filterop_c *ive_filterop_c = NULL;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c = init_ive_filterop_c();
	if (!ive_filterop_c) {
		pr_err("ive_filterop_c init failed\n");
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_UpdateBgModel\n");

	if (g_dump_image_info == TRUE) {
		dump_ive_data("bg_model", pstBgModel);
		dump_ive_image("fg_flag", pstFgFlag);
		dump_ive_image("bg_img", pstBgImg);
		dump_ive_image("pstChgSta", pstChgSta);
		// dump_ive_mem("stat_data", pstStatData);
	}

	ive_reset(ndev, ive_top_c, dev_id);
	// top
	ive_set_wh(ive_top_c, pstBgImg->width, pstBgImg->height, "UpdateBgModel", dev_id);
	ive_top_c->reg_h10.reg_img_in_top_enable = 0;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ive_update_bg_c.reg_h04.reg_enable = 1;
	ive_update_bg_c.reg_h04.reg_updatebg_byp_model = 0;
	writel(ive_update_bg_c.reg_h04.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));

	reg_ctrl3.reg_u32StyBgMinBlendTime =
		pstUpdateBgModelCtrl->sty_bg_min_blend_time;
	reg_ctrl3.reg_u32StyBgMaxBlendTime =
		pstUpdateBgModelCtrl->sty_bg_max_blend_time;
	reg_ctrl5.reg_u16BgMaxFadeTime = pstUpdateBgModelCtrl->bg_max_fade_time;
	reg_ctrl5.reg_u16FgMaxFadeTime = pstUpdateBgModelCtrl->fg_max_fade_time;
	reg_ctrl5.reg_u8StyBgAccTimeRateThr =
		pstUpdateBgModelCtrl->sty_bg_acc_time_rate_thr;
	reg_ctrl5.reg_u8ChgBgAccTimeRateThr =
		pstUpdateBgModelCtrl->chg_bg_acc_time_rate_thr;

	ive_update_bg_c.reg_ctrl0.reg_u32CurFrmNum =
		pstUpdateBgModelCtrl->cur_frm_num;
	ive_update_bg_c.reg_ctrl1.reg_u32PreChkTime =
		pstUpdateBgModelCtrl->pre_chk_time;
	ive_update_bg_c.reg_ctrl2.reg_u32FrmChkPeriod =
		pstUpdateBgModelCtrl->frm_chk_period;
	ive_update_bg_c.reg_ctrl2.reg_u32InitMinTime =
		pstUpdateBgModelCtrl->init_min_time;
	ive_update_bg_c.reg_ctrl3.val = reg_ctrl3.val;
	ive_update_bg_c.reg_ctrl4.reg_u32DynBgMinBlendTime =
		pstUpdateBgModelCtrl->dyn_bg_min_blend_time;
	ive_update_bg_c.reg_ctrl4.reg_u32StaticDetMinTime =
		pstUpdateBgModelCtrl->static_det_min_time;
	ive_update_bg_c.reg_ctrl5.val = reg_ctrl5.val;
	ive_update_bg_c.reg_ctrl6.reg_u8DynBgAccTimeThr =
		pstUpdateBgModelCtrl->dyn_bg_acc_time_thr;
	ive_update_bg_c.reg_ctrl6.reg_u8DynBgDepth = 3; //fix to 3
	ive_update_bg_c.reg_ctrl6.reg_u8BgEffStaRateThr =
		pstUpdateBgModelCtrl->bg_eff_sta_rate_thr;
	ive_update_bg_c.reg_ctrl6.reg_u8AcceBgLearn =
		pstUpdateBgModelCtrl->acce_bg_learn;
	ive_update_bg_c.reg_ctrl6.reg_u8DetChgRegion =
		pstUpdateBgModelCtrl->det_chg_region;
	writel(ive_update_bg_c.reg_ctrl0.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL0));
	writel(ive_update_bg_c.reg_ctrl1.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL1));
	writel(ive_update_bg_c.reg_ctrl2.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL2));
	writel(ive_update_bg_c.reg_ctrl3.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL3));
	writel(ive_update_bg_c.reg_ctrl4.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL4));
	writel(ive_update_bg_c.reg_ctrl5.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL5));
	writel(ive_update_bg_c.reg_ctrl6.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL6));

	for (i = 0; i < 3; i++) {
		_pstFgFlag.phy_addr[i] = pstFgFlag->phy_addr[i];
		//_pstFgFlag.vir_addr[i] = pstFgFlag->vir_addr[i];
		_pstFgFlag.stride[i] = pstFgFlag->stride[i];
		_pstBgModel.phy_addr[i] = pstBgModel->phy_addr;
		//_pstBgModel.vir_addr[i] = pstBgModel->vir_addr;
		_pstBgModel.stride[i] = pstBgModel->stride;
	}
	/*
	 * S0 NULL
	 * S1 _pstBgModel
	 * S2 _pstFgFlag
	 * D0 pstBgImg
	 * D1 pstChgSta
	 * D2
	 */
	src2addr = _pstBgModel.phy_addr[0];
	src3addr = _pstFgFlag.phy_addr[0];
	src2addr_1 = src2addr + (pstBgImg->width * pstBgImg->height * 16);
	set_isp_rdma(MOD_BGU, NULL, dev_id);
	if (src2addr) {
		bgmodel_0_wdma_ctl_c.base_addr.reg_basel =
			src2addr & 0xffffffff;
		bgmodel_0_wdma_ctl_c.sys_control.reg_baseh =
			(src2addr >> 32) & 0xffffffff;
		bgmodel_0_wdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		bgmodel_0_wdma_ctl_c.sys_control.reg_base_sel = 0;
		bgmodel_0_wdma_ctl_c.base_addr.reg_basel = 0;
		bgmodel_0_wdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(bgmodel_0_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (src2addr_1) {
		bgmodel_1_wdma_ctl_c.base_addr.reg_basel =
			src2addr_1 & 0xffffffff;
		bgmodel_1_wdma_ctl_c.sys_control.reg_baseh =
			(src2addr_1 >> 32) & 0xffffffff;
		bgmodel_1_wdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		bgmodel_1_wdma_ctl_c.sys_control.reg_base_sel = 0;
		bgmodel_1_wdma_ctl_c.base_addr.reg_basel = 0;
		bgmodel_1_wdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(bgmodel_1_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (src3addr) {
		fg_wdma_ctl_c.base_addr.reg_basel = src3addr & 0xffffffff;
		fg_wdma_ctl_c.sys_control.reg_baseh =
			(src3addr >> 32) & 0xffffffff;
		fg_wdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		fg_wdma_ctl_c.sys_control.reg_base_sel = 0;
		fg_wdma_ctl_c.base_addr.reg_basel = 0;
		fg_wdma_ctl_c.sys_control.reg_baseh = 0;
	}

	if (set_img_src1(pstCurImg, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return FAILURE;
	}


	writel(fg_wdma_ctl_c.base_addr.val,
		(IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(fg_wdma_ctl_c.sys_control.val,
		(IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	bgmodel_0_rdma_ctl_c.base_addr.reg_basel = src2addr & 0xffffffff;
	bgmodel_0_rdma_ctl_c.sys_control.reg_baseh =
		(src2addr >> 32) & 0xffffffff;
	bgmodel_0_rdma_ctl_c.sys_control.reg_base_sel = 1;
	writel(bgmodel_0_rdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_rdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_SYS_CONTROL));

	bgmodel_1_rdma_ctl_c.base_addr.reg_basel = src2addr_1 & 0xffffffff;
	bgmodel_1_rdma_ctl_c.sys_control.reg_baseh =
		(src2addr_1 >> 32) & 0xffffffff;
	bgmodel_1_rdma_ctl_c.sys_control.reg_base_sel = 1;
	writel(bgmodel_1_rdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_rdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	if (src3addr) {
		fgflag_rdma_ctl_c.base_addr.reg_basel = src3addr & 0xffffffff;
		fgflag_rdma_ctl_c.sys_control.reg_baseh = (src3addr >> 32) & 0xffffffff;
		fgflag_rdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		fgflag_rdma_ctl_c.sys_control.reg_base_sel = 0;
		fgflag_rdma_ctl_c.base_addr.reg_basel = 0;
		fgflag_rdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(fgflag_rdma_ctl_c.base_addr.val,
		(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(fgflag_rdma_ctl_c.sys_control.val,
		(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	ive_match_bg_c.reg_00.reg_matchbg_en = 1;
	ive_match_bg_c.reg_00.reg_matchbg_bypass_model = 1;
	ive_update_bg_c.reg_h04.reg_enable = 1;
	ive_update_bg_c.reg_h04.reg_updatebg_byp_model = 0;
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	// ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 5;
	writel(ive_match_bg_c.reg_00.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	writel(ive_update_bg_c.reg_h04.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	//bypass filterop..
	// ive_filterop_c->reg_h10.reg_filterop_mode = 2;
	// ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 0;
	// ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 1;
	// ive_filterop_c->reg_28.reg_filterop_op2_erodila_en = 0;
	// writel(ive_filterop_c->reg_h10.val, (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	// writel(ive_filterop_c->reg_h14.val, (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	// writel(ive_filterop_c->reg_28.val, (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	ive_top_c->reg_h10.reg_filterop_top_enable = 0;
	ive_top_c->reg_h10.reg_bgm_top_enable = 1;
	ive_top_c->reg_h10.reg_bgu_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	if (pstChgSta->phy_addr[0]) {
		chg_wdma_ctl_c.base_addr.reg_basel =
			pstChgSta->phy_addr[0] & 0xffffffff;
		chg_wdma_ctl_c.sys_control.reg_baseh =
			(pstChgSta->phy_addr[0] >> 32) & 0xffffffff;
		chg_wdma_ctl_c.sys_control.reg_base_sel = 1;
	} else {
		chg_wdma_ctl_c.sys_control.reg_base_sel = 0;
		chg_wdma_ctl_c.base_addr.reg_basel = 0;
		chg_wdma_ctl_c.sys_control.reg_baseh = 0;
	}
	writel(chg_wdma_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(chg_wdma_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (g_dump_dma_info == TRUE) {
		pr_info("BG_UPDATE_BGMODEL_0_WDMA address: 0x%08x %08x\n",
			bgmodel_0_wdma_ctl_c.sys_control.reg_baseh,
			bgmodel_0_wdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_UPDATE_BGMODEL_1_WDMA address: 0x%08x %08x\n",
			bgmodel_1_wdma_ctl_c.sys_control.reg_baseh,
			bgmodel_1_wdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_UPDATE_FG_WDMA address: 0x%08x %08x\n",
			fg_wdma_ctl_c.sys_control.reg_baseh,
			fg_wdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_MATCH_BGMODEL_0_RDMA address: 0x%08x %08x\n",
			bgmodel_0_rdma_ctl_c.sys_control.reg_baseh,
			bgmodel_0_rdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_MATCH_BGMODEL_1_RDMA address: 0x%08x %08x\n",
			bgmodel_1_rdma_ctl_c.sys_control.reg_baseh,
			bgmodel_1_rdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_MATCH_FGFLAG_RDMA address: 0x%08x %08x\n",
			fgflag_rdma_ctl_c.sys_control.reg_baseh,
			fgflag_rdma_ctl_c.base_addr.reg_basel);
		pr_info("BG_UPDATE_CHG_WDMA address: 0x%08x %08x\n",
			chg_wdma_ctl_c.sys_control.reg_baseh,
			chg_wdma_ctl_c.base_addr.reg_basel);
	}

	g_debug_info.addr[WDMA_BGMODEL_0].addr_en = bgmodel_0_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_l = bgmodel_0_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_h = bgmodel_0_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_BGMODEL_1].addr_en = bgmodel_1_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_l = bgmodel_1_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_h = bgmodel_1_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_FG].addr_en = fg_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_FG].addr_l = fg_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_FG].addr_h = fg_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_0].addr_en = bgmodel_0_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_l = bgmodel_0_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_h = bgmodel_0_rdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_1].addr_en = bgmodel_1_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_l = bgmodel_1_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_h = bgmodel_1_rdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GFLAG].addr_en = fgflag_rdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_GFLAG].addr_l = fgflag_rdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_GFLAG].addr_h = fgflag_rdma_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_CHG].addr_en = chg_wdma_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_CHG].addr_l = chg_wdma_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_CHG].addr_h = chg_wdma_ctl_c.sys_control.reg_baseh & 0xff;

	// set_odma(pstBgImg, ive_filterop_c, pstBgImg->width, pstBgImg->height, dev_id);
	ive_filterop_c->odma_reg_00.reg_dma_en = 0;
	ive_filterop_c->reg_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	// ive_filterop_c->reg_h14.reg_filterop_3ch_en = 1;
	writel(ive_filterop_c->odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));



	ret = emit_bgm_tile(ndev, false, false, MOD_BGU, ive_top_c,
			  ive_filterop_c, &img_in_c, NULL, NULL,
			  pstBgImg->stride[0], pstBgImg->width,
			  pstBgImg->height, -1,
			  pstBgImg->type, instant, pstStatData, NULL, NULL, NULL,
			  NULL, &fgflag_rdma_ctl_c, &bgmodel_0_rdma_ctl_c, &difffg_wdma_ctl_c,
			  &bgmodel_1_rdma_ctl_c, &fg_wdma_ctl_c,
			  &bgmodel_0_wdma_ctl_c, &bgmodel_1_wdma_ctl_c,
			  &chg_wdma_ctl_c, &ive_update_bg_c, dev_id);


	kfree(ive_filterop_c);
	kfree(ive_top_c);
	return ret;
#if 0
	ret = ive_go(ndev, ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_BGU_MASK, MOD_BGU);

	stat.pix_num = readl(IVE_BLK_BA.BG_UPDATE_UPDATE_BG +
				   IVE_UPDATE_BG_REG_CTRL7); //9c2fe24
	stat.sum_lum = readl(IVE_BLK_BA.BG_UPDATE_UPDATE_BG +
				   IVE_UPDATE_BG_REG_CTRL8); //9c2fe24

	//writel(0x00000000, (IVE_BLK_BA.BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	//writel(0x00000002, (IVE_BLK_BA.BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));
	kfree(ive_top_c);
	kfree(ive_filterop_c);
	return ret;
#endif
}

s32 ive_bernsen(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstDst,
			ive_bernsen_ctrl_s *pstBernsenCtrl, bool instant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Bernsen\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	if (pstBernsenCtrl->win_size != 3 && pstBernsenCtrl->win_size != 5) {
		pr_err("not support win_size %d, currently only support 3 or 5\n",
			   pstBernsenCtrl->win_size);
		kfree(ive_top_c);
		return FAILURE;
	}

	// top
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "Bernsen", dev_id);

	ive_filterop_c.reg_h10.reg_filterop_mode = 2;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));

	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 5;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.reg_28.reg_filterop_op2_erodila_en = 0;
	writel(ive_filterop_c.reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));
	ive_filterop_c.reg_19.reg_filterop_bernsen_mode =
		pstBernsenCtrl->mode;
	ive_filterop_c.reg_19.reg_filterop_bernsen_win5x5_en =
		pstBernsenCtrl->win_size == 5;
	ive_filterop_c.reg_19.reg_filterop_bernsen_thr = pstBernsenCtrl->thr;
	ive_filterop_c.reg_19.reg_filterop_u8ContrastThreshold =
		pstBernsenCtrl->contrast_threshold;
	writel(ive_filterop_c.reg_19.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_19));

	set_isp_rdma(MOD_BERNSEN, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}
	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	// FIXME: check default is 0
	ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 0;
	writel(ive_top_c->reg_r2y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_BERNSEN,
				instant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = ive_go(ndev, ive_top_c, instant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_BERNSEN, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 _ive_filter(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstDst, ive_filter_ctrl_s *pstFltCtrl,
			bool instant, ive_top_c *ive_top_c,
			img_in_c *img_in_c, ive_filterop_c *ive_filterop_c,
			bool isEmit, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	ive_gmm_c ive_gmm_c = _DEFINE_IVE_GMM_C;
	// top
	if (isEmit)
		ive_reset(ndev, ive_top_c, dev_id);
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "Filter", dev_id);

	ive_filterop_c->reg_4.reg_filterop_h_coef00 = pstFltCtrl->mask[0];
	ive_filterop_c->reg_4.reg_filterop_h_coef01 = pstFltCtrl->mask[1];
	ive_filterop_c->reg_4.reg_filterop_h_coef02 = pstFltCtrl->mask[2];
	ive_filterop_c->reg_4.reg_filterop_h_coef03 = pstFltCtrl->mask[3];
	ive_filterop_c->reg_5.reg_filterop_h_coef04 = pstFltCtrl->mask[4];
	ive_filterop_c->reg_5.reg_filterop_h_coef10 = pstFltCtrl->mask[5];
	ive_filterop_c->reg_5.reg_filterop_h_coef11 = pstFltCtrl->mask[6];
	ive_filterop_c->reg_5.reg_filterop_h_coef12 = pstFltCtrl->mask[7];
	ive_filterop_c->reg_6.reg_filterop_h_coef13 = pstFltCtrl->mask[8];
	ive_filterop_c->reg_6.reg_filterop_h_coef14 = pstFltCtrl->mask[9];
	ive_filterop_c->reg_6.reg_filterop_h_coef20 = pstFltCtrl->mask[10];
	ive_filterop_c->reg_6.reg_filterop_h_coef21 = pstFltCtrl->mask[11];
	ive_filterop_c->reg_7.reg_filterop_h_coef22 = pstFltCtrl->mask[12];
	ive_filterop_c->reg_7.reg_filterop_h_coef23 = pstFltCtrl->mask[13];
	ive_filterop_c->reg_7.reg_filterop_h_coef24 = pstFltCtrl->mask[14];
	ive_filterop_c->reg_7.reg_filterop_h_coef30 = pstFltCtrl->mask[15];
	ive_filterop_c->reg_8.reg_filterop_h_coef31 = pstFltCtrl->mask[16];
	ive_filterop_c->reg_8.reg_filterop_h_coef32 = pstFltCtrl->mask[17];
	ive_filterop_c->reg_8.reg_filterop_h_coef33 = pstFltCtrl->mask[18];
	ive_filterop_c->reg_8.reg_filterop_h_coef34 = pstFltCtrl->mask[19];
	ive_filterop_c->reg_9.reg_filterop_h_coef40 = pstFltCtrl->mask[20];
	ive_filterop_c->reg_9.reg_filterop_h_coef41 = pstFltCtrl->mask[21];
	ive_filterop_c->reg_9.reg_filterop_h_coef42 = pstFltCtrl->mask[22];
	ive_filterop_c->reg_9.reg_filterop_h_coef43 = pstFltCtrl->mask[23];
	ive_filterop_c->reg_10.reg_filterop_h_coef44 = pstFltCtrl->mask[24];
	ive_filterop_c->reg_10.reg_filterop_h_norm = pstFltCtrl->norm;
	writel(ive_filterop_c->reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c->reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c->reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c->reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c->reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c->reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c->reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));
	switch (pstSrc->type) {
	case IVE_IMAGE_TYPE_U8C1:
		//test1:rgb:set gmm_gmm2_yonly:0
		//y_only:set gmm_gmm2_yonly:1
		ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly = 1;
		writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		// pass, filter case
		// TODO: check 1 channels setting
		ive_filterop_c->reg_h10.reg_filterop_mode = 3;
		ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 1;
		ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 1;
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
	case IVE_IMAGE_TYPE_YUV422SP:
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		//test1:rgb:set gmm_gmm2_yonly:0
		//y_only:set gmm_gmm2_yonly:1
		ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly = 0;
		writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

		ive_filterop_c->reg_h10.reg_filterop_mode = 1;
		ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 0;
		ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 0;
		ive_filterop_c->reg_h1c8.reg_filterop_op2_csc_enable = 0;
		ive_filterop_c->reg_h1c8.reg_filterop_op2_csc_enmode = 0;
		ive_filterop_c->reg_h14.reg_filterop_3ch_en = 1;
		writel(ive_top_c->reg_r2y4_14.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));
		writel(ive_filterop_c->reg_h1c8.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));
		break;
	default:
		pr_err("Invalid Image type %d\n", pstSrc->type);
		return FAILURE;
	}
	writel(ive_filterop_c->reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	set_isp_rdma(MOD_FILTER3CH, NULL, dev_id);
	if (isEmit) {

		if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != SUCCESS) {
			return FAILURE;
		}
		// trigger filterop
		//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->reg_20.reg_frame2op_op_mode = 5;
		// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
		ive_top_c->reg_h10.reg_filterop_top_enable = 1;
		writel(ive_top_c->reg_20.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
		writel(ive_top_c->reg_h80.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		writel(ive_top_c->reg_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		if (pstDst->type == IVE_IMAGE_TYPE_U8C1) {
			set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);
			ive_filterop_c->odma_reg_00.reg_dma_en = 0;
			writel(ive_filterop_c->odma_reg_00.val,
				   (IVE_BLK_BA[dev_id].FILTEROP +
				IVE_FILTEROP_ODMA_REG_00));

			ive_filterop_c->reg_h14.reg_op_y_wdma_en = 1;
			ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
			writel(ive_filterop_c->reg_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

			if (pstSrc->width > 480) {
				ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
					&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
					NULL, pstDst, NULL, true, 1, false, 1, false, MOD_FILTER3CH,
					instant, dev_id);
			} else {
				ret = ive_go(
					ndev, ive_top_c, instant,
					IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
					MOD_FILTER3CH, dev_id);
			}
		} else {
			img_in_c->reg_00.reg_auto_csc_en = 0;
			writel(img_in_c->reg_00.val,
				   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

			set_odma(pstDst, ive_filterop_c, pstDst->width,
				pstDst->height, dev_id);

			// NOTICE: test img_in = odma
			ive_filterop_c->reg_h14.reg_op_y_wdma_en = 0;
			writel(ive_filterop_c->reg_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

			if (pstSrc->width > 480) {
				ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
					NULL, NULL, NULL, NULL, pstSrc, NULL,
					NULL, pstDst, NULL, false, 1, false, 1, true, MOD_FILTER3CH,
					instant, dev_id);
			} else {
				ret = ive_go(
					ndev, ive_top_c, instant,
					IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
					MOD_FILTER3CH, dev_id);
			}
		}
	}
	return ret;
}

s32 ive_filter(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst, ive_filter_ctrl_s *pstFltCtrl,
			   bool instant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Filter\n");
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ret = _ive_filter(ndev, pstSrc, pstDst, pstFltCtrl, instant,
				ive_top_c, &img_in_c, &ive_filterop_c, true, dev_id);

	kfree(ive_top_c);
	return ret;
}

s32 _ive_csc(struct ive_device *ndev, ive_src_image_s *pstSrc,
			 ive_dst_image_s *pstDst, ive_csc_ctrl_s *pstCscCtrl,
			 bool instant, ive_top_c *ive_top_c,
			 img_in_c *img_in_c, ive_filterop_c *ive_filterop_c,
			 bool isEmit, s32 dev_id)
{
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	ive_gmm_c ive_gmm_c = _DEFINE_IVE_GMM_C;
	s32 ret = SUCCESS;
	s32 *tbl = NULL;
	s32 coef_BT601_to_GBR_16_235[12] = {
		1024, 0,      1404, 179188, 1024, 344,
		715,  136040, 1024, 1774,   0,	  226505
	}; // 0
	s32 coef_BT709_to_GBR_16_235[12] = {
		1024, 0,     1577, 201339, 1024, 187,
		470,  84660, 1024, 1860,   0,	 237515
	}; // 1
	s32 coef_BT601_to_GBR_0_255[12] = {
		1192, 0,      1634, 227750, 1192, 400,
		833,  139252, 1192, 2066,   0,	  283062
	}; // 2
	s32 coef_BT709_to_GBR_0_255[12] = {
		1192, 0,     1836, 253571, 1192, 218,
		547,  79352, 1192, 2166,   0,	 295776
	}; // 3
	s32 coef_RGB_to_BT601_0_255[12] = {
		306, 601,    117, 512, 176, 347,
		523, 131584, 523, 438, 85,  131584
	}; // 8
	s32 coef_RGB_to_BT709_0_255[12] = {
		218, 732,    74,  512, 120, 403,
		523, 131584, 523, 475, 48,  131584
	}; // 9
	s32 coef_RGB_to_BT601_16_235[12] = {
		263, 516,    100, 16896, 152, 298,
		450, 131584, 450, 377,	 73,  131584
	}; // 10
	s32 coef_RGB_to_BT709_16_235[12] = {
		187, 629,    63,  16896, 103, 346,
		450, 131584, 450, 409,	 41,  131584
	}; // 11
	//s32 coef_AllZero[12] = {
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	//};

	if (isEmit)
		ive_reset(ndev, ive_top_c, dev_id);
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "CSC", dev_id);

	ive_filterop_c->reg_h10.reg_filterop_mode = 1;
	ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c->reg_h1c8.reg_filterop_op2_csc_enable = 1;
	ive_filterop_c->reg_h1c8.reg_filterop_op2_csc_enmode =
		pstCscCtrl->mode;
	ive_filterop_c->reg_h14.reg_filterop_3ch_en = 0;
	writel(ive_filterop_c->reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->reg_h1c8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));

	switch (pstCscCtrl->mode) {
	case IVE_CSC_MODE_VIDEO_BT601_YUV2RGB:
		tbl = coef_BT601_to_GBR_16_235;
		break;
	case IVE_CSC_MODE_VIDEO_BT709_YUV2RGB:
		tbl = coef_BT709_to_GBR_16_235;
		break;
	case IVE_CSC_MODE_PIC_BT601_YUV2RGB:
	case IVE_CSC_MODE_PIC_BT601_YUV2HSV:
	case IVE_CSC_MODE_PIC_BT601_YUV2LAB:
		tbl = coef_BT601_to_GBR_0_255;
		break;
	case IVE_CSC_MODE_PIC_BT709_YUV2RGB:
	case IVE_CSC_MODE_PIC_BT709_YUV2HSV:
	case IVE_CSC_MODE_PIC_BT709_YUV2LAB:
		tbl = coef_BT709_to_GBR_0_255;
		break;
	case IVE_CSC_MODE_VIDEO_BT601_RGB2YUV:
		tbl = coef_RGB_to_BT601_0_255;
		break;
	case IVE_CSC_MODE_VIDEO_BT709_RGB2YUV:
		tbl = coef_RGB_to_BT709_0_255;
		break;
	case IVE_CSC_MODE_PIC_BT601_RGB2YUV:
		tbl = coef_RGB_to_BT601_16_235;
		break;
	case IVE_CSC_MODE_PIC_BT709_RGB2YUV:
		tbl = coef_RGB_to_BT709_16_235;
		break;
	default:
		break;
	}

	if (tbl) {
		//rgb:gmm_gmm2_yonly = 0,yonly:gmm_gmm2_yonly:1
		ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly = 0;
		writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));


		ive_filterop_c->reg_h194
			.reg_filterop_op2_csc_coeff_sw_update = 0;
		ive_filterop_c->reg_csc_coeff_0
			.reg_filterop_op2_csc_coeff_sw_00 = tbl[0];
		ive_filterop_c->reg_csc_coeff_1
			.reg_filterop_op2_csc_coeff_sw_01 = tbl[1];
		ive_filterop_c->reg_csc_coeff_2
			.reg_filterop_op2_csc_coeff_sw_02 = tbl[2];
		ive_filterop_c->reg_csc_coeff_3
			.reg_filterop_op2_csc_coeff_sw_03 = tbl[3];
		ive_filterop_c->reg_csc_coeff_4
			.reg_filterop_op2_csc_coeff_sw_04 = tbl[4];
		ive_filterop_c->reg_csc_coeff_5
			.reg_filterop_op2_csc_coeff_sw_05 = tbl[5];
		ive_filterop_c->reg_csc_coeff_6
			.reg_filterop_op2_csc_coeff_sw_06 = tbl[6];
		ive_filterop_c->reg_csc_coeff_7
			.reg_filterop_op2_csc_coeff_sw_07 = tbl[7];
		ive_filterop_c->reg_csc_coeff_8
			.reg_filterop_op2_csc_coeff_sw_08 = tbl[8];
		ive_filterop_c->reg_csc_coeff_9
			.reg_filterop_op2_csc_coeff_sw_09 = tbl[9];
		ive_filterop_c->reg_csc_coeff_a
			.reg_filterop_op2_csc_coeff_sw_10 = tbl[10];
		ive_filterop_c->reg_csc_coeff_b
			.reg_filterop_op2_csc_coeff_sw_11 = tbl[11];
		writel(ive_filterop_c->reg_h194.val,
				(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H194));
		writel(ive_filterop_c->reg_csc_coeff_0.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_0));
		writel(ive_filterop_c->reg_csc_coeff_1.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_1));
		writel(ive_filterop_c->reg_csc_coeff_2.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_2));
		writel(ive_filterop_c->reg_csc_coeff_3.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_3));
		writel(ive_filterop_c->reg_csc_coeff_4.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_4));
		writel(ive_filterop_c->reg_csc_coeff_5.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_5));
		writel(ive_filterop_c->reg_csc_coeff_6.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_6));
		writel(ive_filterop_c->reg_csc_coeff_7.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_7));
		writel(ive_filterop_c->reg_csc_coeff_8.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_8));
		writel(ive_filterop_c->reg_csc_coeff_9.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_9));
		writel(ive_filterop_c->reg_csc_coeff_a.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_A));
		writel(ive_filterop_c->reg_csc_coeff_b.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_B));
	}

	if (isEmit) {
		img_in_c->reg_068.reg_ip_clr_w1t = 1;
		writel(img_in_c->reg_068.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
		//udelay(3);
		img_in_c->reg_068.reg_ip_clr_w1t = 0;
		writel(img_in_c->reg_068.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));

		set_isp_rdma(MOD_CSC, NULL, dev_id);
		if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != SUCCESS) {
			return FAILURE;
		}
		img_in_c->reg_00.reg_auto_csc_en = 0;
		writel(img_in_c->reg_00.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

		ive_top_c->reg_h10.reg_filterop_top_enable = 1;
		writel(ive_top_c->reg_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		set_odma(pstDst, ive_filterop_c, pstDst->width,
			pstDst->height, dev_id);

		ive_filterop_c->reg_h14.reg_op_y_wdma_en = 0;
		writel(ive_filterop_c->reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		if (pstSrc->width > 480) {
			ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
				NULL, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, false, 1, false, 1, true, MOD_CSC,
				instant, dev_id);
		} else {
			ret = ive_go(
				ndev, ive_top_c, instant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
				MOD_CSC, dev_id);
		}
	}
	return ret;
}

s32 ive_csc(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstDst, ive_csc_ctrl_s *pstCscCtrl,
			bool instant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_CSC\n");
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ret = _ive_csc(ndev, pstSrc, pstDst, pstCscCtrl, instant,
				ive_top_c, &img_in_c, &ive_filterop_c, true, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 ive_filter_and_csc(struct ive_device *ndev,
				 ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
				 ive_filter_and_csc_ctrl_s *pstFltCscCtrl,
				 bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	ive_csc_ctrl_s stCscCtrl;
	ive_filter_ctrl_s stFltCtrl;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_FilterAndCSC\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	if (pstSrc->type == IVE_IMAGE_TYPE_YUV420SP ||
		pstSrc->type == IVE_IMAGE_TYPE_YUV422SP) {
	} else {
		pr_err("only support input fmt YUV420SP/YUV422SP??\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	// set filter
	memcpy(stFltCtrl.mask, pstFltCscCtrl->mask, sizeof(s8) * 25);
	stFltCtrl.norm = pstFltCscCtrl->norm;
	_ive_filter(ndev, pstSrc, pstDst, &stFltCtrl, instant, ive_top_c,
			&img_in_c, &ive_filterop_c, /*isEmit=*/false, dev_id);
	// set csc
	stCscCtrl.mode = pstFltCscCtrl->mode;
	_ive_csc(ndev, pstSrc, pstDst, &stCscCtrl, instant, ive_top_c,
			 &img_in_c, &ive_filterop_c, /*isEmit=*/false, dev_id);
	strcpy(g_debug_info.op_name, "FilterAndCSC");
	ive_filterop_c.reg_h14.reg_filterop_3ch_en = 1;
	ive_filterop_c.reg_h1c8.reg_filterop_op2_csc_enable = 1;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.reg_h1c8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));

	set_isp_rdma(MOD_FILTERCSC, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	img_in_c.reg_00.reg_auto_csc_en = 0;
	writel(img_in_c.reg_00.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

	// NOTICE: need to first set it
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_odma(pstDst, &ive_filterop_c, pstDst->width, pstDst->height, dev_id);

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
			NULL, NULL, NULL, NULL, pstSrc, NULL,
			NULL, pstDst, NULL, false, 1, false, 1, true, MOD_FILTERCSC,
			instant, dev_id);
	} else {
		ret = ive_go(
			ndev, ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
			MOD_FILTERCSC, dev_id);
	}
	kfree(ive_top_c);
	return ret;
}

s32 ive_hist(struct ive_device *ndev, ive_src_image_s *pstSrc,
			 ive_dst_mem_info_s *pstDst, bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_isp_dma_ctl_c(wdma_hist_ctl_c);
	isp_dma_ctl_c wdma_hist_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_HIST_C(ive_hist_c);
	ive_hist_c ive_hist_c = _DEFINE_IVE_HIST_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Hist\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_mem("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "Hist", dev_id);

	set_isp_rdma(MOD_HIST, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	ive_filterop_c.odma_reg_00.reg_dma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

	ive_hist_c.reg_0.reg_ive_hist_enable = 1;
	ive_top_c->reg_h10.reg_hist_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_hist_c.reg_0.val, (IVE_BLK_BA[dev_id].HIST + IVE_HIST_REG_0));

	wdma_hist_ctl_c.base_addr.reg_basel = pstDst->phy_addr & 0xffffffff;
	wdma_hist_ctl_c.sys_control.reg_baseh =
		((u64)pstDst->phy_addr >> 32) & 0xffffffff;
	wdma_hist_ctl_c.sys_control.reg_base_sel = 1;
	if (g_dump_dma_info == TRUE) {
		pr_info("Hist wdma_hist_ctl_c address: 0x%08x %08x\n",
			wdma_hist_ctl_c.sys_control.reg_baseh,
			wdma_hist_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[WDMA_HIST].addr_en = wdma_hist_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_HIST].addr_l = wdma_hist_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_HIST].addr_h = wdma_hist_ctl_c.sys_control.reg_baseh & 0xff;

	writel(wdma_hist_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].HIST_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_hist_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].HIST_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	ret = ive_go(ndev, ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_HIST_MASK, MOD_HIST, dev_id);

	ive_hist_c.reg_0.reg_ive_hist_enable = 0;
	ive_hist_c.reg_0.reg_force_clk_enable = 1;
	writel(ive_hist_c.reg_0.val, (IVE_BLK_BA[dev_id].HIST + IVE_HIST_REG_0));
	kfree(ive_top_c);
	return ret;
}

s32 ive_sobel(struct ive_device *ndev, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstDstH, ive_dst_image_s *pstDstV,
			  ive_sobel_ctrl_s *pstSobelCtrl, bool instant, s32 dev_id)
{
	s32 yunit = 2;
	s32 ret = SUCCESS;
	ive_dst_image_s *firstOut = pstDstH;
	ive_dst_image_s *secondOut = pstDstV;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_c_ctl_c);
	isp_dma_ctl_c wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Sobel\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDstH", pstDstH);
		dump_ive_image("pstDstV", pstDstV);
	}
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "Sobel", dev_id);

	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.reg_h10.reg_filterop_mode = 9;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.reg_4.reg_filterop_h_coef00 = pstSobelCtrl->mask[0];
	ive_filterop_c.reg_4.reg_filterop_h_coef01 = pstSobelCtrl->mask[1];
	ive_filterop_c.reg_4.reg_filterop_h_coef02 = pstSobelCtrl->mask[2];
	ive_filterop_c.reg_4.reg_filterop_h_coef03 = pstSobelCtrl->mask[3];
	ive_filterop_c.reg_5.reg_filterop_h_coef04 = pstSobelCtrl->mask[4];
	ive_filterop_c.reg_5.reg_filterop_h_coef10 = pstSobelCtrl->mask[5];
	ive_filterop_c.reg_5.reg_filterop_h_coef11 = pstSobelCtrl->mask[6];
	ive_filterop_c.reg_5.reg_filterop_h_coef12 = pstSobelCtrl->mask[7];
	ive_filterop_c.reg_6.reg_filterop_h_coef13 = pstSobelCtrl->mask[8];
	ive_filterop_c.reg_6.reg_filterop_h_coef14 = pstSobelCtrl->mask[9];
	ive_filterop_c.reg_6.reg_filterop_h_coef20 = pstSobelCtrl->mask[10];
	ive_filterop_c.reg_6.reg_filterop_h_coef21 = pstSobelCtrl->mask[11];
	ive_filterop_c.reg_7.reg_filterop_h_coef22 = pstSobelCtrl->mask[12];
	ive_filterop_c.reg_7.reg_filterop_h_coef23 = pstSobelCtrl->mask[13];
	ive_filterop_c.reg_7.reg_filterop_h_coef24 = pstSobelCtrl->mask[14];
	ive_filterop_c.reg_7.reg_filterop_h_coef30 = pstSobelCtrl->mask[15];
	ive_filterop_c.reg_8.reg_filterop_h_coef31 = pstSobelCtrl->mask[16];
	ive_filterop_c.reg_8.reg_filterop_h_coef32 = pstSobelCtrl->mask[17];
	ive_filterop_c.reg_8.reg_filterop_h_coef33 = pstSobelCtrl->mask[18];
	ive_filterop_c.reg_8.reg_filterop_h_coef34 = pstSobelCtrl->mask[19];
	ive_filterop_c.reg_9.reg_filterop_h_coef40 = pstSobelCtrl->mask[20];
	ive_filterop_c.reg_9.reg_filterop_h_coef41 = pstSobelCtrl->mask[21];
	ive_filterop_c.reg_9.reg_filterop_h_coef42 = pstSobelCtrl->mask[22];
	ive_filterop_c.reg_9.reg_filterop_h_coef43 = pstSobelCtrl->mask[23];
	ive_filterop_c.reg_10.reg_filterop_h_coef44 = pstSobelCtrl->mask[24];

	writel(ive_filterop_c.reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	if (pstSobelCtrl->out_ctrl == IVE_SOBEL_OUT_CTRL_BOTH ||
		pstSobelCtrl->out_ctrl == IVE_SOBEL_OUT_CTRL_HOR ||
		pstSobelCtrl->out_ctrl == IVE_SOBEL_OUT_CTRL_VER) {
		// valid
		// "0 : h , v  -> wdma_y wdma_c will be active 1 :h only -> wdma_y
		// 2: v only -> wdma_c 3. h , v pack => {v ,h} -> wdma_y "
		ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl =
			(int)pstSobelCtrl->out_ctrl;
		writel(ive_filterop_c.reg_110.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));
	} else {
		pr_err("[IVE] not support out_ctrl\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	set_isp_rdma(MOD_SOBEL, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));

	// "0 : U8 1 : S16 2 : U16"
	// FIXME: check enable in Sobel
	ive_filterop_c.reg_110.reg_filterop_map_enmode = 1;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_filterop_c.reg_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));

	if ((int)pstSobelCtrl->out_ctrl == IVE_SOBEL_OUT_CTRL_BOTH) {
		set_img_dst2(secondOut, &wdma_c_ctl_c, dev_id);
		set_img_dst1(firstOut, &wdma_y_ctl_c, dev_id);
		// enable it
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL,
					&wdma_c_ctl_c, NULL, pstSrc, NULL, NULL,
					firstOut, secondOut, true, yunit, true, 2,
					false, 0, instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
		//ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
	} else if ((int)pstSobelCtrl->out_ctrl == IVE_SOBEL_OUT_CTRL_VER) {
		set_img_dst1(NULL, NULL, dev_id);
		set_img_dst2(secondOut, &wdma_c_ctl_c, dev_id);
		// enable it
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, NULL, NULL, &wdma_c_ctl_c,
					NULL, pstSrc, NULL, NULL, firstOut,
					secondOut, false, yunit, true, 2, false, 0,
					instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	} else {
		set_img_dst1(firstOut, &wdma_y_ctl_c, dev_id);
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, firstOut,
					secondOut, true, yunit, false, 1, false, MOD_SOBEL,
					instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}

	if ((int)pstSobelCtrl->out_ctrl == 0) {
		ret = ive_go(
			ndev, ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK |
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
			MOD_SOBEL, dev_id);
	} else if ((int)pstSobelCtrl->out_ctrl == 1) {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_SOBEL, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
				 MOD_SOBEL, dev_id);
	}

	kfree(ive_top_c);
	return ret;
}

s32 ive_mag_and_ang(struct ive_device *ndev, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstDstMag,
			  ive_dst_image_s *pstDstAng,
			  ive_mag_and_ang_ctrl_s *pstMagAndAngCtrl,
			  bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_c_ctl_c);
	isp_dma_ctl_c wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_MagAndAng\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDstMag", pstDstMag);
		dump_ive_image("pstDstAng", pstDstAng);
	}
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "MagAndAng", dev_id);

	ive_filterop_c.reg_h10.reg_filterop_mode = 7;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.reg_4.reg_filterop_h_coef00 =
		pstMagAndAngCtrl->mask[0];
	ive_filterop_c.reg_4.reg_filterop_h_coef01 =
		pstMagAndAngCtrl->mask[1];
	ive_filterop_c.reg_4.reg_filterop_h_coef02 =
		pstMagAndAngCtrl->mask[2];
	ive_filterop_c.reg_4.reg_filterop_h_coef03 =
		pstMagAndAngCtrl->mask[3];
	ive_filterop_c.reg_5.reg_filterop_h_coef04 =
		pstMagAndAngCtrl->mask[4];
	ive_filterop_c.reg_5.reg_filterop_h_coef10 =
		pstMagAndAngCtrl->mask[5];
	ive_filterop_c.reg_5.reg_filterop_h_coef11 =
		pstMagAndAngCtrl->mask[6];
	ive_filterop_c.reg_5.reg_filterop_h_coef12 =
		pstMagAndAngCtrl->mask[7];
	ive_filterop_c.reg_6.reg_filterop_h_coef13 =
		pstMagAndAngCtrl->mask[8];
	ive_filterop_c.reg_6.reg_filterop_h_coef14 =
		pstMagAndAngCtrl->mask[9];
	ive_filterop_c.reg_6.reg_filterop_h_coef20 =
		pstMagAndAngCtrl->mask[10];
	ive_filterop_c.reg_6.reg_filterop_h_coef21 =
		pstMagAndAngCtrl->mask[11];
	ive_filterop_c.reg_7.reg_filterop_h_coef22 =
		pstMagAndAngCtrl->mask[12];
	ive_filterop_c.reg_7.reg_filterop_h_coef23 =
		pstMagAndAngCtrl->mask[13];
	ive_filterop_c.reg_7.reg_filterop_h_coef24 =
		pstMagAndAngCtrl->mask[14];
	ive_filterop_c.reg_7.reg_filterop_h_coef30 =
		pstMagAndAngCtrl->mask[15];
	ive_filterop_c.reg_8.reg_filterop_h_coef31 =
		pstMagAndAngCtrl->mask[16];
	ive_filterop_c.reg_8.reg_filterop_h_coef32 =
		pstMagAndAngCtrl->mask[17];
	ive_filterop_c.reg_8.reg_filterop_h_coef33 =
		pstMagAndAngCtrl->mask[18];
	ive_filterop_c.reg_8.reg_filterop_h_coef34 =
		pstMagAndAngCtrl->mask[19];
	ive_filterop_c.reg_9.reg_filterop_h_coef40 =
		pstMagAndAngCtrl->mask[20];
	ive_filterop_c.reg_9.reg_filterop_h_coef41 =
		pstMagAndAngCtrl->mask[21];
	ive_filterop_c.reg_9.reg_filterop_h_coef42 =
		pstMagAndAngCtrl->mask[22];
	ive_filterop_c.reg_9.reg_filterop_h_coef43 =
		pstMagAndAngCtrl->mask[23];
	ive_filterop_c.reg_10.reg_filterop_h_coef44 =
		pstMagAndAngCtrl->mask[24];

	ive_filterop_c.reg_11.reg_filterop_v_coef00 =
		-1 * pstMagAndAngCtrl->mask[0];
	ive_filterop_c.reg_11.reg_filterop_v_coef01 =
		-1 * pstMagAndAngCtrl->mask[5];
	ive_filterop_c.reg_11.reg_filterop_v_coef02 =
		-1 * pstMagAndAngCtrl->mask[10];
	ive_filterop_c.reg_11.reg_filterop_v_coef03 =
		-1 * pstMagAndAngCtrl->mask[15];
	ive_filterop_c.reg_12.reg_filterop_v_coef04 =
		-1 * pstMagAndAngCtrl->mask[20];
	ive_filterop_c.reg_12.reg_filterop_v_coef10 =
		-1 * pstMagAndAngCtrl->mask[1];
	ive_filterop_c.reg_12.reg_filterop_v_coef11 =
		-1 * pstMagAndAngCtrl->mask[6];
	ive_filterop_c.reg_12.reg_filterop_v_coef12 =
		-1 * pstMagAndAngCtrl->mask[11];
	ive_filterop_c.reg_13.reg_filterop_v_coef13 =
		-1 * pstMagAndAngCtrl->mask[16];
	ive_filterop_c.reg_13.reg_filterop_v_coef14 =
		-1 * pstMagAndAngCtrl->mask[21];
	ive_filterop_c.reg_13.reg_filterop_v_coef20 =
		-1 * pstMagAndAngCtrl->mask[2];
	ive_filterop_c.reg_13.reg_filterop_v_coef21 =
		-1 * pstMagAndAngCtrl->mask[7];
	ive_filterop_c.reg_14.reg_filterop_v_coef22 =
		-1 * pstMagAndAngCtrl->mask[12];
	ive_filterop_c.reg_14.reg_filterop_v_coef23 =
		-1 * pstMagAndAngCtrl->mask[17];
	ive_filterop_c.reg_14.reg_filterop_v_coef24 =
		-1 * pstMagAndAngCtrl->mask[22];
	ive_filterop_c.reg_14.reg_filterop_v_coef30 =
		-1 * pstMagAndAngCtrl->mask[3];
	ive_filterop_c.reg_15.reg_filterop_v_coef31 =
		-1 * pstMagAndAngCtrl->mask[8];
	ive_filterop_c.reg_15.reg_filterop_v_coef32 =
		-1 * pstMagAndAngCtrl->mask[13];
	ive_filterop_c.reg_15.reg_filterop_v_coef33 =
		-1 * pstMagAndAngCtrl->mask[18];
	ive_filterop_c.reg_15.reg_filterop_v_coef34 =
		-1 * pstMagAndAngCtrl->mask[23];
	ive_filterop_c.reg_16.reg_filterop_v_coef40 =
		-1 * pstMagAndAngCtrl->mask[4];
	ive_filterop_c.reg_16.reg_filterop_v_coef41 =
		-1 * pstMagAndAngCtrl->mask[9];
	ive_filterop_c.reg_16.reg_filterop_v_coef42 =
		-1 * pstMagAndAngCtrl->mask[14];
	ive_filterop_c.reg_16.reg_filterop_v_coef43 =
		-1 * pstMagAndAngCtrl->mask[19];
	ive_filterop_c.reg_17.reg_filterop_v_coef44 =
		-1 * pstMagAndAngCtrl->mask[24];

	writel(ive_filterop_c.reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	writel(ive_filterop_c.reg_11.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_11));
	writel(ive_filterop_c.reg_12.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_12));
	writel(ive_filterop_c.reg_13.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_13));
	writel(ive_filterop_c.reg_14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_14));
	writel(ive_filterop_c.reg_15.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_15));
	writel(ive_filterop_c.reg_16.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_16));
	writel(ive_filterop_c.reg_17.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_17));

	ive_filterop_c.reg_18.reg_filterop_mode_trans = 0;
	ive_filterop_c.reg_18.reg_filterop_mag_thr = pstMagAndAngCtrl->thr;
	writel(ive_filterop_c.reg_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));

	set_isp_rdma(MOD_MAG, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));

	// "0 : U8 1 : S16 2 : U16"
	ive_filterop_c.reg_110.reg_filterop_map_enmode = 1;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	if (pstMagAndAngCtrl->out_ctrl ==
		IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG) {
		//setDMA(pstSrc, pstDstAng, NULL, pstDstMag);
		ive_filterop_c.reg_110.reg_filterop_magang_out_ctrl = 1;
		set_img_dst2(pstDstMag, &wdma_c_ctl_c, dev_id);
		set_img_dst1(pstDstAng, &wdma_y_ctl_c, dev_id);
		// enable it
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.reg_110.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL,
					&wdma_c_ctl_c, NULL, pstSrc, NULL, NULL,
					pstDstAng, pstDstMag, true, 1, true, 2,
					false, MOD_MAG, instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	} else {
		//setDMA(pstSrc, NULL, NULL, pstDstMag);
		ive_filterop_c.reg_110.reg_filterop_magang_out_ctrl = 0;
		set_img_dst2(pstDstMag, &wdma_c_ctl_c, dev_id);
		//set_img_dst1(pstDstAng, NULL);
		// enable it
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.reg_110.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, NULL, NULL, &wdma_c_ctl_c,
					NULL, pstSrc, NULL, NULL, NULL,
					pstDstMag, false, 1, true, 2, false, MOD_MAG,
					instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}

	if (pstMagAndAngCtrl->out_ctrl ==
		IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG) {
		ret = ive_go(
			ndev, ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK |
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
			MOD_MAG, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
				 MOD_MAG, dev_id);
	}

	ive_filterop_c.reg_18.reg_filterop_mode_trans = 1;
	writel(ive_filterop_c.reg_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));
	kfree(ive_top_c);
	return ret;
}

s32 ive_map(struct ive_device *ndev, ive_src_image_s *pstSrc,
			u16 *pstMap, ive_dst_image_s *pstDst,
			ive_map_ctrl_s *pstMapCtrl, bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	s32 l = 0;
	u16 u16Word;
	u16 *_pu16Ptr = NULL;
	u8 *pu8Ptr = NULL;
	u16 *pu16Ptr = NULL;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_MAP_C(ive_map_c);
	ive_map_c ive_map_c = _DEFINE_IVE_MAP_C;
	//DEFINE_isp_dma_ctl_c(wdma_c_ctl_c);
	isp_dma_ctl_c wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	//IVE_BLK_BA_MAP
	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Map\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		// dump_ive_mem("pstMap", pstMap);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "Map", dev_id);

	ive_filterop_c.reg_h10.reg_filterop_mode = 11;
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.reg_110.reg_filterop_map_enmode = pstMapCtrl->mode;

	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.reg_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));


	pu8Ptr = (u8 *)pstMap;
	pu16Ptr = (u16 *)pstMap;

	ive_map_c.reg_1.reg_lut_prog_en = 1;
	ive_map_c.reg_2.reg_lut_st_addr = 0;
	ive_map_c.reg_2.reg_lut_st_w1t = 1;
	ive_map_c.reg_1.reg_lut_wsel =
		(pstMapCtrl->mode == IVE_MAP_MODE_U8) ? 0 : 1;
	writel(ive_map_c.reg_1.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_1));

	writel(ive_map_c.reg_2.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_2));
	for (l = 0; l < 256; l++) {
		if (pstMapCtrl->mode == IVE_MAP_MODE_U8) {
			u16Word = (0x00ff & *pu8Ptr);
			pu8Ptr++;
		} else {
			// u16Word = *pu16Ptr;
			if (l >= 0xB0 && l <= 0xBF)
				u16Word = 0x470 + (l - 0xB0);
			else
				u16Word = (0x00ff & l);
			pu16Ptr++;
		}
		ive_map_c.reg_3.reg_lut_wdata = u16Word;
		ive_map_c.reg_3.reg_lut_w1t = 1;

		writel(ive_map_c.reg_3.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_3));
	}
	ive_map_c.reg_1.reg_lut_rsel =
		(pstMapCtrl->mode == IVE_MAP_MODE_U8) ? 0 : 1;
	ive_map_c.reg_1.reg_lut_prog_en = 0;

	writel(ive_map_c.reg_1.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_1));

	set_isp_rdma(MOD_MAP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		vfree(_pu16Ptr);
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;

	ive_top_c->reg_h10.reg_map_top_enable = 1;
	ive_map_c.reg_0.reg_ip_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_map_c.reg_0.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_0));

	set_img_dst2(pstDst, &wdma_c_ctl_c, dev_id);

	// enable it
	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
			NULL, NULL, &wdma_c_ctl_c, NULL, pstSrc, NULL,
			NULL, pstDst, NULL, false, 1, true, 1, false, MOD_MAP,
			instant, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK, MOD_MAP, dev_id);
	}

	ive_map_c.reg_0.reg_ip_enable = 0;
	writel(ive_map_c.reg_0.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_0));

	kfree(ive_top_c);
	return ret;
}

s32 ive_ncc(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_ncc_dst_mem_s *pstDst,
			bool instant, s32 dev_id)

{
	s32 mode = 0;
	s32 ret = SUCCESS;
	ive_ncc_dst_mem_s ncc;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	//IVE_BLK_BA_MAP
	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_NCC\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		// dump_ive_mem("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc1->width, pstSrc1->height, "NCC", dev_id);

	set_isp_rdma(MOD_NCC, NULL, dev_id);
	if (set_img_src1(pstSrc1, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}
	set_img_src2(pstSrc2, NULL, dev_id);

	mode = ive_get_mod_u8(pstSrc1->type);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		kfree(ive_top_c);
		return FAILURE;
	}
	ive_top_c->reg_3.reg_ive_rdma_img1_mod_u8 = mode;

	ive_top_c->reg_3.reg_imgmux_img0_sel = 0;
	ive_top_c->reg_3.reg_ive_rdma_img1_en = 1;
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 5;
	ive_top_c->reg_14.reg_csc_enable = 0;
	ive_top_c->reg_h10.reg_filterop_top_enable = 0;
	ive_top_c->reg_h10.reg_csc_top_enable = 0;
	ive_top_c->reg_h10.reg_ncc_top_enable = 1;
	ive_top_c->reg_h10.reg_rdma_img1_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_14.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_14));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	ive_filterop_c.odma_reg_00.reg_dma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

	ndev->cur_optype = MOD_NCC;
	// g_pDst = pstDst;

	ret = ive_go(ndev, ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_NCC_MASK, MOD_NCC, dev_id);

	if (ret == SUCCESS) {
		memset((void *)&ncc, 0xCD, sizeof(ive_ncc_dst_mem_s));
		ncc.numerator = (u64)(readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_01))
					<< 32 |
				readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_00);

		ncc.quad_sum1 = (u64)(readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_03))
					<< 32 |
				readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_02);

		ncc.quad_sum2 = (u64)(readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_05))
					<< 32 |
				readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_04);

		memcpy(pstDst, &ncc, sizeof(ive_ncc_dst_mem_s));

	}
	kfree(ive_top_c);
	return ret;
}

s32 ive_integ(struct ive_device *ndev, ive_src_image_s *pstSrc,
			  ive_dst_mem_info_s *pstDst,
			  ive_integ_ctrl_s *pstIntegCtrl, bool instant, s32 dev_id)
{
	s32 ret = 0;
	ive_dst_image_s _pstDst;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_INTG_C(ive_intg_c);
	ive_intg_c ive_intg_c = _DEFINE_IVE_INTG_C;
	//DEFINE_isp_dma_ctl_c(wdma_integ_ctl_c);
	isp_dma_ctl_c wdma_integ_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	//IVE_BLK_BA_MAP
	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Integ\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_mem("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "Integ", dev_id);

	ive_intg_c.reg_0.reg_ive_intg_ctrl = (int)pstIntegCtrl->out_ctrl;
	ive_intg_c.reg_1.reg_ive_intg_stride = (u16)pstSrc->stride[0];
	writel(ive_intg_c.reg_0.val, (IVE_BLK_BA[dev_id].INTG + IVE_INTG_REG_0));
	writel(ive_intg_c.reg_1.val, (IVE_BLK_BA[dev_id].INTG + IVE_INTG_REG_1));

	set_isp_rdma(MOD_INTEG, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	ive_filterop_c.odma_reg_00.reg_dma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.odma_reg_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

	_pstDst.phy_addr[0] = pstDst->phy_addr;

	wdma_integ_ctl_c.base_addr.reg_basel =
		_pstDst.phy_addr[0] & 0xffffffff;
	wdma_integ_ctl_c.sys_control.reg_baseh =
		(_pstDst.phy_addr[0] >> 32) & 0xffffffff;
	wdma_integ_ctl_c.sys_control.reg_base_sel = 1;
	if (g_dump_dma_info == TRUE) {
		pr_info("Integ wdma_integ_ctl_c address: 0x%08x %08x\n",
			wdma_integ_ctl_c.sys_control.reg_baseh,
			wdma_integ_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[WDMA_INTEG].addr_en = wdma_integ_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_INTEG].addr_l = wdma_integ_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_INTEG].addr_h = wdma_integ_ctl_c.sys_control.reg_baseh & 0xff;

	writel(wdma_integ_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].INTG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_integ_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].INTG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	ive_intg_c.reg_0.reg_ive_intg_enable = 1;
	writel(ive_intg_c.reg_0.val, (IVE_BLK_BA[dev_id].INTG + IVE_INTG_REG_0));

	// trigger
	ive_top_c->reg_h10.reg_intg_top_enable = 1;
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 0;
	ive_top_c->reg_14.reg_csc_enable = 0;
	ive_top_c->reg_h10.reg_csc_top_enable = 0;

	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_14.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_14));

	ret = ive_go(ndev, ive_top_c, instant,
			  IVE_TOP_REG_FRAME_DONE_INTG_MASK, MOD_INTEG, dev_id);

	kfree(ive_top_c);
	return ret;
}

s32 ive_lbp(struct ive_device *ndev, ive_src_image_s *pstSrc,
			ive_dst_image_s *pstDst, ive_lbp_ctrl_s *pstLbpCtrl,
			bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	//IVE_BLK_BA_MAP
	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_LBP\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "LBP", dev_id);

	// setting
	ive_filterop_c.reg_20.reg_filterop_lbp_u8bit_thr =
		pstLbpCtrl->thr.u8_val;
	ive_filterop_c.reg_20.reg_filterop_lbp_s8bit_thr =
		pstLbpCtrl->thr.s8_val;
	ive_filterop_c.reg_20.reg_filterop_lbp_enmode = pstLbpCtrl->mode;
	writel(ive_filterop_c.reg_20.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_20));

	// filterop
	ive_filterop_c.reg_h10.reg_filterop_mode = 2;
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 6;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	set_isp_rdma(MOD_LBP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_LBP,
				instant, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_LBP, dev_id);
	}
	kfree(ive_top_c);
	return ret;
}

s32 _ive_16bit_to_8bit(ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
				 ive_top_c *ive_top_c, ive_filterop_c *ive_filterop_c,
				 isp_dma_ctl_c *wdma_y_ctl_c, isp_dma_ctl_c *rdma_eigval_ctl_c, s32 dev_id)
{
	ive_top_c->reg_h10.reg_rdma_eigval_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_isp_rdma(MOD_16To8, NULL, dev_id);
	set_rdma_eigval(pstSrc, rdma_eigval_ctl_c, dev_id);

	ive_top_c->reg_3.reg_mapmux_rdma_sel = 1;
	ive_top_c->reg_3.reg_ive_rdma_eigval_en = 1; //Here?
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;

	// bypass filterop...
	ive_filterop_c->reg_h10.reg_filterop_mode = 2;
	ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 0; //sw_ovw; bypass op1
	ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 1;
	ive_filterop_c->reg_28.reg_filterop_op2_erodila_en = 0; //bypass op2
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_filterop_c->reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	set_img_dst1(pstDst, wdma_y_ctl_c, dev_id);

	ive_filterop_c->reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	return SUCCESS;
}

s32 ive_16bit_to_8bit(struct ive_device *ndev,
				ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
				ive_16bit_to_8bit_ctrl_s *pst16BitTo8BitCtrl,
				bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	u32 tmp;
	u16 u8Num_div_u16Den;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_isp_dma_ctl_c(rdma_eigval_ctl_c);
	isp_dma_ctl_c rdma_eigval_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_16BitTo8Bit\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	tmp = (((u32)pst16BitTo8BitCtrl->numerator << 16) +
		   (pst16BitTo8BitCtrl->denominator >> 1)) /
		  pst16BitTo8BitCtrl->denominator;
	u8Num_div_u16Den = (u16)(tmp & 0xffff);
	ive_top_c->reg_h130.reg_thresh_top_mod = 1;
	ive_top_c->reg_h130.reg_thresh_thresh_en = 0;
	ive_top_c->reg_h10.reg_thresh_top_enable = 1;
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "16BitTo8Bit", dev_id);

	ive_top_c->reg_h134.reg_thresh_16to8_mod = pst16BitTo8BitCtrl->mode;
	ive_top_c->reg_h134.reg_thresh_16to8_u8Num_div_u16Den =
		u8Num_div_u16Den; //0xffff
	ive_top_c->reg_h134.reg_thresh_16to8_s8bias = pst16BitTo8BitCtrl->s8Bias;
	writel(ive_top_c->reg_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c->reg_h134.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H134));
	_ive_16bit_to_8bit(pstSrc, pstDst, ive_top_c, &ive_filterop_c,
		 &wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, NULL,
				&wdma_y_ctl_c, NULL, NULL, &rdma_eigval_ctl_c, pstSrc,
				NULL, NULL, pstDst, NULL, true, 1, false, 1, false, MOD_16To8,
				instant, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_16To8, dev_id);
	}

	ive_top_c->reg_3.reg_mapmux_rdma_sel = 0;
	ive_top_c->reg_3.reg_ive_rdma_eigval_en = 0;
	ive_top_c->reg_h10.reg_rdma_eigval_top_enable = 0;
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	kfree(ive_top_c);
	return ret;
}

s32 ive_thresh_s16(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst,
			   ive_thresh_s16_ctrl_s *pstThrS16Ctrl,
			   bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_isp_dma_ctl_c(rdma_eigval_ctl_c);
	isp_dma_ctl_c rdma_eigval_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Thresh_S16\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	// top
	ive_top_c.reg_h130.reg_thresh_top_mod = 2;
	ive_top_c.reg_h130.reg_thresh_thresh_en = 0;
	ive_top_c.reg_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c.reg_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c.reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ive_set_wh(&ive_top_c, pstDst->width, pstDst->height, "Thresh_S16", dev_id);
	// setting
	ive_top_c.reg_h13c.reg_thresh_s16_enmode = pstThrS16Ctrl->mode;
	ive_top_c.reg_h13c.reg_thresh_s16_u8bit_min =
		pstThrS16Ctrl->min_val.u8_val;
	ive_top_c.reg_h13c.reg_thresh_s16_u8bit_mid =
		pstThrS16Ctrl->mid_val.u8_val;
	ive_top_c.reg_h13c.reg_thresh_s16_u8bit_max =
		pstThrS16Ctrl->max_val.u8_val;
	ive_top_c.reg_h140.reg_thresh_s16_bit_thr_l = pstThrS16Ctrl->low_thr;
	ive_top_c.reg_h140.reg_thresh_s16_bit_thr_h = pstThrS16Ctrl->high_thr;
	writel(ive_top_c.reg_h13c.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H13C));
	writel(ive_top_c.reg_h140.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H140));

	_ive_16bit_to_8bit(pstSrc, pstDst, &ive_top_c, &ive_filterop_c,
		 &wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, &ive_top_c, &ive_filterop_c, NULL,
				&wdma_y_ctl_c, NULL, NULL, &rdma_eigval_ctl_c, pstSrc,
				NULL, NULL, pstDst, NULL, true, 1, false, 1, false, MOD_THRS16,
				instant, dev_id);
	} else {
		ret = ive_go(ndev, &ive_top_c, instant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_THRS16, dev_id);
	}

	return ret;
}

s32 ive_thresh_u16(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst,
			   ive_thresh_u16_ctrl_s *pstThrU16Ctrl,
			   bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_isp_dma_ctl_c(rdma_eigval_ctl_c);
	isp_dma_ctl_c rdma_eigval_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Thresh_U16\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	// top
	ive_top_c.reg_h130.reg_thresh_top_mod = 3;
	ive_top_c.reg_h130.reg_thresh_thresh_en = 0;
	ive_top_c.reg_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c.reg_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c.reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	ive_set_wh(&ive_top_c, pstDst->width, pstDst->height, "Thresh_U16", dev_id);

	// setting
	ive_top_c.reg_h144.reg_thresh_u16_enmode = pstThrU16Ctrl->mode;
	ive_top_c.reg_h144.reg_thresh_u16_u8bit_min = pstThrU16Ctrl->min_val;
	ive_top_c.reg_h144.reg_thresh_u16_u8bit_mid = pstThrU16Ctrl->mid_val;
	ive_top_c.reg_h144.reg_thresh_u16_u8bit_max = pstThrU16Ctrl->max_val;
	ive_top_c.reg_h148.reg_thresh_u16_bit_thr_l = pstThrU16Ctrl->low_thr;
	ive_top_c.reg_h148.reg_thresh_u16_bit_thr_h = pstThrU16Ctrl->high_thr;
	writel(ive_top_c.reg_h144.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H144));
	writel(ive_top_c.reg_h148.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H148));

	_ive_16bit_to_8bit(pstSrc, pstDst, &ive_top_c, &ive_filterop_c,
		 &wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, &ive_top_c, &ive_filterop_c, NULL,
				&wdma_y_ctl_c, NULL, NULL, &rdma_eigval_ctl_c, pstSrc,
				NULL, NULL, pstDst, NULL, true, 1, false, 1, false, MOD_THRU16,
				instant, dev_id);
	} else {
		ret = ive_go(ndev, &ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_THRU16, dev_id);
	}

	return ret;
}

s32 ive_ord_stat_filter(struct ive_device *ndev,
				  ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
				  ive_ord_stat_filter_ctrl_s *pstOrdStatFltCtrl,
				  bool instant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_OrdStatFilter\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "OrdStatFilter", dev_id);

	// TODO: check set to 0
	ive_filterop_c.reg_h10.reg_filterop_mode = 2;
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 4; //sw_ovw; bypass op1
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.reg_18.reg_filterop_order_enmode =
		pstOrdStatFltCtrl->mode;

	//bypass op2
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.reg_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));

	set_isp_rdma(MOD_ORDSTAFTR, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_ORDSTAFTR,
				instant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = ive_go(ndev, ive_top_c, instant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_ORDSTAFTR, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 ive_canny_hys_edge(struct ive_device *ndev,
				 ive_src_image_s *pstSrc, ive_dst_image_s *pstEdge,
				 ive_dst_mem_info_s *pstStack,
				 ive_canny_hys_edge_ctrl_s *pstCannyHysEdgeCtrl,
				 bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	ive_dst_image_s _pstStack;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_c_ctl_c);
	isp_dma_ctl_c wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_CannyHysEdge\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstEdge", pstEdge);
		dump_ive_mem("pstStack", pstStack);
	}
	// top
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "CannyHysEdge", dev_id);

	ive_filterop_c.reg_h10.reg_filterop_mode = MOD_CANNY;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	ive_filterop_c.reg_canny_0.reg_canny_lowthr =
		pstCannyHysEdgeCtrl->low_thr;
	ive_filterop_c.reg_canny_0.reg_canny_hithr =
		pstCannyHysEdgeCtrl->high_thr;
	ive_filterop_c.reg_canny_1.reg_canny_en = 1;
	writel(ive_filterop_c.reg_canny_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_0));
	writel(ive_filterop_c.reg_canny_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_1));

	ive_filterop_c.reg_canny_2.reg_canny_eof = 0xfffe7ffd;
	ive_filterop_c.reg_4.reg_filterop_h_coef00 =
		pstCannyHysEdgeCtrl->mask[0];
	ive_filterop_c.reg_4.reg_filterop_h_coef01 =
		pstCannyHysEdgeCtrl->mask[1];
	ive_filterop_c.reg_4.reg_filterop_h_coef02 =
		pstCannyHysEdgeCtrl->mask[2];
	ive_filterop_c.reg_4.reg_filterop_h_coef03 =
		pstCannyHysEdgeCtrl->mask[3];
	ive_filterop_c.reg_5.reg_filterop_h_coef04 =
		pstCannyHysEdgeCtrl->mask[4];
	ive_filterop_c.reg_5.reg_filterop_h_coef10 =
		pstCannyHysEdgeCtrl->mask[5];
	ive_filterop_c.reg_5.reg_filterop_h_coef11 =
		pstCannyHysEdgeCtrl->mask[6];
	ive_filterop_c.reg_5.reg_filterop_h_coef12 =
		pstCannyHysEdgeCtrl->mask[7];
	ive_filterop_c.reg_6.reg_filterop_h_coef13 =
		pstCannyHysEdgeCtrl->mask[8];
	ive_filterop_c.reg_6.reg_filterop_h_coef14 =
		pstCannyHysEdgeCtrl->mask[9];
	ive_filterop_c.reg_6.reg_filterop_h_coef20 =
		pstCannyHysEdgeCtrl->mask[10];
	ive_filterop_c.reg_6.reg_filterop_h_coef21 =
		pstCannyHysEdgeCtrl->mask[11];
	ive_filterop_c.reg_7.reg_filterop_h_coef22 =
		pstCannyHysEdgeCtrl->mask[12];
	ive_filterop_c.reg_7.reg_filterop_h_coef23 =
		pstCannyHysEdgeCtrl->mask[13];
	ive_filterop_c.reg_7.reg_filterop_h_coef24 =
		pstCannyHysEdgeCtrl->mask[14];
	ive_filterop_c.reg_7.reg_filterop_h_coef30 =
		pstCannyHysEdgeCtrl->mask[15];
	ive_filterop_c.reg_8.reg_filterop_h_coef31 =
		pstCannyHysEdgeCtrl->mask[16];
	ive_filterop_c.reg_8.reg_filterop_h_coef32 =
		pstCannyHysEdgeCtrl->mask[17];
	ive_filterop_c.reg_8.reg_filterop_h_coef33 =
		pstCannyHysEdgeCtrl->mask[18];
	ive_filterop_c.reg_8.reg_filterop_h_coef34 =
		pstCannyHysEdgeCtrl->mask[19];
	ive_filterop_c.reg_9.reg_filterop_h_coef40 =
		pstCannyHysEdgeCtrl->mask[20];
	ive_filterop_c.reg_9.reg_filterop_h_coef41 =
		pstCannyHysEdgeCtrl->mask[21];
	ive_filterop_c.reg_9.reg_filterop_h_coef42 =
		pstCannyHysEdgeCtrl->mask[22];
	ive_filterop_c.reg_9.reg_filterop_h_coef43 =
		pstCannyHysEdgeCtrl->mask[23];
	ive_filterop_c.reg_10.reg_filterop_h_coef44 =
		pstCannyHysEdgeCtrl->mask[24];
	writel(ive_filterop_c.reg_canny_2.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_2));
	writel(ive_filterop_c.reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	_pstStack.phy_addr[0] = pstStack->phy_addr;
	// NOTICE: we leverage stride as size
	_pstStack.stride[0] = pstStack->size;

	set_isp_rdma(MOD_CANNY, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstEdge, &wdma_y_ctl_c, dev_id);
	set_img_dst2(&_pstStack, &wdma_c_ctl_c, dev_id);
	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				   &wdma_y_ctl_c, NULL, &wdma_c_ctl_c, NULL, pstSrc,
				   NULL, NULL, pstEdge, &_pstStack, true, 1, true,
				   4, false, MOD_CANNY, instant, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_CANNY, dev_id);
	}
	ive_filterop_c.reg_canny_1.reg_canny_en = 0;
	writel(ive_filterop_c.reg_canny_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_0));
	kfree(ive_top_c);
	return ret;
}

s32 ive_norm_grad(struct ive_device *ndev, ive_src_image_s *pstSrc,
			 ive_dst_image_s *pstDstH, ive_dst_image_s *pstDstV,
			 ive_dst_image_s *pstDstHV,
			 ive_norm_grad_ctrl_s *pstNormGradCtrl,
			 bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	bool enWdma_y = false, enWdma_c = false;
	s32 yunit = 1;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_c_ctl_c);
	isp_dma_ctl_c wdma_c_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_NormGrad\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDstH", pstDstH);
		dump_ive_image("pstDstV", pstDstV);
		dump_ive_image("pstDstHV", pstDstHV);
	}
	// top
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "NormGrad", dev_id);

	ive_filterop_c.reg_h10.reg_filterop_mode = 8;
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.reg_28.reg_filterop_op2_erodila_en = 0;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	ive_filterop_c.reg_4.reg_filterop_h_coef00 =
		pstNormGradCtrl->mask[0];
	ive_filterop_c.reg_4.reg_filterop_h_coef01 =
		pstNormGradCtrl->mask[1];
	ive_filterop_c.reg_4.reg_filterop_h_coef02 =
		pstNormGradCtrl->mask[2];
	ive_filterop_c.reg_4.reg_filterop_h_coef03 =
		pstNormGradCtrl->mask[3];
	ive_filterop_c.reg_5.reg_filterop_h_coef04 =
		pstNormGradCtrl->mask[4];
	ive_filterop_c.reg_5.reg_filterop_h_coef10 =
		pstNormGradCtrl->mask[5];
	ive_filterop_c.reg_5.reg_filterop_h_coef11 =
		pstNormGradCtrl->mask[6];
	ive_filterop_c.reg_5.reg_filterop_h_coef12 =
		pstNormGradCtrl->mask[7];
	ive_filterop_c.reg_6.reg_filterop_h_coef13 =
		pstNormGradCtrl->mask[8];
	ive_filterop_c.reg_6.reg_filterop_h_coef14 =
		pstNormGradCtrl->mask[9];
	ive_filterop_c.reg_6.reg_filterop_h_coef20 =
		pstNormGradCtrl->mask[10];
	ive_filterop_c.reg_6.reg_filterop_h_coef21 =
		pstNormGradCtrl->mask[11];
	ive_filterop_c.reg_7.reg_filterop_h_coef22 =
		pstNormGradCtrl->mask[12];
	ive_filterop_c.reg_7.reg_filterop_h_coef23 =
		pstNormGradCtrl->mask[13];
	ive_filterop_c.reg_7.reg_filterop_h_coef24 =
		pstNormGradCtrl->mask[14];
	ive_filterop_c.reg_7.reg_filterop_h_coef30 =
		pstNormGradCtrl->mask[15];
	ive_filterop_c.reg_8.reg_filterop_h_coef31 =
		pstNormGradCtrl->mask[16];
	ive_filterop_c.reg_8.reg_filterop_h_coef32 =
		pstNormGradCtrl->mask[17];
	ive_filterop_c.reg_8.reg_filterop_h_coef33 =
		pstNormGradCtrl->mask[18];
	ive_filterop_c.reg_8.reg_filterop_h_coef34 =
		pstNormGradCtrl->mask[19];
	ive_filterop_c.reg_9.reg_filterop_h_coef40 =
		pstNormGradCtrl->mask[20];
	ive_filterop_c.reg_9.reg_filterop_h_coef41 =
		pstNormGradCtrl->mask[21];
	ive_filterop_c.reg_9.reg_filterop_h_coef42 =
		pstNormGradCtrl->mask[22];
	ive_filterop_c.reg_9.reg_filterop_h_coef43 =
		pstNormGradCtrl->mask[23];
	ive_filterop_c.reg_10.reg_filterop_h_coef44 =
		pstNormGradCtrl->mask[24];
	ive_filterop_c.reg_10.reg_filterop_h_norm = pstNormGradCtrl->norm;
	writel(ive_filterop_c.reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	set_isp_rdma(MOD_NORMG, NULL, dev_id);
	if (pstNormGradCtrl->out_ctrl == IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER) {
		set_img_dst1(pstDstH, &wdma_y_ctl_c, dev_id);
		set_img_dst2(pstDstV, &wdma_c_ctl_c, dev_id);
		ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl = 0;
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1; // h
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1; // v
		enWdma_y = enWdma_c = true;
	} else if (pstNormGradCtrl->out_ctrl == IVE_NORM_GRAD_OUT_CTRL_HOR) {
		set_img_dst1(pstDstH, &wdma_y_ctl_c, dev_id);
		set_img_dst2(NULL, NULL, dev_id);
		ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl = 1;
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1; // h
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0; // v
		enWdma_y = true;
	} else if (pstNormGradCtrl->out_ctrl == IVE_NORM_GRAD_OUT_CTRL_VER) {
		set_img_dst1(NULL, NULL, dev_id);
		set_img_dst2(pstDstV, &wdma_c_ctl_c, dev_id);
		ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl = 2;
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0; // h
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 1; // v
		enWdma_c = true;
	} else if (pstNormGradCtrl->out_ctrl ==
		   IVE_NORM_GRAD_OUT_CTRL_COMBINE) {
		set_img_dst1(pstDstHV, &wdma_y_ctl_c, dev_id);
		set_img_dst2(NULL, NULL, dev_id);
		ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl = 3;
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1; // h
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0; // v
		enWdma_y = true;
		yunit = 2;
	} else {
		pr_err("Invalid out_ctrl %d\n", pstNormGradCtrl->out_ctrl);
		kfree(ive_top_c);
		return FAILURE;
	}

	writel(ive_filterop_c.reg_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));

	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		if (pstNormGradCtrl->out_ctrl ==
			IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL,
					&wdma_c_ctl_c, NULL, pstSrc, NULL, NULL,
					pstDstH, pstDstV, enWdma_y, yunit,
					enWdma_c, 1, false, MOD_NORMG, instant, dev_id);
			kfree(ive_top_c);
			return ret;
		} else if (pstNormGradCtrl->out_ctrl ==
			   IVE_NORM_GRAD_OUT_CTRL_HOR) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, pstDstH, NULL,
					enWdma_y, yunit, enWdma_c, 1, false, MOD_NORMG,
					instant, dev_id);
			kfree(ive_top_c);
			return ret;
		} else if (pstNormGradCtrl->out_ctrl ==
			   IVE_NORM_GRAD_OUT_CTRL_VER) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, NULL, NULL, &wdma_c_ctl_c,
					NULL, pstSrc, NULL, NULL, NULL, pstDstV,
					enWdma_y, yunit, enWdma_c, 1, false, MOD_NORMG,
					instant, dev_id);
			kfree(ive_top_c);
			return ret;
		} else if (pstNormGradCtrl->out_ctrl ==
			   IVE_NORM_GRAD_OUT_CTRL_COMBINE) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, pstDstHV,
					NULL, enWdma_y, yunit, enWdma_c, 1,
					false, MOD_NORMG, instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}

	if (ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl == 0) {
		ret = ive_go(
			ndev, ive_top_c, instant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK |
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
			MOD_NORMG, dev_id);
	} else if (ive_filterop_c.reg_110.reg_filterop_norm_out_ctrl == 2) {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
				 MOD_NORMG, dev_id);
	} else { //1 3
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_NORMG, dev_id);
	}
	kfree(ive_top_c);
	return ret;
}

s32 ive_grad_fg(struct ive_device *ndev,
			   ive_src_image_s *pstBgDiffFg,
			   ive_src_image_s *pstCurGrad, ive_src_image_s *pstBgGrad,
			   ive_dst_image_s *pstGradFg,
			   ive_grad_fg_ctrl_s *pstGradFgCtrl, bool instant,
			   s32 dev_id)
{
	s32 mode = 0;
	s32 ret = SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_isp_dma_ctl_c(rdma_gradfg_ctl_c);
	isp_dma_ctl_c rdma_gradfg_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(rdma_img1_ctl_c);
	isp_dma_ctl_c rdma_img1_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	ive_gmm_c ive_gmm_c = _DEFINE_IVE_GMM_C;
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_GradFg\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstBgDiffFg", pstBgDiffFg);
		dump_ive_image("pstCurGrad", pstCurGrad);
		dump_ive_image("pstBgGrad", pstBgGrad);
		dump_ive_image("pstGradFg", pstGradFg);
	}
	//test1:rgb:set gmm_gmm2_yonly:0
	//y_only:set gmm_gmm2_yonly:1
	ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly = 0;
	writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

	// top
	ive_set_wh(ive_top_c, pstBgDiffFg->width, pstBgDiffFg->height, "GradFg", dev_id);

	ive_filterop_c.reg_h10.reg_filterop_mode = 6;
	ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
	writel(ive_filterop_c.reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	// TODO: need softrst?
	//iveReg->ive_filterop_c->reg_33.reg_filterop_op2_gradfg_softrst = 0;
	ive_filterop_c.reg_33.reg_filterop_op2_gradfg_en = 1;
	ive_filterop_c.reg_33.reg_filterop_op2_gradfg_enmode =
		pstGradFgCtrl->mode;
	ive_filterop_c.reg_33.reg_filterop_op2_gradfg_edwdark =
		pstGradFgCtrl->edw_dark;
	ive_filterop_c.reg_33.reg_filterop_op2_gradfg_edwfactor =
		pstGradFgCtrl->edw_factor;
	ive_filterop_c.reg_34.reg_filterop_op2_gradfg_crlcoefthr =
		pstGradFgCtrl->crl_coe_thr;
	ive_filterop_c.reg_34.reg_filterop_op2_gradfg_magcrlthr =
		pstGradFgCtrl->mag_crl_thr;
	ive_filterop_c.reg_34.reg_filterop_op2_gradfg_minmagdiff =
		pstGradFgCtrl->min_mag_diff;
	ive_filterop_c.reg_34.reg_filterop_op2_gradfg_noiseval =
		pstGradFgCtrl->noise_val;
	writel(ive_filterop_c.reg_33.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_33));
	writel(ive_filterop_c.reg_34.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_34));
	//iveReg->setDMA(pstBgDiffFg, pstGradFg, pstCurGrad, NULL, pstBgGrad);
	rdma_gradfg_ctl_c.base_addr.reg_basel =
		pstBgGrad->phy_addr[0] & 0xffffffff;
	rdma_gradfg_ctl_c.sys_control.reg_baseh =
		(pstBgGrad->phy_addr[0] >> 32) & 0xffffffff;
	rdma_gradfg_ctl_c.dma_stride.reg_stride = pstBgGrad->stride[0];
	rdma_gradfg_ctl_c.sys_control.reg_stride_sel = 1;
	rdma_gradfg_ctl_c.sys_control.reg_base_sel = 1;
	writel(rdma_gradfg_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_gradfg_ctl_c.dma_stride.val,
			(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(rdma_gradfg_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_SYS_CONTROL));

	set_isp_rdma(MOD_GRADFG, NULL, dev_id);
	if (set_img_src1(pstBgDiffFg, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}

	set_img_src2(pstCurGrad, &rdma_img1_ctl_c, dev_id);
	if (g_dump_dma_info == TRUE) {
		pr_info("Src3 address: 0x%08x %08x\n",
			rdma_gradfg_ctl_c.sys_control.reg_baseh,
			rdma_gradfg_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[RDMA_RADFG].addr_en = rdma_gradfg_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[RDMA_RADFG].addr_l = rdma_gradfg_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[RDMA_RADFG].addr_h = rdma_gradfg_ctl_c.sys_control.reg_baseh & 0xff;

	mode = ive_get_mod_u8(pstBgDiffFg->type);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		kfree(ive_top_c);
		return FAILURE;
	}
	ive_top_c->reg_3.reg_ive_rdma_img1_mod_u8 = mode;

	// TODO: need to set vld?
	ive_top_c->reg_3.reg_imgmux_img0_sel = 0;
	ive_top_c->reg_3.reg_ive_rdma_img1_en = 1;
	ive_top_c->reg_h10.reg_rdma_img1_top_enable = 1;
	ive_filterop_c.reg_h04.reg_gradfg_bggrad_rdma_en = 1;
	ive_top_c->reg_3.reg_ive_rdma_img1_mod_u8 = 0;
	ive_top_c->reg_3.reg_muxsel_gradfg = 1;
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstGradFg, &wdma_y_ctl_c, dev_id);
	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h04.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	if (pstBgDiffFg->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				   &wdma_y_ctl_c, &rdma_img1_ctl_c, NULL, NULL,
				   pstBgDiffFg, pstCurGrad, pstBgGrad, pstGradFg,
				   NULL, true, 1, false, 1, false, MOD_GRADFG,
				   instant, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_GRADFG, dev_id);
	}

	if (instant) {
		ive_filterop_c.reg_33.reg_filterop_op2_gradfg_en = 0;
		ive_filterop_c.reg_h04.reg_gradfg_bggrad_rdma_en = 0;
		ive_top_c->reg_3.reg_muxsel_gradfg = 0;
		writel(ive_filterop_c.reg_33.val,
			(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_33));
		writel(ive_filterop_c.reg_h04.val,
			(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
		writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

		rdma_gradfg_ctl_c.base_addr.reg_basel = 0;
		rdma_gradfg_ctl_c.sys_control.reg_base_sel = 0;
		rdma_gradfg_ctl_c.sys_control.reg_stride_sel = 0;
		rdma_gradfg_ctl_c.sys_control.reg_seglen_sel = 0;
		rdma_gradfg_ctl_c.sys_control.reg_segnum_sel = 0;
		rdma_gradfg_ctl_c.dma_segnum.reg_segnum = 0;
		rdma_gradfg_ctl_c.dma_seglen.reg_seglen = 0;
		rdma_gradfg_ctl_c.dma_stride.reg_stride = 0;
		ive_filterop_c.reg_33.reg_filterop_op2_gradfg_en = 0;
		ive_filterop_c.reg_h04.reg_gradfg_bggrad_rdma_en = 0;
		ive_top_c->reg_3.reg_muxsel_gradfg = 0;

		writel(rdma_gradfg_ctl_c.base_addr.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(rdma_gradfg_ctl_c.sys_control.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_SYS_CONTROL));
		writel(rdma_gradfg_ctl_c.dma_segnum.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGNUM));
		writel(rdma_gradfg_ctl_c.dma_seglen.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGLEN));
		writel(rdma_gradfg_ctl_c.dma_stride.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_STRIDE));
		writel(ive_filterop_c.reg_33.val,
				(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_33));
		writel(ive_filterop_c.reg_h04.val,
				(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
		writel(ive_top_c->reg_3.val,
				(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	}
	kfree(ive_top_c);
	return ret;
}

s32 ive_sad(struct ive_device *ndev, ive_src_image_s *pstSrc1,
			ive_src_image_s *pstSrc2, ive_dst_image_s *pstSad,
			ive_dst_image_s *pstThr, ive_sad_ctrl_s *pstSadCtrl,
			bool instant, s32 dev_id)
{
	s32 mode = 0;
	s32 ret = SUCCESS;
	ive_dst_image_s *pstSadOut = NULL;
	ive_dst_image_s *pstThrOut = NULL;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_SAD_C(ive_sad_c);
	ive_sad_c ive_sad_c = _DEFINE_IVE_SAD_C;
	//DEFINE_isp_dma_ctl_c(wdma_sad_ctl_c);
	isp_dma_ctl_c wdma_sad_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(wdma_sad_thr_ctl_c);
	isp_dma_ctl_c wdma_sad_thr_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_SAD\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		dump_ive_image("pstSad", pstSad);
		dump_ive_image("pstThr", pstThr);
	}
	// check input
	if (pstSrc1->type != IVE_IMAGE_TYPE_U8C1 ||
		pstSrc2->type != IVE_IMAGE_TYPE_U8C1) {
		pr_err("pstSrc1->type %d and pstSrc2->type %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
			   pstSrc1->type, pstSrc2->type, IVE_IMAGE_TYPE_U8C1);
		return FAILURE;
	}

	// check output
	switch (pstSadCtrl->out_ctrl) {
	case IVE_SAD_OUT_CTRL_16BIT_BOTH:
		if (pstSad->type != IVE_IMAGE_TYPE_U16C1 ||
			pstThr->type != IVE_IMAGE_TYPE_U8C1) {
			pr_err("pstSad->type %d (should be IVE_IMAGE_TYPE_U16C1(%d)",
				   pstSad->type, IVE_IMAGE_TYPE_U16C1);
			pr_err(" pstThr->type %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
				   pstThr->type, IVE_IMAGE_TYPE_U8C1);
			return FAILURE;
		}
		pstSadOut = pstSad;
		pstThrOut = pstThr;
		break;
	case IVE_SAD_OUT_CTRL_16BIT_SAD: // dont care thr
		if (pstSad->type != IVE_IMAGE_TYPE_U16C1) {
			pr_err("pstSad->type %d (should be IVE_IMAGE_TYPE_U16C1(%d)",
				   pstSad->type, IVE_IMAGE_TYPE_U16C1);
			return FAILURE;
		}
		pstSadOut = pstSad;
		break;
	case IVE_SAD_OUT_CTRL_THRESH: // only output thresh
		if (pstThr->type != IVE_IMAGE_TYPE_U8C1) {
			pr_err(" pstThr->type %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
				   pstThr->type, IVE_IMAGE_TYPE_U8C1);
			return FAILURE;
		}
		pstThrOut = pstThr;
		break;
	case IVE_SAD_OUT_CTRL_8BIT_BOTH:
		if (pstSad->type != IVE_IMAGE_TYPE_U8C1 ||
			pstThr->type != IVE_IMAGE_TYPE_U8C1) {
			pr_err("pstSad->type %d (should be IVE_IMAGE_TYPE_U8C1(%d)",
				   pstSad->type, IVE_IMAGE_TYPE_U8C1);
			pr_err(" pstThr->type %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
				   pstThr->type, IVE_IMAGE_TYPE_U8C1);
			return FAILURE;
		}
		pstSadOut = pstSad;
		pstThrOut = pstThr;
		break;
	case IVE_SAD_OUT_CTRL_8BIT_SAD:
		if (pstSad->type != IVE_IMAGE_TYPE_U8C1) {
			pr_err("pstSad->type %d (should be IVE_IMAGE_TYPE_U8C1(%d)",
				   pstSad->type, IVE_IMAGE_TYPE_U8C1);
			return FAILURE;
		}
		pstSadOut = pstSad;
		break;
	default:
		pr_err("not support output type %d, return\n",
			   pstSadCtrl->out_ctrl);
		return FAILURE;
	}
	// top
	//ive_top_c.reg_h10.reg_sad_top_enable = 1; // remove ?
	ive_sad_c.reg_sad_02.reg_sad_enable = 1;
	writel(ive_sad_c.reg_sad_02.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_02));

	// align isp
	ive_set_wh(&ive_top_c, pstSrc1->width, pstSrc1->height, "SAD", dev_id);

	// setting
	ive_sad_c.reg_sad_00.reg_sad_enmode = pstSadCtrl->mode;
	ive_sad_c.reg_sad_00.reg_sad_out_ctrl = pstSadCtrl->out_ctrl;
	ive_sad_c.reg_sad_00.reg_sad_u16bit_thr = pstSadCtrl->thr;
	ive_sad_c.reg_sad_01.reg_sad_u8bit_max = pstSadCtrl->max_val;
	ive_sad_c.reg_sad_01.reg_sad_u8bit_min = pstSadCtrl->min_val;

	writel(ive_sad_c.reg_sad_00.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_00));
	writel(ive_sad_c.reg_sad_01.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_01));

	set_isp_rdma(MOD_SAD, NULL, dev_id);
	if (set_img_src1(pstSrc1, &img_in_c, &ive_top_c, dev_id) != SUCCESS) {
		return FAILURE;
	}

	set_img_src2(pstSrc2, NULL, dev_id);
	//set_img_dst1(pstSadOut, NULL);

	mode = ive_get_mod_u8(pstSrc1->type);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		return FAILURE;
	}
	ive_top_c.reg_3.reg_ive_rdma_img1_mod_u8 = mode;

	// TODO: need to set vld?
	ive_top_c.reg_3.reg_imgmux_img0_sel = 0;
	ive_top_c.reg_3.reg_ive_rdma_img1_en = 1;

	// trigger
	ive_top_c.reg_20.reg_frame2op_op_mode = 6;
	ive_top_c.reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c.reg_h10.reg_filterop_top_enable = 0;
	ive_top_c.reg_14.reg_csc_enable = 0;
	ive_top_c.reg_h10.reg_csc_top_enable = 0;
	ive_top_c.reg_h10.reg_rdma_img1_top_enable = 1;
	ive_top_c.reg_h10.reg_sad_top_enable = 1;
	writel(ive_top_c.reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c.reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c.reg_14.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_14));
	writel(ive_top_c.reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c.reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	if (pstSadOut->phy_addr[0]) {
		wdma_sad_ctl_c.base_addr.reg_basel =
			pstSadOut->phy_addr[0] & 0xffffffff;
		wdma_sad_ctl_c.sys_control.reg_baseh =
			(pstSadOut->phy_addr[0] >> 32) & 0xffffffff;
		wdma_sad_ctl_c.sys_control.reg_base_sel = 1;

		wdma_sad_ctl_c.sys_control.reg_stride_sel = 1; //1;
		wdma_sad_ctl_c.sys_control.reg_seglen_sel = 0;
		wdma_sad_ctl_c.sys_control.reg_segnum_sel = 0;

		// set height
		wdma_sad_ctl_c.dma_segnum.reg_segnum = 0;
		// set width
		wdma_sad_ctl_c.dma_seglen.reg_seglen = 0;
		// set stride
		wdma_sad_ctl_c.dma_stride.reg_stride = pstSadOut->stride[0];
	} else {
		wdma_sad_ctl_c.sys_control.reg_base_sel = 0;
		wdma_sad_ctl_c.base_addr.reg_basel = 0;
		wdma_sad_ctl_c.sys_control.reg_baseh = 0;
	}

	if (pstThrOut->phy_addr[0]) {
		wdma_sad_thr_ctl_c.base_addr.reg_basel =
			pstThrOut->phy_addr[0] & 0xffffffff;
		wdma_sad_thr_ctl_c.sys_control.reg_baseh =
			(pstThrOut->phy_addr[0] >> 32) & 0xffffffff;
		wdma_sad_thr_ctl_c.sys_control.reg_base_sel = 1;

		wdma_sad_thr_ctl_c.sys_control.reg_stride_sel = 1; //1;
		wdma_sad_thr_ctl_c.sys_control.reg_seglen_sel = 0;
		wdma_sad_thr_ctl_c.sys_control.reg_segnum_sel = 0; //1;

		// set height
		wdma_sad_thr_ctl_c.dma_segnum.reg_segnum = 0;
		// set width
		wdma_sad_thr_ctl_c.dma_seglen.reg_seglen = 0;
		// set stride
		wdma_sad_thr_ctl_c.dma_stride.reg_stride = pstThrOut->stride[0];
	} else {
		wdma_sad_thr_ctl_c.sys_control.reg_base_sel = 0;
		wdma_sad_thr_ctl_c.base_addr.reg_basel = 0;
		wdma_sad_thr_ctl_c.sys_control.reg_baseh = 0;
		wdma_sad_thr_ctl_c.sys_control.reg_stride_sel = 0;
		wdma_sad_thr_ctl_c.sys_control.reg_seglen_sel = 0;
		wdma_sad_thr_ctl_c.sys_control.reg_segnum_sel = 0;
		wdma_sad_thr_ctl_c.dma_segnum.reg_segnum = 0;
		wdma_sad_thr_ctl_c.dma_seglen.reg_seglen = 0;
		wdma_sad_thr_ctl_c.dma_stride.reg_stride = 0;
	}
	if (g_dump_dma_info == TRUE) {
		pr_info("Dst Sad address: 0x%08x %08x\n",
			wdma_sad_ctl_c.sys_control.reg_baseh,
			wdma_sad_ctl_c.base_addr.reg_basel);
		pr_info("Dst Sad thr address: 0x%08x %08x\n",
			wdma_sad_thr_ctl_c.sys_control.reg_baseh,
			wdma_sad_thr_ctl_c.base_addr.reg_basel);
	}
	g_debug_info.addr[WDMA_SAD].addr_en = wdma_sad_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_SAD].addr_l = wdma_sad_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_SAD].addr_h = wdma_sad_ctl_c.sys_control.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_SAD_THR].addr_en = wdma_sad_thr_ctl_c.sys_control.reg_base_sel;
	g_debug_info.addr[WDMA_SAD_THR].addr_l = wdma_sad_thr_ctl_c.base_addr.reg_basel;
	g_debug_info.addr[WDMA_SAD_THR].addr_h = wdma_sad_thr_ctl_c.sys_control.reg_baseh & 0xff;

	writel(wdma_sad_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_sad_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(wdma_sad_ctl_c.dma_segnum.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_DMA_SEGNUM));
	writel(wdma_sad_ctl_c.dma_seglen.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_DMA_SEGLEN));
	writel(wdma_sad_ctl_c.dma_stride.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_DMA_STRIDE));

	writel(wdma_sad_thr_ctl_c.base_addr.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_sad_thr_ctl_c.sys_control.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_SYS_CONTROL));
	writel(wdma_sad_thr_ctl_c.dma_segnum.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_DMA_SEGNUM));
	writel(wdma_sad_thr_ctl_c.dma_seglen.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_DMA_SEGLEN));
	writel(wdma_sad_thr_ctl_c.dma_stride.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_DMA_STRIDE));
	ndev->cur_optype = MOD_SAD;
	ret = ive_go(ndev, &ive_top_c, instant,
			 IVE_TOP_REG_FRAME_DONE_SAD_MASK, MOD_SAD, dev_id);

	ive_sad_c.reg_sad_02.reg_sad_enable = 0;
	writel(ive_sad_c.reg_sad_02.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_02));

	ive_sad_c.reg_sad_02.reg_sad_enable = 0;
	writel(ive_sad_c.reg_sad_02.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_02));
	// reset flow
	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_BASE_ADDR));
	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_SYS_CONTROL));

	ive_top_c.reg_3.reg_ive_rdma_img1_en = 0;
	writel(ive_top_c.reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_top_c.reg_20.reg_frame2op_op_mode = 0;
	ive_top_c.reg_h80.reg_frame2op_fg_op_mode = 0;
	writel(ive_top_c.reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c.reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));

	writel(img_in_c.reg_00.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));
	writel(img_in_c.reg_02.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02));
	writel(img_in_c.reg_05.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_05));

	return ret;
}

void _sclr_get_2tap_scale(ive_top_c *ive_top_c, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstDst, s32 dev_id)
{
	u32 src_wd, src_ht, dst_wd, dst_ht, scale_x, scale_y;
	bool fast_area_x = 0, fast_area_y = 0;
	u32 h_nor = 0, v_nor = 0, h_ph = 0, v_ph = 0;
	bool area_fast = false;
	bool scale_down = false;

	if (!pstSrc->width || !pstSrc->height || !pstDst->width ||
		!pstDst->height)
		return;

	src_wd = pstSrc->width;
	src_ht = pstSrc->height;
	dst_wd = pstDst->width;
	dst_ht = pstDst->height;

	// Scale up: bilinear mode
	// Scale down: area mode
	if (src_wd >= dst_wd && src_ht >= dst_ht) {
		scale_down = true;
		fast_area_x = (src_wd % dst_wd) ? false : true;
		fast_area_y = (src_ht % dst_ht) ? false : true;
		area_fast = (fast_area_x && fast_area_y) ? true : false;
	}

	// scale_x = round((src_wd * 2^13)/dst_wd),0) -> 5.13
	scale_x = (src_wd * 8192) / dst_wd;

	// scale_y = round((src_ht * 2^13)/dst_ht),0) -> 5.13
	scale_y = (src_ht * 8192) / dst_ht;

	if (!area_fast) {
		h_nor = (65536 * 8192) / scale_x;
		v_nor = (65536 * 8192) / scale_y;
	} else {
		// h_nor: don't care
		h_nor = (65536 * 8192) / scale_x;
		v_nor = 65536 / ((scale_x >> 13) * (scale_y >> 13));
	}

	// Phase is used when scale up and reg_resize_blnr_mode = 0
	// Note: scale down with nonzero phase caused scaler blocked
	if (!scale_down) {
		h_ph = scale_x / 2;
		v_ph = scale_y / 2;
	}

	ive_top_c->reg_rs_ctrl.reg_resize_area_fast = 1;
	ive_top_c->reg_rs_ctrl.reg_resize_blnr_mode = 0;

	ive_top_c->reg_rs_h_sc.reg_resize_h_sc_fac = scale_x;
	ive_top_c->reg_rs_v_sc.reg_resize_v_sc_fac = scale_y;
	ive_top_c->reg_rs_ph_ini.reg_resize_h_ini_ph = h_ph;
	ive_top_c->reg_rs_ph_ini.reg_resize_v_ini_ph = v_ph;
	ive_top_c->reg_rs_nor.reg_resize_h_nor = h_nor;
	ive_top_c->reg_rs_nor.reg_resize_v_nor = v_nor;
	writel(ive_top_c->reg_rs_ctrl.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_CTRL));
	writel(ive_top_c->reg_rs_h_sc.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_H_SC));
	writel(ive_top_c->reg_rs_v_sc.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_V_SC));
	writel(ive_top_c->reg_rs_ph_ini.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_PH_INI));
	writel(ive_top_c->reg_rs_nor.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_NOR));
}

s32 ive_resize(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst,
			   ive_resize_ctrl_s *pstResizeCtrl, bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	ive_gmm_c ive_gmm_c = _DEFINE_IVE_GMM_C;
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_Resize\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
			char tmp[32];
			dump_ive_image(tmp, pstSrc);
			dump_ive_image(tmp, pstDst);
	}
	if (pstResizeCtrl->mode != IVE_RESIZE_MODE_AREA &&
		pstResizeCtrl->mode != IVE_RESIZE_MODE_LINEAR) {
		pr_err("Invalid Resize mode %d\n", pstResizeCtrl->mode);
		kfree(ive_top_c);
		return FAILURE;
	}
	// for (i = 0; i < pstResizeCtrl->num; i++) {
		if (pstDst->type != IVE_IMAGE_TYPE_U8C3_PLANAR &&
			pstDst->type != IVE_IMAGE_TYPE_U8C1) {
			pr_err("Invalid IMAGE TYPE pstDst %d\n",
				   pstDst->type);
			pr_err("Invalid IMAGE TYPE pstSrc %d\n",
				   pstSrc->type);
			pr_err("Invalid IMAGE Width pstDst %d %d\n",
				   pstDst->width, pstDst->height);
			pr_err("Invalid IMAGE Width pstSrc %d %d\n",
				   pstSrc->width, pstSrc->height);
			kfree(ive_top_c);
			return FAILURE;
		}
		//test1:rgb:set gmm_gmm2_yonly:0
		//y_only:set gmm_gmm2_yonly:1
		if (pstDst->type == IVE_IMAGE_TYPE_U8C3_PLANAR) {
			ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly = 0;
			writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		}
		else{
			ive_gmm_c.reg_gmm_13.reg_gmm_gmm2_yonly = 1;
			writel(ive_gmm_c.reg_gmm_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		}

		// top
		ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "Resize", dev_id);

		// setting
		// TODO: need softrst?
		ive_top_c->reg_rs_src_size.reg_resize_src_wd =
			pstSrc->width - 1;
		ive_top_c->reg_rs_src_size.reg_resize_src_ht =
			pstSrc->height - 1;
		ive_top_c->reg_rs_dst_size.reg_resize_dst_wd =
			pstDst->width - 1;
		ive_top_c->reg_rs_dst_size.reg_resize_dst_ht =
			pstDst->height - 1;
		writel(ive_top_c->reg_rs_src_size.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_SRC_SIZE));
		writel(ive_top_c->reg_rs_dst_size.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_DST_SIZE));
		_sclr_get_2tap_scale(ive_top_c, pstSrc, pstDst, dev_id);

		ive_top_c->reg_rs_ctrl.reg_resize_ip_en = 1;
		ive_top_c->reg_rs_ctrl.reg_resize_dbg_en = 0;
		writel(ive_top_c->reg_rs_ctrl.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_CTRL));

		if (pstResizeCtrl->mode == IVE_RESIZE_MODE_LINEAR) {
			ive_top_c->reg_rs_ctrl.reg_resize_blnr_mode = 1;
			writel(ive_top_c->reg_rs_ctrl.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_CTRL));
		}

		//filter3ch bypass
		if (pstSrc->type == IVE_IMAGE_TYPE_U8C3_PLANAR) {
			ive_filterop_c.reg_h10.reg_filterop_mode = 1;
			ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 0;
			img_in_c.reg_00.reg_auto_csc_en = 0;
			ive_filterop_c.reg_h1c8.reg_filterop_op2_csc_enable = 0;
			ive_filterop_c.reg_h14.reg_filterop_3ch_en = 0;
			writel(img_in_c.reg_00.val,
				   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

		} else {
			ive_filterop_c.reg_h10.reg_filterop_mode = 2;
			ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0;
			ive_filterop_c.reg_h14.reg_filterop_sw_ovw_op = 1;
			ive_filterop_c.reg_h1c8.reg_filterop_op2_csc_enable = 0;
			ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 0;

			ive_top_c->reg_20.reg_frame2op_op_mode = 5;
			ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
			writel(ive_top_c->reg_r2y4_14.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));
			writel(ive_top_c->reg_20.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
			writel(ive_top_c->reg_h80.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		}
		writel(ive_filterop_c.reg_h10.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		writel(ive_filterop_c.reg_h1c8.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));

		set_isp_rdma(MOD_RESIZE, NULL, dev_id);
		if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
			kfree(ive_top_c);
			return FAILURE;
		}
		// trigger filterop
		//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->reg_20.reg_frame2op_op_mode = 5;
		// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
		ive_top_c->reg_h10.reg_filterop_top_enable = 1;
		ive_top_c->reg_h10.reg_resize_top_enable = 1;
		writel(ive_top_c->reg_20.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
		writel(ive_top_c->reg_h80.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		writel(ive_top_c->reg_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

		set_odma(pstDst, &ive_filterop_c, pstDst->width,
			pstDst->height, dev_id);
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		//if (astSrc[i].width > 480) {
		//	ret = emit_tile(ndev, &ive_top_c, &ive_filterop_c, &img_in_c,
		//		NULL, NULL, NULL, NULL, &astSrc[i], NULL,
		//		NULL, &astDst[i], NULL, false, 1, false, 1, true, 0,
		//		instant);
		//} else {
			ret = ive_go(ndev, ive_top_c, instant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
				MOD_RESIZE, dev_id);
		//}
	// }
	kfree(ive_top_c);
	return ret;
}

s32 ive_ccl(struct ive_device *ndev, ive_image_s *src_dst,
			ive_dst_mem_info_s *blob, ive_ccl_ctrl_s *ccl_ctrl,
			bool instant, s32 dev_id)
{
	s32 ret = SUCCESS;
	u64 srcAddr, dstAddr;
	u8 region_num = 0;
	s8 label_status = 0;
	u16 cur_area_thr = 0;

	IVE_CCL_C ive_ccl_c = _DEFINE_IVE_CCL_C;
	isp_dma_ctl_c ccl_src_rdma_label = _DEFINE_isp_dma_ctl_c;
	isp_dma_ctl_c ccl_dst_wdma_label = _DEFINE_isp_dma_ctl_c;
	isp_dma_ctl_c ccl_region_wdma = _DEFINE_isp_dma_ctl_c;
	isp_dma_ctl_c ccl_src_rdma_relabel = _DEFINE_isp_dma_ctl_c;
	isp_dma_ctl_c ccl_dst_wdma_relabel = _DEFINE_isp_dma_ctl_c;

	ive_top_c *ive_top_c = init_ive_top_c();
    if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "mpi_ive_ccl\n");
	ive_reset(ndev, ive_top_c, dev_id);

	ive_set_wh(ive_top_c, src_dst->width, src_dst->height, "CCL", dev_id);

	// set ccl mode
	ive_ccl_c.reg_ccl_00.reg_ccl_mode = ccl_ctrl->mode;
	ive_ccl_c.reg_ccl_01.reg_ccl_area_step = ccl_ctrl->u16Step;
	ive_ccl_c.reg_ccl_01.reg_ccl_area_thr = ccl_ctrl->init_area_thr;

	writel(ive_ccl_c.reg_ccl_00.val, (IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_00));
	writel(ive_ccl_c.reg_ccl_01.val, (IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_01));

    set_isp_rdma(MOD_CCL, NULL, dev_id);

	srcAddr = src_dst->phy_addr[0];
	dstAddr = blob->phy_addr;

	if(srcAddr){
		// SRC RDMA ADDR
		ccl_src_rdma_label.base_addr.reg_basel = srcAddr & 0xffffffff;
		ccl_src_rdma_label.sys_control.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_src_rdma_label.sys_control.reg_base_sel = 1;

		writel(ccl_src_rdma_label.base_addr.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_src_rdma_label.sys_control.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA + ISP_DMA_CTL_SYS_CONTROL));

		// DST RDMA ADDR
		ccl_dst_wdma_label.base_addr.reg_basel = srcAddr & 0xffffffff;
		ccl_dst_wdma_label.sys_control.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_dst_wdma_label.sys_control.reg_base_sel = 1;

		writel(ccl_dst_wdma_label.base_addr.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_dst_wdma_label.sys_control.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA + ISP_DMA_CTL_SYS_CONTROL));

		ccl_src_rdma_relabel.base_addr.reg_basel = srcAddr & 0xffffffff;
		ccl_src_rdma_relabel.sys_control.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_src_rdma_relabel.sys_control.reg_base_sel = 1;

		writel(ccl_src_rdma_relabel.base_addr.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA_RELABEL + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_src_rdma_relabel.sys_control.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA_RELABEL + ISP_DMA_CTL_SYS_CONTROL));

		ccl_dst_wdma_relabel.base_addr.reg_basel = srcAddr & 0xffffffff;
		ccl_dst_wdma_relabel.sys_control.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_dst_wdma_relabel.sys_control.reg_base_sel = 1;

		writel(ccl_dst_wdma_relabel.base_addr.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA_RELABEL + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_dst_wdma_relabel.sys_control.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA_RELABEL + ISP_DMA_CTL_SYS_CONTROL));
	}

	if(dstAddr){
		dstAddr = dstAddr + 0x10;  // skip first 32bits in struct ive_ccblob_s and 16 Bytes align
		ccl_region_wdma.base_addr.reg_basel = dstAddr & 0xffffffff;
		ccl_region_wdma.sys_control.reg_baseh = (dstAddr >> 32) & 0xffffffff;
		ccl_region_wdma.sys_control.reg_base_sel = 1;

		writel(ccl_region_wdma.base_addr.val,
			(IVE_BLK_BA[dev_id].CCL_REGION_WDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_region_wdma.sys_control.val,
			(IVE_BLK_BA[dev_id].CCL_REGION_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	}

	ive_top_c->reg_h10.reg_ccl_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ndev->cur_optype = MOD_CCL;
	ret = ive_go(ndev, ive_top_c, instant, IVE_TOP_REG_FRAME_DONE_CCL_MASK,
			   MOD_CCL, dev_id);

	if(ret == SUCCESS) {
		region_num = (readl(IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_03) &
		        IVE_CCL_REG_CCL_REGION_NUM_MASK) >> IVE_CCL_REG_CCL_REGION_NUM_OFFSET;
		label_status = (readl(IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_03) &
		        IVE_CCL_REG_CCL_LABEL_STATUS_MASK) >> IVE_CCL_REG_CCL_LABEL_STATUS_OFFSET;
		cur_area_thr = (readl(IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_03) &
		        IVE_CCL_REG_CCL_CUR_AREA_THR_MASK)>> IVE_CCL_REG_CCL_CUR_AREA_THR_OFFSET;

		memcpy((char *)(uintptr_t)blob->vir_addr, &cur_area_thr, sizeof(u16));
		memcpy((char *)(uintptr_t)blob->vir_addr + sizeof(u16), &label_status, sizeof(s8));
		memcpy((char *)(uintptr_t)blob->vir_addr + sizeof(u16) + sizeof(s8),
				&region_num, sizeof(u8));
	}

	kfree(ive_top_c);
	return ret;
}

s32 ive_imgIn_to_odma(struct ive_device *ndev,
				ive_src_image_s *pstSrc, ive_dst_image_s *pstDst,
				ive_filter_ctrl_s *pstFltCtrl, bool instant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_imgInToOdma\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	// top
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "ImgInToOdma", dev_id);

	set_isp_rdma(MOD_BYP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}
	ive_filterop_c.reg_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.reg_h14.reg_filterop_3ch_en = 0;

	if (pstDst->type == IVE_IMAGE_TYPE_U8C1) {
		set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);
		ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
		writel(ive_filterop_c.reg_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		ive_filterop_c.odma_reg_00.reg_dma_en = 0;
		writel(ive_filterop_c.odma_reg_00.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
		if (pstDst->width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, pstDst, NULL,
					true, 1, false, 1, false, 0, instant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}
	set_odma(pstDst, &ive_filterop_c, pstDst->width, pstDst->height, dev_id);
	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstDst->width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				NULL, NULL, NULL, NULL, pstSrc, NULL, NULL,
				pstDst, NULL, false, 1, false, 1, true, MOD_BYP,
				instant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = ive_go(ndev, ive_top_c, instant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK, MOD_BYP, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 ive_rgbp_to_yuv_to_erode_to_dilate(struct ive_device *ndev,
					 ive_src_image_s *pstSrc,
					 ive_dst_image_s *pstDst,
					 ive_dst_image_s *pstDst2,
					 ive_filter_ctrl_s *pstFltCtrl,
					 bool instant, s32 dev_id)
{
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "MPI_IVE_rgbPToYuvToErodeToDilate\n");
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "rgbPToYuvToEToD", dev_id);

	set_isp_rdma(MOD_BYP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != SUCCESS) {
		kfree(ive_top_c);
		return FAILURE;
	}
	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	// d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	ive_top_c->reg_r2y4_14.reg_csc_r2y4_enable = 1;
	ive_top_c->reg_h10.reg_r2y4_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_r2y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, NULL, dev_id);
	ive_filterop_c.reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	//iveReg->setDMA(pstSrc, pstDst, NULL, pstDst2);
	ive_go(ndev, ive_top_c, instant,
		   IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_BYP, dev_id);
	kfree(ive_top_c);
	return SUCCESS;
}

s32 ive_hw_equalize_hist(struct ive_device *ndev,
				ive_src_image_s *pstSrc,
				ive_dst_image_s *pstDst,
				ive_equalize_hist_ctrl_s *pstEqualizeHistCtrl,
				bool instant)
{
	pr_err("MPI_IVE_EqualizeHist not implement yet\n");
	return SUCCESS;
}

s32
hw_stbox_flt_and_eig_calc(struct ive_device *ndev, ive_src_image_s *pstSrc,
			   ive_dst_image_s *pstDst, s8 *mask,
			   u16 *max_eig, bool instant,
			   ive_top_c *ive_top_c, ive_filterop_c *ive_filterop_c,
			   img_in_c *img_in_c, isp_dma_ctl_c *wdma_y_ctl_c, s32 dev_id)
{
	TRACE_IVE(IVE_DBG_INFO, "HW_STBoxFltAndEigCalc\n");
	// top
	ive_set_wh(ive_top_c, pstSrc->width, pstSrc->height, "STBoxFAndEigCal", dev_id);

	ive_filterop_c->reg_st_eigval_1.reg_sw_clr_max_eigval = 1;
	writel(ive_filterop_c->reg_st_eigval_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_1));
	ive_filterop_c->reg_h10.reg_filterop_mode = 5;
	ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c->reg_28.reg_filterop_op2_erodila_en = 0;
	ive_filterop_c->reg_canny_0.reg_canny_lowthr = 0;
	ive_filterop_c->reg_canny_0.reg_canny_hithr = 0;
	ive_filterop_c->reg_canny_1.reg_canny_en = 0;
	ive_filterop_c->reg_canny_3.reg_canny_basel = 0;
	ive_filterop_c->reg_canny_4.reg_canny_baseh = 0;
	ive_filterop_c->reg_st_candi_0.reg_st_candi_corner_bypass = 0x0;
	ive_filterop_c->reg_110.reg_filterop_norm_out_ctrl = 3; //1;
	ive_filterop_c->reg_h14.reg_filterop_3ch_en = 0;
	ive_filterop_c->reg_st_eigval_0.reg_st_eigval_tile_num = 0;
	ive_filterop_c->reg_st_eigval_1.reg_sw_clr_max_eigval = 1;
	writel(ive_filterop_c->reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->reg_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));
	writel(ive_filterop_c->reg_canny_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_0));
	writel(ive_filterop_c->reg_canny_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_1));
	writel(ive_filterop_c->reg_canny_3.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_3));
	writel(ive_filterop_c->reg_canny_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_4));
	writel(ive_filterop_c->reg_st_candi_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_CANDI_0));
	writel(ive_filterop_c->reg_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_reg_110));
	writel(ive_filterop_c->reg_st_eigval_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_0));
	writel(ive_filterop_c->reg_st_eigval_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_1));
	ive_filterop_c->reg_4.reg_filterop_h_coef00 = mask[0];
	ive_filterop_c->reg_4.reg_filterop_h_coef01 = mask[1];
	ive_filterop_c->reg_4.reg_filterop_h_coef02 = mask[2];
	ive_filterop_c->reg_4.reg_filterop_h_coef03 = mask[3];
	ive_filterop_c->reg_5.reg_filterop_h_coef04 = mask[4];
	ive_filterop_c->reg_5.reg_filterop_h_coef10 = mask[5];
	ive_filterop_c->reg_5.reg_filterop_h_coef11 = mask[6];
	ive_filterop_c->reg_5.reg_filterop_h_coef12 = mask[7];
	ive_filterop_c->reg_6.reg_filterop_h_coef13 = mask[8];
	ive_filterop_c->reg_6.reg_filterop_h_coef14 = mask[9];
	ive_filterop_c->reg_6.reg_filterop_h_coef20 = mask[10];
	ive_filterop_c->reg_6.reg_filterop_h_coef21 = mask[11];
	ive_filterop_c->reg_7.reg_filterop_h_coef22 = mask[12];
	ive_filterop_c->reg_7.reg_filterop_h_coef23 = mask[13];
	ive_filterop_c->reg_7.reg_filterop_h_coef24 = mask[14];
	ive_filterop_c->reg_7.reg_filterop_h_coef30 = mask[15];
	ive_filterop_c->reg_8.reg_filterop_h_coef31 = mask[16];
	ive_filterop_c->reg_8.reg_filterop_h_coef32 = mask[17];
	ive_filterop_c->reg_8.reg_filterop_h_coef33 = mask[18];
	ive_filterop_c->reg_8.reg_filterop_h_coef34 = mask[19];
	ive_filterop_c->reg_9.reg_filterop_h_coef40 = mask[20];
	ive_filterop_c->reg_9.reg_filterop_h_coef41 = mask[21];
	ive_filterop_c->reg_9.reg_filterop_h_coef42 = mask[22];
	ive_filterop_c->reg_9.reg_filterop_h_coef43 = mask[23];
	ive_filterop_c->reg_10.reg_filterop_h_coef44 = mask[24];
	ive_filterop_c->reg_10.reg_filterop_h_norm = 3;
	writel(ive_filterop_c->reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c->reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c->reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c->reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c->reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c->reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c->reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	set_isp_rdma(MOD_STBOX, NULL, dev_id);
	if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != SUCCESS) {
		return FAILURE;
	}

	ive_top_c->reg_20.reg_frame2op_op_mode = 5;
	ive_top_c->reg_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->reg_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, wdma_y_ctl_c, dev_id);

	ive_filterop_c->reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->width > 480) {
		emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
			 wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL, NULL,
			 pstDst, NULL, true, 2, false, 1, false, MOD_STBOX,
			 instant, dev_id);
	} else {
		ive_go(ndev, ive_top_c, instant,
			   IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
			   MOD_STBOX, dev_id);
	}

	*max_eig = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_0) &
			 IVE_FILTEROP_REG_ST_EIGVAL_MAX_EIGVAL_MASK;
	return SUCCESS;
}

s32 _hw_stcandi_corner(struct ive_device *ndev, ive_src_image_s *pstSrc,
			  ive_dst_image_s *pstDst, u16 max_eig,
			  ive_st_candi_corner_ctrl_s *pstStCandiCornerCtrl,
			  bool instant, ive_top_c *ive_top_c,
			  ive_filterop_c *ive_filterop_c, img_in_c *img_in_c,
			  isp_dma_ctl_c *wdma_y_ctl_c,
			  isp_dma_ctl_c *rdma_eigval_ctl_c,
			  s32 dev_id)
{
	s32 ret = SUCCESS;
	u8 numerator = 255;

	//in case of tile mode
	ive_set_wh(ive_top_c, pstDst->width, pstDst->height, "STCandiCorner", dev_id);

	ive_top_c->reg_h130.reg_thresh_top_mod = 1;
	ive_top_c->reg_h130.reg_thresh_thresh_en = 1;
	ive_top_c->reg_h10.reg_thresh_top_enable = 1;
	ive_top_c->reg_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->reg_h130.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	// setting
	ive_filterop_c->reg_h10.reg_filterop_mode = 10;
	ive_filterop_c->reg_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c->reg_h14.reg_filterop_sw_ovw_op = 0;
	writel(ive_filterop_c->reg_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_top_c->reg_h134.val = IVE_TOP_16_8_8(
		readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H134) & 0xF8, 0, 0,
		IVE_16BIT_TO_8BIT_MODE_U16_TO_U8);
	ive_top_c->reg_h138.val = IVE_TOP_16_8_8(
		readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H138) & 0xFFFF00FE, 0,
		numerator, 1);
	writel(ive_top_c->reg_h134.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H134));
	writel(ive_top_c->reg_h138.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H138));

	ive_top_c->reg_h14c.val =
		IVE_TOP_16_8_8(0, IVE_THRESH_MODE_TO_MINVAL, 0,
				   pstStCandiCornerCtrl->quality_level);
	ive_top_c->reg_h150.val = 0;
	ive_filterop_c->reg_18.reg_filterop_order_enmode =
		IVE_ORD_STAT_FILTER_MODE_MAX;
	writel(ive_top_c->reg_h14c.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H14C));
	writel(ive_top_c->reg_h150.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H150));
	writel(ive_filterop_c->reg_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));
	ive_filterop_c->reg_4.val = 0;
	ive_filterop_c->reg_5.val = 0;
	ive_filterop_c->reg_6.val = 0;
	ive_filterop_c->reg_7.val = 0;
	ive_filterop_c->reg_8.val = 0;
	ive_filterop_c->reg_9.val = 0;
	ive_filterop_c->reg_10.reg_filterop_h_coef44 = 0;
	ive_filterop_c->reg_10.reg_filterop_h_norm = 0;
	writel(ive_filterop_c->reg_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c->reg_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c->reg_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c->reg_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c->reg_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c->reg_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c->reg_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	ive_top_c->reg_h10.reg_img_in_top_enable = 0;
	ive_top_c->reg_h10.reg_rdma_eigval_top_enable = 1;
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_isp_rdma(MOD_STCANDI, NULL, dev_id);
	set_rdma_eigval(pstSrc, rdma_eigval_ctl_c, dev_id);

	ive_top_c->reg_3.reg_mapmux_rdma_sel = 1;
	ive_top_c->reg_3.reg_ive_rdma_eigval_en = 1;
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	set_img_dst1(pstDst, wdma_y_ctl_c, dev_id);
	ive_filterop_c->reg_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->reg_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->reg_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ndev->cur_optype = MOD_STCANDI;
	if (pstSrc->width > 480) {
		ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
				   wdma_y_ctl_c, NULL, NULL, rdma_eigval_ctl_c,
				   pstSrc, NULL, NULL, pstDst, NULL, true, 1, false,
				   1, false, MOD_STCANDI, instant, dev_id);
	} else {
		ret = ive_go(ndev, ive_top_c, instant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_STCANDI, dev_id);
	}

	//Restore setting
	ive_top_c->reg_3.reg_mapmux_rdma_sel = 0;
	ive_top_c->reg_3.reg_ive_rdma_eigval_en = 0;
	ive_top_c->reg_h130.reg_thresh_top_mod = 0;
	ive_top_c->reg_h130.reg_thresh_thresh_en = 0;
	ive_top_c->reg_h138.reg_thresh_st_16to8_en = 0;
	ive_top_c->reg_h10.reg_thresh_top_enable = 0;
	ive_top_c->reg_h10.reg_rdma_eigval_top_enable = 0;
	writel(ive_top_c->reg_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->reg_h130.val,
		(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c->reg_h138.val,
		(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H138));
	writel(ive_top_c->reg_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	return ret;
}

s32 ive_stcandi_corner(struct ive_device *ndev,
				  ive_src_image_s *pstSrc,
				  ive_dst_image_s *pstCandiCorner,
				  ive_st_candi_corner_ctrl_s *pstStCandiCornerCtrl,
				  bool instant, s32 dev_id)
{
	s32 ret = SUCCESS, i;
	u32 u32SizeS8C2, u32SizeU8C1;
	u16 max_eig;
	ive_dst_image_s stEigenMap;
	s8 mask[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
				   2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_img_in_c(img_in_c);
	img_in_c img_in_c = _DEFINE_img_in_c;
	//DEFINE_isp_dma_ctl_c(wdma_y_ctl_c);
	isp_dma_ctl_c wdma_y_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_isp_dma_ctl_c(rdma_eigval_ctl_c);
	isp_dma_ctl_c rdma_eigval_ctl_c = _DEFINE_isp_dma_ctl_c;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	TRACE_IVE(IVE_DBG_INFO, "mpi_ive_st_candi_corner\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstCandiCorner", pstCandiCorner);
	}
	if (pstSrc->type != IVE_IMAGE_TYPE_U8C1) {
		pr_err("pstSrc->type(%d) must be U8C1(%d)\n", pstSrc->type,
			   IVE_IMAGE_TYPE_U8C1);
		kfree(ive_top_c);
		return FAILURE;
	}

	if (pstCandiCorner->type != IVE_IMAGE_TYPE_U8C1) {
		pr_err("pstCandiCorner->type(%d) must be U8C1(%d)\n",
			   pstCandiCorner->type, IVE_IMAGE_TYPE_U8C1);
		kfree(ive_top_c);
		return FAILURE;
	}

	if (pstSrc->height != pstCandiCorner->height ||
		pstSrc->width != pstCandiCorner->width) {
		pr_err("pstCandiCorner->width(%d) and pstCandiCorner->height(%d) ",
			   pstCandiCorner->width, pstCandiCorner->height);
		pr_err("must be equal to pstSrc->width(%d) and pstSrc->height(%d)\n",
			   pstSrc->width, pstSrc->height);
		kfree(ive_top_c);
		return FAILURE;
	}

	if (!pstStCandiCornerCtrl->mem.phy_addr) {
		pr_err("pstStCandiCornerCtrl->mem.phy_addr can't be 0!\n");
		kfree(ive_top_c);
		return FAILURE;
	}

	if (pstStCandiCornerCtrl->mem.size <
		4 * pstSrc->height * pstSrc->stride[0] +
			sizeof(ive_st_max_eig_s)) {
		pr_err("pstStCandiCornerCtrl->mem.size must be greater than or equal to %zu!\n",
			   4 * pstSrc->height * pstSrc->stride[0] +
				   sizeof(ive_st_max_eig_s));
		kfree(ive_top_c);
		return FAILURE;
	}

	if (pstStCandiCornerCtrl->quality_level == 0) {
		pr_err("pstStCandiCornerCtrl->quality_level can't be 0!");
		kfree(ive_top_c);
		return FAILURE;
	}

	u32SizeS8C2 = pstSrc->height * pstSrc->stride[0] * 2;
	u32SizeU8C1 = pstSrc->height * pstSrc->stride[0];
	stEigenMap.phy_addr[0] =
		pstStCandiCornerCtrl->mem.phy_addr + u32SizeS8C2;
	stEigenMap.type = IVE_IMAGE_TYPE_S8C2_PACKAGE;
	stEigenMap.width = pstSrc->width;
	stEigenMap.height = pstSrc->height;
	stEigenMap.stride[0] = pstSrc->stride[0] * 2;



	for(i = 0; i < 2; i++){
		hw_stbox_flt_and_eig_calc(ndev, pstSrc, &stEigenMap, mask,
				   &max_eig, instant, ive_top_c,
				   &ive_filterop_c, &img_in_c, &wdma_y_ctl_c, dev_id);
	}

	g_u32SizeS8C2 = u32SizeS8C2;
	g_u16MaxEig = max_eig;
	g_pStCandiCornerCtrl = pstStCandiCornerCtrl;
	ret = _hw_stcandi_corner(ndev, &stEigenMap, pstCandiCorner,
					max_eig, pstStCandiCornerCtrl, instant,
					ive_top_c, &ive_filterop_c, &img_in_c,
					&wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	// if (instant) {
	// 	ret = copy_to_user((void __user *)(unsigned long)
	// 			pstStCandiCornerCtrl->mem.vir_addr + u32SizeS8C2 * 2,
	// 			&max_eig, sizeof(u16));
	// }
	kfree(ive_top_c);
	return ret;
}

void stcandicorner_workaround(struct ive_device *ndev)
{
	u32 u32Len, i;
	ive_image_s pstSrc, pstDst;
	ive_st_candi_corner_ctrl_s stStCandiCornerCtrl;

	pstSrc.type = IVE_IMAGE_TYPE_U8C1;
	pstSrc.stride[0] = 64;
	pstSrc.width = 64;
	pstSrc.height = 64;
	pstDst.type = IVE_IMAGE_TYPE_U8C1;
	pstDst.stride[0] = 64;
	pstDst.width = 64;
	pstDst.height = 64;
	u32Len = pstSrc.stride[0] * 64;
	base_ion_alloc(&pstSrc.phy_addr[0], (void **)&pstSrc.vir_addr[0],
		"st_src_ive_mesh", u32Len, false);
	base_ion_alloc(&pstDst.phy_addr[0], (void **)&pstDst.vir_addr[0],
		"st_dst_ive_mesh", u32Len, false);
	stStCandiCornerCtrl.quality_level = 25;
	base_ion_alloc((u64 *)&stStCandiCornerCtrl.mem.phy_addr,
		(void **)&stStCandiCornerCtrl.mem.vir_addr,
		"st_ive_mesh", 4 * pstSrc.height * pstSrc.stride[0] +
		sizeof(ive_st_max_eig_s), false);

	for (i = 0; i < IVE_DEV_MAX; i++) {
		ive_stcandi_corner(ndev, &pstSrc,
			&pstDst, &stStCandiCornerCtrl, 0, i);
	}
	base_ion_free(pstSrc.phy_addr[0]);
	base_ion_free(pstDst.phy_addr[0]);
	base_ion_free(stStCandiCornerCtrl.mem.phy_addr);
}

irqreturn_t platform_ive_irq(struct ive_device *ndev, int dev_id)
{
	//pr_err("[IVE] got %s callback\n", __func__);
	complete(&ndev->core[dev_id].frame_done);
	stop_vld_time(ndev->cur_optype, ndev->core[dev_id].tile_num, &ndev->core[dev_id]);
	return IRQ_HANDLED;
}

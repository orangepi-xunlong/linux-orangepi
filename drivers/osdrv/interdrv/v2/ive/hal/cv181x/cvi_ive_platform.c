/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: cvi_ive_platform.c
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
#include "cvi_ive_platform.h"
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

IVE_DST_MEM_INFO_S *g_pDst;
u16 g_u16MaxEig;
u32 g_u32SizeS8C2;
IVE_ST_CANDI_CORNER_CTRL_S *g_pStCandiCornerCtrl;
uintptr_t g_phy_shift[IVE_DEV_MAX];
bool g_dump_reg_info;
bool g_dump_image_info;
bool g_dump_dma_info;

s32 d_num = 0;  //only for debug device num

IVE_TOP_C ive_top_c_l = _DEFINE_IVE_TOP_C;
IVE_FILTEROP_C ive_filterop_c_l = _DEFINE_IVE_FILTEROP_C;

static IVE_TOP_C *init_ive_top_c(void)
{
	IVE_TOP_C *top_c = kzalloc(sizeof(IVE_TOP_C), GFP_ATOMIC);

	if (top_c == NULL) {
		//pr_err("ive_top_c init failed\n");
		return NULL;
	}

	memcpy(top_c, &ive_top_c_l, sizeof(IVE_TOP_C));

	return top_c;
}

static IVE_FILTEROP_C *init_ive_filterop_c(void)
{
	IVE_FILTEROP_C *filterop_c = kzalloc(sizeof(IVE_FILTEROP_C), GFP_ATOMIC);

	if (filterop_c == NULL) {
		//pr_err("ive_filterop_c init failed\n");
		return NULL;
	}

	memcpy(filterop_c, &ive_filterop_c_l, sizeof(IVE_FILTEROP_C));

	return filterop_c;

}

// static void ive_dma_printk(IVE_DMA_C *p)
// {
// 	pr_info("ive_dma\n");
// 	pr_info("\tREG_0.reg_ive_dma_enable = 0x%x\n", p->REG_0.reg_ive_dma_enable);
// 	pr_info("\tREG_0.reg_shdw_sel = 0x%x\n", p->REG_0.reg_shdw_sel);
// 	pr_info("\tREG_0.reg_softrst = 0x%x\n", p->REG_0.reg_softrst);
// 	pr_info("\tREG_0.reg_ive_dma_mode = 0x%x\n", p->REG_0.reg_ive_dma_mode);
// 	pr_info("\tREG_0.reg_force_clk_enable = 0x%x\n", p->REG_0.reg_force_clk_enable);
// 	pr_info("\tREG_0.reg_force_rdma_disable = 0x%x\n", p->REG_0.reg_force_rdma_disable);
// 	pr_info("\tREG_0.reg_force_wdma_disable = 0x%x\n", p->REG_0.reg_force_wdma_disable);
// 	pr_info("\tREG_1.reg_ive_dma_src_stride = 0x%x\n", p->REG_1.reg_ive_dma_src_stride);
// 	pr_info("\tREG_1.reg_ive_dma_dst_stride = 0x%x\n", p->REG_1.reg_ive_dma_dst_stride);
// 	pr_info("\tREG_2.reg_ive_dma_src_mem_addr = 0x%x\n", p->REG_2.reg_ive_dma_src_mem_addr);
// 	pr_info("\tREG_3.reg_ive_dma_dst_mem_addr = 0x%x\n", p->REG_3.reg_ive_dma_dst_mem_addr);
// 	pr_info("\tREG_4.reg_ive_dma_horsegsize = 0x%x\n", p->REG_4.reg_ive_dma_horsegsize);
// 	pr_info("\tREG_4.reg_ive_dma_elemsize = 0x%x\n", p->REG_4.reg_ive_dma_elemsize);
// 	pr_info("\tREG_4.reg_ive_dma_versegrow = 0x%x\n", p->REG_4.reg_ive_dma_versegrow);
// 	pr_info("\tREG_5.reg_ive_dma_u64_val[0] = 0x%x\n", p->REG_5.reg_ive_dma_u64_val[0]);
// 	pr_info("\tREG_5.reg_ive_dma_u64_val[1] = 0x%x\n", p->REG_5.reg_ive_dma_u64_val[1]);
// }

static void ive_filterop_printk(IVE_FILTEROP_C *p)
{
	pr_info("ive_filterop\n");
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->REG_1.reg_softrst);
	pr_info("\tREG_1.reg_softrst_wdma_y = 0x%x\n", p->REG_1.reg_softrst_wdma_y);
	pr_info("\tREG_1.reg_softrst_wdma_c = 0x%x\n", p->REG_1.reg_softrst_wdma_c);
	pr_info("\tREG_1.reg_softrst_rdma_gradfg = 0x%x\n", p->REG_1.reg_softrst_rdma_gradfg);
	pr_info("\tREG_1.reg_softrst_op1 = 0x%x\n", p->REG_1.reg_softrst_op1);
	pr_info("\tREG_1.reg_softrst_filter3ch = 0x%x\n", p->REG_1.reg_softrst_filter3ch);
	pr_info("\tREG_1.reg_softrst_st = 0x%x\n", p->REG_1.reg_softrst_st);
	pr_info("\tREG_1.reg_softrst_odma = 0x%x\n", p->REG_1.reg_softrst_odma);
	pr_info("\tREG_H04.reg_gradfg_bggrad_rdma_en = 0x%x\n", p->REG_H04.reg_gradfg_bggrad_rdma_en);
	pr_info("\tREG_H04.reg_gradfg_bggrad_uv_swap = 0x%x\n", p->REG_H04.reg_gradfg_bggrad_uv_swap);
	pr_info("\tREG_2.reg_shdw_sel = 0x%x\n", p->REG_2.reg_shdw_sel);
	pr_info("\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->REG_3.reg_ctrl_dmy1);
	pr_info("\tREG_h10.reg_filterop_mode = 0x%x\n", p->REG_h10.reg_filterop_mode);
	pr_info("\tREG_h14.reg_filterop_op1_cmd = 0x%x\n", p->REG_h14.reg_filterop_op1_cmd);
	pr_info("\tREG_h14.reg_filterop_sw_ovw_op = 0x%x\n", p->REG_h14.reg_filterop_sw_ovw_op);
	pr_info("\tREG_h14.reg_filterop_3ch_en = 0x%x\n", p->REG_h14.reg_filterop_3ch_en);
	pr_info("\tREG_h14.reg_op_y_wdma_en = 0x%x\n", p->REG_h14.reg_op_y_wdma_en);
	pr_info("\tREG_h14.reg_op_c_wdma_en = 0x%x\n", p->REG_h14.reg_op_c_wdma_en);
	pr_info("\tREG_h14.reg_op_y_wdma_w1b_en = 0x%x\n", p->REG_h14.reg_op_y_wdma_w1b_en);
	pr_info("\tREG_h14.reg_op_c_wdma_w1b_en = 0x%x\n", p->REG_h14.reg_op_c_wdma_w1b_en);
	pr_info("\tREG_4.reg_filterop_h_coef00 = 0x%x\n", p->REG_4.reg_filterop_h_coef00);
	pr_info("\tREG_4.reg_filterop_h_coef01 = 0x%x\n", p->REG_4.reg_filterop_h_coef01);
	pr_info("\tREG_4.reg_filterop_h_coef02 = 0x%x\n", p->REG_4.reg_filterop_h_coef02);
	pr_info("\tREG_4.reg_filterop_h_coef03 = 0x%x\n", p->REG_4.reg_filterop_h_coef03);
	pr_info("\tREG_5.reg_filterop_h_coef04 = 0x%x\n", p->REG_5.reg_filterop_h_coef04);
	pr_info("\tREG_5.reg_filterop_h_coef10 = 0x%x\n", p->REG_5.reg_filterop_h_coef10);
	pr_info("\tREG_5.reg_filterop_h_coef11 = 0x%x\n", p->REG_5.reg_filterop_h_coef11);
	pr_info("\tREG_5.reg_filterop_h_coef12 = 0x%x\n", p->REG_5.reg_filterop_h_coef12);
	pr_info("\tREG_6.reg_filterop_h_coef13 = 0x%x\n", p->REG_6.reg_filterop_h_coef13);
	pr_info("\tREG_6.reg_filterop_h_coef14 = 0x%x\n", p->REG_6.reg_filterop_h_coef14);
	pr_info("\tREG_6.reg_filterop_h_coef20 = 0x%x\n", p->REG_6.reg_filterop_h_coef20);
	pr_info("\tREG_6.reg_filterop_h_coef21 = 0x%x\n", p->REG_6.reg_filterop_h_coef21);
	pr_info("\tREG_7.reg_filterop_h_coef22 = 0x%x\n", p->REG_7.reg_filterop_h_coef22);
	pr_info("\tREG_7.reg_filterop_h_coef23 = 0x%x\n", p->REG_7.reg_filterop_h_coef23);
	pr_info("\tREG_7.reg_filterop_h_coef24 = 0x%x\n", p->REG_7.reg_filterop_h_coef24);
	pr_info("\tREG_7.reg_filterop_h_coef30 = 0x%x\n", p->REG_7.reg_filterop_h_coef30);
	pr_info("\tREG_8.reg_filterop_h_coef31 = 0x%x\n", p->REG_8.reg_filterop_h_coef31);
	pr_info("\tREG_8.reg_filterop_h_coef32 = 0x%x\n", p->REG_8.reg_filterop_h_coef32);
	pr_info("\tREG_8.reg_filterop_h_coef33 = 0x%x\n", p->REG_8.reg_filterop_h_coef33);
	pr_info("\tREG_8.reg_filterop_h_coef34 = 0x%x\n", p->REG_8.reg_filterop_h_coef34);
	pr_info("\tREG_9.reg_filterop_h_coef40 = 0x%x\n", p->REG_9.reg_filterop_h_coef40);
	pr_info("\tREG_9.reg_filterop_h_coef41 = 0x%x\n", p->REG_9.reg_filterop_h_coef41);
	pr_info("\tREG_9.reg_filterop_h_coef42 = 0x%x\n", p->REG_9.reg_filterop_h_coef42);
	pr_info("\tREG_9.reg_filterop_h_coef43 = 0x%x\n", p->REG_9.reg_filterop_h_coef43);
	pr_info("\tREG_10.reg_filterop_h_coef44 = 0x%x\n", p->REG_10.reg_filterop_h_coef44);
	pr_info("\tREG_10.reg_filterop_h_norm = 0x%x\n", p->REG_10.reg_filterop_h_norm);
	pr_info("\tREG_11.reg_filterop_v_coef00 = 0x%x\n", p->REG_11.reg_filterop_v_coef00);
	pr_info("\tREG_11.reg_filterop_v_coef01 = 0x%x\n", p->REG_11.reg_filterop_v_coef01);
	pr_info("\tREG_11.reg_filterop_v_coef02 = 0x%x\n", p->REG_11.reg_filterop_v_coef02);
	pr_info("\tREG_11.reg_filterop_v_coef03 = 0x%x\n", p->REG_11.reg_filterop_v_coef03);
	pr_info("\tREG_12.reg_filterop_v_coef04 = 0x%x\n", p->REG_12.reg_filterop_v_coef04);
	pr_info("\tREG_12.reg_filterop_v_coef10 = 0x%x\n", p->REG_12.reg_filterop_v_coef10);
	pr_info("\tREG_12.reg_filterop_v_coef11 = 0x%x\n", p->REG_12.reg_filterop_v_coef11);
	pr_info("\tREG_12.reg_filterop_v_coef12 = 0x%x\n", p->REG_12.reg_filterop_v_coef12);
	pr_info("\tREG_13.reg_filterop_v_coef13 = 0x%x\n", p->REG_13.reg_filterop_v_coef13);
	pr_info("\tREG_13.reg_filterop_v_coef14 = 0x%x\n", p->REG_13.reg_filterop_v_coef14);
	pr_info("\tREG_13.reg_filterop_v_coef20 = 0x%x\n", p->REG_13.reg_filterop_v_coef20);
	pr_info("\tREG_13.reg_filterop_v_coef21 = 0x%x\n", p->REG_13.reg_filterop_v_coef21);
	pr_info("\tREG_14.reg_filterop_v_coef22 = 0x%x\n", p->REG_14.reg_filterop_v_coef22);
	pr_info("\tREG_14.reg_filterop_v_coef23 = 0x%x\n", p->REG_14.reg_filterop_v_coef23);
	pr_info("\tREG_14.reg_filterop_v_coef24 = 0x%x\n", p->REG_14.reg_filterop_v_coef24);
	pr_info("\tREG_14.reg_filterop_v_coef30 = 0x%x\n", p->REG_14.reg_filterop_v_coef30);
	pr_info("\tREG_15.reg_filterop_v_coef31 = 0x%x\n", p->REG_15.reg_filterop_v_coef31);
	pr_info("\tREG_15.reg_filterop_v_coef32 = 0x%x\n", p->REG_15.reg_filterop_v_coef32);
	pr_info("\tREG_15.reg_filterop_v_coef33 = 0x%x\n", p->REG_15.reg_filterop_v_coef33);
	pr_info("\tREG_15.reg_filterop_v_coef34 = 0x%x\n", p->REG_15.reg_filterop_v_coef34);
	pr_info("\tREG_16.reg_filterop_v_coef40 = 0x%x\n", p->REG_16.reg_filterop_v_coef40);
	pr_info("\tREG_16.reg_filterop_v_coef41 = 0x%x\n", p->REG_16.reg_filterop_v_coef41);
	pr_info("\tREG_16.reg_filterop_v_coef42 = 0x%x\n", p->REG_16.reg_filterop_v_coef42);
	pr_info("\tREG_16.reg_filterop_v_coef43 = 0x%x\n", p->REG_16.reg_filterop_v_coef43);
	pr_info("\tREG_17.reg_filterop_v_coef44 = 0x%x\n", p->REG_17.reg_filterop_v_coef44);
	pr_info("\tREG_17.reg_filterop_v_norm = 0x%x\n", p->REG_17.reg_filterop_v_norm);
	pr_info("\tREG_18.reg_filterop_mode_trans = 0x%x\n", p->REG_18.reg_filterop_mode_trans);
	pr_info("\tREG_18.reg_filterop_order_enmode = 0x%x\n", p->REG_18.reg_filterop_order_enmode);
	pr_info("\tREG_18.reg_filterop_mag_thr = 0x%x\n", p->REG_18.reg_filterop_mag_thr);
	pr_info("\tREG_19.reg_filterop_bernsen_win5x5_en = 0x%x\n", p->REG_19.reg_filterop_bernsen_win5x5_en);
	pr_info("\tREG_19.reg_filterop_bernsen_mode = 0x%x\n", p->REG_19.reg_filterop_bernsen_mode);
	pr_info("\tREG_19.reg_filterop_bernsen_thr = 0x%x\n", p->REG_19.reg_filterop_bernsen_thr);
	pr_info("\tREG_19.reg_filterop_u8ContrastThreshold = 0x%x\n", p->REG_19.reg_filterop_u8ContrastThreshold);
	pr_info("\tREG_20.reg_filterop_lbp_u8bit_thr = 0x%x\n", p->REG_20.reg_filterop_lbp_u8bit_thr);
	pr_info("\tREG_20.reg_filterop_lbp_s8bit_thr = 0x%x\n", p->REG_20.reg_filterop_lbp_s8bit_thr);
	pr_info("\tREG_20.reg_filterop_lbp_enmode = 0x%x\n", p->REG_20.reg_filterop_lbp_enmode);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef00 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef00);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef01 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef01);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef02 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef02);
	pr_info("\tREG_21.reg_filterop_op2_erodila_coef03 = 0x%x\n", p->REG_21.reg_filterop_op2_erodila_coef03);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef04 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef04);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef10 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef10);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef11 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef11);
	pr_info("\tREG_22.reg_filterop_op2_erodila_coef12 = 0x%x\n", p->REG_22.reg_filterop_op2_erodila_coef12);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef13 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef13);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef14 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef14);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef20 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef20);
	pr_info("\tREG_23.reg_filterop_op2_erodila_coef21 = 0x%x\n", p->REG_23.reg_filterop_op2_erodila_coef21);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef22 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef22);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef23 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef23);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef24 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef24);
	pr_info("\tREG_24.reg_filterop_op2_erodila_coef30 = 0x%x\n", p->REG_24.reg_filterop_op2_erodila_coef30);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef31 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef31);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef32 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef32);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef33 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef33);
	pr_info("\tREG_25.reg_filterop_op2_erodila_coef34 = 0x%x\n", p->REG_25.reg_filterop_op2_erodila_coef34);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef40 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef40);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef41 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef41);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef42 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef42);
	pr_info("\tREG_26.reg_filterop_op2_erodila_coef43 = 0x%x\n", p->REG_26.reg_filterop_op2_erodila_coef43);
	pr_info("\tREG_27.reg_filterop_op2_erodila_coef44 = 0x%x\n", p->REG_27.reg_filterop_op2_erodila_coef44);
	pr_info("\tREG_28.reg_filterop_op2_erodila_en = 0x%x\n", p->REG_28.reg_filterop_op2_erodila_en);
	pr_info("\tREG_CSC_DBG_COEFF.reg_csc_dbg_en = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_en);
	pr_info("\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff_sel);
	pr_info("\tREG_CSC_DBG_COEFF.reg_csc_dbg_coeff = 0x%x\n", p->REG_CSC_DBG_COEFF.reg_csc_dbg_coeff);
	pr_info("\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_x);
	pr_info("\tREG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y = 0x%x\n", p->REG_CSC_DBG_PROB_PIX.reg_csc_dbg_prob_y);
	pr_info("\tREG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data = 0x%x\n", p->REG_CSC_DBG_DATA_SRC.reg_csc_dbg_src_data);
	pr_info("\tREG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data = 0x%x\n", p->REG_CSC_DBG_DATA_DAT.reg_csc_dbg_dst_data);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_en = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_en);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_softrst = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_softrst);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_enmode = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_enmode);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_edwdark = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_edwdark);
	pr_info("\tREG_33.reg_filterop_op2_gradfg_edwfactor = 0x%x\n", p->REG_33.reg_filterop_op2_gradfg_edwfactor);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_crlcoefthr = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_crlcoefthr);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_magcrlthr = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_magcrlthr);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_minmagdiff = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_minmagdiff);
	pr_info("\tREG_34.reg_filterop_op2_gradfg_noiseval = 0x%x\n", p->REG_34.reg_filterop_op2_gradfg_noiseval);
	pr_info("\tREG_110.reg_filterop_map_enmode = 0x%x\n", p->REG_110.reg_filterop_map_enmode);
	pr_info("\tREG_110.reg_filterop_norm_out_ctrl = 0x%x\n", p->REG_110.reg_filterop_norm_out_ctrl);
	pr_info("\tREG_110.reg_filterop_magang_out_ctrl = 0x%x\n", p->REG_110.reg_filterop_magang_out_ctrl);
	pr_info("\tODMA_REG_00.reg_dma_blen = 0x%x\n", p->ODMA_REG_00.reg_dma_blen);
	pr_info("\tODMA_REG_00.reg_dma_en = 0x%x\n", p->ODMA_REG_00.reg_dma_en);
	pr_info("\tODMA_REG_00.reg_fmt_sel = 0x%x\n", p->ODMA_REG_00.reg_fmt_sel);
	pr_info("\tODMA_REG_00.reg_sc_odma_hflip = 0x%x\n", p->ODMA_REG_00.reg_sc_odma_hflip);
	pr_info("\tODMA_REG_00.reg_sc_odma_vflip = 0x%x\n", p->ODMA_REG_00.reg_sc_odma_vflip);
	pr_info("\tODMA_REG_00.reg_sc_422_avg = 0x%x\n", p->ODMA_REG_00.reg_sc_422_avg);
	pr_info("\tODMA_REG_00.reg_sc_420_avg = 0x%x\n", p->ODMA_REG_00.reg_sc_420_avg);
	pr_info("\tODMA_REG_00.reg_c_round_mode = 0x%x\n", p->ODMA_REG_00.reg_c_round_mode);
	pr_info("\tODMA_REG_00.reg_bf16_en = 0x%x\n", p->ODMA_REG_00.reg_bf16_en);
	pr_info("\tODMA_REG_01.reg_dma_y_base_low_part = 0x%x\n", p->ODMA_REG_01.reg_dma_y_base_low_part);
	pr_info("\tODMA_REG_02.reg_dma_y_base_high_part = 0x%x\n", p->ODMA_REG_02.reg_dma_y_base_high_part);
	pr_info("\tODMA_REG_03.reg_dma_u_base_low_part = 0x%x\n", p->ODMA_REG_03.reg_dma_u_base_low_part);
	pr_info("\tODMA_REG_04.reg_dma_u_base_high_part = 0x%x\n", p->ODMA_REG_04.reg_dma_u_base_high_part);
	pr_info("\tODMA_REG_05.reg_dma_v_base_low_part = 0x%x\n", p->ODMA_REG_05.reg_dma_v_base_low_part);
	pr_info("\tODMA_REG_06.reg_dma_v_base_high_part = 0x%x\n", p->ODMA_REG_06.reg_dma_v_base_high_part);
	pr_info("\tODMA_REG_07.reg_dma_y_pitch = 0x%x\n", p->ODMA_REG_07.reg_dma_y_pitch);
	pr_info("\tODMA_REG_08.reg_dma_c_pitch = 0x%x\n", p->ODMA_REG_08.reg_dma_c_pitch);
	pr_info("\tODMA_REG_09.reg_dma_x_str = 0x%x\n", p->ODMA_REG_09.reg_dma_x_str);
	pr_info("\tODMA_REG_10.reg_dma_y_str = 0x%x\n", p->ODMA_REG_10.reg_dma_y_str);
	pr_info("\tODMA_REG_11.reg_dma_wd = 0x%x\n", p->ODMA_REG_11.reg_dma_wd);
	pr_info("\tODMA_REG_12.reg_dma_ht = 0x%x\n", p->ODMA_REG_12.reg_dma_ht);
	pr_info("\tODMA_REG_13.reg_dma_debug = 0x%x\n", p->ODMA_REG_13.reg_dma_debug);
	pr_info("\tODMA_REG_14.reg_dma_int_line_target = 0x%x\n", p->ODMA_REG_14.reg_dma_int_line_target);
	pr_info("\tODMA_REG_14.reg_dma_int_line_target_sel = 0x%x\n", p->ODMA_REG_14.reg_dma_int_line_target_sel);
	pr_info("\tODMA_REG_15.reg_dma_int_cycle_line_target = 0x%x\n", p->ODMA_REG_15.reg_dma_int_cycle_line_target);
	pr_info("\tODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x%x\n",
							p->ODMA_REG_15.reg_dma_int_cycle_line_target_sel);
	pr_info("\tODMA_REG_16.reg_dma_latch_line_cnt = 0x%x\n", p->ODMA_REG_16.reg_dma_latch_line_cnt);
	pr_info("\tODMA_REG_16.reg_dma_latched_line_cnt = 0x%x\n", p->ODMA_REG_16.reg_dma_latched_line_cnt);
	pr_info("\tODMA_REG_17.axi_active = 0x%x\n", p->ODMA_REG_17.axi_active);
	pr_info("\tODMA_REG_17.axi_y_active = 0x%x\n", p->ODMA_REG_17.axi_y_active);
	pr_info("\tODMA_REG_17.axi_u_active = 0x%x\n", p->ODMA_REG_17.axi_u_active);
	pr_info("\tODMA_REG_17.axi_v_active = 0x%x\n", p->ODMA_REG_17.axi_v_active);
	pr_info("\tODMA_REG_17.y_buf_full = 0x%x\n", p->ODMA_REG_17.y_buf_full);
	pr_info("\tODMA_REG_17.y_buf_empty = 0x%x\n", p->ODMA_REG_17.y_buf_empty);
	pr_info("\tODMA_REG_17.u_buf_full = 0x%x\n", p->ODMA_REG_17.u_buf_full);
	pr_info("\tODMA_REG_17.u_buf_empty = 0x%x\n", p->ODMA_REG_17.u_buf_empty);
	pr_info("\tODMA_REG_17.v_buf_full = 0x%x\n", p->ODMA_REG_17.v_buf_full);
	pr_info("\tODMA_REG_17.v_buf_empty = 0x%x\n", p->ODMA_REG_17.v_buf_empty);
	pr_info("\tODMA_REG_17.line_target_hit = 0x%x\n", p->ODMA_REG_17.line_target_hit);
	pr_info("\tODMA_REG_17.cycle_line_target_hit = 0x%x\n", p->ODMA_REG_17.cycle_line_target_hit);
	pr_info("\tODMA_REG_17.axi_cmd_cs = 0x%x\n", p->ODMA_REG_17.axi_cmd_cs);
	pr_info("\tODMA_REG_17.y_line_cnt = 0x%x\n", p->ODMA_REG_17.y_line_cnt);
	pr_info("\tREG_CANNY_0.reg_canny_lowthr = 0x%x\n", p->REG_CANNY_0.reg_canny_lowthr);
	pr_info("\tREG_CANNY_0.reg_canny_hithr = 0x%x\n", p->REG_CANNY_0.reg_canny_hithr);
	pr_info("\tREG_CANNY_1.reg_canny_en = 0x%x\n", p->REG_CANNY_1.reg_canny_en);
	pr_info("\tREG_CANNY_1.reg_canny_strong_point_cnt_en = 0x%x\n", p->REG_CANNY_1.reg_canny_strong_point_cnt_en);
	pr_info("\tREG_CANNY_1.reg_canny_non_or_weak = 0x%x\n", p->REG_CANNY_1.reg_canny_non_or_weak);
	pr_info("\tREG_CANNY_1.reg_canny_strong_point_cnt = 0x%x\n", p->REG_CANNY_1.reg_canny_strong_point_cnt);
	pr_info("\tREG_CANNY_2.reg_canny_eof = 0x%x\n", p->REG_CANNY_2.reg_canny_eof);
	pr_info("\tREG_CANNY_3.reg_canny_basel = 0x%x\n", p->REG_CANNY_3.reg_canny_basel);
	pr_info("\tREG_CANNY_4.reg_canny_baseh = 0x%x\n", p->REG_CANNY_4.reg_canny_baseh);
	pr_info("\tREG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x%x\n", p->REG_ST_CANDI_0.reg_st_candi_corner_bypass);
	pr_info("\tREG_ST_CANDI_0.reg_st_candi_corner_switch_src = 0x%x\n",
							p->REG_ST_CANDI_0.reg_st_candi_corner_switch_src);
	pr_info("\tREG_ST_EIGVAL_0.reg_st_eigval_max_eigval = 0x%x\n", p->REG_ST_EIGVAL_0.reg_st_eigval_max_eigval);
	pr_info("\tREG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0x%x\n", p->REG_ST_EIGVAL_0.reg_st_eigval_tile_num);
	pr_info("\tREG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 0x%x\n", p->REG_ST_EIGVAL_1.reg_sw_clr_max_eigval);
	pr_info("\tREG_h190.reg_filterop_op2_csc_tab_sw_0 = 0x%x\n", p->REG_h190.reg_filterop_op2_csc_tab_sw_0);
	pr_info("\tREG_h190.reg_filterop_op2_csc_tab_sw_1 = 0x%x\n", p->REG_h190.reg_filterop_op2_csc_tab_sw_1);
	pr_info("\tREG_h194.reg_filterop_op2_csc_tab_sw_update = 0x%x\n",
								p->REG_h194.reg_filterop_op2_csc_tab_sw_update);
	pr_info("\tREG_h194.reg_filterop_op2_csc_coeff_sw_update = 0x%x\n",
								p->REG_h194.reg_filterop_op2_csc_coeff_sw_update);
	pr_info("\tREG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00 = 0x%x\n",
								p->REG_CSC_COEFF_0.reg_filterop_op2_csc_coeff_sw_00);
	pr_info("\tREG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01 = 0x%x\n",
								p->REG_CSC_COEFF_1.reg_filterop_op2_csc_coeff_sw_01);
	pr_info("\tREG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02 = 0x%x\n",
								p->REG_CSC_COEFF_2.reg_filterop_op2_csc_coeff_sw_02);
	pr_info("\tREG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03 = 0x%x\n",
								p->REG_CSC_COEFF_3.reg_filterop_op2_csc_coeff_sw_03);
	pr_info("\tREG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04 = 0x%x\n",
								p->REG_CSC_COEFF_4.reg_filterop_op2_csc_coeff_sw_04);
	pr_info("\tREG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05 = 0x%x\n",
								p->REG_CSC_COEFF_5.reg_filterop_op2_csc_coeff_sw_05);
	pr_info("\tREG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06 = 0x%x\n",
								p->REG_CSC_COEFF_6.reg_filterop_op2_csc_coeff_sw_06);
	pr_info("\tREG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07 = 0x%x\n",
								p->REG_CSC_COEFF_7.reg_filterop_op2_csc_coeff_sw_07);
	pr_info("\tREG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08 = 0x%x\n",
								p->REG_CSC_COEFF_8.reg_filterop_op2_csc_coeff_sw_08);
	pr_info("\tREG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09 = 0x%x\n",
								p->REG_CSC_COEFF_9.reg_filterop_op2_csc_coeff_sw_09);
	pr_info("\tREG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10 = 0x%x\n",
								p->REG_CSC_COEFF_A.reg_filterop_op2_csc_coeff_sw_10);
	pr_info("\tREG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11 = 0x%x\n",
								p->REG_CSC_COEFF_B.reg_filterop_op2_csc_coeff_sw_11);
	pr_info("\tREG_h1c8.reg_filterop_op2_csc_enmode = 0x%x\n", p->REG_h1c8.reg_filterop_op2_csc_enmode);
	pr_info("\tREG_h1c8.reg_filterop_op2_csc_enable = 0x%x\n", p->REG_h1c8.reg_filterop_op2_csc_enable);
	pr_info("\tREG_cropy_s.reg_crop_y_start_x = 0x%x\n", p->REG_cropy_s.reg_crop_y_start_x);
	pr_info("\tREG_cropy_s.reg_crop_y_end_x = 0x%x\n", p->REG_cropy_s.reg_crop_y_end_x);
	pr_info("\tREG_cropy_e.reg_crop_y_start_y = 0x%x\n", p->REG_cropy_e.reg_crop_y_start_y);
	pr_info("\tREG_cropy_e.reg_crop_y_end_y = 0x%x\n", p->REG_cropy_e.reg_crop_y_end_y);
	pr_info("\tREG_cropy_ctl.reg_crop_y_enable = 0x%x\n", p->REG_cropy_ctl.reg_crop_y_enable);
	pr_info("\tREG_cropc_s.reg_crop_c_start_x = 0x%x\n", p->REG_cropc_s.reg_crop_c_start_x);
	pr_info("\tREG_cropc_s.reg_crop_c_end_x = 0x%x\n", p->REG_cropc_s.reg_crop_c_end_x);
	pr_info("\tREG_cropc_e.reg_crop_c_start_y = 0x%x\n", p->REG_cropc_e.reg_crop_c_start_y);
	pr_info("\tREG_cropc_e.reg_crop_c_end_y = 0x%x\n", p->REG_cropc_e.reg_crop_c_end_y);
	pr_info("\tREG_cropc_ctl.reg_crop_c_enable = 0x%x\n", p->REG_cropc_ctl.reg_crop_c_enable);
	pr_info("\tREG_crop_odma_s.reg_crop_odma_start_x = 0x%x\n", p->REG_crop_odma_s.reg_crop_odma_start_x);
	pr_info("\tREG_crop_odma_s.reg_crop_odma_end_x = 0x%x\n", p->REG_crop_odma_s.reg_crop_odma_end_x);
	pr_info("\tREG_crop_odma_e.reg_crop_odma_start_y = 0x%x\n", p->REG_crop_odma_e.reg_crop_odma_start_y);
	pr_info("\tREG_crop_odma_e.reg_crop_odma_end_y = 0x%x\n", p->REG_crop_odma_e.reg_crop_odma_end_y);
	pr_info("\tREG_crop_odma_ctl.reg_crop_odma_enable = 0x%x\n", p->REG_crop_odma_ctl.reg_crop_odma_enable);
}

static void ive_gmm_printk(IVE_GMM_C *p)
{
	pr_info("ive_gmm\n");
	pr_info("\tREG_GMM_0.reg_gmm_learn_rate = 0x%x\n",
		p->REG_GMM_0.reg_gmm_learn_rate);
	pr_info("\tREG_GMM_0.reg_gmm_bg_ratio = 0x%x\n",
		p->REG_GMM_0.reg_gmm_bg_ratio);
	pr_info("\tREG_GMM_1.reg_gmm_var_thr = 0x%x\n",
		p->REG_GMM_1.reg_gmm_var_thr);
	pr_info("\tREG_GMM_2.reg_gmm_noise_var = 0x%x\n",
		p->REG_GMM_2.reg_gmm_noise_var);
	pr_info("\tREG_GMM_3.reg_gmm_max_var = 0x%x\n",
		p->REG_GMM_3.reg_gmm_max_var);
	pr_info("\tREG_GMM_4.reg_gmm_min_var = 0x%x\n",
		p->REG_GMM_4.reg_gmm_min_var);
	pr_info("\tREG_GMM_5.reg_gmm_init_weight = 0x%x\n",
		p->REG_GMM_5.reg_gmm_init_weight);
	pr_info("\tREG_GMM_6.reg_gmm_detect_shadow = 0x%x\n",
		p->REG_GMM_6.reg_gmm_detect_shadow);
	pr_info("\tREG_GMM_6.reg_gmm_shadow_thr = 0x%x\n",
		p->REG_GMM_6.reg_gmm_shadow_thr);
	pr_info("\tREG_GMM_6.reg_gmm_sns_factor = 0x%x\n",
		p->REG_GMM_6.reg_gmm_sns_factor);
	pr_info("\tREG_GMM_7.reg_gmm2_life_update_factor = 0x%x\n",
		p->REG_GMM_7.reg_gmm2_life_update_factor);
	pr_info("\tREG_GMM_7.reg_gmm2_var_rate = 0x%x\n",
		p->REG_GMM_7.reg_gmm2_var_rate);
	pr_info("\tREG_GMM_8.reg_gmm2_freq_redu_factor = 0x%x\n",
		p->REG_GMM_8.reg_gmm2_freq_redu_factor);
	pr_info("\tREG_GMM_8.reg_gmm2_max_var = 0x%x\n",
		p->REG_GMM_8.reg_gmm2_max_var);
	pr_info("\tREG_GMM_9.reg_gmm2_min_var = 0x%x\n",
		p->REG_GMM_9.reg_gmm2_min_var);
	pr_info("\tREG_GMM_9.reg_gmm2_freq_add_factor = 0x%x\n",
		p->REG_GMM_9.reg_gmm2_freq_add_factor);
	pr_info("\tREG_GMM_10.reg_gmm2_freq_init = 0x%x\n",
		p->REG_GMM_10.reg_gmm2_freq_init);
	pr_info("\tREG_GMM_10.reg_gmm2_freq_thr = 0x%x\n",
		p->REG_GMM_10.reg_gmm2_freq_thr);
	pr_info("\tREG_GMM_11.reg_gmm2_life_thr = 0x%x\n",
		p->REG_GMM_11.reg_gmm2_life_thr);
	pr_info("\tREG_GMM_11.reg_gmm2_sns_factor = 0x%x\n",
		p->REG_GMM_11.reg_gmm2_sns_factor);
	pr_info("\tREG_GMM_12.reg_gmm2_factor = 0x%x\n",
		p->REG_GMM_12.reg_gmm2_factor);
	pr_info("\tREG_GMM_12.reg_gmm2_life_update_factor_mode = 0x%x\n",
		p->REG_GMM_12.reg_gmm2_life_update_factor_mode);
	pr_info("\tREG_GMM_12.reg_gmm2_sns_factor_mode = 0x%x\n",
		p->REG_GMM_12.reg_gmm2_sns_factor_mode);
	pr_info("\tREG_GMM_13.reg_gmm_gmm2_enable = 0x%x\n",
		p->REG_GMM_13.reg_gmm_gmm2_enable);
	pr_info("\tREG_GMM_13.reg_gmm_gmm2_yonly = 0x%x\n",
		p->REG_GMM_13.reg_gmm_gmm2_yonly);
	pr_info("\tREG_GMM_13.reg_gmm_gmm2_shdw_sel = 0x%x\n",
		p->REG_GMM_13.reg_gmm_gmm2_shdw_sel);
	pr_info("\tREG_GMM_13.reg_force_clk_enable = 0x%x\n",
		p->REG_GMM_13.reg_force_clk_enable);
	pr_info("\tREG_GMM_13.reg_gmm_gmm2_model_num = 0x%x\n",
		p->REG_GMM_13.reg_gmm_gmm2_model_num);
	pr_info("\tREG_GMM_13.reg_prob_model_sel = 0x%x\n",
		p->REG_GMM_13.reg_prob_model_sel);
	pr_info("\tREG_GMM_13.reg_prob_byte_sel = 0x%x\n",
		p->REG_GMM_13.reg_prob_byte_sel);
	pr_info("\tREG_GMM_13.reg_prob_bg_sel = 0x%x\n",
		p->REG_GMM_13.reg_prob_bg_sel);
	pr_info("\tREG_GMM_13.reg_prob_en = 0x%x\n", p->REG_GMM_13.reg_prob_en);
	pr_info("\tREG_GMM_14.reg_prob_line = 0x%x\n",
		p->REG_GMM_14.reg_prob_line);
	pr_info("\tREG_GMM_14.reg_prob_pix = 0x%x\n",
		p->REG_GMM_14.reg_prob_pix);
	pr_info("\tREG_GMM_14.reg_prob_byte_data = 0x%x\n",
		p->REG_GMM_14.reg_prob_byte_data);
}

// static void ive_map_printk(IVE_MAP_C *p)
// {
// 	pr_info("ive_map\n");
// 	pr_info("\tREG_0.reg_softrst = 0x%x\n", p->REG_0.reg_softrst);
// 	pr_info("\tREG_0.reg_ip_enable = 0x%x\n", p->REG_0.reg_ip_enable);
// 	pr_info("\tREG_0.reg_ck_enable = 0x%x\n", p->REG_0.reg_ck_enable);
// 	pr_info("\tREG_0.reg_shdw_sel = 0x%x\n", p->REG_0.reg_shdw_sel);
// 	pr_info("\tREG_0.reg_prog_hdk_dis = 0x%x\n", p->REG_0.reg_prog_hdk_dis);
// 	pr_info("\tREG_1.reg_lut_prog_en = 0x%x\n", p->REG_1.reg_lut_prog_en);
// 	pr_info("\tREG_1.reg_lut_wsel = 0x%x\n", p->REG_1.reg_lut_wsel);
// 	pr_info("\tREG_1.reg_lut_rsel = 0x%x\n", p->REG_1.reg_lut_rsel);
// 	pr_info("\tREG_1.reg_sw_lut_rsel = 0x%x\n", p->REG_1.reg_sw_lut_rsel);
// 	pr_info("\tREG_2.reg_lut_st_addr = 0x%x\n", p->REG_2.reg_lut_st_addr);
// 	pr_info("\tREG_2.reg_lut_st_w1t = 0x%x\n", p->REG_2.reg_lut_st_w1t);
// 	pr_info("\tREG_3.reg_lut_wdata = 0x%x\n", p->REG_3.reg_lut_wdata);
// 	pr_info("\tREG_3.reg_lut_w1t = 0x%x\n", p->REG_3.reg_lut_w1t);
// 	pr_info("\tREG_4.reg_sw_lut_raddr = 0x%x\n", p->REG_4.reg_sw_lut_raddr);
// 	pr_info("\tREG_4.reg_sw_lut_r_w1t = 0x%x\n", p->REG_4.reg_sw_lut_r_w1t);
// 	pr_info("\tREG_5.reg_sw_lut_rdata = 0x%x\n", p->REG_5.reg_sw_lut_rdata);
// }

static void ive_match_bg_printk(IVE_MATCH_BG_C *p)
{
	pr_info("ive_match_bg\n");
	pr_info("\tREG_00.reg_matchbg_en = 0x%x\n", p->REG_00.reg_matchbg_en);
	pr_info("\tREG_00.reg_matchbg_bypass_model = 0x%x\n",
		p->REG_00.reg_matchbg_bypass_model);
	pr_info("\tREG_00.reg_matchbg_softrst = 0x%x\n",
		p->REG_00.reg_matchbg_softrst);
	pr_info("\tREG_04.reg_matchbg_curfrmnum = 0x%x\n",
		p->REG_04.reg_matchbg_curfrmnum);
	pr_info("\tREG_08.reg_matchbg_prefrmnum = 0x%x\n",
		p->REG_08.reg_matchbg_prefrmnum);
	pr_info("\tREG_0C.reg_matchbg_timethr = 0x%x\n",
		p->REG_0C.reg_matchbg_timethr);
	pr_info("\tREG_0C.reg_matchbg_diffthrcrlcoef = 0x%x\n",
		p->REG_0C.reg_matchbg_diffthrcrlcoef);
	pr_info("\tREG_0C.reg_matchbg_diffmaxthr = 0x%x\n",
		p->REG_0C.reg_matchbg_diffmaxthr);
	pr_info("\tREG_0C.reg_matchbg_diffminthr = 0x%x\n",
		p->REG_0C.reg_matchbg_diffminthr);
	pr_info("\tREG_0C.reg_matchbg_diffthrinc = 0x%x\n",
		p->REG_0C.reg_matchbg_diffthrinc);
	pr_info("\tREG_0C.reg_matchbg_fastlearnrate = 0x%x\n",
		p->REG_0C.reg_matchbg_fastlearnrate);
	pr_info("\tREG_0C.reg_matchbg_detchgregion = 0x%x\n",
		p->REG_0C.reg_matchbg_detchgregion);
	pr_info("\tREG_10.reg_matchbg_stat_pixnum = 0x%x\n",
		p->REG_10.reg_matchbg_stat_pixnum);
	pr_info("\tREG_14.reg_matchbg_stat_sumlum = 0x%x\n",
		p->REG_14.reg_matchbg_stat_sumlum);
}

// static void ive_sad_printk(IVE_SAD_C *p)
// {
// 	pr_info("ive_sad\n");
// 	pr_info("\tREG_SAD_00.reg_sad_enmode = 0x%x\n", p->REG_SAD_00.reg_sad_enmode);
// 	pr_info("\tREG_SAD_00.reg_sad_out_ctrl = 0x%x\n", p->REG_SAD_00.reg_sad_out_ctrl);
// 	pr_info("\tREG_SAD_00.reg_sad_u16bit_thr = 0x%x\n", p->REG_SAD_00.reg_sad_u16bit_thr);
// 	pr_info("\tREG_SAD_00.reg_sad_shdw_sel = 0x%x\n", p->REG_SAD_00.reg_sad_shdw_sel);
// 	pr_info("\tREG_SAD_01.reg_sad_u8bit_max = 0x%x\n", p->REG_SAD_01.reg_sad_u8bit_max);
// 	pr_info("\tREG_SAD_01.reg_sad_u8bit_min = 0x%x\n", p->REG_SAD_01.reg_sad_u8bit_min);
// 	pr_info("\tREG_SAD_02.reg_sad_enable = 0x%x\n", p->REG_SAD_02.reg_sad_enable);
// 	pr_info("\tREG_SAD_03.reg_force_clk_enable = 0x%x\n", p->REG_SAD_03.reg_force_clk_enable);
// 	pr_info("\tREG_SAD_04.reg_prob_grid_v = 0x%x\n", p->REG_SAD_04.reg_prob_grid_v);
// 	pr_info("\tREG_SAD_04.reg_prob_grid_h = 0x%x\n", p->REG_SAD_04.reg_prob_grid_h);
// 	pr_info("\tREG_SAD_04.reg_prob_pix_v = 0x%x\n", p->REG_SAD_04.reg_prob_pix_v);
// 	pr_info("\tREG_SAD_04.reg_prob_pix_h = 0x%x\n", p->REG_SAD_04.reg_prob_pix_h);
// 	pr_info("\tREG_SAD_05.reg_prob_prev_sum = 0x%x\n", p->REG_SAD_05.reg_prob_prev_sum);
// 	pr_info("\tREG_SAD_05.reg_prob_curr_pix_0 = 0x%x\n", p->REG_SAD_05.reg_prob_curr_pix_0);
// 	pr_info("\tREG_SAD_05.reg_prob_curr_pix_1 = 0x%x\n", p->REG_SAD_05.reg_prob_curr_pix_1);
// 	pr_info("\tREG_SAD_06.reg_prob_en = 0x%x\n", p->REG_SAD_06.reg_prob_en);
// }

static void ive_top_printk(IVE_TOP_C *p)
{
	pr_info("ive_top\n");
	pr_info("\tREG_0.reg_img_in_uv_swap = 0x%x\n", p->REG_0.reg_img_in_uv_swap);
	pr_info("\tREG_0.reg_img_1_uv_swap = 0x%x\n", p->REG_0.reg_img_1_uv_swap);
	pr_info("\tREG_0.reg_rdma_eigval_uv_swap = 0x%x\n", p->REG_0.reg_rdma_eigval_uv_swap);
	pr_info("\tREG_0.reg_trig_cnt = 0x%x\n", p->REG_0.reg_trig_cnt);
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->REG_1.reg_softrst);
	pr_info("\tREG_1.reg_shdw_sel = 0x%x\n", p->REG_1.reg_shdw_sel);
	pr_info("\tREG_1.reg_fmt_vld_fg = 0x%x\n", p->REG_1.reg_fmt_vld_fg);
	pr_info("\tREG_1.reg_fmt_vld_ccl = 0x%x\n", p->REG_1.reg_fmt_vld_ccl);
	pr_info("\tREG_1.reg_fmt_vld_dmaf = 0x%x\n", p->REG_1.reg_fmt_vld_dmaf);
	pr_info("\tREG_1.reg_fmt_vld_lk = 0x%x\n", p->REG_1.reg_fmt_vld_lk);
	pr_info("\tREG_1.reg_cmdq_tsk_trig = 0x%x\n", p->REG_1.reg_cmdq_tsk_trig);
	pr_info("\tREG_1.reg_cmdq_tsk_sel = 0x%x\n", p->REG_1.reg_cmdq_tsk_sel);
	pr_info("\tREG_1.reg_cmdq_tsk_en = 0x%x\n", p->REG_1.reg_cmdq_tsk_en);
	pr_info("\tREG_1.reg_dma_abort = 0x%x\n", p->REG_1.reg_dma_abort);
	pr_info("\tREG_1.reg_wdma_abort_done = 0x%x\n", p->REG_1.reg_wdma_abort_done);
	pr_info("\tREG_1.reg_rdma_abort_done = 0x%x\n", p->REG_1.reg_rdma_abort_done);
	pr_info("\tREG_1.reg_img_in_axi_idle = 0x%x\n", p->REG_1.reg_img_in_axi_idle);
	pr_info("\tREG_1.reg_odma_axi_idle = 0x%x\n", p->REG_1.reg_odma_axi_idle);
	pr_info("\tREG_2.reg_img_widthm1 = 0x%x\n", p->REG_2.reg_img_widthm1);
	pr_info("\tREG_2.reg_img_heightm1 = 0x%x\n", p->REG_2.reg_img_heightm1);
	pr_info("\tREG_3.reg_imgmux_img0_sel = 0x%x\n", p->REG_3.reg_imgmux_img0_sel);
	pr_info("\tREG_3.reg_mapmux_rdma_sel = 0x%x\n", p->REG_3.reg_mapmux_rdma_sel);
	pr_info("\tREG_3.reg_ive_rdma_img1_en = 0x%x\n", p->REG_3.reg_ive_rdma_img1_en);
	pr_info("\tREG_3.reg_ive_rdma_img1_mod_u8 = 0x%x\n", p->REG_3.reg_ive_rdma_img1_mod_u8);
	pr_info("\tREG_3.reg_ive_rdma_eigval_en = 0x%x\n", p->REG_3.reg_ive_rdma_eigval_en);
	pr_info("\tREG_3.reg_muxsel_gradfg = 0x%x\n", p->REG_3.reg_muxsel_gradfg);
	pr_info("\tREG_3.reg_dma_share_mux_selgmm = 0x%x\n", p->REG_3.reg_dma_share_mux_selgmm);
	pr_info("\tREG_h10.reg_img_in_top_enable = 0x%x\n", p->REG_h10.reg_img_in_top_enable);
	pr_info("\tREG_h10.reg_resize_top_enable = 0x%x\n", p->REG_h10.reg_resize_top_enable);
	pr_info("\tREG_h10.reg_gmm_top_enable = 0x%x\n", p->REG_h10.reg_gmm_top_enable);
	pr_info("\tREG_h10.reg_csc_top_enable = 0x%x\n", p->REG_h10.reg_csc_top_enable);
	pr_info("\tREG_h10.reg_rdma_img1_top_enable = 0x%x\n", p->REG_h10.reg_rdma_img1_top_enable);
	pr_info("\tREG_h10.reg_bgm_top_enable = 0x%x\n", p->REG_h10.reg_bgm_top_enable);
	pr_info("\tREG_h10.reg_bgu_top_enable = 0x%x\n", p->REG_h10.reg_bgu_top_enable);
	pr_info("\tREG_h10.reg_r2y4_top_enable = 0x%x\n", p->REG_h10.reg_r2y4_top_enable);
	pr_info("\tREG_h10.reg_map_top_enable = 0x%x\n", p->REG_h10.reg_map_top_enable);
	pr_info("\tREG_h10.reg_rdma_eigval_top_enable = 0x%x\n", p->REG_h10.reg_rdma_eigval_top_enable);
	pr_info("\tREG_h10.reg_thresh_top_enable = 0x%x\n", p->REG_h10.reg_thresh_top_enable);
	pr_info("\tREG_h10.reg_hist_top_enable = 0x%x\n", p->REG_h10.reg_hist_top_enable);
	pr_info("\tREG_h10.reg_intg_top_enable = 0x%x\n", p->REG_h10.reg_intg_top_enable);
	pr_info("\tREG_h10.reg_ncc_top_enable = 0x%x\n", p->REG_h10.reg_ncc_top_enable);
	pr_info("\tREG_h10.reg_sad_top_enable = 0x%x\n", p->REG_h10.reg_sad_top_enable);
	pr_info("\tREG_h10.reg_filterop_top_enable = 0x%x\n", p->REG_h10.reg_filterop_top_enable);
	pr_info("\tREG_h10.reg_dmaf_top_enable = 0x%x\n", p->REG_h10.reg_dmaf_top_enable);
	pr_info("\tREG_h10.reg_ccl_top_enable = 0x%x\n", p->REG_h10.reg_ccl_top_enable);
	pr_info("\tREG_h10.reg_lk_top_enable = 0x%x\n", p->REG_h10.reg_lk_top_enable);
	pr_info("\tREG_11.reg_csc_tab_sw_0 = 0x%x\n", p->REG_11.reg_csc_tab_sw_0);
	pr_info("\tREG_11.reg_csc_tab_sw_1 = 0x%x\n", p->REG_11.reg_csc_tab_sw_1);
	pr_info("\tREG_12.reg_csc_tab_sw_update = 0x%x\n", p->REG_12.reg_csc_tab_sw_update);
	pr_info("\tREG_12.reg_csc_coeff_sw_update = 0x%x\n", p->REG_12.reg_csc_coeff_sw_update);
	pr_info("\tREG_CSC_COEFF_0.reg_csc_coeff_sw_00 = 0x%x\n", p->REG_CSC_COEFF_0.reg_csc_coeff_sw_00);
	pr_info("\tREG_CSC_COEFF_1.reg_csc_coeff_sw_01 = 0x%x\n", p->REG_CSC_COEFF_1.reg_csc_coeff_sw_01);
	pr_info("\tREG_CSC_COEFF_2.reg_csc_coeff_sw_02 = 0x%x\n", p->REG_CSC_COEFF_2.reg_csc_coeff_sw_02);
	pr_info("\tREG_CSC_COEFF_3.reg_csc_coeff_sw_03 = 0x%x\n", p->REG_CSC_COEFF_3.reg_csc_coeff_sw_03);
	pr_info("\tREG_CSC_COEFF_4.reg_csc_coeff_sw_04 = 0x%x\n", p->REG_CSC_COEFF_4.reg_csc_coeff_sw_04);
	pr_info("\tREG_CSC_COEFF_5.reg_csc_coeff_sw_05 = 0x%x\n", p->REG_CSC_COEFF_5.reg_csc_coeff_sw_05);
	pr_info("\tREG_CSC_COEFF_6.reg_csc_coeff_sw_06 = 0x%x\n", p->REG_CSC_COEFF_6.reg_csc_coeff_sw_06);
	pr_info("\tREG_CSC_COEFF_7.reg_csc_coeff_sw_07 = 0x%x\n", p->REG_CSC_COEFF_7.reg_csc_coeff_sw_07);
	pr_info("\tREG_CSC_COEFF_8.reg_csc_coeff_sw_08 = 0x%x\n", p->REG_CSC_COEFF_8.reg_csc_coeff_sw_08);
	pr_info("\tREG_CSC_COEFF_9.reg_csc_coeff_sw_09 = 0x%x\n", p->REG_CSC_COEFF_9.reg_csc_coeff_sw_09);
	pr_info("\tREG_CSC_COEFF_A.reg_csc_coeff_sw_10 = 0x%x\n", p->REG_CSC_COEFF_A.reg_csc_coeff_sw_10);
	pr_info("\tREG_CSC_COEFF_B.reg_csc_coeff_sw_11 = 0x%x\n", p->REG_CSC_COEFF_B.reg_csc_coeff_sw_11);
	pr_info("\tREG_14.reg_csc_enmode = 0x%x\n", p->REG_14.reg_csc_enmode);
	pr_info("\tREG_14.reg_csc_enable = 0x%x\n", p->REG_14.reg_csc_enable);
	pr_info("\tREG_15.reg_lbp_u8bit_thr = 0x%x\n", p->REG_15.reg_lbp_u8bit_thr);
	pr_info("\tREG_15.reg_lbp_s8bit_thr = 0x%x\n", p->REG_15.reg_lbp_s8bit_thr);
	pr_info("\tREG_15.reg_lbp_enmode = 0x%x\n", p->REG_15.reg_lbp_enmode);
	pr_info("\tREG_h54.reg_ive_dma_idle = 0x%x\n", p->REG_h54.reg_ive_dma_idle);
	pr_info("\tREG_h58.reg_ive_gmm_dma_idle = 0x%x\n", p->REG_h58.reg_ive_gmm_dma_idle);
	pr_info("\tREG_16.reg_dbg_en = 0x%x\n", p->REG_16.reg_dbg_en);
	pr_info("\tREG_16.reg_dbg_sel = 0x%x\n", p->REG_16.reg_dbg_sel);
	pr_info("\tREG_h64.reg_dbg_col = 0x%x\n", p->REG_h64.reg_dbg_col);
	pr_info("\tREG_h64.reg_dbg_row = 0x%x\n", p->REG_h64.reg_dbg_row);
	pr_info("\tREG_h68.reg_dbg_status = 0x%x\n", p->REG_h68.reg_dbg_status);
	pr_info("\tREG_h6c.reg_dbg_pix = 0x%x\n", p->REG_h6c.reg_dbg_pix);
	pr_info("\tREG_h6c.reg_dbg_line = 0x%x\n", p->REG_h6c.reg_dbg_line);
	pr_info("\tREG_h70.reg_dbg_data = 0x%x\n", p->REG_h70.reg_dbg_data);
	pr_info("\tREG_h74.reg_dbg_perfmt = 0x%x\n", p->REG_h74.reg_dbg_perfmt);
	pr_info("\tREG_h74.reg_dbg_fmt = 0x%x\n", p->REG_h74.reg_dbg_fmt);
	pr_info("\tREG_20.reg_frame2op_op_mode = 0x%x\n", p->REG_20.reg_frame2op_op_mode);
	pr_info("\tREG_20.reg_frame2op_sub_mode = 0x%x\n", p->REG_20.reg_frame2op_sub_mode);
	pr_info("\tREG_20.reg_frame2op_sub_change_order = 0x%x\n", p->REG_20.reg_frame2op_sub_change_order);
	pr_info("\tREG_20.reg_frame2op_add_mode_rounding = 0x%x\n", p->REG_20.reg_frame2op_add_mode_rounding);
	pr_info("\tREG_20.reg_frame2op_add_mode_clipping = 0x%x\n", p->REG_20.reg_frame2op_add_mode_clipping);
	pr_info("\tREG_20.reg_frame2op_sub_switch_src = 0x%x\n", p->REG_20.reg_frame2op_sub_switch_src);
	pr_info("\tREG_21.reg_fram2op_x_u0q16 = 0x%x\n", p->REG_21.reg_fram2op_x_u0q16);
	pr_info("\tREG_21.reg_fram2op_y_u0q16 = 0x%x\n", p->REG_21.reg_fram2op_y_u0q16);
	pr_info("\tREG_h80.reg_frame2op_fg_op_mode = 0x%x\n", p->REG_h80.reg_frame2op_fg_op_mode);
	pr_info("\tREG_h80.reg_frame2op_fg_sub_mode = 0x%x\n", p->REG_h80.reg_frame2op_fg_sub_mode);
	pr_info("\tREG_h80.reg_frame2op_fg_sub_change_order = 0x%x\n", p->REG_h80.reg_frame2op_fg_sub_change_order);
	pr_info("\tREG_h80.reg_frame2op_fg_add_mode_rounding = 0x%x\n", p->REG_h80.reg_frame2op_fg_add_mode_rounding);
	pr_info("\tREG_h80.reg_frame2op_fg_add_mode_clipping = 0x%x\n", p->REG_h80.reg_frame2op_fg_add_mode_clipping);
	pr_info("\tREG_h80.reg_frame2op_fg_sub_switch_src = 0x%x\n", p->REG_h80.reg_frame2op_fg_sub_switch_src);
	pr_info("\tREG_84.reg_fram2op_fg_x_u0q16 = 0x%x\n", p->REG_84.reg_fram2op_fg_x_u0q16);
	pr_info("\tREG_84.reg_fram2op_fg_y_u0q16 = 0x%x\n", p->REG_84.reg_fram2op_fg_y_u0q16);
	pr_info("\tREG_90.reg_frame_done_img_in = 0x%x\n", p->REG_90.reg_frame_done_img_in);
	pr_info("\tREG_90.reg_frame_done_rdma_img1 = 0x%x\n", p->REG_90.reg_frame_done_rdma_img1);
	pr_info("\tREG_90.reg_frame_done_rdma_eigval = 0x%x\n", p->REG_90.reg_frame_done_rdma_eigval);
	pr_info("\tREG_90.reg_frame_done_resize = 0x%x\n", p->REG_90.reg_frame_done_resize);
	pr_info("\tREG_90.reg_frame_done_gmm = 0x%x\n", p->REG_90.reg_frame_done_gmm);
	pr_info("\tREG_90.reg_frame_done_csc = 0x%x\n", p->REG_90.reg_frame_done_csc);
	pr_info("\tREG_90.reg_frame_done_hist = 0x%x\n", p->REG_90.reg_frame_done_hist);
	pr_info("\tREG_90.reg_frame_done_intg = 0x%x\n", p->REG_90.reg_frame_done_intg);
	pr_info("\tREG_90.reg_frame_done_sad = 0x%x\n", p->REG_90.reg_frame_done_sad);
	pr_info("\tREG_90.reg_frame_done_ncc = 0x%x\n", p->REG_90.reg_frame_done_ncc);
	pr_info("\tREG_90.reg_frame_done_bgm = 0x%x\n", p->REG_90.reg_frame_done_bgm);
	pr_info("\tREG_90.reg_frame_done_bgu = 0x%x\n", p->REG_90.reg_frame_done_bgu);
	pr_info("\tREG_90.reg_frame_done_r2y4 = 0x%x\n", p->REG_90.reg_frame_done_r2y4);
	pr_info("\tREG_90.reg_frame_done_frame2op_bg = 0x%x\n", p->REG_90.reg_frame_done_frame2op_bg);
	pr_info("\tREG_90.reg_frame_done_frame2op_fg = 0x%x\n", p->REG_90.reg_frame_done_frame2op_fg);
	pr_info("\tREG_90.reg_frame_done_map = 0x%x\n", p->REG_90.reg_frame_done_map);
	pr_info("\tREG_90.reg_frame_done_thresh16ro8 = 0x%x\n", p->REG_90.reg_frame_done_thresh16ro8);
	pr_info("\tREG_90.reg_frame_done_thresh = 0x%x\n", p->REG_90.reg_frame_done_thresh);
	pr_info("\tREG_90.reg_frame_done_filterop_odma = 0x%x\n", p->REG_90.reg_frame_done_filterop_odma);
	pr_info("\tREG_90.reg_frame_done_filterop_wdma_y = 0x%x\n", p->REG_90.reg_frame_done_filterop_wdma_y);
	pr_info("\tREG_90.reg_frame_done_filterop_wdma_c = 0x%x\n", p->REG_90.reg_frame_done_filterop_wdma_c);
	pr_info("\tREG_90.reg_frame_done_dmaf = 0x%x\n", p->REG_90.reg_frame_done_dmaf);
	pr_info("\tREG_90.reg_frame_done_ccl = 0x%x\n", p->REG_90.reg_frame_done_ccl);
	pr_info("\tREG_90.reg_frame_done_lk = 0x%x\n", p->REG_90.reg_frame_done_lk);
	pr_info("\tREG_90.reg_frame_done_filterop_wdma_yc = 0x%x\n", p->REG_90.reg_frame_done_filterop_wdma_yc);
	pr_info("\tREG_94.reg_intr_en_hist = 0x%x\n", p->REG_94.reg_intr_en_hist);
	pr_info("\tREG_94.reg_intr_en_intg = 0x%x\n", p->REG_94.reg_intr_en_intg);
	pr_info("\tREG_94.reg_intr_en_sad = 0x%x\n", p->REG_94.reg_intr_en_sad);
	pr_info("\tREG_94.reg_intr_en_ncc = 0x%x\n", p->REG_94.reg_intr_en_ncc);
	pr_info("\tREG_94.reg_intr_en_filterop_odma = 0x%x\n", p->REG_94.reg_intr_en_filterop_odma);
	pr_info("\tREG_94.reg_intr_en_filterop_wdma_y = 0x%x\n", p->REG_94.reg_intr_en_filterop_wdma_y);
	pr_info("\tREG_94.reg_intr_en_filterop_wdma_c = 0x%x\n", p->REG_94.reg_intr_en_filterop_wdma_c);
	pr_info("\tREG_94.reg_intr_en_dmaf = 0x%x\n", p->REG_94.reg_intr_en_dmaf);
	pr_info("\tREG_94.reg_intr_en_ccl = 0x%x\n", p->REG_94.reg_intr_en_ccl);
	pr_info("\tREG_94.reg_intr_en_lk = 0x%x\n", p->REG_94.reg_intr_en_lk);
	pr_info("\tREG_94.reg_intr_en_filterop_wdma_yc = 0x%x\n", p->REG_94.reg_intr_en_filterop_wdma_yc);
	pr_info("\tREG_98.reg_intr_status_hist = 0x%x\n", p->REG_98.reg_intr_status_hist);
	pr_info("\tREG_98.reg_intr_status_intg = 0x%x\n", p->REG_98.reg_intr_status_intg);
	pr_info("\tREG_98.reg_intr_status_sad = 0x%x\n", p->REG_98.reg_intr_status_sad);
	pr_info("\tREG_98.reg_intr_status_ncc = 0x%x\n", p->REG_98.reg_intr_status_ncc);
	pr_info("\tREG_98.reg_intr_status_filterop_odma = 0x%x\n", p->REG_98.reg_intr_status_filterop_odma);
	pr_info("\tREG_98.reg_intr_status_filterop_wdma_y = 0x%x\n", p->REG_98.reg_intr_status_filterop_wdma_y);
	pr_info("\tREG_98.reg_intr_status_filterop_wdma_c = 0x%x\n", p->REG_98.reg_intr_status_filterop_wdma_c);
	pr_info("\tREG_98.reg_intr_status_dmaf = 0x%x\n", p->REG_98.reg_intr_status_dmaf);
	pr_info("\tREG_98.reg_intr_status_ccl = 0x%x\n", p->REG_98.reg_intr_status_ccl);
	pr_info("\tREG_98.reg_intr_status_lk = 0x%x\n", p->REG_98.reg_intr_status_lk);
	pr_info("\tREG_98.reg_intr_status_filterop_wdma_yc = 0x%x\n", p->REG_98.reg_intr_status_filterop_wdma_yc);
	pr_info("\tREG_RS_SRC_SIZE.reg_resize_src_wd = 0x%x\n", p->REG_RS_SRC_SIZE.reg_resize_src_wd);
	pr_info("\tREG_RS_SRC_SIZE.reg_resize_src_ht = 0x%x\n", p->REG_RS_SRC_SIZE.reg_resize_src_ht);
	pr_info("\tREG_RS_DST_SIZE.reg_resize_dst_wd = 0x%x\n", p->REG_RS_DST_SIZE.reg_resize_dst_wd);
	pr_info("\tREG_RS_DST_SIZE.reg_resize_dst_ht = 0x%x\n", p->REG_RS_DST_SIZE.reg_resize_dst_ht);
	pr_info("\tREG_RS_H_SC.reg_resize_h_sc_fac = 0x%x\n", p->REG_RS_H_SC.reg_resize_h_sc_fac);
	pr_info("\tREG_RS_V_SC.reg_resize_v_sc_fac = 0x%x\n", p->REG_RS_V_SC.reg_resize_v_sc_fac);
	pr_info("\tREG_RS_PH_INI.reg_resize_h_ini_ph = 0x%x\n", p->REG_RS_PH_INI.reg_resize_h_ini_ph);
	pr_info("\tREG_RS_PH_INI.reg_resize_v_ini_ph = 0x%x\n", p->REG_RS_PH_INI.reg_resize_v_ini_ph);
	pr_info("\tREG_RS_NOR.reg_resize_h_nor = 0x%x\n", p->REG_RS_NOR.reg_resize_h_nor);
	pr_info("\tREG_RS_NOR.reg_resize_v_nor = 0x%x\n", p->REG_RS_NOR.reg_resize_v_nor);
	pr_info("\tREG_RS_CTRL.reg_resize_ip_en = 0x%x\n", p->REG_RS_CTRL.reg_resize_ip_en);
	pr_info("\tREG_RS_CTRL.reg_resize_dbg_en = 0x%x\n", p->REG_RS_CTRL.reg_resize_dbg_en);
	pr_info("\tREG_RS_CTRL.reg_resize_area_fast = 0x%x\n", p->REG_RS_CTRL.reg_resize_area_fast);
	pr_info("\tREG_RS_CTRL.reg_resize_blnr_mode = 0x%x\n", p->REG_RS_CTRL.reg_resize_blnr_mode);
	pr_info("\tREG_RS_dBG_H1.reg_resize_sc_dbg_h1 = 0x%x\n", p->REG_RS_dBG_H1.reg_resize_sc_dbg_h1);
	pr_info("\tREG_RS_dBG_H2.reg_resize_sc_dbg_h2 = 0x%x\n", p->REG_RS_dBG_H2.reg_resize_sc_dbg_h2);
	pr_info("\tREG_RS_dBG_V1.reg_resize_sc_dbg_v1 = 0x%x\n", p->REG_RS_dBG_V1.reg_resize_sc_dbg_v1);
	pr_info("\tREG_RS_dBG_V2.reg_resize_sc_dbg_v2 = 0x%x\n", p->REG_RS_dBG_V2.reg_resize_sc_dbg_v2);
	pr_info("\tREG_h130.reg_thresh_top_mod = 0x%x\n", p->REG_h130.reg_thresh_top_mod);
	pr_info("\tREG_h130.reg_thresh_thresh_en = 0x%x\n", p->REG_h130.reg_thresh_thresh_en);
	pr_info("\tREG_h130.reg_thresh_softrst = 0x%x\n", p->REG_h130.reg_thresh_softrst);
	pr_info("\tREG_h134.reg_thresh_16to8_mod = 0x%x\n", p->REG_h134.reg_thresh_16to8_mod);
	pr_info("\tREG_h134.reg_thresh_16to8_s8bias = 0x%x\n", p->REG_h134.reg_thresh_16to8_s8bias);
	pr_info("\tREG_h134.reg_thresh_16to8_u8Num_div_u16Den = 0x%x\n", p->REG_h134.reg_thresh_16to8_u8Num_div_u16Den);
	pr_info("\tREG_h138.reg_thresh_st_16to8_en = 0x%x\n", p->REG_h138.reg_thresh_st_16to8_en);
	pr_info("\tREG_h138.reg_thresh_st_16to8_u8Numerator = 0x%x\n", p->REG_h138.reg_thresh_st_16to8_u8Numerator);
	pr_info("\tREG_h138.reg_thresh_st_16to8_maxeigval = 0x%x\n", p->REG_h138.reg_thresh_st_16to8_maxeigval);
	pr_info("\tREG_h13c.reg_thresh_s16_enmode = 0x%x\n", p->REG_h13c.reg_thresh_s16_enmode);
	pr_info("\tREG_h13c.reg_thresh_s16_u8bit_min = 0x%x\n", p->REG_h13c.reg_thresh_s16_u8bit_min);
	pr_info("\tREG_h13c.reg_thresh_s16_u8bit_mid = 0x%x\n", p->REG_h13c.reg_thresh_s16_u8bit_mid);
	pr_info("\tREG_h13c.reg_thresh_s16_u8bit_max = 0x%x\n", p->REG_h13c.reg_thresh_s16_u8bit_max);
	pr_info("\tREG_h140.reg_thresh_s16_bit_thr_l = 0x%x\n", p->REG_h140.reg_thresh_s16_bit_thr_l);
	pr_info("\tREG_h140.reg_thresh_s16_bit_thr_h = 0x%x\n", p->REG_h140.reg_thresh_s16_bit_thr_h);
	pr_info("\tREG_h144.reg_thresh_u16_enmode = 0x%x\n", p->REG_h144.reg_thresh_u16_enmode);
	pr_info("\tREG_h144.reg_thresh_u16_u8bit_min = 0x%x\n", p->REG_h144.reg_thresh_u16_u8bit_min);
	pr_info("\tREG_h144.reg_thresh_u16_u8bit_mid = 0x%x\n", p->REG_h144.reg_thresh_u16_u8bit_mid);
	pr_info("\tREG_h144.reg_thresh_u16_u8bit_max = 0x%x\n", p->REG_h144.reg_thresh_u16_u8bit_max);
	pr_info("\tREG_h148.reg_thresh_u16_bit_thr_l = 0x%x\n", p->REG_h148.reg_thresh_u16_bit_thr_l);
	pr_info("\tREG_h148.reg_thresh_u16_bit_thr_h = 0x%x\n", p->REG_h148.reg_thresh_u16_bit_thr_h);
	pr_info("\tREG_h14c.reg_thresh_u8bit_thr_l = 0x%x\n", p->REG_h14c.reg_thresh_u8bit_thr_l);
	pr_info("\tREG_h14c.reg_thresh_u8bit_thr_h = 0x%x\n", p->REG_h14c.reg_thresh_u8bit_thr_h);
	pr_info("\tREG_h14c.reg_thresh_enmode = 0x%x\n", p->REG_h14c.reg_thresh_enmode);
	pr_info("\tREG_h150.reg_thresh_u8bit_min = 0x%x\n", p->REG_h150.reg_thresh_u8bit_min);
	pr_info("\tREG_h150.reg_thresh_u8bit_mid = 0x%x\n", p->REG_h150.reg_thresh_u8bit_mid);
	pr_info("\tREG_h150.reg_thresh_u8bit_max = 0x%x\n", p->REG_h150.reg_thresh_u8bit_max);
	pr_info("\tREG_h160.reg_ncc_nemerator_l = 0x%x\n", p->REG_h160.reg_ncc_nemerator_l);
	pr_info("\tREG_h164.reg_ncc_nemerator_m = 0x%x\n", p->REG_h164.reg_ncc_nemerator_m);
	pr_info("\tREG_h168.reg_ncc_quadsum0_l = 0x%x\n", p->REG_h168.reg_ncc_quadsum0_l);
	pr_info("\tREG_h16C.reg_ncc_quadsum0_m = 0x%x\n", p->REG_h16C.reg_ncc_quadsum0_m);
	pr_info("\tREG_h170.reg_ncc_quadsum1_l = 0x%x\n", p->REG_h170.reg_ncc_quadsum1_l);
	pr_info("\tREG_h174.reg_ncc_quadsum1_m = 0x%x\n", p->REG_h174.reg_ncc_quadsum1_m);
	pr_info("\tREG_R2Y4_11.reg_csc_r2y4_tab_sw_0 = 0x%x\n", p->REG_R2Y4_11.reg_csc_r2y4_tab_sw_0);
	pr_info("\tREG_R2Y4_11.reg_csc_r2y4_tab_sw_1 = 0x%x\n", p->REG_R2Y4_11.reg_csc_r2y4_tab_sw_1);
	pr_info("\tREG_R2Y4_12.reg_csc_r2y4_tab_sw_update = 0x%x\n", p->REG_R2Y4_12.reg_csc_r2y4_tab_sw_update);
	pr_info("\tREG_R2Y4_12.reg_csc_r2y4_coeff_sw_update = 0x%x\n", p->REG_R2Y4_12.reg_csc_r2y4_coeff_sw_update);
	pr_info("\tREG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00 = 0x%x\n", p->REG_R2Y4_COEFF_0.reg_csc_r2y4_coeff_sw_00);
	pr_info("\tREG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01 = 0x%x\n", p->REG_R2Y4_COEFF_1.reg_csc_r2y4_coeff_sw_01);
	pr_info("\tREG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02 = 0x%x\n", p->REG_R2Y4_COEFF_2.reg_csc_r2y4_coeff_sw_02);
	pr_info("\tREG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03 = 0x%x\n", p->REG_R2Y4_COEFF_3.reg_csc_r2y4_coeff_sw_03);
	pr_info("\tREG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04 = 0x%x\n", p->REG_R2Y4_COEFF_4.reg_csc_r2y4_coeff_sw_04);
	pr_info("\tREG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05 = 0x%x\n", p->REG_R2Y4_COEFF_5.reg_csc_r2y4_coeff_sw_05);
	pr_info("\tREG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06 = 0x%x\n", p->REG_R2Y4_COEFF_6.reg_csc_r2y4_coeff_sw_06);
	pr_info("\tREG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07 = 0x%x\n", p->REG_R2Y4_COEFF_7.reg_csc_r2y4_coeff_sw_07);
	pr_info("\tREG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08 = 0x%x\n", p->REG_R2Y4_COEFF_8.reg_csc_r2y4_coeff_sw_08);
	pr_info("\tREG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09 = 0x%x\n", p->REG_R2Y4_COEFF_9.reg_csc_r2y4_coeff_sw_09);
	pr_info("\tREG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10 = 0x%x\n", p->REG_R2Y4_COEFF_A.reg_csc_r2y4_coeff_sw_10);
	pr_info("\tREG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11 = 0x%x\n", p->REG_R2Y4_COEFF_B.reg_csc_r2y4_coeff_sw_11);
	pr_info("\tREG_R2Y4_14.reg_csc_r2y4_enmode = 0x%x\n", p->REG_R2Y4_14.reg_csc_r2y4_enmode);
	pr_info("\tREG_R2Y4_14.reg_csc_r2y4_enable = 0x%x\n", p->REG_R2Y4_14.reg_csc_r2y4_enable);
}

static void ive_update_bg_printk(IVE_UPDATE_BG_C *p)
{
	pr_info("ive_update_bg\n");
	pr_info("\tREG_1.reg_softrst = 0x%x\n", p->REG_1.reg_softrst);
	pr_info("\tREG_H04.reg_enable = 0x%x\n", p->REG_H04.reg_enable);
	pr_info("\tREG_H04.reg_ck_en = 0x%x\n", p->REG_H04.reg_ck_en);
	pr_info("\tREG_H04.reg_updatebg_byp_model = 0x%x\n", p->REG_H04.reg_updatebg_byp_model);
	pr_info("\tREG_2.reg_shdw_sel = 0x%x\n", p->REG_2.reg_shdw_sel);
	pr_info("\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->REG_3.reg_ctrl_dmy1);
	pr_info("\tREG_ctrl0.reg_u32CurFrmNum = 0x%x\n", p->REG_ctrl0.reg_u32CurFrmNum);
	pr_info("\tREG_ctrl1.reg_u32PreChkTime = 0x%x\n", p->REG_ctrl1.reg_u32PreChkTime);
	pr_info("\tREG_ctrl2.reg_u32FrmChkPeriod = 0x%x\n", p->REG_ctrl2.reg_u32FrmChkPeriod);
	pr_info("\tREG_ctrl2.reg_u32InitMinTime = 0x%x\n", p->REG_ctrl2.reg_u32InitMinTime);
	pr_info("\tREG_ctrl3.reg_u32StyBgMinBlendTime = 0x%x\n", p->REG_ctrl3.reg_u32StyBgMinBlendTime);
	pr_info("\tREG_ctrl3.reg_u32StyBgMaxBlendTime = 0x%x\n", p->REG_ctrl3.reg_u32StyBgMaxBlendTime);
	pr_info("\tREG_ctrl4.reg_u32DynBgMinBlendTime = 0x%x\n", p->REG_ctrl4.reg_u32DynBgMinBlendTime);
	pr_info("\tREG_ctrl4.reg_u32StaticDetMinTime = 0x%x\n", p->REG_ctrl4.reg_u32StaticDetMinTime);
	pr_info("\tREG_ctrl5.reg_u16FgMaxFadeTime = 0x%x\n", p->REG_ctrl5.reg_u16FgMaxFadeTime);
	pr_info("\tREG_ctrl5.reg_u16BgMaxFadeTime = 0x%x\n", p->REG_ctrl5.reg_u16BgMaxFadeTime);
	pr_info("\tREG_ctrl5.reg_u8StyBgAccTimeRateThr = 0x%x\n", p->REG_ctrl5.reg_u8StyBgAccTimeRateThr);
	pr_info("\tREG_ctrl5.reg_u8ChgBgAccTimeRateThr = 0x%x\n", p->REG_ctrl5.reg_u8ChgBgAccTimeRateThr);
	pr_info("\tREG_ctrl6.reg_u8DynBgAccTimeThr = 0x%x\n", p->REG_ctrl6.reg_u8DynBgAccTimeThr);
	pr_info("\tREG_ctrl6.reg_u8BgEffStaRateThr = 0x%x\n", p->REG_ctrl6.reg_u8BgEffStaRateThr);
	pr_info("\tREG_ctrl6.reg_u8DynBgDepth = 0x%x\n", p->REG_ctrl6.reg_u8DynBgDepth);
	pr_info("\tREG_ctrl6.reg_u8AcceBgLearn = 0x%x\n", p->REG_ctrl6.reg_u8AcceBgLearn);
	pr_info("\tREG_ctrl6.reg_u8DetChgRegion = 0x%x\n", p->REG_ctrl6.reg_u8DetChgRegion);
	pr_info("\tREG_ctrl7.reg_stat_pixnum = 0x%x\n", p->REG_ctrl7.reg_stat_pixnum);
	pr_info("\tREG_ctrl8.reg_stat_sumlum = 0x%x\n", p->REG_ctrl8.reg_stat_sumlum);
	pr_info("\tREG_crop_s.reg_crop_start_x = 0x%x\n", p->REG_crop_s.reg_crop_start_x);
	pr_info("\tREG_crop_s.reg_crop_end_x = 0x%x\n", p->REG_crop_s.reg_crop_end_x);
	pr_info("\tREG_crop_e.reg_crop_start_y = 0x%x\n", p->REG_crop_e.reg_crop_start_y);
	pr_info("\tREG_crop_e.reg_crop_end_y = 0x%x\n", p->REG_crop_e.reg_crop_end_y);
	pr_info("\tREG_crop_ctl.reg_crop_enable = 0x%x\n", p->REG_crop_ctl.reg_crop_enable);
}

// static void ive_ncc_printk(IVE_NCC_C *p)
// {
// 	pr_info("ive_ncc\n");
// 	pr_info("\tREG_NCC_00.reg_numerator_l = 0x%x\n", p->REG_NCC_00.reg_numerator_l);
// 	pr_info("\tREG_NCC_01.reg_numerator_h = 0x%x\n", p->REG_NCC_01.reg_numerator_h);
// 	pr_info("\tREG_NCC_02.reg_quadsum0_l = 0x%x\n", p->REG_NCC_02.reg_quadsum0_l);
// 	pr_info("\tREG_NCC_03.reg_quadsum0_h = 0x%x\n", p->REG_NCC_03.reg_quadsum0_h);
// 	pr_info("\tREG_NCC_04.reg_quadsum1_l = 0x%x\n", p->REG_NCC_04.reg_quadsum1_l);
// 	pr_info("\tREG_NCC_05.reg_quadsum1_h = 0x%x\n", p->REG_NCC_05.reg_quadsum1_h);
// 	pr_info("\tREG_NCC_06.reg_crop_enable = 0x%x\n", p->REG_NCC_06.reg_crop_enable);
// 	pr_info("\tREG_NCC_07.reg_crop_start_x = 0x%x\n", p->REG_NCC_07.reg_crop_start_x);
// 	pr_info("\tREG_NCC_08.reg_crop_start_y = 0x%x\n", p->REG_NCC_08.reg_crop_start_y);
// 	pr_info("\tREG_NCC_09.reg_crop_end_x = 0x%x\n", p->REG_NCC_09.reg_crop_end_x);
// 	pr_info("\tREG_NCC_10.reg_crop_end_y = 0x%x\n", p->REG_NCC_10.reg_crop_end_y);
// 	pr_info("\tREG_NCC_11.reg_shdw_sel = 0x%x\n", p->REG_NCC_11.reg_shdw_sel);
// }

static void isp_dma_ctl_printk(ISP_DMA_CTL_C *p)
{
	pr_info("isp_dma_ctl\n");
	pr_info("\tSYS_CONTROL.reg_qos_sel = 0x%x\n",
		p->SYS_CONTROL.reg_qos_sel);
	pr_info("\tSYS_CONTROL.reg_sw_qos = 0x%x\n", p->SYS_CONTROL.reg_sw_qos);
	pr_info("\tSYS_CONTROL.reg_baseh = 0x%x\n", p->SYS_CONTROL.reg_baseh);
	pr_info("\tSYS_CONTROL.reg_base_sel = 0x%x\n",
		p->SYS_CONTROL.reg_base_sel);
	pr_info("\tSYS_CONTROL.reg_stride_sel = 0x%x\n",
		p->SYS_CONTROL.reg_stride_sel);
	pr_info("\tSYS_CONTROL.reg_seglen_sel = 0x%x\n",
		p->SYS_CONTROL.reg_seglen_sel);
	pr_info("\tSYS_CONTROL.reg_segnum_sel = 0x%x\n",
		p->SYS_CONTROL.reg_segnum_sel);
	pr_info("\tSYS_CONTROL.reg_slice_enable = 0x%x\n",
		p->SYS_CONTROL.reg_slice_enable);
	pr_info("\tSYS_CONTROL.reg_dbg_sel = 0x%x\n",
		p->SYS_CONTROL.reg_dbg_sel);
	pr_info("\tBASE_ADDR.reg_basel = 0x%x\n", p->BASE_ADDR.reg_basel);
	pr_info("\tDMA_SEGLEN.reg_seglen = 0x%x\n", p->DMA_SEGLEN.reg_seglen);
	pr_info("\tDMA_STRIDE.reg_stride = 0x%x\n", p->DMA_STRIDE.reg_stride);
	pr_info("\tDMA_SEGNUM.reg_segnum = 0x%x\n", p->DMA_SEGNUM.reg_segnum);
	pr_info("\tDMA_STATUS.reg_status = 0x%x\n", p->DMA_STATUS.reg_status);
	pr_info("\tDMA_SLICESIZE.reg_slice_size = 0x%x\n",
		p->DMA_SLICESIZE.reg_slice_size);
	pr_info("\tDMA_DUMMY.reg_dummy = 0x%x\n", p->DMA_DUMMY.reg_dummy);
}

static void img_in_printk(IMG_IN_C *p)
{
	pr_info("img_in\n");
	pr_info("\tREG_00.reg_src_sel = 0x%x\n", p->REG_00.reg_src_sel);
	pr_info("\tREG_00.reg_fmt_sel = 0x%x\n", p->REG_00.reg_fmt_sel);
	pr_info("\tREG_00.reg_burst_ln = 0x%x\n", p->REG_00.reg_burst_ln);
	pr_info("\tREG_00.reg_img_csc_en = 0x%x\n", p->REG_00.reg_img_csc_en);
	pr_info("\tREG_00.reg_auto_csc_en = 0x%x\n", p->REG_00.reg_auto_csc_en);
	pr_info("\tREG_00.reg_64b_align = 0x%x\n", p->REG_00.reg_64b_align);
	pr_info("\tREG_00.reg_force_clk_enable = 0x%x\n",
		p->REG_00.reg_force_clk_enable);
	pr_info("\tREG_01.reg_src_x_str = 0x%x\n", p->REG_01.reg_src_x_str);
	pr_info("\tREG_01.reg_src_y_str = 0x%x\n", p->REG_01.reg_src_y_str);
	pr_info("\tREG_02.reg_src_wd = 0x%x\n", p->REG_02.reg_src_wd);
	pr_info("\tREG_02.reg_src_ht = 0x%x\n", p->REG_02.reg_src_ht);
	pr_info("\tREG_03.reg_src_y_pitch = 0x%x\n", p->REG_03.reg_src_y_pitch);
	pr_info("\tREG_04.reg_src_c_pitch = 0x%x\n", p->REG_04.reg_src_c_pitch);
	pr_info("\tREG_05.reg_sw_force_up = 0x%x\n", p->REG_05.reg_sw_force_up);
	pr_info("\tREG_05.reg_sw_mask_up = 0x%x\n", p->REG_05.reg_sw_mask_up);
	pr_info("\tREG_05.reg_shrd_sel = 0x%x\n", p->REG_05.reg_shrd_sel);
	pr_info("\tREG_06.reg_dummy_ro = 0x%x\n", p->REG_06.reg_dummy_ro);
	pr_info("\tREG_07.reg_dummy_0 = 0x%x\n", p->REG_07.reg_dummy_0);
	pr_info("\tREG_08.reg_dummy_1 = 0x%x\n", p->REG_08.reg_dummy_1);
	pr_info("\tREG_Y_BASE_0.reg_src_y_base_b0 = 0x%x\n",
		p->REG_Y_BASE_0.reg_src_y_base_b0);
	pr_info("\tREG_Y_BASE_1.reg_src_y_base_b1 = 0x%x\n",
		p->REG_Y_BASE_1.reg_src_y_base_b1);
	pr_info("\tREG_U_BASE_0.reg_src_u_base_b0 = 0x%x\n",
		p->REG_U_BASE_0.reg_src_u_base_b0);
	pr_info("\tREG_U_BASE_1.reg_src_u_base_b1 = 0x%x\n",
		p->REG_U_BASE_1.reg_src_u_base_b1);
	pr_info("\tREG_V_BASE_0.reg_src_v_base_b0 = 0x%x\n",
		p->REG_V_BASE_0.reg_src_v_base_b0);
	pr_info("\tREG_V_BASE_1.reg_src_v_base_b1 = 0x%x\n",
		p->REG_V_BASE_1.reg_src_v_base_b1);
	pr_info("\tREG_040.reg_c00 = 0x%x\n", p->REG_040.reg_c00);
	pr_info("\tREG_040.reg_c01 = 0x%x\n", p->REG_040.reg_c01);
	pr_info("\tREG_044.reg_c02 = 0x%x\n", p->REG_044.reg_c02);
	pr_info("\tREG_048.reg_c10 = 0x%x\n", p->REG_048.reg_c10);
	pr_info("\tREG_048.reg_c11 = 0x%x\n", p->REG_048.reg_c11);
	pr_info("\tREG_04C.reg_c12 = 0x%x\n", p->REG_04C.reg_c12);
	pr_info("\tREG_050.reg_c20 = 0x%x\n", p->REG_050.reg_c20);
	pr_info("\tREG_050.reg_c21 = 0x%x\n", p->REG_050.reg_c21);
	pr_info("\tREG_054.reg_c22 = 0x%x\n", p->REG_054.reg_c22);
	pr_info("\tREG_058.reg_sub_0 = 0x%x\n", p->REG_058.reg_sub_0);
	pr_info("\tREG_058.reg_sub_1 = 0x%x\n", p->REG_058.reg_sub_1);
	pr_info("\tREG_058.reg_sub_2 = 0x%x\n", p->REG_058.reg_sub_2);
	pr_info("\tREG_05C.reg_add_0 = 0x%x\n", p->REG_05C.reg_add_0);
	pr_info("\tREG_05C.reg_add_1 = 0x%x\n", p->REG_05C.reg_add_1);
	pr_info("\tREG_05C.reg_add_2 = 0x%x\n", p->REG_05C.reg_add_2);
	pr_info("\tREG_060.reg_fifo_rd_th_y = 0x%x\n",
		p->REG_060.reg_fifo_rd_th_y);
	pr_info("\tREG_060.reg_fifo_pr_th_y = 0x%x\n",
		p->REG_060.reg_fifo_pr_th_y);
	pr_info("\tREG_060.reg_fifo_rd_th_c = 0x%x\n",
		p->REG_060.reg_fifo_rd_th_c);
	pr_info("\tREG_060.reg_fifo_pr_th_c = 0x%x\n",
		p->REG_060.reg_fifo_pr_th_c);
	pr_info("\tREG_064.reg_os_max = 0x%x\n", p->REG_064.reg_os_max);
	pr_info("\tREG_068.reg_err_fwr_y = 0x%x\n", p->REG_068.reg_err_fwr_y);
	pr_info("\tREG_068.reg_err_fwr_u = 0x%x\n", p->REG_068.reg_err_fwr_u);
	pr_info("\tREG_068.reg_err_fwr_v = 0x%x\n", p->REG_068.reg_err_fwr_v);
	pr_info("\tREG_068.reg_clr_fwr_w1t = 0x%x\n",
		p->REG_068.reg_clr_fwr_w1t);
	pr_info("\tREG_068.reg_err_erd_y = 0x%x\n", p->REG_068.reg_err_erd_y);
	pr_info("\tREG_068.reg_err_erd_u = 0x%x\n", p->REG_068.reg_err_erd_u);
	pr_info("\tREG_068.reg_err_erd_v = 0x%x\n", p->REG_068.reg_err_erd_v);
	pr_info("\tREG_068.reg_clr_erd_w1t = 0x%x\n",
		p->REG_068.reg_clr_erd_w1t);
	pr_info("\tREG_068.reg_lb_full_y = 0x%x\n", p->REG_068.reg_lb_full_y);
	pr_info("\tREG_068.reg_lb_full_u = 0x%x\n", p->REG_068.reg_lb_full_u);
	pr_info("\tREG_068.reg_lb_full_v = 0x%x\n", p->REG_068.reg_lb_full_v);
	pr_info("\tREG_068.reg_lb_empty_y = 0x%x\n", p->REG_068.reg_lb_empty_y);
	pr_info("\tREG_068.reg_lb_empty_u = 0x%x\n", p->REG_068.reg_lb_empty_u);
	pr_info("\tREG_068.reg_lb_empty_v = 0x%x\n", p->REG_068.reg_lb_empty_v);
	pr_info("\tREG_068.reg_ip_idle = 0x%x\n", p->REG_068.reg_ip_idle);
	pr_info("\tREG_068.reg_ip_int = 0x%x\n", p->REG_068.reg_ip_int);
	pr_info("\tREG_068.reg_ip_clr_w1t = 0x%x\n", p->REG_068.reg_ip_clr_w1t);
	pr_info("\tREG_068.reg_clr_int_w1t = 0x%x\n",
		p->REG_068.reg_clr_int_w1t);
	pr_info("\tREG_AXI_ST.reg_axi_idle = 0x%x\n",
		p->REG_AXI_ST.reg_axi_idle);
	pr_info("\tREG_AXI_ST.reg_axi_status = 0x%x\n",
		p->REG_AXI_ST.reg_axi_status);
	pr_info("\tREG_BW_LIMIT.reg_bwl_win = 0x%x\n",
		p->REG_BW_LIMIT.reg_bwl_win);
	pr_info("\tREG_BW_LIMIT.reg_bwl_vld = 0x%x\n",
		p->REG_BW_LIMIT.reg_bwl_vld);
	pr_info("\tREG_BW_LIMIT.reg_bwl_en = 0x%x\n",
		p->REG_BW_LIMIT.reg_bwl_en);
	pr_info("\tREG_CATCH.reg_catch_mode = 0x%x\n",
		p->REG_CATCH.reg_catch_mode);
	pr_info("\tREG_CATCH.reg_dma_urgent_en = 0x%x\n",
		p->REG_CATCH.reg_dma_urgent_en);
	pr_info("\tREG_CATCH.reg_qos_sel_rr = 0x%x\n",
		p->REG_CATCH.reg_qos_sel_rr);
	pr_info("\tREG_CATCH.reg_catch_act_y = 0x%x\n",
		p->REG_CATCH.reg_catch_act_y);
	pr_info("\tREG_CATCH.reg_catch_act_u = 0x%x\n",
		p->REG_CATCH.reg_catch_act_u);
	pr_info("\tREG_CATCH.reg_catch_act_v = 0x%x\n",
		p->REG_CATCH.reg_catch_act_v);
	pr_info("\tREG_CATCH.reg_catch_fail_y = 0x%x\n",
		p->REG_CATCH.reg_catch_fail_y);
	pr_info("\tREG_CATCH.reg_catch_fail_u = 0x%x\n",
		p->REG_CATCH.reg_catch_fail_u);
	pr_info("\tREG_CATCH.reg_catch_fail_v = 0x%x\n",
		p->REG_CATCH.reg_catch_fail_v);
	pr_info("\tREG_CHK_CTRL.reg_chksum_dat_out = 0x%x\n",
		p->REG_CHK_CTRL.reg_chksum_dat_out);
	pr_info("\tREG_CHK_CTRL.reg_checksum_en = 0x%x\n",
		p->REG_CHK_CTRL.reg_checksum_en);
	pr_info("\tCHKSUM_AXI_RD.reg_chksum_axi_rd = 0x%x\n",
		p->CHKSUM_AXI_RD.reg_chksum_axi_rd);
	pr_info("\tSB_REG_CTRL.reg_sb_mode = 0x%x\n",
		p->SB_REG_CTRL.reg_sb_mode);
	pr_info("\tSB_REG_CTRL.reg_sb_size = 0x%x\n",
		p->SB_REG_CTRL.reg_sb_size);
	pr_info("\tSB_REG_CTRL.reg_sb_nb = 0x%x\n", p->SB_REG_CTRL.reg_sb_nb);
	pr_info("\tSB_REG_CTRL.reg_sb_sw_rptr = 0x%x\n",
		p->SB_REG_CTRL.reg_sb_sw_rptr);
	pr_info("\tSB_REG_CTRL.reg_sb_set_str = 0x%x\n",
		p->SB_REG_CTRL.reg_sb_set_str);
	pr_info("\tSB_REG_CTRL.reg_sb_sw_clr = 0x%x\n",
		p->SB_REG_CTRL.reg_sb_sw_clr);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_rptr_ro = 0x%x\n",
		p->SB_REG_C_STAT.reg_u_sb_rptr_ro);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_full = 0x%x\n",
		p->SB_REG_C_STAT.reg_u_sb_full);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_empty = 0x%x\n",
		p->SB_REG_C_STAT.reg_u_sb_empty);
	pr_info("\tSB_REG_C_STAT.reg_u_sb_dptr_ro = 0x%x\n",
		p->SB_REG_C_STAT.reg_u_sb_dptr_ro);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_rptr_ro = 0x%x\n",
		p->SB_REG_C_STAT.reg_v_sb_rptr_ro);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_full = 0x%x\n",
		p->SB_REG_C_STAT.reg_v_sb_full);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_empty = 0x%x\n",
		p->SB_REG_C_STAT.reg_v_sb_empty);
	pr_info("\tSB_REG_C_STAT.reg_v_sb_dptr_ro = 0x%x\n",
		p->SB_REG_C_STAT.reg_v_sb_dptr_ro);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_rptr_ro = 0x%x\n",
		p->SB_REG_Y_STAT.reg_y_sb_rptr_ro);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_full = 0x%x\n",
		p->SB_REG_Y_STAT.reg_y_sb_full);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_empty = 0x%x\n",
		p->SB_REG_Y_STAT.reg_y_sb_empty);
	pr_info("\tSB_REG_Y_STAT.reg_y_sb_dptr_ro = 0x%x\n",
		p->SB_REG_Y_STAT.reg_y_sb_dptr_ro);
	pr_info("\tSB_REG_Y_STAT.reg_sb_empty = 0x%x\n",
		p->SB_REG_Y_STAT.reg_sb_empty);
}

#if IVE_CMDQ
static void cmdq_printk(CMDQ_C *p)
{
	pr_info("cmdq\n");
	pr_info("\tINT_EVENT.reg_cmdq_int = 0x%x\n", p->INT_EVENT.reg_cmdq_int);
	pr_info("\tINT_EVENT.reg_cmdq_end = 0x%x\n", p->INT_EVENT.reg_cmdq_end);
	pr_info("\tINT_EVENT.reg_cmdq_wait = 0x%x\n", p->INT_EVENT.reg_cmdq_wait);
	pr_info("\tINT_EVENT.reg_isp_pslverr = 0x%x\n", p->INT_EVENT.reg_isp_pslverr);
	pr_info("\tINT_EVENT.reg_task_end = 0x%x\n", p->INT_EVENT.reg_task_end);
	pr_info("\tINT_EN.reg_cmdq_int_en = 0x%x\n", p->INT_EN.reg_cmdq_int_en);
	pr_info("\tINT_EN.reg_cmdq_end_en = 0x%x\n", p->INT_EN.reg_cmdq_end_en);
	pr_info("\tINT_EN.reg_cmdq_wait_en = 0x%x\n", p->INT_EN.reg_cmdq_wait_en);
	pr_info("\tINT_EN.reg_isp_pslverr_en = 0x%x\n", p->INT_EN.reg_isp_pslverr_en);
	pr_info("\tINT_EN.reg_task_end_en = 0x%x\n", p->INT_EN.reg_task_end_en);
	pr_info("\tDMA_ADDR.reg_dma_addr_l = 0x%x\n", p->DMA_ADDR_L.reg_dma_addr_l);
	pr_info("\tDMA_CNT.reg_dma_cnt = 0x%x\n", p->DMA_CNT.reg_dma_cnt);
	pr_info("\tDMA_CONFIG.reg_dma_rsv = 0x%x\n", p->DMA_CONFIG.reg_dma_rsv);
	pr_info("\tDMA_CONFIG.reg_adma_en = 0x%x\n", p->DMA_CONFIG.reg_adma_en);
	pr_info("\tDMA_CONFIG.reg_task_en = 0x%x\n", p->DMA_CONFIG.reg_task_en);
	pr_info("\tAXI_CONFIG.reg_max_burst_len = 0x%x\n", p->AXI_CONFIG.reg_max_burst_len);
	pr_info("\tAXI_CONFIG.reg_ot_enable = 0x%x\n", p->AXI_CONFIG.reg_ot_enable);
	pr_info("\tAXI_CONFIG.reg_sw_overwrite = 0x%x\n", p->AXI_CONFIG.reg_sw_overwrite);
	pr_info("\tJOB_CTL.reg_job_start = 0x%x\n", p->JOB_CTL.reg_job_start);
	pr_info("\tJOB_CTL.reg_cmd_restart = 0x%x\n", p->JOB_CTL.reg_cmd_restart);
	pr_info("\tJOB_CTL.reg_restart_hw_mod = 0x%x\n", p->JOB_CTL.reg_restart_hw_mod);
	pr_info("\tJOB_CTL.reg_cmdq_idle_en = 0x%x\n", p->JOB_CTL.reg_cmdq_idle_en);
	pr_info("\tAPB_PARA.reg_base_addr = 0x%x\n", p->APB_PARA.reg_base_addr);
	pr_info("\tAPB_PARA.reg_apb_pprot = 0x%x\n", p->APB_PARA.reg_apb_pprot);
	pr_info("\tDEBUG_BUS0.reg_debus0 = 0x%x\n", p->DEBUG_BUS0.reg_debus0);
	pr_info("\tDEBUG_BUS1.reg_debus1 = 0x%x\n", p->DEBUG_BUS1.reg_debus1);
	pr_info("\tDEBUG_BUS2.reg_debus2 = 0x%x\n", p->DEBUG_BUS2.reg_debus2);
	pr_info("\tDEBUG_BUS3.reg_debus3 = 0x%x\n", p->DEBUG_BUS3.reg_debus3);
	pr_info("\tDEBUG_BUS_SEL.reg_debus_sel = 0x%x\n", p->DEBUG_BUS_SEL.reg_debus_sel);
	pr_info("\tDUMMY.reg_dummy = 0x%x\n", p->DUMMY.reg_dummy);
	pr_info("\tTASK_DONE_STS.reg_task_done = 0x%x\n", p->TASK_DONE_STS.reg_task_done);
	pr_info("\tDMA_ADDR_TSK0.reg_dma_addr_tsk0 = 0x%x\n", p->DMA_ADDR_TSK0.reg_dma_addr_tsk0);
	pr_info("\tDMA_CNT_TSK0.reg_dma_cnt_tsk0 = 0x%x\n", p->DMA_CNT_TSK0.reg_dma_cnt_tsk0);
	pr_info("\tDMA_ADDR_TSK1.reg_dma_addr_tsk1 = 0x%x\n", p->DMA_ADDR_TSK1.reg_dma_addr_tsk1);
	pr_info("\tDMA_CNT_TSK1.reg_dma_cnt_tsk1 = 0x%x\n", p->DMA_CNT_TSK1.reg_dma_cnt_tsk1);
	pr_info("\tDMA_ADDR_TSK2.reg_dma_addr_tsk2 = 0x%x\n", p->DMA_ADDR_TSK2.reg_dma_addr_tsk2);
	pr_info("\tDMA_CNT_TSK2.reg_dma_cnt_tsk2 = 0x%x\n", p->DMA_CNT_TSK2.reg_dma_cnt_tsk2);
	pr_info("\tDMA_ADDR_TSK3.reg_dma_addr_tsk3 = 0x%x\n", p->DMA_ADDR_TSK3.reg_dma_addr_tsk3);
	pr_info("\tDMA_CNT_TSK3.reg_dma_cnt_tsk3 = 0x%x\n", p->DMA_CNT_TSK3.reg_dma_cnt_tsk3);
	pr_info("\tDMA_ADDR_TSK4.reg_dma_addr_tsk4 = 0x%x\n", p->DMA_ADDR_TSK4.reg_dma_addr_tsk4);
	pr_info("\tDMA_CNT_TSK4.reg_dma_cnt_tsk4 = 0x%x\n", p->DMA_CNT_TSK4.reg_dma_cnt_tsk4);
	pr_info("\tDMA_ADDR_TSK5.reg_dma_addr_tsk5 = 0x%x\n", p->DMA_ADDR_TSK5.reg_dma_addr_tsk5);
	pr_info("\tDMA_CNT_TSK5.reg_dma_cnt_tsk5 = 0x%x\n", p->DMA_CNT_TSK5.reg_dma_cnt_tsk5);
	pr_info("\tDMA_ADDR_TSK6.reg_dma_addr_tsk6 = 0x%x\n", p->DMA_ADDR_TSK6.reg_dma_addr_tsk6);
	pr_info("\tDMA_CNT_TSK6.reg_dma_cnt_tsk6 = 0x%x\n", p->DMA_CNT_TSK6.reg_dma_cnt_tsk6);
	pr_info("\tDMA_ADDR_TSK7.reg_dma_addr_tsk7 = 0x%x\n", p->DMA_ADDR_TSK7.reg_dma_addr_tsk7);
	pr_info("\tDMA_CNT_TSK7.reg_dma_cnt_tsk7 = 0x%x\n", p->DMA_CNT_TSK7.reg_dma_cnt_tsk7);
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
u32 get_channel_count(IVE_IMAGE_TYPE_E type)
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

void dump_ive_image(char *name, IVE_IMAGE_S *img)
{
	s32 i = 0;

	pr_info("Image %s\n", name);
	if (img != NULL) {
		pr_info("\tType: %#x\n", img->enType);
		pr_info("\tWidth: %d\n", img->u32Width);
		pr_info("\tHeight: %d\n", img->u32Height);
		for (i = 0; i < get_channel_count(img->enType); i++) {
			if (img->u64PhyAddr[i] != 0 &&
				img->u64PhyAddr[i] != 0xffffffff) {
				pr_info("\tPhy %d Addr: 0x%08llx\n", i,
					img->u64PhyAddr[i] & 0xffffffff);
				pr_info("\tStride %d: %d\n", i, img->u32Stride[i]);
			}
		}
	} else {
		pr_info("\tNULL\n");
	}
}

void dump_ive_data(char *name, IVE_DATA_S *data)
{
	pr_info("Data %s\n", name);
	if (data != NULL) {
		pr_info("\tWidth: %d\n", data->u32Width);
		pr_info("\tHeight: %d\n", data->u32Height);
		if (data->u64PhyAddr != 0 && data->u64PhyAddr != 0xffffffff) {
			pr_info("\tPhy Addr: 0x%08llx\n", data->u64PhyAddr & 0xffffffff);
			pr_info("\tStride: %d\n", data->u32Stride);
		}
	} else {
		pr_info("\tNULL\n");
	}
}

void dump_ive_mem(char *name, IVE_MEM_INFO_S *mem)
{
	pr_info("Mem %s\n", name);
	if (mem != NULL) {
		pr_info("\tSize: %d\n", mem->u32Size);
		if (mem->u64PhyAddr != 0 && mem->u64PhyAddr != 0xffffffff) {
			pr_info("\tPhy Addr: 0x%08llx\n", mem->u64PhyAddr & 0xffffffff);
		}
	} else {
		pr_info("\tNULL\n");
	}
}

s32 get_img_fmt_sel(IVE_IMAGE_TYPE_E enType)
{
	s32 r = CVI_FAILURE;

	switch (enType) {
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

static void cvi_ive_dump_top_reg_state(void)
{
	IVE_TOP_C top;
	pr_info("IVE_BLK_BA_IVE_TOP 0x%p\n",
		IVE_BLK_BA[d_num].IVE_TOP - g_phy_shift[d_num]);
	memcpy(&top, (void *)(IVE_BLK_BA[d_num].IVE_TOP), sizeof(IVE_TOP_C));
	ive_top_printk(&top);
}

static void cvi_ive_dump_filterop_reg_state(void)
{
	IVE_FILTEROP_C filterop;
	pr_info("IVE_BLK_BA_FILTEROP 0x%p\n",
		IVE_BLK_BA[d_num].FILTEROP - g_phy_shift[d_num]);
	memcpy(&filterop, (void *)(IVE_BLK_BA[d_num].FILTEROP),
		   sizeof(IVE_FILTEROP_C));
	ive_filterop_printk(&filterop);

}

static void cvi_ive_dump_img_reg_state(void)
{
	IMG_IN_C img_in;
	ISP_DMA_CTL_C isp_dma;

	//src1
	pr_info("IVE_BLK_BA_IMG_IN 0x%p\n",
		IVE_BLK_BA[d_num].IMG_IN - g_phy_shift[d_num]);
	memcpy(&img_in, (void *)(IVE_BLK_BA[d_num].IMG_IN), sizeof(IMG_IN_C));
	img_in_printk(&img_in);

	//dst1
	pr_info("IVE_BLK_BA_FILTEROP_WDMA_Y 0x%p\n",
		IVE_BLK_BA[d_num].FILTEROP_WDMA_Y - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].FILTEROP_WDMA_Y),
		   sizeof(ISP_DMA_CTL_C));
	isp_dma_ctl_printk(&isp_dma);

	//src2
	pr_info("IVE_BLK_BA_RDMA_IMG1 0x%p\n",
		IVE_BLK_BA[d_num].RDMA_IMG1 - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].RDMA_IMG1),
		   sizeof(ISP_DMA_CTL_C));
	isp_dma_ctl_printk(&isp_dma);

	//dst2
	pr_info("IVE_BLK_BA_FILTEROP_WDMA_C 0x%p\n",
		IVE_BLK_BA[d_num].FILTEROP_WDMA_C - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].FILTEROP_WDMA_C),
		   sizeof(ISP_DMA_CTL_C));
	isp_dma_ctl_printk(&isp_dma);

	//src3
	pr_info("IVE_BLK_BA_DMAF_RDMA 0x%p\n",
		IVE_BLK_BA[d_num].DMAF_RDMA - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].DMAF_RDMA),
		   sizeof(ISP_DMA_CTL_C));
	isp_dma_ctl_printk(&isp_dma);

	//dst3
	pr_info("IVE_BLK_BA_DMAF_WDMA 0x%p\n",
		IVE_BLK_BA[d_num].DMAF_WDMA - g_phy_shift[d_num]);
	memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].DMAF_WDMA),
		   sizeof(ISP_DMA_CTL_C));
	isp_dma_ctl_printk(&isp_dma);
}

s32 cvi_ive_dump_reg_state(bool bDump)
{
	// ISP_DMA_CTL_C isp_dma;
	IVE_GMM_C gmm;
	IVE_MATCH_BG_C bgmodel;
	IVE_UPDATE_BG_C upmodel;
	// IVE_MAP_C map;
	// IVE_DMA_C dma;
	// IVE_NCC_C ncc;
	// IVE_SAD_C sad;


	if (g_dump_reg_info || bDump) {
		//top
		cvi_ive_dump_top_reg_state();

		//filterop
		cvi_ive_dump_filterop_reg_state();

		//print src and dst
		cvi_ive_dump_img_reg_state();

		//SAD SAD_WDMA
		// pr_info("IVE_BLK_BA_SAD_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].SAD_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].SAD_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// //SAD WDMA_THR
		// pr_info("IVE_BLK_BA_SAD_WDMA_THR 0x%p\n",
		// 	IVE_BLK_BA[d_num].SAD_WDMA_THR - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].SAD_WDMA_THR),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// //RDMA_EIGVAL
		// pr_info("IVE_BLK_RDMA_EIGVAL 0x%p\n",
		// 	IVE_BLK_BA[d_num].RDMA_EIGVAL - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].RDMA_EIGVAL),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// //MAP
		// pr_info("IVE_BLK_BA_MAP 0x%p\n", IVE_BLK_BA[d_num].MAP - g_phy_shift[d_num]);
		// memcpy(&map, (void *)(IVE_BLK_BA[d_num].MAP), sizeof(IVE_MAP_C));
		// ive_map_printk(&map);

		// //DMAF
		// pr_info("IVE_BLK_BA_DMAF 0x%p\n",
		// 	IVE_BLK_BA[d_num].DMAF - g_phy_shift[d_num]);
		// memcpy(&dma, (void *)(IVE_BLK_BA[d_num].DMAF), sizeof(IVE_DMA_C));
		// ive_dma_printk(&dma);

		// //NCC
		// pr_info("IVE_BLK_BA_NCC 0x%p\n", IVE_BLK_BA[d_num].NCC - g_phy_shift[d_num]);
		// memcpy(&ncc, (void *)(IVE_BLK_BA[d_num].NCC), sizeof(IVE_NCC_C));
		// ive_ncc_printk(&ncc);

		// //SAD
		// pr_info("IVE_BLK_BA_SAD 0x%p\n", IVE_BLK_BA[d_num].SAD - g_phy_shift[d_num]);
		// memcpy(&sad, (void *)(IVE_BLK_BA[d_num].SAD), sizeof(IVE_SAD_C));
		// ive_sad_printk(&sad);

		// //GMM_MATCH_WDMA
		// pr_info("IVE_BLK_BA_GMM_MATCH_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MATCH_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MATCH_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_FACTOR_RDMA
		// pr_info("IVE_BLK_BA_GMM_FACTOR_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_FACTOR_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_FACTOR_RDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_0
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_0 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_0 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_0),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_1
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_1 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_1 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_1),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_2
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_2 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_2 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_2),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_3
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_3 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_3 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_3),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_RDMA_4
		// pr_info("IVE_BLK_BA_GMM_MODEL_RDMA_4 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_RDMA_4 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_RDMA_4),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_0
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_0 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_0 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_0),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_1
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_1 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_1 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_1),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_2
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_2 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_2 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_2),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_3
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_3 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_3 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_3),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		// //GMM_MODEL_WDMA_4
		// pr_info("IVE_BLK_BA_GMM_MODEL_WDMA_4 0x%p\n",
		// 	IVE_BLK_BA[d_num].GMM_MODEL_WDMA_4 - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].GMM_MODEL_WDMA_4),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);
		//GMM
		pr_info("IVE_BLK_BA_GMM 0x%p\n", IVE_BLK_BA[d_num].GMM - g_phy_shift[d_num]);
		memcpy(&gmm, (void *)(IVE_BLK_BA[d_num].GMM), sizeof(IVE_GMM_C));
		ive_gmm_printk(&gmm);

		pr_info("IVE_BLK_BA_MATCH_BGMODEL 0x%p\n",
			IVE_BLK_BA[d_num].BG_MATCH_IVE_MATCH_BG - g_phy_shift[d_num]);
		memcpy(&bgmodel, (void *)(IVE_BLK_BA[d_num].BG_MATCH_IVE_MATCH_BG),
			   sizeof(IVE_MATCH_BG_C));
		ive_match_bg_printk(&bgmodel);

		pr_info("IVE_BLK_BA_UPDATE_BGMODEL 0x%p\n",
			IVE_BLK_BA[d_num].BG_UPDATE_UPDATE_BG - g_phy_shift[d_num]);
		memcpy(&upmodel, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_UPDATE_BG),
			   sizeof(IVE_UPDATE_BG_C));
		ive_update_bg_printk(&upmodel);

		// pr_info("BG_MATCH_BGMODEL_0_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_0_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_0_RDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_MATCH_BGMODEL_1_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_1_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_BGMODEL_1_RDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_MATCH_FGFLAG_RDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_FGFLAG_RDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_FGFLAG_RDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_UPDATE_BGMODEL_0_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_0_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_0_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_UPDATE_BGMODEL_1_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_1_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_BGMODEL_1_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_UPDATE_FG_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_FG_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_FG_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("BG_MATCH_DIFFFG_WDMA 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_MATCH_DIFFFG_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_MATCH_DIFFFG_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

		// pr_info("IVE_BLK_ID_BG_UPDATE 0x%p\n",
		// 	IVE_BLK_BA[d_num].BG_UPDATE_CHG_WDMA - g_phy_shift[d_num]);
		// memcpy(&isp_dma, (void *)(IVE_BLK_BA[d_num].BG_UPDATE_CHG_WDMA),
		// 	   sizeof(ISP_DMA_CTL_C));
		// isp_dma_ctl_printk(&isp_dma);

	}
	return CVI_SUCCESS;
}

void ive_reset_reg(s32 select, IVE_TOP_C *Top, s32 dev_id)
{
	s32 i = 0;
	s32 size = 0;
	uint32_t *array;

	if (select == 1) {
		size = sizeof(IVE_TOP_C) / sizeof(uint32_t);
		array = (uint32_t *)Top;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IVE_TOP + sizeof(uint32_t) * i));
		}

	} else if (select == 2) {
		//DEFINE_IVE_FILTEROP_C(FilterOp);
		IVE_FILTEROP_C FilterOp = _DEFINE_IVE_FILTEROP_C;

		size = sizeof(IVE_FILTEROP_C) / sizeof(uint32_t);
		array = (uint32_t *)&FilterOp;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].FILTEROP + sizeof(uint32_t) * i));
		}
	} else if (select == 3) {
		//DEFINE_IMG_IN_C(ImageIn);
		IMG_IN_C ImageIn = _DEFINE_IMG_IN_C;

		size = sizeof(IMG_IN_C) / sizeof(uint32_t);
		array = (uint32_t *)&ImageIn;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IMG_IN + sizeof(uint32_t) * i));
		}
	} else if (select == 4) {
		//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
		IVE_MATCH_BG_C ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
		//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
		IVE_UPDATE_BG_C ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;

		size = sizeof(IVE_MATCH_BG_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_match_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_UPDATE_BG_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_update_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
					  sizeof(uint32_t) * i));
		}
	} else {
		//DEFINE_IVE_FILTEROP_C(FilterOp);
		IVE_FILTEROP_C FilterOp = _DEFINE_IVE_FILTEROP_C;
		//DEFINE_IMG_IN_C(ImageIn);
		IMG_IN_C ImageIn = _DEFINE_IMG_IN_C;
		//DEFINE_IVE_DMA_C(DMA);
		IVE_DMA_C DMA = _DEFINE_IVE_DMA_C;
		//DEFINE_ISP_DMA_CTL_C(DMActl);
		ISP_DMA_CTL_C DMActl = _DEFINE_ISP_DMA_CTL_C;
		//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
		IVE_MATCH_BG_C ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
		//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
		IVE_UPDATE_BG_C ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;
		//DEFINE_IVE_HIST_C(ive_hist_c);
		IVE_HIST_C ive_hist_c = _DEFINE_IVE_HIST_C;
		//DEFINE_IVE_MAP_C(ive_map_c);
		IVE_MAP_C ive_map_c = _DEFINE_IVE_MAP_C;
		//DEFINE_IVE_INTG_C(ive_intg_c);
		IVE_INTG_C ive_intg_c = _DEFINE_IVE_INTG_C;
		//DEFINE_IVE_SAD_C(ive_sad_c);
		IVE_SAD_C ive_sad_c = _DEFINE_IVE_SAD_C;
		//DEFINE_IVE_NCC_C(ive_ncc_c);
		IVE_NCC_C ive_ncc_c = _DEFINE_IVE_NCC_C;

		size = sizeof(IVE_TOP_C) / sizeof(uint32_t);
		array = (uint32_t *)Top;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IVE_TOP + sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_FILTEROP_C) / sizeof(uint32_t);
		array = (uint32_t *)&FilterOp;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].FILTEROP + sizeof(uint32_t) * i));
		}
		Top->REG_h10.reg_img_in_top_enable = 1;
		//img_in_printk((IMG_IN_C *)(IVE_BLK_BA.IMG_IN));
		writel(Top->REG_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		size = sizeof(IMG_IN_C) / sizeof(uint32_t);
		array = (uint32_t *)&ImageIn;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].IMG_IN + sizeof(uint32_t) * i));
		}
		//img_in_printk((IMG_IN_C *)(IVE_BLK_BA.IMG_IN));
		Top->REG_h10.reg_img_in_top_enable = 0;
		writel(Top->REG_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		size = sizeof(IVE_DMA_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMA;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF + sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_MATCH_BG_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_match_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_UPDATE_BG_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_update_bg_c;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_HIST_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_hist_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].HIST + sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_MAP_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_map_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].MAP + sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_INTG_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_intg_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].INTG + sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_SAD_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_sad_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].SAD + sizeof(uint32_t) * i));
		}
		size = sizeof(IVE_NCC_C) / sizeof(uint32_t);
		array = (uint32_t *)&ive_ncc_c;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].NCC + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF_RDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].DMAF_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].HIST_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].INTG_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i],
				   (IVE_BLK_BA[dev_id].SAD_WDMA + sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].SAD_WDMA_THR +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_1 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_2 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_3 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_4 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_1 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_2 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
		array = (uint32_t *)&DMActl;
		for (i = 0; i < size; i++) {
			writel(array[i], (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_3 +
					  sizeof(uint32_t) * i));
		}
		size = sizeof(ISP_DMA_CTL_C) / sizeof(uint32_t);
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
		// ive_top_c.REG_90.val = status;
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
		//ive_top_c->REG_98.val = status;
		//ive_top_c->REG_98.val = status;
#if defined(__CV186X__)
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98));
#else
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98));
		writel(status, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98));
#endif
	}
	return status;
}

inline s32 cvi_ive_go(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c,
			  bool bInstant, s32 done_mask, s32 optype, s32 dev_id)
{
	s32 cnt = 0;
	long leavetime = 0;
	s32 ret = CVI_SUCCESS;
	s32 int_mask = 0;
	IVE_FILTEROP_REG_h10_C REG_h10;
	IVE_FILTEROP_REG_h14_C REG_h14;
	IVE_FILTEROP_REG_28_C  REG_28;

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
			ive_top_c->REG_94.reg_intr_en_hist = !bInstant;
		} else if (optype == MOD_NCC) {
			int_mask = IVE_TOP_REG_INTR_STATUS_NCC_MASK;
			ive_top_c->REG_94.reg_intr_en_ncc = !bInstant;
		} else if (optype == MOD_INTEG) {
			int_mask = IVE_TOP_REG_INTR_STATUS_INTG_MASK;
			ive_top_c->REG_94.reg_intr_en_intg = !bInstant;
		} else if (optype == MOD_DMA) {
			int_mask = IVE_TOP_REG_INTR_STATUS_DMAF_MASK;
			ive_top_c->REG_94.reg_intr_en_dmaf = !bInstant;
		} else if (optype == MOD_GRADFG) {
			int_mask = IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_Y_MASK;
			ive_top_c->REG_94.reg_intr_en_filterop_wdma_y = !bInstant;
		} else if (optype == MOD_SAD) {
			int_mask = IVE_TOP_REG_INTR_STATUS_SAD_MASK;
			ive_top_c->REG_94.reg_intr_en_sad = !bInstant;
		} else if (optype == MOD_CCL){
			int_mask = IVE_TOP_REG_INTR_STATUS_CCL_MASK;
			ive_top_c->REG_94.reg_intr_en_ccl = !bInstant;
		} else {
			int_mask = IVE_TOP_REG_INTR_STATUS_FILTEROP_ODMA_MASK |
				   IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_Y_MASK |
				   IVE_TOP_REG_INTR_STATUS_FILTEROP_WDMA_C_MASK;
			ive_top_c->REG_94.reg_intr_en_filterop_odma = !bInstant;
			ive_top_c->REG_94.reg_intr_en_filterop_wdma_y = !bInstant;
			ive_top_c->REG_94.reg_intr_en_filterop_wdma_c = !bInstant;
		}
		writel(ive_top_c->REG_94.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_94));

		REG_h10.val = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10);
		REG_h14.val = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14);
		REG_28.val = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28);

		g_debug_info.op[0].op_en = REG_h14.reg_filterop_sw_ovw_op;
		g_debug_info.op[0].op_sel = REG_h14.reg_filterop_op1_cmd;
		g_debug_info.op[1].op_en = REG_28.reg_filterop_op2_erodila_en;
		g_debug_info.op[1].op_sel = REG_h10.reg_filterop_mode;

		//if (optype != MOD_INTEG && optype != MOD_DMA && optype != MOD_GRADFG && optype != MOD_SAD){
		clear_framedone(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90), 0, dev_id);
		clear_interrupt_status(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98), 0, dev_id);
		//}
		// GoGoGo
		cvi_ive_dump_reg_state(false);
		//cvi_ive_dump_hw_flow();
		if (optype == MOD_DMA) {
			ive_top_c->REG_1.reg_fmt_vld_dmaf = 1;
		} else if (optype == MOD_CCL){
			ive_top_c->REG_1.reg_fmt_vld_fg = 1;
			ive_top_c->REG_1.reg_fmt_vld_ccl = 1;
		} else {
			ive_top_c->REG_1.reg_fmt_vld_fg = 1;
		}

		writel(ive_top_c->REG_1.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_1));
		start_vld_time(optype);
		if (bInstant) {
			CVI_TRACE_IVE(CVI_DBG_INFO, "bInstant is true\n");
			while (((readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90) &
				done_mask) != done_mask) && cnt < 10000) {
				usleep_range(10, 20);
				cnt++;
			}
			stop_vld_time(optype, ndev->core[dev_id].tile_num);
			complete(&ndev->core[dev_id].frame_done);
			if (cnt >= 10000) {
				CVI_TRACE_IVE(CVI_DBG_ERR, "frame done timeout!\n");
				ret = CVI_FAILURE;
			}
		} else {
				CVI_TRACE_IVE(CVI_DBG_INFO, "bInstant is false\n");
			    leavetime = wait_for_completion_timeout(
				&ndev->core[dev_id].frame_done, msecs_to_jiffies(10 * TIMEOUT_MS));

			reinit_completion(&ndev->core[dev_id].frame_done);
			if (leavetime <= 0) {
				CVI_TRACE_IVE(CVI_DBG_ERR, "frame done timeout!\n");
				ret = CVI_FAILURE;
			}
		}
		if (ndev->core[dev_id].tile_num == ndev->core[dev_id].total_tile) {
			complete(&ndev->core[dev_id].op_done);
		}
	//}

	return ret;
}

s32 emit_bgm_tile(
	struct cvi_ive_device *ndev, bool enWdma_y, bool enOdma, s32 optype,
	IVE_TOP_C *ive_top_c, IVE_FILTEROP_C *ive_filterop_c,
	IMG_IN_C *img_in_c, IVE_GMM_C *ive_gmm_c, ISP_DMA_CTL_C *wdma_y_ctl_c,
	u32 Dst0Stride, u32 u32Width, u32 u32Height,
	u32 dstEnType, u32 srcEnType, bool bInstant,
	IVE_BG_STAT_DATA_S *stat, ISP_DMA_CTL_C *gmm_mod_rdma_ctl_c[5],
	ISP_DMA_CTL_C *gmm_mod_wdma_ctl_c[5],
	ISP_DMA_CTL_C *gmm_match_wdma_ctl_c,
	ISP_DMA_CTL_C *gmm_factor_rdma_ctl_c, ISP_DMA_CTL_C *fgflag_rdma_ctl_c,
	ISP_DMA_CTL_C *bgmodel_0_rdma_ctl_c, ISP_DMA_CTL_C *difffg_wdma_ctl_c,
	ISP_DMA_CTL_C *bgmodel_1_rdma_ctl_c, ISP_DMA_CTL_C *fg_wdma_ctl_c,
	ISP_DMA_CTL_C *bgmodel_0_wdma_ctl_c,
	ISP_DMA_CTL_C *bgmodel_1_wdma_ctl_c, ISP_DMA_CTL_C *chg_wdma_ctl_c,
	IVE_UPDATE_BG_C *ive_update_bg_c, s32 dev_id)
{
	u32 rdma_val = 0x201;
	s32 done_mask = 0;
	s32 tileNum = 1 + (u32Width - 1) / 480;
	s32 remain_width = u32Width;
	s32 tileLen[6] = { 0 };
	s32 segLen[6] = { 0 };
	s32 inOffset[6] = { 0 };
	s32 outOffset[6] = { 0 };
	s32 cropstart[6] = { 0 };
	s32 cropend[6] = { 0 };
	s32 i, round, n;
	//DEFINE_IMG_IN_C(_img_in_c);
	IMG_IN_C _img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_GMM_C(_ive_gmm_c);
	IVE_GMM_C _ive_gmm_c = _DEFINE_IVE_GMM_C;
	//DEFINE_ISP_DMA_CTL_C(_wdma_y_ctl_c);
	ISP_DMA_CTL_C _wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_gmm_match_wdma_ctl_c);
	ISP_DMA_CTL_C _gmm_match_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_gmm_factor_rdma_ctl_c);
	ISP_DMA_CTL_C _gmm_factor_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_fgflag_rdma_ctl_c);
	ISP_DMA_CTL_C _fgflag_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_bgmodel_0_rdma_ctl_c);
	ISP_DMA_CTL_C _bgmodel_0_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_difffg_wdma_ctl_c);
	ISP_DMA_CTL_C _difffg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_bgmodel_1_rdma_ctl_c);
	ISP_DMA_CTL_C _bgmodel_1_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_fg_wdma_ctl_c);
	ISP_DMA_CTL_C _fg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_bgmodel_0_wdma_ctl_c);
	ISP_DMA_CTL_C _bgmodel_0_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_bgmodel_1_wdma_ctl_c);
	ISP_DMA_CTL_C _bgmodel_1_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_chg_wdma_ctl_c);
	ISP_DMA_CTL_C _chg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_UPDATE_BG_C(_ive_update_bg_c);
	IVE_UPDATE_BG_C _ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;

	if (img_in_c == NULL) {
		img_in_c = &_img_in_c;
	}
	if (ive_gmm_c == NULL) {
		ive_gmm_c = &_ive_gmm_c;
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
	if (ive_update_bg_c == NULL) {
		ive_update_bg_c = &_ive_update_bg_c;
	}

	if (tileNum > 6)
		return CVI_FAILURE;

	//for (dev_id = 0; dev_id < IVE_DEV_MAX; dev_id++) {
		if (atomic_read(&ndev->core[dev_id].dev_state) == IVE_CORE_STATE_RUNNING) {
			ndev->core[dev_id].total_tile = tileNum - 1;
			//Calcuate tileLen array
			for (n = 0; n < tileNum && remain_width > 0; n++) {
				if (remain_width > 480) {
					tileLen[n] = 480;
					remain_width -= 480;
				} else {
					//in case last tile too short
					if (remain_width < 64) {
						tileLen[n] = 128;
						tileLen[0] -= (128 - remain_width);
					} else {
						tileLen[n] = remain_width;
					}
					remain_width = 0;
				}
				cropstart[n] = 0;
				cropend[n] = tileLen[n] - 1;
				segLen[n] = tileLen[n];
				outOffset[n] = (n == 0) ? 0 : segLen[n - 1];
				inOffset[n] = outOffset[n];
			}

			bgmodel_0_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			bgmodel_0_rdma_ctl_c->DMA_STRIDE.reg_stride = 16 * width_align(u32Width, 16);
			bgmodel_1_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			bgmodel_1_rdma_ctl_c->DMA_STRIDE.reg_stride = 8 * width_align(u32Width, 16);
			fgflag_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			fgflag_rdma_ctl_c->DMA_STRIDE.reg_stride = Dst0Stride;
			difffg_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			difffg_wdma_ctl_c->DMA_STRIDE.reg_stride = 2 * width_align(u32Width, 16);

			bgmodel_0_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			bgmodel_0_wdma_ctl_c->DMA_STRIDE.reg_stride = 16 * width_align(u32Width, 16);
			bgmodel_1_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			bgmodel_1_wdma_ctl_c->DMA_STRIDE.reg_stride = 8 * width_align(u32Width, 16);
			fg_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			fg_wdma_ctl_c->DMA_STRIDE.reg_stride = Dst0Stride;
			chg_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
			chg_wdma_ctl_c->DMA_STRIDE.reg_stride = 4 * Dst0Stride;

			rdma_val = readl(IVE_BLK_BA[dev_id].RDMA);
			// Increase the number of times to read dram
			// reduce read/write switch, bit 16 =1, bit[15:8] = 'd4
			writel(0x10401, IVE_BLK_BA[dev_id].RDMA);

			writel(bgmodel_0_rdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_0_rdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(bgmodel_1_rdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_rdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(fgflag_rdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fgflag_rdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(difffg_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(difffg_wdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_DMA_STRIDE);

			writel(bgmodel_0_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_0_wdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(bgmodel_1_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_wdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(fg_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fg_wdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_DMA_STRIDE);
			writel(chg_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(chg_wdma_ctl_c->DMA_STRIDE.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_DMA_STRIDE);

			if (optype == MOD_BGM || optype == MOD_BGU) {
				stat->u32PixNum = 0;
				stat->u32SumLum = 0;
			}
			if (optype == MOD_BGM) {
				ive_update_bg_c->REG_crop_ctl.reg_crop_enable = 0;
				writel(ive_update_bg_c->REG_crop_ctl.val,
					   IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
						   IVE_UPDATE_BG_REG_CROP_CTL);
				enWdma_y = false;
				enOdma = false;
			} else if (optype == MOD_BGU) {
				if ((readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
					   IVE_UPDATE_BG_REG_CROP_S) &
					 IVE_UPDATE_BG_REG_CROP_END_X_MASK) > 0) {
					ive_update_bg_c->REG_crop_ctl.reg_crop_enable = 1;
					writel(ive_update_bg_c->REG_crop_ctl.val,
						   IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							   IVE_UPDATE_BG_REG_CROP_CTL);
					if (enWdma_y) {
						ive_filterop_c->REG_cropy_s.reg_crop_y_end_x =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_X) >>
							 IVE_UPDATE_BG_REG_CROP_END_X_OFFSET) &
							0xffff;
						ive_filterop_c->REG_cropy_e.reg_crop_y_end_y =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_Y) >>
							 IVE_UPDATE_BG_REG_CROP_END_Y_OFFSET) &
							0xffff;
						ive_filterop_c->REG_cropy_ctl.reg_crop_y_enable =
							0;
						writel(ive_filterop_c->REG_cropy_s.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROPY_S);
						writel(ive_filterop_c->REG_cropy_e.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROPY_E);
						writel(ive_filterop_c->REG_cropy_ctl.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROPY_CTL);
					}
					if (enOdma) {
						ive_filterop_c->REG_crop_odma_s
							.reg_crop_odma_start_x = 0;
						ive_filterop_c->REG_crop_odma_s
							.reg_crop_odma_end_x =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_X) >>
							 IVE_UPDATE_BG_REG_CROP_END_X_OFFSET) &
							0xffff;
						ive_filterop_c->REG_crop_odma_e
							.reg_crop_odma_start_y = 0;
						ive_filterop_c->REG_crop_odma_e
							.reg_crop_odma_end_y =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_Y) >>
							 IVE_UPDATE_BG_REG_CROP_END_Y_OFFSET) &
							0xffff;
						ive_filterop_c->REG_crop_odma_ctl
							.reg_crop_odma_enable = 0;
						writel(ive_filterop_c->REG_crop_odma_s.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROP_ODMA_S);
						writel(ive_filterop_c->REG_crop_odma_e.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROP_ODMA_E);
						writel(ive_filterop_c->REG_crop_odma_ctl.val,
							   IVE_BLK_BA[dev_id].FILTEROP +
								   IVE_FILTEROP_REG_CROP_ODMA_CTL);
					}
				} else {
					ive_update_bg_c->REG_crop_ctl.reg_crop_enable = 0;
					ive_filterop_c->REG_cropy_ctl.reg_crop_y_enable = 0;
					ive_filterop_c->REG_crop_odma_ctl.reg_crop_odma_enable =
						0;
					writel(ive_update_bg_c->REG_crop_ctl.val,
						   IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							   IVE_UPDATE_BG_REG_CROP_CTL);
					writel(ive_filterop_c->REG_cropy_ctl.val,
						   IVE_BLK_BA[dev_id].FILTEROP +
							   IVE_FILTEROP_REG_CROPY_CTL);
					writel(ive_filterop_c->REG_crop_odma_ctl.val,
						   IVE_BLK_BA[dev_id].FILTEROP +
							   IVE_FILTEROP_REG_CROP_ODMA_CTL);
				}
				// ive_update_bg_print(ive_update_bg_c);
			}

			for (round = 0; round < tileNum; round++) {
				ndev->core[dev_id].tile_num = round;
				ive_top_c->REG_2.reg_img_widthm1 = tileLen[round] - 1;
				img_in_c->REG_02.reg_src_wd = tileLen[round] - 1;
				img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 += inOffset[round];
				writel(ive_top_c->REG_2.val,
					   IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_2);
				writel(img_in_c->REG_02.val, IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02);
				writel(img_in_c->REG_Y_BASE_0.val,
					   IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_0);
				if (enWdma_y) {
					wdma_y_ctl_c->BASE_ADDR.reg_basel += outOffset[round];
					wdma_y_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
					wdma_y_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
					wdma_y_ctl_c->DMA_SEGLEN.reg_seglen = 0;
					wdma_y_ctl_c->DMA_STRIDE.reg_stride = Dst0Stride;

					ive_filterop_c->REG_cropy_s.reg_crop_y_start_x =
						cropstart[round];
					ive_filterop_c->REG_cropy_s.reg_crop_y_end_x =
						cropend[round];
					ive_filterop_c->REG_cropy_e.reg_crop_y_start_y = 0;
					ive_filterop_c->REG_cropy_e.reg_crop_y_end_y =
						u32Height - 1;
					ive_filterop_c->REG_cropy_ctl.reg_crop_y_enable = 0;

					writel(wdma_y_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_BASE_ADDR));
					writel(wdma_y_ctl_c->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(wdma_y_ctl_c->DMA_SEGLEN.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(wdma_y_ctl_c->DMA_STRIDE.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_STRIDE));
					writel(ive_filterop_c->REG_cropy_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_S));
					writel(ive_filterop_c->REG_cropy_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_E));
					writel(ive_filterop_c->REG_cropy_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_CTL));
				}
				if (enOdma) {
					ive_filterop_c->ODMA_REG_11.reg_dma_wd =
						segLen[round] - 1;
					writel(ive_filterop_c->ODMA_REG_11.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_11));
					switch (dstEnType) {
					case IVE_IMAGE_TYPE_YUV420P:
					case IVE_IMAGE_TYPE_YUV422P:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_03
							.reg_dma_u_base_low_part +=
							outOffset[round] / 2;
						ive_filterop_c->ODMA_REG_05
							.reg_dma_v_base_low_part +=
							outOffset[round] / 2;
						break;
					case IVE_IMAGE_TYPE_YUV420SP: // NV21
					case IVE_IMAGE_TYPE_YUV422SP:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						img_in_c->REG_U_BASE_0.reg_src_u_base_b0 +=
							inOffset[round];
						img_in_c->REG_00.reg_auto_csc_en = 0;
						writel(img_in_c->REG_U_BASE_0.val,
							   (IVE_BLK_BA[dev_id].IMG_IN +
							IMG_IN_REG_U_BASE_0));
						writel(img_in_c->REG_00.val,
							   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));
						break;
					case IVE_IMAGE_TYPE_U8C3_PLANAR:
						ive_filterop_c->ODMA_REG_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_05
							.reg_dma_v_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C1:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C3_PACKAGE:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round] * 3;
						img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 +=
							inOffset[round] * 2;
						writel(img_in_c->REG_Y_BASE_0.val,
							   (IVE_BLK_BA[dev_id].IMG_IN +
							IMG_IN_REG_Y_BASE_0));
						break;
					default:
						break;
					}
					writel(ive_filterop_c->ODMA_REG_01.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_01));
					writel(ive_filterop_c->ODMA_REG_03.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_03));
					writel(ive_filterop_c->ODMA_REG_05.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_05));

					if (ive_update_bg_c->REG_crop_ctl.reg_crop_enable) {
						ive_filterop_c->ODMA_REG_11.reg_dma_wd =
							(readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
								   IVE_UPDATE_BG_REG_CROP_END_X) >>
							 IVE_UPDATE_BG_REG_CROP_END_X_OFFSET) &
							0xffff;
						writel(ive_filterop_c->ODMA_REG_11.val,
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

						if (i < ive_gmm_c->REG_GMM_13
								.reg_gmm_gmm2_model_num) {
							gmm_mod_rdma_ctl_c[i]
								->BASE_ADDR.reg_basel +=
								u32ModelSize * inOffset[round];
							gmm_mod_rdma_ctl_c[i]
								->DMA_STRIDE.reg_stride =
								u32ModelSize * width_align(u32Width, 16);
							gmm_mod_rdma_ctl_c[i]
								->SYS_CONTROL.reg_stride_sel = 1;

							gmm_mod_wdma_ctl_c[i]
								->BASE_ADDR.reg_basel +=
								u32ModelSize *
								outOffset[round]; //!?
							gmm_mod_wdma_ctl_c[i]
								->DMA_STRIDE.reg_stride =
								u32ModelSize * width_align(u32Width, 16);
							gmm_mod_wdma_ctl_c[i]
								->SYS_CONTROL.reg_stride_sel = 1;

							writel(gmm_mod_rdma_ctl_c[i]
									   ->BASE_ADDR.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_BASE_ADDR));
							writel(gmm_mod_rdma_ctl_c[i]
									   ->DMA_STRIDE.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_DMA_STRIDE));
							writel(gmm_mod_rdma_ctl_c[i]
									   ->SYS_CONTROL.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_SYS_CONTROL));
							writel(gmm_mod_wdma_ctl_c[i]
									   ->BASE_ADDR.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_BASE_ADDR));
							writel(gmm_mod_wdma_ctl_c[i]
									   ->DMA_STRIDE.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_DMA_STRIDE));
							writel(gmm_mod_wdma_ctl_c[i]
									   ->SYS_CONTROL.val,
								   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 +
								i * 0x40 +
								ISP_DMA_CTL_SYS_CONTROL));
						}
					}
					if (ive_gmm_c->REG_GMM_13.reg_gmm_gmm2_enable > 1) {
						gmm_factor_rdma_ctl_c->BASE_ADDR.reg_basel +=
							inOffset[round] * 2;
						gmm_factor_rdma_ctl_c->SYS_CONTROL
							.reg_stride_sel = 1;
						//gmm_factor_rdma_ctl_c->DMA_STRIDE.reg_stride =
						//	u32Width * 2;
						gmm_match_wdma_ctl_c->BASE_ADDR.reg_basel +=
							outOffset[round];
						//gmm_match_wdma_ctl_c->DMA_STRIDE.reg_stride =
						//	u32Width;
						gmm_match_wdma_ctl_c->SYS_CONTROL
							.reg_stride_sel = 1;
						writel(gmm_factor_rdma_ctl_c->BASE_ADDR.val,
							   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
							ISP_DMA_CTL_BASE_ADDR));
						writel(gmm_factor_rdma_ctl_c->SYS_CONTROL.val,
							   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
							ISP_DMA_CTL_SYS_CONTROL));
						writel(gmm_factor_rdma_ctl_c->DMA_STRIDE.val,
							   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA +
							ISP_DMA_CTL_DMA_STRIDE));
						writel(gmm_match_wdma_ctl_c->BASE_ADDR.val,
							   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
							ISP_DMA_CTL_BASE_ADDR));
						writel(gmm_match_wdma_ctl_c->DMA_STRIDE.val,
							   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
							ISP_DMA_CTL_DMA_STRIDE));
						writel(gmm_match_wdma_ctl_c->SYS_CONTROL.val,
							   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA +
							ISP_DMA_CTL_SYS_CONTROL));
					}
				} else if (optype == MOD_BGM || optype == MOD_BGU) {
					bgmodel_0_rdma_ctl_c->BASE_ADDR.reg_basel +=
						inOffset[round] * 16;
					bgmodel_1_rdma_ctl_c->BASE_ADDR.reg_basel +=
						inOffset[round] * 8;
					fgflag_rdma_ctl_c->BASE_ADDR.reg_basel +=
						inOffset[round];

					bgmodel_0_wdma_ctl_c->BASE_ADDR.reg_basel +=
						outOffset[round] * 16;
					bgmodel_1_wdma_ctl_c->BASE_ADDR.reg_basel +=
						outOffset[round] * 8;
					fg_wdma_ctl_c->BASE_ADDR.reg_basel += outOffset[round];
					writel(bgmodel_0_rdma_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(bgmodel_1_rdma_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(fgflag_rdma_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(bgmodel_0_wdma_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(bgmodel_1_wdma_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(fg_wdma_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA +
						ISP_DMA_CTL_BASE_ADDR));

					if (optype == MOD_BGM) {
						difffg_wdma_ctl_c->BASE_ADDR.reg_basel +=
							outOffset[round] * 2;
						writel(difffg_wdma_ctl_c->BASE_ADDR.val,
							   (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA +
							ISP_DMA_CTL_BASE_ADDR));
					}
					if (optype == MOD_BGU) {
						chg_wdma_ctl_c->BASE_ADDR.reg_basel +=
							outOffset[round] * 4;
						writel(chg_wdma_ctl_c->BASE_ADDR.val,
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

				cvi_ive_go(ndev, ive_top_c, bInstant, done_mask, optype, dev_id);
				// clear after a tile
				clear_framedone(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90), 0, dev_id);
				clear_interrupt_status(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98), 0, dev_id);


				if (optype == MOD_BGM) {
					stat->u32PixNum +=
						readl(IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
							  IVE_MATCH_BG_REG_10); //9c2fe24
					stat->u32SumLum +=
						readl(IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG +
							  IVE_MATCH_BG_REG_14); //9c2fe24
				} else if (optype == MOD_BGU) {
					stat->u32PixNum +=
						readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							  IVE_UPDATE_BG_REG_CTRL7); //9c2fe24
					stat->u32SumLum +=
						readl(IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG +
							  IVE_UPDATE_BG_REG_CTRL8); //9c2fe24
				}
			}

			if (optype == MOD_GMM || optype == MOD_GMM2) {
				cvi_ive_reset(ndev, 0, dev_id);
			}
			// add read times, for reduce r/w switch.
			writel(rdma_val, IVE_BLK_BA[dev_id].RDMA);

			if (optype == MOD_GMM || optype == MOD_GMM2) {
				for (i = 0; i < ive_gmm_c->REG_GMM_13.reg_gmm_gmm2_model_num;
					 i++) {
					gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_stride_sel = 0;
					gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_stride_sel = 0;
					writel(gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
						ISP_DMA_CTL_SYS_CONTROL));
				}
			}
			gmm_factor_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			gmm_match_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			bgmodel_0_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			bgmodel_1_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			fgflag_rdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			difffg_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			bgmodel_0_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			bgmodel_1_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			fg_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			chg_wdma_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			wdma_y_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			ive_gmm_c->REG_GMM_13.reg_gmm_gmm2_enable = 0;

			writel(gmm_factor_rdma_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_SYS_CONTROL));
			writel(gmm_match_wdma_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_SYS_CONTROL));
			writel(bgmodel_0_rdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_rdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fgflag_rdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(difffg_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_0_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(bgmodel_1_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(fg_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(chg_wdma_ctl_c->SYS_CONTROL.val,
				   IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_SYS_CONTROL);
			writel(wdma_y_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));

			writel(ive_gmm_c->REG_GMM_13.val,
				   (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		}
	//}

	return CVI_SUCCESS;
}

s32 emit_tile(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c,
		 IVE_FILTEROP_C *ive_filterop_c, IMG_IN_C *img_in_c,
		 ISP_DMA_CTL_C *wdma_y_ctl_c, ISP_DMA_CTL_C *rdma_img1_ctl_c,
		 ISP_DMA_CTL_C *wdma_c_ctl_c, ISP_DMA_CTL_C *rdma_eigval_ctl_c,
		 IVE_IMAGE_S *src1, IVE_IMAGE_S *src2, IVE_IMAGE_S *src3,
		 IVE_IMAGE_S *dst1, IVE_IMAGE_S *dst2, bool enWdma_y,
		 s32 y_unit, bool enWdma_c, s32 c_unit, bool enOdma,
		 s32 optype, bool bInstant, s32 dev_id)
{
	/*
	 * Tile x2 Width: 496~ 928 = 480*n-32*(n-1)
	 * Tile x3 Width: 944~1376
	 * Tile x4 Width:1392~1824
	 * Tile x5 Width:1840~2272
	 */
	s32 img1_unit;
	s32 n = 0, round = 0, done_mask = 0;

	s32 tileNum = (src1->u32Width - 32 + 447) / 448;
	s32 remain_width = src1->u32Width + 32 * (tileNum - 1);
	s32 tileLen[6] = { 0 };
	s32 segLen[6] = { 0 };
	s32 inOffset[6] = { 0 };
	s32 outOffset[6] = { 0 };
	s32 cropstart[6] = { 0 };
	s32 cropend[6] = { 0 };
	bool isCanny = (optype == MOD_CANNY);
	IVE_MAP_C *ive_map_c;
	ISP_DMA_CTL_C *rdma_gradfg_ctl_c;
	//DEFINE_IMG_IN_C(_img_in_c);
	IMG_IN_C _img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_MAP_C(_ive_map_c);
	IVE_MAP_C _ive_map_c = _DEFINE_IVE_MAP_C;
	//DEFINE_ISP_DMA_CTL_C(_wdma_y_ctl_c);
	ISP_DMA_CTL_C _wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_wdma_c_ctl_c);
	ISP_DMA_CTL_C _wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_rdma_img1_ctl_c);
	ISP_DMA_CTL_C _rdma_img1_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_rdma_eigval_ctl_c);
	ISP_DMA_CTL_C _rdma_eigval_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(_rdma_gradfg_ctl_c);
	ISP_DMA_CTL_C _rdma_gradfg_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	ive_map_c = &_ive_map_c;
	rdma_gradfg_ctl_c = &_rdma_gradfg_ctl_c;

	if (tileNum > 6)
		return CVI_FAILURE;

	//for (dev_id = 0; dev_id < IVE_DEV_MAX; dev_id++) {
		if (atomic_read(&ndev->core[dev_id].dev_state) == IVE_CORE_STATE_RUNNING) {
			ndev->core[dev_id].total_tile = tileNum - 1;

			if (img_in_c == NULL) {
				img_in_c = &_img_in_c;
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
					CVI_TRACE_IVE(CVI_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n, tileLen[n], tileLen[n]);
				} else {
					//in case last tile too short
					if (remain_width < 64) {
						tileLen[n] = 128;
						tileLen[0] -= (128 - remain_width);
					} else {
						tileLen[n] = remain_width;
					}
					CVI_TRACE_IVE(CVI_DBG_INFO, "\ttileLen[%d]=0x%04x -> %d\n", n,
							 tileLen[n], tileLen[n]);
					break;
				}
			}

			//Inference other array based on tileLen
			for (n = 0; n < tileNum; n++) {
				s32 nFirst_tile = (n == 0) ? 0 : 1;
				s32 nLast_tile = (n == tileNum - 1) ? 0 : 1;

				cropstart[n] = 16 * nFirst_tile;
				cropend[n] = tileLen[n] - 16 * nLast_tile - 1;
				segLen[n] = tileLen[n] - 16 * nFirst_tile - 16 * nLast_tile;
				outOffset[n] = (n == 0) ? 0 : segLen[n - 1];
				inOffset[n] = (n == 1) ? outOffset[n] - 16 : outOffset[n];
			}

			if (optype == MOD_STBOX) {
				ive_filterop_c->REG_ST_EIGVAL_0.reg_st_eigval_tile_num =
					tileNum - 1;
				writel(ive_filterop_c->REG_ST_EIGVAL_0.val,
					   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_0));
			}

			for (round = 0; round < tileNum; round++) {
				ndev->core[dev_id].tile_num = round;
				ive_top_c->REG_2.reg_img_widthm1 = tileLen[round] - 1;
				img_in_c->REG_02.reg_src_wd = tileLen[round] - 1;
				if (src1->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
					img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 += inOffset[round] * 3;
				} else if (src1->enType == IVE_IMAGE_TYPE_YUV420SP) {
					img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 += inOffset[round];
					img_in_c->REG_U_BASE_0.reg_src_u_base_b0 += inOffset[round];
				} else {
					img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 += inOffset[round];

				}
				writel(ive_top_c->REG_2.val,
					   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_2));
				writel(img_in_c->REG_02.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02));
				writel(img_in_c->REG_Y_BASE_0.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_0));
				writel(img_in_c->REG_U_BASE_0.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
				writel(img_in_c->REG_V_BASE_0.val,
					   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
				if (enWdma_y) {
					wdma_y_ctl_c->BASE_ADDR.reg_basel +=
						y_unit * outOffset[round];
					wdma_y_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
					wdma_y_ctl_c->SYS_CONTROL.reg_seglen_sel = 1;
					wdma_y_ctl_c->SYS_CONTROL.reg_segnum_sel = 1;
					wdma_y_ctl_c->DMA_SEGNUM.reg_segnum = src1->u32Height;
					wdma_y_ctl_c->DMA_SEGLEN.reg_seglen =
						segLen[round] * y_unit;
					// wdma_y_ctl_c->DMA_STRIDE.reg_stride =
					// 		dst1->u32Stride[0] * y_unit;
					wdma_y_ctl_c->DMA_STRIDE.reg_stride =
							dst1->u32Stride[0];

						// wdma_y_ctl_c->DMA_STRIDE.reg_stride =
						//	(optype == MOD_STBOX) ?
						//		dst1->u32Stride[0] * y_unit :
						//		dst1->u32Stride[0];

					ive_filterop_c->REG_cropy_s.reg_crop_y_start_x =
						cropstart[round];
					ive_filterop_c->REG_cropy_s.reg_crop_y_end_x =
						cropend[round];
					ive_filterop_c->REG_cropy_e.reg_crop_y_start_y = 0;
					ive_filterop_c->REG_cropy_e.reg_crop_y_end_y =
						src1->u32Height - 1;
					ive_filterop_c->REG_cropy_ctl.reg_crop_y_enable = 1;

					writel(wdma_y_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_BASE_ADDR));
					writel(wdma_y_ctl_c->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(wdma_y_ctl_c->DMA_SEGNUM.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(wdma_y_ctl_c->DMA_SEGLEN.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(wdma_y_ctl_c->DMA_STRIDE.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y +
						ISP_DMA_CTL_DMA_STRIDE));

					writel(ive_filterop_c->REG_cropy_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_E));
					writel(ive_filterop_c->REG_cropy_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_S));
					writel(ive_filterop_c->REG_cropy_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPY_CTL));
				}
				if (enWdma_c) {
					if (isCanny) {
						//It is only for example, still have no good idea to caculate this...
						wdma_c_ctl_c->BASE_ADDR.reg_basel +=
							(round == 0) ? 0 : 0x12000;
						wdma_c_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
						wdma_c_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
						wdma_c_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
						wdma_c_ctl_c->DMA_SEGNUM.reg_segnum =
							src1->u32Height;
						wdma_c_ctl_c->DMA_SEGLEN.reg_seglen =
							segLen[round] * c_unit;
						wdma_c_ctl_c->DMA_STRIDE.reg_stride = 0;
					} else {
						wdma_c_ctl_c->BASE_ADDR.reg_basel +=
							c_unit * outOffset[round];
						wdma_c_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
						wdma_c_ctl_c->SYS_CONTROL.reg_seglen_sel = 1;
						wdma_c_ctl_c->SYS_CONTROL.reg_segnum_sel = 1;
						wdma_c_ctl_c->DMA_SEGNUM.reg_segnum =
							src1->u32Height;
						wdma_c_ctl_c->DMA_SEGLEN.reg_seglen =
							segLen[round] * c_unit;
						// wdma_c_ctl_c->DMA_STRIDE.reg_stride =
						//	dst1->u32Stride[0] * c_unit;
						if (optype == MOD_MAP) {
							wdma_c_ctl_c->DMA_STRIDE.reg_stride =
								dst1->u32Stride[0]; // * c_unit;
						} else {
							wdma_c_ctl_c->DMA_STRIDE.reg_stride =
								dst2->u32Stride[0]; // * c_unit;

						}
					}
					ive_filterop_c->REG_cropc_s.reg_crop_c_start_x =
						cropstart[round];
					ive_filterop_c->REG_cropc_s.reg_crop_c_end_x =
						cropend[round];
					ive_filterop_c->REG_cropc_e.reg_crop_c_start_y = 0;
					ive_filterop_c->REG_cropc_e.reg_crop_c_end_y =
						src1->u32Height - 1;
					ive_filterop_c->REG_cropc_ctl.reg_crop_c_enable = 1;
					writel(wdma_c_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_BASE_ADDR));
					writel(wdma_c_ctl_c->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(wdma_c_ctl_c->DMA_SEGNUM.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(wdma_c_ctl_c->DMA_SEGLEN.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(wdma_c_ctl_c->DMA_STRIDE.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C +
						ISP_DMA_CTL_DMA_STRIDE));
					writel(ive_filterop_c->REG_cropc_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPC_S));
					writel(ive_filterop_c->REG_cropc_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPC_E));
					writel(ive_filterop_c->REG_cropc_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROPC_CTL));
				}
				if (enOdma) {
					ive_filterop_c->ODMA_REG_11.reg_dma_wd =
						segLen[round] - 1;
					writel(ive_filterop_c->ODMA_REG_11.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_11));

					switch (dst1->enType) {
					case IVE_IMAGE_TYPE_YUV420P:
					case IVE_IMAGE_TYPE_YUV422P:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_03
							.reg_dma_u_base_low_part +=
							outOffset[round] / 2;
						ive_filterop_c->ODMA_REG_05
							.reg_dma_v_base_low_part +=
							outOffset[round] / 2;
						break;
					case IVE_IMAGE_TYPE_YUV420SP: // NV21
					case IVE_IMAGE_TYPE_YUV422SP:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						img_in_c->REG_U_BASE_0.reg_src_u_base_b0 +=
							inOffset[round];
						img_in_c->REG_00.reg_auto_csc_en = 0;
						writel(img_in_c->REG_U_BASE_0.val,
							   (IVE_BLK_BA[dev_id].IMG_IN +
							IMG_IN_REG_U_BASE_0));
						writel(img_in_c->REG_00.val,
							   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));
						break;
					case IVE_IMAGE_TYPE_U8C3_PLANAR:
						ive_filterop_c->ODMA_REG_03
							.reg_dma_u_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_05
							.reg_dma_v_base_low_part +=
							outOffset[round];
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C1:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round];
						break;
					case IVE_IMAGE_TYPE_U8C3_PACKAGE:
						ive_filterop_c->ODMA_REG_01
							.reg_dma_y_base_low_part +=
							outOffset[round] * 3;
						// img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 +=
						// 	inOffset[round];
						// writel(img_in_c->REG_Y_BASE_0.val,
						// 	   (IVE_BLK_BA[dev_id].IMG_IN +
						// 	IMG_IN_REG_Y_BASE_0));
						break;
					default:
						break;
					}
					ive_filterop_c->REG_crop_odma_s.reg_crop_odma_start_x =
						cropstart[round];
					ive_filterop_c->REG_crop_odma_s.reg_crop_odma_end_x =
						cropend[round];
					ive_filterop_c->REG_crop_odma_e.reg_crop_odma_start_y =
						0;
					ive_filterop_c->REG_crop_odma_e.reg_crop_odma_end_y =
						src1->u32Height - 1;
					ive_filterop_c->REG_crop_odma_ctl.reg_crop_odma_enable =
						1;

					writel(ive_filterop_c->ODMA_REG_01.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_01));
					writel(ive_filterop_c->ODMA_REG_03.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_03));
					writel(ive_filterop_c->ODMA_REG_05.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_ODMA_REG_05));
					writel(ive_filterop_c->REG_crop_odma_s.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROP_ODMA_S));
					writel(ive_filterop_c->REG_crop_odma_e.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROP_ODMA_E));
					writel(ive_filterop_c->REG_crop_odma_ctl.val,
						   (IVE_BLK_BA[dev_id].FILTEROP +
						IVE_FILTEROP_REG_CROP_ODMA_CTL));
				}
				if ((readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3) &
					 IVE_TOP_REG_IVE_RDMA_IMG1_EN_MASK)) {
					img1_unit = (optype == MOD_GRADFG) ? 2 : 1;
					rdma_img1_ctl_c->BASE_ADDR.reg_basel +=
						inOffset[round] * img1_unit;
					rdma_img1_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
					rdma_img1_ctl_c->SYS_CONTROL.reg_seglen_sel = 1;
					rdma_img1_ctl_c->SYS_CONTROL.reg_segnum_sel = 1;
					rdma_img1_ctl_c->DMA_SEGNUM.reg_segnum =
						src2->u32Height;
					rdma_img1_ctl_c->DMA_SEGLEN.reg_seglen =
						tileLen[round] * img1_unit;
					rdma_img1_ctl_c->DMA_STRIDE.reg_stride =
						src2->u32Stride[0]; // * img1_unit;
						// src2->u32Stride[0] * img1_unit;
					writel(rdma_img1_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR));
					writel(rdma_img1_ctl_c->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(rdma_img1_ctl_c->DMA_SEGNUM.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGNUM));
					writel(rdma_img1_ctl_c->DMA_SEGLEN.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGLEN));
					writel(rdma_img1_ctl_c->DMA_STRIDE.val,
						   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_STRIDE));
				}
				if ((readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3) &
					 IVE_TOP_REG_IVE_RDMA_EIGVAL_EN_MASK)) {
					rdma_eigval_ctl_c->BASE_ADDR.reg_basel +=
						inOffset[round] * 2;
					rdma_eigval_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
					rdma_eigval_ctl_c->SYS_CONTROL.reg_seglen_sel = 1;
					rdma_eigval_ctl_c->SYS_CONTROL.reg_segnum_sel = 1;
					rdma_eigval_ctl_c->DMA_SEGNUM.reg_segnum =
						src1->u32Height;
					rdma_eigval_ctl_c->DMA_SEGLEN.reg_seglen =
						tileLen[round] * 2;
					rdma_eigval_ctl_c->DMA_STRIDE.reg_stride =
						src1->u32Stride[0];

					writel(rdma_eigval_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_BASE_ADDR));
					writel(rdma_eigval_ctl_c->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(rdma_eigval_ctl_c->DMA_SEGNUM.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(rdma_eigval_ctl_c->DMA_SEGLEN.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(rdma_eigval_ctl_c->DMA_STRIDE.val,
						   (IVE_BLK_BA[dev_id].RDMA_EIGVAL +
						ISP_DMA_CTL_DMA_STRIDE));
				}
				if ((readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04) &
					 IVE_FILTEROP_REG_GRADFG_BGGRAD_RDMA_EN_MASK)) {
					rdma_gradfg_ctl_c->BASE_ADDR.reg_basel +=
						inOffset[round] * 2;
					rdma_gradfg_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
					rdma_gradfg_ctl_c->SYS_CONTROL.reg_seglen_sel = 1;
					rdma_gradfg_ctl_c->SYS_CONTROL.reg_segnum_sel = 1;
					rdma_gradfg_ctl_c->DMA_SEGNUM.reg_segnum =
						src3->u32Height;
					rdma_gradfg_ctl_c->DMA_SEGLEN.reg_seglen =
						tileLen[round] * 2;
					rdma_gradfg_ctl_c->DMA_STRIDE.reg_stride =
						src3->u32Stride[0];

					writel(rdma_gradfg_ctl_c->BASE_ADDR.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_BASE_ADDR));
					writel(rdma_gradfg_ctl_c->SYS_CONTROL.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_SYS_CONTROL));
					writel(rdma_gradfg_ctl_c->DMA_SEGNUM.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_DMA_SEGNUM));
					writel(rdma_gradfg_ctl_c->DMA_SEGLEN.val,
						   (IVE_BLK_BA[dev_id].FILTEROP_RDMA +
						ISP_DMA_CTL_DMA_SEGLEN));
					writel(rdma_gradfg_ctl_c->DMA_STRIDE.val,
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

				cvi_ive_go(ndev, ive_top_c, bInstant, done_mask, optype, dev_id);

				// clear after a tile
				clear_framedone(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_90), 0, dev_id);
				clear_interrupt_status(readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_98), 0, dev_id);

			}

			img_in_c->REG_068.reg_ip_clr_w1t = 1;
			writel(img_in_c->REG_068.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
			// ive_filterop_c->REG_1.val = 0xff;
			//udelay(3);
			wdma_y_ctl_c->BASE_ADDR.reg_basel = 0;
			wdma_y_ctl_c->SYS_CONTROL.reg_base_sel = 0;
			wdma_y_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			wdma_y_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
			wdma_y_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
			wdma_y_ctl_c->DMA_SEGNUM.reg_segnum = 0;
			wdma_y_ctl_c->DMA_SEGLEN.reg_seglen = 0;
			wdma_y_ctl_c->DMA_STRIDE.reg_stride = 0;
			wdma_c_ctl_c->BASE_ADDR.reg_basel = 0;
			wdma_c_ctl_c->SYS_CONTROL.reg_base_sel = 0;
			wdma_c_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			wdma_c_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
			wdma_c_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
			wdma_c_ctl_c->DMA_SEGNUM.reg_segnum = 0;
			wdma_c_ctl_c->DMA_SEGLEN.reg_seglen = 0;
			wdma_c_ctl_c->DMA_STRIDE.reg_stride = 0;
			ive_filterop_c->REG_cropy_s.reg_crop_y_start_x = 0;
			ive_filterop_c->REG_cropy_s.reg_crop_y_end_x = 0;
			ive_filterop_c->REG_cropy_e.reg_crop_y_start_y = 0;
			ive_filterop_c->REG_cropy_e.reg_crop_y_end_y = 0;
			ive_filterop_c->REG_cropy_ctl.reg_crop_y_enable = 0;
			ive_filterop_c->REG_cropc_s.reg_crop_c_start_x = 0;
			ive_filterop_c->REG_cropc_s.reg_crop_c_end_x = 0;
			ive_filterop_c->REG_cropc_e.reg_crop_c_start_y = 0;
			ive_filterop_c->REG_cropc_e.reg_crop_c_end_y = 0;
			ive_filterop_c->REG_cropc_ctl.reg_crop_c_enable = 0;
			ive_filterop_c->REG_crop_odma_s.reg_crop_odma_start_x = 0;
			ive_filterop_c->REG_crop_odma_s.reg_crop_odma_end_x = 0;
			ive_filterop_c->REG_crop_odma_e.reg_crop_odma_start_y = 0;
			ive_filterop_c->REG_crop_odma_e.reg_crop_odma_end_y = 0;
			ive_filterop_c->REG_crop_odma_ctl.reg_crop_odma_enable = 0;
			ive_filterop_c->ODMA_REG_01.reg_dma_y_base_low_part = 0;
			ive_filterop_c->ODMA_REG_02.reg_dma_y_base_high_part = 0;
			ive_filterop_c->ODMA_REG_03.reg_dma_u_base_low_part = 0;
			ive_filterop_c->ODMA_REG_04.reg_dma_u_base_high_part = 0;
			ive_filterop_c->ODMA_REG_05.reg_dma_v_base_low_part = 0;
			ive_filterop_c->ODMA_REG_06.reg_dma_v_base_high_part = 0;
			ive_filterop_c->ODMA_REG_07.reg_dma_y_pitch = 0;
			ive_filterop_c->ODMA_REG_08.reg_dma_c_pitch = 0;
			rdma_eigval_ctl_c->BASE_ADDR.reg_basel = 0;
			rdma_eigval_ctl_c->SYS_CONTROL.reg_base_sel = 0;
			rdma_eigval_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			rdma_eigval_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
			rdma_eigval_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
			rdma_eigval_ctl_c->DMA_SEGNUM.reg_segnum = 0;
			rdma_eigval_ctl_c->DMA_SEGLEN.reg_seglen = 0;
			rdma_eigval_ctl_c->DMA_STRIDE.reg_stride = 0;
			rdma_img1_ctl_c->BASE_ADDR.reg_basel = 0;
			rdma_img1_ctl_c->SYS_CONTROL.reg_base_sel = 0;
			rdma_img1_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
			rdma_img1_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
			rdma_img1_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
			rdma_img1_ctl_c->DMA_SEGNUM.reg_segnum = 0;
			rdma_img1_ctl_c->DMA_SEGLEN.reg_seglen = 0;
			rdma_img1_ctl_c->DMA_STRIDE.reg_stride = 0;
			img_in_c->REG_068.reg_ip_clr_w1t = 0;
			ive_map_c->REG_0.reg_ip_enable = 0;
			ive_filterop_c->REG_CANNY_1.reg_canny_en = 0;
			ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
			ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
			writel(wdma_y_ctl_c->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_BASE_ADDR));
			writel(wdma_y_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));
			writel(wdma_y_ctl_c->DMA_SEGNUM.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGNUM));
			writel(wdma_y_ctl_c->DMA_SEGLEN.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGLEN));
			writel(wdma_y_ctl_c->DMA_STRIDE.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_STRIDE));
			writel(wdma_c_ctl_c->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_BASE_ADDR));
			writel(wdma_c_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_SYS_CONTROL));
			writel(wdma_c_ctl_c->DMA_SEGNUM.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_SEGNUM));
			writel(wdma_c_ctl_c->DMA_SEGLEN.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_SEGLEN));
			writel(wdma_c_ctl_c->DMA_STRIDE.val,
				   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_STRIDE));
			writel(ive_filterop_c->REG_cropy_s.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPY_S));
			writel(ive_filterop_c->REG_cropy_e.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPY_E));
			writel(ive_filterop_c->REG_cropy_ctl.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPY_CTL));
			writel(ive_filterop_c->REG_cropc_s.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPC_S));
			writel(ive_filterop_c->REG_cropc_e.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPC_E));
			writel(ive_filterop_c->REG_cropc_ctl.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROPC_CTL));
			writel(ive_filterop_c->REG_crop_odma_s.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROP_ODMA_S));
			writel(ive_filterop_c->REG_crop_odma_e.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROP_ODMA_E));
			writel(ive_filterop_c->REG_crop_odma_ctl.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CROP_ODMA_CTL));
			writel(ive_filterop_c->ODMA_REG_01.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_01));
			writel(ive_filterop_c->ODMA_REG_02.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_02));
			writel(ive_filterop_c->ODMA_REG_03.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_03));
			writel(ive_filterop_c->ODMA_REG_04.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_04));
			writel(ive_filterop_c->ODMA_REG_05.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_05));
			writel(ive_filterop_c->ODMA_REG_06.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_06));
			writel(ive_filterop_c->ODMA_REG_07.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_07));
			writel(ive_filterop_c->ODMA_REG_08.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_08));
			writel(rdma_eigval_ctl_c->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_BASE_ADDR));
			writel(rdma_eigval_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_SYS_CONTROL));
			writel(rdma_eigval_ctl_c->DMA_SEGNUM.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_SEGNUM));
			writel(rdma_eigval_ctl_c->DMA_SEGLEN.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_SEGLEN));
			writel(rdma_eigval_ctl_c->DMA_STRIDE.val,
				   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_STRIDE));
			writel(rdma_img1_ctl_c->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR));
			writel(rdma_img1_ctl_c->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_SYS_CONTROL));
			writel(rdma_img1_ctl_c->DMA_SEGNUM.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGNUM));
			writel(rdma_img1_ctl_c->DMA_SEGLEN.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_SEGLEN));
			writel(rdma_img1_ctl_c->DMA_STRIDE.val,
				   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_STRIDE));
			writel(img_in_c->REG_068.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
			writel(ive_map_c->REG_0.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_0));
			writel(ive_filterop_c->REG_CANNY_1.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_1));
			writel(ive_filterop_c->REG_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
			if (optype == MOD_GRADFG) {
				rdma_gradfg_ctl_c->BASE_ADDR.reg_basel = 0;
				rdma_gradfg_ctl_c->SYS_CONTROL.reg_base_sel = 0;
				rdma_gradfg_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
				rdma_gradfg_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
				rdma_gradfg_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
				rdma_gradfg_ctl_c->DMA_SEGNUM.reg_segnum = 0;
				rdma_gradfg_ctl_c->DMA_SEGLEN.reg_seglen = 0;
				rdma_gradfg_ctl_c->DMA_STRIDE.reg_stride = 0;
				ive_filterop_c->REG_33.reg_filterop_op2_gradfg_en = 0;
				ive_filterop_c->REG_H04.reg_gradfg_bggrad_rdma_en = 0;
				ive_top_c->REG_3.reg_muxsel_gradfg = 0;

				writel(rdma_gradfg_ctl_c->BASE_ADDR.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_BASE_ADDR));
				writel(rdma_gradfg_ctl_c->SYS_CONTROL.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_SYS_CONTROL));
				writel(rdma_gradfg_ctl_c->DMA_SEGNUM.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGNUM));
				writel(rdma_gradfg_ctl_c->DMA_SEGLEN.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGLEN));
				writel(rdma_gradfg_ctl_c->DMA_STRIDE.val,
					   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_STRIDE));
				writel(ive_filterop_c->REG_33.val,
					   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_33));
				writel(ive_filterop_c->REG_H04.val,
					   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
				writel(ive_top_c->REG_3.val,
					   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
			}
		}
	//}
	return CVI_SUCCESS;
}

void ive_set_int(IVE_TOP_C *ive_top_c, bool isEnable, s32 dev_id)
{
	ive_top_c->REG_94.reg_intr_en_hist = isEnable;
	ive_top_c->REG_94.reg_intr_en_intg = isEnable;
	ive_top_c->REG_94.reg_intr_en_sad = isEnable;
	ive_top_c->REG_94.reg_intr_en_ncc = isEnable;
	ive_top_c->REG_94.reg_intr_en_filterop_odma = isEnable;
	ive_top_c->REG_94.reg_intr_en_filterop_wdma_y = isEnable;
	ive_top_c->REG_94.reg_intr_en_filterop_wdma_c = isEnable;
	ive_top_c->REG_94.reg_intr_en_dmaf = isEnable;
	ive_top_c->REG_94.reg_intr_en_ccl = isEnable;
	ive_top_c->REG_94.reg_intr_en_lk = isEnable;
	writel(ive_top_c->REG_94.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_94));
}

void ive_reset(struct cvi_ive_device *ndev, IVE_TOP_C *ive_top_c, s32 dev_id)
{
	s32 i = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;

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
			ive_top_c->REG_3.reg_imgmux_img0_sel = 1;
			ive_top_c->REG_3.reg_ive_rdma_img1_en = 0;
			ive_top_c->REG_3.reg_mapmux_rdma_sel = 0;
			ive_top_c->REG_3.reg_ive_rdma_eigval_en = 0;
			// default disable it
			ive_top_c->REG_3.reg_dma_share_mux_selgmm = 0;
			ive_top_c->REG_h10.reg_img_in_top_enable = 0;
			ive_top_c->REG_h10.reg_resize_top_enable = 0;
			ive_top_c->REG_h10.reg_gmm_top_enable = 0;
			ive_top_c->REG_h10.reg_csc_top_enable = 0;
			ive_top_c->REG_h10.reg_rdma_img1_top_enable = 0;
			ive_top_c->REG_h10.reg_bgm_top_enable = 0;
			ive_top_c->REG_h10.reg_bgu_top_enable = 0;
			ive_top_c->REG_h10.reg_r2y4_top_enable = 0;
			ive_top_c->REG_h10.reg_map_top_enable = 0;
			ive_top_c->REG_h10.reg_rdma_eigval_top_enable = 0;
			ive_top_c->REG_h10.reg_thresh_top_enable = 0;
			ive_top_c->REG_h10.reg_hist_top_enable = 0;
			ive_top_c->REG_h10.reg_intg_top_enable = 0;
			ive_top_c->REG_h10.reg_ncc_top_enable = 0;
			ive_top_c->REG_h10.reg_sad_top_enable = 0;
			ive_top_c->REG_h10.reg_filterop_top_enable = 0;
			ive_top_c->REG_h10.reg_dmaf_top_enable = 0;
			ive_top_c->REG_h10.reg_ccl_top_enable = 0;
			ive_top_c->REG_h10.reg_lk_top_enable = 0;
			writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
			writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

			ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
			ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
			ive_filterop_c.ODMA_REG_00.reg_dma_en = 0;
			writel(ive_filterop_c.REG_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
			writel(ive_filterop_c.ODMA_REG_00.val,
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

void ive_set_wh(IVE_TOP_C *top, u32 w, u32 h, char *name, s32 dev_id)
{
	top->REG_2.reg_img_heightm1 = h - 1;
	top->REG_2.reg_img_widthm1 = w - 1;
	writel(top->REG_2.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_2));

	strcpy(g_debug_info.op_name, name);
	g_debug_info.src_w = top->REG_2.reg_img_widthm1;
	g_debug_info.src_h = top->REG_2.reg_img_heightm1;
}

s32 set_img_dst1(IVE_DST_IMAGE_S *dst_img, ISP_DMA_CTL_C *wdma_y_ctl_c, s32 dev_id)
{
	// s32 swMode = 0;
	s32 swMode = 0;
	//DEFINE_ISP_DMA_CTL_C(_wdma_y_ctl_c);
	ISP_DMA_CTL_C _wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	if (wdma_y_ctl_c == NULL) {
		wdma_y_ctl_c = &_wdma_y_ctl_c;
	}
	if (dst_img == NULL) {
		wdma_y_ctl_c->BASE_ADDR.reg_basel = 0;
		wdma_y_ctl_c->SYS_CONTROL.reg_baseh = 0;
		wdma_y_ctl_c->SYS_CONTROL.reg_base_sel = 0;
		writel(wdma_y_ctl_c->BASE_ADDR.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_BASE_ADDR));
		writel(wdma_y_ctl_c->SYS_CONTROL.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));

		return CVI_SUCCESS;
	}
	wdma_y_ctl_c->BASE_ADDR.reg_basel = dst_img->u64PhyAddr[0] & 0xffffffff;
	writel(wdma_y_ctl_c->BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_BASE_ADDR));

	wdma_y_ctl_c->SYS_CONTROL.reg_baseh =
		(dst_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	wdma_y_ctl_c->SYS_CONTROL.reg_stride_sel = swMode;
	wdma_y_ctl_c->SYS_CONTROL.reg_seglen_sel = swMode;
	wdma_y_ctl_c->SYS_CONTROL.reg_segnum_sel = swMode;

	if (swMode) {
		// set height
		wdma_y_ctl_c->DMA_SEGNUM.reg_segnum = dst_img->u32Height;
		writel(wdma_y_ctl_c->DMA_SEGNUM.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGNUM));
		// set width
		wdma_y_ctl_c->DMA_SEGLEN.reg_seglen = dst_img->u32Width;
		writel(wdma_y_ctl_c->DMA_SEGLEN.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_SEGLEN));
		// set stride
		wdma_y_ctl_c->DMA_STRIDE.reg_stride = dst_img->u32Stride[0];
		writel(wdma_y_ctl_c->DMA_STRIDE.val,
			   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_STRIDE));
		// FIXME: set U8/S8
	} else {
		// hw mode, no need to set height/width/stride
		// wdma_y_ctl_c->DMA_STRIDE.reg_stride = dst_img->u32Stride[0];
		// writel(wdma_y_ctl_c->DMA_STRIDE.val,
		// 	  (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_DMA_STRIDE));
	}
	wdma_y_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
	wdma_y_ctl_c->SYS_CONTROL.reg_base_sel = 1; // sw specify address
	writel(wdma_y_ctl_c->SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_Y + ISP_DMA_CTL_SYS_CONTROL));
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Dst1 address: 0x%08x %08x\n",
			wdma_y_ctl_c->SYS_CONTROL.reg_baseh,
			wdma_y_ctl_c->BASE_ADDR.reg_basel);
	}

	g_debug_info.addr[WDMA_Y].addr_en = wdma_y_ctl_c->SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_Y].addr_l = wdma_y_ctl_c->BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_Y].addr_h = wdma_y_ctl_c->SYS_CONTROL.reg_baseh & 0xff;

	return CVI_SUCCESS;
}

s32 set_img_dst2(IVE_DST_IMAGE_S *dst_img, ISP_DMA_CTL_C *wdma_c_ctl_c, s32 dev_id)
{
	//DEFINE_ISP_DMA_CTL_C(_wdma_c_ctl_c);
	ISP_DMA_CTL_C _wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	if (wdma_c_ctl_c == NULL) {
		wdma_c_ctl_c = &_wdma_c_ctl_c;
	}

	if (dst_img != NULL) {
		wdma_c_ctl_c->BASE_ADDR.reg_basel =
			dst_img->u64PhyAddr[0] & 0xffffffff;
		wdma_c_ctl_c->SYS_CONTROL.reg_baseh =
			(dst_img->u64PhyAddr[0] >> 32) & 0xffffffff;
		// need to set stride ??
		// wdma_c_ctl_c->DMA_STRIDE.reg_stride = dst_img->u32Stride[0];
		writel(wdma_c_ctl_c->DMA_STRIDE.val,
			(IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_DMA_STRIDE));
		wdma_c_ctl_c->SYS_CONTROL.reg_stride_sel = 0;
		wdma_c_ctl_c->SYS_CONTROL.reg_base_sel = 1;
		if (g_dump_dma_info == CVI_TRUE) {
			pr_info("Dst2 address: 0x%08x %08x\n",
				wdma_c_ctl_c->SYS_CONTROL.reg_baseh,
				wdma_c_ctl_c->BASE_ADDR.reg_basel);
		}
	} else {
		wdma_c_ctl_c->BASE_ADDR.reg_basel = 0;
		wdma_c_ctl_c->SYS_CONTROL.reg_baseh = 0;
		wdma_c_ctl_c->SYS_CONTROL.reg_base_sel = 0;
	}
	writel(wdma_c_ctl_c->BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_c_ctl_c->SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_WDMA_C + ISP_DMA_CTL_SYS_CONTROL));

	g_debug_info.addr[WDMA_C].addr_en = wdma_c_ctl_c->SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_C].addr_l = wdma_c_ctl_c->BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_C].addr_h = wdma_c_ctl_c->SYS_CONTROL.reg_baseh & 0xff;
	return CVI_SUCCESS;
}

s32 set_img_src1(IVE_SRC_IMAGE_S *src_img, IMG_IN_C *img_in_c, IVE_TOP_C *ive_top_c, s32 dev_id)
{
	//test2:reset image in ip before input
	img_in_c->REG_068.reg_ip_clr_w1t = 1;
    writel(img_in_c->REG_068.val,(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
    udelay(10);
    img_in_c->REG_068.reg_ip_clr_w1t = 0;
    writel(img_in_c->REG_068.val,(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));


	ive_top_c->REG_h10.reg_img_in_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	// Burst len unit is byte, 8 means 128bit
	img_in_c->REG_00.reg_burst_ln = 8;
	img_in_c->REG_00.reg_src_sel = 2; // 2 for others: DRMA
	img_in_c->REG_00.reg_fmt_sel = get_img_fmt_sel(src_img->enType);
	writel(img_in_c->REG_00.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

	// set outstanding to 16
	img_in_c->REG_064.reg_os_max = 0xf;
	writel(img_in_c->REG_064.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_064));

	img_in_c->REG_03.reg_src_y_pitch = src_img->u32Stride[0]; //0x160 ok
	writel(img_in_c->REG_03.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_03));
	// NOTICE: need to set 1 to valify settings immediantly
	// img_in_c->REG_05.reg_shrd_sel = 0;
	img_in_c->REG_05.reg_shrd_sel = 1;
	writel(img_in_c->REG_05.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_05));
	img_in_c->REG_02.reg_src_wd = src_img->u32Width - 1;
	img_in_c->REG_02.reg_src_ht = src_img->u32Height - 1;
	// img_in_c->REG_02.val = (src_img->u32Height<<16) | src_img->u32Width;
	writel(img_in_c->REG_02.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02));



	img_in_c->REG_Y_BASE_0.reg_src_y_base_b0 =
		(src_img->u64PhyAddr[0] & 0xffffffff);
	writel(img_in_c->REG_Y_BASE_0.val,
		   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_0));
	img_in_c->REG_Y_BASE_1.reg_src_y_base_b1 =
		(src_img->u64PhyAddr[0] >> 32 & 0xffffffff);
	writel(img_in_c->REG_Y_BASE_1.val,
		   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_Y_BASE_1));
	switch (src_img->enType) {
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U8C1:
		break;
	// api already stride[0] * 3
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		img_in_c->REG_03.reg_src_y_pitch = src_img->u32Stride[0];
		writel(img_in_c->REG_03.val,
				(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_03));
		img_in_c->REG_04.reg_src_c_pitch = 0;
		writel(img_in_c->REG_04.val,
				(IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->REG_U_BASE_0.reg_src_u_base_b0 = 0;
		writel(img_in_c->REG_U_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->REG_U_BASE_1.reg_src_u_base_b1 = 0;
		writel(img_in_c->REG_U_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->REG_V_BASE_0.reg_src_v_base_b0 = 0;
		writel(img_in_c->REG_V_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->REG_V_BASE_1.reg_src_v_base_b1 = 0;
		writel(img_in_c->REG_V_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		img_in_c->REG_04.reg_src_c_pitch = src_img->u32Stride[0];
		writel(img_in_c->REG_04.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->REG_U_BASE_0.reg_src_u_base_b0 =
			(src_img->u64PhyAddr[1] & 0xffffffff);
		writel(img_in_c->REG_U_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->REG_U_BASE_1.reg_src_u_base_b1 =
			((src_img->u64PhyAddr[1] >> 32) & 0xffffffff);
		writel(img_in_c->REG_U_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->REG_V_BASE_0.reg_src_v_base_b0 =
			(src_img->u64PhyAddr[2] & 0xffffffff);
		writel(img_in_c->REG_V_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->REG_V_BASE_1.reg_src_v_base_b1 =
			((src_img->u64PhyAddr[2] >> 32) & 0xffffffff);
		writel(img_in_c->REG_V_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;

	case IVE_IMAGE_TYPE_YUV420SP:
		img_in_c->REG_04.reg_src_c_pitch = src_img->u32Stride[1];
		writel(img_in_c->REG_04.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->REG_U_BASE_0.reg_src_u_base_b0 =
			(src_img->u64PhyAddr[1] & 0xffffffff);
		writel(img_in_c->REG_U_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->REG_U_BASE_1.reg_src_u_base_b1 =
			((src_img->u64PhyAddr[1] >> 32) & 0xffffffff);
		writel(img_in_c->REG_U_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->REG_V_BASE_0.reg_src_v_base_b0 =
			((src_img->u64PhyAddr[1] + 1) & 0xffffffff);
		writel(img_in_c->REG_V_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->REG_V_BASE_1.reg_src_v_base_b1 =
			(((src_img->u64PhyAddr[1] + 1) >> 32) & 0xffffffff);
		writel(img_in_c->REG_V_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		img_in_c->REG_04.reg_src_c_pitch = src_img->u32Stride[0];
		writel(img_in_c->REG_04.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_04));
		img_in_c->REG_U_BASE_0.reg_src_u_base_b0 =
			(src_img->u64PhyAddr[1] & 0xffffffff);
		writel(img_in_c->REG_U_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_0));
		img_in_c->REG_U_BASE_1.reg_src_u_base_b1 =
			((src_img->u64PhyAddr[1] >> 32) & 0xffffffff);
		writel(img_in_c->REG_U_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_U_BASE_1));
		img_in_c->REG_V_BASE_0.reg_src_v_base_b0 =
			((src_img->u64PhyAddr[1] + 1) & 0xffffffff);
		writel(img_in_c->REG_V_BASE_0.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_0));
		img_in_c->REG_V_BASE_1.reg_src_v_base_b1 =
			(((src_img->u64PhyAddr[1] + 1) >> 32) & 0xffffffff);
		writel(img_in_c->REG_V_BASE_1.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_V_BASE_1));
		break;
	default:
		pr_err("[IVE] not support src type\n");
		return CVI_FAILURE;
	}
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Src1 img_in_c address y: 0x%08x %08x\n",
				img_in_c->REG_Y_BASE_1.val, img_in_c->REG_Y_BASE_0.val);
	}
	g_debug_info.src_fmt = img_in_c->REG_00.reg_fmt_sel;
	g_debug_info.addr[RDMA_IMG_IN].addr_en = true;
	g_debug_info.addr[RDMA_IMG_IN].addr_l = img_in_c->REG_Y_BASE_0.reg_src_y_base_b0;
	g_debug_info.addr[RDMA_IMG_IN].addr_h = img_in_c->REG_Y_BASE_1.reg_src_y_base_b1 & 0xff;
	return CVI_SUCCESS;
}

s32 set_img_src2(IVE_SRC_IMAGE_S *src_img, ISP_DMA_CTL_C *rdma_img1_ctl_c, s32 dev_id)
{
	//DEFINE_ISP_DMA_CTL_C(_rdma_img1_ctl_c);
	ISP_DMA_CTL_C _rdma_img1_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	if (rdma_img1_ctl_c == NULL) {
		rdma_img1_ctl_c = &_rdma_img1_ctl_c;
	}
	rdma_img1_ctl_c->BASE_ADDR.reg_basel =
		src_img->u64PhyAddr[0] & 0xffffffff;
	writel(rdma_img1_ctl_c->BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_BASE_ADDR));

	rdma_img1_ctl_c->SYS_CONTROL.reg_baseh =
		(src_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	//rdma_img1_ctl_c->DMA_STRIDE.reg_stride = src_img->u32Stride[0];
	//rdma_img1_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
	rdma_img1_ctl_c->SYS_CONTROL.reg_base_sel = 1;
	writel(rdma_img1_ctl_c->DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_DMA_STRIDE));
	writel(rdma_img1_ctl_c->SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].RDMA_IMG1 + ISP_DMA_CTL_SYS_CONTROL));
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Src2 address: 0x%08x %08x\n",
			rdma_img1_ctl_c->SYS_CONTROL.reg_baseh,
			rdma_img1_ctl_c->BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[RDMA_IMG1].addr_en = rdma_img1_ctl_c->SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_IMG1].addr_l = rdma_img1_ctl_c->BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_IMG1].addr_h = rdma_img1_ctl_c->SYS_CONTROL.reg_baseh & 0xff;
	return CVI_SUCCESS;
}

s32 set_isp_rdma(s32 optype, ISP_RDMA_C *rdma_c, s32 dev_id)
{
	ISP_RDMA_C _rdma_c = _DEFINE_ISP_RDMA_C;
	if (rdma_c == NULL) {
		rdma_c = &_rdma_c;
	}

	if (optype == MOD_GMM || optype == MOD_GMM2 || optype == MOD_BGM || optype == MOD_BGU) {
		// set ostanding to 8
		rdma_c->SHADOW_RD_SEL.reg_max_ostd = 0x8;
		rdma_c->SHADOW_RD_SEL.reg_ostd_sw_en = 1;
	}
	else {
		// set ostanding to 16
		rdma_c->SHADOW_RD_SEL.reg_max_ostd = 0x10;
		rdma_c->SHADOW_RD_SEL.reg_ostd_sw_en = 1;
	}
	writel(rdma_c->SHADOW_RD_SEL.val,
		   (IVE_BLK_BA[dev_id].RDMA + ISP_RDMA_SHADOW_RD_SEL));

	return CVI_SUCCESS;
}


s32 set_rdma_eigval(IVE_SRC_IMAGE_S *pstSrc, ISP_DMA_CTL_C *rdma_eigval_ctl_c, s32 dev_id)
{
	//DEFINE_ISP_DMA_CTL_C(_rdma_eigval_ctl_c);
	ISP_DMA_CTL_C _rdma_eigval_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	if (rdma_eigval_ctl_c == NULL) {
		rdma_eigval_ctl_c = &_rdma_eigval_ctl_c;
	}

	rdma_eigval_ctl_c->BASE_ADDR.reg_basel =
		pstSrc->u64PhyAddr[0] & 0xffffffff;
	rdma_eigval_ctl_c->SYS_CONTROL.reg_baseh =
		(pstSrc->u64PhyAddr[0] >> 32) & 0xffffffff;
	rdma_eigval_ctl_c->DMA_STRIDE.reg_stride = pstSrc->u32Stride[0];
	writel(rdma_eigval_ctl_c->DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_DMA_STRIDE));
	rdma_eigval_ctl_c->SYS_CONTROL.reg_base_sel = 1;

	rdma_eigval_ctl_c->SYS_CONTROL.reg_stride_sel = 1;
	rdma_eigval_ctl_c->SYS_CONTROL.reg_seglen_sel = 0;
	rdma_eigval_ctl_c->SYS_CONTROL.reg_segnum_sel = 0;
	writel(rdma_eigval_ctl_c->BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_eigval_ctl_c->SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].RDMA_EIGVAL + ISP_DMA_CTL_SYS_CONTROL));
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("RdmaEigval address: 0x%08x %08x\n",
			rdma_eigval_ctl_c->SYS_CONTROL.reg_baseh,
			rdma_eigval_ctl_c->BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[RDMA_EIGVAL].addr_en = rdma_eigval_ctl_c->SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_EIGVAL].addr_l = rdma_eigval_ctl_c->BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_EIGVAL].addr_h = rdma_eigval_ctl_c->SYS_CONTROL.reg_baseh & 0xff;
	return CVI_SUCCESS;
}

s32 set_odma(IVE_SRC_IMAGE_S *dst_img, IVE_FILTEROP_C *ive_filterop_c, s32 w,
		s32 h, s32 dev_id)
{
	ive_filterop_c->ODMA_REG_00.reg_fmt_sel = get_img_fmt_sel(dst_img->enType);
	writel(ive_filterop_c->ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	switch (dst_img->enType) {
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV422P:
		ive_filterop_c->ODMA_REG_03.reg_dma_u_base_low_part =
			dst_img->u64PhyAddr[1] & 0xffffffff;
		ive_filterop_c->ODMA_REG_04.reg_dma_u_base_high_part =
			(dst_img->u64PhyAddr[1] >> 32) & 0xffffffff;
		ive_filterop_c->ODMA_REG_05.reg_dma_v_base_low_part =
			(dst_img->u64PhyAddr[2]) & 0xffffffff;
		ive_filterop_c->ODMA_REG_06.reg_dma_v_base_high_part =
			((dst_img->u64PhyAddr[2]) >> 32) & 0xffffffff;
		ive_filterop_c->ODMA_REG_08.reg_dma_c_pitch =
			dst_img->u32Stride[1];
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		// NV21
	case IVE_IMAGE_TYPE_YUV422SP:
		ive_filterop_c->ODMA_REG_03.reg_dma_u_base_low_part =
			dst_img->u64PhyAddr[1] & 0xffffffff;
		ive_filterop_c->ODMA_REG_04.reg_dma_u_base_high_part =
			(dst_img->u64PhyAddr[1] >> 32) & 0xffffffff;
		ive_filterop_c->ODMA_REG_05.reg_dma_v_base_low_part =
			(dst_img->u64PhyAddr[1] + 1) & 0xffffffff;
		ive_filterop_c->ODMA_REG_06.reg_dma_v_base_high_part =
			((dst_img->u64PhyAddr[1] + 1) >> 32) & 0xffffffff;
		ive_filterop_c->ODMA_REG_08.reg_dma_c_pitch =
			dst_img->u32Stride[0];
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		ive_filterop_c->ODMA_REG_03.reg_dma_u_base_low_part =
			dst_img->u64PhyAddr[1] & 0xffffffff;
		ive_filterop_c->ODMA_REG_04.reg_dma_u_base_high_part =
			(dst_img->u64PhyAddr[1] >> 32) & 0xffffffff;
		ive_filterop_c->ODMA_REG_05.reg_dma_v_base_low_part =
			(dst_img->u64PhyAddr[2]) & 0xffffffff;
		ive_filterop_c->ODMA_REG_06.reg_dma_v_base_high_part =
			((dst_img->u64PhyAddr[2]) >> 32) & 0xffffffff;
		ive_filterop_c->ODMA_REG_08.reg_dma_c_pitch =
			dst_img->u32Stride[0];
		break;
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		ive_filterop_c->ODMA_REG_03.reg_dma_u_base_low_part = 0;
		ive_filterop_c->ODMA_REG_04.reg_dma_u_base_high_part = 0;
		ive_filterop_c->ODMA_REG_05.reg_dma_v_base_low_part = 0;
		ive_filterop_c->ODMA_REG_06.reg_dma_v_base_high_part = 0;
		ive_filterop_c->ODMA_REG_08.reg_dma_c_pitch = 0;
		break;
	default:
		pr_err("[IVE] not support dstEnType");
		return CVI_FAILURE;
	}
	writel(ive_filterop_c->ODMA_REG_03.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_03));
	writel(ive_filterop_c->ODMA_REG_04.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_04));
	writel(ive_filterop_c->ODMA_REG_05.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_05));
	writel(ive_filterop_c->ODMA_REG_06.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_06));
	writel(ive_filterop_c->ODMA_REG_08.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_08));
	// odma for filter3ch + csc
	ive_filterop_c->ODMA_REG_01.reg_dma_y_base_low_part =
		dst_img->u64PhyAddr[0] & 0xffffffff;
	writel(ive_filterop_c->ODMA_REG_01.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_01));
	ive_filterop_c->ODMA_REG_02.reg_dma_y_base_high_part =
		(dst_img->u64PhyAddr[0] >> 32) & 0xffffffff;
	writel(ive_filterop_c->ODMA_REG_02.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_02));

	ive_filterop_c->ODMA_REG_07.reg_dma_y_pitch = dst_img->u32Stride[0];
	writel(ive_filterop_c->ODMA_REG_07.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_07));
	// api already * 3, we do not need to do this
	/*
	 * if (dst_img->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
	 *	ive_filterop_c->ODMA_REG_07.reg_dma_y_pitch = 3 * dst_img->u32Stride[0];
	 *}
	 */
	ive_filterop_c->ODMA_REG_11.reg_dma_wd = w - 1;
	ive_filterop_c->ODMA_REG_12.reg_dma_ht = h - 1;
	writel(ive_filterop_c->ODMA_REG_11.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_11));
	writel(ive_filterop_c->ODMA_REG_12.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_12));
	// trigger odma
	ive_filterop_c->ODMA_REG_00.reg_dma_blen = 1;
	ive_filterop_c->ODMA_REG_00.reg_dma_en = 1;
	writel(ive_filterop_c->ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	// enable it
	/*ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
	 * ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	 * writel(ive_filterop_c->REG_h14.val,
	 *     (IVE_BLK_BA.FILTEROP + IVE_FILTEROP_REG_H14));
	 */
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Odma address: 0x%08x %08x (%d %d, s=%d)\n",
				ive_filterop_c->ODMA_REG_02.val,
				ive_filterop_c->ODMA_REG_01.val, w, h,
				dst_img->u32Stride[0]);
	}
	g_debug_info.dst_fmt = ive_filterop_c->ODMA_REG_00.reg_fmt_sel;
	g_debug_info.addr[WDMA_ODMA].addr_en = true;
	g_debug_info.addr[WDMA_ODMA].addr_l = ive_filterop_c->ODMA_REG_01.reg_dma_y_base_low_part;
	g_debug_info.addr[WDMA_ODMA].addr_h = ive_filterop_c->ODMA_REG_02.reg_dma_y_base_high_part & 0xff;
	return CVI_SUCCESS;
}

s32 cvi_ive_base_op(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			bool bInstant, s32 op, void *pstCtrl, s32 dev_id)
{
	s32 ret = 0;
	s32 mode = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_img1_ctl_c);
	ISP_DMA_CTL_C rdma_img1_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		dump_ive_image("pstDst", pstDst);
	}

		if (op == MOD_AND) {
			ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "And", dev_id);
		} else if (op == MOD_OR) {
			ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "Or", dev_id);
		} else if (op == MOD_XOR) {
			ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "Xor", dev_id);
		} else if (op == MOD_ADD) {
			ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "Add", dev_id);
		} else if (op == MOD_SUB) {
			ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "Sub", dev_id);
		} else {
			ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "BaseError", dev_id);
		}

		// setting
		ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
		writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 0;
		writel(ive_top_c->REG_R2Y4_14.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

		if (op == MOD_ADD && pstCtrl != NULL) { //0:and, 1:or, 2:xor, 3:add, 4:sub
			ive_top_c->REG_20.reg_frame2op_add_mode_rounding = 1;
			ive_top_c->REG_20.reg_frame2op_add_mode_clipping = 1;
			//convirt float to unsigned short
			ive_top_c->REG_21.reg_fram2op_x_u0q16 =
				((IVE_ADD_CTRL_S *)pstCtrl)->u0q16X; // 0xffff
			ive_top_c->REG_21.reg_fram2op_y_u0q16 =
				((IVE_ADD_CTRL_S *)pstCtrl)->u0q16Y; // 0xffff
			writel(ive_top_c->REG_21.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_21));
		} else if (op == MOD_SUB && pstCtrl != NULL) {
			ive_top_c->REG_20.reg_frame2op_sub_change_order = 0;
			ive_top_c->REG_20.reg_frame2op_sub_switch_src = 0;
			ive_top_c->REG_20.reg_frame2op_sub_mode =
				((IVE_SUB_CTRL_S *)pstCtrl)->enMode;
		}
		if (op == MOD_ADD) {
			ive_top_c->REG_20.reg_frame2op_op_mode = 3;
		} else if (op == MOD_AND) {
			ive_top_c->REG_20.reg_frame2op_op_mode = 0;
		} else if (op == MOD_OR) {
			ive_top_c->REG_20.reg_frame2op_op_mode = 1;
		} else if (op == MOD_SUB) {
			ive_top_c->REG_20.reg_frame2op_op_mode = 4;
		} else if (op == MOD_XOR) {
			ive_top_c->REG_20.reg_frame2op_op_mode = 2;
		}
		writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));

		set_isp_rdma(op, NULL, dev_id);
		if (set_img_src1(pstSrc1, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
			kfree(ive_top_c);
			return CVI_FAILURE;
		}

		set_img_src2(pstSrc2, &rdma_img1_ctl_c, dev_id);

		set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

		mode = ive_get_mod_u8(pstSrc1->enType);
		if (mode == -1) {
			pr_err("[IVE] not support src type");
			kfree(ive_top_c);
			return CVI_FAILURE;
		}
		ive_top_c->REG_3.reg_ive_rdma_img1_mod_u8 = mode;
		// TODO: need to set vld?
		ive_top_c->REG_3.reg_imgmux_img0_sel = 0;
		ive_top_c->REG_3.reg_ive_rdma_img1_en = 1;
		writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		ive_filterop_c.REG_h10.reg_filterop_mode = 2;
		writel(ive_filterop_c.REG_h10.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));

		ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0; // sw_ovw; bypass op1
		ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0; // bypass op2
		writel(ive_filterop_c.REG_28.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

		ive_top_c->REG_h10.reg_filterop_top_enable = 1;
		ive_top_c->REG_h10.reg_rdma_img1_top_enable = 1;
		writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

		if (pstSrc1->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
					&wdma_y_ctl_c, &rdma_img1_ctl_c, NULL, NULL,
					pstSrc1, pstSrc2, NULL, pstDst, NULL, true, 1,
					false, 1, false, op, bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}

	cvi_ive_go(ndev, ive_top_c, bInstant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, op, dev_id);

	kfree(ive_top_c);
	return CVI_SUCCESS;
}

s32 cvi_ive_reset(struct cvi_ive_device *ndev, s32 select, s32 dev_id)
{
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C ive_top_c = _DEFINE_IVE_TOP_C;

	//udelay(3);
	vi_sys_reg_write_mask(VI_SYS_REG_RST_IVE_TOP0,
				VI_SYS_REG_RST_IVE_TOP0_MASK,
				0x1 << VI_SYS_REG_RST_IVE_TOP0_OFFSET);
	//udelay(3);
	vi_sys_reg_write_mask(VI_SYS_REG_RST_IVE_TOP0,
				VI_SYS_REG_RST_IVE_TOP0_MASK,
				0x0 << VI_SYS_REG_RST_IVE_TOP0_OFFSET);

	ive_set_int(&ive_top_c, 0, dev_id);

	return CVI_SUCCESS;
}

s32 cvi_ive_dump_hw_flow(void)
{
	s32 i = 0;
	IVE_TOP_REG_h10_C TOP_REG_H10;
	IVE_TOP_REG_3_C  TOP_REG_3;
	IVE_FILTEROP_ODMA_REG_00_C ODMA_REG_00;
	IVE_FILTEROP_REG_h14_C REG_h14;

	TOP_REG_H10.val = readl(IVE_BLK_BA[d_num].IVE_TOP + IVE_TOP_REG_H10);
	TOP_REG_3.val = readl(IVE_BLK_BA[d_num].IVE_TOP + IVE_TOP_REG_3);
	ODMA_REG_00.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_ODMA_REG_00);
	REG_h14.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_H14);
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
		if (ODMA_REG_00.reg_dma_en)
			pr_info(" filterop: odma");
		if (REG_h14.reg_op_y_wdma_en)
			pr_info(" filterop: y");
		if (REG_h14.reg_op_c_wdma_en)
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
	return CVI_SUCCESS;
}

s32 cvi_ive_dump_op1_op2_info(void)
{
	IVE_FILTEROP_C ive_filterop_c;
	ive_filterop_c.REG_h10.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_H10);
	ive_filterop_c.REG_h14.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_H14);
	ive_filterop_c.REG_28.val = readl(IVE_BLK_BA[d_num].FILTEROP + IVE_FILTEROP_REG_28);

	pr_info("Ive mode: %d/%d %d/%d\n", ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op,
		ive_filterop_c.REG_h14.reg_filterop_op1_cmd, ive_filterop_c.REG_28.reg_filterop_op2_erodila_en,
		ive_filterop_c.REG_h10.reg_filterop_mode);
	pr_info("\top1: %s\n", ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op?"Enable":"Disable");
	if (ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op) {
		pr_info("\talgo: %d\n", ive_filterop_c.REG_h14.reg_filterop_op1_cmd);
		pr_info("\t 0:OP1_BYP\n\t 1:OP1_FILTER\n\t 2:OP1_DILA\n\t 3:OP1_ERO\n");
		pr_info("\t 4:OP1_ORDERF\n\t 5:OP1_BERN\n\t 6:OP1_LBP\n\t 7:OP1_NORMG\n");
		pr_info("\t 8:OP1_MAG\n\t 9:OP1_SOBEL\n\t 10:OP1_STCANDI\n\t 11:OP1_MAP\n");
	}
	pr_info("\top2: %s\n", ive_filterop_c.REG_28.reg_filterop_op2_erodila_en?"Enable":"Disable");
	if (ive_filterop_c.REG_28.reg_filterop_op2_erodila_en) {
		pr_info("\talgo: %d\n", ive_filterop_c.REG_h10.reg_filterop_mode);
		pr_info("\t 0:MOD_BYP\n\t 1:MOD_FILTER3CH\n\t 2:MOD_DILA\n\t 3:MOD_ERO\n");
		pr_info("\t 4:MOD_CANNY\n\t 5:MOD_STBOX\n\t 6:MOD_GRADFG\n\t 7:MOD_MAG\n");
		pr_info("\t 8:MOD_NORMG\n\t 9:MOD_SOBEL\n\t 10:MOD_STCANDI\n\t 11:MOD_MAP\n");
	}
	return CVI_SUCCESS;
}

s32 cvi_ive_query(struct cvi_ive_device *ndev, bool *pbFinish,
			  bool bBlock, s32 dev_id)
{
	long leavetime = 0;
	*pbFinish = CVI_FAILURE;
	if (!bBlock) {
		leavetime = wait_for_completion_timeout(
			&ndev->core[dev_id].op_done, msecs_to_jiffies(1 * TIMEOUT_MS));
		if (leavetime == 0) {
			CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] stop by timeout\n");
			return CVI_SUCCESS;
		} else if (leavetime < 0) {
			CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] stop by interrupted\n");
			return CVI_SUCCESS;
		}
	} else {
		wait_for_completion(&ndev->core[dev_id].op_done);
	}

	*pbFinish = CVI_TRUE;
	return CVI_SUCCESS;
}

s32 cvi_ive_test(struct cvi_ive_device *ndev, char *addr, u16 *w,
			 u16 *h)
{
	return CVI_SUCCESS;
}

s32 cvi_ive_set_dma_dump(bool enable)
{
	g_dump_dma_info = enable;
	return CVI_SUCCESS;
}

s32 cvi_ive_set_reg_dump(bool enable)
{
	g_dump_reg_info = enable;
	return CVI_SUCCESS;
}

s32 cvi_ive_set_img_dump(bool enable)
{
	g_dump_image_info = enable;
	return CVI_SUCCESS;
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
		return CVI_FAILURE;

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
		CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] assign_block_addr, Virt Addr : 0x%08x, Phy Addr : 0x%08x\n",
			ive_phy_base, ive_phy_base - g_phy_shift[dev_id]);
		CVI_TRACE_IVE(CVI_DBG_INFO, "Dump IVE Register\n");
		array = (uintptr_t *)&IVE_BLK_BA[dev_id];
		for (i = 0; i < 53; i++) {
			CVI_TRACE_IVE(CVI_DBG_INFO, "%s : 0x%x\n", IveRegister[i], array[i] - g_phy_shift[dev_id]);
		}
#endif
		return CVI_SUCCESS;
	}


void cmdq_set_package(struct cmdq_set_reg *set, u32 addr, u32 data)
{
	set->data = data;
	set->addr = addr >> 2;
	set->byte_mask = 0xf;
	set->action = CMDQ_SET_REG;
}

void cmdq_engine(CMDQ_C *ive_cmdq_c, uintptr_t tbl_addr, u16 apb_base,
		bool is_hw_restart, bool is_adma, u16 cnt, s32 dev_id)
{
	// adma or cmdq_set tbl addr
	ive_cmdq_c->DMA_ADDR_L.reg_dma_addr_l = (tbl_addr & 0xFFFFFFFF);
	ive_cmdq_c->DMA_ADDR_H.reg_dma_addr_h = (uint32_t)((uint64_t)tbl_addr >> 32);

	writel(ive_cmdq_c->DMA_ADDR_L.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_ADDR));
	writel(ive_cmdq_c->DMA_ADDR_H.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_ADDR + 4));

	if (!is_adma) {
		ive_cmdq_c->DMA_CNT.reg_dma_cnt = cnt<<3;
		ive_cmdq_c->DMA_CONFIG.reg_adma_en = 0;
		writel(ive_cmdq_c->DMA_CNT.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_CNT));
	} else {
		ive_cmdq_c->DMA_CONFIG.reg_adma_en = 1;
	}
	writel(ive_cmdq_c->DMA_CONFIG.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_DMA_CONFIG));
	ive_cmdq_c->APB_PARA.reg_base_addr = apb_base;
	writel(ive_cmdq_c->APB_PARA.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_APB_PARA));
	// job start
	ive_cmdq_c->JOB_CTL.reg_restart_hw_mod = is_hw_restart;
	ive_cmdq_c->JOB_CTL.reg_job_start = 1;

	writel(ive_cmdq_c->JOB_CTL.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_JOB_CTL));
}

void cmdq_adma_package(struct cmdq_adma *item, u64 addr, u32 size,
		bool is_link, bool is_end)
{
	item->addr = addr;
	item->size = size;
	item->flags_end = is_end ? 1 : 0;
	item->flags_link = is_link ? 1 : 0;
}

s32 cvi_ive_cmdq(struct cvi_ive_device *ndev, s32 dev_id)
{
	return CVI_SUCCESS;  //A2 to do

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
	CMDQ_C ive_cmdq_c = _DEFINE_CMDQ_C;

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_CMDQ\n");
	cmdq_printk(&ive_cmdq_c);

	ive_cmdq_c.INT_EN.reg_cmdq_int_en = 1;
	ive_cmdq_c.INT_EN.reg_cmdq_end_en = 1;
	ive_cmdq_c.INT_EN.reg_cmdq_wait_en = 1;

	writel(ive_cmdq_c.INT_EN.val, (IVE_BLK_BA[dev_id].CMDQ + CMDQ_INT_EN));

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
	return CVI_SUCCESS;
#endif
}

s32 cvi_ive_dma(struct cvi_ive_device *ndev, IVE_DATA_S *pstSrc,
			IVE_DATA_S *pstDst, IVE_DMA_CTRL_S *pstDmaCtrl,
			bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	s32 i = 0;
	s32 size = 0;
	uint32_t *array;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_DMA_C(ive_dma_c);
	IVE_DMA_C ive_dma_c = _DEFINE_IVE_DMA_C;
	//DEFINE_IVE_DMA_C(DMA);
	IVE_DMA_C DMA = _DEFINE_IVE_DMA_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_dma_ctl_c);
	ISP_DMA_CTL_C wdma_dma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_dma_ctl_c);
	ISP_DMA_CTL_C rdma_dma_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_DMA\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_data("pstSrc", pstSrc);
		dump_ive_data("pstDst", pstDst);
	}

	if (pstDst == NULL) {
		if (pstDmaCtrl->enMode == IVE_DMA_MODE_SET_3BYTE ||
			pstDmaCtrl->enMode == IVE_DMA_MODE_SET_8BYTE) {
			pr_err("[IVE] not supper DMA mode of 3BYTE and 8BYTE\n");
			return CVI_FAILURE;
		}
		pstDst = pstSrc;
	}
	ive_set_wh(&ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "DMA", dev_id);
	ive_top_c.REG_h10.reg_dmaf_top_enable = 1;
	writel(ive_top_c.REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	ive_dma_c.REG_0.reg_ive_dma_enable = 1;
	ive_dma_c.REG_0.reg_force_clk_enable = 1;
	ive_dma_c.REG_0.reg_ive_dma_mode = pstDmaCtrl->enMode;
	writel(ive_dma_c.REG_0.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_0));
	ive_dma_c.REG_4.reg_ive_dma_horsegsize = pstDmaCtrl->u8HorSegSize;
	ive_dma_c.REG_4.reg_ive_dma_versegrow = pstDmaCtrl->u8VerSegRows;
	ive_dma_c.REG_4.reg_ive_dma_elemsize = pstDmaCtrl->u8ElemSize;
	writel(ive_dma_c.REG_4.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_4));
	ive_dma_c.REG_1.reg_ive_dma_src_stride = (u16)pstSrc->u32Stride;
	ive_dma_c.REG_1.reg_ive_dma_dst_stride = (u16)pstDst->u32Stride;
	writel(ive_dma_c.REG_1.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_1));

	ive_dma_c.REG_5.reg_ive_dma_u64_val[0] =
		(u32)(pstDmaCtrl->u64Val & 0xffffffff);
	ive_dma_c.REG_5.reg_ive_dma_u64_val[1] =
		(u32)((pstDmaCtrl->u64Val >> 32) & 0xffffffff);
	writel(ive_dma_c.REG_5.val[0], (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_5));
	writel(ive_dma_c.REG_5.val[1],
		   (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_5 + 0x04));

	//Use sw mode for now
	if (0) {
		rdma_dma_ctl_c.SYS_CONTROL.reg_seglen_sel = 1;
		rdma_dma_ctl_c.SYS_CONTROL.reg_segnum_sel = 1;
		rdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 1;
		rdma_dma_ctl_c.DMA_SEGNUM.reg_segnum = pstSrc->u32Height;
		rdma_dma_ctl_c.DMA_SEGLEN.reg_seglen = pstSrc->u32Width;
		rdma_dma_ctl_c.DMA_STRIDE.reg_stride = pstSrc->u32Stride;

		wdma_dma_ctl_c.SYS_CONTROL.reg_seglen_sel = 1;
		wdma_dma_ctl_c.SYS_CONTROL.reg_segnum_sel = 1;
		wdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 1;
		wdma_dma_ctl_c.DMA_SEGNUM.reg_segnum = pstDst->u32Height;
		wdma_dma_ctl_c.DMA_SEGLEN.reg_seglen = pstDst->u32Width;
		wdma_dma_ctl_c.DMA_STRIDE.reg_stride = pstDst->u32Stride;
	} else { // hw mode has issue since r12611
		rdma_dma_ctl_c.SYS_CONTROL.reg_seglen_sel = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_segnum_sel = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 0;
		rdma_dma_ctl_c.DMA_SEGNUM.reg_segnum = 0;
		rdma_dma_ctl_c.DMA_SEGLEN.reg_seglen = 0;
		rdma_dma_ctl_c.DMA_STRIDE.reg_stride = 0;

		wdma_dma_ctl_c.SYS_CONTROL.reg_seglen_sel = 0;
		wdma_dma_ctl_c.SYS_CONTROL.reg_segnum_sel = 0;
		wdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 0;
		wdma_dma_ctl_c.DMA_SEGNUM.reg_segnum = 0;
		wdma_dma_ctl_c.DMA_SEGLEN.reg_seglen = 0;
		wdma_dma_ctl_c.DMA_STRIDE.reg_stride = 0;
	}
	writel(rdma_dma_ctl_c.DMA_SEGNUM.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_SEGNUM));
	writel(rdma_dma_ctl_c.DMA_SEGLEN.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_SEGLEN));
	writel(rdma_dma_ctl_c.DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(wdma_dma_ctl_c.DMA_SEGNUM.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_SEGNUM));
	writel(wdma_dma_ctl_c.DMA_SEGLEN.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_SEGLEN));
	writel(wdma_dma_ctl_c.DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_STRIDE));

	ive_top_c.REG_1.reg_fmt_vld_fg = 1;
	writel(ive_top_c.REG_1.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_1));

	if (ive_dma_c.REG_0.reg_ive_dma_mode == IVE_DMA_MODE_SET_3BYTE ||
		ive_dma_c.REG_0.reg_ive_dma_mode == IVE_DMA_MODE_SET_8BYTE) {
		rdma_dma_ctl_c.BASE_ADDR.reg_basel = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_baseh = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		rdma_dma_ctl_c.DMA_STRIDE.reg_stride = 0;
		rdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 0;
	} else {
		rdma_dma_ctl_c.BASE_ADDR.reg_basel =
			pstSrc->u64PhyAddr & 0xffffffff;
		rdma_dma_ctl_c.SYS_CONTROL.reg_baseh =
			((u64)pstSrc->u64PhyAddr >> 32) & 0xffffffff;
		rdma_dma_ctl_c.DMA_STRIDE.reg_stride = pstSrc->u32Stride;
		rdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 1;
		rdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	}
	writel(rdma_dma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_dma_ctl_c.DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(rdma_dma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_SYS_CONTROL));

	wdma_dma_ctl_c.BASE_ADDR.reg_basel = pstDst->u64PhyAddr & 0xffffffff;
	wdma_dma_ctl_c.SYS_CONTROL.reg_baseh =
		((u64)pstDst->u64PhyAddr >> 32) & 0xffffffff;
	wdma_dma_ctl_c.DMA_STRIDE.reg_stride = pstDst->u32Stride;
	wdma_dma_ctl_c.SYS_CONTROL.reg_stride_sel = 1;
	wdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(wdma_dma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_dma_ctl_c.DMA_STRIDE.val,
			(IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(wdma_dma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Src address: 0x%08x %08x\n",
			rdma_dma_ctl_c.SYS_CONTROL.reg_baseh,
			rdma_dma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("Dst address: 0x%08x %08x\n",
			wdma_dma_ctl_c.SYS_CONTROL.reg_baseh,
			wdma_dma_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[RDMA_DMA].addr_en = rdma_dma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_DMA].addr_l = rdma_dma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_DMA].addr_h = rdma_dma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_DMA].addr_en = wdma_dma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_DMA].addr_l = wdma_dma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_DMA].addr_h = wdma_dma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	ret = cvi_ive_go(ndev, &ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_DMAF_MASK, MOD_DMA, dev_id);

	// add in version 9c2fe24
	ive_dma_c.REG_0.reg_ive_dma_enable = 0;
	writel(ive_dma_c.REG_0.val, (IVE_BLK_BA[dev_id].DMAF + IVE_DMA_REG_0));
	rdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
	rdma_dma_ctl_c.BASE_ADDR.reg_basel = 0;
	rdma_dma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	wdma_dma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
	wdma_dma_ctl_c.BASE_ADDR.reg_basel = 0;
	wdma_dma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	writel(rdma_dma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_dma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].DMAF_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(wdma_dma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_dma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].DMAF_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	size = sizeof(IVE_DMA_C) / sizeof(uint32_t);
	array = (uint32_t *)&DMA;
	for (i = 0; i < size; i++) {
		writel(array[i],
				(IVE_BLK_BA[dev_id].DMAF + sizeof(uint32_t) * i));
	}
	return ret;
}

s32 cvi_ive_add(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			IVE_ADD_CTRL_S *pstAddCtrl, bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] CVI_MPI_IVE_Add\n");
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, MOD_ADD,
				   (void *)pstAddCtrl, dev_id); // 3 add 2 = 0  #2
}

s32 cvi_ive_and(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] debug CVI_MPI_IVE_And\n");
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, MOD_AND,
				   NULL, dev_id); // 3 and 2 = 0  #2
}

s32 cvi_ive_or(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
		   IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		   bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] CVI_MPI_IVE_Or\n");
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, MOD_OR,
				   NULL, dev_id); // 3 or 2 = 2  #3
}

s32 cvi_ive_sub(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			IVE_SUB_CTRL_S *pstSubCtrl, bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] CVI_MPI_IVE_Sub\n");
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, MOD_SUB,
				   (void *)pstSubCtrl, dev_id); // 3 - 2 = 2  #1
}

s32 cvi_ive_xor(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] CVI_MPI_IVE_Xor\n");
	return cvi_ive_base_op(ndev, pstSrc1, pstSrc2, pstDst, bInstant, MOD_XOR,
				   NULL, dev_id); //3 xor 2 = 2  #1
}

s32 cvi_ive_thresh(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstThrCtrl,
			   bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "[IVE] CVI_MPI_IVE_Thresh\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}

	// top
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "Threshold", dev_id);

	ive_top_c->REG_h14c.reg_thresh_u8bit_thr_l = pstThrCtrl->u8LowThr;
	ive_top_c->REG_h14c.reg_thresh_u8bit_thr_h = pstThrCtrl->u8HighThr;
	ive_top_c->REG_h14c.reg_thresh_enmode = pstThrCtrl->enMode;
	writel(ive_top_c->REG_h14c.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H14C));
	ive_top_c->REG_h150.reg_thresh_u8bit_min = pstThrCtrl->u8MinVal;
	ive_top_c->REG_h150.reg_thresh_u8bit_mid = pstThrCtrl->u8MidVal;
	ive_top_c->REG_h150.reg_thresh_u8bit_max = pstThrCtrl->u8MaxVal;
	writel(ive_top_c->REG_h150.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H150));
	ive_top_c->REG_h130.reg_thresh_thresh_en = 1;
	ive_top_c->REG_h130.reg_thresh_top_mod = 0;
	writel(ive_top_c->REG_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));

	set_isp_rdma(MOD_THRESH, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	ive_top_c->REG_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	ive_top_c->REG_h130.reg_thresh_thresh_en = 1;
	writel(ive_top_c->REG_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));

	//bypass filterop...
	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0;
	writel(ive_filterop_c.REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_THRESH,
				bInstant, dev_id);

		kfree(ive_top_c);
		return ret;
	}

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_THRESH, dev_id);

	kfree(ive_top_c);

	return ret;
}

s32 erode_dilate_op(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, u8 *au8Mask,
			bool bInstant, s32 op, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	if (op == MOD_DILA) {
		ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "Dilate", dev_id);
		ive_filterop_c.REG_h10.reg_filterop_mode = 2; // op2 erode or dilate
	} else if (op == MOD_ERO) {
		ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "Erode", dev_id);
		ive_filterop_c.REG_h10.reg_filterop_mode = 3; // op2 erode or dilate
	}
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0; // op1 bypass
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1; // op1 en
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 1; // op2 en
	writel(ive_filterop_c.REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	if (ive_filterop_c.REG_28.reg_filterop_op2_erodila_en == 0) {
		ive_filterop_c.REG_4.reg_filterop_h_coef00 = au8Mask[0];
		ive_filterop_c.REG_4.reg_filterop_h_coef01 = au8Mask[1];
		ive_filterop_c.REG_4.reg_filterop_h_coef02 = au8Mask[2];
		ive_filterop_c.REG_4.reg_filterop_h_coef03 = au8Mask[3];
		writel(ive_filterop_c.REG_4.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
		ive_filterop_c.REG_5.reg_filterop_h_coef04 = au8Mask[4];
		ive_filterop_c.REG_5.reg_filterop_h_coef10 = au8Mask[5];
		ive_filterop_c.REG_5.reg_filterop_h_coef11 = au8Mask[6];
		ive_filterop_c.REG_5.reg_filterop_h_coef12 = au8Mask[7];
		writel(ive_filterop_c.REG_5.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
		ive_filterop_c.REG_6.reg_filterop_h_coef13 = au8Mask[8];
		ive_filterop_c.REG_6.reg_filterop_h_coef14 = au8Mask[9];
		ive_filterop_c.REG_6.reg_filterop_h_coef20 = au8Mask[10];
		ive_filterop_c.REG_6.reg_filterop_h_coef21 = au8Mask[11];
		writel(ive_filterop_c.REG_6.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
		ive_filterop_c.REG_7.reg_filterop_h_coef22 = au8Mask[12];
		ive_filterop_c.REG_7.reg_filterop_h_coef23 = au8Mask[13];
		ive_filterop_c.REG_7.reg_filterop_h_coef24 = au8Mask[14];
		ive_filterop_c.REG_7.reg_filterop_h_coef30 = au8Mask[15];
		writel(ive_filterop_c.REG_7.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
		ive_filterop_c.REG_8.reg_filterop_h_coef31 = au8Mask[16];
		ive_filterop_c.REG_8.reg_filterop_h_coef32 = au8Mask[17];
		ive_filterop_c.REG_8.reg_filterop_h_coef33 = au8Mask[18];
		ive_filterop_c.REG_8.reg_filterop_h_coef34 = au8Mask[19];
		writel(ive_filterop_c.REG_8.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
		ive_filterop_c.REG_9.reg_filterop_h_coef40 = au8Mask[20];
		ive_filterop_c.REG_9.reg_filterop_h_coef41 = au8Mask[21];
		ive_filterop_c.REG_9.reg_filterop_h_coef42 = au8Mask[22];
		ive_filterop_c.REG_9.reg_filterop_h_coef43 = au8Mask[23];
		writel(ive_filterop_c.REG_9.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
		ive_filterop_c.REG_10.reg_filterop_h_coef44 = au8Mask[24];
		writel(ive_filterop_c.REG_10.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));
	} else {
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef00 =
			au8Mask[0];
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef01 =
			au8Mask[1];
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef02 =
			au8Mask[2];
		ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef03 =
			au8Mask[3];
		writel(ive_filterop_c.REG_21.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_21));
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef04 =
			au8Mask[4];
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef10 =
			au8Mask[5];
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef11 =
			au8Mask[6];
		ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef12 =
			au8Mask[7];
		writel(ive_filterop_c.REG_22.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_22));
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef13 =
			au8Mask[8];
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef14 =
			au8Mask[9];
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef20 =
			au8Mask[10];
		ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef21 =
			au8Mask[11];
		writel(ive_filterop_c.REG_23.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_23));
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef22 =
			au8Mask[12];
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef23 =
			au8Mask[13];
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef24 =
			au8Mask[14];
		ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef30 =
			au8Mask[15];
		writel(ive_filterop_c.REG_24.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_24));
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef31 =
			au8Mask[16];
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef32 =
			au8Mask[17];
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef33 =
			au8Mask[18];
		ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef34 =
			au8Mask[19];
		writel(ive_filterop_c.REG_25.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_25));
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef40 =
			au8Mask[20];
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef41 =
			au8Mask[21];
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef42 =
			au8Mask[22];
		ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef43 =
			au8Mask[23];
		writel(ive_filterop_c.REG_26.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_26));
		ive_filterop_c.REG_27.reg_filterop_op2_erodila_coef44 =
			au8Mask[24];
		writel(ive_filterop_c.REG_27.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_27));
	}

	set_isp_rdma(op, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	// FIXME: check default is 0
	ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 0;
	writel(ive_top_c->REG_R2Y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, op,
				bInstant, dev_id);

		kfree(ive_top_c);
		return ret;
	}

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, op, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_erode(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstErodeCtrl,
			  bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Erode\n");
	return erode_dilate_op(ndev, pstSrc, pstDst, pstErodeCtrl->au8Mask,
				   bInstant, 3, dev_id);
}

s32 cvi_ive_dilate(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_DILATE_CTRL_S *pstDilateCtrl, bool bInstant, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Dilate\n");
	return erode_dilate_op(ndev, pstSrc, pstDst, pstDilateCtrl->au8Mask,
				   bInstant, 2, dev_id);
}

s32
cvi_ive_frame_diff_motion(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			IVE_FRAME_DIFF_MOTION_CTRL_S *ctrl, bool bInstant, s32 dev_id)
{
	s32 mode = 0;
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_img1_ctl_c);
	ISP_DMA_CTL_C rdma_img1_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_IVE_FrameDiffMotion\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "FrameDiffMotion", dev_id);

	// 1 sub
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 0;
	writel(ive_top_c->REG_R2Y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

	ive_top_c->REG_20.reg_frame2op_sub_change_order = 0;
	ive_top_c->REG_20.reg_frame2op_sub_switch_src = 0;
	ive_top_c->REG_20.reg_frame2op_sub_mode = ctrl->enSubMode;

	ive_top_c->REG_20.reg_frame2op_op_mode = 4;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));

	// 2 thresh
	ive_top_c->REG_h14c.reg_thresh_u8bit_thr_l = ctrl->u8ThrLow;
	ive_top_c->REG_h14c.reg_thresh_u8bit_thr_h = ctrl->u8ThrHigh;
	ive_top_c->REG_h14c.reg_thresh_enmode = ctrl->enThrMode;
	writel(ive_top_c->REG_h14c.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H14C));
	ive_top_c->REG_h150.reg_thresh_u8bit_min = ctrl->u8ThrMinVal;
	ive_top_c->REG_h150.reg_thresh_u8bit_mid = ctrl->u8ThrMidVal;
	ive_top_c->REG_h150.reg_thresh_u8bit_max = ctrl->u8ThrMaxVal;
	writel(ive_top_c->REG_h150.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H150));

	// 3 Erode Dilate
	ive_top_c->REG_h130.reg_thresh_thresh_en = 1;
	ive_top_c->REG_h130.reg_thresh_top_mod = 0;
	writel(ive_top_c->REG_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	// NOTICE: need to first set it
	ive_top_c->REG_h10.reg_img_in_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 3;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 1;
	writel(ive_filterop_c.REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));
	// 3 Erode
	ive_filterop_c.REG_4.reg_filterop_h_coef00 = ctrl->au8ErodeMask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 = ctrl->au8ErodeMask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 = ctrl->au8ErodeMask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 = ctrl->au8ErodeMask[3];
	writel(ive_filterop_c.REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	ive_filterop_c.REG_5.reg_filterop_h_coef04 = ctrl->au8ErodeMask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 = ctrl->au8ErodeMask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 = ctrl->au8ErodeMask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 = ctrl->au8ErodeMask[7];
	writel(ive_filterop_c.REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	ive_filterop_c.REG_6.reg_filterop_h_coef13 = ctrl->au8ErodeMask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 = ctrl->au8ErodeMask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 = ctrl->au8ErodeMask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 = ctrl->au8ErodeMask[11];
	writel(ive_filterop_c.REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	ive_filterop_c.REG_7.reg_filterop_h_coef22 = ctrl->au8ErodeMask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 = ctrl->au8ErodeMask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 = ctrl->au8ErodeMask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 = ctrl->au8ErodeMask[15];
	writel(ive_filterop_c.REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	ive_filterop_c.REG_8.reg_filterop_h_coef31 = ctrl->au8ErodeMask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 = ctrl->au8ErodeMask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 = ctrl->au8ErodeMask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 = ctrl->au8ErodeMask[19];
	writel(ive_filterop_c.REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	ive_filterop_c.REG_9.reg_filterop_h_coef40 = ctrl->au8ErodeMask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 = ctrl->au8ErodeMask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 = ctrl->au8ErodeMask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 = ctrl->au8ErodeMask[23];
	writel(ive_filterop_c.REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	ive_filterop_c.REG_10.reg_filterop_h_coef44 = ctrl->au8ErodeMask[24];
	writel(ive_filterop_c.REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef00 =
		ctrl->au8DilateMask[0];
	ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef01 =
		ctrl->au8DilateMask[1];
	ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef02 =
		ctrl->au8DilateMask[2];
	ive_filterop_c.REG_21.reg_filterop_op2_erodila_coef03 =
		ctrl->au8DilateMask[3];
	writel(ive_filterop_c.REG_21.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_21));
	ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef04 =
		ctrl->au8DilateMask[4];
	ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef10 =
		ctrl->au8DilateMask[5];
	ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef11 =
		ctrl->au8DilateMask[6];
	ive_filterop_c.REG_22.reg_filterop_op2_erodila_coef12 =
		ctrl->au8DilateMask[7];
	writel(ive_filterop_c.REG_22.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_22));
	ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef13 =
		ctrl->au8DilateMask[8];
	ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef14 =
		ctrl->au8DilateMask[9];
	ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef20 =
		ctrl->au8DilateMask[10];
	ive_filterop_c.REG_23.reg_filterop_op2_erodila_coef21 =
		ctrl->au8DilateMask[11];
	writel(ive_filterop_c.REG_23.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_23));
	ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef22 =
		ctrl->au8DilateMask[12];
	ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef23 =
		ctrl->au8DilateMask[13];
	ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef24 =
		ctrl->au8DilateMask[14];
	ive_filterop_c.REG_24.reg_filterop_op2_erodila_coef30 =
		ctrl->au8DilateMask[15];
	writel(ive_filterop_c.REG_24.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_24));
	ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef31 =
		ctrl->au8DilateMask[16];
	ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef32 =
		ctrl->au8DilateMask[17];
	ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef33 =
		ctrl->au8DilateMask[18];
	ive_filterop_c.REG_25.reg_filterop_op2_erodila_coef34 =
		ctrl->au8DilateMask[19];
	writel(ive_filterop_c.REG_25.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_25));
	ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef40 =
		ctrl->au8DilateMask[20];
	ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef41 =
		ctrl->au8DilateMask[21];
	ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef42 =
		ctrl->au8DilateMask[22];
	ive_filterop_c.REG_26.reg_filterop_op2_erodila_coef43 =
		ctrl->au8DilateMask[23];
	writel(ive_filterop_c.REG_26.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_26));
	ive_filterop_c.REG_27.reg_filterop_op2_erodila_coef44 =
		ctrl->au8DilateMask[24];
	writel(ive_filterop_c.REG_27.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_27));

	set_isp_rdma(MOD_BYP, NULL, dev_id);
	if (set_img_src1(pstSrc1, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	set_img_src2(pstSrc2, &rdma_img1_ctl_c, dev_id);
	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	mode = ive_get_mod_u8(pstSrc1->enType);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	ive_top_c->REG_3.reg_ive_rdma_img1_mod_u8 = mode;

	ive_top_c->REG_3.reg_imgmux_img0_sel = 0;
	ive_top_c->REG_3.reg_ive_rdma_img1_en = 1;
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	ive_top_c->REG_h10.reg_rdma_img1_top_enable = 1;
	ive_top_c->REG_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	if (pstSrc1->u32Width > 480) {
		ret =  emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, &rdma_img1_ctl_c, NULL, NULL,
				pstSrc1, pstSrc2, NULL, pstDst, NULL, true, 1,
				false, 1, false, MOD_MD, bInstant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_MD, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 gmm_gmm2_op(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstBg, IVE_SRC_IMAGE_S *_pstModel,
			IVE_DST_IMAGE_S *pstFg, IVE_TOP_C *ive_top_c,
			IVE_FILTEROP_C *ive_filterop_c, IMG_IN_C *img_in_c,
			IVE_GMM_C *ive_gmm_c, ISP_DMA_CTL_C *gmm_match_wdma_ctl_c,
			ISP_DMA_CTL_C *gmm_factor_rdma_ctl_c, int op, bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	s32 i = 0;
	s32 num = 0;
	u64 dst0addr;
	u32 u32ModelSize;
	ISP_DMA_CTL_C *gmm_mod_rdma_ctl_c[5];
	ISP_DMA_CTL_C *gmm_mod_wdma_ctl_c[5];
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_0);
	ISP_DMA_CTL_C gmm_mod_rdma_0 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_1);
	ISP_DMA_CTL_C gmm_mod_rdma_1 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_2);
	ISP_DMA_CTL_C gmm_mod_rdma_2 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_3);
	ISP_DMA_CTL_C gmm_mod_rdma_3 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_rdma_4);
	ISP_DMA_CTL_C gmm_mod_rdma_4 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_0);
	ISP_DMA_CTL_C gmm_mod_wdma_0 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_1);
	ISP_DMA_CTL_C gmm_mod_wdma_1 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_2);
	ISP_DMA_CTL_C gmm_mod_wdma_2 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_3);
	ISP_DMA_CTL_C gmm_mod_wdma_3 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_mod_wdma_4);
	ISP_DMA_CTL_C gmm_mod_wdma_4 = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;

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
	//IVE_SRC_IMAGE_S stSrc[5];

	//base_ion_alloc(&stSrc[0].u64PhyAddr[0], (void **)&stSrc[i].u64VirAddr[0],
	//	"ive_mesh", pstSrc[0].u32Width * pstSrc[i].u32Height * 8 * 5 + 0x4000, false);

	u32ModelSize = (pstSrc->enType == IVE_IMAGE_TYPE_U8C1) ? 8 : 12;
	for (i = 0; i < 5; i++) {
		//if( i == 0 ){
		//	dst0addr = (((stSrc[0].u64PhyAddr[0] & 0xffffffff) / 0x4000) + 1) * 0x4000;
		//} else {
		//	u64 tmp = pstSrc->u32Width * pstSrc->u32Height * u32ModelSize * i;
		//	u64 tmpaddr = (((stSrc[0].u64PhyAddr[0] & 0xffffffff) / 0x4000) + 1) * 0x4000 + tmp;
		//	dst0addr = (((tmpaddr & 0xffffffff) / 0x4000) + 1) * 0x4000 + 0x800 * i;
		//}
		dst0addr =
			(u64)_pstModel->u64PhyAddr[0] +
			pstSrc->u32Width * pstSrc->u32Height * u32ModelSize * i;
		num = (readl(IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13) &
			   IVE_GMM_REG_GMM_GMM2_MODEL_NUM_MASK) >>
			  IVE_GMM_REG_GMM_GMM2_MODEL_NUM_OFFSET;

		if (i < num) {
			gmm_mod_rdma_ctl_c[i]->BASE_ADDR.reg_basel =
				dst0addr & 0xffffffff;
			gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_baseh =
				(dst0addr >> 32) & 0xffffffff;
			/*pr_err("R/W address : 0x%08x%08x\n",
				gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_baseh,
				gmm_mod_rdma_ctl_c[i]->BASE_ADDR.reg_basel);*/
			gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_base_sel = 1;
			writel(gmm_mod_rdma_ctl_c[i]->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
			gmm_mod_wdma_ctl_c[i]->BASE_ADDR.reg_basel =
				dst0addr & 0xffffffff;
			gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_baseh =
				(dst0addr >> 32) & 0xffffffff;
			gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_base_sel = 1;
			writel(gmm_mod_wdma_ctl_c[i]->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
		} else {
			gmm_mod_rdma_ctl_c[i]->BASE_ADDR.reg_basel = 0;
			gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_baseh = 0;
			gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_base_sel = 0;
			writel(gmm_mod_rdma_ctl_c[i]->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_RDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
			gmm_mod_wdma_ctl_c[i]->BASE_ADDR.reg_basel = 0;
			gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_baseh = 0;
			gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_base_sel = 0;
			writel(gmm_mod_wdma_ctl_c[i]->BASE_ADDR.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_BASE_ADDR));
			writel(gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.val,
				   (IVE_BLK_BA[dev_id].GMM_MODEL_WDMA_0 + i * 0x40 +
				ISP_DMA_CTL_SYS_CONTROL));
		}
		if (g_dump_dma_info == CVI_TRUE) {
			pr_info("[%d]Src GMM address rdma: 0x%08x %08x\n", i,
				gmm_mod_rdma_ctl_c[i]->SYS_CONTROL.reg_baseh,
				gmm_mod_rdma_ctl_c[i]->BASE_ADDR.reg_basel);
			pr_info("Dst GMM address wdma : 0x%08x %08x\n",
				gmm_mod_wdma_ctl_c[i]->SYS_CONTROL.reg_baseh,
				gmm_mod_wdma_ctl_c[i]->BASE_ADDR.reg_basel);
		}
		if (i == 0) {
			g_debug_info.addr[RDMA_MM_MOD].addr_en = gmm_mod_rdma_ctl_c[0]->SYS_CONTROL.reg_base_sel;
			g_debug_info.addr[RDMA_MM_MOD].addr_l = gmm_mod_rdma_ctl_c[0]->BASE_ADDR.reg_basel;
			g_debug_info.addr[RDMA_MM_MOD].addr_h = gmm_mod_rdma_ctl_c[0]->SYS_CONTROL.reg_baseh & 0xff;

			g_debug_info.addr[WDMA_GMM_MOD].addr_en = gmm_mod_wdma_ctl_c[0]->SYS_CONTROL.reg_base_sel;
			g_debug_info.addr[WDMA_GMM_MOD].addr_l = gmm_mod_wdma_ctl_c[0]->BASE_ADDR.reg_basel;
			g_debug_info.addr[WDMA_GMM_MOD].addr_h = gmm_mod_wdma_ctl_c[0]->SYS_CONTROL.reg_baseh & 0xff;
		}
	}

	set_isp_rdma(op, NULL, dev_id);
	if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 5;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	//bypass filterop...
	ive_filterop_c->REG_h10.reg_filterop_mode = 2;
	ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 0; //sw_ovw; bypass op1
	ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 1;
	ive_filterop_c->REG_28.reg_filterop_op2_erodila_en = 0; //bypass op2
	writel(ive_filterop_c->REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	ive_top_c->REG_3.reg_dma_share_mux_selgmm = 1;
	ive_top_c->REG_h10.reg_gmm_top_enable = 1;
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstFg, &wdma_y_ctl_c, dev_id);
	set_odma(pstBg, ive_filterop_c, pstSrc->u32Width, pstSrc->u32Height, dev_id);

	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	return emit_bgm_tile(ndev, true, true, op, ive_top_c, ive_filterop_c,
			   img_in_c, ive_gmm_c, &wdma_y_ctl_c,
			   pstBg->u32Stride[0], pstSrc->u32Width,
			   pstSrc->u32Height, pstBg->enType, pstSrc->enType,
			   bInstant, NULL, gmm_mod_rdma_ctl_c,
			   gmm_mod_wdma_ctl_c, gmm_match_wdma_ctl_c,
			   gmm_factor_rdma_ctl_c, NULL, NULL, NULL, NULL, NULL,
			   NULL, NULL, NULL, NULL, dev_id);

	//for (i = 0; i < 5; i++) {
	//	if(stSrc[i].u64PhyAddr[0] != NULL)
	//		base_ion_free(stSrc[i].u64PhyAddr[0]);
	//}
	//return ret;

	// NOTICE: last one to trigger it
	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK |
				 IVE_TOP_REG_FRAME_DONE_GMM_MASK, op, dev_id);

	ive_gmm_c->REG_GMM_13.reg_gmm_gmm2_enable = 0; //9c2fe24
	writel(ive_gmm_c->REG_GMM_13.val,
		   (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
	return ret;
}

s32 cvi_ive_gmm(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstBg,
			IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl,
			bool bInstant,s32 dev_id)
{
	s32 ret = 0;
	IVE_SRC_IMAGE_S _pstModel;
	IVE_GMM_REG_GMM_0_C REG_GMM_0;
	IVE_GMM_REG_GMM_6_C REG_GMM_6;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	IVE_GMM_C ive_gmm_c = _DEFINE_IVE_GMM_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_GMM\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstFg", pstFg);
		dump_ive_image("pstBg", pstBg);
		dump_ive_mem("pstModel", pstModel);
	}
	if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1 &&
		pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PACKAGE &&
		pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PLANAR) {
		pr_err("pstSrc->enType cannot be (%d)\n", pstSrc->enType);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	// top
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "GMM", dev_id);

	// setting
	REG_GMM_0.reg_gmm_learn_rate = pstGmmCtrl->u0q16LearnRate;
	REG_GMM_0.reg_gmm_bg_ratio = pstGmmCtrl->u0q16BgRatio;
	REG_GMM_6.val = ive_gmm_c.REG_GMM_6.val;
	ive_gmm_c.REG_GMM_0.val = REG_GMM_0.val;
	ive_gmm_c.REG_GMM_1.reg_gmm_var_thr = pstGmmCtrl->u8q8VarThr;
	ive_gmm_c.REG_GMM_2.reg_gmm_noise_var = pstGmmCtrl->u22q10NoiseVar;
	ive_gmm_c.REG_GMM_3.reg_gmm_max_var = pstGmmCtrl->u22q10MaxVar;
	ive_gmm_c.REG_GMM_4.reg_gmm_min_var = pstGmmCtrl->u22q10MinVar;
	ive_gmm_c.REG_GMM_5.reg_gmm_init_weight = pstGmmCtrl->u0q16InitWeight;
	writel(ive_gmm_c.REG_GMM_0.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_0));
	writel(ive_gmm_c.REG_GMM_1.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_1));
	writel(ive_gmm_c.REG_GMM_2.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_2));
	writel(ive_gmm_c.REG_GMM_3.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_3));
	writel(ive_gmm_c.REG_GMM_4.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_4));
	writel(ive_gmm_c.REG_GMM_5.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_5));
	REG_GMM_6.reg_gmm_detect_shadow = 0; // enDetectShadow
	REG_GMM_6.reg_gmm_shadow_thr = 0; // u0q8ShadowThr
	REG_GMM_6.reg_gmm_sns_factor = 8; // u8SnsFactor, as from GMM2 sample
	ive_gmm_c.REG_GMM_6.val = REG_GMM_6.val;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_model_num = pstGmmCtrl->u8ModelNum;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly =
		(pstSrc->enType == IVE_IMAGE_TYPE_U8C1) ? 1 : 0;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_enable = 1;

	writel(ive_gmm_c.REG_GMM_6.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_6));
	writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

	_pstModel.u64PhyAddr[0] = pstModel->u64PhyAddr;
	//_pstModel.u64VirAddr[0] = pstModel->u64VirAddr;

	ret = gmm_gmm2_op(ndev, pstSrc, pstBg, &_pstModel, pstFg, ive_top_c,
			   &ive_filterop_c, &img_in_c, &ive_gmm_c, NULL, NULL, MOD_GMM,
			   bInstant, dev_id);
	kfree(ive_top_c);
	return ret;

}

s32 cvi_ive_gmm2(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_SRC_IMAGE_S *pstFactor, IVE_DST_IMAGE_S *pstFg,
			 IVE_DST_IMAGE_S *pstBg, IVE_DST_IMAGE_S *pstMatchModelInfo,
			 IVE_MEM_INFO_S *pstModel, IVE_GMM2_CTRL_S *pstGmm2Ctrl,
			 bool bInstant, s32 dev_id)
{
	IVE_SRC_IMAGE_S _pstModel;
	u64 dst2addr, src2addr;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	IVE_GMM_C ive_gmm_c = _DEFINE_IVE_GMM_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_match_wdma_ctl_c);
	ISP_DMA_CTL_C gmm_match_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(gmm_factor_rdma_ctl_c);
	ISP_DMA_CTL_C gmm_factor_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}


	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_GMM2\n");
	ive_reset(ndev, ive_top_c, dev_id);
	/*pstSrc->u32Stride[0] = 640;
	pstFactor->u32Stride[0] = 640*2;
	pstFg->u32Stride[0] = 640;
	pstBg->u32Stride[0] = 640;
	pstMatchModelInfo->u32Stride[0] = 640;*/
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstFactor", pstFactor);
		dump_ive_image("pstFg", pstFg);
		dump_ive_image("pstBg", pstBg);
		dump_ive_image("pstMatchModelInfo", pstMatchModelInfo);
		dump_ive_mem("pstModel", pstModel);
	}

	if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1 &&
		pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PACKAGE &&
		pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PLANAR) {
		pr_err("pstSrc->enType cannot be (%d)\n", pstSrc->enType);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	// top
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "GMM2", dev_id);

	// setting
	ive_gmm_c.REG_GMM_7.reg_gmm2_life_update_factor =
		pstGmm2Ctrl->u16GlbLifeUpdateFactor;
	ive_gmm_c.REG_GMM_7.reg_gmm2_var_rate = pstGmm2Ctrl->u16VarRate;
	writel(ive_gmm_c.REG_GMM_7.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_7));
	ive_gmm_c.REG_GMM_8.reg_gmm2_freq_redu_factor =
		pstGmm2Ctrl->u16FreqReduFactor;
	ive_gmm_c.REG_GMM_8.reg_gmm2_max_var = pstGmm2Ctrl->u9q7MaxVar;
	writel(ive_gmm_c.REG_GMM_8.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_8));
	ive_gmm_c.REG_GMM_9.reg_gmm2_min_var = pstGmm2Ctrl->u9q7MinVar;
	ive_gmm_c.REG_GMM_9.reg_gmm2_freq_add_factor =
		pstGmm2Ctrl->u16FreqAddFactor;
	writel(ive_gmm_c.REG_GMM_9.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_9));
	ive_gmm_c.REG_GMM_10.reg_gmm2_freq_init = pstGmm2Ctrl->u16FreqInitVal;
	ive_gmm_c.REG_GMM_10.reg_gmm2_freq_thr = pstGmm2Ctrl->u16FreqThr;
	writel(ive_gmm_c.REG_GMM_10.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_10));
	ive_gmm_c.REG_GMM_11.reg_gmm2_life_thr = pstGmm2Ctrl->u16LifeThr;
	ive_gmm_c.REG_GMM_11.reg_gmm2_sns_factor = pstGmm2Ctrl->u8GlbSnsFactor;
	writel(ive_gmm_c.REG_GMM_11.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_11));
	//iveReg->ive_gmm_c->.reg_gmm2_factor = 0;
	ive_gmm_c.REG_GMM_12.reg_gmm2_life_update_factor_mode =
		pstGmm2Ctrl->enLifeUpdateFactorMode;
	ive_gmm_c.REG_GMM_12.reg_gmm2_sns_factor_mode =
		pstGmm2Ctrl->enSnsFactorMode;
	writel(ive_gmm_c.REG_GMM_12.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_12));
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly =
		(pstSrc->enType == IVE_IMAGE_TYPE_U8C1) ? 1 : 0;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_model_num = pstGmm2Ctrl->u8ModelNum;
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_enable = 2;
	writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

	_pstModel.u64PhyAddr[0] = pstModel->u64PhyAddr;
	/*
	 *S0 pstSrc
	 *S1 pstModel
	 *S2 pstFactor
	 *D0 pstBg
	 *D1 pstFg
	 *D2 pstMatchModelInfo
	 */
	dst2addr = pstMatchModelInfo->u64PhyAddr[0];
	src2addr = pstFactor->u64PhyAddr[0];

	if (dst2addr) {
		gmm_match_wdma_ctl_c.BASE_ADDR.reg_basel =
			dst2addr & 0xffffffff;
		gmm_match_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(dst2addr >> 32) & 0xffffffff;
		gmm_match_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
		gmm_match_wdma_ctl_c.DMA_STRIDE.reg_stride = pstMatchModelInfo->u32Stride[0];
	} else {
		gmm_match_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		gmm_match_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		gmm_match_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
		gmm_match_wdma_ctl_c.DMA_STRIDE.reg_stride = 0;
	}
	writel(gmm_match_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(gmm_match_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(gmm_match_wdma_ctl_c.DMA_STRIDE.val,
			(IVE_BLK_BA[dev_id].GMM_MATCH_WDMA + ISP_DMA_CTL_DMA_STRIDE));
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("GMM WDMA address: 0x%08x %08x\n",
			gmm_match_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			gmm_match_wdma_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[WDMA_GMM_MATCH].addr_en = gmm_match_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_GMM_MATCH].addr_l = gmm_match_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_GMM_MATCH].addr_h = gmm_match_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;
	if (src2addr) {
		gmm_factor_rdma_ctl_c.BASE_ADDR.reg_basel =
			src2addr & 0xffffffff;
		gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src2addr >> 32) & 0xffffffff;
		gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
		gmm_factor_rdma_ctl_c.DMA_STRIDE.reg_stride = pstFactor->u32Stride[0];
	} else {
		gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		gmm_factor_rdma_ctl_c.BASE_ADDR.reg_basel = 0;
		gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
		gmm_factor_rdma_ctl_c.DMA_STRIDE.reg_stride = 0;
	}
	writel(gmm_factor_rdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(gmm_factor_rdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(gmm_factor_rdma_ctl_c.DMA_STRIDE.val,
			(IVE_BLK_BA[dev_id].GMM_FACTOR_RDMA + ISP_DMA_CTL_DMA_STRIDE));

	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("GMM FACTOR RDMA address: 0x%08x %08x\n",
			gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			gmm_factor_rdma_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[RDMA_MM_FACTOR].addr_en = gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_MM_FACTOR].addr_l = gmm_factor_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_MM_FACTOR].addr_h = gmm_factor_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;
	gmm_gmm2_op(ndev, pstSrc, pstBg, &_pstModel, pstFg, ive_top_c,
			&ive_filterop_c, &img_in_c, &ive_gmm_c,
			&gmm_match_wdma_ctl_c, &gmm_factor_rdma_ctl_c, MOD_GMM2, bInstant, dev_id);

	kfree(ive_top_c);
	return CVI_SUCCESS;
}

s32 cvi_ive_match_bg_model(struct cvi_ive_device *ndev,
				 IVE_SRC_IMAGE_S *pstCurImg, IVE_DATA_S *pstBgModel,
				 IVE_IMAGE_S *pstFgFlag, IVE_DST_IMAGE_S *pstDiffFg,
				 IVE_BG_STAT_DATA_S *pstStatData,
				 IVE_MATCH_BG_MODEL_CTRL_S *pstMatchBgModelCtrl,
				 bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	s32 i = 0;
	u64 dst0addr;
	IVE_SRC_IMAGE_S _pstFgFlag;
	IVE_SRC_IMAGE_S _pstBgModel;
	u64 src2addr, src3addr, src2addr_1;
	// IVE_BG_STAT_DATA_S stat;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
	IVE_MATCH_BG_C ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
	//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
	IVE_UPDATE_BG_C ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;
	//DEFINE_ISP_DMA_CTL_C(fgflag_rdma_ctl_c);
	ISP_DMA_CTL_C fgflag_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_0_rdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_0_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_1_rdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_1_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(difffg_wdma_ctl_c);
	ISP_DMA_CTL_C difffg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(fg_wdma_ctl_c);
	ISP_DMA_CTL_C fg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_0_wdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_0_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_1_wdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_1_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	IVE_TOP_C *ive_top_c = NULL;
	IVE_FILTEROP_C *ive_filterop_c = NULL;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c = init_ive_filterop_c();
	if (!ive_filterop_c) {
		pr_err("ive_filterop_c init failed\n");
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_MatchBgModel\n");

	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstCurImg", pstCurImg);
		dump_ive_data("pstBgModel", pstBgModel);
		dump_ive_image("pstFgFlag", pstFgFlag);
		dump_ive_image("pstDiffFg", pstDiffFg);
		// dump_ive_mem("pstStatData", pstStatData);
	}

	ive_reset(ndev, ive_top_c, dev_id);
	// top
	ive_set_wh(ive_top_c, pstCurImg->u32Width, pstCurImg->u32Height, "MatchBgModel", dev_id);

	ive_match_bg_c.REG_00.reg_matchbg_softrst = 0;
	writel(ive_match_bg_c.REG_00.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	//+0x80
	ive_match_bg_c.REG_04.reg_matchbg_curfrmnum =
		pstMatchBgModelCtrl->u32CurFrmNum;
	ive_match_bg_c.REG_08.reg_matchbg_prefrmnum =
		pstMatchBgModelCtrl->u32PreFrmNum;
	writel(ive_match_bg_c.REG_04.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_04));
	writel(ive_match_bg_c.REG_08.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_08));
	ive_match_bg_c.REG_0C.reg_matchbg_timethr =
		pstMatchBgModelCtrl->u16TimeThr;
	ive_match_bg_c.REG_0C.reg_matchbg_diffthrcrlcoef =
		pstMatchBgModelCtrl->u8DiffThrCrlCoef;
	ive_match_bg_c.REG_0C.reg_matchbg_diffmaxthr =
		pstMatchBgModelCtrl->u8DiffMaxThr;
	ive_match_bg_c.REG_0C.reg_matchbg_diffminthr =
		pstMatchBgModelCtrl->u8DiffMinThr;
	ive_match_bg_c.REG_0C.reg_matchbg_diffthrinc =
		pstMatchBgModelCtrl->u8DiffThrInc;
	ive_match_bg_c.REG_0C.reg_matchbg_fastlearnrate =
		pstMatchBgModelCtrl->u8FastLearnRate;
	ive_match_bg_c.REG_0C.reg_matchbg_detchgregion =
		pstMatchBgModelCtrl->u8DetChgRegion;
	writel(ive_match_bg_c.REG_0C.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_0C));
	// TODO: set dma
	set_isp_rdma(MOD_BGM, NULL, dev_id);
	for (i = 0; i < 3; i++) {
		_pstFgFlag.u64PhyAddr[i] = pstFgFlag->u64PhyAddr[i];
		//_pstFgFlag.u64VirAddr[i] = pstFgFlag->u64VirAddr[i];
		_pstFgFlag.u32Stride[i] = pstFgFlag->u32Stride[i];
		_pstBgModel.u64PhyAddr[i] = pstBgModel->u64PhyAddr;
		//_pstBgModel.u64VirAddr[i] = pstBgModel->u64VirAddr;
		_pstBgModel.u32Stride[i] = pstBgModel->u32Stride;
	}
	/*
	 *S0 pstCurImg
	 *S1 _pstBgModel
	 *S2 _pstFgFlag
	 *D0 pstDiffFg
	 *D1 pstFrmDiffFg
	 *D2
	 */
	src2addr = _pstBgModel.u64PhyAddr[0];
	src2addr_1 =
		src2addr + (pstCurImg->u32Width * pstCurImg->u32Height * 16);
	src3addr = _pstFgFlag.u64PhyAddr[0];

	if (src2addr) {
		bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel =
			src2addr & 0xffffffff;
		bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src2addr >> 32) & 0xffffffff;
		bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel = 0;
		bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(bgmodel_0_rdma_ctl_c.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
		ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_rdma_ctl_c.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA +
		ISP_DMA_CTL_SYS_CONTROL));
	if (src2addr_1) {
		bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel =
			src2addr_1 & 0xffffffff;
		bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src2addr_1 >> 32) & 0xffffffff;
		bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel = 0;
		bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(bgmodel_1_rdma_ctl_c.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
		ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_rdma_ctl_c.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA +
		ISP_DMA_CTL_SYS_CONTROL));
	if (src3addr) {
		fgflag_rdma_ctl_c.BASE_ADDR.reg_basel = src3addr & 0xffffffff;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src3addr >> 32) & 0xffffffff;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		fgflag_rdma_ctl_c.BASE_ADDR.reg_basel = 0;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(fgflag_rdma_ctl_c.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
		ISP_DMA_CTL_BASE_ADDR));
	writel(fgflag_rdma_ctl_c.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA +
		ISP_DMA_CTL_SYS_CONTROL));


	if (set_img_src1(pstCurImg, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return CVI_FAILURE;
	}

	bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel = src2addr & 0xffffffff;
	bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh =
		(src2addr >> 32) & 0xffffffff;
	bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(bgmodel_0_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel = src2addr_1 & 0xffffffff;
	bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh =
		(src2addr_1 >> 32) & 0xffffffff;
	bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(bgmodel_1_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	fg_wdma_ctl_c.BASE_ADDR.reg_basel = src3addr & 0xffffffff;
	fg_wdma_ctl_c.SYS_CONTROL.reg_baseh = (src3addr >> 32) & 0xffffffff;
	fg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(fg_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(fg_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	////////////////////////
	ive_match_bg_c.REG_00.reg_matchbg_en = 1;
	ive_match_bg_c.REG_00.reg_matchbg_bypass_model = 0;
	ive_update_bg_c.REG_H04.reg_enable = 0;
	ive_update_bg_c.REG_H04.reg_updatebg_byp_model = 1;
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 5;

	ive_top_c->REG_h10.reg_filterop_top_enable = 0;
	ive_top_c->REG_h10.reg_bgm_top_enable = 1;
	ive_top_c->REG_h10.reg_bgu_top_enable = 1;
	writel(ive_match_bg_c.REG_00.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	writel(ive_update_bg_c.REG_H04.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	///////////////////////////
	dst0addr = pstDiffFg->u64PhyAddr[0];

	if (dst0addr) {
		difffg_wdma_ctl_c.BASE_ADDR.reg_basel = dst0addr & 0xffffffff;
		difffg_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(dst0addr >> 32) & 0xffffffff;
		difffg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		difffg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		difffg_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		difffg_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(difffg_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(difffg_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_DIFFFG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("BG_UPDATE_BGMODEL_0_WDMA address: 0x%08x %08x\n",
			bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_UPDATE_BGMODEL_1_WDMA address: 0x%08x %08x\n",
			bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_UPDATE_FG_WDMA address: 0x%08x %08x\n",
			fg_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			fg_wdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_MATCH_BGMODEL_0_RDMA address: 0x%08x %08x\n",
			bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_MATCH_BGMODEL_1_RDMA address: 0x%08x %08x\n",
			bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_MATCH_FGFLAG_RDMA address: 0x%08x %08x\n",
			fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			fgflag_rdma_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[WDMA_BGMODEL_0].addr_en = bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_l = bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_h = bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_BGMODEL_1].addr_en = bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_l = bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_h = bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_FG].addr_en = fg_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_FG].addr_l = fg_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_FG].addr_h = fg_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_0].addr_en = bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_l = bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_h = bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_1].addr_en = bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_l = bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_h = bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GFLAG].addr_en = fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_GFLAG].addr_l = fgflag_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_GFLAG].addr_h = fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	ive_filterop_c->ODMA_REG_00.reg_dma_en = 0;
	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	// ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 0;
	// ive_filterop_c->REG_h14.reg_filterop_3ch_en = 0;
	writel(ive_filterop_c->ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	ret = emit_bgm_tile(ndev, false, false, MOD_BGM, ive_top_c,
			  ive_filterop_c, &img_in_c, NULL, NULL,
			  pstCurImg->u32Stride[0], pstCurImg->u32Width,
			  pstCurImg->u32Height, -1, pstCurImg->enType, bInstant,
			  pstStatData, NULL, NULL, NULL, NULL, &fgflag_rdma_ctl_c,
			  &bgmodel_0_rdma_ctl_c, &difffg_wdma_ctl_c,
			  &bgmodel_1_rdma_ctl_c, &fg_wdma_ctl_c,
			  &bgmodel_0_wdma_ctl_c, &bgmodel_1_wdma_ctl_c, NULL,
			  &ive_update_bg_c, dev_id);


	kfree(ive_top_c);
	kfree(ive_filterop_c);
	return ret;
#if 0
	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_BGM_MASK, MOD_BGM);

	stat.u32PixNum = readl(IVE_BLK_BA.BG_MATCH_IVE_MATCH_BG +
				   IVE_MATCH_BG_REG_10); //9c2fe24
	stat.u32SumLum = readl(IVE_BLK_BA.BG_MATCH_IVE_MATCH_BG +
				   IVE_MATCH_BG_REG_14); //9c2fe24
	kfree(ive_top_c);
	kfree(ive_filterop_c);
	return ret;
#endif
}

s32 cvi_ive_update_bg_model(struct cvi_ive_device *ndev,
				  IVE_SRC_IMAGE_S *pstCurImg,
				  IVE_DATA_S *pstBgModel, IVE_IMAGE_S *pstFgFlag,
				  IVE_DST_IMAGE_S *pstBgImg,
				  IVE_DST_IMAGE_S *pstChgSta,
				  IVE_BG_STAT_DATA_S *pstStatData,
				  IVE_UPDATE_BG_MODEL_CTRL_S *pstUpdateBgModelCtrl,
				  bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	s32 i;
	IVE_DST_IMAGE_S _pstFgFlag;
	IVE_DST_IMAGE_S _pstBgModel;
	u64 src2addr, src3addr, src2addr_1;
	// IVE_BG_STAT_DATA_S stat;
	IVE_UPDATE_BG_REG_ctrl3_C REG_ctrl3;
	IVE_UPDATE_BG_REG_ctrl5_C REG_ctrl5;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(fg_wdma_ctl_c);
	ISP_DMA_CTL_C fg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_0_wdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_0_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_1_wdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_1_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(fgflag_rdma_ctl_c);
	ISP_DMA_CTL_C fgflag_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_0_rdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_0_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(bgmodel_1_rdma_ctl_c);
	ISP_DMA_CTL_C bgmodel_1_rdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(difffg_wdma_ctl_c);
	ISP_DMA_CTL_C difffg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(chg_wdma_ctl_c);
	ISP_DMA_CTL_C chg_wdma_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_MATCH_BG_C(ive_match_bg_c);
	IVE_MATCH_BG_C ive_match_bg_c = _DEFINE_IVE_MATCH_BG_C;
	//DEFINE_IVE_UPDATE_BG_C(ive_update_bg_c);
	IVE_UPDATE_BG_C ive_update_bg_c = _DEFINE_IVE_UPDATE_BG_C;
	IVE_TOP_C *ive_top_c = NULL;
	IVE_FILTEROP_C *ive_filterop_c = NULL;
	//DEFINE_IVE_TOP_C(ive_top_c);
	ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	ive_filterop_c = init_ive_filterop_c();
	if (!ive_filterop_c) {
		pr_err("ive_filterop_c init failed\n");
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_UpdateBgModel\n");

	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_data("stBgModel", pstBgModel);
		dump_ive_image("stFgFlag", pstFgFlag);
		dump_ive_image("stBgImg", pstBgImg);
		dump_ive_image("pstChgSta", pstChgSta);
		// dump_ive_mem("stStatData", pstStatData);
	}

	ive_reset(ndev, ive_top_c, dev_id);
	// top
	ive_set_wh(ive_top_c, pstBgImg->u32Width, pstBgImg->u32Height, "UpdateBgModel", dev_id);
	ive_top_c->REG_h10.reg_img_in_top_enable = 0;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ive_update_bg_c.REG_H04.reg_enable = 1;
	ive_update_bg_c.REG_H04.reg_updatebg_byp_model = 0;
	writel(ive_update_bg_c.REG_H04.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));

	REG_ctrl3.reg_u32StyBgMinBlendTime =
		pstUpdateBgModelCtrl->u32StyBgMinBlendTime;
	REG_ctrl3.reg_u32StyBgMaxBlendTime =
		pstUpdateBgModelCtrl->u32StyBgMaxBlendTime;
	REG_ctrl5.reg_u16BgMaxFadeTime = pstUpdateBgModelCtrl->u16BgMaxFadeTime;
	REG_ctrl5.reg_u16FgMaxFadeTime = pstUpdateBgModelCtrl->u16FgMaxFadeTime;
	REG_ctrl5.reg_u8StyBgAccTimeRateThr =
		pstUpdateBgModelCtrl->u8StyBgAccTimeRateThr;
	REG_ctrl5.reg_u8ChgBgAccTimeRateThr =
		pstUpdateBgModelCtrl->u8ChgBgAccTimeRateThr;

	ive_update_bg_c.REG_ctrl0.reg_u32CurFrmNum =
		pstUpdateBgModelCtrl->u32CurFrmNum;
	ive_update_bg_c.REG_ctrl1.reg_u32PreChkTime =
		pstUpdateBgModelCtrl->u32PreChkTime;
	ive_update_bg_c.REG_ctrl2.reg_u32FrmChkPeriod =
		pstUpdateBgModelCtrl->u32FrmChkPeriod;
	ive_update_bg_c.REG_ctrl2.reg_u32InitMinTime =
		pstUpdateBgModelCtrl->u32InitMinTime;
	ive_update_bg_c.REG_ctrl3.val = REG_ctrl3.val;
	ive_update_bg_c.REG_ctrl4.reg_u32DynBgMinBlendTime =
		pstUpdateBgModelCtrl->u32DynBgMinBlendTime;
	ive_update_bg_c.REG_ctrl4.reg_u32StaticDetMinTime =
		pstUpdateBgModelCtrl->u32StaticDetMinTime;
	ive_update_bg_c.REG_ctrl5.val = REG_ctrl5.val;
	ive_update_bg_c.REG_ctrl6.reg_u8DynBgAccTimeThr =
		pstUpdateBgModelCtrl->u8DynBgAccTimeThr;
	ive_update_bg_c.REG_ctrl6.reg_u8DynBgDepth = 3; //fix to 3
	ive_update_bg_c.REG_ctrl6.reg_u8BgEffStaRateThr =
		pstUpdateBgModelCtrl->u8BgEffStaRateThr;
	ive_update_bg_c.REG_ctrl6.reg_u8AcceBgLearn =
		pstUpdateBgModelCtrl->u8AcceBgLearn;
	ive_update_bg_c.REG_ctrl6.reg_u8DetChgRegion =
		pstUpdateBgModelCtrl->u8DetChgRegion;
	writel(ive_update_bg_c.REG_ctrl0.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL0));
	writel(ive_update_bg_c.REG_ctrl1.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL1));
	writel(ive_update_bg_c.REG_ctrl2.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL2));
	writel(ive_update_bg_c.REG_ctrl3.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL3));
	writel(ive_update_bg_c.REG_ctrl4.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL4));
	writel(ive_update_bg_c.REG_ctrl5.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL5));
	writel(ive_update_bg_c.REG_ctrl6.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_CTRL6));

	for (i = 0; i < 3; i++) {
		_pstFgFlag.u64PhyAddr[i] = pstFgFlag->u64PhyAddr[i];
		//_pstFgFlag.u64VirAddr[i] = pstFgFlag->u64VirAddr[i];
		_pstFgFlag.u32Stride[i] = pstFgFlag->u32Stride[i];
		_pstBgModel.u64PhyAddr[i] = pstBgModel->u64PhyAddr;
		//_pstBgModel.u64VirAddr[i] = pstBgModel->u64VirAddr;
		_pstBgModel.u32Stride[i] = pstBgModel->u32Stride;
	}
	/*
	 * S0 NULL
	 * S1 _pstBgModel
	 * S2 _pstFgFlag
	 * D0 pstBgImg
	 * D1 pstChgSta
	 * D2
	 */
	src2addr = _pstBgModel.u64PhyAddr[0];
	src3addr = _pstFgFlag.u64PhyAddr[0];
	src2addr_1 = src2addr + (pstBgImg->u32Width * pstBgImg->u32Height * 16);
	set_isp_rdma(MOD_BGU, NULL, dev_id);
	if (src2addr) {
		bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel =
			src2addr & 0xffffffff;
		bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src2addr >> 32) & 0xffffffff;
		bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(bgmodel_0_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_0_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (src2addr_1) {
		bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel =
			src2addr_1 & 0xffffffff;
		bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src2addr_1 >> 32) & 0xffffffff;
		bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(bgmodel_1_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_BGMODEL_1_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (src3addr) {
		fg_wdma_ctl_c.BASE_ADDR.reg_basel = src3addr & 0xffffffff;
		fg_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(src3addr >> 32) & 0xffffffff;
		fg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		fg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		fg_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		fg_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}

	if (set_img_src1(pstCurImg, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		kfree(ive_filterop_c);
		return CVI_FAILURE;
	}


	writel(fg_wdma_ctl_c.BASE_ADDR.val,
		(IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(fg_wdma_ctl_c.SYS_CONTROL.val,
		(IVE_BLK_BA[dev_id].BG_UPDATE_FG_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel = src2addr & 0xffffffff;
	bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh =
		(src2addr >> 32) & 0xffffffff;
	bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(bgmodel_0_rdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_0_rdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_0_RDMA + ISP_DMA_CTL_SYS_CONTROL));

	bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel = src2addr_1 & 0xffffffff;
	bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh =
		(src2addr_1 >> 32) & 0xffffffff;
	bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(bgmodel_1_rdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(bgmodel_1_rdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_BGMODEL_1_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	if (src3addr) {
		fgflag_rdma_ctl_c.BASE_ADDR.reg_basel = src3addr & 0xffffffff;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh = (src3addr >> 32) & 0xffffffff;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		fgflag_rdma_ctl_c.BASE_ADDR.reg_basel = 0;
		fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(fgflag_rdma_ctl_c.BASE_ADDR.val,
		(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(fgflag_rdma_ctl_c.SYS_CONTROL.val,
		(IVE_BLK_BA[dev_id].BG_MATCH_FGFLAG_RDMA + ISP_DMA_CTL_SYS_CONTROL));
	ive_match_bg_c.REG_00.reg_matchbg_en = 1;
	ive_match_bg_c.REG_00.reg_matchbg_bypass_model = 1;
	ive_update_bg_c.REG_H04.reg_enable = 1;
	ive_update_bg_c.REG_H04.reg_updatebg_byp_model = 0;
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	// ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 5;
	writel(ive_match_bg_c.REG_00.val,
		   (IVE_BLK_BA[dev_id].BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	writel(ive_update_bg_c.REG_H04.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	//bypass filterop..
	// ive_filterop_c->REG_h10.reg_filterop_mode = 2;
	// ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 0;
	// ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 1;
	// ive_filterop_c->REG_28.reg_filterop_op2_erodila_en = 0;
	// writel(ive_filterop_c->REG_h10.val, (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	// writel(ive_filterop_c->REG_h14.val, (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	// writel(ive_filterop_c->REG_28.val, (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	ive_top_c->REG_h10.reg_filterop_top_enable = 0;
	ive_top_c->REG_h10.reg_bgm_top_enable = 1;
	ive_top_c->REG_h10.reg_bgu_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	if (pstChgSta->u64PhyAddr[0]) {
		chg_wdma_ctl_c.BASE_ADDR.reg_basel =
			pstChgSta->u64PhyAddr[0] & 0xffffffff;
		chg_wdma_ctl_c.SYS_CONTROL.reg_baseh =
			(pstChgSta->u64PhyAddr[0] >> 32) & 0xffffffff;
		chg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	} else {
		chg_wdma_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		chg_wdma_ctl_c.BASE_ADDR.reg_basel = 0;
		chg_wdma_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}
	writel(chg_wdma_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(chg_wdma_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].BG_UPDATE_CHG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("BG_UPDATE_BGMODEL_0_WDMA address: 0x%08x %08x\n",
			bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_UPDATE_BGMODEL_1_WDMA address: 0x%08x %08x\n",
			bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_UPDATE_FG_WDMA address: 0x%08x %08x\n",
			fg_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			fg_wdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_MATCH_BGMODEL_0_RDMA address: 0x%08x %08x\n",
			bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_MATCH_BGMODEL_1_RDMA address: 0x%08x %08x\n",
			bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_MATCH_FGFLAG_RDMA address: 0x%08x %08x\n",
			fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh,
			fgflag_rdma_ctl_c.BASE_ADDR.reg_basel);
		pr_info("BG_UPDATE_CHG_WDMA address: 0x%08x %08x\n",
			chg_wdma_ctl_c.SYS_CONTROL.reg_baseh,
			chg_wdma_ctl_c.BASE_ADDR.reg_basel);
	}

	g_debug_info.addr[WDMA_BGMODEL_0].addr_en = bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_l = bgmodel_0_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_0].addr_h = bgmodel_0_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_BGMODEL_1].addr_en = bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_l = bgmodel_1_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_BGMODEL_1].addr_h = bgmodel_1_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_FG].addr_en = fg_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_FG].addr_l = fg_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_FG].addr_h = fg_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_0].addr_en = bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_l = bgmodel_0_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_0].addr_h = bgmodel_0_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GMODEL_1].addr_en = bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_l = bgmodel_1_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_GMODEL_1].addr_h = bgmodel_1_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[RDMA_GFLAG].addr_en = fgflag_rdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_GFLAG].addr_l = fgflag_rdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_GFLAG].addr_h = fgflag_rdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_CHG].addr_en = chg_wdma_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_CHG].addr_l = chg_wdma_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_CHG].addr_h = chg_wdma_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	// set_odma(pstBgImg, ive_filterop_c, pstBgImg->u32Width, pstBgImg->u32Height, dev_id);
	ive_filterop_c->ODMA_REG_00.reg_dma_en = 0;
	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	// ive_filterop_c->REG_h14.reg_filterop_3ch_en = 1;
	writel(ive_filterop_c->ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));



	ret = emit_bgm_tile(ndev, false, false, MOD_BGU, ive_top_c,
			  ive_filterop_c, &img_in_c, NULL, NULL,
			  pstBgImg->u32Stride[0], pstBgImg->u32Width,
			  pstBgImg->u32Height, -1,
			  pstBgImg->enType, bInstant, pstStatData, NULL, NULL, NULL,
			  NULL, &fgflag_rdma_ctl_c, &bgmodel_0_rdma_ctl_c, &difffg_wdma_ctl_c,
			  &bgmodel_1_rdma_ctl_c, &fg_wdma_ctl_c,
			  &bgmodel_0_wdma_ctl_c, &bgmodel_1_wdma_ctl_c,
			  &chg_wdma_ctl_c, &ive_update_bg_c, dev_id);


	kfree(ive_filterop_c);
	kfree(ive_top_c);
	return ret;
#if 0
	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_BGU_MASK, MOD_BGU);

	stat.u32PixNum = readl(IVE_BLK_BA.BG_UPDATE_UPDATE_BG +
				   IVE_UPDATE_BG_REG_CTRL7); //9c2fe24
	stat.u32SumLum = readl(IVE_BLK_BA.BG_UPDATE_UPDATE_BG +
				   IVE_UPDATE_BG_REG_CTRL8); //9c2fe24

	//writel(0x00000000, (IVE_BLK_BA.BG_MATCH_IVE_MATCH_BG + IVE_MATCH_BG_REG_00));
	//writel(0x00000002, (IVE_BLK_BA.BG_UPDATE_UPDATE_BG + IVE_UPDATE_BG_REG_H04));
	kfree(ive_top_c);
	kfree(ive_filterop_c);
	return ret;
#endif
}

s32 cvi_ive_bernsen(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst,
			IVE_BERNSEN_CTRL_S *pstBernsenCtrl, bool bInstant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Bernsen\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	if (pstBernsenCtrl->u8WinSize != 3 && pstBernsenCtrl->u8WinSize != 5) {
		pr_err("not support u8WinSize %d, currently only support 3 or 5\n",
			   pstBernsenCtrl->u8WinSize);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	// top
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "Bernsen", dev_id);

	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));

	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 5;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0;
	writel(ive_filterop_c.REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));
	ive_filterop_c.REG_19.reg_filterop_bernsen_mode =
		pstBernsenCtrl->enMode;
	ive_filterop_c.REG_19.reg_filterop_bernsen_win5x5_en =
		pstBernsenCtrl->u8WinSize == 5;
	ive_filterop_c.REG_19.reg_filterop_bernsen_thr = pstBernsenCtrl->u8Thr;
	ive_filterop_c.REG_19.reg_filterop_u8ContrastThreshold =
		pstBernsenCtrl->u8ContrastThreshold;
	writel(ive_filterop_c.REG_19.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_19));

	set_isp_rdma(MOD_BERNSEN, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	// FIXME: check default is 0
	ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 0;
	writel(ive_top_c->REG_R2Y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_BERNSEN,
				bInstant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_BERNSEN, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 _cvi_ive_filter(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,
			bool bInstant, IVE_TOP_C *ive_top_c,
			IMG_IN_C *img_in_c, IVE_FILTEROP_C *ive_filterop_c,
			bool isEmit, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	IVE_GMM_C ive_gmm_c = _DEFINE_IVE_GMM_C;
	// top
	if (isEmit)
		ive_reset(ndev, ive_top_c, dev_id);
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "Filter", dev_id);

	ive_filterop_c->REG_4.reg_filterop_h_coef00 = pstFltCtrl->as8Mask[0];
	ive_filterop_c->REG_4.reg_filterop_h_coef01 = pstFltCtrl->as8Mask[1];
	ive_filterop_c->REG_4.reg_filterop_h_coef02 = pstFltCtrl->as8Mask[2];
	ive_filterop_c->REG_4.reg_filterop_h_coef03 = pstFltCtrl->as8Mask[3];
	ive_filterop_c->REG_5.reg_filterop_h_coef04 = pstFltCtrl->as8Mask[4];
	ive_filterop_c->REG_5.reg_filterop_h_coef10 = pstFltCtrl->as8Mask[5];
	ive_filterop_c->REG_5.reg_filterop_h_coef11 = pstFltCtrl->as8Mask[6];
	ive_filterop_c->REG_5.reg_filterop_h_coef12 = pstFltCtrl->as8Mask[7];
	ive_filterop_c->REG_6.reg_filterop_h_coef13 = pstFltCtrl->as8Mask[8];
	ive_filterop_c->REG_6.reg_filterop_h_coef14 = pstFltCtrl->as8Mask[9];
	ive_filterop_c->REG_6.reg_filterop_h_coef20 = pstFltCtrl->as8Mask[10];
	ive_filterop_c->REG_6.reg_filterop_h_coef21 = pstFltCtrl->as8Mask[11];
	ive_filterop_c->REG_7.reg_filterop_h_coef22 = pstFltCtrl->as8Mask[12];
	ive_filterop_c->REG_7.reg_filterop_h_coef23 = pstFltCtrl->as8Mask[13];
	ive_filterop_c->REG_7.reg_filterop_h_coef24 = pstFltCtrl->as8Mask[14];
	ive_filterop_c->REG_7.reg_filterop_h_coef30 = pstFltCtrl->as8Mask[15];
	ive_filterop_c->REG_8.reg_filterop_h_coef31 = pstFltCtrl->as8Mask[16];
	ive_filterop_c->REG_8.reg_filterop_h_coef32 = pstFltCtrl->as8Mask[17];
	ive_filterop_c->REG_8.reg_filterop_h_coef33 = pstFltCtrl->as8Mask[18];
	ive_filterop_c->REG_8.reg_filterop_h_coef34 = pstFltCtrl->as8Mask[19];
	ive_filterop_c->REG_9.reg_filterop_h_coef40 = pstFltCtrl->as8Mask[20];
	ive_filterop_c->REG_9.reg_filterop_h_coef41 = pstFltCtrl->as8Mask[21];
	ive_filterop_c->REG_9.reg_filterop_h_coef42 = pstFltCtrl->as8Mask[22];
	ive_filterop_c->REG_9.reg_filterop_h_coef43 = pstFltCtrl->as8Mask[23];
	ive_filterop_c->REG_10.reg_filterop_h_coef44 = pstFltCtrl->as8Mask[24];
	ive_filterop_c->REG_10.reg_filterop_h_norm = pstFltCtrl->u8Norm;
	writel(ive_filterop_c->REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c->REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c->REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c->REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c->REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c->REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c->REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));
	switch (pstSrc->enType) {
	case IVE_IMAGE_TYPE_U8C1:
		//test1:rgb:set gmm_gmm2_yonly:0
		//y_only:set gmm_gmm2_yonly:1
		ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly = 1;
		writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		// pass, filter case
		// TODO: check 1 channels setting
		ive_filterop_c->REG_h10.reg_filterop_mode = 3;
		ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 1;
		ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 1;
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
	case IVE_IMAGE_TYPE_YUV422SP:
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		//test1:rgb:set gmm_gmm2_yonly:0
		//y_only:set gmm_gmm2_yonly:1
		ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly = 0;
		writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

		ive_filterop_c->REG_h10.reg_filterop_mode = 1;
		ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
		ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 0;
		ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enable = 0;
		ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enmode = 0;
		ive_filterop_c->REG_h14.reg_filterop_3ch_en = 1;
		writel(ive_top_c->REG_R2Y4_14.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));
		writel(ive_filterop_c->REG_h1c8.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));
		break;
	default:
		pr_err("Invalid Image enType %d\n", pstSrc->enType);
		return CVI_FAILURE;
	}
	writel(ive_filterop_c->REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	set_isp_rdma(MOD_FILTER3CH, NULL, dev_id);
	if (isEmit) {

		if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
		// trigger filterop
		//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->REG_20.reg_frame2op_op_mode = 5;
		// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
		ive_top_c->REG_h10.reg_filterop_top_enable = 1;
		writel(ive_top_c->REG_20.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
		writel(ive_top_c->REG_h80.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		writel(ive_top_c->REG_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		if (pstDst->enType == IVE_IMAGE_TYPE_U8C1) {
			set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);
			ive_filterop_c->ODMA_REG_00.reg_dma_en = 0;
			writel(ive_filterop_c->ODMA_REG_00.val,
				   (IVE_BLK_BA[dev_id].FILTEROP +
				IVE_FILTEROP_ODMA_REG_00));

			ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
			ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
			writel(ive_filterop_c->REG_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

			if (pstSrc->u32Width > 480) {
				ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
					&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
					NULL, pstDst, NULL, true, 1, false, 1, false, MOD_FILTER3CH,
					bInstant, dev_id);
			} else {
				ret = cvi_ive_go(
					ndev, ive_top_c, bInstant,
					IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
					MOD_FILTER3CH, dev_id);
			}
		} else {
			img_in_c->REG_00.reg_auto_csc_en = 0;
			writel(img_in_c->REG_00.val,
				   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

			set_odma(pstDst, ive_filterop_c, pstDst->u32Width,
				pstDst->u32Height, dev_id);

			// NOTICE: test img_in = odma
			ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
			writel(ive_filterop_c->REG_h14.val,
				   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

			if (pstSrc->u32Width > 480) {
				ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
					NULL, NULL, NULL, NULL, pstSrc, NULL,
					NULL, pstDst, NULL, false, 1, false, 1, true, MOD_FILTER3CH,
					bInstant, dev_id);
			} else {
				ret = cvi_ive_go(
					ndev, ive_top_c, bInstant,
					IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
					MOD_FILTER3CH, dev_id);
			}
		}
	}
	return ret;
}

s32 cvi_ive_filter(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,
			   bool bInstant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Filter\n");
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ret = _cvi_ive_filter(ndev, pstSrc, pstDst, pstFltCtrl, bInstant,
				ive_top_c, &img_in_c, &ive_filterop_c, true, dev_id);

	kfree(ive_top_c);
	return ret;
}

s32 _cvi_ive_csc(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl,
			 bool bInstant, IVE_TOP_C *ive_top_c,
			 IMG_IN_C *img_in_c, IVE_FILTEROP_C *ive_filterop_c,
			 bool isEmit, s32 dev_id)
{
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	IVE_GMM_C ive_gmm_c = _DEFINE_IVE_GMM_C;
	s32 ret = CVI_SUCCESS;
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
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "CSC", dev_id);

	ive_filterop_c->REG_h10.reg_filterop_mode = 1;
	ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enable = 1;
	ive_filterop_c->REG_h1c8.reg_filterop_op2_csc_enmode =
		pstCscCtrl->enMode;
	ive_filterop_c->REG_h14.reg_filterop_3ch_en = 0;
	writel(ive_filterop_c->REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->REG_h1c8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));

	switch (pstCscCtrl->enMode) {
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
		ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly = 0;
		writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));


		ive_filterop_c->REG_h194
			.reg_filterop_op2_csc_coeff_sw_update = 0;
		ive_filterop_c->REG_CSC_COEFF_0
			.reg_filterop_op2_csc_coeff_sw_00 = tbl[0];
		ive_filterop_c->REG_CSC_COEFF_1
			.reg_filterop_op2_csc_coeff_sw_01 = tbl[1];
		ive_filterop_c->REG_CSC_COEFF_2
			.reg_filterop_op2_csc_coeff_sw_02 = tbl[2];
		ive_filterop_c->REG_CSC_COEFF_3
			.reg_filterop_op2_csc_coeff_sw_03 = tbl[3];
		ive_filterop_c->REG_CSC_COEFF_4
			.reg_filterop_op2_csc_coeff_sw_04 = tbl[4];
		ive_filterop_c->REG_CSC_COEFF_5
			.reg_filterop_op2_csc_coeff_sw_05 = tbl[5];
		ive_filterop_c->REG_CSC_COEFF_6
			.reg_filterop_op2_csc_coeff_sw_06 = tbl[6];
		ive_filterop_c->REG_CSC_COEFF_7
			.reg_filterop_op2_csc_coeff_sw_07 = tbl[7];
		ive_filterop_c->REG_CSC_COEFF_8
			.reg_filterop_op2_csc_coeff_sw_08 = tbl[8];
		ive_filterop_c->REG_CSC_COEFF_9
			.reg_filterop_op2_csc_coeff_sw_09 = tbl[9];
		ive_filterop_c->REG_CSC_COEFF_A
			.reg_filterop_op2_csc_coeff_sw_10 = tbl[10];
		ive_filterop_c->REG_CSC_COEFF_B
			.reg_filterop_op2_csc_coeff_sw_11 = tbl[11];
		writel(ive_filterop_c->REG_h194.val,
				(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H194));
		writel(ive_filterop_c->REG_CSC_COEFF_0.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_0));
		writel(ive_filterop_c->REG_CSC_COEFF_1.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_1));
		writel(ive_filterop_c->REG_CSC_COEFF_2.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_2));
		writel(ive_filterop_c->REG_CSC_COEFF_3.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_3));
		writel(ive_filterop_c->REG_CSC_COEFF_4.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_4));
		writel(ive_filterop_c->REG_CSC_COEFF_5.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_5));
		writel(ive_filterop_c->REG_CSC_COEFF_6.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_6));
		writel(ive_filterop_c->REG_CSC_COEFF_7.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_7));
		writel(ive_filterop_c->REG_CSC_COEFF_8.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_8));
		writel(ive_filterop_c->REG_CSC_COEFF_9.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_9));
		writel(ive_filterop_c->REG_CSC_COEFF_A.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_A));
		writel(ive_filterop_c->REG_CSC_COEFF_B.val,
				(IVE_BLK_BA[dev_id].FILTEROP +
			IVE_FILTEROP_REG_CSC_COEFF_B));
	}

	if (isEmit) {
		img_in_c->REG_068.reg_ip_clr_w1t = 1;
		writel(img_in_c->REG_068.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));
		//udelay(3);
		img_in_c->REG_068.reg_ip_clr_w1t = 0;
		writel(img_in_c->REG_068.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_068));

		set_isp_rdma(MOD_CSC, NULL, dev_id);
		if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
		img_in_c->REG_00.reg_auto_csc_en = 0;
		writel(img_in_c->REG_00.val,
			   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

		ive_top_c->REG_h10.reg_filterop_top_enable = 1;
		writel(ive_top_c->REG_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
		set_odma(pstDst, ive_filterop_c, pstDst->u32Width,
			pstDst->u32Height, dev_id);

		ive_filterop_c->REG_h14.reg_op_y_wdma_en = 0;
		writel(ive_filterop_c->REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		if (pstSrc->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
				NULL, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, false, 1, false, 1, true, MOD_CSC,
				bInstant, dev_id);
		} else {
			ret = cvi_ive_go(
				ndev, ive_top_c, bInstant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
				MOD_CSC, dev_id);
		}
	}
	return ret;
}

s32 cvi_ive_csc(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl,
			bool bInstant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_CSC\n");
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ret = _cvi_ive_csc(ndev, pstSrc, pstDst, pstCscCtrl, bInstant,
				ive_top_c, &img_in_c, &ive_filterop_c, true, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_FilterAndCSC(struct cvi_ive_device *ndev,
				 IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
				 IVE_FILTER_AND_CSC_CTRL_S *pstFltCscCtrl,
				 bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	IVE_CSC_CTRL_S stCscCtrl;
	IVE_FILTER_CTRL_S stFltCtrl;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_FilterAndCSC\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	if (pstSrc->enType == IVE_IMAGE_TYPE_YUV420SP ||
		pstSrc->enType == IVE_IMAGE_TYPE_YUV422SP) {
	} else {
		pr_err("only support input fmt YUV420SP/YUV422SP??\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	// set filter
	memcpy(stFltCtrl.as8Mask, pstFltCscCtrl->as8Mask, sizeof(s8) * 25);
	stFltCtrl.u8Norm = pstFltCscCtrl->u8Norm;
	_cvi_ive_filter(ndev, pstSrc, pstDst, &stFltCtrl, bInstant, ive_top_c,
			&img_in_c, &ive_filterop_c, /*isEmit=*/false, dev_id);
	// set csc
	stCscCtrl.enMode = pstFltCscCtrl->enMode;
	_cvi_ive_csc(ndev, pstSrc, pstDst, &stCscCtrl, bInstant, ive_top_c,
			 &img_in_c, &ive_filterop_c, /*isEmit=*/false, dev_id);
	strcpy(g_debug_info.op_name, "FilterAndCSC");
	ive_filterop_c.REG_h14.reg_filterop_3ch_en = 1;
	ive_filterop_c.REG_h1c8.reg_filterop_op2_csc_enable = 1;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.REG_h1c8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));

	set_isp_rdma(MOD_FILTERCSC, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	img_in_c.REG_00.reg_auto_csc_en = 0;
	writel(img_in_c.REG_00.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

	// NOTICE: need to first set it
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_odma(pstDst, &ive_filterop_c, pstDst->u32Width, pstDst->u32Height, dev_id);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
			NULL, NULL, NULL, NULL, pstSrc, NULL,
			NULL, pstDst, NULL, false, 1, false, 1, true, MOD_FILTERCSC,
			bInstant, dev_id);
	} else {
		ret = cvi_ive_go(
			ndev, ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
			MOD_FILTERCSC, dev_id);
	}
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_hist(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_MEM_INFO_S *pstDst, bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_hist_ctl_c);
	ISP_DMA_CTL_C wdma_hist_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_HIST_C(ive_hist_c);
	IVE_HIST_C ive_hist_c = _DEFINE_IVE_HIST_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Hist\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_mem("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "Hist", dev_id);

	set_isp_rdma(MOD_HIST, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	ive_filterop_c.ODMA_REG_00.reg_dma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

	ive_hist_c.REG_0.reg_ive_hist_enable = 1;
	ive_top_c->REG_h10.reg_hist_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_hist_c.REG_0.val, (IVE_BLK_BA[dev_id].HIST + IVE_HIST_REG_0));

	wdma_hist_ctl_c.BASE_ADDR.reg_basel = pstDst->u64PhyAddr & 0xffffffff;
	wdma_hist_ctl_c.SYS_CONTROL.reg_baseh =
		((u64)pstDst->u64PhyAddr >> 32) & 0xffffffff;
	wdma_hist_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Hist wdma_hist_ctl_c address: 0x%08x %08x\n",
			wdma_hist_ctl_c.SYS_CONTROL.reg_baseh,
			wdma_hist_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[WDMA_HIST].addr_en = wdma_hist_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_HIST].addr_l = wdma_hist_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_HIST].addr_h = wdma_hist_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	writel(wdma_hist_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].HIST_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_hist_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].HIST_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_HIST_MASK, MOD_HIST, dev_id);

	ive_hist_c.REG_0.reg_ive_hist_enable = 0;
	ive_hist_c.REG_0.reg_force_clk_enable = 1;
	writel(ive_hist_c.REG_0.val, (IVE_BLK_BA[dev_id].HIST + IVE_HIST_REG_0));
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_sobel(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			  IVE_SOBEL_CTRL_S *pstSobelCtrl, bool bInstant, s32 dev_id)
{
	s32 yunit = 2;
	s32 ret = CVI_SUCCESS;
	IVE_DST_IMAGE_S *firstOut = pstDstH;
	IVE_DST_IMAGE_S *secondOut = pstDstV;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_c_ctl_c);
	ISP_DMA_CTL_C wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Sobel\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDstH", pstDstH);
		dump_ive_image("pstDstV", pstDstV);
	}
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "Sobel", dev_id);

	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.REG_h10.reg_filterop_mode = 9;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.REG_4.reg_filterop_h_coef00 = pstSobelCtrl->as8Mask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 = pstSobelCtrl->as8Mask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 = pstSobelCtrl->as8Mask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 = pstSobelCtrl->as8Mask[3];
	ive_filterop_c.REG_5.reg_filterop_h_coef04 = pstSobelCtrl->as8Mask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 = pstSobelCtrl->as8Mask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 = pstSobelCtrl->as8Mask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 = pstSobelCtrl->as8Mask[7];
	ive_filterop_c.REG_6.reg_filterop_h_coef13 = pstSobelCtrl->as8Mask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 = pstSobelCtrl->as8Mask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 = pstSobelCtrl->as8Mask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 = pstSobelCtrl->as8Mask[11];
	ive_filterop_c.REG_7.reg_filterop_h_coef22 = pstSobelCtrl->as8Mask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 = pstSobelCtrl->as8Mask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 = pstSobelCtrl->as8Mask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 = pstSobelCtrl->as8Mask[15];
	ive_filterop_c.REG_8.reg_filterop_h_coef31 = pstSobelCtrl->as8Mask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 = pstSobelCtrl->as8Mask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 = pstSobelCtrl->as8Mask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 = pstSobelCtrl->as8Mask[19];
	ive_filterop_c.REG_9.reg_filterop_h_coef40 = pstSobelCtrl->as8Mask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 = pstSobelCtrl->as8Mask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 = pstSobelCtrl->as8Mask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 = pstSobelCtrl->as8Mask[23];
	ive_filterop_c.REG_10.reg_filterop_h_coef44 = pstSobelCtrl->as8Mask[24];

	writel(ive_filterop_c.REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	if (pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_BOTH ||
		pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_HOR ||
		pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_VER) {
		// valid
		// "0 : h , v  -> wdma_y wdma_c will be active 1 :h only -> wdma_y
		// 2: v only -> wdma_c 3. h , v pack => {v ,h} -> wdma_y "
		ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl =
			(int)pstSobelCtrl->enOutCtrl;
		writel(ive_filterop_c.REG_110.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));
	} else {
		pr_err("[IVE] not support enOutCtrl\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	set_isp_rdma(MOD_SOBEL, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));

	// "0 : U8 1 : S16 2 : U16"
	// FIXME: check enable in Sobel
	ive_filterop_c.REG_110.reg_filterop_map_enmode = 1;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_filterop_c.REG_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));

	if ((int)pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_BOTH) {
		set_img_dst2(secondOut, &wdma_c_ctl_c, dev_id);
		set_img_dst1(firstOut, &wdma_y_ctl_c, dev_id);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL,
					&wdma_c_ctl_c, NULL, pstSrc, NULL, NULL,
					firstOut, secondOut, true, yunit, true, 2,
					false, 0, bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
		//ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	} else if ((int)pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_VER) {
		set_img_dst1(NULL, NULL, dev_id);
		set_img_dst2(secondOut, &wdma_c_ctl_c, dev_id);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, NULL, NULL, &wdma_c_ctl_c,
					NULL, pstSrc, NULL, NULL, firstOut,
					secondOut, false, yunit, true, 2, false, 0,
					bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	} else {
		set_img_dst1(firstOut, &wdma_y_ctl_c, dev_id);
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, firstOut,
					secondOut, true, yunit, false, 1, false, MOD_SOBEL,
					bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}

	if ((int)pstSobelCtrl->enOutCtrl == 0) {
		ret = cvi_ive_go(
			ndev, ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK |
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
			MOD_SOBEL, dev_id);
	} else if ((int)pstSobelCtrl->enOutCtrl == 1) {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_SOBEL, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
				 MOD_SOBEL, dev_id);
	}

	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_mag_and_ang(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDstMag,
			  IVE_DST_IMAGE_S *pstDstAng,
			  IVE_MAG_AND_ANG_CTRL_S *pstMagAndAngCtrl,
			  bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_c_ctl_c);
	ISP_DMA_CTL_C wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_MagAndAng\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDstMag", pstDstMag);
		dump_ive_image("pstDstAng", pstDstAng);
	}
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "MagAndAng", dev_id);

	ive_filterop_c.REG_h10.reg_filterop_mode = 7;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	ive_filterop_c.REG_4.reg_filterop_h_coef00 =
		pstMagAndAngCtrl->as8Mask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 =
		pstMagAndAngCtrl->as8Mask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 =
		pstMagAndAngCtrl->as8Mask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 =
		pstMagAndAngCtrl->as8Mask[3];
	ive_filterop_c.REG_5.reg_filterop_h_coef04 =
		pstMagAndAngCtrl->as8Mask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 =
		pstMagAndAngCtrl->as8Mask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 =
		pstMagAndAngCtrl->as8Mask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 =
		pstMagAndAngCtrl->as8Mask[7];
	ive_filterop_c.REG_6.reg_filterop_h_coef13 =
		pstMagAndAngCtrl->as8Mask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 =
		pstMagAndAngCtrl->as8Mask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 =
		pstMagAndAngCtrl->as8Mask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 =
		pstMagAndAngCtrl->as8Mask[11];
	ive_filterop_c.REG_7.reg_filterop_h_coef22 =
		pstMagAndAngCtrl->as8Mask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 =
		pstMagAndAngCtrl->as8Mask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 =
		pstMagAndAngCtrl->as8Mask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 =
		pstMagAndAngCtrl->as8Mask[15];
	ive_filterop_c.REG_8.reg_filterop_h_coef31 =
		pstMagAndAngCtrl->as8Mask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 =
		pstMagAndAngCtrl->as8Mask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 =
		pstMagAndAngCtrl->as8Mask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 =
		pstMagAndAngCtrl->as8Mask[19];
	ive_filterop_c.REG_9.reg_filterop_h_coef40 =
		pstMagAndAngCtrl->as8Mask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 =
		pstMagAndAngCtrl->as8Mask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 =
		pstMagAndAngCtrl->as8Mask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 =
		pstMagAndAngCtrl->as8Mask[23];
	ive_filterop_c.REG_10.reg_filterop_h_coef44 =
		pstMagAndAngCtrl->as8Mask[24];

	ive_filterop_c.REG_11.reg_filterop_v_coef00 =
		-1 * pstMagAndAngCtrl->as8Mask[0];
	ive_filterop_c.REG_11.reg_filterop_v_coef01 =
		-1 * pstMagAndAngCtrl->as8Mask[5];
	ive_filterop_c.REG_11.reg_filterop_v_coef02 =
		-1 * pstMagAndAngCtrl->as8Mask[10];
	ive_filterop_c.REG_11.reg_filterop_v_coef03 =
		-1 * pstMagAndAngCtrl->as8Mask[15];
	ive_filterop_c.REG_12.reg_filterop_v_coef04 =
		-1 * pstMagAndAngCtrl->as8Mask[20];
	ive_filterop_c.REG_12.reg_filterop_v_coef10 =
		-1 * pstMagAndAngCtrl->as8Mask[1];
	ive_filterop_c.REG_12.reg_filterop_v_coef11 =
		-1 * pstMagAndAngCtrl->as8Mask[6];
	ive_filterop_c.REG_12.reg_filterop_v_coef12 =
		-1 * pstMagAndAngCtrl->as8Mask[11];
	ive_filterop_c.REG_13.reg_filterop_v_coef13 =
		-1 * pstMagAndAngCtrl->as8Mask[16];
	ive_filterop_c.REG_13.reg_filterop_v_coef14 =
		-1 * pstMagAndAngCtrl->as8Mask[21];
	ive_filterop_c.REG_13.reg_filterop_v_coef20 =
		-1 * pstMagAndAngCtrl->as8Mask[2];
	ive_filterop_c.REG_13.reg_filterop_v_coef21 =
		-1 * pstMagAndAngCtrl->as8Mask[7];
	ive_filterop_c.REG_14.reg_filterop_v_coef22 =
		-1 * pstMagAndAngCtrl->as8Mask[12];
	ive_filterop_c.REG_14.reg_filterop_v_coef23 =
		-1 * pstMagAndAngCtrl->as8Mask[17];
	ive_filterop_c.REG_14.reg_filterop_v_coef24 =
		-1 * pstMagAndAngCtrl->as8Mask[22];
	ive_filterop_c.REG_14.reg_filterop_v_coef30 =
		-1 * pstMagAndAngCtrl->as8Mask[3];
	ive_filterop_c.REG_15.reg_filterop_v_coef31 =
		-1 * pstMagAndAngCtrl->as8Mask[8];
	ive_filterop_c.REG_15.reg_filterop_v_coef32 =
		-1 * pstMagAndAngCtrl->as8Mask[13];
	ive_filterop_c.REG_15.reg_filterop_v_coef33 =
		-1 * pstMagAndAngCtrl->as8Mask[18];
	ive_filterop_c.REG_15.reg_filterop_v_coef34 =
		-1 * pstMagAndAngCtrl->as8Mask[23];
	ive_filterop_c.REG_16.reg_filterop_v_coef40 =
		-1 * pstMagAndAngCtrl->as8Mask[4];
	ive_filterop_c.REG_16.reg_filterop_v_coef41 =
		-1 * pstMagAndAngCtrl->as8Mask[9];
	ive_filterop_c.REG_16.reg_filterop_v_coef42 =
		-1 * pstMagAndAngCtrl->as8Mask[14];
	ive_filterop_c.REG_16.reg_filterop_v_coef43 =
		-1 * pstMagAndAngCtrl->as8Mask[19];
	ive_filterop_c.REG_17.reg_filterop_v_coef44 =
		-1 * pstMagAndAngCtrl->as8Mask[24];

	writel(ive_filterop_c.REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	writel(ive_filterop_c.REG_11.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_11));
	writel(ive_filterop_c.REG_12.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_12));
	writel(ive_filterop_c.REG_13.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_13));
	writel(ive_filterop_c.REG_14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_14));
	writel(ive_filterop_c.REG_15.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_15));
	writel(ive_filterop_c.REG_16.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_16));
	writel(ive_filterop_c.REG_17.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_17));

	ive_filterop_c.REG_18.reg_filterop_mode_trans = 0;
	ive_filterop_c.REG_18.reg_filterop_mag_thr = pstMagAndAngCtrl->u16Thr;
	writel(ive_filterop_c.REG_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));

	set_isp_rdma(MOD_MAG, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));

	// "0 : U8 1 : S16 2 : U16"
	ive_filterop_c.REG_110.reg_filterop_map_enmode = 1;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	if (pstMagAndAngCtrl->enOutCtrl ==
		IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG) {
		//setDMA(pstSrc, pstDstAng, NULL, pstDstMag);
		ive_filterop_c.REG_110.reg_filterop_magang_out_ctrl = 1;
		set_img_dst2(pstDstMag, &wdma_c_ctl_c, dev_id);
		set_img_dst1(pstDstAng, &wdma_y_ctl_c, dev_id);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.REG_110.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL,
					&wdma_c_ctl_c, NULL, pstSrc, NULL, NULL,
					pstDstAng, pstDstMag, true, 1, true, 2,
					false, MOD_MAG, bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	} else {
		//setDMA(pstSrc, NULL, NULL, pstDstMag);
		ive_filterop_c.REG_110.reg_filterop_magang_out_ctrl = 0;
		set_img_dst2(pstDstMag, &wdma_c_ctl_c, dev_id);
		//set_img_dst1(pstDstAng, NULL);
		// enable it
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
		writel(ive_filterop_c.REG_110.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		if (pstSrc->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, NULL, NULL, &wdma_c_ctl_c,
					NULL, pstSrc, NULL, NULL, NULL,
					pstDstMag, false, 1, true, 2, false, MOD_MAG,
					bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}

	if (pstMagAndAngCtrl->enOutCtrl ==
		IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG) {
		ret = cvi_ive_go(
			ndev, ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK |
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
			MOD_MAG, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
				 MOD_MAG, dev_id);
	}

	ive_filterop_c.REG_18.reg_filterop_mode_trans = 1;
	writel(ive_filterop_c.REG_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_map(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			u16 *pstMap, IVE_DST_IMAGE_S *pstDst,
			IVE_MAP_CTRL_S *pstMapCtrl, bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	s32 l = 0;
	u16 u16Word;
	u16 *_pu16Ptr = NULL;
	u8 *pu8Ptr = NULL;
	u16 *pu16Ptr = NULL;
	int c_unit = 1;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_MAP_C(ive_map_c);
	IVE_MAP_C ive_map_c = _DEFINE_IVE_MAP_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_c_ctl_c);
	ISP_DMA_CTL_C wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	//IVE_BLK_BA_MAP
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Map\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		// dump_ive_mem("pstMap", pstMap);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "Map", dev_id);

	ive_filterop_c.REG_h10.reg_filterop_mode = 11;
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.REG_110.reg_filterop_map_enmode = pstMapCtrl->enMode;

	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.REG_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));


	pu8Ptr = (u8 *)pstMap;
	pu16Ptr = (u16 *)pstMap;

	ive_map_c.REG_1.reg_lut_prog_en = 1;
	ive_map_c.REG_2.reg_lut_st_addr = 0;
	ive_map_c.REG_2.reg_lut_st_w1t = 1;
	ive_map_c.REG_1.reg_lut_wsel =
		(pstMapCtrl->enMode == IVE_MAP_MODE_U8) ? 0 : 1;
	writel(ive_map_c.REG_1.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_1));

	writel(ive_map_c.REG_2.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_2));
	for (l = 0; l < 256; l++) {
		if (pstMapCtrl->enMode == IVE_MAP_MODE_U8) {
			u16Word = (0x00ff & *pu8Ptr);
			pu8Ptr++;
		} else {
			u16Word = *pu16Ptr;
			// if (l >= 0xB0 && l <= 0xBF)
			// 	u16Word = 0x470 + (l - 0xB0);
			// else
			// 	u16Word = (0x00ff & l);
			pu16Ptr++;
		}
		ive_map_c.REG_3.reg_lut_wdata = u16Word;
		ive_map_c.REG_3.reg_lut_w1t = 1;

		writel(ive_map_c.REG_3.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_3));
	}
	ive_map_c.REG_1.reg_lut_rsel =
		(pstMapCtrl->enMode == IVE_MAP_MODE_U8) ? 0 : 1;
	ive_map_c.REG_1.reg_lut_prog_en = 0;

	writel(ive_map_c.REG_1.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_1));

	set_isp_rdma(MOD_MAP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		vfree(_pu16Ptr);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;

	ive_top_c->REG_h10.reg_map_top_enable = 1;
	ive_map_c.REG_0.reg_ip_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_map_c.REG_0.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_0));

	set_img_dst2(pstDst, &wdma_c_ctl_c, dev_id);

	// enable it
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		c_unit = (pstMapCtrl->enMode == IVE_MAP_MODE_U8) ? 1 : 2;
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
			NULL, NULL, &wdma_c_ctl_c, NULL, pstSrc, NULL,
			NULL, pstDst, NULL, false, 1, true, c_unit, false, MOD_MAP,
			bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK, MOD_MAP, dev_id);
	}

	ive_map_c.REG_0.reg_ip_enable = 0;
	writel(ive_map_c.REG_0.val, (IVE_BLK_BA[dev_id].MAP + IVE_MAP_REG_0));

	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_ncc(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_NCC_DST_MEM_S *pstDst,
			bool bInstant, s32 dev_id)

{
	s32 mode = 0;
	s32 ret = CVI_SUCCESS;
	IVE_NCC_DST_MEM_S ncc;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	//IVE_BLK_BA_MAP
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_NCC\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		// dump_ive_mem("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "NCC", dev_id);

	set_isp_rdma(MOD_NCC, NULL, dev_id);
	if (set_img_src1(pstSrc1, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	set_img_src2(pstSrc2, NULL, dev_id);

	mode = ive_get_mod_u8(pstSrc1->enType);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	ive_top_c->REG_3.reg_ive_rdma_img1_mod_u8 = mode;

	ive_top_c->REG_3.reg_imgmux_img0_sel = 0;
	ive_top_c->REG_3.reg_ive_rdma_img1_en = 1;
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 5;
	ive_top_c->REG_14.reg_csc_enable = 0;
	ive_top_c->REG_h10.reg_filterop_top_enable = 0;
	ive_top_c->REG_h10.reg_csc_top_enable = 0;
	ive_top_c->REG_h10.reg_ncc_top_enable = 1;
	ive_top_c->REG_h10.reg_rdma_img1_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_14.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_14));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	ive_filterop_c.ODMA_REG_00.reg_dma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

	ndev->cur_optype = MOD_NCC;
	// g_pDst = pstDst;

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_NCC_MASK, MOD_NCC, dev_id);

	if (ret == CVI_SUCCESS) {
		memset((void *)&ncc, 0xCD, sizeof(IVE_NCC_DST_MEM_S));
		ncc.u64Numerator = (u64)(readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_01))
					<< 32 |
				readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_00);

		ncc.u64QuadSum1 = (u64)(readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_03))
					<< 32 |
				readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_02);

		ncc.u64QuadSum2 = (u64)(readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_05))
					<< 32 |
				readl(IVE_BLK_BA[dev_id].NCC + IVE_NCC_REG_NCC_04);

		memcpy(pstDst, &ncc, sizeof(IVE_NCC_DST_MEM_S));

	}
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_integ(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_MEM_INFO_S *pstDst,
			  IVE_INTEG_CTRL_S *pstIntegCtrl, bool bInstant, s32 dev_id)
{
	s32 ret = 0;
	IVE_DST_IMAGE_S _pstDst;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_INTG_C(ive_intg_c);
	IVE_INTG_C ive_intg_c = _DEFINE_IVE_INTG_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_integ_ctl_c);
	ISP_DMA_CTL_C wdma_integ_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	//IVE_BLK_BA_MAP
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Integ\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_mem("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "Integ", dev_id);

	ive_intg_c.REG_0.reg_ive_intg_ctrl = (int)pstIntegCtrl->enOutCtrl;
	ive_intg_c.REG_1.reg_ive_intg_stride = (u16)pstSrc->u32Stride[0];
	writel(ive_intg_c.REG_0.val, (IVE_BLK_BA[dev_id].INTG + IVE_INTG_REG_0));
	writel(ive_intg_c.REG_1.val, (IVE_BLK_BA[dev_id].INTG + IVE_INTG_REG_1));

	set_isp_rdma(MOD_INTEG, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	ive_filterop_c.ODMA_REG_00.reg_dma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.ODMA_REG_00.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));

	_pstDst.u64PhyAddr[0] = pstDst->u64PhyAddr;

	wdma_integ_ctl_c.BASE_ADDR.reg_basel =
		_pstDst.u64PhyAddr[0] & 0xffffffff;
	wdma_integ_ctl_c.SYS_CONTROL.reg_baseh =
		(_pstDst.u64PhyAddr[0] >> 32) & 0xffffffff;
	wdma_integ_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Integ wdma_integ_ctl_c address: 0x%08x %08x\n",
			wdma_integ_ctl_c.SYS_CONTROL.reg_baseh,
			wdma_integ_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[WDMA_INTEG].addr_en = wdma_integ_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_INTEG].addr_l = wdma_integ_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_INTEG].addr_h = wdma_integ_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	writel(wdma_integ_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].INTG_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_integ_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].INTG_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	ive_intg_c.REG_0.reg_ive_intg_enable = 1;
	writel(ive_intg_c.REG_0.val, (IVE_BLK_BA[dev_id].INTG + IVE_INTG_REG_0));

	// trigger
	ive_top_c->REG_h10.reg_intg_top_enable = 1;
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 0;
	ive_top_c->REG_14.reg_csc_enable = 0;
	ive_top_c->REG_h10.reg_csc_top_enable = 0;

	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_14.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_14));

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			  IVE_TOP_REG_FRAME_DONE_INTG_MASK, MOD_INTEG, dev_id);

	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_lbp(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_LBP_CTRL_S *pstLbpCtrl,
			bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	//IVE_BLK_BA_MAP
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_LBP\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "LBP", dev_id);

	// setting
	ive_filterop_c.REG_20.reg_filterop_lbp_u8bit_thr =
		pstLbpCtrl->un8BitThr.u8Val;
	ive_filterop_c.REG_20.reg_filterop_lbp_s8bit_thr =
		pstLbpCtrl->un8BitThr.s8Val;
	ive_filterop_c.REG_20.reg_filterop_lbp_enmode = pstLbpCtrl->enMode;
	writel(ive_filterop_c.REG_20.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_20));

	// filterop
	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 6;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	set_isp_rdma(MOD_LBP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_LBP,
				bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_LBP, dev_id);
	}
	kfree(ive_top_c);
	return ret;
}

s32 _cvi_ive_16bit_to_8bit(IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
				 IVE_TOP_C *ive_top_c, IVE_FILTEROP_C *ive_filterop_c,
				 ISP_DMA_CTL_C *wdma_y_ctl_c, ISP_DMA_CTL_C *rdma_eigval_ctl_c, s32 dev_id)
{
	ive_top_c->REG_h10.reg_rdma_eigval_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_isp_rdma(MOD_16To8, NULL, dev_id);
	set_rdma_eigval(pstSrc, rdma_eigval_ctl_c, dev_id);

	ive_top_c->REG_3.reg_mapmux_rdma_sel = 1;
	ive_top_c->REG_3.reg_ive_rdma_eigval_en = 1; //Here?
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;

	// bypass filterop...
	ive_filterop_c->REG_h10.reg_filterop_mode = 2;
	ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 0; //sw_ovw; bypass op1
	ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 1;
	ive_filterop_c->REG_28.reg_filterop_op2_erodila_en = 0; //bypass op2
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_filterop_c->REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	set_img_dst1(pstDst, wdma_y_ctl_c, dev_id);

	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	return CVI_SUCCESS;
}

s32 cvi_ive_16bit_to_8bit(struct cvi_ive_device *ndev,
				IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
				IVE_16BIT_TO_8BIT_CTRL_S *pst16BitTo8BitCtrl,
				bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	u32 tmp;
	u16 u8Num_div_u16Den;

	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_eigval_ctl_c);
	ISP_DMA_CTL_C rdma_eigval_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_16BitTo8Bit\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	tmp = (((u32)pst16BitTo8BitCtrl->u8Numerator << 16) +
		   (pst16BitTo8BitCtrl->u16Denominator >> 1)) /
		  pst16BitTo8BitCtrl->u16Denominator;
	u8Num_div_u16Den = (u16)(tmp & 0xffff);
	ive_top_c->REG_h130.reg_thresh_top_mod = 1;
	ive_top_c->REG_h130.reg_thresh_thresh_en = 0;
	ive_top_c->REG_h10.reg_thresh_top_enable = 1;
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "16BitTo8Bit", dev_id);

	ive_top_c->REG_h134.reg_thresh_16to8_mod = pst16BitTo8BitCtrl->enMode;
	ive_top_c->REG_h134.reg_thresh_16to8_u8Num_div_u16Den =
		u8Num_div_u16Den; //0xffff
	ive_top_c->REG_h134.reg_thresh_16to8_s8bias = pst16BitTo8BitCtrl->s8Bias;
	writel(ive_top_c->REG_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c->REG_h134.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H134));
	_cvi_ive_16bit_to_8bit(pstSrc, pstDst, ive_top_c, &ive_filterop_c,
		 &wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, NULL,
				&wdma_y_ctl_c, NULL, NULL, &rdma_eigval_ctl_c, pstSrc,
				NULL, NULL, pstDst, NULL, true, 1, false, 1, false, MOD_16To8,
				bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_16To8, dev_id);
	}

	ive_top_c->REG_3.reg_mapmux_rdma_sel = 0;
	ive_top_c->REG_3.reg_ive_rdma_eigval_en = 0;
	ive_top_c->REG_h10.reg_rdma_eigval_top_enable = 0;
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_thresh_s16(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_THRESH_S16_CTRL_S *pstThrS16Ctrl,
			   bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_eigval_ctl_c);
	ISP_DMA_CTL_C rdma_eigval_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Thresh_S16\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	// top
	ive_top_c.REG_h130.reg_thresh_top_mod = 2;
	ive_top_c.REG_h130.reg_thresh_thresh_en = 0;
	ive_top_c.REG_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c.REG_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c.REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ive_set_wh(&ive_top_c, pstDst->u32Width, pstDst->u32Height, "Thresh_S16", dev_id);
	// setting
	ive_top_c.REG_h13c.reg_thresh_s16_enmode = pstThrS16Ctrl->enMode;
	ive_top_c.REG_h13c.reg_thresh_s16_u8bit_min =
		pstThrS16Ctrl->un8MinVal.u8Val;
	ive_top_c.REG_h13c.reg_thresh_s16_u8bit_mid =
		pstThrS16Ctrl->un8MidVal.u8Val;
	ive_top_c.REG_h13c.reg_thresh_s16_u8bit_max =
		pstThrS16Ctrl->un8MaxVal.u8Val;
	ive_top_c.REG_h140.reg_thresh_s16_bit_thr_l = pstThrS16Ctrl->s16LowThr;
	ive_top_c.REG_h140.reg_thresh_s16_bit_thr_h = pstThrS16Ctrl->s16HighThr;
	writel(ive_top_c.REG_h13c.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H13C));
	writel(ive_top_c.REG_h140.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H140));

	_cvi_ive_16bit_to_8bit(pstSrc, pstDst, &ive_top_c, &ive_filterop_c,
		 &wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, &ive_top_c, &ive_filterop_c, NULL,
				&wdma_y_ctl_c, NULL, NULL, &rdma_eigval_ctl_c, pstSrc,
				NULL, NULL, pstDst, NULL, true, 1, false, 1, false, MOD_THRS16,
				bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, &ive_top_c, bInstant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_THRS16, dev_id);
	}

	return ret;
}

s32 cvi_ive_thresh_u16(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_THRESH_U16_CTRL_S *pstThrU16Ctrl,
			   bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_eigval_ctl_c);
	ISP_DMA_CTL_C rdma_eigval_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Thresh_U16\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	// top
	ive_top_c.REG_h130.reg_thresh_top_mod = 3;
	ive_top_c.REG_h130.reg_thresh_thresh_en = 0;
	ive_top_c.REG_h10.reg_thresh_top_enable = 1;
	writel(ive_top_c.REG_h130.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c.REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	ive_set_wh(&ive_top_c, pstDst->u32Width, pstDst->u32Height, "Thresh_U16", dev_id);

	// setting
	ive_top_c.REG_h144.reg_thresh_u16_enmode = pstThrU16Ctrl->enMode;
	ive_top_c.REG_h144.reg_thresh_u16_u8bit_min = pstThrU16Ctrl->u8MinVal;
	ive_top_c.REG_h144.reg_thresh_u16_u8bit_mid = pstThrU16Ctrl->u8MidVal;
	ive_top_c.REG_h144.reg_thresh_u16_u8bit_max = pstThrU16Ctrl->u8MaxVal;
	ive_top_c.REG_h148.reg_thresh_u16_bit_thr_l = pstThrU16Ctrl->u16LowThr;
	ive_top_c.REG_h148.reg_thresh_u16_bit_thr_h = pstThrU16Ctrl->u16HighThr;
	writel(ive_top_c.REG_h144.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H144));
	writel(ive_top_c.REG_h148.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H148));

	_cvi_ive_16bit_to_8bit(pstSrc, pstDst, &ive_top_c, &ive_filterop_c,
		 &wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, &ive_top_c, &ive_filterop_c, NULL,
				&wdma_y_ctl_c, NULL, NULL, &rdma_eigval_ctl_c, pstSrc,
				NULL, NULL, pstDst, NULL, true, 1, false, 1, false, MOD_THRU16,
				bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, &ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_THRU16, dev_id);
	}

	return ret;
}

s32 cvi_ive_ord_stat_filter(struct cvi_ive_device *ndev,
				  IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
				  IVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl,
				  bool bInstant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_OrdStatFilter\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "OrdStatFilter", dev_id);

	// TODO: check set to 0
	ive_filterop_c.REG_h10.reg_filterop_mode = 2;
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 4; //sw_ovw; bypass op1
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.REG_18.reg_filterop_order_enmode =
		pstOrdStatFltCtrl->enMode;

	//bypass op2
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.REG_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));

	set_isp_rdma(MOD_ORDSTAFTR, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	// trigger filterop
	//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);

	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				&wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL,
				NULL, pstDst, NULL, true, 1, false, 1, false, MOD_ORDSTAFTR,
				bInstant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_ORDSTAFTR, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_canny_hys_edge(struct cvi_ive_device *ndev,
				 IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstEdge,
				 IVE_DST_MEM_INFO_S *pstStack,
				 IVE_CANNY_HYS_EDGE_CTRL_S *pstCannyHysEdgeCtrl,
				 bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	IVE_DST_IMAGE_S _pstStack;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_c_ctl_c);
	ISP_DMA_CTL_C wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_CannyHysEdge\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstEdge", pstEdge);
		dump_ive_mem("pstStack", pstStack);
	}
	// top
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "CannyHysEdge", dev_id);

	ive_filterop_c.REG_h10.reg_filterop_mode = MOD_CANNY;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	ive_filterop_c.REG_CANNY_0.reg_canny_lowthr =
		pstCannyHysEdgeCtrl->u16LowThr;
	ive_filterop_c.REG_CANNY_0.reg_canny_hithr =
		pstCannyHysEdgeCtrl->u16HighThr;
	ive_filterop_c.REG_CANNY_1.reg_canny_en = 1;
	writel(ive_filterop_c.REG_CANNY_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_0));
	writel(ive_filterop_c.REG_CANNY_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_1));

	ive_filterop_c.REG_CANNY_2.reg_canny_eof = 0xfffe7ffd;
	ive_filterop_c.REG_4.reg_filterop_h_coef00 =
		pstCannyHysEdgeCtrl->as8Mask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 =
		pstCannyHysEdgeCtrl->as8Mask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 =
		pstCannyHysEdgeCtrl->as8Mask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 =
		pstCannyHysEdgeCtrl->as8Mask[3];
	ive_filterop_c.REG_5.reg_filterop_h_coef04 =
		pstCannyHysEdgeCtrl->as8Mask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 =
		pstCannyHysEdgeCtrl->as8Mask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 =
		pstCannyHysEdgeCtrl->as8Mask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 =
		pstCannyHysEdgeCtrl->as8Mask[7];
	ive_filterop_c.REG_6.reg_filterop_h_coef13 =
		pstCannyHysEdgeCtrl->as8Mask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 =
		pstCannyHysEdgeCtrl->as8Mask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 =
		pstCannyHysEdgeCtrl->as8Mask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 =
		pstCannyHysEdgeCtrl->as8Mask[11];
	ive_filterop_c.REG_7.reg_filterop_h_coef22 =
		pstCannyHysEdgeCtrl->as8Mask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 =
		pstCannyHysEdgeCtrl->as8Mask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 =
		pstCannyHysEdgeCtrl->as8Mask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 =
		pstCannyHysEdgeCtrl->as8Mask[15];
	ive_filterop_c.REG_8.reg_filterop_h_coef31 =
		pstCannyHysEdgeCtrl->as8Mask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 =
		pstCannyHysEdgeCtrl->as8Mask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 =
		pstCannyHysEdgeCtrl->as8Mask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 =
		pstCannyHysEdgeCtrl->as8Mask[19];
	ive_filterop_c.REG_9.reg_filterop_h_coef40 =
		pstCannyHysEdgeCtrl->as8Mask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 =
		pstCannyHysEdgeCtrl->as8Mask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 =
		pstCannyHysEdgeCtrl->as8Mask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 =
		pstCannyHysEdgeCtrl->as8Mask[23];
	ive_filterop_c.REG_10.reg_filterop_h_coef44 =
		pstCannyHysEdgeCtrl->as8Mask[24];
	writel(ive_filterop_c.REG_CANNY_2.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_2));
	writel(ive_filterop_c.REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	_pstStack.u64PhyAddr[0] = pstStack->u64PhyAddr;
	// NOTICE: we leverage stride as size
	_pstStack.u32Stride[0] = pstStack->u32Size;

	set_isp_rdma(MOD_CANNY, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstEdge, &wdma_y_ctl_c, dev_id);
	set_img_dst2(&_pstStack, &wdma_c_ctl_c, dev_id);
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				   &wdma_y_ctl_c, NULL, &wdma_c_ctl_c, NULL, pstSrc,
				   NULL, NULL, pstEdge, &_pstStack, true, 1, true,
				   4, false, MOD_CANNY, bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_CANNY, dev_id);
	}
	ive_filterop_c.REG_CANNY_1.reg_canny_en = 0;
	writel(ive_filterop_c.REG_CANNY_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_0));
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_norm_grad(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			 IVE_DST_IMAGE_S *pstDstHV,
			 IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl,
			 bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	bool enWdma_y = false, enWdma_c = false;
	s32 yunit = 1;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_c_ctl_c);
	ISP_DMA_CTL_C wdma_c_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_NormGrad\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDstH", pstDstH);
		dump_ive_image("pstDstV", pstDstV);
		dump_ive_image("pstDstHV", pstDstHV);
	}
	// top
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "NormGrad", dev_id);

	ive_filterop_c.REG_h10.reg_filterop_mode = 8;
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c.REG_28.reg_filterop_op2_erodila_en = 0;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c.REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));

	ive_filterop_c.REG_4.reg_filterop_h_coef00 =
		pstNormGradCtrl->as8Mask[0];
	ive_filterop_c.REG_4.reg_filterop_h_coef01 =
		pstNormGradCtrl->as8Mask[1];
	ive_filterop_c.REG_4.reg_filterop_h_coef02 =
		pstNormGradCtrl->as8Mask[2];
	ive_filterop_c.REG_4.reg_filterop_h_coef03 =
		pstNormGradCtrl->as8Mask[3];
	ive_filterop_c.REG_5.reg_filterop_h_coef04 =
		pstNormGradCtrl->as8Mask[4];
	ive_filterop_c.REG_5.reg_filterop_h_coef10 =
		pstNormGradCtrl->as8Mask[5];
	ive_filterop_c.REG_5.reg_filterop_h_coef11 =
		pstNormGradCtrl->as8Mask[6];
	ive_filterop_c.REG_5.reg_filterop_h_coef12 =
		pstNormGradCtrl->as8Mask[7];
	ive_filterop_c.REG_6.reg_filterop_h_coef13 =
		pstNormGradCtrl->as8Mask[8];
	ive_filterop_c.REG_6.reg_filterop_h_coef14 =
		pstNormGradCtrl->as8Mask[9];
	ive_filterop_c.REG_6.reg_filterop_h_coef20 =
		pstNormGradCtrl->as8Mask[10];
	ive_filterop_c.REG_6.reg_filterop_h_coef21 =
		pstNormGradCtrl->as8Mask[11];
	ive_filterop_c.REG_7.reg_filterop_h_coef22 =
		pstNormGradCtrl->as8Mask[12];
	ive_filterop_c.REG_7.reg_filterop_h_coef23 =
		pstNormGradCtrl->as8Mask[13];
	ive_filterop_c.REG_7.reg_filterop_h_coef24 =
		pstNormGradCtrl->as8Mask[14];
	ive_filterop_c.REG_7.reg_filterop_h_coef30 =
		pstNormGradCtrl->as8Mask[15];
	ive_filterop_c.REG_8.reg_filterop_h_coef31 =
		pstNormGradCtrl->as8Mask[16];
	ive_filterop_c.REG_8.reg_filterop_h_coef32 =
		pstNormGradCtrl->as8Mask[17];
	ive_filterop_c.REG_8.reg_filterop_h_coef33 =
		pstNormGradCtrl->as8Mask[18];
	ive_filterop_c.REG_8.reg_filterop_h_coef34 =
		pstNormGradCtrl->as8Mask[19];
	ive_filterop_c.REG_9.reg_filterop_h_coef40 =
		pstNormGradCtrl->as8Mask[20];
	ive_filterop_c.REG_9.reg_filterop_h_coef41 =
		pstNormGradCtrl->as8Mask[21];
	ive_filterop_c.REG_9.reg_filterop_h_coef42 =
		pstNormGradCtrl->as8Mask[22];
	ive_filterop_c.REG_9.reg_filterop_h_coef43 =
		pstNormGradCtrl->as8Mask[23];
	ive_filterop_c.REG_10.reg_filterop_h_coef44 =
		pstNormGradCtrl->as8Mask[24];
	ive_filterop_c.REG_10.reg_filterop_h_norm = pstNormGradCtrl->u8Norm;
	writel(ive_filterop_c.REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c.REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c.REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c.REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c.REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c.REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c.REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	set_isp_rdma(MOD_NORMG, NULL, dev_id);
	if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER) {
		set_img_dst1(pstDstH, &wdma_y_ctl_c, dev_id);
		set_img_dst2(pstDstV, &wdma_c_ctl_c, dev_id);
		ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl = 0;
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1; // h
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1; // v
		enWdma_y = enWdma_c = true;
	} else if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_HOR) {
		set_img_dst1(pstDstH, &wdma_y_ctl_c, dev_id);
		set_img_dst2(NULL, NULL, dev_id);
		ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl = 1;
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1; // h
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0; // v
		enWdma_y = true;
	} else if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_VER) {
		set_img_dst1(NULL, NULL, dev_id);
		set_img_dst2(pstDstV, &wdma_c_ctl_c, dev_id);
		ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl = 2;
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0; // h
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 1; // v
		enWdma_c = true;
	} else if (pstNormGradCtrl->enOutCtrl ==
		   IVE_NORM_GRAD_OUT_CTRL_COMBINE) {
		set_img_dst1(pstDstHV, &wdma_y_ctl_c, dev_id);
		set_img_dst2(NULL, NULL, dev_id);
		ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl = 3;
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1; // h
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0; // v
		enWdma_y = true;
		yunit = 2;
	} else {
		pr_err("Invalid enOutCtrl %d\n", pstNormGradCtrl->enOutCtrl);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	writel(ive_filterop_c.REG_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));

	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		if (pstNormGradCtrl->enOutCtrl ==
			IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL,
					&wdma_c_ctl_c, NULL, pstSrc, NULL, NULL,
					pstDstH, pstDstV, enWdma_y, yunit,
					enWdma_c, 1, false, MOD_NORMG, bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		} else if (pstNormGradCtrl->enOutCtrl ==
			   IVE_NORM_GRAD_OUT_CTRL_HOR) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, pstDstH, NULL,
					enWdma_y, yunit, enWdma_c, 1, false, MOD_NORMG,
					bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		} else if (pstNormGradCtrl->enOutCtrl ==
			   IVE_NORM_GRAD_OUT_CTRL_VER) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, NULL, NULL, &wdma_c_ctl_c,
					NULL, pstSrc, NULL, NULL, NULL, pstDstV,
					enWdma_y, yunit, enWdma_c, 1, false, MOD_NORMG,
					bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		} else if (pstNormGradCtrl->enOutCtrl ==
			   IVE_NORM_GRAD_OUT_CTRL_COMBINE) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, pstDstHV,
					NULL, enWdma_y, yunit, enWdma_c, 1,
					false, MOD_NORMG, bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}

	if (ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl == 0) {
		ret = cvi_ive_go(
			ndev, ive_top_c, bInstant,
			IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK |
				IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
			MOD_NORMG, dev_id);
	} else if (ive_filterop_c.REG_110.reg_filterop_norm_out_ctrl == 2) {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_C_MASK,
				 MOD_NORMG, dev_id);
	} else { //1 3
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_NORMG, dev_id);
	}
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_grad_fg(struct cvi_ive_device *ndev,
			   IVE_SRC_IMAGE_S *pstBgDiffFg,
			   IVE_SRC_IMAGE_S *pstCurGrad, IVE_SRC_IMAGE_S *pstBgGrad,
			   IVE_DST_IMAGE_S *pstGradFg,
			   IVE_GRAD_FG_CTRL_S *pstGradFgCtrl, bool bInstant,
			   s32 dev_id)
{
	s32 mode = 0;
	s32 ret = CVI_SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_gradfg_ctl_c);
	ISP_DMA_CTL_C rdma_gradfg_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_img1_ctl_c);
	ISP_DMA_CTL_C rdma_img1_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	IVE_GMM_C ive_gmm_c = _DEFINE_IVE_GMM_C;
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_GradFg\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstBgDiffFg", pstBgDiffFg);
		dump_ive_image("pstCurGrad", pstCurGrad);
		dump_ive_image("pstBgGrad", pstBgGrad);
		dump_ive_image("pstGradFg", pstGradFg);
	}
	//test1:rgb:set gmm_gmm2_yonly:0
	//y_only:set gmm_gmm2_yonly:1
	ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly = 0;
	writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));

	// top
	ive_set_wh(ive_top_c, pstBgDiffFg->u32Width, pstBgDiffFg->u32Height, "GradFg", dev_id);

	ive_filterop_c.REG_h10.reg_filterop_mode = 6;
	ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
	writel(ive_filterop_c.REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	// TODO: need softrst?
	//iveReg->ive_filterop_c->REG_33.reg_filterop_op2_gradfg_softrst = 0;
	ive_filterop_c.REG_33.reg_filterop_op2_gradfg_en = 1;
	ive_filterop_c.REG_33.reg_filterop_op2_gradfg_enmode =
		pstGradFgCtrl->enMode;
	ive_filterop_c.REG_33.reg_filterop_op2_gradfg_edwdark =
		pstGradFgCtrl->u8EdwDark;
	ive_filterop_c.REG_33.reg_filterop_op2_gradfg_edwfactor =
		pstGradFgCtrl->u16EdwFactor;
	ive_filterop_c.REG_34.reg_filterop_op2_gradfg_crlcoefthr =
		pstGradFgCtrl->u8CrlCoefThr;
	ive_filterop_c.REG_34.reg_filterop_op2_gradfg_magcrlthr =
		pstGradFgCtrl->u8MagCrlThr;
	ive_filterop_c.REG_34.reg_filterop_op2_gradfg_minmagdiff =
		pstGradFgCtrl->u8MinMagDiff;
	ive_filterop_c.REG_34.reg_filterop_op2_gradfg_noiseval =
		pstGradFgCtrl->u8NoiseVal;
	writel(ive_filterop_c.REG_33.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_33));
	writel(ive_filterop_c.REG_34.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_34));
	//iveReg->setDMA(pstBgDiffFg, pstGradFg, pstCurGrad, NULL, pstBgGrad);
	rdma_gradfg_ctl_c.BASE_ADDR.reg_basel =
		pstBgGrad->u64PhyAddr[0] & 0xffffffff;
	rdma_gradfg_ctl_c.SYS_CONTROL.reg_baseh =
		(pstBgGrad->u64PhyAddr[0] >> 32) & 0xffffffff;
	rdma_gradfg_ctl_c.DMA_STRIDE.reg_stride = pstBgGrad->u32Stride[0];
	rdma_gradfg_ctl_c.SYS_CONTROL.reg_stride_sel = 1;
	rdma_gradfg_ctl_c.SYS_CONTROL.reg_base_sel = 1;
	writel(rdma_gradfg_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(rdma_gradfg_ctl_c.DMA_STRIDE.val,
			(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_STRIDE));
	writel(rdma_gradfg_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_SYS_CONTROL));

	set_isp_rdma(MOD_GRADFG, NULL, dev_id);
	if (set_img_src1(pstBgDiffFg, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	set_img_src2(pstCurGrad, &rdma_img1_ctl_c, dev_id);
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Src3 address: 0x%08x %08x\n",
			rdma_gradfg_ctl_c.SYS_CONTROL.reg_baseh,
			rdma_gradfg_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[RDMA_RADFG].addr_en = rdma_gradfg_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[RDMA_RADFG].addr_l = rdma_gradfg_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[RDMA_RADFG].addr_h = rdma_gradfg_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	mode = ive_get_mod_u8(pstBgDiffFg->enType);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	ive_top_c->REG_3.reg_ive_rdma_img1_mod_u8 = mode;

	// TODO: need to set vld?
	ive_top_c->REG_3.reg_imgmux_img0_sel = 0;
	ive_top_c->REG_3.reg_ive_rdma_img1_en = 1;
	ive_top_c->REG_h10.reg_rdma_img1_top_enable = 1;
	ive_filterop_c.REG_H04.reg_gradfg_bggrad_rdma_en = 1;
	ive_top_c->REG_3.reg_ive_rdma_img1_mod_u8 = 0;
	ive_top_c->REG_3.reg_muxsel_gradfg = 1;
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstGradFg, &wdma_y_ctl_c, dev_id);
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_H04.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	if (pstBgDiffFg->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				   &wdma_y_ctl_c, &rdma_img1_ctl_c, NULL, NULL,
				   pstBgDiffFg, pstCurGrad, pstBgGrad, pstGradFg,
				   NULL, true, 1, false, 1, false, MOD_GRADFG,
				   bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_GRADFG, dev_id);
	}

	if (bInstant) {
		ive_filterop_c.REG_33.reg_filterop_op2_gradfg_en = 0;
		ive_filterop_c.REG_H04.reg_gradfg_bggrad_rdma_en = 0;
		ive_top_c->REG_3.reg_muxsel_gradfg = 0;
		writel(ive_filterop_c.REG_33.val,
			(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_33));
		writel(ive_filterop_c.REG_H04.val,
			(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
		writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

		rdma_gradfg_ctl_c.BASE_ADDR.reg_basel = 0;
		rdma_gradfg_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		rdma_gradfg_ctl_c.SYS_CONTROL.reg_stride_sel = 0;
		rdma_gradfg_ctl_c.SYS_CONTROL.reg_seglen_sel = 0;
		rdma_gradfg_ctl_c.SYS_CONTROL.reg_segnum_sel = 0;
		rdma_gradfg_ctl_c.DMA_SEGNUM.reg_segnum = 0;
		rdma_gradfg_ctl_c.DMA_SEGLEN.reg_seglen = 0;
		rdma_gradfg_ctl_c.DMA_STRIDE.reg_stride = 0;
		ive_filterop_c.REG_33.reg_filterop_op2_gradfg_en = 0;
		ive_filterop_c.REG_H04.reg_gradfg_bggrad_rdma_en = 0;
		ive_top_c->REG_3.reg_muxsel_gradfg = 0;

		writel(rdma_gradfg_ctl_c.BASE_ADDR.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(rdma_gradfg_ctl_c.SYS_CONTROL.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_SYS_CONTROL));
		writel(rdma_gradfg_ctl_c.DMA_SEGNUM.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGNUM));
		writel(rdma_gradfg_ctl_c.DMA_SEGLEN.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_SEGLEN));
		writel(rdma_gradfg_ctl_c.DMA_STRIDE.val,
				(IVE_BLK_BA[dev_id].FILTEROP_RDMA + ISP_DMA_CTL_DMA_STRIDE));
		writel(ive_filterop_c.REG_33.val,
				(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_33));
		writel(ive_filterop_c.REG_H04.val,
				(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H04));
		writel(ive_top_c->REG_3.val,
				(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	}
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_sad(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstSad,
			IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstSadCtrl,
			bool bInstant, s32 dev_id)
{
	s32 mode = 0;
	s32 ret = CVI_SUCCESS;
	IVE_DST_IMAGE_S *pstSadOut = NULL;
	IVE_DST_IMAGE_S *pstThrOut = NULL;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C ive_top_c = _DEFINE_IVE_TOP_C;
	//DEFINE_IVE_SAD_C(ive_sad_c);
	IVE_SAD_C ive_sad_c = _DEFINE_IVE_SAD_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_sad_ctl_c);
	ISP_DMA_CTL_C wdma_sad_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_sad_thr_ctl_c);
	ISP_DMA_CTL_C wdma_sad_thr_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_SAD\n");
	ive_reset(ndev, &ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc1", pstSrc1);
		dump_ive_image("pstSrc2", pstSrc2);
		dump_ive_image("pstSad", pstSad);
		dump_ive_image("pstThr", pstThr);
	}
	// check input
	if (pstSrc1->enType != IVE_IMAGE_TYPE_U8C1 ||
		pstSrc2->enType != IVE_IMAGE_TYPE_U8C1) {
		pr_err("pstSrc1->enType %d and pstSrc2->enType %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
			   pstSrc1->enType, pstSrc2->enType, IVE_IMAGE_TYPE_U8C1);
		return CVI_FAILURE;
	}

	// check output
	switch (pstSadCtrl->enOutCtrl) {
	case IVE_SAD_OUT_CTRL_16BIT_BOTH:
		if (pstSad->enType != IVE_IMAGE_TYPE_U16C1 ||
			pstThr->enType != IVE_IMAGE_TYPE_U8C1) {
			pr_err("pstSad->enType %d (should be IVE_IMAGE_TYPE_U16C1(%d)",
				   pstSad->enType, IVE_IMAGE_TYPE_U16C1);
			pr_err(" pstThr->enType %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
				   pstThr->enType, IVE_IMAGE_TYPE_U8C1);
			return CVI_FAILURE;
		}
		pstSadOut = pstSad;
		pstThrOut = pstThr;
		break;
	case IVE_SAD_OUT_CTRL_16BIT_SAD: // dont care thr
		if (pstSad->enType != IVE_IMAGE_TYPE_U16C1) {
			pr_err("pstSad->enType %d (should be IVE_IMAGE_TYPE_U16C1(%d)",
				   pstSad->enType, IVE_IMAGE_TYPE_U16C1);
			return CVI_FAILURE;
		}
		pstSadOut = pstSad;
		break;
	case IVE_SAD_OUT_CTRL_THRESH: // only output thresh
		if (pstThr->enType != IVE_IMAGE_TYPE_U8C1) {
			pr_err(" pstThr->enType %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
				   pstThr->enType, IVE_IMAGE_TYPE_U8C1);
			return CVI_FAILURE;
		}
		pstThrOut = pstThr;
		break;
	case IVE_SAD_OUT_CTRL_8BIT_BOTH:
		if (pstSad->enType != IVE_IMAGE_TYPE_U8C1 ||
			pstThr->enType != IVE_IMAGE_TYPE_U8C1) {
			pr_err("pstSad->enType %d (should be IVE_IMAGE_TYPE_U8C1(%d)",
				   pstSad->enType, IVE_IMAGE_TYPE_U8C1);
			pr_err(" pstThr->enType %d should be %d(IVE_IMAGE_TYPE_U8C1)\n",
				   pstThr->enType, IVE_IMAGE_TYPE_U8C1);
			return CVI_FAILURE;
		}
		pstSadOut = pstSad;
		pstThrOut = pstThr;
		break;
	case IVE_SAD_OUT_CTRL_8BIT_SAD:
		if (pstSad->enType != IVE_IMAGE_TYPE_U8C1) {
			pr_err("pstSad->enType %d (should be IVE_IMAGE_TYPE_U8C1(%d)",
				   pstSad->enType, IVE_IMAGE_TYPE_U8C1);
			return CVI_FAILURE;
		}
		pstSadOut = pstSad;
		break;
	default:
		pr_err("not support output type %d, return\n",
			   pstSadCtrl->enOutCtrl);
		return CVI_FAILURE;
	}
	// top
	//ive_top_c.REG_h10.reg_sad_top_enable = 1; // remove ?
	ive_sad_c.REG_SAD_02.reg_sad_enable = 1;
	writel(ive_sad_c.REG_SAD_02.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_02));

	// align isp
	ive_set_wh(&ive_top_c, pstSrc1->u32Width, pstSrc1->u32Height, "SAD", dev_id);

	// setting
	ive_sad_c.REG_SAD_00.reg_sad_enmode = pstSadCtrl->enMode;
	ive_sad_c.REG_SAD_00.reg_sad_out_ctrl = pstSadCtrl->enOutCtrl;
	ive_sad_c.REG_SAD_00.reg_sad_u16bit_thr = pstSadCtrl->u16Thr;
	ive_sad_c.REG_SAD_01.reg_sad_u8bit_max = pstSadCtrl->u8MaxVal;
	ive_sad_c.REG_SAD_01.reg_sad_u8bit_min = pstSadCtrl->u8MinVal;

	writel(ive_sad_c.REG_SAD_00.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_00));
	writel(ive_sad_c.REG_SAD_01.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_01));

	set_isp_rdma(MOD_SAD, NULL, dev_id);
	if (set_img_src1(pstSrc1, &img_in_c, &ive_top_c, dev_id) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	set_img_src2(pstSrc2, NULL, dev_id);
	//set_img_dst1(pstSadOut, NULL);

	mode = ive_get_mod_u8(pstSrc1->enType);
	if (mode == -1) {
		pr_err("[IVE] not support src type");
		return CVI_FAILURE;
	}
	ive_top_c.REG_3.reg_ive_rdma_img1_mod_u8 = mode;

	// TODO: need to set vld?
	ive_top_c.REG_3.reg_imgmux_img0_sel = 0;
	ive_top_c.REG_3.reg_ive_rdma_img1_en = 1;

	// trigger
	ive_top_c.REG_20.reg_frame2op_op_mode = 6;
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c.REG_h10.reg_filterop_top_enable = 0;
	ive_top_c.REG_14.reg_csc_enable = 0;
	ive_top_c.REG_h10.reg_csc_top_enable = 0;
	ive_top_c.REG_h10.reg_rdma_img1_top_enable = 1;
	ive_top_c.REG_h10.reg_sad_top_enable = 1;
	writel(ive_top_c.REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c.REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c.REG_14.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_14));
	writel(ive_top_c.REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	writel(ive_top_c.REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	if (pstSadOut->u64PhyAddr[0]) {
		wdma_sad_ctl_c.BASE_ADDR.reg_basel =
			pstSadOut->u64PhyAddr[0] & 0xffffffff;
		wdma_sad_ctl_c.SYS_CONTROL.reg_baseh =
			(pstSadOut->u64PhyAddr[0] >> 32) & 0xffffffff;
		wdma_sad_ctl_c.SYS_CONTROL.reg_base_sel = 1;

		wdma_sad_ctl_c.SYS_CONTROL.reg_stride_sel = 1; //1;
		wdma_sad_ctl_c.SYS_CONTROL.reg_seglen_sel = 0;
		wdma_sad_ctl_c.SYS_CONTROL.reg_segnum_sel = 0;

		// set height
		wdma_sad_ctl_c.DMA_SEGNUM.reg_segnum = 0;
		// set width
		wdma_sad_ctl_c.DMA_SEGLEN.reg_seglen = 0;
		// set stride
		wdma_sad_ctl_c.DMA_STRIDE.reg_stride = pstSadOut->u32Stride[0];
	} else {
		wdma_sad_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		wdma_sad_ctl_c.BASE_ADDR.reg_basel = 0;
		wdma_sad_ctl_c.SYS_CONTROL.reg_baseh = 0;
	}

	if (pstThrOut->u64PhyAddr[0]) {
		wdma_sad_thr_ctl_c.BASE_ADDR.reg_basel =
			pstThrOut->u64PhyAddr[0] & 0xffffffff;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_baseh =
			(pstThrOut->u64PhyAddr[0] >> 32) & 0xffffffff;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_base_sel = 1;

		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_stride_sel = 1; //1;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_seglen_sel = 0;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_segnum_sel = 0; //1;

		// set height
		wdma_sad_thr_ctl_c.DMA_SEGNUM.reg_segnum = 0;
		// set width
		wdma_sad_thr_ctl_c.DMA_SEGLEN.reg_seglen = 0;
		// set stride
		wdma_sad_thr_ctl_c.DMA_STRIDE.reg_stride = pstThrOut->u32Stride[0];
	} else {
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_base_sel = 0;
		wdma_sad_thr_ctl_c.BASE_ADDR.reg_basel = 0;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_baseh = 0;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_stride_sel = 0;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_seglen_sel = 0;
		wdma_sad_thr_ctl_c.SYS_CONTROL.reg_segnum_sel = 0;
		wdma_sad_thr_ctl_c.DMA_SEGNUM.reg_segnum = 0;
		wdma_sad_thr_ctl_c.DMA_SEGLEN.reg_seglen = 0;
		wdma_sad_thr_ctl_c.DMA_STRIDE.reg_stride = 0;
	}
	if (g_dump_dma_info == CVI_TRUE) {
		pr_info("Dst Sad address: 0x%08x %08x\n",
			wdma_sad_ctl_c.SYS_CONTROL.reg_baseh,
			wdma_sad_ctl_c.BASE_ADDR.reg_basel);
		pr_info("Dst Sad thr address: 0x%08x %08x\n",
			wdma_sad_thr_ctl_c.SYS_CONTROL.reg_baseh,
			wdma_sad_thr_ctl_c.BASE_ADDR.reg_basel);
	}
	g_debug_info.addr[WDMA_SAD].addr_en = wdma_sad_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_SAD].addr_l = wdma_sad_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_SAD].addr_h = wdma_sad_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	g_debug_info.addr[WDMA_SAD_THR].addr_en = wdma_sad_thr_ctl_c.SYS_CONTROL.reg_base_sel;
	g_debug_info.addr[WDMA_SAD_THR].addr_l = wdma_sad_thr_ctl_c.BASE_ADDR.reg_basel;
	g_debug_info.addr[WDMA_SAD_THR].addr_h = wdma_sad_thr_ctl_c.SYS_CONTROL.reg_baseh & 0xff;

	writel(wdma_sad_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_sad_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	writel(wdma_sad_ctl_c.DMA_SEGNUM.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_DMA_SEGNUM));
	writel(wdma_sad_ctl_c.DMA_SEGLEN.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_DMA_SEGLEN));
	writel(wdma_sad_ctl_c.DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_DMA_STRIDE));

	writel(wdma_sad_thr_ctl_c.BASE_ADDR.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_BASE_ADDR));
	writel(wdma_sad_thr_ctl_c.SYS_CONTROL.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_SYS_CONTROL));
	writel(wdma_sad_thr_ctl_c.DMA_SEGNUM.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_DMA_SEGNUM));
	writel(wdma_sad_thr_ctl_c.DMA_SEGLEN.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_DMA_SEGLEN));
	writel(wdma_sad_thr_ctl_c.DMA_STRIDE.val,
		   (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_DMA_STRIDE));
	ndev->cur_optype = MOD_SAD;
	ret = cvi_ive_go(ndev, &ive_top_c, bInstant,
			 IVE_TOP_REG_FRAME_DONE_SAD_MASK, MOD_SAD, dev_id);

	ive_sad_c.REG_SAD_02.reg_sad_enable = 0;
	writel(ive_sad_c.REG_SAD_02.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_02));

	ive_sad_c.REG_SAD_02.reg_sad_enable = 0;
	writel(ive_sad_c.REG_SAD_02.val, (IVE_BLK_BA[dev_id].SAD + IVE_SAD_REG_SAD_02));
	// reset flow
	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_BASE_ADDR));
	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA + ISP_DMA_CTL_SYS_CONTROL));

	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_BASE_ADDR));
	writel(0x0000, (IVE_BLK_BA[dev_id].SAD_WDMA_THR + ISP_DMA_CTL_SYS_CONTROL));

	ive_top_c.REG_3.reg_ive_rdma_img1_en = 0;
	writel(ive_top_c.REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	ive_top_c.REG_20.reg_frame2op_op_mode = 0;
	ive_top_c.REG_h80.reg_frame2op_fg_op_mode = 0;
	writel(ive_top_c.REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c.REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));

	writel(img_in_c.REG_00.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));
	writel(img_in_c.REG_02.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_02));
	writel(img_in_c.REG_05.val, (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_05));

	return ret;
}

void _sclr_get_2tap_scale(IVE_TOP_C *ive_top_c, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDst, s32 dev_id)
{
	u32 src_wd, src_ht, dst_wd, dst_ht, scale_x, scale_y;
	bool fast_area_x = 0, fast_area_y = 0;
	u32 h_nor = 0, v_nor = 0, h_ph = 0, v_ph = 0;
	bool area_fast = false;
	bool scale_down = false;

	if (!pstSrc->u32Width || !pstSrc->u32Height || !pstDst->u32Width ||
		!pstDst->u32Height)
		return;

	src_wd = pstSrc->u32Width;
	src_ht = pstSrc->u32Height;
	dst_wd = pstDst->u32Width;
	dst_ht = pstDst->u32Height;

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

	ive_top_c->REG_RS_CTRL.reg_resize_area_fast = 1;
	ive_top_c->REG_RS_CTRL.reg_resize_blnr_mode = 0;

	ive_top_c->REG_RS_H_SC.reg_resize_h_sc_fac = scale_x;
	ive_top_c->REG_RS_V_SC.reg_resize_v_sc_fac = scale_y;
	ive_top_c->REG_RS_PH_INI.reg_resize_h_ini_ph = h_ph;
	ive_top_c->REG_RS_PH_INI.reg_resize_v_ini_ph = v_ph;
	ive_top_c->REG_RS_NOR.reg_resize_h_nor = h_nor;
	ive_top_c->REG_RS_NOR.reg_resize_v_nor = v_nor;
	writel(ive_top_c->REG_RS_CTRL.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_CTRL));
	writel(ive_top_c->REG_RS_H_SC.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_H_SC));
	writel(ive_top_c->REG_RS_V_SC.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_V_SC));
	writel(ive_top_c->REG_RS_PH_INI.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_PH_INI));
	writel(ive_top_c->REG_RS_NOR.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_NOR));
}

s32 cvi_ive_resize(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_RESIZE_CTRL_S *pstResizeCtrl, bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	//DEFINE_IVE_GMM_C(ive_gmm_c);
	IVE_GMM_C ive_gmm_c = _DEFINE_IVE_GMM_C;
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_Resize\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
			char tmp[32];
			dump_ive_image(tmp, pstSrc);
			dump_ive_image(tmp, pstDst);
	}
	if (pstResizeCtrl->enMode != IVE_RESIZE_MODE_AREA &&
		pstResizeCtrl->enMode != IVE_RESIZE_MODE_LINEAR) {
		pr_err("Invalid Resize enMode %d\n", pstResizeCtrl->enMode);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	// for (i = 0; i < pstResizeCtrl->u16Num; i++) {
		if (pstDst->enType != IVE_IMAGE_TYPE_U8C3_PLANAR &&
			pstDst->enType != IVE_IMAGE_TYPE_U8C1) {
			pr_err("Invalid IMAGE TYPE pstDst %d\n",
				   pstDst->enType);
			pr_err("Invalid IMAGE TYPE pstSrc %d\n",
				   pstSrc->enType);
			pr_err("Invalid IMAGE Width pstDst %d %d\n",
				   pstDst->u32Width, pstDst->u32Height);
			pr_err("Invalid IMAGE Width pstSrc %d %d\n",
				   pstSrc->u32Width, pstSrc->u32Height);
			kfree(ive_top_c);
			return CVI_FAILURE;
		}
		//test1:rgb:set gmm_gmm2_yonly:0
		//y_only:set gmm_gmm2_yonly:1
		if (pstDst->enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
			ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly = 0;
			writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		}
		else{
			ive_gmm_c.REG_GMM_13.reg_gmm_gmm2_yonly = 1;
			writel(ive_gmm_c.REG_GMM_13.val, (IVE_BLK_BA[dev_id].GMM + IVE_GMM_REG_GMM_13));
		}

		// top
		ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "Resize", dev_id);

		// setting
		// TODO: need softrst?
		ive_top_c->REG_RS_SRC_SIZE.reg_resize_src_wd =
			pstSrc->u32Width - 1;
		ive_top_c->REG_RS_SRC_SIZE.reg_resize_src_ht =
			pstSrc->u32Height - 1;
		ive_top_c->REG_RS_DST_SIZE.reg_resize_dst_wd =
			pstDst->u32Width - 1;
		ive_top_c->REG_RS_DST_SIZE.reg_resize_dst_ht =
			pstDst->u32Height - 1;
		writel(ive_top_c->REG_RS_SRC_SIZE.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_SRC_SIZE));
		writel(ive_top_c->REG_RS_DST_SIZE.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_DST_SIZE));
		_sclr_get_2tap_scale(ive_top_c, pstSrc, pstDst, dev_id);

		ive_top_c->REG_RS_CTRL.reg_resize_ip_en = 1;
		ive_top_c->REG_RS_CTRL.reg_resize_dbg_en = 0;
		writel(ive_top_c->REG_RS_CTRL.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_CTRL));

		if (pstResizeCtrl->enMode == IVE_RESIZE_MODE_LINEAR) {
			ive_top_c->REG_RS_CTRL.reg_resize_blnr_mode = 1;
			writel(ive_top_c->REG_RS_CTRL.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_RS_CTRL));
		}

		//filter3ch bypass
		if (pstSrc->enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
			ive_filterop_c.REG_h10.reg_filterop_mode = 1;
			ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 0;
			img_in_c.REG_00.reg_auto_csc_en = 0;
			ive_filterop_c.REG_h1c8.reg_filterop_op2_csc_enable = 0;
			ive_filterop_c.REG_h14.reg_filterop_3ch_en = 0;
			writel(img_in_c.REG_00.val,
				   (IVE_BLK_BA[dev_id].IMG_IN + IMG_IN_REG_00));

		} else {
			ive_filterop_c.REG_h10.reg_filterop_mode = 2;
			ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
			ive_filterop_c.REG_h14.reg_filterop_sw_ovw_op = 1;
			ive_filterop_c.REG_h1c8.reg_filterop_op2_csc_enable = 0;
			ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 0;

			ive_top_c->REG_20.reg_frame2op_op_mode = 5;
			ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
			writel(ive_top_c->REG_R2Y4_14.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));
			writel(ive_top_c->REG_20.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
			writel(ive_top_c->REG_h80.val,
				   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		}
		writel(ive_filterop_c.REG_h10.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		writel(ive_filterop_c.REG_h1c8.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H1C8));

		set_isp_rdma(MOD_RESIZE, NULL, dev_id);
		if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
			kfree(ive_top_c);
			return CVI_FAILURE;
		}
		// trigger filterop
		//"2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->REG_20.reg_frame2op_op_mode = 5;
		// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
		//d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
		ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
		ive_top_c->REG_h10.reg_filterop_top_enable = 1;
		ive_top_c->REG_h10.reg_resize_top_enable = 1;
		writel(ive_top_c->REG_20.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
		writel(ive_top_c->REG_h80.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
		writel(ive_top_c->REG_h10.val,
			   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

		set_odma(pstDst, &ive_filterop_c, pstDst->u32Width,
			pstDst->u32Height, dev_id);
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

		//if (astSrc[i].u32Width > 480) {
		//	ret = emit_tile(ndev, &ive_top_c, &ive_filterop_c, &img_in_c,
		//		NULL, NULL, NULL, NULL, &astSrc[i], NULL,
		//		NULL, &astDst[i], NULL, false, 1, false, 1, true, 0,
		//		bInstant);
		//} else {
			ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK,
				MOD_RESIZE, dev_id);
		//}
	// }
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_ccl(struct cvi_ive_device *ndev, IVE_IMAGE_S *stSrcDst,
			IVE_DST_MEM_INFO_S *stBlob, IVE_CCL_CTRL_S *stCclCtrl,
			bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	u64 srcAddr, dstAddr;
	u8 u8RegionNum = 0;
	s8 s8LabelStatus = 0;
	u16 u16CurAreaThr = 0;

	IVE_CCL_C ive_ccl_c = _DEFINE_IVE_CCL_C;
	ISP_DMA_CTL_C ccl_src_rdma_label = _DEFINE_ISP_DMA_CTL_C;
	ISP_DMA_CTL_C ccl_dst_wdma_label = _DEFINE_ISP_DMA_CTL_C;
	ISP_DMA_CTL_C ccl_region_wdma = _DEFINE_ISP_DMA_CTL_C;
	ISP_DMA_CTL_C ccl_src_rdma_relabel = _DEFINE_ISP_DMA_CTL_C;
	ISP_DMA_CTL_C ccl_dst_wdma_relabel = _DEFINE_ISP_DMA_CTL_C;

	IVE_TOP_C *ive_top_c = init_ive_top_c();
    if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_CCL\n");
	ive_reset(ndev, ive_top_c, dev_id);

	ive_set_wh(ive_top_c, stSrcDst->u32Width, stSrcDst->u32Height, "CCL", dev_id);

	// set ccl mode
	ive_ccl_c.REG_CCL_00.reg_ccl_mode = stCclCtrl->enMode;
	ive_ccl_c.REG_CCL_01.reg_ccl_area_step = stCclCtrl->u16Step;
	ive_ccl_c.REG_CCL_01.reg_ccl_area_thr = stCclCtrl->u16InitAreaThr;

	writel(ive_ccl_c.REG_CCL_00.val, (IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_00));
	writel(ive_ccl_c.REG_CCL_01.val, (IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_01));

    set_isp_rdma(MOD_CCL, NULL, dev_id);

	srcAddr = stSrcDst->u64PhyAddr[0];
	dstAddr = stBlob->u64PhyAddr;

	if(srcAddr){
		// SRC RDMA ADDR
		ccl_src_rdma_label.BASE_ADDR.reg_basel = srcAddr & 0xffffffff;
		ccl_src_rdma_label.SYS_CONTROL.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_src_rdma_label.SYS_CONTROL.reg_base_sel = 1;

		writel(ccl_src_rdma_label.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_src_rdma_label.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA + ISP_DMA_CTL_SYS_CONTROL));

		// DST RDMA ADDR
		ccl_dst_wdma_label.BASE_ADDR.reg_basel = srcAddr & 0xffffffff;
		ccl_dst_wdma_label.SYS_CONTROL.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_dst_wdma_label.SYS_CONTROL.reg_base_sel = 1;

		writel(ccl_dst_wdma_label.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_dst_wdma_label.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA + ISP_DMA_CTL_SYS_CONTROL));

		ccl_src_rdma_relabel.BASE_ADDR.reg_basel = srcAddr & 0xffffffff;
		ccl_src_rdma_relabel.SYS_CONTROL.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_src_rdma_relabel.SYS_CONTROL.reg_base_sel = 1;

		writel(ccl_src_rdma_relabel.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA_RELABEL + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_src_rdma_relabel.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].CCL_SRC_RDMA_RELABEL + ISP_DMA_CTL_SYS_CONTROL));

		ccl_dst_wdma_relabel.BASE_ADDR.reg_basel = srcAddr & 0xffffffff;
		ccl_dst_wdma_relabel.SYS_CONTROL.reg_baseh = (srcAddr >> 32) & 0xffffffff;
		ccl_dst_wdma_relabel.SYS_CONTROL.reg_base_sel = 1;

		writel(ccl_dst_wdma_relabel.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA_RELABEL + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_dst_wdma_relabel.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].CCL_DST_WDMA_RELABEL + ISP_DMA_CTL_SYS_CONTROL));
	}

	if(dstAddr){
		dstAddr = dstAddr + 0x10;  // skip first 32bits in struct IVE_CCBLOB_S and 16 Bytes align
		ccl_region_wdma.BASE_ADDR.reg_basel = dstAddr & 0xffffffff;
		ccl_region_wdma.SYS_CONTROL.reg_baseh = (dstAddr >> 32) & 0xffffffff;
		ccl_region_wdma.SYS_CONTROL.reg_base_sel = 1;

		writel(ccl_region_wdma.BASE_ADDR.val,
			(IVE_BLK_BA[dev_id].CCL_REGION_WDMA + ISP_DMA_CTL_BASE_ADDR));
		writel(ccl_region_wdma.SYS_CONTROL.val,
			(IVE_BLK_BA[dev_id].CCL_REGION_WDMA + ISP_DMA_CTL_SYS_CONTROL));
	}

	ive_top_c->REG_h10.reg_ccl_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	ndev->cur_optype = MOD_CCL;
	ret = cvi_ive_go(ndev, ive_top_c, bInstant, IVE_TOP_REG_FRAME_DONE_CCL_MASK,
			   MOD_CCL, dev_id);

	if(ret == CVI_SUCCESS) {
		u8RegionNum = (readl(IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_03) &
		        IVE_CCL_REG_CCL_REGION_NUM_MASK) >> IVE_CCL_REG_CCL_REGION_NUM_OFFSET;
		s8LabelStatus = (readl(IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_03) &
		        IVE_CCL_REG_CCL_LABEL_STATUS_MASK) >> IVE_CCL_REG_CCL_LABEL_STATUS_OFFSET;
		u16CurAreaThr = (readl(IVE_BLK_BA[dev_id].CCL + IVE_CCL_REG_CCL_03) &
		        IVE_CCL_REG_CCL_CUR_AREA_THR_MASK)>> IVE_CCL_REG_CCL_CUR_AREA_THR_OFFSET;

		memcpy((char *)(uintptr_t)stBlob->u64VirAddr, &u16CurAreaThr, sizeof(u16));
		memcpy((char *)(uintptr_t)stBlob->u64VirAddr + sizeof(u16), &s8LabelStatus, sizeof(s8));
		memcpy((char *)(uintptr_t)stBlob->u64VirAddr + sizeof(u16) + sizeof(s8),
				&u8RegionNum, sizeof(u8));
	}

	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_imgIn_to_odma(struct cvi_ive_device *ndev,
				IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
				IVE_FILTER_CTRL_S *pstFltCtrl, bool bInstant, s32 dev_id)
{
	s32 ret = 0;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_imgInToOdma\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstDst", pstDst);
	}
	// top
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "ImgInToOdma", dev_id);

	set_isp_rdma(MOD_BYP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	ive_filterop_c.REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c.REG_h14.reg_filterop_3ch_en = 0;

	if (pstDst->enType == IVE_IMAGE_TYPE_U8C1) {
		set_img_dst1(pstDst, &wdma_y_ctl_c, dev_id);
		ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
		ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
		writel(ive_filterop_c.REG_h14.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
		ive_filterop_c.ODMA_REG_00.reg_dma_en = 0;
		writel(ive_filterop_c.ODMA_REG_00.val,
			   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_ODMA_REG_00));
		if (pstDst->u32Width > 480) {
			ret = emit_tile(ndev, ive_top_c, &ive_filterop_c,
					&img_in_c, &wdma_y_ctl_c, NULL, NULL,
					NULL, pstSrc, NULL, NULL, pstDst, NULL,
					true, 1, false, 1, false, 0, bInstant, dev_id);
			kfree(ive_top_c);
			return ret;
		}
	}
	set_odma(pstDst, &ive_filterop_c, pstDst->u32Width, pstDst->u32Height, dev_id);
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstDst->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, &ive_filterop_c, &img_in_c,
				NULL, NULL, NULL, NULL, pstSrc, NULL, NULL,
				pstDst, NULL, false, 1, false, 1, true, MOD_BYP,
				bInstant, dev_id);
		kfree(ive_top_c);
		return ret;
	}

	ret = cvi_ive_go(ndev, ive_top_c, bInstant,
			  IVE_TOP_REG_FRAME_DONE_FILTEROP_ODMA_MASK, MOD_BYP, dev_id);
	kfree(ive_top_c);
	return ret;
}

s32 cvi_ive_rgbp_to_yuv_to_erode_to_dilate(struct cvi_ive_device *ndev,
					 IVE_SRC_IMAGE_S *pstSrc,
					 IVE_DST_IMAGE_S *pstDst,
					 IVE_DST_IMAGE_S *pstDst2,
					 IVE_FILTER_CTRL_S *pstFltCtrl,
					 bool bInstant, s32 dev_id)
{
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_rgbPToYuvToErodeToDilate\n");
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "rgbPToYuvToEToD", dev_id);

	set_isp_rdma(MOD_BYP, NULL, dev_id);
	if (set_img_src1(pstSrc, &img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		kfree(ive_top_c);
		return CVI_FAILURE;
	}
	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	// "2 frame operation_mode 3'd0: And 3'd1: Or 3'd2: Xor 3'd3: Add 3'd4: Sub 3'
	// d5: Bypass mode output =frame source 0 3'd6: Bypass mode output =frame source 1 default: And"
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	ive_top_c->REG_R2Y4_14.reg_csc_r2y4_enable = 1;
	ive_top_c->REG_h10.reg_r2y4_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_R2Y4_14.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_R2Y4_14));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, NULL, dev_id);
	ive_filterop_c.REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c.REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c.REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	//iveReg->setDMA(pstSrc, pstDst, NULL, pstDst2);
	cvi_ive_go(ndev, ive_top_c, bInstant,
		   IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK, MOD_BYP, dev_id);
	kfree(ive_top_c);
	return CVI_SUCCESS;
}

s32 cvi_ive_hw_equalize_hist(struct cvi_ive_device *ndev,
				IVE_SRC_IMAGE_S *pstSrc,
				IVE_DST_IMAGE_S *pstDst,
				IVE_EQUALIZE_HIST_CTRL_S *pstEqualizeHistCtrl,
				bool bInstant)
{
	pr_err("CVI_MPI_IVE_EqualizeHist not implement yet\n");
	return CVI_SUCCESS;
}

s32
_cvi_hw_stbox_flt_and_eig_calc(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst, s8 *as8Mask,
			   u16 *u16MaxEig, bool bInstant,
			   IVE_TOP_C *ive_top_c, IVE_FILTEROP_C *ive_filterop_c,
			   IMG_IN_C *img_in_c, ISP_DMA_CTL_C *wdma_y_ctl_c, s32 dev_id)
{
	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_HW_STBoxFltAndEigCalc\n");
	// top
	ive_set_wh(ive_top_c, pstSrc->u32Width, pstSrc->u32Height, "STBoxFAndEigCal", dev_id);

	ive_filterop_c->REG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 1;
	writel(ive_filterop_c->REG_ST_EIGVAL_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_1));
	ive_filterop_c->REG_h10.reg_filterop_mode = 5;
	ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
	ive_filterop_c->REG_28.reg_filterop_op2_erodila_en = 0;
	ive_filterop_c->REG_CANNY_0.reg_canny_lowthr = 0;
	ive_filterop_c->REG_CANNY_0.reg_canny_hithr = 0;
	ive_filterop_c->REG_CANNY_1.reg_canny_en = 0;
	ive_filterop_c->REG_CANNY_3.reg_canny_basel = 0;
	ive_filterop_c->REG_CANNY_4.reg_canny_baseh = 0;
	ive_filterop_c->REG_ST_CANDI_0.reg_st_candi_corner_bypass = 0x0;
	ive_filterop_c->REG_110.reg_filterop_norm_out_ctrl = 3; //1;
	ive_filterop_c->REG_h14.reg_filterop_3ch_en = 0;
	ive_filterop_c->REG_ST_EIGVAL_0.reg_st_eigval_tile_num = 0;
	ive_filterop_c->REG_ST_EIGVAL_1.reg_sw_clr_max_eigval = 1;
	writel(ive_filterop_c->REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	writel(ive_filterop_c->REG_28.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_28));
	writel(ive_filterop_c->REG_CANNY_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_0));
	writel(ive_filterop_c->REG_CANNY_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_1));
	writel(ive_filterop_c->REG_CANNY_3.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_3));
	writel(ive_filterop_c->REG_CANNY_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_CANNY_4));
	writel(ive_filterop_c->REG_ST_CANDI_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_CANDI_0));
	writel(ive_filterop_c->REG_110.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_110));
	writel(ive_filterop_c->REG_ST_EIGVAL_0.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_0));
	writel(ive_filterop_c->REG_ST_EIGVAL_1.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_1));
	ive_filterop_c->REG_4.reg_filterop_h_coef00 = as8Mask[0];
	ive_filterop_c->REG_4.reg_filterop_h_coef01 = as8Mask[1];
	ive_filterop_c->REG_4.reg_filterop_h_coef02 = as8Mask[2];
	ive_filterop_c->REG_4.reg_filterop_h_coef03 = as8Mask[3];
	ive_filterop_c->REG_5.reg_filterop_h_coef04 = as8Mask[4];
	ive_filterop_c->REG_5.reg_filterop_h_coef10 = as8Mask[5];
	ive_filterop_c->REG_5.reg_filterop_h_coef11 = as8Mask[6];
	ive_filterop_c->REG_5.reg_filterop_h_coef12 = as8Mask[7];
	ive_filterop_c->REG_6.reg_filterop_h_coef13 = as8Mask[8];
	ive_filterop_c->REG_6.reg_filterop_h_coef14 = as8Mask[9];
	ive_filterop_c->REG_6.reg_filterop_h_coef20 = as8Mask[10];
	ive_filterop_c->REG_6.reg_filterop_h_coef21 = as8Mask[11];
	ive_filterop_c->REG_7.reg_filterop_h_coef22 = as8Mask[12];
	ive_filterop_c->REG_7.reg_filterop_h_coef23 = as8Mask[13];
	ive_filterop_c->REG_7.reg_filterop_h_coef24 = as8Mask[14];
	ive_filterop_c->REG_7.reg_filterop_h_coef30 = as8Mask[15];
	ive_filterop_c->REG_8.reg_filterop_h_coef31 = as8Mask[16];
	ive_filterop_c->REG_8.reg_filterop_h_coef32 = as8Mask[17];
	ive_filterop_c->REG_8.reg_filterop_h_coef33 = as8Mask[18];
	ive_filterop_c->REG_8.reg_filterop_h_coef34 = as8Mask[19];
	ive_filterop_c->REG_9.reg_filterop_h_coef40 = as8Mask[20];
	ive_filterop_c->REG_9.reg_filterop_h_coef41 = as8Mask[21];
	ive_filterop_c->REG_9.reg_filterop_h_coef42 = as8Mask[22];
	ive_filterop_c->REG_9.reg_filterop_h_coef43 = as8Mask[23];
	ive_filterop_c->REG_10.reg_filterop_h_coef44 = as8Mask[24];
	ive_filterop_c->REG_10.reg_filterop_h_norm = 3;
	writel(ive_filterop_c->REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c->REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c->REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c->REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c->REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c->REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c->REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	set_isp_rdma(MOD_STBOX, NULL, dev_id);
	if (set_img_src1(pstSrc, img_in_c, ive_top_c, dev_id) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	ive_top_c->REG_20.reg_frame2op_op_mode = 5;
	ive_top_c->REG_h80.reg_frame2op_fg_op_mode = 6;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_20.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_20));
	writel(ive_top_c->REG_h80.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H80));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_img_dst1(pstDst, wdma_y_ctl_c, dev_id);

	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));

	if (pstSrc->u32Width > 480) {
		emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
			 wdma_y_ctl_c, NULL, NULL, NULL, pstSrc, NULL, NULL,
			 pstDst, NULL, true, 2, false, 1, false, MOD_STBOX,
			 bInstant, dev_id);
	} else {
		cvi_ive_go(ndev, ive_top_c, bInstant,
			   IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
			   MOD_STBOX, dev_id);
	}

	*u16MaxEig = readl(IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_ST_EIGVAL_0) &
			 IVE_FILTEROP_REG_ST_EIGVAL_MAX_EIGVAL_MASK;
	return CVI_SUCCESS;
}

s32 _cvi_hw_stcandi_corner(struct cvi_ive_device *ndev, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDst, u16 u16MaxEig,
			  IVE_ST_CANDI_CORNER_CTRL_S *pstStCandiCornerCtrl,
			  bool bInstant, IVE_TOP_C *ive_top_c,
			  IVE_FILTEROP_C *ive_filterop_c, IMG_IN_C *img_in_c,
			  ISP_DMA_CTL_C *wdma_y_ctl_c,
			  ISP_DMA_CTL_C *rdma_eigval_ctl_c,
			  s32 dev_id)
{
	s32 ret = CVI_SUCCESS;
	u8 u8Numerator = 255;

	//in case of tile mode
	ive_set_wh(ive_top_c, pstDst->u32Width, pstDst->u32Height, "STCandiCorner", dev_id);

	ive_top_c->REG_h130.reg_thresh_top_mod = 1;
	ive_top_c->REG_h130.reg_thresh_thresh_en = 1;
	ive_top_c->REG_h10.reg_thresh_top_enable = 1;
	ive_top_c->REG_h10.reg_filterop_top_enable = 1;
	writel(ive_top_c->REG_h130.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));
	// setting
	ive_filterop_c->REG_h10.reg_filterop_mode = 10;
	ive_filterop_c->REG_h14.reg_filterop_op1_cmd = 0;
	ive_filterop_c->REG_h14.reg_filterop_sw_ovw_op = 0;
	writel(ive_filterop_c->REG_h10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H10));
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ive_top_c->REG_h134.val = IVE_TOP_16_8_8(
		readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H134) & 0xF8, 0, 0,
		IVE_16BIT_TO_8BIT_MODE_U16_TO_U8);
	ive_top_c->REG_h138.val = IVE_TOP_16_8_8(
		readl(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H138) & 0xFFFF00FE, 0,
		u8Numerator, 1);
	writel(ive_top_c->REG_h134.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H134));
	writel(ive_top_c->REG_h138.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H138));

	ive_top_c->REG_h14c.val =
		IVE_TOP_16_8_8(0, IVE_THRESH_MODE_TO_MINVAL, 0,
				   pstStCandiCornerCtrl->u0q8QualityLevel);
	ive_top_c->REG_h150.val = 0;
	ive_filterop_c->REG_18.reg_filterop_order_enmode =
		IVE_ORD_STAT_FILTER_MODE_MAX;
	writel(ive_top_c->REG_h14c.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H14C));
	writel(ive_top_c->REG_h150.val,
		   (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H150));
	writel(ive_filterop_c->REG_18.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_18));
	ive_filterop_c->REG_4.val = 0;
	ive_filterop_c->REG_5.val = 0;
	ive_filterop_c->REG_6.val = 0;
	ive_filterop_c->REG_7.val = 0;
	ive_filterop_c->REG_8.val = 0;
	ive_filterop_c->REG_9.val = 0;
	ive_filterop_c->REG_10.reg_filterop_h_coef44 = 0;
	ive_filterop_c->REG_10.reg_filterop_h_norm = 0;
	writel(ive_filterop_c->REG_4.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_4));
	writel(ive_filterop_c->REG_5.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_5));
	writel(ive_filterop_c->REG_6.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_6));
	writel(ive_filterop_c->REG_7.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_7));
	writel(ive_filterop_c->REG_8.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_8));
	writel(ive_filterop_c->REG_9.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_9));
	writel(ive_filterop_c->REG_10.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_10));

	ive_top_c->REG_h10.reg_img_in_top_enable = 0;
	ive_top_c->REG_h10.reg_rdma_eigval_top_enable = 1;
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	set_isp_rdma(MOD_STCANDI, NULL, dev_id);
	set_rdma_eigval(pstSrc, rdma_eigval_ctl_c, dev_id);

	ive_top_c->REG_3.reg_mapmux_rdma_sel = 1;
	ive_top_c->REG_3.reg_ive_rdma_eigval_en = 1;
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));

	set_img_dst1(pstDst, wdma_y_ctl_c, dev_id);
	ive_filterop_c->REG_h14.reg_op_y_wdma_en = 1;
	ive_filterop_c->REG_h14.reg_op_c_wdma_en = 0;
	writel(ive_filterop_c->REG_h14.val,
		   (IVE_BLK_BA[dev_id].FILTEROP + IVE_FILTEROP_REG_H14));
	ndev->cur_optype = MOD_STCANDI;
	if (pstSrc->u32Width > 480) {
		ret = emit_tile(ndev, ive_top_c, ive_filterop_c, img_in_c,
				   wdma_y_ctl_c, NULL, NULL, rdma_eigval_ctl_c,
				   pstSrc, NULL, NULL, pstDst, NULL, true, 1, false,
				   1, false, MOD_STCANDI, bInstant, dev_id);
	} else {
		ret = cvi_ive_go(ndev, ive_top_c, bInstant,
				 IVE_TOP_REG_FRAME_DONE_FILTEROP_WDMA_Y_MASK,
				 MOD_STCANDI, dev_id);
	}

	//Restore setting
	ive_top_c->REG_3.reg_mapmux_rdma_sel = 0;
	ive_top_c->REG_3.reg_ive_rdma_eigval_en = 0;
	ive_top_c->REG_h130.reg_thresh_top_mod = 0;
	ive_top_c->REG_h130.reg_thresh_thresh_en = 0;
	ive_top_c->REG_h138.reg_thresh_st_16to8_en = 0;
	ive_top_c->REG_h10.reg_thresh_top_enable = 0;
	ive_top_c->REG_h10.reg_rdma_eigval_top_enable = 0;
	writel(ive_top_c->REG_3.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_3));
	writel(ive_top_c->REG_h130.val,
		(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H130));
	writel(ive_top_c->REG_h138.val,
		(IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H138));
	writel(ive_top_c->REG_h10.val, (IVE_BLK_BA[dev_id].IVE_TOP + IVE_TOP_REG_H10));

	return ret;
}

s32 cvi_ive_stcandi_corner(struct cvi_ive_device *ndev,
				  IVE_SRC_IMAGE_S *pstSrc,
				  IVE_DST_IMAGE_S *pstCandiCorner,
				  IVE_ST_CANDI_CORNER_CTRL_S *pstStCandiCornerCtrl,
				  bool bInstant, s32 dev_id)
{
	s32 ret = CVI_SUCCESS, i;
	u32 u32SizeS8C2, u32SizeU8C1;
	u16 u16MaxEig;
	IVE_DST_IMAGE_S stEigenMap;
	s8 as8Mask[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
				   2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
	//DEFINE_IVE_FILTEROP_C(ive_filterop_c);
	IVE_FILTEROP_C ive_filterop_c = _DEFINE_IVE_FILTEROP_C;
	//DEFINE_IMG_IN_C(img_in_c);
	IMG_IN_C img_in_c = _DEFINE_IMG_IN_C;
	//DEFINE_ISP_DMA_CTL_C(wdma_y_ctl_c);
	ISP_DMA_CTL_C wdma_y_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_ISP_DMA_CTL_C(rdma_eigval_ctl_c);
	ISP_DMA_CTL_C rdma_eigval_ctl_c = _DEFINE_ISP_DMA_CTL_C;
	//DEFINE_IVE_TOP_C(ive_top_c);
	IVE_TOP_C *ive_top_c = init_ive_top_c();
	if (!ive_top_c) {
		pr_err("ive_top_c init failed\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	CVI_TRACE_IVE(CVI_DBG_INFO, "CVI_MPI_IVE_STCandiCorner\n");
	ive_reset(ndev, ive_top_c, dev_id);
	if (g_dump_image_info == CVI_TRUE) {
		dump_ive_image("pstSrc", pstSrc);
		dump_ive_image("pstCandiCorner", pstCandiCorner);
	}
	if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1) {
		pr_err("pstSrc->enType(%d) must be U8C1(%d)\n", pstSrc->enType,
			   IVE_IMAGE_TYPE_U8C1);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	if (pstCandiCorner->enType != IVE_IMAGE_TYPE_U8C1) {
		pr_err("pstCandiCorner->enType(%d) must be U8C1(%d)\n",
			   pstCandiCorner->enType, IVE_IMAGE_TYPE_U8C1);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	if (pstSrc->u32Height != pstCandiCorner->u32Height ||
		pstSrc->u32Width != pstCandiCorner->u32Width) {
		pr_err("pstCandiCorner->u32Width(%d) and pstCandiCorner->u32Height(%d) ",
			   pstCandiCorner->u32Width, pstCandiCorner->u32Height);
		pr_err("must be equal to pstSrc->u32Width(%d) and pstSrc->u32Height(%d)\n",
			   pstSrc->u32Width, pstSrc->u32Height);
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	if (!pstStCandiCornerCtrl->stMem.u64PhyAddr) {
		pr_err("pstStCandiCornerCtrl->stMem.u64PhyAddr can't be 0!\n");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	if (pstStCandiCornerCtrl->stMem.u32Size <
		4 * pstSrc->u32Height * pstSrc->u32Stride[0] +
			sizeof(IVE_ST_MAX_EIG_S)) {
		pr_err("pstStCandiCornerCtrl->stMem.u32Size must be greater than or equal to %zu!\n",
			   4 * pstSrc->u32Height * pstSrc->u32Stride[0] +
				   sizeof(IVE_ST_MAX_EIG_S));
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	if (pstStCandiCornerCtrl->u0q8QualityLevel == 0) {
		pr_err("pstStCandiCornerCtrl->u0q8QualityLevel can't be 0!");
		kfree(ive_top_c);
		return CVI_FAILURE;
	}

	u32SizeS8C2 = pstSrc->u32Height * pstSrc->u32Stride[0] * 2;
	u32SizeU8C1 = pstSrc->u32Height * pstSrc->u32Stride[0];
	stEigenMap.u64PhyAddr[0] =
		pstStCandiCornerCtrl->stMem.u64PhyAddr + u32SizeS8C2;
	stEigenMap.enType = IVE_IMAGE_TYPE_S8C2_PACKAGE;
	stEigenMap.u32Width = pstSrc->u32Width;
	stEigenMap.u32Height = pstSrc->u32Height;
	stEigenMap.u32Stride[0] = pstSrc->u32Stride[0] * 2;

	// _CVI_HW_STBoxFltAndEigCalc(ndev, pstSrc, &stEigenMap, as8Mask,
	// 			   &u16MaxEig, bInstant, ive_top_c,
	// 			   &ive_filterop_c, &img_in_c, &wdma_y_ctl_c, dev_id);

	// if (u16MaxEig == 65535) {
	// 	_CVI_HW_STBoxFltAndEigCalc(ndev, pstSrc, &stEigenMap, as8Mask,
	// 				   &u16MaxEig, bInstant, ive_top_c,
	// 				   &ive_filterop_c, &img_in_c,
	// 				   &wdma_y_ctl_c, dev_id);
	// }

	for(i = 0; i < 2; i++){
		_cvi_hw_stbox_flt_and_eig_calc(ndev, pstSrc, &stEigenMap, as8Mask,
				   &u16MaxEig, bInstant, ive_top_c,
				   &ive_filterop_c, &img_in_c, &wdma_y_ctl_c, dev_id);
	}

	g_u32SizeS8C2 = u32SizeS8C2;
	g_u16MaxEig = u16MaxEig;
	g_pStCandiCornerCtrl = pstStCandiCornerCtrl;
	ret = _cvi_hw_stcandi_corner(ndev, &stEigenMap, pstCandiCorner,
					u16MaxEig, pstStCandiCornerCtrl, bInstant,
					ive_top_c, &ive_filterop_c, &img_in_c,
					&wdma_y_ctl_c, &rdma_eigval_ctl_c, dev_id);

	// if (bInstant) {
	// 	ret = copy_to_user((void __user *)(unsigned long)
	// 			pstStCandiCornerCtrl->stMem.u64VirAddr + u32SizeS8C2 * 2,
	// 			&u16MaxEig, sizeof(u16));
	// }
	kfree(ive_top_c);
	return ret;
}

void stcandicorner_workaround(struct cvi_ive_device *ndev)
{
	u32 u32Len, i;
	IVE_IMAGE_S pstSrc, pstDst;
	IVE_ST_CANDI_CORNER_CTRL_S stStCandiCornerCtrl;

	pstSrc.enType = IVE_IMAGE_TYPE_U8C1;
	pstSrc.u32Stride[0] = 64;
	pstSrc.u32Width = 64;
	pstSrc.u32Height = 64;
	pstDst.enType = IVE_IMAGE_TYPE_U8C1;
	pstDst.u32Stride[0] = 64;
	pstDst.u32Width = 64;
	pstDst.u32Height = 64;
	u32Len = pstSrc.u32Stride[0] * 64;
	base_ion_alloc(&pstSrc.u64PhyAddr[0], (void **)&pstSrc.u64VirAddr[0],
		"st_src_ive_mesh", u32Len, false);
	base_ion_alloc(&pstDst.u64PhyAddr[0], (void **)&pstDst.u64VirAddr[0],
		"st_dst_ive_mesh", u32Len, false);
	stStCandiCornerCtrl.u0q8QualityLevel = 25;
	base_ion_alloc((u64 *)&stStCandiCornerCtrl.stMem.u64PhyAddr,
		(void **)&stStCandiCornerCtrl.stMem.u64VirAddr,
		"st_ive_mesh", 4 * pstSrc.u32Height * pstSrc.u32Stride[0] +
		sizeof(IVE_ST_MAX_EIG_S), false);

	for (i = 0; i < IVE_DEV_MAX; i++) {
		cvi_ive_stcandi_corner(ndev, &pstSrc,
			&pstDst, &stStCandiCornerCtrl, 0, i);
	}
	base_ion_free(pstSrc.u64PhyAddr[0]);
	base_ion_free(pstDst.u64PhyAddr[0]);
	base_ion_free(stStCandiCornerCtrl.stMem.u64PhyAddr);
}

irqreturn_t platform_ive_irq(struct cvi_ive_device *ndev, int dev_id)
{
	//pr_err("[IVE] got %s callback\n", __func__);
	complete(&ndev->core[dev_id].frame_done);
	stop_vld_time(ndev->cur_optype, ndev->core[dev_id].tile_num);
	return IRQ_HANDLED;
}

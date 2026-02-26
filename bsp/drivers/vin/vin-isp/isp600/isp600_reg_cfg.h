/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * isp600_reg_cfg.h
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

#ifndef _ISP600_REG_CFG_H_
#define _ISP600_REG_CFG_H_

#define ISP_VIRT_NUM 4

#define ISP_ADDR_BIT_R_SHIFT 2

/*load data*/
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
#define ISP_LOAD_DRAM_SIZE			0xAF40
#else
#define ISP_LOAD_DRAM_SIZE			0x7700
#endif
#define ISP_LOAD_REG_SIZE			0x1000
#define ISP_FE_TBL_SIZE				0x1800
#define ISP_BAYER_TABLE_SIZE		0x2600
#define ISP_RGB_TABLE_SIZE			0x1200
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
#define ISP_YUV_TABLE_SIZE			0x4F40
#else
#define ISP_YUV_TABLE_SIZE			0x1700
#endif
#define ISP_DBG_TABLE_SIZE			0x0000

/*save data*/
#define ISP_SAVE_DRAM_SIZE			0x6a00
#define ISP_SAVE_REG_SIZE			0x0100
#define ISP_STATISTIC_SIZE			0x6900
#define ISP_STAT_TOTAL_SIZE			0x6a00
#define ISP_DBG_STAT_SIZE			0x0000

/*save and load data*/
#define ISP_SAVE_LOAD_DRAM_SIZE			0x3400
#define ISP_SAVE_LOAD_REG_SIZE			0x0100
#define ISP_SAVE_LOAD_STATISTIC_SIZE	0x3300

/*bayer table*/
#define ISP_RSC_TBL_SIZE			0x0800
//#define ISP_MSC_TBL_SIZE			0x0800
#define ISP_MSC_NR_TBL_SIZE			0x03c8
#define ISP_D3D_DK_TBL_SIZE			0x0800
#define ISP_PLTM_TM_TBL_SIZE		0x0800
#define ISP_PLTM_LM_TBL_SIZE		0x0200

/*rgbtable*/
#define ISP_RGB_DRC_TBL_SIZE		0x0200
#define ISP_RGB_GAMMA_TBL_SIZE		0x1000

/*yuv table*/
#define ISP_CEM_TBL0_SIZE			0x0cc0
#define ISP_CEM_TBL1_SIZE			0x0a40

/*save statistics*/
#define ISP_STAT_AE_MEM_SIZE		0x0360
#define ISP_STAT_RES_SIZE			0x0020
#define ISP_STAT_AF_IIR_ACC_SIZE	0x0c00
#define ISP_STAT_AF_FIR_ACC_SIZE	0x0c00
#define ISP_STAT_AF_IIR_CNT_SIZE	0x0c00
#define ISP_STAT_AF_FIR_CNT_SIZE	0x0c00
#define ISP_STAT_AF_HL_CNT_SIZE		0x0c00
#define ISP_STAT_AWB_RGB_MEM_SIZE	0x1800
#define ISP_STAT_AFS_MEM_SIZE		0x0200
#define ISP_STAT_HIST0_MEM_SIZE		0x0400
#define ISP_STAT_HIST1_MEM_SIZE		0x0400
#define ISP_STAT_PLTM_LUM_SIZE		0x0780

#define ISP_STAT_HIST0_MEM_OFS			0x5a80

/*save and load statistics*/
#define ISP_STAT_PLTM_PKX_SIZE		0x3000
#define ISP_STAT_D3D_K_SIZE			0x0300

/*2in1 large_image mode save data*/
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define ISP_AHB0_MEM_OFS			0x20
#define ISP_AHB1_MEM_OFS			0x40
#define ISP_AHB_MEM_SIZE			0x20
#else
#define ISP_AHB0_MEM_OFS			0x20
#define ISP_AHB1_MEM_OFS			0x50
#define ISP_AHB_MEM_SIZE			0x30
#endif
#define ISP_AE_REG_OFS				0x650
#define ISP_AF_REG_OFS				0x670
#define ISP_AWB_REG_OFS				0x6e0
#define ISP_HIST_REG_OFS			0x710
#define ISP_AE_REG_SIZE				0x8
#define ISP_AF_REG_SIZE				0xc
#define ISP_AWB_REG_SIZE			0x8
#define ISP_HIST_REG_SIZE			0x20

#define LENS_UPDATE				(1 << 8)
#define GAMMA_UPDATE			(1 << 9)
#define DRC_UPDATE				(1 << 10)
#define D3D_UPDATE				(1 << 13)
#define PLTM_UPDATE				(1 << 14)
#define CEM_UPDATE				(1 << 15)
#define MSC_UPDATE				(1 << 16)
#define FE0_MSC_UPDATE			(1 << 17)
#define NR_MSC_UPDATE			(1 << 18)
#define FE1_MSC_UPDATE			(1 << 19)
#define FE2_MSC_UPDATE			(1 << 20)

#define TABLE_UPDATE_ALL 		0x1fE700

/*
 *  ISP Module enable
 */

#define WDR_STITCH_EN			(1 << 0)
#define DPC_EN					(1 << 1)
#define CTC_EN					(1 << 2)
#define GCA_EN					(1 << 3)
#define D2D_EN					(1 << 4)
#define D3D_EN					(1 << 5)
#define BLC_EN					(1 << 6)
#define WB_EN					(1 << 7)
#define DG_EN					(1 << 8)
#define RSC_EN     				(1 << 9)
#define MSC_EN     				(1 << 10)
#define PLTM_EN					(1 << 11)
#define LCA_EN					(1 << 12)
#define SHARP_EN				(1 << 13)
#define CCM_EN					(1 << 14)
#define CNR_EN					(1 << 15)
#define DRC_EN					(1 << 16)
#define GAMMA_EN				(1 << 17)
#define CEM_EN					(1 << 18)
#define AE_EN					(1 << 24)
#define AF_EN					(1 << 25)
#define AWB_EN					(1 << 26)
#define AFS_EN					(1 << 27)
#define HIST0_EN				(1 << 28)
#define HIST1_EN				(1 << 29)

#define WDR_EN					(1 << 0)
#define WDR_SPLIT_EN			(1 << 1)
#define CH0_DG_EN				(1 << 8)
#define CH1_DG_EN				(1 << 9)
#define CH2_DG_EN				(1 << 10)

#define ISP_MODULE0_EN_ALL		(0xffffffff)
#define ISP_MODULE1_EN_ALL		(0xffffffff)
/*
 *  ISP interrupt enable
 */
#define FINISH_INT_EN			(1 << 0)
#define START_INT_EN			(1 << 1)
#define PARA_SAVE_INT_EN		(1 << 2)
#define PARA_LOAD_INT_EN		(1 << 3)
#define N_LINE_START_INT_EN		(1 << 4)
#define HB_SHORT_INT_EN			(1 << 9)
#define CFG_ERROR_INT_EN		(1 << 10)
#define FRAME_LOST_INT_EN		(1 << 11)
#define INTER_FIFO_FULL_INT_EN	(1 << 16)
#define WDMA_FIFO_FULL_INT_EN	(1 << 17)
#define WDMA_OVER_BND_INT_EN	(1 << 18)
#define RDMA_FIFO_FULL_INT_EN	(1 << 19)
#define LBC_ERROR_INT_EN		(1 << 20)
#define DDR_RW_ERROR_INT_EN		(1 << 21)
#define AHB_MBUS_W_INT_EN		(1 << 22)

#define ISP_IRQ_EN_ALL	0xffffffff

/*
 *  ISP interrupt status
 */
#define FINISH_PD				(1 << 0)
#define START_PD				(1 << 1)
#define PARA_SAVE_PD			(1 << 2)
#define PARA_LOAD_PD			(1 << 3)
#define N_LINE_START_PD			(1 << 4)
#define HB_SHORT_PD				(1 << 9)
#define CFG_ERROR_PD			(1 << 10)
#define FRAME_LOST_PD			(1 << 11)
#define INTER_FIFO_FULL_PD		(1 << 16)
#define WDMA_FIFO_FULL_PD		(1 << 17)
#define WDMA_OVER_BND_PD		(1 << 18)
#define RDMA_FIFO_FULL_PD		(1 << 19)
#define LBC_ERROR_PD			(1 << 20)
#define DDR_RW_ERROR_PD			(1 << 21)
#define AHB_MBUS_W_PD			(1 << 22)

#define ISP_IRQ_STATUS_ALL		0xffffffff

/*
 *  ISP internal0 status
 */
#define WDMA_PLTM_LUM_FIFO_OV	(1 << 0)
#define WDMA_PLTM_PKX_FIFO_OV	(1 << 1)
#define WDMA_D3D_REC_FIFO_OV	(1 << 2)
#define WDMA_D3D_KSTB_FIFO_OV	(1 << 3)
#define WDMA_D3D_CNTR_FIFO_OV	(1 << 4)
#define WDMA_D3D_CURK_FIFO_OV	(1 << 5)
#define WDMA_LBC_H4DDR_FIFO_OV	(1 << 6)
#define WDMA_LBC_H4DT_FIFO_OV	(1 << 7)
#define RDMA_PLTM_PKX_FIFO_UV	(1 << 9)
#define RDMA_D3D_REC_FIFO_UV	(1 << 10)
#define RDMA_D3D_KSTB_FIFO_UV	(1 << 11)
#define RDMA_D3D_CNTR_FIFO_UV	(1 << 12)
#define RDMA_D3D_CURK_FIFO_UV	(1 << 13)
#define RDMA_D3D_PREK_FIFO_UV	(1 << 14)
#define TWO_BYTE_ERROR			(1 << 21)
#define FMT_CHG_ERROR			(1 << 22)
#define CH2_BTYPE_ERROR			(1 << 24)
#define CH1_BTYPE_ERROR			(1 << 25)
#define CH0_BTYPE_ERROR			(1 << 26)
#define DVLD_ERROR				(1 << 27)
#define HSYNC_ERROR				(1 << 28)
#define VSYNC_ERROR				(1 << 29)
#define WIDTH_ERROR				(1 << 30)
#define HEIGHT_ERROR			(1 << 31)

/*
 *  ISP internal1 status
 */
#define ISP_STATUS_IDLE				0
#define ISP_STATUS_WAIT_ID_RDY		(1 << 0)
#define ISP_STATUS_ID_REF			(1 << 1)
#define ISP_STATUS_ID_CHECK			(1 << 2)
#define ISP_STATUS_READ_SDRAM		(1 << 3)
#define ISP_STATUS_INIT				(1 << 4)
#define ISP_STATUS_RX_RDY			(1 << 5)
#define ISP_STATUS_ISP_PRO_J		(1 << 6)
#define ISP_STATUS_ISP_PRO0			(1 << 7)
#define ISP_STATUS_ISP_PRO1			(1 << 8)
#define ISP_STATUS_WRITE_SDRAM		(1 << 9)
#define ISP_STATUS_FINISH_END_WAIT	(1 << 10)
#define ISP_STATUS_FINISH_END		(1 << 11)
#define ISP_STATUS_CSI_END			(1 << 12)
#define ISP_STATUS_MASK				(0xffff << 0)

#define ISP_CUR_ID					16
#define ISP_CUR_ID_MASK				(0x3 << 16)

#define LBC_DEC_LONG_ERROR		(1 << 24)
#define LBC_DEC_SHORT_ERROR		(1 << 25)
#define LBC_DEC_ERROR			(1 << 26)
#define DBG_LOAK_STATUS			(1 << 31)

/*
 *  ISP internal2 status
 */
#define LCA_BYR_FIFO_W_FULL		(1 << 0)
#define LCA_BYR_FIFO_R_EMP		(1 << 1)
#define LCA_RGB0_FIFO_W_FULL	(1 << 2)
#define LCA_RGB0_FIFO_R_EMP		(1 << 3)
#define DMSC_AVG_FIFO_W_FULL	(1 << 4)
#define DMSC_AVG_FIFO_R_EMP		(1 << 5)
#define DMSC_RGB_FIFO_W_FULL	(1 << 6)
#define DMSC_RGB_FIFO_R_EMP		(1 << 7)
#define DMSC_RATIO_FIFO_W_FULL	(1 << 8)
#define DMSC_RATIO_FIFO_R_EMP	(1 << 9)
#define D3D_DIFF_FIFO_OV		(1 << 10)
#define D3D_LP0_FIFO_OV			(1 << 11)
#define D3D_LP1_FIFO_OV			(1 << 12)
#define D3D_PCNT_FIFO_OV		(1 << 13)
#define D3D_KPACK_FIFO_OV		(1 << 14)
#define D3D_CRKB_FIFO_OV		(1 << 15)
#define D3D_LBC_FIFO_OV			(1 << 16)

struct isp_wdr_mode_cfg {
	unsigned char wdr_ch_seq;
	unsigned char wdr_exp_seq;
	unsigned char wdr_mode;
};

struct isp_lbc_cfg {
	unsigned char mb_num;
	unsigned short mb_min_bit;
	unsigned short mb_min_bits_std[4];
	unsigned short start_qp_std[4];
	unsigned short line_tar_bits;
	unsigned short line_max_bits;
	unsigned short line_stride;
	unsigned short cmp_ratio;

	bool log_en;
	bool is_lossy;
};

enum isp_channel {
	ISP_CH0 = 0,
	ISP_CH1 = 1,
	ISP_CH2 = 2,
	ISP_CH3 = 3,
	ISP_MAX_CH_NUM,
};

struct isp_size {
	u32 width;
	u32 height;
};

struct coor {
	u32 hor;
	u32 ver;
};

struct isp_size_settings {
	struct coor ob_start;
	struct isp_size ob_black;
	struct isp_size ob_valid;
	u32 set_cnt;
};

enum ready_flag {
	PARA_NOT_READY = 0,
	PARA_READY = 1,
};

enum enable_flag {
	DISABLE    = 0,
	ENABLE     = 1,
};

enum isp_input_seq {
	ISP_BGGR = 4,
	ISP_RGGB = 5,
	ISP_GBRG = 6,
	ISP_GRBG = 7,
};

extern int isp_virtual_find_ch[4];
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
extern int isp_virtual_find_logic[7];
extern int isp_virtual_find_sel[7];
extern int isp_ch_find[7];
#else
extern int isp_virtual_find_logic[4];
#endif

void bsp_isp_map_reg_addr(unsigned long id, unsigned long base);
void bsp_isp_map_load_dram_addr(unsigned long id, unsigned long base);
void bsp_isp_map_save_load_dram_addr(unsigned long id, unsigned long base);

/*******isp top control register which we can write directly to register*********/

void bsp_isp_enable(unsigned long id, unsigned int en);
void bsp_isp_mode(unsigned long id, unsigned int mode);
void bsp_isp_top_capture_start(unsigned long id);
void bsp_isp_top_capture_stop(unsigned long id);
void bsp_isp_ver_read_en(unsigned long id, unsigned int en);
void bsp_isp_set_sram_clear(unsigned long id, unsigned int en);
void bsp_isp_set_clk_back_door(unsigned long id, unsigned int en);
unsigned int bsp_isp_get_isp_ver(unsigned long id, unsigned int *major, unsigned int *minor);
unsigned int bsp_isp_get_max_width(unsigned long id);

/*******ispx ahb control register which we can write directly to register*********/

void bsp_isp_capture_start(unsigned long id);
void bsp_isp_capture_stop(unsigned long id);
void bsp_isp_set_para_ready(unsigned long id, int ready);
void bsp_isp_update_table(unsigned long id, unsigned int table_update);
void bsp_isp_set_load_addr(unsigned long id, dma_addr_t addr);
void bsp_isp_irq_enable(unsigned long id, unsigned int irq_flag);
void bsp_isp_irq_disable(unsigned long id, unsigned int irq_flag);
unsigned int bsp_isp_get_irq_status(unsigned long id, unsigned int flag);
void bsp_isp_clr_irq_status(unsigned long id, unsigned int flag);
unsigned int bsp_isp_get_internal_status0(unsigned long id, unsigned int flag);
void bsp_isp_clr_internal_status0(unsigned long id, unsigned int flag);
unsigned int bsp_isp_get_internal_status1(unsigned long id, unsigned int flag);
unsigned int bsp_isp_get_isp_cur_id(unsigned long id);
unsigned int bsp_isp_get_internal_status2(unsigned long id, unsigned int flag);
void bsp_isp_clr_internal_status2(unsigned long id, unsigned int flag);
void bsp_isp_set_saved_addr(unsigned long id, unsigned long addr);
void bsp_isp_set_statistics_addr(unsigned long id, dma_addr_t addr);
void bsp_isp_set_save_load_addr(unsigned long id, dma_addr_t addr);

/*******isp load register which we should write to ddr first*********/

unsigned int bsp_isp_load_update_flag(unsigned long id);
void bsp_isp_set_input_fmt(unsigned long id, unsigned int fmt);
void bsp_isp_set_byr_max_bit(unsigned long id, int bit);
void bsp_isp_set_byr_act_bit(unsigned long id, int bit);
void bsp_isp_set_last_blank_cycle(unsigned long id, unsigned int blank);
void bsp_isp_set_speed_mode(unsigned long id, unsigned int speed);
void bsp_isp_set_line_int_num(unsigned long id, unsigned int line_num);
void bsp_isp_debug_output_cfg(unsigned long id, int enable, int output_sel);
void bsp_isp_set_size(unsigned long id, struct isp_size_settings *size);
void bsp_isp_module0_enable(unsigned long id, unsigned int module_flag);
void bsp_isp_module0_disable(unsigned long id, unsigned int module_flag);
void bsp_isp_module_enable(unsigned long id, unsigned int module_flag);
void bsp_isp_module_disable(unsigned long id, unsigned int module_flag);
void bsp_isp_d3d_ref_frm_mode(unsigned long id, unsigned int d3d_ref);
void bsp_isp_set_d3d_bayer_addr(unsigned long id, dma_addr_t addr);
void bsp_isp_set_d3d_k0_addr(unsigned long id, dma_addr_t addr);
void bsp_isp_set_d3d_k1_addr(unsigned long id, dma_addr_t addr);
void bsp_isp_set_d3d_status_addr(unsigned long id, dma_addr_t addr);
void bsp_isp_set_d3d_lbc_cfg(unsigned long id, struct isp_lbc_cfg d3d_lbc);
void bsp_isp_set_ch_input_bit(unsigned long id, int ch, int bit);
void bsp_isp_set_ch_output_bit(unsigned long id, int ch, int bit);
void bsp_isp_clr_d3d_rec_en(unsigned long id);

/*******syscfg sram boot mode ctrl*********/
void bsp_isp_map_syscfg_addr(unsigned long id, unsigned long base);
void bsp_isp_sram_boot_mode_ctrl(unsigned long id, unsigned int mode);

#endif /*_ISP600_REG_CFG_H_*/

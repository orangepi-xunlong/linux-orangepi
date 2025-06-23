/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __ARMCB_ISP_SUBDEV_H__
#define __ARMCB_ISP_SUBDEV_H__

#include "armcb_v4l2_core.h"
#include "armcb_v4l_sd.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>

#define ISP_NEVENTS (32)

#define I5_INT_MASK_ADDR 0xd0
#define I5_INT_CLEAR_ADDR 0xd4
#define I5_INT_STATUS_ADDR 0xd8
#define I5_INT_ID_ADDR 0x68
#define I5_INT_FRMCNT_ADDR 0x0c

/*Sof signal interrupt mask*/
#define I5_INT_VSYNC_INT_MASK (0x4000000)
/*Software start of load mask*/
#define I5_INT_SW_SOL_MASK (0x1 << 17)
/*3a stats interrupt mask*/
#define I5_INT_3A_INT_MASK (0x20)
/*vout done interrupt mask*/
#define I5_INT_VOUTX_INT_MASK (0xD8)
#define I5_INT_VOUT0_DONE (0x1 << 3)
#define I5_INT_VOUT1_DONE (0x1 << 4)
#define I5_INT_VOUT2_DONE (0x1 << 6)
#define I5_INT_VOUT3_DONE (0x1 << 7)
/*vin interrupt mask*/
#define I5_INT_VIN_EXPX_INT_MASK (0x107)
/*isp error mask*/
#define I5_INT_IRQ_ERR_MASK (0x1F80000)
#define I5_IRQ_EVENT_POST_MASK \
	(I5_INT_VIN_EXPX_INT_MASK | I5_INT_VSYNC_INT_MASK | I5_INT_3A_INT_MASK)
/*afbc mask and status and clear register address*/
#define I5_INT_AFBC_MASK (0x1 << 6)
#define I5_INT_AFBC_DETAIL_ERR_MASK (0xC)
#define I5_INT_AFBC_STATUS_ADDR 0xa110
#define I5_INT_AFBC_CLEAR_ADDR 0xa108

#define I7_INT_AFBC_ERR_MASK (0x1 << 8)
#define I7_INT_AFBC_DETAIL_ERR_MASK (0xC)
#define I7_INT_AFBC_STATUS_ADDR 0x2358
#define I7_INT_AFBC_CLEAR_ADDR 0x2350

#define I7_INT_MASK_ADDR 0x64
#define I7_INT_CLEAR_ADDR 0x68
#define I7_INT_STATUS_ADDR 0x6c
#define I7_INT_ERR_MASK_ADDR 0x74
#define I7_INT_ERR_CLEAR_ADDR 0x78
#define I7_INT_ERR_STATUS_ADDR 0x7c
#define I7_INT_DETAIL_ERR_ADDR1 0xb0
#define I7_INT_DETAIL_ERR_ADDR2 0xb4
#define I7_INT_DETAIL_ERR_ADDR3 0xb8
#define I7_INT_DETAIL_ERR_ADDR4 0xbc
#define I7_INT_FRMCNT_SEL_ADDR 0x44
#define I7_INT_SENSOR_SEL_ADDR 0x50
#define I7_INT_SOL_FRMCNT_ADDR 0xa8
#define I7_INT_3A_FRMCNT_ADDR 0xac
#define I7_INT_ID_ADDR 0xc0
#define I7_INT_SOF_ID_ADDR 0x4c
/*vin_c8*/
#define I7_INT_SOF_FRMCNT_SEL_ADDR 0xb68
/*vin_e4*/
#define I7_INT_SOF_FRMCNT_ADDR 0xb84
/*top_00*/
#define I7_TOP_BASE_ADDR 0x00
/*vin_00*/
#define I7_VIN_BASE_ADDR 0xaa0
/*ifbc_00*/
#define I7_IFBC_BASE_ADDR 0x380
#define DAW0_IFBC_IDMA_REG_1C 0x39C
/*ifbd_00*/
#define I7_IFBD_BASE_ADDR 0x800
/*psc_00*/
#define I7_PSC_BASE_ADDR 0xdc0
/*csa_00*/
#define I7_SCA_BASE_ADDR 0x2028

/*I7 isp output buffer done interrupt mask*/
#define I7_INT_VOUT0_BUF_DONE (0x1 << 13)
#define I7_INT_VOUT1_BUF_DONE (0x1 << 14)
#define I7_INT_VOUT2_BUF_DONE (0x1 << 15)
#define I7_INT_VOUT3_BUF_DONE (0x1 << 16)
#define I7_INT_VOUT4_BUF_DONE (0x1 << 17)
#define I7_INT_VOUT5_BUF_DONE (0x1 << 18)
#define I7_INT_VOUT6_BUF_DONE (0x1 << 19)
#define I7_INT_VOUT7_BUF_DONE (0x1 << 20)
#define I7_INT_VOUT8_BUF_DONE (0x1 << 21)

/*AFBC interrupt*/
#define I7_INT_AFBC_MASK (0x1 << 6)
/*3a stats interrupt mask*/
#define I7_INT_3A_INT_MASK (0x1 << 12)
/*3a dump interrupt mask*/
#define I7_INT_DUMP_INT_MASK (0x1 << 22)
/*Sof signal interrupt mask*/
#define I7_INT_SOF_INT_MASK (0x1 << 24)
/*Line trigger interrupt mask*/
#define I7_INT_LINE_TRIG_MASK (0x1 << 25)
/*full page cmd load done*/
#define I7_INT_MCFB_LOAD_DONE (0x1 << 27)
/*Software start of load mask*/
#define I7_INT_SOL_MASK (0x1 << 31)
/*vout done interrupt mask*/
#define I7_INT_VOUTX_INT_MASK (0x3FE000)

/*I7 isp vin buffer done interrupt mask*/
#define I7_INT_VIN_CHNL_L_BUF_DONE (0x1 << 0)
#define I7_INT_VIN_CHNL_M_BUF_DONE (0x1 << 1)
#define I7_INT_VIN_CHNL_S_BUF_DONE (0x1 << 2)
#define I7_INT_VIN_CHNL_VS_BUF_DONE (0x1 << 3)

/*I7 isp nr info buffer done interrupt mask*/
#define I7_INT_MVD_MV_BUF_DONE (0x1 << 4)
#define I7_INT_MVD_SAD_BUF_DONE (0x1 << 5)
#define I7_INT_RTNR_IIR_BUF_DONE (0x1 << 7)
#define I7_INT_YTNR_IIRY_BUF_DONE (0x1 << 8)
#define I7_INT_YTNR_IIRC_BUF_DONE (0x1 << 9)
#define I7_INT_YTNR_MV_BUF_DONE (0x1 << 10)

/*vin interrupt mask*/
#define I7_INT_VIN_EXPX_INT_MASK ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3))
#define I7_INT_NR_INFO_INT_MASK \
	((1 << 4) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10))

/*post event mask*/
#define I7_IRQ_EVENT_POST_MASK              \
				(I7_INT_AFBC_MASK         |  \
				I7_INT_VIN_EXPX_INT_MASK |  \
				I7_INT_SOF_INT_MASK      |  \
				I7_INT_3A_INT_MASK       |  \
				I7_INT_SOL_MASK          |  \
				I7_INT_LINE_TRIG_MASK    |  \
				I7_INT_MCFB_LOAD_DONE    |  \
				I7_INT_VOUTX_INT_MASK    |  \
				I7_INT_NR_INFO_INT_MASK  |  \
				I7_INT_DUMP_INT_MASK)

#define ERR_DETAIL_NUM (5)
#define ISP_TOP_NUM (109)
#define ISP_VIN_NUM (60)
#define ISP_IFBC_NUM (12)
#define ISP_PSC_NUM (46)
#define ISP_SCA_NUM (6)

typedef enum {
	ISP_INVALID_INT = 0,
	ISP_NORMAL_INT = 1,
	ISP_ERROR_INT = 2,
	ISP_MIXTURE_INT = 3
} armcb_isp_int_type;

typedef enum __err_save_module {
	ISP_VIN = 0,
	ISP_TOP = 1,
	ISP_IFBC = 2,
	ISP_IFBD = 3,
	ISP_PSC = 4,
	ISP_SCA = 5,
	ISP_SAVE_MAX = 6
} err_save_module;

struct isp_irq_info {
	unsigned int changed;
	armcb_isp_int_type int_type;
	unsigned int mask;
	unsigned int status;
	unsigned int id;
	unsigned int sen_id; /// i7
	unsigned int frm_cnt_sof;
	unsigned int frm_cnt_sol;
	unsigned int frm_cnt_3a;
	unsigned int frm_cnt_nxt_sol; /// i7
	unsigned int err_mask; /// i7
	unsigned int err_detail_status[ERR_DETAIL_NUM];
};

struct armcb_isp_subdev {
	struct mutex imutex;
	spinlock_t sdlock;

	struct fwnode_handle *fwnode;
	struct platform_device *ppdev;

	struct v4l2_device v4l2_dev;
	struct armcb_sd_subdev isp_sd;

	struct v4l2_subdev_ops *armcb_isp_subdev_ops;
	unsigned int dev_caps;
	unsigned int id;

	int module_init_status;
	wait_queue_head_t state_wait;

	void __iomem *reg_base;

	void __iomem *reg_base2; /// only for gdc
};

unsigned int armcb_isp_read_reg(unsigned int offset);
unsigned int armcb_isp_read_reg2(unsigned int offset);
void armcb_isp_write_reg(unsigned int offset, unsigned int value);
void armcb_isp_write_reg2(unsigned int offset, unsigned int value);
#ifdef ARMCB_CAM_KO
void *armcb_get_isp_driver_instance(void);
void armcb_isp_driver_destroy(void);
#endif

#endif // __ARMCB_ISP_SUBDEV_H__

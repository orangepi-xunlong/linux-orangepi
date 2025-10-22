/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 *
 * Authors:  Zheng Zequn <zequnzheng@allwinnertech.com>
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

#ifndef __CSIC__TDM200__REG__I__H__
#define __CSIC__TDM200__REG__I__H__

/*
 * Detail information of registers
 */
/*
 * tdm top registers
 */
#define TMD_BASE_ADDR				0x05908000

#define TDM_GOLBAL_CFG0_REG_OFF			0X000
#define TDM_TOP_EN				0
#define TDM_TOP_EN_MASK				(0X1 << TDM_TOP_EN)
#define TDM_EN					1
#define TDM_EN_MASK				(0X1 << TDM_EN)
#define VGM_EN					3
#define VGM_EN_MASK				(0X1 << VGM_EN)
#define TDM_SPEED_DN_EN				4
#define TDM_SPEED_DN_EN_MASK			(0X1 << TDM_SPEED_DN_EN)
#define RX_CHN_CFG_MODE				12
#define RX_CHN_CFG_MODE_MASK			(0X3 << RX_CHN_CFG_MODE)
#define RX_WORK_MODE				14
#define RX_WORK_MODE_MASK			(0X1 << RX_WORK_MODE)
#define TX_CHN_CFG_MODE				16
#define TX_CHN_CFG_MODE_MASK			(0X7 << TX_CHN_CFG_MODE)
#define MODULE_CLK_BACK_DOOR			27
#define MODULE_CLK_BACK_DOOR_MASK		(0X1 << MODULE_CLK_BACK_DOOR)
#define RX_DATA_LINE_FRESH_EN			30
#define RX_DATA_LINE_FRESH_EN_MASK		(0X1 << RX_DATA_LINE_FRESH_EN)
#define TX_DATA_LINE_FRESH_EN			31
#define TX_DATA_LINE_FRESH_EN_MASK		(0X1 << TX_DATA_LINE_FRESH_EN)

#define TDM_INT_BYPASS0_REG_OFF			0X010

#define TDM_INT_STATUS0_REG_OFF			0X018
#define RX_FRM_LOST_PD				0
#define RX_FRM_LOST_PD_MASK			(0X1 << RX_FRM_LOST_PD)
#define RX_FRM_ERR_PD				1
#define RX_FRM_ERR_PD_MASK			(0X1 << RX_FRM_ERR_PD)
#define RX_BTYPE_ERR_PD				2
#define RX_BTYPE_ERR_PD_MASK			(0X1 << RX_BTYPE_ERR_PD)
#define RX_BUF_FULL_PD				3
#define RX_BUF_FULL_PD_MASK			(0X1 << RX_BUF_FULL_PD)
#define RX_COMP_ERR_PD				4
#define RX_COMP_ERR_PD_MASK			(0X1 << RX_COMP_ERR_PD)
#define RX_HB_SHORT_PD				5
#define RX_HB_SHORT_PD_MASK			(0X1 << RX_HB_SHORT_PD)
#define RX_FIFO_FULL_PD				6
#define RX_FIFO_FULL_PD_MASK			(0X1 << RX_FIFO_FULL_PD)
#define TDM_LBC_ERROR_PD			8
#define TDM_LBC_ERROR_PD_MASK			(0X1 << TDM_LBC_ERROR_PD)
#define TX_FIFO_UNDER_PD			9
#define TX_FIFO_UNDER_PD_MASK			(0X1 << TX_FIFO_UNDER_PD)
#define RX0_FRM_DONE_PD				16
#define RX0_FRM_DONE_PD_MASK			(0X1 << RX0_FRM_DONE_PD)
#define RX1_FRM_DONE_PD				17
#define RX1_FRM_DONE_PD_MASK			(0X1 << RX1_FRM_DONE_PD)
#define RX2_FRM_DONE_PD				18
#define RX2_FRM_DONE_PD_MASK			(0X1 << RX2_FRM_DONE_PD)
#define RX3_FRM_DONE_PD				19
#define RX3_FRM_DONE_PD_MASK			(0X1 << RX3_FRM_DONE_PD)
#define TX_FRM_DONE_PD				20
#define TX_FRM_DONE_PD_MASK			(0X1 << TX_FRM_DONE_PD)
#define SPEED_DN_FIFO_FULL_PD			22
#define SPEED_DN_FIFO_FULL_PD_MASK		(0X1 << SPEED_DN_FIFO_FULL_PD)
#define SPEED_DN_HSYN_PD			23
#define SPEED_DN_HSYN_PD_MASK			(0X1 << SPEED_DN_HSYN_PD)
#define RX_CHN_CFG_MODE_PD			24
#define RX_CHN_CFG_MODE_PD_MASK			(0X1 << RX_CHN_CFG_MODE_PD)
#define TX_CHN_CFG_MODE_PD			25
#define TX_CHN_CFG_MODE_PD_MASK			(0X1 << TX_CHN_CFG_MODE_PD)
#define TDM_LBC_FIFO_FULL_PD			26
#define TDM_LBC_FIFO_FULL_PD_MASK		(0X1 << TDM_LBC_FIFO_FULL_PD)

#define TDM_INTERNAL_STATUS0_REG_OFF		0X020
#define RX0_FRM_LOST_PD				(1 << 0)
#define RX1_FRM_LOST_PD				(1 << 1)
#define RX2_FRM_LOST_PD				(1 << 2)
#define RX3_FRM_LOST_PD				(1 << 3)
#define RX0_FRM_ERR_PD				(1 << 8)
#define RX1_FRM_ERR_PD				(1 << 9)
#define RX2_FRM_ERR_PD				(1 << 10)
#define RX3_FRM_ERR_PD				(1 << 11)
#define RX0_BTYPE_ERR_PD			(1 << 16)
#define RX1_BTYPE_ERR_PD			(1 << 17)
#define RX2_BTYPE_ERR_PD			(1 << 18)
#define RX3_BTYPE_ERR_PD			(1 << 19)
#define RX0_BUF_FULL_PD				(1 << 24)
#define RX1_BUF_FULL_PD				(1 << 25)
#define RX2_BUF_FULL_PD				(1 << 26)
#define RX3_BUF_FULL_PD				(1 << 27)

#define TDM_INTERNAL_STATUS1_REG_OFF		0X024
#define TX0_FIFO_UNDER				(1 << 0)
#define TX1_FIFO_UNDER				(1 << 1)
#define TX2_FIFO_UNDER				(1 << 2)
#define RX0_HB_SHORT_PD				(1 << 8)
#define RX1_HB_SHORT_PD				(1 << 9)
#define RX2_HB_SHORT_PD				(1 << 10)
#define RX3_HB_SHORT_PD				(1 << 11)
#define RX0_FIFO_FULL_PD			(1 << 16)
#define RX1_FIFO_FULL_PD			(1 << 17)
#define RX2_FIFO_FULL_PD			(1 << 18)
#define RX3_FIFO_FULL_PD			(1 << 19)
#define RX0_HEAD_FIFO_FULL_0			(1 << 20)
#define RX0_HEAD_FIFO_FULL_1			(1 << 21)
#define RX1_HEAD_FIFO_FULL_0			(1 << 22)
#define RX1_HEAD_FIFO_FULL_1			(1 << 23)
#define LBC0_ERROR				(1 << 24)
#define LBC1_ERROR				(1 << 25)

#define TDM_INTERNAL_STATUS2_REG_OFF		0X028
#define RX_BUF_FULL_ST				(0XF << 0)
#define RX_MBUS_STOP_ST				(0XF << 4)
#define TX_MBUS_STOP_ST				(1 << 8)
#define MBUS_STOP				(1 << 9)

/*
 * VGM registers
 */

#define TMD_VGM_OFFSET				0x060

#define TDM_VGM_CFG0_REG_OFF			0X000
#define VGM_DMODE				0
#define VGM_DMODE_MASK				(0X7 << VGM_DMODE)
#define VGM_SMODE				4
#define VGM_SMODE_MASK				(0X1 << VGM_SMODE)
#define VGM_START				5
#define VGM_START_MASK				(0X1 << VGM_START)
#define VGM_BCYCLE				6
#define VGM_BCYCLE_MASK				(0X1 << VGM_BCYCLE)

#define TDM_VGM_CFG1_REG_OFF			0X004
#define VGM_INPUT_FMT				0
#define VGM_INPUT_FMT_MASK			(0X7 << VGM_INPUT_FMT)
#define VGM_DATA_TYPE				4
#define VGM_DATA_TYPE_MASK			(0X3 << VGM_DATA_TYPE)

#define TDM_VGM_CFG2_REG_OFF			0X008
#define VGM_PARA0				0
#define VGM_PARA0_MASK				(0X7 << VGM_PARA0)
#define VGM_PARA1				4
#define VGM_PARA1_MASK				(0X3 << VGM_PARA1)

/*
 * tdm tx registers
 */
#define TMD_TX_OFFSET				0x0a0

#define TDM_TX_CFG0_REG_OFF			0X000
#define TDM_TX_EN				0
#define TDM_TX_EN_MASK				(0X1 << TDM_TX_EN)
#define TDM_TX_CAP_EN				1
#define TDM_TX_CAP_EN_MASK			(0X1 << TDM_TX_CAP_EN)
#define TDM_TX_FIFO_MODE				31
#define TDM_TX_FIFO_MODE_MASK			(0X1 << TDM_TX_FIFO_MODE)

#define TDM_TX_CFG1_REG_OFF			0X004
#define TDM_TX_H_BLANK				0
#define TDM_TX_H_BLANK_MASK			(0X3FFF << TDM_TX_H_BLANK)

#define TDM_TX_CFG2_REG_OFF			0X008
#define TDM_TX_V_BLANK_FE			0
#define TDM_TX_V_BLANK_FE_MASK			(0X3FFF << TDM_TX_V_BLANK_FE)
#define TDM_TX_V_BLANK_BE			16
#define TDM_TX_V_BLANK_BE_MASK			(0X3FFF << TDM_TX_V_BLANK_BE)

#define TDM_TX_TIME1_CYCLE_OFF			0X00C
#define TDM_TX_T1_CYCLE				0
#define TDM_TX_T1_CYCLE_MASK			(0XFFFFFFFF << TDM_TX_T1_CYCLE)

#define TDM_TX_TIME2_CYCLE_OFF			0X010
#define TDM_TX_T2_CYCLE				0
#define TDM_TX_T2_CYCLE_MASK			(0XFFFFFFFF << TDM_TX_T2_CYCLE)

#define TDM_TX_FIFO_DEPTH_OFF			0X014
#define TDM_TX_HEAD_FIFO			0
#define TDM_TX_HEAD_FIFO_MASK			(0XFFFF << TDM_TX_HEAD_FIFO)
#define TDM_TX_DATA_FIFO			16
#define TDM_TX_DATA_FIFO_MASK			(0XFFFF << TDM_TX_DATA_FIFO)

#define TDM_TX_DATA_RATE_REG_OFF		0X018
#define TDM_TX_INVALID_NUM			0
#define TDM_TX_INVALID_NUM_MASK			(0XFF << TDM_TX_INVALID_NUM)
#define TDM_TX_VALID_NUM			16
#define TDM_TX_VALID_NUM_MASK			(0XFF << TDM_TX_VALID_NUM)

#define TDM_TX_CTRL_ST_REG_OFF			0X020
#define TX_CTRL_ST				0
#define TX_CTRL_ST_MASK				(0XFF << TX_CTRL_ST)

#define TDM_TX_FIFO_INFO0_REG_OFF		0X028
#define TX_DATA_FIFO_LAYER			0
#define TX_DATA_FIFO_LAYER_MASK			(0XFFF << TX_DATA_FIFO_LAYER)
#define TX_HEAD_FIFO_LAYER			12
#define TX_HEAD_FIFO_LAYER_MASK			(0XFFF << TX_HEAD_FIFO_LAYER)
#define TX_DATA_CMD_FINISH			24
#define TX_DATA_CMD_FINISH_MASK			(0x1 << TX_DATA_CMD_FINISH)
#define TX_HEAD_CMD_FINISH			25
#define TX_HEAD_CMD_FINISH_MASK			(0x1 << TX_HEAD_CMD_FINISH)
#define TX_DATA_MBUS_IDLE			26
#define TX_DATA_MBUS_IDLE_MASK			(0x1 << TX_DATA_MBUS_IDLE)
#define TX_HEAD_MBUS_IDLE			27
#define TX_HEAD_MBUS_IDLE_MASK			(0x1 << TX_HEAD_MBUS_IDLE)

#define TDM_TX_FIFO_INFO1_REG_OFF		0X02C
#define TX_DATA_RDMA_LINE			0
#define TX_DATA_RDMA_LINE_MASK			(0X3FFF << TX_DATA_RDMA_LINE)
#define TX_HEAD_RDMA_LINE			16
#define TX_HEAD_RDMA_LINE_MASK			(0X3FFF << TX_HEAD_RDMA_LINE)

#define TDM_TX_ID0_FIFO_DEPTH_OFF		0X040
#define TDM_TX_ID0_HEAD_FIFO			0
#define TDM_TX_ID0_HEAD_FIFO_MASK			(0XFF << TDM_TX_ID0_HEAD_FIFO)
#define TDM_TX_ID0_DATA_FIFO			16
#define TDM_TX_ID0_DATA_FIFO_MASK			(0XFFF << TDM_TX_ID0_DATA_FIFO)

#define TDM_TX_ID1_FIFO_DEPTH_OFF		0X044
#define TDM_TX_ID1_HEAD_FIFO			0
#define TDM_TX_ID1_HEAD_FIFO_MASK			(0XFF << TDM_TX_ID1_HEAD_FIFO)
#define TDM_TX_ID1_DATA_FIFO			16
#define TDM_TX_ID1_DATA_FIFO_MASK			(0XFFF << TDM_TX_ID1_DATA_FIFO)

#define TDM_TX_ID2_FIFO_DEPTH_OFF		0X048
#define TDM_TX_ID2_HEAD_FIFO			0
#define TDM_TX_ID2_HEAD_FIFO_MASK			(0XFF << TDM_TX_ID2_HEAD_FIFO)
#define TDM_TX_ID2_DATA_FIFO			16
#define TDM_TX_ID2_DATA_FIFO_MASK			(0XFFF << TDM_TX_ID2_DATA_FIFO)

#define TDM_TX_ID3_FIFO_DEPTH_OFF		0X04C
#define TDM_TX_ID3_HEAD_FIFO			0
#define TDM_TX_ID3_HEAD_FIFO_MASK			(0XFF << TDM_TX_ID3_HEAD_FIFO)
#define TDM_TX_ID3_DATA_FIFO			16
#define TDM_TX_ID3_DATA_FIFO_MASK			(0XFFF << TDM_TX_ID3_DATA_FIFO)


/*
 * tdm rx registers
 */
#define TMD_RX0_OFFSET				0x100
#define TMD_RX1_OFFSET				0x180
#define TMD_RX2_OFFSET				0x200
#define TMD_RX3_OFFSET				0x280

#define AMONG_RX_OFFSET				0x80

#define TDM_RX_CFG0_REG_OFF			0X000
#define TDM_RX_EN				0
#define TDM_RX_EN_MASK				(0X1 << TDM_RX_EN)
#define TDM_RX_CAP_EN				1
#define TDM_RX_CAP_EN_MASK			(0X1 << TDM_RX_CAP_EN)
#define TDM_RX_ABD_EN				2
#define TDM_RX_ABD_EN_MASK			(0X1 << TDM_RX_ABD_EN)
#define TDM_RX_TX_EN				3
#define TDM_RX_TX_EN_MASK			(0X1 << TDM_RX_TX_EN)
#define TDM_RX_LBC_EN				4
#define TDM_RX_LBC_EN_MASK			(0X1 << TDM_RX_LBC_EN)
#define TDM_RX_PKG_EN				5
#define TDM_RX_PKG_EN_MASK			(0X1 << TDM_RX_PKG_EN)
#define TDM_RX_SYN_EN				6
#define TDM_RX_SYN_EN_MASK			(0X1 << TDM_RX_SYN_EN)
#define TDM_LINE_NUM_DDR_EN			7
#define TDM_LINE_NUM_DDR_EN_MASK		(0X1 << TDM_LINE_NUM_DDR_EN)
#define TDM_RX_BUF_NUM				8
#define TDM_RX_BUF_NUM_MASK			(0XF << TDM_RX_BUF_NUM)
#define TDM_RX_MIN_DDR_SIZE			16
#define TDM_RX_MIN_DDR_SIZE_MASK		(0X3 << TDM_RX_MIN_DDR_SIZE)
#define TDM_RX_INPUT_FMT			24
#define TDM_RX_INPUT_FMT_MASK			(0X7 << TDM_RX_INPUT_FMT)
#define TDM_INPUT_BIT				28
#define TDM_INPUT_BIT_MASK			(0X7 << TDM_INPUT_BIT)

#define TDM_RX_CFG1_REG_OFF			0X004
#define TDM_RX_WIDTH				0
#define TDM_RX_WIDTH_MASK			(0X3FFF << TDM_RX_WIDTH)
#define TDM_RX_HEIGHT				16
#define TDM_RX_HEIGHT_MASK			(0X3FFF << TDM_RX_HEIGHT)

#define TDM_RX_FIFO_DEPTH_REG_OFF		0X008
#define TDM_RX_PKG_LINE_WORDS			0x0
#define TDM_RX_PKG_LINE_WORDS_MASK		(0xFFF << TDM_RX_PKG_LINE_WORDS)
#define TDM_RX_HEAD_FIFO_DEPTH			12
#define TDM_RX_HEAD_FIFO_DEPTH_MASK		(0xFF << TDM_RX_HEAD_FIFO_DEPTH)
#define TDM_RX_DATA_FIFO_DEPTH			20
#define TDM_RX_DATA_FIFO_DEPTH_MASK		(0xFFF << TDM_RX_DATA_FIFO_DEPTH)

#define TDM_RX_CFG2_REG_OFF			0X010
#define TDM_RX_ADDR				0
#define TDM_RX_ADDR_MASK			(0XFFFFFFFF << TDM_RX_ADDR)

#define TDM_RX_FRAME_ERR_REG_OFF		0X020
#define TDM_RX_HB_SHORT_REG_OFF			0X024
#define TDM_RX_LAYER_REG_OFF			0X028
#define TDM_RX_PROC_NUM_REG_OFF			0X02c
#define TDM_RX_STATUS_REG_OFF			0X034

#define TMD_LBC0_OFFSET				0x300
#define TMD_LBC1_OFFSET				0x320
#define AMONG_LBC_OFFSET			0x20

#define TMD_LBC_CFG0_REG_OFF			0x000
#define IS_LOSSY				0x0
#define IS_LOSSY_MASK				(0x1 << IS_LOSSY)
#define STATUS_QP				4
#define STATUS_QP_MASK				(0x3 << STATUS_QP)
#define MB_TH					8
#define MB_TH_MASK				(0x3F << MB_TH)
#define MB_NUM					16
#define MB_NUM_MASK				(0x7F << MB_NUM)

#define TMD_LBC_CFG1_REG_OFF			0x004
#define MB_MIN_BIT				0x0
#define MB_MIN_BIT_MASK				(0x1FF << MB_MIN_BIT)
#define GBGR_RATIO				16
#define GBGR_RATIO_MASK				(0x3FF << GBGR_RATIO)

#define TMD_LBC_CFG2_REG_OFF			0x008
#define LINE_TAR_BIT				0x0
#define LINE_TAR_BIT_MASK			(0xFFFF << LINE_TAR_BIT)
#define LINE_MAX_BIT				16
#define LINE_MAX_BIT_MASK			(0xFFFF << LINE_MAX_BIT)

#define TMD_LBC_STATUS_REG_OFF			0x010
#define LBC_OVER				0
#define LBC_OVER_MASK				(0x1 << LBC_OVER)
#define LBC_SHORT				1
#define LBC_SHORT_MASK				(0x1 << LBC_SHORT)
#define LBC_QP					2
#define LBC_QP_MASK				(0x1 << LBC_QP)

#endif /* __CSIC__TDM__REG__I__H__ */

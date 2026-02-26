/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
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

#ifndef _VIN_RP_H_
#define _VIN_RP_H_

#include <linux/rpbuf.h>
#include <linux/rpmsg.h>

////////////////////////isp rp//////////////////////////////
/* need set the same with melis */
#define ISP0_RPBUF_LOAD_NAME "isp0_load_rpbuf"
#define ISP0_RPBUF_SAVE_NAME "isp0_save_rpbuf"
#define ISP1_RPBUF_LOAD_NAME "isp1_load_rpbuf"
#define ISP1_RPBUF_SAVE_NAME "isp1_save_rpbuf"
#define ISP2_RPBUF_LOAD_NAME "isp2_load_rpbuf"
#define ISP2_RPBUF_SAVE_NAME "isp2_save_rpbuf"
#define ISP_RPBUF_LOAD_LEN ISP_LOAD_DRAM_SIZE
#define ISP_RPBUF_SAVE_LEN (ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_STATISTIC_SIZE)

#define ISP0_RPBUF_LDCI_NAME "isp0_ldci_rpbuf"
#define ISP1_RPBUF_LDCI_NAME "isp1_ldci_rpbuf"
#define ISP2_RPBUF_LDCI_NAME "isp2_ldci_rpbuf"
#define ISP_RPBUF_LDCI_LEN (160 * 90)

#define ISP0_RPMSG_NAME "sunxi,isp0_rpmsg"
#define ISP1_RPMSG_NAME "sunxi,isp1_rpmsg"
#define ISP2_RPMSG_NAME "sunxi,isp2_rpmsg"

struct isp_dev;

enum rpmsg_cmd {
	ISP_REQUEST_SENSOR_INFO = 0,
	ISP_SET_SENSOR_EXP_GAIN,
	ISP_SET_STAT_EN,
	ISP_SET_SAVE_AE,
	ISP_SET_ENCPP_DATA,
	ISP_SET_IR_STATUS,
	ISP_SET_ATTR_IOCTL,
	ISP_REQUEST_SENSOR_STATE,
	ISP_SET_GTM_TPYE,
	ISP_SET_SYS_RESET,
	ISP_SYNC_INFO_TO_VIN,

	VIN_SET_SENSOR_INFO,
	VIN_SET_FRAME_SYNC,
	VIN_SET_V4L2_IOCTL,
	VIN_SET_CLOSE_ISP,
	VIN_SET_ISP_RESET,
	VIN_SET_ISP_START,
	VIN_SET_ATTR_IOCTL,
	VIN_REQUEST_ATTR_IOCTL,
	VIN_SET_SERVER_RESET,
	VIN_SET_SENSOR_STATE,
	VIN_SYNC_ISP_INFO,
};
//////////////////////////end////////////////////////////

int vin_isp_get_hist(struct isp_dev *isp, unsigned int *hist_buf);
int isp_save_rpbuf_send(struct isp_dev *isp, void *buf);
int isp_ldci_rpbuf_send(struct isp_dev *isp, void *vir_buf);
int isp_rpbuf_create(struct isp_dev *isp);
int isp_rpbuf_destroy(struct isp_dev *isp);

int isp_rpmsg_send(struct isp_dev *isp, void *data, int len);
int isp_rpmsg_trysend(struct isp_dev *isp, void *data, int len);

int sunxi_vin_isp_parameter_init(void);
void sunxi_vin_isp_parameter_exit(void);
#endif


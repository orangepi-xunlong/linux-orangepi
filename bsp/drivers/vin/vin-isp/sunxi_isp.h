/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#ifndef _SUNXI_ISP_H_
#define _SUNXI_ISP_H_
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include "../vin-video/vin_core.h"
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
#include "isp520/isp520_reg_cfg.h"
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
#include "isp521/isp521_reg_cfg.h"
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW10)
#include "isp522/isp522_reg_cfg.h"
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#include "isp600/isp600_reg_cfg.h"
#else
#include "isp500/isp500_reg_cfg.h"
#endif
#include "../vin-stat/vin_h3a.h"

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
#include <linux/memblock.h>
#endif
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
#include <linux/rpbuf.h>
#include <linux/aw_rpmsg.h>
#include "../vin-rp/vin_rp.h"
#include "isp_comm.h"
#endif

enum isp_pad {
	ISP_PAD_SINK,
	ISP_PAD_SOURCE_ST,
	ISP_PAD_SOURCE,
	ISP_PAD_NUM,
};

enum encpp_ctrl_id {
	/*encpp_ctrl*/
	ISP_CTRL_ENCPP_EN = 0,
	ISP_CTRL_ENCPP_STATIC_CFG,
	ISP_CTRL_ENCPP_DYNAMIC_CFG,
	ISP_CTRL_ENCODER_3DNR_CFG,
	ISP_CTRL_ENCODER_2DNR_CFG,
};

enum isp_work_mode {
	ISP_ONLINE = 0,
	ISP_OFFLINE = 1,
};

enum isp_sram_boot_mode {
	SRAM_NORMAL_MODE = 0,
	SRAM_BOOT_MODE = 1,
};

enum isp_bit_width {
	RAW_8     = 8,
	RAW_10    = 10,
	RAW_12    = 12,
	RAW_14    = 14,
	RAW_16    = 16,
	RAW_18    = 18,
	RAW_20    = 20,
	RAW_22    = 22,
	RAW_24    = 24,
};

enum isp_byr_max_bit {
	BYR_RAW_14 = 1,
	BYR_RAW_16 = 2,
	BYR_RAW_20 = 4,
};

struct isp_pix_fmt {
	u32 mbus_code;
	enum isp_input_seq infmt;
	u32 fourcc;
	enum isp_bit_width input_bit;
	enum isp_byr_max_bit byr_max_bit;
};

struct sunxi_isp_ctrls {
	struct v4l2_ctrl_handler handler;

	struct v4l2_ctrl *wb_gain[4];	/* wb gain cluster */
	struct v4l2_ctrl *ae_win[4];	/* wb win cluster */
	struct v4l2_ctrl *af_win[4];	/* af win cluster */
};

struct isp_dev {
	struct v4l2_subdev subdev;
	struct media_pad isp_pads[ISP_PAD_NUM];
	struct v4l2_event event;
	struct platform_device *pdev;
	struct sunxi_isp_ctrls ctrls;
	struct mutex subdev_lock;
	struct vin_mm isp_stat;
	struct vin_mm isp_load;
	struct vin_mm isp_save;
	struct vin_mm isp_lut_tbl;
	struct vin_mm isp_drc_tbl;
#if !defined ISP_600
	struct vin_mm d3d_pingpong[3];
	struct vin_mm wdr_pingpong[2];
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	struct isp_lbc_cfg wdr_raw_lbc;
	struct isp_lbc_cfg d3d_k_lbc;
	struct isp_lbc_cfg d3d_raw_lbc;
#endif
#else
	struct vin_mm d3d_pingpong[4];
	struct isp_lbc_cfg d3d_lbc;
	struct vin_mm isp_save_load;
	struct vin_mm load_para[2];
	void __iomem *syscfg_base;
	unsigned int logic_top_stream_count;
	unsigned int work_mode;
	unsigned int delay_init;
	char save_get_flag;
	bool load_select; /*load_select = 0 select load_para[0], load_select = 1 select load_para[1]*/
	bool d3d_rec_reset;
	unsigned int d3d_lbc_ratio;
#endif
	struct isp_size err_size;
	struct isp_size save_size;
	struct isp_debug_mode isp_dbg;
	struct isp_pix_fmt *isp_fmt;
	struct isp_size_settings isp_ob;
	struct v4l2_mbus_framefmt mf;
	struct isp_stat h3a_stat;
	spinlock_t slock;
	void __iomem *base;
	struct work_struct s_sensor_stby_task;
	int irq;
	unsigned int ptn_isp_cnt;
	unsigned int event_lost_cnt;
	unsigned int hb_max;
	unsigned int hb_min;
	unsigned int isp_frame_number;
	unsigned int stream_count;
	unsigned char id;
	char is_empty;
	char use_isp;
	char runtime_flag;
	bool nosend_ispoff;
	char have_init;
	char load_flag;
	char load_save;
	char f1_after_librun;/* fisrt frame after server run */
	char left_right;/* 0: process left, 1: process right */
	char use_cnt;
	char capture_mode;
	char wdr_mode;
	char sensor_lp_mode;
	char ptn_type;
	struct vin_ptn_cfg *ptn_cfg;
	char large_image;/* 2:get merge yuv, 1: get pattern raw (save in kernel), 0: normal */
#ifdef SUPPORT_PTN
	char load_shadow[ISP_LOAD_DRAM_SIZE*3];
#else
	char load_shadow[ISP_LOAD_DRAM_SIZE];
#endif
#if defined ISP_600
	char nr_msc_load_save[ISP_MSC_NR_TBL_SIZE];
	char ae_stat_save[ISP_AE_REG_SIZE];
	char af_stat_save[ISP_AF_REG_SIZE];
	char awb_stat_save[ISP_AWB_REG_SIZE];
	char hist_stat_save[ISP_HIST_REG_SIZE];
	char init_done_flag;
#endif
	bool first_init_server;
	bool isp_server_reset;
	struct device_node *isp_reserved_np;
	struct resource reserved_r;
	int reserved_len;
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	struct rpbuf_controller *controller;
	struct rpbuf_buffer *load_buffer;
	struct rpbuf_buffer *save_buffer;
	struct rpbuf_buffer *ldci_buffer;
	char gtm_type;

	struct rpmsg_device *rpmsg;
	struct work_struct isp_rpmsg_send_task;

	int encpp_en;
	struct encpp_static_sharp_config encpp_static_sharp_cfg;
	struct encpp_dynamic_sharp_config encpp_dynamic_sharp_cfg;
	struct encoder_3dnr_config encoder_3dnr_cfg;
	struct encoder_2dnr_config encoder_2dnr_cfg;
	struct isp_cfg_attr_data isp_cfg_attr;
#endif
};

void sunxi_isp_reset(struct isp_dev *isp);
int sunxi_isp_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms);
int sunxi_isp_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms);
void sunxi_isp_sensor_type(struct v4l2_subdev *sd, int use_isp);
void sunxi_isp_sensor_fps(struct v4l2_subdev *sd, int fps);
void sunxi_isp_debug(struct v4l2_subdev *sd, struct isp_debug_mode *isp_debug);
void sunxi_isp_ptn(struct v4l2_subdev *sd, unsigned int ptn_type);
void sunxi_isp_frame_sync_isr(struct v4l2_subdev *sd);
struct v4l2_subdev *sunxi_isp_get_subdev(int id);
struct v4l2_subdev *sunxi_stat_get_subdev(int id);
int sunxi_isp_platform_register(void);
void sunxi_isp_platform_unregister(void);

#endif /* _SUNXI_ISP_H_ */

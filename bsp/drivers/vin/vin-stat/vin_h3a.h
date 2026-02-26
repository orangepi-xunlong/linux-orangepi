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

#ifndef _VIN_3A_H_
#define _VIN_3A_H_

#include <linux/types.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>

#include "../vin-video/vin_video.h"

#if !defined CONFIG_ISP_SERVER_MELIS
#define STAT_MAX_BUFS		6
#define STAT_NEVENTS		8

#define STAT_BUF_DONE		0	/* Buffer is ready */
#define STAT_NO_BUF		1	/* An error has occurred */

enum ispstat_buf_state_t {
	ISPSTAT_IDLE = 0,
	ISPSTAT_LOAD_SET,
	ISPSTAT_SAVE_SET,
	ISPSTAT_READY,
};

struct ispstat_buffer {
	void *virt_addr;
	void *dma_addr;
	u32 buf_size;
	u32 frame_number;
	u8 empty;
	enum ispstat_buf_state_t state;
};

enum ispstat_state_t {
	ISPSTAT_DISABLED = 0,
	ISPSTAT_ENABLED,
};

struct isp_dev;

struct isp_stat {
	struct v4l2_subdev sd;
	struct media_pad pad;	/* sink pad */

	/* Control */
	bool configured;
	enum ispstat_state_t state;	/* enabling/disabling state */
	struct isp_dev *isp;
	struct mutex ioctl_lock; /* serialize private ioctl */

	/* Buffer */
	u8 stat_en_flag;
	u8 sensor_fps;
	u8 buf_cnt;
	u32 frame_number;
	u32 buf_size;
	u32 event_type;
	struct vin_mm ion_man[STAT_MAX_BUFS]; /* for ion alloc/free manage */
	struct ispstat_buffer buf[STAT_MAX_BUFS];
	struct ispstat_buffer *active_buf;
	struct ispstat_buffer *locked_buf;
};

void isp_stat_load_set(struct isp_stat *stat);
#else /* CONFIG_ISP_SERVER_MELIS */
#include "../vin-rp/vin_rp.h"

#define STAT_MAX_BUFS		3

#define STAT_BUF_DONE		0	/* Buffer is ready */
#define STAT_NO_BUF		1	/* An error has occurred */

enum ispstat_buf_state_t {
	ISPSTAT_IDLE = 0,
	ISPSTAT_LOAD_SET,
	ISPSTAT_SAVE_SET,
	ISPSTAT_READY,
};

struct ispstat_buffer {
	void *virt_addr;
	void *dma_addr;
	u32 buf_size;
	u32 frame_number;
	u8 empty;
	enum ispstat_buf_state_t state;
};

enum ispstat_state_t {
	ISPSTAT_DISABLED = 0,
	ISPSTAT_ENABLED,
};

struct isp_dev;

struct isp_stat {
	struct isp_dev *isp;

	/* Buffer */
	enum ispstat_state_t state;	/* enabling/disabling state */
	struct mutex ioctl_lock; /* serialize private ioctl */
	bool stat_en_flag;
	u8 sensor_fps;
	u8 buf_cnt;
	u32 frame_number;
	u32 buf_size;
	struct vin_mm ion_man[STAT_MAX_BUFS]; /* for ion alloc/free manage */
	struct ispstat_buffer buf[STAT_MAX_BUFS];
	struct ispstat_buffer *active_buf;
	struct ispstat_buffer *locked_buf;
};
int isp_stat_enable(struct isp_stat *stat, u8 enable);
int isp_stat_request_statistics(struct isp_stat *stat);

int isp_config_sensor_info(struct isp_dev *isp);
int isp_reset_config_sensor_info(struct isp_dev *isp, enum rpmsg_cmd cmd);
void isp_sensor_set_exp_gain(struct isp_dev *isp, void *data);
void isp_set_encpp_cfg(struct isp_dev *isp, void *data);
void isp_set_ir_cfg(struct isp_dev *isp, void *data);
void isp_update_isp_attr_cfg(struct isp_dev *isp, void *data);
void isp_save_ae(struct isp_dev *isp, void *data);
void isp_get_sensor_state(struct isp_dev *isp);
int isp_write_nor_flash(loff_t to, loff_t to_offset, size_t len, const u_char *buf);
#endif
void isp_stat_isr(struct isp_stat *stat);
void isp_stat_load_set(struct isp_stat *stat);
int vin_isp_h3a_init(struct isp_dev *isp);
void vin_isp_h3a_cleanup(struct isp_dev *isp);

#endif /* _VIN_3A_H_ */

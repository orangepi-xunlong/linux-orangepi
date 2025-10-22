/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * Authors:  zhengzequn <zequnzheng@allwinnertech.com>
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

#ifndef __BSP__ISP__LDCI_VIDEO_H
#define __BSP__ISP__LDCI_VIDEO_H

#include <linux/videodev2.h>

struct vin_buffer;

/* special video api */
extern int vin_open_special(int id);
extern int vin_s_input_special(int id, int i);
extern int vin_s_parm_special(int id, void *priv, struct v4l2_streamparm *parms);
extern int vin_close_special(int id);
extern int vin_s_fmt_special(int id, struct v4l2_format *f);
extern int vin_g_fmt_special(int id, struct v4l2_format *f);
extern int vin_dqbuffer_special(int id, struct vin_buffer **buf);
extern int vin_qbuffer_special(int id, struct vin_buffer *buf);
extern int vin_streamon_special(int id, enum v4l2_buf_type i);
extern int vin_streamoff_special(int id, enum v4l2_buf_type i);
extern void vin_register_buffer_done_callback(int id, void *func);
extern struct device *vin_get_dev(int id);

/* ldci video api */
int enable_ldci_video(int ldci_video_in_chn);
void disable_ldci_video(int ldci_video_in_chn);
void isp_ldci_send_handle(struct work_struct *work);
int check_ldci_video_relate(int video_id, int ldci_video_id);

#endif

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

#include "../../vin-video/vin_video.h"
#include "../../vin-isp/sunxi_isp.h"
#include "isp_ldci_video.h"

#define LDCI_VIDEO_WIDTH 160
#define LDCI_VIDEO_HEIGHT 90
#define LDCI_VIDEO_FPS 20
#define LDCI_VIDEO_BUF_CNT 3
#define LDCI_VIDEO_ISP_MODE 0
#define LDCI_VIDEO_FMT V4L2_PIX_FMT_GREY

#define LDCI_MAX_CH 4

#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

struct ldci_video_cfg {
	struct vin_mm ldci_vin_buf[LDCI_VIDEO_BUF_CNT];
	struct vin_buffer vin_buf[LDCI_VIDEO_BUF_CNT];
	bool ldci_video_init;
};

static struct ldci_video_cfg ldci_cfg_gbl[LDCI_MAX_CH];

extern struct vin_core *vin_core_gbl[VIN_MAX_DEV];

static int debug;
#define ldci_dbg(level, fmt, arg...) \
	do { \
		if (debug >= level) \
			pr_info("ldci_video: " fmt, ## arg); \
	} while (0)

static int alloc_ldci_video_buf(int ldci_video_in_chn)
{
	struct ldci_video_cfg *ldci_cfg = &ldci_cfg_gbl[ldci_video_in_chn%4];
	struct device *dev;
	int ret = 0;
	int i = 0;

	dev = vin_get_dev(ldci_video_in_chn);

	for (i = 0; i < LDCI_VIDEO_BUF_CNT; i++) {
		if (LDCI_VIDEO_FMT == V4L2_PIX_FMT_GREY)
			ldci_cfg->ldci_vin_buf[i].size = LDCI_VIDEO_WIDTH * LDCI_VIDEO_HEIGHT;
		else
			ldci_cfg->ldci_vin_buf[i].size = LDCI_VIDEO_WIDTH * LDCI_VIDEO_HEIGHT * 3 / 2;
		ret = os_mem_alloc(dev, &ldci_cfg->ldci_vin_buf[i]);
		if (ret < 0) {
			ldci_dbg(0, "vin ldci buf requset failed!\n");
			return -ENOMEM;
		}

		ldci_cfg->vin_buf[i].paddr = ldci_cfg->ldci_vin_buf[i].dma_addr;
		ldci_cfg->vin_buf[i].vir_addr = ldci_cfg->ldci_vin_buf[i].vir_addr;
		vin_qbuffer_special(ldci_video_in_chn, &ldci_cfg->vin_buf[i]);
	}

	ldci_dbg(1, "video%d buffer alloc!\n", ldci_video_in_chn);

	return 0;
}

static void free_ldci_video_buf(int ldci_video_in_chn)
{
	struct ldci_video_cfg *ldci_cfg = &ldci_cfg_gbl[ldci_video_in_chn%4];
	struct device *dev;
	int i = 0;

	dev = vin_get_dev(ldci_video_in_chn);

	for (i = 0; i < LDCI_VIDEO_BUF_CNT; i++) {
		os_mem_free(dev, &ldci_cfg->ldci_vin_buf[i]);
		ldci_cfg->vin_buf[i].paddr = NULL;
		ldci_cfg->vin_buf[i].vir_addr = NULL;
	}

	ldci_dbg(1, "video%d buffer free!\n", ldci_video_in_chn);
}

static void *ldci_video_get_frame(int ldci_video_in_chn, struct vin_buffer **buf)
{
	struct ldci_video_cfg *ldci_cfg = &ldci_cfg_gbl[ldci_video_in_chn%4];
	int id = ldci_video_in_chn;
	int ret;

	if (!ldci_cfg->ldci_video_init)
		return NULL;

	ret = vin_dqbuffer_special(id, buf);
	if (ret) {
		ldci_dbg(0, "vipp%d dqueue buffer error!\n", id);
		return NULL;
	}

	return (*buf)->vir_addr;
}

static void ldci_video_free_frame(int ldci_video_in_chn, struct vin_buffer *buf)
{
	struct ldci_video_cfg *ldci_cfg = &ldci_cfg_gbl[ldci_video_in_chn%4];
	int id = ldci_video_in_chn;
	int ret;

	if (!ldci_cfg->ldci_video_init)
		return;

	ret = vin_qbuffer_special(id, buf);
	if (ret) {
		ldci_dbg(0, "vipp%d queue buffer error!\n", id);
	}
}

static void ldci_video_callback(int id)
{
	struct vin_core *vinc = vin_core_gbl[id];

	schedule_work(&vinc->ldci_buf_send_task);
}

void isp_ldci_send_handle(struct work_struct *work)
{
	struct vin_core *vinc = container_of(work, struct vin_core, ldci_buf_send_task);
	struct isp_dev *isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
	struct vin_buffer *buf;
	void *vir_buf = NULL;

	if (vin_streaming(&vinc->vid_cap) == 0)
		return;

	vir_buf = ldci_video_get_frame(vinc->id, &buf);
	if (!vir_buf)
		return;

	isp_ldci_rpbuf_send(isp, vir_buf);

	ldci_video_free_frame(vinc->id, buf);
}

int enable_ldci_video(int ldci_video_in_chn)
{
	int ret = 0;
	struct v4l2_streamparm parms;
	struct v4l2_format fmt;
	struct ldci_video_cfg *ldci_cfg = &ldci_cfg_gbl[ldci_video_in_chn%4];
	int id = ldci_video_in_chn;

	if (ldci_cfg->ldci_video_init)
		return 0;

	ret = vin_open_special(id);
	if (ret) {
		ldci_dbg(0, "open vipp%d error!\n", id);
		return ret;
	}
	ldci_dbg(1, "open video%d", ldci_video_in_chn);

	ret = vin_s_input_special(id, 0);
	if (ret) {
		ldci_dbg(0, "vipp%d set input error!\n", id);
		return ret;
	}

	CLEAR(parms);
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = LDCI_VIDEO_FPS;
	parms.parm.capture.capturemode = V4L2_MODE_VIDEO;
	/*when different video have the same sensor source, 1:use sensor current win, 0:find the nearest win*/
	parms.parm.capture.reserved[0] = 1;  /////////////////////////////////////notice/////////////////////////////////////
	parms.parm.capture.reserved[1] = LDCI_VIDEO_ISP_MODE; /*2:command, 1: wdr, 0: normal*/
	ret = vin_s_parm_special(id, NULL, &parms);
	if (ret) {
		ldci_dbg(0, "vipp%d set parm error!\n", id);
		return ret;
	}

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width  = LDCI_VIDEO_WIDTH;
	fmt.fmt.pix_mp.height = LDCI_VIDEO_HEIGHT;
	fmt.fmt.pix_mp.pixelformat = LDCI_VIDEO_FMT;
	fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
	ret = vin_s_fmt_special(id, &fmt);
	if (ret) {
		ldci_dbg(0, "vipp%d set fmt error!\n", id);
		return ret;
	}

	ret = alloc_ldci_video_buf(id);
	if (ret) {
		return ret;
	}

	vin_register_buffer_done_callback(id, ldci_video_callback);

	ret = vin_streamon_special(id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	if (ret) {
		ldci_dbg(0, "vipp%d stream on error!\n", id);
		return ret;
	}

	ldci_cfg->ldci_video_init = true;

	return ret;
}

void disable_ldci_video(int ldci_video_in_chn)
{
	struct ldci_video_cfg *ldci_cfg = &ldci_cfg_gbl[ldci_video_in_chn%4];
	int id = ldci_video_in_chn;

	if (!ldci_cfg->ldci_video_init)
		return;

	ldci_cfg->ldci_video_init = false;

	vin_streamoff_special(id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	ldci_dbg(1, "close video%d", ldci_video_in_chn);
	vin_close_special(id);

	free_ldci_video_buf(id);
}

int check_ldci_video_relate(int video_id, int ldci_video_id)
{
	struct vin_core *vinc = vin_core_gbl[video_id];
	struct vin_core *ldic_vinc = vin_core_gbl[ldci_video_id];

	if (vinc && ldic_vinc) {
		if ((video_id != ldci_video_id) && (vinc->csi_sel == ldic_vinc->csi_sel))
			return 0;
		else
			return -1;
	}

	return -1;
}

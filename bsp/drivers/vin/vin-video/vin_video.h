/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
 * vin_video.h for video api
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 * Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _VIN_VIDEO_H_
#define _VIN_VIDEO_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-v4l2.h>
#include "../platform/platform_cfg.h"
#include "../vin-vipp/sunxi_scaler.h"
#if defined CSIC_DMA_VER_140_000
#include "dma140/dma140_reg.h"
#else
#include "dma130/dma_reg.h"
#endif

#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
#include "../vin-isp/isp_special_video/isp_ldci_video.h"
#define LDCI0_VIDEO_CHN 12
#define LDCI1_VIDEO_CHN 13
#define LDCI2_VIDEO_CHN 14
#endif

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
/* save isp parameter to flash */
/* kener user write it to sector, boot0 read it and write to ddr */
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define VIN_SENSOR0_RESERVE_ADDR 0x613FE000 /*104~110 sector, size is 4k - 512b, boot0 read it and write to 0x613FE000*/
#define VIN_SENSOR1_RESERVE_ADDR 0x613FF000 /*112~118 sector, size is 4k - 512b, boot0 read it and write to 0x613FF000*/
#define GET_RV_YUV 0
#endif
#define VIN_RESERVE_SIZE (0x1000 - 0x200) /* 4k - 512 */

#if defined GET_RV_YUV
#define YUV_MEMRESERVE 0x61400000
#define YUV_MEMRESERVE_SIZE 0x02000000
#endif

#define DAY_LIGNT_SENSOR_SIZE   96
#define NIGHT_LIGNT_SENSOR_SIZE 40

enum ir_mode {
	DAY_MODE = 0,
	AUTO_MODE,
	NIGHT_MODE,
};
#pragma pack(1)
typedef struct taglight_sensor_attr_s {
	unsigned short u16LightValue;	/*adc value*/
	unsigned short ae_table_idx;
	unsigned int   u32SnsExposure;
	unsigned int   u32SnsAgain;
} LIGHT_SENSOR_ATTR_S;

typedef struct tagsensor_isp_config_s {
	int sign;                    //id0: 0xAA66AA66, id1: 0xBB66BB66
	int crc;                     //checksum, crc32 check, if crc = 0, do check
	int ver;                     //version, 0x01

	int light_enable;            //boot0 adc en
	int adc_mode;                //ADC mode, 0:the brighter the u16LightValue more, 1:the brighter the u16LightValue smaller
	unsigned short light_def;    //adc threshold: 1,adc_mode=0:smaller than it enter night mode, greater than it or equal enter day mode;
				     //2,adc_mode=1:greater than it enter night mode, smaller than it or equal enter day mode;

	unsigned char ircut_state;   //1:hold, 0:use ir_mode
	unsigned short ir_mode;	     //IR mode 0:force day mode, 1:auto mode, 2:force night mode
	unsigned short lv_liner_def; //lv threshold: smaller than it enter night mode, greater than it or equal enter day mode
	unsigned short lv_hdr_def;   //lv threshold: smaller than it enter night mode, greater than it or equal enter day mode

	unsigned short width;        //init size, 0:mean maximum size
	unsigned short height;
	unsigned short mirror;
	unsigned short filp;
	unsigned short fps;
	unsigned short wdr_mode;     //hdr or normal mode, wdr_mode = 0 mean normal,  wdr_mode = 1 mean hdr
	unsigned char flicker_mode;  //0:disable,1:50HZ,2:60HZ, default 50HZ

	unsigned char venc_format;   //1:H264 2:H265
	/* unsigned char reserv 256 */
	unsigned char sensor_deinit; //sensor not init in melis
	unsigned char get_yuv_en; //get melis yuv en
	unsigned char light_sensor_en; //use LIGHT_SENSOR_ATTR_S en
	unsigned char hdr_light_sensor_ratio; //LIGHT_SENSOR_ATTR_S hdr exposure ratio, default is 16
	unsigned char lightadc_debug_en; //printf data about LIGHT_SENSOR_ATTR_S
	unsigned char pix_num; //two or one pix_num, default is one pix_num
	unsigned char tdm_speed_down_en; //tdm speed down en
	unsigned char reserv[256 - 7];

	LIGHT_SENSOR_ATTR_S linear[DAY_LIGNT_SENSOR_SIZE]; //day linear mode parameter, auto exp
	LIGHT_SENSOR_ATTR_S linear_night[NIGHT_LIGNT_SENSOR_SIZE]; //night linear mode parameter
	LIGHT_SENSOR_ATTR_S hdr[DAY_LIGNT_SENSOR_SIZE];    //day hdr mode paramete
	LIGHT_SENSOR_ATTR_S hdr_night[NIGHT_LIGNT_SENSOR_SIZE];    //night hdr mode parameter
} SENSOR_ISP_CONFIG_S; //size is 3563 < 3584(3.5k)
#pragma pack()

/* kenel auto save isp parameter to flash, in order to use in next time */
#define ISP0_NORFLASH_SAVE (VIN_SENSOR0_RESERVE_ADDR + 512 * 7) /*111 sector, size is 512b, kernel write to flash and boot0 read it and write to ISP0_NORFLASH_SAVE*/
#define ISP1_NORFLASH_SAVE (VIN_SENSOR1_RESERVE_ADDR + 512 * 7) /*119 sector, size is 512b, kernel write to flash and boot0 read it and write to ISP1_NORFLASH_SAVE*/
#define ISP0_THRESHOLD_PARAM_OFFSET 111  /*111 sector, size is 512b*/
#define ISP1_THRESHOLD_PARAM_OFFSET 119  /*119 sector, size is 512b*/
#define VIN_THRESHOLD_PARAM_SIZE (0x200) /* 512 */

#pragma pack(1)
struct isp_autoflash_config_s {
	//ISP_SET_SAVE_AE
	unsigned int ae_tble_idx;
	unsigned int ev_analog_gain;
	unsigned int ev_sensor_exp_line;
	unsigned int ev_short_analog_gain;
	unsigned int ev_short_sensor_exp_line;
	unsigned int reserv1[11];

	unsigned int melisyuv_sign_id;//id0: 0xAA11AA11, id1: 0xBB11BB11
	unsigned int melisyuv_paddr;
	unsigned int melisyuv_size;
	unsigned int not_frame_loss_flag;
	unsigned int reserv2[41];

	//sensor_list, melis identify sensor and then use this to notice kernel which sensor is use
	unsigned int sensorlist_sign_id;//id0: 0xAA22AA22, id1: 0xBB22BB22
	unsigned char sensor_name[20];
	unsigned int sensor_twi_addr;
	unsigned int sensor_detect_id;
	unsigned int reserv3[56];
 };
#pragma pack()

void *vin_map_kernel(unsigned long phys_addr, unsigned long size);
void vin_unmap_kernel(void *vaddr);
#endif

/* buffer for one video frame */
struct vin_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	void *paddr;
	enum vb2_buffer_state state;
	unsigned int first_qbuf;
	bool qbufed;
	void *vir_addr;
#if IS_ENABLED(CONFIG_VIDEO_SUNXI_VIN_SPECIAL)
	/* append for dmabuf mapping */
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int dmabuf_fd;
	dma_addr_t dma_address;
#endif
};

#define VIN_SD_PAD_SINK		0
#define VIN_SD_PAD_SOURCE	1
#define VIN_SD_PADS_NUM		2

enum bk_work_mode {
	BK_ONLINE = 0,
	BK_OFFLINE = 1,
};

enum vin_subdev_ind {
	VIN_IND_SENSOR,
	VIN_IND_MIPI,
	VIN_IND_CSI,
	VIN_IND_ISP,
	VIN_IND_SCALER,
	VIN_IND_CAPTURE,
	VIN_IND_TDM_RX,
	VIN_IND_ACTUATOR,
	VIN_IND_FLASH,
	VIN_IND_STAT,
	VIN_IND_MAX,
};

enum vin_state_flags {
	VIN_LPM,
	VIN_STREAM,
	VIN_BUSY,
	VIN_STATE_MAX,
};

#define vin_lpm(dev) test_bit(VIN_LPM, &(dev)->state)
#define vin_busy(dev) test_bit(VIN_BUSY, &(dev)->state)
#define vin_streaming(dev) test_bit(VIN_STREAM, &(dev)->state)
#define CEIL_EXP(a, b) (((a)>>(b)) + (((a)&((1<<(b))-1)) ? 1 : 0))

struct timeval {
	long		tv_sec;		/* seconds */
	long		tv_usec;	/* microseconds */
};
struct vin_pipeline {
	struct media_pipeline pipe;
	struct v4l2_subdev *sd[VIN_IND_MAX];
};

enum vin_fmt_flags {
	VIN_FMT_YUV = (1 << 0),
	VIN_FMT_RAW = (1 << 1),
	VIN_FMT_RGB = (1 << 2),
	VIN_FMT_OSD = (1 << 3),
	VIN_FMT_MAX,
	/* all possible flags raised */
	VIN_FMT_ALL = (((VIN_FMT_MAX - 1) << 1) - 1),
};


struct vin_fmt {
	u32	mbus_code;
	char	*name;
	u32	mbus_type;
	u32	fourcc;
	u16	memplanes;
	u16	colplanes;
	u8	depth[VIDEO_MAX_PLANES];
	u16	mdataplanes;
	u16	flags;
	enum v4l2_colorspace color;
	enum v4l2_field	field;
};

struct vin_addr {
	u32	y;
	u32	cb;
	u32	cr;
};

struct vin_frame {
	u32	o_width;
	u32	o_height;
	u32	offs_h;
	u32	offs_v;
	u32	width;
	u32	height;
	unsigned long		payload[VIDEO_MAX_PLANES];
	unsigned long		bytesperline[VIDEO_MAX_PLANES];
	struct vin_addr	paddr;
	struct vin_fmt	fmt;
};

/* osd settings */
struct vin_osd {
	u8 is_set;
	u8 ov_set_cnt;
	u8 overlay_en;
	u8 cover_en;
	u8 orl_en;
	u8 overlay_cnt;
	u8 cover_cnt;
	u8 orl_cnt;
	u8 inv_th;
	u8 inverse_close[MAX_OVERLAY_NUM + 1];
	u8 inv_w_rgn[MAX_OVERLAY_NUM + 1];
	u8 inv_h_rgn[MAX_OVERLAY_NUM + 1];
	u8 global_alpha[MAX_OVERLAY_NUM + 1];
	u8 y_bmp_avp[MAX_OVERLAY_NUM + 1];
	u8 yuv_cover[3][MAX_COVER_NUM + 1];
	u8 yuv_orl[3][MAX_ORL_NUM];
	u8 orl_width;
	int chromakey;
	enum vipp_osd_argb overlay_fmt;
	struct vin_mm ov_mask[2];	/* bitmap addr */
	struct v4l2_rect ov_win[MAX_OVERLAY_NUM + 1];	/* position */
	struct v4l2_rect cv_win[MAX_COVER_NUM + 1];	/* position */
	struct v4l2_rect orl_win[MAX_ORL_NUM];	/* position */
	int rgb_cover[MAX_COVER_NUM + 1];
	int rgb_orl[MAX_ORL_NUM];
	struct vin_fmt *fmt;
};

struct vin_vid_cap {
	struct video_device vdev;
	struct vin_frame frame;
	struct vin_osd osd;
	/* video capture */
	struct vb2_queue vb_vidq;
	struct device *dev;
	struct list_head vidq_active;
	struct list_head vidq_done;
	unsigned int isp_wdr_mode;
	unsigned int capture_mode;
	unsigned int buf_byte_size; /* including main and thumb buffer */
	/* working state */
	bool registered;
	bool special_active;
	bool dma_parms_alloc;
	struct mutex lock;
	unsigned int first_flag; /* indicate the first time triggering irq */
	struct timeval ts;
	spinlock_t slock;
	struct vin_pipeline pipe;
	struct vin_core *vinc;
	struct v4l2_subdev subdev;
	struct media_pad vd_pad;
	struct media_pad sd_pads[VIN_SD_PADS_NUM];
	bool user_subdev_api;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl *ae_win[4];	/* wb win cluster */
	struct v4l2_ctrl *af_win[4];	/* af win cluster */
	struct work_struct s_stream_task;
	struct work_struct pipeline_reset_task;
	unsigned long state;
	unsigned int frame_delay_cnt;
	struct dma_lbc_cmp lbc_cmp;
	struct dma_bufa_threshold threshold;
	void (*vin_buffer_process)(int id);
	void (*online_csi_reset_callback)(int id);
};

#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1)
static inline int vin_cmp(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

static inline void vin_swap(void *a, void *b, int size)
{
	int t = *(int *)a;
	*(int *)a = *(int *)b;
	*(int *)b = t;
}

static inline int vin_unique(int *a, int number)
{
	int i, k = 0;

	for (i = 1; i < number; i++) {
		if (a[k] != a[i]) {
			k++;
			a[k] = a[i];
		}
	}
	return k + 1;
}
#endif
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr);
int vin_timer_init(struct vin_core *vinc);
void vin_timer_del(struct vin_core *vinc);
void vin_timer_update(struct vin_core *vinc, int ms);
int sensor_flip_option(struct vin_vid_cap *cap, struct v4l2_control c);
void vin_set_next_buf_addr(struct vin_core *vinc);
void vin_get_rest_buf_cnt(struct vin_core *vinc);
int vin_initialize_capture_subdev(struct vin_core *vinc);
void vin_cleanup_capture_subdev(struct vin_core *vinc);

#endif /* _VIN_VIDEO_H_ */

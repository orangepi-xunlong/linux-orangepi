
/*
 ******************************************************************************
 *
 * bsp_isp_algo.h
 *
 * Hawkview ISP - bsp_isp_algo.h module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/10/08	    First Version
 *
 ******************************************************************************
 */

#ifndef __BSP__ISP__ALGO__H
#define __BSP__ISP__ALGO__H

#include "isp_module_cfg.h"

#define  MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define  MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define  SQUARE(x)	((x) * (x))
#define  CLIP(a, i, s)	(((a) > (s)) ? (s) : MAX(a, i))
#define ISP_CLEAR(x)	(memset(&(x), 0, sizeof (x)))


enum isp_stat_buf_status {
	BUF_IDLE = 0,
	BUF_ACTIVE = 1,
	BUF_LOCKED = 2,
};
struct isp_stat_buffer {
	unsigned int buf_size;
	unsigned int frame_number;
	unsigned int cur_frame;
	unsigned int config_counter;
	enum isp_stat_buf_status buf_status;
	void *stat_buf;
};

typedef union {
	unsigned int dwval;
	struct {
		unsigned int af_sharp:16;
		unsigned int hdr_cnt:4;
		unsigned int flash_ok:1;
		unsigned int capture_ok:1;
		unsigned int fast_capture_ok:1;
		unsigned int res0:9;
	} bits;
} IMAGE_FLAG;

/*
 * Struct	: isp_hdr_setting_t -
 * &:
 * &:
 */
struct hdr_setting_t {
  int hdr_en;
  int hdr_mode;
  int frames_count;
  int total_frames;
  int values[5];
};

/*
 *
 *   struct isp_3a_output - Stores the results of 3A.
 *   It will be used to adjust exposure time, vcm and other regs.
 *
 */
struct isp_3a_result {
	/* AE Output */
	unsigned int exp_line_num;
	unsigned int exp_analog_gain;
	unsigned int exp_digital_gain;
	unsigned int ae_gain;
	unsigned int lv;

	int min_rgb_pre[8];
	int defog_pre;
	unsigned int exp_time;
	unsigned int exp_line_temp;
	unsigned int analog_gain_temp;
	unsigned int digital_gain_temp;
	unsigned int ae_algo_adjust_cnt;
	unsigned int ae_algo_adjust_interval;
	unsigned int motion_flag;


	int flash_on;
	int hdr_req;
	int iso_value;		/* ISO */

	/* Flicker Output */
	int defog_changed;
	/* AF Output */
	int real_vcm_step;
	unsigned int real_vcm_pos;
	IMAGE_FLAG image_quality;

};

/*
 *
 *   struct isp_driver_to_3a_stat - Stores the 3A stat buffer and related settings.
 *
 */
struct isp_driver_to_3a_stat {

	/* v4l2 drivers fill */
	struct isp_stat_buffer *stat_buf_whole;
	void *ae_buf;
	void *af_buf;
	void *awb_buf;
	void *hist_buf;
	void *afs_buf;
	void *awb_win_buf;

	/* ISP drivers fill */

	int min_rgb_saved;
	int c_noise_saved;
};

/*
 *
 *   struct isp_gen_settings - Stores the isp settings
 *   also stores the stat buffer for 3a algorithms.
 */
struct isp_gen_settings {
	int isp_id;
	int isp_initialized;
	/* 3A statistic buffers and other values */
	struct isp_driver_to_3a_stat stat;
	/* ISP module config */
	struct isp_module_config module_cfg;

	int defog_min_rgb;

};

#endif

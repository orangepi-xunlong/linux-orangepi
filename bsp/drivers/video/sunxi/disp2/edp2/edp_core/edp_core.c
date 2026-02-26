/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * core function of edp driver
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "edp_core.h"
#include "../edp_configs.h"
#include "../lowlevel/edp_lowlevel.h"
#include <linux/io.h>
#include <linux/delay.h>

static struct disp_video_timings standard_video_timing[] = {
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 57,
		.hor_front_porch = 62,
		.hor_sync_time = 19,
		.ver_total_time = 525,
		.ver_back_porch = 4,
		.ver_front_porch = 1,
		.ver_sync_time = 3,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 69,
		.hor_front_porch = 63,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 2,
		.ver_front_porch = 44,
		.ver_sync_time = 3,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 60,
		.hor_front_porch = 62,
		.hor_sync_time = 16,
		.ver_total_time = 525,
		.ver_back_porch = 9,
		.ver_front_porch = 30,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 68,
		.hor_front_porch = 64,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 5,
		.ver_front_porch = 39,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1650,
		.hor_back_porch = 220,
		.hor_front_porch = 110,
		.hor_sync_time = 40,
		.ver_total_time = 750,
		.ver_back_porch = 20,
		.ver_front_porch = 5,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1980,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 440,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 88,
		.hor_sync_time = 44,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_60HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 88,
		.hor_sync_time = 44,
		.ver_total_time = 1125,
		.ver_back_porch = 36,
		.ver_front_porch = 4,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_50HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_2560_1600P_60HZ,
		.pixel_clk = 348577920,
		.pixel_repeat = 0,
		.x_res = 2560,
		.y_res = 1600,
		.hor_total_time = 3504,
		.hor_back_porch = 472,
		.hor_front_porch = 192,
		.hor_sync_time = 280,
		.ver_total_time = 1658,
		.ver_back_porch = 49,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,
		.ver_sync_polarity = 1,
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
};


s32 edp_list_standard_mode_num(void)
{
	return sizeof(standard_video_timing)/sizeof(struct disp_video_timings);
}

static bool standard_timing_match(struct disp_video_timings *std_timings, struct disp_video_timings *cmp_timings)
{
	bool match = true;

	if (std_timings->pixel_clk != cmp_timings->pixel_clk)
		match = false;

	if (std_timings->x_res != cmp_timings->x_res)
		match = false;

	if (std_timings->y_res != cmp_timings->y_res)
		match = false;

	if (std_timings->hor_total_time != cmp_timings->hor_total_time)
		match = false;

	if (std_timings->hor_back_porch != cmp_timings->hor_back_porch)
		match = false;

	if (std_timings->hor_front_porch != cmp_timings->hor_front_porch)
		match = false;

	if (std_timings->hor_sync_time != cmp_timings->hor_sync_time)
		match = false;

	if (std_timings->ver_total_time != cmp_timings->ver_total_time)
		match = false;

	if (std_timings->ver_back_porch != cmp_timings->ver_back_porch)
		match = false;

	if (std_timings->ver_front_porch != cmp_timings->ver_front_porch)
		match = false;

	if (std_timings->ver_sync_time != cmp_timings->ver_sync_time)
		match = false;

	return match;
}


u32 timing_to_standard_mode(struct disp_video_timings *timings)
{
	u32 mode = 0xff;
	u32 i, list_num;
	struct disp_video_timings *std_timings;

	std_timings = standard_video_timing;
	list_num = edp_list_standard_mode_num();
	for (i = 0; i < list_num; i++) {
		if (standard_timing_match(std_timings, timings)) {
			mode = std_timings->tv_mode;
			break;
		}
		std_timings++;
	}

	return mode;
}

s32 standard_mode_to_timing(enum disp_tv_mode mode, struct disp_video_timings *tmgs)
{
	u32 i, list_num;

	list_num = edp_list_standard_mode_num();
	for (i = 0; i < list_num; i++) {
		if (standard_video_timing[i].tv_mode == mode) {
			memcpy(tmgs, &standard_video_timing[i], sizeof(struct disp_video_timings));
			return RET_OK;
		}
	}
	return RET_FAIL;
}


/*edp_hal_xxx means xxx function is from lowlevel*/
s32 edp_core_init_early(u32 sel)
{
	return edp_hal_init_early(sel);
}

s32 edp_core_phy_init(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_phy_init(sel, edp_core);
}

void edp_core_set_reg_base(u32 sel, uintptr_t base)
{
	edp_hal_set_reg_base(sel, base);
}

s32 edp_core_get_hotplug_state(u32 sel)
{
	if (edp_hal_get_hotplug_change(sel))
		return edp_hal_get_hotplug_state(sel);
	else
		return -1;
}

/* set color space, color depth */
s32 edp_core_set_video_format(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;

	ret = edp_hal_set_video_format(sel, edp_core);
	if (ret)
		EDP_ERR("edp_core_set_video_format fail!\n");

	return ret;
}

s32 edp_core_set_video_timings(u32 sel, struct disp_video_timings *tmgs)
{
	s32 ret = RET_OK;

	ret = edp_hal_set_video_timings(sel, tmgs);
	if (ret)
		EDP_ERR("edp_core_set_video_timings fail!\n");

	return ret;
}

s32 edp_core_set_transfer_config(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;
	struct edp_lane_para *lane_para = &edp_core->lane_para;

	ret = edp_hal_set_transfer_config(sel, edp_core);
	if (ret) {
		EDP_ERR("edp_core_set_transfer_config fail! Maybe pixelclk out of lane's capability!\n");
		EDP_ERR("Check lane param! Lane param now: lane_cnt:%d lane_rate:%lld bpp:%d pixel_clk:%d\n",
			lane_para->lane_cnt, lane_para->bit_rate, lane_para->bpp, edp_core->timings.pixel_clk);
	}

	return ret;
}

s32 edp_core_enable(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_enable(sel, edp_core);
}

s32 edp_core_disable(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_disable(sel, edp_core);
}



u64 edp_source_get_max_rate(u32 sel)
{
	return edp_hal_get_max_rate(sel);
}

u32 edp_source_get_max_lane(u32 sel)
{
	return edp_hal_get_max_lane(sel);
}

bool edp_source_support_tps3(u32 sel)
{
	return edp_hal_support_tps3(sel);
}

bool edp_source_support_fast_train(u32 sel)
{
	return edp_hal_support_fast_train(sel);
}

bool edp_source_support_audio(u32 sel)
{
	return edp_hal_support_audio(sel);
}

bool edp_source_support_ssc(u32 sel)
{
	return edp_hal_support_ssc(sel);
}

bool edp_source_support_psr(u32 sel)
{
	return edp_hal_support_psr(sel);
}

bool edp_source_support_assr(u32 sel)
{
	return edp_hal_support_assr(sel);
}

bool edp_source_support_mst(u32 sel)
{
	return edp_hal_support_mst(sel);
}

s32 edp_core_query_max_pixclk(u32 sel, struct edp_tx_core *edp_core,
			      struct disp_video_timings *tmgs)
{
	u32 pixclk_max = 0;
	u32 pixclk = tmgs->pixel_clk;
	u32 lane_cnt_max = edp_source_get_max_lane(sel);
	/* lane's bandwidth = lane_rate / 10, such as 2.7G's bandwidth = 270M */
	u64 lane_bandwidth_max = edp_source_get_max_rate(sel) / 10;
	u32 bpp = edp_core->lane_para.bpp;

	/* 1 component's byte = bpp / 8bit */
	/* pixclk * bpp / 8 = lane_bandwidth * lane_cnt */
	pixclk_max = (lane_bandwidth_max / bpp) * lane_cnt_max * 8 ;
	EDP_CORE_DBG("lane_cnt_max:%d lane_bandwidth_max:%lld bpp:%d pixclk_support_max:%d pixclk_cur:%d\n",
		     lane_cnt_max, lane_bandwidth_max, bpp, pixclk_max, pixclk);

	if (pixclk > pixclk_max) {
		EDP_ERR("pixclk setting out of lane's max capability! pixclk_support_max:%d pixclk_cur:%d\n",
			pixclk_max, pixclk);
		return RET_FAIL;
	}

	return RET_OK;
}

s32 edp_core_query_current_pixclk(u32 sel, struct edp_tx_core *edp_core,
				  struct disp_video_timings *tmgs)
{
	u32 pixclk_cur_max = 0;
	u32 pixclk = tmgs->pixel_clk;
	u32 lane_cnt_cur = edp_core->lane_para.lane_cnt;
	/* lane's bandwidth = lane_rate / 10, such as 2.7G's bandwidth = 270M */
	u64 lane_bandwidth_cur = edp_core->lane_para.bit_rate / 10;
	u32 bpp = edp_core->lane_para.bpp;

	if ((lane_cnt_cur == 0) || (lane_bandwidth_cur == 0) || (pixclk == 0)) {
		EDP_ERR("lane param or timings not set, lane_cnt:%d lane_rate:%lld pixclk:%d\n",
			lane_cnt_cur, lane_bandwidth_cur * 10, pixclk);
		EDP_ERR("Maybe lane param not set in dts, or DPCD/EDID read fail via AUX, please check!\n");
		return RET_FAIL;
	}

	/* 1 component's byte = bpp / 8bit */
	/* pixclk * bpp / 8 = lane_bandwidth * lane_cnt */
	pixclk_cur_max = (lane_bandwidth_cur / bpp) * lane_cnt_cur * 8 ;
	EDP_CORE_DBG("lane_cnt_cur:%d lane_bandwidth_cur:%lld bpp:%d pixclk_cur_max:%d pixclk_cur:%d\n",
		     lane_cnt_cur, lane_bandwidth_cur, bpp, pixclk_cur_max, pixclk);

	if (pixclk > pixclk_cur_max) {
		EDP_ERR("pixclk setting out of current lane's capability! pixclk_cur_max:%d pixclk_cur:%d\n",
			pixclk_cur_max, pixclk);
		return RET_FAIL;
	}

	return RET_OK;
}

s32 edp_core_query_transfer_unit(u32 sel, struct edp_tx_core *edp_core,
				 struct disp_video_timings *tmgs)
{
	return edp_hal_query_transfer_unit(sel, edp_core, tmgs);
}


s32 edp_core_query_lane_capability(u32 sel, struct edp_tx_core *edp_core,
				   struct disp_video_timings *tmgs)
{
	s32 ret = 0;

	/* check if pixclk out of lane's max capability */
	ret = edp_core_query_max_pixclk(sel, edp_core, tmgs);
	if (ret)
		return RET_FAIL;

	/* check if pixclk out of current lane config's capability */
	ret = edp_core_query_current_pixclk(sel, edp_core, tmgs);
	if (ret)
		return RET_FAIL;

	/*
	 * some controller's lane has valid link symbol limit per
	 * lane (64 is defined as MAX in DP spec), query if pixclk
	 * out of lane's link symbol limit
	 */
	ret = edp_core_query_transfer_unit(sel, edp_core, tmgs);
	if (ret)
		return RET_FAIL;

	return RET_OK;
}


s32 edp_get_link_status(u32 sel, char *link_status)
{
	return edp_core_aux_read(sel, DPCD_0202H, LINK_STATUS_SIZE, link_status);
}

bool edp_sink_support_fast_train(u32 sel)
{

	char tmp_rx_buf[16];

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if (edp_core_aux_read(sel, DPCD_0003H, 1, &tmp_rx_buf[0]) < 0)
		return false;

	if (tmp_rx_buf[0] & DPCD_FAST_TRAIN_MASK)
		return true;
	else
		return false;
}

bool edp_sink_support_tps3(u32 sel)
{
	char tmp_rx_buf[16];

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if (edp_core_aux_read(sel, DPCD_0002H, 1, &tmp_rx_buf[0]) < 0)
		return false;

	if (tmp_rx_buf[0] & DPCD_TPS3_SUPPORT_MASK)
		return true;
	else
		return false;
}


void edp_core_set_lane_para(u32 sel, struct edp_tx_core *edp_core)
{
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt = edp_core->lane_para.lane_cnt;

	edp_hal_set_lane_rate(sel, bit_rate);
	edp_hal_set_lane_cnt(sel, lane_cnt);
}

void edp_lane_training_para_reset(struct edp_lane_para *lane_para)
{
	lane_para->lane0_sw = 0;
	lane_para->lane1_sw = 0;
	lane_para->lane2_sw = 0;
	lane_para->lane3_sw = 0;
	lane_para->lane0_pre = 0;
	lane_para->lane1_pre = 0;
	lane_para->lane2_pre = 0;
	lane_para->lane3_pre = 0;
}


void edp_core_set_training_para(u32 sel, struct edp_tx_core *edp_core)
{
	u32 sw[4];
	u32 pre[4];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 param_type = edp_core->training_param_type;
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane0_sw;
	sw[1] = edp_core->lane_para.lane1_sw;
	sw[2] = edp_core->lane_para.lane2_sw;
	sw[3] = edp_core->lane_para.lane3_sw;
	pre[0] = edp_core->lane_para.lane0_pre;
	pre[1] = edp_core->lane_para.lane1_pre;
	pre[2] = edp_core->lane_para.lane2_pre;
	pre[3] = edp_core->lane_para.lane3_pre;

	for (i = 0; i < lane_count; i++) {
		EDP_CORE_DBG("set training para: lane[%d] sw:%d pre:%d\n", i, sw[i], pre[i]);
		edp_hal_set_lane_sw_pre(sel, i, sw[i], pre[i], param_type);
	}
}

s32 edp_core_set_training_pattern(u32 sel, u32 pattern)
{
	edp_hal_set_training_pattern(sel, pattern);
	return RET_OK;
}

s32 edp_dpcd_set_lane_para(u32 sel, struct edp_tx_core *edp_core)
{
	char tmp_tx_buf[16];
	s32 ret = RET_FAIL;

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));
	tmp_tx_buf[0] = edp_core->lane_para.bit_rate / 10000000 / 27;
	tmp_tx_buf[1] = edp_core->lane_para.lane_cnt;

	ret = edp_core_aux_write(sel, DPCD_0100H, 2,  &tmp_tx_buf[0]);

	return ret;
}

s32 edp_dpcd_set_training_para(u32 sel, struct edp_tx_core *edp_core)
{
	char tmp_tx_buf[16];
	u32 sw[4];
	u32 pre[4];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane0_sw;
	sw[1] = edp_core->lane_para.lane1_sw;
	sw[2] = edp_core->lane_para.lane2_sw;
	sw[3] = edp_core->lane_para.lane3_sw;
	pre[0] = edp_core->lane_para.lane0_pre;
	pre[1] = edp_core->lane_para.lane1_pre;
	pre[2] = edp_core->lane_para.lane2_pre;
	pre[3] = edp_core->lane_para.lane3_pre;

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	for (i = 0; i < lane_count; i++) {
		tmp_tx_buf[i] = (pre[i] << DPCD_PLEVEL_SHIFT) | (sw[i] << DPCD_VLEVEL_SHIFT);
		if (sw[i] == MAX_VLEVEL)
			tmp_tx_buf[i] |= DPCD_MAX_VLEVEL_REACHED_FLAG;
		if (pre[i] == MAX_PLEVEL)
			tmp_tx_buf[i] |= DPCD_MAX_PLEVEL_REACHED_FLAG;
	}

	edp_core_aux_write(sel, DPCD_0103H, lane_count, &tmp_tx_buf[0]);

	return RET_OK;
}

s32 edp_dpcd_set_training_pattern(u32 sel, u8 pattern)
{
	char tmp_tx_buf[16];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	tmp_tx_buf[0] = pattern;

	if (pattern && (pattern != TRAINING_PATTERN_4))
		tmp_tx_buf[0] |= DPCD_SCRAMBLING_DISABLE_FLAG;

	edp_core_aux_write(sel, DPCD_0102H, 1, &tmp_tx_buf[0]);

	return RET_OK;
}

s32 edp_training_pattern_clear(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;

	edp_core_set_training_pattern(sel, TRAINING_PATTERN_DISABLE);
	ret = edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_DISABLE);
	if (ret < 0)
		return ret;

	usleep_range(edp_core->interval_EQ, 2 * edp_core->interval_EQ);

	return ret;
}

bool edp_cr_training_done(char *link_status, u32 lane_count, u32 *fail_lane)
{
	u32 i = 0;
	u8 lane_status = 0;
	u32 cr_fail = 0;

	for (i = 0; i < lane_count; i++) {
		lane_status = (link_status[DPCD_LANE_STATUS_OFFSET(i)]) & DPCD_LANE_STATUS_MASK(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_STATUS_OFFSET(i),\
		       link_status[DPCD_LANE_STATUS_OFFSET(i)]);

		EDP_CORE_DBG("CR_lane%d_status = 0x%x CR_DONE:%s\n", i, lane_status,
			     lane_status & DPCD_CR_DONE(i) ? "yes" : "no");

		if ((lane_status & DPCD_CR_DONE(i)) != DPCD_CR_DONE(i)) {
			EDP_CORE_DBG("lane_%d CR training fail!\n", i);
			cr_fail |= (1 << i);
		}
	}

	if (cr_fail) {
		*fail_lane = cr_fail;
		return false;
	} else {
		EDP_CORE_DBG("CR training success!\n");
		return true;
	}
}

bool edp_eq_training_done(char *link_status, u32 lane_count, u32 *fail_lane)
{
	u32 i = 0;
	u8 lane_status = 0;
	u8 align_status = 0;
	u32 eq_fail = 0;

	align_status = (link_status[DPCD_0204H - LINK_STATUS_BASE]);
	for (i = 0; i < lane_count; i++) {
		lane_status = (link_status[DPCD_LANE_STATUS_OFFSET(i)]) & DPCD_LANE_STATUS_MASK(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_STATUS_OFFSET(i),\
		       link_status[DPCD_LANE_STATUS_OFFSET(i)]);

		EDP_CORE_DBG("EQ_lane%d_status = 0x%x EQ_DONE:%s SYMBOL_LOCK:%s\n", i, lane_status,
			     lane_status & DPCD_EQ_DONE(i) ? "yes" : "no",
			     lane_status & DPCD_SYMBOL_LOCK(i) ? "yes" : "no");

		if ((lane_status & DPCD_EQ_TRAIN_DONE(i)) !=  DPCD_EQ_TRAIN_DONE(i)) {
			EDP_CORE_DBG("lane_%d EQ training fail!\n", i);
			eq_fail |= (1 << i);
		}
	}

	EDP_CORE_DBG("Align_status = 0x%x INTERLANE_ALIGN:%s\n", align_status,
		     align_status & DPCD_ALIGN_DONE ? "yes" : "no");

	if ((align_status & DPCD_ALIGN_DONE) != DPCD_ALIGN_DONE) {
		EDP_CORE_DBG("EQ training align fail!\n");
		*fail_lane = eq_fail;
		return false;
	}

	if (eq_fail) {
		*fail_lane = eq_fail;
		return false;
	} else {
		EDP_CORE_DBG("EQ training success!\n");
		return true;
	}
}

bool edp_swing_level_reach_max(struct edp_tx_core *edp_core)
{
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 sw[4];
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane0_sw;
	sw[1] = edp_core->lane_para.lane1_sw;
	sw[2] = edp_core->lane_para.lane2_sw;
	sw[3] = edp_core->lane_para.lane3_sw;

	for (i = 0; i < lane_count; i++) {
		if (sw[i] == MAX_VLEVEL)
			return true;
	}

	return false;
}

u32 edp_adjust_train_para(char *link_status, struct edp_tx_core *edp_core)
{
	u32 old_sw[4];
	u32 old_pre[4];
	u32 adjust_sw[4];
	u32 adjust_pre[4];
	u32 i = 0;
	struct edp_lane_para *lane_para = &edp_core->lane_para;
	u32 lane_count = lane_para->lane_cnt;
	u32 change;

	change = 0;

	old_sw[0] = lane_para->lane0_sw;
	old_sw[1] = lane_para->lane1_sw;
	old_sw[2] = lane_para->lane2_sw;
	old_sw[3] = lane_para->lane3_sw;
	old_pre[0] = lane_para->lane0_pre;
	old_pre[1] = lane_para->lane1_pre;
	old_pre[2] = lane_para->lane2_pre;
	old_pre[3] = lane_para->lane3_pre;

	for (i = 0; i < lane_count; i++) {
		adjust_sw[i] = (link_status[DPCD_LANE_ADJ_OFFSET(i)] & VADJUST_MASK(i)) >> VADJUST_SHIFT(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x, adjust_sw[%d] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_ADJ_OFFSET(i),\
		       link_status[DPCD_LANE_ADJ_OFFSET(i)], i, adjust_sw[i]);

		adjust_pre[i] = (link_status[DPCD_LANE_ADJ_OFFSET(i)] & PADJUST_MASK(i)) >> PADJUST_SHIFT(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x, adjust_pre[%d] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_ADJ_OFFSET(i),\
		       link_status[DPCD_LANE_ADJ_OFFSET(i)], i, adjust_pre[i]);
	}

	lane_para->lane0_sw = adjust_sw[0];
	lane_para->lane1_sw = adjust_sw[1];
	lane_para->lane2_sw = adjust_sw[2];
	lane_para->lane3_sw = adjust_sw[3];

	lane_para->lane0_pre = adjust_pre[0];
	lane_para->lane1_pre = adjust_pre[1];
	lane_para->lane2_pre = adjust_pre[2];
	lane_para->lane3_pre = adjust_pre[3];

	for (i = 0; i < lane_count; i++) {
		if (old_sw[i] != adjust_sw[i])
			change |= SW_VOLTAGE_CHANGE_FLAG;
	}

	for (i = 0; i < lane_count; i++) {
		if (old_pre[i] != adjust_pre[i])
			change |= PRE_EMPHASIS_CHANGE_FLAG;
	}

	EDP_CORE_DBG("edp lane para change: %d\n", change);
	return change;
}

s32 edp_link_cr_training(u32 sel, struct edp_tx_core *edp_core)
{
	char link_status[16];
	u32 try_cnt = 0;
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 change;
	u32 timeout = 0;
	u32 fail_lane;

	edp_core_set_training_pattern(sel, TRAINING_PATTERN_1);
	edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_1);

	for (try_cnt = 0; try_cnt < TRY_CNT_MAX; try_cnt++) {
		usleep_range(5 * edp_core->interval_CR, 10 * edp_core->interval_CR);

		memset(link_status, 0, sizeof(link_status));
		fail_lane = 0;
		edp_get_link_status(sel, link_status);
		if (edp_cr_training_done(link_status, lane_count, &fail_lane)) {
			EDP_CORE_DBG("edp training1(clock recovery training) success!\n");
			return RET_OK;
		}

		if (timeout >= TRY_CNT_TIMEOUT) {
			EDP_ERR("edp training1(clock recovery training) timeout!\n");
			return RET_FAIL;
		}

		if (edp_swing_level_reach_max(edp_core)) {
			EDP_ERR("swing voltage reach max level, training1(clock recovery training) fail!\n");
			return RET_FAIL;
		}

		change = edp_adjust_train_para(link_status, edp_core);
		if (change & SW_VOLTAGE_CHANGE_FLAG) {
			try_cnt = 0;
		}

		if ((change & SW_VOLTAGE_CHANGE_FLAG) || (change & PRE_EMPHASIS_CHANGE_FLAG)) {
			edp_core_set_training_para(sel, edp_core);
			edp_dpcd_set_training_para(sel, edp_core);
		}

		timeout++;
	}

	EDP_ERR("CR training result: lane0:%s lane1:%s lane2:%s lane3:%s\n",
		fail_lane & (1 << 0) ? "FAIL" : "PASS",
		fail_lane & (1 << 1) ? "FAIL" : "PASS",
		fail_lane & (1 << 2) ? "FAIL" : "PASS",
		fail_lane & (1 << 3) ? "FAIL" : "PASS");
	EDP_ERR("retry 5 times but still fail, training1(clock recovery training) fail!\n");
	return RET_FAIL;
}

s32 edp_link_eq_training(u32 sel, struct edp_tx_core *edp_core)
{
	char link_status[16];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 try_cnt = 0;
	bool change;
	u32 fail_lane;

	if (edp_source_support_tps3(sel) && edp_sink_support_tps3(sel)) {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_3);
		edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_3);
	} else {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_2);
		edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_2);
	}

	for (try_cnt = 0; try_cnt < TRY_CNT_MAX; try_cnt++) {
		usleep_range(5 * edp_core->interval_EQ, 10 * edp_core->interval_EQ);
		memset(link_status, 0, sizeof(link_status));
		fail_lane = 0;
		edp_get_link_status(sel, link_status);
		if (edp_eq_training_done(link_status, lane_count, &fail_lane)) {
			EDP_CORE_DBG("edp training2 (equalization training) success!\n");
			return RET_OK;
		}

		change = edp_adjust_train_para(link_status, edp_core);
		if ((change & SW_VOLTAGE_CHANGE_FLAG) || (change & PRE_EMPHASIS_CHANGE_FLAG)) {
			edp_core_set_training_para(sel, edp_core);
			edp_dpcd_set_training_para(sel, edp_core);
		}
	}

	EDP_ERR("EQ training result: lane0:%s lane1:%s lane2:%s lane3:%s\n",
		fail_lane & (1 << 0) ? "FAIL" : "PASS",
		fail_lane & (1 << 1) ? "FAIL" : "PASS",
		fail_lane & (1 << 2) ? "FAIL" : "PASS",
		fail_lane & (1 << 3) ? "FAIL" : "PASS");
	EDP_ERR("retry 5 times but still fail, training2(equalization training) fail!\n");
	return RET_FAIL;
}

s32 edp_fast_link_train(u32 sel, struct edp_tx_core *edp_core)
{
	char link_status[LINK_STATUS_SIZE];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 fail_lane = 0;

	edp_core_set_lane_para(sel, edp_core);

	edp_core_set_training_para(sel, edp_core);

	edp_core_set_training_pattern(sel, TRAINING_PATTERN_1);
	usleep_range(5 * edp_core->interval_CR, 10 * edp_core->interval_CR);

	if (edp_source_support_tps3(sel) && edp_sink_support_tps3(sel)) {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_3);
	} else {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_2);
	}
	usleep_range(5 * edp_core->interval_EQ, 10 * edp_core->interval_EQ);

	memset(link_status, 0, sizeof(link_status));
	if (loglevel_debug & 0x2) {
		edp_get_link_status(sel, link_status);
		if (!edp_cr_training_done(link_status, lane_count, &fail_lane)) {
			EDP_ERR("CR training result: lane0:%s lane1:%s lane2:%s lane3:%s\n",
				fail_lane & (1 << 0) ? "FAIL" : "PASS",
				fail_lane & (1 << 1) ? "FAIL" : "PASS",
				fail_lane & (1 << 2) ? "FAIL" : "PASS",
				fail_lane & (1 << 3) ? "FAIL" : "PASS");
			EDP_ERR("edp fast train fail in training1");
			return RET_FAIL;
		}

		fail_lane = 0;
		if (!edp_eq_training_done(link_status, lane_count, &fail_lane)) {
			EDP_ERR("EQ training result: lane0:%s lane1:%s lane2:%s lane3:%s\n",
				fail_lane & (1 << 0) ? "FAIL" : "PASS",
				fail_lane & (1 << 1) ? "FAIL" : "PASS",
				fail_lane & (1 << 2) ? "FAIL" : "PASS",
				fail_lane & (1 << 3) ? "FAIL" : "PASS");
			EDP_ERR("edp fast train fail in training2");
			return RET_FAIL;
		}
	}

	return RET_OK;
}

s32 edp_full_link_train(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;;

	edp_core_set_lane_para(sel, edp_core);
	ret = edp_dpcd_set_lane_para(sel, edp_core);
	if (ret < 0)
		return ret;

	edp_core_set_training_para(sel, edp_core);
	ret = edp_dpcd_set_training_para(sel, edp_core);
	if (ret < 0)
		return ret;

	ret = edp_link_cr_training(sel, edp_core);
	if (ret < 0)
		return ret;

	ret = edp_link_eq_training(sel, edp_core);
	if (ret < 0)
		return ret;

	return ret;
}


s32 edp_link_training(u32 sel, struct edp_tx_core *edp_core)
{
	if (edp_source_support_fast_train(sel) \
		&& edp_sink_support_fast_train(sel))
		return edp_fast_link_train(sel, edp_core);
	else
		return edp_full_link_train(sel, edp_core);
}

s32 edp_main_link_setup(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = 0;

	/* DP Link CTS need ech training start from level-0 */
	edp_lane_training_para_reset(&edp_core->lane_para);

	ret = edp_link_training(sel, edp_core);
	if (ret < 0)
		return ret;
	ret = edp_training_pattern_clear(sel, edp_core);

	return ret;
}

s32 edp_core_link_start(u32 sel)
{
	return edp_hal_link_start(sel);
}

s32 edp_core_link_stop(u32 sel)
{
	return edp_hal_link_stop(sel);
}


/*
 * two case should be considered when judge if mode support
 * 1. source device's (edp phy) capability
 * 2. sink device's capability
 *
 * bit_rate and lane_cnt was updated when sink connect
 *
 * return 0 if mode not support, otherwise mode is support.
 */
s32 edp_mode_support(u32 sel, enum disp_tv_mode mode, struct edp_tx_core *edp_core)
{
	struct disp_video_timings tmgs;
	s32 ret = 0;

	memset(&tmgs, 0, sizeof(struct disp_video_timings));

	/* query if the mode can support by sink capability */
	ret = standard_mode_to_timing(mode, &tmgs);
	if (ret) {
		EDP_ERR("no standard mode found!\n");
		return RET_FAIL;
	}

	ret = edp_core_query_lane_capability(sel, edp_core, &tmgs);
	if (ret)
		return RET_FAIL;

	return RET_OK;
}

s32 edp_low_power_en(u32 sel, bool en)
{
	char tmp_tx_buf[16];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	if (en)
		tmp_tx_buf[0] = DPCD_LOW_POWER_ENTER;
	else
		tmp_tx_buf[0] = DPCD_LOW_POWER_EXIT;

	edp_core_aux_write(sel, DPCD_0600H, 1, &tmp_tx_buf[0]);

	return RET_OK;
}

void edp_core_irq_handler(u32 sel, struct edp_tx_core *edp_core)
{
	edp_hal_irq_handler(sel, edp_core);
}

s32 edp_core_irq_enable(u32 sel, u32 irq_id, bool en)
{
	if (en)
		return edp_hal_irq_enable(sel, irq_id);
	else
		return edp_hal_irq_disable(sel, irq_id);
}

s32 edp_core_irq_query(u32 sel)
{
	return edp_hal_irq_query(sel);
}

s32 edp_core_irq_clear(u32 sel)
{
	return edp_hal_irq_clear(sel);
}

s32 edp_core_get_cur_line(u32 sel)
{
	return edp_hal_get_cur_line(sel);
}

s32 edp_core_get_start_dly(u32 sel)
{
	return edp_hal_get_start_dly(sel);
}

void edp_core_show_builtin_patten(u32 sel, u32 pattern)
{
	edp_hal_show_builtin_patten(sel, pattern);
}

s32 edp_core_aux_read(u32 sel, s32 addr, s32 lenth, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;

	while (retry_cnt < 7) {
		ret = edp_hal_aux_read(sel, addr, lenth, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	if (ret != RET_OK)
		EDP_ERR("edp_aux_read fail, addr:0x%x lenth:0x%x\n", addr, lenth);

	return ret;
}

s32 edp_core_aux_write(u32 sel, s32 addr, s32 lenth, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;

	while (retry_cnt < 7) {
		ret = edp_hal_aux_write(sel, addr, lenth, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	if (ret != RET_OK)
		EDP_ERR("edp_aux_write fail, addr:0x%x lenth:0x%x\n", addr, lenth);

	return ret;
}

s32 edp_core_aux_i2c_read(u32 sel, s32 i2c_addr, s32 lenth, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = edp_hal_aux_i2c_read(sel, i2c_addr, lenth, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	if (ret != RET_OK)
		EDP_ERR("edp_aux_i2c_read fail, addr:0x%x lenth:0x%x\n", i2c_addr, lenth);

	return ret;
}

s32 edp_core_aux_i2c_write(u32 sel, s32 i2c_addr, s32 lenth, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = edp_hal_aux_i2c_write(sel, i2c_addr, lenth, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	if (ret != RET_OK)
		EDP_ERR("edp_aux_i2c_write fail, addr:0x%x lenth:0x%x\n", i2c_addr, lenth);

	return ret;
}

s32 edp_core_audio_set_para(u32 sel, edp_audio_t *para)
{
	return edp_hal_audio_set_para(sel, para);
}

s32 edp_core_audio_enable(u32 sel)
{
	return edp_hal_audio_enable(sel);
}

s32 edp_core_audio_disable(u32 sel)
{
	return edp_hal_audio_disable(sel);
}

s32 edp_core_ssc_enable(u32 sel, bool enable)
{
	return edp_hal_ssc_enable(sel, enable);
}

bool edp_core_ssc_is_enabled(u32 sel)
{
	return edp_hal_ssc_is_enabled(sel);
}

s32 edp_core_ssc_get_mode(u32 sel)
{
	return edp_hal_ssc_get_mode(sel);
}

s32 edp_core_ssc_set_mode(u32 sel, u32 mode)
{
	return edp_hal_ssc_set_mode(sel, mode);
}

s32 edp_core_psr_enable(u32 sel, bool enable)
{
	return edp_hal_psr_enable(sel, enable);
}

bool edp_core_psr_is_enabled(u32 sel)
{
	return edp_hal_psr_is_enabled(sel);
}

s32 edp_core_assr_enable(u32 sel, bool enable)
{
	s32 ret = RET_FAIL;
	char tmp_rx_buf[16];

	ret = edp_hal_assr_enable(sel, enable);
	if (ret)
		return ret;

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));
	ret = edp_core_aux_read(sel, DPCD_010AH, 1, &tmp_rx_buf[0]);
	if (ret)
		return ret;

	if (enable)
		tmp_rx_buf[0] |= DPCD_ASSR_ENABLE_MASK;
	else
		tmp_rx_buf[0] &= ~DPCD_ASSR_ENABLE_MASK;

	ret = edp_core_aux_write(sel, DPCD_010AH, 1,  &tmp_rx_buf[0]);

	return ret;
}

s32 edp_core_set_pattern(u32 sel, u32 pattern)
{
	return edp_hal_set_pattern(sel, pattern);
}

s32 edp_core_get_color_fmt(u32 sel)
{
	return edp_hal_get_color_fmt(sel);
}

s32 edp_core_get_pixclk(u32 sel)
{
	return edp_hal_get_pixclk(sel);
}

s32 edp_core_get_train_pattern(u32 sel)
{
	return edp_hal_get_train_pattern(sel);
}

s32 edp_core_get_lane_para(u32 sel, struct edp_lane_para *tmp_lane_para)
{
	return edp_hal_get_lane_para(sel, tmp_lane_para);
}

s32 edp_core_get_tu_size(u32 sel)
{
	return edp_hal_get_tu_size(sel);
}

s32 edp_core_get_valid_symbol_per_tu(u32 sel)
{
	return edp_hal_get_valid_symbol_per_tu(sel);
}

bool edp_core_audio_is_enabled(u32 sel)
{
	return edp_hal_audio_is_enabled(sel);
}

s32 edp_core_get_audio_if(u32 sel)
{
	return edp_hal_get_audio_if(sel);
}

s32 edp_core_audio_is_mute(u32 sel)
{
	return edp_hal_audio_is_mute(sel);
}

s32 edp_core_get_audio_chn_cnt(u32 sel)
{
	return edp_hal_get_audio_chn_cnt(sel);
}

s32 edp_core_get_audio_date_width(u32 sel)
{
	return edp_hal_get_audio_date_width(sel);
}

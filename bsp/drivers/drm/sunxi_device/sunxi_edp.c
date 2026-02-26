/* SPDX-License-Identifier: GPL-2.0-or-later */
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

#include "sunxi_edp.h"
//#include "hardware/lowlevel_edp/edp_lowlevel.h"
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>

void edp_hw_scrambling_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable);

static const char edid_header[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00
};


/*
 * Search EDID for CEA extension block.
 */
u8 *sunxi_drm_find_edid_extension(const struct edid *edid,
				   int ext_id, int *ext_index)
{
	u8 *edid_ext = NULL;
	int i;

	/* No EDID or EDID extensions */
	if (edid == NULL || edid->extensions == 0)
		return NULL;

	/* Find CEA extension */
	for (i = *ext_index; i < edid->extensions; i++) {
		edid_ext = (u8 *)edid + EDID_LENGTH * (i + 1);
		if (edid_ext[0] == ext_id)
			break;
	}

	if (i >= edid->extensions)
		return NULL;

	*ext_index = i + 1;

	return edid_ext;
}

static s32 edid_block_cnt(struct edid *edid)
{
	return edid->extensions + 1;
}

int edp_get_edid_block(void *data, u8 *block_raw_data,
			  unsigned int block_id, size_t len)
{
	int ret = 0;
	struct sunxi_edp_hw_desc *edp_hw = (struct sunxi_edp_hw_desc *)(data);
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->read_edid_block)
		ret = ops->read_edid_block(edp_hw, block_raw_data, block_id, len);
	else
		ret = RET_OK;

	if (ret < 0) {
		EDP_ERR("EDID: edp read edid block%d failed\n", block_id);
	}

	return ret;
}

s32 edp_edid_put(struct edid *edid)
{
	u32 block_cnt;

	if (edid == NULL) {
		EDP_WRN("EDID: edid is already null\n");
		return 0;
	}

	block_cnt = edid_block_cnt(edid);

	memset(edid, 0, block_cnt * EDID_LENGTH);

	kfree(edid);

	return 0;
}

s32 edp_edid_cea_db_offsets(const u8 *cea, s32 *start, s32 *end)
{
	/* DisplayID CTA extension blocks and top-level CEA EDID
	 * block header definitions differ in the following bytes:
	 *   1) Byte 2 of the header specifies length differently,
	 *   2) Byte 3 is only present in the CEA top level block.
	 *
	 * The different definitions for byte 2 follow.
	 *
	 * DisplayID CTA extension block defines byte 2 as:
	 *   Number of payload bytes
	 *
	 * CEA EDID block defines byte 2 as:
	 *   Byte number (decimal) within this block where the 18-byte
	 *   DTDs begin. If no non-DTD data is present in this extension
	 *   block, the value should be set to 04h (the byte after next).
	 *   If set to 00h, there are no DTDs present in this block and
	 *   no non-DTD data.
	 */
	if (cea[0] == DATA_BLOCK_CTA) {
		*start = 3;
		*end = *start + cea[2];
	} else if (cea[0] == CEA_EXT) {
		/* Data block offset in CEA extension block */
		*start = 4;
		*end = cea[2];
		if (*end == 0)
			*end = 127;
		if (*end < 4 || *end > 127)
			return -ERANGE;
	} else {
		return -EOPNOTSUPP;
	}

	return 0;
}

s32 edp_hw_init_early(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->init_early)
		return ops->init_early(edp_hw);
	else
		return RET_OK;
}

s32 edp_hw_controller_init(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->init)
		return ops->init(edp_hw, edp_core);
	else
		return RET_OK;
}

void edp_hw_set_reg_base(struct sunxi_edp_hw_desc *edp_hw, void __iomem *base)
{

	edp_hw->reg_base = base;
}

void edp_hw_set_top_base(struct sunxi_edp_hw_desc *edp_hw, void __iomem *base)
{

	edp_hw->top_base = base;
}

/*
 * -1: state_not_change
 *  0: change_to_plugout
 *  1: change_to_plugin
 */
s32 edp_hw_get_hotplug_state(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->get_hotplug_change && ops->get_hotplug_state) {
		if (ops->get_hotplug_change(edp_hw))
			return ops->get_hotplug_state(edp_hw);
	}

	return -1;
}

/* set color space, color depth */
s32 edp_hw_set_video_format(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->set_video_format)
		ret = ops->set_video_format(edp_hw, edp_core);
	else
		ret = RET_OK;

	if (ret)
		EDP_ERR("edp_hw_set_video_format fail!\n");

	return ret;
}

s32 edp_hw_set_video_timings(struct sunxi_edp_hw_desc *edp_hw, struct disp_video_timings *tmgs)
{
	s32 ret = RET_OK;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->set_video_timings)
		ret = ops->set_video_timings(edp_hw, tmgs);
	else
		ret = RET_OK;

	if (ret)
		EDP_ERR("edp_hw_set_video_timings fail!\n");

	return ret;
}

s32 edp_hw_set_transfer_config(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;
	struct edp_lane_para *lane_para = &edp_core->lane_para;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->config_tu)
		ret = ops->config_tu(edp_hw, edp_core);
	else
		ret = RET_OK;

	if (ret) {
		EDP_ERR("edp_hw_set_transfer_config fail! Maybe pixelclk out of lane's capability!\n");
		EDP_ERR("Check lane param! Lane param now: lane_cnt:%d lane_rate:%lld bpp:%d pixel_clk:%d\n",
			lane_para->lane_cnt, lane_para->bit_rate, lane_para->bpp, edp_core->timings.pixel_clk);
	}

	return ret;
}

s32 edp_hw_enable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->enable)
		return ops->enable(edp_hw, edp_core);
	else
		return RET_OK;
}

s32 edp_hw_disable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->disable)
		return ops->disable(edp_hw, edp_core);
	else
		return RET_OK;
}

u64 edp_source_get_max_rate(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->support_max_rate)
		return ops->support_max_rate(edp_hw);
	else
		return 0;
}

u32 edp_source_get_max_lane(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->support_max_lane)
		return ops->support_max_lane(edp_hw);
	else
		return 0;
}

bool edp_source_support_tps3(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_tps3)
		return ops->support_tps3(edp_hw);
	else
		return false;
}

bool edp_source_support_fast_training(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_fast_training)
		return ops->support_fast_training(edp_hw);
	else
		return false;
}

bool edp_source_support_audio(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return false;

	if (ops->support_audio)
		return ops->support_audio(edp_hw);
	else
		return false;
}

bool edp_source_support_ssc(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_ssc)
		return ops->support_ssc(edp_hw);
	else
		return false;
}

bool edp_source_support_fec(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_fec)
		return ops->support_fec(edp_hw);
	else
		return false;
}

bool edp_source_support_psr(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_psr)
		return ops->support_psr(edp_hw);
	else
		return false;
}

bool edp_source_support_psr2(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_psr2)
		return ops->support_psr2(edp_hw);
	else
		return false;
}

bool edp_source_support_assr(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_assr)
		return ops->support_assr(edp_hw);
	else
		return false;
}

bool edp_source_support_mst(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_mst)
		return ops->support_mst(edp_hw);
	else
		return false;
}

bool edp_source_support_lane_remap(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_lane_remap)
		return ops->support_lane_remap(edp_hw);
	else
		return false;
}

bool edp_source_support_lane_invert(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_lane_invert)
		return ops->support_lane_invert(edp_hw);
	else
		return false;
}

bool edp_source_support_muti_pixel_mode(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_muti_pixel_mode)
		return ops->support_muti_pixel_mode(edp_hw);
	else
		return false;
}

bool edp_source_need_reset_before_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->need_reset_before_enable)
		return ops->need_reset_before_enable(edp_hw);
	else
		return false;
}

bool edp_source_support_hdcp1x(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return false;

	if (ops->support_hdcp1x)
		return ops->support_hdcp1x(edp_hw);
	else
		return false;
}

bool edp_source_support_hardware_hdcp1x(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return false;

	if (ops->support_hw_hdcp1x)
		return ops->support_hw_hdcp1x(edp_hw);
	else
		return false;
}

bool edp_source_support_hdcp2x(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return false;

	if (ops->support_hdcp2x)
		return ops->support_hdcp2x(edp_hw);
	else
		return false;
}

bool edp_source_support_hardware_hdcp2x(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return false;

	if (ops->support_hw_hdcp2x)
		return ops->support_hw_hdcp2x(edp_hw);
	else
		return false;
}

bool edp_source_support_enhance_frame(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->support_enhance_frame)
		return ops->support_enhance_frame(edp_hw);
	else
		return false;
}

bool edp_set_use_inner_clk(struct sunxi_edp_hw_desc *edp_hw, u32 bypass)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;
	if (ops == NULL)
		return false;

	if (ops->set_use_inner_clk)
		return ops->set_use_inner_clk(edp_hw, bypass);
	else
		return false;
}

s32 edp_hw_query_max_pixclk(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core,
			      struct disp_video_timings *tmgs)
{
	u32 pixclk_max = 0;
	u32 pixclk = tmgs->pixel_clk;
	u32 lane_cnt_max = edp_source_get_max_lane(edp_hw);
	/* lane's bandwidth = lane_rate / 10, such as 2.7G's bandwidth = 270M */
	u64 lane_bandwidth_max = edp_source_get_max_rate(edp_hw) / 10;
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

s32 edp_hw_query_current_pixclk(struct edp_tx_core *edp_core,
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

s32 edp_hw_query_transfer_unit(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core,
				 struct disp_video_timings *tmgs)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->query_tu_capability)
		return ops->query_tu_capability(edp_hw, edp_core, tmgs);
	else
		return RET_OK;
}


s32 edp_hw_query_lane_capability(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core,
				   struct disp_video_timings *tmgs)
{
	s32 ret = 0;

	/* check if pixclk out of lane's max capability */
	ret = edp_hw_query_max_pixclk(edp_hw, edp_core, tmgs);
	if (ret)
		return RET_FAIL;

	/* check if pixclk out of current lane config's capability */
	ret = edp_hw_query_current_pixclk(edp_core, tmgs);
	if (ret)
		return RET_FAIL;

	/*
	 * some controller's lane has valid link symbol limit per
	 * lane (64 is defined as MAX in DP spec), query if pixclk
	 * out of lane's link symbol limit
	 */
	ret = edp_hw_query_transfer_unit(edp_hw, edp_core, tmgs);
	if (ret)
		return RET_FAIL;

	return RET_OK;
}


static void edp_phy_set_lane_para(struct edp_tx_core *edp_core)
{
	struct edp_lane_para *lane_para = &edp_core->lane_para;
	union phy_configure_opts phy_opts;
	struct phy_configure_opts_dp *dp_opts = &phy_opts.dp;

	memset(dp_opts, 0, sizeof(struct phy_configure_opts_dp));
	dp_opts->link_rate = (u32)(lane_para->bit_rate / 1000000);
	dp_opts->lanes = lane_para->lane_cnt;
	dp_opts->set_rate = 1;
	dp_opts->set_lanes = 1;
	if (edp_core->combo_phy)
		phy_configure(edp_core->combo_phy, &phy_opts);

	if (edp_core->dp_phy)
		phy_configure(edp_core->dp_phy, &phy_opts);
}

static void edp_phy_set_training_para(struct edp_tx_core *edp_core)
{
	struct edp_lane_para *lane_para = &edp_core->lane_para;
	union phy_configure_opts phy_opts;
	struct phy_configure_opts_dp *dp_opts = &phy_opts.dp;
	u32 i = 0;

	memset(dp_opts, 0, sizeof(struct phy_configure_opts_dp));
	for (i = 0; i < lane_para->lane_cnt; i++) {
		dp_opts->voltage[i] = lane_para->lane_sw[i];
		dp_opts->pre[i] = lane_para->lane_pre[i];
	}
	dp_opts->set_voltages = 1;
	dp_opts->lanes = lane_para->lane_cnt;;

	if (edp_core->combo_phy)
		phy_configure(edp_core->combo_phy, &phy_opts);

	if (edp_core->dp_phy)
		phy_configure(edp_core->dp_phy, &phy_opts);
}

s32 edp_get_link_status(struct sunxi_edp_hw_desc *edp_hw, char *link_status)
{
	return edp_hw_aux_read(edp_hw, DPCD_0202H, LINK_STATUS_SIZE, link_status);
}

bool edp_sink_support_fast_training(struct sunxi_edp_hw_desc *edp_hw)
{

	char tmp_rx_buf[16];

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if (edp_hw_aux_read(edp_hw, DPCD_0003H, 1, &tmp_rx_buf[0]) < 0)
		return false;

	if (tmp_rx_buf[0] & DPCD_FAST_TRAIN_MASK)
		return true;
	else
		return false;
}

bool edp_sink_support_tps3(struct sunxi_edp_hw_desc *edp_hw)
{
	char tmp_rx_buf[16];

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if (edp_hw_aux_read(edp_hw, DPCD_0002H, 1, &tmp_rx_buf[0]) < 0)
		return false;

	if (tmp_rx_buf[0] & DPCD_TPS3_SUPPORT_MASK)
		return true;
	else
		return false;
}

s32 edp_hw_lane_remap(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, u32 remap_id)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->lane_remap)
		return ops->lane_remap(edp_hw, lane_id, remap_id);
	else
		return RET_OK;
}

s32 edp_hw_lane_invert(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, bool invert)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->lane_invert)
		return ops->lane_invert(edp_hw, lane_id, invert);
	else
		return RET_OK;
}

s32 edp_hw_set_pixel_mode(struct sunxi_edp_hw_desc *edp_hw, u32 pixel_mode)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->set_pixel_mode)
		return ops->set_pixel_mode(edp_hw, pixel_mode);
	else
		return RET_OK;
}

void edp_hw_set_lane_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;
	struct edp_lane_para *lane_para = &edp_core->lane_para;
	u64 bit_rate = lane_para->bit_rate;
	u32 lane_cnt = lane_para->lane_cnt;
	u32 i = 0;

	if (ops == NULL)
		return;

	if (ops->set_lane_rate)
		ops->set_lane_rate(edp_hw, bit_rate);

	if (ops->set_lane_cnt)
		ops->set_lane_cnt(edp_hw, lane_cnt);

	/* set lane remap */
	for (i = 0 ; i < lane_cnt; i++)
		edp_hw_lane_remap(edp_hw, i, lane_para->lane_remap[i]);

	/* set lane invert */
	for (i = 0 ; i < lane_cnt; i++)
		edp_hw_lane_invert(edp_hw, i, lane_para->lane_invert[i]);
}

void edp_lane_training_para_reset(struct edp_lane_para *lane_para)
{
	lane_para->lane_sw[0] = 0;
	lane_para->lane_sw[1] = 0;
	lane_para->lane_sw[2] = 0;
	lane_para->lane_sw[3] = 0;
	lane_para->lane_pre[0] = 0;
	lane_para->lane_pre[1] = 0;
	lane_para->lane_pre[2] = 0;
	lane_para->lane_pre[3] = 0;
}

void edp_hw_set_training_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;
	u32 sw[4];
	u32 pre[4];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 param_type = edp_core->training_param_type;
	u32 i = 0;

	if (ops == NULL)
		return;

	sw[0] = edp_core->lane_para.lane_sw[0];
	sw[1] = edp_core->lane_para.lane_sw[1];
	sw[2] = edp_core->lane_para.lane_sw[2];
	sw[3] = edp_core->lane_para.lane_sw[3];
	pre[0] = edp_core->lane_para.lane_pre[0];
	pre[1] = edp_core->lane_para.lane_pre[1];
	pre[2] = edp_core->lane_para.lane_pre[2];
	pre[3] = edp_core->lane_para.lane_pre[3];

	/*
	 * not sure if both need set in phy and controller in some platform, so
	 * leave both of these code here
	 */
	if (ops->set_lane_sw_pre) {
		for (i = 0; i < lane_count; i++) {
			EDP_CORE_DBG("set training para: lane[%d] sw:%d pre:%d\n", i, sw[i], pre[i]);
			ops->set_lane_sw_pre(edp_hw, i, sw[i], pre[i], param_type);
		}
	}
}



s32 edp_dpcd_set_lane_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	char tmp_tx_buf[16];
	char tmp_rx_buf[16];
	s32 ret = RET_FAIL;

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));
	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));
	ret = edp_hw_aux_read(edp_hw, DPCD_0101H, 1, &tmp_rx_buf[0]);
	if (ret)
		return ret;

	tmp_tx_buf[0] = edp_core->lane_para.bit_rate / 10000000 / 27;
	tmp_tx_buf[1] = (tmp_rx_buf[0] & ~(DPCD_LANE_CNT_MASK)) |
				edp_core->lane_para.lane_cnt;

	ret = edp_hw_aux_write(edp_hw, DPCD_0100H, 2,  &tmp_tx_buf[0]);

	return ret;
}

s32 edp_dpcd_set_training_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	char tmp_tx_buf[16];
	u32 sw[4];
	u32 pre[4];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane_sw[0];
	sw[1] = edp_core->lane_para.lane_sw[1];
	sw[2] = edp_core->lane_para.lane_sw[2];
	sw[3] = edp_core->lane_para.lane_sw[3];
	pre[0] = edp_core->lane_para.lane_pre[0];
	pre[1] = edp_core->lane_para.lane_pre[1];
	pre[2] = edp_core->lane_para.lane_pre[2];
	pre[3] = edp_core->lane_para.lane_pre[3];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	for (i = 0; i < lane_count; i++) {
		tmp_tx_buf[i] = (pre[i] << DPCD_PLEVEL_SHIFT) | (sw[i] << DPCD_VLEVEL_SHIFT);
		if (sw[i] == MAX_VLEVEL)
			tmp_tx_buf[i] |= DPCD_MAX_VLEVEL_REACHED_FLAG;
		if (pre[i] == MAX_PLEVEL)
			tmp_tx_buf[i] |= DPCD_MAX_PLEVEL_REACHED_FLAG;
	}

	edp_hw_aux_write(edp_hw, DPCD_0103H, lane_count, &tmp_tx_buf[0]);

	return RET_OK;
}

s32 edp_dpcd_set_training_pattern(struct sunxi_edp_hw_desc *edp_hw, u8 pattern)
{
	char tmp_tx_buf[16];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	tmp_tx_buf[0] = pattern;

	if (pattern && (pattern != TRAINING_PATTERN_4))
		tmp_tx_buf[0] |= DPCD_SCRAMBLING_DISABLE_FLAG;

	edp_hw_aux_write(edp_hw, DPCD_0102H, 1, &tmp_tx_buf[0]);

	return RET_OK;
}

s32 edp_training_pattern_clear(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;
	u32 lane_count = edp_core->lane_para.lane_cnt;

	edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_DISABLE, lane_count);
	/* add delay to avoid training pattern writing fail in DPCD in some panels */
	usleep_range(2000, 2500);
	ret = edp_dpcd_set_training_pattern(edp_hw, TRAINING_PATTERN_DISABLE);
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
		eq_fail |= (1 << 8);
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

	sw[0] = edp_core->lane_para.lane_sw[0];
	sw[1] = edp_core->lane_para.lane_sw[1];
	sw[2] = edp_core->lane_para.lane_sw[2];
	sw[3] = edp_core->lane_para.lane_sw[3];

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

	old_sw[0] = lane_para->lane_sw[0];
	old_sw[1] = lane_para->lane_sw[1];
	old_sw[2] = lane_para->lane_sw[2];
	old_sw[3] = lane_para->lane_sw[3];
	old_pre[0] = lane_para->lane_pre[0];
	old_pre[1] = lane_para->lane_pre[1];
	old_pre[2] = lane_para->lane_pre[2];
	old_pre[3] = lane_para->lane_pre[3];

	for (i = 0; i < lane_count; i++) {
		adjust_sw[i] = (link_status[DPCD_LANE_ADJ_OFFSET(i)] & VADJUST_MASK(i)) >> VADJUST_SHIFT(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x, adjust_sw[%d] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_ADJ_OFFSET(i),\
		       link_status[DPCD_LANE_ADJ_OFFSET(i)], i, adjust_sw[i]);

		adjust_pre[i] = (link_status[DPCD_LANE_ADJ_OFFSET(i)] & PADJUST_MASK(i)) >> PADJUST_SHIFT(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x, adjust_pre[%d] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_ADJ_OFFSET(i),\
		       link_status[DPCD_LANE_ADJ_OFFSET(i)], i, adjust_pre[i]);
	}

	lane_para->lane_sw[0] = adjust_sw[0];
	lane_para->lane_sw[1] = adjust_sw[1];
	lane_para->lane_sw[2] = adjust_sw[2];
	lane_para->lane_sw[3] = adjust_sw[3];

	lane_para->lane_pre[0] = adjust_pre[0];
	lane_para->lane_pre[1] = adjust_pre[1];
	lane_para->lane_pre[2] = adjust_pre[2];
	lane_para->lane_pre[3] = adjust_pre[3];

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

s32 edp_link_cr_training(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	char link_status[16];
	char training_info[16];
	u32 try_cnt = 0;
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 change;
	u32 timeout = 0;
	u32 fail_lane;
	u32 i;
	u32 training_info_len = 5;

	edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_1, lane_count);
	/* add delay to avoid training pattern writing fail in DPCD, show more in commits */
	usleep_range(2000, 2500);
	edp_dpcd_set_training_pattern(edp_hw, TRAINING_PATTERN_1);

	memset(training_info, 0, sizeof(training_info));
	edp_hw_aux_read(edp_hw, DPCD_0100H, training_info_len, training_info);
	for (i = 0; i < training_info_len; i++)
		EDP_CORE_DBG("CR_TRAINING: DPCD[0x%x] = 0x%x\n", i + DPCD_0100H, training_info[i]);

	for (try_cnt = 0; try_cnt < TRY_CNT_MAX; try_cnt++) {
		usleep_range(5 * edp_core->interval_CR, 10 * edp_core->interval_CR);

		memset(link_status, 0, sizeof(link_status));
		fail_lane = 0;
		edp_get_link_status(edp_hw, link_status);
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

		/*
		 * FIXME: not sure if it's right in such action:
		 * We do set training para in each training not done, and ignore
		 * if change happens.
		 *
		 * We have meet some cause from customer:
		 * training fail firset time and report the same sw/pre in each
		 * adjustment, and it would never change param in obvious code
		 * logic which finally leads the whole training procedure fail.
		 */
		//if ((change & SW_VOLTAGE_CHANGE_FLAG) || (change & PRE_EMPHASIS_CHANGE_FLAG)) {
			edp_phy_set_training_para(edp_core);
			edp_hw_set_training_para(edp_hw, edp_core);
			edp_dpcd_set_training_para(edp_hw, edp_core);
		//}

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

s32 edp_link_eq_training(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	char link_status[16];
	char training_info[16];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 try_cnt = 0;
	bool change;
	u32 fail_lane;
	u32 i;
	u32 training_info_len = 5;

	if (edp_source_support_tps3(edp_hw) && edp_sink_support_tps3(edp_hw)) {
		edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_3, lane_count);
		/* add delay to avoid training pattern writing fail in DPCD, show more in commits */
		usleep_range(2000, 2500);
		edp_dpcd_set_training_pattern(edp_hw, TRAINING_PATTERN_3);
	} else {
		edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_2, lane_count);
		/* add delay to avoid training pattern writing fail in DPCD, show more in commits */
		usleep_range(2000, 2500);
		edp_dpcd_set_training_pattern(edp_hw, TRAINING_PATTERN_2);
	}

	memset(training_info, 0, sizeof(training_info));
	edp_hw_aux_read(edp_hw, DPCD_0100H, training_info_len, training_info);
	for (i = 0; i < training_info_len; i++)
		EDP_CORE_DBG("EQ_Training: DPCD[0x%x] = 0x%x\n", i + DPCD_0100H, training_info[i]);

	for (try_cnt = 0; try_cnt < TRY_CNT_MAX; try_cnt++) {
		usleep_range(5 * edp_core->interval_EQ, 10 * edp_core->interval_EQ);
		memset(link_status, 0, sizeof(link_status));
		fail_lane = 0;
		edp_get_link_status(edp_hw, link_status);
		if (edp_eq_training_done(link_status, lane_count, &fail_lane)) {
			EDP_CORE_DBG("edp training2 (equalization training) success!\n");
			return RET_OK;
		}

		change = edp_adjust_train_para(link_status, edp_core);

		/*
		 * FIXME: not sure if it's right in such action:
		 * We do set training para in each training not done, and ignore
		 * if change happens.
		 *
		 * We have meet some cause from customer:
		 * training fail firset time and report the same sw/pre in each
		 * adjustment, and it would never change param in obvious code
		 * logic which finally leads the whole training procedure fail.
		 */
		//if ((change & SW_VOLTAGE_CHANGE_FLAG) || (change & PRE_EMPHASIS_CHANGE_FLAG)) {
			edp_phy_set_training_para(edp_core);
			edp_hw_set_training_para(edp_hw, edp_core);
			edp_dpcd_set_training_para(edp_hw, edp_core);
		//}
	}

	EDP_ERR("EQ training result: lane0:%s lane1:%s lane2:%s lane3:%s align:%s\n",
		fail_lane & (1 << 0) ? "FAIL" : "PASS",
		fail_lane & (1 << 1) ? "FAIL" : "PASS",
		fail_lane & (1 << 2) ? "FAIL" : "PASS",
		fail_lane & (1 << 3) ? "FAIL" : "PASS",
		fail_lane & (1 << 8) ? "FAIL" : "PASS");
	EDP_ERR("retry 5 times but still fail, training2(equalization training) fail!\n");
	return RET_FAIL;
}

s32 edp_fast_link_train(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	char link_status[LINK_STATUS_SIZE];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 fail_lane = 0;

	edp_phy_set_lane_para(edp_core);
	edp_hw_set_lane_para(edp_hw, edp_core);

	edp_phy_set_training_para(edp_core);
	edp_hw_set_training_para(edp_hw, edp_core);

	edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_1, lane_count);
	usleep_range(5 * edp_core->interval_CR, 10 * edp_core->interval_CR);

	/* add delay to avoid training pattern writing fail in DPCD, show more in commits */
	usleep_range(2000, 2500);
	if (edp_source_support_tps3(edp_hw) && edp_sink_support_tps3(edp_hw)) {
		edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_3, lane_count);
	} else {
		edp_hw_set_pattern(edp_hw, TRAINING_PATTERN_2, lane_count);
	}
	usleep_range(5 * edp_core->interval_EQ, 10 * edp_core->interval_EQ);

	memset(link_status, 0, sizeof(link_status));
	if (loglevel_debug & 0x2) {
		edp_get_link_status(edp_hw, link_status);
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
			EDP_ERR("EQ training result: lane0:%s lane1:%s lane2:%s lane3:%s align:%s\n",
				fail_lane & (1 << 0) ? "FAIL" : "PASS",
				fail_lane & (1 << 1) ? "FAIL" : "PASS",
				fail_lane & (1 << 2) ? "FAIL" : "PASS",
				fail_lane & (1 << 3) ? "FAIL" : "PASS",
				fail_lane & (1 << 8) ? "FAIL" : "PASS");
			EDP_ERR("edp fast train fail in training2");
			return RET_FAIL;
		}
	}

	return RET_OK;
}

s32 edp_full_link_train(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;

	edp_phy_set_lane_para(edp_core);
	edp_hw_set_lane_para(edp_hw, edp_core);
	ret = edp_dpcd_set_lane_para(edp_hw, edp_core);
	if (ret < 0)
		return ret;

	edp_phy_set_training_para(edp_core);
	edp_hw_set_training_para(edp_hw, edp_core);
	ret = edp_dpcd_set_training_para(edp_hw, edp_core);
	if (ret < 0)
		return ret;

	edp_hw_link_soft_reset(edp_hw);

	ret = edp_link_cr_training(edp_hw, edp_core);
	if (ret < 0)
		return ret;

	ret = edp_link_eq_training(edp_hw, edp_core);
	if (ret < 0)
		return ret;

	return ret;
}


s32 edp_link_training(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	if (edp_source_support_fast_training(edp_hw) \
		&& edp_sink_support_fast_training(edp_hw))
		return edp_fast_link_train(edp_hw, edp_core);
	else
		return edp_full_link_train(edp_hw, edp_core);
}

s32 edp_main_link_setup(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core,
			bool bypass, bool force_level)
{
	s32 ret = 0;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	/*
	 * if need force signal test, should use manaual setting level and
	 * should not reset
	 */
	if (!force_level) {
		/* DP Link CTS need ech training start from level-0 */
		edp_lane_training_para_reset(&edp_core->lane_para);
	}

	// disable scrambling before training, because TP1/2/3 use
	// unscrambled patterns
	edp_hw_scrambling_enable(edp_hw, false);

	if (bypass) {
		/* only set lane para and force output if bypass */
		edp_phy_set_lane_para(edp_core);
		edp_hw_set_lane_para(edp_hw, edp_core);
	} else {
		ret = edp_link_training(edp_hw, edp_core);
		if (ret < 0)
			return ret;
	}

	/* if need force signal test, we force set sw and pre after training */
	if (force_level) {
		edp_phy_set_training_para(edp_core);
		edp_hw_set_training_para(edp_hw, edp_core);
	}

	ret = edp_training_pattern_clear(edp_hw, edp_core);

	edp_hw_scrambling_enable(edp_hw, true);

	return ret;
}

s32 edp_hw_link_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	int ret;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->link_soft_reset) {
		ret = ops->link_soft_reset(edp_hw);
	} else
		ret = RET_OK;
	return ret;
}

s32 edp_hw_video_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	int ret;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->video_soft_reset) {
		ret = ops->video_soft_reset(edp_hw);
	} else
		ret = RET_OK;
	return ret;
}

s32 edp_hw_link_start(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->main_link_start)
		return ops->main_link_start(edp_hw);
	else
		return RET_OK;
}

s32 edp_hw_link_stop(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->main_link_stop)
		return ops->main_link_stop(edp_hw);
	else
		return RET_OK;
}

s32 edp_low_power_en(struct sunxi_edp_hw_desc *edp_hw, bool en)
{
	char tmp_tx_buf[16];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	if (en)
		tmp_tx_buf[0] = DPCD_LOW_POWER_ENTER;
	else
		tmp_tx_buf[0] = DPCD_LOW_POWER_EXIT;

	edp_hw_aux_write(edp_hw, DPCD_0600H, 1, &tmp_tx_buf[0]);

	return RET_OK;
}

void edp_hw_irq_handler(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return;

	if (ops->irq_handle)
		ops->irq_handle(edp_hw, edp_core);
}

s32 edp_hw_irq_enable(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id, bool en)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->irq_enable && ops->irq_disable) {
		if (en)
			return ops->irq_enable(edp_hw, irq_id);
		else
			return ops->irq_disable(edp_hw, irq_id);
	} else
		return RET_OK;
}

s32 edp_hw_aux_read(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 lenth, char *buf)
{
	s32 ret = 0;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->aux_read) {
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP, lowlevel must implement with 7
		 * retry cnt
		 */
		ret = ops->aux_read(edp_hw, addr, lenth, buf);
	} else
		ret = RET_OK;

	if (ret != RET_OK)
		EDP_ERR("edp_aux_read fail, addr:0x%x lenth:0x%x\n", addr, lenth);

	return ret;
}

s32 edp_hw_aux_write(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 lenth, char *buf)
{
	s32 ret = 0;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->aux_write) {
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP, lowlevel must implement with 7
		 * retry cnt
		 */
		ret = ops->aux_write(edp_hw, addr, lenth, buf);
	} else
		ret = RET_OK;

	if (ret != RET_OK)
		EDP_ERR("edp_aux_write fail, addr:0x%x lenth:0x%x\n", addr, lenth);

	return ret;
}

s32 edp_hw_aux_i2c_read(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 lenth, char *buf)
{
	s32 ret = 0;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->aux_i2c_read) {
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP, lowlevel must implement with 7
		 * retry cnt
		 */
		ret = ops->aux_i2c_read(edp_hw, i2c_addr, addr, lenth, buf);
	} else
		ret = RET_OK;

	if (ret != RET_OK)
		EDP_ERR("edp_aux_i2c_read fail, addr:0x%x lenth:0x%x\n", i2c_addr, lenth);

	return ret;
}

s32 edp_hw_aux_i2c_write(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 lenth, char *buf)
{
	s32 ret = 0;
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->aux_i2c_write) {
		/*
		 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP, lowlevel must implement with 7
		 * retry cnt
		 */
		ret = ops->aux_i2c_write(edp_hw, i2c_addr, addr, lenth, buf);
	} else
		ret = RET_OK;

	if (ret != RET_OK)
		EDP_ERR("edp_aux_i2c_write fail, addr:0x%x lenth:0x%x\n", i2c_addr, lenth);

	return ret;
}

void edp_hw_scrambling_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return;

	if (ops->scrambling_enable)
		ops->scrambling_enable(edp_hw, enable);
}


s32 edp_hw_ssc_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->ssc_enable)
		return ops->ssc_enable(edp_hw, enable);
	else
		return RET_OK;
}

bool edp_hw_ssc_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return false;

	if (ops->ssc_is_enabled)
		return ops->ssc_is_enabled(edp_hw);
	else
		return false;
}

s32 edp_hw_ssc_get_mode(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->ssc_get_mode)
		return ops->ssc_get_mode(edp_hw);
	else
		return RET_OK;
}

s32 edp_hw_ssc_set_mode(struct sunxi_edp_hw_desc *edp_hw, u32 mode)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->ssc_set_mode)
		return ops->ssc_set_mode(edp_hw, mode);
	else
		return RET_OK;
}

s32 edp_hw_psr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->psr_enable)
		return ops->psr_enable(edp_hw, enable);
	else
		return RET_OK;
}

bool edp_hw_psr_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->psr_is_enabled)
		return ops->psr_is_enabled(edp_hw);
	else
		return false;
}

s32 edp_hw_assr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	s32 ret = RET_FAIL;
	char tmp_rx_buf[16];
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->assr_enable) {
		ret = ops->assr_enable(edp_hw, enable);
		if (ret)
			return ret;

		memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));
		ret = edp_hw_aux_read(edp_hw, DPCD_010AH, 1, &tmp_rx_buf[0]);
		if (ret)
			return ret;

		if (enable)
			tmp_rx_buf[0] |= DPCD_ASSR_ENABLE_MASK;
		else
			tmp_rx_buf[0] &= ~DPCD_ASSR_ENABLE_MASK;

		ret = edp_hw_aux_write(edp_hw, DPCD_010AH, 1,  &tmp_rx_buf[0]);
	} else
		ret = RET_OK;

	return ret;
}

s32 edp_hw_enhance_frame_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	s32 ret = RET_OK;
	char tmp_rx_buf[16];
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->enhance_frame_enable)
		ops->enhance_frame_enable(edp_hw, enable);

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));
	ret = edp_hw_aux_read(edp_hw, DPCD_0101H, 1, &tmp_rx_buf[0]);
	if (ret)
		return ret;

	if (enable)
		tmp_rx_buf[0] |= DPCD_ENHANCE_FRAME_ENABLE_MASK;
	else
		tmp_rx_buf[0] &= ~DPCD_ENHANCE_FRAME_ENABLE_MASK;

	ret = edp_hw_aux_write(edp_hw, DPCD_0101H, 1,  &tmp_rx_buf[0]);

	return ret;
}

bool edp_hw_check_controller_error(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->check_controller_error)
		return ops->check_controller_error(edp_hw);
	else
		return false;
}

s32 edp_hw_set_pattern(struct sunxi_edp_hw_desc *edp_hw, u32 pattern, u32 lane_cnt)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->set_pattern)
		return ops->set_pattern(edp_hw, pattern, lane_cnt);
	else
		return RET_OK;
}

void edp_hw_set_mst(struct sunxi_edp_hw_desc *edp_hw, u32 mst_cnt)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return;

	if (ops->config_mst)
		ops->config_mst(edp_hw, mst_cnt);
}

s32 edp_hw_get_color_fmt(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->get_color_format)
		return ops->get_color_format(edp_hw);
	else
		return RET_OK;
}

u32 edp_hw_get_pixel_mode(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return 1;

	if (ops->get_pixel_mode)
		return ops->get_pixel_mode(edp_hw);
	else
		return 1;
}

u32 edp_hw_get_pixclk(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_pixel_clk)
		return ops->get_pixel_clk(edp_hw);
	else
		return 0;
}

u32 edp_hw_get_pattern(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_pattern)
		return ops->get_pattern(edp_hw);
	else
		return 0;
}

s32 edp_hw_get_lane_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_lane_para *tmp_lane_para)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->get_lane_para)
		return ops->get_lane_para(edp_hw, tmp_lane_para);
	else {
		memset(tmp_lane_para, 0, sizeof(struct edp_lane_para));
		return RET_OK;
	}
}

u32 edp_hw_get_tu_size(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_tu_size)
		return ops->get_tu_size(edp_hw);
	else
		return 0;
}

u32 edp_hw_get_valid_symbol_per_tu(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_video_ops *ops = edp_hw->video_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_tu_valid_symbol)
		return ops->get_tu_valid_symbol(edp_hw);
	else
		return 0;
}

s32 edp_hw_audio_config(struct sunxi_edp_hw_desc *edp_hw, int interface,
		      int chn_cnt, int data_width, int data_rate)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->audio_config)
		return ops->audio_config(edp_hw, interface,
					 chn_cnt, data_width, data_rate);
	else
		return RET_OK;

}

s32 edp_hw_audio_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->audio_enable)
		return ops->audio_enable(edp_hw);
	else
		return RET_OK;
}

s32 edp_hw_audio_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->audio_disable)
		return ops->audio_disable(edp_hw);
	else
		return RET_OK;
}

s32 edp_hw_audio_mute(struct sunxi_edp_hw_desc *edp_hw,
		      bool enable, int direction)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return RET_OK;

	if (ops->audio_mute)
		return ops->audio_mute(edp_hw, enable, direction);
	else
		return RET_OK;
}

bool edp_hw_audio_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return false;

	if (ops->audio_is_enabled)
		return ops->audio_is_enabled(edp_hw);
	else
		return false;
}

u32 edp_hw_audio_get_max_channel(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_audio_max_channel)
		return ops->get_audio_max_channel(edp_hw);
	else
		return 0;
}

u32 edp_hw_get_audio_if(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_audio_if)
		return ops->get_audio_if(edp_hw);
	else
		return 0;
}

bool edp_hw_audio_is_mute(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return false;

	if (ops->audio_is_muted)
		return ops->audio_is_muted(edp_hw);
	else
		return false;
}

u32 edp_hw_get_audio_chn_cnt(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_audio_chn_cnt)
		return ops->get_audio_chn_cnt(edp_hw);
	else
		return 0;
}

u32 edp_hw_get_audio_data_width(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_audio_ops *ops = edp_hw->audio_ops;

	if (ops == NULL)
		return 0;

	if (ops->get_audio_data_width)
		return ops->get_audio_data_width(edp_hw);

	else
		return 0;
}


s32 sunxi_edp_hw_callback_init(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_hw->video_ops = sunxi_edp_get_hw_video_ops();
	edp_hw->audio_ops = sunxi_edp_get_hw_audio_ops();
	edp_hw->hdcp_ops = sunxi_dp_get_hw_hdcp_ops();

	return RET_OK;
}

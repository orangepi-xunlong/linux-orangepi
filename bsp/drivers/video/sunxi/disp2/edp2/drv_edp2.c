/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * edp2 driver
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "drv_edp2.h"
#include "../disp/disp_sys_intf.h"
#if IS_ENABLED(CONFIG_EXTCON)
#include <linux/version.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include "../drivers/extcon/extcon.h"
#endif

s32 edp_disable(u32 sel);
s32 edp_enable(u32 sel);
s32 edp_sw_enable(u32 sel);

static u32 g_edp_num;
struct edp_info_t g_edp_info[EDP_NUM_MAX];
static struct task_struct *edp_task[EDP_NUM_MAX];

static dev_t devid[EDP_NUM_MAX];
static struct cdev *edp_cdev[EDP_NUM_MAX];

static struct class *edp_class[EDP_NUM_MAX];
static struct device *edp_dev[EDP_NUM_MAX];
u32 loglevel_debug;

static int __parse_dump_str(const char *buf, size_t size,
				unsigned long *start, unsigned long *end)
{
	char *ptr = NULL;
	char *ptr2 = (char *)buf;
	int ret = 0, times = 0;

	/* Support single address mode, some time it haven't ',' */
next:

	/*Default dump only one register(*start =*end).
	If ptr is not NULL, we will cover the default value of end.*/
	if (times == 1)
		*start = *end;

	if (!ptr2 || (ptr2 - buf) >= size)
		goto out;

	ptr = ptr2;
	ptr2 = strnchr(ptr, size - (ptr - buf), ',');
	if (ptr2) {
		*ptr2 = '\0';
		ptr2++;
	}

	ptr = strim(ptr);
	if (!strlen(ptr))
		goto next;

	ret = kstrtoul(ptr, 16, end);
	if (!ret) {
		times++;
		goto next;
	} else
	EDP_ERR("String syntax errors: \"%s\"\n", ptr);

out:
	return ret;
}

irqreturn_t edp_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	int sel = pdev->id;
	s32 ret;

	EDP_DRV_DBG("edp irq handler!\n");
	ret = edp_core_get_hotplug_state(sel);
	if (ret >= 0)
		g_edp_info[sel].hpd_state_now =  ret ? true : false;

	/* TODO: such as aux reply error */

	/* any other special irq event */
	edp_core_irq_handler(sel, &g_edp_info[sel].edp_core);
	return IRQ_HANDLED;
}

s32 edp_hardware_reset(u32 sel)
{
	s32 ret = 0;

	if (gpio_is_valid(g_edp_info[sel].rst_gpio)) {
		gpio_direction_output(g_edp_info[sel].rst_gpio, 0);
		mdelay(5);
		gpio_direction_output(g_edp_info[sel].rst_gpio, 1);
	}

	return ret;
}


/**
 * @name       :edp_clk_enable
 * @brief      :enable or disable edp clk
 * @param[IN]  :sel index of edp
 * @param[IN]  :en 1:enable , 0 disable
 * @return     :0 if success
 */
static s32 edp_clk_enable(u32 sel, bool en)
{
	s32 ret = -1;

	if (en) {
		if (!IS_ERR_OR_NULL(g_edp_info[sel].rst_bus)) {
			ret = reset_control_deassert(g_edp_info[sel].rst_bus);
			if (ret) {
				EDP_ERR("deassert reset failed\n");
				return ret;
			}
		}

		if (!IS_ERR_OR_NULL(g_edp_info[sel].clk_bus)) {
			ret = clk_prepare_enable(g_edp_info[sel].clk_bus);
			if (ret) {
				EDP_ERR("enable clk_bus failed\n");
				return ret;
			}
		}

		if (!IS_ERR_OR_NULL(g_edp_info[sel].clk)) {
			ret = clk_prepare_enable(g_edp_info[sel].clk);
			if (ret) {
				EDP_ERR("enable clk failed\n");
				return ret;
			}
		}

		if (!IS_ERR_OR_NULL(g_edp_info[sel].clk_24m)) {
			ret = clk_prepare_enable(g_edp_info[sel].clk_24m);
			if (ret) {
				EDP_ERR("enable clk_24m failed\n");
				return ret;
			}
		}
	} else {
		if (!IS_ERR_OR_NULL(g_edp_info[sel].clk_24m))
			clk_disable_unprepare(g_edp_info[sel].clk_24m);

		if (!IS_ERR_OR_NULL(g_edp_info[sel].clk))
			clk_disable_unprepare(g_edp_info[sel].clk);

		if (!IS_ERR_OR_NULL(g_edp_info[sel].clk_bus))
			clk_disable_unprepare(g_edp_info[sel].clk_bus);

		if (!IS_ERR_OR_NULL(g_edp_info[sel].rst_bus)) {
			ret = reset_control_assert(g_edp_info[sel].rst_bus);
			if (ret) {
				EDP_ERR("assert reset failed\n");
				return ret;
			}
		}
	}

	return ret;
}

void edp_update_capacity(u32 sel)
{
	struct edp_tx_core *edp_core;
	struct edp_rx_cap *sink_cap;
	struct edp_tx_cap *src_cap;
	struct edp_lane_para *lane_para;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->lane_para;
	sink_cap = &g_edp_info[sel].sink_cap;
	src_cap = &g_edp_info[sel].source_cap;

	if (edp_core->dpcd_parsed) {
		lane_para->lane_cnt = min(src_cap->max_lane, sink_cap->max_lane);
		lane_para->bit_rate = min(src_cap->max_rate, sink_cap->max_rate);
	}
}

void sink_cap_reset(u32 sel)
{
	struct edp_rx_cap *sink_cap;

	sink_cap = &g_edp_info[sel].sink_cap;

	memset(sink_cap, 0, sizeof(struct edp_rx_cap));
}


s32 edid_to_sink_info(u32 sel, struct edid *edid)
{
	struct edp_rx_cap *sink_cap;
	u8 *cea;
	s32 start, end;

	sink_cap = &g_edp_info[sel].sink_cap;

	sink_cap->mfg_week = edid->mfg_week;
	sink_cap->mfg_year = edid->mfg_year;
	sink_cap->edid_ver = edid->version;
	sink_cap->edid_rev = edid->revision;

	/*14H: edid input info*/
	sink_cap->input_type = (edid->input & SUNXI_EDID_INPUT_DIGITAL) >> SUNXI_EDID_INPUT_SHIFT;

	/*digital input*/
	if (sink_cap->input_type == 1) {
		switch (edid->input & SUNXI_EDID_DIGITAL_DEPTH_MASK) {
		case SUNXI_EDID_DIGITAL_DEPTH_6:
			sink_cap->bit_depth = 6;
			break;
		case SUNXI_EDID_DIGITAL_DEPTH_8:
			sink_cap->bit_depth = 8;
			break;
		case SUNXI_EDID_DIGITAL_DEPTH_10:
			sink_cap->bit_depth = 10;
			break;
		case SUNXI_EDID_DIGITAL_DEPTH_12:
			sink_cap->bit_depth = 12;
			break;
		case SUNXI_EDID_DIGITAL_DEPTH_14:
			sink_cap->bit_depth = 14;
			break;
		case SUNXI_EDID_DIGITAL_DEPTH_16:
			sink_cap->bit_depth = 16;
			break;
		case SUNXI_EDID_DIGITAL_DEPTH_UNDEF:
		default:
			sink_cap->bit_depth = 0;
			break;
		}

		sink_cap->video_interface = edid->input & SUNXI_EDID_DIGITAL_TYPE_MASK;

		switch (edid->features & SUNXI_EDID_COLOR_FMT_MASK) {
		case SUNXI_EDID_COLOR_FMT_YCC444:
			sink_cap->Ycc444_support = true;
			sink_cap->Ycc422_support = false;
			break;
		case SUNXI_EDID_COLOR_FMT_YCC422:
			sink_cap->Ycc444_support = false;
			sink_cap->Ycc422_support = true;
			break;
		case SUNXI_EDID_COLOR_FMT_YCC444_422:
			sink_cap->Ycc444_support = true;
			sink_cap->Ycc422_support = true;
			break;
		default:
			sink_cap->Ycc444_support = false;
			sink_cap->Ycc422_support = false;
			break;
		}
	}


	sink_cap->width_cm = edid->width_cm;
	sink_cap->height_cm = edid->height_cm;

	cea = edp_edid_cea_extension(edid);

	if (cea) {
		if (edp_edid_cea_tag(cea) != CEA_EXT) {
			EDP_ERR("wrong CEA tag\n");
			return -EOPNOTSUPP;
		}

		if (edp_edid_cea_revision(cea) < 3) {
			EDP_ERR("wrong CEA revision\n");
			return -EOPNOTSUPP;
		}

		if (edp_edid_cea_db_offsets(cea, &start, &end)) {
			EDP_ERR("invalid data block offsets\n");
			return -EPROTO;
		}

		if (cea[3] & SUNXI_CEA_BASIC_AUDIO_MASK)
			sink_cap->audio_support = true;
		else
			sink_cap->audio_support = false;

		if (cea[3] & SUNXI_CEA_YCC444_MASK)
			sink_cap->Ycc444_support = true;
		else
			sink_cap->Ycc444_support = false;

		if (cea[3] & SUNXI_CEA_YCC422_MASK)
			sink_cap->Ycc422_support = true;
		else
			sink_cap->Ycc422_support = false;
	} else
		EDP_DRV_DBG("no CEA Extension found\n");

	return RET_OK;
}


void edid_to_prefer_timings(u32 sel, struct edid *edid)
{
	struct detailed_timing *dt = edid->detailed_timings;
	struct detailed_pixel_timing *tmgs = &dt->data.pixel_data;
	struct disp_video_timings *edid_tmgs;
	struct edp_tx_core *edp_core;
	u32 hactive, vactive, hblank, vblank;
	u32 hsync_offset, hsync_pulse_width, vsync_offset, vsync_pulse_width;
	u32 fps;

	hactive = (tmgs->hactive_hblank_hi & 0xf0) << 4 | tmgs->hactive_lo;
	vactive = (tmgs->vactive_vblank_hi & 0xf0) << 4 | tmgs->vactive_lo;
	hblank = (tmgs->hactive_hblank_hi & 0xf) << 8 | tmgs->hblank_lo;
	vblank = (tmgs->vactive_vblank_hi & 0xf) << 8 | tmgs->vblank_lo;
	hsync_offset = (tmgs->hsync_vsync_offset_pulse_width_hi & 0xc0) << 2 | tmgs->hsync_offset_lo;
	hsync_pulse_width = (tmgs->hsync_vsync_offset_pulse_width_hi & 0x30) << 4 | tmgs->hsync_pulse_width_lo;
	vsync_offset = (tmgs->hsync_vsync_offset_pulse_width_hi & 0xc) << 2 | tmgs->vsync_offset_pulse_width_lo >> 4;
	vsync_pulse_width = (tmgs->hsync_vsync_offset_pulse_width_hi & 0x3) << 4 | (tmgs->vsync_offset_pulse_width_lo & 0xf);

	edp_core = &g_edp_info[sel].edp_core;
	edid_tmgs = &edp_core->edid_timings;

	edid_tmgs->vic = 0;
	edid_tmgs->pixel_clk = le16_to_cpu(dt->pixel_clock) * 10000;
	edid_tmgs->pixel_repeat = 0;
	edid_tmgs->x_res = hactive;
	edid_tmgs->y_res = vactive;
	edid_tmgs->hor_total_time = hactive + hblank;
	edid_tmgs->hor_back_porch = hblank - hsync_offset - hsync_pulse_width;
	edid_tmgs->hor_front_porch = hsync_offset;
	edid_tmgs->hor_sync_time = hsync_pulse_width;
	edid_tmgs->ver_total_time = vactive + vblank;
	edid_tmgs->ver_back_porch = vblank - vsync_offset - vsync_pulse_width;
	edid_tmgs->ver_front_porch = vsync_offset;
	edid_tmgs->ver_sync_time = vsync_pulse_width;

	fps = (edid_tmgs->pixel_clk / edid_tmgs->hor_total_time) / edid_tmgs->ver_total_time;
	/* FIXME: try to limit fps for some senior panel support 90/120hz */
	/* calculation result may excueed 60 such as 61 or 62, so use 65 */
	if (fps > 65 && g_edp_info[sel].fps_limit_60) {
		EDP_INFO("eDP/DP screen's timings:[%dx%d@%dHz], limit to [%dx%d@60Hz] now!\n",
			 edid_tmgs->x_res, edid_tmgs->y_res, fps, edid_tmgs->x_res, edid_tmgs->y_res);
		edid_tmgs->pixel_clk = edid_tmgs->hor_total_time * edid_tmgs->ver_total_time * 60;
	}

	switch (tmgs->misc & SUNXI_EDID_MISC_HPOLAR_MASK) {
	case SUNXI_EDID_MISC_HPOLAR_NEG:
		edid_tmgs->hor_sync_polarity = 0;
		break;
	case SUNXI_EDID_MISC_HPOLAR_POS:
	default:
		edid_tmgs->hor_sync_polarity = 1;
		break;
	}

	switch (tmgs->misc & SUNXI_EDID_MISC_VPOLAR_MASK) {
	case SUNXI_EDID_MISC_VPOLAR_NEG:
		edid_tmgs->ver_sync_polarity = 0;
		break;
	case SUNXI_EDID_MISC_VPOLAR_POS:
	default:
		edid_tmgs->ver_sync_polarity = 1;
		break;
	}

	if (tmgs->misc & SUNXI_EDID_MISC_INTERLACE_MASK)
		edid_tmgs->b_interlace = true;
	else
		edid_tmgs->b_interlace = false;

	edid_tmgs->vactive_space = 0;
	edid_tmgs->trd_mode = 0;

	edid_tmgs->tv_mode = timing_to_standard_mode(edid_tmgs);

	edp_core->edid_timings_parsed = true;
}

s32 edp_parse_edid(u32 sel, struct edid *edid)
{
	s32 ret;
	ret = edid_to_sink_info(sel, edid);
	if (ret < 0)
		return ret;

	edid_to_prefer_timings(sel, edid);

	return ret;
}

void edp_update_timing_from_edid(u32 sel)
{
	struct edp_tx_core *edp_core;

	edp_core = &g_edp_info[sel].edp_core;

	/*set timings by edid*/
	if (edp_core->edid_timings_parsed) {
		memcpy(&edp_core->timings, &edp_core->edid_timings,
		       sizeof(struct disp_video_timings));

		mutex_lock(&g_edp_info[sel].mlock);
		g_edp_info[sel].use_def_timings = false;
		g_edp_info[sel].use_edid_timings = true;
		g_edp_info[sel].edp_core.mode = DISP_TV_MODE_NUM;
		mutex_unlock(&g_edp_info[sel].mlock);
	}
}

s32 edp_misc_parse(struct platform_device *pdev)
{
	u32 sel = pdev->id;
	s32 value = 0;
	s32 ret = -1;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;

	ret = of_property_read_u32(pdev->dev.of_node, "edid_timings_prefer", &value);
	if (!ret)
		g_edp_info[sel].edid_timings_prefer = value ? true : false;
	else
		g_edp_info[sel].edid_timings_prefer = false;

	ret = of_property_read_u32(pdev->dev.of_node, "sink_capacity_prefer", &value);
	if (!ret)
		g_edp_info[sel].sink_capacity_prefer = value ? true : false;
	else
		g_edp_info[sel].sink_capacity_prefer = false;

	ret = of_property_read_u32(pdev->dev.of_node, "timings_fixed", &value);
	if (!ret)
		g_edp_info[sel].timings_fixed = value ? true : false;
	else
		g_edp_info[sel].timings_fixed = false;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_training_param_type", &value);
	if (!ret) {
		if (value >= 3)
			EDP_WRN("edp_training_param_type out of range!\n");
		else
			g_edp_info[sel].edp_core.training_param_type = value;
	}

	/* FIXME: limit some senior panel support 90/120 hz, default enable. we seem can not handle
	 * dual screen with different fps well */
	ret = of_property_read_u32(pdev->dev.of_node, "fps_limit_60", &value);
	if (!ret)
		g_edp_info[sel].fps_limit_60 = value ? true : false;
	else
		g_edp_info[sel].fps_limit_60 = true;

	edp_debug->bypass_training = 0;

	return RET_OK;
}

s32 edp_phy_cfg_parse(struct platform_device *pdev)
{
	s32 ret = -1;
	s32  value = 1;
	u32 sel = pdev->id;
	struct edp_tx_core *edp_core;

	edp_core = &g_edp_info[sel].edp_core;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_ssc_en", &value);
	if (!ret)
		edp_core->ssc_en = value;
	else
		edp_core->ssc_en = 0;

	/*
	 * ssc modulation mode select
	 * 0: center mode
	 * 1: downspread mode
	 *
	 * */
	ret = of_property_read_u32(pdev->dev.of_node, "edp_ssc_mode", &value);
	if (!ret) {
		if ((value != 0) && (value != 1)) {
			EDP_WRN("Out of range, select from: 0-center_mode 1-downspread_mode\n");
			edp_core->ssc_mode = 0;
		} else
			edp_core->ssc_mode = value;
	} else
		edp_core->ssc_mode = 0;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_psr_support", &value);
	if (!ret)
		edp_core->psr_en = value;
	else
		edp_core->psr_en = 0;

	return RET_OK;
}


s32 edp_lane_para_parse(struct platform_device *pdev)
{
	s32 ret = -1;
	s32 value = 1;
	u32 sel = pdev->id;
	struct edp_tx_core *edp_core;
	struct edp_lane_para *def_lane_para;
	struct edp_lane_para *lane_para;

	edp_core = &g_edp_info[sel].edp_core;
	def_lane_para = &edp_core->def_lane_para;
	lane_para = &edp_core->lane_para;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_lane_rate", &value);
	if (!ret) {
		switch (value) {
		case 0:
			def_lane_para->bit_rate = BIT_RATE_1G62;
			break;
		case 1:
			def_lane_para->bit_rate = BIT_RATE_2G7;
			break;
		case 2:
			def_lane_para->bit_rate = BIT_RATE_5G4;
			break;
		default:
			EDP_ERR("edp_rate out of range!\n");
			return RET_FAIL;
		}
	}

	ret = of_property_read_u32(pdev->dev.of_node, "edp_lane_cnt", &value);
	if (!ret) {
		if ((value <= 0) || (value == 3) || (value > 4)) {
			EDP_ERR("edp_lane_cnt out of range!\n");
			return RET_FAIL;
		}
		def_lane_para->lane_cnt = value;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "edp_colordepth", &value);
	if (!ret)
		def_lane_para->colordepth = value;
	else
		/* default use 8-bit */
		def_lane_para->colordepth = 8;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_color_fmt", &value);
	if (!ret)
		def_lane_para->color_fmt = value;
	else
		/* default use RGB */
		def_lane_para->color_fmt = DISP_CSC_TYPE_RGB;

	if (def_lane_para->color_fmt == DISP_CSC_TYPE_RGB)
		def_lane_para->bpp = 3 * def_lane_para->colordepth;
	else if (def_lane_para->color_fmt == DISP_CSC_TYPE_YUV444)
		def_lane_para->bpp = 3 * def_lane_para->colordepth;
	else if (def_lane_para->color_fmt == DISP_CSC_TYPE_YUV422)
		def_lane_para->bpp = 2 * def_lane_para->colordepth;
	else if (def_lane_para->color_fmt == DISP_CSC_TYPE_YUV420)
		def_lane_para->bpp = 3 * def_lane_para->colordepth / 2;

	memset(lane_para, 0, sizeof(struct edp_lane_para));
	memcpy(lane_para, def_lane_para, sizeof(struct edp_lane_para));

	return RET_OK;
}

s32 edp_default_timming_parse(struct platform_device *pdev)
{
	s32 ret = -1;
	s32 value = 1;
	s32 fps = 0;
	u32 sel = pdev->id;
	struct edp_tx_core *edp_core;
	struct disp_video_timings *def_timings;

	edp_core = &g_edp_info[sel].edp_core;
	def_timings = &edp_core->def_timings;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_x", &value);
	if (!ret)
		def_timings->x_res = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_hbp", &value);
	if (!ret)
		def_timings->hor_back_porch = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_hfp", &value);
	if (!ret)
		def_timings->hor_front_porch = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_ht", &value);
	if (!ret)
		def_timings->hor_total_time = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_hspw", &value);
	if (!ret)
		def_timings->hor_sync_time = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_y", &value);
	if (!ret)
		def_timings->y_res = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_vt", &value);
	if (!ret)
		def_timings->ver_total_time = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_vspw", &value);
	if (!ret)
		def_timings->ver_sync_time = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_vbp", &value);
	if (!ret)
		def_timings->ver_back_porch = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_vfp", &value);
	if (!ret)
		def_timings->ver_front_porch = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_hpolar", &value);
	if (!ret)
		def_timings->hor_sync_polarity = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_vpolar", &value);
	if (!ret)
		def_timings->ver_sync_polarity = value;

	ret = of_property_read_u32(pdev->dev.of_node, "edp_fps", &value);
	if (!ret)
		fps = value;

	def_timings->pixel_clk = def_timings->hor_total_time * \
		def_timings->ver_total_time * fps;

	memcpy(&edp_core->timings, &edp_core->def_timings,
	       sizeof(struct disp_video_timings));

	mutex_lock(&g_edp_info[sel].mlock);
	g_edp_info[sel].use_def_timings = true;
	g_edp_info[sel].use_edid_timings = false;
	g_edp_info[sel].edp_core.mode = DISP_TV_MODE_NUM;
	mutex_unlock(&g_edp_info[sel].mlock);

	return RET_OK;
}

static bool disp_is_edp_boot(void)
{
	u32 output_type0, output_mode0;
	u32 output_type1, output_mode1;
	u32 value;

	value = disp_boot_para_parse("boot_disp");
	output_type0 = (value >> 8) & 0xff;
	output_mode0 = (value)&0xff;

	output_type1 = (value >> 24) & 0xff;
	output_mode1 = (value >> 16) & 0xff;

	if ((output_type0 == DISP_OUTPUT_TYPE_EDP) ||
	    (output_type1 == DISP_OUTPUT_TYPE_EDP))
		return true;
	else
		return false;
}


s32 edp_get_mode(u32 sel)
{
	return g_edp_info[sel].edp_core.mode;
}

s32 edp_get_mode_support(u32 sel, enum disp_tv_mode mode)
{
	struct edp_tx_core *edp_core = &g_edp_info[sel].edp_core;

	return edp_mode_support(sel, mode, edp_core);
}

s32 edp_set_mode(u32 sel, enum disp_tv_mode mode)
{
	s32 ret = 0;

	if (g_edp_info[sel].timings_fixed) {
		EDP_INFO("edp timing is fixed, can not be modify\n");
		return RET_OK;
	}

	if (mode >= DISP_TV_MODE_NUM) {
		EDP_ERR("video mode %d is out of support range!\n", mode);
		return RET_FAIL;
	}

	ret = edp_get_mode_support(sel, mode);
	if (ret) {
		EDP_ERR("video mode %d is not support yet!\n", mode);
		return RET_FAIL;
	}

	EDP_DRV_DBG("edp%d mode: %d\n", sel, mode);

	mutex_lock(&g_edp_info[sel].mlock);
	g_edp_info[sel].edp_core.pre_mode = edp_get_mode(sel);
	g_edp_info[sel].edp_core.mode = mode;
	mutex_unlock(&g_edp_info[sel].mlock);
	standard_mode_to_timing(g_edp_info[sel].edp_core.mode,
				     &g_edp_info[sel].edp_core.timings);

	mutex_lock(&g_edp_info[sel].mlock);
	g_edp_info[sel].use_def_timings = false;
	g_edp_info[sel].use_edid_timings = false;
	mutex_unlock(&g_edp_info[sel].mlock);

	return  RET_OK;
}

/**
 * @name       edp_get_video_timing_info
 * @brief      get timing info
 * @param[IN]  sel:index of edp module
 * @param[OUT] video_info:timing info
 * @return     0 if success
 */
static s32 edp_get_video_timing_info(u32 sel,
				      struct disp_video_timings **video_info)
{
	s32 ret = 0;

	*video_info = &g_edp_info[sel].edp_core.timings;
	return ret;
}


s32 edp_read_dpcd(u32 sel, char *dpcd_rx_buf)
{
	u32 i = 0;
	u32 block = 16;
	s32 ret = 0;

	for (i = 0; i < 576 / block; i++) {
		ret = edp_core_aux_read(sel, DPCD_0000H + i * block,
				block, (char *)(dpcd_rx_buf) + (i * block));
		if (ret < 0)
			return ret;
	}

	return 0;
}

s32 edp_read_dpcd_extended(u32 sel, char *dpcd_ext_rx_buf)
{
	return edp_core_aux_read(sel, DPCD_2200H, 16, dpcd_ext_rx_buf);
}

void edp_parse_dpcd(u32 sel, char *dpcd_rx_buf)
{
	struct edp_rx_cap *sink_cap;
	struct edp_tx_core *edp_core;

	sink_cap = &g_edp_info[sel].sink_cap;
	edp_core = &g_edp_info[sel].edp_core;

	/*
	 * Sometimes aux return all 0 data when edp panel/DisplayPort
	 * is not connect, although the aux didn't return fail. We pick
	 * to most important dpcd register as judgement because they will
	 * never set to 0 in DP spec
	 */
	if ((dpcd_rx_buf[0] == 0x00) || dpcd_rx_buf[1] == 0x00) {
		EDP_ERR("dpcd read all 0, read dpcd fail!\n");
		return;
	}

	sink_cap->dpcd_rev = 10 + ((dpcd_rx_buf[0] >> 4) & 0x0f);
	EDP_DRV_DBG("DPCD version:1.%d\n", sink_cap->dpcd_rev % 10);

	if (dpcd_rx_buf[1] == 0x06) {
		sink_cap->max_rate = BIT_RATE_1G62;
		EDP_DRV_DBG("sink max bit rate:1.62Gbps\n");
	} else if (dpcd_rx_buf[1] == 0x0a) {
		sink_cap->max_rate = BIT_RATE_2G7;
		EDP_DRV_DBG("sink max bit rate:2.7Gbps\n");
	} else if (dpcd_rx_buf[1] == 0x14) {
		sink_cap->max_rate = BIT_RATE_5G4;
		EDP_DRV_DBG("sink max bit rate:5.4Gbps\n");
	} else if (dpcd_rx_buf[1] == 0x1e) {
		sink_cap->max_rate = BIT_RATE_8G1;
		EDP_DRV_DBG("sink max bit rate:8.1Gbps\n");
	}

	sink_cap->max_lane = dpcd_rx_buf[2] & EDP_DPCD_MAX_LANE_MASK;
	EDP_DRV_DBG("sink max lane count:%d\n", sink_cap->max_lane);

	if (dpcd_rx_buf[2] & EDP_DPCD_ENHANCE_FRAME_MASK) {
		sink_cap->enhance_frame_support = true;
		EDP_DRV_DBG("enhanced mode:support\n");
	} else {
		sink_cap->enhance_frame_support = false;
		EDP_DRV_DBG("enhanced mode:not support\n");
	}

	if (dpcd_rx_buf[2] & EDP_DPCD_TPS3_MASK) {
		sink_cap->tps3_support = true;
		EDP_DRV_DBG("TPS3: support\n");
	} else {
		sink_cap->tps3_support = false;
		EDP_DRV_DBG("TPS3: not support\n");
	}

	if (dpcd_rx_buf[3] & EDP_DPCD_FAST_TRAIN_MASK) {
		sink_cap->fast_train_support = true;
		EDP_DRV_DBG("fast training: support\n");
	} else {
		sink_cap->fast_train_support = false;
		EDP_DRV_DBG("fast training: not support\n");
	}

	if (dpcd_rx_buf[5] & EDP_DPCD_DOWNSTREAM_PORT_MASK) {
		sink_cap->downstream_port_support = true;
		EDP_DRV_DBG("downstream port:present\n");
	} else {
		sink_cap->downstream_port_support = false;
		EDP_DRV_DBG("downstream port:not present\n");
	}

	sink_cap->downstream_port_type = (dpcd_rx_buf[5] & EDP_DPCD_DOWNSTREAM_PORT_TYPE_MASK);

	sink_cap->downstream_port_cnt = (dpcd_rx_buf[7] & EDP_DPCD_DOWNSTREAM_PORT_CNT_MASK);

	if (dpcd_rx_buf[8] & EDP_DPCD_LOCAL_EDID_MASK) {
		sink_cap->local_edid_support = true;
		EDP_DRV_DBG("ReceiverPort0 Capability_0:Has a local EDID\n");
	} else {
		sink_cap->local_edid_support = false;
		EDP_DRV_DBG("ReceiverPort0 Capability_0:Not has a local EDID\n");
	}

	/* eDP_CONFIGURATION_CAP */
	/* Always reads 0x00 for external receivers */
	if (dpcd_rx_buf[0x0d] != 0) {
		sink_cap->is_edp_device = true;
		EDP_DRV_DBG("Sink device is eDP receiver!\n");
		if (dpcd_rx_buf[0x0d] & EDP_DPCD_ASSR_MASK)
			sink_cap->assr_support = true;
		else
			sink_cap->assr_support = false;

		if (dpcd_rx_buf[0x0d] & EDP_DPCD_FRAME_CHANGE_MASK)
			sink_cap->enhance_frame_support = true;
		else
			sink_cap->enhance_frame_support = false;

	} else {
		sink_cap->is_edp_device = false;
		EDP_DRV_DBG("Sink device is external receiver!\n");
	}

	switch (dpcd_rx_buf[0x0e]) {
	case 0x00:
		/*Link Status/Adjust Request read interval during CR*/
		/*phase --- 100us*/
		edp_core->interval_CR = 100;
		/*Link Status/Adjust Request read interval during EQ*/
		/*phase --- 400us*/
		edp_core->interval_EQ = 400;

		break;
	case 0x01:
		edp_core->interval_CR = 4000;
		edp_core->interval_EQ = 4000;
		break;
	case 0x02:
		edp_core->interval_CR = 8000;
		edp_core->interval_EQ = 8000;
		break;
	case 0x03:
		edp_core->interval_CR = 12000;
		edp_core->interval_EQ = 12000;
		break;
	case 0x04:
		edp_core->interval_CR = 16000;
		edp_core->interval_EQ = 16000;
		break;
	default:
		edp_core->interval_CR = 100;
		edp_core->interval_EQ = 400;
	}

	edp_core->dpcd_parsed = true;
}

#if IS_ENABLED(CONFIG_EXTCON)
static struct extcon_dev *extcon_edp[EDP_NUM_MAX];
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 20, 0)
#define EXTCON_EDP_TYPE EXTCON_DISP_DP
#else
#define EXTCON_EDP_TYPE EXTCON_DISP_EDP
#endif

#define EXTCON_DP_TYPE EXTCON_DISP_DP

static char edp_extcon_name[20];
static const u32 edp_cable[] = {
	EXTCON_EDP_TYPE,
	EXTCON_NONE,
};

static const u32 dp_cable[] = {
	EXTCON_DP_TYPE,
	EXTCON_NONE,
};

s32 edp_report_hpd_work(u32 sel, u32 hpd)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;

	if (g_edp_info[sel].hpd_state == hpd)
		return -1;

	switch (hpd) {

	case EDP_HPD_PLUGIN:
		EDP_DRV_DBG("set EDP EXTCON to 1\n");
		if (desc->type == EDP_OUTPUT)
			extcon_set_state_sync(extcon_edp[sel], EXTCON_EDP_TYPE,
					      EDP_HPD_PLUGIN);
		else
			extcon_set_state_sync(extcon_edp[sel], EXTCON_DP_TYPE,
				      EDP_HPD_PLUGIN);

		break;
	case EDP_HPD_PLUGOUT:
	default:
		EDP_DRV_DBG("set EDP EXTCON to 0\n");
		if (desc->type == EDP_OUTPUT)
			extcon_set_state_sync(extcon_edp[sel], EXTCON_EDP_TYPE,
					      EDP_HPD_PLUGOUT);
		else
			extcon_set_state_sync(extcon_edp[sel], EXTCON_DP_TYPE,
					      EDP_HPD_PLUGOUT);
		break;
	}
	return RET_OK;
}

#else
s32 edp_report_hpd_work(u32 sel, u32 hpd)
{
	EDP_WRN("You need to enable CONFIG_EXTCON!\n");
	return RET_OK;
}
#endif

static s32 edp_init(struct platform_device *pdev)
{
	s32 ret = -1;
	u32 sel = pdev->id;
	bool boot_edp;

	EDP_DRV_DBG("start edp init\n");

	mutex_init(&g_edp_info[sel].mlock);

	ret = edp_misc_parse(pdev);
	if (ret != 0)
		goto OUT;

	ret = edp_phy_cfg_parse(pdev);
	if (ret != 0)
		goto OUT;

	ret = edp_lane_para_parse(pdev);
	if (ret != 0)
		goto OUT;

	ret = edp_default_timming_parse(pdev);
	if (ret != 0)
		goto OUT;

	ret = edp_core_init_early(sel);
	if (ret != 0)
		goto OUT;

	boot_edp = disp_is_edp_boot();

	if (boot_edp)
		g_edp_info[sel].enable = 1;
OUT:
	EDP_DRV_DBG("end of edp init:%d\n", ret);
	return ret;
}

void edp_show_builtin_patten(u32 sel, u32 patten)
{
	edp_core_show_builtin_patten(sel, patten);
}

u32 edp_get_cur_line(u32 sel)
{
	u32 ret = 0;

	ret = edp_core_get_cur_line(sel);
	return ret;
}

s32 edp_get_start_delay(u32 sel)
{
	s32 ret = -1;

	ret = edp_core_get_start_dly(sel);
	return ret;
}

s32 edp_irq_enable(u32 sel, u32 irq_id, u32 en)
{
	return edp_core_irq_enable(sel, irq_id, (bool)en);
}

s32 edp_irq_query(u32 sel)
{
	s32 ret = -1;

	ret = edp_core_irq_query(sel);
	if (ret == 1)
		ret = edp_core_irq_clear(sel);
	return ret;
}

s32 edp_resource_init(struct platform_device *pdev)
{
	u32 clk_index = 0;
	s32 ret = 0;

	pdev->id = of_alias_get_id(pdev->dev.of_node, "edp");
	if (pdev->id < 0) {
		EDP_DEV_ERR(&pdev->dev, "failed to get alias id\n");
		ret = RET_FAIL;
		goto OUT;
	}

	g_edp_info[pdev->id].base_addr =
	    (uintptr_t __force)of_iomap(pdev->dev.of_node, 0);
	if (!g_edp_info[pdev->id].base_addr) {
		EDP_DEV_ERR(&pdev->dev, "fail to get addr for edp%d!\n", pdev->id);
		ret = RET_FAIL;
		goto OUT;
	}

	edp_core_set_reg_base(pdev->id, g_edp_info[pdev->id].base_addr);


	g_edp_info[pdev->id].irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!g_edp_info[pdev->id].irq) {
		EDP_DEV_ERR(&pdev->dev, "Parse irq fail!\n");
		ret = RET_FAIL;
		goto ERR_IOMAP;
	}

	g_edp_info[pdev->id].clk_bus = of_clk_get(pdev->dev.of_node, clk_index);
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].clk_bus)) {
		EDP_DEV_ERR(&pdev->dev, "fail to get clk bus for edp%d!\n", pdev->id);
		ret = RET_FAIL;
		goto ERR_IOMAP;
	}

	clk_index++;
	g_edp_info[pdev->id].clk = of_clk_get(pdev->dev.of_node, clk_index);
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].clk)) {
		EDP_DEV_ERR(&pdev->dev, "fail to get clk for edp%d!\n", pdev->id);
		ret = RET_FAIL;
		goto ERR_CLK_BUS;
	}
	clk_index++;

	g_edp_info[pdev->id].clk_24m = of_clk_get(pdev->dev.of_node, clk_index);
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].clk_24m)) {
		EDP_DRV_DBG("24M clock for edp%d is not need or missing!\n", pdev->id);
	}

	g_edp_info[pdev->id].rst_bus = devm_reset_control_get(&pdev->dev, "rst_bus_edp");
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].rst_bus)) {
		EDP_DEV_ERR(&pdev->dev, "fail to get rst for edp%d!\n", pdev->id);
		ret = RET_FAIL;
		goto ERR_CLK;
	}

	/*parse power resource*/
	g_edp_info[pdev->id].vdd_regulator = regulator_get(&pdev->dev, "vdd-edp");
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].vdd_regulator))
		EDP_DRV_DBG("vdd_edp is null or no need!\n");

	g_edp_info[pdev->id].vcc_regulator = regulator_get(&pdev->dev, "vcc-edp");
	if (IS_ERR_OR_NULL(g_edp_info[pdev->id].vcc_regulator))
		EDP_DRV_DBG("vcc_edp is null or no need!\n");

	g_edp_info[pdev->id].rst_gpio = of_get_named_gpio(pdev->dev.of_node, "edp_hw_reset_pin", 0);;
	if (!gpio_is_valid(g_edp_info[pdev->id].rst_gpio))
		EDP_DRV_DBG("edp reset pin is null or no need!\n");
	else {
		ret = devm_gpio_request(&pdev->dev, g_edp_info[pdev->id].rst_gpio, "edp_hw_reset_pin");

		gpio_direction_output(g_edp_info[pdev->id].rst_gpio, 1);

		g_edp_info[pdev->id].edp_core.rst_gpio = g_edp_info[pdev->id].rst_gpio;
	}

	g_edp_info[pdev->id].dev = &pdev->dev;
	pdev->dev.id = pdev->id;

	return RET_OK;

ERR_CLK:
	if (g_edp_info[pdev->id].clk_24m)
		clk_put(g_edp_info[pdev->id].clk_24m);
	clk_put(g_edp_info[pdev->id].clk);
ERR_CLK_BUS:
	clk_put(g_edp_info[pdev->id].clk_bus);
ERR_IOMAP:
	if (g_edp_info[pdev->id].base_addr)
		iounmap((char __iomem *)g_edp_info[pdev->id].base_addr);
OUT:
	return ret;

}

static ssize_t dpcd_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	int i;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];

	if (!g_edp_info[sel].hpd_state) {
		count += sprintf(buf + count, "[EDP] error: sink is plugout!\n");
		return count;
	}

	if (dpcd_rx_buf == NULL) {
		count += sprintf(buf + count, "[EDP] error: dpcd read uncorrectly!\n");
		return count;
	}

	for (i = 0; i < sizeof(g_edp_info[sel].dpcd_rx_buf); i++) {
		if ((i % 0x10) == 0)
			count += sprintf(buf + count, "\n%02x:", i);
		count += sprintf(buf + count, "  %02x", g_edp_info[sel].dpcd_rx_buf[i]);
	}

	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t resource_lock_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;

	edp_debug->edp_res_lock = simple_strtoul(buf, NULL, 0);

	return count;
}

static ssize_t resource_lock_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;

	count += sprintf(buf + count, "resource_lock = %d\n", edp_debug->edp_res_lock);

	return count;
}

static ssize_t bypass_training_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;

	edp_debug->bypass_training = simple_strtoul(buf, NULL, 0);

	return count;
}

static ssize_t bypass_training_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;

	count += sprintf(buf + count, "bypass_training = %d\n", edp_debug->bypass_training);

	return count;
}

static ssize_t loglevel_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	loglevel_debug = simple_strtoul(buf, NULL, 0);

	return count;
}

static ssize_t loglevel_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	if (!loglevel_debug)
		count += sprintf(buf + count, "0:NONE  1:EDP_DRV  2:EDP_CORE  4:EDP_LOW  8:EDP_EDID\n");
	else
		count += sprintf(buf + count, "loglevel_debug = %d\n", loglevel_debug);

	return count;
}

static ssize_t lane_debug_en_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	struct edp_lane_para *debug_lane_para;
	struct edp_lane_para *backup_lane_para;
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->lane_para;
	debug_lane_para = &edp_core->debug_lane_para;
	backup_lane_para = &edp_core->backup_lane_para;

	edp_debug = &g_edp_info[sel].edp_debug;


	if (!strncmp(buf, "1", 1)) {
		/* only the debug_en turns 0 to 1 should restore lane config */
		if (edp_debug->lane_debug_en != 1) {
			edp_debug->lane_debug_en = 1;
			memcpy(backup_lane_para, lane_para, sizeof(struct edp_lane_para));
		}
		memcpy(lane_para, debug_lane_para, sizeof(struct edp_lane_para));
		g_edp_info[sel].use_debug_para = true;
	} else if (!strncmp(buf, "0", 1)) {
		if (edp_debug->lane_debug_en == 1) {
			edp_debug->lane_debug_en = 0;
			memcpy(lane_para, backup_lane_para, sizeof(struct edp_lane_para));
			memset(backup_lane_para, 0, sizeof(struct edp_lane_para));
		} else {
			/* return if debug_en is already 0 */
			return count;
		}
	} else {
		EDP_WRN("Syntax error, only '0' or '1' support!\n");
		return count;
	}

	edp_disable(sel);

	mdelay(50);

	edp_enable(sel);

	return count;
}

static ssize_t lane_debug_en_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	u32 count = 0;

	edp_debug = &g_edp_info[sel].edp_debug;

	count += sprintf(buf + count, "lane_debug_en: %d\n", edp_debug->lane_debug_en);

	return count;
}

static ssize_t lane_fmt_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	u32 sel = pdev->id;
	u8 *separator = NULL;
	u32 color_fmt = 0;
	u32 color_bits = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->debug_lane_para;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		EDP_WRN("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		color_fmt = simple_strtoul(buf, NULL, 0);
		color_bits = simple_strtoul(separator + 1, NULL, 0);

		if (color_fmt > 2) {
			EDP_WRN("color format should select from: 0:RGB  1:YUV444  2:YUV422\n");
			return count;
		}

		if ((color_bits != 6) && (color_bits != 8) && \
		    (color_bits != 10) && (color_bits != 12) && (color_bits != 16)) {
			EDP_WRN("lane rate should select from: 6/8/10/12/16\n");
			return count;
		}

		lane_para->color_fmt = color_fmt;
		lane_para->colordepth = color_bits;
	}

	return count;
}

static ssize_t lane_fmt_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	count += sprintf(buf + count, "echo [coolor_fmt color_bits] > lane_fmt_debug\n");
	count += sprintf(buf + count, "eg: echo 0 8 > lane_fmt_debug\n");

	return count;
}

static ssize_t lane_cfg_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	u32 sel = pdev->id;
	u8 *separator = NULL;
	u32 lane_cnt = 0;
	u32 lane_rate = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->debug_lane_para;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		EDP_WRN("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		lane_cnt = simple_strtoul(buf, NULL, 0);
		lane_rate = simple_strtoul(separator + 1, NULL, 0);

		if ((lane_cnt != 1) && (lane_cnt != 2) && (lane_cnt != 4)) {
			EDP_WRN("lane cnt should select from: 1/2/4\n");
			return count;
		}

		if ((lane_rate != 162) && (lane_rate != 270) && (lane_rate != 540)) {
			EDP_WRN("lane rate should select from: 162/270/540\n");
			return count;
		}

		lane_para->lane_cnt = lane_cnt;
		lane_para->bit_rate = (u64)lane_rate * 10000000;
	}

	return count;
}

static ssize_t lane_cfg_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	count += sprintf(buf + count, "echo [lane_cnt lane_rate] > lane_cfg_debug\n");
	count += sprintf(buf + count, "eg: echo 4 270 > lane_cfg_debug\n");

	return count;
}

static ssize_t lane_sw_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	u32 sel = pdev->id;
	u8 *separator = NULL;
	u32 lane_num = 0;
	u32 sw_level = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->debug_lane_para;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		EDP_WRN("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		lane_num = simple_strtoul(buf, NULL, 0);
		sw_level = simple_strtoul(separator + 1, NULL, 0);

		if (lane_num > 3)
			EDP_WRN("lane num should select from: 0/1/2/3\n");

		if (sw_level > 3)
			EDP_WRN("swing voltage should select from: 0/1/2/3\n");

		if (lane_num == 0)
			lane_para->lane0_sw = sw_level;
		else if (lane_num == 1)
			lane_para->lane1_sw = sw_level;
		else if (lane_num == 2)
			lane_para->lane2_sw = sw_level;
		else if (lane_num == 3)
			lane_para->lane3_sw = sw_level;
		else
			EDP_WRN("Syntax error!\n");

	}

	return count;
}

static ssize_t lane_sw_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	count += sprintf(buf + count, "echo [lane_num sw_level] > lane_sw_debug\n");
	count += sprintf(buf + count, "eg: echo 1 2 > lane_sw_debug\n");

	return count;
}

static ssize_t lane_pre_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	u32 sel = pdev->id;
	u8 *separator = NULL;
	u32 lane_num = 0;
	u32 pre_level = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->debug_lane_para;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		EDP_WRN("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		lane_num = simple_strtoul(buf, NULL, 0);
		pre_level = simple_strtoul(separator + 1, NULL, 0);

		if (lane_num > 3)
			EDP_WRN("lane num should select from: 0/1/2/3\n");

		if (pre_level > 3)
			EDP_WRN("pre emphasis level should select from: 0/1/2/3\n");

		if (lane_num == 0)
			lane_para->lane0_pre = pre_level;
		else if (lane_num == 1)
			lane_para->lane1_pre = pre_level;
		else if (lane_num == 2)
			lane_para->lane2_pre = pre_level;
		else if (lane_num == 3)
			lane_para->lane3_pre = pre_level;
		else
			EDP_WRN("Syntax error!\n");

	}

	return count;
}

static ssize_t lane_pre_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	count += sprintf(buf + count, "echo [lane_num pre_level] > lane_pre_debug\n");
	count += sprintf(buf + count, "eg: echo 1 2 > lane_pre_debug\n");

	return count;
}

static ssize_t edid_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct edp_tx_core *edp_core = NULL;
	struct edid *edid = NULL;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	if (!strncmp(buf, "1", 1)) {
		edp_core = &g_edp_info[sel].edp_core;

		edid = edp_edid_get(sel);
		if (edid == NULL) {
			EDP_WRN("fail to read edid\n");
			return count;
		}

		edp_parse_edid(sel, edid);
		//edp_update_timing_from_edid(sel);
	} else {
		EDP_WRN("syntax error, try 'echo 1 > edid'!\n");
	}
	edp_core->edid = edid;

	return count;
}

static ssize_t edid_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct edid *edid;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u8 *cea;
	u32 count = 0;
	s32 i, edid_lenth;

	if (!g_edp_info[sel].hpd_state) {
		count += sprintf(buf + count, "[EDP] error: sink is plugout!\n");
		return count;
	}

	edp_core = &g_edp_info[sel].edp_core;
	edid = edp_core->edid;

	if (!edid) {
		count += sprintf(buf + count, "[EDP] error: edid read uncorrectly!\
				 Try to read edid manaually by 'echo 1 > edid'\n");
		return count;
	}

	cea = edp_edid_cea_extension(edid);
	if (!cea)
		edid_lenth = 128;
	 else
		edid_lenth = 256;


	for (i = 0; i < edid_lenth; i++) {
		if ((i % 0x8) == 0)
			count += sprintf(buf + count, "\n%02x:", i);
		count += sprintf(buf + count, "  %02x", *((char *)(edid) + i));
	}

	count += sprintf(buf + count, "\n");

	return count;
}


static ssize_t hotplug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	if (g_edp_info[sel].hpd_state)
		count += sprintf(buf + count, "HPD State: Plugin\n");
	else
		count += sprintf(buf + count, "HPD State: Plugout\n");

	return count;
}

static ssize_t sink_info_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_rx_cap *sink_cap;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;

	if (!g_edp_info[sel].hpd_state) {
		count += sprintf(buf + count, "[EDP] error: sink is plugout!\n");
		return count;
	}

	sink_cap = &g_edp_info[sel].sink_cap;

	if (!sink_cap) {
		count += sprintf(buf + count, "[EDP] eDP sink capacity unknown\n");
		return count;
	}

	count += sprintf(buf + count, "[Capacity Info]\n");
	count += sprintf(buf + count, "dpcd_rev: %d\n", sink_cap->dpcd_rev);
	count += sprintf(buf + count, "is_edp_device: %s\n", sink_cap->is_edp_device ? "Yes" : "No");
	count += sprintf(buf + count, "max_bit_rate: %lld\n", sink_cap->max_rate);
	count += sprintf(buf + count, "max_lane_cnt: %d\n", sink_cap->max_lane);
	count += sprintf(buf + count, "tps3_support: %s\n", sink_cap->tps3_support ? "Yes" : "No");
	count += sprintf(buf + count, "fast_link_train_support: %s\n", sink_cap->fast_train_support ? "Yes" : "No");
	count += sprintf(buf + count, "downstream_port_support: %s\n", sink_cap->downstream_port_support ? "Yes" : "No");

	if (sink_cap->downstream_port_support) {
		switch (sink_cap->downstream_port_type) {
		case 0:
			count += sprintf(buf + count, "downstream_port_type: DP\n");
			break;
		case 1:
			count += sprintf(buf + count, "downstream_port_type: VGA\n");
			break;
		case 2:
			count += sprintf(buf + count, "downstream_port_type: DVI/HDMI/DP++\n");
			break;
		case 3:
		default:
			count += sprintf(buf + count, "downstream_port_type: others\n");
			break;
		}

		count += sprintf(buf + count, "downstream_port_cnt: %d\n", sink_cap->downstream_port_cnt);
	} else {
		count += sprintf(buf + count, "downstream_port_type: NULL\n");
		count += sprintf(buf + count, "downstream_port_cnt: NULL\n");
	}

	count += sprintf(buf + count, "local_edid_support: %s\n", sink_cap->local_edid_support ? "Yes" : "No");
	count += sprintf(buf + count, "assr_support: %s\n", sink_cap->assr_support ? "Yes" : "No");
	count += sprintf(buf + count, "enhance_frame_support: %s\n", sink_cap->enhance_frame_support ? "Yes" : "No");
	count += sprintf(buf + count, "\n");

	/*edid info*/
	count += sprintf(buf + count, "[Edid Info]\n");
	count += sprintf(buf + count, "mfg_year: %d\n", sink_cap->mfg_year + 1990);
	count += sprintf(buf + count, "mfg_week: %d\n", sink_cap->mfg_week);
	count += sprintf(buf + count, "edid_ver: %d\n", sink_cap->edid_ver);
	count += sprintf(buf + count, "edid_rev: %d\n", sink_cap->edid_rev);
	count += sprintf(buf + count, "width_cm: %d\n", sink_cap->width_cm);
	count += sprintf(buf + count, "height_cm: %d\n", sink_cap->height_cm);
	count += sprintf(buf + count, "input_type: %s\n", sink_cap->input_type ? "Digital" : "Analog");
	count += sprintf(buf + count, "input_bit_depth: %d\n", sink_cap->bit_depth);

	switch (sink_cap->video_interface) {
	case SUNXI_EDID_DIGITAL_TYPE_UNDEF:
		count += sprintf(buf + count, "sink_video_interface: Undefined\n");
		break;
	case SUNXI_EDID_DIGITAL_TYPE_DVI:
		count += sprintf(buf + count, "sink_video_interface: DVI\n");
		break;
	case SUNXI_EDID_DIGITAL_TYPE_HDMI_A:
		count += sprintf(buf + count, "sink_video_interface: HDMIa\n");
		break;
	case SUNXI_EDID_DIGITAL_TYPE_HDMI_B:
		count += sprintf(buf + count, "sink_video_interface: HDMIb\n");
		break;
	case SUNXI_EDID_DIGITAL_TYPE_MDDI:
		count += sprintf(buf + count, "sink_video_interface: MDDI\n");
		break;
	case SUNXI_EDID_DIGITAL_TYPE_DP:
		count += sprintf(buf + count, "sink_video_interface: DP/eDP\n");
		break;
	}

	count += sprintf(buf + count, "Ycc444_support: %s\n", sink_cap->Ycc444_support ? "Yes" : "No");
	count += sprintf(buf + count, "Ycc422_support: %s\n", sink_cap->Ycc422_support ? "Yes" : "No");
	count += sprintf(buf + count, "Ycc420_support: %s\n", sink_cap->Ycc420_support ? "Yes" : "No");
	count += sprintf(buf + count, "audio_support: %s\n", sink_cap->audio_support ? "Yes" : "No");

	return count;
}

static ssize_t source_info_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct edp_tx_cap *src_cap;
	struct edp_lane_para tmp_lane_para;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	u32 valid_symbol;
	char color_fmt[10];

	edp_core = &g_edp_info[sel].edp_core;
	src_cap = &g_edp_info[sel].source_cap;

	count += sprintf(buf + count, "[Capacity Info]\n");
	count += sprintf(buf + count, "Max lane Support: %d\n",\
			 src_cap->max_lane);
	count += sprintf(buf + count, "Max Rate Support: %lld\n",\
			 src_cap->max_rate);
	count += sprintf(buf + count, "TPS3 Support: %s\n",\
			 src_cap->tps3_support ? "Yes" : "No");
	count += sprintf(buf + count, "Fast Train Support: %s\n",\
			 src_cap->fast_train_support ? "Yes" : "No");
	count += sprintf(buf + count, "Audio Support: %s\n",\
			 src_cap->audio_support ? "Yes" : "No");
	count += sprintf(buf + count, "SSC Support: %s\n",\
			 src_cap->ssc_support ? "Yes" : "No");
	count += sprintf(buf + count, "ASSR Support: %s\n",\
			 src_cap->assr_support ? "Yes" : "No");
	count += sprintf(buf + count, "PSR Support: %s\n",\
			 src_cap->psr_support ? "Yes" : "No");
	count += sprintf(buf + count, "MST Support: %s\n\n",\
			 src_cap->mst_support ? "Yes" : "No");

	edp_core_get_lane_para(sel, &tmp_lane_para);

	count += sprintf(buf + count, "[Link Info]\n");
	count += sprintf(buf + count, "Bit Rate: %lld\n",\
			 tmp_lane_para.bit_rate);
	count += sprintf(buf + count, "Lane Count: %d\n",\
			 tmp_lane_para.lane_cnt);
	count += sprintf(buf + count, "Pixel Clock: %d\n",\
			 edp_core_get_pixclk(sel));
	count += sprintf(buf + count, "TU Size: %d\n",\
			 edp_core_get_tu_size(sel));
	valid_symbol = edp_core_get_valid_symbol_per_tu(sel);
	count += sprintf(buf + count, "Valid Symbol Per TU: %d.%d\n",\
			 valid_symbol / 10, valid_symbol % 10);

	switch (edp_core_get_color_fmt(sel)) {
	case RGB_6BIT:
		snprintf(color_fmt, sizeof(color_fmt), "RGB_6BIT");
		break;
	case RGB_8BIT:
		snprintf(color_fmt, sizeof(color_fmt), "RGB_8BIT");
		break;
	case RGB_10BIT:
		snprintf(color_fmt, sizeof(color_fmt), "RGB_10BIT");
		break;
	case RGB_12BIT:
		snprintf(color_fmt, sizeof(color_fmt), "RGB_12BIT");
		break;
	case RGB_16BIT:
		snprintf(color_fmt, sizeof(color_fmt), "RGB_16BIT");
		break;
	case YCBCR444_8BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR444_8BIT");
		break;
	case YCBCR444_10BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR444_10BIT");
		break;
	case YCBCR444_12BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR444_12BIT");
		break;
	case YCBCR444_16BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR444_16BIT");
		break;
	case YCBCR422_8BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR422_8BIT");
		break;
	case YCBCR422_10BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR422_10BIT");
		break;
	case YCBCR422_12BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR422_12BIT");
		break;
	case YCBCR422_16BIT:
		snprintf(color_fmt, sizeof(color_fmt), "YCBCR422_16BIT");
		break;
	default:
		snprintf(color_fmt, sizeof(color_fmt), "Unknown");
		break;
	}
	count += sprintf(buf + count, "Color Format: %s\n", color_fmt);

	if (edp_core->ssc_en) {
		count += sprintf(buf + count, "Ssc En: %s\n",\
			edp_core_ssc_is_enabled(sel) ? "Enable" : "Disable");
		if (edp_core_ssc_is_enabled(sel))
			count += sprintf(buf + count, "Ssc Mode: %s\n", \
				 edp_core_ssc_get_mode(sel) ? "Downspread" : "Center");
	}

	if (edp_core->psr_en)
		count += sprintf(buf + count, "Psr En: %s\n",\
				 edp_core_psr_is_enabled(sel) ? "Enable" : "Disable");

	count += sprintf(buf + count, "\n");

	count += sprintf(buf + count, "[Training Info]\n");
	count += sprintf(buf + count, "Voltage Swing0: Level-%d\n",\
			 tmp_lane_para.lane0_sw);
	count += sprintf(buf + count, "Pre Emphasis0:  Level-%d\n",\
			 tmp_lane_para.lane0_pre);
	count += sprintf(buf + count, "Voltage Swing1: Level-%d\n",\
			 tmp_lane_para.lane1_sw);
	count += sprintf(buf + count, "Pre Emphasis1:  Level-%d\n",\
			 tmp_lane_para.lane1_pre);
	count += sprintf(buf + count, "Voltage Swing2: Level-%d\n",\
			 tmp_lane_para.lane2_sw);
	count += sprintf(buf + count, "Pre Emphasis2:  Level-%d\n",\
			 tmp_lane_para.lane2_pre);
	count += sprintf(buf + count, "Voltage Swing3: Level-%d\n",\
			 tmp_lane_para.lane3_sw);
	count += sprintf(buf + count, "Pre Emphasis3:  Level-%d\n",\
			 tmp_lane_para.lane3_pre);

	count += sprintf(buf + count, "\n");

	count += sprintf(buf + count, "[Audio Info]\n");
	if (!edp_core_audio_is_enabled(sel)) {
		count += sprintf(buf + count, "Audio En: Disable\n");
		return count;
	}

	count += sprintf(buf + count, "Audio En: Enable\n");
	count += sprintf(buf + count, "Audio Interface: %s\n", edp_core_get_audio_if(sel) ?\
				 "I2S" : "SPDIF");
	count += sprintf(buf + count, "Mute: %s\n", edp_core_audio_is_mute(sel) ?\
				 "Yes" : "No");
	count += sprintf(buf + count, "Channel Count: %d\n",\
			 edp_core_get_audio_chn_cnt(sel));
	count += sprintf(buf + count, "Data Width: %d bits\n",\
			 edp_core_get_audio_date_width(sel));

	return count;
}

static ssize_t aux_read_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	u32 i, times = 0, show_cnt = 0;
	unsigned long start_reg = 0;
	unsigned long end_reg = 0;
	unsigned long len;
	char tmp_rx_buf[256];

	edp_debug = &g_edp_info[sel].edp_debug;

	if ((edp_debug->aux_read_start == 0) \
		 && (edp_debug->aux_read_end) == 0)
		return sprintf(buf, "%s\n", "echo [0x(start_addr), 0x(end_addr)] > aux_read");


	start_reg = edp_debug->aux_read_start;
	end_reg = edp_debug->aux_read_end;

	if (end_reg < start_reg) {
		return sprintf(buf, "%s,%d err, addresss syntax error!\n", __func__, __LINE__);
	}

	if (start_reg > 0x70000) {
		return sprintf(buf, "%s,%d err, addresss out of range define in eDP spec!\n", __func__, __LINE__);
	}

	len = end_reg - start_reg;
	if (len > 256)
		return sprintf(buf, "%s,%d err, out of length, length should <= 256!\n", __func__, __LINE__);

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	for (i = 0; i < (1+ (len / 16)); i++) {
		if (edp_core_aux_read(sel, (start_reg + (i * 16)), 16, &tmp_rx_buf[i * 16]) < 0) {
			return sprintf(buf, "aux read fail!\n");
		}
	}

	show_cnt += sprintf(buf, "[AUX_READ] Addr:0x%04lx   Lenth:%ld", start_reg, len + 1);
	if ((start_reg % 0x8) == 0)
		times = 1;

	for (i = 0; i < (len + 1); i++) {
		if ((times == 0) && (((start_reg + i) % 0x8) != 0)) {
			show_cnt += sprintf(buf + show_cnt, "\n0x%04lx:", (start_reg + i));
			times = 1;
		}

		if (((start_reg + i) % 0x8) == 0)
			show_cnt += sprintf(buf + show_cnt, "\n0x%04lx:", (start_reg + i));
		show_cnt += sprintf(buf + show_cnt, "  0x%02x", tmp_rx_buf[i]);
	}

	show_cnt += sprintf(buf + show_cnt, "\n");

	return show_cnt;


}

static ssize_t aux_read_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	unsigned long start_reg = 0;
	unsigned long end_reg = 0;

	if (__parse_dump_str(buf, count, &start_reg, &end_reg)) {
		EDP_WRN("%s,%d err, invalid para!\n", __func__, __LINE__);
		return count;
	}

	if (end_reg < start_reg) {
		EDP_WRN("%s,%d err, end address should larger than start address!\n", __func__, __LINE__);
		return count;
	}

	if (start_reg > 0x70000) {
		EDP_WRN("%s,%d err, addresss out of range define in eDP spec!\n", __func__, __LINE__);
		return count;
	}

	edp_debug = &g_edp_info[sel].edp_debug;

	edp_debug->aux_read_start = start_reg;
	edp_debug->aux_read_end = end_reg;

	return count;
}

static ssize_t aux_write_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	char tmp_rx_buf[16];
	u8 regval_after[16], i;
	u32 show_cnt = 0;

	edp_debug = &g_edp_info[sel].edp_debug;

	if (edp_debug->aux_write_len == 0)
		return sprintf(buf, "%s\n", "echo [0x(address) 0x(value)] > aux_write");

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));
	edp_core_aux_read(sel, edp_debug->aux_write_start, edp_debug->aux_write_len, tmp_rx_buf);
	show_cnt += sprintf(buf, "[AUX_WRITE]  Lenth:%d\n", edp_debug->aux_write_len);

	for (i = 0; i < edp_debug->aux_write_len; i++) {
		regval_after[i] = tmp_rx_buf[i];

		show_cnt += sprintf(buf + show_cnt, \
			    "[0x%02x]:val_before:0x%02x  val_wants:0x%02x  val_after:0x%02x\n", \
				edp_debug->aux_write_start + i, edp_debug->aux_write_val_before[i], \
				edp_debug->aux_write_val[i], regval_after[i]);
	}

	return show_cnt;

}

static ssize_t aux_write_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	u8 value = 0, i = 0;
	u8 *separator = NULL;
	char tmp_tx_buf[16];
	char tmp_rx_buf[16];
	u32 reg_addr = 0;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		EDP_WRN("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		edp_debug = &g_edp_info[sel].edp_debug;

		memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));
		memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));
		reg_addr = simple_strtoul(buf, NULL, 0);
		edp_debug->aux_write_start = reg_addr;
		edp_debug->aux_write_len = 0;

		for (i = 0; i < 16; i++) {
			value = simple_strtoul(separator + 1, NULL, 0);

			tmp_tx_buf[i] = value;

			edp_debug->aux_write_val[i] = value;
			edp_debug->aux_write_len++;

			separator = strchr(separator + 1, ' ');
			if (separator == NULL)
				break;
		}

		edp_core_aux_read(sel, reg_addr, edp_debug->aux_write_len, tmp_rx_buf);
		for (i = 0; i < edp_debug->aux_write_len; i++) {
			edp_debug->aux_write_val_before[i] = tmp_rx_buf[i];
		}

		mdelay(1);
		edp_core_aux_write(sel, reg_addr, edp_debug->aux_write_len, &tmp_tx_buf[0]);

	}

	return count;

}

static ssize_t aux_i2c_read_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	u32 i, show_cnt = 0;
	unsigned long i2c_addr = 0;
	unsigned long len;
	char tmp_rx_buf[256];

	edp_debug = &g_edp_info[sel].edp_debug;

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if ((edp_debug->aux_i2c_addr == 0) || (edp_debug->aux_i2c_len) == 0)
		return sprintf(buf, "%s\n", "echo [0x(i2c_addr), 0x(lenth)] > aux_i2c_read");

	i2c_addr = edp_debug->aux_i2c_addr;
	len = edp_debug->aux_i2c_len;

#if IS_ENABLED(CONFIG_DISP_PHY_INNO_EDP_1_3) || \
	IS_ENABLED(CONFIG_DISP_PHY_INNO_EDP_1_3_MODULE)
	edp_core_aux_i2c_write(sel, i2c_addr, 1, &tmp_rx_buf[0]);
#endif

	show_cnt += sprintf(buf, "[AUX_I2C_READ] I2C_Addr:0x%04lx   Lenth:%ld", i2c_addr, len);

	for (i = 0; i < (1+ ((len - 1) / 16)); i++) {
		if (edp_core_aux_i2c_read(sel, i2c_addr, 16, &tmp_rx_buf[i * 16]) < 0) {
			return sprintf(buf, "aux_i2c_read fail!\n");
		}
	}

	for (i = 0; i < len; i++) {
		if ((i % 0x8) == 0)
			show_cnt += sprintf(buf + show_cnt, "\n0x%02x:", i);
		show_cnt += sprintf(buf + show_cnt, "  0x%02x", tmp_rx_buf[i]);
	}


	show_cnt += sprintf(buf + show_cnt, "\n");

	return show_cnt;


}

static ssize_t aux_i2c_read_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;
	unsigned long i2c_addr = 0;
	unsigned long lenth = 0;

	if (__parse_dump_str(buf, count, &i2c_addr, &lenth)) {
		EDP_WRN("%s err, invalid param, line:%d!\n", __func__, __LINE__);
		return count;
	}

	if ((lenth <= 0) || (lenth > 256)) {
		EDP_WRN("%s aux i2c read lenth should between 0 and 256!\n", __func__);
		return count;
	}

	edp_debug = &g_edp_info[sel].edp_debug;

	edp_debug->aux_i2c_addr = i2c_addr;
	edp_debug->aux_i2c_len = lenth;

	return count;
}

static ssize_t lane_config_now_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->lane_para;

	if (g_edp_info[sel].use_debug_para)
		count += sprintf(buf + count, "lane_para_use: debug\n");
	else
		count += sprintf(buf + count, "lane_para_use: default\n");

	count += sprintf(buf + count, "bit_rate:    %lld\n", lane_para->bit_rate);
	count += sprintf(buf + count, "lane_cnt:    %d\n", lane_para->lane_cnt);
	count += sprintf(buf + count, "color depth: %d\n", lane_para->colordepth);
	count += sprintf(buf + count, "color fmt:   %d\n", lane_para->color_fmt);
	count += sprintf(buf + count, "lane0_sw:    %d\n", lane_para->lane0_sw);
	count += sprintf(buf + count, "lane1_sw:    %d\n", lane_para->lane1_sw);
	count += sprintf(buf + count, "lane2_sw:    %d\n", lane_para->lane2_sw);
	count += sprintf(buf + count, "lane3_sw:    %d\n", lane_para->lane3_sw);
	count += sprintf(buf + count, "lane0_pre:   %d\n", lane_para->lane0_pre);
	count += sprintf(buf + count, "lane1_pre:   %d\n", lane_para->lane1_pre);
	count += sprintf(buf + count, "lane2_pre:   %d\n", lane_para->lane2_pre);
	count += sprintf(buf + count, "lane3_pre:   %d\n", lane_para->lane3_pre);
	return count;
}

static ssize_t lane_config_def_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->def_lane_para;

	count += sprintf(buf + count, "bit_rate:    %lld\n", lane_para->bit_rate);
	count += sprintf(buf + count, "lane_cnt:    %d\n", lane_para->lane_cnt);
	count += sprintf(buf + count, "color depth: %d\n", lane_para->colordepth);
	count += sprintf(buf + count, "color fmt:   %d\n", lane_para->color_fmt);
	count += sprintf(buf + count, "lane0_sw:    %d\n", lane_para->lane0_sw);
	count += sprintf(buf + count, "lane1_sw:    %d\n", lane_para->lane1_sw);
	count += sprintf(buf + count, "lane2_sw:    %d\n", lane_para->lane2_sw);
	count += sprintf(buf + count, "lane3_sw:    %d\n", lane_para->lane3_sw);
	count += sprintf(buf + count, "lane0_pre:   %d\n", lane_para->lane0_pre);
	count += sprintf(buf + count, "lane1_pre:   %d\n", lane_para->lane1_pre);
	count += sprintf(buf + count, "lane2_pre:   %d\n", lane_para->lane2_pre);
	count += sprintf(buf + count, "lane3_pre:   %d\n", lane_para->lane3_pre);

	return count;
}

static ssize_t lane_config_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct edp_lane_para *lane_para;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;

	edp_core = &g_edp_info[sel].edp_core;
	lane_para = &edp_core->debug_lane_para;

	count += sprintf(buf + count, "bit_rate:    %lld\n", lane_para->bit_rate);
	count += sprintf(buf + count, "lane_cnt:    %d\n", lane_para->lane_cnt);
	count += sprintf(buf + count, "color depth: %d\n", lane_para->colordepth);
	count += sprintf(buf + count, "color fmt:   %d\n", lane_para->color_fmt);
	count += sprintf(buf + count, "lane0_sw:    %d\n", lane_para->lane0_sw);
	count += sprintf(buf + count, "lane1_sw:    %d\n", lane_para->lane1_sw);
	count += sprintf(buf + count, "lane2_sw:    %d\n", lane_para->lane2_sw);
	count += sprintf(buf + count, "lane3_sw:    %d\n", lane_para->lane3_sw);
	count += sprintf(buf + count, "lane0_pre:   %d\n", lane_para->lane0_pre);
	count += sprintf(buf + count, "lane1_pre:   %d\n", lane_para->lane1_pre);
	count += sprintf(buf + count, "lane2_pre:   %d\n", lane_para->lane2_pre);
	count += sprintf(buf + count, "lane3_pre:   %d\n", lane_para->lane3_pre);

	return count;
}

static ssize_t timings_now_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct disp_video_timings *timings;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	u32 fps;

	edp_core = &g_edp_info[sel].edp_core;
	timings = &edp_core->timings;

	if (g_edp_info[sel].use_def_timings)
		count += sprintf(buf + count, "[timing_use: Default]\n");
	else if (g_edp_info[sel].use_edid_timings)
		count += sprintf(buf + count, "[timing_use: Edid]\n");
	else
		count += sprintf(buf + count, "[timing_use: unkonw]\n");


	fps = (timings->pixel_clk / timings->hor_total_time) / timings->ver_total_time;
	count += sprintf(buf + count, "fps: %d\n", fps);
	count += sprintf(buf + count, "vic: %d\n", timings->vic);
	count += sprintf(buf + count, "tv_mode: %d\n", timings->tv_mode);
	count += sprintf(buf + count, "pixel_clk: %d\n", timings->pixel_clk);
	count += sprintf(buf + count, "pixel_repeat: %d\n", timings->pixel_repeat);
	count += sprintf(buf + count, "x_res: %d\n", timings->x_res);
	count += sprintf(buf + count, "y_res: %d\n", timings->y_res);
	count += sprintf(buf + count, "hor_total_time: %d\n", timings->hor_total_time);
	count += sprintf(buf + count, "hor_back_porch: %d\n", timings->hor_back_porch);
	count += sprintf(buf + count, "hor_front_porch: %d\n", timings->hor_front_porch);
	count += sprintf(buf + count, "hor_sync_time: %d\n", timings->hor_sync_time);
	count += sprintf(buf + count, "ver_total_time: %d\n", timings->ver_total_time);
	count += sprintf(buf + count, "ver_back_porch: %d\n", timings->ver_back_porch);
	count += sprintf(buf + count, "ver_front_porch: %d\n", timings->ver_front_porch);
	count += sprintf(buf + count, "ver_sync_time: %d\n", timings->ver_sync_time);
	count += sprintf(buf + count, "hor_sync_polarity: %d\n", timings->hor_sync_polarity);
	count += sprintf(buf + count, "ver_sync_polarity: %d\n", timings->ver_sync_polarity);
	count += sprintf(buf + count, "b_interlace: %d\n", timings->b_interlace);
	count += sprintf(buf + count, "trd_mode: %d\n", timings->trd_mode);
	count += sprintf(buf + count, "dclk_rate_set: %ld\n", timings->dclk_rate_set);
	count += sprintf(buf + count, "frame_period: %lld\n", timings->frame_period);
	count += sprintf(buf + count, "start_delay: %d\n", timings->start_delay);

	return count;
}

static ssize_t timings_default_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct disp_video_timings *timings;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	u32 fps;

	edp_core = &g_edp_info[sel].edp_core;
	timings = &edp_core->def_timings;

	fps = (timings->pixel_clk / timings->hor_total_time) / timings->ver_total_time;
	count += sprintf(buf + count, "fps: %d\n", fps);
	count += sprintf(buf + count, "vic: %d\n", timings->vic);
	count += sprintf(buf + count, "tv_mode: %d\n", timings->tv_mode);
	count += sprintf(buf + count, "pixel_clk: %d\n", timings->pixel_clk);
	count += sprintf(buf + count, "pixel_repeat: %d\n", timings->pixel_repeat);
	count += sprintf(buf + count, "x_res: %d\n", timings->x_res);
	count += sprintf(buf + count, "y_res: %d\n", timings->y_res);
	count += sprintf(buf + count, "hor_total_time: %d\n", timings->hor_total_time);
	count += sprintf(buf + count, "hor_back_porch: %d\n", timings->hor_back_porch);
	count += sprintf(buf + count, "hor_front_porch: %d\n", timings->hor_front_porch);
	count += sprintf(buf + count, "hor_sync_time: %d\n", timings->hor_sync_time);
	count += sprintf(buf + count, "ver_total_time: %d\n", timings->ver_total_time);
	count += sprintf(buf + count, "ver_back_porch: %d\n", timings->ver_back_porch);
	count += sprintf(buf + count, "ver_front_porch: %d\n", timings->ver_front_porch);
	count += sprintf(buf + count, "ver_sync_time: %d\n", timings->ver_sync_time);
	count += sprintf(buf + count, "hor_sync_polarity: %d\n", timings->hor_sync_polarity);
	count += sprintf(buf + count, "ver_sync_polarity: %d\n", timings->ver_sync_polarity);
	count += sprintf(buf + count, "b_interlace: %d\n", timings->b_interlace);
	count += sprintf(buf + count, "trd_mode: %d\n", timings->trd_mode);
	count += sprintf(buf + count, "dclk_rate_set: %ld\n", timings->dclk_rate_set);
	count += sprintf(buf + count, "frame_period: %lld\n", timings->frame_period);
	count += sprintf(buf + count, "start_delay: %d\n", timings->start_delay);

	return count;
}

static ssize_t timings_edid_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct edp_tx_core *edp_core;
	struct disp_video_timings *timings;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	u32 fps;

	edp_core = &g_edp_info[sel].edp_core;
	timings = &edp_core->edid_timings;

	fps = (timings->pixel_clk / timings->hor_total_time) / timings->ver_total_time;
	count += sprintf(buf + count, "fps: %d\n", fps);
	count += sprintf(buf + count, "vic: %d\n", timings->vic);
	count += sprintf(buf + count, "tv_mode: %d\n", timings->tv_mode);
	count += sprintf(buf + count, "pixel_clk: %d\n", timings->pixel_clk);
	count += sprintf(buf + count, "pixel_repeat: %d\n", timings->pixel_repeat);
	count += sprintf(buf + count, "x_res: %d\n", timings->x_res);
	count += sprintf(buf + count, "y_res: %d\n", timings->y_res);
	count += sprintf(buf + count, "hor_total_time: %d\n", timings->hor_total_time);
	count += sprintf(buf + count, "hor_back_porch: %d\n", timings->hor_back_porch);
	count += sprintf(buf + count, "hor_front_porch: %d\n", timings->hor_front_porch);
	count += sprintf(buf + count, "hor_sync_time: %d\n", timings->hor_sync_time);
	count += sprintf(buf + count, "ver_total_time: %d\n", timings->ver_total_time);
	count += sprintf(buf + count, "ver_back_porch: %d\n", timings->ver_back_porch);
	count += sprintf(buf + count, "ver_front_porch: %d\n", timings->ver_front_porch);
	count += sprintf(buf + count, "ver_sync_time: %d\n", timings->ver_sync_time);
	count += sprintf(buf + count, "hor_sync_polarity: %d\n", timings->hor_sync_polarity);
	count += sprintf(buf + count, "ver_sync_polarity: %d\n", timings->ver_sync_polarity);
	count += sprintf(buf + count, "b_interlace: %d\n", timings->b_interlace);
	count += sprintf(buf + count, "trd_mode: %d\n", timings->trd_mode);
	count += sprintf(buf + count, "dclk_rate_set: %ld\n", timings->dclk_rate_set);
	count += sprintf(buf + count, "frame_period: %lld\n", timings->frame_period);
	count += sprintf(buf + count, "start_delay: %d\n", timings->start_delay);

	return count;
}

static ssize_t hpd_mask_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;

	edp_debug = &g_edp_info[sel].edp_debug;

	return sprintf(buf, "0x%x\n", edp_debug->hpd_mask);
}

static ssize_t hpd_mask_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_debug *edp_debug;
	u32 sel = pdev->id;

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		EDP_WRN("%s syntax error!\n", __func__);
		return count;
	}

	if ((val != 0x0) && (val != 0x10) && (val != 0x11) && (val != 0x110) && \
	    (val != 0x111) && (val != 0x1010) && (val != 0x1011)) {
		EDP_WRN("unavailable param, select from: 0x0/0x10/0x11/0x110/0x111/0x1010/0x1011 !\n");
		return count;
	}

	edp_debug = &g_edp_info[sel].edp_debug;

	edp_debug->hpd_mask = val;

	return count;
}

static ssize_t timings_is_fixed_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	return sprintf(buf, "timings_fixed: %s\n", g_edp_info[sel].timings_fixed ? "Yes" : "No");
}

static ssize_t timings_is_fixed_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		EDP_WRN("%s syntax error!\n", __func__);
		return count;
	}

	if (val != 0)
		 g_edp_info[sel].timings_fixed = true;
	else
		g_edp_info[sel].timings_fixed = false;

	return count;
}

static ssize_t edid_timings_prefer_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	return sprintf(buf, "edid_timings_prefer: %s\n", g_edp_info[sel].edid_timings_prefer ? "Yes" : "No");
}

static ssize_t edid_timings_prefer_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		EDP_WRN("%s syntax error!\n", __func__);
		return count;
	}

	if (val != 0)
		 g_edp_info[sel].edid_timings_prefer = true;
	else
		g_edp_info[sel].edid_timings_prefer = false;

	return count;
}

static ssize_t sink_capacity_prefer_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	return sprintf(buf, "sink_capacity_prefer: %s\n", g_edp_info[sel].sink_capacity_prefer ? "Yes" : "No");
}

static ssize_t sink_capacity_prefer_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		EDP_WRN("%s syntax error!\n", __func__);
		return count;
	}

	if (val != 0)
		 g_edp_info[sel].sink_capacity_prefer = true;
	else
		g_edp_info[sel].sink_capacity_prefer = false;

	return count;
}

static ssize_t training_param_type_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	return sprintf(buf, "training_param_type: %d\n", g_edp_info[sel].edp_core.training_param_type);
}

static ssize_t training_param_type_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		EDP_WRN("%s syntax error!\n", __func__);
		return count;
	}

	if ((val != 0) && (val != 1) && (val != 2)) {
		EDP_WRN("training param type out of range, select from: 0/1/2\n");
		return count;
	}

	g_edp_info[sel].edp_core.training_param_type = val;

	return count;
}

static ssize_t pattern_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 pattern_debug = 0;

	pattern_debug = simple_strtoul(buf, NULL, 0);;

	edp_core_set_pattern(sel, pattern_debug);

	return count;
}

static ssize_t pattern_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	u32 sel = pdev->id;
	u32 count = 0;
	s32 pattern = 0;

	pattern = edp_core_get_train_pattern(sel);
	switch (pattern) {
	case 0:
		count += sprintf(buf + count, "PATTERN_NONE\n");
		break;
	case 1:
		count += sprintf(buf + count, "TPS1\n");
		break;
	case 2:
		count += sprintf(buf + count, "TPS2\n");
		break;
	case 3:
		count += sprintf(buf + count, "TPS3\n");
		break;
	case 4:
		count += sprintf(buf + count, "TPS4\n");
		break;
	case 6:
		count += sprintf(buf + count, "PRBS7\n");
		break;
	default:
		count += sprintf(buf + count, "reversed\n");
		break;

	}

	return count;
}

static ssize_t ssc_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = dev_get_drvdata(dev);
	struct edp_tx_core *edp_core;
	u32 sel = pdev->id;
	u8 *separator = NULL;
	u32 ssc_en = 0;
	u32 ssc_mode = 0;

	edp_core = &g_edp_info[sel].edp_core;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		EDP_WRN("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		ssc_en = simple_strtoul(buf, NULL, 0);
		ssc_mode = simple_strtoul(separator + 1, NULL, 0);

		if (ssc_mode > 1) {
			EDP_WRN("ssc mode should select from: 0/1\n");
			return count;
		}

		if (ssc_en == 0)
			edp_core->ssc_en = 0;
		else
			edp_core->ssc_en = 1;

		edp_core->ssc_mode = ssc_mode;
	}

	return count;
}

static ssize_t ssc_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	count += sprintf(buf + count, "echo [ssc_en ssc_mode] > ssc_debug\n");
	count += sprintf(buf + count, "eg: 0-Center_Mode   1-Downspread_Mode\n");

	return count;
}

static DEVICE_ATTR(dpcd, 0664, dpcd_show, NULL);
static DEVICE_ATTR(edid, 0664, edid_show, edid_store);
static DEVICE_ATTR(lane_debug_en, 0664, lane_debug_en_show, lane_debug_en_store);
static DEVICE_ATTR(lane_fmt_debug, 0664, lane_fmt_debug_show, lane_fmt_debug_store);
static DEVICE_ATTR(lane_cfg_debug, 0664, lane_cfg_debug_show, lane_cfg_debug_store);
static DEVICE_ATTR(lane_sw_debug, 0664, lane_sw_debug_show, lane_sw_debug_store);
static DEVICE_ATTR(lane_pre_debug, 0664, lane_pre_debug_show, lane_pre_debug_store);
static DEVICE_ATTR(resource_lock, 0664, resource_lock_show, resource_lock_store);
static DEVICE_ATTR(bypass_training, 0664, bypass_training_show, bypass_training_store);
static DEVICE_ATTR(loglevel_debug, 0664, loglevel_debug_show, loglevel_debug_store);
static DEVICE_ATTR(hotplug, 0664, hotplug_show, NULL);
static DEVICE_ATTR(hpd_mask, 0664, hpd_mask_show, hpd_mask_store);
static DEVICE_ATTR(sink_info, 0664, sink_info_show, NULL);
static DEVICE_ATTR(source_info, 0664, source_info_show, NULL);
static DEVICE_ATTR(aux_read, 0664, aux_read_show, aux_read_store);
static DEVICE_ATTR(aux_write, 0664, aux_write_show, aux_write_store);
static DEVICE_ATTR(aux_i2c_read, 0664, aux_i2c_read_show, aux_i2c_read_store);
static DEVICE_ATTR(lane_config_now, 0664, lane_config_now_show, NULL);
static DEVICE_ATTR(lane_config_def, 0664, lane_config_def_show, NULL);
static DEVICE_ATTR(lane_config_debug, 0664, lane_config_debug_show, NULL);
static DEVICE_ATTR(timings_now, 0664, timings_now_show, NULL);
static DEVICE_ATTR(timings_default, 0664, timings_default_show, NULL);
static DEVICE_ATTR(timings_edid, 0664, timings_edid_show, NULL);
static DEVICE_ATTR(timings_is_fixed, 0664, timings_is_fixed_show, timings_is_fixed_store);
static DEVICE_ATTR(edid_timings_prefer, 0664, edid_timings_prefer_show, edid_timings_prefer_store);
static DEVICE_ATTR(sink_capacity_prefer, 0664, sink_capacity_prefer_show, sink_capacity_prefer_store);
static DEVICE_ATTR(training_param_type, 0664, training_param_type_show, training_param_type_store);
static DEVICE_ATTR(ssc_debug, 0664, ssc_debug_show, ssc_debug_store);
static DEVICE_ATTR(pattern_debug, 0664, pattern_debug_show, pattern_debug_store);

static struct attribute *edp_attributes[] = {
	&dev_attr_dpcd.attr,
	&dev_attr_edid.attr,
	&dev_attr_lane_debug_en.attr,
	&dev_attr_lane_fmt_debug.attr,
	&dev_attr_lane_cfg_debug.attr,
	&dev_attr_lane_sw_debug.attr,
	&dev_attr_lane_pre_debug.attr,
	&dev_attr_resource_lock.attr,
	&dev_attr_bypass_training.attr,
	&dev_attr_loglevel_debug.attr,
	&dev_attr_hotplug.attr,
	&dev_attr_hpd_mask.attr,
	&dev_attr_sink_info.attr,
	&dev_attr_source_info.attr,
	&dev_attr_aux_read.attr,
	&dev_attr_aux_write.attr,
	&dev_attr_aux_i2c_read.attr,
	&dev_attr_lane_config_now.attr,
	&dev_attr_lane_config_def.attr,
	&dev_attr_lane_config_debug.attr,
	&dev_attr_timings_now.attr,
	&dev_attr_timings_default.attr,
	&dev_attr_timings_edid.attr,
	&dev_attr_timings_is_fixed.attr,
	&dev_attr_edid_timings_prefer.attr,
	&dev_attr_sink_capacity_prefer.attr,
	&dev_attr_training_param_type.attr,
	&dev_attr_ssc_debug.attr,
	&dev_attr_pattern_debug.attr,
	NULL
};

static struct attribute_group edp_attribute_group = {
	.name = "attr",
	.attrs = edp_attributes
};




static s32 edp_open(struct inode *inode, struct file *filp)
{
	return -EINVAL;
}

static s32 edp_release(struct inode *inode, struct file *filp)
{
	return -EINVAL;
}

static ssize_t edp_read(struct file *file, char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static ssize_t edp_write(struct file *file, const char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static s32 edp_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static long edp_ioctl(struct file *filp, u32 cmd, unsigned long arg)
{
	return -EINVAL;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long edp_compat_ioctl(struct file *filp, u32 cmd,
						unsigned long arg)
{
	return -EINVAL;
}
#endif

static const struct file_operations edp_fops = {
	.owner		= THIS_MODULE,
	.open		= edp_open,
	.release	= edp_release,
	.write		= edp_write,
	.read		= edp_read,
	.unlocked_ioctl	= edp_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl	= edp_compat_ioctl,
#endif
	.mmap		= edp_mmap,
};


void edp_get_source_capacity(u32 sel)
{
	struct edp_tx_cap *src_cap;

	src_cap = &g_edp_info[sel].source_cap;

	src_cap->max_lane = edp_source_get_max_lane(sel);
	src_cap->max_rate = edp_source_get_max_rate(sel);
	src_cap->tps3_support = edp_source_support_tps3(sel);
	src_cap->audio_support = edp_source_support_audio(sel);
	src_cap->fast_train_support = edp_source_support_fast_train(sel);
	src_cap->ssc_support = edp_source_support_ssc(sel);
	src_cap->psr_support = edp_source_support_psr(sel);
	src_cap->assr_support = edp_source_support_assr(sel);
	src_cap->mst_support = edp_source_support_mst(sel);
}


s32 snd_edp_get_func(__edp_audio_func *edp_func)
{
	edp_func->edp_audio_enable = edp_core_audio_enable;
	edp_func->edp_audio_disable = edp_core_audio_disable;
	edp_func->edp_audio_set_para = edp_core_audio_set_para;

	return RET_OK;
}
EXPORT_SYMBOL_GPL(snd_edp_get_func);

static void edp_hotplugin_proc(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;

	if (desc->plugin)
		desc->plugin(sel);
}

static void edp_hotplugout_proc(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;

	if (desc->plugout)
		desc->plugout(sel);
}

static void edp_hpd_mask_proc(u32 sel, u32 hpd_mask)
{
	switch (hpd_mask) {
	/* force plugout */
	case 0x10:
		edp_report_hpd_work(sel, EDP_HPD_PLUGOUT);
		edp_hotplugout_proc(sel);
		g_edp_info[sel].hpd_state = FALSE;
		break;
	case 0x110:
	case 0x1010:
		edp_hotplugout_proc(sel);
		g_edp_info[sel].hpd_state = FALSE;
		break;
	/* force plugin */
	case 0x11:
		edp_report_hpd_work(sel, EDP_HPD_PLUGIN);
		edp_hotplugin_proc(sel);
		g_edp_info[sel].hpd_state = TRUE;
		break;
	case 0x111:
	case 0x1011:
		edp_hotplugin_proc(sel);
		g_edp_info[sel].hpd_state = TRUE;
		break;
	default:
		EDP_ERR("Unknown hpd mask!\n");
		break;
	}
}


s32 edp_running_thread(void *parg)
{
	struct edp_info_t *p_edp_info = NULL;
	struct edp_debug *edp_debug;
	u32 sel = 0;

	if (!parg) {
		EDP_ERR("NUll ndl\n");
		return -1;
	}

	p_edp_info = (struct edp_info_t *)parg;
	sel = p_edp_info->dev->id;
	edp_debug = &p_edp_info->edp_debug;

	while (1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(200);

		if (!p_edp_info->suspend) {

			if (edp_debug->hpd_mask & 0x1000)
				continue;

			if (edp_debug->hpd_mask & 0x10) {
				if (edp_debug->hpd_mask != edp_debug->hpd_mask_pre) {
					edp_debug->hpd_mask_pre = edp_debug->hpd_mask;
					edp_hpd_mask_proc(sel, edp_debug->hpd_mask);
				}
				continue;
			} else {
				if (p_edp_info->hpd_state_now != p_edp_info->hpd_state) {
					if (!p_edp_info->hpd_state_now)
						edp_hotplugout_proc(sel);
					else
						edp_hotplugin_proc(sel);

					EDP_DRV_DBG("hpd[%d] = %d\n", sel, p_edp_info->hpd_state_now);
					edp_report_hpd_work(sel, p_edp_info->hpd_state_now);
					p_edp_info->hpd_state = p_edp_info->hpd_state_now;
				}
			}
		}
	}
	return RET_OK;
}

s32 edp_kthread_start(u32 sel)
{
	s32 err = 0;

	if (!edp_task[sel]) {
		edp_task[sel] = kthread_create(edp_running_thread,
					      &(g_edp_info[sel]),
					      "edp detect");
		if (IS_ERR(edp_task[sel])) {
			err = PTR_ERR(edp_task[sel]);
			edp_task[sel] = NULL;
			return err;
		}
		EDP_DRV_DBG("edp_task is ok!\n");
		wake_up_process(edp_task[sel]);
	}
	return RET_OK;
}

s32 edp_kthread_stop(u32 sel)
{
	if (edp_task[sel]) {
		kthread_stop(edp_task[sel]);
		edp_task[sel] = NULL;
	}
	return RET_OK;
}


/**
 * @name       edp_disable
 * @brief      disable edp module
 * @param[IN]  sel:index of edp
 * @param[OUT] none
 * @return     0 if success
 */
s32 edp_disable(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;
	s32 ret = 0;

	if (!desc->disable) {
		EDP_ERR("output disable function missing!\n");
		return RET_FAIL;;
	}

	if (g_edp_info[sel].enable) {
		ret = desc->disable(sel);

		mutex_lock(&g_edp_info[sel].mlock);
		g_edp_info[sel].enable = 0;
		mutex_unlock(&g_edp_info[sel].mlock);
	}
	return ret;
}


s32 edp_enable_early(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;
	s32 ret = 0;

	if (desc->enable_early)
		ret = desc->enable_early(sel);

	return ret;
}

s32 edp_enable(u32 sel)
{
	s32 ret = 0;
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;

	if (!desc->enable) {
		EDP_ERR("output enable function missing!\n");
		goto OUT;
	}

	if (!g_edp_info[sel].enable) {
		ret = desc->enable(sel);
		if (ret)
			goto OUT;

		mutex_lock(&g_edp_info[sel].mlock);
		g_edp_info[sel].enable = 1;
		mutex_unlock(&g_edp_info[sel].mlock);

	} else
		EDP_INFO("edp already enabled!\n");

	return RET_OK;

OUT:
	EDP_ERR("edp fail to enable!\n");
	return ret;
}

s32 edp_sw_enable_early(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;
	s32 ret = 0;

	if (desc->sw_enable_early)
		ret = desc->sw_enable_early(sel);

	return ret;
}

s32 edp_sw_enable(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;
	s32 ret = 0;

	if (desc->sw_enable)
		ret = desc->sw_enable(sel);

	return ret;
}

s32 edp_resume(struct device *dev)
{
	int ret = 0;
	u32 sel = dev->id;
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;

	if (g_edp_info[sel].suspend) {
		mutex_lock(&g_edp_info[sel].mlock);
		g_edp_info[sel].suspend = false;
		mutex_unlock(&g_edp_info[sel].mlock);

		if (desc->resume)
			ret = desc->resume(sel);

		edp_kthread_start(sel);
	}
	return ret;
}

s32 edp_suspend(struct device *dev)
{
	int ret = 0;
	u32 sel = dev->id;
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;

	if (!g_edp_info[sel].suspend) {
		edp_kthread_stop(sel);

		if (desc->suspend)
			ret = desc->suspend(sel);

		mutex_lock(&g_edp_info[sel].mlock);
		g_edp_info[sel].suspend = true;
		mutex_unlock(&g_edp_info[sel].mlock);
	}
	return ret;
}

s32 edp_runtime_suspend(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;
	s32 ret = 0;

	if (desc->runtime_suspend)
		ret = desc->runtime_suspend(sel);

	return ret;
}


s32 edp_runtime_resume(u32 sel)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[sel].desc;
	s32 ret = 0;

	if (desc->runtime_resume)
		ret = desc->runtime_resume(sel);

	return ret;
}

static s32 edp_probe(struct platform_device *pdev)
{
	struct disp_tv_func edp_func;
	s32 ret = -1;
	char edp_dev_name[10];
	struct device *dev = &pdev->dev;
	const struct of_device_id *match;

	EDP_DRV_DBG("edp%d_probe!\n", g_edp_num);

	if (!disp_init_ready())
		return -EPROBE_DEFER;

	if (!g_edp_num)
		memset(&g_edp_info, 0,
		       sizeof(struct edp_info_t) * EDP_NUM_MAX);

	if (g_edp_num > EDP_NUM_MAX - 1) {
		EDP_DEV_ERR(&pdev->dev,
			"g_edp_num(%d) is greater then EDP_NUM_MAX-1(%d)\n",
			g_edp_num, EDP_NUM_MAX - 1);
		goto OUT;
	}

	ret = edp_resource_init(pdev);
	if (ret < 0)
		goto OUT;

	edp_hardware_reset(pdev->id);

	ret = devm_request_irq(&pdev->dev, g_edp_info[pdev->id].irq,
		edp_irq_handler, 0, "edp_irq", pdev);
	if (ret) {
		EDP_DEV_ERR(&pdev->dev, "request irq fail: %d\n", ret);
		goto OUT;
	}

	ret = edp_init(pdev);
	if (ret) {
		EDP_DEV_ERR(&pdev->dev, "edp_init for edp%d fail!\n", pdev->id);
		goto OUT;
	}

	edp_get_source_capacity(pdev->id);

	/*
	 * for edp2.ko, tell disp_edp to finish
	 * some init work when edp2 was ismoded
	 *
	 */
	disp_deliver_edp_dev(&pdev->dev);
	disp_edp_drv_init(pdev->id);

	match = of_match_device(dev->driver->of_match_table, &pdev->dev);
	if (!match) {
		EDP_DEV_ERR(dev, "Unable to match OF ID\n");
		return -ENODEV;
	}
	g_edp_info[pdev->id].desc = (struct sunxi_edp_output_desc *)match->data;

	if (!g_edp_num) {
		memset(&edp_func, 0, sizeof(struct disp_tv_func));
		edp_func.tv_enable_early = edp_enable_early;
		edp_func.tv_enable = edp_enable;
		edp_func.tv_sw_enable_early = edp_sw_enable_early;
		edp_func.tv_sw_enable = edp_sw_enable;
		edp_func.tv_disable = edp_disable;
		edp_func.tv_resume = edp_runtime_resume;
		edp_func.tv_suspend = edp_runtime_suspend;
		edp_func.tv_set_mode = edp_set_mode;
		edp_func.tv_get_mode = edp_get_mode;
		edp_func.tv_mode_support = edp_get_mode_support;
		edp_func.tv_get_video_timing_info = edp_get_video_timing_info;
		edp_func.tv_irq_enable = edp_irq_enable;
		edp_func.tv_irq_query = edp_irq_query;
		edp_func.tv_get_cur_line = edp_get_cur_line;
		edp_func.tv_get_startdelay = edp_get_start_delay;
		edp_func.tv_show_builtin_patten = edp_show_builtin_patten;
		ret = disp_set_edp_func(&edp_func);
		if (ret) {
			EDP_DEV_ERR(&pdev->dev, "disp_set_edp_func edp%d fail!\n",
				pdev->id);
			goto OUT;
		}
	} else
		ret = 0;

	edp_panels_init();

	snprintf(edp_dev_name, sizeof(edp_dev_name), "edp%d", pdev->id);

	/*Create and add a character device*/
	alloc_chrdev_region(&devid[pdev->id], 0, 1, edp_dev_name);/*corely for device number*/

	edp_cdev[pdev->id] = cdev_alloc();

	cdev_init(edp_cdev[pdev->id], &edp_fops);

	edp_cdev[pdev->id]->owner = THIS_MODULE;
	ret = cdev_add(edp_cdev[pdev->id], devid[pdev->id], 1);/*/proc/device/edp*/
	if (ret) {
		EDP_ERR("edp%d cdev_add fail!\n", pdev->id);
		ret = RET_FAIL;
		goto OUT;
	}

	/*Create a path: sys/class/edp*/
	edp_class[pdev->id] = class_create(THIS_MODULE, edp_dev_name);
	if (IS_ERR(edp_class[pdev->id])) {
		EDP_ERR("edp%d class_create fail\n", pdev->id);
		ret = RET_FAIL;
		goto ERR_CDEV;
	}

	/*Create a path "sys/class/edp/edp"*/
	edp_dev[pdev->id] = device_create(edp_class[pdev->id], NULL, devid[pdev->id], NULL, edp_dev_name);
	if (IS_ERR(edp_dev[pdev->id])) {
		EDP_ERR("edp%d device_create fail\n", pdev->id);
		ret = RET_FAIL;
		goto ERR_CLASS;
	}

	/*Create a path: sys/class/edp/edp/attr*/
	ret = sysfs_create_group(&edp_dev[pdev->id]->kobj, &edp_attribute_group);
	if (ret) {
		EDP_ERR("edp sysfs_create_group failed!\n");
		ret = RET_FAIL;
		goto ERR_DEV;
	}

	dev_set_drvdata(edp_dev[pdev->id], pdev);

	if (g_edp_info[pdev->id].desc->init) {
		ret = g_edp_info[pdev->id].desc->init(pdev);
		if (ret) {
			EDP_ERR("edp%d init failed!\n", pdev->id);
			goto ERR_GROUP;
		}
	}

	ret = edp_kthread_start(pdev->id);
	if (ret) {
		EDP_DEV_ERR(&pdev->dev, "edp_kthread_start fail!\n");
		goto ERR_GROUP;
	}


	++g_edp_num;
	EDP_DRV_DBG("edp%d_probe finish!\n", g_edp_num);

	return RET_OK;

ERR_GROUP:
	sysfs_remove_group(&edp_dev[pdev->id]->kobj, &edp_attribute_group);
ERR_DEV:
	device_destroy(edp_class[pdev->id], devid[pdev->id]);
ERR_CLASS:
	class_destroy(edp_class[pdev->id]);
ERR_CDEV:
	cdev_del(edp_cdev[pdev->id]);
OUT:
	return ret;
}

s32 edp_remove(struct platform_device *pdev)
{
	struct sunxi_edp_output_desc *desc = g_edp_info[pdev->id].desc;
	s32 ret = 0;
	u32 i = 0;

	for (i = 0; i < g_edp_num; ++i) {
		edp_kthread_stop(i);
		edp_disable(i);

		if (desc->exit)
			ret = desc->exit(pdev);
	}

	disp_edp_drv_deinit(pdev->id);
	sysfs_remove_group(&edp_dev[pdev->id]->kobj, &edp_attribute_group);
	device_destroy(edp_class[pdev->id], devid[pdev->id]);

	class_destroy(edp_class[pdev->id]);
	cdev_del(edp_cdev[pdev->id]);

	return ret;
}

static const struct dev_pm_ops edp_pm_ops = {
	.suspend = edp_suspend,
	.resume = edp_resume,
};

s32 edp_output_init(struct platform_device *pdev)
{
	u32 sel = pdev->id;

#if IS_ENABLED(CONFIG_EXTCON)
	snprintf(edp_extcon_name, sizeof(edp_extcon_name), "edp%d", sel);
	extcon_edp[sel] = devm_extcon_dev_allocate(&pdev->dev, edp_cable);
	if (IS_ERR_OR_NULL(extcon_edp[sel])) {
		EDP_ERR("devm_extcon_dev_allocate fail:%d", sel);
		return RET_FAIL;
	}
	devm_extcon_dev_register(&pdev->dev, extcon_edp[sel]);
	extcon_edp[sel]->name = edp_extcon_name;
#endif

	return RET_OK;
}

s32 edp_output_exit(struct platform_device *pdev)
{
	return RET_OK;
}

s32 edp_output_sw_enable_early(u32 sel)
{
	struct edp_tx_core *edp_core;
	struct edid *edid;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];
	s32 ret;

	edp_core = &g_edp_info[sel].edp_core;
	if (g_edp_info[sel].vdd_regulator) {
		ret = regulator_enable(g_edp_info[sel].vdd_regulator);
		if (ret) {
			EDP_ERR("vdd-edp enable failed!\n");
			return ret;
		}
	}

	if (g_edp_info[sel].vcc_regulator) {
		ret = regulator_enable(g_edp_info[sel].vcc_regulator);
		if (ret) {
			EDP_ERR("vcc-edp enable failed!\n");
			goto ERR_VCC;
		}
	}

	ret = edp_clk_enable(sel, true);
	if (ret) {
		EDP_ERR("edp%d edp_clk_enable fail!!\n", sel);
		goto ERR_CLK;
	}

	memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));
	ret = edp_read_dpcd(sel, dpcd_rx_buf);
	if (ret < 0)
		EDP_WRN("fail to read edp dpcd!\n");
	else
		edp_parse_dpcd(sel, dpcd_rx_buf);

	edid = edp_edid_get(sel);
	if (edid == NULL)
		EDP_WRN("fail to read edid\n");
	else {
		edp_parse_edid(sel, edid);
		edp_core->edid = edid;
	}

	if (g_edp_info[sel].edid_timings_prefer)
		/* use timing from edid */
		edp_update_timing_from_edid(sel);

	if (g_edp_info[sel].sink_capacity_prefer)
		/* update lane config to adjust source-sink capacity */
		edp_update_capacity(sel);

	if (!edp_core->timings.pixel_clk) {
		EDP_ERR("timing is not set by edid or dts, please check!\n");
		goto ERR_PIXCLK;
	}

	ret = edp_core_query_lane_capability(sel, edp_core, &edp_core->timings);
	if (ret)
		goto ERR_PIXCLK;

	clk_set_rate(g_edp_info[sel].clk, edp_core->timings.pixel_clk);
	g_edp_info[sel].hpd_state_now = true;

	return RET_OK;

ERR_PIXCLK:
	if (edid) {
		edp_edid_put(edid);
		edp_core->edid = NULL;
	}
	memset(&edp_core->edid_timings, 0, sizeof(struct disp_video_timings));
	/* set edid timing mode to an non-exist mode*/
	edp_core->edid_timings.tv_mode = 255;
	edp_core->edid_timings_parsed = false;
	edp_core->dpcd_parsed = false;

	sink_cap_reset(sel);
	memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));
	edp_clk_enable(sel, false);
ERR_CLK:
	if (g_edp_info[sel].vcc_regulator)
		regulator_disable(g_edp_info[sel].vcc_regulator);
ERR_VCC:
	if (g_edp_info[sel].vdd_regulator)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	return ret;
}

s32 edp_output_sw_enable(u32 sel)
{
	return RET_OK;
}

s32 edp_output_enable_early(u32 sel)
{
	struct edp_tx_core *edp_core;
	struct edid *edid;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;
	s32 ret;

	edp_core = &g_edp_info[sel].edp_core;
	if (g_edp_info[sel].vdd_regulator) {
		ret = regulator_enable(g_edp_info[sel].vdd_regulator);
		if (ret) {
			EDP_ERR("vdd-edp enable failed!\n");
			return ret;
		}
	}

	if (g_edp_info[sel].vcc_regulator) {
		ret = regulator_enable(g_edp_info[sel].vcc_regulator);
		if (ret) {
			EDP_ERR("vcc-edp enable failed!\n");
			goto ERR_VCC;
		}
	}

	ret = edp_clk_enable(sel, true);
	if (ret) {
		EDP_ERR("edp%d edp_clk_enable fail!!\n", sel);
		goto ERR_CLK;
	}

	edp_core_phy_init(sel, edp_core);

	memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));
	ret = edp_read_dpcd(sel, dpcd_rx_buf);
	if (ret < 0)
		EDP_WRN("fail to read edp dpcd!\n");
	else
		edp_parse_dpcd(sel, dpcd_rx_buf);

	edid = edp_edid_get(sel);
	if (edid == NULL)
		EDP_WRN("fail to read edid\n");
	else {
		edp_parse_edid(sel, edid);
		edp_core->edid = edid;
	}

	if (g_edp_info[sel].edid_timings_prefer)
		/* use timing from edid */
		edp_update_timing_from_edid(sel);

	if (g_edp_info[sel].sink_capacity_prefer)
		/* update lane config to adjust source-sink capacity */
		edp_update_capacity(sel);

	ret = edp_core_query_lane_capability(sel, edp_core, &edp_core->timings);
	if (ret)
		goto ERR_LANE_CAP;

	return RET_OK;

ERR_LANE_CAP:
	if (!edp_debug->edp_res_lock) {
		if (edid) {
			edp_edid_put(edid);
			edp_core->edid = NULL;
		}
		memset(&edp_core->edid_timings, 0, sizeof(struct disp_video_timings));
		/* set edid timing mode to an non-exist mode*/
		edp_core->edid_timings.tv_mode = 255;
		edp_core->edid_timings_parsed = false;
		edp_core->dpcd_parsed = false;

		sink_cap_reset(sel);
		memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));
	}

	if (!edp_debug->edp_res_lock)
		edp_clk_enable(sel, false);
ERR_CLK:
	if (g_edp_info[sel].vcc_regulator && !edp_debug->edp_res_lock)
		regulator_disable(g_edp_info[sel].vcc_regulator);
ERR_VCC:
	if (g_edp_info[sel].vdd_regulator && !edp_debug->edp_res_lock)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	return ret;
}

s32 edp_output_enable(u32 sel)
{
	struct edp_tx_core *edp_core = &g_edp_info[sel].edp_core;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;
	struct edp_rx_cap *sink_cap = &g_edp_info[sel].sink_cap;
	struct edp_tx_cap *src_cap = &g_edp_info[sel].source_cap;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];
	s32 ret = -1;

	if (!edp_core->timings.pixel_clk) {
		EDP_ERR("timing is not set by edid or dts, please check!\n");
		goto ERR_PHY;
	}

	clk_set_rate(g_edp_info[sel].clk, edp_core->timings.pixel_clk);
	ret = edp_core_enable(sel, edp_core);
	if (ret) {
		EDP_ERR("edp core enable failed!\n");
		goto ERR_PHY;
	}

	if (src_cap->ssc_support) {
		edp_core_ssc_enable(sel, edp_core->ssc_en ? true : false);
		if (edp_core->ssc_en)
			edp_core_ssc_set_mode(sel, edp_core->ssc_mode);
	}

	/* edp need assr enable */
	if (sink_cap->assr_support && src_cap->assr_support) {
		ret = edp_core_assr_enable(sel, true);
		if (ret < 0)
			goto ERR_PHY;
	}

	/* set color space, color depth */
	ret = edp_core_set_video_format(sel, edp_core);
	if (ret < 0)
		goto ERR_PHY;

	/* set specific timings */
	ret = edp_core_set_video_timings(sel, &edp_core->timings);
	if (ret < 0)
		goto ERR_PHY;

	/* set transfer unit */
	ret = edp_core_set_transfer_config(sel, edp_core);
	if (ret < 0)
		goto ERR_PHY;

	if (!edp_debug->bypass_training) {
		ret = edp_main_link_setup(sel, edp_core);
		if (ret < 0)
			goto ERR_PHY;
	}

	edp_core_link_start(sel);

	return ret;

ERR_PHY:
	if (!edp_debug->edp_res_lock) {
		if (edp_core->edid) {
			edp_edid_put(edp_core->edid);
			edp_core->edid = NULL;
		}
		memset(&edp_core->edid_timings, 0, sizeof(struct disp_video_timings));
		/* set edid timing mode to an non-exist mode*/
		edp_core->edid_timings.tv_mode = 255;
		edp_core->edid_timings_parsed = false;
		edp_core->dpcd_parsed = false;

		sink_cap_reset(sel);
		memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));
	}

	if (!edp_debug->edp_res_lock)
		edp_clk_enable(sel, false);

	if (regulator_is_enabled(g_edp_info[sel].vcc_regulator) && !edp_debug->edp_res_lock)
		regulator_disable(g_edp_info[sel].vcc_regulator);

	if (regulator_is_enabled(g_edp_info[sel].vdd_regulator) && !edp_debug->edp_res_lock)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	return ret;
}

s32 edp_output_disable(u32 sel)
{
	s32 ret = 0;
	struct edp_tx_core *edp_core = &g_edp_info[sel].edp_core;
	struct edid *edid;
	struct edp_rx_cap *sink_cap = &g_edp_info[sel].sink_cap;
	struct edp_tx_cap *src_cap = &g_edp_info[sel].source_cap;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];

	ret = edp_core_disable(sel, edp_core);
	if (ret) {
		EDP_ERR("edp core disable failed!\n");
		return ret;
	}

	if (sink_cap->assr_support && src_cap->assr_support)
		edp_core_assr_enable(sel, false);

	ret = edp_clk_enable(sel, false);
	if (ret) {
		EDP_ERR("edp%d edp clk disaable fail!!\n", sel);
		return ret;
	}

	if (g_edp_info[sel].vcc_regulator)
		regulator_disable(g_edp_info[sel].vcc_regulator);

	if (g_edp_info[sel].vdd_regulator)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	edid = edp_core->edid;
	if (edid) {
		edp_edid_put(edid);
		edp_core->edid = NULL;
	}
	memset(&edp_core->edid_timings, 0, sizeof(struct disp_video_timings));
	/* set edid timing mode to an non-exist mode*/
	edp_core->edid_timings.tv_mode = 255;
	edp_core->edid_timings_parsed = false;
	edp_core->dpcd_parsed = false;

	sink_cap_reset(sel);
	memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));


	return ret;
}

/* edp's hpd line is useless in some case, so plugin/plugout is not neccessary */
struct sunxi_edp_output_desc edp_desc = {
	.type		= EDP_OUTPUT,
	.init		= edp_output_init,
	.exit		= edp_output_exit,
	.sw_enable_early = edp_output_sw_enable_early,
	.sw_enable	= edp_output_sw_enable,
	.enable_early	= edp_output_enable_early,
	.enable		= edp_output_enable,
	.disable	= edp_output_disable,
};

s32 dp_output_init(struct platform_device *pdev)
{
	struct edp_tx_core *edp_core;
	s32 ret;
	u32 sel = pdev->id;

	edp_core = &g_edp_info[sel].edp_core;
	if (g_edp_info[sel].vdd_regulator) {
		ret = regulator_enable(g_edp_info[sel].vdd_regulator);
		if (ret) {
			EDP_ERR("vdd-edp enable failed!\n");
			return ret;
		}
	}

	if (g_edp_info[sel].vcc_regulator) {
		ret = regulator_enable(g_edp_info[sel].vcc_regulator);
		if (ret) {
			EDP_ERR("vcc-edp enable failed!\n");
			goto ERR_VCC;
		}
	}

	ret = edp_clk_enable(sel, true);
	if (ret) {
		EDP_ERR("edp%d edp_clk_enable fail!!\n", sel);
		goto ERR_CLK;
	}

	edp_core_phy_init(sel, edp_core);

#if IS_ENABLED(CONFIG_EXTCON)
	snprintf(edp_extcon_name, sizeof(edp_extcon_name), "dp%d", sel);
	extcon_edp[sel] = devm_extcon_dev_allocate(&pdev->dev, dp_cable);
	if (IS_ERR_OR_NULL(extcon_edp[sel])) {
		EDP_ERR("devm_extcon_dev_allocate fail:%d", sel);
		goto ERR_PHY;
	}
	devm_extcon_dev_register(&pdev->dev, extcon_edp[sel]);
	extcon_edp[sel]->name = edp_extcon_name;
#endif

	return ret;

ERR_PHY:
	edp_clk_enable(sel, false);
ERR_CLK:
	if (g_edp_info[sel].vcc_regulator)
		regulator_disable(g_edp_info[sel].vcc_regulator);
ERR_VCC:
	if (g_edp_info[sel].vdd_regulator)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	return ret;
}

s32 dp_output_exit(struct platform_device *pdev)
{
	s32 ret = 0;
	u32 sel = pdev->id;

	ret = edp_clk_enable(sel, false);
	if (ret) {
		EDP_ERR("edp%d edp clk disaable fail!!\n", sel);
		return ret;
	}

	if (g_edp_info[sel].vcc_regulator)
		regulator_disable(g_edp_info[sel].vcc_regulator);

	if (g_edp_info[sel].vdd_regulator)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	return ret;
}

s32 dp_output_sw_enable(u32 sel)
{
	struct edp_tx_core *edp_core = &g_edp_info[sel].edp_core;

	if (!edp_core->timings.pixel_clk) {
		EDP_ERR("timing is not set by edid or dts, please check!\n");
		return RET_FAIL;
	}
	clk_set_rate(g_edp_info[sel].clk, edp_core->timings.pixel_clk);

	return RET_OK;
}

s32 dp_output_enable(u32 sel)
{
	struct edp_tx_core *edp_core = &g_edp_info[sel].edp_core;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;
	struct edp_tx_cap *src_cap = &g_edp_info[sel].source_cap;
	s32 ret = 0;

	if (edp_debug->hpd_mask) {
		if ((edp_debug->hpd_mask == 0x10) || (edp_debug->hpd_mask == 0x110)\
		    || (edp_debug->hpd_mask == 0x1010)) {
			EDP_ERR("sink device unconnect virtual!\n");
			return RET_FAIL;
		}

	} else {
		if (!g_edp_info[sel].hpd_state_now) {
			EDP_ERR("sink device unconnect!\n");
			return RET_FAIL;
		}
	}

	if (!edp_core->timings.pixel_clk) {
		EDP_ERR("timing is not set by edid or dts, please check!\n");
		return RET_FAIL;
	}

	clk_set_rate(g_edp_info[sel].clk, edp_core->timings.pixel_clk);
	ret = edp_core_enable(sel, edp_core);
	if (ret) {
		EDP_ERR("edp core enable failed!\n");
		return ret;
	}

	if (src_cap->ssc_support) {
		edp_core_ssc_enable(sel, edp_core->ssc_en ? true : false);
		if (edp_core->ssc_en)
			edp_core_ssc_set_mode(sel, edp_core->ssc_mode);
	}

	/* set color space, color depth */
	ret = edp_core_set_video_format(sel, edp_core);
	if (ret < 0)
		return RET_FAIL;

	/* set specific timings */
	ret = edp_core_set_video_timings(sel, &edp_core->timings);
	if (ret < 0)
		return RET_FAIL;

	/* set transfer unit */
	ret = edp_core_set_transfer_config(sel, edp_core);
	if (ret < 0)
		return RET_FAIL;

	if (!edp_debug->bypass_training) {
		ret = edp_main_link_setup(sel, edp_core);
		if (ret < 0)
			return RET_FAIL;
	}

	edp_core_link_start(sel);

	return RET_OK;
}

s32 dp_output_disable(u32 sel)
{
	return edp_core_link_stop(sel);
}

s32 dp_output_plugin(u32 sel)
{
	struct edp_tx_core *edp_core;
	struct edid *edid;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];
	char *dpcd_ext_rx_buf = &g_edp_info[sel].dpcd_ext_rx_buf[0];
	s32 ret = 0;

	edp_core = &g_edp_info[sel].edp_core;

	memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));
	ret = edp_read_dpcd(sel, dpcd_rx_buf);
	if (ret < 0)
		EDP_WRN("fail to read edp dpcd!\n");
	else
		edp_parse_dpcd(sel, dpcd_rx_buf);

	/* DP CTS 1.2 Core Rev 1.1, 4.2.2.2 */
	ret = edp_read_dpcd_extended(sel, dpcd_ext_rx_buf);

	/* FIXME: not sure if DisplayPort need fixed lane config setting */
	/* if (g_edp_info[sel].sink_capacity_prefer) */
	/* update lane config to adjust source-sink capacity */
	edp_update_capacity(sel);

	/* DP CTS 1.2 Core Rev 1.1, 4.2.2.8 */
	/* TODO, code for branch device detection */

	edid = edp_edid_get(sel);
	if (edid == NULL)
		EDP_WRN("fail to read edid\n");
	else {
		edp_parse_edid(sel, edid);
		edp_core->edid = edid;
	}

	return RET_OK;
}

s32 dp_output_plugout(u32 sel)
{
	struct edp_tx_core *edp_core;
	struct edid *edid;
	char *dpcd_rx_buf = &g_edp_info[sel].dpcd_rx_buf[0];

	edp_core = &g_edp_info[sel].edp_core;
	edid = edp_core->edid;

	if (edid) {
		edp_edid_put(edid);
		edp_core->edid = NULL;
	}
	memset(&edp_core->edid_timings, 0, sizeof(struct disp_video_timings));
	/* set edid timing mode to an non-exist mode*/
	edp_core->edid_timings.tv_mode = 255;
	edp_core->edid_timings_parsed = false;
	edp_core->dpcd_parsed = false;

	sink_cap_reset(sel);
	memset(dpcd_rx_buf, 0, sizeof(g_edp_info[sel].dpcd_rx_buf));

	return edp_core_link_stop(sel);
}

s32 dp_output_runtime_suspend(u32 sel)
{
	s32 ret = 0;

	ret = edp_clk_enable(sel, false);
	if (ret) {
		EDP_ERR("edp%d edp clk disaable fail!!\n", sel);
		return ret;
	}

	if (g_edp_info[sel].vcc_regulator)
		regulator_disable(g_edp_info[sel].vcc_regulator);

	if (g_edp_info[sel].vdd_regulator)
		regulator_disable(g_edp_info[sel].vdd_regulator);


	return ret;
}

s32 dp_output_runtime_resume(u32 sel)
{
	struct edp_tx_core *edp_core;
	struct edp_debug *edp_debug = &g_edp_info[sel].edp_debug;
	s32 ret;

	edp_core = &g_edp_info[sel].edp_core;
	if (g_edp_info[sel].vdd_regulator) {
		ret = regulator_enable(g_edp_info[sel].vdd_regulator);
		if (ret) {
			EDP_ERR("vdd-edp enable failed!\n");
			return ret;
		}
	}

	if (g_edp_info[sel].vcc_regulator) {
		ret = regulator_enable(g_edp_info[sel].vcc_regulator);
		if (ret) {
			EDP_ERR("vcc-edp enable failed!\n");
			goto ERR_VCC;
		}
	}

	ret = edp_clk_enable(sel, true);
	if (ret) {
		EDP_ERR("edp%d edp_clk_enable fail!!\n", sel);
		goto ERR_CLK;
	}

	edp_core_phy_init(sel, edp_core);
	usleep_range(50, 100);

	if (edp_debug->hpd_mask) {
		if (edp_debug->hpd_mask & 0x10 && (edp_debug->hpd_mask != edp_debug->hpd_mask_pre)) {
			edp_debug->hpd_mask_pre = edp_debug->hpd_mask;
			edp_hpd_mask_proc(sel, edp_debug->hpd_mask);
		}
	} else {
		if (!g_edp_info[sel].hpd_state_now)
			edp_hotplugout_proc(sel);
		else
			edp_hotplugin_proc(sel);

		EDP_DRV_DBG("hpd[%d] = %d\n", sel, g_edp_info[sel].hpd_state_now);
		edp_report_hpd_work(sel, g_edp_info[sel].hpd_state_now);
		g_edp_info[sel].hpd_state = g_edp_info[sel].hpd_state_now;
	}

	return ret;

ERR_CLK:
	if (g_edp_info[sel].vcc_regulator && !edp_debug->edp_res_lock)
		regulator_disable(g_edp_info[sel].vcc_regulator);
ERR_VCC:
	if (g_edp_info[sel].vdd_regulator && !edp_debug->edp_res_lock)
		regulator_disable(g_edp_info[sel].vdd_regulator);

	return ret;
}

struct sunxi_edp_output_desc dp_desc = {
	.type      = DP_OUTPUT,
	.init      = dp_output_init,
	.exit      = dp_output_exit,
	.sw_enable = dp_output_sw_enable,
	.enable    = dp_output_enable,
	.disable   = dp_output_disable,
	.plugin    = dp_output_plugin,
	.plugout   = dp_output_plugout,
	.runtime_suspend = dp_output_runtime_suspend,
	.runtime_resume = dp_output_runtime_resume,
};

static const struct of_device_id sunxi_edp_match[] = {
	{
		.compatible = "allwinner,sunxi-edp0",
		.data = &edp_desc,
	},
	{
		.compatible = "allwinner,sunxi-edp1",
		.data = &edp_desc,
	},
	{
		.compatible = "allwinner,sunxi-dp0",
		.data = &dp_desc,
	},
	{
		.compatible = "allwinner,sunxi-dp1",
		.data = &dp_desc,
	},
	{},
};

static struct platform_driver edp_driver = {
	.probe = edp_probe,
	.remove = edp_remove,
	.driver = {
		.name = "edp",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_edp_match,
		.pm = &edp_pm_ops,
	},
};

s32 __init edp_module_init(void)
{
	s32 ret = 0;


	ret = platform_driver_register(&edp_driver);

	return ret;
}

static void __exit edp_module_exit(void)
{
	platform_driver_unregister(&edp_driver);
}


late_initcall(edp_module_init);
module_exit(edp_module_exit);

MODULE_AUTHOR("huangyongxing@allwinnertech.com");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("edp driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:edp");
/*End of File*/

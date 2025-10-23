// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, version 2.
//
//	This program is distributed in the hope that it will be useful, but
//	WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//	General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_edid.h>
#include <drm/drm_panel.h>
#include <drm/drm_managed.h>
#include <drm/drm_modes.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_file.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_phy.h"
#include "trilin_dptx.h"
#include "trilin_phy.h"
#include "trilin_drm_mst.h"
#include "trilin_drm.h"
#include "trilin_edp_phy.h"
#include "trilin_usbdp_phy.h"

//------------------------------------------------------------------------------
//  Module parameters
//------------------------------------------------------------------------------
static uint trilin_dp_aux_timeout_ms = 50;
module_param_named(aux_timeout_ms, trilin_dp_aux_timeout_ms, uint, 0444);
MODULE_PARM_DESC(aux_timeout_ms, "DP aux timeout value in msec (default: 50)");

/*
 * Some sink requires a delay after power on request
 * According to the DP 1.1 specification, a "Sink Device must exit the
 * power saving state within 1 ms"
 */
#define DEFUALT_DP_POWER_ON_DELAY_MS 4
static uint trilin_dp_power_on_delay_ms = DEFUALT_DP_POWER_ON_DELAY_MS;
module_param_named(power_on_delay_ms, trilin_dp_power_on_delay_ms, uint, 0444);
MODULE_PARM_DESC(power_on_delay_ms, "DP power on delay in msec (default: 4)");

//Define
#define MAX_MST_SLOTS 64
#define MISC0_USE_SYNC_CLOCK 0
#define TRILIN_AVI_SDP_ENABLE 0
static int trilin_dpcd_power_up(struct trilin_dp *dp);

void trilin_dp_write(struct trilin_dp *dp, int offset, u32 val)
{
	if (offset >= TRILIN_DPTX_AUX_COMMAND &&
	    offset <= TRILIN_DPTX_MST_TIMER) {
	} else {
		//DP_DEBUG("0x%0x --> 0x%0x", offset, val);
	}
	writel(val, dp->dp_iomem + offset);
}

u32 trilin_dp_read(struct trilin_dp *dp, int offset)
{
	return readl(dp->dp_iomem + offset);
}

void trilin_phy_write(struct trilin_dp *dp, int offset, u32 val)
{
	//DP_DEBUG("0x%0x --> %d", offset, val);
	writel(val, dp->phy_iomem + offset);
}

u32 trilin_phy_read(struct trilin_dp *dp, int offset)
{
	return readl(dp->phy_iomem + offset);
}

struct trilin_encoder *encoder_to_trilin(struct drm_encoder *encoder)
{
	return container_of(encoder, struct trilin_encoder, base);
}

struct trilin_connector *connector_to_trilin(struct drm_connector *connector)
{
	return container_of(connector, struct trilin_connector, base);
}

static void trilin_link_rate_update(struct trilin_dp *dp, u32 reg)
{
	union phy_configure_opts opts = { 0 };
	struct trilin_phy_t *phy = &dp->phy;
	u8 lanes, max_lanes = dp->link_config.max_lanes;
	int ret;

	if (IS_ERR_OR_NULL(phy->base)) {
		lanes = max_lanes;
	} else {
		lanes = phy_get_bus_width(phy->base);
		if (lanes > max_lanes)
			lanes = max_lanes;
	}

	opts.dp.lanes = lanes;
	opts.dp.link_rate = reg;
	opts.dp.set_lanes = 0;
	opts.dp.set_rate = 1;

	if (phy->phy_ops)
		ret = phy->phy_ops->configure(dp, &opts);
}

/**
 * trilin_dp_phy_ready - Check if PHY is ready
 * @dp: DisplayPort IP core structure
 *
 * Check if PHY is ready. If PHY is not ready, wait 1ms to check for 100 times.
 * This amount of delay was suggested by IP designer.
 *
 * Return: 0 if PHY is ready, or -ENODEV if PHY is not ready.
 */
static int trilin_dp_phy_ready(struct trilin_dp *dp)
{
	u32 i, reg, ready;

	ready = (1 << dp->num_lanes) - 1;

	/* Wait for 100 * 1ms. This should be enough time for PHY to be ready */
	for (i = 0;; i++) {
		reg = trilin_dp_read(dp, TRILIN_DPTX_PHY_STATUS);
		if ((reg & ready) == ready)
			return 0;

		if (i == 100) {
			DP_ERR("PHY isn't ready\n");
			return -ENODEV;
		}

		usleep_range(1000, 1100);
	}

	return 0;
}

/* -----------------------------------------------------------------------------
 * DisplayPort Link Training
 */

/**
 * trilin_dp_max_rate - Calculate and return available max pixel clock
 * The method is link rate * 10 (10kHz) * 0.8 (8b10b) * lane_num / bpp
 *
 * @link_rate: link rate (measured in 10kHz, not kHz)
 * @lane_num: number of lanes
 * @bpp: bits per pixel
 *
 * Return: max pixel clock (KHz) supported by current link config.
 */
int trilin_dp_max_rate(int link_rate, u8 lane_num, u8 bpp)
{
	return (link_rate * lane_num * 8 / bpp);
}

/**
 * trilin_dp_mode_configure - Configure the link values
 * @dp: DisplayPort IP core structure
 * @pclock: pixel clock for requested display mode
 * @current_bw: current link rate
 * @downshift: downshift allowed
 *
 * Find the link configuration values, rate and lane count for requested pixel
 * clock @pclock. The @pclock is stored in the mode to be used in other
 * functions later. The returned rate is downshifted from the current rate
 * @current_bw.
 *
 * Return: Current link rate code, or -EINVAL.
 */
int trilin_dp_mode_configure(struct trilin_dp *dp, int pclock, u8 current_bw,
			     u8 bppex, bool downshift)
{
	int max_rate = dp->link_config.max_rate;
	u8 bw_code;
	u8 max_lanes = dp->link_config.max_lanes;
	u8 max_link_rate_code = drm_dp_link_rate_to_bw_code(max_rate);
	u8 lane_cnt;
	u8 MIN_LANE = 1;
	u8 bpp = bppex ? bppex : 24;

	if (!downshift) {
		bw_code = current_bw;
	} else {
		/* Downshift from current bandwidth */
		switch (current_bw) {
		case DP_LINK_BW_8_1:
			bw_code = DP_LINK_BW_5_4;
			break;
		case DP_LINK_BW_5_4:
			bw_code = DP_LINK_BW_2_7;
			break;
		case DP_LINK_BW_2_7:
			bw_code = DP_LINK_BW_1_62;
			break;
		case DP_LINK_BW_1_62:
			bw_code = DP_LINK_BW_1_62;
			DP_INFO("can't downshift. already lowest link rate\n");
			//return -EINVAL;
			break;
		default:
			/* If not given, start with max supported */
			bw_code = max_link_rate_code;
			break;
		}
	}

	/*
	 * for MST we always configure max link bw - the spec doesn't
	 * seem to suggest we should do otherwise.
	 */
	if (dp->mst.mst_active) {
		MIN_LANE = max_lanes;
		bw_code = max_link_rate_code;
		DP_MST_INFO("set min_lan= %d bw=%d", MIN_LANE, bw_code);
	}

	for (lane_cnt = MIN_LANE; lane_cnt <= max_lanes; lane_cnt <<= 1) {
		int bw;
		u32 rate;

		bw = drm_dp_bw_code_to_link_rate(bw_code);

		rate = trilin_dp_max_rate(bw, lane_cnt, bpp);

		if (pclock <= rate) {
			dp->mode.bw_code = bw_code;
			dp->mode.link_rate = bw;
			dp->mode.lane_cnt = lane_cnt;
			dp->mode.pclock = pclock;
			DP_DEBUG("link rate=%d pixel clock=%d lane=%d",
				 dp->mode.link_rate, dp->mode.pclock, lane_cnt);
			return dp->mode.bw_code;
		}
	}

	DP_ERR("failed to configure: link rate=%d pclock %d, bw %x\n",
	       dp->mode.link_rate, pclock, bw_code);
	return -EINVAL;
}

/**
 * trilin_dp_adjust_train - Adjust train values
 * @dp: DisplayPort IP core structure
 * @link_status: link status from sink which contains requested training values
 */
static void trilin_dp_adjust_train(struct trilin_dp *dp,
				   u8 link_status[DP_LINK_STATUS_SIZE])
{
	u8 *train_set = dp->train_set;
	u8 voltage = 0, preemphasis = 0;
	u8 i;

	for (i = 0; i < dp->mode.lane_cnt; i++) {
		u8 v = drm_dp_get_adjust_request_voltage(link_status, i);
		u8 p = drm_dp_get_adjust_request_pre_emphasis(link_status, i);

		if (v > voltage)
			voltage = v;

		if (p > preemphasis)
			preemphasis = p;
	}

	if (voltage >= DP_TRAIN_VOLTAGE_SWING_LEVEL_3)
		voltage |= DP_TRAIN_MAX_SWING_REACHED;

	if (preemphasis >= DP_TRAIN_PRE_EMPH_LEVEL_2)
		preemphasis |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

	for (i = 0; i < dp->mode.lane_cnt; i++)
		train_set[i] = voltage | preemphasis;
}

static int trilin_dp_update_vs_emph_phy_config(struct trilin_dp *dp)
{
	int ret;
	u8 train;
	union phy_configure_opts opts = { 0 };
	struct trilin_phy_t *phy = &dp->phy;
	u8 lanes, max_lanes = dp->link_config.max_lanes;

	if (IS_ERR_OR_NULL(phy->base)) {
		lanes = max_lanes;
	} else {
		lanes = phy_get_bus_width(phy->base);
		if (lanes > max_lanes)
			lanes = max_lanes;
	}

	opts.dp.lanes = lanes;

	train = dp->train_set[0];
	opts.dp.voltage[0] = (train & DP_TRAIN_VOLTAGE_SWING_MASK) >>
			     DP_TRAIN_VOLTAGE_SWING_SHIFT;
	opts.dp.pre[0] = (train & DP_TRAIN_PRE_EMPHASIS_MASK) >>
			 DP_TRAIN_PRE_EMPHASIS_SHIFT;
	opts.dp.set_voltages = 1;

	if (phy->phy_ops)
		ret = phy->phy_ops->configure(dp, &opts);
	return ret;
}

/**
 * trilin_dp_update_vs_emph - Update the training values
 * @dp: DisplayPort IP core structure
 *
 * Update the training values based on the request from sink. The mapped values
 * are predefined, and values(vs, pe, pc) are from the device manual.
 *
 * Return: 0 if vs and emph are updated successfully, or the error code returned
 * by drm_dp_dpcd_write().
 */
static int trilin_dp_update_vs_emph(struct trilin_dp *dp)
{
	int ret;

	ret = drm_dp_dpcd_write(&dp->aux, DP_TRAINING_LANE0_SET, dp->train_set,
				dp->mode.lane_cnt);
	if (ret < 0) {
		DP_ERR("DP_TRAINING_LANE0_SET %d failed", dp->mode.lane_cnt);
		return ret;
	}

	return trilin_dp_update_vs_emph_phy_config(dp);
}

enum {
	LINK_TRAIN_CLOCK_RECOVERY = 100,
	LINK_TRAIN_CHANNEL_EQUALIZATION,
};

static int trilin_dp_link_set_pattern(struct trilin_dp *dp, int link_train_process, bool fast_train)
{
	int ret = 0;
	u32 pat;

	switch (link_train_process) {
	case LINK_TRAIN_CLOCK_RECOVERY:
		pat = DP_TRAINING_PATTERN_1;
		break;
	case LINK_TRAIN_CHANNEL_EQUALIZATION:
		if (dp->platform_id != CIX_PLATFORM_SOC) {
			pat = DP_TRAINING_PATTERN_2;
		} else {
			if (dp->caps.tps4_supported)
				pat = DP_TRAINING_PATTERN_4;
			else if (dp->caps.tps3_supported)
				pat = DP_TRAINING_PATTERN_3;
			else
				pat = DP_TRAINING_PATTERN_2;
		}
		break;
	default:
		DP_ERR("no this train patten.");
		ret = -1;
		break;
	}

	if (ret < 0)
		return ret;

	if (pat == DP_TRAINING_PATTERN_4) {
		//training pat is 0x4 not 0x7 for dp ctrl
		trilin_dp_write(dp, TRILIN_DPTX_TRAINING_PATTERN_SET, 0x4);
		trilin_dp_write(dp, TRILIN_DPTX_DISABLE_SCRAMBLING, 0);
		if (!fast_train)
			ret = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
						pat);
	} else {
		trilin_dp_write(dp, TRILIN_DPTX_TRAINING_PATTERN_SET, pat);
		if (!fast_train)
			ret = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
						pat | DP_LINK_SCRAMBLING_DISABLE);
	}
	return ret;
}

/**
 * trilin_dp_link_train_cr - Train clock recovery
 * @dp: DisplayPort IP core structure
 *
 * Return: 0 if clock recovery train is done successfully, or corresponding
 * error code.
 */
static int trilin_dp_link_train_cr(struct trilin_dp *dp)
{
	u8 link_status[DP_LINK_STATUS_SIZE];
	u8 lane_cnt = dp->mode.lane_cnt;
	u8 vs = 0, tries = 0;
	u16 max_tries, i;
	bool cr_done;
	int ret;

	ret = trilin_dp_link_set_pattern(dp, LINK_TRAIN_CLOCK_RECOVERY, false);
	if (ret < 0)
		return ret;

	/*
	 * 256 loops should be maximum iterations for 4 lanes and 4 values.
	 * So, This loop should exit before 512 iterations
	 */
	for (max_tries = 0; max_tries < 512; max_tries++) {
		ret = trilin_dp_update_vs_emph(dp);
		if (ret)
			return ret;

		drm_dp_link_train_clock_recovery_delay(&dp->aux, dp->dpcd);
		ret = drm_dp_dpcd_read_link_status(&dp->aux, link_status);
		if (ret < 0)
			return ret;

		cr_done = drm_dp_clock_recovery_ok(link_status, lane_cnt);
		if (cr_done)
			break;

		for (i = 0; i < lane_cnt; i++)
			if (!(dp->train_set[i] & DP_TRAIN_MAX_SWING_REACHED))
				break;
		if (i == lane_cnt)
			break;

		if ((dp->train_set[0] & DP_TRAIN_VOLTAGE_SWING_MASK) == vs)
			tries++;
		else
			tries = 0;

		if (tries == DP_MAX_TRAINING_TRIES)
			break;

		vs = dp->train_set[0] & DP_TRAIN_VOLTAGE_SWING_MASK;
		trilin_dp_adjust_train(dp, link_status);
	}

	if (!cr_done)
		return -EIO;

	dp->train_cr_done = true;
	return 0;
}

/**
 * trilin_dp_link_train_ce - Train channel equalization
 * @dp: DisplayPort IP core structure
 *
 * Return: 0 if channel equalization train is done successfully, or
 * corresponding error code.
 */
static int trilin_dp_link_train_ce(struct trilin_dp *dp)
{
	u8 link_status[DP_LINK_STATUS_SIZE];
	u8 lane_cnt = dp->mode.lane_cnt;
	u32 tries;
	int ret;
	bool ce_done;

	ret = trilin_dp_link_set_pattern(dp, LINK_TRAIN_CHANNEL_EQUALIZATION, false);
	if (ret < 0)
		return ret;

	for (tries = 0; tries < DP_MAX_TRAINING_TRIES; tries++) {
		ret = trilin_dp_update_vs_emph(dp);
		if (ret)
			return ret;

		drm_dp_link_train_channel_eq_delay(&dp->aux, dp->dpcd);
		ret = drm_dp_dpcd_read_link_status(&dp->aux, link_status);
		if (ret < 0) {
			DP_ERR("train_channel_eq failed status:%d", ret);
			return ret;
		}

		ce_done = drm_dp_channel_eq_ok(link_status, lane_cnt);
		if (ce_done)
			break;

		if (dp->platform_id == CIX_PLATFORM_EMU) // skip CE
			goto end;

		trilin_dp_adjust_train(dp, link_status);
	}

	if (!ce_done)
		return -EIO;
end:
	dp->train_ce_done = true;
	return 0;
}

static void trilin_dp_sink_set_msa_timing_ignore(struct trilin_dp *dp,
						struct trilin_dp_panel *dp_panel, bool enable)
{
	struct trilin_connector *conn = dp_panel->connector;

	if (conn && !conn->vrr.enable)
		return;

	if (drm_dp_dpcd_writeb(&dp->aux, DP_DOWNSPREAD_CTRL,
			       enable ? DP_MSA_TIMING_PAR_IGNORE_EN : 0) <= 0)
		DP_WARN("Failed to %s MSA_TIMING_PAR_IGNORE in the sink\n",
			    enable ? "enable" : "disable");
	DP_DEBUG("MSA_TIMING_PAR_IGNORE %s", enable ? "enable" : "disable");
}

/* NOTE: Do not add print info after trilin_dp_train_end.
 * The specification requires that after disabling the training pattern,
 * the idle pattern should be enabled as soon as possible to send to the peer device.
 * However, the specification only describes it as "as soon as" without specific timing requirements
 */
static void trilin_dp_train_end(struct trilin_dp *dp, bool fast_train)
{
	if (!fast_train)
		drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
				DP_TRAINING_PATTERN_DISABLE);
	trilin_dp_write(dp, TRILIN_DPTX_TRAINING_PATTERN_SET,
			DP_TRAINING_PATTERN_DISABLE);
	trilin_dp_write(dp, TRILIN_DPTX_DISABLE_SCRAMBLING, 0);
}

static int trilin_dp_fast_train(struct trilin_dp *dp)
{
	int ret = 0;

	DP_DEBUG("enter");
	//1.config
	trilin_dp_write(dp, TRILIN_DPTX_LANE_COUNT_SET, dp->mode.lane_cnt);
	if (dp->caps.enhanced_framing)
		trilin_dp_write(dp, TRILIN_DPTX_ENHANCED_FRAMING_ENABLE, 1);

	trilin_dp_write(dp, TRILIN_DPTX_LINK_BW_SET, dp->mode.bw_code);
	trilin_link_rate_update(dp, dp->mode.bw_code);
	//2.train
	trilin_dp_write(dp, TRILIN_DPTX_DISABLE_SCRAMBLING, 1);
	ret = trilin_dp_link_set_pattern(dp, LINK_TRAIN_CLOCK_RECOVERY, true);
	if (ret < 0)
		return ret;
	usleep_range(500, 1000);

	ret = trilin_dp_link_set_pattern(dp, LINK_TRAIN_CHANNEL_EQUALIZATION, true);
	if (ret < 0)
		return ret;

	trilin_dp_update_vs_emph_phy_config(dp);
	usleep_range(500, 1000);

	//3.validate link status
	//if (trilin_dp_link_process_link_status_update(dp)) {
	//	ret = -1;
	//}

	//4.disable train
	trilin_dp_train_end(dp, true);
	/* DO NOT PRINT HERE*/
	return ret;
}

static int trilin_dp_link_configure(struct trilin_dp *dp)
{
	u8 bw_code = dp->mode.bw_code;
	u8 lane_cnt = dp->mode.lane_cnt;
	u8 aux_lane_cnt = lane_cnt;
	bool enhanced = dp->caps.enhanced_framing;
	int ret = 0;

	trilin_dp_write(dp, TRILIN_DPTX_LANE_COUNT_SET, lane_cnt);

	if (enhanced) {
		trilin_dp_write(dp, TRILIN_DPTX_ENHANCED_FRAMING_ENABLE, 1);
		aux_lane_cnt |= DP_LANE_COUNT_ENHANCED_FRAME_EN;
	}

	if (dp->dpcd[3] & 0x1) {
		trilin_dp_write(dp, TRILIN_DPTX_DOWNSPREAD_CONTROL, 1);
		drm_dp_dpcd_writeb(&dp->aux, DP_DOWNSPREAD_CTRL,
				   DP_SPREAD_AMP_0_5);
	} else {
		trilin_dp_write(dp, TRILIN_DPTX_DOWNSPREAD_CONTROL, 0);
		drm_dp_dpcd_writeb(&dp->aux, DP_DOWNSPREAD_CTRL, 0);
	}

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_LANE_COUNT_SET, aux_lane_cnt);
	if (ret < 0) {
		DP_ERR("failed to set lane count\n");
		return ret;
	}

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_MAIN_LINK_CHANNEL_CODING_SET,
				 DP_SET_ANSI_8B10B);
	if (ret < 0) {
		DP_ERR("failed to set ANSI 8B/10B encoding\n");
		return ret;
	}

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_LINK_BW_SET, bw_code);
	if (ret < 0) {
		DP_ERR("failed to set DP bandwidth\n");
		return ret;
	}

	//VRR only for SST
	trilin_dp_sink_set_msa_timing_ignore(dp, &dp->dp_panel, true);

	trilin_dp_write(dp, TRILIN_DPTX_LINK_BW_SET, bw_code);

	trilin_link_rate_update(dp, bw_code);

	if (dp->platform_id == CIX_PLATFORM_SOC) {
		ret = trilin_dp_phy_ready(dp);
		if (ret < 0)
			return ret;
	}
	return ret;
}

/**
 * trilin_dp_link_train - Train the link
 * @dp: DisplayPort IP core structure
 *
 * Return: 0 if all trains are done successfully, or corresponding error code.
 */
static int trilin_dp_train(struct trilin_dp *dp)
{
	int ret;
	//1.configure
	ret = trilin_dp_link_configure(dp);
	if (ret < 0)
		return ret;

	//2.train and apply and verify
	trilin_dp_write(dp, TRILIN_DPTX_DISABLE_SCRAMBLING, 1);
	memset(dp->train_set, 0, sizeof(dp->train_set));
	dp->train_cr_done = dp->train_ce_done = false;

	ret = trilin_dp_link_train_cr(dp);
	if (ret) {
		DP_ERR("failed to trilin_dp_link_train_cr. rate:%d lanes:%d\n",
		       dp->mode.link_rate, dp->mode.lane_cnt);
		goto end;
	}

	ret = trilin_dp_link_train_ce(dp);
	if (ret) {
		DP_ERR("failed to trilin_dp_link_train_ce. rate:%d lanes:%d\n",
		       dp->mode.link_rate, dp->mode.lane_cnt);
	}
end:
	//3.train end
	/* disable training pattern even if it is failed */
	trilin_dp_train_end(dp, false);
	/* DO NOT PRINT HERE*/
	return ret;
}

/**
 * trilin_dp_train_loop - Downshift the link rate during training
 * @dp: DisplayPort IP core structure
 *
 * Train the link by downshifting the link rate if training is not successful.
 */
int trilin_dp_train_loop(struct trilin_dp *dp)
{
	struct trilin_dp_mode *mode = &dp->mode;
	u8 bw_cur, bw = mode->bw_code;
	u8 bpp = dp->connector.config.bpp;
	int i, ret, try;

	DP_DEBUG("enter\n");

	for (try = 0; try < DP_MAX_TRAINING_LOOP; try++) {
		i = 0;
		bw_cur = bw;
		do {
			if (!(dp->state & DP_STATE_READY)) {
				DP_DEBUG("disconnected, not do training");
				return 0;
			}

			ret = trilin_dp_train(dp);
			if (!ret)
				return 0;

			ret = trilin_dp_mode_configure(dp, mode->pclock, bw_cur,
						       bpp, true);
			if (ret < 0)
				break;

			bw_cur = ret;

			i++;
			if (i > 10)
				break;

		} while (bw_cur >= DP_LINK_BW_1_62);

		/* retore bw for next try */
		trilin_dp_mode_configure(dp, mode->pclock, 0, bpp, true);
	}

	if (try >= DP_MAX_TRAINING_LOOP)
		DP_ERR("failed to train the DP link\n");
	return ret;
}

/* -----------------------------------------------------------------------------
 * DisplayPort AUX
 */

#define AUX_READ_BIT 0x1

/**
 * trilin_dp_aux_cmd_submit - Submit aux command
 * @dp: DisplayPort IP core structure
 * @cmd: aux command
 * @addr: aux address
 * @buf: buffer for command data
 * @bytes: number of bytes for @buf
 * @reply: reply code to be returned
 *
 * Submit an aux command. All aux related commands, native or i2c aux
 * read/write, are submitted through this function. The function is mapped to
 * the transfer function of struct drm_dp_aux. This function involves in
 * multiple register reads/writes, thus synchronization is needed, and it is
 * done by drm_dp_helper using @hw_mutex. The calling thread goes into sleep
 * if there's no immediate reply to the command submission. The reply code is
 * returned at @reply if @reply != NULL.
 *
 * Return: 0 if the command is submitted properly, or corresponding error code:
 * -EBUSY when there is any request already being processed
 * -ETIMEDOUT when receiving reply is timed out
 * -EIO when received bytes are less than requested
 */
static int trilin_dp_aux_cmd_submit(struct trilin_dp *dp, u32 cmd, u32 addr,
				    u8 *buf, u32 bytes, u8 *reply)
{
	bool is_read = (cmd & AUX_READ_BIT) ? true : false;
	u32 reg, i;

	reg = trilin_dp_read(dp, TRILIN_DPTX_AUX_STATUS);
	if (reg & (TRILIN_DPTX_AUX_STATUS_REQUEST_IN_PROGRESS |
		   TRILIN_DPTX_AUX_STATUS_REPLY_IN_PROGRESS)) {
		DP_DEBUG("TRILIN_DPTX_AUX_STATUS_REPLY_IN_PROGRESS\n");
		return -EBUSY;
	}
	trilin_dp_write(dp, TRILIN_DPTX_AUX_ADDRESS, addr);
	if (!is_read)
		for (i = 0; i < bytes; i++)
			trilin_dp_write(dp, TRILIN_DPTX_AUX_WRITE_FIFO, buf[i]);

	//DP_DEBUG("%s cmd %d addr %d len %d\n", __func__, cmd, addr, bytes);

	reg = cmd << TRILIN_DPTX_AUX_COMMAND_CMD_SHIFT;
	if (!buf || !bytes)
		reg |= TRILIN_DPTX_AUX_COMMAND_ADDRESS_ONLY;
	else
		reg |= (bytes - 1) << TRILIN_DPTX_AUX_COMMAND_BYTES_SHIFT;
	trilin_dp_write(dp, TRILIN_DPTX_AUX_COMMAND, reg);

	/* Wait for reply to be delivered upto 2ms */
	for (i = 0;; i++) {
		reg = trilin_dp_read(dp, TRILIN_DPTX_AUX_STATUS);
		if (reg & TRILIN_DPTX_AUX_STATUS_REPLY_RECEIVED)
			break;

		if ((reg & TRILIN_DPTX_AUX_STATUS_REPLY_ERROR) || (i == 2)) {
			DP_DEBUG("TRILIN_DPTX_AUX_STATUS_REPLY_ERROR\n");
			return -ETIMEDOUT;
		}
		usleep_range(1000, 1100);
	}

	reg = trilin_dp_read(dp, TRILIN_DPTX_AUX_REPLY_CODE);
	if (reply)
		*reply = reg;

	if (dp->platform_id != CIX_PLATFORM_EMU) {
		if (is_read && (reg == TRILIN_DPTX_AUX_REPLY_CODE_AUX_ACK ||
				reg == TRILIN_DPTX_AUX_REPLY_CODE_I2C_ACK)) {
			reg = trilin_dp_read(dp,
					     TRILIN_DPTX_AUX_REPLY_DATA_COUNT);
			if ((reg & TRILIN_DPTX_REPLY_DATA_COUNT_MASK) !=
			    bytes) {
				DP_ERR("TRILIN_DPTX_REPLY_DATA_COUNT_MASK\n");
				return -EIO;
			}
		}
	} else {
		if (is_read) {
			reg = trilin_dp_read(dp,
					     TRILIN_DPTX_AUX_REPLY_DATA_COUNT);
			if ((reg & TRILIN_DPTX_REPLY_DATA_COUNT_MASK) !=
			    bytes) {
				DP_ERR("TRILIN_DPTX_AUX_REPLY_DATA_COUNT\n");
				return -EIO;
			}
		}
	}

	if (is_read) {
		for (i = 0; i < bytes; i++)
			buf[i] = trilin_dp_read(dp, TRILIN_DPTX_AUX_REPLY_DATA);
	}

	return 0;
}

static ssize_t trilin_dp_aux_transfer(struct drm_dp_aux *aux,
				      struct drm_dp_aux_msg *msg)
{
	struct trilin_dp *dp = container_of(aux, struct trilin_dp, aux);
	int ret;
	unsigned int i, iter;
	size_t to_access, accessed_bytes;

	/* Number of loops = timeout in msec / aux delay (400 usec) */
	iter = trilin_dp_aux_timeout_ms * 1000 / 400;
	iter = iter ? iter : 1;

	for (i = 0; i < iter; i++) {
		to_access = 0;
		accessed_bytes = 0;

		/* after sleep, maybe dp is disconnected, so check again */
		if (!trilin_dp_get_hpd_state(dp))
			goto err;
		do {
			to_access = min_t(size_t, DP_AUX_MAX_PAYLOAD_BYTES,
					  msg->size - accessed_bytes);

			ret = trilin_dp_aux_cmd_submit(
				dp, msg->request, msg->address + accessed_bytes,
				msg->buffer + accessed_bytes, to_access,
				&msg->reply);
			if (ret && printk_ratelimit()) {
				DP_ERR("failed to do aux transfer (%d)\n", ret);
				goto err;
			}

			accessed_bytes += to_access;
		} while (accessed_bytes < msg->size);

		if (accessed_bytes == msg->size)
			break;

		usleep_range(400, 500);
	}

	dev_dbg(dp->dev, "aux done after %d retry\n", i);
	return msg->size;
err:
	if (accessed_bytes > to_access)
		accessed_bytes -= to_access;

	return accessed_bytes;
}

/**
 * trilin_dp_aux_register - Initialize and register the DP AUX
 * @dp: DisplayPort IP core structure
 *
 * register the DP AUX adapter.
 *
 * Return: 0 on success, error value otherwise
 */
static int trilin_dp_aux_register(struct trilin_dp *dp)
{
	DP_DEBUG("enter\n");
	dp->aux.name = "Trilin DP AUX";
	dp->aux.dev = dp->dev;
	dp->aux.drm_dev = dp->drm[0];
	dp->aux.transfer = trilin_dp_aux_transfer;
	return drm_dp_aux_register(&dp->aux);
}

/*Program the AUX clock divider and filter*/
static void trilin_dp_aux_clk_divider(struct trilin_dp *dp)
{
	unsigned int rate;
	/*
	 *
	 * Contains the clock divider value for generating the internal 1MHz clock
	 * from the APB host interface clock. The clock divider register provides
	 * integer division only and does not support fractional APB clock rates.
	 * (e.g. set to 75 for a 75MHz APB clock)
	 * 8:0 – APB clock divider value. The valid range is 10 to 400.
	 *
	 */
	rate = dp->aux_clock_divider;
	if (!rate) {
		rate = clk_get_rate(dp->dpsub->apb_clk) / (1000 * 1000);
		if (!rate)
			rate = 25; // fixed value

		dp->aux_clock_divider = rate;
	}

	trilin_dp_write(dp, TRILIN_DPTX_AUX_CLOCK_DIVIDER, rate);
	DP_DEBUG("enter : set aux divider: %d\n", rate);
}

/**
 * trilin_dp_aux_cleanup - Cleanup the DP AUX
 * @dp: DisplayPort IP core structure
 *
 * Unregister the DP AUX adapter.
 */
static void trilin_dp_aux_cleanup(struct trilin_dp *dp)
{
	drm_dp_aux_unregister(&dp->aux);
}

/* -----------------------------------------------------------------------------
 * DisplayPort Generic Support
 */

/**
 * trilin_dp_encoder_mode_set_transfer_unit - Set the transfer unit values
 * @dp: DisplayPort IP core structure
 * @mode: requested display mode
 *
 * Set the transfer unit, and calculate all transfer unit size related values.
 * Calculation is based on DP and IP core specification.
 */
static void trilin_dp_mode_set_transfer_unit(struct trilin_dp *dp, int source,
					     struct drm_display_mode *mode,
					     int bpp)
{
	u32 tu = TRILIN_DPTX_MSA_TRANSFER_UNIT_SIZE_TU_SIZE_DEF;
	u32 bw, vid_kbytes, avg_bytes_per_tu, sym_per_tu, rem_per_tu;
	u32 CUR_REG =
		TRILIN_DPTX_SRC0_TU_CONFIG + TRILIN_DPTX_SOURCE_OFFSET * source;

	/* Use the max transfer unit size (default) */
	trilin_dp_write(dp, CUR_REG, tu);

	vid_kbytes = mode->clock * bpp / 8;
	bw = drm_dp_bw_code_to_link_rate(dp->mode.bw_code);
	avg_bytes_per_tu = vid_kbytes * tu / (dp->mode.lane_cnt * bw / 1000);

	DP_DEBUG("clock=%d bpp=%d avg_bytes_per_tu=%d bw=%d lane=%d",
		 mode->clock, bpp, avg_bytes_per_tu, bw, dp->mode.lane_cnt);
	/* bit 23:16 - SYMBOLS_PER_TU */
	sym_per_tu = (avg_bytes_per_tu / 1000) & 0xff;
	tu |= (sym_per_tu << 16);
	/*
	 * bit 27:24 - FRAC_SYMBOLS_PER_TU: sets the fractional number of valid symbls
	 *   per TU in units of 1/16th. An accumulator is used to increase the number of
	 *   data per transaction unit by 1 every 1/N TUs.
	 */
	rem_per_tu = ((avg_bytes_per_tu % 1000) * 16 / 1000) & 0x0f;
	tu |= (rem_per_tu << 24);

	trilin_dp_write(dp, CUR_REG, tu);
}

/**
 * trilin_dp_mode_set_timing - Configure the main stream
 * @dp: DisplayPort IP core structure
 * @mode: requested display mode
 *
 * Configure the main stream based on the requested mode @mode. Calculation is
 * based on IP core specification.
 */
static void trilin_dp_mode_set_timing(struct trilin_dp *dp, int source,
				      struct drm_display_mode *mode, int bpp)
{
	u8 lane_cnt = dp->mode.lane_cnt;
	u32 reg, wpl;
	unsigned int sec_data_window;
	u32 regs_off = TRILIN_DPTX_SOURCE_OFFSET * source;
	u32 data_control = 0x2004;

	if (dp->mst.mst_active) {
		/* DP1.4 2.6.3.2.2
		 * stream data symbol mapping of MST mode is identical to
		 * that of SST mode in 4-lane Main-Link configuration.
		 */
		lane_cnt = 4;

		//Fixme: data_control is just experience value.
		data_control = 0x2010;
		if (mode->hdisplay == 2560 || mode->hdisplay == 640)
			data_control = 0x201f;
		else if (mode->hdisplay == 1920)
			data_control = 0x2008;
		else if (mode->hdisplay == 1280)
			data_control = 0x200d;
	}

	if (dp->pixel_per_cycle == 2 && dp->mode.lane_cnt == 4 &&
	    dp->mode.link_rate == 810000) {
		data_control = 0x4;
	}

	DP_DEBUG(
		"adjust_mode flags: 0x%0x lane_cnt=%d mode.lans=%d (htotal=%d vtotal=%d) data_control=0x%0x",
		mode->flags, lane_cnt, dp->mode.lane_cnt, mode->htotal,
		mode->vtotal, data_control);

	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_HTOTAL + regs_off,
			mode->htotal);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_VTOTAL + regs_off,
			mode->vtotal);
	trilin_dp_write(
		dp, TRILIN_DPTX_SRC0_MAIN_STREAM_POLARITY + regs_off,
		(!!(mode->flags & DRM_MODE_FLAG_PVSYNC)
		 << TRILIN_DPTX_MAIN_STREAM_POLARITY_VSYNC_SHIFT) |
			(!!(mode->flags & DRM_MODE_FLAG_PHSYNC)
			 << TRILIN_DPTX_MAIN_STREAM_POLARITY_HSYNC_SHIFT));
	trilin_dp_write(
		dp, TRILIN_DPTX_SRC0_USER_SYNC_POLARITY,
		TRILIN_DPTX_MAIN_STREAM_POLARITY_DATAENABLE_HIGH |
			(!!(mode->flags & DRM_MODE_FLAG_PVSYNC)
			 << TRILIN_DPTX_MAIN_STREAM_POLARITY_VSYNC_SHIFT) |
			(!!(mode->flags & DRM_MODE_FLAG_PHSYNC)
			 << TRILIN_DPTX_MAIN_STREAM_POLARITY_HSYNC_SHIFT));
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_HSWIDTH + regs_off,
			mode->hsync_end - mode->hsync_start);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_VSWIDTH + regs_off,
			mode->vsync_end - mode->vsync_start);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_HRES + regs_off,
			mode->hdisplay);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_VRES + regs_off,
			mode->vdisplay);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_HSTART + regs_off,
			mode->htotal - mode->hsync_start);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_VSTART + regs_off,
			mode->vtotal - mode->vsync_start);

	wpl = (mode->hdisplay * bpp + 7) / 8;
	reg = (wpl + lane_cnt - 1) / lane_cnt;
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_USER_DATA_COUNT + regs_off, reg);

	sec_data_window = (mode->htotal - mode->hdisplay) *
			  (dp->mode.link_rate / 40) * 9 / mode->clock;
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_SECONDARY_DATA_WINDOW + regs_off,
			sec_data_window);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_DATA_CONTROL + regs_off,
			data_control);

	/* dp 1ppc or 2ppc */
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_USER_PIXEL_COUNT + regs_off,
			dp->pixel_per_cycle);
}

/**
 * trilin_dp_get_audio_clk_rate - Get the current audio clock rate
 *
 * Return: the current audio clock rate.
 */
static int trilin_dp_get_audio_clk_rate(struct trilin_dp *dp)
{
	return 0;
}

/*
 * Config MSA:
 * In synchronous mode, set the diviers, but now in async mode?
 */
static void trilin_dp_panel_config_msa(struct trilin_dp *dp,
				       struct trilin_dp_panel *dp_panel)
{
	int link_rate;
	int pixel_rate = dp->mode.pclock;
	int audio_link_rate;
	u32 regs_off = TRILIN_DPTX_SOURCE_OFFSET * dp_panel->stream_id;
	struct trilin_connector *conn = dp_panel->connector;

	/* In synchronous mode, set the diviers */
	if (conn->config.misc0 & TRILIN_DPTX_STREAM_MISC0_SYNC_LOCK) {
		link_rate = drm_dp_bw_code_to_link_rate(dp->mode.bw_code);
		trilin_dp_write(dp, TRILIN_DPTX_SRC0_NVID + regs_off,
				link_rate);
		trilin_dp_write(dp, TRILIN_DPTX_SRC0_MVID + regs_off,
				pixel_rate);
		audio_link_rate = trilin_dp_get_audio_clk_rate(dp);
		if (audio_link_rate > 0) {
			DP_INFO("Audio rate: %d /512=%d\n", audio_link_rate,
				audio_link_rate / 512);
			trilin_dp_write(dp, TRILIN_DPTX_SEC0_NAUD + regs_off,
					link_rate);
			trilin_dp_write(dp, TRILIN_DPTX_SEC0_MAUD + regs_off,
					audio_link_rate / 1000);
		}
	}
}

/*
 * As per DP 1.4a spec section 2.2.4.3 [MSA Field for Indication
 * of Color Encoding Format and Content Color Gamut], in order to
 * sending YCBCR 420 or HDR BT.2020 signals we should use DP VSC SDP.
 */
static bool trilin_dp_needs_vsc_sdp(enum trilin_dpsub_format output_format,
				    int colorspace)
{
	if (output_format == TRILIN_DPSUB_FORMAT_YCBCR420)
		return true;

	switch (colorspace) {
	case DRM_MODE_COLORIMETRY_SYCC_601:
	case DRM_MODE_COLORIMETRY_OPYCC_601:
	case DRM_MODE_COLORIMETRY_BT2020_YCC:
	case DRM_MODE_COLORIMETRY_BT2020_RGB:
	case DRM_MODE_COLORIMETRY_BT2020_CYCC:
		return true;
	default:
		break;
	}

	return false;
}

static void trilin_dp_vrr_enable(struct trilin_dp *dp,
				       struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel ? dp_panel->connector : NULL;
	u32 regs_off;

	if (conn && !conn->vrr.enable)
		return;

	regs_off = TRILIN_DPTX_SOURCE_OFFSET * dp_panel->stream_id;
	trilin_dp_write(dp, TRILIN_DPTX_SEC0_ADAPTIVE_SYNC_ENABLE + regs_off,
		1);
}

/* PSR function */
static void trilin_dp_psr_init_dpcd(struct trilin_dp *dp)
{
	u8 val = 8; /* assume the worst if we can't read the value */
	u8 psr_caps, alpm_caps;
	bool y_req, alpm;

	drm_dp_read_desc(&dp->aux, &dp->desc, drm_dp_is_branch(dp->dpcd));

	if (drm_dp_has_quirk(&dp->desc, DP_DPCD_QUIRK_NO_PSR)) {
		DP_DEBUG("PSR support not currently available for this panel\n");
		return;
	}

	drm_dp_dpcd_read(&dp->aux, DP_PSR_SUPPORT, dp->psr_dpcd,
		sizeof(dp->psr_dpcd));

	if (!dp->psr_dpcd[0]) {
		DP_DEBUG("DP_PSR_SUPPORT is no\n");
		return;
	}

	if (drm_dp_dpcd_read(&dp->aux, DP_EDP_DPCD_REV,
		dp->edp_dpcd, sizeof(dp->edp_dpcd)) ==
		sizeof(dp->edp_dpcd)) {
		DP_DEBUG("eDP DPCD: %*ph\n", (int)sizeof(dp->edp_dpcd),
				dp->edp_dpcd);
	}

	if (!(dp->edp_dpcd[1] & DP_EDP_SET_POWER_CAP)) {
		DP_DEBUG("Panel lacks power state control, PSR cannot be enabled\n");
		return;
	}

	dp->caps.psr_sink_support = true;
	dp->psr.main_link_keep_active = false;

	if (dp->psr_dpcd[0] == DP_PSR2_WITH_Y_COORD_IS_SUPPORTED) {
		y_req = dp->psr_dpcd[1] & DP_PSR2_SU_Y_COORDINATE_REQUIRED;
		alpm = false;
		if (drm_dp_dpcd_readb(&dp->aux, DP_RECEIVER_ALPM_CAP,
					&alpm_caps) == 1) {
			alpm = alpm_caps & DP_ALPM_CAP;
		}
		dp->caps.psr2_sink_support = y_req && alpm;
	}

	if (!dp->caps.psr2_sink_support) {
		drm_dp_dpcd_readb(&dp->aux, DP_PSR_CAPS, &psr_caps);
		if (psr_caps != DP_PSR_NO_TRAIN_ON_EXIT) {
			DP_INFO("PSR needs TRAIN_ON_EXIT");
			dp->psr.main_link_keep_active = true;
		}
	}

	if (drm_dp_dpcd_readb(&dp->aux,
		DP_SYNCHRONIZATION_LATENCY_IN_SINK, &val) == 1)
		val &= DP_MAX_RESYNC_FRAME_COUNT_MASK;
	else
		DP_DEBUG("Unable to get sink synchronization latency, assuming 8 frames\n");

	dp->psr.sink_sync_latency = val;
	DP_INFO("Panel Supports PSR %s", dp->caps.psr2_sink_support ? "and PSR2" : "but not PSR2");
}

static void trilin_dp_psr_enable_sink(struct trilin_dp *dp,
		struct trilin_dp_panel *dp_panel)
{
	u8 dpcd_val;
	int ret;
	struct trilin_connector *conn = dp_panel ? dp_panel->connector : NULL;

	if (!dp->caps.psr_sink_support || !dp->psr_default_on
		|| !conn || conn->vrr.enable)
		return;

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_PSR_EN_CFG, 0);
	if (ret != 1) {
		DP_ERR("failed to disable panel psr\n");
		return;
	}

	dpcd_val = DP_PSR_ENABLE;
	/* Enable ALPM at sink for psr2 : Fixme: not support psr2*/
	if (dp->caps.psr2_sink_support && false) {
		drm_dp_dpcd_writeb(&dp->aux, DP_RECEIVER_ALPM_CONFIG,
				   DP_ALPM_ENABLE |
				   DP_ALPM_LOCK_ERROR_IRQ_HPD_ENABLE);
		dpcd_val |= DP_PSR_ENABLE_PSR2 | DP_PSR_IRQ_HPD_WITH_CRC_ERRORS;
	} else {
		if (dp->psr.main_link_keep_active)
			dpcd_val |= DP_PSR_MAIN_LINK_ACTIVE;
		//dpcd_val |= DP_PSR_CRC_VERIFICATION;
	}

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_PSR_EN_CFG, dpcd_val);
	if (ret != 1) {
		DP_ERR("failed to enable panel psr\n");
		return;
	}
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_EDP_CRC_ENABLE, 0x1);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_PSR_3D_ENABLE, 0x1);
	usleep_range(100, 200);
	dp->psr.enable = true;
	trilin_dp_power_on_delay_ms = DEFUALT_DP_POWER_ON_DELAY_MS / 2;
	DP_DEBUG("end");
}

static void trilin_dp_psr_disable_sink(struct trilin_dp *dp)
{
	if (!dp->psr.enable)
		return;

	trilin_dp_write(dp, TRILIN_DPTX_SRC0_PSR_3D_ENABLE, 0x0);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_EDP_CRC_ENABLE, 0x0);

	/* Disable PSR on Sink */
	drm_dp_dpcd_writeb(&dp->aux, DP_PSR_EN_CFG, 0);
	if (dp->psr.psr2_enabled)
		drm_dp_dpcd_writeb(&dp->aux, DP_RECEIVER_ALPM_CONFIG, 0);
	dp->psr.enable = false;
	trilin_dp_power_on_delay_ms = DEFUALT_DP_POWER_ON_DELAY_MS;
	DP_DEBUG("end");
}

static
void trilin_dp_wait_psr_status_ready(struct trilin_dp *dp, bool psr_enable)
{
	int i = 0;
	u8 psr_status;

	while (true) {
		drm_dp_dpcd_readb(&dp->aux, DP_PSR_STATUS, &psr_status);
		if (psr_enable && psr_status == DP_PSR_SINK_ACTIVE_RFB) {
			DP_DEBUG("enable psr waiting psr_status=%s time=%dms"
				, "DP_PSR_SINK_ACTIVE_RFB0", i);
			break;
		} else if (!psr_enable && (psr_status == DP_PSR_SINK_ACTIVE_RESYNC ||
			psr_status == DP_PSR_SINK_INACTIVE)) {
			DP_DEBUG("disable psr waiting psr_status=%s time=%dms",
				psr_status == DP_PSR_SINK_ACTIVE_RESYNC ?
				"DP_PSR_SINK_ACTIVE_RESYNC" : "DP_PSR_SINK_INACTIVE", i);
			break;
		} else {
			usleep_range(1000, 1100);
			if (i++ == 150) {
				DP_WARN("psr_status=%d timeout(150ms)", psr_status);
				break;
			}
		}
	}
}

void trilin_dp_psr_enable(struct trilin_dp *dp,
		struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel ? dp_panel->connector : NULL;
	u8 psr_status;
	int ret;
	struct trilin_phy_t *phy = &dp->phy;

	if (dp->psr.active || !dp->psr.enable ||
		!conn || conn->vrr.enable ||
		(dp_panel && dp_panel->stream_id != 0)) {
		return;
	}

	ret = drm_dp_dpcd_readb(&dp->aux, DP_PSR_STATUS, &psr_status);
	if (ret != 1)
		DP_ERR("Failed to read psr status %d\n", ret);
	else if (psr_status == DP_PSR_SINK_ACTIVE_RFB) {
		DP_WARN("psr status is ACTIVE_RFB");
		return;
	}

	trilin_dp_write(dp, TRILIN_DPTX_SRC0_PSR_STATE, 0x1);
	trilin_dp_wait_psr_status_ready(dp, true);
	//trilin_dp_write(dp, TRILIN_DPTX_SRC0_PSR_STATE, 0x3); //single frame update
	if (!dp->psr.main_link_keep_active) {
		if (phy->phy_ops)
			phy->phy_ops->power(dp, trilin_power_a3);
	}
	dp->psr.active = true;
	DP_DEBUG("end");
}

void trilind_dp_psr_disable(struct trilin_dp *dp,
		struct trilin_dp_panel *dp_panel)
{
	int ret;
	u8 psr_status;
	struct trilin_phy_t *phy = &dp->phy;

	if (!dp->psr.active || (dp_panel && dp_panel->stream_id != 0))
		return;

	if (!dp->psr.main_link_keep_active) {
		if (phy->phy_ops) {
			phy->phy_ops->power(dp, trilin_power_a2);
			phy->phy_ops->power(dp, trilin_power_a0);
		}
		/*Note: For PSR, msleep 2ms not 4ms.*/
		trilin_dpcd_power_up(dp);
	}

	ret = drm_dp_dpcd_readb(&dp->aux, DP_PSR_STATUS, &psr_status);
	if (ret != 1) {
		DP_ERR("Failed to read psr status %d\n", ret);
		return;
	} else if (psr_status == DP_PSR_SINK_INACTIVE) {
		DP_INFO("sink inactive, skip disable psr");
		return;
	}
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_PSR_STATE, 0x0);
	trilin_dp_wait_psr_status_ready(dp, false);

	dp->psr.active = false;
	DP_DEBUG("end");
}

/**
 * trilin_dp_update_misc - Write the misc registers
 * @dp: DisplayPort IP core structure
 *
 * The misc register values are stored in the structure, and this
 * function applies the values into the registers.
 */
static void trilin_dp_update_misc(struct trilin_dp *dp, int source,
				  struct trilin_dp_config *config)
{
	u32 regs_off = TRILIN_DPTX_SOURCE_OFFSET * source;

	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_MISC0 + regs_off,
			config->misc0);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_MISC1 + regs_off,
			config->misc1);
	trilin_dp_write(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_OVERRIDE + regs_off,
			config->override);
}

/**
 * trilin_dp_set_misc - Set the input format
 * @dp: DisplayPort IP core structure
 * @format: input format
 * @bpc: bits per component
 *
 * Update misc register values based on input @format and @bpc.
 *
 * Return: 0 on success, or -EINVAL.
 */
static int trilin_dp_set_misc(struct trilin_dp *dp, int source,
			      struct trilin_dp_config *config)

{
	enum trilin_dpsub_format format = config->format;
	u32 colorspace = config->colorspace;
	unsigned int bpc = config->bpc;

	if (MISC0_USE_SYNC_CLOCK)
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_SYNC_LOCK;
	else
		config->misc0 &= ~TRILIN_DPTX_STREAM_MISC0_SYNC_LOCK;

	config->override = 0; //default
	config->misc1 &= ~TRILIN_DPTX_VSC_COLORIMETRY_EN;

	if (dp->caps.vsc_supported &&
	    trilin_dp_needs_vsc_sdp(format, colorspace)) {
		config->misc1 |= TRILIN_DPTX_VSC_COLORIMETRY_EN;
		switch (bpc) {
		case 6:
			config->override = TRILIN_DPTX_STREAM_OVERRIDE_BPC_6;
			break;
		case 8:
			config->override = TRILIN_DPTX_STREAM_OVERRIDE_BPC_8;
			break;
		case 10:
			config->override = TRILIN_DPTX_STREAM_OVERRIDE_BPC_10;
			break;
		case 12:
			config->override = TRILIN_DPTX_STREAM_OVERRIDE_BPC_12;
			break;
		case 16:
			config->override = TRILIN_DPTX_STREAM_OVERRIDE_BPC_16;
			break;
		default:
			dev_warn(dp->dev,
				 "Not supported bpc (%u). fall back to 8bpc\n",
				 bpc);
			config->override = TRILIN_DPTX_STREAM_OVERRIDE_BPC_8;
			break;
		}

		if (format == TRILIN_DPSUB_FORMAT_YCBCR420)
			config->override |=
				TRILIN_DPTX_STREAM_OVERRIDE_YCbCR_420;

		config->override |= TRILIN_DPTX_STREAM_OVERRIDE_ENABLE;
		config->dynamic_range = DP_DYNAMIC_RANGE_CTA;
		config->content_type = DP_CONTENT_TYPE_NOT_DEFINED;
		goto end;
	}

	config->misc0 &= ~TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_MASK;
	config->misc1 &= ~TRILIN_DPTX_STREAM_MISC1_Y_ONLY_EN;
	switch (format) {
	case TRILIN_DPSUB_FORMAT_RGB:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_RGB;
		break;

	case TRILIN_DPSUB_FORMAT_YCBCR444:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_YCRCB_444;
		break;

	case TRILIN_DPSUB_FORMAT_YCBCR422:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_YCRCB_422;
		break;

	case TRILIN_DPSUB_FORMAT_YONLY:
		config->misc1 |= TRILIN_DPTX_STREAM_MISC1_Y_ONLY_EN;
		break;

	default:
		DP_ERR("Invalid colormetry in DT\n");
		return -EINVAL;
	}

	config->misc0 &= ~TRILIN_DPTX_STREAM_MISC0_BPC_MASK;

	switch (bpc) {
	case 6:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_BPC_6;
		break;
	case 8:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_BPC_8;
		break;
	case 10:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_BPC_10;
		break;
	case 12:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_BPC_12;
		break;
	case 16:
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_BPC_16;
		break;
	default:
		dev_warn(dp->dev, "Not supported bpc (%u). fall back to 8bpc\n",
			 bpc);
		config->misc0 |= TRILIN_DPTX_STREAM_MISC0_BPC_8;
		bpc = 8;
		break;
	}
end:
	trilin_dp_update_misc(dp, source, config);
	return 0;
}

/* ------------------------------------------------------------------------------
 * MST config
 */

//  Function: dptx_set_vc_payload_table
//      This function allocates time slots in the payload table based on the source ID
//      and the number of slots to allocate. It sets the VC payload ID index to
//      the value of 'start' and programs 'slot_count' number of slots with the
//      provided 'source_id'.
//
//      The payload ID table is used by the transmitter core to assign symbol slots
//      in the MTP to a specific input source. Register reads and writes will use
//      this index to access the internal payload ID table memory. The index will
//      automatically increment after each read or write access.
//
//      This is an MST-only function.
//
//  Parameters:
//         dev_id - device ID
//      source_id - source identification number to assign to the payload table
//          start - starting index to begin the assignment
//     slot_count - number of slots to assign
//
//  Returns:
//      None
//------------------------------------------------------------------------------
static void trilin_dp_set_vc_payload_table(struct trilin_dp *dp,
					   uint32_t source_id, uint32_t start,
					   uint32_t slot_count)
{
	trilin_dp_write(dp, TRILIN_DPTX_MST_PID_TABLE_INDEX, start);
	for (uint32_t i = 0; i < slot_count; i++) {
		trilin_dp_write(dp, TRILIN_DPTX_MST_PID_TABLE_ENTRY,
				1 << source_id);
	}
}

static void trilin_dp_reinit_vc_payload_table(struct trilin_dp *dp, int start,
					      int slot_count)
{
	trilin_dp_write(dp, TRILIN_DPTX_MST_PID_TABLE_INDEX, start);
	for (uint32_t i = 0; i < slot_count; i++) {
		trilin_dp_write(dp, TRILIN_DPTX_MST_PID_TABLE_ENTRY,
				0); // clear
	}
}

/* ------------------------------------------------------------------------------
 * Sink pannel
 */

static int trilin_dp_panel_config_misc(struct trilin_dp *dp,
				       struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;

	trilin_dp_set_misc(dp, dp_panel->stream_id, &conn->config);
	return 0;
}

static int
trilin_dp_panel_setup_colorimetry_sdp(struct trilin_dp *dp,
				      struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;

	if (conn->config.override & TRILIN_DPTX_STREAM_OVERRIDE_ENABLE) {
		DP_DEBUG("format=%d colorspace=%0x", conn->config.format, conn->config.colorspace);
		cix_dptx_setup_vsc_sdp(&conn->sdp[CIX_VSC_SDP], &conn->config);
		cix_infoframe_write_packet(dp, dp_panel->stream_id, CIX_VSC_SDP,
					   &conn->sdp[CIX_VSC_SDP], 1);
		if (conn->config.colorspace == DRM_MODE_COLORIMETRY_BT2020_YCC
			|| conn->config.colorspace == DRM_MODE_COLORIMETRY_BT2020_RGB
			|| conn->config.colorspace == DRM_MODE_COLORIMETRY_BT2020_CYCC) {
			conn->hdr_flush = true; //Fixme: test only.
		}
	}
	return 0;
}

static int trilin_dp_panel_setup_avi_sdp(struct trilin_dp *dp,
					 struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;
	struct drm_connector *connector = &conn->base;
	struct drm_connector_state *connector_state = connector->state;
	bool avi_sdp_enable = TRILIN_AVI_SDP_ENABLE;

	if (avi_sdp_enable) {
		cix_dptx_setup_avi_infoframe(&conn->sdp[CIX_AVI_INFO_SDP],
					     conn->config.format,
					     connector_state);
		cix_infoframe_write_packet(dp, dp_panel->stream_id,
					   CIX_AVI_INFO_SDP,
					   &conn->sdp[CIX_AVI_INFO_SDP], 1);
	}
	return 0;
}

int trilin_dp_panel_setup_hdr_sdp(struct trilin_dp *dp,
				  struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;

	if (conn->hdr_flush) {
		DP_DEBUG("enter");
		cix_dp_hdr_metadata_infoframe_sdp_pack(&conn->drm_infoframe,
						       &conn->sdp[CIX_HDR_SDP],
						       sizeof(struct dp_sdp));
		cix_infoframe_write_packet(dp, dp_panel->stream_id, CIX_HDR_SDP,
					   &conn->sdp[CIX_HDR_SDP], 1);
	}
	return 0;
}

void trilin_dp_panel_hw_cfg(struct trilin_dp *dp,
			    struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;
	struct drm_connector *base = &conn->base;
	struct drm_crtc *crtc = base->state->crtc;
	struct drm_crtc_state *crtc_state = crtc->state;
	struct drm_display_mode *mode = &crtc_state->adjusted_mode;

	if (mode == NULL) {
		DP_ERR("Fatal Error: mode is null");
		return;
	}

	/*bits_depth; misc_colorimetry; vsc support (MISC1 bit6);*/
	trilin_dp_panel_config_misc(dp, dp_panel);
	/*link rate, and stream link to config mvid and nvid;*/
	trilin_dp_panel_config_msa(dp, dp_panel);

	/*Fixme: AVI sdp ?*/
	trilin_dp_panel_setup_avi_sdp(dp, dp_panel);
	/*Fixme: add real hdr config BT2020 & metadata*/
	trilin_dp_panel_setup_colorimetry_sdp(dp, dp_panel); //colorspace
	trilin_dp_panel_setup_hdr_sdp(dp, dp_panel);
	trilin_dp_vrr_enable(dp, dp_panel);
	//set transfer unit..
	trilin_dp_mode_set_transfer_unit(dp, dp_panel->stream_id, mode,
					 conn->config.bpp);
	//v_active h_active totalhor_ver,polarity...widebus_en
	trilin_dp_mode_set_timing(dp, dp_panel->stream_id, mode,
				  conn->config.bpp);
}
/* ------------------------------------------------------------------------------
 * Common code for stream
 */

static void trilin_dp_ctrl_set_mst_channel_info(struct trilin_dp *dp,
						enum trilin_dp_stream_id strm,
						u32 start_slot, u32 tot_slots)
{
	if (strm >= dp->max_streams) {
		DP_ERR("invalid input\n");
		return;
	}
	dp->mst_ch_info.slot_info[strm].start_slot = start_slot;
	dp->mst_ch_info.slot_info[strm].tot_slots = tot_slots;
}

int trilin_dp_set_stream_info(struct trilin_dp *dp,
			      struct trilin_dp_panel *dp_panel, u32 stream_id,
			      u32 start_slot, u32 num_slots)
{
	int rc = 0;
	const int max_slots = 64;

	if (stream_id >= dp->max_streams) {
		DP_WARN("invalid stream id:%d\n", stream_id);
		return -EINVAL;
	}

	if (dp_panel && dp_panel->stream_id != stream_id) {
		DP_WARN("invalid stream id:%d -- pannel stream: %d\n",
			stream_id, dp_panel->stream_id);
		dp_panel->stream_id = stream_id;
		//return -EINVAL;
	}

	if (start_slot + num_slots > max_slots) {
		DP_ERR("invalid channel info received. start:%d, slots:%d\n",
		       start_slot, num_slots);
		return -EINVAL;
	}

	mutex_lock(&dp->session_lock);
	trilin_dp_ctrl_set_mst_channel_info(dp, stream_id, start_slot,
					    num_slots);
	mutex_unlock(&dp->session_lock);

	if (!dp_panel)
		DP_WARN("dp_panel did not init for stream %d", stream_id);

	return rc;
}

static int trilin_dp_ctrl_mst_send_act(struct trilin_dp *dp, int table_id)
{
	int rc = 0;
	int reg;

	if (!dp->mst.mst_active)
		return 0;

	trilin_dp_write(dp, TRILIN_DPTX_MST_PID_TABLE_SELECT, table_id);
	trilin_dp_write(dp, TRILIN_DPTX_MST_ALLOCATION_TRIGGER, 0x1);
	usleep_range(6000, 8000); /* Fixme: needs 1 frame time? */

	reg = trilin_dp_read(dp, TRILIN_DPTX_MST_ALLOCATION_TRIGGER);
	if (reg)
		DP_ERR("mst act trigger complete failed\n");
	else
		DP_MST_INFO("mst  ACT trigger complete SUCCESS\n");
	return rc;
}

static void trilin_dp_ctrl_mst_stream_setup(struct trilin_dp *dp,
					    struct trilin_dp_panel *dp_panel,
					    bool enable)
{
	int i;
	u32 inactive_table;
	/*cal stream ...*/
	if (!dp->mst.mst_active)
		return;

	//tmp not used.
	(void)dp_panel;
	(void)enable;

	inactive_table =
		!trilin_dp_read(dp, TRILIN_DPTX_MST_ACTIVE_PAYLOAD_TABLE);
	trilin_dp_write(dp, TRILIN_DPTX_MST_PID_TABLE_SELECT, inactive_table);
	DP_MST_DEBUG("use payload table: %d and clear\n", inactive_table);

	trilin_dp_reinit_vc_payload_table(dp, 0, MAX_MST_SLOTS);

	for (i = DP_STREAM_0; i < dp->max_streams; i++) {
		int start_slot = dp->mst_ch_info.slot_info[i].start_slot;
		int tot_slots = dp->mst_ch_info.slot_info[i].tot_slots;

		if (tot_slots > 0 && start_slot > 0 &&
		    (start_slot + tot_slots) <= MAX_MST_SLOTS) {
			DP_MST_INFO("i=%d write start_slot: %d tot_slots: %d",
				    i, start_slot, tot_slots);
			trilin_dp_set_vc_payload_table(dp, i, start_slot,
						       tot_slots);
		} else {
			DP_MST_DEBUG(
				"i=%d start_slot: %d tot_slots: %d donot write tables.",
				i, start_slot, tot_slots);
		}
	}

	trilin_dp_ctrl_mst_send_act(dp, inactive_table);
}

static int trilin_dp_ctrl_enable_stream_clocks(struct trilin_dp *dp,
					       struct trilin_dp_panel *dp_panel,
					       bool enable)
{
	int rc = 0;
	struct clk *vid_clk;

	if (dp->platform_id != CIX_PLATFORM_SOC)
		return rc;

	if (dp_panel->stream_id == DP_STREAM_0) {
		vid_clk = dp->dpsub->vid_clk0;
	} else if (dp_panel->stream_id == DP_STREAM_1) {
		vid_clk = dp->dpsub->vid_clk1;
	} else if (dp_panel->stream_id == DP_STREAM_2) {
		vid_clk = dp->dpsub->vid_clk2;
	} else if (dp_panel->stream_id == DP_STREAM_3) {
		vid_clk = dp->dpsub->vid_clk3;
	} else {
		DP_ERR("Invalid stream:%d for clk %s\n",
		       dp_panel->stream_id, enable ? "enable" : "disable");
		return -EINVAL;
	}

	if (enable)
		clk_prepare_enable(vid_clk);
	else if (__clk_is_enabled(vid_clk))
		clk_disable_unprepare(vid_clk);

	DP_DEBUG("%s stream%d clk", enable ? "enable" : "disable", dp_panel->stream_id);
	return rc;
}

static int trilin_dp_ctrl_stream_on(struct trilin_dp *dp,
				    struct trilin_dp_panel *dp_panel)
{
	int rc = 0;
	struct dptx_audio *dp_audio = &dp->dp_audio;
	u32 regs_off = TRILIN_DPTX_SOURCE_OFFSET * dp_panel->stream_id;

	/*enable stream power and clock*/
	trilin_dp_ctrl_enable_stream_clocks(dp, dp_panel, true);

	if (dp->enabled_by_gop) {
		DP_DEBUG("[enabled_by_gop]");
		goto end;
	}

	trilin_dp_panel_hw_cfg(dp, dp_panel);
	trilin_dp_ctrl_mst_stream_setup(dp, dp_panel, true);
end:
	trilin_dp_psr_enable_sink(dp, dp_panel);

	trilin_dp_write(dp, TRILIN_DPTX_VIDEO_STREAM_ENABLE + regs_off, 1);
	trilin_dp_write(dp, TRILIN_DPTX_SECONDARY_STREAM_ENABLE + regs_off, 1);

	/* re-config for dptx audio after dp resume back or plugin back if need */
	if (dp_audio->running) {
		DP_INFO("Re-config and enable dptx audio\n");
		dptx_audio_reconfig_and_enable(dp);
	}

	dp->active_panels[dp_panel->stream_id] = dp_panel;
	dp->active_stream_cnt++;

	DP_INFO("trilin_dp_encoder_enable stream_id=%d, active_stream_cnt:%d\n",
		dp_panel->stream_id, dp->active_stream_cnt);

	return rc;
}

static int trilin_dp_ctrl_stream_off(struct trilin_dp *dp,
				     struct trilin_dp_panel *dp_panel)
{
	u32 regs_off = TRILIN_DPTX_SOURCE_OFFSET * dp_panel->stream_id;

	if (dp->edp_panel) {
		if (drm_panel_disable(dp->edp_panel))
			DP_INFO("edp panel disable failed");
	}
	/*reset infoframe*/
	trilin_dp_write(dp, TRILIN_DPTX_SEC0_INFOFRAME_ENABLE + regs_off, 0);
	trilin_dp_write(dp, TRILIN_DPTX_SEC0_INFOFRAME_SELECT + regs_off, 0);
	trilin_dp_write(dp, TRILIN_DPTX_SEC0_INFOFRAME_RATE + regs_off, 0);
	/* vrr disable */
	trilin_dp_write(dp, TRILIN_DPTX_SEC0_ADAPTIVE_SYNC_ENABLE + regs_off, 0);
	/* psr sink disable*/
	trilind_dp_psr_disable(dp, dp_panel);
	trilin_dp_psr_disable_sink(dp);

	trilin_dp_write(dp, TRILIN_DPTX_SECONDARY_STREAM_ENABLE + regs_off, 0);
	trilin_dp_write(dp, TRILIN_DPTX_VIDEO_STREAM_ENABLE + regs_off, 0);
	trilin_dp_ctrl_enable_stream_clocks(dp, dp_panel, false);

	dp->active_panels[dp_panel->stream_id] = NULL;
	dp->active_stream_cnt--;

	DP_INFO("trilin_dp_encoder_disable stream_id=%d, active_stream_cnt=%d\n",
		dp_panel->stream_id, dp->active_stream_cnt);

	return 0;
}

static int trilin_dp_stream_pre_disable(struct trilin_dp *dp,
					struct trilin_dp_panel *dp_panel)
{
	if (!dp->active_stream_cnt) {
		DP_WARN("streams already disabled cnt=%d\n",
			dp->active_stream_cnt);
		return 0;
	}
	trilin_dp_ctrl_mst_stream_setup(dp, dp_panel, false);
	return 0;
}

static int trilin_dp_stream_disable(struct trilin_dp *dp,
				    struct trilin_dp_panel *dp_panel)
{
	if (!dp->active_stream_cnt)
		return 0;

	if (dp_panel->stream_id == dp->max_streams ||
			!dp->active_panels[dp_panel->stream_id]) {
		DP_ERR("panel is already disabled\n");
		return 0;
	}

	trilin_dp_ctrl_stream_off(dp, dp_panel);

	return 0;
}

/*--------------------------------------------------------------------------------------
 * DP Core Ctl
 */

static int trilin_dp_core_power_init(struct trilin_dp *dp)
{
	DP_DEBUG("enter");

	//power on DP_CORE_PM
	//pm_runtime_enable(dp->dev);
	//pm_runtime_get_sync(dp->dev);

	if (!IS_ERR(dp->dpsub->apb_clk))
		clk_prepare_enable(dp->dpsub->apb_clk);
	return 0;
}

static int trilin_dp_core_power_deinit(struct trilin_dp *dp)
{
	DP_DEBUG("enter");

	//power off DP_CORE_PM
	if (!IS_ERR(dp->dpsub->apb_clk))
		if (__clk_is_enabled(dp->dpsub->apb_clk))
			clk_disable_unprepare(dp->dpsub->apb_clk);
	//pm_runtime_put_sync(dp->dev);
	//pm_runtime_disable(dp->dev);
	return 0;
}

static int trilin_dpcd_power_up(struct trilin_dp *dp)
{
	u8 value;
	int i, err;
	struct drm_dp_aux *aux = &dp->aux;

	err = drm_dp_dpcd_readb(aux, DP_SET_POWER, &value);
	if (err < 0)
		return err;

	value &= ~DP_SET_POWER_MASK;
	value |= DP_SET_POWER_D0;

	for (i = 0; i < 3; i++) {
		err = drm_dp_dpcd_writeb(aux, DP_SET_POWER, value);
		if (err == 1)
			break;
		usleep_range(300, 500);
	}

	if (err < 0)
		return err;
	/* Some monitors take time to wake up properly */
	msleep(trilin_dp_power_on_delay_ms);
	return 0;
}

static int trilin_dpcd_power_down(struct trilin_dp *dp)
{
	u8 value;
	int err;
	struct drm_dp_aux *aux = &dp->aux;

	if (!dp->support_d3_cmd)
		return 0;

	if (!trilin_dp_get_hpd_state(dp)) {
		DP_DEBUG("sink device is unplugged, skip following action\n");
		return 0;
	}

	err = drm_dp_dpcd_readb(aux, DP_SET_POWER, &value);
	if (err < 0)
		return err;

	value &= ~DP_SET_POWER_MASK;
	value |= DP_SET_POWER_D3;

	err = drm_dp_dpcd_writeb(aux, DP_SET_POWER, value);
	if (err < 0)
		return err;
	/* 200us propagation time for the power down to take effect */
	usleep_range(200, 205);

	return 0;
}

static int trilin_dp_core_on(struct trilin_dp *dp, bool shallow)
{
	int rc = 0;
	struct trilin_phy_t *phy = &dp->phy;
	bool fast_train_success = false;
	int enable_sources = 0x1; //enable stream 0
	int i;

	DP_DEBUG("enter");
	if (dp->state & DP_STATE_READY) {
		DP_DEBUG("[already ready]");
		return rc;
	}

	dp->state |= DP_STATE_READY;

	if (dp->enabled_by_gop) {
		dp->state |= DP_STATE_INIT_TRAIN;
		DP_DEBUG("[enabled_by_gop]");
		return rc;
	}

	/*Fixme: stream 0 and 1; should follow active_nums..*/
	if (dp->mst.mst_active)
		enable_sources = (1 << dp->max_streams) - 1;

	for (i = 0; i < dp->max_streams; i++)
		trilin_dp_write(dp,
				TRILIN_DPTX_VIDEO_STREAM_ENABLE +
					TRILIN_DPTX_SOURCE_OFFSET * i,
				0);

	if (phy->phy_ops) {
		rc = phy->phy_ops->prepare(dp);
		if (rc) {
			goto end1;
		} else {
			rc = phy->phy_ops->init(dp, trilin_ref_clk_24mhz);
			if (rc) {
				DP_ERR("Failed to initialize DP phy\n");
				goto end2;
			} else {
				DP_INFO("Successly initialize DP phy\n");
			}
		}
	}

	rc = trilin_dpcd_power_up(dp);
	if (rc) {
		DP_ERR("trilin_dpcd_power_up failed: %d", rc);
		goto end3;
	}

	if (dp->train_cr_done && dp->train_ce_done
		&& dp->caps.fast_training && dp->fasttrain_default_on) {
		fast_train_success = !trilin_dp_fast_train(dp);
	}

	if (!fast_train_success)
		rc = trilin_dp_train_loop(dp);

	/*dptx source enable*/
	trilin_dp_write(dp, TRILIN_DPTX_SOFT_RESET, enable_sources);
	trilin_dp_write(dp, TRILIN_DPTX_SOURCE_ENABLE, enable_sources);
	trilin_dp_write(dp, TRILIN_DPTX_FORCE_SCRAMBLER_RESET, enable_sources);
	trilin_dp_write(dp, TRILIN_DPTX_MST_ENABLE, dp->mst.mst_active);

	if (!rc)
		DP_INFO("main link training done! rate:%d lanes:%d %s\n",
			dp->mode.link_rate, dp->mode.lane_cnt, fast_train_success ? "[use fast training]" : "");
	else {
		rc = 0;
		DP_DEBUG("train failed but GO ON");
	}

	DP_DEBUG("Trilinear DPTX %x with %u lanes, platform id %d\n",
		trilin_dp_read(dp, TRILIN_DPTX_CORE_REVISION), dp->num_lanes,
		dp->platform_id);

	if (dp->edp_panel) {
		rc = drm_panel_enable(dp->edp_panel);
		if (rc) {
			DP_INFO("edp panel disable failed but GO ON");
			rc = 0;
		}
	}
	dp->state |= DP_STATE_INIT_TRAIN;
	return rc;
end3:
	if (phy->phy_ops)
		phy->phy_ops->power(dp, trilin_power_a3);
end2:
	if (phy->phy_ops)
		phy->phy_ops->exit(dp);
end1:
	dp->state &= ~DP_STATE_READY;
	return rc;
}

static int trilin_dp_core_off(struct trilin_dp *dp)
{
	int rc = 0;
	struct trilin_phy_t *phy = &dp->phy;

	if (!(dp->state & DP_STATE_READY))
		return rc;

	DP_DEBUG("enter\n");
	trilin_dpcd_power_down(dp);

	if (phy->phy_ops) {
		phy->phy_ops->power(dp, trilin_power_a3);
		phy->phy_ops->exit(dp);
	}

	trilin_dp_write(dp, TRILIN_DPTX_MST_ENABLE, 0);
	trilin_dp_write(dp, TRILIN_DPTX_SOURCE_ENABLE, 0);
	trilin_dp_write(dp, TRILIN_DPTX_SOFT_RESET, 0);
	trilin_dp_write(dp, TRILIN_DPTX_FORCE_SCRAMBLER_RESET, 0);
	trilin_dp_reinit_vc_payload_table(dp, 0, MAX_MST_SLOTS);

	/* clear all mst config*/
	memset(&(dp->mst_ch_info.slot_info),
			0, sizeof(struct trilin_dp_mst_ch_slot_info) * dp->max_streams);

	dp->state &= ~DP_STATE_READY;

	return rc;
}

/**********************************************************************************
 * link state
 */

static int trinlin_dp_panel_read_sink_caps(struct trilin_dp *dp)
{
	const int ADAPTIVE_SYNC_CAPABILITY = 0x2214;
	int rc = 0, rlen = 0;
	u8 rx_feature, adaptive_sync_sdp_support = 0;
	struct drm_dp_aux *drm_aux = &dp->aux;
	struct trilin_dp_link_config *config = &dp->link_config;

	/*Fixme: may need sleep for ready...*/
	rc = drm_dp_read_dpcd_caps(drm_aux, dp->dpcd);
	if (rc) {
		DP_ERR("read dcpd caps failed...\n");
		return rc;
	}

	rc = drm_dp_read_downstream_info(drm_aux, dp->dpcd,
					 dp->downstream_ports);
	if (rc) {
		DP_ERR("read downstream info failed...\n");
		return rc;
	}

	memset(&dp->caps, 0, sizeof(dp->caps));

	dp->caps.enhanced_framing = drm_dp_enhanced_frame_cap(dp->dpcd);
	dp->caps.tps3_supported = drm_dp_tps3_supported(dp->dpcd);
	dp->caps.tps4_supported = drm_dp_tps4_supported(dp->dpcd);
	dp->caps.fast_training = drm_dp_fast_training_cap(dp->dpcd);
	dp->caps.channel_coding = drm_dp_channel_coding_supported(dp->dpcd);
	dp->caps.ssc = !!(dp->dpcd[DP_MAX_DOWNSPREAD] & DP_MAX_DOWNSPREAD_0_5);
	dp->caps.vrr = drm_dp_sink_can_do_video_without_timing_msa(dp->dpcd);
	if(dp->mst_default_on)
		dp->caps.mst = drm_dp_read_mst_cap(&dp->aux, dp->dpcd);

	trilin_dp_psr_init_dpcd(dp);

	rlen = drm_dp_dpcd_read(drm_aux, DP_DPRX_FEATURE_ENUMERATION_LIST,
				&rx_feature, 1);
	if (rlen != 1) {
		DP_DEBUG("failed to read DPRX_FEATURE_ENUMERATION_LIST\n");
		rx_feature = 0;
	} else {
		dp->caps.vsc_supported = !!(
			rx_feature & DP_VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORTED);
		dp->caps.vscext_supported =
			!!(rx_feature & DP_VSC_EXT_VESA_SDP_SUPPORTED) ||
			!!(rx_feature & DP_VSC_EXT_CEA_SDP_SUPPORTED);
		dp->caps.vscext_chaining_supported =
			!!(rx_feature &
			   DP_VSC_EXT_VESA_SDP_CHAINING_SUPPORTED) ||
			!!(rx_feature & DP_VSC_EXT_CEA_SDP_CHAINING_SUPPORTED);
		DP_DEBUG("vsc=%d, vscext=%d, vscext_chaining=%d vs=%d\n",
			dp->caps.vsc_supported, dp->caps.vscext_supported,
			dp->caps.vscext_chaining_supported,
			!!(rx_feature & DP_VSC_EXT_CEA_SDP_SUPPORTED));
	}

	if (drm_dp_dpcd_read(drm_aux, ADAPTIVE_SYNC_CAPABILITY,
			&adaptive_sync_sdp_support, 1) >= 0) {
		dp->caps.arr = adaptive_sync_sdp_support;
	}

	rlen = drm_dp_max_lane_count(dp->dpcd);
	if (rlen > 0) {
		u32 rate;

		config->max_lanes = min_t(u8, rlen, dp->num_lanes);
		rate = drm_dp_max_link_rate(dp->dpcd);
		config->max_rate = min_t(int, rate, dp->max_rate);
	}

	if (dp->hpd_multi_func)
		config->max_lanes = min_t(unsigned int, config->max_lanes, 2);

	dp->sink_count = drm_dp_read_sink_count(&dp->aux);

	DP_DEBUG("mst cap %d rlen=%d max_lanes=%d max_rate=%d sink_count=%d tps3=%d tps4=%d vrr=%d arr=%d psr_cap=%d psr_on=%d fast_training=%d\n",
		dp->caps.mst, rlen, config->max_lanes, config->max_rate,
		dp->sink_count, dp->caps.tps3_supported,
		dp->caps.tps4_supported, dp->caps.vrr, dp->caps.arr,
		dp->caps.psr_sink_support, dp->psr_default_on, dp->caps.fast_training);

	return rc;
}

static bool trilin_dp_link_process_link_status_update(struct trilin_dp *dp)
{
	u8 status[DP_LINK_STATUS_SIZE + 2];
	bool status_update, clock_recovery_ok, channel_eq_ok;
	int ret;

	if (!(dp->state & DP_STATE_ENABLED))
		return false;

	ret = drm_dp_dpcd_read(&dp->aux, DP_SINK_COUNT, status,
			       DP_LINK_STATUS_SIZE + 2);
	if (ret < 0)
		return false;

	/*saved status update but now ignore*/
	dp->status_update = status_update = status[4] & DP_LINK_STATUS_UPDATED;
	if (status_update)
		DP_WARN("link status:[%d] but ignore now", status_update);

	clock_recovery_ok =
		drm_dp_clock_recovery_ok(&status[2], dp->mode.lane_cnt);
	channel_eq_ok = drm_dp_channel_eq_ok(&status[2], dp->mode.lane_cnt);
	if (!clock_recovery_ok || !channel_eq_ok) {
		DP_ERR("clock_recovery_ok[%s], chanel_eq_ok[%s]",
		       clock_recovery_ok ? "YES" : "NO",
			   channel_eq_ok ? "YES" : "NO");
		return true;
	}

	return false;
}

static int trilin_dp_link_process_request(struct trilin_dp *dp)
{
	//test patten not enable now..
	dp->link_request = 0;

	if (trilin_dp_link_process_link_status_update(dp))
		dp->link_request |= DP_LINK_STATUS_UPDATED;

	return 0;
}

static int trilin_dp_link_hdcp_request(struct trilin_dp *dp)
{
	u8 status[DP_LINK_STATUS_SIZE + 2];
	int ret;

	if (!(dp->state & DP_STATE_ENABLED))
		return false;

	ret = drm_dp_dpcd_read(&dp->aux, DP_SINK_COUNT, status,
			       DP_LINK_STATUS_SIZE + 2);
	if (ret < 0)
		return ret;

	if (status[1] & DP_CP_IRQ) {
		ret = drm_dp_dpcd_read(
			&dp->aux, DP_HDCP_2_2_REG_RXSTATUS_OFFSET, status, 1);
		if (ret < 0)
			return ret;

		cix_hdcp_cp_irq_process(&dp->hdcp, status[0]);
	}
	return 0;
}

static int trilin_dp_clear_info(struct trilin_dp *dp,
				struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;
	const int id = dp_panel->stream_id;

	memset(&(dp->mst_ch_info.slot_info[id]), 0,
	       sizeof(struct trilin_dp_mst_ch_slot_info));

	if (conn) {
		conn->hdr_flush = false;
		memset(conn->sdp, 0, sizeof(struct dp_sdp) * CIX_MAX_SDP);
	}
	return 0;
}

/*******************************************************************************
 * host state manager
 *
 */
static void trilin_dp_register_phy(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;
	struct fwnode_handle *fwnode;

	/* register phy here that need power init*/
	if (!phy->phy_ops && dp->platform_id == CIX_PLATFORM_SOC) {
		if (has_acpi_companion(dp->dev)) {
			fwnode = fwnode_find_reference(dp->dev->fwnode, "dp_phy", 0);
			if (!IS_ERR(fwnode)) {
				phy->base = devm_phy_optional_get(
					to_acpi_device_node(fwnode)->dev.parent,
					fwnode_get_name(fwnode));
			} else {
				phy->base = NULL;
			}
		} else {
			phy->base = devm_phy_optional_get(dp->dev, "dp_phy");
		}

		if (IS_ERR_OR_NULL(phy->base))
			DP_WARN("no dp_phy\n");

		trilin_usbdp_phy_register(dp);
		trilin_edp_phy_register(dp);
	}
}

static int reset_dp_and_reinit(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;
	int rc = 0;
	DP_DEBUG("enter");

	if (!IS_ERR(dp->reset)) {
		reset_control_assert(dp->reset);
		usleep_range(10, 20);
		reset_control_deassert(dp->reset);
	}

	if (!IS_ERR(dp->phy_reset)) {
		reset_control_assert(dp->phy_reset);
		usleep_range(10, 20);
		reset_control_deassert(dp->phy_reset);
		phy->state = trilin_phy_power_off;
	}

	usleep_range(100, 200);

	trilin_dp_register_phy(dp);

	/*reset hardware*/
	trilin_dp_write(dp, TRILIN_DPTX_TRANSMITTER_ENABLE, 1);
	trilin_dp_write(dp, TRILIN_DPTX_INTERRUPT_MASK,
			TRILIN_DPTX_INTERRUPT_CFG);

	trilin_dp_aux_clk_divider(dp);

	/* dp phy prepare */
	if (phy->phy_ops) {
		rc = phy->phy_ops->prepare(dp);
		if (rc) {
			DP_ERR("Failed to prepare phy\n");
			return rc;
		} else
			DP_DEBUG("Successly prepare phy\n");
	}
	return rc;
}

int trilin_dp_host_init(struct trilin_dp *dp)
{
	int rc = 0;
	struct trilin_phy_t *phy = &dp->phy;

	DP_DEBUG("enter\n");
	if (dp->state & DP_STATE_INITIALIZED) {
		DP_DEBUG("[already initialized]");
		return rc;
	}

	trilin_dp_core_power_init(dp);

	rc = reset_dp_and_reinit(dp);
	if (rc) {
		DP_ERR("reset_dp_and_reinit failed\n");
		return rc;
	}

	if (dp->edp_panel) {
		rc = drm_panel_prepare(dp->edp_panel);
		if (rc) {
			DP_ERR("edp panel prepare failed");
			goto err_out;
		}
		msleep(50);
		dp->edp_panel_ready = true;
	}

	dp->state |= DP_STATE_INITIALIZED;
	/* log this as it results from user action of cable connection */
	DP_INFO("[OK.]\n");
	return rc;
err_out:
	if (phy->phy_ops)
		phy->phy_ops->exit(dp);
	return rc;
}

static int trilin_dp_host_init_from_bootloader(struct trilin_dp *dp)
{
	int rc = 0;

	trilin_dp_register_phy(dp);

	dp->state |= DP_STATE_INITIALIZED;

	dev_info(dp->dev, "%s reports a plug event\n", __func__);

	/* add force to detect to sync call detect. */
	drm_helper_probe_detect(&dp->connector.base, NULL, false);
	/* then call the hpd envent */
	schedule_delayed_work(&dp->hpd_event_work, 0);
	return rc;
}

static void trilin_dp_host_deinit(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;

	if (dp->active_stream_cnt) {
		DP_DEBUG("active stream present\n");
		return;
	}
	if (!(dp->state & DP_STATE_INITIALIZED)) {
		DP_DEBUG("[not initialized]");
		return;
	}
	if (dp->edp_panel) {
		if (drm_panel_unprepare(dp->edp_panel))
			DP_INFO("unprepare failed");
	}

	if (phy->phy_ops)
		phy->phy_ops->exit(dp);

	/*disable tx*/
	trilin_dp_write(dp, TRILIN_DPTX_TRANSMITTER_ENABLE, 0);
	trilin_dp_write(dp, TRILIN_DPTX_INTERRUPT_MASK,
			TRILIN_DPTX_INTERRUPT_MASK_ALL);
	trilin_dp_core_power_deinit(dp);

	dp->state &= ~DP_STATE_INITIALIZED;
	dp->state &= ~DP_STATE_INIT_TRAIN;
	/* log this as it results from user action of cable dis-connection */
	DP_DEBUG("[OK]\n");
}

static void trilin_dp_display_mst_init(struct trilin_dp *dp)
{
	const unsigned long clear_mstm_ctrl_timeout_us = 50000;
	u8 old_mstm_ctrl;
	int ret, i = 0;

	/* clear sink mst state */
	drm_dp_dpcd_readb(&dp->aux, DP_MSTM_CTRL, &old_mstm_ctrl);
	drm_dp_dpcd_writeb(&dp->aux, DP_MSTM_CTRL, 0);

	/* add extra delay if MST state is not cleared */
	while (old_mstm_ctrl) {
		if (++i > 10)
			break;
		usleep_range(clear_mstm_ctrl_timeout_us,
			     clear_mstm_ctrl_timeout_us + 1000);
		drm_dp_dpcd_readb(&dp->aux, DP_MSTM_CTRL, &old_mstm_ctrl);
	}
	if (i > 0)
		DP_MST_INFO("MSTM_CTRL is not cleared, wait %luus\n",
			    clear_mstm_ctrl_timeout_us * i);

	if (!dp->caps.mst) {
		DP_MST_DEBUG("sink doesn't support mst\n");
		return;
	}

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_MSTM_CTRL,
				 DP_MST_EN | DP_UP_REQ_EN | DP_UPSTREAM_IS_SRC);
	if (ret < 0) {
		DP_ERR("sink mst enablement failed!\n");
		return;
	}
	/*clear stream_info for init, mst_info*/
	trilin_dp_reinit_vc_payload_table(dp, 0, MAX_MST_SLOTS);
	dp->mst.mst_active = true;

	/*reinit mst info*/
	for (i = 0; i < dp->max_mst_encoders; i++)
		dp->mst.mst_panels[i].in_use = false;
}

/* -----------------------------------------------------------------------------
 * Interrupt Handling
 */

bool trilin_dp_get_hpd_state(struct trilin_dp *dp)
{
	return (trilin_dp_read(dp, TRILIN_DPTX_HPD_INPUT_STATE) & 0x0001ul) ?
		       true :
		       false;
}

static bool trilin_dp_is_sink_count_zero(struct trilin_dp *dp)
{
	return drm_dp_is_branch(dp->dpcd) &&
	       (drm_dp_read_sink_count(&dp->aux) == 0);
}

static bool trilin_dp_is_ready(struct trilin_dp *dp)
{
	DP_DEBUG("hpd=%d state=%d sink count:%d is_sink_count_zero=%d\n",
		trilin_dp_get_hpd_state(dp), (dp->state & DP_STATE_CONNECTED),
		drm_dp_read_sink_count(&dp->aux),
		trilin_dp_is_sink_count_zero(dp));
	return trilin_dp_get_hpd_state(dp) && (dp->state & DP_STATE_CONNECTED);
	// && !trilin_dp_is_sink_count_zero(dp);
}

/************ Handle plugin ********************/
int trilin_dp_handle_connect(struct trilin_dp *dp, bool send_notification)
{
	int rc = 0;

	DP_DEBUG("enter\n");
	mutex_lock(&dp->session_lock);
	if (dp->state & DP_STATE_CONNECTED) {
		DP_DEBUG("dp already connected, skipping hpd high\n");
		mutex_unlock(&dp->session_lock);
		return 0;
	}
	if (dp->state & DP_STATE_SUSPENDED) {
		DP_DEBUG("DP_STATE_SUSPENDED return\n");
		goto end;
	}

	dp->state |= DP_STATE_CONNECTED;

	rc = trinlin_dp_panel_read_sink_caps(dp);
	/*
	 * ETIMEDOUT --> cable may have been removed
	 * ENOTCONN --> no downstream device connected
	 */
	if (rc == -ETIMEDOUT || rc == -ENOTCONN) {
		dp->state &= ~DP_STATE_CONNECTED;
		goto end;
	}

	trilin_dp_display_mst_init(dp); //for mst
	if (dp->mst.mst_active) {
		trilin_dp_set_mst_mgr_state(dp, true); //for mst
		dp->active_stream_cnt = 0;
	}
end:
	mutex_unlock(&dp->session_lock);
	return rc;
}

/************ hanlde plugout********************/
int trilin_dp_handle_disconnect(struct trilin_dp *dp, bool send_notification)
{
	int rc = 0;
	int i = 0;

	DP_DEBUG("enter\n");
	mutex_lock(&dp->session_lock);

	if (!(dp->state & DP_STATE_CONNECTED)) {
		DP_DEBUG("already disconnect");
		goto end;
	}

	dp->state &= ~DP_STATE_CONNECTED;
	if (dp->mst.mst_active) {
		/* user mode should active power off to disable encoder */
		trilin_dp_set_mst_mgr_state(dp, false);
		for (i = 0; i < dp->max_mst_encoders; i++)
			dp->mst.mst_panels[i].in_use = false;
		dp->mst.mst_active = false;
	}
end:
	mutex_unlock(&dp->session_lock);
	return rc;
}

/* only called from rmmod */
int trilin_dp_deinit_config(struct trilin_dp *dp)
{
	int rc = 0;

	DP_DEBUG("enter\n");
	cancel_delayed_work_sync(&dp->hpd_irq_work);
	cancel_delayed_work_sync(&dp->hpd_event_work);
	disable_irq(dp->irq);

	mutex_lock(&dp->session_lock);
	trilin_dp_aux_cleanup(dp);
	trilin_dp_host_deinit(dp);
	dp->state &= ~(DP_STATE_CONFIGURED);
	mutex_unlock(&dp->session_lock);
	/*final destry session_lock*/
	mutex_destroy(&dp->session_lock);
	return rc;
}

#define DP_HPD_MAX_TRIES 100

/*hdp event handle plug in and out*/
static void trilin_dp_hpd_event_work_func(struct work_struct *work)
{
	struct trilin_dp *dp;
	struct dptx_audio *dp_audio;
	struct trilin_phy_t *phy;
	int try;
	bool connected;

	dp = container_of(work, struct trilin_dp, hpd_event_work.work);
	phy = &dp->phy;
	dp_audio = &dp->dp_audio;

	if (trilin_dp_get_hpd_state(dp) && !IS_ERR_OR_NULL(phy->base)) {
		volatile enum phy_mode mode;

		for (try = 0; try < DP_HPD_MAX_TRIES; try++) {
			mode = phy_get_mode(phy->base);
			if (mode == PHY_MODE_DP)
				break;
			DP_INFO("mode is not PHY_MODE_DP\n");
			msleep(100);
		}
		if (try >= DP_HPD_MAX_TRIES)
			DP_ERR("Wait too long for phy ready!\n");
	}

	/* add force to detect to sync call detect. */
	drm_helper_probe_detect(&dp->connector.base, NULL, false);

	connected = (dp->status == connector_status_connected);
	if (dp->plugin == connected)
		return;

	dp->plugin = connected;

	if (dp->plugin)
		DP_INFO("dp hpd event received: Plugged\n");
	else
		DP_INFO("dp hpd event received: Unplugged\n");

	if (dp->drm[0])
		drm_helper_hpd_irq_event(dp->drm[0]);

	DP_DEBUG("dp audio plugin status = %d\n", dp->plugin);
	dptx_audio_handle_plugged_change(dp_audio, dp->plugin);

	cix_hdcp_hpd_event_process(&dp->hdcp, dp->plugin);
}

/* hdp irq handle other event */
static void trilin_dp_hpd_irq_work_func(struct work_struct *work)
{
	struct trilin_dp *dp;

	dp = container_of(work, struct trilin_dp, hpd_irq_work.work);
	DP_DEBUG("enter\n");

	if (!trilin_dp_get_hpd_state(dp)) {
		DP_DEBUG("hpd_high off, should update mst state\n");
		return;
	}

	mutex_lock(&dp->session_lock);
	if (!(dp->state & DP_STATE_INITIALIZED)) {
		mutex_unlock(&dp->session_lock);
		goto mst_attention;
	}

	mutex_unlock(&dp->session_lock);
	trilin_dp_link_process_request(dp);

	if (dp->link_request & DP_LINK_STATUS_UPDATED) {
		mutex_lock(&dp->session_lock);
		if (dp->state & DP_STATE_ENABLED)
			trilin_dp_train_loop(dp);
		mutex_unlock(&dp->session_lock);
	}
	trilin_dp_link_hdcp_request(dp);
mst_attention:
	trilin_dp_mst_display_hpd_irq(dp);
}

static irqreturn_t trilin_dp_irq_handler(int irq, void *data)
{
	struct trilin_dp *dp = (struct trilin_dp *)data;
	u32 status, mask;

	status = trilin_dp_read(dp, TRILIN_DPTX_INTERRUPT_CAUSE);
	mask = trilin_dp_read(dp, TRILIN_DPTX_INTERRUPT_MASK);
	if (!(status & ~mask))
		return IRQ_NONE;

	if (status & TRILIN_DPTX_INTERRUPT_HDCP_TIMER_IRQ) {
		dev_dbg_ratelimited(dp->dev, "hdcp tmr\n");
		cix_hdcp_timer_process(&dp->hdcp);
	}

	if (status & TRILIN_DPTX_INTERRUPT_GP_TIMER_IRQ)
		dev_dbg_ratelimited(dp->dev, "gp tmr\n");

	if (status & TRILIN_DPTX_INTERRUPT_REPLY_TIMEOUT)
		dev_dbg_ratelimited(dp->dev, "reply timeout\n");

	if (status & TRILIN_DPTX_INTERRUPT_REPLY_RECIEVED)
		dev_dbg_ratelimited(dp->dev, "reply received\n");

	if (status & TRILIN_DPTX_INTERRUPT_HPD_EVENT) {
		u32 delay = 0;
		if (trilin_dp_get_hpd_state(dp))
			delay = dp->delay_after_hpd;
		schedule_delayed_work(&dp->hpd_event_work,
				      msecs_to_jiffies(delay));
	}

	if (status & TRILIN_DPTX_INTERRUPT_HPD_IRQ)
		schedule_delayed_work(&dp->hpd_irq_work, 0);

	return IRQ_HANDLED;
}

/*--------------------------------------------------------------------------------------
 * Common function for trilin_drm.c and trilin_drm_mst.c
 */

int trilin_dp_pm_prepare(struct trilin_dp *dp)
{
	mutex_lock(&dp->session_lock);
	DP_DEBUG("enter");
	trilin_dp_mst_suspend(dp);
	cancel_delayed_work_sync(&dp->hpd_irq_work);
	cancel_delayed_work_sync(&dp->hpd_event_work);
	disable_irq(dp->irq);
	dp->state |= DP_STATE_SUSPENDED;
	if (!dp->active_stream_cnt) {
		DP_DEBUG("no active stream. just deinit");
		trilin_dp_host_deinit(dp);
	}
	dp->status = connector_status_unknown;
	dp->plugin = false;
	mutex_unlock(&dp->session_lock);
	return 0;
}

int trilin_dp_pm_complete(struct trilin_dp *dp)
{
	mutex_lock(&dp->session_lock);
	DP_DEBUG("enter");
	if (!(dp->state & DP_STATE_SUSPENDED))
		goto end;
	trilin_dp_host_init(dp);

	DP_DEBUG("init hpd: %d", trilin_dp_get_hpd_state(dp));
	enable_irq(dp->irq);
	trilin_dp_mst_resume(dp);
	dp->state &= ~DP_STATE_SUSPENDED;
end:
	mutex_unlock(&dp->session_lock);
	return 0;
}

#define DUMP_DP_REG(reg)                                    \
	seq_printf(m, "0x%0x : 0x%0x\n", (unsigned int)reg, \
		   trilin_dp_read(dp, reg))

void trilin_dp_dump_regs(struct seq_file *m, struct trilin_dp *dp)
{
	int i, j, offset;
	char pidtables[200] = {0};

	seq_puts(m, "----link configuration----\n");
	for (i = TRILIN_DPTX_LINK_BW_SET;
	     i <= TRILIN_DPTX_CUSTOM_80BIT_PATTERN_79_64; i += 4)
		DUMP_DP_REG(i);

	seq_puts(m, "----core enables----\n");
	DUMP_DP_REG(TRILIN_DPTX_TRANSMITTER_ENABLE);
	for (i = TRILIN_DPTX_SOFT_RESET; i <= TRILIN_DPTX_FEC_ENABLE; i += 4)
		DUMP_DP_REG(i);

	seq_printf(
		m,
		"----misc control(scrambler_reset/core_features/core_revision)----\n");
	DUMP_DP_REG(TRILIN_DPTX_FORCE_SCRAMBLER_RESET);
	DUMP_DP_REG(TRILIN_DPTX_CORE_FEATURES);
	DUMP_DP_REG(TRILIN_DPTX_CORE_REVISION);

	seq_puts(m, "----AUX channel----\n");
	for (i = TRILIN_DPTX_AUX_COMMAND;
	     i <= TRILIN_DPTX_AUX_REPLY_TIMEOUT_INTERVAL; i += 4)
		DUMP_DP_REG(i);
	for (i = TRILIN_DPTX_HPD_INPUT_STATE; i <= TRILIN_DPTX_MST_TIMER;
	     i += 4)
		DUMP_DP_REG(i);

	seq_puts(m, "----PHY status----\n");
	DUMP_DP_REG(TRILIN_DPTX_PHY_STATUS);

	if (dp->mst_mgr != NULL) {
		seq_puts(m, "----MST control----\n");
		for (i = TRILIN_DPTX_MST_ENABLE; i <= TRILIN_DPTX_MSO_CONFIG;
		     i += 4) {
			if (i == TRILIN_DPTX_MST_PID_TABLE_ENTRY) {
				trilin_dp_write(
					dp, TRILIN_DPTX_MST_PID_TABLE_INDEX, 0);
				for (j = 0; j < 64; j++) {
					pidtables[j * 2] =
						trilin_dp_read(dp, i) + '0';
					pidtables[j * 2 + 1] = ' ';
				}
				seq_printf(m, "0x%0x : %s\n", i, pidtables);
			} else
				DUMP_DP_REG(i);
		}
	}

	seq_puts(m, "----Main stream control, virtual source 0----\n");
	for (i = TRILIN_DPTX_VIDEO_STREAM_ENABLE;
	     i <= TRILIN_DPTX_SRC0_FRAMING_STATUS; i += 4)
		DUMP_DP_REG(i);
	seq_puts(m, "----Secondary channel, source 0----\n");
	for (i = TRILIN_DPTX_SEC0_AUDIO_ENABLE;
	     i <= TRILIN_DPTX_SEC0_ADAPTIVE_SYNC_ENABLE; i += 4)
		DUMP_DP_REG(i);

	seq_puts(m, "----PSR control, virtual source 0----\n");
	for (i = TRILIN_DPTX_SRC0_EDP_CRC_ENABLE;
		i <= TRILIN_DPTX_SRC0_EDP_CRC_BLUE; i += 4)
		DUMP_DP_REG(i);
	for (i = TRILIN_DPTX_SRC0_PSR_3D_ENABLE;
		i <= TRILIN_DPTX_SRC0_PSR2_UPDATE_WIDTH; i += 4)
		DUMP_DP_REG(i);

	if (dp->mst_mgr != NULL) {
		for (j = 1; j < dp->max_streams; j++) {
			seq_printf(m, "----Main stream control, virtual source %d----\n", j);
			offset = TRILIN_DPTX_SOURCE_OFFSET * j;
			for (i = TRILIN_DPTX_VIDEO_STREAM_ENABLE + offset; i <= TRILIN_DPTX_SRC0_FRAMING_STATUS + offset; i += 4)
				DUMP_DP_REG(i);
			seq_printf(m, "----Secondary channel, source %d----\n", j);
			for (i = TRILIN_DPTX_SEC0_AUDIO_ENABLE + offset; i <= TRILIN_DPTX_SEC0_ADAPTIVE_SYNC_ENABLE + offset; i += 4)
				DUMP_DP_REG(i);
		}
	}
}

int trilin_dp_prepare(struct trilin_dp *dp)
{
	int rc = 0;

	DP_DEBUG("enter\n");
	mutex_lock(&dp->session_lock);

	/*
	 * If DP_STATE_ENABLED, there is nothing left to do.
	 */
	if (dp->state & (DP_STATE_ENABLED)) {
		DP_DEBUG("[already enabled, mst second stream?]");
		goto end;
	}

	if (!(dp->state & DP_STATE_INITIALIZED)) {
		rc = trilin_dp_host_init(dp);
		if (rc) {
			DP_WARN("Host init Failed");
			goto end;
		}
	} else if (dp->state & DP_STATE_INIT_TRAIN){
		reset_dp_and_reinit(dp); //enable dp reset...
	}

	if (!trilin_dp_is_ready(dp)) {
		DP_DEBUG("[not ready]");
		goto end;
	}

	rc = trilin_dp_core_on(dp, true);
end:
	mutex_unlock(&dp->session_lock);
	return rc;
}

int trilin_dp_enable(struct trilin_dp *dp, struct trilin_dp_panel *dp_panel)
{
	int rc = 0;

	DP_DEBUG("enter\n");
	mutex_lock(&dp->session_lock);
	/*
	 * If DP_STATE_INITIALIZED is not set, we should not do any HW
	 * programming.
	 */
	if (!(dp->state & DP_STATE_INITIALIZED)) {
		DP_ERR("[host not ready]");
		goto end;
	}

	if (dp->edp_panel && !dp->edp_panel_ready) {
		drm_panel_prepare(dp->edp_panel);
		msleep(50);
		dp->edp_panel_ready = true;
		DP_INFO("drm_panel_prepare ready");
	}

	//trilin_dp_sink_set_msa_timing_ignore(dp, dp_panel, true);

	rc = trilin_dp_ctrl_stream_on(dp, dp_panel);
	if (rc)
		goto end;

	dp->state |= DP_STATE_ENABLED;
end:
	mutex_unlock(&dp->session_lock);
	return rc;
}

int trilin_dp_post_enable(struct trilin_dp *dp,
			  struct trilin_dp_panel *dp_panel)
{
	//int rc = trilin_dp_train_loop(dp);
	return 0;
}

int trilin_dp_pre_disable(struct trilin_dp *dp, struct trilin_dp_panel *panel)
{
	int rc = 0;

	DP_DEBUG("enter\n");
	mutex_lock(&dp->session_lock);
	if (!(dp->state & DP_STATE_ENABLED)) {
		DP_DEBUG("[not enabled]");
		goto end;
	}
	//fixme: clear_colorspaces(dp_display)?
	rc = trilin_dp_stream_pre_disable(dp, panel);
end:
	mutex_unlock(&dp->session_lock);
	return rc;
}

int trilin_dp_disable(struct trilin_dp *dp, struct trilin_dp_panel *panel)
{
	int rc = 0;

	DP_DEBUG("enter\n");
	mutex_lock(&dp->session_lock);

	if (!(dp->state & DP_STATE_ENABLED)) {
		DP_DEBUG("[not enabled]");
		goto end;
	}

	if (!(dp->state & DP_STATE_INITIALIZED)) {
		DP_DEBUG("[not ready]");
		goto end;
	}

	trilin_dp_stream_disable(dp, panel);
	trilin_dp_clear_info(dp, panel);
	trilin_dp_sink_set_msa_timing_ignore(dp, panel, false);
end:
	mutex_unlock(&dp->session_lock);
	return rc;
}

int trilin_dp_unprepare(struct trilin_dp *dp)
{
	DP_DEBUG("enter\n");

	mutex_lock(&dp->session_lock);
	if (!(dp->state & DP_STATE_ENABLED)) {
		DP_DEBUG("[not enabled]");
		goto end;
	}

	if (dp->active_stream_cnt) {
		DP_DEBUG("having active stream, do not core off");
		goto end;
	}

	trilin_dp_core_off(dp);

	if (dp->state & DP_STATE_SUSPENDED)
		trilin_dp_host_deinit(dp);

	dp->state &= ~DP_STATE_ENABLED;
	/* log this as it results from user action of cable dis-connection */
	DP_DEBUG("[OK]\n");
end:
	mutex_unlock(&dp->session_lock);
	return 0;
}

int trilin_dp_init_config(struct trilin_dp *dp)
{
	int rc;

	DP_DEBUG("enter\n");
	/*work queue*/
	INIT_DELAYED_WORK(&dp->hpd_event_work, trilin_dp_hpd_event_work_func);
	INIT_DELAYED_WORK(&dp->hpd_irq_work, trilin_dp_hpd_irq_work_func);

	/* config dp state*/
	mutex_init(&dp->session_lock);
	dp->state |= (DP_STATE_CONFIGURED);

	rc = trilin_dp_aux_register(dp);
	if (rc) {
		DP_DEBUG("failed to register DP aux\n");
		goto end1;
	}

	/* first call host init */
	if (dp->enabled_by_gop) {
		trinlin_dp_panel_read_sink_caps(dp);
		if (!dp->caps.mst)
			rc = trilin_dp_host_init_from_bootloader(dp);
		else
			dp->enabled_by_gop = 0; //mst disable gop
	}
	if (!dp->enabled_by_gop)
		rc = trilin_dp_host_init(dp);
	if (rc) {
		DP_ERR("host_init Failed");
		goto end2;
	}

	rc = trilin_dp_hdcp_init(dp->dpsub);
	if (rc) {
		DP_ERR("trilin_dp_hdcp_init Failed");
		goto end2;
	}

	irq_set_status_flags(dp->irq, IRQ_DISABLE_UNLAZY);
	rc = devm_request_threaded_irq(dp->dev, dp->irq, NULL,
				       trilin_dp_irq_handler, IRQF_ONESHOT,
				       dev_name(dp->dev), dp);
	if (rc < 0)
		goto end2;

	DP_INFO("init hpd high is %d\n", trilin_dp_get_hpd_state(dp));
	return rc;
end2:
	trilin_dp_aux_cleanup(dp);
end1:
	dp->state &= ~DP_STATE_CONFIGURED;
	cancel_delayed_work_sync(&dp->hpd_irq_work);
	cancel_delayed_work_sync(&dp->hpd_event_work);
	return rc;
}

static int dptx_register_audio_device(struct trilin_dp *dp)
{
	struct dptx_audio *dptx_aud = &dp->dp_audio;
	struct hdmi_codec_pdata codec_data = {
		.ops = &dptx_audio_codec_ops,
		.spdif = 0,
		.i2s = 1,
		.max_i2s_channels = 8,
		.data = dp,
	};
	dptx_aud->running = false;
	dptx_aud->pdev = platform_device_register_data(
		dp->dev, HDMI_CODEC_DRV_NAME, PLATFORM_DEVID_AUTO, &codec_data,
		sizeof(codec_data));

	return PTR_ERR_OR_ZERO(dptx_aud->pdev);
}

static void dptx_unregister_audio_device(void *data)
{
	struct trilin_dp *dp = data;
	struct dptx_audio *dptx_aud = &dp->dp_audio;

	if (dptx_aud->pdev) {
		platform_device_unregister(dptx_aud->pdev);
		dptx_aud->pdev = NULL;
	}
}

int trilin_dp_hdcp_init(struct trilin_dpsub *dpsub)
{
	int ret;
	struct trilin_dp *dp = dpsub->dp;
	struct cix_hdcp *hdcp = &dp->hdcp;

	hdcp->aux = &dp->aux;
	hdcp->state = ST2_H0;
	hdcp->base_addr = dp->dp_iomem;
	hdcp->tmr_addr = hdcp->base_addr + TRILIN_DPTX_HDCP_HOST_TIMER;
	hdcp->timer_in_use = false;
	hdcp->opened = false;
	ret = cix_hdcp_init(hdcp);

	return ret;
}

void trilin_dp_hdcp_uninit(struct trilin_dpsub *dpsub)
{
	struct trilin_dp *dp = dpsub->dp;
	struct cix_hdcp *hdcp = &dp->hdcp;

	cix_hdcp_uninit(hdcp);
}

static int trilin_dp_gop_get(struct trilin_dp *dp)
{
	struct arm_smccc_res res;
	bool enabled_by_gop = 0;

	arm_smccc_smc(CIX_SIP_DP_GOP_CTRL, SKY1_SIP_DP_GOP_GET, 0, 0, 0, 0, 0,
		      0, &res);

	if (res.a0 & DP_GOP_MASK)
		enabled_by_gop = true;
	else
		enabled_by_gop = false;

	return enabled_by_gop;
}

static void trilin_dp_gop_set(void)
{
	struct arm_smccc_res res;
	int dp_gop_bit = 1;

	arm_smccc_smc(CIX_SIP_DP_GOP_CTRL, SKY1_SIP_DP_GOP_SET,
			dp_gop_bit << DP_GOP_SHIFT, 0, 0, 0, 0, 0, &res);
}

int trilin_dp_probe(struct trilin_dpsub *dpsub, struct drm_device *drm)
{
	struct platform_device *pdev = to_platform_device(dpsub->dev);
	struct trilin_dp *dp;
	struct resource *res_dp, *res_phy, *res_rcsu = NULL;
	struct device *dev;
	struct fwnode_handle *fwnode_edp_panel;
	struct drm_panel *edp_panel;
	int ret;
	const char *str_prop;
	u32 is_insmod = 0;
	u32 psr_default_on = 1;

	dev = &pdev->dev;
	fwnode_edp_panel = fwnode_find_reference(dev->fwnode, "edp-panel", 0);
	if (!IS_ERR_OR_NULL(fwnode_edp_panel)) {
		edp_panel = fwnode_drm_find_panel(fwnode_edp_panel);
		if (IS_ERR(edp_panel)) {
			dev_err(dev, "%s, invalid drm_panel\n", __func__);
			return PTR_ERR(edp_panel);
		} else
			dev_info(dev, "%s, edp-panel %s is found\n", __func__,
				 dev_name(edp_panel->dev));
	}

	dp = devm_kzalloc(dpsub->dev, sizeof(*dp), GFP_KERNEL);
	if (!dp)
		return -ENOMEM;

	ret = device_property_read_string(dev, "support_d3_cmd", &str_prop);

	if (ret < 0) {
		/*if there is no this property ,default as support d3 cmd*/
		dp->support_d3_cmd = true;
	} else {
		if (strcmp("yes", str_prop) == 0)
			dp->support_d3_cmd = true;
		else
			dp->support_d3_cmd = false;
	}

	dp->dev = dev;
	dp->dpsub = dpsub;
	dp->status = connector_status_unknown;

	if (!dpsub->drm[0]) {
		dpsub->drm[0] = drm;
	}

	dp->drm = dpsub->drm; //DP_DEBUG needs dp->drm[0]

	dp->num_lanes = TRILIN_DPTX_MAX_LANES;
	dp->max_rate = DP_HIGH_BIT_RATE3;
	dp->max_streams = 2;
	dp->state = DP_STATE_DISCONNECTED;
	dp->platform_id = CIX_PLATFORM_SOC;
#ifdef CONFIG_ARCH_CIX_EMU_FPGA
	dp->platform_id = CIX_PLATFORM_FPGA;
#endif
	dp->force_pixel_per_cycle = 0;
	dp->pixel_per_cycle = 1;
	dp->edp_panel = edp_panel;
	dpsub->dp = dp;
	//drm->dev_private = dp;
	/* Acquire all resources (IOMEM, IRQ and PHYs). */
	if (has_acpi_companion(dev)) {
		res_dp = platform_get_resource(pdev, IORESOURCE_MEM,
					       DPTX_MEM_DP_IDX);
		res_phy = platform_get_resource(pdev, IORESOURCE_MEM,
						DPTX_MEM_DP_PHY_IDX);
		res_rcsu = platform_get_resource(pdev, IORESOURCE_MEM,
						DPTX_MEM_DP_RCSU_IDX);
	} else {
		res_dp = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						      "dp");
		res_phy = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						       "dp_phy");
		res_rcsu = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						       "dp_rcsu");
	}

	dp->dp_iomem = devm_ioremap_resource(dev, res_dp);
	if (IS_ERR(dp->dp_iomem))
		return PTR_ERR(dp->dp_iomem);

	dp->phy_iomem = devm_ioremap_resource(dev, res_phy);
	if (IS_ERR(dp->phy_iomem))
		return PTR_ERR(dp->phy_iomem);

	if (res_rcsu) {
		dp->rcsu_iomem = devm_ioremap_resource(dev, res_rcsu);
		if (IS_ERR(dp->rcsu_iomem))
			DP_WARN("rcsu iomem not set.\n");
	}

	dp->irq = platform_get_irq(pdev, 0);
	if (dp->irq < 0)
		return dp->irq;

	device_property_read_u32(dev, "cix,platform-id", &dp->platform_id);
	device_property_read_u32(dev, "cix,dp-lane-number", &dp->num_lanes);
	device_property_read_u32(dev, "cix,dp-max-rate", &dp->max_rate);
	device_property_read_u32(dev, "cix,aux-clock-divider",
				 &dp->aux_clock_divider);
	device_property_read_u8(dev, "pixel_per_cycle",
				&dp->force_pixel_per_cycle);
	device_property_read_u32(dev, "cix,delay_after_hpd",
				 &dp->delay_after_hpd);
	device_property_read_u32(dev, "cix,dp-max-stream", &dp->max_streams);
	device_property_read_u32(dev, "enabled_by_gop", &dp->enabled_by_gop);
	/*read dpu reseve memory*/
	is_insmod = trilin_dp_gop_get(dp);
	if (is_insmod)
		dp->enabled_by_gop = 0;

	device_property_read_u32(dev, "cix,dp-psr-default-on", &psr_default_on);
	dp->psr_default_on = !!psr_default_on;

	dp->fasttrain_default_on =
			device_property_read_bool(dev, "cix,dp-fasttrain-default-on");

	dp->mst_default_on =
			device_property_read_bool(dev, "cix,dp-mst-default-on");

	dp->cfg_adapter_port = 0;
	device_property_read_u32(dev, "cfg_adapter_port", &dp->cfg_adapter_port);

	dp->max_mst_encoders = dp->max_streams;
	dp->link_config.max_lanes = dp->num_lanes;
	dp->link_config.max_rate = dp->max_rate;

	dp->phy_reset = devm_reset_control_get(dev, "phy_reset");
	dp->reset = devm_reset_control_get(dev, "dp_reset");

	dpsub->apb_clk = devm_clk_get(dev, "apb_clk");
	dpsub->vid_clk0 = devm_clk_get(dev, "vid_clk0");
	dpsub->vid_clk1 = devm_clk_get(dev, "vid_clk1");
	dpsub->vid_clk2 = devm_clk_get(dev, "vid_clk2");
	dpsub->vid_clk3 = devm_clk_get(dev, "vid_clk3");

	ret = dptx_register_audio_device(dp);
	if (ret)
		DP_INFO("Failed to register dptx audio device\n");

	DP_INFO("end\n");
	return ret;
}

void trilin_dp_remove(struct trilin_dpsub *dpsub)
{
	struct trilin_dp *dp = dpsub->dp;
	//cix_hdcp_uninit(&dp->hdcp);
	dptx_unregister_audio_device(dp);
	trilin_dp_deinit_config(dp);

	/*set reserve memory*/
	trilin_dp_gop_set();
	DP_INFO("end\n");
}

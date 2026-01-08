// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_edid.h>
#include <drm/drm_managed.h>
#include <drm/drm_modes.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/kernel.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_dptx.h"
#include "trilin_drm.h"
#include "trilin_drm_mst.h"

#include "../linlon-dp/linlondp_pipeline.h"
#include "../linlon-dp/linlondp_dev.h"
#include "../linlon-dp/linlondp_kms.h"

#define ADJUST_BACKPORCH 1
#define INVERSE_VSYNC 1
#define GET_EDID_RETRY_MAX 50
/*adjust vfp: 1 is kernel and 0 is user*/
#define VRR_ADJUST_VFP_FROM_KERNEL 0

static const int DP_COMMON_LINK_RATES[] = {
	DP_REDUCED_BIT_RATE,
	DP_HIGH_BIT_RATE,
	DP_HIGH_BIT_RATE2,
	DP_HIGH_BIT_RATE3
};
//------------------------------------------------------------------------------
//  Module parameters
//------------------------------------------------------------------------------
static inline struct trilin_dp *encoder_to_dp(struct drm_encoder *encoder)
{
	struct trilin_encoder *enc = encoder_to_trilin(encoder);

	return enc->dp;
}

static inline struct trilin_dp *connector_to_dp(struct drm_connector *connector)
{
	struct trilin_connector *conn = connector_to_trilin(connector);

	return conn->dp;
}

/* -----------------------------------------------------------------------------
 * DRM Connector
 */
static bool trilin_dp_vrr_is_capable(struct trilin_dp *dp,
				struct drm_connector *connector)
{
	const struct drm_display_info *info = &connector->display_info;

	DP_DEBUG("vrr caps:%d max_vfreq=%d min_vfreq=%d", dp->caps.vrr,
			info->monitor_range.max_vfreq, info->monitor_range.min_vfreq);

	return dp->caps.vrr &&
		info->monitor_range.max_vfreq - info->monitor_range.min_vfreq > 10;
}

static enum drm_connector_status
trilin_dp_connector_detect(struct drm_connector *connector, bool force)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	int rc;
	enum drm_connector_status real_status;
	bool vrr_capable = false;

	DP_DEBUG("enter\n");

	mutex_lock(&dp->session_lock);
	if (dp->state & DPTX_STATE_SUSPENDED) {
		DP_DEBUG("DPTX_STATE_SUSPENDED return\n");
		goto end;
	}

	rc = trilin_dp_host_init(dp);
	if (rc)
		goto end;

	mutex_unlock(&dp->session_lock);

	if (!trilin_dp_get_hpd_state(dp)) {
		trilin_dp_handle_disconnect(dp, false);
		real_status = dp->status = connector_status_disconnected;
	} else {
		trilin_dp_handle_connect(dp, false);
		real_status = connector_status_connected;
		if (dp->mst.mst_active) {
			DP_DEBUG(
				"mst device that base connector cannot be used.\n");
			dp->status = connector_status_disconnected;
		} else {
			dp->status = connector_status_connected;
		}
		vrr_capable = trilin_dp_vrr_is_capable(dp, connector);
		DP_DEBUG("[CONNECTOR:%d:%s] VRR capable: %d\n",
			connector->base.id, connector->name, vrr_capable);
		drm_connector_set_vrr_capable_property(connector, vrr_capable);
	}
	drm_dp_set_subconnector_property(connector, real_status, dp->dpcd,
					 dp->downstream_ports);
	return dp->status;
end:
	mutex_unlock(&dp->session_lock);
	dp->status = connector_status_disconnected;
	return dp->status;
}

bool trilin_dp_plugged_status(struct trilin_dp *dp)
{
	return (dp->status == connector_status_connected);
}

static int trilin_dp_connector_atomic_check(struct drm_connector *conn,
					    struct drm_atomic_state *state)
{
	struct drm_connector_state *new_con_state =
		drm_atomic_get_new_connector_state(state, conn);
	struct drm_connector_state *old_con_state =
		drm_atomic_get_old_connector_state(state, conn);
	struct drm_crtc *crtc = new_con_state->crtc;
	struct drm_crtc_state *new_crtc_state;
	struct trilin_dp *dp = connector_to_dp(conn);
	struct trilin_connector *trilin_conn = connector_to_trilin(conn);
	int ret;

	DP_DEBUG("enter\n");

	if (dp->mst_mgr != NULL) {
		ret = drm_dp_mst_root_conn_atomic_check(new_con_state,
							dp->mst_mgr);
		if (ret < 0)
			return ret;
	}

	if (!crtc)
		return 0;

	if (dp->caps.psr_sink_support && dp->psr_config_on)
		new_con_state->self_refresh_aware = true;
	else
		new_con_state->self_refresh_aware = false;

	new_crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(new_crtc_state))
		return PTR_ERR(new_crtc_state);

	if (new_crtc_state->self_refresh_active && !dp->psr.enable) {
		DP_WARN("self_refresh_active is true but no psr sink support");
		return -EINVAL;
	}

	if (!dp->caps.vsc_supported)
		return 0;
	/*
	 * DC considers the stream backends changed if the
	 * static metadata changes. Forcing the modeset also
	 * gives a simple way for userspace to switch from
	 * 8bpc to 10bpc when setting the metadata to enter
	 * or exit HDR.
	 */
	if (new_con_state->colorspace != old_con_state->colorspace) {
		new_crtc_state->mode_changed = true;
		DP_DEBUG("colorspace changed from %d to %d",
			 old_con_state->colorspace, new_con_state->colorspace);
	}

	if (!drm_connector_atomic_hdr_metadata_equal(old_con_state,
						     new_con_state)) {
		if (new_con_state->hdr_output_metadata) {
			ret = drm_hdmi_infoframe_set_hdr_metadata(
				&trilin_conn->drm_infoframe, new_con_state);
			if (ret) {
				DP_ERR("couldn't set HDR metadata in infoframe\n");
				return ret;
			}
			trilin_dp_panel_setup_hdr_sdp(dp,
						      trilin_conn->dp_panel);
			DP_DEBUG("metadata changed.");
		}

		/* Changing the static metadata after it's been
		 * set is permissible, however. So only force a
		 * modeset if we're entering or exiting HDR.
		 */
		//new_crtc_state->mode_changed =
		//	!old_con_state->hdr_output_metadata ||
		//	!new_con_state->hdr_output_metadata;
		DP_DEBUG(
			"metadata changed between 0 and validate value. Do not change mode.");
	}

	return 0;
}

int trilin_connector_update_modes(struct drm_connector *connector,
				  struct edid *edid)
{
	int ret;
	struct trilin_dp *dp = connector_to_dp(connector);

	DP_DEBUG("enter\n");
	drm_connector_update_edid_property(connector, edid);
	ret = drm_add_edid_modes(connector, edid);

	return ret;
}

static const struct drm_display_mode trilin_drm_dmt_modes[] = {
	/* 0x04 - 640x480@60Hz */
	{ DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656, 752, 800,
		   0, 480, 490, 492, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x09 - 800x600@60Hz */
	{ DRM_MODE("800x600", DRM_MODE_TYPE_DRIVER, 40000, 800, 840, 968, 1056,
		   0, 600, 601, 605, 628, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x10 - 1024x768@60Hz */
	{ DRM_MODE("1024x768", DRM_MODE_TYPE_DRIVER, 65000, 1024, 1048, 1184,
		   1344, 0, 768, 771, 777, 806, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x55 - 1280x720@60Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390, 1430,
		   1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x52 - 1920x1080@60Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008, 2052,
		   2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x51 - 3840x2160@60Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016, 4104,
		   4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	/* 0x52 - 3840x2160@90Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 891000, 3840, 4016, 4104,
		   4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x53 - 3840x2160@120Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 1075804, 3840, 3848, 3880,
		   3920, 0, 2160, 2273, 2281, 2287, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
	/* 0x54 - 3840x1080@90Hz 16:9 */
	{ DRM_MODE("3840x1080", DRM_MODE_TYPE_DRIVER, 397605, 3840, 3848, 3880,
		   3920, 0, 1080, 1113, 1121, 1127, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
};

static int trilin_dp_add_virtual_modes_noedid(struct drm_connector *connector)
{
	int i, count, num_modes = 0;
	struct drm_display_mode *mode;
	struct drm_display_mode *preferred_mode;
	struct drm_device *dev = connector->dev;
	struct trilin_dp *dp = connector_to_dp(connector);

	preferred_mode = list_first_entry(&connector->probed_modes,
					  struct drm_display_mode, head);
	dp->my_copied_modes = 0;
	list_for_each_entry(mode, &connector->probed_modes, head) {
		dp->my_copied_modes++;
	}
	DP_DEBUG("preferred_mode: %s %d\n", preferred_mode->name, preferred_mode->clock);

	count = ARRAY_SIZE(trilin_drm_dmt_modes);
	for (i = 0; i < count; i++) {
		const struct drm_display_mode *ptr = &trilin_drm_dmt_modes[i];

		if (ptr->hdisplay >= preferred_mode->hdisplay)
			continue;

		if (ptr->vdisplay >= preferred_mode->vdisplay)
			continue;

		mode = drm_mode_duplicate(dev, ptr);
		if (mode) {
			drm_mode_probed_add(connector, mode);
			num_modes++;
		}
	}

	return num_modes;
}

static int trilin_dp_connector_get_modes(struct drm_connector *connector)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	struct drm_display_info *info = &connector->display_info;
	struct drm_display_mode *mode;
	struct edid *edid;
	int ret = 0;

	DP_DEBUG("enter\n");

	if (dp->platform_id != CIX_PLATFORM_EMU) {
		edid = drm_get_edid(connector, &dp->aux.ddc);
		if (!edid) { /* try once again */
			for (int count = 0; count < GET_EDID_RETRY_MAX;
			     count++) {
				msleep(20);
				edid = drm_get_edid(connector, &dp->aux.ddc);
				if (edid)
					break;
			}
		}

		if (!edid) {
			mode = drm_dp_downstream_mode(connector->dev, dp->dpcd,
						      dp->downstream_ports);
			if (mode) {
				drm_mode_probed_add(connector, mode);
				ret++;
			}
			if (ret == 0) {
				/* fall back to be 1080p */
				ret = drm_add_modes_noedid(connector, 1920,
							   1080);
				drm_set_preferred_mode(connector, 1920, 1080);
				DP_INFO("edid is null and read downstream: count=%d",
					ret);
			}
			return ret;
		}

		ret = trilin_connector_update_modes(connector, edid);
		kfree(edid);

		if (connector->connector_type == DRM_MODE_CONNECTOR_eDP)
			ret += trilin_dp_add_virtual_modes_noedid(connector);
	} else {
		ret = drm_add_modes_noedid(connector, 4096, 4096);
		drm_set_preferred_mode(connector, 640, 480);
	}
	DP_DEBUG("mode count = %d bpc=%d\n", ret, info->bpc);
	return ret;
}

static struct drm_encoder *
trilin_dp_connector_best_encoder(struct drm_connector *connector)
{
	struct trilin_dp *dp = connector_to_dp(connector);

	DP_DEBUG("enter\n");
	return &dp->encoder.base;
}

enum drm_mode_status
trilin_dp_connector_mode_valid(struct drm_connector *connector,
			       struct drm_display_mode *mode)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	//struct trilin_connector *conn = connector_to_trilin(connector);
	const struct drm_display_info *info = &connector->display_info;
	u8 max_lanes = dp->link_config.max_lanes;
	u8 minbpp = 6 * 3; //Fixme: minbpp config from YUV andr RGB format;
	int max_rate = dp->link_config.max_rate;
	int rate;
	int clock = mode->clock;
	u8 pixel_per_cycle = 1;
	//DP_DEBUG("enter\n");

	if (dp->force_pixel_per_cycle != 0)
		pixel_per_cycle = dp->force_pixel_per_cycle;

	if ((clock > TRILIN_MAX_FREQ) && (clock < TRILIN_MAX_FREQ * 2)
		&& !dp->mst.mst_active)
		pixel_per_cycle = 2;

	if (connector->ycbcr_420_allowed && drm_mode_is_420_only(info, mode)) {
		minbpp = 8 * 3 / 2;
		pixel_per_cycle = 1;
	}

	if (pixel_per_cycle == 1 && clock > TRILIN_MAX_FREQ) {
		DP_INFO("filtered mode(%s@%d) for high pixel rate\n",
			mode->name, clock);
		drm_mode_debug_printmodeline(mode);
		return MODE_CLOCK_HIGH;
	}

	/* Check with link rate and lane count */
	rate = trilin_dp_max_rate(max_rate, max_lanes, minbpp);
	if (clock > rate) {
		DP_INFO("filtered mode (%s@%d) for high bandwidth rate=%d: pixel_per_cycle=%d\n",
			mode->name, clock, rate, pixel_per_cycle);
		drm_mode_debug_printmodeline(mode);
		return MODE_CLOCK_HIGH;
	}
	//DP_DEBUG("mode name %s, mode->clock %d, rate %d conn->config.bpp=%d\n",
	//		mode->name, mode->clock, rate, conn->config.bpp);
	return MODE_OK;
}

void trilin_dp_connector_reset(struct drm_connector *connector)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	//linlon-dp call drm_mode_config_reset will reset state again. Skip it.
	DP_DEBUG("enter. Note: This is empty function. It's too late for linlon-dp to reset connector\n");
}

int trilin_dp_fill_modes(struct drm_connector *connector, uint32_t maxX,
			 uint32_t maxY)
{
	struct trilin_dp *dp = connector_to_dp(connector);

	DP_DEBUG("enter: %d\n", connector->base.id);
	return drm_helper_probe_single_connector_modes(connector, maxX, maxY);
}

static int mst_info_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct trilin_connector *conn = connector_to_trilin(connector);

	if (connector->status != connector_status_connected) {
		seq_puts(m, "not connected\n");
		return -ENODEV;
	}

	//trilin_dp_dump_regs(conn->dp);

	if (!conn || conn->type != TRILIN_OUTPUT_DP_MST) {
		seq_puts(m, "not TRILIN_OUTPUT_DP_MST\n");
		return -ENODEV;
	}

	seq_printf(m, "MST Source Port [conn->port:%d]\n",
		   conn->port->port_num);

	if (conn->dp->mst_mgr)
		drm_dp_mst_dump_topology(m, conn->dp->mst_mgr);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(mst_info);

static int register_info_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct trilin_connector *conn = connector_to_trilin(connector);

	if (connector->status != connector_status_connected) {
		seq_puts(m, "not connected\n");
		return -ENODEV;
	}

	trilin_dp_dump_regs(m, conn->dp);
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(register_info);

static int link_rate_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = connector_to_dp(connector);
	u8 link_status[DP_LINK_STATUS_SIZE];
	int ret;

	if (connector->status != connector_status_connected) {
		seq_puts(m, "not connected\n");
		return -ENODEV;
	}

	seq_printf(m, "link rate: %d lanes: %d\n", conn->dp->mode.link_rate,
		   conn->dp->mode.lane_cnt);

	seq_printf(m, "clock_recovery_ok : %s\nchannel_eq_ok : %s\n",
			dp->train_cr_done ? "yes" : "no",
			dp->train_ce_done ? "yes" : "no");

	seq_puts(m, "\n------------ dpcd status ------------\n");
	ret = drm_dp_dpcd_read_link_status(&dp->aux, link_status);
	if (ret < 0) {
		seq_printf(m, "read link status failed\n");
	} else {
		seq_printf(m, "DP_LANE0_1_STATUS : %x\nDP_LANE2_3_STATUS: %x\n",
			    link_status[0], link_status[1]);
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(link_rate);

void trilin_dp_connector_debugfs_init(struct drm_connector *connector,
				      struct dentry *root)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	debugfs_create_file("mst_topology", 0444, root, connector,
			    &mst_info_fops);
	debugfs_create_file("register_info", 0444, root, connector,
			    &register_info_fops);
	debugfs_create_file("link_rate", 0444, root, connector,
			    &link_rate_fops);
	debugfs_create_bool("psr_default_on", 0644, root,
			    &conn->dp->psr_default_on);
	debugfs_create_bool("mst_default_on", 0644, root,
			    &conn->dp->mst_default_on);
}

static const struct drm_connector_funcs trilin_dp_connector_funcs = {
	.detect = trilin_dp_connector_detect,
	.fill_modes = trilin_dp_fill_modes,
	.destroy = drm_connector_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.reset = trilin_dp_connector_reset,
	.debugfs_init = trilin_dp_connector_debugfs_init,
};

static const struct drm_connector_helper_funcs
	trilin_dp_connector_helper_funcs = {
		.get_modes = trilin_dp_connector_get_modes,
		.best_encoder = trilin_dp_connector_best_encoder,
		.mode_valid = trilin_dp_connector_mode_valid,
		.atomic_check = trilin_dp_connector_atomic_check,
	};

/* -----------------------------------------------------------------------------
 * DRM Encoder
 */
static
struct drm_crtc *get_crtc_from_encoder(struct drm_encoder *encoder,
					  struct drm_atomic_state *state)
{
	struct drm_connector *connector;
	struct drm_connector_state *conn_state;

	connector = drm_atomic_get_new_connector_for_encoder(state, encoder);
	if (!connector)
		return NULL;

	conn_state = drm_atomic_get_new_connector_state(state, connector);
	if (!conn_state)
		return NULL;

	return conn_state->crtc;
}

static bool trilin_dp_handle_psr_disable(struct drm_encoder *encoder,
				struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_dp_panel *dp_panel = &dp->dp_panel;

	crtc = get_crtc_from_encoder(encoder, state);
	if (!crtc)
		return false;

	old_crtc_state = drm_atomic_get_old_crtc_state(state, crtc);

	/* Not a full enable, just disable PSR and continue */
	if (old_crtc_state && old_crtc_state->self_refresh_active) {
		trilind_dp_psr_disable(dp, dp_panel);
		return true;
	}
	return false;
}

static bool is_same_mode_compare(struct trilin_dp *dp,
				 struct trilin_dp_panel *dp_panel)
{
	struct trilin_connector *conn = dp_panel->connector;
	struct drm_connector *base = &conn->base;
	struct drm_crtc *crtc = base->state->crtc;
	struct drm_crtc_state *crtc_state = crtc->state;
	struct drm_display_mode *mode = &crtc_state->adjusted_mode;
	u32 htotal, vtotal;
	u32 hswidth, vswidth, hres, vres;
	u32 hstart, vstart;
	u32 link_rate, sec_data_window, sec_data_window_comp;
	u32 regs_off = TRILIN_DPTX_SOURCE_OFFSET * dp_panel->stream_id;

	htotal = trilin_dp_read(dp,
				TRILIN_DPTX_SRC0_MAIN_STREAM_HTOTAL + regs_off);
	vtotal = trilin_dp_read(dp,
				TRILIN_DPTX_SRC0_MAIN_STREAM_VTOTAL + regs_off);
	hswidth = trilin_dp_read(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_HSWIDTH +
					     regs_off);
	vswidth = trilin_dp_read(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_VSWIDTH +
					     regs_off);
	hres = trilin_dp_read(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_HRES + regs_off);
	vres = trilin_dp_read(dp, TRILIN_DPTX_SRC0_MAIN_STREAM_VRES + regs_off);
	hstart = trilin_dp_read(dp,
				TRILIN_DPTX_SRC0_MAIN_STREAM_HSTART + regs_off);
	vstart = trilin_dp_read(dp,
				TRILIN_DPTX_SRC0_MAIN_STREAM_VSTART + regs_off);
	/* Is clock same? */
	link_rate = trilin_dp_read(dp,
				TRILIN_DPTX_LINK_BW_SET + regs_off);
	link_rate = drm_dp_bw_code_to_link_rate(link_rate);
	sec_data_window = trilin_dp_read(dp,
				TRILIN_DPTX_SRC0_SECONDARY_DATA_WINDOW + regs_off);
	sec_data_window_comp = (mode->htotal - mode->hdisplay) *
			  (link_rate / 40) * 9 / mode->clock;

	if (htotal != mode->htotal || vtotal != mode->vtotal ||
		hres != mode->hdisplay || vres != mode->vdisplay ||
	    (mode->hsync_end - mode->hsync_start != hswidth) ||
	    (mode->vsync_end - mode->vsync_start != vswidth) ||
	    (mode->htotal - mode->hsync_start != hstart) ||
	    (mode->vtotal - mode->vsync_start != vstart) ||
	    sec_data_window != sec_data_window_comp) {
		return false;
	}
	return true;
}

static void trilin_dp_encoder_enable(struct drm_encoder *encoder,
				struct drm_atomic_state *state)
{
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_dp_panel *dp_panel = &dp->dp_panel;
	struct trilin_connector *conn = dp_panel->connector;

	int rc = 0;

	if (trilin_dp_handle_psr_disable(encoder, state)) {
		//DP_DEBUG("Not a full enable, just disable PSR and continue\n");
		return;
	}

	DP_INFO("enter\n");

	if (dp->enabled_by_gop) {
		if (is_same_mode_compare(dp, dp_panel) && !conn->vrr.enable) {
			trilin_dp_write(dp, TRILIN_DPTX_INTERRUPT_MASK,
					TRILIN_DPTX_INTERRUPT_CFG);
		} else {
			mutex_lock(&dp->session_lock);
			dp->state &= ~DPTX_STATE_INITIALIZED;
			dp->enabled_by_gop = 0;
			mutex_unlock(&dp->session_lock);
			DP_INFO("reset dp->state for gop\n");
		}
	}

	/*link trainning*/
	rc = trilin_dp_prepare(dp);
	if (rc) {
		DP_ERR("DP display prepare failed, rc=%d\n", rc);
		return;
	}

	/*stream on*/
	trilin_dp_set_stream_info(dp, dp_panel, 0, 0, 0);

	rc = trilin_dp_enable(dp, dp_panel);
	if (rc) {
		DP_ERR("DP display enable failed, rc=%d\n", rc);
		return;
	}
	/*update hdr and hdcp ? */
	trilin_dp_post_enable(dp, dp_panel);
	dp->enabled_by_gop = 0;
}

static void trilin_dp_encoder_disable(struct drm_encoder *encoder,
				struct drm_atomic_state *state)
{
	int rc;
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_dp_panel *dp_panel = &dp->dp_panel;
	struct drm_crtc *crtc;
	struct drm_crtc_state *new_crtc_state;

	if (!(dp->state & DPTX_STATE_INITIALIZED)) {
		DP_DEBUG("[not init]");
		return;
	}

	crtc = get_crtc_from_encoder(encoder, state);
	if (crtc)
		new_crtc_state = drm_atomic_get_new_crtc_state(state, crtc);

	/* Don't do a full disable on PSR transitions */
	if (new_crtc_state && new_crtc_state->self_refresh_active) {
		//DP_DEBUG("Don't do a full disable on PSR transitions");
		trilin_dp_psr_enable(dp, dp_panel);
		return;
	}

	DP_INFO("enter\n");
	rc = trilin_dp_pre_disable(dp, dp_panel);
	if (rc)
		DP_ERR("DP display pre disable failed, rc=%d\n", rc);

	rc = trilin_dp_disable(dp, dp_panel);
	if (rc) {
		DP_ERR("DP display disable failed, rc=%d\n", rc);
		return;
	}

	rc = trilin_dp_unprepare(dp);
	if (rc) {
		DP_ERR("DP display unprepare failed, rc=%d\n", rc);
		return;
	}

	DP_DEBUG("end\n");
}

static u8 trilin_dp_cal_bpc(struct trilin_dp *dp, struct drm_connector_state *connector_state, u8 bpc)
{
	struct drm_display_info *info =
		&connector_state->connector->display_info;

	if ((connector_state->colorspace >= DRM_MODE_COLORIMETRY_BT2020_CYCC) &&
	    (connector_state->colorspace <= DRM_MODE_COLORIMETRY_BT2020_YCC)) {
		DP_DEBUG("colorspace is bt2020. force set bpc (%d) to 10",
			info->bpc);
		bpc = 10;
	}

	if (info->bpc && bpc > info->bpc) {
		DP_DEBUG("downgrading requested %ubpc to display limit %ubpc\n",
			bpc, info->bpc);
		bpc = info->bpc;
	}

	if (connector_state->max_requested_bpc &&
	    bpc > connector_state->max_requested_bpc) {
		DP_DEBUG("downgrading requested %ubpc to property limit %ubpc\n",
			bpc, connector_state->max_requested_bpc);
		bpc = connector_state->max_requested_bpc;
	}
	return bpc;
}

static u8 trilin_dp_cal_bpp(u8 bpc, int format)
{
	u8 bpp = bpc * 3;

	switch (format) {
	case DRM_COLOR_FORMAT_RGB444:
	case DRM_COLOR_FORMAT_YCBCR444:
		bpp = bpc * 3;
		break;
	case DRM_COLOR_FORMAT_YCBCR422:
		bpp = bpc * 2;
		break;
	case DRM_COLOR_FORMAT_YCBCR420:
		bpp = bpc * 3 / 2;
		break;
	default:
		pr_warn("Invalid format in DT.\n");
	}
	return bpp;
}

static void trilin_dp_vrr_config(struct trilin_dp *dp,
				struct trilin_connector *conn,
				struct drm_crtc_state *crtc_state,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_connector *connector = &conn->base;

	if (!trilin_dp_vrr_is_capable(dp, connector))
		return;

	if (adjusted_mode->flags & DRM_MODE_FLAG_INTERLACE)
		return;

	if (!crtc_state->vrr_enabled) {
		//DP_DEBUG("vrr disabled");
		return;
	}
#if VRR_ADJUST_VFP_FROM_KERNEL
	const struct drm_display_info *info = &connector->display_info;
	int vmin, vmax;
	int extension;
	int vrefresh;

	vmin = DIV_ROUND_UP(adjusted_mode->crtc_clock * 1000,
			    adjusted_mode->crtc_htotal *
				    info->monitor_range.max_vfreq);
	vmax = adjusted_mode->crtc_clock * 1000 /
		(adjusted_mode->crtc_htotal * info->monitor_range.min_vfreq);

	vmin = max_t(int, vmin, adjusted_mode->crtc_vtotal);
	vmax = max_t(int, vmax, adjusted_mode->crtc_vtotal);

	if (vmin >= vmax)
		return;

	conn->vrr.vmin = vmin;
	conn->vrr.vmax = vmax;

	extension = conn->vrr.vmax - adjusted_mode->vtotal;
	DP_DEBUG("adjusted_mode->vsync_start  %d  %d %d %d",
		adjusted_mode->vsync_start, adjusted_mode->vdisplay, extension,
		adjusted_mode->vtotal);
	adjusted_mode->vsync_start += extension;
	adjusted_mode->vsync_end += extension;
	adjusted_mode->vtotal += extension;
	vrefresh = adjusted_mode->clock * 1000 /
		(adjusted_mode->vtotal * adjusted_mode->htotal);
	DP_DEBUG(
		"vmin: %d vmax:%d VRR enable adjusted_mode->vtotal adjust form %d to %d vrefresh=%d",
		vmin, vmax, adjusted_mode->vtotal - extension,
		adjusted_mode->vtotal, vrefresh);
#endif
	conn->vrr.enable = true;
}

#define TRILIN_DPTX_MIN_H_BACKPORCH 12

int trilin_dp_encoder_atomic_adjust_mode(struct trilin_dp *dp,
		struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode)
{
	bool adjust_backporch = ADJUST_BACKPORCH; //&& !dp->mst.mst_active;
	int diff = mode->htotal - mode->hsync_end;
	/*
	 * Trilinear DP requires horizontal backporch to be greater than 12.
	 * This limitation may not be compatible with the sink device.
	 */
	if (diff < TRILIN_DPTX_MIN_H_BACKPORCH) {
		diff = TRILIN_DPTX_MIN_H_BACKPORCH - diff;
		if (adjust_backporch) {
			adjusted_mode->htotal += diff;
			adjusted_mode->clock = (int64_t)adjusted_mode->clock *
					       adjusted_mode->htotal /
					       (adjusted_mode->htotal - diff);
		}
		DP_WARN("Note: hbackporch should adjust: %d to %d ? %s\n", diff,
			TRILIN_DPTX_MIN_H_BACKPORCH - diff,
			adjust_backporch ? "YES" : "NO");
	}

	if (INVERSE_VSYNC) {
		if (adjusted_mode->flags & DRM_MODE_FLAG_PVSYNC)
			adjusted_mode->flags &= ~DRM_MODE_FLAG_PVSYNC;
		else
			adjusted_mode->flags |= DRM_MODE_FLAG_PVSYNC;

		if (adjusted_mode->flags & DRM_MODE_FLAG_PHSYNC)
			adjusted_mode->flags &= ~DRM_MODE_FLAG_PHSYNC;
		else
			adjusted_mode->flags |= DRM_MODE_FLAG_PHSYNC;

		DP_DEBUG("adjust_mode flags: 0x%0x adjust_mode: %s-%d mode: %s"
			, adjusted_mode->flags, adjusted_mode->name, adjusted_mode->clock, mode->name);
	}

	return 0;
}

static bool compute_available_clock_rate(struct trilin_dp *dp,
		struct drm_connector_state *connector_state, u8 suggest_bpc,
		int clock, int color_format, int *rt_bpc, int *rt_bpp)
{
	int min_bpc = (color_format == DRM_COLOR_FORMAT_RGB444) ? 6 : 8;
	int bpc = max(suggest_bpc, min_bpc);
	int bpp;
	u8 max_lanes = dp->link_config.max_lanes;
	int rate, max_rate = dp->link_config.max_rate;

	for (; bpc >= min_bpc; bpc = bpc - 2) {
		bpc = trilin_dp_cal_bpc(dp, connector_state, bpc);
		bpp = trilin_dp_cal_bpp(bpc, color_format);
		rate = trilin_dp_max_rate(max_rate, max_lanes, bpp);

		if (clock <= rate) {
			*rt_bpc = bpc;
			*rt_bpp = bpp;
			return true;
		}
	}
	return false;
}

int trilin_dp_encoder_compute_config(struct drm_encoder *encoder,
				  struct drm_crtc_state *crtc_state,
				  struct drm_connector_state *connector_state,
				  u8 suggest_bpc)
{
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct trilin_connector *conn = connector_to_trilin(connector_state->connector);
	struct trilin_dp *dp = encoder_to_dp(encoder);

	struct drm_display_info *info = &connector_state->connector->display_info;
	int ret, bpc, bpp;
	enum trilin_dpsub_format format = TRILIN_DPSUB_FORMAT_RGB;
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(crtc_state);
	int colorspace = connector_state->colorspace;
	int color_format;// = BIT(__ffs(info->color_formats));
	int info_formats = info->color_formats;
	bool success = false;
	int rate, max_rate = dp->link_config.max_rate;
	u8 link_rate_bw_code, i;
	u8 max_lanes = dp->link_config.max_lanes;
	int link_rate = max_rate;

	if (!(info_formats & DRM_COLOR_FORMAT_RGB444)) {
		info_formats |= DRM_COLOR_FORMAT_RGB444;
		DP_WARN("info_format=%0x, force support RGB444", info->color_formats);
	}

	const int COMMON_COLORS_FORMATS[] = {
		DRM_COLOR_FORMAT_RGB444, DRM_COLOR_FORMAT_YCBCR422, DRM_COLOR_FORMAT_YCBCR420};

	for (i = 0; i < ARRAY_SIZE(COMMON_COLORS_FORMATS); i++) {
		color_format = COMMON_COLORS_FORMATS[i];
		if (info_formats & color_format ||
			drm_mode_is_420_only(info, adjusted_mode)) {
			success = compute_available_clock_rate(dp, connector_state, suggest_bpc,
				adjusted_mode->clock, color_format, &bpc, &bpp);
			if (success) {
				if (color_format != DRM_COLOR_FORMAT_RGB444)
					DP_INFO("Use YUV Format=0x%0x", color_format);
				break;
			}
		}
	}

	if (!success) {
		DP_ERR("mode %s pixel rate %d is higher than max rate\n",
			adjusted_mode->name, adjusted_mode->clock);
		return -EINVAL;
	}

	switch (color_format) {
	case DRM_COLOR_FORMAT_YCBCR444:
		format = TRILIN_DPSUB_FORMAT_YCBCR444;
		break;
	case DRM_COLOR_FORMAT_YCBCR422:
		format = TRILIN_DPSUB_FORMAT_YCBCR422;
		break;
	case DRM_COLOR_FORMAT_YCBCR420:
		format = TRILIN_DPSUB_FORMAT_YCBCR420;
		break;
	default:
		format = TRILIN_DPSUB_FORMAT_RGB;
		break;
	}

	kcrtc_st->output_format = color_format; //Let DPU to know the format.

	if (!dp->mst.mst_active) {
		for (i = 0; i < ARRAY_SIZE(DP_COMMON_LINK_RATES); i++) {
			if (DP_COMMON_LINK_RATES[i] > max_rate)
				break;
			rate = trilin_dp_max_rate(DP_COMMON_LINK_RATES[i], max_lanes, bpp);
			if (adjusted_mode->clock <= rate) {
				link_rate = DP_COMMON_LINK_RATES[i];
				break;
			}
		}
	}

	link_rate_bw_code = drm_dp_link_rate_to_bw_code(link_rate);

	DP_DEBUG(
		"Note: disp->bpc(%d) bpc(%d) bpp(%d) colorspace=%d clock=%d format=0x%0x color_formats=%0x link_rate_bw=%0x %d request_max_bpc=%d\n",
		info->bpc, bpc, bpp, colorspace, adjusted_mode->clock,
		color_format, info->color_formats, link_rate_bw_code, link_rate,
		connector_state->max_requested_bpc);

	ret = trilin_dp_mode_configure(dp, adjusted_mode->clock, link_rate_bw_code, bpp, false);
	if (ret < 0)
		return -EINVAL;

	conn->config.format = format;
	conn->config.colorspace = colorspace;
	conn->config.bpc = bpc;
	conn->config.bpp = bpp;

	trilin_dp_vrr_config(dp, conn, crtc_state, adjusted_mode);
	return 0;
}

int trilin_dp_encoder_atomic_check(struct drm_encoder *encoder,
				  struct drm_crtc_state *crtc_state,
				  struct drm_connector_state *connector_state)
{
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_connector *conn = &dp->connector;
	struct drm_connector *connector = &conn->base;
	struct drm_display_info *info =
		&connector_state->connector->display_info;
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct drm_display_mode *crtc_mode = &crtc_state->mode;
	struct drm_display_mode *mode;
	struct drm_display_mode *preferred_mode = NULL;
	int i = 0;

	DP_DEBUG("enter\n");

	if (crtc_state->self_refresh_active && !crtc_state->vrr_enabled)
		return 0;

	if (connector->connector_type == DRM_MODE_CONNECTOR_eDP) {
		list_for_each_entry(mode, &connector->modes, head) {
			if (mode->hdisplay == adjusted_mode->hdisplay &&
				mode->vdisplay == adjusted_mode->vdisplay &&
				drm_mode_vrefresh(mode) == drm_mode_vrefresh(adjusted_mode)) {
				preferred_mode = mode;
				DP_DEBUG("same preferred_mode: %s %d", preferred_mode->name, preferred_mode->clock);
				break;
			} else if (mode->hdisplay >= adjusted_mode->hdisplay &&
				mode->vdisplay >= adjusted_mode->vdisplay &&
				drm_mode_vrefresh(mode) == drm_mode_vrefresh(adjusted_mode)) {
				preferred_mode = mode;
				DP_DEBUG("vfresh same preferred_mode: %s %d", preferred_mode->name, preferred_mode->clock);
				break;
			}
			if (++i >= dp->my_copied_modes) {
				break;
			}
		}

		if (preferred_mode == NULL) {
			preferred_mode = list_first_entry(&connector->modes,
						struct drm_display_mode, head);
			DP_DEBUG("use first preferred_mode");
		}

		drm_mode_copy(adjusted_mode, preferred_mode);
	}

	trilin_dp_encoder_atomic_adjust_mode(dp, crtc_mode, adjusted_mode);

	return trilin_dp_encoder_compute_config(encoder, crtc_state,
						connector_state, info->bpc);
}

static void trilin_dp_rcsu_cfg_adapter(struct trilin_dp *dp, struct drm_connector_state *connector_state)
{
	struct linlondp_crtc *crtc = to_kcrtc(connector_state->crtc);
	int pipe_id = 0;
	int offset;
	u32 val;
	struct trilin_connector *conn = &dp->connector;

	if (IS_ERR_OR_NULL(dp->rcsu_iomem))
		return;

	if (crtc && crtc->master)
		pipe_id = crtc->master->id;

	#define BIT31_30_MASK (3UL << 30)
	#define BIT18_17_MASK (3UL << 17)
	#define BIT21_20_MASK (3UL << 20)

	#define CFG_ADAPTER_VIDEO0_OFFSET 0x300
	#define CFG_ADAPTER_VIDEO1_OFFSET 0x304
	#define CFG_ADAPTER_VIDEO_OFFSET_DP2 0x320
	#define DP_PORT_2 2

	if (dp->cfg_adapter_port == DP_PORT_2) {
		/*For eDP/DP...*/
		offset = CFG_ADAPTER_VIDEO_OFFSET_DP2;
		val = readl(dp->rcsu_iomem + offset);
		val = pipe_id == 0 ? val & ~BIT18_17_MASK : val & ~BIT21_20_MASK;
		// if use yuv. set rcsu.
		if (conn->config.format == TRILIN_DPSUB_FORMAT_YCBCR422)
			val = pipe_id == 0 ? val | (2UL << 17) : val | (2UL << 20);
		else if (conn->config.format == TRILIN_DPSUB_FORMAT_YCBCR420)
			val = pipe_id == 0 ? val | (3UL << 17) : val | (3UL << 20);

		val = (pipe_id == 0 ? val & ~BIT(16) : val & ~BIT(19));
		if (dp->pixel_per_cycle == 2)
			val = (pipe_id == 0 ? val | BIT(16) : val | BIT(19));
		writel(val, dp->rcsu_iomem + offset);
	} else {
		/*For USBDP...*/
		offset = pipe_id == 0 ? CFG_ADAPTER_VIDEO0_OFFSET : CFG_ADAPTER_VIDEO1_OFFSET;
		val = readl(dp->rcsu_iomem + offset);
		val &= ~BIT31_30_MASK;
		// if use yuv. set rcsu.
		if (conn->config.format == TRILIN_DPSUB_FORMAT_YCBCR422)
			val |= (2UL << 30);
		else if (conn->config.format == TRILIN_DPSUB_FORMAT_YCBCR420)
			val |= (3UL << 30);

		val &= ~BIT(29);
		if (dp->pixel_per_cycle == 2)
			val |= BIT(29);
		writel(val, dp->rcsu_iomem + offset);
	}
	DP_DEBUG("set rcsu offset=%0x val=%0x", offset, val);
}

void trilin_dp_encoder_atomic_mode_set(
	struct drm_encoder *encoder, struct drm_crtc_state *crtc_state,
	struct drm_connector_state *connector_state)
{
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_connector *conn = &dp->connector;
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;

	DP_INFO("set mode: %s %d", adjusted_mode->name, adjusted_mode->clock);
	if (dp->force_pixel_per_cycle != 0) {
		dp->pixel_per_cycle = dp->force_pixel_per_cycle;
	} else {
		if ((adjusted_mode->clock > TRILIN_MAX_FREQ) &&
		    (adjusted_mode->clock < TRILIN_MAX_FREQ * 2) &&
			!dp->mst.mst_active &&
			conn->config.format != TRILIN_DPSUB_FORMAT_YCBCR420)
			dp->pixel_per_cycle = 2;
		else
			dp->pixel_per_cycle = 1;
	}
	trilin_dp_rcsu_cfg_adapter(dp, connector_state);

	if (dp->pixel_per_cycle == 2
		&& dp->psr_config_on && dp->caps.psr_sink_support) {
		dp->psr_config_on = false;
		DP_INFO("psr disable in 2ppc mode");
	} else {
		dp->psr_config_on = dp->psr_default_on;
		DP_DEBUG("psr_config_on: %d", dp->psr_config_on);
	}
	if (dp->caps.psr_sink_support && dp->psr_config_on)
		connector_state->self_refresh_aware = true;
	else
		connector_state->self_refresh_aware = false;
}

static const struct drm_encoder_helper_funcs trilin_dp_encoder_helper_funcs = {
	.atomic_enable = trilin_dp_encoder_enable,
	.atomic_disable = trilin_dp_encoder_disable,
	.atomic_mode_set = trilin_dp_encoder_atomic_mode_set,
	.atomic_check = trilin_dp_encoder_atomic_check,
};

static void trilin_dp_add_properties(struct trilin_dp *dp,
				     struct drm_connector *connector)
{
	if (!drm_mode_create_dp_colorspace_property(connector, 0))
		drm_connector_attach_colorspace_property(connector);
	drm_connector_attach_hdr_output_metadata_property(connector);

	connector->max_bpc_property = NULL;
	drm_connector_attach_max_bpc_property(connector, 8, 10); //Fixme: max 10 for dpu, but not dp.
	drm_connector_attach_dp_subconnector_property(connector);
	drm_connector_attach_content_type_property(connector);
	/*VRR */
	drm_connector_attach_vrr_capable_property(connector);
}

/* -----------------------------------------------------------------------------
 * Initialization & Cleanup
 */

static void trilin_dp_encoder_destroy(struct drm_encoder *encoder)
{
	struct trilin_dp *dp = encoder_to_dp(encoder);

	DP_DEBUG("enter");
	trilin_drm_mst_encoder_cleanup(dp);
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs trilin_dp_enc_funcs = {
	.destroy = trilin_dp_encoder_destroy,
};

int trilin_dp_drm_init(struct trilin_dpsub *dpsub)
{
	struct trilin_dp *dp = dpsub->dp;
	struct trilin_encoder *enc = &dp->encoder;
	struct trilin_connector *conn = &dp->connector;
	struct drm_encoder *encoder = &enc->base;
	struct drm_connector *connector = &conn->base;
	int ret;
	int drm_mode_connector = DRM_MODE_CONNECTOR_DisplayPort;

	DP_INFO("begin\n");

	enc->dp = dp;
	conn->dp = dp;
	conn->type = TRILIN_OUTPUT_DP;
	/* Create the DRM encoder and connector. */
	encoder->possible_crtcs = TRILIN_DPTX_POSSIBLE_CRTCS_SST;
	drm_encoder_init(dp->drm[0], encoder, &trilin_dp_enc_funcs,
			 DRM_MODE_ENCODER_TMDS, NULL);

	drm_encoder_helper_add(encoder, &trilin_dp_encoder_helper_funcs);

	connector->polled = DRM_CONNECTOR_POLL_HPD;
	if (dp->edp_panel)
		drm_mode_connector = DRM_MODE_CONNECTOR_eDP;

	ret = drm_connector_init(encoder->dev, connector,
				 &trilin_dp_connector_funcs,
				 drm_mode_connector);
	if (ret) {
		DP_ERR("failed to create the DRM connector\n");
		return ret;
	}

	drm_connector_helper_add(connector, &trilin_dp_connector_helper_funcs);
	drm_connector_register(connector);
	drm_connector_attach_encoder(connector, encoder);

	trilin_drm_mst_encoder_init(dp, connector->base.id);

	//for sst
	if (!IS_ERR_OR_NULL(dp->rcsu_iomem))
		connector->ycbcr_420_allowed = true;
	dp->dp_panel.connector = &dp->connector;
	dp->dp_panel.stream_id = 0;
	dp->connector.dp_panel = &dp->dp_panel;
	/*
	 * Some of the properties below require access to state, like bpc.
	 */
	drm_atomic_helper_connector_reset(connector);
	trilin_dp_add_properties(dp, connector);
	//dp hardware init now
	trilin_dp_init_config(dp);
	return 0;
}

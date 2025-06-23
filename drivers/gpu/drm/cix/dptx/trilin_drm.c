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

#define INVERSE_VSYNC 1
#define GET_EDID_RETRY_MAX 10
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

static enum drm_connector_status
trilin_dp_connector_detect(struct drm_connector *connector, bool force)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	int rc;
	enum drm_connector_status real_status;
	DP_DEBUG("enter\n");

	mutex_lock(&dp->session_lock);
	if (dp->state & DP_STATE_SUSPENDED) {
		DP_DEBUG("DP_STATE_SUSPENDED return\n");
		goto end;
	}

	rc = trilin_dp_host_init(dp);
	if (rc) {
		goto end;
	}

	mutex_unlock(&dp->session_lock);

	if (!trilin_dp_get_hpd_state(dp)) {
		trilin_dp_handle_disconnect(dp, false);
		real_status = dp->status = connector_status_disconnected;
	} else {
		trilin_dp_handle_connect(dp, false);
		real_status = connector_status_connected;
		if (dp->mst.mst_active) {
			DP_DEBUG("mst device that base connector cannot be used.\n");
			dp->status = connector_status_disconnected;
		} else {
			dp->status = connector_status_connected;
		}
	}
	drm_dp_set_subconnector_property(connector, real_status, dp->dpcd, dp->downstream_ports);
	return dp->status;
end:
	mutex_unlock(&dp->session_lock);
	dp->status = connector_status_disconnected;
	return dp->status;
}

static int
trilin_dp_connector_atomic_check(struct drm_connector *conn,
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
		ret = drm_dp_mst_root_conn_atomic_check(new_con_state, dp->mst_mgr);
		if (ret < 0)
			return ret;
	}

	if (!crtc)
		return 0;

	if (!dp->caps.vsc_supported)
		return 0;

	new_crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(new_crtc_state))
		return PTR_ERR(new_crtc_state);

	if (new_con_state->colorspace != old_con_state->colorspace) {
		new_crtc_state->mode_changed = true;
		DP_DEBUG("colorspace changed from %d to %d", old_con_state->colorspace, new_con_state->colorspace);
	}

	if (!drm_connector_atomic_hdr_metadata_equal(old_con_state, new_con_state)) {
		if (new_con_state->hdr_output_metadata) {
			ret = drm_hdmi_infoframe_set_hdr_metadata(&trilin_conn->drm_infoframe, new_con_state);
			if (ret) {
				DP_ERR("couldn't set HDR metadata in infoframe\n");
				return ret;
			}
			trilin_dp_panel_setup_hdr_sdp(dp, trilin_conn->dp_panel);
			DP_DEBUG("metadata changed.");
		}
		/*
		 * DC considers the stream backends changed if the
		 * static metadata changes. Forcing the modeset also
		 * gives a simple way for userspace to switch from
		 * 8bpc to 10bpc when setting the metadata to enter
		 * or exit HDR.
		 *
		 * Changing the static metadata after it's been
		 * set is permissible, however. So only force a
		 * modeset if we're entering or exiting HDR.
		 */
		new_crtc_state->mode_changed =
			!old_con_state->hdr_output_metadata ||
			!new_con_state->hdr_output_metadata;
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
    { DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656,
            752, 800, 0, 480, 490, 492, 525, 0,
            DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
    /* 0x09 - 800x600@60Hz */
    { DRM_MODE("800x600", DRM_MODE_TYPE_DRIVER, 40000, 800, 840,
            968, 1056, 0, 600, 601, 605, 628, 0,
            DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
    /* 0x10 - 1024x768@60Hz */
    { DRM_MODE("1024x768", DRM_MODE_TYPE_DRIVER, 65000, 1024, 1048,
            1184, 1344, 0, 768, 771, 777, 806, 0,
            DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
    /* 0x55 - 1280x720@60Hz */
    { DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
            1430, 1650, 0, 720, 725, 730, 750, 0,
            DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
    /* 0x52 - 1920x1080@60Hz */
    { DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
            2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
            DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
    /* 0x51 - 3840x2160@60Hz 16:9 */
    { DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
            4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
            DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
    /* 0x52 - 3840x2160@90Hz 16:9 */
    { DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 891000, 3840, 4016,
            4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
            DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
    /* 0x53 - 3840x2160@120Hz 16:9 */
    { DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 1075804, 3840, 3848,
            3880, 3920, 0, 2160, 2273, 2281, 2287, 0,
            DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
    /* 0x54 - 3840x1080@90Hz 16:9 */
    { DRM_MODE("3840x1080", DRM_MODE_TYPE_DRIVER, 397605, 3840, 3848,
            3880, 3920, 0, 1080, 1113, 1121, 1127, 0,
            DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC) },
};

static int trilin_dp_add_virtual_modes_noedid(struct drm_connector *connector)
{
	int i, count, num_modes = 0;
	struct drm_display_mode *mode;
	struct drm_display_mode *preferred_mode;
	struct drm_device *dev = connector->dev;

	preferred_mode = list_first_entry(&connector->probed_modes,
					struct drm_display_mode, head);

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
			for (int count = 0; count < GET_EDID_RETRY_MAX; count++) {
				msleep(20);
				edid = drm_get_edid(connector, &dp->aux.ddc);
				if (edid)
					break;
			}
		}

		if (!edid) {
			mode = drm_dp_downstream_mode(connector->dev,
							dp->dpcd,
							dp->downstream_ports);
			if (mode) {
				drm_mode_probed_add(connector, mode);
				ret++;
			}
			if (ret == 0) {
				/* fall back to be 1080p */
				ret = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector,1920, 1080);
				DP_INFO("edid is null and read downstream: count=%d", ret);
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

	dev_dbg(dp->dev, "mode count = %d bpc=%d\n", ret, info->bpc);
	return ret;
}

static struct drm_encoder *
trilin_dp_connector_best_encoder(struct drm_connector *connector)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	DP_DEBUG("enter\n");

	return &dp->encoder.base;
}

enum drm_mode_status trilin_dp_connector_mode_valid(struct drm_connector *connector,
					  struct drm_display_mode *mode)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	struct trilin_connector *conn = connector_to_trilin(connector);
	u8 max_lanes = dp->link_config.max_lanes;
	u8 minbpp = 6 * 3; //Fixme: add format;
	int max_rate = dp->link_config.max_rate;
	int rate;
	int clock = mode->clock;
	u8 pixel_per_cycle = 1;
	//DP_DEBUG("enter\n");

	if (dp->force_pixel_per_cycle != 0)
		pixel_per_cycle = dp->force_pixel_per_cycle;

	if ((clock > TRILIN_MAX_FREQ) && (clock < TRILIN_MAX_FREQ * 2))
		pixel_per_cycle = 2;

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
	} else if (dp->mst.mst_active) {
		rate = rate * 63 / 64; /* one slot for MTPH */
		if (clock > rate) {
			DP_INFO("filtered mode (%s@%d) for high bandwidth in MST mode\n",
				mode->name, clock);
			drm_mode_debug_printmodeline(mode);
			return MODE_CLOCK_HIGH;
		}
	}

	//DP_DEBUG("mode name %s, mode->clock %d, rate %d conn->config.bpp=%d\n",
	//		mode->name, mode->clock, rate, conn->config.bpp);
	return MODE_OK;
}

void trilin_dp_connector_reset(struct drm_connector *connector)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	DP_DEBUG("enter\n");
	drm_atomic_helper_connector_reset(connector);
}

int trilin_dp_fill_modes(struct drm_connector *connector, uint32_t maxX, uint32_t maxY)
{
	struct trilin_dp *dp = connector_to_dp(connector);
	DP_DEBUG("enter: %d\n", connector->base.id);
	return drm_helper_probe_single_connector_modes(connector, maxX, maxY);
}

static int mst_info_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct trilin_connector *conn = connector_to_trilin(connector);

	if (connector->status != connector_status_connected){
		seq_printf(m, "not connected\n");
		return -ENODEV;
	}

	//trilin_dp_dump_regs(conn->dp);

	if (!conn || conn->type != TRILIN_OUTPUT_DP_MST) {
		seq_printf(m, "not TRILIN_OUTPUT_DP_MST\n");
		return -ENODEV;
	}

	seq_printf(m, "MST Source Port [conn->port:%d]\n",
			   conn->port->port_num);

	if (conn->dp->mst_mgr)
		drm_dp_mst_dump_topology(m,  conn->dp->mst_mgr);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(mst_info);

static int register_info_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct trilin_connector *conn = connector_to_trilin(connector);

	if (connector->status != connector_status_connected){
		seq_printf(m, "not connected\n");
		return -ENODEV;
	}

	trilin_dp_dump_regs(m, conn->dp);
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(register_info);

void trilin_dp_connector_debugfs_init(struct drm_connector *connector, struct dentry *root)
{
	debugfs_create_file("mst_topology", 0444, root, connector, &mst_info_fops);
	debugfs_create_file("register_info", 0444, root, connector, &register_info_fops);
}

static const struct drm_connector_funcs trilin_dp_connector_funcs = {
	.detect			= trilin_dp_connector_detect,
	.fill_modes		= trilin_dp_fill_modes,
	.destroy		= drm_connector_cleanup,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
	.reset			= trilin_dp_connector_reset,
	.debugfs_init   = trilin_dp_connector_debugfs_init,
};

static const struct drm_connector_helper_funcs
trilin_dp_connector_helper_funcs = {
	.get_modes	= trilin_dp_connector_get_modes,
	.best_encoder	= trilin_dp_connector_best_encoder,
	.mode_valid	= trilin_dp_connector_mode_valid,
	.atomic_check = trilin_dp_connector_atomic_check,
};

/* -----------------------------------------------------------------------------
 * DRM Encoder
 */
static void trilin_dp_encoder_enable(struct drm_encoder *encoder)
{
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_dp_panel *dp_panel = &dp->dp_panel;
	int rc = 0;
	DP_INFO("enter\n");

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
}

static void trilin_dp_encoder_disable(struct drm_encoder *encoder)
{
	int rc;
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct trilin_dp_panel *dp_panel = &dp->dp_panel;
	DP_INFO("enter\n");

	if (!(dp->state & DP_STATE_INITIALIZED)) {
		DP_DEBUG("[not init]");
		return;
	}

	rc = trilin_dp_pre_disable(dp, dp_panel);
	if (rc) {
		DP_ERR("DP display pre disable failed, rc=%d\n", rc);
	}

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

	dev_dbg(dp->dev, "%s end\n", __func__);
}

u8 trilin_dp_cal_bpc(struct drm_connector_state *connector_state)
{
	u8 bpc = 8;
	struct drm_display_info *info = &connector_state->connector->display_info;

	if ((connector_state->colorspace >= DRM_MODE_COLORIMETRY_BT2020_CYCC) &&
	     (connector_state->colorspace <= DRM_MODE_COLORIMETRY_BT2020_YCC)) {
		pr_info("colorspace is bt2020. force set bpc (%d) to 10", info->bpc);
		bpc = 10;
	}

	if (info->bpc && bpc > info->bpc) {
		pr_info("downgrading requested %ubpc to display limit %ubpc\n", bpc, info->bpc);
		bpc = info->bpc;
	}

	if (connector_state->max_requested_bpc && bpc > connector_state->max_requested_bpc) {
		pr_info("downgrading requested %ubpc to property limit %ubpc\n"
			 	, bpc, connector_state->max_requested_bpc);
		bpc = connector_state->max_requested_bpc;
	}
	return bpc;
}

u8 trilin_dp_cal_bpp(struct drm_connector_state *connector_state, u8 bpc, enum trilin_dpsub_format format)
{
	u8 bpp = bpc * 3;

	switch (format) {
	case TRILIN_DPSUB_FORMAT_RGB:
		bpp = bpc * 3;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR444:
		bpp = bpc * 3;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR422:
		bpp = bpc * 2;
		break;
	case TRILIN_DPSUB_FORMAT_YONLY:
		bpp = bpc * 1;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR420:
		bpp = bpc * 3 / 2;
		break;
	default:
		pr_warn("Invalid format in DT.\n");
	}
	return bpp;
}

#define TRILIN_DPTX_MIN_H_BACKPORCH	20

int trilin_dp_encoder_atomic_adjust_mode(struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode)
{
	int diff = mode->htotal - mode->hsync_end;
	/*
	 * Trilinear DP requires horizontal backporch to be greater than 12.
	 * This limitation may not be compatible with the sink device.
	 */
	if (diff < TRILIN_DPTX_MIN_H_BACKPORCH) {
		int vrefresh = (adjusted_mode->clock * 1000) /
			       (adjusted_mode->vtotal * adjusted_mode->htotal);

		pr_info("hbackporch adjusted: %d to %d\n", diff, TRILIN_DPTX_MIN_H_BACKPORCH - diff);
		diff = TRILIN_DPTX_MIN_H_BACKPORCH - diff;
		adjusted_mode->htotal += diff;
		adjusted_mode->clock = adjusted_mode->vtotal *
				       adjusted_mode->htotal * vrefresh / 1000;
	}
#if INVERSE_VSYNC
	if (adjusted_mode->flags & DRM_MODE_FLAG_PVSYNC)
  		adjusted_mode->flags &= ~DRM_MODE_FLAG_PVSYNC;
  	else
  		adjusted_mode->flags |= DRM_MODE_FLAG_PVSYNC;

  	if (adjusted_mode->flags & DRM_MODE_FLAG_PHSYNC)
  		adjusted_mode->flags &= ~DRM_MODE_FLAG_PHSYNC;
  	else
  		adjusted_mode->flags |= DRM_MODE_FLAG_PHSYNC;

	pr_info("adjust_mode flags: 0x%0x", adjusted_mode->flags);
#endif
	return 0;
}

int trilin_dp_encoder_atomic_check(struct drm_encoder *encoder,
				  struct drm_crtc_state *crtc_state,
				  struct drm_connector_state *connector_state)
{
	struct drm_display_mode *mode = &crtc_state->mode;
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct trilin_connector *conn = connector_to_trilin(connector_state->connector);
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct drm_connector *connector = &dp->connector.base;
	u8 max_lanes = dp->link_config.max_lanes;
	int rate, max_rate = dp->link_config.max_rate;
	struct drm_display_info *info = &connector_state->connector->display_info;
	int ret;
	enum trilin_dpsub_format format = TRILIN_DPSUB_FORMAT_RGB, tmp_format;
	u8 bpc = 8, bpp = 24;
	int colorspace = connector_state->colorspace;
	DP_DEBUG("enter\n");

	if (connector->connector_type == DRM_MODE_CONNECTOR_eDP) {
		struct drm_display_mode *preferred_mode;

		preferred_mode = list_first_entry(&connector->modes,
						struct drm_display_mode, head);

		drm_mode_copy(adjusted_mode, preferred_mode);
	}

	trilin_dp_encoder_atomic_adjust_mode(mode, adjusted_mode);

	if (info->num_bus_formats) {
		switch(info->bus_formats[0]) {
		case DRM_COLOR_FORMAT_YCBCR444:
			format = TRILIN_DPSUB_FORMAT_YCBCR444;
			break;
		case DRM_COLOR_FORMAT_YCBCR422:
			format = TRILIN_DPSUB_FORMAT_YCBCR422;
			break;
		case DRM_COLOR_FORMAT_YCBCR420:
			format = TRILIN_DPSUB_FORMAT_YCBCR420;
			break;
		case DRM_COLOR_FORMAT_RGB444:
		default:
			format = TRILIN_DPSUB_FORMAT_RGB;
			break;
		}
	} else {
		if (info->color_formats & DRM_COLOR_FORMAT_YCBCR444)
			format = TRILIN_DPSUB_FORMAT_YCBCR444;
		else if (info->color_formats & DRM_COLOR_FORMAT_YCBCR422)
			format = TRILIN_DPSUB_FORMAT_YCBCR422;
		else if (connector_state->connector->ycbcr_420_allowed &&
			drm_mode_is_420(info, adjusted_mode))
			format = TRILIN_DPSUB_FORMAT_YCBCR420;
		else
			format = TRILIN_DPSUB_FORMAT_RGB;
	}

	bpc = trilin_dp_cal_bpc(connector_state);

	/*Fixme: Force to rgb...*/
	tmp_format = format;
	format = TRILIN_DPSUB_FORMAT_RGB;
	bpp = trilin_dp_cal_bpp(connector_state, bpc, format);

	DP_DEBUG("Note: force bus format %d to RGB 0. disp->bpc(%d) bpc(%d) bpp(%d) colorspace=%d\n",
				tmp_format, info->bpc, bpc, bpp, colorspace);

	/* Check again as bpp or format might have been chagned */
	rate = trilin_dp_max_rate(max_rate, max_lanes, bpp);
	if (mode->clock > rate) {
		dev_err(dp->dev, "mode %s pixel rate %d is higher than max rate %d\n",
			mode->name, mode->clock, rate);
		return -EINVAL;
	}

	ret = trilin_dp_mode_configure(dp, adjusted_mode->clock, 0, bpp);
	if (ret < 0)
		return -EINVAL;

	conn->config.format = format;
	conn->config.colorspace = colorspace;
	conn->config.bpc = bpc;
	conn->config.bpp = bpp;

	return 0;
}


void trilin_dp_encoder_atomic_mode_set(struct drm_encoder *encoder,
					struct drm_crtc_state *crtc_state,
					struct drm_connector_state *connector_state)
{
	struct trilin_dp *dp = encoder_to_dp(encoder);
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	DP_INFO("set mode: %s", adjusted_mode->name);

	if (dp->force_pixel_per_cycle != 0) {
		dp->pixel_per_cycle = dp->force_pixel_per_cycle;
	} else {
		if ((adjusted_mode->clock > TRILIN_MAX_FREQ) &&
			(adjusted_mode->clock < TRILIN_MAX_FREQ * 2))
			dp->pixel_per_cycle = 2;
		else
			dp->pixel_per_cycle = 1;
	}
}

static const struct drm_encoder_helper_funcs trilin_dp_encoder_helper_funcs = {
	.enable			= trilin_dp_encoder_enable,
	.disable		= trilin_dp_encoder_disable,
	.atomic_mode_set	= trilin_dp_encoder_atomic_mode_set,
	.atomic_check		= trilin_dp_encoder_atomic_check,
};

/*add HDR metata property*/
static void trilin_dp_add_properties(struct trilin_dp *dp, struct drm_connector* connector)
{
	if (!drm_mode_create_dp_colorspace_property(connector))
		drm_connector_attach_colorspace_property(connector);
	drm_connector_attach_hdr_output_metadata_property(connector);

	connector->max_bpc_property = NULL;
	drm_connector_attach_max_bpc_property(connector, 8, 12); //8,12
	drm_connector_attach_dp_subconnector_property(connector);
	drm_connector_attach_content_type_property(connector);
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

	DP_INFO("%s begin\n", __func__);

	enc->dp = dp;
	conn->dp = dp;
	conn->type = TRILIN_OUTPUT_DP;
	/* Create the DRM encoder and connector. */
	encoder->possible_crtcs = TRILIN_DPTX_POSSIBLE_CRTCS_SST;
	drm_encoder_init(dp->drm,
		encoder,
		&trilin_dp_enc_funcs,
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
	dp->dp_panel.connector = &dp->connector;
	dp->dp_panel.stream_id = 0;
	dp->connector.dp_panel = &dp->dp_panel;
	//Fixme: just for sst
	if (connector->funcs->reset)
  		connector->funcs->reset(connector);
	trilin_dp_add_properties(dp, connector);
	//dp hardware init now
	trilin_dp_init_config(dp);
	return 0;
}

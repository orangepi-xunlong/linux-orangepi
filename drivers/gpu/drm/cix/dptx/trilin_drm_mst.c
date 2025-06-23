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
#include <drm/drm_debugfs.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_file.h>
#include <drm/drm_print.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <asm/types.h>
#include <linux/component.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_phy.h"
#include "trilin_dptx.h"
#include "trilin_drm_mst.h"
#include "../linlon-dp/linlondp_pipeline.h"
#include "../linlon-dp/linlondp_kms.h"

#define pipe_name(p) ((p) + 'A')

enum mst_msg_ready_type {
	NONE_MSG_RDY_EVENT = 0,
	DOWN_REP_MSG_RDY_EVENT = 1,
	UP_REQ_MSG_RDY_EVENT = 2,
	DOWN_OR_UP_MSG_RDY_EVENT = 3
};

static const struct drm_connector_funcs trilin_dp_mst_connector_funcs;
static const struct drm_connector_helper_funcs trilin_dp_mst_connector_helper_funcs;

static struct trilin_dp* topology_mgr_to_trilin(struct drm_dp_mst_topology_mgr *mgr)
{
	struct trilin_dp_mst_private* mst_private =
			container_of(mgr, struct trilin_dp_mst_private, mst_mgr);
	return mst_private->dp;
}

static void trilin_handle_mst_sideband_msg(
				struct drm_dp_mst_topology_mgr *mgr,
				enum mst_msg_ready_type msg_rdy_type)
{
	uint8_t esi[DP_PSR_ERROR_STATUS - DP_SINK_COUNT_ESI] = { 0 };
	ssize_t ret;
	bool new_irq_handled = false;
	int dpcd_addr;
	uint8_t dpcd_bytes_to_read;
	const uint8_t max_process_count = 30;
	uint8_t process_count = 0;
	u8 retry;
	struct trilin_dp *dp = topology_mgr_to_trilin(mgr);
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	//DP_MST_DEBUG("enter\n");
	mutex_lock(&mst->poll_irq_lock);

	if (dp->dpcd[DP_DPCD_REV] < 0x12) {
		dpcd_bytes_to_read = DP_LANE0_1_STATUS - DP_SINK_COUNT;
		/* DPCD 0x200 - 0x201 for downstream IRQ */
		dpcd_addr = DP_SINK_COUNT;
	} else {
		dpcd_bytes_to_read = DP_PSR_ERROR_STATUS - DP_SINK_COUNT_ESI;
		// /* DPCD 0x2002 - 0x2005 for downstream IRQ */
		dpcd_addr = DP_SINK_COUNT_ESI;
	}

	while (process_count < max_process_count) {
		u8 ack[DP_PSR_ERROR_STATUS - DP_SINK_COUNT_ESI] = {};

		process_count++;
		ret = drm_dp_dpcd_read(
			&dp->aux,
			dpcd_addr,
			esi,
			dpcd_bytes_to_read);

		if (ret != dpcd_bytes_to_read) {
			DP_ERR("DPCD read bytes is not expected!");
			break;
		}

		//DP_MST_DEBUG( "ESI %02x %02x %02x mst=%d\n"
		//	, esi[0], esi[1], esi[2], dp->mst_mgr->mst_state);

		switch (msg_rdy_type) {
		case DOWN_REP_MSG_RDY_EVENT:
			/* Only handle DOWN_REP_MSG_RDY case*/
			esi[1] &= DP_DOWN_REP_MSG_RDY;
			break;
		case UP_REQ_MSG_RDY_EVENT:
			/* Only handle UP_REQ_MSG_RDY case*/
			esi[1] &= DP_UP_REQ_MSG_RDY;
			break;
		default:
			/* Handle both cases*/
			esi[1] &= (DP_DOWN_REP_MSG_RDY | DP_UP_REQ_MSG_RDY);
			break;
		}

		if (!esi[1])
			break;

		/* handle MST irq */
		if (dp->mst_mgr->mst_state) {
			drm_dp_mst_hpd_irq_handle_event(
						dp->mst_mgr,
						esi,
						ack,
						&new_irq_handled);
		}

		if (new_irq_handled) {
			/* ACK at DPCD to notify down stream */
			for (retry = 0; retry < 3; retry++) {
				ssize_t wret;

				wret = drm_dp_dpcd_writeb(&dp->aux,
							  dpcd_addr + 1,
							  ack[1]);
				if (wret == 1)
					break;
			}

			if (retry == 3) {
				DP_ERR("Failed to ack MST event.\n");
				break;
			}

			drm_dp_mst_hpd_irq_send_new_request(dp->mst_mgr);

			new_irq_handled = false;
		} else {
			break;
		}
	}

	if (process_count == max_process_count)
		DP_ERR("Loop exceeded max iterations");
	//else
		//DP_MST_DEBUG("handle irq success: loop_count=%d", process_count);
	mutex_unlock(&mst->poll_irq_lock);
}

/*Active Call from trilin_dptx hpq_irq*/
void trilin_dp_mst_display_hpd_irq(struct trilin_dp *dp)
{
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	DP_MST_DEBUG("enter\n");

	if (!mst->mst_session_state) {
		DP_DEBUG("mst_hpd_irq received before mst session start\n");
		return;
	}
	DP_MST_DEBUG("enter\n");
	trilin_handle_mst_sideband_msg(dp->mst_mgr, DOWN_OR_UP_MSG_RDY_EVENT);
}

/*Auto Call from mst frameworks*/
static void dp_mst_cbs_poll_hdp_irq(struct drm_dp_mst_topology_mgr *mgr)
{
	struct trilin_dp *dp = topology_mgr_to_trilin(mgr);
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;

	if (!mst->mst_session_state) { //(dp->state & DP_STATE_SUSPENDED)
		DP_DEBUG("mst_hpd_irq received before mst session start\n");
		return;
	}
	DP_MST_DEBUG("enter\n");
	trilin_handle_mst_sideband_msg(mgr, DOWN_REP_MSG_RDY_EVENT);
}

int trilin_dp_set_mst_mgr_state(struct trilin_dp *dp, bool state)
{
	int rc;
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;

	mst->mst_session_state = state;

	rc = drm_dp_mst_topology_mgr_set_mst(dp->mst_mgr, state);
	if (rc < 0) {
		DP_ERR("failed to set topology mgr state to %d. rc %d\n",
				state, rc);
	}
	DP_MST_DEBUG("mst mgr state:%d\n", state);
	return rc;
}

int trilin_dp_mst_suspend(struct trilin_dp *dp)
{
	if (dp->mst.mst_active) {
		drm_dp_mst_topology_mgr_suspend(dp->mst_mgr);
	}
	return 0;
}

int trilin_dp_mst_resume(struct trilin_dp *dp)
{
	int ret = 0;
	if (dp->mst.mst_active && (dp->state & DP_STATE_SUSPENDED)) {
		ret = drm_dp_mst_topology_mgr_resume(dp->mst_mgr, true);
		if (ret) {
			DP_WARN("mst resume failed.");
			trilin_dp_set_mst_mgr_state(dp, false);
			dp->active_stream_cnt = 0;
			dp->mst.mst_active = false;
		} else {
			DP_DEBUG("mst resume success. wait 1-1.5ms");
			usleep_range(1000, 1500);
		}
	}
	return ret;
}

static struct drm_connector *trilin_dp_add_mst_connector(
				struct drm_dp_mst_topology_mgr *mgr,
				struct drm_dp_mst_port *port,
				const char *pathprop)
{
	struct trilin_connector *conn;
	struct drm_connector *connector;
	struct trilin_dp *dp = topology_mgr_to_trilin(mgr);
	struct drm_encoder *encorder;
	int i, ret;

	DP_MST_DEBUG("enter\n");

	conn = kzalloc(sizeof(*conn), GFP_KERNEL);
	if (!conn)
		return NULL;

	for (i = 0; i < MAX_DP_MST_DRM_ENCODERS; i++) {
		if (!dp->mst.mst_panels[i].in_use) {
			dp->mst.mst_panels[i].in_use = true;
			dp->mst.mst_panels[i].panel.stream_id = i;
			dp->mst.mst_panels[i].panel.connector = conn;
			conn->dp_panel = &dp->mst.mst_panels[i].panel;
			break;
		}
	}

	if (i == MAX_DP_MST_DRM_ENCODERS) {
		kfree(conn);
		DP_INFO("Max support two streams, not support pathprop: %s\n", pathprop);
		return NULL;
	}

	conn->port = port;
	conn->dp = dp;
	conn->type = TRILIN_OUTPUT_DP_MST;

	connector = &conn->base;

	drm_dp_mst_get_port_malloc(port);

	ret = drm_connector_init(dp->drm, connector, &trilin_dp_mst_connector_funcs,
				 DRM_MODE_CONNECTOR_DisplayPort);
	if (ret) {
		drm_dp_mst_put_port_malloc(port);
		kfree(conn);
		DP_ERR("drm_connector_init failed..\n");
		return NULL;
	}
	drm_connector_helper_add(connector, &trilin_dp_mst_connector_helper_funcs);

	for (i = 0; i < MAX_DP_MST_DRM_ENCODERS; i++) {
		encorder = &dp->mst.private_info.mst_encoders[i].base;
		ret = drm_connector_attach_encoder(connector, encorder);
  		if (ret) {
			DP_ERR("drm_connector_attach_encoder failed..\n");
  			goto err;
		}
	}

	if (connector->funcs->reset)
  		connector->funcs->reset(connector);

	drm_object_attach_property(&connector->base, dp->drm->mode_config.path_property, 0);
	drm_object_attach_property(&connector->base, dp->drm->mode_config.tile_property, 0);
	drm_connector_set_path_property(connector, pathprop);
	/*
	 * Reuse the prop from the SST connector because we're
	 * not allowed to create new props after device registration.
	 */
	connector->max_bpc_property =
		dp->connector.base.max_bpc_property;
	if (connector->max_bpc_property)
		drm_connector_attach_max_bpc_property(connector, 8, 12);

	DP_MST_INFO( "Successfully create mst connector=%d pathprop=%s port=%d stream_id=%d\n"
		, connector->base.id, pathprop, port->port_num, conn->dp_panel->stream_id);
	return connector;

err:
	drm_connector_cleanup(connector);
	return NULL;
}

static const struct drm_dp_mst_topology_cbs mst_cbs = {
	.add_connector = trilin_dp_add_mst_connector,
	.poll_hpd_irq = dp_mst_cbs_poll_hdp_irq,
};

static int
trilin_dp_mst_connector_late_register(struct drm_connector *connector)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	int rc = 0;
	DP_MST_DEBUG("enter %d\n", conn->port->port_num);
	rc = drm_dp_mst_connector_late_register(connector, conn->port);
	if (rc < 0)
		return rc;
	return rc;
}

static void
trilin_dp_mst_connector_early_unregister(struct drm_connector *connector)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	DP_MST_DEBUG("enter: port%d\n", conn->port->port_num);
	drm_dp_mst_connector_early_unregister(connector, conn->port);
}

static void trilin_dp_mst_connector_destroy(struct drm_connector *connector)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	uint32_t id = conn->dp_panel->stream_id;
	int i;
	DP_MST_DEBUG("enter: connector=%d stream%d port%d\n", connector->base.id, id, conn->port->port_num);

	for (i = 0; i < MAX_DP_MST_DRM_ENCODERS; i++) {
		if (dp->mst.mst_panels[i].panel.stream_id == id) {
			dp->mst.mst_panels[i].in_use = false;
			break;
		}
	}

	drm_connector_cleanup(connector);
	drm_dp_mst_put_port_malloc(conn->port);
	kfree(conn);
}

static const struct drm_connector_funcs trilin_dp_mst_connector_funcs = {
	.fill_modes = trilin_dp_fill_modes,
	//.atomic_get_property = trilin_connector_atomic_get_property,
	// .atomic_set_property = trilin_connector_atomic_set_property,
	.late_register = trilin_dp_mst_connector_late_register,
	.early_unregister = trilin_dp_mst_connector_early_unregister,
	.destroy = trilin_dp_mst_connector_destroy,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.reset			= trilin_dp_connector_reset,
	.debugfs_init   = trilin_dp_connector_debugfs_init,
};

static int trilin_dp_mst_get_modes(struct drm_connector *connector)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	int ret = 0;
	struct edid *edid;
	DP_MST_DEBUG("enter\n");

	if (drm_connector_is_unregistered(connector))
		return trilin_connector_update_modes(connector, NULL);

	edid = drm_dp_mst_get_edid(connector, dp->mst_mgr, conn->port);
	ret = trilin_connector_update_modes(connector, edid);
	kfree(edid);

	return ret;
}

enum drm_mode_status trilin_dp_mst_mode_valid(struct drm_connector *connector,
					  struct drm_display_mode *mode)
{
	const int min_bpp = 8 * 3;
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	const int PBN = drm_dp_calc_pbn_mode(mode->clock, min_bpp, false);
	enum drm_mode_status ret = trilin_dp_connector_mode_valid(connector, mode);
	//DP_MST_DEBUG("enter\n");
	if (ret != MODE_OK)
		return ret;

	if (PBN > conn->port->full_pbn) {
		DP_MST_DEBUG("filter mode(%s) for port%d that pbn=%d > full_pbn=%d.\n"
			, mode->name, conn->port->port_num, PBN, conn->port->full_pbn);
		drm_mode_debug_printmodeline(mode);
		return MODE_CLOCK_HIGH;
	}
	return MODE_OK;
}

static struct drm_encoder *trilin_mst_atomic_best_encoder(struct drm_connector *connector,
							 struct drm_atomic_state *state)
{
	struct drm_connector_state *connector_state = drm_atomic_get_new_connector_state(state,
											 connector);
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	struct trilin_encoder *mst_encoder;
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	struct linlondp_crtc *crtc = to_kcrtc(connector_state->crtc);
	int pipe_id = 0;

	DP_MST_DEBUG("enter\n");

	if (connector_state->best_encoder)
		return connector_state->best_encoder;

	if (crtc && crtc->master) {
		pipe_id = crtc->master->id;
	}
	DP_MST_DEBUG( "name=%s crtc->master: %s pipe=%d\n", connector_state->crtc->name,
			crtc->master != NULL ? "true" : "false", pipe_id);

	if (pipe_id >= MAX_DP_MST_DRM_ENCODERS) {
		DP_ERR("pipe_id %d too large...\n", pipe_id);
		pipe_id = 0;
	}

	mst_encoder = &mst->mst_encoders[pipe_id];
	if (mst_encoder->id != pipe_id) {
		DP_ERR("Should not happen. encoder id %d != pipe_id %d , not config, abort", mst_encoder->id, pipe_id);
		return NULL;
	}

	/* encoder can be choose from different connector*/
	if (conn->dp_panel->stream_id != pipe_id) {
		DP_WARN("conn->dp_panel->stream_id=%d but pipe=%d. align it to pipe_id", conn->dp_panel->stream_id, pipe_id);
		conn->dp_panel->stream_id = pipe_id;
	}

	mst_encoder->connector = conn;
	mst_encoder->dp_panel = conn->dp_panel;
	DP_MST_DEBUG("pipe=%d mst_encoder->id=%d stream=%d enable=%d connect_id=%d"
			, pipe_id, mst_encoder->id, conn->dp_panel->stream_id, mst_encoder->enable, connector->base.id);
	return &mst_encoder->base;
}

static int trilin_dp_mst_conn_atomic_check(struct drm_connector *connector,
					struct drm_atomic_state *state)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;

	DP_MST_DEBUG("enter\n");
	conn->state = state;
	return drm_dp_atomic_release_time_slots(state, dp->mst_mgr, conn->port);
}

static int
trilin_dp_mst_detect(struct drm_connector *connector,
		     struct drm_modeset_acquire_ctx *ctx, bool force)
{
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct trilin_dp *dp = conn->dp;
	struct drm_dp_mst_topology_mgr* mst_mgr = dp->mst_mgr;
	int status;
	DP_MST_DEBUG("enter: \n");

	if (drm_connector_is_unregistered(connector))
		return connector_status_disconnected;

	status = drm_dp_mst_detect_port(connector, ctx, mst_mgr, conn->port);
	DP_MST_DEBUG("port %d status: %d\n",  conn->port->port_num, status);
	return status;
}

static const struct drm_connector_helper_funcs trilin_dp_mst_connector_helper_funcs = {
	.get_modes = trilin_dp_mst_get_modes,
	.mode_valid	= trilin_dp_mst_mode_valid,
	.atomic_best_encoder = trilin_mst_atomic_best_encoder,
	.atomic_check = trilin_dp_mst_conn_atomic_check,
	.detect_ctx = trilin_dp_mst_detect,
};

/* -----------------------------------------------------------------------------
 * DRM Encoder
 */
static void trilin_dp_mst_encoder_destroy(struct drm_encoder *encoder)
{
	struct trilin_encoder *enc = encoder_to_trilin(encoder);
	struct trilin_dp *dp = enc->dp;
	DP_MST_DEBUG("enter\n");
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs trilin_dp_mst_enc_funcs = {
	.destroy = trilin_dp_mst_encoder_destroy,
};

static void _dp_mst_get_vcpi_info(
		struct drm_dp_mst_topology_state *state,
		int vcpi, int *start_slot, int *num_slots, int* pbn)
{
	struct drm_dp_mst_atomic_payload *payload;
	*start_slot = 0;
	*num_slots = 0;

	list_for_each_entry(payload, &state->payloads, next) {
		if (payload->vcpi == vcpi && !payload->delete) {
			*start_slot = payload->vc_start_slot;
			*num_slots = payload->time_slots;
			*pbn = payload->pbn;
			break;
		}
	}
}

static void trilin_dp_mst_update_timeslots(struct trilin_dp_mst_private *mst,
		struct drm_dp_mst_topology_state *mst_state,
		struct trilin_encoder* ext_mst_encoder)
{
	int i;
	struct trilin_encoder *mst_encoder = ext_mst_encoder;
	int pbn=0, start_slot=0, num_slots=0;
	struct trilin_dp *dp = ext_mst_encoder->dp;

	for (i = 0; i < MAX_DP_MST_DRM_ENCODERS; i++) {
		mst_encoder = &mst->mst_encoders[i];

		pbn = 0;
		start_slot = 0;
		num_slots = 0;

		if (mst_encoder->vcpi) {
			_dp_mst_get_vcpi_info(mst_state,
					mst_encoder->vcpi,
					&start_slot, &num_slots, &pbn);
			//DP_MST_DEBUG("encoder:%d vcpi:%d start_slot:%d, num_slots:%d payload pbn:%d mst_pbn: %d \n",
			//	mst_encoder->id, mst_encoder->vcpi, start_slot, num_slots, pbn, mst_encoder->pbn);
			//pbn = mst_encoder->pbn; ///Fixme..why...
		}

		if (mst_encoder == ext_mst_encoder) {
			mst_encoder->num_slots = num_slots;
			mst_encoder->start_slot = start_slot;
			mst_encoder->pbn = pbn;
		}

		trilin_dp_set_stream_info(mst->dp,
				mst_encoder->dp_panel,
				mst_encoder->id,
				start_slot, num_slots);

		DP_MST_DEBUG("encoder:%d vcpi:%d start_slot:%d num_slots:%d, pbn:%d\n",
			mst_encoder->id, mst_encoder->vcpi,
			start_slot, num_slots, pbn);
	}
}

static void _dp_mst_encoders_pre_enable_part1(struct trilin_encoder *mst_encoder)
{
	struct trilin_dp *dp = mst_encoder->dp;
	struct trilin_connector *c_conn = mst_encoder->connector;
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	struct drm_dp_mst_port *port = c_conn->port;
	struct drm_dp_mst_topology_mgr *mgr = dp->mst_mgr;
	struct drm_dp_mst_topology_state *mst_state;
	struct drm_dp_mst_atomic_payload *payload;
	int ret;

	DP_MST_DEBUG("enter: port number=%d\n", port->port_num);

	mst_state = to_drm_dp_mst_topology_state(mgr->base.state);
	payload = drm_atomic_get_mst_payload_state(mst_state, port);
	ret = drm_dp_add_payload_part1(mgr, mst_state, payload);

	if (ret < 0) {
		DP_ERR("WHAT? mst:drm_dp_add_payload_part1 failed. encoder:%d payload(%d %d %d)\n",
				mst_encoder->id, payload->vcpi, payload->pbn, payload->time_slots);
		return;
	}

	mst_encoder->vcpi = payload->vcpi;
	mst_encoder->enable = true;
	DP_MST_DEBUG("Enable mst_encoder: %d payload vcpi=%d port=%d", mst_encoder->id, payload->vcpi, port->port_num);
	trilin_dp_mst_update_timeslots(mst, mst_state, mst_encoder);
}

static void _dp_mst_encoders_pre_enable_part2(struct trilin_encoder *mst_encoder)
{
	struct trilin_dp *dp = mst_encoder->dp;
	struct drm_dp_mst_topology_state *mst_state;
	struct drm_dp_mst_atomic_payload *payload;
	struct drm_dp_mst_topology_mgr *mgr = dp->mst_mgr;
	struct trilin_connector *conn = mst_encoder->connector;
	struct drm_dp_mst_port *port = conn->port;
	struct drm_atomic_state *state = conn->state;

	DP_MST_DEBUG("enter %p\n", state);

	drm_dp_check_act_status(mgr);

	mst_state = to_drm_dp_mst_topology_state(mgr->base.state);
	payload = drm_atomic_get_mst_payload_state(mst_state, port);
	drm_dp_add_payload_part2(mgr, state, payload);
	DP_MST_DEBUG("mst encoder [%d] _pre enable part-2 complete [state=%p]\n", mst_encoder->id, state);
}

static void _dp_mst_encoders_pre_disable(struct trilin_encoder *mst_encoder)
{
	struct trilin_dp *dp = mst_encoder->dp;
	struct trilin_connector *conn = mst_encoder->connector;
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	struct drm_dp_mst_topology_mgr *mgr = dp->mst_mgr;
	struct drm_atomic_state *state = conn->state;
	struct drm_dp_mst_topology_state *old_mst_state, *new_mst_state, *mst_state;
	struct drm_dp_mst_atomic_payload *old_payload, *new_payload;

	DP_MST_DEBUG("enter\n");
	if (state == NULL) {
		DP_MST_INFO("drm_atomic_state is null???? (%p)\n", state);
		return;
	}

	mst_state = drm_atomic_get_new_mst_topology_state(state, mgr);
	old_mst_state = drm_atomic_get_old_mst_topology_state(state, mgr);
	new_mst_state = drm_atomic_get_new_mst_topology_state(state, mgr);

	DP_MST_DEBUG("state=%p old_mst_state=%p new_mst_state=%p mst_state=%p\n"
		, state, old_mst_state, new_mst_state, mst_state);

	if (old_mst_state == NULL || new_mst_state == NULL) {
		return;
	}

	old_payload = drm_atomic_get_mst_payload_state(old_mst_state, conn->port);
	new_payload = drm_atomic_get_mst_payload_state(new_mst_state, conn->port);

	drm_dp_remove_payload(mgr, new_mst_state, old_payload, new_payload);
	trilin_dp_mst_update_timeslots(mst, new_mst_state, mst_encoder);

	mst_encoder->vcpi = 0;
	mst_encoder->pbn = 0;
	mst_encoder->enable = false;
	DP_MST_DEBUG("mst encoder [%u] _pre disable complete\n", mst_encoder->id);
}

static void trilin_mst_encoder_enable(struct drm_encoder *encoder)
{
	struct trilin_encoder *mst_encoder = encoder_to_trilin(encoder);
	struct trilin_dp *dp = mst_encoder->dp;
	struct trilin_dp_panel *dp_panel = mst_encoder->dp_panel;
	struct trilin_connector *conn = mst_encoder->connector;
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	int rc = 0;
	DP_MST_DEBUG( "enter panel=%p\n",dp_panel);

	mutex_lock(&mst->mst_lock);

	/*link trainning*/
	rc = trilin_dp_prepare(dp);
	if (rc) {
		DP_ERR("[%d] DP display prepare failed, rc=%d\n",
		       mst_encoder->id, rc);
		return;
	}

	drm_dp_send_power_updown_phy(dp->mst_mgr, conn->port, true);
	_dp_mst_encoders_pre_enable_part1(mst_encoder);

	/*stream on*/
	rc = trilin_dp_enable(dp, dp_panel);
	if (rc) {
		DP_ERR("[%d] DP display enable failed, rc=%d\n",
		       mst_encoder->id, rc);
		trilin_dp_unprepare(dp);
		mutex_unlock(&mst->mst_lock);
		return;
	} else {
		_dp_mst_encoders_pre_enable_part2(mst_encoder);
	}
	mutex_unlock(&mst->mst_lock);

	/*update hdr and hdcp ? */
	trilin_dp_post_enable(dp, dp_panel);
	DP_MST_DEBUG("end\n");
}

static void trilin_mst_encoder_disable(struct drm_encoder *encoder)
{
	struct trilin_encoder *mst_encoder = encoder_to_trilin(encoder);
	struct trilin_dp_panel *dp_panel = mst_encoder->dp_panel;
	struct trilin_dp *dp = mst_encoder->dp;
	struct trilin_connector *conn = mst_encoder->connector;
	struct trilin_dp_mst_private *mst = &dp->mst.private_info;
	int rc = 0;
	DP_MST_DEBUG("enter\n");

	mutex_lock(&mst->mst_lock);

	if (!(dp->state & DP_STATE_INITIALIZED)) {
		DP_DEBUG("[not init]");
		return;
	}

	_dp_mst_encoders_pre_disable(mst_encoder);

	rc = trilin_dp_pre_disable(dp, dp_panel);
	if (rc) {
		DP_ERR("[%d] DP display pre disable failed, rc=%d\n",
		       mst_encoder->id, rc);
	}

	drm_dp_check_act_status(dp->mst_mgr);

	drm_dp_send_power_updown_phy(dp->mst_mgr, conn->port, false);
	mutex_unlock(&mst->mst_lock);

	rc = trilin_dp_disable(dp, dp_panel);
	if (rc) {
		DP_ERR("[%d] DP display disable failed, rc=%d\n",
		       mst_encoder->id, rc);
		return;
	}

	rc = trilin_dp_unprepare(dp);
	if (rc) {
		DP_ERR("[%d] DP display unprepare failed, rc=%d\n",
		       mst_encoder->id, rc);
		return;
	}
	DP_MST_DEBUG("end\n");
}

/*compupte and updata payload slots*/
static int trilin_mst_encoder_atomic_check(struct drm_encoder *encoder,
					  struct drm_crtc_state *crtc_state,
					  struct drm_connector_state *conn_state)
{
	struct trilin_encoder *mst_encoder = encoder_to_trilin(encoder);
	struct trilin_dp *dp = mst_encoder->dp;
	struct drm_atomic_state *state = conn_state->state;
	struct drm_connector *connector = conn_state->connector;
	struct trilin_connector *conn = connector_to_trilin(connector);
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct trilin_dp_mst_private *mst_private = &dp->mst.private_info;
	struct drm_dp_mst_topology_mgr *mst_mgr = &mst_private->mst_mgr;
	struct drm_dp_mst_port *mst_port = conn->port;
	struct drm_dp_mst_topology_state *mst_state;
	int clock;
	//enum dc_color_depth color_depth;
	//bool is_y420 = false;
	//u8 bpp = 24; //fixme
	int ret = 0;
	const u8 link_coding_cap = DP_CAP_ANSI_8B10B;

	DP_MST_DEBUG("enter");

	if (!mst_port || !mst_private->mst_session_state)
		return 0;

	if (!crtc_state->connectors_changed && !crtc_state->mode_changed)
		return 0;

	ret = trilin_dp_encoder_atomic_check(encoder, crtc_state, conn_state);
	if (ret < 0) {
		return ret;
	}

	mst_state = drm_atomic_get_mst_topology_state(state, mst_mgr);
	if (IS_ERR(mst_state))
		return PTR_ERR(mst_state);

	if (!mst_state->pbn_div) {
		mst_state->pbn_div = drm_dp_get_vc_payload_bw(mst_mgr, dp->mode.link_rate, dp->mode.lane_cnt);
	}

	/*Fixme: suspend & resume state->duplicated is 1*/
	//if (!state->duplicated) {
		clock = adjusted_mode->clock;
		mst_encoder->pbn = drm_dp_calc_pbn_mode(clock, conn->config.bpp, false);
	//}

	mst_encoder->num_slots = drm_dp_atomic_find_time_slots(state, mst_mgr, mst_port, mst_encoder->pbn);
	if (mst_encoder->num_slots < 0) {
		DP_MST_DEBUG("failed finding vcpi slots: %d\n", (int)mst_encoder->num_slots);
		return mst_encoder->num_slots;
	}

	drm_dp_mst_update_slots(mst_state, link_coding_cap);

	ret = drm_dp_mst_atomic_check(state);
	DP_MST_DEBUG("drm_dp_mst_atomic_check : %d pbn=%d num_slots=%d pbn_div=%d clock=%d update state=%p (duplicated=%d)\n", ret
		, mst_encoder->pbn, mst_encoder->num_slots, mst_state->pbn_div, clock, state, state->duplicated);
	return ret;
}

#define TRILIN_DPTX_MIN_H_BACKPORCH	20

const struct drm_encoder_helper_funcs trilin_dp_encoder_helper_funcs = {
	.enable			    = trilin_mst_encoder_enable,
	.disable		    = trilin_mst_encoder_disable,
	.atomic_mode_set	= trilin_dp_encoder_atomic_mode_set,
	.atomic_check		= trilin_mst_encoder_atomic_check,
};

int trilin_drm_mst_encoder_init(struct trilin_dp *dp, int conn_base_id)
{
	int ret;
	int i;
	struct trilin_encoder *mst_encoder;
	struct drm_encoder *encoder;

	DP_MST_DEBUG("enter\n");
	memset(&dp->mst.private_info, 0, sizeof(dp->mst.private_info));

	dp->mst.private_info.mst_mgr.cbs = &mst_cbs;
	dp->mst.private_info.dp = dp;

	mutex_init(&dp->mst.private_info.mst_lock);
	mutex_init(&dp->mst.private_info.poll_irq_lock);
	/* create fake encoders */
	for (i = 0; i < MAX_DP_MST_DRM_ENCODERS; i++) {
		mst_encoder = &dp->mst.private_info.mst_encoders[i];
		encoder = &mst_encoder->base;
		mst_encoder->id = i;
		mst_encoder->dp = dp;
		encoder->possible_crtcs = TRILIN_DPTX_POSSIBLE_CRTCS_MST; //1 << i; // crtc0 only for encoder0
		drm_encoder_init(
			dp->drm,
			encoder,
			&trilin_dp_mst_enc_funcs,
			DRM_MODE_ENCODER_DPMST,
			"DP-MST %c", pipe_name(i));
		drm_encoder_helper_add(encoder, &trilin_dp_encoder_helper_funcs);
	}

	ret = drm_dp_mst_topology_mgr_init(&dp->mst.private_info.mst_mgr, dp->drm, &dp->aux,
					   16, MAX_DP_MST_DRM_ENCODERS,
					   conn_base_id);
	if (ret) {
		dp->mst.private_info.mst_mgr.cbs = NULL;
		return ret;
	}

	dp->mst_mgr = &dp->mst.private_info.mst_mgr;
	dp->mst.private_info.mst_initialized = true;
	DP_MST_DEBUG( "trilin_dp_mst_encoder_init end: baseid: %d\n", conn_base_id);
	return 0;
}

void trilin_drm_mst_encoder_cleanup(struct trilin_dp *dp)
{
	if (!dp->mst_mgr->cbs)
		return;

	drm_dp_mst_topology_mgr_destroy(dp->mst_mgr);
	/* encoders will get killed by normal cleanup */
	dp->mst.private_info.mst_mgr.cbs = NULL;
	dp->mst.private_info.mst_initialized = false;

	mutex_destroy(&dp->mst.private_info.mst_lock);
	mutex_destroy(&dp->mst.private_info.poll_irq_lock);
	DP_MST_DEBUG("end\n");
}

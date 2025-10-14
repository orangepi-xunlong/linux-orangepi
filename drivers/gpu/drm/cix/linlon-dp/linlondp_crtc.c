// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/spinlock.h>
#include <linux/clk-provider.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>
#include <drm/drm_self_refresh_helper.h>

#include "linlondp_dev.h"
#include "linlondp_kms.h"
#include "linlondp_drm.h"

extern int linlondp_atomic_replace_property_blob_from_id(
	struct drm_device *dev, struct drm_property_blob **blob,
	uint64_t blob_id, ssize_t expected_size, ssize_t expected_elem_size,
	bool *replaced);

void linlondp_crtc_get_color_config(struct drm_crtc_state *crtc_st,
				    u32 *color_depths, u32 *color_formats)
{
	struct drm_connector *conn;
	struct drm_connector_state *conn_st;
	u32 conn_color_formats = ~0u;
	int i, min_bpc = 31, conn_bpc = 0;

	for_each_new_connector_in_state(crtc_st->state, conn, conn_st, i) {
		if (conn_st->crtc != crtc_st->crtc)
			continue;

		conn_bpc = conn->display_info.bpc ? conn->display_info.bpc : 8;
		conn_color_formats &= conn->display_info.color_formats;

		if (conn_bpc < min_bpc)
			min_bpc = conn_bpc;
	}

	/* connector doesn't config any color_format, use RGB444 as default */
	if (!conn_color_formats)
		conn_color_formats = DRM_COLOR_FORMAT_RGB444;

	*color_depths = GENMASK(min_bpc, 0);
	*color_formats = conn_color_formats;
}

static void
linlondp_crtc_update_clock_ratio(struct linlondp_crtc_state *kcrtc_st)
{
	u64 pxlclk, aclk;

#if IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	return;
#endif
	if (!kcrtc_st->base.active) {
		kcrtc_st->clock_ratio = 0;
		return;
	}

	pxlclk = kcrtc_st->base.adjusted_mode.crtc_clock * 1000ULL;
	aclk = linlondp_crtc_get_aclk(kcrtc_st);

	kcrtc_st->clock_ratio = div64_u64(aclk << 32, pxlclk);
}

/**
 * linlondp_crtc_atomic_check - build display output data flow
 * @crtc: DRM crtc
 * @state: the crtc state object
 *
 * crtc_atomic_check is the final check stage, so beside build a display data
 * pipeline according to the crtc_state, but still needs to release or disable
 * the unclaimed pipeline resources.
 *
 * RETURNS:
 * Zero for success or -errno
 */
static int linlondp_crtc_atomic_check(struct drm_crtc *crtc,
				      struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state =
		drm_atomic_get_new_crtc_state(state, crtc);
	struct drm_crtc_state *old_crtc_state =
		drm_atomic_get_old_crtc_state(state, crtc);
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(crtc_state);
	int err;

	if (old_crtc_state->vrr_enabled != crtc_state->vrr_enabled)
		crtc_state->mode_changed = true;

	if (drm_atomic_crtc_needs_modeset(crtc_state))
		linlondp_crtc_update_clock_ratio(kcrtc_st);

	if (crtc_state->active) {
		err = linlondp_build_display_data_flow(kcrtc, kcrtc_st);
		if (err)
			return err;
	}

	/* release unclaimed pipeline resources */
	err = linlondp_release_unclaimed_resources(kcrtc->slave, kcrtc_st);
	if (err)
		return err;

	err = linlondp_release_unclaimed_resources(kcrtc->master, kcrtc_st);
	if (err)
		return err;

	return 0;
}

/* For active a crtc, mainly need two parts of preparation
 * 1. adjust display operation mode.
 * 2. enable needed clk
 */
static int linlondp_crtc_prepare(struct linlondp_crtc *kcrtc)
{
	struct linlondp_dev *mdev = kcrtc->base.dev->dev_private;
	struct linlondp_pipeline *master = kcrtc->master;
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(kcrtc->base.state);
	struct drm_display_mode *mode = &kcrtc_st->base.adjusted_mode;
	u32 new_mode;
	int err;

	mutex_lock(&mdev->lock);

	new_mode = mdev->dpmode | BIT(master->id);
	if (WARN_ON(new_mode == mdev->dpmode)) {
		err = 0;
		goto unlock;
	}

	err = mdev->funcs->change_opmode(mdev, new_mode);
	if (err) {
		DRM_ERROR("failed to change opmode: 0x%x -> 0x%x.\n,",
			  mdev->dpmode, new_mode);
		goto unlock;
	}

	mdev->dpmode = new_mode;
	/* Only need to enable aclk on single display mode, but no need to
	 * enable aclk it on dual display mode, since the dual mode always
	 * switch from single display mode, the aclk already enabled, no need
	 * to enable it again.
	 */
	if (new_mode != LINLONDP_MODE_DUAL_DISP) {
		mdev->aclk_freq = linlondp_crtc_get_aclk(kcrtc_st);
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
		err = clk_set_rate(mdev->aclk, mdev->aclk_freq);
		if (err)
			DRM_ERROR("failed to set aclk.\n");
		err = clk_prepare_enable(mdev->aclk);
		if (err)
			DRM_ERROR("failed to enable aclk.\n");
#endif
	}

#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	/* walk round to solve failing to set clock rate in std scence
	 * clk_get_rate will update clock state of ccf
	 */
	clk_get_rate(master->pxlclk);

	err = clk_set_rate(master->pxlclk, mode->crtc_clock * 1000);
	if (err)
		DRM_ERROR("failed to set pxlclk for pipe%d\n", master->id);
	err = clk_prepare_enable(master->pxlclk);
	if (err)
		DRM_ERROR("failed to enable pxl clk for pipe%d.\n", master->id);
#endif
unlock:
	mutex_unlock(&mdev->lock);

	return err;
}

static int linlondp_crtc_unprepare(struct linlondp_crtc *kcrtc)
{
	struct linlondp_dev *mdev = kcrtc->base.dev->dev_private;
	struct linlondp_pipeline *master = kcrtc->master;
	u32 new_mode;
	int err;

	mutex_lock(&mdev->lock);

	new_mode = mdev->dpmode & (~BIT(master->id));

	if (WARN_ON(new_mode == mdev->dpmode)) {
		err = 0;
		goto unlock;
	}

	err = mdev->funcs->change_opmode(mdev, new_mode);
	if (err) {
		DRM_ERROR("failed to change opmode: 0x%x -> 0x%x.\n,",
			  mdev->dpmode, new_mode);
		goto unlock;
	}

	mdev->dpmode = new_mode;
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	clk_disable_unprepare(master->pxlclk);
	if (new_mode == LINLONDP_MODE_INACTIVE)
		clk_disable_unprepare(mdev->aclk);
#endif
	mdev->aclk_freq = 0;
unlock:
	mutex_unlock(&mdev->lock);

	return err;
}

void linlondp_crtc_handle_event(struct linlondp_crtc *kcrtc,
				struct linlondp_events *evts)
{
	struct drm_crtc *crtc = &kcrtc->base;
	u32 events = evts->pipes[kcrtc->master->id];
	struct linlondp_wb_connector *wb_conn = kcrtc->wb_conn;

	if (events & LINLONDP_EVENT_VSYNC)
		drm_crtc_handle_vblank(crtc);

	/* handles writeback event */
	if (wb_conn) {
		if (events & LINLONDP_EVENT_EOW) {
			events &= ~LINLONDP_EVENT_EOW;
			evts->pipes[kcrtc->master->id]
				  &= ~LINLONDP_EVENT_EOW;
			atomic_or(BIT(kcrtc->master->id),
				  &wb_conn->received_eow);
		}

		if (kcrtc->side_by_side &&
		    (evts->pipes[kcrtc->slave->id] & LINLONDP_EVENT_EOW)) {
			evts->pipes[kcrtc->slave->id] &= ~LINLONDP_EVENT_EOW;
			atomic_or(BIT(kcrtc->slave->id),
				  &wb_conn->received_eow);
		}

		if (atomic_cmpxchg(&wb_conn->received_eow,
				   wb_conn->expected_eow,
				   0) == wb_conn->expected_eow) {
			drm_writeback_signal_completion(&wb_conn->base, 0);
		}
	}

	if (events & LINLONDP_EVENT_FLIP) {
		unsigned long flags;
		struct drm_pending_vblank_event *event;

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		if (kcrtc->disable_done) {
			complete_all(kcrtc->disable_done);
			kcrtc->disable_done = NULL;
		} else if (crtc->state->event) {
			event = crtc->state->event;
			/*
			 * Consume event before notifying drm core that flip
			 * happened.
			 */
			crtc->state->event = NULL;
			drm_crtc_send_vblank_event(crtc, event);
		} else {
			DRM_WARN(
				"CRTC[%d]: FLIP happened but no pending commit.\n",
				drm_crtc_index(&kcrtc->base));
		}
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
	}
}

static void linlondp_wb_job_timer_callback(struct timer_list *t)
{
	struct linlondp_wb_timer_data *data = from_timer(data, t, timer);
	struct drm_writeback_job *job;
	unsigned long flags;
	struct linlondp_crtc *kcrtc;
	struct linlondp_wb_connector *wb_conn;

	if (!data || !data->param) {
		DRM_WARN("PARM is null");
		return;
	}
	kcrtc = (struct linlondp_crtc *)data->param;
	wb_conn = kcrtc->wb_conn;
	spin_lock_irqsave(&wb_conn->base.job_lock, flags);
	job = list_first_entry_or_null(&wb_conn->base.job_queue,
				struct drm_writeback_job,
				list_entry);
	if (job) {
		DRM_DEBUG_ATOMIC("Timer fired! has JOB not signal\n");
		spin_unlock_irqrestore(&wb_conn->base.job_lock, flags);
		drm_writeback_signal_completion(&wb_conn->base, 0);
	} else {
		spin_unlock_irqrestore(&wb_conn->base.job_lock, flags);
	}
	wb_conn->timer_data.timer_started = false;
}

static void linlondp_set_wb_job_timer(struct linlondp_crtc *kcrtc)
{
	/*  Fixme: timeout may use 3 frames, now is 70
	 *  struct drm_device *drm = kcrtc->base.dev;
	 *  unsigned int pipe = drm_crtc_index(&kcrtc->base);
	 *  struct drm_vblank_crtc *vblank = &drm->vblank[pipe];
	 *  int timeout = vblank->framedur_ns * 3 / 1000000;
	 */
	struct linlondp_wb_connector *wb_conn = kcrtc->wb_conn;

	if (wb_conn->timer_data.timer_started)
		del_timer(&wb_conn->timer_data.timer);

	wb_conn->timer_data.param = kcrtc;
	timer_setup(&wb_conn->timer_data.timer, linlondp_wb_job_timer_callback, 0);
	mod_timer(&wb_conn->timer_data.timer, jiffies + msecs_to_jiffies(70));
	wb_conn->timer_data.timer_started = true;
}

static void linlondp_crtc_do_flush(struct drm_crtc *crtc,
				   struct drm_crtc_state *old)
{
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(crtc->state);
	struct linlondp_dev *mdev = kcrtc->base.dev->dev_private;
	struct linlondp_pipeline *master = kcrtc->master;
	struct linlondp_pipeline *slave = kcrtc->slave;
	struct linlondp_wb_connector *wb_conn = kcrtc->wb_conn;
	struct drm_connector_state *conn_st;

	DRM_DEBUG_ATOMIC("CRTC%d_FLUSH: active_pipes: 0x%x, affected: 0x%x.\n",
			 drm_crtc_index(crtc), kcrtc_st->active_pipes,
			 kcrtc_st->affected_pipes);

	/* step 1: update the pipeline/component state to HW */
	if (has_bit(master->id, kcrtc_st->affected_pipes))
		linlondp_pipeline_update(master, old->state);

	if (slave && has_bit(slave->id, kcrtc_st->affected_pipes))
		linlondp_pipeline_update(slave, old->state);

	conn_st = wb_conn ? wb_conn->base.base.state : NULL;
	if (conn_st && conn_st->writeback_job) {
		drm_writeback_queue_job(&wb_conn->base, conn_st);
		linlondp_set_wb_job_timer(kcrtc);
	}

	/* step 2: notify the HW to kickoff the update */
	mdev->funcs->flush(mdev, master->id, kcrtc_st->active_pipes);
}

static void linlondp_crtc_atomic_enable(struct drm_crtc *crtc,
					struct drm_atomic_state *state)
{
	int err, repeat = 0;
	struct drm_crtc_state *old = drm_atomic_get_old_crtc_state(state, crtc);
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_dev *mdev = kcrtc->base.dev->dev_private;
	struct linlondp_pipeline *master = kcrtc->master;
	struct drm_crtc_state *crtc_state = crtc->state;
	struct drm_display_mode *mode = &crtc_state->adjusted_mode;

	if (old && old->self_refresh_active) {
		DRM_DEBUG_ATOMIC("%s self_refresh_active, do nothing.\n", __func__);
		drm_crtc_vblank_on(crtc);
		linlondp_crtc_do_flush(crtc, old);
		return;
	}

	dev_info(crtc->dev->dev, "%s enter\n", __func__);

	if (mdev->enabled_by_gop) {
		mdev->enabled_by_gop = 0;
		if (mdev->funcs->gop_mode_changed(mdev, mode)) {
			mdev->funcs->disable_irq(mdev);
			mdev->funcs->close_gop(mdev);

			#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
			err = clk_prepare_enable(master->pxlclk);
			if (err)
				dev_err(mdev->dev, "%s, failed to enable pxlclk.\n", __func__);
			#endif

			if (__clk_is_enabled(master->pxlclk)) {
				clk_disable_unprepare(master->pxlclk);
				dev_info(mdev->dev, "%s, succeed to disable pxlclk.\n", __func__);
			}

			reset_control_assert(mdev->ip_rst);
			mdelay(1);
			reset_control_deassert(mdev->ip_rst);
			mdelay(1);
		}
	}

	err = clk_prepare_enable(mdev->aclk);
	if (err)
		dev_err(mdev->dev, "%s, failed to enable aclk.\n", __func__);
	else
		dev_info(mdev->dev, "%s, succeed to enable aclk.\n", __func__);

	/* wait for pm runtime enabled */
	if (!pm_runtime_enabled(crtc->dev->dev)) {
		do {
			usleep_range(1000, 1200);
			if (++repeat == 500)
				break;
		} while (!pm_runtime_enabled(crtc->dev->dev));

		if (repeat == 500) {
			dev_err(mdev->dev, "%s: pm runtime enable timeout\n", __func__);
			return;
		}
	}

	pm_runtime_get_sync(crtc->dev->dev);
	//enable irq once more in std scence
	mdev->funcs->enable_irq(mdev);
	linlondp_crtc_prepare(to_kcrtc(crtc));
	drm_crtc_vblank_on(crtc);
	WARN_ON(drm_crtc_vblank_get(crtc));
	linlondp_crtc_do_flush(crtc, old);

	dev_info(crtc->dev->dev, "%s exit\n", __func__);
}

void linlondp_crtc_flush_and_wait_for_flip_done(
	struct linlondp_crtc *kcrtc, struct completion *input_flip_done,
	bool disable_crtc)
{
	struct drm_device *drm = kcrtc->base.dev;
	struct linlondp_dev *mdev = kcrtc->master->mdev;
	struct completion *flip_done;
	struct completion temp;
	int timeout;
	unsigned long flags;
	unsigned int pipe = drm_crtc_index(&kcrtc->base);
	struct drm_vblank_crtc *vblank = &drm->vblank[pipe];

	/* if caller doesn't send a flip_done, use a private flip_done */
	if (input_flip_done) {
		flip_done = input_flip_done;
	} else {
		spin_lock_irqsave(&drm->event_lock, flags);
		if (kcrtc->disable_done != NULL) {
			DRM_DEBUG_ATOMIC(
				"CRTC%d has last flip done %p do nothing",
				kcrtc->master->id, kcrtc->disable_done);
			spin_unlock_irqrestore(&drm->event_lock, flags);
			return;
		}
		init_completion(&temp);
		kcrtc->disable_done = &temp;
		flip_done = &temp;
		spin_unlock_irqrestore(&drm->event_lock, flags);
	}

	if (disable_crtc)
		mdev->funcs->flush(mdev, kcrtc->master->id, 0);

	/* wait the flip take affect. wait for a maximum time of 60 frames*/
	timeout = 60 * vblank->framedur_ns / 1000000;
	timeout = wait_for_completion_timeout(flip_done, msecs_to_jiffies(timeout));
	if (timeout == 0) {
		dev_err(mdev->dev, "wait pipe%d flip done timeout\n",
			kcrtc->master->id);
		if (!input_flip_done) {
			spin_lock_irqsave(&drm->event_lock, flags);
			kcrtc->disable_done = NULL;
			spin_unlock_irqrestore(&drm->event_lock, flags);
		}
	}
}

static void linlondp_crtc_atomic_disable(struct drm_crtc *crtc,
					 struct drm_atomic_state *state)
{
	struct drm_crtc_state *old = drm_atomic_get_old_crtc_state(state, crtc);
	struct drm_crtc_state *new_state = drm_atomic_get_new_crtc_state(state, crtc);

	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_crtc_state *old_st = to_kcrtc_st(old);
	struct linlondp_pipeline *master = kcrtc->master;
	struct linlondp_pipeline *slave = kcrtc->slave;
	struct linlondp_dev *mdev = kcrtc->base.dev->dev_private;
	struct completion *disable_done;
	bool needs_phase2 = false;
	bool needs_slave_phase2 = false;

	/*this log helps for really disabling crtc not self_refresh*/
	if (!new_state || !new_state->self_refresh_active)
		dev_info(crtc->dev->dev, "%s enter\n", __func__);

	DRM_DEBUG_ATOMIC("CRTC%d_DISABLE: active_pipes: 0x%x, affected: 0x%x\n",
			 drm_crtc_index(crtc), old_st->active_pipes,
			 old_st->affected_pipes);

	if (slave && has_bit(slave->id, old_st->active_pipes))
		needs_slave_phase2 = linlondp_pipeline_disable(slave, old->state);

	if (has_bit(master->id, old_st->active_pipes))
		needs_phase2 = linlondp_pipeline_disable(master, old->state);

	/* crtc_disable has two scenarios according to the state->active switch.
	 * 1. active -> inactive
	 *    this commit is a disable commit. and the commit will be finished
	 *    or done after the disable operation. on this case we can directly
	 *    use the crtc->state->event to tracking the HW disable operation.
	 * 2. active -> active
	 *    the crtc->commit is not for disable, but a modeset operation when
	 *    crtc is active, such commit actually has been completed by 3
	 *    DRM operations:
	 *    crtc_disable, update_planes(crtc_flush), crtc_enable
	 *    so on this case the crtc->commit is for the whole process.
	 *    we can not use it for tracing the disable, we need a temporary
	 *    flip_done for tracing the disable. and crtc->state->event for
	 *    the crtc_enable operation.
	 *    That's also the reason why skip modeset commit in
	 *    linlondp_crtc_atomic_flush()
	 */
	disable_done = (needs_phase2 || crtc->state->active) ?
			       NULL :
			       &crtc->state->commit->flip_done;

	/* wait phase 1 disable done */
	linlondp_crtc_flush_and_wait_for_flip_done(kcrtc, disable_done, true);

	/* phase 2 */
	if (needs_slave_phase2)
		linlondp_pipeline_disable(slave, old->state);

	if (needs_phase2) {
		linlondp_pipeline_disable(kcrtc->master, old->state);

		disable_done = crtc->state->active ?
				       NULL :
				       &crtc->state->commit->flip_done;

		linlondp_crtc_flush_and_wait_for_flip_done(kcrtc, disable_done, true);
	}

	if (new_state && new_state->self_refresh_active) {
		DRM_DEBUG_ATOMIC("%s, self_refresh_active, do nothing\n", __func__);
		return;
	}

	drm_crtc_vblank_put(crtc);
	drm_crtc_vblank_off(crtc);
	linlondp_crtc_unprepare(kcrtc);
	pm_runtime_put(crtc->dev->dev);
	clk_disable_unprepare(mdev->aclk);

	dev_info(crtc->dev->dev, "%s exit\n", __func__);
}

static void linlondp_crtc_atomic_flush(struct drm_crtc *crtc,
				       struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state =
		drm_atomic_get_new_crtc_state(state, crtc);
	struct drm_crtc_state *old = drm_atomic_get_old_crtc_state(state, crtc);

	/* commit with modeset will be handled in enable/disable */
	if (drm_atomic_crtc_needs_modeset(crtc_state))
		return;

	linlondp_crtc_do_flush(crtc, old);
}

static unsigned long linlondp_smart_aclk_freq(unsigned long pxlclk)
{
	unsigned long aclk_freq = 0;

	if (pxlclk >= 445500000)
		aclk_freq = 800000000;
	else if (pxlclk >= 297000000)
		aclk_freq = 600000000;
	else if (pxlclk >= 148500000)
		aclk_freq = 400000000;
	else if (pxlclk >= 74250000)
		aclk_freq = 300000000;
	else
		aclk_freq = 200000000;

	return aclk_freq;
}

/* Returns the minimum frequency of the aclk rate (main engine clock) in Hz */
static unsigned long linlondp_calc_min_aclk_rate(struct linlondp_crtc *kcrtc,
						 unsigned long pxlclk)
{
	unsigned long min_aclk = 0;
	struct drm_crtc *crtc = &kcrtc->base;
	struct linlondp_dev *mdev = crtc->dev->dev_private;
	/* Once dual-link one display pipeline drives two display outputs,
	 * the aclk needs run on the double rate of pxlclk
	 */
	if (kcrtc->master->dual_link && !kcrtc->side_by_side)
		min_aclk = pxlclk * 2;
	else
		min_aclk = pxlclk;

	if (mdev->smart_aclk_freq)
		min_aclk = linlondp_smart_aclk_freq(min_aclk);

	if (mdev->aclk_freq_fixed)
		min_aclk = mdev->aclk_freq_fixed;

	return min_aclk;
}

/* Get current aclk rate that specified by state */
unsigned long linlondp_crtc_get_aclk(struct linlondp_crtc_state *kcrtc_st)
{
	struct drm_crtc *crtc = kcrtc_st->base.crtc;
	unsigned long pxlclk = kcrtc_st->base.adjusted_mode.crtc_clock * 1000;
	unsigned long min_aclk;

	min_aclk = linlondp_calc_min_aclk_rate(to_kcrtc(crtc), pxlclk);

	return min_aclk;
}

static enum drm_mode_status
linlondp_crtc_mode_valid(struct drm_crtc *crtc,
			 const struct drm_display_mode *m)
{
	struct linlondp_dev *mdev = crtc->dev->dev_private;
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_pipeline *master = kcrtc->master;
	unsigned long min_pxlclk, min_aclk;
	u8 pixel_per_cycle = 1;

	if (m->flags & DRM_MODE_FLAG_INTERLACE)
		return MODE_NO_INTERLACE;

	if (master->force_pixel_per_cycle != 0)
		pixel_per_cycle = master->force_pixel_per_cycle;

	if ((m->clock > 600000) && (m->clock < 600000 * 2))
		pixel_per_cycle = 2;
	else if (m->clock > 600000 * 2) {
		DRM_DEBUG_ATOMIC(
			"engine clk can't satisfy the requirement of %s-clk: %d.\n",
			m->name, m->clock);
		return MODE_CLOCK_HIGH;
	}

#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	min_pxlclk = m->clock * 1000;
	if (master->dual_link || (pixel_per_cycle == 2))
		min_pxlclk /= 2;

	//FIXME
	if (min_pxlclk > clk_round_rate(master->pxlclk, min_pxlclk)) {
		DRM_DEBUG_ATOMIC("pxlclk doesn't support %lu Hz\n", min_pxlclk);

		return MODE_NOCLOCK;
	}

	min_aclk = linlondp_calc_min_aclk_rate(to_kcrtc(crtc), min_pxlclk);
	if (clk_round_rate(mdev->aclk, min_aclk) < min_aclk) {
		DRM_DEBUG_ATOMIC(
			"engine clk can't satisfy the requirement of %s-clk: %lu.\n",
			m->name, min_pxlclk);

		return MODE_CLOCK_HIGH;
	}
#endif
	return MODE_OK;
}

static bool linlondp_crtc_mode_fixup(struct drm_crtc *crtc,
				     const struct drm_display_mode *m,
				     struct drm_display_mode *adjusted_mode)
{
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	unsigned long clk_rate;
	u16 hor_divisor = 1;
	u8 pixel_per_cycle = 1;
	bool rebuilt_crtcs = false;
	struct linlondp_dev *mdev = crtc->dev->dev_private;
	struct linlondp_kms_dev *kms;

	if (kcrtc->master->force_pixel_per_cycle != 0)
		pixel_per_cycle = kcrtc->master->force_pixel_per_cycle;

	if ((m->clock > 600000) && (m->clock < 600000 * 2)) {
		pixel_per_cycle = 2;
		if (!mdev->side_by_side) {
			mdev->side_by_side = 1;
			mdev->auto2ppc = true;
			rebuilt_crtcs = true;
		}
	} else if (mdev->side_by_side && mdev->auto2ppc)  {
		mdev->side_by_side = 0;
		rebuilt_crtcs = true;
	}

	if (rebuilt_crtcs) {
		kms = container_of(crtc->dev, struct linlondp_kms_dev, base);
		linlondp_kms_setup_crtcs(kms, mdev);
		DRM_DEBUG_ATOMIC("%s sidy_by_side. rebuild crtcs",
			mdev->side_by_side ? "enable" : "disable");
	}

	kcrtc->master->pixel_per_cycle = pixel_per_cycle;

	drm_mode_set_crtcinfo(adjusted_mode, 0);
	/* In dual link half the horizontal settings */
	if (kcrtc->master->dual_link)
		hor_divisor *= 2;
	/* In n ppc mode */
	hor_divisor *= pixel_per_cycle;
	if (hor_divisor != 1) {
		adjusted_mode->crtc_clock /= hor_divisor;
		adjusted_mode->crtc_hdisplay /= hor_divisor;
		adjusted_mode->crtc_hsync_start /= hor_divisor;
		adjusted_mode->crtc_hsync_end /= hor_divisor;
		adjusted_mode->crtc_htotal /= hor_divisor;
	}
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	clk_rate = adjusted_mode->crtc_clock * 1000;
	/* crtc_clock will be used as the linlondp output pixel clock */
	adjusted_mode->crtc_clock =
		clk_round_rate(kcrtc->master->pxlclk, clk_rate) / 1000;
#endif
	return true;
}

static const struct drm_crtc_helper_funcs linlondp_crtc_helper_funcs = {
	.atomic_check = linlondp_crtc_atomic_check,
	.atomic_flush = linlondp_crtc_atomic_flush,
	.atomic_enable = linlondp_crtc_atomic_enable,
	.atomic_disable = linlondp_crtc_atomic_disable,
	.mode_valid = linlondp_crtc_mode_valid,
	.mode_fixup = linlondp_crtc_mode_fixup,
};

static void linlondp_crtc_reset(struct drm_crtc *crtc)
{
	struct linlondp_crtc_state *state;

	if (crtc->state)
		__drm_atomic_helper_crtc_destroy_state(crtc->state);

	kfree(to_kcrtc_st(crtc->state));
	crtc->state = NULL;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (state) {
		state->en_protected_mode = false;
		__drm_atomic_helper_crtc_reset(crtc, &state->base);
	}
}

static struct drm_crtc_state *
linlondp_crtc_atomic_duplicate_state(struct drm_crtc *crtc)
{
	struct linlondp_crtc_state *old = to_kcrtc_st(crtc->state);
	struct linlondp_crtc_state *new;

	new = kzalloc(sizeof(*new), GFP_KERNEL);
	if (!new)
		return NULL;

	__drm_atomic_helper_crtc_duplicate_state(crtc, &new->base);

	new->affected_pipes = old->active_pipes;
	new->clock_ratio = old->clock_ratio;
	new->max_slave_zorder = old->max_slave_zorder;
	new->en_protected_mode = old->en_protected_mode;

	return &new->base;
}

static void linlondp_crtc_atomic_destroy_state(struct drm_crtc *crtc,
					       struct drm_crtc_state *state)
{
	__drm_atomic_helper_crtc_destroy_state(state);
	kfree(to_kcrtc_st(state));
}

static int linlondp_crtc_vblank_enable(struct drm_crtc *crtc)
{
	struct linlondp_dev *mdev = crtc->dev->dev_private;
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);

	mdev->funcs->on_off_vblank(mdev, kcrtc->master->id, true);
	return 0;
}

static void linlondp_crtc_vblank_disable(struct drm_crtc *crtc)
{
	struct linlondp_dev *mdev = crtc->dev->dev_private;
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);

	mdev->funcs->on_off_vblank(mdev, kcrtc->master->id, false);
}

static int linlondp_crtc_atomic_get_property(struct drm_crtc *crtc,
					     const struct drm_crtc_state *state,
					     struct drm_property *property,
					     uint64_t *val)
{
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(state);

	if (property == kcrtc->protected_mode_property)
		*val = kcrtc_st->en_protected_mode;
	else if (property == kcrtc->ctm_ext_property) {
		*val = (state->ctm) ? state->ctm->base.id : 0;
	} else {
		DRM_DEBUG_DRIVER("Unknown property %s\n", property->name);
		return -EINVAL;
	}

	return 0;
}

static int linlondp_crtc_atomic_set_property(struct drm_crtc *crtc,
					     struct drm_crtc_state *state,
					     struct drm_property *property,
					     uint64_t val)
{
	struct linlondp_crtc *kcrtc = to_kcrtc(crtc);
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(state);
	int ret = 0;
	bool replaced = false;

	if (property == kcrtc->protected_mode_property)
		kcrtc_st->en_protected_mode = !!val;
	else if (property == kcrtc->ctm_ext_property) {
		ret = linlondp_atomic_replace_property_blob_from_id(
			crtc->dev, &state->ctm, val,
			sizeof(struct color_ctm_ext), -1, &replaced);
		state->color_mgmt_changed |= replaced;
	} else {
		DRM_DEBUG_DRIVER("Unknown property %s\n", property->name);
		return -EINVAL;
	}

	return ret;
}

static int
linlondp_crtc_create_protected_mode_property(struct linlondp_crtc *kcrtc)
{
	struct drm_crtc *crtc = &kcrtc->base;
	struct drm_property *prop;

	prop = drm_property_create_bool(crtc->dev, DRM_MODE_PROP_ATOMIC,
					"PROTECTED_MODE");
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&crtc->base, prop, 0);
	kcrtc->protected_mode_property = prop;

	return 0;
}

static int linlondp_crtc_create_ctm_ext_property(struct linlondp_crtc *kcrtc)
{
	struct drm_crtc *crtc = &kcrtc->base;
	struct drm_property *prop;

	prop = drm_property_create(crtc->dev, DRM_MODE_PROP_BLOB, "CTM_EXT", 0);
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&crtc->base, prop, 0);
	kcrtc->ctm_ext_property = prop;
	return 0;
}

static void linlondp_crtc_destroy(struct drm_crtc *crtc)
{
	drm_self_refresh_helper_cleanup(crtc);
	drm_crtc_cleanup(crtc);
}

static const struct drm_crtc_funcs linlondp_crtc_funcs = {
	//.gamma_set        = drm_atomic_helper_legacy_gamma_set,
	.destroy = linlondp_crtc_destroy,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.reset = linlondp_crtc_reset,
	.atomic_duplicate_state = linlondp_crtc_atomic_duplicate_state,
	.atomic_destroy_state = linlondp_crtc_atomic_destroy_state,
	.enable_vblank = linlondp_crtc_vblank_enable,
	.disable_vblank = linlondp_crtc_vblank_disable,
	.atomic_get_property = linlondp_crtc_atomic_get_property,
	.atomic_set_property = linlondp_crtc_atomic_set_property,
};

int linlondp_kms_setup_crtcs(struct linlondp_kms_dev *kms,
			     struct linlondp_dev *mdev)
{
	struct linlondp_crtc *crtc;
	struct linlondp_pipeline *master, *slave;
	char str[16];
	int i;

	kms->n_crtcs = 0;

	for (i = 0; i < mdev->n_pipelines; i++) {
		crtc = &kms->crtcs[kms->n_crtcs];
		master = mdev->pipelines[i];

		crtc->master = master;
		crtc->slave = linlondp_pipeline_get_slave(master);
		crtc->side_by_side = mdev->side_by_side;

		if (crtc->slave)
			sprintf(str, "pipe-%d", crtc->slave->id);
		else
			sprintf(str, "None");

		DRM_INFO("CRTC-%d: master(pipe-%d) slave(%s) sbs(%s).\n",
			 kms->n_crtcs, master->id, str,
			 crtc->side_by_side ? "On" : "Off");

		kms->n_crtcs++;
	}

	if (mdev->side_by_side) {
		master = mdev->pipelines[mdev->side_by_side_master];
		slave = linlondp_pipeline_get_slave(master);
		for (i = 0; i < master->n_layers; i++)
			master->layers[i]->sbs_slave = slave->layers[i];
	}

	return 0;
}

static struct drm_plane *get_crtc_primary(struct linlondp_kms_dev *kms,
					  struct linlondp_crtc *crtc)
{
	struct linlondp_plane *kplane;
	struct drm_plane *plane;

	drm_for_each_plane(plane, &kms->base) {
		if (plane->type != DRM_PLANE_TYPE_PRIMARY)
			continue;

		kplane = to_kplane(plane);
		/* only master can be primary */
		if (kplane->layer->base.pipeline == crtc->master)
			return plane;
	}

	return NULL;
}

static int linlondp_crtc_add(struct linlondp_kms_dev *kms,
			     struct linlondp_crtc *kcrtc)
{
	struct drm_crtc *crtc = &kcrtc->base;
	int err;

	err = drm_crtc_init_with_planes(&kms->base, crtc,
					get_crtc_primary(kms, kcrtc), NULL,
					&linlondp_crtc_funcs, NULL);
	if (err)
		return err;

	drm_crtc_helper_add(crtc, &linlondp_crtc_helper_funcs);

	if (has_acpi_companion(kms->base.dev)) {
		/* Reuse this field to transfer port fwnode_handle for ACPI */
		crtc->port = (void *)kcrtc->master->fwnode_output_port;
	} else {
		crtc->port = kcrtc->master->of_output_port;
	}

	err = drm_self_refresh_helper_init(crtc);
	if (err)
		dev_warn(kms->base.dev,
		"Failed to init %s with SR helpers %d\n",
		crtc->name, err);

	drm_crtc_enable_color_mgmt(crtc, LINLONDP_COLOR_LUT_SIZE, true, LINLONDP_COLOR_LUT_SIZE);

	err = linlondp_crtc_create_ctm_ext_property(kcrtc);
	if (err)
		return err;

	err = linlondp_crtc_create_protected_mode_property(kcrtc);

	return err;
}

int linlondp_kms_add_crtcs(struct linlondp_kms_dev *kms,
			   struct linlondp_dev *mdev)
{
	int i, err;

	for (i = 0; i < kms->n_crtcs; i++) {
		err = linlondp_crtc_add(kms, &kms->crtcs[i]);
		if (err)
			return err;
	}

	return 0;
}

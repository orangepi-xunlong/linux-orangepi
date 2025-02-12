// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/component.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/trace_events.h>
#include <linux/of_reserved_mem.h>
#include <dt-bindings/display/ky-dpu.h>
#include "ky_cmdlist.h"
#include "ky_dmmu.h"
#include "ky_drm.h"
#include "ky_dpu.h"
#include "ky_gem.h"
#include "ky_lib.h"
#include "ky_bootloader.h"
#include "dpu/dpu_saturn.h"
#include "dpu/dpu_debug.h"
#include "sysfs/sysfs_display.h"
#include "dpu/dpu_trace.h"

LIST_HEAD(dpu_core_head);

static int ky_dpu_init(struct ky_dpu *dpu);
static int ky_dpu_uninit(struct ky_dpu *dpu);
static int dpu_pm_suspend(struct device *dev);
static int dpu_pm_resume(struct device *dev);

static int ky_crtc_atomic_check_color_matrix(struct drm_crtc *crtc,
					  struct drm_crtc_state *state)
{
	struct ky_crtc_state *ac = to_ky_crtc_state(state);
	struct drm_property_blob *blob = ac->color_matrix_blob_prop;
	int *data;
	int n;

	if (blob){
		data = (int *)blob->data;
		for (n = 0; n < 9; n++){
			if ((data[n] > 8191) || (data[n] < -8192)){
				DRM_DEBUG("The value of color matrix coeffs is invalid: value %d, n %d\n", data[n], n);
				return -EINVAL;
			}
			else
				data[n] = data[n] & 0x3FFF;
		}

		for (n = 9; n < 12; n++){
			if ((data[n] > 4095) || (data[n] < -4096)){
				DRM_DEBUG("The value of color matrix offset is invalid: value %d, n %d\n", data[n], n);
				return -EINVAL;
			}
			else
				data[n] = data[n] & 0x3FFF;
		}
	}

	return 0;
}

static int ky_crtc_atomic_check_scaling(struct drm_crtc *crtc,
					    struct drm_crtc_state *crtc_state)
{
	struct drm_plane *plane;
	struct ky_crtc_state *ac = to_ky_crtc_state(crtc_state);
	const struct drm_plane_state *pstate;
	struct ky_dpu_scaler *scaler = NULL;
	struct ky_plane_state *ky_pstate;
	u32 i;

	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, crtc_state) {
		ky_pstate = to_ky_plane_state(pstate);

		if (ky_pstate->use_scl) {
			for (i = 0; i < MAX_SCALER_NUMS; i++) {
				scaler = &(ac->scalers[i]);
				if (scaler->in_use == 0x0 || scaler->rdma_id == ky_pstate->rdma_id) {
					scaler->in_use |= (1 << plane->index);
					scaler->rdma_id = ky_pstate->rdma_id;
					break;
				}
			}

			if (i == MAX_SCALER_NUMS)
				return -EINVAL;
		}
	}

	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, crtc_state) {
		ky_pstate = to_ky_plane_state(pstate);

		if (ky_pstate->rdma_id != RDMA_INVALID_ID) {
			for (i = 0; i < MAX_SCALER_NUMS; i++) {
				scaler = &(ac->scalers[i]);
				if (scaler->rdma_id == ky_pstate->rdma_id)
					ky_pstate->scaler_id = i;
			}
		}
	}

	return 0;
}

/* crtc actual mclk depends on max(mclk, aclk) */
static int ky_crtc_atomic_update_mclk(struct drm_crtc *crtc,
		struct drm_crtc_state *old_state)
{
	struct ky_crtc_state *new_ac = to_ky_crtc_state(crtc->state);

	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct dpu_clk_context *clk_ctx = NULL;

	if (dpu == NULL)
		return 0;

	if (!dpu->enable_auto_fc)
		return 0;

	/* when shutdown, all planes disabled, crtc still work, need keep cur mclk */
	if (new_ac->real_mclk == 0) {
		trace_u64_data("new_ac->real_mclk", new_ac->real_mclk);
		return 0;
	}

	clk_ctx = &dpu->clk_ctx;
	cancel_work_sync(&dpu->work_update_clk); /* incase mclk fq doing */

	dpu->cur_mclk = clk_get_rate(clk_ctx->mclk);
	dpu->new_mclk = clk_round_rate(clk_ctx->mclk, new_ac->real_mclk);

	trace_u64_data("crtc mclk cur", dpu->cur_mclk);
	trace_u64_data("crtc mclk new", dpu->new_mclk);
	if (dpu->core && dpu->core->update_clk && (dpu->new_mclk > dpu->cur_mclk)) {
			trace_u64_data("mclk increase to", dpu->new_mclk);
			dpu->core->update_clk(dpu, dpu->new_mclk);
	}

	cancel_work_sync(&dpu->work_update_bw);

	dpu->new_bw = new_ac->bw;

	trace_u64_data("crtc bw cur", dpu->cur_bw);
	trace_u64_data("crtc bw new", dpu->new_bw);

	if (dpu->core && dpu->core->update_bw && (dpu->new_bw > dpu->cur_bw)) {
			trace_u64_data("bw increase to", dpu->new_bw);
			dpu->core->update_bw(dpu, dpu->new_bw);
	}

	return 0;
}

/* crtc aclk depends on sum of all rdma bw */
static int ky_crtc_atomic_check_aclk(struct drm_crtc *crtc,
					    struct drm_crtc_state *crtc_state)
{
	const struct drm_plane_state *pstate;
	struct drm_plane *plane;
	struct ky_crtc_state *ac = to_ky_crtc_state(crtc_state); /* new state */
	bool scl_en = false;
	uint64_t afbc_effc = ~0ULL;
	uint64_t tmp, tmp1;
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	if (!dpu->enable_auto_fc)
		return 0;

	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, crtc_state) {
		uint64_t tmp_bw = to_ky_plane_state(pstate)->bw;
		bool tmp_scl_en = to_ky_plane_state(pstate)->use_scl;
		ac->bw += tmp_bw;
		if (tmp_scl_en) {
			scl_en = true;
			if (afbc_effc > to_ky_plane_state(pstate)->afbc_effc)
				afbc_effc = to_ky_plane_state(pstate)->afbc_effc;
		}
	}

	if (ac->bw == 0)
		ac->bw = DPU_MIN_QOS_REQ;

	if (ac->bw > (DPU_MAX_QOS_REQ * MHZ2KHZ)) {
		DRM_INFO("crtc:%lld bandwidth too large\n", ac->bw);
		return -EINVAL;
	}

	if (scl_en) {
		//ac->aclk = max(ac->bw / 16 * 10 / 5, ac->mclk / afbc_effc);
		tmp = ac->bw;
		do_div(tmp, 16);
		tmp = tmp * 100;
		do_div(tmp, 50);
		tmp1 = ac->mclk;
		do_div(tmp1, afbc_effc);
		ac->aclk = max(tmp, tmp1);
	}
	else {
		//ac->aclk = ac->bw / 16 * 10 / 5;
		tmp = ac->bw;
		do_div(tmp, 16);
		tmp = tmp * 100;
		do_div(tmp, 50);
		ac->aclk = tmp;
	}

	trace_u64_data("crtc calc bw", ac->bw);
	ac->bw += DPU_MARGIN_QOS_REQ * MHZ2KHZ;

	ac->real_mclk = max(ac->mclk, ac->aclk);

	if (ac->real_mclk < dpu->min_mclk)
		ac->real_mclk = dpu->min_mclk;

	trace_u64_data("crtc fixed bw", ac->bw);
	trace_u64_data("crtc calc aclk", ac->aclk);
	trace_u64_data("crtc calc mclk", ac->mclk);
	trace_u64_data("crtc calc real_mclk", ac->real_mclk);

	return 0;
}

/* crtc mclk depends on max of all rdma mclk */
static int ky_crtc_atomic_check_mclk(struct drm_crtc *crtc,
					    struct drm_crtc_state *crtc_state)
{
	const struct drm_plane_state *pstate;
	struct drm_plane *plane;
	struct ky_crtc_state *ac = to_ky_crtc_state(crtc_state);
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	if (!dpu->enable_auto_fc)
		return 0;

	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, crtc_state) {
		uint64_t tmp_mclk = to_ky_plane_state(pstate)->mclk;
		if (ac->mclk < tmp_mclk)
			ac->mclk = tmp_mclk;
	}

	return 0;
}

static int ky_crtc_atomic_check_fbmem(struct drm_crtc *crtc,
					    struct drm_crtc_state *crtc_state)
{
	struct ky_dpu_rdma *rdmas = to_ky_crtc_state(crtc_state)->rdmas;
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct ky_drm_private *priv = crtc->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	const struct drm_plane_state *pstate;
	struct drm_plane *plane;
	/* Calc each rdma required fbc mem size */
	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, crtc_state) {
		u32 rdma_id = to_ky_plane_state(pstate)->rdma_id;
		u32 layer_fbcmem_size = to_ky_plane_state(pstate)->fbcmem_size;
		if (rdma_id != RDMA_INVALID_ID) {
			if (rdmas[rdma_id].mode == UP_DOWN)
				rdmas[rdma_id].fbcmem.size = max(layer_fbcmem_size,
								 rdmas[rdma_id].fbcmem.size);
			else
				rdmas[rdma_id].fbcmem.size += layer_fbcmem_size;
		}
	}

	/* Adjust each rdma's fbcmem layout */
	return dpu->core->adjust_rdma_fbcmem(hwdev, rdmas);

}

static void ky_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct videomode vm;

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_crtc_mode_set_nofb(dpu->dev_id);
	drm_display_mode_to_videomode(&crtc->mode, &vm);
}

static void ky_crtc_atomic_enable(struct drm_crtc *crtc,
				   struct drm_atomic_state *old_state)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	DRM_INFO("%s(power on)\n", __func__);
	trace_ky_crtc_atomic_enable(dpu->dev_id);

	if (dpu->logo_booton && dpu->type == DSI) {
		pm_runtime_enable(dpu->dev);

		pm_runtime_get_sync(dpu->dev);
		dpu_pm_resume(dpu->dev);
		dpu_pm_suspend(dpu->dev);
		pm_runtime_put_sync(dpu->dev);
		dpu->logo_booton = false;
		msleep(10);
	}

	pm_runtime_get_sync(dpu->dev);
	dpu_pm_resume(dpu->dev);

#ifdef CONFIG_KY_DEBUG
	dpu->is_working = true;
#endif
	drm_crtc_vblank_on(&dpu->crtc);

	ky_dpu_init(dpu);

	if (!IS_ERR_OR_NULL(dpu->enable_gpio)) {
		gpiod_direction_output(dpu->enable_gpio, 1);
	}
}

static void ky_crtc_atomic_disable(struct drm_crtc *crtc,
				    struct drm_atomic_state *old_state)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct drm_device *drm = dpu->crtc.dev;

	DRM_INFO("%s(power off)\n", __func__);
	trace_ky_crtc_atomic_disable(dpu->dev_id);

	if (!IS_ERR_OR_NULL(dpu->enable_gpio)) {
		gpiod_direction_output(dpu->enable_gpio, 0);
	}

	ky_dpu_uninit(dpu);

	drm_crtc_vblank_off(&dpu->crtc);
#ifdef CONFIG_KY_DEBUG
	dpu->is_working = false;
#endif

	dpu_pm_suspend(dpu->dev);
	pm_runtime_put_sync(dpu->dev);

	spin_lock_irq(&drm->event_lock);
	if (crtc->state->event) {
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
	}
	spin_unlock_irq(&drm->event_lock);
}

static int ky_crtc_atomic_check(struct drm_crtc *crtc,
						struct drm_atomic_state *atomic_state)
{
	struct drm_crtc_state *state = drm_atomic_get_new_crtc_state(atomic_state, crtc);
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	int ret = 0;

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_crtc_atomic_check(dpu->dev_id);

	ret = ky_crtc_atomic_check_scaling(crtc, state);

	if (ky_crtc_atomic_check_color_matrix(crtc, state)){
		DRM_DEBUG("The value of color matrix is invalid\n");
		return -EINVAL;
	}

	if (ky_crtc_atomic_check_fbmem(crtc, state)) {
		DRM_DEBUG("Failed to satisfy fbcmem size for all rdmas!\n");
		return -EINVAL;
	}

	if (ky_crtc_atomic_check_mclk(crtc, state)) {
		DRM_ERROR("Failed to satisfy mclk for all rdmas!\n");
		return -EINVAL;
	}

	if (ky_crtc_atomic_check_aclk(crtc, state)) {
		DRM_INFO("Failed to satisfy aclk for all rdmas!\n");
		return -EINVAL;
	}
	return ret;
}

static void ky_crtc_atomic_begin(struct drm_crtc *crtc,
				  struct drm_atomic_state *state)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_crtc_atomic_begin(dpu->dev_id);
}

static void ky_crtc_atomic_flush(struct drm_crtc *crtc,
				  struct drm_atomic_state *state)

{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct drm_crtc_state *old_state = drm_atomic_get_old_crtc_state(state, crtc);

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_crtc_atomic_flush(dpu->dev_id);
	// ky_dpu_wb_config(dpu);
	saturn_conf_dpuctrl_color_matrix(dpu, old_state);
	ky_crtc_atomic_update_mclk(crtc, old_state);
	ky_dpu_run(crtc, old_state);
}

static struct drm_crtc_state *ky_crtc_duplicate_state(struct drm_crtc *crtc)
{
	struct ky_crtc_state *state, *old_state;
	struct ky_drm_private *priv = crtc->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u8 n_rdma, i;

	if (WARN_ON(!crtc->state))
		return NULL;

	old_state = to_ky_crtc_state(crtc->state);
	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;

	__drm_atomic_helper_crtc_duplicate_state(crtc, &state->base);
	memset(&state->scalers, 0x0, sizeof(struct ky_dpu_scaler) * MAX_SCALER_NUMS);

	n_rdma = hwdev->rdma_nums;
	state->rdmas = kzalloc(sizeof(struct ky_dpu_rdma) * n_rdma, GFP_KERNEL);
	if (!state->rdmas) {
		kfree(state);
		return NULL;
	}
	/* Rdma use UP_DOWN mode by default */
	for (i = 0; i < n_rdma; i++) {
		state->rdmas[i].mode = UP_DOWN;
		state->rdmas[i].in_use = false;
	}

	if (state->color_matrix_blob_prop)
		drm_property_blob_get(state->color_matrix_blob_prop);

	return &state->base;
}

static void ky_crtc_destroy_state(struct drm_crtc *crtc,
				      struct drm_crtc_state *state)
{
	struct ky_crtc_state *ky_state = NULL;

	if (state) {
		ky_state = to_ky_crtc_state(state);
		__drm_atomic_helper_crtc_destroy_state(state);

		if (ky_state->color_matrix_blob_prop)
			drm_property_blob_put(ky_state->color_matrix_blob_prop);

		kfree(ky_state->rdmas);
		kfree(ky_state);
	}
}

static void ky_crtc_reset(struct drm_crtc *crtc)
{
	struct ky_crtc_state *state =
		kzalloc(sizeof(*state), GFP_KERNEL);
	struct ky_drm_private *priv = crtc->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u8 n_rdma;

	if (crtc->state)
		ky_crtc_destroy_state(crtc, crtc->state);

	__drm_atomic_helper_crtc_reset(crtc, &state->base);

	n_rdma = hwdev->rdma_nums;
	state->rdmas = kzalloc(sizeof(struct ky_dpu_rdma) * n_rdma, GFP_KERNEL);
	if (!state->rdmas) {
		DRM_ERROR("Failed to allocate memory of struct ky_dpu_rdma!\n");
		return;
	}

}

static int ky_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_crtc_enable_vblank(dpu->dev_id);

	if (dpu->core && dpu->core->enable_vsync)
		dpu->core->enable_vsync(dpu);

	return 0;
}

static void ky_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_crtc_disable_vblank(dpu->dev_id);

	if (dpu->core && dpu->core->disable_vsync)
		dpu->core->disable_vsync(dpu);
}

static int ky_crtc_atomic_set_property(struct drm_crtc *crtc,
				   struct drm_crtc_state *state,
				   struct drm_property *property,
				   uint64_t val)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct ky_crtc_state *s = to_ky_crtc_state(state);
	bool replaced = false;
	int ret = 0;

	DRM_DEBUG("%s() name = %s, val = %llu\n",
		  __func__, property->name, val);

	if (property == dpu->color_matrix_property){
		ret = ky_atomic_replace_property_blob_from_id(crtc->dev,
					&s->color_matrix_blob_prop,
					val,
					-1,
					sizeof(int),
					&replaced);
		return ret;
	} else {
		DRM_ERROR("property %s is invalid\n", property->name);
		return -EINVAL;
	}

	return 0;
}

static int ky_crtc_atomic_get_property(struct drm_crtc *crtc,
					  const struct drm_crtc_state *state,
					  struct drm_property *property,
					  u64 *val)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);
	struct ky_crtc_state *s = to_ky_crtc_state(state);

	DRM_DEBUG("%s() name = %s\n", __func__, property->name);

	if (property == dpu->color_matrix_property){
		if (s->color_matrix_blob_prop)
			*val = (s->color_matrix_blob_prop) ? s->color_matrix_blob_prop->base.id : 0;
	}
	else {
		DRM_ERROR("property %s is invalid\n", property->name);
		return -EINVAL;
	}

	return 0;
}

static const struct drm_crtc_helper_funcs ky_crtc_helper_funcs = {
	.mode_set_nofb = ky_crtc_mode_set_nofb,
	.atomic_check = ky_crtc_atomic_check,
	.atomic_begin = ky_crtc_atomic_begin,
	.atomic_flush = ky_crtc_atomic_flush,
	.atomic_enable = ky_crtc_atomic_enable,
	.atomic_disable = ky_crtc_atomic_disable,
};

static const struct drm_crtc_funcs ky_crtc_funcs = {
	.atomic_get_property = ky_crtc_atomic_get_property,
	.atomic_set_property = ky_crtc_atomic_set_property,
	.destroy = drm_crtc_cleanup,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.reset = ky_crtc_reset,
	.atomic_duplicate_state = ky_crtc_duplicate_state,
	.atomic_destroy_state = ky_crtc_destroy_state,
	.enable_vblank = ky_crtc_enable_vblank,
	.disable_vblank = ky_crtc_disable_vblank,
};

static int ky_crtc_create_properties(struct drm_crtc *crtc)
{
	struct drm_property *prop;
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	DRM_DEBUG("%s()\n", __func__);
	/* create rotation property */

	prop = drm_property_create(crtc->dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
			"color_matrix_coef", 0);
	if (!prop)
		return -ENOMEM;
	drm_object_attach_property(&crtc->base, prop, 0);
	dpu->color_matrix_property = prop;

	return 0;
}

static int ky_crtc_init(struct drm_device *drm, struct drm_crtc *crtc,
			 struct drm_plane *primary, struct device_node *port)
{
	int err;

	/*
	 * set crtc port so that drm_of_find_possible_crtcs call works
	 */
	of_node_put(port);
	crtc->port = port;

	err = drm_crtc_init_with_planes(drm, crtc, primary, NULL,
					&ky_crtc_funcs, NULL);
	if (err) {
		DRM_ERROR("failed to init crtc.\n");
		return err;
	}

	drm_mode_crtc_set_gamma_size(crtc, 256);

	drm_crtc_helper_add(crtc, &ky_crtc_helper_funcs);

	ky_crtc_create_properties(crtc);

	DRM_DEBUG("%s() ok\n", __func__);
	return 0;
}

int ky_dpu_wb_config(struct ky_dpu *dpu)
{
	if (dpu->core && dpu->core->wb_config)
		dpu->core->wb_config(dpu);

	return 0;
}

int ky_dpu_run(struct drm_crtc *crtc,
		struct drm_crtc_state *old_state)
{
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	DRM_DEBUG("%s() type %d \n", __func__, dpu->type);
	trace_ky_dpu_run(dpu->dev_id);

	if (dpu->core && dpu->core->run)
		dpu->core->run(crtc, old_state);

	return 0;
}

int ky_dpu_stop(struct ky_dpu *dpu)
{
	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);
	trace_ky_dpu_stop(dpu->dev_id);

	if (dpu->core && dpu->core->stop)
		dpu->core->stop(dpu);

	drm_crtc_handle_vblank(&dpu->crtc);

	return 0;
}

static int ky_dpu_init(struct ky_dpu *dpu)
{
	trace_ky_dpu_init(dpu->dev_id);

	if (dpu->core && dpu->core->init)
		dpu->core->init(dpu);

	dpu->is_1st_f = true;

	return 0;
}

static int ky_dpu_uninit(struct ky_dpu *dpu)
{
	trace_ky_dpu_uninit(dpu->dev_id);

	if (dpu->core && dpu->core->uninit)
		dpu->core->uninit(dpu);

	return 0;
}

static irqreturn_t ky_dpu_isr(int irq, void *data)
{
	struct ky_dpu *dpu = data;
	u32 int_mask = 0;

	if (dpu->core && dpu->core->isr)
		int_mask = dpu->core->isr(dpu);

	if (int_mask & DPU_INT_UNDERRUN)
		DRM_DEBUG("Warning: dpu underrun!\n");

	return IRQ_HANDLED;
}

static int ky_dpu_irqs_init(struct ky_dpu *dpu,
				struct device_node *np, struct platform_device *pdev)
{
	int err;
	int irq;

	/*request irq*/
	irq = platform_get_irq_byname(pdev, "ONLINE_IRQ");
	if (irq < 0) {
		DRM_ERROR("%s: failed to get ONLINE irq number\n", __func__);
		return -EINVAL;
	}
	DRM_DEBUG("dpu online_irq = %d\n", irq);
	err = request_irq(irq, ky_dpu_isr, 0, "DPU_ONLINE", dpu);
	if (err) {
		DRM_ERROR("error: dpu request online irq failed\n");
		return -EINVAL;
	}

	irq = platform_get_irq_byname(pdev, "OFFLINE_IRQ");
	if (irq < 0) {
		DRM_ERROR("%s: failed to get OFFLINE irq number\n", __func__);
		return -EINVAL;
	}
	DRM_DEBUG("dpu offline_irq = %d\n", irq);
	err = request_irq(irq, ky_dpu_isr, 0, "DPU_OFFLINE", dpu);
	if (err) {
		DRM_ERROR("error: dpu request offline irq failed\n");
		return -EINVAL;
	}

	return 0;
}

static void dpu_wq_update_bw(struct work_struct *work)
{
	struct ky_dpu *dpu =
		container_of(work, struct ky_dpu, work_update_bw);

	if (dpu->core && dpu->core->update_bw) {
		trace_u64_data("bw decrease to", dpu->new_bw);
		dpu->core->update_bw(dpu, dpu->new_bw);
	}
}

static void dpu_wq_update_clk(struct work_struct *work)
{
	struct ky_dpu *dpu =
		container_of(work, struct ky_dpu, work_update_clk);

	if (dpu->core && dpu->core->update_clk) {
		trace_u64_data("mclk decrease to", dpu->new_mclk);
		dpu->core->update_clk(dpu, dpu->new_mclk);
	}
}

#ifdef CONFIG_KY_DEBUG
static bool check_dpu_running_status(struct ky_dpu* dpu)
{
	return dpu->is_working;
}

#define to_dpuinfo(_nb) container_of(_nb, struct ky_dpu, nb)
static int dpu_clkoffdet_notifier_handler(struct notifier_block *nb,
		unsigned long msg, void *data)
{
	struct clk_notifier_data *cnd = data;
	struct ky_dpu *dpu = to_dpuinfo(nb);

	if ((__clk_is_enabled(cnd->clk)) && (msg & PRE_RATE_CHANGE) && (cnd->new_rate == 0) && (cnd->old_rate != 0)) {
		if (dpu->is_dpu_running(dpu))
		return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}
#endif

static int ky_dpu_bind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm_dev = data;
	struct ky_dpu *dpu = dev_get_drvdata(dev);
	struct device_node *np = dev->of_node;
	struct drm_plane *plane;
	int ret;
#ifdef CONFIG_KY_DEBUG
	struct dpu_clk_context *clk_ctx = NULL;
#endif
	DRM_INFO("%s()\n", __func__);

	ret = ky_dpu_irqs_init(dpu, np, pdev);
	if (ret)
		return ret;

	ret = dma_coerce_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		DRM_ERROR("dma_set_mask_and_coherent failed (%d)\n", ret);
		return ret;
	}

	ret = of_reserved_mem_device_init(dpu->dev);
	if (ret) {
		DRM_ERROR("Failed to reserve dpu memory, ret:%d\n", ret);
		return ret;
	}

	dpu->dpu_underrun_wq = create_singlethread_workqueue("dpu_underrun_wq");
	if (!dpu->dpu_underrun_wq) {
		DRM_ERROR("%s: failed to create wq.\n", __func__);
		ret = -ESRCH;
		goto alloc_fail;
	}

	INIT_WORK(&dpu->work_stop_trace, dpu_underrun_wq_stop_trace);
	INIT_WORK(&dpu->work_update_clk, dpu_wq_update_clk);
	INIT_WORK(&dpu->work_update_bw, dpu_wq_update_bw);

	plane = ky_plane_init(drm_dev, dpu);
	if (IS_ERR_OR_NULL(plane)) {
		ret = PTR_ERR(plane);
		goto err_destroy_workqueue;
	}

	ret = ky_crtc_init(drm_dev, &dpu->crtc, plane, np);
	if (ret)
		goto err_destroy_workqueue;

	// ky_wb_init(drm_dev, &dpu->crtc);

	ky_dpu_sysfs_init(dev);

#ifdef CONFIG_KY_DEBUG
	clk_ctx = &dpu->clk_ctx;
	dpu->is_dpu_running = check_dpu_running_status;
	dpu->nb.notifier_call = dpu_clkoffdet_notifier_handler;
	clk_notifier_register(clk_ctx->mclk, &dpu->nb);
#endif
	DRM_DEBUG("dpu driver probe success\n");

	return 0;

err_destroy_workqueue:
	destroy_workqueue(dpu->dpu_underrun_wq);
alloc_fail:
	of_reserved_mem_device_release(dpu->dev);
	return ret;
}

static void ky_dpu_unbind(struct device *dev, struct device *master,
	void *data)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);
#ifdef CONFIG_KY_DEBUG
	struct dpu_clk_context *clk_ctx = &dpu->clk_ctx;
#endif

	DRM_DEBUG("%s()\n", __func__);

	pm_runtime_disable(dev);
	of_reserved_mem_device_release(dpu->dev);
	drm_crtc_cleanup(&dpu->crtc);

#ifdef CONFIG_KY_DEBUG
	clk_notifier_unregister(clk_ctx->mclk, &dpu->nb);
#endif
}

static const struct component_ops dpu_component_ops = {
	.bind = ky_dpu_bind,
	.unbind = ky_dpu_unbind,
};

static int ky_dpu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ky_dpu *dpu;
	struct device_node *np = dev->of_node;
	struct device_node *rmem_np;
	struct device_node *fb_np;
	struct reserved_mem rmem;
	struct resource rsrv_mem;
	int ret;

	const char *str;
	u32 dpu_id;
	u32 dpu_type;
	static int dpu_num = 0;

	DRM_DEBUG("%s()\n", __func__);

	if (!dev->of_node) {
		DRM_DEV_ERROR(dev, "can't find dpu devices\n");
		return -ENODEV;
	}

	dpu = devm_kzalloc(dev, sizeof(*dpu), GFP_KERNEL);
	if (!dpu)
		return -ENOMEM;
	dpu->dev = dev;
	dpu->logo_booton = true;
	dev_set_drvdata(dev, dpu);

	if (of_property_read_u32(np, "pipeline-id", &dpu_id))
		return -EINVAL;
	dpu->dev_id = dpu_id;

	if (of_property_read_u32(np, "type", &dpu_type))
		return -EINVAL;
	dpu->type = dpu_type;

	if (!of_property_read_string(np, "ip", &str)) {
		dpu->core = dpu_core_ops_attach(str);
	} else
		DRM_WARN("ip was not found\n");

	/* Clk dts nodes must be parsed in head of pm_runtime_xxx */
	if (dpu->core && dpu->core->parse_dt)
		dpu->core->parse_dt(dpu, np);

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	dpu->enable_gpio = devm_gpiod_get_optional(dev, "enable",
						GPIOD_IN);
	if (!IS_ERR_OR_NULL(dpu->enable_gpio)) {
		gpiod_direction_output(dpu->enable_gpio, 0);
		msleep(50);
	} else {
		DRM_DEV_DEBUG(dev, "not found enable gpio\n");
	}

	if (dpu->type == DSI) {
		dpu->dsi_reset = devm_reset_control_get_optional_shared(&pdev->dev, "dsi_reset");
		if (IS_ERR_OR_NULL(dpu->dsi_reset)) {
			DRM_DEV_DEBUG(dev, "not found dsi_reset\n");
		}
		dpu->mclk_reset = devm_reset_control_get_optional_shared(&pdev->dev, "mclk_reset");
		if (IS_ERR_OR_NULL(dpu->mclk_reset)) {
			DRM_DEV_DEBUG(dev, "not found mclk_reset\n");
		}
		dpu->esc_reset = devm_reset_control_get_optional_shared(&pdev->dev, "esc_reset");
		if (IS_ERR_OR_NULL(dpu->esc_reset)) {
			DRM_DEV_DEBUG(dev, "not found esc_reset\n");
		}
		dpu->lcd_reset = devm_reset_control_get_optional_shared(&pdev->dev, "lcd_reset");
		if (IS_ERR_OR_NULL(dpu->lcd_reset)) {
			DRM_DEV_DEBUG(dev, "not found lcd_reset\n");
		}
	} else if (dpu->type == HDMI) {
		dpu->hdmi_reset = devm_reset_control_get_optional_shared(&pdev->dev, "hdmi_reset");
		if (IS_ERR_OR_NULL(dpu->hdmi_reset)) {
			DRM_DEV_DEBUG(dev, "not found hdmi_reset\n");
		}
	} else {
		DRM_DEV_ERROR(dev, "can't find dpu type %d\n", dpu->type);
		return -ENODEV;
	}

	dpu_num++;

	/*
	 * To keep bootloader logo on, below operations must be
	 * done in probe func as power domain framework will turn
	 * on/off lcd power domain before/after probe func.
	 */
	if (dpu->logo_booton && dpu->type == HDMI) {
		pm_runtime_enable(&pdev->dev);

		pm_runtime_get_sync(&pdev->dev);
		dpu_pm_resume(&pdev->dev);
		dpu_pm_suspend(&pdev->dev);
		pm_runtime_put_sync(&pdev->dev);
		dpu->logo_booton = false;
		msleep(10);
	}

	rmem_np = of_find_node_by_name(NULL, "reserved-memory");
	if (rmem_np && (dpu_num >= 2)) {
		fb_np = of_find_node_by_name(rmem_np, "framebuffer");
		if (fb_np) {
			ret = of_address_to_resource(fb_np, 0, &rsrv_mem);
			if (ret < 0) {
				DRM_DEV_ERROR(dev, "no reserved memory resource find in reserved framebuffer node\n");
			} else {
				rmem.base = rsrv_mem.start;
				rmem.size = resource_size(&rsrv_mem);

				ky_dpu_free_bootloader_mem(&rmem);
			}
		}
	}

	return component_add(dev, &dpu_component_ops);
}

static int ky_dpu_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &dpu_component_ops);
	return 0;
}

static int dpu_pm_suspend(struct device *dev)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);
	int result;

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	if (dpu->core && dpu->core->disable_clk)
		dpu->core->disable_clk(dpu);

	if (dpu->type == HDMI) {
		if (!IS_ERR_OR_NULL(dpu->hdmi_reset)) {
			result = reset_control_assert(dpu->hdmi_reset);
			if (result < 0) {
				DRM_INFO("Failed to assert hdmi_reset: %d\n", result);
			}
		}
	} else if (dpu->type == DSI) {

		if (!IS_ERR_OR_NULL(dpu->lcd_reset)) {
			result = reset_control_assert(dpu->lcd_reset);
			if (result < 0) {
				DRM_INFO("Failed to assert lcd_reset: %d\n", result);
			}
		}
		if (!IS_ERR_OR_NULL(dpu->esc_reset)) {
			result = reset_control_assert(dpu->esc_reset);
			if (result < 0) {
				DRM_INFO("Failed to assert esc_reset: %d\n", result);
			}
		}
		if (!IS_ERR_OR_NULL(dpu->mclk_reset)) {
			result = reset_control_assert(dpu->mclk_reset);
			if (result < 0) {
				DRM_INFO("Failed to assert mclk_reset: %d\n", result);
			}
		}
		if (!IS_ERR_OR_NULL(dpu->dsi_reset)) {
			result = reset_control_assert(dpu->dsi_reset);
			if (result < 0) {
				DRM_INFO("Failed to assert dsi_reset: %d\n", result);
			}
		}
	}

	return 0;
}

static int dpu_pm_resume(struct device *dev)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);
	int result;

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	if (dpu->type == HDMI) {
		if (!IS_ERR_OR_NULL(dpu->hdmi_reset)) {
			result = reset_control_deassert(dpu->hdmi_reset);
			if (result < 0) {
				DRM_INFO("Failed to deassert hdmi_reset: %d\n", result);
			}
		}
	} else if (dpu->type == DSI){
		if (!IS_ERR_OR_NULL(dpu->dsi_reset)) {
			result = reset_control_deassert(dpu->dsi_reset);
			if (result < 0) {
				DRM_INFO("Failed to deassert dsi_reset: %d\n", result);
			}
		}
		if (!IS_ERR_OR_NULL(dpu->mclk_reset)) {
			result = reset_control_deassert(dpu->mclk_reset);
			if (result < 0) {
				DRM_INFO("Failed to deassert mclk_reset: %d\n", result);
			}
		}
		if (!IS_ERR_OR_NULL(dpu->esc_reset)) {
			result = reset_control_deassert(dpu->esc_reset);
			if (result < 0) {
				DRM_INFO("Failed to deassert esc_reset: %d\n", result);
			}
		}
		if (!IS_ERR_OR_NULL(dpu->lcd_reset)) {
			result = reset_control_deassert(dpu->lcd_reset);
			if (result < 0) {
				DRM_INFO("Failed to deassert lcd_reset: %d\n", result);
			}
		}
	}

	if (dpu->core && dpu->core->enable_clk)
		dpu->core->enable_clk(dpu);

	return 0;
}

static int dpu_rt_pm_suspend(struct device *dev)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	return 0;
}

static int dpu_rt_pm_resume(struct device *dev)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);

	DRM_DEBUG("%s() type %d\n", __func__, dpu->type);

	return 0;
}

static const struct dev_pm_ops dpu_pm_ops = {
	SET_RUNTIME_PM_OPS(dpu_rt_pm_suspend,
			dpu_rt_pm_resume,
			NULL)
};

static const struct of_device_id dpu_match_table[] = {
	{ .compatible = "ky,dpu-online0" },
	{ .compatible = "ky,dpu-online1" },
	{ .compatible = "ky,dpu-online2" },
	{ .compatible = "ky,dpu-offline0" },
	{ .compatible = "ky,dpu-offline1" },
	{},
};
MODULE_DEVICE_TABLE(of, dpu_match_table);

struct platform_driver ky_dpu_driver = {
	.probe = ky_dpu_probe,
	.remove = ky_dpu_remove,
	.driver = {
		.name = "ky-dpu-drv",
		.of_match_table = dpu_match_table,
		.pm = &dpu_pm_ops,
	},
};

MODULE_DESCRIPTION("Ky Display Controller Driver");
MODULE_LICENSE("GPL v2");

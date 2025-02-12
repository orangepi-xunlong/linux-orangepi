// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
//#include <drm/drm_gem_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_writeback.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_of.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include <linux/of_graph.h>
#include "ky_lib.h"
#include "ky_dpu.h"
#include "ky_wb.h"
#include "sysfs/sysfs_display.h"

static const u32 ky_wb_formats[] = {
	DRM_FORMAT_XRGB8888,
};

void ky_wb_atomic_commit(struct drm_device *drm, struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct ky_dpu *dpu;
	struct drm_crtc_state *new_crtc_state;
	struct drm_writeback_connector *wb_conn;
	struct drm_connector_state *conn_state;
	int i;

	for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i) {
		if (!new_crtc_state->active)
			continue;

		dpu = crtc_to_dpu(crtc);
		wb_conn = &dpu->wb_connector;
		conn_state = wb_conn->base.state;

		if (!conn_state)
			return;

		if (conn_state->writeback_job)
			drm_writeback_queue_job(wb_conn, conn_state);
	}
}

static int ky_wb_encoder_atomic_check(struct drm_encoder *encoder,
				    struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *conn_state)
{
	DRM_DEBUG("%s()\n", __func__);

	return 0;
}

static const struct drm_encoder_helper_funcs ky_wb_encoder_helper_funcs = {
	.atomic_check = ky_wb_encoder_atomic_check,
};

static const struct drm_encoder_funcs ky_wb_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int ky_wb_connector_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;

	DRM_DEBUG("%s()\n", __func__);

	return drm_add_modes_noedid(connector, dev->mode_config.max_width,
				    dev->mode_config.max_height);
}

static enum drm_mode_status
ky_wb_connector_mode_valid(struct drm_connector *connector,
			 struct drm_display_mode *mode)
{
	enum drm_mode_status mode_status = MODE_OK;

	DRM_DEBUG("%s(%s)\n", __func__, mode->name);

	return mode_status;
}

static enum drm_connector_status
ky_wb_connector_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static void ky_wb_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_helper_funcs ky_wb_connector_helper_funcs = {
	.get_modes = ky_wb_connector_get_modes,
	.mode_valid = ky_wb_connector_mode_valid,
};

void ky_wb_drm_atomic_helper_connector_destroy_state(struct drm_connector *connector,
					  struct drm_connector_state *state)
{
	struct drm_crtc *crtc = NULL;
	struct ky_dpu *dpu = NULL;

	crtc = state->crtc;
	dpu = crtc_to_dpu(crtc);

	if (dpu->mmu_tbl.va)
		dma_free_coherent(dpu->dev, dpu->mmu_tbl.size, \
			dpu->mmu_tbl.va, dpu->mmu_tbl.pa);
}

static const struct drm_connector_funcs ky_wb_connector_funcs = {
	.reset = drm_atomic_helper_connector_reset,
	.detect = ky_wb_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = ky_wb_connector_destroy,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = ky_wb_drm_atomic_helper_connector_destroy_state,
};

static int ky_wb_connector_init(struct drm_device *drm, struct drm_crtc *crtc)
{
	int ret;
	struct ky_dpu *dpu = crtc_to_dpu(crtc);

	dpu->wb_connector.encoder.possible_crtcs = 1 << drm_crtc_index(crtc);

	drm_connector_helper_add(&dpu->wb_connector.base,
				 &ky_wb_connector_helper_funcs);

	ret = drm_writeback_connector_init(drm, &dpu->wb_connector,
					   &ky_wb_connector_funcs,
					   &ky_wb_encoder_helper_funcs,
					   ky_wb_formats,
					   ARRAY_SIZE(ky_wb_formats),
					   1 << drm_crtc_index(crtc));
	if (ret) {
		DRM_ERROR("drm_connector_init() failed\n");
		return ret;
	}

	return 0;
}

int ky_wb_init(struct drm_device *drm, struct drm_crtc *dpu_crtc)
{
	struct drm_crtc *crtc = dpu_crtc;
	int ret;

	ret = ky_wb_connector_init(drm, crtc);
	if (ret)
		return -1;

	return ret;
}

static int ky_wb_device_create(struct ky_wb *wb, struct device *parent)
{
	int ret;

	wb->dev.class = display_class;
	wb->dev.parent = parent;
	wb->dev.of_node = parent->of_node;
	dev_set_name(&wb->dev, "wb%d", wb->ctx.id);
	dev_set_drvdata(&wb->dev, wb);

	ret = device_register(&wb->dev);
	if (ret)
		DRM_ERROR("wb device register failed\n");

	return ret;
}

static int ky_wb_context_init(struct ky_wb *wb, struct device_node *np)
{
	struct ky_wb_device *ctx = &wb->ctx;
	u32 tmp;

	if (!of_property_read_u32(np, "dev-id", &tmp))
		ctx->id = tmp;

	return 0;
}

static int ky_wb_bind(struct device *dev, struct device *master, void *data)
{

	return 0;
}

static void ky_wb_unbind(struct device *dev,
				struct device *master, void *data)
{
	/* do nothing */
	DRM_DEBUG("%s()\n", __func__);
}

static const struct component_ops ky_wb_component_ops = {
	.bind = ky_wb_bind,
	.unbind = ky_wb_unbind,
};

static int ky_wb_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ky_wb *wb;
	int ret;

	DRM_DEBUG("%s()\n", __func__);

	wb = devm_kzalloc(&pdev->dev, sizeof(*wb), GFP_KERNEL);
	if (!wb) {
		DRM_ERROR("failed to allocate wb data.\n");
		return -ENOMEM;
	}

	ret = ky_wb_context_init(wb, np);
	if (ret) {
		return -EINVAL;
	}

	ky_wb_device_create(wb, &pdev->dev);
	//ky_wb_sysfs_init(&wb->dev);
	platform_set_drvdata(pdev, wb);

	return component_add(&pdev->dev, &ky_wb_component_ops);
}

static int ky_wb_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &ky_wb_component_ops);
	return 0;
}

static const struct of_device_id ky_wb_of_match[] = {
	{.compatible = "ky,wb0"},
	{.compatible = "ky,wb1"},
	{ }
};
MODULE_DEVICE_TABLE(of, ky_wb_of_match);

struct platform_driver ky_wb_driver = {
	.probe = ky_wb_probe,
	.remove = ky_wb_remove,
	.driver = {
		.name = "ky-wb-drv",
		.of_match_table = ky_wb_of_match,
	},
};

// module_platform_driver(ky_wb_driver);
static int ky_wb_driver_init(void)
{
       return platform_driver_register(&ky_wb_driver);
}
late_initcall(ky_wb_driver_init);

MODULE_DESCRIPTION("Ky WB Driver");
MODULE_LICENSE("GPL v2");


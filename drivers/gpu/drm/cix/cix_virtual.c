// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/component.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/fwnode.h>
#include <linux/acpi.h>

#include <drm/drm_of.h>
#include <drm/drm_encoder.h>
#include <drm/drm_edid.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_state_helper.h>
#include "linlon-dp/linlondp_dev.h"

#define XRES_MAX	4096
#define YRES_MAX	4096

#define XRES_DEF	640
#define YRES_DEF	480

struct cix_virtual_dev {
	struct device *dev;
	struct drm_encoder encoder;
	struct drm_connector connector;
};

static const struct drm_display_mode cix_drm_dmt_modes[] = {
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

static int
cix_virtual_encoder_atomic_check(struct drm_encoder *encoder,
				 struct drm_crtc_state *crtc_state,
				 struct drm_connector_state *conn_state)
{
	/* TODO */

	return 0;
}

static const struct drm_encoder_helper_funcs
cix_virtual_encoder_helper_funcs = {
	.atomic_check	= cix_virtual_encoder_atomic_check,
};

static int cix_virtual_add_modes_noedid(struct drm_connector *connector,
			int hdisplay, int vdisplay)
{
	int i, count, num_modes = 0;
	struct drm_display_mode *mode;
	struct drm_device *dev = connector->dev;

	count = ARRAY_SIZE(cix_drm_dmt_modes);

	for (i = 0; i < count; i++) {
		const struct drm_display_mode *ptr = &cix_drm_dmt_modes[i];

		if (hdisplay && vdisplay) {
			if (ptr->hdisplay > hdisplay ||
					ptr->vdisplay > vdisplay)
				continue;
		}

		mode = drm_mode_duplicate(dev, ptr);
		if (mode) {
			drm_mode_probed_add(connector, mode);
			num_modes++;
		}
	}
	return num_modes;
}

static int
cix_virtual_connector_get_modes(struct drm_connector *connector)
{
	int count;

	count = cix_virtual_add_modes_noedid(connector, XRES_MAX, YRES_MAX);
	drm_set_preferred_mode(connector, XRES_DEF, YRES_DEF);

	return count;
}

static const struct drm_connector_helper_funcs
cix_virtual_connector_helper_funcs = {
	.get_modes = cix_virtual_connector_get_modes,
};

static const struct drm_connector_funcs
cix_virtual_connector_funcs = {
	.fill_modes	= drm_helper_probe_single_connector_modes,
	.destroy	= drm_connector_cleanup,
	.reset		= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static uint32_t drm_acpi_crtc_port_mask(struct drm_device *dev,
			    struct fwnode_handle *port)
{
	unsigned int index = 0;
	struct drm_crtc *tmp;

	drm_for_each_crtc(tmp, dev) {
		if ((struct fwnode_handle *)tmp->port == port) {
			pr_info("cix_virtual.drm_acpi_crtc_port_mask, port=%s\n",
				port->ops->get_name(port));
			return 1 << index;
		}

		index++;
	}

	return 0;
}

static uint32_t drm_acpi_find_possible_crtcs(struct drm_device *dev,
				    struct fwnode_handle *port)
{
	struct fwnode_handle *remote_port, *ep;
	uint32_t possible_crtcs = 0;

	fwnode_graph_for_each_endpoint(port, ep) {
		remote_port = fwnode_graph_get_remote_port(ep);
		if (!remote_port) {
			fwnode_handle_put(ep);
			return 0;
		}

		possible_crtcs |= drm_acpi_crtc_port_mask(dev, remote_port);

		fwnode_handle_put(remote_port);
	}

	return possible_crtcs;
}

static int
cix_virtual_bind(struct device *comp, struct device *master,
		 void *master_data)
{
	int ret;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct drm_device *drm = master_data;
	void *np;

	struct cix_virtual_dev *vd = dev_get_drvdata(comp);

	/* TODO: parse dt */

	/* Init virtual encoder */
	encoder = &vd->encoder;

	drm_encoder_helper_add(encoder, &cix_virtual_encoder_helper_funcs);

	ret = drm_simple_encoder_init(drm, encoder, DRM_MODE_ENCODER_VIRTUAL);
	if (ret) {
		dev_err(vd->dev, "failed to init simple encoder : %d\n", ret);
		return ret;
	}

	if (has_acpi_companion(comp)) {
		pr_info("dwx: cix_virtual start to call cix_virtual_bind via ACPI.\n");
		np = comp->fwnode;
		encoder->possible_crtcs = drm_acpi_find_possible_crtcs(drm, (struct fwnode_handle *)np);
	} else {
		pr_info("dwx: cix_virtual start to call cix_virtual_bind via DT.\n");
		np = comp->of_node;
		encoder->possible_crtcs = drm_of_find_possible_crtcs(drm, (struct device_node *)np);
	}
	
	if (encoder->possible_crtcs == 0)
		return -EPROBE_DEFER;

	/* Init virtual connector */
	connector = &vd->connector;

	drm_connector_helper_add(connector, &cix_virtual_connector_helper_funcs);

	ret = drm_connector_init(drm, connector, &cix_virtual_connector_funcs,
				 DRM_MODE_CONNECTOR_VIRTUAL);
	if (ret) {
		dev_err(vd->dev, "failed to init connector: %d\n", ret);
		return ret;
	}

	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		dev_err(vd->dev, "Failed to attach connector to encoder\n");
		return ret;
	}

	/*  Set supported color formats */
	connector->display_info.color_formats = DRM_COLOR_FORMAT_RGB444   |
						DRM_COLOR_FORMAT_YCBCR422 |
						DRM_COLOR_FORMAT_YCBCR420;

	return 0;
}

static void
cix_virtual_unbind(struct device *comp, struct device *master,
		   void *master_data)
{
	struct cix_virtual_dev *vd = dev_get_drvdata(comp);

	drm_connector_cleanup(&vd->connector);
	drm_encoder_cleanup(&vd->encoder);
}

const struct component_ops cix_virtual_ops = {
	.bind	= cix_virtual_bind,
	.unbind	= cix_virtual_unbind,
};

static const struct of_device_id cix_virtual_dt_ids[] = {
	{ .compatible = "cix,virtual-display", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, cix_virtual_dt_ids);

static const struct acpi_device_id cix_virtual_acpi_ids[] = {
	{ .id = "CIXH503F", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, cix_virtual_acpi_ids);

static int cix_virtual_probe(struct platform_device *pdev)
{
	struct cix_virtual_dev *vd;
	struct device *dev = &pdev->dev;
	pr_info("dwx: cix_virtual_probe enter.\n");
	vd = devm_kzalloc(dev, sizeof(*vd), GFP_KERNEL);
	if (!vd)
		return -ENOMEM;

	vd->dev = dev;

	dev_set_drvdata(dev, vd);

	return component_add(dev, &cix_virtual_ops);
}

static int cix_virtual_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	component_del(dev, &cix_virtual_ops);

	dev_set_drvdata(dev, NULL);

	return 0;
}

struct platform_driver cix_virtual_driver = {
	.probe	= cix_virtual_probe,
	.remove	= cix_virtual_remove,
	.driver	= {
		.name = "cix-virtual-display",
		.of_match_table = of_match_ptr(cix_virtual_dt_ids),
		.acpi_match_table = ACPI_PTR(cix_virtual_acpi_ids),
	},
};

module_platform_driver(cix_virtual_driver);

MODULE_AUTHOR("Fancy Fang <fancy.fang@cixtech.com>");
MODULE_DESCRIPTION("Cix DRM Virtual Display Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cix-virtual");

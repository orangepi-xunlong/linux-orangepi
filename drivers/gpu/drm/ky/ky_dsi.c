// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_of.h>
#include <linux/component.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_graph.h>
#include <linux/pm_runtime.h>
#include <video/mipi_display.h>
#include "ky_lib.h"
#include "ky_dpu.h"
#include "ky_dsi.h"
#include "sysfs/sysfs_display.h"

LIST_HEAD(dsi_core_head);

static void ky_dsi_encoder_enable(struct drm_encoder *encoder)
{
	struct ky_dsi *dsi = encoder_to_dsi(encoder);
	//struct ky_dpu *dpu = crtc_to_dpu(encoder->crtc);
	void __iomem *addr = (void __iomem *)ioremap(0xd421a1a8, 100);

	DRM_INFO("%s()\n", __func__);

	if (!dsi->core || !dsi->core->dsi_open) {
		DRM_ERROR("%s(), dsi->core is null!\n", __func__);
		return;
	}

	/* Dsi online setup */
	writel(0x40000001, addr);
	writel(0x1, addr + 0x10);

	dsi->core->dsi_open(&dsi->ctx, false);

	if (dsi->panel) {
		drm_panel_prepare(dsi->panel);
		drm_panel_enable(dsi->panel);
	}

	if (dsi->core && dsi->core->dsi_ready_for_datatx)
		dsi->core->dsi_ready_for_datatx(&dsi->ctx);
}

static void ky_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct ky_dsi *dsi = encoder_to_dsi(encoder);
	struct ky_dpu *dpu = crtc_to_dpu(encoder->crtc);

	DRM_INFO("%s()\n", __func__);

	ky_dpu_stop(dpu);

	if (dsi->core && dsi->core->dsi_close_datatx)
		dsi->core->dsi_close_datatx(&dsi->ctx);

	if (dsi->panel) {
		drm_panel_disable(dsi->panel);
		drm_panel_unprepare(dsi->panel);
	}

	if (dsi->core && dsi->core->dsi_close)
		dsi->core->dsi_close(&dsi->ctx);
}

static void ky_dsi_encoder_mode_set(struct drm_encoder *encoder,
				 struct drm_display_mode *mode,
				 struct drm_display_mode *adj_mode)
{
	struct ky_dsi *dsi = encoder_to_dsi(encoder);

	DRM_DEBUG("%s()\n", __func__);

	drm_display_mode_to_videomode(mode, &dsi->ctx.vm);
}

static int ky_dsi_encoder_atomic_check(struct drm_encoder *encoder,
				    struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *conn_state)
{
	DRM_DEBUG("%s()\n", __func__);

	return 0;
}

static const struct drm_encoder_helper_funcs ky_encoder_helper_funcs = {
	.atomic_check = ky_dsi_encoder_atomic_check,
	.mode_set = ky_dsi_encoder_mode_set,
	.enable = ky_dsi_encoder_enable,
	.disable = ky_dsi_encoder_disable,
};

static const struct drm_encoder_funcs ky_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int ky_dsi_encoder_init(struct drm_device *drm, struct ky_dsi *dsi)
{
	struct drm_encoder *encoder = &dsi->encoder;
	struct device *dev = dsi->host.dev;
	u32 crtc_mask;
	int ret;

	DRM_DEBUG("%s()\n", __func__);

	crtc_mask = drm_of_find_possible_crtcs(drm, dev->of_node);
	if (!crtc_mask) {
		DRM_ERROR("failed to find crtc mask\n");
		return -EINVAL;
	}
	DRM_INFO("find possible crtcs: 0x%08x\n", crtc_mask);

	encoder->possible_crtcs = crtc_mask;
	ret = drm_encoder_init(drm, encoder, &ky_encoder_funcs,
			       DRM_MODE_ENCODER_DSI, NULL);
	if (ret) {
		DRM_ERROR("failed to init dsi encoder\n");
		return ret;
	}

	drm_encoder_helper_add(encoder, &ky_encoder_helper_funcs);

	return 0;
}

static int ky_dsi_find_panel(struct ky_dsi *dsi)
{
	struct device *dev = dsi->host.dev;
	struct device_node *child, *lcds_node;
	struct drm_panel *panel;

	/* search /lcds child node first */
	lcds_node = of_find_node_by_path("/lcds");
	for_each_child_of_node(lcds_node, child) {
		panel = of_drm_find_panel(child);
		if (!IS_ERR(panel)) {
			dsi->panel = panel;
			of_node_put(child);
			return 0;
		}
	}

	/*
	 * If /lcds child node search failed, we search
	 * the child of dsi host node.
	 */
	for_each_child_of_node(dev->of_node, child) {
		panel = of_drm_find_panel(child);
		if (panel) {
			dsi->panel = panel;
			of_node_put(child);
			return 0;
		}
	}

	DRM_ERROR("of_drm_find_panel() failed\n");
	return -ENODEV;
}

static int ky_dsi_phy_attach(struct ky_dsi *dsi)
{
	struct device *dev;

	dev = ky_disp_pipe_get_output(&dsi->dev);
	if (!dev)
		return -ENODEV;

	dsi->ctx.phy = dev_get_drvdata(dev);
	if (!dsi->ctx.phy) {
		DRM_ERROR("dsi attach phy failed\n");
		return -EINVAL;
	}

	return 0;
}

static void ky_dsi_get_mipi_info(struct device_node *lcd_node, struct ky_mipi_info *mipi_info)
{
	int ret;
	u32 val;

	ret = of_property_read_u32(lcd_node, "height", &val);
	if (ret)
		DRM_ERROR("of get mipi height failed\n");
	else
		mipi_info->height = val;

	ret = of_property_read_u32(lcd_node, "width", &val);
	if (ret)
		DRM_ERROR("of get mipi width failed\n");
	else
		mipi_info->width = val;

	ret = of_property_read_u32(lcd_node, "hfp", &val);
	if (ret)
		DRM_ERROR("of get mipi hfp failed\n");
	else
		mipi_info->hfp = val;

	ret = of_property_read_u32(lcd_node, "hbp", &val);
	if (ret)
		DRM_ERROR("of get mipi hbp failed\n");
	else
		mipi_info->hbp = val;

	ret = of_property_read_u32(lcd_node, "hsync", &val);
	if (ret)
		DRM_ERROR("of get mipi hsync failed\n");
	else
		mipi_info->hsync = val;

	ret = of_property_read_u32(lcd_node, "vfp", &val);
	if (ret)
		DRM_ERROR("of get mipi vfp failed\n");
	else
		mipi_info->vfp = val;

	ret = of_property_read_u32(lcd_node, "vbp", &val);
	if (ret)
		DRM_ERROR("of get mipi vbp failed\n");
	else
		mipi_info->vbp = val;

	ret = of_property_read_u32(lcd_node, "vsync", &val);
	if (ret)
		DRM_ERROR("of get mipi vsync failed\n");
	else
		mipi_info->vsync = val;

	ret = of_property_read_u32(lcd_node, "fps", &val);
	if (ret)
		DRM_ERROR("of get mipi fps failed\n");
	else
		mipi_info->fps = val;

	ret = of_property_read_u32(lcd_node, "work-mode", &val);
	if (ret)
		DRM_ERROR("of get mipi work-mode failed\n");
	else
		mipi_info->work_mode = val;

	ret = of_property_read_u32(lcd_node, "rgb-mode", &val);
	if (ret)
		DRM_ERROR("of get mipi rgb-mode failed\n");
	else
		mipi_info->rgb_mode = val;

	ret = of_property_read_u32(lcd_node, "lane-number", &val);
	if (ret)
		DRM_ERROR("of get mipi lane-number failed\n");
	else
		mipi_info->lane_number = val;

	ret = of_property_read_u32(lcd_node, "phy-bit-clock", &val);
	if (ret)
		mipi_info->phy_bit_clock = DPHY_BITCLK_DEFAULT;
	else
		mipi_info->phy_bit_clock = val;


	ret = of_property_read_u32(lcd_node, "phy-esc-clock", &val);
	if (ret)
		mipi_info->phy_esc_clock = DPHY_ESCCLK_DEFAULT;
	else
		mipi_info->phy_esc_clock = val;

	ret = of_property_read_u32(lcd_node, "split-enable", &val);
	if (ret)
		DRM_ERROR("of get mipi split-enable failed\n");
	else
		mipi_info->split_enable = val;

	ret = of_property_read_u32(lcd_node, "eotp-enable", &val);
	if (ret)
		DRM_ERROR("of get mipi eotp-enable failed\n");
	else
		mipi_info->eotp_enable = val;

	ret = of_property_read_u32(lcd_node, "burst-mode", &val);
	if (ret)
		DRM_ERROR("of get mipi burst-mode failed\n");
	else
		mipi_info->burst_mode = val;
}

static void ky_dsi_get_advanced_info(struct ky_dsi *dsi, struct ky_mipi_info *mipi_info)
{
	struct ky_dsi_device *ctx = &dsi->ctx;
	struct ky_dsi_advanced_setting *adv_setting = &ctx->adv_setting;
	struct device_node *np = dsi->dev.of_node;
	int ret;

	/*advanced_setting*/
	ret = of_property_read_u32(np, "lpm_frame_en", &adv_setting->lpm_frame_en);
	if(ret)
		adv_setting->lpm_frame_en = LPM_FRAME_EN_DEFAULT;

	ret = of_property_read_u32(np, "last_line_turn", &adv_setting->last_line_turn);
	if(ret)
		adv_setting->last_line_turn = LAST_LINE_TURN_DEFAULT;

	ret = of_property_read_u32(np, "hex_slot_en", &adv_setting->hex_slot_en);
	if(ret)
		adv_setting->hex_slot_en = HEX_SLOT_EN_DEFAULT;

	ret = of_property_read_u32(np, "hsa_pkt_en", &adv_setting->hsa_pkt_en);
	if(ret){
		if(mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE)
			adv_setting->hsa_pkt_en = HSA_PKT_EN_DEFAULT_SYNC_PULSE;
		else
			adv_setting->hsa_pkt_en = HSA_PKT_EN_DEFAULT_OTHER;
	}

	ret = of_property_read_u32(np, "hse_pkt_en", &adv_setting->hse_pkt_en);
	if(ret){
		if(mipi_info->burst_mode == DSI_BURST_MODE_NON_BURST_SYNC_PULSE)
			adv_setting->hse_pkt_en = HSE_PKT_EN_DEFAULT_SYNC_PULSE;
		else
			adv_setting->hse_pkt_en = HSE_PKT_EN_DEFAULT_OTHER;
	}

	ret = of_property_read_u32(np, "hbp_pkt_en", &adv_setting->hbp_pkt_en);
	if(ret)
		adv_setting->hbp_pkt_en = HBP_PKT_EN_DEFAULT;

	ret = of_property_read_u32(np, "hfp_pkt_en", &adv_setting->hfp_pkt_en);
	if(ret)
		adv_setting->hfp_pkt_en = HFP_PKT_EN_DEFAULT;

	ret = of_property_read_u32(np, "hex_pkt_en", &adv_setting->hex_pkt_en);
	if(ret)
		adv_setting->hex_pkt_en = HEX_PKT_EN_DEFAULT;

	ret = of_property_read_u32(np, "hlp_pkt_en", &adv_setting->hlp_pkt_en);
	if(ret)
		adv_setting->hlp_pkt_en = HLP_PKT_EN_DEFAULT;

	ret = of_property_read_u32(np, "auto_dly_dis", &adv_setting->auto_dly_dis);
	if(ret)
		adv_setting->auto_dly_dis = AUTO_DLY_DIS_DEFAULT;

	ret = of_property_read_u32(np, "timing_check_dis", &adv_setting->timing_check_dis);
	if(ret)
		adv_setting->timing_check_dis = TIMING_CHECK_DIS_DEFAULT;

	ret = of_property_read_u32(np, "hact_wc_en", &adv_setting->hact_wc_en);
	if(ret)
		adv_setting->hact_wc_en = HACT_WC_EN_DEFAULT;

	ret = of_property_read_u32(np, "auto_wc_dis", &adv_setting->auto_wc_dis);
	if(ret)
		adv_setting->auto_wc_dis = AUTO_WC_DIS_DEFAULT;

	ret = of_property_read_u32(np, "vsync_rst_en", &adv_setting->vsync_rst_en);
	if(ret)
		adv_setting->vsync_rst_en = VSYNC_RST_EN_DEFAULT;
}

static int ky_dsi_host_attach(struct mipi_dsi_host *host,
			   struct mipi_dsi_device *slave)
{
	struct ky_dsi *dsi = host_to_dsi(host);
	struct ky_dsi_device *ctx = &dsi->ctx;
	struct ky_mipi_info *mipi_info = &ctx->mipi_info;
	struct device_node *lcd_node;
	int ret;

	DRM_INFO("%s()\n", __func__);

	dsi->slave = slave;

	ret = ky_dsi_phy_attach(dsi);
	if (ret)
		return ret;

	ret = ky_dsi_find_panel(dsi);
	if (ret)
		return ret;

	lcd_node = dsi->panel->dev->of_node;

	ky_dsi_get_mipi_info(lcd_node, mipi_info);
	ky_dsi_get_advanced_info(dsi, mipi_info);

	return 0;
}

static int ky_dsi_host_detach(struct mipi_dsi_host *host,
			   struct mipi_dsi_device *slave)
{
	DRM_INFO("%s()\n", __func__);
	/* do nothing */
	return 0;
}

static ssize_t ky_dsi_host_transfer(struct mipi_dsi_host *host,
				const struct mipi_dsi_msg *msg)
{
	struct ky_dsi *dsi = host_to_dsi(host);
	struct ky_dsi_cmd_desc cmd;
	struct ky_dsi_rx_buf dbuf = {0x0};

	cmd.cmd_type = msg->type;
	cmd.length = msg->tx_len;
	memcpy(cmd.data, msg->tx_buf, cmd.length);

	if(msg->flags & MIPI_DSI_MSG_USE_LPM)
		cmd.lp = 1;
	else
		cmd.lp = 0;

	if (msg->rx_buf && msg->rx_len) {
		if (dsi->core && dsi->core->dsi_read_cmds)
			dsi->core->dsi_read_cmds(&dsi->ctx, &dbuf, &cmd, 1);
		memcpy(msg->rx_buf, dbuf.data, 1);
		return 0;
	}

	if (msg->tx_buf && msg->tx_len) {
		if (dsi->core && dsi->core->dsi_write_cmds)
			dsi->core->dsi_write_cmds(&dsi->ctx, &cmd, 1);
	}

	return 0;
}

static const struct mipi_dsi_host_ops ky_dsi_host_ops = {
	.attach = ky_dsi_host_attach,
	.detach = ky_dsi_host_detach,
	.transfer = ky_dsi_host_transfer,
};

static int ky_dsi_host_init(struct device *dev, struct ky_dsi *dsi)
{
	int ret;

	dsi->host.dev = dev;
	dsi->host.ops = &ky_dsi_host_ops;

	ret = mipi_dsi_host_register(&dsi->host);
	if (ret)
		DRM_ERROR("failed to register dsi host\n");

	return ret;
}

static int ky_dsi_connector_get_modes(struct drm_connector *connector)
{
	struct ky_dsi *dsi = connector_to_dsi(connector);

	DRM_DEBUG("%s()\n", __func__);

	return drm_panel_get_modes(dsi->panel, connector);
}

static enum drm_mode_status
ky_dsi_connector_mode_valid(struct drm_connector *connector,
			 struct drm_display_mode *mode)
{
	enum drm_mode_status mode_status = MODE_OK;

	DRM_DEBUG("%s()\n", __func__);

	return mode_status;
}

static struct drm_encoder *
ky_dsi_connector_best_encoder(struct drm_connector *connector)
{
	struct ky_dsi *dsi = connector_to_dsi(connector);

	DRM_DEBUG("%s()\n", __func__);
	return &dsi->encoder;
}

static struct drm_connector_helper_funcs ky_dsi_connector_helper_funcs = {
	.get_modes = ky_dsi_connector_get_modes,
	.mode_valid = ky_dsi_connector_mode_valid,
	.best_encoder = ky_dsi_connector_best_encoder,
};

static enum drm_connector_status
ky_dsi_connector_detect(struct drm_connector *connector, bool force)
{
	struct ky_dsi *dsi = connector_to_dsi(connector);

	DRM_DEBUG("%s() dsi subconnector type %d status %d\n", __func__, dsi->ctx.dsi_subconnector, dsi->ctx.connector_status);

	if (KY_DSI_SUBCONNECTOR_DP == dsi->ctx.dsi_subconnector)
		return dsi->ctx.connector_status;
	else
		return connector_status_connected;
}

static void ky_dsi_connector_destroy(struct drm_connector *connector)
{
	DRM_INFO("%s()\n", __func__);

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs ky_dsi_atomic_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.detect = ky_dsi_connector_detect,
	.destroy = ky_dsi_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static int ky_dsi_connector_init(struct drm_device *drm, struct ky_dsi *dsi)
{
	struct drm_encoder *encoder = &dsi->encoder;
	struct drm_connector *connector = &dsi->connector;
	int ret;

	DRM_DEBUG("%s()\n", __func__);

	connector->polled = DRM_CONNECTOR_POLL_HPD;

	ret = drm_connector_init(drm, connector,
				&ky_dsi_atomic_connector_funcs,
				DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		DRM_ERROR("drm_connector_init() failed\n");
		return ret;
	}

	drm_connector_helper_add(connector,
				 &ky_dsi_connector_helper_funcs);

	//drm_mode_connector_attach_encoder(connector, encoder);
	drm_connector_attach_encoder(connector, encoder);

	return 0;
}

static int __maybe_unused ky_dsi_bridge_attach(struct ky_dsi *dsi)
{
	struct drm_encoder *encoder = &dsi->encoder;
	struct drm_bridge *bridge = dsi->bridge;
	struct device *dev = dsi->host.dev;
	struct device_node *bridge_node;
	int ret;

	bridge_node = of_graph_get_remote_node(dev->of_node, 2, 0);
	if (!bridge_node)
		return 0;

	bridge = of_drm_find_bridge(bridge_node);
	if (!bridge) {
		DRM_ERROR("of_drm_find_bridge() failed\n");
		return -ENODEV;
	}
	dsi->bridge = bridge;

	ret = drm_bridge_attach(encoder, bridge, NULL, 0);
	if (ret) {
		DRM_ERROR("failed to attach external bridge\n");
		return ret;
	}

	return 0;
}

static irqreturn_t ky_dsi_isr(int irq, void *data)
{
	struct ky_dsi *dsi = data;
	DRM_DEBUG("%s: dsi Enter\n", __func__);

	if (dsi->core && dsi->core->isr)
		dsi->core->isr(&dsi->ctx);

	return IRQ_HANDLED;
}

static int ky_dsi_irq_request(struct ky_dsi *dsi)
{
	int ret;
	int irq;

	irq = irq_of_parse_and_map(dsi->host.dev->of_node, 0);
	if (irq) {
		DRM_DEBUG("dsi irq num = %d\n", irq);
		ret = request_irq(irq, ky_dsi_isr, 0, "DSI", dsi);
		if (ret) {
			DRM_ERROR("dsi failed to request irq int0!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int ky_dsi_bind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct ky_dsi *dsi = dev_get_drvdata(dev);
	int ret;

	DRM_DEBUG("%s()\n", __func__);

	ret = ky_dsi_encoder_init(drm, dsi);
	if (ret)
		goto cleanup_host;

	ret = ky_dsi_connector_init(drm, dsi);
	if (ret)
		goto cleanup_encoder;

	ret = ky_dsi_irq_request(dsi);
	if (ret)
		goto cleanup_connector;

	return 0;

cleanup_connector:
	drm_connector_cleanup(&dsi->connector);
cleanup_encoder:
	drm_encoder_cleanup(&dsi->encoder);
cleanup_host:
	mipi_dsi_host_unregister(&dsi->host);
	return ret;
}

static void ky_dsi_unbind(struct device *dev,
			struct device *master, void *data)
{
	/* do nothing */
	DRM_DEBUG("%s()\n", __func__);

}

static const struct component_ops dsi_component_ops = {
	.bind	= ky_dsi_bind,
	.unbind = ky_dsi_unbind,
};

static int ky_dsi_device_create(struct ky_dsi *dsi, struct device *parent)
{
	int ret;

	dsi->dev.class = display_class;
	dsi->dev.parent = parent;
	dsi->dev.of_node = parent->of_node;
	dev_set_name(&dsi->dev, "dsi%d", dsi->ctx.id);
	dev_set_drvdata(&dsi->dev, dsi);

	ret = device_register(&dsi->dev);
	if (ret)
		DRM_ERROR("dsi device register failed\n");

	return ret;
}

static int ky_dsi_context_init(struct ky_dsi *dsi, struct device_node *np)
{
	struct ky_dsi_device *ctx = &dsi->ctx;
	struct resource r;
	u32 tmp;

	if (dsi->core && dsi->core->parse_dt)
		dsi->core->parse_dt(&dsi->ctx, np);

	if (of_address_to_resource(np, 0, &r)) {
		DRM_ERROR("parse dsi ctrl reg base failed\n");
		return -ENODEV;
	}
	ctx->base_addr = (void __iomem *)ioremap(r.start, resource_size(&r));
	if (NULL == ctx->base_addr) {
		DRM_ERROR("dsi ctrl reg base ioremap failed\n");
		return -ENODEV;
	}

	ctx->dsi_subconnector = KY_DSI_SUBCONNECTOR_MIPI_DSI;
	ctx->previous_connector_status = connector_status_connected;
	ctx->connector_status = connector_status_connected;

	if (!of_property_read_u32(np, "dev-id", &tmp))
		ctx->id = tmp;

	return 0;
}

static int ky_dsi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ky_dsi *dsi;
	const char *str;
	int ret;

	DRM_INFO("%s()\n", __func__);

	dsi = devm_kzalloc(&pdev->dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi) {
		DRM_ERROR("failed to allocate dsi data.\n");
		return -ENOMEM;
	}

	if (!of_property_read_string(np, "ip", &str))
		dsi->core = dsi_core_ops_attach(str);
	else
		DRM_WARN("error: 'ip' was not found\n");

	ret = ky_dsi_context_init(dsi, np);
	if (ret)
		return -EINVAL;

	ky_dsi_device_create(dsi, &pdev->dev);
	ky_dsi_sysfs_init(&dsi->dev);
	platform_set_drvdata(pdev, dsi);

	ret = ky_dsi_host_init(&pdev->dev, dsi);
	if (ret)
		return ret;

	return component_add(&pdev->dev, &dsi_component_ops);
}

static int ky_dsi_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &dsi_component_ops);

	return 0;
}

static const struct of_device_id ky_dsi_of_match[] = {
	{ .compatible = "ky,dsi0-host" },
	{ .compatible = "ky,dsi1-host" },
	{ .compatible = "ky,dsi2-host" },
	{ }
};
MODULE_DEVICE_TABLE(of, ky_dsi_of_match);

struct platform_driver ky_dsi_driver = {
	.probe = ky_dsi_probe,
	.remove = ky_dsi_remove,
	.driver = {
		.name = "ky-dsi-drv",
		.of_match_table = ky_dsi_of_match,
	},
};

MODULE_DESCRIPTION("Ky MIPI DSI HOST Controller Driver");
MODULE_LICENSE("GPL v2");


#include <linux/clk.h>
#include <linux/component.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_graph.h>
#include <video/mipi_display.h>
#include <linux/log2.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/regmap.h>
#include <video/videomode.h>
#include <drm/drm_of.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_dsc.h>
#include <drm/drm_edid.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>
#include <drm/drm_panel.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <video/videomode.h>
#include "cvitek_vo_sys_reg.h"
#include "cvitek_drm.h"
#include "cvitek_disp.h"
#include "cvitek_mipipll_cfg.h"

#define bridge_to_cvitek_lvds(b) \
	container_of(b, struct cvitek_lvds, bridge)

#define encoder_to_cvitek_lvds(c) \
	container_of(c, struct cvitek_lvds, encoder)

#define connector_to_cvitek_lvds(c) \
	container_of(c, struct cvitek_lvds, connector)

#define CLK_LANE_NUM   1
#define LANE_MAX_NUM   5

enum lvds_tx_lane_id {
	MIPI_TX_LANE_CLK = 0,
	MIPI_TX_LANE_0,
	MIPI_TX_LANE_1,
	MIPI_TX_LANE_2,
	MIPI_TX_LANE_3,
	MIPI_TX_LANE_MAX,
};

struct cvitek_lvds {
	struct drm_device *drm;
	struct device *dev;
	struct lvds_match_data *info;

	struct drm_bridge *bridge;
	struct drm_connector connector;
	struct drm_panel *panel;
	struct drm_encoder encoder;
	struct lvds_panel_desc *desc;

	enum lvds_tx_lane_id lanes_map[LANE_MAX_NUM];
	bool lanes_swap[LANE_MAX_NUM];
	u32 data_lanes_num;
	u8 bits;
	bool connectors_changed;
};

struct lvds_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int en;
	unsigned int out_bit;
	unsigned int vesa_mode;
	unsigned int dual_ch;
	unsigned int vs_out_en;
	unsigned int hs_out_en;
	unsigned int hs_blk_en;
	unsigned int ml_swap;
	unsigned int ctrl_rev;
	unsigned int oe_swap;
};

struct panel_lvds {
	struct drm_panel panel;
	struct device *dev;
	struct gpio_desc *power_gpio;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
	bool enabled;
	bool prepared;
	bool data_mirror;
	const struct lvds_panel_desc *desc;
	unsigned int width;
	unsigned int height;
	struct videomode video_mode;
	unsigned int bus_format;
	enum drm_panel_orientation orientation;
};

struct lvds_match_data {
	u32 lvds_id;
};

union disp_lvdstx {
	struct {
		u32 out_bit	: 2;
		u32 vesa_mode	: 1;
		u32 dual_ch	: 1;
		u32 vs_out_en	: 1;
		u32 hs_out_en	: 1;
		u32 hs_blk_en	: 1;
		u32 resv_1	: 1;
		u32 ml_swap	: 1;
		u32 ctrl_rev	: 1;
		u32 oe_swap	: 1;
		u32 en		: 1;
		u32 resv	: 20;
	} b;
	u32 raw;
};

static u8 data_0_lane;

static int lvds_dphy_set_lane(u8 lvds_id, u8 lane_num, enum lvds_tx_lane_id lane_id, bool pn_swap)
{
	if ((lane_num > LANE_MAX_NUM - 1) || (lane_id >= LANE_MAX_NUM))
		return -1;

	_reg_write_mask(REG_DSI_PHY_LANE_SEL(lvds_id), 0x7 << (4 * lane_num), lane_id << (4 * lane_num));
	_reg_write_mask(REG_DSI_PHY_LANE_PN_SWAP(lvds_id), BIT(lane_num), pn_swap << lane_num);

	if (lane_id == MIPI_TX_LANE_CLK)
		_reg_write_mask(REG_DSI_PHY_LANE_SEL(lvds_id), 0x1f << 24, 0);

	if (lane_id == MIPI_TX_LANE_0)
		data_0_lane = lane_num;

	return 0;
}

static void lvds_dphy_lane_en(u8 lvds_id, bool *data_en, bool preamble_en)
{
	u8 val = 0, i = 0;

	for (i = 0; i < LANE_MAX_NUM; ++i)
		val |= (data_en[i] << i);
	if (preamble_en)
		val |= 0x20;
	_reg_write_mask(REG_DSI_PHY_EN(lvds_id), 0x3f, val);
}

static void lvds_dphy_power_on(u8 lvds_id)
{
	_reg_write(REG_DSI_PHY_PD(lvds_id), 0x0);
	_reg_write(REG_DSI_PHY_ESC_INIT(lvds_id), 0x100);
	_reg_write(REG_DSI_PHY_ESC_WAKE(lvds_id), 0x100);
	_reg_write(REG_DSI_PHY_EXT_GPIO(lvds_id), 0x0);
	_reg_write(REG_DSI_PHY_POWER_DOWN_CFG(lvds_id), 0x0);

	_reg_write(REG_DSI_PHY_EN_LVDS_CFG(lvds_id), 0x1f1f);
	_reg_write_mask(REG_DSI_PHY_TXPLL_SETUP(lvds_id), 0x3 << 21, 0x3 << 21);

	_reg_write(REG_DSI_PHY_LVDS_EN(lvds_id), 0x1);
}

static int lvds_dphy_lane_init(struct cvitek_lvds *lvds)
{
	bool lanes_en[LANE_MAX_NUM] = {false, false, false, false, false};
	bool preamble_on = false;
	int ret, i;

	DRM_DEBUG_DRIVER("data lanes: %d %d %d %d %d.	lanes swap: %d %d %d %d %d.\n",
				lvds->lanes_map[0], lvds->lanes_map[1], lvds->lanes_map[2], lvds->lanes_map[3], lvds->lanes_map[4],
				lvds->lanes_swap[0], lvds->lanes_swap[1], lvds->lanes_swap[2], lvds->lanes_swap[3], lvds->lanes_swap[4]);

	for (i = 0; i < LANE_MAX_NUM; i++) {
		if ((lvds->lanes_map[i] < 0) || (lvds->lanes_map[i] >= MIPI_TX_LANE_MAX)) {
			ret = lvds_dphy_set_lane(lvds->info->lvds_id, i, MIPI_TX_LANE_MAX, false);
			if (ret) {
				drm_err(lvds->drm, "lvds_dphy_set_lane [%d] fail.\n", i);
				return ret;
			}
		} else {
			ret = lvds_dphy_set_lane(lvds->info->lvds_id, i, lvds->lanes_map[i], lvds->lanes_swap[i]);
			if (!ret) {
				lanes_en[lvds->lanes_map[i]] = true;
			} else {
				drm_err(lvds->drm, "lvds_dphy_set_lane [%d] fail.\n", i);
				return ret;
			}
		}
	}

	lvds_dphy_lane_en(lvds->info->lvds_id, lanes_en, preamble_on);

	return 0;
}

void disp_lvdstx_set(u8 lvds_id, union disp_lvdstx cfg)
{
	_reg_write(REG_VO_MAC_LVDSTX(lvds_id), cfg.raw);
}

void disp_lvdstx_get(u8 lvds_id, union disp_lvdstx *cfg)
{
	cfg->raw = _reg_read(REG_VO_MAC_LVDSTX(lvds_id));
}

static int lvds_dphy_config(struct cvitek_lvds *lvds)
{
	union disp_lvdstx lvds_cfg;

	disp_lvdstx_get(lvds->info->lvds_id, &lvds_cfg);
	lvds_cfg.b.en = lvds->desc->en;
	lvds_cfg.b.out_bit = lvds->desc->out_bit;
	lvds_cfg.b.vesa_mode = lvds->desc->vesa_mode;
	lvds_cfg.b.dual_ch = lvds->desc->dual_ch;
	lvds_cfg.b.vs_out_en = lvds->desc->vs_out_en;
	lvds_cfg.b.hs_out_en = lvds->desc->hs_out_en;
	lvds_cfg.b.hs_blk_en = lvds->desc->hs_blk_en;
	lvds_cfg.b.ml_swap = lvds->desc->ml_swap;
	lvds_cfg.b.ctrl_rev = lvds->desc->ctrl_rev;
	lvds_cfg.b.oe_swap = lvds->desc->oe_swap;
	disp_lvdstx_set(lvds->info->lvds_id, lvds_cfg);
	return 0;
}

static int cvitek_lvds_init(struct cvitek_lvds *lvds)
{
	int ret;

	lvds_dphy_power_on(lvds->info->lvds_id);
	lvds_dphy_config(lvds);
	ret = lvds_dphy_lane_init(lvds);
	if (ret) {
		drm_err(lvds->drm, "mipi_dphy_lane_init fail.\n");
		return ret;
	}

	dphy_lvds_set_pll(lvds->info->lvds_id, lvds->desc->mode->clock, 1);
	return ret;
}

static void cvitek_lvds_encoder_enable(struct drm_encoder *encoder)
{
	struct cvitek_lvds *lvds = encoder_to_cvitek_lvds(encoder);
	struct drm_panel *cvitek_panel = NULL;
	struct panel_lvds *lvds_panel = NULL;
	int ret;

	ret = drm_of_find_panel_or_bridge(lvds->dev->of_node, 1, 0,
					  &lvds->panel, &lvds->bridge);
	if (ret) {
		drm_err(lvds->drm, "drm_of_find_panel_or_bridge encoder_enable");
		return;
	}

	cvitek_panel = lvds->panel;
	lvds_panel = container_of(cvitek_panel, struct panel_lvds, panel);
	lvds->desc = lvds_panel->desc;

	ret = cvitek_lvds_init(lvds);
	if (ret) {
		drm_err(lvds->drm, "failed to init lvds\n");
		return;
	}

	if (lvds->panel) {
		ret = drm_panel_prepare(lvds->panel);
		if (ret) {
			drm_err(lvds->drm, "failed to prepare panel\n");
			return;
		}

		ret = drm_panel_enable(lvds->panel);
		if (ret) {
			drm_err(lvds->drm, "failed to enable panel\n");
			return;
		}
	}
}


static void cvitek_lvds_encoder_disable(struct drm_encoder *encoder)
{

}

static const struct drm_encoder_helper_funcs cvitek_lvds_encoder_helper_funcs = {
	// .mode_set	= cvitek_lvds_encoder_mode_set,
	.enable		= cvitek_lvds_encoder_enable,
	.disable	= cvitek_lvds_encoder_disable
};

static const struct drm_encoder_funcs cvitek_lvds_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int cvitek_lvds_connector_get_modes(struct drm_connector *connector)
{
	struct cvitek_lvds *lvds = connector_to_cvitek_lvds(connector);
	struct drm_panel *panel = NULL;
	int ret;

	ret = drm_of_find_panel_or_bridge(lvds->dev->of_node, 1, 0,
					  &lvds->panel, &lvds->bridge);
	if (ret)
		return ret;

	panel = lvds->panel;

	return drm_panel_get_modes(panel, connector);
}

static int cvitek_lvds_connector_atomic_check(struct drm_connector *connector,
					    struct drm_atomic_state *state)
{
	struct cvitek_lvds *lvds = connector_to_cvitek_lvds(connector);
	const struct drm_display_mode *panel_mode;
	struct drm_connector_state *conn_state;
	struct drm_crtc_state *crtc_state;

	conn_state = drm_atomic_get_new_connector_state(state, connector);
	if (!conn_state->crtc)
		return 0;
	if (list_empty(&connector->modes)) {
		dev_dbg(lvds->dev, "connector: empty modes list\n");
		return -EINVAL;
	}

	panel_mode = list_first_entry(&connector->modes,
				      struct drm_display_mode, head);

	/* We're not allowed to modify the resolution. */
	crtc_state = drm_atomic_get_crtc_state(state, conn_state->crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (crtc_state->mode.hdisplay != panel_mode->hdisplay ||
	    crtc_state->mode.vdisplay != panel_mode->vdisplay)
		return -EINVAL;

	/* The flat panel mode is fixed, just copy it to the adjusted mode. */
	drm_mode_copy(&crtc_state->adjusted_mode, panel_mode);
	return 0;
}

static const struct drm_connector_helper_funcs cvitek_lvds_connector_helper_funcs = {
	.get_modes = cvitek_lvds_connector_get_modes,
	.atomic_check = cvitek_lvds_connector_atomic_check,
};

static enum drm_connector_status
cvitek_lvds_connector_detect(struct drm_connector *connector, bool force)
{
	struct cvitek_lvds *lvds = connector_to_cvitek_lvds(connector);
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek lvds connector detect. ----\n");

	ret = drm_of_find_panel_or_bridge(lvds->dev->of_node, 1, 0,
					  &lvds->panel, &lvds->bridge);
	if (ret)
		DRM_DEBUG_DRIVER("%s: ---- drm find panel or bridge failed!\n", __func__);

	return lvds->panel ? connector_status_connected :
			    connector_status_disconnected;
}

static const struct drm_connector_funcs cvitek_lvds_connector_funcs = {
	.detect = cvitek_lvds_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

// for exp:
// clock-lane = <2>;				/* Tx PAD 2 as clk lane */
// data-lanes = <0 1 3 4>;			/* Tx PAD 0 as d0 , Tx PAD 1 as d1 ,Tx PAD 3 as d2 ,Tx PAD 4 as d3 ,*/
// lanes-swap = <0 0 0 0 0>;		/* pn swap 0 = false, 1 = true */
static int lvds_parse_lane_data(struct cvitek_lvds *lvds)
{
	struct property *prop;
	int ret, i, len, num_lanes;
	u32 clock_lane;
	u32 data_lanes[LANE_MAX_NUM - 1];

	s32 lanes_map_default[LANE_MAX_NUM] = {MIPI_TX_LANE_0, MIPI_TX_LANE_1, MIPI_TX_LANE_CLK, MIPI_TX_LANE_2, MIPI_TX_LANE_3};
	s32 lanes_map[LANE_MAX_NUM] = {-1, -1, -1, -1, -1};
	u32 lanes_swap[LANE_MAX_NUM] = {false, false, false, false, false};

	prop = of_find_property(lvds->dev->of_node, "clock-lane", &len);
	if (!prop) {
		DRM_DEBUG_DRIVER("failed to find clock-lanes lane mapping, using default\n");
		goto default_lanes_maping;
	}

	num_lanes = len / sizeof(u32);

	if (num_lanes != CLK_LANE_NUM) {
		DRM_ERROR("bad number of clock lanes\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(lvds->dev->of_node, "clock-lane", &clock_lane);
	if (ret) {
		DRM_ERROR("failed to read lane data\n");
		return ret;
	}

	lanes_map[clock_lane] = MIPI_TX_LANE_CLK;

	prop = of_find_property(lvds->dev->of_node, "data-lanes", &len);
	if (!prop) {
		DRM_DEBUG_DRIVER("failed to find data lane mapping, using default\n");
		goto default_lanes_maping;
	}

	num_lanes = len / sizeof(u32);

	if (num_lanes < 1 || num_lanes > LANE_MAX_NUM - CLK_LANE_NUM) {
		DRM_ERROR("bad number of data lanes\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(lvds->dev->of_node, "data-lanes", data_lanes,
					 num_lanes);
	if (ret) {
		DRM_ERROR("failed to read lane data\n");
		return ret;
	}

	for (i = 0; i < num_lanes; i++)
		lanes_map[data_lanes[i]] = MIPI_TX_LANE_0 + i;

	lvds->data_lanes_num = num_lanes;

	prop = of_find_property(lvds->dev->of_node, "lanes-swap", &len);
	if (!prop) {
		DRM_DEBUG_DRIVER("failed to find data lane swap, using default\n");
		goto default_lanes_maping;
	}

	num_lanes = len / sizeof(u32);

	if (num_lanes != LANE_MAX_NUM) {
		DRM_ERROR("bad number of data lanes\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(lvds->dev->of_node, "lanes-swap", lanes_swap,
					 num_lanes);
	if (ret) {
		DRM_ERROR("failed to read lane data\n");
		return ret;
	}

	for (i = 0; i < LANE_MAX_NUM; i++) {
		lvds->lanes_map[i] = lanes_map[i];
		lvds->lanes_swap[i] = lanes_swap[i];
	}

	return 0;

default_lanes_maping:
	for (i = 0; i < LANE_MAX_NUM; i++) {
		lvds->lanes_map[i] = lanes_map_default[i];
		lvds->lanes_swap[i] = lanes_swap[i];
	}

	return 0;
}

static int cvitek_lvds_bind(struct device *dev, struct device *master, void *data)
{
	struct cvitek_lvds *lvds = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	int ret = 0;
	struct device_node  *port;

	DRM_DEBUG_DRIVER("---- enter cvitek lvds bind. ----\n");

	lvds = devm_kzalloc(dev, sizeof(*lvds), GFP_KERNEL);
	if (!lvds)
		return -ENOMEM;
	lvds->drm = drm_dev;
	lvds->dev = dev;

	port = of_graph_get_port_by_id(dev->of_node, 1);
	if (!port) {
		drm_err(drm_dev, "can't found port point, please init lvds panel port!\n");
		return -EINVAL;
	}

	lvds->info = (struct lvds_match_data *)of_device_get_match_data(dev);
	if (!lvds->info)
		return -EINVAL;

	ret = lvds_parse_lane_data(lvds);
	if (ret) {
		DRM_ERROR("failed to parse lane data\n");
		return ret;
	}
	encoder = &lvds->encoder;
	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm_dev,
							    dev->of_node);
	if (!encoder->possible_crtcs) {
		drm_err(drm_dev, "failed to find crtc mask\n");
		return -EINVAL;
	}

	ret = drm_simple_encoder_init(drm_dev, encoder, DRM_MODE_ENCODER_LVDS);
	if (ret < 0) {
		DRM_DEV_ERROR(drm_dev->dev,
			      "failed to initialize encoder: %d\n", ret);
	}
	drm_encoder_helper_add(encoder, &cvitek_lvds_encoder_helper_funcs);


	connector = &lvds->connector;
	connector->dpms = DRM_MODE_DPMS_OFF;
	ret = drm_connector_init(drm_dev, connector,
				 &cvitek_lvds_connector_funcs,
				 DRM_MODE_CONNECTOR_LVDS);
	if (ret < 0) {
		DRM_DEV_ERROR(drm_dev->dev,
			      "failed to initialize connector: %d\n", ret);
	}

	drm_connector_helper_add(connector,
				 &cvitek_lvds_connector_helper_funcs);
	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret < 0) {
		DRM_DEV_ERROR(drm_dev->dev,
			      "failed to attach encoder: %d\n", ret);
	}
	return 0;
}

static void cvitek_lvds_unbind(struct device *dev, struct device *master, void *data)
{
	DRM_DEBUG_DRIVER("---- enter cvitek lvds unbind. ----\n");
}

static const struct component_ops lvds_component_ops = {
	.bind	= cvitek_lvds_bind,
	.unbind	= cvitek_lvds_unbind,
};

struct lvds_match_data cv186x_lvds0 = {
	.lvds_id = 0,
};

struct lvds_match_data cv186x_lvds1 = {
	.lvds_id = 1,
};

static const struct of_device_id lvds_match_table[] = {
	{ .compatible = "cvitek,cv186x_lvds0",
	  .data = &cv186x_lvds0 },
#if IS_ENABLED(CONFIG_CVITEK_LVDS1)
	{ .compatible = "cvitek,cv186x_lvds1",
	  .data = &cv186x_lvds1 },
#endif
	{ /* end node */ },
};

static int cvitek_lvds_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek lvds remove. ----\n");

	component_del(&pdev->dev, &lvds_component_ops);
	return 0;
}

static int cvitek_lvds_probe(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek lvds probe. ----\n");

	return component_add(&pdev->dev, &lvds_component_ops);
}

struct platform_driver cvitek_lvds_driver = {
	.probe = cvitek_lvds_probe,
	.remove = cvitek_lvds_remove,
	.driver = {
		   .name = "cvitek-lvds-drv",
		   .of_match_table = lvds_match_table,
	},
};

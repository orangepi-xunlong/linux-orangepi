#ifndef __SPRD_DSI_H__
#define __SPRD_DSI_H__

#include <linux/of.h>
#include <linux/device.h>
#include <linux/regmap.h>
#include <video/videomode.h>

#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_print.h>
#include <drm/drm_panel.h>

#define encoder_to_dsi(encoder) \
	container_of(encoder, struct cvitek_dsi, encoder)

#define connector_to_dsi(connector) \
	container_of(connector, struct cvitek_dsi, connector)

#define CLK_LANE_NUM   1
#define LANE_MAX_NUM   5
#define MAX_DSI_LP   16

enum dsi_mode {
	DSI_MODE_IDLE = 0,
	DSI_MODE_SPKT = 1,
	DSI_MODE_ESC = 2,
	DSI_MODE_HS = 4,
	DSI_MODE_UNKNOWN,
	DSI_MODE_MAX = DSI_MODE_UNKNOWN,
};

enum video_mode_e {
	BURST_MODE                      = 0x0,
	NON_BURST_MODE_SYNC_PULSES      = 0x1,
	NON_BURST_MODE_SYNC_EVENTS      = 0x2,
	MODE_NONE,
};

enum cvi_dsi_fmt {
	DSI_FMT_RGB888 = 0,
	DSI_FMT_RGB666,
	DSI_FMT_RGB565,
	DSI_FMT_RGB101010,//drm not support
	DSI_FORMAT_UNSUPPORT,
};

static const struct cvitek_format dsi_formats[] = {
	{ MIPI_DSI_FMT_RGB888, DSI_FMT_RGB888 },
	{ MIPI_DSI_FMT_RGB666_PACKED, DSI_FMT_RGB666 },
	{ MIPI_DSI_FMT_RGB565, DSI_FMT_RGB565 },
};

enum mipi_tx_lane_id {
	MIPI_TX_LANE_CLK = 0,
	MIPI_TX_LANE_0,
	MIPI_TX_LANE_1,
	MIPI_TX_LANE_2,
	MIPI_TX_LANE_3,
	MIPI_TX_LANE_MAX,
};

struct dsi_context {
	u32 dsi_id;
	struct clk *dsi_clk;
	struct videomode vm;
	enum mipi_tx_lane_id lanes_map[LANE_MAX_NUM];
	bool lanes_swap[LANE_MAX_NUM];
	u32 data_lanes_num;
	u8 bits;
	bool connectors_changed;
};

struct cvitek_dsi {
	struct drm_device *drm;
	struct mipi_dsi_host host;
	struct mipi_dsi_device *slave;
	struct drm_encoder encoder;
	struct drm_connector connector;
	struct drm_panel *panel;
	struct dsi_context ctx;
};

struct dsi_match_data {
	u32 dsi_id;
};

#endif /* __SPRD_DSI_H__ */

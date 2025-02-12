// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_DSI_H_
#define _KY_DSI_H_

#include <linux/of.h>
#include <linux/device.h>
#include <linux/types.h>
#include <video/videomode.h>

#include <drm/drm_print.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_encoder.h>
#include <drm/drm_connector.h>
#include <drm/drm_bridge.h>
#include <drm/drm_panel.h>

#include "ky_lib.h"
#include "ky_dphy.h"

/* define LCD_IS_READY to bypass kernel dsi driver */
/* #define LCD_IS_READY */

/*advanced setting*/
#define LPM_FRAME_EN_DEFAULT 0
#define LAST_LINE_TURN_DEFAULT 0
#define HEX_SLOT_EN_DEFAULT 0
#define HSA_PKT_EN_DEFAULT_SYNC_PULSE 1
#define HSA_PKT_EN_DEFAULT_OTHER 0
#define HSE_PKT_EN_DEFAULT_SYNC_PULSE 1
#define HSE_PKT_EN_DEFAULT_OTHER 0
#define HBP_PKT_EN_DEFAULT 1
#define HFP_PKT_EN_DEFAULT 0
#define HEX_PKT_EN_DEFAULT 0
#define HLP_PKT_EN_DEFAULT 0
#define AUTO_DLY_DIS_DEFAULT 0
#define TIMING_CHECK_DIS_DEFAULT 0
#define HACT_WC_EN_DEFAULT 1
#define AUTO_WC_DIS_DEFAULT 0
#define VSYNC_RST_EN_DEFAULT 1

#define MAX_TX_CMD_COUNT 256
#define MAX_RX_DATA_COUNT 64

enum ky_mipi_burst_mode{
	DSI_BURST_MODE_NON_BURST_SYNC_PULSE = 0,
	DSI_BURST_MODE_NON_BURST_SYNC_EVENT = 1,
	DSI_BURST_MODE_BURST = 2,
	DSI_BURST_MODE_MAX
};

enum ky_mipi_input_data_mode{
	DSI_INPUT_DATA_RGB_MODE_565 = 0,
	DSI_INPUT_DATA_RGB_MODE_666PACKET = 1,
	DSI_INPUT_DATA_RGB_MODE_666UNPACKET = 2,
	DSI_INPUT_DATA_RGB_MODE_888 = 3,
	DSI_INPUT_DATA_RGB_MODE_MAX
};

enum ky_dsi_work_mode {
	KY_DSI_MODE_VIDEO,
	KY_DSI_MODE_CMD,
	KY_DSI_MODE_MAX
};

enum ky_dsi_cmd_type {
	KY_DSI_DCS_SWRITE = 0x5,
	KY_DSI_DCS_SWRITE1 = 0x15,
	KY_DSI_DCS_LWRITE = 0x39,
	KY_DSI_DCS_READ = 0x6,
	KY_DSI_GENERIC_LWRITE = 0x29,
	KY_DSI_GENERIC_READ1 = 0x14,
	KY_DSI_SET_MAX_PKT_SIZE = 0x37,
};

enum ky_dsi_tx_mode {
	KY_DSI_HS_MODE = 0,
	KY_DSI_LP_MODE = 1,
};

enum ky_dsi_rx_data_type {
	KY_DSI_ACK_ERR_RESP = 0x2,
	KY_DSI_EOTP = 0x8,
	KY_DSI_GEN_READ1_RESP = 0x11,
	KY_DSI_GEN_READ2_RESP = 0x12,
	KY_DSI_GEN_LREAD_RESP = 0x1A,
	KY_DSI_DCS_READ1_RESP = 0x21,
	KY_DSI_DCS_READ2_RESP = 0x22,
	KY_DSI_DCS_LREAD_RESP = 0x1C,
};

enum ky_dsi_polarity {
	KY_DSI_POLARITY_POS = 0,
	KY_DSI_POLARITY_NEG,
	KY_DSI_POLARITY_MAX
};

enum ky_dsi_te_mode {
	KY_DSI_TE_MODE_NO = 0,
	KY_DSI_TE_MODE_A,
	KY_DSI_TE_MODE_B,
	KY_DSI_TE_MODE_C,
	KY_DSI_TE_MODE_MAX,
};

enum ky_dsi_event_id {
	KY_DSI_EVENT_ERROR,
	KY_DSI_EVENT_MAX,
};

enum ky_dsi_status {
	DSI_STATUS_UNINIT = 0,
	DSI_STATUS_OPENED = 1,
	DSI_STATUS_INIT = 2,
	DSI_STATUS_MAX
};

struct ky_dsi_advanced_setting {
	uint32_t lpm_frame_en; /*return to LP mode every frame*/
	uint32_t last_line_turn;
	uint32_t hex_slot_en;
	uint32_t hsa_pkt_en;
	uint32_t hse_pkt_en;
	uint32_t hbp_pkt_en; /*bit:18*/
	uint32_t hfp_pkt_en; /*bit:20*/
	uint32_t hex_pkt_en;
	uint32_t hlp_pkt_en; /*bit:22*/
	uint32_t auto_dly_dis;
	uint32_t timing_check_dis;
	uint32_t hact_wc_en;
	uint32_t auto_wc_dis;
	uint32_t vsync_rst_en;
};

struct ky_mipi_info {
	unsigned int height;
	unsigned int width;
	unsigned int hfp; /*pixel*/
	unsigned int hbp;
	unsigned int hsync;
	unsigned int vfp; /*line*/
	unsigned int vbp;
	unsigned int vsync;
	unsigned int fps;

	unsigned int work_mode; /*command_mode, video_mode*/
	unsigned int rgb_mode;
	unsigned int lane_number;
	unsigned int phy_bit_clock;
	unsigned int phy_esc_clock;
	unsigned int split_enable;
	unsigned int eotp_enable;

	/*for video mode*/
	unsigned int burst_mode;

	/*for cmd mode*/
	unsigned int te_enable;
	unsigned int vsync_pol;
	unsigned int te_pol;
	unsigned int te_mode;

	/*The following fields need not be set by panel*/
	unsigned int real_fps;
};

struct ky_dsi_cmd_desc {
	enum ky_dsi_cmd_type cmd_type;
	uint8_t lp;
	uint32_t delay;	/* time to delay */
	uint32_t length;	/* cmds length */
	uint8_t data[MAX_TX_CMD_COUNT];
};

struct ky_dsi_rx_buf {
	enum ky_dsi_rx_data_type data_type;
	uint32_t length; /* cmds length */
	uint8_t data[MAX_RX_DATA_COUNT];
};

enum ky_dsi_subconnector {
	KY_DSI_SUBCONNECTOR_MIPI_DSI   = 0,
	KY_DSI_SUBCONNECTOR_HDMI       = 1,
	KY_DSI_SUBCONNECTOR_DP         = 2,
	KY_DSI_SUBCONNECTOR_eDP        = 3,
};

struct ky_dsi_device {
	uint16_t id;
	void __iomem *base_addr;
	struct ky_dsi_advanced_setting adv_setting;
	struct ky_mipi_info mipi_info;
	struct videomode vm;
	struct ky_dphy *phy;
	enum ky_dsi_status status;
	enum drm_connector_status previous_connector_status;
	enum drm_connector_status connector_status;
	enum ky_dsi_subconnector dsi_subconnector;
};

struct dsi_core_ops {
	int (*parse_dt)(struct ky_dsi_device *ctx, struct device_node *np);
	int (*isr)(struct ky_dsi_device *ctx);
	int (*dsi_open)(struct ky_dsi_device *ctx, bool ready);
	int (*dsi_close)(struct ky_dsi_device *ctx);
	int (*dsi_write_cmds)(struct ky_dsi_device *ctx, struct ky_dsi_cmd_desc *cmds, int count);
	int (*dsi_read_cmds)(struct ky_dsi_device *ctx, struct ky_dsi_rx_buf *dbuf,
								struct ky_dsi_cmd_desc *cmds, int count);
	int (*dsi_ready_for_datatx)(struct ky_dsi_device *ctx);
	int (*dsi_close_datatx)(struct ky_dsi_device *ctx);
};

struct ky_dsi {
	struct device dev;
	struct mipi_dsi_host host;
	struct mipi_dsi_device *slave;
	struct drm_encoder encoder;
	struct drm_connector connector;
	struct drm_bridge *bridge;
	struct drm_panel *panel;
	struct dsi_core_ops *core;
	struct ky_dsi_device ctx;
};

extern struct list_head dsi_core_head;

#define dsi_core_ops_register(entry) \
	disp_ops_register(entry, &dsi_core_head)
#define dsi_core_ops_attach(str) \
	disp_ops_attach(str, &dsi_core_head)

#define encoder_to_dsi(encoder) \
	container_of(encoder, struct ky_dsi, encoder)
#define host_to_dsi(host) \
	container_of(host, struct ky_dsi, host)
#define connector_to_dsi(connector) \
	container_of(connector, struct ky_dsi, connector)

#endif /* _KY_DSI_H_ */

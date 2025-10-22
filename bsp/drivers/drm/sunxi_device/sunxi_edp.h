/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * edp_core.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __DRM_EDP_CORE_H__
#define __DRM_EDP_CORE_H__

#include <sunxi-log.h>
#include <linux/dev_printk.h>
#include <linux/types.h>
#include <drm/drm_edid.h>
#include <drm/drm_displayid.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-dp.h>
#include <linux/wait.h>
#include <sound/hdmi-codec.h>
#include "include.h"

extern u32 loglevel_debug;

enum edp_hpd_status {
	EDP_HPD_PLUGOUT = 0,
	EDP_HPD_PLUGIN = 1,
};


enum edp_ssc_mode_e {
	SSC_CENTER_MODE = 0,
	SSC_DOWNSPR_MODE,
};

enum edp_pattern_e {
	PATTERN_NONE = 0,
	TPS1,
	TPS2,
	TPS3,
	TPS4,
	PRBS7,
	D10_2,
	HBR2_EYE,
	LINK_QUALITY_PATTERN,
	CP2520_PATTERN2,
	CP2520_PATTERN3,
	SYMBOL_MEASURE_PATTERN,
	PATTERN_80BIT,
};

enum edp_video_mapping_e {
	RGB_6BIT = 0,
	RGB_8BIT,
	RGB_10BIT,
	RGB_12BIT,
	RGB_16BIT,
	YCBCR444_6BIT,
	YCBCR444_8BIT,
	YCBCR444_10BIT,
	YCBCR444_12BIT,
	YCBCR444_16BIT,
	YCBCR422_6BIT,
	YCBCR422_8BIT,
	YCBCR422_10BIT,
	YCBCR422_12BIT,
	YCBCR422_16BIT,
	YCBCR420_6BIT,
	YCBCR420_8BIT,
	YCBCR420_10BIT,
	YCBCR420_12BIT,
	YCBCR420_16BIT,
};

enum dp_hdcp_mode {
	HDCP_NONE_MODE = 0,
	HDCP14_MODE = 1,
	HDCP23_MODE = 2,
};


#define RET_OK (0)
#define RET_FAIL (-1)
#define RET_AUX_DEFER (-2)
#define RET_AUX_NACK  (-3)
#define RET_AUX_NO_STOP (-4)
#define RET_AUX_TIMEOUT (-5)
#define RET_AUX_RPLY_ERR (-6)
#define RET_I2C_AUX_DEFER (-7)
#define RET_I2C_AUX_NACK  (-8)
/* some platform has do retry in lowlevel, if lowlevel rerry
fail in lowlevel, it's not need to do in framework again */
#define RET_AUX_RETRY_FAIL (-9)

#define TRY_CNT_MAX					5
#define TRY_CNT_TIMEOUT				20
#define MAX_VLEVEL					3
#define MAX_PLEVEL					3
#define LINK_STATUS_SIZE			6
#define TRAINING_PATTERN_DISABLE	0
#define TRAINING_PATTERN_1			1
#define TRAINING_PATTERN_2			2
#define TRAINING_PATTERN_3			3
#define TRAINING_PATTERN_4			4

#define SW_VOLTAGE_CHANGE_FLAG		(1 << 0)
#define PRE_EMPHASIS_CHANGE_FLAG	(1 << 1)

#define DPCD_0000H 0x0000
#define DPCD_0001H 0x0001

#define DPCD_0002H 0x0002
#define DPCD_TPS3_SUPPORT_MASK		(1 << 6)

#define DPCD_0003H 0x0003
#define DPCD_FAST_TRAIN_MASK		(1 << 6)

#define DPCD_0004H 0x0004
#define DPCD_0005H 0x0005
#define DPCD_0006H 0x0006
#define DPCD_0100H 0x0100

#define DPCD_0101H 0x0101
#define DPCD_ENHANCE_FRAME_ENABLE_MASK	(1 << 7)
#define DPCD_LANE_CNT_MASK				(0x1f << 0)

#define DPCD_0102H 0x0102
#define DPCD_SCRAMBLING_DISABLE_FLAG	(1 << 5)

#define DPCD_0103H 0x0103
#define DPCD_0104H 0x0104
#define DPCD_0105H 0x0105
#define DPCD_0106H 0x0106
#define DPCD_VLEVEL_SHIFT 0
#define DPCD_PLEVEL_SHIFT 3
#define DPCD_MAX_VLEVEL_REACHED_FLAG	(1 << 2)
#define DPCD_MAX_PLEVEL_REACHED_FLAG	(1 << 5)

#define DPCD_0107H 0x0107
#define DPCD_0108H 0x0108

#define DPCD_010AH 0x010A
#define DPCD_ASSR_ENABLE_MASK       (1 << 0)

#define DPCD_0200H 0x0200
#define DPCD_0201H 0x0201
#define DPCD_0202H 0x0202
#define DPCD_0203H 0x0203
#define LINK_STATUS_BASE DPCD_0202H
#define DPCD_LANE_STATUS_OFFSET(x)	(x / 2)
#define DPCD_LANE_STATUS_SHIFT(x)	(x & 0b1)
#define DPCD_LANE_STATUS_MASK(x)	(0xf << (4 * DPCD_LANE_STATUS_SHIFT(x)))
#define DPCD_CR_DONE(x)				(0x1 << (4 * DPCD_LANE_STATUS_SHIFT(x)))
#define DPCD_EQ_DONE(x)				(0x2 << (4 * DPCD_LANE_STATUS_SHIFT(x)))
#define DPCD_SYMBOL_LOCK(x)			(0x4 << (4 * DPCD_LANE_STATUS_SHIFT(x)))
#define DPCD_EQ_TRAIN_DONE(x)		(DPCD_EQ_DONE(x) | DPCD_SYMBOL_LOCK(x))

#define DPCD_0204H 0x0204
#define DPCD_ALIGN_DONE				(1 << 0)

#define DPCD_0205H 0x0205

#define DPCD_0206H 0x0206
#define DPCD_0207H 0x0207
#define DPCD_LANE_ADJ_OFFSET(x)		((DPCD_0206H - LINK_STATUS_BASE) + (x / 2))
#define VADJUST_SHIFT(x)			(4 * (x & 0b1))
#define PADJUST_SHIFT(x)			((4 * (x & 0b1)) + 2)
#define VADJUST_MASK(x)				(0x3 << VADJUST_SHIFT(x))
#define PADJUST_MASK(x)				(0x3 << PADJUST_SHIFT(x))

#define DPCD_0600H 0x0600
#define DPCD_LOW_POWER_ENTER		0x2
#define DPCD_LOW_POWER_EXIT			0x1

#define DPCD_2200H 0x2200

/* DPCD For HDCP1.x*/
#define DPCD_68000H						(0x68000)
#define DPCD_68005H						(0x68005)
#define DPCD_68007H						(0x68007)
#define DPCD_6800CH						(0x6800C)
#define DPCD_68014H						(0x68014)
#define DPCD_68028H						(0x68028)
#define DPCD_68029H						(0x68029)
#define DPCD_6802AH						(0x6802A)
#define DPCD_6802CH						(0x6802C)
#define DPCD_6803BH						(0x6803B)
#define DPCD_6803CH						(0x6803C)
#define DPCD_680C0H						(0x680C0)

#define HDCP1X_DPCD_BKSV				DPCD_68000H
#define HDCP1X_DPCD_BKSV_BYTES			(5)

#define HDCP1X_DPCD_R0_PRIME			DPCD_68005H
#define HDCP1X_DPCD_R0_PRIME_BYTES		(2)

#define HDCP1X_DPCD_AKSV				DPCD_68007H
#define HDCP1X_DPCD_AKSV_BYTES			(5)

#define HDCP1X_DPCD_AN					DPCD_6800CH
#define HDCP1X_DPCD_AN_BYTES			(8)

#define HDCP1X_DPCD_V_PRIME				DPCD_68014H
#define HDCP1X_DPCD_V_PRIME_BYTES		(20)

#define HDCP1X_DPCD_BCAPS				DPCD_68028H
#define HDCP1X_DPCD_BCAPS_BYTES			(1)
#define HDCP1X_CAPABLE					BIT(0)
#define HDCP1X_REPEATER					BIT(1)

#define HDCP1X_DPCD_BSTATUS				DPCD_68029H
#define HDCP1X_DPCD_BSTATUS_BYTES		(1)
#define HDCP1X_KSV_LIST_READY			BIT(0)
#define HDCP1X_R0_PRIME_AVAILABLE		BIT(1)
#define HDCP1X_REAUTHENTICATION_REQ		BIT(3)

#define HDCP1X_DPCD_BINFO				DPCD_6802AH
#define HDCP1X_DPCD_BINFO_BYTES			(2)
#define HDCP1X_M0_BYTES					(8)
#define HDCP1X_REPEATER_DEVICE_CNT_MASK	0x7F
#define HDCP1X_REPEATER_DEVICE_EXCEED	BIT(7)
#define HDCP1X_REPEATER_DEVICE_DEP_MASK	0x7
#define HDCP1X_REPEATER_CASCADE_EXCEED	BIT(4)

#define HDCP1X_DPCD_KSV_FIFO			DPCD_6802CH
#define HDCP1X_DPCD_KSV_FIFO_BYTES		(15)

#define HDCP1X_DPCD_KSV_AINFO			DPCD_6803BH
#define HDCP1X_DPCD_KSV_AINFO_BYTES		(1)
#define HDCP1X_REAUTH_ENABLE_IRQ_HPD	BIT(0)

#define HDCP1X_DPCD_RESERVED			DPCD_6803CH
#define HDCP1X_DPCD_RESERVED_BYTES		(132)

#define HDCP1X_DPCD_DEBUG				DPCD_680C0H
#define HDCP1X_DPCD_DEBUG_BYTES			(64)

#define EDP_DPCD_MAX_LANE_MASK					(0x1f << 0)
#define EDP_DPCD_ENHANCE_FRAME_MASK				(1 << 7)
#define EDP_DPCD_TPS3_MASK						(1 << 6)
#define EDP_DPCD_FAST_TRAIN_MASK				(1 << 6)
#define EDP_DPCD_DOWNSTREAM_PORT_MASK			(1 << 0)
#define EDP_DPCD_DOWNSTREAM_PORT_TYPE_MASK		(3 << 1)
#define EDP_DPCD_DOWNSTREAM_PORT_CNT_MASK		(0xf << 0)
#define EDP_DPCD_LOCAL_EDID_MASK				(1 << 1)
#define EDP_DPCD_ASSR_MASK						(1 << 0)
#define EDP_DPCD_FRAME_CHANGE_MASK				(1 << 1)


/* DPCD For HDCP2.x*/
#define DPCD_69000H						(0x69000)
#define DPCD_69008H						(0x69008)
#define DPCD_6900BH						(0x6800B)
#define DPCD_69215H						(0x69215)
#define DPCD_6921DH						(0x6921D)
#define DPCD_69220H						(0x69220)
#define DPCD_692A0H						(0x692A0)
#define DPCD_692B0H						(0x692B0)
#define DPCD_692C0H						(0x692C0)
#define DPCD_692E0H						(0x692E0)
#define DPCD_692F0H						(0x692F0)
#define DPCD_692F8H						(0x692F8)
#define DPCD_69318H						(0x69318)
#define DPCD_69328H						(0x69328)
#define DPCD_69330H						(0x69330)
#define DPCD_69332H						(0x69332)
#define DPCD_69335H						(0x69335)
#define DPCD_69345H						(0x69345)
#define DPCD_693E0H						(0x693E0)
#define DPCD_693F0H						(0x693F0)
#define DPCD_693F3H						(0x693F3)
#define DPCD_693F5H						(0x693F5)
#define DPCD_69473H						(0x69473)
#define DPCD_69493H						(0x69493)
#define DPCD_69494H						(0x69494)
#define DPCD_69518H						(0x69518)


/* DPCD reference */
#define HDCP1_MAX_REPEATER_DEV_CNT			(127)
#define HDCP1_MAX_REPEATER_DEV_DEP			(7)
#define HDCP1_KSV_LIST_BYTES_PER_DEV		(5)
#define HDCP1_KSV_LIST_FIFO_BYTES			(15)
#define HDCP1_DEV_CNT_PER_KSV_LIST_FIFO		(3)
#define HDCP1_V_PRIME_BYTES				(20)

#define EDP_DBG(fmt, ...)			sunxi_debug(NULL, "[EDP_DBG]: "fmt, ##__VA_ARGS__)
#define EDP_ERR(fmt, ...)			sunxi_err(NULL, "[EDP_ERR]: "fmt, ##__VA_ARGS__)
#define EDP_WRN(fmt, ...)			sunxi_warn(NULL, "[EDP_WRN]: "fmt, ##__VA_ARGS__)
#define EDP_INFO(fmt, ...)			sunxi_info(NULL, "[EDP_INFO]: "fmt, ##__VA_ARGS__)
#define EDP_DEV_ERR(dev, fmt, ...)	sunxi_err(dev, "[EDP_ERR]: "fmt, ##__VA_ARGS__)

#define EDP_DRV_DBG(fmt, ...) \
			do { \
				if (loglevel_debug & 0x1) \
					sunxi_info(NULL, "[EDP_DRV]: "fmt, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[EDP_DRV]: "fmt, ##__VA_ARGS__); \
			} while (0)

#define EDP_CORE_DBG(fmt, ...) \
			do { \
				if (loglevel_debug & 0x2) \
					sunxi_info(NULL, "[EDP_CORE]: "fmt, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[EDP_CORE]: "fmt, ##__VA_ARGS__); \
			} while (0)

#define EDP_LOW_DBG(fmt, ...) \
			do { \
				if (loglevel_debug & 0x4) \
					sunxi_info(NULL, "[EDP_LOW]: "fmt, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[EDP_LOW]: "fmt, ##__VA_ARGS__); \
			} while (0)

#define EDP_EDID_DBG(fmt, ...) \
			do { \
				if (loglevel_debug & 0x8) \
					sunxi_info(NULL, "[EDP_EDID]: "fmt, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[EDP_EDID]: "fmt, ##__VA_ARGS__); \
			} while (0)

#define EDP_HDCP_DBG(fmt, ...) \
			do { \
				if (loglevel_debug & 0x10) \
					sunxi_info(NULL, "[EDP_HDCP]: "fmt, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[EDP_HDCP]: "fmt, ##__VA_ARGS__); \
			} while (0)

/*edp bit rate  unit:Hz*/
#define BIT_RATE_1G62 ((unsigned long long)1620 * 1000 * 1000)
#define BIT_RATE_2G7  ((unsigned long long)2700 * 1000 * 1000)
#define BIT_RATE_5G4  ((unsigned long long)5400 * 1000 * 1000)
#define BIT_RATE_8G1  ((unsigned long long)8100 * 1000 * 1000)
#define BIT_RATE_2G16 ((unsigned long long)2160 * 1000 * 1000)
#define BIT_RATE_2G43 ((unsigned long long)2430 * 1000 * 1000)
#define BIT_RATE_3G24 ((unsigned long long)3240 * 1000 * 1000)
#define BIT_RATE_4G32 ((unsigned long long)4320 * 1000 * 1000)

enum disp_edp_colordepth {
	EDP_8_BIT = 0,
	EDP_10_BIT = 1,
};

struct edp_lane_para {
	u64 bit_rate;
	u32 lane_cnt;
	u32 lane_sw[4];
	u32 lane_pre[4];
	u32 lane_remap[4];
	u32 lane_invert[4];
	u32 colordepth;
	u32 color_fmt;
	u32 colormetry;
	u32 bpp;
};

struct sunxi_edp_hw_desc;
struct sunxi_edp_hw_video_ops;
struct sunxi_edp_hw_audio_ops;
struct sunxi_edp_hw_hdcp_ops;



struct edp_tx_cap {
	/* decided by edp phy */
	u64 max_rate;
	u32 max_lane;
	bool tps3_support;
	bool fast_train_support;
	bool audio_support;
	bool ssc_support;
	bool psr_support;
	bool assr_support;
	bool fec_support;
	bool mst_support;
	bool psr2_support;
	bool hdcp1x_support;
	bool hdcp2x_support;
	bool hardware_hdcp1x_support;
	bool hardware_hdcp2x_support;
	bool enhance_frame_support;
	bool lane_remap_support;
	bool lane_invert_support;
	bool muti_pixel_mode_support;
	bool need_reset_before_enable;
};

struct edp_rx_cap {
	/*parse from dpcd*/
	u32 dpcd_rev;
	u64 max_rate;
	u32 max_lane;
	bool tps3_support;
	bool fast_train_support;
	bool fast_train_debug;
	bool downstream_port_support;
	u32 downstream_port_type;
	u32 downstream_port_cnt;
	bool local_edid_support;
	bool is_edp_device;
	bool assr_support;
	bool enhance_frame_support;
	bool framing_change_support;

	/*parse from edid*/
	u32 mfg_week;
	u32 mfg_year;
	u32 edid_ver;
	u32 edid_rev;
	u32 input_type;
	u32 bit_depth;
	u32 video_interface;
	u32 width_cm;
	u32 height_cm;

	/*parse from edid_ext*/
	bool Ycc444_support;
	bool Ycc422_support;
	bool Ycc420_support;
	bool audio_support;
};


struct edp_tx_core {
	u32 ssc_en;
	s32 ssc_mode;
	u32 pixel_mode;
	u32 pclk_limit_khz;
	u32 psr_en;
	u32 training_param_type;
	u32 interval_CR;
	u32 interval_EQ;
	/* 0:edp_mode  1:dp-mode*/
	u32 controller_mode;
	bool interlace;
	bool sync_clock;
	bool force_level;
	struct edp_lane_para lane_para;
	struct edp_lane_para debug_lane_para;
	struct edp_lane_para backup_lane_para;

	struct disp_video_timings timings;

	struct edid *edid;

	struct phy *dp_phy;
	struct phy *aux_phy;
	struct phy *combo_phy;
};

/* define this for low-level indepent for muti-controller case */
/* it also useful for new controller IP support */
struct sunxi_edp_hw_desc {
	void __iomem *reg_base;
	void __iomem *top_base;
	u32 cur_aux_request;
	u64 cur_bit_rate;
	struct mutex aux_lock;
	struct sunxi_edp_hw_video_ops *video_ops;
	struct sunxi_edp_hw_audio_ops *audio_ops;
	struct sunxi_edp_hw_hdcp_ops *hdcp_ops;
};

struct sunxi_edp_hw_video_ops {
	bool (*check_controller_error)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*get_hotplug_change)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*get_hotplug_state)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_start_delay)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_cur_line)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_tu_size)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_pixel_clk)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_pixel_mode)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*get_color_format)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_pattern)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*get_lane_para)(struct sunxi_edp_hw_desc *edp_hw, struct edp_lane_para *tmp_lane_para);
	u32 (*get_tu_valid_symbol)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*set_pattern)(struct sunxi_edp_hw_desc *edp_hw, u32 pattern, u32 lane_cnt);
	s32 (*assr_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	s32 (*psr_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	bool (*psr_is_enabled)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*psr2_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	bool (*psr2_is_enabled)(struct sunxi_edp_hw_desc *edp_hw);
	void (*psr2_set_area)(u32 top, u32 bot, u32 left, u32 width);
	s32 (*ssc_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	s32 (*ssc_set_mode)(struct sunxi_edp_hw_desc *edp_hw, u32 mode);
	s32 (*ssc_get_mode)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*ssc_is_enabled)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*aux_read)(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf);
	s32 (*aux_write)(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf);
	s32 (*aux_i2c_read)(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 len, char *buf);
	s32 (*aux_i2c_write)(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 len, char *buf);
	s32 (*read_edid_block)(struct sunxi_edp_hw_desc *edp_hw, u8 *raw_edid, unsigned int block_id, size_t len);
	s32 (*irq_enable)(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id);
	s32 (*irq_disable)(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id);
	void (*irq_handle)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
	s32 (*link_soft_reset)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*video_soft_reset)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*main_link_start)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*main_link_stop)(struct sunxi_edp_hw_desc *edp_hw);
	void (*scrambling_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	void (*enhance_frame_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	void (*fec_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	void (*set_lane_sw_pre)(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, u32 sw, u32 pre, u32 param_type);
	void (*set_lane_cnt)(struct sunxi_edp_hw_desc *edp_hw, u32 lane_cnt);
	void (*set_lane_rate)(struct sunxi_edp_hw_desc *edp_hw, u64 bit_rate);
	s32 (*lane_remap)(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, u32 remap_id);
	s32 (*lane_invert)(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, bool invert);
	s32 (*set_pixel_mode)(struct sunxi_edp_hw_desc *edp_hw, u32 pixel_mode);
	s32 (*init_early)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*init)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
	s32 (*enable)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
	s32 (*disable)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
	s32 (*set_video_timings)(struct sunxi_edp_hw_desc *edp_hw, struct disp_video_timings *tmgs);
	s32 (*set_video_format)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
	s32 (*config_tu)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
	void (*config_mst)(struct sunxi_edp_hw_desc *edp_hw, u32 mst_cnt);
	s32 (*query_tu_capability)(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core, struct disp_video_timings *tmgs);
	u64 (*support_max_rate)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*support_max_lane)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_tps3)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_fast_training)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_psr)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_psr2)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_ssc)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_mst)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_fec)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_assr)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_lane_remap)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_lane_invert)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_muti_pixel_mode)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*need_reset_before_enable)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_enhance_frame)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*set_use_inner_clk)(struct sunxi_edp_hw_desc *edp_hw, u32 bypass);
};

struct sunxi_edp_hw_audio_ops {
	bool (*support_audio)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*audio_is_muted)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*audio_is_enabled)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*audio_enable)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*audio_disable)(struct sunxi_edp_hw_desc *edp_hw);
	s32 (*audio_mute)(struct sunxi_edp_hw_desc *edp_hw,
					  bool enable, int direction);
	s32 (*audio_config)(struct sunxi_edp_hw_desc *edp_hw, int interface,
		      int chn_cnt, int data_width, int data_rate);
	u32 (*get_audio_chn_cnt)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_audio_data_width)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_audio_if)(struct sunxi_edp_hw_desc *edp_hw);
	u32 (*get_audio_max_channel)(struct sunxi_edp_hw_desc *edp_hw);
};

struct sunxi_edp_hw_hdcp_ops {
	bool (*support_hdcp1x)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_hw_hdcp1x)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_hdcp2x)(struct sunxi_edp_hw_desc *edp_hw);
	bool (*support_hw_hdcp2x)(struct sunxi_edp_hw_desc *edp_hw);
	void (*hdcp_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
	void (*hdcp_set_mode)(struct sunxi_edp_hw_desc *edp_hw, enum dp_hdcp_mode mode);
	u64 (*hdcp1_get_an)(struct sunxi_edp_hw_desc *edp_hw);
	u64 (*hdcp1_get_aksv)(struct sunxi_edp_hw_desc *edp_hw);
	void (*hdcp1_write_bksv)(struct sunxi_edp_hw_desc *edp_hw, u64 bksv);
	u64 (*hdcp1_cal_km)(struct sunxi_edp_hw_desc *edp_hw, u64 bksv);
	u32 (*hdcp1_cal_r0)(struct sunxi_edp_hw_desc *edp_hw, u64 an, u64 km);
	u64 (*hdcp1_get_m0)(struct sunxi_edp_hw_desc *edp_hw);
	void (*hdcp1_encrypt_enable)(struct sunxi_edp_hw_desc *edp_hw, bool enable);
};

enum sunxi_dp_hdcp_status {
	DP_HDCP_STATUS_NONE = 0,
	DP_HDCP_STATUS_SUCCESS,
	DP_HDCP_STATUS_FAIL,
	DP_HDCP_STATUS_PENDING,
};

enum sunxi_dp_hdcp1_state {
	HDCP1_DP_STATE_NONE = 0,
	HDCP1_DP_STATE_START,
	HDCP1_A0_DETERMINE_RX_HDCP_CAPABLE,
	HDCP1_A1_EXCHANGE_KSVS_WRITE_AN_AKSV,
	HDCP1_A1_EXCHANGE_KSVS_READ_BKSV,
	HDCP1_A1_EXCHANGE_KSVS_VALIDATE_BKSV,
	HDCP1_A1_EXCHANGE_KSVS_CAL_KM,
	HDCP1_A2_COMPUTATIONS,
	HDCP1_A2_A3_WAIT_FOR_R0_PRIME,
	HDCP1_A2_A3_READ_R0_PRIME,
	HDCP1_A3_VALIDATE_RX,
	HDCP1_A5_TEST_FOR_REPEATER,
	HDCP1_A6_WAIT_FOR_READY,
	HDCP1_A7_READ_KSV_LIST,
	HDCP1_A7_CALCULATE_V,
	HDCP1_A7_VERIFY_V_V_PRIME,
	HDCP1_A4_AUTHENTICATED,
	HDCP1_DP_STATE_END,
	HDCP1_READ_BCAPS_FAIL,
	HDCP1_A0_HDCP_NOT_SUPPORT,
	HDCP1_A1_WRITE_AN_AKSV_FAIL,
	HDCP1_A1_READ_BKSV_FAIL,
	HDCP1_A1_VALIDATE_BKSV_FAIL,
	HDCP1_A1_WRITE_AINFO_FAIL,
	HDCP1_A2_A3_READ_R0_PRIME_FAIL,
	HDCP1_A3_VALIDATE_RX_FAIL,
	HDCP1_A6_READ_BSTATUS_BINFO_FAIL,
	HDCP1_A6_WAIT_FOR_READY_FAIL,
	HDCP1_A6_REPEATER_DEV_DEP_EXCEED_FAIL,
	HDCP1_A7_READ_KSV_LIST_FAIL,
	HDCP1_A7_READ_V_PRIME_FAIL,
	HDCP1_A7_VERIFY_V_V_PRIME_FAIL,
};

struct sunxi_dptx_hdcp1_info {
	enum sunxi_dp_hdcp_status status;
	enum sunxi_dp_hdcp1_state state;

	/* retry count when read V prime fail */
	u32 v_read_retry;

	/* retry count when read r0 prime fail */
	u32 r0_prime_retry;

	/* status parse from HDCP's DPCD*/
	bool r0_prime_ready;
	bool ksv_list_ready;
	bool hdcp1_capable;
	bool rx_is_repeater;
	bool device_exceeded;
	bool cascade_exceeded;

	/* 8-bit data indicate rx capability */
	u8 bcaps;

	/* 40-bit data*/
	u64 aksv;

	/* 64-bit Pseudo random value */
	u64 an;

	/* 40-bit data*/
	u64 bksv;

	/* 64-bit after-calculate data */
	u64 km;

	/* 16-bit after-calculate data */
	u32 r0;
	u32 r0_prime;

	/* ksv list contain all device, count = 5 * dev_cnt*/
	u8 ksv_list[HDCP1_MAX_REPEATER_DEV_CNT * HDCP1_KSV_LIST_BYTES_PER_DEV];
	u32 ksv_cnt;

	/* 16-bit data from DPCD */
	u8 binfo[HDCP1X_DPCD_BINFO_BYTES];

	/* FIXME: consider append m0 to binfo*/
	/* 64-bit data generate by hdcp tx */
	u8 m0[HDCP1X_M0_BYTES];

	/* 20-bytes data after-calculate from hdcp rx */
	u8 v_prime[HDCP1_V_PRIME_BYTES];
	u8 v[HDCP1_V_PRIME_BYTES];

	/* information parse from binfo */
	u32 repeater_dev_cnt;
	u32 repeater_dev_dep;
};

enum sunxi_dp_hdcp2_state {
	HDCP2_DP_STATE_NONE,
};

struct sunxi_dptx_hdcp2x_info {
	enum sunxi_dp_hdcp_status status;
	enum sunxi_dp_hdcp2_state state;
};


struct sunxi_dp_hdcp {
	wait_queue_head_t auth_queue;
	struct mutex auth_lock;
	struct sunxi_edp_hw_desc *edp_hw;

	bool hdcp1_capable;
	bool hdcp2_capable;

	struct sunxi_dptx_hdcp1_info hdcp1_info;
	struct sunxi_dptx_hdcp2x_info hdcp2_info;
};

int edp_get_edid_block(void *data, u8 *edid,
			  unsigned int block, size_t len);
s32 edp_edid_put(struct edid *edid);
s32 edp_edid_cea_db_offsets(const u8 *cea, s32 *start, s32 *end);
u8 *sunxi_drm_find_edid_extension(const struct edid *edid,
				   int ext_id, int *ext_index);


u64 edp_source_get_max_rate(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_source_get_max_lane(struct sunxi_edp_hw_desc *edp_hw);

bool edp_source_support_tps3(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_fast_training(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_ssc(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_psr(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_psr2(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_assr(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_mst(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_fec(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_hdcp1x(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_hdcp2x(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_hardware_hdcp1x(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_hardware_hdcp2x(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_enhance_frame(struct sunxi_edp_hw_desc *edp_hw);
bool edp_set_use_inner_clk(struct sunxi_edp_hw_desc *edp_hw, u32 bypass);
bool edp_source_support_lane_remap(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_lane_invert(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_support_muti_pixel_mode(struct sunxi_edp_hw_desc *edp_hw);
bool edp_source_need_reset_before_enable(struct sunxi_edp_hw_desc *edp_hw);
bool edp_hw_check_controller_error(struct sunxi_edp_hw_desc *edp_hw);

bool edp_hw_ssc_is_enabled(struct sunxi_edp_hw_desc *edp_hw);
bool edp_hw_psr_is_enabled(struct sunxi_edp_hw_desc *edp_hw);

void edp_hw_set_reg_base(struct sunxi_edp_hw_desc *edp_hw, void __iomem *base);
void edp_hw_set_top_base(struct sunxi_edp_hw_desc *edp_hw, void __iomem *base);
void edp_hw_show_builtin_patten(struct sunxi_edp_hw_desc *edp_hw, u32 pattern);
void edp_hw_irq_handler(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
void edp_hw_scrambling_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable);

s32 edp_hw_get_hotplug_state(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_list_standard_mode_num(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_video_soft_reset(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_link_soft_reset(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_link_start(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_link_stop(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_low_power_en(struct sunxi_edp_hw_desc *edp_hw, bool en);
s32 edp_hw_init_early(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_controller_init(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
s32 edp_hw_enable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
s32 edp_hw_disable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
s32 edp_hw_irq_enable(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id, bool en);
s32 edp_hw_irq_query(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_irq_clear(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_get_cur_line(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_get_start_dly(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_aux_read(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf);
s32 edp_hw_aux_write(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf);
s32 edp_hw_aux_i2c_read(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 len, char *buf);
s32 edp_hw_aux_i2c_write(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 len, char *buf);
s32 edp_hw_ssc_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable);
s32 edp_hw_ssc_set_mode(struct sunxi_edp_hw_desc *edp_hw, u32 mode);
s32 edp_hw_ssc_get_mode(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_psr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable);
s32 edp_hw_assr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable);
s32 edp_hw_enhance_frame_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable);
s32 edp_hw_get_color_fmt(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_set_pattern(struct sunxi_edp_hw_desc *edp_hw, u32 pattern, u32 lane_cnt);
u32 edp_hw_get_pixclk(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_get_pixel_mode(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_get_pattern(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_get_lane_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_lane_para *tmp_lane_para);
u32 edp_hw_get_tu_size(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_get_valid_symbol_per_tu(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_set_video_format(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
s32 edp_hw_set_video_timings(struct sunxi_edp_hw_desc *edp_hw, struct disp_video_timings *tmgs);
s32 edp_hw_set_pixel_mode(struct sunxi_edp_hw_desc *edp_hw, u32 pixel_mode);
s32 edp_hw_set_transfer_config(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core);
void edp_hw_set_mst(struct sunxi_edp_hw_desc *edp_hw, u32 mst_cnt);
s32 edp_hw_query_lane_capability(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core,
								   struct disp_video_timings *tmgs);
bool edp_source_support_audio(struct sunxi_edp_hw_desc *edp_hw);
bool edp_hw_audio_is_enabled(struct sunxi_edp_hw_desc *edp_hw);
bool edp_hw_audio_is_mute(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_audio_enable(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_audio_disable(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_get_audio_if(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_get_audio_chn_cnt(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_get_audio_data_width(struct sunxi_edp_hw_desc *edp_hw);
u32 edp_hw_audio_get_max_channel(struct sunxi_edp_hw_desc *edp_hw);
s32 edp_hw_audio_config(struct sunxi_edp_hw_desc *edp_hw, int interface,
		      int chn_cnt, int data_width, int data_rate);
s32 edp_hw_audio_mute(struct sunxi_edp_hw_desc *edp_hw, bool enable, int direction);
s32 edp_main_link_setup(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core,
						bool bypass, bool force_level);
s32 sunxi_edp_hw_callback_init(struct sunxi_edp_hw_desc *edp_hw);



#define NATIVE_READ   0b1001
#define NATIVE_WRITE  0b1000
#define AUX_I2C_READ  0b0001
#define AUX_I2C_WRITE 0b0000

#define AUX_REPLY_DEFER 0b0010
#define AUX_REPLY_NACK  0b0001
#define AUX_REPLY_ACK   0b0000
#define AUX_REPLY_I2C_DEFER 0b1000
#define AUX_REPLY_I2C_NACK  0b0100
#define AUX_REPLY_I2C_ACK   0b0000

#define EDID_ADDR DDC_ADDR

#if IS_ENABLED(CONFIG_AW_DRM_EDP_CONTROLLER_USED)
struct sunxi_edp_hw_video_ops *sunxi_edp_get_hw_video_ops(void);
struct sunxi_edp_hw_audio_ops *sunxi_edp_get_hw_audio_ops(void);

#if IS_ENABLED(CONFIG_AW_DRM_DP_HDCP)
/* hdcp*/
struct sunxi_edp_hw_hdcp_ops *sunxi_dp_get_hw_hdcp_ops(void);
s32 sunxi_dp_hdcp_init(struct sunxi_dp_hdcp *hdcp, struct sunxi_edp_hw_desc *edp_hw);
s32 sunxi_dp_hdcp1_enable(struct sunxi_dp_hdcp *hdcp);
s32 sunxi_dp_hdcp1_disable(struct sunxi_dp_hdcp *hdcp);
bool dprx_hdcp1_capable(struct sunxi_dp_hdcp *hdcp);
#else
static inline struct sunxi_edp_hw_hdcp_ops *sunxi_dp_get_hw_hdcp_ops(void)
{
	EDP_ERR("HDCP is not support for sunxi edp!\n");
	return NULL;
}

static inline u32 sunxi_dp_hdcp_init(struct sunxi_dp_hdcp *hdcp, struct sunxi_edp_hw_desc *edp_hw)
{
	return RET_OK;
}

static inline u32 sunxi_dp_hdcp1_enable(struct sunxi_dp_hdcp *hdcp)
{
	return RET_OK;
}

static inline u32 sunxi_dp_hdcp1_disable(struct sunxi_dp_hdcp *hdcp)
{
	return RET_OK;
}
static inline bool dprx_hdcp1_capable(struct sunxi_dp_hdcp *hdcp)
{
	return false;
}
#endif

#else
static inline struct sunxi_edp_hw_video_ops *sunxi_edp_get_hw_video_ops(void)
{
	EDP_ERR("there is no controller selected for sunxi edp!\n");
	return NULL;
}

static inline struct sunxi_edp_hw_audio_ops *sunxi_edp_get_hw_audio_ops(void)
{
	EDP_ERR("there is no controller selected for sunxi edp!\n");
	return NULL;
}
#endif

#endif /*End of file*/

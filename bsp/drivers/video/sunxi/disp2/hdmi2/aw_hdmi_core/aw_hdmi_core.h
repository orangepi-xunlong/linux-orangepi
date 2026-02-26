/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _AW_HDMI_CORE_H_
#define _AW_HDMI_CORE_H_

#include <uapi/video/sunxi_display2.h>
#include <video/drv_hdmi.h>

#include "dw_hdmi/dw_i2cm.h"
#include "dw_hdmi/dw_mc.h"
#include "dw_hdmi/dw_video.h"
#include "dw_hdmi/dw_audio.h"
#include "dw_hdmi/dw_fc.h"
#include "dw_hdmi/dw_phy.h"
#include "dw_hdmi/dw_scdc.h"
#include "dw_hdmi/dw_edid.h"
#include "dw_hdmi/dw_hdcp.h"
#include "dw_hdmi/dw_access.h"
#include "dw_hdmi/dw_dev.h"
#include "dw_hdmi/dw_hdcp22_tx.h"
#include "dw_hdmi/dw_cec.h"
#include "dw_hdmi/phy_aw.h"
#include "dw_hdmi/phy_inno.h"
#include "dw_hdmi/phy_snps.h"
#include "dw_hdmi/phy_inno_fpga.h"

#define	DISP_CONFIG_UPDATE_NULL			0x0
#define	DISP_CONFIG_UPDATE_MODE		    0x1
#define	DISP_CONFIG_UPDATE_FORMAT		0x2
#define	DISP_CONFIG_UPDATE_BITS			0x4
#define	DISP_CONFIG_UPDATE_EOTF		    0x8
#define	DISP_CONFIG_UPDATE_CS			0x10
#define	DISP_CONFIG_UPDATE_DVI			0x20
#define	DISP_CONFIG_UPDATE_RANGE		0x40
#define	DISP_CONFIG_UPDATE_SCAN		    0x80
#define	DISP_CONFIG_UPDATE_RATIO		0x100

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
enum {
	AW_HDMI_CEC_STAT_DONE		= BIT(0),
	AW_HDMI_CEC_STAT_EOM		= BIT(1),
	AW_HDMI_CEC_STAT_NACK		= BIT(2),
	AW_HDMI_CEC_STAT_ARBLOST	= BIT(3),
	AW_HDMI_CEC_STAT_ERROR_INIT	= BIT(4),
	AW_HDMI_CEC_STAT_ERROR_FOLL	= BIT(5),
	AW_HDMI_CEC_STAT_WAKEUP		= BIT(6),
};

enum {
	AW_HDMI_CEC_FRAME_TYPE_RETRY    = 0,
	AW_HDMI_CEC_FRAME_TYPE_NORMAL   = 1,
	AW_HDMI_CEC_FRAME_TYPE_IMMED    = 2,
};
#endif

enum hdmi_vic {
	/* Refer to CEA 861-D */
	HDMI_UNKNOWN           = 0,
	HDMI_640x480P60_4x3    = 1,
	HDMI_720x480P60_4x3    = 2,
	HDMI_720x480P60_16x9   = 3,
	HDMI_1280x720P60_16x9  = 4,
	HDMI_1920x1080I60_16x9 = 5,
	HDMI_720x480I60_4x3    = 6,
	HDMI_720x480I60_16x9   = 7,
	HDMI_720x240P60_4x3    = 8,
	HDMI_720x240P60_16x9   = 9,
	HDMI_2880x480I60_4x3   = 10,
	HDMI_2880x480I60_16x9  = 11,
	HDMI_2880x240P60_4x3   = 12,
	HDMI_2880x240P60_16x9  = 13,
	HDMI_1440x480P60_4x3   = 14,
	HDMI_1440x480P60_16x9  = 15,
	HDMI_1920x1080P60_16x9 = 16,
	HDMI_720x576P50_4x3    = 17,
	HDMI_720x576P50_16x9   = 18,
	HDMI_1280x720P50_16x9  = 19,
	HDMI_1920x1080I50_16x9 = 20,
	HDMI_720x576I50_4x3    = 21,
	HDMI_720x576I50_16x9   = 22,
	HDMI_720x288P_4x3      = 23,
	HDMI_720x288P_16x9     = 24,
	HDMI_2880x576I50_4x3   = 25,
	HDMI_2880x576I50_16x9  = 26,
	HDMI_2880x288P50_4x3   = 27,
	HDMI_2880x288P50_16x9  = 28,
	HDMI_1440x576P_4x3     = 29,
	HDMI_1440x576P_16x9    = 30,
	HDMI_1920x1080P50_16x9 = 31,
	HDMI_1920x1080P24_16x9 = 32,
	HDMI_1920x1080P25_16x9 = 33,
	HDMI_1920x1080P30_16x9 = 34,
	HDMI_2880x480P60_4x3   = 35,
	HDMI_2880x480P60_16x9  = 36,
	HDMI_2880x576P50_4x3   = 37,
	HDMI_2880x576P50_16x9  = 38,
	HDMI_1920x1080I_T1250_50_16x9 = 39,
	HDMI_1920x1080I100_16x9 = 40,
	HDMI_1280x720P100_16x9  = 41,
	HDMI_720x576P100_4x3    = 42,
	HDMI_720x576P100_16x9   = 43,
	HDMI_720x576I100_4x3    = 44,
	HDMI_720x576I100_16x9   = 45,
	HDMI_1920x1080I120_16x9 = 46,
	HDMI_1280x720P120_16x9  = 47,
	HDMI_720x480P120_4x3    = 48,
	HDMI_720x480P120_16x9   = 49,
	HDMI_720x480I120_4x3    = 50,
	HDMI_720x480I120_16x9   = 51,
	HDMI_720x576P200_4x3    = 52,
	HDMI_720x576P200_16x9   = 53,
	HDMI_720x576I200_4x3    = 54,
	HDMI_720x576I200_16x9   = 55,
	HDMI_720x480P240_4x3    = 56,
	HDMI_720x480P240_16x9   = 57,
	HDMI_720x480I240_4x3    = 58,
	HDMI_720x480I240_16x9   = 59,
	/* Refet to CEA 861-F */
	HDMI_1280x720P24_16x9    = 60,
	HDMI_1280x720P25_16x9    = 61,
	HDMI_1280x720P30_16x9    = 62,
	HDMI_1920x1080P120_16x9  = 63,
	HDMI_1920x1080P100_16x9  = 64,
	HDMI_1280x720P24_64x27   = 65,
	HDMI_1280x720P25_64x27   = 66,
	HDMI_1280x720P30_64x27   = 67,
	HDMI_1280x720P50_64x27   = 68,
	HDMI_1280x720P60_64x27   = 69,
	HDMI_1280x720P100_64x27  = 70,
	HDMI_1280x720P120_64x27  = 71,
	HDMI_1920x1080P24_64x27  = 72,
	HDMI_1920x1080P25_64x27  = 73,
	HDMI_1920x1080P30_64x27  = 74,
	HDMI_1920x1080P50_64x27  = 75,
	HDMI_1920x1080P60_64x27  = 76,
	HDMI_1920x1080P100_64x27 = 77,
	HDMI_1920x1080P120_64x27 = 78,
	HDMI_1680x720P24_64x27   = 79,
	HDMI_1680x720P25_64x27   = 80,
	HDMI_1680x720P30_64x27   = 81,
	HDMI_1680x720P50_64x27   = 82,
	HDMI_1680x720P60_64x27   = 83,
	HDMI_1680x720P100_64x27  = 84,
	HDMI_1680x720P120_64x27  = 85,
	HDMI_2560x1080P24_64x27  = 86,
	HDMI_2560x1080P25_64x27  = 87,
	HDMI_2560x1080P30_64x27  = 88,
	HDMI_2560x1080P50_64x27  = 89,
	HDMI_2560x1080P60_64x27  = 90,
	HDMI_2560x1080P100_64x27 = 91,
	HDMI_2560x1080P120_64x27 = 92,
	HDMI_3840x2160P24_16x9   = 93,
	HDMI_3840x2160P25_16x9   = 94,
	HDMI_3840x2160P30_16x9   = 95,
	HDMI_3840x2160P50_16x9   = 96,
	HDMI_3840x2160P60_16x9   = 97,
	HDMI_4096x2160P24_256x135 = 98,
	HDMI_4096x2160P25_256x135 = 99,
	HDMI_4096x2160P30_256x135 = 100,
	HDMI_4096x2160P50_256x135 = 101,
	HDMI_4096x2160P60_256x135 = 102,
	HDMI_3840x2160P24_64x27   = 103,
	HDMI_3840x2160P25_64x27   = 104,
	HDMI_3840x2160P30_64x27   = 105,
	HDMI_3840x2160P50_64x27   = 106,
	HDMI_3840x2160P60_64x27   = 107,
	HDMI_RESERVED = 108,

	/* special resolution */
	HDMI_2560x1440P60 = 0x201,
	HDMI_1440x2560P70 = 0x202,
	HDMI_1080x1920P60 = 0x203,
};

struct cea_vic {
	int vic;
	const char *name;
};

#define __DEF_VIC(_vic) \
{                       \
	.vic  =  _vic,      \
	.name = #_vic,      \
}

static const struct cea_vic hdmi_cea_vics[] = {
	__DEF_VIC(HDMI_UNKNOWN),
	__DEF_VIC(HDMI_640x480P60_4x3),
	__DEF_VIC(HDMI_720x480P60_4x3),
	__DEF_VIC(HDMI_720x480P60_16x9),
	__DEF_VIC(HDMI_1280x720P60_16x9),
	__DEF_VIC(HDMI_1920x1080I60_16x9),
	__DEF_VIC(HDMI_720x480I60_4x3),
	__DEF_VIC(HDMI_720x480I60_16x9),
	__DEF_VIC(HDMI_720x240P60_4x3),
	__DEF_VIC(HDMI_720x240P60_16x9),
	__DEF_VIC(HDMI_2880x480I60_4x3),
	__DEF_VIC(HDMI_2880x480I60_16x9),
	__DEF_VIC(HDMI_2880x240P60_4x3),
	__DEF_VIC(HDMI_2880x240P60_16x9),
	__DEF_VIC(HDMI_1440x480P60_4x3),
	__DEF_VIC(HDMI_1440x480P60_16x9),
	__DEF_VIC(HDMI_1920x1080P60_16x9),
	__DEF_VIC(HDMI_720x576P50_4x3),
	__DEF_VIC(HDMI_720x576P50_16x9),
	__DEF_VIC(HDMI_1280x720P50_16x9),
	__DEF_VIC(HDMI_1920x1080I50_16x9),
	__DEF_VIC(HDMI_720x576I50_4x3),
	__DEF_VIC(HDMI_720x576I50_16x9),
	__DEF_VIC(HDMI_720x288P_4x3),
	__DEF_VIC(HDMI_720x288P_16x9),
	__DEF_VIC(HDMI_2880x576I50_4x3),
	__DEF_VIC(HDMI_2880x576I50_16x9),
	__DEF_VIC(HDMI_2880x288P50_4x3),
	__DEF_VIC(HDMI_2880x288P50_16x9),
	__DEF_VIC(HDMI_1440x576P_4x3),
	__DEF_VIC(HDMI_1440x576P_16x9),
	__DEF_VIC(HDMI_1920x1080P50_16x9),
	__DEF_VIC(HDMI_1920x1080P24_16x9),
	__DEF_VIC(HDMI_1920x1080P25_16x9),
	__DEF_VIC(HDMI_1920x1080P30_16x9),
	__DEF_VIC(HDMI_2880x480P60_4x3),
	__DEF_VIC(HDMI_2880x480P60_16x9),
	__DEF_VIC(HDMI_2880x576P50_4x3),
	__DEF_VIC(HDMI_2880x576P50_16x9),
	__DEF_VIC(HDMI_1920x1080I_T1250_50_16x9),
	__DEF_VIC(HDMI_1920x1080I100_16x9),
	__DEF_VIC(HDMI_1280x720P100_16x9),
	__DEF_VIC(HDMI_720x576P100_4x3),
	__DEF_VIC(HDMI_720x576P100_16x9),
	__DEF_VIC(HDMI_720x576I100_4x3),
	__DEF_VIC(HDMI_720x576I100_16x9),
	__DEF_VIC(HDMI_1920x1080I120_16x9),
	__DEF_VIC(HDMI_1280x720P120_16x9),
	__DEF_VIC(HDMI_720x480P120_4x3),
	__DEF_VIC(HDMI_720x480P120_16x9),
	__DEF_VIC(HDMI_720x480I120_4x3),
	__DEF_VIC(HDMI_720x480I120_16x9),
	__DEF_VIC(HDMI_720x576P200_4x3),
	__DEF_VIC(HDMI_720x576P200_16x9),
	__DEF_VIC(HDMI_720x576I200_4x3),
	__DEF_VIC(HDMI_720x576I200_16x9),
	__DEF_VIC(HDMI_720x480P240_4x3),
	__DEF_VIC(HDMI_720x480P240_16x9),
	__DEF_VIC(HDMI_720x480I240_4x3),
	__DEF_VIC(HDMI_720x480I240_16x9),

	/* Refet to CEA 861-F */
	__DEF_VIC(HDMI_1280x720P24_16x9),
	__DEF_VIC(HDMI_1280x720P25_16x9),
	__DEF_VIC(HDMI_1280x720P30_16x9),
	__DEF_VIC(HDMI_1920x1080P120_16x9),
	__DEF_VIC(HDMI_1920x1080P100_16x9),
	__DEF_VIC(HDMI_1280x720P24_64x27),
	__DEF_VIC(HDMI_1280x720P25_64x27),
	__DEF_VIC(HDMI_1280x720P30_64x27),
	__DEF_VIC(HDMI_1280x720P50_64x27),
	__DEF_VIC(HDMI_1280x720P60_64x27),
	__DEF_VIC(HDMI_1280x720P100_64x27),
	__DEF_VIC(HDMI_1280x720P120_64x27),
	__DEF_VIC(HDMI_1920x1080P24_64x27),
	__DEF_VIC(HDMI_1920x1080P25_64x27),
	__DEF_VIC(HDMI_1920x1080P30_64x27),
	__DEF_VIC(HDMI_1920x1080P50_64x27),
	__DEF_VIC(HDMI_1920x1080P60_64x27),
	__DEF_VIC(HDMI_1920x1080P100_64x27),
	__DEF_VIC(HDMI_1920x1080P120_64x27),
	__DEF_VIC(HDMI_1680x720P24_64x27),
	__DEF_VIC(HDMI_1680x720P25_64x27),
	__DEF_VIC(HDMI_1680x720P30_64x27),
	__DEF_VIC(HDMI_1680x720P50_64x27),
	__DEF_VIC(HDMI_1680x720P60_64x27),
	__DEF_VIC(HDMI_1680x720P100_64x27),
	__DEF_VIC(HDMI_1680x720P120_64x27),
	__DEF_VIC(HDMI_2560x1080P24_64x27),
	__DEF_VIC(HDMI_2560x1080P25_64x27),
	__DEF_VIC(HDMI_2560x1080P30_64x27),
	__DEF_VIC(HDMI_2560x1080P50_64x27),
	__DEF_VIC(HDMI_2560x1080P60_64x27),
	__DEF_VIC(HDMI_2560x1080P100_64x27),
	__DEF_VIC(HDMI_2560x1080P120_64x27),
	__DEF_VIC(HDMI_3840x2160P24_16x9),
	__DEF_VIC(HDMI_3840x2160P25_16x9),
	__DEF_VIC(HDMI_3840x2160P30_16x9),
	__DEF_VIC(HDMI_3840x2160P50_16x9),
	__DEF_VIC(HDMI_3840x2160P60_16x9),
	__DEF_VIC(HDMI_4096x2160P24_256x135),
	__DEF_VIC(HDMI_4096x2160P25_256x135),
	__DEF_VIC(HDMI_4096x2160P30_256x135),
	__DEF_VIC(HDMI_4096x2160P50_256x135),
	__DEF_VIC(HDMI_4096x2160P60_256x135),
	__DEF_VIC(HDMI_3840x2160P24_64x27),
	__DEF_VIC(HDMI_3840x2160P25_64x27),
	__DEF_VIC(HDMI_3840x2160P30_64x27),
	__DEF_VIC(HDMI_3840x2160P50_64x27),
	__DEF_VIC(HDMI_3840x2160P60_64x27),

	/* special resolution */
	__DEF_VIC(HDMI_2560x1440P60),
	__DEF_VIC(HDMI_1440x2560P70),
	__DEF_VIC(HDMI_1080x1920P60),
};

#define HDMI_1080P_24_3D_FP  (HDMI_1920x1080P24_16x9 + 0x80)
#define HDMI_720P_50_3D_FP   (HDMI_1280x720P50_16x9 + 0x80)
#define HDMI_720P_60_3D_FP   (HDMI_1280x720P60_16x9 + 0x80)

struct disp_hdmi_mode {
    enum disp_tv_mode mode;
    int  hdmi_mode; /* vic code */
};

struct aw_blacklist_edid {
    u8  mft_id[2]; /* EDID manufacture id */
    u8  stib[13];  /* EDID standard timing information blocks */
    u8  checksum;
};

struct aw_blacklist_issue {
    u32 tv_mode;
    u32 issue_type;
};

struct aw_sink_blacklist {
    struct aw_blacklist_edid  sink;
    struct aw_blacklist_issue issue[10];
};

struct hdmi_mode {
    dw_video_param_t      pVideo;
    dw_audio_param_t      pAudio;
    dw_hdcp_param_t       pHdcp;
    dw_product_param_t    pProduct;

    int               edid_done;
    struct edid	      *edid;
    u8		          *edid_ext; /* edid extenssion raw data */
    sink_edid_t	      *sink_cap;
};

struct aw_device_ops {
    s32 (*dev_smooth_enable)(void);
    s32 (*dev_tv_mode_check)(u32 mode);

    int (*dev_get_blacklist_issue)(u32 mode);
    s32 (*dev_tv_mode_get)(void);

    int (*dev_config)(void);

    int (*dev_standby)(void);
    int (*dev_close)(void);

#ifndef SUPPORT_ONLY_HDMI14
    int (*scdc_read)(u8 address, u8 size, u8 *data);
    int (*scdc_write)(u8 address, u8 size, u8 *data);
#endif

    s32 (*dev_tv_mode_update_dtd)(u32 mode);
};

struct aw_video_ops {
    u32 (*get_color_space)(void);
    u32 (*get_color_depth)(void);
    s32 (*get_color_format)(void);
    u32 (*get_color_metry)(void);
    u8 (*get_color_range)(void);
    void (*set_tmds_mode)(u8 enable);
    u32 (*get_tmds_mode)(void);

    void (*set_avmute)(u8 enable);
    u32 (*get_avmute)(void);

    u32 (*get_scramble)(void);
    u32 (*get_pixel_repetion)(void);
    void (*set_drm_up)(dw_fc_drm_pb_t *pb);
    u32 (*get_vic_code)(void);
};

struct aw_audio_ops {
    u32 (*get_layout)(void);
    u32 (*get_channel_count)(void);
    u32 (*get_sample_freq)(void);
    u32 (*get_sample_size)(void);
    u32 (*get_acr_n)(void);
    int (*audio_config)(void);
};

struct aw_phy_ops {
    int (*phy_write)(u16 addr, u32 data);
    int (*phy_read)(u16 addr, u32 *value);
    u32 (*phy_get_rxsense)(void);
    u32 (*phy_get_pll_lock)(void);
    u32 (*phy_get_power)(void);
    void (*phy_set_power)(u8 enable);
    void (*phy_reset)(void);
    int (*phy_resume)(void);
    void (*set_hpd)(u8 enable);
    u8 (*get_hpd)(void);
};

struct aw_hdcp_ops {
    void (*hdcp_close)(void);
    void (*hdcp_configure)(void);
    void (*hdcp_disconfigure)(void);
    int (*hdcp_get_status)(void);
    u32 (*hdcp_get_avmute)(void);
    int (*hdcp_get_type)(void);
    ssize_t (*hdcp_config_dump)(char *buf);
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
    u32 (*hdcp22_get_support)(void);
#endif
};

struct aw_edid_ops {
    void (*main_release)(void);
    void (*main_read)(void);
    void (*set_prefered_video)(void);
    void (*correct_hw_config)(void);
    bool (*get_test_mode)(void);
    void (*set_test_mode)(bool en);
    void (*set_test_data)(const unsigned char *data, unsigned int size);
};

struct aw_cec_ops {
    int (*enable)(void);
    int (*disable)(void);
    s32 (*send)(unsigned char *msg, unsigned size, unsigned frame_type);
    s32 (*receive)(unsigned char *msg, unsigned *size);
    u16 (*get_la)(void);
    int (*set_la)(unsigned int addr);
    u16 (*get_pa)(void);
    int (*get_ir_state)(void);
    void (*clear_ir_state)(unsigned state);
};

/**
 * aw hdmi core
 */
struct aw_hdmi_core_s {
    int   blacklist_sink;
    int   hdmi_tx_phy;
    uintptr_t reg_base;

    struct hdmi_mode    mode;
    struct disp_device_config   config;

    struct device_access  acs_ops;
    struct aw_device_ops dev_ops;
    struct aw_video_ops   video_ops;
    struct aw_audio_ops   audio_ops;
    struct aw_phy_ops     phy_ops;
    struct aw_edid_ops    edid_ops;
    struct aw_hdcp_ops    hdcp_ops;
    struct aw_cec_ops     cec_ops;

    dw_hdmi_dev_t      hdmi_tx;
};

int aw_hdmi_core_init(struct aw_hdmi_core_s *core, int phy, dw_hdcp_param_t *hdcp);

void aw_hdmi_core_exit(struct aw_hdmi_core_s *core);

#endif /* _AW_HDMI_CORE_H_ */

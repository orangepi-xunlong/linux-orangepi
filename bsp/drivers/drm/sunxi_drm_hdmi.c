/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/firmware.h>
#include <linux/component.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_of.h>
#include <drm/drm_connector.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_probe_helper.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)
#include <drm/drm_scdc_helper.h>
#else
#include <drm/display/drm_scdc_helper.h>
#endif

#include <sunxi-gpio.h>
#include <uapi/drm/drm_fourcc.h>
#include <video/sunxi_metadata.h>

#if IS_ENABLED(CONFIG_EXTCON)
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include "../drivers/extcon/extcon.h"
#endif

#include <media/cec.h>
#include <media/cec-notifier.h>

#include "sunxi_device/sunxi_hdmi.h"
#include "sunxi_device/sunxi_tcon.h"

#include "sunxi_drm_drv.h"
#include "sunxi_drm_intf.h"
#include "sunxi_drm_crtc.h"

#if IS_ENABLED(CONFIG_AW_SMC)
extern int sunxi_smc_refresh_hdcp(void);
#endif

#define SUNXI_HDMI_EDID_LENGTH		(1024)
#define SUNXI_HDMI_POWER_CNT	     4
#define SUNXI_HDMI_POWER_NAME	     40

#define SUNXI_HDMI_HPD_FORCE_IN		(0x11)
#define SUNXI_HDMI_HPD_FORCE_OUT	(0x10)
#define SUNXI_HDMI_HPD_MASK_NOTIFY	(0x100)
#define SUNXI_HDMI_HPD_MASK_DETECT	(0x1000)

static char *_shdmi_color_mode_string[] = {
	"RGB888_8BITS",  "YUV444_8BITS",  "YUV422_8BITS",  "YUV420_8BITS",
	"RGB888_10BITS", "YUV444_10BITS", "YUV422_10BITS", "YUV420_10BITS",
	"RGB888_12BITS", "YUV444_12BITS", "YUV422_12BITS", "YUV420_12BITS",
	"RGB888_16BITS", "YUV444_16BITS", "YUV422_16BITS", "YUV420_16BITS",
};

static const struct drm_prop_enum_list shdmi_prop_list_pxfmt[] = {
	{ SHDMI_RGB888_8BITS,  "RGB888_8BITS"  },
	{ SHDMI_YUV444_8BITS,  "YUV444_8BITS"  },
	{ SHDMI_YUV422_8BITS,  "YUV422_8BITS"  },
	{ SHDMI_YUV420_8BITS,  "YUV420_8BITS"  },
	{ SHDMI_RGB888_10BITS, "RGB888_10BITS" },
	{ SHDMI_YUV444_10BITS, "YUV444_10BITS" },
	{ SHDMI_YUV422_10BITS, "YUV422_10BITS" },
	{ SHDMI_YUV420_10BITS, "YUV420_10BITS" },
	{ SHDMI_RGB888_12BITS, "RGB888_12BITS" },
	{ SHDMI_YUV444_12BITS, "YUV444_12BITS" },
	{ SHDMI_YUV422_12BITS, "YUV422_12BITS" },
	{ SHDMI_YUV420_12BITS, "YUV420_12BITS" },
	{ SHDMI_RGB888_16BITS, "RGB888_16BITS" },
	{ SHDMI_YUV444_16BITS, "YUV444_16BITS" },
	{ SHDMI_YUV422_16BITS, "YUV422_16BITS" },
	{ SHDMI_YUV420_16BITS, "YUV420_16BITS" },
};

static const struct drm_prop_enum_list shdmi_prop_list_dr[] = {
	{ SHDMI_SDR,    "SDR"    },
	{ SHDMI_HDR10,  "HDR10"  },
	{ SHDMI_HDR10P, "HDR10P" },
	{ SHDMI_HLG,    "HLG"    },
};

#if IS_ENABLED(CONFIG_EXTCON)
static const unsigned int sunxi_hdmi_cable[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};
#endif

enum sunxi_hdmi_reg_bank_e {
	SHDMI_REG_BANK_CTRL    = 0,
	SHDMI_REG_BANK_PHY     = 1,
	SHDMI_REG_BANK_SCDC    = 2,
	SHDMI_REG_BANK_HPI     = 3
};

struct sunxi_hdmi_res_s {
	struct clk  *clk_hdmi;
	struct clk  *clk_hdmi_24M;
	struct clk  *clk_hdmi_bus;
	struct clk  *clk_hdmi_ddc;
	struct clk  *clk_tcon_tv;
	struct clk  *clk_hdcp;
	struct clk  *clk_hdcp_bus;
	struct clk  *clk_cec;
	struct clk  *clk_cec_parent;
	struct reset_control  *rst_bus_sub;
	struct reset_control  *rst_bus_main;
	struct reset_control  *rst_bus_hdcp;

	char  power_name[SUNXI_HDMI_POWER_CNT][SUNXI_HDMI_POWER_NAME];
	struct regulator  *hdmi_regu[SUNXI_HDMI_POWER_CNT];
};

struct sunxi_hdmi_ctrl_s {
	int drm_enable;
	int drm_mode_set;
	int drm_hpd_force;

	int drv_clock;
	int drv_enable;
	int drv_hpd_state;
	int drv_hpd_mask;
	int drv_boot_enable;
	int drv_sw_enable;
	int drv_res_enable;
	int drv_need_flush;

	/* dts control value */
	unsigned int drv_dts_cec;
	unsigned int drv_dts_hdcp1x;
	unsigned int drv_dts_hdcp2x;
	unsigned int drv_dts_power_cnt;
	unsigned int drv_dts_clk_src;
	unsigned int drv_dts_ddc_index;
	unsigned int drv_dts_res_src;

	/* hdcp control state */
	int drv_hdcp_clock;
	int drv_hdcp_state;
	int drv_hdcp_enable;
	int drv_hdcp_support;
	sunxi_hdcp_type_t drv_hdcp_type;

	int drv_pm_state; /* 1: suspend. 0:resume */

	/* prop control value */
	u32 drv_pixel_format_cap;
	u32 drv_pixel_format;
	u32 drv_dynamic_range_cap;
	u32 drv_dynamic_range;

	/* cec control state */
	int drv_cec_clock;

	int drv_reg_bank;
	enum disp_data_bits   drv_max_bits;

	/* edid control */
	u8	drv_edid_dbg_mode;
	u8	drv_edid_dbg_data[SUNXI_HDMI_EDID_LENGTH];
	u8	drv_edid_dbg_size;
	struct mutex	drv_edid_lock;
	struct edid    *drv_edid_data;
};

struct sunxi_hdmi_cec_s {
	struct cec_notifier		*notify;
	struct cec_adapter		*adap;
	struct cec_msg          rx_msg;

	int    irq;
	bool   tx_done;
	bool   rx_done;
	u8     tx_status;
	u8     enable;
	u16    logic_addr;
};

struct sunxi_drm_hdmi {
	/* drm related members */
	struct device            *dev;
	struct sunxi_drm_device   sdrm;
	struct drm_display_mode   drm_mode;
	struct drm_display_mode   drm_mode_adjust;
	struct i2c_adapter        i2c_adap;
	/* sysfs device */
	dev_t                     hdmi_devid;
	struct class             *hdmi_class;
	struct device            *hdmi_dev;

	/* drm property */
	struct drm_property		*prop_pxfmt_cap; /* pixel format capality */
	struct drm_property		*prop_pxfmt;     /* pixel format */
	struct drm_property		*prop_dr_cap;    /* dynamic range capality */
	struct drm_property		*prop_dr;        /* dynamic range */

	int hdmi_irq;

	/* hdmi hpd work */
	struct task_struct   *hpd_task;
#if IS_ENABLED(CONFIG_EXTCON)
	struct extcon_dev    *hpd_extcon;
#endif

	struct sunxi_hdmi_cec_s        hdmi_cec;

	struct sunxi_hdmi_ctrl_s       hdmi_ctrl;

	struct sunxi_hdmi_res_s        hdmi_res;

	struct disp_device_config      disp_config;
	struct disp_video_timings      disp_timing;

	/* suxni hdmi core */
	struct sunxi_hdmi_s            hdmi_core;
};

static const struct drm_display_mode _sunxi_hdmi_default_modes[] = {
	/* 4 - 1280x720@60Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 16 - 1920x1080@60Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 31 - 1920x1080@50Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 19 - 1280x720@50Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 17 - 720x576@50Hz 4:3 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 2 - 720x480@60Hz 4:3 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
};
/*******************************************************************************
 * drm sunxi hdmi encoder and connector container_of
 ******************************************************************************/
static inline void hdmi_msleep(u8 ms)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(ms));
}

static inline struct sunxi_drm_hdmi *
drm_connector_to_hdmi(struct drm_connector *connector)
{
	struct sunxi_drm_device *sdrm =
			container_of(connector, struct sunxi_drm_device, connector);

	return container_of(sdrm, struct sunxi_drm_hdmi, sdrm);
}

static inline struct sunxi_drm_hdmi *
drm_encoder_to_hdmi(struct drm_encoder *encoder)
{
	struct sunxi_drm_device *sdrm =
			container_of(encoder, struct sunxi_drm_device, encoder);

	return container_of(sdrm, struct sunxi_drm_hdmi, sdrm);
}

static inline bool _shdmi_is_match(int a, int b)
{
	return (a == b) ? true : false;
}

static inline bool _shdmi_only_set(u32 data, u32 bit)
{
	return ((data & bit) == data) ? true : false;
}

static inline bool _shdmi_is_rgb_yuv4448(u32 mode)
{
	return ((mode == SHDMI_RGB888_8BITS) ||
			(mode == SHDMI_YUV444_8BITS)) ? true : false;
}

static inline u32 _shdmi_color_mode_blend(u32 format, u32 bits)
{
	return format + (bits * 4);
}

static inline u8 _shdmi_unblend_color_format(u32 mode)
{
	return (mode % 4);
}

static inline u8 _shdmi_unblend_color_depth(u32 mode)
{
	return (mode / 4);
}

/*******************************************************************************
 * sunxi hdmi driver hdcp function
 ******************************************************************************/
static int _sunxi_drv_hdcp_clock_on(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (pctl->drv_dts_hdcp2x == 0x0) {
		hdmi_trace("hdcp drv hdcp2x dts is disable\n");
		return 0;
	}

	if (pctl->drv_hdcp_clock) {
		hdmi_trace("hdcp drv clock has been enable\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_hdcp)) {
		hdmi_trace("hdcp drv bus gating enable\n");
		reset_control_deassert(pclk->rst_bus_hdcp);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdcp_bus)) {
		hdmi_trace("hdcp drv bus clock enable\n");
		clk_prepare_enable(pclk->clk_hdcp_bus);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdcp)) {
		hdmi_trace("hdcp drv clock enable\n");
		clk_prepare_enable(pclk->clk_hdcp);
	}

	hdmi_trace("hdcp drv clock all enable done\n");
	pctl->drv_hdcp_clock = 0x1;
	return 0;
}

static int _sunxi_drv_hdcp_clock_off(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (!pctl->drv_hdcp_clock) {
		hdmi_trace("hdcp drv clock has been disable\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdcp)) {
		hdmi_trace("hdcp drv clock disable\n");
		clk_disable_unprepare(pclk->clk_hdcp);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdcp_bus)) {
		hdmi_trace("hdcp drv bus clock disable\n");
		clk_disable_unprepare(pclk->clk_hdcp_bus);
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_hdcp)) {
		hdmi_trace("hdcp drv bus gating disable\n");
		reset_control_assert(pclk->rst_bus_hdcp);
	}

	hdmi_trace("hdcp drv clock all disable done\n");
	pctl->drv_hdcp_clock = 0x0;
	return 0;
}

static int _sunxi_drv_hdcp_loading_fw(struct device *dev)
{
	char fw_name[36] = "esm.fex";
	const struct firmware *fw;
	int ret = 0;

	ret = request_firmware_direct(&fw, (const char *)fw_name, dev);
	if (ret < 0) {
		ret = 0;
		goto exit;
	}

	if (fw->size && fw->data) {
		ret = sunxi_hdcp2x_fw_loading(fw->data, fw->size);
		if (ret == 0) {
			/* esm fw loading success */
			ret = fw->size;
			goto exit;
		}
	}
	ret = 0;

exit:
	release_firmware(fw);
	return ret;
}

static int _sunxi_drv_hdcp_get_state(struct sunxi_drm_hdmi *hdmi)
{
	int ret = SUNXI_HDCP_DISABLE;

	if (hdmi->hdmi_ctrl.drv_hdcp_enable)
		ret = sunxi_hdcp_get_state();

	return ret;
}

static void _sunxi_drv_hdcp_state_polling(struct sunxi_drm_hdmi *hdmi)
{
	const char *state[] = {"disable", "processing", "failed", "success"};
	int ret = SUNXI_HDCP_DISABLE;

	/* check hpd plugin */
	if (!hdmi->hdmi_ctrl.drv_hpd_state)
		return;

	ret = _sunxi_drv_hdcp_get_state(hdmi);

	/* check hdcp state change */
	if (hdmi->hdmi_ctrl.drv_hdcp_state == ret)
		return;

	hdmi_inf("hdmi drv get hdcp state [%s] change to [%s]\n",
		state[hdmi->hdmi_ctrl.drv_hdcp_state], state[ret]);
	hdmi->hdmi_ctrl.drv_hdcp_state = ret;
}

static void _sunxi_drv_hdcp_update_support(struct sunxi_drm_hdmi *hdmi)
{
	u8 ret = 0;

	/* check tx dts state */
	if (hdmi->hdmi_ctrl.drv_dts_hdcp1x && sunxi_hdcp1x_get_sink_cap())
		ret |= BIT(SUNXI_HDCP_TYPE_HDCP14);

	if (hdmi->hdmi_ctrl.drv_dts_hdcp2x && sunxi_hdcp2x_get_sink_cap() &&
			sunxi_hdcp2x_fw_state())
		ret |= BIT(SUNXI_HDCP_TYPE_HDCP22);

	hdmi->hdmi_ctrl.drv_hdcp_support = ret;
	hdmi_trace("hdmi drv update hdcp support: %d\n", ret);
}

static void _sunxi_drv_hdcp_release(struct sunxi_drm_hdmi *hdmi)
{
	hdmi->hdmi_ctrl.drv_hdcp_support = 0x0;
	hdmi->hdmi_ctrl.drv_hdcp_type    = SUNXI_HDCP_TYPE_NULL;
}

static void _sunxi_drv_hdcp_disable(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;

	switch (hdmi->hdmi_ctrl.drv_hdcp_type) {
	case SUNXI_HDCP_TYPE_HDCP22:
		ret = sunxi_hdcp2x_config(0x0);
		hdmi_inf("hdmi drv disable hdcp2x %s\n", ret != 0 ? "failed" : "success");
		break;
	case SUNXI_HDCP_TYPE_HDCP14:
		ret = sunxi_hdcp1x_config(0x0);
		hdmi_inf("hdmi drv disable hdcp1x %s\n", ret != 0 ? "failed" : "success");
		break;
	case SUNXI_HDCP_TYPE_NULL:
	default:
		break;
	}

	hdmi->hdmi_ctrl.drv_hdcp_type  = SUNXI_HDCP_TYPE_NULL;
	hdmi->hdmi_ctrl.drv_hdcp_state = SUNXI_HDCP_DISABLE;
}

/**
 * @desc: sunxi enable hdcp
 * if HDCP2x is supported, HDCP2x is used first.
 * otherwise HDCP1x is used.
 */
static int _sunxi_drv_hdcp_enable(struct sunxi_drm_hdmi *hdmi)
{
	u8 ret = 0, cap = 0;

	cap = hdmi->hdmi_ctrl.drv_hdcp_support;
	if (cap == 0) {
		_sunxi_drv_hdcp_release(hdmi);
		hdmi_inf("hdmi check unsupport hdcp\n");
		return 0;
	}

	if (hdmi->hdmi_ctrl.drv_hdcp_type != SUNXI_HDCP_TYPE_NULL)
		return 0;

	/* check and config hdcp2x */
	if (cap & BIT(SUNXI_HDCP_TYPE_HDCP22)) {
		ret = sunxi_hdcp2x_config(0x1);
		if (ret != 0x0) {
			if (cap & BIT(SUNXI_HDCP_TYPE_HDCP14)) {
				hdmi_inf("hdmi drv set hdcp2x failed and auto change to hdcp1x\n");
				goto config_hdcp1x;
			}
			hdmi_inf("hdmi drv only set hdcp2x but failed\n");
			return -1;
		}

		hdmi->hdmi_ctrl.drv_hdcp_type = SUNXI_HDCP_TYPE_HDCP22;
		hdmi_inf("hdmi drv enable hdcp22 done\n");
		return 0;
	}

config_hdcp1x:
	/* check and config hdcp1x */
	if (cap & BIT(SUNXI_HDCP_TYPE_HDCP14)) {
		ret = sunxi_hdcp1x_config(0x1);
		if (ret != 0) {
			hdmi_err("hdmi drv enable hdcp14 failed\n");
			return -1;
		}
		hdmi->hdmi_ctrl.drv_hdcp_type = SUNXI_HDCP_TYPE_HDCP14;
		hdmi_inf("hdmi drv enable hdcp14 done\n");
		return 0;
	}

	hdmi_inf("hdmi drv not-set hdcp\n");
	return 0;
}

#if IS_ENABLED(CONFIG_AW_DRM_HDMI20)
/*******************************************************************************
 * sunxi hdmi snd function
 ******************************************************************************/
/**
 * @desc: sound hdmi audio enable
 * @enable: 1 - enable hdmi audio
 *          0 - disable hdmi audio
 * @channel:
 * @return: 0 - success
 *         -1 - failed
 */
static s32 _sunxi_drv_audio_enable(u8 enable, u8 channel)
{
	int ret = 0;

	if (enable)
		ret = sunxi_hdmi_audio_enable();

	hdmi_inf("hdmi drv audio set %s %s\n",
		enable ? "enable" : "disable", ret ? "failed" : "done");
	return 0;
}

/**
 * @desc: sound hdmi audio param config
 * @audio_para: audio params
 * @return: 0 - success
 *         -1 - failed
 */
static s32 _sunxi_drv_audio_set_info(hdmi_audio_t *audio_para)
{
	int ret = 0;

	ret = sunxi_hdmi_audio_set_info(audio_para);
	hdmi_inf("hdmi drv audio set info %s\n", ret ? "failed" : "done");

	return 0;
}

int snd_hdmi_get_func(__audio_hdmi_func *func)
{
	if (IS_ERR_OR_NULL(func)) {
		shdmi_err(func);
		return -1;
	}

	func->hdmi_audio_enable   = _sunxi_drv_audio_enable;
	func->hdmi_set_audio_para = _sunxi_drv_audio_set_info;
	return 0;
}
EXPORT_SYMBOL_GPL(snd_hdmi_get_func);
#endif

static int _sunxi_drv_hdmi_regulator_on(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0, loop = 0;

	for (loop = 0; loop < hdmi->hdmi_ctrl.drv_dts_power_cnt; loop++) {
		if (!hdmi->hdmi_res.hdmi_regu[loop])
			continue;

		ret = regulator_enable(hdmi->hdmi_res.hdmi_regu[loop]);
		hdmi_trace("hdmi drv enable regulator %s %s\n",
			hdmi->hdmi_res.power_name[loop], ret != 0 ? "failed" : "success");
	}
	return 0;
}

static int _sunxi_drv_hdmi_regulator_off(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0, loop = 0;

	for (loop = 0; loop < hdmi->hdmi_ctrl.drv_dts_power_cnt; loop++) {
		if (!hdmi->hdmi_res.hdmi_regu[loop])
			continue;
		ret = regulator_disable(hdmi->hdmi_res.hdmi_regu[loop]);
		hdmi_trace("hdmi drv disable regulator %s %s\n",
			hdmi->hdmi_res.power_name[loop], ret != 0 ? "failed" : "success");
	}
	return 0;
}

static int _sunxi_drv_hdmi_clock_on(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (pctl->drv_clock) {
		hdmi_trace("hdmi drv clock has been enable\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_main)) {
		hdmi_trace("hdmi drv main bus gating enable\n");
		reset_control_deassert(pclk->rst_bus_main);
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_sub)) {
		hdmi_trace("hdmi drv sub bus gating enable\n");
		reset_control_deassert(pclk->rst_bus_sub);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi_24M)) {
		hdmi_trace("hdmi drv clock 24M enable\n");
		clk_prepare_enable(pclk->clk_hdmi_24M);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi)) {
		hdmi_trace("hdmi drv clock enable\n");
		clk_prepare_enable(pclk->clk_hdmi);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi_ddc)) {
		hdmi_trace("hdmi drv ddc clock enable\n");
		clk_prepare_enable(pclk->clk_hdmi_ddc);
	}

	/* enable hdcp clock */
	_sunxi_drv_hdcp_clock_on(hdmi);

	hdmi_trace("hdmi drv all clock enable done\n");
	pctl->drv_clock = 0x1;
	return 0;
}

static int _sunxi_drv_hdmi_clock_off(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (!pctl->drv_clock) {
		hdmi_trace("hdmi drv clock has been disable\n");
		return 0;
	}

	/* disable hdcp clock */
	_sunxi_drv_hdcp_clock_off(hdmi);

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi_ddc)) {
		hdmi_trace("hdmi drv ddc clock disable\n");
		clk_disable_unprepare(pclk->clk_hdmi_ddc);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi)) {
		hdmi_trace("hdmi drv clock disable\n");
		clk_disable_unprepare(pclk->clk_hdmi);
	}

	if (!IS_ERR_OR_NULL(pclk->clk_hdmi_24M)) {
		hdmi_trace("hdmi drv clock 24M disable\n");
		clk_disable_unprepare(pclk->clk_hdmi_24M);
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_sub)) {
		hdmi_trace("hdmi drv sub bus gating disable\n");
		reset_control_assert(pclk->rst_bus_sub);
	}

	if (!IS_ERR_OR_NULL(pclk->rst_bus_main)) {
		hdmi_trace("hdmi drv main bus gating disable\n");
		reset_control_assert(pclk->rst_bus_main);
	}

	hdmi_trace("hdmi drv all clock disable done\n");
	pctl->drv_clock = 0x0;
	return 0;
}

static int _sunxi_drv_hdmi_set_resource(struct sunxi_drm_hdmi *hdmi, u8 state)
{
	struct device *tcon_dev = NULL;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (hdmi->hdmi_ctrl.drv_res_enable == state) {
		hdmi_inf("hdmi drv resource has been %s\n",
				state ? "enable" : "disable");
		return 0;
	}

	tcon_dev = hdmi->sdrm.tcon_dev;
	if (IS_ERR_OR_NULL(tcon_dev)) {
		shdmi_err(tcon_dev);
		return -1;
	}

	/* hdmi resource enable */
	if (state == SUNXI_HDMI_ENABLE) {
		sunxi_tcon_hdmi_open(tcon_dev, hdmi->hdmi_ctrl.drv_dts_clk_src);
		_sunxi_drv_hdmi_regulator_on(hdmi);
		_sunxi_drv_hdmi_clock_on(hdmi);
	} else {
		_sunxi_drv_hdmi_clock_off(hdmi);
		_sunxi_drv_hdmi_regulator_off(hdmi);
		sunxi_tcon_hdmi_close(tcon_dev);
	}

	hdmi->hdmi_ctrl.drv_res_enable = state;
	hdmi_inf("hdmi drv %s resource done\n", state ? "enable" : "disable");
	return 0;
}

static int _sunxi_drv_hdmi_read_edid(struct sunxi_drm_hdmi *hdmi)
{
	int ret = -1;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	mutex_lock(&hdmi->hdmi_ctrl.drv_edid_lock);

	if (hdmi->hdmi_ctrl.drv_edid_data != NULL) {
		ret = 0;
		goto exit;
	}

	if (hdmi->hdmi_ctrl.drv_edid_dbg_mode) {
		hdmi_inf("hdmi drv use debug edid\n");
		hdmi->hdmi_ctrl.drv_edid_data = (struct edid *)&hdmi->hdmi_ctrl.drv_edid_dbg_data;
		goto edid_parse;
	}

	hdmi->hdmi_ctrl.drv_edid_data = NULL;
	hdmi->hdmi_ctrl.drv_edid_data = drm_get_edid(&hdmi->sdrm.connector, &hdmi->i2c_adap);
	if (IS_ERR_OR_NULL(hdmi->hdmi_ctrl.drv_edid_data)) {
		hdmi_err("hdmi drv i2c read edid failed\n");
		hdmi->hdmi_ctrl.drv_edid_data = NULL;
		ret = -1;
		goto exit;
	}

edid_parse:
	sunxi_hdmi_edid_parse((u8 *)hdmi->hdmi_ctrl.drv_edid_data);
	ret = 0;

exit:
	mutex_unlock(&hdmi->hdmi_ctrl.drv_edid_lock);
	return ret;
}

static int _sunxi_drv_hdmi_hpd_get(struct sunxi_drm_hdmi *hdmi)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return 0;
	}

	return hdmi->hdmi_ctrl.drv_hpd_state ? 0x1 : 0x0;
}

static int _sunxi_drv_hdmi_hpd_set(struct sunxi_drm_hdmi *hdmi, u8 state)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	hdmi->hdmi_ctrl.drv_hpd_state = state;

	if ((hdmi->hdmi_ctrl.drv_hpd_mask & SUNXI_HDMI_HPD_MASK_NOTIFY) ==
			SUNXI_HDMI_HPD_MASK_NOTIFY)
		return 0;

#if IS_ENABLED(CONFIG_EXTCON)
	if (hdmi->hpd_extcon)
		extcon_set_state_sync(hdmi->hpd_extcon, EXTCON_DISP_HDMI,
			state ? true : false);
#endif

	if (!IS_ERR_OR_NULL(hdmi->sdrm.drm_dev))
		drm_helper_hpd_irq_event(hdmi->sdrm.drm_dev);

	return 0;
}

static int
__sunxi_drv_hdmi_set_dynamic_range(struct sunxi_drm_hdmi *hdmi, uint64_t val)
{
	struct disp_device_config *info = &hdmi->disp_config;
	int ret = 0;

	ret = (val & hdmi->hdmi_ctrl.drv_dynamic_range_cap);
	if (ret == 0) {
		hdmi_err("hdmi drv set dynamic range %d unsupport!\n", (u32)val);
		return -1;
	}

	hdmi->hdmi_ctrl.drv_dynamic_range = val;

	if (hdmi->hdmi_ctrl.drv_dynamic_range == BIT(SHDMI_HDR10))
		info->eotf = DISP_EOTF_SMPTE2084;
	else if (hdmi->hdmi_ctrl.drv_dynamic_range == BIT(SHDMI_HLG))
		info->eotf = DISP_EOTF_ARIB_STD_B67;
	else
		info->eotf = DISP_EOTF_GAMMA22;

	return 0;
}

static int
__sunxi_drv_hdmi_get_dynamic_range(struct sunxi_drm_hdmi *hdmi)
{
	struct disp_device_config *info = &hdmi->disp_config;

	if (info->eotf == DISP_EOTF_SMPTE2084)
		hdmi->hdmi_ctrl.drv_dynamic_range = BIT(SHDMI_HDR10);
	else if (info->eotf == DISP_EOTF_ARIB_STD_B67)
		hdmi->hdmi_ctrl.drv_dynamic_range = BIT(SHDMI_HLG);
	else
		hdmi->hdmi_ctrl.drv_dynamic_range = BIT(SHDMI_SDR);

	return 0;
}

static int
__sunxi_drv_hdmi_get_dynamic_range_cap(struct sunxi_drm_hdmi *hdmi)
{
	u32 dr_cap = sunxi_hdmi_get_support_hdr_mode();
	u32 dr = hdmi->hdmi_ctrl.drv_dynamic_range;

	hdmi->hdmi_ctrl.drv_dynamic_range_cap = dr_cap;

	if (!(dr & dr_cap))
		hdmi_trace("hdmi drv check old dynamic range %d unsupport!", dr);

	return 0;
}

static int __sunxi_drv_hdmi_get_pxfmt(struct sunxi_drm_hdmi *hdmi)
{
	struct disp_device_config *info = sunxi_hdmi_get_disp_info();
	u32 bit_map = _shdmi_color_mode_blend(info->format, info->bits);

	hdmi->hdmi_ctrl.drv_pixel_format = BIT(bit_map);
	return 0;
}

static int __sunxi_drv_hdmi_set_pxfmt(struct sunxi_drm_hdmi *hdmi, uint64_t val)
{
	struct disp_device_config *info = &hdmi->disp_config;
	u32 map_bit = ffs(val) - 1;

	if (!(val & hdmi->hdmi_ctrl.drv_pixel_format_cap)) {
		hdmi_trace("hdmi drv check set color mode: %s unsupport.\n",
		_shdmi_color_mode_string[map_bit]);
		return -1;
	}

	info->format = (enum disp_csc_type)_shdmi_unblend_color_format(map_bit);
	info->bits   = (enum disp_data_bits)_shdmi_unblend_color_depth(map_bit);

	hdmi_trace("hdmi drv set color mode: %s\n", _shdmi_color_mode_string[map_bit]);
	return 0;
}

static int __sunxi_drv_hdmi_get_pxfmt_cap(struct sunxi_drm_hdmi *hdmi)
{
	int i = 0, ret = 0;
	u8 format = 0, bits = 0;
	u32 pixel_clk = 0, color_cap = 0, vic = 0;
	enum disp_data_bits max_bits = hdmi->hdmi_ctrl.drv_max_bits;

	if (!_sunxi_drv_hdmi_hpd_get(hdmi)) {
		color_cap = 0;
		goto exit_none;
	}

	pixel_clk = (u32)hdmi->drm_mode.clock;
	if (pixel_clk <= 1000)
		goto exit_none;

	vic = (u32)drm_match_cea_mode(&hdmi->drm_mode);
	color_cap = sunxi_hdmi_get_color_capality(vic);

	if (max_bits == DISP_DATA_8BITS)
		color_cap &= 0x000F;
	else if (max_bits == DISP_DATA_10BITS)
		color_cap &= 0x00FF;
	else if (max_bits == DISP_DATA_12BITS)
		color_cap &= 0x0FFF;
	else if (max_bits == DISP_DATA_16BITS)
		color_cap &= 0xFFFF;

	for (i = 0; i < SHDMI_MAX_MASK; i++) {
		if ((color_cap & BIT(i)) == 0)
			continue;
		format = _shdmi_unblend_color_format(i);
		bits   = _shdmi_unblend_color_depth(i);
		ret = sunxi_hdmi_video_check_tmds_clock(format, bits, pixel_clk);
		if (ret == 0) {
			hdmi_inf("hdmi drv remove vic[%d] %s when clock to max\n",
				vic, _shdmi_color_mode_string[i]);
			color_cap &= ~BIT(i);
		}
	}

exit_none:
	hdmi->hdmi_ctrl.drv_pixel_format_cap = color_cap;
	return 0;
}

/*******************************************************************************
 * drm sunxi hdmi driver functions
 ******************************************************************************/
static u32 __sunxi_drv_hdmi_disp_is_change(struct disp_device_config *old_info,
		struct disp_device_config *new_info)
{
	unsigned long update = 0;

	if (!_shdmi_is_match(old_info->format, new_info->format))
		test_and_set_bit(SUNXI_HDMI_UPDATE_FAORMAT, &update);

	if (!_shdmi_is_match(old_info->bits, new_info->bits))
		test_and_set_bit(SUNXI_HDMI_UPDATE_BITS, &update);

	if (!_shdmi_is_match(old_info->eotf, new_info->eotf))
		test_and_set_bit(SUNXI_HDMI_UPDATE_EOTF, &update);

	if (!_shdmi_is_match(old_info->cs, new_info->cs))
		test_and_set_bit(SUNXI_HDMI_UPDATE_SPACE, &update);

	if (!_shdmi_is_match(old_info->dvi_hdmi, new_info->dvi_hdmi))
		test_and_set_bit(SUNXI_HDMI_UPDATE_DVIHDMI, &update);

	if (!_shdmi_is_match(old_info->range, new_info->range))
		test_and_set_bit(SUNXI_HDMI_UPDATE_RANGE, &update);

	if (!_shdmi_is_match(old_info->scan, new_info->scan))
		test_and_set_bit(SUNXI_HDMI_UPDATE_SCAN, &update);

	if (!_shdmi_is_match(old_info->aspect_ratio, new_info->aspect_ratio))
		test_and_set_bit(SUNXI_HDMI_UPDATE_RATIO, &update);

	hdmi_trace("hdmi drv check change info: 0x%x\n", (u32)update);
	return (u32)update;
}

/**
 * @desc: check current config display info need change or flush.
 * @return: -1 - check failed
 *           0 - not change
 *           1 - need flush
 *           2 - need mode change
 */
static int _sunxi_drv_hdmi_disp_info_check(struct sunxi_drm_hdmi *hdmi)
{
	struct disp_device_config *old_info = sunxi_hdmi_get_disp_info();
	struct disp_device_config *new_info = &hdmi->disp_config;
	u32 old_state, new_state, change_bit, mask_bit;
	bool flush_flag = false;

	change_bit = __sunxi_drv_hdmi_disp_is_change(old_info, new_info);
	if (change_bit == 0)
		goto not_change;

	/* if change bit include other bit.we will set mode change. */
	mask_bit  = SUNXI_HDMI_UPDATE_FAORMAT;
	mask_bit |= SUNXI_HDMI_UPDATE_BITS;
	mask_bit |= SUNXI_HDMI_UPDATE_DVIHDMI;
	if (!_shdmi_only_set((u32)change_bit, mask_bit)) {
		hdmi_trace("hdmi drv check need change when 0x%x\n", change_bit);
		goto need_change;
	}

	/* YUV422-8Bit <-> YUV422-10Bit <-> YUV422-12Bit */
	if ((old_info->format == DISP_CSC_TYPE_YUV422) &&
			(new_info->format == DISP_CSC_TYPE_YUV422))
		flush_flag = true;

	old_state = _shdmi_color_mode_blend(old_info->format, old_info->bits);
	new_state = _shdmi_color_mode_blend(new_info->format, new_info->bits);
	/* YUV444-8Bit <-> RGB-8Bit */
	if (_shdmi_is_rgb_yuv4448(old_state) && _shdmi_is_rgb_yuv4448(new_state))
		flush_flag = true;

	if (flush_flag) {
		hdmi_trace("hdmi drv check use flush when old[%d] - new[%d]\n",
				old_state, new_state);
		hdmi->hdmi_ctrl.drv_need_flush = 0x1;
		goto need_flush;
	}

	/* default is change */
need_change:
	return 2;
need_flush:
	return 1;
not_change:
	return 0;
}

static int _sunxi_drv_hdmi_set_rate(struct sunxi_hdmi_res_s *p_clk)
{
	unsigned long clk_rate = 0;

	if (IS_ERR_OR_NULL(p_clk)) {
		shdmi_err(p_clk);
		return -1;
	}

	clk_rate = clk_get_rate(p_clk->clk_tcon_tv);
	if (clk_rate == 0) {
		hdmi_err("tcon clock rate is 0");
		return -1;
	}

	clk_set_rate(p_clk->clk_hdmi, clk_rate);
	return 0;
}

static int _sunxi_drv_hdmi_smooth_enable(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;

	if (hdmi->hdmi_ctrl.drv_enable) {
		hdmi_inf("hdmi drv has been enable!\n");
		return 0;
	}

	ret = sunxi_hdmi_set_disp_info(&hdmi->disp_config);
	if (ret != 0) {
		hdmi_err("hdmi drv smooth enable set disp info failed\n");
		return -1;
	}

	ret = sunxi_hdmi_smooth_config();
	if (ret != 0) {
		hdmi_err("hdmi drv smooth config failed\n");
		return -1;
	}

	hdmi->hdmi_ctrl.drv_enable = 0x1;
	return 0;
}

static int _sunxi_drv_hdmi_enable(struct sunxi_drm_hdmi *hdmi)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (hdmi->hdmi_ctrl.drv_enable) {
		hdmi_inf("hdmi drv has been enable!\n");
		return 0;
	}

	_sunxi_drv_hdmi_set_rate(&hdmi->hdmi_res);

	/* hdmi driver video ops enable */
	sunxi_hdmi_config();

	if (hdmi->hdmi_ctrl.drv_hdcp_enable)
		_sunxi_drv_hdcp_enable(hdmi);

	hdmi->hdmi_ctrl.drv_enable = 0x1;
	hdmi_inf("hdmi drv enable output done\n");
	return 0;
}

static int _sunxi_drv_hdmi_disable(struct sunxi_drm_hdmi *hdmi)
{
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (!hdmi->hdmi_ctrl.drv_enable) {
		hdmi_inf("hdmi drv has been disable!\n");
		return 0;
	}

	_sunxi_drv_hdcp_disable(hdmi);

	sunxi_hdmi_disconfig();

	hdmi->hdmi_ctrl.drv_enable = 0x0;
	hdmi_trace("hdmi drv disable done\n");
	return 0;
}

static bool _shdmi_cb_check_status(void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;

	return sunxi_tcon_check_fifo_status(hdmi->sdrm.tcon_dev);
}

static void _shdmi_cb_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;

	sunxi_tcon_enable_vblank(hdmi->sdrm.tcon_dev, enable);
}

static bool  _shdmi_cb_sync_time_enough(void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;

	return sunxi_tcon_is_sync_time_enough(hdmi->sdrm.tcon_dev);
}

static int _shdmi_cb_get_cur_line(void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;

	return sunxi_tcon_get_current_line(hdmi->sdrm.tcon_dev);
}

static void _shdmi_cb_atomic_flush(void *data)
{
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)data;
	int ret = 0;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return;
	}

	if (hdmi->hdmi_ctrl.drv_enable != 0x1)
		return;

	if (hdmi->hdmi_ctrl.drv_need_flush != 0x1)
		return;

	ret = sunxi_hdmi_set_disp_info(&hdmi->disp_config);
	if (ret != 0) {
		hdmi_err("hdmi drv flush set disp info failed\n");
		return;
	}

	ret = sunxi_hdmi_smooth_config();
	if (ret != 0) {
		hdmi_err("hdmi drv flush config failed\n");
		return;
	}

	hdmi->hdmi_ctrl.drv_need_flush = 0x0;
	hdmi_inf("hdmi drv atomic flush done\n");
}

static int _sunxi_drv_hdmi_filling_scrtc(struct sunxi_drm_hdmi *hdmi,
		struct sunxi_crtc_state *scrtc)
{
	struct disp_device_config *info = NULL;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (IS_ERR_OR_NULL(scrtc)) {
		shdmi_err(scrtc);
		return -1;
	}

	info = &hdmi->disp_config;

	/* convert disp format to de format */
	if (info->format == DISP_CSC_TYPE_YUV444) {
		scrtc->px_fmt_space = DE_FORMAT_SPACE_YUV;
		scrtc->yuv_sampling = DE_YUV444;
	} else if (info->format == DISP_CSC_TYPE_YUV422) {
		scrtc->px_fmt_space = DE_FORMAT_SPACE_YUV;
		scrtc->yuv_sampling = DE_YUV422;
	} else if (info->format == DISP_CSC_TYPE_YUV420) {
		scrtc->px_fmt_space = DE_FORMAT_SPACE_YUV;
		scrtc->yuv_sampling = DE_YUV420;
	} else {
		scrtc->px_fmt_space = DE_FORMAT_SPACE_RGB;
		scrtc->yuv_sampling = DE_YUV444;
	}

	/* convert disp bits to de bits */
	if (info->bits == DISP_DATA_10BITS)
		scrtc->data_bits = DE_DATA_10BITS;
	else if (info->bits == DISP_DATA_12BITS)
		scrtc->data_bits = DE_DATA_12BITS;
	else if (info->bits == DISP_DATA_16BITS)
		scrtc->data_bits = DE_DATA_16BITS;
	else
		scrtc->data_bits = DE_DATA_8BITS;

	/* convert disp eotf to de eotf */
	if (info->eotf == DISP_EOTF_SMPTE2084)
		scrtc->eotf = DE_EOTF_SMPTE2084;
	else if (info->eotf == DISP_EOTF_ARIB_STD_B67)
		scrtc->eotf = DE_EOTF_ARIB_STD_B67;
	else
		scrtc->eotf = DE_EOTF_GAMMA22;

	/* convert disp sapce to de sapce */
	if (info->cs == DISP_BT709)
		scrtc->color_space = DE_COLOR_SPACE_BT709;
	else if (info->cs == DISP_BT2020NC)
		scrtc->color_space = DE_COLOR_SPACE_BT2020NC;
	else
		scrtc->color_space = DE_COLOR_SPACE_BT601;

	/* convert disp range to de range */
	if (info->range == DISP_COLOR_RANGE_0_255)
		scrtc->color_range = DE_COLOR_RANGE_0_255;
	else if (info->range == DISP_COLOR_RANGE_16_235)
		scrtc->color_range = DE_COLOR_RANGE_16_235;
	else
		scrtc->color_range = DE_COLOR_RANGE_DEFAULT;

	scrtc->tcon_id             = hdmi->sdrm.tcon_id;
	scrtc->sw_enable           = hdmi->hdmi_ctrl.drv_sw_enable;
	scrtc->output_dev_data     = hdmi;
	scrtc->check_status        = _shdmi_cb_check_status;
	scrtc->enable_vblank       = _shdmi_cb_enable_vblank;
	scrtc->is_sync_time_enough = _shdmi_cb_sync_time_enough;
	scrtc->get_cur_line        = _shdmi_cb_get_cur_line;
	scrtc->atomic_flush        = _shdmi_cb_atomic_flush;

	return 0;
}

static int _sunxi_drv_hdmi_select_output(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;
	struct drm_display_info *c_info = &hdmi->sdrm.connector.display_info;
	struct disp_device_config *info = &hdmi->disp_config;
	u32 vic = (u32)drm_match_cea_mode(&hdmi->drm_mode_adjust);
	u32 pixel_clk = hdmi->drm_mode_adjust.clock;

	if (!c_info->is_hdmi) {
		info->dvi_hdmi = DISP_DVI;
		info->format = DISP_CSC_TYPE_RGB;
		info->bits   = DISP_DATA_8BITS;
		info->eotf   = DISP_EOTF_GAMMA22;
		info->cs     = DISP_BT709;
		hdmi_inf("hdmi drv select dvi output\n");
		goto check_clock;
	} else
		info->dvi_hdmi = DISP_HDMI;

	sunxi_hdmi_disp_select_eotf(info);

	sunxi_hdmi_disp_select_space(info, vic);

format_select:
	sunxi_hdmi_disp_select_format(info, vic);

check_clock:
	ret = sunxi_hdmi_video_check_tmds_clock(info->format, info->bits, pixel_clk);
	if (ret == 0x0) {
		/* 1. Reduce color depth */
		if ((info->bits < DISP_DATA_16BITS) &&
				(info->bits != DISP_DATA_8BITS)) {
			info->bits--;
			hdmi_inf("hdmi drv auto download bits: %s\n",
				sunxi_hdmi_color_depth_string(info->bits));
			goto format_select;
		}
		if ((info->format < DISP_CSC_TYPE_YUV420) &&
				(info->format != DISP_CSC_TYPE_YUV420)) {
			info->format++;
			hdmi_inf("hdmi drv auto download format: %s\n",
				sunxi_hdmi_color_format_string(info->format));
			goto format_select;
		}
		hdmi_inf("hdmi drv select output failed when clock overflow\n");
		return -1;
	}

	info->range = (info->format == DISP_CSC_TYPE_RGB) ?
			DISP_COLOR_RANGE_0_255 : DISP_COLOR_RANGE_16_235;
	info->scan  = DISP_SCANINFO_NO_DATA;
	info->aspect_ratio = HDMI_ACTIVE_ASPECT_PICTURE;

	return 0;
}

static int _sunxi_drv_hdmi_hpd_plugin(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;

	hdmi_inf("hdmi drv detect hpd connect\n");

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	ret = _sunxi_drv_hdmi_read_edid(hdmi);
	if (ret != 0)
		hdmi_err("hdmi drv plugin read edid failed\n");

	_sunxi_drv_hdcp_update_support(hdmi);

	if (hdmi->hdmi_ctrl.drv_hdcp_support & BIT(SUNXI_HDCP_TYPE_HDCP22))
		sunxi_hdcp_set_path(SUNXI_HDCP_TYPE_HDCP22);
	else
		sunxi_hdcp_set_path(SUNXI_HDCP_TYPE_HDCP14);

	return 0;
}

static int _sunxi_drv_hdmi_hpd_plugout(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;

	hdmi_inf("hdmi drv detect hpd disconnect\n");

	/* clean drm info */
	hdmi->hdmi_ctrl.drm_enable   = 0x0;
	hdmi->hdmi_ctrl.drm_mode_set = 0x0;
	memset(&hdmi->drm_mode, 0x0, sizeof(struct drm_display_mode));

	hdmi->hdmi_ctrl.drv_pixel_format_cap  = 0x0;
	hdmi->hdmi_ctrl.drv_dynamic_range_cap = 0x0;

	cec_notifier_phys_addr_invalidate(hdmi->hdmi_cec.notify);

	hdmi->hdmi_ctrl.drv_edid_data = NULL;
	ret = _sunxi_drv_hdmi_disable(hdmi);
	if (ret != 0)
		hdmi_err("sunxi hdmi driver disable failed when plugout.\n");

	return 0;
}

/**
 * @return: 0 - normal
 *          1 - hpd force mode
 *          2 - hpd bypass mode
 */
static int _sunxi_drv_hdmi_check_hpd_mask(struct sunxi_drm_hdmi *hdmi)
{
	if ((hdmi->hdmi_ctrl.drv_hpd_mask & SUNXI_HDMI_HPD_FORCE_IN) ==
			SUNXI_HDMI_HPD_FORCE_IN) {
		if (!_sunxi_drv_hdmi_hpd_get(hdmi)) {
			_sunxi_drv_hdmi_hpd_set(hdmi, 0x1);
			hdmi_inf("hdmi drv force mode set plugin\n");
			return 1;
		} else
			return 2;
	} else if ((hdmi->hdmi_ctrl.drv_hpd_mask & SUNXI_HDMI_HPD_FORCE_OUT) ==
			SUNXI_HDMI_HPD_FORCE_OUT) {
		if (_sunxi_drv_hdmi_hpd_get(hdmi)) {
			_sunxi_drv_hdmi_hpd_set(hdmi, 0x0);
			hdmi_inf("hdmi drv force mode set plugout\n");
			return 1;
		} else
			return 2;
	} else if ((hdmi->hdmi_ctrl.drv_hpd_mask & SUNXI_HDMI_HPD_MASK_DETECT) ==
			SUNXI_HDMI_HPD_MASK_DETECT)
		return 2;
	else
		return 0;
}

static int _sunxi_drv_hdmi_thread(void *parg)
{
	int temp_hpd = 0, ret = 0;
	struct sunxi_drm_hdmi *hdmi = (struct sunxi_drm_hdmi *)parg;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	while (1) {
		ret = kthread_should_stop();
		if (ret) {
			hdmi_wrn("hdmi drv hpd thread is stop!!!\n");
			break;
		}

		/* check software debug mode */
		ret = _sunxi_drv_hdmi_check_hpd_mask(hdmi);
		if (ret == 0x1) {
			temp_hpd = _sunxi_drv_hdmi_hpd_get(hdmi);
			goto handle_change;
		} else if (ret == 0x2)
			goto next_loop;

		/* check physical hpd state */
		temp_hpd = sunxi_hdmi_get_hpd();
		if (temp_hpd == _sunxi_drv_hdmi_hpd_get(hdmi))
			goto next_loop;

		mdelay(50);

		temp_hpd = sunxi_hdmi_get_hpd();
		if (temp_hpd == _sunxi_drv_hdmi_hpd_get(hdmi))
			goto next_loop;

handle_change:
		if (temp_hpd)
			_sunxi_drv_hdmi_hpd_plugin(hdmi);
		else
			_sunxi_drv_hdmi_hpd_plugout(hdmi);

		_sunxi_drv_hdmi_hpd_set(hdmi, temp_hpd);

next_loop:
		_sunxi_drv_hdcp_state_polling(hdmi);
		hdmi_msleep(20);
	}

	return 0;
}

/*******************************************************************************
 * @desc: sunxi hdmi driver pm function
 ******************************************************************************/
#ifdef CONFIG_PM_SLEEP
static int _sunxi_drv_hdmi_suspend(struct device *dev)
{
	int ret = 0;
	struct sunxi_drm_hdmi  *hdmi = dev_get_drvdata(dev);
	struct device *tcon_dev = NULL;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		goto exit;
	}

	if (hdmi->hpd_task) {
		kthread_stop(hdmi->hpd_task);
		hdmi->hpd_task = NULL;
	}

	ret = _sunxi_drv_hdmi_disable(hdmi);
	if (ret != 0) {
		hdmi_err("hdmi drv disable failed\n");
		goto suspend_exit;
	}

	tcon_dev = hdmi->sdrm.tcon_dev;
	if (IS_ERR_OR_NULL(tcon_dev)) {
		shdmi_err(tcon_dev);
		goto suspend_exit;
	}
	ret = sunxi_tcon_mode_exit(tcon_dev);
	if (ret != 0) {
		hdmi_err("hdmi tcon mode exit failed\n");
		goto suspend_exit;
	}

	_sunxi_drv_hdmi_set_resource(hdmi, SUNXI_HDMI_DISABLE);
	pm_runtime_put_sync(dev);

suspend_exit:
	hdmi->hdmi_ctrl.drv_edid_data = NULL;
	hdmi->hdmi_ctrl.drv_pm_state  = 0x1;
	hdmi_inf("hdmi drv pm suspend done\n");
exit:
	return 0;
}

static int _sunxi_drv_hdmi_resume(struct device *dev)
{
	struct sunxi_drm_hdmi  *hdmi = dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		goto exit;
	}

	if (!hdmi->hdmi_ctrl.drv_pm_state)
		goto resume_exit;

	pm_runtime_get_sync(hdmi->dev);
	_sunxi_drv_hdmi_set_resource(hdmi, SUNXI_HDMI_ENABLE);

	hdmi->hpd_task =
		kthread_create(_sunxi_drv_hdmi_thread, (void *)hdmi, "hdmi hpd");
	if (!hdmi->hpd_task) {
		hdmi_err("init thread handle hpd is failed!!!\n");
		hdmi->hpd_task = NULL;
		return -1;
	}
	wake_up_process(hdmi->hpd_task);

resume_exit:
	hdmi->hdmi_ctrl.drv_pm_state = 0x0;
	hdmi_inf("hdmi drv pm resume done\n");
exit:
	return 0;
}

static const struct dev_pm_ops sunxi_hdmi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(_sunxi_drv_hdmi_suspend,
				_sunxi_drv_hdmi_resume)
};
#endif

/*******************************************************************************
 * @desc: sunxi hdmi cec adapter function
 ******************************************************************************/
static int _sunxi_drv_cec_clock_on(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (!pctl->drv_dts_cec) {
		hdmi_trace("cec drv cec dts is disable\n");
		return 0;
	}

	if (pctl->drv_cec_clock) {
		hdmi_trace("cec drv clock has been enable\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(pclk->clk_cec)) {
		hdmi_trace("cec drv clock enable\n");
		clk_prepare_enable(pclk->clk_cec);
	}

	hdmi_inf("cec drv clock enable done\n");
	pctl->drv_cec_clock = 0x1;
	return 0;
}

static int _sunxi_drv_cec_clock_off(struct sunxi_drm_hdmi *hdmi)
{
	struct sunxi_hdmi_res_s  *pclk = &hdmi->hdmi_res;
	struct sunxi_hdmi_ctrl_s *pctl = &hdmi->hdmi_ctrl;

	if (!pctl->drv_dts_cec) {
		hdmi_trace("cec drv cec dts is disable\n");
		return 0;
	}

	if (!pctl->drv_cec_clock) {
		hdmi_trace("cec drv clock has been disable\n");
		return 0;
	}

	if (!IS_ERR_OR_NULL(pclk->clk_cec)) {
		hdmi_trace("cec drv clock disable\n");
		clk_disable_unprepare(pclk->clk_cec);
	}

	hdmi_inf("cec drv clock disable done\n");
	pctl->drv_cec_clock = 0x0;
	return 0;
}

static irqreturn_t _sunxi_drv_cec_hardirq(int irq, void *data)
{
	struct cec_adapter *adap = data;
	struct sunxi_drm_hdmi *hdmi = cec_get_drvdata(adap);
	int len;

	u8 stat = sunxi_cec_get_irq_state();

	if (stat == SUNXI_CEC_IRQ_NULL)
		goto none_exit;

	if (stat & SUNXI_CEC_IRQ_ERR_INITIATOR) {
		hdmi->hdmi_cec.tx_status = CEC_TX_STATUS_ERROR;
		hdmi->hdmi_cec.tx_done   = true;
		goto wake_exit;
	} else if (stat & SUNXI_CEC_IRQ_DONE) {
		hdmi->hdmi_cec.tx_status = CEC_TX_STATUS_OK;
		hdmi->hdmi_cec.tx_done = true;
		goto wake_exit;
	} else if (stat & SUNXI_CEC_IRQ_NACK) {
		hdmi->hdmi_cec.tx_status = CEC_TX_STATUS_NACK;
		hdmi->hdmi_cec.tx_done = true;
		goto wake_exit;
	}

	if (stat & SUNXI_CEC_IRQ_EOM) {
		len = sunxi_cec_message_receive((u8 *)&hdmi->hdmi_cec.rx_msg.msg);
		if (len < 0)
			goto none_exit;

		hdmi->hdmi_cec.rx_msg.len = len;
		smp_wmb();
		hdmi->hdmi_cec.rx_done = true;
		goto wake_exit;
	}

none_exit:
	return IRQ_NONE;
wake_exit:
	return IRQ_WAKE_THREAD;
}

static irqreturn_t _sunxi_drv_cec_thread(int irq, void *data)
{
	struct cec_adapter *adap    = data;
	struct sunxi_drm_hdmi *hdmi = cec_get_drvdata(adap);

	if (hdmi->hdmi_cec.tx_done) {
		hdmi->hdmi_cec.tx_done = false;
		cec_transmit_attempt_done(adap, hdmi->hdmi_cec.tx_status);
	}

	if (hdmi->hdmi_cec.rx_done) {
		hdmi->hdmi_cec.rx_done = false;
		smp_rmb();
		cec_received_msg(adap, &hdmi->hdmi_cec.rx_msg);
	}
	return IRQ_HANDLED;
}

static void _sunxi_drv_cec_adap_delect(void *data)
{
	struct sunxi_hdmi_cec_s *cec = data;

	cec_delete_adapter(cec->adap);
}

static int _sunxi_drv_cec_adap_enable(struct cec_adapter *adap, bool state)
{
	struct sunxi_drm_hdmi *hdmi = cec_get_drvdata(adap);

	if (hdmi->hdmi_cec.enable == state) {
		hdmi_inf("sunxi cec drv has been %s\n", state ?  "enable" : "disable");
		return 0;
	}

	if (state == SUNXI_HDMI_ENABLE) {
		/* enable cec clock */
		_sunxi_drv_cec_clock_on(hdmi);
		/* enable cec hardware */
		sunxi_cec_enable(SUNXI_HDMI_ENABLE);
	} else {
		/* disbale cec hardware */
		sunxi_cec_enable(SUNXI_HDMI_DISABLE);
		/* disable cec clock */
		_sunxi_drv_cec_clock_off(hdmi);
	}

	hdmi->hdmi_cec.enable = state;
	hdmi_inf("sunxi cec drv %s finish\n",
			state == SUNXI_HDMI_ENABLE ? "enable" : "disable");

	return 0;
}

static int _sunxi_drv_cec_adap_set_logicaddr(struct cec_adapter *adap, u8 addr)
{
	struct sunxi_drm_hdmi *hdmi = cec_get_drvdata(adap);

	hdmi_inf("cec adapter set logical addr: 0x%x\n", addr);
	if (addr == CEC_LOG_ADDR_INVALID)
		hdmi->hdmi_cec.logic_addr = 0x0;
	else
		hdmi->hdmi_cec.logic_addr = BIT(addr);

	sunxi_cec_set_logic_addr(hdmi->hdmi_cec.logic_addr);
	return 0;
}

static int _sunxi_drv_cec_adap_send(struct cec_adapter *adap, u8 attempts,
				u32 signal_free_time, struct cec_msg *msg)
{
	u8 times = SUNXI_CEC_WAIT_NULL;

	switch (signal_free_time) {
	case CEC_SIGNAL_FREE_TIME_RETRY:
		times = SUNXI_CEC_WAIT_3BIT;
		break;
	case CEC_SIGNAL_FREE_TIME_NEW_INITIATOR:
	default:
		times = SUNXI_CEC_WAIT_5BIT;
		break;
	case CEC_SIGNAL_FREE_TIME_NEXT_XFER:
		times = SUNXI_CEC_WAIT_7BIT;
		break;
	}

	sunxi_cec_message_send(msg->msg, msg->len, times);
	return 0;
}

static const struct cec_adap_ops _sunxi_cec_ops = {
	.adap_enable   = _sunxi_drv_cec_adap_enable,
	.adap_log_addr = _sunxi_drv_cec_adap_set_logicaddr,
	.adap_transmit = _sunxi_drv_cec_adap_send,
};


/*******************************************************************************
 * @desc: sunxi hdmi i2c adapter function
 ******************************************************************************/
static int _sunxi_drv_i2cm_xfer(struct i2c_adapter *adap,
			    struct i2c_msg *msgs, int num)
{
	return sunxi_hdmi_i2cm_xfer(msgs, num);
}

static u32 _sunxi_drv_i2cm_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm sunxi_hdmi_i2cm_algo = {
	.master_xfer	= _sunxi_drv_i2cm_xfer,
	.functionality	= _sunxi_drv_i2cm_func,
};

#define sysfs_show_name(cmd) _sunxi_hdmi_sysfs_##cmd##_show
#define sysfs_show_func(cmd)                          \
	static ssize_t sysfs_show_name(cmd)(struct device *dev, \
			struct device_attribute *attr, char *buf)

#define sysfs_store_name(cmd) _sunxi_hdmi_sysfs_##cmd##_store
#define sysfs_store_func(cmd)                    \
	ssize_t sysfs_store_name(cmd)(struct device *dev,  \
			struct device_attribute *attr, const char *buf, size_t count)

sysfs_show_func(reg_read)
{
	int n = 0;
	n += sprintf(buf + n, "\n[register read]\n");
	n += sprintf(buf + n, "Demo: echo offset,count > read\n");
	n += sprintf(buf + n, " - offset: register offset address.\n");
	n += sprintf(buf + n, " - count: read count register\n");
	return n;
}

sysfs_store_func(reg_read)
{
	unsigned long start_reg = 0;
	unsigned long read_count = 0;
	u32 r_value = 0;
	u32 i;
	u8 *separator;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	separator = strchr(buf, ',');
	if (separator != NULL) {
		if (sunxi_parse_dump_string(buf, count, &start_reg, &read_count)) {
			hdmi_err("input param failed! Use cat read node get more info!\n");
			return 0;
		}
		goto cmd_read;
	} else {
		separator = strchr(buf, ' ');
		if (separator != NULL) {
			start_reg = simple_strtoul(buf, NULL, 0);
			read_count = simple_strtoul(separator + 1, NULL, 0);
			goto cmd_read;
		} else {
			start_reg = simple_strtoul(buf, NULL, 0);
			read_count = 1;
			goto cmd_read;
		}
	}

cmd_read:
	hdmi_inf("read offset: 0x%lx, count: %ld\n", start_reg, read_count);
	for (i = 0; i < read_count; i++) {
		switch (hdmi->hdmi_ctrl.drv_reg_bank) {
		case SHDMI_REG_BANK_PHY:
			sunxi_hdmi_phy_read((u8)start_reg, &r_value);
			hdmi_inf("phy read: 0x%x = 0x%x\n", (u8)start_reg, r_value);
			break;
		case SHDMI_REG_BANK_SCDC:
			r_value = sunxi_hdmi_scdc_read((u8)start_reg);
			hdmi_inf("scdc read: 0x%x = 0x%x\n", (u8)start_reg, (u8)r_value);
			break;
		case SHDMI_REG_BANK_HPI:
			hdmi_inf("current unsupport hpi read\n");
			break;
		default:
			r_value = sunxi_hdmi_ctrl_read(start_reg);
			hdmi_inf("ctrl read: 0x%lx = 0x%x\n", start_reg, r_value);
			break;
		}
		start_reg++;
	}
	return count;
}

sysfs_show_func(reg_write)
{
	int n = 0;
	n += sprintf(buf + n, "\n[register write]\n");
	n += sprintf(buf + n, "Demo: echo offset,value > write\n");
	n += sprintf(buf + n, " - offset: register offset address.\n");
	n += sprintf(buf + n, " - value: write value\n");
	return n;
}

sysfs_store_func(reg_write)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;
	u8 *separator1 = NULL;
	u8 *separator2 = NULL;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	separator1 = strchr(buf, ',');
	separator2 = strchr(buf, ' ');
	if (separator1 != NULL) {
		if (sunxi_parse_dump_string(buf, count, &reg_addr, &value)) {
			hdmi_err("input param failed! Use cat read node get more info!\n");
			return 0;
		}
		goto cmd_write;
	} else if (separator2 != NULL) {
		reg_addr = simple_strtoul(buf, NULL, 0);
		value = simple_strtoul(separator2 + 1, NULL, 0);
		goto cmd_write;
	} else {
		hdmi_err("input param failed! Use cat read node get more info!\n");
		return 0;
	}

cmd_write:
	switch (hdmi->hdmi_ctrl.drv_reg_bank) {
	case SHDMI_REG_BANK_PHY:
		sunxi_hdmi_phy_write((u8)reg_addr, (u32)value);
		hdmi_inf("phy write: 0x%x = 0x%x\n", (u8)reg_addr, (u32)value);
		break;
	case SHDMI_REG_BANK_SCDC:
		sunxi_hdmi_scdc_write(reg_addr, value);
		hdmi_inf("scdc write: 0x%x = 0x%x\n", (u8)reg_addr, (u8)value);
		break;
	case SHDMI_REG_BANK_HPI:
		hdmi_inf("current unsupport hpi write\n");
		break;
	default:
		sunxi_hdmi_ctrl_write((uintptr_t)reg_addr, (u32)value);
		hdmi_inf("ctrl write: 0x%lx = 0x%x\n", reg_addr, (u32)value);
		break;
	}

	return count;
}

sysfs_show_func(reg_bank)
{
	int n = 0;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	n += sprintf(buf + n, "\nhdmi register bank\n");
	n += sprintf(buf + n, "Demo: echo index > rw_bank\n");
	n += sprintf(buf + n, " - %d: switch control write and read.\n", SHDMI_REG_BANK_CTRL);
	n += sprintf(buf + n, " - %d: switch phy write and read.\n", SHDMI_REG_BANK_PHY);
	n += sprintf(buf + n, " - %d: switch scdc write and read\n", SHDMI_REG_BANK_SCDC);
	n += sprintf(buf + n, " - %d: switch esm hpi write and read\n", SHDMI_REG_BANK_HPI);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return n;
	}
	n += sprintf(buf + n, "current reg bank index: %d\n",
		hdmi->hdmi_ctrl.drv_reg_bank);
	return n;
}

sysfs_store_func(reg_bank)
{
	u8 bank = 0;
	char *end;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	bank = (u8)simple_strtoull(buf, &end, 0);
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return count;
	}

	hdmi->hdmi_ctrl.drv_reg_bank = bank;
	hdmi_inf("switch reg bank index: %d\n", hdmi->hdmi_ctrl.drv_reg_bank);
	return count;
}

sysfs_show_func(pattern)
{
	int n = 0;
	n += sprintf(buf + n, "\nhdmi module pattern\n");
	n += sprintf(buf + n, "Demo: echo index > pattern\n");
	n += sprintf(buf + n, " - 1: [frame composer] red pattern.\n");
	n += sprintf(buf + n, " - 2: [frame composer] green pattern.\n");
	n += sprintf(buf + n, " - 3: [frame composer] blue pattern.\n");
	n += sprintf(buf + n, " - 4: [Tcon] color check\n");

	return n;
}

sysfs_store_func(pattern)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);
	int bit = 0;
	char *end;

	bit = (int)simple_strtoull(buf, &end, 0);

	switch (bit) {
	case 1:
		sunxi_hdmi_video_set_pattern(0x1, 0xFF0000);
		break;
	case 2:
		sunxi_hdmi_video_set_pattern(0x1, 0x00FF00);
		break;
	case 3:
		sunxi_hdmi_video_set_pattern(0x1, 0x0000FF);
		break;
	case 4:
		sunxi_hdmi_video_set_pattern(0x0, 0x0);
		sunxi_tcon_show_pattern(hdmi->sdrm.tcon_dev, 0x1);
		break;
	default:
		sunxi_hdmi_video_set_pattern(0x0, 0x0);
		sunxi_tcon_show_pattern(hdmi->sdrm.tcon_dev, 0x0);
		break;
	}

	return count;
}

sysfs_show_func(debug)
{
	int n = 0;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return n;
	}

	n += sprintf(buf + n, "\n[hdmi debug log level]\n");
	n += sprintf(buf + n, "Demo: echo 1 > debug\n");
	n += sprintf(buf + n, "current debug log: %s\n",
			sunxi_hdmi_get_loglevel() ? "enable" : "disable");

	return n;
}

sysfs_store_func(debug)
{
	u8 level = 0;
	char *end;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return count;
	}

	level = (u8)simple_strtoull(buf, &end, 0);

	suxni_hdmi_set_loglevel(level);
	return count;
}

sysfs_show_func(hdmi_source)
{
	int n = 0;
	static const char *state[] = {"disable", "processing", "failed", "success"};
	struct sunxi_drm_hdmi   *hdmi = dev_get_drvdata(dev);

	n += sprintf(buf + n, "\n[ver]\n");
	n += sprintf(buf + n, " - hw: 2.0\n");
	n += sprintf(buf + n, " - sw: 2.25.0403. I5500e572fd4671be68c769d948c11b78bb0a3cb7\n");

	n += sprintf(buf + n, "\n[drv cfg]\n");
	n += sprintf(buf + n, "|       |                  dts                        |                  drm                  |\n");
	n += sprintf(buf + n, "|  name |---------------------------------------------+---------------------------------------|\n");
	n += sprintf(buf + n, "|       | cec  | hdcp1x | hdcp2x | clk_src | resistor | enable | mode set | mode info | force |\n");
	n += sprintf(buf + n, "|-------+------+--------+--------+---------+----------+--------+----------+-----------+-------|\n");
	n += sprintf(buf + n, "| state | %-4s |  %-4s  |  %-4s  |  %-6s | %-8s |  %-4s  |   %-4s   | %-4dx%-4d | %-5s |\n",
		hdmi->hdmi_ctrl.drv_dts_cec ? "on" : "off",
		hdmi->hdmi_ctrl.drv_dts_hdcp1x ? "on" : "off",
		hdmi->hdmi_ctrl.drv_dts_hdcp2x ? "on" : "off",
		hdmi->hdmi_ctrl.drv_dts_clk_src ? "ccmu" : "phypll",
		hdmi->hdmi_ctrl.drv_dts_res_src ? "onboard" : "onchip",
		hdmi->hdmi_ctrl.drm_enable ? "yes" : "no",
		hdmi->hdmi_ctrl.drm_mode_set ? "yes" : "no",
		hdmi->drm_mode.hdisplay, hdmi->drm_mode.vdisplay,
		hdmi->hdmi_ctrl.drm_hpd_force == DRM_FORCE_ON ? "on" : "off");

	n += sprintf(buf + n, "\n[drv state]\n");
	n += sprintf(buf + n, "|       |     driver     |           hpd         |          hdcp      |  cec  |                |\n");
	n += sprintf(buf + n, "|  name |----------------+-----------------------+--------------------+-------+ support format |\n");
	n += sprintf(buf + n, "|       | clock | output | thread | state | mask | enctyption | clock | clock |                |\n");
	n += sprintf(buf + n, "|-------+-------+--------+--------+-------+------+------------+-------+-------+----------------|\n");
	n += sprintf(buf + n, "| state |  %-4s |  %-4s  | %-6s |  %-3s  | 0x%-2x |  %-8s  |  %-3s  |  %-3s  |    0x%-4x      |\n",
		hdmi->hdmi_ctrl.drv_clock ? "on" : "off",
		hdmi->hdmi_ctrl.drv_enable ? "on" : "off",
		hdmi->hpd_task ? "runing" : "stop",
		hdmi->hdmi_ctrl.drv_hpd_state ? "in" : "out",
		hdmi->hdmi_ctrl.drv_hpd_mask,
		state[hdmi->hdmi_ctrl.drv_hdcp_state],
		hdmi->hdmi_ctrl.drv_hdcp_clock ? "on" : "off",
		hdmi->hdmi_ctrl.drv_cec_clock ? "on" : "off",
		hdmi->hdmi_ctrl.drv_pixel_format_cap);

	n += sunxi_hdmi_tx_dump(buf + n);

	n += sprintf(buf + n, "\n");
	return n;
}

sysfs_store_func(hdmi_source)
{
	return count;
}

sysfs_show_func(hdmi_sink)
{
	ssize_t n = 0;
	u8 data = 0, status = 0;
	struct sunxi_drm_hdmi   *hdmi = dev_get_drvdata(dev);
	struct drm_display_info *info = &hdmi->sdrm.connector.display_info;

	if (!_sunxi_drv_hdmi_hpd_get(hdmi)) {
		n += sprintf(buf + n, "not sink info when hpd plugout!\n");
		goto exit;
	}

	n += sprintf(buf + n, "\n========= [hdmi sink] =========\n");
	n += sunxi_hdmi_rx_dump(buf + n);

	/* dump scdc */
	if (!info->hdmi.scdc.supported)
		goto exit;

	n += sprintf(buf + n, "[scdc]\n");
	data = sunxi_hdmi_scdc_read(SCDC_TMDS_CONFIG);
	status = sunxi_hdmi_scdc_read(SCDC_SCRAMBLER_STATUS) & SCDC_SCRAMBLING_STATUS;
	n += sprintf(buf + n, " - clock ratio    : %s\n",
			(data & SCDC_TMDS_BIT_CLOCK_RATIO_BY_40) ? "1/40" : "1/10");
	n += sprintf(buf + n, " - scramble       : %s\n",
			(data & SCDC_TMDS_BIT_CLOCK_RATIO_BY_40) ? "set" : "unset");
	n += sprintf(buf + n, " - scramble state : %s\n",
			status ? "enable" : "disable");

	data = sunxi_hdmi_scdc_read(SCDC_STATUS_FLAGS_0);
	n += sprintf(buf + n, " - clock channel  : %s\n",
			data & SCDC_CLOCK_DETECT ? "lock" : "unlock");
	n += sprintf(buf + n, " - data0 channel  : %s\n",
			data & SCDC_CH0_LOCK ? "lock" : "unlock");
	n += sprintf(buf + n, " - data1 channel  : %s\n",
			data & SCDC_CH1_LOCK ? "lock" : "unlock");
	n += sprintf(buf + n, " - data2 channel  : %s\n",
			data & SCDC_CH2_LOCK ? "lock" : "unlock");

	data = sunxi_hdcp2x_get_sink_cap();
	n += sprintf(buf + n, " - hdcp22 capability: %s\n",
			data ? "support" : "unsupport");

exit:
	return n;
}

sysfs_store_func(hdmi_sink)
{
	return count;
}

sysfs_show_func(hpd_mask)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);
	int n = 0;

	n += sprintf(buf + n, "\n[debug hpd mask]\n");
	n += sprintf(buf + n, " Demo: echo value > hpd_mask\n");
	n += sprintf(buf + n, " - 0x%x: force hpd plugin.\n", SUNXI_HDMI_HPD_FORCE_IN);
	n += sprintf(buf + n, " - 0x%x: force hpd plugout.\n", SUNXI_HDMI_HPD_FORCE_OUT);
	n += sprintf(buf + n, " - 0x%x: mask hpd notify.\n", SUNXI_HDMI_HPD_MASK_NOTIFY);
	n += sprintf(buf + n, " - 0x%x: mask hpd detect.\n", SUNXI_HDMI_HPD_MASK_DETECT);
	n += sprintf(buf + n, "Current hpd mask value: 0x%x\n", hdmi->hdmi_ctrl.drv_hpd_mask);

	return n;
}

sysfs_store_func(hpd_mask)
{
	int err;
	unsigned long val;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);
		return err;
	}

	hdmi->hdmi_ctrl.drv_hpd_mask = (u32)val;
	hdmi_inf("set hpd mask: 0x%x\n", (u32)val);

	return count;
}

sysfs_show_func(set_ddc)
{
	int n = 0;

	n += sprintf(buf + n, "\n[set ddc]\n");
	n += sprintf(buf + n, " Demo: echo mode,rate > ddc_rate\n");
	n += sprintf(buf + n, " @mode: ddc mode.\n");
	n += sprintf(buf + n, "   - 0: standard. (max rate: 100Kbit/s)\n");
	n += sprintf(buf + n, "   - 1: fast.     (max rate: 400Kbit/s)\n");
	n += sprintf(buf + n, " @rate: ddc integer rate.unit:Khz\n");

	return n;
}

sysfs_store_func(set_ddc)
{
	u8 *separator;
	unsigned long mode = 0, rate = 0;
	int ret = 0;

	separator = strchr(buf, ',');
	if (!IS_ERR_OR_NULL(separator)) {
		ret = sunxi_parse_dump_string(buf, count, &mode, &rate);
		if (ret != 0) {
			hdmi_err("ddc rate parse failed\n");
			goto exit;
		}
	} else {
		separator = strchr(buf, ' ');
		if (IS_ERR_OR_NULL(separator)) {
			hdmi_err("ddc rate parse failed\n");
			goto exit;
		}

		mode = simple_strtoul(buf, NULL, 0);
		rate = simple_strtoul(separator + 1, NULL, 0);
	}

	if (((mode == 0) && (rate > 100)) || ((mode == 1) && (rate > 400))) {
		hdmi_err("ddc set params invalid!\n");
		goto exit;
	}

	sunxi_hdmi_i2cm_set_ddc((u32)mode, (u32)rate);

	hdmi_inf("ddc rate config mode: %d value: %d\n", (u32)mode, (u32)rate);
exit:
	return count;
}

sysfs_show_func(edid_debug)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);
	int n = 0;

	n += sprintf(buf + n, "\n[edid debug]\n");
	n += sprintf(buf + n, " Demo: echo value > edid_debug\n");
	n += sprintf(buf + n, " - 0x1: enable use edid debug data\n");
	n += sprintf(buf + n, " - 0x0: disable use edid debug data\n");
	n += sprintf(buf + n, "Current edid debug mode: %d\n",
			hdmi->hdmi_ctrl.drv_edid_dbg_mode);

	return n;
}

sysfs_store_func(edid_debug)
{
	int ret;
	unsigned long val;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (count < 1)
		return -EINVAL;

	ret = kstrtoul(buf, 16, &val);
	if (ret) {
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);
		return ret;
	}

	hdmi->hdmi_ctrl.drv_edid_dbg_mode = (u8)val;
	hdmi_inf("edid debug mode set: %d\n", (u8)val);
	return count;
}

sysfs_show_func(edid_data)
{
	return 0;
}

sysfs_store_func(edid_data)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (count < 1) {
		hdmi_err("edid debug data is too small\n");
		return -EINVAL;
	}

	if (count > SUNXI_HDMI_EDID_LENGTH) {
		hdmi_err("edid debug data is too large\n");
		return -EINVAL;
	}

	memset(hdmi->hdmi_ctrl.drv_edid_dbg_data, 0x0, SUNXI_HDMI_EDID_LENGTH);
	memcpy(hdmi->hdmi_ctrl.drv_edid_dbg_data, buf, count);

	hdmi->hdmi_ctrl.drv_edid_dbg_size = count;
	return count;
}

sysfs_show_func(hdcp_enable)
{
	int n = 0;
	n += sprintf(buf + n, "\n[hdcp enable]\n");
	n += sprintf(buf + n, "Demo: echo index > hdcp_enable\n");
	n += sprintf(buf + n, " - 1: enable  0: disable\n");
	return n;
}

sysfs_store_func(hdcp_enable)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "1", 1) == 0) {
		hdmi->hdmi_ctrl.drv_hdcp_enable = 0x1;
		_sunxi_drv_hdcp_enable(hdmi);
	} else {
		hdmi->hdmi_ctrl.drv_hdcp_enable = 0x0;
		_sunxi_drv_hdcp_disable(hdmi);
	}

	return count;
}

sysfs_show_func(hdcp_type)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);
	char hdcp_type = (char)hdmi->hdmi_ctrl.drv_hdcp_type;
	memcpy(buf, &hdcp_type, 1);
	return 1;
}

sysfs_store_func(hdcp_type)
{
	return count;
}

sysfs_show_func(hdcp_status)
{
	u32 count = sizeof(u8);
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);
	u8 statue = _sunxi_drv_hdcp_get_state(hdmi);

	memcpy(buf, &statue, count);

	return count;
}

sysfs_store_func(hdcp_status)
{
	return count;
}

sysfs_show_func(hdcp_loader)
{
	int n = 0;

	n += sprintf(buf + n, "\n[hdcp loader]\n");
	n += sprintf(buf + n, " - current loader state: %s\n",
			sunxi_hdcp2x_fw_state() ? "loaded" : "not-loaded");
	n += sprintf(buf + n, " - echo 1 > hdcp_loader   #enable hdcp fw load\n");
	n += sprintf(buf + n, "\nNote: You need to configure the firmware path\n");
	n += sprintf(buf + n, "1. Store the firmware in the path you specify, such as /data/fw/\n");
	n += sprintf(buf + n, "2. Set the upper level path of the firmware\n");
	n += sprintf(buf + n, " - demo: echo /data/fw > /sys/module/firmware_class/parameters/path\n");
	n += sprintf(buf + n, "3. Use this cmd for enable hdcp loader\n");

	return n;
}

sysfs_store_func(hdcp_loader)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);
	int ret = 0;

	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "1", 1) == 0) {
		ret = _sunxi_drv_hdcp_loading_fw(dev);
		if (ret == 0) {
			hdmi_inf("sunxi hdcp loading fw failed\n");
			return -EINVAL;
		}
		_sunxi_drv_hdcp_update_support(hdmi);
		return count;
	}

	hdmi_inf("unsupport cmd args: %s\n", buf);
	return -EINVAL;
}

#define sysfs_attr(cmd) DEVICE_ATTR(cmd, 0664, sysfs_show_name(cmd), sysfs_store_name(cmd))

static sysfs_attr(reg_read);
static sysfs_attr(reg_write);
static sysfs_attr(reg_bank);
static sysfs_attr(pattern);
static sysfs_attr(debug);
static sysfs_attr(hdmi_source);
static sysfs_attr(hdmi_sink);
static sysfs_attr(hpd_mask);
static sysfs_attr(set_ddc);
static sysfs_attr(edid_debug);
static sysfs_attr(edid_data);
static sysfs_attr(hdcp_enable);
static sysfs_attr(hdcp_type);
static sysfs_attr(hdcp_status);
static sysfs_attr(hdcp_loader);

static struct attribute *_sunxi_hdmi_attrs[] = {
	&dev_attr_reg_write.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_reg_bank.attr,
	&dev_attr_pattern.attr,
	&dev_attr_debug.attr,
	&dev_attr_hdmi_source.attr,
	&dev_attr_hdmi_sink.attr,
	&dev_attr_hpd_mask.attr,
	&dev_attr_set_ddc.attr,
	&dev_attr_edid_debug.attr,
	&dev_attr_edid_data.attr,
	&dev_attr_hdcp_enable.attr,
	&dev_attr_hdcp_type.attr,
	&dev_attr_hdcp_status.attr,
	&dev_attr_hdcp_loader.attr,
	NULL
};

static struct attribute_group _sunxi_hdmi_group = {
	.name = "attr",
	.attrs = _sunxi_hdmi_attrs,
};

/*******************************************************************************
 * drm sunxi hdmi encoder helper functions
 ******************************************************************************/
static void _sunxi_drm_hdmi_disable(struct drm_encoder *encoder,
		struct drm_atomic_state *state)
{
	int ret = 0;
	struct device *dev = NULL;
	struct sunxi_drm_hdmi *hdmi = drm_encoder_to_hdmi(encoder);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return;
	}

	if (hdmi->hdmi_ctrl.drv_pm_state)
		goto exit;

	ret = _sunxi_drv_hdmi_disable(hdmi);
	if (ret != 0) {
		hdmi_err("hdmi drv disable failed\n");
		return;
	}

	dev = hdmi->sdrm.tcon_dev;
	if (IS_ERR_OR_NULL(dev)) {
		shdmi_err(dev);
		return;
	}

	ret = sunxi_tcon_mode_exit(dev);
	if (ret != 0) {
		hdmi_err("hdmi tcon mode exit failed\n");
		return;
	}

exit:
	hdmi->hdmi_ctrl.drm_enable   = 0x0;
	hdmi->hdmi_ctrl.drm_mode_set = 0x0;
	hdmi->hdmi_ctrl.drv_enable   = 0x0;
	hdmi_inf("drm hdmi atomic disable done >>>>>>>>>>>>>>>>\n");
}

static void _sunxi_drm_hdmi_enable(struct drm_encoder *encoder,
		struct drm_atomic_state *state)
{
	struct sunxi_drm_hdmi *hdmi = drm_encoder_to_hdmi(encoder);
	struct drm_crtc *crtc = encoder->crtc;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc->state);
	struct disp_output_config disp_cfg;
	struct device *tcon_dev = NULL;
	int ret = 0;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return;
	}

	/* tcon hdmi enable */
	tcon_dev = hdmi->sdrm.tcon_dev;
	if (IS_ERR_OR_NULL(tcon_dev)) {
		shdmi_err(tcon_dev);
		return;
	}

	if (hdmi->hdmi_ctrl.drv_pm_state) {
		pm_runtime_get_sync(hdmi->dev);
		_sunxi_drv_hdmi_set_resource(hdmi, SUNXI_HDMI_ENABLE);
	}

	/* set tcon config data */
	memset(&disp_cfg, 0x0, sizeof(disp_cfg));
	memcpy(&disp_cfg.timing, &hdmi->disp_timing, sizeof(struct disp_video_timings));
	disp_cfg.type        = INTERFACE_HDMI;
	disp_cfg.de_id       = sunxi_drm_crtc_get_hw_id(crtc);
	disp_cfg.format      = hdmi->disp_config.format;
	disp_cfg.irq_handler = sunxi_crtc_event_proc;
	disp_cfg.irq_data    = scrtc_state->base.crtc;
	disp_cfg.sw_enable   = hdmi->hdmi_ctrl.drv_sw_enable;

	ret = sunxi_tcon_mode_init(tcon_dev, &disp_cfg);
	if (ret != 0) {
		hdmi_err("sunxi tcon hdmi mode init failed\n");
		return;
	}

	if (hdmi->hdmi_ctrl.drv_sw_enable)
		ret = _sunxi_drv_hdmi_smooth_enable(hdmi);
	else
		ret = _sunxi_drv_hdmi_enable(hdmi);
	if (ret != 0) {
		hdmi_err("drm hdmi enable driver failed\n");
		return;
	}

	hdmi->hdmi_ctrl.drm_enable = 0x1;
	hdmi_inf("drm hdmi atomic enable >>>>>>>>>>>>>>>>\n");
}

static int _sunxi_drm_hdmi_check(struct drm_encoder *encoder,
		    struct drm_crtc_state *crtc_state,
		    struct drm_connector_state *conn_state)
{
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct sunxi_drm_hdmi *hdmi = drm_encoder_to_hdmi(encoder);
	int ret = 0, sw_enable = 0;

	if (IS_ERR_OR_NULL(scrtc_state)) {
		shdmi_err(scrtc_state);
		return -1;
	}

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (conn_state->crtc) {
		sw_enable = sunxi_drm_check_if_need_sw_enable(conn_state->connector);
		hdmi->hdmi_ctrl.drv_sw_enable = sw_enable;
	}

	if (hdmi->hdmi_ctrl.drv_sw_enable) {
		hdmi_inf("drm hdmi check sw enable.\n");
		goto exit_fill;
	}

	memcpy(&hdmi->drm_mode_adjust, &crtc_state->mode, sizeof(struct drm_display_mode));

	ret = _sunxi_drv_hdmi_select_output(hdmi);
	if (ret != 0) {
		hdmi_err("drm hdmi set not change when select output failed\n");
		return -EINVAL;
	}

	ret = _sunxi_drv_hdmi_disp_info_check(hdmi);
	if (ret == 2) {
		crtc_state->mode_changed = true;
		hdmi_inf("drm hdmi set mode change when disp info change\n");
	}

exit_fill:
	ret = _sunxi_drv_hdmi_filling_scrtc(hdmi, scrtc_state);
	if (ret != 0) {
		hdmi_err("drm hdmi filling scrtc failed\n");
		goto exit;
	}

exit:
	hdmi_inf("drm hdmi check mode: %s >>>>>>>>>>>>>>>>\n",
		crtc_state->mode_changed ? "change" : "unchange");
	return 0;
}

static void _sunxi_drm_hdmi_mode_set(struct drm_encoder *encoder,
		struct drm_display_mode *mode,
		struct drm_display_mode *adjust_mode)
{
	int ret = 0;
	struct sunxi_drm_hdmi *hdmi = drm_encoder_to_hdmi(encoder);

	if (!hdmi) {
		hdmi_err("%s param hdmi is null!!!\n", __func__);
		return;
	}

	memcpy(&hdmi->drm_mode, adjust_mode, sizeof(struct drm_display_mode));

	ret = sunxi_hdmi_set_disp_mode(&hdmi->drm_mode);
	if (ret != 0) {
		hdmi_err("drm mode set convert failed\n");
		return;
	}

	ret = sunxi_hdmi_set_disp_info(&hdmi->disp_config);
	if (ret != 0)
		hdmi_inf("drm atomic enable fill config failed\n");

	sunxi_hdmi_select_output_packets(adjust_mode->flags);

	ret = drm_mode_to_sunxi_video_timings(&hdmi->drm_mode, &hdmi->disp_timing);
	if (ret != 0) {
		hdmi_err("drm mode disp_timing %d*%d convert disp disp_timing failed\n",
			hdmi->drm_mode.hdisplay, hdmi->drm_mode.vdisplay);
		return;
	}

	__sunxi_drv_hdmi_get_pxfmt_cap(hdmi);

	hdmi->hdmi_ctrl.drm_mode_set = 0x1;
	hdmi_inf("drm hdmi mode set: %d*%d >>>>>>>>>>>>>>>>\n",
			adjust_mode->hdisplay, adjust_mode->vdisplay);
}

/*******************************************************************************
 * drm sunxi hdmi connect helper functions
 ******************************************************************************/
static int _sunxi_drm_hdmi_get_modes(struct drm_connector *connector)
{
	struct sunxi_drm_hdmi   *hdmi = drm_connector_to_hdmi(connector);
	struct drm_display_mode *mode = NULL;
	struct edid  *raw_edid = NULL;
	struct drm_display_info *info = &connector->display_info;
	int ret = 0, i = 0;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (!_sunxi_drv_hdmi_hpd_get(hdmi))
		return -1;

	ret = _sunxi_drv_hdmi_read_edid(hdmi);
	if (ret != 0) {
		hdmi_err("drm get mode read edid failed\n");
		goto use_default;
	}

	raw_edid = hdmi->hdmi_ctrl.drv_edid_data;
	if (IS_ERR_OR_NULL(raw_edid)) {
		shdmi_err(raw_edid); /*error*/
		return -1;
	}

	drm_connector_update_edid_property(connector, raw_edid);

	if (!IS_ERR_OR_NULL(hdmi->hdmi_cec.notify))
		cec_notifier_set_phys_addr_from_edid(hdmi->hdmi_cec.notify, raw_edid);

	ret = drm_add_edid_modes(connector, raw_edid);
	hdmi_inf("drm get edid support modes: %d\n", ret);
	return ret;

use_default:
	/* can not get edid, but use default mode! */
	hdmi_wrn("hdmi drv use default edid modes\n");
	for (i = 0; i < ARRAY_SIZE(_sunxi_hdmi_default_modes); i++) {
		const struct drm_display_mode *temp_mode = &_sunxi_hdmi_default_modes[i];
		mode = drm_mode_duplicate(connector->dev, temp_mode);
		if (mode) {
			if (i != 0) {
				mode->type = DRM_MODE_TYPE_PREFERRED;
				mode->picture_aspect_ratio = HDMI_PICTURE_ASPECT_NONE;
			}
			drm_mode_probed_add(connector, mode);
			ret++;
		}
	}

	info->hdmi.y420_dc_modes = 0;
	info->color_formats = 0;

	return ret;
}

static enum drm_mode_status _sunxi_drm_hdmi_mode_valid(
		struct drm_connector *connector, struct drm_display_mode *mode)
{
	int rate = drm_mode_vrefresh(mode);

	/* check low i-timing */
	if ((mode->flags & DRM_MODE_FLAG_INTERLACE) && (mode->vdisplay < 1080)) {
		hdmi_trace("drm hdmi unsupport mode %dx%di@%dHz\n",
				mode->hdisplay, mode->vdisplay, rate);
		return MODE_BAD;
	}

	/* check frame rate support */
	if (rate > 60) {
		return MODE_BAD;
	}

	return MODE_OK;
}

/*******************************************************************************
 * drm sunxi hdmi connect functions
 ******************************************************************************/
static enum drm_connector_status
_sunxi_drm_hdmi_detect(struct drm_connector *connector, bool force)
{
	struct sunxi_drm_hdmi *hdmi =  drm_connector_to_hdmi(connector);
	int ret = connector_status_unknown;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return ret;
	}

	ret = _sunxi_drv_hdmi_hpd_get(hdmi);
	hdmi_inf("drm hdmi detect: %s\n", ret ? "connect" : "disconnect");
	return ret == 1 ? connector_status_connected : connector_status_disconnected;
}

static void _sunxi_drm_hdmi_force(struct drm_connector *connector)
{
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(connector);
	char *force_string[] = {"detect", "off", "on", "dvi-on"};

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return ;
	}

	hdmi->hdmi_ctrl.drm_hpd_force = (int)connector->force;

	if (connector->force == DRM_FORCE_ON)
		_sunxi_drv_hdmi_hpd_plugin(hdmi);
	else if (connector->force == DRM_FORCE_OFF)
		_sunxi_drv_hdmi_hpd_plugout(hdmi);

	hdmi_inf("drm hdmi set force hpd: %s\n", force_string[connector->force]);
}

static int _sunxi_drm_hdmi_get_property(struct drm_connector *connector,
		const struct drm_connector_state *state,
		struct drm_property *property, uint64_t *val)
{
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(connector);
	struct disp_device_config *info = sunxi_hdmi_get_disp_info();

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (IS_ERR_OR_NULL(info)) {
		shdmi_err(info);
		return -1;
	}

	if (hdmi->prop_pxfmt_cap == property) {
		__sunxi_drv_hdmi_get_pxfmt_cap(hdmi);
		*val = hdmi->hdmi_ctrl.drv_pixel_format_cap;
		goto get_done;
	}

	if (hdmi->prop_pxfmt == property) {
		__sunxi_drv_hdmi_get_pxfmt(hdmi);
		*val = hdmi->hdmi_ctrl.drv_pixel_format;
		goto get_done;
	}

	if (hdmi->prop_dr_cap == property) {
		__sunxi_drv_hdmi_get_dynamic_range_cap(hdmi);
		*val = hdmi->hdmi_ctrl.drv_dynamic_range_cap;
		goto get_done;
	}

	if (hdmi->prop_dr == property) {
		__sunxi_drv_hdmi_get_dynamic_range(hdmi);
		*val = hdmi->hdmi_ctrl.drv_dynamic_range;
		goto get_done;
	}

	hdmi_err("drm hdmi unsupport get property: %s\n", property->name);
	return -1;
get_done:
	hdmi_trace("drm hdmi get property %s = %lld\n", property->name, *val);
	return 0;
}

static int _sunxi_drm_hdmi_set_property(
		struct drm_connector *connector, struct drm_connector_state *state,
		struct drm_property *property, uint64_t val)
{
	struct sunxi_drm_hdmi *hdmi = drm_connector_to_hdmi(connector);
	int ret = 0;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (hdmi->prop_pxfmt == property) {
		ret = __sunxi_drv_hdmi_set_pxfmt(hdmi, val);
		goto set_done;
	}

	if (hdmi->prop_dr == property) {
		ret = __sunxi_drv_hdmi_set_dynamic_range(hdmi, val);
		goto set_done;
	}

	hdmi_trace("drm hdmi unsupport set property: %s\n", property->name);
	return -1;
set_done:
	hdmi_trace("drm hdmi set property %s: 0x%x %s\n",
		property->name, (u32)val, (ret == 0) ? "success" : "failed");
	return 0;
}

static const struct drm_connector_funcs
sunxi_hdmi_connector_funcs = {
	.detect                 = _sunxi_drm_hdmi_detect,
	.force                  = _sunxi_drm_hdmi_force,
	.atomic_get_property    = _sunxi_drm_hdmi_get_property,
	.atomic_set_property    = _sunxi_drm_hdmi_set_property,
	.fill_modes		        = drm_helper_probe_single_connector_modes,
	.destroy		        = drm_connector_cleanup,
	.reset			        = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
sunxi_hdmi_connector_helper_funcs = {
	.get_modes	= _sunxi_drm_hdmi_get_modes,
	.mode_valid = _sunxi_drm_hdmi_mode_valid,
};

static const struct drm_encoder_helper_funcs
sunxi_hdmi_encoder_helper_funcs = {
	.atomic_disable		= _sunxi_drm_hdmi_disable,
	.atomic_enable		= _sunxi_drm_hdmi_enable,
	.atomic_check		= _sunxi_drm_hdmi_check,
	.mode_set           = _sunxi_drm_hdmi_mode_set,
};

/*******************************************************************************
 *  sunxi hdmi initialization function
 ******************************************************************************/
static int _sunxi_hdmi_init_get_tcon(struct device *dev)
{
	struct device_node *tcon_tv_node  = NULL;
	struct device_node *hdmi_endpoint = NULL;
	struct platform_device *tcon_pdev = NULL;
	struct sunxi_drm_hdmi  *hdmi = dev_get_drvdata(dev);
	struct sunxi_drm_device *sdrm = &hdmi->sdrm;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	hdmi_endpoint = of_graph_get_endpoint_by_regs(dev->of_node, 0, 0);
	if (IS_ERR_OR_NULL(hdmi_endpoint)) {
		shdmi_err(hdmi_endpoint);
		return -1;
	}

	tcon_tv_node = of_graph_get_remote_port_parent(hdmi_endpoint);
	if (IS_ERR_OR_NULL(tcon_tv_node)) {
		shdmi_err(tcon_tv_node);
		goto put_endpoint;
	}

	tcon_pdev = of_find_device_by_node(tcon_tv_node);
	if (IS_ERR_OR_NULL(tcon_pdev)) {
		shdmi_err(tcon_pdev);
		goto put_tcon;
	}

	sdrm->tcon_dev = &tcon_pdev->dev;
	sdrm->tcon_id = sunxi_tcon_of_get_id(sdrm->tcon_dev);
	platform_device_put(tcon_pdev);

	return 0;
put_tcon:
	hdmi_err("hdmi init get tcon dev failed\n");
	of_node_put(tcon_tv_node);
put_endpoint:
	hdmi_err("hdmi init get end-point failed\n");
	of_node_put(hdmi_endpoint);
	return -1;
}

static int _sunxi_hdmi_init_sysfs(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;
	dev_t dev_id = 0;
	struct class *clas = NULL;
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	ret = alloc_chrdev_region(&dev_id, 0, 1, "hdmi");
	if (ret != 0) {
		hdmi_err("hdmi init dev failed when alloc dev id\n");
		goto err_region;
	}
	hdmi->hdmi_devid = dev_id;

	/* creat hdmi class */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	clas = class_create(THIS_MODULE, "hdmi");
#else
	clas = class_create("hdmi");
#endif
	if (IS_ERR_OR_NULL(clas)) {
		shdmi_err(clas);
		goto err_class;
	}
	hdmi->hdmi_class = clas;

	/* creat hdmi device */
	dev = device_create(clas, NULL, dev_id, NULL, "hdmi");
	if (IS_ERR_OR_NULL(dev)) {
		shdmi_err(dev);
		goto err_device;
	}
	hdmi->hdmi_dev = dev;

	/* creat hdmi attr */
	ret = sysfs_create_group(&dev->kobj, &_sunxi_hdmi_group);
	if (ret != 0) {
		hdmi_err("hdmi init dev failed when creat group\n");
		goto err_group;
	}

	dev_set_drvdata(dev, hdmi);
	return 0;

err_group:
	device_destroy(clas, dev_id);
err_device:
	class_destroy(clas);
err_class:
	unregister_chrdev_region(dev_id, 1);
err_region:
	return -1;
}

static int __sunxi_hdmi_init_dts(struct sunxi_drm_hdmi *hdmi)
{
	u32 value = 0;
	int ret = 0;
	struct device *dev = hdmi->dev;
	struct device_node	*node = dev->of_node;
	struct sunxi_hdmi_res_s *pclk = &hdmi->hdmi_res;

	if (IS_ERR_OR_NULL(node)) {
		shdmi_err(node);
		return -1;
	}

	hdmi->hdmi_core.reg_base = (uintptr_t __force)of_iomap(node, 0);
	if (hdmi->hdmi_core.reg_base == 0) {
		hdmi_err("unable to map hdmi registers");
		return -1;
	}

	hdmi->hdmi_irq = platform_get_irq(to_platform_device(dev), 0);
	if (hdmi->hdmi_irq < 0) {
		hdmi_err("hdmi drv detect dts not set irq. cec invalid\n");
		hdmi->hdmi_irq = -1;
	}

	hdmi->hdmi_cec.irq = hdmi->hdmi_irq;

	ret = of_property_read_u32(node, "hdmi_cec_enable", &value);
	hdmi->hdmi_ctrl.drv_dts_cec = (ret != 0x0) ? 0x0 : value;

	/* new node, some plat maybe not set, so default hdcp14 enable */
	ret = of_property_read_u32(node, "hdmi_hdcp1x_enable", &value);
	hdmi->hdmi_ctrl.drv_dts_hdcp1x = (ret != 0x0) ? 0x0 : value;

	ret = of_property_read_u32(node, "hdmi_hdcp2x_enable", &value);
	hdmi->hdmi_ctrl.drv_dts_hdcp2x = (ret != 0x0) ? 0x0 : value;

	ret = of_property_read_u32(node, "hdmi_power_cnt", &value);
	hdmi->hdmi_ctrl.drv_dts_power_cnt = (ret != 0x0) ? 0x0 : value;

	ret = of_property_read_u32(node, "hdmi_clock_source", &value);
	hdmi->hdmi_ctrl.drv_dts_clk_src = (ret != 0x0) ? 0x0 : value;

	/* if dts not set, default use external resistor */
	ret = of_property_read_u32(node, "hdmi_resistor_select", &value);
	hdmi->hdmi_ctrl.drv_dts_res_src = (ret != 0x0) ? 0x1 : value;

	/* if dts not set, default use 0x1F */
	ret = of_property_read_u32(node, "hdmi_ddc_index", &value);
	hdmi->hdmi_ctrl.drv_dts_ddc_index = (ret != 0x0) ? 0x1F : value;

	/* parse tcon clock */
	pclk->clk_tcon_tv = devm_clk_get(dev, "clk_tcon_tv");
	if (IS_ERR_OR_NULL(pclk->clk_tcon_tv))
		pclk->clk_tcon_tv = NULL;

	/* parse hdmi clock */
	pclk->clk_hdmi = devm_clk_get(dev, "clk_hdmi");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi))
		pclk->clk_hdmi = NULL;

	/* parse hdmi 24M clock */
	pclk->clk_hdmi_24M = devm_clk_get(dev, "clk_hdmi_24M");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi_24M))
		pclk->clk_hdmi_24M = NULL;

	/* parse hdmi bus clock */
	pclk->clk_hdmi_bus = devm_clk_get(dev, "clk_bus_hdmi");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi_bus))
		pclk->clk_hdmi_bus = NULL;

	/* parse hdmi ddc clock */
	pclk->clk_hdmi_ddc = devm_clk_get(dev, "clk_ddc");
	if (IS_ERR_OR_NULL(pclk->clk_hdmi_ddc))
		pclk->clk_hdmi_ddc = NULL;

	/* parse hdmi cec clock */
	pclk->clk_cec = devm_clk_get(dev, "clk_cec");
	if (IS_ERR_OR_NULL(pclk->clk_cec))
		pclk->clk_cec = NULL;

	/* parse hdmi cec parent clock */
	pclk->clk_cec_parent = clk_get_parent(pclk->clk_cec);
	if (IS_ERR_OR_NULL(pclk->clk_cec_parent))
		pclk->clk_cec_parent = NULL;

	/* parse hdmi hdcp clock */
	pclk->clk_hdcp = devm_clk_get(dev, "clk_hdcp");
	if (IS_ERR_OR_NULL(pclk->clk_hdcp))
		pclk->clk_hdcp = NULL;

	/* parse hdmi hdcp bus clock */
	pclk->clk_hdcp_bus = devm_clk_get(dev, "clk_bus_hdcp");
	if (IS_ERR_OR_NULL(pclk->clk_hdcp_bus))
		pclk->clk_hdcp_bus = NULL;

	/* parse hdmi hdcp bus reset clock */
	pclk->rst_bus_hdcp = devm_reset_control_get(dev, "rst_bus_hdcp");
	if (IS_ERR_OR_NULL(pclk->rst_bus_hdcp))
		pclk->rst_bus_hdcp = NULL;

	/* parse hdmi sub bus reset clock */
	pclk->rst_bus_sub = devm_reset_control_get(dev, "rst_bus_sub");
	if (IS_ERR_OR_NULL(pclk->rst_bus_sub))
		pclk->rst_bus_sub = NULL;

	/* parse hdmi sub main reset clock */
	pclk->rst_bus_main = devm_reset_control_get(dev, "rst_bus_main");
	if (IS_ERR_OR_NULL(pclk->rst_bus_main))
		pclk->rst_bus_main = NULL;

	hdmi_inf("hdmi drv init dts done\n");
	return 0;
}

static int __sunxi_hdmi_init_resource(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0, loop = 0;
	const char *power_string;
	char power_name[40];
	struct regulator *regu = NULL;

	struct device *dev = hdmi->dev;
	struct sunxi_hdmi_res_s *pres = &hdmi->hdmi_res;

	if (IS_ERR_OR_NULL(dev)) {
		shdmi_err(dev);
		return -1;
	}

	/* get power regulator */
	for (loop = 0; loop < hdmi->hdmi_ctrl.drv_dts_power_cnt; loop++) {
		sprintf(power_name, "hdmi_power%d", loop);
		ret = of_property_read_string(dev->of_node, power_name, &power_string);
		if (ret != 0) {
			hdmi_wrn("hdmi dts not set: hdmi_power%d!\n", loop);
			continue;
		}
		memcpy((void *)pres->power_name[loop],
				power_string, strlen(power_string) + 1);
		regu = regulator_get(dev, pres->power_name[loop]);
		if (IS_ERR_OR_NULL(regu)) {
			hdmi_wrn("init resource to get %s regulator is failed\n",
				pres->power_name[loop]);
			continue;
		}
		pres->hdmi_regu[loop] = regu;
	}

	hdmi->hpd_task =
		kthread_create(_sunxi_drv_hdmi_thread, (void *)hdmi, "hdmi hpd");
	if (IS_ERR_OR_NULL(hdmi->hpd_task)) {
		hdmi_err("init thread handle hpd is failed!!!\n");
		hdmi->hpd_task = NULL;
		return -1;
	}

#if IS_ENABLED(CONFIG_EXTCON)
	hdmi->hpd_extcon = devm_extcon_dev_allocate(dev, sunxi_hdmi_cable);
	if (IS_ERR_OR_NULL(hdmi->hpd_extcon)) {
		hdmi_err("init notify alloc extcon node failed!!!\n");
		return -1;
	}

	ret = devm_extcon_dev_register(dev, hdmi->hpd_extcon);
	if (ret != 0) {
		hdmi_err("init notify register extcon node failed!!!\n");
		return -1;
	}

	hdmi->hpd_extcon->name = "drm-hdmi";
#else
	hdmi_wrn("not set hdmi notify node!\n");
#endif

	_sunxi_drv_hdmi_set_resource(hdmi, SUNXI_HDMI_ENABLE);

	mutex_init(&hdmi->hdmi_ctrl.drv_edid_lock);

	hdmi_inf("hdmi drv init resource done\n");
	return 0;
}

static int __sunxi_hdmi_init_value(struct sunxi_drm_hdmi *hdmi)
{
	struct disp_device_config *info = &hdmi->disp_config;

	info->format       = DISP_CSC_TYPE_RGB;
	info->bits         = DISP_DATA_8BITS;
	info->eotf         = DISP_EOTF_GAMMA22; /* SDR */
	info->cs           = DISP_BT709;
	info->dvi_hdmi     = DISP_HDMI;
	info->range        = DISP_COLOR_RANGE_DEFAULT;
	info->scan         = DISP_SCANINFO_NO_DATA;
	info->aspect_ratio = HDMI_ACTIVE_ASPECT_PICTURE;

	_sunxi_drv_hdcp_release(hdmi);

	hdmi->hdmi_ctrl.drv_edid_data  = NULL;
	hdmi->hdmi_ctrl.drv_pixel_format_cap  = BIT(SHDMI_RGB888_8BITS);
	hdmi->hdmi_ctrl.drv_pixel_format      = BIT(SHDMI_RGB888_8BITS);
	hdmi->hdmi_ctrl.drv_dynamic_range_cap = BIT(SHDMI_SDR);
	hdmi->hdmi_ctrl.drv_dynamic_range     = BIT(SHDMI_SDR);
	hdmi->hdmi_ctrl.drv_max_bits   = DISP_DATA_10BITS;
	return 0;
}

static int ___sunxi_hdmi_init_i2cm_adap(struct sunxi_drm_hdmi *hdmi)
{
	struct i2c_adapter *adap = &hdmi->i2c_adap;
	int ret = 0;

	if (IS_ERR_OR_NULL(adap)) {
		shdmi_err(adap);
		return -1;
	}

	adap->nr    = hdmi->hdmi_ctrl.drv_dts_ddc_index;
	adap->class = I2C_CLASS_DDC;
	adap->owner = THIS_MODULE;
	adap->dev.parent = hdmi->dev;
	adap->algo = &sunxi_hdmi_i2cm_algo;
	strlcpy(adap->name, "SUNXI HDMI", sizeof(adap->name));

	i2c_set_adapdata(adap, hdmi);
	ret = i2c_add_numbered_adapter(adap);
	if (ret) {
		hdmi_err("hdmi drv register %s i2c adapter failed\n", adap->name);
		return -1;
	}
	hdmi_inf("hdmi drv init i2c adapter finish\n");
	return 0;
}

static int ___sunxi_hdmi_init_cec_adap(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;
	struct device *dev = hdmi->dev;
	struct sunxi_hdmi_cec_s *cec = &hdmi->hdmi_cec;

	if (IS_ERR_OR_NULL(dev)) {
		shdmi_err(dev);
		return -1;
	}

	if (hdmi->hdmi_ctrl.drv_dts_cec == 0x0)
		return 0;

	cec->adap = cec_allocate_adapter(&_sunxi_cec_ops, hdmi, "sunxi_cec",
			CEC_CAP_DEFAULTS | CEC_CAP_CONNECTOR_INFO, CEC_MAX_LOG_ADDRS);
	if (IS_ERR_OR_NULL(cec->adap))
		return -1;

	cec->adap->owner = THIS_MODULE;

	ret = devm_add_action(dev, _sunxi_drv_cec_adap_delect, cec);
	if (ret) {
		cec_delete_adapter(cec->adap);
		return -1;
	}

	ret = devm_request_threaded_irq(dev, cec->irq,
			_sunxi_drv_cec_hardirq, _sunxi_drv_cec_thread,
			IRQF_SHARED, "sunxi-cec", cec->adap);
	if (ret < 0)
		return -1;

	cec->notify = cec_notifier_cec_adap_register(dev, NULL, cec->adap);
	if (IS_ERR_OR_NULL(cec->notify))
		return -ENOMEM;

	ret = cec_register_adapter(cec->adap, dev);
	if (ret < 0) {
		cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
		return -1;
	}

	devm_remove_action(dev, _sunxi_drv_cec_adap_delect, cec);

	hdmi_inf("hdmi drv init cec adapter finish\n");
	return 0;
}

static int __sunxi_hdmi_init_adapter(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;

	ret = ___sunxi_hdmi_init_i2cm_adap(hdmi);
	if (ret != 0) {
		hdmi_err("hdmi drv init i2c adapter failed\n");
		return -1;
	}

	ret = ___sunxi_hdmi_init_cec_adap(hdmi);
	if (ret != 0) {
		hdmi_err("hdmi drv init cec adapter failed\n");
		return -1;
	}

	return 0;
}

static int _sunxi_hdmi_init_drv(struct sunxi_drm_hdmi *hdmi)
{
	int ret = -1;

	/* parse sunxi hdmi dts */
	ret = __sunxi_hdmi_init_dts(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init dts failed!!!\n");
		return -1;
	}

	/* sunxi hdmi resource alloc and enable */
	ret = __sunxi_hdmi_init_resource(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init resource failed!!!\n");
		return -1;
	}

	/* hdmi config value init */
	ret = __sunxi_hdmi_init_value(hdmi);
	if (ret != 0)
		hdmi_err("sunxi hdmi init config value failed\n");

	/* hdmi init register adapter */
	ret = __sunxi_hdmi_init_adapter(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init adapter failed\n");
		return -1;
	}

	/* sunxi hdmi core level init */
	hdmi->hdmi_core.dev       = hdmi->dev;
	hdmi->hdmi_core.i2c_adap  = &hdmi->i2c_adap;
	hdmi->hdmi_core.connect   = &hdmi->sdrm.connector;
	hdmi->hdmi_core.clock_src = hdmi->hdmi_ctrl.drv_dts_clk_src;
	hdmi->hdmi_core.resistor_src = hdmi->hdmi_ctrl.drv_dts_res_src;
	hdmi->hdmi_core.smooth_boot  = hdmi->hdmi_ctrl.drv_boot_enable;
	ret = sunxi_hdmi_init(&hdmi->hdmi_core);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init core failed!!!\n");
		return -1;
	}

	return 0;
}

static int _sunxi_hdmi_init_drm(struct sunxi_drm_hdmi *hdmi)
{
	int ret = 0;
	struct sunxi_drm_device   *sdrm = &hdmi->sdrm;
	struct drm_device         *drm  = sdrm->drm_dev;
	struct device_node	  *node = sdrm->tcon_dev->of_node;
	struct drm_encoder    *encoder  = &sdrm->encoder;
	struct drm_connector  *connect  = &sdrm->connector;
	uint64_t min_val = 0x0, max_val = 0xFFFF;

	if (IS_ERR_OR_NULL(node)) {
		shdmi_err(node);
		return -1;
	}

	/* drm encoder register */
	drm_encoder_helper_add(encoder, &sunxi_hdmi_encoder_helper_funcs);
	ret = drm_simple_encoder_init(drm, encoder, DRM_MODE_ENCODER_TMDS);
	if (ret != 0) {
		hdmi_err("drm hdmi register encoder failed\n");
		return -1;
	}
	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm, node);

	/* drm connector register */
	connect->polled            = DRM_CONNECTOR_POLL_HPD;
	connect->connector_type    = DRM_MODE_CONNECTOR_HDMIA;
	connect->interlace_allowed = true;
	connect->ycbcr_420_allowed = true;
	drm_connector_helper_add(connect, &sunxi_hdmi_connector_helper_funcs);
	ret = drm_connector_init_with_ddc(drm, connect,
			&sunxi_hdmi_connector_funcs, DRM_MODE_CONNECTOR_HDMIA,
			&hdmi->i2c_adap);
	if (ret != 0) {
		drm_encoder_cleanup(encoder);
		hdmi_err("drm hdmi register connector failed\n");
		return -1;
	}

	/* bind connector and encoder */
	drm_connector_attach_encoder(connect, encoder);

	if (hdmi->hdmi_ctrl.drv_max_bits == DISP_DATA_8BITS)
		max_val = 0xF;
	else if (hdmi->hdmi_ctrl.drv_max_bits == DISP_DATA_10BITS)
		max_val = 0xFF;
	else if (hdmi->hdmi_ctrl.drv_max_bits == DISP_DATA_12BITS)
		max_val = 0xFFF;
	else
		max_val = 0xFFFF;

	/* drm connector propert register */
	hdmi->prop_pxfmt_cap = sunxi_drm_create_attach_property_enum(drm,
			&connect->base, "pixelformat_support", shdmi_prop_list_pxfmt,
			ARRAY_SIZE(shdmi_prop_list_pxfmt), hdmi->hdmi_ctrl.drv_pixel_format_cap);

	hdmi->prop_pxfmt = sunxi_drm_create_attach_property_range(drm,
			&connect->base, "pixelformat",
			min_val, max_val, hdmi->hdmi_ctrl.drv_pixel_format);

	hdmi->prop_dr_cap = sunxi_drm_create_attach_property_enum(drm,
			&connect->base, "dynamicrange_support", shdmi_prop_list_dr,
			ARRAY_SIZE(shdmi_prop_list_dr), hdmi->hdmi_ctrl.drv_dynamic_range_cap);

	max_val = 0x1F;
	hdmi->prop_dr = sunxi_drm_create_attach_property_range(drm,
			&connect->base, "dynamicrange",
			min_val, max_val, hdmi->hdmi_ctrl.drv_dynamic_range);

	hdmi_inf("drm hdmi connector create finish\n");
	return 0;
}

static int sunxi_hdmi_bind(struct device *dev, struct device *master, void *data)
{
	int ret = 0;
	bool boot_state = 0;
	struct drm_device *drm = NULL;
	struct sunxi_drm_hdmi  *hdmi = NULL;

	if (IS_ERR_OR_NULL(dev)) {
		shdmi_err(dev);
		goto bind_ng;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		goto bind_ng;
	}

	hdmi = devm_kzalloc(dev, sizeof(struct sunxi_drm_hdmi), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		goto bind_ng;
	}
	dev_set_drvdata(dev, hdmi);

	drm = (struct drm_device *)data;
	hdmi->dev = dev;
	hdmi->sdrm.drm_dev = drm;
	hdmi->sdrm.hw_id   = 0;

	ret = _sunxi_hdmi_init_get_tcon(dev);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init get tcon_dev failed\n");
		goto bind_ng;
	}

	boot_state = sunxi_drm_check_device_boot_enabled(drm,
			DRM_MODE_CONNECTOR_HDMIA,
			hdmi->sdrm.hw_id);

	/* init hdmi device sysfs */
	ret = _sunxi_hdmi_init_sysfs(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init sysfs fail\n");
		goto bind_ng;
	}

	/* init hdmi driver hardware */
	ret = _sunxi_hdmi_init_drv(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init drv fail\n");
		goto bind_ng;
	}

	ret = _sunxi_hdmi_init_drm(hdmi);
	if (ret != 0) {
		hdmi_err("sunxi hdmi init creat connect failed\n");
		goto bind_ng;
	}

	hdmi->hdmi_ctrl.drv_boot_enable = boot_state && sunxi_hdmi_get_hpd();
	if (boot_state) {
		hdmi->hdmi_ctrl.drv_enable = 0x1;
		_sunxi_drv_hdmi_hpd_set(hdmi, 0x1);
		_sunxi_drv_hdcp_update_support(hdmi);
	} else
		_sunxi_drv_hdmi_hpd_set(hdmi, 0x0);

	if (IS_ERR_OR_NULL(hdmi->hpd_task)) {
		goto bind_ng;
	} else {
		wake_up_process(hdmi->hpd_task);
		hdmi_trace("hdmi init start hpd detect task\n");
	}

	hdmi_inf("hdmi devices bind done <<<<<<<<<<\n\n");
	return 0;

bind_ng:
	hdmi_err("hdmi devices bind failed!!!!!\n");
	return -1;
}

/*******************************************************************************
 *  sunxi hdmi deinitialization function
 ******************************************************************************/
static int _sunxi_hdmi_deinit_sysfs(struct device *dev)
{
	dev_t dev_id;
	struct kobject *kobj = NULL;
	struct class   *clas = NULL;
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		goto ret_err;
	}

	dev_id = hdmi->hdmi_devid;
	kobj = &hdmi->hdmi_dev->kobj;
	clas = hdmi->hdmi_class;

	if (IS_ERR_OR_NULL(kobj)) {
		shdmi_err(kobj);
		goto ret_err;
	}

	if (IS_ERR_OR_NULL(clas)) {
		shdmi_err(clas);
		goto ret_err;
	}

	sysfs_remove_group(kobj, &_sunxi_hdmi_group);

	device_destroy(clas, dev_id);

	class_destroy(clas);

	unregister_chrdev_region(dev_id, 1);

	return 0;
ret_err:
	return -1;
}

static int _sunxi_hdmi_deinit_drv(struct device *dev)
{
	struct sunxi_drm_hdmi *hdmi = dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return -1;
	}

	if (hdmi->hpd_task) {
		/* destory hdmi hpd thread */
		kthread_stop(hdmi->hpd_task);
		hdmi->hpd_task = NULL;
	}

	sunxi_hdmi_exit();

	i2c_del_adapter(&hdmi->i2c_adap);

	cec_notifier_cec_adap_unregister(hdmi->hdmi_cec.notify, hdmi->hdmi_cec.adap);

	cec_unregister_adapter(hdmi->hdmi_cec.adap);

	_sunxi_drv_hdmi_regulator_off(hdmi);

	hdmi = NULL;
	return 0;
}

static void sunxi_hdmi_unbind(struct device *dev, struct device *master, void *data)
{
	int ret = 0;

	ret = _sunxi_hdmi_deinit_sysfs(dev);
	if (ret != 0)
		hdmi_wrn("hdmi dev exit failed\n\n");

	ret = _sunxi_hdmi_deinit_drv(dev);
	if (ret != 0)
		hdmi_wrn("hdmi drv exit failed\n");

	hdmi_inf("hdmi drv unbind done\n");
}

static const struct of_device_id sunxi_hdmi_match[] = {
	{ .compatible =	"allwinner,sunxi-hdmi" },
	{ }
};

static const struct component_ops sunxi_hdmi_compoent_ops = {
	.bind   = sunxi_hdmi_bind,
	.unbind = sunxi_hdmi_unbind,
};

static int sunxi_hdmi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	return component_add(dev, &sunxi_hdmi_compoent_ops);
}

static int sunxi_hdmi_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	component_del(dev, &sunxi_hdmi_compoent_ops);

	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	return 0;
}

struct platform_driver sunxi_hdmi_platform_driver = {
	.probe  = sunxi_hdmi_probe,
	.remove = sunxi_hdmi_remove,
	.driver = {
		   .name = "allwinner,sunxi-hdmi",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_hdmi_match,
		   #ifdef CONFIG_PM_SLEEP
		   .pm = &sunxi_hdmi_pm_ops,
		   #endif
	},
};

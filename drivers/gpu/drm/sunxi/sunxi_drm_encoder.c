/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui <cuiyuntao@allwinnter.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_core.h"
#include "subdev/sunxi_lcd.h"
#include "subdev/sunxi_hdmi.h"
#include "subdev/sunxi_common.h"
#include "drm_de/drm_al.h"

bool sunxi_tcon_common_query_irq(void *irq_data, int need_irq);

struct sunxi_drm_encoder *
    get_sunxi_enc(struct drm_device *dev, unsigned int nr)
{
	struct drm_encoder *enc = NULL;

	list_for_each_entry(enc, &dev->mode_config.encoder_list, head) {
		if (to_sunxi_encoder(enc)->enc_id == nr) {
			return to_sunxi_encoder(enc);
		}
	}

	return NULL;
}

struct drm_connector *sunxi_get_conct(struct drm_encoder *encoder)
{
	struct drm_connector *conct;

	list_for_each_entry(conct, &encoder->dev->mode_config.connector_list, head) {
		if (conct->encoder == encoder) {
			return conct;
		}
	}

	return NULL;
}

static void sunxi_drm_encoder_reset(struct drm_encoder *encoder)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_enc->hw_res;

	if(hw_res->ops && hw_res->ops->reset)
		hw_res->ops->reset(encoder);
}

void sunxi_drm_encoder_enable(struct drm_encoder *encoder)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_enc->hw_res;

	if(hw_res && hw_res->ops &&
		hw_res->ops->enable && !hw_res->en_ctl_by) {
		hw_res->ops->enable(encoder);
	}
}

/* disable encoder when not in use - more explicit than dpms off */
void sunxi_drm_encoder_disable(struct drm_encoder *encoder)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res;
	bool inused = drm_helper_encoder_in_use(encoder);
	hw_res = sunxi_enc->hw_res;

	if (hw_res && hw_res->ops &&
		hw_res->ops->disable && (!hw_res->en_ctl_by||!inused)) {
		hw_res->ops->disable(encoder);
	}
}

static void sunxi_drm_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res = sunxi_enc->hw_res;

	/* on :clk, irq, output ok
	* standby suspend: disable irq
	* off: disable irq,
	* if on we must display the content when off
	*/

	bool inused = drm_helper_encoder_in_use(encoder);
	switch (mode) {
	case  DRM_MODE_DPMS_ON:
		if(inused) {
			sunxi_drm_encoder_enable(encoder);
		}
		break;
	case DRM_MODE_DPMS_OFF:
		sunxi_drm_encoder_disable(encoder);
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		sunxi_irq_disable(hw_res);
		break;
	default:
		return;
	}
	sunxi_enc->dpms = mode;
}

static bool sunxi_drm_encoder_mode_fixup(struct drm_encoder *encoder,
		   const struct drm_display_mode *mode,
		   struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void sunxi_drm_encoder_prepare(struct drm_encoder *encoder)
{
	/*set the clock and crtc global_cfg*/
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);

	sunxi_drm_encoder_disable(encoder);
	sunxi_drm_encoder_reset(encoder);

	sunxi_set_global_cfg(encoder->crtc, MANAGER_TCON_DIRTY, &sunxi_enc->enc_id);
}

static void sunxi_drm_encoder_commit(struct drm_encoder *encoder)
{
	sunxi_drm_encoder_enable(encoder);
	return ;
}

static inline void sunxi_drm_encoder_set_timming(struct drm_encoder *encoder,
            struct drm_display_mode *mode)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res = sunxi_enc->hw_res;

	if (hw_res && hw_res->ops && hw_res->ops->set_timming)
		hw_res->ops->set_timming(encoder, mode);
}

static void sunxi_drm_encoder_mode_set(struct drm_encoder *encoder,
		struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode)
{
	/*set the hardware Tcon or to mem cache of the register*/
	sunxi_drm_encoder_set_timming(encoder, mode);
}

static struct drm_encoder_helper_funcs sunxi_encoder_helper_funcs = {
	.dpms = sunxi_drm_encoder_dpms,
	.mode_fixup = sunxi_drm_encoder_mode_fixup,
	.mode_set = sunxi_drm_encoder_mode_set,
	.prepare = sunxi_drm_encoder_prepare,
	.commit = sunxi_drm_encoder_commit,
	.disable = sunxi_drm_encoder_disable,
};

int sunxi_encoder_updata_delayed(struct drm_encoder *encoder)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res = sunxi_enc->hw_res;

	return hw_res->ops->user_def(encoder);
}

irqreturn_t sunxi_encoder_vsync_handle(int irq, void *data)
{
	struct drm_connector *conct;
	struct drm_encoder *encoder = (struct drm_encoder *)data;
	struct sunxi_drm_connector *sunxi_con;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(encoder);
	if (!drm_helper_encoder_in_use(data)) {
		/* avoid disable the irq with the user thread, return */
		return IRQ_HANDLED;
	}

	if (!sunxi_irq_query(sunxi_encoder->hw_res, encoder, QUERY_VSYNC_IRQ))
		return IRQ_HANDLED;

	list_for_each_entry(conct,
		&encoder->dev->mode_config.connector_list, head) {
		if (conct->encoder == encoder) {
			sunxi_con = to_sunxi_connector(conct);
		if(sunxi_con->hw_res != NULL &&
			sunxi_con->hw_res->ops != NULL &&
			sunxi_con->hw_res->ops->vsync_proc != NULL) {

			sunxi_con->hw_res->ops->vsync_proc(sunxi_con);
			}
		}
	}

	sunxi_crtc = to_sunxi_crtc(encoder->crtc);
	if(sunxi_crtc != NULL &&
		sunxi_crtc->hw_res != NULL &&
		sunxi_crtc->hw_res->ops != NULL &&
		sunxi_crtc->hw_res->ops->vsync_proc != NULL) {

		sunxi_crtc->hw_res->ops->vsync_proc(sunxi_crtc);
	}

	if(sunxi_encoder != NULL &&
		sunxi_encoder->hw_res != NULL &&
		sunxi_encoder->hw_res->ops != NULL &&
		sunxi_encoder->hw_res->ops->vsync_proc != NULL) {

		sunxi_encoder->hw_res->ops->vsync_proc(sunxi_encoder);
	}
	return IRQ_HANDLED;
}

static void sunxi_drm_encoder_destroy(struct drm_encoder *encoder)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(encoder);
	struct sunxi_hardware_res *hw_res;

	hw_res = sunxi_encoder->hw_res;
	drm_encoder_cleanup(encoder);
	sunxi_hwres_destroy(hw_res);
	kfree(sunxi_encoder);
}

static struct drm_encoder_funcs sunxi_encoder_funcs = {
	.reset = sunxi_drm_encoder_reset,
	.destroy = sunxi_drm_encoder_destroy,
};

bool sunxi_tcon_common_reset(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;

	hw_res = sunxi_encoder->hw_res;

	sunxi_clk_set(hw_res);
	sunxi_clk_enable(hw_res);
	tcon_init(sunxi_encoder->enc_id);
	sunxi_irq_request(hw_res);

	return true;
}

bool sunxi_tcon_common_init(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_encoder->hw_res;

	if (hw_res->clk)
		hw_res->parent_clk = clk_get_parent(hw_res->clk);

	return true;
}

bool sunxi_tcon_hdmi_init(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_encoder->hw_res;

	return true;
}

bool sunxi_tcon_common_enable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;

	hw_res = sunxi_encoder->hw_res;
	sunxi_clk_enable(hw_res);
	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	return true;
}

bool sunxi_tcon_common_disable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_encoder->hw_res;

	if (sunxi_encoder->enc_id == 1)
		tcon1_close(1);
	if (sunxi_encoder->enc_id == 0)
		tcon0_close(0);

	sunxi_irq_free(hw_res);
	sunxi_clk_disable(hw_res);

	return true;
}

int sunxi_tcon_common_set_timing(void *data, struct drm_display_mode *mode)
{
	return 0;
}

int sunxi_tcon_common_check_line(void *data)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(data);
	int cur_line = 0, start_delay = 0;

	cur_line = tcon_get_cur_line(sunxi_enc->enc_id, sunxi_enc->part_id);
	start_delay = tcon_get_start_delay(sunxi_enc->enc_id, sunxi_enc->part_id);

	return start_delay - cur_line - ENCODER_UPDATA_DELAYED;
}

#ifdef SUPPORT_DSI
bool sunxi_tcon_dsi_enable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
			to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	struct sunxi_lcd_private *lcd_private;

	hw_res = sunxi_encoder->hw_res;
	lcd_private = (struct sunxi_lcd_private *)hw_res->private;

	tcon0_open(sunxi_encoder->enc_id, lcd_private->panel);
	dsi_open(sunxi_encoder->enc_id, lcd_private->panel);

	DRM_DEBUG_KMS("[%d]\n",__LINE__);

	return true;
}

bool sunxi_tcon_dsi_disable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_encoder->hw_res;

	dsi_close(sunxi_encoder->enc_id);
	tcon0_close(sunxi_encoder->enc_id);
	tcon_exit(sunxi_encoder->enc_id);

	sunxi_irq_free(hw_res);
	sunxi_clk_disable(hw_res);

	return true;
}

int sunxi_tcon_dsi_set_timing(void *data, struct drm_display_mode *mode)
{
	struct sunxi_drm_encoder *sunxi_encoder =
	to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	struct sunxi_lcd_private *lcd_private;

	hw_res = sunxi_encoder->hw_res;
	lcd_private = (struct sunxi_lcd_private *)hw_res->private;

	tcon0_set_dclk_div(sunxi_encoder->enc_id, lcd_private->clk_info->tcon_div);
	tcon0_cfg(sunxi_encoder->enc_id, lcd_private->panel);
	tcon0_cfg_ext(sunxi_encoder->enc_id, lcd_private->extend_panel);
	return 0;
}

int sunxi_tcon_dsi_check_line(void *data)
{
	struct sunxi_drm_encoder *sunxi_enc = to_sunxi_encoder(data);
	int cur_line = 0, start_delay = 0;

	cur_line = dsi_get_cur_line(sunxi_enc->enc_id);
	start_delay = dsi_get_start_delay(sunxi_enc->enc_id);

	return start_delay - cur_line - ENCODER_UPDATA_DELAYED;
}
#endif

bool sunxi_tcon_lvds_enable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	struct sunxi_lcd_private *lcd_private;

	hw_res = sunxi_encoder->hw_res;
	lcd_private = (struct sunxi_lcd_private *)hw_res->private;

	tcon0_open(sunxi_encoder->enc_id, lcd_private->panel);
	lvds_open(sunxi_encoder->enc_id, lcd_private->panel);

	DRM_DEBUG_KMS("[%d]\n",__LINE__);

	return true;
}

bool sunxi_tcon_lvds_disable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_encoder->hw_res;

	lvds_close(sunxi_encoder->enc_id);
	tcon0_close(sunxi_encoder->enc_id);
	tcon_exit(sunxi_encoder->enc_id);

	sunxi_irq_free(hw_res);
	sunxi_clk_disable(hw_res);

    	return true;
}

int sunxi_tcon_lvds_set_timing(void *data, struct drm_display_mode *mode)
{
	struct sunxi_drm_encoder *sunxi_encoder =
	to_sunxi_encoder(data);
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_lcd_private *lcd_private;
	int crtc_id = 0;
	sunxi_crtc = to_sunxi_crtc(sunxi_encoder->drm_encoder.crtc);
	crtc_id = sunxi_crtc ? sunxi_crtc->crtc_id : 0;

	DRM_DEBUG_KMS("[%d] encoder_id:%d\n",__LINE__, sunxi_encoder->enc_id);

	hw_res = sunxi_encoder->hw_res;
	lcd_private = (struct sunxi_lcd_private *)hw_res->private;

	tcon0_set_dclk_div(sunxi_encoder->enc_id, lcd_private->clk_info->tcon_div);
	tcon0_cfg(sunxi_encoder->enc_id, lcd_private->panel);
	tcon0_cfg_ext(sunxi_encoder->enc_id, lcd_private->extend_panel);
	tcon0_src_select(sunxi_encoder->enc_id, LCD_SRC_DE, crtc_id);

	    return 0;
}

bool sunxi_tcon_hdmi_reset(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;
	hw_res = sunxi_encoder->hw_res;
	sunxi_irq_request(hw_res);
	return true;
}

bool sunxi_tcon_hdmi_enable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(data);
	tcon1_open(sunxi_encoder->enc_id);
	return true;
}

bool sunxi_tcon_hdmi_disable(void *data)
{
	struct sunxi_drm_encoder *sunxi_encoder =
			to_sunxi_encoder(data);
	struct sunxi_hardware_res *hw_res;

	hw_res = sunxi_encoder->hw_res;

	tcon1_close(sunxi_encoder->enc_id);
	tcon_exit(sunxi_encoder->enc_id);
	tcon1_hdmi_clk_enable(sunxi_encoder->enc_id, 0);

	sunxi_irq_free(hw_res);
	sunxi_clk_disable(hw_res);

	return true;
}

int sunxi_tcon_hdmi_set_timing(void *data, struct drm_display_mode *mode)
{
	struct sunxi_drm_encoder *sunxi_encoder =
	to_sunxi_encoder(data);
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_hdmi_private *hdmi_private;
	struct disp_video_timings *hdmi_timing;
	int crtc_id = 0;
	sunxi_crtc = to_sunxi_crtc(sunxi_encoder->drm_encoder.crtc);
	crtc_id = sunxi_crtc ? sunxi_crtc->crtc_id : 0;
	hw_res = sunxi_encoder->hw_res;
	hdmi_private = (struct sunxi_hdmi_private *)hw_res->private;
	hdmi_timing = hdmi_private->timing;
	if (MODE_OK != sunxi_hdmi_mode_timmings(hdmi_timing, mode)) {
		DRM_ERROR("failed to change mode to hdmi timming.\n");
		return -EINVAL;
	}

	DRM_INFO("[%d] encoder_id: %d vic:%d\n",__LINE__,
		sunxi_encoder->enc_id, hdmi_timing->vic);

	hw_res->clk_rate = hdmi_timing->pixel_clk * (hdmi_timing->pixel_repeat + 1);
	sunxi_clk_set(hw_res);
	sunxi_clk_enable(hw_res);
	tcon1_hdmi_clk_enable(sunxi_encoder->enc_id,1);//need check Jet Cui
	tcon_init(sunxi_encoder->enc_id);

	sunxi_set_global_cfg(sunxi_encoder->drm_encoder.crtc,
			MANAGER_COLOR_SPACE_DIRTY, &hdmi_private->video_para.is_yuv);

	tcon1_set_timming(sunxi_encoder->enc_id, hdmi_timing);
	tcon1_hdmi_color_remap(sunxi_encoder->enc_id, hdmi_private->video_para.is_yuv);
	tcon1_src_select(sunxi_encoder->enc_id, LCD_SRC_DE, crtc_id);

	return 0;
}

bool sunxi_tcon_updata_reg(void *data)
{
	return true;
}

void sunxi_tcon_vsync_proc(void *data)
{
	/* */
	return;
}

#ifdef SUPPORT_DSI
struct sunxi_hardware_ops tcon_dsi_ops = {
	.init = sunxi_tcon_common_init,
	.reset = sunxi_tcon_common_reset,
	.enable = sunxi_tcon_dsi_enable,
	.disable = sunxi_tcon_dsi_disable,
	.updata_reg = sunxi_tcon_updata_reg,
	.vsync_proc = sunxi_tcon_vsync_proc,
	.irq_query = sunxi_tcon_common_query_irq,
	.vsync_delayed_do = NULL,
	.set_timming = sunxi_tcon_dsi_set_timing,
	.user_def = sunxi_tcon_dsi_check_line,
};
#endif

struct sunxi_hardware_ops tcon_lvds_ops = {
	.init = sunxi_tcon_common_init,
	.reset = sunxi_tcon_common_reset,
	.enable = sunxi_tcon_lvds_enable,
	.disable = sunxi_tcon_lvds_disable,
	.updata_reg = sunxi_tcon_updata_reg,
	.vsync_proc = sunxi_tcon_vsync_proc,
	.irq_query = sunxi_tcon_common_query_irq,
	.vsync_delayed_do = NULL,
	.set_timming = sunxi_tcon_lvds_set_timing,
	.user_def = sunxi_tcon_common_check_line,
};

struct sunxi_hardware_ops tcon_hdmi_ops = {
	.init = sunxi_tcon_hdmi_init,
	.reset = sunxi_tcon_hdmi_reset,
	.enable = sunxi_tcon_hdmi_enable,
	.disable = sunxi_tcon_hdmi_disable,
	.updata_reg = sunxi_tcon_updata_reg,
	.vsync_proc = sunxi_tcon_vsync_proc,
	.irq_query = sunxi_tcon_common_query_irq,
	.vsync_delayed_do = NULL,
	.set_timming = sunxi_tcon_hdmi_set_timing,
	.user_def = sunxi_tcon_common_check_line,
};

struct sunxi_hardware_ops tcon_tv_ops = {
	.init = sunxi_tcon_common_init,
	.reset = sunxi_tcon_common_reset,
	.enable = sunxi_tcon_common_enable,
	.disable = sunxi_tcon_common_disable,
	.updata_reg = sunxi_tcon_updata_reg,
	.vsync_proc = sunxi_tcon_vsync_proc,
	.irq_query = sunxi_tcon_common_query_irq,
	.vsync_delayed_do = NULL,
	.set_timming = sunxi_tcon_common_set_timing,
	.user_def = sunxi_tcon_common_check_line,
};

bool sunxi_tcon_common_query_irq(void *irq_data, int need_irq)
{
	struct sunxi_drm_encoder *sunxi_encoder =
		to_sunxi_encoder(irq_data);
#ifdef CONFIG_ARCH_SUN8IW11
	enum __lcd_irq_id_t id;
#endif
#ifdef CONFIG_ARCH_SUN50IW1P1
	__lcd_irq_id_t id;
#endif
#ifdef CONFIG_ARCH_SUN50IW2P1
	enum __lcd_irq_id_t id;
#endif

	if (QUERY_VSYNC_IRQ == need_irq) {
		if (sunxi_encoder->part_id == 1) {
			id = LCD_IRQ_TCON1_VBLK;
		}else{
			id = LCD_IRQ_TCON0_VBLK;
		}
	}else {
		return false;
	}

	if(tcon_irq_query(sunxi_encoder->enc_id, id))
		return true;
	return false;
}

int sunxi_drm_encoder_create(struct drm_device *dev, unsigned int possible_crtcs,
				int fix_crtc, int id, struct sunxi_hardware_res *hw_res)
{
	struct sunxi_drm_encoder *sunxi_enc;
	struct drm_encoder *encoder = NULL;
	struct sunxi_drm_crtc *sunxi_crtc = NULL;
	DRM_DEBUG_KMS("[%d]\n",__LINE__);

	if (!hw_res) {
		DRM_ERROR("failed to allocate encoder\n");
		return -EINVAL;
	}

	sunxi_enc = kzalloc(sizeof(*sunxi_enc), GFP_KERNEL);
	if (!sunxi_enc) {
		DRM_ERROR("failed to allocate encoder\n");
		return -EINVAL;
	}

	sunxi_enc->enc_id = id;
	sunxi_enc->hw_res = hw_res;
	if (hw_res != NULL) {
		hw_res->irq_arg = (void *)sunxi_enc;
		hw_res->irq_handle = sunxi_encoder_vsync_handle;
	}

	encoder = &sunxi_enc->drm_encoder;
	encoder->possible_crtcs = possible_crtcs;
	drm_encoder_init(dev, encoder, &sunxi_encoder_funcs,
			DRM_MODE_ENCODER_TMDS);

	drm_encoder_helper_add(encoder, &sunxi_encoder_helper_funcs);
	if (fix_crtc != -1)
		sunxi_crtc = get_sunxi_crt(dev, fix_crtc);
	encoder->crtc = &sunxi_crtc->drm_crtc;
	DRM_DEBUG_KMS("[%d] encoder_id:%d(%d) possible_crtcs:0x%x\n", __LINE__,
	    sunxi_enc->enc_id, encoder->base.id, possible_crtcs);

	return 0;
}

int sunxi_encoder_assign_ops(struct drm_device *dev, int nr,
    enum encoder_ops_type type, void *private)
{
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_lcd_private *lcd_p;

	sunxi_encoder = get_sunxi_enc(dev, nr);
	if (!sunxi_encoder)
	return -EINVAL;

	sunxi_encoder->hw_res->private = private;
	hw_res = sunxi_encoder->hw_res;
	switch(type) {
#ifdef SUPPORT_DSI
	case ENCODER_OPS_DSI:
		lcd_p = (struct sunxi_lcd_private *)private;
		hw_res->ops = &tcon_dsi_ops;
		hw_res->clk_rate = lcd_p->panel->lcd_dclk_freq * 1000000
		* lcd_p->clk_info->dsi_div;
		hw_res->parent_clk_rate = hw_res->clk_rate * lcd_p->clk_info->lcd_div;
		hw_res->en_ctl_by = 1;
		sunxi_encoder->part_id = 0;
		break;
#endif
	case ENCODER_OPS_LVDS:
		lcd_p = (struct sunxi_lcd_private *)private;
		hw_res->ops = &tcon_lvds_ops;
		hw_res->clk_rate = lcd_p->panel->lcd_dclk_freq * 1000000
		*lcd_p->clk_info->tcon_div;
		hw_res->parent_clk_rate = hw_res->clk_rate * lcd_p->clk_info->lcd_div;
		hw_res->en_ctl_by = 1;
		sunxi_encoder->part_id = 0;
		break;
	case ENCODER_OPS_TV:
		hw_res->ops = &tcon_tv_ops;
		sunxi_encoder->part_id = 1;
		break;
	case ENCODER_OPS_HDMI:
		hw_res->ops = &tcon_hdmi_ops;
		sunxi_encoder->part_id = 1;
		/* we must set the clk when set timing for hdmi */
		break;
	default:
		DRM_ERROR("give us a err ops\n");
		return -EINVAL;
	}

	if (hw_res->ops->init) {
		if (!hw_res->ops->init(sunxi_encoder)) {
			DRM_ERROR("init encoder err.\n");
			return -EINVAL;
		}
	}
	/* Currently the tcon just support 1 ops for connector */
	/* must assign 1 times, avoid mutex with vsync handle. becareful */
	DRM_INFO("[%d]:type:%d parent_rate:%lu  rate:%lu. encoder[%d][%d][%d] \n",__LINE__, type,
			hw_res->parent_clk_rate, hw_res->clk_rate, sunxi_encoder->enc_id,sunxi_encoder->part_id,
			sunxi_encoder->drm_encoder.base.id);

	return 0;
}

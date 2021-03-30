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

#include <drm/sunxi_drm.h>
#include "sunxi_drm_core.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_panel.h"
#include "subdev/sunxi_lcd.h"
#include "subdev/sunxi_common.h"
#include "drm_de/drm_al.h"


struct sunxi_drm_connector* 
    get_sunxi_conct(struct drm_device *dev, unsigned int nr)
{
	struct drm_connector *conct;

	list_for_each_entry(conct, &dev->mode_config.connector_list, head) {
		if (to_sunxi_connector(conct)->connector_id == nr) {
			return to_sunxi_connector(conct);
		}
	}
	return NULL;
}

int sunxi_drm_deal_hotplug(struct drm_device *dev)
{
	return 0;
}

static int sunxi_drm_connector_get_modes(struct drm_connector *connector)
{
	struct sunxi_drm_connector *sunxi_connector =
			to_sunxi_connector(connector);
	unsigned int count = 0;
	struct sunxi_panel *sunxi_panel = sunxi_connector->panel;
	struct panel_ops   *panel_ops = sunxi_panel->panel_ops;


	if(panel_ops && panel_ops->get_modes)
		count =  panel_ops->get_modes(sunxi_panel);
	DRM_DEBUG_KMS("[%d] id:%d count:%d\n", __LINE__, connector->base.id, count);

	return count;
}

static int sunxi_drm_connector_mode_valid(struct drm_connector *connector,
					    struct drm_display_mode *mode)
{
	struct sunxi_drm_connector *sunxi_connector =
				to_sunxi_connector(connector);
	struct sunxi_panel *sunxi_panel = sunxi_connector->panel;
	struct panel_ops   *panel_ops = sunxi_panel->panel_ops;

	int ret = MODE_BAD;

	if(panel_ops && panel_ops->check_valid_mode)
		ret =  panel_ops->check_valid_mode(sunxi_panel, mode);
	DRM_DEBUG_KMS("[%d] valid:%d\n", __LINE__, ret);
	return ret;
}

struct drm_encoder *sunxi_drm_best_encoder(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_mode_object *obj;
	struct drm_encoder *encoder = NULL, *best_encoder = NULL;
	int i;


	for (i = 0; i < DRM_CONNECTOR_MAX_ENCODER; i++) {
		if (connector->encoder_ids[i] != 0) {
			obj = drm_mode_object_find(dev, connector->encoder_ids[i],
							DRM_MODE_OBJECT_ENCODER);
			if (!obj) {
				DRM_DEBUG_KMS("Unknown ENCODER ID %d\n",
				connector->encoder_ids[i]);
				continue;
			}

			encoder = obj_to_encoder(obj);
			if (best_encoder == NULL)
				best_encoder = encoder;

			if (connector->encoder == encoder) {
				best_encoder = encoder;
				break;
			}else{
				if(!drm_helper_encoder_in_use(encoder)) {
					best_encoder = encoder;
					break;
				}
			}
		}
	}
	DRM_DEBUG_KMS("[%d] id:%d.\n", __LINE__, best_encoder->base.id);
	return best_encoder;
}

static struct drm_connector_helper_funcs sunxi_connector_helper_funcs = {
	.get_modes	= sunxi_drm_connector_get_modes,
	.mode_valid	= sunxi_drm_connector_mode_valid,
	.best_encoder	= sunxi_drm_best_encoder,
};

void sunxi_drm_connector_enable(struct drm_connector *connector)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res  *hw_res;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_connector = to_sunxi_connector(connector);
	hw_res = sunxi_connector->hw_res;
	if (hw_res != NULL &&
		hw_res->ops != NULL &&
		hw_res->ops->enable != NULL) {
		hw_res->ops->enable(connector);
	}
}

void sunxi_drm_connector_disable(struct drm_connector *connector)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res  *hw_res;

	sunxi_connector = to_sunxi_connector(connector);
	hw_res = sunxi_connector->hw_res;
	if (hw_res != NULL &&
		hw_res->ops != NULL &&
		hw_res->ops->disable != NULL) {
		hw_res->ops->disable(connector);
	}
}

void sunxi_drm_connector_set_timming(struct drm_connector *connector)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct drm_display_mode *mode = NULL;
	struct sunxi_hardware_res *hw_res;

	sunxi_connector = to_sunxi_connector(connector);
	hw_res = sunxi_connector->hw_res;
	if (connector->encoder != NULL &&
		connector->encoder->crtc != NULL) {
		mode =  &connector->encoder->crtc->mode;
	}
	DRM_DEBUG_KMS("[%d] id:%d mode_id:%d\n", __LINE__,
		connector->base.id, mode->base.id);

	if (hw_res->ops->set_timming) {
		hw_res->ops->set_timming(connector, mode);
	}
}

void sunxi_drm_display_power(struct drm_connector *connector, int mode)
{
	struct sunxi_drm_connector *sunxi_connector;
	sunxi_connector = to_sunxi_connector(connector);

	DRM_DEBUG_KMS("[%d]id:%d  mode:%d pre:(%d)\n", __LINE__,
	    connector->base.id, mode, sunxi_connector->dmps);
	switch (mode) {
	case DRM_MODE_DPMS_ON:
		if (sunxi_connector->dmps > DRM_MODE_DPMS_ON && connector->encoder) {
			sunxi_drm_connector_set_timming(connector);
			sunxi_drm_connector_reset(connector);
			sunxi_drm_connector_enable(connector);
		}
		if (!connector->encoder)
		    return;
		break;
	case DRM_MODE_DPMS_STANDBY:
		/* TODO */
		break;
	case DRM_MODE_DPMS_SUSPEND:
		/* TODO */
		break;
	case DRM_MODE_DPMS_OFF:
		if (sunxi_connector->dmps == DRM_MODE_DPMS_ON)
			sunxi_drm_connector_disable(connector);
		break;
	default:
		return;
	}
	sunxi_connector->dmps = mode;
}

static void sunxi_drm_connector_dpms(struct drm_connector *connector,
					int mode)
{
	struct sunxi_drm_connector *sunxi_connector;
	sunxi_connector = to_sunxi_connector(connector);

	DRM_DEBUG_KMS("[%d] id:%d  power:%d  pre :%d\n", __LINE__,
		sunxi_connector->connector_id, mode, sunxi_connector->dmps);

	/* drm_helper_connector_dpms:connector->dpms = mode */
	/* fix for the lcd power up flow */
	if (mode >= sunxi_connector->dmps)
		sunxi_drm_display_power(connector, mode);

	drm_helper_connector_dpms(connector, mode);

	if (mode < sunxi_connector->dmps)
		sunxi_drm_display_power(connector, mode);

}

/* get detection status of display device. */
static enum drm_connector_status
sunxi_drm_connector_detect(struct drm_connector *connector, bool force)
{
	struct sunxi_drm_connector *sunxi_connector =
		to_sunxi_connector(connector);
	enum drm_connector_status status = connector_status_disconnected;

	if (sunxi_connector->panel->panel_ops->detect)
		status = sunxi_connector->panel->panel_ops->detect(sunxi_connector->panel);

//	DRM_INFO("[%d]  status:%d\n", __LINE__, status);

	//if (status == connector_status_disconnected)
	//sunxi_drm_connector_dpms(connector, DRM_MODE_DPMS_OFF);
	//if (status == connector_status_connected)
	//sunxi_drm_connector_dpms(connector, DRM_MODE_DPMS_ON);
	return status;
}

void sunxi_chain_enable(struct drm_connector *connector,
        enum chain_bit_mask id)
{

	struct drm_encoder *encoder;
	struct sunxi_drm_encoder *sunxi_enc;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_drm_connector *sunxi_con;
	encoder = connector->encoder;
	/* if there the order, will add future */

	if (id & CHAIN_BIT_CONNECTER) {
		sunxi_con = to_sunxi_connector(connector);
		hw_res = sunxi_con->hw_res;
		if (hw_res && hw_res->ops &&
			hw_res->ops->enable && hw_res->en_ctl_by) {

			hw_res->ops->enable(encoder);
		} 
	}

	if (id & CHAIN_BIT_ENCODER) {
		sunxi_enc = to_sunxi_encoder(encoder);
		hw_res = sunxi_enc->hw_res;
		if (hw_res && hw_res->ops &&
			hw_res->ops->enable && hw_res->en_ctl_by) {

			hw_res->ops->enable(encoder);
		} 
	}

	if (id & CHAIN_BIT_CRTC) {
		if (encoder && encoder->crtc) {
			sunxi_crtc = to_sunxi_crtc(encoder->crtc);
			hw_res = sunxi_crtc->hw_res;
			if (hw_res && hw_res->ops &&
				hw_res->ops->enable && hw_res->en_ctl_by) {

				hw_res->ops->enable(encoder);
			}
		}
	}
}

void sunxi_chain_disable(struct drm_connector *connector,
        enum chain_bit_mask id)
{

	struct drm_encoder *encoder;
	struct sunxi_drm_encoder *sunxi_enc;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_drm_connector *sunxi_con;
	struct sunxi_hardware_res *hw_res;

	encoder = connector->encoder;
	/* if there the order, will add future */
	if (id & CHAIN_BIT_CONNECTER) {
		sunxi_con = to_sunxi_connector(connector);
		hw_res = sunxi_con->hw_res;
		if (hw_res && hw_res->ops &&
			hw_res->ops->disable && hw_res->en_ctl_by) {

			hw_res->ops->disable(encoder);
		} 
	}

	if (id & CHAIN_BIT_ENCODER) {
		sunxi_enc = to_sunxi_encoder(encoder);
		hw_res = sunxi_enc->hw_res;
		if (hw_res && hw_res->ops &&
			hw_res->ops->disable && hw_res->en_ctl_by) {

			hw_res->ops->disable(encoder);
		} 
	}

	if (id & CHAIN_BIT_CRTC) {
		if (encoder && encoder->crtc) {
			sunxi_crtc = to_sunxi_crtc(encoder->crtc);
			hw_res = sunxi_crtc->hw_res;
			if (hw_res && hw_res->ops &&
				hw_res->ops->disable && hw_res->en_ctl_by)

				hw_res->ops->disable(encoder);
		}
	}
}

void sunxi_drm_connector_reset(struct drm_connector *connector)
{
	/*commit the mode to the connector*/
	struct sunxi_drm_connector *sunxi_connector =
		to_sunxi_connector(connector);

	DRM_DEBUG_KMS("id:%d\n", connector->base.id);
	/*modify the irq chain, and register it*/
	if (!connector->encoder ||
		!connector->encoder->crtc) {
		sunxi_irq_free(sunxi_connector->hw_res);
		sunxi_connector->hw_res->ops->disable(connector);
		return;
	}
	sunxi_connector->hw_res->ops->reset(connector);
}

static void sunxi_drm_connector_destroy(struct drm_connector *connector)
{
	struct sunxi_drm_connector *sunxi_connector =
		to_sunxi_connector(connector);
	struct sunxi_hardware_res *hw_res;

	DRM_DEBUG_KMS("id:%d\n",connector->base.id);

	hw_res = sunxi_connector->hw_res;
	drm_sysfs_connector_remove(connector);
	drm_connector_cleanup(connector);
	if (sunxi_connector->panel) {
		sunxi_panel_destroy(sunxi_connector->panel);
	}
	if (hw_res) {
		/* no need to free hw_res twice, the first time is in sunxi_panel_destroy */
		//sunxi_hwres_destroy(hw_res);
	}
	kfree(sunxi_connector);
}

irqreturn_t sunxi_connector_vsync_handle(int irq, void *data)
{
	struct drm_encoder *encoder;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_connector *sunxi_con;
	struct drm_connector *conct = (struct drm_connector *)(data);

	sunxi_con = to_sunxi_connector(conct);
	encoder = conct->encoder;
	sunxi_encoder = to_sunxi_encoder(encoder);
	sunxi_crtc = to_sunxi_crtc(encoder->crtc);
	if (!encoder || !sunxi_crtc) {
		return IRQ_HANDLED;
	}

	if (!sunxi_irq_query(sunxi_con->hw_res, conct, QUERY_VSYNC_IRQ))
		return IRQ_HANDLED;

	if (sunxi_con->hw_res != NULL &&
		sunxi_con->hw_res->ops != NULL &&
		sunxi_con->hw_res->ops->vsync_proc != NULL) {

		sunxi_con->hw_res->ops->vsync_proc(sunxi_crtc);
	}

	if (sunxi_crtc->hw_res != NULL &&
		sunxi_crtc->hw_res->ops != NULL &&
		sunxi_crtc->hw_res->ops->vsync_proc != NULL) {

		sunxi_crtc->hw_res->ops->vsync_proc(sunxi_crtc);
	}

	if (sunxi_encoder->hw_res != NULL &&
		sunxi_encoder->hw_res->ops != NULL &&
		sunxi_encoder->hw_res->ops->vsync_proc != NULL) {

		sunxi_encoder->hw_res->ops->vsync_proc(sunxi_encoder);
	}

	return IRQ_HANDLED;
}

static int sunxi_drm_connector_set_pro(struct drm_connector *connector,
        struct drm_property *property, uint64_t val)
{
	struct sunxi_drm_connector *sunxi_connector =
		to_sunxi_connector(connector);
	DRM_DEBUG_KMS("[%d] id:%d\n",__LINE__, connector->base.id);

	sunxi_connector = sunxi_connector;
	return 0;
}

static struct drm_connector_funcs sunxi_connector_funcs = {
	.dpms		= sunxi_drm_connector_dpms,
	.fill_modes	= drm_helper_probe_single_connector_modes,
	.detect		= sunxi_drm_connector_detect,
	.destroy	= sunxi_drm_connector_destroy,
	.set_property = sunxi_drm_connector_set_pro,
	.reset        = sunxi_drm_connector_reset,
};

int sunxi_drm_connector_create(struct drm_device *dev, int possible_enc, int fix_enc, int id,
     struct sunxi_panel *panel, enum disp_output_type disp_out_type, struct sunxi_hardware_res *hw_res)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct drm_connector *connector;
	struct drm_encoder	 *encoder;
	struct sunxi_lcd_private  *sunxi_lcd_p = NULL;
	int type;
	int err;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_connector = kzalloc(sizeof(*sunxi_connector), GFP_KERNEL);
	if (!sunxi_connector) {
		DRM_ERROR("failed to allocate connector\n");
		return -EINVAL;
	}

	connector = &sunxi_connector->drm_connector;
	switch (disp_out_type) {
	case DISP_OUTPUT_TYPE_LCD:
		connector->interlace_allowed = false;
		connector->polled = 0;
		sunxi_connector->disp_out_type = DISP_OUTPUT_TYPE_LCD;
		sunxi_lcd_p = (struct sunxi_lcd_private *)panel->private;

		switch (sunxi_lcd_p->panel->lcd_if) {
		case LCD_IF_HV:
			type = DRM_MODE_CONNECTOR_Unknown;
			break;
		case LCD_IF_CPU:
			type = DRM_MODE_CONNECTOR_Unknown;
			break;
		case LCD_IF_LVDS:
				type = DRM_MODE_CONNECTOR_LVDS;
			if (sunxi_encoder_assign_ops(dev, fix_enc, ENCODER_OPS_LVDS, panel->private))
				goto out;
			break;
		case LCD_IF_DSI:
			type = DRM_MODE_CONNECTOR_Unknown;
			if (sunxi_encoder_assign_ops(dev, fix_enc, ENCODER_OPS_DSI, panel->private))
				goto out;
			break;
		case LCD_IF_EDP:
			type = DRM_MODE_CONNECTOR_eDP;
			break;
		case LCD_IF_EXT_DSI:
			type = DRM_MODE_CONNECTOR_Unknown;
			break;
		default:
			DRM_ERROR("give us a err lcd panel type.\n");
			goto out;
		}
		break;
	case DISP_OUTPUT_TYPE_HDMI:
		type = DRM_MODE_CONNECTOR_HDMIA;
		connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
		sunxi_connector->disp_out_type = DISP_OUTPUT_TYPE_HDMI;
		connector->interlace_allowed = 1;
		err = sunxi_encoder_assign_ops(dev, fix_enc, ENCODER_OPS_HDMI, panel->private);
		if (err && fix_enc != -1)
			goto out;
		break;
	case DISP_OUTPUT_TYPE_TV:
		type = DRM_MODE_CONNECTOR_TV;
		connector->polled = 0;
		sunxi_connector->disp_out_type = DISP_OUTPUT_TYPE_TV;
		err = sunxi_encoder_assign_ops(dev, fix_enc, ENCODER_OPS_TV, panel->private);
		if (err && fix_enc != -1)
			goto out;
		break;
	case DISP_OUTPUT_TYPE_VGA:
		type = DRM_MODE_CONNECTOR_VGA;
		connector->polled = 0;
		sunxi_connector->disp_out_type = DISP_OUTPUT_TYPE_VGA;
		err = sunxi_encoder_assign_ops(dev, fix_enc, ENCODER_OPS_VGA, panel->private);
		if (err && fix_enc != -1)
			goto out;
		break;

	default:
		type = DRM_MODE_CONNECTOR_Unknown;
		DRM_ERROR("give us a err type disp_out_type.\n");
		goto out;
	}

	connector->dpms = DRM_MODE_DPMS_OFF;
	sunxi_connector->dmps = DRM_MODE_DPMS_OFF;
	sunxi_connector->panel = panel;
	sunxi_connector->connector_id = id;
	panel->drm_connector = &sunxi_connector->drm_connector;
	panel->type = sunxi_connector->disp_out_type; // when call sunxi_panel_destroy, it will not find any device, so add it;

	sunxi_connector->hw_res = hw_res;
	if (hw_res != NULL) {
		hw_res->irq_arg = (void *)sunxi_connector;
		hw_res->irq_handle = sunxi_connector_vsync_handle;
		if (hw_res->ops && hw_res->ops->init)
			hw_res->ops->init(sunxi_connector);
	}

	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		if (possible_enc & 1) {
			err = drm_mode_connector_attach_encoder(connector, encoder);
			if (err) {
				DRM_ERROR("failed to attach a connector to a encoder\n");
			}
		}
		possible_enc >>= 1; 
	}

	drm_connector_init(dev, connector, &sunxi_connector_funcs, type);
	drm_connector_helper_add(connector, &sunxi_connector_helper_funcs);
	err = drm_sysfs_connector_add(connector);
	if (err)
		goto err_connector;

	DRM_INFO("[%d]sunxi_con_id:%d(%d), fix:%d\n", __LINE__,
		sunxi_connector->connector_id, connector->base.id,  fix_enc);

	return 0;

err_connector:
	drm_connector_cleanup(connector);
out:
	kfree(sunxi_connector);
	return -EINVAL;
}


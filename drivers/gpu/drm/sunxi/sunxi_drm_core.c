/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include "sunxi_drm_core.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "subdev/sunxi_lcd.h"
#include "subdev/sunxi_hdmi.h"
#include "subdev/sunxi_common.h"
#include "drm_de/drm_al.h"
#include "subdev/sunxi_rotate.h"

static bool has_res[DISP_MOD_NUM+3][3] = {
	/* A64:{reg_base,irq,clk} */
	{1,1,1}, /*DISP_MOD_DE*/
#if defined(HAVE_DEVICE_COMMON_MODULE)
	{1,0,0},  /*DISP_MOD_DEVICE*/
	{0,1,1},  /*DISP_MOD_LCD0*/
	{0,1,1},  /*DISP_MOD_LCD1*/
	{0,1,1},  /*DISP_MOD_LCD2*/
	{0,1,1},  /*DISP_MOD_LCD3*/
#else
	{0,0,0},  /*DISP_MOD_DEVICE*/
	{1,1,1},  /*DISP_MOD_LCD0*/
	{1,1,1},  /*DISP_MOD_LCD1*/
	{1,1,1},  /*DISP_MOD_LCD2*/
	{1,1,1},  /*DISP_MOD_LCD3*/
#endif
#if defined(SUPPORT_DSI)
	{1,1,1},  /*DISP_MOD_DSI0*/
#else
	{0,0,0},  /*DISP_MOD_DSI0*/
#endif
	{0,0,0},  /*DISP_MOD_DSI1*/
	{0,0,0},  /*DISP_MOD_DSI2*/
	{1,1,1},  /*DISP_MOD_HDMI, but not in display device*/
	{0,0,1},  /*DISP_MOD_LVDS*/
	{1,1,1},  /*DISP_MOD_EINK*/
	{1,1,1}  /*DISP_MOD_EDMA*/    
};

bool sunxi_find_irq_res(struct drm_connector *connector, unsigned int *irq,
    void **irq_arg, irqreturn_t(**irq_handle)(int, void *), bool *enalbed)
{
	struct sunxi_drm_connector    *sunxi_connector;
	struct sunxi_drm_encoder	  *sunxi_encoder;
	struct sunxi_drm_crtc         *sunxi_crtc;
	struct sunxi_hardware_res     *hw_res;

	sunxi_connector = to_sunxi_connector(connector);
	hw_res = sunxi_connector->hw_res;

	if (hw_res && hw_res->irq_uesd) {

		*irq = hw_res->irq_no;
		*irq_arg = hw_res->irq_arg;
		*irq_handle = sunxi_crtc_vsync_handle;
		*enalbed = hw_res->irq_enable;
		return true;
	}

	sunxi_encoder = to_sunxi_encoder(connector->encoder);
	if (sunxi_encoder) {
		hw_res = sunxi_connector->hw_res;
	} else {
		hw_res = NULL;
	}
	if (hw_res && hw_res->irq_uesd) {
		*irq = hw_res->irq_no;
		*irq_arg = hw_res->irq_arg;
		*irq_handle = sunxi_crtc_vsync_handle;
		*enalbed = hw_res->irq_enable;
		return true;
	}

	sunxi_crtc = to_sunxi_crtc(sunxi_encoder->drm_encoder.crtc);
	if (sunxi_crtc) {
		hw_res = sunxi_connector->hw_res;
	} else {
		hw_res = NULL;
	}
	if (hw_res && hw_res->irq_uesd) {
		*irq = hw_res->irq_no;
		*irq_arg = hw_res->irq_arg;
		*irq_handle = sunxi_crtc_vsync_handle;
		*enalbed = hw_res->irq_enable;
		return true;
	}

	*irq = 0;
	*irq_arg = NULL;
	*irq_handle = NULL;
	return false;
}


int sunxi_drm_get_res_info(disp_bsp_init_para *para,
        struct sunxi_hardware_res *hw_res, bool has_res[3])
{

	if (has_res[0]) {
		hw_res->reg_base = para->reg_base[hw_res->res_id];
	}
	if (has_res[1]) {
		hw_res->irq_no = para->irq_no[hw_res->res_id];
	}
	if (has_res[2]) {
		hw_res->clk = para->mclk[hw_res->res_id];
	}
	return 0;
}

struct sunxi_hardware_res *sunxi_hwres_init(disp_mod_id id)
{
	struct sunxi_hardware_res *hw_res;
	hw_res = kzalloc(sizeof(struct sunxi_hardware_res), GFP_KERNEL);  
	if (!hw_res) {
		DRM_ERROR("failed to alloc lcd sunxi_hardware_res.\n");
		return NULL;
	}
	hw_res->res_id = id;
	return hw_res;
}

void sunxi_hwres_destroy(struct sunxi_hardware_res *hw_res)
{
	sunxi_clk_disable(hw_res);
	sunxi_irq_free(hw_res);
	kfree(hw_res);
}

/* same with display driver */
int sunxi_display_init(disp_bsp_init_para *para)
{
	struct device_node *node;
	int i = 0, counter = 0;


	memset(para, 0, sizeof(disp_bsp_init_para));
	node = sunxi_drm_get_name_node("sunxi-disp");
	if (!node) {
		DRM_ERROR("get sunxi-disp node err.\n ");
		return -EINVAL;
	}
	para->reg_base[DISP_MOD_DE] = (uintptr_t __force)of_iomap(node, counter);
	if (!para->reg_base[DISP_MOD_DE]) {
		DRM_ERROR( "unable to map de registers\n");
		goto err_iomap;
	}
	counter ++;

#if defined(HAVE_DEVICE_COMMON_MODULE)
	para->reg_base[DISP_MOD_DEVICE] = (uintptr_t __force)of_iomap(node, counter);
	if (!para->reg_base[DISP_MOD_DEVICE]) {
		DRM_ERROR( "unable to map device common module registers\n");
		goto err_iomap;
	}
	counter ++;
#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		para->reg_base[DISP_MOD_LCD0 + i] = (uintptr_t __force)of_iomap(node, counter);
		if (!para->reg_base[DISP_MOD_LCD0 + i]) {
			DRM_ERROR( "unable to map timing controller %d registers\n", i);
			goto err_iomap;
		}
		counter ++;
	}

#if defined(SUPPORT_DSI)
	para->reg_base[DISP_MOD_DSI0] = (uintptr_t __force)of_iomap(node, counter);
	if (!para->reg_base[DISP_MOD_DSI0]) {
		DRM_ERROR("unable to map dsi registers\n");
		goto err_iomap;
	}
	counter ++;
#endif

#if defined(SUPPORT_EINK)
	para->reg_base[DISP_MOD_EINK] = (uintptr_t __force)of_iomap(node, counter);
	if (!para->reg_base[DISP_MOD_EINK]) {
		DRM_ERROR( "unable to map eink registers\n");
		goto err_iomap;
	}
	counter ++;
#endif

	/* parse and map irq */
	/* lcd0/1/2.. - dsi */
	counter = 0;
	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		para->irq_no[DISP_MOD_LCD0 + i] = irq_of_parse_and_map(node, counter);
		if (!para->irq_no[DISP_MOD_LCD0 + i]) {
			DRM_ERROR("irq_of_parse_and_map irq %d fail for tcon%d\n", counter, i);
		}
		counter ++;
	}

#if defined(SUPPORT_DSI)
	para->irq_no[DISP_MOD_DSI0] = irq_of_parse_and_map(node, counter);
	if (!para->irq_no[DISP_MOD_DSI0]) {
		DRM_ERROR("irq_of_parse_and_map irq %d fail for dsi\n", i);
	}
	counter ++;
#endif

#if defined(SUPPORT_EINK)
	para->irq_no[DISP_MOD_DE] = irq_of_parse_and_map(node, counter);
	if (!para->irq_no[DISP_MOD_DE]) {
		DRM_ERROR("irq_of_parse_and_map de irq %d fail for dsi\n", i);
	}
	counter ++;

	para->irq_no[DISP_MOD_EINK] = irq_of_parse_and_map(node, counter);
	if (!para->irq_no[DISP_MOD_EINK]) {
		DRM_ERROR("irq_of_parse_and_map eink irq %d fail for dsi\n", i);
	}
	counter ++;
#endif


	/* get clk */
	/* de - [device(tcon-top)] - lcd0/1/2.. - lvds - dsi */
	counter = 0;
	para->mclk[DISP_MOD_DE] = of_clk_get(node, counter);
	if (IS_ERR(para->mclk[DISP_MOD_DE])) {
		DRM_ERROR("fail to get clk for de\n");
	}
	counter ++;

#if defined(HAVE_DEVICE_COMMON_MODULE)
	para->mclk[DISP_MOD_DEVICE] = of_clk_get(node, counter);
	if (IS_ERR(para->mclk[DISP_MOD_DEVICE])) {
		DRM_ERROR("fail to get clk for device common module\n");
	}
	counter ++;
#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		para->mclk[DISP_MOD_LCD0 + i] = of_clk_get(node, counter);
		if (IS_ERR(para->mclk[DISP_MOD_LCD0 + i])) {
			DRM_ERROR("fail to get clk for timing controller%d\n", i);
		}
		counter ++;
	}

#if defined(SUPPORT_LVDS)
	para->mclk[DISP_MOD_LVDS] = of_clk_get(node, counter);
	if (IS_ERR(para->mclk[DISP_MOD_LVDS])) {
		DRM_ERROR("fail to get clk for lvds\n");
	}
	counter ++;
#endif

#if defined(SUPPORT_DSI)
	para->mclk[DISP_MOD_DSI0] = of_clk_get(node, counter);
	DRM_INFO("[dengbo]: sunxi_display_init 15\n");
	if (IS_ERR(para->mclk[DISP_MOD_DSI0])) {
		DRM_ERROR( "fail to get clk for dsi\n");
	}
	counter ++;
#endif

#if defined(SUPPORT_EINK)
	para->mclk[DISP_MOD_EINK] = of_clk_get(node, counter);
	DRM_INFO("[dengbo]: sunxi_display_init 16\n");
	if (IS_ERR(para->mclk[DISP_MOD_EINK])) {
		DRM_ERROR("fail to get clk for eink\n");
	}
	counter ++;

	para->mclk[DISP_MOD_EDMA] = of_clk_get(node, counter);
	if (IS_ERR(para->mclk[DISP_MOD_EDMA])) {
		DRM_ERROR("fail to get clk for edma\n");
	}
	counter ++;
#endif
    return 0;

err_iomap:
	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		if (para->reg_base[i])
			iounmap((char __iomem *)para->reg_base[i]);
	}

	return -EINVAL;
}

int sunxi_drm_init(struct drm_device *dev)
{
	int i, j, ret, max_crtc, max_enc, max_connector;
	int possible_up = 0x00, fix_up = -1, asigned = 0x00;
	int lcd_id = 0, hdmi_id = 0;
	struct sunxi_panel *sunxi_panel;
	struct sunxi_hardware_res *hw_res;
	disp_bsp_init_para para;
	disp_mod_id id;
	enum disp_output_type disp_out_type;
	struct resource *res_irq;

	if (sunxi_display_init(&para)) {
		goto err;
	}
	dev->vblank_disable_allowed = 1;
	dev->irq_enabled = 1;
	sunxi_drm_init_al(&para);
	max_crtc = sunxi_drm_get_max_crtc();
	max_enc = sunxi_drm_get_max_encoder();
	max_connector = sunxi_drm_get_max_connector();

	res_irq =  kzalloc(sizeof(struct resource), GFP_KERNEL);
		if (res_irq == NULL) {
		DRM_ERROR("fail to alloc res_irq for drm.\n");
	}

	init_crtc_array(max_crtc);
	for (i = 0, id = DISP_MOD_DE; i < max_crtc; i++) {
		hw_res = sunxi_hwres_init(DISP_MOD_DE);
		if (!hw_res) {
		goto err;
		}
		ret = sunxi_drm_get_res_info(&para, hw_res, has_res[id]);
		if (ret) {
			sunxi_hwres_destroy(hw_res);
			goto err;
		}

		ret = sunxi_drm_crtc_create(dev, i, hw_res);
		if (ret) {
			sunxi_hwres_destroy(hw_res);
			goto err;
		}
		DRM_DEBUG_KMS("[%d]:id[%d] reg_base[0x%lx] irq_id[%d]\n",__LINE__,
		hw_res->res_id, hw_res->reg_base, hw_res->irq_no);
	}
#ifdef HAVE_DEVICE_COMMON_MODULE
	id = DISP_MOD_DEVICE;
#else
	id = DISP_MOD_LCD0;
#endif
	possible_up = 0x03;
	for (i = 0; i < max_enc; i++ ) {
#ifdef HAVE_DEVICE_COMMON_MODULE
		hw_res = sunxi_hwres_init(DISP_MOD_DEVICE);
#else
		hw_res = sunxi_hwres_init(DISP_MOD_LCD0 + i);
		id += i;;
#endif
		if (!hw_res) {
			goto err;
		}

		fix_up = i;
		if (fix_up >= max_crtc)
			fix_up = -1;
		ret = sunxi_drm_get_res_info(&para, hw_res, has_res[id]);
		if (ret) {
			sunxi_hwres_destroy(hw_res);
			goto err;
		}

		ret = sunxi_drm_encoder_create(dev, possible_up, fix_up, i, hw_res);
		if (ret) {
			goto err;
		}
		DRM_DEBUG_KMS("[%d]:id[%d] reg_base[0x%lx] irq_id[%d]\n",__LINE__, 
		hw_res->res_id, hw_res->reg_base, hw_res->irq_no);
		if (res_irq && hw_res->irq_no && has_res[id][1]) {
			/* for drm_wait_vblank */
			res_irq->start = hw_res->irq_no;
			res_irq->flags = IORESOURCE_IRQ;
			res_irq->name = "drm_irq";
			platform_device_add_resources(dev->platformdev, res_irq, 1);
			kfree(res_irq);
			res_irq = NULL;
		}
	}
	asigned = 0x00;
	for (i = 0, id = DISP_MOD_DSI0, lcd_id = 0, hdmi_id = 0;
			(id < DISP_MOD_NUM) && (i < max_connector);
			id++) {
		hw_res = sunxi_hwres_init(id);
		if (!hw_res) {
			goto err;
		}
		fix_up = -1;
		possible_up = 0;
		disp_out_type = DISP_OUTPUT_TYPE_NONE;
		switch (id) {
		case DISP_MOD_DSI0:
		case DISP_MOD_DSI1:
		case DISP_MOD_DSI2:
		case DISP_MOD_LVDS:
			sunxi_panel = sunxi_lcd_init(hw_res, i, lcd_id);
			if (!sunxi_panel) {
			sunxi_hwres_destroy(hw_res);
			continue;
			}
			ret = sunxi_drm_get_res_info(&para, hw_res, has_res[id]);
			if (ret) {
				sunxi_hwres_destroy(hw_res);
				goto err;
			}
			for (j = 0; j < max_enc; j++) {
				if (sunxi_drm_encoder_support(j, DISP_OUTPUT_TYPE_LCD)) {
					possible_up |= 1<<j;
					if(!(asigned & (1 << j)) && fix_up == -1) {
						asigned |= 1 << j;
						fix_up = j;
					}
				}
			}
			disp_out_type = DISP_OUTPUT_TYPE_LCD;
			lcd_id++;
			break;
		case DISP_MOD_HDMI:
			sunxi_panel = sunxi_hdmi_pan_init(hw_res, i, hdmi_id);
			if (!sunxi_panel) {
				sunxi_hwres_destroy(hw_res);
				continue;
			}
			for (j = 0; j < max_enc; j++) {
				if (sunxi_drm_encoder_support(j, DISP_OUTPUT_TYPE_HDMI)) {
					possible_up |= 1<<j;
					if (!(asigned & (1 << j)) && fix_up == -1) {
						asigned |= 1 << j;
						fix_up = j;
					}
				}
			}
			disp_out_type = DISP_OUTPUT_TYPE_HDMI;
			hdmi_id++;
			break;
		default:
			DRM_ERROR("temple we don't support %d.\n", id);
			sunxi_hwres_destroy(hw_res);
			continue;
		}
		DRM_DEBUG_KMS("[%d]:id[%d] reg_base[0x%lx] irq_id[%d]\n", __LINE__,
			hw_res->res_id, hw_res->reg_base, hw_res->irq_no);

		ret = sunxi_drm_connector_create(dev, possible_up, fix_up, i, sunxi_panel, disp_out_type, hw_res);
		if (ret) {
		sunxi_lcd_destroy(sunxi_panel, hw_res);
		sunxi_hwres_destroy(hw_res);
		continue;
		}
		i++;
	}

	if (i == 0)
		goto err;
	if (sunxi_drm_rotate_init(dev->dev_private))
		DRM_ERROR("rotate hw init err.\n");

	return 0;
err:
    	return -EINVAL;
}

void sunxi_drm_destroy(struct drm_device *dev)
{
    	drm_mode_config_cleanup(dev);
}

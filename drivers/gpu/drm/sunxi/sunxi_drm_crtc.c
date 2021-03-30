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
/*CRT -->Display Engine*/

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <linux/clk.h>
#include <drm/sunxi_drm.h>
#include "sunxi_drm_drv.h"
#include "sunxi_drm_core.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_plane.h"
#include "drm_de/drm_al.h"
#include "subdev/sunxi_common.h"
#include "sunxi_drm_fb.h"

static struct drm_crtc **crtc_array;

int init_crtc_array(int num)
{
	crtc_array = kzalloc(sizeof(struct drm_crtc *) *num, GFP_KERNEL);
	if (!crtc_array) {
		DRM_ERROR("sunxi crtc must have hardware resource.\n");
		return -EINVAL;
	}
	return 0;
}

struct drm_encoder *sunxi_get_encoder(struct drm_crtc *crtc)
{
	struct drm_encoder *encoder;
	list_for_each_entry(encoder, &crtc->dev->mode_config.encoder_list, head) {
		if (encoder->crtc == crtc) {
			return encoder;
		}
	}
	return NULL;
}

struct drm_connector  *sunxi_crtc_get_conector(struct drm_crtc *crtc)
{

	struct drm_encoder  *encoder;
	encoder = sunxi_get_encoder(crtc);
	if(!encoder)
		return NULL;
	return sunxi_get_conct(encoder);
}

struct sunxi_drm_crtc *get_sunxi_crt(struct drm_device *dev, int nr)
{
	struct drm_crtc *crtc = NULL;

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		if (to_sunxi_crtc(crtc)->crtc_id == nr) {
			return to_sunxi_crtc(crtc);
		}
	}
	return NULL;
}

enum disp_output_type get_sunxi_crtc_out_type(int nr)
{
	struct drm_crtc *crtc = NULL;
	struct drm_encoder	 *drm_encoder;
	struct drm_connector *drm_connector;
	bool find = 0;

	if (crtc_array == NULL)
		goto ret_err;
	crtc = crtc_array[nr];
	DRM_DEBUG_KMS("[%d] \n",__LINE__);

	list_for_each_entry(drm_encoder, &crtc->dev->mode_config.encoder_list, head) {
		if (drm_encoder->crtc == crtc) {
			find =1;
			break;
		}
	}

	if (!find) 
		goto ret_err;

	list_for_each_entry(drm_connector, &crtc->dev->mode_config.encoder_list, head) {
		if (drm_connector->encoder == drm_encoder) {
		return to_sunxi_connector(drm_connector)->disp_out_type;
		}
	}

ret_err:
	return DISP_OUTPUT_TYPE_NONE;
}

static inline struct drm_encoder *sunxi_crtc_get_encoder(struct drm_crtc *crtc)
{
	struct drm_encoder *encoder;

	list_for_each_entry(encoder, &crtc->dev->mode_config.encoder_list, head) {
		if (encoder->crtc == crtc) {
			return encoder;
		}
	}

	return NULL;
}

static void sunxi_drm_crtc_disable(struct drm_crtc *crtc)
{
	/* disable is the crtc does not be used, so becareful */
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	struct sunxi_hardware_ops *hw_ops = sunxi_crtc->hw_res->ops;

	DRM_DEBUG_KMS("[%d] crtc_id:%d \n",__LINE__,sunxi_crtc->crtc_id);

	hw_ops->disable(crtc);
}

static void sunxi_drm_crtc_enable(struct drm_crtc *crtc)
{
	/* disable is the crtc does not be used, so becareful */
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	struct sunxi_hardware_ops *hw_ops = sunxi_crtc->hw_res->ops;

	DRM_DEBUG_KMS("[%d] crtc_id:%d \n",__LINE__,sunxi_crtc->crtc_id);

	hw_ops->enable(crtc);
}

static void sunxi_drm_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	/* dpm just open the dmps , nothing about crtc used */
	struct sunxi_drm_crtc *sunxi_crtc;
	bool inused, enable;
	struct sunxi_hardware_ops *hw_ops;

	DRM_DEBUG_KMS("[%d] mode[%d]\n",__LINE__,mode);

	sunxi_crtc = to_sunxi_crtc(crtc);
	hw_ops = sunxi_crtc->hw_res->ops;

	/* on :clk, irq, output ok
	* standby suspend: disable irq
	* off: disable irq,
	* if on we must display the content when off
	*/

	inused = drm_helper_crtc_in_use(crtc);
	switch (mode) {
	case DRM_MODE_DPMS_ON:
		if (inused) {
			if (sunxi_crtc->dpms > mode) {
				/* make sure display the last off content. */
				enable = 1;
				sunxi_set_global_cfg(crtc, MANAGER_ENABLE_DIRTY, &enable);
				sunxi_crtc->update_frame_user_cnt++;
			}
			sunxi_drm_crtc_enable(&sunxi_crtc->drm_crtc);
		}
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		sunxi_irq_disable(sunxi_crtc->hw_res);
		break;
	case DRM_MODE_DPMS_OFF:
		if (!inused) {
			sunxi_drm_crtc_finish_pageflip(sunxi_crtc->drm_crtc.dev, sunxi_crtc);
			sunxi_crtc->update_frame_user_cnt = 0;
			sunxi_crtc->update_frame_ker_cnt = 0;
		}
		drm_vblank_off(crtc->dev, sunxi_crtc->crtc_id);
		enable = 0;
		/* in fact disable the cfg not updata to hardware */
		sunxi_set_global_cfg(crtc, MANAGER_ENABLE_DIRTY, &enable);
		sunxi_drm_crtc_disable(&sunxi_crtc->drm_crtc);
		break;
	default:
		return;
	}
	sunxi_crtc->dpms = mode;
}

static void sunxi_drm_crtc_prepare(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	//struct disp_manager_data *global_cfgs = sunxi_crtc->global_cfgs;

	DRM_DEBUG_KMS("[%d]\n",__LINE__);

	/* we colse the enhance, smartlight etc. we reset the hw */
	//memset(&global_cfgs->config, 0, sizeof(struct disp_manager_info));

	sunxi_drm_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);
	sunxi_reset_plane(crtc);
	sunxi_drm_crtc_dpms(crtc, DRM_MODE_DPMS_ON);
	/*find the fb plane*/
	if(sunxi_set_fb_plane(crtc, 0, 0))
		DRM_ERROR("failed set fb_plane.\n");
	/* clear the wait for event*/
	sunxi_drm_crtc_finish_pageflip(sunxi_crtc->drm_crtc.dev, sunxi_crtc);
	sunxi_crtc->user_skip_cnt = 0;
	sunxi_crtc->ker_skip_cnt = 0;
	sunxi_crtc->ker_update_frame = 0;
	sunxi_crtc->update_frame_ker_cnt = 0;
	sunxi_crtc->update_frame_user_cnt = 0;
}

static void sunxi_drm_crtc_commit(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	struct sunxi_hardware_ops *ops = sunxi_crtc->hw_res->ops;

	DRM_DEBUG_KMS("[%d]  crtc_id:%d\n", __LINE__, sunxi_crtc->crtc_id);
	/*commit the global cfg and plane cfgs to the reg or memory cache*/
	ops->updata_reg(crtc);
}

static bool sunxi_drm_crtc_mode_fixup(struct drm_crtc *crtc,
			    const struct drm_display_mode *mode,
			    struct drm_display_mode *adjusted_mode)
{
	DRM_DEBUG_KMS("[%d]\n", __LINE__);
	/* check it  for the linebuf  and other?*/
	return true;
}

bool sunxi_set_global_cfg(struct drm_crtc *crtc,
        int type, void *data)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	struct disp_manager_data *global_cfg = sunxi_crtc->global_cfgs;
	struct drm_display_mode *mode = NULL;

	DRM_DEBUG_KMS("[%d] id:%d\n", __LINE__, crtc->base.id);

	switch (type) {
	/* screen windows size */
	case MANAGER_ENABLE_DIRTY:
		global_cfg->config.enable = *((bool *)data);
		global_cfg->flag |= MANAGER_ENABLE_DIRTY;
		break;
	case MANAGER_SIZE_DIRTY:
		mode = (struct drm_display_mode *)(data);
		global_cfg->config.size.width = mode->hdisplay;
		global_cfg->config.size.height = mode->vdisplay;
		global_cfg->flag |= MANAGER_SIZE_DIRTY;
		break;
	case MANAGER_BACK_COLOR_DIRTY:
		/* background  color*/
		global_cfg->config.back_color.alpha = 0;
		global_cfg->config.back_color.red = 0;
		global_cfg->config.back_color.blue = 0;
		global_cfg->config.back_color.green = 0;
		global_cfg->flag |= MANAGER_BACK_COLOR_DIRTY;
		break;
	case MANAGER_CK_DIRTY:
		/*color key*/
		global_cfg->flag |= MANAGER_CK_DIRTY;
		break;
	case MANAGER_COLOR_RANGE_DIRTY:
		global_cfg->flag |= MANAGER_COLOR_RANGE_DIRTY;
		break;
	case MANAGER_COLOR_SPACE_DIRTY:
		global_cfg->config.cs = *((char *)(data));
		global_cfg->flag |= MANAGER_COLOR_SPACE_DIRTY;
		break;
	case MANAGER_BLANK_DIRTY:
		global_cfg->config.blank = *((bool *)data);
		global_cfg->flag |= MANAGER_BLANK_DIRTY;
		break;
	case MANAGER_DEHZ_DIRTY:
		global_cfg->config.de_freq = *(unsigned int*)(data);
		break;
	case MANAGER_TCON_DIRTY:
		global_cfg->config.hwdev_index = *((int*)(data));
		global_cfg->flag |= MANAGER_ENABLE_DIRTY;
		break;
	default:
		global_cfg->flag = MANAGER_ALL_DIRTY;
		DRM_ERROR("failed page flip request.\n");
	}

	return true;
}

static int sunxi_crtc_updata_fb(struct drm_crtc *crtc, int x, int y)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	unsigned int crtc_w;
	unsigned int crtc_h;
	unsigned int fb_src_w;
	unsigned int fb_src_h;
	struct sunxi_drm_framebuffer *sunxi_fb;
	struct drm_plane *plane = sunxi_crtc->fb_plane;

	if (plane == NULL) {
		DRM_ERROR("get fb plane err.\n");
		return -EINVAL; 
	}
	sunxi_fb = to_sunxi_fb(crtc->fb);
	crtc_w = crtc->mode.hdisplay;
	crtc_h = crtc->mode.vdisplay;

	fb_src_w = crtc->fb->width - x;
	fb_src_h = crtc->fb->height - y;
	if (sunxi_fb->fb_flag & SUNXI_FBDEV_FLAGS) {
		fb_src_h = crtc->fb->height / FBDEV_BUF_NUM;
	}

	return sunxi_update_plane(plane, crtc, crtc->fb, 0, 0, crtc_w, crtc_h,
				    x<<16, y<<16, fb_src_w<<16, fb_src_h<<16);
}

static int sunxi_drm_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
					  struct drm_framebuffer *old_fb)
{
	int ret;

	DRM_DEBUG_KMS("[%d] %p\n",__LINE__, crtc->fb);

	ret = sunxi_crtc_updata_fb(crtc, x, y);
	if (ret)
		return ret;
	sunxi_drm_crtc_commit(crtc);

	return 0;
}

static int sunxi_drm_crtc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
			  struct drm_display_mode *adjusted_mode, int x, int y,
			  struct drm_framebuffer *old_fb)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);

	DRM_DEBUG_KMS("[%d] \n",__LINE__);

	if (!crtc->fb) {
		DRM_ERROR("give us a NULL fb, disable the crtc.\n");
		sunxi_drm_crtc_disable(crtc);
		return -EINVAL;
	}

	sunxi_set_global_cfg(crtc, MANAGER_SIZE_DIRTY, adjusted_mode);
	sunxi_drm_crtc_updata_fps(sunxi_crtc->crtc_id, adjusted_mode->vrefresh);

	return sunxi_crtc_updata_fb(crtc, x, y);
}

static void sunxi_drm_crtc_load_lut(struct drm_crtc *crtc)
{
	DRM_DEBUG_KMS("[%d] \n",__LINE__);
	/* drm framework doesn't check NULL */
}

static struct drm_crtc_helper_funcs sunxi_crtc_helper_funcs = {
	.dpms		= sunxi_drm_crtc_dpms,
	.prepare	= sunxi_drm_crtc_prepare,
	.commit		= sunxi_drm_crtc_commit,
	.mode_fixup	= sunxi_drm_crtc_mode_fixup,
	.mode_set	= sunxi_drm_crtc_mode_set,
	.mode_set_base	= sunxi_drm_crtc_mode_set_base,
	.load_lut	= sunxi_drm_crtc_load_lut,
	.disable	= sunxi_drm_crtc_disable,
};

static int sunxi_manage_filp_data(struct drm_crtc *crtc,
        	struct drm_pending_vblank_event *event)
{
	struct sunxi_flip_user_date flip_data;
	__u32  plane_id = 0;
	int zoder = 0;
	struct sunxi_drm_crtc *sunxi_crtc;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	sunxi_crtc = to_sunxi_crtc(crtc);
	if (event && event->event.user_data) {
		if (copy_from_user(&flip_data, (void __user *)event->event.user_data,
				sizeof(struct sunxi_flip_user_date)) != 0) {
			return -EINVAL;
		}

		plane_id = flip_data.plane_id;
		zoder = flip_data.zpos;
	}else{
		if (sunxi_crtc->fb_plane != NULL)
			return 0;
	}

	return sunxi_set_fb_plane(crtc, plane_id, zoder);
}

static int sunxi_drm_crtc_page_flip(struct drm_crtc *crtc,
				      struct drm_framebuffer *fb,
				      struct drm_pending_vblank_event *event)
{
	struct drm_device *dev = crtc->dev;
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	struct drm_framebuffer *old_fb = crtc->fb;

	int ret = -EINVAL;

	DRM_DEBUG_KMS("(%d) \n", __LINE__);

	/* when the page flip is requested, crtc's dpms should be on */
	if (sunxi_crtc->dpms > DRM_MODE_DPMS_ON) {
		DRM_ERROR("failed page flip request.\n");
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);

	ret = drm_vblank_get(dev, sunxi_crtc->crtc_id);
	if (ret) {
		DRM_DEBUG("failed to acquire vblank counter\n");
		goto out;
	}

	crtc->fb = fb;
	ret = sunxi_manage_filp_data(crtc, event);
	if (ret < 0) {
		DRM_ERROR("failed manage the page flip event.\n");
		goto out;
	}

	ret = sunxi_drm_crtc_mode_set_base(crtc, crtc->x, crtc->y, NULL);
	if (ret) {
		crtc->fb = old_fb;
		drm_vblank_put(dev, sunxi_crtc->crtc_id);
		goto out;
	}
	if (event) {
		/* make sure the list are the updata list in irq handle */
		spin_lock_irq(&dev->event_lock);
		list_add_tail(&event->base.link,
			&sunxi_crtc->pageflip_event_list);
		spin_unlock_irq(&dev->event_lock);
	}
out:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

void sunxi_drm_crtc_finish_pageflip(struct drm_device *dev,
        struct sunxi_drm_crtc *sunxi_crtc)
{
	struct drm_pending_vblank_event *e, *t;
	unsigned long flags;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	spin_lock_irqsave(&dev->event_lock, flags);

	list_for_each_entry_safe(e, t, &sunxi_crtc->pageflip_event_list, base.link) {

		list_del(&e->base.link);
		drm_send_vblank_event(dev, sunxi_crtc->crtc_id, e);
		drm_vblank_put(dev, sunxi_crtc->crtc_id);
	}

	spin_unlock_irqrestore(&dev->event_lock, flags);
}

int sunxi_drm_info_fb_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_crtc *crtc;
	struct sunxi_fb_info_cmd *fb_info_plane;
	struct drm_mode_object *obj;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_drm_plane *sunxi_plane;
	fb_info_plane =  (struct sunxi_fb_info_cmd *)data;

	DRM_DEBUG_KMS("setfb_ioctl: crtc_id[%d] plane[%d] zoder[%d]\n",
	fb_info_plane->crtc_id, fb_info_plane->plane_id, fb_info_plane->zorder);

	obj = drm_mode_object_find(dev, fb_info_plane->crtc_id,
				DRM_MODE_OBJECT_CRTC);
	if (!obj) {
		ret = -EINVAL;
		goto out;
	}
	crtc = obj_to_crtc(obj);
	sunxi_crtc = to_sunxi_crtc(crtc);
	if (fb_info_plane->set_get) {
		return sunxi_set_fb_plane(crtc, fb_info_plane->plane_id, fb_info_plane->zorder);
	}else {
		if (sunxi_crtc->fb_plane) {
			sunxi_plane = to_sunxi_plane(sunxi_crtc->fb_plane);
			fb_info_plane->plane_id = sunxi_crtc->fb_plane->base.id;
			fb_info_plane->zorder = sunxi_plane->plane_cfg->config.info.zorder;
			return 0;
		}   
	}
out:
	DRM_ERROR("[%d] info_fb_ioctl err....\n", __LINE__);
	return -EINVAL;
}

int sunxi_drm_flip_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_crtc *crtc;
	struct sunxi_flip_user_date *flip;
	struct drm_mode_object *obj;

	flip = (struct sunxi_flip_user_date *)data;
	obj = drm_mode_object_find(dev, flip->crtc_id,
				DRM_MODE_OBJECT_CRTC);
	if (!obj) {
		ret = -EINVAL;
		goto out;
	}
	crtc = obj_to_crtc(obj);
	sunxi_drm_crtc_commit(crtc);

out:
	return ret;
}

static void sunxi_drm_crtc_destroy(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);
	struct sunxi_hardware_res *hw_res;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	hw_res = sunxi_crtc->hw_res;
	drm_crtc_cleanup(crtc);

	if (hw_res) {
		sunxi_hwres_destroy(hw_res);
	}
	kfree(sunxi_crtc->plane_array);

	kfree(sunxi_crtc);
}

bool sunxi_drm_crtc_set3d(struct drm_crtc *crtc, bool on_off)
{
	return 1;
}

bool sunxi_drm_crtc_enhance_hf(struct drm_crtc *crtc, bool on_off)
{
	return 1;
}

bool sunxi_drm_crtc_enhance(struct drm_crtc *crtc, bool on_off)
{
	return 1;
}

bool sunxi_drm_crtc_smart(struct drm_crtc *crtc, bool on_off)
{
	return 1;
}

inline int sunxi_mode_check(int new_mode, int old_mode)
{
	int mode_marsk = (1 << CRTC_MODE_LAST)-1;
	new_mode &= mode_marsk;
	old_mode &= mode_marsk;
	return new_mode^old_mode;
}

static int sunxi_drm_crtc_set_property(struct drm_crtc *crtc,
					struct drm_property *property,
					uint64_t val)
{
	struct drm_device *dev = crtc->dev;
	struct sunxi_drm_private *dev_priv = dev->dev_private;
	uint64_t new_mode = val;
	uint64_t old_mode;
	int ret;
	enum sunxi_crtc_mode mode = CRTC_MODE_3D;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	if (property == dev_priv->crtc_mode_property) {
		ret = drm_object_property_get_value(&crtc->base, property, &old_mode);
		if (ret) {
			DRM_ERROR("failed to get crtc disp_mode value.\n");
			return -EINVAL;
		}

		ret = sunxi_mode_check(new_mode, old_mode);
		while (ret && mode < CRTC_MODE_LAST) {
			if(ret & (1 << mode)) {
				bool on_off = new_mode & (1 << mode);
				switch (mode) {
				case CRTC_MODE_3D:
					sunxi_drm_crtc_set3d(crtc, on_off);
					break;
				case CRTC_MODE_ENHANCE:
					sunxi_drm_crtc_enhance(crtc, on_off);
					break;
				case CRTC_MODE_ENHANCE_HF:
					sunxi_drm_crtc_enhance_hf(crtc, on_off);
					break;
				case CRTC_MODE_SMART_LIGHT:
					sunxi_drm_crtc_smart(crtc, on_off);
					break;
				default:
					break;
				}
			}
			mode++;
		}
		return 0;
	}

	return -EINVAL;
}

static int sunxi_crtc_set_config(struct drm_mode_set *set)
{
	int ret = -EINVAL;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct disp_manager_data *save_global_cfgs;

	sunxi_crtc = to_sunxi_crtc(set->crtc);
	DRM_DEBUG_KMS("[%d] fb:%p \n", __LINE__,set->fb);

	save_global_cfgs = sunxi_crtc->global_cfgs;
	sunxi_crtc->global_cfgs = kzalloc(sizeof(struct disp_manager_data), GFP_KERNEL);
	if (sunxi_crtc->global_cfgs == NULL) {
		DRM_ERROR("failed to allocate global_cfgs.\n");
		return -ENOMEM;
	}
	/* change the */
	ret = drm_crtc_helper_set_config(set);

	if (ret) {
		kfree(sunxi_crtc->global_cfgs);
		sunxi_crtc->global_cfgs = save_global_cfgs;
	}else{
		kfree(save_global_cfgs);
	}
	return ret;
}

static struct drm_crtc_funcs sunxi_crtc_funcs = {
	.set_config	= sunxi_crtc_set_config,
	.page_flip	= sunxi_drm_crtc_page_flip,
	.destroy	= sunxi_drm_crtc_destroy,
	.set_property	= sunxi_drm_crtc_set_property,
};

static const struct drm_prop_enum_list mode_names[] = {
	{ CRTC_MODE_3D,     "3d_mode" },// ToDo for many 3D mode, will set it to layer
	{ CRTC_MODE_ENHANCE, "enhance" },
	{ CRTC_MODE_ENHANCE_HF, "enhance_half" },
	{ CRTC_MODE_SMART_LIGHT, "smart_light" },//ToDo
};

static void sunxi_drm_crtc_attach_mode_property(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct  sunxi_drm_private *dev_priv = dev->dev_private;
	struct drm_property *prop;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	prop = dev_priv->crtc_mode_property;
	if (!prop) {
		prop = drm_property_create_bitmask(dev, 0, "disp_mode", mode_names,
						ARRAY_SIZE(mode_names));
		if (!prop)
			return;

		dev_priv->crtc_mode_property = prop;
	}

	drm_object_attach_property(&crtc->base, prop, 0);
}

bool sunxi_de_enable(void *data)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(data);

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	sunxi_clk_enable(sunxi_crtc->hw_res);
	sunxi_drm_crtc_clk_enable(sunxi_crtc->crtc_id);
	sunxi_irq_enable(sunxi_crtc->hw_res);
	return true;
}

bool sunxi_de_disable(void *data)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(data);

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	sunxi_drm_crtc_clk_disable(sunxi_crtc->crtc_id);
	sunxi_irq_free(sunxi_crtc->hw_res);
	sunxi_clk_disable(sunxi_crtc->hw_res);
	return true;
}

void sunxi_de_vsync_proc(void *data)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(data);
	int try_count = 1, current_delayed = 0;
	struct drm_encoder *encoder;

	/* get the Tcon sync line,
	  * and determine update and try lock times.
	  */
	encoder = sunxi_crtc_get_encoder(&sunxi_crtc->drm_crtc);
	current_delayed = sunxi_encoder_updata_delayed(encoder);
	if (current_delayed < 0) {
		DRM_ERROR("crtc_id:%d no more time :%d.\n", sunxi_crtc->crtc_id, current_delayed);
		goto false;
	}

	/* move it to the delayed work? THK a lot. in here */
	if (sunxi_crtc->chain_irq_enable) {
		drm_handle_vblank(sunxi_crtc->drm_crtc.dev, sunxi_crtc->crtc_id);
		sunxi_drm_crtc_finish_pageflip(sunxi_crtc->drm_crtc.dev, sunxi_crtc);
	}

	if(sunxi_crtc->update_frame_ker_cnt == sunxi_crtc->update_frame_user_cnt)
		return ;

	try_count = current_delayed * 20;
	if (try_count < 0) {
		DRM_ERROR("crtc_id:%d  no more time :%d.\n", sunxi_crtc->crtc_id, current_delayed);
		goto false; 
	}
	while (try_count-- && !spin_trylock(&sunxi_crtc->update_reg_lock)) {
		if (!try_count) {
			DRM_DEBUG_KMS("crtc_id:%d   lose the lock.\n", sunxi_crtc->crtc_id);
			goto false;
		}
	}
	/* in_update_frame  will use if DE can set  the layer(hardware register) for 1 or  */
	if (!sunxi_crtc->in_update_frame &&
			sunxi_crtc->update_frame_ker_cnt != sunxi_crtc->update_frame_user_cnt) {
		sunxi_drm_sync_reg(sunxi_crtc->crtc_id);
		sunxi_drm_updata_reg(sunxi_crtc->crtc_id);

		/*careful for unsinged long roll */
		sunxi_crtc->user_skip_cnt += 
		sunxi_crtc->update_frame_user_cnt - (sunxi_crtc->update_frame_ker_cnt + 1);
		sunxi_crtc->update_frame_ker_cnt = sunxi_crtc->update_frame_user_cnt;
	}

	spin_unlock(&sunxi_crtc->update_reg_lock);

	schedule_work(&sunxi_crtc->delayed_work);
	return ;

false:
	if(sunxi_crtc->update_frame_ker_cnt != sunxi_crtc->update_frame_user_cnt) {
		sunxi_crtc->ker_skip_cnt++;
	}
	return ;
}

bool sunxi_de_updata_reg(void *data)
{
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(data);
	struct disp_manager_data *global_cfgs = sunxi_crtc->global_cfgs;
	struct sunxi_drm_connector *sunxi_connector;
	struct drm_connector *connector;

	int i;
	struct sunxi_drm_plane *sunxi_plane;

	DRM_DEBUG_KMS("[%d] crtc_id:%d\n", __LINE__, sunxi_crtc->crtc_id);

	connector = sunxi_crtc_get_conector(&sunxi_crtc->drm_crtc);
	if(!connector)
		return false;
	sunxi_connector = to_sunxi_connector(connector);

	spin_lock(&sunxi_crtc->update_reg_lock);
	sunxi_crtc->update_frame_user_cnt++;

	if (global_cfgs->flag) {
		DRM_DEBUG_KMS("[%d]crtc_id:%d cfg:0x%x mux:%d, size(%d,%d)\n", __LINE__,
		sunxi_crtc->crtc_id, global_cfgs->flag, sunxi_crtc->global_cfgs->config.hwdev_index,
		sunxi_crtc->global_cfgs->config.size.width, sunxi_crtc->global_cfgs->config.size.height);

		sunxi_drm_apply_cache(sunxi_crtc->crtc_id, sunxi_crtc->global_cfgs);
		global_cfgs->flag = 0;
	}

	if (sunxi_crtc->has_delayed_updata) {
		for (i = 0; i < sunxi_crtc->plane_of_de; i++) {
			sunxi_plane = to_sunxi_plane(sunxi_crtc->plane_array[i]);
			if (sunxi_plane->delayed_updata) {
				sunxi_plane->hw_res->ops->updata_reg(sunxi_plane);
				sunxi_plane->delayed_updata = 0;
			}
		}
		sunxi_crtc->has_delayed_updata = 0;
	}

	sunxi_drm_updata_crtc(sunxi_crtc, sunxi_connector);

	spin_unlock(&sunxi_crtc->update_reg_lock);

	return true;
}

bool sunxi_de_query_irq(void *irq_data, int need_irq)
{
	//struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(irq_data);
	return false;
}
/*
void sunxi_de_vsync_delayed_work(void *data)
{

}
*/
struct sunxi_hardware_ops de_hw_ops = {
	.reset          = NULL,
	.enable         = sunxi_de_enable,
	.disable        = sunxi_de_disable,
	.updata_reg     = sunxi_de_updata_reg,
	.vsync_proc   = sunxi_de_vsync_proc,
	.irq_query    = NULL,
	.vsync_delayed_do = NULL,
};

irqreturn_t sunxi_crtc_vsync_handle(int irq, void *data)
{
	int i;
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_plane *sunxi_plane;
	struct drm_encoder *encoder;
	struct drm_connector *drm_connector;
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(data);
	void (*vsync_proc)(void *irq_data);

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	if (!drm_helper_crtc_in_use(data))
		goto handle_end;

	if (!sunxi_irq_query(sunxi_crtc->hw_res, data, QUERY_VSYNC_IRQ))
		return IRQ_HANDLED;

	vsync_proc = sunxi_crtc->hw_res->ops->vsync_proc;
	if (vsync_proc != NULL)
		vsync_proc(sunxi_crtc);

	for (i = 0; i < sunxi_crtc->plane_of_de; i++) {
		sunxi_plane = to_sunxi_plane(sunxi_crtc->plane_array[i]);
		vsync_proc = sunxi_plane->hw_res->ops->vsync_proc;
		if(vsync_proc != NULL)
			vsync_proc(sunxi_plane);
	}
	/* we hardware only support 1 chain */
	encoder = sunxi_get_encoder(&sunxi_crtc->drm_crtc);
	if (!encoder)
		goto handle_end;
	sunxi_encoder = to_sunxi_encoder(encoder);
	if (!sunxi_encoder)
		goto handle_end;
	vsync_proc = sunxi_encoder->hw_res->ops->vsync_proc;
	if (vsync_proc != NULL)
		vsync_proc(sunxi_encoder);

	drm_connector = sunxi_get_conct(encoder);
	if (!drm_connector)
		goto handle_end;
	sunxi_connector = to_sunxi_connector(drm_connector);
	vsync_proc = sunxi_connector->hw_res->ops->vsync_proc;
	if(vsync_proc != NULL)
		vsync_proc(sunxi_connector);

handle_end:
	return IRQ_HANDLED;
}

int sunxi_drm_enable_vblank(struct drm_device *dev, int crtc)
{
	struct sunxi_drm_crtc *sunxi_crtc = NULL;
	/* crtc start from 0 */
	sunxi_crtc = get_sunxi_crt(dev, crtc);
	sunxi_crtc->chain_irq_enable = 1;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	return 0;
}

void sunxi_drm_disable_vblank(struct drm_device *dev, int crtc)
{
	struct sunxi_drm_crtc *sunxi_crtc = NULL;
	sunxi_crtc = get_sunxi_crt(dev, crtc);
	sunxi_crtc->chain_irq_enable = 0;

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	return ;
}

void sunxi_crtc_delay_work(struct work_struct *work)
{
	int i;
	struct sunxi_drm_plane  *sunxi_plane;
	struct sunxi_drm_crtc *sunxi_crtc =
	container_of(work, struct sunxi_drm_crtc, delayed_work);
	void (*vsync_delayed_do)(void *data);

	DRM_DEBUG_KMS("[%d] \n", __LINE__);

	vsync_delayed_do = sunxi_crtc->hw_res->ops->vsync_delayed_do;
	if (vsync_delayed_do != NULL) {
		vsync_delayed_do(sunxi_crtc);
	}
	for (i = 0; i < sunxi_crtc->plane_of_de; i++) {
		sunxi_plane = to_sunxi_plane(sunxi_crtc->plane_array[i]);
		vsync_delayed_do = sunxi_plane->hw_res->ops->vsync_delayed_do;
		if(vsync_delayed_do != NULL)
			vsync_delayed_do(sunxi_plane);
	}
}

int sunxi_drm_crtc_create(struct drm_device *dev,
	unsigned int nr, struct sunxi_hardware_res *hw_res)
{
	struct sunxi_drm_crtc *sunxi_crtc = NULL; 
	struct drm_crtc *crtc = NULL;
	struct drm_property *prop = NULL;
	struct drm_plane *drm_plane = NULL;
	struct disp_layer_config_data *plane_cfg;
	int i = 0, j = 0, z = 0, v = 0;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	if (!hw_res) {
		DRM_ERROR("sunxi crtc must have hardware resource.\n");
		return -EINVAL;
	}

	sunxi_crtc = kzalloc(sizeof(*sunxi_crtc), GFP_KERNEL);
	if (!sunxi_crtc) {
		DRM_ERROR("failed to allocate sunxi crtc\n");
		goto crtc_err;
	}

	sunxi_crtc->crtc_id = nr;
	crtc = &sunxi_crtc->drm_crtc;

	/*to the DE BSP get nr of lyr and chnl*/
	sunxi_crtc->chn_count = sunxi_drm_get_num_chns_by_crtc(nr);
	sunxi_crtc->plane_of_de = sunxi_drm_get_plane_by_crtc(nr);
	if (sunxi_crtc->plane_of_de < 0) {
		DRM_ERROR("failed to get the lyr count.\n");
		goto crtc_err;
	}

	prop = drm_property_create_range(dev, 0, "zpos", 0,
				sunxi_crtc->plane_of_de - 1);
	if (!prop)
		goto prop_err;
	sunxi_crtc->plane_zpos_property = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_IMMUTABLE, "channel_id", 0,
						sunxi_crtc->chn_count - 1);
	if (!prop)
		goto prop_err;

	sunxi_crtc->channel_id_property = prop;
	/* Currently chosse 4, if de change we will change it */
	z = sunxi_drm_get_crtc_pipe_plane(nr, 0);
	prop = drm_property_create_range(dev, DRM_MODE_PROP_IMMUTABLE, "plane_id_chn", 0,
					z - 1);
	if (!prop)
		goto prop_err;
	sunxi_crtc->plane_id_chn_property = prop;

	sunxi_crtc->plane_array = 
	kzalloc(sizeof(struct drm_plane*) * sunxi_crtc->plane_of_de, GFP_KERNEL);
	if (!sunxi_crtc->plane_array) {
		DRM_ERROR("failed to allocate sunxi plane array.\n");
		goto array_err;
	}

	sunxi_crtc->plane_cfgs = 
	kzalloc(sizeof(struct disp_layer_config_data) * sunxi_crtc->plane_of_de, GFP_KERNEL);
	if (!sunxi_crtc->plane_cfgs) {
		DRM_ERROR("failed to allocate sunxi plane array.\n");
		goto plane_cfgs_err;
	}

	for (j = 0, v = 0; j < sunxi_crtc->chn_count; j++) {
		z = sunxi_drm_get_crtc_pipe_plane(nr, j);
		for (i = 0; i < z; i++, v++) {
			plane_cfg = sunxi_crtc->plane_cfgs + v;
			drm_plane = sunxi_plane_init(dev, &sunxi_crtc->drm_crtc, plane_cfg, j, i, false);
			if (!drm_plane) {
				DRM_ERROR("failed to allocate sunxi plane.\n");
				goto plane_err;
			}
			sunxi_crtc->plane_array[v] = drm_plane;
		}
	}

	sunxi_crtc->global_cfgs = kzalloc(sizeof(struct disp_manager_data), GFP_KERNEL);
	if (!sunxi_crtc->global_cfgs) {
		DRM_ERROR("failed to allocate sunxi crtc global_cfgs.\n");
		goto plane_err;
	}
	/* init the de hardware, clock all on, contorl the sub switch */
	sunxi_crtc->hw_res = hw_res;
	if (hw_res->ops == NULL) {
		hw_res->ops = &de_hw_ops;
		hw_res->irq_arg = (void*)sunxi_crtc;
		hw_res->irq_handle = sunxi_crtc_vsync_handle;
		hw_res->irq_uesd = false;
	}

	sunxi_updata_crtc_freq(clk_get_rate(hw_res->clk));

	INIT_LIST_HEAD(&sunxi_crtc->pageflip_event_list);
	INIT_WORK(&sunxi_crtc->delayed_work, sunxi_crtc_delay_work);

	spin_lock_init(&sunxi_crtc->update_reg_lock);
	drm_crtc_init(dev, crtc, &sunxi_crtc_funcs);
	drm_crtc_helper_add(crtc, &sunxi_crtc_helper_funcs);

	sunxi_drm_crtc_attach_mode_property(crtc);
	crtc_array[nr] = &sunxi_crtc->drm_crtc;

	DRM_INFO("[%d] crtc_id:%d(%d)\n", __LINE__, sunxi_crtc->crtc_id, crtc->base.id);

	return 0;

plane_err:
	for (i = 0; i < sunxi_crtc->plane_of_de; i++) {
		if(sunxi_crtc->plane_array[i] != NULL)
			kfree(sunxi_crtc->plane_array[i]);
		sunxi_crtc->plane_array[i] = NULL;
	}
	kfree(sunxi_crtc->plane_cfgs);
	sunxi_crtc->plane_cfgs = NULL;

plane_cfgs_err:
	kfree(sunxi_crtc->plane_array);
	sunxi_crtc->plane_array = NULL;

array_err:
prop_err:
	if(sunxi_crtc->channel_id_property)
		drm_property_destroy(sunxi_crtc->drm_crtc.dev,
			sunxi_crtc->channel_id_property);

	if(sunxi_crtc->plane_id_chn_property)
		drm_property_destroy(sunxi_crtc->drm_crtc.dev,
			sunxi_crtc->plane_id_chn_property);

	if(sunxi_crtc->plane_zpos_property)
		drm_property_destroy(sunxi_crtc->drm_crtc.dev,
			sunxi_crtc->plane_zpos_property);    

	kfree(sunxi_crtc);

crtc_err:

	return -EINVAL;
}

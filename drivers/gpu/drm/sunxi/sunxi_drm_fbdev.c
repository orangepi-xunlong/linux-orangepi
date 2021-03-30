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
#include <drm/drm_crtc.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include "sunxi_drm_gem.h"
#include "sunxi_drm_fb.h"
#include "sunxi_drm_dmabuf.h"
#include "sunxi_drm_fbdev.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_connector.h"

struct fb_dmabuf_export
{
	__u32 fd;
	__u32 flags;
};

struct __fb_addr_para
{
	uintptr_t fb_paddr;
	int fb_size;
};

struct __fb_addr_para g_fb_addr;

#define FBIOGET_DMABUF       _IOR('F', 0x21, struct fb_dmabuf_export)

int sunxi_drm_fb_ioctl(struct fb_info *info, unsigned int cmd,
			unsigned long arg)
{
	struct drm_fb_helper *helper;
	struct drm_framebuffer *fb;
	struct sunxi_drm_framebuffer *sunxi_fb;
	void __user *argp = (void __user *)arg;
	int crtc = 0;
	DRM_DEBUG_KMS("### %s:%s() ###\n", __FILE__,__func__);

	helper = info->par;
	fb = helper->fb;
	sunxi_fb = to_sunxi_fb(fb);

	switch(cmd) {
	case FBIO_WAITFORVSYNC:
	if (get_user(crtc, (int *)(argp))) {
		return -EFAULT;
	}
	/* TODO JetCui */
	break;
	case FBIOGET_DMABUF:
	{     
	struct fb_dmabuf_export dma_arg;
	struct dma_buf *buf;
	int fd;
	if (copy_from_user(&dma_arg, argp, sizeof(dma_arg))) {
		return -EFAULT;
	}

	buf = sunxi_dmabuf_prime_export(NULL,
	sunxi_fb->gem_obj[0], dma_arg.flags);

	fd = dma_buf_fd(buf, dma_arg.flags);
	if (fd < 0) {
		DRM_ERROR("can not get dma_buf fd.\n");
		dma_buf_put(buf);
		return -ENODEV;
	}

	dma_arg.fd = (__u32)fd;
	if (copy_to_user(argp, &dma_arg, sizeof(dma_arg))) {
		DRM_ERROR("can not copy dma_buf fd.\n");
		dma_buf_put(buf);
		return -ENODEV;
	}
	}
		break;
	default:
	DRM_ERROR("bad cmd to drm fb.\n");
	}
	return 0;
}

int sunxi_drm_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct drm_fb_helper *helper = (struct drm_fb_helper *)info->par;
	struct sunxi_drm_fbdev *sunxi_fbdev = to_sunxi_fbdev(helper);
	struct sunxi_drm_framebuffer *sunxi_fb = to_sunxi_fb(sunxi_fbdev->drm_fb_helper.fb);
	struct drm_gem_object *sunxi_gem_obj = sunxi_fb->gem_obj[0];
	struct sunxi_drm_gem_buf *buffer = (struct sunxi_drm_gem_buf *)sunxi_gem_obj->driver_private;
	unsigned long vm_size;
	int ret;

	DRM_DEBUG_KMS("### %s:%s() ###\n", __FILE__,__func__);

	vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

	vm_size = vma->vm_end - vma->vm_start;

	if (vm_size > buffer->size) {
		DRM_ERROR("failed to mmap %ld > %ld.\n",vm_size, buffer->size);
		return -EINVAL;
	}
	ret = dma_mmap_attrs(helper->dev->dev, vma, buffer->kvaddr,
	buffer->dma_addr, buffer->size, &buffer->dma_attrs);
	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	return 0;
}
/*
int sunxi_fb_fpan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
    struct drm_fb_helper *fb_helper = info->par;
	struct drm_device *dev = fb_helper->dev;
    struct drm_framebuffer *fb;

    DRM_DEBUG_KMS("%s:(%s):%d ###\n", __FILE__,__func__,__LINE__);

    fb = fb_helper->fb;
	drm_modeset_lock_all(dev);
    fb->offsets[0] = var->yoffset * fb->pitches[0];
    //var->yoffset = 0;
	drm_modeset_unlock_all(dev);
    
	return drm_fb_helper_pan_display(var, info);
    
}
*/
void sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para)
{
	if (fb_addr_para){
		fb_addr_para->fb_paddr = g_fb_addr.fb_paddr;
		fb_addr_para->fb_size  = g_fb_addr.fb_size;
	}
}
EXPORT_SYMBOL(g_fb_addr);
EXPORT_SYMBOL(sunxi_get_fb_addr_para);

static struct fb_ops sunxi_drm_fb_ops = {
	.owner = THIS_MODULE,
	.fb_mmap = sunxi_drm_fb_mmap,
#if defined(CONFIG_FB_CONSOLE_SUNXI)
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
#endif
	.fb_check_var = drm_fb_helper_check_var,
	.fb_set_par = drm_fb_helper_set_par,
	.fb_blank = drm_fb_helper_blank,
	.fb_pan_display	= drm_fb_helper_pan_display,
	.fb_setcmap = drm_fb_helper_setcmap,
	.fb_ioctl = sunxi_drm_fb_ioctl,
#ifdef CONFIG_COMPAT
	.fb_compat_ioctl = NULL,
#endif
};

static int sunxi_fb_fill_fbdev(struct drm_fb_helper *helper,
				     struct drm_framebuffer *fb)
{
	struct fb_info *fbi = helper->fbdev;
	struct drm_device *dev = helper->dev;
	struct sunxi_drm_gem_buf *buffer;
	unsigned long offset;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	drm_fb_helper_fill_fix(fbi, fb->pitches[0], fb->depth);
	drm_fb_helper_fill_var(fbi, helper, fb->width, fb->height / FBDEV_BUF_NUM);//TODO for picth
	buffer = sunxi_drm_framebuffer_buffer(fb, 0);
	if (!buffer) {
		DRM_LOG_KMS("buffer is null.\n");
		return -EFAULT;
	}

	offset = fbi->var.xoffset * (fb->bits_per_pixel >> 3);
	offset += fbi->var.yoffset * fb->pitches[0];

	dev->mode_config.fb_base = (resource_size_t)buffer->dma_addr;
	fbi->screen_base = buffer->kvaddr + offset;
	fbi->fix.smem_start = (unsigned long)buffer->dma_addr;

	fbi->screen_size = buffer->size;
	fbi->fix.smem_len = buffer->size;
	printk("addr:0x%lx  size:%ld\n",(unsigned long)buffer->dma_addr,buffer->size);
	g_fb_addr.fb_paddr = (unsigned long)buffer->dma_addr;
	g_fb_addr.fb_size = buffer->size;

	return 0;
}

void sunxi_fb_gamma_set(struct drm_crtc *crtc, u16 red, u16 green,
		  u16 blue, int regno)
{
}

void sunxi_fb_gamma_get(struct drm_crtc *crtc, u16 *red, u16 *green,
		  u16 *blue, int regno)
{

}

int sunxi_fb_probe(struct drm_fb_helper *helper,
		struct drm_fb_helper_surface_size *sizes)
{
	struct drm_gem_object *gem_obj[MAX_FB_BUFFER];
	struct drm_device *dev = helper->dev;
	struct fb_info *fbi;
	struct drm_mode_fb_cmd2 mode_cmd = { 0 };
	struct platform_device *pdev = dev->platformdev;
	struct drm_mode_create_dumb dump_args = { 0 };
	int ret;


	sizes->surface_depth = 32;
	mode_cmd.width = sizes->fb_width;
	mode_cmd.height = sizes->fb_height * FBDEV_BUF_NUM;//double buffers? or more?
	mode_cmd.pitches[0] = sizes->fb_width * (sizes->surface_bpp >> 3);//with GPU
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
							  sizes->surface_depth);

	dump_args.height = mode_cmd.height;
	dump_args.width = mode_cmd.width;
	dump_args.bpp = sizes->surface_bpp;
	dump_args.flags = SUNXI_BO_CONTIG |SUNXI_BO_CACHABLE;
	DRM_INFO("surface(%d x %d), fb(%d x %d) and bpp(%d) form(%d)\n",
			sizes->surface_width, sizes->surface_height,
			sizes->fb_width, sizes->fb_height,
			sizes->surface_bpp, mode_cmd.pixel_format);
	mutex_lock(&dev->struct_mutex);

	fbi = framebuffer_alloc(0, &pdev->dev);
	if (!fbi) {
		DRM_ERROR("failed to allocate fb info.\n");
		ret = -ENOMEM;
		goto out;
	}

	/*if sync with bootloader, creat the gem_obj from paddr */
	gem_obj[0] = sunxi_drm_gem_creat(dev, &dump_args);
	if (NULL == gem_obj[0]) {
	DRM_ERROR("failed to creat sunxi gem for fbdev.\n");
		goto err_release_framebuffer;
	}

	helper->fb = sunxi_drm_framebuffer_creat(dev, &mode_cmd,
			gem_obj, 1, SUNXI_FBDEV_FLAGS);
	if (IS_ERR(helper->fb)) {
		DRM_ERROR("failed to create drm framebuffer.\n");
		ret = PTR_ERR(helper->fb);
		goto err_destroy_gem;
	}

	helper->fbdev = fbi;

	fbi->par = helper;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->fbops = &sunxi_drm_fb_ops;

	ret = fb_alloc_cmap(&fbi->cmap, 256, 0);
	if (ret) {
		DRM_ERROR("failed to allocate cmap.\n");
		goto err_destroy_framebuffer;
	}

	ret = sunxi_fb_fill_fbdev(helper, helper->fb);
	if (ret < 0)
		goto err_dealloc_cmap;

	mutex_unlock(&dev->struct_mutex);
	return ret;

err_dealloc_cmap:
	fb_dealloc_cmap(&fbi->cmap);
err_destroy_framebuffer:
	drm_framebuffer_cleanup(helper->fb);
err_destroy_gem:
	drm_gem_object_unreference(gem_obj[0]);
err_release_framebuffer:
	framebuffer_release(fbi);

/*
* if failed, all resources allocated above would be released by
* drm_mode_config_cleanup() when drm_load() had been called prior
* to any specific driver such as fimd or hdmi driver.
*/
out:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

static struct drm_display_mode *sunx_pick_cmdline_mode(struct drm_fb_helper_connector *fb_helper_conn)
{
	struct drm_cmdline_mode *cmdline_mode;
	struct drm_display_mode *mode = NULL;

	cmdline_mode = &fb_helper_conn->cmdline_mode;
	if (cmdline_mode->specified == false)
		return mode;

	list_for_each_entry(mode, &fb_helper_conn->connector->modes, head) {
		/* check width/height */
		if (mode->hdisplay != cmdline_mode->xres ||
			mode->vdisplay != cmdline_mode->yres)
			continue;

		if (cmdline_mode->refresh_specified) {
			if (mode->vrefresh != cmdline_mode->refresh)
			continue;
		}

		if (cmdline_mode->interlace) {
			if (!(mode->flags & DRM_MODE_FLAG_INTERLACE))
				continue;
			}
		return mode;
	}
	return mode;
}

static struct drm_display_mode *sunxi_find_similar_modes(struct drm_display_mode *ref_mode,
				struct drm_connector *connector)
{
#ifdef CONFIG_ARM
	unsigned int ref_factor, tmp_factor, little_factor = 1000000, cmp_factor;
#endif
#ifdef CONFIG_ARM64
	unsigned long long ref_factor, tmp_factor, little_factor = 100000000, cmp_factor;
#endif
	struct drm_display_mode *mode = NULL;
	struct drm_display_mode *fix_mode = NULL;
#ifdef CONFIG_ARM
	ref_factor = ((ref_mode->hdisplay) << 10) / ref_mode->vdisplay;
#endif
#ifdef CONFIG_ARM64
	ref_factor = (((unsigned long long)ref_mode->hdisplay) << 10) / ref_mode->vdisplay;
#endif

	fix_mode = list_entry(connector->modes.next, struct drm_display_mode, head);

	list_for_each_entry(mode, &connector->modes, head) {
		/* check width/height */
		if (mode->hdisplay < ref_mode->hdisplay || mode->vdisplay < ref_mode->vdisplay)
			continue;
#ifdef CONFIG_ARM
		tmp_factor = ((mode->hdisplay) << 10) / mode->vdisplay;
#endif
#ifdef CONFIG_ARM64
		tmp_factor = (((unsigned long long)mode->hdisplay) << 10) / mode->vdisplay;
#endif
		if (tmp_factor > ref_factor) {
			cmp_factor = tmp_factor - ref_factor;
		}else {
			cmp_factor = ref_factor - tmp_factor;
		}
		if (little_factor > cmp_factor) {
			little_factor = cmp_factor;
			fix_mode = mode;
		}
	}
	DRM_DEBUG_KMS("ref width(%d) height(%d) and fix_w(%d) h(%d)\n",
	ref_mode->hdisplay, ref_mode->vdisplay, fix_mode->hdisplay, fix_mode->vdisplay);

	return fix_mode;
}

bool sunxi_fb_initial_config(struct drm_fb_helper *fb_helper,
		struct drm_fb_helper_crtc **crtcs,
		struct drm_display_mode **modes,
		bool *enabled, int width, int height)
{
	/*set the mode or the connector and becareful for using 1 fb*/
	int i, ref = -1, c = 0;
	struct drm_connector *connector;
	struct sunxi_drm_connector *sunxi_connector;
	struct drm_display_mode *ref_modes = NULL;
	struct drm_encoder	    *encoder;
	struct drm_connector_helper_funcs *connector_funcs;
	unsigned int assign_crtc = 0;
	struct drm_fb_helper_crtc *crtc;

retry:
	assign_crtc = 0;
	/* pick the modes for connector */
	for (i = 0; i < fb_helper->connector_count; i++) {
		connector = fb_helper->connector_info[i]->connector;
		sunxi_connector = to_sunxi_connector(connector);
		if (!enabled[i]) {
		modes[i] = NULL;
		continue;
		}

		if (ref_modes == NULL && sunxi_connector->disp_out_type == DISP_OUTPUT_TYPE_LCD) {
			ref_modes = list_entry(connector->modes.next, struct drm_display_mode, head);
			modes[i] = ref_modes;
			goto retry;
		}

		modes[i] = sunx_pick_cmdline_mode(fb_helper->connector_info[i]);
		if (modes[i]) {
			ref = i;
			if (!ref_modes) {
				goto retry;
			}
		}

		if (ref == -1) {
			modes[i] = list_entry(connector->modes.next, struct drm_display_mode, head);
			ref = i;
		}

		if (ref_modes) {
			modes[i] = sunxi_find_similar_modes(ref_modes, connector);
		} else {
			modes[i] = sunxi_find_similar_modes(modes[ref], connector);
		}
		if (!modes[i])
			continue;

		connector_funcs = connector->helper_private;
		encoder = connector_funcs->best_encoder(connector);
		if (!encoder)
			continue;

		for (c = 0; c < fb_helper->crtc_count; c++) {
			crtc = &fb_helper->crtc_info[c];

			if ((encoder->possible_crtcs & (1 << c)) == 0)
				continue;
			if ((assign_crtc & (1 << c)) == 0) {
				assign_crtc |= (1 << c);
				crtcs[i] = crtc;
				DRM_INFO("connetor(%d)--->crtc(%d) encoder:%d,resolution(%d x %d)\n",
				i,c,encoder->base.id, modes[i]->hdisplay, modes[i]->vdisplay);
				break;
			}
		}
	}

	return true;
}

struct drm_fb_helper_funcs sunxi_drm_fb_helper = {
	.gamma_set = NULL,
	.gamma_get = NULL,
	.fb_probe = sunxi_fb_probe,
	.initial_config = sunxi_fb_initial_config,
};

int sunxi_drm_fbdev_creat(struct drm_device *dev)
{
	struct sunxi_drm_fbdev *fbdev;
	struct sunxi_drm_private *private = dev->dev_private;
	struct drm_fb_helper *helper;
	unsigned int num_crtc;
	int ret;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	if (!dev->mode_config.num_crtc || !dev->mode_config.num_connector)
		return 0;

	fbdev = kzalloc(sizeof(*fbdev), GFP_KERNEL);
	if (!fbdev) {
		DRM_ERROR("failed to allocate drm fbdev.\n");
		return -ENOMEM;
	}

	private->fb_helper = helper = &fbdev->drm_fb_helper;
	helper->funcs = &sunxi_drm_fb_helper;

	num_crtc = dev->mode_config.num_crtc;

	ret = drm_fb_helper_init(dev, helper, num_crtc, dev->mode_config.num_connector);
	if (ret < 0) {
		DRM_ERROR("failed to initialize drm fb helper.\n");
		goto err_init;
	}

	ret = drm_fb_helper_single_add_all_connectors(helper);
	if (ret < 0) {
		DRM_ERROR("failed to register drm_fb_helper_connector.\n");
		goto err_setup;
	}

	/* TODO disable all the possible outputs/crtcs before entering KMS mode */
	drm_helper_disable_unused_functions(dev);

	ret = drm_fb_helper_initial_config(helper, PREFERRED_BPP);
	if (ret < 0) {
		DRM_ERROR("failed to set up hw configuration.\n");
		goto err_setup;
	}

	return 0;

err_setup:
	drm_fb_helper_fini(helper);

err_init:
	private->fb_helper = NULL;
	kfree(fbdev);

	return ret;
}

void sunxi_drm_fbdev_destroy(struct drm_device *dev)
{
	struct sunxi_drm_private *private = dev->dev_private;
	struct sunxi_drm_fbdev *fbdev;
	struct drm_framebuffer *fb;
	struct sunxi_drm_framebuffer *sunxi_fb;
	struct drm_fb_helper	*fb_helper;

	if (!private || !private->fb_helper)
		return;
	fb_helper = private->fb_helper;
	fbdev = to_sunxi_fbdev(private->fb_helper);
	fb = fbdev->drm_fb_helper.fb;
	if (!fb) {
		return;
	}

	sunxi_fb = (struct sunxi_drm_framebuffer *)fb;
	/* release drm framebuffer and gem_obj */
	if (fb_helper->fb && fb_helper->fb->funcs) {
		fb = fb_helper->fb;
		if (fb) {
			drm_framebuffer_unregister_private(fb);
			drm_framebuffer_remove(fb);
		}
	}

	/* release linux framebuffer */
	if (fb_helper->fbdev) {
		struct fb_info *info;
		int ret;

		info = fb_helper->fbdev;
		ret = unregister_framebuffer(info);
		if (ret < 0)
			DRM_DEBUG_KMS("failed unregister_framebuffer()\n");

		if (info->cmap.len)
			fb_dealloc_cmap(&info->cmap);

		framebuffer_release(info);
	}

	drm_fb_helper_fini(fb_helper);
	kfree(fbdev);
	private->fb_helper = NULL;
}

void sunxi_drm_fbdev_restore_mode(struct drm_device *dev)
{
	struct sunxi_drm_private *private = dev->dev_private;

	if (!private || !private->fb_helper)
		return;

	drm_modeset_lock_all(dev);
	drm_fb_helper_restore_fbdev_mode(private->fb_helper);
	drm_modeset_unlock_all(dev);
}


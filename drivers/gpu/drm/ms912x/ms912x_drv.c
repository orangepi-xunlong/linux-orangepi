// SPDX-License-Identifier: GPL-2.0-only
#include <linux/module.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_file.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_gem_vram_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <linux/dma-buf.h>

#include "ms912x.h"
#include "usb_hal_interface.h"

static int g_hal_index = 0;

DEFINE_DRM_GEM_FOPS(ms912x_driver_fops);

static struct drm_driver driver = {
	.driver_features = DRIVER_ATOMIC | DRIVER_GEM | DRIVER_MODESET,
	.lastclose = drm_fb_helper_lastclose,
	/* GEM hooks */
	.fops = &ms912x_driver_fops,
	.gem_create_object = drm_gem_shmem_create_object_cached,
	DRM_GEM_SHMEM_DRIVER_OPS,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static const struct drm_mode_config_funcs ms912x_mode_config_funcs = {
	.fb_create = drm_gem_fb_create_with_dirty,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static void ms912x_pipe_enable(struct drm_simple_display_pipe *pipe,
			       struct drm_crtc_state *crtc_state,
			       struct drm_plane_state *plane_state)
{
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
	struct drm_display_mode *drm_mode = &crtc_state->mode;
	struct drm_framebuffer *fb = plane_state->fb;
	struct usb_hal_video_mode usb_hal_mode;
	u32 fourcc = fb->format->format;
	int ret;

	DRM_DEBUG("ms912x_pipe_enable\n");

	if (crtc_state->mode_changed) {
		usb_hal_disable(ms912x->hal);
		usb_hal_mode.width = drm_mode->hdisplay;
		usb_hal_mode.height = drm_mode->vdisplay;
		usb_hal_mode.rate = drm_mode_vrefresh(drm_mode);
		ret = usb_hal_get_vic(ms912x->hal, usb_hal_mode.width,
				      usb_hal_mode.height, usb_hal_mode.rate,
				      &usb_hal_mode.vic);
		if (ret)
		{
			DRM_ERROR("get vic failed! width:%d height:%d\n\n",
				  usb_hal_mode.width, usb_hal_mode.height);
			return;
		}
		usb_hal_enable(ms912x->hal, &usb_hal_mode, fourcc);
		ms912x->is_enable = true;
	}
}

static void ms912x_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
	DRM_DEBUG("ms912x_pipe_disable\n");
	usb_hal_disable(ms912x->hal);
	ms912x->is_enable = false;
}

static enum drm_mode_status
ms912x_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
		       const struct drm_display_mode *mode)
{
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
	struct usb_hal_video_mode usb_hal_mode;
	int ret;
	usb_hal_mode.width = mode->hdisplay;
	usb_hal_mode.height = mode->vdisplay;
	usb_hal_mode.rate = drm_mode_vrefresh(mode);
	DRM_DEBUG("ms912x_pipe_mode_valid\n");

	ret = usb_hal_get_vic(ms912x->hal, usb_hal_mode.width,
			      usb_hal_mode.height, usb_hal_mode.rate,
			      &usb_hal_mode.vic);
	if (ret)
	{
		DRM_DEBUG("get vic failed! width:%d height:%d\n\n",
			  usb_hal_mode.width, usb_hal_mode.height);
		return MODE_BAD;
	}

	DRM_DEBUG("get vic sucess! width:%d height:%d\n\n",
		  usb_hal_mode.width, usb_hal_mode.height);
	return MODE_OK;
}

static int ms912x_pipe_check(struct drm_simple_display_pipe *pipe,
		      struct drm_plane_state *new_plane_state,
		      struct drm_crtc_state *new_crtc_state)
{
	return 0;
}

static int ms912x_update_frame(struct ms912x_device *ms912x, struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_gem_cma_object *cma_obj = drm_fb_cma_get_gem_obj(fb, 0);
	struct dma_buf_attachment *import_attach = cma_obj->base.import_attach;
	int ret = 0;
	void *vaddr;
	int len = fb->pitches[0] * (state->src_h >> 16);

	if (import_attach) {
		ret = dma_buf_begin_cpu_access(import_attach->dmabuf,
					       DMA_FROM_DEVICE);
		if (ret)
			return ret;
	}

	vaddr = drm_gem_shmem_vmap(fb->obj[0]);
	if (IS_ERR(vaddr)) {
		DRM_ERROR("failed to vmap fb\n");
		goto out_dma_buf_end_cpu_access;
	}

	usb_hal_update_frame(ms912x->hal, vaddr, fb->pitches[0], len, DRM_FORMAT_ARGB8888, 0);

	drm_gem_shmem_vunmap(fb->obj[0], vaddr);

out_dma_buf_end_cpu_access:
	if (import_attach) {
		dma_buf_end_cpu_access(import_attach->dmabuf,
				       DMA_FROM_DEVICE);
	}

	return ret;
}

static void ms912x_pipe_update(struct drm_simple_display_pipe *pipe,
			       struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = pipe->plane.state;
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
	DRM_DEBUG("ms912x_pipe_update\n");

	if (!state->fb)
		return;

	if(ms912x->is_enable)
		ms912x_update_frame(ms912x, state);
}

static const struct drm_simple_display_pipe_funcs ms912x_pipe_funcs = {
	.enable = ms912x_pipe_enable,
	.disable = ms912x_pipe_disable,
	.check = ms912x_pipe_check,
	.mode_valid = ms912x_pipe_mode_valid,
	.update = ms912x_pipe_update,
	.prepare_fb = drm_gem_fb_simple_display_pipe_prepare_fb,
};

static const uint32_t ms912x_pipe_formats[] = {
	DRM_FORMAT_XRGB8888,
};

struct usb_hal *usb_intf_device_to_hal_func(struct device *dev)
{
	struct ms912x_device *ms912x = dev_get_drvdata(dev);
	return ms912x->hal;
}

static int ms912x_usb_probe(struct usb_interface *interface,
			    const struct usb_device_id *id)
{
	int ret;
	struct ms912x_device *ms912x;
	struct drm_device *dev;
	struct usb_hal *hal;

	ms912x = devm_drm_dev_alloc(&interface->dev, &driver,
				    struct ms912x_device, drm);
	if (IS_ERR(ms912x))
		return PTR_ERR(ms912x);

	ms912x->intf = interface;
	dev = &ms912x->drm;
	usb_set_intfdata(interface, ms912x);

	ret = kfifo_alloc(&ms912x->fifo, 1024, GFP_KERNEL);
	if (ret) {
		drm_err(dev, "alloc kfifo failed!\n ret = %d\n", ret);
		return -ENOMEM;
	}

	hal = usb_hal_init(interface, id, &ms912x->fifo, g_hal_index);
	if (IS_ERR(hal)) {
		drm_err(dev, "usb hal init failed!\n");
		return PTR_ERR(hal);
	}

	g_hal_index++;
	ms912x->hal = hal;

	ret = drmm_mode_config_init(dev);
	if (ret)
		goto err_destroy_usb_hal;

	dev->mode_config.min_width = 8;
	dev->mode_config.max_width = 1920;
	dev->mode_config.min_height = 8;
	dev->mode_config.max_height = 1080;
	dev->mode_config.funcs = &ms912x_mode_config_funcs;

	ret = ms912x_connector_init(ms912x);
	if (ret)
		goto err_destroy_usb_hal;

	ret = drm_simple_display_pipe_init(&ms912x->drm, &ms912x->display_pipe,
					   &ms912x_pipe_funcs,
					   ms912x_pipe_formats,
					   ARRAY_SIZE(ms912x_pipe_formats),
					   NULL, &ms912x->connector);
	if (ret)
		goto err_destroy_usb_hal;

	drm_mode_config_reset(dev);

	drm_kms_helper_poll_init(dev);

	ret = drm_dev_register(dev, 0);
	if (ret)
		goto err_destroy_usb_hal;

	drm_fbdev_generic_setup(dev, 0);

	return 0;

err_destroy_usb_hal:
	usb_hal_destroy(ms912x->hal);
	return ret;
}

static void ms912x_usb_disconnect(struct usb_interface *interface)
{
	struct ms912x_device *ms912x = usb_get_intfdata(interface);
	struct drm_device *dev = &ms912x->drm;
	int i;

	usb_hal_disable(ms912x->hal);
	for (i = 0; i < 100; i++)
	{
		if (!usb_hal_is_disabled(ms912x->hal))
		{
			msleep(50);
		}
	}
	usb_hal_destroy(ms912x->hal);

	drm_dev_unregister(dev);
	drm_kms_helper_poll_fini(dev);
	drm_dev_unplug(dev);
	drm_atomic_helper_shutdown(dev);
}

static const struct usb_device_id id_table[] = {
    {
	.idVendor = 0x345f,
	.idProduct = 0x9132,
	.bInterfaceClass = 0xff,
	.bInterfaceSubClass = 0x00,
	.bInterfaceProtocol = 0x00,
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR |
		       USB_DEVICE_ID_MATCH_PRODUCT |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS |
		       USB_DEVICE_ID_MATCH_INT_PROTOCOL,
    },
    {},
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver ms912x_driver = {
	.name = "ms912x",
	.probe = ms912x_usb_probe,
	.disconnect = ms912x_usb_disconnect,
	.id_table = id_table,
};
module_usb_driver(ms912x_driver);
MODULE_LICENSE("GPL");

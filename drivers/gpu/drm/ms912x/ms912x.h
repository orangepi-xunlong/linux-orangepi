
#ifndef MS912X_H
#define MS912X_H

#include <linux/mm_types.h>
#include <linux/scatterlist.h>
#include <linux/usb.h>
#include <linux/kfifo.h>

#include <drm/drm_device.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem.h>
#include <drm/drm_simple_kms_helper.h>

#define DRIVER_NAME "ms912x"
#define DRIVER_DESC "MacroSilicon USB to VGA/HDMI"
#define DRIVER_DATE "20220101"

#define DRIVER_MAJOR 0
#define DRIVER_MINOR 0
#define DRIVER_PATCHLEVEL 1

struct ms912x_device {
	struct drm_device drm;
	struct usb_interface *intf;
	struct usb_hal *hal;
	struct kfifo fifo;
	bool is_enable;

	struct drm_connector connector;
	struct drm_simple_display_pipe display_pipe;
};

#define to_ms912x(x) container_of(x, struct ms912x_device, drm)

int ms912x_connector_init(struct ms912x_device *ms912x);

#endif
/*
 * Kernel based bootsplash.
 *
 * (Main file: Glue code, workers, timer, PM, kernel and userland API)
 *
 * Authors:
 * Max Staudt <mstaudt@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#define pr_fmt(fmt) "bootsplash: " fmt


#include <linux/atomic.h>
#include <linux/bootsplash.h>
#include <linux/console.h>
#include <linux/device.h> /* dev_warn() */
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/selection.h> /* console_blanked */
#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/vt_kern.h>
#include <linux/workqueue.h>

#include "bootsplash_internal.h"
#include "uapi/linux/bootsplash_file.h"


/*
 * We only have one splash screen, so let's keep a single
 * instance of the internal state.
 */
static struct splash_priv splash_state;


static void splash_callback_redraw_vc(struct work_struct *ignored)
{
	if (console_blanked)
		return;

	console_lock();
	if (vc_cons[fg_console].d)
		update_screen(vc_cons[fg_console].d);
	console_unlock();
}

static void splash_callback_animation(struct work_struct *ignored)
{
	if (bootsplash_would_render_now()) {
		/* This will also re-schedule this delayed worker */
		splash_callback_redraw_vc(ignored);
	}
}


static bool is_fb_compatible(const struct fb_info *info)
{
	if (!(info->flags & FBINFO_BE_MATH)
	    != !fb_be_math((struct fb_info *)info)) {
		dev_warn(info->device,
			 "Can't draw on foreign endianness framebuffer.\n");

		return false;
	}

	if (info->flags & FBINFO_MISC_TILEBLITTING) {
		dev_warn(info->device,
			 "Can't draw splash on tiling framebuffer.\n");

		return false;
	}

	if (info->fix.type != FB_TYPE_PACKED_PIXELS
	    || (info->fix.visual != FB_VISUAL_TRUECOLOR
		&& info->fix.visual != FB_VISUAL_DIRECTCOLOR)) {
		dev_warn(info->device,
			 "Can't draw splash on non-packed or non-truecolor framebuffer.\n");

		dev_warn(info->device,
			 "  type: %u   visual: %u\n",
			 info->fix.type, info->fix.visual);

		return false;
	}

	if (info->var.bits_per_pixel != 16
	    && info->var.bits_per_pixel != 24
	    && info->var.bits_per_pixel != 32) {
		dev_warn(info->device,
			 "We only support drawing on framebuffers with 16, 24, or 32 bpp, not %d.\n",
			 info->var.bits_per_pixel);

		return false;
	}

	return true;
}


/*
 * Called by fbcon_switch() when an instance is activated or refreshed.
 */
void bootsplash_render_full(struct fb_info *info)
{
	bool is_update = false;

	mutex_lock(&splash_state.data_lock);

	/*
	 * If we've painted on this FB recently, we don't have to do
	 * the sanity checks and background drawing again.
	 */
	if (splash_state.splash_fb == info)
		is_update = true;


	if (!is_update) {
		/* Check whether we actually support this FB. */
		splash_state.splash_fb = NULL;

		if (!is_fb_compatible(info))
			goto out;

		/* Draw the background only once */
		bootsplash_do_render_background(info, splash_state.file);

		/* Mark this FB as last seen */
		splash_state.splash_fb = info;
	}

	bootsplash_do_render_pictures(info, splash_state.file, is_update);

	bootsplash_do_render_flush(info);

	bootsplash_do_step_animations(splash_state.file);

	/* Schedule update for animated splash screens */
	if (splash_state.file->frame_ms > 0)
		schedule_delayed_work(&splash_state.dwork_animation,
				      msecs_to_jiffies(
				      splash_state.file->frame_ms));

out:
	mutex_unlock(&splash_state.data_lock);
}


/*
 * External status enquiry and on/off switch
 */
bool bootsplash_would_render_now(void)
{
	return !oops_in_progress
		&& !console_blanked
		&& splash_state.file
		&& bootsplash_is_enabled();
}

void bootsplash_mark_dirty(void)
{
	mutex_lock(&splash_state.data_lock);
	splash_state.splash_fb = NULL;
	mutex_unlock(&splash_state.data_lock);
}

bool bootsplash_is_enabled(void)
{
	bool was_enabled;

	/* Make sure we have the newest state */
	smp_rmb();

	was_enabled = test_bit(0, &splash_state.enabled);

	return was_enabled;
}

void bootsplash_disable(void)
{
	int was_enabled;

	was_enabled = test_and_clear_bit(0, &splash_state.enabled);

	if (was_enabled) {
		if (oops_in_progress) {
			/* Redraw screen now so we can see a panic */
			if (vc_cons[fg_console].d)
				update_screen(vc_cons[fg_console].d);
		} else {
			/* No urgency, redraw at next opportunity */
			schedule_work(&splash_state.work_redraw_vc);
		}
	}
}

void bootsplash_enable(void)
{
	bool was_enabled;

	if (oops_in_progress)
		return;

	was_enabled = test_and_set_bit(0, &splash_state.enabled);

	if (!was_enabled) {
		/* Force a full redraw when the splash is re-activated */
		bootsplash_mark_dirty();

		schedule_work(&splash_state.work_redraw_vc);
	}
}


/*
 * Userland API via platform device in sysfs
 */
static ssize_t splash_show_enabled(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bootsplash_is_enabled());
}

static ssize_t splash_store_enabled(struct device *device,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	bool enable;
	int err;

	if (!buf || !count)
		return -EFAULT;

	err = kstrtobool(buf, &enable);
	if (err)
		return err;

	if (enable)
		bootsplash_enable();
	else
		bootsplash_disable();

	return count;
}

static ssize_t splash_store_drop_splash(struct device *device,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct splash_file_priv *fp;

	if (!buf || !count || !splash_state.file)
		return count;

	mutex_lock(&splash_state.data_lock);
	fp = splash_state.file;
	splash_state.file = NULL;
	mutex_unlock(&splash_state.data_lock);

	/* Redraw the text console */
	schedule_work(&splash_state.work_redraw_vc);

	bootsplash_free_file(fp);

	return count;
}

static ssize_t splash_store_load_file(struct device *device,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct splash_file_priv *fp, *fp_old;

	if (!count)
		return 0;

	fp = bootsplash_load_firmware(&splash_state.splash_device->dev,
				      buf);

	if (!fp)
		return -ENXIO;

	mutex_lock(&splash_state.data_lock);
	fp_old = splash_state.file;
	splash_state.splash_fb = NULL;
	splash_state.file = fp;
	mutex_unlock(&splash_state.data_lock);

	/* Update the splash or text console */
	schedule_work(&splash_state.work_redraw_vc);

	bootsplash_free_file(fp_old);
	return count;
}

static DEVICE_ATTR(enabled, 0644, splash_show_enabled, splash_store_enabled);
static DEVICE_ATTR(drop_splash, 0200, NULL, splash_store_drop_splash);
static DEVICE_ATTR(load_file, 0200, NULL, splash_store_load_file);


static struct attribute *splash_dev_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_drop_splash.attr,
	&dev_attr_load_file.attr,
	NULL
};

ATTRIBUTE_GROUPS(splash_dev);




/*
 * Power management fixup via platform device
 *
 * When the system is woken from sleep or restored after hibernating, we
 * cannot expect the screen contents to still be present in video RAM.
 * Thus, we have to redraw the splash if we're currently active.
 */
static int splash_resume(struct device *device)
{
	/*
	 * Force full redraw on resume since we've probably lost the
	 * framebuffer's contents meanwhile
	 */
	bootsplash_mark_dirty();

	if (bootsplash_would_render_now())
		schedule_work(&splash_state.work_redraw_vc);

	return 0;
}

static int splash_suspend(struct device *device)
{
	cancel_delayed_work_sync(&splash_state.dwork_animation);
	cancel_work_sync(&splash_state.work_redraw_vc);

	return 0;
}


static const struct dev_pm_ops splash_pm_ops = {
	.thaw = splash_resume,
	.restore = splash_resume,
	.resume = splash_resume,
	.suspend = splash_suspend,
	.freeze = splash_suspend,
};

static struct platform_driver splash_driver = {
	.driver = {
		.name = "bootsplash",
		.pm = &splash_pm_ops,
	},
};


/*
 * Main init
 */
void bootsplash_init(void)
{
	int ret;
	struct splash_file_priv *fp;

	/* Initialized already? */
	if (splash_state.splash_device)
		return;


	/* Register platform device to export user API */
	ret = platform_driver_register(&splash_driver);
	if (ret) {
		pr_err("platform_driver_register() failed: %d\n", ret);
		goto err;
	}

	splash_state.splash_device
		= platform_device_alloc("bootsplash", 0);

	if (!splash_state.splash_device)
		goto err_driver;

	splash_state.splash_device->dev.groups = splash_dev_groups;

	ret = platform_device_add(splash_state.splash_device);
	if (ret) {
		pr_err("platform_device_add() failed: %d\n", ret);
		goto err_device;
	}


	mutex_init(&splash_state.data_lock);
	set_bit(0, &splash_state.enabled);

	INIT_WORK(&splash_state.work_redraw_vc, splash_callback_redraw_vc);
	INIT_DELAYED_WORK(&splash_state.dwork_animation,
			  splash_callback_animation);


	if (!splash_state.bootfile || !strlen(splash_state.bootfile))
		return;

	fp = bootsplash_load_firmware(&splash_state.splash_device->dev,
				      splash_state.bootfile);

	if (!fp)
		goto err;

	mutex_lock(&splash_state.data_lock);
	splash_state.splash_fb = NULL;
	splash_state.file = fp;
	mutex_unlock(&splash_state.data_lock);

	return;

err_device:
	platform_device_put(splash_state.splash_device);
	splash_state.splash_device = NULL;
err_driver:
	platform_driver_unregister(&splash_driver);
err:
	pr_err("Failed to initialize.\n");
}


module_param_named(bootfile, splash_state.bootfile, charp, 0444);
MODULE_PARM_DESC(bootfile, "Bootsplash file to load on boot");

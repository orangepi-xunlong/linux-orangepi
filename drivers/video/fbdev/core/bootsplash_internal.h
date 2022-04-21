/*
 * Kernel based bootsplash.
 *
 * (Internal data structures used at runtime)
 *
 * Authors:
 * Max Staudt <mstaudt@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __BOOTSPLASH_INTERNAL_H
#define __BOOTSPLASH_INTERNAL_H


#include <linux/types.h>
#include <linux/fb.h>
#include <linux/firmware.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include "uapi/linux/bootsplash_file.h"


/*
 * Runtime types
 */
struct splash_blob_priv {
	struct splash_blob_header *blob_header;
	const void *data;
};


struct splash_pic_priv {
	const struct splash_pic_header *pic_header;

	struct splash_blob_priv *blobs;
	u16 blobs_loaded;

	u16 anim_nextframe;
};


struct splash_file_priv {
	const struct firmware *fw;
	const struct splash_file_header *header;

	struct splash_pic_priv *pics;

	/*
	 * A local copy of the frame delay in the header.
	 * We modify it to keep the code simple.
	 */
	u16 frame_ms;
};


struct splash_priv {
	/* Bootup and runtime state */
	char *bootfile;

	/*
	 * Enabled/disabled state, to be used with atomic bit operations.
	 *   Bit 0: 0 = Splash hidden
	 *          1 = Splash shown
	 *
	 * Note: fbcon.c uses this twice, by calling
	 *       bootsplash_would_render_now() in set_blitting_type() and
	 *       in fbcon_switch().
	 *       This is racy, but eventually consistent: Turning the
	 *       splash on/off will cause a redraw, which calls
	 *       fbcon_switch(), which calls set_blitting_type().
	 *       So the last on/off toggle will make things consistent.
	 */
	unsigned long enabled;

	/* Our gateway to userland via sysfs */
	struct platform_device *splash_device;

	struct work_struct work_redraw_vc;
	struct delayed_work dwork_animation;

	/* Splash data structures including lock for everything below */
	struct mutex data_lock;

	struct fb_info *splash_fb;

	struct splash_file_priv *file;
};



/*
 * Rendering functions
 */
void bootsplash_do_render_background(struct fb_info *info,
				     const struct splash_file_priv *fp);
void bootsplash_do_render_pictures(struct fb_info *info,
				   const struct splash_file_priv *fp,
				   bool is_update);
void bootsplash_do_render_flush(struct fb_info *info);
void bootsplash_do_step_animations(struct splash_file_priv *fp);


void bootsplash_free_file(struct splash_file_priv *fp);
struct splash_file_priv *bootsplash_load_firmware(struct device *device,
						  const char *path);

#endif

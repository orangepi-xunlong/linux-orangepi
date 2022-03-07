/*
 *  linux/drivers/video/fbdev/core/dummyblit.c -- Dummy Blitting Operation
 *
 *  Authors:
 *  Max Staudt <mstaudt@suse.de>
 *
 *  These functions are used in place of blitblit/tileblit to suppress
 *  fbcon's text output while a splash is shown.
 *
 *  Only suppressing actual rendering keeps the text buffer in the VC layer
 *  intact and makes it easy to switch back from the bootsplash to a full
 *  text console with a simple redraw (with the original functions in place).
 *
 *  Based on linux/drivers/video/fbdev/core/bitblit.c
 *       and linux/drivers/video/fbdev/core/tileblit.c
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/module.h>
#include <linux/fb.h>
#include <linux/vt_kern.h>
#include <linux/console.h>
#include <asm/types.h>
#include "fbcon.h"

static void dummy_bmove(struct vc_data *vc, struct fb_info *info, int sy,
			int sx, int dy, int dx, int height, int width)
{
	;
}

static void dummy_clear(struct vc_data *vc, struct fb_info *info, int sy,
			int sx, int height, int width)
{
	;
}

static void dummy_putcs(struct vc_data *vc, struct fb_info *info,
			const unsigned short *s, int count, int yy, int xx,
			int fg, int bg)
{
	;
}

static void dummy_clear_margins(struct vc_data *vc, struct fb_info *info,
				int color, int bottom_only)
{
	;
}

static void dummy_cursor(struct vc_data *vc, struct fb_info *info, int mode,
			int softback_lines, int fg, int bg)
{
	;
}

static int dummy_update_start(struct fb_info *info)
{
	/*
	 * Copied from bitblit.c and tileblit.c
	 *
	 * As of Linux 4.12, nobody seems to care about our return value.
	 */
	struct fbcon_ops *ops = info->fbcon_par;
	int err;

	err = fb_pan_display(info, &ops->var);
	ops->var.xoffset = info->var.xoffset;
	ops->var.yoffset = info->var.yoffset;
	ops->var.vmode = info->var.vmode;
	return err;
}

void fbcon_set_dummyops(struct fbcon_ops *ops)
{
	ops->bmove = dummy_bmove;
	ops->clear = dummy_clear;
	ops->putcs = dummy_putcs;
	ops->clear_margins = dummy_clear_margins;
	ops->cursor = dummy_cursor;
	ops->update_start = dummy_update_start;
	ops->rotate_font = NULL;
}
EXPORT_SYMBOL_GPL(fbcon_set_dummyops);

MODULE_AUTHOR("Max Staudt <mstaudt@suse.de>");
MODULE_DESCRIPTION("Dummy Blitting Operation");
MODULE_LICENSE("GPL");

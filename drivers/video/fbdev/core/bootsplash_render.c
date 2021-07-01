/*
 * Kernel based bootsplash.
 *
 * (Rendering functions)
 *
 * Authors:
 * Max Staudt <mstaudt@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#define pr_fmt(fmt) "bootsplash: " fmt


#include <linux/bootsplash.h>
#include <linux/fb.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>

#include "bootsplash_internal.h"
#include "uapi/linux/bootsplash_file.h"




/*
 * Rendering: Internal drawing routines
 */


/*
 * Pack pixel into target format and do Big/Little Endian handling.
 * This would be a good place to handle endianness conversion if necessary.
 */
static inline u32 pack_pixel(const struct fb_var_screeninfo *dst_var,
			     u8 red, u8 green, u8 blue)
{
	u32 dstpix;

	/* Quantize pixel */
	red = red >> (8 - dst_var->red.length);
	green = green >> (8 - dst_var->green.length);
	blue = blue >> (8 - dst_var->blue.length);

	/* Pack pixel */
	dstpix = red << (dst_var->red.offset)
		| green << (dst_var->green.offset)
		| blue << (dst_var->blue.offset);

	/*
	 * Move packed pixel to the beginning of the memory cell,
	 * so we can memcpy() it out easily
	 */
#ifdef __BIG_ENDIAN
	switch (dst_var->bits_per_pixel) {
	case 16:
		dstpix <<= 16;
		break;
	case 24:
		dstpix <<= 8;
		break;
	case 32:
		break;
	}
#else
	/* This is intrinsically unnecessary on Little Endian */
#endif

	return dstpix;
}


/*
 * Copy from source and blend into the destination picture.
 * Currently assumes that the source picture is 24bpp.
 * Currently assumes that the destination is <= 32bpp.
 */
static int splash_convert_to_fb(u8 *dst,
				const struct fb_var_screeninfo *dst_var,
				unsigned int dst_stride,
				unsigned int dst_xoff,
				unsigned int dst_yoff,
				const u8 *src,
				unsigned int src_width,
				unsigned int src_height)
{
	unsigned int x, y;
	unsigned int src_stride = 3 * src_width; /* Assume 24bpp packed */
	u32 dst_octpp = dst_var->bits_per_pixel / 8;

	dst_xoff += dst_var->xoffset;
	dst_yoff += dst_var->yoffset;

	/* Copy with stride and pixel size adjustment */
	for (y = 0;
	     y < src_height && y + dst_yoff < dst_var->yres_virtual;
	     y++) {
		const u8 *srcline = src + (y * src_stride);
		u8 *dstline = dst + ((y + dst_yoff) * dst_stride)
				  + (dst_xoff * dst_octpp);

		for (x = 0;
		     x < src_width && x + dst_xoff < dst_var->xres_virtual;
		     x++) {
			u8 red, green, blue;
			u32 dstpix;

			/* Read pixel */
			red = *srcline++;
			green = *srcline++;
			blue = *srcline++;

			/* Write pixel */
			dstpix = pack_pixel(dst_var, red, green, blue);
			memcpy(dstline, &dstpix, dst_octpp);

			dstline += dst_octpp;
		}
	}

	return 0;
}


void bootsplash_do_render_background(struct fb_info *info,
				     const struct splash_file_priv *fp)
{
	unsigned int x, y;
	u32 dstpix;
	u32 dst_octpp = info->var.bits_per_pixel / 8;

	dstpix = pack_pixel(&info->var,
			    fp->header->bg_red,
			    fp->header->bg_green,
			    fp->header->bg_blue);

	for (y = 0; y < info->var.yres_virtual; y++) {
		u8 *dstline = info->screen_buffer + (y * info->fix.line_length);

		for (x = 0; x < info->var.xres_virtual; x++) {
			memcpy(dstline, &dstpix, dst_octpp);

			dstline += dst_octpp;
		}
	}
}


void bootsplash_do_render_pictures(struct fb_info *info,
				   const struct splash_file_priv *fp,
				   bool is_update)
{
	unsigned int i;

	for (i = 0; i < fp->header->num_pics; i++) {
		struct splash_blob_priv *bp;
		struct splash_pic_priv *pp = &fp->pics[i];
		const struct splash_pic_header *ph = pp->pic_header;
		long dst_xoff, dst_yoff;

		if (pp->blobs_loaded < 1)
			continue;

		/* Skip static pictures when refreshing animations */
		if (ph->anim_type == SPLASH_ANIM_NONE && is_update)
			continue;

		bp = &pp->blobs[pp->anim_nextframe];

		if (!bp || bp->blob_header->type != 0)
			continue;

		switch (ph->position) {
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_TOP_LEFT:
			dst_xoff = 0;
			dst_yoff = 0;

			dst_xoff += ph->position_offset;
			dst_yoff += ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_TOP:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = 0;

			dst_yoff += ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_TOP_RIGHT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_yoff = 0;

			dst_xoff -= ph->position_offset;
			dst_yoff += ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_RIGHT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff -= ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_BOTTOM_RIGHT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_yoff = info->var.yres - pp->pic_header->height;

			dst_xoff -= ph->position_offset;
			dst_yoff -= ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_BOTTOM:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;

			dst_yoff -= ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_BOTTOM_LEFT:
			dst_xoff = 0 + ph->position_offset;
			dst_yoff = info->var.yres - pp->pic_header->height
						  - ph->position_offset;
			break;
		case SPLASH_POS_FLAG_CORNER | SPLASH_CORNER_LEFT:
			dst_xoff = 0;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff += ph->position_offset;
			break;

		case SPLASH_CORNER_TOP_LEFT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff -= ph->position_offset;
			dst_yoff -= ph->position_offset;
			break;
		case SPLASH_CORNER_TOP:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_yoff -= ph->position_offset;
			break;
		case SPLASH_CORNER_TOP_RIGHT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff += ph->position_offset;
			dst_yoff -= ph->position_offset;
			break;
		case SPLASH_CORNER_RIGHT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff += ph->position_offset;
			break;
		case SPLASH_CORNER_BOTTOM_RIGHT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff += ph->position_offset;
			dst_yoff += ph->position_offset;
			break;
		case SPLASH_CORNER_BOTTOM:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_yoff += ph->position_offset;
			break;
		case SPLASH_CORNER_BOTTOM_LEFT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff -= ph->position_offset;
			dst_yoff += ph->position_offset;
			break;
		case SPLASH_CORNER_LEFT:
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;

			dst_xoff -= ph->position_offset;
			break;

		default:
			/* As a fallback, center the picture. */
			dst_xoff = info->var.xres - pp->pic_header->width;
			dst_xoff /= 2;
			dst_yoff = info->var.yres - pp->pic_header->height;
			dst_yoff /= 2;
			break;
		}

		if (dst_xoff < 0
		    || dst_yoff < 0
		    || dst_xoff + pp->pic_header->width > info->var.xres
		    || dst_yoff + pp->pic_header->height > info->var.yres) {
			pr_info_once("Picture %u is out of bounds at current resolution: %dx%d\n"
				     "(this will only be printed once every reboot)\n",
				     i, info->var.xres, info->var.yres);

			continue;
		}

		/* Draw next splash frame */
		splash_convert_to_fb(info->screen_buffer, &info->var,
				info->fix.line_length, dst_xoff, dst_yoff,
				bp->data,
				pp->pic_header->width, pp->pic_header->height);
	}
}


void bootsplash_do_render_flush(struct fb_info *info)
{
	/*
	 * FB drivers using deferred_io (such as Xen) need to sync the
	 * screen after modifying its contents. When the FB is mmap()ed
	 * from userspace, this happens via a dirty pages callback, but
	 * when modifying the FB from the kernel, there is no such thing.
	 *
	 * So let's issue a fake fb_copyarea (copying the FB onto itself)
	 * to trick the FB driver into syncing the screen.
	 *
	 * A few DRM drivers' FB implementations are broken by not using
	 * deferred_io when they really should - we match on the known
	 * bad ones manually for now.
	 */
	if (info->fbdefio
	    || !strcmp(info->fix.id, "astdrmfb")
	    || !strcmp(info->fix.id, "cirrusdrmfb")
	    || !strcmp(info->fix.id, "mgadrmfb")) {
		struct fb_copyarea area;

		area.dx = 0;
		area.dy = 0;
		area.width = info->var.xres;
		area.height = info->var.yres;
		area.sx = 0;
		area.sy = 0;

		info->fbops->fb_copyarea(info, &area);
	}
}


void bootsplash_do_step_animations(struct splash_file_priv *fp)
{
	unsigned int i;

	/* Step every animation once */
	for (i = 0; i < fp->header->num_pics; i++) {
		struct splash_pic_priv *pp = &fp->pics[i];

		if (pp->blobs_loaded < 2
		    || pp->pic_header->anim_loop > pp->blobs_loaded)
			continue;

		if (pp->pic_header->anim_type == SPLASH_ANIM_LOOP_FORWARD) {
			pp->anim_nextframe++;
			if (pp->anim_nextframe >= pp->pic_header->num_blobs)
				pp->anim_nextframe = pp->pic_header->anim_loop;
		}
	}
}

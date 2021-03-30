/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/jiffies.h>
#include <linux/sunxi_tr.h>
#include "include.h"
#include "car_reverse.h"
#include "../../../video/sunxi/disp2/disp/dev_disp.h"

#undef _TRANSFORM_DEBUG

#define BUFFER_CNT	(4)
#define BUFFER_MASK	(0x03)

#define AUXLAYER_WIDTH	(480)
#define AUXLAYER_HEIGHT	(854)
#define AUXLAYER_SIZE	(AUXLAYER_WIDTH * AUXLAYER_HEIGHT * 4)

struct rect {
	int x, y;
	int w, h;
};

struct preview_private_data {
	struct device *dev;
	struct rect src;
	struct rect frame;
	struct rect screen;
	struct disp_layer_config config[2];
	int layer_cnt;

	int format;

	int rotation;
	unsigned long tr_handler;
	tr_info transform;
	struct buffer_pool *disp_buffer;
	struct buffer_node *fb[BUFFER_CNT];

#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
	struct buffer_pool *auxiliary_line;
	struct buffer_node *auxlayer;
#endif
};

/* for auxiliary line */
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
extern int draw_auxiliary_line(void *base, int width, int height);
int init_auxiliary_line(void);
#endif

/* function from sunxi transform driver */
extern unsigned long sunxi_tr_request(void);
extern int sunxi_tr_release(unsigned long hdl);
extern int sunxi_tr_commit(unsigned long hdl, tr_info *info);
extern int sunxi_tr_set_timeout(unsigned long hdl, unsigned long timeout);
extern int sunxi_tr_query(unsigned long hdl);

/* function from display driver */
extern struct disp_manager *disp_get_layer_manager(u32 disp);
static struct preview_private_data preview;

int preview_output_start(struct preview_params *params)
{
	int i;
	struct rect perfect;
	struct disp_manager *mgr = disp_get_layer_manager(0);

	if (!mgr || !mgr->force_set_layer_config) {
		logerror("preview init error\n");
		return -1;
	}

	memset(&preview, 0, sizeof(preview));
	preview.dev = params->dev;
	preview.src.w = params->src_width;
	preview.src.h = params->src_height;
	preview.format = params->format;
	preview.layer_cnt = 1;

	if (mgr->device && mgr->device->get_resolution) {
		mgr->device->get_resolution(mgr->device, &preview.screen.w, &preview.screen.h);
		logdebug("screen size: %dx%d\n", preview.screen.w, preview.screen.h);
	} else {
		preview.screen.w = 400;
		preview.screen.h = 400;
		logerror("can't get screen size, use default: %dx%d\n",
			preview.screen.w, preview.screen.h);
	}

	if (params->rotation) {
		perfect.w = params->screen_height;
		perfect.h = params->screen_width;
	} else {
		perfect.w = params->screen_width;
		perfect.h = params->screen_height;
	}
	preview.frame.w = (perfect.w > preview.screen.w) ? preview.screen.w : perfect.w;
	preview.frame.h = (perfect.h > preview.screen.h) ? preview.screen.h : perfect.h;

	preview.frame.x = (preview.screen.w - preview.frame.w) / 2;
	preview.frame.y = (preview.screen.h - preview.frame.h) / 2;

	if (params->rotation) {
		preview.rotation = params->rotation;
		preview.tr_handler = sunxi_tr_request();
		if (!preview.tr_handler) {
			logerror("request transform channel failed\n");
			return -1;
		}

		preview.disp_buffer = alloc_buffer_pool(preview.dev, BUFFER_CNT,
					preview.src.w * preview.src.h * 2);
		if (!preview.disp_buffer) {
			sunxi_tr_release(preview.tr_handler);
			logerror("request display buffer failed\n");
			return -1;
		}
		for (i = 0; i < BUFFER_CNT; i++)
			preview.fb[i] = preview.disp_buffer->dequeue_buffer(preview.disp_buffer);
	}
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
	init_auxiliary_line();
#endif
	return 0;
}

#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
int init_auxiliary_line(void)
{
	struct disp_layer_config *config;
	void *start;
	void *end;

	preview.auxiliary_line = alloc_buffer_pool(preview.dev, 1, AUXLAYER_SIZE);
	if (!preview.auxiliary_line) {
		logerror("request auxiliary line buffer failed\n");
		return -1;
	}
	preview.auxlayer = preview.auxiliary_line->dequeue_buffer(preview.auxiliary_line);
	if (!preview.auxlayer) {
		logerror("no buffer in buffer pool\n");
		return -1;
	}

	memset(preview.auxlayer->vir_address, 0, AUXLAYER_SIZE);
	draw_auxiliary_line(preview.auxlayer->vir_address, AUXLAYER_WIDTH, AUXLAYER_HEIGHT);

	start = preview.auxlayer->vir_address;
	end   = (void *)((unsigned long)start + preview.auxlayer->size);
	dmac_flush_range(start, end);

	config = &preview.config[1];
	memset(config, 0, sizeof(struct disp_layer_config));

	config->channel  = 1;
	config->enable   = 1;
	config->layer_id = 0;

	config->info.fb.addr[0] = (unsigned int)preview.auxlayer->phy_address;
	config->info.fb.format  = DISP_FORMAT_ARGB_8888;

	config->info.fb.size[0].width  = AUXLAYER_WIDTH;
	config->info.fb.size[0].height = AUXLAYER_HEIGHT;
	config->info.fb.size[1].width  = AUXLAYER_WIDTH;
	config->info.fb.size[1].height = AUXLAYER_HEIGHT;
	config->info.fb.size[2].width  = AUXLAYER_WIDTH;
	config->info.fb.size[2].height = AUXLAYER_HEIGHT;

	config->info.mode   = LAYER_MODE_BUFFER;
	config->info.zorder = 1;

	config->info.fb.crop.width  = (unsigned long long)AUXLAYER_WIDTH  << 32;
	config->info.fb.crop.height = (unsigned long long)AUXLAYER_HEIGHT << 32;

	config->info.alpha_mode        = 0;               /* pixel alpha */
	config->info.alpha_value       = 0;
	config->info.screen_win.x      = preview.frame.x;
	config->info.screen_win.y      = preview.frame.y;
	config->info.screen_win.width  = preview.frame.w;
	config->info.screen_win.height = preview.frame.h;

	preview.layer_cnt++;
	return 0;
}

void deinit_auxiliary_line(void)
{
	if (preview.auxlayer)
		preview.auxiliary_line->queue_buffer(preview.auxiliary_line, preview.auxlayer);
	free_buffer_pool(preview.dev, preview.auxiliary_line);
}
#endif

int preview_output_stop(void)
{
	int i;
	struct disp_manager *mgr = disp_get_layer_manager(0);
	if (!mgr || !mgr->force_set_layer_config_exit) {
		logerror("preview stop error\n");
		return -1;
	}
	mgr->force_set_layer_config_exit(mgr);
	msleep(100);

	if (preview.tr_handler)
		sunxi_tr_release(preview.tr_handler);

	if (preview.disp_buffer) {
		for (i = 0; i < BUFFER_CNT; i++) {
			if (!preview.fb[i])
				continue;

			preview.disp_buffer->queue_buffer(preview.disp_buffer, preview.fb[i]);
		}

		free_buffer_pool(preview.dev, preview.disp_buffer);
	}
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
	deinit_auxiliary_line();
#endif
	return 0;
}

int image_rotate(struct buffer_node *frame, struct buffer_node **rotate)
{
	static int active;
	int retval;
	struct buffer_node *node;
	tr_info *info = &preview.transform;

#ifdef _TRANSFORM_DEBUG
	unsigned long start_jiffies;
	unsigned long end_jiffies;
	unsigned long time;
#endif

	active++;
	node = preview.fb[active & BUFFER_MASK];
	if (!node || !frame) {
		logerror("%s, alloc buffer failed\n", __func__);
		return -1;
	}

	info->mode = TR_ROT_90;
	info->src_frame.fmt = TR_FORMAT_YUV420_SP_UVUV;
	info->src_frame.laddr[0]  = (unsigned int)frame->phy_address;
	info->src_frame.laddr[1]  = (unsigned int)frame->phy_address + preview.src.w * preview.src.h;
	info->src_frame.laddr[2]  = 0;
	info->src_frame.pitch[0]  = preview.src.w;
	info->src_frame.pitch[1]  = preview.src.w / 2;
	info->src_frame.pitch[2]  = 0;
	info->src_frame.height[0] = preview.src.h;
	info->src_frame.height[1] = preview.src.h / 2;
	info->src_frame.height[2] = 0;
	info->src_rect.x = 0;
	info->src_rect.y = 0;
	info->src_rect.w = preview.src.w;
	info->src_rect.h = preview.src.h;

	info->dst_frame.fmt = TR_FORMAT_YUV420_P;
	info->dst_frame.laddr[0]  = (unsigned long)node->phy_address;
	info->dst_frame.laddr[1]  = (unsigned long)node->phy_address +
					preview.src.w * preview.src.h;
	info->dst_frame.laddr[2]  = (unsigned long)node->phy_address +
					preview.src.w * preview.src.h +
					preview.src.w * preview.src.h / 4;
	info->dst_frame.pitch[0]  = preview.src.h;
	info->dst_frame.pitch[1]  = preview.src.h / 2;
	info->dst_frame.pitch[2]  = preview.src.h / 2;
	info->dst_frame.height[0] = preview.src.w;
	info->dst_frame.height[1] = preview.src.w / 2;
	info->dst_frame.height[2] = preview.src.w / 2;
	info->dst_rect.x = 0;
	info->dst_rect.y = 0;
	info->dst_rect.w = preview.src.h;
	info->dst_rect.h = preview.src.w;

#ifdef _TRANSFORM_DEBUG
	start_jiffies = jiffies;
#endif
	if (sunxi_tr_commit(preview.tr_handler, info) < 0) {
		logerror("transform commit error!\n");
		return -1;
	}

	while (1) {
		retval = sunxi_tr_query(preview.tr_handler);
		if (retval == 0 || retval == -1)
			break;
		msleep(1);
	}
	*rotate = node;

#ifdef _TRANSFORM_DEBUG
	end_jiffies = jiffies;
	time = jiffies_to_msecs(end_jiffies - start_jiffies);
	logerror("TR:%ld ms\n", time);
#endif

	return retval;
}

void preview_update(struct buffer_node *frame)
{
	struct disp_layer_config *config = &preview.config[0];
	struct buffer_node *rotate = NULL;
	int buffer_format = preview.format;
	int width  = preview.src.w;
	int height = preview.src.h;

	struct disp_manager *mgr = disp_get_layer_manager(0);
	if (!mgr || !mgr->force_set_layer_config) {
		logerror("preview update error\n");
		return;
	}

	if (preview.rotation && image_rotate(frame, &rotate) == 0) {
		if (!rotate) {
			logerror("image rotate error\n");
			return;
		}
		buffer_format = V4L2_PIX_FMT_YVU420;
		width  = preview.src.h;
		height = preview.src.w;
	}

	switch (buffer_format) {
	case V4L2_PIX_FMT_NV21:
		config->info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
		config->info.fb.addr[0] = (unsigned int)frame->phy_address;
		config->info.fb.addr[1] = (unsigned int)frame->phy_address + (width * height);
		config->info.fb.addr[2] = 0;
		config->info.fb.size[0].width  = width;
		config->info.fb.size[0].height = height;
		config->info.fb.size[1].width  = width  / 2;
		config->info.fb.size[1].height = height / 2;
		config->info.fb.size[2].width  = 0;
		config->info.fb.size[2].height = 0;
		config->info.fb.align[1] = 0;
		config->info.fb.align[2] = 0;

		config->info.fb.crop.width  = (unsigned long long)width  << 32;
		config->info.fb.crop.height = (unsigned long long)height << 32;
		config->info.screen_win.x = preview.frame.x;
		config->info.screen_win.y = preview.frame.y;
		config->info.screen_win.width  = preview.frame.w;
		config->info.screen_win.height = preview.frame.h;

		config->channel  = 0;
		config->layer_id = 0;
		config->enable   = 1;

		config->info.mode        = LAYER_MODE_BUFFER;
		config->info.zorder      = 12;
		config->info.alpha_mode  = 1;
		config->info.alpha_value = 0xff;
		break;
	case V4L2_PIX_FMT_YVU420:
		config->info.fb.format  = DISP_FORMAT_YUV420_P;
		config->info.fb.addr[0] = (unsigned int)rotate->phy_address;
		config->info.fb.addr[1] = (unsigned int)rotate->phy_address + (width * height) + (width * height / 4);
		config->info.fb.addr[2] = (unsigned int)rotate->phy_address + (width * height);
		config->info.fb.size[0].width  = width;
		config->info.fb.size[0].height = height;
		config->info.fb.size[1].width  = width  / 2;
		config->info.fb.size[1].height = height / 2;
		config->info.fb.size[2].width  = width  / 2;
		config->info.fb.size[2].height = height / 2;
		config->info.fb.align[1] = 0;
		config->info.fb.align[2] = 0;

		config->info.fb.crop.width  = (unsigned long long)width  << 32;
		config->info.fb.crop.height = (unsigned long long)height << 32;
		config->info.screen_win.x = preview.frame.x;
		config->info.screen_win.y = preview.frame.y;
		config->info.screen_win.width  = preview.frame.w;	/* FIXME */
		config->info.screen_win.height = preview.frame.h;

		config->channel  = 0;
		config->layer_id = 0;
		config->enable   = 1;

		config->info.mode        = LAYER_MODE_BUFFER;
		config->info.zorder      = 0;
		config->info.alpha_mode  = 1;
		config->info.alpha_value = 0xff;
		break;
	default:
		logerror("%s: unknown pixel format, skip\n", __func__);
		break;
	}

	mgr->force_set_layer_config(mgr, &preview.config[0], preview.layer_cnt);
}

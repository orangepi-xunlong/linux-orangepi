/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse preview display module
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "car_reverse.h"
#include "buffer_pool.h"
#include "include.h"
#include "../../video/sunxi/disp2/disp/de/include.h"
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <video/sunxi_display2.h>
#include <asm/cacheflush.h>

#define BUFFER_CNT (5)

struct rect {
	int x, y;
	int w, h;
};

struct preview_private_data {
	struct rect src;
	struct rect frame;
	struct rect screen;
	/* if over view is not used, 0 for car_preview, 1 for aux_line */
	struct disp_layer_config2 config[4];
	int layer_cnt;

	int format;
	int input_src;
	int rotation;
	int oview_mode;
	int is_enable;
	int screen_size_adaptive;
	int g2d_used;
	int di_used;
};


static struct mutex oview_mutex;

/* function from display driver */
extern struct disp_manager *disp_get_layer_manager(u32 disp);
extern int disp_get_num_screens(void);
extern s32 bsp_disp_shadow_protect(u32 disp, bool protect);
static struct preview_private_data preview[3];

int preview_output_config(struct disp_manager *mgr, int screen_id, \
		int default_width, int default_height, int adaptive_size)
{
	struct rect perfect;

	preview[screen_id].screen.w = default_width;
	preview[screen_id].screen.h = default_height;
	if (adaptive_size) {
		if (mgr->device && mgr->device->get_resolution) {
			mgr->device->get_resolution(mgr->device, &preview[screen_id].screen.w,
					&preview[screen_id].screen.h);
			PREVIEW_DISPLAY_DBG("adaptive screen[%d] size: %dx%d\n", \
				screen_id, preview[screen_id].screen.w, preview[screen_id].screen.h);
		} else {
			CAR_REVERSE_ERR("get_resolution is not support in disp device, use default\n");
		}
	}

	perfect.w = preview[screen_id].screen.w;
	perfect.h = preview[screen_id].screen.h;
	if (preview[screen_id].g2d_used) {
		if ((preview[screen_id].rotation == 1) || (preview[screen_id].rotation == 3)) {
			perfect.w = preview[screen_id].screen.h;
			perfect.h = preview[screen_id].screen.w;
		}
	}

	preview[screen_id].frame.w =
		(perfect.w > preview[screen_id].screen.w)
		? preview[screen_id].screen.w
		: perfect.w;
	preview[screen_id].frame.h =
		(perfect.h > preview[screen_id].screen.h)
		? preview[screen_id].screen.h
		: perfect.h;

	preview[screen_id].frame.x =
		(preview[screen_id].screen.w - preview[screen_id].frame.w) / 2;
	preview[screen_id].frame.y =
		(preview[screen_id].screen.h - preview[screen_id].frame.h) / 2;

	PREVIEW_DISPLAY_DBG("preview_screen_size: x:%d y:%d w:%d h:%d\n", preview[screen_id].screen.x, \
		preview[screen_id].screen.y, preview[screen_id].screen.w, preview[screen_id].screen.h);
	PREVIEW_DISPLAY_DBG("preview_frame_size: x:%d y:%d w:%d h:%d\n", preview[screen_id].frame.x, \
		preview[screen_id].frame.y, preview[screen_id].frame.w, preview[screen_id].frame.h);

	preview[screen_id].is_enable = 1;
	return 0;
}

int preview_output_init(struct preview_params *params)
{
	int num_screens, screen_id;
	int i = 0, try_cnt = 10;
	struct disp_manager *mgr = NULL;
	num_screens = disp_get_num_screens();

	mutex_init(&oview_mutex);
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config2) {
			CAR_REVERSE_ERR("screen %d preview init error\n", screen_id);
			return -1;
		}
		memset(&(preview[screen_id]), 0, sizeof(preview[screen_id]));
		preview[screen_id].src.w = params->src_width;
		preview[screen_id].src.h = params->src_height;
		preview[screen_id].layer_cnt = 1;
		preview[screen_id].input_src = params->input_src;
		preview[screen_id].oview_mode = params->oview_type;
		preview[screen_id].rotation = params->rotation;
		preview[screen_id].g2d_used = params->g2d_used;
		preview[screen_id].di_used = params->di_used;
		preview[screen_id].screen_size_adaptive = params->screen_size_adaptive;
		preview[screen_id].format = params->format;

		if (mgr->device) {
			/* ensure de and display device ready, especially
			 * when resume sequence is not specifily during standby*/
			for (i = 0; i < try_cnt; i++) {
				if (mgr->device->is_enabled(mgr->device)) {
					break;
				}
				msleep(50);
			}

			if (mgr->device->is_enabled(mgr->device)) {
				preview_output_config(mgr, screen_id, params->screen_width, \
					params->screen_height, params->screen_size_adaptive);
			}
		}
	}
	return 0;
}


int preview_output_disable(void)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;
	num_screens = disp_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->device)
			continue;
		preview[screen_id].config[0].enable = 0;
		preview[screen_id].config[1].enable = 0;
		preview[screen_id].config[2].enable = 0;
		preview[screen_id].config[3].enable = 0;

		mgr->force_set_layer_config2(mgr, preview[screen_id].config,
					    preview[screen_id].layer_cnt);
		msleep(20);
	}
	return 0;
}

int preview_output_exit(struct preview_params *params)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;
	num_screens = disp_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config2) {
			CAR_REVERSE_ERR("screen %d preview stop error\n", screen_id);
			return -1;
		}
		mgr->force_set_layer_config_exit(mgr);
		msleep(20);
	}
	return 0;
}


void preview_layer_config_update(struct buffer_node *frame)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;
	struct disp_layer_config2 *config = NULL;
	int buffer_format;
	int width;
	int height;
	if (frame == NULL) {
		CAR_REVERSE_ERR("preview frame is null\n");
		return;
	}

	num_screens = disp_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config2) {
			CAR_REVERSE_ERR("preview update error\n");
			return;
		}
		if (!mgr->device || !mgr->device->is_enabled(mgr->device))
			continue;
		if (!preview[screen_id].is_enable) {
			CAR_REVERSE_ERR("preview output config is not init yet!\n");
		}
		config = &preview[screen_id].config[0];
		buffer_format = preview[screen_id].format;
		width = preview[screen_id].src.w;
		height = preview[screen_id].src.h;

		if (preview[screen_id].g2d_used && preview[screen_id].rotation) {
			if (preview[screen_id].rotation == 1 || preview[screen_id].rotation == 3) {
				width = preview[screen_id].src.h;
				height = preview[screen_id].src.w;
			}
		}

		switch (buffer_format) {
		case V4L2_PIX_FMT_NV21:
			config->info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
			config->info.fb.fd = frame->dmabuf_fd;

			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height / 2;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.x = (unsigned long long)0 << 32;
			config->info.fb.crop.y = (unsigned long long)0 << 32;
			config->info.fb.crop.width = (unsigned long long)(width) << 32;
			config->info.fb.crop.height = (unsigned long long)height << 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width = preview[screen_id].frame.w;
			config->info.screen_win.height = preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER_FORCE;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		case V4L2_PIX_FMT_NV61:
			config->info.fb.format = DISP_FORMAT_YUV422_SP_VUVU;
			config->info.fb.fd = frame->dmabuf_fd;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.x = (unsigned long long)40 << 32;
			config->info.fb.crop.y = (unsigned long long)0 << 32;
			config->info.fb.crop.width = (long)(width - 80) << 32;
			config->info.fb.crop.height = (unsigned long long)height << 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width = preview[screen_id].frame.w;
			config->info.screen_win.height = preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER_FORCE;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		case V4L2_PIX_FMT_YVU420:
			config->info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
			config->info.fb.fd = frame->dmabuf_fd;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height / 2;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.width = (unsigned long long)width << 32;
			config->info.fb.crop.height = (unsigned long long)height << 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width = preview[screen_id].frame.w; /* FIXME */
			config->info.screen_win.height = preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER_FORCE;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		case V4L2_PIX_FMT_YUV422P:
			config->info.fb.format = DISP_FORMAT_YUV422_SP_VUVU;
			config->info.fb.fd = frame->dmabuf_fd;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.width = (unsigned long long)width << 32;
			config->info.fb.crop.height = (unsigned long long)height << 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width = preview[screen_id].frame.w; /* FIXME */
			config->info.screen_win.height = preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER_FORCE;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		default:
			CAR_REVERSE_ERR("unknown pixel format, skip\n");
			break;
		}
	}
}

void preview_Ov_layer_config_update(struct buffer_node **frame_input, int overview_type)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;
	struct disp_layer_config2 *config = NULL;
	int buffer_format;
	int width;
	int height;
	int i = 0;
	unsigned layer_cnt = 0;
	struct buffer_node *frame = NULL;

	if (frame_input == NULL) {
		CAR_REVERSE_ERR("Ov preview frame is null\n");
		return;
	}

	mutex_lock(&oview_mutex);
	num_screens = disp_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config2) {
			CAR_REVERSE_ERR("preview update error\n");
			return;
		}
		if (!mgr->device || !mgr->device->is_enabled(mgr->device))
			continue;
		buffer_format = preview[screen_id].format;
		width = preview[screen_id].src.w;
		height = preview[screen_id].src.h;

		layer_cnt = 0;
		for (i = 0; i < CAR_MAX_CH && i < overview_type; i++) {
			frame = frame_input[i];
			config = &preview[screen_id].config[i];
			switch (buffer_format) {
			case V4L2_PIX_FMT_NV21:
				config->info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
				config->info.fb.fd = frame->dmabuf_fd;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height / 2;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.x = (unsigned long long)40 << 32;
				config->info.fb.crop.y = (unsigned long long)0 << 32;
				config->info.fb.crop.width = (unsigned long long) (width - 80) << 32;
				config->info.fb.crop.height = (unsigned long long)height << 32;
				if (overview_type == 2) {
					if (i == 0) {
						config->info.screen_win.x = 0;
						config->info.screen_win.y = 0;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h;
					}
					if (i == 1) {
						config->info.screen_win.x = preview[screen_id].frame.w / 2;
						config->info.screen_win.y = 0;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h;
					}
					config->channel = 0;
					config->layer_id = i;
					config->info.zorder = i;
				} else if (overview_type == 3) {
					if (i == 0) {
						config->info.screen_win.x = 0;
						config->info.screen_win.y = 0;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					if (i == 1) {
						config->info.screen_win.x = preview[screen_id].frame.w / 2;
						config->info.screen_win.y = 0;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					if (i == 2) {
						config->info.screen_win.x = preview[screen_id].frame.w / 4;
						config->info.screen_win.y = preview[screen_id].frame.h / 2;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					config->channel = 0;
					config->layer_id = i;
					config->info.zorder = i;
				} else if (overview_type == 4) {
					if (i == 0) {
						config->info.screen_win.x = 0;
						config->info.screen_win.y = 0;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					if (i == 1) {
						config->info.screen_win.x = preview[screen_id].frame.w / 2;
						config->info.screen_win.y = 0;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					if (i == 2) {
						config->info.screen_win.x = 0;
						config->info.screen_win.y = preview[screen_id].frame.h / 2;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					if (i == 3) {
						config->info.screen_win.x = preview[screen_id].frame.w / 2;
						config->info.screen_win.y = preview[screen_id].frame.h / 2;
						config->info.screen_win.width = preview[screen_id].frame.w / 2;
						config->info.screen_win.height = preview[screen_id].frame.h / 2;
					}
					config->channel = 0;
					config->layer_id = i;
					config->info.zorder = i;
				} else {
					CAR_REVERSE_ERR("unknown overview_type(%d), skip\n", overview_type);
					break;
				}
				config->enable = 1;
				config->info.mode = LAYER_MODE_BUFFER_FORCE;
				config->info.alpha_mode = 0;
				config->info.alpha_value = 0xff;
				layer_cnt++;
				break;
			default:
				CAR_REVERSE_ERR("unknown pixel format, skip\n");
				break;
			}
		}
		preview[screen_id].layer_cnt = layer_cnt;
	}
	mutex_unlock(&oview_mutex);
	return ;
}


void aux_line_layer_config_update(struct buffer_node *frame)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;
	struct disp_layer_config2 *config = NULL;
	int buffer_format;
	int width;
	int height;
	unsigned layer_cnt = 1;
	if (frame == NULL) {
		CAR_REVERSE_ERR("preview frame is null\n");
		return;
	}

	num_screens = disp_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config2) {
			CAR_REVERSE_ERR("preview update error\n");
			return;
		}
		if (!mgr->device || !mgr->device->is_enabled(mgr->device))
			continue;
		if (!preview[screen_id].is_enable) {
			CAR_REVERSE_ERR("preview output config is not init yet!\n");
			continue;
		}

		layer_cnt = 1;
		config = &preview[screen_id].config[1];
		buffer_format = preview[screen_id].format;

		if (preview[screen_id].rotation == 1 || preview[screen_id].rotation == 3) {
			width = AUXLAYER_HEIGHT;
			height = AUXLAYER_WIDTH;
		} else {
			width = AUXLAYER_WIDTH;
			height = AUXLAYER_HEIGHT;
		}

		config->info.fb.format = DISP_FORMAT_ARGB_8888;
		config->info.fb.fd = frame->dmabuf_fd;

		config->info.fb.size[0].width = width;
		config->info.fb.size[0].height = height;
		config->info.fb.size[1].width = 0;
		config->info.fb.size[1].height = 0;
		config->info.fb.size[2].width = 0;
		config->info.fb.size[2].height = 0;
		config->info.fb.align[1] = 0;
		config->info.fb.align[2] = 0;

		config->info.fb.crop.x = (unsigned long long)0 << 32;
		config->info.fb.crop.y = (unsigned long long)0 << 32;
		config->info.fb.crop.width = (unsigned long long)(width) << 32;
		config->info.fb.crop.height = (unsigned long long)(height) << 32;
		config->info.screen_win.x = preview[screen_id].frame.x;
		config->info.screen_win.y = preview[screen_id].frame.y;
		config->info.screen_win.width = preview[screen_id].frame.w;
		config->info.screen_win.height = preview[screen_id].frame.h;

		config->channel = 1;
		config->layer_id = 0;
		config->enable = 1;

		config->info.mode = LAYER_MODE_BUFFER_FORCE;
		config->info.zorder = 2;
		config->info.alpha_mode = 0;
		config->info.alpha_value = 0;
		layer_cnt++;
		preview[screen_id].layer_cnt = layer_cnt;
	}
}

void preview_display(void)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;

	mutex_lock(&oview_mutex);
	num_screens = disp_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config2) {
			CAR_REVERSE_ERR("preview update error\n");
			return;
		}
		if (!mgr->device || !mgr->device->is_enabled(mgr->device))
			continue;
		bsp_disp_shadow_protect(screen_id, true);
		mgr->force_set_layer_config2(mgr, preview[screen_id].config, preview[screen_id].layer_cnt);
		bsp_disp_shadow_protect(screen_id, false);
	}
	mutex_unlock(&oview_mutex);
}

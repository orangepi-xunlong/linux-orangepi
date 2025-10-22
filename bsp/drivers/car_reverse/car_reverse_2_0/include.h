/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse head file
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

#ifndef __CAR_REVERSE_MISC_H__
#define __CAR_REVERSE_MISC_H__

#include "car_reverse.h"
#include "buffer_pool.h"

#define AUXLAYER_WIDTH (720)
#define AUXLAYER_HEIGHT (480)

#if IS_ENABLED(CONFIG_SUPPORT_PREVIEW_ROTATOR) || IS_ENABLED(CONFIG_SUPPORT_PREVIEW_ROTATOR_MODULE)
int preview_rotator_init(struct preview_params *params);
int preview_rotator_exit(void);
int preview_image_rotate(struct buffer_node *frame, struct buffer_node **rotate, int src_w,
			 int src_h, int angle, int format);
#else
static inline int preview_rotator_init(struct preview_params *params)
{
	CAR_REVERSE_ERR("car reverse preview rotator is not enable yet!\n");
	return 0;
}

static inline int preview_rotator_exit(void)
{
	CAR_REVERSE_ERR("car reverse preview rotator is not enable yet!\n");
	return 0;
}

static inline int preview_image_rotate(struct buffer_node *frame, struct buffer_node **rotate, int src_w,
			 int src_h, int angle, int format)
{
	CAR_REVERSE_ERR("car reverse preview rotator is not enable yet!\n");
	return 0;
}
#endif /* end of preview rotator */


#if IS_ENABLED(CONFIG_SUPPORT_PREVIEW_ENHANCER) || IS_ENABLED(CONFIG_SUPPORT_PREVIEW_ENHANCER_MODULE)
int preview_enhancer_init(struct preview_params *params);
int preview_enhancer_exit(void);
int preview_image_enhance(struct buffer_node *frame, struct buffer_node **di_frame, int src_width,
			 int src_height, int format);
#else
static inline int preview_enhancer_init(struct preview_params *params)
{
	CAR_REVERSE_ERR("car reverse preview enhancer is not enable yet!\n");
	return -1;
}

static inline int preview_enhancer_exit(void)
{
	CAR_REVERSE_ERR("car reverse preview enhancer is not enable yet!\n");
	return -1;
}

static inline int preview_image_enhance(struct buffer_node *frame, struct buffer_node **di_frame, int src_width,
			 int src_height, int format)
{
	CAR_REVERSE_ERR("car reverse preview enhancer is not enable yet!\n");
	return -1;
}
#endif /* end of preview enhancer */


#if IS_ENABLED(CONFIG_SUPPORT_AUXILIARY_LINE) || IS_ENABLED(CONFIG_SUPPORT_AUXILIARY_LINE_MODULE)
int auxiliary_line_init(void);
int static_aux_line_update(struct buffer_node *aux, int orientation, int lr, int width, int height);
int dynamic_aux_line_update(struct buffer_node *aux, int orientation, int lr, int width, int height);
#else
static inline int auxiliary_line_init(void)
{
	CAR_REVERSE_ERR("car reverse auxiliary line is not enable yet!\n");
	return -1;
}

static inline int static_aux_line_update(struct buffer_node *aux, int orientation, int lr, int width, int height)
{
	CAR_REVERSE_ERR("car reverse auxiliary line is not enable yet!\n");
	return -1;
}

static inline int dynamic_aux_line_update(struct buffer_node *aux, int orientation, int lr, int width, int height)
{
	CAR_REVERSE_ERR("car reverse auxiliary line is not enable yet!\n");
	return -1;
}
#endif /* end of aux line */

int preview_output_init(struct preview_params *params);
int preview_output_exit(struct preview_params *params);
void preview_display(void);
void preview_layer_config_update(struct buffer_node *frame);
void aux_line_layer_config_update(struct buffer_node *frame);
void preview_Ov_layer_config_update(struct buffer_node **frame, int overview_type);
int preview_output_disable(void);

#endif

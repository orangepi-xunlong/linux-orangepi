/*
 * Fast car reverse head file
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

#ifndef __CAR_REVERSE_MISC_H__
#define __CAR_REVERSE_MISC_H__

#include "car_reverse.h"

struct buffer_pool *
		alloc_buffer_pool(struct device *dev, int depth, int buf_size);
void free_buffer_pool(struct device *dev, struct buffer_pool *bp);
struct buffer_node *__buffer_node_alloc(struct device *dev, int size, int flags);
void __buffer_node_free(struct device *dev, struct buffer_node *node);
void rest_buffer_pool(struct device *dev, struct buffer_pool *bp);
void dump_buffer_pool(struct device *dev, struct buffer_pool *bp);
int preview_output_start(struct preview_params *params);
int preview_output_stop(struct preview_params *params);
void preview_update(struct buffer_node *frame, int orientation, int lr_direct);
void preview_update_Ov(struct buffer_node **frame, int orientation, int lr_direct);
void display_frame_work(void);
int video_source_connect(struct preview_params *params, int tvd_fd);
int video_source_disconnect(struct preview_params *params, int tvd_fd);
int video_source_streamon(int tvd_fd);
int video_source_streamoff(int tvd_fd);

struct buffer_node *video_source_dequeue_buffer(int tvd_fd);
void video_source_queue_buffer(struct buffer_node *node,  int tvd_fd);

int video_source_streamon_vin(int tvd_fd);
int video_source_streamoff_vin(int tvd_fd);

struct buffer_node *video_source_dequeue_buffer_vin(int tvd_fd);
void video_source_queue_buffer_vin(struct buffer_node *node, int tvd_fd);


#endif

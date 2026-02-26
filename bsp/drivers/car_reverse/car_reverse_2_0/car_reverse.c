/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse driver module
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
#include "video_source.h"
#include "include.h"

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define MODULE_NAME "car-reverse"

static dev_t devid;
static struct cdev *car_reverse_cdev;
static struct class *car_reverse_class;
static struct device *cardev;

u32 reverse_loglevel_debug;

struct car_reverse_private_data {
	struct platform_device *pdev;
	struct device *dev;
	struct preview_params config;
	struct preview_params def_config;
	struct car_reverse_debug debug;

	struct buffer_pool *buffer_pool;
	struct buffer_pool *bufferOv_pool[CAR_MAX_CH];
	struct buffer_node *buffer_auxiliary_line;
	struct buffer_node *buffer_di[2];
	/* 0:for rotate  1:for scaler  2:for auxiliary_line */
	struct buffer_node *buffer_g2d[G2D_BUF_MAX];

	struct work_struct status_detect;
	struct workqueue_struct *preview_workqueue;

	struct task_struct *display_update_task;
	struct mutex preview_lock;

	int reverse_gpio;

	int status;
	int thread_mask;
	int discard_frame;
	int standby;
	int first_buf_unready;
	spinlock_t thread_lock;
	struct mutex alloc_lock;
	struct mutex free_lock;
};

static struct car_reverse_private_data *car_reverse;

static int car_reverse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int car_reverse_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t car_reverse_read(struct file *file, char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static ssize_t car_reverse_write(struct file *file, const char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static int car_reverse_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static long car_reverse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long arg64[2] = {0};

	if (copy_from_user
	    ((void *)arg64, (void __user *)arg, 2 * sizeof(unsigned long))) {
		CAR_REVERSE_WRN("copy_from_user fail\n");
		return -EFAULT;
	}

	switch (cmd) {
	case CMD_CAR_REVERSE_FORCE_SET_STATE:
		CAR_DRIVER_DBG("ioctl buffer[0]:%ld\n", arg64[0]);
		if ((arg64[0] > CAR_REVERSE_NULL) && (arg64[0] < CAR_REVERSE_STATUS_MAX)) {
			if (car_reverse->debug.user_status == arg64[0])
				return 0;
			car_reverse->debug.user_status = arg64[0];
			queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
		} else if (arg64[0] == CAR_REVERSE_NULL)
			car_reverse->debug.user_status = arg64[0];
		else
			CAR_REVERSE_ERR("CMD_CAR_REVERSE_FORCE_SET_STATE: unsupport status(0x%lx)!\n", arg64[0]);
		break;
	default:
		CAR_REVERSE_ERR("ERROR cmd:%d!\n", cmd);
		return -EINVAL;
	}
	return 0;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long car_reverse_compat_ioctl(struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	return car_reverse_ioctl(filp, cmd, (unsigned long)arg);
}
#endif

static const struct file_operations car_reverse_fops = {
	.owner		= THIS_MODULE,
	.open		= car_reverse_open,
	.release	= car_reverse_release,
	.write		= car_reverse_write,
	.read		= car_reverse_read,
	.unlocked_ioctl	= car_reverse_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl	= car_reverse_compat_ioctl,
#endif
	.mmap		= car_reverse_mmap,
};

static void of_get_value_by_name(struct platform_device *pdev, const char *name,
				 int *ret, unsigned int defval)
{
	if (of_property_read_u32(pdev->dev.of_node, name, ret) != 0) {
		CAR_REVERSE_DEV_ERR(&pdev->dev, "missing property '%s', default value %d\n",
			name, defval);
		*ret = defval;
	}
}

static void of_get_gpio_by_name(struct platform_device *pdev, const char *name,
				int *ret)
{
	int gpio_index;
	enum of_gpio_flags config;

	gpio_index = of_get_named_gpio_flags(pdev->dev.of_node, name, 0, &config);
	if (!gpio_is_valid(gpio_index)) {
		CAR_REVERSE_DEV_ERR(&pdev->dev, "failed to get gpio '%s'\n", name);
		*ret = 0;
		return;
	}
	*ret = gpio_index;
}

static void parse_config(struct platform_device *pdev,
			 struct car_reverse_private_data *priv)
{
	of_get_value_by_name(pdev, "video_channel", &priv->config.video_channel, 0);
	of_get_value_by_name(pdev, "screen_width", &priv->config.screen_width, 0);
	of_get_value_by_name(pdev, "screen_height", &priv->config.screen_height, 0);
	of_get_value_by_name(pdev, "video_source_width", &priv->config.src_width, 0);
	of_get_value_by_name(pdev, "video_source_height", &priv->config.src_height, 0);
	of_get_value_by_name(pdev, "screen_size_adaptive", &priv->config.screen_size_adaptive, 0);
	of_get_value_by_name(pdev, "discard_frame", &priv->config.discard_frame, 0);
	of_get_value_by_name(pdev, "video_source", &priv->config.input_src, 0);
	if (priv->config.input_src == 0) // vin not use src size adaptive
		priv->config.src_size_adaptive = 0;
	else
		of_get_value_by_name(pdev, "src_size_adaptive", &priv->config.src_size_adaptive, 0);

	of_get_value_by_name(pdev, "rotation", &priv->config.rotation, 0);
	of_get_value_by_name(pdev, "auxiliary_line_type", &priv->config.aux_line_type, 0);
	of_get_value_by_name(pdev, "di_used", &priv->config.di_used, 0);
	of_get_value_by_name(pdev, "g2d_used", &priv->config.g2d_used, 0);
	of_get_gpio_by_name(pdev, "reverse_int_pin", &priv->reverse_gpio);
	of_get_value_by_name(pdev, "overview_type", &priv->config.oview_type, 0);
	of_get_value_by_name(pdev, "format_type", &priv->config.format, 0);
	of_get_value_by_name(pdev, "aux_angle", &priv->config.aux_angle, 0);
	of_get_value_by_name(pdev, "aux_lr", &priv->config.aux_lr, 0);

	switch (priv->config.format) {
	case 0:
		priv->config.format = V4L2_PIX_FMT_NV21;
		break;
	case 1:
		priv->config.format = V4L2_PIX_FMT_NV12;
		break;
	case 2:
		priv->config.format = V4L2_PIX_FMT_NV61;
		break;
	case 3:
		priv->config.format = V4L2_PIX_FMT_YUV420;
		break;
	case 4:
		priv->config.format = V4L2_PIX_FMT_YUV422P;
		break;
	default:
		priv->config.format = V4L2_PIX_FMT_NV21;
	}

	if (priv->config.oview_type == 1)
		priv->config.oview_type = 0;

	memcpy(&priv->def_config, &priv->config, sizeof(struct preview_params));

	priv->debug.force_status = CAR_REVERSE_NULL;
	priv->debug.user_status = CAR_REVERSE_NULL;

}

/* car reverse callback, called after video source finsh buffer handle */
void car_reverse_display_update(int video_id)
{
	spin_lock(&car_reverse->thread_lock);
	if (car_reverse->thread_mask == THREAD_NEED_STOP) {
		spin_unlock(&car_reverse->thread_lock);
		CAR_REVERSE_ERR("car reverse is not ready!\n");
		return;
	}
	if (car_reverse->display_update_task)
		wake_up_process(car_reverse->display_update_task);

	car_reverse->thread_mask = THREAD_RUN;
	spin_unlock(&car_reverse->thread_lock);
}

static void car_reverse_single_display(struct buffer_node **update_frame, \
				      struct buffer_node **enhanced_frame)
{
	struct buffer_node *aux_node = car_reverse->buffer_auxiliary_line;
	struct buffer_node *update_aux_frame = NULL;
	struct preview_params *config = &car_reverse->config;
	int aux_angle = car_reverse->config.aux_angle;
	int aux_lr = car_reverse->config.aux_lr;
	int ret = 0;

	if (car_reverse->config.aux_line_type) {
		if (car_reverse->config.aux_line_type == 1) {
			static_aux_line_update(aux_node, aux_angle, aux_lr,
						       AUXLAYER_WIDTH, AUXLAYER_HEIGHT);
			update_aux_frame = aux_node;
		} else if (car_reverse->config.aux_line_type == 2) {
			dynamic_aux_line_update(aux_node, aux_angle, aux_lr,
						AUXLAYER_WIDTH, AUXLAYER_HEIGHT);
			update_aux_frame = aux_node;
		} else
			CAR_REVERSE_WRN("Unknown aux_line_type!\n");

		if (car_reverse->config.g2d_used && config->rotation) {
			ret = preview_image_rotate(aux_node, &car_reverse->buffer_g2d[G2D_BUF_AUXLINE],
				AUXLAYER_WIDTH, AUXLAYER_HEIGHT, config->rotation, V4L2_PIX_FMT_BGR24);
			if (ret < 0)
				CAR_REVERSE_ERR("preview_image_rotate fail, use origin frame!\n");
			else
				update_aux_frame = car_reverse->buffer_g2d[G2D_BUF_AUXLINE];
		}

		aux_line_layer_config_update(update_aux_frame);
	}

	if (car_reverse->config.di_used) {
		ret = preview_image_enhance(*update_frame, &car_reverse->buffer_di[0], \
				config->src_width, config->src_height, config->format);
		if (ret < 0)
			CAR_REVERSE_ERR("preview_image_enhance fail, use origin frame!\n");
		else {
			*enhanced_frame = car_reverse->buffer_di[0];
			*update_frame = car_reverse->buffer_di[0];
		}
	}

	if (car_reverse->config.g2d_used) {
		if (config->rotation) {
			ret = preview_image_rotate(*enhanced_frame, \
					 &car_reverse->buffer_g2d[G2D_BUF_ROTATE], \
				config->src_width, config->src_height, config->rotation, config->format);
			if (ret < 0)
				CAR_REVERSE_ERR("preview_image_rotate fail, use origin frame!\n");
			else
				*update_frame = car_reverse->buffer_g2d[G2D_BUF_ROTATE];

		}
	}

	preview_layer_config_update(*update_frame);
	preview_display();
}

static void car_reverse_muti_display(struct buffer_node **update_frameOv, int overview_type)
{
	preview_Ov_layer_config_update(update_frameOv, overview_type);
	preview_display();
}

static int display_update_thread(void *data)
{
	int i = 0;
	int overview_type = car_reverse->config.oview_type;
	struct buffer_node *node = NULL;
	struct buffer_node *update_frameOv[4];
	struct buffer_node *update_frame = NULL;
	struct buffer_node *enhanced_frame = NULL;
	struct preview_params *config = &car_reverse->config;

	for (i = 0; i < CAR_MAX_CH; i++)
		update_frameOv[i] = NULL;

	while (!kthread_should_stop()) {
		if (overview_type) {
			car_reverse->first_buf_unready = false;
			/* dequeue ready buffer from each channel's video source */
			mutex_lock(&car_reverse->preview_lock);
			for (i = 0; (i < CAR_MAX_CH) && (i < overview_type); i++) {
				node = video_source_dequeue_buffer(i + config->video_channel);
				if (IS_ERR_OR_NULL(node)) {
					CAR_DRIVER_DBG("video source_%d buffer is not ready!\n", \
						       i + config->video_channel);
				} else {
					update_frameOv[i] = node;
					video_source_queue_buffer(node, i + config->video_channel);
				}

				if (update_frameOv[i] == NULL) {
					car_reverse->first_buf_unready = true;
				}
			}
			mutex_unlock(&car_reverse->preview_lock);

			if (car_reverse->discard_frame != 0) {
				car_reverse->discard_frame--;
			} else {
				car_reverse->discard_frame = 0;
			}

			/* diplay if discard_frame is skiped */
			if (car_reverse->discard_frame == 0) {
				/* if first frame is not ready, skip it */
				if (car_reverse->first_buf_unready)
					continue;
				car_reverse_muti_display(update_frameOv, overview_type);

			}
		} else {
			/* dequeue ready buffer from video source */
			mutex_lock(&car_reverse->preview_lock);
			node = video_source_dequeue_buffer(config->video_channel);
			if (IS_ERR_OR_NULL(node)) {
				CAR_DRIVER_DBG("video source_%d buffer is not ready!\n", \
					       config->video_channel);
				/* if first frame is not ready, skip it */
				if ((update_frame->dmabuf_fd == 0) || (enhanced_frame->dmabuf_fd == 0))
					continue;
			} else {
				update_frame = node;
				enhanced_frame = node;
				video_source_queue_buffer(node, config->video_channel);
			}
			mutex_unlock(&car_reverse->preview_lock);


			if (car_reverse->discard_frame != 0) {
				car_reverse->discard_frame--;
			} else {
				car_reverse->discard_frame = 0;
			}
			if (car_reverse->discard_frame == 0) {
				car_reverse_single_display(&update_frame, &enhanced_frame);
			}
		}

		if (kthread_should_stop()) {
			break;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return 0;
}

static int car_reverse_buffer_alloc(int alloc_path)
{
	int overview_type = car_reverse->config.oview_type;
	unsigned int node_registered = 0;
	int heigth, width;
	int i = 0, ret = 0;

	heigth = car_reverse->config.src_height; //MAX_BUFFER_SIZE_HEIGHT;
	width = car_reverse->config.src_width; //MAX_BUFFER_SIZE_WIDTH;

	mutex_lock(&car_reverse->alloc_lock);
	if (overview_type) {
		/* alloc buffer pool for muti-preview diaplay */
		for (i = 0; (i < CAR_MAX_CH) && (i < overview_type); i++) {
			car_reverse->bufferOv_pool[i] = alloc_buffer_pool(car_reverse->dev, \
					SWAP_BUFFER_CNT, width * heigth * 2, alloc_path);
			if (!car_reverse->bufferOv_pool[i]) {
				CAR_REVERSE_ERR("alloc buffer memory bufferOv_pool failed\n");
				ret = -1;
				goto free_buffer;
			}
			node_registered++;
		}
	} else {
		/* alloc buffer pool for singal preview diaplay */
		car_reverse->buffer_pool = alloc_buffer_pool(car_reverse->dev, SWAP_BUFFER_CNT,
							     width * heigth * 2, alloc_path);
		if (!car_reverse->buffer_pool) {
			ret = -1;
			mutex_unlock(&car_reverse->alloc_lock);
			goto free_buffer;
		}
	}

	/* alloc buffer for uxiliary_line */
	if (car_reverse->buffer_auxiliary_line == 0 && car_reverse->config.aux_line_type) {
		car_reverse->buffer_auxiliary_line = \
			buffer_node_alloc(car_reverse->dev, AUXLAYER_WIDTH * AUXLAYER_HEIGHT * 4, alloc_path);

		if (!car_reverse->buffer_auxiliary_line) {
			CAR_REVERSE_ERR("alloc buffer_auxiliary_line  memory failed\n");
			ret = -1;
			goto free_buffer;
		}
	}

	/* alloc buffer for di writeback */
	if (car_reverse->buffer_di[0] == 0 && car_reverse->config.di_used) {
		car_reverse->buffer_di[0] = \
			buffer_node_alloc(car_reverse->dev, width * heigth * 2, alloc_path);

		if (!car_reverse->buffer_di[0]) {
			CAR_REVERSE_ERR("alloc buffer_di[0] memory failed\n");
			ret = -1;
			goto free_buffer;
		}
	}

	if (car_reverse->buffer_di[1] == 0 && car_reverse->config.di_used) {
		car_reverse->buffer_di[1] = \
			buffer_node_alloc(car_reverse->dev, width * heigth * 2, alloc_path);

		if (!car_reverse->buffer_di[1]) {
			CAR_REVERSE_ERR("alloc buffer_di[1] memory failed\n");
			ret = -1;
			goto free_buffer;
		}
	}

	/* alloc buffer node for g2d writeback */
	if (car_reverse->buffer_g2d[G2D_BUF_ROTATE] == 0 && car_reverse->config.g2d_used) {
		car_reverse->buffer_g2d[G2D_BUF_ROTATE] = \
			buffer_node_alloc(car_reverse->dev, width * heigth * 8, alloc_path);

		if (!car_reverse->buffer_g2d[G2D_BUF_ROTATE]) {
			CAR_REVERSE_ERR("alloc buffer_g2d[G2D_BUF_ROTATE] memory failed\n");
			ret = -1;
			goto free_buffer;
		}
	}

	if (car_reverse->buffer_g2d[G2D_BUF_SCALER] == 0 && car_reverse->config.g2d_used) {
		car_reverse->buffer_g2d[G2D_BUF_SCALER] = \
			buffer_node_alloc(car_reverse->dev, width * heigth * 8, alloc_path);

		if (!car_reverse->buffer_g2d[G2D_BUF_SCALER]) {
			CAR_REVERSE_ERR("alloc buffer_g2d[G2D_BUF_SCALER] memory failed\n");
			ret = -1;
			goto free_buffer;
		}
	}

	if (car_reverse->buffer_g2d[G2D_BUF_AUXLINE] == 0 && car_reverse->config.g2d_used) {
		car_reverse->buffer_g2d[G2D_BUF_AUXLINE] = \
			buffer_node_alloc(car_reverse->dev, AUXLAYER_WIDTH * AUXLAYER_HEIGHT * 4, alloc_path);

		if (!car_reverse->buffer_g2d[G2D_BUF_AUXLINE]) {
			CAR_REVERSE_ERR("alloc buffer_g2d[G2D_BUF_AUXLINE] memory failed\n");
			ret = -1;
			goto free_buffer;
		}
	}

	mutex_unlock(&car_reverse->alloc_lock);
	return 0;

free_buffer:
	if (car_reverse->buffer_pool) {
		free_buffer_pool(car_reverse->buffer_pool);
		car_reverse->buffer_pool = NULL;
	}

	for (i = 0; i < node_registered++; i++) {
		if (car_reverse->bufferOv_pool[i]) {
			free_buffer_pool(car_reverse->bufferOv_pool[i]);
			car_reverse->bufferOv_pool[i] = NULL;
		}
	}

	if (car_reverse->buffer_auxiliary_line) {
		free_buffer_node(car_reverse->buffer_auxiliary_line);
		car_reverse->buffer_auxiliary_line = NULL;
	}

	if (car_reverse->buffer_di[0]) {
		free_buffer_node(car_reverse->buffer_di[0]);
		car_reverse->buffer_di[0] = NULL;
	}

	if (car_reverse->buffer_di[1]) {
		free_buffer_node(car_reverse->buffer_di[1]);
		car_reverse->buffer_di[1] = NULL;
	}

	if (car_reverse->buffer_g2d[G2D_BUF_ROTATE]) {
		free_buffer_node(car_reverse->buffer_g2d[G2D_BUF_ROTATE]);
		car_reverse->buffer_g2d[G2D_BUF_ROTATE] = NULL;
	}

	if (car_reverse->buffer_g2d[G2D_BUF_SCALER]) {
		free_buffer_node(car_reverse->buffer_g2d[G2D_BUF_SCALER]);
		car_reverse->buffer_g2d[G2D_BUF_SCALER] = NULL;
	}

	if (car_reverse->buffer_g2d[G2D_BUF_AUXLINE]) {
		free_buffer_node(car_reverse->buffer_g2d[G2D_BUF_AUXLINE]);
		car_reverse->buffer_g2d[G2D_BUF_AUXLINE] = NULL;
	}
	mutex_unlock(&car_reverse->alloc_lock);

	return -1;

}

static int car_reverse_buffer_free(void)
{
	int i = 0;
	int overview_type = car_reverse->config.oview_type;

	mutex_lock(&car_reverse->free_lock);
	if (car_reverse->buffer_pool) {
		free_buffer_pool(car_reverse->buffer_pool);
		car_reverse->buffer_pool = NULL;
	}

	for (i = 0; (i < CAR_MAX_CH) && (i < overview_type); i++) {
		free_buffer_pool(car_reverse->bufferOv_pool[i]);
		car_reverse->bufferOv_pool[i] = NULL;
	}

	if (car_reverse->buffer_auxiliary_line) {
		free_buffer_node(car_reverse->buffer_auxiliary_line);
		car_reverse->buffer_auxiliary_line = NULL;
	}

	if (car_reverse->buffer_di[0]) {
		free_buffer_node(car_reverse->buffer_di[0]);
		car_reverse->buffer_di[0] = NULL;
	}

	if (car_reverse->buffer_di[1]) {
		free_buffer_node(car_reverse->buffer_di[1]);
		car_reverse->buffer_di[1] = NULL;
	}

	if (car_reverse->buffer_g2d[G2D_BUF_ROTATE]) {
		free_buffer_node(car_reverse->buffer_g2d[G2D_BUF_ROTATE]);
		car_reverse->buffer_g2d[G2D_BUF_ROTATE] = NULL;
	}

	if (car_reverse->buffer_g2d[G2D_BUF_SCALER]) {
		free_buffer_node(car_reverse->buffer_g2d[G2D_BUF_SCALER]);
		car_reverse->buffer_g2d[G2D_BUF_SCALER] = NULL;
	}

	if (car_reverse->buffer_g2d[G2D_BUF_AUXLINE]) {
		free_buffer_node(car_reverse->buffer_g2d[G2D_BUF_AUXLINE]);
		car_reverse->buffer_g2d[G2D_BUF_AUXLINE] = NULL;
	}
	mutex_unlock(&car_reverse->free_lock);

	return 0;
}

static int car_reverse_get_status(void)
{
	return car_reverse->status;
}


static int car_reverse_preview_start(void)
{
	int ret = 0;
	int i, n;
	struct buffer_node *node;
	struct buffer_pool *bp ;
	int overview_type = car_reverse->config.oview_type;
	int alloc_path;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		CAR_REVERSE_WRN("car reverse is already start!\n");
		return 0;
	}

	car_reverse->display_update_task =
		kthread_create(display_update_thread, NULL, "car-reverse-preview");
	if (!car_reverse->display_update_task) {
		CAR_REVERSE_ERR("failed to create kthread\n");
		return -1;
	}

	ret = video_source_select(car_reverse->config.input_src);
	if (ret < 0) {
		CAR_REVERSE_ERR("failed to open video source\n");
		return -1;
	}

	if (car_reverse->config.input_src == 2)
		alloc_path = 0;
	else
		alloc_path = 1;

	/* alloc_path: 0-cma_alloc  1-iommu_alloc */
	ret = car_reverse_buffer_alloc(alloc_path);
	if (ret < 0)
		goto destory_thread;

	if (car_reverse->config.g2d_used)
		preview_rotator_init(&car_reverse->config);

	if (car_reverse->config.di_used)
		preview_enhancer_init(&car_reverse->config);

	if (car_reverse->config.aux_line_type)
		auxiliary_line_init();

	preview_output_init(&car_reverse->config);

	if (overview_type) {
		for (n = 0; (n < CAR_MAX_CH) && (n < overview_type); n++) {
			bp = car_reverse->bufferOv_pool[n];

			ret = video_source_connect(&car_reverse->config, \
					 n + car_reverse->config.video_channel);
			if (ret != 0) {
				CAR_REVERSE_ERR("can't connect to video source!\n");
				goto free_buffer;
			}

			for (i = 0; i < SWAP_BUFFER_CNT; i++) {
				node = buffer_pool_dequeue_buffer(bp);
				video_source_queue_buffer(node, \
					n + car_reverse->config.video_channel);
			}
		}


		for (n = 0; (n < CAR_MAX_CH) && (n < overview_type); n++) {
			video_source_streamon(n + car_reverse->config.video_channel);
		}

	} else {
		bp = car_reverse->buffer_pool;

		ret = video_source_connect(&car_reverse->config,
						car_reverse->config.video_channel);
		if (ret != 0) {
			CAR_REVERSE_ERR("can't connect to video source!\n");
			goto free_buffer;
		}

		for (i = 0; i < SWAP_BUFFER_CNT; i++) {
			node = buffer_pool_dequeue_buffer(bp);
			video_source_queue_buffer(node, car_reverse->config.video_channel);
		}

		video_source_streamon(car_reverse->config.video_channel);
	}


	car_reverse->status = CAR_REVERSE_START;
	car_reverse->discard_frame = car_reverse->config.discard_frame;

	spin_lock(&car_reverse->thread_lock);
	car_reverse->thread_mask = 0;
	spin_unlock(&car_reverse->thread_lock);

	return 0;

free_buffer:
	car_reverse_buffer_free();
destory_thread:
	kthread_stop(car_reverse->display_update_task);
	car_reverse->display_update_task = NULL;
	return -1;
}

static int car_reverse_preview_stop(void)
{
	struct buffer_node *node = NULL;
	struct buffer_pool *bp;

	int i;
	int overview_type = car_reverse->config.oview_type;

	if (car_reverse_get_status() == CAR_REVERSE_STOP) {
		CAR_REVERSE_WRN("car reverse is already stop!\n");
		return 0;
	}

	car_reverse->status = CAR_REVERSE_STOP;
	if (overview_type) {
		for (i = 0; (i < CAR_MAX_CH) && (i < overview_type); i++)
			video_source_streamoff(i + car_reverse->config.video_channel);
	} else {
			video_source_streamoff(car_reverse->config.video_channel);
	}

	spin_lock(&car_reverse->thread_lock);
	car_reverse->thread_mask = THREAD_NEED_STOP;
	spin_unlock(&car_reverse->thread_lock);

	while (car_reverse->thread_mask == THREAD_RUN)
		msleep(1);
	if (car_reverse->display_update_task) {
		kthread_stop(car_reverse->display_update_task);
		car_reverse->display_update_task = NULL;
	}
	preview_output_exit(&car_reverse->config);

	if (car_reverse->config.g2d_used) {
		preview_rotator_exit();
	}

	if (car_reverse->config.di_used) {
		preview_enhancer_exit();
	}

	if (overview_type) {
		for (i = 0; (i < CAR_MAX_CH) && (i < overview_type); i++)
			video_source_force_reset_buffer(i + car_reverse->config.video_channel);

		for (i = 0; (i < CAR_MAX_CH) && (i < overview_type); i++) {
			bp = car_reverse->bufferOv_pool[i];
			while (1) {
				node = video_source_dequeue_buffer(i + \
						car_reverse->config.video_channel);
				if (!IS_ERR_OR_NULL(node)) {
					buffer_pool_queue_buffer(bp, node);
					CAR_DRIVER_DBG("video source buffer has return to buffer pool!\n");
				} else
					break;
			}
			video_source_disconnect(i + car_reverse->config.video_channel);
		}
	} else {
		video_source_force_reset_buffer(car_reverse->config.video_channel);
		while (1) {
			node = video_source_dequeue_buffer(car_reverse->config.video_channel);
			if (!IS_ERR_OR_NULL(node)) {
				buffer_pool_queue_buffer(car_reverse->buffer_pool, node);
				CAR_DRIVER_DBG("video source buffer has return to buffer pool!\n");
			} else
				break;
		}

		video_source_disconnect(car_reverse->config.video_channel);
	}

	car_reverse_buffer_free();

	video_source_release();

	spin_lock(&car_reverse->thread_lock);
	car_reverse->thread_mask = 0;
	spin_unlock(&car_reverse->thread_lock);

	return 0;
}

static int car_reverse_gpio_status(void)
{
	int value = 1;
	value = gpio_get_value(car_reverse->reverse_gpio);
	if (value == 0) {
		return CAR_REVERSE_START;
	} else {
		return CAR_REVERSE_STOP;
	}
}

/*
 *  current status | gpio status | next status
 *  ---------------+-------------+------------
 *		STOP	 |	STOP	 |	HOLD
 *  ---------------+-------------+------------
 *		STOP	 |	START	|	START
 *  ---------------+-------------+------------
 *		START	|	STOP	 |	STOP
 *  ---------------+-------------+------------
 *		START	|	START	|	HOLD
 *  ---------------+-------------+------------
 */
const int _transfer_table[3][3] = {
	[CAR_REVERSE_HOLD] = {0, 0, 0},
	[CAR_REVERSE_START] = {0, CAR_REVERSE_HOLD, CAR_REVERSE_STOP},
	[CAR_REVERSE_STOP] = {0, CAR_REVERSE_START, CAR_REVERSE_HOLD},
};

static int car_reverse_get_next_status(void)
{
	int next_status;
	int gpio_status = car_reverse_gpio_status();
	int curr_status = car_reverse->status;
	next_status = _transfer_table[curr_status][gpio_status];
	return next_status;
}

static void status_detect_func(struct work_struct *work)
{
	int ret;
	int status = car_reverse_get_next_status();

	if (car_reverse->standby)
		status = CAR_REVERSE_STOP;

	if (car_reverse->debug.force_status == CAR_REVERSE_STOP \
	    || car_reverse->debug.user_status == CAR_REVERSE_STOP) {
		status = CAR_REVERSE_STOP;
		CAR_DRIVER_DBG("force stop car reverse, return\n");
	}

	if (car_reverse->debug.force_status == CAR_REVERSE_START \
	    || car_reverse->debug.user_status == CAR_REVERSE_START) {
		status = CAR_REVERSE_START;
		CAR_DRIVER_DBG("force start car reverse, return\n");
	}

	switch (status) {
	case CAR_REVERSE_START:
		ret = car_reverse_preview_start();
		CAR_DRIVER_DBG("start car reverse, return %d\n", ret);
		break;
	case CAR_REVERSE_STOP:
		ret = car_reverse_preview_stop();
		CAR_DRIVER_DBG("stop car reverse, return %d\n", ret);
		break;
	case CAR_REVERSE_HOLD:
	default:
		break;
	}
	return;
}

static irqreturn_t reverse_irq_handle(int irqnum, void *data)
{
	CAR_DRIVER_DBG("reverse_irq_handle enter\n ");
	if (car_reverse->debug.force_status != CAR_REVERSE_HOLD \
	    || car_reverse->debug.user_status != CAR_REVERSE_HOLD)
		return IRQ_HANDLED;

	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	return IRQ_HANDLED;
}


static ssize_t force_start_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	switch (car_reverse->debug.force_status) {
	case CAR_REVERSE_STOP:
		return sprintf(buf, "force_start = stop\n");
	case CAR_REVERSE_START:
		return sprintf(buf, "force_start = start\n");
	default:
		return sprintf(buf, "force_start = null\n");
	}
}

static ssize_t force_start_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	if (!strncmp(buf, "stop", 4)) {
		if (car_reverse->debug.force_status == CAR_REVERSE_STOP)
			return count;
		car_reverse->debug.force_status = CAR_REVERSE_STOP;
	} else if (!strncmp(buf, "start", 5)) {
		if (car_reverse->debug.force_status == CAR_REVERSE_START)
			return count;
		car_reverse->debug.force_status = CAR_REVERSE_START;
	} else if (!strncmp(buf, "null", 4)) {
		car_reverse->debug.force_status = CAR_REVERSE_NULL;
		return count;
	} else {
		CAR_REVERSE_ERR("Invalid param!\n");
		return count;
	}
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	/*flush_workqueue(car_reverse->preview_workqueue);*/
	return count;
}


static ssize_t di_used_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, car_reverse->config.di_used ? "Enable\n" : "Disabled\n");
}

static ssize_t di_used_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 enable = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	enable = simple_strtoul(buf, NULL, 0);

	car_reverse->config.di_used = (enable > 0) ? 1 : 0;

	return count;
}


static ssize_t g2d_used_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, car_reverse->config.g2d_used ? "Enable\n" : "Disabled\n");
}

static ssize_t g2d_used_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 enable = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	enable = simple_strtoul(buf, NULL, 0);

	car_reverse->config.g2d_used = (enable > 0) ? 1 : 0;

	return count;
}

static ssize_t rotation_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	switch (car_reverse->config.rotation) {
	case 0x1:
		return sprintf(buf, "90\n");
	case 0x2:
		return sprintf(buf, "180\n");
	case 0x3:
		return sprintf(buf, "270\n");
	case 0xe:
		return sprintf(buf, "Horizon Mirror\n");
	case 0xf:
		return sprintf(buf, "Vertical Mirror\n");
	case 0:
	default:
		return sprintf(buf, "0\n");
	}
}

static ssize_t rotation_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 rotation = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	rotation = simple_strtoul(buf, NULL, 0);

	if ((rotation > 3) && (rotation != 0xe) && (rotation != 0xf))
		car_reverse->config.rotation = 0;
	else
		car_reverse->config.rotation = rotation;

	return count;
}

static ssize_t aux_line_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (car_reverse->config.aux_line_type == 1)
		return sprintf(buf, "Static\n");
	else if (car_reverse->config.aux_line_type == 2)
		return sprintf(buf, "Dynamic\n");
	else
		return sprintf(buf, "None\n");
}

static ssize_t aux_line_type_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 type = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	type = simple_strtoul(buf, NULL, 0);

	if (type > 2)
		CAR_REVERSE_ERR("Invalid param!\n");


	car_reverse->config.aux_line_type = type;

	return count;
}

static ssize_t over_view_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", car_reverse->config.oview_type);
}

static ssize_t over_view_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 over_view = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	over_view = simple_strtoul(buf, NULL, 0);

	if ((over_view > 4) || (over_view == 1))
		car_reverse->config.oview_type = 0;
	else
		car_reverse->config.oview_type = over_view;

	return count;
}

static ssize_t start_channel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", car_reverse->config.video_channel);
}

static ssize_t start_channel_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 channel = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	channel = simple_strtoul(buf, NULL, 0);

	car_reverse->config.video_channel = channel;

	return count;
}

static ssize_t discard_frame_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", car_reverse->config.discard_frame);
}

static ssize_t discard_frame_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 discard = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	discard = simple_strtoul(buf, NULL, 0);

	car_reverse->config.discard_frame = discard;

	return count;
}

static ssize_t video_source_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (car_reverse->config.input_src == 1)
		return sprintf(buf, "TVD\n");
	else if (car_reverse->config.input_src == 2)
		return sprintf(buf, "Virtual\n");
	else
		return sprintf(buf, "VIN\n");
}

static ssize_t video_source_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 value = 0;
	u32 ret = 0;


	value = simple_strtoul(buf, NULL, 0);

	if (value > 2) {
		CAR_REVERSE_ERR("Invalid param!\n");
		return count;
	} else {
		if (car_reverse_get_status() == CAR_REVERSE_START) {
			ret = car_reverse_preview_stop();
			if (ret < 0) {
				CAR_REVERSE_ERR("car reverse fail to stop!\n");
				return count;
			}
			flush_workqueue(car_reverse->preview_workqueue);
		}

		car_reverse->config.input_src = value;
	}

	return count;
}


static ssize_t format_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	switch (car_reverse->config.format) {
	case V4L2_PIX_FMT_NV12:
		return sprintf(buf, "NV12\n");
	case V4L2_PIX_FMT_NV61:
		return sprintf(buf, "NV61\n");
	case V4L2_PIX_FMT_YUV420:
		return sprintf(buf, "YUV420\n");
	case V4L2_PIX_FMT_YUV422P:
		return sprintf(buf, "YUV422P\n");
	case V4L2_PIX_FMT_NV21:
	default:
		return sprintf(buf, "NV21\n");
	}
}

static ssize_t format_type_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 value = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	value = simple_strtoul(buf, NULL, 0);
	switch (value) {
	case 0:
		car_reverse->config.format = V4L2_PIX_FMT_NV21;
		break;
	case 1:
		car_reverse->config.format = V4L2_PIX_FMT_NV12;
		break;
	case 2:
		car_reverse->config.format = V4L2_PIX_FMT_NV61;
		break;
	case 3:
		car_reverse->config.format = V4L2_PIX_FMT_YUV420;
		break;
	case 4:
		car_reverse->config.format = V4L2_PIX_FMT_YUV422P;
		break;
	default:
		car_reverse->config.format = V4L2_PIX_FMT_NV21;
	}

	return count;
}

static ssize_t aux_angle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", car_reverse->config.aux_angle);
}

static ssize_t aux_angle_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 value = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	value = simple_strtoul(buf, NULL, 0);

	car_reverse->config.aux_angle = value;

	return count;
}

static ssize_t aux_lr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", car_reverse->config.aux_lr);
}

static ssize_t aux_lr_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 value = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	value = simple_strtoul(buf, NULL, 0);

	car_reverse->config.aux_lr = value;

	return count;
}

static ssize_t reset_default_config_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 ret = 0;

	if (!strncmp(buf, "1", 1)) {
		if (car_reverse_get_status() == CAR_REVERSE_START) {
			ret = car_reverse_preview_stop();
			if (ret < 0) {
				CAR_REVERSE_ERR("car reverse fail to stop!\n");
				return count;
			}
			flush_workqueue(car_reverse->preview_workqueue);
		}

		memcpy(&car_reverse->config, &car_reverse->def_config, sizeof(struct preview_params));
	} else
		CAR_REVERSE_ERR("Invalid param!\n");

	return count;
}

static ssize_t loglevel_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	reverse_loglevel_debug = simple_strtoul(buf, NULL, 0);

	return count;
}

static ssize_t loglevel_debug_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	u32 count = 0;
	if (!reverse_loglevel_debug) {
		count += sprintf(buf + count, \
			"0:NONE  1:DRVIVER  2:BUFFER_POOL  4:VIDEO_SOURCE  8:PREVIEW_DISPLAY");
		count += sprintf(buf + count, "  16:G2D/DI  32:AUXILIARY_LINE\n");
	} else
		count += sprintf(buf + count, "loglevel_debug = %d\n", reverse_loglevel_debug);

	return count;
}

static ssize_t src_size_adaptive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, car_reverse->config.src_size_adaptive ? "Enable\n" : "Disabled\n");
}

static ssize_t src_size_adaptive_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 value = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	value = simple_strtoul(buf, NULL, 0);

	/* vin not use src size adaptive */
	if (car_reverse->config.input_src == 0)
		car_reverse->config.src_size_adaptive = 0;
	else
		car_reverse->config.src_size_adaptive = (value > 0) ? 1 : 0;

	return count;
}

static ssize_t screen_size_adaptive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, car_reverse->config.screen_size_adaptive ? "Enable\n" : "Disabled\n");
}

static ssize_t screen_size_adaptive_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 value = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	value = simple_strtoul(buf, NULL, 0);

	car_reverse->config.screen_size_adaptive = (value > 0) ? 1 : 0;

	return count;
}

static ssize_t screen_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d X %d\n", car_reverse->config.screen_width, \
		       car_reverse->config.screen_height);
}

static ssize_t screen_size_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u8 *separator = NULL;
	u32 width = 0;
	u32 height = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		CAR_REVERSE_ERR("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		width = simple_strtoul(buf, NULL, 0);
		height = simple_strtoul(separator + 1, NULL, 0);

		car_reverse->config.screen_width = width;
		car_reverse->config.screen_height = height;
	}

	return count;
}

static ssize_t src_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d X %d\n", car_reverse->config.src_width, \
		       car_reverse->config.src_height);
}

static ssize_t src_size_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u8 *separator = NULL;
	u32 width = 0;
	u32 height = 0;
	u32 ret = 0;

	if (car_reverse_get_status() == CAR_REVERSE_START) {
		ret = car_reverse_preview_stop();
		if (ret < 0) {
			CAR_REVERSE_ERR("car reverse fail to stop!\n");
			return count;
		}
		flush_workqueue(car_reverse->preview_workqueue);
	}

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		CAR_REVERSE_ERR("%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		width = simple_strtoul(buf, NULL, 0);
		height = simple_strtoul(separator + 1, NULL, 0);

		car_reverse->config.src_width = width;
		car_reverse->config.src_height = height;
	}

	return count;
}

static ssize_t config_summary_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 count = 0;

	count += sprintf(buf + count, "[Car Reverse Config Summary]\n");

	count += sprintf(buf + count, "Video_Source: ");
	if (car_reverse->config.input_src == 1)
		count += sprintf(buf + count, "TVD\n");
	else if (car_reverse->config.input_src == 2)
		count += sprintf(buf + count, "Virtual\n");
	else
		count += sprintf(buf + count, "VIN\n");

	count += sprintf(buf + count, "Over_View: %d\n", car_reverse->config.oview_type);

	switch (car_reverse->config.format) {
	case 1:
		count += sprintf(buf + count, "Format: NV12\n");
		break;
	case 2:
		count += sprintf(buf + count, "Format: NV61\n");
		break;
	case 3:
		count += sprintf(buf + count, "Format: YUV420\n");
		break;
	case 4:
		count += sprintf(buf + count, "Format: YUV422P\n");
		break;
	case 0:
	default:
		count += sprintf(buf + count, "Format: NV21\n");
	}

	count += sprintf(buf + count, "Video_Channel: %d\n", \
			 car_reverse->config.video_channel);

	count += sprintf(buf + count, "Discard_Frame: %d\n", \
			 car_reverse->config.discard_frame);

	count += sprintf(buf + count, car_reverse->config.screen_size_adaptive ? \
			 "Screen_Size_Adaptive: Enable\n" : "Screen_Size_Adaptive: Disabled\n");

	count += sprintf(buf + count, car_reverse->config.src_size_adaptive ? \
			 "Src_Size_Adaptive: Enable\n" : "Src_Size_Adaptive: Disabled\n");

	count += sprintf(buf + count, "Scrren_Size: %d X %d\n", \
		car_reverse->config.screen_width, car_reverse->config.screen_height);
	count += sprintf(buf + count, "Src_Size: %d X %d\n", \
		car_reverse->config.src_width, car_reverse->config.src_height);

	count += sprintf(buf + count, "G2D_Used: %s   ", \
			 car_reverse->config.g2d_used ? "Enable\n" : "Disabled\n");
	switch (car_reverse->config.rotation) {
	case 0x1:
		count += sprintf(buf + count, "Rotation: 90\n");
		break;
	case 0x2:
		count += sprintf(buf + count, "Rotation: 180\n");
		break;
	case 0x3:
		count += sprintf(buf + count, "Rotation: 270\n");
		break;
	case 0xe:
		count += sprintf(buf + count, "Rotation: Horizon Mirror\n");
		break;
	case 0xf:
		count += sprintf(buf + count, "Rotation: Vertical Mirror\n");
		break;
	case 0:
	default:
		count += sprintf(buf + count, "Rotation: 0\n");
	}

	count += sprintf(buf + count, "DI_Used: %s", \
			 car_reverse->config.di_used ? "Enable\n" : "Disabled\n");

	if (car_reverse->config.aux_line_type == 1)
		count += sprintf(buf + count, "Aux_Line_Type: Static   ");
	else if (car_reverse->config.aux_line_type == 2)
		count += sprintf(buf + count, "Aux_Line_Type: Dynamic  ");
	else
		count += sprintf(buf + count, "Aux_Line_Type: None  ");
	count += sprintf(buf + count, "Aux_Angle: %d  ", car_reverse->config.aux_angle);
	count += sprintf(buf + count, "Left/Right: %s  ", \
			 (car_reverse->config.aux_lr > 0) ? "Left\n" : "Right\n");

	return count;
}

static DEVICE_ATTR(force_start, 0660, force_start_show, force_start_store);
static DEVICE_ATTR(di_used, 0660, di_used_show, di_used_store);
static DEVICE_ATTR(g2d_used, 0660, g2d_used_show, g2d_used_store);
static DEVICE_ATTR(rotation, 0660, rotation_show, rotation_store);
static DEVICE_ATTR(aux_line_type, 0660, aux_line_type_show, aux_line_type_store);
static DEVICE_ATTR(over_view, 0660, over_view_show, over_view_store);
static DEVICE_ATTR(start_channel, 0660, start_channel_show, start_channel_store);
static DEVICE_ATTR(discard_frame, 0660, discard_frame_show, discard_frame_store);
static DEVICE_ATTR(video_source, 0660, video_source_show, video_source_store);
static DEVICE_ATTR(format_type, 0660, format_type_show, format_type_store);
static DEVICE_ATTR(aux_angle, 0660, aux_angle_show, aux_angle_store);
static DEVICE_ATTR(aux_lr, 0660, aux_lr_show, aux_lr_store);
static DEVICE_ATTR(reset_default_config, 0660, NULL, reset_default_config_store);
static DEVICE_ATTR(reverse_loglevel_debug, 0660, loglevel_debug_show, loglevel_debug_store);
static DEVICE_ATTR(src_size_adaptive, 0660, src_size_adaptive_show, src_size_adaptive_store);
static DEVICE_ATTR(screen_size_adaptive, 0660, screen_size_adaptive_show, screen_size_adaptive_store);
static DEVICE_ATTR(screen_size, 0660, screen_size_show, screen_size_store);
static DEVICE_ATTR(src_size, 0660, src_size_show, src_size_store);
static DEVICE_ATTR(config_summary, 0660, config_summary_show, NULL);


static struct attribute *car_reverse_attributes[] = {
	&dev_attr_force_start.attr,
	&dev_attr_di_used.attr,
	&dev_attr_g2d_used.attr,
	&dev_attr_rotation.attr,
	&dev_attr_aux_line_type.attr,
	&dev_attr_over_view.attr,
	&dev_attr_start_channel.attr,
	&dev_attr_discard_frame.attr,
	&dev_attr_video_source.attr,
	&dev_attr_format_type.attr,
	&dev_attr_aux_angle.attr,
	&dev_attr_aux_lr.attr,
	&dev_attr_reset_default_config.attr,
	&dev_attr_reverse_loglevel_debug.attr,
	&dev_attr_src_size_adaptive.attr,
	&dev_attr_screen_size_adaptive.attr,
	&dev_attr_screen_size.attr,
	&dev_attr_src_size.attr,
	&dev_attr_config_summary.attr,
	NULL
};

static struct attribute_group car_reverse_attribute_group = {
	.name = "debug",
	.attrs = car_reverse_attributes
};


static int car_reverse_probe(struct platform_device *pdev)
{
	int ret = 0;
	long reverse_pin_irqnum;

	if (!pdev->dev.of_node) {
		CAR_REVERSE_DEV_ERR(&pdev->dev, "of_node is missing\n");
		ret = -EINVAL;
		goto _err_out;
	}
	car_reverse = devm_kzalloc(
		&pdev->dev, sizeof(struct car_reverse_private_data), GFP_KERNEL);
	if (!car_reverse) {
		CAR_REVERSE_DEV_ERR(&pdev->dev, "kzalloc for private data failed\n");
		ret = -ENOMEM;
		goto _err_out;
	}

	parse_config(pdev, car_reverse);
	car_reverse->pdev = pdev;
	car_reverse->dev = &pdev->dev;
	platform_set_drvdata(pdev, car_reverse);

	spin_lock_init(&car_reverse->thread_lock);
	mutex_init(&car_reverse->alloc_lock);
	mutex_init(&car_reverse->free_lock);
	mutex_init(&car_reverse->preview_lock);

	/* Create and add a character device */
	alloc_chrdev_region(&devid, 0, 1, "car_reverse");/* corely for device number */
	car_reverse_cdev = cdev_alloc();
	cdev_init(car_reverse_cdev, &car_reverse_fops);
	car_reverse_cdev->owner = THIS_MODULE;

	ret = cdev_add(car_reverse_cdev, devid, 1);/* /proc/device/car_reverse */
	if (ret) {
		CAR_REVERSE_ERR("Error: car_reverse cdev_add fail.\n");
		return -1;
	}

	/* Create a path: sys/class/car_reverse */
	car_reverse_class = class_create(THIS_MODULE, "car_reverse");
	if (IS_ERR(car_reverse_class)) {
		CAR_REVERSE_ERR("Error:car_reverse class_create fail\n");
		return -1;
	}

	/* Create a path "sys/class/car_reverse/car_reverse" */
	cardev = device_create(car_reverse_class, NULL, devid, NULL, "car_reverse");
	ret = sysfs_create_group(&cardev->kobj, &car_reverse_attribute_group);

	car_reverse->status = CAR_REVERSE_STOP;
	reverse_pin_irqnum = gpio_to_irq(car_reverse->reverse_gpio);
	if (IS_ERR_VALUE(reverse_pin_irqnum)) {
		CAR_REVERSE_DEV_ERR(&pdev->dev,
			"map gpio [%d] to virq failed, errno = %ld\n",
			car_reverse->reverse_gpio, reverse_pin_irqnum);
		ret = -EINVAL;
		goto _err_out;
	}
	car_reverse->preview_workqueue =
		create_singlethread_workqueue("car-reverse-wq");
	if (!car_reverse->preview_workqueue) {
		CAR_REVERSE_DEV_ERR(&pdev->dev, "create workqueue failed\n");
		ret = -EINVAL;
		goto _err_out;
	}
	INIT_WORK(&car_reverse->status_detect, status_detect_func);
	car_reverse->standby = 0;

	if (devm_request_irq(car_reverse->dev, reverse_pin_irqnum, reverse_irq_handle,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"car-reverse", pdev)) {
		CAR_REVERSE_DEV_ERR(&pdev->dev, "request irq %ld failed\n",
			reverse_pin_irqnum);
		ret = -EBUSY;
		goto _err_out;
	}

	car_reverse_video_source_init();

	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	CAR_REVERSE_DEV_INFO(&pdev->dev, "car reverse module probe ok\n");
	return 0;

_err_out:
	CAR_REVERSE_DEV_ERR(&pdev->dev, "car reverse module exit, errno %d!\n", ret);
	return ret;
}

static int car_reverse_remove(struct platform_device *pdev)
{
	struct car_reverse_private_data *priv = car_reverse;

	free_irq(gpio_to_irq(priv->reverse_gpio), pdev);
	cancel_work_sync(&priv->status_detect);
	if (priv->preview_workqueue != NULL) {
		flush_workqueue(priv->preview_workqueue);
		destroy_workqueue(priv->preview_workqueue);
		priv->preview_workqueue = NULL;
	}

	sysfs_remove_group(&cardev->kobj, &car_reverse_attribute_group);
	device_destroy(car_reverse_class, devid);

	class_destroy(car_reverse_class);
	cdev_del(car_reverse_cdev);
	CAR_REVERSE_DEV_INFO(&pdev->dev, "car reverse module exit\n");
	return 0;
}

static int car_reverse_suspend(struct device *dev)
{
	int ret;
	if (car_reverse->status == CAR_REVERSE_START) {
		car_reverse->standby = 1;

		CAR_DRIVER_DBG("car_reverse_suspend\n");
		ret = car_reverse_preview_stop();
		flush_workqueue(car_reverse->preview_workqueue);
	} else {
		car_reverse->standby = 0;
	}
	return 0;
}

static int car_reverse_resume(struct device *dev)
{
	if (car_reverse->standby) {
		car_reverse->standby = 0;
		queue_work(car_reverse->preview_workqueue,
			&car_reverse->status_detect);
	}
	return 0;
}

static const struct dev_pm_ops car_reverse_pm_ops = {
	.suspend = car_reverse_suspend,
	.resume = car_reverse_resume,
};

static const struct of_device_id car_reverse_dt_ids[] = {
	{.compatible = "allwinner,sunxi-car-reverse"}, {},
};

static struct platform_driver car_reverse_driver = {
	.probe = car_reverse_probe,
	.remove = car_reverse_remove,
	.driver = {
		.name = MODULE_NAME,
		.pm = &car_reverse_pm_ops,
		.of_match_table = car_reverse_dt_ids,
	},
};

static int __init car_reverse_module_init(void)
{
	int ret;

	ret = platform_driver_register(&car_reverse_driver);
	if (ret) {
		CAR_REVERSE_ERR("platform driver register failed\n");
		return ret;
	}
	return 0;
}

static void __exit car_reverse_module_exit(void)
{
	kfree(car_reverse);

	platform_driver_unregister(&car_reverse_driver);
}

#if IS_ENABLED(CONFIG_VIDEO_SUNXI_CAR_REVERSE2)
late_initcall(car_reverse_module_init);
#else
module_init(car_reverse_module_init);
#endif
module_exit(car_reverse_module_exit);

MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(ANDROID_GKI_VFS_EXPORT_ONLY);
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
MODULE_AUTHOR("<huangyongxing@allwinnertech.com>");
MODULE_DESCRIPTION("Sunxi fast car reverse image preview");
MODULE_VERSION("1.0.0");

/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#ifndef _DRM_SUNXI_ROTATE_H_
#define _DRM_SUNXI_ROTATE_H_

#include <linux/sunxi_tr.h>

enum rotate_process{
	ROTATE_IN_IDLE = 0,
	ROTATE_IN_WORK,
	ROTATE_IN_FINISH,
	ROTAE_IN_BAD,
};

enum {
	ROTATE_ERROR = -1,
	ROTATE_OK = 0,
	ROTATE_BUSY,
	ROTAE_NO_START,
};

#define ROTATE_DAEMON_HZ  100

struct sunxi_rotate_private {
	struct list_head rotate_work;
	spinlock_t head_lock;
	struct list_head qurey_head;
	struct mutex qurey_mlock;
	int query_cnt;
	struct sunxi_rotate_task *current_task;

	spinlock_t process_lock;
	enum rotate_process active;
	wait_queue_head_t task_wq;
	struct delayed_work daemon_work;
	struct clk *clk;
	int irq;
	struct mutex user_mlock;
	struct idr user_idr;
	int used_cnt;
};

struct sunxi_rotate_task {
	struct kref refcount;
	struct list_head w_head;
	struct list_head q_head;
	tr_info info;
	unsigned long timeout;
	int status;//0 ok, -1 error, 1 busy, 2 not start
	bool sleep_mode;
};

#define to_sunxi_rotate_q(x)	container_of(x, struct sunxi_rotate_task, q_head)
#define to_sunxi_rotate_w(x)	container_of(x, struct sunxi_rotate_task, w_head)
#define to_sunxi_rotate_priv(x)	container_of(x, struct sunxi_rotate_private, daemon_work)

int sunxi_drm_rotate_init(void *dev_private);

int sunxi_drm_rotate_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
int sunxi_drm_rotate_destroy(struct sunxi_rotate_private *rotate_private);
#endif

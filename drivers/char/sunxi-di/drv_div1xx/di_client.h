/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI_CLIENT_H_
#define _DI_CLIENT_H_

#include "sunxi_di.h"
#include "../common/di_utils.h"

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define DI_CLIENT_CNT_MAX 32

#define DI_IN_FB_NUM_MAX 3
#define DI_OUT_FB_NUM_MAX 3
#define DI_FB_NUM_MAX (DI_IN_FB_NUM_MAX + DI_OUT_FB_NUM_MAX)

enum di_mode {
	DI_MODE_INVALID = 0,
	DI_MODE_60HZ,
	DI_MODE_30HZ,
	DI_MODE_BOB,
	DI_MODE_WEAVE,
	DI_MODE_TNR, /* only tnr */
};

enum {
	DI_PROC_STATE_IDLE = 0,
	DI_PROC_STATE_FINISH = DI_PROC_STATE_IDLE,
	DI_PROC_STATE_WAIT2START,
	DI_PROC_STATE_2START,
	DI_PROC_STATE_WAIT4FINISH,
	DI_PROC_STATE_FINISH_ERR,
};

/* for process fb */
struct di_dma_fb {
	struct di_fb *fb;
	struct di_dma_item *dma_item;
};

struct di_client {
	struct list_head node;
	const char *name;

	/* user setting para */
	struct di_timeout_ns timeout;
	struct di_process_fb_arg fb_arg;

	struct di_dma_item *in_fb0;
	struct di_dma_item *in_fb1;
	struct di_dma_item *in_fb2;
	struct di_dma_item *out_fb0;
	struct di_dma_item *out_fb1;
	struct di_dma_item *out_tnr_fb;

	struct di_mapped_buf *md_buf;

	/* runtime context */
	wait_queue_head_t wait;
	atomic_t wait_con;
	u64 proc_fb_seqno;

	/* dev_cdata must be at last */
	uintptr_t dev_cdata;
};

void *di_client_create(const char *name);
void di_client_destroy(void *c);

int di_client_mem_request(struct di_client *c, void *data);
int di_client_mem_release(struct di_client *c, void *data);

int di_client_get_version(struct di_client *c, struct di_version *version);
int di_client_set_timeout(struct di_client *c, struct di_timeout_ns *timeout);
int di_client_process_fb(struct di_client *c, struct di_process_fb_arg *fb_arg);

#endif /* ifndef _DI_CLIENT_H_ */

// SPDX-License-Identifier: GPL-2.0
/*
 * cam_block.h - camera block functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_PDEV_H_
#define _KY_PDEV_H_
#include <linux/types.h>
#include <media/media-entity.h>

struct spm_camera_block;

struct spm_camera_block_ops {
	void (*release)(struct spm_camera_block *b);
};

struct spm_camera_block {
	atomic_t ref_cnt;
	unsigned long base_addr;
	int irq_num;
	struct spm_camera_block_ops *ops;
};

#define SC_BLOCK(p) (is_subdev((struct media_entity*)(p)) ? (&((struct spm_camera_subdev*)(p))->sc_block) : (&((struct spm_camera_vnode*)(p))->sc_block))

void spm_camera_block_init(struct spm_camera_block *b,
			   struct spm_camera_block_ops *ops);
void spm_camera_block_set_base_addr(struct spm_camera_block *b,
				    unsigned long base_addr);
int spm_camera_block_get(struct spm_camera_block *b);
int spm_camera_block_put(struct media_entity *me);
#endif

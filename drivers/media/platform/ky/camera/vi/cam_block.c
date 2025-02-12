// SPDX-License-Identifier: GPL-2.0
/*
 * cam_block.c - camera block functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include <linux/list.h>
#include <asm/atomic.h>
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>
#include "cam_block.h"
#include "vdev.h"
#include "subdev.h"
#include "mlink.h"

void spm_camera_block_init(struct spm_camera_block *b, struct spm_camera_block_ops *ops)
{
	atomic_set(&b->ref_cnt, 1);
	b->ops = ops;
}

void spm_camera_block_set_base_addr(struct spm_camera_block *b, unsigned long base_addr)
{
	b->base_addr = base_addr;
}

int spm_camera_block_get(struct spm_camera_block *b)
{
	return atomic_inc_return(&b->ref_cnt);
}

static int __spm_camera_block_put(struct spm_camera_block *b)
{
	int ret = atomic_dec_return(&b->ref_cnt);

	if (0 == ret && b->ops && b->ops->release)
		b->ops->release(b);
	return ret;
}

int spm_camera_block_put(struct media_entity *me)
{
	struct spm_camera_vnode *spm_vnode = NULL;
	struct spm_camera_subdev *sc_subdev = NULL;
	struct spm_camera_block *b = NULL;

	if (is_subdev(me)) {
		sc_subdev = (struct spm_camera_subdev *)me;
		b = &sc_subdev->sc_block;
	} else {
		spm_vnode = (struct spm_camera_vnode *)me;
		b = &spm_vnode->sc_block;
	}

	return __spm_camera_block_put(b);
}


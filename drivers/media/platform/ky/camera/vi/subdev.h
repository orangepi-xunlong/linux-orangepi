// SPDX-License-Identifier: GPL-2.0
/*
 * subdev.h - subdev functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_SUBDEV_H_
#define _KY_SUBDEV_H_
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/x1/x1_videodev2.h>
#include <linux/notifier.h>
#include <cam_plat.h>
#include "mlink.h"
#include "cam_block.h"

#define SUBDEV_IDX_MASK		(0xff)
#define SUBDEV_SUBGRP_MASK	(0xff00)
#define SUBDEV_SUBGRP_OFFSET	(8)
#define SUBDEV_GRP_MASK		(0xff0000)
#define SUBDEV_GRP_OFFSET	(16)

#define SD_GRP_ID(grp, sub, id)	(((grp) << SUBDEV_GRP_OFFSET) | ((sub) << SUBDEV_SUBGRP_OFFSET) | (id))
#define SD_IDX(grp_id)		((grp_id) & SUBDEV_IDX_MASK)
#define SD_SUB(grp_id)		(((grp_id) & SUBDEV_SUBGRP_MASK) >> SUBDEV_SUBGRP_OFFSET)
#define SD_GRP(grp_id)		(((grp_id) & SUBDEV_GRP_MASK) >> SUBDEV_GRP_OFFSET)

#define SUBDEV_MAX_PADS	(16)

struct spm_camera_subdev {
	struct plat_cam_subdev pcsd;
	char name[KY_VI_ENTITY_NAME_LEN];
	struct spm_camera_pipeline sc_pipeline;
	struct spm_camera_block sc_block;
	struct notifier_block vnode_nb;
	uint32_t pads_stream_enable;
	int is_resetting;
	long (*ioctl)(struct v4l2_subdev *sd, unsigned int cmd, void *arg);
	void (*release)(struct spm_camera_subdev *sc_subdev);
	void (*notify)(struct spm_camera_subdev *sc_subdev, unsigned int notification,
		       void *arg);
};

static inline struct spm_camera_subdev *v4l2_subdev_to_sc_subdev(struct v4l2_subdev *sd)
{
	return (struct spm_camera_subdev *)sd;
}

static inline struct spm_camera_subdev* media_entity_to_sc_subdev(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;

	if (!is_subdev(me))
		return NULL;
	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_sc_subdev(sd);
}

static inline void *spm_subdev_get_drvdata(struct spm_camera_subdev *sc_subdev)
{
	return sc_subdev->pcsd.token;
}

int spm_subdev_init(unsigned int grp_id,
		    const char *name,
		    int is_sensor,
		    const struct v4l2_subdev_ops *ops,
		    unsigned int pads_cnt,
		    struct media_pad *pads,
		    void *drvdata, struct spm_camera_subdev *sc_subdev);
long spm_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg);
#ifdef CONFIG_COMPAT
long spm_subdev_compat_ioctl32(struct v4l2_subdev *sd, unsigned int cmd,
			       unsigned long arg);
#endif
int spm_subdev_reset(struct v4l2_subdev *sd, u32 val);
int spm_subdev_pad_s_stream(struct spm_camera_subdev *sc_subdev, unsigned int pad, int enable);
#endif

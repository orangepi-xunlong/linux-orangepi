/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * camera_version_utils.h - utilities for different kernel versions
 * camera driver supports
 *
 * Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
 */
#ifndef __camera_version_utils__
#define __camera_version_utils__

#include <linux/videodev2.h>

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/debugfs.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-mediabus.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-dv-timings.h>

int tegra_media_entity_init(struct media_entity *entity, u16 num_pads,
		struct media_pad *pad, bool is_subdev, bool is_sensor);

int tegra_media_create_link(struct media_entity *source, u16 source_pad,
		struct media_entity *sink, u16 sink_pad, u32 flags);

bool tegra_is_v4l2_subdev(struct media_entity *entity);

bool tegra_v4l2_match_dv_timings(struct v4l2_dv_timings *t1,
				struct v4l2_dv_timings *t2,
				unsigned int pclock_delta,
				bool match_reduced_fps);

int tegra_vb2_dma_init(struct device *dev, void **alloc_ctx,
		unsigned int size, atomic_t *refcount);

void tegra_vb2_dma_cleanup(struct device *dev, void *alloc_ctx,
		atomic_t *refcount);

#endif

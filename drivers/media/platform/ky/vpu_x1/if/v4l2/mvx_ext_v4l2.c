/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include "mvx_bitops.h"
#include "mvx_buffer.h"
#include "mvx_ext_if.h"
#include "mvx_firmware.h"
#include "mvx_if.h"
#include "mvx_mmu.h"
#include "mvx_session.h"

#include "mvx_v4l2_buffer.h"
#include "mvx_v4l2_session.h"
#include "mvx_v4l2_vidioc.h"
#include "mvx_v4l2_fops.h"
#include "mvx_log_group.h"

static const struct v4l2_file_operations mvx_v4l2_fops = {
	.owner          = THIS_MODULE,
	.open           = mvx_v4l2_open,
	.release        = mvx_v4l2_release,
	.poll           = mvx_v4l2_poll,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2, /* for mvx_v4l2_vidioc_default() */
#endif
	.mmap           = mvx_v4l2_mmap
};

static const struct v4l2_ioctl_ops mvx_v4l2_ioctl_ops = {
	.vidioc_querycap                = mvx_v4l2_vidioc_querycap,
	.vidioc_enum_fmt_vid_cap        = mvx_v4l2_vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out        = mvx_v4l2_vidioc_enum_fmt_vid_out,
	//.vidioc_enum_fmt_vid_cap_mplane = mvx_v4l2_vidioc_enum_fmt_vid_cap,
	//.vidioc_enum_fmt_vid_out_mplane = mvx_v4l2_vidioc_enum_fmt_vid_out,
	.vidioc_enum_framesizes         = mvx_v4l2_vidioc_enum_framesizes,
	.vidioc_g_fmt_vid_cap           = mvx_v4l2_vidioc_g_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap_mplane    = mvx_v4l2_vidioc_g_fmt_vid_cap,
	.vidioc_g_fmt_vid_out           = mvx_v4l2_vidioc_g_fmt_vid_out,
	.vidioc_g_fmt_vid_out_mplane    = mvx_v4l2_vidioc_g_fmt_vid_out,
	.vidioc_s_fmt_vid_cap           = mvx_v4l2_vidioc_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap_mplane    = mvx_v4l2_vidioc_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_out           = mvx_v4l2_vidioc_s_fmt_vid_out,
	.vidioc_s_fmt_vid_out_mplane    = mvx_v4l2_vidioc_s_fmt_vid_out,
	.vidioc_try_fmt_vid_cap         = mvx_v4l2_vidioc_try_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap_mplane  = mvx_v4l2_vidioc_try_fmt_vid_cap,
	.vidioc_try_fmt_vid_out         = mvx_v4l2_vidioc_try_fmt_vid_out,
	.vidioc_try_fmt_vid_out_mplane  = mvx_v4l2_vidioc_try_fmt_vid_out,
	.vidioc_g_selection                  = mvx_v4l2_vidioc_g_selection,
	.vidioc_s_selection             = mvx_v4l2_vidioc_s_selection,
	.vidioc_streamon                = mvx_v4l2_vidioc_streamon,
	.vidioc_streamoff               = mvx_v4l2_vidioc_streamoff,
	.vidioc_encoder_cmd             = mvx_v4l2_vidioc_encoder_cmd,
	.vidioc_try_encoder_cmd         = mvx_v4l2_vidioc_try_encoder_cmd,
	.vidioc_decoder_cmd             = mvx_v4l2_vidioc_decoder_cmd,
	.vidioc_try_decoder_cmd         = mvx_v4l2_vidioc_try_decoder_cmd,
	.vidioc_reqbufs                 = mvx_v4l2_vidioc_reqbufs,
	.vidioc_create_bufs             = mvx_v4l2_vidioc_create_bufs,
	.vidioc_querybuf                = mvx_v4l2_vidioc_querybuf,
	.vidioc_qbuf                    = mvx_v4l2_vidioc_qbuf,
	.vidioc_dqbuf                   = mvx_v4l2_vidioc_dqbuf,
	.vidioc_subscribe_event         = mvx_v4l2_vidioc_subscribe_event,
	.vidioc_unsubscribe_event       = v4l2_event_unsubscribe,
	.vidioc_default                 = mvx_v4l2_vidioc_default,
	.vidioc_g_parm = mvx_v4l2_vidioc_g_parm,
	.vidioc_s_parm = mvx_v4l2_vidioc_s_parm,
};

/****************************************************************************
 * Exported functions and variables
 ****************************************************************************/

int mvx_ext_if_construct(struct mvx_ext_if *ext,
			 struct device *dev,
			 struct mvx_fw_cache *cache,
			 struct mvx_client_ops *client_ops,
			 struct dentry *parent)
{
	int ret;
	const char name[] = "mvx";

	ext->dev = dev;
	ext->cache = cache;
	ext->client_ops = client_ops;
	mutex_init(&ext->lock);

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ext->dsessions = debugfs_create_dir("session", parent);
		if (IS_ERR_OR_NULL(ext->dsessions))
			return -ENOMEM;
	}

	ret = v4l2_device_register(dev, &ext->v4l2_dev);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
			      "Failed to register V4L2 device. ret=%d.", ret);
		goto delete_dentry;
	}

        /* Video device. */
        ext->vdev.fops = &mvx_v4l2_fops;
        ext->vdev.ioctl_ops = &mvx_v4l2_ioctl_ops;
        ext->vdev.release = video_device_release_empty;
        ext->vdev.vfl_dir = VFL_DIR_M2M;
        ext->vdev.v4l2_dev = &ext->v4l2_dev;
        ext->vdev.device_caps = V4L2_CAP_VIDEO_M2M |
            V4L2_CAP_VIDEO_M2M_MPLANE |
            V4L2_CAP_EXT_PIX_FORMAT |
            V4L2_CAP_STREAMING;
        strncpy(ext->vdev.name, name, sizeof(ext->vdev.name));

	video_set_drvdata(&ext->vdev, ext);

	ret = video_register_device(&ext->vdev, VFL_TYPE_VIDEO, -1);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
			      "Failed to register video device. ret=%d.",
			      ret);
		goto unregister_device;
	}

	return 0;

unregister_device:
	v4l2_device_unregister(&ext->v4l2_dev);

delete_dentry:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(ext->dsessions);

	return ret;
}

void mvx_ext_if_destruct(struct mvx_ext_if *ext)
{
	video_unregister_device(&ext->vdev);
	v4l2_device_unregister(&ext->v4l2_dev);

	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(ext->dsessions);
}

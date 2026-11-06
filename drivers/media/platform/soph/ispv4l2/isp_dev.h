/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ISP_DEV_H
#define _ISP_DEV_H

#include <linux/compat.h>
#include <linux/iopoll.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/iommu.h>
#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>
#include <linux/platform_device.h>
#include <media/media-entity.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-device.h>
#include <linux/platform_device.h>
#include <media/v4l2-fwnode.h>
#include <linux/of_graph.h>
#include <media/media-device.h>
#include <media/v4l2-mc.h>
#include <linux/media.h>

#include "vi_defines.h"

#define ISP_V4L2_EVENT_ELEMS 4
#define ISP_SUBDEV_NAME   "ispv4l2_subdev"

#define GRP_ID_SENSOR		BIT(0)
#define GRP_ID_CIF			BIT(1)
#define GRP_ID_ISP			BIT(2)

#define MAX_SENSOR_NUM 12

enum sop_isp_pad {
	ISP_PAD_SINK,
	ISP_PAD_SOURCE,
	ISP_PAD_MAX
};

struct sop_sensor_info {
	struct v4l2_subdev *sd;
	struct device *dev;
	struct v4l2_mbus_config mbus;
	struct v4l2_subdev_frame_interval fi;
	struct v4l2_subdev_format fmt[2];
	struct v4l2_subdev_pad_config cfg;
};

struct sop_isp_subdev {
	struct v4l2_subdev sd;
	struct media_pad pads[ISP_PAD_MAX];
	atomic_t frm_sync_seq;
};

struct sop_isp_async_subdev {
	struct v4l2_async_subdev asd;
	struct v4l2_mbus_config mbus;
};

struct sop_isp_device {
	char name[128];
	struct mutex  mutex;
	struct device *dev;
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	struct sop_vi_dev vi_dev;
	struct sop_isp_subdev isp_sdev;
	struct media_device media_dev;
	struct v4l2_async_notifier notifier;
	struct sop_sensor_info *active_sensor;
	struct sop_sensor_info sensors[MAX_SENSOR_NUM];
	struct v4l2_subdev *cif_sdev;
	int num_sensors;
	int is_powerOn;
};
#endif /* _ISP_DEV_H */

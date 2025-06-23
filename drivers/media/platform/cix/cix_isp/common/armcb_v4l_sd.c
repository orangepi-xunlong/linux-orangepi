// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "armcb_v4l_sd.h"
#include "armcb_v4l2_core.h"
#include "media/v4l2-device.h"
#include "system_logger.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

#ifndef ARMCB_CAM_KO
static void armcb_subdev_unregister_nodes(struct video_device *vdev)
{
	struct v4l2_subdev *sd = video_get_drvdata(vdev);

	sd->devnode = NULL;
	kfree(vdev);
	LOG(LOG_INFO, "unregister v4l-subdev");
}
#endif

static int armcb_subdev_register_nodes_async(struct v4l2_subdev *sd)
{
	int res = 0;

	if (WARN_ON(!sd) || WARN_ON(!sd->name)) {
		LOG(LOG_ERR, "Invalid input v4l2_subdev is NULL");
		return -EINVAL;
	}

	res = v4l2_async_register_subdev(sd);
	if (res < 0) {
		LOG(LOG_ERR, "v4l2_device_register_subdev failed for %s",
		    sd->name);
		WARN_ON(1);
	}

	return res;
}

int armcb_camera_async_complete(struct v4l2_async_notifier *notifier)
{
	int ret = 0;
	struct v4l2_device *v4l2_dev = notifier->v4l2_dev;
	struct v4l2_subdev *sd = NULL;
	struct video_device *vdev = NULL;
#ifdef ARMCB_CAM_KO
	ret = v4l2_device_register_subdev_nodes(v4l2_dev);
#else
	int nr = ARMCB_SUBDEV_NODE_HW_IMGSENS0;

	list_for_each_entry(sd, &v4l2_dev->subdevs, list) {
		if (!(sd->flags & V4L2_SUBDEV_FL_HAS_DEVNODE))
			continue;

		if (sd->devnode)
			continue;

		vdev = kzalloc(sizeof(*vdev), GFP_KERNEL);
		if (!vdev) {
			ret = -ENOMEM;
			goto clean_up;
		}

		video_set_drvdata(vdev, sd);
		(void)strscpy(vdev->name, sd->name, sizeof(vdev->name));
		vdev->dev_parent = sd->dev;
		vdev->v4l2_dev = v4l2_dev;
		vdev->fops = &v4l2_subdev_fops;
		vdev->release = armcb_subdev_unregister_nodes;
		LOG(LOG_INFO, "step6 name(%s) grp_id(%d)", sd->name,
		    sd->grp_id);
		nr = (sd->grp_id < (u32)ARMCB_SUBDEV_NODE_IDX_END) ?
			     sd->grp_id :
			     nr;
		ret = __video_register_device(vdev, VFL_TYPE_SUBDEV, nr, 1,
					      sd->owner);
		if (ret < 0) {
			kfree(vdev);
			goto clean_up;
		}
		sd->devnode = vdev;
	}
#endif

#if defined(CONFIG_MEDIA_CONTROLLER)
	list_for_each_entry(sd, &v4l2_dev->subdevs, list) {
		vdev = sd->devnode;
		if (!vdev) {
			LOG(LOG_ERR, "subdev node %s register failed.",
			    sd->name);
			continue;
		}
		vdev->entity.name = dev_name(&vdev->dev);
		vdev->entity.function = sd->entity.function;
		ret = media_device_register_entity(v4l2_dev->mdev,
						   &vdev->entity);
		if (ret < 0) {
			LOG(LOG_ERR,
			    "register media entity for subdev %s failed.",
			    vdev->dev.kobj.name);
			goto clean_up;
		}
		LOG(LOG_INFO,
		    "register media entity(id %d, name %s) for subdev node %s, sd %s",
		    sd->entity.graph_obj.id, vdev->entity.name,
		    vdev->dev.kobj.name, vdev->name);
	}
	/* Register the media device */
	ret = media_device_register(v4l2_dev->mdev);
	if (ret) {
		LOG(LOG_ERR, "media device register failed (ret=%d)", ret);
		return ret;
	}
#endif
	return ret;

clean_up:
	list_for_each_entry(sd, &v4l2_dev->subdevs, list) {
		if (!sd->devnode)
			break;
		video_unregister_device(sd->devnode);
	}

	return ret;
}

const struct v4l2_file_operations armcb_v4l2_subdev_fops = {};

static void armcb_add_sd_in_position(struct armcb_sd_subdev *armcb_sdreg,
				     struct list_head *sd_list)
{
	struct armcb_sd_subdev *temp_sd = NULL;

	LOG(LOG_INFO, "+");
	list_for_each_entry(temp_sd, sd_list, list) {
		if (temp_sd == armcb_sdreg) {
			LOG(LOG_ERR, "failed to add the same subdev ");
			return;
		}

		if (armcb_sdreg->close_seq < temp_sd->close_seq) {
			list_add_tail(&armcb_sdreg->list, &temp_sd->list);
			return;
		}
	}
	LOG(LOG_INFO, "-");

	list_add_tail(&armcb_sdreg->list, sd_list);
}

int armcb_subdev_register(struct armcb_sd_subdev *armcb_sdreg,
			  unsigned int cam_id)
{
	armcb_v4l2_dev_t *parmcb_dev = NULL;
	struct v4l2_device *pvdev = NULL;

	LOG(LOG_INFO, "+   %d", cam_id);
	parmcb_dev = armcb_v4l2_core_get_dev(cam_id);
	if (!parmcb_dev) {
		LOG(LOG_ERR, "Invalid input armcb_sbreg is NULL");
		return -EINVAL;
	}

	pvdev = &parmcb_dev->v4l2_dev;
	if (WARN_ON(!pvdev) || WARN_ON(!pvdev->dev)) {
		LOG(LOG_ERR, "Invalid pvdev/dev is NULL");
		return -EIO;
	}

	mutex_lock(&parmcb_dev->ordered_sd_mutex);
	armcb_add_sd_in_position(armcb_sdreg, &parmcb_dev->ordered_sd_list);
	mutex_unlock(&parmcb_dev->ordered_sd_mutex);

	return armcb_subdev_register_nodes_async(&armcb_sdreg->sd);
}

int armcb_subdev_unregister(struct armcb_sd_subdev *armcb_sdreg)
{
	if (WARN_ON(!armcb_sdreg)) {
		LOG(LOG_ERR, "Invalid pvdev/dev is NULL");
		return -EINVAL;
	}

	v4l2_device_unregister_subdev(&armcb_sdreg->sd);
	return 0;
}

static struct v4l2_subdev *armcb_subdev_find(const char *name)
{
	unsigned long flags = 0;
	struct v4l2_subdev *subdevI = NULL;
	struct v4l2_subdev *subdevO = NULL;
	armcb_v4l2_dev_t *parmcb_dev = armcb_v4l2_core_get_dev(0);
	struct v4l2_device *pvdev = &parmcb_dev->v4l2_dev;

	spin_lock_irqsave(&pvdev->lock, flags);

	if (!list_empty(&pvdev->subdevs)) {
		list_for_each_entry(subdevI, &pvdev->subdevs, list)
			if (!strcmp(name, subdevI->name)) {
				subdevO = subdevI;
				break;
			}
	}

	spin_unlock_irqrestore(&pvdev->lock, flags);

	return subdevO;
}

void armcb_v4l2_subdev_notify(struct v4l2_subdev *sd, unsigned int notification,
			      void *arg)
{
	struct v4l2_subdev *subdev = NULL;
	struct armcb_subdev_req *sd_req = NULL;

	if (WARN_ON(!sd) || WARN_ON(!arg)) {
		LOG(LOG_ERR, "Invalid sd/arg is NULL");
		return;
	}

	if (!armcb_subdev_find(sd->name)) {
		LOG(LOG_ERR, "sd->name armcb_subdev_find error");
		return;
	}

	sd_req = (struct armcb_subdev_req *)arg;

	switch (notification) {
	case ARMCB_SUBDEV_NOTIFY_GET:
		sd_req->subdev = armcb_subdev_find(sd_req->name);
		break;
	case ARMCB_SUBDEV_NOTIFY_PUT:
		subdev = armcb_subdev_find(sd_req->name);
		break;
	case ARMCB_SUBDEV_NOTIFY_REQ:
		break;
	default:
		break;
	}
}

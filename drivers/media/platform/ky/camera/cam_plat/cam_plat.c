// SPDX-License-Identifier: GPL-2.0
/*
 * plat_cam.c - Driver for KY X1 Platform Camera Manager
 *
 * Copyright(C) 2023 KY Micro Limited.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/x1/x1_plat_cam.h>
#include "cam_dbg.h"
#include "cam_plat.h"

#define PLAT_CAM_DEVICE_COMPATIBLE	"ky,plat-cam"

struct plat_cam_device {
	struct v4l2_device v4l2_dev;
	struct kref ref;
	struct mutex mutex;
};

#define to_plat_cam_dev(v4l2_dev)	\
	(struct plat_cam_device *)(v4l2_dev)

unsigned long phys_cam2cpu(unsigned long phys_addr)
{
	if (phys_addr >= 0x80000000UL) {
		phys_addr += 0x80000000UL;
	}
	return phys_addr;
}

unsigned long phys_cpu2cam(unsigned long phys_addr)
{
	if (phys_addr >= 0x100000000UL) {
		phys_addr -= 0x80000000UL;
	}
	return phys_addr;
}

EXPORT_SYMBOL(phys_cpu2cam);

static void plat_cam_sd_notify(struct v4l2_subdev *sd,
			       unsigned int notification, void *arg)
{
	struct v4l2_subdev *subdev;
	struct plat_cam_device *plat_cam_dev;
	struct plat_cam_subdev *csd;
	struct v4l2_device *v4l2_dev;

	//remove after x1isp register as v4l2 subdev
	v4l2_dev = plat_cam_v4l2_device_get();
	plat_cam_dev = to_plat_cam_dev(v4l2_dev);
	if (unlikely(sd && sd->v4l2_dev != v4l2_dev))
		goto done;

	mutex_lock(&plat_cam_dev->mutex);
	v4l2_device_for_each_subdev(subdev, v4l2_dev) {
		if (subdev == sd)
			continue;
		csd = subdev_to_plat_csd(subdev);
		if (csd->spm_ops && csd->spm_ops->notify)
			csd->spm_ops->notify(subdev, notification, arg);
	}
	mutex_unlock(&plat_cam_dev->mutex);

done:
	plat_cam_v4l2_device_put(v4l2_dev);
	return;
}

int plat_cam_register_subdev(struct plat_cam_subdev *csd)
{
	struct v4l2_subdev *sd;
	struct plat_cam_device *plat_cam_dev;
	struct v4l2_device *v4l2_dev;
	int ret;

	if (!csd || !csd->name) {
		cam_err("invalid arguments");
		return -EINVAL;
	}
	sd = &csd->sd;
	v4l2_dev = plat_cam_v4l2_device_get();
	if (!v4l2_dev) {
		cam_err("failed to get v4l2 device");
		return -ENODEV;
	}
	plat_cam_dev = to_plat_cam_dev(v4l2_dev);

	mutex_lock(&plat_cam_dev->mutex);

	v4l2_subdev_init(sd, csd->ops);
	sd->owner = NULL;
	sd->internal_ops = csd->internal_ops;
	snprintf(sd->name, ARRAY_SIZE(sd->name), csd->name);
	v4l2_set_subdevdata(sd, csd->token);

	sd->flags = csd->sd_flags;

	if (csd->pads_cnt == 0 || csd->pads == NULL)
		ret = media_entity_pads_init(&sd->entity, 0, NULL);
	else
		ret = media_entity_pads_init(&sd->entity, csd->pads_cnt, csd->pads);
	if (ret) {
		cam_err("Failed to register subdev\n");
		goto reg_fail;
	}

	sd->entity.function = csd->ent_function;

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret) {
		cam_err("register subdev failed");
		goto reg_fail;
	}

	ret = v4l2_device_register_subdev_nodes(v4l2_dev);
	if (ret) {
		cam_err("failed to register %s node", sd->name);
		goto reg_fail;
	}

	sd->entity.name = video_device_node_name(sd->devnode);

	if (csd->spm_ops && csd->spm_ops->registered) {
		ret = csd->spm_ops->registered(sd);
		if (ret)
			goto reg_fail;
	}

	mutex_unlock(&plat_cam_dev->mutex);
	return 0;

reg_fail:
	mutex_unlock(&plat_cam_dev->mutex);
	plat_cam_v4l2_device_put(v4l2_dev);
	return ret;
}

EXPORT_SYMBOL(plat_cam_register_subdev);

int plat_cam_unregister_subdev(struct plat_cam_subdev *csd)
{
	struct plat_cam_device *plat_cam_dev;
	struct v4l2_device *v4l2_dev;

	v4l2_dev = csd->sd.v4l2_dev;
	plat_cam_dev = to_plat_cam_dev(v4l2_dev);

	mutex_lock(&plat_cam_dev->mutex);
	if (csd->spm_ops && csd->spm_ops->unregistered)
		csd->spm_ops->unregistered(&csd->sd);
	v4l2_device_unregister_subdev(&csd->sd);
	mutex_unlock(&plat_cam_dev->mutex);

	plat_cam_v4l2_device_put(v4l2_dev);

	return 0;
}

EXPORT_SYMBOL(plat_cam_unregister_subdev);

static struct plat_cam_device *g_dev;
static int plat_cam_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct plat_cam_device *plat_cam_dev;
	struct v4l2_device *v4l2_dev;

	plat_cam_dev = kzalloc(sizeof(*plat_cam_dev), GFP_KERNEL);
	if (!plat_cam_dev) {
		cam_err("could not allocate memory\n");
		return -ENOMEM;
	}
	kref_init(&plat_cam_dev->ref);
	mutex_init(&plat_cam_dev->mutex);

	/* setup v4l2 device */
	v4l2_dev = &plat_cam_dev->v4l2_dev;
	ret = v4l2_device_register(&(pdev->dev), v4l2_dev);
	if (ret)
		return ret;
	g_dev = plat_cam_dev;

#if defined(CONFIG_MEDIA_CONTROLLER)
	/* setup media device */
	v4l2_dev->mdev = kzalloc(sizeof(*v4l2_dev->mdev), GFP_KERNEL);
	if (!v4l2_dev->mdev) {
		cam_err("could not allocate memory\n");
		ret = -ENOMEM;
		goto media_fail;
	}

	media_device_init(v4l2_dev->mdev);
	v4l2_dev->mdev->dev = &(pdev->dev);
	strlcpy(v4l2_dev->mdev->model, PLAT_CAM_NAME, sizeof(v4l2_dev->mdev->model));

	ret = __media_device_register(v4l2_dev->mdev, NULL);
	if (ret)
		goto media_fail;
#endif
	v4l2_dev->notify = plat_cam_sd_notify;

	return 0;

#if defined(CONFIG_MEDIA_CONTROLLER)
media_fail:
	v4l2_device_unregister(v4l2_dev);
	kfree(plat_cam_dev);
#endif
	return ret;
}

static int plat_cam_remove(struct platform_device *pdev)
{
	struct v4l2_device *v4l2_dev = dev_get_drvdata(&pdev->dev);
	struct plat_cam_device *plat_cam_dev = to_plat_cam_dev(v4l2_dev);

#if defined(CONFIG_MEDIA_CONTROLLER)
	media_device_unregister(v4l2_dev->mdev);
	kfree(v4l2_dev->mdev);
	v4l2_dev->mdev = NULL;
#endif
	v4l2_device_unregister(v4l2_dev);
	g_dev = NULL;

	mutex_destroy(&plat_cam_dev->mutex);
	kfree(plat_cam_dev);

	return 0;
}

static const struct of_device_id plat_cam_dt_match[] = {
	{ .compatible = PLAT_CAM_DEVICE_COMPATIBLE, .data = NULL },
	{},
};

static struct platform_driver plat_cam_driver = {
	.driver = {
		.name	= PLAT_CAM_NAME,
		.of_match_table = plat_cam_dt_match,
	},
	.probe = plat_cam_probe,
	.remove = plat_cam_remove,
};

struct v4l2_device *plat_cam_v4l2_device_get(void)
{
	if (g_dev) {
		kref_get(&g_dev->ref);
		return &(g_dev->v4l2_dev);
	} else
		return NULL;
}

EXPORT_SYMBOL(plat_cam_v4l2_device_get);

int plat_cam_v4l2_device_put(struct v4l2_device *v4l2_dev)
{
#if 0
	struct plat_cam_device *plat_cam_dev = to_plat_cam_dev(v4l2_dev);
	struct platform_driver *plat_driver;

	if (!v4l2_dev || !v4l2_dev->dev)
		return -ENODEV;

	plat_driver = to_platform_driver(v4l2_dev->dev->driver);

	kref_put(&plat_cam_dev->ref, NULL);

	if (kref_read(&plat_cam_dev->ref) == 1) {
		plat_driver->remove = plat_cam_remove;
		platform_driver_unregister(plat_driver);
		kfree(plat_driver->driver.name);
		kfree(plat_driver->driver.of_match_table);
		kfree(plat_driver);
	}
#else
	if (g_dev)
		kref_put(&g_dev->ref, NULL);
#endif
	return 0;
}

EXPORT_SYMBOL(plat_cam_v4l2_device_put);

module_platform_driver(plat_cam_driver);

MODULE_DESCRIPTION("KY Camera Platform Driver");
MODULE_LICENSE("GPL");

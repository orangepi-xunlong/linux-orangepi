// SPDX-License-Identifier: BSD
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/consumer.h>
#include "isp_dev.h"

/********************* vi function ***********************/
extern int vi_core_init(struct platform_device *pdev);
extern int vi_register_video_device(struct sop_isp_device *isp_dev);
extern int vi_destroy_instance(struct sop_isp_device *isp_dev);
extern void vi_resume(struct sop_vi_dev *vdev);
extern void vi_suspend(struct sop_vi_dev *vdev);

/********************* ispsubdev function ***********************/
extern int isp_subdev_register(struct sop_isp_device *isp_dev);
extern void isp_subdev_unregister(struct sop_isp_device *isp_dev);

/************************* media controller ***************************/
static int sop_isp_create_links(struct sop_isp_device *dev)
{
	unsigned int s, pad;
	int ret = 0;

	/* sensor links(or mipi-phy) */
	for (s = 0; s < dev->num_sensors; ++s) {
		struct sop_sensor_info *sensor = &dev->sensors[s];
		// u32 type = sensor->sd->entity.function;
		for (pad = 0; pad < sensor->sd->entity.num_pads; pad++)
			if (sensor->sd->entity.pads[pad].flags & MEDIA_PAD_FL_SOURCE)
				break;

		if (pad == sensor->sd->entity.num_pads) {
			dev_err(dev->dev, "failed to find src pad for %s\n",
				sensor->sd->name);
			return -ENXIO;
		}
	}
	return ret;
}

static int subdev_notifier_complete(struct v4l2_async_notifier *notifier)
{
	struct sop_isp_device *dev;
	struct v4l2_device *v4l2_dev;
	int ret;

	dev = container_of(notifier, struct sop_isp_device, notifier);
	v4l2_dev = &dev->v4l2_dev;

	v4l2_info(v4l2_dev, "Async subdev notifier start\n");

	mutex_lock(&dev->media_dev.graph_mutex);
	ret = sop_isp_create_links(dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to create links:%d\n", ret);
		goto unlock;
	}

	ret = v4l2_device_register_subdev_nodes(v4l2_dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to register subdev node:%d\n", ret);
		goto unlock;
	}

	v4l2_info(v4l2_dev, "Async subdev notifier completed\n");

unlock:
	mutex_unlock(&dev->media_dev.graph_mutex);
	return ret;
}

static int subdev_notifier_bound(struct v4l2_async_notifier *notifier,
				 struct v4l2_subdev *subdev,
				 struct v4l2_async_subdev *asd)
{
	struct sop_isp_device *isp_dev = container_of(notifier,
					struct sop_isp_device, notifier);
	struct sop_isp_async_subdev *s_asd = container_of(asd,
					struct sop_isp_async_subdev, asd);

	v4l2_info(&isp_dev->v4l2_dev, "notify bound, num:%d\n", isp_dev->num_sensors);

	if (isp_dev->num_sensors == ARRAY_SIZE(isp_dev->sensors))
		return -EBUSY;

	isp_dev->sensors[isp_dev->num_sensors].mbus = s_asd->mbus;
	isp_dev->sensors[isp_dev->num_sensors].sd = subdev;
	++isp_dev->num_sensors;

	return 0;
}

static int sop_isp_fwnode_parse(struct device *dev,
			       struct v4l2_fwnode_endpoint *vep,
			       struct v4l2_async_subdev *asd)
{
	struct sop_isp_async_subdev *sop_asd =
			container_of(asd, struct sop_isp_async_subdev, asd);
	struct v4l2_fwnode_bus_parallel *bus = &vep->bus.parallel;

	/*
	 * MIPI sensor is linked with a mipi dphy and its media bus config can
	 * not be get in here
	 */
	if (vep->bus_type != V4L2_MBUS_BT656 &&
	    vep->bus_type != V4L2_MBUS_PARALLEL)
		return 0;

	sop_asd->mbus.flags = bus->flags;
	sop_asd->mbus.type = vep->bus_type;

	return 0;
}

static void subdev_notifier_unbind(struct v4l2_async_notifier *notifier,
				   struct v4l2_subdev *subdev,
				   struct v4l2_async_subdev *asd)
{
	struct sop_isp_device *isp_dev = container_of(notifier,
			struct sop_isp_device, notifier);
	struct sop_isp_subdev *isp_sdev = &isp_dev->isp_sdev;
	struct v4l2_subdev *isp_sd = &isp_sdev->sd;
	int i;

	v4l2_info(&isp_dev->v4l2_dev, "+\n");

	for (i = 0; i < isp_dev->num_sensors; i++) {
		if (isp_dev->sensors[i].sd == subdev) {
			media_entity_call(&isp_sd->entity, link_setup,
				isp_sd->entity.pads, subdev->entity.pads, 0);
			isp_dev->sensors[i].sd = NULL;
		}
	}
}

static const struct v4l2_async_notifier_operations subdev_notifier_ops = {
	.bound = subdev_notifier_bound,
	.complete = subdev_notifier_complete,
	.unbind = subdev_notifier_unbind,
};

static int isp_subdev_notifier(struct sop_isp_device *isp_dev)
{
	struct v4l2_async_notifier *ntf = &isp_dev->notifier;
	struct device *dev = isp_dev->dev;
	int ret;

	v4l2_info(&isp_dev->v4l2_dev, "init notify\n");

	v4l2_async_notifier_init(ntf);
	ret = v4l2_async_notifier_parse_fwnode_endpoints(
		dev, ntf, sizeof(struct sop_isp_async_subdev), sop_isp_fwnode_parse);

	if (ret < 0) {
		v4l2_err(&isp_dev->v4l2_dev, "Failed to parse fwnode ep:%d\n", ret);
		return ret;
	}

	ntf->ops = &subdev_notifier_ops;

	return v4l2_async_notifier_register(&isp_dev->v4l2_dev, ntf);
}

/***************************** platform deive *************************/

static int sop_isp_register_platform_subdevs(struct sop_isp_device *dev)
{
	int ret;

	ret = isp_subdev_register(dev);
	if (ret < 0)
		goto err_unreg_isp_subdev;

	ret = isp_subdev_notifier(dev);

	return 0;

err_unreg_isp_subdev:
	isp_subdev_unregister(dev);

	return ret;
}

static const struct media_device_ops sop_isp_media_ops = {
	.link_notify = v4l2_pipeline_link_notify,
};


static int sop_isp_plat_probe(struct platform_device *pdev)
{
	struct sop_isp_device *isp_dev;
	struct v4l2_device *v4l2_dev;
	int num_sensors = MAX_SENSOR_NUM;
	int ret;

	isp_dev = devm_kzalloc(&pdev->dev, sizeof(*isp_dev), GFP_KERNEL);
	if (!isp_dev)
		return -ENOMEM;

	memset(&isp_dev->vi_dev, 0, sizeof(struct sop_vi_dev));

	mutex_init(&isp_dev->mutex);
	isp_dev->dev = &pdev->dev;

	dev_set_drvdata(&pdev->dev, isp_dev);

	snprintf(isp_dev->media_dev.model,
		sizeof(isp_dev->media_dev.model), "%s", "isp_media");
	strscpy(isp_dev->media_dev.driver_name, isp_dev->name,
		sizeof(isp_dev->media_dev.driver_name));

	isp_dev->media_dev.dev = &pdev->dev;
	isp_dev->media_dev.ops = &sop_isp_media_ops;

	v4l2_dev = &isp_dev->v4l2_dev;
	v4l2_dev->mdev = &isp_dev->media_dev;
	strlcpy(v4l2_dev->name, isp_dev->name, sizeof(v4l2_dev->name));

	ret = v4l2_device_register(&pdev->dev, &isp_dev->v4l2_dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to register v4l2 device:%d\n", ret);
		return ret;
	}

	media_device_init(&isp_dev->media_dev);
	ret = media_device_register(&isp_dev->media_dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to register media device:%d\n", ret);
		goto err_unreg_v4l2_dev;
	}

	/* create & register platefom subdev (from of_node) */
	ret = sop_isp_register_platform_subdevs(isp_dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to register subdevs:%d\n", ret);
		goto err_unreg_media_dev;
	}
	ret = v4l2_device_register_subdev_nodes(v4l2_dev);
	if (ret < 0) {
		v4l2_err(v4l2_dev, "Failed to register subdevs node:%d\n", ret);
		goto err_unreg_media_dev;
	}

	while (num_sensors--)
		vi_register_video_device(isp_dev);

	ret = vi_core_init(pdev);
	if (ret < 0)
		v4l2_err(v4l2_dev, "vi core init fail !\n");

	platform_set_drvdata(pdev, v4l2_dev);

	v4l2_info(v4l2_dev, "probe success\n");

	return 0;

err_unreg_media_dev:
	media_device_unregister(&isp_dev->media_dev);
err_unreg_v4l2_dev:
	v4l2_device_unregister(&isp_dev->v4l2_dev);
	return ret;
}

static int sop_isp_plat_remove(struct platform_device *pdev)
{
	struct v4l2_device *v4l2_dev = platform_get_drvdata(pdev);
	struct sop_isp_device *isp_dev = container_of(v4l2_dev,
						      struct sop_isp_device, v4l2_dev);;

	media_device_unregister(&isp_dev->media_dev);
	v4l2_async_notifier_unregister(&isp_dev->notifier);
	v4l2_async_notifier_cleanup(&isp_dev->notifier);
	v4l2_device_unregister(&isp_dev->v4l2_dev);
	isp_subdev_unregister(isp_dev);
	vi_destroy_instance(isp_dev);
	media_device_cleanup(&isp_dev->media_dev);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static int sop_isp_plat_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct v4l2_device *v4l2_dev = platform_get_drvdata(pdev);
	struct sop_isp_device *isp_dev = container_of(v4l2_dev,
						      struct sop_isp_device, v4l2_dev);;

	dev_info(&pdev->dev, "suspend start\n");
	vi_suspend(&isp_dev->vi_dev);
	dev_info(&pdev->dev, "suspend end\n");

	return 0;
}

static int sop_isp_plat_resume(struct platform_device *pdev)
{
	struct v4l2_device *v4l2_dev = platform_get_drvdata(pdev);
	struct sop_isp_device *isp_dev = container_of(v4l2_dev,
						      struct sop_isp_device, v4l2_dev);;

	dev_info(&pdev->dev, "resume start\n");
	vi_resume(&isp_dev->vi_dev);
	dev_info(&pdev->dev, "resume end\n");

	return 0;
}

static const struct of_device_id sop_isp_plat_of_match[] = {
	{
		.compatible = "cvitek,ispv4l2",
	},
	{},
};

struct platform_driver sop_isp_plat_drv = {
	.driver = {
		   .name = "ispv4l2",
		   .of_match_table = of_match_ptr(sop_isp_plat_of_match),
	},
	.probe = sop_isp_plat_probe,
	.remove = sop_isp_plat_remove,
	.suspend = sop_isp_plat_suspend,
	.resume = sop_isp_plat_resume,
};

static int __init isp_drv_init(void)
{
	return platform_driver_register(&sop_isp_plat_drv);
}

static void __exit isp_drv_exit(void)
{
	platform_driver_unregister(&sop_isp_plat_drv);
}

module_init(isp_drv_init);
module_exit(isp_drv_exit);

MODULE_AUTHOR("Cvitek ISP team");
MODULE_DESCRIPTION("Cvitek ISP platform driver");
MODULE_LICENSE("Dual BSD/GPL");

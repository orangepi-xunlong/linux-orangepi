// SPDX-License-Identifier: GPL-2.0
#include "csi_dma_cap.h"
#include "csi_bridge_hw.h"
#include "csi_rcsu_hw.h"
#include "csi_common.h"

enum cix_dphy_pads {
	CIX_CSI_DMA_PAD_SINK,
	CIX_CSI_DMA_PAD_MAX,
};

static int csi_dma_pipeline_set_stream(struct csi_dma_pipeline *p, bool on)
{
	int loop;
	int ret;

	/*csi_dma -> sensor */
	for (loop = 0; loop < p->num_subdevs; loop++) {
		ret = v4l2_subdev_call(p->subdevs[loop], video, s_stream, on);
		if (ret) {
			//debug info
			break;
		}
	}

	return ret;
}

static int csi_dma_pipeline_prepare(struct csi_dma_pipeline *p)
{
	struct v4l2_subdev *sd;
	struct csi_dma_dev *csi_dma = container_of(p, struct csi_dma_dev, pipe);
	struct media_entity *me = &csi_dma->dma_cap->vdev.entity;
	struct media_pad *pad = NULL;
	struct device *dev = &csi_dma->pdev->dev;
	struct device *supply = dev;
	struct device *consumer = dev;
	int loop;

	p->num_subdevs = 0;
	memset(p->subdevs, 0, sizeof(p->subdevs));

	while (1) {
		/* Find remote source pad */
		for (loop = 0; loop < me->num_pads; loop++) {
			struct media_pad *s_pad = &me->pads[loop];

			if (!(s_pad->flags & MEDIA_PAD_FL_SINK))
				continue;

			pad = media_pad_remote_pad_unique(s_pad);
			if (pad)
				break;
		}

		if (!pad)
			return -1;
		sd = media_entity_to_v4l2_subdev(pad->entity);
		dev_info(dev, "get the subdev name %s entity\n", sd->name);
		p->subdevs[p->num_subdevs++] = sd;
		me = &sd->entity;

		/*add device link for pm */
		supply = sd->dev;
		device_link_add(consumer, supply, DL_FLAG_STATELESS);
		consumer = sd->dev;

		/*is the terminor subdev */
		if (me->function == MEDIA_ENT_F_CAM_SENSOR) {
			csi_dma->sensor_sd = sd;
			dev_info(dev, "pipeline get the sensor subdev\n");
			break;
		}
	}
	return 0;
}

static int csi_dma_pipeline_open(struct csi_dma_pipeline *p,
				 struct v4l2_subdev **sensor_sd, bool prepare)
{
	return 0;
}

static int csi_dma_pipeline_close(struct csi_dma_pipeline *p)
{
	/*clean the all subdev information*/
	p->num_subdevs = 0;
	memset(p->subdevs, 0, sizeof(p->subdevs));
	return 0;
}

static int subdev_notifier_bound(struct v4l2_async_notifier *notifier,
		struct v4l2_subdev *subdev,
		struct v4l2_async_connection *asd)
{
	struct csi_dma_dev *csi_dma =
		container_of(notifier, struct csi_dma_dev, notifier);
	struct device *dev = csi_dma->dev;
	int ret;

	dev_info(dev, "csi_dma bound enter\n");

	/*find remote entity source pad */
	csi_dma->dma_cap->source_pad = media_entity_get_fwnode_pad(
		&subdev->entity, subdev->fwnode, MEDIA_PAD_FL_SOURCE);

	if (csi_dma->dma_cap->source_pad < 0) {
		dev_err(dev, "Couldn't find output pad for subdev %s\n",
			subdev->name);
		return -1;
	}

	csi_dma->dma_cap->source_subdev = subdev;

	dev_info(dev, "csi_dma source name %s index %d sink name %s index %d\n",
		 subdev->name, csi_dma->dma_cap->source_pad,
		 csi_dma->dma_cap->vdev.name, CIX_CSI_DMA_PAD_SINK);
	/*create the link*/
	ret = media_create_pad_link(
		&subdev->entity, csi_dma->dma_cap->source_pad,
		&csi_dma->dma_cap->vdev.entity, CIX_CSI_DMA_PAD_SINK,
		MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	return ret;
}

static int subdev_notifier_complete(struct v4l2_async_notifier *notifier)
{
	struct csi_dma_dev *csi_dma =
		container_of(notifier, struct csi_dma_dev, notifier);
	struct device *dev = csi_dma->dev;
	int ret;

	dev_info(dev, "csi_dma complete enter\n");
	ret = v4l2_device_register_subdev_nodes(&csi_dma->v4l2_dev);
	csi_dma_pipeline_prepare(&csi_dma->pipe);

	return ret;
}

static const struct v4l2_async_notifier_operations subdev_notifier_ops = {
	.bound = subdev_notifier_bound,
	.complete = subdev_notifier_complete,
};

static int csi_dma_subdev_notifier(struct csi_dma_dev *csi_dma)
{
	struct v4l2_async_notifier *ntf = &csi_dma->notifier;
	int ret;

	v4l2_async_nf_init(ntf, &csi_dma->v4l2_dev);
	ret = v4l2_async_nf_parse_fwnode_endpoints(
			csi_dma->dev, ntf, sizeof(v4l2_async_subdev), NULL);
	if (ret < 0) {
		dev_err(csi_dma->dev, "%s: parse fwnode failed\n", __func__);
		return ret;
	}

	ntf->ops = &subdev_notifier_ops;

	return v4l2_async_nf_register(ntf);
}

static struct csi_rcsu_dev *rcsu_hw_attach(struct csi_dma_dev *csi_dma)
{
	struct platform_device *plat_dev;
	struct fwnode_handle *np;
	struct csi_rcsu_dev *rcsu_hw;
	struct device *dev = csi_dma->dev;
	struct device *tdev = NULL;

	np = fwnode_find_reference(dev->fwnode, "cix,rcsu", 0);
	if (IS_ERR(np) || !np->ops->device_is_available(np)) {
		dev_err(dev, "failed to get rcsu csi dma %d hw node\n", csi_dma->id);
		return NULL;
	}

	tdev = bus_find_device_by_fwnode(&platform_bus_type, np);
	plat_dev = tdev ? to_platform_device(tdev) : NULL;

	fwnode_handle_put(np);
	if (!plat_dev) {
		dev_err(dev, "failed to get dphy%d hw from node\n",
			csi_dma->id);
		return NULL;
	}

	rcsu_hw = platform_get_drvdata(plat_dev);
	if (!rcsu_hw) {
		dev_err(dev, "failed attach rcsu hw\n");
		return NULL;
	}

	dev_info(dev, "attach rcsu hardware success\n");

	return rcsu_hw;
}

static struct csi_dma_hw_dev *cix_bridge_hw_attach(struct csi_dma_dev *csi_dma)
{
	struct platform_device *plat_dev;
	struct fwnode_handle *np;
	struct csi_dma_hw_dev *csi_bridge_hw;
	struct device *dev = csi_dma->dev;
	struct device *tdev = NULL;

	np = fwnode_find_reference(dev->fwnode, "cix,hw", 0);
	if (IS_ERR(np) || !np->ops->device_is_available(np)) {
		dev_err(dev, "failed to get bridge hw node\n");
		return NULL;
	}

	tdev = bus_find_device_by_fwnode(&platform_bus_type, np);
	plat_dev = tdev ? to_platform_device(tdev) : NULL;

	fwnode_handle_put(np);
	if (!plat_dev) {
		dev_err(dev, "failed to get bridge hw from node\n");
		return NULL;
	}

	csi_bridge_hw = platform_get_drvdata(plat_dev);
	if (!csi_bridge_hw) {
		dev_err(dev, "failed attach bridge hw\n");
		return NULL;
	}

	dev_info(dev, "attach bridge hardware success\n");

	return csi_bridge_hw;
}

static int csi_dma_parse(struct csi_dma_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;
	struct device_node *node = dev->of_node;
	int ret = 0;

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u32(dev, "csi-dma-id", &csi_dma->id);
	} else {
		ret = csi_dma->id =
			of_alias_get_id(node, CIX_DMA_OF_NODE_NAME);
	}

	if ((ret < 0) || (csi_dma->id >= CIX_DMA_DEV_MAX_DEVS)) {
		dev_err(dev, "Invalid driver data or device id (%d)\n",
			csi_dma->id);
		return -EINVAL;
	}

	dev_info(dev, "csi dma id %d\n", csi_dma->id);

	return 0;
}

static const struct of_device_id csi_dma_of_match[] = {
	{ .compatible = "cix,cix-csidma"},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, csi_dma_of_match);

static const struct acpi_device_id csi_dma_acpi_match[] = {
	{ .id = "CIXH3028"},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, csi_dma_acpi_match);

static int csi_dma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct csi_dma_dev *csi_dma;
	struct v4l2_device *v4l2_dev;
	int ret = 0;

	csi_dma = devm_kzalloc(dev, sizeof(*csi_dma), GFP_KERNEL);
	if (!csi_dma)
		return -ENOMEM;

	csi_dma->pdev = pdev;
	csi_dma->dev = dev;

	ret = csi_dma_parse(csi_dma);
	if (ret < 0)
		return ret;

	csi_dma->rcsu_dev = rcsu_hw_attach(csi_dma);
	if (!csi_dma->rcsu_dev) {
		dev_err(dev, "Can't attach rcsu hw device\n");
		return -EINVAL;
	}

	csi_dma->bridge_dev = cix_bridge_hw_attach(csi_dma);
	if (!csi_dma->bridge_dev) {
		dev_err(dev, "Can't attach bridge hw device\n");
		return -EINVAL;
	}

	spin_lock_init(&csi_dma->slock);
	mutex_init(&csi_dma->lock);
	atomic_set(&csi_dma->usage_count, 0);

	/*csi_dma pipeline init*/
	csi_dma->pipe.open = csi_dma_pipeline_open;
	csi_dma->pipe.close = csi_dma_pipeline_close;
	csi_dma->pipe.set_stream = csi_dma_pipeline_set_stream;

	strscpy(csi_dma->media_dev.model, dev_name(dev),
		sizeof(csi_dma->media_dev.model));

	csi_dma->media_dev.dev = dev;

	v4l2_dev = &csi_dma->v4l2_dev;
	v4l2_dev->mdev = &csi_dma->media_dev;

	strscpy(v4l2_dev->name, dev_name(dev), sizeof(v4l2_dev->name));
	ret = v4l2_device_register(dev, &csi_dma->v4l2_dev);
	if (ret < 0)
		return ret;

	media_device_init(&csi_dma->media_dev);
	ret = media_device_register(&csi_dma->media_dev);
	if (ret < 0) {
		dev_err(dev, "Failed to register media device: %d\n", ret);
		goto err_unreg_v4l2_dev;
	}

	ret = csi_dma_register_stream(csi_dma);
	if (ret) {
		dev_err(dev, "registered the stream failed\n");
		goto err_unreg_media_dev;
	}

	/*register buffer updata callback*/
	csi_dma_register_buffer_done(csi_dma);

	ret = csi_dma_subdev_notifier(csi_dma);
	if (ret) {
		dev_err(dev, "registered notifier failed\n");
		goto err_unreg_media_dev;
	}

	platform_set_drvdata(pdev, csi_dma);

	pm_runtime_enable(dev);

	dev_info(dev, "csi_dma %d registered successfully\n", csi_dma->id);
	return 0;

err_unreg_media_dev:
	media_device_unregister(&csi_dma->media_dev);
err_unreg_v4l2_dev:
	v4l2_device_unregister(&csi_dma->v4l2_dev);

	return ret;
}

static int csi_dma_remove(struct platform_device *pdev)
{
	struct csi_dma_dev *csi_dma = platform_get_drvdata(pdev);

	csi_dma_unregister_stream(csi_dma);

	media_device_unregister(&csi_dma->media_dev);
	v4l2_async_nf_unregister(&csi_dma->notifier);
	v4l2_async_nf_cleanup(&csi_dma->notifier);
	v4l2_device_unregister(&csi_dma->v4l2_dev);

	pm_runtime_disable(&pdev->dev);
	dev_info(csi_dma->dev, "csi_dma remove\n");

	return 0;
}

#ifdef CONFIG_PM

static int csi_dma_rpm_suspend(struct device *dev)
{
	struct csi_dma_dev *csi_dma = dev_get_drvdata(dev);
	struct csi_bridge_hw_drv_data *hw_drv;

	hw_drv = (struct csi_bridge_hw_drv_data *)csi_dma->bridge_dev->drv_data;
	if (!hw_drv)
		dev_info(csi_dma->dev, "csi bridge hardware attach failed\n");

	if (csi_dma->stream_on) {
		csi_dma_cap_enable_irq(csi_dma,0);
		v4l2_subdev_call(csi_dma->sensor_sd,video,s_stream,0);
		csi_dma_cap_store(csi_dma);
	}

	hw_drv->csi_bridge_hw_suspend(csi_dma->bridge_dev);

	return 0;
}

static int csi_dma_rpm_resume(struct device *dev)
{
	struct csi_dma_dev *csi_dma = dev_get_drvdata(dev);
	struct csi_bridge_hw_drv_data *hw_drv;

	hw_drv = (struct csi_bridge_hw_drv_data *)csi_dma->bridge_dev->drv_data;
	if (!hw_drv)
		dev_info(csi_dma->dev, "csi bridge hardware attach failed\n");

	hw_drv->csi_bridge_hw_resume(csi_dma->bridge_dev);

	if (csi_dma->stream_on) {
		csi_dma_cap_restore(csi_dma);
		csi_dma_cap_stream_start(csi_dma, &csi_dma->dma_cap->src_f);
		csi_dma_config_rcsu(csi_dma->dma_cap);
		v4l2_subdev_call(csi_dma->sensor_sd,video,s_stream,1);
	}

	return 0;
}
#endif


#ifdef CONFIG_PM_SLEEP

static int csi_dma_suspend(struct device *dev)
{
	return pm_runtime_force_suspend(dev);
}

static int csi_dma_resume(struct device *dev)
{
	pm_runtime_force_resume(dev);

	return 0;
}
#endif

static const struct dev_pm_ops csi_dma_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(csi_dma_suspend, csi_dma_resume)
#endif

#ifdef CONFIG_PM
	SET_RUNTIME_PM_OPS(csi_dma_rpm_suspend, csi_dma_rpm_resume, NULL)
#endif
};

static struct platform_driver
	csi_dma_driver = { .probe = csi_dma_probe,
			   .remove = csi_dma_remove,
			   .driver = {
				   .of_match_table = csi_dma_of_match,
				   .acpi_match_table =
					   ACPI_PTR(csi_dma_acpi_match),
				   .name = CSI_DMA_DRIVER_NAME,
				   .pm = &csi_dma_pm_ops,
			   } };
module_platform_driver(csi_dma_driver);

MODULE_AUTHOR("CIX Tech.");
MODULE_DESCRIPTION("CIX CSI Bridge driver");
MODULE_LICENSE("GPL");

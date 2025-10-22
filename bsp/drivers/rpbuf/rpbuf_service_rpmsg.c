// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * RPMsg-based RPBuf service driver.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rpmsg.h>
#include <linux/aw_rpmsg.h>

#include "rpbuf_internal.h"

#define SUNXI_RPBUF_SERVICE_VERSION "1.1.1"

struct rpmsg_rpbuf_service_instance {
	struct rpmsg_device *rpdev;
};

static int rpmsg_rpbuf_service_cb(struct rpmsg_device *rpdev, void *data, int len,
				  void *priv, u32 src)
{
	struct rpbuf_service *service = dev_get_drvdata(&rpdev->dev);

	return rpbuf_service_get_notification(service, data, len);
}

static int rpmsg_rpbuf_service_notify(void *msg, int msg_len, void *priv)
{
	struct rpmsg_rpbuf_service_instance *inst = priv;
	struct rpmsg_device *rpdev = inst->rpdev;

	return rpmsg_trysend(rpdev->ept, msg, msg_len);
}

static const struct rpbuf_service_ops rpmsg_rpbuf_service_ops = {
	.notify = rpmsg_rpbuf_service_notify,
};

static int rpmsg_rpbuf_service_probe(struct rpmsg_device *rpdev)
{
	struct device *dev = &rpdev->dev;
	struct rpmsg_rpbuf_service_instance *inst;
	struct rpbuf_service *service;
	struct device *dev_tmp;
	int i;
	int ret;

	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	inst = devm_kzalloc(dev, sizeof(struct rpmsg_rpbuf_service_instance), GFP_KERNEL);
	if (IS_ERR_OR_NULL(inst)) {
		dev_err(dev, "kzalloc for rpmsg_rpbuf_service_instance failed\n");
		ret = -ENOMEM;
		goto err_out;
	}
	inst->rpdev = rpdev;

	service = rpbuf_create_service(dev, &rpmsg_rpbuf_service_ops, (void *)inst);
	if (!service) {
		dev_err(dev, "rpbuf_create_service failed\n");
		ret = -ENOMEM;
		goto err_free_inst;
	}

	/*
	 * We should find the underlying remoteproc device.
	 *
	 * Normally, the rpmsg device is created from:
	 *     remoteproc device    <--- defined by vendors, usually in device tree
	 *     |--- rproc     <--- rproc_alloc()
	 *          |--- rproc_vdev    <---- rproc_handle_vdev()
	 *               |--- virtio    <--- register_virtio_device()
	 *                    |--- rpmsg    <--- rpmsg_register_device()
	 *
	 * Therefore the 4th parent of rpmsg device is the underlying remoteproc
	 * device.
	 */
	dev_tmp = dev;
	for (i = 0; i < 4; i++) {
		dev_tmp = dev_tmp->parent;
		if (!dev_tmp) {
			dev_err(dev, "cannot get rpmsg device parent %d\n", i);
			ret = -ENOENT;
			goto err_destroy_service;
		}
		dev_info(dev, "rpmsg device parent %d: %s\n", i, dev_name(dev_tmp));
	}

	/* We use the underlying remoteproc device as rpbuf link token */
	ret = rpbuf_register_service(service, dev_tmp);
	if (ret < 0) {
		dev_err(dev, "rpbuf_register_service failed\n");
		goto err_destroy_service;
	}

	dev_set_drvdata(dev, service);

	return 0;

err_destroy_service:
	rpbuf_destroy_service(service);
err_free_inst:
	devm_kfree(dev, inst);
err_out:
	return ret;
}

static void rpmsg_rpbuf_service_remove(struct rpmsg_device *rpdev)
{
	struct device *dev = &rpdev->dev;
	struct rpbuf_service *service = dev_get_drvdata(dev);
	struct rpmsg_rpbuf_service_instance *inst = service->priv;

	if (IS_ERR_OR_NULL(service)) {
		dev_err(dev, "invalid rpbuf_service ptr\n");
		return;
	}

	dev_set_drvdata(dev, NULL);

	rpbuf_unregister_service(service);
	rpbuf_destroy_service(service);
	devm_kfree(dev, inst);
}

static struct rpmsg_device_id rpmsg_rpbuf_service_id_table[] = {
	{ .name	= "rpbuf-service" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_rpbuf_service_id_table);

static struct rpmsg_driver rpmsg_rpbuf_service = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_rpbuf_service_id_table,
	.probe		= rpmsg_rpbuf_service_probe,
	.callback	= rpmsg_rpbuf_service_cb,
	.remove		= rpmsg_rpbuf_service_remove,
};
#ifdef CONFIG_AW_RPROC_FAST_BOOT
fast_rpmsg_driver(rpmsg_rpbuf_service);
#else
module_rpmsg_driver(rpmsg_rpbuf_service);
#endif

MODULE_DESCRIPTION("RPMsg-based RPBuf service driver");
MODULE_AUTHOR("Junyan Lin <junyanlin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPBUF_SERVICE_VERSION);

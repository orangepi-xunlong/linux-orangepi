/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright © 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/of.h>

#include "sunxi_rproc_standby.h"

static LIST_HEAD(sunxi_rproc_standby_head);

struct sunxi_rproc_standby *sunxi_rproc_standby_find(const char *name)
{
	struct sunxi_rproc_standby *rproc_standby, *tmp_save;

	list_for_each_entry_safe(rproc_standby, tmp_save, &sunxi_rproc_standby_head, list) {
		if (!strcmp(rproc_standby->name, name))
			return rproc_standby;
	}

	return NULL;
}

int sunxi_rproc_standby_ops_register(const char *name, struct sunxi_rproc_standby_ops *ops, void *priv)
{
	struct sunxi_rproc_standby *rproc_standby;

	rproc_standby = (struct sunxi_rproc_standby *)kmalloc(sizeof(*rproc_standby), GFP_KERNEL);
	if (!rproc_standby)
		return -ENOMEM;

	rproc_standby->name = name;
	rproc_standby->ops = ops;
	rproc_standby->priv = priv;
	list_add(&rproc_standby->list, &sunxi_rproc_standby_head);

	return 0;
}

int sunxi_rproc_standby_ops_unregister(const char *name)
{
	struct sunxi_rproc_standby *rproc_standby;

	rproc_standby = sunxi_rproc_standby_find(name);
	if (!rproc_standby)
		return -EINVAL;

	list_del(&rproc_standby->list);
	kfree(rproc_standby);

	return 0;
}


int sunxi_rproc_standby_init_prepare(struct sunxi_rproc_standby *rproc_standby)
{
	struct device *dev = rproc_standby->dev;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	int ret;
	uint32_t sum = 0;

	ret = of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	if (ret) {
		dev_err(dev, "fail to pemory-region iterator init for standby prepare, return %d\n", ret);
		return -ENODEV;
	}

	while (of_phandle_iterator_next(&it) == 0) {
		if (of_get_property(it.node, "sleep-data-loss", NULL))
			sum += 1;
	}

	rproc_standby->num_memstore = sum;
	dev_info(dev, "memstore num: %d\n", rproc_standby->num_memstore);

	return 0;
}

int sunxi_rproc_standby_init(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->init ?
		rproc_standby->ops->init(rproc_standby, pdev) : 0;
}

void sunxi_rproc_standby_exit(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev)
{
	if (rproc_standby && rproc_standby->ops && rproc_standby->ops->exit)
		rproc_standby->ops->exit(rproc_standby, pdev);
}

int sunxi_rproc_standby_start(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->start ?
		rproc_standby->ops->start(rproc_standby) : 0;
}

int sunxi_rproc_standby_stop(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->stop ?
		rproc_standby->ops->stop(rproc_standby) : 0;
}

int sunxi_rproc_standby_attach(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->attach ?
		rproc_standby->ops->attach(rproc_standby) : 0;
}

int sunxi_rproc_standby_detach(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->detach ?
		rproc_standby->ops->detach(rproc_standby) : 0;
}

int sunxi_rproc_standby_prepare(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->prepare ?
		rproc_standby->ops->prepare(rproc_standby) : 0;
}

int sunxi_rproc_standby_suspend(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->suspend ?
		rproc_standby->ops->suspend(rproc_standby) : 0;
}

int sunxi_rproc_standby_suspend_late(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->suspend_late ?
		rproc_standby->ops->suspend_late(rproc_standby) : 0;
}

int sunxi_rproc_standby_resume_early(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->resume_early ?
		rproc_standby->ops->resume_early(rproc_standby) : 0;
}

int sunxi_rproc_standby_resume(struct sunxi_rproc_standby *rproc_standby)
{
	return rproc_standby && rproc_standby->ops && rproc_standby->ops->resume ?
		rproc_standby->ops->resume(rproc_standby) : 0;
}

void sunxi_rproc_standby_complete(struct sunxi_rproc_standby *rproc_standby)
{
	if (rproc_standby && rproc_standby->ops && rproc_standby->ops->complete)
		rproc_standby->ops->complete(rproc_standby);
}

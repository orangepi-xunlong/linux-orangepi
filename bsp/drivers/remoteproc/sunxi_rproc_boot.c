/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2025, Allwinnertech
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
#include "sunxi_rproc_boot.h"

static LIST_HEAD(sunxi_rproc_priv_head);

struct sunxi_rproc_priv *sunxi_rproc_priv_find(const char *name)
{
	struct sunxi_rproc_priv *rproc_priv, *rproc_tmp;

	list_for_each_entry_safe(rproc_priv, rproc_tmp, &sunxi_rproc_priv_head, list) {
		if (!strcmp(rproc_priv->name, name))
			return rproc_priv;
	}
	return NULL;
}
EXPORT_SYMBOL(sunxi_rproc_priv_find);

int sunxi_rproc_priv_ops_register(const char *name, struct sunxi_rproc_ops *rproc_ops, void *priv)
{
	struct sunxi_rproc_priv *rproc_priv;

	rproc_priv = kmalloc(sizeof(*rproc_priv), GFP_KERNEL);
	if (!rproc_priv)
		return -ENOMEM;
	list_add(&rproc_priv->list, &sunxi_rproc_priv_head);

	rproc_priv->name = name;
	rproc_priv->ops = rproc_ops;
	rproc_priv->priv = priv;

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_priv_ops_register);

int sunxi_rproc_priv_ops_unregister(const char *name)
{
	struct sunxi_rproc_priv *rproc_priv;

	rproc_priv = sunxi_rproc_priv_find(name);
	if (!rproc_priv)
		return -EBUSY;

	list_del(&rproc_priv->list);
	kfree(rproc_priv);

	return 0;
}
EXPORT_SYMBOL(sunxi_rproc_priv_ops_unregister);

int sunxi_rproc_priv_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	return rproc_priv->ops->resource_get(rproc_priv, pdev);
}
EXPORT_SYMBOL(sunxi_rproc_priv_resource_get);

void sunxi_rproc_priv_resource_put(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	rproc_priv->ops->resource_put(rproc_priv, pdev);
}
EXPORT_SYMBOL(sunxi_rproc_priv_resource_put);

int sunxi_rproc_priv_attach(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->attach(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_attach);

int sunxi_rproc_priv_start(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->start(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_start);

int sunxi_rproc_priv_stop(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->stop(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_stop);

int sunxi_rproc_priv_reset(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->reset(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_reset);

int sunxi_rproc_priv_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return rproc_priv->ops->set_localram(rproc_priv, value);
}
EXPORT_SYMBOL(sunxi_rproc_priv_set_localram);

int sunxi_rproc_priv_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return rproc_priv->ops->set_runstall(rproc_priv, value);
}
EXPORT_SYMBOL(sunxi_rproc_priv_set_runstall);

bool sunxi_rproc_priv_is_booted(struct sunxi_rproc_priv *rproc_priv)
{
	return rproc_priv->ops->is_booted(rproc_priv);
}
EXPORT_SYMBOL(sunxi_rproc_priv_is_booted);

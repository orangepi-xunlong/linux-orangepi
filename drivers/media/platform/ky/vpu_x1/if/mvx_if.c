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
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/export.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/of_device.h>
#include "mvx_ext_if.h"
#include "mvx_if.h"
#include "mvx_log_group.h"
#include "mvx_firmware.h"
#include "mvx_firmware_cache.h"
#include "mvx_secure.h"
#include "mvx_session.h"

/****************************************************************************
 * Types
 ****************************************************************************/

/**
 * struct mvx_if_ctx - Device context.
 *
 * There is one instance of this structure for each device.
 */
struct mvx_if_ctx {
	struct device *dev;
	struct mvx_ext_if ext;
	struct mvx_fw_cache firmware;
	struct mvx_client_ops *client_ops;
	struct mvx_if_ops if_ops;
	struct mvx_secure secure;
	struct kobject kobj;
	struct completion kobj_unregister;
	struct dentry *dentry;
};

/****************************************************************************
 * Static variables and functions
 ****************************************************************************/

/* Physical hardware can handle 40 physical bits. */
static uint64_t mvx_if_dma_mask = DMA_BIT_MASK(40);

static struct mvx_if_ctx *if_ops_to_if_ctx(struct mvx_if_ops *ops)
{
	return container_of(ops, struct mvx_if_ctx, if_ops);
}

static void if_release(struct kobject *kobj)
{
	struct mvx_if_ctx *ctx = container_of(kobj, struct mvx_if_ctx, kobj);

	complete(&ctx->kobj_unregister);
}

static struct kobj_type if_ktype = {
	.release   = if_release,
	.sysfs_ops = &kobj_sysfs_ops
};

/****************************************************************************
 * Exported variables and functions
 ****************************************************************************/

struct mvx_if_ops *mvx_if_create(struct device *dev,
				 struct mvx_client_ops *client_ops,
				 void *priv)
{
	struct mvx_if_ctx *ctx;
	int ret;

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO, "probe");

	dev->dma_mask = &mvx_if_dma_mask;
	dev->coherent_dma_mask = mvx_if_dma_mask;

	/*
	 * This parameter is indirectly used by DMA-API to limit a lookup
	 * through a hash table with allocated DMA regions. If the value is
	 * not high enough, a lookup will be terminated too early and a false
	 * negative warning will be generated for every DMA operation.
	 *
	 * To prevent this behavior vb2-dma-contig allocator keeps this value
	 * set to the maximum requested buffer size. Unfortunately this is not
	 * done for vb2-dma-sg which we are using, so we have to implement the
	 * same logic.
	 *
	 * In this change I set a value permanently to 2Gb, but in the next
	 * commit a functionality similar to vb2-dma-contig will be added.
	 *
	 * Mentioned structure also has one more member: segment_boundary_mask.
	 * It has to be investigated if any value should be assigned to it.
	 *
	 * See the following kernel commit for the reference:
	 * 3f03396918962b2f8b888d02b23cd1e0c88bf5e5
	 */
	dev->dma_parms = devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
	if (dev->dma_parms == NULL)
		return ERR_PTR(-ENOMEM);

	dma_set_max_seg_size(dev, SZ_2G);

#if (KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE) && IS_ENABLED(CONFIG_OF)
	of_dma_configure(dev, dev->of_node, true);
#endif

	/* Create device context and store pointer in device private data. */
	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL) {
		ret = -ENOMEM;
		goto free_dma_parms;
	}

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		char name[20];

		scnprintf(name, sizeof(name), "%s%u", MVX_IF_NAME, dev->id);
		ctx->dentry = debugfs_create_dir(name, NULL);
		if (IS_ERR_OR_NULL(ctx->dentry)) {
			ret = -EINVAL;
			goto free_ctx;
		}
	}

	/* Store context in device private data. */
	ctx->dev = dev;
	ctx->client_ops = client_ops;

	/* Initialize if ops. */
	ctx->if_ops.irq = mvx_session_irq;

	init_completion(&ctx->kobj_unregister);

	/* Create sysfs entry for the device */
	ret = kobject_init_and_add(&ctx->kobj, &if_ktype,
				   kernel_kobj, "amvx%u", dev->id);
	if (ret != 0) {
		kobject_put(&ctx->kobj);
		goto remove_debugfs;
	}

	/* Initialize secure video. */
	ret = mvx_secure_construct(&ctx->secure, dev);
	if (ret != 0)
		goto delete_kobject;

	/* Initialize firmware cache. */
	ret = mvx_fw_cache_construct(&ctx->firmware, dev, &ctx->secure,
				     &ctx->kobj);
	if (ret != 0)
		goto destroy_secure;

	/* Create the external device interface. */
	ret = mvx_ext_if_construct(&ctx->ext, dev, &ctx->firmware,
				   ctx->client_ops, ctx->dentry);
	if (ret != 0)
		goto destroy_fw_cache;

	return &ctx->if_ops;

destroy_fw_cache:
	mvx_fw_cache_destruct(&ctx->firmware);

destroy_secure:
	mvx_secure_destruct(&ctx->secure);

delete_kobject:
	kobject_put(&ctx->kobj);

remove_debugfs:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(ctx->dentry);

free_ctx:
	devm_kfree(dev, ctx);

free_dma_parms:
	devm_kfree(dev, dev->dma_parms);

	return ERR_PTR(ret);
}

void mvx_if_destroy(struct mvx_if_ops *if_ops)
{
	struct mvx_if_ctx *ctx = if_ops_to_if_ctx(if_ops);
	struct device *dev = ctx->dev;

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO, "remove");

	mvx_ext_if_destruct(&ctx->ext);
	mvx_fw_cache_destruct(&ctx->firmware);
	mvx_secure_destruct(&ctx->secure);
	kobject_put(&ctx->kobj);
	wait_for_completion(&ctx->kobj_unregister);
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(ctx->dentry);

	devm_kfree(dev, dev->dma_parms);
	devm_kfree(dev, ctx);

	dev->dma_mask = NULL;
	dev->coherent_dma_mask = 0;
}

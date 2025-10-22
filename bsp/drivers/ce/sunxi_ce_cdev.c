/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * The driver of SUNXI SecuritySystem controller.
 *
 * Copyright (C) 2013 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>

#include "sunxi_ce_cdev.h"
#include "v5/sunxi_ce_reg.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>


sunxi_ce_cdev_t	*ce_cdev;

static DEFINE_MUTEX(ce_lock);

void ce_dev_lock(void)
{
	mutex_lock(&ce_lock);
}

void ce_dev_unlock(void)
{
	mutex_unlock(&ce_lock);
}

void __iomem *ss_membase(void)
{
	return ce_cdev->base_addr;
}

void ce_reset(void)
{
	SS_ENTER();
	reset_control_assert(ce_cdev->reset);
	reset_control_deassert(ce_cdev->reset);
#ifdef CE_DBL_ENT_SRC_EN
	/* TRNG double entropy should be enabled when ce is enabled */
	ss_trng_dbl_ent_en();
#endif
}

#if IS_ENABLED(CONFIG_AW_HWRNG_DRIVER)
int ce_get_hardware_random(u8 *dst, u32 dlen, int flag)
{
	crypto_rng_req_ctx_t req;

	memset(&req, 0x0, sizeof(crypto_rng_req_ctx_t));

	req.channel_id = 0;
	req.trng = 1;
	req.dst_buffer = dst;
	req.dst_length = dlen;

	return do_rng_crypto(&req);
}
#endif

/* Requeset the resource: IRQ, mem */
static int sunxi_ce_res_request(sunxi_ce_cdev_t *p_cdev)
{
	int ret = 0;
	struct device_node *pnode = p_cdev->pnode;

	p_cdev->irq = irq_of_parse_and_map(pnode, SS_RES_INDEX);
	if (p_cdev->irq == 0) {
		SS_ERR("Failed to get the CE IRQ.\n");
		return -EINVAL;
	}

	ret = request_irq(p_cdev->irq,
			sunxi_ce_irq_handler, 0, p_cdev->dev_name, p_cdev);
	if (ret != 0) {
		SS_ERR("Cannot request IRQ\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_OF)
	p_cdev->base_addr = of_iomap(pnode, SS_RES_INDEX);
	if (p_cdev->base_addr == NULL) {
		SS_ERR("Unable to remap IO\n");
		return -ENXIO;
	}
#endif

	return 0;
}

/* Release the resource: IRQ, mem */
static int sunxi_ce_res_release(sunxi_ce_cdev_t *p_cdev)
{
	if (p_cdev->base_addr)
		iounmap(p_cdev->base_addr);
	if (p_cdev->irq)
		free_irq(p_cdev->irq, p_cdev);
	if (p_cdev->pnode)
		of_node_put(p_cdev->pnode);
	return 0;
}

static s32 sunxi_get_ce_device_node(struct device_node **pnode, void __iomem **base,
		s8 *compatible)
{
	*pnode = of_find_compatible_node(NULL, NULL, compatible);
	if (IS_ERR_OR_NULL(*pnode)) {
		SS_ERR("Failed to find \"%s\" in dts.\n", compatible);
		return -EINVAL;
	}

	*base = of_iomap(*pnode, 0);  /* reg[0] must be accessible. */
	if (*base == NULL) {
		SS_ERR("Unable to remap IO\n");
		return -2;
	}
	SS_DBG("Base addr of \"%s\" is %p\n", compatible, *base);
	return 0;
}

static int sunxi_ce_hw_init(sunxi_ce_cdev_t *p_cdev)
{
	int ret = 0;
#ifndef SET_CE_CLKFRE_MODE2
	u32 gen_clkrate;
#endif

	ret = sunxi_get_ce_device_node(&p_cdev->pnode, &p_cdev->base_addr, SUNXI_CE_DEV_NODE_NAME);
	if (ret) {
		SS_ERR("sunxi_get_ce_device_node fail, return %x\n", ret);
		return -EINVAL;
	}

	/* get src clk reg */
	p_cdev->pclk = of_clk_get_by_name(p_cdev->pnode, "clk_src");
	if (IS_ERR_OR_NULL(p_cdev->pclk)) {
		pr_warn("Unable to get src clock\n");
	}

	/* get ce clk reg */
	p_cdev->ce_clk = of_clk_get_by_name(p_cdev->pnode, "ce_clk");
	if (IS_ERR_OR_NULL(p_cdev->ce_clk)) {
		SS_ERR("Fail to get module clk\n");
		return PTR_ERR(p_cdev->ce_clk);
	}

	/* get bus ce reg */
	p_cdev->bus_clk = of_clk_get_by_name(p_cdev->pnode, "bus_ce");
	if (IS_ERR_OR_NULL(p_cdev->bus_clk)) {
		SS_ERR("Fail to get bus clk\n");
		return PTR_ERR(p_cdev->bus_clk);
	}

	/* get mbus ce reg */
	p_cdev->mbus_clk = of_clk_get_by_name(p_cdev->pnode, "mbus_ce");
	if (IS_ERR_OR_NULL(p_cdev->mbus_clk)) {
		pr_warn("Fail to get bus clk\n");
	}

	/* get ce sys clk reg */
	p_cdev->ce_sys_clk = of_clk_get_by_name(p_cdev->pnode, "ce_sys_clk");
	if (IS_ERR_OR_NULL(p_cdev->ce_sys_clk))
		SS_ERR("Fail to get module ce sys clk\n");

#ifndef SET_CE_CLKFRE_MODE2
	/* get ce need configure clk */
	if (of_property_read_u32(p_cdev->pnode, "clock-frequency", &gen_clkrate)) {

		SS_ERR("Unable to get clock-frequency.\n");
		/* return -EINVAL; */
	}
	SS_DBG("The clk freq: %d\n", gen_clkrate);
#endif
	p_cdev->reset = of_reset_control_get(p_cdev->pnode, NULL);
	if (IS_ERR(p_cdev->reset)) {
		SS_ERR("Fail to get reset clk\n");
		return PTR_ERR(p_cdev->reset);
	}

	/* deassert ce reset */
	if (reset_control_deassert(p_cdev->reset)) {
		SS_ERR("Couldn't deassert reset\n");
		return -EBUSY;
	}

	if (!IS_ERR_OR_NULL(p_cdev->pclk)) {
		SS_DBG("SS mclk %luMHz, p_cdev->pclk %luMHz\n", clk_get_rate(p_cdev->ce_clk)/1000000,
				clk_get_rate(p_cdev->pclk)/1000000);
	}

	/* enable ce gating */
	if (clk_prepare_enable(p_cdev->bus_clk)) {
		SS_ERR("Couldn't enable bus gating\n");
		return -EBUSY;
	}

	/* enable ce clk */
	if (clk_prepare_enable(p_cdev->ce_clk)) {
		SS_ERR("Couldn't enable ce clock\n");
		return -EBUSY;
	}

	/* enable ce mbus_clock */
	if (!IS_ERR_OR_NULL(p_cdev->mbus_clk)) {
		if (clk_prepare_enable(p_cdev->mbus_clk)) {
			SS_ERR("Couldn't enable ce mbus clock\n");
			return -EBUSY;
		}
	}

	/* enable ce sys clk */
	if (!IS_ERR_OR_NULL(p_cdev->ce_sys_clk)) {
		if (clk_prepare_enable(p_cdev->ce_sys_clk))
			SS_ERR("Couldn't enable ce sys clock\n");
	}

	sunxi_ce_res_request(p_cdev);

#ifdef CE_DBL_ENT_SRC_EN
	/* TRNG double entropy should be enabled when ce is enabled */
	ss_trng_dbl_ent_en();
#endif

	return 0;
}

static int sunxi_ce_hw_exit(void)
{
	if (!IS_ERR_OR_NULL(ce_cdev->ce_sys_clk))
		clk_disable_unprepare(ce_cdev->ce_sys_clk);
	if (!IS_ERR_OR_NULL(ce_cdev->mbus_clk))
		clk_disable_unprepare(ce_cdev->mbus_clk);
	if (!IS_ERR_OR_NULL(ce_cdev->ce_clk))
		clk_disable_unprepare(ce_cdev->ce_clk);
	if (!IS_ERR_OR_NULL(ce_cdev->bus_clk))
		clk_disable_unprepare(ce_cdev->bus_clk);

	if (!IS_ERR_OR_NULL(ce_cdev->reset))
		reset_control_assert(ce_cdev->reset);

	sunxi_ce_res_release(ce_cdev);
	return 0;
}

static u64 sunxi_ss_dma_mask = DMA_BIT_MASK(64);


static int sunxi_ce_open(struct inode *inode, struct file *file)
{
	file->private_data = ce_cdev;
	return 0;
}

static int sunxi_ce_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static ssize_t sunxi_ce_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t sunxi_ce_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static int sunxi_ce_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}

static int sunxi_copy_from_user(u8 **src, u32 size)
{
	int ret = -1;
	u8 *tmp = NULL;

	tmp = kzalloc(size, GFP_KERNEL);
	if (!tmp) {
		SS_ERR("kzalloc fail\n");
		return -ENOMEM;
	}

	ret = copy_from_user(tmp, *src, size);
	if (ret) {
		SS_ERR("copy_from_user fail\n");
		return ret;
	}

	*src = tmp;

	return 0;
}

static int sunxi_ce_channel_request(void)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&ce_cdev->lock, flags);
	for (i = 0; i < SS_FLOW_NUM; i++) {
		if (ce_cdev->flows[i].available == SS_FLOW_AVAILABLE) {
			ce_cdev->flows[i].available = SS_FLOW_UNAVAILABLE;
			SS_DBG("The flow %d is available.\n", i);
			break;
		}
	}
	ce_cdev->flag = 1;
	spin_unlock_irqrestore(&ce_cdev->lock, flags);

	if (i == SS_FLOW_NUM) {
		SS_ERR("Failed to get an available flow.\n");
		i = -1;
	}
	return i;
}

static void sunxi_ce_channel_free(int id)
{
	unsigned long flags;

	spin_lock_irqsave(&ce_cdev->lock, flags);
	ce_cdev->flows[id].available = SS_FLOW_AVAILABLE;
	ce_cdev->flag = 0;
	SS_DBG("The flow %d is free.\n", id);
	spin_unlock_irqrestore(&ce_cdev->lock, flags);
}

static int ce_release_resources(void *req_ctx, ulong cmd)
{
	crypto_aes_req_ctx_t *aes_req_ctx = (crypto_aes_req_ctx_t *)req_ctx;
	crypto_hash_req_ctx_t *hash_req_ctx = (crypto_hash_req_ctx_t *)req_ctx;
	crypto_rng_req_ctx_t *rng_req_ctx = (crypto_rng_req_ctx_t *)req_ctx;
#ifdef SS_SUPPORT_CE_V5
	crypto_rsa_req_ctx_t *rsa_req_ctx = (crypto_rsa_req_ctx_t *)req_ctx;
	crypto_ecc_req_ctx_t *ecc_req_ctx = (crypto_ecc_req_ctx_t *)req_ctx;
#endif
	switch (cmd) {
	case CE_IOC_AES_CRYPTO:
		if (!aes_req_ctx) {
			SS_ERR("input is NULL\n");
			return -EINVAL;
		}

		if (aes_req_ctx->key_buffer)
			kfree(aes_req_ctx->key_buffer);
		if (aes_req_ctx->iv_buf)
			kfree(aes_req_ctx->iv_buf);

		if (aes_req_ctx->ion_flag == 0) {
			if (aes_req_ctx->src_buffer)
				kfree(aes_req_ctx->src_buffer);
			if (aes_req_ctx->dst_buffer)
				kfree(aes_req_ctx->dst_buffer);
		}

		if (aes_req_ctx)
			kfree(aes_req_ctx);

		ce_dev_unlock();
		break;
#ifdef SS_SUPPORT_CE_V5
	case CE_IOC_RSA_CRYPTO:
		if (!rsa_req_ctx) {
			SS_ERR("input is NULL\n");
			return -EINVAL;
		}

		if (rsa_req_ctx->sign_buffer)
			kfree(rsa_req_ctx->sign_buffer);
		if (rsa_req_ctx->pkey_buffer)
			kfree(rsa_req_ctx->pkey_buffer);
		if (rsa_req_ctx->dst_buffer)
			kfree(rsa_req_ctx->dst_buffer);
		if (rsa_req_ctx)
			kfree(rsa_req_ctx);
		ce_dev_unlock();

		break;
	case CE_IOC_ECC_CRYPTO:
		if (!ecc_req_ctx) {
			SS_ERR("input is NULL\n");
			return -EINVAL;
		}

		if (ecc_req_ctx->dst_buffer)
			kfree(ecc_req_ctx->dst_buffer);
		if (ecc_req_ctx->src_buffer)
			kfree(ecc_req_ctx->src_buffer);
		if (ecc_req_ctx->key_buffer)
			kfree(ecc_req_ctx->key_buffer);
		if (ecc_req_ctx->iv_buffer)
			kfree(ecc_req_ctx->iv_buffer);
		if (ecc_req_ctx)
			kfree(ecc_req_ctx);
		ce_dev_unlock();

		break;
	case CE_IOC_HASH_CRYPTO:
		if (!hash_req_ctx) {
			SS_ERR("input is NULL\n");
			return -EINVAL;
		}

		if (hash_req_ctx->text_buffer)
			kfree(hash_req_ctx->text_buffer);
		if (hash_req_ctx->key_buffer)
			kfree(hash_req_ctx->key_buffer);
		if (hash_req_ctx->dst_buffer)
			kfree(hash_req_ctx->dst_buffer);
		if (hash_req_ctx->iv_buffer)
			kfree(hash_req_ctx->iv_buffer);
		if (hash_req_ctx)
			kfree(hash_req_ctx);
		ce_dev_unlock();
		break;
	case CE_IOC_RNG_CRYPTO:
		if (!rng_req_ctx) {
			SS_ERR("input is NULL\n");
			return -EINVAL;
		}

		if (rng_req_ctx->key_buffer)
			kfree(rng_req_ctx->key_buffer);

		if (rng_req_ctx->src_buffer)
			kfree(rng_req_ctx->src_buffer);
		if (rng_req_ctx->dst_buffer)
			kfree(rng_req_ctx->dst_buffer);

		if (rng_req_ctx)
			kfree(rng_req_ctx);

		ce_dev_unlock();
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int ioctl_aes_crypto(unsigned int cmd, unsigned long arg)
{
	crypto_aes_req_ctx_t *aes_req_ctx;
	int ret;
	phys_addr_t usr_key_addr;
	phys_addr_t usr_dst_addr;

	SS_DBG("arg_size = 0x%x\n", _IOC_SIZE(cmd));
	if (_IOC_SIZE(cmd) != sizeof(crypto_aes_req_ctx_t)) {
		SS_DBG("arg_size != sizeof(crypto_aes_req_ctx_t)\n");
		return -EINVAL;
	}

	ce_dev_lock();

	aes_req_ctx = kzalloc(sizeof(crypto_aes_req_ctx_t), GFP_KERNEL);
	if (!aes_req_ctx) {
		SS_ERR("kzalloc aes_req_ctx fail\n");
		return -ENOMEM;
	}

	ret = copy_from_user(aes_req_ctx, (crypto_aes_req_ctx_t *)arg,
			sizeof(crypto_aes_req_ctx_t));
	if (ret) {
		SS_ERR("copy_from_user fail\n");
		ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
		return ret;
	}
	usr_key_addr = (phys_addr_t)(uintptr_t)aes_req_ctx->key_buffer;
	usr_dst_addr = (phys_addr_t)(uintptr_t)aes_req_ctx->dst_buffer;
	SS_DBG("usr_key_addr = 0x%px usr_dst_addr = 0x%px\n",
			(void *)(uintptr_t)usr_key_addr, (void *)(uintptr_t)usr_dst_addr);
	ret = sunxi_copy_from_user(&aes_req_ctx->key_buffer, aes_req_ctx->key_length);
	if (ret) {
		SS_ERR("key_buffer copy_from_user fail\n");
		ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
		return ret;
	}

	SS_DBG("aes_req_ctx->key_buffer = 0x%px\n", aes_req_ctx->key_buffer);
	ret = sunxi_copy_from_user(&aes_req_ctx->iv_buf, aes_req_ctx->iv_length);
	if (ret) {
		SS_ERR("iv_buffer copy_from_user fail\n");
		ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
		return ret;
	}

	if (aes_req_ctx->ion_flag) {
		SS_DBG("src_phy = 0x%lx, dst_phy = 0x%lx\n", aes_req_ctx->src_phy, aes_req_ctx->dst_phy);
	} else {
		SS_DBG("aes do copy\n");
		ret = sunxi_copy_from_user(&aes_req_ctx->src_buffer, aes_req_ctx->src_length);
		if (ret) {
			SS_ERR("src_buffer copy_from_user fail\n");
			ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
			return ret;
		}

		ret = sunxi_copy_from_user(&aes_req_ctx->dst_buffer, aes_req_ctx->dst_length);
		if (ret) {
			SS_ERR("dst_buffer copy_from_user fail\n");
			ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
			return ret;
		}
	}

	SS_DBG("do_aes_crypto start\n");
	ret = do_aes_crypto(aes_req_ctx);
	if (ret) {
		ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
		SS_ERR("do_aes_crypto fail\n");
		return ret;
	}

	if (aes_req_ctx->ion_flag) {
	} else {
		ret = copy_to_user((u8 *)(uintptr_t)usr_dst_addr, aes_req_ctx->dst_buffer, aes_req_ctx->dst_length);
		if (ret) {
			SS_ERR(" dst_buffer copy_from_user fail\n");
			ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);
			return ret;
		}
	}

	ce_release_resources((void *)aes_req_ctx, CE_IOC_AES_CRYPTO);

	return 0;
}

#ifdef SS_SUPPORT_CE_V5
static int ioctl_rsa_crypto(unsigned int cmd, unsigned long arg)
{
	crypto_rsa_req_ctx_t *rsa_req_ctx;
	int ret;
	phys_addr_t usr_dst_addr;
	SS_DBG("arg_size = 0x%x\n", _IOC_SIZE(cmd));
	if (_IOC_SIZE(cmd) != sizeof(crypto_rsa_req_ctx_t)) {
		SS_DBG("arg_size != sizeof(crypto_rsa_req_ctx_t)\n");
		return -EINVAL;
	}

	ce_dev_lock();
	rsa_req_ctx = kzalloc(sizeof(crypto_rsa_req_ctx_t), GFP_KERNEL);
	if (!rsa_req_ctx) {
		SS_ERR("kzalloc rsa_req_ctx fail\n");
		return -ENOMEM;
	}

	ret = copy_from_user(rsa_req_ctx, (crypto_rsa_req_ctx_t *)arg,
			     sizeof(crypto_rsa_req_ctx_t));
	if (ret) {
		SS_ERR("copy_from_user fail\n");
		ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);
		return ret;
	}
	usr_dst_addr = (phys_addr_t)rsa_req_ctx->dst_buffer;
	ret = sunxi_copy_from_user(&rsa_req_ctx->dst_buffer,
				   rsa_req_ctx->dst_length);
	if (ret) {
		SS_ERR("dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);
		return ret;
	};

	ret = sunxi_copy_from_user(&rsa_req_ctx->sign_buffer,
				   rsa_req_ctx->sign_length);
	if (ret) {
		SS_ERR("sign_buffer copy_from_user fail\n");
		ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);
		return ret;
	};

	ret = sunxi_copy_from_user(&rsa_req_ctx->pkey_buffer,
				   rsa_req_ctx->pkey_length);
	if (ret) {
		SS_ERR("pkey_buffer copy_from_user fail\n");
		ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);
		return ret;
	};

	SS_DBG("do_rsa_crypto start\n");
	ret = do_rsa_crypto(rsa_req_ctx);
	if (ret) {
		ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);
		SS_ERR("do_aes_crypto fail\n");
		return ret;
	}

	ret = copy_to_user((u8 *)usr_dst_addr, rsa_req_ctx->dst_buffer,
			   rsa_req_ctx->dst_length);
	if (ret) {
		SS_ERR(" dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);
		return ret;
	}

	ce_release_resources((void *)rsa_req_ctx, CE_IOC_RSA_CRYPTO);

	return 0;
}

static int ioctl_ecc_crypto(unsigned int cmd, unsigned long arg)
{
	crypto_ecc_req_ctx_t *ecc_req_ctx;
	int ret;
	phys_addr_t usr_dst_addr;
	SS_DBG("arg_size = 0x%x\n", _IOC_SIZE(cmd));
	if (_IOC_SIZE(cmd) != sizeof(crypto_ecc_req_ctx_t)) {
		SS_DBG("arg_size != sizeof(crypto_ecc_req_ctx_t)\n");
		return -EINVAL;
	}

	ce_dev_lock();
	ecc_req_ctx = kzalloc(sizeof(crypto_ecc_req_ctx_t), GFP_KERNEL);
	if (!ecc_req_ctx) {
		SS_ERR("kzalloc ecc_req_ctx fail\n");
		return -ENOMEM;
	}

	ret = copy_from_user(ecc_req_ctx, (crypto_ecc_req_ctx_t *)arg,
			     sizeof(crypto_ecc_req_ctx_t));
	if (ret) {
		SS_ERR("copy_from_user fail\n");
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		return ret;
	}
	usr_dst_addr = (phys_addr_t)ecc_req_ctx->dst_buffer;
	ret = sunxi_copy_from_user(&ecc_req_ctx->dst_buffer,
				   ecc_req_ctx->dst_length);
	if (ret) {
		SS_ERR("dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		return ret;
	};

	ret = sunxi_copy_from_user(&ecc_req_ctx->iv_buffer,
				   ecc_req_ctx->iv_length);
	if (ret) {
		SS_ERR("iv_buffer copy_from_user fail\n");
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		return ret;
	};

	ret = sunxi_copy_from_user(&ecc_req_ctx->key_buffer,
				   ecc_req_ctx->key_length);
	if (ret) {
		SS_ERR("key_buffer copy_from_user fail\n");
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		return ret;
	};

	ret = sunxi_copy_from_user(&ecc_req_ctx->src_buffer,
				   ecc_req_ctx->src_length);
	if (ret) {
		SS_ERR("src_buffer copy_from_user fail\n");
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		return ret;
	};

	SS_DBG("do_ecc_crypto start\n");
	ret = do_ecc_crypto(ecc_req_ctx);
	if (ret) {
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		SS_ERR("do_ecc_crypto fail\n");
		return ret;
	}

	ret = copy_to_user((u8 *)usr_dst_addr, ecc_req_ctx->dst_buffer,
			   ecc_req_ctx->dst_length);
	if (ret) {
		SS_ERR(" dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
		return ret;
	}

	ce_release_resources((void *)ecc_req_ctx, CE_IOC_ECC_CRYPTO);
	return 0;
}

static int ioctl_hash_crypto(unsigned int cmd, unsigned long arg)
{
	crypto_hash_req_ctx_t *hash_req_ctx;
	int ret;
	phys_addr_t usr_dst_addr;
	SS_DBG("arg_size = 0x%x\n", _IOC_SIZE(cmd));
	if (_IOC_SIZE(cmd) != sizeof(crypto_hash_req_ctx_t)) {
		SS_DBG("arg_size != sizeof(crypto_rsa_req_ctx_t)\n");
		return -EINVAL;
	}

	ce_dev_lock();
	hash_req_ctx = kzalloc(sizeof(crypto_hash_req_ctx_t), GFP_KERNEL);
	if (!hash_req_ctx) {
		SS_ERR("kzalloc hash_req_ctx fail\n");
		return -ENOMEM;
	}

	ret = copy_from_user(hash_req_ctx, (crypto_hash_req_ctx_t *)arg,
			     sizeof(crypto_hash_req_ctx_t));
	if (ret) {
		SS_ERR("copy_from_user fail\n");
		ce_release_resources((void *)hash_req_ctx, CE_IOC_HASH_CRYPTO);
		return ret;
	}
	usr_dst_addr = (phys_addr_t)(uintptr_t)hash_req_ctx->dst_buffer;
	ret = sunxi_copy_from_user(&hash_req_ctx->dst_buffer,
				   hash_req_ctx->dst_length);
	if (ret) {
		SS_ERR("dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)hash_req_ctx, CE_IOC_HASH_CRYPTO);
		return ret;
	}
	if (hash_req_ctx->ion_flag) {
		;
	} else {
		ret = sunxi_copy_from_user(&hash_req_ctx->text_buffer,
					   hash_req_ctx->text_length);
		if (ret) {
			SS_ERR("text_buffer copy_from_user fail\n");
			ce_release_resources((void *)hash_req_ctx,
					     CE_IOC_HASH_CRYPTO);
			return ret;
		}
	}
	if (hash_req_ctx->key_length) {
		ret = sunxi_copy_from_user(&hash_req_ctx->key_buffer,
					   hash_req_ctx->key_length);
		if (ret) {
			SS_ERR("hash_buffer copy_from_user fail\n");
			ce_release_resources((void *)hash_req_ctx,
					     CE_IOC_HASH_CRYPTO);
			return ret;
		}
	}

	if (hash_req_ctx->iv_length) {
		ret = sunxi_copy_from_user(&hash_req_ctx->iv_buffer,
					   hash_req_ctx->iv_length);
		if (ret) {
			SS_ERR("iv_buffer copy_from_user fail\n");
			ce_release_resources((void *)hash_req_ctx,
					     CE_IOC_HASH_CRYPTO);
			return ret;
		}
	}

	SS_DBG("do_hash_crypto start\n");
	ret = do_hash_crypto(hash_req_ctx);
	if (ret) {
		ce_release_resources((void *)hash_req_ctx, CE_IOC_HASH_CRYPTO);
		SS_ERR("do_hash_crypto fail\n");
		return ret;
	}

	ret = copy_to_user((u8 *)(uintptr_t)usr_dst_addr, hash_req_ctx->dst_buffer,
			   hash_req_ctx->dst_length);
	if (ret) {
		SS_ERR(" dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)hash_req_ctx, CE_IOC_HASH_CRYPTO);
		return ret;
	}

	ce_release_resources((void *)hash_req_ctx, CE_IOC_HASH_CRYPTO);

	return 0;
}

static int ioctl_rng_crypto(unsigned int cmd, unsigned long arg)
{
	crypto_rng_req_ctx_t *rng_req_ctx;
	int ret;
	phys_addr_t usr_key_addr;
	phys_addr_t usr_dst_addr;

	SS_DBG("arg_size = 0x%x\n", _IOC_SIZE(cmd));
	if (_IOC_SIZE(cmd) != sizeof(crypto_rng_req_ctx_t)) {
		SS_DBG("arg_size != sizeof(crypto_rng_req_ctx_t)\n");
		return -EINVAL;
	}

	ce_dev_lock();

	rng_req_ctx = kzalloc(sizeof(crypto_rng_req_ctx_t), GFP_KERNEL);
	if (!rng_req_ctx) {
		SS_ERR("kzalloc rng_req_ctx fail\n");
		return -ENOMEM;
	}

	ret = copy_from_user(rng_req_ctx, (crypto_rng_req_ctx_t *)arg,
			sizeof(crypto_rng_req_ctx_t));
	if (ret) {
		SS_ERR("copy_from_user fail\n");
		ce_release_resources((void *)rng_req_ctx, CE_IOC_RNG_CRYPTO);
		return ret;
	}

	usr_key_addr = (phys_addr_t)(uintptr_t)rng_req_ctx->key_buffer;
	usr_dst_addr = (phys_addr_t)(uintptr_t)rng_req_ctx->dst_buffer;
	SS_DBG("usr_key_addr = 0x%px usr_dst_addr = 0x%px\n",
			(void *)(uintptr_t)usr_key_addr, (void *)(uintptr_t)usr_dst_addr);

	ret = sunxi_copy_from_user(&rng_req_ctx->key_buffer, rng_req_ctx->key_length);
	if (ret) {
		SS_ERR("key_buffer copy_from_user fail\n");
		ce_release_resources((void *)rng_req_ctx, CE_IOC_RNG_CRYPTO);
		return ret;
	}
	SS_DBG("rng_req_ctx->key_buffer = 0x%px\n", rng_req_ctx->key_buffer);

	ret = sunxi_copy_from_user(&rng_req_ctx->dst_buffer, rng_req_ctx->dst_length);
	if (ret) {
		SS_ERR("dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)rng_req_ctx, CE_IOC_RNG_CRYPTO);
		return ret;
	}

	SS_DBG("do_rng_crypto start\n");
	ret = do_rng_crypto(rng_req_ctx);
	if (ret) {
		ce_release_resources((void *)rng_req_ctx, CE_IOC_RNG_CRYPTO);
		SS_ERR("do_rng_crypto fail\n");
		return ret;
	}

	ret = copy_to_user((u8 *)(uintptr_t)usr_dst_addr, rng_req_ctx->dst_buffer, rng_req_ctx->dst_length);
	if (ret) {
		SS_ERR(" dst_buffer copy_from_user fail\n");
		ce_release_resources((void *)rng_req_ctx, CE_IOC_RNG_CRYPTO);
		return ret;
	}

	ce_release_resources((void *)rng_req_ctx, CE_IOC_RNG_CRYPTO);

	return 0;
}
#endif

static long sunxi_ce_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int channel_id;
	int ret;

	SS_DBG("cmd = 0x%x\n", cmd);
	switch (cmd) {
	case CE_IOC_REQUEST:
	{
		channel_id = sunxi_ce_channel_request();
		if (channel_id < 0) {
			SS_ERR("Failed to sunxi_ce_channel_request\n");
			return -EAGAIN;
		}
		SS_DBG("channel_id = %d\n", channel_id);
		ret = put_user(channel_id, (int *)arg);
		if (ret < 0) {
			SS_ERR("Failed to put_user\n");
			return ret;
		}

		break;
	}
	case CE_IOC_FREE:
	{
		ret = get_user(channel_id, (int *)arg);
		if (ret < 0) {
			SS_ERR("Failed to get_user\n");
			return ret;
		}
		sunxi_ce_channel_free(channel_id);

		break;
	}
	case CE_IOC_AES_CRYPTO:
	{
		ret = ioctl_aes_crypto(CE_IOC_AES_CRYPTO, arg);
		if (ret < 0) {
			SS_ERR("aes crypto failed\n");
			return ret;
		}
		break;
	}
#ifdef SS_SUPPORT_CE_V5
	case CE_IOC_RSA_CRYPTO:
	{
		ret = ioctl_rsa_crypto(CE_IOC_RSA_CRYPTO, arg);
		if (ret < 0) {
			SS_ERR("rsa crypto failed\n");
			return ret;
		}
		break;
	}
	case CE_IOC_ECC_CRYPTO:
	{
		ret = ioctl_ecc_crypto(CE_IOC_ECC_CRYPTO, arg);
		if (ret < 0) {
			SS_ERR("ecc crypto failed\n");
			return ret;
		}
		break;
	}
	case CE_IOC_HASH_CRYPTO:
	{
		ret = ioctl_hash_crypto(CE_IOC_HASH_CRYPTO, arg);
		if (ret < 0) {
			SS_ERR("hash crypto failed\n");
			return ret;
		}
		break;
	}
	case CE_IOC_RNG_CRYPTO:
	{
		ret = ioctl_rng_crypto(CE_IOC_RNG_CRYPTO, arg);
		if (ret < 0) {
			SS_ERR("rng crypto failed\n");
			return ret;
		}
		break;
	}
#endif
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct file_operations ce_fops = {
	.owner          = THIS_MODULE,
	.open           = sunxi_ce_open,
	.release        = sunxi_ce_release,
	.write          = sunxi_ce_write,
	.read           = sunxi_ce_read,
	.unlocked_ioctl = sunxi_ce_ioctl,
	.mmap           = sunxi_ce_mmap,
};


static int sunxi_ce_setup_cdev(void)
{
	int ret = 0;

	if (ce_cdev)
		kfree(ce_cdev);
	ce_cdev = kzalloc(sizeof(sunxi_ce_cdev_t), GFP_KERNEL);
	if (!ce_cdev) {
		SS_ERR("kzalloc fail\n");
		return -ENOMEM;
	}

	/* alloc devid */
	ret = alloc_chrdev_region(&ce_cdev->devid, 0, 1, SUNXI_SS_DEV_NAME);
	if (ret < 0) {
		SS_ERR("alloc_chrdev_region fail\n");
		kfree(ce_cdev);
		return -EINVAL;
	}

	SS_DBG("ce_cdev->devid = %d\n", ce_cdev->devid & 0xfff00000);
	/* alloc pcdev */
	ce_cdev->pcdev = cdev_alloc();
	if (!ce_cdev->pcdev) {
		SS_ERR("cdev_alloc fail\n");
		ret = -ENOMEM;
		goto err0;
	}

	/* bing ce_fops */
	cdev_init(ce_cdev->pcdev, &ce_fops);
	ce_cdev->pcdev->owner = THIS_MODULE;

	/* register cdev */
	ret = cdev_add(ce_cdev->pcdev, ce_cdev->devid, 1);
	if (ret) {
		SS_ERR("cdev_add fail\n");
		ret = -1;
		goto err0;
	}

	/* create device note */
	ce_cdev->pclass = class_create(THIS_MODULE, SUNXI_SS_DEV_NAME);
	if (IS_ERR(ce_cdev->pclass)) {
		SS_ERR("class_create fail\n");
		ret = -1;
		goto err0;
	}
	ce_cdev->pdevice = device_create(ce_cdev->pclass, NULL, ce_cdev->devid, NULL,
						SUNXI_SS_DEV_NAME);

#if defined(SS_SUPPORT_CE_V5)
	/* init task_pool */
	ce_cdev->task_pool = dma_pool_create("task_pool", ce_cdev->pdevice,
			sizeof(struct ce_task_desc), 4, 0);
	if (ce_cdev->task_pool == NULL)
		return -ENOMEM;
#endif
#ifdef CONFIG_OF
	ce_cdev->pdevice->dma_mask = &sunxi_ss_dma_mask;
	ce_cdev->pdevice->coherent_dma_mask = DMA_BIT_MASK(64);
#endif
	memcpy(ce_cdev->dev_name, SUNXI_SS_DEV_NAME, 3);

	return 0;

err0:
	if (ce_cdev)
		kfree(ce_cdev);
	unregister_chrdev_region(ce_cdev->devid, 1);
	return ret;

}

static int sunxi_ce_exit_cdev(void)
{
	if (ce_cdev->task_pool)
		dma_pool_destroy(ce_cdev->task_pool);

	device_destroy(ce_cdev->pclass, ce_cdev->devid);
	class_destroy(ce_cdev->pclass);

	unregister_chrdev_region(ce_cdev->devid, 1);

	cdev_del(ce_cdev->pcdev);

	kfree(ce_cdev);

	return 0;
}

static int __init sunxi_ce_module_init(void)
{
	int ret = 0;

	SS_DBG("sunxi_ce_cdev_init\n");
	ret = sunxi_ce_setup_cdev();
	if (ret < 0) {
		SS_ERR("sunxi_ce_setup_cdev() failed, return %d\n", ret);
		return ret;
	}

	ret = sunxi_ce_hw_init(ce_cdev);
	if (ret < 0) {
		SS_ERR("sunxi_ce_hw_init failed, return %d\n", ret);
		goto err0;
	}

#if IS_ENABLED(CONFIG_AW_HWRNG_DRIVER)
	ret = sunxi_register_hwrng(ce_cdev);
	if (ret < 0) {
		SS_ERR("sunxi_register_hwrng failed, return %d\n", ret);
		goto err1;
	}
#endif

	spin_lock_init(&ce_cdev->lock);
	return ret;

#if IS_ENABLED(CONFIG_AW_HWRNG_DRIVER)
err1:
	sunxi_ce_hw_exit();
#endif
err0:
	sunxi_ce_exit_cdev();

	return ret;
}

static void __exit sunxi_ce_module_exit(void)
{
	SS_DBG("sunxi_ce_module_exit\n");
	sunxi_ce_exit_cdev();
	sunxi_ce_hw_exit();
}

module_init(sunxi_ce_module_init);
module_exit(sunxi_ce_module_exit);

MODULE_AUTHOR("mintow");
MODULE_VERSION("1.1.6");
MODULE_DESCRIPTION("SUNXI CE Controller Driver");
MODULE_ALIAS("platform:"SUNXI_SS_DEV_NAME);
MODULE_LICENSE("GPL");

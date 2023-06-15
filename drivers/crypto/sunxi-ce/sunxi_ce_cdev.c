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
#include <linux/clk/sunxi.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>

#include "sunxi_ce_cdev.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>


sunxi_ce_cdev_t	*ce_cdev;

static DEFINE_MUTEX(ss_lock);

void ce_dev_lock(void)
{
	mutex_lock(&ss_lock);
}

void ce_dev_unlock(void)
{
	mutex_unlock(&ss_lock);
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
}

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

#ifdef CONFIG_OF
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
		return -1;
	}

	*base = of_iomap(*pnode, 0); /* reg[0] must be accessible. */
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
	struct clk *pclk = NULL;
	u32 gen_clkrate;

	ret = sunxi_get_ce_device_node(&p_cdev->pnode, &p_cdev->base_addr, SUNXI_CE_DEV_NODE_NAME);
	if (ret) {
		SS_ERR("sunxi_get_ce_device_node fail, return %x\n", ret);
		return -1;
	}

	/*get periph0 clk reg*/
	pclk = of_clk_get(p_cdev->pnode, 1);
	if (IS_ERR_OR_NULL(pclk)) {
		SS_ERR("Unable to get pll clock, return %x\n", PTR_RET(pclk));
		return PTR_RET(pclk);
	}

	/*get ce clk reg*/
	p_cdev->ce_clk = of_clk_get(p_cdev->pnode, 0);
	if (IS_ERR_OR_NULL(p_cdev->ce_clk)) {
		SS_ERR("Fail to get module clk, ret %x\n", PTR_RET(p_cdev->ce_clk));
		return PTR_RET(p_cdev->ce_clk);
	}

	/*get ce need configure clk*/
	if (of_property_read_u32(p_cdev->pnode, "clock-frequency", &gen_clkrate)) {

		SS_ERR("Unable to get clock-frequency.\n");
		return -EINVAL;
	}
	SS_DBG("The clk freq: %d\n", gen_clkrate);

	p_cdev->reset = devm_reset_control_get(p_cdev->pdevice, NULL);
	if (IS_ERR(p_cdev->reset)) {
		SS_ERR("Fail to get reset clk, ret %x\n", PTR_RET(p_cdev->reset));
		return PTR_RET(p_cdev->reset);
	}

	/*configure ce clk form periph0*/
#ifdef CONFIG_EVB_PLATFORM
	ret = clk_set_parent(p_cdev->ce_clk, pclk);
	if (ret != 0) {
		SS_ERR("clk_set_parent() failed! return %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(p_cdev->ce_clk, gen_clkrate);
	if (ret != 0) {
		SS_ERR("Set rate(%d) failed! ret %d\n", gen_clkrate, ret);
		return ret;
	}
#endif
	SS_DBG("SS mclk %luMHz, pclk %luMHz\n", clk_get_rate(p_cdev->ce_clk)/1000000,
			clk_get_rate(pclk)/1000000);

	/*enable ce clk*/
	if (clk_prepare_enable(p_cdev->ce_clk)) {
		SS_ERR("Couldn't enable module clock\n");
		return -EBUSY;
	}

	clk_put(pclk);

	sunxi_ce_res_request(p_cdev);

	return 0;
}

static int sunxi_ce_hw_exit(void)
{
	clk_disable_unprepare(ce_cdev->ce_clk);
	clk_put(ce_cdev->ce_clk);
	ce_cdev->ce_clk = NULL;
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
		return -EAGAIN;
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
	SS_DBG("The flow %d is available.\n", id);
	spin_unlock_irqrestore(&ce_cdev->lock, flags);
}

static long sunxi_ce_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	crypto_aes_req_ctx_t *aes_req_ctx;
	int channel_id = -1;
	int ret;
	phys_addr_t usr_key_addr;
	phys_addr_t usr_dst_addr;

	SS_DBG("cmd = %u\n", cmd);
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
			return -EAGAIN;
		}

		break;
	}

	case CE_IOC_FREE:
	{
		ret = get_user(channel_id, (int *)arg);
		if (ret < 0) {
			SS_ERR("Failed to get_user\n");
			return -EAGAIN;
		}
		sunxi_ce_channel_free(channel_id);

		break;
	}

	case CE_IOC_AES_CRYPTO:
	{
		SS_DBG("arg_size = %d\n", _IOC_SIZE(cmd));
		if (_IOC_SIZE(cmd) != sizeof(crypto_aes_req_ctx_t)) {
			SS_DBG("arg_size != sizeof(crypto_aes_req_ctx_t)\n");
			return -EINVAL;
		}

		aes_req_ctx = kzalloc(sizeof(crypto_aes_req_ctx_t), GFP_KERNEL);
		if (!aes_req_ctx) {
			SS_ERR("kzalloc aes_req_ctx fail\n");
			return -ENOMEM;
		}

		ret = copy_from_user(aes_req_ctx, (crypto_aes_req_ctx_t *)arg,
				sizeof(crypto_aes_req_ctx_t));
		if (ret) {
			SS_ERR("copy_from_user fail\n");
			kfree(aes_req_ctx);
			return -EAGAIN;
		}
		usr_key_addr = (phys_addr_t)aes_req_ctx->key_buffer;
		usr_dst_addr = (phys_addr_t)aes_req_ctx->dst_buffer;
		SS_DBG("usr_key_addr = 0x%px usr_dst_addr = 0x%px\n",
				(void *)usr_key_addr, (void *)usr_dst_addr);
		ret = sunxi_copy_from_user(&aes_req_ctx->key_buffer, aes_req_ctx->key_length);
		if (ret) {
			SS_ERR("key_buffer copy_from_user fail\n");
			return -EAGAIN;
		}

		SS_DBG("aes_req_ctx->key_buffer = 0x%px\n", aes_req_ctx->key_buffer);
		ret = sunxi_copy_from_user(&aes_req_ctx->iv_buf, aes_req_ctx->iv_length);
		if (ret) {
			SS_ERR("iv_buffer copy_from_user fail\n");
			return -EAGAIN;
		}

		if (aes_req_ctx->ion_flag) {
			SS_DBG("src_phy = 0x%lx, dst_phy = 0x%lx\n", aes_req_ctx->src_phy, aes_req_ctx->dst_phy);
		} else {
			ret = sunxi_copy_from_user(&aes_req_ctx->src_buffer, aes_req_ctx->src_length);
			if (ret) {
				SS_ERR("src_buffer copy_from_user fail\n");
				return -EAGAIN;
			}

			ret = sunxi_copy_from_user(&aes_req_ctx->dst_buffer, aes_req_ctx->dst_length);
			if (ret) {
				SS_ERR("dst_buffer copy_from_user fail\n");
				return -EAGAIN;
			}
		}

		SS_ERR("do_aes_crypto start\n");
		ret = do_aes_crypto(aes_req_ctx);
		if (ret) {
			kfree(aes_req_ctx);
			SS_ERR("do_aes_crypto fail\n");
			return -3;
		}

		if (aes_req_ctx->ion_flag) {
		} else {
			ret = copy_to_user((u8 *)usr_dst_addr, aes_req_ctx->dst_buffer, aes_req_ctx->dst_length);
			if (ret) {
				SS_ERR(" dst_buffer copy_from_user fail\n");
				kfree(aes_req_ctx);
				return -EAGAIN;
			}
		}
		break;
	}

	default:
		return -EINVAL;
		break;

	}
	return 0;
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

	ce_cdev = kzalloc(sizeof(sunxi_ce_cdev_t), GFP_KERNEL);
	if (!ce_cdev) {
		SS_ERR("kzalloc fail\n");
		return -ENOMEM;
	}

	/*alloc devid*/
	ret = alloc_chrdev_region(&ce_cdev->devid, 0, 1, SUNXI_SS_DEV_NAME);
	if (ret < 0) {
		SS_ERR("alloc_chrdev_region fail\n");
		kfree(ce_cdev);
		return -1;
	}

	SS_DBG("ce_cdev->devid = %d\n", ce_cdev->devid & 0xfff00000);
	/*alloc pcdev*/
	ce_cdev->pcdev = cdev_alloc();
	if (!ce_cdev->pcdev) {
		SS_ERR("cdev_alloc fail\n");
		ret = -ENOMEM;
		goto err0;
	}

	/*bing ce_fops*/
	cdev_init(ce_cdev->pcdev, &ce_fops);
	ce_cdev->pcdev->owner = THIS_MODULE;

	/*register cdev*/
	ret = cdev_add(ce_cdev->pcdev, ce_cdev->devid, 1);
	if (ret) {
		SS_ERR("cdev_add fail\n");
		ret = -1;
		goto err0;
	}

	/*create device note*/
	ce_cdev->pclass = class_create(THIS_MODULE, SUNXI_SS_DEV_NAME);
	if (IS_ERR(ce_cdev->pclass)) {
		SS_ERR("class_create fail\n");
		ret = -1;
		goto err0;
	}
	ce_cdev->pdevice = device_create(ce_cdev->pclass, NULL, ce_cdev->devid, NULL,
						SUNXI_SS_DEV_NAME);

	/*init task_pool*/
	ce_cdev->task_pool = dma_pool_create("task_pool", ce_cdev->pdevice,
			sizeof(struct ce_task_desc), 4, 0);
	if (ce_cdev->task_pool == NULL)
		return -ENOMEM;

#ifdef CONFIG_OF
	ce_cdev->pdevice->dma_mask = &sunxi_ss_dma_mask;
	ce_cdev->pdevice->coherent_dma_mask = DMA_BIT_MASK(32);
#endif

	return 0;

err0:
	if (ce_cdev)
		kfree(ce_cdev);
	unregister_chrdev_region(ce_cdev->devid, 1);
	return ret;

}

static int sunxi_ce_exit_cdev(void)
{
	device_destroy(ce_cdev->pclass, ce_cdev->devid);
	class_destroy(ce_cdev->pclass);

	unregister_chrdev_region(ce_cdev->devid, 1);

	cdev_del(ce_cdev->pcdev);
	kfree(ce_cdev);

	if (ce_cdev->task_pool)
		dma_pool_destroy(ce_cdev->task_pool);

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
		return ret;
	}
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
MODULE_DESCRIPTION("SUNXI CE Controller Driver");
MODULE_ALIAS("platform:"SUNXI_SS_DEV_NAME);
MODULE_LICENSE("GPL");

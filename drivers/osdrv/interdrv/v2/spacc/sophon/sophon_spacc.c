// SPDX-License-Identifier: GPL-3.0-or-later

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/version.h>
#include <linux/compat.h>

#include <linux/init.h>
#include <linux/dma-buf.h>

#include <linux/arm-smccc.h>
#include <linux/uaccess.h>

#include <linux/dma-direction.h>
#include <linux/dma-map-ops.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>

#include "sophon_spacc.h"

#define DEVICE_NAME "spacc"

#define OPTEE_SMC_CALL_CV_BASE64 0x0300000D

struct cvi_spacc_device {
	struct device *dev;
	void __iomem *spacc_base;
	int spacc_major;
	struct class *spacc_class;

	void *pool;
	u32 pool_size;
	u32 data_size;
	u32 read_size;

	// for sha256/sha1
	u32 state[8];
};

static struct cvi_spacc_device g_spacc_dev = { 0 };

static int cvi_spacc_create_pool(unsigned int size)
{
	if (g_spacc_dev.pool_size)
		devm_kfree(g_spacc_dev.dev, g_spacc_dev.pool);

	g_spacc_dev.pool = devm_kzalloc(g_spacc_dev.dev, size, GFP_KERNEL);
	if (!g_spacc_dev.pool) {
		g_spacc_dev.pool_size = 0;
		return -ENOMEM;
	}

	g_spacc_dev.pool_size = size;
	return 0;
}

static int cvi_spacc_base64(u32 customer_code, u32 action)
{
	struct arm_smccc_res res = { 0 };
	phys_addr_t value_phys;

	value_phys = virt_to_phys(g_spacc_dev.pool);
	//for gcc 9.3.0
	arch_sync_dma_for_device(value_phys, g_spacc_dev.data_size,
				 DMA_TO_DEVICE);
#if 0
	__dma_map_area(phys_to_virt(value_phys), g_spacc_dev.data_size, DMA_TO_DEVICE);
#endif

	arm_smccc_smc(OPTEE_SMC_CALL_CV_BASE64, (unsigned long)value_phys,
		      g_spacc_dev.data_size, (unsigned long)value_phys,
		      customer_code, action, 0, 0, &res);
	pr_debug("res a0 : %lu\n", res.a0);

	return res.a0;
}

static int cvi_spacc_base64_inner(struct cvi_spacc_base64_inner *b64)
{
	struct arm_smccc_res res = { 0 };
	arch_sync_dma_for_device(b64->src, b64->len, DMA_TO_DEVICE);
	// __dma_map_area(phys_to_virt(b64->src), b64->len, DMA_TO_DEVICE);

	arm_smccc_smc(OPTEE_SMC_CALL_CV_BASE64, b64->src, b64->len, b64->dst,
		      b64->customer_code, b64->action, 0, 0, &res);
	arch_sync_dma_for_device(b64->dst, res.a0, DMA_FROM_DEVICE);
	// __dma_map_area(phys_to_virt(b64->dst), res.a0, DMA_FROM_DEVICE);
	pr_debug("res a0 : %lu\n", res.a0);

	return res.a0;
}

static int cvi_spacc_aes(spacc_exec_config *config)
{
	struct arm_smccc_res res = { 0 };
	phys_addr_t src_phys;
	phys_addr_t key_phys, iv_phys;
	void *key_kernel_addr, *iv_kernel_addr;

	int ret;
	uint64_t key_len;
	uint64_t arg7 = ((uint64_t)config->algo << 48) |
			((uint64_t)config->mode << 32) |
			((uint64_t)config->key_mode << 16) |
			((uint64_t)config->otp << 4 | (uint64_t)config->action);
	switch (config->key_mode) {
	case AES_128BIT:
		key_len = 16;
		break;
	case AES_192BIT:
		key_len = 24;
		break;
	case AES_256BIT:
		key_len = 32;
		break;
	}
	if (config->otp != USE_OTP_KEY) {
		key_kernel_addr = kmalloc(key_len, GFP_KERNEL);
		if (copy_from_user(key_kernel_addr, config->key, key_len) !=
		    0) {
			pr_err("copy_from_user key failed !");
			return -EFAULT;
		}
		key_phys = virt_to_phys(key_kernel_addr);
	}
	if (config->mode != AES_ECB) {
		iv_kernel_addr = kmalloc(16, GFP_KERNEL);
		if (copy_from_user(iv_kernel_addr, config->iv, 16) != 0) {
			pr_err("copy_from_user iv failed !");
			return -EFAULT;
		}
		iv_phys = virt_to_phys(iv_kernel_addr);
	}

	src_phys = virt_to_phys(g_spacc_dev.pool);

	arch_sync_dma_for_device(src_phys, g_spacc_dev.data_size,
				 DMA_TO_DEVICE);
	arch_sync_dma_for_device(iv_phys, 16, DMA_TO_DEVICE);
	arch_sync_dma_for_device(key_phys, key_len, DMA_TO_DEVICE);

	arm_smccc_smc(OPTEE_SMC_CALL_CV_SPACC_EXEC, (unsigned long)src_phys, (unsigned long)src_phys,
		      g_spacc_dev.data_size, (unsigned long)key_phys,
		      (unsigned long)iv_phys, key_len, arg7, &res);
	arch_sync_dma_for_device(src_phys, g_spacc_dev.data_size,
				 DMA_FROM_DEVICE);

	pr_err("result len : %lu\n", res.a0);
	return res.a0;
}

static int spacc_open(struct inode *inode, struct file *file)
{
	g_spacc_dev.read_size = 0;
	g_spacc_dev.data_size = 0;
	return 0;
}

static ssize_t spacc_read(struct file *filp, char *buf, size_t count,
			  loff_t *f_pos)
{
	int ret;

	if (!g_spacc_dev.data_size)
		return 0;

	ret = g_spacc_dev.data_size - g_spacc_dev.read_size;
	count = (ret >= count) ? count : ret;
	ret = copy_to_user(
		buf, (char *)g_spacc_dev.pool + g_spacc_dev.read_size, count);
	if (ret != 0)
		return -1;

	g_spacc_dev.read_size += count;
	if (g_spacc_dev.data_size == g_spacc_dev.read_size)
		g_spacc_dev.data_size = 0;

	return count;
}

static ssize_t spacc_write(struct file *filp, const char *buf, size_t count,
			   loff_t *f_pos)
{
	int ret;

	if (!g_spacc_dev.pool_size)
		return -ENOMEM;

	ret = g_spacc_dev.pool_size - g_spacc_dev.data_size;
	count = (ret >= count) ? count : ret;

	ret = copy_from_user((char *)g_spacc_dev.pool + g_spacc_dev.data_size,
			     buf, count);
	if (ret != 0)
		return -1;

	g_spacc_dev.data_size += count;
	return count;
}

static int spacc_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long spacc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;

	switch (cmd) {
	case IOCTL_SPACC_CREATE_POOL: {
		unsigned int size = 0;

		ret = copy_from_user((unsigned char *)&size,
				     (unsigned char *)arg, sizeof(size));
		if (ret != 0)
			return -1;

		ret = cvi_spacc_create_pool(size);
		if (ret != 0)
			return -1;

		break;
	}
	case IOCTL_SPACC_GET_POOL_SIZE: {
		ret = copy_to_user((unsigned char *)arg,
				   (unsigned char *)&g_spacc_dev.pool_size,
				   sizeof(g_spacc_dev.pool_size));
		if (ret != 0)
			return -1;

		break;
	}
	case IOCTL_SPACC_BASE64: {
		struct cvi_spacc_base64 b64 = { 0 };
		u32 padding_size = 0;

		ret = copy_from_user((unsigned char *)&b64,
				     (unsigned char *)arg, sizeof(b64));
		if (ret != 0)
			return -1;

		if (!b64.action) {
			char *buf = g_spacc_dev.pool;

			if (buf[g_spacc_dev.data_size - 1] == '=')
				padding_size++;
			if (buf[g_spacc_dev.data_size - 2] == '=')
				padding_size++;
		}

		ret = cvi_spacc_base64(b64.customer_code, b64.action);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}

		if (!b64.action)
			ret -= padding_size;

		g_spacc_dev.data_size = ret;
		g_spacc_dev.read_size = 0;
		break;
	}
	case IOCTL_SPACC_BASE64_INNER: {
		struct cvi_spacc_base64_inner b64;

		ret = copy_from_user((unsigned char *)&b64,
				     (unsigned char *)arg, sizeof(b64));
		if (ret != 0)
			return -1;

		ret = cvi_spacc_base64_inner(&b64);
		break;
	}
	case IOCTL_SPACC_AES_ACTION: {
		spacc_exec_config config = { 0 };

		if (copy_from_user(&config, (spacc_exec_config __user *)arg,
				   sizeof(spacc_exec_config))) {
			return -EFAULT;
		}

		ret = cvi_spacc_aes(&config);
		if (ret < 0) {
			pr_err("plat_cryptodma_do failed\n");
			return -1;
		}
		g_spacc_dev.data_size = ret;
		g_spacc_dev.read_size = 0;

		break;
	}
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long space_compat_ptr_ioctl(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd,
					  (unsigned long)compat_ptr(arg));
}
#endif

const struct file_operations spacc_fops = {
	.owner = THIS_MODULE,
	.open = spacc_open,
	.read = spacc_read,
	.write = spacc_write,
	.release = spacc_release,
	.unlocked_ioctl = spacc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = space_compat_ptr_ioctl,
#endif
};

static int cvitek_spacc_drv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	g_spacc_dev.dev = dev;
	g_spacc_dev.spacc_major = register_chrdev(0, DEVICE_NAME, &spacc_fops);
	if (g_spacc_dev.spacc_major < 0) {
		pr_err("can't register major number\n");
		goto failed;
	}

	g_spacc_dev.spacc_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(g_spacc_dev.spacc_class)) {
		pr_err("Err: failed in My Driver class.\n");
		goto failed;
	}

	device_create(g_spacc_dev.spacc_class, NULL,
		      MKDEV(g_spacc_dev.spacc_major, 0), NULL, DEVICE_NAME);
	return ret;
failed:
	if (g_spacc_dev.spacc_major > 0)
		unregister_chrdev(g_spacc_dev.spacc_major, DEVICE_NAME);
	return -ENOMEM;
}

static int cvitek_spacc_drv_remove(struct platform_device *pdev)
{
	device_destroy(g_spacc_dev.spacc_class,
		       MKDEV(g_spacc_dev.spacc_major, 0));
	class_destroy(g_spacc_dev.spacc_class);
	unregister_chrdev(g_spacc_dev.spacc_major, DEVICE_NAME);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cvitek_spacc_of_match[] = {
	{
		.compatible = "cvitek,spacc",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cvitek_spacc_of_match);
#endif

static struct platform_driver cvitek_spacc_driver = {
	.probe		= cvitek_spacc_drv_probe,
	.remove		= cvitek_spacc_drv_remove,
	.driver		= {
		.name	= "cvitek_spacc",
		.of_match_table = of_match_ptr(cvitek_spacc_of_match),
	},
};

module_platform_driver(cvitek_spacc_driver);

MODULE_AUTHOR("Sophon");
MODULE_DESCRIPTION("Cvitek Spacc Driver");
MODULE_LICENSE("GPL");

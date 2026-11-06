/*
 * Bitmain SoCs Reset Controller driver
 *
 * Copyright (c) 2018 Bitmain Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/compat.h>

#include "cvi_wiegand.h"
#include "cvi_wiegand_ioctl.h"

#define WGN_MAX_NUM 3
static DECLARE_BITMAP(dev_mask, WGN_MAX_NUM);

#define CVI_WIEGAND_CDEV_NAME "cvi-wiegand"
#define CVI_WIEGAND_CLASS_NAME "cvi-wiegand"

struct class *wiegand_class;
static dev_t wiegand_cdev_id;

static char flag = 'n';
static DECLARE_WAIT_QUEUE_HEAD(wq);

static inline void mmio_clrsetbits_32(uintptr_t addr,
				      uint32_t clear,
				      uint32_t set)
{
	void *vaddr = ioremap(addr, 0x4);

	iowrite32((ioread32(vaddr) & ~clear) | set, vaddr);
}

static int platform_wiegand_clk_init(struct cvi_wiegand_device *ndev)
{
	//enable clock
	if (ndev->clk_wiegand) {
		pr_debug("wiegand enable clock\n");
		clk_prepare_enable(ndev->clk_wiegand);
		//clk_prepare_enable(ndev->clk_wiegand1);
		ndev->clk_wiegand_en = 1;
	}

	return 0;
}

static void platform_wiegand_clk_deinit(struct cvi_wiegand_device *ndev)
{
	//disable clock
	if (ndev->clk_wiegand) {
		pr_debug("wiegand disable clock\n");
		clk_disable_unprepare(ndev->clk_wiegand);
		//clk_disable_unprepare(ndev->clk_wiegand1);
		ndev->clk_wiegand_en = 0;
	}
}

static irqreturn_t cvi_wiegand_irq(int irq, void *data)
{
	struct cvi_wiegand_device *ndev = data;

	pr_debug("cvi_wiegand_irq\n");

	//clear irq
	writel(0x111, ndev->wiegand_vaddr + IRQ_CLEAR);

	flag = 'y';

	wake_up_interruptible(&wq);

	return IRQ_HANDLED;
}

static int cvi_wiegand_set_tx_cfg(struct cvi_wiegand_device *ndev, struct wgn_tx_cfg *tx_cfg_ptr)
{
	uint32_t value = 0;

	pr_debug("cvi_wiegand_set_tx_cfg\n");

	if (tx_cfg_ptr->tx_lowtime > 0xFFFFFF)
		tx_cfg_ptr->tx_lowtime = 0xFFFFFF;
	writel(tx_cfg_ptr->tx_lowtime, ndev->wiegand_vaddr + TX_CONFIG0);

	if (tx_cfg_ptr->tx_hightime > 0xFFFFFF)
		tx_cfg_ptr->tx_hightime = 0xFFFFFF;
	writel(tx_cfg_ptr->tx_hightime, ndev->wiegand_vaddr + TX_CONFIG1);

	if (tx_cfg_ptr->tx_bitcount > 128)
		tx_cfg_ptr->tx_bitcount = 128;
	if (tx_cfg_ptr->tx_msb1st > 1)
		tx_cfg_ptr->tx_msb1st = 1;
	if (tx_cfg_ptr->tx_opendrain > 1)
		tx_cfg_ptr->tx_opendrain = 1;

	value = (tx_cfg_ptr->tx_opendrain << 16) | (tx_cfg_ptr->tx_msb1st << 8) | tx_cfg_ptr->tx_bitcount;
	writel(value, ndev->wiegand_vaddr + TX_CONFIG2);

	return 0;
}

static int cvi_wiegand_set_rx_cfg(struct cvi_wiegand_device *ndev, struct wgn_rx_cfg *rx_cfg_ptr)
{
	uint32_t value = 0;

	pr_debug("cvi_wiegand_set_rx_cfg\n");

	if (rx_cfg_ptr->rx_debounce > 0xFFFF)
		rx_cfg_ptr->rx_debounce = 0xFFFF;
	writel(rx_cfg_ptr->rx_debounce, ndev->wiegand_vaddr + RX_CONFIG0);

	if (rx_cfg_ptr->rx_idle_timeout > 0xFFFFFFFF)
		rx_cfg_ptr->rx_idle_timeout = 0xFFFFFFFF;
	writel(rx_cfg_ptr->rx_idle_timeout, ndev->wiegand_vaddr + RX_CONFIG1);

	if (rx_cfg_ptr->rx_bitcount > 128)
		rx_cfg_ptr->rx_bitcount = 128;
	if (rx_cfg_ptr->rx_msb1st > 1)
		rx_cfg_ptr->rx_msb1st = 1;

	value = (rx_cfg_ptr->rx_msb1st << 8) | rx_cfg_ptr->rx_bitcount;
	writel(value, ndev->wiegand_vaddr + RX_CONFIG2);

	return 0;
}

static int cvi_wiegand_get_tx_cfg(struct cvi_wiegand_device *ndev, struct wgn_tx_cfg *tx_cfg_ptr)
{
	uint32_t value;

	pr_debug("cvi_wiegand_get_tx_cfg\n");

	value = readl(ndev->wiegand_vaddr + TX_CONFIG0);
	tx_cfg_ptr->tx_lowtime = value & 0xFFFFFF;
	value = readl(ndev->wiegand_vaddr + TX_CONFIG1);
	tx_cfg_ptr->tx_hightime = value & 0xFFFFFF;
	value = readl(ndev->wiegand_vaddr + TX_CONFIG2);
	tx_cfg_ptr->tx_bitcount = value & 0x7F;
	tx_cfg_ptr->tx_msb1st = (value & 0x100) >> 8;
	tx_cfg_ptr->tx_opendrain = (value & 0x10000) >> 16;

	return 0;
}

static int cvi_wiegand_get_rx_cfg(struct cvi_wiegand_device *ndev, struct wgn_rx_cfg *rx_cfg_ptr)
{
	uint32_t value;

	pr_debug("cvi_wiegand_get_rx_cfg\n");
	value = readl(ndev->wiegand_vaddr + RX_CONFIG0);
	rx_cfg_ptr->rx_debounce = value & 0xFFFF;
	value = readl(ndev->wiegand_vaddr + RX_CONFIG1);
	rx_cfg_ptr->rx_idle_timeout = value & 0xFFFFFFFF;
	value = readl(ndev->wiegand_vaddr + RX_CONFIG2);
	rx_cfg_ptr->rx_bitcount = value & 0x7F;
	rx_cfg_ptr->rx_msb1st = (value & 0x100) >> 8;

	return 0;
}

static int cvi_wiegand_tx(struct cvi_wiegand_device *ndev, unsigned long arg)
{
	u32 timeout_num = 0;
	pr_debug("cvi_wiegand_tx\n");
	pr_debug("low tx_data: %#X\n", (uint32_t)(ndev->tx_data));
	pr_debug("high tx_data: %#X\n", (uint32_t)(ndev->tx_data >> 32));

	writel((uint32_t)(ndev->tx_data), ndev->wiegand_vaddr + TX_BUFFER);
	writel((uint32_t)(ndev->tx_data >> 32), ndev->wiegand_vaddr + TX_BUFFER1);

	while (readl(ndev->wiegand_vaddr + TX_BUSY)) {
		if (timeout_num >= 100000) {
			pr_err("TX_BUSY\n");
			return -EBUSY;
		}
		timeout_num++;
		usleep_range(10, 11);
	}

	writel(1, ndev->wiegand_vaddr + TX_TRIGGER);
	return 0;
}

static int cvi_wiegand_rx(struct cvi_wiegand_device *ndev, unsigned long arg, int timeoutflag)
{
	uint32_t value;

	// Clear RX buffer
	writel(0x1, ndev->wiegand_vaddr + RX_BUFFER_CLEAR);

	// Enable RX receive interrupt
	writel(0x100, ndev->wiegand_vaddr + IRQ_ENABLE);
	writel(0x111, ndev->wiegand_vaddr + IRQ_CLEAR);

	// Enable RX
	value = readl(ndev->wiegand_vaddr + RX_CONFIG2);
	value |= 0x1000;
	writel(value, ndev->wiegand_vaddr + RX_CONFIG2);

	pr_info("cvi_wiegand_rx...\n");

	pr_debug("Scheduling Out\n");
	if (timeoutflag == 1) {
		if (wait_event_interruptible_timeout(wq, flag == 'y', 3*HZ) == 0)
			return 1;
	} else {
		wait_event_interruptible(wq, flag == 'y');
	}
	flag = 'n';
	pr_debug("Woken Up\n");

	return 0;
}

ssize_t cvi_wiegand_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	struct cvi_wiegand_device *ndev = filp->private_data;

	cvi_wiegand_rx(ndev, 0, 1);

	return 0;
}

#if 0
ssize_t cvi_wiegand_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	if (copy_from_user(&flag, buff, 1))
		return -EFAULT;

	pr_debug("cvi_wiegand_write\n");

	wake_up_interruptible(&wq);

	return count;
}
#endif

static long cvi_wiegand_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int settimeout = 0;
	struct cvi_wiegand_device *ndev = filp->private_data;
	long ret = 0;
	struct wgn_tx_cfg tx_cfg;
	struct wgn_rx_cfg rx_cfg;
	uint64_t tx_data;
	uint64_t rx_data;

	switch (cmd) {

	case IOCTL_WGN_SET_TX_CFG:
		if (copy_from_user(&tx_cfg, (void *)arg, sizeof(tx_cfg))) {
			pr_err("copy_from_user failed.\n");
			break;
		}
		ret = cvi_wiegand_set_tx_cfg(ndev, &tx_cfg);
		break;

	case IOCTL_WGN_SET_RX_CFG:
		if (copy_from_user(&rx_cfg, (void *)arg, sizeof(rx_cfg))) {
			pr_err("copy_from_user failed.\n");
			break;
		}
		ret = cvi_wiegand_set_rx_cfg(ndev, &rx_cfg);
		break;

	case IOCTL_WGN_GET_TX_CFG:
		memset(&tx_cfg, 0, sizeof(tx_cfg));
		ret = cvi_wiegand_get_tx_cfg(ndev, &tx_cfg);
		if (copy_to_user((uint32_t *) arg, &tx_cfg, sizeof(tx_cfg)))
			return -EFAULT;
		break;

	case IOCTL_WGN_GET_RX_CFG:
		memset(&rx_cfg, 0, sizeof(rx_cfg));
		ret = cvi_wiegand_get_rx_cfg(ndev, &rx_cfg);
		if (copy_to_user((uint32_t *) arg, &rx_cfg, sizeof(rx_cfg)))
			return -EFAULT;
		break;

	case IOCTL_WGN_TX:
		if (copy_from_user(&tx_data, (void *)arg, sizeof(tx_data))) {
			pr_err("copy_from_user failed.\n");
			break;
		}
		ndev->tx_data = tx_data;
		ret = cvi_wiegand_tx(ndev, arg);
		break;

	case IOCTL_WGN_RX:
		if (copy_from_user(&settimeout, (void *)arg, sizeof(settimeout))) {
			pr_err("copy_from_user failed.\n");
			break;
		}
		ret = cvi_wiegand_rx(ndev, arg, settimeout);
		break;

	case IOCTL_WGN_GET_VAL:
		//check data
		if ((readl(ndev->wiegand_vaddr + RX_BUFFER_VALID) & 0x1) == 0 ||
		    (readl(ndev->wiegand_vaddr + RX_BUFFER_FLAG) & 0x180) != 0) {
			pr_debug("RX buffer is INVALID\n");
			rx_data = 0;
		} else {
			//retrieve data
			pr_debug("RX buffer is VALID\n");
			rx_data = readl(ndev->wiegand_vaddr + RX_BUFFER1);
			rx_data <<= 32;
			rx_data |= readl(ndev->wiegand_vaddr + RX_BUFFER);
		}

		//clear rx_buffer
		writel(0x1, ndev->wiegand_vaddr + RX_BUFFER_CLEAR);

		if (copy_to_user((uint32_t *)arg, &rx_data, sizeof(rx_data)))
			return -EFAULT;
		break;

	default:
		return -ENOTTY;
	}

	return ret;
}

static int cvi_wiegand_open(struct inode *inode, struct file *filp)
{
	struct cvi_wiegand_device *ndev =
		container_of(inode->i_cdev, struct cvi_wiegand_device, cdev);

	pr_debug("cvi_wiegand_open\n");

	platform_wiegand_clk_init(ndev);

	filp->private_data = ndev;

	return 0;
}

static int cvi_wiegand_close(struct inode *inode, struct file *filp)
{
	struct cvi_wiegand_device *ndev =
		container_of(inode->i_cdev, struct cvi_wiegand_device, cdev);

	//disable wiegand interrupt
	writel(0x0, ndev->wiegand_vaddr + IRQ_ENABLE);

	platform_wiegand_clk_deinit(ndev);

	filp->private_data = NULL;

	pr_debug("cvi_wiegand_close\n");
	return 0;
}

#ifdef CONFIG_COMPAT
static long cvi_wiegand_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations wiegand_fops = {
	.owner = THIS_MODULE,
	.open = cvi_wiegand_open,
	.release = cvi_wiegand_close,
	.read = cvi_wiegand_read,
	// .write = cvi_wiegand_write,
	.unlocked_ioctl = cvi_wiegand_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cvi_wiegand_compat_ptr_ioctl,
#endif
};

int cvi_wiegand_register_cdev(struct cvi_wiegand_device *ndev)
{
	cdev_init(&ndev->cdev, &wiegand_fops);
	ndev->cdev.owner = THIS_MODULE;

	ndev->id = find_next_zero_bit(dev_mask, WGN_MAX_NUM, 0);
	if (ndev->id < WGN_MAX_NUM)
		set_bit(ndev->id, dev_mask);

	// pr_info("wiegand_cdev_id %d\n", wiegand_cdev_id);
	// pr_info("ndev->id %d\n", ndev->id);
	// pr_info("MKDEV(MAJOR(wiegand_cdev_id), ndev->id) %d\n", MKDEV(MAJOR(wiegand_cdev_id), ndev->id));

	cdev_add(&ndev->cdev, MKDEV(MAJOR(wiegand_cdev_id), ndev->id), 1);

	device_create(wiegand_class, NULL, MKDEV(MAJOR(wiegand_cdev_id), ndev->id), NULL, "%s%d",
	      CVI_WIEGAND_CDEV_NAME, ndev->id);

	return 0;
}

static int cvi_wiegand_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_wiegand_device *ndev;
	struct resource *res;
	int ret;
	pr_debug("cvi_wiegand_probe start\n");

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "wiegand");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve wiegand io\n");
		return -ENXIO;
	}

	ndev->wiegand_vaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ndev->wiegand_vaddr))
		return PTR_ERR(ndev->wiegand_vaddr);

	ndev->wiegand_irq = platform_get_irq(pdev, 0);
	if (ndev->wiegand_irq < 0) {
		dev_err(dev, "failed to retrieve wiegand irq");
		return -ENXIO;
	}

	ret = devm_request_irq(&pdev->dev, ndev->wiegand_irq, cvi_wiegand_irq, 0,
							"cvi-wiegand", ndev);
	if (ret)
		return -ENXIO;

	ndev->clk_wiegand = devm_clk_get(&pdev->dev, "clk_wgn");
	if (IS_ERR(ndev->clk_wiegand)) {
		dev_err(dev, "failed to retrieve wiegand clk_wgn\n");
		ndev->clk_wiegand = NULL;
	}

	ndev->clk_wiegand_en = 0;

	ndev->rst_wiegand = devm_reset_control_get(&pdev->dev, "res_wgn");
	if (IS_ERR(ndev->rst_wiegand)) {
		dev_err(dev, "failed to retrieve wiegand res_wgn\n");
		ndev->rst_wiegand = NULL;
	}

	ret = cvi_wiegand_register_cdev(ndev);
	if (ret < 0) {
		dev_err(dev, "regsiter chrdev error\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);
	pr_debug("cvi_wiegand_probe end\n");
	return 0;
}

static int cvi_wiegand_remove(struct platform_device *pdev)
{
	struct cvi_wiegand_device *ndev = platform_get_drvdata(pdev);

	device_destroy(wiegand_class, wiegand_cdev_id);

	cdev_del(&ndev->cdev);

	platform_set_drvdata(pdev, NULL);
	pr_debug("=== cvi_wiegand_remove\n");

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int dw_wiegand_suspend(struct device *dev)
{
	struct cvi_wiegand_device *dw_wiegand = dev_get_drvdata(dev);

	if (dw_wiegand->clk_wiegand_en == 1)
		clk_disable_unprepare(dw_wiegand->clk_wiegand);

	return 0;
}

static int dw_wiegand_resume(struct device *dev)
{
	struct cvi_wiegand_device *dw_wiegand = dev_get_drvdata(dev);

	if (dw_wiegand->clk_wiegand_en == 1)
		clk_prepare_enable(dw_wiegand->clk_wiegand);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(dw_wiegand_pm_ops, dw_wiegand_suspend, dw_wiegand_resume);
static const struct of_device_id cvi_wiegand_match[] = {
	{ .compatible = "cvitek,wiegand" },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_wiegand_match);

static struct platform_driver cvi_wiegand_driver = {
	.probe = cvi_wiegand_probe,
	.remove = cvi_wiegand_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = "cvi-wiegand",
			.of_match_table = cvi_wiegand_match,
			.pm	= &dw_wiegand_pm_ops,
		},
};

static int __init wgn_init(void)
{
	int rc;

	wiegand_class = class_create(THIS_MODULE, CVI_WIEGAND_CLASS_NAME);
	if (IS_ERR(wiegand_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(wiegand_class);
	}

	rc = alloc_chrdev_region(&wiegand_cdev_id, 0, WGN_MAX_NUM, CVI_WIEGAND_CDEV_NAME);
	if (rc < 0) {
		pr_err("alloc chrdev failed\n");
		return rc;
	}

	rc = platform_driver_register(&cvi_wiegand_driver);
	return rc;
}

static void __exit wgn_exit(void)
{
	platform_driver_unregister(&cvi_wiegand_driver);
	unregister_chrdev_region(wiegand_cdev_id, WGN_MAX_NUM);
	class_destroy(wiegand_class);
}

module_init(wgn_init);
module_exit(wgn_exit)

MODULE_AUTHOR("Mark Hsieh<mark.hsieh@wisecore.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC wiegand driver");
MODULE_LICENSE("GPL");

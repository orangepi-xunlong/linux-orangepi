/*
 * drivers/media/tsc/tscdrv.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * csjamesdeng <csjamesdeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/rmap.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <asm-generic/gpio.h>
#include <asm/current.h>
#include <linux/sys_config.h>
#include <linux/platform_device.h>
#include <linux/clk/sunxi.h>
#include "dvb_drv_sunxi.h"
#include "tscdrv.h"
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>

#define dev_tsc_printk(level, msg...) printk(level "tsc: " msg)

static struct tsc_dev *tsc_devp;
static struct device_node *node;
static int tsc_dev_major = TSCDEV_MAJOR;
static int tsc_dev_minor = TSCDEV_MINOR;
static spinlock_t tsc_spin_lock;
static int clk_status;
static struct of_device_id sun50i_tsc_match[] = {
	{ .compatible = "allwinner,sun50i-tsc",},
	{}
};

MODULE_DEVICE_TABLE(of, sun50i_tsc_match);

static DECLARE_WAIT_QUEUE_HEAD(wait_proc);

#ifdef CONFIG_PM
int enable_tsc_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&tsc_spin_lock, flags);
	if (clk_status == 1)
		goto enalbe_out;

	clk_status = 1;
	sunxi_periph_reset_deassert(tsc_devp->tsc_clk);
	if (clk_prepare_enable(tsc_devp->tsc_clk)) {
		dev_tsc_printk(KERN_WARNING, "enable tsc_clk failed.\n");
		goto enalbe_out;
	} else {
		res = 0;
	}
	enalbe_out:
	spin_unlock_irqrestore(&tsc_spin_lock, flags);
	return res;
}

int disable_tsc_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&tsc_spin_lock, flags);
	if (clk_status == 0) {
		res = 0;
		goto disable_out;
	}
	clk_status = 0;
	if ((NULL == tsc_devp->tsc_clk)
		|| (IS_ERR(tsc_devp->tsc_clk))) {
		dev_tsc_printk(KERN_WARNING, "tsc_clk is invalid!\n");
	} else {
		clk_disable_unprepare(tsc_devp->tsc_clk);
		sunxi_periph_reset_assert(tsc_devp->tsc_clk);
		res = 0;
	}
	disable_out:
	spin_unlock_irqrestore(&tsc_spin_lock, flags);
	return res;
}
#endif

/*
 * interrupt service routine
 * To wake up wait queue
 */
static irqreturn_t tscdriverinterrupt(int irq, void *dev_id)
{
	struct iomap_para addrs = tsc_devp->iomap_addrs;
	unsigned long tsc_int_status_reg;
	unsigned long tsc_int_ctrl_reg;
	unsigned int  tsc_status;
	unsigned int  tsc_interrupt_enable;

	tsc_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x80 + 0x08);
	tsc_int_status_reg = (unsigned long)(addrs.regs_macc + 0x80 + 0x18);
	tsc_interrupt_enable = ioread32((void *)(tsc_int_ctrl_reg));
	tsc_status = ioread32((void *)(tsc_int_status_reg));
	iowrite32(tsc_interrupt_enable, (void *)(tsc_int_ctrl_reg));
	iowrite32(tsc_status, (void *)(tsc_int_status_reg));
	tsc_devp->irq_flag = 1;
	wake_up_interruptible(&wait_proc);

	return IRQ_HANDLED;
}

static void close_all_fillters(struct tsc_dev *devp)
{
	int i;
	unsigned int value = 0;
	struct iomap_para addrs = tsc_devp->iomap_addrs;

	/*close tsf0*/
	iowrite32(0, (void *)(addrs.regs_macc + 0x80 + 0x10));
	iowrite32(0, (void *)(addrs.regs_macc + 0x80 + 0x30));
	for (i = 0; i < 32; i++) {
		iowrite32(i, (void *)(addrs.regs_macc + 0x80 + 0x3c));
		value = (0<<16 | 0x1fff);
		iowrite32(value, (void *)(addrs.regs_macc + 0x80 + 0x4c));
	}
}

/*
 * poll operateion for wait for TS irq
 */
unsigned int tscdev_poll(struct file *filp, struct poll_table_struct *wait)
{
	int mask = 0;

	poll_wait(filp, &tsc_devp->wq, wait);
	if (tsc_devp->irq_flag == 1)
		mask |= POLLIN | POLLRDNORM;
	return mask;
}

/*
 * ioctl function
 */
long tscdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;
	struct intrstatus statusdata;
	int arg_rate = (int)arg;
	unsigned long tsc_parent_clk_rate;

	ret = 0;
	if (_IOC_TYPE(cmd) != TSCDEV_IOC_MAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) > TSCDEV_IOC_MAXNR)
		return -EINVAL;

	switch (cmd) {
	case TSCDEV_WAIT_INT:
		ret = wait_event_interruptible_timeout(wait_proc, tsc_devp->irq_flag, HZ * 1);
		if (!ret && !tsc_devp->irq_flag) {
			/* case: wait timeout. */
			dev_tsc_printk(KERN_WARNING, "wait interrupt timeout.\n");
			memset(&statusdata, 0, sizeof(statusdata));
		} else {
			/* case: interrupt occured. */
			tsc_devp->irq_flag = 0;
			statusdata.port0chan    = tsc_devp->intstatus.port0chan;
			statusdata.port0pcr     = tsc_devp->intstatus.port0pcr;
		}
		/* copy status data to user. */
		if (copy_to_user((struct intrstatus *)arg, &(tsc_devp->intstatus),
			sizeof(struct intrstatus))) {
			return -EFAULT;
		}
		break;

	case TSCDEV_GET_PHYSICS:
		return 0;

	case TSCDEV_ENABLE_INT:
		enable_irq(tsc_devp->irq);
		break;

	case TSCDEV_DISABLE_INT:
		tsc_devp->irq_flag = 1;
		wake_up_interruptible(&wait_proc);
		disable_irq(tsc_devp->irq);
		break;

	case TSCDEV_RELEASE_SEM:
		tsc_devp->irq_flag = 1;
		wake_up_interruptible(&wait_proc);
		break;

	case TSCDEV_GET_CLK:
		tsc_devp->tsc_clk = of_clk_get(node, 1);
		if (!tsc_devp->tsc_clk || IS_ERR(tsc_devp->tsc_clk)) {
			dev_tsc_printk(KERN_WARNING, "get tsc clk failed.\n");
			ret = -EINVAL;
		}
		break;

	case TSCDEV_PUT_CLK:

		clk_put(tsc_devp->tsc_clk);

		break;

	case TSCDEV_ENABLE_CLK:
		/* clk_prepare_enable(tsc_devp->tsc_clk); */
		ret = enable_tsc_hw_clk();
		if (ret < 0) {
			dev_tsc_printk(KERN_WARNING, "%s: tsc clk enable failed!\n",
			__func__);
			return -EFAULT;
		}
		break;

	case TSCDEV_DISABLE_CLK:
		/* clk_disable_unprepare(tsc_devp->tsc_clk); */
		ret = disable_tsc_hw_clk();
		if (ret < 0) {
			dev_tsc_printk(KERN_WARNING, "%s: tsc clk disable failed!\n",
			__func__);
		}
		break;

	case TSCDEV_GET_CLK_FREQ:
		ret = clk_get_rate(tsc_devp->tsc_clk);
		break;

	case TSCDEV_SET_SRC_CLK_FREQ:
		writel(0x1, tsc_devp->iomap_addrs.regs_macc);
		break;

	case TSCDEV_SET_CLK_FREQ:
		if (clk_get_rate(tsc_devp->tsc_clk)/1000000 != arg_rate) {
			if (!clk_set_rate(tsc_devp->tsc_parent_pll_clk, arg_rate*1000000)) {
				tsc_parent_clk_rate = clk_get_rate(tsc_devp->tsc_parent_pll_clk);
				if (clk_set_rate(tsc_devp->tsc_clk, tsc_parent_clk_rate))
					dev_tsc_printk(KERN_WARNING, "set tsc clock failed!\n");
			} else
				dev_tsc_printk(KERN_WARNING, "set pll4 clock failed!\n");
		}
		ret = clk_get_rate(tsc_devp->tsc_clk);
		break;

	default:
		dev_tsc_printk(KERN_WARNING, "invalid cmd!\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int tscdev_open(struct inode *inode, struct file *filp)
{
	/* unsigned long clk_rate; */
	if (down_interruptible(&tsc_devp->sem)) {
		dev_tsc_printk(KERN_WARNING, "down interruptible failed!\n");
		return -ERESTARTSYS;
	}
	/* init other resource here */
	tsc_devp->irq_flag = 0;
	up(&tsc_devp->sem);
	nonseekable_open(inode, filp);
	return 0;
}

static int tscdev_release(struct inode *inode, struct file *filp)
{
	if (down_interruptible(&tsc_devp->sem))
		return -ERESTARTSYS;
	close_all_fillters(tsc_devp);
	/* release other resource here */
	tsc_devp->irq_flag = 1;
	up(&tsc_devp->sem);
	return 0;
}

void tscdev_vma_open(struct vm_area_struct *vma)
{
	dev_tsc_printk(KERN_DEBUG, "%s\n", __func__);
}

void tscdev_vma_close(struct vm_area_struct *vma)
{
	dev_tsc_printk(KERN_DEBUG, "%s\n", __func__);
}

static struct vm_operations_struct tscdev_remap_vm_ops = {
	.open  = tscdev_vma_open,
	.close = tscdev_vma_close,
};

static int tscdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long temp_pfn;

	if (vma->vm_end - vma->vm_start == 0) {
		dev_tsc_printk(KERN_WARNING, "vm_end is equal vm_start : %lx\n", vma->vm_start);
		return 0;
	}
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) {
		dev_tsc_printk(KERN_WARNING, "vm_pgoff is %lx,it is large than the largest page number\n",
			vma->vm_pgoff);
		return -EINVAL;
	}
	temp_pfn = REGS_BASE >> 12;
	/* Set reserved and I/O flag for the area. */
	vma->vm_flags |= /*VM_RESERVED | */VM_IO;
	/* Select uncached access. */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
		(vma->vm_end - vma->vm_start), vma->vm_page_prot)) {
		return -EAGAIN;
	}
	vma->vm_ops = &tscdev_remap_vm_ops;
	tscdev_vma_open(vma);
	return 0;
}

#ifndef NEW
static int tsc_select_gpio_state(struct pinctrl *pctrl, char *name)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		dev_tsc_printk(KERN_WARNING, "pinctrl_lookup_state(%s) failed!\n", name);
		return -1;
	}
	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		dev_tsc_printk(KERN_WARNING, "pinctrl_select_state(%s) failed!\n", name);
	return ret;
}
#endif

static unsigned int request_tsc_pio(struct platform_device *pdev)
{
    /* request device pinctrl, set as default state */
#ifdef NEW
	int ret = 0;
	if ((tsc_devp->port_config.port0config && tsc_devp->port_config.port1config)
		|| (tsc_devp->port_config.port2config && tsc_devp->port_config.port3config)) {
		dev_tsc_printk(KERN_WARNING, "port config error!\n");
		return -EINVAL;
	}
	if (tsc_devp->port_config.port0config
		|| tsc_devp->port_config.port1config
		|| tsc_devp->port_config.port2config
		|| tsc_devp->port_config.port3config) {
		if (!tsc_devp->pinctrl) {
			tsc_devp->pinctrl = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR_OR_NULL(tsc_devp->pinctrl)) {
				dev_tsc_printk(KERN_WARNING, "request pinctrl handle for tsc failed!\n");
				return -EINVAL;
			}
		}
	}
	if (tsc_devp->port_config.port0config) {
		if (!tsc_devp->ts0_pinstate) {
			tsc_devp->ts0_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts0-default");
			if (IS_ERR_OR_NULL(tsc_devp->ts0_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		if (!tsc_devp->ts0sleep_pinstate) {
			tsc_devp->ts0sleep_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts0-sleep");
			if (IS_ERR_OR_NULL(tsc_devp->ts0sleep_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts0_pinstate);
		if (ret) {
				dev_tsc_printk(KERN_WARNING, "%s %d: select state failed!\n",
				__func__, __LINE__);
				return ret;
		}
	}
	if (tsc_devp->port_config.port1config) {
		if (!tsc_devp->ts1_pinstate) {
			tsc_devp->ts1_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts1-default");
			if (IS_ERR_OR_NULL(tsc_devp->ts1_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		if (!tsc_devp->ts1sleep_pinstate) {
			tsc_devp->ts1sleep_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts1-sleep");
			if (IS_ERR_OR_NULL(tsc_devp->ts1sleep_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts1_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select state failed!\n",
			__func__, __LINE__);
			return ret;
		}
	}
	if (tsc_devp->port_config.port2config) {
		if (!tsc_devp->ts2_pinstate) {
			tsc_devp->ts2_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts2-default");
			if (IS_ERR_OR_NULL(tsc_devp->ts2_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		if (!tsc_devp->ts2sleep_pinstate) {
			tsc_devp->ts2sleep_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts2-sleep");
			if (IS_ERR_OR_NULL(tsc_devp->ts2sleep_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts2_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select state failed!\n",
			__func__, __LINE__);
			return ret;
		}
	}
	if (tsc_devp->port_config.port3config) {
		if (!tsc_devp->ts3_pinstate) {
			tsc_devp->ts3_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts3-default");
			if (IS_ERR_OR_NULL(tsc_devp->ts3_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		if (!tsc_devp->ts3sleep_pinstate) {
			tsc_devp->ts3sleep_pinstate = pinctrl_lookup_state(tsc_devp->pinctrl, "ts3-sleep");
			if (IS_ERR_OR_NULL(tsc_devp->ts3sleep_pinstate)) {
				dev_tsc_printk(KERN_WARNING, "%s %d: lookup state failed!\n",
				__func__, __LINE__);
				return -EINVAL;
			}
		}
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts3_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select state failed!\n",
			__func__, __LINE__);
			return ret;
		}
	}
	return ret;
#endif
#ifndef NEW
	tsc_devp->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(tsc_devp->pinctrl)) {
		dev_tsc_printk(KERN_WARNING, "devm get pinctrl handle for device failed!\n");
		return -1;
	}
	return tsc_select_gpio_state(tsc_devp->pinctrl, "default");
#endif
}

static int release_tsc_pio(void)
{
	int ret = 0;
	if (tsc_devp->port_config.port0config) {
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts0sleep_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select sleep state failed!\n",
			__func__, __LINE__);
			return ret;
		}
	}
	if (tsc_devp->port_config.port1config) {
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts1sleep_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select sleep state failed!\n",
			__func__, __LINE__);
			return ret;
		}
	}
	if (tsc_devp->port_config.port2config) {
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts2sleep_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select sleep state failed!\n",
			__func__, __LINE__);
			return ret;
		}

	}
	if (tsc_devp->port_config.port3config) {
		ret = pinctrl_select_state(tsc_devp->pinctrl, tsc_devp->ts3sleep_pinstate);
		if (ret) {
			dev_tsc_printk(KERN_WARNING, "%s %d: select sleep state failed!\n",
			__func__, __LINE__);
			return ret;
		}

	}
	if (tsc_devp->port_config.port0config
		|| tsc_devp->port_config.port1config
		|| tsc_devp->port_config.port2config
		|| tsc_devp->port_config.port3config) {
		if (tsc_devp->pinctrl != NULL) {
			devm_pinctrl_put(tsc_devp->pinctrl);
			tsc_devp->pinctrl = NULL;
			tsc_devp->ts0_pinstate = NULL;
			tsc_devp->ts1_pinstate = NULL;
			tsc_devp->ts2_pinstate = NULL;
			tsc_devp->ts3_pinstate = NULL;
			tsc_devp->ts0sleep_pinstate = NULL;
			tsc_devp->ts1sleep_pinstate = NULL;
			tsc_devp->ts2sleep_pinstate = NULL;
			tsc_devp->ts3sleep_pinstate = NULL;
		}
	}
	return ret;
}

static struct file_operations tscdev_fops = {
	.owner          = THIS_MODULE,
	.mmap           = tscdev_mmap,
	.poll           = tscdev_poll,
	.open           = tscdev_open,
	.release        = tscdev_release,
	.llseek         = no_llseek,
	.unlocked_ioctl = tscdev_ioctl,
};

static int  tscdev_init(struct platform_device *pdev)
{
#ifdef NEW
	unsigned int temp_val = 0;
#endif
	int ret = 0;
	int devno;
	unsigned long clk_rate;
	struct device_node *node;
	dev_t dev;

	dev = 0;
	node = pdev->dev.of_node;
	/* register or alloc the device number. */
	if (tsc_dev_major) {
		dev = MKDEV(tsc_dev_major, tsc_dev_minor);
		ret = register_chrdev_region(dev, 1, "ts0");
	} else {
		ret = alloc_chrdev_region(&dev, tsc_dev_minor, 1, "ts0");
		tsc_dev_major = MAJOR(dev);
		tsc_dev_minor = MINOR(dev);
	}
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "ts0: can't get major: %d.\n", tsc_dev_major);
		ret = -EINVAL;
		goto error0;
	}
	spin_lock_init(&tsc_spin_lock);
	tsc_devp = kmalloc(sizeof(struct tsc_dev), GFP_KERNEL);
	if (tsc_devp == NULL) {
		dev_tsc_printk(KERN_WARNING, "malloc mem for tsc device err.\n");
		ret = -ENOMEM;
		goto error0;
	}
	memset(tsc_devp, 0, sizeof(struct tsc_dev));
	tsc_devp->irq = irq_of_parse_and_map(node, 0);
	if (tsc_devp->irq <= 0) {
		dev_tsc_printk(KERN_WARNING, "can not parse irq.\n");
		ret = -EINVAL;
		goto error0;
	}
	sema_init(&tsc_devp->sem, 1);
	init_waitqueue_head(&tsc_devp->wq);
	memset(&tsc_devp->iomap_addrs, 0, sizeof(struct iomap_para));
	ret = request_irq(SUNXI_IRQ_TS, tscdriverinterrupt, 0, "ts0", NULL);
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "request irq err\n");
		ret = -EINVAL;
		goto error0;
	}
	/* map for macc io space */
	tsc_devp->iomap_addrs.regs_macc = of_iomap(node, 0);
	if (!tsc_devp->iomap_addrs.regs_macc) {
		dev_tsc_printk(KERN_WARNING, "ve can't map registers.\n");
		ret = -EINVAL;
		goto error0;
	}
	tsc_devp->tsc_parent_pll_clk = of_clk_get(node, 0);
	if ((!tsc_devp->tsc_parent_pll_clk) || IS_ERR(tsc_devp->tsc_parent_pll_clk)) {
		dev_tsc_printk(KERN_WARNING, "try to get tsc_parent_pll_clk failed!\n");
		ret = -EINVAL;
		goto error0;
	}
	tsc_devp->tsc_clk = of_clk_get(node, 1);
	if (!tsc_devp->tsc_clk || IS_ERR(tsc_devp->tsc_clk)) {
		dev_tsc_printk(KERN_WARNING, "get tsc_clk failed.\n");
		ret = -EINVAL;
		goto error0;
	}
	/* no reset tsc module. */
	sunxi_periph_reset_assert(tsc_devp->tsc_clk);
	clk_set_parent(tsc_devp->tsc_clk, tsc_devp->tsc_parent_pll_clk);
	/* set ts clock rate. */
	clk_rate = clk_get_rate(tsc_devp->tsc_parent_pll_clk);
	clk_rate /= 2;

	if (clk_set_rate(tsc_devp->tsc_clk, clk_rate) < 0) {
		dev_tsc_printk(KERN_WARNING, "set clk rate error.\n");
		ret = -EINVAL;
		goto error0;
	}
	clk_rate = clk_get_rate(tsc_devp->tsc_clk);
	dev_tsc_printk(KERN_NOTICE, "clock rate %ld\n", clk_rate);
	clk_prepare_enable(tsc_devp->tsc_clk);

	/* Create char device */
	devno = MKDEV(tsc_dev_major, tsc_dev_minor);
	cdev_init(&tsc_devp->cdev, &tscdev_fops);
	tsc_devp->cdev.owner = THIS_MODULE;
	ret = cdev_add(&tsc_devp->cdev, devno, 1);
	if (ret) {
		dev_tsc_printk(KERN_WARNING, "err:%d add tscdev.", ret);
		ret = -EINVAL;
		goto error0;
	}
	tsc_devp->tsc_class = class_create(THIS_MODULE, "ts0");
	tsc_devp->dev = device_create(tsc_devp->tsc_class, NULL, devno, NULL, "ts0");

#ifdef NEW
	ret = of_property_read_u32(node, "ts0config", &temp_val);
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s %d: configuration missing or invalid.\n",
		__func__, __LINE__);
		ret = -EINVAL;
		goto error0;
	} else {
		tsc_devp->port_config.port0config = temp_val;
	}
	temp_val = 0;
	ret = of_property_read_u32(node, "ts1config", &temp_val);
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s %d: configuration missing or invalid.\n",
		__func__, __LINE__);
		ret = -EINVAL;
		goto error0;
	} else {
		tsc_devp->port_config.port1config = temp_val;
	}
	temp_val = 0;
	ret = of_property_read_u32(node, "ts2config", &temp_val);
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s %d: configuration missing or invalid.\n",
		__func__, __LINE__);
		ret = -EINVAL;
		goto error0;
	} else {
		tsc_devp->port_config.port2config = temp_val;
	}
	temp_val = 0;
	ret = of_property_read_u32(node, "ts3config", &temp_val);
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s %d: configuration missing or invalid.\n",
		__func__, __LINE__);
		ret = -EINVAL;
		goto error0;
	} else {
		tsc_devp->port_config.port3config = temp_val;
	}
#endif
	if (request_tsc_pio(pdev)) {
		dev_tsc_printk(KERN_WARNING, "%s: request tsc pio failed!\n",
		__func__);
		ret = -EINVAL;
		goto error0;
	}
	return 0;

error0:
	if (tsc_devp) {
		kfree(tsc_devp);
		tsc_devp = NULL;
	}
	return ret;
}

static void  tscdev_exit(void)
{
	dev_t dev;
	int ret = 0;

	dev = MKDEV(tsc_dev_major, tsc_dev_minor);
	free_irq(SUNXI_IRQ_TS, NULL);
	iounmap(tsc_devp->iomap_addrs.regs_macc);
	iounmap(tsc_devp->iomap_addrs.regs_ccmu);
    /* Destroy char device */
	if (tsc_devp) {
		cdev_del(&tsc_devp->cdev);
		device_destroy(tsc_devp->tsc_class, dev);
		class_destroy(tsc_devp->tsc_class);
	}
	if (NULL == tsc_devp->tsc_clk
	|| IS_ERR(tsc_devp->tsc_clk)) {
		dev_tsc_printk(KERN_WARNING, "tsc_clk handle is invalid.\n");
	} else {
		/* clk_disable_unprepare(tsc_devp->tsc_clk); */
		ret = disable_tsc_hw_clk();
		if (ret < 0) {
			dev_tsc_printk(KERN_WARNING, "%s: tsc clk disable failed.\n",
			__func__);
		}
		clk_put(tsc_devp->tsc_clk);
		tsc_devp->tsc_clk = NULL;
	}
	if (NULL == tsc_devp->tsc_parent_pll_clk
		|| IS_ERR(tsc_devp->tsc_parent_pll_clk)) {
		dev_tsc_printk(KERN_WARNING, "tsc_parent_pll_clk handle is invalid.\n");
	} else {
		clk_put(tsc_devp->tsc_parent_pll_clk);
	}
	/* release ts pin */
	release_tsc_pio();
	unregister_chrdev_region(dev, 1);
	if (tsc_devp) {
		kfree(tsc_devp);
		tsc_devp = NULL;
	}
}


#ifdef CONFIG_PM
static int snd_sw_tsc_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;

	ret = release_tsc_pio();
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "tsc release gpio failed!\n");
		return -EFAULT;
	}
	ret = disable_tsc_hw_clk();
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s: tsc clk disable failed!\n",
		__func__);
		return -EFAULT;
	}
	dev_tsc_printk(KERN_NOTICE, "standby suspend succeed!\n");
	return 0;
}
static int snd_sw_tsc_resume(struct platform_device *pdev)
{
	int ret = 0;

	ret = enable_tsc_hw_clk();
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s: tsc clk enable failed!\n",
		__func__);
		return -EFAULT;
	}
	ret = request_tsc_pio(pdev);
	if (ret < 0) {
		dev_tsc_printk(KERN_WARNING, "%s: request tsc pio failed!\n",
		__func__);
		return -EFAULT;
	}
	dev_tsc_printk(KERN_NOTICE, "standby resume succeed!\n");
	return 0;
}
#endif

static int sunxi_tsc_remove(struct platform_device *pdev)
{
	tscdev_exit();
	return 0;
}

static int  sunxi_tsc_probe(struct platform_device *pdev)
{
	tscdev_init(pdev);
	return 0;
}

static struct platform_driver sunxi_tsc_driver = {
	.probe = sunxi_tsc_probe,
	.remove = sunxi_tsc_remove,
#ifdef CONFIG_PM
	.suspend = snd_sw_tsc_suspend,
	.resume  = snd_sw_tsc_resume,
#endif
	.driver  = {
	.name = "ts0",
	.owner = THIS_MODULE,
	.of_match_table = sun50i_tsc_match,
	},
};

static int __init sunxi_tsc_init(void)

{
	int ret;
	ret = platform_driver_register(&sunxi_tsc_driver);
	return ret;
}

static void __exit sunxi_tsc_exit(void)
{
	platform_driver_unregister(&sunxi_tsc_driver);
}

module_init(sunxi_tsc_init);
module_exit(sunxi_tsc_exit);
MODULE_AUTHOR("Soft-Reuuimlla");
MODULE_DESCRIPTION("User mode tsc device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:tsc-sunxi");


/*
 * drivers/media/video/tsc/tscdrv.c
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
#include <mach/sys_config.h>
#include <linux/platform_device.h>
#include "dvb_drv.h"
#include "tscdrv.h"


static struct tsc_dev *tsc_devp = NULL;

static DECLARE_WAIT_QUEUE_HEAD(wait_proc);

/*
 * unmap/release iomem
 */
static void tsiomem_unregister(struct tsc_dev *devp)
{
    /* Release io mem */
    iounmap(devp->regsaddr);
    devp->regsaddr = NULL;
    if (devp->regs) {
        release_resource(devp->regs);
        devp->regs = NULL;
    }
}

/*
 * ioremap and request iomem
 */
static int register_tsiomem(struct tsc_dev *devp)
{
    char *addr;
    int ret;
    struct resource *res;

    ret = check_mem_region(REGS_BASE, REGS_SIZE);
    pr_info("%s: check_mem_region return: %d\n", __func__, ret);
    res = request_mem_region(REGS_BASE, REGS_SIZE, "ts regs");
    if (res == NULL)    {
        pr_err("%s: cannot reserve region for register\n", __func__);
        goto err;
    }
    devp->regs = res;

    addr = ioremap(REGS_BASE, REGS_SIZE);
    if (!addr) {
        pr_err("%s: cannot map region for register\n", __func__);
        goto err;
    }

    devp->regsaddr = addr;
    pr_info("%s: devp->regsaddr: 0x%08x\n", __func__, (unsigned int)devp->regsaddr);

    return 0;

err:
    if (devp->regs) {
        release_resource(devp->regs);
        devp->regs = NULL;
    }
    return -1;
}

/*
 * interrupt service routine
 * To wake up wait queue
 */
static irqreturn_t tscdriverinterrupt(int irq, void *dev_id)
{
       // printk("intrupts!\n");
    tsc_devp->intstatus.port0pcr    = ioread32((void *)(tsc_devp->regsaddr + 0x88));
    tsc_devp->intstatus.port0chan   = ioread32((void *)(tsc_devp->regsaddr + 0x98));
    tsc_devp->intstatus.port1pcr    = ioread32((void *)(tsc_devp->regsaddr + 0x108));
    tsc_devp->intstatus.port1chan   = ioread32((void *)(tsc_devp->regsaddr + 0x118));

    iowrite32(tsc_devp->intstatus.port0pcr, (void *)(tsc_devp->regsaddr + 0x88));
    iowrite32(tsc_devp->intstatus.port0chan, (void *)(tsc_devp->regsaddr + 0x98));
    iowrite32(tsc_devp->intstatus.port1pcr, (void *)(tsc_devp->regsaddr + 0x108));
    iowrite32(tsc_devp->intstatus.port1chan, (void *)(tsc_devp->regsaddr + 0x118));
    tsc_devp->irq_flag = 1;

    wake_up_interruptible(&wait_proc);

    return IRQ_HANDLED;
}

static void close_all_fillters(struct tsc_dev *devp)
{
	int i;
	unsigned int value = 0;
	unsigned int tsf0_addr = 0x80;
	unsigned int tsf1_addr = 0x100;
	//close tsf0
	iowrite32(0, (void *)(tsc_devp->regsaddr + tsf0_addr + 0x10));
	iowrite32(0, (void *)(tsc_devp->regsaddr + tsf0_addr + 0x30));

	for(i = 0; i < 32; i ++) {
		iowrite32(i, (void *)(tsc_devp->regsaddr + tsf0_addr + 0x3c));
		value = (0<<16 | 0x1fff);
		iowrite32(value, (void *)(tsc_devp->regsaddr + tsf0_addr + 0x4c));
	}

	//close tsf1
	iowrite32(0, (void *)(tsc_devp->regsaddr + tsf1_addr + 0x10));
	iowrite32(0, (void *)(tsc_devp->regsaddr + tsf1_addr + 0x30));
	for(i = 0; i < 32; i ++) {
		iowrite32(i, (void *)(tsc_devp->regsaddr + tsf1_addr + 0x3c));
		value = (0<<16 | 0x1fff);
		iowrite32(value, (void *)(tsc_devp->regsaddr + tsf1_addr + 0x4c));
	}
}

/*
 * poll operateion for wait for TS irq
 */
unsigned int tscdev_poll(struct file *filp, struct poll_table_struct *wait)
{
    int mask = 0;
    struct tsc_dev *devp = filp->private_data;

    poll_wait(filp, &devp->wq, wait);

    if (devp->irq_flag == 1) {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

/*
 * ioctl function
 */
long tscdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    spinlock_t *lock;
    struct tsc_dev *devp;
    struct clk_para temp_clk;
    struct clk *clk_hdle;
    struct intrstatus statusdata;

    ret = 0;
    devp = filp->private_data;
    lock = &devp->lock;
    //pr_err("tscdev_ioctl");
    if (_IOC_TYPE(cmd) != TSCDEV_IOC_MAGIC)
        return -EINVAL;
    if (_IOC_NR(cmd) > TSCDEV_IOC_MAXNR)
        return -EINVAL;

    switch (cmd) {
        case TSCDEV_WAIT_INT:
            ret = wait_event_interruptible_timeout(wait_proc, devp->irq_flag, HZ * 1);
            if (!ret && !devp->irq_flag) {
                //case: wait timeout.
                pr_err("%s: wait timeout\n", __func__);
                memset(&statusdata, 0, sizeof(statusdata));
            } else {
                //case: interrupt occured.
                devp->irq_flag = 0;
                statusdata.port0chan    = devp->intstatus.port0chan;
                statusdata.port0pcr     = devp->intstatus.port0pcr;
                statusdata.port1chan    = devp->intstatus.port1chan;
                statusdata.port1pcr     = devp->intstatus.port1pcr;
            }

            //copy status data to user
            if (copy_to_user((struct intrstatus *)arg, &(devp->intstatus),
                             sizeof(struct intrstatus))) {
                return -EFAULT;
            }

            break;

        case TSCDEV_GET_PHYSICS:
            return 0;

        case TSCDEV_ENABLE_INT:
            enable_irq(devp->irq);
            break;

        case TSCDEV_DISABLE_INT:
            tsc_devp->irq_flag = 1;
            wake_up_interruptible(&wait_proc);
            disable_irq(devp->irq);
            break;

        case TSCDEV_RELEASE_SEM:
            tsc_devp->irq_flag = 1;
            wake_up_interruptible(&wait_proc);
            break;

        case TSCDEV_GET_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                               sizeof(struct clk_para))) {
                pr_err("%s: get clk error\n", __func__);
                return -EFAULT;
            }
            clk_hdle = clk_get(devp->dev, temp_clk.clk_name);

            if (copy_to_user((char *) & ((struct clk_para *)arg)->handle,
                             &clk_hdle, 4)) {
                pr_err("%s: get clk error\n", __func__);
                return -EFAULT;
            }
            break;

        case TSCDEV_PUT_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                               sizeof(struct clk_para))) {
                pr_err("%s: put clk error\n", __func__);
                return -EFAULT;
            }
            clk_put((struct clk *)temp_clk.handle);

            break;

        case TSCDEV_ENABLE_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                               sizeof(struct clk_para))) {
                pr_err("%s: enable clk error\n", __func__);
                return -EFAULT;
            }

            clk_prepare_enable((struct clk *)temp_clk.handle);

            break;

        case TSCDEV_DISABLE_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                               sizeof(struct clk_para))) {
                pr_err("%s: disable clk error\n", __func__);
                return -EFAULT;
            }

            clk_disable_unprepare((struct clk *)temp_clk.handle);

            break;

        case TSCDEV_GET_CLK_FREQ:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                               sizeof(struct clk_para))) {
                pr_err("%s: get clk freq error\n", __func__);
                return -EFAULT;
            }
            temp_clk.clk_rate = clk_get_rate((struct clk *)temp_clk.handle);

            if (copy_to_user((char *) & ((struct clk_para *)arg)->clk_rate,
                             &temp_clk.clk_rate, 4)) {
                pr_err("%s: get clk freq error\n", __func__);
                return -EFAULT;
            }
            break;

        case TSCDEV_SET_SRC_CLK_FREQ:
            break;

        case TSCDEV_SET_CLK_FREQ:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                               sizeof(struct clk_para))) {
                pr_err("%s: set clk error\n", __func__);
                return -EFAULT;
            }

            clk_set_rate((struct clk *)temp_clk.handle, temp_clk.clk_rate);

            break;

        default:
            pr_err("%s: invalid cmd\n", __func__);
            ret = -EINVAL;
            break;
    }

    return ret;
}

static int tscdev_open(struct inode *inode, struct file *filp)
{
    int ret;
    struct tsc_dev *devp;
    struct clk_para tmp_clk;

    devp = container_of(inode->i_cdev, struct tsc_dev, cdev);
    filp->private_data = devp;

    if (down_interruptible(&devp->sem)) {
        return -ERESTARTSYS;
    }

    if(devp->is_opened) {
    	pr_err("open tsc drivers from process:%d", current->pid);
    	return -EPERM;
    }

    //open ts clock
    strcpy(tmp_clk.clk_name, "ts");
    tsc_devp->tsc_clk = clk_get(tsc_devp->dev, tmp_clk.clk_name);
    if (!tsc_devp->tsc_clk || IS_ERR(tsc_devp->tsc_clk)) {
        pr_err("%s: get tsc clk failed\n", __func__);
        ret = -EINVAL;
        goto err;
    }

    if (clk_prepare_enable(tsc_devp->tsc_clk) < 0) {
        pr_err("%s: enable tsc clk error\n", __func__);
        ret = -EINVAL;
        goto err;
    }

	
    strcpy(tmp_clk.clk_name, "pll_periph0");
    tsc_devp->parent = clk_get(tsc_devp->dev, tmp_clk.clk_name);
    if (!tsc_devp->parent || IS_ERR(tsc_devp->parent)) {
        pr_err("%s: get pll5 clk failed\n", __func__);
        ret = -EINVAL;
        goto err;
    }
    clk_set_parent(tsc_devp->tsc_clk, tsc_devp->parent);


    //set ts clock rate
    tmp_clk.clk_rate = clk_get_rate(tsc_devp->parent);
    pr_info("%s: parent clock rate %d\n", __func__, tmp_clk.clk_rate);
    tmp_clk.clk_rate /= 2;
    pr_info("%s: clock rate %d\n", __func__, tmp_clk.clk_rate);
    if (clk_set_rate(tsc_devp->tsc_clk, tmp_clk.clk_rate) < 0) {
        pr_err("%s: set clk rate error\n", __func__);
        ret = -EINVAL;
        goto err;
    }
    tmp_clk.clk_rate = clk_get_rate(tsc_devp->tsc_clk);
    pr_info("%s: clock rate %d", __func__, tmp_clk.clk_rate);

    /* init other resource here */
    devp->irq_flag = 0;
    devp->is_opened = 1;
    up(&devp->sem);

    nonseekable_open(inode, filp);

    return 0;

err:
	if (devp->tsc_clk) {
		clk_disable_unprepare(tsc_devp->tsc_clk);
		clk_put(tsc_devp->tsc_clk);
		devp->tsc_clk = NULL;
	}

	if(tsc_devp->parent) {
		//clk_disable_unprepare(tsc_devp->parent);
		clk_put(tsc_devp->parent);
		tsc_devp->parent = NULL;
	}
	
    return ret;
}

static int tscdev_release(struct inode *inode, struct file *filp)
{
    struct tsc_dev *devp;

    devp = filp->private_data;
    pr_err("tscdev_release");
    if (down_interruptible(&devp->sem)) {
        return -ERESTARTSYS;
    }
    close_all_fillters(devp);

    if (devp->tsc_clk) {
        clk_disable_unprepare(tsc_devp->tsc_clk);
        clk_put(tsc_devp->tsc_clk);
        devp->tsc_clk = NULL;
    }

    if(tsc_devp->parent) {
    	//clk_disable_unprepare(tsc_devp->parent);
    	clk_put(tsc_devp->parent);
    	tsc_devp->parent = NULL;
    }
	
    /* release other resource here */
    devp->irq_flag = 1;
    devp->is_opened = 0;
    up(&devp->sem);

    return 0;
}

void tscdev_vma_open(struct vm_area_struct *vma)
{
    pr_info("%s\n", __func__);
}

void tscdev_vma_close(struct vm_area_struct *vma)
{
    pr_info("%s\n", __func__);
}

static struct vm_operations_struct tscdev_remap_vm_ops = {
    .open  = tscdev_vma_open,
    .close = tscdev_vma_close,
};

static int tscdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned int mypfn;
    unsigned int vmsize;
    vmsize = vma->vm_end - vma->vm_start;
    if (vmsize <= PAGE_SIZE) { // if mmap registers, vmsize must be less than PAGE_SIZE
        mypfn = REGS_BASE >> 12;

        vma->vm_flags |= VM_RESERVED | VM_IO;
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        if (remap_pfn_range(vma, vma->vm_start, mypfn, vmsize, vma->vm_page_prot)) {
            return -EAGAIN;
        }
    }

    vma->vm_ops = &tscdev_remap_vm_ops;
    tscdev_vma_open(vma);

    return 0;
}

static unsigned int request_tsc_pio(void)
{
	/* request device pinctrl, set as default state */ 
	tsc_devp->pinctrl = devm_pinctrl_get_select_default(tsc_devp->dev); 
	if (IS_ERR_OR_NULL(tsc_devp->pinctrl)) { 
		pr_err("request pinctrl handle for device [%s] failed\n", 
		dev_name(tsc_devp->dev)); 
		return -EINVAL; 
	}

	return 0;
}

static void release_tsc_pio(void)
{
	if (tsc_devp->pinctrl != NULL) {
		devm_pinctrl_put(tsc_devp->pinctrl);
	}
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

static int tsc_used = 0;

static int __init tscdev_init(void)
{
    int ret;
    int devno;
    dev_t dev = 0;

    script_item_value_type_e type;
    script_item_u val;

    type = script_get_item("ts0", "tsc_used", &val);
    if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
        pr_err("%s: get tsc_used failed\n", __func__);
        return 0;
    }

    if (val.val != 1) {
        pr_err("%s: tsc driver is disabled\n", __func__);
        return 0;
    }
    tsc_used = val.val;

    tsc_devp = kmalloc(sizeof(struct tsc_dev), GFP_KERNEL);
    if (tsc_devp == NULL) {
        pr_err("%s: malloc memory for tsc device error\n", __func__);
        return -ENOMEM;
    }
    memset(tsc_devp, 0, sizeof(struct tsc_dev));

    tsc_devp->ts_dev_major  = TSCDEV_MAJOR;
    tsc_devp->ts_dev_minor  = TSCDEV_MINOR;

    /* register char device */
    dev = MKDEV(tsc_devp->ts_dev_major, tsc_devp->ts_dev_minor);
    if (tsc_devp->ts_dev_major) {
        ret = register_chrdev_region(dev, 1, "ts0");
    } else {
        ret = alloc_chrdev_region(&dev, tsc_devp->ts_dev_minor, 1, "ts0");
        tsc_devp->ts_dev_major = MAJOR(dev);
        tsc_devp->ts_dev_minor = MINOR(dev);
    }
    if (ret < 0) {
        pr_err("%s: can't get major %d", __func__, tsc_devp->ts_dev_major);
        return -EFAULT;
    }

    sema_init(&tsc_devp->sem, 1);
    init_waitqueue_head(&tsc_devp->wq);

    /* request TS irq */
    ret = request_irq(TS_IRQ_NO, tscdriverinterrupt, 0, "ts0", NULL);
    if (ret < 0) {
        pr_err("%s: request irq error\n", __func__);
        ret = -EINVAL;
        goto err2;
    }
    tsc_devp->irq = TS_IRQ_NO;

    /* create char device */
    devno = MKDEV(tsc_devp->ts_dev_major, tsc_devp->ts_dev_minor);
    cdev_init(&tsc_devp->cdev, &tscdev_fops);
    tsc_devp->cdev.owner = THIS_MODULE;
    tsc_devp->cdev.ops = &tscdev_fops;
    ret = cdev_add(&tsc_devp->cdev, devno, 1);
    if (ret) {
        pr_err("%s: add tsc char device error\n", __func__);
        ret = -EINVAL;
        goto err3;
    }

    tsc_devp->class = class_create(THIS_MODULE, "ts0");
    if (IS_ERR(tsc_devp->class)) {
        pr_err("%s: create tsc_dev class failed\n", __func__);
        ret = -EINVAL;
        goto err4;
    }

    tsc_devp->dev = device_create(tsc_devp->class, NULL, devno, NULL, "ts0");
    if (IS_ERR(tsc_devp->dev)) {
        pr_err("%s: create tsc_dev device failed\n", __func__);
        ret = -EINVAL;
        goto err5;
    }

	    /* request TS pio */
    if (request_tsc_pio()) {
        pr_err("%s: request tsc pio failed\n", __func__);
        ret = -EINVAL;
        goto err6;
    }

    if (register_tsiomem(tsc_devp)) {
        pr_err("%s: register ts io memory error\n", __func__);
        ret = -EINVAL;
        goto err7;
    }

    return 0;

err7:
    release_tsc_pio();
err6:
    device_destroy(tsc_devp->class, dev);
err5:
    class_destroy(tsc_devp->class);
err4:
    cdev_del(&tsc_devp->cdev);
err3:
    free_irq(TS_IRQ_NO, NULL);
err2:
    unregister_chrdev_region(dev, 1);
    if (tsc_devp) {
        kfree(tsc_devp);
    }

    return ret;
}

static void __exit tscdev_exit(void)
{
    dev_t dev;

    if (tsc_used != 1) {
        pr_err("%s: tsc driver is disabled\n", __func__);
        return;
    }

    if (tsc_devp == NULL) {
        pr_err("%s: invalid tsc_devp\n", __func__);
        return;
    }

    /* unregister iomem and iounmap */
    tsiomem_unregister(tsc_devp);

    dev = MKDEV(tsc_devp->ts_dev_major, tsc_devp->ts_dev_minor);
    device_destroy(tsc_devp->class, dev);
    class_destroy(tsc_devp->class);
    cdev_del(&tsc_devp->cdev);

    /* release ts irq */
    free_irq(TS_IRQ_NO, NULL);

    /* release ts pin */
    release_tsc_pio();

    unregister_chrdev_region(dev, 1);

    kfree(tsc_devp);
}


#ifdef CONFIG_PM
static int snd_sw_tsc_suspend(struct platform_device *pdev,pm_message_t state)
{
	return 0;
}

static int snd_sw_tsc_resume(struct platform_device *pdev)
{
	return 0;
}

#endif

static int __devexit sunxi_tsc_remove(struct platform_device *pdev)
{
	tscdev_exit();
	return 0;
}

static int __devinit sunxi_tsc_probe(struct platform_device *pdev)
{
		tscdev_init();
		return 0;
}

static struct resource sunxi_tsc_resource[] = {
	[0] = {
		.start = TS_IRQ_NO,
		.end   = TS_IRQ_NO,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device sunxi_device_tsc = {
	.name	= "ts0",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sunxi_tsc_resource),
	.resource	= sunxi_tsc_resource,
};

static struct platform_driver sunxi_tsc_driver = {
	.probe		= sunxi_tsc_probe,
	.remove		= __devexit_p(sunxi_tsc_remove),
#ifdef CONFIG_PM
	.suspend	= snd_sw_tsc_suspend,
	.resume		= snd_sw_tsc_resume,
#endif
	.driver		= {
		.name	= "ts0",
		.owner	= THIS_MODULE,
	},
};

static int __init sunxi_tsc_init(void)
{
	platform_device_register(&sunxi_device_tsc);
	printk("sunxi tsc version 0.1 \n");
	return platform_driver_register(&sunxi_tsc_driver);
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




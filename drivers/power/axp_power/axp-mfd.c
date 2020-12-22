/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/arisc/arisc.h>

#include "axp-cfg.h"

static int power_start = 0;
static DEFINE_SPINLOCK(axp_list_lock);
static LIST_HEAD(mfd_list);

#ifdef CONFIG_SUNXI_ARISC
static int axp_mfd_irq_cb(void *arg)
{
	struct axp_dev *dev;
	unsigned long irqflags;

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_for_each_entry(dev, &mfd_list, list) {
		(void)schedule_work(&dev->irq_work);
	}
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	return 0;
}
#endif

struct axp_dev *axp_dev_lookup(int type)
{
	struct axp_dev *dev = NULL;
	unsigned long irqflags;

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_for_each_entry(dev, &mfd_list, list) {
		if (type == dev->type)
			goto out;
	}
	spin_unlock_irqrestore(&axp_list_lock, irqflags);
	return NULL;
out:
	spin_unlock_irqrestore(&axp_list_lock, irqflags);
	return dev;
}
EXPORT_SYMBOL_GPL(axp_dev_lookup);

static void axp_power_off(void)
{
#if defined (CONFIG_AW_AXP22)
	uint8_t val;
	struct axp_dev *axp;
	axp = axp_dev_lookup(AXP22);
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return;
	}
	if(axp22_config.pmu_pwroff_vol >= 2600 && axp22_config.pmu_pwroff_vol <= 3300){
		if (axp22_config.pmu_pwroff_vol > 3200){
			val = 0x7;
		}else if (axp22_config.pmu_pwroff_vol > 3100){
			val = 0x6;
		}else if (axp22_config.pmu_pwroff_vol > 3000){
			val = 0x5;
		}else if (axp22_config.pmu_pwroff_vol > 2900){
			val = 0x4;
		}else if (axp22_config.pmu_pwroff_vol > 2800){
			val = 0x3;
		}else if (axp22_config.pmu_pwroff_vol > 2700){
			val = 0x2;
		}else if (axp22_config.pmu_pwroff_vol > 2600){
			val = 0x1;
		}else
			val = 0x0;
		axp_update(axp->dev, AXP22_VOFF_SET, val, 0x7);
	}
	val = 0xff;
	printk("[axp] send power-off command!\n");

	mdelay(20);

	if(power_start != 1){
		axp_read(axp->dev, AXP22_STATUS, &val);
		if(val & 0xF0){
			axp_read(axp->dev, AXP22_MODE_CHGSTATUS, &val);
	    		if(val & 0x20){
	    			printk("[axp] set flag!\n");
				axp_read(axp->dev, AXP22_BUFFERC, &val);
				if (0x0d != val)
					axp_write(axp->dev, AXP22_BUFFERC, 0x0f);
	    			mdelay(20);
		    		printk("[axp] reboot!\n");
				machine_restart(NULL);
		    		printk("[axp] warning!!! arch can't ,reboot, maybe some error happend!\n");
	    		}
		}
	}
	axp_read(axp->dev, AXP22_BUFFERC, &val);
	if (0x0d != val)
		axp_write(axp->dev, AXP22_BUFFERC, 0x00);

	mdelay(20);
	axp_set_bits(axp->dev, AXP22_OFF_CTL, 0x80);
	mdelay(20);
	printk("[axp] warning!!! axp can't power-off, maybe some error happend!\n");
#elif defined (CONFIG_AW_AXP81X)
	axp81x_power_off();
#elif defined (CONFIG_AW_AXP19)
	axp19_power_off(power_start);
#elif defined (CONFIG_AW_AXP20)
	axp20_power_off(power_start);
#endif

}

static int axp_mfd_create_attrs(struct axp_dev *dev)
{
	int j,ret;

	for (j = 0; j < dev->attrs_number; j++) {
		ret = device_create_file(dev->dev, (dev->attrs+j));
		if (ret)
			goto sysfs_failed;
	}
	return ret;

sysfs_failed:
	while (j--)
		device_remove_file(dev->dev, (dev->attrs+j));
	return ret;
}

static int __remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static int axp_mfd_remove_subdevs(struct axp_dev *dev)
{
	return device_for_each_child(dev->dev, NULL, __remove_subdev);
}

static int axp_mfd_add_subdevs(struct axp_dev *dev)
{
	struct axp_funcdev_info *regl_dev;
	struct axp_funcdev_info *sply_dev;
	struct platform_device *pdev;
	int i, ret = 0;

	/* register for regultors */
	for (i = 0; i < dev->pdata->num_regl_devs; i++) {
		regl_dev = &dev->pdata->regl_devs[i];
		pdev = platform_device_alloc(regl_dev->name, regl_dev->id);
		pdev->dev.parent = dev->dev;
		pdev->dev.platform_data = regl_dev->platform_data;
		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}

	/* register for power supply */
	for (i = 0; i < dev->pdata->num_sply_devs; i++) {
		sply_dev = &dev->pdata->sply_devs[i];
		pdev = platform_device_alloc(sply_dev->name, sply_dev->id);
		pdev->dev.parent = dev->dev;
		pdev->dev.platform_data = sply_dev->platform_data;
		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}

	return 0;

failed:
	axp_mfd_remove_subdevs(dev);
	return ret;
}

int axp_register_mfd(struct axp_dev *dev)
{
	int ret = -1;
	unsigned long irqflags;

	spin_lock_init(&dev->spinlock);
	mutex_init(&dev->lock);
	BLOCKING_INIT_NOTIFIER_HEAD(&dev->notifier_list);

	ret = dev->ops->init_chip(dev);
	if (ret)
		goto out_free_dev;

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_add(&dev->list, &mfd_list);
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	ret = axp_mfd_add_subdevs(dev);
	if (ret)
		goto out_free_dev;

	ret = axp_mfd_create_attrs(dev);
	if(ret)
		goto out_free_dev;

	if (AXP22 == dev->type)
		power_start = axp22_config.power_start;

	return 0;
out_free_dev:
	return ret;
}
EXPORT_SYMBOL(axp_register_mfd);

void axp_unregister_mfd(struct axp_dev *dev)
{
	unsigned long irqflags;

	if (dev == NULL)
		return;

	axp_mfd_remove_subdevs(dev);

	spin_lock_irqsave(&axp_list_lock, irqflags);
	list_del(&dev->list);
	spin_unlock_irqrestore(&axp_list_lock, irqflags);

	return;
}
EXPORT_SYMBOL_GPL(axp_unregister_mfd);

static int __init axp_mfd_init(void)
{	
	int ret = 0;

	/* PM hookup */
	if(!pm_power_off)
		pm_power_off = axp_power_off;

#ifdef CONFIG_SUNXI_ARISC
	ret = arisc_nmi_cb_register(NMI_INT_TYPE_PMU, axp_mfd_irq_cb, NULL);
	if (ret) {
		printk("axp failed to reg irq cb\n");
	}
#else
#endif
	return ret;
}

static void __exit axp_mfd_exit(void)
{
	pm_power_off = NULL;

#ifdef CONFIG_SUNXI_ARISC
	arisc_nmi_cb_unregister(NMI_INT_TYPE_PMU, axp_mfd_irq_cb);
#else
#endif

}

subsys_initcall(axp_mfd_init);
module_exit(axp_mfd_exit);

MODULE_DESCRIPTION("PMIC MFD Driver for AXP");
MODULE_AUTHOR("LiMing X-POWERS");
MODULE_LICENSE("GPL");
 

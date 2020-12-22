/*
 * sunxi-di.c DE-Interlace driver
 *
 * Copyright (C) 2013-2014 allwinner.
 *	Ming Li<liming@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/ion_sunxi.h>
#include <asm/irq.h>
#include <mach/sys_config.h>
#include "sunxi-di.h"

static di_struct di_data;
static int sunxi_di_major = -1;
static struct class *di_dev_class;
static struct workqueue_struct *di_wq = NULL;
static struct work_struct  di_work;
static struct timer_list *s_timer;
static struct device *di_device = NULL;
static struct clk *di_clk;
static struct clk *di_clk_source;

static u32 debug_mask = 0x0;

static void di_complete_check_set(u32 data)
{
	atomic_set(&di_data.di_complete, data);

	return;
}

static int32_t di_complete_check_get(void)
{
	u32 data_temp = 0;

	data_temp = atomic_read(&di_data.di_complete);

	return data_temp;
}

static void di_timer_handle(unsigned long arg)
{
	u32 flag_size = 0;

	di_complete_check_set(DI_MODULE_TIMEOUT);
	wake_up_interruptible(&di_data.wait);
	flag_size = (FLAG_WIDTH*FLAG_HIGH)/4;
	di_irq_enable(0);
	di_irq_clear();
	di_reset();
	memset(di_data.in_flag, 0, flag_size);
	memset(di_data.out_flag, 0, flag_size);
	dprintk(DEBUG_INT, "di_timer_handle: timeout \n");
}

static void di_work_func(struct work_struct *work)
{
	del_timer_sync(s_timer);
	return;
}

static irqreturn_t di_irq_service(int irqno, void *dev_id)
{
	int ret;

	dprintk(DEBUG_INT, "%s: enter \n", __func__);
	di_irq_enable(0);
	ret = di_get_status();
	if (0 == ret) {
		di_complete_check_set(0);
		wake_up_interruptible(&di_data.wait);
		queue_work(di_wq, &di_work);
	} else {
		di_complete_check_set(-ret);
		wake_up_interruptible(&di_data.wait);
	}
	di_irq_clear();
	di_reset();
	return IRQ_HANDLED;
}

static int di_clk_cfg(void)
{
	unsigned long rate = 0;

	di_clk_source = clk_get(NULL, BUS_CLK_NAME);
	if (!di_clk_source || IS_ERR(di_clk_source)) {
		printk(KERN_ERR "try to get di_clk_source clock failed!\n");
		return -1;
	}

	rate = clk_get_rate(di_clk_source);
	dprintk(DEBUG_INIT, "%s: get di_clk_source rate %luHZ\n", __func__, rate);

	di_clk = clk_get(NULL, MODULE_CLK_NAME);
	if (!di_clk || IS_ERR(di_clk)) {
		printk(KERN_ERR "try to get di clock failed!\n");
		return -1;
	}

	if(clk_set_parent(di_clk, di_clk_source)) {
		printk(KERN_ERR "%s: set di_clk parent to di_clk_source failed!\n", __func__);
		return -1;
	}
#if defined CONFIG_ARCH_SUN9IW1P1
#else
	rate = rate/2;
	if (clk_set_rate(di_clk, rate)) {
		printk(KERN_ERR "set di clock freq to PLL_PERIPH0/2 failed!\n");
		return -1;
	}
	rate = clk_get_rate(di_clk);
	dprintk(DEBUG_INIT, "%s: get di_clk rate %dHZ\n", __func__, (__u32)rate);
#endif
	return 0;
}

static void di_clk_uncfg(void)
{
	if(NULL == di_clk || IS_ERR(di_clk)) {
		printk(KERN_ERR "di_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_put(di_clk);
		di_clk = NULL;
	}

	if(NULL == di_clk_source || IS_ERR(di_clk_source)) {
		printk(KERN_ERR "di_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(di_clk_source);
		di_clk_source = NULL;
	}
	return;
}

static int di_clk_enable(void)
{
	if(NULL == di_clk || IS_ERR(di_clk)) {
		printk(KERN_ERR "di_clk handle is invalid, just return!\n");
		return -1;
	} else {
		if (clk_prepare_enable(di_clk)) {
			printk(KERN_ERR "try to enable di_clk failed!\n");
			return -1;
		}
	}
	return 0;
}

static void di_clk_disable(void)
{
	if(NULL == di_clk || IS_ERR(di_clk)) {
		printk(KERN_ERR "di_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(di_clk);
	}
	return;
}

static int sunxi_di_params_init(void)
{
	int ret = 0;
	ret = di_clk_cfg();
	if (ret) {
		printk(KERN_ERR "%s: clk cfg failed.\n", __func__);
		goto clk_cfg_fail;
	}

	if (request_irq(SW_INT_IRQNO_DI, di_irq_service, 0, "DE-Interlace",
			di_device)) {
		ret = -EBUSY;
		printk(KERN_ERR "%s: request irq failed.\n", __func__);
		goto request_irq_err;
	}
	di_set_reg_base((unsigned int)DI_BASE);
	return 0;
request_irq_err:
	di_clk_uncfg();
clk_cfg_fail:
	return ret;
}

static void sunxi_di_params_exit(void)
{
  di_clk_uncfg();
  free_irq(SW_INT_IRQNO_DI, di_device);
}

static void sunxi_di_disabledrv(void)
{
  di_irq_enable(0);
  di_reset();
  di_internal_clk_disable();
  di_clk_disable();
  if (NULL != di_data.in_flag)
    sunxi_buf_free(di_data.in_flag, di_data.in_flag_phy, di_data.flag_size);
  if (NULL != di_data.out_flag)
    sunxi_buf_free(di_data.out_flag, di_data.out_flag_phy, di_data.flag_size);
}

static int sunxi_di_enabledrv(void)
{
  int ret;
  
  di_data.flag_size = (FLAG_WIDTH*FLAG_HIGH)/4;

  di_data.in_flag = sunxi_buf_alloc(di_data.flag_size, &(di_data.in_flag_phy));
  if (!di_data.in_flag) {
    printk(KERN_ERR "%s: request in_flag mem failed\n", __func__);
    return -1;
  }

  di_data.out_flag = sunxi_buf_alloc(di_data.flag_size, &(di_data.out_flag_phy));
  if (!di_data.out_flag) {
    printk(KERN_ERR "%s: request out_flag mem failed\n", __func__);
    sunxi_buf_free(di_data.in_flag, di_data.in_flag_phy, di_data.flag_size);
    return -1;
  }

  ret = di_clk_enable();
  if (ret) {
    printk(KERN_ERR "%s: enable clk failed.\n", __func__);
    return ret;
  }
  
  di_internal_clk_enable();
  di_set_init(di_data.mode);
  
  return 0;
}

#ifdef CONFIG_PM
static int sunxi_di_suspend(struct device *dev)
{
  dprintk(DEBUG_SUSPEND, "enter: sunxi_di_suspend. \n");

  if (atomic_read(&di_data.enable))
    sunxi_di_disabledrv();

  return 0;
}

static int sunxi_di_resume(struct device *dev)
{
  dprintk(DEBUG_SUSPEND, "enter: sunxi_di_resume. \n");
  
  if (atomic_read(&di_data.enable))
    return sunxi_di_enabledrv();

  return 0;
}
#endif

static long sunxi_di_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = 0;
	unsigned int field = 0;

	dprintk(DEBUG_TEST, "%s: enter!!\n", __func__);
	switch (cmd) {
	case DI_IOCSTART:
		{
			__di_para_t __user *di_para = argp;

			dprintk(DEBUG_DATA_INFO, "%s: input_fb.addr[0] = 0x%x\n", __func__, di_para->input_fb.addr[0]);
			dprintk(DEBUG_DATA_INFO, "%s: input_fb.addr[1] = 0x%x\n", __func__, di_para->input_fb.addr[1]);
			dprintk(DEBUG_DATA_INFO, "%s: input_fb.size.width = %d\n", __func__, di_para->input_fb.size.width);
			dprintk(DEBUG_DATA_INFO, "%s: input_fb.size.height = %d\n", __func__, di_para->input_fb.size.height);
			dprintk(DEBUG_DATA_INFO, "%s: input_fb.format = %d\n", __func__, di_para->input_fb.format);

			dprintk(DEBUG_DATA_INFO, "%s: pre_fb.addr[0] = 0x%x\n", __func__, di_para->pre_fb.addr[0]);
			dprintk(DEBUG_DATA_INFO, "%s: pre_fb.addr[1] = 0x%x\n", __func__, di_para->pre_fb.addr[1]);
			dprintk(DEBUG_DATA_INFO, "%s: pre_fb.size.width = %d\n", __func__, di_para->pre_fb.size.width);
			dprintk(DEBUG_DATA_INFO, "%s: pre_fb.size.height = %d\n", __func__, di_para->pre_fb.size.height);
			dprintk(DEBUG_DATA_INFO, "%s: pre_fb.format = %d\n", __func__, di_para->pre_fb.format);
			dprintk(DEBUG_DATA_INFO, "%s: source_regn.width = %d\n", __func__, di_para->source_regn.width);
			dprintk(DEBUG_DATA_INFO, "%s: source_regn.height = %d\n", __func__, di_para->source_regn.height);

			dprintk(DEBUG_DATA_INFO, "%s: output_fb.addr[0] = 0x%x\n", __func__, di_para->output_fb.addr[0]);
			dprintk(DEBUG_DATA_INFO, "%s: output_fb.addr[1] = 0x%x\n", __func__, di_para->output_fb.addr[1]);
			dprintk(DEBUG_DATA_INFO, "%s: output_fb.size.width = %d\n", __func__, di_para->output_fb.size.width);
			dprintk(DEBUG_DATA_INFO, "%s: output_fb.size.height = %d\n", __func__, di_para->output_fb.size.height);
			dprintk(DEBUG_DATA_INFO, "%s: output_fb.format = %d\n", __func__, di_para->output_fb.format);
			dprintk(DEBUG_DATA_INFO, "%s: out_regn.width = %d\n", __func__, di_para->out_regn.width);
			dprintk(DEBUG_DATA_INFO, "%s: out_regn.height = %d\n", __func__, di_para->out_regn.height);
			dprintk(DEBUG_DATA_INFO, "%s: field = %d\n", __func__, di_para->field);
			dprintk(DEBUG_DATA_INFO, "%s: top_field_first = %d\n", __func__, di_para->top_field_first);

			/* when di is in work, wait*/
			ret = di_complete_check_get();
			while (1 == ret) {
				msleep(1);
				ret = di_complete_check_get();
			}
			di_complete_check_set(1);

			field = di_para->top_field_first?di_para->field:(1-di_para->field);

			dprintk(DEBUG_DATA_INFO, "%s: field = %d\n", __func__, field);
			dprintk(DEBUG_DATA_INFO, "%s: in_flag_phy = 0x%x\n", __func__, di_data.in_flag_phy);
			dprintk(DEBUG_DATA_INFO, "%s: out_flag_phy = 0x%x\n", __func__, di_data.out_flag_phy);

			if (0 == field)
				ret = di_set_para(di_para, di_data.in_flag_phy, di_data.out_flag_phy, field);
			else
				ret = di_set_para(di_para, di_data.out_flag_phy, di_data.in_flag_phy, field);
			if (ret) {
				printk(KERN_ERR "%s: deinterlace work failed.\n", __func__);
				return -1;
			} else {
				di_irq_enable(1);
				di_start();
				mod_timer(s_timer, jiffies + msecs_to_jiffies(DI_TIMEOUT));
			}

			if (!(filp->f_flags & O_NONBLOCK)) {
				ret = wait_event_interruptible(di_data.wait, (di_complete_check_get() != 1));
				if (ret)
					return ret;
			}
			ret = di_complete_check_get();
		}
		break;
	default:
		break;
	}
	dprintk(DEBUG_TEST, "%s: out!!\n", __func__);
	return ret;
}

static int sunxi_di_open(struct inode *inode, struct file *file)
{
  int ret;
  dprintk(DEBUG_DATA_INFO, "%s: enter!!\n", __func__);

  //FIXME: This variable should be counter!
  atomic_set(&di_data.enable, 1);

  ret = sunxi_di_enabledrv();
  if(ret)
    return ret;
  
  return 0;
}

static int sunxi_di_release(struct inode *inode, struct file *file)
{
  dprintk(DEBUG_DATA_INFO, "%s: enter!!\n", __func__);

  atomic_set(&di_data.enable, 0);
  sunxi_di_disabledrv();

  return 0;
}

static const struct file_operations sunxi_di_fops = {
	.owner = THIS_MODULE,
	.llseek = noop_llseek,
	.unlocked_ioctl = sunxi_di_ioctl,
	.open = sunxi_di_open,
	.release = sunxi_di_release,
};

static int __init sunxi_di_init(void)
{
	script_item_u used;
	script_item_u mode;
	script_item_value_type_e type;
	int ret;

	dprintk(DEBUG_INIT, "%s: enter!!\n", __func__);

	type = script_get_item("di_para", "di_used", &used);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_ERR "%s get di_used err!\n", __func__);
		return -1;
	}

	if (1 != used.val)
		return -1;

	type = script_get_item("di_para", "di_by_pass", &mode);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_INFO "%s get di_by_pass err!\n", __func__);
	} else {
		di_data.mode = mode.val;
	}

	atomic_set(&di_data.di_complete, 0);
	atomic_set(&di_data.enable, 0);

	init_waitqueue_head(&di_data.wait);

	s_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	if (!s_timer) {
		ret =  - ENOMEM;
		printk(KERN_ERR " %s FAIL TO  Request Time\n", __func__);
		return -1;
	}
	init_timer(s_timer);
	s_timer->function = &di_timer_handle;

	di_wq = create_singlethread_workqueue("di_wq");
	if (!di_wq) {
		printk(KERN_ERR "Creat DE-Interlace workqueue failed.\n");
		ret = -ENOMEM;
		goto create_work_err;
	}

	INIT_WORK(&di_work, di_work_func);

	if (sunxi_di_major == -1) {
		if ((sunxi_di_major = register_chrdev (0, DI_MODULE_NAME, &sunxi_di_fops)) < 0) {
			printk(KERN_ERR "%s: Failed to register character device\n", __func__);
			ret = -1;
			goto register_chrdev_err;
		} else
			dprintk(DEBUG_INIT, "%s: sunxi_di_major = %d\n", __func__, sunxi_di_major);
	}

	di_dev_class = class_create(THIS_MODULE, DI_MODULE_NAME);
	if (IS_ERR(di_dev_class))
		return -1;
	di_device = device_create(di_dev_class, NULL,  MKDEV(sunxi_di_major, 0), NULL, DI_MODULE_NAME);

	ret = sunxi_di_params_init();
	if (ret) {
		printk(KERN_ERR "%s di init params failed!\n", __func__);
		goto init_para_err;
	}

#ifdef CONFIG_PM
	di_data.di_pm_domain.ops.suspend = sunxi_di_suspend;
	di_data.di_pm_domain.ops.resume = sunxi_di_resume;
	di_device->pm_domain = &(di_data.di_pm_domain);
#endif

	return 0;

init_para_err:
	if (sunxi_di_major > 0) {
		device_destroy(di_dev_class, MKDEV(sunxi_di_major, 0));
		class_destroy(di_dev_class);
		unregister_chrdev(sunxi_di_major, DI_MODULE_NAME);
	}
register_chrdev_err:
	cancel_work_sync(&di_work);
	if (NULL != di_wq) {
		flush_workqueue(di_wq);
		destroy_workqueue(di_wq);
		di_wq = NULL;
	}
create_work_err:
	kfree(s_timer);

	return ret;
}

static void __exit sunxi_di_exit(void)
{
	sunxi_di_params_exit();
	if (sunxi_di_major > 0) {
		device_destroy(di_dev_class, MKDEV(sunxi_di_major, 0));
		class_destroy(di_dev_class);
		unregister_chrdev(sunxi_di_major, DI_MODULE_NAME);
	}
	cancel_work_sync(&di_work);
	if (NULL != di_wq) {
		flush_workqueue(di_wq);
		destroy_workqueue(di_wq);
		di_wq = NULL;
	}
	kfree(s_timer);

	printk(KERN_INFO "%s: module unloaded\n", __func__);
}
 module_init(sunxi_di_init);
 module_exit(sunxi_di_exit);
 module_param_named(debug_mask, debug_mask, int, 0644);
 MODULE_DESCRIPTION("DE-Interlace driver");
 MODULE_AUTHOR("Ming Li<liming@allwinnertech.com>");
 MODULE_LICENSE("GPL");


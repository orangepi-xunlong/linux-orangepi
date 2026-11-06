/*
 * Cvitek SoCs Reset Controller driver
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
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/compat.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "cvi_mon_interface.h"
#include "mon_platform.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
#include <linux/sched/signal.h>
#endif

#define CVI_MON_CDEV_NAME "cvi-mon"
#define CVI_MON_CLASS_NAME "cvi-mon"

struct cvi_list_node {
	struct device *dev;
	struct list_head list;
	pid_t pid;
	uint32_t seq_no;
	int dmabuf_fd;
//	enum dma_data_direction dma_dir;
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *dma_attach;
	struct sg_table *dma_sgt;
	void *dmabuf_vaddr;
	uint64_t dmabuf_paddr;
	int ret;
};

struct cvi_mon_bw_info {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 timer_start;
#else
	struct timespec timer_start;
#endif
	uint32_t timer_current_us;
	struct hrtimer hr_timer;
	uint8_t hr_timer_enable;
};

#define TASK_LIST_MAX 100
#define DONE_LIST_MAX 1000
#ifndef CONFIG_ARCH_CV186X
#define MON_PRC_NAME "cvitek/mon"
#else
#define MON_PRC_NAME "mon"
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
typedef struct legacy_timer_emu {
	struct timer_list t;
	void (*function)(unsigned long);
	unsigned long data;
} _timer;
#else
typedef struct timer_list _timer;
#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))

struct class *mon_class;
static dev_t mon_cdev_id;
struct proc_dir_entry *mon_proc_dir;

static uint32_t mon_window_ms = 20;
static uint32_t enable_mon_bw_profiling;
static struct cvi_mon_bw_info mon_bw_info = {0};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
#if 0
static void legacy_timer_emu_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);

	lt->function(lt->data);
}
#endif
#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))



#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static inline long get_duration_us(const struct timespec64 *start, const struct timespec64 *end)
#else
static inline long get_duration_us(const struct timespec *start, const struct timespec *end)
#endif
{
	long event_duration_us = (end->tv_nsec - start->tv_nsec) / 1000;

	event_duration_us += (end->tv_sec - start->tv_sec) * 1000000;

	return event_duration_us;
}

//static void cvi_mon_bw_profile_timer_handler(unsigned long data)
enum hrtimer_restart cvi_mon_bw_profile_timer_handler(struct hrtimer *timer)
{
	//stop last
	axi_mon_snapshot_all();

	//read value
	axi_mon_get_info_all(mon_bw_info.timer_current_us);

	hrtimer_forward_now(&mon_bw_info.hr_timer, ms_to_ktime(mon_window_ms));
	return HRTIMER_RESTART;
}

static void cvi_mon_bw_profile_timer_remove(void)
{
	hrtimer_cancel(&mon_bw_info.hr_timer);
}

static int __init cvi_mon_bw_profile_timer_init(void)
{
	hrtimer_init(&mon_bw_info.hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mon_bw_info.hr_timer.function = &cvi_mon_bw_profile_timer_handler;
	hrtimer_start(&mon_bw_info.hr_timer, ms_to_ktime(mon_window_ms), HRTIMER_MODE_REL);
	return 0;
}

static int cvi_mon_bw_proc_show(struct seq_file *m, void *v)
{
	if (enable_mon_bw_profiling) {
		seq_puts(m, "mon bw profiling is enabled\n");
	} else {
		seq_puts(m, "mon bw profiling is disabled\n");
	}
	return 0;
}

static ssize_t cvi_mon_bw_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	enable_mon_bw_profiling = input_param;

	if (mon_bw_info.hr_timer.function)
		cvi_mon_bw_profile_timer_remove();

	memset(&mon_bw_info, 0, sizeof(struct cvi_mon_bw_info));

	if (enable_mon_bw_profiling) {
		cvi_mon_bw_profile_timer_init();
		axi_mon_reset_all();
		axi_mon_start_all();
		pr_err("bandwidth profiling is started\n");
	} else {
		axi_mon_stop_all();
		axi_mon_dump();
		axi_mon_reset_all();
		pr_err("bandwidth profiling is ended\n");
	}

	return count;
}

static int cvi_mon_bw_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cvi_mon_bw_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops mon_bw_proc_ops = {
	.proc_open = cvi_mon_bw_proc_open,
	.proc_read = seq_read,
	.proc_write = cvi_mon_bw_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations mon_bw_proc_ops = {
	.owner = THIS_MODULE,
	.open = cvi_mon_bw_proc_open,
	.read = seq_read,
	.write = cvi_mon_bw_proc_write,
	.release = single_release,
};
#endif

static ssize_t cvi_mon_window_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect\n");
		return count;
	}

	//reset related info
	mon_window_ms = input_param;
	return count;
}

static int cvi_mon_window_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "mon bw window=%dms\n", mon_window_ms);
	return 0;
}

static int cvi_mon_window_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cvi_mon_window_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops mon_window_proc_ops = {
	.proc_open = cvi_mon_window_proc_open,
	.proc_read = seq_read,
	.proc_write = cvi_mon_window_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations mon_window_proc_ops = {
	.owner = THIS_MODULE,
	.open = cvi_mon_window_proc_open,
	.read = seq_read,
	.write = cvi_mon_window_proc_write,
	.release = single_release,
};
#endif

static irqreturn_t cvi_aximon_irq(int irq, void *data)
{
	return IRQ_NONE;
}

static void cvi_mon_cleanup_done_list(struct cvi_mon_device *ndev,
				     struct cvi_mon_work *mon_work)
{
	u32 done_list_count = 0;
	struct cvi_list_node *pos, *tmp;
	struct task_struct *task;
	bool alive;

	spin_lock(&mon_work->done_list_lock);

	list_for_each_entry(pos, &mon_work->done_list, list) {
		done_list_count++;
	}

	if (done_list_count < DONE_LIST_MAX) {
		spin_unlock(&mon_work->done_list_lock);
		return;
	}

	dev_info(ndev->dev, "done list too much node, clean up\n");

	list_for_each_entry_safe(pos, tmp, &mon_work->done_list, list) {
		alive = false;
		for_each_process(task) {
			if (pos->pid == task->pid) {
				alive = true;
				break;
			}
		}
		if (!alive) {
			list_del(&pos->list);
			vfree(pos);
		}
	}

	spin_unlock(&mon_work->done_list_lock);
}

static void mon_thread_run(struct cvi_mon_device *ndev)
{
	struct cvi_mon_work *mon_work = &ndev->mon_work;
	struct cvi_list_node *first_node;

	spin_lock(&mon_work->task_list_lock);
	first_node = list_first_entry(&mon_work->task_list,
				      struct cvi_list_node, list);
	list_del(&first_node->list);
	spin_unlock(&mon_work->task_list_lock);

	spin_lock(&mon_work->done_list_lock);
	list_add_tail(&first_node->list, &mon_work->done_list);
	spin_unlock(&mon_work->done_list_lock);

	wake_up_all(&mon_work->done_wait_queue);

	cvi_mon_cleanup_done_list(ndev, mon_work);
}

static int task_list_empty(struct cvi_mon_work *mon_work)
{
	int ret;

	spin_lock(&mon_work->task_list_lock);
	ret = list_empty(&mon_work->task_list);
	spin_unlock(&mon_work->task_list_lock);

	return ret;
}

static void work_thread_exit(struct cvi_mon_device *ndev)
{
	struct cvi_mon_work *mon_work = &ndev->mon_work;
	struct cvi_list_node *pos, *tmp;

	spin_lock(&mon_work->task_list_lock);
	list_for_each_entry_safe(pos, tmp, &mon_work->task_list, list) {
		list_del(&pos->list);
		vfree(pos);
	}
	spin_unlock(&mon_work->task_list_lock);

	spin_lock(&mon_work->done_list_lock);
	list_for_each_entry_safe(pos, tmp, &mon_work->done_list, list) {
		list_del(&pos->list);
		vfree(pos);
	}
	spin_unlock(&mon_work->done_list_lock);
}

static int work_thread_main(void *data)
{
	struct cvi_mon_device *ndev = (struct cvi_mon_device *)data;
	struct cvi_mon_work *mon_work = &ndev->mon_work;

	dev_dbg(ndev->dev, "enter mon work thread\n");

	while (!kthread_should_stop()) {
		wait_event_interruptible(mon_work->task_wait_queue,
					 !task_list_empty(mon_work) ||
						 kthread_should_stop());

		if (!task_list_empty(mon_work))
			mon_thread_run(ndev);
	}

	work_thread_exit(ndev);

	dev_dbg(ndev->dev, "exit mon work thread\n");

	return 0;
}

static int work_thread_init(struct cvi_mon_device *ndev)
{
	struct cvi_mon_work *mon_work = &ndev->mon_work;

	init_waitqueue_head(&mon_work->task_wait_queue);
	init_waitqueue_head(&mon_work->done_wait_queue);
	INIT_LIST_HEAD(&mon_work->task_list);
	spin_lock_init(&mon_work->task_list_lock);
	INIT_LIST_HEAD(&mon_work->done_list);
	spin_lock_init(&mon_work->done_list_lock);

	mon_work->work_thread =
		kthread_run(work_thread_main, ndev, "cvi_mon_work");
	if (IS_ERR(mon_work->work_thread)) {
		dev_err(ndev->dev, "kthread run fail\n");
		return PTR_ERR(mon_work->work_thread);
	}

	return 0;
}

static long cvi_mon_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int cvi_mon_open(struct inode *inode, struct file *filp)
{
	struct cvi_mon_device *ndev =
		container_of(inode->i_cdev, struct cvi_mon_device, cdev);
	unsigned long flags = 0;

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count++;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	filp->private_data = ndev;

	return 0;
}

static int cvi_mon_close(struct inode *inode, struct file *filp)
{
	unsigned long flags = 0;
	struct cvi_mon_device *ndev =
		container_of(inode->i_cdev, struct cvi_mon_device, cdev);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count--;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	filp->private_data = NULL;

	return 0;
}

#ifdef CONFIG_COMPAT
static long cvi_mon_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations mon_fops = {
	.owner = THIS_MODULE,
	.open = cvi_mon_open,
	.release = cvi_mon_close,
	.unlocked_ioctl = cvi_mon_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cvi_mon_compat_ptr_ioctl,
#endif
};

int cvi_mon_register_cdev(struct cvi_mon_device *ndev)
{
	int ret;

	mon_class = class_create(THIS_MODULE, CVI_MON_CLASS_NAME);
	if (IS_ERR(mon_class)) {
		pr_err("create mon class failed\n");
		return PTR_ERR(mon_class);
	}

	ret = alloc_chrdev_region(&mon_cdev_id, 0, 1, CVI_MON_CDEV_NAME);
	if (ret < 0) {
		pr_err("alloc mon chrdev failed\n");
		return ret;
	}

	cdev_init(&ndev->cdev, &mon_fops);
	ndev->cdev.owner = THIS_MODULE;
	cdev_add(&ndev->cdev, mon_cdev_id, 1);

	device_create(mon_class, ndev->dev, mon_cdev_id, NULL, "%s%d",
		      CVI_MON_CDEV_NAME, 0);

	return 0;
}

static int cvi_mon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_mon_device *ndev;
	struct resource *res;
	int ret;

	pr_debug("===cvi_mon_probe start\n");

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pcmon");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve pcmon io\n");
		return -ENXIO;
	}
	ndev->pcmon_vaddr = devm_ioremap_resource(&pdev->dev, res);

#ifndef CONFIG_ARCH_CV186X
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_ctrl");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve aximon io\n");
		return -ENXIO;
	}
	ndev->ddr_ctrl_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_phyd");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve aximon io\n");
		return -ENXIO;
	}
	ndev->ddr_phyd_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_aximon");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve aximon io\n");
		return -ENXIO;
	}
	ndev->ddr_aximon_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_top");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve aximon io\n");
		return -ENXIO;
	}
	ndev->ddr_top_vaddr = devm_ioremap_resource(&pdev->dev, res);
#else
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_ctrl");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_ctrl io\n");
		return -ENXIO;
	}
	ndev->ddr_ctrl_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_phyd");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_phyd io\n");
		return -ENXIO;
	}
	ndev->ddr_phyd_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_aximon_real");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_aximon_real io\n");
		return -ENXIO;
	}
	ndev->ddr_aximon_real_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_aximon_offline");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_aximon_real io\n");
		return -ENXIO;
	}
	ndev->ddr_aximon_offline_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_aximon_bulk");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_aximon_real io\n");
		return -ENXIO;
	}
	ndev->ddr_aximon_bulk_vaddr = devm_ioremap_resource(&pdev->dev, res);
#if 0
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_ddrmon_sys1");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_ddrmon_sys1 io\n");
		return -ENXIO;
	}
	ndev->ddr_ddrmon_sys1_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_ddrmon_sys2");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_ddrmon_sys2 io\n");
		return -ENXIO;
	}
	ndev->ddr_ddrmon_sys2_vaddr = devm_ioremap_resource(&pdev->dev, res);
#endif
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr_top");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve ddr_top io\n");
		return -ENXIO;
	}
	ndev->ddr_top_vaddr = devm_ioremap_resource(&pdev->dev, res);
#endif
	ndev->aximon_irq = platform_get_irq(pdev, 0);
	if (ndev->aximon_irq < 0) {
		dev_err(dev, "failed to retrieve aximon irq");
		return -ENXIO;
	}

	ret = devm_request_irq(&pdev->dev, ndev->aximon_irq, cvi_aximon_irq, 0,
			       "cvi-aximon", ndev);
	if (ret)
		return -ENXIO;

	init_completion(&ndev->aximon_completion);
	mutex_init(&ndev->dev_lock);
	spin_lock_init(&ndev->close_lock);

	ret = cvi_mon_register_cdev(ndev);
	if (ret < 0) {
		pr_err("regsiter chrdev error\n");
		return ret;
	}

	ret = work_thread_init(ndev);
	if (ret < 0) {
		pr_err("work thread init error\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);

	//create mon proc
	mon_proc_dir = proc_mkdir(MON_PRC_NAME, NULL);

#if 0
	if (proc_create_data("mon_profiling", 0644, mon_proc_dir, &mon_proc_ops, ndev) == NULL)
		pr_err("mon profiling proc creation failed\n");
#endif

	if (proc_create_data("bw_profiling", 0644, mon_proc_dir, &mon_bw_proc_ops, ndev) == NULL)
		pr_err("mon bw_profiling proc creation failed\n");

	if (proc_create_data("profiling_window_ms", 0644, mon_proc_dir, &mon_window_proc_ops, ndev) == NULL)
		pr_err("mon profiling_window_ms proc creation failed\n");

	axi_mon_init(ndev);

	pr_debug("===cvi_mon_probe end\n");
	return 0;
}

static int cvi_mon_remove(struct platform_device *pdev)
{
	struct cvi_mon_device *ndev = platform_get_drvdata(pdev);
	struct cvi_mon_work *mon_work = &ndev->mon_work;

	kthread_stop(mon_work->work_thread);
	device_destroy(mon_class, mon_cdev_id);

	cdev_del(&ndev->cdev);
	unregister_chrdev_region(mon_cdev_id, 1);
	class_destroy(mon_class);

	platform_set_drvdata(pdev, NULL);
	pr_debug("===cvi_mon_remove\n");

	proc_remove(mon_proc_dir);
	return 0;
}

static const struct of_device_id cvi_mon_match[] = {
	{ .compatible = "cvitek,mon" },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_mon_match);

static struct platform_driver cvi_mon_driver = {
	.probe = cvi_mon_probe,
	.remove = cvi_mon_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = "cvi-mon",
			.of_match_table = cvi_mon_match,
		},
};
module_platform_driver(cvi_mon_driver);

MODULE_AUTHOR("Wellken Chen<wellken.chen@cvitek.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC MON driver");
MODULE_LICENSE("GPL");


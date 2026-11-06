// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ive_interface.c
 * Description: ive kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <asm/atomic.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/signal.h>
#endif
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/compat.h>

#include "ive_interface.h"
#include "ive_platform.h"
#include "ive_debug.h"

#define IVE_CDEV_NAME "soph-ive"
#define IVE_CLASS_NAME "soph-ive"
#define IVE_PROC_NAME "soph/ive_hw_profiling"

static const char *const ive_clk_name[IVE_DEV_MAX][2] = {
						{"clk_vi_sys3", "clk_ive0"},
						{"clk_vi_sys3", "clk_ive1"}};


#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
uint32_t get_duration_us(const struct timespec64 *start,
				const struct timespec64 *end)
#else
uint32_t get_duration_us(const struct timespec *start,
				const struct timespec *end)
#endif
{
	uint32_t event_duration_us;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 temp;
#else
	struct timespec temp;
#endif
	if ((end->tv_nsec - start->tv_nsec) < 0) {
		temp.tv_sec = end->tv_sec - start->tv_sec - 1;
		temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
	} else {
		temp.tv_sec = end->tv_sec - start->tv_sec;
		temp.tv_nsec = end->tv_nsec - start->tv_nsec;
	}
	event_duration_us = (uint32_t) (temp.tv_nsec / 1000);
	event_duration_us += temp.tv_sec * 1000000;

	return event_duration_us;
}

#define IVE_IRQ_THREAD_FUNC 0
u32 ive_log_lv = IVE_DBG_ERR;
// static char *g_kdata;
struct class *class_id;
static dev_t cdev_id;
static uint32_t g_enable_usage_profiling;
static struct ive_profiling_info *g_time_infos;
//update proc info
static void ive_update_timer(struct timer_list *timer);
DEFINE_TIMER(timer_proc, ive_update_timer);
static atomic_t g_timer_added = ATOMIC_INIT(0);

//timer callback
static ive_timer_cb g_core_cb;
static void *g_core_data;

const char * const ive_irq_name[IVE_DEV_MAX] = {"ive_irq0", "ive_irq1"};

// proc_operations function
static int ive_proc_open(struct inode *inode, struct file *file);
static ssize_t ive_proc_write(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos);
// file_operations function
static int ive_open(struct inode *inode, struct file *filp);
static int ive_close(struct inode *inode, struct file *filp);
static long ive_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg);
#ifdef CONFIG_COMPAT
static long ive_compat_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long arg);
#endif
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops ive_proc_ops = {
	.proc_open = ive_proc_open,
	.proc_read = seq_read,
	.proc_write = ive_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations ive_proc_ops = {
	.owner = THIS_MODULE,
	.open = ive_proc_open,
	.read = seq_read,
	.write = ive_proc_write,
	.release = single_release,
};
#endif

static const struct file_operations ive_fops = {
	.owner = THIS_MODULE,
	.open = ive_open,
	.release = ive_close,
	.unlocked_ioctl = ive_ioctl, //2.6.36
#ifdef CONFIG_COMPAT
	.compat_ioctl = ive_compat_ioctl, //2.6.36
#endif
};

void register_timer_fun(ive_timer_cb cb, void *data)
{
	g_core_cb = cb;
	g_core_data = data;
}

static void start_ioctl_time(struct ive_profiling_info *pinfo, char *name)
{
	int i = 0;

	if (g_enable_usage_profiling) {
		strcpy(pinfo->op_name, name);
		pinfo->time_ioctl_diff_us = 0;
		for (i = 0; i < 6; i++) {
			pinfo->time_vld_diff_us[i] = 0;
		}
		pinfo->time_tile_diff_us = 0;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&pinfo->time_ioctl_start);
#else
		getnstimeofday(&pinfo->time_ioctl_start);
#endif
	}
}

static void stop_ioctl_time(struct ive_profiling_info *pinfo)
{
	if (g_enable_usage_profiling) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&pinfo->time_ioctl_end);
#else
		getnstimeofday(&pinfo->time_ioctl_end);
#endif
		pinfo->time_ioctl_diff_us =
			get_duration_us(&pinfo->time_ioctl_start, &pinfo->time_ioctl_end);
	}
}

void start_vld_time(int optype, struct ive_dev_core* core)
{
	if (g_enable_usage_profiling && optype < MOD_ALL &&
		optype >= MOD_BYP &&
		strlen(g_time_infos[optype].op_name) > 0) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&g_time_infos[optype].time_vld_start);
#else
		getnstimeofday(&g_time_infos[optype].time_vld_start);
#endif
		core->ts_start = g_time_infos[optype].time_vld_start;
	}
}

void stop_vld_time(int optype, int tile_num, struct ive_dev_core* core)
{
	if (tile_num > 6)
		return;
	if (g_enable_usage_profiling && optype < MOD_ALL &&
		optype >= MOD_BYP &&
		strlen(g_time_infos[optype].op_name) > 0) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&g_time_infos[optype].time_vld_end);
#else
		getnstimeofday(&g_time_infos[optype].time_vld_end);
#endif
		g_time_infos[optype].time_vld_diff_us[tile_num] =
			get_duration_us(&g_time_infos[optype].time_vld_start,
			&g_time_infos[optype].time_vld_end);
	}
	g_time_infos[optype].time_tile_diff_us +=
		g_time_infos[optype].time_vld_diff_us[tile_num];
	core->ts_end = g_time_infos[optype].time_vld_end;
	core->hw_duration = get_duration_us(&core->ts_start, &core->ts_end);
	core->hw_duration_total += core->hw_duration;
}

void ive_timer_core_update(void *data)
{

	int i;
	int duration;
	struct ive_device *dev = (struct ive_device *)data;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		struct timespec64 cur_time;
		static struct timespec64 pre_time = {0};
		ktime_get_real_ts64(&cur_time);
#else
		struct timespec cur_time;
		static struct timespec pre_time = {0};
		getnstimeofday(&cur_time);
#endif

	duration = get_duration_us(&pre_time, &cur_time);
	pre_time = cur_time;

	if (duration > 2000000)
		return;

	for (i = DEV_IVE_0; i < DEV_IVE_MAX; ++i) {
		dev->core[i].duty_ratio = (dev->core[i].hw_duration_total * 100) / duration;
		dev->core[i].hw_duration_total = 0;
	}
}

static void ive_update_timer(struct timer_list *timer)
{
	(void)(timer);
	g_core_cb(g_core_data);
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
}

#if IVE_IRQ_THREAD_FUNC
static void ive_frame_finish(struct ive_device *dev, int dev_id)
{
	//struct ive_dev_core *ive_core = &dev->core[dev_id];

	return;

	//to do
#if 0
	if (atomic_read(&core->state) != IVE_CORE_STATE_END)
		return;

	atomic_set(&core->state, IVE_CORE_STATE_IDLE);
#endif
}

static irqreturn_t ive_irq_finish_thread_func(int irq, void *data)
{
	struct ive_device *ndev = (struct ive_device *)data;
	int i, dev_id = -1;

	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "invalid ive_device\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < IVE_DEV_MAX; i++) {
		if (ndev->ive_irq[i] == irq) {
			dev_id = i;
			break;
		}
	}

	if (dev_id >= 0 && dev_id < IVE_DEV_MAX)
		ive_frame_finish(ndev, dev_id);
	else
		TRACE_IVE(IVE_DBG_ERR, "invalid ive_dev num[%d]\n", dev_id);
	return IRQ_HANDLED;
}
#endif

static irqreturn_t ive_irq_handler(int irq, void *data)
{
	struct ive_device *ndev = data;
	irqreturn_t ret;
	int i, dev_id = -1;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		if (ndev->ive_irq[i] == irq) {
			dev_id = i;
			break;
		}
	}

	spin_lock(&ndev->core[dev_id].dev_lock);
	//pr_info("[IVE] ive use_count %d\n", ndev->use_count);
	if (ndev->use_count == 0) {
		atomic_set(&ndev->core[i].dev_state, IVE_CORE_STATE_END);
		spin_unlock(&ndev->core[dev_id].dev_lock);
		return IRQ_HANDLED;
	}

	ret = platform_ive_irq(ndev, dev_id);
	atomic_set(&ndev->core[i].dev_state, IVE_CORE_STATE_END);
	spin_unlock(&ndev->core[dev_id].dev_lock);

	return ret;
}

static int ive_proc_show(struct seq_file *m, void *v)
{
	int i = 0, tile = 0;
	struct ive_device *ndev= (struct ive_device *)m->private;
	if (g_enable_usage_profiling) {
		char const *row_name[] = {"op name", "start(s)", "ioctl(us)",
							"tile0(us)", "tile1(us)", "tile2(us)", "tile3(us)",
							"tile4(us)", "tile5(us)", "tileSum(us)"};
		int row_space[] = { -15, 10, 10, 10, 10, 10, 10, 10, 10, 10};
		int table[] = { 20, 21, 22, 23, 24, 3, 2, 25, 26, 27,
						28, 31, 33, 35, 1, 29, 30, 4, 6, 7,
						8, 9, 10, 11, 15, 16, 17, 19, 18, 36,
						12, 34, 13, 14, 32, 5};

		seq_puts(m, "[IVE] ive time profiling\n");
		seq_printf(m, "%*s| %*s| %*s| %*s| %*s| %*s| %*s| %*s| %*s| %*s\n",
		row_space[0], row_name[0], row_space[1], row_name[1],
		row_space[2], row_name[2], row_space[3], row_name[3],
		row_space[4], row_name[4], row_space[5], row_name[5],
		row_space[6], row_name[6], row_space[7], row_name[7],
		row_space[8], row_name[8], row_space[9], row_name[9]);

		for (i = 0; i < 36; i++) {
			uint32_t second_vld_time[6] = {0};
			uint32_t second_tile_time = 0;
			uint32_t id = table[i];

			if (strlen(g_time_infos[id].op_name) > 0) {
				if (id == 10) {
					for (tile = 0; tile < 6; tile++) {
						second_vld_time[tile] = g_time_infos[5].time_vld_diff_us[tile];
					}
					second_tile_time = g_time_infos[5].time_tile_diff_us;
				} else if (id == 5) {
					continue;
				}
				seq_printf(
					m, "%*s| %*lld| %*u| %*d| %*d| %*d| %*d| %*d| %*d| %*d\n",
					row_space[0],
					g_time_infos[id].op_name,
					row_space[1],
					g_time_infos[id].time_ioctl_start.tv_sec,
					row_space[2],
					g_time_infos[id].time_ioctl_diff_us,
					row_space[3],
					g_time_infos[id].time_vld_diff_us[0] + second_vld_time[0],
					row_space[4],
					g_time_infos[id].time_vld_diff_us[1] + second_vld_time[1],
					row_space[5],
					g_time_infos[id].time_vld_diff_us[2] + second_vld_time[2],
					row_space[6],
					g_time_infos[id].time_vld_diff_us[3] + second_vld_time[3],
					row_space[7],
					g_time_infos[id].time_vld_diff_us[4] + second_vld_time[4],
					row_space[8],
					g_time_infos[id].time_vld_diff_us[5] + second_vld_time[5],
					row_space[9],
					g_time_infos[id].time_tile_diff_us + second_tile_time);
			}
		}
		seq_printf(m, "[IVE CORE 0] duty_ratio = %d%%\n", ndev->core[DEV_IVE_0].duty_ratio);
		seq_printf(m, "[IVE CORE 1] duty_ratio = %d%%\n", ndev->core[DEV_IVE_1].duty_ratio);
	} else {
		seq_puts(m, "[IVE] ive time profiling is disabled\n");
	}
	return 0;
}

static ssize_t ive_proc_write(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	uint32_t user_input_param = 0, i = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &user_input_param)) {
		pr_err("\n[IVE] input parameter incorrect\n");
		return count;
	}

	// reset related info
	if (user_input_param == 0) {
		g_enable_usage_profiling = 0;
		pr_err("\n[IVE] Time profiling is ended\n");
	} else if (user_input_param == 1) {
		for (i = 0; i < MOD_ALL; i++) {
			memset(&g_time_infos[i], 0,
			       sizeof(struct ive_profiling_info));
		}
		g_enable_usage_profiling = 1;
		pr_err("\n[IVE] Time profiling is started\n");
	} else if (user_input_param == 2) {
		ive_set_reg_dump(true);
		pr_err("\n[IVE] Enable dump reg state\n");
	} else if (user_input_param == 3) {
		ive_set_reg_dump(false);
		pr_err("\n[IVE] Disable dump reg state\n");
	} else if (user_input_param == 4) {
		ive_set_dma_dump(true);
		pr_err("\n[IVE] Enable dump dma phy addr\n");
	} else if (user_input_param == 5) {
		ive_set_dma_dump(false);
		pr_err("\n[IVE] Disable dump dma phy addr\n");
	} else if (user_input_param == 6) {
		ive_set_img_dump(true);
		pr_err("\n[IVE] Enable dump ive_image_s, ive_data_s, ive_mem_info_s\n");
	} else if (user_input_param == 7) {
		ive_set_img_dump(false);
		pr_err("\n[IVE] Disable dump ive_image_s, ive_data_s, ive_mem_info_s\n");
	} else if (user_input_param == 10) {
		ive_dump_op1_op2_info();
	} else if (user_input_param == 11) {
		ive_dump_hw_flow();
	} else {
		pr_err("\nIVE Command List:\n"
				"\t0: Set time profiling stop\n"
				"\t1: Set time profiling start\n"
				"\t2: Enable print reg state\n"
				"\t3: Disable print reg state\n"
				"\t4: Enable print dma phy addr\n"
				"\t5: Disable print dma phy addr\n"
				"\t6: Enable print ive_image_s, ive_data_s, ive_mem_info_s\n"
				"\t7: Disable print ive_image_s, ive_data_s, ive_mem_info_s\n"
				"\t10: Dump ive op1/op2 mode info\n"
				"\t11: Dump hardware info\n");
	}
	return count;
}

static int ive_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ive_proc_show, PDE_DATA(inode));
}

#ifdef CONFIG_COMPAT
static long ive_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd,
					  (unsigned long)compat_ptr(arg));
}
#endif
static long ive_add_task(struct ive_device *ndev, unsigned int cmd, unsigned long arg, char *kdata, void *buffer)
{
	struct ive_task *task;
	unsigned long flags;
	s32 sync_io_ret = 1;
	s32 ret = -1;

	unsigned long timeout = msecs_to_jiffies(IVE_SYNC_IO_WAIT_TIMEOUT_MS);
	if (ndev == NULL) {
		TRACE_IVE(IVE_DBG_ERR, "drv point is NULL\n");
		return ret;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);
	task = kzalloc(sizeof(struct ive_task), GFP_ATOMIC);
	atomic_set(&task->state, IVE_TASK_STATE_READY);
	list_add_tail(&task->node, &ndev->tsk_list);
	task->dev_id = -1;
	task->task_type = cmd;
	task->input_data = kdata;
	task->buffer = buffer;
	init_waitqueue_head(&task->task_done_wait);
	task->task_done_evt = false;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	TRACE_IVE(IVE_DBG_DEBUG, "add task success, task[%p], input data [%p], task type[%d]\n"
		, task, task->input_data, _IOC_NR(cmd));

	ive_notify_wkup_evt_kth(ndev, IVE_EVENT_WKUP);

	sync_io_ret = wait_event_timeout(task->task_done_wait, task->task_done_evt, timeout);
	if (sync_io_ret <= 0) {
		TRACE_IVE(IVE_DBG_ERR, "user thread wait timeout, ret[%d]\n", sync_io_ret);
		ret = -1;
	} else
		ret = 0;
	ive_notify_wkup_evt_kth(ndev, IVE_EVENT_EOF);


	if (atomic_read(&task->state) == IVE_TASK_STATE_DONE) {
		kfree(task);
		task = NULL;
	}
	return ret;

}
static void ive_task_done(struct ive_device *ndev, struct ive_task *task)
{
	unsigned long flags;

	if (!task) {
		TRACE_IVE(IVE_DBG_ERR, "task is NULL\n");
		return;
	}

	if (atomic_read(&task->state) == IVE_TASK_STATE_RUNNING) {
		atomic_set(&task->state, IVE_TASK_STATE_DONE);
		atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);

		spin_lock_irqsave(&ndev->close_lock, flags);
		task->task_done_evt = true;
		spin_unlock_irqrestore(&ndev->close_lock, flags);
		wake_up(&task->task_done_wait);

		TRACE_IVE(IVE_DBG_DEBUG, "task done ,wake up user thread\n");
	} else {
		TRACE_IVE(IVE_DBG_DEBUG, "task state is invaild\n");
	}
}

void ive_notify_wkup_evt_kth(void *data, enum ive_wait_evt evt)
{
	struct ive_device *ndev = (struct ive_device *)data;
	unsigned long flags;


	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "ive device isn't created yet\n");
		return;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->evt |= evt;
	TRACE_IVE(IVE_DBG_DEBUG, "wakeup kthread dev evt[%d]", ndev->evt);
	spin_unlock_irqrestore(&ndev->close_lock, flags);

	wake_up_interruptible(&ndev->wait);
}

static long ive_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct ive_device *ndev = filp->private_data;
	struct ive_ioctl_arg *ioctl_arg;
	s32 ret = -1;
	char *kdata = NULL;
	bool copy_buf = false;
	void *buffer = NULL;

	ioctl_arg = (struct ive_ioctl_arg *)arg;
	kdata = kmalloc(512,GFP_KERNEL);
	if (!kdata) {
		TRACE_IVE(IVE_DBG_ERR, "vmalloc fail\n");
		goto error;
	}
	if (copy_from_user(ioctl_arg, (void __user *)arg, sizeof(ioctl_arg)) != 0) {
		TRACE_IVE(IVE_DBG_ERR, "copy to user fail\n");
		goto error;
	}
	if (copy_from_user(kdata, (void __user *)ioctl_arg->input_data, 512) != 0) {
		TRACE_IVE(IVE_DBG_ERR, "copy to user fail\n");
		goto error;
	}

	if (cmd == IVE_IOC_MAP || cmd == IVE_IOC_NCC ||
		cmd == IVE_IOC_CCL || cmd == IVE_IOC_MATCH_BGMODEM ||
		cmd == IVE_IOC_UPDATE_BGMODEL) {

		buffer = kmalloc(ioctl_arg->size,GFP_KERNEL);
		if (!buffer) {
			TRACE_IVE(IVE_DBG_ERR, "vmalloc fail\n");
			goto error;
		}
		copy_buf = true;
		if (copy_from_user(buffer, (void __user *)ioctl_arg->buffer, ioctl_arg->size) != 0) {
			TRACE_IVE(IVE_DBG_ERR, "copy to user fail\n");
			goto error;
		}
	}

	ret = ive_add_task(ndev, cmd, arg, kdata, buffer);


	if (copy_buf) {
		if (copy_to_user((void __user *)ioctl_arg->buffer, buffer, ioctl_arg->size) != 0) {
			TRACE_IVE(IVE_DBG_ERR, "copy to user fail\n");
			ret = -EFAULT;
		}
	}

	if (copy_to_user((void __user *)ioctl_arg->input_data, kdata, 512) != 0) {
		TRACE_IVE(IVE_DBG_ERR, "copy to user fail\n");
		ret = -EFAULT;
	}

	if (copy_to_user((void __user *)arg, ioctl_arg, sizeof(ioctl_arg)) != 0) {
		TRACE_IVE(IVE_DBG_ERR, "copy to user fail\n");
		ret = -EFAULT;
	}
	if (kdata) {
		kfree(kdata);
		kdata = NULL;
	}

	if (copy_buf && buffer) {
		kfree(buffer);
		buffer= NULL;
	}

	return ret;

error:
	if (kdata) {
		kfree(kdata);
		kdata = NULL;
	}

	if (copy_buf && buffer) {
		kfree(buffer);
		buffer= NULL;
	}
	return -1;
}

static void ive_clk_init(struct ive_device *ndev)
{
	u8 i;
	struct ive_dev_core core;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		core = ndev->core[i];
		// if (core.clk_src)
		// 	clk_prepare_enable(core.clk_src);

		if (core.clk_ive) {
			clk_prepare_enable(core.clk_ive);
			TRACE_IVE(IVE_DBG_DEBUG, "clk_enable ive_core[%d]\n", i);
		}

	}
}

static void ive_clk_deinit(struct ive_device *ndev)
{
	u8 i;
	struct ive_dev_core core;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		core = ndev->core[i];

		if (core.clk_ive && __clk_is_enabled(core.clk_ive)) {
			clk_disable_unprepare(core.clk_ive);
			TRACE_IVE(IVE_DBG_DEBUG, "clk_disable ive_core[%d]\n", i);
		}

	}
}
s32 is_ive_suspend(struct ive_device *ndev)
{
	int ret = 0;

	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "dev ptr is null\n");
		return -1;
	}
	if (atomic_read(&ndev->clk_flg) == IVE_READY_SUSPEND)
		ret = 1;
	return ret;
}

static int ive_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ive_device *ndev;
	int ret = 0;
	ndev= dev_get_drvdata(&pdev->dev);

	TRACE_IVE(IVE_DBG_DEBUG, "ive suspend start\n");
	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "dev ptr is null\n");
		return 1;
	}
	if (!ndev->work_thread) {
		TRACE_IVE(IVE_DBG_ERR, "ive thread not initialized yet\n");
		return 1;
	}

	if (!list_empty(&ndev->tsk_list) || atomic_read(&ndev->core[0].dev_state) == IVE_DEV_STATE_RUNNING
		|| atomic_read(&ndev->core[1].dev_state) == IVE_DEV_STATE_RUNNING) {
			sema_init(&ndev->sem, 0);
			ret = down_timeout(&ndev->sem, msecs_to_jiffies(IVE_IDLE_WAIT_TIMEOUT_MS));
			if (ret == -ETIME) {
				TRACE_IVE(IVE_DBG_ERR, "get sem timeout ,dev not idle yet\n");
				return ret;
			}
	}

	atomic_set(&ndev->core[0].dev_state, IVE_DEV_STATE_END);
	atomic_set(&ndev->core[1].dev_state, IVE_DEV_STATE_END);
	atomic_set(&ndev->clk_flg, IVE_READY_SUSPEND);

	if (atomic_cmpxchg(&g_timer_added, 1, 0) == 1)
		del_timer(&timer_proc);

	ive_clk_deinit(ndev);
	pr_info("ive suspended\n");

	return ret;
}
static int ive_resume(struct platform_device *pdev)
{
	struct ive_device *ndev;
	int ret = 0;
	ndev= dev_get_drvdata(&pdev->dev);

	TRACE_IVE(IVE_DBG_DEBUG, "ive resume start\n");
	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "dev ptr is null\n");
		return 1;
	}
	if (!ndev->work_thread) {
		TRACE_IVE(IVE_DBG_ERR, "ive thread not initialized yet\n");
		return 1;
	}

	atomic_set(&ndev->core[0].dev_state, IVE_DEV_STATE_END);
	atomic_set(&ndev->core[1].dev_state, IVE_DEV_STATE_END);
	atomic_set(&ndev->clk_flg, IVE_READY_RESUME);

	ive_clk_init(ndev);
	if (atomic_cmpxchg(&g_timer_added, 0, 1) == 0)
		add_timer(&timer_proc);
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
	pr_info("ive resumed\n");

	return ret;
}


static int ive_open(struct inode *inode, struct file *filp)
{
	//struct ive_device *ndev =
	//	container_of(filp->private_data, struct ive_device, miscdev);
	struct ive_device *ndev =
		container_of(inode->i_cdev, struct ive_device, cdev);
	unsigned long flags = 0;

	//open clk
	// ive_clk_init(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count++;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	filp->private_data = ndev;
	return 0;
}

static int ive_close(struct inode *inode, struct file *filp)
{
	struct ive_device *ndev = filp->private_data;
	unsigned long flags = 0;

	//close clk
	// ive_clk_deinit(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count--;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	filp->private_data = NULL;

	return 0;
}

//int cvi_ive_register_misc(struct ive_device *ndev)
//{
//	int rc;
//
//	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
//	ndev->miscdev.name = IVE_CDEV_NAME;
//	ndev->miscdev.fops = &ive_fops;
//
//	rc = misc_register(&ndev->miscdev);
//	if (rc) {
//		dev_err(ndev->dev,
//		"cvi_ive: failed to register misc device.\n");
//		return rc;
//	}
//
//	return 0;
//}

int ive_register_cdev(struct ive_device *ndev)
{
	int ret;
	// Create device to /sys/class/
	class_id = class_create(THIS_MODULE, IVE_CLASS_NAME);
	if (IS_ERR(class_id)) {
		pr_err("[IVE] create class failed\n");
		return PTR_ERR(class_id);
	}
	// Apply for a character device driver id (cdev)
	ret = alloc_chrdev_region(&cdev_id, 0, 1, IVE_CDEV_NAME);
	if (ret < 0) {
		pr_err("[IVE] alloc chrdev failed\n");
		return ret;
	}
	// Init character device and link file_ops
	cdev_init(&ndev->cdev, &ive_fops);
	ndev->cdev.owner = THIS_MODULE;
	// Add cdev to kernel character device list
	cdev_add(&ndev->cdev, cdev_id, 1);
	// Automatically create device node under /dev/
	device_create(class_id, ndev->dev, cdev_id, NULL, "%s",
		      IVE_CDEV_NAME);

	return 0;
}

static u8 ive_get_idle_coreid(struct ive_device *ndev)
{
	u8 coreid;
	enum ive_core_state state;

	for (coreid = 0; coreid < IVE_DEV_MAX; coreid++) {
		state = atomic_read(&ndev->core[coreid].dev_state);
		if (state == IVE_CORE_STATE_END)
			break;
	}

	return coreid;
}

static void ive_submit_hw(struct ive_device *ndev, int top_id, struct ive_task *task)
{
	unsigned long flags;
	s32 ret = -1;
	char *g_kdata;

	g_kdata = task->input_data;
	spin_lock_irqsave(&ndev->close_lock, flags);
	task->dev_id = top_id;
	atomic_set(&task->state, IVE_TASK_STATE_RUNNING);
	atomic_set(&ndev->core[top_id].dev_state, IVE_CORE_STATE_RUNNING);
	ndev->core[top_id].work_tsk = task;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	TRACE_IVE(IVE_DBG_DEBUG, "use core[%d]\n", task->dev_id);
	TRACE_IVE(IVE_DBG_DEBUG, "task type[%d], input ptr[%p]\n", _IOC_NR(task->task_type), task->input_data);


		switch (task->task_type) {
		case IVE_IOC_QUERY: {
			bool bFinish;
			struct ive_query_arg *val =
					(struct ive_query_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_QUERY], "QUERY");
			ret = ive_query(ndev, &bFinish, val->bBlock, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_QUERY]);
		} break;
		case IVE_IOC_RESET: {
			start_ioctl_time(&g_time_infos[MOD_RESET], "RESET");
			ret = _ive_reset(ndev, *((int *) g_kdata), task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_RESET]);
		} break;
		case IVE_IOC_DUMP: {
			start_ioctl_time(&g_time_infos[MOD_DUMP], "DUMP");
			ret = ive_dump_reg_state(true);    // not use dev_id ,use d_num
			stop_ioctl_time(&g_time_infos[MOD_DUMP]);
		} break;
		case IVE_IOC_TEST: {
			struct ive_test_arg *val =
					(struct ive_test_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_TEST], "Test");
			ret = ive_test(ndev, val->addr, &val->width,
						&val->height);
			stop_ioctl_time(&g_time_infos[MOD_TEST]);
		} break;
		case IVE_IOC_DMA: {
			struct ive_ioctl_dma_arg *val =
					(struct ive_ioctl_dma_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_DMA], "DMA");
			ret = ive_dma(ndev, &val->src, &val->dst,
						&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_DMA]);
		} break;
		case IVE_IOC_AND: {
			struct ive_ioctl_and_arg *val =
					(struct ive_ioctl_and_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_AND], "And");
			ret = ive_and(ndev, &val->src1, &val->src2,
						&val->dst, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_AND]);
		} break;
		case IVE_IOC_OR: {
			struct ive_ioctl_or_arg *val =
					(struct ive_ioctl_or_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_OR], "Or");
			ret = ive_or(ndev, &val->src1, &val->src2,
						&val->dst, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_OR]);
		} break;
		case IVE_IOC_XOR: {
			struct ive_ioctl_xor_arg *val =
					(struct ive_ioctl_xor_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_XOR], "Xor");
			ret = ive_xor(ndev, &val->src1, &val->src2,
						&val->dst, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_XOR]);
		} break;
		case IVE_IOC_ADD: {
			struct ive_ioctl_add_arg *val =
					(struct ive_ioctl_add_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ADD], "Add");
			ret = ive_add(ndev, &val->src1, &val->src2,
						&val->dst, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ADD]);
		} break;
		case IVE_IOC_SUB: {
			struct ive_ioctl_sub_arg *val =
					(struct ive_ioctl_sub_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_SUB], "Sub");
			ret = ive_sub(ndev, &val->src1, &val->src2,
						&val->dst, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_SUB]);
		} break;
		case IVE_IOC_THRESH: {
			struct ive_ioctl_thresh_arg *val =
					(struct ive_ioctl_thresh_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_THRESH], "Thresh");
			ret = ive_thresh(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_THRESH]);
		} break;
		case IVE_IOC_DILATE: {
			struct ive_ioctl_dilate_arg *val =
					(struct ive_ioctl_dilate_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_DILA], "Dilate");
			ret = ive_dilate(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_DILA]);
		} break;
		case IVE_IOC_ERODE: {
			struct ive_ioctl_erode_arg *val =
					(struct ive_ioctl_erode_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ERO], "Erode");
			ret = ive_erode(ndev, &val->src, &val->dst,
						&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ERO]);
		} break;
		case IVE_IOC_MATCH_BGMODEM: {
			struct ive_ioctl_match_bgmodel_arg *val =
					(struct ive_ioctl_match_bgmodel_arg *) g_kdata;

			task->buffer = (ive_bg_stat_data_s *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_BGM], "MatchBgModel");
			ret = ive_match_bg_model(ndev, &val->cur_img,
							&val->bg_model,
							&val->fg_flag, &val->stDiffFg,
							task->buffer, &val->ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BGM]);
		} break;
		case IVE_IOC_UPDATE_BGMODEL: {
			struct ive_ioctl_update_bgmodel_arg *val =
					(struct ive_ioctl_update_bgmodel_arg *) g_kdata;

			task->buffer = (ive_bg_stat_data_s *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_BGU], "UpdateBgModel");
			ret = ive_update_bg_model(ndev, &val->cur_img,
							&val->bg_model,
							&val->fg_flag, &val->bg_img,
							&val->chg_sta,
							task->buffer,
							&val->ctrl, val->instant,
							task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BGU]);
		} break;
		case IVE_IOC_GMM: {
			struct ive_ioctl_gmm_arg *val =
					(struct ive_ioctl_gmm_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_GMM], "GMM");
			ret = ive_gmm(ndev, &val->src, &val->fg,
						&val->bg, &val->model, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_GMM]);
		} break;
		case IVE_IOC_GMM2: {
			struct ive_ioctl_gmm2_arg *val =
					(struct ive_ioctl_gmm2_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_GMM2], "GMM2");
			ret = ive_gmm2(ndev, &val->src, &val->factor,
						&val->fg, &val->bg, &val->stInfo,
						&val->model, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_GMM2]);
		} break;

		case IVE_IOC_BERNSEN: {
			struct ive_ioctl_bernsen_arg *val =
					(struct ive_ioctl_bernsen_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_BERNSEN], "Bernsen");
			ret = ive_bernsen(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BERNSEN]);
		} break;
		case IVE_IOC_FILTER: {
			struct ive_ioctl_filter_arg *val =
					(struct ive_ioctl_filter_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_FILTER3CH], "Filter");
			ret = ive_filter(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_FILTER3CH]);
		} break;
		case IVE_IOC_SOBEL: {
			struct ive_ioctl_sobel_arg *val =
					(struct ive_ioctl_sobel_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_SOBEL], "Sobel");
			ret = ive_sobel(ndev, &val->src, &val->dst_h,
						&val->dst_v, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_SOBEL]);
		} break;
		case IVE_IOC_MAG_AND_ANG: {
			struct ive_ioctl_maganang_arg *val =
					(struct ive_ioctl_maganang_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_MAG], "MagAndAng");
			ret = ive_mag_and_ang(ndev, &val->src, &val->dst_mag,
						&val->dst_ang, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_MAG]);
		} break;
		case IVE_IOC_CSC: {
			struct ive_ioctl_csc_arg *val =
					(struct ive_ioctl_csc_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_CSC], "CSC");
			ret = ive_csc(ndev, &val->src, &val->dst,
						&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_CSC]);
		} break;
		case IVE_IOC_HIST: {
			struct ive_ioctl_hist_arg *val =
					(struct ive_ioctl_hist_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_HIST], "Hist");
			ret = ive_hist(ndev, &val->src, &val->dst,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_HIST]);
		} break;
		case IVE_IOC_FILTER_AND_CSC: {
			struct ive_ioctl_filter_and_csc_arg *val =
					(struct ive_ioctl_filter_and_csc_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_FILTERCSC], "FilterAndCSC");
			ret = ive_filter_and_csc(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_FILTERCSC]);
		} break;
		case IVE_IOC_MAP: {
			struct ive_ioctl_map_arg *val =
					(struct ive_ioctl_map_arg *) g_kdata;

			task->buffer = (u16 *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_MAP], "Map");
			ret = ive_map(ndev, &val->src, task->buffer,
						&val->dst, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_MAP]);
		} break;
		case IVE_IOC_NCC: {
			struct ive_ioctl_ncc_arg *val =
					(struct ive_ioctl_ncc_arg *) g_kdata;

			task->buffer = (ive_ncc_dst_mem_s *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_NCC], "NCC");
			ret = ive_ncc(ndev, &val->src1, &val->src2,
						task->buffer, val->instant, task->dev_id);

			stop_ioctl_time(&g_time_infos[MOD_NCC]);
		} break;
		case IVE_IOC_INTEG: {
			struct ive_ioctl_integ_arg *val =
					(struct ive_ioctl_integ_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_INTEG], "Integ");
			ret = ive_integ(ndev, &val->src, &val->dst,
						&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_INTEG]);
		} break;
		case IVE_IOC_LBP: {
			struct ive_ioctl_lbp_arg *val =
					(struct ive_ioctl_lbp_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_LBP], "LBP");
			ret = ive_lbp(ndev, &val->src, &val->dst,
						&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_LBP]);
		} break;
		case IVE_IOC_THRESH_S16: {
			struct ive_ioctl_thresh_s16_arg *val =
					(struct ive_ioctl_thresh_s16_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_THRS16], "Thresh_S16");
			ret = ive_thresh_s16(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_THRS16]);
		} break;
		case IVE_IOC_THRESH_U16: {
			struct ive_ioctl_thresh_u16_arg *val =
					(struct ive_ioctl_thresh_u16_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_THRU16], "Thresh_U16");
			ret = ive_thresh_u16(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_THRU16]);
		} break;
		case IVE_IOC_16BIT_TO_8BIT: {
			struct ive_ioctl_16bit_to_8bit_arg *val =
					(struct ive_ioctl_16bit_to_8bit_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_16To8], "16BitTo8Bit");
			ret = ive_16bit_to_8bit(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_16To8]);
		} break;
		case IVE_IOC_ORD_STAT_FILTER: {
			struct ive_ioctl_ord_stat_filter_arg *val =
					(struct ive_ioctl_ord_stat_filter_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ORDSTAFTR], "OrdStatFilter");
			ret = ive_ord_stat_filter(ndev, &val->src,
							&val->dst, &val->ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ORDSTAFTR]);
		} break;
		case IVE_IOC_CANNYHYSEDGE: {
			struct ive_ioctl_canny_hys_edge_arg *val =
					(struct ive_ioctl_canny_hys_edge_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_CANNY], "CannyHysEdge");
			ret = ive_canny_hys_edge(ndev, &val->src, &val->dst,
							&val->stack, &val->ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_CANNY]);
		} break;
		case IVE_IOC_NORMGRAD: {
			struct ive_ioctl_norm_grad_arg *val =
					(struct ive_ioctl_norm_grad_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_NORMG], "NormGrad");
			ret = ive_norm_grad(ndev, &val->src, &val->dst_h,
							&val->dst_v, &val->dst_hv,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_NORMG]);
		} break;
		case IVE_IOC_GRADFG: {
			struct ive_ioctl_grad_fg_arg *val =
					(struct ive_ioctl_grad_fg_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_GRADFG], "GradFg");
			ret = ive_grad_fg(ndev, &val->bg_diff_fg,
							&val->cur_grad, &val->bg_grad,
							&val->grad_fg, &val->ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_GRADFG]);
		} break;
		case IVE_IOC_SAD: {
			struct ive_ioctl_sad_arg *val =
					(struct ive_ioctl_sad_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_SAD], "SAD");
			ret = ive_sad(ndev, &val->src1, &val->src2,
						&val->sad, &val->thr, &val->ctrl,
						val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_SAD]);
		} break;
		case IVE_IOC_RESIZE: {
			struct ive_ioctl_resize_arg *val =
					(struct ive_ioctl_resize_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_RESIZE], "Resize");
			ret = ive_resize(ndev, &val->src, &val->dst, &val->ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_RESIZE]);
		} break;
		case IVE_IOC_CCL: {
			struct ive_ioctl_ccl_arg *val =
					(struct ive_ioctl_ccl_arg *) g_kdata;

			val->blob.vir_addr = (uintptr_t)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_NORMG], "CCL");
			ret = ive_ccl(ndev, &val->src_dst,
							&val->blob, &val->ccl_ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_NORMG]);
		} break;
		case IVE_IOC_IMGIN_To_ODMA: {
			struct ive_ioctl_filter_arg *val =
					(struct ive_ioctl_filter_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_BYP], "imgInToOdma");
			ret = ive_imgIn_to_odma(ndev, &val->src, &val->dst,
							&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BYP]);
		} break;
		case IVE_IOC_RGBP2YUV2ERODE2DILATE: {
			struct ive_ioctl_rgbPToYuvToErodeToDilate *val =
					(struct ive_ioctl_rgbPToYuvToErodeToDilate *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ED],
					"rgbPToYuvToErodeToDilate");
			ret = ive_rgbp_to_yuv_to_erode_to_dilate(
				ndev, &val->src, &val->dst1, &val->dst2,
				&val->ctrl, val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ED]);
		} break;
		case IVE_IOC_ST_CANDI_CORNER: {
			struct ive_ioctl_stcandicorner *val =
					(struct ive_ioctl_stcandicorner *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_STCANDI], "STCandiCorner");
			start_ioctl_time(&g_time_infos[MOD_STBOX], "STBox");
			ret = ive_stcandi_corner(ndev, &val->src,
							&val->dst, &val->ctrl,
							val->instant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_STCANDI]);
			stop_ioctl_time(&g_time_infos[MOD_STBOX]);
		} break;
		case IVE_IOC_MD: {
			struct ive_ioctl_md *val = (struct ive_ioctl_md *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_MD], "FrameDiffDetect");
			ret = ive_frame_diff_motion(ndev, &val->src1,
								&val->src2, &val->dst,
								&val->ctrl,
								val->instant,
								task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_MD]);
		} break;
		case IVE_IOC_CMDQ: {
			start_ioctl_time(&g_time_infos[MOD_CMDQ], "CmdQ");
			ret = ive_cmdq(ndev, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_CMDQ]);
		} break;
		default:
			atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);
			ive_task_done(ndev, task);
			TRACE_IVE(IVE_DBG_ERR, "invalid ioctl cmd[%d]\n", task->task_type);
		}
		if (ret) {
			atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);
			ive_task_done(ndev, task);
			dev_err(ndev->dev,
				"[IVE] ioctl _IOC_NR(%d) fail\n", _IOC_NR(task->task_type));
		}
	// }
	ive_task_done(ndev, task);
	atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);
}

static int ive_try_submit_hw(struct ive_device *ndev, struct ive_task *task)
{
	int i, top_id, ret = -1;
	enum ive_core_state state;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		state = atomic_read(&ndev->core[i].dev_state);

		if (state == IVE_CORE_STATE_END) {
			top_id = i;

			ive_submit_hw(ndev, top_id, task);
			ret = 0;
			break;
		}
	}

	if (ret)
		TRACE_IVE(IVE_DBG_NOTICE, "ive submit hw fail, hw busy\n");
	return ret;
}

static bool ive_have_idle_core(struct ive_device *ndev)
{
	u8 coreid;
	enum ive_core_state state;

	for (coreid = 0; coreid < IVE_DEV_MAX; coreid++) {
		state = atomic_read(&ndev->core[coreid].dev_state);
		if (state == IVE_CORE_STATE_END)
			return true;
	}
	return false;
}

static void ive_try_commit_task(struct ive_device *ndev, struct ive_task *task)
{

	if (!ndev || !task) {
		TRACE_IVE(IVE_DBG_ERR, "ndev or task is nullptr\n");
		return;
	}

	if (atomic_read(&task->state) != IVE_TASK_STATE_RUNNING) {
		TRACE_IVE(IVE_DBG_ERR, "task is working, try commit task failed\n");
		return;
	}

	if (!ive_have_idle_core(ndev)) {
		TRACE_IVE(IVE_DBG_ERR, "no idle core\n");
		return;
	}

	ive_try_submit_hw(ndev, task);

	return;

}
static void ive_clr_evt_kth(void *data)
{
	struct ive_device *ndev = (struct ive_device *)data;
	unsigned long flags;
	enum ive_wait_evt evt;

	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "ive device isn't created yet\n");
		return;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);
	evt = ndev->evt;
	ndev->evt &= ~evt;
	TRACE_IVE(IVE_DBG_DEBUG, "old evt[%d], new evt[%d]", evt, ndev->evt);
	spin_unlock_irqrestore(&ndev->close_lock, flags);
}

static int ive_event_handler_th(void *data)
{
	struct ive_device *ndev = (struct ive_device *)data;
	struct ive_task *task, *task_tmp;
	unsigned long flags;
	int ret;
	unsigned long idle_timeout = msecs_to_jiffies(IVE_IDLE_WAIT_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(IVE_EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	u8 idle_coreid;
	bool have_idle_task = false;

	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "ive device isn't created yet\n");
		return -1;
	}

	while (!kthread_should_stop()) {
		if (!ndev)
			break;

		ret = wait_event_interruptible_timeout(ndev->wait,
			ndev->evt || kthread_should_stop(), timeout);

		if (ret < 0 || kthread_should_stop())
			break;

		idle_coreid = ive_get_idle_coreid(ndev);

		if (idle_coreid >= IVE_DEV_MAX) {
			TRACE_IVE(IVE_DBG_ERR, "ive device hw busy, no idle core\n");
			goto continue_th;
		}

		if (is_ive_suspend(ndev))
			continue;

		if (list_empty(&ndev->tsk_list)) {
			TRACE_IVE(IVE_DBG_DEBUG, "task list is empty\n");
			goto continue_th;
		}

		spin_lock_irqsave(&ndev->close_lock, flags);
		list_for_each_entry_safe(task, task_tmp, &ndev->tsk_list, node) {
			if (task->dev_id == -1) {
				TRACE_IVE(IVE_DBG_DEBUG, "got idle task[%p]", task);
				have_idle_task = true;
				break;
			}
		}

		if (!have_idle_task) {
			TRACE_IVE(IVE_DBG_DEBUG, "no idle task ,task[%p] task_tmp[%p] dev_id[%d]"
				, task, task_tmp, task_tmp->dev_id);
			spin_unlock_irqrestore(&ndev->close_lock, flags);
			goto continue_th;
		}
		atomic_set(&task->state, IVE_TASK_STATE_RUNNING);
		list_del(&task->node);
		TRACE_IVE(IVE_DBG_DEBUG, "del list node[%p]", task);
		spin_unlock_irqrestore(&ndev->close_lock, flags);

		ive_try_commit_task(ndev, task);

continue_th:
		ive_clr_evt_kth(ndev);

		timeout = list_empty(&ndev->tsk_list) ? idle_timeout : eof_timeout;

	}

	return 0;
}

static int ive_sw_init(struct ive_device *ndev)
{
	s32 ret = SUCCESS;
	struct sched_param task;

	if (ndev == NULL) {
		TRACE_IVE(IVE_DBG_ERR, "drv point is NULL\n");
		return FAILURE;
	}

	INIT_LIST_HEAD(&ndev->tsk_list);
	init_waitqueue_head(&ndev->wait);
	sema_init(&ndev->sem, 0);
	ndev->evt = IVE_EVENT_BUSY_OR_NOT_STAT;
	ndev->work_thread = kthread_run(ive_event_handler_th, (void *)ndev, "ive_event_handler_th");
	if (IS_ERR(ndev->work_thread)) {
		TRACE_IVE(IVE_DBG_ERR, "create ive thread failed\n");
		return -1;
	}

	// Same as sched_set_fifo in linux 5.x
	task.sched_priority = MAX_USER_RT_PRIO - 10;
	ret = sched_setscheduler(ndev->work_thread, SCHED_FIFO, &task);
	if (ret)
		TRACE_IVE(IVE_DBG_WARN, "ive thread priority update failed: %d\n", ret);

	if (atomic_cmpxchg(&g_timer_added,0, 1) == 0)
		add_timer(&timer_proc);
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));

	register_timer_fun(ive_timer_core_update, (void *)ndev);

	return ret;
}

static void ive_sw_deinit(struct ive_device *ndev)
{
	if (ndev == NULL) {
		return;
	}

	if (!IS_ERR(ndev->work_thread)) {
		if (kthread_stop(ndev->work_thread))
			TRACE_IVE(IVE_DBG_ERR, "stop ive work_thread failed\n");
	}

	list_del_init(&ndev->tsk_list);

	if (atomic_cmpxchg(&g_timer_added, 1, 0) == 1)
		del_timer_sync(&timer_proc);


}
static int instance_init(struct platform_device *pdev)
{
	s32 ret = 0;
	struct ive_device *ndev;

	ndev = dev_get_drvdata(&pdev->dev);
	if (!ndev) {
		TRACE_IVE(IVE_DBG_ERR, "ive get driver data failed!\n");
		return FAILURE;
	}

	if (ive_sw_init(ndev)) {
		pr_err("ive software init failed\n");
		goto err_sw_init;
	}

	return ret;

err_sw_init:
	ive_sw_deinit(ndev);

	return ret;
}

static int ive_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ive_device *ndev;
	struct resource *res[IVE_DEV_MAX];
	int i,ret;

	TRACE_IVE(IVE_DBG_INFO, "ive probe statrt\n");
	// Alloc a zero ive_device struct, and it will auto free when remod
	ndev = devm_kzalloc(&pdev->dev, sizeof(struct ive_device),
			    GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	for (i = 0; i < ARRAY_SIZE(ndev->ive_base); ++i) {
		res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (unlikely(res[i] == NULL)) {
			dev_err(&pdev->dev, "invalid ive resource\n");
			return -EINVAL;
		}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ndev->ive_base[i] = devm_ioremap(&pdev->dev, res[i]->start,
					res[i]->end - res[i]->start);
				//ndev->ive_base[i] = devm_ioremap_resource(dev, res[i]);
#else
		ndev->ive_base[i] = devm_ioremap_nocache(&pdev->dev, res[i]->start,
								res[i]->end - res[i]->start);
#endif

		//if (IS_ERR(ndev->ive_base[i]))
			//return PTR_ERR(ndev->ive_base[i]);

		if (assign_ive_block_addr(ndev->ive_base[i], i))
			return PTR_ERR(ndev->ive_base[i]);

		ndev->ive_irq[i] = platform_get_irq_byname(pdev, ive_irq_name[i]);
		if (ndev->ive_irq[i] <= 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n", ive_irq_name[i]);
			return -EBUSY;
		}
		spin_lock_init(&ndev->core[i].dev_lock);
		init_completion(&ndev->core[i].frame_done);
		init_completion(&ndev->core[i].op_done);
		atomic_set(&ndev->core[i].dev_state, IVE_CORE_STATE_END);
#if 1
		ret = devm_request_irq(&pdev->dev, ndev->ive_irq[i], ive_irq_handler,
						IRQF_TRIGGER_NONE, ive_irq_name[i], ndev);
#else
		ret = devm_request_threaded_irq(&pdev->dev, irq_num[i], ive_irq_handler, ive_irq_finish_thread_func,
						IRQF_TRIGGER_NONE, ive_irq_name[i], ndev)
#endif
		if (ret) {
			dev_err(&pdev->dev,
				"Unable to request interrupt for device (err=%d).\n",
				ret);
			return -ENXIO;
		}

		ndev->core[i].irq_num =  ndev->ive_irq[i];

	}
	ndev->use_count = 0;
	spin_lock_init(&ndev->close_lock);
	init_completion(&ndev->frame_done);
	init_completion(&ndev->op_done);

	g_time_infos = devm_kzalloc(&pdev->dev,
				  MOD_ALL * sizeof(struct ive_profiling_info),
				  GFP_KERNEL);

	ret = ive_register_cdev(ndev);
	if (ret < 0) {
		pr_err("[IVE] register chrdev error\n");
		return ret;
	}

	// Create drvdata(global variables)
	platform_set_drvdata(pdev, ndev);
	// Create ive proc descript
	// ndev->proc_dir = proc_mkdir("soph", NULL);
	if (proc_create_data(IVE_PROC_NAME, 0644, NULL,
			     &ive_proc_ops, ndev) == NULL)
		pr_err("[IVE] ive hw_profiling proc creation failed\n");
	g_enable_usage_profiling = 1;

	//clk init
	for (i = 0; i < IVE_DEV_MAX; i++) {
		ndev->core[i].clk_src = devm_clk_get(dev, ive_clk_name[i][0]);
		if (IS_ERR(ndev->core[i].clk_src)) {
			pr_err("Cannot get vi_sys3 clk for ive%d\n", i);
			ndev->core[i].clk_src = NULL;
		}

		ndev->core[i].clk_ive = devm_clk_get(dev, ive_clk_name[i][1]);
		if (IS_ERR(ndev->core[i].clk_ive)) {
			pr_err("Cannot get ive clk for ive%d\n", i);
			ndev->core[i].clk_ive = NULL;
		}
	}

	instance_init(pdev);

	// stcandicorner_workaround(ndev);
	return 0;
}

static int ive_remove(struct platform_device *pdev)
{
	// Get drvdata(global variables)
	struct ive_device *ndev = platform_get_drvdata(pdev);

	ive_sw_deinit(ndev);

	device_destroy(class_id, cdev_id);

	cdev_del(&ndev->cdev);

	unregister_chrdev_region(cdev_id, 1);

	//misc_deregister(&ndev->miscdev);

	class_destroy(class_id);

	platform_set_drvdata(pdev, NULL);


	// remove ive proc
	remove_proc_entry(IVE_PROC_NAME, NULL);
	return 0;
}

// #ifdef CONFIG_PM
// static int cvi_ive_suspend(struct device *dev)
// {
// 	//[TODO]
// 	pr_debug("[IVE] ive_suspend\n");
// 	return 0;
// }

// static int cvi_ive_resume(struct device *dev)
// {
// 	//[TODO]
// 	pr_debug("[IVE] ive_resume\n");
// 	return 0;
// }
// #endif
// #ifdef CONFIG_PM
// static SIMPLE_DEV_PM_OPS(cvi_ive_pm_ops, cvi_ive_suspend, cvi_ive_resume);
// #endif
static const struct of_device_id ive_match[] = {
	{ .compatible = "cvitek,ive" },
	{},
};
MODULE_DEVICE_TABLE(of, ive_match);

static struct platform_driver ive_driver = {
	.probe = ive_probe,
	.remove = ive_remove,
	.suspend = ive_suspend,
	.resume = ive_resume,
	.driver = {
			.owner = THIS_MODULE,
			.name = IVE_CDEV_NAME,
// #ifdef CONFIG_PM
// 			.pm = &cvi_ive_pm_ops,
// #endif
			.of_match_table = ive_match,  // add ive to dtsi
		},
};

module_platform_driver(ive_driver);

MODULE_AUTHOR("Ken Lin<ken.lin@cvitek.com>");
MODULE_DESCRIPTION("Cvitek SoC IVE driver");
MODULE_LICENSE("GPL");

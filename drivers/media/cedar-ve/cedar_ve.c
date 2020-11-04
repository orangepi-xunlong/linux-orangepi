/*
 * drivers\media\cedar_ve
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.allwinnertech.com>
 * fangning<fangning@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/mm.h>
#include <asm/siginfo.h>
#include <asm/signal.h>
#include <linux/clk/sunxi.h>
#include <linux/debugfs.h>

#if (defined CONFIG_ARCH_SUN8IW12P1)
#include <linux/mpp.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "cedar_ve.h"
#include "ve_mem_list.h"
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/sunxi_mbus.h>


struct regulator *regu;

#define DRV_VERSION "0.01alpha"

#undef USE_CEDAR_ENGINE

#ifndef CEDARDEV_MAJOR
#define CEDARDEV_MAJOR (150)
#endif
#ifndef CEDARDEV_MINOR
#define CEDARDEV_MINOR (0)
#endif

#define MACC_REGS_BASE		(0x01C0E000)

#ifndef CONFIG_OF
#define SUNXI_IRQ_VE		(90)
#endif

/* #define CEDAR_DEBUG */
#define cedar_ve_printk(level, msg...) printk(level "cedar_ve: " msg)

#define VE_LOGD(fmt, arg...) printk(KERN_DEBUG"VE: "fmt"\n", ##arg)
#define VE_LOGI(fmt, arg...) printk(KERN_INFO"VE: "fmt"\n", ##arg)
#define VE_LOGW(fmt, arg...) printk(KERN_WARNING"VE: "fmt"\n", ##arg)
#define VE_LOGE(fmt, arg...) printk(KERN_ERR"VE: "fmt"\n", ##arg)

#define VE_CLK_HIGH_WATER  (900)
#define VE_CLK_LOW_WATER   (100)

#define PRINTK_IOMMU_ADDR 0
struct dentry *ve_debugfs_root;
#define VE_DEBUGFS_MAX_CHANNEL 16
#define VE_DEBUGFS_BUF_SIZE 1024

struct ve_debugfs_proc {
	unsigned int	len;
	char			data[VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL];
};

struct ve_debugfs_buffer {
	unsigned char	 cur_channel_id;
	unsigned int	proc_len[VE_DEBUGFS_MAX_CHANNEL];
	char			*proc_buf[VE_DEBUGFS_MAX_CHANNEL];
	char			*data;
	struct mutex lock_proc;
};
struct ve_debugfs_buffer ve_debug_proc_info;

struct __cedarv_task {
	int task_prio;
	int ID;
	unsigned long timeout;
	unsigned int frametime;
	unsigned int block_mode;
};

struct cedarv_engine_task {
	struct __cedarv_task t;
	struct list_head list;
	struct task_struct *task_handle;
	unsigned int status;
	unsigned int running;
	unsigned int is_first_task;
};

struct cedarv_engine_task_info {
	int task_prio;
	unsigned int frametime;
	unsigned int total_time;
};

struct cedarv_regop {
	unsigned long addr;
	unsigned int value;
};

struct cedarv_env_infomation_compat {
	unsigned int phymem_start;
	int	 phymem_total_size;
	u32	 address_macc;
};

struct __cedarv_task_compat {
	int task_prio;
	int ID;
	u32 timeout;
	unsigned int frametime;
	unsigned int block_mode;
};

struct cedarv_regop_compat {
	u32 addr;
	unsigned int value;
};

struct VE_PROC_INFO {
	unsigned char   channel_id;
	unsigned int	proc_info_len;
};

int g_dev_major = CEDARDEV_MAJOR;
int g_dev_minor = CEDARDEV_MINOR;
/*S_IRUGO represent that g_dev_major can be read,but canot be write*/
module_param(g_dev_major, int, 0444);
module_param(g_dev_minor, int, 0444);

struct clk *ve_moduleclk;
struct clk *ve_parent_pll_clk;
struct clk *ve_power_gating;

static unsigned long ve_parent_clk_rate = 300000000;

struct iomap_para {
	volatile char *regs_macc;
	volatile char *regs_avs;
};

static DECLARE_WAIT_QUEUE_HEAD(wait_ve);
struct cedar_dev {
	struct cdev cdev;				 /* char device struct				   */
	struct device *dev;				 /* ptr to class device struct		   */
	struct device *platform_dev;	 /* ptr to class device struct		   */
	struct class  *class;			 /* class for auto create device node  */

	struct semaphore sem;			 /* mutual exclusion semaphore		   */

	wait_queue_head_t wq;			 /* wait queue for poll ops			   */

	struct iomap_para iomap_addrs;	 /* io remap addrs					   */

	struct timer_list cedar_engine_timer;
	struct timer_list cedar_engine_timer_rel;

	u32 irq;						 /* cedar video engine irq number	   */
	u32 de_irq_flag;					/* flag of video decoder engine irq generated */
	u32 de_irq_value;					/* value of video decoder engine irq		  */
	u32 en_irq_flag;					/* flag of video encoder engine irq generated */
	u32 en_irq_value;					/* value of video encoder engine irq		  */
	u32 irq_has_enable;
	u32 ref_count;
	int last_min_freq;

	u32 jpeg_irq_flag;					  /* flag of video jpeg dec irq generated */
	u32 jpeg_irq_value;					  /* value of video jpeg dec  irq */

	volatile unsigned int *sram_bass_vir;
	volatile unsigned int *clk_bass_vir;

	struct mutex lock_vdec;
	struct mutex lock_jdec;
	struct mutex lock_venc;
	struct mutex lock_00_reg;
	struct mutex lock_04_reg;
	struct aw_mem_list_head	   list;		/* buffer list */
	struct mutex lock_mem;
	unsigned char bMemDevAttachFlag;
};

struct ve_info { /* each object will bind a new file handler */
	unsigned int set_vol_flag;

	struct mutex lock_flag_io;
	u32 lock_flags; /* if flags is 0, means unlock status */
};

struct user_iommu_param {
	int				fd;
	unsigned int	iommu_addr;
};

struct cedarv_iommu_buffer {
	struct aw_mem_list_head i_list;
	int				fd;
	unsigned long	iommu_addr;
	struct dma_buf	*dma_buf;
	struct dma_buf_attachment *attachment;
	struct sg_table	*sgt;
	int    p_id;
};

static struct cedar_dev *cedar_devp;

/*
 * Video engine interrupt service routine
 * To wake up ve wait queue
 */

#if defined(CONFIG_OF)
static struct of_device_id sunxi_cedar_ve_match[] = {
	{ .compatible = "allwinner,sunxi-cedar-ve",},
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_cedar_ve_match);
#endif

static irqreturn_t VideoEngineInterupt(int irq, void *dev)
{
	unsigned long ve_int_status_reg;
	unsigned long ve_int_ctrl_reg;
	unsigned int status;
	volatile int val;
	int modual_sel;
	unsigned int interrupt_enable;
	struct iomap_para addrs = cedar_devp->iomap_addrs;

	modual_sel = readl(addrs.regs_macc + 0);
#ifdef CONFIG_ARCH_SUN3IW1P1
	if (modual_sel&0xa) {
		if ((modual_sel&0xb) == 0xb) {
			/*jpg enc*/
			ve_int_status_reg = (unsigned long)(addrs.regs_macc + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/*isp*/
			ve_int_status_reg = (unsigned long)(addrs.regs_macc + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		if (status && interrupt_enable) {
			/*disable interrupt*/
			if ((modual_sel & 0xb) == 0xb) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else{
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xa00 + 0x08);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			}

			cedar_devp->en_irq_value = 1;	/*hx modify 2011-8-1 16:08:47*/
			cedar_devp->en_irq_flag = 1;
			/*any interrupt will wake up wait queue*/
			wake_up(&wait_ve);		  /*ioctl*/
		}
	}
#else
	if (modual_sel&(3<<6)) {
		if (modual_sel&(1<<7)) {
			/*avc enc*/
			ve_int_status_reg = (unsigned long)(addrs.regs_macc + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/*isp*/
			ve_int_status_reg = (unsigned long)(addrs.regs_macc + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		/*modify by fangning 2013-05-22*/
		if (status && interrupt_enable) {
			/*disable interrupt*/
			/*avc enc*/
			if (modual_sel&(1<<7)) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				/*isp*/
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xa00 + 0x08);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			}
			/*hx modify 2011-8-1 16:08:47*/
			cedar_devp->en_irq_value = 1;
			cedar_devp->en_irq_flag = 1;
			/*any interrupt will wake up wait queue*/
			wake_up(&wait_ve);
		}
	}
#endif

#if ((defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN50I) || \
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1))
	if (modual_sel&(0x20)) {
		ve_int_status_reg = (unsigned long)(addrs.regs_macc + 0xe00 + 0x1c);
		ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0xe00 + 0x14);
		interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x38);

		status = readl((void *)ve_int_status_reg);

		if ((status&0x7) && interrupt_enable) {
			/*disable interrupt*/
			val = readl((void *)ve_int_ctrl_reg);
			writel(val & (~0x38), (void *)ve_int_ctrl_reg);

			cedar_devp->jpeg_irq_value = 1;
			cedar_devp->jpeg_irq_flag = 1;

			/*any interrupt will wake up wait queue*/
			wake_up(&wait_ve);
		}
	}
#endif

	modual_sel &= 0xf;
	if (modual_sel <= 4) {
		/*estimate Which video format*/
		switch (modual_sel) {
		case 0: /*mpeg124*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_macc + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7c);
			break;
		case 1: /*h264*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_macc + 0x200 + 0x28);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x200 + 0x20);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 2: /*vc1*/
			ve_int_status_reg = (unsigned long)(addrs.regs_macc +
				0x300 + 0x2c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x300 + 0x24);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 3: /*rv*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_macc + 0x400 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x400 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;

		case 4: /*hevc*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_macc + 0x500 + 0x38);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x500 + 0x30);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;

		default:
			ve_int_status_reg = (unsigned long)(addrs.regs_macc + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_macc + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			cedar_ve_printk(KERN_WARNING, "ve mode :%x " "not defined!\n", modual_sel);
			break;
		}

		status = readl((void *)ve_int_status_reg);

		/*modify by fangning 2013-05-22*/
		if ((status&0xf) && interrupt_enable) {
			/*disable interrupt*/
			if (modual_sel == 0) {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7c), (void *)ve_int_ctrl_reg);
			} else {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0xf), (void *)ve_int_ctrl_reg);
			}

			cedar_devp->de_irq_value = 1;
			cedar_devp->de_irq_flag = 1;
			/*any interrupt will wake up wait queue*/
			wake_up(&wait_ve);
		}
	}

	return IRQ_HANDLED;
}

static int clk_status;
static LIST_HEAD(run_task_list);
static LIST_HEAD(del_task_list);
static spinlock_t cedar_spin_lock;
#define CEDAR_RUN_LIST_NONULL	-1
#define CEDAR_NONBLOCK_TASK	 0
#define CEDAR_BLOCK_TASK 1
#define CLK_REL_TIME 10000
#define TIMER_CIRCLE 50
#define TASK_INIT	   0x00
#define TASK_TIMEOUT   0x55
#define TASK_RELEASE   0xaa
#define SIG_CEDAR		35

int enable_cedar_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	if (clk_status == 1)
		goto out;

	clk_status = 1;

	sunxi_periph_reset_deassert(ve_moduleclk);
	if (clk_enable(ve_moduleclk)) {
		cedar_ve_printk(KERN_WARNING, "enable ve_moduleclk failed;\n");
		goto out;
	} else {
		res = 0;
	}

	AW_MEM_INIT_LIST_HEAD(&cedar_devp->list);
#ifdef CEDAR_DEBUG
	printk("%s,%d\n", __func__, __LINE__);
#endif

out:
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	return res;
}

int disable_cedar_hw_clk(void)
{
	unsigned long flags;
	struct aw_mem_list_head *pos, *q;
	int res = -EFAULT;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	if (clk_status == 0) {
		res = 0;
		spin_unlock_irqrestore(&cedar_spin_lock, flags);
		goto out;
	}
	clk_status = 0;

	if ((ve_moduleclk == NULL) || (IS_ERR(ve_moduleclk)))
		cedar_ve_printk(KERN_WARNING, "ve_moduleclk is invalid\n");
	else {
		clk_disable(ve_moduleclk);
		sunxi_periph_reset_assert(ve_moduleclk);
		res = 0;
	}
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	mutex_lock(&cedar_devp->lock_mem);
	aw_mem_list_for_each_safe(pos, q, &cedar_devp->list) {
		struct cedarv_iommu_buffer *tmp;

		tmp = aw_mem_list_entry(pos, struct cedarv_iommu_buffer, i_list);
		aw_mem_list_del(pos);
		kfree(tmp);
	}
	mutex_unlock(&cedar_devp->lock_mem);

#ifdef CEDAR_DEBUG
	printk("%s,%d\n", __func__, __LINE__);
#endif

out:
	return res;
}

void cedardev_insert_task(struct cedarv_engine_task *new_task)
{
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	if (list_empty(&run_task_list))
		new_task->is_first_task = 1;


	list_for_each_entry(task_entry, &run_task_list, list) {
		if ((task_entry->is_first_task == 0) && (task_entry->running == 0) && (task_entry->t.task_prio < new_task->t.task_prio)) {
			break;
		}
	}

	list_add(&new_task->list, task_entry->list.prev);

#ifdef CEDAR_DEBUG
	printk("%s,%d, TASK_ID:", __func__, __LINE__);
	list_for_each_entry(task_entry, &run_task_list, list) {
		printk("%d!", task_entry->t.ID);
	}
	printk("\n");
#endif

	mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

int cedardev_del_task(int task_id)
{
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	list_for_each_entry(task_entry, &run_task_list, list) {
		if (task_entry->t.ID == task_id && task_entry->status != TASK_RELEASE) {
			task_entry->status = TASK_RELEASE;

			spin_unlock_irqrestore(&cedar_spin_lock, flags);
			mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);
			return 0;
		}
	}
	spin_unlock_irqrestore(&cedar_spin_lock, flags);

	return -1;
}

int cedardev_check_delay(int check_prio)
{
	struct cedarv_engine_task *task_entry;
	int timeout_total = 0;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);
	list_for_each_entry(task_entry, &run_task_list, list) {
		if ((task_entry->t.task_prio >= check_prio) || (task_entry->running == 1) || (task_entry->is_first_task == 1))
			timeout_total = timeout_total + task_entry->t.frametime;
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
#ifdef CEDAR_DEBUG
	printk("%s,%d,%d\n", __func__, __LINE__, timeout_total);
#endif
	return timeout_total;
}

static void cedar_engine_for_timer_rel(unsigned long arg)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	if (list_empty(&run_task_list)) {
		ret = disable_cedar_hw_clk();
		if (ret < 0) {
			cedar_ve_printk(KERN_WARNING, "clk disable error!\n");
		}
	} else {
		cedar_ve_printk(KERN_WARNING, "clk disable time out but task left\n");
		mod_timer(&cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

static void cedar_engine_for_events(unsigned long arg)
{
	struct cedarv_engine_task *task_entry, *task_entry_tmp;
	struct siginfo info;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	list_for_each_entry_safe(task_entry, task_entry_tmp, &run_task_list, list) {
		mod_timer(&cedar_devp->cedar_engine_timer_rel, jiffies + msecs_to_jiffies(CLK_REL_TIME));
		if (task_entry->status == TASK_RELEASE ||
				time_after(jiffies, task_entry->t.timeout)) {
			if (task_entry->status == TASK_INIT)
				task_entry->status = TASK_TIMEOUT;
			list_move(&task_entry->list, &del_task_list);
		}
	}

	list_for_each_entry_safe(task_entry, task_entry_tmp, &del_task_list, list) {
		info.si_signo = SIG_CEDAR;
		info.si_code = task_entry->t.ID;
		if (task_entry->status == TASK_TIMEOUT) {
			info.si_errno = TASK_TIMEOUT;
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		} else if (task_entry->status == TASK_RELEASE) {
			info.si_errno = TASK_RELEASE;
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}
		list_del(&task_entry->list);
		kfree(task_entry);
	}

	if (!list_empty(&run_task_list)) {
		task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
		if (task_entry->running == 0) {
			task_entry->running = 1;
			info.si_signo = SIG_CEDAR;
			info.si_code = task_entry->t.ID;
			info.si_errno = TASK_INIT;
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}

		mod_timer(&cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

static long compat_cedardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long  ret = 0;
	int ve_timeout = 0;
	/*struct cedar_dev *devp;*/
#ifdef USE_CEDAR_ENGINE
	int rel_taskid = 0;
	struct __cedarv_task task_ret;
	struct cedarv_engine_task *task_ptr = NULL;
#endif
	unsigned long flags;
	struct ve_info *info;

	info = filp->private_data;

	switch (cmd) {
	case IOCTL_ENGINE_REQ:
#ifdef USE_CEDAR_ENGINE
			if (copy_from_user(&task_ret, (void __user *)arg,
				sizeof(struct __cedarv_task))) {
				cedar_ve_printk(KERN_WARNING, "USE_CEDAR_ENGINE copy_from_user fail\n");
				return -EFAULT;
			}
			spin_lock_irqsave(&cedar_spin_lock, flags);

			if (!list_empty(&run_task_list) &&
				(task_ret.block_mode == CEDAR_NONBLOCK_TASK)) {
				spin_unlock_irqrestore(&cedar_spin_lock, flags);
				return CEDAR_RUN_LIST_NONULL;
			}
			spin_unlock_irqrestore(&cedar_spin_lock, flags);

			task_ptr = kmalloc(sizeof(struct cedarv_engine_task), GFP_KERNEL);
			if (!task_ptr) {
				cedar_ve_printk(KERN_WARNING, "get task_ptr error\n");
				return PTR_ERR(task_ptr);
			}
			task_ptr->task_handle = current;
			task_ptr->t.ID = task_ret.ID;
			/*ms to jiffies*/
			task_ptr->t.timeout = jiffies +
				msecs_to_jiffies(1000*task_ret.timeout);
			task_ptr->t.frametime = task_ret.frametime;
			task_ptr->t.task_prio = task_ret.task_prio;
			task_ptr->running = 0;
			task_ptr->is_first_task = 0;
			task_ptr->status = TASK_INIT;

			cedardev_insert_task(task_ptr);

			ret = enable_cedar_hw_clk();
			if (ret < 0) {
				cedar_ve_printk(KERN_WARNING, "IOCTL_ENGINE_REQ clk enable error!\n");
				return -EFAULT;
			}
			return task_ptr->is_first_task;
#else
			if (down_interruptible(&cedar_devp->sem))
				return -ERESTARTSYS;
			cedar_devp->ref_count++;
			if (cedar_devp->ref_count == 1) {
				cedar_devp->last_min_freq = 0;
				enable_cedar_hw_clk();
			}
			up(&cedar_devp->sem);
			break;
#endif
		case IOCTL_ENGINE_REL:
#ifdef USE_CEDAR_ENGINE
			rel_taskid = (int)arg;

			ret = cedardev_del_task(rel_taskid);
#else
			if (down_interruptible(&cedar_devp->sem))
				return -ERESTARTSYS;
			cedar_devp->ref_count--;
			if (cedar_devp->ref_count == 0) {
				ret = disable_cedar_hw_clk();
				if (ret < 0) {
					cedar_ve_printk(KERN_WARNING, "IOCTL_ENGINE_REL clk disable error!\n");
					up(&cedar_devp->sem);
					return -EFAULT;
				}
			}
			up(&cedar_devp->sem);
#endif
			return ret;
		case IOCTL_ENGINE_CHECK_DELAY:
			{
				struct cedarv_engine_task_info task_info;

				if (copy_from_user(&task_info, (void __user *)arg,
					sizeof(struct cedarv_engine_task_info))) {
					cedar_ve_printk(KERN_WARNING, "%d copy_from_user fail\n",
						IOCTL_ENGINE_CHECK_DELAY);
					return -EFAULT;
				}
				task_info.total_time = cedardev_check_delay(task_info.task_prio);
#ifdef CEDAR_DEBUG
				printk("%s,%d,%d\n", __func__, __LINE__, task_info.total_time);
#endif
				task_info.frametime = 0;
				spin_lock_irqsave(&cedar_spin_lock, flags);
				if (!list_empty(&run_task_list)) {

					struct cedarv_engine_task *task_entry;
#ifdef CEDAR_DEBUG
					printk("%s,%d\n", __func__, __LINE__);
#endif
					task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
					if (task_entry->running == 1)
						task_info.frametime = task_entry->t.frametime;
#ifdef CEDAR_DEBUG
					printk("%s,%d,%d\n", __func__, __LINE__, task_info.frametime);
#endif
				}
				spin_unlock_irqrestore(&cedar_spin_lock, flags);

				if (copy_to_user((void *)arg, &task_info, sizeof(struct cedarv_engine_task_info))) {
					cedar_ve_printk(KERN_WARNING, "%d copy_to_user fail\n",
						IOCTL_ENGINE_CHECK_DELAY);
					return -EFAULT;
				}
			}
			break;
		case IOCTL_WAIT_VE_DE:
			ve_timeout = (int)arg;
			cedar_devp->de_irq_value = 0;

			spin_lock_irqsave(&cedar_spin_lock, flags);
			if (cedar_devp->de_irq_flag)
				cedar_devp->de_irq_value = 1;
			spin_unlock_irqrestore(&cedar_spin_lock, flags);
			wait_event_timeout(wait_ve, cedar_devp->de_irq_flag, ve_timeout*HZ);
			cedar_devp->de_irq_flag = 0;

			return cedar_devp->de_irq_value;

		case IOCTL_WAIT_VE_EN:

			ve_timeout = (int)arg;
			cedar_devp->en_irq_value = 0;

			spin_lock_irqsave(&cedar_spin_lock, flags);
			if (cedar_devp->en_irq_flag)
				cedar_devp->en_irq_value = 1;
			spin_unlock_irqrestore(&cedar_spin_lock, flags);

			wait_event_timeout(wait_ve, cedar_devp->en_irq_flag, ve_timeout*HZ);
			cedar_devp->en_irq_flag = 0;

			return cedar_devp->en_irq_value;

#if ((defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN50I) || \
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1))

		case IOCTL_WAIT_JPEG_DEC:
			ve_timeout = (int)arg;
			cedar_devp->jpeg_irq_value = 0;

			spin_lock_irqsave(&cedar_spin_lock, flags);
			if (cedar_devp->jpeg_irq_flag)
				cedar_devp->jpeg_irq_value = 1;
			spin_unlock_irqrestore(&cedar_spin_lock, flags);

			wait_event_timeout(wait_ve, cedar_devp->jpeg_irq_flag, ve_timeout*HZ);
			cedar_devp->jpeg_irq_flag = 0;
			return cedar_devp->jpeg_irq_value;
#endif
		case IOCTL_ENABLE_VE:
			if (clk_prepare_enable(ve_moduleclk)) {
				cedar_ve_printk(KERN_WARNING, "IOCTL_ENABLE_VE enable ve_moduleclk failed!\n");
			}
			break;

		case IOCTL_DISABLE_VE:
			if ((ve_moduleclk == NULL) || IS_ERR(ve_moduleclk)) {
				cedar_ve_printk(KERN_WARNING, "IOCTL_DISABLE_VE ve_moduleclk is invalid\n");
				return -EFAULT;
			} else {
				clk_disable_unprepare(ve_moduleclk);
			}
			break;

		case IOCTL_RESET_VE:
			sunxi_periph_reset_assert(ve_moduleclk);
			sunxi_periph_reset_deassert(ve_moduleclk);
			break;

		case IOCTL_SET_DRAM_HIGH_CHANNAL:
		{

			int flag = (int)arg;

			if (flag == 1)
			    mbus_port_setpri(26, 1);
			else
			    mbus_port_setpri(26, 0);

			break;
		}

		case IOCTL_SET_VE_FREQ:
			{
				int arg_rate = (int)arg;

				if (cedar_devp->last_min_freq == 0) {
					cedar_devp->last_min_freq = arg_rate;
				} else {
					if (arg_rate > cedar_devp->last_min_freq) {
						arg_rate = cedar_devp->last_min_freq;
					} else {
						cedar_devp->last_min_freq = arg_rate;
					}
				}
				if (arg_rate >= VE_CLK_LOW_WATER &&
						arg_rate <= VE_CLK_HIGH_WATER &&
						clk_get_rate(ve_moduleclk)/1000000 != arg_rate) {
					if (!clk_set_rate(ve_parent_pll_clk, arg_rate*1000000)) {
						ve_parent_clk_rate = clk_get_rate(ve_parent_pll_clk);
						if (clk_set_rate(ve_moduleclk, ve_parent_clk_rate)) {
							cedar_ve_printk(KERN_WARNING, "set ve clock failed\n");
						}

					} else {
						cedar_ve_printk(KERN_WARNING, "set pll4 clock failed\n");
					}
				}
				ret = clk_get_rate(ve_moduleclk);
				break;
			}
		case IOCTL_GETVALUE_AVS2:
		case IOCTL_ADJUST_AVS2:
		case IOCTL_ADJUST_AVS2_ABS:
		case IOCTL_CONFIG_AVS2:
		case IOCTL_RESET_AVS2:
		case IOCTL_PAUSE_AVS2:
		case IOCTL_START_AVS2:
			cedar_ve_printk(KERN_WARNING, "do not supprot this ioctrl now\n");
			break;

		case IOCTL_GET_ENV_INFO:
			{
				struct cedarv_env_infomation_compat env_info;

				env_info.phymem_start = 0;
				env_info.phymem_total_size = 0;
				env_info.address_macc = 0;
				if (copy_to_user((char *)arg, &env_info,
					sizeof(struct cedarv_env_infomation_compat)))
					return -EFAULT;
			}
			break;
		case IOCTL_GET_IC_VER:
			{
				return 0;
			}
		case IOCTL_SET_REFCOUNT:
			cedar_devp->ref_count = (int)arg;
			break;
		case IOCTL_SET_VOL:
			{

#if defined CONFIG_ARCH_SUN9IW1P1
				int ret;
				int vol = (int)arg;

				if (down_interruptible(&cedar_devp->sem)) {
					return -ERESTARTSYS;
				}
				info->set_vol_flag = 1;

				/*set output voltage to arg mV*/
				ret = regulator_set_voltage(regu, vol*1000, 3300000);
				if (IS_ERR(regu)) {
					cedar_ve_printk(KERN_WARNING, \
						"fail to set axp15_dcdc4 regulator voltage!\n");
				}
				up(&cedar_devp->sem);
#endif
				break;
			}
			case IOCTL_GET_LOCK: {
				int lock_ctl_ret = 0;
				u32 lock_type = arg;
				struct ve_info *vi = filp->private_data;

				if (lock_type == VE_LOCK_VDEC)
					mutex_lock(&cedar_devp->lock_vdec);
				else if (lock_type == VE_LOCK_VENC)
					mutex_lock(&cedar_devp->lock_venc);
				else if (lock_type == VE_LOCK_JDEC)
					mutex_lock(&cedar_devp->lock_jdec);
				else if (lock_type == VE_LOCK_00_REG)
					mutex_lock(&cedar_devp->lock_00_reg);
				else if (lock_type == VE_LOCK_04_REG)
					mutex_lock(&cedar_devp->lock_04_reg);
				else
					VE_LOGE("invalid lock type '%d'", lock_type);

				if ((vi->lock_flags&lock_type) != 0)
						VE_LOGE("when get lock, this should be 0!!!");

				mutex_lock(&vi->lock_flag_io);
				vi->lock_flags |= lock_type;
				mutex_unlock(&vi->lock_flag_io);

				return lock_ctl_ret;
			}
			case IOCTL_SET_PROC_INFO:
			{
				struct VE_PROC_INFO ve_info;
				unsigned char channel_id = 0;
				if (ve_debugfs_root == NULL)
					return 0;

				mutex_lock(&ve_debug_proc_info.lock_proc);
				if (copy_from_user(&ve_info, (void __user *)arg, sizeof(struct VE_PROC_INFO))) {
					cedar_ve_printk(KERN_WARNING, "IOCTL_SET_PROC_INFO copy_from_user fail\n");
					mutex_unlock(&ve_debug_proc_info.lock_proc);
					return -EFAULT;
				}

				channel_id = ve_info.channel_id;
				if (channel_id >= VE_DEBUGFS_MAX_CHANNEL) {
					cedar_ve_printk(KERN_WARNING, "set channel[%c] is bigger than max channel[%d]\n",
											channel_id, VE_DEBUGFS_MAX_CHANNEL);
					mutex_unlock(&ve_debug_proc_info.lock_proc);
					return -EFAULT;
				}

				ve_debug_proc_info.cur_channel_id = ve_info.channel_id;
				ve_debug_proc_info.proc_len[channel_id] = ve_info.proc_info_len;
				ve_debug_proc_info.proc_buf[channel_id] = ve_debug_proc_info.data +
													channel_id * VE_DEBUGFS_BUF_SIZE;
				break;
			}
			case IOCTL_COPY_PROC_INFO:
			{
				unsigned char channel_id;
				if (ve_debugfs_root == NULL)
					return 0;

				channel_id = ve_debug_proc_info.cur_channel_id;
				if (copy_from_user(ve_debug_proc_info.proc_buf[channel_id],
								  (void __user *)arg,
								ve_debug_proc_info.proc_len[channel_id])) {
					cedar_ve_printk(KERN_WARNING, "IOCTL_COPY_PROC_INFO copy_from_user fail\n");
							mutex_unlock(&ve_debug_proc_info.lock_proc);
					return -EFAULT;
				}
				mutex_unlock(&ve_debug_proc_info.lock_proc);
				break;
			}
			case IOCTL_STOP_PROC_INFO:
			{
				unsigned char channel_id;
				if (ve_debugfs_root == NULL)
					return 0;

				channel_id = arg;
				ve_debug_proc_info.proc_buf[channel_id] = NULL;

				break;
			}
			case IOCTL_RELEASE_LOCK: {
				int lock_ctl_ret = 0;

				do {
						u32 lock_type = arg;
						struct ve_info *vi = filp->private_data;

						if (!(vi->lock_flags & lock_type)) {
								VE_LOGE("Not lock? flags: '%x/%x'.", vi->lock_flags,
								lock_type);
								lock_ctl_ret = -1;
								break; /* break 'do...while' */
						}

						mutex_lock(&vi->lock_flag_io);
						vi->lock_flags &= (~lock_type);
						mutex_unlock(&vi->lock_flag_io);

						if (lock_type == VE_LOCK_VDEC)
							mutex_unlock(&cedar_devp->lock_vdec);
						else if (lock_type == VE_LOCK_VENC)
							mutex_unlock(&cedar_devp->lock_venc);
						else if (lock_type == VE_LOCK_JDEC)
							mutex_unlock(&cedar_devp->lock_jdec);
						else if (lock_type == VE_LOCK_00_REG)
							mutex_unlock(&cedar_devp->lock_00_reg);
						else if (lock_type == VE_LOCK_04_REG)
							mutex_unlock(&cedar_devp->lock_04_reg);
						else
							VE_LOGE("invalid lock type '%d'", lock_type);
				} while (0);
				return lock_ctl_ret;
		}
		case IOCTL_GET_IOMMU_ADDR:
		{
			int ret, i;
			struct sg_table *sgt, *sgt_bak;
			struct scatterlist *sgl, *sgl_bak;
			struct user_iommu_param sUserIommuParam;
			struct cedarv_iommu_buffer *pVeIommuBuf = NULL;
	    cedar_devp->bMemDevAttachFlag = 1;
			pVeIommuBuf =
			(struct cedarv_iommu_buffer *)kmalloc(sizeof(struct cedarv_iommu_buffer), GFP_KERNEL);
			if (pVeIommuBuf == NULL) {
				VE_LOGE("IOCTL_GET_IOMMU_ADDR malloc cedarv_iommu_buffererror\n");
				return -EFAULT;
			}
			if (copy_from_user(&sUserIommuParam, (void __user *)arg,
				sizeof(struct user_iommu_param))) {
				VE_LOGE("IOCTL_GET_IOMMU_ADDR copy_from_user error");
				return -EFAULT;
			}

			pVeIommuBuf->fd = sUserIommuParam.fd;
			pVeIommuBuf->dma_buf = dma_buf_get(pVeIommuBuf->fd);
			if (pVeIommuBuf->dma_buf < 0) {
				VE_LOGE("ve get dma_buf error");
				return -EFAULT;
			}

			pVeIommuBuf->attachment = dma_buf_attach(pVeIommuBuf->dma_buf,
															cedar_devp->platform_dev);
			if (pVeIommuBuf->attachment < 0) {
				VE_LOGE("ve get dma_buf_attachment error");
				goto RELEASE_DMA_BUF;
			}

			sgt = dma_buf_map_attachment(pVeIommuBuf->attachment,
											DMA_BIDIRECTIONAL);

			sgt_bak = kmalloc(sizeof(struct sg_table), GFP_KERNEL | __GFP_ZERO);
			if (sgt_bak == NULL)
				VE_LOGE("alloc sgt fail\n");

			ret = sg_alloc_table(sgt_bak, sgt->nents, GFP_KERNEL);
			if (ret != 0)
				VE_LOGE("alloc sgt fail\n");

			sgl_bak = sgt_bak->sgl;
			for_each_sg(sgt->sgl, sgl, sgt->nents, i)  {
				sg_set_page(sgl_bak, sg_page(sgl), sgl->length, sgl->offset);
				sgl_bak = sg_next(sgl_bak);
			}

			pVeIommuBuf->sgt = sgt_bak;
			if (pVeIommuBuf->sgt < 0) {
				VE_LOGE("ve get sg_table error\n");
				goto RELEASE_DMA_BUF;
			}

			ret = dma_map_sg(cedar_devp->platform_dev, pVeIommuBuf->sgt->sgl,
									pVeIommuBuf->sgt->nents,
									DMA_BIDIRECTIONAL);
			if (ret != 1) {
				VE_LOGE("ve dma_map_sg error\n");
				goto RELEASE_DMA_BUF;
			}

			pVeIommuBuf->iommu_addr = sg_dma_address(pVeIommuBuf->sgt->sgl);
			sUserIommuParam.iommu_addr = (unsigned int)(pVeIommuBuf->iommu_addr & 0xffffffff);

			if (copy_to_user((void __user *)arg, &sUserIommuParam, sizeof(struct user_iommu_param))) {
				VE_LOGE("ve get iommu copy_to_user error\n");
				goto RELEASE_DMA_BUF;
			}

			pVeIommuBuf->p_id = current->tgid;
			#if PRINTK_IOMMU_ADDR
			cedar_ve_printk(KERN_DEBUG, "fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, sg_table:%p, nents:%d, pid:%d\n",
			pVeIommuBuf->fd,
			pVeIommuBuf->iommu_addr,
			pVeIommuBuf->dma_buf,
			pVeIommuBuf->attachment,
			pVeIommuBuf->sgt,
			pVeIommuBuf->sgt->nents,
			pVeIommuBuf->p_id);
			#endif

			mutex_lock(&cedar_devp->lock_mem);
			aw_mem_list_add_tail(&pVeIommuBuf->i_list, &cedar_devp->list);
			mutex_unlock(&cedar_devp->lock_mem);
			break;

RELEASE_DMA_BUF:
			if (pVeIommuBuf->dma_buf > 0) {
				if (pVeIommuBuf->attachment > 0) {
					if (pVeIommuBuf->sgt > 0) {
						dma_unmap_sg(cedar_devp->platform_dev,
									pVeIommuBuf->sgt->sgl,
									pVeIommuBuf->sgt->nents,
									DMA_BIDIRECTIONAL);
						dma_buf_unmap_attachment(pVeIommuBuf->attachment, pVeIommuBuf->sgt,
												DMA_BIDIRECTIONAL);
						sg_free_table(pVeIommuBuf->sgt);
						kfree(pVeIommuBuf->sgt);
					}

					dma_buf_detach(pVeIommuBuf->dma_buf,
								pVeIommuBuf->attachment);
				}

				dma_buf_put(pVeIommuBuf->dma_buf);
				return -1;
			}
			kfree(pVeIommuBuf);
			break;
		}
		case IOCTL_FREE_IOMMU_ADDR:
		{
			struct user_iommu_param sUserIommuParam;
			struct cedarv_iommu_buffer *pVeIommuBuf;

			if (copy_from_user(&sUserIommuParam, (void __user *)arg,
				sizeof(struct user_iommu_param))) {
				VE_LOGE("IOCTL_FREE_IOMMU_ADDR copy_from_user error");
				return -EFAULT;
			}
			mutex_lock(&cedar_devp->lock_mem);
			aw_mem_list_for_each_entry(pVeIommuBuf, &cedar_devp->list, i_list) {
				if (pVeIommuBuf->fd == sUserIommuParam.fd &&
						pVeIommuBuf->p_id == current->tgid) {
					#if PRINTK_IOMMU_ADDR
					cedar_ve_printk(KERN_DEBUG, "free: fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, sg_table:%p nets:%d, pid:%d\n",
					pVeIommuBuf->fd,
					pVeIommuBuf->iommu_addr,
					pVeIommuBuf->dma_buf,
					pVeIommuBuf->attachment,
					pVeIommuBuf->sgt,
					pVeIommuBuf->sgt->nents,
					pVeIommuBuf->p_id);
					#endif

					if (pVeIommuBuf->dma_buf > 0) {
						if (pVeIommuBuf->attachment > 0) {
							if (pVeIommuBuf->sgt > 0) {
								dma_unmap_sg(cedar_devp->platform_dev,
											pVeIommuBuf->sgt->sgl,
											pVeIommuBuf->sgt->nents,
											DMA_BIDIRECTIONAL);
								dma_buf_unmap_attachment(pVeIommuBuf->attachment,
													pVeIommuBuf->sgt,
														DMA_BIDIRECTIONAL);
								sg_free_table(pVeIommuBuf->sgt);
								kfree(pVeIommuBuf->sgt);
							}

							dma_buf_detach(pVeIommuBuf->dma_buf, pVeIommuBuf->attachment);
						}

						dma_buf_put(pVeIommuBuf->dma_buf);
					}

					aw_mem_list_del(&pVeIommuBuf->i_list);
					kfree(pVeIommuBuf);
					break;
				}
			}
			mutex_unlock(&cedar_devp->lock_mem);
			break;
		}
		default:
			return -1;
	}
	return ret;
}

static int cedardev_open(struct inode *inode, struct file *filp)
{
	struct ve_info *info;

	info = kmalloc(sizeof(struct ve_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->set_vol_flag = 0;

	filp->private_data = info;
	if (down_interruptible(&cedar_devp->sem)) {
		return -ERESTARTSYS;
	}

	/* init other resource here */
	if (cedar_devp->ref_count == 0) {
		cedar_devp->de_irq_flag = 0;
		cedar_devp->en_irq_flag = 0;
		cedar_devp->jpeg_irq_flag = 0;
	}

	up(&cedar_devp->sem);
	nonseekable_open(inode, filp);

	mutex_init(&info->lock_flag_io);
	info->lock_flags = 0;

	return 0;
}

static int cedardev_release(struct inode *inode, struct file *filp)
{
	struct ve_info *info;
	struct cedarv_iommu_buffer *pVeIommuBuf1;
	struct aw_mem_list_head *pos, *q;

	info = filp->private_data;
	mutex_lock(&info->lock_flag_io);
	//if the process abort, this will free iommu_buffer
	if (cedar_devp->bMemDevAttachFlag) {
		aw_mem_list_for_each_safe(pos, q, &cedar_devp->list) {
	    pVeIommuBuf1 = aw_mem_list_entry(pos, struct cedarv_iommu_buffer, i_list);
	{
	if (pVeIommuBuf1->p_id == current->tgid) {
		   #if PRINTK_IOMMU_ADDR
			cedar_ve_printk(KERN_DEBUG, "free: fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, sg_table:%p nets:%d, pid:%d\n",
			pVeIommuBuf1->fd,
			pVeIommuBuf1->iommu_addr,
			pVeIommuBuf1->dma_buf,
			pVeIommuBuf1->attachment,
			pVeIommuBuf1->sgt,
			pVeIommuBuf1->sgt->nents,
			pVeIommuBuf1->p_id);
		    #endif
			if (pVeIommuBuf1->dma_buf > 0) {
				if (pVeIommuBuf1->attachment > 0) {
					if (pVeIommuBuf1->sgt > 0) {
						dma_unmap_sg(cedar_devp->platform_dev,
									pVeIommuBuf1->sgt->sgl,
									pVeIommuBuf1->sgt->nents,
									DMA_BIDIRECTIONAL);
						dma_buf_unmap_attachment(pVeIommuBuf1->attachment,
											pVeIommuBuf1->sgt,
												DMA_BIDIRECTIONAL);
						sg_free_table(pVeIommuBuf1->sgt);
						kfree(pVeIommuBuf1->sgt);
					}

					dma_buf_detach(pVeIommuBuf1->dma_buf, pVeIommuBuf1->attachment);
				}

				dma_buf_put(pVeIommuBuf1->dma_buf);
			}
			mutex_lock(&cedar_devp->lock_mem);
			aw_mem_list_del(&pVeIommuBuf1->i_list);
			kfree(pVeIommuBuf1);
			mutex_unlock(&cedar_devp->lock_mem);
		}
	}
	}
	}

	/* lock status */
	if (info->lock_flags) {
			VE_LOGW("release lost-lock...");
			if (info->lock_flags & VE_LOCK_VDEC)
				mutex_unlock(&cedar_devp->lock_vdec);

			if (info->lock_flags & VE_LOCK_VENC)
				mutex_unlock(&cedar_devp->lock_venc);

			if (info->lock_flags & VE_LOCK_JDEC)
				mutex_unlock(&cedar_devp->lock_jdec);

			if (info->lock_flags & VE_LOCK_00_REG)
				mutex_unlock(&cedar_devp->lock_00_reg);

			if (info->lock_flags & VE_LOCK_04_REG)
				mutex_unlock(&cedar_devp->lock_04_reg);

			info->lock_flags = 0;
		}

	mutex_unlock(&info->lock_flag_io);
	mutex_destroy(&info->lock_flag_io);

	if (down_interruptible(&cedar_devp->sem)) {
		return -ERESTARTSYS;
	}

#if defined CONFIG_ARCH_SUN9IW1P1

	if (info->set_vol_flag == 1) {
		regulator_set_voltage(regu, 900000, 3300000);
		if (IS_ERR(regu)) {
			cedar_ve_printk(KERN_WARNING, \
				"some error happen, fail to set axp15_dcdc4 regulator voltage!\n");
			return -EINVAL;
		}
	}
#endif

	/* release other resource here */
	if (cedar_devp->ref_count == 0) {
		cedar_devp->de_irq_flag = 1;
		cedar_devp->en_irq_flag = 1;
		cedar_devp->jpeg_irq_flag = 1;
	}
	up(&cedar_devp->sem);

	kfree(info);
	return 0;
}

static void cedardev_vma_open(struct vm_area_struct *vma)
{
}

static void cedardev_vma_close(struct vm_area_struct *vma)
{
}

static struct vm_operations_struct cedardev_remap_vm_ops = {
	.open  = cedardev_vma_open,
	.close = cedardev_vma_close,
};

static int cedardev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long temp_pfn;

	if (vma->vm_end - vma->vm_start == 0) {
		cedar_ve_printk(KERN_WARNING, "vma->vm_end is equal vma->vm_start : %lx\n",\
			vma->vm_start);
		return 0;
	}
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) {
		cedar_ve_printk(KERN_WARNING, \
			"the vma->vm_pgoff is %lx,it is large than the largest page number\n", vma->vm_pgoff);
		return -EINVAL;
	}


	temp_pfn = MACC_REGS_BASE >> 12;


	/* Set reserved and I/O flag for the area. */
	vma->vm_flags |= /*VM_RESERVED | */VM_IO;
	/* Select uncached access. */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
				vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}


	vma->vm_ops = &cedardev_remap_vm_ops;
	cedardev_vma_open(vma);

	return 0;
}

#ifdef CONFIG_PM
static int snd_sw_cedar_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;

	printk("[cedar] standby suspend\n");
	ret = disable_cedar_hw_clk();

#if defined CONFIG_ARCH_SUN9IW1P1
	clk_disable_unprepare(ve_power_gating);
#endif

	if (ret < 0) {
		cedar_ve_printk(KERN_WARNING, "cedar clk disable somewhere error!\n");
		return -EFAULT;
	}

	return 0;
}

static int snd_sw_cedar_resume(struct platform_device *pdev)
{
	int ret = 0;

	printk("[cedar] standby resume\n");

#if defined CONFIG_ARCH_SUN9IW1P1
	clk_prepare_enable(ve_power_gating);
#endif

	if (cedar_devp->ref_count == 0) {
		return 0;
	}

	ret = enable_cedar_hw_clk();
	if (ret < 0) {
		cedar_ve_printk(KERN_WARNING, "cedar clk enable somewhere error!\n");
		return -EFAULT;
	}
	return 0;
}
#endif

static const struct file_operations cedardev_fops = {
	.owner	 = THIS_MODULE,
	.mmap	 = cedardev_mmap,
	.open	 = cedardev_open,
	.release = cedardev_release,
	.llseek	 = no_llseek,
	.unlocked_ioctl	  = compat_cedardev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_cedardev_ioctl,
#endif
};

static int ve_debugfs_open(struct inode *inode, struct file *file)
{
	int i = 0;
	char *pData;
	struct ve_debugfs_proc *pVeProc;

	pVeProc = kmalloc(sizeof(struct ve_debugfs_proc), GFP_KERNEL);
	if (pVeProc == NULL) {
		VE_LOGE("kmalloc pVeProc fail\n");
		return -ENOMEM;
	}
	pVeProc->len = 0;
	memset(pVeProc->data, 0, VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL);

	pData = pVeProc->data;
	mutex_lock(&ve_debug_proc_info.lock_proc);
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++) {

	if (ve_debug_proc_info.proc_buf[i] != NULL) {
			memcpy(pData, ve_debug_proc_info.proc_buf[i],
						ve_debug_proc_info.proc_len[i]);
			pData		+= ve_debug_proc_info.proc_len[i];
			pVeProc->len += ve_debug_proc_info.proc_len[i];
		}
	}
    mutex_unlock(&ve_debug_proc_info.lock_proc);

	file->private_data = pVeProc;
	return 0;
}

static ssize_t ve_debugfs_read(struct file *file, char __user *user_buf,
					  size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc *pVeProc = file->private_data;

	if (pVeProc->len == 0) {
	    VE_LOGD("there is no any codec working currently\n");
	return 0;
    }

	return simple_read_from_buffer(user_buf, nbytes, ppos, pVeProc->data,
					   pVeProc->len);
}

static int ve_debugfs_release(struct inode *inode, struct file *file)
{
	struct ve_debugfs_proc *pVeProc = file->private_data;

	kfree(pVeProc);
	pVeProc = NULL;
	file->private_data = NULL;

	return 0;
}

static const struct file_operations ve_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = ve_debugfs_open,
	.llseek = no_llseek,
	.read = ve_debugfs_read,
	.release = ve_debugfs_release,
};

int sunxi_ve_debug_register_driver(void)
{
	struct dentry *dent;

#if (defined CONFIG_ARCH_SUN8IW12P1)
	ve_debugfs_root = debugfs_mpp_root;
#endif

	if (ve_debugfs_root == NULL) {
		VE_LOGE("get debugfs_mpp_root is NULL, please check mpp\n");
		return -ENOENT;
	}

	dent = debugfs_create_file("ve", 0444, ve_debugfs_root,
				   NULL, &ve_debugfs_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(ve_debugfs_root);
		ve_debugfs_root = NULL;
		return -ENODEV;
	}

	return 0;
}

void sunxi_ve_debug_unregister_driver(void)
{
	if (ve_debugfs_root == NULL)
		return;
	debugfs_remove_recursive(ve_debugfs_root);
	ve_debugfs_root = NULL;
}

static int cedardev_init(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;
	int devno;
	unsigned int val;

#if defined(CONFIG_OF)
	struct device_node *node;
#endif
	dev_t dev;

	dev = 0;

	VE_LOGD("install start!!!\n");

#if defined(CONFIG_OF)
	node = pdev->dev.of_node;
#endif

	/*register or alloc the device number.*/
	if (g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);
		ret = register_chrdev_region(dev, 1, "cedar_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "cedar_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		cedar_ve_printk(KERN_WARNING, "cedar_dev: can't get major %d\n", \
			g_dev_major);
		return ret;
	}
	spin_lock_init(&cedar_spin_lock);
	cedar_devp = kmalloc(sizeof(struct cedar_dev), GFP_KERNEL);
	if (cedar_devp == NULL) {
		cedar_ve_printk(KERN_WARNING, "malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	memset(cedar_devp, 0, sizeof(struct cedar_dev));

	cedar_devp->platform_dev = &pdev->dev;

#if defined(CONFIG_OF)
	cedar_devp->irq = irq_of_parse_and_map(node, 0);
	cedar_ve_printk(KERN_INFO, "cedar-ve the get irq is %d\n", \
		cedar_devp->irq);
	if (cedar_devp->irq <= 0)
		cedar_ve_printk(KERN_WARNING, "Can't parse IRQ");
#else
	cedar_devp->irq = SUNXI_IRQ_VE;
#endif

	sema_init(&cedar_devp->sem, 1);
	init_waitqueue_head(&cedar_devp->wq);

	memset(&cedar_devp->iomap_addrs, 0, sizeof(struct iomap_para));

	ret = request_irq(cedar_devp->irq, VideoEngineInterupt, 0, "cedar_dev", NULL);
	if (ret < 0) {
		cedar_ve_printk(KERN_WARNING, "request irq err\n");
		return -EINVAL;
	}

	/* map for macc io space */
#if defined(CONFIG_OF)
	cedar_devp->iomap_addrs.regs_macc = of_iomap(node, 0);
	if (!cedar_devp->iomap_addrs.regs_macc)
		cedar_ve_printk(KERN_WARNING, "ve Can't map registers");

	cedar_devp->sram_bass_vir = (unsigned int *)of_iomap(node, 1);
	if (!cedar_devp->sram_bass_vir)
		cedar_ve_printk(KERN_WARNING, "ve Can't map sram_bass_vir registers");

	cedar_devp->clk_bass_vir = (unsigned int *)of_iomap(node, 2);
	if (!cedar_devp->clk_bass_vir)
		cedar_ve_printk(KERN_WARNING, "ve Can't map clk_bass_vir registers");
#endif

#if (defined CONFIG_ARCH_SUNIVW1P1) /*for 1663*/
	val = readl(cedar_devp->clk_bass_vir+6);
	val &= 0x7fff80f0;
	val = val | (1<<31) | (8<<8);
	writel(val, cedar_devp->clk_bass_vir+6);

	/*set VE clock dividor*/
	val = readl(cedar_devp->clk_bass_vir+79);
	val |= (1<<31);
	writel(val, cedar_devp->clk_bass_vir+79);

	/*Active AHB bus to MACC*/
	val = readl(cedar_devp->clk_bass_vir+25);
	val |= (1<<0);
	writel(val, cedar_devp->clk_bass_vir+25);

	/*Power on and release reset ve*/
	val = readl(cedar_devp->clk_bass_vir+177);
	val &= ~(1<<0); /*reset ve*/
	writel(val, cedar_devp->clk_bass_vir+177);

	val = readl(cedar_devp->clk_bass_vir+177);
	val |= (1<<0);
	writel(val, cedar_devp->clk_bass_vir+177);

	/*gate on the bus to SDRAM*/
	val = readl(cedar_devp->clk_bass_vir+64);
	val |= (1<<0);
	writel(val, cedar_devp->clk_bass_vir+64);

	/*VE_SRAM mapping to AC320*/
	val = readl(cedar_devp->sram_bass_vir);
	val &= 0x80000000;
	writel(val, cedar_devp->sram_bass_vir);

	/*remapping SRAM to MACC for codec test*/
	val = readl(cedar_devp->sram_bass_vir);
	val |= 0x7fffffff;
	writel(val, cedar_devp->sram_bass_vir);

	/*clear bootmode bit for give sram to ve*/
	val = readl((cedar_devp->sram_bass_vir + 1));
	val &= 0xefffffff;
	writel(val, (cedar_devp->sram_bass_vir + 1));
#elif (defined CONFIG_ARCH_SUN3IW1P1)
	val = readl(cedar_devp->clk_bass_vir+6);
	val &= 0x7fff80f0;
	val = val | (1<<31) | (49<<8) | (3<<0);
	writel(val, cedar_devp->clk_bass_vir+6);

	/*set VE clock dividor*/
	val = readl(cedar_devp->clk_bass_vir+79);
	val |= (1<<31);
	writel(val, cedar_devp->clk_bass_vir+79);

	/*Active AHB bus to MACC*/
	val = readl(cedar_devp->clk_bass_vir+25);
	val |= (1<<0);
	writel(val, cedar_devp->clk_bass_vir+25);

	/*Power on and release reset ve*/
	val = readl(cedar_devp->clk_bass_vir+177);
	val &= ~(1<<0); /*reset ve*/
	writel(val, cedar_devp->clk_bass_vir+177);

	val = readl(cedar_devp->clk_bass_vir+177);
	val |= (1<<0);
	writel(val, cedar_devp->clk_bass_vir+177);

	/*gate on the bus to SDRAM*/
	val = readl(cedar_devp->clk_bass_vir+64);
	val |= (1<<0);
	writel(val, cedar_devp->clk_bass_vir+64);

	/*VE_SRAM mapping to AC320*/
	val = readl(cedar_devp->sram_bass_vir);
	val &= 0x80000000;
	writel(val, cedar_devp->sram_bass_vir);

	/*remapping SRAM to MACC for codec test*/
	val = readl(cedar_devp->sram_bass_vir);
	val |= 0x7fffffff;
	writel(val, cedar_devp->sram_bass_vir);

	/*switch to normal mode from boot mode*/
	val = readl(cedar_devp->sram_bass_vir+1);
	val &= 0xefffffff;
	writel(val, cedar_devp->sram_bass_vir+1);
#else
	#if (defined CONFIG_ARCH_SUN8IW12P1 || defined CONFIG_ARCH_SUN8IW17P1 || \
		defined CONFIG_ARCH_SUN8IW15P1 || defined CONFIG_ARCH_SUN50IW9P1)
		/*remapping SRAM to MACC for codec test*/
		val = readl(cedar_devp->sram_bass_vir);
		val |= 0x7fffffff;
		writel(val, cedar_devp->sram_bass_vir);

		/*VE_SRAM mapping to AC320*/
		val = readl(cedar_devp->sram_bass_vir);
		val &= 0x80000000;
		writel(val, cedar_devp->sram_bass_vir);
	#else
		/*VE_SRAM mapping to AC320*/
		val = readl(cedar_devp->sram_bass_vir);
		val &= 0x80000000;
		writel(val, cedar_devp->sram_bass_vir);

		/*remapping SRAM to MACC for codec test*/
		val = readl(cedar_devp->sram_bass_vir);
		val |= 0x7fffffff;
		writel(val, cedar_devp->sram_bass_vir);
	#endif

	/*clear bootmode bit for give sram to ve*/
	val = readl((cedar_devp->sram_bass_vir + 1));
	val &= 0xfeffffff;
	writel(val, (cedar_devp->sram_bass_vir + 1));
#endif


#if defined(CONFIG_OF)
	ve_parent_pll_clk = of_clk_get(node, 0);
	if ((!ve_parent_pll_clk) || IS_ERR(ve_parent_pll_clk)) {
		cedar_ve_printk(KERN_WARNING, "try to get ve_parent_pll_clk fail\n");
		return -EINVAL;
	}

	ve_moduleclk = of_clk_get(node, 1);
	if (!ve_moduleclk || IS_ERR(ve_moduleclk)) {
		cedar_ve_printk(KERN_WARNING, "get ve_moduleclk failed;\n");
	}
#endif

	/*no reset ve module*/
	sunxi_periph_reset_assert(ve_moduleclk);
	clk_prepare(ve_moduleclk);

	/* Create char device */
	devno = MKDEV(g_dev_major, g_dev_minor);
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	/* cedar_devp->cdev.ops = &cedardev_fops; */
	ret = cdev_add(&cedar_devp->cdev, devno, 1);
	if (ret) {
		cedar_ve_printk(KERN_WARNING, "Err:%d add cedardev", ret);
	}
	cedar_devp->class = class_create(THIS_MODULE, "cedar_dev");
	cedar_devp->dev	  = device_create(cedar_devp->class, NULL, devno, NULL, "cedar_dev");

	setup_timer(&cedar_devp->cedar_engine_timer, cedar_engine_for_events, (unsigned long)cedar_devp);
	setup_timer(&cedar_devp->cedar_engine_timer_rel, cedar_engine_for_timer_rel, (unsigned long)cedar_devp);

	mutex_init(&cedar_devp->lock_vdec);
	mutex_init(&cedar_devp->lock_venc);
	mutex_init(&cedar_devp->lock_jdec);
	mutex_init(&cedar_devp->lock_00_reg);
	mutex_init(&cedar_devp->lock_04_reg);
	mutex_init(&cedar_devp->lock_mem);

	ret = sunxi_ve_debug_register_driver();
	if (ret) {
		VE_LOGE("sunxi ve debug register driver failed!\n");
		return ret;
	}

	memset(&ve_debug_proc_info, 0, sizeof(struct ve_debugfs_buffer));
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++) {
		ve_debug_proc_info.proc_buf[i] = NULL;
	}
	ve_debug_proc_info.data = kmalloc(VE_DEBUGFS_BUF_SIZE*VE_DEBUGFS_MAX_CHANNEL, GFP_KERNEL);
	if (!ve_debug_proc_info.data) {
		VE_LOGE("kmalloc proc buffer failed!\n");
		return -ENOMEM;
	}
	mutex_init(&ve_debug_proc_info.lock_proc);
	VE_LOGD("ve_debug_proc_info:%p, data:%p, lock:%p\n",
							&ve_debug_proc_info,
							ve_debug_proc_info.data,
							&ve_debug_proc_info.lock_proc);

	VE_LOGD("install end!!!\n");
	return 0;
}

static void cedardev_exit(void)
{
	dev_t dev;

	dev = MKDEV(g_dev_major, g_dev_minor);

	free_irq(cedar_devp->irq, NULL);
	iounmap(cedar_devp->iomap_addrs.regs_macc);
	/* Destroy char device */
	if (cedar_devp) {
		cdev_del(&cedar_devp->cdev);
		device_destroy(cedar_devp->class, dev);
		class_destroy(cedar_devp->class);
	}

	if (NULL == ve_moduleclk || IS_ERR(ve_moduleclk)) {
		cedar_ve_printk(KERN_WARNING, "ve_moduleclk handle is invalid,just return!\n");
	} else {
		clk_disable_unprepare(ve_moduleclk);
		clk_put(ve_moduleclk);
		ve_moduleclk = NULL;
	}


	if (NULL == ve_parent_pll_clk || IS_ERR(ve_parent_pll_clk)) {
		cedar_ve_printk(KERN_WARNING, "ve_parent_pll_clk handle is invalid,just return!\n");
	} else {
		clk_put(ve_parent_pll_clk);
	}

#if defined CONFIG_ARCH_SUN9IW1P1
	regulator_put(regu);

	if (NULL == ve_power_gating || IS_ERR(ve_power_gating)) {
		cedar_ve_printk(KERN_WARNING, "ve_power_gating handle is invalid,just return!\n");
	} else {
		clk_disable_unprepare(ve_power_gating);
		clk_put(ve_power_gating);
		ve_power_gating = NULL;
	}
#endif

	unregister_chrdev_region(dev, 1);
	if (cedar_devp) {
		kfree(cedar_devp);
	}

	sunxi_ve_debug_unregister_driver();
	kfree(ve_debug_proc_info.data);
}

static int	sunxi_cedar_remove(struct platform_device *pdev)
{
	cedardev_exit();
	return 0;
}

static int	sunxi_cedar_probe(struct platform_device *pdev)
{
	cedardev_init(pdev);
	return 0;
}

static struct platform_driver sunxi_cedar_driver = {
	.probe		= sunxi_cedar_probe,
	.remove		= sunxi_cedar_remove,
#if defined(CONFIG_PM)
	.suspend	= snd_sw_cedar_suspend,
	.resume		= snd_sw_cedar_resume,
#endif
	.driver		= {
		.name	= "sunxi-cedar",
		.owner	= THIS_MODULE,

#if defined(CONFIG_OF)
		.of_match_table = sunxi_cedar_ve_match,
#endif
	},
};

static int __init sunxi_cedar_init(void)
{
	/*need not to gegister device here,because the device is registered by device tree */
	/*platform_device_register(&sunxi_device_cedar);*/
	printk("sunxi cedar version 0.1\n");
	return platform_driver_register(&sunxi_cedar_driver);
}

static void __exit sunxi_cedar_exit(void)
{
	platform_driver_unregister(&sunxi_cedar_driver);
}

module_init(sunxi_cedar_init);
module_exit(sunxi_cedar_exit);


MODULE_AUTHOR("Soft-Reuuimlla");
MODULE_DESCRIPTION("User mode CEDAR device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:cedarx-sunxi");


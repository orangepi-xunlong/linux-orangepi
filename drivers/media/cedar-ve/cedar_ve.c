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
#include <linux/signal.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/mm.h>
#include <asm/siginfo.h>
#include <asm/signal.h>
#include <linux/sched/signal.h>
//#include <linux/clk/sunxi.h>
#include <linux/debugfs.h>

#if IS_ENABLED(CONFIG_SUNXI_MPP)
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
//#include <linux/sunxi_mbus.h>
#include <linux/reset.h>
#include <asm/cacheflush.h>
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

/* just for decoder case with v5v200*/
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
	#define VE_POWER_MANAGE_VALID (1)
#else
	#define VE_POWER_MANAGE_VALID (0)
#endif

#define CEDAR_DEBUG 0
#define cedar_ve_printk(level, msg...) printk(level "cedar_ve: " msg)

#define VE_LOGD(fmt, arg...) printk(KERN_DEBUG"VE: "fmt"\n", ##arg)
#define VE_LOGI(fmt, arg...) printk(KERN_INFO"VE: "fmt"\n", ##arg)
#define VE_LOGW(fmt, arg...) printk(KERN_WARNING"VE: "fmt"\n", ##arg)
#define VE_LOGE(fmt, arg...) printk(KERN_ERR"VE: "fmt"\n", ##arg)

#define VE_CLK_HIGH_WATER  (900)
#define VE_CLK_LOW_WATER   (100)

#define PRINTK_IOMMU_ADDR 0
#define MAX_VE_DEBUG_INFO_NUM (16)

/* the struct must be same with cedarc/ve/veAw/veAw.h*/
struct debug_head_info {
	unsigned int pid;
	unsigned int tid;
	unsigned int length;
};
struct ve_debug_info {
	struct debug_head_info head_info;
	char *data;
};

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

struct iomap_para {
	volatile char *regs_ve;
	volatile char *regs_sys_cfg;
	volatile unsigned int *regs_ccmu;
	unsigned int *prcm_bass_vir;/*  PRCM: power reset clock management*/
};

static DECLARE_WAIT_QUEUE_HEAD(wait_ve);

struct cedar_dev {
	struct cdev cdev;				 /* char device struct				   */
	struct device *dev;				 /* ptr to class device struct		   */
	struct device *plat_dev;		 /* ptr to class device struct		   */
	struct class  *class;			 /* class for auto create device node  */

	struct semaphore sem;			 /* mutual exclusion semaphore		   */
	spinlock_t lock;
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
	int ref_count;
	int last_min_freq;

	u32 jpeg_irq_flag;					  /* flag of video jpeg dec irq generated */
	u32 jpeg_irq_value;					  /* value of video jpeg dec  irq */

	struct mutex lock_vdec;
	struct mutex lock_jdec;
	struct mutex lock_venc;
	struct mutex lock_00_reg;
	struct mutex lock_04_reg;
	struct aw_mem_list_head	   list;		/* buffer list */
	struct mutex lock_mem;
	unsigned char bMemDevAttachFlag;
	u32 power_manage_request_ref;
	struct ve_debug_info  debug_info[MAX_VE_DEBUG_INFO_NUM];
	int debug_info_cur_index;
	struct mutex lock_debug_info;
	struct reset_control *reset;
	struct clk *ve_clk;
	struct clk *bus_clk;
	struct clk *mbus_clk;
	struct clk *parent_clk;
	struct clk *power_clk;
	struct dentry *debug_root;
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

struct dma_buf_info {
	struct aw_mem_list_head i_list;
	int				fd;
	unsigned long	addr;
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



extern void cedar_dma_flush_range(const void *, size_t);
static int map_dma_buf_addr(int fd, unsigned int *addr);
static void unmap_dma_buf_addr(int unmap_all, int fd, unsigned int addr);

static irqreturn_t VideoEngineInterupt(int irq, void *dev)
{
	unsigned long ve_int_status_reg;
	unsigned long ve_int_ctrl_reg;
	unsigned int status;
	volatile int val;
	int modual_sel;
	unsigned int interrupt_enable;
	struct iomap_para addrs = cedar_devp->iomap_addrs;

	modual_sel = readl(addrs.regs_ve + 0x0);
#ifdef CONFIG_ARCH_SUN3IW1P1
	if (modual_sel&0xa) {
		if ((modual_sel&0xb) == 0xb) {
			/*jpg enc*/
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/*isp*/
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		if (status && interrupt_enable) {
			/*disable interrupt*/
			if ((modual_sel & 0xb) == 0xb) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
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
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/*isp*/
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		/*modify by fangning 2013-05-22*/
		if (status && interrupt_enable) {
			/*disable interrupt*/
			/*avc enc*/
			if (modual_sel&(1<<7)) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				/*isp*/
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
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
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1) \
	|| (defined CONFIG_ARCH_SUN8IW16P1) || (defined CONFIG_ARCH_SUN8IW19P1))
	if (modual_sel&(0x20)) {
		ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xe00 + 0x1c);
		ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xe00 + 0x14);
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
	if (modual_sel <= 6) {
		/*estimate Which video format*/
		switch (modual_sel) {
		case 0: /*mpeg124*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7c);
			break;
		case 1: /*h264*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x200 + 0x28);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x200 + 0x20);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 2: /*vc1*/
			ve_int_status_reg = (unsigned long)(addrs.regs_ve +
				0x300 + 0x2c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x300 + 0x24);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 3: /*rv*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x400 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x400 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 4: /*hevc*/
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x500 + 0x38);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x500 + 0x30);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 6: /*scaledown*/
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xF00);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xF00 + 0x04);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			break;
		default:
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			VE_LOGW("ve mode :%x " "not defined!\n", modual_sel);
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
	int res = 0;
	spin_lock_irqsave(&cedar_devp->lock, flags);

	if (clk_status == 1)
		goto out;

	clk_status = 1;

	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	reset_control_deassert(cedar_devp->reset);

	if (clk_prepare_enable(cedar_devp->bus_clk)) {
		VE_LOGW("enable bus clk gating failed;\n");
		res = -EINVAL;
		goto out;
	}

	if (clk_prepare_enable(cedar_devp->mbus_clk)) {
		VE_LOGW("enable mbus clk gating failed;\n");
		res = -EINVAL;
		goto out;
	}

	if (clk_prepare_enable(cedar_devp->ve_clk)) {
		VE_LOGW("enable ve clk gating failed;\n");
		res = -EINVAL;
		goto out;
	}

	AW_MEM_INIT_LIST_HEAD(&cedar_devp->list);

#ifdef CEDAR_DEBUG
	printk("%s,%d\n", __func__, __LINE__);
#endif

out:
	return res;
}

int disable_cedar_hw_clk(void)
{
	unsigned long flags;
	int res = 0;

	spin_lock_irqsave(&cedar_devp->lock, flags);

	if (clk_status == 0) {
		spin_unlock_irqrestore(&cedar_devp->lock, flags);
		goto out;
	}
	clk_status = 0;

	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	clk_disable_unprepare(cedar_devp->ve_clk);
	clk_disable_unprepare(cedar_devp->mbus_clk);
	clk_disable_unprepare(cedar_devp->bus_clk);
	reset_control_assert(cedar_devp->reset);

	unmap_dma_buf_addr(1, 0, 0);

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

	spin_lock_irqsave(&cedar_devp->lock, flags);

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

	spin_unlock_irqrestore(&cedar_devp->lock, flags);
}

int cedardev_del_task(int task_id)
{
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_devp->lock, flags);

	list_for_each_entry(task_entry, &run_task_list, list) {
		if (task_entry->t.ID == task_id && task_entry->status != TASK_RELEASE) {
			task_entry->status = TASK_RELEASE;

			spin_unlock_irqrestore(&cedar_devp->lock, flags);
			mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);
			return 0;
		}
	}
	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	return -1;
}

int cedardev_check_delay(int check_prio)
{
	struct cedarv_engine_task *task_entry;
	int timeout_total = 0;
	unsigned long flags;

	spin_lock_irqsave(&cedar_devp->lock, flags);
	list_for_each_entry(task_entry, &run_task_list, list) {
		if ((task_entry->t.task_prio >= check_prio) || (task_entry->running == 1) || (task_entry->is_first_task == 1))
			timeout_total = timeout_total + task_entry->t.frametime;
	}

	spin_unlock_irqrestore(&cedar_devp->lock, flags);
#ifdef CEDAR_DEBUG
	printk("%s,%d,%d\n", __func__, __LINE__, timeout_total);
#endif
	return timeout_total;
}

static void cedar_engine_for_timer_rel(struct timer_list *list)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&cedar_devp->lock, flags);

	if (list_empty(&run_task_list)) {
		ret = disable_cedar_hw_clk();
		if (ret < 0) {
			VE_LOGW("clk disable error!\n");
		}
	} else {
		VE_LOGW("clk disable time out but task left\n");
		mod_timer(&cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}

	spin_unlock_irqrestore(&cedar_devp->lock, flags);
}

static void cedar_engine_for_events(struct timer_list *list)
{
	struct cedarv_engine_task *task_entry, *task_entry_tmp;
	struct kernel_siginfo sig_info;
	unsigned long flags;

	spin_lock_irqsave(&cedar_devp->lock, flags);

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
		sig_info.si_signo = SIG_CEDAR;
		sig_info.si_code = task_entry->t.ID;
		if (task_entry->status == TASK_TIMEOUT) {
			sig_info.si_errno = TASK_TIMEOUT;
			send_sig_info(SIG_CEDAR, &sig_info, task_entry->task_handle);
		} else if (task_entry->status == TASK_RELEASE) {
			sig_info.si_errno = TASK_RELEASE;
			send_sig_info(SIG_CEDAR, &sig_info, task_entry->task_handle);
		}
		list_del(&task_entry->list);
		kfree(task_entry);
	}

	if (!list_empty(&run_task_list)) {
		task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
		if (task_entry->running == 0) {
			task_entry->running = 1;
			sig_info.si_signo = SIG_CEDAR;
			sig_info.si_code = task_entry->t.ID;
			sig_info.si_errno = TASK_INIT;
			send_sig_info(SIG_CEDAR, &sig_info, task_entry->task_handle);
		}

		mod_timer(&cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}

	spin_unlock_irqrestore(&cedar_devp->lock, flags);
}

#if 0/*(defined CONFIG_ARCH_SUN8IW16P1)*/
static int setVeFreqByReg(int fq)
{
	int max_count = 10000;
	int count = 0;
	unsigned int reg;
	int dst_freq = fq;
	unsigned int offset = (0x58/sizeof(unsigned int *));

	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);

	/*set VE freq*/
	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);
	reg &= 0x7ffa0000;
	fq = (fq/6) - 1;

	/* step 0  lock enable write 0  */
	reg = reg & (~(1 << 29));
	writel(reg, cedar_devp->iomap_addrs.regs_ccmu + offset);

	/* step 1 write clock parameter*/
	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);
	reg = reg | (fq<<8) | (1<<1) | (1<<0);

	writel(reg, cedar_devp->iomap_addrs.regs_ccmu + offset);

	/* step 2 write PLL enable */
	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);
	reg = reg | (1<<31);
	writel(reg, cedar_devp->iomap_addrs.regs_ccmu + offset);

	/* step 3  lock enable write 1 */
	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);
	reg = reg | (1<<29);
	writel(reg, cedar_devp->iomap_addrs.regs_ccmu + offset);

	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);

	/* step 4  check bit28(lock) is 1 or not  */
	do {
		count++;
		if (count > max_count) {
			VE_LOGE("set ve freq failed, bit28 = 0x%x", reg);
			break;
		}
		udelay(5);
		reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);
		reg = reg & (1 << 28);
	} while (reg == 0);

	reg = readl(cedar_devp->iomap_addrs.regs_ccmu + offset);

	VE_LOGW("ve freq reg = %x, dstFq = %d MHz, cout = %d, max = %d\n",
			reg, dst_freq, count, max_count);
	udelay(20);

	return 0;
}
#endif

#if VE_POWER_MANAGE_VALID
static int ve_power_manage_setup(void)
{
	int ret = 0;
	unsigned int reg;
	unsigned int power_off_gating_reg_offset = 0;
	unsigned int power_switch_reg_offset = 0;

	power_off_gating_reg_offset = (0x258/sizeof(unsigned int *));
	power_switch_reg_offset = (0x264/sizeof(unsigned int *));

	/* set VE Power Switch Reg bit[15:0] to 0*/
	reg = readl(cedar_devp->prcm_bass_vir + power_switch_reg_offset);
	reg = reg >> 16;
	reg = reg << 16;
	writel(reg, cedar_devp->prcm_bass_vir + power_switch_reg_offset);

	/* if VE Power Switch Reg bit[31:16] == 1, means setup sucees*/
	reg = readl(cedar_devp->prcm_bass_vir + power_switch_reg_offset);

	if ((reg >> 16) == 0)
		ret = 0;
	else
		ret = -1;

	/*VE_LOGI("setup: power_switch_reg = 0x%x, ret = %d",reg, ret);*/

	/* set VE Power off Gating Reg bit[0] to 0*/
	reg = readl(cedar_devp->prcm_bass_vir + power_off_gating_reg_offset);
	reg = reg & ~1;
	writel(reg, cedar_devp->prcm_bass_vir + power_off_gating_reg_offset);

	reg = readl(cedar_devp->prcm_bass_vir + power_off_gating_reg_offset);
	/*VE_LOGI("setup: power_off_reg = 0x%x",reg);*/

	/* reset ve gating */
	reset_control_deassert(cedar_devp->reset);
	reset_control_assert(cedar_devp->reset);
	return ret;
}

static int ve_power_manage_shutdown(void)
{
	int max_count = 1000;
	int count = 0;
	int ret = 0;
	unsigned int reg;
	unsigned int power_off_gating_reg_offset = 0;
	unsigned int power_switch_reg_offset = 0;

	power_off_gating_reg_offset	= (0x258/sizeof(unsigned int *));
	power_switch_reg_offset = (0x264/sizeof(unsigned int *));

	/* reset ve gating */
	reset_control_deassert(cedar_devp->reset);
	reset_control_assert(cedar_devp->reset);

	/* set VE Power off Gating Reg bit[0] to 1*/
	reg = readl(cedar_devp->prcm_bass_vir + power_off_gating_reg_offset);
	reg = reg | 1;
	writel(reg, cedar_devp->prcm_bass_vir + power_off_gating_reg_offset);

	reg = readl(cedar_devp->prcm_bass_vir + power_off_gating_reg_offset);
	/*VE_LOGI("shutdown: power_off_reg = 0x%x",reg); */

	/* set VE Power Switch Reg bit[15:0] to 0xffff*/
	reg = readl(cedar_devp->prcm_bass_vir + power_switch_reg_offset);
	reg = reg >> 16;
	reg = reg << 16;
	reg = reg | 0xffff;
	writel(reg, cedar_devp->prcm_bass_vir + power_switch_reg_offset);

	/* if VE Power Switch Reg bit[31:16] == 0xffff, means shutdown sucees*/
	reg = 0;
	do {
		count++;
		if (count > max_count) {
			VE_LOGE("shutdown ve power failed, power_switch_reg = 0x%x",
					reg);
			break;
		}

		udelay(5);
		reg = readl(cedar_devp->prcm_bass_vir + power_switch_reg_offset);
	} while ((reg >> 16) != 0xffff);

	if ((reg >> 16) == 0xffff)
		ret = 0;
	else
		ret = -1;


	/*VE_LOGI("shutdown: power_switch_reg = 0x%x, ret = %d, count = %d, max = %d", */
	/*		reg, ret, count, max_count);*/

	return ret;
}

#endif

static int map_dma_buf_addr(int fd, unsigned int *addr)
{
	struct sg_table *sgt;
	struct dma_buf_info *buf_info;

	buf_info = (struct dma_buf_info *)kmalloc(sizeof(struct dma_buf_info), GFP_KERNEL);
	if (buf_info == NULL) {
		VE_LOGE("malloc dma_buf_info error\n");
		return -1;
	}

	memset(buf_info, 0, sizeof(struct dma_buf_info));
	buf_info->dma_buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(buf_info->dma_buf)) {
		VE_LOGE("ve get dma_buf error");
		goto BUF_FREE;
	}

	buf_info->attachment = dma_buf_attach(buf_info->dma_buf, cedar_devp->plat_dev);
	if (IS_ERR_OR_NULL(buf_info->attachment)) {
		VE_LOGE("ve get dma_buf_attachment error");
		goto BUF_PUT;
	}

	sgt = dma_buf_map_attachment(buf_info->attachment, DMA_BIDIRECTIONAL);

	buf_info->sgt = sgt;
	if (IS_ERR_OR_NULL(buf_info->sgt)) {
		VE_LOGE("ve get sg_table error\n");
		goto BUF_DETATCH;
	}

	buf_info->addr = sg_dma_address(buf_info->sgt->sgl);
	buf_info->fd = fd;
	buf_info->p_id = current->tgid;
	#if PRINTK_IOMMU_ADDR
	VE_LOGI("fd:%d, buf_info:%p addr:%lx, dma_buf:%p, \
			dma_buf_attach:%p, sg_table:%p, nents:%d, pid:%d\n",
			buf_info->fd,
			buf_info,
			buf_info->addr,
			buf_info->dma_buf,
			buf_info->attachment,
			buf_info->sgt,
			buf_info->sgt->nents,
			buf_info->p_id);
	#endif

	mutex_lock(&cedar_devp->lock_mem);
	aw_mem_list_add_tail(&buf_info->i_list, &cedar_devp->list);
	mutex_unlock(&cedar_devp->lock_mem);

	*addr = buf_info->addr;
	return 0;

	dma_buf_unmap_attachment(buf_info->attachment, buf_info->sgt,
							DMA_BIDIRECTIONAL);
BUF_DETATCH:
	dma_buf_detach(buf_info->dma_buf, buf_info->attachment);
BUF_PUT:
	dma_buf_put(buf_info->dma_buf);
BUF_FREE:
	kfree(buf_info);
	return -1;
}

static void unmap_dma_buf_addr(int unmap_all, int fd, unsigned int addr)
{
	struct dma_buf_info *buf_info;
	struct aw_mem_list_head *pos;
	struct aw_mem_list_head *next;
	int tmp_fd;
    unsigned int tmp_addr;
	mutex_lock(&cedar_devp->lock_mem);
	/*
	aw_mem_list_for_each_entry(buf_info, &cedar_devp->list, i_list) {
		#if PRINTK_IOMMU_ADDR
		VE_LOGI("list2 fd:%d, buf_info:%p addr:%lx, dma_buf:%p,
				dma_buf_attach:%p, sg_table:%p, nents:%d, pid:%d\n",
			buf_info->fd,
			buf_info,
			buf_info->addr,
			buf_info->dma_buf,
			buf_info->attachment,
			buf_info->sgt,
			buf_info->sgt->nents,
			buf_info->p_id);
		#endif
	}
	*/
	aw_mem_list_for_each_safe(pos, next, &cedar_devp->list) {
		buf_info = (struct dma_buf_info *)pos;
		if (unmap_all) {
			tmp_fd = buf_info->fd;
			tmp_addr = buf_info->addr;
		} else {
			tmp_fd = fd;
			tmp_addr = addr;
		}
		if (buf_info->fd == tmp_fd && buf_info->p_id == current->tgid && buf_info->addr == addr) {
			#if PRINTK_IOMMU_ADDR
			VE_LOGI("free: fd:%d, buf_info:%p iommu_addr:%lx, dma_buf:%p, \
					dma_buf_attach:%p, sg_table:%p nets:%d, pid:%d\n",
			buf_info->fd,
			buf_info,
			buf_info->addr,
			buf_info->dma_buf,
			buf_info->attachment,
			buf_info->sgt,
			buf_info->sgt->nents,
			buf_info->p_id);
			#endif

			if (buf_info->dma_buf > 0) {
				if (buf_info->attachment > 0) {
					if (buf_info->sgt > 0) {
						dma_buf_unmap_attachment(buf_info->attachment,
											buf_info->sgt,
												DMA_BIDIRECTIONAL);
					}
					dma_buf_detach(buf_info->dma_buf, buf_info->attachment);
				}
				dma_buf_put(buf_info->dma_buf);
			}
			aw_mem_list_del(&buf_info->i_list);
			kfree(buf_info);
			if (unmap_all == 0)
				break;
		}
	}
	mutex_unlock(&cedar_devp->lock_mem);
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
	case IOCTL_PROC_INFO_COPY:
	{
		int i = 0;
		struct ve_debug_info *debug_info = NULL;
		struct debug_head_info head_info;

		/* parser head info */
		cedar_devp->debug_info_cur_index = -1;
		if (copy_from_user(&head_info, (void __user *)arg,
			sizeof(struct debug_head_info))) {
			VE_LOGW("SET_DEBUG_INFO copy_from_user fail\n");
			return -EFAULT;
		}


		/*VE_LOGD("size of debug_head_info: %d", (int)sizeof(struct debug_head_info)); */
		/*VE_LOGD("head_info: %d, %d, %d", head_info.pid, head_info.tid,head_info.length);*/


		mutex_lock(&cedar_devp->lock_debug_info);
		/* match debug info by pid and tid*/
		for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
			if (cedar_devp->debug_info[i].head_info.pid == head_info.pid
			&& cedar_devp->debug_info[i].head_info.tid == head_info.tid)
				break;
		}

		if (i >= MAX_VE_DEBUG_INFO_NUM) {
			for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
				if (cedar_devp->debug_info[i].head_info.pid == 0
				&& cedar_devp->debug_info[i].head_info.tid == 0)
					break;
			}

			if (i >= MAX_VE_DEBUG_INFO_NUM) {
				VE_LOGW("SET_DEBUG_INFO  overflow  MAX_VE_DEBUG_INFO_NUM[%d]\n",
								MAX_VE_DEBUG_INFO_NUM);
			} else {
				cedar_devp->debug_info_cur_index = i;
				cedar_devp->debug_info[i].head_info.pid    = head_info.pid;
				cedar_devp->debug_info[i].head_info.tid    = head_info.tid;
				cedar_devp->debug_info[i].head_info.length = head_info.length;
			}
		} else {
			cedar_devp->debug_info_cur_index = i;
			cedar_devp->debug_info[i].head_info.length = head_info.length;
		}


		if (cedar_devp->debug_info_cur_index < 0) {
			VE_LOGE("COPY_DEBUG_INFO, cur_index[%d] is invalided\n",
					cedar_devp->debug_info_cur_index);
			mutex_unlock(&cedar_devp->lock_debug_info);
			return -EFAULT;
		}

		/* copy data of debug info */
		debug_info = &cedar_devp->debug_info[cedar_devp->debug_info_cur_index];


		/*VE_LOGD("COPY_DEBUG_INFO, index = %d, pid = %d, tid = %d, len = %d, data = %p\n", */
		/*    cedar_devp->debug_info_cur_index, debug_info->head_info.pid, */
		/*    debug_info->head_info.tid, debug_info->head_info.length, debug_info->data);*/


		if (debug_info->data)
			vfree(debug_info->data);

		debug_info->data = vmalloc(debug_info->head_info.length);


		/*VE_LOGD("vmalloc: data = %p, len = %d\n",*/
		/*    debug_info->data, debug_info->head_info.length);*/


		if (debug_info->data == NULL) {
			mutex_unlock(&cedar_devp->lock_debug_info);
			VE_LOGE("vmalloc for debug_info->data failed");
			return -EFAULT;
		}

		if (copy_from_user(debug_info->data, (void __user *)(arg + sizeof(struct debug_head_info)),
			debug_info->head_info.length)) {
			mutex_unlock(&cedar_devp->lock_debug_info);
			VE_LOGE("COPY_DEBUG_INFO copy_from_user fail\n");
			return -EFAULT;
		}

		mutex_unlock(&cedar_devp->lock_debug_info);

		break;
	}
	case IOCTL_PROC_INFO_STOP:
	{
		int i = 0;
		struct debug_head_info head_info;

		if (copy_from_user(&head_info, (void __user *)arg, sizeof(struct debug_head_info))) {
			VE_LOGW("SET_DEBUG_INFO copy_from_user fail\n");
			return -EFAULT;
		}

		/*VE_LOGD("STOP_DEBUG_INFO, pid = %d, tid = %d\n",*/
		/*		head_info.pid, head_info.tid); */

		mutex_lock(&cedar_devp->lock_debug_info);
		for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
			if (cedar_devp->debug_info[i].head_info.pid == head_info.pid
				&& cedar_devp->debug_info[i].head_info.tid == head_info.tid)
				break;
		}

		VE_LOGI("STOP_DEBUG_INFO, i = %d\n", i);
		if (i >= MAX_VE_DEBUG_INFO_NUM) {
			VE_LOGE("STOP_DEBUG_INFO: can not find the match pid[%d] and tid[%d]\n",
				head_info.pid, head_info.tid);
		} else {
			if (cedar_devp->debug_info[i].data)
				vfree(cedar_devp->debug_info[i].data);

			cedar_devp->debug_info[i].data = NULL;
			cedar_devp->debug_info[i].head_info.pid = 0;
			cedar_devp->debug_info[i].head_info.tid = 0;
			cedar_devp->debug_info[i].head_info.length = 0;
		}

		mutex_unlock(&cedar_devp->lock_debug_info);

		break;
	}
	case IOCTL_ENGINE_REQ:
#ifdef USE_CEDAR_ENGINE
			if (copy_from_user(&task_ret, (void __user *)arg,
				sizeof(struct __cedarv_task))) {
				VE_LOGW("USE_CEDAR_ENGINE copy_from_user fail\n");
				return -EFAULT;
			}
			spin_lock_irqsave(&cedar_devp->lock, flags);

			if (!list_empty(&run_task_list) &&
				(task_ret.block_mode == CEDAR_NONBLOCK_TASK)) {
				spin_unlock_irqrestore(&cedar_devp->lock, flags);
				return CEDAR_RUN_LIST_NONULL;
			}
			spin_unlock_irqrestore(&cedar_devp->lock, flags);

			task_ptr = kmalloc(sizeof(struct cedarv_engine_task), GFP_KERNEL);
			if (!task_ptr) {
				VE_LOGW("get task_ptr error\n");
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
				VE_LOGW("IOCTL_ENGINE_REQ clk enable error!\n");
				return -EFAULT;
			}
			return task_ptr->is_first_task;
#else
			if (down_interruptible(&cedar_devp->sem))
				return -ERESTARTSYS;

			if (cedar_devp->ref_count < 0) {
				VE_LOGW("request ve ref count error!");
				up(&cedar_devp->sem);
				return -EFAULT;
			}

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

			if (cedar_devp->ref_count < 0) {
				VE_LOGW("relsese ve ref count error!");
				up(&cedar_devp->sem);
				return -EFAULT;
			}

			if (cedar_devp->ref_count == 0) {
				ret = disable_cedar_hw_clk();
				if (ret < 0) {
					VE_LOGW("IOCTL_ENGINE_REL clk disable error!\n");
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
					VE_LOGW("%d copy_from_user fail\n",
						IOCTL_ENGINE_CHECK_DELAY);
					return -EFAULT;
				}
				task_info.total_time = cedardev_check_delay(task_info.task_prio);
#ifdef CEDAR_DEBUG
				printk("%s,%d,%d\n", __func__, __LINE__, task_info.total_time);
#endif
				task_info.frametime = 0;
				spin_lock_irqsave(&cedar_devp->lock, flags);
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
				spin_unlock_irqrestore(&cedar_devp->lock, flags);

				if (copy_to_user((void *)arg, &task_info, sizeof(struct cedarv_engine_task_info))) {
					VE_LOGW("%d copy_to_user fail\n",
						IOCTL_ENGINE_CHECK_DELAY);
					return -EFAULT;
				}
			}
			break;
		case IOCTL_WAIT_VE_DE:
			ve_timeout = (int)arg;
			cedar_devp->de_irq_value = 0;

			spin_lock_irqsave(&cedar_devp->lock, flags);
			if (cedar_devp->de_irq_flag)
				cedar_devp->de_irq_value = 1;
			spin_unlock_irqrestore(&cedar_devp->lock, flags);
			wait_event_timeout(wait_ve, cedar_devp->de_irq_flag, ve_timeout*HZ);
			cedar_devp->de_irq_flag = 0;

			return cedar_devp->de_irq_value;

		case IOCTL_WAIT_VE_EN:

			ve_timeout = (int)arg;
			cedar_devp->en_irq_value = 0;

			spin_lock_irqsave(&cedar_devp->lock, flags);
			if (cedar_devp->en_irq_flag)
				cedar_devp->en_irq_value = 1;
			spin_unlock_irqrestore(&cedar_devp->lock, flags);

			wait_event_timeout(wait_ve, cedar_devp->en_irq_flag, ve_timeout*HZ);
			cedar_devp->en_irq_flag = 0;

			return cedar_devp->en_irq_value;

#if ((defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN50I) || \
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1) \
	|| (defined CONFIG_ARCH_SUN8IW16P1) || (defined CONFIG_ARCH_SUN8IW19P1))

		case IOCTL_WAIT_JPEG_DEC:
			ve_timeout = (int)arg;
			cedar_devp->jpeg_irq_value = 0;

			spin_lock_irqsave(&cedar_devp->lock, flags);
			if (cedar_devp->jpeg_irq_flag)
				cedar_devp->jpeg_irq_value = 1;
			spin_unlock_irqrestore(&cedar_devp->lock, flags);

			wait_event_timeout(wait_ve, cedar_devp->jpeg_irq_flag, ve_timeout*HZ);
			cedar_devp->jpeg_irq_flag = 0;
			return cedar_devp->jpeg_irq_value;
#endif
		case IOCTL_ENABLE_VE:
			if (clk_prepare_enable(cedar_devp->ve_clk)) {
				VE_LOGW("IOCTL_ENABLE_VE enable cedar_devp->ve_clk failed!\n");
			}
			break;

		case IOCTL_DISABLE_VE:
			if ((cedar_devp->ve_clk == NULL) || IS_ERR(cedar_devp->ve_clk)) {
				VE_LOGW("IOCTL_DISABLE_VE cedar_devp->ve_clk is invalid\n");
				return -EFAULT;
			} else {
				clk_disable_unprepare(cedar_devp->ve_clk);
			}
			break;

		case IOCTL_RESET_VE:
			reset_control_reset(cedar_devp->reset);
			break;

		case IOCTL_SET_DRAM_HIGH_CHANNAL:
		{
			/*
			int flag = (int)arg;

			if (flag == 1)
				mbus_port_setpri(26, 1);
			else
				mbus_port_setpri(26, 0);

			VE_LOGW("this kernel version not supproti!\n");
			*/
			break;
		}

		case IOCTL_SET_VE_FREQ:
			{
				int arg_rate = (int)arg;
				//note now we not support set parent clk
				if (arg_rate >= VE_CLK_LOW_WATER && arg_rate <= VE_CLK_HIGH_WATER) {
					if (clk_set_rate(cedar_devp->ve_clk, arg_rate*1000000)) {
						VE_LOGW("set clock failed\n");
					}
				}
				ret = clk_get_rate(cedar_devp->ve_clk);
				VE_LOGW("VE real_freq=%ld\n", ret);
				break;
			}
		case IOCTL_GETVALUE_AVS2:
		case IOCTL_ADJUST_AVS2:
		case IOCTL_ADJUST_AVS2_ABS:
		case IOCTL_CONFIG_AVS2:
		case IOCTL_RESET_AVS2:
		case IOCTL_PAUSE_AVS2:
		case IOCTL_START_AVS2:
			VE_LOGW("do not supprot this ioctrl now\n");
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
					VE_LOGW(\
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
				u32 lock_type = VE_LOCK_PROC_INFO;
				struct ve_info *vi = filp->private_data;

				if (cedar_devp->debug_root == NULL)
					return 0;

				mutex_lock(&ve_debug_proc_info.lock_proc);
				if (copy_from_user(&ve_info, (void __user *)arg, sizeof(struct VE_PROC_INFO))) {
					VE_LOGW("IOCTL_SET_PROC_INFO copy_from_user fail\n");
					mutex_unlock(&ve_debug_proc_info.lock_proc);
					return -EFAULT;
				}

				channel_id = ve_info.channel_id;
				if (channel_id >= VE_DEBUGFS_MAX_CHANNEL) {
					VE_LOGW("set channel[%c] is bigger than max channel[%d]\n",
											channel_id, VE_DEBUGFS_MAX_CHANNEL);
					mutex_unlock(&ve_debug_proc_info.lock_proc);
					return -EFAULT;
				}

				mutex_lock(&vi->lock_flag_io);
				vi->lock_flags |= lock_type;
				mutex_unlock(&vi->lock_flag_io);

				ve_debug_proc_info.cur_channel_id = ve_info.channel_id;
				ve_debug_proc_info.proc_len[channel_id] = ve_info.proc_info_len;
				ve_debug_proc_info.proc_buf[channel_id] = ve_debug_proc_info.data +
													channel_id * VE_DEBUGFS_BUF_SIZE;
				break;
			}
			case IOCTL_COPY_PROC_INFO:
			{
				unsigned char channel_id;
				u32 lock_type = VE_LOCK_PROC_INFO;
				struct ve_info *vi = filp->private_data;

				if (cedar_devp->debug_root == NULL)
					return 0;

				mutex_lock(&vi->lock_flag_io);
				vi->lock_flags &= (~lock_type);
				mutex_unlock(&vi->lock_flag_io);

				channel_id = ve_debug_proc_info.cur_channel_id;
				if (copy_from_user(ve_debug_proc_info.proc_buf[channel_id],
								  (void __user *)arg,
								ve_debug_proc_info.proc_len[channel_id])) {
					VE_LOGW("IOCTL_COPY_PROC_INFO copy_from_user fail\n");
							mutex_unlock(&ve_debug_proc_info.lock_proc);
					return -EFAULT;
				}

				mutex_unlock(&ve_debug_proc_info.lock_proc);
				break;
			}
			case IOCTL_STOP_PROC_INFO:
			{
				unsigned char channel_id;

				if (cedar_devp->debug_root == NULL)
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
			/* just for compatible, kernel 5.4 should not use it*/
			struct user_iommu_param parm;
			cedar_devp->bMemDevAttachFlag = 1;

			if (copy_from_user(&parm, (void __user *)arg,
				sizeof(struct user_iommu_param))) {
				VE_LOGE("IOCTL_GET_IOMMU_ADDR copy_from_user error");
				return -EFAULT;
			}
			if (map_dma_buf_addr(parm.fd, &parm.iommu_addr) != 0) {
				VE_LOGE("IOCTL_GET_IOMMU_ADDR map dma buf fail");
				return -EFAULT;
			}
			if (copy_to_user((void __user *)arg, &parm,
				sizeof(struct user_iommu_param))) {
				VE_LOGE("ve get iommu copy_to_user error\n");
				return -EFAULT;
			}
			break;
		}
		case IOCTL_FREE_IOMMU_ADDR:
		{
			/* just for compatible, kernel 5.4 should not use it*/
			struct user_iommu_param parm;

			if (copy_from_user(&parm, (void __user *)arg,
				sizeof(struct user_iommu_param))) {
				VE_LOGE("IOCTL_FREE_IOMMU_ADDR copy_from_user error");
				return -EFAULT;
			}

			unmap_dma_buf_addr(0, parm.fd, parm.iommu_addr);
			break;
		}
		case IOCTL_MAP_DMA_BUF:
		{
			struct dma_buf_param parm;
			cedar_devp->bMemDevAttachFlag = 1;

			if (copy_from_user(&parm, (void __user *)arg, sizeof(struct dma_buf_param))) {
				VE_LOGE("IOCTL_GET_IOMMU_ADDR copy_from_user error");
				return -EFAULT;
			}
			if (map_dma_buf_addr(parm.fd, &parm.phy_addr) != 0) {
				VE_LOGE("IOCTL_GET_IOMMU_ADDR map dma buf fail");
				return -EFAULT;
			}
			if (copy_to_user((void __user *)arg, &parm, sizeof(struct dma_buf_param))) {
				VE_LOGE("ve get iommu copy_to_user error\n");
				return -EFAULT;
			}
			break;
		}
		case IOCTL_UNMAP_DMA_BUF:
		{
			struct dma_buf_param parm;

			if (copy_from_user(&parm, (void __user *)arg, sizeof(struct dma_buf_param))) {
				VE_LOGE("IOCTL_FREE_IOMMU_ADDR copy_from_user error");
				return -EFAULT;
			}

			unmap_dma_buf_addr(0, parm.fd, parm.phy_addr);
			break;
		}
		case IOCTL_FLUSH_CACHE_RANGE:
		{
			struct cache_range data;

			if (copy_from_user(&data, (void __user *)arg, sizeof(struct cache_range)))
				return -EFAULT;

			if (IS_ERR_OR_NULL((void *)data.start) || IS_ERR_OR_NULL((void *)data.end)) {
				VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
					   (u32)data.start, (u32)data.end);
				return -EFAULT;
			}

			//VE_LOGI("ion flush_range start:%lx end:%lx size:%lx\n",
			//		 data.start, data.end, data.end - data.start);
#if IS_ENABLED(CONFIG_ARM64)
			cedar_dma_flush_range((void *)data.start, data.end - data.start);
#else
			dmac_flush_range((void *)data.start, (void *)data.end);
#endif
			if (copy_to_user((void __user *)arg, &data, sizeof(data)))
				return -EFAULT;
			break;
		}
		case IOCTL_POWER_SETUP:
		{
#if VE_POWER_MANAGE_VALID
			if (cedar_devp->power_manage_request_ref == 0) {
				/* ensure ve is not work, or will cause error */
				mutex_lock(&cedar_devp->lock_vdec);
				mutex_lock(&cedar_devp->lock_venc);
				mutex_lock(&cedar_devp->lock_jdec);
				mutex_lock(&cedar_devp->lock_00_reg);
				mutex_lock(&cedar_devp->lock_04_reg);

				ve_power_manage_setup();

				mutex_unlock(&cedar_devp->lock_vdec);
				mutex_unlock(&cedar_devp->lock_venc);
				mutex_unlock(&cedar_devp->lock_jdec);
				mutex_unlock(&cedar_devp->lock_00_reg);
				mutex_unlock(&cedar_devp->lock_04_reg);
			}

			cedar_devp->power_manage_request_ref++;
#endif
			break;
		}
		case IOCTL_POWER_SHUTDOWN:
		{
#if VE_POWER_MANAGE_VALID
			cedar_devp->power_manage_request_ref--;

			if (cedar_devp->power_manage_request_ref == 0) {
				/* ensure ve is not work, or will cause error */
				mutex_lock(&cedar_devp->lock_vdec);
				mutex_lock(&cedar_devp->lock_venc);
				mutex_lock(&cedar_devp->lock_jdec);
				mutex_lock(&cedar_devp->lock_00_reg);
				mutex_lock(&cedar_devp->lock_04_reg);

				ve_power_manage_shutdown();

				mutex_unlock(&cedar_devp->lock_vdec);
				mutex_unlock(&cedar_devp->lock_venc);
				mutex_unlock(&cedar_devp->lock_jdec);
				mutex_unlock(&cedar_devp->lock_00_reg);
				mutex_unlock(&cedar_devp->lock_04_reg);
			}
#endif
			break;
		}
		default:
			VE_LOGW("not support the ioctl cmd = 0x%x", cmd);
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

	info = filp->private_data;
	mutex_lock(&info->lock_flag_io);
	//if the process abort, this will free iommu_buffer
	unmap_dma_buf_addr(1, 0, 0);

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

			if (info->lock_flags & VE_LOCK_PROC_INFO)
				mutex_unlock(&ve_debug_proc_info.lock_proc);

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
			VE_LOGW(\
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
		VE_LOGW("vma->vm_end is equal vma->vm_start : %lx\n",\
			vma->vm_start);
		return 0;
	}
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) {
		VE_LOGW(\
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

static int sunxi_cedar_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;

	printk("[cedar] standby suspend\n");
	ret = disable_cedar_hw_clk();

#if defined CONFIG_ARCH_SUN9IW1P1
	clk_disable_unprepare(cedar_devp->power_clk);
#endif

	if (ret < 0) {
		VE_LOGW("cedar clk disable somewhere error!\n");
		return -EFAULT;
	}

	return 0;
}

static int sunxi_cedar_resume(struct platform_device *pdev)
{
	int ret = 0;

	printk("[cedar] standby resume\n");

#if defined CONFIG_ARCH_SUN9IW1P1
	clk_prepare_enable(cedar_devp->power_clk);
#endif

	if (cedar_devp->ref_count == 0) {
		return 0;
	}

	ret = enable_cedar_hw_clk();
	if (ret < 0) {
		VE_LOGW("cedar clk enable somewhere error!\n");
		return -EFAULT;
	}
	return 0;
}




#if IS_ENABLED(CONFIG_DEBUG_FS)
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
			memcpy(pData, ve_debug_proc_info.proc_buf[i], ve_debug_proc_info.proc_len[i]);
			pData += ve_debug_proc_info.proc_len[i];
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

	//debugfs_mpp_root defined out of this module
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	cedar_devp->debug_root = debugfs_mpp_root;
#else
	cedar_devp->debug_root = debugfs_create_dir("enc_test", NULL);
#endif

	if (IS_ERR_OR_NULL(cedar_devp->debug_root)) {
		VE_LOGE("debugfs root is null please check!");
		return -ENOENT;
	}
	dent = debugfs_create_file("ve", 0444, cedar_devp->debug_root,
				   NULL, &ve_debugfs_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(cedar_devp->debug_root);
		cedar_devp->debug_root = NULL;
		return -ENODEV;
	}

	return 0;
}

void sunxi_ve_debug_unregister_driver(void)
{
	if (cedar_devp->debug_root == NULL) {
		VE_LOGW("note: debug root already is null");
		return;
	}
	debugfs_remove_recursive(cedar_devp->debug_root);
	cedar_devp->debug_root = NULL;
}
#endif

#if (defined CONFIG_ARCH_SUNIVW1P1 || defined CONFIG_ARCH_SUN3IW1P1)
static void set_ccmu_by_self(void)
{
	unsigned int val;

	if (cedar_devp->iomap_addrs.regs_ccmu == NULL) {
		VE_LOGW("ccmu regs addr is null");
		return;
	}
	val = readl(cedar_devp->iomap_addrs.regs_ccmu+6);
	val &= 0x7fff80f0;
#if (defined CONFIG_ARCH_SUNIVW1P1) /* for 1663*/
	val = val | (1<<31) | (8<<8);
#elif (defined CONFIG_ARCH_SUN3IW1P1)
	val = val | (1<<31) | (49<<8) | (3<<0);
#endif
	writel(val, cedar_devp->iomap_addrs.regs_ccmu+6);

	/*set VE clock dividor*/
	val = readl(cedar_devp->iomap_addrs.regs_ccmu+79);
	val |= (1<<31);
	writel(val, cedar_devp->iomap_addrs.regs_ccmu+79);

	/*Active AHB bus to MACC*/
	val = readl(cedar_devp->iomap_addrs.regs_ccmu+25);
	val |= (1<<0);
	writel(val, cedar_devp->iomap_addrs.regs_ccmu+25);

	/*Power on and release reset ve*/
	val = readl(cedar_devp->iomap_addrs.regs_ccmu+177);
	val &= ~(1<<0); /*reset ve*/
	writel(val, cedar_devp->iomap_addrs.regs_ccmu+177);

	val = readl(cedar_devp->iomap_addrs.regs_ccmu+177);
	val |= (1<<0);
	writel(val, cedar_devp->iomap_addrs.regs_ccmu+177);

	/*gate on the bus to SDRAM*/
	val = readl(cedar_devp->iomap_addrs.regs_ccmu+64);
	val |= (1<<0);
	writel(val, cedar_devp->iomap_addrs.regs_ccmu+64);
}
#endif

static void set_sys_cfg_by_self(void)
{
	unsigned int val;
	if (cedar_devp->iomap_addrs.regs_sys_cfg == NULL) {
	VE_LOGW("sys config regs addr is null");
		return;
	}
	// remapping SRAM to MACC for codec test
	val = readl(cedar_devp->iomap_addrs.regs_sys_cfg);
	val &= 0xfffffffe;
	writel(val, cedar_devp->iomap_addrs.regs_sys_cfg);
	//clear bootmode bit for give sram to ve
	val = readl((cedar_devp->iomap_addrs.regs_sys_cfg + 0x4));
	val &= 0xfeffffff;
	writel(val, (cedar_devp->iomap_addrs.regs_sys_cfg + 0x4));
}

static void set_system_register(void)
{
	//modify ccmu and sys_config register config
#if (defined CONFIG_ARCH_SUNIVW1P1 || defined CONFIG_ARCH_SUN3IW1P1)
	set_ccmu_by_self();
#endif
	set_sys_cfg_by_self();
}

static int deal_with_resouce(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *node = pdev->dev.of_node;

	/*4.register irq function*/
	cedar_devp->irq = irq_of_parse_and_map(node, 0);
	VE_LOGI("cedar-ve the get irq is %d\n", cedar_devp->irq);

	ret = request_irq(cedar_devp->irq, VideoEngineInterupt, 0, "cedar_dev", NULL);
	if (ret < 0) {
		VE_LOGW("request irq err\n");
		return -1;
	}

	memset(&cedar_devp->iomap_addrs, 0, sizeof(struct iomap_para));
	// map for macc io space
	cedar_devp->iomap_addrs.regs_ve = of_iomap(node, 0);
	if (!cedar_devp->iomap_addrs.regs_ve) {
		VE_LOGW("ve Can't map registers");
		return -1;
	}

	//map for sys_config io space
	cedar_devp->iomap_addrs.regs_sys_cfg = (char *)of_iomap(node, 1);
	if (!cedar_devp->iomap_addrs.regs_sys_cfg) {
		VE_LOGW("ve Can't map regs_sys_cfg registers, maybe no use");
	}

	//map for ccmu io space
	cedar_devp->iomap_addrs.regs_ccmu = (unsigned int *)of_iomap(node, 2);
	if (!cedar_devp->iomap_addrs.regs_ccmu) {
		VE_LOGW("ve Can't map regs_ccmu registers, maybe no use");
	}
#if VE_POWER_MANAGE_VALID
	cedar_devp->iomap_addrs.prcm_bass_vir = (unsigned int *)of_iomap(node, 3);
	if (!cedar_devp->iomap_addrs.prcm_bass_vir) {
		VE_LOGW("ve Can't map prcm_bass_vir registers");
	}
#endif

	//get clk
	cedar_devp->ve_clk = devm_clk_get(cedar_devp->plat_dev, "ve");
	if (IS_ERR(cedar_devp->ve_clk)) {
		VE_LOGW("try to get ve clk fail\n");
		return -1;
	}

	cedar_devp->bus_clk = devm_clk_get(cedar_devp->plat_dev, "bus_ve");
	if (IS_ERR(cedar_devp->bus_clk)) {
		VE_LOGW("try to get bus clk fail\n");
		return -1;
	}

	cedar_devp->mbus_clk = devm_clk_get(cedar_devp->plat_dev, "mbus_ve");
	if (IS_ERR(cedar_devp->mbus_clk)) {
		VE_LOGW("try to get mbus clk fail\n");
		return -1;
	}

	cedar_devp->parent_clk = clk_get_parent(cedar_devp->ve_clk);
	if (IS_ERR(cedar_devp->parent_clk)) {
		VE_LOGW("try to get parent clk fail\n");
		return -1;
	}
	//todo: you maybe need to change parent
	#if defined(CONFIG_ARCH_SUN8IW19P1)
	if (clk_set_parent(cedar_devp->ve_clk, cedar_devp->parent_clk)) {
		VE_LOGW("set ve clk and parent clk failed;\n");
		return -1;
	}
	#endif

	cedar_devp->reset = devm_reset_control_get(cedar_devp->plat_dev, NULL);
	if (IS_ERR(cedar_devp->reset)) {
		VE_LOGE("get reset fail\n");
		return -1;
	}

	set_system_register();

	return 0;
}

static const struct file_operations cedardev_fops = {
	.owner	 = THIS_MODULE,
	.mmap	 = cedardev_mmap,
	.open	 = cedardev_open,
	.release = cedardev_release,
	.llseek	 = no_llseek,
	.unlocked_ioctl	= compat_cedardev_ioctl,
	.compat_ioctl   = compat_cedardev_ioctl,
};


static int create_char_device(void)
{
	dev_t dev = 0;
	int ret = 0;
	int devno;

	//1.register or alloc the device number.
	if (g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);
		ret = register_chrdev_region(dev, 1, "cedar_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "cedar_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		VE_LOGE("cedar_dev: can't get major %d\n", g_dev_major);
		return ret;
	}

	//2.create char device
	devno = MKDEV(g_dev_major, g_dev_minor);
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	ret = cdev_add(&cedar_devp->cdev, devno, 1);
	if (ret) {
		VE_LOGE("Err:%d add cedar-dev fail\n", ret);
		goto region_del;
	}
	//3.create class and new device for auto device node
	cedar_devp->class = class_create(THIS_MODULE, "cedar_ve");
	if (IS_ERR_OR_NULL(cedar_devp->class)) {
		ret = -EINVAL;
		goto dev_del;
	}
	cedar_devp->dev	  = device_create(cedar_devp->class, NULL, devno, NULL, "cedar_dev");
	if (IS_ERR_OR_NULL(cedar_devp->dev)) {
		ret = -EINVAL;
		goto class_del;
	}
	return ret;

class_del:
	class_destroy(cedar_devp->class);
dev_del:
	cdev_del(&cedar_devp->cdev);
region_del:
	unregister_chrdev_region(dev, 1);

	return ret;
}

static ssize_t ve_info_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	int i = 0;
	int nTotalSize = 0;

	/* VE_LOGD("show: buf = %p\n",buf); */

	count += sprintf(buf + count, "********* ve proc info **********\n");

	mutex_lock(&cedar_devp->lock_debug_info);
	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (cedar_devp->debug_info[i].head_info.pid != 0
		&& cedar_devp->debug_info[i].head_info.tid != 0) {
			nTotalSize += cedar_devp->debug_info[i].head_info.length;

			#if 0
			VE_LOGD("cpy-show: i = %d, size = %d, total size = %d, data = %p, cout = %d, id = %d, %d\n",
					i,
					cedar_devp->debug_info[i].head_info.length,
					nTotalSize,
					cedar_devp->debug_info[i].data, (int)count,
					cedar_devp->debug_info[i].head_info.pid,
					cedar_devp->debug_info[i].head_info.tid);
			#endif

			if (nTotalSize > PAGE_SIZE)
				break;

			memcpy(buf + count, cedar_devp->debug_info[i].data,
					cedar_devp->debug_info[i].head_info.length);

			count += cedar_devp->debug_info[i].head_info.length;
		}

	}

	mutex_unlock(&cedar_devp->lock_debug_info);
	/*VE_LOGD("return count = %d\n",(int)count);*/
	return count;
}

/*sysfs create file, just support show*/
static DEVICE_ATTR(ve_info, 0664, ve_info_show, NULL);

static struct attribute *ve_attributes[] = {
	&dev_attr_ve_info.attr,
	NULL
};

static struct attribute_group ve_attribute_group = {
	.name = "attr",
	.attrs = ve_attributes
};

static int cedardev_init(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;

	VE_LOGD("install start!!!\n");

	(void)i;
	cedar_devp = kmalloc(sizeof(struct cedar_dev), GFP_KERNEL);
	if (cedar_devp == NULL) {
		VE_LOGW("malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	memset(cedar_devp, 0, sizeof(struct cedar_dev));

	//1.register cdev
	if (create_char_device() != 0) {
		ret = -EINVAL;
		goto free_devp;
	}
	//2.init some data
	spin_lock_init(&cedar_devp->lock);
	cedar_devp->plat_dev = &pdev->dev;
	sema_init(&cedar_devp->sem, 1);
	init_waitqueue_head(&cedar_devp->wq);

	timer_setup(&cedar_devp->cedar_engine_timer, cedar_engine_for_events, TIMER_DEFERRABLE);
	timer_setup(&cedar_devp->cedar_engine_timer_rel, cedar_engine_for_timer_rel, TIMER_DEFERRABLE);

	mutex_init(&cedar_devp->lock_vdec);
	mutex_init(&cedar_devp->lock_venc);
	mutex_init(&cedar_devp->lock_jdec);
	mutex_init(&cedar_devp->lock_00_reg);
	mutex_init(&cedar_devp->lock_04_reg);
	mutex_init(&cedar_devp->lock_mem);
	mutex_init(&cedar_devp->lock_debug_info);

	//3.config some register
	if (deal_with_resouce(pdev)) {
		ret = -EINVAL;
		goto free_devp;
	}

	//4.create sysfs file on new device
	if (sysfs_create_group(&cedar_devp->dev->kobj, &ve_attribute_group)) {
		VE_LOGW("sysfs create group failed, maybe ok!");
		ret = -EINVAL;
		goto free_devp;
	}

	//5.create debugfs file
#if IS_ENABLED(CONFIG_DEBUG_FS)
	ret = sunxi_ve_debug_register_driver();
	if (ret) {
		VE_LOGW("sunxi ve debug register fail");
		ret = -EINVAL;
		goto free_devp;
	}

	memset(&ve_debug_proc_info, 0, sizeof(struct ve_debugfs_buffer));
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++)
		ve_debug_proc_info.proc_buf[i] = NULL;

	ve_debug_proc_info.data =
		kmalloc(VE_DEBUGFS_BUF_SIZE*VE_DEBUGFS_MAX_CHANNEL, GFP_KERNEL);
	if (!ve_debug_proc_info.data) {
		VE_LOGE("kmalloc proc buffer failed!\n");
		ret = -ENOMEM;
		goto free_devp;
	}
	mutex_init(&ve_debug_proc_info.lock_proc);
	VE_LOGI("ve_debug_proc_info:%p, data:%p, lock:%p\n",
			&ve_debug_proc_info,
			ve_debug_proc_info.data,
			&ve_debug_proc_info.lock_proc);

#endif

	VE_LOGD("install end!!!\n");
	return 0;

free_devp:
	kfree(cedar_devp);
	return ret;
}

static void cedardev_exit(void)
{
	int i = 0;
	dev_t dev;

	dev = MKDEV(g_dev_major, g_dev_minor);

	free_irq(cedar_devp->irq, NULL);
	iounmap(cedar_devp->iomap_addrs.regs_ve);
	/* Destroy char device */

	cdev_del(&cedar_devp->cdev);
	device_destroy(cedar_devp->class, dev);
	class_destroy(cedar_devp->class);
	unregister_chrdev_region(dev, 1);

	//todo: power clk not support
#if defined CONFIG_ARCH_SUN9IW1P1
	regulator_put(regu);

	if (NULL == cedar_devp->power_clk || IS_ERR(cedar_devp->power_clk)) {
		VE_LOGW("cedar_devp->power_clk handle is invalid,just return!\n");
	} else {
		clk_disable_unprepare(cedar_devp->power_clk);
		clk_put(cedar_devp->power_clk);
		cedar_devp->power_clk = NULL;
	}
#endif

	//check whether free debug buffer
	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (cedar_devp->debug_info[i].data != NULL) {
			VE_LOGW("vfree : debug_info[%d].data = %p", i, cedar_devp->debug_info[i].data);
			vfree(cedar_devp->debug_info[i].data);
		}
	}

#if IS_ENABLED(CONFIG_DEBUG_FS)
	sunxi_ve_debug_unregister_driver();
	kfree(ve_debug_proc_info.data);
#endif

	kfree(cedar_devp);

	VE_LOGD("cedar-ve exit");
}

static int	sunxi_cedar_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	VE_LOGD("sunxi_cedar_remove");
	pdev->id = of_alias_get_id(np, "ve");
	if (pdev->id < 0) {
		VE_LOGD("failed to get alias ve id\n");
	} else if (pdev->id == 1) {
		VE_LOGI("device ve1 just return");
		return 0;
	}
	cedardev_exit();
	return 0;
}

static int	sunxi_cedar_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	pdev->id = of_alias_get_id(np, "ve");
	if (pdev->id < 0) {
		VE_LOGD("failed to get alias ve id\n");
	} else if (pdev->id == 1) {
		VE_LOGI("device ve1 just use to add iommu master");
		return 0;
	}
	cedardev_init(pdev);
	return 0;
}

static struct of_device_id sunxi_cedar_match[] = {
	{ .compatible = "allwinner,sunxi-cedar-ve",},
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_cedar_match);

static struct platform_driver sunxi_cedar_driver = {
	.probe		= sunxi_cedar_probe,
	.remove		= sunxi_cedar_remove,
	.suspend	= sunxi_cedar_suspend,
	.resume		= sunxi_cedar_resume,
	.driver		= {
		.name	= "sunxi-cedar",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_cedar_match,
	},
};

static int __init sunxi_cedar_init(void)
{
	/* we don't register device which is registered by device tree */
	/* platform_device_register(&sunxi_device_cedar);              */
	printk("sunxi cedar version 1.1\n");
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

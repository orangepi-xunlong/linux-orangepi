// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's VE driver
 *
 * Copyright (c) 2016, yangcaoyuan<yangcaoyuan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#define SUNXI_MODNAME	"google_vp9"
#include <sunxi-log.h>
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
#include <sunxi-clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "google_vp9.h"
#include "vp9_mem_list.h"
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/reset.h>


#define DRV_VERSION "1.0.0"

#ifndef VP9DEV_MAJOR
#define VP9DEV_MAJOR (160)
#endif
#ifndef VP9DEV_MINOR
#define VP9DEV_MINOR (0)
#endif

#define MACC_VP9_REGS_BASE      (0x01C0D000)

/* #define VP9_DEBUG */
#define VE_LOGK(fmt, arg...) pr_info(fmt, ##arg)
#define VE_LOGV(fmt, arg...)
#define VE_LOGD(fmt, arg...) sunxi_debug(NULL, fmt, ##arg)
#define VE_LOGI(fmt, arg...) sunxi_info(NULL,  fmt, ##arg)
#define VE_LOGW(fmt, arg...) sunxi_warn(NULL,  fmt, ##arg)
#define VE_LOGE(fmt, arg...) sunxi_err(NULL,   fmt, ##arg)

#define GOOGLE_VP9_CLK_HIGH_WATER  (700)
#define GOOGLE_VP9_CLK_LOW_WATER   (100)

#define PRINTK_IOMMU_ADDR 0
#define SET_CCMU_BY_SHELF 0
static int vp9_dev_major = VP9DEV_MAJOR;
static int vp9_dev_minor = VP9DEV_MINOR;
module_param(vp9_dev_major, int, 0444);
module_param(vp9_dev_minor, int, 0444);


struct iomap_para {
	char *regs_macc;
	char *regs_ccmu;
};

struct user_iommu_param {
	int				fd;
	unsigned int	iommu_addr;
};

struct cedarv_iommu_buffer {
	struct aw_mem_list_head i_list;
	int				fd;
	unsigned long	iommu_addr;
	struct dma_buf  *dma_buf;
	struct dma_buf_attachment *attachment;
	struct sg_table	*sgt;
	int    p_id;
};

static DECLARE_WAIT_QUEUE_HEAD(wait_vp9);
struct googlevp9_dev {
	struct cdev cdev;	             /* char device struct                 */
	struct device *dev;              /* ptr to class device struct         */
	struct device *platform_dev;	 /* ptr to class device struct		   */
	struct class  *class;            /* class for auto create device node  */

	struct semaphore sem;            /* mutual exclusion semaphore         */

	wait_queue_head_t wq;            /* wait queue for poll ops            */

	struct iomap_para iomap_addrs;   /* io remap addrs                     */

	struct timer_list vp9_engine_timer;
	struct timer_list vp9_engine_timer_rel;

	u32 irq;                         /* cedar video engine irq number      */
	u32 de_irq_flag;                    /* flag of video decoder engine irq generated */
	u32 de_irq_value;                   /* value of video decoder engine irq          */
	u32 irq_has_enable;
	u32 ref_count;

	unsigned int *sram_bass_vir;
	unsigned int *clk_bass_vir;

	struct aw_mem_list_head    list;        /* buffer list */
	struct mutex lock_mem;
	unsigned char bMemDevAttachFlag;
	struct clk  *av1_clk;
	struct clk  *bus_av1_clk;
	struct clk  *bus_ve_clk;
	struct clk  *mbus_av1_clk;
	struct reset_control *reset;
	struct reset_control *reset_ve;
};

struct google_vp9_info {
	unsigned int set_vol_flag;
};

struct googlevp9_dev *google_vp9_devp;


static irqreturn_t GoogleVp9Interrupt(int irq, void *dev)
{
	char *av1_reg_2 = NULL;
	unsigned int status = 0;
	unsigned int interrupt_enable = 0;
	struct iomap_para addrs = google_vp9_devp->iomap_addrs;
	/* 1. check and get the interrupt enable bits */
	/* 2. check and get the interrupt status bits */
	/* 3. clear the interrupt enable bits */
	/* 4. set the irq_value and irq_flag */
	/* 5. wake up the user mode interrupt_func */
	av1_reg_2 = (addrs.regs_macc + 0x08);
	status = readl((void *)av1_reg_2);
	interrupt_enable = status & 0x0800;

	/* only check status[bit:18,16,13,12,11,8], enable[bit:4] */
	if ((status & 0x7c) && (interrupt_enable)) {
		/* need check we must clear the interrupt enable bits or not? */
		if (status & 0x10) {
			google_vp9_devp->de_irq_value = 1;
			google_vp9_devp->de_irq_flag = 1;
		}
		writel(status & 0xfffff7ff, (void *)av1_reg_2);
		wake_up_interruptible(&wait_vp9);
	}

	return IRQ_HANDLED;
}

static int clk_status;

static spinlock_t google_vp9_spin_lock;

int enable_google_vp9_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&google_vp9_spin_lock, flags);

	if (clk_status == 1) {
		res = 0 ;
		goto out;
	}
	clk_status = 1;

	reset_control_deassert(google_vp9_devp->reset_ve);
	reset_control_deassert(google_vp9_devp->reset);

	if (clk_prepare_enable(google_vp9_devp->bus_ve_clk)) {
		VE_LOGE("enable bus ve clk gating failed;\n");
		goto out;
	}

	if (clk_prepare_enable(google_vp9_devp->bus_av1_clk)) {
		VE_LOGE("enable bus clk gating failed;\n");
		goto out;
	}

	if (clk_prepare_enable(google_vp9_devp->mbus_av1_clk)) {
		VE_LOGE("enable mbus clk gating failed;\n");
		goto out;
	}

	if (clk_prepare_enable(google_vp9_devp->av1_clk)) {
		VE_LOGE("enable ve clk gating failed;\n");
		goto out;
	}
	res = 0;
	AW_MEM_INIT_LIST_HEAD(&google_vp9_devp->list);
	VE_LOGD("vp9 clk enable!\n");
out:
	spin_unlock_irqrestore(&google_vp9_spin_lock, flags);
	return res;
}

int disable_google_vp9_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;
	struct aw_mem_list_head *pos, *q;

	spin_lock_irqsave(&google_vp9_spin_lock, flags);

	if (clk_status == 0) {
		res = 0;
		goto out;
	}
	clk_status = 0;

	clk_disable_unprepare(google_vp9_devp->av1_clk);
	clk_disable_unprepare(google_vp9_devp->mbus_av1_clk);
	clk_disable_unprepare(google_vp9_devp->bus_av1_clk);
	clk_disable_unprepare(google_vp9_devp->bus_ve_clk);
	reset_control_assert(google_vp9_devp->reset);
	reset_control_assert(google_vp9_devp->reset_ve);

	aw_mem_list_for_each_safe(pos, q, &google_vp9_devp->list) {
		struct cedarv_iommu_buffer *tmp;

		tmp = aw_mem_list_entry(pos, struct cedarv_iommu_buffer, i_list);
		aw_mem_list_del(pos);
		kfree(tmp);
	}
	res = 0;
	VE_LOGD("vp9 clk disable!\n");

out:
	spin_unlock_irqrestore(&google_vp9_spin_lock, flags);
	return res;
}

static long compat_googlevp9dev_ioctl(struct file *filp,
		unsigned int cmd,
		unsigned long arg)
{
	long  ret = 0;
	int ve_timeout = 0;
	unsigned long flags;
	struct google_vp9_info *info;

	info = filp->private_data;
	switch (cmd) {
	case VP9_IOCTL_ENGINE_REQ:
		google_vp9_devp->ref_count++;
		if (google_vp9_devp->ref_count == 1)
			enable_google_vp9_hw_clk();
		break;
	case VP9_IOCTL_ENGINE_REL:
		google_vp9_devp->ref_count--;
		if (google_vp9_devp->ref_count == 0) {
			ret = disable_google_vp9_hw_clk();
			if (ret < 0) {
				VE_LOGW("IOCTL_ENGINE_REL clk disable error!\n");
				return -EFAULT;
			}
		}
		return ret;
	case VP9_IOCTL_WAIT_INTERRUPT:
		ve_timeout = (int)arg;
		google_vp9_devp->de_irq_value = 0;

		spin_lock_irqsave(&google_vp9_spin_lock, flags);
		if (google_vp9_devp->de_irq_flag)
			google_vp9_devp->de_irq_value = 1;
		spin_unlock_irqrestore(&google_vp9_spin_lock, flags);

		wait_event_interruptible_timeout(wait_vp9,
			google_vp9_devp->de_irq_flag, ve_timeout*HZ);
		google_vp9_devp->de_irq_flag = 0;

		return google_vp9_devp->de_irq_value;

	case VP9_IOCTL_RESET:
		VE_LOGD("vp9 reset!\n");
		reset_control_reset(google_vp9_devp->reset);
		break;

	case VP9_IOCTL_SET_FREQ:
	{
		int arg_rate = (int)arg;

		if (arg_rate >= GOOGLE_VP9_CLK_LOW_WATER &&
		    arg_rate <= GOOGLE_VP9_CLK_HIGH_WATER &&
		    clk_get_rate(google_vp9_devp->av1_clk) / 1000000 != arg_rate) {
			if (clk_set_rate(google_vp9_devp->av1_clk, arg_rate * 1000000)) {
				VE_LOGW("set pll_vp9_parent clock failed\n");
			}
		}
		ret = clk_get_rate(google_vp9_devp->av1_clk);
		VE_LOGD("real_freq:%ld\n", ret);
		break;
	}

	case VP9_IOCTL_GET_ENV_INFO:
		{
			struct vp9_env_information_compat env_info;

			env_info.phymem_start = 0;
			env_info.phymem_total_size = 0;
			env_info.address_macc = 0;
			if (copy_to_user((char *)arg, &env_info,
				sizeof(env_info)))
				return -EFAULT;
		}
		break;

	case IOCTL_GET_IOMMU_ADDR:
	{
		struct sg_table *sgt;
		struct user_iommu_param sUserIommuParam;
		struct cedarv_iommu_buffer *pVeIommuBuf = NULL;

		google_vp9_devp->bMemDevAttachFlag = 1;

		pVeIommuBuf = kmalloc(sizeof(*pVeIommuBuf), GFP_KERNEL);
		if (pVeIommuBuf == NULL)
			return -ENOMEM;

		if (copy_from_user(&sUserIommuParam, (void __user *)arg,
			sizeof(sUserIommuParam))) {
			VE_LOGE("IOCTL_GET_IOMMU_ADDR copy_from_user erro\n");
			return -EFAULT;
		}

		pVeIommuBuf->fd = sUserIommuParam.fd;

		pVeIommuBuf->dma_buf = dma_buf_get(pVeIommuBuf->fd);
		if (pVeIommuBuf->dma_buf < 0) {
			VE_LOGE("vp9 get dma_buf error\n");
			return -EFAULT;
		}

		pVeIommuBuf->attachment = dma_buf_attach(pVeIommuBuf->dma_buf,
							 google_vp9_devp->platform_dev);
		if (pVeIommuBuf->attachment < 0) {
			VE_LOGE("vp9 get dma_buf_attachment error\n");
			goto RELEASE_DMA_BUF;
		}

		sgt = dma_buf_map_attachment(pVeIommuBuf->attachment, DMA_BIDIRECTIONAL);
		pVeIommuBuf->sgt = sgt;

		pVeIommuBuf->iommu_addr = sg_dma_address(pVeIommuBuf->sgt->sgl);
		sUserIommuParam.iommu_addr = (unsigned int)(pVeIommuBuf->iommu_addr & 0xffffffff);


		if (copy_to_user((void __user *)arg, &sUserIommuParam, sizeof(sUserIommuParam))) {
			VE_LOGE("vp9 get iommu copy_to_user error\n");
			goto RELEASE_DMA_BUF;
		}

		pVeIommuBuf->p_id = current->tgid;
		#if PRINTK_IOMMU_ADDR
		VE_LOGD("fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, " \
			"sg_table:%p, nents:%d, p_id:%d\n",
			pVeIommuBuf->fd,
			pVeIommuBuf->iommu_addr,
			pVeIommuBuf->dma_buf,
			pVeIommuBuf->attachment,
			pVeIommuBuf->sgt,
			pVeIommuBuf->sgt->nents,
			pVeIommuBuf->p_id);
		#endif

		mutex_lock(&google_vp9_devp->lock_mem);
		aw_mem_list_add_tail(&pVeIommuBuf->i_list, &google_vp9_devp->list);
		mutex_unlock(&google_vp9_devp->lock_mem);
		break;

RELEASE_DMA_BUF:
			if (pVeIommuBuf->dma_buf > 0) {
				if (pVeIommuBuf->attachment > 0) {
					if (pVeIommuBuf->sgt > 0) {
						dma_buf_unmap_attachment(pVeIommuBuf->attachment,
									 pVeIommuBuf->sgt,
									 DMA_BIDIRECTIONAL);
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
			sizeof(sUserIommuParam))) {
			VE_LOGE("IOCTL_FREE_IOMMU_ADDR copy_from_user error\n");
			return -EFAULT;
		}
		aw_mem_list_for_each_entry(pVeIommuBuf, &google_vp9_devp->list, i_list) {
			if (pVeIommuBuf->fd == sUserIommuParam.fd &&
				pVeIommuBuf->p_id == current->tgid) {
				#if PRINTK_IOMMU_ADDR
				VE_LOGD("free: fd:%d, iommu_addr:%lx, dma_buf:%p, " \
					"dma_buf_attach:%p, sg_table:%p, nets:%d, p_id:%d\n",
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
							dma_buf_unmap_attachment(pVeIommuBuf->attachment,
										 pVeIommuBuf->sgt,
										 DMA_BIDIRECTIONAL);
						}

						dma_buf_detach(pVeIommuBuf->dma_buf, pVeIommuBuf->attachment);
					}

					dma_buf_put(pVeIommuBuf->dma_buf);
				}

				mutex_lock(&google_vp9_devp->lock_mem);
				aw_mem_list_del(&pVeIommuBuf->i_list);
				kfree(pVeIommuBuf);
				mutex_unlock(&google_vp9_devp->lock_mem);
				break;
			}
		}
		break;
	}
	default:
		return -1;
	}
	return ret;
}

static int googlevp9dev_open(struct inode *inode, struct file *filp)
{
	struct google_vp9_info *info;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->set_vol_flag = 0;

	filp->private_data = info;
	if (down_interruptible(&google_vp9_devp->sem))
		return -ERESTARTSYS;

	if (google_vp9_devp->ref_count == 0)
		google_vp9_devp->de_irq_flag = 0;

	up(&google_vp9_devp->sem);
	nonseekable_open(inode, filp);
	return 0;
}

static int googlevp9dev_release(struct inode *inode, struct file *filp)
{
	struct google_vp9_info *info;
	struct aw_mem_list_head *pos, *q;
	struct cedarv_iommu_buffer *pVeIommuBuf;

	info = filp->private_data;
	if (google_vp9_devp->bMemDevAttachFlag) {
		aw_mem_list_for_each_safe(pos, q, &google_vp9_devp->list) {
			pVeIommuBuf = aw_mem_list_entry(pos, struct cedarv_iommu_buffer, i_list);
			if (pVeIommuBuf->p_id == current->tgid) {
				#if PRINTK_IOMMU_ADDR
				VE_LOGD("free: fd:%d, iommu_addr:%lx, dma_buf:%p, " \
					"dma_buf_attach:%p, sg_table:%p, nets:%d, p_id:%d\n",
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
							dma_buf_unmap_attachment(pVeIommuBuf->attachment,
										 pVeIommuBuf->sgt,
										 DMA_BIDIRECTIONAL);
						}
						dma_buf_detach(pVeIommuBuf->dma_buf,
							       pVeIommuBuf->attachment);
					}
					dma_buf_put(pVeIommuBuf->dma_buf);
				}
				mutex_lock(&google_vp9_devp->lock_mem);
				aw_mem_list_del(&pVeIommuBuf->i_list);
				kfree(pVeIommuBuf);
				mutex_unlock(&google_vp9_devp->lock_mem);
			}
		}
	}

	if (down_interruptible(&google_vp9_devp->sem))
		return -ERESTARTSYS;

	/* release other resource here */
	if (google_vp9_devp->ref_count == 0)
		google_vp9_devp->de_irq_flag = 1;

	up(&google_vp9_devp->sem);

	kfree(info);
	return 0;
}

static void googlevp9dev_vma_open(struct vm_area_struct *vma)
{
}

static void googlevp9dev_vma_close(struct vm_area_struct *vma)
{
}

static struct vm_operations_struct cedardev_remap_vm_ops = {
	.open  = googlevp9dev_vma_open,
	.close = googlevp9dev_vma_close,
};

static int googlevp9dev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long temp_pfn;

	if (vma->vm_end - vma->vm_start == 0) {
		VE_LOGW("vma->vm_end is equal vma->vm_start : %lx\n", vma->vm_start);
		return 0;
	}
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) {
		VE_LOGW("the vma->vm_pgoff is %lx,it is large than the largest page number\n",
			vma->vm_pgoff);
		return -EINVAL;
	}


	temp_pfn = MACC_VP9_REGS_BASE >> 12;


	/* Set reserved and I/O flag for the area. */
	vma->vm_flags |= /* VM_RESERVED | */VM_IO;
	/* Select uncached access. */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}


	vma->vm_ops = &cedardev_remap_vm_ops;
	googlevp9dev_vma_open(vma);

	return 0;
}


static int snd_sw_google_vp9_suspend(struct platform_device *pdev,
										pm_message_t state)
{
	int ret = 0;

	VE_LOGW("standby suspend\n");
	ret = disable_google_vp9_hw_clk();

	if (ret < 0) {
		VE_LOGW("google_vp9 clk disable somewhere error!\n");
		return -EFAULT;
	}

	return 0;
}

static int snd_sw_google_vp9_resume(struct platform_device *pdev)
{
	int ret = 0;

	VE_LOGW("standby resume\n");

	if (google_vp9_devp->ref_count == 0)
		return 0;

	ret = enable_google_vp9_hw_clk();
	if (ret < 0) {
		VE_LOGW("google_vp9 clk enable somewhere error!\n");
		return -EFAULT;
	}
	return 0;
}


static const struct file_operations googlevp9dev_fops = {
	.owner   = THIS_MODULE,
	.mmap    = googlevp9dev_mmap,
	.open    = googlevp9dev_open,
	.release = googlevp9dev_release,
	.llseek  = no_llseek,
	.unlocked_ioctl   = compat_googlevp9dev_ioctl,
	.compat_ioctl     = compat_googlevp9dev_ioctl,

};

static int create_char_device(void)
{
	dev_t dev = 0;
	int ret = 0;
	int devno;

	/* 1.register or alloc the device number. */
	if (vp9_dev_major) {
		dev = MKDEV(vp9_dev_major, vp9_dev_minor);
		ret = register_chrdev_region(dev, 1, "googlevp9_dev");
	} else {
		ret = alloc_chrdev_region(&dev, vp9_dev_major, 1, "googlevp9_dev");
		vp9_dev_major = MAJOR(dev);
		vp9_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		VE_LOGW("cedar_dev: can't get major %d\n", vp9_dev_major);
		return ret;
	}

	/* 2.create char device */
	devno = MKDEV(vp9_dev_major, vp9_dev_minor);
	cdev_init(&google_vp9_devp->cdev, &googlevp9dev_fops);
	google_vp9_devp->cdev.owner = THIS_MODULE;
	ret = cdev_add(&google_vp9_devp->cdev, devno, 1);
	if (ret) {
		VE_LOGW("Err:%d add cedar-dev fail\n", ret);
		goto region_del;
	}
	/* 3.create class and new device for auto device node */
	google_vp9_devp->class = class_create(THIS_MODULE, "cedar_av1");
	if (IS_ERR_OR_NULL(google_vp9_devp->class)) {
		ret = -EINVAL;
		goto dev_del;
	}
	google_vp9_devp->dev = device_create(google_vp9_devp->class, NULL, devno, NULL, "cedar_av1");
	if (IS_ERR_OR_NULL(google_vp9_devp->dev)) {
		ret = -EINVAL;
		goto class_del;
	}
	return ret;

class_del:
	class_destroy(google_vp9_devp->class);
dev_del:
	cdev_del(&google_vp9_devp->cdev);
region_del:
	unregister_chrdev_region(dev, 1);

	return ret;
}

#if SET_CCMU_BY_SHELF
static int set_ccmu_by_shelf(char *ccmu_base)
{
	unsigned int v;
	/*
	 * reg = readl(ccmu_base + 0x58);
	 * reg &= 0x7ffa0000;
	 *
	 * fq = fq/6 - 1;
	 *
	 * reg = reg & (~(1 << 29));
	 * writel(reg,ccmu_base + 0x58);
	 *
	 * reg = readl(ccmu_base + 0x58);
	 * reg = reg | (fq<<8) | (1<<1) | (1<<0);
	 * writel(reg,ccmu_base + 0x58);
	 *
	 * reg = readl(ccmu_base + 0x58);
	 * reg = reg | (1<<31);
	 * writel(reg,ccmu_base + 0x58);
	 *
	 * reg = readl(ccmu_base + 0x58);
	 * reg = reg | (1<<29);
	 * writel(reg,ccmu_base + 0x58);
	 *
	 * count = 0;
	 * do {
	 *	mdelay(5);
	 *	reg = readl(ccmu_base + 0x58);
	 *	reg = reg & (1<<28);
	 *	count++;
	 * } while(reg == 0 && count < 10);
	 *
	 * if (count >= 10 && reg == 0) {
	 *    VE_LOGW("Err: clock is unlock\n");
	 *	return -1;
	 * }
	 */
	v = readl(ccmu_base + 0x690);
	v |= (1U<<31);
	writel(v, ccmu_base + 0x690);

	v = readl(ccmu_base + 0x69c);
	v |= (1<<0);
	writel(v, ccmu_base + 0x69c);

	v = readl(ccmu_base + 0x804);
	v |= (1U<<1);
	writel(v, ccmu_base + 0x804);

	v = readl(ccmu_base + 0x69c);
	v &= ~(1<<16);
	writel(v, ccmu_base + 0x69c);

	v = readl(ccmu_base + 0x69c);
	v |= (1<<16);
	writel(v, ccmu_base + 0x69c);
	return 0;
}
#endif

static int googleVp9dev_init(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *node;
	dev_t dev;

	dev = 0;

	VE_LOGW("[google av1]: install start!!!\n");

	google_vp9_devp = kmalloc(sizeof(*google_vp9_devp), GFP_KERNEL);
	if (google_vp9_devp == NULL) {
		VE_LOGW("malloc mem for google vp9 device err\n");
		return -ENOMEM;
	}

	memset(google_vp9_devp, 0, sizeof(*google_vp9_devp));
	node = pdev->dev.of_node;
	google_vp9_devp->platform_dev = &pdev->dev;

	/* register or alloc the device number. */
	if (create_char_device() != 0) {
		ret = -EINVAL;
		goto free_devp;
	}

	google_vp9_devp->irq = irq_of_parse_and_map(node, 0);
	VE_LOGI("google vp9: the get irq is %d\n", google_vp9_devp->irq);
	if (google_vp9_devp->irq <= 0) {
		VE_LOGW("Can't parse IRQ, use default!!!\n");
		ret = -EINVAL;
		goto free_devp;
	}

	spin_lock_init(&google_vp9_spin_lock);

	sema_init(&google_vp9_devp->sem, 1);
	init_waitqueue_head(&google_vp9_devp->wq);

	mutex_init(&google_vp9_devp->lock_mem);

	memset(&google_vp9_devp->iomap_addrs, 0, sizeof(google_vp9_devp->iomap_addrs));
	ret = request_irq(google_vp9_devp->irq, GoogleVp9Interrupt, 0, "googlevp9_dev", NULL);
	if (ret < 0) {
		VE_LOGW("request irq err\n");
		ret = -EINVAL;
		goto free_devp;
	}

	/* map for macc io space */
	google_vp9_devp->iomap_addrs.regs_macc = of_iomap(node, 0);
	if (!google_vp9_devp->iomap_addrs.regs_macc) {
		VE_LOGW("vp9 Can't map registers\n");
		ret = -EINVAL;
		goto free_devp;
	}
	/* map for ccmu io space */
	google_vp9_devp->iomap_addrs.regs_ccmu = of_iomap(node, 1);
	if (!google_vp9_devp->iomap_addrs.regs_ccmu) {
		VE_LOGW("vp9 Can't map ccmu registers\n");
		ret = -EINVAL;
		goto free_devp;
	}

	/*
	 * if (set_ccmu_by_shelf(google_vp9_devp->iomap_addrs.regs_ccmu) == 0) {
	 * 	VE_LOGW("now read reg0:%x\n", readl(google_vp9_devp->iomap_addrs.regs_macc));
	 * 	writel(0x2342, google_vp9_devp->iomap_addrs.regs_macc + 0x130);
	 * 	VE_LOGW("ji***read24:%x\n", readl(google_vp9_devp->iomap_addrs.regs_macc + 0x130));
	 * } else {
	 * 	ret = -EINVAL;
	 * 	goto free_devp;
	 * }
	 */
	/* get clock */
	google_vp9_devp->av1_clk = devm_clk_get(google_vp9_devp->platform_dev, "av1");
	if (IS_ERR(google_vp9_devp->av1_clk)) {
		VE_LOGW("try to get ve clk fail\n");
		ret = -EINVAL;
		goto free_devp;
	}

	google_vp9_devp->bus_av1_clk = devm_clk_get(google_vp9_devp->platform_dev, "bus_av1");
	if (IS_ERR(google_vp9_devp->bus_av1_clk)) {
		VE_LOGW("try to get bus av1 clk fail\n");
		ret = -EINVAL;
		goto free_devp;
	}

	google_vp9_devp->bus_ve_clk = devm_clk_get(google_vp9_devp->platform_dev, "bus_ve");
	if (IS_ERR(google_vp9_devp->bus_ve_clk)) {
		VE_LOGW("try to get bus ve clk fail\n");
		ret = -EINVAL;
		goto free_devp;
	}

	google_vp9_devp->mbus_av1_clk = devm_clk_get(google_vp9_devp->platform_dev, "mbus_av1");
	if (IS_ERR(google_vp9_devp->mbus_av1_clk)) {
		VE_LOGW("try to get mbus clk fail\n");
		ret = -EINVAL;
		goto free_devp;
	}

	google_vp9_devp->reset = devm_reset_control_get(google_vp9_devp->platform_dev, "reset_av1");
	if (IS_ERR(google_vp9_devp->reset)) {
		VE_LOGW("get reset fail\n");
		ret = -EINVAL;
		goto free_devp;
	}
	google_vp9_devp->reset_ve = devm_reset_control_get_shared(google_vp9_devp->platform_dev, "reset_ve");
	if (IS_ERR(google_vp9_devp->reset_ve)) {
		VE_LOGW("get reset all fail\n");
		ret = -EINVAL;
		goto free_devp;
	}

	VE_LOGW("[google av1]: install end!!!\n");
	return 0;
free_devp:
	kfree(google_vp9_devp);
	return ret;
}


static void googleVp9dev_exit(void)
{
	dev_t dev;

	dev = MKDEV(vp9_dev_major, vp9_dev_minor);

	free_irq(google_vp9_devp->irq, NULL);
	iounmap(google_vp9_devp->iomap_addrs.regs_macc);
	/* Destroy char device */

	cdev_del(&google_vp9_devp->cdev);
	device_destroy(google_vp9_devp->class, dev);
	class_destroy(google_vp9_devp->class);
	unregister_chrdev_region(dev, 1);
	kfree(google_vp9_devp);
}

static int  sunxi_google_vp9_remove(struct platform_device *pdev)
{
	googleVp9dev_exit();
	return 0;
}

static int  sunxi_google_vp9_probe(struct platform_device *pdev)
{
	googleVp9dev_init(pdev);
	return 0;
}

static const struct of_device_id sunxi_google_vp9_match[] = {
	{ .compatible = "allwinner,sunxi-cedar-av1",},
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_google_vp9_match);

static struct platform_driver sunxi_google_vp9_driver = {
	.probe		= sunxi_google_vp9_probe,
	.remove		= sunxi_google_vp9_remove,
	.suspend	= snd_sw_google_vp9_suspend,
	.resume		= snd_sw_google_vp9_resume,
	.driver		= {
		.name	= "sunxi-av1",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_google_vp9_match,
	},
};

static int __init sunxi_google_vp9_init(void)
{
	VE_LOGD("sunxi google vp9 version 1.0\n");
	return platform_driver_register(&sunxi_google_vp9_driver);
}

static void __exit sunxi_google_vp9_exit(void)
{
	platform_driver_unregister(&sunxi_google_vp9_driver);
}

module_init(sunxi_google_vp9_init);
module_exit(sunxi_google_vp9_exit);


MODULE_AUTHOR("jilinglin");
MODULE_DESCRIPTION("User mode GOOGLE VP9/AV1 device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:cedarx-sunxi");

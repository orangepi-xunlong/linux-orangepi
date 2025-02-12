// SPDX-License-Identifier: GPL-2.0
/*
* V2D driver for Ky
* Copyright (C) 2023 Ky Co., Ltd.
*
*/

#include "v2d_priv.h"
#include "v2d_drv.h"
#include "v2d_reg.h"
#include <linux/clk-provider.h>
#include <linux/dma-fence.h>
#include <linux/sync_file.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <uapi/linux/sched/types.h>
#include <linux/dma-buf.h>
#include <linux/highmem.h>

#define  V2D_DRV_NAME		"ky_v2d"
struct v2d_info *v2dInfo;

#ifdef CONFIG_KY_DEBUG
static bool check_v2d_running_status(struct v2d_info *pV2dInfo)
{
	return pV2dInfo->b_v2d_running;
}
#define to_devinfo(_nb) container_of(_nb, struct v2d_info, nb)
static int v2d_clkoffdet_notifier_handler(struct notifier_block *nb,
					  unsigned long msg, void *data)
{
	struct clk_notifier_data *cnd = data;
	struct v2d_info *pV2dInfo = to_devinfo(nb);
	if ((__clk_is_enabled(cnd->clk)) && (msg & PRE_RATE_CHANGE) &&
	    (cnd->new_rate == 0) && (cnd->old_rate != 0)) {
		if (pV2dInfo->is_v2d_running(pV2dInfo))
			return NOTIFY_BAD;
	}
	return NOTIFY_OK;
}
#endif

static void v2d_clk_on(struct v2d_info *info)
{
	clk_prepare_enable(info->clkcore);
	clk_prepare_enable(info->clkio);
#ifdef CONFIG_KY_DEBUG
	info->b_v2d_running = true;
#endif
}

static void v2d_clk_off(struct v2d_info *info)
{
#ifdef CONFIG_KY_DEBUG
	info->b_v2d_running = false;
#endif
	if (__clk_is_enabled(info->clkio)) {
		clk_disable_unprepare(info->clkio);
	}
	if (__clk_is_enabled(info->clkcore)) {
		clk_disable_unprepare(info->clkcore);
	}
}

static ssize_t v2d_sysfs_clkrate_get(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct v2d_info *info  = dev_get_drvdata(dev);
	long rate = 0;
	rate = clk_get_rate(info->clkcore);
	return scnprintf(buf, PAGE_SIZE, "%d\n", (int)rate);
}

static ssize_t v2d_sysfs_clkrate_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct v2d_info *info  = dev_get_drvdata(dev);
	long rate = 0;
	rate = simple_strtol(buf, NULL, 10);
	if (0 != rate)
	{
		clk_set_rate(info->clkcore, rate);
	}
	return count;
}

static struct device_attribute v2d_sysfs_files[] = {
	__ATTR(clkrate, S_IRUGO | S_IWUSR, v2d_sysfs_clkrate_get, v2d_sysfs_clkrate_set),
};

static int v2d_suspend(struct device *dev)
{
	return 0;
}

static int v2d_resume(struct device *dev)
{
	return 0;
}

static int v2d_runtime_suspend(struct device *dev)
{
	struct v2d_info *info  = dev_get_drvdata(dev);
	if (!IS_ERR_OR_NULL(info->clkcore))
	{
		clk_disable_unprepare(info->clkcore);
		V2DLOGI("v2d: clock off.\n");
	}
	return 0;
}

static int v2d_runtime_resume(struct device *dev)
{
	struct v2d_info *info  = dev_get_drvdata(dev);
	long clk_rate = 0;

	if (!IS_ERR_OR_NULL(info->clkcore))
	{
			clk_prepare_enable(info->clkcore);
			clk_rate = clk_get_rate(info->clkcore);
			V2DLOGI("v2d: clock on, rate: %ld\n", clk_rate);
	}
	return 0;
}

static const struct dev_pm_ops v2d_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(v2d_suspend, v2d_resume)
	.runtime_suspend = v2d_runtime_suspend,
	.runtime_resume = v2d_runtime_resume,
};

static irqreturn_t v2d_irq_handler(int32_t irq, void *dev_id)
{
	unsigned long flags =  0;
	uint32_t irqstatus = 0;
	uint32_t irqerr = 0;

	struct v2d_info *info = (struct v2d_info *)dev_id;

	if (!info) {
		V2DLOGE("v2d info is NULL!\n");
		return IRQ_NONE;
	}

	spin_lock_irqsave(&info->power_spinlock, flags);
	if (!info->refcount)
	{
		spin_unlock_irqrestore(&info->power_spinlock, flags);
		V2DLOGE("v2d power is off !\n");
		return IRQ_NONE;
	}
	iommu_irq_reset();
	irqstatus = v2d_irq_status();
	irqerr = v2d_irqerr_status();
	v2d_irqerr_clear(irqerr);
	v2d_irq_clear(irqstatus);
	if (irqerr){
		V2DLOGE("%s irq %d irq_status 0x%x,irqerr 0x%x\n", __func__, irq, irqstatus, irqerr);
		info->do_reset = 1;
		queue_work(info->v2d_job_done_wq, &info->work);
	} else if ((irqstatus == V2D_EOF_IRQ_STATUS) || (irqstatus == (V2D_EOF_IRQ_STATUS|V2D_FBCENC_IRQ_STATUS))) {
		queue_work(info->v2d_job_done_wq, &info->work);
	}
	spin_unlock_irqrestore(&info->power_spinlock, flags);
	return IRQ_HANDLED;
}


static DEFINE_SPINLOCK(v2d_fence_lock);
static const char *v2d_fence_get_driver_name(struct dma_fence *fence)
{
	return "v2d";
}

static const char *v2d_fence_get_timeline_name(struct dma_fence *fence)
{
	return "v2d.timeline";
}

static bool v2d_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static void v2d_fence_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	snprintf(str, size, "%llu", fence->seqno);
}

const struct dma_fence_ops v2d_fence_ops = {
	.wait = dma_fence_default_wait,
	.get_driver_name = v2d_fence_get_driver_name,
	.get_timeline_name = v2d_fence_get_timeline_name,
	.enable_signaling = v2d_fence_enable_signaling,
	.fence_value_str = v2d_fence_fence_value_str
};

int v2d_fence_generate(struct v2d_info *info, struct dma_fence **fence, int *fence_fd)
{
	struct sync_file *sync_file = NULL;
	int fd;

	struct dma_fence *dmaFence;
	dmaFence = kzalloc(sizeof(*dmaFence), GFP_KERNEL);
	if (!dmaFence)
		return -ENOMEM;

	dma_fence_init(dmaFence, &v2d_fence_ops, &v2d_fence_lock, info->context, atomic_inc_return(&info->seqno));
	*fence = dmaFence;
	/* create a sync_file fd representing the fence */
	#ifdef CONFIG_SYNC_FILE
	sync_file = sync_file_create(*fence);
	#endif
	if (!sync_file) {
		dma_fence_put(*fence);
		return -ENOMEM;
	}
	fd = get_unused_fd_flags(O_CLOEXEC);
	*fence_fd = fd;
	if(fd<0)
	{
		dma_fence_put(*fence);
		fput(sync_file->file);
		return -ENOMEM;
	}
	fd_install(fd, sync_file->file);
	return 0;
}

void v2d_fence_wait(struct v2d_info *info, struct dma_fence *fence)
{
	int err = dma_fence_wait_timeout(fence, false, msecs_to_jiffies(V2D_SHORT_FENCE_TIMEOUT));
	if (err > 0)
		return;

	if (err == 0)
		err = dma_fence_wait_timeout(fence, false, msecs_to_jiffies(V2D_LONG_FENCE_TIMEOUT));

	if (err <= 0)
		dev_warn(&info->pdev->dev, "error waiting on fence: %d\n", err);
}

void kfree_v2d_post_task(struct v2d_pending_post_task *element)
{
	if (!element)
	{
		return;
	}
	if (!element->pTask)
	{
		kfree(element);
		return;
	}
	kfree(element->pTask);
	kfree(element);
}

#define V2D_TBU_BASE_VA (0x80000000)
#define V2D_TBU_VA_STEP (0x2000000)
static int v2d_get_dmabuf(struct v2d_info *v2dinfo, struct v2d_pending_post_task *cfg)
{
	V2D_SUBMIT_TASK_S *pTask = cfg->pTask;
	V2D_SURFACE_S *pLayer0, *pLayer1, *pDst, *pMask;
	struct dma_buf *dmabuf = NULL;
	int fd;

	pLayer0 = &pTask->param.layer0;
	pLayer1 = &pTask->param.layer1;
	pDst    = &pTask->param.dst;
	pMask   = &pTask->param.mask;

	if (pLayer0->fbc_enable || pLayer0->fd) {
		cfg->info[0].valid = 1;
		cfg->info[0].tbu_id = 0;
		fd = pLayer0->fbc_enable ? pLayer0->fbcDecInfo.fd : pLayer0->fd;
		dmabuf = dma_buf_get(fd);
		if (IS_ERR(dmabuf)) {
			pr_err("v2d layer0 get dmabuf fail fd:%d\n", fd);
			return -1;
		}
		cfg->info[0].dmabuf = dmabuf;
	}

	if (pLayer1->fbc_enable || pLayer1->fd) {
		cfg->info[1].valid = 1;
		cfg->info[1].tbu_id = -1;
		fd = pLayer1->fbc_enable ? pLayer1->fbcDecInfo.fd : pLayer1->fd;
		dmabuf = dma_buf_get(fd);
		if (IS_ERR(dmabuf)) {
			pr_err("v2d layer1 get dmabuf fail fd:%d\n", fd);
			return -1;
		}
		cfg->info[1].dmabuf = dmabuf;
	}

	cfg->info[2].valid = 1;
	cfg->info[2].tbu_id = 1;
	fd = pDst->fbc_enable ? pDst->fbcEncInfo.fd : pDst->fd;
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("v2d layer1 get dmabuf fail fd:%d\n", fd);
		return -1;
	}
	cfg->info[2].dmabuf = dmabuf;

	if (pMask->fd) {
		cfg->info[3].valid = 1;
		cfg->info[3].tbu_id = -1;
		dmabuf = dma_buf_get(pMask->fd);
		if (IS_ERR(dmabuf)) {
			pr_err("v2d mask get dmabuf fail fd:%d\n", fd);
			return -1;
		}
		cfg->info[3].dmabuf = dmabuf;
	}
	return 0;
}

static int get_addr_from_dmabuf(struct v2d_info *v2dinfo, struct v2d_dma_buf_info *pInfo, dma_addr_t *paddr)
{
	struct device *dev = &v2dinfo->pdev->dev;
	struct sg_table *sgt;
	dma_addr_t addr;
	int ret, flags;
	size_t size = 0;
	ret = 0;

	pInfo->attach  = dma_buf_attach(pInfo->dmabuf, dev);
	if (IS_ERR(pInfo->attach)) {
		pr_err("v2d get dma buf attach fail\n");
		goto err_dmabuf_put;
	}
	pInfo->sgtable = dma_buf_map_attachment_unlocked(pInfo->attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(pInfo->sgtable)) {
		pr_err("v2d get dma buf map attachment fail\n");
		goto err_dmabuf_detach;
	}
	sgt = pInfo->sgtable;
	flags = IOMMU_READ | IOMMU_CACHE | IOMMU_WRITE;

	if (sgt->nents == 1) {
		addr = sg_dma_address(sgt->sgl);
	} else {
		addr = V2D_TBU_BASE_VA + (dma_addr_t)(pInfo->tbu_id)*V2D_TBU_VA_STEP;
		size = v2d_iommu_map_sg(addr, sgt->sgl, sgt->orig_nents, flags);
		if (!size) {
			pr_err("v2d iommu map sgtable fail\n");
			goto err_dmabuf_unmap;
		}
	}
	*paddr = addr;
	return ret;

err_dmabuf_unmap:
	dma_buf_unmap_attachment_unlocked(pInfo->attach, pInfo->sgtable, DMA_BIDIRECTIONAL);
err_dmabuf_detach:
	dma_buf_detach(pInfo->dmabuf, pInfo->attach);
err_dmabuf_put:
	dma_buf_put(pInfo->dmabuf);
	return -1;

}

static int v2d_get_dma_addr(struct v2d_info *v2dinfo, struct v2d_pending_post_task *cfg)
{
	V2D_SUBMIT_TASK_S *pTask = cfg->pTask;
	V2D_SURFACE_S *pLayer0, *pLayer1, *pDst, *pMask;
	dma_addr_t addr;
	struct v2d_dma_buf_info *pInfo;
	int ret = 0;
	pLayer0 = &pTask->param.layer0;
	pLayer1 = &pTask->param.layer1;
	pDst    = &pTask->param.dst;
	pMask   = &pTask->param.mask;

	pInfo = &cfg->info[0];
	if (pInfo->valid) {
		ret = get_addr_from_dmabuf(v2dinfo, pInfo, &addr);
		if (ret) return ret;
		if (pLayer0->fbc_enable) {
			pLayer0->fbcDecInfo.headerAddr_l = addr & 0xFFFFFFFF;
			pLayer0->fbcDecInfo.headerAddr_h = (addr >> 32) & 0xFFFFFFFF;
		} else {
			pLayer0->phyaddr_y_l  = addr & 0xFFFFFFFF;
			pLayer0->phyaddr_y_h  = (addr >> 32) & 0xFFFFFFFF;
			pLayer0->phyaddr_uv_l = pLayer0->offset ? (pLayer0->phyaddr_y_l+pLayer0->offset) : 0;
			pLayer0->phyaddr_uv_h = pLayer0->offset ? pLayer0->phyaddr_y_h : 0;
		}
	}

	pInfo = &cfg->info[1];
	if (pInfo->valid) {
		ret = get_addr_from_dmabuf(v2dinfo, pInfo, &addr);
		if (ret) return ret;
		if (pLayer1->fbc_enable) {
			pLayer1->fbcDecInfo.headerAddr_l = addr & 0xFFFFFFFF;
			pLayer1->fbcDecInfo.headerAddr_h = (addr >> 32) & 0xFFFFFFFF;
		} else {
			pLayer1->phyaddr_y_l  = addr & 0xFFFFFFFF;
			pLayer1->phyaddr_y_h  = (addr >> 32) & 0xFFFFFFFF;
			pLayer1->phyaddr_uv_l = pLayer0->offset ? (pLayer0->phyaddr_y_l+pLayer0->offset) : 0;
			pLayer1->phyaddr_uv_h = pLayer0->offset ? pLayer0->phyaddr_y_h : 0;
		}
	}

	pInfo = &cfg->info[2];
	if (pInfo->valid) {
		ret = get_addr_from_dmabuf(v2dinfo, pInfo, &addr);
		if (ret) return ret;
		if (pDst->fbc_enable) {
			pDst->fbcEncInfo.headerAddr_l  = addr & 0xFFFFFFFF;
			pDst->fbcEncInfo.headerAddr_h  = (addr >> 32) & 0xFFFFFFFF;
			pDst->fbcEncInfo.payloadAddr_l = pDst->fbcEncInfo.headerAddr_l + pDst->fbcEncInfo.offset;
			pDst->fbcEncInfo.payloadAddr_h = pDst->fbcEncInfo.headerAddr_h;
		} else {
			pDst->phyaddr_y_l  = addr & 0xFFFFFFFF;
			pDst->phyaddr_y_h  = (addr >> 32) & 0xFFFFFFFF;
			pDst->phyaddr_uv_l = pDst->offset ? (pDst->phyaddr_y_l+pDst->offset) : 0;
			pDst->phyaddr_uv_h = pDst->offset ? pDst->phyaddr_y_h : 0;
		}
	}

	pInfo = &cfg->info[3];
	if (pInfo->valid) {
		ret = get_addr_from_dmabuf(v2dinfo, pInfo, &addr);
		if (ret) return ret;
		pMask->phyaddr_y_l = addr & 0xFFFFFFFF;
		pMask->phyaddr_y_h = (addr >> 32) & 0xFFFFFFFF;
	}
	return ret;
}

static void v2d_put_dmabuf(struct v2d_info *v2dinfo, struct v2d_pending_post_task *cfg)
{
	int i;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sg_table;
	struct v2d_dma_buf_info *pInfo;

	for (i=0; i<4; i++) {
		pInfo    = &cfg->info[i];
		dmabuf   = pInfo->dmabuf;
		attach   = pInfo->attach;
		sg_table = pInfo->sgtable;

		if (dmabuf && attach && sg_table) {
			dma_buf_unmap_attachment_unlocked(attach, sg_table, DMA_BIDIRECTIONAL);
			dma_buf_detach(dmabuf, attach);
			dma_buf_put(dmabuf);
		}
	}
	v2d_iommu_map_end();
}

int v2d_job_submit(struct v2d_info *info, V2D_SUBMIT_TASK_S *psubmit)
{
	int err = 0;
 	V2D_SUBMIT_TASK_S *pTask = NULL;
	struct v2d_pending_post_task *cfg = NULL;
	struct dma_fence *fence = NULL;
	pTask = kzalloc(sizeof(V2D_SUBMIT_TASK_S), GFP_KERNEL);
	if (!pTask){
		err = -ENOMEM;
		goto error;
	}
	memset(pTask,0,sizeof(V2D_SUBMIT_TASK_S));
	if(copy_from_user(pTask,(uint32_t *)psubmit, sizeof(V2D_SUBMIT_TASK_S)) != 0) {
		err = -EINVAL;
		goto error;
	}
	if(v2d_fence_generate(info, &fence, &pTask->completeFencefd))
	{
		printk(KERN_ERR "%s" "-%s-Failed to generate fence(%pf),fd(%d)-slot1\n", "v2d", __func__,fence, pTask->completeFencefd);
		err = -EINVAL;
		goto error;
	}
	if (0 != copy_to_user((__user uint8_t *)psubmit+offsetof(V2D_SUBMIT_TASK_S, completeFencefd), &pTask->completeFencefd, sizeof(int32_t))) {
		pTask->completeFencefd = -1;
		err = -EINVAL;
		goto error;
	}
	mutex_lock(&info->client_lock);
	cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg){
		mutex_unlock(&info->client_lock);
		err = -ENOMEM;
		goto error;
	}
	memset(cfg,0,sizeof(struct v2d_pending_post_task));
	INIT_LIST_HEAD(&cfg->head);
	cfg->pTask = pTask;
	if (pTask->completeFencefd>=0)
	{
		cfg->pCompleteFence = fence;
	}
	if (pTask->acquireFencefd>=0)
	{
		#ifdef CONFIG_SYNC_FILE
		cfg->pAcquireFence = sync_file_get_fence(cfg->pTask->acquireFencefd);
		#endif
	}
	err = v2d_get_dmabuf(info, cfg);
	if (err) {
		mutex_unlock(&info->client_lock);
		kfree(cfg);
		goto error;
	}
	mutex_lock(&info->post_lock);
	list_add_tail(&cfg->head, &info->post_list);
	kthread_queue_work(&info->post_worker, &info->post_work);
	mutex_unlock(&info->post_lock);
	mutex_unlock(&info->client_lock);
	return 0;

error:
	if(pTask){
		kfree(pTask);
	}
	return err;
}

void v2d_work_done(struct work_struct *data)
{
	struct v2d_pending_post_task *element, *tmp;
	int refcount;
	struct dma_fence *pCompleteFence = NULL;
	struct v2d_info *info = container_of(data, struct v2d_info, work);

	mutex_lock(&info->free_lock);
	list_for_each_entry_safe(element, tmp, &info->free_list, head) {
		if (element->pTask->completeFencefd>=0)
		{
			pCompleteFence = element->pCompleteFence;
			if(NULL != pCompleteFence) {
				dma_fence_signal(pCompleteFence);
				dma_fence_put(pCompleteFence);
			}
		}
		v2d_put_dmabuf(info, element);
		mutex_lock(&info->power_mutex);
		info->refcount--;
		refcount = info->refcount;
		if (info->do_reset) {
			v2d_golbal_reset();
			info->do_reset = 0;
		}
		if(!refcount)
		{
			v2d_irq_disable();
			v2d_clk_off(info);
		}
		mutex_unlock(&info->power_mutex);
		list_del(&element->head);
		kfree_v2d_post_task(element);
		up(&info->sem_lock);
	}
	mutex_unlock(&info->free_lock);
}

void do_softreset(void)
{
	struct v2d_pending_post_task *element, *tmp;
	struct dma_fence *pCompleteFence = NULL;
	int refcount;
	unsigned long flags =  0;
	struct v2d_info *info = v2dInfo;

	mutex_lock(&info->free_lock);
	list_for_each_entry_safe(element, tmp, &info->free_list, head) {
		if (element->pTask->completeFencefd>=0)
		{
			pCompleteFence = element->pCompleteFence;
			if(NULL != pCompleteFence) {
				dma_fence_signal(pCompleteFence);
				dma_fence_put(pCompleteFence);
			}
		}
		v2d_put_dmabuf(info, element);
		mutex_lock(&info->power_mutex);
		spin_lock_irqsave(&info->power_spinlock, flags);
		info->refcount--;
		refcount = info->refcount;
		v2d_dump_irqraw_status();
		v2d_golbal_reset();
		spin_unlock_irqrestore(&info->power_spinlock, flags);
		if(!refcount)
		{
			v2d_irq_disable();
			v2d_clk_off(info);
		}
		mutex_unlock(&info->power_mutex);
		list_del(&element->head);
		kfree_v2d_post_task(element);
		up(&info->sem_lock);
	}
	mutex_unlock(&info->free_lock);
	flush_workqueue(info->v2d_job_done_wq);
}

void v2d_post_work_func(struct kthread_work *work)
{
	struct v2d_info *info = container_of(work, struct v2d_info, post_work);
	struct v2d_pending_post_task *post, *next;
	int refcount;
	unsigned long flags = 0;
	struct dma_fence *pAcquireFence = NULL;
	mutex_lock(&info->post_lock);
	list_for_each_entry_safe(post, next, &info->post_list, head) {
		while(down_timeout(&info->sem_lock, msecs_to_jiffies(2500)))
		{
			printk(KERN_ERR "%s hang do softreset\n", "v2d");
			do_softreset();
		}
		if (post->pTask->acquireFencefd>=0)
		{
			pAcquireFence = post->pAcquireFence;
			v2d_fence_wait(info, pAcquireFence);
			dma_fence_put(pAcquireFence);
		}
		list_del(&post->head);
		mutex_lock(&info->free_lock);
		list_add_tail(&post->head, &info->free_list);
		mutex_unlock(&info->free_lock);
		mutex_lock(&info->power_mutex);
		spin_lock_irqsave(&info->power_spinlock, flags);
		refcount = info->refcount;
		info->refcount++;
		spin_unlock_irqrestore(&info->power_spinlock, flags);
		if(!refcount)
		{
			v2d_clk_on(info);
			v2d_irq_enable();
		}
		if (v2d_get_dma_addr(info, post)) {
			queue_work(info->v2d_job_done_wq, &info->work);
		} else {
			config_v2d_hw(post->pTask);
		}
		mutex_unlock(&info->power_mutex);
	}
	mutex_unlock(&info->post_lock);
}


static DEFINE_MUTEX(v2d_wr_lock);
static DEFINE_MUTEX(v2d_dev_lock);
static int v2d_dev_ref = 0;
static int v2d_dev_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&(v2d_dev_lock));
	filp->private_data = (void *)v2dInfo;
	v2d_dev_ref++;
	mutex_unlock(&(v2d_dev_lock));
	return 0;
}

static int v2d_dev_release(struct inode *inode, struct file *filp)
{
	mutex_lock(&(v2d_dev_lock));
	v2d_dev_ref--;
	mutex_unlock(&(v2d_dev_lock));
	return 0;
}

ssize_t v2d_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

ssize_t v2d_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int ret;
	struct v2d_info *pInfo;

	mutex_lock(&(v2d_wr_lock));
	pInfo = (struct v2d_info *)filp->private_data;
	ret = v2d_job_submit(pInfo, (V2D_SUBMIT_TASK_S*)buf);
	if (ret) {
		mutex_unlock(&(v2d_wr_lock));
		V2DLOGE("v2d faild to write msg %d\n", ret);
		return -EIO;
	}
	mutex_unlock(&(v2d_wr_lock));
	return count;
}

static int v2d_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err;
	unsigned long size = vma->vm_end - vma->vm_start;

	if ((vma->vm_pgoff + (size >> PAGE_SHIFT)) > (1 + (P4D_SHIFT >> PAGE_SHIFT))) {
			pr_err("out of physical memory\n");
			return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_ops = NULL;
	err = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
									size, vma->vm_page_prot);
	if (err) {
			pr_err("failed to v2d map memroy\n");
			return -ENXIO;
	}

	return 0;
}

static const struct file_operations v2d_dev_fops = {
	.owner	= THIS_MODULE,
	.open	= v2d_dev_open,
	.release = v2d_dev_release,
	.read	= v2d_dev_read,
	.write	= v2d_dev_write,
	.poll	= NULL,
	.mmap	= v2d_mmap,
};

extern struct v2d_iommu_res sV2dIommuRes;
static int v2d_iommu_init(struct platform_device *pdev, void __iomem *base)
{
	struct device *dev = &pdev->dev;
	int i;
	struct v2d_iommu_res *v2d_res = &sV2dIommuRes;
	void *va_temp;
	dma_addr_t pa_temp;

	v2d_res->base = base;
	v2d_res->page_size = SZ_4K;
	va_temp = dma_alloc_coherent(dev, MAX_SIZE_PER_TTB*TBU_INSTANCES_NUM, &pa_temp, GFP_KERNEL|GFP_DMA);
	if (!va_temp) {
		pr_err("v2d iommu no memory for %d tbu_ins!\n",
			TBU_INSTANCES_NUM);
		return -1;
	}

	for (i = 0; i < TBU_INSTANCES_NUM; i++) {
		struct tbu_instance *tbu = NULL;
		if (i <TBU_INSTANCES_NUM) {
			tbu = &v2d_res->tbu_ins[i];
			tbu->ins_id = i;
		}
		tbu->ttb_va = va_temp + i * MAX_SIZE_PER_TTB;
		tbu->ttb_pa = pa_temp + i * MAX_SIZE_PER_TTB;
	}

	v2d_res->va_base = BASE_VIRTUAL_ADDRESS;
	v2d_res->va_end  = BASE_VIRTUAL_ADDRESS + TBU_NUM * VA_STEP_PER_TBU;
	v2d_res->time_out_cycs = DEFAULT_TIMEOUT_CYCS;

	return 0;
}

static int v2d_iommu_deinit(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct v2d_iommu_res *v2d_res = &sV2dIommuRes;

	dma_free_coherent(dev,
				MAX_SIZE_PER_TTB*TBU_INSTANCES_NUM,
				v2d_res->tbu_ins[0].ttb_va,
				v2d_res->tbu_ins[0].ttb_pa);

	return 0;
}


static u64 v2d_dmamask = 0xffffffffffUL;
static int v2d_probe(struct platform_device *pdev)
{
	struct v2d_info *info;
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *base;
	int i, rval = 0;
	struct sched_param param;
	int ret;

	info = devm_kzalloc(dev, sizeof(struct v2d_info), GFP_KERNEL);
	if (info == NULL) {
		return -ENOMEM;
	}
	dev->dma_mask = &v2d_dmamask;
	dev->coherent_dma_mask = 0xffffffffffull;

	info->clkcore = devm_clk_get(dev, "v2d-core");
	if (IS_ERR(info->clkcore)) {
		V2DLOGE("Could not get v2d core clk!\n");
		return -EINVAL;
	}

	info->v2d_reset = devm_reset_control_get_optional_shared(&pdev->dev, "v2d_reset");
	if (IS_ERR_OR_NULL(info->v2d_reset)) {
		V2DLOGE("Could not get v2d reset!\n");
		return -EINVAL;
	}

	ret = reset_control_deassert(info->v2d_reset);
	if (ret < 0) {
		V2DLOGI("Failed to deassert v2d_reset\n");
	}
	clk_prepare_enable(info->clkcore);
	clk_set_rate(info->clkcore, 409600000);

	info->clkio = devm_clk_get(dev, "v2d-io");
	if (IS_ERR(info->clkio)) {
		V2DLOGE("Could not get v2d io clk!\n");
		return -EINVAL;
	}
	/* get v2d regs base */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "v2dreg");
	if (res == NULL) {
		 return -ENOENT;
	}
	base = devm_ioremap(dev, res->start, resource_size(res));
	if (base == NULL) {
		return -EIO;
	}
	info->v2dreg_iomem_base = base;
	info->irq = platform_get_irq(pdev, 0);
	//V2DLOGI("v2d irq num = %d\n", info->irq);
	if (info->irq < 0) {
		return -ENOENT;
	}
	rval = devm_request_irq(dev, info->irq, v2d_irq_handler, IRQF_SHARED, "v2d-irq", info);
	if (rval) {
		return rval;
	}
	v2d_iommu_init(pdev, base);

	for (i = 0; i < ARRAY_SIZE(v2d_sysfs_files); i++) {
		rval = device_create_file(dev, &v2d_sysfs_files[i]);
		if (rval)
			return rval;
	}
	mutex_init(&info->power_mutex);
	spin_lock_init(&info->power_spinlock);
	info->refcount = 0;
	info->pdev = pdev;
	platform_set_drvdata(pdev, info);
	info->mdev.minor  = MISC_DYNAMIC_MINOR;
	info->mdev.name   = "v2d_dev";
	info->mdev.fops   = &v2d_dev_fops;
	rval = misc_register(&info->mdev);
	if (rval) {
		V2DLOGE("failed register v2d misc device ret=%d\n", rval);
		goto err_misc;
	}
	sema_init(&info->sem_lock, 1);
	info->context = dma_fence_context_alloc(1);
	info->v2d_job_done_wq = alloc_workqueue("ky_v2d", WQ_HIGHPRI | WQ_UNBOUND, 1);
	if (NULL == info->v2d_job_done_wq) {
		V2DLOGE( "%s: alloc_workqueue failed\n", __func__);
		goto err;
	}
	INIT_WORK(&info->work, v2d_work_done);
	mutex_init(&info->client_lock);
	INIT_LIST_HEAD(&info->post_list);
	mutex_init(&info->post_lock);
	INIT_LIST_HEAD(&info->free_list);
	mutex_init(&info->free_lock);
	kthread_init_worker(&info->post_worker);
	info->post_thread = kthread_run(kthread_worker_fn, &info->post_worker, "v2d");
	if (IS_ERR(info->post_thread)) {
		rval = PTR_ERR(info->post_thread);
		info->post_thread = NULL;
		V2DLOGE("%s: failed to run config posting thread: %d\n", __func__, rval);
		goto err;
	}
	param.sched_priority = 1;
	sched_setscheduler(info->post_thread, SCHED_FIFO, &param);
	kthread_init_work(&info->post_work, v2d_post_work_func);
#ifdef CONFIG_KY_DEBUG
	info->is_v2d_running = check_v2d_running_status;
	info->nb.notifier_call = v2d_clkoffdet_notifier_handler;
	clk_notifier_register(info->clkcore, &info->nb);
#endif
	//V2DLOGI("probe v2d driver done!\n");
	v2dInfo = info;

	return 0;

err:
	if(info->post_thread)
		 kthread_stop(info->post_thread);
	if(info->v2d_job_done_wq)
		destroy_workqueue(info->v2d_job_done_wq);

err_misc:
	for (i = 0; i < ARRAY_SIZE(v2d_sysfs_files); i++) {
		device_remove_file(dev, &v2d_sysfs_files[i]);
	}
	misc_deregister(&info->mdev);

	return rval;
}

static int v2d_remove(struct platform_device *pdev)
{
	struct v2d_info *info = platform_get_drvdata(pdev);
	struct device *dev = &info->pdev->dev;
	int i;
	int ret;

	//V2DLOGI("remove v2d driver!\n");
	v2d_iommu_deinit(pdev);
	devm_free_irq(dev, info->irq, info);
	kthread_flush_worker(&info->post_worker);
	kthread_stop(info->post_thread);
	for (i = 0; i < ARRAY_SIZE(v2d_sysfs_files); i++) {
		device_remove_file(dev, &v2d_sysfs_files[i]);
	}
#ifdef CONFIG_KY_DEBUG
	info->is_v2d_running = NULL;
	info->nb.notifier_call = NULL;
	clk_notifier_unregister(info->clkcore, &info->nb);
#endif
	misc_deregister(&info->mdev);
	if(info->v2d_job_done_wq)
		destroy_workqueue(info->v2d_job_done_wq);

	if (__clk_is_enabled(info->clkcore)) {
		clk_disable_unprepare(info->clkcore);
	}
	ret = reset_control_assert(info->v2d_reset);
	if (ret < 0) {
		V2DLOGI("Failed to assert v2d_reset\n");
	}

	v2dInfo = NULL;

	return 0;
}

static const struct of_device_id v2d_drv_match_table[] = {
	{ .compatible = "ky,v2d" },
	{},
};

MODULE_DEVICE_TABLE(of, v2d_drv_match_table);

static struct platform_driver v2d_driver = {
	.driver 	= {
		.name	= V2D_DRV_NAME,
		.of_match_table = of_match_ptr(v2d_drv_match_table),
		.pm		= &v2d_pm_ops,
	},
	.probe		= v2d_probe,
	.remove 	= v2d_remove,
};

static int __init v2d_init(void)
{
	return platform_driver_register(&v2d_driver);
}

static void __exit v2d_exit(void)
{
	platform_driver_unregister(&v2d_driver);
}

module_init(v2d_init);
module_exit(v2d_exit);

MODULE_DESCRIPTION("Ky V2D driver");
MODULE_LICENSE("GPL");

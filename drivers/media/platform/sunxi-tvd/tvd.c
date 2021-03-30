/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/string.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
#include <linux/freezer.h>
#endif

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-core.h>
#include <linux/sys_config.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/switch.h>

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
#include <linux/sunxi_dramfreq.h>
#endif

#include "tvd.h"

#define TVD_MODULE_NAME "sunxi_tvd"
#define MIN_WIDTH  (32)
#define MIN_HEIGHT (32)
#define MAX_WIDTH  (4096)
#define MAX_HEIGHT (4096)
#define MAX_BUFFER (32*1024*1024)
#define NUM_INPUTS 1
#define CVBS_INTERFACE   0
#define YPBPRI_INTERFACE 1
#define YPBPRP_INTERFACE 2
#define NTSC   0
#define PAL    1
#define NONE   2

#define TVD_MAX_POWER_NUM 2
#define TVD_MAX_GPIO_NUM 2
#define TVD_MAX 4
#define TVD_MAJOR_VERSION 1
#define TVD_MINOR_VERSION 0
#define TVD_RELEASE		  0
#define TVD_VERSION \
  KERNEL_VERSION(TVD_MAJOR_VERSION, TVD_MINOR_VERSION, TVD_RELEASE)

static unsigned int tvd_dbg_en;
static unsigned int tvd_dbg_sel;
#define TVD_DBG_DUMP_LEN 0x200000
#define TVD_NAME_LEN 32
static char tvd_dump_file_name[TVD_NAME_LEN];
static struct tvd_status tvd_status[4];
static struct tvd_dev *tvd[4];
static unsigned int tvd_hot_plug;

/* use for reversr special interfaces */
static int tvd_count;
static int fliter_count;
static struct mutex fliter_lock;
static struct mutex power_lock;
static char tvd_power[TVD_MAX_POWER_NUM][32];
static struct gpio_config tvd_gpio_config[TVD_MAX_GPIO_NUM];
static struct regulator *regu[TVD_MAX_POWER_NUM];
static atomic_t tvd_used_power_num = ATOMIC_INIT(0);
static atomic_t tvd_used_gpio_num = ATOMIC_INIT(0);
static atomic_t gpio_power_enable_count = ATOMIC_INIT(0);

static irqreturn_t tvd_isr(int irq, void *priv);
static irqreturn_t tvd_isr_special(int irq, void *priv);
static int __tvd_fetch_sysconfig(int sel, char *sub_name, int value[]);
static int __tvd_auto_plug_enable(struct tvd_dev *dev);
static int __tvd_auto_plug_disable(struct tvd_dev *dev);

static ssize_t tvd_dbg_en_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tvd_dbg_en);
}

static ssize_t tvd_dbg_en_store(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		pr_debug("Invalid size\n");
		return err;
	}

	if(val < 0 || val > 1) {
		pr_debug("Invalid value, 0~1 is expected!\n");
	} else {
		tvd_dbg_en = val;

		pr_debug("tvd_dbg_en = %ld\n", val);
	}

	return count;
}

static ssize_t tvd_dbg_lv_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tvd_dbg_sel);
}

static ssize_t tvd_dbg_lv_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);
	if (err) {
		pr_debug("Invalid size\n");
		return err;
	}

	if(val < 0 || val > 4) {
		pr_debug("Invalid value, 0~3 is expected!\n");
	} else {
		tvd_dbg_sel = val;
		pr_debug("tvd_dbg_sel = %d\n", tvd_dbg_sel);
		tvd_isr(46, tvd[tvd_dbg_sel]);
	}

	return count;
}

static ssize_t tvd_dbg_dump_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	struct tvd_dev *tvd_dev = (struct tvd_dev *)dev_get_drvdata(dev);
	struct file *pfile;
	mm_segment_t old_fs;
	ssize_t bw;
	dma_addr_t buf_dma_addr;
	void *buf_addr;

	buf_addr = dma_alloc_coherent(dev, TVD_DBG_DUMP_LEN,
	    (dma_addr_t *)&buf_dma_addr, GFP_KERNEL);
	if (!buf_addr) {
		pr_warn("%s(), dma_alloc_coherent fail, size=0x%x\n",
		    __func__, TVD_DBG_DUMP_LEN);

		return 0;
	}

	/* start debug mode */
	if (tvd_dbgmode_dump_data(tvd_dev->sel,
	    0, buf_dma_addr, TVD_DBG_DUMP_LEN / 2)) {
		pr_warn("%s(), debug mode start fail\n", __func__);
		goto exit;
	}

	pfile = filp_open(tvd_dump_file_name,
	    O_RDWR|O_CREAT|O_EXCL, 0755);
	if (IS_ERR(pfile)) {
		pr_warn("%s, open %s err\n",
		    __func__, tvd_dump_file_name);
		goto exit;
	}
	pr_warn("%s, open %s ok\n",
	    __func__, tvd_dump_file_name);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	bw = pfile->f_op->write(pfile,
	    (const char *)buf_addr, TVD_DBG_DUMP_LEN, &pfile->f_pos);

	if (unlikely(bw != TVD_DBG_DUMP_LEN))
		pr_warn("%s, write %s err at byte offset %llu\n",
		    __func__, tvd_dump_file_name, pfile->f_pos);
	set_fs(old_fs);
	filp_close(pfile, NULL);
	pfile = NULL;

exit:
	dma_free_coherent(dev, TVD_DBG_DUMP_LEN, buf_addr, buf_dma_addr);

	return 0;
}

static ssize_t tvd_dbg_dump_store(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf, size_t count)
{
	memset(tvd_dump_file_name,  '\0', TVD_NAME_LEN);
	count = (count > TVD_NAME_LEN) ? TVD_NAME_LEN : count;
	memcpy(tvd_dump_file_name, buf, count - 1);

	pr_info("%s(), get dump file name %s\n",
	    __func__, tvd_dump_file_name);

	return count;
}

static DEVICE_ATTR(tvd_dbg_en, S_IRUGO|S_IWUSR|S_IWGRP,
		tvd_dbg_en_show, tvd_dbg_en_store);

static DEVICE_ATTR(tvd_dbg_lv, S_IRUGO|S_IWUSR|S_IWGRP,
		tvd_dbg_lv_show, tvd_dbg_lv_store);
static DEVICE_ATTR(tvd_dump, S_IRUGO|S_IWUSR|S_IWGRP,
		tvd_dbg_dump_show, tvd_dbg_dump_store);

static struct attribute *tvd0_attributes[] = {
	&dev_attr_tvd_dbg_en.attr,
	&dev_attr_tvd_dbg_lv.attr,
	&dev_attr_tvd_dump.attr,
	NULL
};

static struct attribute *tvd1_attributes[] = {
	&dev_attr_tvd_dbg_en.attr,
	&dev_attr_tvd_dbg_lv.attr,
	&dev_attr_tvd_dump.attr,
	NULL
};

static struct attribute *tvd2_attributes[] = {
	&dev_attr_tvd_dbg_en.attr,
	&dev_attr_tvd_dbg_lv.attr,
	&dev_attr_tvd_dump.attr,
	NULL
};

static struct attribute *tvd3_attributes[] = {
	&dev_attr_tvd_dbg_en.attr,
	&dev_attr_tvd_dbg_lv.attr,
	&dev_attr_tvd_dump.attr,
	NULL
};

static struct attribute_group tvd_attribute_group[] = {
	{	.name = "tvd0_attr",
		.attrs = tvd0_attributes
	},
	{	.name = "tvd1_attr",
		.attrs = tvd1_attributes
	},
	{	.name = "tvd2_attr",
		.attrs = tvd2_attributes
	},
	{	.name = "tvd3_attr",
		.attrs = tvd3_attributes
	},

};

static struct tvd_fmt formats[] = {
	{
		.name = "planar UVUV",
		.fourcc = V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
	},
	{
		.name = "planar VUVU",
		.fourcc = V4L2_PIX_FMT_NV21,
		.output_fmt = TVD_PL_YUV420,
		.depth = 12,
	},
	{
		.name = "planar UVUV",
		.fourcc = V4L2_PIX_FMT_NV16,
		.output_fmt = TVD_PL_YUV422,
		.depth = 16,
	},
	{
		.name = "planar VUVU",
		.fourcc = V4L2_PIX_FMT_NV61,
		.output_fmt = TVD_PL_YUV422,
		.depth = 16,
	},
	/* this format is not standard, just for allwinner. */
	{
		.name = "planar PACK",
		.fourcc = 0,
		.output_fmt = TVD_MB_YUV420,
		.depth = 12,
	},
};

static int inline tvd_is_generating(struct tvd_dev *dev)
{
	int ret;
	ret = test_bit(0, &dev->generating);
	return ret;
}

static void inline tvd_start_generating(struct tvd_dev *dev)
{
	set_bit(0, &dev->generating);
	return;
}

static void inline tvd_stop_generating(struct tvd_dev *dev)
{
	clear_bit(0, &dev->generating);
	return;
}

static int tvd_is_opened(struct tvd_dev *dev)
{
	 int ret;
	 mutex_lock(&dev->opened_lock);
	 ret = test_bit(0, &dev->opened);
	 mutex_unlock(&dev->opened_lock);
	 return ret;
}

static void tvd_start_opened(struct tvd_dev *dev)
{
	 mutex_lock(&dev->opened_lock);
	 set_bit(0, &dev->opened);
	 mutex_unlock(&dev->opened_lock);
}

static void tvd_stop_opened(struct tvd_dev *dev)
{
	 mutex_lock(&dev->opened_lock);
	 clear_bit(0, &dev->opened);
	 mutex_unlock(&dev->opened_lock);
}

static int __tvd_clk_init(struct tvd_dev *dev)
{
	int div = 0, ret = 0;
	unsigned long p = 297000000;

	pr_debug("%s: dev->interface = %d, dev->system = %d\n",
		__func__, dev->interface, dev->system);

	dev->parent = clk_get_parent(dev->clk);
	if (IS_ERR_OR_NULL(dev->parent) || IS_ERR_OR_NULL(dev->clk))
		return -EINVAL;

	/* parent is 297M */
	ret = clk_set_rate(dev->parent, p);
	if (ret) {
		ret = -EINVAL;
		goto out;
	}

	if (dev->interface == CVBS_INTERFACE) {
		/* cvbs interface */
		if (PAL == dev->system)
			div = 10;
		else
			div = 11;

	} else if (dev->interface == YPBPRI_INTERFACE) {
		/* ypbprI interface */
		div = 11;
	} else if (dev->interface == YPBPRP_INTERFACE) {
		/* ypbprP interface */
		ret = clk_set_rate(dev->parent, 594000000);
		p = 594000000;
		div = 11;
	} else {
		pr_err("%s: interface is err!\n", __func__);
		return -EINVAL;
	}

	pr_debug("div = %d\n", div);

	p /= div;
	ret = clk_set_rate(dev->clk, p);
	if (ret) {
		ret = -EINVAL;
		goto out;
	}

	pr_debug("%s: parent = %lu, clk = %lu\n",
		__func__, clk_get_rate(dev->parent), clk_get_rate(dev->clk));

out:
	return ret;
}

static int __tvd_clk_enable(struct tvd_dev *dev)
{
	int ret = 0;

	ret = clk_prepare_enable(dev->clk_top);
	if (ret) {
		pr_err("%s: tvd top clk enable err!", __func__);
		clk_disable(dev->clk_top);
		return ret;
	}

	ret = clk_prepare_enable(dev->clk);
	if (ret) {
		pr_err("%s: tvd clk enable err!", __func__);
		clk_disable(dev->clk);
	}

	return ret;
}

static int __tvd_clk_disable(struct tvd_dev *dev)
{
	int ret = 0;

	clk_disable(dev->clk);
	clk_disable(dev->clk_top);

	return ret;
}

static int __tvd_init(struct tvd_dev *dev)
{
	tvd_top_set_reg_base((unsigned long)dev->regs_top);
	tvd_set_reg_base(dev->sel, (unsigned long)dev->regs_tvd);

	return 0;
}

static int __tvd_config(struct tvd_dev *dev)
{
	tvd_init(dev->sel, dev->interface);
	tvd_config(dev->sel, dev->interface, dev->system);
	tvd_set_wb_width(dev->sel, dev->width);
	tvd_set_wb_width_jump(dev->sel, dev->width);
	if (dev->interface == YPBPRP_INTERFACE)
		tvd_set_wb_height(dev->sel, dev->height);	/*P,no div*/
	else
		tvd_set_wb_height(dev->sel, dev->height/2);

	/* pl_yuv420, mb_yuv420, pl_yuv422 */
	tvd_set_wb_fmt(dev->sel, dev->fmt->output_fmt);
	switch (dev->fmt->fourcc) {

	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
		tvd_set_wb_uv_swap(dev->sel, 0);
		break;

	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV61:
		tvd_set_wb_uv_swap(dev->sel, 1);
		break;
	}

	return 0;
}

static int __tvd_3d_comp_mem_request(struct tvd_dev *dev, int size)
{
	unsigned long phyaddr;

	dev->fliter.size = PAGE_ALIGN(size);
	dev->fliter.vir_address = dma_alloc_coherent(dev->v4l2_dev.dev, size,
					(dma_addr_t *)&phyaddr, GFP_KERNEL);
	dev->fliter.phy_address = (void *)phyaddr;
	if (IS_ERR_OR_NULL(dev->fliter.vir_address)) {
		pr_err("%s: 3d fliter buf_alloc failed!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static void __tvd_3d_comp_mem_free(struct tvd_dev *dev)
{
	u32 actual_bytes;
	actual_bytes = PAGE_ALIGN(dev->fliter.size);
	if (dev->fliter.phy_address && dev->fliter.vir_address)
		dma_free_coherent(dev->v4l2_dev.dev, actual_bytes,
					dev->fliter.vir_address,
					(dma_addr_t)dev->fliter.phy_address);
}

/*
 * set width,set height, set jump, set wb addr, set 3d_comb
*/
static void __tvd_set_addr(struct tvd_dev *dev, struct tvd_buffer *buffer)
{
	struct tvd_buffer *buf = buffer;
	dma_addr_t addr_org;
	struct vb2_buffer *vb_buf = &buf->vb;
	unsigned int c_offset = 0;

	if(vb_buf == NULL || vb_buf->planes[0].mem_priv == NULL) {
		pr_err("%s: vb_buf->priv is NULL!\n", __func__);
		return;
	}

	addr_org = vb2_dma_contig_plane_dma_addr(vb_buf, 0);
	switch (dev->fmt->output_fmt) {

	case TVD_PL_YUV422:
	case TVD_PL_YUV420:
		c_offset = dev->width * dev->height;
		break;

	case TVD_MB_YUV420:
		c_offset = 0;
		break;
	default:
		break;
	}

	/* set y_addr,c_addr */
	pr_debug("%s: format:%d, addr_org = 0x%x, addr_org + c_offset = 0x%x\n",
		__func__, dev->format, addr_org, addr_org + c_offset);
	tvd_set_wb_addr(dev->sel, addr_org, addr_org + c_offset);
}

/*
 *  the interrupt routine
*/
static irqreturn_t tvd_isr(int irq, void *priv)
{
	struct tvd_buffer *buf;
	unsigned long flags;
	struct list_head *entry_tmp;
	u32 irq_status = 0;
	struct tvd_dev *dev = (struct tvd_dev *)priv;
	struct tvd_dmaqueue *dma_q = &dev->vidq;
	u32 value = (1 << TVD_IRQ_FIFO_C_O)|(1 << TVD_IRQ_FIFO_Y_O) \
				|(1 << TVD_IRQ_FIFO_C_U)|(1 << TVD_IRQ_FIFO_Y_U) \
				|(1 << TVD_IRQ_WB_ADDR_CHANGE_ERR);

	if (dev->special_active == 1) {
		return tvd_isr_special(irq, priv);
	}

	if(tvd_is_generating(dev) == 0) {
		tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);
		return IRQ_HANDLED;
	}
	tvd_dma_irq_status_get(dev->sel, &irq_status);
	if ((irq_status & value) != 0)
		tvd_dma_irq_status_clear_err_flag(dev->sel, value);
	spin_lock_irqsave(&dev->slock, flags);

	if (0 == dev->first_flag) {
		/* if is first frame, flag set 1 */
		dev->first_flag = 1;
		goto set_next_addr;
	}
	if (list_empty(&dma_q->active) || dma_q->active.next->next == (&dma_q->active)) {
		pr_debug("No active queue to serve\n");
		goto unlock;
	}

	buf = list_entry(dma_q->active.next, struct tvd_buffer, list);
	/* Nobody is waiting for this buffer*/
	if (!waitqueue_active(&buf->vb.vb2_queue->done_wq)) {
		pr_debug("%s: Nobody is waiting on this video buffer,buf = 0x%p\n",__func__, buf);
	}
	entry_tmp = &buf->list;
	if ((entry_tmp == NULL) || (entry_tmp->prev == NULL) || (entry_tmp->next == NULL) \
		|| (entry_tmp->prev == LIST_POISON2) || (entry_tmp->next == LIST_POISON1) \
		|| (entry_tmp->prev->next == NULL) || (entry_tmp->next->prev == NULL)) {
		if (entry_tmp == NULL)
			pr_err("%s: buf NULL pointer error \n", __func__);
		pr_err("%s: buf list error \n", __func__);
		goto unlock;
	}

	list_del(&buf->list);
	dev->ms += jiffies_to_msecs(jiffies - dev->jiffies);
	dev->jiffies = jiffies;
	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

	if (list_empty(&dma_q->active)) {
		pr_debug("%s: No more free frame\n", __func__);
		goto unlock;
	}

	/* hardware need one frame */
	if ((&dma_q->active) == dma_q->active.next->next) {
		pr_debug("No more free frame on next time\n");
		goto unlock;
	}

set_next_addr:
	buf = list_entry(dma_q->active.next->next, struct tvd_buffer, list);
	__tvd_set_addr(dev, buf);

unlock:
	spin_unlock(&dev->slock);
	tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);

	return IRQ_HANDLED;
}

/*
 * Videobuf operations
*/
static int queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
				unsigned int *nbuffers, unsigned int *nplanes,
				unsigned int sizes[], void *alloc_ctxs[])
{
	struct tvd_dev *dev = vb2_get_drv_priv(vq);
	unsigned int size;

	switch (dev->fmt->output_fmt) {
		case TVD_MB_YUV420:
		case TVD_PL_YUV420:
			size = dev->width * dev->height * 3/2;
			break;
		case TVD_PL_YUV422:
		default:
			size = dev->width * dev->height * 2;
			break;
	}

	if (size == 0)
		return -EINVAL;

	if (*nbuffers < 3) {
		*nbuffers = 3;
		pr_err("buffer conunt invalid, min is 3!\n");
	} else if (*nbuffers > 10) {
		*nbuffers = 10;
		pr_err("buffer conunt invalid, max 10!\n");
	}

  	dev->frame_size = size;
	sizes[0] = size;
 	*nplanes = 1;
	alloc_ctxs[0] = dev->alloc_ctx;

	pr_debug("%s, buffer count=%d, size=%d\n", __func__, *nbuffers, size);

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct tvd_dev *dev = vb2_get_drv_priv(vb->vb2_queue);
	struct tvd_buffer *buf = container_of(vb, struct tvd_buffer, vb);
	unsigned long size;

	pr_debug("%s: buffer_prepare\n", __func__);

	if (dev->width < MIN_WIDTH || dev->width  > MAX_WIDTH ||
		dev->height < MIN_HEIGHT || dev->height > MAX_HEIGHT)
	{
		return -EINVAL;
	}

	size = dev->frame_size;

	if (vb2_plane_size(vb, 0) < size) {
		pr_err("%s data will not fit into plane (%lu < %lu)\n",
				__func__, vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(&buf->vb, 0, size);

	vb->v4l2_planes[0].m.mem_offset = vb2_dma_contig_plane_dma_addr(vb, 0);

	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct tvd_dev *dev = vb2_get_drv_priv(vb->vb2_queue);
	struct tvd_buffer *buf = container_of(vb, struct tvd_buffer, vb);
	struct tvd_dmaqueue *vidq = &dev->vidq;
	unsigned long flags = 0;

	pr_debug("%s: \n", __func__);
	spin_lock_irqsave(&dev->slock, flags);
	list_add_tail(&buf->list, &vidq->active);
	spin_unlock_irqrestore(&dev->slock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct tvd_dev *dev = vb2_get_drv_priv(vq);

	pr_debug("%s:\n", __func__);
	tvd_start_generating(dev);
	return 0;
}

/* abort streaming and wait for last buffer */
static int stop_streaming(struct vb2_queue *vq)
{
	struct tvd_dev *dev = vb2_get_drv_priv(vq);
	struct tvd_dmaqueue *dma_q = &dev->vidq;
	unsigned long flags = 0;

	pr_debug("%s:\n", __func__);
	tvd_stop_generating(dev);

	spin_lock_irqsave(&dev->slock, flags);
	/* Release all active buffers */
	while (!list_empty(&dma_q->active)) {
		struct tvd_buffer *buf;
		buf = list_entry(dma_q->active.next, struct tvd_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		pr_debug("[%p/%d] done\n", buf, buf->vb.v4l2_buf.index);
	}
	spin_unlock_irqrestore(&dev->slock, flags);

	return 0;
}

static void tvd_lock(struct vb2_queue *vq)
{
	struct tvd_dev *dev = vb2_get_drv_priv(vq);
	mutex_lock(&dev->buf_lock);
}

static void tvd_unlock(struct vb2_queue *vq)
{
	struct tvd_dev *dev = vb2_get_drv_priv(vq);
	mutex_unlock(&dev->buf_lock);
}

static const struct vb2_ops tvd_video_qops = {
	.queue_setup		= queue_setup,
	.buf_prepare		= buffer_prepare,
	.buf_queue			= buffer_queue,
	.start_streaming	= start_streaming,
	.stop_streaming 	= stop_streaming,
	.wait_prepare		= tvd_unlock,
	.wait_finish		= tvd_lock,
};

/*
 * IOCTL vidioc handling
 */
static int vidioc_querycap(struct file *file, void  *priv,
				struct v4l2_capability *cap)
{
	struct tvd_dev *dev = video_drvdata(file);

	strcpy(cap->driver, "sunxi-tvd");
	strcpy(cap->card, "sunxi-tvd");
	strlcpy(cap->bus_info, dev->v4l2_dev.name, sizeof(cap->bus_info));

	cap->version = TVD_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE
				| V4L2_CAP_STREAMING
				| V4L2_CAP_READWRITE;

	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	struct tvd_fmt *fmt;

	if (f->index > ARRAY_SIZE(formats)-1)
		return -EINVAL;

	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static void __get_status(struct tvd_dev *dev, unsigned int *locked,
			unsigned int *system)
{
	int i = 0;

	if (dev->interface > 0) {
		/* ypbpr signal, search i/p */
		dev->interface = 1;
		for (i = 0; i < 2; i++) {
			__tvd_clk_init(dev);
			mdelay(200);

			tvd_get_status(dev->sel, locked, system);
			if (*locked)
				break;

			if (dev->interface < 2)
				dev->interface++;
		}
	} else if (dev->interface == 0) {
		/* cvbs signal */
		mdelay(200);
		tvd_get_status(dev->sel, locked, system);
	}
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct tvd_dev *dev = video_drvdata(file);
	u32 locked = 0, system = 2;

	f->fmt.pix.width = dev->width;
	f->fmt.pix.height = dev->height;

	__get_status(dev, &locked, &system);
	f->fmt.pix.priv = dev->interface;

	if (!locked) {
		pr_debug("%s: signal is not locked.\n", __func__);
		return -EAGAIN;
	} else {
		/* system: 1->pal, 0->ntsc */
		if (system == PAL) {
			f->fmt.pix.width = 720;
			f->fmt.pix.height = 576;
		} else if (system == NTSC) {
			f->fmt.pix.width = 720;
			f->fmt.pix.height = 480;
		} else {
			pr_err("system is not sure.\n");
		}
	}
	pr_debug("system = %d, w = %d, h = %d\n",
		system, f->fmt.pix.width, f->fmt.pix.height);
	return 0;
}

static struct tvd_fmt *__get_format(struct v4l2_format *f)
{
	struct tvd_fmt *fmt;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats); i++) {
		fmt = &formats[i];
		/* user defined struct raw_data: 5->pixelformat */
		if (fmt->fourcc == f->fmt.pix.pixelformat) {
			pr_debug("fourcc = %d\n", fmt->fourcc);
			break;
		}
	}
	if (i == ARRAY_SIZE(formats))
		return NULL;

	return &formats[i];
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct tvd_dev *dev = video_drvdata(file);
	int ret = 0;
	int used = 0;
	int value = 0, mode = 0;

	if (tvd_is_generating(dev)) {
		pr_err("%s device busy\n", __func__);
		return -EBUSY;
	}

	dev->fmt = __get_format(f);
	if (!dev->fmt) {
		pr_err("Fourcc format (0x%08x) invalid.\n",
			f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	/* tvd ypbpr now only support 720*480 & 720*576 */

	dev->width = f->fmt.pix.width;
	dev->height = f->fmt.pix.height;
	dev->fmt->field = V4L2_FIELD_NONE;
	if (dev->height == 576) {
		dev->system = PAL;
		/* To solve the problem of PAL signal is not well.
		 * Accoding to the hardware designer, tvd need 29.7M
		 * clk input on PAL system, so here adjust clk again.
		 * Before this modify, PAL and NTSC have the same
		 * frequency which is 27M.
		 */
		__tvd_clk_init(dev);
	} else {
		dev->system = NTSC;
	}
	pr_debug("interface=%d\n",dev->interface);
	pr_debug("system=%d\n",dev->system);
	pr_debug("format=%d\n",dev->format);
	pr_debug("width=%d\n",dev->width);
	pr_debug("height=%d\n",dev->height);

	__tvd_config(dev);

	/* agc function */
	ret = __tvd_fetch_sysconfig(dev->sel, (char *)"agc_auto_enable", &mode);
	if (ret)
		goto cagc;

	if (mode == 0) {
		/* manual mode */
		ret = __tvd_fetch_sysconfig(dev->sel,
						(char *)"agc_manual_value",
						&value);
		if (ret)
			goto cagc;
		tvd_agc_manual_config(dev->sel, (u32)value);
	} else {
		/* auto mode */
		tvd_agc_auto_config(dev->sel);
	}

cagc:
	ret = __tvd_fetch_sysconfig(dev->sel, (char *)"cagc_enable", &value);
	if (ret)
		goto _3d_fliter;
	tvd_cagc_config(dev->sel, (u32)value);

_3d_fliter:
	/* 3d fliter */
	__tvd_fetch_sysconfig(dev->sel, (char *)"fliter_used", &used);
	dev->fliter.used = used;

	if (dev->fliter.used) {
		mutex_lock(&fliter_lock);
		if (fliter_count < FLITER_NUM) {
			if (__tvd_3d_comp_mem_request(dev,
						(int)TVD_3D_COMP_BUFFER_SIZE)) {
				/* no mem support for 3d fliter */
				dev->fliter.used = 0;
				mutex_unlock(&fliter_lock);
				goto out;
			}
			fliter_count++;
		}
		tvd_3d_mode(dev->sel, 1, (u32)dev->fliter.phy_address);
		mutex_unlock(&fliter_lock);
	}
out:
	return 0;
}

static int vidioc_reqbufs(struct file *file, void *priv,
        struct v4l2_requestbuffers *p)
{
	struct tvd_dev *dev = video_drvdata(file);

	pr_debug("%s:\n", __func__);
	return vb2_reqbufs(&dev->vb_vidq, p);
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct tvd_dev *dev = video_drvdata(file);

	return vb2_querybuf(&dev->vb_vidq, p);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct tvd_dev *dev = video_drvdata(file);

	return vb2_qbuf(&dev->vb_vidq, p);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	int ret = 0;
	struct tvd_dev *dev = video_drvdata(file);

	pr_debug("%s:\n", __func__);
	ret = vb2_dqbuf(&dev->vb_vidq, p, file->f_flags & O_NONBLOCK);

	return ret;
}

#ifdef CONFIG_VIDEO_V4L1_COMPAT
static int vidiocgmbuf(struct file *file, void *priv, struct video_mbuf *mbuf)
{
	struct tvd_dev *dev = video_drvdata(file);

	return videobuf_cgmbuf(&dev->vb_vidq, mbuf, 8);
}
#endif

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct tvd_dev *dev = video_drvdata(file);
	struct tvd_dmaqueue *dma_q = &dev->vidq;
	struct tvd_buffer *buf;

	int ret = 0;

	mutex_lock(&dev->stream_lock);
	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = -EINVAL;
		goto streamon_unlock;
	}

	if (tvd_is_generating(dev)) {
		pr_err("stream has been already on\n");
		ret = -1;
		goto streamon_unlock;
	}

	/* Resets frame counters */
	dev->ms = 0;
	dev->jiffies = jiffies;

	dma_q->frame = 0;
	dma_q->ini_jiffies = jiffies;

	ret = vb2_streamon(&dev->vb_vidq, i);
	if (ret)
		goto streamon_unlock;

	buf = list_entry(dma_q->active.next,struct tvd_buffer, list);
	__tvd_set_addr(dev,buf);

	tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);
	tvd_irq_enable(dev->sel, TVD_IRQ_FRAME_END);
	tvd_capture_on(dev->sel);
	tvd_start_generating(dev);

streamon_unlock:
	mutex_unlock(&dev->stream_lock);

	return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct tvd_dev *dev = video_drvdata(file);
	struct tvd_dmaqueue *dma_q = &dev->vidq;
	int ret = 0;

	mutex_lock(&dev->stream_lock);

	pr_debug("%s:\n", __func__);
	if (!tvd_is_generating(dev)) {
		pr_err("%s: stream has been already off\n", __func__);
		ret = 0;
		goto streamoff_unlock;
	}

	tvd_stop_generating(dev);

	/* Resets frame counters */
	dev->ms = 0;
	dev->jiffies = jiffies;

	dma_q->frame = 0;
	dma_q->ini_jiffies = jiffies;

  	/* disable hardware */
	tvd_irq_disable(dev->sel, TVD_IRQ_FRAME_END);
	tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);
	tvd_capture_off(dev->sel);

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = -EINVAL;
		goto streamoff_unlock;
	}

	ret = vb2_streamoff(&dev->vb_vidq, i);
	if (ret!=0) {
		pr_err("%s: videobu_streamoff error!\n", __func__);
		goto streamoff_unlock;
	}

streamoff_unlock:
	mutex_unlock(&dev->stream_lock);

	return ret;
}

static int vidioc_enum_input(struct file *file, void *priv,
        struct v4l2_input *inp)
{
	if (inp->index > NUM_INPUTS-1) {
		pr_err("%s: input index invalid!\n", __func__);
		return -EINVAL;
	}

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct tvd_dev *dev = video_drvdata(file);

	pr_debug("%s:\n", __func__);
	*i = dev->input;
	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct tvd_dev *dev = video_drvdata(file);

	pr_debug("%s: input_num = %d\n", __func__, i);
	/* only one device */
	if (i > 0) {
		pr_err("%s: set input error!\n", __func__);
		return -EINVAL;
	}
	dev->input = i;

	return 0;
}

static int vidioc_g_parm(struct file *file, void *priv,
    struct v4l2_streamparm *parms)
{
	struct tvd_dev *dev = video_drvdata(file);

	pr_debug("%s\n", __func__);
	if(parms->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		parms->parm.capture.timeperframe.numerator=dev->fps.numerator;
		parms->parm.capture.timeperframe.denominator=dev->fps.denominator;
	}

	return 0;
}

static int vidioc_s_parm(struct file *file, void *priv,
			struct v4l2_streamparm *parms)
{
	struct tvd_dev *dev = video_drvdata(file);

	if (parms->parm.capture.capturemode != V4L2_MODE_VIDEO
		&& parms->parm.capture.capturemode != V4L2_MODE_IMAGE
		&& parms->parm.capture.capturemode != V4L2_MODE_PREVIEW)
		parms->parm.capture.capturemode = V4L2_MODE_PREVIEW;

	dev->capture_mode = parms->parm.capture.capturemode;

	return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *priv,
				  struct v4l2_frmsizeenum *fsize)
{
	int i;
	static const struct v4l2_frmsize_discrete sizes[] = {
		{
			.width = 720,
			.height = 480,
		},
		{
			.width = 720,
			.height = 576,
		},
	};

	/* there are two kinds of framesize*/
	if (fsize->index > 1)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(formats); i++)
		if (formats[i].fourcc == fsize->pixel_format)
			break;
	if (i == ARRAY_SIZE(formats)) {
		pr_err("format not found\n");
		return -EINVAL;
	}
	fsize->discrete.width = sizes[fsize->index].width;
	fsize->discrete.height = sizes[fsize->index].height;

	return 0;
}


static ssize_t tvd_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct tvd_dev *dev = video_drvdata(file);

	if(tvd_is_generating(dev)) {
		return vb2_read(&dev->vb_vidq, data, count, ppos,
					file->f_flags & O_NONBLOCK);
	} else {
		pr_err("%s: tvd is not generating!\n", __func__);
		return -EINVAL;
	}
}

static unsigned int tvd_poll(struct file *file, struct poll_table_struct *wait)
{
	struct tvd_dev *dev = video_drvdata(file);
	struct vb2_queue *q = &dev->vb_vidq;

	if(tvd_is_generating(dev)) {
		return vb2_poll(q, file, wait);
	} else {
		pr_err("%s: tvd is not generating!\n", __func__);
		return -EINVAL;
	}
}

static int __tvd_power_enable(struct regulator *regu, bool is_true)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(regu)) {
		pr_err("regulator is err.\n");
		return -EBUSY;
	}

	if (is_true)
		ret = regulator_enable(regu);
	else
		ret = regulator_disable(regu);

	return ret;
}

static int __tvd_gpio_request(struct gpio_config *pin_cfg)
{
	int ret = 0;
	char pin_name[32] = {0};
	u32 config;

	ret = gpio_request(pin_cfg->gpio, NULL);
	if (ret) {
		pr_err("tvd gpio(%d) request err!\n", pin_cfg->gpio);
		return -EBUSY;
	}

	sunxi_gpio_to_name(pin_cfg->gpio, pin_name);

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
	pin_config_set(SUNXI_PINCTRL, pin_name, config);
	if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD,
						pin_cfg->pull);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
	}
	if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV,
						pin_cfg->drv_level);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
	}
	if (pin_cfg->data != GPIO_DATA_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT,
						pin_cfg->data);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
	}

	return 0;
}

static int tvd_open(struct file *file)
{
	struct tvd_dev *dev = video_drvdata(file);
	int ret = -1;
	int i = 0;

	pr_debug("%s:\n", __func__);

	if (tvd_hot_plug)
		__tvd_auto_plug_disable(dev);

	if (tvd_is_opened(dev)) {
		pr_err("%s: device open busy\n", __func__);
		return -EBUSY;
	}
	dev->system = NTSC;

	/* gpio power, open only once */
	mutex_lock(&power_lock);
	if (!atomic_read(&gpio_power_enable_count)) {
		for (i = 0; i < atomic_read(&tvd_used_gpio_num); i++)
			ret = __tvd_gpio_request(&tvd_gpio_config[i]);
	}
	atomic_inc(&gpio_power_enable_count);

	/* pmu power */
	for (i = 0; i < atomic_read(&tvd_used_power_num); i++) {
		ret = __tvd_power_enable(regu[i], true);
		if (ret)
			pr_err("power(%s) enable failed.\n", &tvd_power[i][0]);
	}
	mutex_unlock(&power_lock);

	if (__tvd_clk_init(dev)) {
		pr_err("%s: clock init fail!\n", __func__);
	}
	ret = __tvd_clk_enable(dev);
	__tvd_init(dev);
	tvd_init(dev->sel, dev->interface);
	dev->input = 0;	/* default input null */

	tvd_start_opened(dev);

	return ret;
}

static int tvd_close(struct file *file)
{
	struct tvd_dev *dev = video_drvdata(file);
	int ret = 0;
	int i = 0;

	pr_debug("tvd_close\n");

	tvd_stop_generating(dev);
	__tvd_clk_disable(dev);

	vb2_queue_release(&dev->vb_vidq);
	tvd_stop_opened(dev);

	mutex_lock(&fliter_lock);
	if (fliter_count > 0 && dev->fliter.used) {
		__tvd_3d_comp_mem_free(dev);
		fliter_count--;
	}
	mutex_unlock(&fliter_lock);

	/* close pmu power */
	mutex_lock(&power_lock);
	for (i = 0; i < atomic_read(&tvd_used_power_num); i++) {
		ret = __tvd_power_enable(regu[i], false);
		if (ret)
			pr_err("power(%s) disable failed.\n", &tvd_power[i][0]);
	}

	if (atomic_dec_and_test(&gpio_power_enable_count)) {
		for (i = 0; i < atomic_read(&tvd_used_gpio_num); i++)
			gpio_free(tvd_gpio_config[i].gpio);
	}
	mutex_unlock(&power_lock);

	if (tvd_hot_plug)
		__tvd_auto_plug_enable(dev);

	pr_debug("tvd_close end\n");

	return ret;
}

static int tvd_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct tvd_dev *dev = video_drvdata(file);
	int ret;
	pr_debug("%s: mmap called, vma=0x%08lx\n",
		__func__, (unsigned long)vma);
	ret = vb2_mmap(&dev->vb_vidq, vma);
	pr_debug("%s: vma start=0x%08lx, size=%ld, ret=%d\n",
		__func__, (unsigned long)vma->vm_start,
		(unsigned long)vma->vm_end - (unsigned long)vma->vm_start, ret);

	return ret;
}

static int tvd_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	struct tvd_dev *dev = container_of(ctrl->handler, struct tvd_dev, ctrl_handler);
	struct v4l2_control c;

	memset((void*)&c, 0, sizeof(struct v4l2_control));
	c.id = ctrl->id;
	switch (c.id) {
	case V4L2_CID_BRIGHTNESS:
		c.value = tvd_get_luma(dev->sel);
		break;
	case V4L2_CID_CONTRAST:
		c.value = tvd_get_contrast(dev->sel);
		break;
	case V4L2_CID_SATURATION:
		c.value = tvd_get_saturation(dev->sel);
		break;
	}
	ctrl->val = c.value;

	return ret;
}

static int tvd_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct tvd_dev *dev = container_of(ctrl->handler, struct tvd_dev, ctrl_handler);
	int ret = 0;
	struct v4l2_control c;

	pr_debug("%s: %s set value: 0x%x\n", __func__, ctrl->name, ctrl->val);
	c.id = ctrl->id;
	c.value = ctrl->val;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		pr_debug("%s: V4L2_CID_BRIGHTNESS sel=%d, val=%d,\n",
			__func__, dev->sel, ctrl->val);
		tvd_set_luma(dev->sel, ctrl->val);
		break;
	case V4L2_CID_CONTRAST:
		pr_debug("%s: V4L2_CID_CONTRAST sel=%d, val=%d,\n",
			__func__, dev->sel, ctrl->val);
		tvd_set_contrast(dev->sel, ctrl->val);
		break;
	case V4L2_CID_SATURATION:
		pr_debug("%s: V4L2_CID_SATURATION sel=%d, val=%d,\n",
			__func__, dev->sel, ctrl->val);
		tvd_set_saturation(dev->sel, ctrl->val);
		break;
	}

	return ret;
}

static void __tvd_set_addr_special(struct tvd_dev *dev,
					struct tvd_buffer *buffer)
{
	unsigned long addr_org;
	unsigned int c_offset = 0;

	if (buffer == NULL || buffer->paddr == NULL) {
		pr_err("%s: vb_buf->priv is NULL!\n", __func__);
		return;
	}
	addr_org = (unsigned long)buffer->paddr;
	switch (dev->fmt->output_fmt) {
	case TVD_PL_YUV422:
	case TVD_PL_YUV420:
		c_offset = dev->width * dev->height;
		break;
	case TVD_MB_YUV420:
		c_offset = 0;
		break;
	default:
		break;
	}
	tvd_set_wb_addr(dev->sel, addr_org, addr_org + c_offset);
	pr_debug("%s: format:%d, addr_org = 0x%p, addr_org + c_offset = 0x%p\n",
		__func__, dev->format, (void *)addr_org,
		(void *)(addr_org + c_offset));
}

/* tvd device for special, 0 ~ tvd_count-1 */
int tvd_info_special(void)
{
	return tvd_count;
}
EXPORT_SYMBOL(tvd_info_special);

int tvd_open_special(int tvd_fd)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	int ret = -1;
	int i = 0;
	struct tvd_dmaqueue *active = &dev->vidq_special;
	struct tvd_dmaqueue *done = &dev->done_special;

	pr_debug("%s:\n", __func__);
	if (tvd_is_opened(dev)) {
		pr_err("%s: device open busy\n", __func__);
		return -EBUSY;
	}
	dev->system = NTSC;

	/* gpio power, open only once */
	mutex_lock(&power_lock);
	if (!atomic_read(&gpio_power_enable_count)) {
		for (i = 0; i < atomic_read(&tvd_used_gpio_num); i++)
			ret = __tvd_gpio_request(&tvd_gpio_config[i]);
	}
	atomic_inc(&gpio_power_enable_count);

	/* pmu power */
	for (i = 0; i < atomic_read(&tvd_used_power_num); i++) {
		ret = __tvd_power_enable(regu[i], true);
		if (ret)
			pr_err("power(%s) enable failed.\n", &tvd_power[i][0]);
	}
	mutex_unlock(&power_lock);

	INIT_LIST_HEAD(&active->active);
	INIT_LIST_HEAD(&done->active);
	if (__tvd_clk_init(dev))
		pr_err("%s: clock init fail!\n", __func__);

	ret = __tvd_clk_enable(dev);
	__tvd_init(dev);
	dev->input = 0;
	dev->special_active = 1;
	tvd_start_opened(dev);

	return ret;
}
EXPORT_SYMBOL(tvd_open_special);

int tvd_close_special(int tvd_fd)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	int ret = 0;
	int i = 0;
	struct tvd_dmaqueue *active = &dev->vidq_special;
	struct tvd_dmaqueue *done = &dev->done_special;

	pr_debug("%s:\n", __func__);
	tvd_stop_generating(dev);
	__tvd_clk_disable(dev);
	tvd_stop_opened(dev);

	INIT_LIST_HEAD(&active->active);
	INIT_LIST_HEAD(&done->active);
	dev->special_active = 0;

	/* close pmu power */
	mutex_lock(&power_lock);
	for (i = 0; i < atomic_read(&tvd_used_power_num); i++) {
		ret = __tvd_power_enable(regu[i], false);
		if (ret)
			pr_err("power(%s) disable failed.\n", &tvd_power[i][0]);
	}

	if (atomic_dec_and_test(&gpio_power_enable_count)) {
		for (i = 0; i < atomic_read(&tvd_used_gpio_num); i++)
			gpio_free(tvd_gpio_config[i].gpio);
	}
	mutex_unlock(&power_lock);

	return ret;
}
EXPORT_SYMBOL(tvd_close_special);

int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	int ret = 0;

	pr_debug("%s:\n", __func__);
	if (tvd_is_generating(dev)) {
		pr_err("%s device busy\n", __func__);
		return -EBUSY;
	}
	dev->fmt = __get_format(f);
	if (!dev->fmt) {
		pr_err("Fourcc format (0x%08x) invalid.\n",
			f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	dev->width = f->fmt.pix.width;
	dev->height = f->fmt.pix.height;
	dev->fmt->field = V4L2_FIELD_NONE;
	if (dev->height == 576) {
		dev->system = PAL;
		/* To solve the problem of PAL signal is not well.
		 * Accoding to the hardware designer, tvd need 29.7M
		 * clk input on PAL system, so here adjust clk again.
		 * Before this modify, PAL and NTSC have the same
		 * frequency which is 27M.
		 */
		__tvd_clk_init(dev);
	} else {
		dev->system = NTSC;
	}
	__tvd_config(dev);

	pr_debug("interface=%d\n", dev->interface);
	pr_debug("system=%d\n", dev->system);
	pr_debug("format=%d\n", dev->format);
	pr_debug("width=%d\n", dev->width);
	pr_debug("height=%d\n", dev->height);

	return ret;
}
EXPORT_SYMBOL(vidioc_s_fmt_vid_cap_special);

int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	u32 locked = 0, system = 2;

	f->fmt.pix.width = dev->width;
	f->fmt.pix.height = dev->height;

	__get_status(dev, &locked, &system);

	if (!locked) {
		pr_debug("%s: signal is not locked.\n", __func__);
		return -EAGAIN;
	} else {
		/* system: 1->pal, 0->ntsc */
		if (system == PAL) {
			f->fmt.pix.width = 720;
			f->fmt.pix.height = 576;
		} else if (system == NTSC) {
			f->fmt.pix.width = 720;
			f->fmt.pix.height = 480;
		} else {
			pr_err("system is not sure.\n");
		}
	}

	pr_debug("system = %d, w = %d, h = %d\n",
		system, f->fmt.pix.width, f->fmt.pix.height);

	return 0;
}
EXPORT_SYMBOL(vidioc_g_fmt_vid_cap_special);

int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf)
{
	int ret = 0;
	unsigned long flags = 0;
	struct tvd_dev *dev = tvd[tvd_fd];
	struct tvd_dmaqueue *done = &dev->done_special;

	spin_lock_irqsave(&dev->slock, flags);
	if (!list_empty(&done->active)) {
		*buf = list_first_entry(&done->active, struct tvd_buffer, list);
		list_del(&((*buf)->list));
		(*buf)->state = VB2_BUF_STATE_DEQUEUED;
	} else {
		ret = -1;
	}
	spin_unlock_irqrestore(&dev->slock, flags);

	return ret;
}
EXPORT_SYMBOL(dqbuffer_special);

int qbuffer_special(int tvd_fd, struct tvd_buffer *buf)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	struct tvd_dmaqueue *vidq = &dev->vidq_special;
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&dev->slock, flags);
	list_add_tail(&buf->list, &vidq->active);
	buf->state = VB2_BUF_STATE_QUEUED;
	spin_unlock_irqrestore(&dev->slock, flags);

	return ret;
}
EXPORT_SYMBOL(qbuffer_special);

int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	struct tvd_dmaqueue *dma_q = &dev->vidq_special;
	struct tvd_buffer *buf = NULL;
	int ret = 0;

	pr_debug("%s:\n", __func__);
	mutex_lock(&dev->stream_lock);
	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = -EINVAL;
		goto streamon_unlock;
	}
	if (tvd_is_generating(dev)) {
		pr_err("stream has been already on\n");
		ret = -1;
		goto streamon_unlock;
	}
	dev->ms = 0;
	dev->jiffies = jiffies;
	dma_q->frame = 0;
	dma_q->ini_jiffies = jiffies;

	if (!list_empty(&dma_q->active)) {
		buf = list_entry(dma_q->active.next, struct tvd_buffer, list);
	} else {
		pr_err("stream on, but no buffer now.\n");
		goto streamon_unlock;
	}

	__tvd_set_addr_special(dev, buf);
	tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);
	tvd_irq_enable(dev->sel, TVD_IRQ_FRAME_END);
	tvd_capture_on(dev->sel);
	tvd_start_generating(dev);

streamon_unlock:
	mutex_unlock(&dev->stream_lock);

	return ret;
}
EXPORT_SYMBOL(vidioc_streamon_special);

int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i)
{
	struct tvd_dev *dev = tvd[tvd_fd];
	struct tvd_dmaqueue *dma_q = &dev->vidq_special;
	struct tvd_dmaqueue *donelist = &dev->done_special;
	struct tvd_buffer *buffer;
	unsigned long flags = 0;
	int ret = 0;

	mutex_lock(&dev->stream_lock);

	pr_debug("%s:\n", __func__);
	if (!tvd_is_generating(dev)) {
		pr_err("%s: stream has been already off\n", __func__);
		ret = 0;
		goto streamoff_unlock;
	}
	tvd_stop_generating(dev);
	dev->ms = 0;
	dev->jiffies = jiffies;
	dma_q->frame = 0;
	dma_q->ini_jiffies = jiffies;

	tvd_irq_disable(dev->sel, TVD_IRQ_FRAME_END);
	tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);
	tvd_capture_off(dev->sel);
	spin_lock_irqsave(&dev->slock, flags);

	while (!list_empty(&dma_q->active)) {
		buffer = list_first_entry(&dma_q->active,
						struct tvd_buffer, list);
		list_del(&buffer->list);
		list_add(&buffer->list, &donelist->active);
	}

	spin_unlock_irqrestore(&dev->slock, flags);

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = -EINVAL;
		goto streamoff_unlock;
	}
	if (ret != 0) {
		pr_err("%s: videobu_streamoff error!\n", __func__);
		goto streamoff_unlock;
	}

streamoff_unlock:
	mutex_unlock(&dev->stream_lock);

	return ret;
}
EXPORT_SYMBOL(vidioc_streamoff_special);

static void (*tvd_buffer_done)(int tvd_fd);
void tvd_register_buffer_done_callback(void *func)
{
	pr_debug("%s\n", __func__);
	tvd_buffer_done = func;
}
EXPORT_SYMBOL(tvd_register_buffer_done_callback);

static irqreturn_t tvd_isr_special(int irq, void *priv)
{
	struct tvd_buffer *buf;
	unsigned long flags;
	struct tvd_dev *dev = (struct tvd_dev *)priv;
	struct tvd_dmaqueue *dma_q = &dev->vidq_special;
	struct tvd_dmaqueue *done = &dev->done_special;
	int need_callback = 0;

	if (tvd_is_generating(dev) == 0) {
		tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);
		return IRQ_HANDLED;
	}

	spin_lock_irqsave(&dev->slock, flags);

	if (0 == dev->first_flag) {
		dev->first_flag = 1;
		goto set_next_addr;
	}
	if (list_empty(&dma_q->active)
		|| dma_q->active.next->next == (&dma_q->active)) {
		pr_debug("No active queue to serve\n");
		goto unlock;
	}

	buf = list_entry(dma_q->active.next, struct tvd_buffer, list);
	list_del(&buf->list);
	dev->ms += jiffies_to_msecs(jiffies - dev->jiffies);
	dev->jiffies = jiffies;
	list_add_tail(&buf->list, &done->active);
	need_callback = 1;

	if (list_empty(&dma_q->active)) {
		pr_debug("%s: No more free frame\n", __func__);
		goto unlock;
	}
	if ((&dma_q->active) == dma_q->active.next->next) {
		pr_debug("No more free frame on next time\n");
		goto unlock;
	}

set_next_addr:
	buf = list_entry(dma_q->active.next->next, struct tvd_buffer, list);
	__tvd_set_addr_special(dev, buf);

unlock:
	spin_unlock(&dev->slock);

	if (need_callback && tvd_buffer_done)
		tvd_buffer_done(dev->id);

	tvd_irq_status_clear(dev->sel, TVD_IRQ_FRAME_END);

	return IRQ_HANDLED;
}

/* ------------------------------------------------------------------
	File operations for the device
   ------------------------------------------------------------------*/

static const struct v4l2_ctrl_ops tvd_ctrl_ops = {
	.g_volatile_ctrl = tvd_g_volatile_ctrl,
	.s_ctrl = tvd_s_ctrl,
};

static const struct v4l2_file_operations tvd_fops = {
	.owner          = THIS_MODULE,
	.open           = tvd_open,
	.release        = tvd_close,
	.read           = tvd_read,
	.poll           = tvd_poll,
	.ioctl          = video_ioctl2,

#ifdef CONFIG_COMPAT
	//.compat_ioctl32 = tvd_compat_ioctl32,
#endif
	.mmap           = tvd_mmap,
};

static const struct v4l2_ioctl_ops tvd_ioctl_ops = {
	.vidioc_querycap          = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_enum_framesizes   = vidioc_enum_framesizes,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs           = vidioc_reqbufs,
	.vidioc_querybuf          = vidioc_querybuf,
	.vidioc_qbuf              = vidioc_qbuf,
	.vidioc_dqbuf             = vidioc_dqbuf,
	.vidioc_enum_input        = vidioc_enum_input,
	.vidioc_g_input           = vidioc_g_input,
	.vidioc_s_input           = vidioc_s_input,
	.vidioc_streamon          = vidioc_streamon,
	.vidioc_streamoff         = vidioc_streamoff,
	.vidioc_g_parm            = vidioc_g_parm,
	.vidioc_s_parm            = vidioc_s_parm,
#ifdef CONFIG_VIDEO_V4L1_COMPAT
	.vidiocgmbuf              = vidiocgmbuf,
#endif
};

static struct video_device tvd_template[] = {
	[0] = {
		.name       = "tvd_0",
		.fops       = &tvd_fops,
		.ioctl_ops  = &tvd_ioctl_ops,
		.release    = video_device_release,
	},
    [1] = {
        .name       = "tvd_1",
        .fops       = &tvd_fops,
        .ioctl_ops  = &tvd_ioctl_ops,
        .release    = video_device_release,
    },
    [2] = {
        .name       = "tvd_2",
        .fops       = &tvd_fops,
        .ioctl_ops  = &tvd_ioctl_ops,
        .release    = video_device_release,
    },
    [3] = {
        .name       = "tvd_3",
        .fops       = &tvd_fops,
        .ioctl_ops  = &tvd_ioctl_ops,
        .release    = video_device_release,
    },
};

static int __tvd_init_controls(struct v4l2_ctrl_handler *hdl)
{

	unsigned int ret = 0;

	v4l2_ctrl_handler_init(hdl, 4);
	v4l2_ctrl_new_std(hdl, &tvd_ctrl_ops, V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(hdl, &tvd_ctrl_ops, V4L2_CID_CONTRAST, 0, 128, 1, 0);
	v4l2_ctrl_new_std(hdl, &tvd_ctrl_ops, V4L2_CID_SATURATION, -4, 4, 1, 0);

	if (hdl->error) {
		pr_err("%s: hdl init err!\n", __func__);
		ret = hdl->error;
        v4l2_ctrl_handler_free(hdl);
	}
	return ret;
}

static int __tvd_fetch_sysconfig(int sel, char *sub_name, int value[])
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	int ret = 0;

	len = sprintf(compat, "allwinner,sunxi-tvd%d", sel);
	if (len > 32)
		pr_err("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		pr_err("of_find_compatible_node %s fail\n", compat);
		return -EINVAL;
	}

	if (of_property_read_u32_array(node, sub_name, value, 1)) {
		pr_err("of_property_read_u32_array %s.%s fail\n", compat, sub_name);
		return -EINVAL;
	}

	return ret;
}

static int __jude_config(struct tvd_dev *dev)
{
	int ret = 0;
	int id = dev->id;

	if (id > 3 || id < 0) {
		pr_err("%s: id is wrong!\n", __func__);
		return -ENODEV;
	}

	/* first set sel as id */
	dev->sel = id;
	pr_debug("%s: sel = %d.\n", __func__, dev->sel);

	ret = __tvd_fetch_sysconfig(id, (char *)"tvd_used", &tvd_status[id].tvd_used);
	if (ret) {
		pr_err("%s: fetch tvd_used%d err!", __func__, id);
		return -EINVAL;
	}
	if (!tvd_status[id].tvd_used) {
		pr_debug("%s: tvd_status[%d].used is null.\n", __func__, id);
		return -ENODEV;
	}

	ret = __tvd_fetch_sysconfig(id, (char *)"tvd_if", &tvd_status[id].tvd_if);
	if (ret) {
		pr_err("%s: fetch tvd_if%d err!", __func__, id);
		return -EINVAL;
	}
	dev->interface = tvd_status[id].tvd_if;

	if (id > 0) {
		if (tvd_status[0].tvd_used && tvd_status[0].tvd_if > 0 ) {
		/* when tvd0 used and was configed as ypbpr,can not use tvd1,2 */
			if (id == 1 || id == 2) {
				return -ENODEV;
			} else if (id == 3) {
				/* reset id as 1, for video4 ypbpr,video5 cvbs*/
				dev->id = 1;
				return 0;
			}
		} else {
			return 0;
		}
	}

	return 0;

}

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)

static char switch_lock_name[20];
static char switch_system_name[20];
static struct switch_dev switch_lock[TVD_MAX];
static struct switch_dev switch_system[TVD_MAX];
static struct task_struct *tvd_task;

static int __tvd_auto_plug_init(struct tvd_dev *dev)
{
	int ret = 0;

	snprintf(switch_lock_name, sizeof(switch_lock_name), "tvd_lock%d",
		dev->sel);

	switch_lock[dev->sel].name = switch_lock_name;
	ret = switch_dev_register(&switch_lock[dev->sel]);


	snprintf(switch_system_name, sizeof(switch_system_name), "tvd_system%d",
		dev->sel);

	switch_system[dev->sel].name = switch_system_name;
	ret = switch_dev_register(&switch_system[dev->sel]);

	return ret;
}

static void __tvd_auto_plug_exit(struct tvd_dev *dev)
{
	switch_dev_unregister(&switch_lock[dev->sel]);
	switch_dev_unregister(&switch_system[dev->sel]);
}

static int __tvd_detect_thread(void *parg)
{
	s32 i = 0;
	u32 locked = 0;
	u32 system = 2;
	static u32 systems[TVD_MAX];
	static bool hpd[TVD_MAX];


	for (i = 0; i < TVD_MAX; i++) {
		systems[i] = NONE;
		hpd[i] = false;
	}

	for (;;) {
		if (kthread_should_stop())
			break;

		for (i = 0; i < tvd_count; i++) {
			tvd_get_status(i, &locked, &system);
			if (hpd[i] != locked) {
				pr_debug("reverse hpd=%d, i = %d\n", locked, i);
				hpd[i] = locked;
				switch_set_state(&switch_lock[i], locked);
			}
			if (systems[i] != system) {
				pr_debug("system = %d, i = %d\n", system, i);
				systems[i] = system;
				switch_set_state(&switch_system[i], system);
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(50);
	}

	return 0;
}

static int __tvd_auto_plug_enable(struct tvd_dev *dev)
{
	int ret = 0;
	int i = 0;

	dev->system = NTSC;

	/* gpio power, open only once */
	mutex_lock(&power_lock);
	if (!atomic_read(&gpio_power_enable_count)) {
		for (i = 0; i < atomic_read(&tvd_used_gpio_num); i++)
			ret = __tvd_gpio_request(&tvd_gpio_config[i]);
	}
	atomic_inc(&gpio_power_enable_count);

	/* pmu power */
	for (i = 0; i < atomic_read(&tvd_used_power_num); i++) {
		ret = __tvd_power_enable(regu[i], true);
		if (ret)
			pr_err("power(%s) enable failed.\n", &tvd_power[i][0]);
	}
	mutex_unlock(&power_lock);

	if (__tvd_clk_init(dev))
		pr_err("%s: clock init fail!\n", __func__);

	ret = __tvd_clk_enable(dev);
	__tvd_init(dev);
	tvd_init(dev->sel, dev->interface);

	/* Set system as NTSC */
	dev->width = 720;
	dev->height = 480;
	dev->fmt = &formats[0];

	ret = __tvd_config(dev);

	/* enable detect thread */
	if (!tvd_task) {
		tvd_task = kthread_create(__tvd_detect_thread, (void *)0,
						"tvd detect");
		if (IS_ERR(tvd_task)) {
			s32 err = 0;
			err = PTR_ERR(tvd_task);
			tvd_task = NULL;
			return err;
		}
		wake_up_process(tvd_task);
	}

	return ret;
}

static int __tvd_auto_plug_disable(struct tvd_dev *dev)
{
	int ret = 0;
	int i = 0;

	__tvd_clk_disable(dev);

	/* close pmu power */
	mutex_lock(&power_lock);
	for (i = 0; i < atomic_read(&tvd_used_power_num); i++) {
		ret = __tvd_power_enable(regu[i], false);
		if (ret)
			pr_err("power(%s) disable failed.\n", &tvd_power[i][0]);
	}

	if (atomic_dec_and_test(&gpio_power_enable_count)) {
		for (i = 0; i < atomic_read(&tvd_used_gpio_num); i++)
			gpio_free(tvd_gpio_config[i].gpio);
	}
	mutex_unlock(&power_lock);

	return ret;
}

#else
static int __tvd_auto_plug_init(struct tvd_dev *dev)
{
	return 0;
}

static void __tvd_auto_plug_exit(struct tvd_dev *dev)
{

}

static int __tvd_auto_plug_enable(struct tvd_dev *dev)
{
	pr_warn("there is no switch class for tvd\n");
	return 0;
}

static int __tvd_auto_plug_disable(struct tvd_dev *dev)
{
	return 0;
}
#endif

static void __iomem  *tvd_top;
struct clk* tvd_clk_top;

static int __tvd_probe_init(int sel, struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct tvd_dev *dev;
	int ret = 0;
	struct video_device *vfd;
	struct vb2_queue *q;

	pr_debug("%s: \n", __func__);

	/*request mem for dev*/
	dev = kzalloc(sizeof(struct tvd_dev), GFP_KERNEL);
	if (!dev) {
		pr_err("request dev mem failed!\n");
		return  -ENOMEM;
	}

	pdev->id = sel;
	if (pdev->id < 0) {
		pr_err("TVD failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}

	dev->id = pdev->id;
	dev->sel = sel;
	dev->pdev = pdev;
	dev->generating = 0;
	dev->opened = 0;

	spin_lock_init(&dev->slock);

	/* fetch sysconfig,and judge support */
	ret = __jude_config(dev);
	if (ret) {
		pr_err("%s:tvd%d is not used by sysconfig.\n", __func__, dev->id);
		ret = -EINVAL;
		goto freedev;
	}

	tvd[dev->id] = dev;
	tvd_count++;
	dev->irq = irq_of_parse_and_map(np, 0);
	if (dev->irq <= 0) {
		pr_err("failed to get IRQ resource\n");
		ret = -ENXIO;
		goto iomap_tvd_err;
	}

	dev->regs_tvd = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(dev->regs_tvd)) {
		dev_err(&pdev->dev, "unable to tvd registers\n");
		ret = -EINVAL;
		goto iomap_top_err;
	}
	dev->regs_top = tvd_top;
	dev->clk_top = tvd_clk_top;

	/* register irq */
	ret = request_irq(dev->irq, tvd_isr, IRQF_DISABLED, pdev->name, dev);

	/* get tvd clk ,name fix */
	dev->clk = of_clk_get(np, 0);//fix
	if (IS_ERR_OR_NULL(dev->clk)) {
		pr_err("get tvd clk error!\n");
		goto iomap_tvd_err;
	}

	/* register v4l2 device */
	sprintf(dev->v4l2_dev.name, "tvd_v4l2_dev%d", dev->id);
	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret) {
		pr_err("Error registering v4l2 device\n");
		goto iomap_tvd_err;
	}

	ret = __tvd_init_controls(&dev->ctrl_handler);
	if (ret) {
		pr_err("Error v4l2 ctrls new!!\n");
		goto v4l2_register_err;
	}
	dev->v4l2_dev.ctrl_handler = &dev->ctrl_handler;

	dev_set_drvdata(&dev->pdev->dev, dev);
	pr_info("%s: v4l2 subdev register.\n", __func__);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_enable(&dev->pdev->dev);
#endif

	vfd = video_device_alloc();
	if (!vfd) {
		pr_err("%s: Error video_device_alloc!\n", __func__);
		goto v4l2_register_err;
	}
	*vfd = tvd_template[dev->id];
	vfd->v4l2_dev = &dev->v4l2_dev;

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, dev->id + 4);
	if (ret < 0) {
		pr_err("Error video_register_device!!\n");
		goto video_device_alloc_err;
	}

	video_set_drvdata(vfd, dev);

	list_add_tail(&dev->devlist, &devlist); //use this for what?

	dev->vfd = vfd;
	pr_info("V4L2 tvd device registered as %s\n",video_device_node_name(vfd));

	/* Initialize videobuf2 queue as per the buffer type */
	dev->alloc_ctx = vb2_dma_contig_init_ctx(&dev->pdev->dev);
	if (IS_ERR(dev->alloc_ctx)) {
		pr_err("Failed to get the context\n");
		goto video_device_register_err;
	}

	/* initialize queue */
	q = &dev->vb_vidq;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = dev;
	q->buf_struct_size = sizeof(struct tvd_buffer);
	q->ops = &tvd_video_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	ret = vb2_queue_init(q);
	if (ret) {
		pr_err("vb2_queue_init failed\n");
		vb2_dma_contig_cleanup_ctx(dev->alloc_ctx);
		goto video_device_register_err;
	}

	INIT_LIST_HEAD(&dev->vidq.active);
	ret = sysfs_create_group(&dev->pdev->dev.kobj, &tvd_attribute_group[dev->id]);
	if (ret) {
		pr_err("sysfs_create failed\n");
		goto vb2_queue_err;
	}

	mutex_init(&dev->stream_lock);
	mutex_init(&dev->opened_lock);
	mutex_init(&dev->buf_lock);

	if (tvd_hot_plug) {
		__tvd_auto_plug_init(dev);
		__tvd_auto_plug_enable(dev);
	}

	return 0;

vb2_queue_err:
	vb2_queue_release(q);

video_device_register_err:
	v4l2_device_unregister(&dev->v4l2_dev);

video_device_alloc_err:
	video_device_release(vfd);

v4l2_register_err:
	v4l2_device_unregister(&dev->v4l2_dev);

iomap_tvd_err:
	iounmap((char __iomem *)dev->regs_tvd);

iomap_top_err:
	iounmap((char __iomem *)dev->regs_top);

freedev:
	kfree(dev);

	return ret;
}

static int tvd_probe(struct platform_device *pdev)
{
	int ret = 0, i = 0;
	unsigned int tvd_num = 0;
	struct device_node *sub_tvd = NULL;
	struct platform_device *sub_pdev = NULL;
	struct device_node *np = pdev->dev.of_node;
	char sub_name[32] = {0};
	const char *str;

	mutex_init(&power_lock);
	mutex_init(&fliter_lock);
	tvd_top = of_iomap(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(tvd_top)) {
		dev_err(&pdev->dev, "unable to map tvd top registers\n");
		ret = -EINVAL;
		goto out;
	}

	tvd_clk_top = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(tvd_clk_top)) {
		pr_err("get tvd clk error!\n");
		goto iomap_tvd_err;
	}

	of_property_read_u32(pdev->dev.of_node, "tvd_hot_plug", &tvd_hot_plug);

	for (i = 0; i < TVD_MAX_POWER_NUM; i++) {
		snprintf(sub_name, sizeof(sub_name), "tvd_power%d", i);
		if (!of_property_read_string(pdev->dev.of_node, sub_name,
						&str)) {
			atomic_inc(&tvd_used_power_num);
			memcpy(&tvd_power[i][0], str, strlen(str)+1);
			regu[i] = regulator_get(NULL, &tvd_power[i][0]);
		}
	}

	for (i = 0; i < TVD_MAX_GPIO_NUM; i++) {
		int gpio;
		snprintf(sub_name, sizeof(sub_name), "tvd_gpio%d", i);
		gpio = of_get_named_gpio_flags(pdev->dev.of_node, sub_name, 0,
				(enum of_gpio_flags *)&tvd_gpio_config[i]);
		if (gpio_is_valid(gpio))
			atomic_inc(&tvd_used_gpio_num);
	}

	if (of_property_read_u32(pdev->dev.of_node, "tvd-number", &tvd_num) < 0) {
		dev_err(&pdev->dev, "unable to get tvd-number, force to one!\n");
		tvd_num = 1;
	}

	for (i = 0; i < tvd_num; i++) {
		sub_tvd = of_parse_phandle(pdev->dev.of_node, "tvds", i);
		sub_pdev = of_find_device_by_node(sub_tvd);
		if (!sub_pdev) {
			dev_err(&pdev->dev, "fail to find device for tvd%d!\n", i);
			continue;
		}
		if (sub_pdev) {
			ret = __tvd_probe_init(i, sub_pdev);
			if(ret!=0) {
				/* one tvd may init fail because of the sysconfig */
				pr_debug("tvd%d init is failed\n", i);
				ret = 0;
			}
		}
	}

iomap_tvd_err:

	iounmap((char __iomem *)tvd_top);

out:
	return ret;
}

static int tvd_release(void)/*fix*/
{
	struct tvd_dev *dev;
	struct list_head *list;

	pr_debug("%s: \n", __func__);
	while (!list_empty(&devlist)) {
		list = devlist.next;
		list_del(list);
		dev = list_entry(list, struct tvd_dev, devlist);
		kfree(dev);
	}
	pr_debug("tvd_release ok!\n");

	return 0;
}

static int tvd_remove(struct platform_device *pdev)
{
	struct tvd_dev *dev=(struct tvd_dev *)dev_get_drvdata(&(pdev)->dev);
	int i = 0;

	free_irq(dev->irq, dev);
	__tvd_clk_disable(dev);
	iounmap(dev->regs_top);
	iounmap(dev->regs_tvd);
	mutex_destroy(&dev->stream_lock);
	mutex_destroy(&dev->opened_lock);
	mutex_destroy(&dev->buf_lock);
	sysfs_remove_group(&dev->pdev->dev.kobj, &tvd_attribute_group[dev->id]);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&dev->pdev->dev);
#endif

	video_unregister_device(dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);
	v4l2_ctrl_handler_free(&dev->ctrl_handler);
	vb2_dma_contig_cleanup_ctx(dev->alloc_ctx);

	for (i = 0; i < atomic_read(&tvd_used_power_num); i++)
		regulator_put(regu[i]);

	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int tvd_runtime_suspend(struct device *d)
{
	return 0;
}

static int tvd_runtime_resume(struct device *d)
{
	return 0;
}

static int tvd_runtime_idle(struct device *d)
{
	if(d) {
		pm_runtime_mark_last_busy(d);
		pm_request_autosuspend(d);
	} else {
		pr_err("%s, tvd device is null\n", __func__);
	}

	return 0;
}
#endif

static int tvd_suspend(struct device *d)
{
	if (tvd_task) {
		if (!kthread_stop(tvd_task))
			tvd_task = NULL;
	}

	return 0;
}

static int tvd_resume(struct device *d)
{
	if (!tvd_task) {
		tvd_task = kthread_create(__tvd_detect_thread, (void *)0,
						"tvd detect");
		if (IS_ERR(tvd_task)) {
			s32 err = 0;
			err = PTR_ERR(tvd_task);
			tvd_task = NULL;
			return err;
		}
		wake_up_process(tvd_task);
	}

	return 0;
}

static void tvd_shutdown(struct platform_device *pdev)
{

}

static const struct dev_pm_ops tvd_runtime_pm_ops =
{
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend	= tvd_runtime_suspend,
	.runtime_resume 	= tvd_runtime_resume,
	.runtime_idle		= tvd_runtime_idle,
#endif
	.suspend    	= tvd_suspend,
	.resume     	= tvd_resume,
};

static const struct of_device_id sunxi_tvd_match[] = {
	{ .compatible = "allwinner,sunxi-tvd", },
	{},
};

static struct platform_driver tvd_driver = {
	.probe    = tvd_probe,
	.remove   = tvd_remove,
	.shutdown = tvd_shutdown,
	.driver = {
		.name   = TVD_MODULE_NAME,
		.owner  = THIS_MODULE,
        .of_match_table = sunxi_tvd_match,
        .pm     = &tvd_runtime_pm_ops,
	}
};

static int __init tvd_module_init(void)
{
	int ret;

	pr_info("Welcome to tv decoder driver\n");

	/*add sysconfig judge,if no use,return.*/
	ret = platform_driver_register(&tvd_driver);
	if (ret) {
		pr_err("platform driver register failed\n");
		return ret;
	}
	pr_info("tvd_init end\n");

	return 0;
}

static void __exit tvd_module_exit(void)
{
	int i = 0;

	pr_info("tvd_exit\n");
	if (tvd_hot_plug) {
		for (i = 0; i < tvd_count; i++)
			__tvd_auto_plug_exit(tvd[i]);
	}

	tvd_release();
	platform_driver_unregister(&tvd_driver);
}

subsys_initcall_sync(tvd_module_init);
module_exit(tvd_module_exit);

MODULE_AUTHOR("zengqi");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("tvd driver for sunxi");

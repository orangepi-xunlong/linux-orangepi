/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs g2d driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "g2d_driver_i.h"
#include "linux/hwspinlock.h"
#include "linux/pm_runtime.h"
#include "linux/pm_domain.h"
#include "linux/hwspinlock.h"
#include <uapi/linux/sunxi-g2d.h>

#define CREATE_TRACE_POINTS
#include "../g2d_trace.h"

#if IS_ENABLED(CONFIG_G2D_SYNCFENCE)
extern int syncfence_init(void);
extern void syncfence_exit(void);
#endif

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
#define PM_ARRAY_SIZE  8
#endif

#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
static int g2d_hwspinlock_id = 23;
struct hwspinlock *hwlock;
unsigned long hwspinlock_flag;
#endif

/* alloc based on 4K byte */
#define G2D_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)
#define ALLOC_USING_DMA
static struct info_mem g2d_mem[MAX_G2D_MEM_INDEX];
static int g2d_mem_sel;
static enum g2d_scan_order scan_order;
static struct mutex global_lock;

static struct class *g2d_class;
static struct cdev *g2d_cdev;
static dev_t devid;
static struct device *g2d_dev;
static struct device *dmabuf_dev;
__g2d_drv_t g2d_ext_hd;
__g2d_info_t para;

u32 dbg_info;
u32 time_info;

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long long id;
};

#if !defined(CONFIG_OF)
static struct resource g2d_resource[2] = {

	[0] = {
	       .start = SUNXI_MP_PBASE,
	       .end = SUNXI_MP_PBASE + 0x000fffff,
	       .flags = IORESOURCE_MEM,
	       .resource_size = 0x00100000,
	       },
	[1] = {
	       .start = INTC_IRQNO_DE_MIX,
	       .end = INTC_IRQNO_DE_MIX,
	       .flags = IORESOURCE_IRQ,
	       .resource_size = 0x1,
	       },
};
#endif

static ssize_t g2d_debug_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "debug=%d\n", dbg_info);
}

static ssize_t g2d_debug_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "1", 1) == 0)
		dbg_info = 1;
	else if (strncasecmp(buf, "0", 1) == 0)
		dbg_info = 0;
	else
		G2D_WARN("Illegal input, please input 1 or 0 to open "
			 "or close debug_func!\n");

	return count;
}

static DEVICE_ATTR(debug, 0660,
		   g2d_debug_show, g2d_debug_store);

static ssize_t g2d_func_runtime_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "func_runtime=%d\n", time_info);
}

static ssize_t g2d_func_runtime_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "1", 1) == 0)
		time_info = 1;
	else if (strncasecmp(buf, "0", 1) == 0)
		time_info = 0;
	else
		G2D_WARN("Illegal input, please input 1 or 0 to open "
			 "or close timestamp_func!\n");

	return count;
}

static DEVICE_ATTR(func_runtime, 0660,
		   g2d_func_runtime_show, g2d_func_runtime_store);

static ssize_t g2d_standby_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "suspend", 7) == 0) {
		G2D_INFO("self suspend\n");
		g2d_suspend(NULL);
	} else if (strncasecmp(buf, "resume", 6) == 0) {
		G2D_INFO("self resume\n");
		g2d_resume(NULL);
	} else
		G2D_WARN("Illegal input, please input 7 or 6 to supend "
			 "or resume g2d!\n");

	return count;
}

static DEVICE_ATTR(standby, 0660, NULL, g2d_standby_store);

static struct attribute *g2d_attributes[] = {
	&dev_attr_debug.attr,
	&dev_attr_func_runtime.attr,
	&dev_attr_standby.attr,
	NULL
};

static struct attribute_group g2d_attribute_group = {
	.name = "attr",
	.attrs = g2d_attributes
};

static int g2d_clock_prepare(const __g2d_info_t *info)
{
	int ret = 0;
	if (info->bus_clk) {
		ret |=  clk_prepare(info->bus_clk);
	}
	if (info->clk) {
		if (info->clk_parent) {
			clk_set_parent(info->clk, info->clk_parent);
		}
		ret |= clk_prepare(info->clk);
	}
	if (info->mbus_clk) {
		ret |= clk_prepare(info->mbus_clk);
	}
	if (ret != 0)
		G2D_ERR("clock prepare error\n");

	return ret;
}

static int g2d_clock_enable(const __g2d_info_t *info)
{
	int ret = 0;

	if (info->reset) {
		ret = reset_control_deassert(info->reset);
		if (ret != 0) {
			G2D_ERR("deassert error\n");
			return ret;
		}
	}
	if (info->bus_clk)
		ret |=  clk_enable(info->bus_clk);
	if (info->clk)
		ret |= clk_enable(info->clk);
	if (info->mbus_clk)
		ret |= clk_enable(info->mbus_clk);
	if (ret != 0)
		G2D_ERR("clock enable error\n");
	return ret;
}

static int g2d_clock_unprepare(const __g2d_info_t *info)
{
	if (info->clk)
		clk_unprepare(info->clk);
	if (info->bus_clk)
		clk_unprepare(info->bus_clk);
	if (info->mbus_clk)
		clk_unprepare(info->mbus_clk);
	return 0;
}

static int g2d_clock_disable(const __g2d_info_t *info)
{
	if (info->clk)
		clk_disable(info->clk);
	if (info->bus_clk)
		clk_disable(info->bus_clk);
	if (info->mbus_clk)
		clk_disable(info->mbus_clk);
	if (info->reset)
		reset_control_assert(info->reset);
	return 0;
}

#ifdef G2D_V2X_SUPPORT
static int g2d_dma_map(int fd, struct dmabuf_item *item)
{
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	G2D_TRACE_BEGIN("g2d_dma_map");
	if (fd < 0) {
		G2D_WARN("dma_buf_id(%d) is invalid\n", fd);
		goto exit;
	}
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		G2D_WARN("dma_buf_get failed, fd=%d\n", fd);
		goto exit;
	}

	attachment = dma_buf_attach(dmabuf, dmabuf_dev);
	if (IS_ERR(attachment)) {
		G2D_WARN("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(sgt)) {
		G2D_WARN("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	item->fd = fd;
	item->buf = dmabuf;
	item->sgt = sgt;
	item->attachment = attachment;
	item->dma_addr = sg_dma_address(sgt->sgl);
	ret = 0;
	goto exit;

err_buf_detach:
	dma_buf_detach(dmabuf, attachment);
err_buf_put:
	dma_buf_put(dmabuf);
exit:
	G2D_TRACE_END("");
	return ret;
}

static void g2d_dma_unmap(struct dmabuf_item *item)
{
	G2D_TRACE_BEGIN("g2d_dma_unmap");
	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(item->buf, item->attachment);
	dma_buf_put(item->buf);
	G2D_TRACE_END("");
}
#endif

static struct g2d_format_attr fmt_attr_tbl[] = {
	/*
	      format                            bits
						   hor_rsample(u,v)
							  ver_rsample(u,v)
								uvc
								   interleave
								       factor
									  div
	 */
	{ G2D_FORMAT_ARGB8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_ABGR8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_RGBA8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_BGRA8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_XRGB8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_XBGR8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_RGBX8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_BGRX8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_RGB888, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ G2D_FORMAT_BGR888, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ G2D_FORMAT_RGB565, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_BGR565, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_ARGB4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_ABGR4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_RGBA4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_BGRA4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_ARGB1555, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_ABGR1555, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_RGBA5551, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_BGRA5551, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_ARGB2101010, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_ABGR2101010, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_RGBA1010102, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_BGRA1010102, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_IYUV422_V0Y1U0Y0, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_IYUV422_Y1V0Y0U0, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_IYUV422_U0Y1V0Y0, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_IYUV422_Y1U0Y0V0, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ G2D_FORMAT_YUV422_PLANAR, 8,  2, 2, 1, 1, 0, 0, 2, 1},
	{ G2D_FORMAT_YUV420_PLANAR, 8,  2, 2, 2, 2, 0, 0, 3, 2},
	{ G2D_FORMAT_YUV411_PLANAR, 8,  4, 4, 1, 1, 0, 0, 3, 2},
	{ G2D_FORMAT_YUV422UVC_U1V1U0V0, 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ G2D_FORMAT_YUV422UVC_V1U1V0U0, 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ G2D_FORMAT_YUV420UVC_U1V1U0V0, 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ G2D_FORMAT_YUV420UVC_V1U1V0U0, 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ G2D_FORMAT_YUV411UVC_U1V1U0V0, 8,  4, 4, 1, 1, 1, 0, 3, 2},
	{ G2D_FORMAT_YUV411UVC_V1U1V0U0, 8,  4, 4, 1, 1, 1, 0, 3, 2},
	{ G2D_FORMAT_Y8, 8,  1, 1, 1, 1, 0, 0, 1, 1},
	{ G2D_FORMAT_YVU10_444, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_YUV10_444, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ G2D_FORMAT_YVU10_P210, 10, 2, 2, 1, 1, 0, 0, 4, 1},
	{ G2D_FORMAT_YVU10_P010, 10, 2, 2, 2, 2, 0, 0, 3, 1},
};

unsigned int dma_buf_size(const struct dmabuf_item *item)
{
	struct scatterlist *sgl;
	struct sg_table *sgt = item->sgt;
	unsigned int size = 0;
	unsigned int i = 0;

	for_each_sg(sgt->sgl, sgl, sgt->nents, i) {
		size += sg_dma_len(sgl);
	}

	return size;
}

s32 g2d_set_info(g2d_image_enh *g2d_img, const struct dmabuf_item *item)
{
	s32 ret = -1;
	u32 i = 0;
	u32 len = ARRAY_SIZE(fmt_attr_tbl);
	u32 y_width, y_height, u_width, u_height;
	u32 y_pitch, u_pitch;
	u32 y_size, u_size;

	g2d_img->laddr[0] = item->dma_addr;

	if (g2d_img->format >= G2D_FORMAT_MAX) {
		G2D_WARN("format 0x%x is out of range\n",
			g2d_img->format);
		goto exit;
	}

	for (i = 0; i < len; ++i) {

		if (fmt_attr_tbl[i].format == g2d_img->format) {
			y_width = g2d_img->width;
			y_height = g2d_img->height;
			u_width = y_width/fmt_attr_tbl[i].hor_rsample_u;
			u_height = y_height/fmt_attr_tbl[i].ver_rsample_u;

			y_pitch = G2DALIGN(y_width, g2d_img->align[0]);
			u_pitch = G2DALIGN(u_width * (fmt_attr_tbl[i].uvc + 1),
					g2d_img->align[1]);

			y_size = y_pitch * y_height;
			u_size = u_pitch * u_height;
			g2d_img->laddr[1] = g2d_img->laddr[0] + y_size;
			g2d_img->laddr[2] = g2d_img->laddr[0] + y_size + u_size;

			if (g2d_img->format == G2D_FORMAT_YUV420_PLANAR) {
				/* v */
				g2d_img->laddr[1] = g2d_img->laddr[0] + y_size + u_size;
				g2d_img->laddr[2] = g2d_img->laddr[0] + y_size; /* u */
			}

			ret = 0;
			break;
		}
	}
	if (ret != 0)
		G2D_WARN("format 0x%x is invalid\n", g2d_img->format);
exit:
	return ret;

}

__s32 drv_g2d_init(void)
{
	g2d_init_para init_para;

	G2D_INFO("drv_g2d_init\n");
	init_para.g2d_base = (unsigned long) para.io;
	memset(&g2d_ext_hd, 0, sizeof(__g2d_drv_t));
	init_waitqueue_head(&g2d_ext_hd.queue);
	g2d_init(&init_para);

	return 0;
}

int g2d_get_layout_version(void)
{
#ifdef G2D_V2X_SUPPORT
	return 2;
#else
	return 1;
#endif
}
EXPORT_SYMBOL(g2d_get_layout_version);

void *g2d_malloc(__u32 bytes_num, uintptr_t *phy_addr)
{
	void *address = NULL;

#ifdef ALLOC_USING_DMA
	u32 actual_bytes;

	if (bytes_num != 0) {
		actual_bytes = G2D_BYTE_ALIGN(bytes_num);

		address = dma_alloc_coherent(para.dev, actual_bytes,
					     (dma_addr_t *) phy_addr,
					     GFP_KERNEL);
		if (address) {
			G2D_DBG("dma_alloc_coherent ok, address=0x%p, size=0x%x\n",
			    (void *)(*(unsigned long *)phy_addr), bytes_num);
			return address;
		}
		G2D_WARN("dma_alloc_coherent fail, size=0x%x\n", bytes_num);
		return NULL;
	}
	G2D_WARN("request memory size is zero\n");
#else
	unsigned map_size = 0;
	struct page *page;

	if (bytes_num != 0) {
		map_size = PAGE_ALIGN(bytes_num);
		page = alloc_pages(GFP_KERNEL, get_order(map_size));
		if (page != NULL) {
			address = page_address(page);
			if (address == NULL) {
				free_pages((unsigned long)(page),
					   get_order(map_size));
				G2D_WARN("page_address fail\n");
				return NULL;
			}
			*phy_addr = virt_to_phys(address);
			return address;
		}
		G2D_WARN("alloc_pages fail\n");
		return NULL;
	}
	G2D_WARN("request memory size is zero\n");
#endif

	return NULL;
}

void g2d_free(void *virt_addr, void *phy_addr, unsigned int size)
{
#ifdef ALLOC_USING_DMA

	u32 actual_bytes;

	actual_bytes = PAGE_ALIGN(size);
	if (phy_addr && virt_addr)
		dma_free_coherent(para.dev, actual_bytes, virt_addr,
				  (dma_addr_t) phy_addr);
#else
	unsigned map_size = PAGE_ALIGN(size);
	unsigned page_size = map_size;

	if (virt_addr == NULL)
		return;

	free_pages((unsigned long)virt_addr, get_order(page_size));
#endif
}

__s32 g2d_get_free_mem_index(void)
{
	__u32 i = 0;

	for (i = 0; i < MAX_G2D_MEM_INDEX; i++) {
		if (g2d_mem[i].b_used == 0)
			return i;
	}
	return -1;
}

int g2d_mem_request(__u32 size)
{
	__s32 sel;
	unsigned long ret = 0;
	uintptr_t phy_addr;

	sel = g2d_get_free_mem_index();
	if (sel < 0) {
		G2D_WARN("g2d_get_free_mem_index fail\n");
		return -EINVAL;
	}

	ret = (unsigned long)g2d_malloc(size, &phy_addr);
	if (ret != 0) {
		g2d_mem[sel].virt_addr = (void *)ret;
		memset(g2d_mem[sel].virt_addr, 0, size);
		g2d_mem[sel].phy_addr = phy_addr;
		g2d_mem[sel].mem_len = size;
		g2d_mem[sel].b_used = 1;

		G2D_DBG("map_g2d_memory[%d]: pa=%08lx va=%p size:%x\n", sel,
		     g2d_mem[sel].phy_addr, g2d_mem[sel].virt_addr, size);
		return sel;
	}
	G2D_WARN("fail to alloc reserved memory\n");
	return -ENOMEM;
}

int g2d_mem_release(__u32 sel)
{
	if (g2d_mem[sel].b_used == 0) {
		G2D_WARN("mem not used in g2d_mem_release,%d\n", sel);
		return -EINVAL;
	}

	g2d_free((void *)g2d_mem[sel].virt_addr, (void *)g2d_mem[sel].phy_addr,
		 g2d_mem[sel].mem_len);
	memset(&g2d_mem[sel], 0, sizeof(g2d_mem[sel]));

	return 0;
}

int g2d_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long mypfn = vma->vm_pgoff;
	unsigned long vmsize = vma->vm_end - vma->vm_start;

	vma->vm_pgoff = 0;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, mypfn,
			    vmsize, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

int g2d_open(struct inode *inode, struct file *file)
{
	mutex_lock(&para.mutex);
	para.user_cnt++;
	if (para.user_cnt == 1) {
		g2d_clock_prepare(&para);
#ifndef CONFIG_PM_GENERIC_DOMAINS
		g2d_clock_enable(&para);
#endif
		para.opened = true;
#if defined(G2D_V2X_SUPPORT) && !defined(CONFIG_PM_GENERIC_DOMAINS)
		g2d_bsp_open();
#endif
	}

	mutex_unlock(&para.mutex);
	return 0;
}
EXPORT_SYMBOL(g2d_open);

int g2d_release(struct inode *inode, struct file *file)
{
	mutex_lock(&para.mutex);
	para.user_cnt--;
	if (para.user_cnt == 0) {
#ifndef CONFIG_PM_GENERIC_DOMAINS
		g2d_clock_disable(&para);
#endif
		g2d_clock_unprepare(&para);
		para.opened = false;
#if defined(G2D_V2X_SUPPORT) && !defined(CONFIG_PM_GENERIC_DOMAINS)
		g2d_bsp_close();
#endif
	}

	mutex_unlock(&para.mutex);

	mutex_lock(&global_lock);
	scan_order = G2D_SM_TDLR;
	mutex_unlock(&global_lock);

	return 0;
}
EXPORT_SYMBOL(g2d_release);

irqreturn_t g2d_handle_irq(int irq, void *dev_id)
{
#ifdef G2D_V2X_SUPPORT
	__u32 mixer_irq_flag, rot_irq_flag;

	mixer_irq_flag = mixer_irq_query();
	rot_irq_flag = rot_irq_query();

	if (mixer_irq_flag == 0) {
		g2d_mixer_reset();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
	} else if (rot_irq_flag == 0) {
		/* g2d_rot_reset(); */
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
	}
#else
	__u32 mod_irq_flag, cmd_irq_flag;

	mod_irq_flag = mixer_get_irq();
	cmd_irq_flag = mixer_get_irq0();
	if (mod_irq_flag & G2D_FINISH_IRQ) {
		mixer_clear_init();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
	} else if (cmd_irq_flag & G2D_FINISH_IRQ) {
		mixer_clear_init0();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
	}
#endif
	return IRQ_HANDLED;
}

int g2d_init(g2d_init_para *para)
{
	mixer_set_reg_base(para->g2d_base);

	return 0;
}

int g2d_exit(void)
{
	__u8 err = 0;

	return err;
}

int g2d_wait_cmd_finish(void)
{
	long timeout = 100;	/* 100ms */

	timeout = wait_event_timeout(g2d_ext_hd.queue,
				     g2d_ext_hd.finish_flag == 1,
				     msecs_to_jiffies(timeout));
	if (timeout <= 0) {
#ifdef G2D_V2X_SUPPORT
		g2d_bsp_reset();
#else
		mixer_clear_init();
		mixer_clear_init0();
#endif

#if IS_ENABLED(CONFIG_AW_IOMMU)
		if (para.iommu_master_id != -1)
			sunxi_reset_device_iommu(para.iommu_master_id);
#endif

		G2D_WARN("G2D irq pending flag timeout\n");
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
		return -1;
	}
	g2d_ext_hd.finish_flag = 0;

	return 0;
}

int g2d_blit(g2d_blt *para)
{
	__s32 ret = 0;
	__u32 tmp_w, tmp_h;
	struct dmabuf_item *src_item = NULL;
	struct dmabuf_item *dst_item = NULL;

	if ((para->flag & G2D_BLT_ROTATE90) ||
			(para->flag & G2D_BLT_ROTATE270)) {
		tmp_w = para->src_rect.h;
		tmp_h = para->src_rect.w;
	} else {
		tmp_w = para->src_rect.w;
		tmp_h = para->src_rect.h;
	}
	/* check the parameter valid */
	if (((para->src_rect.x < 0) &&
	     ((-para->src_rect.x) > para->src_rect.w)) ||
	    ((para->src_rect.y < 0) &&
	     ((-para->src_rect.y) > para->src_rect.h)) ||
	    ((para->dst_x < 0) &&
	     ((-para->dst_x) > tmp_w)) ||
	    ((para->dst_y < 0) &&
	     ((-para->dst_y) > tmp_h)) ||
	    ((para->src_rect.x > 0) &&
	     (para->src_rect.x > para->src_image.w - 1)) ||
	    ((para->src_rect.y > 0) &&
	     (para->src_rect.y > para->src_image.h - 1)) ||
	    ((para->dst_x > 0) &&
	     (para->dst_x > para->dst_image.w - 1)) ||
	    ((para->dst_y > 0) && (para->dst_y > para->dst_image.h - 1))) {
		G2D_WARN("invalid blit parameter setting");
		return -EINVAL;
	}
	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		src_item = kmalloc(sizeof(*src_item),
				   GFP_KERNEL | __GFP_ZERO);
		if (src_item == NULL) {
			pr_err("[G2D]malloc memory of size %u fail!\n",
				   (unsigned int)sizeof(*src_item));
			goto EXIT;
		}
		dst_item = kmalloc(sizeof(*dst_item),
				   GFP_KERNEL | __GFP_ZERO);
		if (dst_item == NULL) {
			pr_err("[G2D]malloc memory of size %u fail!\n",
				(unsigned int)sizeof(*dst_item));
			goto FREE_SRC;
		}
	}

	if (((para->src_rect.x < 0) &&
				((-para->src_rect.x) < para->src_rect.w))) {
		para->src_rect.w = para->src_rect.w + para->src_rect.x;
		para->src_rect.x = 0;
	} else if ((para->src_rect.x + para->src_rect.w)
			> para->src_image.w) {
		para->src_rect.w = para->src_image.w - para->src_rect.x;
	}
	if (((para->src_rect.y < 0) &&
				((-para->src_rect.y) < para->src_rect.h))) {
		para->src_rect.h = para->src_rect.h + para->src_rect.y;
		para->src_rect.y = 0;
	} else if ((para->src_rect.y + para->src_rect.h)
			> para->src_image.h) {
		para->src_rect.h = para->src_image.h - para->src_rect.y;
	}

	if (((para->dst_x < 0) && ((-para->dst_x) < tmp_w))) {
		para->src_rect.w = tmp_w + para->dst_x;
		para->src_rect.x = (-para->dst_x);
		para->dst_x = 0;
	} else if ((para->dst_x + tmp_w) > para->dst_image.w) {
		para->src_rect.w = para->dst_image.w - para->dst_x;
	}
	if (((para->dst_y < 0) && ((-para->dst_y) < tmp_h))) {
		para->src_rect.h = tmp_h + para->dst_y;
		para->src_rect.y = (-para->dst_y);
		para->dst_y = 0;
	} else if ((para->dst_y + tmp_h) > para->dst_image.h)
		para->src_rect.h = para->dst_image.h - para->dst_y;

	g2d_ext_hd.finish_flag = 0;

	/* Add support inverted order copy, however,
	 * hardware have a bug when reciving y coordinate,
	 * it use (y + height) rather than (y) on inverted
	 * order mode, so here adjust it before pass it to hardware.
	 */

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		ret = g2d_dma_map(para->src_image.addr[0], src_item);
		if (ret != 0) {
			pr_err("[G2D]map cur_item fail!\n");
			goto FREE_DST;
		}
		para->src_image.addr[0] = src_item->dma_addr;

		ret = g2d_dma_map(para->dst_image.addr[0], dst_item);
		if (ret != 0) {
			pr_err("[G2D]map dst_item fail!\n");
			goto SRC_DMA_UNMAP;
		}
		para->dst_image.addr[0] = dst_item->dma_addr;
	}

	mutex_lock(&global_lock);
	if (scan_order > G2D_SM_TDRL)
		para->dst_y += para->src_rect.h;
	mutex_unlock(&global_lock);

	ret = mixer_blt(para, scan_order);

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		g2d_dma_unmap(dst_item);
SRC_DMA_UNMAP:
		g2d_dma_unmap(src_item);
FREE_DST:
		kfree(dst_item);
FREE_SRC:
		kfree(src_item);
	}
EXIT:
	return ret;
}

int g2d_fill(g2d_fillrect *para)
{
	__s32 ret = 0;
	struct dmabuf_item *dst_item = NULL;

	/* check the parameter valid */
	if (((para->dst_rect.x < 0) &&
	     ((-para->dst_rect.x) > para->dst_rect.w)) ||
	    ((para->dst_rect.y < 0) &&
	     ((-para->dst_rect.y) > para->dst_rect.h)) ||
	    ((para->dst_rect.x > 0) &&
	     (para->dst_rect.x > para->dst_image.w - 1)) ||
	    ((para->dst_rect.y > 0) &&
	     (para->dst_rect.y > para->dst_image.h - 1))) {
		G2D_WARN("invalid fillrect parameter setting");
		return -EINVAL;
	}

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		dst_item = kmalloc(sizeof(*dst_item),
				   GFP_KERNEL | __GFP_ZERO);
		if (dst_item == NULL) {
			pr_err("[G2D]malloc memory of size %u fail!\n",
				(unsigned int)sizeof(*dst_item));
			goto EXIT;
		}
	}

	if (((para->dst_rect.x < 0) &&
				((-para->dst_rect.x) < para->dst_rect.w))) {
		para->dst_rect.w = para->dst_rect.w + para->dst_rect.x;
		para->dst_rect.x = 0;
	} else if ((para->dst_rect.x + para->dst_rect.w)
			> para->dst_image.w) {
		para->dst_rect.w = para->dst_image.w - para->dst_rect.x;
	}
	if (((para->dst_rect.y < 0) &&
				((-para->dst_rect.y) < para->dst_rect.h))) {
		para->dst_rect.h = para->dst_rect.h + para->dst_rect.y;
		para->dst_rect.y = 0;
	} else if ((para->dst_rect.y + para->dst_rect.h)
			> para->dst_image.h)
		para->dst_rect.h = para->dst_image.h - para->dst_rect.y;

	g2d_ext_hd.finish_flag = 0;

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		ret = g2d_dma_map(para->dst_image.addr[0], dst_item);
		if (ret != 0) {
			pr_err("[G2D]map dst_item fail!\n");
			goto FREE_DST;
		}
		para->dst_image.addr[0] = dst_item->dma_addr;
	}

	ret = mixer_fillrectangle(para);

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		g2d_dma_unmap(dst_item);
FREE_DST:
		kfree(dst_item);
	}
EXIT:
	return ret;
}

int g2d_stretchblit(g2d_stretchblt *para)
{
	__s32 ret = 0;
	struct dmabuf_item *src_item = NULL;
	struct dmabuf_item *dst_item = NULL;

	/* check the parameter valid */
	if (((para->src_rect.x < 0) &&
	     ((-para->src_rect.x) > para->src_rect.w)) ||
	    ((para->src_rect.y < 0) &&
	     ((-para->src_rect.y) > para->src_rect.h)) ||
	    ((para->dst_rect.x < 0) &&
	     ((-para->dst_rect.x) > para->dst_rect.w)) ||
	    ((para->dst_rect.y < 0) &&
	     ((-para->dst_rect.y) > para->dst_rect.h)) ||
	    ((para->src_rect.x > 0) &&
	     (para->src_rect.x > para->src_image.w - 1)) ||
	    ((para->src_rect.y > 0) &&
	     (para->src_rect.y > para->src_image.h - 1)) ||
	    ((para->dst_rect.x > 0) &&
	     (para->dst_rect.x > para->dst_image.w - 1)) ||
	    ((para->dst_rect.y > 0) &&
	     (para->dst_rect.y > para->dst_image.h - 1))) {
		G2D_WARN("invalid stretchblit parameter setting");
		return -EINVAL;
	}

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		src_item = kmalloc(sizeof(*src_item),
				   GFP_KERNEL | __GFP_ZERO);
		if (src_item == NULL) {
			pr_err("[G2D]malloc memory of size %u fail!\n",
				   (unsigned int)sizeof(*src_item));
			goto EXIT;
		}
		dst_item = kmalloc(sizeof(*dst_item),
				   GFP_KERNEL | __GFP_ZERO);
		if (dst_item == NULL) {
			pr_err("[G2D]malloc memory of size %u fail!\n",
				(unsigned int)sizeof(*dst_item));
			goto FREE_SRC;
		}
	}

	if (((para->src_rect.x < 0) &&
				((-para->src_rect.x) < para->src_rect.w))) {
		para->src_rect.w = para->src_rect.w + para->src_rect.x;
		para->src_rect.x = 0;
	} else if ((para->src_rect.x + para->src_rect.w)
			> para->src_image.w) {
		para->src_rect.w = para->src_image.w - para->src_rect.x;
	}
	if (((para->src_rect.y < 0) &&
				((-para->src_rect.y) < para->src_rect.h))) {
		para->src_rect.h = para->src_rect.h + para->src_rect.y;
		para->src_rect.y = 0;
	} else if ((para->src_rect.y + para->src_rect.h)
			> para->src_image.h) {
		para->src_rect.h = para->src_image.h - para->src_rect.y;
	}

	if (((para->dst_rect.x < 0) &&
				((-para->dst_rect.x) < para->dst_rect.w))) {
		para->dst_rect.w = para->dst_rect.w + para->dst_rect.x;
		para->dst_rect.x = 0;
	} else if ((para->dst_rect.x + para->dst_rect.w)
			> para->dst_image.w) {
		para->dst_rect.w = para->dst_image.w - para->dst_rect.x;
	}
	if (((para->dst_rect.y < 0) &&
				((-para->dst_rect.y) < para->dst_rect.h))) {
		para->dst_rect.h = para->dst_rect.h + para->dst_rect.y;
		para->dst_rect.y = 0;
	} else if ((para->dst_rect.y + para->dst_rect.h)
			> para->dst_image.h) {
		para->dst_rect.h = para->dst_image.h - para->dst_rect.y;
	}

	g2d_ext_hd.finish_flag = 0;

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		ret = g2d_dma_map(para->src_image.addr[0], src_item);
		if (ret != 0) {
			pr_err("[G2D]map cur_item fail!\n");
			goto FREE_DST;
		}
		para->src_image.addr[0] = src_item->dma_addr;

		ret = g2d_dma_map(para->dst_image.addr[0], dst_item);
		if (ret != 0) {
			pr_err("[G2D]map dst_item fail!\n");
			goto SRC_DMA_UNMAP;
		}
		para->dst_image.addr[0] = dst_item->dma_addr;
	}

	/* Add support inverted order copy, however,
	 * hardware have a bug when reciving y coordinate,
	 * it use (y + height) rather than (y) on inverted
	 * order mode, so here adjust it before pass it to hardware.
	 */

	mutex_lock(&global_lock);
	if (scan_order > G2D_SM_TDRL)
		para->dst_rect.y += para->src_rect.h;
	mutex_unlock(&global_lock);

	ret = mixer_stretchblt(para, scan_order);

	if (para->flag & G2D_BLT_USE_DMABUFHEAP) {
		g2d_dma_unmap(dst_item);
SRC_DMA_UNMAP:
		g2d_dma_unmap(src_item);
FREE_DST:
		kfree(dst_item);
FREE_SRC:
		kfree(src_item);
	}
EXIT:
	return ret;
}

#ifdef G2D_V2X_SUPPORT
int g2d_fill_h(g2d_fillrect_h *para)
{
	__s32 ret = 0;
	struct dmabuf_item *dst_item = NULL;

	if (!para->dst_image_h.use_phy_addr) {

		dst_item = kzalloc(sizeof(*dst_item), GFP_KERNEL);
		if (dst_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
				(unsigned int)sizeof(*dst_item));
			goto EXIT;
		}
	}
	/* check the parameter valid */
	if (((para->dst_image_h.clip_rect.x < 0) &&
	     ((-para->dst_image_h.clip_rect.x) >
	      para->dst_image_h.clip_rect.w)) ||
	    ((para->dst_image_h.clip_rect.y < 0) &&
	     ((-para->dst_image_h.clip_rect.y) >
	      para->dst_image_h.clip_rect.h)) ||
	    ((para->dst_image_h.clip_rect.x > 0) &&
	     (para->dst_image_h.clip_rect.x > para->dst_image_h.width - 1))
	    || ((para->dst_image_h.clip_rect.y > 0) &&
		(para->dst_image_h.clip_rect.y >
		 para->dst_image_h.height - 1))) {
		G2D_WARN("invalid fillrect parameter setting\n");
		return -EINVAL;
	}
	if (((para->dst_image_h.clip_rect.x < 0) &&
				((-para->dst_image_h.clip_rect.x) <
				 para->dst_image_h.clip_rect.w))) {
		para->dst_image_h.clip_rect.w =
			para->dst_image_h.clip_rect.w +
			para->dst_image_h.clip_rect.x;
		para->dst_image_h.clip_rect.x = 0;
	} else if ((para->dst_image_h.clip_rect.x +
				para->dst_image_h.clip_rect.w)
			> para->dst_image_h.width) {
		para->dst_image_h.clip_rect.w =
			para->dst_image_h.width -
			para->dst_image_h.clip_rect.x;
	}
	if (((para->dst_image_h.clip_rect.y < 0) &&
				((-para->dst_image_h.clip_rect.y) <
				 para->dst_image_h.clip_rect.h))) {
		para->dst_image_h.clip_rect.h =
			para->dst_image_h.clip_rect.h +
			para->dst_image_h.clip_rect.y;
		para->dst_image_h.clip_rect.y = 0;
	} else if ((para->dst_image_h.clip_rect.y +
				para->dst_image_h.clip_rect.h)
			> para->dst_image_h.height) {
		para->dst_image_h.clip_rect.h =
			para->dst_image_h.height -
			para->dst_image_h.clip_rect.y;
	}

	para->dst_image_h.bbuff = 1;
	para->dst_image_h.gamut = G2D_BT709;
	para->dst_image_h.mode = 0;

	g2d_ext_hd.finish_flag = 0;

	if (!para->dst_image_h.use_phy_addr) {
		ret = g2d_dma_map(para->dst_image_h.fd, dst_item);
		if (ret != 0) {
			G2D_WARN("map cur_item fail\n");
			goto FREE_DST;
		}

		g2d_set_info(&para->dst_image_h, dst_item);
	}

	ret = g2d_fillrectangle(&para->dst_image_h, para->dst_image_h.color);

	if (ret)
		G2D_WARN("G2D FILLRECTANGLE Failed!\n");
	if (!para->dst_image_h.use_phy_addr)
		g2d_dma_unmap(dst_item);
FREE_DST:
	if (!para->dst_image_h.use_phy_addr)
		kfree(dst_item);
EXIT:
	return ret;
}

int g2d_blit_h(g2d_blt_h *para)
{
	__s32 ret = 0;
	struct dmabuf_item *src_item = NULL;
	struct dmabuf_item *dst_item = NULL;
	__u32 src_buffersize;
	__u32 dst_buffersize;

	if (!para->src_image_h.use_phy_addr) {

		src_item = kzalloc(sizeof(*src_item), GFP_KERNEL);
		if (src_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*src_item));
			goto EXIT;
		}
	}

	if (!para->dst_image_h.use_phy_addr) {
		dst_item = kzalloc(sizeof(*dst_item), GFP_KERNEL);
		if (dst_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*dst_item));
			goto FREE_SRC;
		}
	}
	/* check the parameter valid */
	if (((para->src_image_h.clip_rect.x < 0) &&
	     ((-para->src_image_h.clip_rect.x) >
	      para->src_image_h.clip_rect.w)) ||
	    ((para->src_image_h.clip_rect.y < 0) &&
	     ((-para->src_image_h.clip_rect.y) >
	      para->src_image_h.clip_rect.h)) ||
	    ((para->src_image_h.clip_rect.x > 0) &&
	     (para->src_image_h.clip_rect.x >
	      para->src_image_h.width - 1)) ||
	    ((para->src_image_h.clip_rect.y > 0) &&
	     (para->src_image_h.clip_rect.y >
	      para->src_image_h.height - 1)) ||
	    ((para->dst_image_h.clip_rect.x > 0) &&
	     (para->dst_image_h.clip_rect.x >
	      para->dst_image_h.width - 1)) ||
	    ((para->dst_image_h.clip_rect.y > 0) &&
	     (para->dst_image_h.clip_rect.y > para->dst_image_h.height - 1))) {
		G2D_WARN("invalid bitblit parameter setting\n");
		return -EINVAL;
	}
	if (((para->src_image_h.clip_rect.x < 0) &&
				((-para->src_image_h.clip_rect.x) <
				 para->src_image_h.clip_rect.w))) {
		para->src_image_h.clip_rect.w =
			para->src_image_h.clip_rect.w +
			para->src_image_h.clip_rect.x;
		para->src_image_h.clip_rect.x = 0;
	} else if ((para->src_image_h.clip_rect.x +
				para->src_image_h.clip_rect.w)
			> para->src_image_h.width) {
		para->src_image_h.clip_rect.w =
			para->src_image_h.width -
			para->src_image_h.clip_rect.x;
	}
	if (((para->src_image_h.clip_rect.y < 0) &&
				((-para->src_image_h.clip_rect.y) <
				 para->src_image_h.clip_rect.h))) {
		para->src_image_h.clip_rect.h =
			para->src_image_h.clip_rect.h +
			para->src_image_h.clip_rect.y;
		para->src_image_h.clip_rect.y = 0;
	} else if ((para->src_image_h.clip_rect.y +
				para->src_image_h.clip_rect.h)
			> para->src_image_h.height) {
		para->src_image_h.clip_rect.h =
			para->src_image_h.height -
			para->src_image_h.clip_rect.y;
	}

	if (((para->dst_image_h.clip_rect.x < 0) &&
				((-para->dst_image_h.clip_rect.x) <
				 para->dst_image_h.clip_rect.w))) {
		para->dst_image_h.clip_rect.w =
			para->dst_image_h.clip_rect.w +
			para->dst_image_h.clip_rect.x;
		para->dst_image_h.clip_rect.x = 0;
	} else if ((para->dst_image_h.clip_rect.x +
				para->dst_image_h.clip_rect.w)
			> para->dst_image_h.width) {
		para->dst_image_h.clip_rect.w =
			para->dst_image_h.width -
			para->dst_image_h.clip_rect.x;
	}
	if (((para->dst_image_h.clip_rect.y < 0) &&
				((-para->dst_image_h.clip_rect.y) <
				 para->dst_image_h.clip_rect.h))) {
		para->dst_image_h.clip_rect.h =
			para->dst_image_h.clip_rect.h +
			para->dst_image_h.clip_rect.y;
		para->dst_image_h.clip_rect.y = 0;
	} else if ((para->dst_image_h.clip_rect.y +
				para->dst_image_h.clip_rect.h)
			> para->dst_image_h.height) {
		para->dst_image_h.clip_rect.h =
			para->dst_image_h.height -
			para->dst_image_h.clip_rect.y;
	}

	g2d_ext_hd.finish_flag = 0;

	/* Add support inverted order copy, however,
	 * hardware have a bug when reciving y coordinate,
	 * it use (y + height) rather than (y) on inverted
	 * order mode, so here adjust it before pass it to hardware.
	 */

	para->src_image_h.bpremul = 0;
	para->src_image_h.bbuff = 1;
	para->src_image_h.gamut = G2D_BT709;

	para->dst_image_h.bpremul = 0;
	para->dst_image_h.bbuff = 1;
	para->dst_image_h.gamut = G2D_BT709;

	if (!para->src_image_h.use_phy_addr) {
		ret = g2d_dma_map(para->src_image_h.fd, src_item);
		if (ret != 0) {
			G2D_WARN("map cur_item fail\n");
			goto FREE_DST;
		}
		g2d_set_info(&para->src_image_h, src_item);
		src_buffersize = dma_buf_size(src_item);
		g2d_set_src_image_buf_size(src_buffersize);
	}

	if (!para->dst_image_h.use_phy_addr) {
		ret = g2d_dma_map(para->dst_image_h.fd, dst_item);
		if (ret != 0) {
			G2D_WARN("map dst_item fail\n");
			goto SRC_DMA_UNMAP;
		}
		g2d_set_info(&para->dst_image_h, dst_item);
		dst_buffersize = dma_buf_size(dst_item);
		g2d_set_dst_image_buf_size(dst_buffersize);
	}

	G2D_TRACE_BEGIN("g2d_bsp_bitblt");
	ret = g2d_bsp_bitblt(&para->src_image_h,
					&para->dst_image_h, para->flag_h);
	G2D_TRACE_END("");

	if (ret)
		G2D_WARN("G2D BITBLT Failed\n");

	if (!para->dst_image_h.use_phy_addr)
		g2d_dma_unmap(dst_item);
SRC_DMA_UNMAP:
	if (!para->src_image_h.use_phy_addr)
		g2d_dma_unmap(src_item);
FREE_DST:
	if (!para->dst_image_h.use_phy_addr)
		kfree(dst_item);
FREE_SRC:
	if (!para->src_image_h.use_phy_addr)
		kfree(src_item);
EXIT:
	return ret;
}
EXPORT_SYMBOL(g2d_blit_h);

/**
 * g2d_bsp_blit_h
 * @info: g2d_blt_h
 *
 * DESCRIPTION:
 * The current interface is provided for internal calls of driver,
 * and the power and clock can be turned on and off without using ioctl.
 * eg: Framebuffer rotation driver.
 *
 */
int g2d_bsp_blit_h(g2d_blt_h *info)
{
	__s32 ret = 0;

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_get_sync(para.dev);
#endif

	ret = g2d_blit_h(info);

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_put_sync(para.dev);
#endif

	return ret;
}
EXPORT_SYMBOL(g2d_bsp_blit_h);

int g2d_bld_h(g2d_bld *para)
{
	__s32 ret = 0;
	struct dmabuf_item *src_item = NULL;
	struct dmabuf_item *dst_item = NULL;

	if (!para->src_image[0].use_phy_addr) {

		src_item = kzalloc(sizeof(*src_item), GFP_KERNEL);
		if (src_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*src_item));
			goto EXIT;
		}
	}

	if (!para->dst_image.use_phy_addr) {
		dst_item = kzalloc(sizeof(*dst_item), GFP_KERNEL);
		if (dst_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*dst_item));
			goto FREE_SRC;
		}
	}

	/* check the parameter valid */
	if (((para->src_image[0].clip_rect.x < 0) &&
	     ((-para->src_image[0].clip_rect.x) >
	      para->src_image[0].clip_rect.w)) ||
	    ((para->src_image[0].clip_rect.y < 0) &&
	     ((-para->src_image[0].clip_rect.y) >
	      para->src_image[0].clip_rect.h)) ||
	    ((para->src_image[0].clip_rect.x > 0) &&
	     (para->src_image[0].clip_rect.x >
	      para->src_image[0].width - 1)) ||
	    ((para->src_image[0].clip_rect.y > 0) &&
	     (para->src_image[0].clip_rect.y >
	      para->src_image[0].height - 1)) ||
	    ((para->dst_image.clip_rect.x > 0) &&
	     (para->dst_image.clip_rect.x > para->dst_image.width - 1))
	    || ((para->dst_image.clip_rect.y > 0) &&
		(para->dst_image.clip_rect.y >
		 para->dst_image.height - 1))) {
		G2D_ERR("invalid blit parameter setting\n");
		return -EINVAL;
	}
	if (((para->src_image[0].clip_rect.x < 0) &&
				((-para->src_image[0].clip_rect.x) <
				 para->src_image[0].clip_rect.w))) {
		para->src_image[0].clip_rect.w =
			para->src_image[0].clip_rect.w +
			para->src_image[0].clip_rect.x;
		para->src_image[0].clip_rect.x = 0;
	} else if ((para->src_image[0].clip_rect.x +
				para->src_image[0].clip_rect.w)
			> para->src_image[0].width) {
		para->src_image[0].clip_rect.w =
			para->src_image[0].width -
			para->src_image[0].clip_rect.x;
	}
	if (((para->src_image[0].clip_rect.y < 0) &&
				((-para->src_image[0].clip_rect.y) <
				 para->src_image[0].clip_rect.h))) {
		para->src_image[0].clip_rect.h =
			para->src_image[0].clip_rect.h +
			para->src_image[0].clip_rect.y;
		para->src_image[0].clip_rect.y = 0;
	} else if ((para->src_image[0].clip_rect.y +
				para->src_image[0].clip_rect.h)
			> para->src_image[0].height) {
		para->src_image[0].clip_rect.h =
			para->src_image[0].height -
			para->src_image[0].clip_rect.y;
	}

	para->src_image[0].bpremul = 0;
	para->src_image[0].bbuff = 1;
	para->src_image[0].gamut = G2D_BT709;

	para->dst_image.bpremul = 0;
	para->dst_image.bbuff = 1;
	para->dst_image.gamut = G2D_BT709;

	g2d_ext_hd.finish_flag = 0;

	if (!para->src_image[0].use_phy_addr) {
		ret = g2d_dma_map(para->src_image[0].fd, src_item);
		if (ret != 0) {
			G2D_WARN("map src_item fail\n");
			goto FREE_DST;
		}
		g2d_set_info(&para->src_image[0], src_item);
	}
	if (!para->dst_image.use_phy_addr) {
		ret = g2d_dma_map(para->dst_image.fd, dst_item);
		if (ret != 0) {
			G2D_WARN("map dst_item fail\n");
			goto SRC_DMA_UNMAP;
		}
		g2d_set_info(&para->dst_image, dst_item);
	}
	ret = g2d_bsp_bld(&para->src_image[0], &para->dst_image,
						para->bld_cmd, &para->ck_para);

	if (ret)
		G2D_WARN("G2D BITBLT Failed\n");

	if (!para->dst_image.use_phy_addr)
		g2d_dma_unmap(dst_item);
SRC_DMA_UNMAP:
	if (!para->src_image[0].use_phy_addr)
		g2d_dma_unmap(src_item);
FREE_DST:
	if (!para->dst_image.use_phy_addr)
		kfree(dst_item);
FREE_SRC:
	if (!para->src_image[0].use_phy_addr)
		kfree(src_item);
EXIT:
	return ret;
}

int g2d_maskblt_h(g2d_maskblt *para)
{
	__s32 ret = 0;
	struct dmabuf_item *src_item = NULL;
	struct dmabuf_item *ptn_item = NULL;
	struct dmabuf_item *mask_item = NULL;
	struct dmabuf_item *dst_item = NULL;

	if (!para->src_image_h.use_phy_addr) {

		src_item = kzalloc(sizeof(*src_item), GFP_KERNEL);
		if (src_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*src_item));
			goto EXIT;
		}
		ptn_item = kzalloc(sizeof(*ptn_item), GFP_KERNEL);
		if (ptn_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*ptn_item));
			goto FREE_SRC;
		}

		mask_item = kzalloc(sizeof(*mask_item), GFP_KERNEL);
		if (mask_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)sizeof(*mask_item));
			goto FREE_PTN;
		}
		dst_item = kzalloc(sizeof(*dst_item), GFP_KERNEL);
		if (dst_item == NULL) {
			G2D_WARN("malloc memory of size %u fail\n",
			       (unsigned int)(unsigned int)sizeof(
				   *dst_item));
			goto FREE_MASK;
		}
	}
	/* check the parameter valid */
	if (((para->dst_image_h.clip_rect.x < 0) &&
	     ((-para->dst_image_h.clip_rect.x) >
	      para->dst_image_h.clip_rect.w)) ||
	    ((para->dst_image_h.clip_rect.y < 0) &&
	     ((-para->dst_image_h.clip_rect.y) >
	      para->dst_image_h.clip_rect.h)) ||
	    ((para->dst_image_h.clip_rect.x > 0) &&
	     (para->dst_image_h.clip_rect.x >
	      para->dst_image_h.width - 1)) ||
	    ((para->dst_image_h.clip_rect.y > 0) &&
	     (para->dst_image_h.clip_rect.y > para->dst_image_h.height - 1))) {
		G2D_WARN("invalid maskblt parameter setting\n");
		return -EINVAL;
	}
	if (((para->dst_image_h.clip_rect.x < 0) &&
				((-para->dst_image_h.clip_rect.x) <
				 para->dst_image_h.clip_rect.w))) {
		para->dst_image_h.clip_rect.w =
			para->dst_image_h.clip_rect.w +
			para->dst_image_h.clip_rect.x;
		para->dst_image_h.clip_rect.x = 0;
	} else if ((para->dst_image_h.clip_rect.x +
				para->dst_image_h.clip_rect.w)
			> para->dst_image_h.width) {
		para->dst_image_h.clip_rect.w =
			para->dst_image_h.width -
			para->dst_image_h.clip_rect.x;
	}
	if (((para->dst_image_h.clip_rect.y < 0) &&
				((-para->dst_image_h.clip_rect.y) <
				 para->dst_image_h.clip_rect.h))) {
		para->dst_image_h.clip_rect.h =
			para->dst_image_h.clip_rect.h +
			para->dst_image_h.clip_rect.y;
		para->dst_image_h.clip_rect.y = 0;
	} else if ((para->dst_image_h.clip_rect.y +
				para->dst_image_h.clip_rect.h)
			> para->dst_image_h.height) {
		para->dst_image_h.clip_rect.h =
			para->dst_image_h.height -
			para->dst_image_h.clip_rect.y;
	}

	if (!para->src_image_h.use_phy_addr) {
		ret = g2d_dma_map(para->src_image_h.fd, src_item);
		if (ret != 0) {
			G2D_WARN("map src_item fail\n");
			goto FREE_DST;
		}
		ret = g2d_dma_map(para->ptn_image_h.fd, ptn_item);
		if (ret != 0) {
			G2D_WARN("map ptn_item fail\n");
			goto SRC_DMA_UNMAP;
		}
		ret = g2d_dma_map(para->mask_image_h.fd, mask_item);
		if (ret != 0) {
			G2D_WARN("map mask_item fail\n");
			goto PTN_DMA_UNMAP;
		}
		ret = g2d_dma_map(para->dst_image_h.fd, dst_item);
		if (ret != 0) {
			G2D_WARN("map dst_item fail\n");
			goto MASK_DMA_UNMAP;
		}

		g2d_set_info(&para->src_image_h, src_item);
		g2d_set_info(&para->ptn_image_h, ptn_item);
		g2d_set_info(&para->mask_image_h, mask_item);
		g2d_set_info(&para->dst_image_h, dst_item);
	}

	para->src_image_h.bbuff = 1;
	para->src_image_h.gamut = G2D_BT709;

	para->ptn_image_h.bbuff = 1;
	para->ptn_image_h.gamut = G2D_BT709;

	para->mask_image_h.bbuff = 1;
	para->mask_image_h.gamut = G2D_BT709;

	para->dst_image_h.bbuff = 1;
	para->dst_image_h.gamut = G2D_BT709;

	g2d_ext_hd.finish_flag = 0;

	ret =
	    g2d_bsp_maskblt(&para->src_image_h, &para->ptn_image_h,
			    &para->mask_image_h, &para->dst_image_h,
			    para->back_flag, para->fore_flag);

	if (ret)
		G2D_WARN("G2D MASKBLT Failed\n");
	if (!para->src_image_h.use_phy_addr)
		g2d_dma_unmap(dst_item);
MASK_DMA_UNMAP:
	if (!para->src_image_h.use_phy_addr)
		g2d_dma_unmap(mask_item);
PTN_DMA_UNMAP:
	if (!para->src_image_h.use_phy_addr)
		g2d_dma_unmap(ptn_item);
SRC_DMA_UNMAP:
	if (!para->src_image_h.use_phy_addr)
		g2d_dma_unmap(src_item);
FREE_DST:
	if (!para->src_image_h.use_phy_addr)
		kfree(dst_item);
FREE_MASK:
	if (!para->src_image_h.use_phy_addr)
		kfree(mask_item);
FREE_PTN:
	if (!para->src_image_h.use_phy_addr)
		kfree(ptn_item);
FREE_SRC:
	if (!para->src_image_h.use_phy_addr)
		kfree(src_item);
EXIT:
	return ret;
}
#endif

/*
int g2d_set_palette_table(g2d_palette *para)
{

	if ((para->pbuffer == NULL) || (para->size < 0) ||
			(para->size > 1024)) {
		G2D_WARN("para invalid in mixer_set_palette\n");
		return -1;
	}

	mixer_set_palette(para);

	return 0;
}
*/

/*
int g2d_cmdq(unsigned int para)
{
	__s32 err = 0;

	g2d_ext_hd.finish_flag = 0;
	err = mixer_cmdq(para);

	return err;
}
*/

void g2d_query_hardware_version(struct g2d_hardware_version *v)
{

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10P1)
#define SYS_CFG_BASE 0x03000000
#define VER_REG_OFFS 0x00000024
	void __iomem *io = NULL;
	io = ioremap(SYS_CFG_BASE, 0x100);
	if (io == NULL) {
		G2D_WARN("ioremap of sys_cfg register failed\n");
		return;
	}
	v->chip_version = readl(io + VER_REG_OFFS);
	iounmap(io);
#else
	v->chip_version = 0;
#endif

	v->g2d_version = g2d_ip_version();
	G2D_INFO("g2d version: %08x chip version: %08x", v->g2d_version, v->chip_version);
}

int g2d_ioctl_mutex_lock(void)
{
#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
	int ret;
	int i;
	if (hwlock) {
		for (i = 0; i < 200; i++) {
			ret =  __hwspin_trylock(hwlock, HWLOCK_RAW, &hwspinlock_flag);
			if (ret != 0) {
				msleep(3);
				continue;
			} else
				break;
		}
		if (ret != 0) {
			G2D_ERR("try to get hwspinlock filed 200 times\n");
			return -1;
		}
	}
	enable_irq(para.irq);
#endif
	if (!mutex_trylock(&para.mutex))
		mutex_lock(&para.mutex);
	return 0;

}
EXPORT_SYMBOL_GPL(g2d_ioctl_mutex_lock);

int g2d_ioctl_mutex_unlock(void)
{
#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
	disable_irq(para.irq);
	if (hwlock)
		__hwspin_unlock(hwlock, HWLOCK_RAW, &hwspinlock_flag);
#endif
	mutex_unlock(&para.mutex);
	return 0;

}
EXPORT_SYMBOL_GPL(g2d_ioctl_mutex_unlock);

long g2d_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	__s32 ret = 0;
	unsigned int size;
	unsigned int sel;
	struct timespec64 test_start, test_end;
	unsigned int runtime;

	if (time_info == 1)
		ktime_get_real_ts64(&test_start);

	ret = g2d_ioctl_mutex_lock();
	if (ret < 0)
		return -EFAULT;

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_get_sync(para.dev);
#endif

	switch (cmd) {

		/* Proceed to the operation */
	case G2D_CMD_BITBLT:{
			g2d_blt blit_para;

			if (copy_from_user(&blit_para, (g2d_blt *) arg,
					   sizeof(g2d_blt))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = g2d_blit(&blit_para);
			break;
		}
	case G2D_CMD_FILLRECT:{
			g2d_fillrect fill_para;

			if (copy_from_user(&fill_para, (g2d_fillrect *) arg,
					   sizeof(g2d_fillrect))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = g2d_fill(&fill_para);
			break;
		}
	case G2D_CMD_STRETCHBLT:{
			g2d_stretchblt stre_para;

			if (copy_from_user(&stre_para, (g2d_stretchblt *) arg,
					   sizeof(g2d_stretchblt))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = g2d_stretchblit(&stre_para);
			break;
		}
/*	case G2D_CMD_PALETTE_TBL:{
		g2d_palette pale_para;

		if (copy_from_user(&pale_para, (g2d_palette *)arg,
					sizeof(g2d_palette))) {
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_set_palette_table(&pale_para);
		break;
	}
	case G2D_CMD_QUEUE:{
		unsigned int cmdq_addr;

		if (copy_from_user(&cmdq_addr,
				(unsigned int *)arg, sizeof(unsigned int))) {
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_cmdq(cmdq_addr);
		break;
	}
*/
#ifdef G2D_V2X_SUPPORT
	case G2D_CMD_BITBLT_H:{
			g2d_blt_h *blit_para = (g2d_blt_h *)kmalloc(sizeof(g2d_blt_h),
							GFP_KERNEL);
			if (!blit_para) {
				G2D_WARN("blit_para kmalloc failed\n");
				ret = -EFAULT;
				goto err_noput;
			}
			if (copy_from_user(blit_para, (g2d_blt_h *) arg,
					   sizeof(g2d_blt_h))) {
				G2D_WARN("BITBLT copy from user failed\n");
				ret = -EFAULT;
				kfree(blit_para);
				goto err_noput;
			}
			ret = g2d_blit_h(blit_para);
			kfree(blit_para);
			break;
		}
	case G2D_CMD_FILLRECT_H:{
			g2d_fillrect_h fill_para;

			if (copy_from_user(&fill_para, (g2d_fillrect_h *) arg,
					   sizeof(g2d_fillrect_h))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = g2d_fill_h(&fill_para);
			break;
		}
	case G2D_CMD_BLD_H:{
			g2d_bld bld_para;

			if (copy_from_user(&bld_para, (g2d_bld *) arg,
					   sizeof(g2d_bld))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = g2d_bld_h(&bld_para);
			break;
		}
	case G2D_CMD_MASK_H:{
			g2d_maskblt mask_para;

			if (copy_from_user(&mask_para, (g2d_maskblt *) arg,
					   sizeof(g2d_maskblt))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = g2d_maskblt_h(&mask_para);
			break;
		}
#endif
		/* just management memory for test */
	case G2D_CMD_MEM_REQUEST:
		get_user(size, (unsigned int __user *)arg);
		ret = g2d_mem_request(size);
		break;

	case G2D_CMD_MEM_RELEASE:
		get_user(sel, (unsigned int __user *)arg);
		ret = g2d_mem_release(sel);
		break;

	case G2D_CMD_MEM_SELIDX:
		get_user(sel, (unsigned int __user *)arg);
		g2d_mem_sel = sel;
		break;

	case G2D_CMD_MEM_GETADR:
		get_user(sel, (unsigned int __user *)arg);
		if (g2d_mem[sel].b_used) {
			ret = g2d_mem[sel].phy_addr;
		} else {
			G2D_WARN("mem not used in G2D_CMD_MEM_GETADR\n");
			ret = -1;
		}
		break;

	case G2D_CMD_INVERTED_ORDER:
		{
			if (arg > G2D_SM_DTRL) {
				G2D_WARN("scan mode is err\n");
				ret = -EINVAL;
				goto err_noput;
			}

			mutex_lock(&global_lock);
			scan_order = arg;
			mutex_unlock(&global_lock);
			break;
		}

	case G2D_CMD_QUERY_VERSION:
		{
			struct g2d_hardware_version version;
			g2d_query_hardware_version(&version);

			if (copy_to_user((struct g2d_hardware_version *)arg, &version,
						sizeof(struct g2d_hardware_version))) {
				ret = -EFAULT;
				goto err_noput;
			}

			break;
		}

		/* Invalid IOCTL call */
	default:
		{
			ret = -EFAULT;
			goto err_noput;
			break;
		}
	}

err_noput:

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_put_sync(para.dev);
#endif

	g2d_ioctl_mutex_unlock();

	if (time_info == 1) {
		ktime_get_real_ts64(&test_end);
		runtime = (test_end.tv_sec - test_start.tv_sec) * 1000000 +
			(test_end.tv_nsec - test_start.tv_nsec) / NSEC_PER_USEC;
		G2D_DBG("g2d_ioctl use %u us\n", runtime);
	}
	return ret;
}

static const struct file_operations g2d_fops = {
	.owner = THIS_MODULE,
	.open = g2d_open,
	.release = g2d_release,
	.unlocked_ioctl = g2d_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl   = g2d_ioctl,
#endif
	.mmap = g2d_mmap,
};

static int get_iommu_master_id(char *name)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	int ret;
	u32 data[3];

	len = sprintf(compat, "allwinner,sunxi-%s", name);

	if (len > 32) {
		G2D_ERR("size of mian_name is out of range\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, compat);

	if (!node) {
		G2D_ERR("of_find_compatible_node %s fail\n", compat);
		return -1;
	}

	/* get iommu master id */
	ret = of_property_read_variable_u32_array(node, "iommus", data, 0, ARRAY_SIZE(data));

	if (ret < 0) {
		return -1;
	}

	return data[1];
}

static u64 sunxi_g2d_dma_mask = DMA_BIT_MASK(32);

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
static int g2d_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int array_size)
{
	int i;
	struct device_link *link;
	struct device *pd_dev;

	if (dev->pm_domain)
		return 0;
	for (i = 0; i < array_size; i++) {
		if (values_of_power_domain_names[i] == NULL) {
			break;
		}
		pd_dev = dev_pm_domain_attach_by_name(dev, values_of_power_domain_names[i]);
		if (IS_ERR(pd_dev))
			return PTR_ERR(pd_dev);

		if (!pd_dev)
			return 0;
		link = device_link_add(dev, pd_dev,
				DL_FLAG_STATELESS |
				DL_FLAG_PM_RUNTIME);
		if (!link) {
			G2D_ERR("Failed to add device_link to %s.\n", values_of_power_domain_names[i]);
			return -EINVAL;
		}
	}
	return 0;
}
#endif

static int g2d_probe(struct platform_device *pdev)
{
#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	const char *values_of_power_domain_names[PM_ARRAY_SIZE];
#endif
#if !defined(CONFIG_OF)
	int size;
	struct resource *res;
#endif
	int ret = 0;
	__g2d_info_t *info = NULL;

	info = &para;
	info->dev = &pdev->dev;
	dmabuf_dev = &pdev->dev;
	dmabuf_dev->dma_mask = &sunxi_g2d_dma_mask;
	dmabuf_dev->coherent_dma_mask = DMA_BIT_MASK(32);
	platform_set_drvdata(pdev, info);

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	ret = of_property_read_string(pdev->dev.of_node, "power-domain-names", values_of_power_domain_names);
	ret = g2d_attach_pd(para.dev, values_of_power_domain_names, PM_ARRAY_SIZE);
	pm_runtime_enable(para.dev);
#endif
#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
	hwlock = hwspin_lock_request_specific(g2d_hwspinlock_id);
	if (!hwlock) {
		pr_err("G2D: Hwspinlock request is failed!\n");
	}
#endif
#if !defined(CONFIG_OF)
	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		G2D_ERR("failed to get memory register\n");
		ret = -ENXIO;
		goto dealloc_fb;
	}

	size = (res->end - res->start) + 1;
	/* map the memory */
	info->io = ioremap(res->start, size);
	if (info->io == NULL) {
		G2D_ERR("iorGmap() of register failed\n");
		ret = -ENXIO;
		goto dealloc_fb;
	}
#else
	info->io = of_iomap(pdev->dev.of_node, 0);
	if (info->io == NULL) {
		G2D_ERR("iormap() of register failed\n");
		ret = -ENXIO;
		goto dealloc_fb;
	}
#endif

#if !defined(CONFIG_OF)
	/* get the irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		G2D_ERR("failed to get irq resource\n");
		ret = -ENXIO;
		goto release_regs;
	}
	info->irq = res->start;
#else
	info->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!info->irq) {
		G2D_ERR("irq_of_parse_and_map irq fail for transform\n");
		ret = -ENXIO;
		goto release_regs;
	}
#endif
#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
	if (hwlock) {
		ret =  __hwspin_lock_timeout(hwlock, 1000, HWLOCK_RAW, &hwspinlock_flag);
		if (ret != 0) {
			pr_err("G2D: Hwspinlock is already taken \n");
			hwspin_lock_free(hwlock);
			return -1;
		} else {
			/* request the irq */
			ret = request_irq(info->irq, g2d_handle_irq, 0,
					  dev_name(&pdev->dev), NULL);
			if (ret) {
				G2D_ERR("failed to install irq resource\n");
				goto release_regs;
			}
			disable_irq(para.irq);
			__hwspin_unlock(hwlock, HWLOCK_RAW, &hwspinlock_flag);
		}
	}
#endif
#if !IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
	/* request the irq */
	ret = request_irq(info->irq, g2d_handle_irq, 0,
			  dev_name(&pdev->dev), NULL);
	if (ret) {
		G2D_ERR("failed to install irq resource\n");
		goto release_regs;
	}
#endif
#if IS_ENABLED(CONFIG_OF)
	/* clk init */
	info->clk = devm_clk_get(&pdev->dev, "g2d");
	if (IS_ERR(info->clk)) {
		G2D_ERR("fail to get clk\n");
		ret = PTR_ERR(info->clk);
		goto out_dispose_mapping;

	} else {
		info->clk_parent = clk_get_parent(info->clk);
		info->bus_clk = devm_clk_get(&pdev->dev, "bus");
		info->mbus_clk = devm_clk_get(&pdev->dev, "mbus_g2d");
		if (IS_ERR(info->mbus_clk)) {
			G2D_INFO("mbus clock get failed\n");
			info->mbus_clk = NULL;
		}
		info->reset = devm_reset_control_get(&pdev->dev, NULL);
	}
#endif

	info->iommu_master_id = get_iommu_master_id("g2d");
	drv_g2d_init();
	mutex_init(&info->mutex);
	mutex_init(&global_lock);

	ret = sysfs_create_group(&g2d_dev->kobj, &g2d_attribute_group);
	if (ret < 0)
		G2D_ERR("sysfs_create_file fail\n");
#if (IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK) && !IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS))
	/* to avoid g2d used by rv but no power */
	pm_runtime_get_sync(para.dev);
#endif

	return 0;

out_dispose_mapping:
#if !defined(CONFIG_OF)
	irq_dispose_mapping(info->irq);
#endif
release_regs:
#if !defined(CONFIG_OF)
	iounmap(info->io);
#endif
dealloc_fb:
	platform_set_drvdata(pdev, NULL);

	return ret;
}

static int g2d_remove(struct platform_device *pdev)
{
#if (IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK) && !IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS))
	pm_runtime_put_sync(para.dev);
#endif
#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
	if (hwlock)
		hwspin_lock_free(hwlock);
#endif
#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_disable(para.dev);
#endif

	free_irq(para.irq, NULL);

#if !defined(CONFIG_OF)
	iounmap(para.io);
#endif
	platform_set_drvdata(pdev, NULL);

	sysfs_remove_group(&g2d_dev->kobj, &g2d_attribute_group);

	G2D_INFO("Driver unloaded succesfully.\n");
	return 0;
}

int g2d_suspend(struct device *dev)
{
	int ret = 0;
	mutex_lock(&para.mutex);
	if (para.opened) {
		ret = pm_runtime_force_suspend(dev);
	}
	mutex_unlock(&para.mutex);
	G2D_INFO("g2d_suspend succesfully.\n");

	return ret;
}

int g2d_resume(struct device *dev)
{
	int ret = 0;
	mutex_lock(&para.mutex);
	if (para.opened) {
		ret = pm_runtime_force_resume(dev);
	}
	mutex_unlock(&para.mutex);
	G2D_INFO("g2d_resume succesfully.\n");

	return ret;
}

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
static int g2d_runtime_resume(struct device *dev)
{

	g2d_clock_enable(&para);

#ifdef G2D_V2X_SUPPORT
	g2d_bsp_open();
#endif

	return 0;
}

static int g2d_runtime_suspend(struct device *dev)
{

	g2d_clock_disable(&para);

#ifdef G2D_V2X_SUPPORT
	g2d_bsp_close();
#endif

	return 0;
}
#endif

static const struct dev_pm_ops g2d_pm_ops = {
	.suspend = g2d_suspend,
	.resume = g2d_resume,

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	.runtime_suspend = g2d_runtime_suspend,
	.runtime_resume = g2d_runtime_resume,
#endif

};
#if !defined(CONFIG_OF)
struct platform_device g2d_device = {

	.name = "g2d",
	.id = -1,
	.num_resources = ARRAY_SIZE(g2d_resource),
	.resource = g2d_resource,
	.dev = {

		},
};
#else
static const struct of_device_id sunxi_g2d_match[] = {
	{.compatible = "allwinner,sunxi-g2d",},
	{},
};
#endif

static struct platform_driver g2d_driver = {
	.probe = g2d_probe,
	.remove = g2d_remove,
	.driver = {

		   .owner = THIS_MODULE,
		   .name = "g2d",
		   .pm   = &g2d_pm_ops,
		   .of_match_table = sunxi_g2d_match,
		   },
};

int __init g2d_module_init(void)
{
	int ret = 0, err;

	alloc_chrdev_region(&devid, 0, 1, "g2d_chrdev");
	if (g2d_cdev) {
		kfree(g2d_cdev);
		g2d_cdev = NULL;
	}
	g2d_cdev = cdev_alloc();
	cdev_init(g2d_cdev, &g2d_fops);
	g2d_cdev->owner = THIS_MODULE;
	err = cdev_add(g2d_cdev, devid, 1);
	if (err) {
		G2D_ERR("I was assigned major number %d\n", MAJOR(devid));
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	g2d_class = class_create(THIS_MODULE, "g2d");
#else
	g2d_class = class_create("g2d");
#endif
	if (IS_ERR(g2d_class)) {
		G2D_ERR("create class error\n");
		return -1;
	}

	g2d_dev = device_create(g2d_class, NULL, devid, NULL, "g2d");
#if !defined(CONFIG_OF)
	ret = platform_device_register(&g2d_device);
#endif
	if (ret == 0)
		ret = platform_driver_register(&g2d_driver);

#if IS_ENABLED(CONFIG_G2D_SYNCFENCE)
	syncfence_init();
#endif

	G2D_INFO("Module initialized.major:%d\n", MAJOR(devid));
	return ret;
}

static void __exit g2d_module_exit(void)
{
	G2D_INFO("g2d_module_exit\n");
	/* kfree(g2d_ext_hd.g2d_finished_sem); */

#if IS_ENABLED(CONFIG_G2D_SYNCFENCE)
	syncfence_exit();
#endif

	platform_driver_unregister(&g2d_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&g2d_device);
#endif
	device_destroy(g2d_class, devid);
	class_destroy(g2d_class);

	cdev_del(g2d_cdev);
}
/*subsys_initcall_sync(g2d_module_init);*/
module_init(g2d_module_init);
module_exit(g2d_module_exit);

MODULE_AUTHOR("yupu_tang");
MODULE_AUTHOR("tyle <tyle@allwinnertech.com>");
MODULE_DESCRIPTION("g2d driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_IMPORT_NS(DMA_BUF);

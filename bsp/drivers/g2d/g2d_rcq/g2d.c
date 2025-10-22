/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "g2d_driver_i.h"
#include "g2d_top.h"
#include "g2d_debug.h"
#if IS_ENABLED(CONFIG_G2D_MIXER)
#include "g2d_mixer.h"
#endif
#if IS_ENABLED(CONFIG_G2D_ROTATE)
#include "g2d_rotate.h"
#endif
#include "linux/pm_runtime.h"
#include "linux/pm_domain.h"
#include "linux/hwspinlock.h"
#include "../g2d_buf_cache.h"

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
#define PM_ARRAY_SIZE  8
#endif

#if IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK)
static int g2d_hwspinlock_id = 23;
struct hwspinlock *hwlock;
unsigned long hwspinlock_flag;
#endif

#if IS_ENABLED(CONFIG_G2D_SYNCFENCE)
extern int syncfence_init(void);
extern void syncfence_exit(void);
#endif

/* alloc based on 4K byte */
#define G2D_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)
#define ALLOC_USING_DMA
static enum g2d_scan_order scan_order;
static struct mutex global_lock;

static struct class *g2d_class;
static struct cdev *g2d_cdev;
static dev_t devid;
static struct device *g2d_dev;
static struct device *dmabuf_dev;

u32 g_time_info;
struct g2d_time_info g2d_time_inf;

__g2d_drv_t g2d_ext_hd;
__g2d_info_t para;

__u32 dbg_info;

static struct g2d_format_attr fmt_attr_tbl[] = {
/*
format  bits hor_rsample(u,v) ver_rsample(u,v) uvc interleave factor div
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

static __u32 g2d_time_count(bool if_start)
{
	if (g_time_info == 1) {
		if (if_start) {
			memset(&g2d_time_inf, 0, sizeof(struct g2d_time_info));
			ktime_get_real_ts64(&(g2d_time_inf.time_start));
			return 0;
		} else {
			__u32 func_runtime;
			ktime_get_real_ts64(&(g2d_time_inf.time_end));
			func_runtime = (g2d_time_inf.time_end.tv_sec - g2d_time_inf.time_start.tv_sec) * 1000000 +
				(g2d_time_inf.time_end.tv_nsec - g2d_time_inf.time_start.tv_nsec) / NSEC_PER_USEC;
			return func_runtime;
		}
	}
	return 0;
}

void *g2d_malloc(__u32 bytes_num, __u32 *phy_addr)
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
			return address;
		}
		G2D_WARN("dma_alloc_coherent fail, size=0x%x\n", bytes_num);
		return NULL;
	}
	G2D_WARN("reuquet memory size is zero\n");
#else
	unsigned int map_size = 0;
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
	G2D_WARN("size is zero\n");
#endif

	return NULL;
}

void g2d_free(void *virt_addr, uintptr_t phy_addr, unsigned int size)
{
#ifdef ALLOC_USING_DMA
	u32 actual_bytes;

	actual_bytes = PAGE_ALIGN(size);
	if (phy_addr && virt_addr)
		dma_free_coherent(para.dev, actual_bytes, virt_addr,
				  (dma_addr_t) phy_addr);
#else
	unsigned int map_size = PAGE_ALIGN(size);
	unsigned int page_size = map_size;

	if (virt_addr == NULL)
		return;

	free_pages((unsigned long)virt_addr, get_order(page_size));
#endif
}


#if IS_ENABLED(CONFIG_G2D_BUF_CACHED)

int g2d_dma_map(int fd, struct dmabuf_item *item)
{
	int ret = 0;

	if (fd < 0 || !item) {
		G2D_ERR("g2d_dma_map error, invalid dma-buf fd or param\n");
		return -EINVAL;
	}

	ret = g2d_buf_cached_map(fd, &item->dma_addr);
	if (ret == 0) {
		item->fd = fd;
		item->buf = NULL;
		item->sgt = NULL;
		item->attachment = NULL;
		return 0;
	}

	G2D_ERR("g2d_buf_cached_map error, ret=%d\n", ret);
	return ret;
}

void g2d_dma_unmap(struct dmabuf_item *item)
{
	int ret = 0;

	if (!item || item->fd < 0) {
		G2D_ERR("g2d_dma_unmap error, invalid dma-buf fd or param\n");
		return;
	}

	ret = g2d_buf_cached_unmap(item->fd, item->dma_addr);
	if (ret != 0) {
		G2D_ERR("g2d_buf_cached_unmap error, ret=%d\n", ret);
	}

	return;
}

#else

int g2d_dma_map(int fd, struct dmabuf_item *item)
{
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	g2d_time_inf.dma_map_start = g2d_time_count(false);
	if (fd < 0) {
		G2D_WARN("dma_buf_id %d is invalid\n", fd);
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
	g2d_time_inf.dma_map_end = g2d_time_count(false);
	return ret;
}

void g2d_dma_unmap(struct dmabuf_item *item)
{
	g2d_time_inf.dma_unmap_start = g2d_time_count(false);
	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(item->buf, item->attachment);
	dma_buf_put(item->buf);
	g2d_time_inf.dma_unmap_end = g2d_time_count(false);
}

#endif // CONFIG_G2D_BUF_CACHED

__s32 g2d_set_info(g2d_image_enh *g2d_img, struct dmabuf_item *item)
{
	__s32 ret = -1;
	__u32 i = 0;
	__u32 len = ARRAY_SIZE(fmt_attr_tbl);
	__u32 y_width, y_height, u_width, u_height;
	__u32 y_pitch, u_pitch;
	__u32 y_size, u_size;

	g2d_img->laddr[0] = item->dma_addr;

	if (g2d_img->format >= G2D_FORMAT_MAX) {
		G2D_WARN("format 0x%x is out of range\n", g2d_img->format);
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

__s32 g2d_byte_cal(__u32 format, __u32 *ycnt, __u32 *ucnt, __u32 *vcnt)
{
	*ycnt = 0;
	*ucnt = 0;
	*vcnt = 0;
	if (format <= G2D_FORMAT_BGRX8888)
		*ycnt = 4;

	else if (format <= G2D_FORMAT_BGR888)
		*ycnt = 3;

	else if (format <= G2D_FORMAT_BGRA5551)
		*ycnt = 2;

	else if (format <= G2D_FORMAT_BGRA1010102)
		*ycnt = 4;

	else if (format <= 0x23) {
		*ycnt = 2;
	}

	else if (format <= 0x25) {
		*ycnt = 1;
		*ucnt = 2;
	}

	else if (format == 0x26) {
		*ycnt = 1;
		*ucnt = 1;
		*vcnt = 1;
	}

	else if (format <= 0x29) {
		*ycnt = 1;
		*ucnt = 2;
	}

	else if (format == 0x2a) {
		*ycnt = 1;
		*ucnt = 1;
		*vcnt = 1;
	}

	else if (format <= 0x2d) {
		*ycnt = 1;
		*ucnt = 2;
	}

	else if (format == 0x2e) {
		*ycnt = 1;
		*ucnt = 1;
		*vcnt = 1;
	}

	else if (format == 0x30)
		*ycnt = 1;

	else if (format <= 0x36) {
		*ycnt = 2;
		*ucnt = 4;
	}

	else if (format <= 0x39)
		*ycnt = 6;
	return 0;
}


/*
 */
__u32 cal_align(__u32 width, __u32 align)
{
	int slide = 0;
	int number = 2;

	switch (align) {
	case 0:
		return width;
	case 2:
		return (width + 1) >> 1 << 1;
	case 4:
		return (width + 3) >> 2 << 2;
	case 8:
		return (width + 7) >> 3 << 3;
	case 16:
		return (width + 15) >> 4 << 4;
	case 32:
		return (width + 31) >> 5 << 5;
	case 64:
		return (width + 63) >> 6 << 6;
	case 128:
		return (width + 127) >> 7 << 7;
	default:
		while (number <= align) {
			number = number << 1;
			slide  = slide + 1;
		}
		return ((width + align - 1) >> slide << slide);
	}
}


__s32 g2d_image_check(g2d_image_enh *p_image)
{
	__s32 ret = -EINVAL;

	if (!p_image) {
		G2D_WARN("NUll pointer\n");
		goto OUT;
	}

	if (((p_image->clip_rect.x < 0) &&
	     ((-p_image->clip_rect.x) > p_image->clip_rect.w)) ||
	    ((p_image->clip_rect.y < 0) &&
	     ((-p_image->clip_rect.y) > p_image->clip_rect.h)) ||
	    ((p_image->clip_rect.x > 0) &&
	     (p_image->clip_rect.x > p_image->width - 1)) ||
	    ((p_image->clip_rect.y > 0) &&
	     (p_image->clip_rect.y > p_image->height - 1))) {
		G2D_WARN("Invalid imager parameter setting\n");
		goto OUT;
	}

	if (((p_image->clip_rect.x < 0) &&
				((-p_image->clip_rect.x) <
				 p_image->clip_rect.w))) {
		p_image->clip_rect.w =
			p_image->clip_rect.w +
			p_image->clip_rect.x;
		p_image->clip_rect.x = 0;
	} else if ((p_image->clip_rect.x +
				p_image->clip_rect.w)
			> p_image->width) {
		p_image->clip_rect.w =
			p_image->width -
			p_image->clip_rect.x;
	}
	if (((p_image->clip_rect.y < 0) &&
				((-p_image->clip_rect.y) <
				 p_image->clip_rect.h))) {
		p_image->clip_rect.h =
			p_image->clip_rect.h +
			p_image->clip_rect.y;
		p_image->clip_rect.y = 0;
	} else if ((p_image->clip_rect.y +
				p_image->clip_rect.h)
			> p_image->height) {
		p_image->clip_rect.h =
			p_image->height -
			p_image->clip_rect.y;
	}

	p_image->bpremul = 0;
	/* Setted by user. */
	/* p_image->bbuff = 1; */
	ret = 0;
OUT:
	return ret;

}

static void g2d_dump_time_info(void)
{
	if (g_time_info == 1) {
		G2D_INFO("g2d unlock use           %u us\n",
			g2d_time_inf.waiting_runtime);
		G2D_INFO("g2d dma_map use       %u us\n",
			g2d_time_inf.dma_map_end - g2d_time_inf.dma_map_start);
		G2D_INFO("g2d para_config use      %u us\n",
			g2d_time_inf.para_config_runtime - g2d_time_inf.waiting_runtime);
		G2D_INFO("g2d hardware_process use %u us\n",
			g2d_time_inf.hardware_process_runtime - g2d_time_inf.para_config_runtime);
		G2D_INFO("g2d dma_unmap use       %u us\n",
			g2d_time_inf.dma_unmap_end - g2d_time_inf.dma_unmap_start);
		G2D_INFO("g2d total_time use       %u us\n",
			g2d_time_inf.total_runtime);
	}
}

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
	if (info->ahb_clk) {
		ret |= clk_prepare(info->ahb_clk);
	}
	if (info->mbus_vo_clk) {
		ret |= clk_prepare(info->mbus_vo_clk);
	}
	if (info->mbus_desys_clk) {
		ret |= clk_prepare(info->mbus_desys_clk);
	}
	if (info->ahb_de_clk) {
		ret |= clk_prepare(info->ahb_de_clk);
	}
	if (info->vo_clk) {
		ret |= clk_prepare(info->vo_clk);
	}
	if (info->hb_clk) {
		ret |= clk_prepare(info->hb_clk);
	}
	if (ret != 0)
		G2D_ERR("clock prepare error\n");

	return ret;
}

static int g2d_clock_enable(__g2d_info_t *info)
{
	int ret = 0;

	if (info->vo_reset) {
		ret |= reset_control_deassert(info->vo_reset);
		if (ret != 0) {
			G2D_ERR("deassert vo_reset error\n");
			return ret;
		}
	}
	if (info->reset) {
		ret = reset_control_deassert(info->reset);
		if (ret != 0) {
			G2D_ERR("deassert error\n");
			return ret;
		}
	}
	if (info->desys_reset) {
		ret |= reset_control_deassert(info->desys_reset);
		if (ret != 0) {
			G2D_ERR("deassert desys_reset error\n");
			return ret;
		}
	}

	if (info->mbus_vo_clk)
		ret |= clk_enable(info->mbus_vo_clk);
	if (info->vo_clk)
		ret |= clk_enable(info->vo_clk);
	if (info->mbus_desys_clk)
		ret |= clk_enable(info->mbus_desys_clk);
	if (info->ahb_de_clk)
		ret |= clk_enable(info->ahb_de_clk);
	if (info->ahb_clk)
		ret |= clk_enable(info->ahb_clk);
	if (info->hb_clk)
		ret |= clk_enable(info->hb_clk);
	if (info->mbus_clk)
		ret |= clk_enable(info->mbus_clk);
	if (info->bus_clk)
		ret |=  clk_enable(info->bus_clk);
	if (info->clk)
		ret |= clk_enable(info->clk);
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
	if (info->hb_clk)
		clk_unprepare(info->hb_clk);
	if (info->ahb_clk)
		clk_unprepare(info->ahb_clk);
	if (info->vo_clk)
		clk_unprepare(info->vo_clk);
	if (info->mbus_vo_clk)
		clk_unprepare(info->mbus_vo_clk);
	if (info->mbus_desys_clk)
		clk_unprepare(info->mbus_desys_clk);
	if (info->ahb_de_clk)
		clk_unprepare(info->ahb_de_clk);
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
	if (info->hb_clk)
		clk_disable(info->hb_clk);
	if (info->ahb_clk)
		clk_disable(info->ahb_clk);
	if (info->vo_clk)
		clk_disable(info->vo_clk);
	if (info->mbus_vo_clk)
		clk_disable(info->mbus_vo_clk);
	if (info->mbus_desys_clk)
		clk_disable(info->mbus_desys_clk);
	if (info->ahb_de_clk)
		clk_disable(info->ahb_de_clk);

	if (info->reset)
		reset_control_assert(info->reset);
	if (info->vo_reset)
		reset_control_assert(info->vo_reset);
	if (info->desys_reset)
		reset_control_assert(info->desys_reset);
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
#ifndef CONFIG_PM_GENERIC_DOMAINS
		g2d_bsp_open();
#endif
	}

	mutex_unlock(&para.mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(g2d_open);

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
#ifndef CONFIG_PM_GENERIC_DOMAINS
		g2d_bsp_close();
#endif
	}

	mutex_unlock(&para.mutex);

	mutex_lock(&global_lock);
	scan_order = G2D_SM_TDLR;
	mutex_unlock(&global_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(g2d_release);

int g2d_blit_h(g2d_blt_h *para)
{
	int ret = -1;
#if IS_ENABLED(CONFIG_G2D_ROTATE)
	ret = g2d_rotate_set_para(&para->src_image_h,
			    &para->dst_image_h,
			    para->flag_h);
#else
	G2D_ERR("Please enable CONFIG_G2D_ROTATE\n");
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(g2d_blit_h);

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

#if IS_ENABLED(CONFIG_AW_IOMMU) && (IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1))
extern void sunxi_reset_device_iommu(unsigned int master_id);
#endif

int g2d_wait_cmd_finish(unsigned int timeout)
{
	g2d_time_inf.para_config_runtime = g2d_time_count(false);
	timeout = wait_event_timeout(g2d_ext_hd.queue,
				     g2d_ext_hd.finish_flag == 1,
				     msecs_to_jiffies(timeout));
	g2d_time_inf.hardware_process_runtime = g2d_time_count(false);
	if (timeout == 0) {
		g2d_bsp_reset();
		G2D_ERR("G2D irq pending flag timeout\n");

		/* reset iommu */
#if IS_ENABLED(CONFIG_AW_IOMMU) && (IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1))
		sunxi_reset_device_iommu(G2D_IOMMU_MASTER_ID);
#endif
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
		return -1;
	}
	g2d_ext_hd.finish_flag = 0;

	return 0;
}

irqreturn_t g2d_handle_irq(int irq, void *dev_id)
{

#if IS_ENABLED(CONFIG_G2D_MIXER)
#if G2D_MIXER_RCQ_USED == 1
	if (g2d_top_rcq_task_irq_query()) {
		g2d_top_mixer_reset();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
		return IRQ_HANDLED;
	}
#else
	if (g2d_mixer_irq_query()) {
		g2d_top_mixer_reset();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
		return IRQ_HANDLED;
	}
#endif
#endif

#if IS_ENABLED(CONFIG_G2D_ROTATE)
	if (g2d_rot_irq_query()) {
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
		return IRQ_HANDLED;
	}
#endif

	return IRQ_HANDLED;
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
	int ret = -1;

	g2d_time_count(true);
	ret = g2d_ioctl_mutex_lock();
	g2d_time_inf.waiting_runtime = g2d_time_count(false);

	if (ret < 0)
		return -EFAULT;

	g2d_ext_hd.finish_flag = 0;

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_get_sync(para.dev);
#endif

	switch (cmd) {
	case G2D_CMD_MIXER_TASK:
		{
#if IS_ENABLED(CONFIG_G2D_MIXER)
		int i;
		struct mixer_para *p_mixer_para = NULL;
		unsigned long karg[2];
		unsigned long ubuffer[2] = { 0 };

		if (copy_from_user((void *)karg, (void __user *)arg,
				   sizeof(unsigned long) * 2)) {
			ret = -EFAULT;
			goto err_noput;
		}
		ubuffer[0] = *(unsigned long *)karg;
		ubuffer[1] = (*(unsigned long *)(karg + 1));

		p_mixer_para = kmalloc(sizeof(*p_mixer_para) * ubuffer[1],
			       GFP_KERNEL | __GFP_ZERO);
		if (!p_mixer_para)
			goto err_noput;
		if (copy_from_user(p_mixer_para, (void __user *)ubuffer[0],
				   sizeof(*p_mixer_para) * ubuffer[1])) {
			ret = -EFAULT;
			goto err_noput;
		}
		if (dbg_info) {
			for (i = 0; i < ubuffer[1]; i++) {
				dump_mixer_para_info(&(p_mixer_para[i]));
			}
		}
		ret  = mixer_task_process(&para, p_mixer_para, ubuffer[1]);
		kfree(p_mixer_para);
#endif
		break;
		}
	case G2D_CMD_CREATE_TASK:
		{
#if IS_ENABLED(CONFIG_G2D_MIXER)
			int i;
			struct mixer_para *p_mixer_para = NULL;
			unsigned long karg[2];
			unsigned long ubuffer[2] = { 0 };

			if (copy_from_user((void *)karg, (void __user *)arg,
					   sizeof(unsigned long) * 2)) {
				ret = -EFAULT;
				goto err_noput;
			}
			ubuffer[0] = *(unsigned long *)karg;
			ubuffer[1] = (*(unsigned long *)(karg + 1));
			p_mixer_para = kmalloc(sizeof(*p_mixer_para) * ubuffer[1],
					       GFP_KERNEL | __GFP_ZERO);
			if (!p_mixer_para)
				goto err_noput;
			if (copy_from_user(p_mixer_para, (void __user *)ubuffer[0],
					   sizeof(*p_mixer_para) * ubuffer[1])) {
				ret = -EFAULT;
				goto err_noput;
			}
			if (dbg_info) {
				for (i = 0; i < ubuffer[1]; i++) {
					dump_mixer_para_info(&(p_mixer_para[i]));
				}
			}
			ret = create_mixer_task(&para, p_mixer_para, ubuffer[1]);
			if (copy_to_user((void __user *)ubuffer[0], p_mixer_para,
					 sizeof(*p_mixer_para) * ubuffer[1])) {
				G2D_WARN("copy_to_user fail\n");
				return  -EFAULT;
			}
			kfree(p_mixer_para);
#endif
			break;
		}
	case G2D_CMD_TASK_APPLY:
		{
#if IS_ENABLED(CONFIG_G2D_MIXER)
			unsigned long karg[1];
			unsigned long ubuffer[1] = { 0 };
			struct g2d_mixer_task *p_task = NULL;

			if (copy_from_user((void *)karg, (void __user *)arg,
					   sizeof(unsigned long))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ubuffer[0] = *(unsigned long *)karg;
			p_task = g2d_mixer_get_inst(ubuffer[0]);
			if (!p_task) {
				ret = -EFAULT;
				goto err_noput;
			}
			ret = p_task->apply(p_task);
#endif
			break;
		}
	case G2D_CMD_TASK_DESTROY:
		{
#if IS_ENABLED(CONFIG_G2D_MIXER)
			unsigned long karg[1];
			unsigned long ubuffer[1] = { 0 };
			struct g2d_mixer_task *p_task = NULL;

			if (copy_from_user((void *)karg, (void __user *)arg,
					   sizeof(unsigned long))) {
				ret = -EFAULT;
				goto err_noput;
			}
			ubuffer[0] = *(unsigned long *)karg;
			p_task = g2d_mixer_get_inst(ubuffer[0]);

			if (!p_task) {
				ret = -EFAULT;
				G2D_WARN("Fail to find mixer task inst:%lu\n", ubuffer[0]);
				goto err_noput;
			}

			ret = p_task->destory(p_task);
#endif
			break;
		}
	case G2D_CMD_TASK_GET_PARA:
		{
#if IS_ENABLED(CONFIG_G2D_MIXER)
			struct g2d_mixer_task *p_task = NULL;
			unsigned long karg[2];
			unsigned long ubuffer[2] = { 0 };

			if (copy_from_user((void *)karg, (void __user *)arg,
					   sizeof(unsigned long) * 2)) {
				ret = -EFAULT;
				goto err_noput;
			}
			ubuffer[0] = *(unsigned long *)karg;
			ubuffer[1] = (*(unsigned long *)(karg + 1));

			p_task = g2d_mixer_get_inst(ubuffer[0]);
			if (!p_task || !ubuffer[1]) {
				ret = -EFAULT;
				goto err_noput;
			}

			ret = copy_to_user(
			    (void __user *)ubuffer[1], p_task->p_para,
			    sizeof(*(p_task->p_para)) * p_task->frame_cnt);
#endif
			break;
		}
	case G2D_CMD_BITBLT_H:
		{
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
		if (dbg_info) {
			dump_g2d_blt_h_info(blit_para);
		}
		if (blit_para->flag_h & 0xff00) {
			ret = g2d_blit_h(blit_para);
		}
#if IS_ENABLED(CONFIG_G2D_MIXER)
		else {
			struct mixer_para *mixer_blit_para = (struct mixer_para *)
				kmalloc(sizeof(struct mixer_para), GFP_KERNEL);
			/* mixer module */
			memset(mixer_blit_para, 0, sizeof(*mixer_blit_para));
			memcpy(&(mixer_blit_para->dst_image_h),
			       &(blit_para->dst_image_h), sizeof(g2d_image_enh));
			memcpy(&(mixer_blit_para->src_image_h),
			       &(blit_para->src_image_h), sizeof(g2d_image_enh));
			mixer_blit_para->flag_h = blit_para->flag_h;
			mixer_blit_para->op_flag = OP_BITBLT;
			ret = mixer_task_process(&para, mixer_blit_para, 1);
			kfree(mixer_blit_para);
		}

#endif
		kfree(blit_para);
		break;
		}
	case G2D_CMD_LBC_ROT:
		{
			g2d_lbc_rot lbc_para;

			if (copy_from_user(&lbc_para, (g2d_lbc_rot *)arg,
						sizeof(g2d_lbc_rot))) {
				ret = -EFAULT;
				goto err_noput;
			}
			if (dbg_info) {
				dump_g2d_lbc_rot_info(&lbc_para);
			}

			ret = g2d_lbc_rot_set_para(&lbc_para);
			break;
		}

	case G2D_CMD_BLD_H:{
#if IS_ENABLED(CONFIG_G2D_MIXER)
			g2d_bld *bld_para = NULL;
			struct mixer_para *mixer_bld_para = NULL;

			bld_para = kmalloc(sizeof(*bld_para), GFP_KERNEL);
			if (!bld_para) {
				G2D_WARN("bld_para kmalloc failed\n");
				ret = -EFAULT;
				goto err_noput;
			}
			mixer_bld_para = kmalloc(sizeof(*mixer_bld_para), GFP_KERNEL);
			if (!mixer_bld_para) {
				G2D_WARN("mixer_bld_para kmalloc failed\n");
				ret = -EFAULT;
				kfree(bld_para);
				goto err_noput;
			}
			if (copy_from_user(bld_para, (g2d_bld *) arg,
					   sizeof(g2d_bld))) {
				ret = -EFAULT;
				kfree(bld_para);
				kfree(mixer_bld_para);
				goto err_noput;
			}
			if (dbg_info) {
				dump_g2d_bld_info(bld_para);
			}
			memset(mixer_bld_para, 0, sizeof(*mixer_bld_para));
			memcpy(&mixer_bld_para->dst_image_h,
			       &bld_para->dst_image, sizeof(g2d_image_enh));
			memcpy(&mixer_bld_para->src_image_h,
			       &bld_para->src_image[0], sizeof(g2d_image_enh));
			/* ptn use as src */
			memcpy(&mixer_bld_para->ptn_image_h,
			       &bld_para->src_image[1], sizeof(g2d_image_enh));
			memcpy(&mixer_bld_para->ck_para, &bld_para->ck_para,
			       sizeof(g2d_ck));
			mixer_bld_para->bld_cmd = bld_para->bld_cmd;
			mixer_bld_para->op_flag = OP_BLEND;

			ret  = mixer_task_process(&para, mixer_bld_para, 1);
			kfree(bld_para);
			kfree(mixer_bld_para);
#endif
			break;
		}
	case G2D_CMD_FILLRECT_H:{
#if IS_ENABLED(CONFIG_G2D_MIXER)
		g2d_fillrect_h fill_para;
		struct mixer_para mixer_fill_para;

		if (copy_from_user(&fill_para, (g2d_fillrect_h *) arg,
				   sizeof(g2d_fillrect_h))) {
			ret = -EFAULT;
			goto err_noput;
		}
		if (dbg_info) {
			dump_g2d_fillrect_h_info(&fill_para);
		}
		memset(&mixer_fill_para, 0, sizeof(mixer_fill_para));
		memcpy(&mixer_fill_para.dst_image_h,
		       &fill_para.dst_image_h, sizeof(g2d_image_enh));
		mixer_fill_para.op_flag = OP_FILLRECT;

		ret  = mixer_task_process(&para, &mixer_fill_para, 1);
#endif
		break;
	}
	case G2D_CMD_MASK_H:{
#if IS_ENABLED(CONFIG_G2D_MIXER)
			g2d_maskblt mask_para;
			struct mixer_para mixer_mask_para;

			if (copy_from_user(&mask_para, (g2d_maskblt *) arg,
					   sizeof(g2d_maskblt))) {
				ret = -EFAULT;
				goto err_noput;
			}
			if (dbg_info) {
				dump_g2d_maskblt_info(&mask_para);
			}
			memset(&mixer_mask_para, 0, sizeof(mixer_mask_para));
			memcpy(&mixer_mask_para.ptn_image_h,
			       &mask_para.ptn_image_h, sizeof(g2d_image_enh));
			memcpy(&mixer_mask_para.mask_image_h,
			       &mask_para.mask_image_h, sizeof(g2d_image_enh));
			memcpy(&mixer_mask_para.dst_image_h,
			       &mask_para.dst_image_h, sizeof(g2d_image_enh));
			memcpy(&mixer_mask_para.src_image_h,
			       &mask_para.src_image_h, sizeof(g2d_image_enh));
			mixer_mask_para.back_flag = mask_para.back_flag;
			mixer_mask_para.fore_flag = mask_para.fore_flag;
			mixer_mask_para.op_flag = OP_MASK;

			ret  = mixer_task_process(&para, &mixer_mask_para, 1);
#endif
			break;
		}
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

	default:
		goto err_noput;
		break;
	}

err_noput:

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
	pm_runtime_put_sync(para.dev);
#endif
	g2d_ioctl_mutex_unlock();
	g2d_time_inf.total_runtime = g2d_time_count(false);
	g2d_dump_time_info();
	return ret;
}

__s32 drv_g2d_init(void)
{
	memset(&g2d_ext_hd, 0, sizeof(__g2d_drv_t));
	init_waitqueue_head(&g2d_ext_hd.queue);
	g2d_top_set_base((unsigned long) para.io);

#if IS_ENABLED(CONFIG_G2D_ROTATE)
	g2d_rot_set_base((unsigned long) para.io);
#endif
	return 0;
}

static ssize_t g2d_debug_show(struct device *dev,
							  struct device_attribute *attr, char *buf)
{
	ssize_t wc = 0;
	wc += sprintf(buf + wc, "debug=%d\n\n", dbg_info);

#if IS_ENABLED(CONFIG_G2D_BUF_CACHED)
	wc += g2d_buf_cache_debug_show(buf + wc);
#endif

	return wc;
}

static ssize_t g2d_debug_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "0", 1) == 0)
		dbg_info = 0;
	else if (strncasecmp(buf, "1", 1) == 0)
		dbg_info = 1;
	else if (strncasecmp(buf, "2", 1) == 0)
		dbg_info = 2;
	else
		G2D_WARN("Error input\n");

	return count;
}

static DEVICE_ATTR(debug, 0660,
		   g2d_debug_show, g2d_debug_store);

static ssize_t g2d_func_runtime_show(struct device *dev,
									 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "func_runtime = %d us\n", g2d_time_inf.total_runtime);
}

static ssize_t g2d_func_runtime_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "1", 1) == 0)
		g_time_info = 1;
	else if (strncasecmp(buf, "0", 1) == 0)
		g_time_info = 0;
	else
		G2D_WARN("Error input\n");

	return count;
}

static DEVICE_ATTR(func_runtime, 0660,
		   g2d_func_runtime_show, g2d_func_runtime_store);

static struct attribute *g2d_attributes[] = {
	&dev_attr_debug.attr,
	&dev_attr_func_runtime.attr,
	NULL
};

static struct attribute_group g2d_attribute_group = {
	.name = "attr",
	.attrs = g2d_attributes
};

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
			G2D_ERR("Failed to add device_link to %s\n", values_of_power_domain_names[i]);
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

	info->io = of_iomap(pdev->dev.of_node, 0);
	if (info->io == NULL) {
		G2D_ERR("iormap() of register failed\n");
		ret = -ENXIO;
		goto dealloc_fb;
	}

	info->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!info->irq) {
		G2D_ERR("irq_of_parse_and_map irq fail for transform\n");
		ret = -ENXIO;
		goto release_regs;
	}

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
	/* clk init */
	info->clk = devm_clk_get(&pdev->dev, "g2d");
	if (IS_ERR(info->clk)) {
		G2D_ERR("fail to get clk\n");
		ret = -ENXIO;
		goto out_dispose_mapping;
	} else {
		info->clk_parent = clk_get_parent(info->clk);
		info->bus_clk = devm_clk_get(&pdev->dev, "bus");
		if (IS_ERR(info->bus_clk))
			info->bus_clk = NULL;
		info->mbus_clk = devm_clk_get(&pdev->dev, "mbus_g2d");
		if (IS_ERR(info->mbus_clk))
			info->mbus_clk = NULL;
		info->ahb_clk = devm_clk_get(&pdev->dev, "ahb_g2d");
		if (IS_ERR(info->ahb_clk))
			info->ahb_clk = NULL;
		info->vo_clk = devm_clk_get(&pdev->dev, "vo");
		if (IS_ERR(info->vo_clk))
			info->vo_clk = NULL;
		info->mbus_vo_clk = devm_clk_get(&pdev->dev, "mbus_vo");
		if (IS_ERR(info->mbus_vo_clk))
			info->mbus_vo_clk = NULL;
		info->mbus_desys_clk = devm_clk_get(&pdev->dev, "mbus_desys");
		if (IS_ERR(info->mbus_desys_clk))
			info->mbus_desys_clk = NULL;
		info->ahb_de_clk = devm_clk_get(&pdev->dev, "ahb_de");
		if (IS_ERR(info->ahb_de_clk))
			info->ahb_de_clk = NULL;
		info->hb_clk = devm_clk_get(&pdev->dev, "g2d_hb");
		if (IS_ERR(info->hb_clk))
			info->hb_clk = NULL;
		info->reset = devm_reset_control_get(&pdev->dev, NULL);
		if (IS_ERR(info->reset)) {
			G2D_WARN("reset get failed\n");
			info->reset = NULL;
		}
		info->vo_reset = devm_reset_control_get_optional_shared(&pdev->dev, "rst_bus_vo");
		if (IS_ERR(info->vo_reset)) {
			G2D_WARN("vo_reset get failed\n");
			info->vo_reset = NULL;
		}
		info->desys_reset = devm_reset_control_get_optional_shared(&pdev->dev, "rst_bus_desys");
		if (IS_ERR(info->desys_reset)) {
			G2D_WARN("desys_reset get failed\n");
			info->desys_reset = NULL;
		}
	}

	drv_g2d_init();
	mutex_init(&info->mutex);
	mutex_init(&global_lock);

#if IS_ENABLED(CONFIG_G2D_BUF_CACHED)
	g2d_buf_cache_init(dmabuf_dev, 32);
#endif

	ret = sysfs_create_group(&g2d_dev->kobj, &g2d_attribute_group);
	if (ret < 0)
		G2D_ERR("sysfs_create_file fail\n");

#if (IS_ENABLED(CONFIG_G2D_USE_HWSPINLOCK) && !IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS))
	/* to avoid g2d used by rv but no power */
	pm_runtime_get_sync(para.dev);
#endif
	return 0;

out_dispose_mapping:
	irq_dispose_mapping(info->irq);
release_regs:
	iounmap(info->io);
dealloc_fb:
	platform_set_drvdata(pdev, NULL);

	return ret;
}


static const struct file_operations g2d_fops = {
	.owner = THIS_MODULE,
	.open = g2d_open,
	.release = g2d_release,
	.unlocked_ioctl = g2d_ioctl,
	.mmap = g2d_mmap,
};

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

#if IS_ENABLED(CONFIG_G2D_BUF_CACHED)
	g2d_buf_cache_exit();
#endif

	free_irq(para.irq, NULL);
	platform_set_drvdata(pdev, NULL);

	sysfs_remove_group(&g2d_dev->kobj, &g2d_attribute_group);

	G2D_INFO("Driver unloaded succesfully\n");
	return 0;
}

static int g2d_suspend(struct device *dev)
{
	int ret = 0;
	mutex_lock(&para.mutex);
	if (para.opened) {
		ret = pm_runtime_force_suspend(para.dev);
	}
	mutex_unlock(&para.mutex);
	G2D_INFO("g2d_suspend succesfully\n");

	return ret;
}

static int g2d_resume(struct device *dev)
{
	int ret = 0;
	mutex_lock(&para.mutex);
	if (para.opened) {
		ret = pm_runtime_force_resume(para.dev);
	}
	mutex_unlock(&para.mutex);
	G2D_INFO("g2d_resume succesfully\n");

	return ret;
}

#if IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)
static int g2d_runtime_resume(struct device *dev)
{

	g2d_clock_enable(&para);

	g2d_bsp_open();

	return 0;
}

static int g2d_runtime_suspend(struct device *dev)
{

	g2d_clock_disable(&para);

	g2d_bsp_close();

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

static const struct of_device_id sunxi_g2d_match[] = {
	{.compatible = "allwinner,sunxi-g2d",},
	{},
};

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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	g2d_class = class_create("g2d");
#else
	g2d_class = class_create(THIS_MODULE, "g2d");
#endif
	if (IS_ERR(g2d_class)) {
		G2D_ERR("create class error\n");
		return -1;
	}

	g2d_dev = device_create(g2d_class, NULL, devid, NULL, "g2d");
	if (ret == 0)
		ret = platform_driver_register(&g2d_driver);

#if IS_ENABLED(CONFIG_G2D_SYNCFENCE)
	syncfence_init();
#endif

	G2D_INFO("rcq version initialized.major:%d\n", MAJOR(devid));
	G2D_INFO("g2d_module_init\n");
	return ret;
}

static void __exit g2d_module_exit(void)
{

#if IS_ENABLED(CONFIG_G2D_SYNCFENCE)
	syncfence_exit();
#endif

	platform_driver_unregister(&g2d_driver);
	device_destroy(g2d_class, devid);
	class_destroy(g2d_class);

	cdev_del(g2d_cdev);
	G2D_INFO("g2d_module_exit\n");
}

/*subsys_initcall_sync(g2d_module_init);*/
module_init(g2d_module_init);
module_exit(g2d_module_exit);

MODULE_AUTHOR("zxb <zhengxiaobin@allwinnertech.com>");
MODULE_DESCRIPTION("g2d(rcq) driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(DMA_BUF);

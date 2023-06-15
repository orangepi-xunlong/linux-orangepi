/*
 * eink_fbdev/eink_fbdev.c
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#include "eink_fbdev.h"

#define EINK_FB_MAX 1
#define EINK_BYTE_ALIGN(x, a) (((x) + (a)-1L) & ~((a)-1L))

struct fb_info_t {
	struct device *dev;
	bool fb_enable[EINK_FB_MAX];
	struct fb_info *fbinfo[EINK_FB_MAX];
	int blank[EINK_FB_MAX];
	int fb_index[EINK_FB_MAX];
	u32 pseudo_palette[EINK_FB_MAX][16];
	struct eink_img cur_img[EINK_FB_MAX];
	void *cur_vir_addr[EINK_FB_MAX];
	struct eink_img last_img[EINK_FB_MAX];
	void *last_vir_addr[EINK_FB_MAX];
	unsigned int order;
	struct disp_layer_config_inner eink_para[EINK_FB_MAX];
	struct mutex update_lock;
};

static struct fb_info_t g_fbi;

static void *__dma_malloc(u32 num_bytes, void *phys_addr)
{
	u32 actual_bytes;
	void *address = NULL;

	if (num_bytes != 0) {
		actual_bytes = EINK_BYTE_ALIGN(num_bytes, 4096);

		address =
		    dma_alloc_coherent(g_fbi.dev, actual_bytes,
				       (dma_addr_t *)phys_addr, GFP_KERNEL);
		if (address) {
			return address;
		} else {
			pr_err("dma_alloc_coherent fail, size=0x%x\n",
			       num_bytes);
			return NULL;
		}
	} else {
		pr_err("%s size is zero\n", __func__);
	}

	return NULL;
}

static int fb_map_video_memory(struct fb_info *info)
{
	info->screen_base = (char __iomem *)__dma_malloc(
	    info->fix.smem_len, (u32 *)(&info->fix.smem_start));
	if (info->screen_base) {
		pr_err("%s(reserve),va=0x%p, pa=0x%p size:0x%x\n", __func__,
		       (void *)info->screen_base, (void *)info->fix.smem_start,
		       (unsigned int)info->fix.smem_len);
		memset((void *__force)info->screen_base, 0x0,
		       info->fix.smem_len);

		return 0;
	}

	pr_err("%s fail!\n", __func__);
	return -ENOMEM;

	return 0;
}

static int eink_fb_open(struct fb_info *info, int user)
{
	return 0;
}
static int eink_fb_release(struct fb_info *info, int user)
{
	return 0;
}

static int __eink_update(struct fb_info *info)
{
	struct eink_upd_cfg eink_cfg;
	struct eink_manager *eink_mgr = NULL;
	struct eink_img tmp;
	void *tmp_vir;
	int ret = -1;
	eink_mgr = get_eink_manager();

	mutex_lock(&g_fbi.update_lock);
#ifdef CONFIG_ARM64
	__dma_flush_range((void *)info->screen_base, info->fix.smem_len);
#else
	dmac_flush_range((void *)info->screen_base,
			 (void *)(info->screen_base + info->fix.smem_len));
#endif
	ret = eink_mgr->eink_fmt_cvt_img(
	    (struct disp_layer_config_inner *)&g_fbi.eink_para[info->node],
	    (unsigned int)1, &g_fbi.last_img[info->node],
	    &g_fbi.cur_img[info->node]);

	if (ret || !g_fbi.cur_img[info->node].upd_win.right) {
		goto OUT;
	}
	eink_cfg.order = g_fbi.order;
	memcpy(&eink_cfg.img, &g_fbi.cur_img[info->node],
	       sizeof(struct eink_img));
	eink_cfg.force_fresh = 0;

	if (g_fbi.order == 0) {
		eink_cfg.img.upd_mode = EINK_GC16_MODE;
		eink_cfg.img.upd_all_en = 1;
		eink_cfg.img.upd_win.right = eink_cfg.img.size.width - 1;
		eink_cfg.img.upd_win.bottom = eink_cfg.img.size.height - 1;
		eink_cfg.img.upd_win.left = 0;
		eink_cfg.img.upd_win.top = 0;
	}

	ret = eink_mgr->eink_update(eink_mgr, (struct eink_upd_cfg *)&eink_cfg);
	++g_fbi.order;
OUT:
	memcpy(&tmp, &g_fbi.last_img, sizeof(struct eink_img));
	tmp_vir = g_fbi.last_vir_addr[info->node];
	memcpy(&g_fbi.last_img, &g_fbi.cur_img, sizeof(struct eink_img));
	g_fbi.last_vir_addr[info->node] = g_fbi.cur_vir_addr[info->node];
	memcpy(&g_fbi.cur_img, &tmp, sizeof(struct eink_img));
	g_fbi.cur_vir_addr[info->node] = tmp_vir;
	mutex_unlock(&g_fbi.update_lock);
	return ret;
}

static int eink_fb_pan_display(struct fb_var_screeninfo *var,
			       struct fb_info *info)
{

	g_fbi.eink_para[info->node].info.fb.crop.x = ((long long)var->xoffset)
						     << 32;
	g_fbi.eink_para[info->node].info.fb.crop.y =
	    ((unsigned long long)(var->yoffset)) << 32;
	g_fbi.eink_para[info->node].info.fb.crop.width = ((long long)var->xres)
							 << 32;
	g_fbi.eink_para[info->node].info.fb.crop.height =
	    ((long long)(var->yres)) << 32;
	__eink_update(info);
	return 0;
}

static int eink_fb_set_par(struct fb_info *info)
{
	return 0;
}

static int eink_fb_check_var(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	return 0;
}

static int eink_fb_blank(int blank_mode, struct fb_info *info)
{
	if (blank_mode == FB_BLANK_POWERDOWN)
		return eink_suspend(g_fbi.dev);
	else
		return eink_resume(g_fbi.dev);
}

static inline u32 convert_bitfield(int val, struct fb_bitfield *bf)
{
	u32 mask = ((1 << bf->length) - 1) << bf->offset;

	return (val << bf->offset) & mask;
}

static int eink_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
			     unsigned blue, unsigned transp,
			     struct fb_info *info)
{
	u32 val;
	u32 ret = 0;

	switch (info->fix.visual) {
	case FB_VISUAL_PSEUDOCOLOR:
		ret = -EINVAL;
		break;
	case FB_VISUAL_TRUECOLOR:
		if (regno < 16) {
			val = convert_bitfield(transp, &info->var.transp) |
			      convert_bitfield(red, &info->var.red) |
			      convert_bitfield(green, &info->var.green) |
			      convert_bitfield(blue, &info->var.blue);
			((u32 *)info->pseudo_palette)[regno] = val;
		} else {
			ret = 0;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int eink_fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	unsigned int j, r = 0;
	unsigned char hred, hgreen, hblue, htransp = 0xff;
	unsigned short *red, *green, *blue, *transp;

	red = cmap->red;
	green = cmap->green;
	blue = cmap->blue;
	transp = cmap->transp;

	for (j = 0; j < cmap->len; j++) {
		hred = *red++;
		hgreen = *green++;
		hblue = *blue++;
		if (transp)
			htransp = (*transp++) & 0xff;
		else
			htransp = 0xff;

		r = eink_fb_setcolreg(cmap->start + j, hred, hgreen, hblue,
				      htransp, info);
		if (r)
			return r;
	}

	return 0;
}

static struct fb_ops eink_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = eink_fb_open,
	.fb_release = eink_fb_release,
	.fb_pan_display = eink_fb_pan_display,
	.fb_check_var = eink_fb_check_var,
	.fb_set_par = eink_fb_set_par,
	.fb_blank = eink_fb_blank,
#if defined(CONFIG_FB_CONSOLE_SUNXI)
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
#endif
	.fb_setcmap = eink_fb_setcmap,
	.fb_setcolreg = eink_fb_setcolreg,

};

#ifdef CONFIG_FB_DEFERRED_IO
static void __deferred_io(struct fb_info *info, struct list_head *pagelist)
{
	__eink_update(info);
}
#endif

static int __setup_layer(int fbid)
{
	struct eink_manager *eink_mgr = NULL;
	dma_addr_t phy_addr = 0;

	eink_mgr = get_eink_manager();
	if (!eink_mgr)
		return -1;

	memset(&g_fbi.eink_para[fbid], 0,
	       sizeof(struct disp_layer_config_inner));

	g_fbi.eink_para[fbid].channel = fbid;
	g_fbi.eink_para[fbid].layer_id = 0;
	g_fbi.eink_para[fbid].enable = 1;
	g_fbi.eink_para[fbid].info.mode = LAYER_MODE_BUFFER;
	g_fbi.eink_para[fbid].info.zorder = 16;
	g_fbi.eink_para[fbid].info.alpha_mode = 1;
	g_fbi.eink_para[fbid].info.alpha_value = 0xff;
	g_fbi.eink_para[fbid].info.screen_win.width =
	    eink_mgr->panel_info.width;
	g_fbi.eink_para[fbid].info.screen_win.height =
	    eink_mgr->panel_info.height;
	g_fbi.eink_para[fbid].info.screen_win.x = 0;
	g_fbi.eink_para[fbid].info.screen_win.y = 0;
	g_fbi.eink_para[fbid].info.fb.format = DISP_FORMAT_ARGB_8888;
	g_fbi.eink_para[fbid].info.fb.crop.x = 0;
	g_fbi.eink_para[fbid].info.fb.crop.y = 0;
	g_fbi.eink_para[fbid].info.fb.crop.width =
	    ((long long)eink_mgr->panel_info.width << 32);
	g_fbi.eink_para[fbid].info.fb.crop.height =
	    ((long long)eink_mgr->panel_info.height << 32);
	g_fbi.eink_para[fbid].info.fb.addr[0] =
	    (unsigned long long)g_fbi.fbinfo[fbid]->fix.smem_start;
	g_fbi.eink_para[fbid].info.fb.addr[1] = 0;
	g_fbi.eink_para[fbid].info.fb.addr[2] = 0;
	g_fbi.eink_para[fbid].info.fb.flags = DISP_BF_NORMAL;
	g_fbi.eink_para[fbid].info.fb.scan = DISP_SCAN_PROGRESSIVE;
	g_fbi.eink_para[fbid].info.fb.size[0].width =
	    eink_mgr->panel_info.width;
	g_fbi.eink_para[fbid].info.fb.size[0].height =
	    eink_mgr->panel_info.height;
	g_fbi.eink_para[fbid].info.fb.size[1].width =
	    eink_mgr->panel_info.width;
	g_fbi.eink_para[fbid].info.fb.size[1].height =
	    eink_mgr->panel_info.height;
	g_fbi.eink_para[fbid].info.fb.size[2].width =
	    eink_mgr->panel_info.width;
	g_fbi.eink_para[fbid].info.fb.size[2].height =
	    eink_mgr->panel_info.height;
	g_fbi.eink_para[fbid].info.fb.color_space = DISP_BT601;
	g_fbi.eink_para[fbid].info.fb.fd = -1;

	g_fbi.order = 0;
	memset(&g_fbi.cur_img[fbid], 0, sizeof(struct eink_img));

	g_fbi.cur_img[fbid].out_fmt = EINK_Y8;
	g_fbi.cur_img[fbid].dither_mode = QUANTIZATION;
	g_fbi.cur_img[fbid].upd_mode = EINK_GU16_MODE;
	g_fbi.cur_img[fbid].size.width = eink_mgr->panel_info.width;
	g_fbi.cur_img[fbid].size.height = eink_mgr->panel_info.height;
	g_fbi.cur_img[fbid].size.align = 4;
	g_fbi.cur_img[fbid].upd_all_en = 0;
	g_fbi.cur_img[fbid].win_calc_en = 1;
	g_fbi.cur_img[fbid].pitch = EINK_BYTE_ALIGN(
	    eink_mgr->panel_info.width, g_fbi.cur_img[fbid].size.align);
	g_fbi.cur_vir_addr[fbid] = __dma_malloc(
	    g_fbi.cur_img[fbid].pitch * g_fbi.cur_img[fbid].size.height,
	    (void *)&phy_addr);
	if (!g_fbi.cur_vir_addr[fbid]) {
		pr_err("Malloc cur_img fail!\n");
		return -1;
	}
	g_fbi.cur_img[fbid].fd = -1;
	g_fbi.cur_img[fbid].paddr = (u32)phy_addr;

	memcpy(&g_fbi.last_img[fbid], &g_fbi.cur_img[fbid],
	       sizeof(struct eink_img));

	g_fbi.last_vir_addr[fbid] = __dma_malloc(
	    g_fbi.last_img[fbid].pitch * g_fbi.last_img[fbid].size.height,
	    (void *)&phy_addr);
	if (!g_fbi.last_vir_addr[fbid]) {
		pr_err("Malloc last_img fail!\n");
		return -1;
	}
	g_fbi.last_img[fbid].paddr = (u32)phy_addr;
	memset(g_fbi.last_vir_addr[fbid], 0xff,
	       g_fbi.last_img[fbid].pitch * g_fbi.last_img[fbid].size.height);
	return 0;
}

int eink_fbdev_init(struct device *p_dev)
{
	int i = 0, buffer_num = 2, ret = -1;
	struct eink_manager *eink_mgr = NULL;
#ifdef CONFIG_FB_DEFERRED_IO
	struct fb_deferred_io *fbdefio = NULL;
#endif

	g_fbi.dev = p_dev;

	eink_mgr = get_eink_manager();
	if (!eink_mgr)
		return -1;
	mutex_init(&g_fbi.update_lock);

	for (i = 0; i < EINK_FB_MAX; i++) {
		g_fbi.fbinfo[i] = framebuffer_alloc(0, g_fbi.dev);
		g_fbi.fbinfo[i]->fbops = &eink_fb_ops;
		g_fbi.fbinfo[i]->flags = 0;
		g_fbi.fbinfo[i]->device = g_fbi.dev;
		g_fbi.fbinfo[i]->par = &g_fbi;
		g_fbi.fbinfo[i]->var.xoffset = 0;
		g_fbi.fbinfo[i]->var.yoffset = 0;
		g_fbi.fbinfo[i]->var.nonstd = 0;
		g_fbi.fbinfo[i]->var.bits_per_pixel = 32;
		g_fbi.fbinfo[i]->var.transp.length = 8;
		g_fbi.fbinfo[i]->var.red.length = 8;
		g_fbi.fbinfo[i]->var.green.length = 8;
		g_fbi.fbinfo[i]->var.blue.length = 8;
		g_fbi.fbinfo[i]->var.blue.offset = 0;
		g_fbi.fbinfo[i]->var.green.offset =
		    g_fbi.fbinfo[i]->var.blue.offset +
		    g_fbi.fbinfo[i]->var.blue.length;
		g_fbi.fbinfo[i]->var.red.offset =
		    g_fbi.fbinfo[i]->var.green.offset +
		    g_fbi.fbinfo[i]->var.green.length;
		g_fbi.fbinfo[i]->var.transp.offset =
		    g_fbi.fbinfo[i]->var.red.offset +
		    g_fbi.fbinfo[i]->var.red.length;
		g_fbi.fbinfo[i]->var.activate = FB_ACTIVATE_FORCE;
		g_fbi.fbinfo[i]->fix.type = FB_TYPE_PACKED_PIXELS;
		g_fbi.fbinfo[i]->fix.type_aux = 0;
		g_fbi.fbinfo[i]->fix.visual = FB_VISUAL_TRUECOLOR;
		g_fbi.fbinfo[i]->fix.xpanstep = 1;
		g_fbi.fbinfo[i]->fix.ypanstep = 1;
		g_fbi.fbinfo[i]->fix.ywrapstep = 0;
		g_fbi.fbinfo[i]->fix.accel = FB_ACCEL_NONE;
		g_fbi.fbinfo[i]->fix.line_length =
		    g_fbi.fbinfo[i]->var.xres_virtual * 4;
		g_fbi.fbinfo[i]->fix.smem_len =
		    g_fbi.fbinfo[i]->fix.line_length *
		    g_fbi.fbinfo[i]->var.yres_virtual * 2;
		g_fbi.fbinfo[i]->screen_base = NULL;
		g_fbi.fbinfo[i]->pseudo_palette = g_fbi.pseudo_palette[i];
		g_fbi.fbinfo[i]->fix.smem_start = 0x0;
		g_fbi.fbinfo[i]->fix.mmio_start = 0;
		g_fbi.fbinfo[i]->fix.mmio_len = 0;

		if (fb_alloc_cmap(&g_fbi.fbinfo[i]->cmap, 256, 1) < 0)
			return -ENOMEM;

		buffer_num = 2;

#ifdef CONFIG_FB_DEFERRED_IO
		fbdefio = NULL;
		fbdefio = devm_kzalloc(g_fbi.dev, sizeof(struct fb_deferred_io),
				       GFP_KERNEL);
		if (!fbdefio)
			return -1;
		g_fbi.fbinfo[i]->fbdefio = fbdefio;
		fbdefio->delay = HZ / 16;
		fbdefio->deferred_io = __deferred_io;
		fb_deferred_io_init(g_fbi.fbinfo[i]);
#endif

		g_fbi.fbinfo[i]->var.xoffset = 0;
		g_fbi.fbinfo[i]->var.yoffset = 0;
		g_fbi.fbinfo[i]->var.xres = eink_mgr->panel_info.width;
		g_fbi.fbinfo[i]->var.yres = eink_mgr->panel_info.height;
		g_fbi.fbinfo[i]->var.xres_virtual = eink_mgr->panel_info.width;
		g_fbi.fbinfo[i]->fix.line_length =
		    (g_fbi.fbinfo[i]->var.xres *
		     g_fbi.fbinfo[i]->var.bits_per_pixel) >>
		    3;

		g_fbi.fbinfo[i]->fix.smem_len =
		    g_fbi.fbinfo[i]->fix.line_length *
		    g_fbi.fbinfo[i]->var.yres * buffer_num;

		if (g_fbi.fbinfo[i]->fix.line_length != 0)
			g_fbi.fbinfo[i]->var.yres_virtual =
			    g_fbi.fbinfo[i]->fix.smem_len /
			    g_fbi.fbinfo[i]->fix.line_length;
		fb_map_video_memory(g_fbi.fbinfo[i]);

		g_fbi.fbinfo[i]->var.width = eink_mgr->panel_info.width;
		g_fbi.fbinfo[i]->var.height = eink_mgr->panel_info.width;
		g_fbi.fb_enable[i] = 1;
		g_fbi.fb_index[i] = i;
		ret = __setup_layer(i);
		if (ret) {
			pr_err("setup layer fail:%d\n", ret);
			break;
		}
		ret = register_framebuffer(g_fbi.fbinfo[i]);
		if (ret) {
			pr_err("register_framebuffer fail:%d\n", ret);
			break;
		}
	}
	return ret;
}

void __dma_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = EINK_BYTE_ALIGN(num_bytes, 4096);
	if (phys_addr && virt_addr)
		dma_free_coherent(g_fbi.dev, actual_bytes, virt_addr,
				  (dma_addr_t)phys_addr);
}

static inline void fb_unmap_video_memory(struct fb_info *info)
{
	if (!info->screen_base) {
		pr_err("%s: screen_base is null\n", __func__);
		return;
	}
	pr_info("%s: screen_base=0x%p, smem=0x%p, len=0x%x\n", __func__,
		(void *)info->screen_base, (void *)info->fix.smem_start,
		info->fix.smem_len);
	__dma_free((void *__force)info->screen_base,
		   (void *)info->fix.smem_start, info->fix.smem_len);
	info->screen_base = 0;
	info->fix.smem_start = 0;
}

int eink_fb_exit(void)
{
	unsigned int fb_id = 0;

	for (fb_id = 0; fb_id < EINK_FB_MAX; fb_id++) {
		if (g_fbi.fbinfo[fb_id]) {
#ifdef CONFIG_FB_DEFERRED_IO
			fb_deferred_io_cleanup(g_fbi.fbinfo[fb_id]);
#endif
			fb_dealloc_cmap(&g_fbi.fbinfo[fb_id]->cmap);
			fb_unmap_video_memory(g_fbi.fbinfo[fb_id]);
			unregister_framebuffer(g_fbi.fbinfo[fb_id]);
			framebuffer_release(g_fbi.fbinfo[fb_id]);
			__dma_free(
			    (void *__force)g_fbi.cur_vir_addr[fb_id],
			    (void *)(unsigned long)g_fbi.cur_img[fb_id].paddr,
			    g_fbi.cur_img[fb_id].pitch *
				g_fbi.cur_img[fb_id].size.height);
			__dma_free(
			    (void *__force)g_fbi.last_vir_addr[fb_id],
			    (void *)(unsigned long)g_fbi.last_img[fb_id].paddr,
			    g_fbi.last_img[fb_id].pitch *
				g_fbi.last_img[fb_id].size.height);
			g_fbi.fbinfo[fb_id] = NULL;
		}
	}

	return 0;
}

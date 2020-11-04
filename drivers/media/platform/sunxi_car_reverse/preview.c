/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "../../../video/fbdev/sunxi/disp2/disp/dev_disp.h"


#ifdef CONFIG_SUPPORT_BIRDVIEW
#include "BVKernal/G2dApi.h"
#include "BVKernal/birdview_api.h"
#endif
#include "colormap.h"
#include "car_reverse.h"
#include "include.h"
#include <linux/g2d_driver.h>
#include <linux/jiffies.h>
#include <linux/sunxi_tr.h>
#include <asm/cacheflush.h>
#if defined CONFIG_ARCH_SUN8IW17P1
#include <asm/glue-cache.h>
#endif
#undef _TRANSFORM_DEBUG

#define BUFFER_CNT (5)
#define BUFFER_MASK (0x03)
#define AUXLAYER_WIDTH (720)
#define AUXLAYER_HEIGHT (480)
#define AUXLAYER_SIZE (AUXLAYER_WIDTH * AUXLAYER_HEIGHT * 4)

struct rect {
	int x, y;
	int w, h;
};

struct preview_private_data {
	struct device *dev;
	struct rect src;
	struct rect frame;
	struct rect screen;
	struct disp_layer_config config[2];
	int layer_cnt;

	int format;
	int input_src;
	int rotation;
	int mirror;
	unsigned long tr_handler;
	tr_info transform;
	int oview_rdy;
	int count_render;
	int count_display;
	int viewthread;
	struct buffer_pool *disp_buffer;
	struct buffer_node *fb[BUFFER_CNT];
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
	struct buffer_pool *auxiliary_line;
	struct buffer_node *auxlayer;
	struct buffer_node *auxlayer_disp;
#endif
};

struct workqueue_struct *aut_preview_wq;

/* for auxiliary line */
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
extern int draw_auxiliary_line(void *base, int width,
								int height, int rotate,
								int lr);
int draw_image(void *base, unsigned int x,
			unsigned int y, unsigned int w,
			unsigned int h, int stride_w,
			int stride_h, unsigned char *data,
			unsigned int len);
extern void init_auxiliary_paramter(int ld, int hgap, int vgap, int sA, int mA);
int init_auxiliary_line(int rotate, int);
int init_draw_ui(int screen_id, int w, int h);
int draw_ui(int screen_id, int w, int h);
#endif

/* function from sunxi transform driver */
#ifdef CONFIG_SUNXI_TRANSFORM
extern unsigned long sunxi_tr_request(void);
extern int sunxi_tr_release(unsigned long hdl);
extern int sunxi_tr_commit(unsigned long hdl, tr_info *info);
extern int sunxi_tr_set_timeout(unsigned long hdl, unsigned long timeout);
extern int sunxi_tr_query(unsigned long hdl);
#else
unsigned long sunxi_tr_request(void)
{
	return 0;
}
int sunxi_tr_release(unsigned long hdl)
{
	return 0;
}
int sunxi_tr_commit(unsigned long hdl, tr_info *info)
{
	return -1;
}
int sunxi_tr_set_timeout(unsigned long hdl, unsigned long timeout)
{
	return 0;
}
int sunxi_tr_query(unsigned long hdl)
{
	return 0;
}

#endif
#ifdef CONFIG_SUNXI_G2D
extern int g2d_blit(g2d_blt *para);
extern int g2d_stretchblit(g2d_stretchblt *para);
extern int g2d_open(struct inode *inode, struct file *file);
extern int g2d_release(struct inode *inode, struct file *file);
#endif
static struct mutex g2d_mutex;

#ifdef CONFIG_SUPPORT_BIRDVIEW
#define SHI_DONG 1
#else
#define SHI_DONG 0
#endif

#if SHI_DONG
static BV_Inst_t ShiTong_Inst[2];
static BIRDVIEW_CONFIG ov_configData[2];
static AW360_CONFIG aw360_config;
#endif
/* function from display driver */
extern struct disp_manager *disp_get_layer_manager(u32 disp);
extern int disp_get_num_screens(void);
extern s32 bsp_disp_shadow_protect(u32 disp, bool protect);
static struct preview_private_data preview[3];
int image_rotate(struct buffer_node *frame, struct buffer_node **rotate,
		int screen_id, int type);

#if SHI_DONG

int g2dInit(void)
{
	return 1;
}

int g2dUnit(int g2dHandle)
{
	return 0;
}

int g2dAllocBuff(stV4L2BUF *bufHandle, int width, int hight)
{
	struct buffer_node *handle = NULL;
	handle = __buffer_node_alloc(preview[0].dev, width * hight * 2, 1);
	bufHandle->addrPhyY = (unsigned long)(handle->phy_address);
	bufHandle->addrVirY = (unsigned long)(handle->vir_address);
	bufHandle->buf_node = (void *)handle;
	return 0;
}

int g2dFreeBuff(stV4L2BUF *bufHandle)
{
	if (bufHandle && bufHandle->buf_node) {
		__buffer_node_free(preview[0].dev,
		(struct buffer_node *)bufHandle->buf_node);
	}
	return 0;
}

void *g2dMalloc(void **handle, int sz)
{
	struct buffer_node *buf_node = NULL;
	buf_node = __buffer_node_alloc(preview[0].dev, sz, 1);
	*handle = buf_node;
	return (void *)(buf_node->vir_address);
}

void g2dFree(void *handle)
{
	if (handle) {
		__buffer_node_free(preview[0].dev,
					(struct buffer_node *)handle);
	}
	return;
}

int g2dScale(int g2dHandle, unsigned char *src, int src_width, int src_height,
		 unsigned char *dst, int dst_width, int dst_height)
{

#ifdef CONFIG_SUNXI_G2D
	static struct file g2d_mfile;
	g2d_stretchblt scale;
	int retval = -1;

	if (g2dHandle <= 0)
		return -1;
	g2d_open(0, &g2d_mfile);
	mutex_lock(&g2d_mutex);
	scale.flag = G2D_BLT_NONE;
	scale.src_image.addr[0] = (unsigned long)src;
	scale.src_image.addr[1] = (unsigned long)src + src_width * src_height;
	scale.src_image.w = src_width;
	scale.src_image.h = src_height;
	scale.src_image.format = G2D_FMT_PYUV420UVC;
	scale.src_image.pixel_seq = G2D_SEQ_NORMAL;
	scale.src_rect.x = 0;
	scale.src_rect.y = 0;
	scale.src_rect.w = src_width;
	scale.src_rect.h = src_height;
	scale.dst_image.addr[0] = (unsigned long)dst;
	scale.dst_image.addr[1] = (unsigned long)dst + dst_width * dst_height;
	scale.dst_image.w = dst_width;
	scale.dst_image.h = dst_height;
	scale.dst_image.format = G2D_FMT_PYUV420UVC;
	scale.dst_image.pixel_seq = G2D_SEQ_NORMAL;
	scale.dst_rect.x = 0;
	scale.dst_rect.y = 0;
	scale.dst_rect.w = dst_width;
	scale.dst_rect.h = dst_height;
	scale.color = 0xff;
	scale.alpha = 0xff;
	retval = g2d_stretchblit(&scale);
	mutex_unlock(&g2d_mutex);
	if (retval < 0) {
		printk(KERN_ERR
				"g2d_scale: failed to call G2D_CMD_STRETCHBLT!!!\n");
		g2d_release(0, &g2d_mfile);
		return -1;
	}
	g2d_release(0, &g2d_mfile);
#endif
	return 0;
}

int g2dScaleRGB(void *src, int srcx, int srcy,
		int src_width, int src_height,
		void *dst, int dstx, int dsty,
		int dst_width, int dst_height)
{
#ifdef CONFIG_SUNXI_G2D
	static struct file g2d_mfile;
	g2d_stretchblt scale;
	int retval = -1;
	g2d_open(0, &g2d_mfile);
	mutex_lock(&g2d_mutex);
	scale.flag = G2D_BLT_NONE;
	scale.src_image.addr[0] = (unsigned long)src;
	scale.src_image.w = src_width;
	scale.src_image.h = src_height;
	scale.src_image.format = G2D_FORMAT_ARGB8888;
	scale.src_image.pixel_seq = G2D_SEQ_NORMAL;
	scale.src_rect.x = srcx;
	scale.src_rect.y = srcy;
	scale.src_rect.w = src_width;
	scale.src_rect.h = src_height;
	scale.dst_image.addr[0] = (unsigned long)dst;
	scale.dst_image.w = dst_width;
	scale.dst_image.h = dst_height;
	scale.dst_image.format = G2D_FORMAT_ARGB8888;
	scale.dst_image.pixel_seq = G2D_SEQ_NORMAL;
	scale.dst_rect.x = dstx;
	scale.dst_rect.y = dsty;
	scale.dst_rect.w = dst_width;
	scale.dst_rect.h = dst_height;
	scale.color = 0xff;
	scale.alpha = 0xff;
	retval = g2d_stretchblit(&scale);
	mutex_unlock(&g2d_mutex);
	if (retval < 0) {
		printk(KERN_ERR
				"g2d_scale: failed to call G2D_CMD_STRETCHBLT!!!\n");
		g2d_release(0, &g2d_mfile);
		return -1;
	}
	g2d_release(0, &g2d_mfile);
#endif
	return 0;
}

int g2dClip(int g2dHandle, void *psrc,
		int src_w, int src_h, int src_x,
		int src_y, int width, int height,
		void *pdst, int dst_w, int dst_h,
		int dst_x, int dst_y)
{
#ifdef CONFIG_SUNXI_G2D
	g2d_blt blit_para;
	int err = -1;
	static struct file g2d_mfile;
	if (g2dHandle <= 0)
		return -1;
	g2d_open(0, &g2d_mfile);
	mutex_lock(&g2d_mutex);
	blit_para.src_image.addr[0] = (unsigned long)psrc;
	blit_para.src_image.addr[1] = (unsigned long)psrc + src_w * src_h;
	blit_para.src_image.w = src_w;
	blit_para.src_image.h = src_h;
	blit_para.src_image.format = G2D_FMT_PYUV420UVC;
	blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.src_rect.x = src_x;
	blit_para.src_rect.y = src_y;
	blit_para.src_rect.w = width;
	blit_para.src_rect.h = height;
	blit_para.dst_image.addr[0] = (unsigned long)pdst;
	blit_para.dst_image.addr[1] = (unsigned long)pdst + dst_w * dst_h;
	blit_para.dst_image.w = dst_w;
	blit_para.dst_image.h = dst_h;
	blit_para.dst_image.format = G2D_FMT_PYUV420UVC;
	blit_para.dst_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.dst_x = dst_x;
	blit_para.dst_y = dst_y;
	blit_para.color = 0xff;
	blit_para.alpha = 0xff;
	blit_para.flag = G2D_BLT_NONE;
	err = g2d_blit(&blit_para);
	mutex_unlock(&g2d_mutex);
	if (err < 0) {
		g2d_release(0, &g2d_mfile);
		printk(KERN_ERR "----------------------------------------------"
				"---g2d_clip: failed to call "
				"G2D_CMD_BITBLT!!!\n");
		return -1;
	}
	g2d_release(0, &g2d_mfile);
#endif
	return 0;
}

int g2dRotate(int g2dHandle, g2dRotateAngle angle,
			unsigned char *src,
			int src_width, int src_height,
			unsigned char *dst, int dst_width,
			int dst_height)
{
#ifdef CONFIG_SUNXI_G2D
	g2d_stretchblt str;
	int retval = -1;
	static struct file g2d_mfile;
	if (g2dHandle <= 0)
		return -1;
	g2d_open(0, &g2d_mfile);
	mutex_lock(&g2d_mutex);
	str.src_image.addr[0] = (unsigned long)src;
	str.src_image.addr[1] = (unsigned long)src + src_width * src_height;
	str.src_image.w = src_width;
	str.src_image.h = src_height;
	str.src_image.format = G2D_FMT_PYUV420UVC;
	str.src_image.pixel_seq = G2D_SEQ_NORMAL;
	str.src_rect.x = 0;
	str.src_rect.y = 0;
	str.src_rect.w = src_width;
	str.src_rect.h = src_height;
	str.dst_image.addr[0] = (unsigned long)dst;
	str.dst_image.addr[1] = (unsigned long)dst + dst_width * dst_height;
	str.dst_image.h = dst_height;
	str.dst_image.w = dst_width;
	str.dst_image.format = G2D_FMT_PYUV420UVC;
	str.dst_image.pixel_seq = G2D_SEQ_NORMAL;
	str.dst_rect.x = 0;
	str.dst_rect.y = 0;
	str.dst_rect.w = dst_width;
	str.dst_rect.h = dst_height;
	str.color = 0xff;
	str.alpha = 0xff;
	str.flag = G2D_BLT_NONE;
	switch (angle) {
	case G2D_ROTATE90:
		str.flag = G2D_BLT_ROTATE90;
		break;
	case G2D_ROTATE180:
		str.flag = G2D_BLT_ROTATE180;
		break;
	case G2D_ROTATE270:
		str.flag = G2D_BLT_ROTATE270;
		break;
	case G2D_FLIP_HORIZONTAL:
		str.flag = G2D_BLT_FLIP_HORIZONTAL;
		break;
	case G2D_FLIP_VERTICAL:
		str.flag = G2D_BLT_FLIP_VERTICAL;
		break;
	case G2D_MIRROR45:
		str.flag = G2D_BLT_MIRROR45;
		break;
	case G2D_MIRROR135:
		str.flag = G2D_BLT_MIRROR135;
		break;
	}
	retval = g2d_stretchblit(&str);
	mutex_unlock(&g2d_mutex);
	if (retval < 0) {
		g2d_release(0, &g2d_mfile);
		printk(KERN_ERR "--------------------------------------------"
				"g2dRotate: failed to call "
				"G2D_CMD_STRETCHBLT!!!\n");
		return -1;
	}
	g2d_release(0, &g2d_mfile);
#endif
	return 0;
}

static G2dOpsS _G2dOpsS;

G2dOpsS *GetG2dOpsS(void)
{
	printk(KERN_ERR "------------------------------_GetG2dOpsS-------------"
			"-----------------------------");
	_G2dOpsS.fpG2dInit = g2dInit;
	_G2dOpsS.fpG2dUnit = g2dUnit;
	_G2dOpsS.fpG2dAllocBuff = g2dAllocBuff;
	_G2dOpsS.fpG2dFreeBuff = g2dFreeBuff;
	_G2dOpsS.fpG2dMalloc = g2dMalloc;
	_G2dOpsS.fpG2dFree = g2dFree;
	_G2dOpsS.fpG2dScale = g2dScale;
	_G2dOpsS.fpG2dClip = g2dClip;
	_G2dOpsS.fpG2dRotate = g2dRotate;
	return &_G2dOpsS;
}

#endif
#define CONFIG_PHY_ADR 0x48200000

void preview_oview_init(int screen_id, int flags)
{
#if SHI_DONG
	void *ov_config_handler = 0;
	if (screen_id == 0 && flags) {
		int ret = -1;
		ShiTong_Inst[screen_id] = NULL;
		ov_config_handler = (void __iomem *)phys_to_virt(CONFIG_PHY_ADR);
		memset(&aw360_config, 0, sizeof(aw360_config));
		if (ov_config_handler) {
			memcpy(&aw360_config, ov_config_handler, sizeof(aw360_config));
		}

		shareDataStruct(&ov_configData[screen_id]);
		if (aw360_config.size == sizeof(AW360_CONFIG)) {
			memcpy(&ov_configData[screen_id],
			&aw360_config.birdviewConfig, sizeof(BIRDVIEW_CONFIG));
			printk(KERN_ERR "preview_oview_init got aw360_config\n");
		} else {
			bvLoadDefault(Product_AHD);
			printk(KERN_ERR "preview_oview_init aw360_config default\n");
		}
		ret = CreateBVInst(&ShiTong_Inst[screen_id], Product_AHD,
					Data_Four, GetG2dOpsS());
		if (ret < 0) {
			logerror("CreateBVInst failed (ret = %d)\n", ret);
		} else {
			ConfigBVInstEx(ShiTong_Inst[screen_id],
						&ov_configData[screen_id].config,
						&ov_configData[screen_id].configExt);
			ChangeBVDisplay(ShiTong_Inst[screen_id],
					BV_Display_Birdview_Rear);
			logerror("CreateBVInst config success!\n");
		}
		preview[screen_id].oview_rdy = 1;
	}
#endif
	return;
}

void preview_oview_stop(int screen_id, int flags)
{
#if SHI_DONG
	if (screen_id == 0 && flags) {
		DestroyBVInst(ShiTong_Inst[screen_id]);
		preview[screen_id].oview_rdy = 0;
	}
#endif
	return;
}
unsigned int aut_display_start;
void display_frame_work(void)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL; /*disp_get_layer_manager(0);*/
	struct disp_layer_config *config = NULL;
	struct buffer_node *disp_frame = NULL;
	int buffer_format;
	int width;
	int height;
	int count = 0;

	if (aut_display_start) {
		num_screens = disp_get_num_screens();
		for (screen_id = 0; screen_id < num_screens; screen_id++) {
			mgr = disp_get_layer_manager(screen_id);
			if (!mgr || !mgr->force_set_layer_config) {
				logerror("preview update error\n");
				return;
			}
			if (preview[screen_id].count_display ==
				preview[screen_id].count_render) {
				return;
			}
			count = preview[screen_id].count_display;
			if (count == BUFFER_CNT || count == 0)
				count = 4;
			else
				count++;

			disp_frame = preview[screen_id].fb[count];
			if (disp_frame == NULL) {
				return;
			}
			config = &preview[screen_id].config[0];
			buffer_format = preview[screen_id].format;
			width = preview[screen_id].src.w;
			height = preview[screen_id].src.h;

			switch (buffer_format) {
			case V4L2_PIX_FMT_NV21:
				config->info.fb.format =
					DISP_FORMAT_YUV420_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					(long)disp_frame->phy_address +
					(width * height);
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height / 2;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.x = (unsigned long long)40
							<< 32;
				config->info.fb.crop.y = (unsigned long long)0
							<< 32;
				config->info.fb.crop.width =
					(unsigned long long)(width - 80) << 32;
				config->info.fb.crop.height =
					(unsigned long long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w;
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			case V4L2_PIX_FMT_NV61:
				config->info.fb.format =
					DISP_FORMAT_YUV422_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					config->info.fb.addr[0] + width * height;
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.x = (unsigned long long)40
							 << 32;
				config->info.fb.crop.y = (unsigned long long)0
							 << 32;
				config->info.fb.crop.width =
					(unsigned long long)(width - 80) << 32;
				config->info.fb.crop.height =
					(unsigned long long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w;
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			case V4L2_PIX_FMT_YVU420:
				config->info.fb.format =
					DISP_FORMAT_YUV420_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					(long)disp_frame->phy_address +
					(width * height);
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height / 2;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.width =
					(unsigned long long)width << 32;
				config->info.fb.crop.height =
					(unsigned long long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w; /* FIXME */
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			case V4L2_PIX_FMT_YUV422P:
				config->info.fb.format =
					DISP_FORMAT_YUV422_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					(long)disp_frame->phy_address +
					(width * height);
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.width =
					(unsigned long long)width << 32;
				config->info.fb.crop.height =
					(unsigned long long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w; /* FIXME */
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			default:
				logerror("%s: unknown pixel format, skip\n",
					 __func__);
				break;
			}
			bsp_disp_shadow_protect(screen_id, true);
			mgr->force_set_layer_config(
				mgr, &preview[screen_id].config[0],
				preview[screen_id].layer_cnt);
			bsp_disp_shadow_protect(screen_id, false);
		}
		preview[screen_id].count_display = count;
	}
}

int preview_output_start(struct preview_params *params)
{
	struct rect perfect;
	int i;
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL; /*disp_get_layer_manager(0);*/
	aut_display_start = 0;
	num_screens = disp_get_num_screens();
	mutex_init(&g2d_mutex);
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config) {
			logerror("screen %d preview init error\n", screen_id);
			return -1;
		}
		i = 0;
		memset(&(preview[screen_id]), 0, sizeof(preview[screen_id]));
		preview[screen_id].dev = params->dev;
		preview[screen_id].src.w = params->src_width;
		preview[screen_id].src.h = params->src_height;
		preview[screen_id].format = params->format;
		preview[screen_id].layer_cnt = 1;
		preview[screen_id].input_src = params->input_src;
		preview[screen_id].viewthread = params->viewthread;
		if (mgr->device && mgr->device->get_resolution) {
			mgr->device->get_resolution(
				mgr->device, &preview[screen_id].screen.w,
				&preview[screen_id].screen.h);
			logdebug("screen size: %dx%d\n",
				 preview[screen_id].screen.w,
				 preview[screen_id].screen.h);
		} else {
			preview[screen_id].screen.w = params->screen_width;
			preview[screen_id].screen.h = params->screen_height;
			logerror("can't get screen size, use default: %dx%d : "
				 "rotation = %d\n",
				 preview[screen_id].screen.w,
				 preview[screen_id].screen.h, params->rotation);
		}

		if (params->rotation == 1 || params->rotation == 3) {
			perfect.w = params->screen_height;
			perfect.h = params->screen_width;
		} else {
			perfect.w = params->screen_width;
			perfect.h = params->screen_height;
		}
		preview[screen_id].frame.w =
			(perfect.w > preview[screen_id].screen.w)
			? preview[screen_id].screen.w
			: perfect.w;
		preview[screen_id].frame.h =
			(perfect.h > preview[screen_id].screen.h)
			? preview[screen_id].screen.h
			: perfect.h;

		preview[screen_id].frame.x =
			(preview[screen_id].screen.w - preview[screen_id].frame.w) /
			2;
		preview[screen_id].frame.y =
			(preview[screen_id].screen.h - preview[screen_id].frame.h) /
			2;
		preview[screen_id].mirror = params->pr_mirror;
#ifdef CONFIG_SUNXI_TRANSFORM
		if (params->rotation) {
			preview[screen_id].rotation = params->rotation;
			preview[screen_id].tr_handler = sunxi_tr_request();
			if (!preview[screen_id].tr_handler) {
				logerror("request transform channel failed\n");
				return -1;
			}

			preview[screen_id].disp_buffer = alloc_buffer_pool(
				preview[screen_id].dev, BUFFER_CNT,
				preview[screen_id].src.w *
				preview[screen_id].src.h * 2);
			if (!preview[screen_id].disp_buffer) {
				sunxi_tr_release(preview[screen_id].tr_handler);
				logerror("request display buffer failed\n");
				return -1;
			}
			for (i = 0; i < BUFFER_CNT; i++)
				preview[screen_id].fb[i] =
					preview[screen_id]
				.disp_buffer->dequeue_buffer(
				preview[screen_id].disp_buffer);
		}
#endif
#ifdef CONFIG_SUNXI_G2D
		if (params->rotation || params->pr_mirror) {
			preview[screen_id].rotation = params->rotation;
			preview[screen_id].disp_buffer = alloc_buffer_pool(
				preview[screen_id].dev, BUFFER_CNT,
				preview[screen_id].src.w *
				preview[screen_id].src.h * 2);
			if (!preview[screen_id].disp_buffer) {
				logerror("request display buffer failed\n");
				return -1;
			}
			for (i = 0; i < BUFFER_CNT; i++) {
				preview[screen_id].fb[i] =
					preview[screen_id]
					.disp_buffer->dequeue_buffer(
						preview[screen_id].disp_buffer);
				memset(preview[screen_id]
							 .fb[i]
							 ->vir_address,
							 0x10,
							 preview[screen_id].src.w *
							 preview[screen_id].src.h);
				memset(preview[screen_id]
								 .fb[i]
								 ->vir_address +
							 preview[screen_id].src.w *
								 preview[screen_id].src.h,
							 0x80,
							 preview[screen_id].src.w *
							 preview[screen_id].src.h);
			}
		}
#endif
		if (params->car_oview_mode) {
			if (!preview[screen_id].disp_buffer) {
				preview[screen_id].disp_buffer =
					alloc_buffer_pool(
					preview[screen_id].dev, BUFFER_CNT,
					preview[screen_id].src.w *
						preview[screen_id].src.h * 2);

				if (!preview[screen_id].disp_buffer) {
					logerror(
						"request display buffer failed\n");
					return -1;
				}
				for (i = 0; i < BUFFER_CNT; i++) {
					preview[screen_id].fb[i] =
						preview[screen_id]
						.disp_buffer->dequeue_buffer(
							preview[screen_id]
							.disp_buffer);
					memset(preview[screen_id]
							 .fb[i]
							 ->vir_address,
							 0x10,
							 preview[screen_id].src.w *
							 preview[screen_id].src.h);
					memset(preview[screen_id]
								 .fb[i]
								 ->vir_address +
							 preview[screen_id].src.w *
								 preview[screen_id].src.h,
							 0x80,
							 preview[screen_id].src.w *
							 preview[screen_id].src.h /
							 2);
				}
			}
		}
		preview_oview_init(screen_id, params->car_oview_mode);
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
		if (params->input_src == 0) {
			init_auxiliary_paramter(800, 50, 200, 25, 80);
			init_auxiliary_line(params->rotation, screen_id);
		} else if (params->input_src && params->car_oview_mode) {
			init_draw_ui(0, MAP_W, MAP_H);
			draw_ui(0, MAP_W, MAP_H);
		}
#endif
	}
	return 0;
}

#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
int init_draw_ui(int screen_id, int w, int h)
{
#if SHI_DONG
	if (screen_id == 0) {
		preview[screen_id].auxiliary_line =
			alloc_buffer_pool(preview[screen_id].dev, 2, w * h * 4);

		if (!preview[screen_id].auxiliary_line) {
			logerror("request init_draw_ui	buffer failed\n");
			return -1;
		}

		preview[screen_id].auxlayer =
			preview[screen_id].auxiliary_line->dequeue_buffer(
			preview[screen_id].auxiliary_line);
		if (!preview[screen_id].auxlayer) {
			logerror("no buffer in buffer pool\n");
			return -1;
		}

		preview[screen_id].auxlayer_disp =
			preview[screen_id].auxiliary_line->dequeue_buffer(
			preview[screen_id].auxiliary_line);
		if (!preview[screen_id].auxlayer_disp) {
			logerror("no buffer in buffer pool\n");
			return -1;
		}
		preview[screen_id].layer_cnt++;
	}
#endif
	return 0;
}

int draw_ui(int screen_id, int w, int h)
{
#if SHI_DONG
	void *start1, *start2;
	void *end1, *end2;
	pRect_t loc;
	struct disp_layer_config *config;
	start1 = preview[screen_id].auxlayer->vir_address;
	end1 = (void *)((unsigned long)start1 + w * h * 4);
	start2 = preview[screen_id].auxlayer_disp->vir_address;
	end2 = (void *)((unsigned long)start2 + w * h * 4);
	if (screen_id == 0) {

		memset(preview[screen_id].auxlayer->vir_address, 0, w * h * 4);
		memset(preview[screen_id].auxlayer_disp->vir_address, 0,
				 w * h * 4);
		__dma_flush_range(start1, end1);
		__dma_flush_range(start2, end2);

		draw_image(preview[screen_id].auxlayer_disp->vir_address, 0, 0,
				 w, h, w, h, color_map, sizeof(color_map));
		__dma_flush_range(start1, end1);

		config = &preview[screen_id].config[1];
		memset(config, 0, sizeof(struct disp_layer_config));
		loc = getOverlayPos(0, 0, 0);
		__dma_flush_range(start2, end2);
		config->channel = 1;
		config->enable = 1;
		config->layer_id = 0;

		config->info.fb.addr[0] =
			(unsigned int)preview[screen_id].auxlayer_disp->phy_address;
		config->info.fb.format = DISP_FORMAT_ARGB_8888;

		config->info.fb.size[0].width = w;
		config->info.fb.size[0].height = h;
		config->info.fb.size[1].width = w;
		config->info.fb.size[1].height = h;
		config->info.fb.size[2].width = w;
		config->info.fb.size[2].height = h;
		config->info.mode = LAYER_MODE_BUFFER;
		config->info.zorder = 1;
		config->info.fb.crop.width = (unsigned long long)(w) << 32;
		config->info.fb.crop.height = (unsigned long long)(h) << 32;
		config->info.alpha_mode = 0; /* pixel alpha */
		config->info.alpha_value = 0;
		config->info.screen_win.x =
			(loc->top * preview[screen_id].frame.w /
			preview[screen_id].src.w) -
			40;
		config->info.screen_win.y =
			(loc->left * preview[screen_id].frame.w /
			 preview[screen_id].src.w) +
			40;
		config->info.screen_win.width = loc->width *
						preview[screen_id].frame.w /
						preview[screen_id].src.w;
		config->info.screen_win.height = loc->height *
						 preview[screen_id].frame.h /
						 preview[screen_id].src.h;
	}
#endif
	return 0;
}

int init_auxiliary_line(int rotate, int screen_id)
{
	const struct rect _rect[2] = {
		{0, 0, AUXLAYER_HEIGHT, AUXLAYER_WIDTH},
		{0, 0, AUXLAYER_WIDTH, AUXLAYER_HEIGHT},

	};
	int idx;
	int buffer_size;
	struct disp_layer_config *config;
	void *start;
	void *end;
	if (preview[screen_id].rotation == 1 ||
		preview[screen_id].rotation == 3)
		idx = 0;
	else
		idx = 1;
	buffer_size = _rect[idx].w * _rect[idx].h * 4;
	preview[screen_id].auxiliary_line =
		alloc_buffer_pool(preview[screen_id].dev, 2, buffer_size);

	if (!preview[screen_id].auxiliary_line) {
		logerror("request auxiliary line buffer failed\n");
		return -1;
	}
	preview[screen_id].auxlayer =
		preview[screen_id].auxiliary_line->dequeue_buffer(
		preview[screen_id].auxiliary_line);
	if (!preview[screen_id].auxlayer) {
		logerror("no buffer in buffer pool\n");
		return -1;
	}

	preview[screen_id].auxlayer_disp =
		preview[screen_id].auxiliary_line->dequeue_buffer(
		preview[screen_id].auxiliary_line);
	if (!preview[screen_id].auxlayer_disp) {
		logerror("no buffer in buffer pool\n");
		return -1;
	}

	memset(preview[screen_id].auxlayer->vir_address, 0, AUXLAYER_SIZE);
	draw_auxiliary_line(preview[screen_id].auxlayer->vir_address,
				AUXLAYER_WIDTH, AUXLAYER_HEIGHT, 0, 0);
	start = preview[screen_id].auxlayer->vir_address;
	end =
		(void *)((unsigned long)start + preview[screen_id].auxlayer->size);
	__dma_flush_range(start, end);

	start = preview[screen_id].auxlayer_disp->vir_address;
	end = (void *)((unsigned long)start +
				 preview[screen_id].auxlayer_disp->size);
	__dma_flush_range(start, end);

	image_rotate(NULL, NULL, screen_id, 1);

	config = &preview[screen_id].config[1];
	memset(config, 0, sizeof(struct disp_layer_config));

	config->channel = 1;
	config->enable = 1;
	config->layer_id = 0;

	config->info.fb.addr[0] =
		(unsigned int)preview[screen_id].auxlayer_disp->phy_address;
	config->info.fb.format = DISP_FORMAT_ARGB_8888;

	config->info.fb.size[0].width = _rect[idx].w;
	config->info.fb.size[0].height = _rect[idx].h;
	config->info.fb.size[1].width = _rect[idx].w;
	config->info.fb.size[1].height = _rect[idx].h;
	config->info.fb.size[2].width = _rect[idx].w;
	config->info.fb.size[2].height = _rect[idx].h;
	config->info.mode = LAYER_MODE_BUFFER;
	config->info.zorder = 1;
	config->info.fb.crop.width = (unsigned long long)_rect[idx].w << 32;
	config->info.fb.crop.height = (unsigned long long)_rect[idx].h << 32;

	config->info.alpha_mode = 0; /* pixel alpha */
	config->info.alpha_value = 0;
	config->info.screen_win.x = preview[screen_id].frame.x;
	config->info.screen_win.y = preview[screen_id].frame.y;
	config->info.screen_win.width = preview[screen_id].frame.w;
	config->info.screen_win.height = preview[screen_id].frame.h;
	preview[screen_id].layer_cnt++;
	return 0;
}

void deinit_auxiliary_line(int screen_id)
{
	if (preview[screen_id].auxlayer) {
		preview[screen_id].auxiliary_line->queue_buffer(
			preview[screen_id].auxiliary_line,
			preview[screen_id].auxlayer);
		preview[screen_id].auxlayer = 0;
	}
	if (preview[screen_id].auxlayer_disp) {
		preview[screen_id].auxiliary_line->queue_buffer(
			preview[screen_id].auxiliary_line,
			preview[screen_id].auxlayer_disp);
		preview[screen_id].auxlayer_disp = 0;
	}
	if (preview[screen_id].auxiliary_line)
		free_buffer_pool(preview[screen_id].dev,
				 preview[screen_id].auxiliary_line);
}
#endif

int preview_output_stop(struct preview_params *params)
{
	int i;
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL;
	aut_display_start = 0;
	num_screens = disp_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config) {
			logerror("screen %d preview stop error\n", screen_id);
			return -1;
		}
		mgr->force_set_layer_config_exit(mgr);
		msleep(100);

		if (preview[screen_id].tr_handler)
			sunxi_tr_release(preview[screen_id].tr_handler);

		if (preview[screen_id].disp_buffer) {
			for (i = 0; i < BUFFER_CNT; i++) {
				if (!preview[screen_id].fb[i])
					continue;

				preview[screen_id].disp_buffer->queue_buffer(
					preview[screen_id].disp_buffer,
					preview[screen_id].fb[i]);
			}

			free_buffer_pool(preview[screen_id].dev,
					 preview[screen_id].disp_buffer);
		}

#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
		if (params->input_src == 0 ||
			(params->input_src && params->car_oview_mode)) {
			deinit_auxiliary_line(screen_id);
		}
#endif
#if SHI_DONG
		preview_oview_stop(screen_id, params->car_oview_mode);
#endif
	}

	return 0;
}

int image_rotate(struct buffer_node *frame, struct buffer_node **rotate,
		 int screen_id, int type)
{
#if defined(CONFIG_SUNXI_TRANSFORM) || defined(CONFIG_SUNXI_G2D)
	static int active;
	struct buffer_node *node;
#endif
	int retval = -1;
	if (type == 0) {
#ifdef CONFIG_SUNXI_TRANSFORM
		tr_info *info = &preview[screen_id].transform;
#ifdef _TRANSFORM_DEBUG
		unsigned long start_jiffies;
		unsigned long end_jiffies;
		unsigned long time;
#endif

		active++;
		node = preview[screen_id].fb[active & BUFFER_MASK];
		if (!node || !frame) {
			logerror("%s, alloc buffer failed\n", __func__);
			return -1;
		}

		info->mode = TR_ROT_90;
		info->src_frame.fmt = TR_FORMAT_YUV420_SP_UVUV;
		info->src_frame.laddr[0] = (unsigned int)frame->phy_address;
		info->src_frame.laddr[1] =
			(unsigned int)frame->phy_address +
			preview[screen_id].src.w * preview[screen_id].src.h;
		info->src_frame.laddr[2] = 0;
		info->src_frame.pitch[0] = preview[screen_id].src.w;
		info->src_frame.pitch[1] = preview[screen_id].src.w / 2;
		info->src_frame.pitch[2] = 0;
		info->src_frame.height[0] = preview[screen_id].src.h;
		info->src_frame.height[1] = preview[screen_id].src.h / 2;
		info->src_frame.height[2] = 0;
		info->src_rect.x = 0;
		info->src_rect.y = 0;
		info->src_rect.w = preview[screen_id].src.w;
		info->src_rect.h = preview[screen_id].src.h;

		info->dst_frame.fmt = TR_FORMAT_YUV420_P;
		info->dst_frame.laddr[0] = (unsigned long)node->phy_address;
		info->dst_frame.laddr[1] =
			(unsigned long)node->phy_address +
			preview[screen_id].src.w * preview[screen_id].src.h;
		info->dst_frame.laddr[2] =
			(unsigned long)node->phy_address +
			preview[screen_id].src.w * preview[screen_id].src.h +
			preview[screen_id].src.w * preview[screen_id].src.h / 4;
		info->dst_frame.pitch[0] = preview[screen_id].src.h;
		info->dst_frame.pitch[1] = preview[screen_id].src.h / 2;
		info->dst_frame.pitch[2] = preview[screen_id].src.h / 2;
		info->dst_frame.height[0] = preview[screen_id].src.w;
		info->dst_frame.height[1] = preview[screen_id].src.w / 2;
		info->dst_frame.height[2] = preview[screen_id].src.w / 2;
		info->dst_rect.x = 0;
		info->dst_rect.y = 0;
		info->dst_rect.w = preview[screen_id].src.h;
		info->dst_rect.h = preview[screen_id].src.w;

#ifdef _TRANSFORM_DEBUG
		start_jiffies = jiffies;
#endif
		if (sunxi_tr_commit(preview[screen_id].tr_handler, info) < 0) {
			logerror("transform commit error!\n");
			return -1;
		}

		while (1) {
			retval = sunxi_tr_query(preview[screen_id].tr_handler);
			if (retval == 0 || retval == -1)
				break;
			msleep(1);
		}
		*rotate = node;

#ifdef _TRANSFORM_DEBUG
		end_jiffies = jiffies;
		time = jiffies_to_msecs(end_jiffies - start_jiffies);
		logerror("TR:%ld ms\n", time);
#endif
#endif
#ifdef CONFIG_SUNXI_G2D
		g2d_stretchblt blit_para;
		struct file g2d_file;
		g2d_open(0, &g2d_file);
		active++;
		node = preview[screen_id].fb[active & BUFFER_MASK];
		if (!node || !frame) {
			logerror("%s, alloc buffer failed\n", __func__);
			return -1;
		}

		mutex_lock(&g2d_mutex);

		blit_para.src_image.addr[0] =
			(unsigned long)frame->phy_address;
		blit_para.src_image.addr[1] =
			(unsigned long)frame->phy_address +
			preview[screen_id].src.w * preview[screen_id].src.h;
		blit_para.src_image.w = preview[screen_id].src.w;
		blit_para.src_image.h = preview[screen_id].src.h;
		if (preview[screen_id].format == V4L2_PIX_FMT_NV21)
			blit_para.src_image.format = G2D_FMT_PYUV420UVC;
		else
			blit_para.src_image.format = G2D_FMT_PYUV422UVC;
		blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;
		blit_para.src_rect.x = 0;
		blit_para.src_rect.y = 0;
		blit_para.src_rect.w = preview[screen_id].src.w;
		blit_para.src_rect.h = preview[screen_id].src.h;

		blit_para.dst_image.addr[0] =
			(unsigned long)node->phy_address;
		blit_para.dst_image.addr[1] =
			(unsigned long)node->phy_address +
			preview[screen_id].src.w * preview[screen_id].src.h;
		if (preview[screen_id].format == V4L2_PIX_FMT_NV21)
			blit_para.dst_image.format = G2D_FMT_PYUV420UVC;
		else
			blit_para.dst_image.format = G2D_FMT_PYUV422UVC;
		blit_para.dst_image.pixel_seq = G2D_SEQ_NORMAL;
		blit_para.dst_rect.x = 0;
		blit_para.dst_rect.y = 0;
		if (preview[screen_id].rotation == 1 ||
			preview[screen_id].rotation == 3) {
			blit_para.dst_image.w = preview[screen_id].src.h;
			blit_para.dst_image.h = preview[screen_id].src.w;
			blit_para.dst_rect.w = preview[screen_id].src.h;
			blit_para.dst_rect.h = preview[screen_id].src.w;
		} else {
			blit_para.dst_image.w = preview[screen_id].src.w;
			blit_para.dst_image.h = preview[screen_id].src.h;
			blit_para.dst_rect.w = preview[screen_id].src.w;
			blit_para.dst_rect.h = preview[screen_id].src.h;
		}
		blit_para.color = 0xff;
		blit_para.alpha = 0xff;
		if (preview[screen_id].rotation == 1)
			blit_para.flag = G2D_BLT_ROTATE90;
		if (preview[screen_id].rotation == 2)
			blit_para.flag = G2D_BLT_ROTATE180;
		if (preview[screen_id].rotation == 3)
			blit_para.flag = G2D_BLT_ROTATE270;
		if (preview[screen_id].mirror) {
			blit_para.flag |= G2D_BLT_FLIP_HORIZONTAL;
		}
		retval = g2d_stretchblit(&blit_para);
		mutex_unlock(&g2d_mutex);
		if (retval < 0) {
			printk(KERN_ERR
					 "%s: g2d_blit G2D_BLT_ROTATE180 failed\n",
					 __func__);
			g2d_release(0, &g2d_file);
			return retval;
		}
		g2d_release(0, &g2d_file);
		*rotate = node;
#endif
	}
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
	if (type == 1) {
#ifdef CONFIG_SUNXI_G2D
		g2d_stretchblt blit_para;
		struct file g2d_file;
		g2d_open(0, &g2d_file);
		mutex_lock(&g2d_mutex);
		blit_para.src_image.addr[0] =
			(unsigned long)(unsigned int)preview[screen_id]
			.auxlayer->phy_address;
		blit_para.src_image.w = AUXLAYER_WIDTH;
		blit_para.src_image.h = AUXLAYER_HEIGHT;
		blit_para.src_image.format = G2D_FORMAT_ARGB8888;
		blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;
		blit_para.src_rect.x = 0;
		blit_para.src_rect.y = 0;
		blit_para.src_rect.w = AUXLAYER_WIDTH;
		blit_para.src_rect.h = AUXLAYER_HEIGHT;

		blit_para.dst_image.addr[0] =
			(unsigned long)(unsigned int)preview[screen_id]
			.auxlayer_disp->phy_address;
		blit_para.dst_image.format = G2D_FORMAT_ARGB8888;
		blit_para.dst_image.pixel_seq = G2D_SEQ_NORMAL;
		blit_para.dst_rect.x = 0;
		blit_para.dst_rect.y = 0;
		if (preview[screen_id].rotation == 1 ||
			preview[screen_id].rotation == 3) {
			blit_para.dst_image.w = AUXLAYER_HEIGHT;
			blit_para.dst_image.h = AUXLAYER_WIDTH;
			blit_para.dst_rect.w = AUXLAYER_HEIGHT;
			blit_para.dst_rect.h = AUXLAYER_WIDTH;
		} else {
			blit_para.dst_image.w = AUXLAYER_WIDTH;
			blit_para.dst_image.h = AUXLAYER_HEIGHT;
			blit_para.dst_rect.w = AUXLAYER_WIDTH;
			blit_para.dst_rect.h = AUXLAYER_HEIGHT;
		}
		blit_para.color = 0xff;
		blit_para.alpha = 0xff;

		blit_para.flag = G2D_BLT_NONE;
		/*
		retval = g2d_stretchblit(&blit_para);
		if (retval < 0) {
			printk(KERN_ERR "%s: g2d_blit G2D_BLT_NONE failed\n",
		__func__);
			g2d_release(0, &g2d_file);
			return retval;
		}
		*/

		if (preview[screen_id].rotation == 1)
			blit_para.flag = G2D_BLT_ROTATE90;
		if (preview[screen_id].rotation == 2)
			blit_para.flag = G2D_BLT_ROTATE180;
		if (preview[screen_id].rotation == 3)
			blit_para.flag = G2D_BLT_ROTATE270;
		if (preview[screen_id].mirror)
			blit_para.flag |= G2D_BLT_FLIP_HORIZONTAL;
		retval = g2d_stretchblit(&blit_para);
		mutex_unlock(&g2d_mutex);

		if (retval < 0) {
			printk(KERN_ERR
					 "%s: g2d_blit G2D_BLT_ROTATE180 failed\n",
					 __func__);
			g2d_release(0, &g2d_file);
			return retval;
		}
		g2d_release(0, &g2d_file);
#endif
	}
#endif
	return retval;
}

int copy_frame(struct buffer_node *frame, struct buffer_node *node, int format,
			 int screen_id)
{
	int retval = -1;
#ifdef CONFIG_SUNXI_G2D
	g2d_stretchblt blit_para;
	struct file g2d_file;
	mutex_lock(&g2d_mutex);
	g2d_open(0, &g2d_file);
	blit_para.src_image.addr[0] =
		(unsigned long)frame->phy_address;
	blit_para.src_image.addr[1] =
		(unsigned long)frame->phy_address +
		preview[screen_id].src.w * preview[screen_id].src.h;
	blit_para.src_image.w = preview[screen_id].src.w;
	blit_para.src_image.h = preview[screen_id].src.h;
	blit_para.src_image.format = format;
	if (format == V4L2_PIX_FMT_NV21)
		blit_para.src_image.format = G2D_FMT_PYUV420UVC;
	else
		blit_para.src_image.format = G2D_FMT_PYUV422UVC;
	blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.src_rect.x = 0;
	blit_para.src_rect.y = 0;
	blit_para.src_rect.w = preview[screen_id].src.w;
	blit_para.src_rect.h = preview[screen_id].src.h;

	blit_para.dst_image.addr[0] =
		(unsigned long)node->phy_address;
	blit_para.dst_image.addr[1] =
		(unsigned long)node->phy_address +
		preview[screen_id].src.w * preview[screen_id].src.h;
	if (format == V4L2_PIX_FMT_NV21)
		blit_para.dst_image.format = G2D_FMT_PYUV420UVC;
	else
		blit_para.dst_image.format = G2D_FMT_PYUV422UVC;
	blit_para.dst_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.dst_rect.x = 0;
	blit_para.dst_rect.y = 0;
	blit_para.color = 0xff;
	blit_para.alpha = 0xff;
	blit_para.dst_image.w = preview[screen_id].src.w;
	blit_para.dst_image.h = preview[screen_id].src.h;
	blit_para.dst_rect.w = preview[screen_id].src.w;
	blit_para.dst_rect.h = preview[screen_id].src.h;
	blit_para.flag = G2D_BLT_NONE;
	retval = g2d_stretchblit(&blit_para);
	mutex_unlock(&g2d_mutex);
	if (retval < 0) {
		printk(KERN_ERR "%s: g2d_blit G2D_BLT_NONE failed\n", __func__);
		g2d_release(0, &g2d_file);
		return retval;
	}
	g2d_release(0, &g2d_file);
#endif
	return retval;
}

int merge_frame(struct buffer_node *frame, struct buffer_node *node, int format,
		int screen_id, int sequence)
{
	int retval = -1;
#ifdef CONFIG_SUNXI_G2D
	g2d_stretchblt blit_para;
	struct file g2d_file;
	g2d_rect _rect;
	if (sequence == 0) {
		_rect.x = 0;
		_rect.y = 0;
		_rect.w = preview[screen_id].src.w / 2;
		_rect.h = preview[screen_id].src.h / 2;
	} else if (sequence == 1) {
		_rect.x = preview[screen_id].src.w / 2;
		_rect.y = 0;
		_rect.w = preview[screen_id].src.w / 2;
		_rect.h = preview[screen_id].src.h / 2;
	} else if (sequence == 2) {
		_rect.x = 0;
		_rect.y = preview[screen_id].src.h / 2;
		_rect.w = preview[screen_id].src.w / 2;
		_rect.h = preview[screen_id].src.h / 2;
	} else {
		_rect.x = preview[screen_id].src.w / 2;
		;
		_rect.y = preview[screen_id].src.h / 2;
		_rect.w = preview[screen_id].src.w / 2;
		_rect.h = preview[screen_id].src.h / 2;
	}
	g2d_open(0, &g2d_file);
	mutex_lock(&g2d_mutex);
	blit_para.src_image.addr[0] =
		(unsigned long)frame->phy_address;
	blit_para.src_image.addr[1] =
		(unsigned long)frame->phy_address +
		preview[screen_id].src.w * preview[screen_id].src.h;
	blit_para.src_image.w = preview[screen_id].src.w;
	blit_para.src_image.h = preview[screen_id].src.h;
	if (format == V4L2_PIX_FMT_NV21)
		blit_para.src_image.format = G2D_FMT_PYUV420UVC;
	else
		blit_para.src_image.format = G2D_FMT_PYUV422UVC;
	blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.src_rect.x = 0;
	blit_para.src_rect.y = 0;
	blit_para.src_rect.w = preview[screen_id].src.w;
	blit_para.src_rect.h = preview[screen_id].src.h;

	blit_para.dst_image.addr[0] =
		(unsigned long)node->phy_address;
	blit_para.dst_image.addr[1] =
		(unsigned long)node->phy_address +
		preview[screen_id].src.w * preview[screen_id].src.h;
	if (format == V4L2_PIX_FMT_NV21)
		blit_para.dst_image.format = G2D_FMT_PYUV420UVC;
	else
		blit_para.dst_image.format = G2D_FMT_PYUV422UVC;
	blit_para.dst_image.pixel_seq = G2D_SEQ_NORMAL;
#if 1
	blit_para.dst_image.w = preview[screen_id].src.w;
	blit_para.dst_image.h = preview[screen_id].src.h;
	blit_para.dst_rect.x = _rect.x;
	blit_para.dst_rect.y = _rect.y;
	blit_para.dst_rect.w = _rect.w;
	blit_para.dst_rect.h = _rect.h;
#else
	blit_para.dst_image.w = preview[screen_id].src.w;
	blit_para.dst_image.h = preview[screen_id].src.h;
	blit_para.dst_rect.x = 0;
	blit_para.dst_rect.y = 0;
	blit_para.dst_rect.w = preview[screen_id].src.w;
	blit_para.dst_rect.h = preview[screen_id].src.h;

#endif
	blit_para.color = 0xff;
	blit_para.alpha = 0xff;
	blit_para.flag = G2D_BLT_NONE;
	retval = g2d_stretchblit(&blit_para);
	mutex_unlock(&g2d_mutex);
	if (retval < 0) {
		printk(KERN_ERR "%s: g2d_blit G2D_BLT_NONE failed\n", __func__);
		g2d_release(0, &g2d_file);
		return retval;
	}
	g2d_release(0, &g2d_file);
#endif
	return retval;
}

void preview_update(struct buffer_node *frame, int orientation, int lr_direct)
{
	int num_screens, screen_id;
	struct disp_manager *mgr = NULL; /*disp_get_layer_manager(0);*/
	struct disp_layer_config *config = NULL;
	struct buffer_node *rotate = NULL;
	int buffer_format;
	int width;
	int height;
	if (frame == NULL) {
		logerror("preview frame is null\n");
		return;
	}
	num_screens = disp_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config) {
			logerror("preview update error\n");
			return;
		}
		config = &preview[screen_id].config[0];
		buffer_format = preview[screen_id].format;
		width = preview[screen_id].src.w;
		height = preview[screen_id].src.h;

		if ((preview[screen_id].rotation ||
			 preview[screen_id].mirror) &&
			image_rotate(frame, &rotate, screen_id, 0) == 0) {
			if (!rotate) {
				logerror("image rotate error\n");
				return;
			}
			if (preview[screen_id].format == V4L2_PIX_FMT_NV21)
				buffer_format = V4L2_PIX_FMT_YVU420;
			else
				buffer_format = V4L2_PIX_FMT_YUV422P;
			if (preview[screen_id].rotation == 1 ||
				preview[screen_id].rotation == 3) {
				width = preview[screen_id].src.h;
				height = preview[screen_id].src.w;
				buffer_format = V4L2_PIX_FMT_YVU420;
			}
		}

		switch (buffer_format) {
		case V4L2_PIX_FMT_NV21:
			config->info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
			config->info.fb.addr[0] =
				(long)frame->phy_address;
			config->info.fb.addr[1] =
				(long)frame->phy_address + (width * height);
			config->info.fb.addr[2] = 0;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height / 2;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.x = (unsigned long long)40 << 32;
			config->info.fb.crop.y = (unsigned long long)0 << 32;
			config->info.fb.crop.width =
				(unsigned long long)(width - 80) << 32;
			config->info.fb.crop.height = (unsigned long long)height
								<< 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width =
				preview[screen_id].frame.w;
			config->info.screen_win.height =
				preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		case V4L2_PIX_FMT_NV61:
			config->info.fb.format = DISP_FORMAT_YUV422_SP_VUVU;
			config->info.fb.addr[0] =
				(long)frame->phy_address;
			config->info.fb.addr[1] =
				config->info.fb.addr[0] + width * height;
			config->info.fb.addr[2] = 0;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.x = (unsigned long long)40 << 32;
			config->info.fb.crop.y = (unsigned long long)0 << 32;
			config->info.fb.crop.width =
				(long)(width - 80) << 32;
			config->info.fb.crop.height = (unsigned long long)height
								<< 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width =
				preview[screen_id].frame.w;
			config->info.screen_win.height =
				preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		case V4L2_PIX_FMT_YVU420:
			config->info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
			config->info.fb.addr[0] =
				(long)rotate->phy_address;
			config->info.fb.addr[1] =
				(long)rotate->phy_address +
				(width * height);
			config->info.fb.addr[2] = 0;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height / 2;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.width = (unsigned long long)width
							 << 32;
			config->info.fb.crop.height = (unsigned long long)height
								<< 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width =
				preview[screen_id].frame.w; /* FIXME */
			config->info.screen_win.height =
				preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		case V4L2_PIX_FMT_YUV422P:
			config->info.fb.format = DISP_FORMAT_YUV422_SP_VUVU;
			config->info.fb.addr[0] =
				(long)rotate->phy_address;
			config->info.fb.addr[1] =
				(long)rotate->phy_address +
				(width * height);
			config->info.fb.addr[2] = 0;
			config->info.fb.size[0].width = width;
			config->info.fb.size[0].height = height;
			config->info.fb.size[1].width = width / 2;
			config->info.fb.size[1].height = height;
			config->info.fb.size[2].width = 0;
			config->info.fb.size[2].height = 0;
			config->info.fb.align[1] = 0;
			config->info.fb.align[2] = 0;

			config->info.fb.crop.width = (unsigned long long)width
							 << 32;
			config->info.fb.crop.height = (unsigned long long)height
								<< 32;
			config->info.screen_win.x = preview[screen_id].frame.x;
			config->info.screen_win.y = preview[screen_id].frame.y;
			config->info.screen_win.width =
				preview[screen_id].frame.w; /* FIXME */
			config->info.screen_win.height =
				preview[screen_id].frame.h;

			config->channel = 0;
			config->layer_id = 0;
			config->enable = 1;

			config->info.mode = LAYER_MODE_BUFFER;
			config->info.zorder = 0;
			config->info.alpha_mode = 1;
			config->info.alpha_value = 0xff;
			break;
		default:
			logerror("%s: unknown pixel format, skip\n", __func__);
			break;
		}
		bsp_disp_shadow_protect(screen_id, true);
		mgr->force_set_layer_config(mgr, &preview[screen_id].config[0],
						preview[screen_id].layer_cnt);
		bsp_disp_shadow_protect(screen_id, false);
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
		if (preview[screen_id].input_src == 0) {
			draw_auxiliary_line(
				preview[screen_id].auxlayer->vir_address,
				AUXLAYER_WIDTH, AUXLAYER_HEIGHT, orientation,
				lr_direct);
			image_rotate(NULL, NULL, screen_id, 1);
		}
#endif
	}
}

void preview_update_Ov(struct buffer_node **frame, int orientation,
				 int lr_direct)
{
	int num_screens = 0, screen_id = 0, i = 0;
	struct disp_manager *mgr = NULL; /*disp_get_layer_manager(0);*/
	struct disp_layer_config *config = NULL;
	struct buffer_node *rotate = NULL;
	struct buffer_node *disp_frame = NULL;
	int buffer_format;
	int width;
	int height;
	int count = 0;
#if SHI_DONG
	stV4L2BUF srcf[4];
	stV4L2BUF dstf;
	memset(srcf, 0, sizeof(stV4L2BUF) * 4);
	memset(&dstf, 0, sizeof(stV4L2BUF));

	if (frame == NULL || preview[screen_id].oview_rdy == 0) {
		logerror("preview frame is null\n");
		return;
	}
#else
	if (frame == NULL) {
		logerror("preview frame is null\n");
		return;
	}

#endif
	num_screens = disp_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = disp_get_layer_manager(screen_id);
		if (!mgr || !mgr->force_set_layer_config) {
			logerror("preview update error\n");
			return;
		}
#if SHI_DONG
		if (preview[screen_id].oview_rdy == 0) {
			continue;
		}
#endif
		if ((preview[screen_id].rotation ||
			 preview[screen_id].mirror)) {
			for (i = 0; i < CAR_MAX_CH; i++) {
				if (image_rotate(frame[i], &rotate, screen_id,
						 0) == 0) {
					if (!rotate) {
						logerror(
							"image rotate error\n");
						return;
					}
					copy_frame(rotate, frame[i],
							 preview[screen_id].format,
							 screen_id);
				}
			}
			if (preview[screen_id].format == V4L2_PIX_FMT_NV21)
				buffer_format = V4L2_PIX_FMT_YVU420;
			else
				buffer_format = V4L2_PIX_FMT_YUV422P;
			if (preview[screen_id].rotation == 1 ||
				preview[screen_id].rotation == 3) {
				width = preview[screen_id].src.h;
				height = preview[screen_id].src.w;
				buffer_format = V4L2_PIX_FMT_YVU420;
			}
		}

		if ((preview[screen_id].count_render + 1) == BUFFER_CNT ||
			preview[screen_id].count_render == 0)
			count = 4;
		else
			count = preview[screen_id].count_render + 1;
		disp_frame = preview[screen_id].fb[count];
		if (preview[screen_id].viewthread) {
			if (disp_frame == NULL &&
				(count == preview[screen_id].count_display) &&
				(preview[screen_id].count_display != 0))
				return;
		} else {
			if (disp_frame == NULL)
				return;
		}
		config = &preview[screen_id].config[0];
		buffer_format = preview[screen_id].format;
		width = preview[screen_id].src.w;
		height = preview[screen_id].src.h;
#if SHI_DONG
		if (screen_id == 0) {
			srcf[0].addrPhyY =
				(unsigned long)(frame[0]->phy_address);
			srcf[1].addrPhyY =
				(unsigned long)(frame[1]->phy_address);
			srcf[2].addrPhyY =
				(unsigned long)(frame[2]->phy_address);
			srcf[3].addrPhyY =
				(unsigned long)(frame[3]->phy_address);
			srcf[0].addrVirY =
				(unsigned long)(frame[0]->vir_address);
			srcf[1].addrVirY =
				(unsigned long)(frame[1]->vir_address);
			srcf[2].addrVirY =
				(unsigned long)(frame[2]->vir_address);
			srcf[3].addrVirY =
				(unsigned long)(frame[3]->vir_address);
			__dma_flush_range((void *)srcf[0].addrVirY,
					 (void *)srcf[0].addrVirY +
						 width * height * 3 / 2);
			__dma_flush_range((void *)srcf[1].addrVirY,
					 (void *)srcf[1].addrVirY +
						 width * height * 3 / 2);
			__dma_flush_range((void *)srcf[2].addrVirY,
					 (void *)srcf[2].addrVirY +
						 width * height * 3 / 2);
			__dma_flush_range((void *)srcf[3].addrVirY,
					 (void *)srcf[3].addrVirY +
						 width * height * 3 / 2);
			dstf.addrPhyY =
				(unsigned long)(disp_frame->phy_address);
			dstf.addrVirY =
				(unsigned long)(disp_frame->vir_address);
			RenderBVImageMulti(
				ShiTong_Inst[screen_id], (void *)(&srcf[0]), 0, 0,
		(void *)(&srcf[1]), 0, 0, (void *)(&srcf[2]), 0, 0,
		(void *)(&srcf[3]), 0, 0, (void *)(&dstf), 0, 0,
		width, height, width);
		__dma_flush_range((void *)dstf.addrVirY,
		(void *)dstf.addrVirY +
		width * height * 3 / 2);

		} else {
			for (i = 0; i < CAR_MAX_CH; i++) {
				merge_frame(frame[i], disp_frame, buffer_format,
						screen_id, i);
			}
		}

#else
		for (i = 0; i < CAR_MAX_CH; i++) {
			__dma_flush_range((void *)(frame[i]->vir_address),
			(long)(frame[i]->vir_address + width * height * 3 / 2));
			merge_frame(frame[i], disp_frame, buffer_format,
					screen_id, i);
		}
#endif
		preview[screen_id].count_render = count;
		if (preview[screen_id].viewthread) {
			aut_display_start = 1;
		} else {
			aut_display_start = 0;
			switch (buffer_format) {
			case V4L2_PIX_FMT_NV21:
				config->info.fb.format =
					DISP_FORMAT_YUV420_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					(long)disp_frame->phy_address +
					(width * height);
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height / 2;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.x = (unsigned long long)0
							 << 32;
				config->info.fb.crop.y = (unsigned long long)0
							 << 32;
				config->info.fb.crop.width =
					(long)(width) << 32;
				config->info.fb.crop.height =
					(long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w;
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			case V4L2_PIX_FMT_NV61:
				config->info.fb.format =
					DISP_FORMAT_YUV422_SP_VUVU;
				config->info.fb.addr[0] =
				 (long)disp_frame->phy_address;
				config->info.fb.addr[1] =
				config->info.fb.addr[0] + width * height;
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.x = (unsigned long long)0
							 << 32;
				config->info.fb.crop.y = (unsigned long long)0
							 << 32;
				config->info.fb.crop.width =
					(unsigned long long)(width) << 32;
				config->info.fb.crop.height =
					(unsigned long long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w;
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			case V4L2_PIX_FMT_YVU420:
				config->info.fb.format =
					DISP_FORMAT_YUV420_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					(long)disp_frame->phy_address +
					(width * height);
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height / 2;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.width =
					(unsigned long long)width << 32;
				config->info.fb.crop.height =
					(unsigned long long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w; /* FIXME */
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			case V4L2_PIX_FMT_YUV422P:
				config->info.fb.format =
					DISP_FORMAT_YUV422_SP_VUVU;
				config->info.fb.addr[0] =
					(long)disp_frame->phy_address;
				config->info.fb.addr[1] =
					(long)disp_frame->phy_address +
					(width * height);
				config->info.fb.addr[2] = 0;
				config->info.fb.size[0].width = width;
				config->info.fb.size[0].height = height;
				config->info.fb.size[1].width = width / 2;
				config->info.fb.size[1].height = height;
				config->info.fb.size[2].width = 0;
				config->info.fb.size[2].height = 0;
				config->info.fb.align[1] = 0;
				config->info.fb.align[2] = 0;

				config->info.fb.crop.width =
					(long)width << 32;
				config->info.fb.crop.height =
					(long)height << 32;
				config->info.screen_win.x =
					preview[screen_id].frame.x;
				config->info.screen_win.y =
					preview[screen_id].frame.y;
				config->info.screen_win.width =
					preview[screen_id].frame.w; /* FIXME */
				config->info.screen_win.height =
					preview[screen_id].frame.h;

				config->channel = 0;
				config->layer_id = 0;
				config->enable = 1;

				config->info.mode = LAYER_MODE_BUFFER;
				config->info.zorder = 0;
				config->info.alpha_mode = 1;
				config->info.alpha_value = 0xff;
				break;
			default:
				logerror("%s: unknown pixel format, skip\n",
					__func__);
				break;
			}
			bsp_disp_shadow_protect(screen_id, true);
			mgr->force_set_layer_config(
				mgr, &preview[screen_id].config[0],
				preview[screen_id].layer_cnt);
			bsp_disp_shadow_protect(screen_id, false);
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
			if (preview[screen_id].input_src == 0) {
		draw_auxiliary_line(
		preview[screen_id].auxlayer->vir_address,
		AUXLAYER_WIDTH, AUXLAYER_HEIGHT,
		orientation, lr_direct);
		image_rotate(NULL, NULL, screen_id, 1);
			}
#endif
		}
	}
}

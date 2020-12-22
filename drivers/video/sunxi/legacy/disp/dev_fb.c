/* linux/drivers/video/sunxi/disp/dev_fb.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Framebuffer driver for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "drv_disp_i.h"
#include "dev_disp.h"

extern fb_info_t g_fbi;

#define FBHANDTOID(handle)  ((handle) - 100)
#define FBIDTOHAND(ID)  ((ID) + 100)

static struct __fb_addr_para g_fb_addr;

__s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para)
{
	if(fb_addr_para){
		fb_addr_para->fb_paddr = g_fb_addr.fb_paddr;
		fb_addr_para->fb_size  = g_fb_addr.fb_size;
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL(sunxi_get_fb_addr_para);


//              0:ARGB    1:BRGA    2:ABGR    3:RGBA
//seq           ARGB        BRGA       ARGB       BRGA
//br_swqp    0              0            1              1
static __s32 parser_disp_init_para(__disp_init_t * init_para)
{
	int  value;
	int  i;

	memset(init_para, 0, sizeof(__disp_init_t));

	if(OSAL_Script_FetchParser_Data("disp_init", "disp_init_enable", &value, 1) < 0) {
		__wrn("fetch script data disp_init.disp_init_enable fail\n");
		return -1;
	}
	init_para->b_init = value;

	if(OSAL_Script_FetchParser_Data("disp_init", "disp_mode", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.disp_mode fail\n");
		return -1;
	}
	init_para->disp_mode= value;

	//screen0
	if(OSAL_Script_FetchParser_Data("disp_init", "screen0_output_type", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.screen0_output_type fail\n");
		return -1;
	}
	if(value == 0) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_NONE;
	}	else if(value == 1) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_LCD;
	}	else if(value == 2)	{
		init_para->output_type[0] = DISP_OUTPUT_TYPE_TV;
	}	else if(value == 3)	{
		init_para->output_type[0] = DISP_OUTPUT_TYPE_HDMI;
	}	else if(value == 4)	{
		init_para->output_type[0] = DISP_OUTPUT_TYPE_VGA;
	}	else {
		__wrn("invalid screen0_output_type %d\n", init_para->output_type[0]);
		return -1;
	}

	if(OSAL_Script_FetchParser_Data("disp_init", "screen0_output_mode", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.screen0_output_mode fail\n");
		return -1;
	}
	if(init_para->output_type[0] == DISP_OUTPUT_TYPE_TV || init_para->output_type[0] == DISP_OUTPUT_TYPE_HDMI
	    || init_para->output_type[0] == DISP_OUTPUT_TYPE_VGA) {
		init_para->output_mode[0]= value;
	}

	//screen1
	if(OSAL_Script_FetchParser_Data("disp_init", "screen1_output_type", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.screen1_output_type fail\n");
		return -1;
	}
	if(value == 0) {
		init_para->output_type[1] = DISP_OUTPUT_TYPE_NONE;
	}	else if(value == 1)	{
		init_para->output_type[1] = DISP_OUTPUT_TYPE_LCD;
	}	else if(value == 2)	{
		init_para->output_type[1] = DISP_OUTPUT_TYPE_TV;
	}	else if(value == 3)	{
		init_para->output_type[1] = DISP_OUTPUT_TYPE_HDMI;
	}	else if(value == 4)	{
		init_para->output_type[1] = DISP_OUTPUT_TYPE_VGA;
	}	else {
		__wrn("invalid screen1_output_type %d\n", init_para->output_type[1]);
		return -1;
	}

	if(OSAL_Script_FetchParser_Data("disp_init", "screen1_output_mode", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.screen1_output_mode fail\n");
		return -1;
	}
	if(init_para->output_type[1] == DISP_OUTPUT_TYPE_TV || init_para->output_type[1] == DISP_OUTPUT_TYPE_HDMI
	    || init_para->output_type[0] == DISP_OUTPUT_TYPE_VGA) {
		init_para->output_mode[1]= value;
	}

	//fb0
	init_para->buffer_num[0]= 2;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb0_format", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb0_format fail\n");
		return -1;
	}
	init_para->format[0]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb0_pixel_sequence", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb0_pixel_sequence fail\n");
		return -1;
	}
	init_para->seq[0]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb0_scaler_mode_enable", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb0_scaler_mode_enable fail\n");
		return -1;
	}
	init_para->scaler_mode[0]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb0_width", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.fb0_width fail\n");
		return -1;
	}
	init_para->fb_width[0]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb0_height", &value, 1) < 0)	{
		__wrn("fetch script data disp_init.fb0_height fail\n");
		return -1;
	}
	init_para->fb_height[0]= value;

	//fb1
	init_para->buffer_num[1]= 2;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb1_format", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb1_format fail\n");
		return -1;
	}
	init_para->format[1]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb1_pixel_sequence", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb1_pixel_sequence fail\n");
		return -1;
	}
	init_para->seq[1]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb1_scaler_mode_enable", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb1_scaler_mode_enable fail\n");
		return -1;
	}
	init_para->scaler_mode[1]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb1_width", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb1_width fail\n");
		return -1;
	}
	init_para->fb_width[1]= value;

	if(OSAL_Script_FetchParser_Data("disp_init", "fb1_height", &value, 1) < 0) {
		__wrn("fetch script data disp_init.fb1_height fail\n");
		return -1;
	}
	init_para->fb_height[1]= value;


	__inf("====display init para begin====\n");
	__inf("b_init:%d\n", init_para->b_init);
	__inf("disp_mode:%d\n\n", init_para->disp_mode);
	for(i=0; i<2; i++) {
		__inf("output_type[%d]:%d\n", i, init_para->output_type[i]);
		__inf("output_mode[%d]:%d\n", i, init_para->output_mode[i]);
	}
	for(i=0; i<2; i++) {
		__inf("buffer_num[%d]:%d\n", i, init_para->buffer_num[i]);
		__inf("format[%d]:%d\n", i, init_para->format[i]);
		__inf("seq[%d]:%d\n", i, init_para->seq[i]);
		__inf("br_swap[%d]:%d\n", i, init_para->br_swap[i]);
		__inf("b_scaler_mode[%d]:%d\n", i, init_para->scaler_mode[i]);
		__inf("fb_width[%d]:%d\n", i, init_para->fb_width[i]);
		__inf("fb_height[%d]:%d\n", i, init_para->fb_height[i]);
	}
	__inf("====display init para end====\n");

	return 0;
}

__s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var)
{
	__u32 i=0, j=0;

	if(!base)
		return -1;

	for(i = 0; i<height; i++) {
		for(j = 0; j<width/4; j++) {
			__u32 offset = 0;

			if(var->bits_per_pixel == 32)	{
					offset = width * i + j;
					sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset));

					offset = width * i + j + width/4;
					sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->green.length)-1)<<var->green.offset));

					offset = width * i + j + width/4*2;
					sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->blue.length)-1)<<var->blue.offset));

					offset = width * i + j + width/4*3;
					sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset) | (((1<<var->green.length)-1)<<var->green.offset));
				}
				else if(var->bits_per_pixel == 16) {
					offset = width * i + j;
					sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset));

					offset = width * i + j + width/4;
					sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->green.length)-1)<<var->green.offset));

					offset = width * i + j + width/4*2;
					sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->blue.length)-1)<<var->blue.offset));

					offset = width * i + j + width/4*3;
					sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset) | (((1<<var->green.length)-1)<<var->green.offset));
			}
		}
	}

	return 0;
}

__s32 fb_draw_gray_pictures(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var)
{
	__u32 time = 0;

	for(time = 0; time<18; time++) {
		__u32 i=0, j=0;

		for(i = 0; i<height; i++)	{
			for(j = 0; j<width; j++) {
				__u32 addr = base + (i*width+ j)*4;
				__u32 value = (0xff<<24) | ((time*15)<<16) | ((time*15)<<8) | (time*15);

				sys_put_wvalue(addr, value);
			}
		}
	}
	return 0;
}

static int Fb_map_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	struct page *page;

	page = alloc_pages(GFP_KERNEL, get_order(map_size));
	if(page != NULL) {
		info->screen_base = page_address(page);
		info->fix.smem_start = virt_to_phys(info->screen_base);
		memset(info->screen_base,0x0,info->fix.smem_len);
		__inf("Fb_map_video_memory(alloc pages), pa=0x%08lx size:0x%x\n",info->fix.smem_start, info->fix.smem_len);
		return 0;
	}	else {
		__wrn("alloc_pages fail! size:0x%x\n", info->fix.smem_len);
		return -ENOMEM;
	}
#else
	info->screen_base = (char __iomem *)disp_malloc(info->fix.smem_len, (__u32 *)(&info->fix.smem_start));
	if(info->screen_base)	{
		__inf("Fb_map_video_memory(reserve), pa=0x%x size:0x%x\n",(unsigned int)info->fix.smem_start, (unsigned int)info->fix.smem_len);
		memset((void* __force)info->screen_base,0x0,info->fix.smem_len);

		g_fb_addr.fb_paddr = (unsigned int)info->fix.smem_start;
		g_fb_addr.fb_size=info->fix.smem_len;

		return 0;
	} else {
		__wrn("disp_malloc fail!\n");
		return -ENOMEM;
	}

	return 0;
#endif
}


static inline void Fb_unmap_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);

	free_pages((unsigned long)info->screen_base,get_order(map_size));
#else
	disp_free((void * __force)info->screen_base, (void*)info->fix.smem_start, info->fix.smem_len);
#endif
}



static __s32 disp_fb_to_var(__disp_pixel_fmt_t format, __disp_pixel_seq_t seq, __bool br_swap, struct fb_var_screeninfo *var)//todo
{
	if(format==DISP_FORMAT_ARGB8888) {
		var->bits_per_pixel = 32;
		var->transp.length = 8;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		/* argb */
		if(seq == DISP_SEQ_ARGB && br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
			var->transp.offset = var->red.offset + var->red.length;
		}
		/* bgra */
		else if(seq == DISP_SEQ_BGRA && br_swap == 0)	{
			var->transp.offset = 0;
			var->red.offset = var->transp.offset + var->transp.length;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
		}
		/* abgr */
		else if(seq == DISP_SEQ_ARGB && br_swap == 1)	{
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
			var->transp.offset = var->blue.offset + var->blue.length;
		}
		/* rgba */
		else if(seq == DISP_SEQ_BGRA && br_swap == 1)	{
			var->transp.offset = 0;
			var->blue.offset = var->transp.offset + var->transp.length;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
		}
	}
	else if(format==DISP_FORMAT_RGB888) {
		var->bits_per_pixel = 24;
		var->transp.length = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		/* rgb */
		if(br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
		}
		/* bgr */
		else {
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
		}
	}
	else if(format==DISP_FORMAT_RGB655)	{
		var->bits_per_pixel = 16;
		var->transp.length = 0;
		var->red.length = 6;
		var->green.length = 5;
		var->blue.length = 5;
		/* rgb */
		if(br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
		}
		/* bgr */
		else {
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
		}
	}
	else if(format==DISP_FORMAT_RGB565) {
		var->bits_per_pixel = 16;
		var->transp.length = 0;
		var->red.length = 5;
		var->green.length = 6;
		var->blue.length = 5;
		/* rgb */
		if(br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
		}
		/* bgr */
		else {
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
		}
	}
	else if(format==DISP_FORMAT_RGB556) {
		var->bits_per_pixel = 16;
		var->transp.length = 0;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 6;
		/* rgb */
		if(br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
		}
		/* bgr */
		else {
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->blue.offset + var->blue.length;
		}
	}
	else if(format==DISP_FORMAT_ARGB1555) {
		var->bits_per_pixel = 16;
		var->transp.length = 1;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		/* rgb */
		if(br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
			var->transp.offset = var->red.offset + var->red.length;
		}
		/* bgr */
		else {
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
			var->transp.offset = var->blue.offset + var->blue.length;
		}
	}
	else if(format==DISP_FORMAT_RGBA5551) {
		var->bits_per_pixel = 16;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->transp.length = 1;
		/* rgba */
		if(br_swap == 0) {
			var->transp.offset = 0;
			var->blue.offset = var->transp.offset + var->transp.length;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
		}
		/* bgra */
		else {
			var->transp.offset = 0;
			var->red.offset = var->transp.offset + var->transp.length;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
		}
	}
	else if(format==DISP_FORMAT_ARGB4444) {
		var->bits_per_pixel = 16;
		var->transp.length = 4;
		var->red.length = 4;
		var->green.length = 4;
		var->blue.length = 4;
		/* argb */
		if(br_swap == 0) {
			var->blue.offset = 0;
			var->green.offset = var->blue.offset + var->blue.length;
			var->red.offset = var->green.offset + var->green.length;
			var->transp.offset = var->red.offset + var->red.length;
		}
		/* abgr */
		else {
			var->red.offset = 0;
			var->green.offset = var->red.offset + var->red.length;
			var->blue.offset = var->green.offset + var->green.length;
			var->transp.offset = var->blue.offset + var->blue.length;
		}
	}

	return 0;
}

static __s32 var_to_disp_fb(__disp_fb_t *fb, struct fb_var_screeninfo *var, struct fb_fix_screeninfo * fix)//todo
{
	if(var->nonstd == 0)//argb
	{
		var->reserved[0] = DISP_MOD_INTERLEAVED;
		var->reserved[1] = DISP_FORMAT_ARGB8888;
		var->reserved[2] = DISP_SEQ_ARGB;
		var->reserved[3] = 0;

		switch (var->bits_per_pixel)
		{
			case 1:
				var->red.offset = var->green.offset = var->blue.offset	= 0;
				var->red.length	= var->green.length = var->blue.length	= 1;
				var->reserved[1] = DISP_FORMAT_1BPP;
				break;

			case 2:
				var->red.offset = var->green.offset = var->blue.offset	= 0;
				var->red.length	= var->green.length = var->blue.length	= 2;
				var->reserved[1] = DISP_FORMAT_2BPP;
				break;

			case 4:
				var->red.offset = var->green.offset = var->blue.offset	= 0;
				var->red.length	= var->green.length = var->blue.length	= 4;
				var->reserved[1] = DISP_FORMAT_4BPP;
				break;

			case 8:
				var->red.offset = var->green.offset = var->blue.offset	= 0;
				var->red.length	= var->green.length = var->blue.length	= 8;
				var->reserved[1] = DISP_FORMAT_8BPP;
				break;

			case 16:
				if(var->red.length==6 && var->green.length==5 && var->blue.length==5)	{
					var->reserved[1] = DISP_FORMAT_RGB655;
					/* rgb */
					if(var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0) {
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
					/* bgr */
					else if(var->blue.offset == 11 && var->green.offset == 6 && var->red.offset == 0)	{
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 1;
					}	else {
						__wrn("invalid RGB655 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
				}
				else if(var->red.length==5 && var->green.length==6 && var->blue.length==5) {
					var->reserved[1] = DISP_FORMAT_RGB565;
					/* rgb */
					if(var->red.offset == 11 && var->green.offset == 5 && var->blue.offset == 0) {
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
					/* bgr */
					else if(var->blue.offset == 11 && var->green.offset == 5 && var->red.offset == 0)	{
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 1;
					}	else {
						__wrn("invalid RGB565 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
				}
				else if(var->red.length==5 && var->green.length==5 && var->blue.length==6) {
					var->reserved[1] = DISP_FORMAT_RGB556;
					/* rgb */
					if(var->red.offset == 11 && var->green.offset == 6 && var->blue.offset == 0) {
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
					/* bgr */
					else if(var->blue.offset == 10 && var->green.offset == 5 && var->red.offset == 0)	{
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 1;
					}	else {
						__wrn("invalid RGB556 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
				}
				else if(var->transp.length==1 && var->red.length==5 && var->green.length==5 && var->blue.length==5)	{
					var->reserved[1] = DISP_FORMAT_ARGB1555;
					/* argb */
					if(var->transp.offset == 15 && var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0) {
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
					/* abgr */
					else if(var->transp.offset == 15 && var->blue.offset == 10 && var->green.offset == 5 && var->red.offset == 0) {
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 1;
					}	else {
						__wrn("invalid ARGB1555 format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
				}
				else if(var->transp.length==4 && var->red.length==4 && var->green.length==4 && var->blue.length==4)	{
					var->reserved[1] = DISP_FORMAT_ARGB4444;
					/* argb */
					if(var->transp.offset == 12 && var->red.offset == 8 && var->green.offset == 4 && var->blue.offset == 0)	{
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
					/* abgr */
					else if(var->transp.offset == 12 && var->blue.offset == 8 && var->green.offset == 4 && var->red.offset == 0) {
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 1;
					}	else {
						__wrn("invalid ARGB4444 format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
						var->reserved[2] = DISP_SEQ_ARGB;
						var->reserved[3] = 0;
					}
				}	else {
					__wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
					return -EINVAL;
				}
				break;

			case 24:
				var->red.length		= 8;
				var->green.length	= 8;
				var->blue.length	= 8;
				var->reserved[1] = DISP_FORMAT_RGB888;
				/* rgb */
				if(var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0) {
					var->reserved[2] = DISP_SEQ_ARGB;
					var->reserved[3] = 0;
				}
				/* bgr */
				else if(var->blue.offset == 16 && var->green.offset == 8&& var->red.offset == 0) {
					var->reserved[2] = DISP_SEQ_ARGB;
					var->reserved[3] = 1;
				}	else {
					__wrn("invalid RGB888 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
					var->reserved[2] = DISP_SEQ_ARGB;
					var->reserved[3] = 0;
				}
				break;

			case 32:
				var->transp.length  = 8;
				var->red.length		= 8;
				var->green.length	= 8;
				var->blue.length	= 8;
				var->reserved[1] = DISP_FORMAT_ARGB8888;
				/* argb */
				if(var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0) {
					var->reserved[2] = DISP_SEQ_ARGB;
					var->reserved[3] = 0;
				}
				/* bgra */
				else if(var->blue.offset == 24 && var->green.offset == 16 && var->red.offset == 8) {
					var->reserved[2] = DISP_SEQ_BGRA;
					var->reserved[3] = 0;
				}
				/* abgr */
				else if(var->blue.offset == 16 && var->green.offset == 8 && var->red.offset == 0)	{
					var->reserved[2] = DISP_SEQ_ARGB;
					var->reserved[3] = 1;
				}
				/* rgba */
				else if(var->red.offset == 24 && var->green.offset == 16 && var->blue.offset == 8) {
					var->reserved[2] = DISP_SEQ_BGRA;
					var->reserved[3] = 1;
				}	else {
					__wrn("invalid argb format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
					var->reserved[2] = DISP_SEQ_ARGB;
					var->reserved[3] = 0;
				}
				break;

			default:
				__wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
				return -EINVAL;
		}
	}

	fb->mode = var->reserved[0];
	fb->format = var->reserved[1];
	fb->seq = var->reserved[2];
	fb->br_swap = var->reserved[3];
	fb->size.width = var->xres_virtual;

	fix->line_length = (var->xres_virtual * var->bits_per_pixel) / 8;

	return 0;
}


static int Fb_open(struct fb_info *info, int user)
{
	return 0;
}
static int Fb_release(struct fb_info *info, int user)
{
	return 0;
}


static int Fb_wait_for_vsync(struct fb_info *info)
{
	unsigned long count;
	__u32 sel = 0;
	int ret;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
			if(bsp_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_NONE) {
				return 0;
			}

			count = g_fbi.wait_count[sel];
			ret = wait_event_interruptible_timeout(g_fbi.wait[sel], count != g_fbi.wait_count[sel], msecs_to_jiffies(50));
			if (ret == 0)	{
				__inf("timeout\n");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static int Fb_pan_display(struct fb_var_screeninfo *var,struct fb_info *info)
{
	__u32 sel = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	//__inf("Fb_pan_display\n");

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
			__s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
			__disp_layer_info_t layer_para;
			__u32 buffer_num = 1;
			__u32 y_offset = 0;

			if(g_fbi.fb_mode[info->node] == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)	{
				if(sel != var->reserved[0])	{
					return -1;
				}
			}

			if(g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB) {
				buffer_num = 2;
			}
			if((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB)) {
				y_offset = var->yres / 2;
			}

			bsp_disp_layer_get_para(sel, layer_hdl, &layer_para);

			if(layer_para.mode == DISP_LAYER_WORK_MODE_SCALER) {
				layer_para.src_win.x = var->xoffset;
				layer_para.src_win.y = var->yoffset + y_offset;
				layer_para.src_win.width = var->xres;
				layer_para.src_win.height = var->yres / buffer_num;

				bsp_disp_layer_set_src_window(sel, layer_hdl, &(layer_para.src_win));
			}	else {
				layer_para.src_win.x = var->xoffset;
				layer_para.src_win.y = var->yoffset + y_offset;
				layer_para.src_win.width = var->xres;
				layer_para.src_win.height = var->yres / buffer_num;

				layer_para.scn_win.width = var->xres;
				layer_para.scn_win.height = var->yres / buffer_num;

				bsp_disp_layer_set_src_window(sel, layer_hdl, &(layer_para.src_win));
				bsp_disp_layer_set_screen_window(sel, layer_hdl, &(layer_para.scn_win));
			}
		}
	}

	Fb_wait_for_vsync(info);

	return 0;
}

static int Fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)//todo
{
	return 0;
}

static int Fb_set_par(struct fb_info *info)//todo
{
	__u32 sel = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Fb_set_par\n");

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
			struct fb_var_screeninfo *var = &info->var;
			struct fb_fix_screeninfo * fix = &info->fix;
			__s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
			__disp_layer_info_t layer_para;
			__u32 buffer_num = 1;
			__u32 y_offset = 0;

			if(g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB) {
				buffer_num = 2;
			}
			if((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB)) {
				y_offset = var->yres / 2;
			}
			bsp_disp_layer_get_para(sel, layer_hdl, &layer_para);

			var_to_disp_fb(&(layer_para.fb), var, fix);
			layer_para.src_win.x = var->xoffset;
			layer_para.src_win.y = var->yoffset + y_offset;
			layer_para.src_win.width = var->xres;
			layer_para.src_win.height = var->yres / buffer_num;
			if(layer_para.mode != DISP_LAYER_WORK_MODE_SCALER) {
				layer_para.scn_win.width = layer_para.src_win.width;
				layer_para.scn_win.height = layer_para.src_win.height;
			}
			bsp_disp_layer_set_para(sel, layer_hdl, &layer_para);
		}
	}
	return 0;
}


static int Fb_setcolreg(unsigned regno,unsigned red, unsigned green, unsigned blue,unsigned transp, struct fb_info *info)
{
	__u32 sel = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Fb_setcolreg,regno=%d,a=%d,r=%d,g=%d,b=%d\n",regno, transp,red, green, blue);

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
			unsigned int val;

			switch (info->fix.visual)	{
			case FB_VISUAL_PSEUDOCOLOR:
				if (regno < 256) {
					val = (transp<<24) | (red<<16) | (green<<8) | blue;
					bsp_disp_set_palette_table(sel, &val, regno*4, 4);
				}
				break;

			default:
				break;
			}
		}
	}

	return 0;
}

static int Fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	__u32 sel = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Fb_setcmap\n");

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
			unsigned int j = 0, val = 0;
			unsigned char hred, hgreen, hblue, htransp = 0xff;
			unsigned short *red, *green, *blue, *transp;

			red = cmap->red;
			green = cmap->green;
			blue = cmap->blue;
			transp = cmap->transp;

			for (j = 0; j < cmap->len; j++)	{
				hred = (*red++)&0xff;
				hgreen = (*green++)&0xff;
				hblue = (*blue++)&0xff;
				if (transp)	{
					htransp = (*transp++)&0xff;
				}	else {
				  htransp = 0xff;
				}

				val = (htransp<<24) | (hred<<16) | (hgreen<<8) |hblue;
				bsp_disp_set_palette_table(sel, &val, (cmap->start + j) * 4, 4);
			}
		}
	}
	return 0;
}

static int Fb_blank(int blank_mode, struct fb_info *info)
{
	__u32 sel = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Fb_blank,mode:%d\n",blank_mode);

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
		__s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];

		if (blank_mode == FB_BLANK_POWERDOWN)	{
			bsp_disp_layer_close(sel, layer_hdl);
		}	else {
			bsp_disp_layer_open(sel, layer_hdl);
		}
		//DRV_disp_wait_cmd_finish(sel);
		}
	}
	return 0;
}

static int Fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	__inf("Fb_cursor\n");

	return 0;
}

__s32 DRV_disp_vsync_event(__u32 sel)
{
	g_fbi.vsync_timestamp[sel] = ktime_get();

	schedule_work(&g_fbi.vsync_work[sel]);

	return 0;
}

__s32 DRV_disp_take_effect_event(__u32 sel)
{
	if(sel == 0 && g_fbi.cur_count != g_fbi.cb_w_conut) {
		g_fbi.cur_count = g_fbi.cb_w_conut;
		//printk(KERN_WARNING "##take effect:%d\n", g_fbi.cur_count);
	}

	return 0;
}

static void send_vsync_work_0(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC0=%llu",ktime_to_ns(g_fbi.vsync_timestamp[0]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
}

static void send_vsync_work_1(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC1=%llu",ktime_to_ns(g_fbi.vsync_timestamp[1]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
}

__s32 DRV_disp_int_process(__u32 sel)
{
	g_fbi.wait_count[sel]++;
	wake_up_interruptible(&g_fbi.wait[sel]);

	g_fbi.reg_active[sel] = 1;

	if((g_fbi.reg_active[0] || !TCON_get_open_status(0))
	    && (g_fbi.reg_active[1] || !TCON_get_open_status(1))) {
		if(g_fbi.cb_r_conut != g_fbi.cb_w_conut) {
			schedule_work(&g_fbi.post2_cb_work);
		}
	}

	return 0;
}

static void post2_cb(struct work_struct *work)
{
	int r_count = 0;

	mutex_lock(&g_fbi.runtime_lock);
	r_count = g_fbi.cb_r_conut;
	while(r_count != g_fbi.cb_w_conut) {
	if(r_count >= 9) {
		r_count = 0;
	}	else {
		r_count++;
	}
	if(r_count == g_fbi.cur_count) {
		break;
	}	else if(g_fbi.cb_arg[r_count] != NULL)	{
		//printk(KERN_WARNING "##r_conut:%d %x\n", r_count, (unsigned int)g_fbi.cb_arg[r_count]);
		g_fbi.cb_fn(g_fbi.cb_arg[r_count], 1);
		g_fbi.cb_arg[r_count] = NULL;
		g_fbi.cb_r_conut = r_count;
	}
	}

	mutex_unlock(&g_fbi.runtime_lock);
}

static int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, int ui32DispcDataLength, void (*cb_fn)(void *, int), void *cb_arg)
{
	__disp_layer_info_t         layer_info;
	int i,disp,hdl;
	int start_idx, layer_num = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	bsp_disp_cmd_cache(0);
	bsp_disp_cmd_cache(1);
	for(disp = 0; disp < num_screens; disp++)	{
		if(disp == 0)	{
			start_idx = 0;
			layer_num = psDispcData->primary_display_layer_num;
		}	else {
			start_idx = psDispcData->primary_display_layer_num;
			layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
		}

		for(i=0; i<4; i++) {
			hdl = 100 + i;

			bsp_disp_layer_get_para(disp, hdl, &layer_info);
			if(layer_info.mode == DISP_LAYER_WORK_MODE_SCALER) {
				if((i >= layer_num) || (psDispcData->layer_info[start_idx + i].mode == DISP_LAYER_WORK_MODE_NORMAL)) {
					bsp_disp_layer_release(disp, hdl);
					bsp_disp_layer_request(disp, DISP_LAYER_WORK_MODE_NORMAL);
				}
			}
		}
	}

	for(disp = 0; disp < num_screens; disp++) {
		int haveFbTarget = 0;

		if(disp == 0)	{
			start_idx = 0;
			layer_num = psDispcData->primary_display_layer_num;
		}	else {
			start_idx = psDispcData->primary_display_layer_num;
			layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
		}

		for(i=0; i<4; i++) {
			hdl = 100 + i;

			if(i < layer_num)	{
				memcpy(&layer_info, &psDispcData->layer_info[start_idx + i], sizeof(__disp_layer_info_t));

				if(layer_info.fb.mode == DISP_MOD_NON_MB_PLANAR) {
					if(layer_info.fb.format == DISP_FORMAT_YUV420) {
						layer_info.fb.addr[2] = layer_info.fb.addr[0] + layer_info.fb.size.width * layer_info.fb.size.height;
						layer_info.fb.addr[1] = layer_info.fb.addr[2] + (layer_info.fb.size.width * layer_info.fb.size.height)/4;
					}
				}
				bsp_disp_layer_set_para(disp, hdl, &layer_info);
				bsp_disp_layer_open(disp, hdl);
				bsp_disp_layer_set_top(disp, hdl);

				if(i==1 && (psDispcData->layer_info[start_idx + 1].prio < psDispcData->layer_info[start_idx].prio))	{
					haveFbTarget = 1;
				}
			}	else {
				bsp_disp_layer_close(disp, hdl);
			}

			if(haveFbTarget) {
				bsp_disp_layer_set_top(disp, 100);
			}
		}
	}

	bsp_disp_cmd_submit(0);
	bsp_disp_cmd_submit(1);

	mutex_lock(&g_fbi.runtime_lock);
	if(g_fbi.b_no_output) {
		cb_fn(cb_arg, 1);
	}	else {
		g_fbi.cb_fn = cb_fn;
		if(g_fbi.cb_w_conut >= 9)	{
			g_fbi.cb_w_conut = 0;
		}	else {
			g_fbi.cb_w_conut++;
		}
		g_fbi.cb_arg[g_fbi.cb_w_conut] = cb_arg;
		//printk(KERN_WARNING "##w_conut:%d %x %d\n", g_fbi.cb_w_conut, (unsigned int)cb_arg, psDispcData->post2_layers);
	}

	/* have external display */
	if(psDispcData->post2_layers > psDispcData->primary_display_layer_num) {
		g_fbi.reg_active[0] = 0;
		g_fbi.reg_active[1] = 0;
	} else {
		g_fbi.reg_active[0] = 0;
		g_fbi.reg_active[1] = 1;
	}
	mutex_unlock(&g_fbi.runtime_lock);

	return 0;
}

int dispc_blank(int disp, int blank)
{
	int i = 0;

	if(blank)	{
		for(i=0; i<4; i++) {
			bsp_disp_layer_close(disp, 100 + i);
		}
	}
	g_fbi.blank[disp] = blank;

	return 0;
}

static int Fb_ioctl(struct fb_info *info, unsigned int cmd,unsigned long arg)
{
	long ret = 0;
	unsigned long layer_hdl = 0;

	switch (cmd) {
	case FBIOGET_LAYER_HDL_0:
		if(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1) {
			layer_hdl = g_fbi.layer_hdl[info->node][0];
			ret = copy_to_user((void __user *)arg, &layer_hdl, sizeof(unsigned long));
		}	else {
			ret = -1;
		}
		break;

	case FBIOGET_LAYER_HDL_1:
		if(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0) {
			layer_hdl = g_fbi.layer_hdl[info->node][1];
			ret = copy_to_user((void __user *)arg, &layer_hdl, sizeof(unsigned long));
		}	else {
			ret = -1;
		}
		break;

#if 0
	case FBIOGET_VBLANK:
	{
		struct fb_vblank vblank;
		__disp_tcon_timing_t tt;
		__u32 line = 0;
		__u32 sel;

		sel = (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)?1:0;
		line = bsp_disp_get_cur_line(sel);
		bsp_disp_get_timming(sel, &tt);

		memset(&vblank, 0, sizeof(struct fb_vblank));
		vblank.flags |= FB_VBLANK_HAVE_VBLANK;
		vblank.flags |= FB_VBLANK_HAVE_VSYNC;
		if(line <= (tt.ver_total_time-tt.ver_pixels))	{
			vblank.flags |= FB_VBLANK_VBLANKING;
		}
		if((line > tt.ver_front_porch) && (line < (tt.ver_front_porch+tt.ver_sync_time)))	{
			vblank.flags |= FB_VBLANK_VSYNCING;
		}

		if (copy_to_user((void __user *)arg, &vblank, sizeof(struct fb_vblank)))
		ret = -EFAULT;

		break;
	}
#endif

	case FBIO_WAITFORVSYNC:
	{
	//ret = Fb_wait_for_vsync(info);
	break;
	}

	default:
	//__inf("not supported fb io cmd:%x\n", cmd);
	break;
	}
	return ret;
}

static struct fb_ops dispfb_ops =
{
	.owner		    = THIS_MODULE,
	.fb_open        = Fb_open,
	.fb_release     = Fb_release,
	.fb_pan_display	= Fb_pan_display,
	.fb_ioctl       = Fb_ioctl,
	.fb_check_var   = Fb_check_var,
	.fb_set_par     = Fb_set_par,
	.fb_setcolreg   = Fb_setcolreg,
	.fb_setcmap     = Fb_setcmap,
	.fb_blank       = Fb_blank,
	.fb_cursor      = Fb_cursor,
};

/* Greatest common divisor of x and y */
static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0) {
		unsigned long r = x % y;
		x = y;
		y = r;
	}

	return x;
}

/* Least common multiple of x and y */
static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

/* Round x up to a multiple of y */
static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

__s32 Display_Fb_Request(__u32 fb_id, __disp_fb_create_para_t *fb_para)
{
	struct fb_info *info = NULL;
	__s32 hdl = 0;
	__disp_layer_info_t layer_para;
	__u32 sel;
	__u32 xres, yres;
	unsigned long ulLCM;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Display_Fb_Request,fb_id:%d\n", fb_id);

	if(g_fbi.fb_enable[fb_id]) {
		__wrn("Display_Fb_Request, fb%d is already requested!\n", fb_id);
		return DIS_NO_RES;
	}
	info = g_fbi.fbinfo[fb_id];

	xres = fb_para->width;
	yres = fb_para->height;
	if((0 == xres) || (0 == yres)) {
		__wrn("invalid paras xres(%d, yres(%d) in Display_Fb_Request\n", xres, yres);
		return DIS_FAIL;
	}

	info->var.xoffset       = 0;
	info->var.yoffset       = 0;
	info->var.xres          = xres;
	info->var.yres          = yres;
	info->var.xres_virtual  = xres;
	info->fix.line_length   = (fb_para->width * info->var.bits_per_pixel) >> 3;
	ulLCM = LCM(info->fix.line_length, PAGE_SIZE);
	info->fix.smem_len      = RoundUpToMultiple(info->fix.line_length * fb_para->height, ulLCM) * fb_para->buffer_num;
	info->var.yres_virtual  = info->fix.smem_len / info->fix.line_length;
	Fb_map_video_memory(info);

	for(sel = 0; sel < num_screens; sel++) {
		if(((sel==0) && (fb_para->fb_mode != FB_MODE_SCREEN1))
		    || ((sel==1) && (fb_para->fb_mode != FB_MODE_SCREEN0)))	{
			__u32 y_offset = 0, src_width = xres, src_height = yres;

			if(((sel==0) && (fb_para->fb_mode == FB_MODE_SCREEN0 || fb_para->fb_mode == FB_MODE_DUAL_SAME_SCREEN_TB))
			    || ((sel==1) && (fb_para->fb_mode == FB_MODE_SCREEN1))
			    || ((sel == fb_para->primary_screen_id) && (fb_para->fb_mode == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)))	{
				__disp_tcon_timing_t tt;

				if(bsp_disp_get_timming(sel, &tt) >= 0)	{
					info->var.pixclock = 1000000000 / tt.pixel_clk;
					info->var.left_margin = tt.hor_back_porch;
					info->var.right_margin = tt.hor_front_porch;
					info->var.upper_margin = tt.ver_back_porch;
					info->var.lower_margin = tt.ver_front_porch;
					info->var.hsync_len = tt.hor_sync_time;
					info->var.vsync_len = tt.ver_sync_time;
				}
				info->var.width = bsp_disp_get_screen_physical_width(sel);
				info->var.height = bsp_disp_get_screen_physical_height(sel);
			}

			if(fb_para->fb_mode == FB_MODE_DUAL_SAME_SCREEN_TB)	{
				src_height = yres/ 2;
				if(sel == 0) {
					y_offset = yres / 2;
				}
			}

			memset(&layer_para, 0, sizeof(__disp_layer_info_t));
			layer_para.mode = fb_para->mode;
			layer_para.scn_win.width = src_width;
			layer_para.scn_win.height = src_height;
			if(fb_para->fb_mode == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS) {
				if(sel != fb_para->primary_screen_id)	{
					layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
					layer_para.scn_win.width = fb_para->aux_output_width;
					layer_para.scn_win.height = fb_para->aux_output_height;
				}	else if(fb_para->mode == DISP_LAYER_WORK_MODE_SCALER)	{
					layer_para.scn_win.width = fb_para->output_width;
					layer_para.scn_win.height = fb_para->output_height;
				}
			}	else if(fb_para->mode == DISP_LAYER_WORK_MODE_SCALER)	{
				layer_para.scn_win.width = fb_para->output_width;
				layer_para.scn_win.height = fb_para->output_height;
			}

			hdl = bsp_disp_layer_request(sel, layer_para.mode);

			if(hdl == 0) {
				__wrn("Display_Fb_Request, ch%d no layer resource\n", sel);
				Fb_unmap_video_memory(info);

				return DIS_NO_RES;
			}
			layer_para.pipe = 0;
			layer_para.alpha_en = 1;
			layer_para.alpha_val = 0xff;
			layer_para.ck_enable = 0;
			layer_para.src_win.x = 0;
			layer_para.src_win.y = y_offset;
			layer_para.src_win.width = src_width;
			layer_para.src_win.height = src_height;
			layer_para.scn_win.x = 0;
			layer_para.scn_win.y = 0;
			var_to_disp_fb(&(layer_para.fb), &(info->var), &(info->fix));
			layer_para.fb.addr[0] = (__u32)info->fix.smem_start;
			layer_para.fb.addr[1] = 0;
			layer_para.fb.addr[2] = 0;
			layer_para.fb.size.width = fb_para->width;
			layer_para.fb.size.height = fb_para->height;
			layer_para.fb.cs_mode = DISP_BT601;
			layer_para.b_from_screen = 0;
			bsp_disp_layer_set_para(sel, hdl, &layer_para);

			bsp_disp_layer_open(sel, hdl);

			g_fbi.layer_hdl[fb_id][sel] = hdl;
		}
	}

	g_fbi.fb_enable[fb_id] = 1;
	g_fbi.fb_mode[fb_id] = fb_para->fb_mode;
	memcpy(&g_fbi.fb_para[fb_id], fb_para, sizeof(__disp_fb_create_para_t));

	return DIS_SUCCESS;
}

__s32 Display_Fb_Release(__u32 fb_id)
{
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Display_Fb_Release, fb_id:%d\n", fb_id);

	if(g_fbi.fb_enable[fb_id]) {
		__u32 sel = 0;
		struct fb_info * info = g_fbi.fbinfo[fb_id];

		for(sel = 0; sel < num_screens; sel++) {
			if(((sel==0) && (g_fbi.fb_mode[fb_id] != FB_MODE_SCREEN1))
			    || ((sel==1) && (g_fbi.fb_mode[fb_id] != FB_MODE_SCREEN0)))	{
				__s32 layer_hdl = g_fbi.layer_hdl[fb_id][sel];

				bsp_disp_layer_release(sel, layer_hdl);
			}
		}
		g_fbi.layer_hdl[fb_id][0] = 0;
		g_fbi.layer_hdl[fb_id][1] = 0;
		g_fbi.fb_mode[fb_id] = FB_MODE_SCREEN0;
		memset(&g_fbi.fb_para[fb_id], 0, sizeof(__disp_fb_create_para_t));
		g_fbi.fb_enable[fb_id] = 0;

		Fb_unmap_video_memory(info);

		return DIS_SUCCESS;
	}	else {
		__wrn("invalid paras fb_id:%d in Display_Fb_Release\n", fb_id);
		return DIS_FAIL;
	}
}

__s32 Display_Fb_get_para(__u32 fb_id, __disp_fb_create_para_t *fb_para)
{
	__inf("Display_Fb_Release, fb_id:%d\n", fb_id);

	if(g_fbi.fb_enable[fb_id]) {
		memcpy(fb_para, &g_fbi.fb_para[fb_id], sizeof(__disp_fb_create_para_t));

		return DIS_SUCCESS;
	}	else {
		__wrn("invalid paras fb_id:%d in Display_Fb_get_para\n", fb_id);
		return DIS_FAIL;
	}
}

__s32 Display_get_disp_init_para(__disp_init_t * init_para)
{
	memcpy(init_para, &g_fbi.disp_init, sizeof(__disp_init_t));

	return 0;
}

__s32 Display_set_fb_timming(__u32 sel)
{
	__u8 fb_id=0;

	for(fb_id=0; fb_id<FB_MAX; fb_id++) {
		if(g_fbi.fb_enable[fb_id]) {
			if(((sel==0) && (g_fbi.fb_mode[fb_id] == FB_MODE_SCREEN0 || g_fbi.fb_mode[fb_id] == FB_MODE_DUAL_SAME_SCREEN_TB))
			    || ((sel==1) && (g_fbi.fb_mode[fb_id] == FB_MODE_SCREEN1))
			    || ((sel == g_fbi.fb_para[fb_id].primary_screen_id)
			    && (g_fbi.fb_mode[fb_id] == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)))	{
				__disp_tcon_timing_t tt;

				if(bsp_disp_get_timming(sel, &tt)>=0)	{
					g_fbi.fbinfo[fb_id]->var.pixclock = 1000000000 / tt.pixel_clk;
					g_fbi.fbinfo[fb_id]->var.left_margin = tt.hor_back_porch;
					g_fbi.fbinfo[fb_id]->var.right_margin = tt.hor_front_porch;
					g_fbi.fbinfo[fb_id]->var.upper_margin = tt.ver_back_porch;
					g_fbi.fbinfo[fb_id]->var.lower_margin = tt.ver_front_porch;
					g_fbi.fbinfo[fb_id]->var.hsync_len = tt.hor_sync_time;
					g_fbi.fbinfo[fb_id]->var.vsync_len = tt.ver_sync_time;
				}
			}
		}
	}

	return 0;
}

__s32 Fb_Init(void)
{
	__disp_fb_create_para_t fb_para;
	__s32 i;
	__bool need_open_hdmi = 0;
	__u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	pr_info("[DISP]==Fb_Init==\n");

	INIT_WORK(&g_fbi.vsync_work[0], send_vsync_work_0);
	INIT_WORK(&g_fbi.vsync_work[1], send_vsync_work_1);
	INIT_WORK(&g_fbi.post2_cb_work, post2_cb);
	mutex_init(&g_fbi.runtime_lock);

	for(i=0; i<8; i++) {
		g_fbi.fbinfo[i] = framebuffer_alloc(0, g_fbi.dev);
		g_fbi.fbinfo[i]->fbops   = &dispfb_ops;
		g_fbi.fbinfo[i]->flags   = 0;
		g_fbi.fbinfo[i]->device  = g_fbi.dev;
		g_fbi.fbinfo[i]->par     = &g_fbi;
		g_fbi.fbinfo[i]->var.xoffset         = 0;
		g_fbi.fbinfo[i]->var.yoffset         = 0;
		g_fbi.fbinfo[i]->var.xres            = 800;
		g_fbi.fbinfo[i]->var.yres            = 480;
		g_fbi.fbinfo[i]->var.xres_virtual    = 800;
		g_fbi.fbinfo[i]->var.yres_virtual    = 480*2;
		g_fbi.fbinfo[i]->var.nonstd = 0;
		g_fbi.fbinfo[i]->var.bits_per_pixel = 32;
		g_fbi.fbinfo[i]->var.transp.length = 8;
		g_fbi.fbinfo[i]->var.red.length = 8;
		g_fbi.fbinfo[i]->var.green.length = 8;
		g_fbi.fbinfo[i]->var.blue.length = 8;
		g_fbi.fbinfo[i]->var.transp.offset = 24;
		g_fbi.fbinfo[i]->var.red.offset = 16;
		g_fbi.fbinfo[i]->var.green.offset = 8;
		g_fbi.fbinfo[i]->var.blue.offset = 0;
		g_fbi.fbinfo[i]->var.activate = FB_ACTIVATE_FORCE;
		g_fbi.fbinfo[i]->fix.type	    = FB_TYPE_PACKED_PIXELS;
		g_fbi.fbinfo[i]->fix.type_aux	= 0;
		g_fbi.fbinfo[i]->fix.visual 	= FB_VISUAL_TRUECOLOR;
		g_fbi.fbinfo[i]->fix.xpanstep	= 1;
		g_fbi.fbinfo[i]->fix.ypanstep	= 1;
		g_fbi.fbinfo[i]->fix.ywrapstep	= 0;
		g_fbi.fbinfo[i]->fix.accel	    = FB_ACCEL_NONE;
		g_fbi.fbinfo[i]->fix.line_length = g_fbi.fbinfo[i]->var.xres_virtual * 4;
		g_fbi.fbinfo[i]->fix.smem_len = g_fbi.fbinfo[i]->fix.line_length * g_fbi.fbinfo[i]->var.yres_virtual * 2;
		g_fbi.fbinfo[i]->screen_base = NULL;
		g_fbi.fbinfo[i]->fix.smem_start = 0x0;

		register_framebuffer(g_fbi.fbinfo[i]);
	}
	parser_disp_init_para(&(g_fbi.disp_init));


	if(g_fbi.disp_init.b_init) {
		__u32 sel = 0;

		for(sel = 0; sel<num_screens; sel++) {
			if((sel==0) && (g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN0))	{
				if(g_fbi.disp_init.output_type[sel] == DISP_OUTPUT_TYPE_HDMI)	{
					need_open_hdmi = 1;
				}
			}
		}
	}

	if(g_fbi.disp_init.b_init) {
		__u32 fb_num = 0;

		fb_num = (g_fbi.disp_init.disp_mode==DISP_INIT_MODE_TWO_DIFF_SCREEN)?2:1;
		for(i = 0; i<fb_num; i++)	{
			__u32 screen_id = i;

			disp_fb_to_var(g_fbi.disp_init.format[i], g_fbi.disp_init.seq[i], g_fbi.disp_init.br_swap[i], &(g_fbi.fbinfo[i]->var));

			if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN1)	{
				screen_id = 1;
			}
			fb_para.buffer_num= g_fbi.disp_init.buffer_num[i];
			if((g_fbi.disp_init.fb_width[i] == 0) || (g_fbi.disp_init.fb_height[i] == 0))	{
				fb_para.width = bsp_disp_get_screen_width_from_output_type(screen_id,
				    g_fbi.disp_init.output_type[screen_id], g_fbi.disp_init.output_mode[screen_id]);
				fb_para.height = bsp_disp_get_screen_height_from_output_type(screen_id,
				    g_fbi.disp_init.output_type[screen_id], g_fbi.disp_init.output_mode[screen_id]);
			}	else {
				fb_para.width = g_fbi.disp_init.fb_width[i];
				fb_para.height = g_fbi.disp_init.fb_height[i];
			}
			fb_para.output_width = bsp_disp_get_screen_width_from_output_type(screen_id,
				    g_fbi.disp_init.output_type[screen_id], g_fbi.disp_init.output_mode[screen_id]);
			fb_para.output_height = bsp_disp_get_screen_height_from_output_type(screen_id,
				    g_fbi.disp_init.output_type[screen_id], g_fbi.disp_init.output_mode[screen_id]);
			fb_para.mode = (g_fbi.disp_init.scaler_mode[i]==0)?DISP_LAYER_WORK_MODE_NORMAL:DISP_LAYER_WORK_MODE_SCALER;
			if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN0)	{
				fb_para.fb_mode = FB_MODE_SCREEN0;
			}	else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN1) {
				fb_para.fb_mode = FB_MODE_SCREEN1;
			}	else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_TWO_DIFF_SCREEN) {
				if(i == 0) {
					fb_para.fb_mode = FB_MODE_SCREEN0;
				}	else {
					fb_para.fb_mode = FB_MODE_SCREEN1;
				}
			}
			else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_TWO_SAME_SCREEN) {
				fb_para.fb_mode = FB_MODE_DUAL_SAME_SCREEN_TB;
				fb_para.height *= 2;
				fb_para.output_height *= 2;
			}	else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS) {
				fb_para.fb_mode = FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS;
				fb_para.output_width = bsp_disp_get_screen_width_from_output_type(fb_para.primary_screen_id,
				    g_fbi.disp_init.output_type[fb_para.primary_screen_id], g_fbi.disp_init.output_mode[fb_para.primary_screen_id]);
				fb_para.output_height = bsp_disp_get_screen_height_from_output_type(fb_para.primary_screen_id,
				    g_fbi.disp_init.output_type[fb_para.primary_screen_id], g_fbi.disp_init.output_mode[fb_para.primary_screen_id]);
				fb_para.aux_output_width = bsp_disp_get_screen_width(1 - fb_para.primary_screen_id);
				fb_para.aux_output_height = bsp_disp_get_screen_width_from_output_type(1 - fb_para.primary_screen_id,
				    g_fbi.disp_init.output_type[1 - fb_para.primary_screen_id], g_fbi.disp_init.output_mode[1 - fb_para.primary_screen_id]);
			}
			Display_Fb_Request(i, &fb_para);
#if defined (CONFIG_FPGA_V4_PLATFORM)
			fb_draw_colorbar((__u32)g_fbi.fbinfo[i]->screen_base, fb_para.width, fb_para.height*fb_para.buffer_num, &(g_fbi.fbinfo[i]->var));
#endif
		}
#if 0
		if(g_fbi.disp_init.scaler_mode[0])	{
			bsp_disp_print_reg(0, DISP_MOD_FE0, 0);
		}
		if(g_fbi.disp_init.scaler_mode[1]) {
			bsp_disp_print_reg(0, DISP_MOD_FE1, 0);
		}
		if(g_fbi.disp_init.disp_mode != DISP_INIT_MODE_SCREEN1)	{
			bsp_disp_print_reg(0, DISP_MOD_BE0, 0);
			bsp_disp_print_reg(0, DISP_MOD_LCD0, 0);
			if((g_fbi.disp_init.output_type[0] == DISP_OUTPUT_TYPE_TV) || (g_fbi.disp_init.output_type[0] == DISP_OUTPUT_TYPE_VGA))	{
				bsp_disp_print_reg(0, DISP_MOD_TVE0, 0);
			}
		}
		if(g_fbi.disp_init.disp_mode != DISP_INIT_MODE_SCREEN0)	{
			bsp_disp_print_reg(0, DISP_MOD_BE1, 0);
			bsp_disp_print_reg(0, DISP_MOD_LCD1, 0);
			if((g_fbi.disp_init.output_type[1] == DISP_OUTPUT_TYPE_TV) || (g_fbi.disp_init.output_type[1] == DISP_OUTPUT_TYPE_VGA))	{
				bsp_disp_print_reg(0, DISP_MOD_TVE1, 0);
			}
		}
		bsp_disp_print_reg(0, DISP_MOD_CCMU, 0);
		bsp_disp_print_reg(0, DISP_MOD_PWM, 0);
		bsp_disp_print_reg(0, DISP_MOD_PIOC, 0);
		#endif
	}

	return 0;
}

__s32 Fb_Exit(void)
{
	__u8 fb_id=0;

	for(fb_id=0; fb_id<FB_MAX; fb_id++) {
		if(g_fbi.fbinfo[fb_id] != NULL) {
			Display_Fb_Release(FBIDTOHAND(fb_id));
		}
	}

	for(fb_id=0; fb_id<8; fb_id++) {
		unregister_framebuffer(g_fbi.fbinfo[fb_id]);
		framebuffer_release(g_fbi.fbinfo[fb_id]);
		g_fbi.fbinfo[fb_id] = NULL;
	}

	return 0;
}

EXPORT_SYMBOL(dispc_gralloc_queue);


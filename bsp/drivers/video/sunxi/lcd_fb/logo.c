/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * logo.c
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
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
#include "logo.h"
#include <linux/memblock.h>
#include <linux/version.h>

static u32 disp_reserve_size;
static u32 disp_reserve_base;

#ifndef MODULE
int disp_reserve_mem(bool reserve)
{
	if (!disp_reserve_size || !disp_reserve_base)
		return -EINVAL;

	if (reserve)
		memblock_reserve(disp_reserve_base, disp_reserve_size);
	else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		memblock_free(__va(disp_reserve_base), disp_reserve_size);
#else
		memblock_free(disp_reserve_base, disp_reserve_size);
#endif

	return 0;
}

static int __init early_disp_reserve(char *str)
{
	u32 temp[3] = {0};

	if (!str)
		return -EINVAL;

	get_options(str, 3, temp);

	disp_reserve_size = temp[1];
	disp_reserve_base = temp[2];
	if (temp[0] != 2 || disp_reserve_size <= 0)
		return -EINVAL;

	/* arch_mem_end = memblock_end_of_DRAM(); */
	disp_reserve_mem(true);
	pr_info("disp reserve base 0x%x ,size 0x%x\n",
					disp_reserve_base, disp_reserve_size);
	return 0;
}
early_param("disp_reserve", early_disp_reserve);
#endif

static int bmp_to_fb(const void *psrc, struct bmp_header *bmp_header,
			  struct fb_info *info, void *pdst)
{
	int srclinesize, dstlinesize, w, h;
	const unsigned char *psrcline = NULL, *psrcdot = NULL;
	unsigned char *pdstline = NULL, *pdstdot = NULL;
	int i = 0, j = 0, zero_num = 0, bmp_real_height = 0;
	int BppBmp = 3;

	if (!psrc || !pdst || !bmp_header || !info) {
		lcd_fb_wrn("Invalid parameter\n");
		return -1;
	}

	if (bmp_header->bit_count == 24)
		zero_num = (4 - ((3 * bmp_header->width) % 4)) & 3;

	w = (info->var.xres < bmp_header->width) ? info->var.xres
						 : bmp_header->width;
	bmp_real_height = (bmp_header->height & 0x80000000) ? (-bmp_header->
						 height) : (bmp_header->height);
	h = (info->var.yres < bmp_real_height) ? info->var.yres : bmp_real_height;

	BppBmp = bmp_header->bit_count >> 3;
	srclinesize = bmp_header->width * BppBmp + zero_num;
	dstlinesize = info->var.xres * (info->var.bits_per_pixel >> 3);
	psrcline = (const unsigned char *)psrc;
	pdstline = (unsigned char *)pdst;

	if (bmp_header->height & 0x80000000) {
		for (i = 0; i < h; ++i) {
			psrcdot = psrcline;
			pdstdot = pdstline;
			if (info->var.bits_per_pixel == bmp_header->bit_count) {
				memcpy(pdstline, psrcline, srclinesize);
			} else {
				for (j = 0; j < w; ++j) {
					if (info->var.bits_per_pixel == 32 &&
					    bmp_header->bit_count == 24) {
						*pdstdot++ = psrcdot[j * BppBmp];
						*pdstdot++ = psrcdot[j * BppBmp + 1];
						*pdstdot++ = psrcdot[j * BppBmp + 2];
						*pdstdot++ = 0xff;
					} else if (info->var.bits_per_pixel == 16 &&
						   bmp_header->bit_count >= 24) {
						/* little endian */
						*pdstdot++ =
							((psrcdot[j * BppBmp + 1] << 3) &
							 0xe0) +
							((psrcdot[j * BppBmp] >> 3) & 0x1f);
						*pdstdot++ =
							(psrcdot[j * BppBmp + 2] & 0xf8) +
							((psrcdot[j * BppBmp + 1] >> 5) &
							 0x07);
					}
				}
			}
			psrcline += srclinesize;
			pdstline += dstlinesize;
		}
	} else {
		psrcline = (const unsigned char *)psrc +
			   (bmp_real_height - 1) * srclinesize;

		for (i = h - 1; i >= 0; --i) {
			psrcdot = psrcline;
			pdstdot = pdstline;
			if (info->var.bits_per_pixel == bmp_header->bit_count) {
				memcpy(pdstline, psrcline, srclinesize);
			} else {
				for (j = 0; j < w; ++j) {
					if (info->var.bits_per_pixel == 32 &&
					    bmp_header->bit_count == 24) {
						*pdstdot++ = psrcdot[j * BppBmp];
						*pdstdot++ = psrcdot[j * BppBmp + 1];
						*pdstdot++ = psrcdot[j * BppBmp + 2];
						*pdstdot++ = 0xff;
					} else if (info->var.bits_per_pixel == 16 &&
						   bmp_header->bit_count >= 24) {
						/* little endian */
						*pdstdot++ =
							((psrcdot[j * BppBmp + 1] << 3) &
							 0xe0) +
							((psrcdot[j * BppBmp] >> 3) & 0x1f);
						*pdstdot++ =
							(psrcdot[j * BppBmp + 2] & 0xf8) +
							((psrcdot[j * BppBmp + 1] >> 5) &
							 0x07);
					}
				}
			}
			psrcline -= srclinesize;
			pdstline += dstlinesize;
		}
	}

	return 0;
}

static void *Fb_map_kernel(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

static void Fb_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}

static int Fb_map_kernel_logo(struct fb_info *info)
{
	void *vaddr = NULL;
	uintptr_t paddr = 0;
	void *screen_offset = NULL, *image_offset = NULL;
	char *bmp_data = NULL;
	struct bmp_pad_header bmp_pad_header;
	struct bmp_header *bmp_header;
	unsigned int x, y, bmp_bpix;
	unsigned int effective_width, effective_height;
	uintptr_t offset;

	paddr = disp_reserve_base;
	if (!paddr || !disp_reserve_size || !info || !info->screen_base) {
		lcd_fb_wrn("Fb_map_kernel_logo failed!");
		return -1;
	}

	/* parser bmp header */
	offset = paddr & ~PAGE_MASK;
	vaddr = (void *)Fb_map_kernel(paddr, sizeof(struct bmp_header));
	if (vaddr == NULL) {
		lcd_fb_wrn("fb_map_kernel failed, paddr=0x%p,size=0x%x\n",
		      (void *)paddr, (unsigned int)sizeof(struct bmp_header));
		return -1;
	}

	memcpy(&bmp_pad_header.signature[0], vaddr + offset,
	       sizeof(struct bmp_header));
	bmp_header = (struct bmp_header *) &bmp_pad_header.signature[0];
	if ((bmp_header->signature[0] != 'B')
	    || (bmp_header->signature[1] != 'M')) {
		Fb_unmap_kernel(vaddr);
#if IS_ENABLED(CONFIG_DECOMPRESS_LZMA)
		return lzma_decode(paddr, info);
#else
		lcd_fb_wrn("this is not a bmp picture.\n");
		return -1;

#endif
	}

	bmp_bpix = bmp_header->bit_count / 8;

	if ((bmp_bpix != 3) && (bmp_bpix != 4)) {
		lcd_fb_wrn("Only bmp24 and bmp32 is supported!\n");
		return -1;
	}

	x = bmp_header->width;
	y = (bmp_header->height & 0x80000000) ? (-bmp_header->
						 height) : (bmp_header->height);
	if (x != info->var.xres || y != info->var.yres) {
		Fb_unmap_kernel(vaddr);
		lcd_fb_wrn("Bmp  [%u x %u] is not equal to fb[%d x %d]\n", x, y,
			   info->var.xres, info->var.yres);
		return -1;
	}

	if ((paddr <= 0) || x <= 1 || y <= 1) {
		lcd_fb_wrn("kernel logo para error!\n");
		return -EINVAL;
	}

	Fb_unmap_kernel(vaddr);

	/* map the total bmp buffer */
	vaddr =
	    (void *)Fb_map_kernel(paddr,
				  disp_reserve_size);
	if (vaddr == NULL) {
		lcd_fb_wrn("fb_map_kernel failed, paddr=0x%p,size=0x%x\n",
		      (void *)paddr,
		      (unsigned int)(x * y * bmp_bpix +
		       sizeof(struct bmp_header)));
		return -1;
	}

	bmp_data = (char *)(vaddr + bmp_header->data_offset + offset);
	image_offset = (void *)bmp_data;
	effective_width = x;
	effective_height = y;
	screen_offset =
		(void *__force)info->screen_base;

	bmp_to_fb(image_offset, bmp_header, info,
		  screen_offset);

	Fb_unmap_kernel(vaddr);
	return 0;
}

int logo_parse(struct fb_info *info)
{
	int ret = -1;

	ret = Fb_map_kernel_logo(info);

#ifndef MODULE
	disp_reserve_mem(false);
#endif
	return ret;;
}

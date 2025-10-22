/* SPDX-License-Identifier: GPL-2.0-or-later */
/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2021 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2017 - 2021 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program;
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

/*
 *
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#ifndef __VIP__ION__H__
#define __VIP__ION__H__

#include <linux/device.h>
// #include <linux/clk.h>
// #include <linux/clk/sunxi.h>
#include <linux/interrupt.h>


// #include <linux/ion.h>      /*for all "ion api"*/
// #include <linux/ion_sunxi.h>    /*for import "sunxi_ion_client_create"*/
#include <linux/dma-mapping.h>  /*just include "PAGE_SIZE" macro*/
#include <linux/dma-buf.h>



typedef struct _vip_ion_mm {
	size_t size;
	dma_addr_t phy_addr;
	void *vir_addr;
	dma_addr_t dma_addr;
	struct dma_buf *dma_buffer;
	struct ion_client *client;
	struct ion_handle *handle;
} vip_ion_mm_t;

int aw_vip_mem_alloc(struct device *dev, vip_ion_mm_t *vip_mem, const char *name);
void aw_vip_mem_free(struct device *dev, vip_ion_mm_t *vip_mem);
/*void aw_vip_mem_flush(struct device *dev, vip_ion_mm_t *vip_mem);
int aw_vip_mmap(vip_ion_mm_t *vip_mem, struct vm_area_struct *vma, unsigned long pgoff);*/
int aw_vip_munmap(vip_ion_mm_t *vip_mem);

#endif	/*__VIN__OS__H__*/

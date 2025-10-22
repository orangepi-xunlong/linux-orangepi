/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2024 Vivante Corporation
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
*    Copyright (C) 2014 - 2024 Vivante Corporation
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
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#if gcdENABLE_TTM
#ifndef __gc_hal_kernel_ttm_h_
#define __gc_hal_ketnel_ttm_h_

#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_vram_mm_helper.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/ttm/ttm_bo_api.h>
#include <drm/ttm/ttm_page_alloc.h>

#define VIV_MEM_DOMAIN_CPU      (1 << 0)
#define VIV_MEM_DOMAIN_GTT      (1 << 1)
#define VIV_MEM_DOMAIN_VRAM     (1 << 2)
#define VIV_MEM_DOMAIN_PRIV     (1 << 3)

#define VIV_MEM_CREATE_CPU_ACCESS_REQUIRED  (1 << 0)
#define VIV_MEM_CREATE_CPU_NO_CPU_ACCESS    (1 << 1)
#define VIV_MEM_CREATE_CONTIGUOUS_VRAM      (1 << 2)
#define VIV_MEM_CREATE_WAITING_MOVE         (1 << 10)

typedef struct _viv_ttm {
    struct ttm_bo_device bdev;
    gctPOINTER vram_pin_size;
    gctPOINTER visible_pin_size;
} viv_ttm_t;

struct _viv_evict_info {
    gctUINT32 evictedHandle;
    gctPOINTER pageTables;
    gctADDRESS address;
};

typedef struct _viv_bo {
    gckOS os;
    gckKERNEL kernel;
    struct ttm_buffer_object tbo;
    struct ttm_placement placement;
    struct ttm_place    placements[3];
    struct ttm_place    busy_placements[3];
    gctUINT32 preferred_domains;
    gctUINT32 allowed_domains;
    struct ttm_bo_kmap_obj  kmap;
    struct list_head head;
    struct drm_file *reserved_by;
    struct list_head entry;
    struct list_head vma_list;
    gctUINT32 pin_count;
    /*TODO SHUAI reset cpu accessable flags*/
    gctUINT32 flags;
    gckVIDMEM_NODE nodeObject;
    gctBOOL evicting;
    gctUINT32 evictedHandle;
    gctBOOL isImport;
    gctBOOL isExport;
    gctUINT64 userLogical;
    gctUINT32 gpuAddress;
    struct _viv_evict_info *evictInfo;
    gctUINT32 type;
    struct file *file;
    gctUINT64 vma_offset;

    /* per-perocess information */
    gctUINT32 nodeHandle;
    gctUINT32 gemHandle;
    gctUINT32 processID;
    gctPOINTER next;
} viv_bo_t;

struct viv_bo_param {
    unsigned long            size;
    gctINT32                 byte_align;
    gctUINT32                domain;
    gctUINT32                preferred_domain;
    gctUINT32                flags;
    enum ttm_bo_type         type;
    gctUINT32                vidMemType;
    struct dma_resv          *resv;
};

int gckDRM_TTMInit(struct drm_device *drm_drv);

gctPOINTER
gckDRM_GetGALFromTTM(struct ttm_bo_device *dev);

gctPOINTER
gckDRM_GetVIVBo(struct ttm_buffer_object *bo);

gctPOINTER
gckDRM_GetNodeObjFromHandle(gckKERNEL kernel, gctUINT32 processID,
                                    gctUINT32 handle);

#endif
#endif /* gcdENABLE_TTM */

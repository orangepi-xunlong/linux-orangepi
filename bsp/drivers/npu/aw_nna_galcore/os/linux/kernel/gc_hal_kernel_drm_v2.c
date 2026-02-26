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

#if gcdENABLE_DRM

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
# include <drm/drm_drv.h>
# include <drm/drm_file.h>
# include <drm/drm_ioctl.h>
#else
# include <drm/drmP.h>
#endif
#include <drm/drm_gem.h>
#include <linux/dma-buf.h>
#include "gc_hal_kernel_linux.h"
#include "gc_hal_drm.h"
#include "gc_hal_kernel_bo.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
# include <drm/ttm/ttm_bo.h>
#else
# include <drm/ttm/ttm_bo_api.h>
#endif

#define _GC_OBJ_ZONE gcvZONE_KERNEL

#if gcdENABLE_TTM
/*******************************************************************************
 ****************************** gckKERNEL DRM Code *****************************
 *******************************************************************************/

struct viv_gem_object {
    struct drm_gem_object   base;
    uint32_t                node_handle;
    gckVIDMEM_NODE          node_object;
    gctBOOL                 cacheable;
};

static struct vm_operations_struct viv_ttm_vm_ops;
static const struct vm_operations_struct *ttm_vm_ops = gcvNULL;

gctBOOL
_CheckBoAccessable(struct _viv_bo *bo)
{
    gctUINT32 mem_type = bo->tbo.mem.mem_type;

    return mem_type == TTM_PL_PRIV ? gcvFALSE : gcvTRUE;
}

/**
 * this is the function for page fault, when cpu(user) want to write to a
 * logical, while there is no actual physical page map to this logical, system
 * will call this function to insert one physical page to the logical address.
 *
 * And this function will mark this node as a dirty status, that means before
 * kernel commit command buffer, this node will try to sync data with mirror
 * buffer first.
 */
vm_fault_t
_MapMirrorBufferIfW(gckKERNEL kernel, struct vm_area_struct *vma,
                      struct _viv_bo *bo, gctUINT64 address)
{
    gctUINT64 page_offset, page_last;
    struct ttm_buffer_object *tbo = &bo->tbo;
    vm_fault_t ret = VM_FAULT_NOPAGE;
    gckVIDMEM_NODE nodeObject;
    gctUINT32 processID;
    gcsBUFSYNC *buf_sync;
    PLINUX_MDL mdl;
    struct page **pages = gcvNULL;
    struct page *page;
    gctUINT32 i;
    unsigned long pfn;
    gceSTATUS status;
    struct vm_area_struct cvma;

    page_offset = ((address - vma->vm_start) >> PAGE_SHIFT) +
                    vma->vm_pgoff - drm_vma_node_start(&tbo->base.vma_node);

    page_last = vma_pages(vma) + vma->vm_pgoff -
                drm_vma_node_start(&tbo->base.vma_node);

    gckOS_GetProcessID(&processID);
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID,
                                        bo->nodeHandle,
                                        &nodeObject));

    if (!nodeObject->mirror.mirrorNode) {
    /* create mirror buffer for exclusive BO in virtual pool */
        gcmkONERROR(gckVIDMEM_NODE_AllocateMirrorBuf(kernel, nodeObject,
                                        gcvMIRROR_TYPE_SYSTEM_MEMORY_MIRROR));
    }

    /* get the page which the mirror buffer in virtual pool */
    /* only support we will allocate virtual memory from GFP allocator */
    mdl = (PLINUX_MDL)nodeObject->mirror.mirrorNode->node->VidMem.physical;
    if (!mdl->pages) {
        gcmkPRINT("there is no page in the array, maybe not gfp allocator");
        BUG_ON(1);
    } else {
        pages = mdl->pages;
    }

    if (!nodeObject->dirty) {
        /* add node to sync list, prepare to sync mirror => invisible */
        gcmkVERIFY_OK(gckOS_Allocate(kernel->os, sizeof(gcsBUFSYNC),
                                     (gctPOINTER *)&buf_sync));
        gcmkVERIFY_OK(gckOS_ZeroMemory((gctPOINTER)buf_sync,
                                        sizeof(gcsBUFSYNC)));
        gcmkVERIFY_OK(gckVIDMEM_NODE_GetSize(kernel, nodeObject, &buf_sync->size));
        buf_sync->nodeObj = nodeObject;
        buf_sync->next = gcvNULL;
        gcmkONERROR(gckOS_AcquireMutex(kernel->os, kernel->bufSyncListMutex,
                                       gcvINFINITE));
        if (!kernel->bufSyncList) {
            /* if no node add to the list at present */
            kernel->bufSyncList = buf_sync;
        } else {
            /* add node to the head of list */
            buf_sync->next = kernel->bufSyncList;
            kernel->bufSyncList = buf_sync;
        }
        nodeObject->dirty = gcvTRUE;
        gcmkONERROR(gckOS_ReleaseMutex(kernel->os, kernel->bufSyncListMutex));
    }

    cvma = *vma;
    cvma.vm_page_prot = vm_get_page_prot(cvma.vm_flags);
    cvma.vm_page_prot = ttm_io_prot(tbo->mem.placement,
                        cvma.vm_page_prot);

    for (i = 0; i < 1; ++i) {
        page = pages[page_offset];
        if (unlikely(!page && i == 0)) {
            ret = VM_FAULT_OOM;
            goto out_io_unlock;
        } else if (unlikely(!page)) {
            break;
        }
        page->index = drm_vma_node_start(&tbo->base.vma_node) +
            page_offset;
        pfn = page_to_pfn(page);


        if (vma->vm_flags & VM_MIXEDMAP)
            ret = vmf_insert_mixed(vma, address,
                    __pfn_to_pfn_t(pfn, PFN_DEV));
        else
            ret = vmf_insert_pfn(vma, address, pfn);

        /* Never error on prefaulted PTEs */
        if (unlikely((ret & VM_FAULT_ERROR))) {
            if (i == 0)
                goto out_io_unlock;
            else
                break;
        }
        address += PAGE_SIZE;
        if (unlikely(++page_offset >= page_last))
            break;
    }

    nodeObject->mappedOffset = (gctINT32)page_offset - 1;

    ret = VM_FAULT_NOPAGE;

out_io_unlock:
    if (ret && ret != VM_FAULT_NOPAGE)
        WARN_ON(1);

OnError:
    return ret;
}

/**
 * same as _MapMirrorBufferIfW, but this function is prepared for when cpu try
 * to read from a logical.
 * because the read bahavoir is behind of this function, so we should sync the
 * data between mirror buffer here.
 */
vm_fault_t
_MapMirrorBufferIfR(gckKERNEL kernel, struct vm_area_struct *vma,
                      struct _viv_bo *bo, gctUINT64 address)
{
    gctUINT64 page_offset, page_last;
    struct ttm_buffer_object *tbo = &bo->tbo;
    vm_fault_t ret = VM_FAULT_NOPAGE;
    gckVIDMEM_NODE nodeObject;
    gctUINT32 processID;
    PLINUX_MDL mdl;
    struct page **pages = gcvNULL;
    struct page *page;
    gctUINT32 i;
    unsigned long pfn;
    gceSTATUS status;
    gctSIZE_T size;

    page_offset = ((address - vma->vm_start) >> PAGE_SHIFT) +
                    vma->vm_pgoff - drm_vma_node_start(&tbo->base.vma_node);

    page_last = vma_pages(vma) + vma->vm_pgoff -
                drm_vma_node_start(&tbo->base.vma_node);

    gckOS_GetProcessID(&processID);
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID,
                                        bo->nodeHandle,
                                        &nodeObject));

    if (!nodeObject->mirror.mirrorNode) {
    /* create mirror buffer for exclusive BO in virtual pool */
        gcmkONERROR(gckVIDMEM_NODE_AllocateMirrorBuf(kernel, nodeObject,
                                        gcvMIRROR_TYPE_SYSTEM_MEMORY_MIRROR));
    }

    /* sync the memory between node and mirror */
    gcmkONERROR(gckVIDMEM_NODE_GetSize(kernel, nodeObject, &size));
    gcmkONERROR(gckKERNEL_SyncVideoMemoryMirror(kernel, nodeObject, 0, size,
                                    gcvSYNC_MEMORY_DIRECTION_LOCAL_TO_SYSTEM));

    /* get the page which the mirror buffer in virtual pool */
    /* only support we will allocate virtual memory from GFP allocator */
    mdl = (PLINUX_MDL)nodeObject->mirror.mirrorNode->node->VidMem.physical;
    if (!mdl->pages) {
        gcmkPRINT("there is no page in the array, maybe not gfp allocator");
        BUG_ON(1);
    } else {
        pages = mdl->pages;
    }

    for (i = 0; i < 1; ++i) {
        page = pages[page_offset];
        if (unlikely(!page && i == 0)) {
            ret = VM_FAULT_OOM;
            goto out_io_unlock;
        } else if (unlikely(!page)) {
            break;
        }
        page->index = drm_vma_node_start(&tbo->base.vma_node) +
            page_offset;
        pfn = page_to_pfn(page);


        if (vma->vm_flags & VM_MIXEDMAP)
            ret = vmf_insert_mixed(vma, address,
                    __pfn_to_pfn_t(pfn, PFN_DEV));
        else
            ret = vmf_insert_pfn(vma, address, pfn);

        /* Never error on prefaulted PTEs */
        if (unlikely((ret & VM_FAULT_ERROR))) {
            if (i == 0)
                goto out_io_unlock;
            else
                break;
        }
        address += PAGE_SIZE;
        if (unlikely(++page_offset >= page_last))
            break;
    }
    ret = VM_FAULT_NOPAGE;

out_io_unlock:
    if (ret && ret != VM_FAULT_NOPAGE)
        WARN_ON(1);

OnError:
    return ret;
}

vm_fault_t
_MapImport(gckKERNEL kernel, struct vm_area_struct *vma,
                      struct _viv_bo *bo, gctUINT64 address)
{
    return VM_FAULT_NOPAGE;
}

/**
 * this function will check the memory if it can be access by cpu:
 *
 * For accessable memory(virtual, local external, contiguous), this function
 * will try to call the ttm page fault function, to map the memory.
 *
 * For unaccessable memory(local exclusive at present), this function will try
 * to call _MapMirrorBufferIfW/R to insert the physical page to logical.
 */
static vm_fault_t viv_ttm_fault(struct vm_fault *vmf)
{
    struct ttm_buffer_object *tbo =
                        (struct ttm_buffer_object *)vmf->vma->vm_private_data;
    struct _viv_bo *bo = gckDRM_GetVIVBo(tbo);
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(tbo->bdev);
    vm_fault_t ret = VM_FAULT_NOPAGE;
    gctUINT64 address = vmf->address;
    struct vm_area_struct *vma = vmf->vma;

    gctBOOL accessable = gcvTRUE;

    gckDEVICE device = gal_dev->devices[0];
    gckKERNEL kernel = device->kernels[0];

    /* check if cpu can access the pool of this memory */
    accessable = _CheckBoAccessable(bo);
    if (accessable) {
        if (bo->isImport)
            ret = _MapImport(kernel, vma, bo, address);
        else
            ret = ttm_vm_ops->fault(vmf);
    } else {
        /* Check if this vmf is from a write operation from cpu instead of read */
        if (vmf->flags & FAULT_FLAG_WRITE) {
            ret = _MapMirrorBufferIfW(kernel, vma, bo, address);
        } else {
            ret = _MapMirrorBufferIfR(kernel, vma, bo, address);
        }
    }

    return ret;
}

static void ttm_bo_vm_open_viv(struct vm_area_struct *vma)
{

}

static void ttm_bo_vm_close_viv(struct vm_area_struct *vma)
{
    vma->vm_private_data = NULL;
}

int viv_ttm_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int r;
    struct drm_file *file_priv = filp->private_data;
    gckGALDEVICE gal_dev = (gckGALDEVICE)file_priv->minor->dev->dev_private;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    if (gal_dev == NULL) {
        gcmkFOOTER_DRM();
        return -EINVAL;
    }

    r = ttm_bo_mmap(filp, vma, &gal_dev->ttm.bdev);

    if (unlikely(r != 0)) {
        gcmkPRINT("something error in mmap");
        gcmkFOOTER_DRM();
        return r;
    }

    if (ttm_vm_ops == gcvNULL) {
        ttm_vm_ops = vma->vm_ops;
        viv_ttm_vm_ops = *ttm_vm_ops;
        viv_ttm_vm_ops.fault = &viv_ttm_fault;
        viv_ttm_vm_ops.open = &ttm_bo_vm_open_viv;
        viv_ttm_vm_ops.close = &ttm_bo_vm_close_viv;
    }

    vma->vm_ops = &viv_ttm_vm_ops;

    gcmkFOOTER_DRM();

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void viv_gem_free_object(struct drm_gem_object *gem_obj);
struct dma_buf *viv_gem_prime_export(struct drm_gem_object *gem_obj, int flags);

static const struct drm_gem_object_funcs viv_gem_object_funcs = {
    .free   = viv_gem_free_object,
    .export = viv_gem_prime_export,
};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
struct dma_buf *viv_gem_prime_export(struct drm_gem_object *gem_obj, int flags)
{
    struct drm_device *drm = gem_obj->dev;
#else
struct dma_buf *viv_gem_prime_export(struct drm_device *drm, struct drm_gem_object *gem_obj, int flags)
{
#endif
    struct _viv_bo *bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    gckVIDMEM_NODE nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;
    struct dma_buf *dmabuf  = gcvNULL;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    if (gal_dev) {
        gckDEVICE device = gal_dev->devices[0];
        gckKERNEL kernel = device->kernels[0];

        bo->isExport = gcvTRUE;

        gcmkVERIFY_OK(gckVIDMEM_NODE_Export(kernel, nodeObj, flags,
                                            (gctPOINTER *)&dmabuf, gcvNULL));
    } else {
        gcmkPRINT("galdevice is null");
    }
    gcmkFOOTER_DRM();

    return dmabuf;
}

void _FillBoWithHandle(gckGALDEVICE gal_dev, struct _viv_bo *bo,
                     gctUINT32 nodeHandle, gctUINT32 size)
{
    struct ttm_buffer_object *tbo = &bo->tbo;
    gckDEVICE device = gal_dev->devices[0];
    gckKERNEL kernel = device->kernels[0];
    gctUINT32 processID;
    gckVIDMEM_NODE nodeObject;
    struct ttm_bo_device *bdev = &gal_dev->ttm.bdev;
    struct ttm_mem_global *mem_glob = bdev->glob->mem_glob;
    gctINT32 ret;
    gctUINT64 acc_size = 0;
    struct ttm_operation_ctx ctx = { gcvTRUE, gcvFALSE };

    gcmkHEADER_DRM();

    /* init tbo like ttm_bo_init_reserved do*/
    kref_init(&tbo->kref);
    kref_init(&tbo->list_kref);
    atomic_set(&tbo->cpu_writers, 0);

    ret = ttm_mem_global_alloc(mem_glob, acc_size, &ctx);
    tbo->destroy = &gckDRM_BODestroy;
    INIT_LIST_HEAD(&tbo->lru);
    INIT_LIST_HEAD(&tbo->ddestroy);
    INIT_LIST_HEAD(&tbo->swap);
    INIT_LIST_HEAD(&tbo->io_reserve_lru);
    mutex_init(&tbo->wu_mutex);
    tbo->type = ttm_bo_type_sg;
    tbo->bdev = bdev;
    bo->os = kernel->os;

    /* set actual memory to tbo */
    gckOS_GetProcessID(&processID);
    gckVIDMEM_HANDLE_Lookup(kernel, processID, nodeHandle, &nodeObject);
    gckDRM_BOLink2NodeObj(gal_dev, processID, nodeHandle, nodeObject, bo);
    tbo->mem.mm_node = (gctPOINTER)nodeObject;
    tbo->mem.num_pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    tbo->num_pages = tbo->mem.num_pages;
    tbo->mem.size = tbo->mem.num_pages << PAGE_SHIFT;
    tbo->mem.mem_type = TTM_PL_SYSTEM;
    tbo->mem.page_alignment = tbo->mem.num_pages;
    tbo->mem.bus.io_reserved_vm = false;
    tbo->mem.bus.io_reserved_count = 0;
    tbo->priority = 2;
    tbo->moving = NULL;
    tbo->acc_size = acc_size;
    tbo->sg = gcvNULL;

    if (nodeObject->pool == gcvPOOL_LOCAL_EXTERNAL) {
        tbo->mem.placement = (TTM_PL_FLAG_VRAM | TTM_PL_FLAG_UNCACHED);
        tbo->mem.mem_type = TTM_PL_VRAM;
    } else if (nodeObject->pool == gcvPOOL_LOCAL_EXCLUSIVE) {
        tbo->mem.placement = (TTM_PL_FLAG_PRIV | TTM_PL_FLAG_UNCACHED);
        tbo->mem.mem_type = TTM_PL_PRIV;
    } else {
        tbo->mem.placement = (TTM_PL_FLAG_TT | TTM_PL_FLAG_CACHED);
        tbo->mem.mem_type = TTM_PL_TT;
    }
    /* gem init has finish doing resv init and node reset */
    drm_vma_offset_add(&bdev->vma_manager, &tbo->base.vma_node,
                     tbo->mem.num_pages);
    gcmkFOOTER_DRM_ARG("Import Handle = 0x%x", nodeHandle);
}

struct drm_gem_object *viv_gem_prime_import(struct drm_device *drm, struct dma_buf *dmabuf)
{
    struct _viv_bo *bo;
    gctPOINTER pointer;

    gcsHAL_INTERFACE iface;
    gckGALDEVICE gal_dev;
    gckDEVICE device;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        return gcvNULL;

    device = gal_dev->devices[0];

    gckOS_ZeroMemory(&iface, sizeof(iface));
    iface.command = gcvHAL_WRAP_USER_MEMORY;
    iface.hardwareType = device->defaultHwType;
    iface.u.WrapUserMemory.desc.flag = gcvALLOC_FLAG_DMABUF;
    iface.u.WrapUserMemory.desc.handle = -1;
    iface.u.WrapUserMemory.desc.dmabuf = gcmPTR_TO_UINT64(dmabuf);
    gckDEVICE_Dispatch(device, &iface);

    /* ioctl output */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    gem_obj->funcs = &viv_gem_object_funcs;
#endif

    gckOS_Allocate(gal_dev->os, sizeof(struct _viv_bo), &pointer);
    gckOS_ZeroMemory(pointer, sizeof(struct _viv_bo));

    bo = (struct _viv_bo *)pointer;

    drm_gem_private_object_init(drm, &bo->tbo.base, dmabuf->size);

    _FillBoWithHandle(gal_dev, bo, iface.u.WrapUserMemory.node, dmabuf->size);

    bo->isImport = gcvTRUE;

    gcmkFOOTER_DRM();

    return &bo->tbo.base;
}

int viv_gem_create_object_byTTM(struct drm_device *dev,
                                         u32 size, struct drm_gem_object **gem_obj)
{
    struct viv_bo_param bp;
    struct _viv_bo *bo;
    //int ret = 0;
    gckGALDEVICE gal_dev = (gckGALDEVICE)dev->dev_private;
    gceSTATUS status = gcvSTATUS_OK;
    gcmkHEADER_DRM();

    bp.size = size;
    bp.domain = VIV_MEM_DOMAIN_VRAM;
    bp.flags = VIV_MEM_CREATE_CPU_ACCESS_REQUIRED;
    bp.type = ttm_bo_type_device;
    bp.preferred_domain = VIV_MEM_DOMAIN_VRAM;
    bp.byte_align = PAGE_SIZE;
    bp.resv = gcvNULL;

    gckDRM_BOCreate(gal_dev, &bp, &bo);

    *gem_obj = &bo->tbo.base;

    gcmkFOOTER_DRM();

    return 0;
}

/**
 * this function will be called at:
 *
 * 1. gem close. Because the gem kref is always kept as 1 in our driver, close
 *  close will try to release the memory.
 * 2. gem handle delete in release. Only viv driver will try to send the release
 *  ioctl to kernel.
 */
void viv_gem_free_object(struct drm_gem_object *gem_obj)
{
    struct _viv_bo *bo;
    gctUINT32 nodeHandle;
    gctUINT32 gemHandle;

    gcmkHEADER_DRM();

    if (!gem_obj) {
        goto OnError;
    }

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeHandle = bo->nodeHandle;
    gemHandle = bo->gemHandle;

    while (kref_read(&bo->tbo.kref)) {
        ttm_bo_put(&bo->tbo);
    }

OnError:
    gcmkFOOTER_DRM_ARG("nodeHandle = 0x%x, gemHandle = 0x%x", nodeHandle, gemHandle);
    return;
}

static int viv_ioctl_gem_create_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    int ret = 0;
    struct drm_viv_gem_create *args = (struct drm_viv_gem_create *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct viv_bo_param bp;
    struct _viv_bo *bo;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gctUINT64 alignSize = PAGE_ALIGN(args->size);

    gcmkHEADER_DRM();
    bp.size = alignSize;
    args->type = args->type & 0xFF;
    if (args->type != gcvVIDMEM_TYPE_COMMAND &&
        args->type != gcvVIDMEM_TYPE_BITMAP &&
        args->type != gcvVIDMEM_TYPE_FENCE &&
        args->type != gcvVIDMEM_TYPE_GENERIC)
        bp.domain = VIV_MEM_DOMAIN_PRIV;
    else
        bp.domain = VIV_MEM_DOMAIN_VRAM;

    if (args->domain)
        bp.domain = args->domain;

    bp.flags = VIV_MEM_CREATE_CPU_ACCESS_REQUIRED |
               VIV_MEM_CREATE_WAITING_MOVE;
    bp.type = ttm_bo_type_device;
    bp.preferred_domain = bp.domain;
    bp.byte_align = PAGE_SIZE;
    bp.resv = gcvNULL;
    bp.vidMemType = args->type;

    gcmkONERROR(gckDRM_BOCreate(gal_dev, &bp, &bo));

    gem_obj = &bo->tbo.base;
    args->node = bo->nodeHandle;
    ret = drm_gem_handle_create(file, gem_obj, &args->handle);
    bo->gemHandle = args->handle;
    /* drop reference from allocate - handle holds it now */
    drm_gem_object_unreference_unlocked(gem_obj);

OnError:
    gcmkFOOTER_DRM_ARG("nodeHandle = 0x%x, gemHandle = 0x%x", args->node, bo->gemHandle);

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

/**
 * This function receive a gem handle or a viv handle, and try to map the memory,
 * to get the gpu virtual address for gpu access and cpu virtual address for
 * user access.
 *
 * Unlike the previous driver, the cpu virtual address has not yet been mapped to
 * the physical page. In the other words, when a user try to access this address,
 * a page fault will be triggered first.
 *
 * If a bo is locked by this ioctl, it means this bo is in used by GPU.
 */
static int viv_ioctl_gem_lock_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_lock *args = (struct drm_viv_gem_lock *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gckKERNEL kernel = gal_dev->devices[0]->kernels[0];
    gckVIDMEM_NODE nodeObject;
    struct _viv_bo *bo;
    gctUINT64 offset = 0;
    gctUINT64 userLogical = 0;
    gctUINT32 bytes = 0;
    gctUINT32 processID;

    gcsHAL_INTERFACE iface;
    gctUINT32 copyLen;

    gcmkHEADER_DRM();

    copyLen = copy_from_user(&iface,
                                 gcmUINT64_TO_PTR(args->priv),
                                 sizeof(gcsHAL_INTERFACE));

    /* try to get the memory node information
     * viv path get by nodeHandle
     * gem path get by gemHandle
     */
    if (args->handle == 0 && args->node != 0) {
        gckOS_GetProcessID(&processID);
        gckVIDMEM_HANDLE_Lookup(kernel, processID, args->node, &nodeObject);
        gckDRM_BOGetFromNodeHandle(gal_dev, processID, args->node, &bo);
        if (!bo)
            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        bytes = bo->tbo.base.size;
    } else {
        gckOS_GetProcessID(&processID);
        gem_obj = drm_gem_object_lookup(file, args->handle);
        bo = container_of(gem_obj, struct _viv_bo, tbo.base);
        gckVIDMEM_HANDLE_Lookup(kernel, processID, bo->nodeHandle, &nodeObject);
        bytes = gem_obj->size;
    }
    if (bo->isImport)
        nodeObject->needUnmap = 1;
    else
        nodeObject->needUnmap = 0;
    if (args->priv != 0)
        gcmkONERROR(gckDRM_BOLock(gal_dev, bo, gcvSTATUS_FALSE, &args->logical, &args->address, &iface));
    else
        gcmkONERROR(gckDRM_BOLock(gal_dev, bo, gcvSTATUS_FALSE, &args->logical, &args->address, gcvNULL));

    offset = drm_vma_node_offset_addr(&bo->tbo.base.vma_node);

/* if import an invisible node we should node map userlogiical by viv path */
    if (bo->isImport && args->logical) {
        bo->userLogical = args->logical;
        copyLen = copy_to_user(gcmUINT64_TO_PTR(args->priv),
                               &iface,
                               sizeof(gcsHAL_INTERFACE));
        goto OnError;
    }

    userLogical = vm_mmap(file->filp, 0L, bytes,
                  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, offset);

    bo->vma_offset = offset;
    bo->file = file->filp;
    /*vm_mmap will call the mmap function so we do not need call viv_ttm_mmap by ourshelves*/
    if (userLogical != -EINVAL) {
        bo->userLogical = userLogical;
        iface.u.LockVideoMemory.memory = userLogical;
        args->logical = userLogical;
        nodeObject->node->VidMem.logical = gcmUINT64_TO_PTR(userLogical);
    } else {
       gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* this function will fill the mdlMap and mdl */
    gcmkONERROR(gckKERNEL_MapVideoMemory(kernel, gcvTRUE, nodeObject->node->VidMem.pool,
                                                 nodeObject->node->VidMem.physical,
                                                 nodeObject->node->VidMem.offset,
                                                 nodeObject->node->VidMem.bytes,
                                                 &nodeObject->node->VidMem.logical));

    copyLen = copy_to_user(gcmUINT64_TO_PTR(args->priv),
                           &iface,
                           sizeof(gcsHAL_INTERFACE));

OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);

    gcmkFOOTER_DRM_ARG("nodeHandle = 0x%x userLogical = 0x%llx address = 0x%x",
                        bo->nodeHandle, bo->userLogical, args->address);

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_unlock_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_unlock *args = (struct drm_viv_gem_unlock *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    struct _viv_bo *bo;
    gctINT32 ret = 0;
    gctUINT32 bytes = 0;
    gcsHAL_INTERFACE iface;
    gctUINT32 copyLen;
    gctUINT32 pool = gcvPOOL_UNKNOWN;
    gctBOOL async;
    gckKERNEL kernel = gal_dev->devices[0]->kernels[0];
    gckVIDMEM_NODE nodeObject;
    gctUINT32 processID;

    gcmkHEADER_DRM();

    copyLen = copy_from_user(&iface,
                                 gcmUINT64_TO_PTR(args->priv),
                                 sizeof(gcsHAL_INTERFACE));

    if (args->handle == 0 && args->node != 0) {
        gckOS_GetProcessID(&processID);
        gckVIDMEM_HANDLE_Lookup(kernel, processID, args->node, &nodeObject);
        gckDRM_BOGetFromNodeHandle(gal_dev, processID, args->node, &bo);
        bytes = bo->tbo.base.size;
        pool = nodeObject->pool;
        async = args->async;
        if (!bo)
            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        gcmkONERROR(gckDRM_BOUnlock(gal_dev, bo, bo->nodeHandle, &async, 0));
    } else {
        gem_obj = drm_gem_object_lookup(file, args->handle);
        bo = container_of(gem_obj, struct _viv_bo, tbo.base);
        bytes = gem_obj->size;
           async = args->async;
        gcmkONERROR(gckDRM_BOUnlock(gal_dev, bo, bo->nodeHandle, &async, 1));
    }

    if (ret == -EINVAL)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    iface.u.UnlockVideoMemory.pool = pool;
    iface.u.UnlockVideoMemory.bytes = bytes;
    iface.u.UnlockVideoMemory.asynchroneous = async;

    copyLen = copy_to_user(gcmUINT64_TO_PTR(args->priv),
                           &iface,
                           sizeof(gcsHAL_INTERFACE));
OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);

    gcmkFOOTER_DRM_ARG("Handle = 0x%x Kref = 0x%x", bo->nodeHandle,
                                                    kref_read(&bo->tbo.kref));
    return gcmIS_ERROR(status) ? -ENOTTY : 0;;
}

static int viv_ioctl_gem_release_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_release *args = (struct drm_viv_gem_release *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo;
    gctUINT32 nodeHandle, gemHandle;
    gctUINT32 processID;

    gcmkHEADER_DRM();

    /* args->handle : GEM handle
       args->node : node handle */
    if (args->handle == 0 && args->node != 0) {
        gckOS_GetProcessID(&processID);
        gckDRM_BOGetFromNodeHandle(gal_dev, processID, args->node, &bo);
        drm_gem_handle_delete(file, bo->gemHandle);
    } else {
        gem_obj = drm_gem_object_lookup(file, args->handle);
        bo = container_of(gem_obj, struct _viv_bo, tbo.base);
        gcmkPRINT("gem release");
    }

    nodeHandle = bo->nodeHandle;
    gemHandle = bo->gemHandle;
    if (kref_read(&bo->tbo.kref)) {
        ttm_bo_put(&bo->tbo);
    }

    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);

    gcmkFOOTER_DRM_ARG("nodeHandle = 0x%x gemHandle = 0x%x", nodeHandle, gemHandle);
    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_wrap_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_wrap *args = (struct drm_viv_gem_wrap *)data;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gckDEVICE device = gal_dev->devices[0];
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo;
    gcsHAL_INTERFACE iface;
    gctUINT32 copyLen;
    gctPOINTER pointer;
    gctUINT32 nodeHandle, gemHandle;

    gcmkHEADER_DRM();
    copyLen = copy_from_user(&iface,
                              gcmUINT64_TO_PTR(args->priv),
                              sizeof(gcsHAL_INTERFACE));
    status = gckDEVICE_Dispatch(device, &iface);

    gckOS_Allocate(gal_dev->os, sizeof(struct _viv_bo), &pointer);
    gckOS_ZeroMemory(pointer, sizeof(struct _viv_bo));
    bo = (struct _viv_bo *)pointer;
    drm_gem_private_object_init(drm, &bo->tbo.base, iface.u.WrapUserMemory.bytes);
    _FillBoWithHandle(gal_dev, bo, iface.u.WrapUserMemory.node, iface.u.WrapUserMemory.bytes);
    bo->isImport = gcvTRUE;
    nodeHandle = bo->nodeHandle;
    gemHandle = bo->gemHandle;

    /* Copy data back to the user. */
    copyLen = copy_to_user(gcmUINT64_TO_PTR(args->priv),
                           &iface,
                           sizeof(gcsHAL_INTERFACE));

    gcmkFOOTER_DRM_ARG("nodeHandle = 0x%x gemHandle = 0x%x", nodeHandle, gemHandle);

    return gcmIS_ERROR(status) ? -ENOTTY : 0;;

}

static int
viv_ioctl_gem_pf_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_pf *args = (struct drm_viv_gem_pf *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo;

    gcmkHEADER_DRM();

    if (args->handle == 0 && args->node != 0) {
        gctUINT32 processID;

        gckOS_GetProcessID(&processID);
        gckDRM_BOGetFromNodeHandle(gal_dev, processID, args->node, &bo);
        if (!bo)
            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    } else {
        gem_obj = drm_gem_object_lookup(file, args->handle);
        bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    }

    gckDRM_BORevokeMap((gckVIDMEM_NODE)bo->tbo.mem.mm_node);

OnError:
    gcmkFOOTER_DRM();

    return 0;
}

static int viv_ioctl_gem_move_byTTM(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_move *args = (struct drm_viv_gem_move *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo;

    gcmkHEADER_DRM();

    /* args->handle : GEM handle
       args->node : node handle */
    if (args->handle == 0 && args->node != 0) {
        gctUINT32 processID;

        gckOS_GetProcessID(&processID);
        gckDRM_BOGetFromNodeHandle(gal_dev, processID, args->node, &bo);
        if (!bo)
            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    } else {
        gem_obj = drm_gem_object_lookup(file, args->handle);
        bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    }

    gcmkONERROR(gckDRM_BOValidate(gal_dev, bo, args->domain));

    gcmkFOOTER_DRM_ARG("Domain = 0x%x", args->domain);
    return 0;

OnError:
    gcmkFOOTER_DRM();

    return -ENOTTY;
}

static int viv_ioctl_gem_cache(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_cache *args = (struct drm_viv_gem_cache *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;

    gcsHAL_INTERFACE iface;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;
    gckDEVICE device;
    gceCACHEOPERATION cache_op = 0;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    device = gal_dev->devices[0];

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);

    switch (args->op) {
    case DRM_VIV_GEM_CLEAN_CACHE:
        cache_op = gcvCACHE_CLEAN;
        break;
    case DRM_VIV_GEM_INVALIDATE_CACHE:
        cache_op = gcvCACHE_INVALIDATE;
        break;
    case DRM_VIV_GEM_FLUSH_CACHE:
        cache_op = gcvCACHE_FLUSH;
        break;
    case DRM_VIV_GEM_MEMORY_BARRIER:
        cache_op = gcvCACHE_MEMORY_BARRIER;
        break;
    default:
        break;
    }

    gckOS_ZeroMemory(&iface, sizeof(iface));
    iface.command           = gcvHAL_CACHE;
    iface.hardwareType      = device->defaultHwType;
    iface.u.Cache.node      = bo->nodeHandle;
    iface.u.Cache.operation = cache_op;
    iface.u.Cache.logical   = args->logical;
    iface.u.Cache.bytes     = args->bytes;
    gcmkONERROR(gckDEVICE_Dispatch(device, &iface));

OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_query(struct drm_device *drm, void *data, struct drm_file *file)
{
    struct drm_viv_gem_query *args = (struct drm_viv_gem_query *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;
    gckVIDMEM_NODE nodeObj;

    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;

    switch (args->param) {
    case DRM_VIV_GEM_PARAM_POOL:
        args->value = (__u64)nodeObj->pool;
        break;
    case DRM_VIV_GEM_PARAM_SIZE:
        args->value = (__u64)gem_obj->size;
        break;
    default:
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
    }
    gcmkFOOTER_DRM();

OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_timestamp(struct drm_device *drm,
                                   void *data, struct drm_file *file)
{
    struct drm_viv_gem_timestamp *args    = (struct drm_viv_gem_timestamp *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;
    gckVIDMEM_NODE nodeObj;

    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;

    nodeObj->timeStamp += args->inc;
    args->timestamp = nodeObj->timeStamp;

OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);
    gcmkFOOTER_DRM();

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_set_tiling(struct drm_device *drm,
                                    void *data, struct drm_file *file)
{
    struct drm_viv_gem_set_tiling *args    = (struct drm_viv_gem_set_tiling *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;
    gckVIDMEM_NODE nodeObj;

    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;

    nodeObj->tilingMode  = args->tiling_mode;
    nodeObj->tsMode      = args->ts_mode;
    nodeObj->tsCacheMode = args->ts_cache_mode;
    nodeObj->clearValue  = args->clear_value;

OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);
    gcmkFOOTER_DRM();

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_get_tiling(struct drm_device *drm,
                                    void *data, struct drm_file *file)
{
    struct drm_viv_gem_get_tiling *args = (struct drm_viv_gem_get_tiling *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;
    gckVIDMEM_NODE nodeObj;

    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;

    args->tiling_mode   = nodeObj->tilingMode;
    args->ts_mode       = nodeObj->tsMode;
    args->ts_cache_mode = nodeObj->tsCacheMode;
    args->clear_value   = nodeObj->clearValue;

OnError:
    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);
    gcmkFOOTER_DRM();

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_attach_aux(struct drm_device *drm,
                                    void *data, struct drm_file *file)
{
    struct drm_viv_gem_attach_aux *args = (struct drm_viv_gem_attach_aux *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;
    gckVIDMEM_NODE nodeObj;
    struct drm_gem_object *gem_ts_obj = gcvNULL;

    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;
    gckDEVICE device;

    gcmkHEADER_DRM();

    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    device = gal_dev->devices[0];

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;

    /* do not support re-attach */
    if (nodeObj->tsNode)
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);

    if (args->ts_handle) {
        struct _viv_bo *viv_ts_bo;
        gckKERNEL kernel = device->kernels[0];
        gcsHAL_INTERFACE iface;
        gckVIDMEM_NODE tsNode;
        gctBOOL is128BTILE = gckHARDWARE_IsFeatureAvailable(kernel->hardware,
                                                            gcvFEATURE_128BTILE);
        gctBOOL is2BitPerTile = is128BTILE ?
                                gcvFALSE :
                                gckHARDWARE_IsFeatureAvailable(kernel->hardware,
                                                               gcvFEATURE_TILE_STATUS_2BITS);
        gctBOOL isCompressionDEC400 = gckHARDWARE_IsFeatureAvailable(kernel->hardware,
                                                                     gcvFEATURE_COMPRESSION_DEC400);
        gctPOINTER entry = gcvNULL;
        gckVIDMEM_NODE ObjNode = gcvNULL;
        gctUINT32 processID = 0;
        gctUINT32 tileStatusFiller = (isCompressionDEC400 ||
                                           ((kernel->hardware->identity.chipModel == gcv500) &&
                                            (kernel->hardware->identity.chipRevision > 2))) ?
                                          0xFFFFFFFF :
                                          is2BitPerTile ? 0x55555555 : 0x11111111;

        gem_ts_obj = drm_gem_object_lookup(file, args->ts_handle);
        if (!gem_ts_obj)
            gcmkONERROR(gcvSTATUS_NOT_FOUND);

        viv_ts_bo = container_of(gem_ts_obj, struct _viv_bo, tbo.base);
        tsNode = (gckVIDMEM_NODE)viv_ts_bo->tbo.mem.mm_node;

        gcmkONERROR(gckVIDMEM_NODE_Reference(kernel, tsNode));
        nodeObj->tsNode = tsNode;

        /* Fill tile status node with tileStatusFiller value first time to avoid GPU hang. */
        /* Lock tile status node. */
        gckOS_ZeroMemory(&iface, sizeof(iface));
        iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
        iface.hardwareType = device->defaultHwType;
        iface.u.LockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_LOCK |
                                       gcvLOCK_VIDEO_MEMORY_OP_MAP;
        iface.u.LockVideoMemory.node = viv_ts_bo->nodeHandle;
        /*TODO shuai add cacheable to viv_bo*/
        //iface.u.LockVideoMemory.cacheable = gcvTRUE;
        iface.u.LockVideoMemory.cacheable = viv_ts_bo->flags;
        gcmkONERROR(gckDEVICE_Dispatch(device, &iface));

        gcmkONERROR(gckOS_GetProcessID(&processID));
        gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID, viv_ts_bo->nodeHandle, &ObjNode));
        gcmkONERROR(gckVIDMEM_NODE_LockCPU(kernel, ObjNode, gcvFALSE, gcvFALSE, &entry));

        /* Fill tile status node with tileStatusFiller. */
        memset(entry, tileStatusFiller, (__u64)gem_ts_obj->size);
        gcmkONERROR(gckVIDMEM_NODE_UnlockCPU(kernel, ObjNode, 0, gcvFALSE, gcvFALSE));

        /* UnLock tile status node. */
        memset(&iface, 0, sizeof(iface));
        iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
        iface.hardwareType = device->defaultHwType;
        iface.u.UnlockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_UNLOCK |
                                       gcvLOCK_VIDEO_MEMORY_OP_UNMAP;
        iface.u.UnlockVideoMemory.node = (gctUINT64)viv_ts_bo->nodeHandle;
        iface.u.UnlockVideoMemory.type = gcvSURF_TYPE_UNKNOWN;
        gcmkONERROR(gckDEVICE_Dispatch(device, &iface));

        memset(&iface, 0, sizeof(iface));
        iface.command = gcvHAL_BOTTOM_HALF_UNLOCK_VIDEO_MEMORY;
        iface.hardwareType = device->defaultHwType;
        iface.u.BottomHalfUnlockVideoMemory.node = (gctUINT64)viv_ts_bo->nodeHandle;
        iface.u.BottomHalfUnlockVideoMemory.type = gcvSURF_TYPE_UNKNOWN;
        gcmkONERROR(gckDEVICE_Dispatch(device, &iface));
    }

OnError:
    if (gem_obj) {
        drm_gem_object_unreference_unlocked(gem_obj);
    gcmkFOOTER_DRM();

        if (gem_ts_obj)
            drm_gem_object_unreference_unlocked(gem_ts_obj);
    }
    gcmkFOOTER_DRM();

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

static int viv_ioctl_gem_ref_node(struct drm_device *drm,
                                  void *data, struct drm_file *file)
{
    struct drm_viv_gem_ref_node *args = (struct drm_viv_gem_ref_node *)data;
    struct drm_gem_object *gem_obj = gcvNULL;
    struct _viv_bo *bo;

    gceSTATUS status  = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;
    gckDEVICE device;
    gckKERNEL kernel = gcvNULL;
    gctUINT32 processID;
    gckVIDMEM_NODE nodeObj;
    gceDATABASE_TYPE type;
    gctUINT32 nodeHandle = 0, tsNodeHandle = 0;
    gctBOOL referred = gcvFALSE;
    gctBOOL isContiguous = gcvFALSE;
    int ret = 0;

    gcmkHEADER_DRM();

    gcmkPRINT("ref node=======");
    gal_dev = (gckGALDEVICE)drm->dev_private;
    if (!gal_dev)
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    device = gal_dev->devices[0];

    kernel = device->kernels[0];

    gem_obj = drm_gem_object_lookup(file, args->handle);
    if (!gem_obj)
        gcmkONERROR(gcvSTATUS_NOT_FOUND);

    bo = container_of(gem_obj, struct _viv_bo, tbo.base);
    nodeObj = (gckVIDMEM_NODE)bo->tbo.mem.mm_node;

    gcmkONERROR(gckOS_GetProcessID(&processID));
    gcmkONERROR(gckVIDMEM_HANDLE_Allocate(kernel, nodeObj, &nodeHandle));

    type = gcvDB_VIDEO_MEMORY |
           (nodeObj->type << gcdDB_VIDEO_MEMORY_TYPE_SHIFT) |
           (nodeObj->pool << gcdDB_VIDEO_MEMORY_POOL_SHIFT);

    gcmkONERROR(gckVIDMEM_NODE_IsContiguous(kernel, nodeObj, &isContiguous));
    if (isContiguous)
        type |= (gcvDB_CONTIGUOUS << gcdDB_VIDEO_MEMORY_DBTYPE_SHIFT);

    gcmkONERROR(gckKERNEL_AddProcessDB(kernel, processID, type,
                                       gcmINT2PTR(nodeHandle), gcvNULL, 0));
    gcmkONERROR(gckVIDMEM_NODE_Reference(kernel, nodeObj));
    referred = gcvTRUE;
    if (nodeObj->tsNode) {
        type = gcvDB_VIDEO_MEMORY |
               (nodeObj->tsNode->type << gcdDB_VIDEO_MEMORY_TYPE_SHIFT) |
               (nodeObj->tsNode->pool << gcdDB_VIDEO_MEMORY_POOL_SHIFT);

        gcmkONERROR(gckVIDMEM_NODE_IsContiguous(kernel, nodeObj->tsNode, &isContiguous));
        if (isContiguous)
            type |= (gcvDB_CONTIGUOUS << gcdDB_VIDEO_MEMORY_DBTYPE_SHIFT);

        gcmkONERROR(gckVIDMEM_HANDLE_Allocate(kernel, nodeObj->tsNode, &tsNodeHandle));
        gcmkONERROR(gckKERNEL_AddProcessDB(kernel, processID, type,
                                           gcmINT2PTR(tsNodeHandle), gcvNULL, 0));
        gcmkONERROR(gckVIDMEM_NODE_Reference(kernel, nodeObj->tsNode));
    }
    args->node = nodeHandle;
    args->ts_node = tsNodeHandle;

OnError:
    if (gcmIS_ERROR(status) && kernel) {
        gctUINT32 processID;

        gcmkVERIFY_OK(gckOS_GetProcessID(&processID));

        if (tsNodeHandle)
            gckVIDMEM_HANDLE_Dereference(kernel, processID, tsNodeHandle);

        if (nodeHandle)
            gckVIDMEM_HANDLE_Dereference(kernel, processID, nodeHandle);

        if (referred)
            gcmkONERROR(gckVIDMEM_NODE_Dereference(kernel, nodeObj));

        args->node = 0;
        args->ts_node = 0;

        ret = -ENOTTY;
    }

    if (gem_obj)
        drm_gem_object_unreference_unlocked(gem_obj);
    gcmkFOOTER_DRM();

    return ret;
}

static const struct drm_ioctl_desc viv_ioctls[] = {
    DRM_IOCTL_DEF_DRV(VIV_GEM_CREATE,        viv_ioctl_gem_create_byTTM,     DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_LOCK,          viv_ioctl_gem_lock_byTTM,       DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_UNLOCK,        viv_ioctl_gem_unlock_byTTM,     DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_CACHE,         viv_ioctl_gem_cache,      DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_QUERY,         viv_ioctl_gem_query,      DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_TIMESTAMP,     viv_ioctl_gem_timestamp,  DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_SET_TILING,    viv_ioctl_gem_set_tiling, DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_GET_TILING,    viv_ioctl_gem_get_tiling, DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_ATTACH_AUX,    viv_ioctl_gem_attach_aux, DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_REF_NODE,      viv_ioctl_gem_ref_node,   DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_RELEASE,       viv_ioctl_gem_release_byTTM,  DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_WRAP,          viv_ioctl_gem_wrap_byTTM,  DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_MOVE_BUFFER,   viv_ioctl_gem_move_byTTM,  DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(VIV_GEM_PF,            viv_ioctl_gem_pf_byTTM,    DRM_AUTH | DRM_RENDER_ALLOW),
};

static int viv_drm_load(struct drm_device *dev, unsigned long useless)
{
    int ret = 0;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();
    ret = gckDRM_TTMInit(dev);
    gcmkFOOTER_DRM();
    return ret;
}

int viv_drm_open(struct drm_device *drm, struct drm_file *file)
{
    gctINT i, dev_index;
    gctUINT32 pid = _GetProcessID();
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gckDEVICE device;
    gceSTATUS status = gcvSTATUS_OK;
    gcmkHEADER_DRM();

    for (dev_index = 0; dev_index < gal_dev->args.devCount; dev_index++) {
        device = gal_dev->devices[dev_index];

        for (i = 0; i < gcdMAX_GPU_COUNT; ++i) {
            if (device->kernels[i])
                gcmkONERROR(gckKERNEL_AttachProcessEx(device->kernels[i], gcvTRUE, pid));
        }
    }

    file->driver_priv = gcmINT2PTR(pid);

OnError:
    gcmkFOOTER_DRM();
    return gcmIS_ERROR(status) ? -ENODEV : 0;
}

void viv_drm_postclose(struct drm_device *drm, struct drm_file *file)
{
    gctINT i, dev_index;
    gctUINT32 pid = gcmPTR2SIZE(file->driver_priv);
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gckDEVICE device;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    for (dev_index = 0; dev_index < gal_dev->args.devCount; dev_index++) {
        device = gal_dev->devices[dev_index];

        for (i = 0; i < gcdMAX_GPU_COUNT; ++i) {
            if (device->kernels[i])
                gcmkVERIFY_OK(gckKERNEL_AttachProcessEx(device->kernels[i], gcvFALSE, pid));
        }
    }
    gcmkFOOTER_DRM();
}

static const struct file_operations viv_drm_fops = {
    .owner              = THIS_MODULE,
    .open               = drm_open,
    .release            = drm_release,
    .unlocked_ioctl     = drm_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl       = drm_compat_ioctl,
#endif
    .poll               = drm_poll,
    .read               = drm_read,
    .llseek             = no_llseek,
    .mmap               = viv_ttm_mmap, /* Need? */
};

static struct drm_driver viv_drm_driver = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    .driver_features    = DRIVER_GEM | DRIVER_RENDER,
#else
    .driver_features    = DRIVER_GEM | DRIVER_PRIME | DRIVER_RENDER,
#endif
    .open               = viv_drm_open,
    .postclose          = viv_drm_postclose,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
    .gem_free_object_unlocked = viv_gem_free_object,
# else
    .gem_free_object          = viv_gem_free_object,
# endif
#endif
    .prime_handle_to_fd = drm_gem_prime_handle_to_fd,
    .prime_fd_to_handle = drm_gem_prime_fd_to_handle,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
    .gem_prime_export   = viv_gem_prime_export,
#endif
    .gem_prime_import   = viv_gem_prime_import,
    .load               = viv_drm_load,
    .ioctls             = viv_ioctls,
    .num_ioctls         = DRM_VIV_NUM_IOCTLS,
    .fops               = &viv_drm_fops,
    .name               = "vivante",
    .desc               = "vivante DRM",
    .date               = "20170808",
    .major              = 1,
    .minor              = 0,
};

int
viv_drm_probe(struct device *dev)
{
    int ret = 0;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE gal_dev = gcvNULL;
    struct drm_device *drm = gcvNULL;

    gal_dev = (gckGALDEVICE)dev_get_drvdata(dev);
    if (!gal_dev) {
        ret = -ENODEV;
        gcmkONERROR(gcvSTATUS_INVALID_OBJECT);
    }

    drm = drm_dev_alloc(&viv_drm_driver, dev);
    if (IS_ERR(drm)) {
        ret = PTR_ERR(drm);
        gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }
    drm->dev_private = (void *)gal_dev;

    ret = drm_dev_register(drm, 0);
    if (ret)
        gcmkONERROR(gcvSTATUS_GENERIC_IO);

    gal_dev->drm = (void *)drm;

OnError:
    if (gcmIS_ERROR(status)) {
        if (drm)
            drm_dev_unref(drm);
        pr_err("galcore: Failed to setup drm device.\n");
    }
    return ret;
}

int
viv_drm_remove(struct device *dev)
{
    gckGALDEVICE gal_dev = (gckGALDEVICE)dev_get_drvdata(dev);

    if (gal_dev) {
        struct drm_device *drm = (struct drm_device *)gal_dev->drm;

        drm_dev_unregister(drm);
        drm_dev_unref(drm);
    }

    gcmkFOOTER_DRM();

    return 0;
}

#endif /* gcdENABLE_TTM */
#endif /* gcdENABLE_DRM */

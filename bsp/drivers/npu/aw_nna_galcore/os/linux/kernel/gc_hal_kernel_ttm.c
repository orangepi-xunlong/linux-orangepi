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
#include "gc_hal_kernel_linux.h"
#include "gc_hal_drm.h"
#include "gc_hal_kernel_allocator.h"
#include <drm/drm_device.h>
#include "gc_hal_kernel_bo.h"

#define _GC_OBJ_ZONE gcvZONE_OS

/*
 * _SetEvictInfo - save evicted node handle
 *
 * @bo : contain the target struct pointer @evictInfo
 * @newHandle : the new node handle to evict to.
 *
 * called by *(get_node)
 */
gceSTATUS
_SetEvictInfo(gckKERNEL kernel, struct _viv_bo *bo, gctUINT32 processID, gctUINT32 newHandle)
{
    gckVIDMEM_NODE oldObject;
    gctUINT32 index = 0;
    gceHARDWARE_TYPE hwType;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    gcmkVERIFY_OK(gckKERNEL_GetHardwareType(kernel, &hwType));

    if (kernel->sharedPageTable) {
        index = (gctUINT32)hwType;
        gcmkASSERT(index < gcvHARDWARE_NUM_TYPES);
    } else {
        index = (gctUINT32)kernel->core;
        gcmkASSERT(index < gcvCORE_COUNT);
    }

    if (kernel->processPageTable)
        index = 0;

    bo->evictInfo->evictedHandle = newHandle;
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID,
                                        bo->nodeHandle,
                                        &oldObject));
    bo->evictInfo->pageTables = oldObject->node->VidMem.pageTables[index];
    bo->evictInfo->address = oldObject->node->VidMem.addresses[index];

OnError:
    gcmkFOOTER_DRM_ARG("OldHandle = 0x%x newHandle = 0x%x address = 0x%lx",
                        bo->nodeHandle, newHandle, bo->evictInfo->address);

    return status;
}

gctPOINTER
gckDRM_GetGALFromTTM(struct ttm_bo_device *bdev)
{
    return container_of(bdev, struct _gckGALDEVICE, ttm.bdev);
}

gctPOINTER
gckDRM_GetVIVBo(struct ttm_buffer_object *bo)
{
    return container_of(bo, struct _viv_bo, tbo);
}

gctPOINTER
gckDRM_GetNodeObjFromHandle(gckKERNEL kernel,
                            gctUINT32 processID,
                            gctUINT32 handle)
{
    gctUINT32 pid;
    gckVIDMEM_NODE nodeObj;

    if (processID <= 0) {
        gckOS_GetProcessID(&pid);
        if (pid <= 0) {
            gcmkPRINT("we shouldn't use default value in interrupt thread");
            BUG_ON(1);
        }
    } else {
        pid = processID;
    }

    gckVIDMEM_HANDLE_Lookup(kernel, pid, handle, &nodeObj);

    return (gctPOINTER)nodeObj;
}

static int
viv_manager_init(struct ttm_mem_type_manager *man, unsigned long psize)
{
    return 0;
}

static int
viv_manager_fini(struct ttm_mem_type_manager *man)
{
    return 0;
}

gceSTATUS
_ReleaseVideoMemoryTTM(gckKERNEL Kernel, gctUINT32 ProcessID, gctUINT32 Handle)
{
    gceSTATUS status;
    gckVIDMEM_NODE nodeObject;
    gceDATABASE_TYPE type;
    gctBOOL isContiguous;

    gcmkHEADER_ARG("Kernel=%p ProcessID=%d Handle=%d", Kernel, ProcessID, Handle);

    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(Kernel, ProcessID, Handle, &nodeObject));

    type = gcvDB_VIDEO_MEMORY | (nodeObject->type << gcdDB_VIDEO_MEMORY_TYPE_SHIFT) |
           (nodeObject->pool << gcdDB_VIDEO_MEMORY_POOL_SHIFT);

    gcmkONERROR(gckVIDMEM_NODE_IsContiguous(Kernel, nodeObject, &isContiguous));

    if (isContiguous)
        type |= (gcvDB_CONTIGUOUS << gcdDB_VIDEO_MEMORY_DBTYPE_SHIFT);

    gcmkONERROR(gckKERNEL_RemoveProcessDB(Kernel, ProcessID, type, gcmINT2PTR(Handle)));

    gcmkONERROR(gckVIDMEM_HANDLE_Dereference(Kernel, ProcessID, Handle));

    gcmkONERROR(gckVIDMEM_NODE_DereferenceEx(Kernel, nodeObject, ProcessID));

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

static void
viv_manager_del(struct ttm_mem_type_manager *man, struct ttm_mem_reg *reg)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo = container_of(reg, struct _viv_bo, tbo.mem);
    struct drm_device *drm = bo->tbo.base.dev;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm->dev_private;
    gckDEVICE device = gal_dev->devices[0];
    gctUINT32 handle;
    gctUINT32 processID;

    if (!reg->mm_node) {
        gcmkPRINT("mm_node has already release");
    }
    /*
     * evicted memory will release at move clean function.
     */
    handle = bo->nodeHandle;

    gcmkHEADER_DRM_ARG("nodeHandle = 0x%x", handle);

    processID = _GetProcessID();

    if (processID != bo->processID)
        processID = bo->processID;

    gckDRM_BOUnlink2NodeObj(gal_dev, processID, handle);

    {
        gckKERNEL kernel = device->map[device->defaultHwType].kernels[0];

        status = _ReleaseVideoMemoryTTM(kernel, processID, handle);
    }

    if (status != gcvSTATUS_OK) {
        gcmkPRINT("delete mm_node failed");
    }

    /* need free this struct? */
    if (!reg->mm_node) {
        gcmkPRINT("this node has already release\n");
    } else {
        reg->mm_node = NULL;
    }

    gcmkFOOTER_DRM();
}

static void
viv_manager_debug(struct ttm_mem_type_manager *man,
              struct drm_printer *printer)
{
}

static int
viv_visible_manager_new(struct ttm_mem_type_manager *man,
                            struct ttm_buffer_object *bo,
                            const struct ttm_place *place,
                            struct ttm_mem_reg *reg)
{
    gcsHAL_INTERFACE iface;
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(bo->bdev);
    viv_bo_t *viv_bo = gckDRM_GetVIVBo(bo);
    gckDEVICE device;
    gckKERNEL kernel;
    gctUINT32 processID;
    gckVIDMEM_NODE nodeObject;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 flags = gcvALLOC_FLAG_DMABUF_EXPORTABLE | gcvALLOC_FLAG_CONTIGUOUS;
    gctUINT64 bytes = reg->size;
    gctUINT32 pool = gcvPOOL_LOCAL_EXTERNAL;
    gcuVIDMEM_NODE_PTR node;
    gckVIDMEM memory_pool;
    int ret = 0;

    gcmkHEADER_DRM_ARG("bytes = 0x%lx", bytes);

    device = gal_dev->devices[0];
    gcmkONERROR(gckOS_ZeroMemory(&iface, sizeof(iface)));
    iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    iface.hardwareType = device->defaultHwType;
    iface.u.AllocateLinearVideoMemory.bytes = bytes;
    iface.u.AllocateLinearVideoMemory.alignment = reg->page_alignment;
    iface.u.AllocateLinearVideoMemory.type = viv_bo->type;
    iface.u.AllocateLinearVideoMemory.flag = flags;
    iface.u.AllocateLinearVideoMemory.pool = pool;
    /* need reset type flag and pool according placement*/
    gcmkONERROR(gckDEVICE_Dispatch(device, &iface));

    kernel = device->map[device->defaultHwType].kernels[0];
    gckOS_GetProcessID(&processID);
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID,
                                        iface.u.AllocateLinearVideoMemory.node,
                                        &nodeObject));

    if (viv_bo->evictInfo) {
        _SetEvictInfo(kernel, viv_bo, processID,
                      iface.u.AllocateLinearVideoMemory.node);
    } else {
        gckDRM_BOLink2NodeObj(gal_dev, processID, iface.u.AllocateLinearVideoMemory.node,
                              nodeObject, viv_bo);
    }
/*
    this func move to gckDRM_BOLink2NodeObj
    nodeObject->bo = (gctPOINTER)viv_bo;
*/
    /* update & save info from interface */
    bytes = iface.u.AllocateLinearVideoMemory.bytes;
    pool = iface.u.AllocateLinearVideoMemory.pool;

    node = nodeObject->node;
    memory_pool = node->VidMem.parent;
    if (memory_pool && memory_pool->object.type == gcvOBJ_VIDMEM) {
        reg->start = node->VidMem.offset >> PAGE_SHIFT;
    } else {
        gcmkPRINT("VIV TTM is not support this pool");
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    reg->mm_node = (gctPOINTER)nodeObject;

    gcmkFOOTER_DRM_ARG("Pool = %d Size = 0x%lx Start = 0x%lx handle = 0x%lx Obj = 0x%x",
                        pool, bytes, reg->start, viv_bo->nodeHandle, nodeObject);
    return ret;

OnError:
    if (status == gcvSTATUS_OUT_OF_MEMORY) {
        /* if there is not enough memory, do not return error code */
        reg->mm_node = gcvNULL;
        ret = 0;
    } else if (status != gcvSTATUS_OK) {
        ret = EINVAL;
    }

    gcmkFOOTER_DRM();
    return ret;
}

static int
viv_invisible_manager_new(struct ttm_mem_type_manager *man,
                            struct ttm_buffer_object *bo,
                            const struct ttm_place *place,
                            struct ttm_mem_reg *reg)
{
    gcsHAL_INTERFACE iface;
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(bo->bdev);
    gckDEVICE device;
    gckKERNEL kernel;
    gctUINT32 processID;
    gckVIDMEM_NODE nodeObject;
    gctUINT32 flags = gcvALLOC_FLAG_DMABUF_EXPORTABLE | gcvALLOC_FLAG_CONTIGUOUS;
    gctUINT64 bytes = reg->size;
    gctUINT32 pool = gcvPOOL_LOCAL_EXCLUSIVE;
    gceSTATUS status = gcvSTATUS_OK;
    int ret = 0;
    viv_bo_t *viv_bo = gckDRM_GetVIVBo(bo);
    gcuVIDMEM_NODE_PTR node;
    gckVIDMEM memory_pool;

    gcmkHEADER_DRM_ARG("bytes = 0x%lx type = 0x%x", bytes, viv_bo->type);

    device = gal_dev->devices[0];
    gcmkONERROR(gckOS_ZeroMemory(&iface, sizeof(iface)));
    iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    iface.hardwareType = device->defaultHwType;
    iface.u.AllocateLinearVideoMemory.bytes = bytes;
    iface.u.AllocateLinearVideoMemory.alignment = reg->page_alignment;
    iface.u.AllocateLinearVideoMemory.type = viv_bo->type;
    iface.u.AllocateLinearVideoMemory.flag = flags;
    iface.u.AllocateLinearVideoMemory.pool = pool;
    /* need reset type flag and pool according placement*/
    gcmkONERROR(gckDEVICE_Dispatch(device, &iface));

    kernel = device->map[device->defaultHwType].kernels[0];
    gckOS_GetProcessID(&processID);
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID,
                                        iface.u.AllocateLinearVideoMemory.node,
                                        &nodeObject));
    /* save the node handle from interface */
    bytes = iface.u.AllocateLinearVideoMemory.bytes;
    pool = iface.u.AllocateLinearVideoMemory.pool;

    if (viv_bo->evictInfo) {
        _SetEvictInfo(kernel, viv_bo, processID,
                      iface.u.AllocateLinearVideoMemory.node);
    } else {
        gckDRM_BOLink2NodeObj(gal_dev, processID, iface.u.AllocateLinearVideoMemory.node,
                              nodeObject, viv_bo);
    }
/*
    this func move to gckDRM_BOLink2NodeObj
    nodeObject->bo = (gctPOINTER)viv_bo;
*/
    node = nodeObject->node;
    memory_pool = node->VidMem.parent;
    if (memory_pool && memory_pool->object.type == gcvOBJ_VIDMEM) {
        reg->start = node->VidMem.offset >> PAGE_SHIFT;
    } else {
        gcmkPRINT("VIV TTM is not support this pool");
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    reg->mm_node = (gctPOINTER)nodeObject;
    gcmkFOOTER_DRM_ARG("Pool = %d Size = 0x%lx Start = 0x%lx nodeHandle = 0x%x Obj = 0x%x",
                        pool, bytes, reg->start, viv_bo->nodeHandle, nodeObject);
    return ret;

OnError:
    if (status == gcvSTATUS_OUT_OF_MEMORY) {
        /* if there is not enough memory, do not return error code */
        reg->mm_node = gcvNULL;
        ret = 0;
    } else if (status != gcvSTATUS_OK) {
        ret = EINVAL;
    }

    gcmkFOOTER_DRM();
    return ret;
}

static int
viv_gtt_manager_new(struct ttm_mem_type_manager *man,
                            struct ttm_buffer_object *bo,
                            const struct ttm_place *place,
                            struct ttm_mem_reg *reg)
{
    return 0;
}

const struct ttm_mem_type_manager_func viv_visible_manager = {
    .init = viv_manager_init,
    .takedown = viv_manager_fini,
    .get_node = viv_visible_manager_new,
    .put_node = viv_manager_del,
    .debug = viv_manager_debug,
};

const struct ttm_mem_type_manager_func viv_invisible_manager = {
    .init = viv_manager_init,
    .takedown = viv_manager_fini,
    .get_node = viv_invisible_manager_new,
    .put_node = viv_manager_del,
    .debug = viv_manager_debug,
};

const struct ttm_mem_type_manager_func viv_gtt_manager = {
    .init = viv_manager_init,
    .takedown = viv_manager_fini,
    .get_node = viv_gtt_manager_new,
    .put_node = viv_manager_del,
    .debug = viv_manager_debug,
};

static int
viv_ttm_init_mem_type(struct ttm_bo_device *bdev, uint32_t type,
             struct ttm_mem_type_manager *man)
{
    int ret = 0;

    gcmkHEADER_DRM_ARG("type = %d", type);

    switch (type) {
    case TTM_PL_SYSTEM:
        man->flags = TTM_MEMTYPE_FLAG_MAPPABLE;
        man->available_caching = TTM_PL_MASK_CACHING;
        man->default_caching = TTM_PL_FLAG_CACHED;
        break;
    case TTM_PL_VRAM:
        man->flags = TTM_MEMTYPE_FLAG_FIXED |
                 TTM_MEMTYPE_FLAG_MAPPABLE;
        man->available_caching = TTM_PL_FLAG_UNCACHED |
                     TTM_PL_FLAG_WC;
        man->default_caching = TTM_PL_FLAG_WC;

        man->func = &viv_visible_manager;
        man->io_reserve_fastpath = false;
        man->use_io_reserve_lru = true;
        break;
    case TTM_PL_TT:
        /* GTT memory  */
        man->func = &viv_gtt_manager;
        man->gpu_offset = 0;
        man->available_caching = TTM_PL_MASK_CACHING;
        man->default_caching = TTM_PL_FLAG_CACHED;
        man->flags = TTM_MEMTYPE_FLAG_MAPPABLE | TTM_MEMTYPE_FLAG_CMA;
        break;
    case TTM_PL_PRIV:
        man->flags = TTM_MEMTYPE_FLAG_FIXED;
        man->available_caching = TTM_PL_FLAG_UNCACHED |
                     TTM_PL_FLAG_WC;
        man->default_caching = TTM_PL_FLAG_WC;

        man->func = &viv_invisible_manager;
        man->io_reserve_fastpath = false;
        man->use_io_reserve_lru = true;
        break;
    default:
        ret = -EINVAL;
    }

    gcmkFOOTER_DRM_ARG("flags = 0x%x caching = 0x%x / 0x%x",
                        man->flags, man->available_caching, man->default_caching);
    return ret;
}

static void
backend_func_destroy(struct ttm_tt *tt)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    ttm_tt_fini(tt);
    kfree(tt);

    gcmkFOOTER_DRM();
}

static struct ttm_backend_func backend_func = {
    .destroy = backend_func_destroy
};

static struct ttm_tt *
bo_driver_ttm_tt_create(struct ttm_buffer_object *bo,
                          uint32_t page_flags)
{
    struct ttm_tt *tt;
    int ret;
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(bo->bdev);
    gckOS Os = gal_dev->os;
    gctPOINTER pointer;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM_ARG("page_flags = 0x%x", page_flags);

    gcmkONERROR(gckOS_Allocate(Os, sizeof(*tt), &pointer));
    gcmkONERROR(gckOS_ZeroMemory(pointer, sizeof(*tt)));
    tt = (struct ttm_tt *)pointer;

    tt->func = &backend_func;

    ret = ttm_tt_init(tt, bo, page_flags);
    if (ret < 0) {
        status = gcvSTATUS_OUT_OF_MEMORY;
        goto OnError;
    }

    gcmkFOOTER_DRM();
    return tt;

OnError:
    gcmkFOOTER_DRM();
    return NULL;
}

/* we will determine the place where the tbo will be evict to. */
static void bo_driver_evict_flags(struct ttm_buffer_object *tbo,
                                  struct ttm_placement *placement)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo = gckDRM_GetVIVBo(tbo);
    struct ttm_mem_reg *mem = &tbo->mem;
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(tbo->bdev);

    gcmkHEADER_DRM();

    /* we will evict one tbo in VRAM to PRIV(exclusive pool) */
    if (mem->mem_type == TTM_PL_VRAM)
        gckDRM_SetPlaceFromDomain(bo, VIV_MEM_DOMAIN_PRIV);
    else
        gcmkPRINT("evict is not support this kind of domain");

    gckOS_Allocate(gal_dev->os, sizeof(struct _viv_evict_info),
                   (gctPOINTER)&bo->evictInfo);
    gckOS_ZeroMemory((gctPOINTER)bo->evictInfo, sizeof(struct _viv_evict_info));

    *placement = bo->placement;

    gcmkFOOTER_DRM();
}

static int
bo_driver_verify_access(struct ttm_buffer_object *tbo,
                   struct file *filp)
{
    gceSTATUS status = gcvSTATUS_OK;
    int ret = 0;

    gcmkHEADER_DRM();

    ret = drm_vma_node_verify_access(&tbo->base.vma_node, filp->private_data);

    if (ret)
        gcmkONERROR(gcvSTATUS_INVALID_ADDRESS);

OnError:
    gcmkFOOTER_DRM();
    return ret;
}

static int
bo_driver_io_mem_reserve(struct ttm_bo_device *bdev,
                    struct ttm_mem_reg *mem)
{
    struct ttm_mem_type_manager *man = bdev->man + mem->mem_type;
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(bdev);
    gceSTATUS status = gcvSTATUS_OK;
    int ret = 0;

    gcmkHEADER_DRM_ARG("mem_type = %d is_iomem = %d man->flag = 0x%x",
                        mem->mem_type, mem->bus.is_iomem, man->flags);

    if (!(man->flags & TTM_MEMTYPE_FLAG_MAPPABLE)) {
        status = gcvSTATUS_INVALID_ARGUMENT;
        ret = -EINVAL;
        goto OnError;
    }

    mem->bus.addr = NULL;
    mem->bus.size = mem->num_pages << PAGE_SHIFT;

    switch (mem->mem_type) {
    case TTM_PL_SYSTEM: /* nothing to do */
        mem->bus.offset = 0;
        mem->bus.base = 0;
        mem->bus.is_iomem = false;
        break;
    case TTM_PL_VRAM:
        mem->bus.offset = mem->start << PAGE_SHIFT;
        mem->bus.base = gal_dev->args.externalBase[0];
        mem->bus.is_iomem = true;
        break;
    default:
        status = gcvSTATUS_INVALID_ARGUMENT;
        ret = -EINVAL;
        goto OnError;
    }

OnError:
    gcmkFOOTER_DRM();
    return ret;
}

static void
bo_driver_io_mem_free(struct ttm_bo_device *bdev,
                  struct ttm_mem_reg *mem)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    gcmkFOOTER_DRM();
}

static int
viv_ttm_populate(struct ttm_tt *ttm, struct ttm_operation_ctx *ctx)
{
    gceSTATUS status = gcvSTATUS_OK;
    int ret = 0;

    gcmkHEADER_DRM();

    ret = ttm_pool_populate(ttm, ctx);

    if (ret)
        status = gcvSTATUS_OUT_OF_RESOURCES;

    gcmkFOOTER_DRM();
    return ret;
}

static int
viv_bo_move(struct ttm_buffer_object *tbo,
                       bool evict,
                       struct ttm_operation_ctx *ctx,
                       struct ttm_mem_reg *new_reg)
{
    struct ttm_mem_reg *old_reg = &tbo->mem;
    int ret = 0;
    gceSTATUS status = gcvSTATUS_OK;
    gcsDMA_TRANS_INFO info;
    gckGALDEVICE gal_dev = gckDRM_GetGALFromTTM(tbo->bdev);
    struct _viv_bo *bo = gckDRM_GetVIVBo(tbo);

    gcmkHEADER_DRM_ARG("old mem_type = %d, new mem_type = %d",
                        old_reg->mem_type, new_reg->mem_type);

    ret = ttm_bo_wait(tbo, ctx->interruptible, ctx->no_wait_gpu);
    if (ret) {
        status = gcvSTATUS_TIMEOUT;
        goto OnError;
    }

    /* Fake move for the bo first create */
    if (old_reg->mem_type == TTM_PL_SYSTEM && !tbo->ttm) {
        tbo->mem = *new_reg;
        new_reg->mm_node = gcvNULL;

        gcmkFOOTER_DRM();
        return ret;
    }

    /* set the nodeObject */
    info.src_node = old_reg->mm_node;
    info.dst_node = new_reg->mm_node;
    info.offset = 0;

    /* check if the node size is not equal */
    if (old_reg->size == new_reg->size) {
        info.bytes = old_reg->size;
    } else {
        gcmkPRINT("WARNING! old reg size is not equal new's");
        BUG_ON(1);
    }

    /* set move direction */
    if (old_reg->mem_type == TTM_PL_VRAM && new_reg->mem_type == TTM_PL_PRIV)
        info.reason = gcvSYNC_MEMORY_DIRECTION_SYSTEM_TO_LOCAL;
    else if (old_reg->mem_type == TTM_PL_PRIV && new_reg->mem_type == TTM_PL_VRAM)
        info.reason = gcvSYNC_MEMORY_DIRECTION_LOCAL_TO_SYSTEM;
    else
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

    /* move */
    gcmkONERROR(gckDRM_BOMove(gal_dev, &info));
    gcmkONERROR(gckDRM_BOMoveClean(gal_dev, bo, new_reg));

    /* release the old memory */
    //ttm_bo_mem_put(tbo, &tbo->mem);
    ttm_flag_masked(&old_reg->placement, TTM_PL_FLAG_SYSTEM,
                                         TTM_PL_MASK_MEM);
    old_reg->mem_type = TTM_PL_SYSTEM;

    *old_reg = *new_reg;
    new_reg->mm_node = gcvNULL;

OnError:
    if (status != gcvSTATUS_OK)
        ret = -EINVAL;
    gcmkFOOTER_DRM();
    return ret;

}

static int
viv_ttm_fault_reserve_notify(struct ttm_buffer_object *tbo)
{
    gctUINT32 ret = 0;
    struct ttm_placement *placement;
    struct ttm_mem_reg *mem = &tbo->mem;
    int i = 0;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    if (ret) {
        gcmkPRINT("Error happen in viv_ttm_fault_reserve_notify, err node %d\n", ret);
        status = gcvSTATUS_OUT_OF_RESOURCES;

#if gcdENABLE_DRM_DEBUG
        gcmkPRINT("Target placement:");
        for (i = 0; i < placement->num_placement; i++) {
            const struct ttm_place *place = &placement->placement[i];
            gcmkPRINT("place[%d]: fpfn = 0x%x lpfn = 0x%x flag = 0x%x\n",
                        i, place->fpfn, place->lpfn, place->flags);
        }
        gcmkPRINT("Present Bo Place:");
        gcmkPRINT("start = 0x%x pages = 0x%x placement = 0x%x\n",
                    mem->start, mem->num_pages, mem->placement);
#endif
        gcmkFOOTER_DRM();
        return ret;
    }

    //maybe need add some dma sync code.
    gcmkFOOTER_DRM();
    return 0;
}

bool viv_bo_eviction_valuable(struct ttm_buffer_object *tbo,
                              const struct ttm_place *place)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    /* Don't evict this BO if it's outside of the
     * requested placement range
     */
    if (place->fpfn >= (tbo->mem.start + tbo->mem.size) ||
        (place->lpfn && place->lpfn <= tbo->mem.start)) {
            return false;
            gcmkFOOTER_DRM();
        }

    /* check if the bo is locked by user */
    gcmkFOOTER_DRM();
    return true;
}


static struct ttm_bo_driver viv_ttm_driver = {
    .ttm_tt_create = bo_driver_ttm_tt_create,
    .ttm_tt_populate = viv_ttm_populate,
    //.ttm_tt_unpopulate = ttm_pool_unpopulate,
    .move = viv_bo_move,
    .init_mem_type = viv_ttm_init_mem_type,
    .eviction_valuable = viv_bo_eviction_valuable,
    .evict_flags = bo_driver_evict_flags,
    .verify_access = bo_driver_verify_access,
    .fault_reserve_notify = viv_ttm_fault_reserve_notify,
    .io_mem_reserve = bo_driver_io_mem_reserve,
    .io_mem_free = bo_driver_io_mem_free,
};

extern gceSTATUS
gckDRM_TTMInit(struct drm_device *drm_dev)
{
    gceSTATUS status = gcvSTATUS_OK;
    int ret;
    gckGALDEVICE gal_dev = (gckGALDEVICE)drm_dev->dev_private;
    viv_ttm_t *viv_ttm = &gal_dev->ttm;

    gcmkHEADER_DRM();

    ret = ttm_bo_device_init(&viv_ttm->bdev, &viv_ttm_driver,
                             drm_dev->anon_inode->i_mapping,
                             gcvTRUE);

    if (ret) {
        gcmkPRINT("Create TTM device failed, error code -%d\n", ret);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        goto OnError;
    } else {
        gcmkPRINT("Create TTM device succeed!\n");
    }

    gckOS_AtomConstruct(gal_dev->os, &viv_ttm->vram_pin_size);
    gckOS_AtomConstruct(gal_dev->os, &viv_ttm->visible_pin_size);

    /* construct vram-visible domain */
    ret = ttm_bo_init_mm(&viv_ttm->bdev, TTM_PL_VRAM,
                         gal_dev->args.externalSize[0] >> PAGE_SHIFT);

    if (ret) {
        gcmkPRINT("failed to init VRAM-visible domain, error code -%d", ret);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        return status;
    } else {
        gcmkPRINT("Init VRAM-visible domain, size = %x", gal_dev->args.externalSize[0]);
    }

    /* construct vram-invisible domain */
    ret = ttm_bo_init_mm(&viv_ttm->bdev, TTM_PL_PRIV,
                         gal_dev->args.exclusiveSize[0] >> PAGE_SHIFT);

    if (ret) {
        gcmkPRINT("failed to init VRAM-invisible domain, error code -%d", ret);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        return status;
    } else {
        gcmkPRINT("Init VRAM-invisible domain, size = %lx", gal_dev->args.exclusiveSize[0]);
    }

    ret = ttm_bo_init_mm(&viv_ttm->bdev, TTM_PL_TT,
                         gal_dev->args.exclusiveSize[0] >> PAGE_SHIFT);

OnError:
    gcmkFOOTER_DRM();
    return status;
}
#endif /* gcdENABLE_TTM */

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
#include "gc_hal_base.h"
#include "shared/gc_hal_enum_shared.h"
#include "gc_hal_kernel_platform.h"
#include "gc_hal_kernel_allocator.h"
#include "gc_hal_kernel_bo.h"

#define _GC_OBJ_ZONE gcvZONE_OS

#define WATER_LINE 64*1024*1024

bool ttm_mem_reg_is_pci(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem)
{
    struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];

    if (!(man->flags & TTM_MEMTYPE_FLAG_FIXED)) {
        if (mem->mem_type == TTM_PL_SYSTEM)
            return false;

        if (man->flags & TTM_MEMTYPE_FLAG_CMA)
            return false;

        if (mem->placement & TTM_PL_FLAG_CACHED)
            return false;
    }
    return true;
}

static void ttm_tt_clear_mapping(struct ttm_tt *ttm)
{
    pgoff_t i;
    struct page **page = ttm->pages;

    if (ttm->page_flags & TTM_PAGE_FLAG_SG)
        return;

    for (i = 0; i < ttm->num_pages; ++i) {
        (*page)->mapping = NULL;
        (*page++)->index = 0;
    }
}

void ttm_tt_unbind(struct ttm_tt *ttm)
{
    int ret;

    if (ttm->state == tt_bound) {
        ret = ttm->func->unbind(ttm);
        BUG_ON(ret);
        ttm->state = tt_unbound;
    }
}

void ttm_tt_unpopulate(struct ttm_tt *ttm)
{
    if (ttm->state == tt_unpopulated)
        return;

    ttm_tt_clear_mapping(ttm);
    if (ttm->bdev->driver->ttm_tt_unpopulate)
        ttm->bdev->driver->ttm_tt_unpopulate(ttm);
    else
        ttm_pool_unpopulate(ttm);
}


void ttm_mem_io_free_vm(struct ttm_buffer_object *bo)
{
    struct ttm_mem_reg *mem = &bo->mem;

    if (mem->bus.io_reserved_vm) {
        mem->bus.io_reserved_vm = false;
        list_del_init(&bo->io_reserve_lru);
        ttm_mem_io_free(bo->bdev, mem);
    }
}

int ttm_tt_create(struct ttm_buffer_object *bo, bool zero_alloc)
{
    struct ttm_bo_device *bdev = bo->bdev;
    uint32_t page_flags = 0;

    dma_resv_assert_held(bo->base.resv);

    if (bdev->need_dma32)
        page_flags |= TTM_PAGE_FLAG_DMA32;

    if (bdev->no_retry)
        page_flags |= TTM_PAGE_FLAG_NO_RETRY;

    switch (bo->type) {
    case ttm_bo_type_device:
        if (zero_alloc)
            page_flags |= TTM_PAGE_FLAG_ZERO_ALLOC;
        break;
    case ttm_bo_type_kernel:
        break;
    case ttm_bo_type_sg:
        page_flags |= TTM_PAGE_FLAG_SG;
        break;
    default:
        bo->ttm = NULL;
        pr_err("Illegal buffer object type\n");
        return -EINVAL;
    }

    bo->ttm = bdev->driver->ttm_tt_create(bo, page_flags);
    if (unlikely(bo->ttm == NULL))
        return -ENOMEM;

    return 0;
}

void ttm_bo_unmap_virtual_locked(struct ttm_buffer_object *bo)
{
    struct ttm_bo_device *bdev = bo->bdev;

    drm_vma_node_unmap(&bo->base.vma_node, bdev->dev_mapping);
    ttm_mem_io_free_vm(bo);
}

void ttm_tt_destroy(struct ttm_tt *ttm)
{
    if (ttm == NULL)
        return;

    ttm_tt_unbind(ttm);

    if (ttm->state == tt_unbound)
        ttm_tt_unpopulate(ttm);

    if (!(ttm->page_flags & TTM_PAGE_FLAG_PERSISTENT_SWAP) &&
        ttm->swap_storage)
        fput(ttm->swap_storage);

    ttm->swap_storage = NULL;
    ttm->func->destroy(ttm);
}

gceSTATUS
_SetPlaceFromDomain(struct _viv_bo *bo, gctUINT32 domain)
{
    gceSTATUS status = gcvSTATUS_OK;

    struct ttm_placement *placement = &bo->placement;
    struct ttm_place *places = bo->placements;
    gctUINT64 flags = bo->flags;
    gctUINT32 cnt = 0;
    gckGALDEVICE gal_dev = (gckGALDEVICE)gckDRM_GetGALFromTTM(bo->tbo.bdev);
    gctUINT64 visible_vram_size = gal_dev->args.externalSize[0];

    gcmkHEADER_DRM_ARG("BO = %p, domain = %d", bo, domain);
    if (domain & VIV_MEM_DOMAIN_VRAM) {
        gctUINT32 visible_pfn = visible_vram_size >> PAGE_SHIFT;
        gckVIDMEM videoMemory;

        places[cnt].fpfn = 0;
        places[cnt].lpfn = 0;
        places[cnt].flags = TTM_PL_MASK_CACHING |
                            TTM_PL_FLAG_VRAM;

        if (flags & VIV_MEM_CREATE_CPU_ACCESS_REQUIRED)
            places[cnt].lpfn = visible_pfn;
        else
            places[cnt].fpfn |= TTM_PL_FLAG_TOPDOWN;

        if (flags & VIV_MEM_CREATE_CONTIGUOUS_VRAM)
            places[cnt].flags |= TTM_PL_FLAG_CONTIGUOUS;

        cnt++;

        /* SHUAI TODO: add water line check to make sure whether we need move*/
        /* just set water line as 0, (create -> cpu w/r -> move) */
        gckKERNEL_GetVideoMemoryPool(gal_dev->devices[0]->kernels[0], gcvPOOL_LOCAL_EXTERNAL,
                                     &videoMemory);

        if (videoMemory->freeBytes < WATER_LINE)
            bo->flags |= VIV_MEM_CREATE_WAITING_MOVE;
    }

    if (domain & VIV_MEM_DOMAIN_PRIV) {
        gctUINT32 pfn = gal_dev->args.exclusiveSize[0] >> PAGE_SHIFT;

        places[cnt].fpfn = 0;
        places[cnt].lpfn = pfn;
        places[cnt].flags = TTM_PL_MASK_CACHING |
                            TTM_PL_FLAG_PRIV;

        cnt++;
    }

    /*TODO: add other memory domain*/

    placement->num_placement = cnt;
    placement->placement = places;
    placement->num_busy_placement = cnt;
    placement->busy_placement = places;

    gcmkFOOTER_DRM_ARG("num_placement = %d, num_busy_placement = %d",
                        placement->num_placement, placement->num_busy_placement);
    return status;
}

gceSTATUS
_CreateBufferObj(gckGALDEVICE dev,
                 struct viv_bo_param *bp,
                 struct _viv_bo **bo_ptr)
{
    gceSTATUS status = gcvSTATUS_OK;
    viv_ttm_t *viv_mm = &dev->ttm;
    struct drm_device *drm_dev = dev->drm;
    struct _viv_bo *bo;
    gctUINT64 page_align, size = bp->size;
    gctUINT64 acc_size;
    gctINT32 ret;
    gctPOINTER pointer;
    gckOS Os = dev->os;

    gcmkHEADER_DRM_ARG("size = %lx byte_align = %x domain = %x flags = %x type = %d",
                        bp->size, bp->byte_align, bp->domain, bp->flags, bp->type);

    page_align = gcmALIGN(bp->byte_align, PAGE_SIZE) >> PAGE_SHIFT;
    size = gcmALIGN(size, PAGE_SIZE);
    *bo_ptr = gcvNULL;

    acc_size = ttm_bo_dma_acc_size(&viv_mm->bdev, size, sizeof(struct _viv_bo));

    gcmkONERROR(gckOS_Allocate(Os, sizeof(struct _viv_bo), &pointer));
    gcmkONERROR(gckOS_ZeroMemory(pointer, sizeof(struct _viv_bo)));
    bo = (struct _viv_bo *)pointer;

    drm_gem_object_init(drm_dev, &bo->tbo.base, size);

    bo->preferred_domains = bp->preferred_domain ? bp->preferred_domain :
                            bp->domain;
    bo->allowed_domains = bo->preferred_domains;
    bo->flags = bp->flags;
    bo->tbo.bdev = &viv_mm->bdev;
    bo->os = Os;
    bo->type = bp->vidMemType;
    //bo->tbo.priority = 1;
    gcmkONERROR(_SetPlaceFromDomain(bo, bp->domain));

    ret = ttm_bo_init(&viv_mm->bdev,
                    &bo->tbo,
                    size,
                    bp->type,
                    &bo->placement,
                    page_align >> PAGE_SHIFT,
                    gcvFALSE,
                    acc_size,
                    gcvNULL,
                    bp->resv,
                    &gckDRM_BODestroy);
    if (ret) {
        gcmkPRINT("Create TTM Bo failed, error code: -%d\n", ret);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        goto OnError;
    }

    if (!bp->resv)
        gckDRM_BOUnreserve(bo);

    *bo_ptr = bo;

OnError:
    /* TTM will help to release related resource, we could only foucs
     * on VIV resource.
     */

    gcmkFOOTER_DRM();
    return status;

}

void gckDRM_SetPlaceFromDomain(struct _viv_bo *bo, gctUINT32 domain)
{
    gceSTATUS status;

    gcmkHEADER_DRM();
    status = _SetPlaceFromDomain(bo, domain);
    gcmkFOOTER_DRM();
}

void
gckDRM_BOUnreserve(struct _viv_bo *bo)
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmkHEADER_DRM();
    ttm_bo_unreserve(&bo->tbo);
    gcmkFOOTER_DRM();
}

gceSTATUS
gckDRM_BOReserve(struct _viv_bo *bo, gctBOOL no_irq)
{
    gctINT32 ret = 0;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM_ARG("no_irq = %d", no_irq);

    ret = __ttm_bo_reserve(&bo->tbo, !no_irq, gcvFALSE, gcvNULL);

    if (unlikely(ret != 0)) {
        gcmkPRINT("gckDRM_BOReserve failed, error code -%d", ret);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        goto OnError;
    }

OnError:
    gcmkFOOTER_DRM();
    return status;
}

void
gckDRM_BODestroy(struct ttm_buffer_object *tbo)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *bo = container_of(tbo, struct _viv_bo, tbo);

    gcmkHEADER_DRM_ARG("tbo = %p", tbo);

    if (bo->tbo.base.import_attach)
        drm_prime_gem_destroy(&bo->tbo.base, bo->tbo.sg);

    drm_gem_object_release(&bo->tbo.base);

    gcmkOS_SAFE_FREE(bo->os, bo);

    gcmkFOOTER_DRM();
}

gceSTATUS
gckDRM_BOCreate(gckGALDEVICE dev,
                struct viv_bo_param *bp,
                struct _viv_bo **bo_ptr)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    gcmkONERROR(_CreateBufferObj(dev, bp, bo_ptr));

OnError:
    gcmkFOOTER_DRM();
    return status;
}

/*
    This function is for viv_bo to link to nodeObject,
    due to viv_bo is per-process, so if we want to get viv_bo from
    nodeHandle or nodeObject, we should check the process and the handle
    is matched.
*/
gceSTATUS
gckDRM_BOLink2NodeObj(gckGALDEVICE gal_dev, gctUINT32 processID, gctUINT32 nodeHandle,
                      gckVIDMEM_NODE nodeObject, struct _viv_bo *bo)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *temp_bo;
    gcmkHEADER_DRM();

    bo->nodeHandle = nodeHandle;
    bo->processID = processID;
    bo->next = gcvNULL;

    gckOS_AcquireMutex(gal_dev->os, nodeObject->mutex, gcvINFINITE);
    temp_bo = (struct _viv_bo *)nodeObject->bo;

    if (!temp_bo) {
        nodeObject->bo = (gctPOINTER)bo;
    } else {
        while (gcvTRUE) {
            if (!temp_bo->next) {
                temp_bo->next = (gctPOINTER)bo;
                break;
            } else {
                temp_bo = (struct _viv_bo *)temp_bo->next;
            }
        }
    }
    gckOS_ReleaseMutex(gal_dev->os, nodeObject->mutex);

    gcmkFOOTER_DRM();
    return status;
}

gceSTATUS
gckDRM_BOUnlink2NodeObj(gckGALDEVICE gal_dev, gctUINT32 processID, gctUINT32 nodeHandle)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct _viv_bo *temp_bo;
    gckDEVICE device = gal_dev->devices[0];
    gckKERNEL kernel = device->kernels[0];
    gckVIDMEM_NODE nodeObject;
    struct _viv_bo *pre_bo;

    gcmkHEADER_DRM();

    status = gckVIDMEM_HANDLE_Lookup(kernel, processID, nodeHandle, &nodeObject);

    if (status != gcvSTATUS_OK)
        goto OnError;

    gckOS_AcquireMutex(gal_dev->os, nodeObject->mutex, gcvINFINITE);
    temp_bo = (struct _viv_bo *)nodeObject->bo;
    if (temp_bo->nodeHandle == nodeHandle && temp_bo->processID == processID) {
        nodeObject->bo = temp_bo->next;
    } else {
        pre_bo = temp_bo;
        temp_bo = (struct _viv_bo *)temp_bo->next;
        while (gcvTRUE) {
            if (!temp_bo) {
                gcmkPRINT("not found");
            }
            if (temp_bo->nodeHandle == nodeHandle && temp_bo->processID == processID) {
                pre_bo->next = temp_bo->next;
                break;
            } else {
                pre_bo = temp_bo;
                temp_bo = (struct _viv_bo *)temp_bo->next;
            }
        }
    }

    gckOS_ReleaseMutex(gal_dev->os, nodeObject->mutex);

OnError:
    gcmkFOOTER_DRM();
    return status;
}

gceSTATUS
gckDRM_BOGetFromNodeHandle(gckGALDEVICE gal_dev, gctUINT32 processID,
                           gctUINT32 nodeHandle,
                           struct _viv_bo **bo)
{
    gceSTATUS status = gcvSTATUS_OK;
    gckDEVICE device = gal_dev->devices[0];
    gckKERNEL kernel = device->kernels[0];
    gckVIDMEM_NODE nodeObject;
    struct _viv_bo *temp_bo;
    //gctUINT32 i = 0;
    gcmkHEADER_DRM();

    gckVIDMEM_HANDLE_Lookup(kernel, processID, nodeHandle, &nodeObject);
    gckOS_AcquireMutex(gal_dev->os, nodeObject->mutex, gcvINFINITE);

    temp_bo = (struct _viv_bo *)nodeObject->bo;
    while (gcvTRUE) {
        if (!temp_bo) {
            gcmkPRINT("no viv_bo in this nodeObject");
            status = gcvSTATUS_INVALID_DATA;
            *bo = gcvNULL;
            break;
        } else {
            if (temp_bo->nodeHandle == nodeHandle &&
                temp_bo->processID == processID) {
                /* get the correct bo */
                *bo = temp_bo;
                break;
            } else {
                temp_bo = (struct _viv_bo *)temp_bo->next;
            }
        }
    }
    gckOS_ReleaseMutex(gal_dev->os, nodeObject->mutex);

    gcmkFOOTER_DRM();
    return status;

}

static int ttm_bo_handle_move_mem(struct ttm_buffer_object *bo,
                  struct ttm_mem_reg *mem, bool evict,
                  struct ttm_operation_ctx *ctx)
{
    struct ttm_bo_device *bdev = bo->bdev;
    bool old_is_pci = ttm_mem_reg_is_pci(bdev, &bo->mem);
    bool new_is_pci = ttm_mem_reg_is_pci(bdev, mem);
    struct ttm_mem_type_manager *old_man = &bdev->man[bo->mem.mem_type];
    struct ttm_mem_type_manager *new_man = &bdev->man[mem->mem_type];
    int ret = 0;

    if (old_is_pci || new_is_pci ||
        ((mem->placement & bo->mem.placement & TTM_PL_MASK_CACHING) == 0)) {
        ret = ttm_mem_io_lock(old_man, true);
        if (unlikely(ret != 0))
            goto out_err;
        ttm_bo_unmap_virtual_locked(bo);
        ttm_mem_io_unlock(old_man);
    }

    /*
     * Create and bind a ttm if required.
     */

    if (!(new_man->flags & TTM_MEMTYPE_FLAG_FIXED)) {
        if (bo->ttm == NULL) {
            bool zero = !(old_man->flags & TTM_MEMTYPE_FLAG_FIXED);
            ret = ttm_tt_create(bo, zero);
            if (ret)
                goto out_err;
        }

        ret = ttm_tt_set_placement_caching(bo->ttm, mem->placement);
        if (ret)
            goto out_err;

        if (mem->mem_type != TTM_PL_SYSTEM) {
            ret = ttm_tt_bind(bo->ttm, mem, ctx);
            if (ret)
                goto out_err;
       }

        if (bo->mem.mem_type == TTM_PL_SYSTEM) {
            if (bdev->driver->move_notify)
                bdev->driver->move_notify(bo, evict, mem);
            bo->mem = *mem;
            mem->mm_node = NULL;
            goto moved;
      }
    }

    if (bdev->driver->move_notify)
        bdev->driver->move_notify(bo, evict, mem);

    if (!(old_man->flags & TTM_MEMTYPE_FLAG_FIXED) &&
        !(new_man->flags & TTM_MEMTYPE_FLAG_FIXED)) {
        ret = ttm_bo_move_ttm(bo, ctx, mem);
    } else if (bdev->driver->move) {
        ret = bdev->driver->move(bo, evict, ctx, mem);
    } else {
        ret = ttm_bo_move_memcpy(bo, ctx, mem);
    }

    if (ret) {
        if (bdev->driver->move_notify) {
            swap(*mem, bo->mem);
            bdev->driver->move_notify(bo, false, mem);
            swap(*mem, bo->mem);
        }

        goto out_err;
    }

moved:
    if (bo->evicted) {
        if (bdev->driver->invalidate_caches) {
            ret = bdev->driver->invalidate_caches(bdev, bo->mem.placement);
            if (ret)
                pr_err("Can not flush read caches\n");
        }
       bo->evicted = false;
    }

    if (bo->mem.mm_node)
        bo->offset = (bo->mem.start << PAGE_SHIFT) +
            bdev->man[bo->mem.mem_type].gpu_offset;
    else
        bo->offset = 0;

    ctx->bytes_moved += bo->num_pages << PAGE_SHIFT;
    return 0;

out_err:
    new_man = &bdev->man[bo->mem.mem_type];
    if (new_man->flags & TTM_MEMTYPE_FLAG_FIXED) {
        ttm_tt_destroy(bo->ttm);
        bo->ttm = NULL;
    }

    return ret;
}

static int ttm_bo_move_buffer(struct ttm_buffer_object *bo,
                  struct ttm_placement *placement,
                  struct ttm_operation_ctx *ctx)
{
    int ret = 0;
    struct ttm_mem_reg mem;

    dma_resv_assert_held(bo->base.resv);

    mem.num_pages = bo->num_pages;
    mem.size = mem.num_pages << PAGE_SHIFT;
    mem.page_alignment = bo->mem.page_alignment;
    mem.bus.io_reserved_vm = false;
    mem.bus.io_reserved_count = 0;
    /*
     * Determine where to move the buffer.
     */
    ret = ttm_bo_mem_space(bo, placement, &mem, ctx);
    if (ret) {
        WARN_ON(1);
        goto out_unlock;
    }
    ret = ttm_bo_handle_move_mem(bo, &mem, false, ctx);
        if (ret) {
        WARN_ON(1);
        goto out_unlock;
    }
out_unlock:
    if (ret && mem.mm_node)
        ttm_bo_mem_put(bo, &mem);
    return ret;
}

gceSTATUS
gckDRM_BOValidate(gckGALDEVICE dev, struct _viv_bo *bo, gctUINT32 domain)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct ttm_buffer_object *tbo = &bo->tbo;
    gctUINT32 ret = 0;
    struct ttm_placement *placement;
    struct ttm_mem_reg *mem = &tbo->mem;
    gctUINT32 i = 0;
    struct ttm_operation_ctx ctx = { gcvFALSE, gcvFALSE };
    gctUINT32 new_flags;

    gcmkHEADER_DRM();

    gckDRM_SetPlaceFromDomain(bo, domain);
    placement = &bo->placement;

    gckOS_Allocate(dev->os, sizeof(struct _viv_evict_info),
                   (gctPOINTER)&bo->evictInfo);
    gckOS_ZeroMemory((gctPOINTER)bo->evictInfo, sizeof(struct _viv_evict_info));

//    ret = ttm_bo_validate(tbo, &bo->placement, &ctx);
        if (!ttm_bo_mem_compat(placement, &tbo->mem, &new_flags)) {
            ret = ttm_bo_move_buffer(tbo, placement, &ctx);
            if (ret)
                return ret;
        }

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

    gcmkFOOTER_DRM();
    return 0;

}

gceSTATUS
gckDRM_BOMove(gckGALDEVICE dev,
                gcsDMA_TRANS_INFO *info)
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsPLATFORM *platform = dev->platform;

    gcmkHEADER_DRM_ARG("bytes = 0x%x offset = 0x%x reason = %d",
                        info->bytes, info->offset, info->reason);

    if (platform && platform->ops->dmaCopy) {
        status = platform->ops->dmaCopy(dev->devices[0]->kernels[0], info);
    } else {
        gcmkPRINT("Some problem happened in platform or dmaCopy operation");
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

OnError:
    gcmkFOOTER_DRM();
    return status;
}

gceSTATUS
gckDRM_BOMoveClean(gckGALDEVICE gal_dev, struct _viv_bo *bo, struct ttm_mem_reg *new_reg)
{
    gckDEVICE device = gal_dev->devices[0];
    gckKERNEL kernel = device->kernels[0];
    gceSTATUS status = gcvSTATUS_OK;
    gckVIDMEM_NODE newObject, oldObject;
    gctUINT32 newHandle = 1;
    gctUINT32 oldHandle = bo->nodeHandle;
    gctUINT32 processID = 0;
    gckMMU mmu;
    gctPOINTER memoryHandle;
    gctSIZE_T offset;

    memoryHandle = (gctPOINTER)bo->evictInfo->pageTables;
    newHandle = bo->evictInfo->evictedHandle;
    gcmkHEADER_DRM_ARG("newHandle = 0x%x oldHandle = 0x%x", newHandle, oldHandle);

    gckOS_GetProcessID(&processID);
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID, oldHandle, &oldObject));
    gcmkONERROR(gckVIDMEM_HANDLE_Lookup(kernel, processID, newHandle, &newObject));
    gcmkONERROR(gckKERNEL_GetCurrentMMU(kernel, gcvTRUE, processID, &mmu));

/* due to much of function will use the nodeObj, so i decide to save the old
obj, and only free the gcu node. So that need:

    1, free old gcu node
    2, set new obj's gcu node to old one
    3, free new obj, be careful about the pointer in the struct
    4, update the info for the old struct
*/
    /* STEP 1 & 2 */
    gcmkONERROR(gckVIDMEM_Exchange(kernel, oldObject, newObject));

    /* STEP 3 */
    gcmkONERROR(gckVIDMEM_MoveClean(kernel, newObject, newHandle));

    /* STEP 4 */
    gcmkONERROR(gckVIDMEM_NODE_GetMemoryHandle(kernel, oldObject, &memoryHandle));
    gcmkONERROR(gckVIDMEM_NODE_GetOffset(kernel, oldObject, &offset));
    gcmkONERROR(gckOS_MapPagesEx(kernel->os, kernel, mmu,
                                 memoryHandle,
                                 offset,
                                 oldObject->node->VidMem.pageCount,
                                 bo->evictInfo->address,
                                 bo->evictInfo->pageTables,
                                 gcvTRUE,
                                 oldObject->node->VidMem.type));

    gcmkONERROR(gckMMU_Flush(mmu, oldObject->node->VidMem.type));
    /* old Obj do not need to update gpu address and page address*/


    /* set the evictInfo pointer to null. */
    if (bo->evictInfo) {
        gcmkOS_SAFE_FREE(kernel->os, bo->evictInfo);
    }
OnError:
    gcmkFOOTER_DRM();
    return status;

}


/*  get physical address for first lock
    or update address after move*/
gceSTATUS
gckDRM_BOLock(gckGALDEVICE gal_dev, struct _viv_bo *bo, gctBOOL cacheable,
              gctUINT64_PTR logical, gctUINT32_PTR address, gcsHAL_INTERFACE *interface)
{
    gckDEVICE device = gal_dev->devices[0];
    gcsHAL_INTERFACE *iface;
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL unroll = 0;

    gcmkHEADER_DRM();

    gckDRM_BOReserve(bo, 1);

    if (!interface) {
        gckOS_Allocate(device->os, sizeof(gcsHAL_INTERFACE),
                       (gctPOINTER)&iface);
        gckOS_ZeroMemory(iface, sizeof(*iface));
        iface->command = gcvHAL_LOCK_VIDEO_MEMORY;
        iface->hardwareType = device->defaultHwType;
        iface->u.LockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_LOCK;
                                 //gcvLOCK_VIDEO_MEMORY_OP_MAP;
        if (bo->isImport)
            iface->u.LockVideoMemory.op |= gcvLOCK_VIDEO_MEMORY_OP_MAP;
        iface->u.LockVideoMemory.node = bo->nodeHandle;
        iface->u.LockVideoMemory.cacheable = cacheable;
    } else {
        iface = interface;
        iface->u.LockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_LOCK;
        if (iface->u.LockVideoMemory.op & gcvLOCK_VIDEO_MEMORY_OP_MAP) {
            gcmkPRINT("useless map");
            iface->u.LockVideoMemory.op = 0;
            iface->u.LockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_LOCK;
        }
        if (bo->isImport)
            iface->u.LockVideoMemory.op |= gcvLOCK_VIDEO_MEMORY_OP_MAP;
    }
    gcmkONERROR(gckDEVICE_Dispatch(device, iface));

    /* MMU for 2D engine is not on yet, 2D uses physical address directly,
     * anyway, 2D can't address past 4G.
     */
    if (bo->isImport)
        *logical = iface->u.LockVideoMemory.memory;
    //*logical = iface.u.LockVideoMemory.memory;

    *address = (uint32_t)iface->u.LockVideoMemory.address;
    bo->gpuAddress = *address;
/*set vma related(mmap)*/

    gckDRM_BOUnreserve(bo);

    gcmkFOOTER_DRM_ARG("gpu address = 0x%lx", *address);
    return status;

OnError:
    if (unroll) {
        memset(&iface, 0, sizeof(iface));
        iface->command = gcvHAL_UNLOCK_VIDEO_MEMORY;
        iface->hardwareType = device->defaultHwType;
        iface->u.UnlockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_UNLOCK;
        iface->u.UnlockVideoMemory.node = (gctUINT64)bo->nodeHandle;
        iface->u.UnlockVideoMemory.type = gcvVIDMEM_TYPE_GENERIC;
        gckDEVICE_Dispatch(device, iface);
    }
    gcmkFOOTER_DRM();

    return status;
}

gceSTATUS
gckDRM_BOUnlock(gckGALDEVICE gal_dev,
                struct _viv_bo *bo,
                gctUINT32 nodeHandle,
                gctBOOL *async,
                gctBOOL bottom)
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status = gcvSTATUS_OK;
    gckDEVICE device;

    gcmkHEADER_DRM_ARG("Handle = 0x%x IfBottom = %d", nodeHandle, bottom);

    device = gal_dev->devices[0];

    memset(&iface, 0, sizeof(iface));
    iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
    iface.hardwareType = device->defaultHwType;
    iface.u.UnlockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_UNLOCK
                                     | gcvLOCK_VIDEO_MEMORY_OP_UNMAP;

    iface.u.UnlockVideoMemory.node = (gctUINT64)nodeHandle;
    iface.u.UnlockVideoMemory.type = gcvVIDMEM_TYPE_GENERIC;
    iface.u.UnlockVideoMemory.asynchroneous = *async;
    gcmkONERROR(gckDEVICE_Dispatch(device, &iface));
    *async = iface.u.UnlockVideoMemory.asynchroneous;
    if (bottom) {
    memset(&iface, 0, sizeof(iface));
    iface.command = gcvHAL_BOTTOM_HALF_UNLOCK_VIDEO_MEMORY;
    iface.hardwareType = device->defaultHwType;
    iface.u.BottomHalfUnlockVideoMemory.node = (gctUINT64)nodeHandle;
    iface.u.BottomHalfUnlockVideoMemory.type = gcvVIDMEM_TYPE_GENERIC;
    gcmkONERROR(gckDEVICE_Dispatch(device, &iface));
    }

OnError:
    gcmkFOOTER_DRM();

    return gcmIS_ERROR(status) ? -ENOTTY : 0;
}

void
gckDRM_BORevokeMap(gckVIDMEM_NODE nodeObj)
{
    struct _viv_bo *bo;
    struct drm_gem_object *gemObj;
    struct drm_device *drm_dev;
  //  struct vm_area_struct *vma;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_DRM();

    if (!nodeObj) {
        gcmkPRINT("nodeObj is null");
        BUG_ON(1);
    }
    bo = (struct _viv_bo *)nodeObj->bo;
    if (!bo) {
//        gcmkPRINT("nodeObj don't have bo %d obj 0x%x ", _GetProcessID(), nodeObj);
        gcmkFOOTER_DRM();
        return;
//        BUG_ON(1);
    }
    gemObj = &bo->tbo.base;
    if (!gemObj) {
        gcmkPRINT("bo don't have gem");
        BUG_ON(1);
    }
    drm_dev = gemObj->dev;
    drm_vma_node_unmap(&gemObj->vma_node, drm_dev->anon_inode->i_mapping);
//    vma = find_vma(current->mm, bo->userLogical);
//    zap_vma_ptes(vma, vma->vm_start, vma->vm_end - vma->vm_start);

    gcmkFOOTER_DRM();
}
#endif /* gcdENABLE_TTM */

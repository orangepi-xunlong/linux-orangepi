/*
 * Copyright (C) 2012-2013 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/fs.h>	   /* file system operations */
#include <asm/uaccess.h>	/* user space access */
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include <linux/rbtree.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/spinlock.h>

#include "mali_ukk.h"
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_session.h"
#include "mali_kernel_linux.h"

#include "mali_kernel_memory_engine.h"
#include "mali_memory.h"
#include "mali_dma_buf.h"


struct mali_dma_buf_attachment {
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	struct mali_session_data *session;
	int map_ref;
	spinlock_t map_lock;
	mali_bool is_mapped;
	wait_queue_head_t wait_queue;
};

void mali_dma_buf_release(void *ctx, void *handle)
{
	struct mali_dma_buf_attachment *mem;

	mem = (struct mali_dma_buf_attachment *)handle;

	MALI_DEBUG_PRINT(3, ("Mali DMA-buf: release attachment %p\n", mem));

	MALI_DEBUG_ASSERT_POINTER(mem);
	MALI_DEBUG_ASSERT_POINTER(mem->attachment);
	MALI_DEBUG_ASSERT_POINTER(mem->buf);

#if defined(CONFIG_MALI_DMA_BUF_MAP_ON_ATTACH)
	/* We mapped implicitly on attach, so we need to unmap on release */
	mali_dma_buf_unmap(mem);
#endif

	/* Wait for buffer to become unmapped */
	wait_event(mem->wait_queue, 0 == mem->map_ref);
	MALI_DEBUG_ASSERT(!mem->is_mapped);

	dma_buf_detach(mem->buf, mem->attachment);
	dma_buf_put(mem->buf);

	_mali_osk_free(mem);
}

/*
 * Map DMA buf attachment \a mem into \a session at virtual address \a virt.
 */
int mali_dma_buf_map(struct mali_dma_buf_attachment *mem, struct mali_session_data *session, u32 virt, u32 *offset, u32 flags)
{
	struct mali_page_directory *pagedir;
	struct scatterlist *sg;
	unsigned long irq_flags;
	int i;

	MALI_DEBUG_ASSERT_POINTER(mem);
	MALI_DEBUG_ASSERT_POINTER(session);
	MALI_DEBUG_ASSERT(mem->session == session);

	spin_lock_irqsave(&mem->map_lock, irq_flags);

	mem->map_ref++;

	MALI_DEBUG_PRINT(5, ("Mali DMA-buf: map attachment %p, new map_ref = %d\n", mem, mem->map_ref));

	if (1 == mem->map_ref)
	{
		spin_unlock_irqrestore(&mem->map_lock, irq_flags);

		/* First reference taken, so we need to map the dma buf */
		MALI_DEBUG_ASSERT(!mem->is_mapped);

		pagedir = mali_session_get_page_directory(session);
		MALI_DEBUG_ASSERT_POINTER(pagedir);

		mem->sgt = dma_buf_map_attachment(mem->attachment, DMA_BIDIRECTIONAL);
		if (IS_ERR_OR_NULL(mem->sgt))
		{
			MALI_DEBUG_PRINT_ERROR(("Failed to map dma-buf attachment\n"));
			return -EFAULT;
		}

		for_each_sg(mem->sgt->sgl, sg, mem->sgt->nents, i)
		{
			u32 size = sg_dma_len(sg);
			dma_addr_t phys = sg_dma_address(sg);

			/* sg must be page aligned. */
			MALI_DEBUG_ASSERT(0 == size % MALI_MMU_PAGE_SIZE);

			mali_mmu_pagedir_update(pagedir, virt, phys, size, MALI_CACHE_STANDARD);

			virt += size;
			*offset += size;
		}

		if (flags & MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE)
		{
			u32 guard_phys;
			MALI_DEBUG_PRINT(7, ("Mapping in extra guard page\n"));

			guard_phys = sg_dma_address(mem->sgt->sgl);
			mali_mmu_pagedir_update(pagedir, virt, guard_phys, MALI_MMU_PAGE_SIZE, MALI_CACHE_STANDARD);
		}

		spin_lock_irqsave(&mem->map_lock, irq_flags);
		mem->is_mapped = MALI_TRUE;
		spin_unlock_irqrestore(&mem->map_lock, irq_flags);

		/* Wake up any thread waiting for buffer to become mapped */
		wake_up_all(&mem->wait_queue);
	}
	else
	{
		/* Wait for buffer to become mapped */
		if (!mem->is_mapped)
		{
			MALI_DEBUG_PRINT(4, ("Mali DMA-buf: Waiting for buffer to become mapped...\n"));
		}
		spin_unlock_irqrestore(&mem->map_lock, irq_flags);
		wait_event(mem->wait_queue, MALI_TRUE == mem->is_mapped);
	}

	return 0;
}

void mali_dma_buf_unmap(struct mali_dma_buf_attachment *mem)
{
	unsigned long irq_flags;

	MALI_DEBUG_ASSERT_POINTER(mem);
	MALI_DEBUG_ASSERT_POINTER(mem->attachment);
	MALI_DEBUG_ASSERT_POINTER(mem->buf);

	spin_lock_irqsave(&mem->map_lock, irq_flags);

	mem->map_ref--;

	MALI_DEBUG_PRINT(5, ("Mali DMA-buf: unmap attachment %p, new map_ref = %d\n", mem, mem->map_ref));

	if (0 == mem->map_ref)
	{
		dma_buf_unmap_attachment(mem->attachment, mem->sgt, DMA_BIDIRECTIONAL);

		mem->is_mapped = MALI_FALSE;
	}

	spin_unlock_irqrestore(&mem->map_lock, irq_flags);

	/* Wake up any thread waiting for buffer to become unmapped */
	wake_up_all(&mem->wait_queue);
}

#if !defined(CONFIG_MALI_DMA_BUF_MAP_ON_ATTACH)
int mali_dma_buf_map_job(struct mali_pp_job *job)
{
	mali_memory_allocation *descriptor;
	struct mali_dma_buf_attachment *mem;
	_mali_osk_errcode_t err;
	int i;
	u32 offset = 0;
	int ret = 0;

	_mali_osk_lock_wait( job->session->memory_lock, _MALI_OSK_LOCKMODE_RW );

	for (i = 0; i < job->num_memory_cookies; i++)
	{
		int cookie = job->memory_cookies[i];

		if (0 == cookie)
		{
			/* 0 is not a valid cookie */
			MALI_DEBUG_ASSERT(NULL == job->dma_bufs[i]);
			continue;
		}

		MALI_DEBUG_ASSERT(0 < cookie);

		err = mali_descriptor_mapping_get(job->session->descriptor_mapping,
				cookie, (void**)&descriptor);

		if (_MALI_OSK_ERR_OK != err)
		{
			MALI_DEBUG_PRINT_ERROR(("Mali DMA-buf: Failed to get descriptor for cookie %d\n", cookie));
			ret = -EFAULT;
			MALI_DEBUG_ASSERT(NULL == job->dma_bufs[i]);
			continue;
		}

		if (mali_dma_buf_release != descriptor->physical_allocation.release)
		{
			/* Not a DMA-buf */
			MALI_DEBUG_ASSERT(NULL == job->dma_bufs[i]);
			continue;
		}

		mem = (struct mali_dma_buf_attachment *)descriptor->physical_allocation.handle;

		MALI_DEBUG_ASSERT_POINTER(mem);
		MALI_DEBUG_ASSERT(mem->session == job->session);

		err = mali_dma_buf_map(mem, mem->session, descriptor->mali_address, &offset, descriptor->flags);
		if (0 != err)
		{
			MALI_DEBUG_PRINT_ERROR(("Mali DMA-buf: Failed to map dma-buf for cookie %d at mali address %x\b",
			                        cookie, descriptor->mali_address));
			ret = -EFAULT;
			MALI_DEBUG_ASSERT(NULL == job->dma_bufs[i]);
			continue;
		}

		/* Add mem to list of DMA-bufs mapped for this job */
		job->dma_bufs[i] = mem;
	}

	_mali_osk_lock_signal( job->session->memory_lock, _MALI_OSK_LOCKMODE_RW );

	return ret;
}

void mali_dma_buf_unmap_job(struct mali_pp_job *job)
{
	int i;
	for (i = 0; i < job->num_dma_bufs; i++)
	{
		if (NULL == job->dma_bufs[i]) continue;

		mali_dma_buf_unmap(job->dma_bufs[i]);
		job->dma_bufs[i] = NULL;
	}
}
#endif /* !CONFIG_MALI_DMA_BUF_MAP_ON_ATTACH */

/* Callback from memory engine which will map into Mali virtual address space */
static mali_physical_memory_allocation_result mali_dma_buf_commit(void* ctx, mali_allocation_engine * engine, mali_memory_allocation * descriptor, u32* offset, mali_physical_memory_allocation * alloc_info)
{
	struct mali_session_data *session;
	struct mali_dma_buf_attachment *mem;

	MALI_DEBUG_ASSERT_POINTER(ctx);
	MALI_DEBUG_ASSERT_POINTER(engine);
	MALI_DEBUG_ASSERT_POINTER(descriptor);
	MALI_DEBUG_ASSERT_POINTER(offset);
	MALI_DEBUG_ASSERT_POINTER(alloc_info);

	/* Mapping dma-buf with an offset is not supported. */
	MALI_DEBUG_ASSERT(0 == *offset);

	session = (struct mali_session_data *)descriptor->mali_addr_mapping_info;
	MALI_DEBUG_ASSERT_POINTER(session);

	mem = (struct mali_dma_buf_attachment *)ctx;

	MALI_DEBUG_ASSERT(mem->session == session);

#if defined(CONFIG_MALI_DMA_BUF_MAP_ON_ATTACH)
	if (0 == mali_dma_buf_map(mem, session, descriptor->mali_address, offset, descriptor->flags))
	{
		MALI_DEBUG_ASSERT(*offset == descriptor->size);
#else
	{
#endif
		alloc_info->ctx = NULL;
		alloc_info->handle = mem;
		alloc_info->next = NULL;
		alloc_info->release = mali_dma_buf_release;

		return MALI_MEM_ALLOC_FINISHED;
	}

	return MALI_MEM_ALLOC_INTERNAL_FAILURE;
}

int mali_attach_dma_buf(struct mali_session_data *session, _mali_uk_attach_dma_buf_s __user *user_arg)
{
	mali_physical_memory_allocator external_memory_allocator;
	struct dma_buf *buf;
	struct mali_dma_buf_attachment *mem;
	_mali_uk_attach_dma_buf_s args;
	mali_memory_allocation *descriptor;
	int md;
	int fd;

	/* Get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if (0 != copy_from_user(&args, (void __user *)user_arg, sizeof(_mali_uk_attach_dma_buf_s)))
	{
		return -EFAULT;
	}


	fd = args.mem_fd;

	buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(buf))
	{
		MALI_DEBUG_PRINT_ERROR(("Failed to get dma-buf from fd: %d\n", fd));
		return PTR_RET(buf);
	}

	/* Currently, mapping of the full buffer are supported. */
	if (args.size != buf->size)
	{
		MALI_DEBUG_PRINT_ERROR(("dma-buf size doesn't match mapping size.\n"));
		dma_buf_put(buf);
		return -EINVAL;
	}

	mem = _mali_osk_calloc(1, sizeof(struct mali_dma_buf_attachment));
	if (NULL == mem)
	{
		MALI_DEBUG_PRINT_ERROR(("Failed to allocate dma-buf tracing struct\n"));
		dma_buf_put(buf);
		return -ENOMEM;
	}

	mem->buf = buf;
	mem->session = session;
	mem->map_ref = 0;
	spin_lock_init(&mem->map_lock);
	init_waitqueue_head(&mem->wait_queue);

	mem->attachment = dma_buf_attach(mem->buf, &mali_platform_device->dev);
	if (NULL == mem->attachment)
	{
		MALI_DEBUG_PRINT_ERROR(("Failed to attach to dma-buf %d\n", fd));
		dma_buf_put(mem->buf);
		_mali_osk_free(mem);
		return -EFAULT;
	}

	/* Map dma-buf into this session's page tables */

	/* Set up Mali memory descriptor */
	descriptor = _mali_osk_calloc(1, sizeof(mali_memory_allocation));
	if (NULL == descriptor)
	{
		MALI_DEBUG_PRINT_ERROR(("Failed to allocate descriptor dma-buf %d\n", fd));
		mali_dma_buf_release(NULL, mem);
		return -ENOMEM;
	}

	descriptor->size = args.size;
	descriptor->mapping = NULL;
	descriptor->mali_address = args.mali_address;
	descriptor->mali_addr_mapping_info = (void*)session;
	descriptor->process_addr_mapping_info = NULL; /* do not map to process address space */
	descriptor->lock = session->memory_lock;

	if (args.flags & _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE)
	{
		descriptor->flags = MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE;
	}
	_mali_osk_list_init( &descriptor->list );

	/* Get descriptor mapping for memory. */
	if (_MALI_OSK_ERR_OK != mali_descriptor_mapping_allocate_mapping(session->descriptor_mapping, descriptor, &md))
	{
		MALI_DEBUG_PRINT_ERROR(("Failed to create descriptor mapping for dma-buf %d\n", fd));
		_mali_osk_free(descriptor);
		mali_dma_buf_release(NULL, mem);
		return -EFAULT;
	}

	MALI_DEBUG_ASSERT(0 < md);

	external_memory_allocator.allocate = mali_dma_buf_commit;
	external_memory_allocator.allocate_page_table_block = NULL;
	external_memory_allocator.ctx = mem;
	external_memory_allocator.name = "DMA-BUF Memory";
	external_memory_allocator.next = NULL;

	/* Map memory into session's Mali virtual address space. */
	_mali_osk_lock_wait(session->memory_lock, _MALI_OSK_LOCKMODE_RW);
	if (_MALI_OSK_ERR_OK != mali_allocation_engine_allocate_memory(mali_mem_get_memory_engine(), descriptor, &external_memory_allocator, NULL))
	{
		_mali_osk_lock_signal(session->memory_lock, _MALI_OSK_LOCKMODE_RW);

		MALI_DEBUG_PRINT_ERROR(("Failed to map dma-buf %d into Mali address space\n", fd));
		mali_descriptor_mapping_free(session->descriptor_mapping, md);
		mali_dma_buf_release(NULL, mem);
		return -ENOMEM;
	}
	_mali_osk_lock_signal(session->memory_lock, _MALI_OSK_LOCKMODE_RW);

	/* Return stuff to user space */
	if (0 != put_user(md, &user_arg->cookie))
	{
		/* Roll back */
		MALI_DEBUG_PRINT_ERROR(("Failed to return descriptor to user space for dma-buf %d\n", fd));
		mali_descriptor_mapping_free(session->descriptor_mapping, md);
		mali_dma_buf_release(NULL, mem);
		return -EFAULT;
	}

	return 0;
}

int mali_release_dma_buf(struct mali_session_data *session, _mali_uk_release_dma_buf_s __user *user_arg)
{
	int ret = 0;
	_mali_uk_release_dma_buf_s args;
	mali_memory_allocation *descriptor;

	/* get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if ( 0 != copy_from_user(&args, (void __user *)user_arg, sizeof(_mali_uk_release_dma_buf_s)) )
	{
		return -EFAULT;
	}

	MALI_DEBUG_PRINT(3, ("Mali DMA-buf: release descriptor cookie %d\n", args.cookie));

	_mali_osk_lock_wait( session->memory_lock, _MALI_OSK_LOCKMODE_RW );

	descriptor = mali_descriptor_mapping_free(session->descriptor_mapping, args.cookie);

	if (NULL != descriptor)
	{
		MALI_DEBUG_PRINT(3, ("Mali DMA-buf: Releasing dma-buf at mali address %x\n", descriptor->mali_address));

		/* Will call back to mali_dma_buf_release() which will release the dma-buf attachment. */
		mali_allocation_engine_release_memory(mali_mem_get_memory_engine(), descriptor);

		_mali_osk_free(descriptor);
	}
	else
	{
		MALI_DEBUG_PRINT_ERROR(("Invalid memory descriptor %d used to release dma-buf\n", args.cookie));
		ret = -EINVAL;
	}

	_mali_osk_lock_signal( session->memory_lock, _MALI_OSK_LOCKMODE_RW );

	/* Return the error that _mali_ukk_map_external_ump_mem produced */
	return ret;
}

int mali_dma_buf_get_size(struct mali_session_data *session, _mali_uk_dma_buf_get_size_s __user *user_arg)
{
	_mali_uk_dma_buf_get_size_s args;
	int fd;
	struct dma_buf *buf;

	/* get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if ( 0 != copy_from_user(&args, (void __user *)user_arg, sizeof(_mali_uk_dma_buf_get_size_s)) )
	{
		return -EFAULT;
	}

	/* Do DMA-BUF stuff */
	fd = args.mem_fd;

	buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(buf))
	{
		MALI_DEBUG_PRINT_ERROR(("Failed to get dma-buf from fd: %d\n", fd));
		return PTR_RET(buf);
	}

	if (0 != put_user(buf->size, &user_arg->size))
	{
		dma_buf_put(buf);
		return -EFAULT;
	}

	dma_buf_put(buf);

	return 0;
}

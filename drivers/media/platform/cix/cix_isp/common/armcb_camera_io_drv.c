// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/ftrace.h>
#include <linux/miscdevice.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "armcb_camera_io_drv.h"
#include "armcb_platform.h"
#include "armcb_register.h"
#include "armcb_sensor.h"
#include "isp_hw_ops.h"
#include "system_dma.h"
#include "system_logger.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

//#define MEM_DEBUG

#define MMUSID_MASK (0x000FFFF0)
#define MMUSID (0x1F)
#define ISP0_STRP_PIN2 (0x308)
#define ISP0_STRP_PIN3 (0x30c)
#define ISP1_STRP_PIN0 (0x300)

#define SEG_SIZE 0x1FFFFFFF

#define DEVICE_NAME "ISP-Mem"

struct armcb_ispmem_info *cam_mem_info;
static struct current_status cmamem_status;
static struct cmamem_dev cmamem_dev;
static struct cmamem_block *cmamem_block_head;
static int mem_block_count;
static struct cam_buf_table buf_tbl;

DEFINE_MUTEX(phy_mutex);

#ifdef ARMCB_CAM_AHB

u32 armcb_ahb_pmctrl_res_read_reg(u32 offset)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;

	virt_addr = armcb_ahb_pmctrl_res_get_addr_base();
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}
void armcb_ahb_pmctrl_res_write_reg(u32 offset, u32 value)
{
	void __iomem *virt_addr = NULL;

	virt_addr = armcb_ahb_pmctrl_res_get_addr_base();
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}

u32 armcb_ahb_rcsuisp0_res_read_reg(u32 offset)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;

	virt_addr = armcb_ahb_rcsuisp0_res_get_addr_base();
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void armcb_ahb_rcsuisp0_res_write_reg(u32 offset, u32 value)
{
	void __iomem *virt_addr = NULL;

	virt_addr = armcb_ahb_rcsuisp0_res_get_addr_base();
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}

u32 armcb_ahb_rcsuisp1_res_read_reg(u32 offset)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;

	virt_addr = armcb_ahb_rcsuisp1_res_get_addr_base();
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void armcb_ahb_rcsuisp1_res_write_reg(u32 offset, u32 value)
{
	void __iomem *virt_addr = NULL;

	virt_addr = armcb_ahb_rcsuisp1_res_get_addr_base();
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}
#endif

u32 armcb_apb2_read_reg(u32 offset)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;

	virt_addr = armcb_apb2_get_addr_base();
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
#ifndef ARMCB_CAM_DEBUG
	reg_val = readl(virt_addr + offset);
#endif

	return reg_val;
}

void armcb_apb2_write_reg(u32 offset, u32 value)
{
	void __iomem *virt_addr = NULL;

	virt_addr = armcb_apb2_get_addr_base();
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
#ifndef ARMCB_CAM_DEBUG
	writel(value, virt_addr + offset);
#endif
}

u32 armcb_register_get_int32(u32 phy_addr)
{
	void __iomem *virt_addr;
	u32 reg_val = 0;

	mutex_lock(&phy_mutex);
	/* ioremap: convert phy addr to virtual addr*/
	virt_addr = (void __iomem *)ioremap(phy_addr, 4);
	reg_val = *(u32 *)virt_addr;
	iounmap((void __iomem *)virt_addr);
	mutex_unlock(&phy_mutex);

	return reg_val;
}

void armcb_register_set_int32(u32 phy_addr, u32 value)
{
	void __iomem *virt_addr;

	mutex_lock(&phy_mutex);
	/* ioremap: convert phy addr to virtual addr*/
	virt_addr = (void __iomem *)ioremap(phy_addr, 4);
	/* write value to virtual addr */
	*(u32 *)virt_addr = value;
	/*read back the value of virtual addr */
#ifndef ARMCB_CAM_DEBUG
	iounmap((void __iomem *)virt_addr);
#endif

	mutex_unlock(&phy_mutex);
}

#ifdef ARMCB_CAM_AHB

void __iomem *armcb_ahb_pmctrl_res_get_addr_base(void)
{
	return cam_mem_info->ahb_pmctrl_res_base_addr;
}

void __iomem *armcb_ahb_rcsuisp0_res_get_addr_base(void)
{
	return cam_mem_info->ahb_rcsuisp0_res_base_addr;
}

void __iomem *armcb_ahb_rcsuisp1_res_get_addr_base(void)
{
	return cam_mem_info->ahb_rcsuisp1_res_base_addr;
}
#endif

void __iomem *armcb_apb2_get_addr_base(void)
{
	return cam_mem_info->apb2_base_addr;
}

void __iomem *armcb_ispmem_get_cdma_base(void)
{
	return cam_mem_info->cdma_base_addr;
}

static int ispmem_open(struct inode *node, struct file *file)
{
	return 0;
}

#ifdef MEM_DEBUG
static void ispmem_cma_dump(struct cmamem_block *memory_block)
{
	LOG(LOG_INFO, "CMA name:   %s", memory_block->name);
	LOG(LOG_INFO, "CMA id:     %d", memory_block->id);
	LOG(LOG_INFO, "Is  usebuf: %d", memory_block->is_use_buffer);
	LOG(LOG_INFO, "PHY Base:   0x%lx", memory_block->phy_addr);
	LOG(LOG_INFO, "KER Base:   %p", memory_block->kernel_addr);
	LOG(LOG_INFO, "USR Base:   0x%lx", memory_block->usr_addr);
	LOG(LOG_INFO, "Length:     0x%x", memory_block->len);
}
#endif

static long ispmem_cma_alloc(struct file *file, unsigned long arg)
{
	struct cmamem_block *memory_block;
	struct mem_block cma_info_temp;
	int size;
	int ret = 0;

	if (copy_from_user(&cma_info_temp, (void __user *)arg,
			   sizeof(struct mem_block))) {
		LOG(LOG_ERR, "copy_from_user error:%d", ret);
		return -1;
	}

	if (cma_info_temp.name[0] == '\0') {
		LOG(LOG_ERR, "no set mem name, please set");
		return -1;
	}

	if (cma_info_temp.len) {
		size = PAGE_ALIGN(cma_info_temp.len);
		cma_info_temp.len = size;
#ifdef MEM_DEBUG
		LOG(LOG_INFO, "len:%u, is_use_buffer:%u.", cma_info_temp.len,
			cma_info_temp.is_use_buffer);
		LOG(LOG_INFO, "cmamem_dev.pddev= 0x%p", cmamem_dev.pddev);
#endif
		if (cmamem_dev.has_iommu) {
			cma_info_temp.kernel_addr = dma_alloc_attrs(
				cmamem_dev.pddev, size,
				(dma_addr_t *)(&(cma_info_temp.phy_addr)),
				GFP_KERNEL, DMA_ATTR_FORCE_CONTIGUOUS);
		} else {
			cma_info_temp.kernel_addr = dma_alloc_coherent(
				cmamem_dev.pddev, size,
				(dma_addr_t *)(&(cma_info_temp.phy_addr)),
				GFP_KERNEL);
		}
		if (!cma_info_temp.phy_addr) {
			LOG(LOG_ERR, "dma alloc fail!");
			return -ENOMEM;
		}

#ifdef MEM_DEBUG
		LOG(LOG_INFO, "kernel_addr = 0x%lx, phy_addr = 0x%lx",
			cma_info_temp.kernel_addr,
			(dma_addr_t)cma_info_temp.phy_addr);
#endif
		cma_info_temp.id = ++mem_block_count;

		cmamem_status.vir_addr = cma_info_temp.kernel_addr;
		cmamem_status.phy_addr = cma_info_temp.phy_addr;
		cmamem_status.id_count = cma_info_temp.id;
		cmamem_status.status = HAVE_ALLOCED;

		cma_info_temp.usr_addr = vm_mmap(
			file, 0, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0);
		if (cma_info_temp.usr_addr < 0) {
			LOG(LOG_ERR, "do_mmap fail:%d! (%lu)", __LINE__,
				cma_info_temp.usr_addr);
			cma_info_temp.id = --mem_block_count;
			return -ENOMEM;
		}
	} else {
		LOG(LOG_ERR, "the len is NULL");
		return -1;
	}

	if (copy_to_user((void __user *)arg, (void *)(&cma_info_temp),
			 sizeof(struct mem_block))) {
		LOG(LOG_ERR, "fail to copy_to_user cma_info.");
		return -EFAULT;
	}

	/* setup the memory block */
	memory_block = kmalloc(
		sizeof(struct cmamem_block), GFP_KERNEL);
	if (memory_block == NULL) {
		LOG(LOG_ERR, "failed to kmalloc memory");
		mem_block_count--;
		return -1;
	}

	if (cma_info_temp.name[0] != '\0')
		memcpy(memory_block->name, cma_info_temp.name, 10);

	memory_block->id            =   cma_info_temp.id;
	memory_block->is_free       =   0;
	memory_block->is_use_buffer =   cma_info_temp.is_use_buffer;
	memory_block->usr_addr      =   cma_info_temp.usr_addr;
	memory_block->kernel_addr   =   cma_info_temp.kernel_addr;
	memory_block->phy_addr      =   cma_info_temp.phy_addr;
	memory_block->len           =   cma_info_temp.len;

#ifdef MEM_DEBUG
	ispmem_cma_dump(memory_block);
#endif

	/* add to memory block queue */
	list_add_tail(&memory_block->memqueue_list,
			  &cmamem_block_head->memqueue_list);

	return 0;
}

static int ispmem_cma_free(struct file *file, unsigned long arg)
{
	int ret = 0;
	struct cmamem_block *memory_block = NULL;
	struct cmamem_block *memory_block_next = NULL;
	struct mem_block cma_info_temp;

	if (copy_from_user(&cma_info_temp, (void __user *)arg,
			   sizeof(struct mem_block))) {
		LOG(LOG_ERR, "ispmem_cma_alloc:copy_from_user error:%d", ret);
		return -1;
	}
#ifdef MEM_DEBUG
	LOG(LOG_INFO, "will delete the mem name:%s", cma_info_temp.name);
#endif

	/// list_for_each_entry_safe for list_del
	list_for_each_entry_safe(memory_block, memory_block_next,
				  &cmamem_block_head->memqueue_list,
				  memqueue_list) {
		if (memory_block) {
			if (cma_info_temp.id == memory_block->id) {
				if (memory_block->is_free == 0) {
#ifdef MEM_DEBUG
					LOG(LOG_INFO,
						"delete the mem id:%d, name:%s",
						cma_info_temp.id,
						cma_info_temp.name);
#endif
					vm_munmap(memory_block->usr_addr,
						  memory_block->len);
					dma_free_coherent(
						cmamem_dev.pddev,
						memory_block->len,
						memory_block->kernel_addr,
						memory_block->phy_addr);

					memory_block->is_free = 1;

					list_del(&memory_block->memqueue_list);
					kfree(memory_block);
					break;
				}
			}
		}
	}

	return 0;
}
static int ispmem_cma_import(struct file *file, unsigned long arg)
{
	int ret = 0;
	struct cmamem_block *memory_block = NULL;
	struct cmamem_block *memory_block_next = NULL;
	struct mem_block cma_info_temp;

	if (copy_from_user(&cma_info_temp, (void __user *)arg,
			   sizeof(struct mem_block))) {
		LOG(LOG_ERR, "copy_from_user error:%d", ret);
		return -1;
	}

	/// list_for_each_entry_safe for list_del
	list_for_each_entry_safe(memory_block, memory_block_next,
				  &cmamem_block_head->memqueue_list,
				  memqueue_list) {
		if (memory_block) {
			if (cma_info_temp.id == memory_block->id) {
				if (memory_block->is_free == 0) {
					LOG(LOG_DEBUG,
						"import the mem id:%d, name:%s",
						cma_info_temp.id,
						cma_info_temp.name);
					if (cma_info_temp.name[0] != '\0') {
						memcpy(memory_block->name,
							   cma_info_temp.name, 10);
					}

					cma_info_temp.is_use_buffer =
						memory_block->is_use_buffer;
					cma_info_temp.usr_addr =
						memory_block->usr_addr;
					cma_info_temp.kernel_addr =
						memory_block->kernel_addr;
					cma_info_temp.phy_addr =
						memory_block->phy_addr;
					cma_info_temp.len = memory_block->len;

					if (copy_to_user(
							(void __user *)arg,
							(void *)(&cma_info_temp),
							sizeof(struct mem_block))) {
						LOG(LOG_ERR,
							"fail to copy_to_user cma_info.");
						return -EFAULT;
					}
				}
				break;
			}
		}
	}

	return 0;
}

static int ispmem_cma_free_all(void)
{
	struct cmamem_block *memory_block = NULL;
	struct cmamem_block *memory_block_next = NULL;

#ifdef MEM_DEBUG
	LOG(LOG_INFO, "will delete all cma mem.");
#endif

	/// list_for_each_entry_safe for list_del
	list_for_each_entry_safe(memory_block, memory_block_next,
				  &cmamem_block_head->memqueue_list,
				  memqueue_list) {
		if (memory_block && memory_block->id > 0) {
			if (memory_block->is_free == 0) {
#ifdef MEM_DEBUG
				LOG(LOG_INFO, "delete the mem id:%d, name:%s",
					memory_block->id, memory_block->name);
#endif
				dma_free_coherent(cmamem_dev.pddev,
						  memory_block->len,
						  memory_block->kernel_addr,
						  memory_block->phy_addr);

				memory_block->is_free = 1;
				list_del(&memory_block->memqueue_list);
				kfree(memory_block);
			}
		}
	}

#ifdef MEM_DEBUG
	LOG(LOG_INFO, "free memory done");
#endif

	return 0;
}

static int ispmem_cma_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;

	if (cmamem_status.status != HAVE_ALLOCED) {
		LOG(LOG_ERR, "you should allocated memory firstly.");
		return -EINVAL;
	}

	if (dma_mmap_coherent(cam_mem_info->pddev, vma, cmamem_status.vir_addr,
				  cmamem_status.phy_addr, size)) {
		LOG(LOG_ERR, "dma mmap failed .");
		return -EIO;
	}

	vma->vm_flags &= ~VM_IO;
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);

	cmamem_status.status = HAVE_MMAPED;
	return 0;
}

#ifdef CONFIG_ARENA_FPGA_PLATFORM
static int ispmem_xdma(void *arg)
{
	struct armcb_xdma_req xdma_req;
	int res = 0;

	res = copy_from_user(&xdma_req, arg, sizeof(struct armcb_xdma_req));
	if (res != 0) {
		LOG(LOG_ERR, "copy_from_user error ! 0x%p res = %d", arg, res);
		return -EFAULT;
	}

#if __SIZEOF_POINTER__ == 4
	if (xdma(xdma_req.rmt_addr, (u32)xdma_req.local_addr, xdma_req.size,
		 xdma_req.direction) < 0) {
		LOG(LOG_ERR, "xdma failed !");
		return -EFAULT;
	}
#endif

	return res;
}

/// test AXI bus transfer efficiency
static int ispmem_xdma_test(void *arg)
{
	int res = 0;
	s64 cost_time = 0;
	ktime_t ktime = 0;
	ktime_t start_time = 0;
	struct perf_bus_params bus_params = { 0 };
	struct perf_bus_params *pkel_bus_params = &bus_params;
	struct perf_bus_params *puser_bus_params =
		(struct perf_bus_params *)arg;

	if (WARN_ON(!puser_bus_params)) {
		LOG(LOG_ERR, "input parameters is invalid ");
		return -EINVAL;
	}

	start_time = ktime_get();
	res = copy_from_user(pkel_bus_params, puser_bus_params,
				 sizeof(struct perf_bus_params));
	if (res != 0) {
		LOG(LOG_ERR, "copy_from_user error! 0x%p res = %d",
			puser_bus_params, res);
		return -EFAULT;
	}

	/// read test
	ktime = ktime_get();
	if (xdma(pkel_bus_params->rmt_addr, pkel_bus_params->local_addr,
		 pkel_bus_params->bytes, 0) < 0) {
		LOG(LOG_ERR, "xdma failed !");
		return -EFAULT;
	}
	ktime = ktime_sub(ktime_get(), ktime);
	cost_time = ktime_to_us(ktime);
	if (cost_time > 0)
		pkel_bus_params->read_cost = (unsigned int)cost_time;

	/// write test
	ktime = ktime_get();
	if (xdma(pkel_bus_params->rmt_addr, pkel_bus_params->local_addr,
		 pkel_bus_params->bytes, 1) < 0) {
		LOG(LOG_ERR, "xdma failed !");
		return -EFAULT;
	}
	ktime = ktime_sub(ktime_get(), ktime);
	cost_time = ktime_to_us(ktime);
	if (cost_time > 0)
		pkel_bus_params->write_cost = (unsigned int)cost_time;

	res = copy_to_user(puser_bus_params, pkel_bus_params,
			   sizeof(struct perf_bus_params));
	if (res != 0) {
		LOG(LOG_ERR, "copy_to_user error ! 0x%p res = %d",
			puser_bus_params, res);
		return -EFAULT;
	}
	ktime = ktime_sub(ktime_get(), start_time);
	cost_time = ktime_to_us(ktime);
	LOG(LOG_INFO, "xdma test total cost %lld us!", cost_time);

	return res;
}
#endif
static int cam_mem_get_avaliable_buf_idx(void)
{
	int idx = 0;

	mutex_lock(&buf_tbl.m_lock);
	for (idx = 0; idx < CAM_MEM_BUFQ_MAX; idx++) {
		if (!buf_tbl.bitMap[idx]) {
			buf_tbl.bitMap[idx] = 1;
			break;
		}
	}

	if (idx >= CAM_MEM_BUFQ_MAX) {
		idx = -1;
		LOG(LOG_ERR, "Error! No available buffer\n");
	}

	mutex_unlock(&buf_tbl.m_lock);
	return idx;
}

static void cam_mem_put_bux_idx(int idx)
{
	mutex_lock(&buf_tbl.m_lock);
	buf_tbl.bitMap[idx] = 0;
	mutex_unlock(&buf_tbl.m_lock);
}

static int cam_mem_tbl_init(void)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < CAM_MEM_BUFQ_MAX; i++) {
		buf_tbl.bitMap[i] = 0;
		buf_tbl.bufq[i].fd = -1;
		buf_tbl.bufq[i].bufIdx = -1;
		buf_tbl.bufq[i].flags = 0;
		buf_tbl.bufq[i].flags = 0;
	}

	return ret;
}

static int cam_mem_release_all(void)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < CAM_MEM_BUFQ_MAX; i++) {
		if (buf_tbl.bitMap[i]) {
			LOG(LOG_ERR,
				"Error! buf idx %d still alive, Possible memory leak\n",
				i);
			///@TODO
			/// unmap and release buffer here
			ret = -EINVAL;
		}

		buf_tbl.bitMap[i] = 0;
		buf_tbl.bufq[i].fd = -1;
		buf_tbl.bufq[i].bufIdx = -1;
		buf_tbl.bufq[i].flags = 0;
		buf_tbl.bufq[i].flags = 0;
	}

	return ret;
}

static int cam_mem_buf_map(unsigned long arg)
{
	int ret = 0;
	struct sg_table *table;
	struct device *dev = cam_mem_info->pddev;
	struct dma_buf *dmabuf;
	unsigned long phy_addr;
	unsigned long size;
	struct hw_mem_map_cmd map_cmd;
	struct dma_buf_attachment *attachment;
	int idx;
	unsigned long kvaddr;
	struct iosys_map map;

	if (copy_from_user(&map_cmd, (void __user *)arg,
			   sizeof(struct hw_mem_map_cmd))) {
		LOG(LOG_ERR, "CAM_HW_BUFFER_MAP :copy_from_user error !\n");
		return -EINVAL;
	}

	dmabuf = dma_buf_get(map_cmd.fd);
	attachment = dma_buf_attach(dmabuf, cam_mem_info->pddev);
	table = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
	phy_addr = sg_dma_address(table->sgl);
	size = sg_dma_len(table->sgl);
	ret = (unsigned long)dma_buf_vmap(dmabuf, &map);

	kvaddr = ret ? 0 : (uintptr_t)map.vaddr;

	if (kvaddr == 0) {
		dev_err(dev, "kernel kmap failed");
		ret = -EINVAL;
		goto dma_kmap_failed;
	}

	idx = cam_mem_get_avaliable_buf_idx();

	buf_tbl.bufq[idx].dma = dmabuf;
	buf_tbl.bufq[idx].bufIdx = idx;
	buf_tbl.bufq[idx].len = size;
	buf_tbl.bufq[idx].phy_addr = phy_addr;
	buf_tbl.bufq[idx].kvaddr = kvaddr;
	buf_tbl.bufq[idx].attachment = attachment;
	buf_tbl.bufq[idx].table = table;

	/// copy to user
	map_cmd.out.phyAddr = phy_addr;
	map_cmd.out.kvAddr = kvaddr;
	map_cmd.out.kBufhandle = idx;
	if (copy_to_user((void __user *)arg, &map_cmd,
			 sizeof(struct hw_mem_map_cmd))) {
		LOG(LOG_ERR, "CAM_HW_BUFFER_MAP :copy_from_user error !\n");
		ret = -EINVAL;
		goto dma_kmap_failed;
	}

	return ret;

dma_kmap_failed:
	dma_buf_unmap_attachment(attachment, table, DMA_BIDIRECTIONAL);
	dma_buf_detach(dmabuf, attachment);
	dma_buf_put(dmabuf);
	return ret;
}

static int cam_mem_buf_release(unsigned long arg)
{
	int ret = 0;
	int idx = 0;
	struct hw_mem_release_cmd release_cmd;
	struct iosys_map map;

	if (copy_from_user(&release_cmd, (void __user *)arg,
			   sizeof(struct hw_mem_release_cmd))) {
		LOG(LOG_ERR, "CAM_HW_BUFFER_MAP :copy_from_user error !");
		ret = -EINVAL;
	}

	idx = release_cmd.kBufhandle;
	map.is_iomem = false;
	map.vaddr = (void *)buf_tbl.bufq[idx].kvaddr;

	if (idx >= 0) {
		dma_buf_unmap_attachment(buf_tbl.bufq[idx].attachment,
					 buf_tbl.bufq[idx].table,
					 DMA_BIDIRECTIONAL);
		dma_buf_detach(buf_tbl.bufq[idx].dma,
				   buf_tbl.bufq[idx].attachment);
		dma_buf_vunmap(buf_tbl.bufq[idx].dma, &map);
		dma_buf_put(buf_tbl.bufq[idx].dma);
		cam_mem_put_bux_idx(idx);
	} else {
		LOG(LOG_ERR, "error buffer idx");
		ret = -EINVAL;
	}

	return ret;
}
static u32 Global_PowerDone;
int armcb_isp_power(int enable, unsigned long arg)
{
	u32 freq[2];
	u64 a_clk_rate;
	u64 s_clk_rate;
	int reg_data;

	LOG(LOG_INFO, " function enter  %d", enable);
	if (copy_from_user(&freq, (void __user *)arg, sizeof(freq))) {
		LOG(LOG_ERR, "copy_from_user error !");
		return -1;
	}
	LOG(LOG_INFO, "enable is %d, copy_from_user  %d %d!",
		enable, freq[0], freq[1]);

	if (enable && (Global_PowerDone == 0)) {
		if ((freq[0] > ISP_PLL_CLK) || (freq[0] < 0)) {
			LOG(LOG_ERR,
				"a_freq: %d  is invalid and freq = ISP_PLL_CLK!",
				freq[0]);
			freq[0] = ISP_PLL_CLK;
		}

		if ((freq[1] > ISP_PLL_CLK) || (freq[1] < 0)) {
			LOG(LOG_ERR,
				"s_freq: %d  is invalid and freq = ISP_PLL_CLK!",
				freq[1]);
			freq[1] = ISP_PLL_CLK;
		}
		/********************TODO***************************
		 ****************Convert frequency to HZ************
		 *******************freq * 1000*********************/
		a_clk_rate = freq[0] * 1000 * 1000;
		s_clk_rate = freq[1] * 1000 * 1000;
		LOG(LOG_INFO, "a_clk_rate is  %ld, s_clk_rate is  %ld, !",
			a_clk_rate, s_clk_rate);
		pm_runtime_get_sync(cam_mem_info->pddev);
		clk_set_rate(cam_mem_info->isp_aclk, a_clk_rate);
		clk_set_rate(cam_mem_info->isp_sclk, s_clk_rate);

		reg_data = armcb_ahb_rcsuisp0_res_read_reg(ISP0_STRP_PIN2);
		reg_data &= ~MMUSID_MASK;
		reg_data |= ((MMUSID << 4) & MMUSID_MASK);
		armcb_ahb_rcsuisp0_res_write_reg(ISP0_STRP_PIN2, reg_data);

		reg_data = armcb_ahb_rcsuisp1_res_read_reg(ISP1_STRP_PIN0);
		reg_data &= ~MMUSID_MASK;
		reg_data |= ((MMUSID << 4) & MMUSID_MASK);
		armcb_ahb_rcsuisp1_res_write_reg(ISP1_STRP_PIN0, reg_data);
		Global_PowerDone = 1;

	} else if ((enable == 0) && (Global_PowerDone == 1)) {
		Global_PowerDone = 0;
		pm_runtime_put(cam_mem_info->pddev);
	}
	return 0;
}

static long ispmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int res = 0;

	LOG(LOG_DEBUG, "cmd=%x", cmd);
	switch (cmd) {
	case VIDIOC_S_PLATFORM_HW_INIT:
		armcb_isp_power(1, arg);
#ifdef ARMCB_FPGA_HW
		armcb_fpga_hwcfg_init();
#endif
		break;
	case VIDIOC_S_PLATFORM_HW_UNINIT:
		armcb_isp_power(0, arg);
		break;
	case CMEM_ALLOCATE:
		mutex_lock(&cmamem_dev.cmamem_lock);
		res = ispmem_cma_alloc(filp, arg);
		if (res < 0)
			LOG(LOG_ERR, "alloc error!");
		mutex_unlock(&cmamem_dev.cmamem_lock);
		break;
	case CMEM_FREE:
		mutex_lock(&cmamem_dev.cmamem_lock);
		res = ispmem_cma_free(filp, arg);
		if (res < 0)
			LOG(LOG_ERR, "free error!");
		mutex_unlock(&cmamem_dev.cmamem_lock);
		break;
	case CAM_HW_BUFFER_MAP: {
		res = cam_mem_buf_map(arg);
		break;
	}
	case CMEM_CMA_IMPORT: {
		res = ispmem_cma_import(filp, arg);
		break;
	}
	case CAM_HW_BUFFER_RELEASE: {
		res = cam_mem_buf_release(arg);
		break;
	}
#ifdef CONFIG_ARENA_FPGA_PLATFORM
	case ARMCB_VIDIOC_ISP_XDMA:
		res = ispmem_xdma((void *)arg);
		break;
	case ARMCB_VIDIOC_ISP_XDMA_TEST:
		res = ispmem_xdma_test((void *)arg);
		break;
#endif
	default:
		LOG(LOG_ERR, "unkonw ioctl 0x%x", arg);
		res = -EINVAL;
		break;
	}

	return res;
}

#ifdef CONFIG_ARENA_FPGA_PLATFORM
#define DISP_DDR_BUFFER (1920 * 1080)
#define DISP_BUFFER_NUM (12)

static int ispmem_clear_disp_buffer(void)
{
	int res = 0;
#if __SIZEOF_POINTER__ == 4
	void *pKerAddr = NULL;
	dma_addr_t dmaHdl = 0;
	size_t bufLens = 0;
	int i = 0;

	bufLens = PAGE_ALIGN(DISP_DDR_BUFFER);

	/// 1 malloc 1M DMA Buffer
	pKerAddr = dma_alloc_coherent(cmamem_dev.pddev, bufLens, &dmaHdl,
					  GFP_KERNEL);
	if (!pKerAddr) {
		LOG(LOG_ERR, "dma alloc fail!");
		res = -ENOMEM;
		goto exit;
	}

	memset(pKerAddr, 0, bufLens);
	for (i = 0; i < DISP_BUFFER_NUM; i++) {
		res = xdma(DISPBUF_BASE + i * DISP_DDR_BUFFER, (u32)dmaHdl,
			   DISP_DDR_BUFFER, 1);
		if (res < 0) {
			LOG(LOG_ERR, "xdma failed !");
			goto exit;
		}
	}

exit:
	if (pKerAddr)
		dma_free_coherent(cmamem_dev.pddev, bufLens, pKerAddr, dmaHdl);

	mdelay(10);
#endif

	return res;
}
#endif

static int ispmem_release(struct inode *node, struct file *file)
{
#ifdef CONFIG_ARENA_FPGA_PLATFORM
	ispmem_clear_disp_buffer();
#endif
	ispmem_cma_free_all();

	mem_block_count = 0;

	return 0;
}

static const struct file_operations ispmem_dev_fops = {
	.owner = THIS_MODULE,
	.open = ispmem_open,
	.unlocked_ioctl = ispmem_ioctl,
	.release = ispmem_release,
	.mmap = ispmem_cma_mmap,
};

static struct miscdevice ispmem_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &ispmem_dev_fops,
};

static int armcb_ispmem_probe(struct platform_device *pdev)
{
	int res = -1;
	struct iommu_group *group = NULL;
	struct reset_control *reset = NULL;
	struct device *dev = &pdev->dev;

	if (misc_register(&ispmem_misc)) {
		LOG(LOG_ERR, "failed to register ispmem driver.");
		return -ENODEV;
	}

	cam_mem_info =
		devm_kzalloc(&pdev->dev, sizeof(*cam_mem_info), GFP_KERNEL);
	if (!cam_mem_info) {
		LOG(LOG_ERR, "failed to alloc cam_mem_info.");
		return -ENOMEM;
	}

	/* get resources of CLK and reset . */
	cam_mem_info->isp_aclk = devm_clk_get_optional(dev, "isp_aclk");
	if (IS_ERR(cam_mem_info->isp_aclk)) {
		LOG(LOG_ERR, "failed to get isp aclk\n");
		return PTR_ERR(cam_mem_info->isp_aclk);
	}

	cam_mem_info->isp_sclk = devm_clk_get_optional(dev, "isp_sclk");
	if (IS_ERR(cam_mem_info->isp_sclk)) {
		LOG(LOG_ERR, "failed to get isp sclk\n");
		return PTR_ERR(cam_mem_info->isp_sclk);
	}

	reset = devm_reset_control_get(dev, "isp_sreset");
	if (IS_ERR(reset)) {
		LOG(LOG_ERR, "Failed to get isp sreset control\n");
		return PTR_ERR(reset);
	}
	cam_mem_info->isp_sreset = reset;

	reset = devm_reset_control_get(dev, "isp_areset");
	if (IS_ERR(reset)) {
		LOG(LOG_ERR, "Failed to get isp areset reset control\n");
		return PTR_ERR(reset);
	}
	cam_mem_info->isp_areset = reset;

	reset = devm_reset_control_get(dev, "isp_hreset");
	if (IS_ERR(reset)) {
		LOG(LOG_ERR, "Failed to get isp hreset reset control\n");
		return PTR_ERR(reset);
	}
	cam_mem_info->isp_hreset = reset;

	reset = devm_reset_control_get(dev, "isp_gdcreset");
	if (IS_ERR(reset)) {
		LOG(LOG_ERR, "Failed to get isp gdcreset reset control\n");
		return PTR_ERR(reset);
	}
	cam_mem_info->isp_gdcreset = reset;

#ifndef ARMCB_CAM_DEBUG
#ifdef ARMCB_CAM_AHB
	res = fwnode_property_read_u32(pdev->dev.fwnode, "ahb-pmctrl-res-base",
					   &cam_mem_info->ahb_pmctrl_res_base);
	if (res < 0) {
		cam_mem_info->ahb_pmctrl_res_base = 0;
		LOG(LOG_ERR, "failed to get ahb-pmctrl-res-base.");
	}
	res = fwnode_property_read_u32(pdev->dev.fwnode, "ahb-pmctrl-res-size",
					   &cam_mem_info->ahb_pmctrl_res_size);
	if (res < 0) {
		cam_mem_info->ahb_pmctrl_res_size = 0;
		LOG(LOG_ERR, "failed to get ahb-pmctrl-res-size.");
	}

	res = fwnode_property_read_u32(pdev->dev.fwnode,
					   "ahb-rcsuisp0-res-base",
					   &cam_mem_info->ahb_rcsuisp0_res_base);
	if (res < 0) {
		cam_mem_info->ahb_rcsuisp0_res_base = 0;
		LOG(LOG_ERR, "failed to get ahb-rcsuisp0-res-base.");
	}
	res = fwnode_property_read_u32(pdev->dev.fwnode,
					   "ahb-rcsuisp0-res-size",
					   &cam_mem_info->ahb_rcsuisp0_res_size);
	if (res < 0) {
		cam_mem_info->ahb_rcsuisp0_res_size = 0;
		LOG(LOG_ERR, "failed to get ahb-rcsuisp0-res-size.");
	}
	res = fwnode_property_read_u32(pdev->dev.fwnode,
					   "ahb-rcsuisp1-res-base",
					   &cam_mem_info->ahb_rcsuisp1_res_base);
	if (res < 0) {
		cam_mem_info->ahb_rcsuisp1_res_base = 0;
		LOG(LOG_ERR, "failed to get ahb-rcsuisp1-res-base.");
	}
	res = fwnode_property_read_u32(pdev->dev.fwnode,
					   "ahb-rcsuisp1-res-size",
					   &cam_mem_info->ahb_rcsuisp1_res_size);
	if (res < 0) {
		cam_mem_info->ahb_rcsuisp1_res_size = 0;
		LOG(LOG_ERR, "failed to get ahb-rcsuisp1-res-size.");
	}
#else
	res = fwnode_property_read_u32(pdev->dev.fwnode, "apb2-base",
					   &cam_mem_info->apb2_base);
	if (res < 0) {
		cam_mem_info->apb2_base = 0;
		LOG(LOG_ERR, "failed to get apb2-base.");
	}

	res = fwnode_property_read_u32(pdev->dev.fwnode, "apb2-size",
					   &cam_mem_info->apb2_size);
	if (res < 0) {
		cam_mem_info->apb2_size = 0;
		LOG(LOG_ERR, "failed to get apb2-size.");
	}
#endif
#endif

#ifndef QEMU_ON_VEXPRESS
#ifdef ARMCB_CAM_AHB
	if (cam_mem_info->ahb_pmctrl_res_base != 0 &&
		cam_mem_info->ahb_pmctrl_res_size != 0) {
		cam_mem_info->ahb_pmctrl_res_base_addr = devm_ioremap(
			&pdev->dev, cam_mem_info->ahb_pmctrl_res_base,
			cam_mem_info->ahb_pmctrl_res_size);
		if (!cam_mem_info->ahb_pmctrl_res_base_addr) {
			cam_mem_info->ahb_pmctrl_res_base_addr = 0;
			LOG(LOG_WARN,
				"failed to ioremap ahb-pmctrl_res register region.");
		}
	}
	LOG(LOG_INFO,
		"ahb-pmctrl_res-base=0x%x, ahb-pmctrl_res-size=0x%x "
		"ahb_pmctrl_res_base_addr=%p",
		cam_mem_info->ahb_pmctrl_res_base,
		cam_mem_info->ahb_pmctrl_res_size,
		cam_mem_info->ahb_pmctrl_res_base_addr);

	if (cam_mem_info->ahb_rcsuisp0_res_base != 0 &&
		cam_mem_info->ahb_rcsuisp0_res_size != 0) {
		cam_mem_info->ahb_rcsuisp0_res_base_addr = devm_ioremap(
			&pdev->dev, cam_mem_info->ahb_rcsuisp0_res_base,
			cam_mem_info->ahb_rcsuisp0_res_size);
		if (!cam_mem_info->ahb_rcsuisp0_res_base_addr) {
			cam_mem_info->ahb_rcsuisp0_res_base_addr = 0;
			LOG(LOG_WARN,
				"failed to ioremap ahb-rcsuisp0_res register region.");
		}
	}
	LOG(LOG_INFO,
		"ahb-rcsuisp0_res-base=0x%x, ahb-rcsuisp0_res-size=0x%x "
		"ahb_rcsuisp0_res_base_addr=%p",
		cam_mem_info->ahb_rcsuisp0_res_base,
		cam_mem_info->ahb_rcsuisp0_res_size,
		cam_mem_info->ahb_rcsuisp0_res_base_addr);
	if (cam_mem_info->ahb_rcsuisp1_res_base != 0 &&
		cam_mem_info->ahb_rcsuisp1_res_size != 0) {
		cam_mem_info->ahb_rcsuisp1_res_base_addr = devm_ioremap(
			&pdev->dev, cam_mem_info->ahb_rcsuisp1_res_base,
			cam_mem_info->ahb_rcsuisp1_res_size);
		if (!cam_mem_info->ahb_rcsuisp1_res_base_addr) {
			cam_mem_info->ahb_rcsuisp1_res_base_addr = 0;
			LOG(LOG_WARN,
				"failed to ioremap ahb-rcsuisp1_res register region.");
		}
	}
	LOG(LOG_INFO,
		"ahb-rcsuisp1_res-base=0x%x, ahb-rcsuisp1_res-size=0x%x "
		"ahb_rcsuisp1_res_base_addr=%p",
		cam_mem_info->ahb_rcsuisp1_res_base,
		cam_mem_info->ahb_rcsuisp1_res_size,
		cam_mem_info->ahb_rcsuisp1_res_base_addr);
#else
	if (cam_mem_info->apb2_base != 0 && cam_mem_info->apb2_size != 0) {
		cam_mem_info->apb2_base_addr =
			devm_ioremap(&pdev->dev, cam_mem_info->apb2_base,
					 cam_mem_info->apb2_size);
		if (!cam_mem_info->apb2_base_addr) {
			cam_mem_info->apb2_base_addr = 0;
			LOG(LOG_WARN,
				"failed to ioremap apb2 register region.");
		}
	}
	LOG(LOG_INFO, "apb2-base=0x%x, apb2-size=0x%x apb2_base_addr=%p",
		cam_mem_info->apb2_base, cam_mem_info->apb2_size,
		cam_mem_info->apb2_base_addr);
#endif
#ifdef CONFIG_ARENA_FPGA_PLATFORM
	res = fwnode_property_read_u32(pdev->dev.fwnode, "cdma-base",
					   &cam_mem_info->cdma_base);
	if (res < 0) {
		cam_mem_info->cdma_base = 0;
		LOG(LOG_ERR, "failed to get cdma-base.");
	}

	res = fwnode_property_read_u32(pdev->dev.fwnode, "cdma-size",
					   &cam_mem_info->cdma_size);
	if (res < 0) {
		cam_mem_info->cdma_size = 0;
		LOG(LOG_ERR, "failed to get cdma-size.");
	}

	if (cam_mem_info->cdma_base != 0 && cam_mem_info->cdma_size != 0) {
		cam_mem_info->cdma_base_addr =
			devm_ioremap(&pdev->dev, cam_mem_info->cdma_base,
					 cam_mem_info->cdma_size);
		if (!cam_mem_info->cdma_base_addr) {
			cam_mem_info->cdma_base_addr = 0;
			LOG(LOG_WARN, "failed to ioremap axi register region.");
		}
	}
#endif
#endif

	cam_mem_info->pddev = &pdev->dev;

	platform_set_drvdata(pdev, cam_mem_info->pddev);

	mutex_init(&cmamem_dev.cmamem_lock);
	cmamem_dev.count = 0;
	cmamem_dev.pddev = &pdev->dev;
	cmamem_block_head = devm_kzalloc(
		&pdev->dev, sizeof(struct cmamem_block), GFP_KERNEL);
	pdev->dev.dma_parms = &cmamem_block_head->dma_parms;
	dma_set_max_seg_size(&pdev->dev, SEG_SIZE);
	cmamem_block_head->id = -1;
	mem_block_count = 0;
	INIT_LIST_HEAD(&cmamem_block_head->memqueue_list);
	if (has_acpi_companion(&pdev->dev)) {
	/*
	 * Now no memory has reserved for aeu.
	 * if need to request a dma reserved memory for aeu,
	 * just add reserved range[base, size] to reserved_memory.c
	 * and Dsdt-ResLookup.asl
	 */
		if (pdev->dev.dma_mem)
			res = 0;
		else
			res = -ENODEV;
	} else {
		res = of_reserved_mem_device_init(&pdev->dev);
	}

	if (res)
		LOG(LOG_WARN, "fail to get reserved memory (%d)\n", res);

	group = iommu_group_get(cmamem_dev.pddev);
	if (group) {
		cmamem_dev.has_iommu = true;
		iommu_group_put(group);
	}

	LOG(LOG_INFO, " ISP is%s behind an IOMMU",
		cmamem_dev.has_iommu ? "" : " not");

	res = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (res)
		LOG(LOG_WARN, "dma_set_mask_and_coherent failed (%d)\n", res);

	if (cmamem_dev.has_iommu) {
		res = of_dma_configure(cmamem_dev.pddev,
					   cmamem_dev.pddev->of_node, true);
		if (res)
			LOG(LOG_WARN, "dma configure failed (%d)\n", res);
	}

	cmamem_status.status = UNKNOW_STATUS;
	cmamem_status.id_count = -1;
	cmamem_status.vir_addr = 0;
	cmamem_status.phy_addr = 0;

	mutex_init(&buf_tbl.m_lock);
	res = cam_mem_tbl_init();
	pm_runtime_enable(dev);

	LOG(LOG_INFO, "function exit: %d", res);
	return res;
}

static int armcb_ispmem_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	ispmem_cma_free_all();
	ret = cam_mem_release_all();
	pm_runtime_disable(dev);
	mutex_destroy(&buf_tbl.m_lock);
	misc_deregister(&ispmem_misc);
	return ret;
}

static const struct of_device_id armcb_ispmem_dt_match[] = {
	{ .compatible = "armcb,isp-mem" },
	{}
};
MODULE_DEVICE_TABLE(of, armcb_ispmem_dt_match);

static const struct acpi_device_id armcb_ispmem_acpi_match[] = {
	{ .id = "CIXH3025", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, armcb_ispmem_acpi_match);

#ifdef CONFIG_PM

static int armcb_ispmem_dev_rpm_suspend(struct device *dev)
{
	LOG(LOG_INFO, "function enter");

	if (!cam_mem_info->isp_sreset || !cam_mem_info->isp_areset ||
		!cam_mem_info->isp_hreset || !cam_mem_info->isp_gdcreset ||
		!cam_mem_info->isp_sclk || !cam_mem_info->isp_aclk)
		return -EINVAL;

	clk_disable_unprepare(cam_mem_info->isp_sclk);
	clk_disable_unprepare(cam_mem_info->isp_aclk);
	mdelay(1);
	reset_control_assert(cam_mem_info->isp_gdcreset);
	reset_control_assert(cam_mem_info->isp_hreset);
	reset_control_assert(cam_mem_info->isp_sreset);
	reset_control_assert(cam_mem_info->isp_areset);
	LOG(LOG_INFO, "function exit");
	return 0;
}

static int armcb_ispmem_dev_rpm_resume(struct device *dev)
{
	int ret;

	LOG(LOG_INFO, "function enter");
	if (!cam_mem_info->isp_sreset || !cam_mem_info->isp_areset ||
		!cam_mem_info->isp_hreset || !cam_mem_info->isp_gdcreset ||
		!cam_mem_info->isp_sclk || !cam_mem_info->isp_aclk)
		return -EINVAL;

	ret = clk_prepare_enable(cam_mem_info->isp_sclk);
	if (ret < 0) {
		LOG(LOG_ERR, "enable isp_sclk error");
		return ret;
	}
	ret = clk_prepare_enable(cam_mem_info->isp_aclk);
	if (ret < 0) {
		LOG(LOG_ERR, "enable isp_aclk error");
		return ret;
	}
	mdelay(1);
	reset_control_assert(cam_mem_info->isp_sreset);
	reset_control_assert(cam_mem_info->isp_areset);
	reset_control_assert(cam_mem_info->isp_hreset);
	reset_control_assert(cam_mem_info->isp_gdcreset);
	mdelay(1);
	reset_control_deassert(cam_mem_info->isp_sreset);
	reset_control_deassert(cam_mem_info->isp_areset);
	reset_control_deassert(cam_mem_info->isp_hreset);
	reset_control_deassert(cam_mem_info->isp_gdcreset);

	LOG(LOG_INFO, "function exit");
	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP
static int armcb_ispmem_dev_suspend(struct device *dev)
{
	LOG(LOG_INFO, "function enter");
	return pm_runtime_force_suspend(dev);
}

static int armcb_ispmem_dev_resume(struct device *dev)
{
	LOG(LOG_INFO, "function enter");
	return pm_runtime_force_resume(dev);
}
#endif

static const struct dev_pm_ops armcb_ispmem_dev_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(armcb_ispmem_dev_suspend,
				armcb_ispmem_dev_resume)
#endif
#ifdef CONFIG_PM
		SET_RUNTIME_PM_OPS(armcb_ispmem_dev_rpm_suspend,
				   armcb_ispmem_dev_rpm_resume, NULL)
#endif
};

static struct platform_driver armcb_ispmem_driver = {
	.probe = armcb_ispmem_probe,
	.remove = armcb_ispmem_remove,
	.driver = {
		.name = "armcb,isp-mem",
		.owner = THIS_MODULE,
		.of_match_table = armcb_ispmem_dt_match,
		.acpi_match_table = ACPI_PTR(armcb_ispmem_acpi_match),
		.pm = &armcb_ispmem_dev_pm_ops,
	},
};

#ifndef ARMCB_CAM_KO
static int __init armcb_ispmem_module_init(void)
{
	if (platform_driver_register(&armcb_ispmem_driver)) {
		LOG(LOG_ERR, "failed to register armcb ispmem driver");
		return -ENODEV;
	}

	return 0;
}

static void __exit armcb_ispmem_module_exit(void)
{
	platform_driver_unregister(&armcb_ispmem_driver);
}
#endif

#ifndef ARMCB_CAM_KO
module_init(armcb_ispmem_module_init);
module_exit(armcb_ispmem_module_exit);

MODULE_ALIAS("platform:armchina");
MODULE_DESCRIPTION("Arm China ispmem driver");
MODULE_LICENSE("GPL v2");
#else
MODULE_IMPORT_NS(DMA_BUF);
static void *g_instance;

void *armcb_get_cam_io_drv_instance(void)
{
	if (platform_driver_register(&armcb_ispmem_driver) < 0) {
		LOG(LOG_ERR, "register cam io drv failed.\n");
		return NULL;
	}
	g_instance = (void *)&armcb_ispmem_driver;
	return g_instance;
}

void armcb_cam_io_drv_destroy(void)
{
	if (g_instance)
		platform_driver_unregister(
			(struct platform_driver *)g_instance);
}
#endif

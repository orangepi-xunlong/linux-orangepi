/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef __ARMCB_ISPMEM_H__
#define __ARMCB_ISPMEM_H__
#include "armcb_isp.h"
#include "types_utils.h"
#include <linux/clk.h>
#include <linux/dma-buf.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/videobuf2-v4l2.h>

#define DISP_BUFFER_SIZE_1080P (2200 * 1125 * 4 * 3)

#define ISP_PLL_CLK 1200

enum sys_ispmem_type {
	SYS_ISPMEM_CMA_XDMA_CTRL_REG,
	SYS_ISPMEM_CMA_DUMP_CTRL_REG,
	SYS_ISPMEM_REG_TYPE_NUM_MAX
};

struct sys_ispmem_param {
	enum sys_ispmem_type memtype;
	unsigned int phyaddr_base;
	unsigned int kvirt_addr;
	char *puvirt_addr;
	unsigned int buf_size;
	void *arg;
};

struct cma_mem_info {
	unsigned long size;
	dma_addr_t dma;
	void *virt;
};

struct armcb_ispmem_info {
	struct device *pddev;

	struct clk *isp_aclk;
	struct clk *isp_sclk;
	struct reset_control *isp_sreset;
	struct reset_control *isp_areset;
	struct reset_control *isp_hreset;
	struct reset_control *isp_gdcreset;

#ifdef ARMCB_CAM_AHB
	unsigned int ahb_pmctrl_res_base;
	unsigned int ahb_pmctrl_res_size;

	unsigned int ahb_rcsuisp0_res_base;
	unsigned int ahb_rcsuisp0_res_size;
	unsigned int ahb_rcsuisp1_res_base;
	unsigned int ahb_rcsuisp1_res_size;
#endif
	unsigned int apb2_base;
	unsigned int apb2_size;
	unsigned int cdma_base;
	unsigned int cdma_size;
#ifdef ARMCB_CAM_AHB
	void __iomem *ahb_pmctrl_res_base_addr;
	void __iomem *ahb_rcsuisp0_res_base_addr;
	void __iomem *ahb_rcsuisp1_res_base_addr;
#endif
	void __iomem *apb2_base_addr;
	void __iomem *cdma_base_addr;
};

enum cma_status {
	UNKNOW_STATUS = 0,
	HAVE_ALLOCED = 1,
	HAVE_MMAPED = 2,
};

struct cmamem_dev {
	unsigned int count;
	bool has_iommu;
	struct device *pddev;
	struct mutex cmamem_lock;
};

struct cmamem_block {
	unsigned char name[10];
	unsigned char is_use_buffer;
	unsigned char is_free;
	int id;
	unsigned int offset;
	unsigned int len;
	unsigned long phy_addr;
	unsigned long usr_addr;
	void *kernel_addr;
	struct list_head memqueue_list;
	struct device_dma_parameters dma_parms;
};

struct current_status {
	int status;
	int id_count;
	void *vir_addr;
	dma_addr_t phy_addr;
};

#define CAM_MEM_BUFQ_MAX 1024

struct cam_mem_buf {
	struct dma_buf *dma;
	int fd;
	int bufIdx;
	unsigned char align;
	unsigned int flags;
	unsigned int len;
	unsigned long phy_addr; /// physical address
	unsigned long kvaddr; /// kernel virtual address
	struct dma_buf_attachment *attachment;
	struct sg_table *table;
};

struct cam_buf_table {
	struct mutex m_lock;
	/// @TODO use bitmap replace
	unsigned char bitMap[CAM_MEM_BUFQ_MAX];
	struct cam_mem_buf bufq[CAM_MEM_BUFQ_MAX];
};

#define XST_SUCCESS 0L
#define XST_FAILURE 1L

// This range in XC7Z020 CPU address domain is mapped to XCVU440's DDR memory.
// XC7Z020 CPU must access this range by CDMA.
// CDMA in XC7020 PL side is configured as this address mapping.
//
#define MEM_START_440 0x40000000
#define MEM_END_440 0x7FFFFFFF

//-- Display buffer on XCVU440.
//
#define DISPBUF_BASE MEM_START_440

#ifdef ARMCB_CAM_AHB
u32 armcb_ahb_pmctrl_res_read_reg(u32 offset);
void armcb_ahb_pmctrl_res_write_reg(u32 offset, u32 value);

u32 armcb_ahb_rcsuisp0_res_read_reg(u32 offset);
void armcb_ahb_rcsuisp0_res_write_reg(u32 offset, u32 value);

u32 armcb_ahb_rcsuisp1_res_read_reg(u32 offset);
void armcb_ahb_rcsuisp1_res_write_reg(u32 offset, u32 value);
#endif
u32 armcb_apb2_read_reg(u32 offset);
void armcb_apb2_write_reg(u32 offset, u32 value);

void __iomem *armcb_ispmem_get_vdma_base(void);
void __iomem *armcb_ispmem_get_cdma_base(void);
void __iomem *armcb_ispmem_get_vdmaisp_base(void);
void __iomem *armcb_ispmem_get_xvtc_base(void);
void __iomem *armcb_ispmem_get_vdmasensor_base(void);
#ifdef ARMCB_CAM_AHB

void __iomem *armcb_ahb_pmctrl_res_get_addr_base(void);

void __iomem *armcb_ahb_rcsuisp0_res_get_addr_base(void);
void __iomem *armcb_ahb_rcsuisp1_res_get_addr_base(void);
#endif
void __iomem *armcb_apb2_get_addr_base(void);
void __iomem *armcb_spi_get_addr_base(void);

#ifdef ARMCB_CAM_KO
void *armcb_get_cam_io_drv_instance(void);
void armcb_cam_io_drv_destroy(void);
#endif

#endif

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __JPU_DRV_H__
#define __JPU_DRV_H__

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/dma-buf.h>
#include <linux/highmem.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/iommu.h>

#define JDI_IOCTL_MAGIC  'J'

#define JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY          _IO(JDI_IOCTL_MAGIC, 0)
#define JDI_IOCTL_FREE_PHYSICALMEMORY               _IO(JDI_IOCTL_MAGIC, 1)
#define JDI_IOCTL_WAIT_INTERRUPT                    _IO(JDI_IOCTL_MAGIC, 2)
#define JDI_IOCTL_SET_CLOCK_GATE                    _IO(JDI_IOCTL_MAGIC, 3)
#define JDI_IOCTL_RESET                             _IO(JDI_IOCTL_MAGIC, 4)
#define JDI_IOCTL_GET_INSTANCE_POOL                 _IO(JDI_IOCTL_MAGIC, 5)
#define JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO    _IO(JDI_IOCTL_MAGIC, 6)
#define JDI_IOCTL_GET_REGISTER_INFO                 _IO(JDI_IOCTL_MAGIC, 7)
#define JDI_IOCTL_OPEN_INSTANCE                     _IO(JDI_IOCTL_MAGIC, 8)
#define JDI_IOCTL_CLOSE_INSTANCE                    _IO(JDI_IOCTL_MAGIC, 9)
#define JDI_IOCTL_GET_INSTANCE_NUM                  _IO(JDI_IOCTL_MAGIC, 10)
#define JDI_IOCTL_CFG_MMU                     		_IO(JDI_IOCTL_MAGIC, 11)
#define JDI_IOCTL_RELEASE_MMU                     	_IO(JDI_IOCTL_MAGIC, 12)

enum {
	INT_JPU_DONE = 0,
	INT_JPU_ERROR = 1,
	INT_JPU_BIT_BUF_EMPTY = 2,
	INT_JPU_BIT_BUF_FULL = 2,
	INT_JPU_OVERFLOW,
	INT_JPU_PARTIAL_BUFFER_0,
	INT_JPU_PARTIAL_BUFFER_1,
	INT_JPU_PARTIAL_BUFFER_2,
	INT_JPU_PARTIAL_BUFFER_3,
	INT_JPU_STOP,
	INT_JPU_CFG_DONE,
	INT_JPU_SOF,
};

typedef struct jpudrv_buffer_t {
	unsigned int size;
	unsigned long phys_addr;
	unsigned long base;	/* kernel logical address in use kernel */
	unsigned long virt_addr;	/* virtual user space address */
} jpudrv_buffer_t;

typedef struct jpudrv_inst_info_t {
	unsigned int inst_idx;
	int inst_open_count;	/* for output only */
} jpudrv_inst_info_t;

typedef struct jpudrv_intr_info_t {
	unsigned int timeout;
	int intr_reason;
	unsigned int inst_idx;
} jpudrv_intr_info_t;

typedef struct jpu_dma_buf_info {
	struct dma_buf *dmabuf;
	int buf_fd;
	struct dma_buf_attachment *attach;
	struct sg_table *sgtable;
	int tbu_id;
	u32 append_buf_size;	//append buffer to workarourd when picture size is not 16 align 
} jpu_dma_buf_info;
typedef struct jpu_dma_cfg {
	int intput_buf_fd;
	int output_buf_fd;
	unsigned int intput_virt_addr;
	unsigned int output_virt_addr;
	unsigned int data_size;
	unsigned int append_buf_size;
} JPU_DMA_CFG;
struct tbu_instance {
	int ins_id;
	u32 *ttb_va;
	dma_addr_t ttb_pa;
	u64 ttb_size;
	u64 va_base;
	u64 va_end;
	bool always_preload;
	bool enable_preload;
	u32 nsaid;
	u32 qos;
	bool secure_enable;
};
#endif

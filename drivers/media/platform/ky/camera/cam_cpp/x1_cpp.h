/* SPDX-License-Identifier: GPL-2.0 */
/*
 * x1_cpp.h - Driver for KY X1 Camera Post Process
 * lizhirong <zhirong.li@ky.com>
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __X1_CPP_H__
#define __X1_CPP_H__

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <media/x1/x1_cpp_uapi.h>
#include <linux/reset.h>
#include "cam_plat.h"
#include "cpp_dmabuf.h"
#include "cpp_iommu.h"

#define MAX_ACTIVE_CPP_INSTANCE (1)
#define MAX_CPP_V4L2_EVENTS (8)

enum cpp_state {
	CPP_STATE_OFF,
	CPP_STATE_IDLE,
	CPP_STATE_ACTIVE,
	CPP_STATE_RST,
	CPP_STATE_ERR,
};

struct cpp_queue_cmd {
	struct list_head list_frame;
	u64 ts_reg_config;
	u64 ts_frm_trigger;
	u64 ts_frm_finish;
	u32 frame_id;
	u32 client_id;
	atomic_t in_processing;
	// struct cpp_frame_info frame_info;
	struct cpp_reg_cfg_cmd hw_cmds[MAX_REG_CMDS];
	struct cpp_dma_port_info *dma_ports[MAX_DMA_PORT];
};

struct device_queue {
	struct list_head list;
	spinlock_t lock;
	int max;
	int len;
	const char *name;
};

struct cpp_tasklet_event {
	u8 used;
	u32 irq_status;
};

struct cpp_run_work {
	struct workqueue_struct *run_wq;
	struct work_struct work;
	struct completion run_complete;
};

/* Instance is already queued on the job_queue */
#define TRANS_QUEUED (1 << 0)
/* Instance is currently running in hardware */
#define TRANS_RUNNING (1 << 1)
/* Instance is currently aborting */
#define TRANS_ABORT (1 << 2)

struct cpp_device;

struct cpp_ctx {
	struct cpp_device *cpp_dev;
	struct device_queue idleq;
	struct device_queue frmq;

	/* For device job queue */
	struct list_head queue;
	unsigned long job_flags;
};

struct cpp_hw_ops {
	int (*global_reset)(struct cpp_device *vfe);
	void (*enable_clk_gating)(struct cpp_device *cpp, u8 enable);
	int (*set_burst_len)(struct cpp_device *cpp);
	void (*enable_irqs_common)(struct cpp_device *cpp, u8 enable);
	irqreturn_t (*isr)(int irq, void *data);
	void (*debug_dump)(struct cpp_device *cpp);
	int (*cfg_port_dmad)(struct cpp_device *cpp,
			     struct cpp_dma_port_info *port_info, u8 port_id);
	u32 (*hw_version)(struct cpp_device *cpp);
};

struct cpp_device {
	struct platform_device *pdev;
	struct plat_cam_subdev csd;
	struct resource *mem;
	struct resource *irq;
	void __iomem *regs_base;
//	struct clk *ahb_clk;
	struct reset_control *ahb_reset;
	struct reset_control *isp_cpp_reset;
	struct reset_control *isp_ci_reset;
	struct reset_control *lcd_mclk_reset;

	struct clk *fnc_clk;
	struct clk *bus_clk;
	struct clk *dpu_clk;
	struct mutex mutex;
	struct completion reset_complete;
	u32 open_cnt;
	enum cpp_state state;
	u8 mapped;
	void *shared_mem;
	unsigned long shared_size;

	struct cpp_hw_info hw_info;
	struct cpp_run_work run_work;

	spinlock_t job_spinlock;
	struct list_head job_queue;
	struct cpp_ctx *curr_ctx;
	struct cpp_ctx priv;

	const struct cpp_hw_ops *ops;

	struct cpp_iommu_device *mmu_dev;
};

extern const struct cpp_hw_ops cpp_ops_2_0;

static inline u32 cpp_reg_read_relaxed(struct cpp_device *cpp_dev, u32 reg)
{
	return readl_relaxed(cpp_dev->regs_base + reg);
}

static inline void cpp_reg_write_relaxed(struct cpp_device *cpp_dev, u32 reg, u32 val)
{
	if (0xffff8000 & reg) {	/* block reg violation */
		pr_err("reg write relaxed violation, 0x%x", reg);
		return;
	}
	writel_relaxed(val, cpp_dev->regs_base + reg);
}

static inline u64 cpp_reg_read64(struct cpp_device *cpp_dev, u32 lower, u32 upper)
{
	u64 val;
	val = (u64)ioread32(cpp_dev->regs_base + upper) << 32;
	val += (u64)ioread32(cpp_dev->regs_base + lower);
	return val;
}

static inline u32 cpp_reg_read(struct cpp_device *cpp_dev, u32 reg)
{
	return ioread32(cpp_dev->regs_base + reg);
}

static inline void cpp_reg_write(struct cpp_device *cpp_dev, u32 reg, u32 val)
{
	iowrite32(val, cpp_dev->regs_base + reg);
}

static inline void cpp_reg_write_mask(struct cpp_device *cpp_dev, u32 reg,
				      u32 val, u32 mask)
{
	u32 v;

	if (!mask)
		return;

	v = cpp_reg_read(cpp_dev, reg);
	v = (v & ~mask) | (val & mask);
	cpp_reg_write(cpp_dev, reg, v);
}

static inline void cpp_reg_set_bit(struct cpp_device *cpp_dev, u32 reg, u32 val)
{
	cpp_reg_write_mask(cpp_dev, reg, val, val);
}

static inline void cpp_reg_clr_bit(struct cpp_device *cpp_dev, u32 reg, u32 val)
{
	cpp_reg_write_mask(cpp_dev, reg, 0, val);
}

#endif /* ifndef __X1_CPP_H__ */

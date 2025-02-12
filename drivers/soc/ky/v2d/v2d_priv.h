// SPDX-License-Identifier: GPL-2.0
#ifndef __KY_V2D_PRIV_H__
#define __KY_V2D_PRIV_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/genalloc.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/iommu.h>
#include <linux/pm_qos.h>
#include <linux/reset.h>

#define V2D_PRINT_DEBUG
#define V2D_FALSE 0
#define V2D_TRUE  1

#ifdef V2D_PRINT_ERROR
#define V2D_LOG_LEVEL_ERROR
#endif

#ifdef V2D_PRINT_WARNING
#define V2D_LOG_LEVEL_ERROR
#define V2D_LOG_LEVEL_WARNING
#endif

#ifdef V2D_PRINT_INFO
#define V2D_LOG_LEVEL_ERROR
#define V2D_LOG_LEVEL_WARNING
#define V2D_LOG_LEVEL_INFO
#endif

#ifdef V2D_PRINT_DEBUG
#define V2D_LOG_LEVEL_ERROR
#define V2D_LOG_LEVEL_WARNING
#define V2D_LOG_LEVEL_INFO
#define V2D_LOG_LEVEL_DEBUG
#endif

#ifdef V2D_LOG_LEVEL_ERROR
#define V2DLOGE(fmt, ...) pr_err(fmt, ## __VA_ARGS__)
#else
#define V2DLOGE(fmt, ...)
#endif

#ifdef V2D_LOG_LEVEL_WARNING
#define V2DLOGW(fmt, ...) pr_warn(fmt, ## __VA_ARGS__)
#else
#define V2DLOGW(fmt, ...)
#endif

#ifdef V2D_LOG_LEVEL_INFO
#define V2DLOGI(fmt, ...) pr_info(fmt, ## __VA_ARGS__)
#else
#define V2DLOGI(fmt, ...)
#endif

#ifdef V2D_LOG_LEVEL_DEBUG
#define V2DLOGD(fmt, ...) pr_debug(fmt, ## __VA_ARGS__)
#else
#define V2DLOGD(fmt, ...)
#endif
#define V2D_SHORT_FENCE_TIMEOUT (1 * MSEC_PER_SEC)
#define V2D_LONG_FENCE_TIMEOUT (2 * MSEC_PER_SEC)
#define V2D_DISABLE_BAND_CAL
struct v2d_info {
	struct platform_device  *pdev;
	struct miscdevice       mdev;
	int32_t                 irq;
	void __iomem            *v2dreg_iomem_base;
	struct clk              *clkcore;
	struct clk              *clkio;
	struct reset_control    *v2d_reset;
	int                     refcount;
	int                     do_reset;
	struct mutex            power_mutex;
	spinlock_t              power_spinlock;
	struct work_struct      work;
	struct workqueue_struct *v2d_job_done_wq;
	uint64_t                context;
	atomic_t                seqno;
	struct semaphore        sem_lock;
	struct mutex            client_lock;
	struct list_head        post_list;
	struct mutex            post_lock;
	struct kthread_worker   post_worker;
	struct task_struct      *post_thread;
	struct kthread_work     post_work;
	struct list_head        free_list;
	struct mutex            free_lock;
#if IS_ENABLED(CONFIG_KY_DDR_FC) && defined(CONFIG_PM)
#ifndef V2D_DISABLE_BAND_CAL
	struct ky_bw_con *ddr_qos_cons;
#endif
#endif
#ifdef CONFIG_KY_DEBUG
	bool b_v2d_running;
	bool (*is_v2d_running)(struct v2d_info *pV2dInfo);
	struct notifier_block nb;
#endif
};

#define BASE_VIRTUAL_ADDRESS 0x80000000
#define VA_STEP_PER_TBU 0x2000000
#define MAX_ENTRIES_PER_TTB 8096
#define ENTRY_SIZE 4
#define MAX_SIZE_PER_TTB (MAX_ENTRIES_PER_TTB*ENTRY_SIZE)
#define DEFAULT_TIMEOUT_CYCS 0x80000
#define V2D_MMU_PGSIZE_BITMAP 0x02FFF000 /* 4K~32M */
#define TBU_INSTANCES_NUM (3)
#define TTB_ENTRY_SHIFT 12
#define AQUIRE_TIMEOUT_MS 100

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

struct v2d_iommu_res {
	void __iomem *base;
	u32 time_out_cycs;
	u32 page_size;
	u64 va_base;
	u64 va_end;
	struct tbu_instance tbu_ins[TBU_INSTANCES_NUM];
	int tbu_ins_map;
	bool is_hw_enable;
};
#endif  /* __KY_V2D_PRIV_H__*/

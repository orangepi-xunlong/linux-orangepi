/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ive_interface.h
 * Description: ive driver interface header file
 */

#ifndef __IVE_INTERFACE_H__
#define __IVE_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/version.h>
#include <linux/defines.h>
#include <uapi/linux/sched/types.h>
#include <linux/semaphore.h>
#include <linux/clk.h>


#if defined(__CV186X__)
#define IVE_DEV_MAX 2
#else
#define IVE_DEV_MAX 1
#endif
#define IVE_SYNC_IO_WAIT_TIMEOUT_MS (1000*10)
#define IVE_IDLE_WAIT_TIMEOUT_MS    (1000*5)
#define IVE_EOF_WAIT_TIMEOUT_MS     (1000*1)

enum ive_wait_evt {
	IVE_EVENT_BUSY_OR_NOT_STAT = 0x0,
	IVE_EVENT_WKUP = 0x1,
	IVE_EVENT_EOF =  0x2,
	IVE_EVENT_RST =  0x4,
};

enum ive_core_state {
	IVE_CORE_STATE_IDLE,
	IVE_CORE_STATE_RUNNING,
	IVE_CORE_STATE_END,
};

enum ive_dev_state {
	IVE_DEV_STATE_RUNNING,
	IVE_DEV_STATE_READY,
	IVE_DEV_STATE_END,
};

enum ive_clk_state {
	IVE_IDLE = 0x0,
	IVE_READY_SUSPEND,
	IVE_READY_RESUME,
};

enum ive_dev_type {
	DEV_IVE_0 = 0,
#if defined(__CV186X__)
	DEV_IVE_1,
#endif
	DEV_IVE_MAX,
};

enum ive_task_state {
	IVE_TASK_STATE_READY,
	IVE_TASK_STATE_RUNNING,
	IVE_TASK_STATE_DONE,
};

struct ive_task {
	struct list_head node;// task node
	atomic_t state;// ive_task_state
	unsigned int task_type;
	char *input_data;
	void *buffer;
	int dev_id;
	wait_queue_head_t task_done_wait;
	bool task_done_evt;
};

struct ive_dev_core {
	enum ive_dev_type dev_type;
	int irq_num;
	struct clk *clk_src;
	struct clk *clk_ive;
	spinlock_t dev_lock;
	struct completion frame_done;
	struct completion op_done;
	int cur_optype;
	int tile_num;
	int total_tile;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 ts_start;
	struct timespec64 ts_end;
#else
	struct timespec ts_start;
	struct timespec ts_end;
#endif
	u32 hw_duration;
	u32 hw_duration_total;
	u32 duty_ratio;
//for task func
	atomic_t dev_state;//device_state
	struct ive_task *work_tsk;
	struct work_struct work_frm_done;
	wait_queue_head_t cmdq_wq;
};

struct ive_device {
	struct device *dev;
	struct cdev cdev;
	void __iomem *ive_base[IVE_DEV_MAX]; /* NULL if not initialized. */
	int ive_irq[IVE_DEV_MAX]; /* alarm and periodic irq */
	struct ive_dev_core core[IVE_DEV_MAX];
	// struct proc_dir_entry *proc_dir;
	spinlock_t close_lock;
	struct completion frame_done;
	struct completion op_done;
	struct list_head tsk_list;//task_list
	wait_queue_head_t wait;
	int cur_optype;
	int tile_num;
	int total_tile;
	int use_count;
	uintptr_t *private_data;
	struct task_struct *work_thread;
	enum ive_wait_evt evt;//wait state
	struct semaphore sem;
	atomic_t clk_flg;

};

struct ive_profiling_info {
	char op_name[16];
	int tile_num;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 time_ioctl_start;
	struct timespec64 time_ioctl_end;
	struct timespec64 time_vld_start;
	struct timespec64 time_vld_end;
#else
	struct timespec time_ioctl_start;
	struct timespec time_ioctl_end;
	struct timespec time_vld_start;
	struct timespec time_vld_end;
#endif
	uint32_t time_ioctl_diff_us;
	uint32_t time_vld_diff_us[6];
	uint32_t time_tile_diff_us;
};
typedef void (*ive_timer_cb)(void *data);
void start_vld_time(int optype, struct ive_dev_core *core);
void stop_vld_time(int optype, int tile_num, struct ive_dev_core *core);
void ive_notify_wkup_evt_kth(void *data, enum ive_wait_evt evt);
s32 is_ive_suspend(struct ive_device *ndev);

extern bool __clk_is_enabled(struct clk *clk);
#endif /* __IVE_INTERFACE_H__ */

#ifndef _LDC_CORE_H_
#define _LDC_CORE_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/ldc_uapi.h>
#include <ldc_common.h>

enum ldc_core_state {
	LDC_CORE_STATE_IDLE,
	LDC_CORE_STATE_RUNNING,
	LDC_CORE_STATE_END,
};

enum ldc_dev_state {
	LDC_DEV_STATE_RUNNING,
	LDC_DEV_STATE_STOP,
};

enum ldc_type {
	DEV_LDC_0 = 0,
	DEV_LDC_1,
	DEV_DWA_0,
	DEV_DWA_1,
	DEV_LDC_MAX,
};

enum ldc_wait_evt {
	LDC_EVENT_BUSY_OR_NOT_STAT = 0x0,
	LDC_EVENT_WKUP = 0x1,
	LDC_EVENT_EOF =  0x2,
	LDC_EVENT_RST =  0x4,
};


struct ldc_core {
	spinlock_t core_lock;
	enum ldc_type dev_type;
	int irq_num;
	struct clk *clk_src;
	struct clk *clk_apb;
	struct clk *clk_ldc;
	unsigned int clk_sys_freq;
	atomic_t state;//ldc_core_state
	struct list_head list;
#if LDC_USE_WORKQUEUE
	struct work_struct work_frm_done;
#endif
	wait_queue_head_t cmdq_wq;
	bool cmdq_evt;
};


struct ldc_vdev {
	struct miscdevice miscdev;
	atomic_t state;//ldc_dev_state
	struct ldc_core core[LDC_DEV_MAX_CNT];
	int core_num;
	//atomic_t cur_irq_core_id;
	atomic_t clk_en;
	void *shared_mem;
	struct task_struct *thread;
	wait_queue_head_t wait;
	enum ldc_wait_evt evt;
	struct list_head job_list;
	int job_cnt;
	spinlock_t wdev_lock;
	//struct workqueue_struct *workqueue;
	struct ldc_vb_doneq vb_doneq;
	vb_pool vb_pool;
	struct semaphore sem;
};

struct ldc_vdev *ldc_get_dev(void);
struct fasync_struct *ldc_get_dev_fasync(void);
void ldc_enable_dev_clk(int coreid, bool en);
void ldc_core_init(int top_id);
void ldc_core_deinit(int top_id);
void ldc_dev_init(struct ldc_vdev *dev);
void ldc_dev_deinit(struct ldc_vdev *dev);
int ldc_suspend(struct device *dev);
int ldc_resume(struct device *dev);
bool is_ldc_suspended(void);
void ldc_clk_init(struct ldc_vdev *dev);
void ldc_clk_deinit(struct ldc_vdev *dev);

#endif /* _VIP_LDC_H_ */

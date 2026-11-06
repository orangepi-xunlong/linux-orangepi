#ifndef _STITCH_CORE_H_
#define _STITCH_CORE_H_

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/pm_qos.h>
#include <linux/miscdevice.h>

#include <linux/iommu.h>

#include <linux/defines.h>
#include <linux/stitch_uapi.h>

#include "stitch_ctx.h"


#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

enum stitch_dev_state {
	STITCH_DEV_STATE_IDLE,
	STITCH_DEV_STATE_RUNNING,
	STITCH_DEV_STATE_END,
};

struct stitch_dev {
	struct miscdevice miscdev;
	atomic_t state;//stitch_dev_state
	unsigned int irq_num;
	struct clk *clk_src;
	struct clk *clk_apb;
	struct clk *clk_stitch;
	unsigned int clk_sys_freq;
	spinlock_t lock;
	struct stitch_job *job;
	atomic_t working_grp;
};

atomic_t * stitch_get_open_count(void);
struct stitch_dev * stitch_get_dev(void);
void stitch_enable_dev_clk(bool en);
int stitch_suspend(struct device *dev);
int stitch_resume(struct device *dev);
bool is_stitch_suspended(void);

#endif /* _STITCH_CORE_H_ */

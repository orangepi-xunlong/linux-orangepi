#ifndef __CVI_MON_INTERFACE_H__
#define __CVI_MON_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/dma-direction.h>

struct cvi_mon_work {
	struct task_struct *work_thread;
	wait_queue_head_t task_wait_queue;
	wait_queue_head_t done_wait_queue;
	struct list_head task_list;
	spinlock_t task_list_lock;
	struct list_head done_list;
	spinlock_t done_list_lock;
};

struct cvi_mon_device {
	struct device *dev;
	dev_t cdev_id;
	struct cdev cdev;
	struct completion aximon_completion;
	uint8_t __iomem *pcmon_vaddr;
	uint8_t __iomem *ddr_ctrl_vaddr;		//0x08004000
	uint8_t __iomem *ddr_phyd_vaddr;		//0x08006000
#ifndef CONFIG_ARCH_CV186X
	uint8_t __iomem *ddr_aximon_vaddr;	//0x08008000
#else
	uint8_t __iomem *ddr_aximon_real_vaddr;	//0x08008000
	uint8_t __iomem *ddr_aximon_offline_vaddr;	//0x08008000
	uint8_t __iomem *ddr_aximon_bulk_vaddr;	//0x08008000
	uint8_t __iomem *ddr_ddrmon_sys1_vaddr;
	uint8_t __iomem *ddr_ddrmon_sys2_vaddr;
#endif
	uint8_t __iomem *ddr_top_vaddr;			//0x0800A000
	int aximon_irq;
	struct mutex dev_lock;
	spinlock_t close_lock;
	int use_count;
	void *private_data;
	struct cvi_mon_work mon_work;
};

#endif


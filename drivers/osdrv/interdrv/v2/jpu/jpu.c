//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/of_reserved_mem.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/cma.h>
#include <linux/of.h>
#include "ion.h"
#include <linux/compat.h>

#if KERNEL_VERSION(5, 4, 0) < LINUX_VERSION_CODE
#include <linux/sched/signal.h>
#endif


#include "jpuconfig.h"
#include "jpu.h"

// #define ENABLE_CLK_CONTROL 1

//#define ENABLE_DEBUG_MSG
#ifdef ENABLE_DEBUG_MSG
#define DPRINTK(args, ...)        do { \
    pr_err("%s:%d: " args, __func__, __LINE__, ##__VA_ARGS__); \
} while(0)
#else
#define DPRINTK(args...)
#endif

static struct device *jpu_dev;
/* definitions to be changed as customer  configuration */
/* if you want to have clock gating scheme frame by frame */
//#define JPU_SUPPORT_CLOCK_CONTROL
#define JPU_SUPPORT_ISR
/* if the platform driver knows the name of this driver */
/* JPU_PLATFORM_DEVICE_NAME */
#define JPU_SUPPORT_PLATFORM_DRIVER_REGISTER
#define JPU_SUPPORT_ION_MEMORY

/* if this driver knows the dedicated video memory address */
//#define JPU_SUPPORT_RESERVED_VIDEO_MEMORY        //if this driver knows the dedicated video memory address

#define JPU_PLATFORM_DEVICE_NAME    "sophgo,jpu"
#define JPU_CLK_NAME                "jpege"
#define JPU_DEV_NAME                "jpu"

#define JPEG_TOP_REG				0x21000010
#define JPU_REG_BASE_ADDR           0x75300000
#define JPU_REG_SIZE                0x300


#ifdef JPU_SUPPORT_ISR
#define JPU_IRQ_NUM                 (15+32)
/* if the driver want to disable and enable IRQ whenever interrupt asserted. */
// #define JPU_IRQ_CONTROL
#endif


#ifndef VM_RESERVED	/*for kernel up to 3.7.0 version*/
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

typedef struct jpu_drv_context_t {
	struct fasync_struct *async_queue;
	u32 open_count;                     /*!<< device reference count. Not instance count */
	u32 interrupt_reason[MAX_NUM_INSTANCE];
	u32 dev_id;
	int irq;
} jpu_drv_context_t;


/* To track the allocated memory buffer */
typedef struct jpudrv_buffer_pool_t {
	struct list_head        list;
	struct jpudrv_buffer_t  jb;
	struct file            *filp;
} jpudrv_buffer_pool_t;

/* To track the instance index and buffer in instance pool */
typedef struct jpudrv_instance_list_t {
	struct list_head    list;
	unsigned long       inst_idx;
	struct file        *filp;
} jpudrv_instance_list_t;

typedef struct jpudrv_instance_pool_t {
	unsigned char codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
} jpudrv_instance_pool_t;

#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
//	#define JPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE  (16*1024*1024)
//	#define JPU_DRAM_PHYSICAL_BASE              (0x8AA00000)
#define JPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE  (4*1024*1024*1024)
#define JPU_DRAM_PHYSICAL_BASE              (0x0)
#include "jmm.h"
static jpu_mm_t         s_jmem;
static jpudrv_buffer_t  s_video_memory = {0};
#endif /* JPU_SUPPORT_RESERVED_VIDEO_MEMORY */


static int jpu_hw_reset(void);
static void jpu_clk_disable(struct clk *clk);
static int jpu_clk_enable(struct clk *clk);
static struct clk *jpu_clk_get(struct device *dev);
static void jpu_clk_put(struct clk *clk);
// end customer definition
#define JPU_DEV_MAX (4)

static jpudrv_buffer_t s_instance_pool = {0};
static jpu_drv_context_t s_jpu_drv_context[JPU_DEV_MAX];
static int s_jpu_major;
static struct cdev s_jpu_cdev[JPU_DEV_MAX];
static struct clk *s_jpu_clk;
static int s_jpu_open_ref_count;


static struct class *jpu_class;


static jpudrv_buffer_t s_jpu_register[JPU_DEV_MAX] = {0};

typedef struct _jpu_power_ctrl_ {
	struct reset_control *jpu_rst;
	struct clk           *jpu_clk;
	struct clk *jpu_apb_clk;
	struct clk *jpu_axi_clk;
} jpu_power_ctrl;
static jpu_power_ctrl jpu_pwm_ctrl = {0};



static int s_interrupt_flag[MAX_NUM_INSTANCE];
static wait_queue_head_t s_interrupt_wait_q[MAX_NUM_INSTANCE];


static spinlock_t s_jpu_lock = __SPIN_LOCK_UNLOCKED(s_jpu_lock);
#if KERNEL_VERSION(2, 6, 36) > LINUX_VERSION_CODE
static DECLARE_MUTEX(s_jpu_sem);
#else
static DEFINE_SEMAPHORE(s_jpu_sem);
#endif
static struct list_head s_jbp_head = LIST_HEAD_INIT(s_jbp_head);
static struct list_head s_inst_list_head = LIST_HEAD_INIT(s_inst_list_head);


#define NPT_BASE                                0x0000
#define NPT_REG_SIZE                            0x300
#define MJPEG_PIC_STATUS_REG(_inst_no)          (NPT_BASE + (_inst_no*NPT_REG_SIZE) + 0x004)

#define ReadJpuRegister(addr, id)          readl(s_jpu_register[id].virt_addr + addr)
#define WriteJpuRegister(addr, val, id)    writel(val, s_jpu_register[id].virt_addr + addr)

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
#define IOREMAP(addr, size) ioremap(addr, size)
#else
#define IOREMAP(addr, size) ioremap_nocache(addr, size)
#endif


static int jpu_alloc_dma_buffer(jpudrv_buffer_t *jb)
{
    if (!jb)
        return -1;
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
    jb->phys_addr = (unsigned long long)jmem_alloc(&s_jmem, jb->size, 0);
    if ((unsigned long)jb->phys_addr  == (unsigned long)-1) {
        DPRINTK("[JPUDRV] Physical memory allocation error size=%lu\n", jb->size);
        return -1;
    }

    jb->base = (unsigned long)(s_video_memory.base + (jb->phys_addr - s_video_memory.phys_addr));
    jb->virt_addr = jb->base;
#elif defined(JPU_SUPPORT_ION_MEMORY)
    if (base_ion_alloc((uint64_t *)&jb->phys_addr, (void **)&jb->virt_addr, "jpeg_ion", jb->size, 0) != 0) {
        DPRINTK("[JPUDRV] Physical memory allocation error size=%lu\n", jb->size);
        return -1;
    }
    jb->base = jb->virt_addr;

#else
    jb->base = (unsigned long)dma_alloc_coherent(jpu_dev, PAGE_ALIGN(jb->size), (dma_addr_t *) (&jb->phys_addr), GFP_DMA | GFP_KERNEL);
    if ((void *)(jb->base) == NULL) {
        DPRINTK("[JPUDRV] Physical memory allocation error size=%lu\n", jb->size);
        return -1;
    }
    jb->virt_addr = jb->base;

    // pr_info("mark jpu alloc phys:0x%lx, virt:0x%lx, size:0x%x \n", jb->phys_addr, jb->virt_addr, jb->size);
#endif /* JPU_SUPPORT_RESERVED_VIDEO_MEMORY */
    return 0;
}

static void jpu_free_dma_buffer(jpudrv_buffer_t *jb)
{
    if (!jb) {
        return;
    }
    if (jb->base)
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
        jmem_free(&s_jmem, jb->phys_addr, 0);
#elif defined(JPU_SUPPORT_ION_MEMORY)
        base_ion_free(jb->phys_addr);
#else
        dma_free_coherent(jpu_dev, PAGE_ALIGN(jb->size), (void *)jb->base, jb->phys_addr);
#endif /* JPUR_SUPPORT_RESERVED_VIDEO_MEMORY */
}


static int jpu_free_instances(struct file *filp)
{
	jpudrv_instance_list_t *vil, *n;
	jpudrv_instance_pool_t *vip;
	void *vip_base;
	int instance_pool_size_per_core;
	void *jdi_mutexes_base;
	const int PTHREAD_MUTEX_T_DESTROY_VALUE = 0xdead10cc;
	int i;

	DPRINTK("[JPUDRV] %s\n", __func__);


	/* s_instance_pool.size  assigned to the size of all core once call JDI_IOCTL_GET_INSTANCE_POOL by user. */
	instance_pool_size_per_core = (s_instance_pool.size /  MAX_NUM_JPU_CORE);
	list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
		if (vil->filp == filp) {
			vip_base = (void *)(s_instance_pool.base + instance_pool_size_per_core);
			DPRINTK("[JPUDRV] instance crash instIdx=%d, vip_base=%p, size_per_core=%d\n",
				__func__,
				(int)vil->inst_idx, vip_base, (int)instance_pool_size_per_core);
			vip = (jpudrv_instance_pool_t *)vip_base;
			if (vip) {
				// only first 4 byte is key point(inUse of CodecInst in jpuapi)
				// to free the corresponding instance.
				memset(&vip->codecInstPool[vil->inst_idx], 0x00, 4);
#define PTHREAD_MUTEX_T_HANDLE_SIZE 4
				jdi_mutexes_base = (vip_base +
						    (instance_pool_size_per_core - PTHREAD_MUTEX_T_HANDLE_SIZE * 4));
				DPRINTK("[JPUDRV] %s : force to destroy jdi_mutexes_base=%p in userspace\n",
					__func__, jdi_mutexes_base);
				if (jdi_mutexes_base) {
					for (i = 0; i < 4; i++) {
						memcpy(jdi_mutexes_base,
						       &PTHREAD_MUTEX_T_DESTROY_VALUE,
						       PTHREAD_MUTEX_T_HANDLE_SIZE);
						jdi_mutexes_base += PTHREAD_MUTEX_T_HANDLE_SIZE;
					}
				}
			}
			s_jpu_open_ref_count--;
			list_del(&vil->list);
			kfree(vil);
		}
	}
	return 1;
}

static int jpu_free_buffers(struct file *filp)
{
	jpudrv_buffer_pool_t *pool, *n;
	jpudrv_buffer_t jb;

	DPRINTK("[JPUDRV] %s\n", __func__);

	list_for_each_entry_safe(pool, n, &s_jbp_head, list) {
		if (pool->filp == filp) {
			jb = pool->jb;
			if (jb.base) {
				jpu_free_dma_buffer(&jb);
				list_del(&pool->list);
				kfree(pool);
			}
		}
	}

	return 0;
}


static irqreturn_t jpu_irq_handler(int irq, void *data)
{
	jpu_drv_context_t  *p_jpu_drv = (jpu_drv_context_t *)data;
	int i;
	u32 flag;
	u32 dev_id = p_jpu_drv->dev_id;

	DPRINTK("[JPUDRV][+]%s\n", __func__);

#ifdef JPU_IRQ_CONTROL
	disable_irq_nosync(p_jpu_drv->irq);
#endif
	flag = 0;
	for (i = 0; i < MAX_NUM_REGISTER_SET; i++) {
		flag = ReadJpuRegister(MJPEG_PIC_STATUS_REG(i), dev_id);
		if (flag != 0) {
			break;
		}
	}

	p_jpu_drv->interrupt_reason[i] = flag;
	s_interrupt_flag[i] = 1;
	// clear irq
	WriteJpuRegister(MJPEG_PIC_STATUS_REG(i), flag , dev_id);
	DPRINTK("[JPUDRV][%d] INTERRUPT FLAG: %08x, %08x\n", i,
	p_jpu_drv->interrupt_reason[i], MJPEG_PIC_STATUS_REG(i));

	if (p_jpu_drv->async_queue)
		kill_fasync(&p_jpu_drv->async_queue, SIGIO, POLL_IN);    // notify the interrupt to userspace

	wake_up_interruptible(&s_interrupt_wait_q[i]);

	DPRINTK("[JPUDRV][-]%s flag=0x%x\n", __func__, flag);
	return IRQ_HANDLED;
}

static int jpu_open(struct inode *inode, struct file *filp)
{
	u32 dev_id = iminor(inode);
	DPRINTK("[JPUDRV][+] %s\n", __func__);

	spin_lock(&s_jpu_lock);

	s_jpu_drv_context[dev_id].open_count++;

	filp->private_data = (void *)(&s_jpu_drv_context[dev_id]);
	spin_unlock(&s_jpu_lock);

	DPRINTK("[JPUDRV][-] %s\n", __func__);

	return 0;
}

static long jpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	int ret = 0;
	struct jpu_drv_context_t *p_jpu_drv = filp->private_data;
	u32 dev_id = p_jpu_drv->dev_id;

	switch (cmd) {
	case JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY: {
		jpudrv_buffer_pool_t *jbp;

		DPRINTK("[JPUDRV][+]JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");
		ret = down_interruptible(&s_jpu_sem);
		if (ret == 0) {
			jbp = kzalloc(sizeof(jpudrv_buffer_pool_t), GFP_KERNEL);
			if (!jbp) {
				up(&s_jpu_sem);
				return -ENOMEM;
			}

			ret = copy_from_user(&(jbp->jb), (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));
			if (ret) {
				kfree(jbp);
				up(&s_jpu_sem);
				return -EFAULT;
			}

			ret = jpu_alloc_dma_buffer(&(jbp->jb));
			if (ret == -1) {
				ret = -ENOMEM;
				kfree(jbp);
				up(&s_jpu_sem);
				break;
			}
			ret = copy_to_user((void __user *)arg, &(jbp->jb), sizeof(jpudrv_buffer_t));
			if (ret) {
				kfree(jbp);
				ret = -EFAULT;
				up(&s_jpu_sem);
				break;
			}

			jbp->filp = filp;
			spin_lock(&s_jpu_lock);
			list_add(&jbp->list, &s_jbp_head);
			spin_unlock(&s_jpu_lock);

			up(&s_jpu_sem);
		}
		DPRINTK("[JPUDRV][-]JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");
	}
	break;
	case JDI_IOCTL_FREE_PHYSICALMEMORY: {
		jpudrv_buffer_pool_t *jbp, *n;
		jpudrv_buffer_t jb;

		DPRINTK("[JPUDRV][+]VDI_IOCTL_FREE_PHYSICALMEMORY\n");
		ret = down_interruptible(&s_jpu_sem);
		if (ret == 0) {
			ret = copy_from_user(&jb, (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));
			if (ret) {
				up(&s_jpu_sem);
				return -EACCES;
			}

			if (jb.base)
				jpu_free_dma_buffer(&jb);

			spin_lock(&s_jpu_lock);
			list_for_each_entry_safe(jbp, n, &s_jbp_head, list) {
				if (jbp->jb.base == jb.base) {
					list_del(&jbp->list);
					kfree(jbp);
					break;
				}
			}
			spin_unlock(&s_jpu_lock);

			up(&s_jpu_sem);
		}
		DPRINTK("[JPUDRV][-]VDI_IOCTL_FREE_PHYSICALMEMORY\n");
	}
	break;
	case JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO: {
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
		if (s_video_memory.base != 0) {
			ret = copy_to_user((void __user *)arg, &s_video_memory, sizeof(jpudrv_buffer_t));
			if (ret != 0)
				ret = -EFAULT;
		} else {
			ret = -EFAULT;
		}
#endif /* JPU_SUPPORT_RESERVED_VIDEO_MEMORY */
	}
	break;
	case JDI_IOCTL_WAIT_INTERRUPT: {
		jpudrv_intr_info_t info;
		struct jpu_drv_context_t *dev = (struct jpu_drv_context_t *)filp->private_data;
		u32 instance_no;

		DPRINTK("[JPUDRV][+]JDI_IOCTL_WAIT_INTERRUPT\n");
		ret = copy_from_user(&info, (jpudrv_intr_info_t *)arg, sizeof(jpudrv_intr_info_t));
		if (ret != 0)
			return -EFAULT;

		instance_no = info.inst_idx;
		DPRINTK("[JPUDRV] INSTANCE NO: %d\n", instance_no);
		ret = wait_event_interruptible_timeout(s_interrupt_wait_q[instance_no],
						       s_interrupt_flag[instance_no] != 0,
						       msecs_to_jiffies(info.timeout));
		if (!ret) {
			DPRINTK("[JPUDRV] INSTANCE NO: %d ETIME\n", instance_no);
			ret = -ETIME;
			break;
		}

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			DPRINTK("[JPUDRV] INSTANCE NO: %d ERESTARTSYS\n", instance_no);
			break;
		}

		DPRINTK("[JPUDRV] INST(%d) s_interrupt_flag(%d), reason(0x%08lx)\n",
			instance_no,
			s_interrupt_flag[instance_no],
			dev->interrupt_reason[instance_no]);

		info.intr_reason = dev->interrupt_reason[instance_no];
		s_interrupt_flag[instance_no] = 0;
		dev->interrupt_reason[instance_no] = 0;
		ret = copy_to_user((void __user *)arg, &info, sizeof(jpudrv_intr_info_t));
#ifdef JPU_IRQ_CONTROL
		enable_irq(p_jpu_drv->irq);
#endif
		DPRINTK("[VPUDRV][-]VDI_IOCTL_WAIT_INTERRUPT\n");
		if (ret != 0)
			return -EFAULT;
	}
	break;
	case JDI_IOCTL_SET_CLOCK_GATE: {
		u32 clkgate;

		if (get_user(clkgate, (u32 __user *) arg))
			return -EFAULT;

#ifdef JPU_SUPPORT_CLOCK_CONTROL
		if (clkgate)
			jpu_clk_enable(s_jpu_clk);
		else
			jpu_clk_disable(s_jpu_clk);
#endif /* JPU_SUPPORT_CLOCK_CONTROL */
	}
	break;
	case JDI_IOCTL_GET_INSTANCE_POOL:
		DPRINTK("[JPUDRV][+]JDI_IOCTL_GET_INSTANCE_POOL\n");

		ret = down_interruptible(&s_jpu_sem);
		if (ret == 0) {
			if (s_instance_pool.base != 0) {
				ret = copy_to_user((void __user *)arg,
						   &s_instance_pool,
						   sizeof(jpudrv_buffer_t));
			} else {
				ret = copy_from_user(&s_instance_pool, (jpudrv_buffer_t *)arg, sizeof(jpudrv_buffer_t));

				if (ret != 0) {
					ret = -EFAULT;
					pr_err("copy_from_user fail\n");
					up(&s_jpu_sem);
					break;
				}

				s_instance_pool.size      = PAGE_ALIGN(s_instance_pool.size);
				s_instance_pool.base      = (unsigned long)vmalloc(s_instance_pool.size);
				s_instance_pool.phys_addr = s_instance_pool.base;

				if (s_instance_pool.base != 0) {
					/*clearing memory*/
					memset((void *)s_instance_pool.base, 0x0, s_instance_pool.size);
					ret = copy_to_user((void __user *)arg,
							   &s_instance_pool,
							   sizeof(jpudrv_buffer_t));

					/* success to get memory for instance pool */
					if (ret == 0) {
						up(&s_jpu_sem);
						break;
					}
				}
				ret = -EFAULT;

			}
			up(&s_jpu_sem);
		}

		DPRINTK("[JPUDRV][-]JDI_IOCTL_GET_INSTANCE_POOL: %s base: %lx, size: %d\n",
			(ret == 0 ? "OK" : "NG"), s_instance_pool.base, s_instance_pool.size);
		break;
	case JDI_IOCTL_OPEN_INSTANCE: {
		jpudrv_inst_info_t inst_info;

		if (copy_from_user(&inst_info, (jpudrv_inst_info_t *)arg, sizeof(jpudrv_inst_info_t)))
			return -EFAULT;

		spin_lock(&s_jpu_lock);
		s_jpu_open_ref_count++; /* flag just for that jpu is in opened or closed */
		inst_info.inst_open_count = s_jpu_open_ref_count;
		spin_unlock(&s_jpu_lock);

		if (copy_to_user((void __user *)arg, &inst_info, sizeof(jpudrv_inst_info_t))) {
			return -EFAULT;
		}

		DPRINTK("[JPUDRV] JDI_IOCTL_OPEN_INSTANCE inst_idx=%d, s_jpu_open_ref_count=%d, inst_open_count=%d\n",
			(int)inst_info.inst_idx, s_jpu_open_ref_count, inst_info.inst_open_count);
	}
	break;
	case JDI_IOCTL_CLOSE_INSTANCE: {
		jpudrv_inst_info_t inst_info;

		DPRINTK("[JPUDRV][+]JDI_IOCTL_CLOSE_INSTANCE\n");
		if (copy_from_user(&inst_info, (jpudrv_inst_info_t *)arg, sizeof(jpudrv_inst_info_t)))
			return -EFAULT;

		spin_lock(&s_jpu_lock);
		s_jpu_open_ref_count--; /* flag just for that jpu is in opened or closed */
		inst_info.inst_open_count = s_jpu_open_ref_count;
		spin_unlock(&s_jpu_lock);

		if (copy_to_user((void __user *)arg, &inst_info, sizeof(jpudrv_inst_info_t)))
			return -EFAULT;

		DPRINTK("[JPUDRV] JDI_IOCTL_CLOSE_INSTANCE inst_idx=%d, s_jpu_open_ref_count=%d, inst_open_count=%d\n",
			(int)inst_info.inst_idx, s_jpu_open_ref_count, inst_info.inst_open_count);
	}
	break;
	case JDI_IOCTL_GET_INSTANCE_NUM: {
		jpudrv_inst_info_t inst_info;

		DPRINTK("[JPUDRV][+]JDI_IOCTL_GET_INSTANCE_NUM\n");

		ret = copy_from_user(&inst_info, (jpudrv_inst_info_t *)arg, sizeof(jpudrv_inst_info_t));
		if (ret != 0)
			break;

		spin_lock(&s_jpu_lock);
		inst_info.inst_open_count = s_jpu_open_ref_count;
		spin_unlock(&s_jpu_lock);

		ret = copy_to_user((void __user *)arg, &inst_info, sizeof(jpudrv_inst_info_t));

		DPRINTK("[JPUDRV] JDI_IOCTL_GET_INSTANCE_NUM inst_idx=%d, open_count=%d\n", (int)inst_info.inst_idx,
			inst_info.inst_open_count);
	}
	break;
	case JDI_IOCTL_RESET:
		jpu_hw_reset();
		break;

	case JDI_IOCTL_GET_REGISTER_INFO:
		DPRINTK("[JPUDRV][+]JDI_IOCTL_GET_REGISTER_INFO\n");
		ret = copy_to_user((void __user *)arg, &s_jpu_register[dev_id], sizeof(jpudrv_buffer_t));
		if (ret != 0)
			ret = -EFAULT;
		DPRINTK("[JPUDRV][-]JDI_IOCTL_GET_REGISTER_INFO phy=0x%lx, virtr=0x%lx, size=%d\n",
			s_jpu_register[dev_id].phys_addr,
			s_jpu_register[dev_id].virt_addr,
			s_jpu_register[dev_id].size);
		break;
	default: {
		pr_err("No such IOCTL, cmd is %d\n", cmd);
	}
	break;
	}
	return ret;
}

static ssize_t jpu_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{

	return -1;
}

static ssize_t jpu_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{

	/* DPRINTK("[VPUDRV] vpu_write len=%d\n", (int)len); */
	if (!buf) {
		pr_err("[VPUDRV] vpu_write buf = NULL error\n");
		return -EFAULT;
	}

	return -1;
}

static int jpu_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	u32 open_count;

	DPRINTK("[JPUDRV][+] %s\n", __func__);

	ret = down_interruptible(&s_jpu_sem);
	if (ret  == 0) {

		/* found and free the not handled buffer by user applications */
		jpu_free_buffers(filp);

		/* found and free the not closed instance by user applications */
		jpu_free_instances(filp);
		DPRINTK("[JPUDRV] open_count: %d\n", s_jpu_drv_context[0].open_count);
		spin_lock(&s_jpu_lock);
		s_jpu_drv_context[0].open_count--;
		open_count = s_jpu_drv_context[0].open_count;
		spin_unlock(&s_jpu_lock);
		if (open_count == 0) {
			if (s_instance_pool.base) {
				DPRINTK("[JPUDRV] free instance pool\n");
				vfree((const void *)s_instance_pool.base);
				s_instance_pool.base = 0;
			}


		}
	}
	up(&s_jpu_sem);
	DPRINTK("[JPUDRV][-] %s\n", __func__);

	return 0;
}


static int jpu_fasync(int fd, struct file *filp, int mode)
{
	struct jpu_drv_context_t *dev = (struct jpu_drv_context_t *)filp->private_data;

	return fasync_helper(fd, filp, mode, &dev->async_queue);
}




static int jpu_map_to_register(struct file *fp, struct vm_area_struct *vm)
{
	unsigned long pfn;
	struct jpu_drv_context_t *p_jpu_drv = fp->private_data;
	u32 dev_id = p_jpu_drv->dev_id;

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = s_jpu_register[dev_id].phys_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end - vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}

static int jpu_map_to_physical_memory(struct file *fp, struct vm_area_struct *vm)
{
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start,
			       vm->vm_pgoff, vm->vm_end - vm->vm_start,
			       vm->vm_page_prot) ? -EAGAIN : 0;
}

static int jpu_map_to_instance_pool_memory(struct file *fp, struct vm_area_struct *vm)
{
	int ret;
	long length = vm->vm_end - vm->vm_start;
	unsigned long start = vm->vm_start;
	char *vmalloc_area_ptr = (char *)s_instance_pool.base;
	unsigned long pfn;

	vm->vm_flags |= VM_RESERVED;

	/* loop over all pages, map it page individually */
	while (length > 0) {
		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		ret = remap_pfn_range(vm, start, pfn, PAGE_SIZE, PAGE_SHARED);
		if (ret < 0) {
			return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;
	}

	return 0;
}

/*
 * @brief memory map interface for jpu file operation
 * @return  0 on success or negative error code on error
 */
static int jpu_mmap(struct file *fp, struct vm_area_struct *vm)
{
	struct jpu_drv_context_t *p_jpu_drv = fp->private_data;
	u32 dev_id = p_jpu_drv->dev_id;
	if (vm->vm_pgoff == 0)
		return jpu_map_to_instance_pool_memory(fp, vm);

	if (vm->vm_pgoff == (s_jpu_register[dev_id].phys_addr >> PAGE_SHIFT))
		return jpu_map_to_register(fp, vm);

	return jpu_map_to_physical_memory(fp, vm);
}

#ifdef CONFIG_COMPAT
static long jpu_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

const struct file_operations jpu_fops = {
	.owner = THIS_MODULE,
	.open = jpu_open,
	.read = jpu_read,
	.write = jpu_write,
	.unlocked_ioctl = jpu_ioctl,
	.release = jpu_release,
	.fasync = jpu_fasync,
	.mmap = jpu_mmap,
#ifdef CONFIG_COMPAT
	.compat_ioctl = jpu_compat_ptr_ioctl,
#endif
};

static int jpu_register_clk(struct platform_device *pdev)
{
	int ret;
	const char *clk_name;

	of_property_read_string_index(pdev->dev.of_node, "clock-names", 0, &clk_name);

	jpu_pwm_ctrl.jpu_apb_clk = devm_clk_get(&pdev->dev, clk_name);
	if (IS_ERR(jpu_pwm_ctrl.jpu_apb_clk)) {
		ret = PTR_ERR(jpu_pwm_ctrl.jpu_apb_clk);
		dev_err(&pdev->dev, "failed to retrieve jpu %s clock",  clk_name);
		return ret;
	}
	pr_err("retrive %s clk succeed.\n", clk_name);
	of_property_read_string_index(pdev->dev.of_node, "clock-names", 1, &clk_name);

	jpu_pwm_ctrl.jpu_axi_clk  = devm_clk_get(&pdev->dev, clk_name);
	if (IS_ERR(jpu_pwm_ctrl.jpu_axi_clk)) {
		ret = PTR_ERR(jpu_pwm_ctrl.jpu_axi_clk);
		dev_err(&pdev->dev, "failed to retrieve jpu %s clock", clk_name);
		return ret;
	}
	pr_err("retrive %s clk succeed.\n", clk_name);
	return 0;
}

static int jpu_probe(struct platform_device *pdev)
{
	int err = 0;
	struct resource *res = NULL;
	u32 dev_id = 0;
	char *dev_name;
	struct cma *cma;
	unsigned int *virt_top_addr;
	unsigned int value;
	dev_t dev;
	int irq;

	DPRINTK("[JPUDRV] %s\n", __func__);
	if (pdev)
		device_property_read_u32(&pdev->dev, "dev-id", &dev_id);
	if (pdev) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	}

	if (res) {/* if platform driver is implemented */
		s_jpu_register[dev_id].phys_addr = res->start;
		s_jpu_register[dev_id].virt_addr = (unsigned long)IOREMAP(res->start, res->end - res->start);
		s_jpu_register[dev_id].size      = res->end - res->start;
	}
	DPRINTK("[JPUDRV] : physical base addr==0x%lx, virtual base=0x%lx\n",
	s_jpu_register[dev_id].phys_addr, s_jpu_register[dev_id].virt_addr);

		device_property_read_u32(&pdev->dev, "dev-id", &dev_id);

	dev_name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "%s%d", JPU_DEV_NAME, dev_id);
	if (!s_jpu_major) {
	/* get the major number of the character device */
		err = alloc_chrdev_region(&dev, dev_id, dev_id + 1, JPU_DEV_NAME);
		if (err < 0) {
			err = -EBUSY;
			pr_err("could not allocate major number\n");
			goto ERROR_PROVE_DEVICE;
		}
		s_jpu_major = MAJOR(dev);
	} else {
		dev = MKDEV(s_jpu_major, dev_id);
		err = register_chrdev_region(dev, 1, JPU_DEV_NAME);
		if (err) {
			pr_err("reg chrdev region fail\n");
			goto ERROR_PROVE_DEVICE;
		}
	}

	/* initialize the device structure and register the device with the kernel */
	cdev_init(&s_jpu_cdev[dev_id], &jpu_fops);
	s_jpu_cdev[dev_id].owner = THIS_MODULE;

	if ((cdev_add(&s_jpu_cdev[dev_id], dev, 1)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate chrdev\n");

		goto ERROR_PROVE_DEVICE;
	}
	jpu_dev = &pdev->dev;
	err = dma_set_mask_and_coherent(jpu_dev, DMA_BIT_MASK(64));
	if (err) {
		pr_err("dma_set_mask_and_coherent 64 fail\n");
		err = dma_set_mask_and_coherent(jpu_dev, DMA_BIT_MASK(32));
	}
	if (err) {
		pr_err("dma_set_mask_and_coherent 32 fail\n");
		goto ERROR_PROVE_DEVICE;
	}

	device_create(jpu_class, NULL, dev, NULL, "%s", dev_name);
	if (pdev)
		s_jpu_clk = jpu_clk_get(&pdev->dev);
	else
		s_jpu_clk = jpu_clk_get(NULL);

	s_jpu_clk = NULL;
	if (!s_jpu_clk) {
		pr_err("[JPUDRV] : not support clock controller.\n");
	} else {
		DPRINTK("[JPUDRV] : get clock controller s_jpu_clk=%p\n", s_jpu_clk);
	}

#ifdef JPU_SUPPORT_CLOCK_CONTROL
#else
	jpu_clk_enable(s_jpu_clk);
#endif


#ifdef JPU_SUPPORT_ISR
#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res) {/* if platform driver is implemented */
		irq = res->start;
		DPRINTK("[JPUDRV] : jpu irq number get from platform driver irq=0x%x\n", irq);
	} else {
		DPRINTK("[JPUDRV] : jpu irq number get from defined value irq=0x%x\n", irq);
	}
#else
	DPRINTK("[JPUDRV] : jpu irq number get from defined value irq=0x%x\n", s_jpu_irq);
#endif


	err = request_irq(irq, jpu_irq_handler, 0, dev_name, (void *)(&s_jpu_drv_context[dev_id]));
	if (err) {
		pr_err("[JPUDRV] :  fail to register interrupt handler\n");
		goto ERROR_PROVE_DEVICE;
	}
	s_jpu_drv_context[dev_id].irq = irq;
#endif

#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
	s_video_memory.size      = JPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE;
	s_video_memory.phys_addr = JPU_DRAM_PHYSICAL_BASE;
	s_video_memory.base = (unsigned long)IOREMAP(s_video_memory.phys_addr, PAGE_ALIGN(s_video_memory.size));
	if (!s_video_memory.base) {
		pr_err("[JPUDRV] :  fail to remap video memory physical phys_addr=0x%lx, base=0x%lx, size=%d\n",
		       s_video_memory.phys_addr, s_video_memory.base, s_video_memory.size);
		goto ERROR_PROVE_DEVICE;
	}

	if (jmem_init(&s_jmem, s_video_memory.phys_addr, s_video_memory.size) < 0) {
		pr_err("[JPUDRV] :  fail to init vmem system\n");
		goto ERROR_PROVE_DEVICE;
	}
	DPRINTK("[JPUDRV] success to probe jpu device with reserved video memory phys_addr=0x%lx, base=0x%lx\n",
		s_video_memory.phys_addr, s_video_memory.base);
#else
	DPRINTK("[JPUDRV] success to probe jpu device with non reserved video memory\n");
#endif

	s_jpu_drv_context[dev_id].dev_id = dev_id;
	platform_set_drvdata(pdev, &s_jpu_drv_context[dev_id]);

	virt_top_addr = IOREMAP(JPEG_TOP_REG, 4);
	writel((1 << 4) | (1 << 8) | (1 << 12) | (1 << 16), virt_top_addr);

	return 0;


ERROR_PROVE_DEVICE:

	if (s_jpu_major)
		unregister_chrdev_region(s_jpu_major, 1);

	if (s_jpu_register[dev_id].virt_addr)
		iounmap((void *)s_jpu_register[dev_id].virt_addr);

	return err;
}

static int jpu_remove(struct platform_device *pdev)
{
	struct jpu_drv_context_t *p_jpu_drv = platform_get_drvdata(pdev);
	u32 dev_id = p_jpu_drv->dev_id;
	dev_t dev;
	DPRINTK("[JPUDRV] %s\n", __func__);
#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER

	if (s_instance_pool.base) {
		vfree((const void *)s_instance_pool.base);
		s_instance_pool.base = 0;
	}
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) {
		iounmap((void *)s_video_memory.base);
		s_video_memory.base = 0;
		jmem_exit(&s_jmem);
	}
#endif

#ifdef JPU_SUPPORT_ISR
	if (p_jpu_drv->irq)
		free_irq(p_jpu_drv->irq, (void *)(&s_jpu_drv_context[dev_id]));
#endif /* JPU_SUPPORT_ISR */
	dev = MKDEV(s_jpu_major, dev_id);
	device_destroy(jpu_class, dev);
	cdev_del(&s_jpu_cdev[dev_id]);
	unregister_chrdev_region(dev, 1);


	if (s_jpu_register[dev_id].virt_addr)
		iounmap((void *)s_jpu_register[dev_id].virt_addr);

	jpu_clk_put(s_jpu_clk);

#endif /* JPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

	return 0;
}

#ifdef CONFIG_PM
static int jpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	jpu_clk_disable(s_jpu_clk);
	return 0;

}
static int jpu_resume(struct platform_device *pdev)
{
	jpu_clk_enable(s_jpu_clk);

	return 0;
}
#else
#define    jpu_suspend    NULL
#define    jpu_resume    NULL
#endif /* !CONFIG_PM */

#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER
static const struct of_device_id bm_jpu_match_table[] = {
	{.compatible = "sophgo,vc_drv"},
	{},
};

static struct platform_driver jpu_driver = {
	.driver = {
		.name = JPU_PLATFORM_DEVICE_NAME,
		.of_match_table = bm_jpu_match_table,
	},
	.probe      = jpu_probe,
	.remove     = jpu_remove,
	.suspend    = jpu_suspend,
	.resume     = jpu_resume,
};
#endif /* JPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

static int __init jpu_init(void)
{
	int res = 0;
	u32 i;

	DPRINTK("[JPUDRV] begin %s\n", __func__);
	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		init_waitqueue_head(&s_interrupt_wait_q[i]);
	}
	s_instance_pool.base = 0;
	jpu_class = class_create(THIS_MODULE, "jpu");
	if (IS_ERR(jpu_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(jpu_class);
	}
#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	res = platform_driver_register(&jpu_driver);
#else
	res = jpu_probe(NULL);
#endif /* JPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

	DPRINTK("[JPUDRV] end %s result=0x%x\n", __func__, res);
	return res;
}

static void __exit jpu_exit(void)
{
	DPRINTK("[JPUDRV] [+]%s\n", __func__);
#ifdef JPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	platform_driver_unregister(&jpu_driver);

	class_destroy(jpu_class);
	s_jpu_major = 0;

#else /* JPU_SUPPORT_PLATFORM_DRIVER_REGISTER */
#ifdef JPU_SUPPORT_CLOCK_CONTROL
#else
	jpu_clk_disable(s_jpu_clk);
#endif /* JPU_SUPPORT_CLOCK_CONTROL */
	jpu_clk_put(s_jpu_clk);
	if (s_instance_pool.base) {
		vfree((const void *)s_instance_pool.base);
		s_instance_pool.base = 0;
	}
#ifdef JPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) {
		iounmap((void *)s_video_memory.base);
		s_video_memory.base = 0;

		jmem_exit(&s_jmem);
	}
#endif /* JPU_SUPPORT_RESERVED_VIDEO_MEMORY */

#endif /* JPU_SUPPORT_PLATFORM_DRIVER_REGISTER */
	DPRINTK("[JPUDRV] [-]%s\n", __func__);
}

MODULE_AUTHOR("sophgo");
MODULE_DESCRIPTION("JPU linux driver");
MODULE_LICENSE("GPL");

module_init(jpu_init);
module_exit(jpu_exit);

static int jpu_hw_reset(void)
{
	DPRINTK("[JPUDRV] request jpu reset from application.\n");
	return 0;
}


struct clk *jpu_clk_get(struct device *dev)
{
#ifdef ENABLE_CLK_CONTROL
	return devm_clk_get(dev, JPU_CLK_NAME);
#else
	return NULL;
#endif
}
void jpu_clk_put(struct clk *clk)
{
#ifdef ENABLE_CLK_CONTROL
	if (!(clk == NULL || IS_ERR(clk)))
		clk_put(clk);
#endif
}
int jpu_clk_enable(struct clk *clk)
{
#ifdef ENABLE_CLK_CONTROL
	if (clk) {
		DPRINTK("[JPUDRV] %s\n", __func__);
		// You need to implement it
		clk_prepare_enable(clk);
		return 1;
	}
#endif
	return 0;
}

void jpu_clk_disable(struct clk *clk)
{
#ifdef ENABLE_CLK_CONTROL
	if (!(clk == NULL || IS_ERR(clk))) {
		DPRINTK("[JPUDRV] %s\n", __func__);
		clk_disable_unprepare(clk);
		// You need to implement it
	}
#endif
}




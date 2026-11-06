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
#include <linux/kfifo.h>
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
#include <linux/compat.h>

#include "vpuconfig.h"
#include "vpuerror.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "vpu.h"

#define VE_TOP_EXT_ADDR	0x21000010

static unsigned long ve_virt_top;

static u32 devid;
static u32 core;

static struct device *vpu_dev;


//#define ENABLE_DEBUG_MSG
#ifdef ENABLE_DEBUG_MSG
#define DPRINTK(args...)		printk(KERN_INFO args);
#else
#define DPRINTK(args...)
#endif
#include "wave5_regdefine.h"
#include "wave6_regdefine.h"
#include "coda9_regdefine.h"

#define VPU_PRODUCT_CODE_REGISTER                 (0x1044)

/* definitions to be changed as customer  configuration */
/* if you want to have clock gating scheme frame by frame */
/* #define VPU_SUPPORT_CLOCK_CONTROL */

/* if the driver want to use interrupt service from kernel ISR */
#ifdef SUPPORT_INTERRUPT
#define VPU_SUPPORT_ISR
#else
#endif

#ifdef VPU_SUPPORT_ISR
/* if the driver want to disable and enable IRQ whenever interrupt asserted. */
//#define VPU_IRQ_CONTROL
#endif

/* if the platform driver knows the name of this driver */
/* VPU_PLATFORM_DEVICE_NAME */
#define VPU_SUPPORT_PLATFORM_DRIVER_REGISTER

/* if this driver knows the dedicated video memory address */
// #define VPU_SUPPORT_RESERVED_VIDEO_MEMORY

#define VPU_PLATFORM_DEVICE_NAME "vpu1"
#define VPU_CLK_NAME "vcodec"
#define VPU_DEV_NAME "vpu"

/* if the platform driver knows this driver */
/* the definition of VPU_REG_BASE_ADDR and VPU_REG_SIZE are not meaningful */

#define VPU_REG_BASE_ADDR 0x75000000
#define VPU_REG_SIZE (0x4000*MAX_NUM_VPU_CORE)

#ifdef VPU_SUPPORT_ISR
#define VPU_IRQ_NUM (23+32)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define IOREMAP(addr, size) ioremap(addr, size)
#else
#define IOREMAP(addr, size) ioremap_nocache(addr, size)
#endif

// static struct timespec64 intr_tv_last = {0};
// static unsigned long  consume_us_all = 0;
// static unsigned long  consume_times = 0;

/* this definition is only for chipsnmedia FPGA board env */
/* so for SOC env of customers can be ignored */

#ifndef VM_RESERVED	/*for kernel up to 3.7.0 version*/
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

typedef struct fps_info_t {
    int is_first;
    struct timespec64 intr_tv_last;
    unsigned long  consume_us_all;
    unsigned long  consume_times;
} vpu_fps_info;

static vpu_fps_info s_fps_info[32];

typedef struct vpu_drv_context_t {
	struct fasync_struct *async_queue;
	unsigned long interrupt_reason[MAX_NUM_INSTANCE];
	u32 open_count;					 /*!<< device reference count. Not instance count */
	int support_cq;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 irq_timestamp;
#else
	struct timespec irq_timestamp;
#endif
} vpu_drv_context_t;

/* To track the allocated memory buffer */
typedef struct vpudrv_buffer_pool_t {
	struct list_head list;
	struct vpudrv_buffer_t vb;
	struct file *filp;
} vpudrv_buffer_pool_t;

/* To track the instance index and buffer in instance pool */
typedef struct vpudrv_instanace_list_t {
	struct list_head list;
	unsigned long inst_idx;
	unsigned long core_idx;
	struct file *filp;
} vpudrv_instanace_list_t;

typedef struct vpudrv_instance_pool_t {
	unsigned char codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
	vpudrv_buffer_t    vpu_common_buffer;
	int vpu_instance_num;
	int instance_pool_inited;
	void* pendingInst;
	int pendingInstIdxPlus1;
	int doSwResetInstIdxPlus1;
} vpudrv_instance_pool_t;


#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
#	define VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE (62*1024*1024)
#	define VPU_DRAM_PHYSICAL_BASE 0x86C00000
#include "vmm.h"
static video_mm_t s_vmem;
static vpudrv_buffer_t s_video_memory = {0};
#endif /*VPU_SUPPORT_RESERVED_VIDEO_MEMORY*/

#define VPU_WAKE_MODE 0
#define VPU_SLEEP_MODE 1
typedef enum {
	VPUAPI_RET_SUCCESS,
	VPUAPI_RET_FAILURE, // an error reported by FW
	VPUAPI_RET_TIMEOUT,
	VPUAPI_RET_STILL_RUNNING,
	VPUAPI_RET_INVALID_PARAM,
	VPUAPI_RET_MAX
} VpuApiRet;
static int vpuapi_close(u32 core, u32 inst);
static int vpuapi_dec_set_stream_end(u32 core, u32 inst);
static int vpuapi_dec_clr_all_disp_flag(u32 core, u32 inst);
static int vpuapi_get_output_info(u32 core, u32 inst, u32 *error_reason);
#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)
static int vpu_sleep_wake(u32 core, int mode);
#endif
static int vpu_do_sw_reset(u32 core, u32 inst, u32 error_reason);
static int vpu_close_instance(u32 core, u32 inst);
static int vpu_check_is_decoder(u32 core, u32 inst);

static int device_hw_reset(void);
static int device_vpu_reset(void);
static void vpu_clk_disable(struct clk *clk);
static int vpu_clk_enable(struct clk *clk);
static struct clk *vpu_clk_get(struct device *dev);
static void vpu_clk_put(struct clk *clk);

/* end customer definition */
static vpudrv_buffer_t s_instance_pool = {0};
static vpudrv_buffer_t s_common_memory = {0};
static vpu_drv_context_t s_vpu_drv_context;
static int s_vpu_major;
static struct cdev s_vpu_cdev;
static struct class *vpu_class;


static struct clk *s_vpu_clk;
static int s_vpu_open_ref_count;
#ifdef VPU_SUPPORT_ISR
static int s_vpu_irq = VPU_IRQ_NUM;
#endif



static vpudrv_buffer_t s_vpu_register = {0};

static int s_interrupt_flag[MAX_NUM_INSTANCE];
static wait_queue_head_t s_interrupt_wait_q[MAX_NUM_INSTANCE];
typedef struct kfifo kfifo_t;
static kfifo_t s_interrupt_pending_q[MAX_NUM_INSTANCE];
static spinlock_t s_kfifo_lock = __SPIN_LOCK_UNLOCKED(s_kfifo_lock);

static spinlock_t s_vpu_lock = __SPIN_LOCK_UNLOCKED(s_vpu_lock);
static DEFINE_SEMAPHORE(s_vpu_sem);
static struct list_head s_vbp_head = LIST_HEAD_INIT(s_vbp_head);
static struct list_head s_inst_list_head = LIST_HEAD_INIT(s_inst_list_head);

static vpu_bit_firmware_info_t s_bit_firmware_info[MAX_NUM_VPU_CORE];
#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)
static u32	s_vpu_reg_store[MAX_NUM_VPU_CORE][64];
#endif
// BIT_RUN command
enum {
	DEC_SEQ_INIT = 1,
	ENC_SEQ_INIT = 1,
	DEC_SEQ_END = 2,
	ENC_SEQ_END = 2,
	PIC_RUN = 3,
	SET_FRAME_BUF = 4,
	ENCODE_HEADER = 5,
	ENC_PARA_SET = 6,
	DEC_PARA_SET = 7,
	DEC_BUF_FLUSH = 8,
	RC_CHANGE_PARAMETER    = 9,
	VPU_SLEEP = 10,
	VPU_WAKE = 11,
	ENC_ROI_INIT = 12,
	FIRMWARE_GET = 0xf
};
/**
* @brief    This is an enumeration for declaring return codes from API function calls.
The meaning of each return code is the same for all of the API functions, but the reasons of
non-successful return might be different. Some details of those reasons are
briefly described in the API definition chapter. In this chapter, the basic meaning
of each return code is presented.
*/
typedef enum {
	RETCODE_SUCCESS,                    /**< This means that operation was done successfully.  */  /* 0  */
	RETCODE_FAILURE,                    /**< This means that operation was not done successfully. When un-recoverable decoder error happens such as header parsing errors, this value is returned from VPU API.  */
	RETCODE_INVALID_HANDLE,             /**< This means that the given handle for the current API function call was invalid (for example, not initialized yet, improper function call for the given handle, etc.).  */
	RETCODE_INVALID_PARAM,              /**< This means that the given argument parameters (for example, input data structure) was invalid (not initialized yet or not valid anymore). */
	RETCODE_INVALID_COMMAND,            /**< This means that the given command was invalid (for example, undefined, or not allowed in the given instances).  */
	RETCODE_ROTATOR_OUTPUT_NOT_SET,     /**< This means that rotator output buffer was not allocated even though postprocessor (rotation, mirroring, or deringing) is enabled. */  /* 5  */
	RETCODE_ROTATOR_STRIDE_NOT_SET,     /**< This means that rotator stride was not provided even though postprocessor (rotation, mirroring, or deringing) is enabled.  */
	RETCODE_FRAME_NOT_COMPLETE,         /**< This means that frame decoding operation was not completed yet, so the given API function call cannot be allowed.  */
	RETCODE_INVALID_FRAME_BUFFER,       /**< This means that the given source frame buffer pointers were invalid in encoder (not initialized yet or not valid anymore).  */
	RETCODE_INSUFFICIENT_FRAME_BUFFERS, /**< This means that the given numbers of frame buffers were not enough for the operations of the given handle. This return code is only received when calling VPU_DecRegisterFrameBuffer() or VPU_EncRegisterFrameBuffer() function. */
	RETCODE_INVALID_STRIDE,             /**< This means that the given stride was invalid (for example, 0, not a multiple of 8 or smaller than picture size). This return code is only allowed in API functions which set stride.  */   /* 10 */
	RETCODE_WRONG_CALL_SEQUENCE,        /**< This means that the current API function call was invalid considering the allowed sequences between API functions (for example, missing one crucial function call before this function call).  */
	RETCODE_CALLED_BEFORE,              /**< This means that multiple calls of the current API function for a given instance are invalid. */
	RETCODE_NOT_INITIALIZED,            /**< This means that VPU was not initialized yet. Before calling any API functions, the initialization API function, VPU_Init(), should be called at the beginning.  */
	RETCODE_USERDATA_BUF_NOT_SET,       /**< This means that there is no memory allocation for reporting userdata. Before setting user data enable, user data buffer address and size should be set with valid value. */
	RETCODE_MEMORY_ACCESS_VIOLATION,    /**< This means that access violation to the protected memory has been occurred. */   /* 15 */
	RETCODE_VPU_RESPONSE_TIMEOUT,       /**< This means that VPU response time is too long, time out. */
	RETCODE_INSUFFICIENT_RESOURCE,      /**< This means that VPU cannot allocate memory due to lack of memory. */
	RETCODE_NOT_FOUND_BITCODE_PATH,     /**< This means that BIT_CODE_FILE_PATH has a wrong firmware path or firmware size is 0 when calling VPU_InitWithBitcode() function.  */
	RETCODE_NOT_SUPPORTED_FEATURE,      /**< This means that HOST application uses an API option that is not supported in current hardware.  */
	RETCODE_NOT_FOUND_VPU_DEVICE,       /**< This means that HOST application uses the undefined product ID. */   /* 20 */
	RETCODE_CP0_EXCEPTION,              /**< This means that coprocessor exception has occurred. (WAVE only) */
	RETCODE_STREAM_BUF_FULL,            /**< This means that stream buffer is full in encoder. */
	RETCODE_ACCESS_VIOLATION_HW,        /**< This means that GDI access error has occurred. It might come from violation of write protection region or spec-out GDI read/write request. (WAVE only) */
	RETCODE_QUERY_FAILURE,              /**< This means that query command was not successful. (WAVE5 only) */
	RETCODE_QUEUEING_FAILURE,           /**< This means that commands cannot be queued. (WAVE5 only) */
	RETCODE_VPU_STILL_RUNNING,          /**< This means that VPU cannot be flushed or closed now, because VPU is running. (WAVE5 only) */
	RETCODE_REPORT_NOT_READY,           /**< This means that report is not ready for Query(GET_RESULT) command. (WAVE5 only) */
	RETCODE_VLC_BUF_FULL,               /**< This means that VLC buffer is full in encoder. (WAVE5 only) */
	RETCODE_VPU_BUS_ERROR,              /**< This means that unrecoverable failure is occurred like as VPU bus error. In this case, host should call VPU_SwReset with SW_RESET_FORCE mode. (WAVE5 only) */
	RETCODE_INVALID_SFS_INSTANCE,       /**< This means that current instance can't run sub-framesync. (already an instance was running with sub-frame sync (WAVE5 only) */
} RetCode;

typedef enum {
	INT_WAVE5_INIT_VPU          = 0,
	INT_WAVE5_WAKEUP_VPU        = 1,
	INT_WAVE5_SLEEP_VPU         = 2,
	INT_WAVE5_CREATE_INSTANCE   = 3,
	INT_WAVE5_FLUSH_INSTANCE    = 4,
	INT_WAVE5_DESTROY_INSTANCE  = 5,
	INT_WAVE5_INIT_SEQ          = 6,
	INT_WAVE5_SET_FRAMEBUF      = 7,
	INT_WAVE5_DEC_PIC           = 8,
	INT_WAVE5_ENC_PIC           = 8,
	INT_WAVE5_ENC_SET_PARAM     = 9,
	INT_WAVE5_DEC_QUERY         = 14,
	INT_WAVE5_BSBUF_EMPTY       = 15,
	INT_WAVE5_BSBUF_FULL        = 15,
} Wave5InterruptBit;

typedef enum {
	INT_WAVE6_INIT_VPU          = 0,
	INT_WAVE6_WAKEUP_VPU        = 1,
	INT_WAVE6_SLEEP_VPU         = 2,
	INT_WAVE6_CREATE_INSTANCE   = 3,
	INT_WAVE6_FLUSH_INSTANCE    = 4,
	INT_WAVE6_DESTROY_INSTANCE  = 5,
	INT_WAVE6_INIT_SEQ          = 6,
	INT_WAVE6_SET_FRAMEBUF      = 7,
	INT_WAVE6_DEC_PIC           = 8,
	INT_WAVE6_ENC_PIC           = 8,
	INT_WAVE6_ENC_SET_PARAM     = 9,
	INT_WAVE6_BSBUF_EMPTY       = 15,
	INT_WAVE6_BSBUF_FULL        = 15,
} Wave6InterruptBit;

static vpudrv_instance_pool_t *get_instance_pool_handle(u32 core)
{
	int instance_pool_size_per_core;
	void *vip_base;

	if (core > MAX_NUM_VPU_CORE)
		return NULL;

	if (s_instance_pool.base == 0) {
		return NULL;
	}
	instance_pool_size_per_core = (s_instance_pool.size/MAX_NUM_VPU_CORE); /* s_instance_pool.size  assigned to the size of all core once call VDI_IOCTL_GET_INSTANCE_POOL by user. */
	vip_base = (void *)(s_instance_pool.base + (instance_pool_size_per_core*core));

	return (vpudrv_instance_pool_t *)vip_base;
}
static void *get_mutex_base(u32 core, unsigned int type)
{
	int instance_pool_size_per_core;
	void *vip_base;
	void *vdi_mutexes_base;
	void *mutex;

	if (core > MAX_NUM_VPU_CORE) {
		return NULL;
	}
	if (s_instance_pool.base == 0) {
		return NULL;
	}

	instance_pool_size_per_core = (s_instance_pool.size/MAX_NUM_VPU_CORE); /* s_instance_pool.size  assigned to the size of all core once call VDI_IOCTL_GET_INSTANCE_POOL by user. */
	vip_base = (void *)(s_instance_pool.base + (instance_pool_size_per_core*core));
	vdi_mutexes_base = (vip_base + (instance_pool_size_per_core - (sizeof(void *)*VDI_NUM_LOCK_HANDLES)));
	// DPRINTK("[VPUDRV]+%s vip_base=%p, vdi_mutexes_base=%p, instance_pool_size_per_core=%d, sizeof=%d, sizeof2=%d\n", __FUNCTION__, vip_base, vdi_mutexes_base, instance_pool_size_per_core, sizeof(void *), (sizeof(void *)*VDI_NUM_LOCK_HANDLES));
	if (type == VPUDRV_MUTEX_VPU) {
		mutex = (void *)(vdi_mutexes_base + 0*(sizeof(void *)));
	}
	else if (type == VPUDRV_MUTEX_DISP_FALG) {
		mutex = (void *)(vdi_mutexes_base + 1*(sizeof(void *)));
	}
	else if (type == VPUDRV_MUTEX_VMEM) {
		mutex = (void *)(vdi_mutexes_base + 2*(sizeof(void *)));
	}
	else if (type == VPUDRV_MUTEX_RESET) {
		mutex = (void *)(vdi_mutexes_base + 3*(sizeof(void *)));
	}
	else if (type == VPUDRV_MUTEX_REV1) {
		mutex = (void *)(vdi_mutexes_base + 4*(sizeof(void *)));
	}
	else if (type == VPUDRV_MUTEX_REV2) {
		mutex = (void *)(vdi_mutexes_base + 5*(sizeof(void *)));
	}
	else {
		DPRINTK("[VPUDRV]%s unknown MUTEX_TYPE type=%d\n", __FUNCTION__, type);
		return NULL;
	}
	return mutex;
}
static int vdi_lock(u32 core, unsigned int type)
{
	int ret;
	void *mutex;
	int count;
	int sync_ret;
	int sync_val = current->tgid;
	volatile int *sync_lock_ptr = NULL;

	mutex = get_mutex_base(core, type);
	if (mutex == NULL) {
		DPRINTK("[VPUDRV]%s fail to get mutex base, core=%d, type=%d\n", __FUNCTION__, core, type);
		return -1;
	}

	ret = 0;
	count = 0;
	sync_lock_ptr = (volatile int *)mutex;
	// DPRINTK("[VPUDRV]+%s type=%d, lock_ptr=%p, lock_data=%d, pid=%d, tgid=%d\n", __FUNCTION__, type, sync_lock_ptr, *sync_lock_ptr, current->pid, current->tgid);
	while((sync_ret = __sync_val_compare_and_swap(sync_lock_ptr, 0, sync_val)) != 0)
	{
		count++;
		if (count > ATOMIC_SYNC_TIMEOUT) {
			DPRINTK("[VPUDRV]%s failed to get lock type=%d, sync_ret=%d, sync_val=%d, sync_ptr=%d, pid=%d, tgid=%d \n", __FUNCTION__, type, sync_ret, sync_val, (int)*sync_lock_ptr, current->pid, current->tgid);
			ret = -ETIME;
			break;
		}
		msleep(1);
	}
	// DPRINTK("[VPUDRV]-%s, type=%d, ret=%d\n", __FUNCTION__, type, ret);
	return ret;
}
static void vdi_unlock(u32 core, unsigned int type)
{
	// DPRINTK("[VPUDRV]+%s, type=%d\n", __FUNCTION__, type);
	void *mutex;
	volatile int *sync_lock_ptr = NULL;

	mutex = get_mutex_base(core, type);
	if (mutex == NULL) {
		DPRINTK("[VPUDRV]%s fail to get mutex base, core=%d, type=%d\n", __FUNCTION__, core, type);
		return;
	}

	sync_lock_ptr = (volatile int *)mutex;
	__sync_lock_release(sync_lock_ptr);
	// DPRINTK("[VPUDRV]-%s, type=%d\n", __FUNCTION__, type);
}
static void vdi_lock_release(u32 core, unsigned int type)
{
	// DPRINTK("[VPUDRV]+%s, type=%d\n", __FUNCTION__, type);
	void *mutex;
	volatile int *sync_lock_ptr = NULL;

	mutex = get_mutex_base(core, type);
	if (mutex == NULL) {
		DPRINTK("[VPUDRV]%s fail to get mutex base, core=%d, type=%d\n", __FUNCTION__, core, type);
		return;
	}

	sync_lock_ptr = (volatile int *)mutex;
	DPRINTK("[VPUDRV]-%s core=%d, type=%d, lock_pid=%d, current_pid=%d, tgid=%d \n", __FUNCTION__, core, type, (volatile int)*sync_lock_ptr, current->pid, current->tgid);
	if (*sync_lock_ptr == current->tgid) {
		__sync_lock_release(sync_lock_ptr);
	}
	// DPRINTK("[VPUDRV]-%s\n", __FUNCTION__);
}
#define	ReadVpuRegister(addr)         *(volatile unsigned int *)(s_vpu_register.virt_addr + s_bit_firmware_info[core].reg_base_offset + addr)
#define	WriteVpuRegister(addr, val)   *(volatile unsigned int *)(s_vpu_register.virt_addr + s_bit_firmware_info[core].reg_base_offset + addr) = (unsigned int)val


static int vpu_alloc_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return -1;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	vb->phys_addr = (unsigned long)vmem_alloc(&s_vmem, vb->size, 0);
	if ((unsigned long)vb->phys_addr  == (unsigned long)-1) {
		pr_err("[VPUDRV] Physical memory allocation error size=%ld\n", vb->size);
		return -1;
	}

	vb->base = (unsigned long)(s_video_memory.base + (vb->phys_addr - s_video_memory.phys_addr));
	vb->virt_addr = vb->base;
#else
	vb->base = (unsigned long)dma_alloc_coherent(vpu_dev, PAGE_ALIGN(vb->size), (dma_addr_t *) (&vb->phys_addr), GFP_DMA | GFP_KERNEL);
	if ((void *)(vb->base) == NULL)	{
		pr_err("[VPUDRV] Physical memory allocation error size=%ld\n", vb->size);
		return -1;
	}
#endif
	return 0;
}


static void vpu_free_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (vb->base)
		vmem_free(&s_vmem, vb->phys_addr, 0);
#else
	if (vb->base)
		dma_free_coherent(vpu_dev, PAGE_ALIGN(vb->size), (void *)vb->base, vb->phys_addr);
#endif
}

static int vpu_free_instances(struct file *filp)
{
	vpudrv_instanace_list_t *vil, *n;
	vpudrv_instance_pool_t *vip;
	void *vip_base;
	int instance_pool_size_per_core;
	int i;

	DPRINTK("[VPUDRV]+%s\n", __FUNCTION__);

	instance_pool_size_per_core = (s_instance_pool.size/MAX_NUM_VPU_CORE); /* s_instance_pool.size  assigned to the size of all core once call VDI_IOCTL_GET_INSTANCE_POOL by user. */

	list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
	{
		if (vil->filp == filp) {
			vip_base = (void *)(s_instance_pool.base + (instance_pool_size_per_core*vil->core_idx));
			DPRINTK("[VPUDRV] vpu_free_instances detect instance crash instIdx=%d, coreIdx=%d, vip_base=%p, instance_pool_size_per_core=%d\n", (int)vil->inst_idx, (int)vil->core_idx, vip_base, (int)instance_pool_size_per_core);
			vip = (vpudrv_instance_pool_t *)vip_base;
			if (vip) {
				for (i=0; i < VPUDRV_MUTEX_MAX; i++) {
					vdi_lock_release((u32)vil->core_idx, i);
				}
				vpu_close_instance((u32)vil->core_idx, (u32)vil->inst_idx);
				memset(&vip->codecInstPool[vil->inst_idx], 0x00, 4);	/* only first 4 byte is key point(inUse of CodecInst in vpuapi) to free the corresponding instance. */
			}
			s_vpu_open_ref_count--;
			list_del(&vil->list);
			kfree(vil);
		}
	}
	DPRINTK("[VPUDRV]-%s\n", __FUNCTION__);
	return 1;
}

static int vpu_free_buffers(struct file *filp)
{
	vpudrv_buffer_pool_t *pool, *n;
	vpudrv_buffer_t vb;

	DPRINTK("[VPUDRV] vpu_free_buffers\n");

	list_for_each_entry_safe(pool, n, &s_vbp_head, list)
	{
		if (pool->filp == filp) {
			vb = pool->vb;
			if (vb.base) {
				vpu_free_dma_buffer(&vb);
				list_del(&pool->list);
				kfree(pool);
			}
		}
	}

	return 0;
}

static inline u32 get_inst_idx(u32 reg_val)
{
	u32 inst_idx;
	int i;
	for (i=0; i < MAX_NUM_INSTANCE; i++)
	{
		if(((reg_val >> i)&0x01) == 1)
			break;
	}
	inst_idx = i;
	return inst_idx;
}

static s32 get_vpu_inst_idx(vpu_drv_context_t *dev, u32 *reason, u32 empty_inst, u32 done_inst, u32 seq_inst)
{
	s32 inst_idx;
	u32 reg_val;
	u32 int_reason;

	int_reason = *reason;
	DPRINTK("[VPUDRV][+]%s, int_reason=0x%x, empty_inst=0x%x, done_inst=0x%x, product_code\n", __func__, int_reason, empty_inst, done_inst);
	//DPRINTK("[VPUDRV][+]%s, int_reason=0x%x, empty_inst=0x%x, done_inst=0x%x\n", __func__, int_reason, empty_inst, done_inst);

	if (int_reason & (1 << INT_WAVE5_DEC_PIC))
	{
		reg_val = done_inst;
		inst_idx = get_inst_idx(reg_val);
		*reason  = (1 << INT_WAVE5_DEC_PIC);
		DPRINTK("[VPUDRV]	%s, W5_RET_QUEUE_CMD_DONE_INST DEC_PIC reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
		goto GET_VPU_INST_IDX_HANDLED;
	}

	if (int_reason & (1 << INT_WAVE5_BSBUF_EMPTY))
	{
		reg_val = empty_inst;
		inst_idx = get_inst_idx(reg_val);
		*reason = (1 << INT_WAVE5_BSBUF_EMPTY);
		DPRINTK("[VPUDRV]	%s, W5_RET_BS_EMPTY_INST reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
		goto GET_VPU_INST_IDX_HANDLED;
	}

	if (int_reason & (1 << INT_WAVE5_INIT_SEQ))
	{
		reg_val = seq_inst;
		inst_idx = get_inst_idx(reg_val);
		*reason  = (1 << INT_WAVE5_INIT_SEQ);
		DPRINTK("[VPUDRV]	%s, RET_SEQ_DONE_INSTANCE_INFO INIT_SEQ reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
		goto GET_VPU_INST_IDX_HANDLED;
	}

	if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM))
	{
		reg_val = seq_inst;
		inst_idx = get_inst_idx(reg_val);
		*reason  = (1 << INT_WAVE5_ENC_SET_PARAM);
		DPRINTK("[VPUDRV]	%s, RET_SEQ_DONE_INSTANCE_INFO ENC_SET_PARAM reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
		goto GET_VPU_INST_IDX_HANDLED;
	}

#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
	if (int_reason & (1 << INT_WAVE5_ENC_SRC_RELEASE))
	{
		reg_val = done_inst;
		inst_idx = get_inst_idx(reg_val);
		*reason  = (1 << INT_WAVE5_ENC_SRC_RELEASE);
		DPRINTK("[VPUDRV]	%s, W5_RET_QUEUE_CMD_DONE_INST ENC_SET_PARAM reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
		goto GET_VPU_INST_IDX_HANDLED;
	}
#endif

	inst_idx = -1;
	*reason  = 0;
	DPRINTK("[VPUDRV]	%s, UNKNOWN INTERRUPT REASON: %08x\n", __func__, int_reason);

GET_VPU_INST_IDX_HANDLED:

	DPRINTK("[VPUDRV][-]%s, inst_idx=%d. *reason=0x%x\n", __func__, inst_idx, *reason);

	return inst_idx;
}

#ifdef VPU_SUPPORT_ISR
static irqreturn_t vpu_irq_handler(int irq, void *dev_id)
{
	vpu_drv_context_t *dev = (vpu_drv_context_t *)dev_id;

	/* this can be removed. it also work in VPU_WaitInterrupt of API function */
	int core;
	int product_code;
	u32 intr_reason;
	s32 intr_inst_index;
	DPRINTK("[VPUDRV][+]%s\n", __func__);

#ifdef VPU_IRQ_CONTROL
	disable_irq_nosync(s_vpu_irq);
#endif

	intr_inst_index = 0;
	intr_reason = 0;
	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		if (s_bit_firmware_info[core].size == 0) {/* it means that we didn't get an information the current core from API layer. No core activated.*/
			DPRINTK("[VPUDRV] :  s_bit_firmware_info[core].size is zero\n");
			continue;
		}
		product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

		if(PRODUCT_CODE_W6_SERIES(product_code)) {
			if (!(dev->support_cq)) {
				if (ReadVpuRegister(W6_VPU_VPU_INT_STS) > 0) {
					intr_reason = ReadVpuRegister(W6_VPU_VINT_REASON);

					intr_inst_index = 0; // in case of wave6 seriese. treats intr_inst_index is already 0(because of noCommandQueue
					kfifo_in_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
					WriteVpuRegister(W6_VPU_VINT_REASON_CLR, intr_reason);
					WriteVpuRegister(W6_VPU_VINT_CLEAR, 1);
					DPRINTK("[VPUDRV] vpu_irq_handler reason=0x%x\n", intr_reason);
				}
			} else {
				if (ReadVpuRegister(W6_VPU_VPU_INT_STS) > 0) {
					u32 done_inst;
					u32 i, reason, reason_clr;
					reason = ReadVpuRegister(W6_VPU_VINT_REASON);
					done_inst = ReadVpuRegister(W6_CMD_DONE_INST);
					reason_clr = reason;

					DPRINTK("[VPUDRV] vpu_irq_handler reason=0x%x, done_inst=0x%x\n", reason, done_inst);
					for (i=0; i<MAX_NUM_INSTANCE; i++) {
						if (0 == done_inst) {
							break;
						}
						if ( !(done_inst & (1<<i)))
							continue; // no done_inst in number i instance

						intr_reason     = reason;
						intr_inst_index = get_inst_idx(done_inst);

						if (intr_inst_index >= 0 && intr_inst_index < MAX_NUM_INSTANCE) {
							done_inst = done_inst & ~(1UL << intr_inst_index);
							WriteVpuRegister(W6_CMD_DONE_INST, done_inst);
							if (!kfifo_is_full(&s_interrupt_pending_q[intr_inst_index])) {
								kfifo_in_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
							}
							else {
								DPRINTK("[VPUDRV] :  kfifo_is_full kfifo_count=%d \n", kfifo_len(&s_interrupt_pending_q[intr_inst_index]));
							}
						}
						else {
							DPRINTK("[VPUDRV] :  intr_inst_index is wrong intr_inst_index=%d \n", intr_inst_index);
						}
					}

					WriteVpuRegister(W6_VPU_VINT_REASON_CLR, reason_clr);
					WriteVpuRegister(W6_VPU_VINT_CLEAR, 1);
				}
			}
		} else if (PRODUCT_CODE_W5_SERIES(product_code)) {
			if (ReadVpuRegister(W5_VPU_VPU_INT_STS) > 0) {
				u32 empty_inst;
				u32 done_inst;
				u32 seq_inst;
				u32 i, reason, reason_clr;

				#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
				ktime_get_ts64(&dev->irq_timestamp);
				#else
				ktime_get_ts(&dev->irq_timestamp);
				#endif

				reason     = ReadVpuRegister(W5_VPU_VINT_REASON);
				empty_inst = ReadVpuRegister(W5_RET_BS_EMPTY_INST);
				done_inst  = ReadVpuRegister(W5_RET_QUEUE_CMD_DONE_INST);
				seq_inst   = ReadVpuRegister(W5_RET_SEQ_DONE_INSTANCE_INFO);
				reason_clr = reason;

				DPRINTK("[VPUDRV] vpu_irq_handler reason=0x%x, empty_inst=0x%x, done_inst=0x%x, seq_inst=0x%x \n", reason, empty_inst, done_inst, seq_inst);
				for (i=0; i<MAX_NUM_INSTANCE; i++) {
					if (0 == empty_inst && 0 == done_inst && 0 == seq_inst) break;
					intr_reason = reason;
					intr_inst_index = get_vpu_inst_idx(dev, &intr_reason, empty_inst, done_inst, seq_inst);
					DPRINTK("[VPUDRV]     > instance_index: %d, intr_reason: %08x empty_inst: %08x done_inst: %08x seq_inst: %08x\n", intr_inst_index, intr_reason, empty_inst, done_inst, seq_inst);
					if (intr_inst_index >= 0 && intr_inst_index < MAX_NUM_INSTANCE) {
						if (intr_reason == (1 << INT_WAVE5_BSBUF_EMPTY)) {
							empty_inst = empty_inst & ~(1 << intr_inst_index);
							WriteVpuRegister(W5_RET_BS_EMPTY_INST, empty_inst);
							if (0 == empty_inst) {
								reason &= ~(1<<INT_WAVE5_BSBUF_EMPTY);
							}
							DPRINTK("[VPUDRV]	%s, W5_RET_BS_EMPTY_INST Clear empty_inst=0x%x, intr_inst_index=%d\n", __func__, empty_inst, intr_inst_index);
						}
						if (intr_reason == (1 << INT_WAVE5_DEC_PIC))
						{
							done_inst = done_inst & ~(1 << intr_inst_index);
							WriteVpuRegister(W5_RET_QUEUE_CMD_DONE_INST, done_inst);
							if (0 == done_inst) {
								reason &= ~(1<<INT_WAVE5_DEC_PIC);
							}
							DPRINTK("[VPUDRV]	%s, W5_RET_QUEUE_CMD_DONE_INST Clear done_inst=0x%x, intr_inst_index=%d\n", __func__, done_inst, intr_inst_index);
						}
						if ((intr_reason == (1 << INT_WAVE5_INIT_SEQ)) || (intr_reason == (1 << INT_WAVE5_ENC_SET_PARAM)))
						{
							seq_inst = seq_inst & ~(1 << intr_inst_index);
							WriteVpuRegister(W5_RET_SEQ_DONE_INSTANCE_INFO, seq_inst);
							if (0 == seq_inst) {
								reason &= ~(1<<INT_WAVE5_INIT_SEQ | 1<<INT_WAVE5_ENC_SET_PARAM);
							}
							DPRINTK("[VPUDRV]	%s, W5_RET_SEQ_DONE_INSTANCE_INFO Clear done_inst=0x%x, intr_inst_index=%d\n", __func__, done_inst, intr_inst_index);
						}
						if (!kfifo_is_full(&s_interrupt_pending_q[intr_inst_index])) {
							if (intr_reason == (1 << INT_WAVE5_ENC_PIC)) {
								u32 ll_intr_reason = (1 << INT_WAVE5_ENC_PIC);
								kfifo_in_spinlocked(&s_interrupt_pending_q[intr_inst_index], &ll_intr_reason, sizeof(u32), &s_kfifo_lock);
							}
							else
								kfifo_in_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
						}
						else {
							DPRINTK("[VPUDRV] :  kfifo_is_full kfifo_count=%d \n", kfifo_len(&s_interrupt_pending_q[intr_inst_index]));
						}
					}
					else {
						DPRINTK("[VPUDRV] :  intr_inst_index is wrong intr_inst_index=%d \n", intr_inst_index);
					}
				}

				if (0 != reason)
					DPRINTK("INTERRUPT REASON REMAINED: %08x\n", reason);
				WriteVpuRegister(W5_VPU_VINT_REASON_CLR, reason_clr);

				WriteVpuRegister(W5_VPU_VINT_CLEAR, 0x1);
			}
		}
		else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
			if (ReadVpuRegister(BIT_INT_STS)) {
				intr_reason = ReadVpuRegister(BIT_INT_REASON);
				intr_inst_index = 0; // in case of coda seriese. treats intr_inst_index is already 0
				kfifo_in_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
				WriteVpuRegister(BIT_INT_CLEAR, 0x1);
			}
		}
		else {
			DPRINTK("[VPUDRV] Unknown product id : %08x\n", product_code);
			continue;
		}
		DPRINTK("[VPUDRV] product: 0x%08x intr_reason: 0x%08x\n", product_code, intr_reason);
	}

	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);	/* notify the interrupt to user space */

	for (intr_inst_index = 0; intr_inst_index < MAX_NUM_INSTANCE; intr_inst_index++) {
		if (kfifo_len(&s_interrupt_pending_q[intr_inst_index])) {
			s_interrupt_flag[intr_inst_index] = 1;
			wake_up_interruptible(&s_interrupt_wait_q[intr_inst_index]);
		}
	}
	DPRINTK("[VPUDRV][-]%s\n\n", __func__);

	return IRQ_HANDLED;
}
#endif

static int vpu_open(struct inode *inode, struct file *filp)
{
	DPRINTK("[VPUDRV][+] %s\n", __func__);
	spin_lock(&s_vpu_lock);

	if (s_vpu_drv_context.open_count == 0) {
#ifdef VPU_SUPPORT_CLOCK_CONTROL
#else
		vpu_clk_enable(s_vpu_clk);
#endif
	}
	s_vpu_drv_context.open_count++;

	filp->private_data = (void *)(&s_vpu_drv_context);
	spin_unlock(&s_vpu_lock);

	DPRINTK("[VPUDRV][-] %s\n", __func__);

	return 0;
}

/*static int vpu_ioctl(struct inode *inode, struct file *filp, u_int cmd, u_long arg) // for kernel 2.6.9 of C&M*/
static long vpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	int ret = 0;
	int tv_i = 0;
	struct vpu_drv_context_t *dev = (struct vpu_drv_context_t *)filp->private_data;

	switch (cmd) {
	case VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY:
		{
			vpudrv_buffer_pool_t *vbp;

			DPRINTK("[VPUDRV][+]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");

			if ((ret = down_interruptible(&s_vpu_sem)) == 0) {
				vbp = kzalloc(sizeof(*vbp), GFP_KERNEL);
				if (!vbp) {
					up(&s_vpu_sem);
					return -ENOMEM;
				}

				ret = copy_from_user(&(vbp->vb), (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
				if (ret) {
					kfree(vbp);
					up(&s_vpu_sem);
					return -EFAULT;
				}

				ret = vpu_alloc_dma_buffer(&(vbp->vb));
				if (ret == -1) {
					ret = -ENOMEM;
					kfree(vbp);
					up(&s_vpu_sem);
					break;
				}
				ret = copy_to_user((void __user *)arg, &(vbp->vb), sizeof(vpudrv_buffer_t));
				if (ret) {
					kfree(vbp);
					ret = -EFAULT;
					up(&s_vpu_sem);
					break;
				}

				vbp->filp = filp;
				spin_lock(&s_vpu_lock);
				list_add(&vbp->list, &s_vbp_head);
				spin_unlock(&s_vpu_lock);

				up(&s_vpu_sem);
			}
			DPRINTK("[VPUDRV][-]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");
		}
		break;
	case VDI_IOCTL_FREE_PHYSICALMEMORY:
		{
			vpudrv_buffer_pool_t *vbp, *n;
			vpudrv_buffer_t vb;
			DPRINTK("[VPUDRV][+]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

			if ((ret = down_interruptible(&s_vpu_sem)) == 0) {

				ret = copy_from_user(&vb, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
				if (ret) {
					up(&s_vpu_sem);
					return -EACCES;
				}

				if (vb.base)
					vpu_free_dma_buffer(&vb);

				spin_lock(&s_vpu_lock);
				list_for_each_entry_safe(vbp, n, &s_vbp_head, list)
				{
					if (vbp->vb.base == vb.base) {
						list_del(&vbp->list);
						kfree(vbp);
						break;
					}
				}
				spin_unlock(&s_vpu_lock);

				up(&s_vpu_sem);
			}
			DPRINTK("[VPUDRV][-]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

		}
		break;
	case VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO:
		{
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
			DPRINTK("[VPUDRV][+]VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO\n");
			if (s_video_memory.base != 0) {
				ret = copy_to_user((void __user *)arg, &s_video_memory, sizeof(vpudrv_buffer_t));
				if (ret != 0)
					ret = -EFAULT;
			} else {
				ret = -EFAULT;
			}
			DPRINTK("[VPUDRV][-]VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO\n");
#endif
		}
		break;

	case VDI_IOCTL_WAIT_INTERRUPT:
		{
			vpudrv_intr_info_t info;
			u32 intr_inst_index;
			u32 intr_reason_in_q;
			u32 interrupt_flag_in_q;
			DPRINTK("[VPUDRV][+]VDI_IOCTL_WAIT_INTERRUPT\n");

			ret = copy_from_user(&info, (vpudrv_intr_info_t *)arg, sizeof(vpudrv_intr_info_t));
			if (ret != 0)
			{
				return -EFAULT;
			}
			intr_inst_index = info.intr_inst_index;

			intr_reason_in_q = 0;
			interrupt_flag_in_q = kfifo_out_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason_in_q, sizeof(u32), &s_kfifo_lock);
			if (interrupt_flag_in_q > 0)
			{
				dev->interrupt_reason[intr_inst_index] = intr_reason_in_q;
				// pr_err("[VPUDRV] Interrupt Remain : intr_inst_index=%d, intr_reason_in_q=0x%x, interrupt_flag_in_q=%d\n", intr_inst_index, intr_reason_in_q, interrupt_flag_in_q);
				goto INTERRUPT_REMAIN_IN_QUEUE;
			}
#ifdef SUPPORT_TIMEOUT_RESOLUTION
			kt =  ktime_set(0, info.timeout*1000*1000);
			ret = wait_event_interruptible_hrtimeout(s_interrupt_wait_q[intr_inst_index], s_interrupt_flag[intr_inst_index] != 0, kt);
#else
			ret = wait_event_interruptible_timeout(s_interrupt_wait_q[intr_inst_index], s_interrupt_flag[intr_inst_index] != 0, msecs_to_jiffies(info.timeout));
#endif
#ifdef SUPPORT_TIMEOUT_RESOLUTION
			if (ret == -ETIME) {
				//DPRINTK("[VPUDRV][-]VDI_IOCTL_WAIT_INTERRUPT timeout = %d \n", info.timeout);
				break;
			}
#endif
			if (!ret) {
				ret = -ETIME;
				break;
			}

			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}

			intr_reason_in_q = 0;
			interrupt_flag_in_q = kfifo_out_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason_in_q, sizeof(u32), &s_kfifo_lock);
			if (interrupt_flag_in_q > 0) {
				dev->interrupt_reason[intr_inst_index] = intr_reason_in_q;
			}
			else {
				dev->interrupt_reason[intr_inst_index] = 0;
			}
			DPRINTK("[VPUDRV] inst_index(%d), s_interrupt_flag(%d), reason(0x%08lx)\n", intr_inst_index, s_interrupt_flag[intr_inst_index], dev->interrupt_reason[intr_inst_index]);

INTERRUPT_REMAIN_IN_QUEUE:
			info.intr_reason = dev->interrupt_reason[intr_inst_index];
			s_interrupt_flag[intr_inst_index] = 0;
			dev->interrupt_reason[intr_inst_index] = 0;
			info.intr_tv_sec = dev->irq_timestamp.tv_sec;
			info.intr_tv_nsec = dev->irq_timestamp.tv_nsec;
#ifdef VPU_IRQ_CONTROL
			enable_irq(s_vpu_irq);
#endif
			// printk(" vpu_irq_handler  line=%d\n", __LINE__);
			struct timespec64 intr_tv;
			ktime_get_ts64(&intr_tv);

			int dis_idx = ReadVpuRegister(W5_RET_DEC_DISPLAY_INDEX);
			if ((info.intr_reason == 0X100) && (dis_idx >= 0)) {
			    if (s_fps_info[intr_inst_index].is_first != 0) {
				unsigned long consume_us = (intr_tv.tv_sec*1000*1000 + intr_tv.tv_nsec/1000) - (s_fps_info[intr_inst_index].intr_tv_last.tv_sec*1000*1000 + (s_fps_info[intr_inst_index].intr_tv_last.tv_nsec/1000));
				s_fps_info[intr_inst_index].consume_us_all += consume_us;
				s_fps_info[intr_inst_index].consume_times  += 1;
				int fps = (1000*1000*1000/(s_fps_info[intr_inst_index].consume_us_all/s_fps_info[intr_inst_index].consume_times))/1000;
				int fps_all = 0;
				for (tv_i =0; tv_i<8; tv_i++) {
					fps_all += (1000*1000*1000/(s_fps_info[tv_i].consume_us_all/s_fps_info[tv_i].consume_times))/1000;
				}
				// printk("===> core%d recv interrupt === intr_inst_index(%d) info.intr_reason=0X%x cur frame consume_us(%d)   consume_us_all=(%d)  consume_times=(%d) fps=(%d)  fps_all(%d). \n", core, intr_inst_index, info.intr_reason, consume_us, s_fps_info[intr_inst_index].consume_us_all, s_fps_info[intr_inst_index].consume_times, fps, fps_all);
				if (s_fps_info[intr_inst_index].consume_times == 200) {
					s_fps_info[intr_inst_index].consume_us_all = 0;
					s_fps_info[intr_inst_index].consume_times  = 0;
				}
			    } else {
				s_fps_info[intr_inst_index].is_first = 1;
                            }
			    s_fps_info[intr_inst_index].intr_tv_last.tv_sec  = intr_tv.tv_sec;
			    s_fps_info[intr_inst_index].intr_tv_last.tv_nsec = intr_tv.tv_nsec;
			}
			// printk("intr_tv_last tv_sec=%d  tv_nsec=%d \n", s_fps_info[intr_inst_index].intr_tv_last.tv_sec, s_fps_info[intr_inst_index].intr_tv_last.tv_nsec);
			// printk("vpu_irq_handler  line=%d  intr_inst_index=%d dev=0X%x  info.intr_reason=0X%x \n", __LINE__, intr_inst_index, dev, info.intr_reason);

			ret = copy_to_user((void __user *)arg, &info, sizeof(vpudrv_intr_info_t));
			DPRINTK("[VPUDRV][-]VDI_IOCTL_WAIT_INTERRUPT\n");
			if (ret != 0)
			{
				return -EFAULT;
			}
		}
		break;
	case VDI_IOCTL_SET_CLOCK_GATE:
		{
			u32 clkgate;

			DPRINTK("[VPUDRV][+]VDI_IOCTL_SET_CLOCK_GATE\n");
			if (get_user(clkgate, (u32 __user *) arg))
				return -EFAULT;
#ifdef VPU_SUPPORT_CLOCK_CONTROL
			if (clkgate)
				vpu_clk_enable(s_vpu_clk);
			else
				vpu_clk_disable(s_vpu_clk);
#endif
			DPRINTK("[VPUDRV][-]VDI_IOCTL_SET_CLOCK_GATE\n");

		}
		break;
	case VDI_IOCTL_GET_INSTANCE_POOL:
		{
			DPRINTK("[VPUDRV][+]VDI_IOCTL_GET_INSTANCE_POOL\n");
			if ((ret = down_interruptible(&s_vpu_sem)) == 0) {
				if (s_instance_pool.base != 0) {
					ret = copy_to_user((void __user *)arg, &s_instance_pool, sizeof(vpudrv_buffer_t));
					if (ret != 0)
						ret = -EFAULT;
				} else {
					ret = copy_from_user(&s_instance_pool, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
					if (ret == 0) {
						s_instance_pool.base = (unsigned long)vmalloc(PAGE_ALIGN(s_instance_pool.size));
						s_instance_pool.phys_addr = s_instance_pool.base;

						if (s_instance_pool.base != 0) {
							memset((void *)s_instance_pool.base, 0x0, PAGE_ALIGN(s_instance_pool.size)); /*clearing memory*/
							ret = copy_to_user((void __user *)arg, &s_instance_pool, sizeof(vpudrv_buffer_t));
							if (ret == 0) {
								/* success to get memory for instance pool */
								up(&s_vpu_sem);
								break;
							}
						}
					}
					ret = -EFAULT;
				}
				up(&s_vpu_sem);
			}
			DPRINTK("[VPUDRV][-]VDI_IOCTL_GET_INSTANCE_POOL\n");
		}
		break;
	case VDI_IOCTL_GET_COMMON_MEMORY:
		{
			DPRINTK("[VPUDRV][+]VDI_IOCTL_GET_COMMON_MEMORY\n");
			if (s_common_memory.size != 0) {
				ret = copy_to_user((void __user *)arg, &s_common_memory, sizeof(vpudrv_buffer_t));
				if (ret != 0)
					ret = -EFAULT;
			} else {
				ret = copy_from_user(&s_common_memory, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
				if (ret == 0) {
					if (vpu_alloc_dma_buffer(&s_common_memory) != -1) {
						ret = copy_to_user((void __user *)arg, &s_common_memory, sizeof(vpudrv_buffer_t));
						if (ret == 0) {
							/* success to get memory for common memory */
							break;
						}
					}
				}
				ret = -EFAULT;
			}
			DPRINTK("[VPUDRV][-]VDI_IOCTL_GET_COMMON_MEMORY\n");
			//we set ve top regsiter here for encoder
			if (devid == 0) {
				ve_virt_top = (unsigned long)IOREMAP(VE_TOP_EXT_ADDR, 4);
				*(volatile unsigned int *)ve_virt_top = 1;
			}
		}
		break;
	case VDI_IOCTL_OPEN_INSTANCE:
		{
			vpudrv_inst_info_t inst_info;
			vpudrv_instanace_list_t *vil, *n;

			vil = kzalloc(sizeof(*vil), GFP_KERNEL);
			if (!vil)
				return -ENOMEM;
			if (copy_from_user(&inst_info, (vpudrv_inst_info_t *)arg, sizeof(vpudrv_inst_info_t))) {
				kfree(vil);
				return -EFAULT;
			}

			vil->inst_idx = inst_info.inst_idx;
			vil->core_idx = inst_info.core_idx;
			vil->filp = filp;

			spin_lock(&s_vpu_lock);
			list_add(&vil->list, &s_inst_list_head);

			inst_info.inst_open_count = 0; /* counting the current open instance number */
			list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
			{
				if (vil->core_idx == inst_info.core_idx)
					inst_info.inst_open_count++;
			}

			if (inst_info.inst_open_count == 1) {
				dev->support_cq = inst_info.support_cq;
			}
			kfifo_reset(&s_interrupt_pending_q[inst_info.inst_idx]);
			spin_unlock(&s_vpu_lock);

			s_vpu_open_ref_count++; /* flag just for that vpu is in opened or closed */

			if (copy_to_user((void __user *)arg, &inst_info, sizeof(vpudrv_inst_info_t))) {
				kfree(vil);
				return -EFAULT;
			}

			DPRINTK("[VPUDRV] VDI_IOCTL_OPEN_INSTANCE core_idx=%d, inst_idx=%d, s_vpu_open_ref_count=%d, inst_open_count=%d\n", (int)inst_info.core_idx, (int)inst_info.inst_idx, s_vpu_open_ref_count, inst_info.inst_open_count);
		}
		break;
	case VDI_IOCTL_CLOSE_INSTANCE:
		{
			vpudrv_inst_info_t inst_info;
			vpudrv_instanace_list_t *vil, *n;
			u32 found = 0;

			DPRINTK("[VPUDRV][+]VDI_IOCTL_CLOSE_INSTANCE\n");
			if (copy_from_user(&inst_info, (vpudrv_inst_info_t *)arg, sizeof(vpudrv_inst_info_t)))
				return -EFAULT;

			spin_lock(&s_vpu_lock);
			list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
			{
				if (vil->inst_idx == inst_info.inst_idx && vil->core_idx == inst_info.core_idx) {
					list_del(&vil->list);
					kfree(vil);
					found = 1;
					break;
				}
			}

			if (0 == found) {
				spin_unlock(&s_vpu_lock);
				return -EINVAL;
			}

			inst_info.inst_open_count = 0; /* counting the current open instance number */
			list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
			{
				if (vil->core_idx == inst_info.core_idx)
					inst_info.inst_open_count++;
			}
			kfifo_reset(&s_interrupt_pending_q[inst_info.inst_idx]);
			spin_unlock(&s_vpu_lock);

			s_vpu_open_ref_count--; /* flag just for that vpu is in opened or closed */

			if (copy_to_user((void __user *)arg, &inst_info, sizeof(vpudrv_inst_info_t)))
				return -EFAULT;

			DPRINTK("[VPUDRV] VDI_IOCTL_CLOSE_INSTANCE core_idx=%d, inst_idx=%d, s_vpu_open_ref_count=%d, inst_open_count=%d\n", (int)inst_info.core_idx, (int)inst_info.inst_idx, s_vpu_open_ref_count, inst_info.inst_open_count);
		}
		break;
	case VDI_IOCTL_GET_INSTANCE_NUM:
		{
			vpudrv_inst_info_t inst_info;
			vpudrv_instanace_list_t *vil, *n;
			DPRINTK("[VPUDRV][+]VDI_IOCTL_GET_INSTANCE_NUM\n");

			ret = copy_from_user(&inst_info, (vpudrv_inst_info_t *)arg, sizeof(vpudrv_inst_info_t));
			if (ret != 0)
				break;

			spin_lock(&s_vpu_lock);
			inst_info.inst_open_count = 0;
			list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
			{
				if (vil->core_idx == inst_info.core_idx)
					inst_info.inst_open_count++;
			}
			spin_unlock(&s_vpu_lock);

			ret = copy_to_user((void __user *)arg, &inst_info, sizeof(vpudrv_inst_info_t));

			DPRINTK("[VPUDRV] VDI_IOCTL_GET_INSTANCE_NUM core_idx=%d, inst_idx=%d, open_count=%d\n", (int)inst_info.core_idx, (int)inst_info.inst_idx, inst_info.inst_open_count);
		}
		break;
	case VDI_IOCTL_RESET:
		{
			device_hw_reset();
		}
		break;
	case VDI_IOCTL_VPU_RESET:
		{
			device_vpu_reset();
		}
		break;
	case VDI_IOCTL_GET_REGISTER_INFO:
		{
			DPRINTK("[VPUDRV][+]VDI_IOCTL_GET_REGISTER_INFO\n");
			ret = copy_to_user((void __user *)arg, &s_vpu_register, sizeof(vpudrv_buffer_t));
			if (ret != 0)
				ret = -EFAULT;
			DPRINTK("[VPUDRV][-]VDI_IOCTL_GET_REGISTER_INFO s_vpu_register.phys_addr==0x%lx, s_vpu_register.virt_addr=0x%lx, s_vpu_register.size=%d\n", s_vpu_register.phys_addr , s_vpu_register.virt_addr, s_vpu_register.size);
		}
		break;
	default:
		{
			DPRINTK("[VPUDRV] No such IOCTL, cmd is %d\n", cmd);
		}
		break;
	}
	return ret;
}

static ssize_t vpu_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{

	return -1;
}

static ssize_t vpu_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{

	/* DPRINTK("[VPUDRV] vpu_write len=%d\n", (int)len); */
	if (!buf) {
		pr_err("[VPUDRV] vpu_write buf = NULL error \n");
		return -EFAULT;
	}

	if (len == sizeof(vpu_bit_firmware_info_t))	{
		vpu_bit_firmware_info_t *bit_firmware_info;

		bit_firmware_info = kmalloc(sizeof(vpu_bit_firmware_info_t), GFP_KERNEL);
		if (!bit_firmware_info) {
			pr_err("[VPUDRV] vpu_write  bit_firmware_info allocation error \n");
			return -EFAULT;
		}

		if (copy_from_user(bit_firmware_info, buf, len)) {
			pr_err("[VPUDRV] vpu_write copy_from_user error for bit_firmware_info\n");
			kfree(bit_firmware_info);
			return -EFAULT;
		}

		if (bit_firmware_info->size == sizeof(vpu_bit_firmware_info_t)) {
			DPRINTK("[VPUDRV] vpu_write set bit_firmware_info coreIdx=0x%x, reg_base_offset=0x%x size=0x%x, bit_code[0]=0x%x\n",
			bit_firmware_info->core_idx, (int)bit_firmware_info->reg_base_offset, bit_firmware_info->size, bit_firmware_info->bit_code[0]);

			if (bit_firmware_info->core_idx > MAX_NUM_VPU_CORE) {
				DPRINTK("[VPUDRV] vpu_write coreIdx[%d] is exceeded than MAX_NUM_VPU_CORE[%d]\n", bit_firmware_info->core_idx, MAX_NUM_VPU_CORE);
				return -ENODEV;
			}

			memcpy(&s_bit_firmware_info[bit_firmware_info->core_idx], bit_firmware_info, sizeof(vpu_bit_firmware_info_t));
			kfree(bit_firmware_info);

			return len;
		}

		kfree(bit_firmware_info);
	}

	return -1;
}

static int vpu_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	u32 open_count;
	int i;
	pr_err("[VPUDRV] +vpu_release\n");

	if ((ret = down_interruptible(&s_vpu_sem)) == 0) {

		/* found and free the not handled buffer by user applications */
		vpu_free_buffers(filp);

		/* found and free the not closed instance by user applications */
		vpu_free_instances(filp);
		spin_lock(&s_vpu_lock);
		s_vpu_drv_context.open_count--;
		open_count = s_vpu_drv_context.open_count;
		spin_unlock(&s_vpu_lock);
		if (open_count == 0) {
			for (i=0; i<MAX_NUM_INSTANCE; i++) {
				kfifo_reset(&s_interrupt_pending_q[i]);
			}
			if (s_instance_pool.base) {
				pr_err("[VPUDRV] free instance pool\n");
				vfree((const void *)s_instance_pool.base);
				s_instance_pool.base = 0;
			}
#ifdef VPU_SUPPORT_CLOCK_CONTROL
#else
			vpu_clk_disable(s_vpu_clk);
#endif
		}
	}
	up(&s_vpu_sem);
	pr_err("[VPUDRV] - vpu_release\n");

	return 0;
}

static int vpu_fasync(int fd, struct file *filp, int mode)
{
	struct vpu_drv_context_t *dev = (struct vpu_drv_context_t *)filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}


static int vpu_map_to_register(struct file *fp, struct vm_area_struct *vm)
{
	unsigned long pfn;

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = s_vpu_register.phys_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end-vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}

static int vpu_map_to_physical_memory(struct file *fp, struct vm_area_struct *vm)
{
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff, vm->vm_end-vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}

static int vpu_map_to_instance_pool_memory(struct file *fp, struct vm_area_struct *vm)
{
	int ret;
	long length = vm->vm_end - vm->vm_start;
	unsigned long start = vm->vm_start;
	char *vmalloc_area_ptr = (char *)s_instance_pool.base;
	unsigned long pfn;

	vm->vm_flags |= VM_RESERVED;

	/* loop over all pages, map it page individually */
	while (length > 0)
	{
		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		if ((ret = remap_pfn_range(vm, start, pfn, PAGE_SIZE, PAGE_SHARED)) < 0) {
				return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;
	}


	return 0;
}

/*!
 * @brief memory map interface for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_mmap(struct file *fp, struct vm_area_struct *vm)
{
	if (vm->vm_pgoff == 0)
		return vpu_map_to_instance_pool_memory(fp, vm);

	if (vm->vm_pgoff == (s_vpu_register.phys_addr>>PAGE_SHIFT))
		return vpu_map_to_register(fp, vm);

	return vpu_map_to_physical_memory(fp, vm);
}

#ifdef CONFIG_COMPAT
static long vpu_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

struct file_operations vpu_fops = {
	.owner = THIS_MODULE,
	.open = vpu_open,
	.read = vpu_read,
	.write = vpu_write,
	/*.ioctl = vpu_ioctl, // for kernel 2.6.9 of C&M*/
	.unlocked_ioctl = vpu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vpu_compat_ptr_ioctl,
#endif
	.release = vpu_release,
	.fasync = vpu_fasync,
	.mmap = vpu_mmap,
};
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
static int vpu_reserved_memory_anayze(struct platform_device *pdev)
{
	struct device_node *target = NULL;
	struct reserved_mem *prmem = NULL;
#if 0
	if (pdev) {
		target =
			of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	}

	if (!target) {
        pr_err("Cannot find node 'memory-region'\n");
        return -EINVAL;
    }

	if (target) {
		prmem = of_reserved_mem_lookup(target);
		of_node_put(target);

		if (!prmem) {
			pr_err(": cannot acquire memory-region\n");
			//return -1;
		}
	} else {
		pr_err(": cannot find the node, memory-region\n");
		//return -1;
	}
	pr_err("vpu dts prmem_base:%llx size:%x\n",prmem->base, prmem->size);
#else
	prmem = vmalloc(sizeof(struct reserved_mem));
	if(devid == 0) {
		// prmem->base = 0x182000000;
		// prmem->size = 0x20000000;
	} else if(devid == 1){
		prmem->base = 0x60000000;
		prmem->size = 0x20000000;
	}
	else if(devid == 2){
		prmem->base = 0x80000000;
		prmem->size = 0x20000000;
	}
	pr_err("vpu phy prmem_base:%llx size:%x\n",prmem->base, prmem->size);
#endif

	pr_err("vpu prmem_base:%llx size:%x\n",prmem->base, prmem->size);
	s_video_memory.size      = (unsigned int)prmem->size;
    s_video_memory.phys_addr = (unsigned long)prmem->base;
    s_video_memory.base = s_video_memory.phys_addr;//(unsigned long)IOREMAP(s_video_memory.phys_addr, PAGE_ALIGN(s_video_memory.size));

    pr_err("[VPUDRV] success to probe jpu device with reserved video memory phys_addr=0x%lx, base=0x%lx\n", s_video_memory.phys_addr, s_video_memory.base);


	return 0;
}
#endif


static int vpu_probe(struct platform_device *pdev)
{
	int err = 0;

	char *vdev_name;
	struct resource *res = NULL;

	DPRINTK("[VPUDRV] vpu_probe\n");
	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {/* if platform driver is implemented */
		s_vpu_register.phys_addr = res->start;
		s_vpu_register.virt_addr = (unsigned long)IOREMAP(res->start, res->end - res->start);
		s_vpu_register.size = res->end - res->start;
		pr_err("[VPUDRV] : vpu base address get from platform driver physical base addr==0x%lx, virtual base=0x%lx\n", s_vpu_register.phys_addr , s_vpu_register.virt_addr);
	} else {
		s_vpu_register.phys_addr = VPU_REG_BASE_ADDR;
		s_vpu_register.virt_addr = (unsigned long)IOREMAP(s_vpu_register.phys_addr, VPU_REG_SIZE);
		s_vpu_register.size = VPU_REG_SIZE;
		pr_err("[VPUDRV] : vpu base address get from defined value physical base addr==0x%lx, virtual base=0x%lx\n", s_vpu_register.phys_addr, s_vpu_register.virt_addr);
	}
	if(res)
		device_property_read_u32(&pdev->dev, "dev-id", &devid);
	pr_err("[VPUDRV] devid %d\n",devid);
	core = devid;
	vdev_name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "%s%d", VPU_DEV_NAME,devid);

	vpu_class = class_create(THIS_MODULE, vdev_name);
	if (IS_ERR(vpu_class))
	{
		DPRINTK("create class failed\n");
		return PTR_ERR(vpu_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&s_vpu_major, 0, 1, vdev_name)) < 0) {
		err = -EBUSY;
		DPRINTK("could not allocate major number\n");
		goto ERROR_PROVE_DEVICE;
	}
	pr_info("SUCCESS alloc_chrdev_region\n");

	/* initialize the device structure and register the device with the kernel */
	cdev_init(&s_vpu_cdev, &vpu_fops);
	s_vpu_cdev.owner = THIS_MODULE;
	if ((cdev_add(&s_vpu_cdev, s_vpu_major, 1)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate chrdev\n");

		goto ERROR_PROVE_DEVICE;
	}

	device_create(vpu_class, &pdev->dev, s_vpu_major, NULL, "%s", vdev_name);
#if 0
	if (pdev)
		s_vpu_clk = vpu_clk_get(&pdev->dev);
	else
		s_vpu_clk = vpu_clk_get(NULL);

	if (!s_vpu_clk)
		DPRINTK("[VPUDRV] : not support clock controller.\n");
	else
		DPRINTK("[VPUDRV] : get clock controller s_vpu_clk=%p\n", s_vpu_clk);
#endif

#ifdef VPU_SUPPORT_ISR
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res) {/* if platform driver is implemented */
		s_vpu_irq = res->start;
		DPRINTK("[VPUDRV] : vpu irq number get from platform driver irq=0x%x\n", s_vpu_irq);
	} else {
		DPRINTK("[VPUDRV] : vpu irq number get from defined value irq=0x%x\n", s_vpu_irq);
	}
#else
	DPRINTK("[VPUDRV] : vpu irq number get from defined value irq=0x%x\n", s_vpu_irq);
#endif

	err = request_irq(s_vpu_irq, vpu_irq_handler, IRQF_TRIGGER_RISING, "VPU_CODEC_IRQ", (void *)(&s_vpu_drv_context));
	if (err) {
		pr_err("[VPUDRV] :  fail to register interrupt handler\n");
		goto ERROR_PROVE_DEVICE;
	}
#endif
	vpu_dev = &pdev->dev;
	dma_set_mask_and_coherent(vpu_dev, DMA_BIT_MASK(64));

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
#if 1
	if (vpu_reserved_memory_anayze(pdev) < 0) {
		pr_err("fail to vpu_allocate_memory\n");
		goto ERROR_PROVE_DEVICE;
	}
#else
	s_video_memory.size = VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE;
	s_video_memory.phys_addr = VPU_DRAM_PHYSICAL_BASE;
	s_video_memory.base = (unsigned long)IOREMAP(s_video_memory.phys_addr, PAGE_ALIGN(s_video_memory.size));
#endif
	if (!s_video_memory.base) {
		pr_err("[VPUDRV] :  fail to remap video memory physical phys_addr==0x%lx, base==0x%lx, size=%ld\n", s_video_memory.phys_addr, s_video_memory.base, s_video_memory.size);
		goto ERROR_PROVE_DEVICE;
	}


	if (vmem_init(&s_vmem, s_video_memory.phys_addr, s_video_memory.size) < 0) {
		pr_err("[VPUDRV] :  fail to init vmem system\n");
		goto ERROR_PROVE_DEVICE;
	}
	pr_err("[VPUDRV] success to probe vpu device with reserved video memory phys_addr==0x%lx, base = =0x%lx\n", s_video_memory.phys_addr, s_video_memory.base);
#else
	DPRINTK("[VPUDRV] success to probe vpu device with non reserved video memory\n");
#endif

	//ve_virt_top = (unsigned long)IOREMAP(VE_TOP_EXT_ADDR, 4);
	// *(volatile unsigned int *)ve_virt_top = 1;

	return 0;

ERROR_PROVE_DEVICE:

	if (s_vpu_major)
		unregister_chrdev_region(s_vpu_major, 1);

	if (s_vpu_register.virt_addr)
		iounmap((void *)s_vpu_register.virt_addr);

	return err;
}
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
static int vpu_remove(struct platform_device *pdev)
{
	DPRINTK("[VPUDRV] vpu_remove\n");

	if (s_instance_pool.base) {
		vfree((const void *)s_instance_pool.base);
		s_instance_pool.base = 0;
	}

	if (s_common_memory.base) {
		vpu_free_dma_buffer(&s_common_memory);
		s_common_memory.base = 0;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) {
		iounmap((void *)s_video_memory.base);
		s_video_memory.base = 0;
		vmem_exit(&s_vmem);
	}
#endif

	if (s_vpu_major > 0) {
		device_destroy(vpu_class, s_vpu_major);
		cdev_del(&s_vpu_cdev);
		unregister_chrdev_region(s_vpu_major, 1);
		class_destroy(vpu_class);
		s_vpu_major = 0;
	}

#ifdef VPU_SUPPORT_ISR
	if (s_vpu_irq)
		free_irq(s_vpu_irq, &s_vpu_drv_context);
#endif

	if (s_vpu_register.virt_addr)
		iounmap((void *)s_vpu_register.virt_addr);

	vpu_clk_put(s_vpu_clk);
	iounmap(ve_virt_top);

	return 0;
}
#endif /*VPU_SUPPORT_PLATFORM_DRIVER_REGISTER*/

#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)

static int vpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	int core;
	int ret;

	DPRINTK("[VPUDRV] vpu_suspend\n");

	vpu_clk_enable(s_vpu_clk);

	if (s_vpu_open_ref_count > 0) {
		for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
			if (s_bit_firmware_info[core].size == 0)
				continue;
			ret = vpu_sleep_wake(core, VPU_SLEEP_MODE);
			if (ret != VPUAPI_RET_SUCCESS) {
				goto DONE_SUSPEND;
			}
		}
	}

	//vpu_clk_disable(s_vpu_clk);
	return 0;

DONE_SUSPEND:

	//vpu_clk_disable(s_vpu_clk);

	return -EAGAIN;

}
static int vpu_resume(struct platform_device *pdev)
{
	int core;
	int ret;

	DPRINTK("[VPUDRV] vpu_resume\n");

	vpu_clk_enable(s_vpu_clk);

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		if (s_bit_firmware_info[core].size == 0) {
			continue;
		}
		ret = vpu_sleep_wake(core, VPU_WAKE_MODE);
		if (ret != VPUAPI_RET_SUCCESS) {
			goto DONE_WAKEUP;
		}
	}

	if (s_vpu_open_ref_count == 0)
		vpu_clk_disable(s_vpu_clk);

DONE_WAKEUP:

	if (s_vpu_open_ref_count > 0)
		vpu_clk_enable(s_vpu_clk);

	return 0;
}
#else
#define	vpu_suspend	NULL
#define	vpu_resume	NULL
#endif				/* !CONFIG_PM */

#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER

static const struct of_device_id vpu_match_table[] = {
    {.compatible = "sophgo,vpu1"},
    {},
};


static struct platform_driver vpu_driver = {
	.driver = {
		   .name = VPU_PLATFORM_DEVICE_NAME,
		   .of_match_table = vpu_match_table,
		   },
	.probe = vpu_probe,
	.remove = vpu_remove,
	.suspend = vpu_suspend,
	.resume = vpu_resume,
};
#endif /* VPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

static int __init vpu_init(void)
{
	int res;
	int i;
	DPRINTK("[VPUDRV] begin vpu_init\n");
	for (i=0; i<MAX_NUM_INSTANCE; i++) {
		init_waitqueue_head(&s_interrupt_wait_q[i]);
	}


	for (i=0; i < 32; i++) {
		s_fps_info[i].is_first = 0;
		s_fps_info[i].intr_tv_last.tv_sec = 0;
		s_fps_info[i].intr_tv_last.tv_nsec = 0;
		s_fps_info[i].consume_us_all = 0;
		s_fps_info[i].consume_times = 0;
	}



	for (i=0; i<MAX_NUM_INSTANCE; i++) {
#define MAX_INTERRUPT_QUEUE (16*MAX_NUM_INSTANCE)
		res = kfifo_alloc(&s_interrupt_pending_q[i], MAX_INTERRUPT_QUEUE*sizeof(u32), GFP_KERNEL);
		if (res) {
			DPRINTK("[VPUDRV] kfifo_alloc failed 0x%x\n", res);
		}
	}
	s_common_memory.base = 0;
	s_instance_pool.base = 0;
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	res = platform_driver_register(&vpu_driver);
#else
	res = vpu_probe(NULL);
#endif /* VPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

	DPRINTK("[VPUDRV] end vpu_init result=0x%x\n", res);
	return res;
}

static void __exit vpu_exit(void)
{
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
	DPRINTK("[VPUDRV] vpu_exit\n");

	platform_driver_unregister(&vpu_driver);

#else /* VPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

#ifdef VPU_SUPPORT_CLOCK_CONTROL
#else
	vpu_clk_disable(s_vpu_clk);
#endif
	vpu_clk_put(s_vpu_clk);

	if (s_instance_pool.base) {
		vfree((const void *)s_instance_pool.base);
		s_instance_pool.base = 0;
	}

	if (s_common_memory.size) {
		vpu_free_dma_buffer(&s_common_memory);
        s_common_memory.size = 0;
		s_common_memory.base = 0;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) {
		//iounmap((void *)s_video_memory.base);
		//s_video_memory.base = 0;

		vmem_exit(&s_vmem);
	}
#endif

	if (s_vpu_major > 0) {
		cdev_del(&s_vpu_cdev);
		unregister_chrdev_region(s_vpu_major, 1);
		s_vpu_major = 0;
	}

#ifdef VPU_SUPPORT_ISR
	if (s_vpu_irq)
		free_irq(s_vpu_irq, &s_vpu_drv_context);
#endif

	{
		int i;
		for (i=0; i<MAX_NUM_INSTANCE; i++) {
			kfifo_free(&s_interrupt_pending_q[i]);
		}
	}

	if (s_vpu_register.virt_addr) {
		iounmap((void *)s_vpu_register.virt_addr);
		s_vpu_register.virt_addr = 0x00;
	}

#endif /* VPU_SUPPORT_PLATFORM_DRIVER_REGISTER */

	return;
}

MODULE_AUTHOR("sophgo");
MODULE_DESCRIPTION("VPU linux driver");
MODULE_LICENSE("GPL");


module_init(vpu_init);
module_exit(vpu_exit);

int device_hw_reset(void)
{
	DPRINTK("[VPUDRV] request device hw reset from application. \n");
	return 0;
}

int device_vpu_reset(void)
{
	DPRINTK("[VPUDRV] request device vpu reset from application. \n");
	return 0;
}

struct clk *vpu_clk_get(struct device *dev)
{
	return clk_get(dev, VPU_CLK_NAME);
}
void vpu_clk_put(struct clk *clk)
{
	if (!(clk == NULL || IS_ERR(clk)))
		clk_put(clk);
}
int vpu_clk_enable(struct clk *clk)
{
		if (!(clk == NULL || IS_ERR(clk))) {
		/* the bellow is for C&M EVB.*/
		/*
		{
			struct clk *s_vpuext_clk = NULL;
			s_vpuext_clk = clk_get(NULL, "vcore");
			if (s_vpuext_clk)
			{
				DPRINTK("[VPUDRV] vcore clk=%p\n", s_vpuext_clk);
				clk_enable(s_vpuext_clk);
			}

			DPRINTK("[VPUDRV] vbus clk=%p\n", s_vpuext_clk);
			if (s_vpuext_clk)
			{
				s_vpuext_clk = clk_get(NULL, "vbus");
				clk_enable(s_vpuext_clk);
			}
		}
		*/
		/* for C&M EVB. */

		DPRINTK("[VPUDRV] vpu_clk_enable\n");
		//customers needs implementation to turn on clock like clk_enable(clk)
		return 1;
	}

	return 0;
}

void vpu_clk_disable(struct clk *clk)
{
	if (!(clk == NULL || IS_ERR(clk))) {
		DPRINTK("[VPUDRV] vpu_clk_disable\n");
		//customers needs implementation to turn off clock like clk_disable(clk)
	}
}



#define FIO_TIMEOUT         100
static void WriteVpuFIORegister(u32 core, u32 addr, u32 data)
{
	unsigned int ctrl;
	unsigned int count = 0;
	WriteVpuRegister(W5_VPU_FIO_DATA, data);
	ctrl  = (addr&0xffff);
	ctrl |= (1<<16);    /* write operation */
	WriteVpuRegister(W5_VPU_FIO_CTRL_ADDR, ctrl);

	count = FIO_TIMEOUT;
	while (count--) {
		ctrl = ReadVpuRegister(W5_VPU_FIO_CTRL_ADDR);
		if (ctrl & 0x80000000) {
			break;
		}
	}
}
static u32 ReadVpuFIORegister(u32 core, u32 addr)
{
	u32 ctrl;
	u32 count = 0;
	u32 data  = 0xffffffff;

	ctrl  = (addr&0xffff);
	ctrl |= (0<<16);    /* read operation */
	WriteVpuRegister(W5_VPU_FIO_CTRL_ADDR, ctrl);
	count = FIO_TIMEOUT;
	while (count--) {
		ctrl = ReadVpuRegister(W5_VPU_FIO_CTRL_ADDR);
		if (ctrl & 0x80000000) {
			data = ReadVpuRegister(W5_VPU_FIO_DATA);
			break;
		}
	}

	return data;
}

static int vpuapi_wait_reset_busy(u32 core)
{
	int ret;
	u32 val = 0;
	int product_code;
	unsigned long timeout = jiffies + VPU_BUSY_CHECK_TIMEOUT;

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
	while(1)
	{
		if (PRODUCT_CODE_W5_SERIES(product_code)) {
			val = ReadVpuRegister(W5_VPU_RESET_STATUS);
		}

		if (val == 0) {
			ret = VPUAPI_RET_SUCCESS;
			break;
		}

		if (time_after(jiffies, timeout)) {
			DPRINTK("vpuapi_wait_reset_busy after BUSY timeout");
			ret = VPUAPI_RET_TIMEOUT;
			break;
		}
		udelay(0);	// delay more to give idle time to OS;
	}

	return ret;
}

static int vpuapi_wait_vpu_busy(u32 core, u32 reg)
{
	int ret;
	u32 val = 0;
	u32 cmd;
	u32 pc;
	int product_code;
	unsigned long timeout = jiffies + VPU_BUSY_CHECK_TIMEOUT;

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
	while(1)
	{
		if (PRODUCT_CODE_W6_SERIES(product_code)) {
			val = ReadVpuRegister(reg);
			cmd = ReadVpuRegister(W6_COMMAND);
			pc = ReadVpuRegister(W6_VCPU_CUR_PC);
		} else if (PRODUCT_CODE_W5_SERIES(product_code)) {
			val = ReadVpuRegister(reg);
			cmd = ReadVpuRegister(W5_COMMAND);
			pc = ReadVpuRegister(W5_VCPU_CUR_PC);
		} else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
			val = ReadVpuRegister(reg);
			cmd = ReadVpuRegister(BIT_RUN_COMMAND);
			pc = ReadVpuRegister(BIT_CUR_PC);
		}

		if (val == 0) {
			ret = VPUAPI_RET_SUCCESS;
			break;
		}

		if (time_after(jiffies, timeout)) {
			DPRINTK("%s timeout cmd=0x%x, pc=0x%x\n", __FUNCTION__, cmd, pc);
			ret = VPUAPI_RET_TIMEOUT;
			break;
		}
		udelay(0);	// delay more to give idle time to OS;
	}

	return ret;
}
static int vpuapi_wait_bus_busy(u32 core, u32 bus_busy_reg_addr)
{
	int ret;
	u32 val;
	int product_code;
	unsigned long timeout = jiffies + VPU_BUSY_CHECK_TIMEOUT;

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
	ret = VPUAPI_RET_SUCCESS;
	while(1)
	{
		if (PRODUCT_CODE_W5_SERIES(product_code)) {
			val = ReadVpuFIORegister(core, bus_busy_reg_addr);
			if (val == 0x3f)
				break;
		} else {
			ret = VPUAPI_RET_INVALID_PARAM;
			break;
		}

		if (time_after(jiffies, timeout)) {
			DPRINTK("%s timeout \n", __FUNCTION__);
			ret = VPUAPI_RET_TIMEOUT;
			break;
		}
		udelay(0);	// delay more to give idle time to OS;
	}

	return ret;
}
#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)
static int coda_sleep_wake(u32 core, int mode)
{
	int i;
	u32 val;
	unsigned long timeout = jiffies + VPU_DEC_TIMEOUT;
	if (mode == VPU_SLEEP_MODE) {
		while (ReadVpuRegister(BIT_BUSY_FLAG)) {
			if (time_after(jiffies, timeout)) {
				return VPUAPI_RET_TIMEOUT;
			}
		}

		for (i = 0; i < 64; i++) {
			s_vpu_reg_store[core][i] = ReadVpuRegister(BIT_BASE+(0x100+(i * 4)));
		}
	}
	else {
		WriteVpuRegister(BIT_CODE_RUN, 0);

		/*---- LOAD BOOT CODE*/
		for (i = 0; i < 512; i++) {
			val = s_bit_firmware_info[core].bit_code[i];
			WriteVpuRegister(BIT_CODE_DOWN, ((i << 16) | val));
		}

		for (i = 0 ; i < 64 ; i++)
			WriteVpuRegister(BIT_BASE+(0x100+(i * 4)), s_vpu_reg_store[core][i]);

		WriteVpuRegister(BIT_BUSY_FLAG, 1);
		WriteVpuRegister(BIT_CODE_RESET, 1);
		WriteVpuRegister(BIT_CODE_RUN, 1);

		while (ReadVpuRegister(BIT_BUSY_FLAG)) {
			if (time_after(jiffies, timeout)) {
				return VPUAPI_RET_TIMEOUT;
			}
		}
	}

	return VPUAPI_RET_SUCCESS;
}
#endif
// PARAMETER
/// mode 0 => wake
/// mode 1 => sleep
// return
static int wave_sleep_wake(u32 core, int mode)
{
	u32 val;
	if (mode == VPU_SLEEP_MODE) {
		if (vpuapi_wait_vpu_busy(core, W5_VPU_BUSY_STATUS) == VPUAPI_RET_TIMEOUT) {
			return VPUAPI_RET_TIMEOUT;
		}

		WriteVpuRegister(W5_VPU_BUSY_STATUS, 1);
		WriteVpuRegister(W5_COMMAND, W5_SLEEP_VPU);
		WriteVpuRegister(W5_VPU_HOST_INT_REQ, 1);

		if (vpuapi_wait_vpu_busy(core, W5_VPU_BUSY_STATUS) == VPUAPI_RET_TIMEOUT) {
			return VPUAPI_RET_TIMEOUT;
		}

		if (ReadVpuRegister(W5_RET_SUCCESS) == 0) {
			val = ReadVpuRegister(W5_RET_FAIL_REASON);
			if (val == WAVE5_SYSERR_VPU_STILL_RUNNING) {
				return VPUAPI_RET_STILL_RUNNING;
			}
			else {
				return VPUAPI_RET_FAILURE;
			}
		}
	}
	else {
		int i;
		u32 val;
		u32 remapSize;
		u32 codeBase;
		u32 codeSize;

		WriteVpuRegister(W5_PO_CONF, 0);
		for (i=W5_CMD_REG_END; i < W5_CMD_REG_END; i++)
		{
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
			if (i == W5_SW_UART_STATUS)
				continue;
#endif
			if (i == W5_RET_BS_EMPTY_INST || i == W5_RET_QUEUE_CMD_DONE_INST || i == W5_RET_SEQ_DONE_INSTANCE_INFO)
				continue;

			WriteVpuRegister(i, 0);
		}

		codeBase = s_common_memory.phys_addr;
		codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);

		remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
		val = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (W_REMAP_INDEX0<<12) | (0<<16) | (1<<11) | remapSize;
		WriteVpuRegister(W5_VPU_REMAP_CTRL,  val);
		WriteVpuRegister(W5_VPU_REMAP_VADDR, W_REMAP_INDEX0*W_REMAP_MAX_SIZE);
		WriteVpuRegister(W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX0*W_REMAP_MAX_SIZE);

		remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
		val = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (W_REMAP_INDEX1<<12) | (0<<16) | (1<<11) | remapSize;
		WriteVpuRegister(W5_VPU_REMAP_CTRL,  val);
		WriteVpuRegister(W5_VPU_REMAP_VADDR, W_REMAP_INDEX1*W_REMAP_MAX_SIZE);
		WriteVpuRegister(W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX1*W_REMAP_MAX_SIZE);

		WriteVpuRegister(W5_ADDR_CODE_BASE,  codeBase);
		WriteVpuRegister(W5_CODE_SIZE,       codeSize);
		WriteVpuRegister(W5_CODE_PARAM,      (WAVE_UPPER_PROC_AXI_ID<<4) | 0);
		WriteVpuRegister(W5_HW_OPTION,       0);
		// encoder
		val  = (1<<INT_WAVE5_ENC_SET_PARAM);
		val |= (1<<INT_WAVE5_ENC_PIC);
		val |= (1<<INT_WAVE5_BSBUF_FULL);
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
		val |= (1<<INT_WAVE5_ENC_SRC_RELEASE);
#endif
		// decoder
		val |= (1<<INT_WAVE5_INIT_SEQ);
		val |= (1<<INT_WAVE5_DEC_PIC);
		val |= (1<<INT_WAVE5_BSBUF_EMPTY);
		WriteVpuRegister(W5_VPU_VINT_ENABLE,  val);

	val = ReadVpuRegister(W5_VPU_RET_VPU_CONFIG0);
	if (((val>>16)&1) == 1) {
		val = ((WAVE5_PROC_AXI_ID<<28)  |
				  (WAVE5_PRP_AXI_ID<<24)   |
				  (WAVE5_FBD_Y_AXI_ID<<20) |
				  (WAVE5_FBC_Y_AXI_ID<<16) |
				  (WAVE5_FBD_C_AXI_ID<<12) |
				  (WAVE5_FBC_C_AXI_ID<<8)  |
				  (WAVE5_PRI_AXI_ID<<4)    |
				  (WAVE5_SEC_AXI_ID<<0));
		WriteVpuFIORegister(core, W5_BACKBONE_PROG_AXI_ID, val);
	}

		WriteVpuRegister(W5_VPU_BUSY_STATUS, 1);
		WriteVpuRegister(W5_COMMAND, W5_WAKEUP_VPU);
		WriteVpuRegister(W5_VPU_REMAP_CORE_START, 1);

		if (vpuapi_wait_vpu_busy(core, W5_VPU_BUSY_STATUS) == VPUAPI_RET_TIMEOUT) {
			return VPUAPI_RET_TIMEOUT;
		}

		val = ReadVpuRegister(W5_RET_SUCCESS);
		if (val == 0) {
			return VPUAPI_RET_FAILURE;
		}

	}
	return VPUAPI_RET_SUCCESS;
}
#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)
static int wave6_sleep_wake(u32 core, int mode)
{
	u32 val;
	if (mode == VPU_SLEEP_MODE) {
		if (vpuapi_wait_vpu_busy(core, W6_VPU_BUSY_STATUS) == VPUAPI_RET_TIMEOUT)
			return VPUAPI_RET_STILL_RUNNING;
	} else {
		u32 val;
		u32 codeBase;
		u32 i;

		codeBase = s_common_memory.phys_addr;

		for (i = 0; i < WAVE6_MAX_CODE_BUF_SIZE/W_REMAP_MAX_SIZE; i++) {
			val = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (i<<12) | (1<<11) | ((W_REMAP_MAX_SIZE>>12)&0x1ff);
			WriteVpuRegister(W6_VPU_REMAP_CTRL,  val);
			WriteVpuRegister(W6_VPU_REMAP_VADDR, i*W_REMAP_MAX_SIZE);
			WriteVpuRegister(W6_VPU_REMAP_PADDR, codeBase + i*W_REMAP_MAX_SIZE);
		}

		/* Interrupt */
		val = 0;
		// encoder
		val  = (1<<INT_WAVE6_ENC_SET_PARAM);
		val |= (1<<INT_WAVE6_ENC_PIC);
		val |= (1<<INT_WAVE6_BSBUF_FULL);
		// decoder
		val |= (1<<INT_WAVE6_INIT_SEQ);
		val |= (1<<INT_WAVE6_DEC_PIC);
		val |= (1<<INT_WAVE6_BSBUF_EMPTY);
		WriteVpuRegister(W6_VPU_VINT_ENABLE, val);

		WriteVpuRegister(W6_VPU_CMD_BUSY_STATUS, 1);
		WriteVpuRegister(W6_COMMAND, W6_WAKEUP_VPU);
		WriteVpuRegister(W6_VPU_REMAP_CORE_START, 1);

		if (vpuapi_wait_vpu_busy(core, W6_VPU_CMD_BUSY_STATUS) == VPUAPI_RET_TIMEOUT) {
			return VPUAPI_RET_TIMEOUT;
		}

		val = ReadVpuRegister(W6_RET_SUCCESS);
		if (val == 0) {
			return VPUAPI_RET_FAILURE;
		}

	}
	return VPUAPI_RET_SUCCESS;
}
#endif
static int coda_close_instance(u32 core, u32 inst)
{
	int ret = 0;

	DPRINTK("[VPUDRV]+%s core=%d, inst=%d\n", __FUNCTION__, core, inst);
	if (vpu_check_is_decoder(core, inst) == 1) {
		vpuapi_dec_set_stream_end(core, inst);
	}

	ret = vpuapi_wait_vpu_busy(core, BIT_BUSY_FLAG);
	if (ret != VPUAPI_RET_SUCCESS) {
		goto HANDLE_ERROR;
	}

	ret = vpuapi_close(core, inst);
	if (ret != VPUAPI_RET_SUCCESS) {
		goto HANDLE_ERROR;
	}

	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	return 1;
HANDLE_ERROR:
	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	return 0;
}

static int wave_close_instance(u32 core, u32 inst)
{
	int ret;
	u32 error_reason = 0;
	unsigned long timeout = jiffies + VPU_DEC_TIMEOUT;

	DPRINTK("[VPUDRV]+%s core=%d, inst=%d\n", __FUNCTION__, core, inst);
	if (vpu_check_is_decoder(core, inst) == 1) {
		ret = vpuapi_dec_set_stream_end(core, inst);
		ret = vpuapi_dec_clr_all_disp_flag(core, inst);
	}
	while ((ret = vpuapi_close(core, inst)) == VPUAPI_RET_STILL_RUNNING) {
		ret = vpuapi_get_output_info(core, inst, &error_reason);
		DPRINTK("[VPUDRV]%s core=%d, inst=%d, ret=%d, error_reason=0x%x\n", __FUNCTION__, core, inst, ret, error_reason);
		if (ret == VPUAPI_RET_SUCCESS) {
			if ((error_reason & 0xf0000000)) {
				if (vpu_do_sw_reset(core, inst, error_reason) == VPUAPI_RET_TIMEOUT) {
					break;
				}
			}
		}

		if (vpu_check_is_decoder(core, inst) == 1) {
			ret = vpuapi_dec_set_stream_end(core, inst);
			ret = vpuapi_dec_clr_all_disp_flag(core, inst);
		}

		msleep(10);	// delay for vpuapi_close

		if (time_after(jiffies, timeout)) {
			DPRINTK("[VPUDRV]%s vpuapi_close flow timeout ret=%d, inst=%d\n", __FUNCTION__, ret, inst);
			goto HANDLE_ERROR;
		}
	}


	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	return 1;
HANDLE_ERROR:
	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	return 0;
}

static int wave6_close_instance(u32 core, u32 inst)
{
	int ret = 0;

	DPRINTK("[VPUDRV]+%s core=%d, inst=%d\n", __FUNCTION__, core, inst);
	if (vpu_check_is_decoder(core, inst) == 1) {
		vpuapi_dec_set_stream_end(core, inst);
	}

	ret = vpuapi_wait_vpu_busy(core, W6_VPU_CMD_BUSY_STATUS);
	if (ret != VPUAPI_RET_SUCCESS) {
		goto HANDLE_ERROR;
	}

	ret = vpuapi_close(core, inst);
	if (ret != VPUAPI_RET_SUCCESS) {
		goto HANDLE_ERROR;
	}

	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	return 1;
HANDLE_ERROR:
	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	return 0;
}

int vpu_check_is_decoder(u32 core, u32 inst)
{
	u32 is_decoder;
	unsigned char *codec_inst;
	vpudrv_instance_pool_t *vip = get_instance_pool_handle(core);
	DPRINTK("[VPUDRV]+%s\n", __FUNCTION__);

	if (vip == NULL) {
		return 0;
	}

	codec_inst = &vip->codecInstPool[inst][0];
	codec_inst = codec_inst + (sizeof(u32) * 7); // indicates isDecoder in CodecInst structure in vpuapifunc.h
	memcpy(&is_decoder, codec_inst, 4);

	DPRINTK("[VPUDRV]-%s is_decoder=0x%x\n", __FUNCTION__, is_decoder);
	return (is_decoder == 1)?1:0;
}

int vpu_close_instance(u32 core, u32 inst)
{
	u32 product_code;
	int success;
	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
	DPRINTK("[VPUDRV]+%s core=%d, inst=%d, product_code=0x%x\n", __FUNCTION__, core, inst, product_code);

	if (PRODUCT_CODE_W6_SERIES(product_code)) {
		success = wave6_close_instance(core, inst);
	} else if (PRODUCT_CODE_W5_SERIES(product_code)) {
		success = wave_close_instance(core, inst);
	} else if(PRODUCT_CODE_CODA_SERIES(product_code)){
		success = coda_close_instance(core, inst);
	} else {
		DPRINTK("[VPUDRV]vpu_close_instance Unknown product id : %08x\n", product_code);
		success = 0;
	}

	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, success);
	return success;
}
#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)
int vpu_sleep_wake(u32 core, int mode)
{
	int inst;
	int ret = VPUAPI_RET_SUCCESS;
	int product_code;
	unsigned long timeout = jiffies + VPU_DEC_TIMEOUT;
	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

	if (PRODUCT_CODE_W6_SERIES(product_code)) {
		if (mode == VPU_SLEEP_MODE) {
			while ((ret = wave6_sleep_wake(core, VPU_SLEEP_MODE)) == VPUAPI_RET_STILL_RUNNING) {
				msleep(10);
				if (time_after(jiffies, timeout)) {
					return VPUAPI_RET_TIMEOUT;
				}
			}
		}
		else {
			ret = wave6_sleep_wake(core, VPU_WAKE_MODE);
		}
	} else if (PRODUCT_CODE_W5_SERIES(product_code)) {
		if (mode == VPU_SLEEP_MODE) {
			while((ret = wave_sleep_wake(core, VPU_SLEEP_MODE)) == VPUAPI_RET_STILL_RUNNING) {
				for (inst = 0; inst < MAX_NUM_INSTANCE; inst++) {
				}
				msleep(10);
				if (time_after(jiffies, timeout)) {
					return VPUAPI_RET_TIMEOUT;
				}
			}
		}
		else {
			ret = wave_sleep_wake(core, VPU_WAKE_MODE);
		}
	} else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
		if (mode == VPU_SLEEP_MODE) {
			ret = coda_sleep_wake(core, VPU_SLEEP_MODE);
		}
		else {
			ret = coda_sleep_wake(core, VPU_WAKE_MODE);
		}
	}
	return ret;
}
#endif
// PARAMETER
// reset_mode
// 0 : safely
// 1 : force
int vpuapi_sw_reset(u32 core, u32 inst, int reset_mode)
{
	u32 val = 0;
	int product_code;
	int ret;
	u32 supportDualCore;
	u32 supportBackbone;
	u32 supportVcoreBackbone;
	u32 supportVcpuBackbone;
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	u32 regSwUartStatus;
#endif
	vdi_lock(core, VPUDRV_MUTEX_VPU);

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
	if (PRODUCT_CODE_W5_SERIES(product_code)) {
		WriteVpuRegister(W5_VPU_BUSY_STATUS, 0);
		DPRINTK("[VPUDRV]%s mode=%d\n", __FUNCTION__, reset_mode);
		if (reset_mode == 0) {
			ret = wave_sleep_wake(core, VPU_SLEEP_MODE);
			DPRINTK("[VPUDRV]%s Sleep done ret=%d\n", __FUNCTION__, ret);
			if (ret != VPUAPI_RET_SUCCESS) {
				vdi_unlock(core, VPUDRV_MUTEX_VPU);
				return ret;
			}
		}

		val = ReadVpuRegister(W5_VPU_RET_VPU_CONFIG0);
		//Backbone
		if (((val>>16) & 0x1) == 0x01) {
			supportBackbone = 1;
		} else {
			supportBackbone = 0;
		}
		//VCore Backbone
		if (((val>>22) & 0x1) == 0x01) {
			supportVcoreBackbone = 1;
		} else {
			supportVcoreBackbone = 0;
		}
		//VPU Backbone
		if (((val>>28) & 0x1) == 0x01) {
			supportVcpuBackbone = 1;
		} else {
			supportVcpuBackbone = 0;
		}

		val = ReadVpuRegister(W5_VPU_RET_VPU_CONFIG1);
		//Dual Core
		if (((val>>26) & 0x1) == 0x01) {
			supportDualCore = 1;
		} else {
			supportDualCore = 0;
		}

		if (supportBackbone == 1) {
			if (supportDualCore == 1) {

				WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x7);
				if (vpuapi_wait_bus_busy(core, W5_BACKBONE_BUS_STATUS_VCORE0) != VPUAPI_RET_SUCCESS) {
					WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
					vdi_unlock(core, VPUDRV_MUTEX_VPU);
					return VPUAPI_RET_TIMEOUT;
				}

				WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE1, 0x7);
				if (vpuapi_wait_bus_busy(core, W5_BACKBONE_BUS_STATUS_VCORE1) != VPUAPI_RET_SUCCESS) {
					WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE1, 0x00);
					vdi_unlock(core, VPUDRV_MUTEX_VPU);
					return VPUAPI_RET_TIMEOUT;
				}
			}
			else {
				if (supportVcoreBackbone == 1) {
					if (supportVcpuBackbone == 1) {
						// Step1 : disable request
						WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCPU, 0xFF);

						// Step2 : Waiting for completion of bus transaction
						if (vpuapi_wait_bus_busy(core, W5_BACKBONE_BUS_STATUS_VCPU) != VPUAPI_RET_SUCCESS) {
							WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCPU, 0x00);
							vdi_unlock(core, VPUDRV_MUTEX_VPU);
							return VPUAPI_RET_TIMEOUT;
						}
					}
					// Step1 : disable request
					WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x7);

					// Step2 : Waiting for completion of bus transaction
					if (vpuapi_wait_bus_busy(core, W5_BACKBONE_BUS_STATUS_VCORE0) != VPUAPI_RET_SUCCESS) {
						WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
						vdi_unlock(core, VPUDRV_MUTEX_VPU);
						return VPUAPI_RET_TIMEOUT;
					}
				}
				else {
					// Step1 : disable request
					WriteVpuFIORegister(core, W5_COMBINED_BACKBONE_BUS_CTRL, 0x7);

					// Step2 : Waiting for completion of bus transaction
					if (vpuapi_wait_bus_busy(core, W5_COMBINED_BACKBONE_BUS_STATUS) != VPUAPI_RET_SUCCESS) {
						WriteVpuFIORegister(core, W5_COMBINED_BACKBONE_BUS_CTRL, 0x00);
						vdi_unlock(core, VPUDRV_MUTEX_VPU);
						return VPUAPI_RET_TIMEOUT;
					}
				}
			}
		}
		else {
			// Step1 : disable request
			WriteVpuFIORegister(core, W5_GDI_BUS_CTRL, 0x100);

			// Step2 : Waiting for completion of bus transaction
			if (vpuapi_wait_bus_busy(core, W5_GDI_BUS_STATUS) != VPUAPI_RET_SUCCESS) {
				WriteVpuFIORegister(core, W5_GDI_BUS_CTRL, 0x00);
				vdi_unlock(core, VPUDRV_MUTEX_VPU);
				return VPUAPI_RET_TIMEOUT;
			}
		}

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
		regSwUartStatus = ReadVpuRegister(W5_SW_UART_STATUS);
#endif
		val = W5_RST_BLOCK_ALL;
		WriteVpuRegister(W5_VPU_RESET_REQ, val);

		if (vpuapi_wait_reset_busy(core) != VPUAPI_RET_SUCCESS) {
			WriteVpuRegister(W5_VPU_RESET_REQ, 0);
			vdi_unlock(core, VPUDRV_MUTEX_VPU);
			return VPUAPI_RET_TIMEOUT;
		}

		WriteVpuRegister(W5_VPU_RESET_REQ, 0);
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
		WriteVpuRegister(W5_SW_UART_STATUS, regSwUartStatus); // enable SW UART.
#endif

		DPRINTK("[VPUDRV]%s VPU_RESET done RESET_REQ=0x%x\n", __FUNCTION__, val);

		// Step3 : must clear GDI_BUS_CTRL after done SW_RESET
		if (supportBackbone == 1) {
			if (supportDualCore == 1) {
				WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
				WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE1, 0x00);
			} else {
				if (supportVcoreBackbone == 1) {
					if (supportVcpuBackbone == 1) {
						WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCPU, 0x00);
					}
					WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
				} else {
					WriteVpuFIORegister(core, W5_COMBINED_BACKBONE_BUS_CTRL, 0x00);
				}
			}
		} else {
			WriteVpuFIORegister(core, W5_GDI_BUS_CTRL, 0x00);
		}
	} else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
		DPRINTK("[VPUDRV] %s doesn't support swreset for coda \n", __FUNCTION__);
		vdi_unlock(core, VPUDRV_MUTEX_VPU);
		return VPUAPI_RET_INVALID_PARAM;
	} else if (PRODUCT_CODE_W6_SERIES(product_code)) {
		DPRINTK("[VPUDRV] %s doesn't support swreset for wave6 \n", __FUNCTION__);
		vdi_unlock(core, VPUDRV_MUTEX_VPU);
		return VPUAPI_RET_INVALID_PARAM;
	} else {
		DPRINTK("[VPUDRV] Unknown product code : %08x\n", product_code);
		return -1;
	}

	ret = wave_sleep_wake(core, VPU_WAKE_MODE);

	DPRINTK("[VPUDRV]%s Wake done ret = %d\n", __FUNCTION__, ret);
	vdi_unlock(core, VPUDRV_MUTEX_VPU);

	return ret;
}
static int coda_issue_command(u32 core, u32 inst, u32 cmd)
{
	int ret;

	// WriteVpuRegister(BIT_WORK_BUF_ADDR, inst->CodecInfo->decInfo.vbWork.phys_addr);
	WriteVpuRegister(BIT_BUSY_FLAG, 1);
	WriteVpuRegister(BIT_RUN_INDEX, inst);
	// WriteVpuRegister(BIT_RUN_COD_STD, codec_mode);
	// WriteVpuRegister(BIT_RUN_AUX_STD, codec_aux_mode)
	WriteVpuRegister(BIT_RUN_COMMAND, cmd);

	ret = vpuapi_wait_vpu_busy(core, BIT_BUSY_FLAG);

	return ret;
}

static int wave6_issue_command(u32 core, u32 inst, u32 cmd)
{
	int ret;
	u32 codec_mode;
	unsigned char *codec_inst;
	vpudrv_instance_pool_t *vip = get_instance_pool_handle(core);

	if (vip == NULL) {
		return VPUAPI_RET_INVALID_PARAM;
	}

	codec_inst = &vip->codecInstPool[inst][0];
	codec_inst = codec_inst + (sizeof(u32)* 3); // indicates codecMode in CodecInst structure in vpuapifunc.h
	memcpy(&codec_mode, codec_inst, 4);

	WriteVpuRegister(W6_CMD_INSTANCE_INFO, (codec_mode << 16) | (inst & 0xffff));
	WriteVpuRegister(W6_VPU_CMD_BUSY_STATUS, 1);
	WriteVpuRegister(W6_COMMAND, cmd);

	WriteVpuRegister(W6_VPU_HOST_INT_REQ, 1);

	ret = vpuapi_wait_vpu_busy(core, W6_VPU_CMD_BUSY_STATUS);

	return ret;
}

static int wave_issue_command(u32 core, u32 inst, u32 cmd)
{
	int ret;
	u32 codec_mode;
	unsigned char *codec_inst;
	vpudrv_instance_pool_t *vip = get_instance_pool_handle(core);

	if (vip == NULL) {
		return VPUAPI_RET_INVALID_PARAM;
	}

	codec_inst = &vip->codecInstPool[inst][0];
	codec_inst = codec_inst + (sizeof(u32) * 3); // indicates codecMode in CodecInst structure in vpuapifunc.h
	memcpy(&codec_mode, codec_inst, 4);

	WriteVpuRegister(W5_CMD_INSTANCE_INFO, (codec_mode<<16)|(inst&0xffff));
	WriteVpuRegister(W5_VPU_BUSY_STATUS, 1);
	WriteVpuRegister(W5_COMMAND, cmd);

	WriteVpuRegister(W5_VPU_HOST_INT_REQ, 1);

	ret = vpuapi_wait_vpu_busy(core, W5_VPU_BUSY_STATUS);

	return ret;
}

static int wave_send_query_command(unsigned long core, unsigned long inst, u32 queryOpt)
{
	int ret;
	WriteVpuRegister(W5_QUERY_OPTION, queryOpt);
	WriteVpuRegister(W5_VPU_BUSY_STATUS, 1);
	ret = wave_issue_command(core, inst, W5_QUERY);
	if (ret != VPUAPI_RET_SUCCESS) {
		DPRINTK("[VPUDRV]%s fail1 ret=%d\n", __FUNCTION__, ret);
		return ret;
	}

	if (ReadVpuRegister(W5_RET_SUCCESS) == 0) {
		DPRINTK("[VPUDRV]%s success=%d\n", __FUNCTION__, ReadVpuRegister(W5_RET_SUCCESS));
		return VPUAPI_RET_FAILURE;
	}

	return VPUAPI_RET_SUCCESS;
}
int vpuapi_get_output_info(u32 core, u32 inst, u32 *error_reason)
{
	int ret = VPUAPI_RET_SUCCESS;
	u32 val;

	vdi_lock(core, VPUDRV_MUTEX_VPU);

	ret = wave_send_query_command(core, inst, GET_RESULT);
	if (ret != VPUAPI_RET_SUCCESS) {
		goto HANDLE_ERROR;
	}

	DPRINTK("[VPUDRV]+%s success=%d, fail_reason=0x%x, error_reason=0x%x\n", __FUNCTION__, ReadVpuRegister(W5_RET_DEC_DECODING_SUCCESS),ReadVpuRegister(W5_RET_FAIL_REASON), ReadVpuRegister(W5_RET_DEC_ERR_INFO));
	val = ReadVpuRegister(W5_RET_DEC_DECODING_SUCCESS);
	if ((val & 0x01) == 0) {
#ifdef SUPPORT_SW_UART
		*error_reason = 0;
#else
		*error_reason = ReadVpuRegister(W5_RET_DEC_ERR_INFO);
#endif
	}
	else {
		*error_reason = 0x00;
	}

HANDLE_ERROR:
	if (ret != VPUAPI_RET_SUCCESS) {
		DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	}
	vdi_unlock(core, VPUDRV_MUTEX_VPU);
	return ret;
}
int vpuapi_dec_clr_all_disp_flag(u32 core, u32 inst)
{
	int ret = VPUAPI_RET_SUCCESS;
	u32 val = 0;

	vdi_lock(core, VPUDRV_MUTEX_VPU);

	WriteVpuRegister(W5_CMD_DEC_CLR_DISP_IDC, 0xffffffff);
	WriteVpuRegister(W5_CMD_DEC_SET_DISP_IDC, 0);
	ret = wave_send_query_command(core, inst, UPDATE_DISP_FLAG);
	if (ret != VPUAPI_RET_FAILURE) {
		goto HANDLE_ERROR;
	}

	val = ReadVpuRegister(W5_RET_SUCCESS);
	if (val == 0) {
		ret = VPUAPI_RET_FAILURE;
		goto HANDLE_ERROR;
	}

HANDLE_ERROR:
	if (ret != VPUAPI_RET_SUCCESS) {
		DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	}
	vdi_unlock(core, VPUDRV_MUTEX_VPU);
	return ret;
}
int vpuapi_dec_set_stream_end(u32 core, u32 inst)
{
	int ret = VPUAPI_RET_SUCCESS;
	u32 val;
	int product_code;

	vdi_lock(core, VPUDRV_MUTEX_VPU);

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

	if (PRODUCT_CODE_W6_SERIES(product_code)) {
		// nothing to do
		DPRINTK("vpuapi_dec_set_stream_end(): Please implement vpuapi_dec_set_stream_end for wave6 decoder.\n");
	} else if (PRODUCT_CODE_W5_SERIES(product_code)) {
		WriteVpuRegister(W5_BS_OPTION, (1/*STREAM END*/<<1) | (1/*explictEnd*/));
		// WriteVpuRegister(core, W5_BS_WR_PTR, pDecInfo->streamWrPtr); // keep not to be changed

		ret = wave_issue_command(core, inst, W5_UPDATE_BS);
		if (ret != VPUAPI_RET_SUCCESS) {
			goto HANDLE_ERROR;
		}

		val = ReadVpuRegister(W5_RET_SUCCESS);
		if (val == 0) {
			ret = VPUAPI_RET_FAILURE;
			goto HANDLE_ERROR;
		}
	} else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
		ret = VPUAPI_RET_SUCCESS;
		val = ReadVpuRegister(BIT_BIT_STREAM_PARAM);
		val |= 1 << 2;
		WriteVpuRegister(BIT_BIT_STREAM_PARAM, val);
	}
HANDLE_ERROR:
	if (ret != VPUAPI_RET_SUCCESS) {
		DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	}
	vdi_unlock(core, VPUDRV_MUTEX_VPU);
	return ret;
}
int vpuapi_close(u32 core, u32 inst)
{
	int ret = VPUAPI_RET_SUCCESS;
	u32 val;
	int product_code;

	vdi_lock(core, VPUDRV_MUTEX_VPU);
	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
	DPRINTK("[VPUDRV]+%s core=%d, inst=%d, product_code=0x%x\n", __FUNCTION__, core, inst, product_code);

	if (PRODUCT_CODE_W6_SERIES(product_code)) {
		ret = wave6_issue_command(core, inst, W6_DESTROY_INSTANCE);
		if (ret != VPUAPI_RET_SUCCESS) {
			goto HANDLE_ERROR;
		}
	} else if (PRODUCT_CODE_W5_SERIES(product_code)) {
		ret = wave_issue_command(core, inst, W5_DESTROY_INSTANCE);
		if (ret != VPUAPI_RET_SUCCESS) {
			goto HANDLE_ERROR;
		}

		val = ReadVpuRegister(W5_RET_SUCCESS);
		if (val == 0) {
			val = ReadVpuRegister(W5_RET_FAIL_REASON);
			if (val == WAVE5_SYSERR_VPU_STILL_RUNNING) {
				ret = VPUAPI_RET_STILL_RUNNING;
			}
			else {
				ret = VPUAPI_RET_FAILURE;
			}
		}
		else {
			ret = VPUAPI_RET_SUCCESS;
		}
	} else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
		ret = coda_issue_command(core, inst, DEC_SEQ_END);
		if (ret != VPUAPI_RET_SUCCESS) {
			goto HANDLE_ERROR;
		}
	}

HANDLE_ERROR:
	if (ret != VPUAPI_RET_SUCCESS) {
		DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	}
	DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
	vdi_unlock(core, VPUDRV_MUTEX_VPU);
	return ret;
}
int vpu_do_sw_reset(u32 core, u32 inst, u32 error_reason)
{
	int ret;
	vpudrv_instance_pool_t *vip = get_instance_pool_handle(core);
	int doSwResetInstIdx;

	DPRINTK("[VPUDRV]+%s core=%d, inst=%d, error_reason=0x%x\n", __FUNCTION__, core, inst, error_reason);
	if (vip == NULL)
		return VPUAPI_RET_FAILURE;

	vdi_lock(core, VPUDRV_MUTEX_RESET);
	ret = VPUAPI_RET_SUCCESS;
	doSwResetInstIdx = vip->doSwResetInstIdxPlus1-1;
	if (doSwResetInstIdx == inst || (error_reason & 0xf0000000)) {
		ret = vpuapi_sw_reset(core, inst, 0);
		if (ret == VPUAPI_RET_STILL_RUNNING) {
			DPRINTK("[VPUDRV] %s VPU is still running\n", __FUNCTION__);
			vip->doSwResetInstIdxPlus1 = (inst+1);
		}
		else if (ret == VPUAPI_RET_SUCCESS) {
			DPRINTK("[VPUDRV] %s success\n", __FUNCTION__);
			vip->doSwResetInstIdxPlus1 = 0;
		}
		else {
			DPRINTK("[VPUDRV] %s Fail result=0x%x\n", __FUNCTION__, ret);
			vip->doSwResetInstIdxPlus1 = 0;
		}
	}
	vdi_unlock(core, VPUDRV_MUTEX_RESET);
	DPRINTK("[VPUDRV]-%s vip->doSwResetInstIdx=%d, ret=%d\n", __FUNCTION__, vip->doSwResetInstIdxPlus1, ret);
	msleep(10);
	return ret;
}


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
#include <linux/of_reserved_mem.h>
#include <linux/proc_fs.h>
#include <linux/clk-provider.h>
#include "vpuconfig.h"
#include "vpuerror.h"
#include "jpuconfig.h"
#include "ion.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "vpu.h"
//#define ENABLE_DEBUG_MSG
#ifdef ENABLE_DEBUG_MSG
#define DPRINTK(args...)		printk(KERN_INFO args);
#else
#define DPRINTK(args...)
#endif
#include "wave/wave5_regdefine.h"
#include "wave/wave6_regdefine.h"
#include "coda9/coda9_regdefine.h"

#define VPU_PRODUCT_CODE_REGISTER                 (0x1044)
#define VPU_STAT_CYCLES                           (575000000)
/* definitions to be changed as customer  configuration */
/* if you want to have clock gating scheme frame by frame */
#define VPU_SUPPORT_CLOCK_CONTROL

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
#define VPU_SUPPORT_ION_MEMORY

#define VPU_PLATFORM_DEVICE_NAME "vpu"
#define VPU_CLK_NAME 	"vcodec"
#define VPU_CLASS_NAME  "vpu"
#define VPU_DEV_NAME 	"vpu"

/* if the platform driver knows this driver */
/* the definition of VPU_REG_BASE_ADDR and VPU_REG_SIZE are not meaningful */

#define VPU_REG_BASE_ADDR 0x75000000
#define VPU_REG_SIZE (0x10000*MAX_NUM_VPU_CORE)

#ifdef VPU_SUPPORT_ISR
#define VPU_IRQ_NUM (23+32)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define IOREMAP(addr, size) ioremap(addr, size)
#else
#define IOREMAP(addr, size) ioremap_nocache(addr, size)
#endif

/* this definition is only for chipsnmedia FPGA board env */
/* so for SOC env of customers can be ignored */

#ifndef VM_RESERVED	/*for kernel up to 3.7.0 version*/
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define MAX_ALLOC_RETRY_CNT 5

typedef struct vpu_drv_context_t {
    struct fasync_struct *async_queue;
    unsigned long interrupt_reason[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
    u32 open_count;                     /*!<< device reference count. Not instance count */
    int support_cq;
} vpu_drv_context_t;

typedef struct vpu_drv_context_all_t
{
    vpu_drv_context_t *p_vpu_context;
    int core_idx;
} vpu_drv_context_all_t;

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

static struct proc_dir_entry *entry = NULL;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
#	define VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE (256*1024*1024)
#	define VPU_DRAM_PHYSICAL_BASE 0x1A0000000
#include "vmm.h"
static video_mm_t s_vmem;
static int s_video_memory_flag = 0;
static vpudrv_buffer_t s_video_memory = {0};
#endif /*VPU_SUPPORT_RESERVED_VIDEO_MEMORY*/
int chip_id   = 0;
EXPORT_SYMBOL(chip_id);

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

typedef struct _vpu_power_ctl_ {
    struct clk *ve_clk[3];
    struct clk *vd_core0_clk[4];
    struct clk *vd_core1_clk[4];
} vpu_power_ctrl;

static const char *const vpu_clk_name[11] = {
    "ve_src0",
    "ve_src1",
    "apb_vesys_ve",
    "vd0_src0",
    "vd0_src1",
    "vd0_apb_vd",
    "vd0_vdsys_vd",
    "vd1_src0",
    "vd1_src1",
    "vd1_apb_vd",
    "vd1_vdsys_vd",
};

static vpu_power_ctrl vpu_pw_ctl = {0};

static int s_vpu_reg_phy_base[MAX_NUM_VPU_CORE] = {0x50440000, 0x50450000, 0x50460000};

static int video_cap = 0;

static int vpuapi_close(u32 core, u32 inst);
static int vpuapi_dec_set_stream_end(u32 core, u32 inst);
static int vpuapi_dec_clr_all_disp_flag(u32 core, u32 inst);
static int vpuapi_get_output_info(u32 core, u32 inst, u32 *error_reason);
#if defined(VPU_SUPPORT_PLATFORM_DRIVER_REGISTER) && defined(CONFIG_PM)
static int vpu_sleep_wake(u32 core, int mode);
#endif
static int vpu_do_sw_reset(u32 core, u32 inst, u32 error_reason);
static int vpu_close_instance_internal(u32 core, u32 inst);
static int vpu_check_is_decoder(u32 core, u32 inst);
#ifdef VPU_SUPPORT_CLOCK_CONTROL
void vpu_clk_disable(int core_idx);
void vpu_clk_enable(int core_idx);
struct clk *vpu_clk_get(struct device *dev);
void vpu_clk_put(struct clk *clk);
#endif
void vpu_top_reset(unsigned long core_idx);

/* end customer definition */
static vpudrv_buffer_t s_instance_pool[MAX_NUM_VPU_CORE] = {0};
static vpudrv_buffer_t s_common_memory[MAX_NUM_VPU_CORE] = {0};
static vpu_drv_context_t s_vpu_drv_context[MAX_NUM_VPU_CORE] = {0};
static struct device *vpu_dev;
#ifdef VPU_SUPPORT_CLOCK_CONTROL
struct clk *s_vpu_clk;
#endif

#ifdef VPU_SUPPORT_ISR
static int s_vpu_irq[MAX_NUM_VPU_CORE] = {45,39,42};
#endif

#define MAX_VPU_STAT_WIN_SIZE  10
#define VPU_INFO_STAT_INTERVAL 100

typedef struct vpu_statistic_info {
    int vpu_open_ref_count[MAX_NUM_VPU_CORE];
    uint64_t vpu_working_time_in_ms[MAX_NUM_VPU_CORE];
    uint64_t vpu_total_time_in_ms[MAX_NUM_VPU_CORE];
    atomic_t vpu_busy_status[MAX_NUM_VPU_CORE];
    char vpu_status_array[MAX_NUM_VPU_CORE][MAX_VPU_STAT_WIN_SIZE];
    int vpu_status_index[MAX_NUM_VPU_CORE];
    int vpu_instant_usage[MAX_NUM_VPU_CORE];
    int vpu_instant_interval;

    int64_t vpu_mem_total_size;
    int64_t vpu_mem_free_size;
    int64_t vpu_mem_used_size;

    int vpu_stat_fps[MAX_NUM_VPU_CORE];
    int vpu_realtime_fps[MAX_NUM_VPU_CORE];
    uint64_t vpu_stat_lasttime[MAX_NUM_VPU_CORE];

    uint64_t vpu_stat_cycles[MAX_NUM_VPU_CORE];
    int vpu_working_array[MAX_NUM_VPU_CORE][MAX_VPU_STAT_WIN_SIZE];
    int vpu_stat_enable[MAX_NUM_VPU_CORE];
}vpu_statistic_info_t;

// static DEFINE_MUTEX(s_vpu_proc_lock);
static vpu_statistic_info_t s_vpu_usage_info;
static struct task_struct *s_vpu_monitor_task = NULL;

vpudrv_buffer_t s_vpu_register[MAX_NUM_VPU_CORE] = {0};
static struct file *gfilp[MAX_NUM_VPU_CORE];

// for multi instance interrupt, SUPPORT_MULTI_INST_INTR
static int s_interrupt_flag[MAX_NUM_INSTANCE*MAX_NUM_VPU_CORE];
static wait_queue_head_t s_interrupt_wait_q[MAX_NUM_INSTANCE*MAX_NUM_VPU_CORE];
typedef struct kfifo kfifo_t;
static kfifo_t s_interrupt_pending_q[MAX_NUM_INSTANCE*MAX_NUM_VPU_CORE];

static spinlock_t s_kfifo_lock = __SPIN_LOCK_UNLOCKED(s_kfifo_lock);

// static spinlock_t s_vpu_lock = __SPIN_LOCK_UNLOCKED(s_vpu_lock);
static DEFINE_MUTEX(s_vpu_lock);
static DEFINE_MUTEX(s_extern_buffer_lock);

static struct list_head s_vbp_head = LIST_HEAD_INIT(s_vbp_head);
static struct list_head s_inst_list_head = LIST_HEAD_INIT(s_inst_list_head);
static struct list_head s_vbp_extern = LIST_HEAD_INIT(s_vbp_extern);

static vpu_bit_firmware_info_t s_bit_firmware_info[MAX_NUM_VPU_CORE] = {0};
static int vpu_show_fps = 0;
module_param(vpu_show_fps, uint, 0644);

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

static int get_vpu_core_num(int chip_id, int video_cap){
    return MAX_NUM_VPU_CORE;
}

void vpu_update_stat_cycles(int coreIdx, int hwCycles)
{
    s_vpu_usage_info.vpu_stat_cycles[coreIdx] += hwCycles;
}

void vpu_clear_stat_info(int coreIdx)
{
    s_vpu_usage_info.vpu_stat_cycles[coreIdx] = 0;
    s_vpu_usage_info.vpu_realtime_fps[coreIdx] = 0;
    s_vpu_usage_info.vpu_working_time_in_ms[coreIdx] = 0;
    s_vpu_usage_info.vpu_total_time_in_ms[coreIdx] = 0;
    s_vpu_usage_info.vpu_status_index[coreIdx] = 0;
    s_vpu_usage_info.vpu_instant_usage[coreIdx] = 0;
    memset(s_vpu_usage_info.vpu_working_array[coreIdx], 0, MAX_VPU_STAT_WIN_SIZE*sizeof(int));
    s_vpu_usage_info.vpu_stat_enable[coreIdx] = 0;
}

int device_hw_reset(void)
{
    DPRINTK("[VPUDRV] request device hw reset from application. \n");
    return 0;
}

int vpu_hw_reset(int core_idx)
{
    DPRINTK("[VPUDRV] request device vpu reset from application. \n");
    vpu_top_reset(core_idx);
    return 0;
}

static int vpu_check_usage_info(vpu_statistic_info_t *vpu_usage_info)
{
    int ret = 0, i;

    // mutex_lock(&s_vpu_proc_lock);
    /* update usage */
    for (i = 0; i <  get_vpu_core_num(chip_id, video_cap); i++){
        int vpu_woking_time_ms = 0;
        int vpu_instant_usage = 0;
        int j;

        if (vpu_usage_info->vpu_stat_enable[i] == 0) {
            continue;
        }

        vpu_woking_time_ms = (uint64_t)vpu_usage_info->vpu_stat_cycles[i]*1000/VPU_STAT_CYCLES;
        vpu_woking_time_ms = (vpu_woking_time_ms > VPU_INFO_STAT_INTERVAL) ? VPU_INFO_STAT_INTERVAL: vpu_woking_time_ms;
        vpu_usage_info->vpu_stat_cycles[i] = 0;

        vpu_usage_info->vpu_working_time_in_ms[i] += vpu_woking_time_ms;
        vpu_usage_info->vpu_total_time_in_ms[i] += VPU_INFO_STAT_INTERVAL;

        vpu_usage_info->vpu_working_array[i][vpu_usage_info->vpu_status_index[i]] = vpu_woking_time_ms;
        vpu_usage_info->vpu_status_index[i]++;
        vpu_usage_info->vpu_status_index[i] %= MAX_VPU_STAT_WIN_SIZE;

        for (j = 0; j < MAX_VPU_STAT_WIN_SIZE; j++)
            vpu_instant_usage += vpu_usage_info->vpu_working_array[i][j];

        vpu_usage_info->vpu_instant_usage[i] = (vpu_instant_usage)/MAX_VPU_STAT_WIN_SIZE;
        vpu_usage_info->vpu_instant_usage[i] = (vpu_usage_info->vpu_instant_usage[i] > 100) ? 100 : vpu_usage_info->vpu_instant_usage[i];
    }
    // mutex_unlock(&s_vpu_proc_lock);

    return ret;
}

int vpu_monitor_thread(void *data)
{
    int ret = 0;
    vpu_statistic_info_t *vpu_usage_info = (vpu_statistic_info_t *)data;

    set_current_state(TASK_INTERRUPTIBLE);
    while (!kthread_should_stop()) {
        vpu_check_usage_info(vpu_usage_info);
        msleep(VPU_INFO_STAT_INTERVAL);
    }

    s_vpu_monitor_task = NULL;
    return ret;
}

static vpudrv_instance_pool_t *get_instance_pool_handle(u32 core)
{
    int instance_pool_size_per_core;
    void *vip_base;

    if (core > MAX_NUM_VPU_CORE)
        return NULL;

    if (s_instance_pool[core].base == 0) {
        return NULL;
    }
    instance_pool_size_per_core = (s_instance_pool[core].size); /* s_instance_pool.size  assigned to the size of all core once call VDI_IOCTL_GET_INSTANCE_POOL by user. */
    // vip_base = (void *)(s_instance_pool[core].base + (instance_pool_size_per_core*core));
    vip_base = (void *)(s_instance_pool[core].base);

    return (vpudrv_instance_pool_t *)vip_base;
}

#define	ReadVpuRegister(addr)         *(volatile unsigned int *)(s_vpu_register[core].virt_addr + s_bit_firmware_info[core].reg_base_offset + addr)
#define	WriteVpuRegister(addr, val)   *(volatile unsigned int *)(s_vpu_register[core].virt_addr + s_bit_firmware_info[core].reg_base_offset + addr) = (unsigned int)val

static int vpu_alloc_dma_buffer(vpudrv_buffer_t *vb)
{
    char ion_name[32] = {0};
    if (!vb)
        return -1;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
    vb->phys_addr = (unsigned long)vmem_alloc(&s_vmem, vb->size, 0);
    if ((unsigned long)vb->phys_addr  == (unsigned long)-1) {
        printk(KERN_ERR "[VPUDRV] Physical memory allocation error size=%ld\n", vb->size);
        return -1;
    }

    vb->base = (unsigned long)(s_video_memory.base + (vb->phys_addr - s_video_memory.phys_addr));
#elif defined(VPU_SUPPORT_ION_MEMORY)
    sprintf(ion_name, "vcodec_%d_ion", vb->core_idx);
    if (base_ion_alloc((uint64_t *)&vb->phys_addr, (void **)&vb->virt_addr, ion_name, vb->size, vb->is_cached) != 0) {
        printk(KERN_ERR"[VDI] fail to allocate ion memory. size=%ld\n", vb->size);
        return -1;
    }
    vb->base = vb->virt_addr;
#else
    vb->base = (unsigned long)dma_alloc_coherent(vpu_dev, PAGE_ALIGN(vb->size), (dma_addr_t *) (&vb->phys_addr), GFP_DMA | GFP_KERNEL);
    if ((void *)(vb->base) == NULL)    {
        printk(KERN_ERR "[VPUDRV] Physical memory allocation error size=%ld\n", vb->size);
        return -1;
    }
#endif
    return 0;
}

int vpu_register_clk(struct platform_device *pdev)
{
    int i = 0;

    for (i = 0; i < 3; i++) {
        vpu_pw_ctl.ve_clk[i] = devm_clk_get(&pdev->dev, vpu_clk_name[i]);
        if (IS_ERR(vpu_pw_ctl.ve_clk[i])) {
            dev_err(&pdev->dev, "failed to get ve_clk_%d\n", i);
            return -1;
        }
    }

    for (i = 0; i < 4; i++) {
        vpu_pw_ctl.vd_core0_clk[i] = devm_clk_get(&pdev->dev, vpu_clk_name[i + 3]);
        if (IS_ERR(vpu_pw_ctl.vd_core0_clk[i])) {
            dev_err(&pdev->dev, "failed to get vd_core0_clk %d\n", i);
            return -1;
        }

        vpu_pw_ctl.vd_core1_clk[i] = devm_clk_get(&pdev->dev, vpu_clk_name[i + 7]);
        if (IS_ERR(vpu_pw_ctl.vd_core1_clk[i])) {
            dev_err(&pdev->dev, "failed to get vd_core1_clk %d\n", i);
            return -1;
        }
    }
    return 0;
}

static void vpu_free_dma_buffer(vpudrv_buffer_t *vb)
{
    if (!vb)
        return;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
    if (vb->base)
        vmem_free(&s_vmem, vb->phys_addr, 0);
#elif defined(VPU_SUPPORT_ION_MEMORY)
    if (vb->base != 0)
        base_ion_free(vb->phys_addr);
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

    DPRINTK("[VPUDRV]+%s\n", __FUNCTION__);

    list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
    {
        if (vil->filp == filp) {
            vip_base = (void *)(s_instance_pool[vil->core_idx].base);
            DPRINTK("[VPUDRV] vpu_free_instances detect instance crash instIdx=%d, coreIdx=%d, vip_base=%p, instance_pool_size_per_core=%d\n", (int)vil->inst_idx, (int)vil->core_idx, vip_base, (int)instance_pool_size_per_core);
            vip = (vpudrv_instance_pool_t *)vip_base;
            if (vip) {
                vpu_close_instance_internal((u32)vil->core_idx, (u32)vil->inst_idx);
                memset(&vip->codecInstPool[vil->inst_idx], 0x00, 4);    /* only first 4 byte is key point(inUse of CodecInst in vpuapi) to free the corresponding instance. */
            }

            s_vpu_usage_info.vpu_open_ref_count[vil->core_idx]--;
            list_del(&vil->list);
            kfree(vil);
        }
    }
    DPRINTK("[VPUDRV]-%s\n", __FUNCTION__);
    return 1;
}

static int vpu_free_buffers(int core_idx)
{
    vpudrv_buffer_pool_t *pool, *n;
    vpudrv_buffer_t vb;

    DPRINTK("[VPUDRV] vpu_free_buffers\n");

    list_for_each_entry_safe(pool, n, &s_vbp_head, list)
    {
        if (pool->vb.core_idx == core_idx) {
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
    DPRINTK("[VPUDRV][+]%s, int_reason=0x%x, empty_inst=0x%x, done_inst=0x%x, product_code=0x%\n", __func__, int_reason, empty_inst, done_inst);
    //printk(KERN_ERR "[VPUDRV][+]%s, int_reason=0x%x, empty_inst=0x%x, done_inst=0x%x\n", __func__, int_reason, empty_inst, done_inst);

    if (int_reason & (1 << INT_WAVE5_DEC_PIC))
    {
        reg_val = done_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_DEC_PIC);
        DPRINTK("[VPUDRV]    %s, W5_RET_QUEUE_CMD_DONE_INST DEC_PIC reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    if (int_reason & (1 << INT_WAVE5_BSBUF_EMPTY))
    {
        reg_val = empty_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason = (1 << INT_WAVE5_BSBUF_EMPTY);
        DPRINTK("[VPUDRV]    %s, W5_RET_BS_EMPTY_INST reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    if (int_reason & (1 << INT_WAVE5_INIT_SEQ))
    {
        reg_val = seq_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_INIT_SEQ);
        DPRINTK("[VPUDRV]    %s, RET_SEQ_DONE_INSTANCE_INFO INIT_SEQ reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM))
    {
        reg_val = seq_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_ENC_SET_PARAM);
        DPRINTK("[VPUDRV]    %s, RET_SEQ_DONE_INSTANCE_INFO ENC_SET_PARAM reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    if (int_reason & (1 << INT_WAVE5_ENC_SRC_RELEASE))
    {
        reg_val = done_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_ENC_SRC_RELEASE);
        DPRINTK("[VPUDRV]    %s, W5_RET_QUEUE_CMD_DONE_INST ENC_SET_PARAM reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }
#endif

    inst_idx = -1;
    *reason  = 0;
    DPRINTK("[VPUDRV]    %s, UNKNOWN INTERRUPT REASON: %08x\n", __func__, int_reason);

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
    u32 intr_reason = 0;
    s32 intr_inst_index = 0;
    DPRINTK("[VPUDRV][+]%s\n", __func__);

#ifdef VPU_IRQ_CONTROL
    disable_irq_nosync(s_vpu_irq);
#endif

    for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
        if (s_vpu_irq[core] == irq)
            break;
    }

    {
        if (s_bit_firmware_info[core].size == 0) {/* it means that we didn't get an information the current core from API layer. No core activated.*/
            DPRINTK("[VPUDRV] :  s_bit_firmware_info[%d].size is zero\n", core);
            return IRQ_HANDLED;
        }
        product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

        // w6 series code not used
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
                            return IRQ_HANDLED; // no done_inst in number i instance

                        intr_reason     = reason;
                        intr_inst_index = get_inst_idx(done_inst);

                        if (intr_inst_index >= 0 && intr_inst_index < MAX_NUM_INSTANCE) {
                            done_inst = done_inst & ~(1UL << intr_inst_index);
                            WriteVpuRegister(W6_CMD_DONE_INST, done_inst);
                            if (!kfifo_is_full(&s_interrupt_pending_q[intr_inst_index])) {
                                kfifo_in_spinlocked(&s_interrupt_pending_q[intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
                            }
                            else {
                                printk(KERN_ERR "[VPUDRV] :  kfifo_is_full kfifo_count=%d \n", kfifo_len(&s_interrupt_pending_q[intr_inst_index]));
                            }
                        }
                        else {
                            printk(KERN_ERR "[VPUDRV] :  intr_inst_index is wrong intr_inst_index=%d \n", intr_inst_index);
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
                            DPRINTK("[VPUDRV]    %s, W5_RET_BS_EMPTY_INST Clear empty_inst=0x%x, intr_inst_index=%d\n", __func__, empty_inst, intr_inst_index);
                        }
                        if (intr_reason == (1 << INT_WAVE5_DEC_PIC))
                        {
                            done_inst = done_inst & ~(1 << intr_inst_index);
                            WriteVpuRegister(W5_RET_QUEUE_CMD_DONE_INST, done_inst);
                            if (0 == done_inst) {
                                reason &= ~(1<<INT_WAVE5_DEC_PIC);
                            }
                            DPRINTK("[VPUDRV]    %s, W5_RET_QUEUE_CMD_DONE_INST Clear done_inst=0x%x, intr_inst_index=%d\n", __func__, done_inst, intr_inst_index);
                        }
                        if ((intr_reason == (1 << INT_WAVE5_INIT_SEQ)) || (intr_reason == (1 << INT_WAVE5_ENC_SET_PARAM)))
                        {
                            seq_inst = seq_inst & ~(1 << intr_inst_index);
                            WriteVpuRegister(W5_RET_SEQ_DONE_INSTANCE_INFO, seq_inst);
                            if (0 == seq_inst) {
                                reason &= ~(1<<INT_WAVE5_INIT_SEQ | 1<<INT_WAVE5_ENC_SET_PARAM);
                            }
                            DPRINTK("[VPUDRV]    %s, W5_RET_SEQ_DONE_INSTANCE_INFO Clear done_inst=0x%x, intr_inst_index=%d\n", __func__, done_inst, intr_inst_index);
                        }
                        if (!kfifo_is_full(&s_interrupt_pending_q[core*MAX_NUM_INSTANCE+intr_inst_index])) {
                            if (intr_reason == (1 << INT_WAVE5_ENC_PIC)) {
                                u32 ll_intr_reason = (1 << INT_WAVE5_ENC_PIC);
                                kfifo_in_spinlocked(&s_interrupt_pending_q[core*MAX_NUM_INSTANCE+intr_inst_index], &ll_intr_reason, sizeof(u32), &s_kfifo_lock);
                            }
                            else
                                kfifo_in_spinlocked(&s_interrupt_pending_q[core*MAX_NUM_INSTANCE+intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
                        }
                        else {
                            printk(KERN_ERR "[VPUDRV] :  kfifo_is_full kfifo_count=%d \n", kfifo_len(&s_interrupt_pending_q[core*MAX_NUM_INSTANCE+intr_inst_index]));
                        }
                    }
//                    else {
//                        printk(KERN_ERR "[VPUDRV] :  intr_inst_index is wrong intr_inst_index=%d \n", intr_inst_index);
//                    }
                }

                if (0 != reason)
                    printk(KERN_ERR "INTERRUPT REASON REMAINED: %08x\n", reason);
                WriteVpuRegister(W5_VPU_VINT_REASON_CLR, reason_clr);

                WriteVpuRegister(W5_VPU_VINT_CLEAR, 0x1);
            }
        }
        else if (PRODUCT_CODE_CODA_SERIES(product_code)) {
            if (ReadVpuRegister(BIT_INT_STS)) {
                intr_reason = ReadVpuRegister(BIT_INT_REASON);
                intr_inst_index = 0; // in case of coda seriese. treats intr_inst_index is already 0
                kfifo_in_spinlocked(&s_interrupt_pending_q[core*MAX_NUM_INSTANCE+intr_inst_index], &intr_reason, sizeof(u32), &s_kfifo_lock);
                WriteVpuRegister(BIT_INT_CLEAR, 0x1);
            }
        }
        else {
            DPRINTK("[VPUDRV] Unknown product id : %08x\n", product_code);
            return IRQ_HANDLED;
        }
        DPRINTK("[VPUDRV] product: 0x%08x intr_reason: 0x%08x\n", product_code, intr_reason);

    #if 0
        if (dev->async_queue)
            kill_fasync(&dev->async_queue, SIGIO, POLL_IN);    /* notify the interrupt to user space */
    #endif

        for (intr_inst_index = 0; intr_inst_index < MAX_NUM_INSTANCE; intr_inst_index++) {
            if (kfifo_len(&s_interrupt_pending_q[core*MAX_NUM_INSTANCE+intr_inst_index])) {
                s_interrupt_flag[core*MAX_NUM_INSTANCE+intr_inst_index] = 1;
                wake_up_interruptible(&s_interrupt_wait_q[core*MAX_NUM_INSTANCE+intr_inst_index]);
            }
        }
    }

    DPRINTK("[VPUDRV][-]%s\n\n", __func__);

    return IRQ_HANDLED;
}
#endif

int vpu_op_open(int core_idx)
{
    int ret = 0;
    vpu_drv_context_all_t *p_drv_context = NULL;
    DPRINTK("[VPUDRV][+] %s\n", __func__);

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if ((ret =  mutex_lock_interruptible(&s_vpu_lock)) == 0)
    {
        if (gfilp[core_idx] == NULL)
            gfilp[core_idx] = vmalloc(sizeof(struct file));

        p_drv_context = vmalloc(sizeof(vpu_drv_context_all_t));//(void *)(&s_vpu_drv_context);
        gfilp[core_idx]->private_data = p_drv_context;
        if(gfilp[core_idx]->private_data == NULL) {
            pr_err("can not allocate memory for private data.\n");
            mutex_unlock(&s_vpu_lock);
            return -1;
        }

        p_drv_context->core_idx = -1;
        p_drv_context->p_vpu_context = &s_vpu_drv_context[core_idx];

        // if (!s_vpu_drv_context.open_count)
        //     bm_vpu_deassert(&vpu_rst_ctrl);

        s_vpu_drv_context[core_idx].open_count++;
#ifdef VPU_SUPPORT_CLOCK_CONTROL
        vpu_clk_enable(core_idx);
#endif

        mutex_unlock(&s_vpu_lock);
    }

    DPRINTK("[VPUDRV][-] %s\n", __func__);

    return ret;
}

long vpu_get_common_memory(vpudrv_buffer_t *vdb)
{
    long ret;

    if (vdb->core_idx >=  get_vpu_core_num(chip_id, video_cap)) {
        ret = -EFAULT;
        pr_err("[%s,%d] core_idx incorrect!\n", __func__, __LINE__);
        return ret;
    }

    if (s_common_memory[vdb->core_idx].base == 0) {
        memcpy(&s_common_memory[vdb->core_idx], vdb, sizeof(vpudrv_buffer_t));
        ret = vpu_alloc_dma_buffer(&s_common_memory[vdb->core_idx]);
    }
    memcpy(vdb, &s_common_memory[vdb->core_idx], sizeof(vpudrv_buffer_t));

    if(ret != 0) {
        ret = -EFAULT;
    }

    return ret;
}

long vpu_get_instance_pool(vpudrv_buffer_t *info)
{
    long ret = 0;
    vpu_drv_context_all_t *p_drv_context = (vpu_drv_context_all_t *)gfilp[info->core_idx]->private_data;

    p_drv_context->core_idx = info->core_idx;//info.base;
    if(info->core_idx >=  get_vpu_core_num(chip_id, video_cap))
        return -EFAULT;

    // if ((ret =    mutex_lock_interruptible(&s_vpu_lock)) != 0) {
    //     return -ERESTARTSYS;
    // }
    mutex_lock(&s_vpu_lock);

    if (s_instance_pool[info->core_idx].base != 0) {
        memcpy(info, &s_instance_pool[info->core_idx], sizeof(vpudrv_buffer_t));
        mutex_unlock(&s_vpu_lock);

        return ret;
    }

    memcpy(&s_instance_pool[info->core_idx], info, sizeof(vpudrv_buffer_t));
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
    //s_instance_pool.size = PAGE_ALIGN(s_instance_pool.size);
    s_instance_pool[info->core_idx].base = (unsigned long)vmalloc(s_instance_pool[info->core_idx].size);
    s_instance_pool[info->core_idx].phys_addr = (unsigned long)vmalloc_to_pfn((void *)s_instance_pool[info->core_idx].base) << PAGE_SHIFT;

    if (s_instance_pool[info->core_idx].base != 0)
#else

    if (vpu_alloc_dma_buffer(&s_instance_pool[info->core_idx]) != -1)
#endif
    {
        memset((void *)s_instance_pool[info->core_idx].base, 0x0, s_instance_pool[info->core_idx].size); /*clearing memory*/
        memcpy(info, &s_instance_pool[info->core_idx], sizeof(vpudrv_buffer_t));
        mutex_unlock(&s_vpu_lock);
        return 0;
    }

    mutex_unlock(&s_vpu_lock);
    return -EFAULT;
}

long vpu_open_instance(vpudrv_inst_info_t *inst_info)
{
    vpudrv_instanace_list_t *vil;

    vil = kzalloc(sizeof(*vil), GFP_KERNEL);
    if (!vil)
        return -ENOMEM;

    vil->inst_idx = inst_info->inst_idx;
    vil->core_idx = inst_info->core_idx;
    vil->filp = (struct file *)gfilp[inst_info->core_idx];
    mutex_lock(&s_vpu_lock);
    list_add(&vil->list, &s_inst_list_head);

    s_vpu_usage_info.vpu_open_ref_count[vil->core_idx]++; /* flag just for that vpu is in opened or closed */
    inst_info->inst_open_count = s_vpu_usage_info.vpu_open_ref_count[vil->core_idx];
#ifdef SUPPORT_MULTI_INST_INTR
    kfifo_reset(&s_interrupt_pending_q[inst_info->core_idx*MAX_NUM_INSTANCE+inst_info->inst_idx]);
    s_interrupt_flag[inst_info->core_idx*MAX_NUM_INSTANCE+inst_info->inst_idx] = 0;
    // release_vpu_create_inst_flag(inst_info->core_idx, inst_info->inst_idx);
#else
    s_interrupt_flag[inst_info->core_idx] = 0;
#endif
    s_vpu_usage_info.vpu_stat_enable[vil->core_idx] = 1;

    /* launch vpu monitor thread */
    if (s_vpu_monitor_task == NULL){
        s_vpu_monitor_task = kthread_run(vpu_monitor_thread, &s_vpu_usage_info, "soph_vpu_monitor");
        if (IS_ERR(s_vpu_monitor_task)) {
            pr_err("create vpu monitor thread failed\n");
            s_vpu_monitor_task = NULL;
        } else
            pr_info("create vpu monitor thread done\n");
    }
    mutex_unlock(&s_vpu_lock);

    DPRINTK("[VPUDRV] VDI_IOCTL_OPEN_INSTANCE core_idx=%d, inst_idx=%d, vpu_open_ref_count=%d, inst_open_count=%d\n", \
            (int)inst_info->core_idx, (int)inst_info->inst_idx, s_vpu_usage_info.vpu_open_ref_count[inst_info->core_idx], inst_info->inst_open_count);

    return 0;
}

long vpu_close_instance(vpudrv_inst_info_t *inst_info)
{
    vpudrv_instanace_list_t *vil, *n;

    DPRINTK("[VPUDRV][+]VDI_IOCTL_CLOSE_INSTANCE\n");

    mutex_lock(&s_vpu_lock);
    list_for_each_entry_safe(vil, n, &s_inst_list_head, list)
    {
        if (vil->inst_idx == inst_info->inst_idx && vil->core_idx == inst_info->core_idx) {
            s_vpu_usage_info.vpu_open_ref_count[vil->core_idx]--; /* flag just for that vpu is in opened or closed */
            inst_info->inst_open_count = s_vpu_usage_info.vpu_open_ref_count[vil->core_idx]; /* counting the current open instance number */
            list_del(&vil->list);
            kfree(vil);
            // dev->crst_cxt[inst_info->core_idx].instcall[inst_info->inst_idx] = 0;
            break ;
        }
    }

    if (s_vpu_usage_info.vpu_open_ref_count[inst_info->core_idx] == 0) {
        vpu_clear_stat_info(inst_info->core_idx);
    }
    mutex_unlock(&s_vpu_lock);

    DPRINTK("[VPUDRV][-] VDI_IOCTL_CLOSE_INSTANCE core_idx=%d, inst_idx=%d, vpu_open_ref_count=%d, inst_open_count=%d\n", \
            (int)inst_info->core_idx, (int)inst_info->inst_idx, s_vpu_usage_info.vpu_open_ref_count[inst_info->core_idx], inst_info->inst_open_count);

    return 0;
}

long vpu_flush_dcache(vpudrv_buffer_t *info)
{
#ifdef VPU_SUPPORT_ION_MEMORY
    return base_ion_cache_flush(info->phys_addr, (void *)info->virt_addr, info->size);
#endif
    return 0;
}

long vpu_invalidate_dcache(vpudrv_buffer_t *info)
{
#ifdef VPU_SUPPORT_ION_MEMORY
    return base_ion_cache_invalidate(info->phys_addr, (void *)info->virt_addr, info->size);
#endif

    return 0;
}

long vpu_allocate_physical_memory(vpudrv_buffer_t *vdb)
{
    long ret;
    vpudrv_buffer_pool_t *vbp;
    int retry_cnt = 0;

    DPRINTK("[VPUDRV][+]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");

    vbp = kzalloc(sizeof(*vbp), GFP_KERNEL);
    if (!vbp) {
        return -ENOMEM;
    }

    memcpy(&(vbp->vb), vdb, sizeof(vpudrv_buffer_t));
    vbp->vb.offset = 0;

    do {
        if (retry_cnt >= MAX_ALLOC_RETRY_CNT) {
            pr_err("[%s,%d] fail !\n", __func__, __LINE__);
            ret = -ENOMEM;
            kfree(vbp);
            return ret;
        }

        retry_cnt++;
        vbp->vb.size += vbp->vb.offset;
        ret = vpu_alloc_dma_buffer(&(vbp->vb));
        if (ret == -1) {
            ret = -ENOMEM;
            kfree(vbp);
            return ret;
        }

        if ((vbp->vb.phys_addr & 0xFFFFFFFF) < WAVE5_MAX_CODE_BUF_SIZE) {
            vpu_free_dma_buffer(&(vbp->vb));
            vbp->vb.base = 0;
            vbp->vb.phys_addr = 0;
            vbp->vb.offset = WAVE5_MAX_CODE_BUF_SIZE;
        }
    } while (!(vbp->vb.base));

    if (vbp->vb.offset > 0) {
        vbp->vb.phys_addr += vbp->vb.offset;
        vbp->vb.virt_addr += vbp->vb.offset;
        vbp->vb.base += vbp->vb.offset;
        vbp->vb.size -= vbp->vb.offset;
    }

    //vbp->filp = filp;
    mutex_lock(&s_vpu_lock);
    list_add(&vbp->list, &s_vbp_head);
    mutex_unlock(&s_vpu_lock);

    memcpy(vdb, &(vbp->vb), sizeof(vpudrv_buffer_t));

    DPRINTK("[VPUDRV][-]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");

    return 0;
}

long vpu_free_physical_memory(vpudrv_buffer_t *vdb)
{
    vpudrv_buffer_pool_t *vbp, *n;
    DPRINTK("[VPUDRV][+]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

    mutex_lock(&s_vpu_lock);
    if (vdb->base) {
        if (vdb->offset > 0) {
            vdb->phys_addr -= vdb->offset;
        }

        vpu_free_dma_buffer(vdb);
    }

    list_for_each_entry_safe(vbp, n, &s_vbp_head, list)
    {
        if (vbp->vb.base == vdb->base) {
            list_del(&vbp->list);
            kfree(vbp);
            break;
        }
    }
    mutex_unlock(&s_vpu_lock);

    DPRINTK("[VPUDRV][-]VDI_IOCTL_FREE_PHYSICALMEMORY\n");
    return 0;
}

long vpu_allocate_extern_memory(vpudrv_buffer_t *vdb)
{
    long ret;
    vpudrv_buffer_pool_t *vbp;

    vbp = kzalloc(sizeof(*vbp), GFP_KERNEL);
    if (!vbp) {
        return -ENOMEM;
    }

    memcpy(&(vbp->vb), vdb, sizeof(vpudrv_buffer_t));
    vbp->vb.offset = 0;
    ret = vpu_alloc_dma_buffer(&(vbp->vb));
    if (ret == -1) {
        ret = -ENOMEM;
        kfree(vbp);
        return ret;
    }

    mutex_lock(&s_extern_buffer_lock);
    list_add(&vbp->list, &s_vbp_extern);
    mutex_unlock(&s_extern_buffer_lock);

    memcpy(vdb, &(vbp->vb), sizeof(vpudrv_buffer_t));

    DPRINTK("alloc vbp:0x%lx,0x%lx vdb:0x%lx,0x%lx"
        , vbp->vb.base, vbp->vb.phys_addr, vdb->base, vdb->phys_addr);

    DPRINTK("[VPUDRV][-]VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY\n");

    return 0;
}

long vpu_free_extern_memory(vpudrv_buffer_t *vdb)
{
    vpudrv_buffer_pool_t *vbp, *n;
    int find_buffer = 0;
    DPRINTK("[VPUDRV][+]VDI_IOCTL_FREE_PHYSICALMEMORY\n");

    // Check if the buffer has been released
    mutex_lock(&s_extern_buffer_lock);
    list_for_each_entry_safe(vbp, n, &s_vbp_extern, list)
    {
        if (vbp->vb.phys_addr == vdb->phys_addr) {
            find_buffer = 1;
            vpu_free_dma_buffer(vdb);
            list_del(&vbp->list);
            kfree(vbp);
            break;
        }
    }
    mutex_unlock(&s_extern_buffer_lock);

    DPRINTK("[VPUDRV][-]VDI_IOCTL_FREE_PHYSICALMEMORY\n");
    return 0;
}

int vpu_free_extern_buffers(int core_idx)
{
    vpudrv_buffer_pool_t *pool, *n;
    vpudrv_buffer_t vb;

    DPRINTK("[VPUDRV] vpu_free_extern_buffers\n");
    if (s_vpu_usage_info.vpu_open_ref_count[core_idx]) {
        return 0;
    }

    mutex_lock(&s_extern_buffer_lock);
    list_for_each_entry_safe(pool, n, &s_vbp_extern, list)
    {
        if (pool->vb.core_idx == core_idx) {
            vb = pool->vb;
            if (vb.base) {
                vpu_free_dma_buffer(&vb);
                list_del(&pool->list);
                kfree(pool);
            }
        }
    }
    mutex_unlock(&s_extern_buffer_lock);

    return 0;
}

long vpu_get_free_mem_size(unsigned long *size)
{
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
    vmem_info_t vinfo;
    // unsigned long size;
    DPRINTK("[VPUDRV][+]VDI_IOCTL_GET_FREE_MEM_SIZE\n");
    vmem_get_info(&s_vmem, &vinfo);

    *size = vinfo.free_pages * vinfo.page_size;

    DPRINTK("[VPUDRV][-]VDI_IOCTL_GET_FREE_MEM_SIZE\n");
#endif
    return 0;
}

long vpu_set_clock_gate(int core_idx, unsigned int *clkgate)
{
    DPRINTK("[VPUDRV][+]VDI_IOCTL_SET_CLOCK_GATE\n");

#ifdef VPU_SUPPORT_CLOCK_CONTROL
    if (*clkgate)
        vpu_clk_enable(core_idx);
    else
        vpu_clk_disable(core_idx);
#endif
    DPRINTK("[VPUDRV][-]VDI_IOCTL_SET_CLOCK_GATE\n");
    return 0;
}

static uint64_t vpu_gettime(void)
{
    struct timespec64 tv;
    ktime_get_ts64(&tv);

    return tv.tv_sec * 1000 + tv.tv_nsec / 1000000; // in ms
}

long vpu_wait_interrupt(vpudrv_intr_info_t *info)
{
    long ret;
#ifdef SUPPORT_TIMEOUT_RESOLUTION
    ktime_t kt;
#endif

#ifdef SUPPORT_MULTI_INST_INTR
    u32 intr_inst_index = info->intr_inst_index;
    u32 intr_reason_in_q;
    u32 interrupt_flag_in_q;
#endif
    u32 core_idx;
    vpu_drv_context_all_t *p_drv_context = (vpu_drv_context_all_t *)gfilp[info->core_idx]->private_data;
    struct vpu_drv_context_t *dev = p_drv_context->p_vpu_context;

    DPRINTK("[VPUDRV][+]VDI_IOCTL_WAIT_INTERRUPT\n");

    core_idx = info->core_idx;
    DPRINTK("[VPUDRV]--core index: %d\n", core_idx);

    atomic_inc(&s_vpu_usage_info.vpu_busy_status[core_idx]);

    intr_reason_in_q = 0;
    interrupt_flag_in_q = kfifo_out_spinlocked(&s_interrupt_pending_q[core_idx*MAX_NUM_INSTANCE+intr_inst_index], &intr_reason_in_q, sizeof(u32), &s_kfifo_lock);
    if (interrupt_flag_in_q > 0)
    {
        dev->interrupt_reason[core_idx*MAX_NUM_INSTANCE+intr_inst_index] = intr_reason_in_q;
        DPRINTK("[VPUDRV] Interrupt Remain : core=%d, intr_inst_index=%d, intr_reason_in_q=0x%x, interrupt_flag_in_q=%d\n", core_idx, intr_inst_index, intr_reason_in_q, interrupt_flag_in_q);
        goto INTERRUPT_REMAIN_IN_QUEUE;
    }

#ifdef SUPPORT_MULTI_INST_INTR
#ifdef SUPPORT_TIMEOUT_RESOLUTION
    kt =  ktime_set(0, info->timeout*1000*1000);
    ret = wait_event_interruptible_hrtimeout(s_interrupt_wait_q[core_idx*MAX_NUM_INSTANCE+intr_inst_index], s_interrupt_flag[core_idx*MAX_NUM_INSTANCE+intr_inst_index] != 0, kt);
#else
    ret = wait_event_interruptible_timeout(s_interrupt_wait_q[core_idx*MAX_NUM_INSTANCE+intr_inst_index], s_interrupt_flag[core_idx*MAX_NUM_INSTANCE+intr_inst_index] != 0, usecs_to_jiffies(info->timeout));
#endif
#else
    ret = wait_event_interruptible_timeout(s_interrupt_wait_q[core_idx], s_interrupt_flag[core_idx] != 0, msecs_to_jiffies(info->timeout));
#endif
#ifdef SUPPORT_TIMEOUT_RESOLUTION
    if (ret == -ETIME) {
        //DPRINTK("[VPUDRV][-]VDI_IOCTL_WAIT_INTERRUPT timeout = %d \n", info.timeout);
        atomic_dec(&s_vpu_usage_info.vpu_busy_status[core_idx]);
        return ret;
    }
#else
    if (!ret) {
        ret = -ETIME;
        atomic_dec(&s_vpu_usage_info.vpu_busy_status[core_idx]);
        //DPRINTK("[VPUDRV][-]VDI_IOCTL_WAIT_INTERRUPT timeout = %d \n", info.timeout);
        return ret;
    }
#endif
    if (signal_pending(current)) {
        printk(KERN_ERR "[VPUDRV] signal_pending failed\n");
        ret = -ERESTARTSYS;
        atomic_dec(&s_vpu_usage_info.vpu_busy_status[core_idx]);
        return ret;
    }

#ifdef SUPPORT_MULTI_INST_INTR
    intr_reason_in_q = 0;
    interrupt_flag_in_q = kfifo_out_spinlocked(&s_interrupt_pending_q[core_idx*MAX_NUM_INSTANCE+intr_inst_index], &intr_reason_in_q, sizeof(u32), &s_kfifo_lock);
    if (interrupt_flag_in_q > 0) {
        dev->interrupt_reason[core_idx*MAX_NUM_INSTANCE+intr_inst_index] = intr_reason_in_q;
    }
    else {
        dev->interrupt_reason[core_idx*MAX_NUM_INSTANCE+intr_inst_index] = 0;
    }
#endif

#ifdef SUPPORT_MULTI_INST_INTR
    DPRINTK("[VPUDRV] inst_index(%d), s_interrupt_flag(%d), reason(0x%08lx)\n", intr_inst_index, s_interrupt_flag[core_idx*MAX_NUM_INSTANCE+intr_inst_index], dev->interrupt_reason[core_idx*MAX_NUM_INSTANCE+intr_inst_index]);
#else
    DPRINTK("[VPUDRV]    s_interrupt_flag(%d), reason(0x%08lx)\n", s_interrupt_flag[core_idx], dev->interrupt_reason[core_idx]);
#endif

INTERRUPT_REMAIN_IN_QUEUE:
#ifdef SUPPORT_MULTI_INST_INTR
    info->intr_reason = dev->interrupt_reason[core_idx*MAX_NUM_INSTANCE+intr_inst_index];
    s_interrupt_flag[core_idx*MAX_NUM_INSTANCE+intr_inst_index] = 0;
    dev->interrupt_reason[core_idx*MAX_NUM_INSTANCE+intr_inst_index] = 0;
#else
    info->intr_reason = dev->interrupt_reason[core_idx];
    s_interrupt_flag[core_idx] = 0;
    dev->interrupt_reason[core_idx] = 0;
#endif

    if (info->intr_reason & (1<<INT_WAVE5_DEC_PIC)) {
        s_vpu_usage_info.vpu_stat_fps[core_idx] += 1;
    }

    if (!s_vpu_usage_info.vpu_stat_lasttime[core_idx]) {
        s_vpu_usage_info.vpu_stat_lasttime[core_idx] = vpu_gettime();
    }

    if (s_vpu_usage_info.vpu_stat_lasttime[core_idx] && (vpu_gettime() - s_vpu_usage_info.vpu_stat_lasttime[core_idx] >= 1000)) {
        if (vpu_show_fps)
            pr_info("perf stat core:%d, fps:%d \n", core_idx, s_vpu_usage_info.vpu_stat_fps[core_idx]);
        s_vpu_usage_info.vpu_stat_lasttime[core_idx] = vpu_gettime();
        s_vpu_usage_info.vpu_realtime_fps[core_idx] = s_vpu_usage_info.vpu_stat_fps[core_idx];
        s_vpu_usage_info.vpu_stat_fps[core_idx] = 0;
    }

    atomic_dec(&s_vpu_usage_info.vpu_busy_status[core_idx]);

    return 0;
}

ssize_t vpu_op_write(const char *buf, size_t len)
{
    int ret = 0;
    vpu_bit_firmware_info_t *bit_firmware_info;

    if (!buf) {
        printk(KERN_ERR "[VPUDRV] vpu_write buf = NULL error \n");
        return -EFAULT;
    }

    {
        mutex_lock(&s_vpu_lock);
        bit_firmware_info = (vpu_bit_firmware_info_t *)buf;

        if (bit_firmware_info->size == sizeof(vpu_bit_firmware_info_t)) {
            DPRINTK("[VPUDRV] vpu_write set bit_firmware_info coreIdx=0x%x, reg_base_offset=0x%x size=0x%x, bit_code[0]=0x%x\n",
            bit_firmware_info->core_idx, (int)bit_firmware_info->reg_base_offset, bit_firmware_info->size, bit_firmware_info->bit_code[0]);

            if (bit_firmware_info->core_idx >  get_vpu_core_num(chip_id, video_cap)) {
                mutex_unlock(&s_vpu_lock);
                printk(KERN_ERR "[VPUDRV] vpu_write coreIdx[%d] is exceeded than  MAX_NUM_VPU_CORE[%d]\n", bit_firmware_info->core_idx, get_vpu_core_num(chip_id, video_cap));
                return -ENODEV;
            }
            if(s_bit_firmware_info[bit_firmware_info->core_idx].size != bit_firmware_info->size)
                memcpy((void *)&s_bit_firmware_info[bit_firmware_info->core_idx], bit_firmware_info, sizeof(vpu_bit_firmware_info_t));
            // s_init_flag[bit_firmware_info->core_idx] = s_bit_firmware_info[bit_firmware_info->core_idx].size;
            mutex_unlock(&s_vpu_lock);
            return len;
        }
        mutex_unlock(&s_vpu_lock);
    }

    return ret;
}

void vpu_top_reset(unsigned long core_idx)
{
    unsigned int *top_reg_virt_addr;
    unsigned int reg_val;

    top_reg_virt_addr = (unsigned int *)IOREMAP(0x28103000, 4);
    reg_val = readl(top_reg_virt_addr);

    if (core_idx == 0)
        reg_val &= ~(1<<17);//vesys_ve
    else if (core_idx == 1)
        reg_val &= ~(1<<9);//vdsys0_vd
    else if (core_idx == 2)
        reg_val &= ~(1<<13);//vdsys1_vd
    writel(reg_val, top_reg_virt_addr);

    if (core_idx == 0)
        reg_val |= (1<<17);//vesys_ve
    else if (core_idx == 1)
        reg_val |= (1<<9);//vdsys0_vd
    else if (core_idx == 2)
        reg_val |= (1<<13);//vdsys1_vd
    writel(reg_val, top_reg_virt_addr);

    iounmap((void *)top_reg_virt_addr);
}

// reference vpu_release
int vpu_op_close(int core_idx)
{
    u32 open_count;
    int i;
    struct file *filp = gfilp[core_idx];
    DPRINTK("[VPUDRV] +%s\n", __FUNCTION__);

    mutex_lock(&s_vpu_lock);

    /* found and free the not handled buffer by user applications */
    vpu_free_buffers(core_idx);

    /* found and free the not closed instance by user applications */
    vpu_free_instances(filp);

    s_vpu_drv_context[core_idx].open_count--;
    open_count = s_vpu_drv_context[core_idx].open_count;
    if (open_count == 0) {
        for (i=0; i<MAX_NUM_INSTANCE; i++) {
            kfifo_reset(&s_interrupt_pending_q[core_idx*MAX_NUM_INSTANCE + i]);
        }

        if (s_instance_pool[core_idx].base) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
            vfree((const void *)s_instance_pool[core_idx].base);
#else
            vpu_free_dma_buffer(&s_instance_pool[core_idx]);
#endif
            s_instance_pool[core_idx].base = 0;
            s_instance_pool[core_idx].phys_addr = 0;
        }
        if (s_common_memory[core_idx].base) {
            vpu_free_dma_buffer(&s_common_memory[core_idx]);
            s_common_memory[core_idx].base = 0;
        }

        vpu_top_reset(core_idx);

#ifdef VPU_SUPPORT_CLOCK_CONTROL
        vpu_clk_disable(core_idx);
#endif
    }

    if(gfilp[core_idx]->private_data != NULL)
        vfree(gfilp[core_idx]->private_data);

    if (gfilp[core_idx] != NULL)
        vfree(gfilp[core_idx]);

    gfilp[core_idx]= NULL;
    DPRINTK("[VPUDRV] -%s\n", __FUNCTION__);

    // when all core idle, stop vpuinfo thread
    if (!gfilp[0] && !gfilp[1] && !gfilp[2] && s_vpu_monitor_task) {
        kthread_stop(s_vpu_monitor_task);
        s_vpu_monitor_task = NULL;
        pr_info("vpu monitor thread released\n");
    }

    mutex_unlock(&s_vpu_lock);

    return 0;
}

#if defined(CONFIG_PM)

int vpu_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    int core;
    int ret = VPUAPI_RET_SUCCESS;

    DPRINTK("[VPUDRV] vpu_suspend\n");

    for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
        if (s_vpu_drv_context[core].open_count == 0)
            continue;

        ret = vpu_sleep_wake(core, VPU_SLEEP_MODE);

#ifdef VPU_SUPPORT_CLOCK_CONTROL
        vpu_clk_disable(core);
#endif

        if (ret != VPUAPI_RET_SUCCESS) {
            return -EAGAIN;
        }
    }

    return 0;
}

int vpu_drv_resume(struct platform_device *pdev)
{
    int core;
    int ret = VPUAPI_RET_SUCCESS;

    DPRINTK("[VPUDRV] vpu_resume\n");

    for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
        if (s_vpu_drv_context[core].open_count == 0) {
            continue;
        }

#ifdef VPU_SUPPORT_CLOCK_CONTROL
        vpu_clk_enable(core);
#endif

        if (s_vpu_drv_context[core].open_count)
            ret = vpu_sleep_wake(core, VPU_WAKE_MODE);

        if (ret != VPUAPI_RET_SUCCESS) {
            return -EAGAIN;
        }
    }

    return 0;
}
#endif /* !CONFIG_PM */

int check_vpu_core_busy(vpu_statistic_info_t *vpu_usage_info, int coreIdx)
{
    int ret = 0;

    int core = coreIdx;
    if (s_bit_firmware_info[coreIdx].size) {
        if (vpu_usage_info->vpu_open_ref_count[coreIdx] > 0){
            if (ReadVpuRegister(W5_VPU_BUSY_STATUS) == 1)
                ret = 1;
            else if (atomic_read(&vpu_usage_info->vpu_busy_status[coreIdx]) > 0)
                ret = 1;
        }
    }

    return ret;
}

#if 0
static int get_inst_info_idx(unsigned long flag)
{
    int i = -1;
    for(i=0; i< get_vpu_core_num(chip_id, video_cap)*MAX_NUM_INSTANCE; i++)
    {
        if(s_vpu_inst_info[i].flag == flag)
            return i;
    }

    for(i=0; i< get_vpu_core_num(chip_id, video_cap)*MAX_NUM_INSTANCE; i++)
    {
        if(s_vpu_inst_info[i].flag == 0)
            break;
    }

    return i;
}

static void printf_inst_info(void *mem, int size)
{
    char temp[1024]      = { 0 };
    int  single_list_len = 0;
    int  total_len       = 0;
    int  total_len_first = 0;
    int  i;
    sprintf(mem, "\"link\":[\n");
    total_len = strlen(mem);
    total_len_first = total_len;
    for(i=0; i< get_vpu_core_num(chip_id, video_cap)*MAX_NUM_INSTANCE; i++)
    {
        if(s_vpu_inst_info[i].flag != 0 && s_vpu_inst_info[i].url != NULL) {
            sprintf(temp, "{\"id\" : \"%pU\", \"time\" : %lu, \"status\" : %lu, \"url\" : \"%s\"},\n", s_vpu_inst_info[i].uuid, s_vpu_inst_info[i].time, s_vpu_inst_info[i].status, s_vpu_inst_info[i].url);
            single_list_len = strlen(temp);
            if(single_list_len + total_len > size - 1 ){
                pr_err("maybe overflow the memory, please check..\n");
                break;
            }
            sprintf(mem + total_len, "%s",temp);
            total_len = strlen(mem);
        }
    }
    if(total_len > total_len_first)
        sprintf(mem + total_len - 2, "]}\n");
    else
    {
        sprintf(mem + total_len, "]}\n");
    }
}
#endif
#define MAX_DATA_SIZE (2048* get_vpu_core_num(chip_id, video_cap)*MAX_NUM_INSTANCE) // [size(url)+size(uuid)+size(misc)]*core*inst + size(head)
static ssize_t info_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    char *dat = vmalloc(MAX_DATA_SIZE);
    int len = 0;
    int err = 0;
    int i = 0;
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
    vmem_info_t vinfo;
    vmem_get_info(&s_vmem, &vinfo);

    size = vinfo.free_pages * (unsigned long)vinfo.page_size;
    sprintf(dat, "{\"total_mem_size\" : %lld, \"used_mem_size\" : %lld, \"free_mem_size\" : %lld,\n", \
            s_vpu_usage_info.vpu_mem_total_size, s_vpu_usage_info.vpu_mem_used_size, s_vpu_usage_info.vpu_mem_free_size);
#else
    sprintf(dat, "{");
#endif
    len = strlen(dat);
    sprintf(dat + len, "\"core\" : [\n");
    for(i=0; i< get_vpu_core_num(chip_id, video_cap)-1; i++) {
        len = strlen(dat);
        sprintf(dat + len, "{\"id\":%d, \"link_num\":%d, \"usage(instant|long)\":%d%%|%llu%%, \"fps\":%d}, \n", i, s_vpu_usage_info.vpu_open_ref_count[i], \
                s_vpu_usage_info.vpu_instant_usage[i], s_vpu_usage_info.vpu_working_time_in_ms[i]*100/s_vpu_usage_info.vpu_total_time_in_ms[i], \
                s_vpu_usage_info.vpu_realtime_fps[i]);
    }
    len = strlen(dat);
    sprintf(dat + len, "{\"id\":%d, \"link_num\":%d, \"usage(instant|long)\":%d%%|%llu%%, \"fps\":%d}]}\n", i, s_vpu_usage_info.vpu_open_ref_count[i], \
            s_vpu_usage_info.vpu_instant_usage[i], s_vpu_usage_info.vpu_working_time_in_ms[i]*100/s_vpu_usage_info.vpu_total_time_in_ms[i], \
            s_vpu_usage_info.vpu_realtime_fps[i]);

    len = strlen(dat);
    // if ((err =  mutex_lock_interruptible(&s_vpu_proc_lock)) == 0) {
    //     // printf_inst_info(dat+len, MAX_DATA_SIZE-len);
    //     mutex_unlock(&s_vpu_proc_lock);
    // }
    len = strlen(dat) + 1;
    if (size < len) {
        printk("read buf too small\n");
        err = -EIO;
        goto info_read_error;
    }

    if (*ppos >= len) {
        err = -1;
        goto info_read_error;
    }

    err = copy_to_user(buf, dat, len);
info_read_error:
    if (err)
    {
        vfree(dat);
        return 0;
    }

    *ppos = len;
    vfree(dat);
    return len;
}
static ssize_t info_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
//for debug the vpu.
    char data[1024] = { 0 };
    int ret = 0;
    int err = 0;
    int core_idx = -1;
    int inst_idx = -1;
    int in_num = -1;
    int out_num = -1;
    int len;
    unsigned long flag;
    data[1023] = '\0';
    len = size > 1023 ? 1023 : size;
    //if(size > 1024) { // not to use vmalloc here, to avoid overflow attack from application
    //    data[1023] = '\0';
    //    len = 1023;
    //} else
    //    len = size;
    err = copy_from_user(data, buf, len);
    if (err) return -EINVAL;

    sscanf(data, "%ld ", &flag);
    //pr_info("flag : %ld\n", flag);

    if(flag >  get_vpu_core_num(chip_id, video_cap)) {
        unsigned long __time;
        unsigned long __status;
        unsigned char *__url;
        __url = (unsigned char *)vmalloc(1024);
        __url[0]='\0'; //if in sscanf No URL scanned There may be no '\0' terminator
        if(__url == NULL){
            pr_info("vmalloc url memory failed\n");
            return -ENOMEM;
        }

        ret = sscanf(data, "%ld %ld %ld, %s", &flag, &__time, &__status, __url);
        #if 0
        if ((err =  mutex_lock_interruptible(&s_vpu_proc_lock)) == 0) {
            if(__status == 1) {
                set_stop_inst_info(flag);
            }
            else {
                clean_timeout_inst_info(60*1000);

                i = get_inst_info_idx(flag);
                if(i< get_vpu_core_num(chip_id, video_cap)*MAX_NUM_INSTANCE) {
                    if(s_vpu_inst_info[i].url != NULL)
                        vfree(s_vpu_inst_info[i].url);
                    j = strlen(__url);
                    s_vpu_inst_info[i].url = vmalloc(j+1);
                    memcpy(s_vpu_inst_info[i].url, __url, j);
                    s_vpu_inst_info[i].url[j] = 0;
                    s_vpu_inst_info[i].time = __time;
                    s_vpu_inst_info[i].status = __status;
                    s_vpu_inst_info[i].flag = flag;
                    s_vpu_inst_info[i].pid = current->tgid;
                    generate_random_uuid(s_vpu_inst_info[i].uuid);
                }
            }
            mutex_unlock(&s_vpu_proc_lock);
        }
        #endif

        if (__url) vfree(__url);
        return size;
    }

    sscanf(data, "%d %d %d %d", &core_idx, &inst_idx, &in_num, &out_num);
    pr_info("%s", data);
    pr_info("core_idx: %d, inst: %d, in_num: %d, out_num: %d\n", core_idx, inst_idx, in_num, out_num);
#if 0
    if(s_vpu_dump_flag) {
        if(core_idx >=0 && core_idx <  get_vpu_core_num(chip_id, video_cap) && inst_idx >= 0 && inst_idx < MAX_NUM_INSTANCE)
        {
            if(in_num >= 0)
                s_vpu_dump_flag[core_idx * MAX_NUM_INSTANCE * 2 + inst_idx * 2] = in_num;
            if(out_num >= 0)
                s_vpu_dump_flag[core_idx * MAX_NUM_INSTANCE * 2 + inst_idx * 2 + 1] = out_num;
        }
        else if(core_idx == -1) {
            for(i=0; i <  get_vpu_core_num(chip_id, video_cap); i++)
            {
                for(j=0; j < 2*MAX_NUM_INSTANCE; j=j+2)
                {
                    pr_info("[%d, %d]; ", s_vpu_dump_flag[i*MAX_NUM_INSTANCE*2+j], s_vpu_dump_flag[i*MAX_NUM_INSTANCE*2+j+1]);
                }
                pr_info("\n");
            }
        }
        else if(core_idx == -2)
        {
            memset(s_vpu_dump_flag, 0, sizeof(u32)* get_vpu_core_num(chip_id, video_cap)*MAX_NUM_INSTANCE*2);
        }
    }
#endif
    return size;
}
static const struct proc_ops proc_info_operations = {
    .proc_read   = info_read,
    .proc_write  = info_write,
};

int vpu_drv_platform_init(struct platform_device *pdev)
{
    int ret;
    int i;
    int err = 0;
    struct resource *res = NULL;
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
    struct device_node *target = NULL;
    struct resource rmem;
#endif
    DPRINTK("[VPUDRV] begin vpu_init\n");

    // todo: get chip_id
    chip_id = 0x1866;

    for (i=0; i<MAX_NUM_INSTANCE*get_vpu_core_num(chip_id, video_cap); i++) {
        init_waitqueue_head(&s_interrupt_wait_q[i]);
    }

    for (i=0; i<MAX_NUM_INSTANCE*get_vpu_core_num(chip_id, video_cap); i++) {
#define MAX_INTERRUPT_QUEUE (16*MAX_NUM_INSTANCE)
        ret = kfifo_alloc(&s_interrupt_pending_q[i], MAX_INTERRUPT_QUEUE*sizeof(u32), GFP_KERNEL);
        if (ret) {
            DPRINTK("[VPUDRV] kfifo_alloc failed 0x%x\n", res);
        }
    }
    mutex_lock(&s_vpu_lock);
    for(i=0; i< get_vpu_core_num(chip_id, video_cap); i++) {
        s_instance_pool[i].base = 0;
        s_common_memory[i].base = 0;
    }
    mutex_unlock(&s_vpu_lock);

    for(i=0; i< MAX_NUM_VPU_CORE; i++) {
        if (pdev)
            res = platform_get_resource(pdev, IORESOURCE_MEM, i+ MAX_NUM_JPU_CORE);
        if (res) {/* if platform driver is implemented */
            s_vpu_register[i].phys_addr = res->start;
            s_vpu_register[i].virt_addr = (unsigned long)IOREMAP(res->start, res->end - res->start);
            s_vpu_register[i].size = res->end - res->start;
            DPRINTK("[VPUDRV] : vpu base address get from platform driver physical base addr==0x%lx, virtual base=0x%lx, core idx = %d\n", s_vpu_register[i].phys_addr , s_vpu_register[i].virt_addr, i);
        } else {
            s_vpu_register[i].phys_addr = s_vpu_reg_phy_base[i];
            s_vpu_register[i].virt_addr = (unsigned long)IOREMAP(s_vpu_register[i].phys_addr, VPU_REG_SIZE);
            s_vpu_register[i].size = VPU_REG_SIZE;
            DPRINTK("[VPUDRV] : vpu base address get from defined value physical base addr==0x%lx, virtual base=0x%lx, core idx = %d\n", s_vpu_register[i].phys_addr, s_vpu_register[i].virt_addr, i);
        }
    }

    #ifdef VPU_SUPPORT_ISR
#ifdef VPU_SUPPORT_PLATFORM_DRIVER_REGISTER
    for (i=0; i< MAX_NUM_VPU_CORE; i++) {
        if (pdev) {
            res = platform_get_resource(pdev, IORESOURCE_IRQ, i + MAX_NUM_JPU_CORE);
        }

        if (res) {/* if platform driver is implemented */
            s_vpu_irq[i] = res->start;
            printk(KERN_INFO "[VPUDRV] : vpu irq number get from platform driver %d irq=0x%x\n", i, s_vpu_irq[i]);
        } else {
            printk(KERN_ERR "[VPUDRV] : vpu irq number get from defined value irq=0x%x\n", s_vpu_irq[i]);
        }
#else
        pr_err("[VPUDRV] : vpu irq number get from defined value irq=0x%x\n", s_vpu_irq[i]);
#endif
        err = request_irq(s_vpu_irq[i], vpu_irq_handler, IRQF_TRIGGER_NONE, "VPU_CODEC_IRQ", (void *)(&s_vpu_drv_context[i]));
        if (err) {
            printk(KERN_ERR "[VPUDRV] :    fail to register interrupt handler, err:%d\n", err);
            goto ERROR_PROVE_DEVICE;
        }
    }

#endif

    vpu_dev = &pdev->dev;
    dma_set_mask_and_coherent(vpu_dev, DMA_BIT_MASK(64));
    if (err) {
        printk(KERN_ERR "dma_set_mask_and_coherent 64 fail\n");
        err = dma_set_mask_and_coherent(vpu_dev, DMA_BIT_MASK(32));
    }
    if (err) {
        printk(KERN_ERR "dma_set_mask_and_coherent 32 fail\n");
    }
    DPRINTK("[VPUDRV] success to probe vpu device with non reserved video memory\n");

    entry = proc_create("soph/vpuinfo", 0666, NULL, &proc_info_operations);

    ret = vpu_register_clk(pdev);
    if (ret != 0) {
        DPRINTK("[VPUDRV] vpu register clk error\n");
    }

    return 0;

ERROR_PROVE_DEVICE:

    for(i=0; i< get_vpu_core_num(chip_id,video_cap); i++) {
        if (s_vpu_register[i].virt_addr)
            iounmap((void *)s_vpu_register[i].virt_addr);
    }

    printk("[VPUDRV]  parse reserved mem end\n");
    return err;
}


int vpu_drv_platform_exit(void)
{
    int i;
    DPRINTK("[VPUDRV] vpu_remove\n");

    mutex_lock(&s_vpu_lock);
    for (i=0; i< get_vpu_core_num(chip_id, video_cap); i++) {
        if (s_instance_pool[i].base) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
            vfree((const void *)s_instance_pool[i].base);
#else
            vpu_free_dma_buffer(&s_instance_pool[i]);
#endif
            s_instance_pool[i].base = 0;
        }

        if (s_common_memory[i].base) {
            vpu_free_dma_buffer(&s_common_memory[i]);
            s_common_memory[i].base = 0;
        }
    }
    mutex_unlock(&s_vpu_lock);


#ifdef VPU_SUPPORT_ISR
    for (i=0; i< MAX_NUM_VPU_CORE; i++) {
        if( s_vpu_irq[i] )
            free_irq(s_vpu_irq[i], &s_vpu_drv_context[i]);
    }
#endif

    for(i=0; i< get_vpu_core_num(chip_id,video_cap); i++) {
        if (s_vpu_register[i].virt_addr)
            iounmap((void *)s_vpu_register[i].virt_addr);
    }

    /* stop vpu monitor thread */
    if (s_vpu_monitor_task){
        kthread_stop(s_vpu_monitor_task);
        s_vpu_monitor_task = NULL;
        pr_info("vpu monitor thread released\n");
    }

    if (entry) {
        proc_remove(entry);
        entry = NULL;
    }
    return 0;
}

#ifdef VPU_SUPPORT_CLOCK_CONTROL
struct clk *vpu_clk_get(struct device *dev)
{
    return clk_get(dev, VPU_CLK_NAME);
}
void vpu_clk_put(struct clk *clk)
{
    if (!(clk == NULL || IS_ERR(clk)))
        clk_put(clk);
}

void vpu_clk_enable(int core_idx)
{
    int i = 0;
    if (core_idx == 0) {
        for (i = 0; i < 3; i++) {
            if (!__clk_is_enabled(vpu_pw_ctl.ve_clk[i]))
                clk_prepare_enable(vpu_pw_ctl.ve_clk[i]);
        }
    } else if (core_idx == 1) {
        for (i = 0; i < 4; i++) {
            if (!__clk_is_enabled(vpu_pw_ctl.vd_core0_clk[i]))
                clk_prepare_enable(vpu_pw_ctl.vd_core0_clk[i]);
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (!__clk_is_enabled(vpu_pw_ctl.vd_core1_clk[i]))
                clk_prepare_enable(vpu_pw_ctl.vd_core1_clk[i]);
        }
    }
}

void vpu_clk_disable(int core_idx)
{
    int i;
    if (core_idx == 0) {
        for (i = 0; i < 3; i++) {
            if (__clk_is_enabled(vpu_pw_ctl.ve_clk[i]))
                clk_disable_unprepare(vpu_pw_ctl.ve_clk[i]);
        }

    } else if (core_idx == 1){
        for (i = 0; i < 4; i++) {
            if (__clk_is_enabled(vpu_pw_ctl.vd_core0_clk[i]))
                clk_disable_unprepare(vpu_pw_ctl.vd_core0_clk[i]);
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (__clk_is_enabled(vpu_pw_ctl.vd_core1_clk[i]))
                clk_disable_unprepare(vpu_pw_ctl.vd_core1_clk[i]);
        }
    }
}
#endif

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
        usleep_range(5, 10);   // delay more to give idle time to OS;
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
            printk(KERN_ERR "%s timeout cmd=0x%x, pc=0x%x\n", __FUNCTION__, cmd, pc);
            ret = VPUAPI_RET_TIMEOUT;
            break;
        }
        usleep_range(5, 10);    // delay more to give idle time to OS;
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
            printk(KERN_ERR "%s timeout \n", __FUNCTION__);
            ret = VPUAPI_RET_TIMEOUT;
            break;
        }
        usleep_range(5, 10);    // delay more to give idle time to OS;
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
        u64 codeBase;
        u32 codeSize;
        u32 originValue;

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

        val = W5_RST_BLOCK_ALL;
        WriteVpuRegister(W5_VPU_RESET_REQ, val);

        if (vpuapi_wait_vpu_busy(core, W5_VPU_RESET_STATUS) == VPUAPI_RET_TIMEOUT) {
            WriteVpuRegister(W5_VPU_RESET_REQ, 0);
            return VPUAPI_RET_TIMEOUT;
        }

        WriteVpuRegister(W5_VPU_RESET_REQ, 0);

        codeBase = s_common_memory[core].phys_addr;
        codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
        if (core == 0) {
            unsigned int *reg_addr = ioremap(VE_TOP_EXT_ADDR, 4);
            originValue = readl(reg_addr);
            writel((codeBase>>32) | originValue, reg_addr);
        } else {
            WriteVpuFIORegister(core, 0xFEC0, codeBase>>32);
            WriteVpuFIORegister(core, 0x8EC0, codeBase>>32);
            WriteVpuFIORegister(core, 0x8EC4, codeBase>>32);
        }

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
        u32 codeBase;
        u32 i;

        codeBase = s_common_memory[core].phys_addr;

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

        msleep(10);    // delay for vpuapi_close

        if (time_after(jiffies, timeout)) {
            printk(KERN_ERR "[VPUDRV]%s vpuapi_close flow timeout ret=%d, inst=%d\n", __FUNCTION__, ret, inst);
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

int vpu_close_instance_internal(u32 core, u32 inst)
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
        printk(KERN_ERR "[VPUDRV]vpu_close_instance Unknown product id : %08x\n", product_code);
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

    product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
    if (PRODUCT_CODE_W5_SERIES(product_code)) {
        WriteVpuRegister(W5_VPU_BUSY_STATUS, 0);
        DPRINTK("[VPUDRV]%s mode=%d\n", __FUNCTION__, reset_mode);
        if (reset_mode == 0) {
            ret = wave_sleep_wake(core, VPU_SLEEP_MODE);
            DPRINTK("[VPUDRV]%s Sleep done ret=%d\n", __FUNCTION__, ret);
            if (ret != VPUAPI_RET_SUCCESS) {
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
                    return VPUAPI_RET_TIMEOUT;
                }

                WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE1, 0x7);
                if (vpuapi_wait_bus_busy(core, W5_BACKBONE_BUS_STATUS_VCORE1) != VPUAPI_RET_SUCCESS) {
                    WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE1, 0x00);
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
                            return VPUAPI_RET_TIMEOUT;
                        }
                    }
                    // Step1 : disable request
                    WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x7);

                    // Step2 : Waiting for completion of bus transaction
                    if (vpuapi_wait_bus_busy(core, W5_BACKBONE_BUS_STATUS_VCORE0) != VPUAPI_RET_SUCCESS) {
                        WriteVpuFIORegister(core, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
                        return VPUAPI_RET_TIMEOUT;
                    }
                }
                else {
                    // Step1 : disable request
                    WriteVpuFIORegister(core, W5_COMBINED_BACKBONE_BUS_CTRL, 0x7);

                    // Step2 : Waiting for completion of bus transaction
                    if (vpuapi_wait_bus_busy(core, W5_COMBINED_BACKBONE_BUS_STATUS) != VPUAPI_RET_SUCCESS) {
                        WriteVpuFIORegister(core, W5_COMBINED_BACKBONE_BUS_CTRL, 0x00);
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
        return VPUAPI_RET_INVALID_PARAM;
    } else if (PRODUCT_CODE_W6_SERIES(product_code)) {
        DPRINTK("[VPUDRV] %s doesn't support swreset for wave6 \n", __FUNCTION__);
        return VPUAPI_RET_INVALID_PARAM;
    } else {
        printk(KERN_ERR "[VPUDRV] Unknown product code : %08x\n", product_code);
        return -1;
    }

    ret = wave_sleep_wake(core, VPU_WAKE_MODE);

    DPRINTK("[VPUDRV]%s Wake done ret = %d\n", __FUNCTION__, ret);

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
        printk(KERN_ERR "[VPUDRV]%s fail1 ret=%d\n", __FUNCTION__, ret);
        return ret;
    }

    if (ReadVpuRegister(W5_RET_SUCCESS) == 0) {
        printk(KERN_ERR "[VPUDRV]%s success=%d\n", __FUNCTION__, ReadVpuRegister(W5_RET_SUCCESS));
        return VPUAPI_RET_FAILURE;
    }

    return VPUAPI_RET_SUCCESS;
}
int vpuapi_get_output_info(u32 core, u32 inst, u32 *error_reason)
{
    int ret = VPUAPI_RET_SUCCESS;
    u32 val;

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
        printk(KERN_ERR "[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
    }

    return ret;
}
int vpuapi_dec_clr_all_disp_flag(u32 core, u32 inst)
{
    int ret = VPUAPI_RET_SUCCESS;
    u32 val = 0;

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
        printk(KERN_ERR "[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
    }

    return ret;
}
int vpuapi_dec_set_stream_end(u32 core, u32 inst)
{
    int ret = VPUAPI_RET_SUCCESS;
    u32 val;
    int product_code;

    product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

    if (PRODUCT_CODE_W6_SERIES(product_code)) {
        // nothing to do
        printk(KERN_INFO "vpuapi_dec_set_stream_end(): Please implement vpuapi_dec_set_stream_end for wave6 decoder.\n");
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
        printk(KERN_ERR "[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
    }

    return ret;
}
int vpuapi_close(u32 core, u32 inst)
{
    int ret = VPUAPI_RET_SUCCESS;
    u32 val;
    int product_code;

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
        printk(KERN_ERR "[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);
    }
    DPRINTK("[VPUDRV]-%s ret=%d\n", __FUNCTION__, ret);

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
    DPRINTK("[VPUDRV]-%s vip->doSwResetInstIdx=%d, ret=%d\n", __FUNCTION__, vip->doSwResetInstIdxPlus1, ret);
    msleep(10);
    return ret;
}


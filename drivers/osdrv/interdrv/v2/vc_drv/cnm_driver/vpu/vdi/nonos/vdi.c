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

#include <stdio.h>
#include <stdlib.h>
#include "../vdi.h"
#include "../vdi_osal.h"
#include "../mm.h"
#include "coda9/coda9_regdefine.h"
#include "wave/wave5_regdefine.h"
#include "wave/wave6_regdefine.h"
#include "vpuapi/vpuapi.h"
#include "main_helper.h"
#include "misc/debug.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#else
#if (REQUIRED_VPU_MEMORY_SIZE > VPUDRV_INIT_VIDEO_MEMORY_SIZE_IN_BYTE)
#error "Warnning : VPU memory will be overflow"
#endif
#endif

// #define SUPPORT_MULTI_INST_INTR_WITH_THREAD
#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
#else
static int vpu_irq_handler(void *context);
#endif

#define VPU_BIT_REG_SIZE                    (0x4000*MAX_NUM_VPU_CORE)
#    define VPU_BIT_REG_BASE                0x40000000

#define VDI_SRAM_BASE_ADDR                  0x00
#define VPU_DRAM_PHYSICAL_BASE              0x00
#define VPU_DRAM_SIZE                       (128*1024*1024)

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define VPU_CORE_BASE_OFFSET                0x4000
#endif

#define VDI_SYSTEM_ENDIAN                   VDI_LITTLE_ENDIAN
#define VDI_128BIT_BUS_SYSTEM_ENDIAN        VDI_128BIT_LITTLE_ENDIAN



typedef struct vpu_buffer_t vpudrv_buffer_t;

typedef struct vpu_buffer_pool_t
{
    vpudrv_buffer_t vdb;
    int inuse;
} vpu_buffer_pool_t;

typedef struct  {
    unsigned long coreIdx;
    int vpu_fd;
    vpu_instance_pool_t *pvip;
    int task_num;
    int clock_state;
    vpudrv_buffer_t vdb_video_memory;
    vpudrv_buffer_t vdb_register;
    vpu_buffer_t vpu_common_memory;
    vpu_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    int vpu_buffer_pool_count;
    int product_code;

} vdi_info_t;

static vdi_info_t s_vdi_info[MAX_VPU_CORE_NUM];
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
static vpu_instance_pool_t s_vip[MAX_VPU_CORE_NUM];	// it can be used for a buffer space to save context for app process. to support running VPU in multiple process. this space should be a shared buffer.
#else
static vpu_instance_pool_t s_vip;	// it can be used for a buffer space to save context for app process. to support running VPU in multiple process. this space should be a shared buffer.
#endif
static int swap_endian(unsigned long coreIdx, unsigned char *data, int len, int endian);

#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
#include <string.h>
#include <pthread.h>
#endif
static int request_irq(unsigned long coreIdx);
static void free_irq(unsigned long coreIdx);
typedef struct  {
    int core_idx;
#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
    pthread_t thread_id;
#endif
    int thread_run;
    int product_code;
    Queue *intrQ[MAX_NUM_INSTANCE];
} IrqContext;
IrqContext s_irqContext[MAX_NUM_VPU_CORE];

int vdi_lock(unsigned long coreIdx)
{
    /* need to implement */
    return 0;
}

void vdi_unlock(unsigned long coreIdx)
{
    /* need to implement */
}

int vdi_disp_lock(unsigned long coreIdx)
{
    /* need to implement */

    return 0;
}

void vdi_disp_unlock(unsigned long coreIdx)
{

    /* need to implement */
}



int vmem_lock(unsigned long coreIdx)
{

    /* need to implement */

    return 0;
}

void vmem_unlock(unsigned long coreIdx)
{
    /* need to implement */
}


int vdi_probe(unsigned long coreIdx)
{
    int ret;

    ret = vdi_init(coreIdx);
    vdi_release(coreIdx);

    return ret;
}


int vdi_init(unsigned long coreIdx)
{
    int ret;
    vdi_info_t *vdi;
    int i;
    Uint32 product_code;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[coreIdx];

    if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
    {
        vdi->task_num++;
        return 0;
    }

    vdi->vpu_fd = 1;
    osal_memset(vdi->vpu_buffer_pool, 0x00, sizeof(vpu_buffer_pool_t)*MAX_VPU_BUFFER_POOL);

    if (!vdi_get_instance_pool(coreIdx))
    {
        VLOG(ERR, "[VDI] fail to create shared info for saving context \n");
        goto ERR_VDI_INIT;
    }

    vdi->vdb_video_memory.phys_addr = VPU_DRAM_PHYSICAL_BASE;
    vdi->vdb_video_memory.size      = VPU_DRAM_SIZE;

#if 0
    if (REQUIRED_VPU_MEMORY_SIZE > vdi->vdb_video_memory.size)
    {
        VLOG(ERR, "[VDI] Warning : VPU memory will be overflow\n");
    }
#endif

    if (vdi_allocate_common_memory(coreIdx) < 0)
    {
        VLOG(ERR, "[VDI] fail to get vpu common buffer from driver\n");
        goto ERR_VDI_INIT;
    }

    if (!vdi->pvip->instance_pool_inited) {

        int* pCodecInst;
        osal_memset(&vdi->pvip->vmem, 0x00, sizeof(video_mm_t));

        for( i = 0; i < MAX_NUM_INSTANCE; i++) {
            pCodecInst = (int *)vdi->pvip->codecInstPool[i];
            pCodecInst[1] = i;    // indicate instIndex of CodecInst
            pCodecInst[0] = 0;    // indicate inUse of CodecInst
        }
        vdi->pvip->instance_pool_inited = 1;
    }

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    ret = vmem_init(&vdi->pvip->vmem, vdi->vdb_video_memory.phys_addr + (vdi->pvip->vpu_common_buffer.size*MAX_VPU_CORE_NUM),
            vdi->vdb_video_memory.size - (vdi->pvip->vpu_common_buffer.size*MAX_VPU_CORE_NUM));
#else
    ret = vmem_init(&vdi->pvip->vmem, vdi->vdb_video_memory.phys_addr + vdi->pvip->vpu_common_buffer.size,
            vdi->vdb_video_memory.size - vdi->pvip->vpu_common_buffer.size);
#endif

    if (ret < 0)
    {
        VLOG(ERR, "[VDI] fail to init vpu memory management logic\n");
        goto ERR_VDI_INIT;
    }

    vdi->vdb_register.phys_addr = VPU_BIT_REG_BASE;
    vdi->vdb_register.virt_addr = VPU_BIT_REG_BASE;
    vdi->vdb_register.size = VPU_BIT_REG_SIZE;

    vdi_set_clock_gate(coreIdx, TRUE);
    request_irq(coreIdx);
    vdi->product_code = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    product_code = vdi->product_code;
    if (PRODUCT_CODE_W_SERIES(product_code))
    {
        if (vdi_read_register(coreIdx, W5_VCPU_CUR_PC) == 0) // if BIT processor is not running.
        {
            for (i=0; i<64; i++)
                vdi_write_register(coreIdx, (i*4) + 0x100, 0x0);
        }
    }
    else // CODA9XX
    {
        if (vdi_read_register(coreIdx, BIT_CUR_PC) == 0) // if BIT processor is not running.
        {
            for (i=0; i<64; i++)
                vdi_write_register(coreIdx, (i*4) + 0x100, 0x0);
        }
    }
    vdi_set_clock_gate(coreIdx, FALSE);

    vdi->coreIdx = coreIdx;
    vdi->task_num++;

    VLOG(INFO, "[VDI] success to init driver \n");

    return 0;

ERR_VDI_INIT:

    vdi_release(coreIdx);
    return -1;
}

int vdi_set_bit_firmware_to_pm(unsigned long coreIdx, const unsigned short *code)
{
    return 0;
}

int vdi_release(unsigned long coreIdx)
{
    int i;
    vpudrv_buffer_t vdb = {0, };
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return 0;

    if (vdi_lock(coreIdx) < 0)
    {
        VLOG(ERR, "[VDI] fail to handle lock function\n");
        return -1;
    }

    if (vdi->task_num > 1) // means that the opened instance remains
    {
        vdi->task_num--;
        vdi_unlock(coreIdx);
        return 0;
    }

    free_irq(coreIdx);
    vmem_exit(&vdi->pvip->vmem);

    osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));

    // get common memory information to free virtual address
    vdb.size = 0;
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr &&
                vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size))
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (vdb.size > 0)
        osal_memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));

    vdi->task_num--;
    vdi->vpu_fd = -1;

    vdi_unlock(coreIdx);

    osal_memset(vdi, 0x00, sizeof(vdi_info_t));

    return 0;
}

int vdi_get_common_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

    return 0;
}

int vdi_allocate_common_memory(unsigned long core_idx)
{
    vdi_info_t *vdi = &s_vdi_info[core_idx];
    vpudrv_buffer_t vdb;
    int i;

    if (core_idx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
        return -1;

    if (vdi->pvip->vpu_common_buffer.size == 0)
    {
        vdb.size = SIZE_COMMON*MAX_VPU_CORE_NUM;
        vdb.phys_addr = vdi->vdb_video_memory.phys_addr; // set at the beginning of base address
        vdb.virt_addr = vdi->vdb_video_memory.phys_addr;
        vdb.base = vdi->vdb_video_memory.base;

        // convert os driver buffer type to vpu buffer type
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
        vdi->pvip->vpu_common_buffer.phys_addr = (PhysicalAddress)(vdb.phys_addr + (core_idx*SIZE_COMMON));
        vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base + (core_idx*SIZE_COMMON));
        vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (core_idx*SIZE_COMMON));
#else
        vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
        vdi->pvip->vpu_common_buffer.phys_addr = (PhysicalAddress)(vdb.phys_addr);
        vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base);
        vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr);
#endif

        osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

    }
    else
    {
        vdb.size = SIZE_COMMON*MAX_VPU_CORE_NUM;
        vdb.phys_addr = vdi->vdb_video_memory.phys_addr; // set at the beginning of base address
        vdb.base =  vdi->vdb_video_memory.base;
        vdb.virt_addr = vdi->vdb_video_memory.phys_addr;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (core_idx*SIZE_COMMON));
#else
        vdi->pvip->vpu_common_buffer.virt_addr = vdb.virt_addr;
#endif
        osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

        VLOG(INFO, "[VDI] vdi_allocate_common_memory, physaddr=0x%x, virtaddr=0x%x\n", (int)vdi->pvip->vpu_common_buffer.phys_addr, (int)vdi->pvip->vpu_common_buffer.virt_addr);
    }

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    VLOG(INFO, "[VDI] vdi_get_common_memory physaddr=0x%x, size=%d, virtaddr=0x%x\n", (int)vdi->vpu_common_memory.phys_addr, (int)vdi->vpu_common_memory.size, (int)vdi->vpu_common_memory.virt_addr);

    return 0;
}

vpu_instance_pool_t *vdi_get_instance_pool(unsigned long coreIdx)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return NULL;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd ==0x00 )
        return NULL;

    if (!vdi->pvip)
    {
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdi->pvip = &s_vip[coreIdx];
#else
        vdi->pvip = &s_vip;
#endif
        osal_memset(vdi->pvip, 0, sizeof(vpu_instance_pool_t));
    }

    return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(unsigned long coreIdx, unsigned long instIdx, int support_cq)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdi->pvip->vpu_instance_num++;

    return 0;
}

int vdi_close_instance(unsigned long coreIdx, unsigned long instIdx)
{
    vdi_info_t *vdi = NULL;
    IrqContext *irqContext = &s_irqContext[coreIdx];
    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdi->pvip->vpu_instance_num--;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        Queue_Flush(irqContext->intrQ[instIdx]);
    }
    else {
        if (vdi->task_num > 1) {// means that the opened instance remains
            vdi_unlock(coreIdx);
            return 0;
        }
        Queue_Flush(irqContext->intrQ[0]); // coda only use intrQ[0]
    }
    return 0;
}

int vdi_get_instance_num(unsigned long coreIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->pvip->vpu_instance_num;
}

int vdi_hw_reset(unsigned long coreIdx) // DEVICE_ADDR_SW_RESET
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || !vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    // to do any action for hw reset

    return 0;
}

int vdi_vpu_reset(unsigned long coreIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_VPU_CORE_NUM)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || !vdi || vdi->vpu_fd ==-1 || vdi->vpu_fd == 0x00)
        return -1;

    // to do any action for vpu reset

    return 0;
}

void vdi_write_register(unsigned long coreIdx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi = NULL;
    unsigned int offset_addr = 0;
    unsigned long *reg_addr;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    offset_addr = coreIdx * VPU_CORE_BASE_OFFSET;
#endif

    reg_addr = (unsigned long *)(addr + (unsigned long)vdi->vdb_register.virt_addr + offset_addr);
    *(volatile unsigned int *)reg_addr = data;
}

unsigned int vdi_read_register(unsigned long coreIdx, unsigned int addr)
{
    vdi_info_t *vdi = NULL;
    unsigned int offset_addr = 0;
    unsigned long *reg_addr;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return (unsigned int)-1;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    offset_addr = coreIdx * VPU_CORE_BASE_OFFSET;
#endif

    reg_addr = (unsigned long *)(addr + (unsigned long)vdi->vdb_register.virt_addr + offset_addr);
    return *(volatile unsigned int *)reg_addr;
}

#define FIO_TIMEOUT         10000
unsigned int vdi_fio_read_register(unsigned long coreIdx, unsigned int addr)
{
    unsigned int ctrl;
    unsigned int count = 0;
    unsigned int data  = 0xffffffff;

    ctrl  = (addr&0xffff);
    ctrl |= (0<<16);    /* read operation */
    vdi_write_register(coreIdx, W5_VPU_FIO_CTRL_ADDR, ctrl);
    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(coreIdx, W5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            data = vdi_read_register(coreIdx, W5_VPU_FIO_DATA);
            break;
        }
    }

    return data;
}

void vdi_fio_write_register(unsigned long coreIdx, unsigned int addr, unsigned int data)
{
    unsigned int ctrl;
    unsigned int count = 0;

    vdi_write_register(coreIdx, W5_VPU_FIO_DATA, data);
    ctrl  = (addr&0xffff);
    ctrl |= (1<<16);    /* write operation */
    vdi_write_register(coreIdx, W5_VPU_FIO_CTRL_ADDR, ctrl);

    count = FIO_TIMEOUT;
    while (count--) {
        ctrl = vdi_read_register(coreIdx, W5_VPU_FIO_CTRL_ADDR);
        if (ctrl & 0x80000000) {
            break;
        }
    }
}

int vdi_clear_memory(unsigned long coreIdx, PhysicalAddress addr, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset;
    int i;
    Uint8*  zero;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    offset = (unsigned long)(addr - vdb.phys_addr);

    zero = (Uint8*)osal_malloc(len);
    osal_memset((void*)zero, 0x00, len);

    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), zero, len);

    osal_free(zero);

    return len;
}

int vdi_write_memory(unsigned long coreIdx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb = {0};
    unsigned long offset;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) { //lint !e644
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", addr);
        return -1;
    }

    offset =  (unsigned long)(addr -vdb.phys_addr);

    swap_endian(coreIdx, data, len, endian);
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), data, len);

    return len;

}

int vdi_read_memory(unsigned long coreIdx, PhysicalAddress addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi = NULL;
    vpudrv_buffer_t vdb = {0};
    unsigned long offset;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) //lint !e644
        return -1;

    offset =  (unsigned long)(addr -vdb.phys_addr);

    osal_memcpy(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
    swap_endian(coreIdx, data, len,  endian);

    return len;
}

int vdi_allocate_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi = NULL;
    int i;
    unsigned long offset;
    vpudrv_buffer_t vdb = {0};

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdb.size = vb->size;
    vmem_lock(coreIdx);
    vdb.phys_addr = (PhysicalAddress)vmem_alloc(&vdi->pvip->vmem, vdb.size, 0);
    vmem_unlock(coreIdx);

    if ((PhysicalAddress)vdb.phys_addr == (PhysicalAddress)-1)
        return -1; // not enough memory

    offset = (unsigned long)(vdb.phys_addr - vdi->vdb_video_memory.phys_addr);
    vdb.base = (unsigned long )vdi->vdb_video_memory.base + offset;
    vdb.virt_addr = vdb.phys_addr;

    vb->phys_addr = (unsigned long)vdb.phys_addr;
    vb->base = (unsigned long)vdb.base;
    vb->virt_addr = (unsigned long)vb->phys_addr;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    if (MAX_VPU_BUFFER_POOL == i) {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory, vpu_buffer_pool_count=%d MAX_VPU_BUFFER_POOL=%d\n", vdi->vpu_buffer_pool_count, MAX_VPU_BUFFER_POOL);
        return -1;
    }

    return 0;
}

int vdi_attach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;
    unsigned long offset;
    vpudrv_buffer_t vdb = {0};

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    offset = (unsigned long)(vdb.phys_addr - vdi->vdb_video_memory.phys_addr);
    vdb.base = (unsigned long )vdi->vdb_video_memory.base + offset;
    vdb.virt_addr = vb->virt_addr;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
        else
        {
            if (vdi->vpu_buffer_pool[i].inuse == 0)
            {
                vdi->vpu_buffer_pool[i].vdb = vdb;
                vdi->vpu_buffer_pool_count++;
                vdi->vpu_buffer_pool[i].inuse = 1;
                break;
            }
        }
    }

    return 0;
}

int vdi_dettach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (vb->size == 0)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            break;
        }
    }

    return 0;
}

void vdi_free_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb, int memTypes, int instIndex)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb = {0};

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#else
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;
#endif
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return;

    if (vb->size == 0)
        return;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (!vdb.size) //lint !e644
    {
        VLOG(ERR, "[VDI] invalid buffer to free address = 0x%x\n", (int)vdb.virt_addr);
        return ;
    }
    vmem_lock(coreIdx);
    vmem_free(&vdi->pvip->vmem, (unsigned long)vdb.phys_addr, 0);
    vmem_unlock(coreIdx);
    osal_memset(vb, 0, sizeof(vpu_buffer_t));
}


int vdi_get_sram_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    Uint32      sram_size=0;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vb || !vdi)
        return -1;

    switch (vdi->product_code) {
    case BODA950_CODE:
    case CODA960_CODE:
    case CODA980_CODE:
    // 4K : 0x34600, FHD : 0x17D00
        sram_size = 0x34600; break;
    case WAVE511_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     */
        sram_size = 0x1F800; break;
    case WAVE517_CODE:
    /* 10bit profile : 8Kx8K -> 272384, 4Kx2K -> 104448
     */
        sram_size = 0x42800; break;
    case WAVE537_CODE:
    /* 10bit profile : 8Kx8K -> 272384, 4Kx2K -> 104448
     */
        sram_size = 0x42800; break;
    case WAVE521_CODE:
    /* 10bit profile : 8Kx8K -> 126976, 4Kx2K -> 63488
     */
        sram_size = 0x1F000; break;
    case WAVE521E1_CODE:
    /* 10bit profile : 8Kx8K -> 126976, 4Kx2K -> 63488
     */
        sram_size = 0x1F000; break;
    case WAVE521C_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     * NOTE: Decoder > Encoder
     */
        sram_size = 0x1F800; break;
    case WAVE521C_DUAL_CODE:
    /* 10bit profile : 8Kx8K -> 129024, 4Kx2K -> 64512
     * NOTE: Decoder > Encoder
     */
        sram_size = 0x1F800; break;
    case WAVE617_CODE:
    case WAVE627_CODE:
    case WAVE633_CODE:
    case WAVE637_CODE:
    case WAVE663_CODE:
    case WAVE677_CODE:
        sram_size = 0x1C1500; break;
    default:
        VLOG(ERR, "[VDI] check product_code(%x)\n", vdi->product_code);
        break;
    }


    // if we can know the sram address directly in vdi layer, we use it first for sdram address
    vb->phys_addr = VDI_SRAM_BASE_ADDR+(coreIdx*sram_size);
    vb->size      = sram_size;

    return 0;
}

int vdi_set_clock_gate(unsigned long coreIdx, int enable)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W5_SERIES(vdi->product_code) || PRODUCT_CODE_W6_D_SERIES(vdi->product_code) || vdi->product_code == 0) {
        // CommandQueue does not support clock gate
        // first value 0
        vdi->clock_state = 1;
        return 0;
    }

    vdi->clock_state = enable;

    return 0;
}

int vdi_get_clock_gate(unsigned long coreIdx)
{
    vdi_info_t *vdi = NULL;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->clock_state;
}

int vdi_wait_bus_busy(unsigned long coreIdx, int timeout, unsigned int gdi_busy_flag)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    Uint32 gdi_status_check_value = 0x3f;
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        gdi_status_check_value = 0x3f;
        if (PRODUCT_CODE_W6_SERIES(vdi->product_code)) {
            gdi_status_check_value = 0;
        }
        if (vdi->product_code == WAVE521C_CODE || vdi->product_code == WAVE521_CODE || vdi->product_code == WAVE521E1_CODE) {
            gdi_status_check_value = 0x00ff1f3f;
        }
    }
    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.

    while(1)
    {
        if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
            if (vdi_fio_read_register(coreIdx, gdi_busy_flag) == gdi_status_check_value) break;
        }
        else {
            if (vdi_read_register(coreIdx, gdi_busy_flag) == 0x77) break;
        }
        //osal_msleep(1);    // 1ms sec
        //if (count++ > timeout)
        //    return -1;
    }

    return 0;
}

int vdi_wait_vpu_busy(unsigned long coreIdx, int timeout, unsigned int addr_bit_busy_flag)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.

    while(1)
    {
        if (vdi_read_register(coreIdx, addr_bit_busy_flag) == 0)
            break;

        //osal_msleep(1);    // 1ms sec
        //if (count++ > timeout)
        //    return -1;
    }

    return 0;
}

int vdi_wait_vcpu_bus_busy(unsigned long coreIdx, int timeout, unsigned int gdi_busy_flag)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.
    while(1)
    {
        if (vdi_fio_read_register(coreIdx, gdi_busy_flag) == 0x00)
            break;
        //osal_msleep(1);    // 1ms sec
        //if (count++ > timeout)
        //    return -1;
    }

    return 0;
}

int vdi_wait_interrupt(unsigned long core_idx, unsigned int instIdx, int timeout)
{
    vdi_info_t *vdi = &s_vdi_info[core_idx];
    int intr_reason;
    //unsigned long cur_time;

    //VDI must implement timeout action in this function for multi-vpu core scheduling efficiency.
    //the setting small value as timeout gives a chance to wait the other vpu core.
    // Uint64           startTime, endTime;
    IrqContext *irqContext = &s_irqContext[core_idx];
    unsigned int *ptr_intr_reason;

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
#else
    // this case is for testing when there is no way to have thread and isr_handler in customer's system.
    // basically. the best is that vpu_irq_handler is called at irq_handler in interrupt service routine of customers's system.
    // if vpu_irq_handler is called at irq_handler. the below line can be removed.
    if (PRODUCT_CODE_W6_S_SERIES(vdi->product_code) || PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        int startTime=osal_gettime();
        int endTime=osal_gettime();
        while(vpu_irq_handler((void *)irqContext) < 0) {
            if(abs(endTime-startTime) >= (Uint64)timeout) {
                return -1;
            } else {
                osal_msleep(0);
                endTime=osal_gettime();
            }
        }
    } else {
        vpu_irq_handler((void *)irqContext);
    }
#endif

    //cur_time = jiffies;

    while(1)
    {
        ptr_intr_reason = (unsigned int *)Queue_Dequeue(irqContext->intrQ[instIdx]);
        if (ptr_intr_reason)
        {
            intr_reason = *ptr_intr_reason;
//            VLOG(INFO, "intr_reason=%d inst=%d\n", intr_reason, instIdx);
            if (intr_reason)
            {
                break;
            }
        }
        /*
           if(jiffies_to_msecs(jiffies - cur_time) > timeout) {
           return -1;
           }
           */
    }

    return intr_reason;
}

int vdi_get_system_endian(unsigned long coreIdx)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code))
        return VDI_128BIT_BUS_SYSTEM_ENDIAN;
    else
        return VDI_SYSTEM_ENDIAN;
}

int vdi_convert_endian(unsigned long coreIdx, unsigned int endian)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        switch (endian) {
            case VDI_LITTLE_ENDIAN:       endian = 0x00; break;
            case VDI_BIG_ENDIAN:          endian = 0x0f; break;
            case VDI_32BIT_LITTLE_ENDIAN: endian = 0x04; break;
            case VDI_32BIT_BIG_ENDIAN:    endian = 0x03; break;
        }
    }
    return (endian&0x0f);
}

static Uint32 convert_endian_coda9_to_wave4(Uint32 endian)
{
    Uint32 converted_endian = endian;
    switch(endian) {
        case VDI_LITTLE_ENDIAN:       converted_endian = 0; break;
        case VDI_BIG_ENDIAN:          converted_endian = 7; break;
        case VDI_32BIT_LITTLE_ENDIAN: converted_endian = 4; break;
        case VDI_32BIT_BIG_ENDIAN:    converted_endian = 3; break;
    }
    return converted_endian;
}

int swap_endian(unsigned long coreIdx, unsigned char *data, int len, int endian)
{
    vdi_info_t* vdi = &s_vdi_info[coreIdx];
    int         changes;
    int         sys_endian;
    BOOL        byteChange, wordChange, dwordChange, lwordChange;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
        sys_endian = VDI_128BIT_BUS_SYSTEM_ENDIAN;
    }
    else if(PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        sys_endian = VDI_SYSTEM_ENDIAN;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    endian     = vdi_convert_endian(coreIdx, endian);
    sys_endian = vdi_convert_endian(coreIdx, sys_endian);
    if (endian == sys_endian)
        return 0;

    if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
    }
    else if (PRODUCT_CODE_CODA_SERIES(vdi->product_code)) {
        endian     = convert_endian_coda9_to_wave4(endian);
        sys_endian = convert_endian_coda9_to_wave4(sys_endian);
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", vdi->product_code);
        return -1;
    }

    changes     = endian ^ sys_endian;
    byteChange  = changes&0x01;
    wordChange  = ((changes&0x02) == 0x02);
    dwordChange = ((changes&0x04) == 0x04);
    lwordChange = ((changes&0x08) == 0x08);

    if (byteChange)  byte_swap(data, len);
    if (wordChange)  word_swap(data, len);
    if (dwordChange) dword_swap(data, len);
    if (lwordChange) lword_swap(data, len);

    return 1;
}

#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
static pthread_mutex_t s_irqMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static inline unsigned int get_inst_idx(unsigned int reg_val)
{
    unsigned int inst_idx;
    int i;
    for (i=0; i < MAX_NUM_INSTANCE; i++) {
        if(((reg_val >> i)&0x01) == 1)
            break;
    }
    inst_idx = i;
    return inst_idx;
}

static Int32 get_vpu_inst_idx(Uint32 *reason, Uint32 empty_inst, Uint32 done_inst, Uint32 seq_inst)
{
    Int32  inst_idx;
    Uint32 reg_val;
    Uint32 int_reason;

    int_reason = *reason;
    //    VLOG(INFO, "[VPUDRV][+] int_reason=0x%x, empty_inst=0x%x, done_inst=0x%x\n", int_reason, empty_inst, done_inst);
    if (int_reason & (1 << INT_WAVE5_DEC_PIC)) {
        reg_val = done_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_DEC_PIC);
        //VLOG(INFO, "[VPUDRV]    %s, W5_RET_QUEUE_CMD_DONE_INST DEC_PIC reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    if (int_reason & (1 << INT_WAVE5_BSBUF_EMPTY)) {
        reg_val = empty_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason = (1 << INT_WAVE5_BSBUF_EMPTY);
        //VLOG(INFO, "[VPUDRV]    %s, W5_RET_BS_EMPTY_INST reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    if (int_reason & (1 << INT_WAVE5_INIT_SEQ)) {
        reg_val  = seq_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_INIT_SEQ);
        //VLOG(INFO, "[VPUDRV]    %s, W5_RET_QUEUE_CMD_DONE_INST INIT_SEQ reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
        reg_val = seq_inst;
        inst_idx = get_inst_idx(reg_val);
        *reason  = (1 << INT_WAVE5_ENC_SET_PARAM);
        //VLOG(INFO, "[VPUDRV]    %s, W5_RET_QUEUE_CMD_DONE_INST INT_WAVE5_ENC_SET_PARAM reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);
        goto GET_VPU_INST_IDX_HANDLED;
    }

    // if  interrupt is not for empty and dec_pic and init_seq.
    inst_idx = -1;
    *reason  = 0;
    //VLOG(INFO, "[VPUDRV]    %s, W5_RET_SEQ_DONE_INSTANCE_INFO reg_val=0x%x, inst_idx=%d\n", __func__, reg_val, inst_idx);

GET_VPU_INST_IDX_HANDLED:
    //VLOG(INFO, "[VPUDRV][-]%s, inst_idx=%d. *reason=0x%x\n", __func__, inst_idx, *reason);
    return inst_idx;
}

int vpu_irq_handler(void *context)
{
    IrqContext *irqContext = (IrqContext *)context;
    Uint32 intr_reason = 0;
    Int32  intr_inst_index = 0;

    //VLOG(INFO, "[VDI][+]%s\n", __func__);
    if (PRODUCT_CODE_W6_SERIES(irqContext->product_code)) {
        if (vdi_read_register(irqContext->core_idx, W6_VPU_VPU_INT_STS) > 0) {
            Uint32 reason;
            reason     = vdi_read_register(irqContext->core_idx, W6_VPU_VINT_REASON);
            if (Queue_Enqueue(irqContext->intrQ[intr_inst_index], &reason) == FALSE) {
                VLOG(ERR, "[VDI] :  Queue_Enqueue error count=%d \n", Queue_Get_Cnt(irqContext->intrQ[intr_inst_index]));
            }
            vdi_write_register(irqContext->core_idx, W6_VPU_VINT_REASON_CLR, reason);
            vdi_write_register(irqContext->core_idx, W6_VPU_VINT_CLEAR, 1);
        } else {
            return -1;
        }
    } else if (PRODUCT_CODE_W5_SERIES(irqContext->product_code)) {
        if (vdi_read_register(irqContext->core_idx, W5_VPU_VPU_INT_STS) > 0) {
            Uint32 empty_inst;
            Uint32 done_inst;
            Uint32 seq_inst;
            Uint32 i, reason, reason_clr;

            reason     = vdi_read_register(irqContext->core_idx, W5_VPU_VINT_REASON);
            empty_inst = vdi_read_register(irqContext->core_idx, W5_RET_BS_EMPTY_INST);
            done_inst  = vdi_read_register(irqContext->core_idx, W5_RET_QUEUE_CMD_DONE_INST);
            seq_inst   = vdi_read_register(irqContext->core_idx, W5_RET_SEQ_DONE_INSTANCE_INFO);
            reason_clr = reason;

            VLOG(INFO, "[VPUDRV] VDI reason=0x%x, empty_inst=0x%x, done_inst=0x%x, seq_inst=0x%x \n", reason, empty_inst, done_inst, seq_inst);

            for (i=0; i<MAX_NUM_INSTANCE; i++) {
                if (0 == empty_inst && 0 == done_inst && 0 == seq_inst) break;
                intr_reason     = reason;
                intr_inst_index = get_vpu_inst_idx(&intr_reason, empty_inst, done_inst, seq_inst);
                if (intr_inst_index >= 0 && intr_inst_index < MAX_NUM_INSTANCE) {
                    if (intr_reason == (1 << INT_WAVE5_BSBUF_EMPTY)) {
                        empty_inst = empty_inst & ~(1UL << intr_inst_index);
                        vdi_write_register(irqContext->core_idx, W5_RET_BS_EMPTY_INST, empty_inst);
                        if (0 == empty_inst) {
                            reason &= ~(1<<INT_WAVE5_BSBUF_EMPTY);
                        }
                    }
                    if (intr_reason == (1 << INT_WAVE5_DEC_PIC)) {
                        done_inst = done_inst & ~(1UL << intr_inst_index);
                        vdi_write_register(irqContext->core_idx, W5_RET_QUEUE_CMD_DONE_INST, done_inst);
                        if (0 == done_inst) {
                            reason &= ~(1<<INT_WAVE5_DEC_PIC);
                        }
                    }
                    if ((intr_reason == (1 << INT_WAVE5_INIT_SEQ)) || (intr_reason == (1 << INT_WAVE5_ENC_SET_PARAM)))
                    {
                        seq_inst = seq_inst & ~(1UL << intr_inst_index);
                        vdi_write_register(irqContext->core_idx, W5_RET_SEQ_DONE_INSTANCE_INFO, seq_inst);
                        if (0 == seq_inst) {
                            reason &= ~(1<<INT_WAVE5_INIT_SEQ | 1<<INT_WAVE5_ENC_SET_PARAM);
                        }
                    }

                    if (intr_reason == (1 << INT_WAVE5_ENC_PIC)) {
                        unsigned int ll_intr_reason = (1 << INT_WAVE5_ENC_PIC);
                        if (Queue_Enqueue(irqContext->intrQ[intr_inst_index], &ll_intr_reason) == FALSE) {
                            VLOG(ERR, "[VDI] :  Queue_Enqueue error count=%d \n", Queue_Get_Cnt(irqContext->intrQ[intr_inst_index]));
                        }
                    }
                    else {
                        if (Queue_Enqueue(irqContext->intrQ[intr_inst_index], &intr_reason) == FALSE) {
                            VLOG(ERR, "[VDI] :  Queue_Enqueue error count=%d \n", Queue_Get_Cnt(irqContext->intrQ[intr_inst_index]));
                        }
                    }
                }
                else {
                    VLOG(ERR, "[VDI] :  intr_inst_index is wrong intr_inst_index=%d \n", intr_inst_index);
                }
            }
            vdi_write_register(irqContext->core_idx, W5_VPU_VINT_REASON_CLR, reason_clr);
            vdi_write_register(irqContext->core_idx, W5_VPU_VINT_CLEAR, 1);
            if (0 != reason) {
                VLOG(ERR, "INTERRUPT REASON REMAINED: %08x\n", reason);
            }
        } else {
            return -1;
        }
    }
    else if (PRODUCT_CODE_CODA_SERIES(irqContext->product_code)) {
        if (vdi_read_register(irqContext->core_idx, BIT_INT_STS) > 0) {

            intr_reason = vdi_read_register(irqContext->core_idx, BIT_INT_REASON);
            if (Queue_Enqueue(irqContext->intrQ[0], &intr_reason) == FALSE) {
                VLOG(ERR, "[VDI] :  Queue_Enqueue error count=%d \n", Queue_Get_Cnt(irqContext->intrQ[0]));
            }
            vdi_write_register(irqContext->core_idx, BIT_INT_CLEAR, 0x1);
        } else {
            return -1;
        }
    } else {
        VLOG(ERR, "[VDI] Unknown product id : %08x\n", irqContext->product_code);
    }
    //VLOG(INFO, "[VPUDRV] product: 0x%08x\n", irqContext->product_code);
    //VLOG(INFO, "[VDI][-]%s\n", __func__);
    return 1;
}

#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
void polling_interrupt_work(void *context)
{
    IrqContext *irqContext = (IrqContext *)context;
    VLOG(INFO, "start polling_interrupt_work\n");

    while(irqContext->thread_run == 1) {
        pthread_mutex_lock(&s_irqMutex);
        vpu_irq_handler((void *)irqContext);
        pthread_mutex_unlock(&s_irqMutex);
        osal_msleep(0);
    }

    VLOG(INFO, "exit polling_interrupt_work\n");
}
#endif
int request_irq(unsigned long coreIdx)
{
#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
    int ret, i;
#else
    int i;
#endif

    IrqContext *irqContext;

    if ( coreIdx >= MAX_NUM_VPU_CORE)
        return 0;

    irqContext = &s_irqContext[coreIdx];
    if (!irqContext)
        return 0;

    if (irqContext->thread_run == 1)
        return 1;
    for ( i = 0 ; i < MAX_NUM_INSTANCE ; i++) {
        irqContext->intrQ[i] = Queue_Create_With_Lock(512, sizeof(unsigned int));
    }
    irqContext->core_idx = coreIdx;
    irqContext->thread_run = 1;

    irqContext->product_code = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    if (!PRODUCT_CODE_W_SERIES(irqContext->product_code) && ! PRODUCT_CODE_CODA_SERIES(irqContext->product_code)) {
        VLOG(ERR, "[create_irq_poll_thread] Unknown product id : %08x\n", irqContext->product_code);
        return 0;
    }

#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
    ret = pthread_create(&irqContext->thread_id, NULL, (void *)polling_interrupt_work, irqContext);//lint !e611

    if (ret != 0) {
        free_irq(coreIdx);
        return 0;
    }
#endif

    return 1;
}

void free_irq(unsigned long coreIdx)
{
    IrqContext *irqContext;
    int i;

    if ( coreIdx >= MAX_NUM_VPU_CORE)
        return ;

    irqContext = &s_irqContext[coreIdx];

    if (irqContext->thread_run == 1) {
        irqContext->thread_run = 0;

#ifdef SUPPORT_MULTI_INST_INTR_WITH_THREAD
        if (irqContext->thread_id) {
            pthread_join(irqContext->thread_id, NULL);
            irqContext->thread_id = 0;
        }
#endif

        for ( i = 0 ; i < MAX_NUM_INSTANCE ; i++) {
            if (irqContext->intrQ[i]) {
                Queue_Destroy(irqContext->intrQ[i]);
                irqContext->intrQ[i] = NULL;
            }
        }
        osal_memset(irqContext, 0x00, sizeof(IrqContext));
    }

    return ;
}


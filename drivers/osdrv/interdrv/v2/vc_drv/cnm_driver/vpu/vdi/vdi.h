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

#ifndef _VDI_H_
#define _VDI_H_

#include "vdi_debug.h"
#include "vpuconfig.h"
#include "vputypes.h"
#if defined(linux) || defined(__linux)
#include "linux/driver/vpu.h"
#endif

/************************************************************************/
/* COMMON REGISTERS                                                     */
/************************************************************************/
#define VPU_PRODUCT_NAME_REGISTER                 (0x1040)
#define VPU_PRODUCT_CODE_REGISTER                 (0x1044)

// #define SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define MAX_VPU_CORE_NUM MAX_NUM_VPU_CORE

#define MAX_VPU_BUFFER_POOL (MAX_NUM_INSTANCE*100)

#define VpuWriteReg( CORE, ADDR, DATA )                 vdi_write_register( CORE, ADDR, DATA )					// system register write
#define VpuReadReg( CORE, ADDR )                        vdi_read_register( CORE, ADDR )							// system register read
#define VpuWriteMem( CORE, ADDR, DATA, LEN, ENDIAN )    vdi_write_memory( CORE, ADDR, DATA, LEN, ENDIAN )		// system memory write
#define VpuReadMem( CORE, ADDR, DATA, LEN, ENDIAN )     vdi_read_memory( CORE, ADDR, DATA, LEN, ENDIAN )		// system memory read

typedef struct vpu_buffer_t {
    Uint32 size;
    PhysicalAddress phys_addr;
    unsigned long   base;
    unsigned long   virt_addr;
} vpu_buffer_t;

typedef enum {
    VDI_LITTLE_ENDIAN = 0,      /* 64bit LE */
    VDI_BIG_ENDIAN,             /* 64bit BE */
    VDI_32BIT_LITTLE_ENDIAN,
    VDI_32BIT_BIG_ENDIAN,
    /* WAVE PRODUCTS */
    VDI_128BIT_LITTLE_ENDIAN    = 16,
    VDI_128BIT_LE_BYTE_SWAP,
    VDI_128BIT_LE_WORD_SWAP,
    VDI_128BIT_LE_WORD_BYTE_SWAP,
    VDI_128BIT_LE_DWORD_SWAP,
    VDI_128BIT_LE_DWORD_BYTE_SWAP,
    VDI_128BIT_LE_DWORD_WORD_SWAP,
    VDI_128BIT_LE_DWORD_WORD_BYTE_SWAP,
    VDI_128BIT_BE_DWORD_WORD_BYTE_SWAP,
    VDI_128BIT_BE_DWORD_WORD_SWAP,
    VDI_128BIT_BE_DWORD_BYTE_SWAP,
    VDI_128BIT_BE_DWORD_SWAP,
    VDI_128BIT_BE_WORD_BYTE_SWAP,
    VDI_128BIT_BE_WORD_SWAP,
    VDI_128BIT_BE_BYTE_SWAP,
    VDI_128BIT_BIG_ENDIAN        = 31,
    VDI_ENDIAN_MAX
} EndianMode;

#define VDI_128BIT_ENDIAN_MASK 0xf
typedef enum {
    DEC_TASK      = 0,
    DEC_WORK      = 1,
    DEC_FBC       = 2,
    DEC_FBCY_TBL  = 3,
    DEC_FBCC_TBL  = 4,
    DEC_BS        = 5,
    DEC_FB_LINEAR = 6,
    DEC_MV        = 7,
    DEC_DEF_CDF   = 8,
    DEC_ETC       = 9,
    DEC_COMMON    = 10,
    DEC_TEMP      = 12,
    DEC_SEG_MAP   = 13,
    ENC_TASK      = 50,
    ENC_WORK      = 51,
    ENC_FBC       = 52,
    ENC_FBCY_TBL  = 53,
    ENC_FBCC_TBL  = 54,
    ENC_BS        = 55,
    ENC_SRC       = 56,
    ENC_MV        = 57,
    ENC_DEF_CDF   = 58,
    ENC_SUBSAMBUF = 59,
    ENC_ETC       = 60,
    ENC_TEMP      = 61,
    ENC_AR        = 62, //Adative round
    MEM_TYPE_MAX
} MemTypes;

typedef struct vpu_instance_pool_t {
    unsigned char   codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];  // Since VDI don't know the size of CodecInst structure, VDI should have the enough space not to overflow.
    vpu_buffer_t    vpu_common_buffer;
    int             vpu_instance_num;
    int             instance_pool_inited;
    void*           pendingInst;
    int             pendingInstIdxPlus1;
    Uint32          lastPerformanceCycles;
} vpu_instance_pool_t;

#define MAX_VPU_ION_BUFFER_NAME	(32)
#define AR_DEFAULT_EXTRA_LINE   512

#define DEBUG_ALLOCATED_MEM_INIT_VAL 0xAAAAAAAA

#if defined (__cplusplus)
extern "C" {
#endif
extern int vdi_probe(unsigned long core_idx);
extern int vdi_init(unsigned long core_idx);
extern int vdi_release(unsigned long core_idx);
extern vpu_instance_pool_t * vdi_get_instance_pool(unsigned long core_idx);
extern int vdi_allocate_common_memory(unsigned long core_idx);
extern int vdi_get_common_memory(unsigned long core_idx, vpu_buffer_t *vb);
extern int vdi_allocate_dma_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex);
extern int vdi_attach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb);
extern void vdi_free_dma_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex);
extern int vdi_get_sram_memory(unsigned long core_idx, vpu_buffer_t *vb);
extern int vdi_dettach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb);
extern int vdi_insert_extern_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex);
extern void vdi_remove_extern_memory(unsigned long core_idx, vpu_buffer_t *vb, int memTypes, int instIndex);

extern int vdi_wait_interrupt(unsigned long coreIdx, unsigned int instIdx, int timeout);
extern int vdi_wait_vpu_busy(unsigned long core_idx, int timeout, unsigned int addr_bit_busy_flag);
extern int vdi_wait_vcpu_bus_busy(unsigned long core_idx, int timeout, unsigned int gdi_busy_flag);
extern int vdi_wait_bus_busy(unsigned long core_idx, int timeout, unsigned int gdi_busy_flag);
extern int vdi_hw_reset(unsigned long core_idx);
extern int vdi_vpu_reset(unsigned long core_idx);
extern int vdi_set_clock_gate(unsigned long core_idx, int enable);
extern int vdi_get_clock_gate(unsigned long core_idx);
    /**
     * @brief       make clock stable before changing clock frequency
     * @detail      Before inoking vdi_set_clock_freg() caller MUST invoke vdi_ready_change_clock() function.
     *              after changing clock frequency caller also invoke vdi_done_change_clock() function.
     * @return  0   failure
     *          1   success
     */
extern int vdi_ready_change_clock(unsigned long core_idx);
extern int vdi_set_change_clock(unsigned long core_idx, unsigned long clock_mask);
extern int vdi_done_change_clock(unsigned long core_idx);
extern int  vdi_get_instance_num(unsigned long core_idx);
extern void vdi_write_register(unsigned long core_idx, unsigned int addr, unsigned int data);
extern unsigned int vdi_read_register(unsigned long core_idx, unsigned int addr);
extern void vdi_fio_write_register(unsigned long core_idx, unsigned int addr, unsigned int data);
extern unsigned int vdi_fio_read_register(unsigned long core_idx, unsigned int addr);
extern int vdi_clear_memory(unsigned long core_idx, PhysicalAddress addr, int len, int endian);
extern int vdi_set_memory(unsigned long core_idx, PhysicalAddress addr, int len, int endian, Uint32 data);
extern int vdi_write_memory(unsigned long core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian);
extern int vdi_read_memory(unsigned long core_idx, PhysicalAddress addr, unsigned char *data, int len, int endian);

extern int vdi_lock(unsigned long core_idx);
extern void vdi_unlock(unsigned long core_idx);
extern int vdi_disp_lock(unsigned long core_idx);
extern void vdi_disp_unlock(unsigned long core_idx);
extern int vdi_open_instance(unsigned long core_idx, unsigned long inst_idx, int support_cq);
extern int vdi_close_instance(unsigned long core_idx, unsigned long inst_idx);
extern int vdi_set_bit_firmware_to_pm(unsigned long core_idx, const unsigned short *code);
extern int vdi_get_system_endian(unsigned long core_idx);
extern int vdi_convert_endian(unsigned long core_idx, unsigned int endian);


#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
extern int vdi_get_task_num(unsigned long core_idx);
#endif
extern int vdi_set_ddr_map(unsigned long core_idx, unsigned int ext_addr);
extern int vdi_get_ddr_map(unsigned long core_idx);
extern int vdi_get_instance_count(unsigned long core_idx);
extern int vdi_request_instance(unsigned long core_idx);
extern int vdi_release_instance(unsigned long core_idx);
extern int vdi_invalidate_ion_cache(uint64_t u64PhyAddr, void *pVirAddr,
                 uint32_t u32Len);
extern int vdi_flush_ion_cache(uint64_t u64PhyAddr, void *pVirAddr, uint32_t u32Len);
#if defined (__cplusplus)
}
#endif

#endif //#ifndef _VDI_H_


/****************************************************************************
*
*    Copyright (c) 2005 - 2024 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/

#ifndef __gc_hal_kernel_device_h_
#define __gc_hal_kernel_device_h_

#include "gc_hal_ta.h"
#include "ioLib.h"

typedef struct _gcsDEVICE_CONSTRUCT_ARGS
{
    gctBOOL             recovery;
    gctUINT             stuckDump;
    gctUINT             gpu3DMinClock;

    gctBOOL             contiguousRequested;
    gcsPLATFORM*        platform;
    gctBOOL             mmu;
    gctBOOL             registerMemMapped;
    gctPOINTER          registerMemAddress;
#if gcdDEC_ENABLE_AHB
    gctUINT32           registerMemBaseDEC300;
    gctSIZE_T           registerMemSizeDEC300;
#endif
    gctINT              irqs[gcvCORE_COUNT];
    gctUINT             registerBases[gcvCORE_COUNT];
    gctUINT             registerSizes[gcvCORE_COUNT];
    gctBOOL             powerManagement;
    gctBOOL             gpuProfiler;
    gctUINT             chipIDs[gcvCORE_COUNT];
}
gcsDEVICE_CONSTRUCT_ARGS;

/******************************************************************************\
************************** gckGALDEVICE Structure ******************************
\******************************************************************************/

typedef struct _gckGALDEVICE
{
    /* Objects. */
    gckOS               os;
    gckKERNEL           kernels[gcdMAX_GPU_COUNT];

    gcsPLATFORM*        platform;

    /* Attributes. */
    gctPHYS_ADDR_T      internalBase;
    gctSIZE_T           internalSize;
    gctPHYS_ADDR        internalPhysical;
    gctUINT32           internalPhysName;
    gctPOINTER          internalLogical;
    gckVIDMEM           internalVidMem;

    gctPHYS_ADDR_T      externalBase;
    gctSIZE_T           externalSize;
    gctPHYS_ADDR        externalPhysical;
    gctUINT32           externalPhysName;
    gctPOINTER          externalLogical;
    gckVIDMEM           externalVidMem;

    gctPHYS_ADDR_T      contiguousBase;
    gctSIZE_T           contiguousSize;

    gckVIDMEM           contiguousVidMem;
    gctPOINTER          contiguousLogical;
    gctPHYS_ADDR        contiguousPhysical;
    gctUINT32           contiguousPhysName;

    gctSIZE_T           systemMemorySize;
    gctUINT32           systemMemoryBaseAddress;
    gctPOINTER          registerBases[gcdMAX_GPU_COUNT];
    gctSIZE_T           registerSizes[gcdMAX_GPU_COUNT];

    gctUINT32           baseAddress;
    gctUINT32           physBase;
    gctUINT32           physSize;

    /* By request_mem_region. */
    gctUINT32           requestedRegisterMemBases[gcdMAX_GPU_COUNT];
    gctSIZE_T           requestedRegisterMemSizes[gcdMAX_GPU_COUNT];

    /* By request_mem_region. */
    gctUINT32           requestedContiguousBase;
    gctSIZE_T           requestedContiguousSize;

    /* IRQ management. */
    gctINT              irqLines[gcdMAX_GPU_COUNT];
    gctBOOL             isrInitializeds[gcdMAX_GPU_COUNT];

    /* Thread management. */
    gctINT              threadCtxts[gcdMAX_GPU_COUNT];
    SEM_ID              semas[gcdMAX_GPU_COUNT];
    gctBOOL             threadInitializeds[gcdMAX_GPU_COUNT];
    gctBOOL             killThread;

    /* Signal management. */
    gctINT              signal;

    /* States before suspend. */
    gceCHIPPOWERSTATE   statesStored[gcdMAX_GPU_COUNT];

    gckDEVICE           device;

    gcsDEVICE_CONSTRUCT_ARGS args;

    /* gctsOs object for trust application. */
    gctaOS              taos;

#if gcdENABLE_DRM
    void*               drm;
#endif
}
* gckGALDEVICE;

typedef struct _gcsHAL_PRIVATE_DATA
{
    DEV_HDR             pDevHdr;
    gckGALDEVICE        device;
    /*
     * 'fput' schedules actual work in '__fput' in a different thread.
     * So the process opens the device may not be the same as the one that
     * closes it.
     */
    gctUINT32           pidOpen;
} GPU_DEV;

gceSTATUS gckGALDEVICE_Setup_ISR(
    IN gceCORE Core
    );

gceSTATUS gckGALDEVICE_Setup_ISR_VG(
    IN gckGALDEVICE Device
    );

gceSTATUS gckGALDEVICE_Release_ISR(
    IN gceCORE Core
    );

gceSTATUS gckGALDEVICE_Release_ISR_VG(
    IN gckGALDEVICE Device
    );

gceSTATUS gckGALDEVICE_Start_Threads(
    IN gckGALDEVICE Device
    );

gceSTATUS gckGALDEVICE_Stop_Threads(
    gckGALDEVICE Device
    );

gceSTATUS gckGALDEVICE_Start(
    IN gckGALDEVICE Device
    );

gceSTATUS gckGALDEVICE_Stop(
    gckGALDEVICE Device
    );

gceSTATUS gckGALDEVICE_Construct(
    IN gctINT IrqLine,
    IN gctUINT32 RegisterMemBase,
    IN gctSIZE_T RegisterMemSize,
    IN gctINT IrqLine2D,
    IN gctUINT32 RegisterMemBase2D,
    IN gctSIZE_T RegisterMemSize2D,
    IN gctINT IrqLineVG,
    IN gctUINT32 RegisterMemBaseVG,
    IN gctSIZE_T RegisterMemSizeVG,
    IN gctUINT32 ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctUINT32 ExternalBase,
    IN gctSIZE_T ExternalSize,
    IN gctSIZE_T BankSize,
    IN gctINT FastClear,
    IN gctINT Compression,
    IN gctUINT32 PhysBaseAddr,
    IN gctUINT32 PhysSize,
    IN gctINT Signal,
    IN gctUINT LogFileSize,
    IN gctINT PowerManagement,
    IN gctINT GpuProfiler,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args,
    OUT gckGALDEVICE *Device
    );

gceSTATUS gckGALDEVICE_Destroy(
    IN gckGALDEVICE Device
    );

static gcmINLINE gckKERNEL
_GetValidKernel(
    gckGALDEVICE Device
    )
{
    if (Device->kernels[gcvCORE_MAJOR])
    {
        return Device->kernels[gcvCORE_MAJOR];
    }
    else
    if (Device->kernels[gcvCORE_2D])
    {
        return Device->kernels[gcvCORE_2D];
    }
    else
    if (Device->kernels[gcvCORE_VG])
    {
        return Device->kernels[gcvCORE_VG];
    }
    else
    {
        gcmkASSERT(gcvFALSE);
        return gcvNULL;
    }
}

#endif /* __gc_hal_kernel_device_h_ */

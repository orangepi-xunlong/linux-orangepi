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

#ifndef _gc_hal_kernel_platform_h_
#define _gc_hal_kernel_platform_h_

typedef struct _gcsMODULE_PARAMETERS
{
    gctINT  irqLine;
    gctUINT registerMemBase;
    gctUINT registerMemSize;
    gctINT  irqLine2D;
    gctUINT registerMemBase2D;
    gctUINT registerMemSize2D;
    gctINT  irqLineVG;
    gctUINT registerMemBaseVG;
    gctUINT registerMemSizeVG;
    gctUINT contiguousSize;
    gctUINT contiguousBase;
    gctUINT contiguousRequested;
    gctUINT externalSize;
    gctUINT externalBase;
    gctUINT bankSize;
    gctINT  fastClear;
    gceCOMPRESSION_OPTION compression;
    gctINT  powerManagement;
    gctINT  gpuProfiler;
    gctINT  gSignal;
    gctUINT baseAddress;
    gctUINT physSize;
    gctUINT logFileSize;
    gctUINT recovery;
    gctUINT stuckDump;
    gctUINT showArgs;
    gctUINT gpu3DMinClock;
    gctBOOL registerMemMapped;
    gctPOINTER registerMemAddress;
    gctINT  irqs[gcvCORE_COUNT];
    gctUINT registerBases[gcvCORE_COUNT];
    gctUINT registerSizes[gcvCORE_COUNT];
    gctUINT chipIDs[gcvCORE_COUNT];
}
gcsMODULE_PARAMETERS;

typedef struct soc_platform gcsPLATFORM;

typedef struct soc_platform_ops
{

    /*******************************************************************************
    **
    **  adjustParam
    **
    **  Override content of arguments, if a argument is not changed here, it will
    **  keep as default value or value set by insmod command line.
    */
    gceSTATUS
    (*adjustParam)(
        IN gcsPLATFORM * Platform,
        OUT gcsMODULE_PARAMETERS *Args
        );

    /*******************************************************************************
    **
    **  getPower
    **
    **  Prepare power and clock operation.
    */
    gceSTATUS
    (*getPower)(
        IN gcsPLATFORM * Platform
        );

    /*******************************************************************************
    **
    **  putPower
    **
    **  Finish power and clock operation.
    */
    gceSTATUS
    (*putPower)(
        IN gcsPLATFORM * Platform
        );

    /*******************************************************************************
    **
    **  setPower
    **
    **  Set power state of specified GPU.
    **
    **  INPUT:
    **
    **      gceCORE GPU
    **          GPU neeed to config.
    **
    **      gceBOOL Enable
    **          Enable or disable power.
    */
    gceSTATUS
    (*setPower)(
        IN gcsPLATFORM * Platform,
        IN gceCORE GPU,
        IN gctBOOL Enable
        );

    /*******************************************************************************
    **
    **  setClock
    **
    **  Set clock state of specified GPU.
    **
    **  INPUT:
    **
    **      gceCORE GPU
    **          GPU neeed to config.
    **
    **      gceBOOL Enable
    **          Enable or disable clock.
    */
    gceSTATUS
    (*setClock)(
        IN gcsPLATFORM * Platform,
        IN gceCORE GPU,
        IN gctBOOL Enable
        );

    /*******************************************************************************
    **
    **  reset
    **
    **  Reset GPU outside.
    **
    **  INPUT:
    **
    **      gceCORE GPU
    **          GPU neeed to reset.
    */
    gceSTATUS
    (*reset)(
        IN gcsPLATFORM * Platform,
        IN gceCORE GPU
        );

    /*******************************************************************************
    **
    **  getGPUPhysical
    **
    **  Convert CPU physical address to GPU physical address if they are
    **  different.
    */
    gceSTATUS
    (*getGPUPhysical)(
        IN gcsPLATFORM * Platform,
        IN gctPHYS_ADDR_T CPUPhysical,
        OUT gctPHYS_ADDR_T * GPUPhysical
        );

    /*******************************************************************************
    **
    **  getCPUPhysical
    **
    **  Convert GPU physical address to CPU physical address if they are
    **  different.
    */
    gceSTATUS
    (*getCPUPhysical)(
        IN gcsPLATFORM * Platform,
        IN gctPHYS_ADDR_T GPUPhysical,
        OUT gctPHYS_ADDR_T * CPUPhysical
        );

    /*******************************************************************************
    **
    **  shrinkMemory
    **
    **  Do something to collect memory, eg, act as oom killer.
    */
    gceSTATUS
    (*shrinkMemory)(
        IN gcsPLATFORM * Platform
        );

    /*******************************************************************************
    **
    ** getPolicyID
    **
    ** Get policyID for a specified surface type.
    */
    gceSTATUS
    (*getPolicyID)(
        IN gcsPLATFORM * Platform,
        IN gceSURF_TYPE Type,
        OUT gctUINT32_PTR PolicyID,
        OUT gctUINT32_PTR AXIConfig
        );
}
gcsPLATFORM_OPERATIONS;

struct soc_platform
{
    const char *name;
    gcsPLATFORM_OPERATIONS* ops;
};

int soc_platform_init(gcsPLATFORM **platform);
int soc_platform_terminate(gcsPLATFORM *platform);

#endif

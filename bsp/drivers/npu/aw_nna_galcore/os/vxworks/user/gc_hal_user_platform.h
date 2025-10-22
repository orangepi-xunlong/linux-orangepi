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

#ifndef _gc_hal_user_platform_h_
#define _gc_hal_user_platform_h_

typedef struct _gcsPLATFORM * gcoPLATFORM;

typedef struct _gcsPLATFORM_OPERATIONS
{
    /*******************************************************************************
    **
    **  getGPUPhysical
    **
    **  Convert CPU physical address to GPU physical address if they are
    **  different.
    */
    gceSTATUS
    (*getGPUPhysical)(
        IN gcoPLATFORM Platform,
        IN gctPHYS_ADDR_T CPUPhysical,
        OUT gctPHYS_ADDR_T * GPUPhysical
        );

    /*******************************************************************************
    **
    **  getGPUPhysical
    **
    **  Convert GPU physical address to CPU physical address if they are
    **  different.
    */
    gceSTATUS
    (*getCPUPhysical)(
        IN gcoPLATFORM Platform,
        IN gctUINT32 GPUPhysical,
        OUT gctPHYS_ADDR_T * CPUPhysical
        );
}
gcsPLATFORM_OPERATIONS;

typedef struct _gcsPLATFORM
{
    gcsPLATFORM_OPERATIONS* ops;
}
gcsPLATFORM;

void
gcoPLATFORM_QueryOperations(
    IN gcsPLATFORM_OPERATIONS ** Operations
    );
#endif

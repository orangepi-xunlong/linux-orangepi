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

#include "gc_hal_kernel_vxworks.h"
#include "gc_hal_kernel_platform.h"

gceSTATUS
_AdjustParam(
    IN gcsPLATFORM *Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
    Args->contiguousSize = 0x5000000;
    Args->powerManagement = 1;
    Args->physSize = 0x80000000;
    Args->irqLine = 38;
    Args->registerMemMapped = gcvTRUE;
    Args->registerMemBase = 0xbfe40000;
    Args->registerMemAddress = 0xbfe40000;

    return gcvSTATUS_OK;
}

static struct soc_platform_ops default_ops =
{
    .adjustParam   = _AdjustParam,
};

static struct soc_platform default_platform =
{
    .name = __FILE__,
    .ops  = &default_ops,
};

int soc_platform_init(struct soc_platform **platform)
{
    *platform = &default_platform;
    return 0;
}

int soc_platform_terminate(struct soc_platform *platform)
{
    return 0;
}


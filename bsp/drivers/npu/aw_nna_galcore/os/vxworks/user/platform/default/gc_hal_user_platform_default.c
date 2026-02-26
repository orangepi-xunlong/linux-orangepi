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

#include "gc_hal_user_vxworks.h"
#include "gc_hal_user_platform.h"

gcsPLATFORM_OPERATIONS platformOperations =
{
};

void
gcoPLATFORM_QueryOperations(
    IN gcsPLATFORM_OPERATIONS ** Operations
    )
{
     *Operations = &platformOperations;
}


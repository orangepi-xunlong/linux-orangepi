// SPDX-License-Identifier:  LGPL-2.1 OR BSD-3-Clause
/*
 * Copyright (c) 2022, Chips&Media
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "vpuapifunc.h"
#include "main_helper.h"

#define MAX_FEEDING_SIZE        0x400000        /* 4MBytes */
#define DEFAULT_FEEDING_SIZE    0x20000         /* 128KBytes */

typedef struct FeederFixedContext {
    void*           address;
    Uint32          size;
    Uint32          offset;
    Uint32          feedingSize;
    BOOL            eos;
} FeederFixedContext;

void* BSFeederBuffer_Create(
    const char* path,
    CodStd      codecId
    )
{
    FeederFixedContext*  context=NULL;

    UNREFERENCED_PARAMETER(codecId);

    context = (FeederFixedContext*)osal_malloc(sizeof(FeederFixedContext));
    if (context == NULL) {
        VLOG(ERR, "%s:%d failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    context->feedingSize = DEFAULT_FEEDING_SIZE;
    context->eos         = FALSE;
    context->offset = 0;
    return (void*)context;
}

BOOL BSFeederBuffer_Destroy(
    void* feeder
    )
{
    FeederFixedContext* context = (FeederFixedContext*)feeder;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    osal_free(context);

    return TRUE;
}

void BSFeederBuffer_SetData(
    void*       feeder,
    char*       address,
    Uint32       size)
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    context->address = address;
    context->size = size;
}

void BSFeederBuffer_SetEos(void*       feeder)
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    context->eos = TRUE;
}

BOOL BSFeederBuffer_GetEos(void*       feeder)
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    return context->eos;
}

Int32 BSFeederBuffer_Act(
    void*       feeder,
    BSChunk*    chunk
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return 0;
    }
    // Due to memory performance, memset is temporarily commented
    // osal_memset(chunk->data, 0x00, chunk->size);
    if (context->size == 0) {
        chunk->eos = TRUE;
        return 0;
    }

    do {
        osal_memcpy(chunk->data, context->address, context->size);
    } while (FALSE);

    return context->size;
}

BOOL BSFeederBuffer_Rewind(
    void*       feeder
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;

    context->eos = FALSE;

    return TRUE;
}

void BSFeederBuffer_SetFeedingSize(
    void*   feeder,
    Uint32  feedingSize
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    if (feedingSize > 0) {
        context->feedingSize = feedingSize;
    }
}


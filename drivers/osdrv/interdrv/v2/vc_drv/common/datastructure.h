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
#ifndef __DATA_STRUCTURE_H__
#define __DATA_STRUCTURE_H__

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/************************************************************************/
/* Queue                                                                */
/************************************************************************/
typedef struct {
    void*   data;
} QueueData;

typedef struct {
    uint8_t*    buffer;
    uint32_t      size;
    uint32_t      itemSize;
    uint32_t      count;
    uint32_t      front;
    uint32_t      rear;
    struct mutex* lock;
} Queue;

Queue* Queue_Create(
    uint32_t    itemCount,
    uint32_t    itemSize
    );

Queue* Queue_Create_With_Lock(
    uint32_t    itemCount,
    uint32_t    itemSize
    );

void Queue_Destroy(
    Queue*      queue
    );

/**
 * \brief       Enqueue with deep copy
 */
int Queue_Enqueue(
    Queue*      queue,
    void*       data
    );

/**
 * \brief       Caller has responsibility for releasing the returned data
 */
void* Queue_Dequeue(
    Queue*      queue
    );

void Queue_Flush(
    Queue*      queue
    );

void* Queue_Peek(
    Queue*      queue
    );

uint32_t Queue_Get_Cnt(
    Queue*      queue
    );

/**
 * \brief       @dstQ is NULL, it allocates Queue structure and then copy from @srcQ.
 */
Queue* Queue_Copy(
    Queue*  dstQ,
    Queue*  srcQ
    );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DATA_STRUCTURE_H__ */



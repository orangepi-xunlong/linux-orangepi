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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "datastructure.h"

Queue* Queue_Create(
    uint32_t      itemCount,
    uint32_t      itemSize
    )
{
    Queue* queue = NULL;
    uint32_t size  = itemCount*itemSize;

    if ((queue=(Queue *)vmalloc(sizeof(Queue))) == NULL)
        return NULL;

    queue->size   = itemCount;
    queue->itemSize = itemSize;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (uint8_t*)vmalloc(size);
    queue->lock = NULL;

    return queue;
}

Queue* Queue_Create_With_Lock(
    uint32_t      itemCount,
    uint32_t      itemSize
    )
{
    Queue* queue = NULL;
    uint32_t size  = itemCount*itemSize;

    if ((queue=(Queue *)vmalloc(sizeof(Queue))) == NULL)
        return NULL;

    queue->size   = itemCount;
    queue->itemSize = itemSize;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (uint8_t*)vmalloc(size);
    queue->lock = vmalloc(sizeof(struct mutex));
    mutex_init(queue->lock);

    return queue;
}

void Queue_Destroy(
    Queue*      queue
    )
{
    if (queue == NULL)
        return;

    if (queue->buffer)
        vfree(queue->buffer);

    if (queue->lock) {
        mutex_destroy(queue->lock);
        vfree(queue->lock);
    }

    vfree(queue);
}

int Queue_Enqueue(
    Queue*      queue,
    void*       data
    )
{
    uint8_t*      ptr;
    uint32_t      offset;

    if (queue == NULL) return 0;

    /* Queue is full */
    if (queue->count == queue->size)
        return -1;

    if (queue->lock)
        mutex_lock(queue->lock);

    offset = queue->rear * queue->itemSize;
    ptr = &queue->buffer[offset];
    memcpy(ptr, data, queue->itemSize);
    queue->rear++;
    queue->rear %= queue->size;
    queue->count++;

    if (queue->lock)
        mutex_unlock(queue->lock);

    return 1;
}

void* Queue_Dequeue(
    Queue*      queue
    )
{
    void* data;
    uint32_t offset;

    if (queue == NULL)
        return NULL;

    /* Queue is empty */
    if (queue->count == 0)
        return NULL;

    if (queue->lock)
        mutex_lock(queue->lock);

    offset = queue->front * queue->itemSize;
    data   = (void*)&queue->buffer[offset];
    queue->front++;
    queue->front %= queue->size;
    queue->count--;

    if (queue->lock)
        mutex_unlock(queue->lock);

    return data;
}

void Queue_Flush(
    Queue*      queue
    )
{
    if (queue == NULL)
        return;

    if (queue->lock)
        mutex_lock(queue->lock);

    queue->count = 0;
    queue->front = 0;
    queue->rear  = 0;

    if (queue->lock)
        mutex_unlock(queue->lock);

    return;
}

void* Queue_Peek(
    Queue*      queue
    )
{
    uint32_t      offset;
    void*       temp;

    if (queue == NULL)
        return NULL;

    /* Queue is empty */
    if (queue->count == 0)
        return NULL;

    if (queue->lock)
        mutex_lock(queue->lock);

    offset = queue->front * queue->itemSize;
    temp = (void*)&queue->buffer[offset];

    if (queue->lock)
        mutex_unlock(queue->lock);

    return  temp;
}

uint32_t   Queue_Get_Cnt(
    Queue*      queue
    )
{
    uint32_t      cnt;

    if (queue == NULL)
        return 0;

    if (queue->lock)
        mutex_lock(queue->lock);

    cnt = queue->count;

    if (queue->lock)
        mutex_unlock(queue->lock);

    return cnt;
}

Queue* Queue_Copy(
    Queue*  dstQ,
    Queue*  srcQ
    )
{
    Queue*   queue = NULL;
    uint32_t bufferSize;

    if (dstQ == NULL) {
        if ((queue=(Queue *)vmalloc(sizeof(Queue))) == NULL)
            return NULL;
        memset((void*)queue, 0x00, sizeof(Queue));
    }
    else {
        queue = dstQ;
    }

    bufferSize      = srcQ->size * srcQ->itemSize;
    queue->size     = srcQ->size;
    queue->itemSize = srcQ->itemSize;
    queue->count    = srcQ->count;
    queue->front    = srcQ->front;
    queue->rear     = srcQ->rear;

    if (queue->buffer) {
        vfree(queue->buffer);
    }

    queue->buffer   = (uint8_t*)vmalloc(bufferSize);

    memcpy(queue->buffer, srcQ->buffer, bufferSize);

    return queue;
}


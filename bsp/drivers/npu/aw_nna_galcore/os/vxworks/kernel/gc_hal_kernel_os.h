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

#ifndef __gc_hal_kernel_os_h_
#define __gc_hal_kernel_os_h_

/*
 * This is a simple doubly linked list implementation.
 */

struct list_head {
    struct list_head *next; /* next in chain */
    struct list_head *prev; /* previous in chain */
};


/* Initialise a static list */
#define LIST_HEAD(name) \
struct list_head name = { &(name), &(name)}



/* Initialise a list head to an empty list */
#define INIT_LIST_HEAD(p) \
do { \
    (p)->next = (p);\
    (p)->prev = (p); \
} while (0)

static gcmINLINE
void list_add(struct list_head *new_entry,
                struct list_head *list)
{
    struct list_head *list_next = list->next;

    list->next = new_entry;
    new_entry->prev = list;
    new_entry->next = list_next;
    list_next->prev = new_entry;
}

static gcmINLINE
void list_add_tail(struct list_head *new_entry,
                struct list_head *list)
{
    struct list_head *list_prev = list->prev;

    list->prev = new_entry;
    new_entry->next = list;
    new_entry->prev = list_prev;
    list_prev->next = new_entry;
}


static gcmINLINE
void list_del(struct list_head *entry)
{
    struct list_head *list_next = entry->next;
    struct list_head *list_prev = entry->prev;

    list_next->prev = list_prev;
    list_prev->next = list_next;
}

static gcmINLINE
void list_del_init(struct list_head *entry)
{
    list_del(entry);
    entry->next = entry->prev = entry;
}


static gcmINLINE
int list_empty(struct list_head *entry)
{
    return (entry->next == entry);
}

#define list_entry(entry, type, member) \
    ((type *)((char *)(entry)-(unsigned long)(&((type *)NULL)->member)))

#define list_for_each(itervar, list) \
    for (itervar = (list)->next; itervar != (list); itervar = itervar->next)

#define list_for_each_entry(pos, head, member)                \
    for (pos = list_entry((head)->next, typeof(*pos), member);    \
         &pos->member != (head);     \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_safe(itervar, save_var, list) \
    for (itervar = (list)->next, save_var = (list)->next->next; \
         itervar != (list); \
         itervar = save_var, save_var = save_var->next)

#define list_for_each_entry_safe(pos, n, head, member)            \
    for (pos = list_entry((head)->next, typeof(*pos), member),    \
         n = list_entry(pos->member.next, typeof(*pos), member);    \
         &pos->member != (head);                     \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))


typedef struct _gcsOSTIMER * gcsOSTIMER_PTR;
typedef struct _gcsOSTIMER
{
    gctUINT32     taskID;
    gctBOOL       isStart;
    gctTIMERFUNCTION        function;
    gctPOINTER              data;
    gctUINT32     delay;
} gcsOSTIMER;

typedef struct _VX_MDL     VX_MDL,     *PVX_MDL;
typedef struct _VX_MDL_MAP VX_MDL_MAP, *PVX_MDL_MAP;

struct _VX_MDL_MAP
{
    gctINT                  pid;

    /* map references. */
    gctUINT32               count;

    struct vm_area_struct * vma;
    gctPOINTER              vmaAddr;
    gctBOOL                 cacheable;

    struct list_head        link;
};

struct _VX_MDL
{
    gckOS                   os;

    atomic_t                refs;

    char *                  addr;

    size_t                  bytes;
    gctINT                  numPages;
    gctBOOL                 contiguous;
    gctBOOL                 cacheable;

    pthread_mutex_t         mapsMutex;
    struct list_head        mapsHead;

    /* Pointer to allocator which allocates memory for this mdl. */
    void *                  allocator;

    /* Private data used by allocator. */
    void *                  priv;

    gctUINT32               gid;

    struct list_head        link;
};

extern PVX_MDL_MAP
FindMdlMap(
    IN PVX_MDL Mdl,
    IN gctINT PID
    );

typedef struct _DRIVER_ARGS
{
    gctUINT64               InputBuffer;
    gctUINT64               InputBufferSize;
    gctUINT64               OutputBuffer;
    gctUINT64               OutputBufferSize;
}
DRIVER_ARGS;

#endif /* __gc_hal_kernel_os_h_ */

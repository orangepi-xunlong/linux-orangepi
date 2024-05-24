/* SPDX-License-Identifier:  LGPL-2.1 OR BSD-3-Clause */
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
 *
 * Copyright (C) 2022 StarFive Technology Co., Ltd.
*/

#ifndef __JPU_DRV_H__
#define __JPU_DRV_H__

#include <linux/fs.h>
#include <linux/types.h>

#define JDI_IOCTL_MAGIC  'J'

#define JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY          _IO(JDI_IOCTL_MAGIC, 0)
#define JDI_IOCTL_FREE_PHYSICALMEMORY               _IO(JDI_IOCTL_MAGIC, 1)
#define JDI_IOCTL_WAIT_INTERRUPT                    _IO(JDI_IOCTL_MAGIC, 2)
#define JDI_IOCTL_SET_CLOCK_GATE                    _IO(JDI_IOCTL_MAGIC, 3)
#define JDI_IOCTL_RESET                             _IO(JDI_IOCTL_MAGIC, 4)
#define JDI_IOCTL_GET_INSTANCE_POOL                 _IO(JDI_IOCTL_MAGIC, 5)
#define JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO    _IO(JDI_IOCTL_MAGIC, 6)
#define JDI_IOCTL_GET_REGISTER_INFO                 _IO(JDI_IOCTL_MAGIC, 7)
#define JDI_IOCTL_OPEN_INSTANCE                     _IO(JDI_IOCTL_MAGIC, 8)
#define JDI_IOCTL_CLOSE_INSTANCE                    _IO(JDI_IOCTL_MAGIC, 9)
#define JDI_IOCTL_GET_INSTANCE_NUM                  _IO(JDI_IOCTL_MAGIC, 10)
#define JDI_IOCTL_FLUSH_DCACHE                 		_IO(JDI_IOCTL_MAGIC, 11)
#define JDI_IOCTL_GET_PHYSICAL_MEMORY               _IO(JDI_IOCTL_MAGIC, 12)

#define MEM2SYS(addr) ((addr) > 0x40000000 && (addr) < 0x240000000 ? ((addr)+0x400000000):(addr))

#ifndef PTHREAD_MUTEX_ROBUST_NP
#define PTHREAD_MUTEX_ROBUST_NP     1
#endif

typedef struct jpudrv_flush_cache_t {
    unsigned long start;
    unsigned long size;
    unsigned char flag;
} jpudrv_flush_cache_t;

typedef struct jpudrv_buffer_t {
    unsigned int size;
    unsigned long phys_addr;
    unsigned long base;                     /* kernel logical address in use kernel */
    unsigned long virt_addr;                /* virtual user space address */
} jpudrv_buffer_t;

typedef struct jpudrv_inst_info_t {
    unsigned int inst_idx;
    int inst_open_count;    /* for output only*/
} jpudrv_inst_info_t;

typedef struct jpudrv_intr_info_t {
    unsigned int    timeout;
    int             intr_reason;
    unsigned int    inst_idx;
} jpudrv_intr_info_t;
#endif

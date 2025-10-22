/*************************************************************************/ /*!
@File
@Title          Kernel/User mode general purpose shared memory.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    General purpose shared memory (i.e. information page) mapped by
                kernel space driver and user space clients. All information page
                entries are sizeof(IMG_UINT32) on both 32/64-bit environments.
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef INFO_PAGE_DEFS_H
#define INFO_PAGE_DEFS_H

/* Info page is divided in "blocks" of size INFO_PAGE_CHUNK_SIZE. Each block
 * should start with the INFO_PAGE_[NAME]_BLOCK_START macro which takes the
 * value of previous block (except for the first block which starts from 0).
 *
 * Each last value of the block (INFO_PAGE_[NAME]_BLOCK_END) should be unused
 * within that block since it's a first value of the next block. This value
 * should be a multiple of INFO_PAGE_CHUNK_SIZE.
 *
 * Care must be taken to not go over allowed number of elements in each block
 * which is marked with the INFO_PAGE_[NAME]_BLOCK_END macro.
 *
 * Blocks consist of entries that are defined with the INFO_PAGE_ENTRY() macro.
 * Each entry must define a unique index within the block and as mentioned
 * can't go over the INFO_PAGE_[NAME]_BLOCK_END limit.
 *
 * Always add blocks to the end of the existing list and update
 * INFO_PAGE_TOTAL_SIZE after.
 *
 * See current usage of the Info Page below for examples.
 */

#define INFO_PAGE_CHUNK_SIZE                        8
#define INFO_PAGE_BLOCK_END(start,size)             ((start) + (size) * INFO_PAGE_CHUNK_SIZE)
#define INFO_PAGE_ENTRY(start,index)                ((start) + (index))
#define INFO_PAGE_SIZE_IN_BYTES(end)                ((end) * sizeof(IMG_UINT32))

#define INFO_PAGE_CACHEOP_BLOCK_START               0
#define INFO_PAGE_CACHEOP_BLOCK_END                 INFO_PAGE_BLOCK_END(INFO_PAGE_CACHEOP_BLOCK_START, 1)
#define INFO_PAGE_HWPERF_BLOCK_START                INFO_PAGE_CACHEOP_BLOCK_END
#define INFO_PAGE_HWPERF_BLOCK_END                  INFO_PAGE_BLOCK_END(INFO_PAGE_HWPERF_BLOCK_START, 1)
#define INFO_PAGE_TIMEOUT_BLOCK_START               INFO_PAGE_HWPERF_BLOCK_END
#define INFO_PAGE_TIMEOUT_BLOCK_END                 INFO_PAGE_BLOCK_END(INFO_PAGE_TIMEOUT_BLOCK_START, 2)
#define INFO_PAGE_BRIDGE_BLOCK_START                INFO_PAGE_TIMEOUT_BLOCK_END
#define INFO_PAGE_BRIDGE_BLOCK_END                  INFO_PAGE_BLOCK_END(INFO_PAGE_BRIDGE_BLOCK_START, 1)
#define INFO_PAGE_DEBUG_BLOCK_START                 INFO_PAGE_BRIDGE_BLOCK_END
#define INFO_PAGE_DEBUG_BLOCK_END                   INFO_PAGE_BLOCK_END(INFO_PAGE_DEBUG_BLOCK_START, 1)
#define INFO_PAGE_DEVMEM_BLOCK_START                INFO_PAGE_DEBUG_BLOCK_END
#define INFO_PAGE_DEVMEM_BLOCK_END                  INFO_PAGE_BLOCK_END(INFO_PAGE_DEVMEM_BLOCK_START, 1)

/* IMPORTANT: Make sure this always uses the last INFO_PAGE_[NAME]_BLOCK_END definition.*/
#define INFO_PAGE_TOTAL_SIZE                        INFO_PAGE_SIZE_IN_BYTES(INFO_PAGE_DEVMEM_BLOCK_END)

/* CacheOp information page entries */

#define CACHEOP_INFO_UMKMTHRESHLD                   INFO_PAGE_ENTRY(INFO_PAGE_CACHEOP_BLOCK_START, 0) /*!< UM=>KM routing threshold in bytes */
#define CACHEOP_INFO_KMDFTHRESHLD                   INFO_PAGE_ENTRY(INFO_PAGE_CACHEOP_BLOCK_START, 1) /*!< KM/DF threshold in bytes */
#define CACHEOP_INFO_LINESIZE                       INFO_PAGE_ENTRY(INFO_PAGE_CACHEOP_BLOCK_START, 2) /*!< CPU data cache line size */
#define CACHEOP_INFO_PGSIZE                         INFO_PAGE_ENTRY(INFO_PAGE_CACHEOP_BLOCK_START, 3) /*!< CPU MMU page size */

/* HWPerf information page entries */

#define HWPERF_FILTER_SERVICES_IDX                  INFO_PAGE_ENTRY(INFO_PAGE_HWPERF_BLOCK_START, 0)
#define HWPERF_FILTER_EGL_IDX                       INFO_PAGE_ENTRY(INFO_PAGE_HWPERF_BLOCK_START, 1)
#define HWPERF_FILTER_OPENGLES_IDX                  INFO_PAGE_ENTRY(INFO_PAGE_HWPERF_BLOCK_START, 2)
#define HWPERF_FILTER_OPENCL_IDX                    INFO_PAGE_ENTRY(INFO_PAGE_HWPERF_BLOCK_START, 3)
#define HWPERF_FILTER_VULKAN_IDX                    INFO_PAGE_ENTRY(INFO_PAGE_HWPERF_BLOCK_START, 4)
#define HWPERF_FILTER_OPENGL_IDX                    INFO_PAGE_ENTRY(INFO_PAGE_HWPERF_BLOCK_START, 5)

/* Timeout values */

#define TIMEOUT_INFO_VALUE_RETRIES                  INFO_PAGE_ENTRY(INFO_PAGE_TIMEOUT_BLOCK_START, 0)
#define TIMEOUT_INFO_VALUE_TIMEOUT_MS               INFO_PAGE_ENTRY(INFO_PAGE_TIMEOUT_BLOCK_START, 1)
#define TIMEOUT_INFO_CONDITION_RETRIES              INFO_PAGE_ENTRY(INFO_PAGE_TIMEOUT_BLOCK_START, 2)
#define TIMEOUT_INFO_CONDITION_TIMEOUT_MS           INFO_PAGE_ENTRY(INFO_PAGE_TIMEOUT_BLOCK_START, 3)
#define TIMEOUT_INFO_TASK_QUEUE_RETRIES             INFO_PAGE_ENTRY(INFO_PAGE_TIMEOUT_BLOCK_START, 4)
#define TIMEOUT_INFO_TASK_QUEUE_FLUSH_TIMEOUT_MS    INFO_PAGE_ENTRY(INFO_PAGE_TIMEOUT_BLOCK_START, 5)

/* Bridge Info */

#define BRIDGE_INFO_RGX_BRIDGES                     INFO_PAGE_ENTRY(INFO_PAGE_BRIDGE_BLOCK_START, 0)
#define BRIDGE_INFO_PVR_BRIDGES                     INFO_PAGE_ENTRY(INFO_PAGE_BRIDGE_BLOCK_START, 1)

/* Debug features */

#define DEBUG_FEATURE_FLAGS                         INFO_PAGE_ENTRY(INFO_PAGE_DEBUG_BLOCK_START, 0)

#define DEBUG_FEATURE_FULL_SYNC_TRACKING_ENABLED    0x1 /* flag - not part of info page */
#define DEBUG_FEATURE_PAGE_FAULT_DEBUG_ENABLED      0x2 /* flag - not part of info page */

/* Device memory related information */

/* This value is 64-bits wide, next value should have index larger by 2 */
#define DEVMEM_INFO_PHYS_BUF_MAX_SIZE               INFO_PAGE_ENTRY(INFO_PAGE_DEVMEM_BLOCK_START, 0)

#endif /* INFO_PAGE_DEFS_H */

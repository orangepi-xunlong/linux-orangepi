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

#ifndef __gc_hal_kernel_metadata_h_
#define __gc_hal_kernel_metadata_h_

#ifdef __cplusplus
extern "C" {
#endif

/* Macro to combine four characters into a Character Code. */
#define __FOURCC(a, b, c, d) \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define VIV_VIDMEM_METADATA_MAGIC __FOURCC('v', 'i', 'v', 'm')

/* Metadata for cross-device fd share with additional (ts) info. */
typedef struct _VIV_VIDMEM_METADATA {
    uint32_t magic;

    int32_t  ts_fd;
    void     *ts_dma_buf;

    uint32_t fc_enabled;
    uint32_t fc_value;
    uint32_t fc_value_upper;

    uint32_t compressed;
    uint32_t compress_format;
} _VIV_VIDMEM_METADATA;

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_kernel_metadata_h_ */



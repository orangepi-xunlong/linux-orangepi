/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * Description:
 */

#ifndef __CVITEK_ION_ALLOC_H__
#define __CVITEK_ION_ALLOC_H__

#include "../../uapi/ion.h"

#define HEAP_QUERY_CNT	6

int cvi_ion_alloc(enum ion_heap_type type, size_t len, bool mmap_cache);
void cvi_ion_free(int fd);
int bm_ion_alloc(int heap_id, size_t len, bool mmap_cache);
void bm_ion_free(int fd);

#endif /* __CVITEK_ION_ALLOC_H__ */

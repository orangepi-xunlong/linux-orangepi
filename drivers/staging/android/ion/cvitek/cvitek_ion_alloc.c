// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * Description:
 */

#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/resource.h>

#include "../ion.h"
#include "cvitek_ion_alloc.h"

int cvi_ion_alloc(enum ion_heap_type type, size_t len, bool mmap_cache)
{
	struct ion_heap_query query;
	int ret = 0, index;
	unsigned int heap_id;
	struct ion_heap_data *heap_data;
	struct ion_buffer *buf;
#if defined(__arm__) || defined(__aarch64__)
	mm_segment_t old_fs = get_fs();
#endif
	memset(&query, 0, sizeof(struct ion_heap_query));
	query.cnt = HEAP_QUERY_CNT;
	heap_data = vzalloc(sizeof(*heap_data) * HEAP_QUERY_CNT);
	query.heaps = (unsigned long)heap_data;
	if (!query.heaps)
		return -ENOMEM;

	pr_debug("%s: len %zu looking for type %d and mmap it as %s\n",
		 __func__, len, type,
		 (mmap_cache) ? "cacheable" : "un-cacheable");

#if defined(__arm__) || defined(__aarch64__)
	set_fs(KERNEL_DS);
#else
	mm_segment_t old_fs = force_uaccess_begin();
#endif
	ret = ion_query_heaps(&query, true);

#if defined(__arm__) || defined(__aarch64__)
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif

	if (ret != 0)
		return ret;

	heap_id = HEAP_QUERY_CNT + 1;
	/* here only return the 1st match
	 * heap id that user requests for
	 */
	for (index = 0; index < query.cnt; index++) {
		if (heap_data[index].type == type) {
			heap_id = heap_data[index].heap_id;
			break;
		}
	}
	vfree(heap_data);
	return ion_alloc(len, 1 << heap_id,
			 ((mmap_cache) ? 1 : 0), &buf);
}
EXPORT_SYMBOL(cvi_ion_alloc);

void cvi_ion_free(int fd)
{
	ion_free(fd);
}
EXPORT_SYMBOL(cvi_ion_free);

int bm_ion_alloc(int heap_id, size_t len, bool mmap_cache)
{
	struct rlimit new_limit = {.rlim_max = 40960, .rlim_cur = 40960};
	struct rlimit old_limit = {0};
	struct ion_heap_query query;
	int ret = 0;
	struct ion_heap_data *heap_data;
	struct ion_buffer *buf;
#if defined(__arm__) || defined(__aarch64__)
	mm_segment_t old_fs = get_fs();
#endif
	memset(&query, 0, sizeof(struct ion_heap_query));
	query.cnt = HEAP_QUERY_CNT;
	heap_data = vzalloc(sizeof(*heap_data) * HEAP_QUERY_CNT);
	query.heaps = (unsigned long)heap_data;
	if (!query.heaps) {
		pr_err("vzalloc(%lu) failed\n", sizeof(*heap_data) * HEAP_QUERY_CNT);
		return -ENOMEM;
	}

	pr_debug("%s: len %zu looking for heapID %d and mmap it as %s\n",
		 __func__, len, heap_id,
		 (mmap_cache) ? "cacheable" : "un-cacheable");

#if defined(__arm__) || defined(__aarch64__)
	set_fs(KERNEL_DS);
#else
	mm_segment_t old_fs = force_uaccess_begin();
#endif
	ret = ion_query_heaps(&query, true);

#if defined(__arm__) || defined(__aarch64__)
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif

	if (ret != 0) {
		pr_err("ion_query_heaps failed,ret = %d\n", ret);
		return ret;
	}

	vfree(heap_data);
	//check kernel-thread resource.
	ret = do_prlimit(current, RLIMIT_NOFILE, NULL, &old_limit);
	if (old_limit.rlim_cur <= INR_OPEN_CUR) {
		//if kernel-thread <= 1024(open files), set RLIMIT_NOFILE to 40960.
		ret = do_prlimit(current, RLIMIT_NOFILE, &new_limit, NULL);
		if (ret < 0)
			pr_err("[%s] pid=%d,name=%s, do_prlimit error! ret = %d\n", __func__, current->pid, current->comm, ret);
	}
	ret = ion_alloc(len, 1 << heap_id,
			((mmap_cache) ? 1 : 0), &buf);
	if (ret < 0)
		pr_err("[%s] pid=%d,name=%s, ret = %d, rlim_cur = %lu(%x)\n"
			, __func__, current->pid, current->comm, ret, old_limit.rlim_cur, INR_OPEN_CUR);

	return ret;
}
EXPORT_SYMBOL(bm_ion_alloc);

void bm_ion_free(int fd)
{
	ion_free(fd);
}
EXPORT_SYMBOL(bm_ion_free);

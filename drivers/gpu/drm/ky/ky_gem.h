// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_GEM_H_
#define _KY_GEM_H_

#include <linux/scatterlist.h>
#include <drm/drm_device.h>
#include <drm/drm_gem.h>
#include "ky_dmmu.h"

struct ky_gem_object {
	struct drm_gem_object base;
	struct sg_table *sgt;
	void *vaddr;
	int vmap_cnt;
	struct mutex vmap_lock;
	struct list_head ttb_entry;
};

static inline struct ky_gem_object *
to_ky_obj(struct drm_gem_object *gem_obj)
{
	return container_of(gem_obj, struct ky_gem_object, base);
}

struct drm_gem_object *
ky_gem_prime_import_sg_table(struct drm_device *drm, struct dma_buf_attachment *attch,
				  struct sg_table *sgt);
int ky_gem_dumb_create(struct drm_file *file_priv, struct drm_device *drm,
			struct drm_mode_create_dumb *args);
int ky_gem_mmap(struct file *filp, struct vm_area_struct *vma);

#endif /* _KY_GEM_H_ */

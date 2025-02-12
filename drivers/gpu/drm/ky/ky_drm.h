// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_DRM_H_
#define _KY_DRM_H_

#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_drv.h>
#include <drm/drm_vblank.h>
#include <drm/drm_probe_helper.h>
#include <linux/dma-mapping.h>
#include "ky_cmdlist.h"

struct ky_hw_device {
	void __iomem *base;
	phys_addr_t phy_addr;
	u8 plane_nums;
	u8 rdma_nums;
	const struct ky_hw_rdma *rdmas;
	u8 n_formats;
	const struct dpu_format_id *formats;
	u8 n_fbcmems;
	const u32 *fbcmem_sizes;
	const u32 *rdma_fixed_fbcmem_sizes;
	u32 solid_color_shift;
	int hdr_coef_size;
	int scale_coef_size;
	bool is_hdmi;
};

struct ky_drm_private {
	struct drm_device *ddev;
	struct device *dev;
	struct ky_hw_device *hwdev;
	int hw_ver;
	bool contig_mem;
	int num_pipes;
	struct cmdlist **cmdlist_groups;
	struct cmdlist_reg *cmdlist_regs;
	int cmdlist_num;
};

extern struct platform_driver ky_dpu_driver;
extern struct platform_driver ky_dphy_driver;
extern struct platform_driver ky_dsi_driver;

int ky_wb_init(struct drm_device *drm, struct drm_crtc *crtc);
void ky_wb_atomic_commit(struct drm_device *drm, struct drm_atomic_state *old_state);

#endif /* _KY_DRM_H_ */

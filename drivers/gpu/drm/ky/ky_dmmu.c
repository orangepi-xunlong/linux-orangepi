// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include "linux/compiler.h"
#include <linux/stddef.h>
#include <linux/gfp.h>
#include <linux/dma-mapping.h>
#include <drm/drm_framebuffer.h>

#include "ky_cmdlist.h"
#include "ky_dmmu.h"
#include "ky_gem.h"
#include "ky_drm.h"
#include "ky_dpu.h"
#include "ky_dpu_reg.h"

static inline void ky_dmmu_fill_pgtable(uint32_t *ttbr, struct sg_table *sgt)
{
	struct sg_dma_page_iter dma_iter;
	uint32_t i = 0, temp = 0;

	for_each_sgtable_dma_page(sgt, &dma_iter, 0)
		*ttbr++ = (sg_page_iter_dma_address(&dma_iter) >> PAGE_SHIFT) & 0x3FFFFF;
	/*
	 * Due to slicon's alignment access requirement, fill more mmu entries
	 * to avoid ddrc filter error. We simply use the last valid ttbr value.
	 */
	 temp = *(ttbr - 1);
	 for (i = 0; i < HW_ALIGN_TTB_NUM; i++)
		*ttbr++ = temp;
}

int ky_dmmu_map(struct drm_plane *plane, struct dpu_mmu_tbl *mmu_tbl, u8 tbu_id, bool wb)
{
	struct drm_framebuffer *fb = plane->state->fb;
	struct ky_drm_private *priv = fb->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	const struct drm_format_info *format = NULL;
	struct sg_table *sgt = NULL;
	uint32_t total_size, offset1, offset2;
	struct tbu_instance tbu = {0x0};
	u8 fbc_mode, plane_num, rdma_id;
	u32 val;

	rdma_id = tbu_id / 2;

	format = fb->format;
	sgt = to_ky_obj(fb->obj[0])->sgt;

	if (!wb && priv->contig_mem) {
		phys_addr_t contig_pa = sg_dma_address(sgt->sgl);

		write_to_cmdlist(priv, MMU_REG, MMU_BASE_ADDR, TBU[tbu_id].TBU_Ctrl, 0x0);
#if defined (CONFIG_ARM64) || defined (CONFIG_ARM_LPAE) || defined (CONFIG_ARCH_RV64I)
		CONFIG_RDMA_ADDR_REG(priv, 0, rdma_id, contig_pa);
		CONFIG_RDMA_ADDR_REG(priv, 1, rdma_id, (contig_pa + fb->offsets[1]));
		CONFIG_RDMA_ADDR_REG(priv, 2, rdma_id, (contig_pa + fb->offsets[2]));
#else
		CONFIG_RDMA_ADDR_REG_32(priv, 0, rdma_id, contig_pa);
		CONFIG_RDMA_ADDR_REG_32(priv, 1, rdma_id, (contig_pa + fb->offsets[1]));
		CONFIG_RDMA_ADDR_REG_32(priv, 2, rdma_id, (contig_pa + fb->offsets[2]));
#endif
		write_to_cmdlist(priv, RDMA_PATH_X_REG, RDMA_BASE_ADDR(rdma_id), \
				 LEFT_RDMA_STRIDE0, (fb->pitches[1] << 16) | fb->pitches[0]);

		return 0;
	}

	plane_num = format->num_planes;
	fbc_mode = (fb->modifier > 0);
	total_size = roundup(fb->obj[0]->size, PAGE_SIZE);
	offset1 = plane_num > 1 ? fb->offsets[1] : total_size;
	offset2 = plane_num > 2 ? fb->offsets[2] : total_size;

	switch (plane_num) {
	case 3:
		tbu.ttb_pa[2] = mmu_tbl->pa + (offset2 >> PAGE_SHIFT) * 4;
		tbu.tbu_va[2] = TBU_BASE_VA(tbu_id) + offset2;
		tbu.ttb_size[2] = PAGE_ALIGN(total_size - rounddown(offset2, PAGE_SIZE)) >> PAGE_SHIFT;
		fallthrough;
	case 2:
		tbu.ttb_pa[1] = mmu_tbl->pa + (offset1 >> PAGE_SHIFT) * 4;
		tbu.tbu_va[1] = TBU_BASE_VA(tbu_id) + offset1;
		tbu.ttb_size[1] = PAGE_ALIGN(offset2 - rounddown(offset1, PAGE_SIZE)) >> PAGE_SHIFT;
		fallthrough;
	case 1:
		tbu.ttb_pa[0] = mmu_tbl->pa;
		tbu.tbu_va[0] = TBU_BASE_VA(tbu_id);
		tbu.ttb_size[0] = PAGE_ALIGN(offset1) >> PAGE_SHIFT;
		fallthrough;
	default:
		break;
	}
	/* Special handling for afbc */
	if (fbc_mode) {
		tbu.tbu_va[1] = tbu.tbu_va[0];
		tbu.ttb_pa[1] = tbu.ttb_pa[0];
		tbu.ttb_size[1] = tbu.ttb_size[0];
	}

	ky_dmmu_fill_pgtable((uint32_t *)(mmu_tbl->va), sgt);

	val = 0x1 | (fbc_mode << 1) | ((fbc_mode ? 1 : plane_num - 1) << 2);
	/* Config hw regs */
	if (wb) {
		val = val | (DPU_QOS_LOW << 8);
		CONFIG_WB_ADDR_REG(hwdev, 0, tbu.tbu_va[0]);
		CONFIG_WB_ADDR_REG(hwdev, 1, tbu.tbu_va[1]);
		dpu_write_reg(hwdev, WB_TOP_REG, WB0_TOP_BASE_ADDR, \
				wb_wdma_stride, (fb->pitches[1] << 16) | fb->pitches[0]);
		CONFIG_TBU_REGS(priv, hwdev, 0, tbu_id);
		CONFIG_TBU_REGS(priv, hwdev, 1, tbu_id);
		CONFIG_TBU_REGS(priv, hwdev, 2, tbu_id);
		dpu_write_reg(hwdev, MMU_REG, MMU_BASE_ADDR, v.TBU[tbu_id].TBU_Ctrl, val);
	} else {
		val = val | (DPU_QOS_URGENT << 8);
#if defined (CONFIG_ARM64) || defined (CONFIG_ARM_LPAE) || defined (CONFIG_ARCH_RV64I)
		CONFIG_RDMA_ADDR_REG(priv, 0, rdma_id, tbu.tbu_va[0]);
		CONFIG_RDMA_ADDR_REG(priv, 1, rdma_id, tbu.tbu_va[1]);
		CONFIG_RDMA_ADDR_REG(priv, 2, rdma_id, tbu.tbu_va[2]);
#else
		CONFIG_RDMA_ADDR_REG_32(priv, 0, rdma_id, tbu.tbu_va[0]);
		CONFIG_RDMA_ADDR_REG_32(priv, 1, rdma_id, tbu.tbu_va[1]);
		CONFIG_RDMA_ADDR_REG_32(priv, 2, rdma_id, tbu.tbu_va[2]);
#endif
		write_to_cmdlist(priv, RDMA_PATH_X_REG, RDMA_BASE_ADDR(rdma_id), \
				LEFT_RDMA_STRIDE0, (fb->pitches[1] << 16) | fb->pitches[0]);
		CONFIG_TBU_REGS(priv, NULL, 0, tbu_id);
		CONFIG_TBU_REGS(priv, NULL, 1, tbu_id);
		CONFIG_TBU_REGS(priv, NULL, 2, tbu_id);
		write_to_cmdlist(priv, MMU_REG, MMU_BASE_ADDR, TBU[tbu_id].TBU_Ctrl, val);
	}

	return 0;
}

void ky_dmmu_unmap(struct drm_plane *plane)
{
	return;
}

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/dma-mapping.h>
#include <linux/sort.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include "ky_cmdlist.h"
#include "ky_dpu_reg.h"
#include "ky_drm.h"
#include "dpu/dpu_saturn.h"

#define CL_HEADER_SZ sizeof(struct cmdlist_header)
#define CL_ROW_SZ    sizeof(struct cmdlist_row)

static inline struct
ky_plane_state *cl_to_ky_pstate(const struct cmdlist *cl)
{
	return container_of(cl, struct ky_plane_state, cl);
}

static void print_row(struct cmdlist_row *row) {
	u32 * p = (u32 *) row;
	DRM_DEBUG("print_row: 0x%02x, 0x%02x, 0x%02x, 0x%02x", *p, *(p+1), *(p+2), *(p+3));
}

static int cmdlist_reg_cmp(const void * r1, const void * r2) {
	const struct cmdlist_reg *reg1 = (const struct cmdlist_reg *) r1;
	const struct cmdlist_reg *reg2 = (const struct cmdlist_reg *) r2;
	if (reg1->offset > reg2->offset)
		return 1;
	else if (reg1->offset < reg2->offset)
		return -1;
	else
		return 0;
}

static void cmdlist_reg_swap(void * r1, void * r2, int size) {
	struct cmdlist_reg *reg1 = (struct cmdlist_reg *) r1;
	struct cmdlist_reg *reg2 = (struct cmdlist_reg *) r2;
	struct cmdlist_reg tmp = *reg1;
	*reg1 = *reg2;
	*reg2 = tmp;
}

void cmdlist_regs_packing(struct drm_plane *plane) {
	struct ky_dpu *dpu = crtc_to_dpu(plane->state->crtc);
	struct ky_plane_state *ky_pstate = to_ky_plane_state(plane->state);
	struct cmdlist *cl = &ky_pstate->cl;
	struct cmdlist_row *row;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct cmdlist_reg *regs = priv->cmdlist_regs;
	int i;

	cl->size = PER_CMDLIST_SIZE;
	cl->va = dma_alloc_coherent(dpu->dev, cl->size, &cl->pa, GFP_KERNEL | __GFP_ZERO);
	if (cl->va == NULL) {
		DRM_ERROR("Failed to allocate %d bytes for dpu plane%d cmdlist buffer\n",
			  PER_CMDLIST_SIZE, plane->state->zpos);
		return;
	}

	sort(regs, priv->cmdlist_num, sizeof(struct cmdlist_reg), cmdlist_reg_cmp, cmdlist_reg_swap);
	for (i = 0; i < priv->cmdlist_num; i++) {
		DRM_DEBUG("cmdlist_reg, regs[%d] = {0x%02x, 0x%02x}", i, regs[i].offset, regs[i].value);
	}

	DRM_DEBUG("-----cmdlist_regs_packing----- priv->cmdlist_num = %u, rch_id = %d, ch_y = %u\n",
		  priv->cmdlist_num, ky_pstate->rdma_id, ky_pstate->state.crtc_y);

	row = (struct cmdlist_row *)((char *)cl->va + CL_HEADER_SZ);
	for (i = 0; i < priv->cmdlist_num;) {
		row->module_cfg_addr = regs[i].offset >> 2;
		row->module_regs[0] = regs[i].value;
		row->module_cfg_num = 1;
		row->module_cfg_strobe = 0xf;
		if (i + 1 < priv->cmdlist_num) {
			if (regs[i + 1].offset - regs[i].offset == sizeof(u32)) {
				row->module_regs[1] = regs[i + 1].value;
				row->module_cfg_num = 2;
				row->module_cfg_strobe = 0xff;
			}
		}
		if (i + 2 < priv->cmdlist_num) {
			if (regs[i + 2].offset - regs[i].offset == sizeof(u32) * 2) {
				row->module_regs[2] = regs[i + 2].value;
				row->module_cfg_num = 3;
				row->module_cfg_strobe = 0xfff;
			}
		}

		print_row(row);

		i += row->module_cfg_num;
		cl->nod_len++;
		row = (struct cmdlist_row *)((char *)row + CL_ROW_SZ);
	}
	priv->cmdlist_num = 0; //clear cmdlist_regs buffer
	DRM_DEBUG("-----cmdlist_regs_packing----- row_num = %d\n", row->module_cfg_num);
}

static inline void fill_top_row(struct cmdlist *cl) {
	struct cmdlist_header *header;
	struct cmdlist_row *row;
	unsigned int cl_addr_h = 0;
	struct ky_plane_state *ky_pstate = cl_to_ky_pstate(cl);
	u8 rch_id = ky_pstate->rdma_id;
	u8 scl_id = ky_pstate->scaler_id;
	bool use_scl = ky_pstate->use_scl;
	unsigned int zpos = ky_pstate->state.zpos;
	u32 size = 0;

	// fill header
	header = (struct cmdlist_header *)(cl->va);
	if (cl->next) {
		header->next_list_addr = cl->next->pa;
	} else
		header->list_tag = 1;
	// end cl->nod_len = cl->nod_len + 4, 3(row) + 1(head)
	header->nod_len = cl->nod_len + 4;

	// fill scaler row;
	row = (struct cmdlist_row *)((char *)header + CL_HEADER_SZ + cl->nod_len * CL_ROW_SZ);
	row->module_cfg_addr = ((DPU_CTRL_BASE_ADDR + DPU_NUM_REUSE_SCL) >> 2) + scl_id;
	row->module_cfg_strobe = 0xff;
	if (scl_id != SCALER_INVALID_ID) {
		row->module_cfg_num = 1;
		if (use_scl)
			row->module_regs[0] = 1;
		else
			row->module_regs[0] = 0;
	} else
		row->module_cfg_num = 0;

	// fill ch_y row
	row = (struct cmdlist_row *)((char *)row + CL_ROW_SZ);
	cl_addr_h = header->next_list_addr >> 32;
	row->module_cfg_addr = (CMDLIST_BASE_ADDR + CMDLIST_CH_Y + rch_id * 4) >> 2;
	row->module_cfg_num = cl->next ? 1 : 0;
	row->module_cfg_strobe = 0xf;
	row->module_regs[0] = cl->next ? cl_to_ky_pstate(cl->next)->state.crtc_y << 8 | \
			      (cl_addr_h & CMDLIST_ADDRH_MASK) : 0;
	row->module_regs[1] = 0;

	// fill last_row
	row = (struct cmdlist_row *)((char *)row + CL_ROW_SZ);
	row->module_cfg_addr = (DPU_CTRL_BASE_ADDR + CMDLIST_CFG_READY) >> 2;
	row->module_cfg_num = 1;
	row->module_cfg_strobe = 0xf0f;
	row->row_eof_tag = 3;
	row->module_regs[0] = 1 << rch_id;
	row->module_regs[2] = rch_id;

	size = (unsigned long)((char *)row + CL_ROW_SZ) - (unsigned long)header;
	if (size > PER_CMDLIST_SIZE)
		DRM_ERROR("plane%d cmdlist occupies %d bytes!\n", zpos, size);
}

void cmdlist_sort_by_group(struct drm_crtc *crtc) {
	struct cmdlist * cur_cl;
	struct cmdlist * p;
	struct cmdlist * prev;
	struct drm_plane *plane;
	struct ky_dpu_rdma *rdmas = to_ky_crtc_state(crtc->state)->rdmas;
	struct ky_drm_private *priv = crtc->dev->dev_private;

	drm_atomic_crtc_for_each_plane(plane, crtc) {
		struct ky_plane_state *ky_pstate = to_ky_plane_state(plane->state);
		u32 rdma_id = ky_pstate->rdma_id;
		cur_cl = (struct cmdlist *)(&(ky_pstate->cl));

		if (!cur_cl->va)
			continue;

		rdmas[rdma_id].in_use = true;
		if (priv->cmdlist_groups[rdma_id]) {
			p = priv->cmdlist_groups[rdma_id];
			prev = NULL;
			while(p) {
				if (cl_to_ky_pstate(p)->state.crtc_y < ky_pstate->state.crtc_y) {
					prev = p;
					p = p->next;
				}
				else
					break;
			}
			if (!prev) {
				priv->cmdlist_groups[rdma_id] = cur_cl;
				cur_cl->next = p;
			} else {
				prev->next = cur_cl;
				cur_cl->next = p;
			}
		} else {
			priv->cmdlist_groups[rdma_id] = cur_cl;
			cur_cl->next = NULL;
		}
	}
}

void cmdlist_atomic_commit(struct drm_crtc *crtc,
			   struct drm_crtc_state *old_state) {
	int i;
	struct cmdlist *cur_cl, *first_cl;
	u32 val;
	struct ky_drm_private *priv = crtc->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	struct ky_dpu_rdma *cur_rdmas = to_ky_crtc_state(crtc->state)->rdmas;
	struct ky_dpu_rdma *old_rdmas = to_ky_crtc_state(old_state)->rdmas;

	for (i = 0; i < hwdev->rdma_nums; i++) {
		/* Shut down the rdma used in previous frame first */
		if (old_rdmas[i].in_use) {
			dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR,
			              dpu_ctl_reg_69[i].ctl_nml_cmdlist_rch_en, 0);
			dpu_write_reg(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, value32[i], 0);
		}

		if (cur_rdmas[i].in_use) {
			DRM_DEBUG("+++++cmdlist_atomic_commit+++++ cmdlist group = %d\n", i);
			cur_cl = priv->cmdlist_groups[i];
			first_cl = cur_cl;
			while(cur_cl) {
				fill_top_row(cur_cl);
				cur_cl = cur_cl->next;
			}
			val = cl_to_ky_pstate(first_cl)->state.crtc_y;
			dpu_write_reg(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_14[i].cmdlist_ch_y, val);
			val = ((priv->cmdlist_groups[i]->pa) & CMDLIST_ADDRL_ALIGN_MASK) >> CMDLIST_ADDRL_ALIGN_BITS;
			dpu_write_reg(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_0[i].cmdlist_ch_start_addrl, val);
#if defined (CONFIG_ARM64) || defined (CONFIG_ARM_LPAE)
			val = (priv->cmdlist_groups[i]->pa) >> 32;
			dpu_write_reg(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_14[i].cmdlist_ch_start_addrh, val);
#else
			dpu_write_reg(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_14[i].cmdlist_ch_start_addrh, 0);
#endif
			dpu_write_reg(hwdev, DPU_CTL_REG, DPU_CTRL_BASE_ADDR,
			              dpu_ctl_reg_69[i].ctl_nml_cmdlist_rch_en, 1);
			priv->cmdlist_groups[i] = NULL;
		}
	}
}

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _SATURN_FW_CMDLIST_H_
#define _SATURN_FW_CMDLIST_H_

#include <linux/stddef.h>
#include <linux/types.h>
#include <drm/drm_crtc.h>
#include <drm/drm_plane.h>

#define PER_CMDLIST_SIZE             4096
#define CMDLIST_CH_Y                 0x38
#define DPU_NUM_REUSE_SCL            0xF4
#define CMDLIST_CFG_READY            0x14C

#define CMDLIST_ADDRH_MASK		(0x3)

#define CMDLIST_REGS_NUM    (PAGE_SIZE / sizeof(struct cmdlist_reg))

struct cmdlist {
	u16 nod_len;        //the number of cmdlist row.
	u32 size;
	void *va;
	dma_addr_t pa;
	struct cmdlist * next;
};

/* cmdlist structure */
struct cmdlist_header {
	u64 next_list_addr : 34; //the next cmdlist address
	// reserved
	u32  :6;
	u32 nod_len : 16;
	// if the cmdlist is last one tag = 1, otherwish tag = 0
	u32 list_tag : 1;
	// reserved
	u32   : 7;
	u64   : 64;
};

struct cmdlist_row {
	u32 module_cfg_addr   : 16;
	//the last row must 1-2, otherwise 1-3.
	u32 module_cfg_num    : 2;
	u32 module_cfg_strobe : 12;
	//the last row tag = 3, otherwise tag = 0.
	u32 row_eof_tag       : 2;
	u32 module_regs[3];
};

struct cmdlist_reg {
    uint32_t offset;
    uint32_t value;
};

#define dpu_offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->v.MEMBER)
#define write_to_cmdlist(priv, module_name, module_base, reg, val) \
{ \
	u32 offset = module_base + dpu_offsetof(module_name, reg); \
	priv->cmdlist_regs[priv->cmdlist_num].value = val; \
	priv->cmdlist_regs[priv->cmdlist_num++].offset = offset; \
}

void cmdlist_regs_packing(struct drm_plane *plane);
void cmdlist_sort_by_group(struct drm_crtc *crtc);
void cmdlist_atomic_commit(struct drm_crtc *crtc,
			   struct drm_crtc_state *old_state);
#endif

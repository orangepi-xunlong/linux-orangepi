// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/io.h>
#include <linux/trace_events.h>
#include <drm/drm_gem.h>
#include <drm/drm_framebuffer.h>
#include "dpu_debug.h"
#include "dpu_trace.h"
#include "./../ky_dpu_reg.h"
#include "./../ky_drm.h"

dpu_reg_enum SATURN_LE_DPU_REG_ENUM_LISTS[] = {
	E_DPU_TOP_REG,
	E_DPU_CTRL_REG,
	E_DPU_CTRL_REG,
	E_DPU_CMDLIST_REG,
	E_DPU_INT_REG,
	E_DMA_TOP_CTRL_REG,
	E_RDMA_LAYER0_REG,
	E_RDMA_LAYER1_REG,
	E_RDMA_LAYER2_REG,
	E_RDMA_LAYER3_REG,
	E_MMU_TBU0_REG,
	E_MMU_TBU2_REG,
	E_MMU_TBU4_REG,
	E_MMU_TBU6_REG,
	E_COMPOSER2_REG,
	E_SCALER0_REG,
	E_OUTCTRL2_REG
};

static dpu_reg_dump_t dpu_reg_dump_array[] = {
	{E_DPU_TOP_REG, "DPU_TOP", DPU_TOP_BASE_ADDR, 218},

	{E_DPU_CTRL_REG, "DPU_CTRL", DPU_CTRL_BASE_ADDR, 5},
	{E_DPU_CTRL_REG, "DPU_CTRL", DPU_CTRL_BASE_ADDR + 0x24, 8},
	{E_DPU_CTRL_REG, "DPU_CTRL", DPU_CTRL_BASE_ADDR + 0x54, 8},
	{E_DPU_CTRL_REG, "DPU_CTRL", DPU_CTRL_BASE_ADDR + 0x84, 19},
	{E_DPU_CTRL_REG, "DPU_CTRL", DPU_CTRL_BASE_ADDR + 0xe4, 8},
	{E_DPU_CTRL_REG, "DPU_CTRL", DPU_CTRL_BASE_ADDR + 0x114, 25},

	{E_DPU_CRG_REG, "DPU_CRG", DPU_CRG_BASE_ADDR, 5},

	{E_DPU_CMDLIST_REG, "DPU_CMDLIST", CMDLIST_BASE_ADDR, 44},

	{E_DPU_INT_REG, "DPU_INT", DPU_INT_BASE_ADDR, 40},

	{E_DMA_TOP_CTRL_REG, "DMA_TOP_CTRL", DMA_TOP_BASE_ADDR, 25},

	{E_RDMA_LAYER0_REG, "RDMA_LAYER0", RDMA0_BASE_ADDR, 31},
	{E_RDMA_LAYER0_REG, "RDMA_LAYER0", RDMA0_BASE_ADDR + 0x80, 57},

	{E_RDMA_LAYER1_REG, "RDMA_LAYER1", RDMA1_BASE_ADDR, 31},
	{E_RDMA_LAYER1_REG, "RDMA_LAYER1", RDMA1_BASE_ADDR + 0x80, 57},

	{E_RDMA_LAYER2_REG, "RDMA_LAYER2", RDMA2_BASE_ADDR, 31},
	{E_RDMA_LAYER2_REG, "RDMA_LAYER2", RDMA2_BASE_ADDR + 0x80, 57},

	{E_RDMA_LAYER3_REG, "RDMA_LAYER3" ,RDMA3_BASE_ADDR, 31},
	{E_RDMA_LAYER3_REG, "RDMA_LAYER3", RDMA3_BASE_ADDR + 0x80, 57},

	{E_RDMA_LAYER4_REG, "RDMA_LAYER4", RDMA4_BASE_ADDR, 46},

	{E_RDMA_LAYER5_REG, "RDMA_LAYER5", RDMA5_BASE_ADDR, 46},

	{E_RDMA_LAYER6_REG, "RDMA_LAYER6", RDMA6_BASE_ADDR, 46},

	{E_RDMA_LAYER7_REG, "RDMA_LAYER7", RDMA7_BASE_ADDR, 46},

	{E_RDMA_LAYER8_REG, "RDMA_LAYER8", RDMA8_BASE_ADDR, 46},

	{E_RDMA_LAYER9_REG, "RDMA_LAYER9", RDMA9_BASE_ADDR, 46},

	{E_RDMA_LAYER10_REG, "RDMA_LAYER10", RDMA10_BASE_ADDR, 46},

	{E_RDMA_LAYER11_REG, "RDMA_LAYER11", RDMA11_BASE_ADDR, 46},

	{E_MMU_TBU0_REG, "MMU_TBU0", MMU_TBU_BASE_ADDR + 0 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU1_REG, "MMU_TBU1", MMU_TBU_BASE_ADDR + 1 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU2_REG, "MMU_TBU2", MMU_TBU_BASE_ADDR + 2 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU3_REG, "MMU_TBU3", MMU_TBU_BASE_ADDR + 3 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU4_REG, "MMU_TBU4", MMU_TBU_BASE_ADDR + 4 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU5_REG, "MMU_TBU5", MMU_TBU_BASE_ADDR + 5 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU6_REG, "MMU_TBU6", MMU_TBU_BASE_ADDR + 6 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU7_REG, "MMU_TBU7", MMU_TBU_BASE_ADDR + 7 * MMU_TBU_SIZE, 13},
	{E_MMU_TBU8_REG, "MMU_TBU8", MMU_TBU_BASE_ADDR + 8 * MMU_TBU_SIZE, 13},
	{E_MMU_TOP_REG,  "MMU_TOP",  MMU_TOP_BASE_ADDR, 13},

	{E_LP0_REG, "LP0", LP0_BASE_ADDR, 81},
	{E_LP1_REG, "LP1", LP1_BASE_ADDR, 81},
	{E_LP2_REG, "LP2", LP2_BASE_ADDR, 81},
	{E_LP3_REG, "LP3", LP3_BASE_ADDR, 81},
	{E_LP4_REG, "LP4", LP4_BASE_ADDR, 81},
	{E_LP5_REG, "LP5", LP5_BASE_ADDR, 81},
	{E_LP6_REG, "LP6", LP6_BASE_ADDR, 81},
	{E_LP7_REG, "LP7", LP7_BASE_ADDR, 81},
	{E_LP8_REG, "LP8", LP8_BASE_ADDR, 81},
	{E_LP9_REG, "LP9", LP9_BASE_ADDR, 81},
	{E_LP10_REG, "LP10", LP10_BASE_ADDR, 81},
	{E_LP11_REG, "LP11", LP11_BASE_ADDR, 81},

	{E_LM0_REG, "LMERGE0", LMERGE0_BASE_ADDR, 4},
	{E_LM1_REG, "LMERGE1", LMERGE1_BASE_ADDR, 4},
	{E_LM2_REG, "LMERGE2", LMERGE2_BASE_ADDR, 4},
	{E_LM3_REG, "LMERGE3", LMERGE3_BASE_ADDR, 4},
	{E_LM4_REG, "LMERGE4", LMERGE4_BASE_ADDR, 4},
	{E_LM5_REG, "LMERGE5", LMERGE5_BASE_ADDR, 4},
	{E_LM6_REG, "LMERGE6", LMERGE6_BASE_ADDR, 4},
	{E_LM7_REG, "LMERGE7", LMERGE7_BASE_ADDR, 4},
	{E_LM8_REG, "LMERGE8", LMERGE8_BASE_ADDR, 4},
	{E_LM9_REG, "LMERGE9", LMERGE9_BASE_ADDR, 4},
	{E_LM10_REG, "LMERGE10", LMERGE10_BASE_ADDR, 4},
	{E_LM11_REG, "LMERGE11", LMERGE11_BASE_ADDR, 4},

	{E_COMPOSER0_REG, "COMPOSER0", CMP0_BASE_ADDR, 146},
	{E_COMPOSER1_REG, "COMPOSER1", CMP1_BASE_ADDR, 146},
	{E_COMPOSER2_REG, "COMPOSER2", CMP2_BASE_ADDR, 146},
	{E_COMPOSER3_REG, "COMPOSER3", CMP3_BASE_ADDR, 146},

	{E_SCALER0_REG, "SCALER0", SCALER0_ONLINE_BASE_ADDR, 121},
	{E_SCALER1_REG, "SCALER1", SCALER1_ONLINE_BASE_ADDR, 121},

	{E_OUTCTRL0_REG, "OUTCTRL0", OUTCTRL0_BASE_ADDR, 55},
	{E_PP0_REG, "PP0", PP0_BASE_ADDR, 86},
	{E_OUTCTRL1_REG, "OUTCTRL1", OUTCTRL1_BASE_ADDR, 55},
	{E_PP1_REG, "PP1", PP1_BASE_ADDR, 86},
	{E_OUTCTRL2_REG, "OUTCTRL2", OUTCTRL2_BASE_ADDR, 55},
	{E_PP2_REG, "PP2", PP2_BASE_ADDR, 86},
	{E_OUTCTRL3_REG, "OUTCTRL3", OUTCTRL3_BASE_ADDR, 55},
	{E_PP3_REG, "PP3", PP3_BASE_ADDR, 86},

	{E_WB_TOP_0_REG, "WB_TOP_0", WB0_TOP_BASE_ADDR, 54},
	{E_WB_TOP_1_REG, "WB_TOP_1", WB1_TOP_BASE_ADDR, 54},
};

static void dump_dpu_regs_by_enum(void __iomem *io_base, phys_addr_t phy_base, dpu_reg_enum reg_enum, u8 trace_dump)
{
	int i;
	int j;
	uint32_t reg_num;
	void __iomem *io_addr;
	phys_addr_t phy_addr;

	dpu_reg_dump_t *tmp = &dpu_reg_dump_array[0];

	for (i = 0; i < ARRAY_SIZE(dpu_reg_dump_array); i++) {
		if (tmp->index == reg_enum) {
			reg_num = tmp->dump_reg_num;
			io_addr = io_base + tmp->module_offset;
			phy_addr = phy_base + tmp->module_offset;
			if (trace_dump) {
				trace_dpu_reg_info(tmp->module_name, phy_addr, reg_num);
				for (j = 0; j < reg_num; j++) {
					trace_dpu_reg_dump(phy_addr + j * 4, readl(io_addr + j * 4));
				}
			} else {
				printk(KERN_DEBUG "%d-%s, address:0x%08llx, num:%d\n", tmp->index, tmp->module_name, phy_addr, reg_num);
				for (j = 0; j < reg_num; j++) {
					printk(KERN_DEBUG "0x%08llx: 0x%08x\n", (phy_addr + j * 4), readl(io_addr + j * 4));
				}
			}
		}
		tmp++;
	}
}

bool dpu_reg_enum_valid(dpu_reg_enum reg_enum)
{
	int size = ARRAY_SIZE(SATURN_LE_DPU_REG_ENUM_LISTS);
	int i = 0;

	for (i = 0; i < size; i++) {
		if (reg_enum == SATURN_LE_DPU_REG_ENUM_LISTS[i])
			return true;
	}

	return false;
}

void dump_dpu_regs(struct ky_dpu *dpu, dpu_reg_enum reg_enum, u8 trace_dump)
{
	dpu_reg_enum tmp = E_DPU_TOP_REG;
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	void __iomem* reg_io_base = hwdev->base;
	phys_addr_t reg_phy_base = hwdev->phy_addr;

	if (reg_enum > E_DPU_DUMP_ALL) {
		pr_err("invalid dump regsiter enum\n");
		return;
	} else if (reg_enum == E_DPU_DUMP_ALL) {
		for (; tmp < E_DPU_DUMP_ALL; tmp++) {
			if (dpu_reg_enum_valid(tmp))
				dump_dpu_regs_by_enum(reg_io_base, reg_phy_base, tmp, trace_dump);
		}
	} else {
		dump_dpu_regs_by_enum(reg_io_base, reg_phy_base, reg_enum, trace_dump);
	}
}

static void dpu_debug_mode(struct ky_hw_device *hwdev, int pipeline_id, bool enable)
{
	u32 base = DPU_CTRL_BASE_ADDR;

	switch (pipeline_id) {
	case ONLINE0:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl0_dbg_mod, enable ? 1 : 0);
		break;
	case ONLINE1:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl1_dbg_mod, enable ? 1 : 0);
		break;
	case ONLINE2:
		dpu_write_reg(hwdev, DPU_CTL_REG, base, ctl2_dbg_mod, enable ? 1 : 0);
		break;
	case OFFLINE0:
	case OFFLINE1:
	default:
		DRM_ERROR("pipeline id is invalid!\n");
		break;
	}
}
#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
static struct file *gki_filp_open(const char *filename, int flags, umode_t mode)
{
	return 0;
}
static ssize_t gki_kernel_write(struct file *file, const void *buf, size_t count,
			    loff_t *pos)
{
	return 0;
}
#endif
#define DPU_BUFFER_DUMP_FILE "/mnt/dpu_buffer_dump"
int dpu_buffer_dump(struct drm_plane *plane) {
	unsigned int buffer_size = 0;
	int i = 0;
	void *mmu_tbl_vaddr = NULL;
	phys_addr_t dpu_buffer_paddr = 0;
	void __iomem *dpu_buffer_vaddr = NULL;
	loff_t pos = 0;
	static int dump_once = true;
	struct file *filep = NULL;
	struct ky_plane_state *ky_pstate = to_ky_plane_state(plane->state);

	if (!dump_once)
		return 0;

	mmu_tbl_vaddr = ky_pstate->mmu_tbl.va;
	buffer_size = plane->state->fb->obj[0]->size >> PAGE_SHIFT;

#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
	filep = gki_filp_open(DPU_BUFFER_DUMP_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
#else
	filep = filp_open(DPU_BUFFER_DUMP_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
#endif

	if (IS_ERR(filep)) {
		printk("Open file %s error\n", DPU_BUFFER_DUMP_FILE);
		return -EINVAL;
	}
	for (i = 0; i < buffer_size; i++) {
		dpu_buffer_paddr = *(volatile u32 __force *)mmu_tbl_vaddr;
		dpu_buffer_paddr = dpu_buffer_paddr << PAGE_SHIFT;
		if (dpu_buffer_paddr >= 0x80000000UL) {
			dpu_buffer_paddr += 0x80000000UL;
		}
		dpu_buffer_vaddr = phys_to_virt((unsigned long)dpu_buffer_paddr);
		mmu_tbl_vaddr += 4;
#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
		gki_kernel_write(filep, (void *)dpu_buffer_vaddr, PAGE_SIZE, &pos);
#else
		kernel_write(filep, (void *)dpu_buffer_vaddr, PAGE_SIZE, &pos);
#endif
	}

	filp_close(filep, NULL);
	filep = NULL;

	dump_once = false;

	return 0;
}

void dpu_dump_reg(struct ky_dpu *dpu)
{
	struct ky_drm_private *priv = dpu->crtc.dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;

	if (!dpu->enable_dump_reg)
		return;

	dpu_debug_mode(hwdev, ONLINE2, 1);
	dump_dpu_regs(dpu, E_DPU_DUMP_ALL, 1);
	dpu_debug_mode(hwdev, ONLINE2, 0);
}

void dpu_dump_fps(struct ky_dpu *dpu)
{
	struct timespec64 cur_tm, tmp_tm;

	if (!dpu->enable_dump_fps)
		return;

	ktime_get_real_ts64(&cur_tm);
	tmp_tm = timespec64_sub(cur_tm, dpu->last_tm);
	dpu->last_tm.tv_sec = cur_tm.tv_sec;
	dpu->last_tm.tv_nsec = cur_tm.tv_nsec;
	if (tmp_tm.tv_sec == 0)
		trace_printk("fps: %ld\n", 1000000000 / (tmp_tm.tv_nsec / 1000));
}

void dpu_underrun_wq_stop_trace(struct work_struct *work)
{
// #ifndef MODULE
// 	trace_set_clr_event("dpu", NULL, false);
// #endif
}

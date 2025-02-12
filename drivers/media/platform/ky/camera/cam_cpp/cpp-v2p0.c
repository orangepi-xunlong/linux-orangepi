// SPDX-License-Identifier: GPL-2.0
/*
 * cpp-v2p0.c
 *
 * Driver for KY X1 Camera Post Process v2.0
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include "regs-cpp-v2p0.h"
#include "regs-fbc-v2p0.h"
#include "x1_cpp.h"
#include "cpp_dmabuf.h"
#include "cam_dbg.h"

#undef CAM_MODULE_TAG
#define CAM_MODULE_TAG CAM_MDL_CPP

#ifdef CONFIG_KY_FPGA
#define CPP_RESET_TIMEOUT_MS (1000)
#else
#define CPP_RESET_TIMEOUT_MS (500)
#endif

static void cpp20_3dnr_src_dmad_cfg(struct cpp_device *cpp_dev,
				    uint64_t yll0_dmad, uint64_t yll1_dmad,
				    uint64_t yll2_dmad, uint64_t yll3_dmad,
				    uint64_t yll4_dmad, uint64_t uvll0_dmad,
				    uint64_t uvll1_dmad, uint64_t uvll2_dmad,
				    uint64_t uvll3_dmad, uint64_t uvll4_dmad)
{
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L4, lower_32_bits(yll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L3, lower_32_bits(yll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L2, lower_32_bits(yll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L1, lower_32_bits(yll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L0, lower_32_bits(yll0_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L4, lower_32_bits(uvll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L3, lower_32_bits(uvll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L2, lower_32_bits(uvll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L1, lower_32_bits(uvll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L0, lower_32_bits(uvll0_dmad));

	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L4_H, upper_32_bits(yll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L3_H, upper_32_bits(yll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L2_H, upper_32_bits(yll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L1_H, upper_32_bits(yll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YINPUTBASEADDR_L0_H, upper_32_bits(yll0_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L4_H, upper_32_bits(uvll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L3_H, upper_32_bits(uvll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L2_H, upper_32_bits(uvll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L1_H, upper_32_bits(uvll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVINPUTBASEADDR_L0_H, upper_32_bits(uvll0_dmad));
}

static void cpp20_3dnr_pre_dmad_cfg(struct cpp_device *cpp_dev,
				    uint64_t yll0_dmad, uint64_t yll1_dmad,
				    uint64_t yll2_dmad, uint64_t yll3_dmad,
				    uint64_t yll4_dmad, uint64_t uvll0_dmad,
				    uint64_t uvll1_dmad, uint64_t uvll2_dmad,
				    uint64_t uvll3_dmad, uint64_t uvll4_dmad)
{
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L4, lower_32_bits(yll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L3, lower_32_bits(yll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L2, lower_32_bits(yll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L1, lower_32_bits(yll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L0, lower_32_bits(yll0_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L4, lower_32_bits(uvll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L3, lower_32_bits(uvll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L2, lower_32_bits(uvll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L1, lower_32_bits(uvll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L0, lower_32_bits(uvll0_dmad));

	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L4_H, upper_32_bits(yll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L3_H, upper_32_bits(yll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L2_H, upper_32_bits(yll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L1_H, upper_32_bits(yll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L0_H, upper_32_bits(yll0_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L4_H, upper_32_bits(uvll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L3_H, upper_32_bits(uvll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L2_H, upper_32_bits(uvll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L1_H, upper_32_bits(uvll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L0_H, upper_32_bits(uvll0_dmad));
}

static void cpp20_3dnr_pre_kgain_cfg(struct cpp_device *cpp_dev,
				     uint64_t kll0_dmad, uint64_t kll1_dmad,
				     uint64_t kll2_dmad, uint64_t kll3_dmad,
				     uint64_t kll4_dmad)
{
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L4, lower_32_bits(kll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L3, lower_32_bits(kll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L2, lower_32_bits(kll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L1, lower_32_bits(kll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L0, lower_32_bits(kll0_dmad));

	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L4_H, upper_32_bits(kll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L3_H, upper_32_bits(kll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L2_H, upper_32_bits(kll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L1_H, upper_32_bits(kll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L0_H, upper_32_bits(kll0_dmad));
}

static void cpp20_3dnr_out_dmad_cfg(struct cpp_device *cpp_dev,
				    uint64_t yll0_dmad, uint64_t yll1_dmad,
				    uint64_t yll2_dmad, uint64_t yll3_dmad,
				    uint64_t yll4_dmad, uint64_t uvll0_dmad,
				    uint64_t uvll1_dmad, uint64_t uvll2_dmad,
				    uint64_t uvll3_dmad, uint64_t uvll4_dmad)
{
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L4, lower_32_bits(yll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L3, lower_32_bits(yll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L2, lower_32_bits(yll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L1, lower_32_bits(yll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L0, lower_32_bits(yll0_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L4, lower_32_bits(uvll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L3, lower_32_bits(uvll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L2, lower_32_bits(uvll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L1, lower_32_bits(uvll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L0, lower_32_bits(uvll0_dmad));

	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L4_H, upper_32_bits(yll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L3_H, upper_32_bits(yll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L2_H, upper_32_bits(yll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L1_H, upper_32_bits(yll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_YWBASEADDR_L0_H, upper_32_bits(yll0_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L4_H, upper_32_bits(uvll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L3_H, upper_32_bits(uvll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L2_H, upper_32_bits(uvll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L1_H, upper_32_bits(uvll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_UVWBASEADDR_L0_H, upper_32_bits(uvll0_dmad));
}

static void cpp20_3dnr_out_kgain_cfg(struct cpp_device *cpp_dev,
				     uint64_t kll0_dmad, uint64_t kll1_dmad,
				     uint64_t kll2_dmad, uint64_t kll3_dmad,
				     uint64_t kll4_dmad)
{
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L4, lower_32_bits(kll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L3, lower_32_bits(kll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L2, lower_32_bits(kll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L1, lower_32_bits(kll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L0, lower_32_bits(kll0_dmad));

	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L4_H, upper_32_bits(kll4_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L3_H, upper_32_bits(kll3_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L2_H, upper_32_bits(kll2_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L1_H, upper_32_bits(kll1_dmad));
	cpp_reg_write(cpp_dev, REG_CPP_KWBASEADDR_L0_H, upper_32_bits(kll0_dmad));
}

static void cpp20_tnrdec_dmad_cfg(struct cpp_device *cpp_dev, uint64_t dmad)
{
	cpp_reg_write(cpp_dev, REG_FBC_TNRDEC_HL_ADDR, lower_32_bits(dmad));
	cpp_reg_write(cpp_dev, REG_FBC_TNRDEC_HH_ADDR, upper_32_bits(dmad));
}

static void cpp20_tnrenc_dmad_cfg(struct cpp_device *cpp_dev, uint64_t dmad0,
				  uint64_t dmad1)
{
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_HL_ADDR, lower_32_bits(dmad0));
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_HH_ADDR, upper_32_bits(dmad0));
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_PL_ADDR, lower_32_bits(dmad1));
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_PH_ADDR, upper_32_bits(dmad1));
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_Y_ADDR, 0x00000000);
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_C_ADDR, 0x10000000);
}

static int cpp_global_reset(struct cpp_device *cpp_dev)
{
	unsigned long time;

	reinit_completion(&cpp_dev->reset_complete);
	cpp_dev->state = CPP_STATE_RST;

	cpp_reg_set_bit(cpp_dev, REG_CPP_IRQ_MASK, QIRQ_STAT_GLB_RST_DONE);
	cpp_reg_write_mask(cpp_dev, REG_CPP_QUEUE_CTRL, (0x10 << 24),
			   QCTRL_GLB_RST_CYC_MASK);
	cpp_reg_set_bit(cpp_dev, REG_CPP_QUEUE_CTRL, QCTRL_GLB_RST);

	time = wait_for_completion_timeout(&cpp_dev->reset_complete,
					   msecs_to_jiffies(CPP_RESET_TIMEOUT_MS));
	if (!time) {
		cam_err("reset timeout waiting %dms", CPP_RESET_TIMEOUT_MS);
		return -EIO;
	}

	return 0;
}

static uint32_t cpp20_get_hw_version(struct cpp_device *cpp_dev)
{
	uint32_t chipId, hwVersion;

	chipId = cpp_reg_read(cpp_dev, REG_CPP_CHIP_ID);
	if (chipId == 0x69950) {
		hwVersion = CPP_HW_VERSION_2_0;
	} else if (0x699a0) {
		hwVersion = CPP_HW_VERSION_2_1;
	} else {
		pr_err("%s: invalid chip id 0x%x\n", __func__, chipId);
		hwVersion = 0;
	}

	return hwVersion;
}

static void cpp_enable_clk_gating(struct cpp_device *cpp_dev, u8 enable)
{
	if (enable) {
		cpp_reg_set_bit(cpp_dev, REG_FBC_TNRDEC_CG_EN, 0x1);
		cpp_reg_set_bit(cpp_dev, REG_CPP_3DNR_CG_CTRL, 0x3);
		cpp_reg_set_bit(cpp_dev, REG_CPP_QUEUE_CTRL, 0x70000);
	} else {
		cpp_reg_clr_bit(cpp_dev, REG_FBC_TNRDEC_CG_EN, 0x1);
		cpp_reg_clr_bit(cpp_dev, REG_CPP_3DNR_CG_CTRL, 0x3);
		cpp_reg_clr_bit(cpp_dev, REG_CPP_QUEUE_CTRL, 0x70000);
	}
}

/**
 * val: 0-64byte 1-128byte 2-256byte 3-512byte 4-1024byte
 */
static void set_3dnr_rd_burst(struct cpp_device *cpp, u8 val)
{
	val &= 0xff;
	cpp_reg_write_mask(cpp, REG_CPP_3DNR_BST_LEN, val << 8, 0xff00);
}

/**
 * no more than 0x40 on z1, or hardware hang when axi slow
 * fixed on asic_a0

 * val: burst length=val*16 byte
 */
static void set_3dnr_wr_burst(struct cpp_device *cpp, u8 val)
{
	val &= 0xff;
	cpp_reg_write_mask(cpp, REG_CPP_3DNR_BST_LEN, val, 0xff);
}

static void set_fbc_dec_burst(struct cpp_device *cpp, u8 val)
{
	val &= 0xf;
	cpp_reg_write_mask(cpp, REG_FBC_TNRDEC_PERF_CTRL, val << 4, 0xf0);
}

static void set_fbc_enc_burst(struct cpp_device *cpp, u8 val)
{
	val &= 0x7f;
	cpp_reg_write_mask(cpp, REG_FBC_TNRENC_DMAC_LENGTH, val, 0x7f);
}

static int cpp_set_burst_len(struct cpp_device *cpp)
{
	set_3dnr_wr_burst(cpp, 0x10);
	set_3dnr_rd_burst(cpp, 0x2);
	set_fbc_dec_burst(cpp, 0x7);
	set_fbc_enc_burst(cpp, 0x7);

	return 0;
}

static void cpp_enable_irqs_common(struct cpp_device *cpp_dev, u8 enable)
{
	if (enable) {
		cpp_reg_set_bit(cpp_dev, REG_CPP_IRQ_MASK, QIRQ_MASK_GEN);
		cpp_reg_set_bit(cpp_dev, REG_FBC_TNRDEC_IRQ_MASK,
				FIRQ_MASK_DEC_GEN);
		cpp_reg_set_bit(cpp_dev, REG_FBC_TNRENC_IRQ_MASK,
				FIRQ_MASK_ENC_GEN);
		cpp_dev->mmu_dev->ops->setup_timeout_address(cpp_dev->mmu_dev);
	} else {
		cpp_reg_clr_bit(cpp_dev, REG_FBC_TNRENC_IRQ_MASK,
				FIRQ_MASK_ENC_GEN);
		cpp_reg_clr_bit(cpp_dev, REG_FBC_TNRDEC_IRQ_MASK,
				FIRQ_MASK_DEC_GEN);
		cpp_reg_clr_bit(cpp_dev, REG_CPP_IRQ_MASK, QIRQ_MASK_GEN);
	}
}

__maybe_unused static void iommu_isr(struct cpp_device *cpp_dev)
{
	u32 iommu_status;

	iommu_status = cpp_reg_read(cpp_dev, 0x1010);
	cpp_reg_write(cpp_dev, 0x1010, iommu_status);
	cam_err("cpp iommu irq status: 0x%08x", iommu_status);
}

static void cpp_isr_err_process(struct cpp_device *cpp_dev, u32 irq_status,
				u32 iommu_status)
{
	if (irq_status & QIRQ_MASK_ERR) {
		cpp_dev->state = CPP_STATE_ERR;
		cam_err("irq err status: 0x%08x", irq_status);
	}

	if (iommu_status) {
		cpp_dev->state = CPP_STATE_ERR;
		cam_err("iommu irq status: 0x%08x", iommu_status);
	}
}

static void cpp20_fbc_dec_handler(struct cpp_device *cpp_dev)
{
	uint32_t fbcDecStat;

	/*
	 * static const char *const dec_irq_msg[] = {
	 *     "decode_eof",  "cfg_swaped",   "hdr_rdma", "pld_rdma",
	 *     "core_eof",    "wlbuf_eof",    "hdr_err",  "payload_err",
	 *     "slv_req_err", "rdma_timeout", "dmac_err",
	 * };
	 */
	fbcDecStat = cpp_reg_read(cpp_dev, REG_FBC_TNRDEC_IRQ_STATUS);
	cpp_reg_write(cpp_dev, REG_FBC_TNRDEC_IRQ_STATUS, fbcDecStat);

	if (fbcDecStat & FIRQ_MASK_DEC_ERR)
		cam_err("tnrdec: 0x%x = 0x%08x", REG_FBC_TNRDEC_IRQ_STATUS, fbcDecStat);
}

static void cpp20_fbc_enc_handler(struct cpp_device *cpp_dev)
{
	uint32_t fbcEncStat;

	fbcEncStat = cpp_reg_read(cpp_dev, REG_FBC_TNRENC_IRQ_STATUS);
	cpp_reg_write(cpp_dev, REG_FBC_TNRENC_IRQ_RAW, fbcEncStat);

	if (fbcEncStat & FIRQ_MASK_ENC_ERR)
		cam_err("tnrenc: 0x%x = 0x%08x", REG_FBC_TNRENC_IRQ_STATUS, fbcEncStat);

	if (fbcEncStat & BIT(16))
		cam_dbg("tnrenc: dma_wr_eof");

	if (fbcEncStat & BIT(17))
		cam_dbg("tnrenc: cfg_update_done");
}

static irqreturn_t cpp_isr(int irq, void *data)
{
	struct cpp_device *cpp_dev = (struct cpp_device *)data;
	u32 irq_status, iommu_status = 0;
	int i;

	static const char *const cpp_irq_msg[] = {
		"frame_done", "slice_done",
		"slice_sof", "top_ctrl_out_done",
		"rdma_done", "dmac_werr",
		"dmac_rerr", "wdma_timeout",
		"rdma_timeout", "global_reset_done",
		"afbc_dec0", "afbc_dec1",
		"afbc_enc", "iommu",
		"3dnr_eof", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
		"rsvd", "rsvd",
	};

	irq_status = cpp_reg_read(cpp_dev, REG_CPP_IRQ_STATUS);

	if (irq_status & QIRQ_MASK_IOMMU) {
		iommu_status = cpp_reg_read(cpp_dev, 0x1010);
		cpp_reg_write(cpp_dev, 0x1010, iommu_status);
	}

	/* clear 2nd level irq before 1st */
	if (irq_status & QIRQ_STAT_FBC_DEC0)
		cpp20_fbc_dec_handler(cpp_dev);
	if (irq_status & QIRQ_STAT_FBC_ENC)
		cpp20_fbc_enc_handler(cpp_dev);
	cpp_reg_write(cpp_dev, REG_CPP_IRQ_STATUS, irq_status);

	cpp_isr_err_process(cpp_dev, irq_status, iommu_status);

	if (irq_status & QIRQ_STAT_GLB_RST_DONE) {
		complete(&cpp_dev->reset_complete);
		cam_dbg("global reset done");
		return IRQ_HANDLED;
	}

	if (irq_status & QIRQ_STAT_FRM_DONE || cpp_dev->state == CPP_STATE_ERR)
		complete(&cpp_dev->run_work.run_complete);

	for (i = 0; i < 32; i++)
		if (irq_status & (1 << i))
			cam_dbg("isr %s", cpp_irq_msg[i]);

	return IRQ_HANDLED;
}

static int cpp20_3dnr_dmad_cfg(struct cpp_device *cpp_dev,
			       struct cpp_dma_port_info *port_info, u8 port_id)
{
	if (port_info == NULL || port_id >= MAX_DMA_PORT) {
		cam_err("%s: invalid port_info %p, port_id %d", __func__,
			port_info, port_id);
		return -EINVAL;
	}

	if (port_id == MAC_DMA_PORT_R0 && port_info->fbc_enabled == true) {
		cam_err("fbc is not supported on MAC_DMA_PORT_R0");
		return -1;
	}

	if (port_id == MAC_DMA_PORT_R0) {
		cpp20_3dnr_src_dmad_cfg(cpp_dev,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L0].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L1].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L2].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L3].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L4].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L0].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L1].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L2].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L3].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L4].phy_addr);
	} else if (port_id == MAC_DMA_PORT_R1) {
		cpp20_3dnr_pre_dmad_cfg(cpp_dev,
					port_info->fbc_enabled == true ? 0x200000000 : port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L0].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L1].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L2].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L3].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L4].phy_addr,
					port_info->fbc_enabled == true ? 0x210000000 : port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L0].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L1].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L2].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L3].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L4].phy_addr);

		if (port_info->fbc_enabled == true)
			cpp20_tnrdec_dmad_cfg(cpp_dev,
					      port_info->dma_chnls[MAC_DMA_CHNL_FBC_HEADER].phy_addr);

		cpp20_3dnr_pre_kgain_cfg(cpp_dev,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L0].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L1].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L2].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L3].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L4].phy_addr);
	} else if (port_id == MAC_DMA_PORT_W0) {
		cpp20_3dnr_out_dmad_cfg(cpp_dev,
					port_info->fbc_enabled == true ? 0x200000000 : port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L0].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L1].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L2].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L3].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_Y_L4].phy_addr,
					port_info->fbc_enabled == true ? 0x210000000 : port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L0].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L1].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L2].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L3].phy_addr,
					port_info->dma_chnls[MAC_DMA_CHNL_DWT_C_L4].phy_addr);

		if (port_info->fbc_enabled == true)
			cpp20_tnrenc_dmad_cfg(cpp_dev,
					      port_info->dma_chnls[MAC_DMA_CHNL_FBC_HEADER].phy_addr,
					      port_info->dma_chnls[MAC_DMA_CHNL_FBC_HEADER].phy_addr +
					      port_info->dma_chnls[MAC_DMA_CHNL_FBC_PAYLOAD].offset -
					      port_info->dma_chnls[MAC_DMA_CHNL_FBC_HEADER].offset);

		cpp20_3dnr_out_kgain_cfg(cpp_dev,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L0].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L1].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L2].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L3].phy_addr,
					 port_info->dma_chnls[MAC_DMA_CHNL_KGAIN_L4].phy_addr);
	} else {
		pr_err("%s: invalid dma port id %d\n", __func__, port_id);
		return -EINVAL;
	}

	return 0;
}

static void cpp20_debug_dump(struct cpp_device *cpp_dev)
{
	cam_info("0x3f8 : 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x",
		 cpp_reg_read(cpp_dev, 0x3f8), cpp_reg_read(cpp_dev, 0x3fc),
		 cpp_reg_read(cpp_dev, 0x400), cpp_reg_read(cpp_dev, 0x404),
		 cpp_reg_read(cpp_dev, 0x408), cpp_reg_read(cpp_dev, 0x40c));
	cam_info("0x7000: 0x%08x 0x%08x 0x%08x 0x%08x",
		 cpp_reg_read(cpp_dev, 0x7000), cpp_reg_read(cpp_dev, 0x7004),
		 cpp_reg_read(cpp_dev, 0x7008), cpp_reg_read(cpp_dev, 0x700c));
	cam_info("src-y: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_YINPUTBASEADDR_L0, REG_CPP_YINPUTBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YINPUTBASEADDR_L1, REG_CPP_YINPUTBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YINPUTBASEADDR_L2, REG_CPP_YINPUTBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YINPUTBASEADDR_L3, REG_CPP_YINPUTBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YINPUTBASEADDR_L4, REG_CPP_YINPUTBASEADDR_L4_H));
	cam_info("src-c: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_UVINPUTBASEADDR_L0, REG_CPP_UVINPUTBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVINPUTBASEADDR_L1, REG_CPP_UVINPUTBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVINPUTBASEADDR_L2, REG_CPP_UVINPUTBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVINPUTBASEADDR_L3, REG_CPP_UVINPUTBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVINPUTBASEADDR_L4,REG_CPP_UVINPUTBASEADDR_L4_H));
	cam_info("pre-y: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L0, REG_CPP_PRE_YINPUTBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L1, REG_CPP_PRE_YINPUTBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L2, REG_CPP_PRE_YINPUTBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L3, REG_CPP_PRE_YINPUTBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_YINPUTBASEADDR_L4, REG_CPP_PRE_YINPUTBASEADDR_L4_H));
	cam_info("pre-c: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L0, REG_CPP_PRE_UVINPUTBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L1, REG_CPP_PRE_UVINPUTBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L2, REG_CPP_PRE_UVINPUTBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L3, REG_CPP_PRE_UVINPUTBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_UVINPUTBASEADDR_L4, REG_CPP_PRE_UVINPUTBASEADDR_L4_H));
	cam_info("pre-k: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L0, REG_CPP_PRE_KINPUTBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L1, REG_CPP_PRE_KINPUTBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L2, REG_CPP_PRE_KINPUTBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L3, REG_CPP_PRE_KINPUTBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_PRE_KINPUTBASEADDR_L4, REG_CPP_PRE_KINPUTBASEADDR_L4_H));
	cam_info("dst-y: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_YWBASEADDR_L0, REG_CPP_YWBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YWBASEADDR_L1, REG_CPP_YWBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YWBASEADDR_L2, REG_CPP_YWBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YWBASEADDR_L3, REG_CPP_YWBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_YWBASEADDR_L4, REG_CPP_YWBASEADDR_L4_H));
	cam_info("dst-c: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_UVWBASEADDR_L0, REG_CPP_UVWBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVWBASEADDR_L1, REG_CPP_UVWBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVWBASEADDR_L2, REG_CPP_UVWBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVWBASEADDR_L3, REG_CPP_UVWBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_UVWBASEADDR_L4, REG_CPP_UVWBASEADDR_L4_H));
	cam_info("dst-k: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_CPP_KWBASEADDR_L0, REG_CPP_KWBASEADDR_L0_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_KWBASEADDR_L1, REG_CPP_KWBASEADDR_L1_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_KWBASEADDR_L2, REG_CPP_KWBASEADDR_L2_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_KWBASEADDR_L3, REG_CPP_KWBASEADDR_L3_H),
		 cpp_reg_read64(cpp_dev, REG_CPP_KWBASEADDR_L4, REG_CPP_KWBASEADDR_L4_H));
	cam_info("tnrdec: 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_FBC_TNRDEC_HL_ADDR, REG_FBC_TNRDEC_HH_ADDR));
	cam_info("tnrenc: 0x%llx 0x%llx",
		 cpp_reg_read64(cpp_dev, REG_FBC_TNRENC_HL_ADDR, REG_FBC_TNRENC_HH_ADDR),
		 cpp_reg_read64(cpp_dev, REG_FBC_TNRENC_PL_ADDR, REG_FBC_TNRENC_PH_ADDR));
}

const struct cpp_hw_ops cpp_ops_2_0 = {
	.global_reset = cpp_global_reset,
	.enable_clk_gating = cpp_enable_clk_gating,
	.set_burst_len = cpp_set_burst_len,
	.enable_irqs_common = cpp_enable_irqs_common,
	.isr = cpp_isr,
	.debug_dump = cpp20_debug_dump,
	.cfg_port_dmad = cpp20_3dnr_dmad_cfg,
	.hw_version = cpp20_get_hw_version,
};

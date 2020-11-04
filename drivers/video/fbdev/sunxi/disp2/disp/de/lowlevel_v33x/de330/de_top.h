/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_TOP_H_
#define _DE_TOP_H_

#include <linux/types.h>

#define DE_TOP_REG_OFFSET (0x8000)
#define DE_TOP_REG_SIZE   (0x0220)

#define DE_TOP_RTWB_OFFSET (0x010000)
#define DE_TOP_RTMX_OFFSET (0x100000)

#define DE_CHN_SIZE            (0x20000) /* 128K */
#define DE_CHN_OFFSET(phy_chn) (0x100000 + DE_CHN_SIZE * (phy_chn))

#define DE_DISP_SIZE           (0x20000) /* 128K */
#define DE_DISP_OFFSET(disp)   (0x280000 + DE_DISP_SIZE * (disp))

#define CHN_CCSC_OFFSET    (0x00800)
#define CHN_OVL_OFFSET     (0x01000)
#define CHN_DBV_OFFSET     (0x02000)
#define CHN_SCALER_OFFSET  (0x04000)
#define CHN_FBD_ATW_OFFSET (0x05000)
#define CHN_CDC_OFFSET     (0x08000)
#define CHN_FCE_OFFSET     (0x10000)
#define CHN_PEAK_OFFSET    (0x10800)
#define CHN_LTI_OFFSET     (0x10C00)
#define CHN_BLS_OFFSET     (0x11000)
#define CHN_FCC_OFFSET     (0x11400)
#define CHN_DNS_OFFSET     (0x14000)
#define CHN_DI300_OFFSET   (0x14400)
#define CHN_SNR_OFFSET     (0x14000)

#define DISP_BLD_OFFSET    (0x01000)
#define DISP_DEP_OFFSET    (0x02000)
#define DISP_DB3_OFFSET    (0x03000)
#define DISP_FMT_OFFSET    (0x05000)
#define DISP_DSC_OFFSET    (0x06000)
#define DISP_KSC_OFFSET    (0x08000)

#define RTWB_WB_OFFSET     (0x01000)
#define RTWB_CDC_OFFSET    (0x08000)

enum de_rtwb_mode {
	TIMING_FROM_TCON = 0,
	SELF_GENERATED_TIMING = 1,
};

enum de_clk_id {
	DE_CLK_NONE = 0,
	DE_CLK_CORE0 = 1,
	DE_CLK_CORE1 = 2,
	DE_CLK_CORE2 = 3,
	DE_CLK_CORE3 = 4,
	DE_CLK_WB = 5,
};

enum de_rtwb_mux_id {
	RTWB_MUX_FROM_BLENDER0 = 0,
	RTWB_MUX_FROM_DSC0     = 1,
	RTWB_MUX_FROM_BLENDER1 = 2,
	RTWB_MUX_FROM_DSC1     = 3,
	RTWB_MUX_FROM_BLENDER2 = 4,
	RTWB_MUX_FROM_DSC2     = 5,
	RTWB_MUX_FROM_BLENDER3 = 6,
	RTWB_MUX_FROM_DSC3     = 7,
};

enum de_irq_flag {
	DE_IRQ_FLAG_FRAME_END  = 0x1 << 4,
	DE_IRQ_FLAG_ERROR      = 0,
	DE_IRQ_FLAG_RCQ_FINISH = 0x1 << 6,
	DE_IRQ_FLAG_RCQ_ACCEPT = 0x1 << 7,
	DE_IRQ_FLAG_MASK =
		DE_IRQ_FLAG_FRAME_END
		| DE_IRQ_FLAG_ERROR
		| DE_IRQ_FLAG_RCQ_FINISH
		| DE_IRQ_FLAG_RCQ_ACCEPT,
};

enum de_irq_state {
	DE_IRQ_STATE_FRAME_END  = 0x1 << 0,
	DE_IRQ_STATE_ERROR      = 0,
	DE_IRQ_STATE_RCQ_FINISH = 0x1 << 2,
	DE_IRQ_STATE_RCQ_ACCEPT = 0x1 << 3,
	DE_IRQ_STATE_MASK =
		DE_IRQ_STATE_FRAME_END
		| DE_IRQ_STATE_ERROR
		| DE_IRQ_STATE_RCQ_FINISH
		| DE_IRQ_STATE_RCQ_ACCEPT,
};

enum de_wb_irq_flag {
	DE_WB_IRQ_FLAG_RCQ_FINISH = 0x1 << 6,
	DE_WB_IRQ_FLAG_RCQ_ACCEPT = 0x1 << 7,
	DE_WB_IRQ_FLAG_MASK =
		DE_WB_IRQ_FLAG_RCQ_FINISH
		| DE_WB_IRQ_FLAG_RCQ_ACCEPT,
};

enum de_wb_irq_state {
	DE_WB_IRQ_STATE_RCQ_FINISH = 0x1 << 2,
	DE_WB_IRQ_STATE_RCQ_ACCEPT = 0x1 << 3,
	DE_WB_IRQ_STATE_MASK =
		DE_WB_IRQ_STATE_RCQ_FINISH
		| DE_WB_IRQ_STATE_RCQ_ACCEPT,
};


uintptr_t de_top_get_reg_base(void);
void de_top_set_reg_base(u8 __iomem *reg_base);

s32 de_top_set_clk_enable(u32 clk_no, u8 en);

s32 de_top_set_de2tcon_mux(u32 disp, u32 tcon);
u32 de_top_get_tcon_from_mux(u32 disp);

u32 de_top_get_ip_version(void);

s32 de_top_set_rtwb_mux(u32 wb_id, enum de_rtwb_mux_id id);
enum de_rtwb_mux_id de_top_get_rtwb_mux(u32 wb_id);

s32 de_top_set_vchn2core_mux(u32 phy_chn, u32 phy_disp);
s32 de_top_set_uchn2core_mux(u32 phy_chn, u32 phy_disp);
s32 de_top_set_port2vchn_mux(u32 phy_disp, u32 port, u32 phy_chn);
s32 de_top_set_port2uchn_mux(u32 phy_disp, u32 port, u32 phy_chn);

u32 de_top_get_ahb_config_flag(void);
s32 de_top_set_lut_debug_enable(u8 en);

s32 de_top_set_rtmx_enable(u32 disp, u8 en);
s32 de_top_enable_irq(u32 disp, u32 irq_flag, u32 en);
u32 de_top_query_state_with_clear(u32 disp, u32 irq_state);
s32 de_top_set_out_size(u32 disp, u32 width, u32 height);
s32 de_top_get_out_size(u32 disp, u32 *width, u32 *height);
s32 de_top_set_rcq_update(u32 disp, u32 update);
s32 de_top_set_rcq_head(u32 disp, u64 addr, u32 len);

s32 de_top_wb_enable_irq(u32 wb, u32 irq_flag, u32 en);
u32 de_top_wb_query_state_with_clear(u32 wb, u32 irq_state);
s32 de_top_wb_set_rcq_update(u32 wb, u32 update);
s32 de_top_wb_set_rcq_head(u32 wb, u64 addr, u32 len);


extern void *disp_malloc(u32 num_bytes, void *phys_addr);
extern void disp_free(void *virt_addr, void *phys_addr, u32 num_bytes);
void *de_top_reg_memory_alloc(
	u32 size, void *phy_addr, u32 rcq_used);
void de_top_reg_memory_free(
	void *virt_addr, void *phys_addr, u32 num_bytes);
s32 de_top_mem_pool_alloc(void);
s32 de_top_start_rtwb(u32 wb_id, u32 en);
s32 de_top_set_rtwb_mode(u32 wb_id, enum de_rtwb_mode mode);
void de_top_mem_pool_free(void);

#endif /* #ifndef _DE_TOP_H_ */

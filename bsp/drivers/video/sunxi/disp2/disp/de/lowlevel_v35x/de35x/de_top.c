/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "../../include.h"
#include "de_top.h"
#include "de_rtmx.h"

#if defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN60IW1)
#define DE_MBUS_CLOCK_ADDR           (0x8008)
#define DE2TCON_MUX_OFFSET           (0x8010)
#define DE_VER_CTL_OFFSET            (0x8014)
#define DE_RTWB_CTL_OFFSET           (0x8020)
#define DE_RTWB_MUX_OFFSET           (0x8020)
#define DE_VCH2CORE_MUX_OFFSET       (0x8024) /* CHN_MUX */
#define DE_UCH2CORE_MUX_OFFSET       (0x8024) /* CHN_MUX */
#define DE_PORT2CHN_MUX_OFFSET(disp) (0x8028 + (disp) * 0x4)
#define DE_ASYNC_BRIDGE_OFFSET       (0x804c)
#define DE_BUF_DEPTH_OFFSET(disp)    (0x8050 + (disp) * 0x4)
#define DE_DEBUG_CTL_OFFSET          (0x80E0)
#define RTMX_GLB_CTL_OFFSET(disp)    (0x8100 + (disp) * 0x40)
#define RTMX_GLB_STS_OFFSET(disp)    (0x8104 + (disp) * 0x40)
#define RTMX_OUT_SIZE_OFFSET(disp)   (0x8108 + (disp) * 0x40)
#define RTMX_AUTO_CLK_OFFSET(disp)   (0x810C + (disp) * 0x40)
#define RTMX_RCQ_CTL_OFFSET(disp)    (0x8110 + (disp) * 0x40)

#define RTWB_RCQ_IRQ_OFFSET          (0x8200)
#define RTWB_RCQ_STS_OFFSET          (0x8204)
#define RTWB_RCQ_CTL_OFFSET          (0x8210)

#define DE_AHB_RESET_SHIFT           (0)
#define DE_MODULE_CLK_SHIFT          (0)
#define DE_RTWB_START_SHIFT          (4)
#define DE_RTWB_MODE_SHIFT           (5)
#define DE_VCH2CORE_MUX_SHIFT        (1)
#define DE_UCH2CORE_MUX_SHIFT        (1)
#define DE_UCH2CORE_MUX_SHIFT_OFFSET (16)

#elif defined(CONFIG_ARCH_SUN60IW2)
#define DE_MBUS_CLOCK_ADDR           (0x8008)
#define DE_RESERVE_CTL_OFFSET        (0x800C)
#define DE2TCON_MUX_OFFSET           (0x8010)
#define DE_VER_CTL_OFFSET            (0x8014)
#define DE_RTWB_CTL_OFFSET           (0x8020)
#define DE_RTWB_MUX_OFFSET           (0x8024)
#define DE_VCH2CORE_MUX_OFFSET       (0x8028)
#define DE_UCH2CORE_MUX_OFFSET       (0x802C)
#define DE_PORT2CHN_MUX_OFFSET(disp) (0x8030 + (disp) * 0x4)
#define DE_ASYNC_BRIDGE_OFFSET       (0x804C)
#define DE_BUF_DEPTH_OFFSET(disp)    (0x8050 + (disp) * 0x4)
#define DE_OFFLINE_CTL_OFFSET        (0x80D0)
#define DE_OFFLINE_OUTPUT_OFFSET     (0x80D4)
#define DE_OFFLINE_HIGH_ADDR_OFFSET  (0x80D8)
#define DE_OFFLINE_LOW_ADDR_OFFSET   (0x80DC)
#define DE_DEBUG_CTL_OFFSET          (0x80E0)
#define DE_AHB_TIMEOUT_OFFSET        (0x80E4)
#define DE_GATING_CTL_OFFSET         (0x80E8)
#define RTMX_GLB_CTL_OFFSET(disp)    (0x8100 + (disp) * 0x40)
#define RTMX_GLB_STS_OFFSET(disp)    (0x8104 + (disp) * 0x40)
#define RTMX_OUT_SIZE_OFFSET(disp)   (0x8108 + (disp) * 0x40)
#define RTMX_AUTO_CLK_OFFSET(disp)   (0x810C + (disp) * 0x40)
#define RTMX_RCQ_CTL_OFFSET(disp)    (0x8110 + (disp) * 0x40)

#define RTWB_RCQ_IRQ_OFFSET          (0x8200)
#define RTWB_RCQ_STS_OFFSET          (0x8204)
#define RTWB_RCQ_CTL_OFFSET          (0x8210)

#define DE_AHB_RESET_SHIFT           (2)
#define DE_MODULE_CLK_SHIFT          (2)
#define DE_RTWB_START_SHIFT          (0)
#define DE_RTWB_MODE_SHIFT           (4)
#define DE_VCH2CORE_MUX_SHIFT        (2)
#define DE_UCH2CORE_MUX_SHIFT        (2)
#define DE_UCH2CORE_MUX_SHIFT_OFFSET (0)
#endif

struct de_clk_para {
	enum de_clk_id clk_no;

	u32 ahb_reset_adr;
	u32 ahb_reset_shift;
	u32 ahb_reset_width;

	u32 mod_en_adr;
	u32 mod_en_shift;
	u32 mod_en_width;
};

static struct de_clk_para de_clk_tbl[] = {
	{
		.clk_no             = DE_CLK_CORE0,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 0 << DE_AHB_RESET_SHIFT,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 0 << DE_MODULE_CLK_SHIFT,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_CORE1,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 1 << DE_AHB_RESET_SHIFT,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 1 << DE_MODULE_CLK_SHIFT,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_CORE2,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 2 << DE_AHB_RESET_SHIFT,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 2 << DE_MODULE_CLK_SHIFT,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_CORE3,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 3 << DE_AHB_RESET_SHIFT,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 3 << DE_MODULE_CLK_SHIFT,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_WB,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 4 << DE_AHB_RESET_SHIFT,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 4 << DE_MODULE_CLK_SHIFT,
		.mod_en_width       = 1,
	},
};

static u8 __iomem *de_base;
static enum de_rtwb_mode rtwb_mode = TIMING_FROM_TCON;

uintptr_t de_top_get_reg_base(void)
{
	return (uintptr_t)de_base;
}

void de_top_set_reg_base(u8 __iomem *reg_base)
{
	de_base = reg_base;
}

static void __de_mbus_clk_enable(u8 en)
{
	u32 reg_val;
	u8 __iomem *reg_base;

	reg_base = de_base + DE_MBUS_CLOCK_ADDR;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(0, 1, reg_val, en & 0x1);
	writel(reg_val, reg_base);
}

/* set bit to 0 to reset */
#if defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN60IW1)
static void __de_mbus_reset(u8 en) {};
#elif defined(CONFIG_ARCH_SUN60IW2)
static void __de_mbus_reset(u8 en)
{
	u32 reg_val;
	u8 __iomem *reg_base;

	reg_base = de_base + DE_MBUS_CLOCK_ADDR;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(4, 1, reg_val, (en & 0x1));
	writel(reg_val, reg_base);
}
#endif

static s32 __de_clk_enable(struct de_clk_para *para, u8 en)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 val = en ? 0x1 : 0x0;

	reg_base = para->ahb_reset_adr + de_base;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(para->ahb_reset_shift, para->ahb_reset_width,
		reg_val, val);
	writel(reg_val, reg_base);

	reg_base = para->mod_en_adr + de_base;
		reg_val = readl(reg_base);
	reg_val = SET_BITS(para->mod_en_shift, para->mod_en_width,
		reg_val, val);
	writel(reg_val, reg_base);

	return 0;
}

s32 de_top_set_clk_enable(u32 clk_no, u8 en)
{
	static atomic_t mbus_ref_count;
	u32 i = 0;
	u32 size = sizeof(de_clk_tbl) / sizeof(de_clk_tbl[0]);
	s32 count = 0;

	for (i = 0; i < size; ++i) {
		struct de_clk_para *para = &(de_clk_tbl[i]);
		if (clk_no == para->clk_no)
			__de_clk_enable(para, en);
	}

	if (en)
		count = atomic_inc_return(&mbus_ref_count);
	else
		count = atomic_dec_return(&mbus_ref_count);

	if (en) {
		__de_mbus_clk_enable(1);
		__de_mbus_reset(1);
	} else if (0 == count) {
		__de_mbus_clk_enable(0);
		__de_mbus_reset(0);
	} else if (0 > count) {
		DE_WARN("mbus ref count=%d\n", count);
	}

	return 0;
}

s32 de_top_set_de2tcon_mux(u32 disp, u32 tcon)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 4;
	u32 hw_disp = de_feat_get_hw_disp(disp);
	u32 shift = hw_disp << 2;

	DE_WARN("tcon %d\n", tcon);
	reg_base = de_base + DE2TCON_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, tcon);
	writel(reg_val, reg_base);
	return 0;
}

u32 de_top_get_tcon_from_mux(u32 disp)
{
	u32 width = 2;
	u32 hw_disp = de_feat_get_hw_disp(disp);
	u32 shift = hw_disp << 2;
	u32 reg_val = readl(de_base + DE2TCON_MUX_OFFSET);

	return GET_BITS(shift, width, reg_val);
}

u32 de_top_get_ip_version(void)
{
	u32 reg_val = readl(de_base + DE_VER_CTL_OFFSET);

	return (reg_val >> 16) & 0xFFFF;
}

s32 de_top_set_rtwb_mux(u32 wb_id, enum de_rtwb_mux_id id)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 3;
	u32 shift = 0;

	reg_base = de_base + DE_RTWB_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, id);
	writel(reg_val, reg_base);
	return 0;
}

enum de_rtwb_mux_id de_top_get_rtwb_mux(u32 wb_id)
{
	u32 reg_val = readl(de_base + DE_RTWB_MUX_OFFSET);

	return (enum de_rtwb_mux_id)(reg_val & 0x7);
}

s32 de_top_set_rtwb_mode(u32 wb_id, enum de_rtwb_mode mode)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 1;
	u32 shift = DE_RTWB_MODE_SHIFT;

	reg_base = de_base + DE_RTWB_CTL_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, mode == TIMING_FROM_TCON ? 0 : 1);
	writel(reg_val, reg_base);
	rtwb_mode = mode;
	return 0;
}

enum de_rtwb_mode de_top_get_rtwb_mode(u32 wb_id)
{
	/*only disp0 support rtwb*/
	if (wb_id == 0)
		return rtwb_mode;

	return TIMING_FROM_TCON;
}

s32 de_top_start_rtwb(u32 wb_id, u32 en)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 1;
	u32 shift = DE_RTWB_START_SHIFT;

	reg_base = de_base + DE_RTWB_CTL_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, en & 0x1);
	writel(reg_val, reg_base);
	return 0;
}

#if defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN60IW1)
s32 de_top_reset_chn2core_mux(u32 phy_disp)
{
	u8 __iomem *reg_base = de_base + DE_VCH2CORE_MUX_OFFSET;
	u32 reg_val = readl(reg_base);
	if (reg_val != 0x0 || phy_disp != 0) {
		DE_WARN("not expected val=%x, disp=%d\n", reg_val, phy_disp);
		return 0;
	}
	reg_val = 0xffffffff;
	writel(reg_val, reg_base);
	return 0;
}
#elif defined(CONFIG_ARCH_SUN60IW2)
s32 de_top_reset_chn2core_mux(u32 phy_disp)
{
	u8 __iomem *reg_base1 = de_base + DE_VCH2CORE_MUX_OFFSET;
	u8 __iomem *reg_base2 = de_base + DE_UCH2CORE_MUX_OFFSET;
	u32 reg_val1 = readl(reg_base1);
	u32 reg_val2 = readl(reg_base2);
	if (reg_val1 != 0x0 || reg_val2 != 0x0 || phy_disp != 0) {
		DE_WARN("not expected val0=%x, val1=%x, disp=%d\n", reg_val1, reg_val2, phy_disp);
		return 0;
	}
	reg_val1 = 0xffffffff;
	writel(reg_val1, reg_base1);
	writel(reg_val1, reg_base2);
	return 0;
}

#endif

s32 de_top_set_vchn2core_mux(
	u32 phy_chn, u32 phy_disp)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 2;
	u32 shift = phy_chn << DE_VCH2CORE_MUX_SHIFT;
	u32 hw_disp = de_feat_get_hw_disp(phy_disp);

	reg_base = de_base + DE_VCH2CORE_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, hw_disp);
	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_set_uchn2core_mux(
	u32 phy_chn, u32 phy_disp)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 2;
	u32 shift = ((phy_chn - 6) << DE_UCH2CORE_MUX_SHIFT) + DE_UCH2CORE_MUX_SHIFT_OFFSET;
	u32 hw_disp = de_feat_get_hw_disp(phy_disp);

	reg_base = de_base + DE_UCH2CORE_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, hw_disp);
	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_set_port2vchn_mux(u32 phy_disp,
	u32 port, u32 phy_chn)
{
	u32 hw_disp = de_feat_get_hw_disp(phy_disp);
	u8 __iomem *reg_base = de_base
		+ DE_PORT2CHN_MUX_OFFSET(hw_disp);
	u32 width = 4;
	u32 shift = port << 2;
	u32 reg_val = readl(reg_base);

	reg_val = SET_BITS(shift, width, reg_val, phy_chn);
	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_set_port2uchn_mux(u32 phy_disp,
	u32 port, u32 phy_chn)
{
	u32 hw_disp = de_feat_get_hw_disp(phy_disp);
	u8 __iomem *reg_base = de_base
		+ DE_PORT2CHN_MUX_OFFSET(hw_disp);
	u32 width = 4;
	u32 shift = port << 2;
	u32 reg_val = readl(reg_base);

	reg_val = SET_BITS(shift, width, reg_val, (phy_chn + 2));
	writel(reg_val, reg_base);
	return 0;
}

u32 de_top_get_ahb_config_flag(void)
{
	u8 __iomem *reg_base = de_base + DE_DEBUG_CTL_OFFSET;
	u32 reg_val = readl(reg_base);

	writel(reg_val & 0x10, reg_base);
	return (reg_val >> 4) & 0x1;
}

s32 de_top_set_lut_debug_enable(u8 en)
{
	u8 __iomem *reg_base = de_base + DE_DEBUG_CTL_OFFSET;
	u32 reg_val = readl(reg_base);

	if (en)
		reg_val |= 0x1;
	else
		reg_val &= ~0x1;
	writel(reg_val, reg_base);
	return 0;
}

/* display */
s32 de_top_set_rtmx_enable(u32 disp, u8 en)
{
	u32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_GLB_CTL_OFFSET(hw_disp);
	u32 reg_val = readl(reg_base);
	unsigned int ic_ver = 0;

	ic_ver = sunxi_get_soc_ver();
	if (ic_ver < 2)
		writel(0x10, de_base + DE_ASYNC_BRIDGE_OFFSET);
	else
		writel(0x0, de_base + DE_ASYNC_BRIDGE_OFFSET);

#if defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN60IW1)
	if (disp == 0)
		writel(0x6000, de_base + DE_BUF_DEPTH_OFFSET(disp));
#elif defined(CONFIG_ARCH_SUN60IW2)
	writel(0x2000, de_base + DE_BUF_DEPTH_OFFSET(disp));
#endif

	if (en)
		reg_val |= 0x1;
	else
		reg_val &= ~0x1;
	writel(reg_val, reg_base);

	reg_base = de_base + RTMX_AUTO_CLK_OFFSET(hw_disp);
	if (en)
		writel(0x1, reg_base);
	else
		writel(0x0, reg_base);

	return 0;
}

s32 de_top_set_out_size(u32 disp, u32 width, u32 height)
{
	u32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_OUT_SIZE_OFFSET(hw_disp);
	u32 reg_val = ((width - 1) & 0x1FFF)
		| (((height - 1) & 0x1FFF) << 16);

	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_get_out_size(u32 disp, u32 *width, u32 *height)
{
	u32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_OUT_SIZE_OFFSET(hw_disp);
	u32 reg_val = readl(reg_base);

	if (width)
		*width = (reg_val & 0x1FFF) + 1;
	if (height)
		*height = ((reg_val >> 16) & 0x1FFF) + 1;
	return 0;
}

s32 de_top_enable_irq(u32 disp, u32 irq_flag, u32 en)
{
	s32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_GLB_CTL_OFFSET(hw_disp);
	u32 reg_val = readl(reg_base);

	if (en)
		reg_val |= irq_flag;
	else
		reg_val &= ~irq_flag;
	writel(reg_val, reg_base);
	return 0;
}

u32 de_top_query_state_with_clear(u32 disp, u32 irq_state)
{
	s32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_GLB_STS_OFFSET(hw_disp);
	u32 reg_val = readl(reg_base);
	u32 state = reg_val & irq_state & DE_IRQ_STATE_MASK;

	reg_val &= ~DE_IRQ_STATE_MASK;
	reg_val |= state;
	writel(reg_val, reg_base); /* w1c */

	return state;
}

u32 de_top_query_state_is_busy(u32 disp)
{
	s32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_GLB_STS_OFFSET(hw_disp);
	u32 reg_val = readl(reg_base);
	u32 state = reg_val & 0x10;

	return state;
}

s32 de_top_set_rcq_update(u32 disp, u32 update)
{
	s32 hw_disp = de_feat_get_hw_disp(disp);
	update = update ? 1 : 0;
	writel(update, de_base + RTMX_RCQ_CTL_OFFSET(hw_disp));
	return 0;
}

s32 de_top_set_rcq_head(u32 disp, u64 addr, u32 len)
{
	s32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base = de_base + RTMX_RCQ_CTL_OFFSET(hw_disp);
	u32 haddr = (u32)(addr >> 32);

	writel((u32)addr, reg_base + 0x4);
	writel(haddr, reg_base + 0x8);
	writel(len, reg_base + 0xC);
	return 0;
}

s32 de_top_wb_enable_irq(u32 wb, u32 irq_flag, u32 en)
{
	u8 __iomem *reg_base = de_base + RTWB_RCQ_IRQ_OFFSET;
	u32 reg_val = readl(reg_base);

	if (en)
		reg_val |= irq_flag;
	else
		reg_val &= ~irq_flag;
	writel(reg_val, reg_base);
	return 0;
}

u32 de_top_wb_query_state_with_clear(u32 wb, u32 irq_state)
{
	u8 __iomem *reg_base = de_base + RTWB_RCQ_STS_OFFSET;
	u32 reg_val = readl(reg_base);
	u32 state = reg_val & irq_state & DE_WB_IRQ_STATE_MASK;

	reg_val &= ~DE_WB_IRQ_STATE_MASK;
	reg_val |= state;
	writel(reg_val, reg_base); /* w1c */
#if IS_ENABLED(CONFIG_ARCH_SUN50IW5T)
	writel(reg_val, (u8 __iomem *)(de_base + 0x81c4)); /* w1c */
#endif

	return state;
}

s32 de_top_wb_set_rcq_update(u32 wb, u32 update)
{
	update = update ? 1 : 0;
	writel(update, de_base + RTWB_RCQ_CTL_OFFSET);
	return 0;
}

s32 de_top_wb_set_rcq_head(u32 wb, u64 addr, u32 len)
{
	u8 __iomem *reg_base = de_base + RTWB_RCQ_CTL_OFFSET;
	u32 haddr = (u8)(addr >> 32);

	writel((u32)addr, reg_base + 0x4);
	writel(haddr, reg_base + 0x8);
	writel(len, reg_base + 0xC);

	return 0;
}

void *de_rcq_vir_addr;
void *de_rcq_vir_addr_orig;
dma_addr_t de_rcq_phy_addr;

void *de_offline_vir_addr;
dma_addr_t de_offline_phy_addr;
s32 offline_buf_size;

#define DE_BYTE_ALIGN(x) (((x + (32 - 1)) >> 5) << 5)
/*#define DE_BYTE_ALIGN(x) (((x + (4*1024 - 1)) >> 12) << 12)*/
#define DE_BLOCK_SIZE 256000


s32 de_top_mem_pool_alloc(void)
{
	s32 ret = 0;

#if RTMX_USE_RCQ
	if (de_rcq_vir_addr_orig && de_rcq_phy_addr) {
		disp_free(de_rcq_vir_addr_orig, (void *)de_rcq_phy_addr, DE_BLOCK_SIZE);
		de_rcq_vir_addr = NULL;
		de_rcq_vir_addr_orig = NULL;
		de_rcq_phy_addr = 0;
	}
	de_rcq_vir_addr = disp_malloc(DE_BLOCK_SIZE, &de_rcq_phy_addr);
	de_rcq_vir_addr_orig = de_rcq_vir_addr;
	if (!de_rcq_vir_addr)
		ret = -1;
#endif

	return ret;
}

void de_top_mem_pool_free(void)
{
#if RTMX_USE_RCQ
	disp_free(de_rcq_vir_addr_orig, (void *)de_rcq_phy_addr, DE_BLOCK_SIZE);
#endif
}

void *de_top_reg_memory_alloc(u32 size, void *phy_addr, u32 rcq_used)
{
	void *viraddr = NULL;
	static u32 byte_used;
	if (rcq_used) {
		if (de_rcq_vir_addr && de_rcq_phy_addr) {
			*(dma_addr_t *)phy_addr = (dma_addr_t)de_rcq_phy_addr;
			viraddr = de_rcq_vir_addr;
			de_rcq_phy_addr =
			    (dma_addr_t)((u8 __iomem *)de_rcq_phy_addr +
					 DE_BYTE_ALIGN(size));
			de_rcq_vir_addr = (void *)((u8 *)de_rcq_vir_addr +
						   DE_BYTE_ALIGN(size));
			byte_used += DE_BYTE_ALIGN(size);
			if (byte_used > DE_BLOCK_SIZE) {
				DE_WARN("Malloc %d byte fail, out of total "
				       "memory!!\n",
				       DE_BYTE_ALIGN(size));
				viraddr = NULL;
				*(dma_addr_t *)phy_addr = (dma_addr_t)NULL;
			}
			return viraddr;
		} else {
			*(dma_addr_t *)phy_addr = (dma_addr_t)NULL;
			return NULL;
		}
	} else {
		if (phy_addr)
			*(u8 **)phy_addr = NULL;
		return kmalloc(size, GFP_KERNEL | __GFP_ZERO);
	}
}


void de_top_reg_memory_free(
	void *virt_addr, void *phys_addr, u32 num_bytes)
{
	if (phys_addr == NULL)
		kfree(virt_addr);
}

s32 de_top_get_pingpang_buf_size(void)
{
#if defined(RTMX_SUPPORT_OFFLINE)
	return offline_buf_size;
#endif
	return 0;
}

void *de_top_get_pingpang_vir_addr(void)
{
#if defined(RTMX_SUPPORT_OFFLINE)
	return de_offline_vir_addr;
#endif
	return NULL;
}

s32 de_top_pingpang_buf_alloc(s32 width, s32 height)
{
#if defined(RTMX_SUPPORT_OFFLINE)
	if (width <= 0 || height <= 0)
		return -1;

	if (de_offline_vir_addr && de_offline_phy_addr) {
		de_top_pingpang_buf_free();
	}

	/* 3 is enough for rgb data, 4 for maybe 10bit data */
	offline_buf_size = PAGE_ALIGN((width * height * 4 * 2));
	de_offline_vir_addr = disp_malloc(offline_buf_size, &de_offline_phy_addr);
	if (!de_offline_vir_addr || !de_offline_phy_addr) {
		DE_WARN("pingpang buf alloc err");
		return -1;
	}
#endif
	return 0;
}

void de_top_pingpang_buf_free(void)
{
#if defined(RTMX_SUPPORT_OFFLINE)
	disp_free(de_offline_vir_addr, (void *)de_offline_phy_addr, offline_buf_size);
	de_offline_vir_addr = NULL;
	de_offline_phy_addr = 0;
	offline_buf_size = 0;
#endif
}

s32 de_top_set_offline_head(void)
{
#if defined(RTMX_SUPPORT_OFFLINE)
	u8 __iomem *reg_base;
	u32 haddr = (u32)((u64)de_offline_phy_addr >> 32);

	if (!de_offline_phy_addr)
		return -1;

	reg_base = de_base + DE_OFFLINE_LOW_ADDR_OFFSET;
	writel((u32)de_offline_phy_addr, reg_base);
	reg_base = de_base + DE_OFFLINE_HIGH_ADDR_OFFSET;
	writel(haddr, reg_base);
#endif
	return 0;
}

s32 de_top_set_offline_enable(u32 en)
{
#if defined(RTMX_SUPPORT_OFFLINE)
	u8 __iomem *reg_base;
	u32 reg_val;

	reg_base = de_base + DE_OFFLINE_CTL_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(0, 1, reg_val, (en & 0x1));
	writel(reg_val, reg_base);

	reg_val = readl(reg_base);
	reg_val = SET_BITS(4, 1, reg_val, (en & 0x1));
	writel(reg_val, reg_base);
#endif
	return 0;
}

#if defined(CONFIG_ARCH_SUN55IW3) || defined(CONFIG_ARCH_SUN60IW1)
s32 de_top_set_ahb_read_mode(u32 mode) { return 0; }
s32 de_top_query_ahb_state_with_clear(u32 state) { return 0; }
s32 de_top_set_clk_gating(u32 gating_id, u32 en) { return 0; }
#elif defined(CONFIG_ARCH_SUN60IW2)
/* 0 normal mode, 1 conflict mode */
s32 de_top_set_ahb_read_mode(u32 mode)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 1;
	u32 shift = 8;

	reg_base = de_base + DE_RESERVE_CTL_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, (mode & 0x1));
	writel(reg_val, reg_base);

	return 0;
}

s32 de_top_query_ahb_state_with_clear(u32 state)
{
	u8 __iomem *reg_base = de_base + DE_RESERVE_CTL_OFFSET;
	u32 reg_val = readl(reg_base);
	u32 real_state = reg_val & state & (0x1 << 16);

	reg_val &= ~(0x1 << 16);
	reg_val |= real_state;
	writel(reg_val, reg_base); /* w1c */

	return real_state;
}

s32 de_top_set_clk_gating(u32 gating_id, u32 en)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 1;
	u32 shift = gating_id;

	reg_base = de_base + DE_GATING_CTL_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, (en & 0x1));
	writel(reg_val, reg_base);
	return 0;
}
#endif

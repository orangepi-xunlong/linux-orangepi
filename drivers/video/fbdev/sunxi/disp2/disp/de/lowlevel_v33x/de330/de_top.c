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

#define DE_MBUS_CLOCK_ADDR           (0x8008)
#define DE2TCON_MUX_OFFSET           (0x8010)
#define DE_VER_CTL_OFFSET            (0x8014)
#define DE_RTWB_MUX_OFFSET           (0x8020)
#define DE_CHN2CORE_MUX_OFFSET       (0x8024)
#define DE_PORT2CHN_MUX_OFFSET(disp) (0x8028 + (disp) * 0x4)
#define DE_DEBUG_CTL_OFFSET          (0x80E0)
#define RTMX_GLB_CTL_OFFSET(disp)    (0x8100 + (disp) * 0x40)
#define RTMX_GLB_STS_OFFSET(disp)    (0x8104 + (disp) * 0x40)
#define RTMX_OUT_SIZE_OFFSET(disp)   (0x8108 + (disp) * 0x40)
#define RTMX_AUTO_CLK_OFFSET(disp)   (0x810C + (disp) * 0x40)
#define RTMX_RCQ_CTL_OFFSET(disp)    (0x8110 + (disp) * 0x40)

#define RTWB_RCQ_IRQ_OFFSET          (0x8200)
#define RTWB_RCQ_STS_OFFSET          (0x8204)
#define RTWB_RCQ_CTL_OFFSET          (0x8210)

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
		.ahb_reset_shift    = 0,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 0,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_CORE1,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 1,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 1,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_CORE2,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 2,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 2,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_CORE3,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 3,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 3,
		.mod_en_width       = 1,
	},
	{
		.clk_no             = DE_CLK_WB,
		.ahb_reset_adr      = 0x8000,
		.ahb_reset_shift    = 4,
		.ahb_reset_width    = 1,
		.mod_en_adr         = 0x8004,
		.mod_en_shift       = 4,
		.mod_en_width       = 1,
	},
};


static u8 __iomem *de_base;

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
	u32 reg_val = en & 0x1;

	writel(reg_val, de_base + DE_MBUS_CLOCK_ADDR);
}

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
	__inf("clk %d reset enable\n", para->clk_no);

	reg_base = para->mod_en_adr + de_base;
		reg_val = readl(reg_base);
	reg_val = SET_BITS(para->mod_en_shift, para->mod_en_width,
		reg_val, val);
	writel(reg_val, reg_base);
	__inf("clk %d mod enable\n", para->clk_no);

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
	} else if (0 == count) {
		__de_mbus_clk_enable(0);
	} else if (0 > count) {
		DE_WRN("mbus ref count=%d\n", count);
	}

	return 0;
}

s32 de_top_set_de2tcon_mux(u32 disp, u32 tcon)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 2;
	u32 shift = disp << 1;

	reg_base = de_base + DE2TCON_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, tcon);
	writel(reg_val, reg_base);
	return 0;
}

u32 de_top_get_tcon_from_mux(u32 disp)
{
	u32 width = 2;
	u32 shift = disp << 1;
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
	writel(readl(de_base + DE_RTWB_MUX_OFFSET) | id,
	       de_base + DE_RTWB_MUX_OFFSET);
	return 0;
}

enum de_rtwb_mux_id de_top_get_rtwb_mux(u32 wb_id)
{
	u32 reg_val = readl(de_base + DE_RTWB_MUX_OFFSET);

	return (enum de_rtwb_mux_id)(reg_val & 0x7);
}

s32 de_top_set_rtwb_mode(u32 wb_id, enum de_rtwb_mode mode)
{
	if (mode == TIMING_FROM_TCON)
		writel(readl(de_base + DE_RTWB_MUX_OFFSET) & 0xFFFFFFDF,
		       de_base + DE_RTWB_MUX_OFFSET);
	else
		writel(readl(de_base + DE_RTWB_MUX_OFFSET) | 0x20,
		       de_base + DE_RTWB_MUX_OFFSET);
	return 0;
}

s32 de_top_start_rtwb(u32 wb_id, u32 en)
{
	if (en)
		writel(readl(de_base + DE_RTWB_MUX_OFFSET) | 0x10,
		       de_base + DE_RTWB_MUX_OFFSET);
	else
		writel(readl(de_base + DE_RTWB_MUX_OFFSET) & 0xffffffef,
		       de_base + DE_RTWB_MUX_OFFSET);
	return 0;
}

s32 de_top_set_vchn2core_mux(
	u32 phy_chn, u32 phy_disp)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 2;
	u32 shift = phy_chn << 1;

	reg_base = de_base + DE_CHN2CORE_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, phy_disp);
	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_set_uchn2core_mux(
	u32 phy_chn, u32 phy_disp)
{
	u8 __iomem *reg_base;
	u32 reg_val;
	u32 width = 2;
	u32 shift = ((phy_chn - 6) << 1) + 16;

	reg_base = de_base + DE_CHN2CORE_MUX_OFFSET;
	reg_val = readl(reg_base);
	reg_val = SET_BITS(shift, width, reg_val, phy_disp);
	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_set_port2vchn_mux(u32 phy_disp,
	u32 port, u32 phy_chn)
{
	u8 __iomem *reg_base = de_base
		+ DE_PORT2CHN_MUX_OFFSET(phy_disp);
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
	u8 __iomem *reg_base = de_base
		+ DE_PORT2CHN_MUX_OFFSET(phy_disp);
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

s32 de_top_set_rtmx_enable(u32 disp, u8 en)
{
	u8 __iomem *reg_base = de_base + RTMX_GLB_CTL_OFFSET(disp);
	u32 reg_val = readl(reg_base);

	if (en)
		reg_val |= 0x1;
	else
		reg_val &= ~0x1;
	writel(reg_val, reg_base);

	reg_base = de_base + RTMX_AUTO_CLK_OFFSET(disp);
	if (en)
		writel(0x1, reg_base);
	else
		writel(0x0, reg_base);

	return 0;
}

s32 de_top_set_out_size(u32 disp, u32 width, u32 height)
{
	u8 __iomem *reg_base = de_base + RTMX_OUT_SIZE_OFFSET(disp);
	u32 reg_val = ((width - 1) & 0x1FFF)
		| (((height - 1) & 0x1FFF) << 16);

	writel(reg_val, reg_base);
	return 0;
}

s32 de_top_get_out_size(u32 disp, u32 *width, u32 *height)
{
	u8 __iomem *reg_base = de_base + RTMX_OUT_SIZE_OFFSET(disp);
	u32 reg_val = readl(reg_base);

	if (width)
		*width = (reg_val & 0x1FFF) + 1;
	if (height)
		*height = ((reg_val >> 16) & 0x1FFF) + 1;
	return 0;
}

s32 de_top_enable_irq(u32 disp, u32 irq_flag, u32 en)
{
	u8 __iomem *reg_base = de_base + RTMX_GLB_CTL_OFFSET(disp);
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
	u8 __iomem *reg_base = de_base + RTMX_GLB_STS_OFFSET(disp);
	u32 reg_val = readl(reg_base);
	u32 state = reg_val & irq_state & DE_IRQ_STATE_MASK;

	reg_val &= ~DE_IRQ_STATE_MASK;
	reg_val |= state;
	writel(reg_val, reg_base); /* w1c */

	return state;
}

s32 de_top_set_rcq_update(u32 disp, u32 update)
{
	update = update ? 1 : 0;
	writel(update, de_base + RTMX_RCQ_CTL_OFFSET(disp));
	return 0;
}

s32 de_top_set_rcq_head(u32 disp, u64 addr, u32 len)
{
	u8 __iomem *reg_base = de_base + RTMX_RCQ_CTL_OFFSET(disp);
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
#if defined(CONFIG_ARCH_SUN50IW5T)
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

#define DE_BYTE_ALIGN(x) (((x + (32 - 1)) >> 5) << 5)
/*#define DE_BYTE_ALIGN(x) (((x + (4*1024 - 1)) >> 12) << 12)*/
#define DE_BLOCK_SIZE 204800


s32 de_top_mem_pool_alloc(void)
{
	s32 ret = 0;

#if RTMX_USE_RCQ
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
				DE_WRN("Malloc %d byte fail, out of total "
				       "memory!!!\n",
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

#include "de_clock.h"

static u32 top_base = 0;
static u32 de_clk_freq = 0;

de_top_para disp_de_top_tbl[] =
{
  /*              mod_id            freq           gate             reset             dram              mod               div */
	DE_TOP_CFG(MOD_CLK_DEBE0,   396000000,  0xf3000008, 8,  0xf300000c, 8,  0xf3000004, 8,  0xf3000000, 8,  0xf3000020, 16)
	DE_TOP_CFG(MOD_CLK_DEBE1,   396000000,  0xf3000008, 9,  0xf300000c, 9,  0xf3000004, 9,  0xf3000000, 9,  0xf3000020, 20)
	DE_TOP_CFG(MOD_CLK_DEBE2,   396000000,  0xf3000008, 10, 0xf300000c, 10, 0xf3000004, 10, 0xf3000000, 10, 0xf3000020, 24)
	DE_TOP_CFG(MOD_CLK_DEFE0,   396000000,  0xf3000008, 0,  0xf300000c, 0,  0xf3000004, 0,  0xf3000000, 0,  0xf3000020, 0)
	DE_TOP_CFG(MOD_CLK_DEFE1,   396000000,  0xf3000008, 1,  0xf300000c, 1,  0xf3000004, 1,  0xf3000000, 1,  0xf3000020, 4)
	DE_TOP_CFG(MOD_CLK_DEFE2,   396000000,  0xf3000008, 2,  0xf300000c, 2,  0xf3000004, 2,  0xf3000000, 2,  0xf3000020, 8)
	DE_TOP_CFG(MOD_CLK_IEPDEU0, 0,          0xf3000008, 4,  0xf300000c, 4,  0xf3000004, 4,  0xf3000000, 4,  0x0,        32)
	DE_TOP_CFG(MOD_CLK_IEPDEU1, 0,          0xf3000008, 5,  0xf300000c, 5,  0xf3000004, 5,  0xf3000000, 5,  0x0,        32)
	DE_TOP_CFG(MOD_CLK_IEPDRC0, 0,          0xf3000008, 12, 0xf300000c, 12, 0xf3000004, 12, 0xf3000000, 12, 0x0,        32)
	DE_TOP_CFG(MOD_CLK_IEPDRC1, 0,          0xf3000008, 13, 0xf300000c, 13, 0xf3000004, 13, 0xf3000000, 13, 0x0,        32)
	DE_TOP_CFG(MOD_CLK_MERGE,   0,          0x0 ,       32, 0xf300000c, 20, 0x0,        32, 0xf3000000, 20, 0x0,        32)
};

#define disp_clk_inf(clk_id, clk_name, clk_src_name, clk_freq)\
	{.id = clk_id, .name = clk_name, .src_name = clk_src_name, .freq = clk_freq}

__disp_clk_t disp_clk_pll_tbl[] =
{
	disp_clk_inf(SYS_CLK_PLL7,  "pll7",   NULL, 0),
	disp_clk_inf(SYS_CLK_PLL8,  "pll8",   NULL, 297000000),
	disp_clk_inf(SYS_CLK_PLL10, "pll10",  NULL, 0),
};

__disp_clk_t disp_clk_mod_tbl[] =
{
	disp_clk_inf(MOD_CLK_DE_TOP,    "de",          "pll10", 396000000),
	disp_clk_inf(MOD_CLK_LCD0CH0,   "lcd0",        "pll7",  0),
	disp_clk_inf(MOD_CLK_LCD0CH1,   "lcd0",        "pll7",  0),
	disp_clk_inf(MOD_CLK_LCD1CH0,   "lcd1",        "pll7",  0),
	disp_clk_inf(MOD_CLK_LCD1CH1,   "lcd1",        "pll8",  0),
	disp_clk_inf(MOD_CLK_HDMI,      "hdmi",        "pll8",  0),
	disp_clk_inf(MOD_CLK_HDMI_DDC,  "hdmi_slow",   "pll8",  0),
	disp_clk_inf(MOD_CLK_MIPIDSIS,  "mipi_dsi0",   "pll7",  0),
	disp_clk_inf(MOD_CLK_MIPIDSIP,  "mipi_dsi1",   "pll7",  0),
	disp_clk_inf(MOD_CLK_LVDS,      "lvds",        NULL,    0),
	disp_clk_inf(MOD_CLK_EDP,       "edp",         NULL,    0),
};

u32 de_clk_get_freq(u32 freq_level)
{
	if(freq_level == 0)
		de_clk_freq = 297000000;
	else if(freq_level == 1)
		de_clk_freq = 396000000;

	return de_clk_freq;
}

s32 de_top_clk_enable(u32 clk_no, u32 enable)
{
	u32 i;
	u32 reg_val;

	for(i=0; i<(sizeof(disp_de_top_tbl)/sizeof(de_top_para)); i++) {
		if((disp_de_top_tbl[i].clk_no == clk_no)) {
			if(enable)
			{
				if(disp_de_top_tbl[i].ahb_reset_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].ahb_reset_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].ahb_reset_shift, 1, reg_val, 1);
					writel(reg_val, disp_de_top_tbl[i].ahb_reset_adr);
					__inf("clk %d reset enable\n", clk_no);
				}

				if(disp_de_top_tbl[i].ahb_gate_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].ahb_gate_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].ahb_gate_shift, 1, reg_val, 1);
					writel(reg_val, disp_de_top_tbl[i].ahb_gate_adr);
					__inf("clk %d gate enable\n", clk_no);
				}

				if(disp_de_top_tbl[i].mod_enable_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].mod_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].mod_enable_shift, 1, reg_val, 1);
					writel(reg_val, disp_de_top_tbl[i].mod_adr);
					__inf("clk %d mod enable\n", clk_no);
				}

				if(disp_de_top_tbl[i].dram_gate_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].dram_gate_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].dram_gate_shift, 1, reg_val, 1);
					writel(reg_val, disp_de_top_tbl[i].dram_gate_adr);
					__inf("clk %d dram enable\n", clk_no);
				}
			}
			else
			{
				if(disp_de_top_tbl[i].dram_gate_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].dram_gate_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].dram_gate_shift, 1, reg_val, 0);
					writel(reg_val, disp_de_top_tbl[i].dram_gate_adr);
					__inf("clk %d dram disable\n", clk_no);
				}

				if(disp_de_top_tbl[i].mod_enable_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].mod_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].mod_enable_shift, 1, reg_val, 0);
					writel(reg_val, disp_de_top_tbl[i].mod_adr);
					__inf("clk %d mod disable\n", clk_no);
				}

				if(disp_de_top_tbl[i].ahb_gate_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].ahb_gate_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].ahb_gate_shift, 1, reg_val, 0);
					writel(reg_val, disp_de_top_tbl[i].ahb_gate_adr);
					__inf("clk %d gate disable\n", clk_no);
				}
#if 0
				if(disp_de_top_tbl[i].ahb_reset_shift < 32)
				{
					reg_val = readl(disp_de_top_tbl[i].ahb_reset_adr);
					reg_val = SET_BITS(disp_de_top_tbl[i].ahb_reset_shift, 1, reg_val, 0);
					writel(reg_val, disp_de_top_tbl[i].ahb_reset_adr);
					__inf("clk %d reset disable\n", clk_no);
				}
#endif
			}
		}
	}

	return -1;
}

s32 de_top_clk_set_freq(u32 clk_no, u32 freq)
{
	de_top_para *top_clk;
	u32 i = 0;
	u32 src_freq;
	u32 div;
	u32 reg_val;
	struct clk* hSysClk = NULL;

	freq = de_clk_freq;

	if(!freq) {
		for(i = 0; i < (sizeof(disp_de_top_tbl) / sizeof(de_top_para)); i++) {
			if((disp_de_top_tbl[i].clk_no == clk_no)) {
				freq = disp_de_top_tbl[i].freq;
				__inf("clk %d freq is %d\n", disp_de_top_tbl[i].clk_no, disp_de_top_tbl[i].freq);
				break;
			}
		}
	}

	if(i == (sizeof(disp_de_top_tbl) / sizeof(de_top_para)))
		__wrn("clk %d is not initalized\n", clk_no);

	if(!freq) {
		__wrn("get clk %d freq fail\n", clk_no);
		return -1;
	}

	hSysClk = clk_get(NULL, "de");

	if(NULL == hSysClk || IS_ERR(hSysClk)) {
		__wrn("Fail to get handle for system clock de.\n");
		return -1;
	}

	src_freq = clk_get_rate(hSysClk);
	clk_put(hSysClk);

	div = (src_freq + freq / 2 - 1) / freq;

	if((div < 1) || (div > 16)) {
		__wrn("div is overflow\n");
		return -1;
	}

	for(i=0; i<(sizeof(disp_de_top_tbl)/sizeof(de_top_para)); i++) {
		if((disp_de_top_tbl[i].clk_no == clk_no)) {
			top_clk = &disp_de_top_tbl[i];

			reg_val = readl(disp_de_top_tbl[i].mod_div_adr);
			reg_val = SET_BITS(disp_de_top_tbl[i].mod_div_shift, 4, reg_val, (div - 1));
			writel(reg_val, disp_de_top_tbl[i].mod_div_adr);

			return 0;
		}
	}

	__wrn("clk %d not foundis not initializd\n", clk_no);

	return -1;
}

s32 de_top_set_reg_base(u32 reserve, u32 reg_base)
{
	top_base = reg_base;

	return 0;
}


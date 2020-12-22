#ifndef __DE_CLOCK_H__
#define __DE_CLOCK_H__

#include "ebios_de.h"

#define DE_TOP_CFG(_clk_no, _freq, _ahb_gate_adr, _ahb_gate_shift, _ahb_reset_adr, _ahb_reset_shift,\
					_dram_gate_adr, _dram_gate_shift, _mod_adr, _mod_enable_shift, _mod_div_adr, _mod_div_shift)\
{\
	.clk_no = _clk_no,\
	.freq = _freq,\
	.ahb_gate_adr = (void __iomem *)_ahb_gate_adr,\
	.ahb_gate_shift = _ahb_gate_shift,\
	.ahb_reset_adr = (void __iomem *)_ahb_reset_adr,\
	.ahb_reset_shift = _ahb_reset_shift,\
	.dram_gate_adr = (void __iomem *)_dram_gate_adr,\
	.dram_gate_shift = _dram_gate_shift,\
	.mod_adr = (void __iomem *)_mod_adr,\
	.mod_enable_shift = _mod_enable_shift,\
	.mod_div_adr = (void __iomem *)_mod_div_adr,\
	.mod_div_shift = _mod_div_shift\
},

typedef enum
{
	CLK_NONE = 0,

	SYS_CLK_PLL3 = 1,
	SYS_CLK_PLL7 = 2,
	SYS_CLK_PLL8 = 3,
	SYS_CLK_PLL10 = 4,
	SYS_CLK_PLL3X2 = 5,
	SYS_CLK_PLL6 = 6,
	SYS_CLK_PLL6x2 = 7,
	SYS_CLK_PLL7X2 = 8,
	SYS_CLK_MIPIPLL = 9,

	MOD_CLK_DEBE0 = 16,
	MOD_CLK_DEBE1 = 17,
	MOD_CLK_DEFE0 = 18,
	MOD_CLK_DEFE1 = 19,
	MOD_CLK_LCD0CH0 = 20,
	MOD_CLK_LCD0CH1 = 21,
	MOD_CLK_LCD1CH0 = 22,
	MOD_CLK_LCD1CH1 = 23,
	MOD_CLK_HDMI = 24,
	MOD_CLK_HDMI_DDC = 25,
	MOD_CLK_MIPIDSIS = 26,
	MOD_CLK_MIPIDSIP = 27,
	MOD_CLK_IEPDRC0 = 28,
	MOD_CLK_IEPDRC1 = 29,
	MOD_CLK_IEPDEU0 = 30,
	MOD_CLK_IEPDEU1 = 31,
	MOD_CLK_LVDS = 32,
	MOD_CLK_EDP  = 33,
	MOD_CLK_DEBE2 = 34,
	MOD_CLK_DEFE2 = 35,
	MOD_CLK_SAT0 = 36,
	MOD_CLK_SAT1 = 37,
	MOD_CLK_SAT2 = 38,
	MOD_CLK_MERGE = 39,
	MOD_CLK_DE_TOP = 40,
}__disp_clk_id_t;

typedef struct
{
	__disp_clk_id_t       id;     /* clock id         */
	char        *name;  /* clock name       */
	char *src_name;
	u32 freq;
	struct clk  *hdl;
}__disp_clk_t;

typedef struct {
	__disp_clk_id_t clk_no;
	u32 freq;
	void __iomem * ahb_gate_adr;
	u32 ahb_gate_shift;
	void __iomem * ahb_reset_adr;
	u32 ahb_reset_shift;
	void __iomem * dram_gate_adr;
	u32 dram_gate_shift;
	void __iomem * mod_adr;
	u32 mod_enable_shift;
	void __iomem * mod_div_adr;
	u32 mod_div_shift;
}de_top_para;


extern __disp_clk_t disp_clk_pll_tbl[3];
extern __disp_clk_t disp_clk_mod_tbl[19];
extern de_top_para disp_de_top_tbl[];

extern u32 de_clk_get_freq(u32 freq_level);
extern s32 de_top_clk_enable(u32 clk_no, u32 enable);
extern s32 de_top_clk_set_freq(u32 clk_no, u32 freq);

#endif

#ifndef  __DE_CLOCK_H__
#define  __DE_CLOCK_H__

#include "ebios_de.h"

typedef enum
{
	CLK_NONE = 0,

	SYS_CLK_PLL3 = 1,
	SYS_CLK_PLL7 = 2,
	SYS_CLK_PLL9 = 3,
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
}__disp_clk_id_t;

typedef struct
{
	__disp_clk_id_t       id;     /* clock id         */
	char        *name;  /* clock name       */
	char *src_name;
	u32 freq;
	struct clk  *hdl;
}__disp_clk_t;

extern __disp_clk_t disp_clk_pll_tbl[3];
extern __disp_clk_t disp_clk_mod_tbl[8];

#endif   //__DE_CLOCK_H__

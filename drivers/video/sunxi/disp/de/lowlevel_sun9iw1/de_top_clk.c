
#include "de_top_clk.h"

static u32 top_base = 0;

struct de_top_clk de_top_clk_array[] =
{
  /*              mod_id   div_reg,shist,width,en_reg,shift,rst_reg,shift,bus_reg,shift,dram_reg,shift */
	DE_TOP_CLK(DISP_MOD_FE0  ,0x20,    0,   4,   0x00,  0 ,     0x0c,   0 ,    0x08,  0 ,    0x04,   0  )
	DE_TOP_CLK(DISP_MOD_FE1  ,0x20,    4,   4,   0x00,  1 ,     0x0c,   1 ,    0x08,  1 ,    0x04,   1  )
	DE_TOP_CLK(DISP_MOD_FE2  ,0x20,    8,   4,   0x00,  2 ,     0x0c,   2 ,    0x08,  2 ,    0x04,   2  )
	DE_TOP_CLK(DISP_MOD_BE0  ,0x20,   16,   4,   0x00,  8 ,     0x0c,   8 ,    0x08,  8 ,    0x04,   8  )
	DE_TOP_CLK(DISP_MOD_BE1  ,0x20,   20,   4,   0x00,  9 ,     0x0c,   9 ,    0x08,  9 ,    0x04,   9  )
	DE_TOP_CLK(DISP_MOD_BE2  ,0x20,   24,   4,   0x00,  10,     0x0c,   10,    0x08,  10,    0x04,   10 )
	DE_TOP_CLK(DISP_MOD_DEU0 ,0x20,    0,   0,   0x00,  4 ,     0x0c,   4 ,    0x08,  4 ,    0x04,   4  )
	DE_TOP_CLK(DISP_MOD_DEU1 ,0x20,    0,   0,   0x00,  5 ,     0x0c,   5 ,    0x08,  5 ,    0x04,   5  )
	DE_TOP_CLK(DISP_MOD_DRC0 ,0x20,    0,   0,   0x00,  12,     0x0c,   12,    0x08,  12,    0x04,   12 )
	DE_TOP_CLK(DISP_MOD_DRC1 ,0x20,    0,   0,   0x00,  13,     0x0c,   13,    0x08,  13,    0x04,   13 )
	//DE_TOP_CLK(DISP_MOD_MERGE,0x20,    0,   0,   0x00,  20,  0x0c,   20,    0x08,  13,    0x04,   13 )
};

s32 de_top_clk_enable(u32 mod_id, u32 enable)
{
	struct de_top_clk *top_clk;
	u32 i;

	for(i=0; i<(sizeof(de_top_clk_array)/sizeof(struct de_top_clk)); i++) {
		if((de_top_clk_array[i].mod_id == mod_id) && (0 != top_base)) {
			u32 reg_val;
			top_clk = &de_top_clk_array[i];

			/* bus clk */
			reg_val = readl(top_base + top_clk->gate.bus_off);
			reg_val = SET_BITS(top_clk->gate.bus_shift, 1, reg_val, 1);
			writel(reg_val, top_base + top_clk->gate.bus_off);
			/* module clk */
			reg_val = readl(top_base + top_clk->gate.enable_off);
			reg_val = SET_BITS(top_clk->gate.enable_shift, 1, reg_val, 1);
			writel(reg_val, top_base + top_clk->gate.enable_off);
			/* reset clk */
			reg_val = readl(top_base + top_clk->gate.reset_off);
			reg_val = SET_BITS(top_clk->gate.reset_shift, 1, reg_val, 1);
			writel(reg_val, top_base + top_clk->gate.reset_off);
			/* dram clk */
			reg_val = readl(top_base + top_clk->gate.dram_off);
			reg_val = SET_BITS(top_clk->gate.dram_shift, 1, reg_val, 1);
			writel(reg_val, top_base + top_clk->gate.dram_off);

			return 0;
		}
	}

	return -1;
}

s32 de_top_clk_div(u32 mod_id, u32 div)
{
	struct de_top_clk *top_clk;
	u32 i;

	for(i=0; i<(sizeof(de_top_clk_array)/sizeof(struct de_top_clk)); i++) {
		if((de_top_clk_array[i].mod_id == mod_id) && (0 != top_base)) {
			u32 reg_val;
			top_clk = &de_top_clk_array[i];

			reg_val = readl(top_base + top_clk->divider.reg_off);
			reg_val = SET_BITS(top_clk->divider.shift, top_clk->divider.width, reg_val, div);
			writel(reg_val, top_base + top_clk->divider.reg_off);

			return 0;
		}
	}

	return -1;
}

s32 de_top_set_reg_base(u32 reserve, u32 reg_base)
{
	top_base = reg_base;

	return 0;
}


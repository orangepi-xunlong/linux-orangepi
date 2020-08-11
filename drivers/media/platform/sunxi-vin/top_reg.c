/*
 * linux-4.9/drivers/media/platform/sunxi-vin/top_reg.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include "top_reg_i.h"
#include "top_reg.h"

#include "utility/vin_io.h"

/*isp_id isp_input pasrer_id parser_ch*/
static int isp_input[4][4][4][4] = {
#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)

	/*isp0 input0~3*/
		/*parser0*/     /*parse1*/    /*parser2*/    /*parser3*/
	{
		{{0, 0, 0, 0}, {1, 2, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} }
	},

	/*isp1 input0~3*/
	{
		{{1, 2, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} }
	},
#else
	/*isp0 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp1 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp2 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp3 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	}
#endif
};

/*vipp_id isp_id isp_ch*/
static int vipp_input[8][4][4] = {
#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)
	/*vipp0*/
	/*isp0*/        /*isp1*/      /*isp2*/       /*isp3*/
	{{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 2, 0, 0}, {1, 3, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 2, 0}, {1, 0, 3, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 2}, {1, 4, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0} },
#else
	{{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 2, 0, 0}, {1, 3, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 2, 0}, {1, 0, 3, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 2}, {1, 4, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 0, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 2, 0, 0}, {1, 3, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 2, 0}, {1, 0, 3, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 2}, {1, 4, 0, 3} }
#endif
};

volatile void __iomem *csic_top_base[MAX_CSIC_TOP_NUM];

int csic_top_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSIC_TOP_NUM - 1)
		return -1;
	csic_top_base[sel] = (volatile void __iomem *)addr;

	return 0;
}

void csic_top_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_TOP_EN_MASK, 1 << CSIC_TOP_EN);
}

void csic_top_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_TOP_EN_MASK, 0 << CSIC_TOP_EN);
}

void csic_top_sram_pwdn(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_SRAM_PWDN_MASK, en << CSIC_SRAM_PWDN);
}

void csic_top_version_read_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_VER_EN_MASK, en << CSIC_VER_EN);
}

void csic_isp_input_select(unsigned int sel, unsigned int isp, unsigned int in,
				unsigned int psr, unsigned int ch)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_ISP0_IN0_REG_OFF + isp * 16 + in * 4,
			isp_input[isp][in][psr][ch]);
}

void csic_vipp_input_select(unsigned int sel, unsigned int vipp,
				unsigned int isp, unsigned int ch)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_VIPP0_IN_REG_OFF + vipp * 4,
			vipp_input[vipp][isp][ch]);
}

void csic_feature_list_get(unsigned int sel, struct csic_feature_list *fl)
{
	unsigned int reg_val = 0;

	reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_FEATURE_REG_OFF);
	fl->dma_num = (reg_val & CSIC_DMA_NUM_MASK) >> CSIC_DMA_NUM;
	fl->vipp_num = (reg_val & CSIC_VIPP_NUM_MASK) >> CSIC_VIPP_NUM;
	fl->isp_num = (reg_val & CSIC_ISP_NUM_MASK) >> CSIC_ISP_NUM;
	fl->ncsi_num = (reg_val & CSIC_NCSI_NUM_MASK) >> CSIC_NCSI_NUM;
	fl->mcsi_num = (reg_val & CSIC_MCSI_NUM_MASK) >> CSIC_MCSI_NUM;
	fl->parser_num = (reg_val & CSIC_PARSER_NUM_MASK) >> CSIC_PARSER_NUM;
}

void csic_version_get(unsigned int sel, struct csic_version *v)
{
	unsigned int reg_val = 0;

	reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_VER_REG_OFF);
	v->ver_small = (reg_val & CSIC_VER_SMALL_MASK) >> CSIC_VER_SMALL;
	v->ver_big = (reg_val & CSIC_VER_BIG_MASK) >> CSIC_VER_BIG;
}

void csic_ptn_generation_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_GEN_EN_REG_OFF,
			CSIC_PTN_GEN_CYCLE_MASK, 0 << CSIC_PTN_GEN_CYCLE);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_GEN_EN_REG_OFF,
			CSIC_PTN_GEN_EN_MASK, en << CSIC_PTN_GEN_EN);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_GEN_EN_REG_OFF,
			CSIC_PTN_GEN_START_MASK, en << CSIC_PTN_GEN_START);
}

void csic_ptn_control(unsigned int sel, int mode, int dw, int port)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_CLK_DIV_MASK, 0 << CSIC_PTN_CLK_DIV);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_MODE_MASK, mode << CSIC_PTN_MODE);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_DATA_WIDTH_MASK, dw << CSIC_PTN_DATA_WIDTH);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_CTRL_REG_OFF,
			CSIC_PTN_PORT_SEL_MASK, port << CSIC_PTN_PORT_SEL);
}

void csic_ptn_length(unsigned int sel, unsigned int len)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_PTN_LEN_REG_OFF, len);
}

void csic_ptn_addr(unsigned int sel, unsigned long dma_addr)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_PTN_ADDR_REG_OFF, dma_addr >> 2);
}

void csic_ptn_size(unsigned int sel, unsigned int w, unsigned int h)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_SIZE_REG_OFF,
			CSIC_PTN_WIDTH_MASK, w << CSIC_PTN_WIDTH);
	vin_reg_clr_set(csic_top_base[sel] + CSIC_PTN_SIZE_REG_OFF,
			CSIC_PTN_HEIGHT_MASK, h << CSIC_PTN_HEIGHT);
}


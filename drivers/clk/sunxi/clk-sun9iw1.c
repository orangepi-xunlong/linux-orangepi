/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk-private.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk/sunxi.h>
#include <mach/sys_config.h>
#include <mach/sunxi-smc.h>
#include "clk-sunxi.h"
#include "clk-factors.h"
#include "clk-periph.h"
#include "clk-sun9iw1.h"
#include "clk-sun9iw1_tbl.c"

static DEFINE_SPINLOCK(clk_lock);
void __iomem *sunxi_clk_base;
void __iomem *sunxi_clk_cpus_base;
int	sunxi_clk_maxreg =SUNXI_CLK_MAX_REG;
int cpus_clk_maxreg = CPUS_CLK_MAX_REG;
#ifdef CONFIG_SUNXI_CLK_DUMMY_DEBUG
unsigned int dummy_reg[1024]; 
unsigned int dummy_cpus_reg[1024]; 
unsigned int dummy_readl(unsigned int* regaddr)
{
	unsigned int val;
	val = *regaddr;
	printk("--%s-- dummy_readl to read reg 0x%x with val 0x%x\n",__func__,((unsigned int)regaddr - (unsigned int)&dummy_reg[0]),val);
	return val;
}
void  dummy_writel(unsigned int val,unsigned int* regaddr)
	{
		*regaddr = val;
		printk("--%s-- dummy_writel to write reg 0x%x with val 0x%x\n",__func__,((unsigned int)regaddr - (unsigned int)&dummy_reg[0]),val);
	}

void dummy_reg_init(void)
{
	memset(dummy_reg,0x0,sizeof(dummy_reg));				
	dummy_reg[PLL1_CFG/4]=0x02100300;
	dummy_reg[PLL2_CFG/4]=0x02100300;
	dummy_reg[PLL3_CFG/4]=0x00340914;
	dummy_reg[PLL4_CFG/4]=0x00000900;
	dummy_reg[PLL5_CFG/4]=0x00040800;
	dummy_reg[PLL6_CFG/4]=0x00040800;
	dummy_reg[PLL7_CFG/4]=0x00311700;	
	dummy_reg[PLL8_CFG/4]=0x00311700;
	dummy_reg[PLL9_CFG/4]=0x00040800;
	dummy_reg[PLL10_CFG/4]=0x00040800;
	dummy_reg[PLL11_CFG/4]=0x00040800;	
	dummy_reg[PLL12_CFG/4]=0x00040800;	
	dummy_reg[AXI0_CFG/4] =0x00000100;
	dummy_reg[AXI1_CFG/4] =0x00000100;	
	dummy_reg[ATS_CFG/4]  =0x80000000;	
	dummy_reg[TRACE_CFG/4]=0x80000000;	
	dummy_reg[DRAM_CFG/4] =0x00003010;
	dummy_reg[MP_CFG/4]   =0x0B000000;
	dummy_reg[LCD0_CFG/4] =0x08000000;
	dummy_reg[LCD1_CFG/4] =0x09000000;
	dummy_reg[MIPI_DSI0/4] =0x08000000;
	dummy_reg[HDMI_CFG/4] =0x09000000;
	dummy_reg[FD_CFG/4]   =0x0c000000;
	dummy_reg[GPU_AXI/4] =0x01000000;
}
#endif // of CONFIG_SUNXI_CLK_DUMMY_DEBUG


/*                        ns  nw  ks  kw  ms  mw  ps  pw  d1s  d1w  d2s  d2w  {frac  out  mode}  en-s   sdmss  sdmsw  sdmpat      sdmval*/
SUNXI_CLK_FACTORS(pll1,   8,  8,  0,  0,  0,  0,  16, 1,  0,   0,   0,   0,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS(pll2,   8,  8,  0,  0,  0,  0,  16, 1,  0,   0,   0,   0,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS(pll3,   8,  8,  0,  0,  0,  0,  0,  6,  16,  1,   18,  1,    0,    0,   0,     31,    24,    1,       PLL3_PAT, 0xc000e147);
SUNXI_CLK_FACTORS(pll4,   8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS(pll5,   8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS_UPDATE(pll6,   8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0,		30);
SUNXI_CLK_FACTORS(pll7,   8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   0,   0,    0,    0,   0,     31,    24,    0,       PLL7_PAT, 0xd1303333);
SUNXI_CLK_FACTORS(pll8,   8,  8,  0,  0,  0,  0,  0,  2,  16,  1,   0,   0,    0,    0,   0,     31,    24,    0,       PLL8_PAT, 0xd1303333);
SUNXI_CLK_FACTORS(pll9,   8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    24,    0,       PLL9_PAT, 0xd1303333);
SUNXI_CLK_FACTORS(pll10,  8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS(pll11,  8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS(pll12,  8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0);
static int get_factors_common(u32 rate, u32 parent_rate, struct clk_factors_value *factor,struct sunxi_clk_factors_config* config,unsigned int max,struct sunxi_clk_factor_freq table[],unsigned long tbl_count)
{
    u64 tmp_rate;
    int index;
    if(!factor)
        return -1;

    tmp_rate = rate>max ? max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;

        if(sunxi_clk_get_common_factors_search(config,factor, table,index,tbl_count))
 			return -1;
    return 0;
}
static int get_factors_pll3(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    if(rate == 22579200) {
        factor->factorn = 54;
        factor->factord1 = 0;
        factor->factord2 = 1;
        factor->factorp = 28;
        sunxi_clk_factor_pll3.sdmval = 0xc00121ff;
    } else if(rate == 24576000) {
        factor->factorn = 61;
         factor->factord1 = 0;
        factor->factord2 = 1;
        factor->factorp = 29;
        sunxi_clk_factor_pll3.sdmval = 0xc000e147;
    } else
        return -1;

    return 0;
}
static int get_factors_pll1(u32 rate, u32 parent_rate, struct clk_factors_value *factor) 
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll1, pll1_max,factor_pll1_tbl,sizeof(factor_pll1_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll2(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll2, pll2_max,factor_pll2_tbl,sizeof(factor_pll2_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll4(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll4, pll4_max,factor_pll4_tbl,sizeof(factor_pll4_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll5(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll5, pll5_max,factor_pll5_tbl,sizeof(factor_pll5_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll6(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll6, pll6_max,factor_pll6_tbl,sizeof(factor_pll6_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll7(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll7, pll7_max,factor_pll7_tbl,sizeof(factor_pll7_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll8(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll8, pll8_max,factor_pll8_tbl,sizeof(factor_pll8_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll9(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll9, pll9_max,factor_pll9_tbl,sizeof(factor_pll9_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll10(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll10, pll10_max,factor_pll10_tbl,sizeof(factor_pll10_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll11(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll11, pll11_max,factor_pll11_tbl,sizeof(factor_pll11_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static int get_factors_pll12(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    return get_factors_common(rate,parent_rate,factor, &sunxi_clk_factor_pll12, pll12_max,factor_pll12_tbl,sizeof(factor_pll12_tbl)/sizeof(struct sunxi_clk_factor_freq));
}
static unsigned long calc_rate_pll1(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factorp?4:1));
    return (unsigned long)tmp_rate;
}

static unsigned long calc_rate_pll3(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    if((factor->factorn == 54)  && (factor->factord1 == 0) && (factor->factord2 == 1) && (factor->factorp == 28))
        return 22579200;
    else if((factor->factorn == 61) && (factor->factord1 == 0) && (factor->factord2 == 1) && (factor->factorp == 29))
        return 24576000;
    else
    {
        tmp_rate = tmp_rate * (factor->factorn);
        do_div(tmp_rate, (factor->factord1+1) * (factor->factord2+1) * (factor->factorp+1));
        return (unsigned long)tmp_rate;
    }
}

static unsigned long calc_rate_media(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factord1+1) *(factor->factord2+1));
    return (unsigned long)tmp_rate;
}
static unsigned long calc_rate_pll7(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factord1+1));
    return (unsigned long)tmp_rate;
}
static unsigned long calc_rate_pll8(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factord1+1)*(1 << factor->factorp));
    return (unsigned long)tmp_rate;
}

static const char *hosc_parents[]   = {"hosc"}; //usbotg/edp /hdmi_slow /mipi_csi/csi_isp/csi_misc/avs
/*
PLL1/PLL2(200-3G) N8X6_K20x2_P16x1
PLL3/Audio												N8x6_K20x2_D1_16x1_D2_18X1_P0x6
PLL4/PLL9/PLL10/PLL11							N8x6_K20x2_D1_16x1_D2_18X1
PLL5/PLL6/PLL7										N8x6_K20x2_D1_16x1_D2_18X1
PLL8						  								N8x6_K20x2_D1_16x1_P0x2
*/
//pll1:c0cpu,   pll2:c1cpu,   pll3:audio, pll4:periph0,   pll5:ve,     pll6:ddr
//pll7:video0,  pll8:video1,  pll9:gpu,   pll10:de,       pll11:isp,   pll12:periph1
static struct factor_init_data sunxi_factos[] = {
    /* name  parent        parent_num, flags reg       lock_reg   lock_bit  config                     get_factors          calc_rate       priv_ops*/
    {"pll1", hosc_parents, 1,CLK_GET_RATE_NOCACHE,PLL1_CFG, LOCK_STAT, 0,   &sunxi_clk_factor_pll1,  &get_factors_pll1,   &calc_rate_pll1  ,(struct clk_ops*)NULL},
    {"pll2", hosc_parents, 1,CLK_GET_RATE_NOCACHE,PLL2_CFG, LOCK_STAT, 1,   &sunxi_clk_factor_pll2,  &get_factors_pll2,   &calc_rate_pll1  ,(struct clk_ops*)NULL},
    {"pll3", hosc_parents, 1,          0,    PLL3_CFG, LOCK_STAT, 2,        &sunxi_clk_factor_pll3,  &get_factors_pll3,   &calc_rate_pll3  ,(struct clk_ops*)NULL},
    {"pll4", hosc_parents, 1,          0,    PLL4_CFG, LOCK_STAT, 3,        &sunxi_clk_factor_pll4,  &get_factors_pll4,   &calc_rate_media ,(struct clk_ops*)NULL},
    {"pll5", hosc_parents, 1,          0,    PLL5_CFG, LOCK_STAT, 4,        &sunxi_clk_factor_pll5,  &get_factors_pll5,   &calc_rate_media ,(struct clk_ops*)NULL},
    {"pll6", hosc_parents, 1,          0,    PLL6_CFG, LOCK_STAT, 5,        &sunxi_clk_factor_pll6,  &get_factors_pll6,   &calc_rate_media ,(struct clk_ops*)NULL},
    {"pll7", hosc_parents, 1,CLK_GET_RATE_NOCACHE,PLL7_CFG, LOCK_STAT, 6,   &sunxi_clk_factor_pll7,  &get_factors_pll7,   &calc_rate_pll7  ,(struct clk_ops*)NULL},
    {"pll8", hosc_parents, 1,CLK_GET_RATE_NOCACHE,PLL8_CFG, LOCK_STAT, 7,   &sunxi_clk_factor_pll8,  &get_factors_pll8,   &calc_rate_pll8  ,(struct clk_ops*)NULL},
    {"pll9", hosc_parents, 1,          0,    PLL9_CFG, LOCK_STAT, 8,        &sunxi_clk_factor_pll9,  &get_factors_pll9,   &calc_rate_media ,(struct clk_ops*)NULL},
    {"pll10",hosc_parents, 1,          0,    PLL10_CFG,LOCK_STAT, 9,        &sunxi_clk_factor_pll10, &get_factors_pll10,  &calc_rate_media ,(struct clk_ops*)NULL},
    {"pll11",hosc_parents, 1,          0,    PLL11_CFG,LOCK_STAT, 10,       &sunxi_clk_factor_pll11, &get_factors_pll11,  &calc_rate_media ,(struct clk_ops*)NULL},
    {"pll12",hosc_parents, 1,          0,    PLL12_CFG,LOCK_STAT, 11,       &sunxi_clk_factor_pll12, &get_factors_pll12,  &calc_rate_media ,(struct clk_ops*)NULL},    
};


struct periph_init_data {
    const char          *name;
    unsigned long       flags;        
    const char          **parent_names;
    int                 num_parents;
    struct sunxi_clk_periph *periph;
};

static const char *cluster0_parents[]   = {"hosc", "pll1"};
static const char *cluster1_parents[]   = {"hosc", "pll2"};
static const char *axi0_parents[]   = {"cluster0"};
static const char *axi1_parents[]   = {"cluster1"};
static const char	*gt_parents[]     = {"hosc","pll4","pll12","pll12"};
static const char *ahb_parents[]    = {"gt","pll4","pll12","pll12"};		//ahb0/ahb1/ahb2
static const char *apb_parents[]    = {"hosc","pll4"};						//apb0/apb1
static const char	*cci400_parents[]    = {"hosc","pll4","pll12","pll12"}; //cci400
static const char	*ats_parents[]    = {"hosc","pll4","",""};   //ats/trace
static const char	*outa_parents[]   = {"hosc_750","losc","hosc",""};
static const char	*outb_parents[]   = {"hosc_750","losc","hosc",""};
static const char	*storage_parents[]= {"hosc","pll4","","",  "","","","",  "","","","",  "","","",""}; //nand0_0/0_1/1_0/1_1 sdmmc0/1/2/3  ts/ss spi0/1/2/3
static const char	*ss_parents[]= {"hosc","pll4","","",  "","","","",  "","","","",  "","pll12","",""}; //ss
static const char *audio_parents[]  = {"pll3"}; //i2s0/1 spdif/ac97
static const char *sdr_parents[]    = {"pll4","","","pll6",  "","","","",  "","","","",  "","","",""}; //sdr0/sdr1
static const char *de_parents[]     = {"pll10"};
static const char	*mp_parents[]   	= {"","","","",  "","","","",  "","pll8","pll9","pll10",  "","","",""};
static const char	*display_parents[]= {"","","","",  "","","","",  "pll7","pll8","","",  "","","",""}; //lcd0/1 mipi_dsi0 hdmi
static const char	*mipidsi1_parents[]= {"hosc","","","",  "","","","",  "","pll8","","",  "","","",""}; //mipi_dsi1
static const char *csi_parents[]    = {"hosc","","","",  "","","","",  "","pll8","","",  "","","",""}; //csi0/cs1
static const char *fd_parents[]    = {"","pll4","","", "","","","", "","","","",  "pll11","","",""}; //fd
static const char *ve_parents[]     = {"pll5"};
static const char *gpu_parents[]    = {"pll9"}; //gpucore/gpumem 
static const char *gpuaxi_parents[] = {"","pll4","","", "","","","", "","","pll9","", "","","",""}; //gpuaxi
static const char *sata_parents[]   = {"pll4"}; //sata
static const char *mipihsi_parents[]= {"hosc","pll4","","",  "","","","",  "","","","",  "","","",""}; //mipi_hsi
static const char *gpadc_parents[]  = {"hosc","","","",  "pll3","","","losc",  "","","","",  "","","",""}; //gpadc
static const char *cirtx_parents[]  = {"hosc","","","",  "","","","losc",  "","","","",  "","","",""}; //gpadc
static const char *ahb0mod_parents[]= {"ahb0"};
static const char *ahb1mod_parents[]= {"ahb1"};
static const char *ahb2mod_parents[]= {"ahb2"};
static const char *apb0mod_parents[]= {"apb0"};
static const char *apb1mod_parents[]= {"apb1"};
static const char *cpurdev_parents[]  = {"losc", "hosc","",""};
static const char *isp_parents[]     = {"pll11"};
static struct sunxi_clk_comgate com_gates[]={
{"nand0",    0,  0x3,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
{"nand1",    0,  0x3,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
{"sdmmc",    0,  0xf,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
{"mipi_dsi", 0,  0x3,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
{"csi",      0, 0x1f,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
{"gpu",      0,  0x7,    RST_GATE_SHARE,                 0},
};
/*
SUNXI_CLK_PERIPH(name,    mux_reg,    mux_shift, mux_width, div_reg,    div_mshift, div_mwidth, div_nshift, div_nwidth, gate_flags, enable_reg, reset_reg, bus_gate_reg, drm_gate_reg, enable_shift, reset_shift, bus_gate_shift, dram_gate_shift, lock,com_gate,com_gate_off)
*/
SUNXI_CLK_PERIPH(cluster0, CPU_CFG,    0,        1,         0,          0,          0,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cluster1, CPU_CFG,    8,        1,         0,          0,          0,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(axi0,     0,          0,        0,         AXI0_CFG,   0,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(axi1,     0,          0,        0,         AXI1_CFG,   0,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(gt,       GT_CFG,     24,       2,         GT_CFG,   	0,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ahb0,     AHB0_CFG,   24,       2,         AHB0_CFG,   0,          0,          0,          2,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ahb1,     AHB1_CFG,   24,       2,         AHB1_CFG,   0,          0,          0,          2,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ahb2,     AHB2_CFG,   24,       2,         AHB2_CFG,   0,          0,          0,          2,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(apb0,     APB0_CFG,   24,       1,         APB0_CFG,   0,          0,          0,          2,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(apb1,     APB1_CFG,   24,       1,         APB1_CFG,   0,          5,          16,         2,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cci400, CCI400_CFG,   24,       2,         CCI400_CFG, 0,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ats,      ATS_CFG,    24,       2,         ATS_CFG,    0,          3,          0,          0,          0,          ATS_CFG,    0,         0,            0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(trace,    TRACE_CFG,  24,       2,         TRACE_CFG,  0,          3,          0,          0,          0,          TRACE_CFG,  0,         0,            0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(outa,     CLK_OUTA,   24,       2,         CLK_OUTA,   8,          5,          20,         2,          0,          CLK_OUTA,   0,         0,            0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(outb,     CLK_OUTB,   24,       2,         CLK_OUTB,   8,          5,          20,         2,          0,          CLK_OUTB,   0,         0,            0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(nand0_0,  NAND0_CFG0, 24,       4,         NAND0_CFG0, 0,          4,          16,         2,          0,          NAND0_CFG0, AHB0_RST,  AHB0_GATE,    0,            31,          13,           13,             0,            &clk_lock,&com_gates[0],    0);
SUNXI_CLK_PERIPH(nand0_1,  NAND0_CFG1, 24,       4,         NAND0_CFG1, 0,          4,          16,         2,          0,          NAND0_CFG1, AHB0_RST,  AHB0_GATE,    0,            31,          13,           13,             0,            &clk_lock,&com_gates[0],    1);
SUNXI_CLK_PERIPH(nand1_0,  NAND1_CFG0, 24,       4,         NAND1_CFG0, 0,          4,          16,         2,          0,          NAND1_CFG0, AHB0_RST,  AHB0_GATE,    0,            31,          12,           12,             0,            &clk_lock,&com_gates[1],    0);
SUNXI_CLK_PERIPH(nand1_1,  NAND1_CFG1, 24,       4,         NAND1_CFG1, 0,          4,          16,         2,          0,          NAND1_CFG1, AHB0_RST,  AHB0_GATE,    0,            31,          12,           12,             0,            &clk_lock,&com_gates[1],    1);
SUNXI_CLK_PERIPH(sdmmc0,   SD0_CFG,    24,       4,         SD0_CFG,    0,          4,          16,         2,          0,          SD0_CFG,    AHB0_RST,  AHB0_GATE,    0,            31,           8,           8,              0,            &clk_lock,&com_gates[2],    0);
SUNXI_CLK_PERIPH(sdmmc1,   SD1_CFG,    24,       4,         SD1_CFG,    0,          4,          16,         2,          0,          SD1_CFG,    AHB0_RST,  AHB0_GATE,    0,            31,           8,           8,              0,            &clk_lock,&com_gates[2],    1);
SUNXI_CLK_PERIPH(sdmmc2,   SD2_CFG,    24,       4,         SD2_CFG,    0,          4,          16,         2,          0,          SD2_CFG,    AHB0_RST,  AHB0_GATE,    0,            31,           8,           8,              0,            &clk_lock,&com_gates[2],    2);
SUNXI_CLK_PERIPH(sdmmc3,   SD3_CFG,    24,       4,         SD3_CFG,    0,          4,          16,         2,          0,          SD3_CFG,    AHB0_RST,  AHB0_GATE,    0,            31,           8,           8,              0,            &clk_lock,&com_gates[2],    3);
SUNXI_CLK_PERIPH(ts,   	   TS_CFG,     24,       4,         TS_CFG,     0,          4,          16,         2,          0,          TS_CFG,     AHB0_RST,  AHB0_GATE,    0,            31,          18,          18,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ss,   	   SS_CFG,     24,       4,         SS_CFG,     0,          4,          16,         2,          0,          SS_CFG,     AHB0_RST,  AHB0_GATE,    0,            31,           5,           5,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spi0,     SPI0_CFG,   24,       4,         SPI0_CFG,   0,          4,          16,         2,          0,          SPI0_CFG,   AHB0_RST,  AHB0_GATE,    0,            31,          20,          20,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spi1,     SPI1_CFG,   24,       4,         SPI1_CFG,   0,          4,          16,         2,          0,          SPI1_CFG,   AHB0_RST,  AHB0_GATE,    0,            31,          21,          21,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spi2,     SPI2_CFG,   24,       4,         SPI2_CFG,   0,          4,          16,         2,          0,          SPI2_CFG,   AHB0_RST,  AHB0_GATE,    0,            31,          22,          22,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spi3,     SPI3_CFG,   24,       4,         SPI3_CFG,   0,          4,          16,         2,          0,          SPI3_CFG,   AHB0_RST,  AHB0_GATE,    0,            31,          23,          23,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(i2s0,     0,           0,       0,         I2S0_CFG,   0,          4,           0,         0,          0,          I2S0_CFG,   APB0_RST,  APB0_GATE,    0,            31,          12,          12,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(i2s1,     0,           0,       0,         I2S1_CFG,   0,          4,           0,         0,          0,          I2S1_CFG,   APB0_RST,  APB0_GATE,    0,            31,          13,          13,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spdif,    0,           0,       0,         SPDIF_CFG,  0,          4,           0,         0,          0,          SPDIF_CFG,  APB0_RST,  APB0_GATE,    0,            31,           1,           1,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbotg,   0,           0,       0,         0,          0,          0,           0,         0,          0,          0,          AHB1_RST,  AHB1_GATE,    0,             0,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbotgphy,0,           0,       0,         0,          0,          0,           0,         0,          0,          USB_CFG,    AHB1_RST,  0,            0,            31,           1,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbhci,   0,           0,       0,         0,          0,          0,           0,         0,          0,          0,          0,         AHB1_GATE,    0,             0,           0,           1,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(sdr0,     DRAM_CFG,   12,       4,         DRAM_CFG,   8,          4,           0,         0,          0,          0,          0,         0,            0,             0,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(de,       0,           0,       0,         DE_CFG,     0,          4,           0,         0,          0,          DE_CFG,     AHB2_RST,  AHB2_GATE,    0,            31,           7,           7,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(edp,      0,           0,       0,         0,          0,          0,           0,         0,          0,          EDP_CFG,    AHB2_RST,  AHB2_GATE,    0,            31,           2,           2,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mp,       MP_CFG,     24,       4,         MP_CFG,     0,          4,           0,         0,          0,          MP_CFG,     AHB2_RST,  AHB2_GATE,    0,            31,           8,           8,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lcd0,     LCD0_CFG,   24,       4,         LCD0_CFG,   0,          4,           0,         0,          0,          LCD0_CFG,   AHB2_RST,  AHB2_GATE,    0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lcd1,     LCD1_CFG,   24,       4,         LCD1_CFG,   0,          4,           0,         0,          0,          LCD1_CFG,   AHB2_RST,  AHB2_GATE,    0,            31,           1,           1,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsi0,MIPI_DSI0,  24,       4,         MIPI_DSI0,  0,          4,           0,         0,          0,          MIPI_DSI0,  AHB2_RST,  AHB2_GATE,    0,            31,          11,          11,              0,            &clk_lock,&com_gates[3],    0);
SUNXI_CLK_PERIPH(mipi_dsi1,MIPI_DSI1,  24,       4,         MIPI_DSI1,  0,          4,           0,         0,          0,          MIPI_DSI1,  AHB2_RST,  AHB2_GATE,    0,            31,          11,          11,              0,            &clk_lock,&com_gates[3],    1);
SUNXI_CLK_PERIPH(hdmi,	   HDMI_CFG,   24,       4,         HDMI_CFG,   0,          4,           0,         0,          0,          HDMI_CFG,   AHB2_RST,  AHB2_GATE,AHB2_RST,         31,           5,           5,              6,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(hdmi_slow,0,  			0,       0,         0,          0,          0,           0,         0,          0,          HDMI_SLOW,  0,          0,           0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mipi_csi, 0,  			0,       0,         MIPI_CSI,   0,          4,           0,         0,          0,          MIPI_CSI,   AHB2_RST,  AHB2_GATE,    0,            31,           4,           4,              0,            &clk_lock,&com_gates[4],    0);
SUNXI_CLK_PERIPH(csi_isp,  0,  			0,       0,         CSI_ISP,    0,          4,           0,         0,          0,          CSI_ISP,    AHB2_RST,  AHB2_GATE,    0,            31,           4,           4,              0,            &clk_lock,&com_gates[4],    1);
SUNXI_CLK_PERIPH(csi_misc, 0,  			0,       0,         0,          0,          0,           0,         0,          0,          CSI_ISP,    AHB2_RST,  AHB2_GATE,    0,            16,           4,           4,              0,            &clk_lock,&com_gates[4],    2);
SUNXI_CLK_PERIPH(csi0_mclk,CSI0_MCLK,  24,       4,         CSI0_MCLK,  0,          4,           0,         0,          0,          CSI0_MCLK,  AHB2_RST,  AHB2_GATE,    0,            31,           4,           4,              0,            &clk_lock,&com_gates[4],    3);
SUNXI_CLK_PERIPH(csi1_mclk,CSI1_MCLK,  24,       4,         CSI1_MCLK,  0,          4,           0,         0,          0,          CSI1_MCLK,  AHB2_RST,  AHB2_GATE,    0,            31,           4,           4,              0,            &clk_lock,&com_gates[4],    4);
SUNXI_CLK_PERIPH(fd,	   FD_CFG,     24,       4,         FD_CFG,     0,          4,           0,         0,          0,          FD_CFG,     AHB0_RST,  AHB0_GATE,    0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ve,	   0,           0,       0,         VE_CFG,    16,          3,           0,         0,          0,          VE_CFG,     AHB0_RST,  AHB0_GATE,    0,            31,           1,           1,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(avs,	   0,           0,       0,         0,          0,          0,           0,         0,          0,          AVS_CFG,    0,         0,            0,            31,           0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(gpuctrl,  0,           0,       0,         0,          0,          0,           0,         0,          0,          0,          AHB0_RST,  AHB0_GATE,    0,             0,           3,           3,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(gpucore,  0,           0,       0,         GPU_CORE,   0,          3,           0,         0,          0,          GPU_CORE,   AHB2_RST,  0,            0,            31,           9,           0,              0,            &clk_lock,&com_gates[5],    0);
SUNXI_CLK_PERIPH(gpumem,   0,           0,       0,         GPU_MEM,    0,          3,           0,         0,          0,          GPU_MEM,    AHB2_RST,  0,            0,            31,           9,           0,              0,            &clk_lock,&com_gates[5],    1);
SUNXI_CLK_PERIPH(gpuaxi,   GPU_AXI,    24,       4,         GPU_AXI,    0,          4,           0,         0,          0,          GPU_AXI,    AHB2_RST,  0,            0,            31,           9,           0,              0,            &clk_lock,&com_gates[5],    2);
SUNXI_CLK_PERIPH(sata,	   0,           0,       0,         SATA_CFG,   0,          4,           0,         0,          0,          SATA_CFG,   AHB0_RST,  AHB0_GATE,    0,            31,          16,          16,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ac97,	   0,           0,       0,         AC97_CFG,   0,          4,           0,         0,          0,          AC97_CFG,   APB0_RST,  APB0_GATE,    0,            31,          11,          11,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mipi_hsi, MIPI_HSI,    24,      4,         MIPI_HSI,   0,          4,           0,         0,          0,          MIPI_HSI,   AHB1_RST,  AHB0_GATE,    0,            31,           9,          15,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(gpadc,    GP_ADC,      24,      4,         GP_ADC,     0,          4,           16,        2,          0,          GP_ADC,     APB0_RST,  APB0_GATE,    0,            31,          17,          17,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cirtx,    CIR_TX,      24,      4,         CIR_TX,     0,          4,           16,        2,          0,          CIR_TX,     APB0_RST,  APB0_GATE,    0,            31,          19,          19,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(sdram,	   0,           0,        0,         0,         0,          0,           0,         0,          0,          DRAM_CFG,   AHB0_RST,  AHB0_GATE,    0,            31,          14,          14,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(dma,      0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          AHB1_RST,  AHB1_GATE,    0,            0,           24,          24,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(hstmr,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          AHB1_RST,  AHB1_GATE,    0,            0,           23,          23,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spinlock, 0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          AHB1_RST,  AHB1_GATE,    0,            0,           22,          22,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(msgbox,   0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          AHB1_RST,  AHB1_GATE,    0,            0,           21,          21,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(gmac,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          AHB1_RST,  AHB1_GATE,    0,            0,           17,          17,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twd,      0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          0,         APB0_GATE,    0,            0,            0,          18,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lradc,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB0_RST,  APB0_GATE,    0,            0,           15,          15,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(pio,      0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          0,         APB0_GATE,    0,            0,            0,           5,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart5,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,           21,          21,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart4,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,           20,          20,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart3,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,           19,          19,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart2,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,           18,          18,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart1,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,           17,          17,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart0,    0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,           16,          16,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi4,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,            4,           4,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi3,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,            3,           3,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi2,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,            2,           2,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi1,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,            1,           1,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi0,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          APB1_RST,  APB1_GATE,    0,            0,            0,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lvds,     0,           0,        0,         0,         0,          0,           0,         0,          0,          0,          AHB2_RST,  0,            0,            0,            3,           0,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cpur1w,   CPUS_ONEWRITE,24,     2,         CPUS_ONEWRITE,0,        5,          16,         2,          0,          CPUS_ONEWRITE,CPUS_APB0_RST,CPUS_APB0_GATE,0,     31,            5,           5,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cpurcir,  CPUS_CIR,  24,        2,         CPUS_CIR,   0,          4,          16,         2,          0,          CPUS_CIR,   CPUS_APB0_RST,CPUS_APB0_GATE,0,       31,            1,           1,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cpuri2s0, 0,           0,       0,         CPUS_I2S0,  0,          4,           0,         0,          0,          CPUS_I2S0,  CPUS_APB0_RST,  CPUS_APB0_GATE,0,     31,           17,          17,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cpuri2s1, 0,           0,       0,         CPUS_I2S1,  0,          4,           0,         0,          0,          CPUS_I2S1,  CPUS_APB0_RST,  CPUS_APB0_GATE,0,     31,           18,          18,              0,            &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cpurvddve,0,           0,       0,         0,          0,          0,           0,         0,          0,          CPUS_VDDSYS_GATE,0,     0,           0,            2,            0,           0,              0,            &clk_lock,NULL,             0);
static struct periph_init_data sunxi_periphs_init[] = {
    {"cluster0", CLK_GET_RATE_NOCACHE,	cluster0_parents,     ARRAY_SIZE(cluster0_parents),     &sunxi_clk_periph_cluster0},
    {"cluster1", CLK_GET_RATE_NOCACHE,	cluster1_parents,     ARRAY_SIZE(cluster1_parents),     &sunxi_clk_periph_cluster1},
    {"axi0",     0,       				axi0_parents,     ARRAY_SIZE(axi0_parents),     &sunxi_clk_periph_axi0},
    {"axi1",     0,       				axi1_parents,     ARRAY_SIZE(axi1_parents),     &sunxi_clk_periph_axi1},
    {"gt",       0,       				gt_parents,       ARRAY_SIZE(gt_parents),       &sunxi_clk_periph_gt},
    {"ahb0",     0,       				ahb_parents,      ARRAY_SIZE(ahb_parents),      &sunxi_clk_periph_ahb0},
    {"ahb1",     0,       				ahb_parents,      ARRAY_SIZE(ahb_parents),      &sunxi_clk_periph_ahb1},
    {"ahb2",     0,       				ahb_parents,      ARRAY_SIZE(ahb_parents),      &sunxi_clk_periph_ahb2},                
    {"apb0",     0,       				apb_parents,      ARRAY_SIZE(apb_parents),      &sunxi_clk_periph_apb0},
    {"apb1",     0,       				apb_parents,      ARRAY_SIZE(apb_parents),      &sunxi_clk_periph_apb1},
    {"cci400",   0,       				cci400_parents,   ARRAY_SIZE(cci400_parents),   &sunxi_clk_periph_cci400},
    {"ats",      0,       				ats_parents,      ARRAY_SIZE(ats_parents),      &sunxi_clk_periph_ats},
    {"trace",    0,       				ats_parents,      ARRAY_SIZE(ats_parents),      &sunxi_clk_periph_trace},
    {"outa",     0,       				outa_parents,     ARRAY_SIZE(outa_parents),     &sunxi_clk_periph_outa},
    {"outb",     0,       				outb_parents,     ARRAY_SIZE(outb_parents),     &sunxi_clk_periph_outb},
    {"nand0_0",  0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_nand0_0},
    {"nand0_1",  0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_nand0_1},
    {"nand1_0",  0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_nand1_0},
    {"nand1_1",  0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_nand1_1},
    {"sdmmc0",   CLK_IGNORE_AUTORESET,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_sdmmc0},                                        
    {"sdmmc1",   CLK_IGNORE_AUTORESET,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_sdmmc1},   
    {"sdmmc2",   CLK_IGNORE_AUTORESET,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_sdmmc2},   
    {"sdmmc3",   CLK_IGNORE_AUTORESET,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_sdmmc3},   
    {"ts",       0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_ts},
    {"ss",       0,       			  ss_parents,       ARRAY_SIZE(ss_parents),       &sunxi_clk_periph_ss},
    {"spi0",     0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_spi0},
    {"spi1",     0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_spi1},
    {"spi2",     0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_spi2},
    {"spi3",     0,       			  storage_parents,  ARRAY_SIZE(storage_parents),  &sunxi_clk_periph_spi3},
    {"i2s0",     0,       			  audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_i2s0},
    {"i2s1",     0,       			  audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_i2s1},
    {"spdif",    0,       			  audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_spdif},
    {"usbotg",   0,       			  ahb0mod_parents,  ARRAY_SIZE(ahb0mod_parents),  &sunxi_clk_periph_usbotg},
    {"usbotgphy", 0,       			  hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_usbotgphy},
    {"usbhci",   0,       			  ahb0mod_parents,  ARRAY_SIZE(ahb0mod_parents),  &sunxi_clk_periph_usbhci},
    {"sdr0",     0,       			  sdr_parents,      ARRAY_SIZE(sdr_parents),      &sunxi_clk_periph_sdr0},
    {"de",       CLK_IGNORE_AUTORESET,de_parents,       ARRAY_SIZE(de_parents),       &sunxi_clk_periph_de},
    {"edp",      0,       			  hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_edp},
    {"mp",       0,       			  mp_parents,       ARRAY_SIZE(mp_parents),       &sunxi_clk_periph_mp},
    {"lcd0",     0,       			  display_parents,  ARRAY_SIZE(display_parents),  &sunxi_clk_periph_lcd0},
    {"lcd1",     0,       			  display_parents,  ARRAY_SIZE(display_parents),  &sunxi_clk_periph_lcd1},
    {"mipi_dsi0",0,       			  display_parents,  ARRAY_SIZE(display_parents),  &sunxi_clk_periph_mipi_dsi0},
    {"mipi_dsi1",0,       			  mipidsi1_parents, ARRAY_SIZE(mipidsi1_parents), &sunxi_clk_periph_mipi_dsi1},                                                                                        
    {"hdmi",	 0,       			  display_parents,  ARRAY_SIZE(display_parents),  &sunxi_clk_periph_hdmi},
    {"hdmi_slow",0,       			  hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_hdmi_slow},
    {"mipi_csi", 0,       			  hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_mipi_csi},
    {"csi_isp",  0,       			  isp_parents,      ARRAY_SIZE(isp_parents),      &sunxi_clk_periph_csi_isp},
    {"csi_misc", 0,       			  hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_csi_misc},
    {"csi0_mclk",0,       			  csi_parents,      ARRAY_SIZE(csi_parents),      &sunxi_clk_periph_csi0_mclk},
    {"csi1_mclk",0,       			  csi_parents,      ARRAY_SIZE(csi_parents),      &sunxi_clk_periph_csi1_mclk},
    {"fd",		 0,       			  fd_parents,       ARRAY_SIZE(fd_parents),       &sunxi_clk_periph_fd},
    {"ve",		 0,       			  ve_parents,       ARRAY_SIZE(ve_parents),       &sunxi_clk_periph_ve},
    {"avs",		 0,       			  hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_avs},
    {"gpuctrl",	 CLK_IGNORE_AUTORESET,gpu_parents,      ARRAY_SIZE(gpu_parents),      &sunxi_clk_periph_gpuctrl},
    {"gpucore",	 CLK_IGNORE_AUTORESET,gpu_parents,      ARRAY_SIZE(gpu_parents),      &sunxi_clk_periph_gpucore},
    {"gpumem",	 CLK_IGNORE_AUTORESET,gpu_parents,      ARRAY_SIZE(gpu_parents),      &sunxi_clk_periph_gpumem},
    {"gpuaxi",	 CLK_IGNORE_AUTORESET,gpuaxi_parents,   ARRAY_SIZE(gpuaxi_parents),   &sunxi_clk_periph_gpuaxi},
    {"sata",	 0,         	    sata_parents,     ARRAY_SIZE(sata_parents),     &sunxi_clk_periph_sata}, 
    {"ac97",     0,       			  audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_ac97},
    {"mipi_hsi", 0,           		mipihsi_parents,  ARRAY_SIZE(mipihsi_parents),  &sunxi_clk_periph_mipi_hsi},
    {"gpadc",    0,       			  gpadc_parents,    ARRAY_SIZE(gpadc_parents),    &sunxi_clk_periph_gpadc},
    {"cirtx",    0,       			  cirtx_parents,    ARRAY_SIZE(cirtx_parents),    &sunxi_clk_periph_cirtx},                      
    {"sdram",    0,       					ahb0mod_parents,  ARRAY_SIZE(ahb0mod_parents),  &sunxi_clk_periph_sdram},
    {"dma",      0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_dma},
    {"hstmr",    0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_hstmr},        
    {"spinlock", 0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_spinlock},
    {"msgbox",   0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_msgbox},
    {"gmac",     0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_gmac},
    {"lvds",     0,       				  ahb2mod_parents,  ARRAY_SIZE(ahb2mod_parents),  &sunxi_clk_periph_lvds},   
    {"twd",      0,       				apb0mod_parents,  ARRAY_SIZE(apb0mod_parents),  &sunxi_clk_periph_twd},    
    {"lradc",    0,       				apb0mod_parents,  ARRAY_SIZE(apb0mod_parents),  &sunxi_clk_periph_lradc},
    {"pio",      0,       				apb0mod_parents,  ARRAY_SIZE(apb0mod_parents),  &sunxi_clk_periph_pio},        
    {"twi0",     0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_twi0},
    {"twi1",     0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_twi1},
    {"twi2",     0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_twi2},
    {"twi3",     0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_twi3},
    {"twi4",     0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_twi4},    
    {"uart0",    0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_uart0},
    {"uart1",    0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_uart1},
    {"uart2",    0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_uart2},
    {"uart3",    0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_uart3},
    {"uart4",    0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_uart4},
    {"uart5",    0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_uart5},
};
static struct periph_init_data sunxi_periphs_cpus_init[] = {
    {"cpur1w",CLK_GET_RATE_NOCACHE,     cpurdev_parents,ARRAY_SIZE(cpurdev_parents),    &sunxi_clk_periph_cpur1w},   
    {"cpurcir",CLK_GET_RATE_NOCACHE,    cpurdev_parents,ARRAY_SIZE(cpurdev_parents),    &sunxi_clk_periph_cpurcir},        
    {"cpuri2s0",CLK_GET_RATE_NOCACHE,   audio_parents,ARRAY_SIZE(audio_parents),        &sunxi_clk_periph_cpuri2s0},  
    {"cpuri2s1",CLK_GET_RATE_NOCACHE,   audio_parents,ARRAY_SIZE(audio_parents),        &sunxi_clk_periph_cpuri2s1},      
    {"cpurvddve",CLK_REVERT_ENABLE,     hosc_parents,ARRAY_SIZE(hosc_parents),          &sunxi_clk_periph_cpurvddve},
};

static char *force_enable_clks[] = {"pll7", "pll8"};

struct sunxi_reg_ops clk_regops={sunxi_smc_readl, sunxi_smc_writel};
void __init sunxi_init_clocks(void)
{
    int     i;
	struct clk *clk;
    struct factor_init_data *factor;
    struct periph_init_data *periph;
#ifdef CONFIG_SUNXI_CLK_DUMMY_DEBUG
		sunxi_clk_base = &dummy_reg[0]; 
 		sunxi_clk_cpus_base = &dummy_cpus_reg[0];        
		dummy_reg_init();
#else
    /* get clk register base address */
    sunxi_clk_base = IO_ADDRESS(0x06000000);
    sunxi_clk_cpus_base = IO_ADDRESS(0x08001400);        
#endif
    sunxi_clk_factor_initlimits();
    /* register oscs */
    clk = clk_register_fixed_rate(NULL, "losc", NULL, CLK_IS_ROOT, 32768);
    clk_register_clkdev(clk, "losc", NULL);

    clk = clk_register_fixed_rate(NULL, "hosc", NULL, CLK_IS_ROOT, 24000000);
    clk_register_clkdev(clk, "hosc", NULL);

    clk = clk_register_fixed_factor(NULL, "hosc_750", "hosc", 0, 1, 750);
    clk_register_clkdev(clk, "hosc_750", NULL);
    
    /* register normal factors, based on sunxi factor framework */
    for(i=0; i<ARRAY_SIZE(sunxi_factos); i++) {
        factor = &sunxi_factos[i];
        factor->priv_regops = &clk_regops;
        clk = sunxi_clk_register_factors(NULL,  sunxi_clk_base, &clk_lock,factor);
        clk_register_clkdev(clk, factor->name, NULL);
    }
    /* force enable only for sun9iw1 PLLs */
    for(i=0;i < ARRAY_SIZE(force_enable_clks);i++)
    {
        clk = clk_get(NULL,force_enable_clks[i]);
        if(!clk || IS_ERR(clk))
        {
            clk = NULL;
            printk("Error not get clk %s\n",force_enable_clks[i]);
            continue;
        }
        clk_prepare_enable(clk);
        clk_put(clk);
        clk=NULL;
    }
    /* register periph clock */
    for(i=0; i<ARRAY_SIZE(sunxi_periphs_init); i++) {
        periph = &sunxi_periphs_init[i];
        periph->periph->priv_regops = &clk_regops;
        clk = sunxi_clk_register_periph(periph->name, periph->parent_names,
                        periph->num_parents,periph->flags, sunxi_clk_base, periph->periph);
        clk_register_clkdev(clk, periph->name, NULL);
    }

    for(i=0; i<ARRAY_SIZE(sunxi_periphs_cpus_init); i++) {
        periph = &sunxi_periphs_cpus_init[i];
        periph->periph->priv_regops = &clk_regops;
        clk = sunxi_clk_register_periph(periph->name, periph->parent_names,
                        periph->num_parents,periph->flags, sunxi_clk_cpus_base , periph->periph);
        clk_register_clkdev(clk, periph->name, NULL);
        }    
    /* write PLL BIAS default here */
    writel(0x80040000, sunxi_clk_base + PLL1_BIAS);
    writel(0x80040000, sunxi_clk_base + PLL2_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL3_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL4_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL5_BIAS);
    //do not set PLL6 let boot or DVFS to set
    writel(0x00040000, sunxi_clk_base + PLL7_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL8_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL9_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL10_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL11_BIAS);
    writel(0x00040000, sunxi_clk_base + PLL12_BIAS);
#ifdef CONFIG_COMMON_CLK_ENABLE_SYNCBOOT_EARLY
	clk_syncboot();
#endif
}

/*
********************************************************************************
*                                                   OSAL
*
*                                     (c) Copyright 2008-2009, Kevin China
*                                             				All Rights Reserved
*
* File    : OSAL_Clock.c
* By      : Sam.Wu
* Version : V1.00
* Date    : 2011/3/25 20:25
* Description :
* Update   :  date      author      version     notes
********************************************************************************
*/
#include "OSAL.h"
#include "OSAL_Clock.h"
#if !defined(CONFIG_ARCH_SUN6I)
#include <linux/clk/sunxi.h>
#include <linux/clk-private.h>
#endif

#ifndef __OSAL_CLOCK_MASK__

#if 0
#define disp_clk_inf(clk_id, clk_name)   {.id = clk_id, .name = clk_name}
#if defined CONFIG_ARCH_SUN8IW1P1
static __disp_clk_t disp_clk_tbl[] =
{
	disp_clk_inf(SYS_CLK_PLL3,        "pll3"      ),
	disp_clk_inf(SYS_CLK_PLL7,        "pll7"      ),
	disp_clk_inf(SYS_CLK_PLL9,        "pll9"      ),
	disp_clk_inf(SYS_CLK_PLL10,       "pll10"     ),
	disp_clk_inf(SYS_CLK_PLL3X2,      "pll3x2"    ),
	disp_clk_inf(SYS_CLK_PLL6,        "pll6"      ),
	disp_clk_inf(SYS_CLK_PLL6x2,      "pll6x2"    ),
	disp_clk_inf(SYS_CLK_PLL7X2,      "pll7x2"    ),
	disp_clk_inf(SYS_CLK_MIPIPLL,     "pll_mipi"  ),

	disp_clk_inf(MOD_CLK_DEBE0,       "debe0"     ),
	disp_clk_inf(MOD_CLK_DEBE1,       "debe1"     ),
	disp_clk_inf(MOD_CLK_DEFE0,       "defe0"     ),
	disp_clk_inf(MOD_CLK_DEFE1,       "defe1"     ),
	disp_clk_inf(MOD_CLK_LCD0CH0,     "lcd0ch0"   ),
	disp_clk_inf(MOD_CLK_LCD0CH1,     "lcd0ch1"   ),
	disp_clk_inf(MOD_CLK_LCD1CH0,     "lcd1ch0"   ),
	disp_clk_inf(MOD_CLK_LCD1CH1,     "lcd1ch1"   ),
	disp_clk_inf(MOD_CLK_HDMI,        "hdmi"      ),
	disp_clk_inf(MOD_CLK_HDMI_DDC,    "hdmi_ddc"  ),
	disp_clk_inf(MOD_CLK_MIPIDSIS,    "mipidsi"  ),
	disp_clk_inf(MOD_CLK_MIPIDSIP,    "mipidphy"  ),
	disp_clk_inf(MOD_CLK_IEPDRC0,     "drc0"    ),
	disp_clk_inf(MOD_CLK_IEPDRC1,     "drc1"   ),
	disp_clk_inf(MOD_CLK_IEPDEU0,     "deu0"   ),
	disp_clk_inf(MOD_CLK_IEPDEU1,     "deu1"   ),
	disp_clk_inf(MOD_CLK_LVDS,        "lvds"      ),
};
#elif defined CONFIG_ARCH_SUN8IW3P1
static __disp_clk_t disp_clk_tbl[] =
{
	disp_clk_inf(SYS_CLK_PLL3,        "pll3"      ),
	disp_clk_inf(SYS_CLK_PLL7,        "none"      ),
	disp_clk_inf(SYS_CLK_PLL9,        "pll9"      ),
	disp_clk_inf(SYS_CLK_PLL10,       "pll10"     ),
	disp_clk_inf(SYS_CLK_PLL3X2,      "pll3x2"    ),
	disp_clk_inf(SYS_CLK_PLL6,        "pll6"      ),
	disp_clk_inf(SYS_CLK_PLL6x2,      "pll6x2"    ),
	disp_clk_inf(SYS_CLK_PLL7X2,      "none"      ),
	disp_clk_inf(SYS_CLK_MIPIPLL,     "pll_mipi"  ),

	disp_clk_inf(MOD_CLK_DEBE0,       "debe0"     ),
	disp_clk_inf(MOD_CLK_DEBE1,       "none"      ),
	disp_clk_inf(MOD_CLK_DEFE0,       "defe0"     ),
	disp_clk_inf(MOD_CLK_DEFE1,       "none"      ),
	disp_clk_inf(MOD_CLK_LCD0CH0,     "lcd0ch0"   ),
	disp_clk_inf(MOD_CLK_LCD0CH1,     "none"      ),
	disp_clk_inf(MOD_CLK_LCD1CH0,     "lcd1ch0"   ),
	disp_clk_inf(MOD_CLK_LCD1CH1,     "none"      ),
	disp_clk_inf(MOD_CLK_HDMI,        "none"      ),
	disp_clk_inf(MOD_CLK_HDMI_DDC,    "none"      ),
	disp_clk_inf(MOD_CLK_MIPIDSIS,    "mipidsi"   ),
	disp_clk_inf(MOD_CLK_MIPIDSIP,    "mipidphy"  ),
	disp_clk_inf(MOD_CLK_IEPDRC0,     "drc0"      ),
	disp_clk_inf(MOD_CLK_IEPDRC1,     "none"      ),
	disp_clk_inf(MOD_CLK_IEPDEU0,     "none"      ),
	disp_clk_inf(MOD_CLK_IEPDEU1,     "none"      ),
	disp_clk_inf(MOD_CLK_LVDS,        "lvds"      ),
};
#elif defined CONFIG_ARCH_SUN8IW5P1
static __disp_clk_t disp_clk_tbl[] =
{
	disp_clk_inf(SYS_CLK_PLL3,        "pll_video" ),
	disp_clk_inf(SYS_CLK_PLL10,       "pll_de"    ),
	disp_clk_inf(SYS_CLK_MIPIPLL,     "pll_mipi"  ),

	disp_clk_inf(MOD_CLK_DEBE0,       "debe0"     ),
	disp_clk_inf(MOD_CLK_DEBE1,       "none"      ),
	disp_clk_inf(MOD_CLK_DEFE0,       "defe0"     ),
	disp_clk_inf(MOD_CLK_DEFE1,       "none"      ),
	disp_clk_inf(MOD_CLK_LCD0CH0,     "lcd0ch0"   ),
	disp_clk_inf(MOD_CLK_LCD0CH1,     "none"      ),
	disp_clk_inf(MOD_CLK_LCD1CH0,     "none"   ),
	disp_clk_inf(MOD_CLK_LCD1CH1,     "none"      ),
	disp_clk_inf(MOD_CLK_HDMI,        "none"      ),
	disp_clk_inf(MOD_CLK_HDMI_DDC,    "none"      ),
	disp_clk_inf(MOD_CLK_MIPIDSIS,    "mipidsi"   ),
	disp_clk_inf(MOD_CLK_MIPIDSIP,    "mipidphy"  ),
	disp_clk_inf(MOD_CLK_IEPDRC0,     "drc0"      ),
	disp_clk_inf(MOD_CLK_IEPDRC1,     "none"      ),
	disp_clk_inf(MOD_CLK_IEPDEU0,     "none"      ),
	disp_clk_inf(MOD_CLK_IEPDEU1,     "none"      ),
	disp_clk_inf(MOD_CLK_LVDS,        "lvds"      ),
	disp_clk_inf(MOD_CLK_SAT0,        "sat"       ),
};
#elif defined CONFIG_ARCH_SUN6I
static __disp_clk_t disp_clk_tbl[] =
{
    disp_clk_inf(SYS_CLK_PLL3,        CLK_SYS_PLL3      ),
    disp_clk_inf(SYS_CLK_PLL7,        CLK_SYS_PLL7      ),
    disp_clk_inf(SYS_CLK_PLL9,        CLK_SYS_PLL9      ),
    disp_clk_inf(SYS_CLK_PLL10,       CLK_SYS_PLL10     ),
    disp_clk_inf(SYS_CLK_PLL3X2,      CLK_SYS_PLL3X2    ),
    disp_clk_inf(SYS_CLK_PLL6,        CLK_SYS_PLL6      ),
    disp_clk_inf(SYS_CLK_PLL6x2,      CLK_SYS_PLL6X2    ),
    disp_clk_inf(SYS_CLK_PLL7X2,      CLK_SYS_PLL7X2    ),
    disp_clk_inf(SYS_CLK_MIPIPLL,     CLK_SYS_MIPI_PLL  ),

    disp_clk_inf(MOD_CLK_DEBE0,       CLK_MOD_DEBE0     ),
    disp_clk_inf(MOD_CLK_DEBE1,       CLK_MOD_DEBE1     ),
    disp_clk_inf(MOD_CLK_DEFE0,       CLK_MOD_DEFE0     ),
    disp_clk_inf(MOD_CLK_DEFE1,       CLK_MOD_DEFE1     ),
    disp_clk_inf(MOD_CLK_LCD0CH0,     CLK_MOD_LCD0CH0   ),
    disp_clk_inf(MOD_CLK_LCD0CH1,     CLK_MOD_LCD0CH1   ),
    disp_clk_inf(MOD_CLK_LCD1CH0,     CLK_MOD_LCD1CH0   ),
    disp_clk_inf(MOD_CLK_LCD1CH1,     CLK_MOD_LCD1CH1   ),
    disp_clk_inf(MOD_CLK_HDMI,        CLK_MOD_HDMI      ),
    disp_clk_inf(MOD_CLK_HDMI_DDC,    CLK_MOD_HDMI_DDC  ),
    disp_clk_inf(MOD_CLK_MIPIDSIS,    CLK_MOD_MIPIDSIS  ),
    disp_clk_inf(MOD_CLK_MIPIDSIP,    CLK_MOD_MIPIDSIP  ),
    disp_clk_inf(MOD_CLK_IEPDRC0,     CLK_MOD_IEPDRC0   ),
    disp_clk_inf(MOD_CLK_IEPDRC1,     CLK_MOD_IEPDRC1   ),
    disp_clk_inf(MOD_CLK_IEPDEU0,     CLK_MOD_IEPDEU0   ),
    disp_clk_inf(MOD_CLK_IEPDEU1,     CLK_MOD_IEPDEU1   ),
    disp_clk_inf(MOD_CLK_LVDS,        CLK_MOD_LVDS      ),

    disp_clk_inf(AHB_CLK_MIPIDSI,     CLK_AHB_MIPIDSI   ),
    disp_clk_inf(AHB_CLK_LCD0,        CLK_AHB_LCD0      ),
    disp_clk_inf(AHB_CLK_LCD1,        CLK_AHB_LCD1      ),
    disp_clk_inf(AHB_CLK_HDMI,        CLK_AHB_HDMI      ),
    disp_clk_inf(AHB_CLK_DEBE0,       CLK_AHB_DEBE0     ),
    disp_clk_inf(AHB_CLK_DEBE1,       CLK_AHB_DEBE1     ),
    disp_clk_inf(AHB_CLK_DEFE0,       CLK_AHB_DEFE0     ),
    disp_clk_inf(AHB_CLK_DEFE1,       CLK_AHB_DEFE1     ),
    disp_clk_inf(AHB_CLK_DEU0,        CLK_AHB_DEU0      ),
    disp_clk_inf(AHB_CLK_DEU1,        CLK_AHB_DEU1      ),
    disp_clk_inf(AHB_CLK_DRC0,        CLK_AHB_DRC0      ),
    disp_clk_inf(AHB_CLK_DRC1,        CLK_AHB_DRC1      ),

    disp_clk_inf(DRAM_CLK_DRC0,       CLK_DRAM_DRC0     ),
    disp_clk_inf(DRAM_CLK_DRC1,       CLK_DRAM_DRC1     ),
    disp_clk_inf(DRAM_CLK_DEU0,       CLK_DRAM_DEU0     ),
    disp_clk_inf(DRAM_CLK_DEU1,       CLK_DRAM_DEU1     ),
    disp_clk_inf(DRAM_CLK_DEFE0,      CLK_DRAM_DEFE0    ),
    disp_clk_inf(DRAM_CLK_DEFE1,      CLK_DRAM_DEFE1    ),
    disp_clk_inf(DRAM_CLK_DEBE0,      CLK_DRAM_DEBE0    ),
    disp_clk_inf(DRAM_CLK_DEBE1,      CLK_DRAM_DEBE1    ),
};
#else
static __disp_clk_t disp_clk_tbl[] =
{
	disp_clk_inf(SYS_CLK_PLL3,        "none"      ),
	disp_clk_inf(SYS_CLK_PLL7,        "pll7"      ),
	disp_clk_inf(SYS_CLK_PLL9,        "pll9"      ),
	disp_clk_inf(SYS_CLK_PLL10,       "pll10"     ),
	disp_clk_inf(SYS_CLK_PLL3X2,      "pll3x2"    ),
	disp_clk_inf(SYS_CLK_PLL6,        "pll6"      ),
	disp_clk_inf(SYS_CLK_PLL6x2,      "pll6x2"    ),
	disp_clk_inf(SYS_CLK_PLL7X2,      "none"      ),
	disp_clk_inf(SYS_CLK_MIPIPLL,     "pll_mipi"  ),

	disp_clk_inf(MOD_CLK_DEBE0,       "de"        ),
	disp_clk_inf(MOD_CLK_DEBE1,       "de"        ),
	disp_clk_inf(MOD_CLK_LCD0CH0,     "lcd0"   ),
	disp_clk_inf(MOD_CLK_LCD0CH1,     "none"      ),
	disp_clk_inf(MOD_CLK_LCD1CH0,     "lcd1"   ),
	disp_clk_inf(MOD_CLK_LCD1CH1,     "none"      ),
	disp_clk_inf(MOD_CLK_HDMI,        "none"      ),
	disp_clk_inf(MOD_CLK_HDMI_DDC,    "none"      ),
	disp_clk_inf(MOD_CLK_MIPIDSIS,    "mipi_dsi0"   ),
	disp_clk_inf(MOD_CLK_MIPIDSIP,    "mipi_dsi1"  ),
	disp_clk_inf(MOD_CLK_LVDS,        "lvds"      ),
};
#endif
#endif
s32 osal_init_clk_pll(void)
{
	u32 i;
	u32 count;
	s32 retCode = -1;
	struct clk* hSysClk = NULL;

	count = sizeof(disp_clk_pll_tbl) / sizeof(__disp_clk_t);

	for(i= 0; i < count; i++) {
		if(disp_clk_pll_tbl[i].freq) {
			hSysClk = clk_get(NULL, disp_clk_pll_tbl[i].name);
			retCode = clk_set_rate(hSysClk, disp_clk_pll_tbl[i].freq);
			if(0 != retCode) {
				__wrn("Fail to set nFreq[%d] for sys clk[%s].\n", disp_clk_pll_tbl[i].freq, disp_clk_pll_tbl[i].name);
				clk_put(hSysClk);
				return retCode;
			}
			clk_put(hSysClk);
			hSysClk = NULL;
		}
	}

	return 0;
}

static __hdle osal_ccmu_get_clk_by_name(__disp_clk_id_t clk_no)
{
	u32 i;
	u32 count;

	count = sizeof(disp_clk_pll_tbl)/sizeof(__disp_clk_t);

	for(i=0;i<count;i++) {
		if((disp_clk_pll_tbl[i].id == clk_no)) {
			//memcpy(clk_name, disp_clk_tbl[i].name, strlen(disp_clk_tbl[i].name)+1);
			return (__hdle)&disp_clk_pll_tbl[i];
		}
	}

	count = sizeof(disp_clk_mod_tbl) / sizeof(__disp_clk_t);

	for(i = 0; i < count; i++) {
		if((disp_clk_mod_tbl[i].id == clk_no)) {
			//memcpy(clk_name, disp_clk_tbl[i].name, strlen(disp_clk_tbl[i].name)+1);
			return (__hdle)&disp_clk_mod_tbl[i];
		}
	}

	return 0;
}

static u32 osal_ccmu_get_clk_src_by_name(char *src_name)
{
	u32 i;
	u32 count;

	count = sizeof(disp_clk_pll_tbl)/sizeof(__disp_clk_t);

	for(i=0;i<count;i++) {
		if(!strcmp(disp_clk_pll_tbl[i].name, src_name)) {
			return disp_clk_pll_tbl[i].id;
		}
	}

	count = sizeof(disp_clk_mod_tbl) / sizeof(__disp_clk_t);

	for(i = 0; i < count; i++) {
		if(!strcmp(disp_clk_mod_tbl[i].name, src_name)) {
			return disp_clk_mod_tbl[i].id;
		}
	}

	return 0;
}

s32 OSAL_CCMU_SetSrcFreq( u32 nSclkNo, u32 nFreq )
{
	struct clk* hSysClk = NULL;
	s32 retCode = -1;
	__disp_clk_t *disp_clk = NULL;

	disp_clk = (__disp_clk_t*)osal_ccmu_get_clk_by_name(nSclkNo);
	if(disp_clk == NULL) {
		__inf("Fail to get clk name from clk id [%d].\n", nSclkNo);
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_SetSrcFreq, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}
	__inf("OSAL_CCMU_SetSrcFreq,  <%s,%d>\n", disp_clk->name, nFreq);

	hSysClk = clk_get(NULL, disp_clk->name);

	if(NULL == hSysClk || IS_ERR(hSysClk)) {
		__wrn("Fail to get handle for system clock %s.\n", disp_clk->name);
		return -1;
	}

	if(nFreq == clk_get_rate(hSysClk)) {
		__inf("Sys clk %s freq is alreay %d, not need to set.\n", disp_clk->name, nFreq);
		clk_put(hSysClk);
		return 0;
	}
	__inf("OSAL_CCMU_SetSrcFreq, clk hdl=0x%x\n", (int)hSysClk);

	retCode = clk_set_rate(hSysClk, nFreq);
	if(0 != retCode) {
		__wrn("Fail to set nFreq[%d] for sys clk[%d].\n", nFreq, nSclkNo);
		clk_put(hSysClk);
		return retCode;
	}
	clk_put(hSysClk);
	hSysClk = NULL;

	return retCode;
}

u32 OSAL_CCMU_GetSrcFreq( u32 nSclkNo )
{
	struct clk* hSysClk = NULL;
	u32 nFreq = 0;
	__disp_clk_t *disp_clk = NULL;

	disp_clk = (__disp_clk_t *)osal_ccmu_get_clk_by_name(nSclkNo);
	if(disp_clk == NULL) {
		__inf("Fail to get clk name from clk id [%d].\n", nSclkNo);
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_GetSrcFreq, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	__inf("OSAL_CCMU_GetSrcFreq,  clk_name[%d]=%s\n", nSclkNo,disp_clk->name);

	hSysClk = clk_get(NULL, disp_clk->name);

	if(NULL == hSysClk || IS_ERR(hSysClk)) {
		__wrn("Fail to get handle for system clock %s.\n", disp_clk->name);
		return -1;
	}
	nFreq = clk_get_rate(hSysClk);
	clk_put(hSysClk);
	hSysClk = NULL;
	__inf("OSAL_CCMU_GetSrcFreq, freq of clk %s is %d\n", disp_clk->name, nFreq);

	return nFreq;
}

__hdle OSAL_CCMU_OpenMclk( s32 nMclkNo )
{
	struct clk* hModClk = NULL;
	__disp_clk_t *disp_clk = NULL;

	disp_clk = (__disp_clk_t*)osal_ccmu_get_clk_by_name(nMclkNo);
	if(disp_clk == NULL) {
		__inf("Fail to get clk name from clk id [%d].\n", nMclkNo);
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_OpenMclk, name of clk %d is none! \n", disp_clk->id);
		return (__hdle)disp_clk;
	}

	hModClk = clk_get(NULL, disp_clk->name);

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("clk_get %s fail\n", disp_clk->name);
	}

	disp_clk->hdl = hModClk;

	__inf("OSAL_CCMU_OpenMclk,  clk_name[%d]=%s, hdl=0x%x\n", nMclkNo,disp_clk->name, (unsigned int)hModClk);

	return (__hdle)disp_clk;
}

s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_CloseMclk, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;
	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	clk_put(hModClk);
	disp_clk->hdl = NULL;

	return 0;
}

s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk)
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hSysClk = NULL;
	struct clk* hModClk;
	s32 retCode = -1;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_SetMclkSrc, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;
	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	if(NULL == disp_clk->src_name) {
		__wrn("NULL pll src\n");
		return -1;
	}

	__inf("OSAL_CCMU_SetMclkSrc, modclk=%s, src=%s\n", disp_clk->name, disp_clk->src_name);

	hSysClk = clk_get(NULL,  disp_clk->src_name);

	if((NULL == hSysClk) || (IS_ERR(hSysClk))) {
		__wrn("Fail to get handle for system clock %s.\n", disp_clk->src_name);
		return -1;
	}

	if(clk_get_parent(hModClk) == hSysClk) {
		__inf("Parent is alreay %s, not need to set.\n", disp_clk->src_name);
		clk_put(hSysClk);
		return 0;
	}

	retCode = clk_set_parent(hModClk, hSysClk);
	if(0 != retCode) {
		__wrn("Fail to set parent %s for clk %s\n", disp_clk->src_name, disp_clk->name);
		clk_put(hSysClk);
		return -1;
	}

	clk_put(hSysClk);

	return retCode;
}

u32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_SetMclkSrc, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	return osal_ccmu_get_clk_src_by_name(disp_clk->src_name);
}

s32 OSAL_CCMU_SetMclkFreq( __hdle hMclk, u32 nFreq )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_SetMclkDiv, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	if(!nFreq)
		nFreq = disp_clk->freq;

	if(!nFreq) {
		__wrn("clk %d freq is 0, fail\n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	__inf("OSAL_CCMU_SetMclkFreq<%s,%u>\n",disp_clk->name, nFreq);

	__inf("clk_set_rate<%s,%u>\n",disp_clk->name, nFreq);

	return clk_set_rate(hModClk, nFreq);
}

u32 OSAL_CCMU_GetMclkFreq( __hdle hMclk )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk = (struct clk*)hMclk;
	u32 mod_freq;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_GetMclkFreq, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	__inf("OSAL_CCMU_GetMclkFreq of clk %s\n", disp_clk->name);

	mod_freq = clk_get_rate(hModClk);

	if(mod_freq == 0) {
		return 0;
	}
	__inf("OSAL_CCMU_GetMclkFreq, mode_rate=%d\n", mod_freq);

	return mod_freq;
}

s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, s32 bOnOff )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;
	s32 ret = 0;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("OSAL_CCMU_MclkOnOff, bOnOff=%d, NULL hdle\n", bOnOff);
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_MclkOnOff, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	__inf("OSAL_CCMU_MclkOnOff<%s,%d>\n", disp_clk->name, bOnOff);

	if(bOnOff) {
#if !defined(CONFIG_ARCH_SUN6I)
		//if(hModClk->enable_count == 0)
#endif
		{
			ret = clk_prepare_enable(hModClk);
			__inf("OSAL_CCMU_MclkOnOff, clk_prepare_enable %s, ret=%d\n", disp_clk->name, ret);
		}
	} else {
#if !defined(CONFIG_ARCH_SUN6I)
		if(hModClk->enable_count > 0)
#endif
		{
			clk_disable(hModClk);
			__inf("OSAL_CCMU_MclkOnOff, clk_disable_unprepare %s, ret=%d\n", disp_clk->name, ret);
		}
	}
	return ret;
}

s32 OSAL_CCMU_MclkReset(__hdle hMclk, s32 bReset)
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__inf("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_MclkReset, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	__inf("OSAL_CCMU_MclkReset<%s,%d>\n",disp_clk->name, bReset);

	if(bReset) {
		//sunxi_periph_reset_assert(hModClk);
	}	else {
		//sunxi_periph_reset_deassert(hModClk);
	}
#if defined(CONFIG_ARCH_SUN6I)
	clk_reset(hModClk, bReset);
#endif

	return 0;
}
#else

typedef u32 CSP_CCM_sysClkNo_t;


s32 OSAL_CCMU_SetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo, u32 nFreq )
{
	return 0;
}

u32 OSAL_CCMU_GetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo )
{
	return 0;
}

__hdle OSAL_CCMU_OpenMclk( s32 nMclkNo )
{
	return 0;
}

s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
	return 0;
}

s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk)
{
	return 0;
}

u32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
	return 0;
}

s32 OSAL_CCMU_SetMclkFreq( __hdle hMclk, u32 nFreq)
{
	return 0;
}

u32 OSAL_CCMU_GetMclkFreq( __hdle hMclk )
{
	return 0;
}

s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, s32 bOnOff )
{
	return 0;
}

s32 OSAL_CCMU_MclkReset(__hdle hMclk, s32 bReset)
{
	return 0;
}
#endif


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
#include <linux/clk/sunxi.h>
#include <linux/clk-private.h>



#ifndef __OSAL_CLOCK_MASK__

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

	disp_clk_inf(AHB_CLK_MIPIDSI,     "none"   ),
	disp_clk_inf(AHB_CLK_LCD0,        "none"      ),
	disp_clk_inf(AHB_CLK_LCD1,        "none"      ),
	disp_clk_inf(AHB_CLK_HDMI,        "none"      ),
	disp_clk_inf(AHB_CLK_DEBE0,       "none"     ),
	disp_clk_inf(AHB_CLK_DEBE1,       "none"     ),
	disp_clk_inf(AHB_CLK_DEFE0,       "none"     ),
	disp_clk_inf(AHB_CLK_DEFE1,       "none"     ),
	disp_clk_inf(AHB_CLK_DEU0,        "none"      ),
	disp_clk_inf(AHB_CLK_DEU1,        "none"      ),
	disp_clk_inf(AHB_CLK_DRC0,        "none"      ),
	disp_clk_inf(AHB_CLK_DRC1,        "none"      ),

	disp_clk_inf(DRAM_CLK_DRC0,       "none"     ),
	disp_clk_inf(DRAM_CLK_DRC1,       "none"     ),
	disp_clk_inf(DRAM_CLK_DEU0,       "none"     ),
	disp_clk_inf(DRAM_CLK_DEU1,       "none"     ),
	disp_clk_inf(DRAM_CLK_DEFE0,      "none"    ),
	disp_clk_inf(DRAM_CLK_DEFE1,      "none"    ),
	disp_clk_inf(DRAM_CLK_DEBE0,      "none"    ),
	disp_clk_inf(DRAM_CLK_DEBE1,      "none"    ),
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

	disp_clk_inf(AHB_CLK_MIPIDSI,     "none"   ),
	disp_clk_inf(AHB_CLK_LCD0,        "none"      ),
	disp_clk_inf(AHB_CLK_LCD1,        "none"      ),
	disp_clk_inf(AHB_CLK_HDMI,        "none"      ),
	disp_clk_inf(AHB_CLK_DEBE0,       "none"     ),
	disp_clk_inf(AHB_CLK_DEBE1,       "none"     ),
	disp_clk_inf(AHB_CLK_DEFE0,       "none"     ),
	disp_clk_inf(AHB_CLK_DEFE1,       "none"     ),
	disp_clk_inf(AHB_CLK_DEU0,        "none"      ),
	disp_clk_inf(AHB_CLK_DEU1,        "none"      ),
	disp_clk_inf(AHB_CLK_DRC0,        "none"      ),
	disp_clk_inf(AHB_CLK_DRC1,        "none"      ),

	disp_clk_inf(DRAM_CLK_DRC0,       "none"     ),
	disp_clk_inf(DRAM_CLK_DRC1,       "none"     ),
	disp_clk_inf(DRAM_CLK_DEU0,       "none"     ),
	disp_clk_inf(DRAM_CLK_DEU1,       "none"     ),
	disp_clk_inf(DRAM_CLK_DEFE0,      "none"    ),
	disp_clk_inf(DRAM_CLK_DEFE1,      "none"    ),
	disp_clk_inf(DRAM_CLK_DEBE0,      "none"    ),
	disp_clk_inf(DRAM_CLK_DEBE1,      "none"    ),
};

#else
static __disp_clk_t disp_clk_tbl[] =
{

};
#endif
static __hdle osal_ccmu_get_clk_by_name(__disp_clk_id_t clk_no)
{
	__u32 i;
	__u32 count;

	count = sizeof(disp_clk_tbl)/sizeof(__disp_clk_t);

	for(i=0;i<count;i++) {
		if((disp_clk_tbl[i].id == clk_no)) {
			//memcpy(clk_name, disp_clk_tbl[i].name, strlen(disp_clk_tbl[i].name)+1);
			return (__hdle)&disp_clk_tbl[i];
		}
	}

	return 0;
}

__s32 OSAL_CCMU_SetSrcFreq( __u32 nSclkNo, __u32 nFreq )
{
	struct clk* hSysClk = NULL;
	s32 retCode = -1;
	__disp_clk_t *disp_clk = NULL;


	disp_clk = (__disp_clk_t*)osal_ccmu_get_clk_by_name(nSclkNo);
	if(disp_clk == NULL) {
		__wrn("Fail to get clk name from clk id [%d].\n", nSclkNo);
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

__u32 OSAL_CCMU_GetSrcFreq( __u32 nSclkNo )
{
	struct clk* hSysClk = NULL;
	u32 nFreq = 0;
	__disp_clk_t *disp_clk = NULL;

	disp_clk = (__disp_clk_t *)osal_ccmu_get_clk_by_name(nSclkNo);
	if(disp_clk == NULL) {
		__wrn("Fail to get clk name from clk id [%d].\n", nSclkNo);
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

__hdle OSAL_CCMU_OpenMclk( __s32 nMclkNo )
{
	struct clk* hModClk = NULL;
	__disp_clk_t *disp_clk = NULL;

	disp_clk = (__disp_clk_t*)osal_ccmu_get_clk_by_name(nMclkNo);
	if(disp_clk == NULL) {
		__wrn("Fail to get clk name from clk id [%d].\n", nMclkNo);
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

__s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__wrn("NULL hdle\n");
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

__s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk, __u32 nSclkNo )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hSysClk = NULL;
	struct clk* hModClk;
	s32 retCode = -1;
	__disp_clk_t *disp_clk_sys;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__wrn("NULL hdle\n");
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

	disp_clk_sys = (__disp_clk_t*)osal_ccmu_get_clk_by_name(nSclkNo);
	if(disp_clk_sys == NULL) {
		__wrn("Fail to get clk name from clk id [%d].\n", nSclkNo);
		return -1;
	} else if(!strcmp(disp_clk_sys->name, "none")) {
		__inf("OSAL_CCMU_GetSrcFreq, name of clk %d is none! \n", disp_clk_sys->id);
		return -1;
	}

	__inf("OSAL_CCMU_SetMclkSrc, modclk=%s, src=%s\n", disp_clk->name, disp_clk_sys->name);

	__inf("OSAL_CCMU_SetMclkSrc,  clk_name[%d]=%s\n", nSclkNo, disp_clk_sys->name);

	hSysClk = clk_get(NULL,  disp_clk_sys->name);

	if((NULL == hSysClk) || (IS_ERR(hSysClk))) {
		__wrn("Fail to get handle for system clock %s.\n", disp_clk_sys->name);
		return -1;
	}

	if(clk_get_parent(hModClk) == hSysClk) {
		__inf("Parent is alreay %s, not need to set.\n", disp_clk_sys->name);
		clk_put(hSysClk);
		return 0;
	}
	retCode = clk_set_parent(hModClk, hSysClk);
	if(0 != retCode) {
		__wrn("Fail to set parent %s for clk %s\n", disp_clk_sys->name, disp_clk->name);
		clk_put(hSysClk);
		return -1;
	}

	clk_put(hSysClk);

	return retCode;
}

__s32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
	int sysClkNo = 0;
#if 0
	struct clk* hModClk = (struct clk*)hMclk;
	struct clk* hParentClk = clk_get_parent(hModClk);
	const int TOTAL_SYS_CLK = sizeof(_sysClkName)/sizeof(char*);

	for (; sysClkNo <  TOTAL_SYS_CLK; sysClkNo++) {
		struct clk* tmpSysClk = clk_get(NULL, _sysClkName[sysClkNo]);

		if(tmpSysClk == NULL)
		continue;

		if(hParentClk == tmpSysClk) {
			clk_put(tmpSysClk);
			break;
		}
		clk_put(tmpSysClk);
	}

	if(sysClkNo >= TOTAL_SYS_CLK) {
		__wrn("Failed to get parent clk.\n");
		return -1;
	}
#endif
	return sysClkNo;
}

__s32 OSAL_CCMU_SetMclkDiv( __hdle hMclk, __s32 nDiv )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;
	struct clk* hParentClk;
	u32         srcRate;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__wrn("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_SetMclkDiv, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	__inf("OSAL_CCMU_SetMclkDiv<%s,%d>\n",disp_clk->name, nDiv);

	if(nDiv == 0) {
		return -1;
	}

	hParentClk  = clk_get_parent(hModClk);
	if(NULL == hParentClk || IS_ERR(hParentClk)) {
		__inf("fail to get parent of clk %s \n", disp_clk->name);
		return -1;
	}

	srcRate = clk_get_rate(hParentClk);

	__inf("clk_set_rate<%s,%d>, src_rate=%d\n",disp_clk->name, srcRate/nDiv, srcRate);
	return clk_set_rate(hModClk, srcRate/nDiv);
}

__u32 OSAL_CCMU_GetMclkDiv( __hdle hMclk )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk = (struct clk*)hMclk;
	struct clk* hParentClk;
	u32 mod_freq;
	u32         srcRate;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__wrn("NULL hdle\n");
		return -1;
	} else if(!strcmp(disp_clk->name, "none")) {
		__inf("OSAL_CCMU_GetMclkDiv, name of clk %d is none! \n", disp_clk->id);
		return -1;
	}

	hModClk = disp_clk->hdl;

	if(NULL == hModClk || IS_ERR(hModClk)) {
		__wrn("NULL hdle\n");
		return -1;
	}

	__inf("OSAL_CCMU_GetMclkDiv of clk %s\n", disp_clk->name);

	hParentClk  = clk_get_parent(hModClk);
	if(NULL == hParentClk || IS_ERR(hParentClk)) {
		__wrn("fail to get parent of clk %s \n", disp_clk->name);
		return -1;
	}

	srcRate = clk_get_rate(hParentClk);
	mod_freq = clk_get_rate(hModClk);

	if(mod_freq == 0) {
		return 0;
	}
	__inf("OSAL_CCMU_GetMclkDiv, scrRate=%d, mode_rate=%d, div=%d\n", srcRate, mod_freq, srcRate/mod_freq);

	return srcRate/mod_freq;
}

__s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, __s32 bOnOff )
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;
	__s32 ret = 0;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__wrn("OSAL_CCMU_MclkOnOff, bOnOff=%d, NULL hdle\n", bOnOff);
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
		if(hModClk->enable_count == 0)
		{
			ret = clk_prepare_enable(hModClk);
			__inf("OSAL_CCMU_MclkOnOff, clk_prepare_enable %s, ret=%d\n", disp_clk->name, ret);
		}
	} else {
		while(hModClk->enable_count > 0)
		{
			clk_disable(hModClk);
			__inf("OSAL_CCMU_MclkOnOff, clk_disable_unprepare %s, ret=%d\n", disp_clk->name, ret);
		}
	}
	return ret;
}

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset)
{
	__disp_clk_t *disp_clk = (__disp_clk_t *)hMclk;
	struct clk* hModClk;

	if(NULL == disp_clk || IS_ERR(disp_clk)) {
		__wrn("NULL hdle\n");
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

	return 0;
}
#else

typedef __u32 CSP_CCM_sysClkNo_t;


__s32 OSAL_CCMU_SetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo, __u32 nFreq )
{
	return 0;
}

__u32 OSAL_CCMU_GetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo )
{
	return 0;
}

__hdle OSAL_CCMU_OpenMclk( __s32 nMclkNo )
{
	return 0;
}

__s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
	return 0;
}

__s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk, CSP_CCM_sysClkNo_t nSclkNo )
{
	return 0;
}

__s32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
	return 0;
}

__s32 OSAL_CCMU_SetMclkDiv( __hdle hMclk, __s32 nDiv )
{
	return 0;
}

__u32 OSAL_CCMU_GetMclkDiv( __hdle hMclk )
{
	return 0;
}

__s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, __s32 bOnOff )
{
	return 0;
}

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset)
{
	return 0;
}
#endif


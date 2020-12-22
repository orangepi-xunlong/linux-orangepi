/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_clock.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 13:40
* Descript: ccmu process for platform standby;
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include "standby_i.h"

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned short *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

#ifdef CONFIG_ARCH_SUN8IW8P1
#else
static void *r_prcm;
#endif

static __ccmu_reg_list_t   *CmuReg;

//==============================================================================
// CLOCK SET FOR SYSTEM STANDBY
//==============================================================================



#ifdef CONFIG_ARCH_SUN8I
static __ccmu_pll1_reg0000_t		CmuReg_Pll1Ctl_tmp;
static __ccmu_sysclk_ratio_reg0050_t	CmuReg_SysClkDiv_tmp;
#ifdef CONFIG_ARCH_SUN8IW8P1
#define PIO_INT_DEB_REG (AW_GPIO_BASE_PA + 0x258)
static __u32 pio_int_deb_back = 0;
static __ccmu_ahb1_ratio_reg0054_t CmuReg_ahb1_tmp;
static __ccmu_ahb1_ratio_reg0054_t CmuReg_ahb1_backup;
static __ccmu_apb2_ratio_reg0058_t CmuReg_apb2_tmp;
static __ccmu_apb2_ratio_reg0058_t CmuReg_apb2_backup;
static __ccmu_ahb2_ratio_reg005c_t CmuReg_ahb2_tmp;
static __ccmu_ahb2_ratio_reg005c_t CmuReg_ahb2_backup;

/*
*********************************************************************************************************
*                           standby_clk_init
*
*Description: ccu init for platform standby
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 standby_clk_init(void)
{
    CmuReg = (__ccmu_reg_list_t   *)IO_ADDRESS(AW_CCM_BASE);
    
    return 0;
}


/*
*********************************************************************************************************
*                           standby_clk_exit
*
*Description: ccu exit for platform standby
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 standby_clk_exit(void)
{
    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_core2losc
*
* Description: switch core clock to 32k low osc.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_core2losc(void)
{
    unsigned int tmp;
    //CmuReg->SysClkDiv.CpuClkSrc = 0;
    /* cpu frequency is internal Losc hz */
    tmp = readl(&(CmuReg->SysClkDiv));
    tmp &= (~(0x00030000));
    writel(tmp, &(CmuReg->SysClkDiv));

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_core2hosc
*
* Description: switch core clock to 24M high osc.
*
* Arguments  : none
*
*********************************************************************************************************
*/
__s32 standby_clk_core2hosc(void)
{	
    CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval; 
    CmuReg_SysClkDiv_tmp.bits.CpuClkSrc = 1;
    CmuReg->SysClkDiv.dwval = CmuReg_SysClkDiv_tmp.dwval;

    return 0;
}

/*
*********************************************************************************************************
*                                     standby_clk_core2pll
*
* Description: switch core clock to core pll.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_core2pll(void)
{
    CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval; 
    CmuReg_SysClkDiv_tmp.bits.CpuClkSrc = 2;
    CmuReg->SysClkDiv.dwval = CmuReg_SysClkDiv_tmp.dwval;

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_plldisable
*
* Description: disable dram pll.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_plldisable(void)
{
    unsigned int tmp;

    //pll1
    tmp = readl(&(CmuReg->Pll1Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll1Ctl));

    //pll6: for periph0
    tmp = readl(&(CmuReg->Pll6Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll6Ctl));

    //pll9: periph 1
    tmp = readl(&(CmuReg->Pll9Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll9Ctl));

#if 0	
    //pll2: for audio
    tmp = readl(&(CmuReg->Pll2Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll2Ctl));

    //pll3: for video
    tmp = readl(&(CmuReg->Pll3Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll3Ctl));

    //pll4: for ve
    tmp = readl(&(CmuReg->Pll4Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll4Ctl));

    //pll5: for ddr
    tmp = readl(&(CmuReg->Pll5Ctl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->Pll5Ctl));

    //pll7: for isp
    tmp = readl(&(CmuReg->PllIspCtl));
    tmp &= (~(0x80000000));
    writel(tmp, &(CmuReg->PllIspCtl));
#endif 

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_pllenable
*
* Description: enable dram pll.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_pllenable(void)
{
    unsigned int tmp;

    //pll1
    tmp = readl(&(CmuReg->Pll1Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll1Ctl));

    //pll6: for periph0
    tmp = readl(&(CmuReg->Pll6Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll6Ctl));

    //pll9: periph 1
    tmp = readl(&(CmuReg->Pll9Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll9Ctl));

#if 0
    //pll2: for audio
    tmp = readl(&(CmuReg->Pll2Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll2Ctl));

    //pll3: for video
    tmp = readl(&(CmuReg->Pll3Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll3Ctl));

    //pll4: for ve
    tmp = readl(&(CmuReg->Pll4Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll4Ctl));

    //pll5: for ddr
    tmp = readl(&(CmuReg->Pll5Ctl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->Pll5Ctl));

    //pll7: for isp
    tmp = readl(&(CmuReg->PllIspCtl));
    tmp |= ((0x80000000));
    writel(tmp, &(CmuReg->PllIspCtl));
#endif 

    return 0;
}
#if 0
/*
*********************************************************************************************************
*                                     standby_clk_pll1enable
*
* Description: enable pll1.
*
* Arguments  : none
*
*********************************************************************************************************
*/
__s32 standby_clk_pll1enable(void)
{
    CmuReg_Pll1Ctl_tmp.dwval = CmuReg->Pll1Ctl.dwval;
    CmuReg_Pll1Ctl_tmp.bits.PLLEn = 1;
    CmuReg->Pll1Ctl.dwval = CmuReg_Pll1Ctl_tmp.dwval;

    return 0;
}
#endif


/*
*********************************************************************************************************
*                                     standby_clk_hoscdisable
*
* Description: disable HOSC.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_hoscdisable(void)
{
    unsigned int tmp;
    //0xf1c00000 + 0xf4 (system_ctrl: pll ctrl reg1)
    //bit2: hosc; 
    //bit1: ldo for analog; 
    //bit0: ldo for digital

    //keyfield set to: 0xa7
    tmp = readl((void *)(0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    tmp |= ((0xa7000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    //disable hosc
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0x00000004));
    writel(tmp, (0xf1c00000 + 0xf4));

    //keyfield set to: 0
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_hoscenable
*
* Description: enable HOSC.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_hoscenable(void)
{
    unsigned int tmp;
    //0xf1c00000 + 0xf4 (system_ctrl: pll ctrl reg1)
    //bit2: hosc; 
    //bit1: ldo for analog; 
    //bit0: ldo for digital

    //keyfield set to: 0xa7
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    tmp |= ((0xa7000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    //enable hosc
    tmp = readl((0xf1c00000 + 0xf4));
    tmp |= ((0x00000004));
    writel(tmp, (0xf1c00000 + 0xf4));

    //keyfield set to: 0
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_ldodisable
*
* Description: disable LDO.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_ldodisable(void)
{
    unsigned int tmp;
    //0xf1c00000 + 0xf4 (system_ctrl: pll ctrl reg1)
    //bit2: hosc; 
    //bit1: ldo for analog; 
    //bit0: ldo for digital

    //keyfield set to: 0xa7
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    tmp |= ((0xa7000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    //disable ldo
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0x00000003));
    writel(tmp, (0xf1c00000 + 0xf4));

    //keyfield set to: 0
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_ldoenable
*
* Description: enable LDO.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_ldoenable(void)
{
    unsigned int tmp;
    //0xf1c00000 + 0xf4 (system_ctrl: pll ctrl reg1)
    //bit2: hosc; 
    //bit1: ldo for analog; 
    //bit0: ldo for digital

    //keyfield set to: 0xa7
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    tmp |= ((0xa7000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    //enable ldo
    tmp = readl((0xf1c00000 + 0xf4));
    tmp |= ((0x00000003));
    writel(tmp, (0xf1c00000 + 0xf4));

    //keyfield set to: 0
    tmp = readl((0xf1c00000 + 0xf4));
    tmp &= (~(0xff000000));
    writel(tmp, (0xf1c00000 + 0xf4));

    return 0;
}


/*
 *********************************************************************************************************
 *                                     standby_clk_setdiv
 *
* Description: switch core clock to 32k low osc.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_setdiv(struct standby_clk_div_t  *clk_div)
{
    unsigned int tmp;
    
    if(!clk_div)
    {
        return -1;
    }
    
    //axi
    CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval; 
    CmuReg_SysClkDiv_tmp.bits.AXIClkDiv = clk_div->axi_div;
    CmuReg->SysClkDiv.dwval = CmuReg_SysClkDiv_tmp.dwval;

    //ahb1
    CmuReg_ahb1_tmp.dwval = CmuReg->Ahb1Div.dwval; 
    CmuReg_ahb1_tmp.bits.Ahb1Div = clk_div->ahb_div;
    CmuReg_ahb1_tmp.bits.Ahb1PreDiv = clk_div->ahb_pre_div;
    CmuReg->Ahb1Div.dwval = CmuReg_ahb1_tmp.dwval;
    
    //apb2
    CmuReg_apb2_tmp.dwval = CmuReg->Apb2Div.dwval; 
    CmuReg_apb2_tmp.bits.DivM = clk_div->apb_div;
    CmuReg_apb2_tmp.bits.DivN = clk_div->apb_pre_div;
    CmuReg->Apb2Div.dwval = CmuReg_apb2_tmp.dwval;
    
    
    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_getdiv
*
* Description: switch core clock to 32k low osc.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_getdiv(struct standby_clk_div_t  *clk_div)
{
    if(!clk_div)
    {
        return -1;
    }
    
    //axi
    CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval; 
    clk_div->axi_div = CmuReg_SysClkDiv_tmp.bits.AXIClkDiv;
    
    //ahb1
    CmuReg_ahb1_tmp.dwval = CmuReg->Ahb1Div.dwval; 
    clk_div->ahb_div = CmuReg_ahb1_tmp.bits.Ahb1Div;
    clk_div->ahb_pre_div = CmuReg_ahb1_tmp.bits.Ahb1PreDiv;
 
    //apb2
    CmuReg_apb2_tmp.dwval = CmuReg->Apb2Div.dwval; 
    clk_div->apb_div = CmuReg_apb2_tmp.bits.DivM;
    clk_div->apb_pre_div = CmuReg_apb2_tmp.bits.DivN;
    
    return 0;
}

/*
*********************************************************************************************************
*                                     standby_clk_set_pll_factor
*
* Description: set pll factor, target cpu freq is 384M hz
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/

__s32 standby_clk_set_pll_factor(struct pll_factor_t *pll_factor)
{
    __ccmu_pll1_reg0000_t pll1_ctrl;

    if(!pll_factor)
    {
	return -1;
    }

    pll1_ctrl.dwval = readl(&(CmuReg->Pll1Ctl));

    pll1_ctrl.bits.FactorN = pll_factor->FactorN;
    pll1_ctrl.bits.FactorK = pll_factor->FactorK;
    pll1_ctrl.bits.FactorM = pll_factor->FactorM;
    pll1_ctrl.bits.FactorP = pll_factor->FactorP;

    writel(pll1_ctrl.dwval, &(CmuReg->Pll1Ctl));
    //busy_waiting();

    return 0;
}

/*
 *********************************************************************************************************
 *                                     standby_clk_get_pll_factor
 *
 * Description: 
 *
 * Arguments  : none
 *
 * Returns    : 0;
 *********************************************************************************************************
 */

__s32 standby_clk_get_pll_factor(struct pll_factor_t *pll_factor)
{
    __ccmu_pll1_reg0000_t pll1_ctrl;

    if(!pll_factor)
    {
	return -1;
    }

    pll1_ctrl.dwval = readl(&(CmuReg->Pll1Ctl));
	
    pll_factor->FactorN = pll1_ctrl.bits.FactorN;
    pll_factor->FactorK = pll1_ctrl.bits.FactorK;
    pll_factor->FactorM = pll1_ctrl.bits.FactorM;
    pll_factor->FactorP = pll1_ctrl.bits.FactorP;
	
    //busy_waiting();

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_apb2losc
*
* Description: switch apb2 clock to 32k low osc.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_apb2losc(void)
{
    unsigned int tmp;

    //apb2
    CmuReg_apb2_tmp.dwval = CmuReg->Apb2Div.dwval; 
    CmuReg_apb2_tmp.bits.ClkSrc = APB2_CLKSRC_LOSC;
    CmuReg->Apb2Div.dwval = CmuReg_apb2_tmp.dwval;

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_apb2hosc
*
* Description: switch apb2 clock to 24M hosc.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_apb2hosc(void)
{
    unsigned int tmp;

    //apb2
    CmuReg_apb2_tmp.dwval = CmuReg->Apb2Div.dwval; 
    CmuReg_apb2_tmp.bits.ClkSrc = APB2_CLKSRC_HOSC;
    CmuReg->Apb2Div.dwval = CmuReg_apb2_tmp.dwval;

    return 0;
}

/*
*********************************************************************************************************
*                                     standby_clk_bus_src_backup
*
* Description: switch ahb2->?
*		      ahb1->?.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_bus_src_backup(void)
{
    //backup bus src cfg
    /* backup ahb clk src */
    CmuReg_ahb1_backup.dwval = CmuReg->Ahb1Div.dwval; 
    CmuReg_ahb2_backup.dwval = CmuReg->Ahb2Div.dwval; 
    /* backup apb clk src */
    CmuReg_apb2_backup.dwval = CmuReg->Apb2Div.dwval; 

    return 0;
}

/*
*********************************************************************************************************
*                                     standby_clk_bus_src_set
*
* Description: switch ahb2->ahb1->axi.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/

__s32 standby_clk_bus_src_set(void)
{
    /* change ahb2 clock to ahb1*/
    CmuReg_ahb2_tmp.dwval = CmuReg->Ahb2Div.dwval; 
    CmuReg_ahb2_tmp.bits.ClkSrc = AHB2_CLKSRC_AHB1;
    CmuReg->Ahb2Div.dwval = CmuReg_ahb2_tmp.dwval;
    //printk("CmuReg_ahb2_backup, %x!\n", CmuReg_ahb2_backup);
    
    /* change ahb1 clock to axi */
    CmuReg_ahb1_tmp.dwval = CmuReg->Ahb1Div.dwval; 
    CmuReg_ahb1_tmp.bits.ClkSrc = AHB1_CLKSRC_AXI;
    CmuReg->Ahb1Div.dwval = CmuReg_ahb1_tmp.dwval;
    // printk("CmuReg_ahb1_backup, %x!\n", CmuReg_ahb1_backup);
    
    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_bus_src_restore
*
* Description: switch ahb2->?
*		      ahb1->?.
*
* Arguments  : none
*
* Returns    : 0;
*********************************************************************************************************
*/
__s32 standby_clk_bus_src_restore(void)
{
    /* restore ahb clk src */
    CmuReg->Ahb2Div.dwval = CmuReg_ahb2_backup.dwval;
    CmuReg->Ahb1Div.dwval = CmuReg_ahb1_backup.dwval;

    /* restore apb clk src */
    CmuReg->Apb2Div.dwval = CmuReg_apb2_backup.dwval;

    return 0;
}


/*
*********************************************************************************************************
*                                     standby_clk_dramgating
*
* Description: gating dram clock.
*
* Arguments  : onoff    dram clock gating on or off;
*
* Returns    : 0;
*********************************************************************************************************
*/
void standby_clk_dramgating(int onoff)
{
    unsigned int tmp;
    tmp = readl(&(CmuReg->Pll5Ctl));

    if(onoff) {
	tmp |= (0x80000000);
    }
    else {
	tmp &= (~0x80000000);
    }

    writel(tmp, &(CmuReg->Pll5Ctl));

    return ;
}

#else

/*
*********************************************************************************************************
*                           standby_clk_init
*
*Description: ccu init for platform standby
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 standby_clk_init(void)
{
#ifdef CONFIG_ARCH_SUN8IW8P1
#else
    r_prcm = (void *)IO_ADDRESS(AW_R_PRCM_BASE);
#endif

    CmuReg = (__ccmu_reg_list_t   *)IO_ADDRESS(AW_CCM_BASE);
    

    return 0;
}


/*
*********************************************************************************************************
*                           standby_clk_exit
*
*Description: ccu exit for platform standby
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 standby_clk_exit(void)
{

    return 0;
}

/*
*********************************************************************************************************
*                                     standby_clk_core2hosc
*
* Description: switch core clock to 24M high osc.
*
* Arguments  : none
*
*********************************************************************************************************
*/
__s32 standby_clk_core2hosc(void)
{	
    CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval; 
    CmuReg_SysClkDiv_tmp.bits.CpuClkSrc = 1;
    CmuReg->SysClkDiv.dwval = CmuReg_SysClkDiv_tmp.dwval;
    return ;
}



#ifdef CONFIG_ARCH_SUN8IW8P1
#else
/*
*********************************************************************************************************
*                                     standby_clk_ldoenable
*
* Description: enable LDO, ld01, hosc.
*
* Arguments  : none
*
*********************************************************************************************************
*/
static void standby_cpus_clk_ldoenable(void)
{
	//cpus power domain, offset 0x44, how to enable?
	__u32 tmp;
	tmp = readl(r_prcm + PLL_CTRL_REG1_OFFSET );
	tmp &= ~(0xff000000);
	tmp |= (0xa7000000);
	writel(tmp, r_prcm + PLL_CTRL_REG1_OFFSET);

	//enalbe ldo, ldo1,crystal
	tmp = readl(r_prcm + PLL_CTRL_REG1_OFFSET );
	tmp &= ~(0x00000007);
	tmp |= (0x00000007);
	writel(tmp, r_prcm + PLL_CTRL_REG1_OFFSET);

	//disable change.
	tmp = readl(r_prcm + PLL_CTRL_REG1_OFFSET );
	tmp &= ~(0xff000000);
	writel(tmp, r_prcm + PLL_CTRL_REG1_OFFSET);
	
	return ;
}
#endif

__s32 standby_clk_ldoenable(void)
{
#ifdef CONFIG_ARCH_SUN8IW8P1
#else
    standby_cpus_clk_ldoenable();
#endif
}
/*
*********************************************************************************************************
*                                     standby_clk_pll1enable
*
* Description: enable pll1.
*
* Arguments  : none
*
*********************************************************************************************************
*/
void standby_clk_pll1enable(void)
{
    CmuReg_Pll1Ctl_tmp.dwval = CmuReg->Pll1Ctl.dwval;
    CmuReg_Pll1Ctl_tmp.bits.PLLEn = 1;
    CmuReg->Pll1Ctl.dwval = CmuReg_Pll1Ctl_tmp.dwval;
    return ;
}
#endif
#endif

#ifdef CONFIG_ARCH_SUN9IW1
/*
*********************************************************************************************************
*                           standby_clk_init
*
*Description: ccu init for platform standby
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 standby_clk_init(void)
{
   

    return 0;
}


/*
*********************************************************************************************************
*                           standby_clk_exit
*
*Description: ccu exit for platform standby
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 standby_clk_exit(void)
{

    return 0;
}

/*
*********************************************************************************************************
*                                     standby_clk_core2hosc
*
* Description: switch core clock to 24M high osc.
*
* Arguments  : none
*
*********************************************************************************************************
*/
__s32 standby_clk_core2hosc(void)
{	
	return ;
}


/*
*********************************************************************************************************
*                                     standby_clk_ldoenable
*
* Description: enable LDO, ld01, hosc.
*
* Arguments  : none
*
*********************************************************************************************************
*/
__s32 standby_clk_ldoenable(void)
{
	
	return ;
}

/*
*********************************************************************************************************
*                                     standby_clk_pll1enable
*
* Description: enable pll1.
*
* Arguments  : none
*
*********************************************************************************************************
*/
void standby_clk_pll1enable(void)
{
	return ;
}
#endif


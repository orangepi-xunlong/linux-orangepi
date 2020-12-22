/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, gq.yang China
*                                             All Rights Reserved
*
* File    : super_cpus.c
* By      : gq.yang
* Version : v1.0
* Date    : 2013-3-23 20:13
* Descript: interrupt for platform mem
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "super_cpus.h"
#ifdef CONFIG_ARCH_SUN8IW1P1
/*
*********************************************************************************************************
*				       SET CPU RESET STATE
*
* Description:	set the reset state of cpu.
*
* Arguments  :	cpu_num : the cpu id which we want to set reset status.
*				state	: the reset state which we want to set.
*
* Returns    :	OK if get power status succeeded, others if failed.
*********************************************************************************************************
*/
__s32 cpucfg_set_cpu_reset_state(__u32 cpu_num, __s32 state)
{
	volatile __u32 value;
	
	//set cpu state
	value  = readl(AW_R_CPUCFG_BASE + CPUX_RESET_CTL(cpu_num));
	value &= ~(CPU_RESET_MASK);
	value |= state;
	writel(value, AW_R_CPUCFG_BASE + CPUX_RESET_CTL(cpu_num));
	
	return OK;
}

void super_enable_aw_cpu(int cpu)
{
	__u32 paddr;
	__u32 pwr_reg;
	
	paddr = CPUX_STARTUP_ADDR;
	writel(paddr, (AW_R_CPUCFG_BASE) + AW_CPUCFG_P_REG0);
	
	/* step1: Assert nCOREPORESET LOW and hold L1RSTDISABLE LOW.
	      Ensure DBGPWRDUP is held LOW to prevent any external
	      debug access to the processor.
	*/
	/* assert cpu core reset */
	writel(0, (AW_R_CPUCFG_BASE) + CPUX_RESET_CTL(cpu));
	/* L1RSTDISABLE hold low */
	pwr_reg = readl((AW_R_CPUCFG_BASE) + AW_CPUCFG_GENCTL);
	pwr_reg &= ~(1<<cpu);
	writel(pwr_reg, (AW_R_CPUCFG_BASE) + AW_CPUCFG_GENCTL);

#ifndef CONFIG_FPGA_V4_PLATFORM	
	/* step2: release power clamp */
	//write bit3, bit4 to 0
	writel(0xe7, (AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(cpu));
	while((0xe7) != readl((AW_R_CPUCFG_BASE) + AW_CPUX_PWR_CLAMP_STATUS(cpu)))
	    ;
	//write 012567 bit to 0
	writel(0x00, (AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(cpu));
	while((0x00) != readl((AW_R_CPUCFG_BASE) + AW_CPUX_PWR_CLAMP_STATUS(cpu)))
	    ;
	delay_ms(2);
#endif	
	
	/* step3: clear power-off gating */
	pwr_reg = readl((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG);
	pwr_reg &= ~(0x00000001<<cpu);
	writel(pwr_reg, (AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG);
	delay_ms(1);
	
	/* step4: de-assert core reset */
	writel(3, (AW_R_CPUCFG_BASE) + CPUX_RESET_CTL(cpu));

}
#endif


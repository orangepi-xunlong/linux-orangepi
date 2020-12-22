/* Copyright (C) 
* 2014 - AllwinnerTech
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/

/**
* @file sunxi_oops.h
* @brief: hooks for oops,
*         print more information and enable sdcard jtag.
* @author AllwinnerTech
* @version v0.1
* @date 2014-12-10
*/


#ifndef __SUNXI_OOPS_H__
#define __SUNXI_OOPS_H__

#include <linux/io.h>
#include <mach/platform.h>

#ifdef CONFIG_SUNXI_OOPS_HOOK

#ifdef CONFIG_ARCH_SUN8IW7P1

/* primary jtag */
/* PA0 -- MS0
 * PA1 -- CK0
 * PA2 -- DO0
 * PA3 -- DI0
 */
#define PRIMARY_JTAG_CONFIG_REG_VA	(SUNXI_PIO_VBASE + 0x00)
#define PRIMARY_JTAG_EN_MASK		((0x3<<12)|(0x3<<8) |(0x3<<4)|0x3)
#define PRIMARY_JTAG_PIN_MASK		((0x7<<12)|(0x7<<8) |(0x7<<4)|0x7)

/* primary jtag */
/* PF0 -- MS
 * PF1 -- DI
 * PF3 -- DO
 * PF5 -- CK
 */
#define SDCARD_JTAG_CONFIG_REG_VA	(SUNXI_PIO_VBASE + 0xb4)
#define SDCARD_JTAG_EN_MASK			((0x3<<20)|(0x3<<12) |(0x3<<4)|0x3)
#define SDCARD_JTAG_PIN_MASK		((0x7<<20)|(0x7<<12) |(0x7<<4)|0x7)

/* cpux pll */
#define CPUX_PLL_CONFIG		(SUNXI_CCM_VBASE + 0x00)
#define FACTOR_M_SHIFT		(0)
#define FACTOR_M_MASK		(0x03)

#define FACTOR_K_SHIFT		(4)
#define FACTOR_K_MASK		(0x03)

#define FACTOR_N_SHIFT		(8)
#define FACTOR_N_MASK		(0x1f)

#define FACTOR_P_SHIFT		(16)
#define FACTOR_P_MASK		(0x03)

/* cpu freq = 24*N*K/(M*P) MHz */
#define CPUX_FREQ_MHZ(val)	(24)*(((val)>>FACTOR_N_SHIFT & FACTOR_N_MASK) + 1)*\
								 (((val)>>FACTOR_K_SHIFT & FACTOR_K_MASK) + 1)/\
								 (((val)>>FACTOR_M_SHIFT & FACTOR_M_MASK) + 1)/\
								 (((val)>>FACTOR_P_SHIFT & FACTOR_P_MASK) + 1)

/* dram frequency and gpu frequency */
#define GPU_PERDIV_M_MASK		(0x0f)
#define GPU_PERDIV_M_SHIFT		(0)

#define GPU_FACTOR_N_MASK		(0x3f)
#define GPU_FACTOR_N_SHIFT		(8)

#define GPU_FREQ_MHZ(val)	(24)*(((val)>>GPU_FACTOR_N_SHIFT & GPU_FACTOR_N_MASK) + 1)/\
								 (((val)>>GPU_PERDIV_M_SHIFT & GPU_PERDIV_M_MASK) + 1)

#define DRAM_PERDIV_M_MASK		(0x03)
#define DRAM_PERDIV_M_SHIFT		(0)

#define DRAM_FACTOR_K_MASK		(0x03)
#define DRAM_FACTOR_K_SHIFT		(4)

#define DRAM_FACTOR_N_MASK		(0x1f)
#define DRAM_FACTOR_N_SHIFT		(8)

/* the dram frequency is pll_dram/2 */
#define DRAM_FREQ_MHZ(val)	(24)*(((val)>>DRAM_FACTOR_N_SHIFT & DRAM_FACTOR_N_MASK) + 1)*\
								 (((val)>>DRAM_FACTOR_K_SHIFT & DRAM_FACTOR_K_MASK) + 1)/\
								 (((val)>>DRAM_PERDIV_M_SHIFT & DRAM_PERDIV_M_MASK) + 1)/2

#define GPU_PLL_CONFIG_REG_VA	(SUNXI_CCM_VBASE + 0x0038)
#define DRAM_PLL_CONFIG_REG_VA	(SUNXI_CCM_VBASE + 0x0020)

/* temperature */
#define THERMAL_DATA_REG_VA (0xf1c25000 + 0x80)
#define CALC_TEMP(val)		( 217-((val)*1000/8253) ) /* modify from sunxi-temperature.c */

#else

#warning "Please modify 'arch/arm/mach-sunxi/include/mach/sunxi_oops.h' to enable sunxi_oops_hook support"
#warning "or, unconfig SUNXI_OOPS_HOOK in menuconfig"

#endif	/* CONFIG_ARCH_SUN8IW7P1 */


inline void sunxi_oops_hook(void)
{
	unsigned int val;
	unsigned int pll;

	/* disable primary jtag */
	#if defined(PRIMARY_JTAG_CONFIG_REG_VA) && defined(PRIMARY_JTAG_EN_MASK) && defined(PRIMARY_JTAG_PIN_MASK)
	val = readl(PRIMARY_JTAG_CONFIG_REG_VA);
	val |= PRIMARY_JTAG_PIN_MASK;
	writel(val, PRIMARY_JTAG_CONFIG_REG_VA);
	#endif

	/* enable sdcard jtag */
	#if defined(SDCARD_JTAG_CONFIG_REG_VA) && defined(SDCARD_JTAG_EN_MASK) && defined(SDCARD_JTAG_PIN_MASK)
	val = readl(SDCARD_JTAG_CONFIG_REG_VA);
	val &= ~SDCARD_JTAG_PIN_MASK;
	val |= SDCARD_JTAG_EN_MASK;
	writel(val, SDCARD_JTAG_CONFIG_REG_VA);
	printk(KERN_EMERG "sunxi oops: enable sdcard JTAG interface\n");
	#endif

	#if defined(CPUX_PLL_CONFIG) && defined(CPUX_FREQ_MHZ)
	pll = readl(CPUX_PLL_CONFIG);
	printk(KERN_EMERG "sunxi oops: cpu frequency: %d MHz\n", CPUX_FREQ_MHZ(pll));
	#endif

	#if defined(DRAM_PLL_CONFIG_REG_VA) && defined(DRAM_FREQ_MHZ)
	pll = readl(DRAM_PLL_CONFIG_REG_VA);
	printk(KERN_EMERG "sunxi oops: ddr frequency: %d MHz\n", DRAM_FREQ_MHZ(pll));
	#endif

	#if defined(GPU_PLL_CONFIG_REG_VA) && defined(GPU_FREQ_MHZ)
	pll = readl(GPU_PLL_CONFIG_REG_VA);
	printk(KERN_EMERG "sunxi oops: gpu frequency: %d MHz\n", GPU_FREQ_MHZ(pll));
	#endif

	#if defined(THERMAL_DATA_REG_VA) && defined(CALC_TEMP)
	val = readl(THERMAL_DATA_REG_VA);
	printk(KERN_EMERG "sunxi oops: cpu temperature: %d \n", CALC_TEMP(val));
	#endif
}

#endif	/* CONFIG_SUNXI_OOPS_HOOK */

#endif	/* __SUNXI_OOPS_H__ */

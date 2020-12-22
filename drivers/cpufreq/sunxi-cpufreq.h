/*
 * drivers/cpufreq/sunxi-cpufreq.h
 *
 * Copyright (c) 2012 Softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __sunxi_CPU_FREQ_H__
#define __sunxi_CPU_FREQ_H__

#include <linux/types.h>
#include <linux/cpufreq.h>

#define CPUFREQ_ERR(format,args...)   printk(KERN_ERR "[cpu_freq] ERR:"format,##args)


#define SUNXI_CPUFREQ_MAX           (1536000000)    /* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_MIN           (60000000)      /* config the minimum frequency of sunxi core */
#define SUNXI_FREQTRANS_LATENCY     (2000000)       /* config the transition latency, based on ns */
#if !defined(CONFIG_ARCH_SUN8IW7P1) && !defined(CONFIG_ARCH_SUN8IW8P1)
#define SUNXI_CPUFREQ_CPUVDD        "axp22_dcdc3"
#endif

#ifdef CONFIG_ARCH_SUN8IW3P1
#define SUNXI_CPUFREQ_VOLT_LIMIT    (1100)
#endif

struct sunxi_clk_div_t {
    __u32   cpu_div:4;      /* division of cpu clock, divide core_pll */
    __u32   axi_div:4;      /* division of axi clock, divide cpu clock*/
    __u32   ahb_div:4;      /* division of ahb clock, divide axi clock*/
    __u32   apb_div:4;      /* division of apb clock, divide ahb clock*/
    __u32   reserved:16;
};

struct cpufreq_dvfs {
    unsigned int freq;   /* cpu frequency */
    unsigned int volt;   /* voltage for the frequency */
};

struct sunxi_cpu_freq_t {
    __u32                   pll;    /* core pll frequency value */
    struct sunxi_clk_div_t  div;    /* division configuration   */
};


#define SUNXI_CLK_DIV(cpu_div, axi_div, ahb_div, apb_div)       \
                ((cpu_div<<0)|(axi_div<<4)|(ahb_div<<8)|(apb_div<<12))

#endif  /* #ifndef __sunxi_CPU_FREQ_H__ */



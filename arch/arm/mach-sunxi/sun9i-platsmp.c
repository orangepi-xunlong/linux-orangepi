/*
 * linux/arch/arm/mach-sunxi/sunxi-platsmp.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sunxi soc platform smp operations.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/errno.h>

#include <asm/smp.h>
#include <asm/mcpm.h>
#include <asm/io.h>
#include <asm/smp_scu.h>
#include <asm/hardware/gic.h>


bool __init sun9i_smp_init_ops(void)
{
#ifdef CONFIG_MCPM
	/* the mcpm smp ops just use for MCPM platform */
	mcpm_smp_set_ops();
	return true;
#else
	/* default use machine_desc->smp ops */
	return false;
#endif
}

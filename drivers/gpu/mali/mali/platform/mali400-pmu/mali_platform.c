/*
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"
#include "mali_mem_validation.h"

#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk-private.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>

static struct clk *mali_clk = NULL;
static struct clk *gpu_pll  = NULL;

_mali_osk_errcode_t mali_platform_init(void)
{
	int freq = 252; /* 252 MHz */

	gpu_pll = clk_get(NULL, PLL_GPU_CLK);

	if (!gpu_pll || IS_ERR(gpu_pll))	{
		printk(KERN_ERR "Failed to get gpu pll clock!\n");
		return -1;
	}

	mali_clk = clk_get(NULL, GPU_CLK);
	if (!mali_clk || IS_ERR(mali_clk)) {
		printk(KERN_ERR "Failed to get mali clock!\n");
		return -1;
	}

	if (clk_set_rate(gpu_pll, freq * 1000 * 1000)) {
		printk(KERN_ERR "Failed to set gpu pll clock!\n");
		return -1;
	}

	if (clk_set_rate(mali_clk, freq * 1000 * 1000)) {
		printk(KERN_ERR "Failed to set mali clock!\n");
		return -1;
	}
	
	if (mali_clk->enable_count == 0) {
		if (clk_prepare_enable(gpu_pll))
			printk(KERN_ERR "Failed to enable gpu pll!\n");

		if (clk_prepare_enable(mali_clk))
			printk(KERN_ERR "Failed to enable mali clock!\n");
	}

	pr_info("mali clk: %d MHz\n", freq);

    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
	if (mali_clk->enable_count == 1) {
		clk_disable_unprepare(mali_clk);
		clk_disable_unprepare(gpu_pll);
	}

    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
    MALI_SUCCESS;
}

void mali_gpu_utilization_handler(u32 utilization)
{
}

void set_mali_parent_power_domain(void* dev)
{
}



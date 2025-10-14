// SPDX-License-Identifier: GPL-2.0
/*
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include "clk.h"

#ifndef MODULE

static bool cix_uart_clocks_keep;
static int cix_uart_clocks_enabled;
static struct clk **cix_uart_clocks;

static int __init cix_uart_clocks_keep_param(char *str)
{
	cix_uart_clocks_keep = 1;

	return 0;
}
early_param("earlycon", cix_uart_clocks_keep_param);

void cix_uart_clocks_register(void)
{
	unsigned int i, clk_count = 0;
	cix_uart_clocks_enabled = 0;

#ifdef CONFIG_OF
	if (!of_stdout || !cix_uart_clocks_keep)
		return;

	clk_count = of_clk_get_parent_count(of_stdout);
	pr_info("%s: uart clock count=%d!\n", __func__, clk_count);

	if (!clk_count) {
		pr_err("%s: uart clock no need to register, count=%d!\n",
		       __func__, clk_count);
		return;
	}


	cix_uart_clocks = kcalloc(clk_count, sizeof(struct clk *), GFP_KERNEL);
	if (!cix_uart_clocks)
		return;

	for (i = 0; i < clk_count; i++) {
		cix_uart_clocks[cix_uart_clocks_enabled] = of_clk_get(of_stdout, i);

		if (IS_ERR(cix_uart_clocks[cix_uart_clocks_enabled]))
			return;

		if (cix_uart_clocks[cix_uart_clocks_enabled])
			clk_prepare_enable(cix_uart_clocks[cix_uart_clocks_enabled++]);
	}
#endif
}
EXPORT_SYMBOL_GPL(cix_uart_clocks_register);

static int __init cix_clk_disable_uart(void)
{
	if (cix_uart_clocks_keep && cix_uart_clocks_enabled) {
		int i;

		for (i = 0; i < cix_uart_clocks_enabled; i++) {
			clk_disable_unprepare(cix_uart_clocks[i]);
			clk_put(cix_uart_clocks[i]);
		}
		kfree(cix_uart_clocks);
	}

	return 0;
}
late_initcall_sync(cix_clk_disable_uart);
#endif

MODULE_AUTHOR("Copyright 2024 Cix Technology Group Co., Ltd.");
MODULE_DESCRIPTION("Cix clock driver");
MODULE_LICENSE("GPL v2");

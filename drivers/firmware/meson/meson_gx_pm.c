/*
 * Amlogic Meson GX Power Management
 *
 * Copyright (c) 2016 Baylibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/arm-smccc.h>

#include <uapi/linux/psci.h>

#include <asm/suspend.h>

/*
 * The Amlogic GX SoCs uses a special argument value to the
 * PSCI CPU_SUSPEND method to enter SUSPEND_MEM.
 */

#define MESON_SUSPEND_PARAM	0x0010000
#define PSCI_FN_NATIVE(version, name)	PSCI_##version##_FN64_##name

static int meson_gx_suspend_finish(unsigned long arg)
{
	struct arm_smccc_res res;

	arm_smccc_smc(PSCI_FN_NATIVE(0_2, CPU_SUSPEND), arg,
		      virt_to_phys(cpu_resume), 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int meson_gx_suspend_enter(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_MEM:
		return cpu_suspend(MESON_SUSPEND_PARAM,
				   meson_gx_suspend_finish);
	}

	return -EINVAL;
}

static const struct platform_suspend_ops meson_gx_pm_ops = {
		.enter = meson_gx_suspend_enter,
		.valid = suspend_valid_only_mem,
};

static const struct of_device_id meson_gx_pm_match[] = {
	{ .compatible = "amlogic,meson-gx-pm", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, meson_gx_pm_match);

static int meson_gx_pm_probe(struct platform_device *pdev)
{
	suspend_set_ops(&meson_gx_pm_ops);

	return 0;
}

static struct platform_driver meson_gx_pm_driver = {
	.probe = meson_gx_pm_probe,
	.driver = {
		.name = "meson-gx-pm",
		.of_match_table = meson_gx_pm_match,
	},
};

module_platform_driver(meson_gx_pm_driver);

MODULE_AUTHOR("Neil Armstrong <narmstrong@baylibre.com>");
MODULE_DESCRIPTION("Amlogic Meson GX PM driver");
MODULE_LICENSE("GPL v2");

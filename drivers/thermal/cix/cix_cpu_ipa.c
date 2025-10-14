// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic Cix IPA support driver
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.All Rights Reserved.
 */

#include <linux/device.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define REG_OFFSET 0x40

typedef struct {
	volatile uint32_t off_cnt;
	uint32_t rsvd[13];
	int32_t dynamic_power;
	int32_t static_power;
} CPU_IPA_INFO;

struct cpu_ipa {
	struct device *dev;
	void __iomem *regs;
} *ci;

static int cix_get_static_power(int cpu)
{
	int pcpu = MPIDR_AFFINITY_LEVEL(cpu_logical_map(cpu), 1);

	CPU_IPA_INFO *info = ci->regs + pcpu * REG_OFFSET;

	return info->static_power;
}

static int cix_get_dynamic_power(int cpu)
{
	int pcpu = MPIDR_AFFINITY_LEVEL(cpu_logical_map(cpu), 1);

	CPU_IPA_INFO *info = ci->regs + pcpu * REG_OFFSET;

	return info->dynamic_power;
}

int cix_get_static_power_cpus(cpumask_var_t cpus)
{
	int cpu;
	int total_power = 0;

	for_each_cpu(cpu, cpus) {
		total_power += cix_get_static_power(cpu);
	}
	return total_power;
}
EXPORT_SYMBOL(cix_get_static_power_cpus);

int cix_get_dynamic_power_cpus(cpumask_var_t cpus)
{
	int cpu;
	int total_power = 0;

	for_each_cpu(cpu, cpus) {
		total_power += cix_get_dynamic_power(cpu);
	}
	return total_power;
}
EXPORT_SYMBOL(cix_get_dynamic_power_cpus);

static int cpu_ipa_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	ci = devm_kzalloc(&pdev->dev, sizeof(*ci), GFP_KERNEL);
	if (!ci)
		return -ENOMEM;

	ci->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ci->regs))
		return PTR_ERR(ci->regs);

	ci->dev = dev;
	platform_set_drvdata(pdev, ci);
	return 0;
}

static void cpu_ipa_shutdown(struct platform_device *pdev)
{
	return;
}

#ifdef CONFIG_PM_SLEEP
static int cpu_ipa_resume(struct device *dev)
{
	return 0;
}

static int cpu_ipa_suspend(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops cpu_ipa_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(cpu_ipa_suspend, cpu_ipa_resume)
};

static const struct of_device_id cpu_ipa_of_match[] = {
	{ .compatible = "cix,cpu-ipa", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, cpu_ipa_of_match);

static struct platform_driver cpu_ipa_platdrv = {
	.probe		= cpu_ipa_probe,
	.shutdown	= cpu_ipa_shutdown,
	.driver = {
		.name	= "cpu-ipa",
		.pm	= &cpu_ipa_pm,
		.of_match_table = of_match_ptr(cpu_ipa_of_match),
	},
};
module_platform_driver(cpu_ipa_platdrv);

MODULE_DESCRIPTION("Generic Cix IPA support driver");
MODULE_AUTHOR("Cixtech,Inc.");
MODULE_LICENSE("GPL v2");

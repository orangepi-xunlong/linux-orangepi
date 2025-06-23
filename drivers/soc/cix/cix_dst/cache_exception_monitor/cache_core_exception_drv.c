// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>
#include <asm/smp_plat.h>
#ifdef CONFIG_ACPI
#include <acpi/processor.h>
#endif
#include "cache_exception_interface.h"
#include "../dst_print.h"

#define MAX_CACHE_EXCEP_CORES 16

static struct cache_excep_drvdata *g_drvdata[MAX_CACHE_EXCEP_CORES];
static int cpu_hot_init = 0;

#ifdef CONFIG_ACPI
static int acpi_handle_to_logical_cpuid(acpi_handle handle)
{
	int i;
	struct acpi_processor *pr;

	for_each_possible_cpu(i) {
		pr = per_cpu(processors, i);
		if (pr && pr->handle == handle)
			break;
	}

	return i;
}
#endif

static int cache_excep_of_get_cpu(struct device *dev)
{
	int cpu;
	struct device_node *dn;

	if (!dev->of_node)
		return -ENODEV;

	dn = of_parse_phandle(dev->of_node, "cpu", 0);
	if (!dn)
		return -ENODEV;

	cpu = of_cpu_node_to_id(dn);
	of_node_put(dn);

	return cpu;
}

#ifdef CONFIG_ACPI
static int cache_excep_acpi_get_cpu(struct device *dev)
{
	int cpu;
	acpi_handle cpu_handle;
	acpi_status status;
	struct acpi_device *adev = ACPI_COMPANION(dev);

	if (!adev)
		return -ENODEV;
	status = acpi_get_parent(adev->handle, &cpu_handle);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	cpu = acpi_handle_to_logical_cpuid(cpu_handle);
	if (cpu >= nr_cpu_ids)
		return -ENODEV;
	return cpu;
}
#else
static int cache_excep_acpi_get_cpu(struct device *dev)
{
	return 0;
}
#endif

int cache_excep_get_cpu(struct device *dev)
{
	if (is_of_node(dev->fwnode))
		return cache_excep_of_get_cpu(dev);
	else if (is_acpi_device_node(dev->fwnode))
		return cache_excep_acpi_get_cpu(dev);
	return 0;
}

static int cache_excep_online_init(unsigned int cpu)
{
	struct cache_excep_drvdata *drvdata = g_drvdata[cpu];

	DST_PRINT_DBG("%s: cpu%d online \n", __func__, cpu);
	if (drvdata && (drvdata->state == 0)) {
		DST_PRINT_PN("%s: init cache monitor registers for cpu%d\n", __func__, cpu);
		cache_excep_init(drvdata);
		drvdata->state = 1;
	}
	return 0;
}

static int cache_excep_offline_init(unsigned int cpu)
{
	struct cache_excep_drvdata *drvdata = g_drvdata[cpu];

	DST_PRINT_DBG("%s: uninit cache monitor registers for cpu%d\n", __func__, cpu);

	if (drvdata)
		drvdata->state = 0;

	return 0;
}

static irqreturn_t cache_excep_irq_handler(int irq, void *data)
{
	struct cache_excep_drvdata *drvdata = data;

	DST_PRINT_PN("cache_excep_irq_handler: cpu=%d irq=%d\n", drvdata->cpu, irq);

	if (drvdata->core_type == CORE_HAYES) {
		cache_excep_record(CACHE_EXCEP_L1);
		cache_excep_record(CACHE_EXCEP_L2);
	} else if (drvdata->core_type == CORE_HUNTER) {
		cache_excep_record(CACHE_EXCEP_CORE);
	}

	return IRQ_HANDLED;
}

#define CACHE_EXCEP_INTR_REQUEST(dev, cpu, irq, handler, name, data)          \
	if (irq > 0) {                                                        \
		ret = devm_request_irq(dev, irq, handler, IRQF_ONESHOT, name, \
				       data);                                 \
		if (ret) {                                                    \
			dev_err(dev, "IRQ request failed: %s (%d) -- %d\n",   \
				name, irq, ret);                              \
			return ret;                                           \
		}                                                             \
		irq_set_affinity_hint(irq, cpumask_of(cpu));                  \
	}

static int cache_excep_probe_dev(struct platform_device *pdev)
{
	int ret;
	struct cache_excep_drvdata *drvdata;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, drvdata);

	drvdata->core_type = CORE_HUNTER;
	drvdata->cpu = cache_excep_get_cpu(&pdev->dev);
	if (drvdata->cpu < 0)
		return drvdata->cpu;

	g_drvdata[drvdata->cpu] = drvdata;;
	DST_PRINT_DBG("%s: cpu%d probing...\n", __func__, drvdata->cpu);

	if (device_property_read_bool(&pdev->dev, "has-complex")) {
		drvdata->core_type = CORE_HAYES;
		drvdata->complex_errirq =
			platform_get_irq_byname(pdev, "complex_errirq");
		drvdata->complex_faultirq =
			platform_get_irq_byname(pdev, "complex_faultirq");

		CACHE_EXCEP_INTR_REQUEST(&pdev->dev, drvdata->cpu,
					 drvdata->complex_errirq,
					 cache_excep_irq_handler,
					 "complex_errirq", drvdata);

		CACHE_EXCEP_INTR_REQUEST(&pdev->dev, drvdata->cpu,
					 drvdata->complex_faultirq,
					 cache_excep_irq_handler,
					 "complex_faultirq", drvdata);
	}

	drvdata->errirq = platform_get_irq_byname(pdev, "core_errirq");
	drvdata->faultirq = platform_get_irq_byname(pdev, "core_faultirq");

	CACHE_EXCEP_INTR_REQUEST(&pdev->dev, drvdata->cpu, drvdata->errirq,
				 cache_excep_irq_handler, "core_errirq",
				 drvdata);

	CACHE_EXCEP_INTR_REQUEST(&pdev->dev, drvdata->cpu, drvdata->faultirq,
				 cache_excep_irq_handler, "core_faultirq",
				 drvdata);

	if (!cpu_hot_init) {
		ret = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN,
						"cpuonoff:online",
						cache_excep_online_init, cache_excep_offline_init);
		if (ret < 0)
			DST_PRINT_ERR("cpu_on_off cpuhp_setup_state_nocalls failed\n");
		else
			cpu_hot_init = 1;
	}

	drvdata->state = 0;
	if (cpu_online(drvdata->cpu)) {
		drvdata->state = 1;
		ret = smp_call_function_single(drvdata->cpu, cache_excep_init, drvdata, 1);
		if (ret) {
			dev_err(&pdev->dev, "%s: ret=%d", __func__, ret);
		}
	}
	DST_PRINT_DBG("%s: cpu%d probed...\n", __func__, drvdata->cpu);
	return 0;
}

static int __exit cache_excep_remove_dev(struct platform_device *pdev)
{
	int ret = 0;
	struct cache_excep_drvdata *drvdata = dev_get_drvdata(&pdev->dev);

	if (drvdata && cpu_online(drvdata->cpu) ) {
		ret = smp_call_function_single(drvdata->cpu, cache_excep_uninit,
					 drvdata, 1);
		if (ret)
			dev_err(&pdev->dev, "%s: ret=%d", __func__, ret);
	}
	return ret;
}

static int cache_excep_suspend(struct device *dev)
{
	struct cache_excep_drvdata *drvdata = dev_get_drvdata(dev);

	drvdata->state = 0;
	return 0;
}

static int cache_excep_resume(struct device *dev)
{
	struct cache_excep_drvdata *drvdata = dev_get_drvdata(dev);
	int ret = 0;

	if (cpu_online(drvdata->cpu) && (drvdata->state == 0)) {
		drvdata->state = 1;
		ret = smp_call_function_single(drvdata->cpu, cache_excep_init, drvdata, 1);
		if (ret) {
			dev_err(dev, "%s: ret=%d", __func__, ret);
		}
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(cache_excep_pm_ops, cache_excep_suspend,
		cache_excep_resume);

static const struct of_device_id cache_excp_of_ids[] = {
	{ .compatible = "cix,sky1_exception_core_cache" },
	{ }
};
MODULE_DEVICE_TABLE(of, cache_excp_of_ids);

static const struct acpi_device_id cache_excp_acpi_ids[] = {
	{ "CIXH10F3", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, cache_excp_acpi_ids);

static struct platform_driver cache_core_exception_driver = {
	.probe		= cache_excep_probe_dev,
	.remove		= cache_excep_remove_dev,
	.driver			= {
		.name			= "sky1_exception_core_cache",
		.pm	= &cache_excep_pm_ops,
		.of_match_table		= cache_excp_of_ids,
		.acpi_match_table = ACPI_PTR(cache_excp_acpi_ids),
	},
};

static int __init cache_core_exception_init(void)
{
	int ret;
	ret = platform_driver_register(&cache_core_exception_driver);
	if (!ret)
		return 0;

	DST_PRINT_ERR("Error registering core cache exception driver\n");
	return ret;
}

static void __exit cache_core_exception_exit(void)
{
	platform_driver_unregister(&cache_core_exception_driver);
}

subsys_initcall(cache_core_exception_init);
module_exit(cache_core_exception_exit);

MODULE_AUTHOR("Vincent Wu <vincent.wu@cixtech.com>");
MODULE_DESCRIPTION("Sky1 cache exception monitor driver");
MODULE_LICENSE("GPL v2");

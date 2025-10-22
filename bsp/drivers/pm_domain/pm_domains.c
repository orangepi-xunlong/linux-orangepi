// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner power domain support.
 *
 * Copyright (c) 2021 ALLWINNER, Co. Ltd.
 */

#include <sunxi-log.h>
#include <linux/reset.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <linux/pm_clock.h>
#include <linux/pm_domain.h>
#include <linux/of_address.h>
#include <linux/of_clk.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <dt-bindings/power/tv303-power.h>
#include <dt-bindings/power/r528-power.h>
#include <dt-bindings/power/a523-power.h>
#include <dt-bindings/power/v851-power.h>

struct sunxi_domain_info {
	u32 domain_id;
	u32 wait_mode;
	u32 pwr_on_delay;
	u32 pwr_off_delay;
	u32 idle_mask;
	u32 status_mask;
	u32 trans_complete_mask;
};

struct sunxi_pmu_info {
	u32 wait_mode_offset;
	u32 pwr_off_delay_offset;
	u32 pwr_on_delay_offset;
	u32 pwr_offset;
	u32 status_offset;
	u32 num_domains;
	const struct sunxi_domain_info *domain_info;
};

struct sunxi_pm_domain {
	struct generic_pm_domain genpd;
	const struct sunxi_domain_info *info;
	struct sunxi_pmu *pmu;
};

struct sunxi_pmu {
	struct device *dev;
	struct clk *clk;
	struct reset_control *reset;
	struct regmap *regmap;
	const struct sunxi_pmu_info *info;
	struct mutex mutex; /* mutex lock for pmu */
	struct genpd_onecell_data genpd_data;
	struct generic_pm_domain *domains[];
};

#define DRIVER_NAME	"power-domain-driver"

#define to_sunxi_pd(gpd) container_of(gpd, struct sunxi_pm_domain, genpd)

#define DOMAIN(id, wait, pwr_on, pwr_off, imask, smask, trans_mask)	\
{							\
	.domain_id = (id),			\
	.wait_mode = (wait),			\
	.pwr_on_delay = (pwr_on),				\
	.pwr_off_delay = (pwr_off),			\
	.idle_mask = (imask),			\
	.status_mask = (smask),				\
	.trans_complete_mask = (trans_mask),			\
}

#define COMMAND_ON	0x1
#define COMMAND_OFF	0x2
#define STATUS_ON	0x10000
#define STATUS_OFF	0x20000
#define COMPLETE	BIT(1)
#define BASE(id)	((id) << 7)

static bool sunxi_pmu_domain_is_idle(struct sunxi_pm_domain *pd)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;
	unsigned int val;

	regmap_read(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->status_offset, &val);
	return (val & pd_info->idle_mask) == 0;
}

static bool sunxi_pmu_domain_is_on(struct sunxi_pm_domain *pd)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;
	unsigned int val;

	regmap_read(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->status_offset, &val);

	return (val & pd->info->status_mask) == STATUS_ON;
}

static bool sunxi_pmu_domain_is_complete(struct sunxi_pm_domain *pd)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;
	unsigned int val;

	regmap_read(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->status_offset, &val);

	return (val & pd->info->trans_complete_mask) == COMPLETE;
}

static void sunxi_do_pmu_set_power_domain(struct sunxi_pm_domain *pd,
					     bool on)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;
	struct generic_pm_domain *genpd = &pd->genpd;
	bool is_on, is_complete;
	unsigned int val;

	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->pwr_offset,
			     on ? COMMAND_ON : COMMAND_OFF);

	dsb(sy);

	if (readx_poll_timeout_atomic(sunxi_pmu_domain_is_complete, pd, is_complete,
				      is_complete == true, 0, 10000)) {
		sunxi_err(pmu->dev,
			"failed to set domain '%s', val=%d\n",
			genpd->name, is_complete);
		return;
	}

	/* clear the complete bit */
	regmap_read(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->status_offset, &val);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->status_offset, val);

	if (readx_poll_timeout_atomic(sunxi_pmu_domain_is_on, pd, is_on,
				      is_on == on, 0, 10000)) {
		sunxi_err(pmu->dev,
			"failed to set domain '%s', val=%d\n",
			genpd->name, is_on);
		return;
	}
}

static int sunxi_pd_init(struct sunxi_pm_domain *pd)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;

	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->wait_mode_offset,
			     pd_info->wait_mode);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->pwr_on_delay_offset,
			     pd_info->pwr_on_delay);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->pwr_off_delay_offset,
			     pd_info->pwr_off_delay);
	return 0;
}

static int sunxi_pd_power(struct sunxi_pm_domain *pd, bool power_on)
{
	struct sunxi_pmu *pmu = pd->pmu;
	struct generic_pm_domain *genpd = &pd->genpd;
	bool is_idle;
	int ret;

	ret = readx_poll_timeout_atomic(sunxi_pmu_domain_is_idle, pd,
						is_idle, is_idle == true, 0, 10000);
	if (ret) {
		sunxi_err(pmu->dev,
			"failed to set idle on domain '%s', val=%d\n",
			genpd->name, is_idle);
		return ret;
	}

	mutex_lock(&pmu->mutex);

	if (sunxi_pmu_domain_is_on(pd) != power_on)
		sunxi_do_pmu_set_power_domain(pd, power_on);

	mutex_unlock(&pmu->mutex);
	return 0;
}

static int sunxi_pd_power_on(struct generic_pm_domain *domain)
{
	struct sunxi_pm_domain *pd = to_sunxi_pd(domain);
	struct sunxi_pmu *pmu = pd->pmu;
	struct generic_pm_domain *genpd = &pd->genpd;

	sunxi_debug(pmu->dev, "set domain '%s' on\n", genpd->name);

	return sunxi_pd_power(pd, true);
}

static int sunxi_pd_power_off(struct generic_pm_domain *domain)
{
	struct sunxi_pm_domain *pd = to_sunxi_pd(domain);
	struct sunxi_pmu *pmu = pd->pmu;
	struct generic_pm_domain *genpd = &pd->genpd;

	sunxi_debug(pmu->dev, "set domain '%s' off\n", genpd->name);

	return sunxi_pd_power(pd, false);
}

static int sunxi_pd_attach_dev(struct generic_pm_domain *genpd,
				  struct device *dev)
{
	struct clk *clk;
	int i;
	int error;

	sunxi_debug(dev, "attaching to power domain '%s'\n", genpd->name);

	error = pm_clk_create(dev);
	if (error) {
		sunxi_err(dev, "pm_clk_create failed %d\n", error);
		return error;
	}

	i = 0;
	while ((clk = of_clk_get(dev->of_node, i++)) && !IS_ERR(clk)) {
		sunxi_debug(dev, "adding clock '%pC' to list of PM clocks\n", clk);
		error = pm_clk_add_clk(dev, clk);
		if (error) {
			sunxi_err(dev, "pm_clk_add_clk failed %d\n", error);
			clk_put(clk);
			pm_clk_destroy(dev);
			return error;
		}
	}

	return 0;
}

static void sunxi_pd_detach_dev(struct generic_pm_domain *genpd,
				   struct device *dev)
{
	sunxi_debug(dev, "detaching from power domain '%s'\n", genpd->name);

	pm_clk_destroy(dev);
}

static int sunxi_pm_add_one_domain(struct sunxi_pmu *pmu,
				      struct device_node *node)
{
	const struct sunxi_domain_info *pd_info;
	struct sunxi_pm_domain *pd;
	u32 id;
	bool is_always_on;
	int error;

	is_always_on = of_property_read_bool(node, "ppu-always-on");

	error = of_property_read_u32(node, "reg", &id);
	if (error) {
		sunxi_err(pmu->dev,
			"%pOFn: failed to retrieve domain id (reg): %d\n",
			node, error);
		return -EINVAL;
	}

	if (id >= pmu->info->num_domains) {
		sunxi_err(pmu->dev, "%pOFn: invalid domain id %d\n",
			node, id);
		return -EINVAL;
	}

	pd_info = &pmu->info->domain_info[id];
	if (!pd_info) {
		sunxi_err(pmu->dev, "%pOFn: undefined domain id %d\n",
			node, id);
		return -EINVAL;
	}

	pd = devm_kzalloc(pmu->dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	pd->info = pd_info;
	pd->pmu = pmu;

	error = sunxi_pd_init(pd);
	if (error) {
		sunxi_err(pmu->dev,
			"failed to power on domain '%pOFn': %d\n",
			node, error);
		goto err;
	}

	pd->genpd.name = node->name;
	pd->genpd.power_off = sunxi_pd_power_off;
	pd->genpd.power_on = sunxi_pd_power_on;
	pd->genpd.attach_dev = sunxi_pd_attach_dev;
	pd->genpd.detach_dev = sunxi_pd_detach_dev;
	pd->genpd.flags = (is_always_on) ? GENPD_FLAG_ALWAYS_ON : 0;
	pm_genpd_init(&pd->genpd, NULL, false);

	pmu->genpd_data.domains[id] = &pd->genpd;
	return 0;

err:
	return error;
}

static void sunxi_pm_remove_one_domain(struct sunxi_pm_domain *pd)
{
	int ret;

	/*
	 * We're in the error cleanup already, so we only complain,
	 * but won't emit another error on top of the original one.
	 */
	ret = pm_genpd_remove(&pd->genpd);
	if (ret < 0)
		sunxi_err(pd->pmu->dev, "failed to remove domain '%s' : %d - state may be inconsistent\n",
			pd->genpd.name, ret);
}

static void sunxi_pm_domain_cleanup(struct sunxi_pmu *pmu)
{
	struct generic_pm_domain *genpd;
	struct sunxi_pm_domain *pd;
	int i;

	for (i = 0; i < pmu->genpd_data.num_domains; i++) {
		genpd = pmu->genpd_data.domains[i];
		if (genpd) {
			pd = to_sunxi_pd(genpd);
			sunxi_pm_remove_one_domain(pd);
		}
	}

	/* devm will free our memory */
}

static int sunxi_pm_add_subdomain(struct sunxi_pmu *pmu,
				     struct device_node *parent)
{
	struct device_node *np;
	struct generic_pm_domain *child_domain, *parent_domain;
	int error;

	for_each_child_of_node(parent, np) {
		u32 idx;

		error = of_property_read_u32(parent, "reg", &idx);
		if (error) {
			sunxi_err(pmu->dev,
				"%pOFn: failed to retrieve domain id (reg): %d\n",
				parent, error);
			goto err_out;
		}
		parent_domain = pmu->genpd_data.domains[idx];

		error = sunxi_pm_add_one_domain(pmu, np);
		if (error) {
			sunxi_err(pmu->dev, "failed to handle node %pOFn: %d\n",
				np, error);
			goto err_out;
		}

		error = of_property_read_u32(np, "reg", &idx);
		if (error) {
			sunxi_err(pmu->dev,
				"%pOFn: failed to retrieve domain id (reg): %d\n",
				np, error);
			goto err_out;
		}
		child_domain = pmu->genpd_data.domains[idx];

		error = pm_genpd_add_subdomain(parent_domain, child_domain);
		if (error) {
			sunxi_err(pmu->dev, "%s failed to add subdomain %s: %d\n",
				parent_domain->name, child_domain->name, error);
			goto err_out;
		} else {
			sunxi_debug(pmu->dev, "%s add subdomain: %s\n",
				parent_domain->name, child_domain->name);
		}

		sunxi_pm_add_subdomain(pmu, np);
	}

	return 0;

err_out:
	of_node_put(np);
	return error;
}

static int sunxi_pm_domain_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *node;
	struct device *parent;
	struct sunxi_pmu *pmu;
	const struct of_device_id *match;
	const struct sunxi_pmu_info *pmu_info;
	int error;

	if (!np) {
		sunxi_err(dev, "device tree node not found\n");
		return -ENODEV;
	}

	match = of_match_device(dev->driver->of_match_table, dev);
	if (!match || !match->data) {
		sunxi_err(dev, "missing pmu data\n");
		return -EINVAL;
	}

	pmu_info = match->data;

	pmu = devm_kzalloc(dev,
			   struct_size(pmu, domains, pmu_info->num_domains),
			   GFP_KERNEL);
	if (!pmu)
		return -ENOMEM;

	pmu->dev = &pdev->dev;
	platform_set_drvdata(pdev, pmu);

	mutex_init(&pmu->mutex);

	pmu->info = pmu_info;

	pmu->genpd_data.domains = pmu->domains;
	pmu->genpd_data.num_domains = pmu_info->num_domains;

	parent = dev->parent;
	if (!parent) {
		sunxi_err(dev, "no parent for syscon devices\n");
		return -ENODEV;
	}

	pmu->regmap = syscon_node_to_regmap(parent->of_node);
	if (IS_ERR(pmu->regmap)) {
		sunxi_err(dev, "no regmap available\n");
		return PTR_ERR(pmu->regmap);
	}

	error = -ENODEV;

	for_each_available_child_of_node(np, node) {
		error = sunxi_pm_add_one_domain(pmu, node);
		if (error) {
			sunxi_err(dev, "failed to handle node %pOFn: %d\n",
				node, error);
			of_node_put(node);
			goto err_out;
		}

		error = sunxi_pm_add_subdomain(pmu, node);
		if (error < 0) {
			sunxi_err(dev, "failed to handle subdomain node %pOFn: %d\n",
				node, error);
			of_node_put(node);
			goto err_out;
		}
	}

	if (error) {
		sunxi_debug(dev, "no power domains defined\n");
		goto err_out;
	}

	pmu->reset = devm_reset_control_get(dev, "ppu_rst");
	if (IS_ERR(pmu->reset)) {
		error = PTR_ERR(pmu->reset);
		sunxi_err(dev, "failed to get reset control: %ld\n", PTR_ERR(pmu->reset));
		goto err_out;
	}

	pmu->clk = devm_clk_get(dev, "ppu");
	if (IS_ERR(pmu->clk)) {
		error = PTR_ERR(pmu->clk);
		sunxi_err(&pdev->dev, "failed to get clock: %ld\n", PTR_ERR(pmu->clk));
		goto err_out;
	}

	error = reset_control_deassert(pmu->reset);
	if (error) {
		sunxi_err(dev, "reset control deassert failed!\n");
		goto err_out;
	}

	error = clk_prepare_enable(pmu->clk);
	if (error) {
		sunxi_err(dev, "clk prepare enable failed!\n");
		goto assert_reset;
	}

	error = of_genpd_add_provider_onecell(np, &pmu->genpd_data);
	if (error) {
		sunxi_err(dev, "failed to add provider: %d\n", error);
		goto clk_disable;
	}

	return 0;

clk_disable:
	clk_disable_unprepare(pmu->clk);

assert_reset:
	reset_control_assert(pmu->reset);

err_out:
	sunxi_pm_domain_cleanup(pmu);
	return error;
}

static int sunxi_pm_domain_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_pmu *pmu = platform_get_drvdata(pdev);

	if (IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)) {
		of_genpd_del_provider(np);
		sunxi_pm_domain_cleanup(pmu);
	}

	clk_disable_unprepare(pmu->clk);
	reset_control_assert(pmu->reset);
	return 0;
}

static const struct sunxi_domain_info tv303_pm_domains[] = {
	[TV303_PD_GPU] = DOMAIN(TV303_PD_GPU,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[TV303_PD_TVFE] = DOMAIN(TV303_PD_TVFE,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[TV303_PD_TVCAP] = DOMAIN(TV303_PD_TVCAP, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
	[TV303_PD_VE] = DOMAIN(TV303_PD_VE, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
	[TV303_PD_AV1] = DOMAIN(TV303_PD_AV1, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
};

static const struct sunxi_pmu_info tv303_pmu = {
	.wait_mode_offset = 0x14,
	.pwr_off_delay_offset = 0x18,
	.pwr_on_delay_offset = 0x1c,
	.pwr_offset = 0x20,
	.status_offset = 0x24,
	.num_domains = ARRAY_SIZE(tv303_pm_domains),
	.domain_info = tv303_pm_domains,
};

static const struct sunxi_domain_info r528_pm_domains[] = {
	[R528_PD_CPU] = DOMAIN(R528_PD_CPU,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[R528_PD_VE] = DOMAIN(R528_PD_VE,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[R528_PD_DSP] = DOMAIN(R528_PD_DSP, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
};

static const struct sunxi_pmu_info r528_pmu = {
	.wait_mode_offset = 0x14,
	.pwr_off_delay_offset = 0x18,
	.pwr_on_delay_offset = 0x1c,
	.pwr_offset = 0x20,
	.status_offset = 0x24,
	.num_domains = ARRAY_SIZE(r528_pm_domains),
	.domain_info = r528_pm_domains,
};

static const struct sunxi_domain_info a523_pm_domains[] = {
	[A523_PD_DSP] = DOMAIN(A523_PD_DSP,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[A523_PD_NPU] = DOMAIN(A523_PD_NPU,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[A523_PD_AUDIO] = DOMAIN(A523_PD_AUDIO, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
	[A523_PD_SRAM] = DOMAIN(A523_PD_SRAM, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
	[A523_PD_RISCV] = DOMAIN(A523_PD_RISCV, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
};

static const struct sunxi_pmu_info a523_pmu = {
	.wait_mode_offset = 0x14,
	.pwr_off_delay_offset = 0x18,
	.pwr_on_delay_offset = 0x1c,
	.pwr_offset = 0x20,
	.status_offset = 0x24,
	.num_domains = ARRAY_SIZE(a523_pm_domains),
	.domain_info = a523_pm_domains,
};

static const struct sunxi_domain_info v851_pm_domains[] = {
	[V851_PD_E907] = DOMAIN(V851_PD_E907,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[V851_PD_NPU] = DOMAIN(V851_PD_NPU,  0x8, 0x080808,  0x080808,  BIT(3),  0x30000,  BIT(1)),
	[V851_PD_VE] = DOMAIN(V851_PD_VE, 0x8, 0x080808, 0x080808,  BIT(3),  0x30000,  BIT(1)),
};

static const struct sunxi_pmu_info v851_pmu = {
	.wait_mode_offset = 0x14,
	.pwr_off_delay_offset = 0x18,
	.pwr_on_delay_offset = 0x1c,
	.pwr_offset = 0x20,
	.status_offset = 0x24,
	.num_domains = ARRAY_SIZE(v851_pm_domains),
	.domain_info = v851_pm_domains,
};

static const struct of_device_id sunxi_pm_domain_dt_match[] = {
	{
		.compatible = "allwinner,tv303-power-controller",
		.data = (void *)&tv303_pmu,
	},
	{
		.compatible = "allwinner,r528-power-controller",
		.data = (void *)&r528_pmu,
	},
	{
		.compatible = "allwinner,a523-power-controller",
		.data = (void *)&a523_pmu,
	},
	{
		.compatible = "allwinner,v851-power-controller",
		.data = (void *)&v851_pmu,
	},
	{ /* sentinel */ },
};

static struct platform_driver power_domain_driver = {
	.probe = sunxi_pm_domain_probe,
	.remove  = sunxi_pm_domain_remove,
	.driver = {
		.name   = "sunxi-pm-domain",
		.of_match_table = sunxi_pm_domain_dt_match,
		/*
		 * We can't forcibly eject devices form power domain,
		 * so we can't really remove power domains once they
		 * were added.
		 */
		.suppress_bind_attrs = true,
	},
};

static int __init sunxi_power_domain_init(void)
{
	return platform_driver_register(&power_domain_driver);
}

static void __exit sunxi_power_domain_exit(void)
{
	platform_driver_unregister(&power_domain_driver);
}

subsys_initcall(sunxi_power_domain_init);
module_exit(sunxi_power_domain_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner power domain driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.1");

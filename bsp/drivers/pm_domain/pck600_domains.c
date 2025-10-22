// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner pck-600 power domain support.
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
#include <dt-bindings/power/a523-power.h>
#include <dt-bindings/power/sun60iw2-power.h>

struct sunxi_domain_info {
	u32 domain_id;
	u32 device_ctrl0_delay;
	u32 device_ctrl1_delay;
	u32 logic_power_switch0_delay;
	u32 logic_power_switch1_delay;
	u32 off2on_delay;
	u32 status_mask;
};

struct sunxi_pmu_info {
	u32 pwr_offset;
	u32 status_offset;
	u32 device_ctrl0_delay_offset;
	u32 device_ctrl1_delay_offset;
	u32 logic_power_switch0_delay_offset;
	u32 logic_power_switch1_delay_offset;
	u32 off2on_delay_offset;
	u32 num_domains;
	bool has_rst_clk;
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

#define DRIVER_NAME	"pck-600-domain-driver"

#define to_sunxi_pd(gpd) container_of(gpd, struct sunxi_pm_domain, genpd)

#define DOMAIN(id, ctrl0, ctrl1, logic0, logic1, off2on, smask)	\
{							\
	.domain_id = (id),			\
	.device_ctrl0_delay = (ctrl0),			\
	.device_ctrl1_delay = (ctrl1),				\
	.logic_power_switch0_delay = (logic0),			\
	.logic_power_switch1_delay = (logic1),			\
	.off2on_delay = (off2on),				\
	.status_mask = (smask),			\
}

#define COMMAND_ON	0x8
#define COMMAND_OFF	0x0
#define STATUS_ON	0x8
#define STATUS_OFF	0x0
#define BASE(id)	((id) << 12)

static bool sunxi_pmu_domain_is_on(struct sunxi_pm_domain *pd)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;
	unsigned int val;

	regmap_read(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->status_offset, &val);

	return (val & pd->info->status_mask) == STATUS_ON;
}

static void sunxi_do_pmu_set_power_domain(struct sunxi_pm_domain *pd,
					     bool on)
{
	struct sunxi_pmu *pmu = pd->pmu;
	const struct sunxi_domain_info *pd_info = pd->info;
	struct generic_pm_domain *genpd = &pd->genpd;
	bool is_on;

	regmap_write_bits(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->pwr_offset, pd_info->status_mask,
			     on ? COMMAND_ON : COMMAND_OFF);

	dsb(sy);

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

	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->device_ctrl0_delay_offset,
			     pd_info->device_ctrl0_delay);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->device_ctrl1_delay_offset,
			     pd_info->device_ctrl1_delay);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->logic_power_switch0_delay_offset,
			     pd_info->logic_power_switch0_delay);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->logic_power_switch1_delay_offset,
			     pd_info->logic_power_switch1_delay);
	regmap_write(pmu->regmap, BASE(pd_info->domain_id) + pmu->info->off2on_delay_offset,
			     pd_info->off2on_delay);
	return 0;
}

static int sunxi_pd_power(struct sunxi_pm_domain *pd, bool power_on)
{
	struct sunxi_pmu *pmu = pd->pmu;

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
	if (pmu_info->has_rst_clk) {
	pmu->reset = devm_reset_control_get(dev, "pck_rst");
	if (IS_ERR(pmu->reset)) {
		error = PTR_ERR(pmu->reset);
		sunxi_err(dev, "failed to get reset control: %ld\n", PTR_ERR(pmu->reset));
		goto err_out;
		}
	}

	pmu->clk = devm_clk_get(dev, "pck");
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

static const struct sunxi_domain_info a523_pck600_pm_domains[] = {
	[A523_PCK_VE] = DOMAIN(A523_PCK_VE,  0xffffff, 0xffff,  0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_GPU] = DOMAIN(A523_PCK_GPU,  0xffffff, 0xffff,  0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_VI] = DOMAIN(A523_PCK_VI, 0xffffff, 0xffff, 0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_VO0] = DOMAIN(A523_PCK_VO0, 0xffffff, 0xffff, 0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_VO1] = DOMAIN(A523_PCK_VO1, 0xffffff, 0xffff, 0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_DE] = DOMAIN(A523_PCK_DE, 0xffffff, 0xffff, 0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_NAND] = DOMAIN(A523_PCK_NAND, 0xffffff, 0xffff, 0x8080808,  0x808,  0x8,  0xf),
	[A523_PCK_PCIE] = DOMAIN(A523_PCK_PCIE, 0xffffff, 0xffff, 0x8080808,  0x808,  0x8,  0xf),
};

static const struct sunxi_pmu_info a523_pck600_pmu = {
	.pwr_offset = 0x0,
	.status_offset = 0x8,
	.device_ctrl0_delay_offset = 0x170,
	.device_ctrl1_delay_offset = 0x174,
	.logic_power_switch0_delay_offset = 0xc00,
	.logic_power_switch1_delay_offset = 0xc04,
	.off2on_delay_offset = 0xc10,
	.num_domains = ARRAY_SIZE(a523_pck600_pm_domains),
	.domain_info = a523_pck600_pm_domains,
	.has_rst_clk = true,
};

static const struct sunxi_domain_info sun60iw2_pck600_pm_domains[] = {
	[SUN60IW2_PCK_VI] = DOMAIN(SUN60IW2_PCK_VI,  0x1f1f1f, 0x1f1f,  0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_DE_SYS] = DOMAIN(SUN60IW2_PCK_DE_SYS,  0x1f1f1f, 0x1f1f,  0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_VE_DEC] = DOMAIN(SUN60IW2_PCK_VE_DEC, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_VE_ENC] = DOMAIN(SUN60IW2_PCK_VE_ENC, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_NPU] = DOMAIN(SUN60IW2_PCK_NPU, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_GPU_TOP] = DOMAIN(SUN60IW2_PCK_GPU_TOP,  0x1f1f1f, 0x1f1f,  0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_GPU_CORE] = DOMAIN(SUN60IW2_PCK_GPU_CORE,  0x1f1f1f, 0x1f1f,  0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_PCIE] = DOMAIN(SUN60IW2_PCK_PCIE, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_USB2] = DOMAIN(SUN60IW2_PCK_USB2, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_VO] = DOMAIN(SUN60IW2_PCK_VO, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
	[SUN60IW2_PCK_VO1] = DOMAIN(SUN60IW2_PCK_VO1, 0x1f1f1f, 0x1f1f, 0x8080808,  0x808,  0x8,  0xf),
};
static const struct sunxi_pmu_info sun60iw2_pck600_pmu = {
	.pwr_offset = 0x0,
	.status_offset = 0x8,
	.device_ctrl0_delay_offset = 0x170,
	.device_ctrl1_delay_offset = 0x174,
	.logic_power_switch0_delay_offset = 0xc00,
	.logic_power_switch1_delay_offset = 0xc04,
	.off2on_delay_offset = 0xc10,
	.num_domains = ARRAY_SIZE(sun60iw2_pck600_pm_domains),
	.domain_info = sun60iw2_pck600_pm_domains,
	.has_rst_clk = false,
};
static const struct of_device_id sunxi_pm_domain_dt_match[] = {
	{
		.compatible = "allwinner,a523-pck-600",
		.data = (void *)&a523_pck600_pmu,
	},
	{
		.compatible = "allwinner,sun60iw2-pck-600",
		.data = (void *)&sun60iw2_pck600_pmu,
	},
	{ /* sentinel */ },
};

static struct platform_driver power_domain_driver = {
	.probe = sunxi_pm_domain_probe,
	.remove  = sunxi_pm_domain_remove,
	.driver = {
		.name   = "sunxi-pck600-domain",
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

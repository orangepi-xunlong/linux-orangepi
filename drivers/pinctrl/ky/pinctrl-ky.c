// SPDX-License-Identifier: GPL-2.0+
//
// Copyright (c) 2023, ky Corporation.
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include "../core.h"
#include "pinctrl-ky.h"

struct ky_pinctrl_data {
	struct device *dev;
	struct pinctrl_dev *pctl;
	void __iomem *base;
	struct reset_control *rstc;
	struct ky_pinctrl_soc_data *soc;
};

static int ky_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);

	return d->soc->ngroups;
}

static const char *ky_get_group_name(struct pinctrl_dev *pctldev,
					unsigned group)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);

	if (group >= d->soc->ngroups)
		return NULL;

	return d->soc->groups[group].name;
}

static int ky_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
				const unsigned **pins, unsigned *num_pins)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);

	if (group >= d->soc->ngroups)
		return -EINVAL;

	*pins = d->soc->groups[group].pin_ids;
	*num_pins = d->soc->groups[group].npins;

	return 0;
}

static void ky_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
				unsigned offset)
{
	seq_printf(s, " %s", dev_name(pctldev->dev));
}

static int ky_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np,
				struct pinctrl_map **map, unsigned *num_maps)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);
	const struct ky_group *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	int map_num = 1;
	int i, j;

	/*
	 * first find the group of this node and check if we need create
	 * config maps for pins
	 */
	grp = NULL;
	for (i = 0; i < d->soc->ngroups; i++) {
		if (!strcmp(d->soc->groups[i].name, np->name)) {
			grp = &d->soc->groups[i];
			break;
		}
	}
	if (!grp) {
		dev_err(d->dev, "unable to find group for node %s\n",
			np->name);
		return -EINVAL;
	}

	for (i = 0; i < grp->npins; i++)
		map_num++;

	new_map = kmalloc_array(map_num, sizeof(struct pinctrl_map),
				GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = map_num;

	/* create mux map */
	parent = of_get_parent(np);
	if (!parent) {
		kfree(new_map);
		return -EINVAL;
	}
	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = np->name;
	new_map[0].data.mux.group = np->name;
	of_node_put(parent);

	/* create config map */
	new_map++;
	for (i = j = 0; i < grp->npins; i++) {
		new_map[j].type = PIN_MAP_TYPE_CONFIGS_PIN;
		new_map[j].data.configs.group_or_pin =
			pin_get_name(pctldev, grp->pins[i].pin_id);
		new_map[j].data.configs.configs = &grp->pins[i].config;
		new_map[j].data.configs.num_configs = 1;
		j++;
	}

	dev_dbg(pctldev->dev, "maps: function %s group %s num %d\n",
		(*map)->data.mux.function, (*map)->data.mux.group, map_num);

	return 0;
}

static void ky_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	u32 i;

	for (i = 0; i < num_maps; i++) {
		if (map[i].type == PIN_MAP_TYPE_MUX_GROUP)
			kfree(map[i].data.mux.group);
		if (map[i].type == PIN_MAP_TYPE_CONFIGS_PIN)
			kfree(map[i].data.configs.configs);
	}

	kfree(map);
}

static const struct pinctrl_ops ky_pinctrl_ops = {
	.get_groups_count = ky_get_groups_count,
	.get_group_name = ky_get_group_name,
	.get_group_pins = ky_get_group_pins,
	.pin_dbg_show = ky_pin_dbg_show,
	.dt_node_to_map = ky_dt_node_to_map,
	.dt_free_map = ky_dt_free_map,
};

static int ky_pinctrl_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);

	return d->soc->nfunctions;
}

static const char *ky_pinctrl_get_func_name(struct pinctrl_dev *pctldev,
						unsigned function)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);

	return d->soc->functions[function].name;
}

static int ky_pinctrl_get_func_groups(struct pinctrl_dev *pctldev,
					unsigned group,
					const char * const **groups,
					unsigned * const num_groups)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);

	*groups = d->soc->functions[group].groups;
	*num_groups = d->soc->functions[group].ngroups;

	return 0;
}

static void ky_pinctrl_rmwl(u32 value, u32 mask, u8 shift, void __iomem *reg)
{
	u32 tmp;

	tmp = readl(reg);
	tmp &= ~(mask << shift);
	tmp |= value << shift;
	writel(tmp, reg);
}

static int ky_pinctrl_set_mux(struct pinctrl_dev *pctldev, unsigned selector,
				unsigned group)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);
	struct ky_group *g = &d->soc->groups[group];
	void __iomem *reg;
	u8 bank, shift, width;
	u16 offset;
	u32 i;

	for (i = 0; i < g->npins; i++) {
		bank = PINID_TO_BANK(g->pin_ids[i]);
		offset = PINID_TO_PIN(g->pin_ids[i]);
		reg = d->base + d->soc->regs->cfg;
		reg += bank * d->soc->regs->reg_len + offset * 4;
		shift = d->soc->pinconf->fs_shift;
		width = d->soc->pinconf->fs_width;

		dev_dbg(d->dev, "set mux: bank %d 0ffset %d val 0x%lx\n",
			bank, offset, g->pins[i].muxsel);

		ky_pinctrl_rmwl(g->pins[i].muxsel, GENMASK((width-1),0), shift, reg);
	}

	return 0;
}

static const struct pinmux_ops ky_pinmux_ops = {
	.get_functions_count = ky_pinctrl_get_funcs_count,
	.get_function_name = ky_pinctrl_get_func_name,
	.get_function_groups = ky_pinctrl_get_func_groups,
	.set_mux = ky_pinctrl_set_mux,
};

static int ky_pinconf_get(struct pinctrl_dev *pctldev,
			unsigned pin, unsigned long *config)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);
	u8 bank;
	u16 offset = 0;
	u64 reg = 0;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_PIN(pin);
	reg = (u64)(d->base + d->soc->regs->cfg);
	reg += bank * d->soc->regs->reg_len + offset * 4;

	*config = readl((void *)reg);
	return 0;
}

static int ky_pinconf_set(struct pinctrl_dev *pctldev,
			unsigned pin, unsigned long *configs,
			unsigned num_configs)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);
	struct ky_pinctrl_soc_data *soc = d->soc;
	const struct ky_regs *regs = soc->regs;
	const struct ky_pin_conf *pin_conf = soc->pinconf;
	int i;
	u8 bank;
	u32 od, pull_en, pull, ds, st, rte;
	u16 offset = 0;
	u64 reg = 0;

	dev_dbg(d->dev, "pinconf set pin %s\n",
		pin_get_name(pctldev, pin));

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_PIN(pin);
	reg = (u64)(d->base + regs->cfg);
	reg += bank * regs->reg_len + offset * 4;

	for (i = 0; i < num_configs; i++) {
		volatile long config;

		config = readl((void *)reg);

		od = OD_DIS << pin_conf->od_shift;
		pull_en = PE_EN << pin_conf->pe_shift;
		pull = CONFIG_TO_PULL(configs[i]) << pin_conf->pull_shift;
		ds = CONFIG_TO_DS(configs[i]) << pin_conf->ds_shift;
		st = ST_DIS << pin_conf->st_shift;
		rte = RTE_EN << pin_conf->rte_shift;

		config |= (od | pull_en | pull | ds | st | rte);
		writel(config, (void *)reg);
		dev_dbg(d->dev, "write: bank %d 0ffset %d val 0x%lx\n",
			bank, offset, config);
	} /* for each config */

	return 0;
}

static void ky_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned pin)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);
	u8 bank;
	u16 offset = 0;
	u64 reg = 0;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_PIN(pin);
	reg = (u64)(d->base + d->soc->regs->cfg);
	reg += bank * d->soc->regs->reg_len + offset * 4;

	seq_printf(s, "0x%lx", readl((void *)reg));
}

static void ky_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s, unsigned group)
{
	struct ky_pinctrl_data *d = pinctrl_dev_get_drvdata(pctldev);
	struct ky_group *grp;
	unsigned long config;
	const char *name;
	int i, ret;

	if (group > d->soc->ngroups)
		return;

	seq_puts(s, "\n");
	grp = &d->soc->groups[group];
	for (i = 0; i < grp->npins; i++) {
		struct ky_pin *pin = &grp->pins[i];

		name = pin_get_name(pctldev, pin->pin_id);
		ret = ky_pinconf_get(pctldev, pin->pin_id, &config);
		if (ret)
			return;
		seq_printf(s, "%s: 0x%lx", name, config);
	}
}

static const struct pinconf_ops ky_pinconf_ops = {
	.pin_config_get = ky_pinconf_get,
	.pin_config_set = ky_pinconf_set,
	.pin_config_dbg_show = ky_pinconf_dbg_show,
	.pin_config_group_dbg_show = ky_pinconf_group_dbg_show,
};

static struct pinctrl_desc ky_pinctrl_desc = {
	.pctlops = &ky_pinctrl_ops,
	.pmxops = &ky_pinmux_ops,
	.confops = &ky_pinconf_ops,
	.owner = THIS_MODULE,
};

static const char *get_pin_name_from_soc(struct ky_pinctrl_soc_data *soc,
					const unsigned int pin_id)
{
	int i;

	for (i = 0; i < soc->npins; i++) {
		if (soc->pins[i].number == pin_id)
			return soc->pins[i].name;
	}

	return NULL;
}

static int ky_pinctrl_parse_groups(struct device_node *np,
					struct ky_group *grp,
					struct ky_pinctrl_data *d,
					u32 index)
{
	int size, i;
	const __be32 *list;

	dev_dbg(d->dev, "group(%d): %s\n", index, np->name);

	/* Initialise group */
	grp->name = np->name;

	/*
	 * the binding format is ky,pins = <PIN MUX CONFIG>,
	 * do sanity check and calculate pins number
	 */
	list = of_get_property(np, "ky,pins", &size);
	if (!list) {
		dev_err(d->dev, "no ky,pins property in node %s\n",
			np->full_name);
		return -EINVAL;
	}

	if (!size || size % KY_PIN_SIZE) {
		dev_err(d->dev, "Invalid ky,pins property in node %s\n",
			np->full_name);
		return -EINVAL;
	}

	grp->npins = size / KY_PIN_SIZE;
	grp->pins = devm_kcalloc(d->dev, grp->npins,
				sizeof(struct ky_pin), GFP_KERNEL);
	grp->pin_ids = devm_kcalloc(d->dev, grp->npins,
					sizeof(unsigned int), GFP_KERNEL);
	if (!grp->pins || !grp->pin_ids)
		return -ENOMEM;

	for (i = 0; i < grp->npins; i++) {
		struct ky_pin *pin = &grp->pins[i];
		u8 pull_val, driver_strength;

		pin->pin_id = be32_to_cpu(*list++);
		pin->muxsel = be32_to_cpu(*list++) & 0xF;
		pull_val = be32_to_cpu(*list++) & 0x1;
		driver_strength =  be32_to_cpu(*list++) & 0xF;
		pin->config = (pull_val << PULL_SHIFT) | (driver_strength << DS_SHIFT);
		grp->pin_ids[i] = grp->pins[i].pin_id;

		dev_dbg(d->dev, "%s: 0x%04x 0x%04lx",
			get_pin_name_from_soc(d->soc, pin->pin_id), pin->muxsel, pin->config);
	}

	return 0;
}

static int ky_pinctrl_parse_functions(struct device_node *np,
					struct ky_pinctrl_data *d)
{
	struct ky_pinctrl_soc_data *soc = d->soc;
	struct device_node *child;
	struct ky_function *f;
	u32 i = 0, idxf = 0, idxg = 0;
	const char *fn, *fnull = "";
	int ret;

	/* Count groups for each function */
	fn = fnull;
	f = &soc->functions[idxf];
	for_each_child_of_node(np, child) {
		if (strcmp(fn, child->name)) {
			struct device_node *child2;

			/*
			 * This reference is dropped by
			 * of_get_next_child(np, * child)
			 */
			of_node_get(child);

			/*
			 * The logic parsing the functions from dt currently
			 * doesn't handle if functions with the same name are
			 * not grouped together. Only the first contiguous
			 * cluster is usable for each function name. This is a
			 * bug that is not trivial to fix, but at least warn
			 * about it.
			 */
			for (child2 = of_get_next_child(np, child);
				child2 != NULL;
				child2 = of_get_next_child(np, child2)) {
				if (!strcmp(child2->name, fn))
					dev_warn(d->dev,
						"function nodes must be grouped by name (failed for: %s)",
						fn);
			}

			f = &soc->functions[idxf++];
			f->name = fn = child->name;
		}
		f->ngroups++;
		dev_dbg(d->dev, "function(%d): %s\n", idxf-1, f->name);
	}

	/* Get groups for each function */
	idxf = 0;
	fn = fnull;
	for_each_child_of_node(np, child) {
		if (strcmp(fn, child->name)) {
			f = &soc->functions[idxf++];
			f->groups = devm_kcalloc(d->dev,
						f->ngroups,
						sizeof(*f->groups),
						GFP_KERNEL);
			if (!f->groups) {
				of_node_put(child);
				return -ENOMEM;
			}
			fn = child->name;
			i = 0;
		}

		f->groups[i] = child->name;
		ret = ky_pinctrl_parse_groups(child, &soc->groups[idxg++], d, i++);
		if (ret) {
			of_node_put(child);
			return ret;
		}
	}

	return 0;
}

static int ky_pinctrl_probe_dt(struct platform_device *pdev,
				struct ky_pinctrl_data *d)
{
	struct ky_pinctrl_soc_data *soc = d->soc;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	const char *fn, *fnull = "";

	if (!np)
		return -ENODEV;

	/* Count total functions and groups */
	fn = fnull;
	for_each_child_of_node(np, child) {
		soc->ngroups++;
		if (strcmp(fn, child->name)) {
			fn = child->name;
			soc->nfunctions++;
		}
	}

	if (soc->nfunctions <= 0) {
		dev_err(&pdev->dev, "It has no functions\n");
		return -EINVAL;
	}

	soc->functions = devm_kcalloc(&pdev->dev, soc->nfunctions,
					sizeof(struct ky_function),
					GFP_KERNEL);
	if (!soc->functions)
		return -ENOMEM;

	soc->groups = devm_kcalloc(&pdev->dev, soc->ngroups,
					sizeof(struct ky_group),
					GFP_KERNEL);
	if (!soc->groups)
		return -ENOMEM;

	ky_pinctrl_parse_functions(np, d);

	return 0;
}

int ky_pinctrl_probe(struct platform_device *pdev,
			struct ky_pinctrl_soc_data *soc)
{
	struct ky_pinctrl_data *d;
	struct resource *res;
	int ret;

	d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	d->dev = &pdev->dev;
	d->soc = soc;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	d->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!d->base)
		return -EADDRNOTAVAIL;

	ky_pinctrl_desc.pins = d->soc->pins;
	ky_pinctrl_desc.npins = d->soc->npins;
	ky_pinctrl_desc.name = dev_name(&pdev->dev);

	platform_set_drvdata(pdev, d);

	d->rstc = devm_reset_control_get_optional_exclusive(&pdev->dev, NULL);
	if (IS_ERR(d->rstc)) {
		ret = PTR_ERR(d->rstc);
		dev_err(&pdev->dev, "failed to get reset.\n");
		goto err;
	}
	reset_control_deassert(d->rstc);

	ret = ky_pinctrl_probe_dt(pdev, d);
	if (ret) {
		dev_err(&pdev->dev, "dt probe failed: %d\n", ret);
		goto err;
	}

	d->pctl = pinctrl_register(&ky_pinctrl_desc, &pdev->dev, d);
	if (IS_ERR(d->pctl)) {
		dev_err(&pdev->dev, "Couldn't register ky pinctrl driver\n");
		ret = PTR_ERR(d->pctl);
		goto err;
	}

	return 0;

err:
	iounmap(d->base);
	return ret;
}
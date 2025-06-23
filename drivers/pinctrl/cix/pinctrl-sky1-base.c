// SPDX-License-Identifier: GPL-2.0+
//
// Author: Jerry Zhu <Jerry.Zhu@cixtech.com>

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/acpi.h>

#include "../core.h"
#include "../pinconf.h"
#include "../pinmux.h"
#include "pinctrl-sky1.h"
#include "../pinctrl-acpi.h"

#define SKY1_PIN_SIZE		(0xc)
#define SKY1_MUX_MASK		(0x180L)
#define SKY1_CONF_MASK		(0x7fL)
#define PADS_FUNCS_MASK		(0x3)
#define PADS_FUNCS_BITS		(0x7)
#define PADS_CONFS_MASK		(0x7f)

static inline const struct group_desc *sky1_pinctrl_find_group_by_name(
				struct pinctrl_dev *pctldev,
				const char *name)
{
	const struct group_desc *grp = NULL;
	int i;

	for (i = 0; i < pctldev->num_groups; i++) {
		grp = pinctrl_generic_get_group(pctldev, i);
		if (grp && !strcmp(grp->name, name))
			break;
	}

	return grp;
}

static void sky1_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
		   unsigned offset)
{
	seq_printf(s, "%s", dev_name(pctldev->dev));
}

static int sky1_dt_node_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map **map, unsigned *num_maps)
{
	struct sky1_pinctrl *spctl = pinctrl_dev_get_drvdata(pctldev);
	const struct group_desc *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	struct sky1_pin *pin;
	int map_num = 1;
	int i, j;

	/*
	 * first find the group of this node and check if we need create
	 * config maps for pins
	 */
	grp = sky1_pinctrl_find_group_by_name(pctldev, np->name);
	if (!grp) {
		dev_err(spctl->dev, "unable to find group for node %pOFn\n", np);
		return -EINVAL;
	}

	map_num += grp->num_pins;

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
	new_map[0].data.mux.function = parent->name;
	new_map[0].data.mux.group = np->name;
	of_node_put(parent);

	/* create config map */
	new_map++;
	for (i = j = 0; i < grp->num_pins; i++) {
		pin = &((struct sky1_pin *)(grp->data))[i];

		new_map[j].type = PIN_MAP_TYPE_CONFIGS_PIN;
		new_map[j].data.configs.group_or_pin =
			pin_get_name(pctldev, pin->offset/4);
		new_map[j].data.configs.configs = &pin->configs;
		new_map[j].data.configs.num_configs = 1;

		j++;
	}

	dev_dbg(pctldev->dev, "maps: function %s group %s num %d\n",
		(*map)->data.mux.function, (*map)->data.mux.group, map_num);

	return 0;
}

static inline const char *
sky1_pinctrl_find_pin_group(struct pinctrl_dev *pctldev,
			    unsigned selector,
			    unsigned int *pins_in,
			    size_t npins_in)
{
	int i, j;
	int ret;
	const char *const *groups;
	unsigned num_groups;
	const unsigned *pins;
	unsigned num_pins;

	ret = pinmux_generic_get_function_groups(pctldev, selector, &groups,
						 &num_groups);
	for (i = 0; i < num_groups; i++) {
		ret = pinctrl_get_group_pins(pctldev, groups[i], &pins,
					     &num_pins);
		if (npins_in != num_pins)
			continue;
		for (j = 0; j < num_pins; j++) {
			if (pins[j] == pins_in[0])
				return groups[i];
		}
	}
	return NULL;
}

static int sky1_acpi_node_to_map(struct pinctrl_dev *pctldev,
				struct pinctrl_acpi_resource *info,
				struct pinctrl_map **map,
				unsigned *num_maps_out)
{
	struct pinctrl_map *new_map;
	struct function_desc *fun_desc;
	struct pinctrl_acpi_config_node *config_node;
	struct pinctrl_map_configs *map_config;
	const char *grp;
	size_t num_maps = 1;
	int fun_selector;

	new_map = kzalloc(sizeof(struct pinctrl_map), GFP_KERNEL);

	switch (info->type) {
	case PINCTRL_ACPI_PIN_FUNCTION:
		fun_selector = info->function.function_number;
		grp = sky1_pinctrl_find_pin_group(pctldev, fun_selector,
						  info->function.pins,
						  info->function.npins);
		if (!grp) {
			dev_err(pctldev->dev, "Pin group cannot be found\n");
			return -1;
		}
		new_map->type = PIN_MAP_TYPE_MUX_GROUP;
		new_map->data.mux.group = grp;
		fun_desc = pinmux_generic_get_function(pctldev, fun_selector);
		if (!fun_desc) {
			dev_err(pctldev->dev, "Func numb %d cannot be mapped\n",
				info->function.function_number);
			return -1;
		}
		new_map->data.mux.function = fun_desc->name;
		break;
	case PINCTRL_ACPI_PIN_CONFIG:
		map_config = &new_map->data.configs;
		new_map->type = PIN_MAP_TYPE_CONFIGS_PIN;
		map_config->group_or_pin = pin_get_name(pctldev,
							info->config.pin);
		map_config->configs = devm_kcalloc(pctldev->dev,
						   info->config.nconfigs,
						   sizeof(unsigned long),
						   GFP_KERNEL);
		map_config->num_configs = 0;
		list_for_each_entry(config_node, info->config.configs, node)
			map_config->configs[map_config->num_configs++] =
				config_node->config;
		break;
	default:
		dev_warn(pctldev->dev, "Not support for type %d\n", info->type);
		devm_kfree(pctldev->dev, new_map);
		return -EINVAL;
	}

	*map = new_map;
	*num_maps_out = num_maps;

	return 0;
}

static void sky1_dt_free_map(struct pinctrl_dev *pctldev,
			     struct pinctrl_map *map,
			     unsigned num_maps)
{
	kfree(map);
}

static void sky1_acpi_free_map(struct pinctrl_dev *pctldev,
			       struct pinctrl_map *map,
			       unsigned num_maps)
{
	kfree(map);
}

static const struct pinctrl_ops sky1_pctrl_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.pin_dbg_show = sky1_pin_dbg_show,
	.dt_node_to_map = sky1_dt_node_to_map,
	.acpi_node_to_map = sky1_acpi_node_to_map,
	.dt_free_map = sky1_dt_free_map,
	.acpi_free_map = sky1_acpi_free_map,
};

static int sky1_pmx_set_one_pin(struct sky1_pinctrl *spctl,
				    struct sky1_pin *pin)
{
	u32 reg_val;
	u32 *pin_reg;

	pin_reg = spctl->base + pin->offset;
	if (!has_acpi_companion(spctl->dev)) {
		reg_val = readl(pin_reg);
		reg_val &= ~SKY1_MUX_MASK;
		reg_val |= pin->configs & SKY1_MUX_MASK;
	} else {
		reg_val = pin->configs & (SKY1_MUX_MASK|SKY1_CONF_MASK);
	}
	writel(reg_val, pin_reg);

	dev_dbg(spctl->dev, "write: offset 0x%x val 0x%x\n",
		pin->offset, reg_val);
	return 0;
}

static int sky1_pmx_set(struct pinctrl_dev *pctldev, unsigned selector,
		       unsigned group)
{
	struct sky1_pinctrl *spctl = pinctrl_dev_get_drvdata(pctldev);
	struct function_desc *func;
	struct group_desc *grp;
	struct sky1_pin *pin;
	unsigned int npins;
	int i, err;

	/*
	 * Configure the mux mode for each pin in the group for a specific
	 * function.
	 */
	grp = pinctrl_generic_get_group(pctldev, group);
	if (!grp)
		return -EINVAL;

	func = pinmux_generic_get_function(pctldev, selector);
	if (!func)
		return -EINVAL;

	npins = grp->num_pins;

	dev_dbg(spctl->dev, "enable function %s group %s\n",
		func->name, grp->name);

	for (i = 0; i < npins; i++) {
		/*
		 * Config for Sky1 one pin
		 */
		pin = &((struct sky1_pin *)(grp->data))[i];
		err = sky1_pmx_set_one_pin(spctl, pin);
		if (err)
			return err;
	}

	return 0;
}

struct pinmux_ops sky1_pmx_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = sky1_pmx_set,
};

static int sky1_pinconf_get(struct pinctrl_dev *pctldev,
			   unsigned pin_id, unsigned long *config)
{
	struct sky1_pinctrl *spctl = pinctrl_dev_get_drvdata(pctldev);
	*config = readl(spctl->base + spctl->pin_regs[pin_id]);

	return 0;
}

static int sky1_pinconf_set(struct pinctrl_dev *pctldev,
			   unsigned pin_id, unsigned long *configs,
			   unsigned num_configs)
{
	struct sky1_pinctrl *spctl = pinctrl_dev_get_drvdata(pctldev);
	u32 reg_val;
	u32 *pin_reg;
	int i;

	pin_reg = spctl->base + spctl->pin_regs[pin_id];

	for(i = 0; i < num_configs; i++) {
		reg_val = readl(pin_reg);
		reg_val &= ~SKY1_CONF_MASK;
		reg_val |= (configs[i] & SKY1_CONF_MASK);
		writel(reg_val, pin_reg);

		dev_dbg(spctl->dev, "write: offset 0x%x val 0x%x\n",
			spctl->pin_regs[pin_id], reg_val);
	}

	return 0;
}

static void sky1_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				   struct seq_file *s, unsigned pin_id)
{
	struct sky1_pinctrl *spctl = pinctrl_dev_get_drvdata(pctldev);
	u32 config;
	u32 *pin_reg;

	if (spctl->pin_regs[pin_id] == -1) {
		seq_puts(s, "N/A");
		return;
	}

	pin_reg = spctl->base + spctl->pin_regs[pin_id];
	config = readl(pin_reg) & SKY1_CONF_MASK;

	seq_printf(s, "0x%x", config);
}

static void sky1_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
					 struct seq_file *s, unsigned group)
{
	struct group_desc *grp;
	unsigned long config;
	const char *name;
	int i, pin_id, ret;

	if (group >= pctldev->num_groups)
		return;

	seq_puts(s, "\n");
	grp = pinctrl_generic_get_group(pctldev, group);
	if (!grp)
		return;

	for (i = 0; i < grp->num_pins; i++) {
		struct sky1_pin *pin = &(((struct sky1_pin *)(grp->data))[i]);
		pin_id = pin->offset / 4;

		name = pin_get_name(pctldev, pin_id);
		ret = sky1_pinconf_get(pctldev, pin_id, &config);
		if (ret)
			return;
		seq_printf(s, "  %s: 0x%lx\n", name, config);
	}
}

static const struct pinconf_ops sky1_pinconf_ops = {
	.pin_config_get = sky1_pinconf_get,
	.pin_config_set = sky1_pinconf_set,
	.pin_config_dbg_show = sky1_pinconf_dbg_show,
	.pin_config_group_dbg_show = sky1_pinconf_group_dbg_show,
};

/*
 * Each pin represented in sky1,pins consists of
 * a number of u32 OFFSET and a number of u32 CONFIGS,
 * the total size is OFFSET + CONFIGS for each pin.
 *
 * Default:
 *     <offset, configs>
 *     <4byte,  4byte>
 */

static void sky1_pinctrl_parse_pin(struct sky1_pinctrl *spctl,
				       unsigned int *pin_id,
				       struct sky1_pin *pin,
				       const __be32 **list_p,
				       struct device_node *np)
{
	const __be32 *list = *list_p;
	unsigned int configs0, configs1;
	const struct sky1_pinctrl_soc_info *info = spctl->info;

	pin->offset = be32_to_cpu(*list++);
	*pin_id = pin->offset / 4;
	pin->pin_id = *pin_id;

	configs0 = be32_to_cpu(*list++);
	configs1 = be32_to_cpu(*list++);
	pin->configs =  (((configs0 & PADS_FUNCS_MASK) << PADS_FUNCS_BITS) | (configs1 & PADS_CONFS_MASK));
	spctl->pin_regs[*pin_id] = pin->offset;

	*list_p = list;

	dev_dbg(spctl->dev, "%s: 0x%x 0x%08lx", info->pins[*pin_id].name,
		pin->offset, pin->configs);
}

static int sky1_pinctrl_parse_groups(struct device_node *np,
				    struct group_desc *grp,
				    struct sky1_pinctrl *spctl,
				    u32 index)
{
	struct sky1_pin *pin;
	int size;
	const __be32 *list;
	int i;

	dev_dbg(spctl->dev, "group(%d): %pOFn\n", index, np);

	/* Initialise group */
	grp->name = np->name;

	/*
	 * the binding format is sky1,pins = <PIN_FUNC_ID CONFIG ...>,
	 * do sanity check and calculate pins number
	 *
	 * First try legacy 'sky1,pins' property, then fall back to the
	 * generic 'pinmux'.
	 *
	 * Note: for generic 'pinmux' case, there's no CONFIG part in
	 * the binding format.
	 */
	list = of_get_property(np, "sky1,pins", &size);
	if (!list) {
		list = of_get_property(np, "pinmux", &size);
		if (!list) {
			dev_err(spctl->dev,
				"no sky1,pins and pins property in node %pOF\n", np);
			return -EINVAL;
		}
	}

	/* we do not check return since it's safe node passed down */
	if (!size || size % SKY1_PIN_SIZE) {
		dev_err(spctl->dev, "Invalid sky1,pins or pins property in node %pOF\n", np);
		return -EINVAL;
	}

	grp->num_pins = size / SKY1_PIN_SIZE;
	grp->data = devm_kcalloc(spctl->dev,
				 grp->num_pins, sizeof(struct sky1_pin),
				 GFP_KERNEL);
	grp->pins = devm_kcalloc(spctl->dev,
				 grp->num_pins, sizeof(unsigned int),
				 GFP_KERNEL);
	if (!grp->pins || !grp->data)
		return -ENOMEM;

	for (i = 0; i < grp->num_pins; i++) {
		pin = &((struct sky1_pin *)(grp->data))[i];
		sky1_pinctrl_parse_pin(spctl, &grp->pins[i],
				           pin, &list, np);
	}

	return 0;
}

static int sky1_pinctrl_parse_functions(struct device_node *np,
				       struct sky1_pinctrl *spctl,
				       u32 index)
{
	struct pinctrl_dev *pctl = spctl->pctl;
	struct device_node *child;
	struct function_desc *func;
	struct group_desc *grp;
	const char **group_names;
	u32 i = 0;

	dev_dbg(pctl->dev, "parse function(%d): %pOFn\n", index, np);

	func = pinmux_generic_get_function(pctl, index);
	if (!func)
		return -EINVAL;

	/* Initialise function */
	func->name = np->name;
	func->num_group_names = of_get_child_count(np);
	if (func->num_group_names == 0) {
		dev_err(spctl->dev, "no groups defined in %pOF\n", np);
		return -EINVAL;
	}
	group_names = devm_kcalloc(spctl->dev, func->num_group_names,
					 sizeof(char *), GFP_KERNEL);
	if (!group_names)
		return -ENOMEM;

	for_each_child_of_node(np, child)
		group_names[i++] = child->name;
	func->group_names = group_names;

	i = 0;
	for_each_child_of_node(np, child) {
		grp = devm_kzalloc(spctl->dev, sizeof(struct group_desc),
				   GFP_KERNEL);
		if (!grp) {
			of_node_put(child);
			return -ENOMEM;
		}

		mutex_lock(&spctl->mutex);
		radix_tree_insert(&pctl->pin_group_tree,
				  spctl->group_index++, grp);
		mutex_unlock(&spctl->mutex);

		sky1_pinctrl_parse_groups(child, grp, spctl, i++);
	}

	return 0;
}

/*
 * Check if the DT contains pins in the direct child nodes. This indicates the
 * newer DT format to store pins. This function returns true if the first found
 * sky1,pins property is in a child of np. Otherwise false is returned.
 */
static bool sky1_pinctrl_dt_is_flat_functions(struct device_node *np)
{
	struct device_node *function_np;
	struct device_node *pinctrl_np;

	for_each_child_of_node(np, function_np) {
		if (of_property_read_bool(function_np, "sky1,pins")) {
			of_node_put(function_np);
			return true;
		}

		for_each_child_of_node(function_np, pinctrl_np) {
			if (of_property_read_bool(pinctrl_np, "sky1,pins")) {
				of_node_put(pinctrl_np);
				of_node_put(function_np);
				return false;
			}
		}
	}

	return true;
}

static int sky1_pinctrl_probe_dt(struct platform_device *pdev,
				struct sky1_pinctrl *spctl)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	struct pinctrl_dev *pctl = spctl->pctl;
	u32 nfuncs = 0;
	u32 i = 0;
	bool flat_funcs;

	if (!np)
		return -ENODEV;

	flat_funcs = sky1_pinctrl_dt_is_flat_functions(np);
	if (flat_funcs) {
		nfuncs = 1;
	} else {
		nfuncs = of_get_child_count(np);
		if (nfuncs == 0) {
			dev_err(&pdev->dev, "no functions defined\n");
			return -EINVAL;
		}
	}

	for (i = 0; i < nfuncs; i++) {
		struct function_desc *function;

		function = devm_kzalloc(&pdev->dev, sizeof(*function),
					GFP_KERNEL);
		if (!function)
			return -ENOMEM;

		mutex_lock(&spctl->mutex);
		radix_tree_insert(&pctl->pin_function_tree, i, function);
		mutex_unlock(&spctl->mutex);
	}
	pctl->num_functions = nfuncs;

	spctl->group_index = 0;
	if (flat_funcs) {
		pctl->num_groups = of_get_child_count(np);
	} else {
		pctl->num_groups = 0;
		for_each_child_of_node(np, child)
			pctl->num_groups += of_get_child_count(child);
	}

	if (flat_funcs) {
		sky1_pinctrl_parse_functions(np, spctl, 0);
	} else {
		i = 0;
		for_each_child_of_node(np, child)
			sky1_pinctrl_parse_functions(child, spctl, i++);
	}

	return 0;
}

static void
sky1_free_acpi_group_desc(struct pinctrl_acpi_group_desc *acpi_grp_desc)
{
	if (!acpi_grp_desc)
		return;

	if (acpi_grp_desc->vendor_data)
		kfree(acpi_grp_desc->vendor_data);

	if (acpi_grp_desc->num_pins)
		kfree(acpi_grp_desc->pins);

	if (acpi_grp_desc->name)
		kfree(acpi_grp_desc->name);

	kfree(acpi_grp_desc);
}

static int
sky1_pinctrl_parse_groups_acpi(struct pinctrl_acpi_group_desc *acpi_grp,
			       struct sky1_pinctrl *spctl,
			       struct group_desc **group)
{
	struct group_desc *grp;
	__be16 *vendor;
	struct sky1_pin *pin;
	struct pinctrl_dev *pctl = spctl->pctl;
	int i;

	grp = devm_kzalloc(spctl->dev,
			   sizeof(struct group_desc),
			   GFP_KERNEL);
	if (!grp)
		return -ENOMEM;

	grp->name = devm_kstrdup(spctl->dev, acpi_grp->name, GFP_KERNEL);
	grp->num_pins = acpi_grp->num_pins;
	grp->data = devm_kcalloc(spctl->dev,grp->num_pins,
				 sizeof(struct sky1_pin),
				 GFP_KERNEL);
	grp->pins = devm_kcalloc(spctl->dev, grp->num_pins,
				 sizeof(unsigned int),
				 GFP_KERNEL);
	if (!grp->pins || !grp->data)
		return -ENOMEM;

	vendor = (__be16 *)acpi_grp->vendor_data;
	for (i = 0; i < grp->num_pins; i++) {
		pin = &((struct sky1_pin *)(grp->data))[i];
		/* decode vendor data to pid mux_reg offset, config_val */
		pin->pin_id = acpi_grp->pins[i];
		pin->offset = be16_to_cpu(*vendor++);
		pin->configs = be16_to_cpu(*vendor++);

		spctl->pin_regs[pin->pin_id] = pin->offset;
		if (pin->offset/4 != pin->pin_id)
			dev_warn(spctl->dev, "pin and reg unmatched!");
		grp->pins[i] = pin->pin_id;

		dev_dbg(spctl->dev, "%s: 0x%x 0x%08lx\n",
			spctl->info->pins[pin->pin_id].name,
			pin->offset, pin->configs);
	}
	*group = grp;
	mutex_lock(&spctl->mutex);
	radix_tree_insert(&pctl->pin_group_tree,
			  spctl->group_index++,
			  grp);
	mutex_unlock(&spctl->mutex);

	return 0;
}

static int sky1_pinctrl_probe_acpi(struct platform_device *pdev,
				  struct sky1_pinctrl *spctl)
{
	struct pinctrl_acpi_group_desc *acpi_grp, *temp;
	struct list_head pinctrl_group_list;
	struct acpi_device *acpi_dev = ACPI_COMPANION(&pdev->dev);
	struct function_desc *function;
	int ret;
	struct pinctrl_dev *pctl = spctl->pctl;
	int num_groups = 0;
	const char **group_names;
	struct group_desc *group;
	int index = 0;

	/* Get list of pin group definitions for the device */
	INIT_LIST_HEAD(&pinctrl_group_list);
	ret = pinctrl_acpi_get_pin_groups(acpi_dev, &pinctrl_group_list);

	/* Iterate over the list to find number of groups */
	list_for_each_entry(acpi_grp, &pinctrl_group_list, list)
		num_groups++;

	/* Create function node and add to pinctrl */
	function = devm_kzalloc(&pdev->dev, sizeof(*function), GFP_KERNEL);
	if (!function)
		return -ENOMEM;

	mutex_lock(&spctl->mutex);
	radix_tree_insert(&pctl->pin_function_tree, 0, function);
	mutex_unlock(&spctl->mutex);

	group_names = devm_kzalloc(spctl->dev,
				   num_groups * sizeof(char *),
				   GFP_KERNEL);
	if (!group_names)
		return -ENOMEM;

	function->group_names = group_names;
	function->num_group_names = num_groups;
	function->name = dev_name(&pdev->dev);

	pctl->num_groups = num_groups;
	pctl->num_functions = 1;
	spctl->group_index = 0;

	list_for_each_entry_safe(acpi_grp, temp, &pinctrl_group_list, list) {
		ret = sky1_pinctrl_parse_groups_acpi(acpi_grp, spctl, &group);
		if (!ret)
			group_names[index++] = group->name;

		list_del(&acpi_grp->list);
		sky1_free_acpi_group_desc(acpi_grp);
	}

	return 0;
}

int sky1_base_pinctrl_probe(struct platform_device *pdev,
		      const struct sky1_pinctrl_soc_info *info)
{
	struct pinctrl_desc *sky1_pinctrl_desc;
	struct sky1_pinctrl *spctl;
	int ret, i;

	if (!info || !info->pins || !info->npins) {
		dev_err(&pdev->dev, "wrong pinctrl info\n");
		return -EINVAL;
	}

	/* Create state holders etc for this driver */
	spctl = devm_kzalloc(&pdev->dev, sizeof(*spctl), GFP_KERNEL);
	if (!spctl)
		return -ENOMEM;

	spctl->pin_regs = devm_kmalloc_array(&pdev->dev, info->npins,
					    sizeof(*spctl->pin_regs),
					    GFP_KERNEL);
	if(!spctl->pin_regs)
		return -ENOMEM;

	for (i = 0; i < info->npins; i++ ) {
		spctl->pin_regs[i]  = -1;
	}

	spctl->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(spctl->base))
		return PTR_ERR(spctl->base);

	sky1_pinctrl_desc = devm_kzalloc(&pdev->dev, sizeof(*sky1_pinctrl_desc),
					GFP_KERNEL);
	if (!sky1_pinctrl_desc)
		return -ENOMEM;

	sky1_pinctrl_desc->name = dev_name(&pdev->dev);
	sky1_pinctrl_desc->pins = info->pins;
	sky1_pinctrl_desc->npins = info->npins;
	sky1_pinctrl_desc->pctlops = &sky1_pctrl_ops;
	sky1_pinctrl_desc->pmxops = &sky1_pmx_ops;
	sky1_pinctrl_desc->confops = &sky1_pinconf_ops;
	sky1_pinctrl_desc->owner = THIS_MODULE;

	mutex_init(&spctl->mutex);

	spctl->info = info;
	spctl->dev = &pdev->dev;
	platform_set_drvdata(pdev, spctl);
	ret = devm_pinctrl_register_and_init(&pdev->dev,
					     sky1_pinctrl_desc, spctl,
					     &spctl->pctl);
	if (ret) {
		dev_err(&pdev->dev, "could not register SKY1 pinctrl driver\n");
		return ret;
	}

	if (!has_acpi_companion(&pdev->dev))
		ret = sky1_pinctrl_probe_dt(pdev, spctl);
	else
		ret = sky1_pinctrl_probe_acpi(pdev, spctl);

	if (ret) {
		dev_err(&pdev->dev, "fail to probe dt or acpi properties\n");
		return ret;
	}

	pinctrl_provide_dummies();
	dev_info(&pdev->dev, "initialized SKY1 pinctrl driver\n");

	return pinctrl_enable(spctl->pctl);
}
EXPORT_SYMBOL_GPL(sky1_base_pinctrl_probe);


MODULE_AUTHOR("Jerry Zhu <Jerry.Zhu@cixtech.com>");
MODULE_DESCRIPTION("Cix SKy1 common pinctrl driver");
MODULE_LICENSE("GPL v2");

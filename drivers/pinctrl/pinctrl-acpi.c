// SPDX-License-Identifier: GPL-2.0
/*
 * ACPI helpers for PinCtrl API
 *
 * Copyright (C) 2022 Linaro Ltd.
 * Author: Niyas Sait <niyas.sait@linaro.org>
 */
#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/list.h>

#include "pinctrl-acpi.h"
#include "core.h"

/**
 * struct pin_config_lookup_info - context to use for pin config look up
 * @pctrl: pinctrl descriptor
 * @config_maps: list of &struct config_map_info
 */
struct pin_config_lookup_info {
	struct pinctrl *pctrl;
	struct list_head config_maps;
};

/**
 * struct config_map_info - context for config map
 * @pin: pin for the configs
 * @pinctrl_acpi: pin controller ACPI name
 * @configs: list of &struct pinctrl_acpi_config_node
 * @node: list node
 * @nconfigs: number of configs
 */
struct config_map_info {
	char *pinctrl_acpi;
	unsigned int pin;
	struct list_head configs;
	struct list_head node;
	size_t nconfigs;
};

/**
 * struct pinctrl_acpi_map - mapping table chunk parsed from ACPI
 * @node: list node for struct pinctrl's @acpi_maps field
 * @pctldev: the pin controller that allocated this struct, and will free it
 * @map: the mapping table entries
 * @num_maps: number of mapping table entries
 */
struct pinctrl_acpi_map {
	struct list_head node;
	struct pinctrl_dev *pctldev;
	struct pinctrl_map *map;
	size_t num_maps;
};

/**
 * struct pin_groups_lookup_info - context to use for pin group look up
 * @group: group to use for look up
 * @pins: populated pin array for the group
 * @npins: number of pins found for the group
 */
struct pin_groups_lookup_info {
	const char *group;
	unsigned int *pins;
	size_t npins;
};

static void acpi_free_map(struct pinctrl_dev *pctldev, struct pinctrl_map *map,
			  unsigned int num_maps)
{
	int i;

	for (i = 0; i < num_maps; ++i) {
		kfree_const(map[i].dev_name);
		map[i].dev_name = NULL;
	}

	if (pctldev) {
		const struct pinctrl_ops *ops = pctldev->desc->pctlops;

		if (ops->acpi_free_map)
			ops->acpi_free_map(pctldev, map, num_maps);
	} else {
		kfree(map);
	}
}

/**
 * pinctrl_acpi_free_maps() - free pinctrl ACPI maps
 * @p: pinctrl descriptor for the device
 */
void pinctrl_acpi_free_maps(struct pinctrl *p)
{
	struct pinctrl_acpi_map *acpi_map, *tmp;

	list_for_each_entry_safe(acpi_map, tmp, &p->acpi_maps, node) {
		pinctrl_unregister_mappings(acpi_map->map);
		list_del(&acpi_map->node);
		acpi_free_map(acpi_map->pctldev, acpi_map->map,
			      acpi_map->num_maps);
		kfree(acpi_map);
	}
}

static int acpi_remember_or_free_map(struct pinctrl *p, const char *statename,
				     struct pinctrl_dev *pctldev,
				     struct pinctrl_map *map,
				     unsigned int num_maps)
{
	struct pinctrl_acpi_map *acpi_map;
	int i;

	for (i = 0; i < num_maps; i++) {
		const char *devname;

		devname = kstrdup_const(dev_name(p->dev), GFP_KERNEL);
		if (!devname)
			goto err_free_map;

		map[i].dev_name = devname;
		map[i].name = statename;
		if (pctldev)
			map[i].ctrl_dev_name = dev_name(pctldev->dev);
	}

	acpi_map = kzalloc(sizeof(*acpi_map), GFP_KERNEL);
	if (!acpi_map)
		goto err_free_map;

	acpi_map->pctldev = pctldev;
	acpi_map->map = map;
	acpi_map->num_maps = num_maps;
	list_add_tail(&acpi_map->node, &p->acpi_maps);

	return pinctrl_register_mappings(map, num_maps);

err_free_map:
	acpi_free_map(pctldev, map, num_maps);
	return -ENOMEM;
}

static struct pinctrl_dev *get_pinctrl_dev_from_acpi_name(char *pinctrl_acpi)
{
	struct acpi_device *adev;
	const char *dev_name;
	acpi_status status;
	acpi_handle handle;

	status = acpi_get_handle(NULL, pinctrl_acpi, &handle);
	if (ACPI_FAILURE(status))
		return NULL;

	adev = acpi_get_acpi_dev(handle);
	if (!adev)
		return NULL;

	dev_name = acpi_dev_name(adev);
	if (!dev_name)
		return NULL;

	return get_pinctrl_dev_from_devname(dev_name);
}

static int acpi_to_generic_pin_config(unsigned int acpi_param,
				      unsigned int acpi_value,
				      unsigned int *pin_config)
{
	enum pin_config_param genconf_param;

	switch (acpi_param) {
	case ACPI_PIN_CONFIG_BIAS_PULL_UP:
		genconf_param = PIN_CONFIG_BIAS_PULL_UP;
		break;
	case ACPI_PIN_CONFIG_BIAS_PULL_DOWN:
		genconf_param = PIN_CONFIG_BIAS_PULL_DOWN;
		break;
	case ACPI_PIN_CONFIG_BIAS_DEFAULT:
		genconf_param = PIN_CONFIG_BIAS_PULL_PIN_DEFAULT;
		break;
	case ACPI_PIN_CONFIG_BIAS_DISABLE:
		genconf_param = PIN_CONFIG_BIAS_DISABLE;
		break;
	case ACPI_PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
		genconf_param = PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
		break;
	case ACPI_PIN_CONFIG_BIAS_BUS_HOLD:
		genconf_param = PIN_CONFIG_BIAS_BUS_HOLD;
		break;
	case ACPI_PIN_CONFIG_DRIVE_OPEN_DRAIN:
		genconf_param = PIN_CONFIG_DRIVE_OPEN_DRAIN;
		break;
	case ACPI_PIN_CONFIG_DRIVE_OPEN_SOURCE:
		genconf_param = PIN_CONFIG_DRIVE_OPEN_SOURCE;
		break;
	case ACPI_PIN_CONFIG_DRIVE_PUSH_PULL:
		genconf_param = PIN_CONFIG_DRIVE_PUSH_PULL;
		break;
	case ACPI_PIN_CONFIG_DRIVE_STRENGTH:
		genconf_param = PIN_CONFIG_DRIVE_STRENGTH;
		break;
	case ACPI_PIN_CONFIG_SLEW_RATE:
		genconf_param = PIN_CONFIG_SLEW_RATE;
		break;
	case ACPI_PIN_CONFIG_INPUT_DEBOUNCE:
		genconf_param = PIN_CONFIG_INPUT_DEBOUNCE;
		break;
	case ACPI_PIN_CONFIG_INPUT_SCHMITT_TRIGGER:
		genconf_param = PIN_CONFIG_INPUT_SCHMITT_ENABLE;
		break;
	default:
		return -EINVAL;
	}

	*pin_config = pinconf_to_config_packed(genconf_param, acpi_value);

	return 0;
}

static int add_to_config_map(struct list_head *config_maps, char *pinctrl_acpi,
			     unsigned int pin, unsigned long config)
{
	struct config_map_info *config_map;
	struct pinctrl_acpi_config_node *config_node;

	config_node =
		kzalloc(sizeof(struct pinctrl_acpi_config_node), GFP_KERNEL);
	if (!config_node)
		return -ENOMEM;

	config_node->config = config;
	INIT_LIST_HEAD(&config_node->node);

	list_for_each_entry(config_map, config_maps, node) {
		if (strcmp(config_map->pinctrl_acpi, pinctrl_acpi) == 0 &&
		    config_map->pin == pin) {
			list_add(&config_node->node, &config_map->configs);
			config_map->nconfigs++;
			return 0;
		}
	}

	config_map = kzalloc(sizeof(struct config_map_info), GFP_KERNEL);
	if (!config_map)
		goto err_free_config_node;

	config_map->pin = pin;
	config_map->pinctrl_acpi = pinctrl_acpi;
	config_map->nconfigs = 1;
	INIT_LIST_HEAD(&config_map->node);
	INIT_LIST_HEAD(&config_map->configs);
	list_add(&config_node->node, &config_map->configs);
	list_add(&config_map->node, config_maps);

	return 0;

err_free_config_node:
	kfree(config_node);
	return -ENOMEM;
}

static int
acpi_pin_resource_to_pinctrl_map(struct pinctrl *p, char *pinctrl_acpi,
				 struct pinctrl_acpi_resource *resource)
{
	const struct pinctrl_ops *ops;
	struct pinctrl_map *new_map;
	struct pinctrl_dev *pctldev;
	unsigned int num_maps;
	int ret;

	pctldev = get_pinctrl_dev_from_acpi_name(pinctrl_acpi);
	if (!pctldev) {
		dev_err(p->dev, "pctldev with ACPI name '%s' not found\n",
			pinctrl_acpi);
		return -ENXIO;
	}

	ops = pctldev->desc->pctlops;
	if (!ops->acpi_node_to_map) {
		dev_err(p->dev, "pctldev %s doesn't support ACPI\n",
			dev_name(pctldev->dev));
		return -ENXIO;
	}

	ret = ops->acpi_node_to_map(pctldev, resource, &new_map, &num_maps);
	if (ret < 0)
		return ret;

	ret = acpi_remember_or_free_map(p, "default", pctldev, new_map,
					num_maps);

	return ret;
}

static int acpi_pin_function_to_pinctrl_map(struct pinctrl *p,
					    char *pinctrl_acpi,
					    unsigned int *pins, size_t npins,
					    int function)
{
	struct pinctrl_acpi_resource resource = {
		.type = PINCTRL_ACPI_PIN_FUNCTION,
		.function.pins = pins,
		.function.npins = npins,
		.function.function_number = function
	};

	return acpi_pin_resource_to_pinctrl_map(p, pinctrl_acpi, &resource);
}

static int acpi_pin_config_to_pinctrl_map(struct pinctrl *p, char *pinctrl_acpi,
					  unsigned int pin,
					  struct list_head *configs,
					  size_t nconfigs)
{
	struct pinctrl_acpi_resource resource = {
		.type = PINCTRL_ACPI_PIN_CONFIG,
		.config.pin = pin,
		.config.configs = configs,
		.config.nconfigs = nconfigs
	};

	return acpi_pin_resource_to_pinctrl_map(p, pinctrl_acpi, &resource);
}

static int
process_pin_config_from_pin_function(struct pinctrl *p,
				     struct acpi_resource_pin_function *ares)
{
	struct pinctrl_acpi_config_node config_node;
	int i, ret;

	INIT_LIST_HEAD(&config_node.node);

	switch (ares->pin_config) {
	case ACPI_PIN_CONFIG_PULLUP:
		config_node.config = PIN_CONFIG_BIAS_PULL_UP;
		break;
	case ACPI_PIN_CONFIG_PULLDOWN:
		config_node.config = PIN_CONFIG_BIAS_PULL_DOWN;
		break;
	default:
		config_node.config = PIN_CONFIG_BIAS_PULL_PIN_DEFAULT;
		break;
	}

	for (i = 0; i < ares->pin_table_length; i++) {
		ret = acpi_pin_config_to_pinctrl_map(
			p, ares->resource_source.string_ptr, ares->pin_table[i],
			&config_node.node, 1);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int process_pin_function(struct pinctrl *p,
				struct acpi_resource_pin_function *ares)
{
	unsigned int function;
	char *pinctrl_acpi;
	unsigned int *pins;
	size_t npins;
	int i, ret;

	function = ares->function_number;
	pinctrl_acpi = ares->resource_source.string_ptr;
	npins = ares->pin_table_length;
	pins = kcalloc(npins, sizeof(*pins), GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0; i < npins; i++)
		pins[i] = ares->pin_table[i];

	ret = acpi_pin_function_to_pinctrl_map(p, pinctrl_acpi, pins, npins,
					       function);
	if (ret >= 0)
		ret = process_pin_config_from_pin_function(p, ares);

	kfree(pins);

	return ret;
}

static int process_pin_config(struct list_head *config_maps,
			      struct acpi_resource_pin_config *ares)
{
	unsigned int config;
	int i, ret;

	ret = acpi_to_generic_pin_config(ares->pin_config_type,
					 ares->pin_config_value, &config);
	if (ret < 0)
		return ret;

	for (i = 0; i < ares->pin_table_length; i++) {
		ret = add_to_config_map(config_maps,
					ares->resource_source.string_ptr,
					ares->pin_table[i], config);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int find_pin_group_cb(struct acpi_resource *ares, void *data)
{
	struct acpi_resource_pin_group *ares_pin_group;
	struct pin_groups_lookup_info *info = data;
	int i;

	ares_pin_group = &ares->data.pin_group;

	if (ares->type != ACPI_RESOURCE_TYPE_PIN_GROUP)
		return 1;

	if (strcmp(ares_pin_group->resource_label.string_ptr, info->group))
		return 1;

	info->npins = ares_pin_group->pin_table_length;
	info->pins = kcalloc(info->npins, sizeof(*info->pins), GFP_KERNEL);
	if (!info->pins)
		return -ENOMEM;

	for (i = 0; i < ares_pin_group->pin_table_length; i++)
		info->pins[i] = ares_pin_group->pin_table[i];

	return 1;
}

static int get_pins_in_acpi_pin_group(struct acpi_device *adev,
				      char *group_name, unsigned int **pins,
				      size_t *npins)
{
	struct pin_groups_lookup_info info = { .group = group_name };
	struct list_head res_list;
	int ret;

	INIT_LIST_HEAD(&res_list);
	ret = acpi_dev_get_resources(adev, &res_list, find_pin_group_cb, &info);
	if (ret < 0)
		return ret;

	acpi_dev_free_resource_list(&res_list);

	*pins = info.pins;
	*npins = info.npins;

	return 0;
}

static int process_pin_group_config(struct pinctrl *p,
				    struct list_head *config_maps,
				    struct acpi_resource_pin_group_config *ares)
{
	struct acpi_device *adev;
	struct pinctrl_dev *pctl_dev;
	unsigned int config;
	int ret;
	size_t npins;
	unsigned int *pins;
	char *pinctrl_acpi;
	char *group;

	ret = acpi_to_generic_pin_config(ares->pin_config_type,
					 ares->pin_config_value, &config);
	if (ret < 0)
		return ret;

	pinctrl_acpi = ares->resource_source.string_ptr;
	pctl_dev = get_pinctrl_dev_from_acpi_name(pinctrl_acpi);
	if (!pctl_dev) {
		dev_err(p->dev, "pctldev with ACPI name '%s' not found\n",
			pinctrl_acpi);
		return -ENXIO;
	}

	adev = ACPI_COMPANION(pctl_dev->dev);
	group = ares->resource_source_label.string_ptr;
	ret = get_pins_in_acpi_pin_group(adev, group, &pins, &npins);
	if (ret < 0)
		return ret;

	for (int i = 0; i < npins; i++) {
		ret = add_to_config_map(config_maps, pinctrl_acpi, pins[i],
					config);
		if (ret < 0)
			break;
	}

	kfree(pins);

	return ret;
}

static int
process_pin_group_function(struct pinctrl *p,
			   struct acpi_resource_pin_group_function *ares)
{
	struct pinctrl_dev *pctl_dev;
	struct acpi_device *adev;
	unsigned int *pins;
	char *pinctrl_acpi;
	char *group;
	size_t npins;
	int ret;

	pinctrl_acpi = ares->resource_source.string_ptr;
	pctl_dev = get_pinctrl_dev_from_acpi_name(pinctrl_acpi);
	if (!pctl_dev) {
		dev_err(p->dev, "pctldev with ACPI name '%s' not found\n",
			pinctrl_acpi);
		return -ENXIO;
	}

	adev = ACPI_COMPANION(pctl_dev->dev);
	group = ares->resource_source_label.string_ptr;
	ret = get_pins_in_acpi_pin_group(adev, group, &pins, &npins);
	if (ret < 0)
		return ret;

	ret = acpi_pin_function_to_pinctrl_map(p, pinctrl_acpi, pins, npins,
					       ares->function_number);

	kfree(pins);

	return ret;
}

static int parse_acpi_pin_function_resources(struct acpi_resource *ares,
					     void *data)
{
	struct pinctrl *p = data;
	int ret;

	dev_dbg(p->dev, "type = %d\n", ares->type);
	switch (ares->type) {
	case ACPI_RESOURCE_TYPE_PIN_FUNCTION:
		ret = process_pin_function(p, &ares->data.pin_function);
		if (ret < 0)
			return ret;
		break;
	case ACPI_RESOURCE_TYPE_PIN_GROUP_FUNCTION:
		ret = process_pin_group_function(
			p, &ares->data.pin_group_function);
		if (ret < 0)
			return ret;
		break;
	}

	return 1;
}

static int parse_acpi_pin_config_resources(struct acpi_resource *ares,
					   void *data)
{
	struct pin_config_lookup_info *info = data;
	int ret;

	switch (ares->type) {
	case ACPI_RESOURCE_TYPE_PIN_CONFIG:
		ret = process_pin_config(&info->config_maps,
					 &ares->data.pin_config);
		if (ret < 0)
			return ret;
		break;
	case ACPI_RESOURCE_TYPE_PIN_GROUP_CONFIG:
		ret = process_pin_group_config(info->pctrl, &info->config_maps,
					       &ares->data.pin_group_config);
		if (ret < 0)
			return ret;
		break;
	}

	return 1;
}

static int parse_acpi_pin_functions(struct pinctrl *p)
{
	struct list_head res_list;
	struct acpi_device *adev;
	int ret;

	INIT_LIST_HEAD(&res_list);
	adev = ACPI_COMPANION(p->dev);
	ret = acpi_dev_get_resources(adev, &res_list,
				     parse_acpi_pin_function_resources, p);
	if (ret < 0)
		return ret;

	acpi_dev_free_resource_list(&res_list);

	return 0;
}

static void free_config_nodes(struct list_head *config_nodes)
{
	struct pinctrl_acpi_config_node *config_node, *tmp;

	list_for_each_entry_safe(config_node, tmp, config_nodes, node) {
		list_del(&config_node->node);
		kfree(config_node);
	}
}

static void free_config_maps(struct list_head *config_maps)
{
	struct config_map_info *config_map, *tmp;

	list_for_each_entry_safe(config_map, tmp, config_maps, node) {
		list_del(&config_map->node);
		free_config_nodes(&config_map->configs);
		kfree(config_map);
	}
}

static int parse_acpi_pin_configs(struct pinctrl *p)
{
	struct pin_config_lookup_info info = { .pctrl = p };
	struct config_map_info *config_map;
	struct list_head res_list;
	int ret;

	INIT_LIST_HEAD(&res_list);
	INIT_LIST_HEAD(&info.config_maps);
	ret = acpi_dev_get_resources(ACPI_COMPANION(p->dev), &res_list,
				     parse_acpi_pin_config_resources, &info);
	if (ret < 0)
		return ret;

	list_for_each_entry(config_map, &info.config_maps, node) {
		ret = acpi_pin_config_to_pinctrl_map(
			p, config_map->pinctrl_acpi, config_map->pin,
			&config_map->configs, config_map->nconfigs);
		if (ret < 0)
			break;
	}

	free_config_maps(&info.config_maps);
	acpi_dev_free_resource_list(&res_list);

	return ret;
}

/**
 * pinctrl_acpi_to_map() - pinctrl map from ACPI pin resources for given device
 * @p: pinctrl descriptor for the device
 *
 * This will parse the ACPI pin resources for the device and creates pinctrl map.
 *
 * Return: Returns 0 on success, negative errno on failure.
 */
int pinctrl_acpi_to_map(struct pinctrl *p)
{
	int ret;

	if (!has_acpi_companion(p->dev))
		return -ENXIO;

	ret = parse_acpi_pin_functions(p);
	if (ret < 0)
		return ret;

	ret = parse_acpi_pin_configs(p);
	if (ret < 0)
		return ret;

	return 0;
}

static int pinctrl_acpi_populate_group_desc(struct acpi_resource *ares,
					    void *data)
{
	struct acpi_resource_pin_group *ares_pin_group;
	struct pinctrl_acpi_group_desc *desc;
	struct list_head *group_desc_list = data;
	unsigned int alloc_size;

	if (ares->type != ACPI_RESOURCE_TYPE_PIN_GROUP)
		return 1;

	ares_pin_group = &ares->data.pin_group;

	desc = kzalloc(sizeof(struct pinctrl_acpi_group_desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	desc->name = kstrdup_const(ares_pin_group->resource_label.string_ptr,
				   GFP_KERNEL);

	desc->num_pins = ares_pin_group->pin_table_length;
	desc->vendor_length = ares_pin_group->vendor_length;
	if (ares_pin_group->vendor_length) {
		alloc_size = ares_pin_group->vendor_length *
			sizeof(ares_pin_group->vendor_data[0]);
		desc->vendor_data = kzalloc(alloc_size, GFP_KERNEL);
		memcpy(desc->vendor_data, ares_pin_group->vendor_data,
		       alloc_size);
	}

	if (ares_pin_group->pin_table_length) {
		alloc_size = ares_pin_group->pin_table_length *
			sizeof(ares_pin_group->pin_table[0]);
		desc->pins = kzalloc(alloc_size, GFP_KERNEL);
		memcpy(desc->pins, ares_pin_group->pin_table, alloc_size);
	}

	INIT_LIST_HEAD(&desc->list);
	list_add(&desc->list, group_desc_list);

	return 1;
}

/* Get list of acpi pin groups definitions for the controller */
int pinctrl_acpi_get_pin_groups(struct acpi_device *adev,
				struct list_head *group_desc_list)
{
	struct list_head res_list;
	int ret;

	INIT_LIST_HEAD(&res_list);
	INIT_LIST_HEAD(group_desc_list);

	ret = acpi_dev_get_resources(adev, &res_list,
				     pinctrl_acpi_populate_group_desc,
				     group_desc_list);
	if (ret < 0)
		return ret;

	acpi_dev_free_resource_list(&res_list);

	return 0;
}
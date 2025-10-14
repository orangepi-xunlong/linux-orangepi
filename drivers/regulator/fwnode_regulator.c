// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Fwnode helpers for regulator framework
 *
 * Copyright (C) 2025 CIX, Inc.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/property.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fwnode_regulator.h>

#include "internal.h"

static const char *const regulator_states[PM_SUSPEND_MAX + 1] = {
	[PM_SUSPEND_STANDBY]	= "regulator-state-standby",
	[PM_SUSPEND_MEM]	= "regulator-state-mem",
	[PM_SUSPEND_MAX]	= "regulator-state-disk",
};

static void fill_limit(int *limit, int val)
{
	if (val)
		if (val == 1)
			*limit = REGULATOR_NOTIF_LIMIT_ENABLE;
		else
			*limit = val;
	else
		*limit = REGULATOR_NOTIF_LIMIT_DISABLE;
}

static void fwnode_get_regulator_prot_limits(struct fwnode_handle *np,
				struct regulation_constraints *constraints)
{
	u32 pval;
	int i, ret;
	static const char *const props[] = {
		"regulator-oc-%s-microamp",
		"regulator-ov-%s-microvolt",
		"regulator-temp-%s-kelvin",
		"regulator-uv-%s-microvolt",
	};
	struct notification_limit *limits[] = {
		&constraints->over_curr_limits,
		&constraints->over_voltage_limits,
		&constraints->temp_limits,
		&constraints->under_voltage_limits,
	};
	bool set[4] = {0};

	/* Protection limits: */
	for (i = 0; i < ARRAY_SIZE(props); i++) {
		char prop[255];
		bool found;
		int j;
		static const char *const lvl[] = {
			"protection", "error", "warn"
		};
		int *l[] = {
			&limits[i]->prot, &limits[i]->err, &limits[i]->warn,
		};

		for (j = 0; j < ARRAY_SIZE(lvl); j++) {
			snprintf(prop, 255, props[i], lvl[j]);
			ret = fwnode_property_read_u32(np, prop, &pval);
			if (!ret) {
				fill_limit(l[j], pval);
				found = true;
			}
			set[i] |= found;
		}
	}
	constraints->over_current_detection = set[0];
	constraints->over_voltage_detection = set[1];
	constraints->over_temp_detection = set[2];
	constraints->under_voltage_detection = set[3];
}

static int fwnode_get_regulation_constraints(struct device *dev,
					struct fwnode_handle *np,
					struct regulator_init_data **init_data,
					const struct regulator_desc *desc)
{
	struct regulation_constraints *constraints = &(*init_data)->constraints;
	struct regulator_state *suspend_state;
	struct fwnode_handle *suspend_np;
	unsigned int mode;
	int ret, i, len;
	int n_phandles;
	u32 pval;

	n_phandles = fwnode_count_reference_with_args(np, "regulator-coupled-with",
						NULL);
	if (IS_ERR(n_phandles))
		n_phandles = 0;

	n_phandles = max(n_phandles, 0);

	fwnode_property_read_string(np, "regulator-name", &constraints->name);

	if (!fwnode_property_read_u32(np, "regulator-min-microvolt", &pval))
		constraints->min_uV = pval;

	if (!fwnode_property_read_u32(np, "regulator-max-microvolt", &pval))
		constraints->max_uV = pval;

	/* Voltage change possible? */
	if (constraints->min_uV != constraints->max_uV)
		constraints->valid_ops_mask |= REGULATOR_CHANGE_VOLTAGE;

	/* Do we have a voltage range, if so try to apply it? */
	if (constraints->min_uV && constraints->max_uV)
		constraints->apply_uV = true;

	if (!fwnode_property_read_u32(np, "regulator-microvolt-offset", &pval))
		constraints->uV_offset = pval;
	if (!fwnode_property_read_u32(np, "regulator-min-microamp", &pval))
		constraints->min_uA = pval;
	if (!fwnode_property_read_u32(np, "regulator-max-microamp", &pval))
		constraints->max_uA = pval;

	if (!fwnode_property_read_u32(np, "regulator-input-current-limit-microamp",
				  &pval))
		constraints->ilim_uA = pval;

	/* Current change possible? */
	if (constraints->min_uA != constraints->max_uA)
		constraints->valid_ops_mask |= REGULATOR_CHANGE_CURRENT;

	constraints->boot_on = fwnode_property_read_bool(np, "regulator-boot-on");
	constraints->always_on = fwnode_property_read_bool(np, "regulator-always-on");
	if (!constraints->always_on) /* status change should be possible. */
		constraints->valid_ops_mask |= REGULATOR_CHANGE_STATUS;

	constraints->pull_down = fwnode_property_read_bool(np, "regulator-pull-down");

	if (fwnode_property_read_bool(np, "regulator-allow-bypass"))
		constraints->valid_ops_mask |= REGULATOR_CHANGE_BYPASS;

	if (fwnode_property_read_bool(np, "regulator-allow-set-load"))
		constraints->valid_ops_mask |= REGULATOR_CHANGE_DRMS;

	ret = fwnode_property_read_u32(np, "regulator-ramp-delay", &pval);
	if (!ret) {
		if (pval)
			constraints->ramp_delay = pval;
		else
			constraints->ramp_disable = true;
	}

	ret = fwnode_property_read_u32(np, "regulator-settling-time-us", &pval);
	if (!ret)
		constraints->settling_time = pval;

	ret = fwnode_property_read_u32(np, "regulator-settling-time-up-us", &pval);
	if (!ret)
		constraints->settling_time_up = pval;
	if (constraints->settling_time_up && constraints->settling_time) {
		pr_warn("%pFWn: ambiguous configuration for settling time, ignoring 'regulator-settling-time-up-us'\n",
			np);
		constraints->settling_time_up = 0;
	}

	ret = fwnode_property_read_u32(np, "regulator-settling-time-down-us",
				   &pval);
	if (!ret)
		constraints->settling_time_down = pval;
	if (constraints->settling_time_down && constraints->settling_time) {
		pr_warn("%pFWn: ambiguous configuration for settling time, ignoring 'regulator-settling-time-down-us'\n",
			np);
		constraints->settling_time_down = 0;
	}

	ret = fwnode_property_read_u32(np, "regulator-enable-ramp-delay", &pval);
	if (!ret)
		constraints->enable_time = pval;

	constraints->soft_start = fwnode_property_read_bool(np,
					"regulator-soft-start");
	ret = fwnode_property_read_u32(np, "regulator-active-discharge", &pval);
	if (!ret) {
		constraints->active_discharge =
				(pval) ? REGULATOR_ACTIVE_DISCHARGE_ENABLE :
					REGULATOR_ACTIVE_DISCHARGE_DISABLE;
	}

	if (!fwnode_property_read_u32(np, "regulator-initial-mode", &pval)) {
		if (desc && desc->of_map_mode) {
			mode = desc->of_map_mode(pval);
			if (mode == REGULATOR_MODE_INVALID)
				pr_err("%pFWn: invalid mode %u\n", np, pval);
			else
				constraints->initial_mode = mode;
		} else {
			pr_warn("%pFWn: mapping for mode %d not defined\n",
				np, pval);
		}
	}

	len = fwnode_property_count_u32(np, "regulator-allowed-modes");

	if (len > 0) {
		if (desc && desc->fwnode_map_mode) {
			void *dbuf = kzalloc(len*4, GFP_KERNEL);
			ret = fwnode_property_read_u32_array(np, "regulator-allowed-modes", dbuf, len);
			if (!ret) {
				for (i = 0; i < len; i++) {
					pval = *((u32 *)dbuf + i);
					mode = desc->fwnode_map_mode(pval);
					if (mode == REGULATOR_MODE_INVALID)
						pr_err("%pFWn: invalid regulator-allowed-modes element %u\n",
							np, pval);
					else
						constraints->valid_modes_mask |= mode;
				}
			}
			kfree(dbuf);
			if (constraints->valid_modes_mask)
				constraints->valid_ops_mask
					|= REGULATOR_CHANGE_MODE;
		} else {
			pr_warn("%pFWn: mode mapping not defined\n", np);
		}
	}

	if (!fwnode_property_read_u32(np, "regulator-system-load", &pval))
		constraints->system_load = pval;

	if (n_phandles) {
		constraints->max_spread = devm_kzalloc(dev,
				sizeof(*constraints->max_spread) * n_phandles,
				GFP_KERNEL);

		if (!constraints->max_spread)
			return -ENOMEM;

		fwnode_property_read_u32_array(np, "regulator-coupled-max-spread",
					   constraints->max_spread, n_phandles);
	}

	if (!fwnode_property_read_u32(np, "regulator-max-step-microvolt",
				  &pval))
		constraints->max_uV_step = pval;

	constraints->over_current_protection = fwnode_property_read_bool(np,
					"regulator-over-current-protection");

	fwnode_get_regulator_prot_limits(np, constraints);

	for (i = 0; i < ARRAY_SIZE(regulator_states); i++) {
		switch (i) {
		case PM_SUSPEND_MEM:
			suspend_state = &constraints->state_mem;
			break;
		case PM_SUSPEND_MAX:
			suspend_state = &constraints->state_disk;
			break;
		case PM_SUSPEND_STANDBY:
			suspend_state = &constraints->state_standby;
			break;
		case PM_SUSPEND_ON:
		case PM_SUSPEND_TO_IDLE:
		default:
			continue;
		}

		suspend_np = fwnode_get_named_child_node(np, regulator_states[i]);
		if (!suspend_np)
			continue;
		if (!suspend_state) {
			fwnode_handle_put(suspend_np);
			continue;
		}

		if (!fwnode_property_read_u32(suspend_np, "regulator-mode",
					  &pval)) {
			if (desc && desc->fwnode_map_mode) {
				mode = desc->fwnode_map_mode(pval);
				if (mode == REGULATOR_MODE_INVALID)
					pr_err("%pFWn: invalid mode %u\n",
					       np, pval);
				else
					suspend_state->mode = mode;
			} else {
				pr_warn("%pFWn: mapping for mode %d not defined\n",
					np, pval);
			}
		}

		if (fwnode_property_read_bool(suspend_np,
					"regulator-on-in-suspend"))
			suspend_state->enabled = ENABLE_IN_SUSPEND;
		else if (fwnode_property_read_bool(suspend_np,
					"regulator-off-in-suspend"))
			suspend_state->enabled = DISABLE_IN_SUSPEND;

		if (!fwnode_property_read_u32(suspend_np,
				"regulator-suspend-min-microvolt", &pval))
			suspend_state->min_uV = pval;

		if (!fwnode_property_read_u32(suspend_np,
				"regulator-suspend-max-microvolt", &pval))
			suspend_state->max_uV = pval;

		if (!fwnode_property_read_u32(suspend_np,
					"regulator-suspend-microvolt", &pval))
			suspend_state->uV = pval;
		else /* otherwise use min_uV as default suspend voltage */
			suspend_state->uV = suspend_state->min_uV;

		if (fwnode_property_read_bool(suspend_np,
					"regulator-changeable-in-suspend"))
			suspend_state->changeable = true;

		if (i == PM_SUSPEND_MEM)
			constraints->initial_state = PM_SUSPEND_MEM;

		fwnode_handle_put(suspend_np);
		suspend_state = NULL;
		suspend_np = NULL;
	}

	return 0;
}

/**
 * fwnode_get_regulator_init_data - extract regulator_init_data structure info
 * @dev: device requesting for regulator_init_data
 * @node: regulator device node
 * @desc: regulator description
 *
 * Populates regulator_init_data structure by extracting data from device
 * tree node, returns a pointer to the populated structure or NULL if memory
 * alloc fails.
 */
struct regulator_init_data *fwnode_get_regulator_init_data(struct device *dev,
					  struct fwnode_handle *node,
					  const struct regulator_desc *desc)
{
	struct regulator_init_data *init_data;

	if (!node)
		return NULL;

	init_data = devm_kzalloc(dev, sizeof(*init_data), GFP_KERNEL);
	if (!init_data)
		return NULL; /* Out of memory? */

	if (fwnode_get_regulation_constraints(dev, node, &init_data, desc))
		return NULL;

	return init_data;
}
EXPORT_SYMBOL_GPL(fwnode_get_regulator_init_data);

struct devm_fwnode_regulator_matches {
	struct fwnode_regulator_match *matches;
	unsigned int num_matches;
};

static void devm_fwnode_regulator_put_matches(struct device *dev, void *res)
{
	struct devm_fwnode_regulator_matches *devm_matches = res;
	int i;

	for (i = 0; i < devm_matches->num_matches; i++)
		fwnode_handle_put(devm_matches->matches[i].fwnode);
}

/**
 * fwnode_regulator_match - extract multiple regulator init data from device tree.
 * @dev: device requesting the data
 * @node: parent device node of the regulators
 * @matches: match table for the regulators
 * @num_matches: number of entries in match table
 *
 * This function uses a match table specified by the regulator driver to
 * parse regulator init data from the device tree. @node is expected to
 * contain a set of child nodes, each providing the init data for one
 * regulator. The data parsed from a child node will be matched to a regulator
 * based on either the deprecated property regulator-compatible if present,
 * or otherwise the child node's name. Note that the match table is modified
 * in place and an additional of_node reference is taken for each matched
 * regulator.
 *
 * Returns the number of matches found or a negative error code on failure.
 */
int fwnode_regulator_match(struct device *dev, struct fwnode_handle *node,
		       struct fwnode_regulator_match *matches,
		       unsigned int num_matches)
{
	unsigned int count = 0;
	unsigned int i;
	int ret;
	const char *name;
	struct fwnode_handle *child;
	struct devm_fwnode_regulator_matches *devm_matches;

	if (!dev || !node)
		return -EINVAL;

	devm_matches = devres_alloc(devm_fwnode_regulator_put_matches,
				    sizeof(struct devm_fwnode_regulator_matches),
				    GFP_KERNEL);
	if (!devm_matches)
		return -ENOMEM;

	devm_matches->matches = matches;
	devm_matches->num_matches = num_matches;

	devres_add(dev, devm_matches);

	for (i = 0; i < num_matches; i++) {
		struct fwnode_regulator_match *match = &matches[i];
		match->init_data = NULL;
		match->fwnode = NULL;
	}

	fwnode_for_each_child_node(node, child) {
		ret = fwnode_property_read_string(child,
					"regulator-compatible", &name);
		if (ret || !name)
			name = fwnode_get_name(child);
		for (i = 0; i < num_matches; i++) {
			struct fwnode_regulator_match *match = &matches[i];
			if (match->fwnode)
				continue;

			if (strcmp(match->name, name))
				continue;

			match->init_data =
				fwnode_get_regulator_init_data(dev, child,
							   match->desc);
			if (!match->init_data) {
				dev_err(dev,
					"failed to parse Fwnode for regulator %pFWn\n",
					child);
				fwnode_handle_put(child);
				return -EINVAL;
			}
			match->fwnode = fwnode_handle_get(child);
			count++;
			break;
		}
	}

	return count;
}
EXPORT_SYMBOL_GPL(fwnode_regulator_match);

static struct
fwnode_handle *regulator_fwnode_get_init_node(struct device *dev,
					const struct regulator_desc *desc)
{
	struct fwnode_handle *search, *child;
	const char *name;
	int ret;

	if (!dev->fwnode || !desc->fwnode_match)
		return NULL;

	if (desc->regulators_node) {
		search = fwnode_get_named_child_node(dev->fwnode,
					      desc->regulators_node);
	} else {
		search = fwnode_handle_get(dev->fwnode);

		if (!strcmp(desc->fwnode_match, fwnode_get_name(search)))
			return search;
	}

	if (!search) {
		dev_dbg(dev, "Failed to find regulator container node '%s'\n",
			desc->regulators_node);
		return NULL;
	}

	fwnode_for_each_available_child_node(search, child) {
		ret = fwnode_property_read_string(child, "regulator-compatible", &name);
		if (ret || !name) {
			name = fwnode_get_name(child);
		}

		if (!strcmp(desc->fwnode_match, name)) {
			fwnode_handle_put(search);
			/*
			 * 'fwnode_handle_get(child)' is already performed by the
			 * for_each loop.
			 */
			return child;
		}
	}

	fwnode_handle_put(search);

	return NULL;
}

struct regulator_init_data *regulator_fwnode_get_init_data(struct device *dev,
					    const struct regulator_desc *desc,
					    struct regulator_config *config,
					    struct fwnode_handle **node)
{
	struct fwnode_handle *child;
	struct regulator_init_data *init_data = NULL;

	child = regulator_fwnode_get_init_node(config->dev, desc);
	if (!child)
		return NULL;

	init_data = fwnode_get_regulator_init_data(dev, child, desc);
	if (!init_data) {
		dev_err(dev, "failed to parse Fwnode for regulator %pFWn\n", child);
		goto error;
	}

	if (desc->fwnode_parse_cb) {
		int ret;

		ret = desc->fwnode_parse_cb(child, desc, config);
		if (ret) {
			if (ret == -EPROBE_DEFER) {
				fwnode_handle_put(child);
				return ERR_PTR(-EPROBE_DEFER);
			}
			dev_err(dev,
				"driver callback failed to parse Fwnode for regulator %pFWn\n",
				child);
			goto error;
		}
	}

	*node = child;

	return init_data;

error:
	fwnode_handle_put(child);

	return NULL;
}

struct regulator_dev *fwnode_find_regulator_by_node(struct fwnode_handle *np)
{
	struct device *dev;

	dev = class_find_device_by_fwnode(&regulator_class, np);

	return dev ? dev_to_rdev(dev) : NULL;
}

/*
 * Returns number of regulators coupled with rdev.
 */
int fwnode_get_n_coupled(struct regulator_dev *rdev)
{
	struct fwnode_handle *node = rdev->dev.fwnode;
	int n_phandles;

	n_phandles = fwnode_count_reference_with_args(node,
						"regulator-coupled-with",
						NULL);

	return (n_phandles > 0) ? n_phandles : 0;
}

/* Looks for "to_find" fwnode in src's "regulator-coupled-with" property */
static bool fwnode_coupling_find_node(struct fwnode_handle *src,
				  struct fwnode_handle *to_find,
				  int *index)
{
	int n_phandles, i;
	bool found = false;

	n_phandles = fwnode_count_reference_with_args(src,
						"regulator-coupled-with",
						NULL);

	for (i = 0; i < n_phandles; i++) {
		struct fwnode_handle *tmp = fwnode_find_reference(src,
					   "regulator-coupled-with", i);

		if (!tmp)
			break;

		/* found */
		if (tmp == to_find)
			found = true;

		fwnode_handle_put(tmp);

		if (found) {
			*index = i;
			break;
		}
	}

	return found;
}

/**
 * fwnode_check_coupling_data - Parse rdev's coupling properties and check data
 *			    consistency
 * @rdev: pointer to regulator_dev whose data is checked
 *
 * Function checks if all the following conditions are met:
 * - rdev's max_spread is greater than 0
 * - all coupled regulators have the same max_spread
 * - all coupled regulators have the same number of regulator_dev phandles
 * - all regulators are linked to each other
 *
 * Returns true if all conditions are met.
 */
bool fwnode_check_coupling_data(struct regulator_dev *rdev)
{
	struct fwnode_handle *node = rdev->dev.fwnode;
	int n_phandles = fwnode_get_n_coupled(rdev);
	struct fwnode_handle *c_node;
	int index;
	int i;
	bool ret = true;

	/* iterate over rdev's phandles */
	for (i = 0; i < n_phandles; i++) {
		int max_spread = rdev->constraints->max_spread[i];
		int c_max_spread, c_n_phandles;

		if (max_spread <= 0) {
			dev_err(&rdev->dev, "max_spread value invalid\n");
			return false;
		}

		c_node = fwnode_find_reference(node,
					  "regulator-coupled-with", i);

		if (!c_node)
			ret = false;

		c_n_phandles = fwnode_count_reference_with_args(c_node,
							  "regulator-coupled-with",
							  NULL);

		if (c_n_phandles != n_phandles) {
			dev_err(&rdev->dev, "number of coupled reg phandles mismatch\n");
			ret = false;
			goto clean;
		}

		if (!fwnode_coupling_find_node(c_node, node, &index)) {
			dev_err(&rdev->dev, "missing 2-way linking for coupled regulators\n");
			ret = false;
			goto clean;
		}

		int count = fwnode_property_count_u32(c_node, "regulator-coupled-max-spread");
		if (count > 0) {
			void *dbuf = kzalloc(4*count, GFP_KERNEL);
			if (!fwnode_property_read_u32_array(c_node, "regulator-coupled-max-spread", dbuf, count)) {
				c_max_spread = *((u32 *)dbuf + index);
			} else {
				ret = false;
				goto clean;
			}
		} else {
			ret = false;
			goto clean;
		}

		if (c_max_spread != max_spread) {
			dev_err(&rdev->dev,
				"coupled regulators max_spread mismatch\n");
			ret = false;
			goto clean;
		}

clean:
		fwnode_handle_put(c_node);
		if (!ret)
			break;
	}

	return ret;
}

/**
 * fwnode_parse_coupled_regulator() - Get regulator_dev pointer from rdev's property
 * @rdev: Pointer to regulator_dev, whose fwnode is used as a source to parse
 *	  "regulator-coupled-with" property
 * @index: Index in phandles array
 *
 * Returns the regulator_dev pointer parsed from AcpiTable. If it has not been yet
 * registered, returns NULL
 */
struct regulator_dev *fwnode_parse_coupled_regulator(struct regulator_dev *rdev,
						 int index)
{
	struct fwnode_handle *node = rdev->dev.fwnode;
	struct fwnode_handle *c_node;
	struct regulator_dev *c_rdev;

	c_node = fwnode_find_reference(node, "regulator-coupled-with", index);
	if (!c_node)
		return NULL;

	c_rdev = fwnode_find_regulator_by_node(c_node);

	fwnode_handle_put(c_node);

	return c_rdev;
}

/*
 * Check if name is a supply name according to the '*-supply' pattern
 * return 0 if false
 * return length of supply name without the -supply
 */
static int is_supply_name(const char *name)
{
	int strs, i;

	strs = strlen(name);
	/* string need to be at minimum len(x-supply) */
	if (strs < 8)
		return 0;
	for (i = strs - 6; i > 0; i--) {
		/* find first '-' and check if right part is supply */
		if (name[i] != '-')
			continue;
		if (strcmp(name + i + 1, "supply") != 0)
			return 0;
		return i;
	}
	return 0;
}
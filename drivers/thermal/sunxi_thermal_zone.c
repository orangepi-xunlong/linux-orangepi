// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi thermal zone manage module
 * Copyright (C) 2021 huangyongxing@allwinnertech.com
 */

#include <linux/thermal.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/types.h>
#include <linux/slab.h>
#include "thermal_core.h"
#include <linux/module.h>

static LIST_HEAD(sunxi_thermal_zone_list);

struct __sunxi_thermal_zone {
	enum thermal_device_mode mode;
	int passive_delay;
	int polling_delay;
	int slope;
	int offset;

	/* trip data */
	int ntrips;
	struct thermal_trip *trips;

	/* cooling binding data */
	int num_tbps;
	struct __thermal_bind_params *tbps;

	/* sensor interface */
	void *sensor_data;
	const struct thermal_zone_of_device_ops *ops;
};

struct sunxi_thermal_zone {
	struct device_node	*ts_node;
	int			ts_id;
	struct list_head	sunxi_tz_list;
	struct thermal_zone_device *tzd;
	struct __sunxi_thermal_zone *tz;
	void *sensor_data;
	const struct thermal_zone_of_device_ops *ops;
};

static int sunxi_thermal_get_temp(struct thermal_zone_device *tzd, int *temp)
{
	struct __sunxi_thermal_zone *tz = tzd->devdata;

	if (!tz->ops->get_temp)
		return -EINVAL;

	return tz->ops->get_temp(tz->sensor_data, temp);
}

static int sunxi_thermal_get_trend(struct thermal_zone_device *tzd, int trip,
			enum thermal_trend *trend)
{
	struct __sunxi_thermal_zone *tz = tzd->devdata;

	if (!tz->ops->get_trend)
		return -EINVAL;

	return tz->ops->get_trend(tz->sensor_data, trip, trend);
}


struct thermal_zone_device *populate_sunxi_thermal_zone(struct device_node *np)
{
	struct sunxi_thermal_zone *sunxi_tz, *pos;
	struct __sunxi_thermal_zone *tz;
	struct of_phandle_args sensor_specs;
	struct thermal_zone_device *tzd;
	int ret, id;

	sunxi_tz = kzalloc(sizeof(*sunxi_tz), GFP_KERNEL);
	if (!sunxi_tz)
		return ERR_PTR(-ENOMEM);

	tzd = thermal_zone_get_zone_by_name(np->name);

	ret = of_parse_phandle_with_args(np, "thermal-sensors",
					 "#thermal-sensor-cells",
					 0, &sensor_specs);
	if (ret)
		return ERR_PTR(-EINVAL);

	if (sensor_specs.args_count >= 1) {
		id = sensor_specs.args[0];
		WARN(sensor_specs.args_count > 1,
		     "%pOFn: too many cells in sensor specifier %d\n",
		     sensor_specs.np, sensor_specs.args_count);
	} else {
		id = 0;
	}

	sunxi_tz->tzd = tzd;
	sunxi_tz->ts_node = sensor_specs.np;
	sunxi_tz->ts_id = id;
	tz = tzd->devdata;
	sunxi_tz->tz = tz;

	if (tz->ops && tz->sensor_data) {
		sunxi_tz->ops = tz->ops;
		sunxi_tz->sensor_data = tz->sensor_data;
		list_add(&sunxi_tz->sunxi_tz_list, &sunxi_thermal_zone_list);
	}

	if (!tz->ops) {
		list_for_each_entry(pos, &sunxi_thermal_zone_list, sunxi_tz_list) {
			if ((sensor_specs.np == pos->ts_node) && (id == pos->ts_id)) {
				mutex_lock(&tzd->lock);
				tz->ops = pos->ops;
				tz->sensor_data = pos->sensor_data;
				tzd->ops->get_temp = sunxi_thermal_get_temp;
				tzd->ops->get_trend = sunxi_thermal_get_trend;
				mutex_unlock(&tzd->lock);
				tzd->ops->set_mode(tzd, THERMAL_DEVICE_ENABLED);
				pr_debug("sunxi_thermal_zone match muti sensor!\n");
			}
		};
	}

	return tzd;
};

static int __init sunxi_thermal_zone_init(void)
{
	struct device_node *np, *child;

	np = of_find_node_by_name(NULL, "thermal-zones");
	if (!np) {
		pr_debug("unable to find thermal zones\n");
		return -ENODEV;
	}

	for_each_available_child_of_node(np, child) {
		struct thermal_zone_device *tzd;
		struct thermal_zone_params *tzp;
		const char *governor_name;

		tzd = populate_sunxi_thermal_zone(child);
		if (IS_ERR(tzd))
			continue;

		tzp = tzd->tzp;

		if (!of_property_read_string(child, "governor-name", &governor_name))
			if (!strlen(governor_name))
				pr_err("Failed to get thermal governor-name\n");
		strcpy(tzp->governor_name, governor_name);

		thermal_zone_device_set_policy(tzd, tzp->governor_name);
	}

	return 0;
}

late_initcall_sync(sunxi_thermal_zone_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("huangyongxing<huangyongxing@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi thermal zone driver.");

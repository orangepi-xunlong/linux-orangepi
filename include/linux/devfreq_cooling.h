/* SPDX-License-Identifier: GPL-2.0 */
/*
 * devfreq_cooling: Thermal cooling device implementation for devices using
 *                  devfreq
 *
 * Copyright (C) 2014-2015 ARM Limited
 *
 */

#ifndef __DEVFREQ_COOLING_H__
#define __DEVFREQ_COOLING_H__

#include <linux/devfreq.h>
#include <linux/thermal.h>


/**
 * struct devfreq_cooling_power - Devfreq cooling power ops
 * @get_real_power:	When this is set, the framework uses it to ask the
 *			device driver for the actual power.
 *			Some devices have more sophisticated methods
 *			(like power counters) to approximate the actual power
 *			that they use.
 *			This function provides more accurate data to the
 *			thermal governor. When the driver does not provide
 *			such function, framework just uses pre-calculated
 *			table and scale the power by 'utilization'
 *			(based on 'busy_time' and 'total_time' taken from
 *			devfreq 'last_status').
 *			The value returned by this function must be lower
 *			or equal than the maximum power value
 *			for the current	state
 *			(which can be found in power_table[state]).
 *			When this interface is used, the power_table holds
 *			max total (static + dynamic) power value for each OPP.
 */
struct devfreq_cooling_power {
	int (*get_real_power)(struct devfreq *df, u32 *power,
			      unsigned long freq, unsigned long voltage);
};

/**
 * struct devfreq_cooling_device - Devfreq cooling device
 *		devfreq_cooling_device registered.
 * @cdev:	Pointer to associated thermal cooling device.
 * @cooling_ops: devfreq callbacks to thermal cooling device ops
 * @devfreq:	Pointer to associated devfreq device.
 * @cooling_state:	Current cooling state.
 * @freq_table:	Pointer to a table with the frequencies sorted in descending
 *		order.  You can index the table by cooling device state
 * @max_state:	It is the last index, that is, one less than the number of the
 *		OPPs
 * @power_ops:	Pointer to devfreq_cooling_power, a more precised model.
 * @res_util:	Resource utilization scaling factor for the power.
 *		It is multiplied by 100 to minimize the error. It is used
 *		for estimation of the power budget instead of using
 *		'utilization' (which is	'busy_time' / 'total_time').
 *		The 'res_util' range is from 100 to power * 100	for the
 *		corresponding 'state'.
 * @capped_state:	index to cooling state with in dynamic power budget
 * @req_max_freq:	PM QoS request for limiting the maximum frequency
 *			of the devfreq device.
 * @em_pd:		Energy Model for the associated Devfreq device
 */
struct devfreq_cooling_device {
	struct thermal_cooling_device *cdev;
	struct thermal_cooling_device_ops cooling_ops;
	struct devfreq *devfreq;
	unsigned long cooling_state;
	u32 *freq_table;
	size_t max_state;
	struct devfreq_cooling_power *power_ops;
	u32 res_util;
	int capped_state;
	struct dev_pm_qos_request req_max_freq;
	struct em_perf_domain *em_pd;
};

#ifdef CONFIG_DEVFREQ_THERMAL

struct thermal_cooling_device *
of_devfreq_cooling_register_power(struct device_node *np, struct devfreq *df,
				  struct devfreq_cooling_power *dfc_power);
struct thermal_cooling_device *
of_devfreq_cooling_register(struct device_node *np, struct devfreq *df);
struct thermal_cooling_device *devfreq_cooling_register(struct devfreq *df);
void devfreq_cooling_unregister(struct thermal_cooling_device *dfc);
struct thermal_cooling_device *
devfreq_cooling_em_register(struct devfreq *df,
			    struct devfreq_cooling_power *dfc_power);

#else /* !CONFIG_DEVFREQ_THERMAL */

static inline struct thermal_cooling_device *
of_devfreq_cooling_register_power(struct device_node *np, struct devfreq *df,
				  struct devfreq_cooling_power *dfc_power)
{
	return ERR_PTR(-EINVAL);
}

static inline struct thermal_cooling_device *
of_devfreq_cooling_register(struct device_node *np, struct devfreq *df)
{
	return ERR_PTR(-EINVAL);
}

static inline struct thermal_cooling_device *
devfreq_cooling_register(struct devfreq *df)
{
	return ERR_PTR(-EINVAL);
}

static inline struct thermal_cooling_device *
devfreq_cooling_em_register(struct devfreq *df,
			    struct devfreq_cooling_power *dfc_power)
{
	return ERR_PTR(-EINVAL);
}

static inline void
devfreq_cooling_unregister(struct thermal_cooling_device *dfc)
{
}

#endif /* CONFIG_DEVFREQ_THERMAL */
#endif /* __DEVFREQ_COOLING_H__ */

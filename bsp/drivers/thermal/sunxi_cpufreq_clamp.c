/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs power domain test driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/thermal.h>
#include <linux/module.h>

/* KHz */
#define CPUB_UPPER_LIMIT_FREQ		(1992000 - 1)

/* ms */
#define THERMAL_POLLING_DELAY		(10 * 1000)

/* Celsius */
#define CPUB_UPPER_LIMIT_TEMP		(70)
#define HYSTERESIS			(3)

#define CPUB_CORE_NUM			(4)
#define CPUB_THERMAL_ZONE		"cpub_thermal_zone"

static struct sunxi_clamp_control *clamp_control;

struct sunxi_clamp_control {
	bool				clamped;
	struct freq_qos_request		qos_req;
	struct thermal_zone_device	*thermal;
	struct delayed_work		thermal_mon;
};

static void thermal_zone_monitor(struct work_struct *work)
{
	struct sunxi_clamp_control *clamp =
		container_of(work, typeof(*clamp), thermal_mon.work);
	int ret, temperature = 0;

	ret = thermal_zone_get_temp(clamp->thermal, &temperature);
	if (ret) {
		sunxi_err(NULL, "Failed to get temperature (%d)\n", ret);
	}
	temperature /= 1000;

	if (temperature >= CPUB_UPPER_LIMIT_TEMP && !clamp->clamped) {
		clamp->clamped = true;
		freq_qos_update_request(&clamp->qos_req, CPUB_UPPER_LIMIT_FREQ);
		sunxi_info(NULL, "%s: Clamping CPU frequency to %d\n", __func__, CPUB_UPPER_LIMIT_FREQ);
	} else if (temperature < (CPUB_UPPER_LIMIT_TEMP - HYSTERESIS) && clamp->clamped) {
		clamp->clamped = false;
		freq_qos_update_request(&clamp->qos_req, INT_MAX);
		sunxi_info(NULL, "%s: CPU frequency unclamped\n", __func__);
	}

	schedule_delayed_work(&clamp->thermal_mon, msecs_to_jiffies(THERMAL_POLLING_DELAY));
}

static int __init sunxi_cpufreq_clamp_init(void)
{
	struct cpufreq_policy *policy;
	struct sunxi_clamp_control *clamp;
	int ret;

	clamp = kzalloc(sizeof(struct sunxi_clamp_control), GFP_KERNEL);
	if (clamp == NULL) {
		ret = -ENOMEM;
		return ret;
	}

	clamp->thermal = thermal_zone_get_zone_by_name(CPUB_THERMAL_ZONE);
	if (IS_ERR(clamp->thermal)) {
		sunxi_err(NULL, "Failed to get thermal zone (%ld)\n", PTR_ERR(clamp->thermal));
		ret = -EPROBE_DEFER;
		goto fail;
	}

	policy = cpufreq_cpu_get(CPUB_CORE_NUM);
	if (!policy) {
		sunxi_warn(NULL, "Cpufreq policy not found cpu0\n");
		ret = -EPROBE_DEFER;
		goto fail;
	}

	ret = freq_qos_add_request(&policy->constraints, &clamp->qos_req, FREQ_QOS_MAX,
				   policy->cpuinfo.max_freq);

	cpufreq_cpu_put(policy);

	if (ret < 0) {
		sunxi_err(NULL, "Failed to add freq constraint (%d)\n", ret);
		goto fail;
	}

	clamp_control = clamp;

	INIT_DELAYED_WORK(&clamp->thermal_mon, thermal_zone_monitor);
	schedule_delayed_work(&clamp->thermal_mon, msecs_to_jiffies(THERMAL_POLLING_DELAY));

	return 0;

fail:
	kfree(clamp);

	return ret;
}

static void __exit sunxi_cpufreq_clamp_exit(void)
{
	struct sunxi_clamp_control *clamp = clamp_control;

	if (clamp) {
		cancel_delayed_work_sync(&clamp->thermal_mon);
		freq_qos_remove_request(&clamp->qos_req);
		kfree(clamp);
	}
}

late_initcall(sunxi_cpufreq_clamp_init);
module_exit(sunxi_cpufreq_clamp_exit);

MODULE_AUTHOR("Maijianzhang<maijianzhang@allwinnertech.com");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("CPU frequency clamp for thermal control");
MODULE_LICENSE("GPL");

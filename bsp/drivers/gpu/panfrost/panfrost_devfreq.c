// SPDX-License-Identifier: GPL-2.0
/* Copyright 2019 Collabora ltd. */

#include <linux/clk.h>
#include <linux/devfreq.h>
#include <linux/devfreq_cooling.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>

#include "panfrost_device.h"
#include "panfrost_devfreq.h"

#include <sunxi-sid.h>
#if (defined(CONFIG_PM_OPP) && defined(CONFIG_ARCH_SUN55IW3))
static int sunxi_match_vf_table(u32 combi, u32 *index)
{
	struct device_node *np = NULL;
	int nsels, ret, i;
	u32 tmp;

	np = of_find_node_by_name(NULL, "gpu_vf_mapping_table");
	if (!np) {
		pr_err("Unable to find node 'note'\n");
		return -EINVAL;
	}

	if (!of_get_property(np, "table", &nsels))
		return -EINVAL;

	nsels /= sizeof(u32);
	if (!nsels) {
		pr_err("invalid table property size\n");
		return -EINVAL;
	}

	for (i = 0; i < nsels / 2; i++) {
		ret = of_property_read_u32_index(np, "table", i * 2, &tmp);
		if (ret) {
			pr_err("could not retrieve table property: %d\n", ret);
			return ret;
		}

		if (tmp == combi) {
			ret = of_property_read_u32_index(np, "table", i * 2 + 1, &tmp);
			if (ret) {
				pr_err("could not retrieve table property: %d\n", ret);
				return ret;
			}

			*index = tmp;
			break;
		} else {
			continue;
		}
	}

	if (i == nsels/2)
		pr_notice("%s %d, could not match vf table, i:%d", __func__, __LINE__, i);

	return 0;
}

#define MAX_NAME_LEN	8
#define SUN55IW3_MARKETID_EFUSE_OFF	(0x00)
#define SUN55IW3_DVFS_EFUSE_OFF		(0x48)
static void sunxi_get_gpu_opp_table_name(char *name, unsigned int u_volt)
{
	u32 marketid, back_dvfs, dvfs, combi;
	u32 index = 1;

	sunxi_get_module_param_from_sid(&marketid, SUN55IW3_MARKETID_EFUSE_OFF, 4);
	marketid &= 0xffff;
	sunxi_get_module_param_from_sid(&dvfs, SUN55IW3_DVFS_EFUSE_OFF, 4);
	back_dvfs = (dvfs >> 12) & 0xff;

	if (back_dvfs)
		combi = back_dvfs;
	else
		combi = (dvfs >> 4) & 0xff;

	if ((marketid == 0x5200) && (combi == 0x00))
		index = 0;
	else
		sunxi_match_vf_table(combi, &index);

	snprintf(name, MAX_NAME_LEN, "vf%u%.3d", index, u_volt);
}

static void sunxi_set_gpu_opp_table_name(struct panfrost_device *pfdev)
{
	char opp_table_name[MAX_NAME_LEN] = "0x00";
	unsigned int u_volt = 900000;
	struct regulator *mali_regulator;

	/* To get mali supply's voltage */
	mali_regulator = regulator_get_optional(pfdev->dev,
				pfdev->comp->supply_names[0]);
	if (IS_ERR(mali_regulator)) {
		dev_err(pfdev->dev, "sunxi get mali_regulator failed, ret is %ld",
				PTR_ERR(mali_regulator));
		mali_regulator = NULL;
	}

	if (mali_regulator) {
		u_volt = regulator_get_voltage(mali_regulator);
		regulator_put(mali_regulator);
		mali_regulator = NULL;
	}
	dev_info(pfdev->dev, "get gpu's volt is %d mv\n", u_volt / 1000);

	/* uV -> mV */
	u_volt = u_volt / 1000;
	sunxi_get_gpu_opp_table_name(opp_table_name, u_volt);
	dev_info(pfdev->dev, "mali get opp_table_name is %s\n", opp_table_name);

	pfdev->opp_table = dev_pm_opp_set_prop_name(pfdev->dev, opp_table_name);
	if (IS_ERR(pfdev->opp_table)) {
		dev_err(pfdev->dev, "Failed to set prop name, use default vf\n");
	}

}
#endif /* CONFIG_PM_OPP */

static void panfrost_devfreq_update_utilization(struct panfrost_devfreq *pfdevfreq)
{
	ktime_t now, last;

	now = ktime_get();
	last = pfdevfreq->time_last_update;

	if (pfdevfreq->busy_count > 0)
		pfdevfreq->busy_time += ktime_sub(now, last);
	else
		pfdevfreq->idle_time += ktime_sub(now, last);

	pfdevfreq->time_last_update = now;
}

static int panfrost_devfreq_target(struct device *dev, unsigned long *freq,
				   u32 flags)
{
	struct dev_pm_opp *opp;

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp))
		return PTR_ERR(opp);
	dev_pm_opp_put(opp);

	return dev_pm_opp_set_rate(dev, *freq);
}

static void panfrost_devfreq_reset(struct panfrost_devfreq *pfdevfreq)
{
	pfdevfreq->busy_time = 0;
	pfdevfreq->idle_time = 0;
	pfdevfreq->time_last_update = ktime_get();
}

static int panfrost_devfreq_get_dev_status(struct device *dev,
					   struct devfreq_dev_status *status)
{
	struct panfrost_device *pfdev = dev_get_drvdata(dev);
	struct panfrost_devfreq *pfdevfreq = &pfdev->pfdevfreq;
	unsigned long irqflags;

	status->current_frequency = clk_get_rate(pfdev->clock);

	spin_lock_irqsave(&pfdevfreq->lock, irqflags);

	panfrost_devfreq_update_utilization(pfdevfreq);

	status->total_time = ktime_to_ns(ktime_add(pfdevfreq->busy_time,
						   pfdevfreq->idle_time));

	status->busy_time = ktime_to_ns(pfdevfreq->busy_time);

	panfrost_devfreq_reset(pfdevfreq);

	spin_unlock_irqrestore(&pfdevfreq->lock, irqflags);

	dev_dbg(pfdev->dev, "busy %lu total %lu %lu %% freq %lu MHz\n",
		status->busy_time, status->total_time,
		status->busy_time / (status->total_time / 100),
		status->current_frequency / 1000 / 1000);

	return 0;
}

static struct devfreq_dev_profile panfrost_devfreq_profile = {
	.timer = DEVFREQ_TIMER_DELAYED,
	.polling_ms = 50, /* ~3 frames */
	.target = panfrost_devfreq_target,
	.get_dev_status = panfrost_devfreq_get_dev_status,
};

int panfrost_devfreq_init(struct panfrost_device *pfdev)
{
	int ret;
	struct dev_pm_opp *opp;
	unsigned long cur_freq;
	struct device *dev = &pfdev->pdev->dev;
	struct devfreq *devfreq;
	struct thermal_cooling_device *cooling;
	struct panfrost_devfreq *pfdevfreq = &pfdev->pfdevfreq;
	unsigned long freq = ULONG_MAX;

	ret = 0;
	if (pfdev->comp->num_supplies > 1) {
		/*
		 * GPUs with more than 1 supply require platform-specific handling:
		 * continue without devfreq
		 */
		DRM_DEV_INFO(dev, "More than 1 supply is not supported yet\n");
		return 0;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	ret = devm_pm_opp_set_regulators(dev, pfdev->comp->supply_names,
					 pfdev->comp->num_supplies);
	if (ret) {
		/* Continue if the optional regulator is missing */
		if (ret != -ENODEV) {
			if (ret != -EPROBE_DEFER)
				DRM_DEV_ERROR(dev, "Couldn't set OPP regulators\n");
			return ret;
		}
	}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	pfdevfreq->regulators_opp_table = dev_pm_opp_set_regulators(
					      dev,
					      pfdev->comp->supply_names,
					      pfdev->comp->num_supplies);
	if (IS_ERR(pfdevfreq->regulators_opp_table)) {
		ret = PTR_ERR(pfdevfreq->regulators_opp_table);
		/* Continue if the optional regulator is missing */
		if (ret != -ENODEV) {
			DRM_DEV_ERROR(dev, "Couldn't set OPP regulators\n");
			goto err_opp_out;
		}
	}
#endif


#if (defined(CONFIG_PM_OPP) && defined(CONFIG_ARCH_SUN55IW3))
	/* Set mali regulator's name, the opp will process opp_tables
	 * automatically; Unsupported frequency points will be deleted.
	 */
	sunxi_set_gpu_opp_table_name(pfdev);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	ret = devm_pm_opp_of_add_table(dev);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ret = dev_pm_opp_of_add_table(dev);
#endif
	if (ret) {
		/* Optional, continue without devfreq */
		if (ret == -ENODEV)
			ret = 0;
		goto err_opp_out;
	}

	pfdevfreq->opp_of_table_added = true;

	spin_lock_init(&pfdevfreq->lock);

	panfrost_devfreq_reset(pfdevfreq);

	/* Set tp the highest frequency during initialization */
	opp = dev_pm_opp_find_freq_floor(pfdev->dev, &freq);
	if (IS_ERR_OR_NULL(opp)) {
		dev_err(pfdev->dev, "sunxi init gpu Failed to get opp (%ld)\n",
				PTR_ERR(opp));
	} else {
		clk_set_rate(pfdev->parent_clock, freq);
		clk_set_rate(pfdev->clock, freq);
		/* dev_pm_opp_find_freq_floor will call dev_pm_opp_get(),
		 * whilh increment the reference count of OPP.
		 *
		 * So, The callers are required to call dev_pm_opp_put()
		 * for the returned OPP after use.
		 */
		dev_pm_opp_put(opp);
	}

	cur_freq = clk_get_rate(pfdev->clock);

	opp = devfreq_recommended_opp(dev, &cur_freq, 0);
	if (IS_ERR(opp)) {
		ret = PTR_ERR(opp);
		goto err_opp_out;
	}

	panfrost_devfreq_profile.initial_freq = cur_freq;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	/*
	 * Set the recommend OPP this will enable and configure the regulator
	 * if any and will avoid a switch off by regulator_late_cleanup()
	 */
	ret = dev_pm_opp_set_opp(dev, opp);
	if (ret) {
		DRM_DEV_ERROR(dev, "Couldn't set recommended OPP\n");
		goto err_opp_out;
	}
#endif

	dev_pm_opp_put(opp);

	/*
	 * Setup default thresholds for the simple_ondemand governor.
	 * The values are chosen based on experiments.
	 */
	pfdevfreq->gov_data.upthreshold = 45;
	pfdevfreq->gov_data.downdifferential = 5;

	devfreq = devm_devfreq_add_device(dev, &panfrost_devfreq_profile,
					  DEVFREQ_GOV_SIMPLE_ONDEMAND,
					  &pfdevfreq->gov_data);
	if (IS_ERR(devfreq)) {
		DRM_DEV_ERROR(dev, "Couldn't initialize GPU devfreq\n");
		return PTR_ERR(devfreq);
	}
	pfdevfreq->devfreq = devfreq;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	cooling = devfreq_cooling_em_register(devfreq, NULL);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	cooling = of_devfreq_cooling_register(dev->of_node, devfreq);
#endif
	if (IS_ERR(cooling))
		DRM_DEV_INFO(dev, "Failed to register cooling device\n");
	else
		pfdevfreq->cooling = cooling;
	return 0;

err_opp_out:
	panfrost_devfreq_fini(pfdev);
	return ret;
}

void panfrost_devfreq_fini(struct panfrost_device *pfdev)
{
	struct panfrost_devfreq *pfdevfreq = &pfdev->pfdevfreq;

#if (defined(CONFIG_PM_OPP) && defined(CONFIG_ARCH_SUN55IW3))
	if (pfdev->opp_table) {
		/*
		 * NOTE: To release resources blocked for prop-name,
		 * pfdev->opp_table is returned by dev_pm_opp_put_prop_name.
		 *
		 * This is required only for the V2 bindings, and is called
		 * for a matching dev_pm_opp_set_prop_name(). Until this is
		 * called, the opp_table structure will not be freed.
		 *
		 * Otherwise, rmmod and reinsmod panfrost.ko,
		 * dev_pm_opp_set_regulators will be failed.
		 */
		dev_pm_opp_put_prop_name(pfdev->opp_table);
		pfdev->opp_table = NULL;
	}
#endif
	if (pfdevfreq->cooling) {
		devfreq_cooling_unregister(pfdevfreq->cooling);
		pfdevfreq->cooling = NULL;
	}

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)))
	if (pfdevfreq->opp_of_table_added) {
		dev_pm_opp_of_remove_table(&pfdev->pdev->dev);
		pfdevfreq->opp_of_table_added = false;
	}

	if (pfdevfreq->regulators_opp_table) {
		dev_pm_opp_put_regulators(pfdevfreq->regulators_opp_table);
		pfdevfreq->regulators_opp_table = NULL;
	}
#endif
}

void panfrost_devfreq_resume(struct panfrost_device *pfdev)
{
	struct panfrost_devfreq *pfdevfreq = &pfdev->pfdevfreq;

	if (!pfdevfreq->devfreq)
		return;

	panfrost_devfreq_reset(pfdevfreq);

	devfreq_resume_device(pfdevfreq->devfreq);
}

void panfrost_devfreq_suspend(struct panfrost_device *pfdev)
{
	struct panfrost_devfreq *pfdevfreq = &pfdev->pfdevfreq;

	if (!pfdevfreq->devfreq)
		return;

	devfreq_suspend_device(pfdevfreq->devfreq);
}

void panfrost_devfreq_record_busy(struct panfrost_devfreq *pfdevfreq)
{
	unsigned long irqflags;

	if (!pfdevfreq->devfreq)
		return;

	spin_lock_irqsave(&pfdevfreq->lock, irqflags);

	panfrost_devfreq_update_utilization(pfdevfreq);

	pfdevfreq->busy_count++;

	spin_unlock_irqrestore(&pfdevfreq->lock, irqflags);
}

void panfrost_devfreq_record_idle(struct panfrost_devfreq *pfdevfreq)
{
	unsigned long irqflags;

	if (!pfdevfreq->devfreq)
		return;

	spin_lock_irqsave(&pfdevfreq->lock, irqflags);

	panfrost_devfreq_update_utilization(pfdevfreq);

	WARN_ON(--pfdevfreq->busy_count < 0);

	spin_unlock_irqrestore(&pfdevfreq->lock, irqflags);
}

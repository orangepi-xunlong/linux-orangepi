// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic Cix Bus frequency driver with DEVFREQ Framework
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.All Rights Reserved.
 */

#include <linux/devfreq.h>
#include <linux/devfreq-event.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/scmi_protocol.h>

#define EVENT_ENABLE	1
#define UP_THRESHOLD	(80)
#define DOWN_THRESHOLD	(20)
#define MS_PER_SEC	(1000)
#define PERIOD_RATIO	(10)

struct cix_bus {
	struct device *dev;
	struct devfreq *devfreq;
	struct devfreq_event_dev *edev;
	struct devfreq_simple_ondemand_data ondemand_data;
	struct mutex lock;
	unsigned long curr_freq;
};

/*
 * devfreq function for both simple-ondemand and passive governor
 */
static int cix_bus_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct cix_bus *bus = dev_get_drvdata(dev);
	struct dev_pm_opp *new_opp;
	int ret = 0;

	/* Get correct frequency for bus. */
	new_opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(new_opp)) {
		dev_err(dev, "failed to get recommended opp instance\n");
		return PTR_ERR(new_opp);
	}

	dev_pm_opp_put(new_opp);

	/* Change voltage and frequency according to new OPP level */
	mutex_lock(&bus->lock);
	ret = scmi_device_set_freq(dev, *freq);
	if (!ret)
		bus->curr_freq = *freq;

	mutex_unlock(&bus->lock);

	return ret;
}

static int cix_bus_get_dev_status(struct device *dev,
				     struct devfreq_dev_status *stat)
{
	struct cix_bus *bus = dev_get_drvdata(dev);
	struct devfreq_event_data edata;
	int ret;

	stat->current_frequency = bus->curr_freq;

#if EVENT_ENABLE
	ret = devfreq_event_get_event(bus->edev, &edata);
	if (ret < 0) {
		dev_err(dev, "failed to get event from devfreq-event devices\n");
		stat->total_time = stat->busy_time = 0;
		return ret;
	}

	stat->busy_time = edata.load_count;
	/* get normal bw */
	edata.total_count = bus->curr_freq *
			bus->devfreq->profile->polling_ms /
			(MS_PER_SEC * PERIOD_RATIO);
	stat->total_time = edata.total_count;

	stat->busy_time = min(stat->busy_time, stat->total_time);
	dev_dbg(dev, "Usage of devfreq-event : %lu/%lu\n", stat->busy_time,
							stat->total_time);

#endif
	return ret;
}

static int cix_bus_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct cix_bus *bus = dev_get_drvdata(dev);

	mutex_lock(&bus->lock);
	*freq = scmi_device_get_freq(dev);
	mutex_unlock(&bus->lock);

	return 0;
}

static void cix_bus_exit(struct device *dev)
{
	struct cix_bus *bus = dev_get_drvdata(dev);
	int ret;
#if EVENT_ENABLE
	ret = devfreq_event_disable_edev(bus->edev);
	if (ret < 0)
		dev_warn(dev, "failed to disable the devfreq-event devices\n");
#endif
	dev_pm_opp_of_remove_table(dev);
}

static struct devfreq_dev_profile cix_bus_devfreq_profile = {
	.polling_ms = 50,
	.target = cix_bus_target,
	.get_dev_status = cix_bus_get_dev_status,
	.get_cur_freq = cix_bus_get_cur_freq,
	.exit = cix_bus_exit,
};

static int cix_bus_events_parse_of(struct device_node *np,
					struct cix_bus *bus)
{
	struct device *dev = bus->dev;
	int ret;
#if EVENT_ENABLE
	bus->edev = devfreq_event_get_edev_by_phandle(dev,
						"devfreq-events", 0);
	if (IS_ERR(bus->edev)) {
		ret = -EPROBE_DEFER;
		return ret;
	}
	ret = devfreq_event_enable_edev(bus->edev);
	if (ret < 0) {
		dev_err(dev, "failed to enable devfreq-event devices\n");
		return ret;
	}
#endif
	return 0;
}

static int cix_bus_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const char *gov = DEVFREQ_GOV_USERSPACE;
	struct cix_bus *bus;
	int ret, nr_opp;

	bus = devm_kzalloc(&pdev->dev, sizeof(*bus), GFP_KERNEL);
	if (!bus)
		return -ENOMEM;

	mutex_init(&bus->lock);
	bus->dev = &pdev->dev;
	platform_set_drvdata(pdev, bus);
	ret = cix_bus_events_parse_of(dev->of_node, bus);
	if (ret < 0)
		return ret;

	nr_opp = dev_pm_opp_get_opp_count(dev);
	if (nr_opp <= 0) {
		ret = scmi_device_opp_table_parse(dev);
		if (ret) {
			dev_warn(dev, "failed to add opps to the device\n");
			return ret;
		}
	};

	bus->ondemand_data.upthreshold = UP_THRESHOLD;
	bus->ondemand_data.downdifferential = DOWN_THRESHOLD;
	bus->devfreq = devm_devfreq_add_device(dev, &cix_bus_devfreq_profile,
					      gov, &bus->ondemand_data);
	if (IS_ERR(bus->devfreq)) {
		dev_err(dev, "failed to add devfreq device\n");
		return PTR_ERR(bus->devfreq);
	}

	/* Register opp_notifier to catch the change of OPP  */
	ret = devm_devfreq_register_opp_notifier(dev, bus->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to register opp notifier\n");
		goto err;
	}
	return 0;

err:
	dev_pm_opp_of_remove_table(dev);
	devfreq_event_disable_edev(bus->edev);

	return ret;
}

static void cix_bus_shutdown(struct platform_device *pdev)
{
	struct cix_bus *bus = dev_get_drvdata(&pdev->dev);
	int ret = 0;

	ret = devfreq_event_disable_edev(bus->edev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to disable the devfreq-event devices\n");
		return;
	}

	ret = devfreq_suspend_device(bus->devfreq);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to suspend the devfreq devices\n");
		return;
	}

}

#ifdef CONFIG_PM_SLEEP
static int cix_bus_resume(struct device *dev)
{
	struct cix_bus *bus = dev_get_drvdata(dev);
	int ret;

	ret = devfreq_event_enable_edev(bus->edev);
	if (ret < 0) {
		dev_err(dev, "failed to enable the devfreq-event devices\n");
		return ret;
	}

	ret = devfreq_resume_device(bus->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to resume the devfreq devices\n");
		return ret;
	}
	return 0;
}

static int cix_bus_suspend(struct device *dev)
{
	struct cix_bus *bus = dev_get_drvdata(dev);
	int ret;

	ret = devfreq_event_disable_edev(bus->edev);
	if (ret < 0) {
		dev_err(dev, "failed to disable the devfreq-event devices\n");
		return ret;
	}

	ret = devfreq_suspend_device(bus->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}
	return 0;
}
#endif

static const struct dev_pm_ops cix_bus_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(cix_bus_suspend, cix_bus_resume)
};

static const struct of_device_id cix_bus_of_match[] = {
	{ .compatible = "cix,bus-ci700", },
	{ .compatible = "cix,bus-ni700", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, cix_bus_of_match);

static struct platform_driver cix_bus_platdrv = {
	.probe		= cix_bus_probe,
	.shutdown	= cix_bus_shutdown,
	.driver = {
		.name	= "cix-bus",
		.pm	= &cix_bus_pm,
		.of_match_table = of_match_ptr(cix_bus_of_match),
	},
};
module_platform_driver(cix_bus_platdrv);

MODULE_DESCRIPTION("Generic Cix Bus frequency driver");
MODULE_AUTHOR("Cixtech,Inc.");
MODULE_LICENSE("GPL v2");

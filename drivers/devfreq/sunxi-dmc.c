/*
 * Allwinner GPU power domain support.
 *
 * Copyright (C) 2019 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Implementation of gpu specific power domain control which is used in
 * conjunction with runtime-pm. Support for both device-tree and non-device-tree
 * based power domain support is included.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/devfreq-event.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/suspend.h>

#define DRIVER_NAME	"devfreq Driver"

struct sunxi_dmcfreq {
	struct device *dev;
	struct devfreq *devfreq;
	struct devfreq_simple_ondemand_data ondemand_data;
	struct clk *dmc_clk;
	struct devfreq_event_dev *edev;
	struct mutex lock;

	unsigned long rate, target_rate;
	struct dev_pm_opp *curr_opp;
};

static int sunxi_dmc_target(struct device *dev,
						unsigned long *freq, u32 flags)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	unsigned long target_rate;
	struct dev_pm_opp *opp;
	int rc = 0;

	rcu_read_lock();
	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		rcu_read_unlock();
		return PTR_ERR(opp);
	}

	target_rate = dev_pm_opp_get_freq(opp);
	dmcfreq->rate = dev_pm_opp_get_freq(dmcfreq->curr_opp);
	rcu_read_unlock();

	if (dmcfreq->rate == target_rate)
		return 0;

	/* start frequency scaling */
	mutex_lock(&dmcfreq->lock);

	rc = clk_set_rate(dmcfreq->dmc_clk, target_rate);
	if (rc)
		goto out;

	usleep_range(200, 300);
	dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk);
	if (dmcfreq->rate != target_rate)
		dev_err(dev, "Get wrong ddr frequency, Request frequency %lu,\
			Current frequency %lu\n", target_rate, dmcfreq->rate);

	devfreq_event_disable_edev(dmcfreq->edev);
	devfreq_event_enable_edev(dmcfreq->edev);
	dmcfreq->curr_opp = opp;

out:
	mutex_unlock(&dmcfreq->lock);
	return rc;
}

static int sunxi_get_dev_status(struct device *dev,
				struct devfreq_dev_status *stat)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	struct devfreq_event_data edata;
	ret = devfreq_event_get_event(dmcfreq->edev, &edata);
	if (ret < 0)
		return ret;

	stat->current_frequency = dmcfreq->rate;
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int sunxi_dmcfreq_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);

	*freq = dmcfreq->rate;

	return 0;
}

static struct devfreq_dev_profile sunxi_dmcfreq_profile = {
	.polling_ms     = 100,
	.target         = sunxi_dmc_target,
	.get_dev_status = sunxi_get_dev_status,
	.get_cur_freq   = sunxi_dmcfreq_get_cur_freq,
};

static const struct of_device_id sunxi_dmcfreq_match[] = {
	{ .compatible = "allwinner,sunxi-dmc" },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_dmcfreq_match);

static int sunxi_dmcfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmcfreq *dmcfreq;
	int rc = 0;

	dmcfreq = devm_kzalloc(dev, sizeof(*dmcfreq), GFP_KERNEL);
	if (!dmcfreq)
		return -ENOMEM;

	mutex_init(&dmcfreq->lock);

	dmcfreq->dmc_clk = devm_clk_get(dev, "dram");
	if (IS_ERR(dmcfreq->dmc_clk)) {
		dev_err(&pdev->dev, "devm_clk_get error!\n");
		return PTR_ERR(dmcfreq->dmc_clk);
	}

	dmcfreq->edev = devfreq_event_get_edev_by_phandle(dev, 0);
	if (IS_ERR(dmcfreq->edev)) {
		dev_err(&pdev->dev, "event get phandle error!\n");
		return -EPROBE_DEFER;
	}

	rc = devfreq_event_enable_edev(dmcfreq->edev);
	if (rc < 0) {
		dev_err(&pdev->dev, "devfreq_event_enable_edev error!\n");
		return -ENODEV;
	}

	/*
	 * We add a devfreq driver to our parent since it has a device tree node
	 * with operating points.
	 */
	rc = dev_pm_opp_of_add_table(dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "dev_pm_opp_of_add_table error!\n");
		goto err_opp;
	}

	of_property_read_u32(np, "upthreshold",
			     &dmcfreq->ondemand_data.upthreshold);
	of_property_read_u32(np, "downdifferential",
			     &dmcfreq->ondemand_data.downdifferential);

	dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk);

	rcu_read_lock();
	dmcfreq->curr_opp = devfreq_recommended_opp(dev, &dmcfreq->rate,
					DEVFREQ_FLAG_LEAST_UPPER_BOUND);
	if (IS_ERR(dmcfreq->curr_opp)) {
		dev_err(&pdev->dev, "devfreq_recommended_opp error!\n");
		rcu_read_unlock();
		rc = PTR_ERR(dmcfreq->curr_opp);
		goto err;
	}
	rcu_read_unlock();

	/* Add devfreq device to monitor */
	dmcfreq->devfreq = devm_devfreq_add_device(dev,
						   &sunxi_dmcfreq_profile,
						   "userspace",
						   &(dmcfreq->ondemand_data));
	if (IS_ERR(dmcfreq->devfreq)) {
		dev_err(&pdev->dev, "devm_devfreq_add_device error!\n");
		rc = PTR_ERR(dmcfreq->devfreq);
		goto err;
	}
	devm_devfreq_register_opp_notifier(dev, dmcfreq->devfreq);

	dmcfreq->dev = dev;
	platform_set_drvdata(pdev, dmcfreq);

	return 0;

err:
	dev_pm_opp_of_remove_table(dev);
err_opp:
	devfreq_event_disable_edev(dmcfreq->edev);
	return rc;
}

static int sunxi_dmcfreq_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_dmcfreq *dmcfreq = platform_get_drvdata(pdev);

	dev_pm_opp_of_remove_table(dev);
	devfreq_event_disable_edev(dmcfreq->edev);
	return 0;
}

static __maybe_unused int sunxi_dmcfreq_suspend(struct device *dev)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	ret = devfreq_event_disable_edev(dmcfreq->edev);
	if (ret < 0) {
		dev_err(dev, "failed to disable the devfreq-event devices\n");
		return ret;
	}

	ret = devfreq_suspend_device(dmcfreq->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}

	return 0;
}

static __maybe_unused int sunxi_dmcfreq_resume(struct device *dev)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	ret = devfreq_event_enable_edev(dmcfreq->edev);
	if (ret < 0) {
		dev_err(dev, "failed to enable the devfreq-event devices\n");
		return ret;
	}

	ret = devfreq_resume_device(dmcfreq->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to resume the devfreq devices\n");
		return ret;
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(sunxi_dmcfreq_pm, sunxi_dmcfreq_suspend,
			 sunxi_dmcfreq_resume);

static struct platform_driver sunxi_dmcfreq_driver = {
	.probe  = sunxi_dmcfreq_probe,
	.remove  = sunxi_dmcfreq_remove,
	.driver = {
		.name = "sunxi-dmcfreq",
		.pm = &sunxi_dmcfreq_pm,
		.of_match_table = sunxi_dmcfreq_match,
	},
};

module_platform_driver(sunxi_dmcfreq_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SUNXI dmcfreq driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.0");

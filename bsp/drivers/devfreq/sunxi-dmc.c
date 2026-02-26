/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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
#include <sunxi-log.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/devfreq-event.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include "../crashdump/sunxi-crashdump.h"

#define DRIVER_NAME	"devfreq Driver"
#define DEVFREQ_EN 0x4

struct sunxi_dmcfreq {
	struct device *dev;
	struct devfreq *devfreq;
	struct devfreq_simple_ondemand_data ondemand_data;
	struct clk *dmc_clk;
	struct devfreq_event_dev *edev;
	struct mutex lock;

	int input_boost_enable;
	int input_boost_time;
	struct input_handler input_handler;
	atomic_t input_set_target_max;
	struct workqueue_struct *boost_workqueue;
	struct work_struct boost_work;
	struct timer_list boost_timer;

	unsigned long rate, target_rate;
};

static const struct input_device_id sunxi_dmcfreq_input_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
				BIT_MASK(ABS_MT_POSITION_Y) },
	},
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	},
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ },
};

static void sunxi_dmcfreq_timer_func(struct timer_list *timer)
{
	struct sunxi_dmcfreq *dmcfreq = container_of(timer, struct sunxi_dmcfreq, boost_timer);

	atomic_set(&dmcfreq->input_set_target_max, 0);
}

static void sunxi_boost_work_events(struct work_struct *work)
{
	struct sunxi_dmcfreq *dmcfreq = container_of(work, struct sunxi_dmcfreq, boost_work);

	if (!atomic_read(&dmcfreq->input_set_target_max)) {
		atomic_set(&dmcfreq->input_set_target_max, 1);
	} else {
		goto change_time;
	}

	mutex_lock(&(dmcfreq->devfreq)->lock);
	update_devfreq(dmcfreq->devfreq);
	mutex_unlock(&(dmcfreq->devfreq)->lock);

change_time:
	mod_timer(&dmcfreq->boost_timer, jiffies + msecs_to_jiffies(dmcfreq->input_boost_time));
}

static void sunxi_dmcfreq_input_event(struct input_handle *handle,
		unsigned int type,
		unsigned int code,
		int value)
{
	struct sunxi_dmcfreq *dmcfreq = handle->private;

	if (type != EV_ABS && type != EV_KEY)
		return;

	queue_work(dmcfreq->boost_workqueue, &dmcfreq->boost_work);
}

static int sunxi_dmcfreq_input_connect(struct input_handler *handler,
		struct input_dev *dev,
		const struct input_device_id *id)
{
	int error;
	struct input_handle *handle;
	struct sunxi_dmcfreq *dmcfreq = container_of(handler, struct sunxi_dmcfreq, input_handler);

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "dmcfreq";
	handle->private = dmcfreq;

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void sunxi_dmcfreq_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static void sunxi_dmcfreq_boost_init(struct sunxi_dmcfreq *dmcfreq)
{
	dmcfreq->boost_workqueue = create_singlethread_workqueue("dmcfreq_boost_workqueue");
	if (!dmcfreq->boost_workqueue) {
		sunxi_err(dmcfreq->dev, "Could not create boost workqueue\n");
		return;
	}
	flush_workqueue(dmcfreq->boost_workqueue);
	INIT_WORK(&dmcfreq->boost_work, sunxi_boost_work_events);

	timer_setup(&dmcfreq->boost_timer, sunxi_dmcfreq_timer_func, 0);

	dmcfreq->input_handler.event = sunxi_dmcfreq_input_event;
	dmcfreq->input_handler.connect = sunxi_dmcfreq_input_connect;
	dmcfreq->input_handler.disconnect = sunxi_dmcfreq_input_disconnect;
	dmcfreq->input_handler.name = "dmcfreq";
	dmcfreq->input_handler.id_table = sunxi_dmcfreq_input_ids;
	if (input_register_handler(&dmcfreq->input_handler))
		sunxi_err(dmcfreq->dev, "failed to register input handler\n");
}

static int sunxi_dmc_target(struct device *dev,
						unsigned long *freq, u32 flags)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	unsigned long target_rate;
	unsigned long last_rate_tmp;
	struct dev_pm_opp *opp;
	int rc = 0;
	unsigned int timeout;
	u64 start_time;
	u64 end_time;


	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp))
		return PTR_ERR(opp);

	if (atomic_read(&dmcfreq->input_set_target_max)) {
		target_rate = dmcfreq->devfreq->scaling_max_freq;
	} else {
		target_rate = dev_pm_opp_get_freq(opp);
	}

	//target_rate = dev_pm_opp_get_freq(opp);
	dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk);

	if (dmcfreq->rate == target_rate)
		return 0;

	start_time = ktime_get();
	last_rate_tmp = dmcfreq->rate;
	/* start frequency scaling */
	mutex_lock(&dmcfreq->lock);
	rc = clk_set_rate(dmcfreq->dmc_clk, target_rate);
	if (rc)
		goto out;

	timeout = 200;
	while ((dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk)) != target_rate) {
		if (!timeout) {
			sunxi_err(dev, "change dram clock error!\n");
			rc = -ETIMEDOUT;
			goto out;
		}

		timeout--;
		cpu_relax();
		msleep(2);
	}

	devfreq_event_disable_edev(dmcfreq->edev);
	devfreq_event_enable_edev(dmcfreq->edev);

out:
	mutex_unlock(&dmcfreq->lock);
	end_time = ktime_get();
	sunxi_get_freq_info(dev, NULL, start_time, end_time, last_rate_tmp, target_rate);
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

	stat->current_frequency = clk_get_rate(dmcfreq->dmc_clk);
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;

	return ret;
}

static int sunxi_dmcfreq_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);

	*freq = clk_get_rate(dmcfreq->dmc_clk);

	return 0;
}

static void sunxi_adjust_freq(struct device *dev, unsigned long freq, unsigned int dram_div)
{
	unsigned long freq0 = (freq << 2) / (((dram_div >> 24) & 0x1f) + 1);
	unsigned long freq1 = (freq << 2) / (((dram_div >> 16) & 0x1f) + 1);
	unsigned long freq2 = (freq << 2) / (((dram_div >> 8) & 0x1f) + 1);
	unsigned long freq3 = (freq << 2) / ((dram_div & 0x1f) + 1);

	dev_pm_opp_of_remove_table(dev);
	dev_pm_opp_add(dev, freq0, 0);
	dev_pm_opp_add(dev, freq1, 0);
	dev_pm_opp_add(dev, freq2, 0);
	dev_pm_opp_add(dev, freq3, 0);
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
	struct device_node *dram_np;
	struct sunxi_dmcfreq *dmcfreq;
	unsigned int tpr13;
	unsigned int dram_div;
	int rc = 0;

	dram_np = of_find_node_by_path("/dram");
	if (!dram_np) {
		sunxi_err(&pdev->dev, "failed to find dram node\n");
		return -ENODEV;
	}

	rc = of_property_read_u32(dram_np, "dram_para[30]", &tpr13);
	if (rc) {
		rc = of_property_read_u32(dram_np, "dram_para30", &tpr13);
		if (rc) {
			sunxi_err(&pdev->dev, "failed to find dram_div\n");
			return -ENODEV;
		}
	}
	if ((tpr13 & DEVFREQ_EN) == 0) {
		sunxi_info(&pdev->dev, "disable devfreq\n");
		return -ENODEV;
	}

	rc = of_property_read_u32(dram_np, "dram_para[24]", &dram_div);
	if (rc) {
		rc = of_property_read_u32(dram_np, "dram_para24", &dram_div);
		if (rc) {
			sunxi_err(&pdev->dev, "failed to find dram_div\n");
			return -ENODEV;
		}
	}

	dmcfreq = devm_kzalloc(dev, sizeof(*dmcfreq), GFP_KERNEL);
	if (!dmcfreq)
		return -ENOMEM;

	mutex_init(&dmcfreq->lock);

	dmcfreq->dmc_clk = devm_clk_get(dev, "dram");
	if (IS_ERR(dmcfreq->dmc_clk)) {
		sunxi_err(&pdev->dev, "devm_clk_get error!\n");
		return PTR_ERR(dmcfreq->dmc_clk);
	}

	dmcfreq->edev = devfreq_event_get_edev_by_phandle(dev, "devfreq-events", 0);
	if (IS_ERR(dmcfreq->edev)) {
		sunxi_err(&pdev->dev, "event get phandle error!\n");
		return -EPROBE_DEFER;
	}

	/*
	 * We add a devfreq driver to our parent since it has a device tree node
	 * with operating points.
	 */
	rc = dev_pm_opp_of_add_table(dev);
	if (rc < 0) {
		sunxi_err(&pdev->dev, "dev_pm_opp_of_add_table error!\n");
		goto err_opp;
	}

	of_property_read_u32(np, "upthreshold",
			     &dmcfreq->ondemand_data.upthreshold);
	of_property_read_u32(np, "downdifferential",
			     &dmcfreq->ondemand_data.downdifferential);

	dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk);
	sunxi_adjust_freq(dev, dmcfreq->rate, dram_div);
	dmcfreq->dev = dev;
	platform_set_drvdata(pdev, dmcfreq);

	/* Add devfreq device to monitor */
	dmcfreq->devfreq = devm_devfreq_add_device(dev,
						   &sunxi_dmcfreq_profile,
						   "performance",
						   &(dmcfreq->ondemand_data));
	if (IS_ERR(dmcfreq->devfreq)) {
		sunxi_err(&pdev->dev, "devm_devfreq_add_device error!\n");
		rc = PTR_ERR(dmcfreq->devfreq);
		goto err;
	}
	devm_devfreq_register_opp_notifier(dev, dmcfreq->devfreq);

	rc = devfreq_event_enable_edev(dmcfreq->edev);
	if (rc < 0) {
		sunxi_err(&pdev->dev, "devfreq_event_enable_edev error!\n");
		goto err;
	}

	/* input boost init */
	rc = of_property_read_u32(np, "input-boost-enable",
			     &dmcfreq->input_boost_enable);
	if (rc) {
		sunxi_err(NULL, "devfreq read input-boost-enable fail\n");
		dmcfreq->input_boost_enable = 0;
	}

	rc = of_property_read_u32(np, "input-boost-time",
			     &dmcfreq->input_boost_time);
	if (rc) {
		sunxi_err(NULL, "devfreq read input-boost-time fail\n");
		dmcfreq->input_boost_time = 500;
	}

	atomic_set(&dmcfreq->input_set_target_max, 0);
	if (dmcfreq->input_boost_enable) {
		sunxi_dmcfreq_boost_init(dmcfreq);
	}
	/* input boost init end*/

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

	if (dmcfreq == NULL)
		return 0;

	if (dmcfreq->input_boost_enable) {
		del_timer(&dmcfreq->boost_timer);
		destroy_workqueue(dmcfreq->boost_workqueue);
	}

	dev_pm_opp_of_remove_table(dev);
	devfreq_event_disable_edev(dmcfreq->edev);
	return 0;
}

static __maybe_unused int sunxi_dmcfreq_suspend(struct device *dev)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	if (dmcfreq == NULL)
		return 0;

	ret = devfreq_event_disable_edev(dmcfreq->edev);
	if (ret < 0) {
		sunxi_err(dev, "failed to disable the devfreq-event devices\n");
		return ret;
	}

	ret = devfreq_suspend_device(dmcfreq->devfreq);
	if (ret < 0) {
		sunxi_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}

	return 0;
}

static __maybe_unused int sunxi_dmcfreq_resume(struct device *dev)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	if (dmcfreq == NULL)
		return 0;

	ret = devfreq_event_enable_edev(dmcfreq->edev);
	if (ret < 0) {
		sunxi_err(dev, "failed to enable the devfreq-event devices\n");
		return ret;
	}

	ret = devfreq_resume_device(dmcfreq->devfreq);
	if (ret < 0) {
		sunxi_err(dev, "failed to resume the devfreq devices\n");
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

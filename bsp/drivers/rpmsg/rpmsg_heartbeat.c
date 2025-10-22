/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner rpmsg-heartbeat driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

/* #define DEBUG */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include <linux/types.h>

#define HEART_RATE		(2 * HZ)
#define TICK_INTERVAL		100

extern int sunxi_rproc_report_crash(char *name, enum rproc_crash_type type);

struct rpmsg_heartbeat {
	struct rpmsg_device *rpdev;
	uint32_t tick;
	char master[32];
	u8 sleep;
};

struct hearbeat_packet {
	char name[32];
	uint32_t tick;
};

char rproc_name[32];  /* global rproc name to report crash */

static void time_out_handler(struct timer_list *unused)
{
	sunxi_rproc_report_crash(rproc_name, RPROC_WATCHDOG);
}

static DEFINE_TIMER(rpmsg_heartbeat_timer, time_out_handler);

static int rpmsg_heartbeat_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct rpmsg_heartbeat *chip = dev_get_drvdata(&rpdev->dev);
	struct hearbeat_packet *pack = data;

	dev_dbg(&rpdev->dev, "%s heartbeat: %d\n", pack->name, pack->tick);

	if (chip->master[0] == '\0') {
		memcpy(chip->master, pack->name, 32);
		chip->master[31] = '\0';
		memcpy(rproc_name, chip->master, 32);
	}

	if (chip->sleep) {
		dev_err(&rpdev->dev, "%s invalid heartbeat\n", pack->name);
		return 0;
	}

	if (chip->tick != pack->tick)
		chip->tick = pack->tick;

	chip->tick++;
	chip->tick %= TICK_INTERVAL;
	mod_timer(&rpmsg_heartbeat_timer, jiffies + HEART_RATE);

	return 0;
}

static int rpmsg_heartbeat_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_heartbeat *chip;

	chip = devm_kzalloc(&rpdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->rpdev = rpdev;
	chip->tick = 0;

	dev_set_drvdata(&rpdev->dev, chip);
	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	return 0;
}

static void rpmsg_heartbeat_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));
}

#ifdef CONFIG_PM_SLEEP
static int sunxi_rpmsg_hearbeat_suspend(struct device *dev)
{
	struct rpmsg_heartbeat *chip = NULL;
	chip = dev_get_drvdata(dev);

	chip->sleep = 1;
	del_timer_sync(&rpmsg_heartbeat_timer);

	return 0;
}

static void sunxi_rpmsg_hearbeat_resume(struct device *dev)
{
	struct rpmsg_heartbeat *chip = NULL;
	chip = dev_get_drvdata(dev);

	chip->sleep = 0;
	mod_timer(&rpmsg_heartbeat_timer, jiffies + HEART_RATE);
}

static struct dev_pm_ops sunxi_rpmsg_hearbeat_pm_ops = {
	.prepare = sunxi_rpmsg_hearbeat_suspend,
	.complete = sunxi_rpmsg_hearbeat_resume,
};
#endif

static struct rpmsg_device_id rpmsg_driver_hearbeat_id_table[] = {
	{ .name	= "sunxi,rpmsg_heartbeat" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_hearbeat_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
	.drv = {
		.name	= KBUILD_MODNAME,
#ifdef CONFIG_PM_SLEEP
		.pm = &sunxi_rpmsg_hearbeat_pm_ops,
#endif
	},
	.id_table	= rpmsg_driver_hearbeat_id_table,
	.probe		= rpmsg_heartbeat_probe,
	.callback	= rpmsg_heartbeat_cb,
	.remove		= rpmsg_heartbeat_remove,
};
module_rpmsg_driver(rpmsg_sample_client);

MODULE_DESCRIPTION("Remote Processor Heartbeat Receive Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lijiajian <lijiajian@allwinnertech.com>");
MODULE_VERSION("1.0.0");

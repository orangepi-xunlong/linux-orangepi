/*
 * dsm_earlyboot_client.c
 *
 * init some earlyboot dsm clients
 *
 * Copyright (c) 2015-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <dsm/dsm_pub.h>

#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <huawei_platform/log/hw_log.h>

#define HWLOG_TAG	DSM_EARLYBOOT_CLIENT
HWLOG_REGIST();

static struct dsm_dev public_client_devices[] = {
	{
		.name = "dsm_apcp",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
	{
		.name = "dsm_spi",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
	{
		.name = "smartpa",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
	{
		.name = "dsm_selinux",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
#ifdef CONFIG_HUAWEI_SDCARD_VOLD
	{
		.name = "sdcard_vold",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
#endif
	{
		.name = "fbe_vold",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
	{
		.name = "dsm_e4defrag",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
	{
		.name = "dsm_uart",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
	{
		.name = "dsm_f2fsck",
		.device_name = NULL,
		.ic_name = NULL,
		.module_name = NULL,
		.fops = NULL,
		.buff_size = 1024,
	},
};

int get_spi_client(struct dsm_client **dev)
{
	*dev = dsm_find_client("dsm_spi");
	return 0;
}

int get_selinux_client(struct dsm_client **dev)
{
	*dev = dsm_find_client("dsm_selinux");
	return 0;
}

int dsm_earlyboot_client_remove(struct platform_device *dev)
{
	hwlog_info("called remove %s\n", __func__);
	return 0;
}

static int __init dsm_earlyboot_client_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(public_client_devices); i++)
		dsm_register_client(&public_client_devices[i]);
	return 0;
}

static void __exit dsm_earlyboot_client_exit(void)
{
}

late_initcall(dsm_earlyboot_client_init);
module_exit(dsm_earlyboot_client_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei dsm earlyboot client");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");

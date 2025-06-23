// SPDX-License-Identifier: GPL-2.0
/*
 * DSM driver for PM exception
 *
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/string.h>
#include "pm_exception_dsm.h"
#include "pm_exception_drv.h"

#define ERROR_LEVEL 1
#define INFO_LEVEL  1
#define DEBUG_LEVEL 0

struct dsm_pm_client {
	struct dsm_client *dsm_client;
	char *dsm_str_info_buffer;
};

static struct dsm_dev dsm_pmic_excp = {
	.name = DSM_PMIC_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_PMIC_BUF_SIZE,
};

static struct delayed_work g_get_data_wq;

static struct dsm_pm_client *dsm_pm_client_table;
static struct dsm_dev *dsm_dev_table[PM_DEVICE_MAX] = {
	&dsm_pmic_excp
};

int pm_calc_error_id(uint8_t src, uint8_t type, uint16_t id)
{
    return (DSM_PM_ERROR_START + 100000 * src + 1000 * type + id);
}

static int pm_dsm_register(struct dsm_pm_client *dsm_client,
		struct dsm_dev *dsm_dev)
{
	if (dsm_dev == NULL || dsm_client == NULL)
		return -EINVAL;

	dsm_client->dsm_client = dsm_register_client(dsm_dev);
	if (dsm_client->dsm_client == NULL) {
		dsm_client->dsm_client =
			dsm_find_client((char *)dsm_dev->name);
		if (dsm_client->dsm_client == NULL) {
			pr_err("dsm_pm_client register failed\n");
			return -ENOMEM;
		}
		pr_err("dsm_pm_client find in dsm_server\n");
	}

	dsm_client->dsm_str_info_buffer =
		kzalloc(dsm_dev->buff_size, GFP_KERNEL);
	if (!dsm_client->dsm_str_info_buffer) {
		pr_err("dsm pm %s malloc buffer failed\n",
				dsm_dev->name);
		return -ENOMEM;
	}

	return 0;
}

static void pmic_exception_work(struct work_struct *work)
{
	get_pm_exception_data();
}

static int pm_dsm_init(void)
{
	int i, ret;
	size_t size;

	if (dsm_pm_client_table == NULL) {
		size = sizeof(*dsm_pm_client_table) * PM_DEVICE_MAX;
		dsm_pm_client_table = kzalloc(size, GFP_KERNEL);
		if (dsm_pm_client_table == NULL) {
			pr_err("dsm_pm_client table malloc failed\n");
			return -ENOMEM;
		}
	}

	for (i = 0; i < PM_DEVICE_MAX; i++) {
		if (dsm_pm_client_table[i].dsm_client != NULL ||
			dsm_dev_table[i] == NULL)
			continue;

		ret = pm_dsm_register(dsm_pm_client_table + i,
				dsm_dev_table[i]);
		if (ret)
			pr_err("dsm dev %s register failed %d\n",
					dsm_dev_table[i]->name, ret);
	}

	INIT_DELAYED_WORK(&g_get_data_wq, pmic_exception_work);
	schedule_delayed_work(&g_get_data_wq, msecs_to_jiffies(10000));

	return 0;
}
static void pm_dsm_deinit(void)
{
	int i;

	if (dsm_pm_client_table == NULL)
		return;

	for (i = 0; i < PM_DEVICE_MAX; i++) {
		// kfree(NULL) is safe, check not required
		kfree(dsm_pm_client_table[i].dsm_str_info_buffer);
		dsm_pm_client_table[i].dsm_str_info_buffer = NULL;
	}

	kfree(dsm_pm_client_table);
	dsm_pm_client_table = NULL;
}

int pm_dsm_report_num(enum pm_device_type dev_type, int error_no,
		unsigned int mesg_no)
{
	int err;

	if (dsm_pm_client_table == NULL ||
		dsm_pm_client_table[dev_type].dsm_client == NULL) {
		pr_err("dsm_pm_client did not register\n");
		return -EINVAL;
	}

	err = dsm_client_ocuppy(dsm_pm_client_table[dev_type].dsm_client);
	if (err != 0) {
		pr_err("user buffer is busy\n");
		return -EBUSY;
	}

	pr_debug("report error_no=0x%x, mesg_no=0x%x\n", error_no, mesg_no);
	dsm_client_record(dsm_pm_client_table[dev_type].dsm_client,
			"Message code = 0x%x\n", mesg_no);
	dsm_client_notify(dsm_pm_client_table[dev_type].dsm_client,
			error_no);
	return 0;
}
EXPORT_SYMBOL(pm_dsm_report_num);

int pm_dsm_report_info(enum pm_device_type dev_type, int error_no,
		char *fmt, ...)
{
	int ret, err;
	va_list args;

	pr_info("begin, errorno %d, dev_type %d", error_no, dev_type);
	if ((dsm_pm_client_table == NULL) ||
		(dsm_pm_client_table[dev_type].dsm_client == NULL)) {
		pr_err("dsm_pm_client did not register\n");
		ret = -EINVAL;
		goto out;
	}

	va_start(args, fmt);
	ret = vsnprintf(dsm_pm_client_table[dev_type].dsm_str_info_buffer,
			dsm_dev_table[dev_type]->buff_size, fmt, args);
	va_end(args);

	err = dsm_client_ocuppy(dsm_pm_client_table[dev_type].dsm_client);
	if (err != 0) {
		pr_err("user buffer is busy\n");
		ret = -EBUSY;
		goto out;
	}

	pr_debug("report dsm_error_no = %d, %s\n",
			error_no,
			dsm_pm_client_table[dev_type].dsm_str_info_buffer);
	dsm_client_record(dsm_pm_client_table[dev_type].dsm_client, "%s\n",
			dsm_pm_client_table[dev_type].dsm_str_info_buffer);
	dsm_client_notify(dsm_pm_client_table[dev_type].dsm_client,
			error_no);
out:
	return ret;
}
EXPORT_SYMBOL(pm_dsm_report_info);

late_initcall(pm_dsm_init);
module_exit(pm_dsm_deinit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Cix dsm driver for PM");
MODULE_AUTHOR("Copyright 2024 Cix Technology Group Co., Ltd.");

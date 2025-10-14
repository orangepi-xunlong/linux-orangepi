// SPDX-License-Identifier: GPL-2.0
/*
 * DSM driver for PM exception
 *
 */

#include <linux/module.h>
#include "pm_exception_dsm.h"
#include "pm_exception_drv.h"
#include "../dst_print.h"

#define ERROR_LEVEL 1
#define INFO_LEVEL 1
#define DEBUG_LEVEL 0

struct dsm_pm_client {
	struct dsm_client *dsm_client;
	char *dsm_str_info_buffer;
};

#ifdef CONFIG_HISYSEVENT
struct error_no_map {
	int error_no;
	char *name;
};

static int pmic_errorno_to_str(int error_no, char *str, int buff_len);

static struct dsm_client_ops pmic_dsm_ops = {
	.poll_state = NULL,
	.dump_func = NULL,
	.errorno_to_str = pmic_errorno_to_str,
};

struct error_no_map pmic_error_no_map[] = {
    {920010001, "DSM_PM_PMIC_EXCEPTION"},
    {920010002, "DSM_PM_CHARGER_EXCEPTION"},
    {920010003, "DSM_PM_EC_EXCEPTION"},
};
#endif /* CONFIG_HISYSEVENT */

static struct dsm_dev dsm_pmic_excp = {
#ifdef CONFIG_HISYSEVENT
	.name = "POWER",
#else
	.name = DSM_PMIC_NAME,
#endif
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
#ifdef CONFIG_HISYSEVENT
	.fops = &pmic_dsm_ops,
#else
	.fops = NULL,
#endif /* CONFIG_HISYSEVENT */
	.buff_size = DSM_PMIC_BUF_SIZE,
};

static struct delayed_work g_get_data_wq;

static struct dsm_pm_client *dsm_pm_client_table;
static struct dsm_dev *dsm_dev_table[PM_DEVICE_MAX] = { &dsm_pmic_excp };

#ifdef CONFIG_HISYSEVENT
static int pmic_errorno_to_str(int error_no, char *str, int buff_len)
{
	int i;
	int audio_error_no_map_len;

	if (str == NULL)
		return -EINVAL;

	audio_error_no_map_len = sizeof(pmic_error_no_map) / sizeof(pmic_error_no_map[0]);
	for (i = 0; i < audio_error_no_map_len; i++) {
		if (pmic_error_no_map[i].error_no == error_no) {
			if (strlen(pmic_error_no_map[i].name) >= buff_len)
				return -1;

			strncpy(str, pmic_error_no_map[i].name, buff_len - 1);
			pr_debug("error_no:%d, err_str:%s\n", error_no, str);
			return 0;
		}
	}
	return -1;
}
#endif /* CONFIG_HISYSEVENT */

static int pm_dsm_register(struct dsm_pm_client *dsm_client,
			   struct dsm_dev *dsm_dev)
{
	if (dsm_dev == NULL || dsm_client == NULL)
		return -EINVAL;

	dsm_client->dsm_client = dsm_register_client(dsm_dev);
	if (dsm_client->dsm_client == NULL) {
		dsm_client->dsm_client = dsm_find_client((char *)dsm_dev->name);
		if (dsm_client->dsm_client == NULL) {
			DST_ERR("dsm_pm_client register failed\n");
			return -ENOMEM;
		}
		DST_ERR("dsm_pm_client find in dsm_server\n");
	}

	dsm_client->dsm_str_info_buffer =
		kzalloc(dsm_dev->buff_size, GFP_KERNEL);
	if (!dsm_client->dsm_str_info_buffer) {
		DST_ERR("dsm pm %s malloc buffer failed\n", dsm_dev->name);
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
			DST_ERR("dsm_pm_client table malloc failed\n");
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
			DST_ERR("dsm dev %s register failed %d\n",
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
		DST_ERR("dsm_pm_client did not register\n");
		return -EINVAL;
	}

	err = dsm_client_ocuppy(dsm_pm_client_table[dev_type].dsm_client);
	if (err != 0) {
		DST_ERR("user buffer is busy\n");
		return -EBUSY;
	}

	DST_DBG("report error_no=0x%x, mesg_no=0x%x\n", error_no, mesg_no);
	dsm_client_record(dsm_pm_client_table[dev_type].dsm_client,
			  "Message code = 0x%x\n", mesg_no);
	dsm_client_notify(dsm_pm_client_table[dev_type].dsm_client, error_no);
	return 0;
}
EXPORT_SYMBOL(pm_dsm_report_num);

int pm_dsm_report_info(enum pm_device_type dev_type, int error_no, char *fmt,
		       ...)
{
	int ret, err;
	va_list args;

	DST_PN("begin, errorno %d, dev_type %d", error_no, dev_type);
	if ((dsm_pm_client_table == NULL) ||
	    (dsm_pm_client_table[dev_type].dsm_client == NULL)) {
		DST_ERR("dsm_pm_client did not register\n");
		ret = -EINVAL;
		goto out;
	}

	va_start(args, fmt);
	ret = vsnprintf(dsm_pm_client_table[dev_type].dsm_str_info_buffer,
			dsm_dev_table[dev_type]->buff_size, fmt, args);
	va_end(args);

	err = dsm_client_ocuppy(dsm_pm_client_table[dev_type].dsm_client);
	if (err != 0) {
		DST_ERR("user buffer is busy\n");
		ret = -EBUSY;
		goto out;
	}

	DST_DBG("report dsm_error_no = %d, %s\n", error_no,
		 dsm_pm_client_table[dev_type].dsm_str_info_buffer);
	dsm_client_record(dsm_pm_client_table[dev_type].dsm_client, "%s\n",
			  dsm_pm_client_table[dev_type].dsm_str_info_buffer);
	dsm_client_notify(dsm_pm_client_table[dev_type].dsm_client, error_no);
out:
	return ret;
}
EXPORT_SYMBOL(pm_dsm_report_info);

late_initcall(pm_dsm_init);
module_exit(pm_dsm_deinit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Cix dsm driver for PM");
MODULE_AUTHOR("Copyright 2024 Cix Technology Group Co., Ltd.");

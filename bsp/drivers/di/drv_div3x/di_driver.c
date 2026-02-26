/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "../common/di_debug.h"
#include "di_driver.h"
#include "di_dev.h"
#include "di_fops.h"
#include "../common/di_utils.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/delay.h>

#define DI_MODULE_NAME "deinterlace"
#define TAG "[DI]"

#define DI_VERSION_MAJOR 1
#define DI_VERSION_MINOR 0
#define DI_VERSION_PATCHLEVEL 0

static struct di_driver_data *di_drvdata;
static unsigned int di_debug_mode;
static struct tnr_module_param_t gtnr_pqpara;

#if IS_ENABLED(CONFIG_SUNXI_DI_AMP_SERVER)

extern void sunxi_enable_device_iommu(unsigned int master_id, bool flag);
static irqreturn_t di_irq_handler(int irq, void *dev_id);
static int di_rv_start(void);
static int di_rv_stop(void);

static struct di_amp_ctrl amp_ops = {
	.rv_start = di_rv_start,
	.rv_stop = di_rv_stop,
};

static int di_rv_start(void)
{
	DI_DEBUG("di_rv_start \n");
	if (!amp_ops.rv_irq_state) {
		devm_free_irq(di_drvdata->dev, di_drvdata->irq_no, (void *) di_drvdata);
		amp_ops.rv_irq_state = true;
	}

	if (!amp_ops.iommu_need) {
		sunxi_enable_device_iommu(di_drvdata->master_id, 0);
		amp_ops.iommu_need = true;
	}

	if (!amp_ops.pm_state) {
		pm_runtime_get_sync(di_drvdata->dev);
		amp_ops.pm_state = true;
	}
	return 0;
}

static int di_rv_stop(void)
{
	int ret = 0;

	DI_DEBUG("di_rv_stop \n");
	if (amp_ops.rv_irq_state) {
		ret = devm_request_irq(di_drvdata->dev, di_drvdata->irq_no,
				di_irq_handler, 0, dev_name(di_drvdata->dev), di_drvdata);
		amp_ops.rv_irq_state = false;
	}

	if (amp_ops.iommu_need) {
		sunxi_enable_device_iommu(di_drvdata->master_id, 1);
		amp_ops.iommu_need = false;
	}

	if (amp_ops.pm_state) {
		pm_runtime_put_sync(di_drvdata->dev);
		amp_ops.pm_state = false;
	}

	return ret;
}

#endif

extern const char *__clk_get_name(const struct clk *clk);

unsigned int di_device_get_debug_mode(void)
{
	return di_debug_mode;
}

int di_drv_get_version(struct di_version *version)
{
	if (version) {
		version->version_major = DI_VERSION_MAJOR;
		version->version_minor = DI_VERSION_MINOR;
		version->version_patchlevel = DI_VERSION_PATCHLEVEL;
		version->ip_version = di_dev_get_ip_version();
		return 0;
	}
	return -EINVAL;
}

static ssize_t di_device_debug_mode_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "1:enable debug mode   0:disable debug mode\n");
	n += sprintf(buf + n, "current debug_mode:%d\n", di_debug_mode);

	return n;
}

static ssize_t di_device_debug_mode_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	char *end;

	di_debug_mode = (unsigned int)simple_strtoull(buf, &end, 0);

	return count;
}

static DEVICE_ATTR(debug_mode, S_IWUSR | S_IRUGO,
	di_device_debug_mode_show, di_device_debug_mode_store);

static ssize_t
di_device_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n,
	"echo [level] > /sys/class/deinterlace/deinterlace/debug\n");

	n += sprintf(buf + n, "level 0: disable all kinds of di logs\n");
	n += sprintf(buf + n, "level 1: enable error di logs\n");
	n += sprintf(buf + n, "level 2: enable info di logs\n");
	n += sprintf(buf + n, "level 3: enable debug di logs\n");
	n += sprintf(buf + n, "level 4: enable debug di time detect logs\n");
	n += sprintf(buf + n, "level 5: enable film mode detect logs\n");

	n += sprintf(buf + n, "\nNow the debug level is:%d\n", debug_mask);

	return n;
}

static ssize_t
di_device_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int val;
	char *end;

	retval = count;
	val = simple_strtoull(buf, &end, 0);

	if (val < DEBUG_LEVEL_MAX) {
		debug_mask = val;
	} else {
		pr_err("ERROR: invalid input log level:%u\n", val);
		retval = -EINVAL;
	}

	return retval;
}

static DEVICE_ATTR(debug, S_IWUSR | S_IRUGO,
	di_device_debug_show, di_device_debug_store);

static ssize_t
di_device_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct di_driver_data *data = di_drvdata;
	ssize_t n = 0;

	n += sprintf(buf + n, "DI Current Info:\n");
	n += sprintf(buf + n, "irq_no:%u\n", data->irq_no);
	n += sprintf(buf + n, "dev enable:%u pm_state:%s\n",
		data->dev_enable, data->pm_state ? "suspend" : "resume");
	n += sprintf(buf + n, "need_apply_fixed_para:%u\n",
		data->need_apply_fixed_para);
	n += sprintf(buf + n, "driver state:%s\n",
		data->state ? "busy" : "idle");
	return n;
}

static DEVICE_ATTR(info, S_IWUSR | S_IRUGO,
	di_device_info_show, NULL);

static char debug_client_name[30];

static ssize_t dump_client_info(struct di_client *client, char *buf)
{
	ssize_t n = 0;

	if (!client)
		return 0;
	n += sprintf(buf + n, "clients:%s basic info:\n", client->name);
	n += sprintf(buf + n, "di_mode:%s\n",
		client->mode == DI_MODE_60HZ ? "60HZ" :
		client->mode == DI_MODE_30HZ ? "30HZ" :
		client->mode == DI_MODE_BOB ? "bob" :
		client->mode == DI_MODE_WEAVE ? "weave" :
		client->mode == DI_MODE_TNR ? "only tnr" : "Unknowed");
	n += sprintf(buf + n, "proc_fb_seqno:%llu\n", client->proc_fb_seqno);

	n += sprintf(buf + n, "di_detect_result:%s\n",
		client->di_detect_result ? "progressive" : "interlace");
	n += sprintf(buf + n, "interlace_detected_counts:%llu\n",
		client->interlace_detected_counts);
	n += sprintf(buf + n, "lastest_interlace_detected_frame:%llu\n",
		client->lastest_interlace_detected_frame);
	n += sprintf(buf + n, "progressive_detected_counts:%llu\n",
		client->progressive_detected_counts);
	n += sprintf(buf + n, "progressive_detected_first_frame:%llu\n",
		client->progressive_detected_first_frame);
	n += sprintf(buf + n, "lastest_progressive_detected_frame:%llu\n",
		client->lastest_progressive_detected_frame);
	n += sprintf(buf + n, "warning!!! detection:interlace_detected_counts_exceed_first_progressive_frame:%llu\n",
		client->interlace_detected_counts_exceed_first_p_frame);
	return n;
}
static ssize_t di_device_client_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct di_client *client;
	bool find = false;
	struct di_driver_data *drvdata = di_drvdata;

	n += sprintf(buf + n, "All of the di clients name:\n");
	list_for_each_entry(client, &drvdata->clients, node)
		n += sprintf(buf + n, "%s\n", client->name);
	n += sprintf(buf + n, "\n\n");

	list_for_each_entry(client, &drvdata->clients, node) {
		n += dump_client_info(client, buf + n);
		if (!strcmp(client->name, debug_client_name)) {
			find = true;
			break;
		}
	}

	if (!find) {
		n += sprintf(buf + n, "Wrong debug_client_name:%s, please ",
			debug_client_name);
		n += sprintf(buf + n,
		 "echo [client_name] > /sys/class/deinterlace/deinterlace/client\n");
		return n;
	}

	n += sprintf(buf + n, "%s info\n",
			debug_client_name);

	return n;
}

static ssize_t
di_device_client_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct di_client *client;
	bool find = false;
	struct di_driver_data *drvdata = di_drvdata;
	char name[30] = { 0 };

	memcpy(name, buf, count);

	name[count - 1] = '\0';

	list_for_each_entry(client, &drvdata->clients, node) {
		if (!strcmp(name, client->name)) {
			find = true;
			break;
		}
	}

	if (!find) {
		DI_ERR("ERROR client name input:%s\n", buf);
		return count;
	}

	strcpy(debug_client_name, name);

	DI_INFO("set the debug client name:%s\n", debug_client_name);

	return count;
}

static DEVICE_ATTR(client, S_IWUSR | S_IRUGO,
	di_device_client_show, di_device_client_store);

static ssize_t di_device_timeout_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct di_client *client;
	bool find = false;
	struct di_driver_data *drvdata = di_drvdata;


	n += sprintf(buf + n, "debug_client_name:%s\n", debug_client_name);

	list_for_each_entry(client, &drvdata->clients, node) {
		if (!strcmp(client->name, debug_client_name)) {
			find = true;
			n += sprintf(buf + n, "wait4start:%lld  wait4finish:%lld\n",
					client->timeout.wait4start, client->timeout.wait4finish);
			break;
		}
	}

	if (!find) {
		n += sprintf(buf + n, "Wrong debug_client_name:%s, please ",
				debug_client_name);
		n += sprintf(buf + n,
		 "echo [client_name] > /sys/class/deinterlace/deinterlace/client\n");
	}

	return n;
}

static ssize_t
di_device_timeout_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t n = 0;
	u64 start, finish;
	char *end;
	struct di_client *client;
	bool find = false;
	struct di_driver_data *drvdata = di_drvdata;


	DI_INFO("debug_client_name:%s\n", debug_client_name);

	list_for_each_entry(client, &drvdata->clients, node) {
		if (!strcmp(client->name, debug_client_name)) {
			find = true;
			start = simple_strtoull(buf, &end, 0);
			DI_INFO("set timeout, wait2start:%lld\n", start);

			if ((*end != ' ') && (*end != ',')) {
				pr_err("error separator:%c\n", *end);
				return count;
			}

			finish = simple_strtoull(end + 1, &end, 0);
			DI_INFO("set timeout, wait2finish:%lld\n", finish);

			client->timeout.wait4start = start;
			client->timeout.wait4finish = finish;

			DI_INFO("set timeout wait4start:%lld  wait4finish:%lld\n",
				client->timeout.wait4start, client->timeout.wait4finish);
			break;
		}
	}

	if (!find) {
		DI_INFO("Wrong debug_client_name:%s, please ", debug_client_name);
		DI_INFO("echo [client_name] > /sys/class/deinterlace/deinterlace/client\n");
		return n;
	}

	return count;
}

static DEVICE_ATTR(timeout, S_IWUSR | S_IRUGO,
	di_device_timeout_show, di_device_timeout_store);

static ssize_t di_device_tnrmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct di_client *client;
	bool find = false;
	struct di_driver_data *drvdata = di_drvdata;


	n += sprintf(buf + n, "debug_client_name:%s\n", debug_client_name);

	list_for_each_entry(client, &drvdata->clients, node) {
		if (!strcmp(client->name, debug_client_name)) {
			find = true;
			n += sprintf(buf + n, "TNR mode:%u  level:%u\n",
				client->tnr_mode.mode, client->tnr_mode.level);
			break;
		}
	}

	if (!find) {
		n += sprintf(buf + n, "Wrong debug_client_name:%s, please ",
				debug_client_name);
		n += sprintf(buf + n,
		 "echo [client_name] > /sys/class/deinterlace/deinterlace/client\n");
	}

	return n;
}

static ssize_t
di_device_tnrmode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t n = 0;
	unsigned int mode, level;
	char *end;
	struct di_client *client;
	bool find = false;
	struct di_driver_data *drvdata = di_drvdata;


	DI_INFO("debug_client_name:%s\n", debug_client_name);

	list_for_each_entry(client, &drvdata->clients, node) {
		if (!strcmp(client->name, debug_client_name)) {
			find = true;
			mode = (unsigned int)simple_strtoull(buf, &end, 0);

			if ((*end != ' ') && (*end != ',')) {
				pr_err("error separator:%c\n", *end);
				return count;
			}

			level = (unsigned int)simple_strtoull(end + 1, &end, 0);

			client->tnr_mode.mode = mode;
			client->tnr_mode.level = level;

			DI_INFO("TNR mode:%d  level:%d\n",
				client->tnr_mode.mode, client->tnr_mode.level);
			break;
		}
	}

	if (!find) {
		DI_INFO("Wrong debug_client_name:%s, please ", debug_client_name);
		DI_INFO("echo [client_name] > /sys/class/deinterlace/deinterlace/client\n");
		return n;
	}

	return count;
}

static DEVICE_ATTR(tnrmode, S_IWUSR | S_IRUGO,
	di_device_tnrmode_show, di_device_tnrmode_store);

static struct attribute *di_device_attrs[] = {
	 &dev_attr_debug_mode.attr,
	&dev_attr_debug.attr,
	&dev_attr_info.attr,
	&dev_attr_client.attr,
	&dev_attr_timeout.attr,
	&dev_attr_tnrmode.attr,
	NULL
};

static struct attribute_group di_device_attr_group = {
	.attrs	= di_device_attrs,
};

static const struct attribute_group *di_device_attr_groups[] = {
	&di_device_attr_group,
	NULL
};

static int di_init_hw(struct di_driver_data *drvdata)
{
	di_dev_set_reg_base(drvdata->reg_base);
	return 0;
}

static int di_clk_enable(struct di_driver_data *drvdata)
{
	long rate = 0;
	int ret = 0;

	if (!IS_ERR_OR_NULL(drvdata->iclk) && !IS_ERR_OR_NULL(drvdata->clk_bus)
	    && !IS_ERR_OR_NULL(drvdata->rst_bus_di)) {

		if (!IS_ERR_OR_NULL(drvdata->rst_bus_desys)) {
			ret = reset_control_deassert(drvdata->rst_bus_desys);
			if (ret) {
				DI_ERR(TAG"reset_control_deassert desys failed");
				return -1;
			}
		}

		ret = reset_control_deassert(drvdata->rst_bus_di);
		if (ret) {
			DI_ERR(TAG"reset_control_deassert bus_di failed");
			return -1;
		}

		rate = clk_round_rate(drvdata->iclk, drvdata->iclk_freq);
		if (clk_set_rate(drvdata->iclk, rate)) {
			DI_ERR(TAG"clk_set_rate failed");
			return -1;
		}

		DI_DEBUG("DI clk_freq=%u iclk(%s) clk_bus(%s)\n",
					(unsigned)clk_get_rate(drvdata->iclk),
					__clk_get_name(drvdata->iclk),
					__clk_get_name(drvdata->clk_bus));

		ret = clk_prepare_enable(drvdata->clk_bus);
		if (ret) {
			DI_ERR(TAG"clk_prepare_enable clk_bus failed");
			return -1;
		}

		ret = clk_prepare_enable(drvdata->iclk);
		if (ret) {
			DI_ERR(TAG"clk_prepare_enable iclk failed!\n");
			return ret;
		}
	} else {
		DI_ERR(TAG"di clk handle is invalid for enable\n");
		return -1;
	}
	return 0;
}

static int di_clk_disable(struct di_driver_data *drvdata)
{
	int ret;

	if (!IS_ERR_OR_NULL(drvdata->iclk) && !IS_ERR_OR_NULL(drvdata->clk_bus)
	    && !IS_ERR_OR_NULL(drvdata->rst_bus_di)) {
		clk_disable_unprepare(drvdata->iclk);
		clk_disable_unprepare(drvdata->clk_bus);

		ret = reset_control_assert(drvdata->rst_bus_di);
		if (ret) {
			DI_ERR(TAG"reset_control_assert rst_bus_di failed");
			return -1;
		}

		if (!IS_ERR_OR_NULL(drvdata->rst_bus_desys)) {
			ret = reset_control_assert(drvdata->rst_bus_desys);
			if (ret) {
				DI_ERR(TAG"reset_control_assert desys failed");
				return -1;
			}
		}
	} else {
		DI_ERR(TAG"di clk handle is invalid!");
		return -1;
	}

	return 0;
}

static int di_check_enable_device_locked(
	struct di_driver_data *drvdata)
{
	int ret = 0;

	DI_DEBUG(TAG"client_cnt=%d, pm_state=%d, dev_en=%d\n",
		drvdata->client_cnt, drvdata->pm_state, drvdata->dev_enable);

	if (drvdata->pm_state == DI_PM_STATE_SUSPEND)
		return 0;

	if ((drvdata->client_cnt > 0)
		&& (drvdata->dev_enable == false)) {
		ret = pm_runtime_get_sync(drvdata->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(drvdata->dev);
			return ret;
		}

		drvdata->dev_enable = true;
		di_dev_enable_irq(DI_IRQ_FLAG_PROC_FINISH, 1);
	} else if ((drvdata->client_cnt == 0)
		&& (drvdata->dev_enable == true)) {
		di_dev_enable_irq(DI_IRQ_FLAG_PROC_FINISH, 0);
		ret = pm_runtime_put_sync(drvdata->dev);
		if (ret < 0) {
			pm_runtime_get_noresume(drvdata->dev);
			return ret;
		}

		drvdata->dev_enable = false;
	} else if (drvdata->client_cnt < 0) {
		DI_ERR(TAG"err client_cnt=%d\n", drvdata->client_cnt);
		return -EINVAL;
	}

	return 0;
}

bool di_drv_is_valid_client(struct di_client *c)
{
	struct di_driver_data *drvdata = di_drvdata;
	struct di_client *client;
	bool valid = false;

	if (c) {
		mutex_lock(&drvdata->mlock);
		list_for_each_entry(client, &drvdata->clients, node) {
			if (client == c) {
				valid = true;
				break;
			}
		}
		mutex_unlock(&drvdata->mlock);
	}

	if (!valid)
		DI_ERR("invalid client[0x%p]\n", c);

	return valid;
}

int di_drv_client_inc(struct di_client *c)
{
	struct di_driver_data *drvdata = di_drvdata;
	int client_cnt;

	mutex_lock(&drvdata->mlock);
	client_cnt = drvdata->client_cnt + 1;
	if (client_cnt > DI_CLIENT_CNT_MAX) {
		mutex_unlock(&drvdata->mlock);
		DI_ERR(TAG"%s: %d > max_clients[%d]\n",
			__func__, client_cnt, DI_CLIENT_CNT_MAX);
		return -EINVAL;
	}
	drvdata->client_cnt = client_cnt;
	list_add_tail(&c->node, &drvdata->clients);
	di_check_enable_device_locked(drvdata);
	mutex_unlock(&drvdata->mlock);

	return 0;
}

int di_drv_client_dec(struct di_client *c)
{
	struct di_driver_data *drvdata = di_drvdata;
	int client_cnt;

	mutex_lock(&drvdata->mlock);
	list_del(&c->node);
	if (drvdata->pre_client == c) {
		drvdata->pre_client = NULL;
		drvdata->need_apply_fixed_para = true;
	}
	client_cnt = drvdata->client_cnt;
	if (client_cnt > 0) {
		drvdata->client_cnt--;
		di_check_enable_device_locked(drvdata);
	} else {
		mutex_unlock(&drvdata->mlock);
		DI_INFO(TAG"%s:client_cnt=%d\n", __func__, client_cnt);
		return -EINVAL;
	}
	mutex_unlock(&drvdata->mlock);

	return 0;
}

int di_drv_clients_set_tnrpara(struct di_client *client, struct tnr_module_param_pqd *data, int update)
{
	struct di_driver_data *drvdata = di_drvdata;
	struct di_client *client_i;
	int ret = -1;

	ret = di_dev_set_tnrpara((void *)client, data);
	if (ret != 0) {
		DI_ERR("%s, di_dev_set_tnrpara process fail\n", __func__);
		return ret;
	}

	/* save PQTool TNR parament and update to all clients */
	if (update == 1) {
		mutex_lock(&drvdata->mlock);
		memcpy(&client->tnr_pqpara.qptool_para, data, sizeof(struct tnr_module_param_pqd));
		memcpy(&gtnr_pqpara, &client->tnr_pqpara, sizeof(client->tnr_pqpara));
		list_for_each_entry(client_i, &drvdata->clients, node) {
			memcpy(&client_i->tnr_pqpara, &gtnr_pqpara, sizeof(gtnr_pqpara));
		}
		mutex_unlock(&drvdata->mlock);
	}

	return 0;
}

int di_drv_get_global_tnr_pqpara(struct tnr_module_param_t *data)
{
	if (gtnr_pqpara.qptool_para.id == TNR_PQTOOL_ID) {
		memcpy(data, &gtnr_pqpara, sizeof(gtnr_pqpara));
		return 0;
	} else {
		return -1;
	}
}

static int di_drv_wait2start(
	struct di_driver_data *drvdata, struct di_client *c)
{
	const u64 wait2start = c->timeout.wait4start;
	long ret = 0;
	unsigned long flags;
	u32 id;
	u32 wait_con;

	spin_lock_irqsave(&drvdata->queue_lock, flags);

	if (drvdata->task_cnt >= DI_TASK_CNT_MAX) {
		spin_unlock_irqrestore(&drvdata->queue_lock, flags);
		DI_ERR(TAG"too many tasks %d\n", drvdata->task_cnt);
		return -EBUSY;
	}

	id = (drvdata->front + drvdata->task_cnt) % DI_TASK_CNT_MAX;
	drvdata->queue[id] = c;
	drvdata->task_cnt++;
	if (drvdata->state == DI_DRV_STATE_IDLE) {
		drvdata->state = DI_DRV_STATE_BUSY;
		atomic_set(&c->wait_con, DI_PROC_STATE_2START);
		spin_unlock_irqrestore(&drvdata->queue_lock, flags);
		return 0;
	}

	if (wait2start == 0) {
		drvdata->queue[id] = NULL;
		drvdata->task_cnt--;
		spin_unlock_irqrestore(&drvdata->queue_lock, flags);
		DI_ERR(TAG"wait4start=%lluns too short to wait\n",
			wait2start);
		return -ETIMEDOUT;
	}
	atomic_set(&c->wait_con, DI_PROC_STATE_WAIT2START);

	spin_unlock_irqrestore(&drvdata->queue_lock, flags);

	ret = wait_event_interruptible_hrtimeout(c->wait,
		atomic_read(&c->wait_con) == DI_PROC_STATE_2START,
		ns_to_ktime(wait2start));

	if (atomic_read(&c->wait_con) != DI_PROC_STATE_2START) {
		spin_lock_irqsave(&drvdata->queue_lock, flags);
		wait_con = atomic_read(&c->wait_con); /* check-again */
		if (wait_con != DI_PROC_STATE_2START) {
			drvdata->queue[id] = NULL;
			drvdata->task_cnt--;
			spin_unlock_irqrestore(&drvdata->queue_lock, flags);
			DI_ERR(TAG"wait2start(%lluns) fail, con=%u, ret(%ld)\n",
				wait2start, wait_con, ret);
			return -ETIMEDOUT;
		}
		spin_unlock_irqrestore(&drvdata->queue_lock, flags);
	}

	return 0;
}

static int di_drv_wait4finish(
	struct di_driver_data *drvdata, struct di_client *c)
{
	long ret = 0;
	unsigned long flags;
	int wait_con;
	const u64 wait4finish = c->timeout.wait4finish;

	ret = wait_event_interruptible_hrtimeout(c->wait,
		atomic_read(&c->wait_con) != DI_PROC_STATE_WAIT4FINISH,
		ns_to_ktime(wait4finish));

	if (atomic_read(&c->wait_con) != DI_PROC_STATE_FINISH) {
		spin_lock_irqsave(&drvdata->queue_lock, flags);
		wait_con = atomic_read(&c->wait_con); /* check-again */
		if (wait_con == DI_PROC_STATE_WAIT4FINISH) {
			di_dev_reset();
			di_dev_query_state_with_clear(DI_IRQ_STATE_PROC_FINISH);
			drvdata->queue[drvdata->front] = NULL;
			drvdata->front = (drvdata->front + 1) % DI_TASK_CNT_MAX;
			drvdata->task_cnt--;
			drvdata->state = DI_DRV_STATE_IDLE;
		}
		spin_unlock_irqrestore(&drvdata->queue_lock, flags);

		if (wait_con == DI_PROC_STATE_WAIT4FINISH) {
			DI_ERR(TAG"wait4finish(%lluns) timeout, ret=%ld\n",
				wait4finish, ret);
			return ret ? ret : -ETIME;
		} else if (wait_con != DI_PROC_STATE_FINISH) {
			DI_ERR(TAG"wait4finish(%lluns) err, ret=%ld, con=%u\n",
				wait4finish, ret, wait_con);
			return ret ? ret : -wait_con;
		}
	}

	DI_DEBUG("Processing frame %llu\n", c->proc_fb_seqno);
	c->proc_fb_seqno++;
	return 0;
}

static void di_drv_start(struct di_driver_data *drvdata, struct di_client *c)
{
	unsigned long flags;

	spin_lock_irqsave(&drvdata->queue_lock, flags);
	atomic_set(&c->wait_con, DI_PROC_STATE_WAIT4FINISH);
	di_dev_start(1);
	spin_unlock_irqrestore(&drvdata->queue_lock, flags);
}

static void di_drv_survey_spot(
	struct di_driver_data *drvdata, struct di_client *c)
{
	struct di_client *pre_client = NULL;

	mutex_lock(&drvdata->mlock);

	if (((drvdata->pre_client == NULL)
		&& (drvdata->need_apply_fixed_para == false))
		|| (drvdata->pre_client == c))
		goto out;

	pre_client = drvdata->pre_client;
	if (pre_client) {
		if ((pre_client->proc_fb_seqno > 0)
			&& (pre_client->para_checked == true))
			di_dev_save_spot(pre_client);
	}
	di_dev_restore_spot(c);
	c->apply_fixed_para = true;

out:
	drvdata->pre_client = c;
	drvdata->need_apply_fixed_para = false;
	mutex_unlock(&drvdata->mlock);
}

/* caller must make sure c is valid */
int di_drv_process_fb(struct di_client *c)
{
	struct di_driver_data *drvdata = di_drvdata;
	int ret = 0;

	ret = di_drv_wait2start(drvdata, c);
	if (ret)
		return ret;

	di_drv_survey_spot(drvdata, c);
	if (unlikely(c->apply_fixed_para)) {
		c->apply_fixed_para = false;
		di_dev_apply_fixed_para(c);
	}
	ret = di_dev_apply_para(c);
	di_dev_dump_reg_value();
	di_drv_start(drvdata, c);

	ret |= di_drv_wait4finish(drvdata, c);

	return ret;
}

static irqreturn_t di_irq_handler(int irq, void *dev_id)
{
	struct di_driver_data *drvdata = dev_id;
	struct di_client *c;
	int wait_con;
	u32 hw_state;

	if (irq != drvdata->irq_no)
		return IRQ_NONE;

	spin_lock(&drvdata->queue_lock);

	hw_state = di_dev_query_state_with_clear(DI_IRQ_STATE_PROC_FINISH);

	if (drvdata->task_cnt == 0)
		goto irq_out;

	c = drvdata->queue[drvdata->front];
	wait_con = atomic_read(&c->wait_con);
	if (wait_con == DI_PROC_STATE_WAIT4FINISH) {
		if (hw_state & DI_IRQ_STATE_PROC_FINISH) {
			di_dev_get_proc_result(c);
			atomic_set(&c->wait_con, DI_PROC_STATE_FINISH);
			wake_up_interruptible(&c->wait);
		} else {
			di_dev_reset();
			atomic_set(&c->wait_con, DI_PROC_STATE_FINISH_ERR);
			wake_up_interruptible(&c->wait);
		}
		drvdata->queue[drvdata->front] = NULL;
		drvdata->task_cnt--;
		drvdata->state = DI_DRV_STATE_IDLE;

		if (drvdata->task_cnt == 0)
			goto irq_out;

		drvdata->front = (drvdata->front + 1) % DI_TASK_CNT_MAX;
		c = drvdata->queue[drvdata->front];
		wait_con = atomic_read(&c->wait_con);
	}

	if (wait_con == DI_PROC_STATE_WAIT2START) {
		atomic_set(&c->wait_con, DI_PROC_STATE_2START);
		drvdata->state = DI_DRV_STATE_BUSY;
		wake_up_interruptible(&c->wait);
	}

irq_out:
	spin_unlock(&drvdata->queue_lock);

	return IRQ_HANDLED;
}

/* unload resources of di device */
static void di_unload_resource(struct di_driver_data *drvdata)
{
	if (drvdata->reg_base)
		iounmap(drvdata->reg_base);

	if (drvdata->irq_no != 0) {
		DI_DEBUG(TAG"should ummap irq[%d]...\n", drvdata->irq_no);
		devm_free_irq(drvdata->dev, drvdata->irq_no, drvdata);
	}

	if (!IS_ERR_OR_NULL(drvdata->iclk))
		clk_put(drvdata->iclk);
}

/* parse and load resources of di device */
static int di_parse_dt(struct platform_device *pdev,
	struct di_driver_data *drvdata)
{
	int ret = 0;
	struct device_node *node = pdev->dev.of_node;
#if IS_ENABLED(CONFIG_SUNXI_DI_AMP_SERVER)
	u32 data[3];
#endif

	/* clk */
	drvdata->iclk = of_clk_get(node, 0);
	if (IS_ERR_OR_NULL(drvdata->iclk)) {
		DI_ERR(TAG"get di clock failed!\n");
		ret = -1;
		if (IS_ERR(drvdata->iclk))
			ret = PTR_ERR(drvdata->iclk);
		goto err_out;
	}

	drvdata->clk_bus = of_clk_get(node, 1);
	if (IS_ERR_OR_NULL(drvdata->clk_bus)) {
		DI_ERR(TAG"get clk bus clock failed!\n");
		ret = -1;
		if (IS_ERR(drvdata->clk_bus))
			ret = PTR_ERR(drvdata->clk_bus);
		goto err_out;
	}

	ret = of_property_read_u32(node, "clock-frequency", &drvdata->iclk_freq);
	if (ret) {
		DI_ERR("Failed to get di clock frequency\n");
		goto err_out;
	}

	/* reset control */
	drvdata->rst_bus_di = devm_reset_control_get(&pdev->dev, "rst_bus_di");
	if (IS_ERR(drvdata->rst_bus_di)) {
		DI_ERR(TAG"get di bus reset control failed!\n");
		ret = -1;
		if (IS_ERR(drvdata->rst_bus_di))
			ret = PTR_ERR(drvdata->rst_bus_di);
		goto err_out;
	}

	drvdata->rst_bus_desys = devm_reset_control_get_optional_shared(&pdev->dev,
							       "rst_bus_desys");
	if (IS_ERR(drvdata->rst_bus_desys)) {
		DI_INFO("desys_reset get failed, Maybe not need\n");
		drvdata->rst_bus_desys = NULL;
	}

	/* irq */
	drvdata->irq_no = irq_of_parse_and_map(node, 0);
	if (drvdata->irq_no == 0) {
		DI_ERR(TAG"platform_get_irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

#if !IS_ENABLED(CONFIG_SUNXI_DI_AMP_SERVER)
	ret = devm_request_irq(&pdev->dev, drvdata->irq_no,
		di_irq_handler, 0, dev_name(&pdev->dev), drvdata);
	if (ret) {
		DI_ERR(TAG"devm_request_irq failed\n");
		goto err_out;
	}
	DI_DEBUG(TAG"di irq_no=%u\n", drvdata->irq_no);
#else
	amp_ops.rv_irq_state = true;
	if (of_property_read_variable_u32_array(node, "iommus", data, 0, ARRAY_SIZE(data)) < 0) {
		drvdata->master_id = -1;
	} else {
		drvdata->master_id = data[1];
	}
	DI_DEBUG("get master id %d \n", drvdata->master_id);
	amp_ops.iommu_need = false;
#endif

	/* reg */
	drvdata->reg_base = of_iomap(node, 0);
	if (!drvdata->reg_base) {
		DI_ERR(TAG"of_iomap failed\n");
		ret =  -ENOMEM;
		goto err_out;
	}
	DI_DEBUG(TAG"di reg_base=0x%p\n", drvdata->reg_base);

err_out:
	return ret;
}

const struct file_operations di_fops = {
	.owner = THIS_MODULE,
	.open = di_open,
	.release = di_release,
	.unlocked_ioctl = di_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = di_ioctl,
#endif
	.mmap = di_mmap,
};

static int di_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *node = pdev->dev.of_node;
	struct di_driver_data *drvdata = NULL;

	DI_INFO("DI probe");

	if (!of_device_is_available(node)) {
		DI_ERR(TAG"DEINTERLACE device is not configed\n");
		return -ENODEV;
	}

	drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL) {
		DI_ERR(TAG"kzalloc for drvdata failed\n");
		return -ENOMEM;
	}

	ret = di_parse_dt(pdev, drvdata);
	if (ret)
		goto probe_done;

	di_utils_set_dma_dev(&pdev->dev);

	ret = di_init_hw(drvdata);
	if (ret)
		goto probe_done;

	mutex_init(&drvdata->mlock);
	INIT_LIST_HEAD(&drvdata->clients);
	spin_lock_init(&drvdata->queue_lock);

	alloc_chrdev_region(&drvdata->devt, 0, 1, DI_MODULE_NAME);
	drvdata->pcdev = cdev_alloc();
	cdev_init(drvdata->pcdev, &di_fops);
	drvdata->pcdev->owner = THIS_MODULE;
	ret = cdev_add(drvdata->pcdev, drvdata->devt, 1);
	if (ret) {
		DI_ERR(TAG"cdev add major(%d).\n", MAJOR(drvdata->devt));
		goto probe_done;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	drvdata->pclass = class_create(DI_MODULE_NAME);
#else
	drvdata->pclass = class_create(THIS_MODULE, DI_MODULE_NAME);
#endif
	if (IS_ERR(drvdata->pclass)) {
		DI_ERR(TAG"create class error\n");
		ret = PTR_ERR(drvdata->pclass);
		goto probe_done;
	}

	drvdata->pdev = device_create_with_groups(
			drvdata->pclass,  NULL, drvdata->devt,
			NULL, di_device_attr_groups,
			DI_MODULE_NAME);
	if (IS_ERR(drvdata->pdev)) {
		DI_ERR(TAG"device_create error\n");
		ret = PTR_ERR(drvdata->pdev);
		goto probe_done;
	}

	drvdata->dev = &pdev->dev;
	di_drvdata = drvdata;
	platform_set_drvdata(pdev, (void *)drvdata);
	pm_runtime_enable(&pdev->dev);

#if IS_ENABLED(CONFIG_SUNXI_DI_AMP_SERVER)
	DI_DEBUG("register rpmsg \n");

	if (di_drvdata->master_id > 0) {
		amp_deinterlace_init(&amp_ops);
	}
	amp_ops.pm_state = true;
#endif
	return 0;

probe_done:
	if (ret) {
		di_dev_exit();
		di_unload_resource(drvdata);
		kfree(drvdata);
		dev_err(&pdev->dev, "probe failed, errno %d!\n", ret);
	}

	return ret;

}

static int di_remove(struct platform_device *pdev)
{
	struct di_driver_data *drvdata;

	dev_info(&pdev->dev, "%s\n", __func__);

	drvdata = platform_get_drvdata(pdev);
	if (drvdata != NULL) {
		platform_set_drvdata(pdev, NULL);
		di_drvdata = NULL;

		if (drvdata->client_cnt > 0)
			DI_ERR(TAG"still has client_cnt=%d\n",
				drvdata->client_cnt);

		device_destroy(drvdata->pclass, drvdata->devt);
		class_destroy(drvdata->pclass);
		cdev_del(drvdata->pcdev);
		unregister_chrdev_region(drvdata->devt, 1);

		di_dev_exit();
		pm_runtime_disable(drvdata->dev);
		di_unload_resource(drvdata);

		kfree(drvdata);

#if IS_ENABLED(CONFIG_SUNXI_DI_AMP_SERVER)
		DI_DEBUG("exit rpmsg \n");
		if (di_drvdata->master_id > 0) {
			amp_deinterlace_exit();
		}
#endif
	}

	return 0;
}

static int di_suspend(struct device *dev)
{
	struct di_driver_data *drvdata = dev_get_drvdata(dev);
	int ret;

	if (drvdata != NULL) {
		if (drvdata->state == DI_DRV_STATE_BUSY)
			DI_INFO(TAG"drv busy on suspend !\n");

		mutex_lock(&drvdata->mlock);
		drvdata->pm_state = DI_PM_STATE_SUSPEND;
		if (drvdata->dev_enable == true) {
			if (drvdata->pre_client)
				di_dev_save_spot(drvdata->pre_client);
			di_dev_enable_irq(DI_IRQ_FLAG_PROC_FINISH, 0);
			ret = pm_runtime_put_sync(dev);
			if (ret) {
				pm_runtime_get_noresume(dev);
				mutex_unlock(&drvdata->mlock);
				return ret;
			}

			drvdata->dev_enable = false;
		}
		mutex_unlock(&drvdata->mlock);
	}

	return 0;
}

static int di_resume(struct device *dev)
{
	struct di_driver_data *drvdata = dev_get_drvdata(dev);
	int ret;

	if (drvdata != NULL) {
		mutex_lock(&drvdata->mlock);
		if (drvdata->client_cnt > 0) {
			ret = pm_runtime_get_sync(dev);
			if (ret < 0) {
				pm_runtime_put_noidle(dev);
				mutex_unlock(&drvdata->mlock);
				return ret;
			}
			drvdata->dev_enable = true;
			di_dev_enable_irq(DI_IRQ_FLAG_PROC_FINISH, 1);
			if (drvdata->pre_client) {
				di_dev_restore_spot(drvdata->pre_client);
				drvdata->pre_client->apply_fixed_para = true;
			}
		}
		drvdata->pm_state = DI_PM_STATE_RESUME;
		mutex_unlock(&drvdata->mlock);
	}

	return 0;
}

static int di_runtime_suspend(struct device *dev)
{
	struct di_driver_data *drvdata = dev_get_drvdata(dev);
	int ret = 0;

	if (drvdata != NULL) {
		ret = di_clk_disable(drvdata);
	} else {
		ret = -EFAULT;
		DI_ERR("drvdata is NULL!");
	}

	return ret;
}

static int di_runtime_resume(struct device *dev)
{
	struct di_driver_data *drvdata = dev_get_drvdata(dev);
	int ret = 0;

	if (drvdata != NULL) {
		ret = di_clk_enable(drvdata);
	} else {
		ret = -EFAULT;
		DI_ERR("drvdata is NULL!");
	}

	return ret;
}

static const struct dev_pm_ops di_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(di_suspend, di_resume)
	SET_RUNTIME_PM_OPS(di_runtime_suspend, di_runtime_resume, NULL)
};

static const struct of_device_id di_dt_match[] = {
	{.compatible = "allwinner,sunxi-deinterlace"},
	{},
};

static struct platform_driver di_driver = {
	.probe = di_probe,
	.remove = di_remove,
	.driver = {
		.name = DI_MODULE_NAME,
		.pm = &di_pm_ops,
		.of_match_table = di_dt_match,
	},
};

module_platform_driver(di_driver);

int debug_mask = DEBUG_LEVEL_ERR;
module_param_named(debug_mask, debug_mask, int, 0644);

MODULE_DEVICE_TABLE(of, di_dt_match);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hezuyao@allwinnertech.com");
MODULE_AUTHOR("zhengwanyu@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi De-Interlace");
MODULE_VERSION("1.0.0");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
MODULE_IMPORT_NS(DMA_BUF);
#endif
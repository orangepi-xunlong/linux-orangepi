/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk.h>
#include "clk-debugfs.h"
#include <linux/reset.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#if IS_ENABLED(CONFIG_AW_CCU)
#include "./sunxi-ng/ccu_common.h"
#include "./sunxi-ng/ccu_nkmp.h"
#else /* !IS_ENABLED(CONFIG_AW_CCU) */
#define SUNXI_MODNAME "ccu-legacy"
#include <sunxi-log.h>
#endif /* IS_ENABLED(CONFIG_AW_CCU) */

static struct dentry *my_ccudbg_root;
static struct testclk_data testclk_priv;

void clktest_register_rst(struct device *reset_dev)
{
	testclk_priv.reset_dev = reset_dev;;
}
EXPORT_SYMBOL_GPL(clktest_register_rst);

static int sunxi_rst_get_names(void)
{
	struct device_node *np = testclk_priv.reset_dev->of_node;
	int ret, i;

	testclk_priv.rst_count = of_property_count_strings(np, "reset-names");
	if (testclk_priv.rst_count <= 0)
		return -ENODATA;

	testclk_priv.reset_name = kzalloc(testclk_priv.rst_count, GFP_KERNEL);
	if (!testclk_priv.reset_name)
		return -ENOMEM;

	if (testclk_priv.rst_count > 0) {
		ret = of_property_read_string_array(np, "reset-names",
							testclk_priv.reset_name, testclk_priv.rst_count);
		if (ret < 0) {
			sunxi_err(NULL, "fail to get reset names\n");
			return -ENODATA;
		}
	}

	memset(testclk_priv.info, 0x0, sizeof(testclk_priv.info));

	for (i = 0; i < testclk_priv.rst_count; i++) {
		strcat(testclk_priv.info, testclk_priv.reset_name[i]);
		strcat(testclk_priv.info, " ");
	}

	return 0;
}
static void clktest_process(void)
{
	int i, j, enabled, command;
	unsigned long rate;
	struct clk *cur_clk = NULL;
	struct clk *parent_clk = NULL;
	struct reset_control *reset = NULL;
	int ret;
	unsigned long start = 0;

	ret = kstrtoul(testclk_priv.start, 0, (unsigned long *)&start);

	if (start == 1) {
		if (!strcmp(testclk_priv.command, "getparents"))
			command = 1;
		else if (!strcmp(testclk_priv.command, "getparent"))
			command = 2;
		else if (!strcmp(testclk_priv.command, "setparent"))
			command = 3;
		else if (!strcmp(testclk_priv.command, "getrate"))
			command = 4;
		else if (!strcmp(testclk_priv.command, "setrate"))
			command = 5;
		else if (!strcmp(testclk_priv.command, "is_enabled"))
			command = 6;
		else if (!strcmp(testclk_priv.command, "disable"))
			command = 7;
		else if (!strcmp(testclk_priv.command, "enable"))
			command = 8;
		else if (!strcmp(testclk_priv.command, "get_calc_cache"))
			command = 9;
		else if (!strcmp(testclk_priv.command, "get_rstnames"))
			command = 20;
		else if (!strcmp(testclk_priv.command, "assert"))
			command = 21;
		else if (!strcmp(testclk_priv.command, "deassert"))
			command = 22;
		else if (!strcmp(testclk_priv.command, "reset"))
			command = 23;
		else if (!strcmp(testclk_priv.command, "get_rststatus"))
			command = 24;
		else {
			sunxi_info(NULL, "Error Not support command %s\n",
			       testclk_priv.command);
			command = 0xff;
		}
		if (command < 20) {
			cur_clk = clk_get(NULL, testclk_priv.name);
			if (!cur_clk || IS_ERR(cur_clk)) {
				cur_clk = NULL;
				sunxi_info(NULL, "Error Found clk %s\n",
				       testclk_priv.name);
				strcpy(testclk_priv.info, "Error");
				return;
			}
		} else if (command > 9) {
			if (!testclk_priv.reset_dev) {
				sunxi_err(NULL, "not support rst test, please check if SUNXXIWXX_TEST_CCU is enabled\n");
				return;
			}

			reset = devm_reset_control_get(testclk_priv.reset_dev, testclk_priv.name);
			if (IS_ERR_OR_NULL(reset)) {
				sunxi_err(NULL, "get reset control failed\n");
				return;
			}
		}

		switch (command) {
		case 1: /* getparents */
		{
			j = 0;
			memset(testclk_priv.info, 0x0,
			       sizeof(testclk_priv.info));
			for (i = 0; i < cur_clk->core->num_parents; i++) {
				memcpy(&testclk_priv.info[j],
				       cur_clk->core->parents[i].name,
				       strlen(cur_clk->core
						  ->parents[i].name));
				j += strlen(
				    cur_clk->core->parents[i].name);
				testclk_priv.info[j] = ' ';
				j++;
			}
			testclk_priv.info[j] = '\0';
			break;
		}
		case 2: /* getparent */
		{

			parent_clk = clk_get_parent(cur_clk);
			if (!parent_clk || IS_ERR(parent_clk)) {
				sunxi_info(NULL, "Error Found parent of %s\n",
				       cur_clk->core->name);
				strcpy(testclk_priv.info, "Error");
			} else
				strcpy(testclk_priv.info,
				       parent_clk->core->name);
			break;
		}
		case 3: /* setparent */
		{
			if (cur_clk->core->parent) {
				parent_clk =
				    clk_get(NULL, testclk_priv.param);
				if (!parent_clk || IS_ERR(parent_clk)) {
					sunxi_info(NULL,
						   "Error Found parent %s\n",
						   testclk_priv.param);
					strcpy(testclk_priv.info,
					       "Error");
				} else {
					clk_set_parent(cur_clk,
						       parent_clk);
					strcpy(testclk_priv.info,
					       cur_clk->core->parent
						   ->name);
					clk_put(parent_clk);
				}
			} else
				strcpy(testclk_priv.info, "Error");
			break;
		}
		case 4: /* getrate */
		{
			rate = clk_get_rate(cur_clk);
			if (rate)

				sprintf(testclk_priv.info, "%lu",
					rate);
			else
				strcpy(testclk_priv.info, "Error");
			break;
		}
		case 5: /* setrate */
		{
			ret = kstrtoul(testclk_priv.param, 0,
					(unsigned long *)&rate);
			if (rate) {
				clk_set_rate(cur_clk, rate);
				sprintf(testclk_priv.info, "%lu",
					rate);
			} else
				strcpy(testclk_priv.info, "Error");
			break;
		}
		case 6: /* is_enabled */
		{
			if (!cur_clk->core->ops->is_enabled)
				enabled =
				    cur_clk->core->enable_count ? 1 : 0;
			else
				enabled =
				    cur_clk->core->ops->is_enabled(
					cur_clk->core->hw);
			if (enabled)
				strcpy(testclk_priv.info, "enabled");
			else
				strcpy(testclk_priv.info, "disabled");
			break;
		}
		case 7: /* disable */
		{
			if (!cur_clk->core->ops->is_enabled)
				enabled =
				    cur_clk->core->enable_count ? 1 : 0;
			else
				enabled =
				    cur_clk->core->ops->is_enabled(
					cur_clk->core->hw);
			if (enabled)
				clk_disable_unprepare(cur_clk);
			strcpy(testclk_priv.info, "disabled");
			break;
		}
		case 8: /* enable */
		{
			clk_prepare_enable(cur_clk);
			strcpy(testclk_priv.info, "enabled");
			break;
		}
		case 9:
		{
#if IS_ENABLED(CONFIG_AW_CCU)
			struct ccu_common *com;
			struct _ccu_nkmp_cache *nkmp_cache;
			char buftmp[50];
			unsigned int byte_size = 0, buf_size = 0;

			memset(testclk_priv.info, 0x0,
			       sizeof(testclk_priv.info));
			com = hw_to_ccu_common(__clk_get_hw(cur_clk));
			if (!(com->features & CCU_FEATURE_CLAC_CACHED)) {
				pr_err("not support calc cache!\n");
				break;
			}
			if (CCU_FEATURE_GET_TYPE(com->features) == CCU_FEATURE_TYPE_NKMP) {
				list_for_each_entry(nkmp_cache, &com->calc_cache, node) {
					buf_size = sprintf(buftmp, "type:nkmp; rate: %lld n:%d; k:%d; m:%d; p:%d\n",
						nkmp_cache->rate, nkmp_cache->n, nkmp_cache->k,
						nkmp_cache->m, nkmp_cache->p);

					buf_size += 1;
					byte_size += buf_size;
					if (byte_size > 1024)
						sunxi_err(NULL, "The size of the info[%d] array has exceeded 1024 bytes, \
								please increase the length of the array.\n", byte_size);

					strcat(testclk_priv.info, buftmp);
				}
			}
#endif
			break;
		}

		case 20: /* get_reset_names */
		{
			if (!testclk_priv.reset_dev) {
				sunxi_err(NULL, "not support rst test, please check if SUNXXIWXX_TEST_CCU is enabled\n");
				break;
			}

			sunxi_rst_get_names();
			break;
		}

		case 21: /* assert */
		{
			ret = reset_control_assert(reset);
			if (ret) {
				sunxi_err(NULL, "assert rst failed!\n");
				return;
			}
			strcpy(testclk_priv.info, "assert");
			break;
		}
		case 22: /* deassert */
		{
			ret = reset_control_deassert(reset);
			if (ret) {
				sunxi_err(NULL, "deassert rst failed!\n");
				return;
			}
			strcpy(testclk_priv.info, "deassert");
			break;
		}
		case 23: /* reset */
		{
			ret = reset_control_reset(reset);
			if (ret) {
				sunxi_err(NULL, "reset rst failed!\n");
				return;
			}
			strcpy(testclk_priv.info, "reset");
			break;
		}
		case 24: /* status */
		{
			ret = reset_control_status(reset);
			if (ret == 1) {
				strcpy(testclk_priv.info, "assert");
			} else if (ret == 0) {
				strcpy(testclk_priv.info, "deassert");
			} else {
				strcpy(testclk_priv.info, "failed");
			}
			break;
		}

		default:
			break;
		}
		if (cur_clk)
			clk_put(cur_clk);

		if (reset)
			reset_control_put(reset);
	}
}
/* command */
static int ccudbg_command_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int ccudbg_command_release(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t ccudbg_command_read(struct file *file, char __user *buf,
				   size_t count, loff_t *ppos)
{
	int len = strlen(testclk_priv.command);

	strcpy(testclk_priv.tmpbuf, testclk_priv.command);
	testclk_priv.tmpbuf[len] = 0x0A;
	testclk_priv.tmpbuf[len + 1] = 0x0;
	len = strlen(testclk_priv.tmpbuf);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user((void __user *)buf,
				 (const void *)testclk_priv.tmpbuf,
				 (unsigned long)len))
			return -EFAULT;
		*ppos += count;
	} else
		count = 0;
	return count;
}

static ssize_t ccudbg_command_write(struct file *file, const char __user *buf,
				    size_t count, loff_t *ppos)
{
	if (count >= sizeof(testclk_priv.command))
		return 0;
	if (copy_from_user(testclk_priv.command, buf, count))
		return -EFAULT;
	if (testclk_priv.command[count - 1] == 0x0A)
		testclk_priv.command[count - 1] = 0;
	else
		testclk_priv.command[count] = 0;
	return count;
}
/* name */
static int ccudbg_name_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int ccudbg_name_release(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t ccudbg_name_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	int len = strlen(testclk_priv.name);

	strcpy(testclk_priv.tmpbuf, testclk_priv.name);
	testclk_priv.tmpbuf[len] = 0x0A;
	testclk_priv.tmpbuf[len + 1] = 0x0;
	len = strlen(testclk_priv.tmpbuf);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user((void __user *)buf,
				 (const void *)testclk_priv.tmpbuf,
				 (unsigned long)len))
			return -EFAULT;
		*ppos += count;
	} else
		count = 0;
	return count;
}

static ssize_t ccudbg_name_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	if (count >= sizeof(testclk_priv.name))
		return 0;
	if (copy_from_user(testclk_priv.name, buf, count))
		return -EFAULT;
	if (testclk_priv.name[count - 1] == 0x0A)
		testclk_priv.name[count - 1] = 0;
	else
		testclk_priv.name[count] = 0;
	return count;
}
/* start */
static int ccudbg_start_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int ccudbg_start_release(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t ccudbg_start_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	int len = strlen(testclk_priv.start);

	strcpy(testclk_priv.tmpbuf, testclk_priv.start);
	testclk_priv.tmpbuf[len] = 0x0A;
	testclk_priv.tmpbuf[len + 1] = 0x0;
	len = strlen(testclk_priv.tmpbuf);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user((void __user *)buf,
				 (const void *)testclk_priv.tmpbuf,
				 (unsigned long)len))
			return -EFAULT;
		*ppos += count;
	} else
		count = 0;
	return count;
}
static ssize_t ccudbg_start_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	if (count >= sizeof(testclk_priv.start))
		return 0;
	if (copy_from_user(testclk_priv.start, buf, count))
		return -EFAULT;
	if (testclk_priv.start[count - 1] == 0x0A)
		testclk_priv.start[count - 1] = 0;
	else
		testclk_priv.start[count] = 0;
	clktest_process();
	return count;
}
/* param */
static int ccudbg_param_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int ccudbg_param_release(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t ccudbg_param_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	int len = strlen(testclk_priv.param);

	strcpy(testclk_priv.tmpbuf, testclk_priv.param);
	testclk_priv.tmpbuf[len] = 0x0A;
	testclk_priv.tmpbuf[len + 1] = 0x0;
	len = strlen(testclk_priv.tmpbuf);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user((void __user *)buf,
				 (const void *)testclk_priv.tmpbuf,
				 (unsigned long)len))
			return -EFAULT;
		*ppos += count;
	} else
		count = 0;
	return count;
}
static ssize_t ccudbg_param_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	if (count >= sizeof(testclk_priv.param))
		return 0;
	if (copy_from_user(testclk_priv.param, buf, count))
		return -EFAULT;
	if (testclk_priv.param[count - 1] == 0x0A)
		testclk_priv.param[count - 1] = 0;
	else
		testclk_priv.param[count] = 0;
	return count;
}
/* info */
static int ccudbg_info_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int ccudbg_info_release(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t ccudbg_info_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	int len = strlen(testclk_priv.info);

	strcpy(testclk_priv.tmpbuf, testclk_priv.info);
	testclk_priv.tmpbuf[len] = 0x0A;
	testclk_priv.tmpbuf[len + 1] = 0x0;
	len = strlen(testclk_priv.tmpbuf);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user((void __user *)buf,
				 (const void *)testclk_priv.tmpbuf,
				 (unsigned long)len))
			return -EFAULT;
		*ppos += count;
	} else
		count = 0;
	return count;
}
static ssize_t ccudbg_info_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	if (count >= sizeof(testclk_priv.info))
		return 0;
	if (copy_from_user(testclk_priv.info, buf, count))
		return -EFAULT;
	if (testclk_priv.info[count - 1] == 0x0A)
		testclk_priv.info[count - 1] = 0;
	else
		testclk_priv.info[count] = 0;
	return count;
}
static const struct file_operations command_ops = {
	.write      = ccudbg_command_write,
	.read       = ccudbg_command_read,
	.open       = ccudbg_command_open,
	.release    = ccudbg_command_release,
};
static const struct file_operations name_ops = {
	.write      = ccudbg_name_write,
	.read       = ccudbg_name_read,
	.open       = ccudbg_name_open,
	.release    = ccudbg_name_release,
};
static const struct file_operations start_ops = {
	.write      = ccudbg_start_write,
	.read       = ccudbg_start_read,
	.open       = ccudbg_start_open,
	.release    = ccudbg_start_release,
};
static const struct file_operations param_ops = {
	.write      = ccudbg_param_write,
	.read       = ccudbg_param_read,
	.open       = ccudbg_param_open,
	.release    = ccudbg_param_release,
};
static const struct file_operations info_ops = {
	.write      = ccudbg_info_write,
	.read       = ccudbg_info_read,
	.open       = ccudbg_info_open,
	.release    = ccudbg_info_release,
};
static int __init debugfs_test_init(void)
{
	my_ccudbg_root = debugfs_create_dir("ccudbg", NULL);
	if (!debugfs_create_file("command", 0644, my_ccudbg_root, NULL,
				 &command_ops))
		goto Fail;
	if (!debugfs_create_file("name", 0644, my_ccudbg_root, NULL, &name_ops))
		goto Fail;
	if (!debugfs_create_file("start", 0644, my_ccudbg_root, NULL,
				 &start_ops))
		goto Fail;
	if (!debugfs_create_file("param", 0644, my_ccudbg_root, NULL,
				 &param_ops))
		goto Fail;
	if (!debugfs_create_file("info", 0644, my_ccudbg_root, NULL, &info_ops))
		goto Fail;
	return 0;

Fail:
	debugfs_remove_recursive(my_ccudbg_root);
	my_ccudbg_root = NULL;
	return -ENOENT;
}

late_initcall(debugfs_test_init);
MODULE_DESCRIPTION("Allwinner clk debug driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.5");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Allwinner mm Vendor Hooks
 *
 * Copyright (C) 2022 Allwinner.
 */

#define pr_fmt(fmt) "sunxi_mm: " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <trace/hooks/vmscan.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

static int g_direct_swappiness = 60;
static int g_swappiness = 160;
#define SHRINK_BYPASS_JIFFIES 3

#define PARA_BUF_LEN 128
static struct proc_dir_entry *para_entry, *para_entry2;
static struct proc_dir_entry *shrink_bypass_entry;
static unsigned long shrink_slab_count, shrink_slab_bypass;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 25)
unsigned long shrink_slab_bypass_jiffies;
static int shrink_slab_kret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	long retval;

	if (!current_is_kswapd())
		return 0;
	retval = regs_return_value(regs);

	if (retval == 0)
		shrink_slab_bypass_jiffies = jiffies;
	return 0;
}

static struct kretprobe shrink_slab_kretprobe = {
	.handler = shrink_slab_kret_handler,
	.maxactive = 20,
	.kp.symbol_name = "shrink_slab",
};

static void sunxi_shrink_slab_bypass(void *data, gfp_t gfp_mask, int nid,
				     struct mem_cgroup *memcg, int priority, bool *bypass)
{
	if (!current_is_kswapd())
		return;
	if (priority < 4)
		return;
	if (jiffies - shrink_slab_bypass_jiffies < SHRINK_BYPASS_JIFFIES) {
		*bypass = true;
		shrink_slab_bypass++;
	} else {
		shrink_slab_count++;
	}
}

int register_shrink_slab_bypass_hook(void)
{
	int ret;

	ret = register_kretprobe(&shrink_slab_kretprobe);
	if (ret < 0) {
		pr_err("register kretprobe:%s failed, ret:%d\n",
		       shrink_slab_kretprobe.kp.symbol_name, ret);
		return ret;
	}

	ret = register_trace_android_vh_shrink_slab_bypass(sunxi_shrink_slab_bypass, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_vh_shrink_slab_bypass failed! ret=%d\n", ret);
		goto err_out;
	}
	return ret;
err_out:
	unregister_kretprobe(&shrink_slab_kretprobe);
	return ret;
}

void unregister_shrink_slab_bypass_hook(void)
{
	unregister_trace_android_vh_shrink_slab_bypass(sunxi_shrink_slab_bypass, NULL);
	unregister_kretprobe(&shrink_slab_kretprobe);
}

static void sunxi_set_swappiness(void *data, int *swappiness)
{
	if (!current_is_kswapd())
		*swappiness = g_direct_swappiness;
	else
		*swappiness = g_swappiness;

	return;
}

static void sunxi_set_inactive_ratio(void *data, unsigned long *inactive_ratio, int file)
{
	if (file)
		*inactive_ratio = min(2UL, *inactive_ratio);
	else
		*inactive_ratio = 1;

	return;
}
#endif

static void sunxi_set_balance_anon_file_reclaim(void *data, bool *balance_anon_file_reclaim)
{
	*balance_anon_file_reclaim = true;

	return;
}

static int register_sunxi_mm_vendor_hooks(void)
{
	int ret = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 25)
	ret = register_trace_android_vh_tune_swappiness(sunxi_set_swappiness, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_vh_set_swappiness failed! ret=%d\n", ret);
		goto out;
	}

	ret = register_trace_android_vh_tune_inactive_ratio(sunxi_set_inactive_ratio, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_vh_tune_inactive_ratio failed! ret=%d\n", ret);
		goto out;
	}
	ret = register_shrink_slab_bypass_hook();
	if (ret != 0) {
		pr_err("register_shrink_slab_bypass failed! ret=%d\n", ret);
		goto out;
	}
#endif

	ret = register_trace_android_rvh_set_balance_anon_file_reclaim(
						sunxi_set_balance_anon_file_reclaim, NULL);
	if (ret != 0) {
		pr_err("register_trace_android_rvh_set_balance_anon_file_reclaim failed! ret=%d\n", ret);
		goto out;
	}

out:
	return ret;
}

static void unregister_sunxi_mm_vendor_hooks(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 25)
	unregister_trace_android_vh_tune_swappiness(sunxi_set_swappiness, NULL);
	unregister_trace_android_vh_tune_inactive_ratio(sunxi_set_inactive_ratio, NULL);
	unregister_shrink_slab_bypass_hook();
#endif

	return;
}

static ssize_t direct_swappiness_write(struct file *file,
		const char __user *buff, size_t len, loff_t *ppos)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	char *str;
	long val;
	int ret = 0;

	if (len > PARA_BUF_LEN - 1) {
		pr_err("len %ld is too long\n", len);
		return -EINVAL;
	}

	if (copy_from_user(&kbuf, buff, len))
		return -EFAULT;
	kbuf[len] = '\0';

	str = strstrip(kbuf);
	if (!str) {
		pr_err("buff %s is invalid\n", kbuf);
		return -EINVAL;
	}

	ret = kstrtoul(str, 0, &val);
	if (ret)
		return -EINVAL;
	else
		g_direct_swappiness = val;

	return len;
}

static ssize_t direct_swappiness_read(struct file *file,
		char __user *buffer, size_t count, loff_t *off)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	int len;

	len = snprintf(kbuf, PARA_BUF_LEN, "%d\n", g_direct_swappiness);
	if (len == PARA_BUF_LEN)
		kbuf[len - 1] = '\0';

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static ssize_t swappiness_write(struct file *file, const char __user *buff,
				size_t len, loff_t *ppos)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	char *str;
	long val;
	int ret = 0;

	if (len > PARA_BUF_LEN - 1) {
		pr_err("len %ld is too long\n", len);
		return -EINVAL;
	}

	if (copy_from_user(&kbuf, buff, len))
		return -EFAULT;
	kbuf[len] = '\0';

	str = strstrip(kbuf);
	if (!str) {
		pr_err("buff %s is invalid\n", kbuf);
		return -EINVAL;
	}

	ret = kstrtoul(str, 0, &val);
	if (ret)
		return -EINVAL;

	g_swappiness = val;

	return len;
}

static ssize_t swappiness_read(struct file *file, char __user *buffer,
			       size_t count, loff_t *off)
{
	char kbuf[PARA_BUF_LEN] = {'\0'};
	int len;

	len = snprintf(kbuf, PARA_BUF_LEN, "%d\n", g_swappiness);
	if (len == PARA_BUF_LEN)
		kbuf[len - 1] = '\0';

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static ssize_t shrink_bypass_read(struct file *file, char __user *buffer,
				  size_t count, loff_t *off)
{
	char *kbuf;
	int len;

	kbuf = kzalloc(256, GFP_KERNEL);
	len = sprintf(kbuf, "shrink_slab_count:%d\n", shrink_slab_count);
	len += sprintf(kbuf + len, "shrink_slab_bypass:%d\n", shrink_slab_bypass);

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buffer, kbuf + *off, (len < count ? len : count)))
		return -EFAULT;

	*off += (len < count ? len : count);
	return (len < count ? len : count);
}

static const struct proc_ops proc_direct_swappiness_para_ops = {
	.proc_write          = direct_swappiness_write,
	.proc_read		= direct_swappiness_read,
};

static const struct proc_ops proc_swappiness_para_ops = {
	.proc_write          = swappiness_write,
	.proc_read		= swappiness_read,
};

static const struct proc_ops proc_shrink_bypass_ops = {
	.proc_read		= shrink_bypass_read,
};

static int __init create_sunxi_vm_proc(void)
{
	struct proc_dir_entry *root_dir_entry = proc_mkdir("sunxi_vm", NULL);

	para_entry = proc_create((root_dir_entry ?
				"direct_swappiness" : "sunxi_vm/direct_swappiness"),
			0600, root_dir_entry, &proc_direct_swappiness_para_ops);

	if (para_entry)
		pr_info("Register sunxi direct_swappiness passed.\n");

	para_entry2 = proc_create((root_dir_entry ?
				"swappiness" : "sunxi_vm/swappiness"),
			0600, root_dir_entry, &proc_swappiness_para_ops);

	if (para_entry)
		pr_info("Register sunxi swappiness passed.\n");

	shrink_bypass_entry = proc_create((root_dir_entry ?
				"shrink_bypass" : "sunxi_vm/shrink_bypass"),
			0400, root_dir_entry, &proc_shrink_bypass_ops);

	if (shrink_bypass_entry)
		pr_info("Register sunxi shrink bypass passed.\n");
	return 0;
}

static void destroy_sunxi_vm_proc(void)
{
	remove_proc_subtree("sunxi_vm", NULL);
}


static int __init sunxi_mm_init(void)
{
	int ret = 0;

	ret = create_sunxi_vm_proc();
	if (ret)
		return ret;

	ret = register_sunxi_mm_vendor_hooks();
	if (ret != 0) {
		destroy_sunxi_vm_proc();
		return ret;
	}

	return 0;
}

static void __exit sunxi_mm_exit(void)
{
	unregister_sunxi_mm_vendor_hooks();
	destroy_sunxi_vm_proc();

	pr_info("sunxi_mm_exit succeed!\n");

	return;
}

module_init(sunxi_mm_init);
module_exit(sunxi_mm_exit);

module_param_named(vm_swappiness, g_swappiness, int, S_IRUGO | S_IWUSR);
module_param_named(direct_vm_swappiness, g_direct_swappiness, int, S_IRUGO | S_IWUSR);
MODULE_VERSION("1.0.2");
MODULE_AUTHOR("henryli");
MODULE_LICENSE("GPL v2");

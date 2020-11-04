// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 frank@allwinnertech.com
 */

#include <linux/device.h>
#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#define DEFINE_SHOW_ATTRIBUTE(__name)					\
static int __name ## _open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, __name ## _show, inode->i_private);	\
}									\
									\
static const struct file_operations __name ## _fops = {			\
	.owner		= THIS_MODULE,					\
	.open		= __name ## _open,				\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.release	= single_release,				\
}

static int cpufreq_table_show(struct seq_file *s, void *unused)
{
	struct dev_pm_opp *opp;
	int i, max_opps;
	unsigned long rate, vol;
	struct device *dev;

	dev = get_cpu_device(0);
	if (!dev)
		return 0;

	seq_printf(s, "freq(kHz)\tvol(mv)\n");
	seq_printf(s, "-----------------------\n");

	rcu_read_lock();

	max_opps = dev_pm_opp_get_opp_count(dev);
	if (max_opps < 0)
		goto out;

	for (i = 0, rate = 0; i < max_opps; i++, rate++) {
		/* find next rate */
		opp = dev_pm_opp_find_freq_ceil(dev, &rate);
		vol = dev_pm_opp_get_voltage(opp);

		seq_printf(s, "%ld\t\t", rate / 1000);
		seq_printf(s, "%ld\n", vol / 1000);
	}

out:
	rcu_read_unlock();

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cpufreq_table);

static int __init vf_init(void)
{
	debugfs_create_file("cpufreq_table", 0444, NULL, NULL,
			    &cpufreq_table_fops);
	return 0;
}

module_init(vf_init);
MODULE_LICENSE("GPL");

/*
 * drivers/usb/sunxi_usb/udc/usb3/debugfs.c
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller debugfs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */


#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/usb/ch9.h>

#include "core.h"
#include "gadget.h"
#include "io.h"
#include "debug.h"
#include "osal.h"

static int sunxi_regdump_show(struct seq_file *s, void *unused)
{
	DMSG_INFO("%s\n", __func__);
	return 0;
}

static int sunxi_regdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_regdump_show, inode->i_private);
}

static const struct file_operations sunxi_regdump_fops = {
	.open			= sunxi_regdump_open,
	.read			= seq_read,
	.release		= single_release,
};

static int sunxi_mode_show(struct seq_file *s, void *unused)
{
	struct sunxi_otgc		*otgc = s->private;
	unsigned long		flags;
	u32			reg;

	spin_lock_irqsave(&otgc->lock, flags);
	reg = get_sunxi_gctl_regs(otgc->regs);
	spin_unlock_irqrestore(&otgc->lock, flags);

	switch (SUNXI_GCTL_PRTCAP(reg)) {
	case SUNXI_GCTL_PRTCAP_HOST:
		seq_printf(s, "host\n");
		break;
	case SUNXI_GCTL_PRTCAP_DEVICE:
		seq_printf(s, "device\n");
		break;
	case SUNXI_GCTL_PRTCAP_OTG:
		seq_printf(s, "OTG\n");
		break;
	default:
		seq_printf(s, "UNKNOWN %08x\n", SUNXI_GCTL_PRTCAP(reg));
	}

	return 0;
}

static int sunxi_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_mode_show, inode->i_private);
}

static ssize_t sunxi_mode_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct sunxi_otgc		*otgc = s->private;
	unsigned long		flags;
	u32			mode = 0;
	char			buf[32];

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "host", 4))
		mode |= SUNXI_GCTL_PRTCAP_HOST;

	if (!strncmp(buf, "device", 6))
		mode |= SUNXI_GCTL_PRTCAP_DEVICE;

	if (!strncmp(buf, "otg", 3))
		mode |= SUNXI_GCTL_PRTCAP_OTG;

	if (mode) {
		spin_lock_irqsave(&otgc->lock, flags);
		sunxi_set_mode(otgc, mode);
		spin_unlock_irqrestore(&otgc->lock, flags);
	}
	return count;
}

static const struct file_operations sunxi_mode_fops = {
	.open			= sunxi_mode_open,
	.write			= sunxi_mode_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int sunxi_testmode_show(struct seq_file *s, void *unused)
{
	struct sunxi_otgc		*otgc = s->private;
	unsigned long		flags;
	u32			reg;

	spin_lock_irqsave(&otgc->lock, flags);
	reg = get_sunxi_dctl_tstctrl_mark(otgc->regs);
	spin_unlock_irqrestore(&otgc->lock, flags);

	switch (reg) {
	case 0:
		seq_printf(s, "no test\n");
		break;
	case TEST_J:
		seq_printf(s, "test_j\n");
		break;
	case TEST_K:
		seq_printf(s, "test_k\n");
		break;
	case TEST_SE0_NAK:
		seq_printf(s, "test_se0_nak\n");
		break;
	case TEST_PACKET:
		seq_printf(s, "test_packet\n");
		break;
	case TEST_FORCE_EN:
		seq_printf(s, "test_force_enable\n");
		break;
	default:
		seq_printf(s, "UNKNOWN %d\n", reg);
	}

	return 0;
}

static int sunxi_testmode_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_testmode_show, inode->i_private);
}

static ssize_t sunxi_testmode_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct sunxi_otgc		*otgc = s->private;
	unsigned long		flags;
	u32			testmode = 0;
	char			buf[32];

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "test_j", 6))
		testmode = TEST_J;
	else if (!strncmp(buf, "test_k", 6))
		testmode = TEST_K;
	else if (!strncmp(buf, "test_se0_nak", 12))
		testmode = TEST_SE0_NAK;
	else if (!strncmp(buf, "test_packet", 11))
		testmode = TEST_PACKET;
	else if (!strncmp(buf, "test_force_enable", 17))
		testmode = TEST_FORCE_EN;
	else
		testmode = 0;

	spin_lock_irqsave(&otgc->lock, flags);
	sunxi_gadget_set_test_mode(otgc->regs, testmode);
	spin_unlock_irqrestore(&otgc->lock, flags);

	return count;
}

static const struct file_operations sunxi_testmode_fops = {
	.open			= sunxi_testmode_open,
	.write			= sunxi_testmode_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int sunxi_link_state_show(struct seq_file *s, void *unused)
{
	DMSG_INFO("%s\n", __func__);
	return 0;
}

static int sunxi_link_state_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_link_state_show, inode->i_private);
}

static ssize_t sunxi_link_state_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	DMSG_INFO("%s\n", __func__);
	return count;
}

static const struct file_operations sunxi_link_state_fops = {
	.open			= sunxi_link_state_open,
	.write			= sunxi_link_state_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

int __devinit sunxi_debugfs_init(struct sunxi_otgc *otgc)
{
	struct dentry		*root;
	struct dentry		*file;
	int			ret;

	root = debugfs_create_dir(dev_name(otgc->dev), NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	otgc->root = root;

	file = debugfs_create_file("regdump", S_IRUGO, root, otgc,
			&sunxi_regdump_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("mode", S_IRUGO | S_IWUSR, root,
			otgc, &sunxi_mode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("testmode", S_IRUGO | S_IWUSR, root,
			otgc, &sunxi_testmode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("link_state", S_IRUGO | S_IWUSR, root,
			otgc, &sunxi_link_state_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void __devexit sunxi_debugfs_exit(struct sunxi_otgc *otgc)
{
	debugfs_remove_recursive(otgc->root);
	otgc->root = NULL;
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dwc3-debug.c - DesignWare USB3 DRD Controller DebugFS file for allwinner platform
 *
 * Copyright (c) 2024 Allwinner Technology Co., Ltd.
 *
 * debugfs.c - DesignWare USB3 DRD Controller DebugFS file
 *
 * Copyright (C) 2010-2011 Texas Instruments Incorporated - https://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
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

#define DWC3_COMPLIANCE_PATTERN_NUM (13)
typedef enum dwc3_cp_pat {
	DWC3_CP00 = 0,
	DWC3_CP01,
	DWC3_CP02,
	DWC3_CP03,
	DWC3_CP04,
	DWC3_CP05,
	DWC3_CP06,
	DWC3_CP07,
	DWC3_CP08,
	DWC3_CP09,
	DWC3_CP10,
	DWC3_CP11,
	DWC3_CP12,
	DWC3_CP_UNKNOWN,
} dwc3_cp_pat_t;

static dwc3_cp_pat_t cur_pat = DWC3_CP_UNKNOWN;

static int dwc3_compliance_show(struct seq_file *s, void *unused)
{
	if (cur_pat == DWC3_CP_UNKNOWN) {
		seq_printf(s, "Sorry, The Compliance Test Hasn't Started.\nYou Can Use it For Test:\necho CP00 > /sys/kernel/debug/usb/4d00000.xhci2-controller/compliance\n");
	} else {
		seq_printf(s, "Current Compliance Test Pattern: CP%02d\n", cur_pat);
	}

	return 0;
}

static int dwc3_compliance_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_compliance_show, inode->i_private);
}

static dwc3_cp_pat_t dwc3_compliance_parse_pattern(const char *buf)
{
	enum dwc3_cp_pat cp;
	if (!strncmp(buf, "CP00", 4))
		cp = DWC3_CP00;
	else if (!strncmp(buf, "CP01", 4))
		cp = DWC3_CP01;
	else if (!strncmp(buf, "CP02", 4))
		cp = DWC3_CP02;
	else if (!strncmp(buf, "CP03", 4))
		cp = DWC3_CP03;
	else if (!strncmp(buf, "CP04", 4))
		cp = DWC3_CP04;
	else if (!strncmp(buf, "CP05", 4))
		cp = DWC3_CP05;
	else if (!strncmp(buf, "CP06", 4))
		cp = DWC3_CP06;
	else if (!strncmp(buf, "CP07", 4))
		cp = DWC3_CP07;
	else if (!strncmp(buf, "CP08", 4))
		cp = DWC3_CP08;
	else if (!strncmp(buf, "CP09", 4))
		cp = DWC3_CP09;
	else if (!strncmp(buf, "CP10", 4))
		cp = DWC3_CP10;
	else if (!strncmp(buf, "CP11", 4))
		cp = DWC3_CP11;
	else if (!strncmp(buf, "CP12", 4))
		cp = DWC3_CP12;
	else
		cp = DWC3_CP_UNKNOWN;

	return cp;
}

static ssize_t dwc3_compliance_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct dwc3		*dwc = s->private;
	dwc3_cp_pat_t		pat;
	u32			step = 0;
	char			buf[32];
	unsigned long		flags;
	u32			reg;

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	pat = dwc3_compliance_parse_pattern(buf);
	if (pat == DWC3_CP_UNKNOWN) {
		sunxi_info(NULL, "this is unknown pattern %02d!\n", pat);
		return count;
	}
	sunxi_info(NULL, "Start Compliance Test (%02d -> %02d)\n", cur_pat, pat);

	if (pat >= cur_pat)
		step = pat - cur_pat;
	else if (pat < cur_pat)
		step = pat + DWC3_COMPLIANCE_PATTERN_NUM - cur_pat;

	sunxi_info(NULL, "Please disconnect the DUT!\n");
	spin_lock_irqsave(&dwc->lock, flags);
	reg = dwc3_sunxi_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
	spin_unlock_irqrestore(&dwc->lock, flags);
	sunxi_info(NULL, "DWC3_GUSB3PIPECTL0 0x%04X: 0x%08X\n", DWC3_GUSB3PIPECTL(0), reg);

	if (reg & BIT(30))
		sunxi_info(NULL, "Current CP%02d, Test CP%02d\n", cur_pat, pat);
	else
		step++;

	sunxi_info(NULL, "Total step is %d\n", step);
	while (step) {
		reg = dwc3_sunxi_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
		reg &= ~BIT(30);
		dwc3_sunxi_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);
		reg |= BIT(30);
		dwc3_sunxi_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);
		step--;
	}
	cur_pat = pat;
	sunxi_info(NULL, "CP%02d Change Finish, You Can Start Test By Oscilloscope!\n", cur_pat);

	return count;
}

static const struct file_operations dwc3_compliance_fops = {
	.open = dwc3_compliance_open,
	.write = dwc3_compliance_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void dwc3_debug_init(struct dwc3 *dwc)
{
	struct dentry *root = debugfs_lookup(dev_name(dwc->dev), usb_debug_root);

	/* Create rootdir with parent 'usb' present not yet! */
	if (!root)
		root = debugfs_create_dir(dev_name(dwc->dev), usb_debug_root);

	debugfs_create_file("compliance", 0644, root, dwc, &dwc3_compliance_fops);
}

void dwc3_debug_exit(struct dwc3 *dwc)
{
	debugfs_lookup_and_remove(dev_name(dwc->dev), usb_debug_root);
}

static int dwc3_force_host_show(struct seq_file *s, void *unused)
{
	struct dwc3_sunxi_plat	*dwc3 = s->private;
	struct dwc3		*dwc = dwc3->dwc;
	unsigned long		flags;
	u32			reg;
	int			ret;

	if (!dwc3->u2drd_u3host_quirk) {
		seq_puts(s, "not supported\n");
		return 0;
	}

	ret = pm_runtime_resume_and_get(dwc->dev);
	if (ret < 0)
		return ret;

	spin_lock_irqsave(&dwc->lock, flags);
	reg = dwc3_sunxi_readl(dwc->regs, DWC3_GCTL);
	spin_unlock_irqrestore(&dwc->lock, flags);

	switch (DWC3_GCTL_PRTCAP(reg)) {
	case DWC3_GCTL_PRTCAP_HOST:
		seq_puts(s, "enabled\n");
		break;
	case DWC3_GCTL_PRTCAP_DEVICE:
	case DWC3_GCTL_PRTCAP_OTG:
	default:
		seq_puts(s, "disabled\n");
	}

	pm_runtime_put_sync(dwc->dev);

	return 0;
}

static int dwc3_force_host_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_force_host_show, inode->i_private);
}

static ssize_t dwc3_force_host_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct dwc3_sunxi_plat	*dwc3 = s->private;
	char			buf[32];

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "enabled", 7))
		dwc3_set_host(dwc3, true);
	else if (!strncmp(buf, "disabled", 8))
		dwc3_set_host(dwc3, false);
	else
		return -EINVAL;

	return count;
}

static const struct file_operations dwc3_force_host_fops = {
	.open = dwc3_force_host_open,
	.write = dwc3_force_host_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void dwc3_sunxi_debug_init(struct dwc3_sunxi_plat *dwc3)
{
	struct dwc3 *dwc = dwc3->dwc;
	struct dentry *root = debugfs_lookup(dev_name(dwc->dev), usb_debug_root);

	/* Create rootdir with parent 'usb' present not yet! */
	if (!root)
		root = debugfs_create_dir(dev_name(dwc->dev), usb_debug_root);

	debugfs_create_file("force_host", 0644, root, dwc3, &dwc3_force_host_fops);
}

void dwc3_sunxi_debug_exit(struct dwc3_sunxi_plat *dwc3)
{
	struct dwc3 *dwc = dwc3->dwc;
	debugfs_lookup_and_remove(dev_name(dwc->dev), usb_debug_root);
}

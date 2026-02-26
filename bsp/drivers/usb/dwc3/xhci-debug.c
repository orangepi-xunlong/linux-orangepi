// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * xhci-debug.c - xHCI debugfs interface for allwinner platform
 *
 * Copyright (c) 2024 Allwinner Technology Co., Ltd.
 *
 * xhci-debugfs.c - xHCI debugfs interface
 *
 * Copyright (C) 2017 Intel Corporation
 *
 * Author: Lu Baolu <baolu.lu@linux.intel.com>
 */

#include <linux/slab.h>
#include <linux/uaccess.h>

#include "xhci.h"
#include "xhci-debugfs.h"

static int xhci_portsc_show(struct seq_file *s, void *unused)
{
	struct xhci_port	*port = s->private;
	u32			portsc;
	char			str[XHCI_MSG_MAX];

	portsc = readl(port->addr);
	seq_printf(s, "%s\n", xhci_decode_portsc(str, portsc));

	return 0;
}

static int xhci_port_open(struct inode *inode, struct file *file)
{
	return single_open(file, xhci_portsc_show, inode->i_private);
}

static ssize_t xhci_port_write(struct file *file,  const char __user *ubuf,
			       size_t count, loff_t *ppos)
{
	struct seq_file		*s = file->private_data;
	struct xhci_port	*port = s->private;
	struct xhci_hcd		*xhci = hcd_to_xhci(port->rhub->hcd);
	char			buf[32];
	u32			portsc;
	unsigned long		flags;
	u32			command;
	u32			state;

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "reset", 5)) {
		spin_lock_irqsave(&xhci->lock, flags);
		sunxi_info(NULL, "RESET XHCI Controller\n");
		state = readl(&xhci->op_regs->status);
		sunxi_info(NULL, "state:0x%08x\n", state);

		sunxi_info(NULL, "Stop HCD\n");
		command = readl(&xhci->op_regs->command);
		sunxi_info(NULL, "command:0x%08x\n", command);
		command &= ~CMD_RUN;
		writel(command, &xhci->op_regs->command);
		sunxi_info(NULL, "command:0x%08x\n", readl(&xhci->op_regs->command));

		state = readl(&xhci->op_regs->status);
		sunxi_info(NULL, "state:0x%08x\n", state);

		sunxi_info(NULL, "Resetting HCD\n");
		command = readl(&xhci->op_regs->command);
		sunxi_info(NULL, "command:0x%08x\n", command);
		command |= CMD_RESET;
		writel(command, &xhci->op_regs->command);
		sunxi_info(NULL, "command:0x%08x\n", readl(&xhci->op_regs->command));

		while (1) {
			command = readl(&xhci->op_regs->command);
			if (!(command & BIT(1))) {
				sunxi_info(NULL, "command:0x%08x\n", command);
				break;
			}
		}
		sunxi_info(NULL, "Reset complete\n");

		portsc = readl(port->addr);
		sunxi_info(NULL, "portsc:0x%08x\n", portsc);
		portsc &= ~PORT_POWER;
		writel(portsc, port->addr);
		sunxi_info(NULL, "portsc:0x%08x\n", readl(port->addr));
		sunxi_info(NULL, "RESET XHCI Controller finished\n");
		spin_unlock_irqrestore(&xhci->lock, flags);
	} else {
		return -EINVAL;
	}
	return count;
}

static const struct file_operations port_fops = {
	.open			= xhci_port_open,
	.write			= xhci_port_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static void xhci_debugfs_create_port(struct xhci_hcd *xhci,
				      struct dentry *parent)
{
	unsigned int		num_ports;
	char			port_name[8];
	struct xhci_port	*port;
	struct dentry		*dir;

	num_ports = HCS_MAX_PORTS(xhci->hcs_params1);

	parent = debugfs_lookup("ports", parent);

	while (num_ports--) {
		scnprintf(port_name, sizeof(port_name), "port%02d",
			  num_ports + 1);
		dir = debugfs_lookup(port_name, parent);
		port = &xhci->hw_ports[num_ports];
		debugfs_create_file("port", 0644, dir, port, &port_fops);
	}
}

static void xhci_debugfs_destroy_port(struct xhci_hcd *xhci,
				      struct dentry *parent)
{
	unsigned int		num_ports;
	char			port_name[8];
	struct dentry		*dir;

	num_ports = HCS_MAX_PORTS(xhci->hcs_params1);

	parent = debugfs_lookup("ports", parent);

	while (num_ports--) {
		scnprintf(port_name, sizeof(port_name), "port%02d",
			  num_ports + 1);
		dir = debugfs_lookup(port_name, parent);
		debugfs_lookup_and_remove("port", dir);
	}
}

void xhci_debug_init(struct dwc3 *dwc)
{
	struct platform_device *pdev = dwc->xhci;
	struct usb_hcd *hcd = NULL;
	struct xhci_hcd *xhci = NULL;

	if (pdev == NULL)
		return;

	hcd = platform_get_drvdata(pdev);
	if (!hcd)
		return;

	xhci = hcd_to_xhci(hcd);

	xhci_debugfs_create_port(xhci, xhci->debugfs_root);
}

void xhci_debug_exit(struct dwc3 *dwc)
{
	struct platform_device *pdev = dwc->xhci;
	struct usb_hcd *hcd = NULL;
	struct xhci_hcd *xhci = NULL;

	if (pdev == NULL)
		return;

	hcd = platform_get_drvdata(pdev);
	if (!hcd)
		return;

	xhci = hcd_to_xhci(hcd);

	xhci_debugfs_destroy_port(xhci, xhci->debugfs_root);
}

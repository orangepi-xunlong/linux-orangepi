/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __SUNXI_DRM_NOTIFY_H__
#define __SUNXI_DRM_NOTIFY_H__

#include <linux/notifier.h>
#include <linux/export.h>
#include <linux/rbtree.h>

#define SUNXI_EVENT_BLANK         9
#define SUNXI_PANEL_EVENT_BLANK   9
#define SUNXI_PANEL_EVENT_UNBLANK 10

struct sunxi_disp_notify {
	struct work_struct disp_work;
	int blank;
};

struct sunxi_notify_event {
    void *data;
    int *reserved;
};

extern int sunxi_disp_register_client(struct notifier_block *nb);
extern int sunxi_disp_unregister_client(struct notifier_block *nb);
extern void sunxi_disp_notify_call_chain(int cmd, int flag);

#endif /* End of file */

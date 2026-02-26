/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <video/sunxi_drm_notify.h>

struct sunxi_disp_notify __sunxi_disp_notify;

static BLOCKING_NOTIFIER_HEAD(sunxi_disp_notifier_list);

/**
 *	sunxi_disp_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 *
 *	Return: 0 on success, negative error code on failure.
 */
int sunxi_disp_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sunxi_disp_notifier_list, nb);
}
EXPORT_SYMBOL(sunxi_disp_register_client);

/**
 *	sunxi_disp_unregister_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 *
 *	Return: 0 on success, negative error code on failure.
 */
int sunxi_disp_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&sunxi_disp_notifier_list, nb);
}
EXPORT_SYMBOL(sunxi_disp_unregister_client);

/**
 * __sunxi_disp_notify_call_chain - notify clients of fb_events
 * @val: value passed to callback
 * @v: pointer passed to callback
 *
 * Return: The return value of the last notifier function
 */
int __sunxi_disp_notify_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&sunxi_disp_notifier_list, val, v);
}

static void sunxi_disp_notify_tp_work(struct work_struct *work)
{
	struct sunxi_notify_event disp_event;
	struct sunxi_disp_notify *sunxi_disp_notify = container_of(work, struct sunxi_disp_notify, disp_work);

	disp_event.data = &sunxi_disp_notify->blank;
	__sunxi_disp_notify_call_chain(SUNXI_EVENT_BLANK, &disp_event);
}

/**
 * __sunxi_disp_notify_call_chain - notify clients of fb_events
 * @cmd: value of v->data, such as BLANK or UNBLANK.
 * @flag: 1: sync notify, 0: async notify
 *
 * Return: The return value of the last notifier function
 */
void sunxi_disp_notify_call_chain(int cmd, int flag)
{
	struct sunxi_notify_event disp_event;

	if (flag) {
		disp_event.data = &cmd;
		__sunxi_disp_notify_call_chain(SUNXI_EVENT_BLANK, &disp_event);
	} else {
		__sunxi_disp_notify.blank = cmd;
		schedule_work(&__sunxi_disp_notify.disp_work);
	}
}
EXPORT_SYMBOL(sunxi_disp_notify_call_chain);

void sunxi_disp_notify_init(void)
{
	INIT_WORK(&__sunxi_disp_notify.disp_work, sunxi_disp_notify_tp_work);
}

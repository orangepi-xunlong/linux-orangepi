// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kmemleak.h>
#include <asm/cacheflush.h>
#include <linux/version.h>
#include <linux/input.h>
#include "sunxi-crashdump.h"

#define CRASHDUMP_MAGIC_VOLUP  (0x766c7570) /* v-l-u-p */
#define CRASHDUMP_MAGIC_VOLDN  (0X766c646e) /* v-l-d-n */
#define SUNXI_CRASHDUMP_KEY_DEBUG 0

void sunxi_crashdump_key(unsigned int code, int value)
{
	static unsigned int volup_p;
	static unsigned int voldown_p;
	static unsigned int loopcount;
	static unsigned long vol_pressed;

#if SUNXI_CRASHDUMP_KEY_DEBUG
	static int count;

	count++;

	pr_info("Test %d:key code(%d) value(%d),(up:%d,down:%d),lpct(%d),vop(%ld)\n", count,
		code, value, volup_p, voldown_p, loopcount, vol_pressed);
#endif

	/* Wish enter crashdump through external trigger.
	 * method: hold the volume up and volume down then
	 * press power key five times */
	if (value) {
		if (code == KEY_VOLUMEUP)
			volup_p = CRASHDUMP_MAGIC_VOLUP;
		if (code == KEY_VOLUMEDOWN)
			voldown_p = CRASHDUMP_MAGIC_VOLDN;

		if ((volup_p == CRASHDUMP_MAGIC_VOLUP) && (voldown_p == CRASHDUMP_MAGIC_VOLDN)) {
			if (!vol_pressed)
				vol_pressed = jiffies;

			if (code == KEY_POWER) {
				++loopcount;
				pr_info("%s: Crash key count : %d,vol_pressed:%ld\n", __func__,
					loopcount, vol_pressed);
				if (time_before(jiffies, vol_pressed + 5 * HZ)) {
					if (loopcount == 5)
						panic("sunxi crashdump by key");
				} else {
					pr_info("%s: exceed 5s(%u) between power key and volume up/volume down key 5 times\n",
						__func__, jiffies_to_msecs(jiffies - vol_pressed));
					volup_p = 0;
					voldown_p = 0;
					loopcount = 0;
					vol_pressed = 0;
				}
			}
		}
	} else {
		if (code == KEY_VOLUMEUP) {
			volup_p = 0;
			loopcount = 0;
			vol_pressed = 0;
		}
		if (code == KEY_VOLUMEDOWN) {
			voldown_p = 0;
			loopcount = 0;
			vol_pressed = 0;
		}
	}
}

static int crashdump_connect(struct input_handler *handler,
			 struct input_dev *dev,
			 const struct input_device_id *id)
{
	struct input_handle *crashdump_handle;
	int error;

	crashdump_handle = kzalloc(sizeof(*crashdump_handle), GFP_KERNEL);
	if (!crashdump_handle)
		return -ENOMEM;

	crashdump_handle->dev = dev;
	crashdump_handle->handler = handler;
	crashdump_handle->name = "crashdump";

	error = input_register_handle(crashdump_handle);
	if (error) {
		pr_err("Failed to register input sysrq handler, error %d\n",
			error);
		goto err_free;
	}

	error = input_open_device(crashdump_handle);
	if (error) {
		pr_err("Failed to open input device, error %d\n", error);
		goto err_unregister;
	}

	return 0;

err_unregister:
	input_unregister_handle(crashdump_handle);
err_free:
	kfree(crashdump_handle);
	return error;
}

static void crashdump_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}


static void crashdump_event(struct input_handle *handle,
	unsigned int type, unsigned int code, int value)
{
	if (type == EV_KEY && code != BTN_TOUCH)
		sunxi_crashdump_key(code, value);
}

static const struct input_device_id sysdump_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{},
};

static struct input_handler sysdump_handler = {
	.event = crashdump_event,
	.connect	= crashdump_connect,
	.disconnect	= crashdump_disconnect,
	.name = "crashdump_key",
	.id_table	= sysdump_ids,
};

int sunxi_crashdump_key_register(void)
{
	if (input_register_handler(&sysdump_handler)) {
		pr_err("register sunxi crashdump key failed.\n");
		return -1;
	}

	return 0;
}

void sunxi_crashdump_key_unregister(void)
{
	input_unregister_handler(&sysdump_handler);
}

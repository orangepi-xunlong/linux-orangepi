/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef SUNXI_HID_H
#define SUNXI_HID_H

#include <linux/types.h>
#include <linux/usb/g_hid.h>
#include <uapi/linux/input-event-codes.h>

/* 0: Multi-Touch, 1: Single-Touch */
#define HID_TOUCH_MODE (0)
#if HID_TOUCH_MODE
#define HID_REPORT_DESC_LEN (13)
#else
#define HID_REPORT_DESC_LEN (52)
#endif

extern const char *libhid_version;
extern struct hidg_func_descriptor hidg_func_desc;
extern unsigned char g_hid_data[HID_REPORT_DESC_LEN];
void libhid_handle_abs_event(unsigned int code, int value);
void libhid_handle_key_event(unsigned int code, int value);
void libhid_handle_syn_event(unsigned int code, int value);
void libhid_set_input_range(int x_min, int x_max, int y_min, int y_max);
void libhid_platform_verify(void);

#endif /* SUNXI_HID_H */
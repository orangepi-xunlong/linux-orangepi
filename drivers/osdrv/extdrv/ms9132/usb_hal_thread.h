/* Copyright (C) 2023 MacroSilicon Technology Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * usb_hal_thread.h -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#ifndef __USB_HAL_THREAD_H__
#define __USB_HAL_THREAD_H__

#define USB_HAL_BUF_TIMEOUT 2000
#define USB_HAL_BUF_WAIT_TIME 20

struct usb_hal_dev;

int usb_hal_state_machine_entry(void *data);
void usb_hal_stop_thread(struct usb_hal_dev *usb_dev);

#endif
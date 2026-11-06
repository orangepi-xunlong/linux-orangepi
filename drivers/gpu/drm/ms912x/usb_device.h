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
 * usb_device.h -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#ifndef __MSDISP_USB_DEVICE_H__
#define __MSDISP_USB_DEVICE_H__z

#include "hal_adaptor.h"

#define MSDISP_9132_VENDOR 0x345f
#define MSDISP_9132_PRODUCT 0x9132

extern const struct msdisp_hal_id ms9132_id;
extern struct msdisp_hal_dev ms9132_dev;

#endif
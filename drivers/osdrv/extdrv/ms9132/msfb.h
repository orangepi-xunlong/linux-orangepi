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
 * msfb.h -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#ifndef __MSFB_H__
#define __MSFB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif

#include <linux/fb.h>

#define MSFB_HDMI_HPD_DISCONNECTED 0
#define MSFB_HDMI_HPD_CONNECTED 1

#define FBIOGET_HDMI_HPD_MSFB _IOR('F', 0x90, unsigned int)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
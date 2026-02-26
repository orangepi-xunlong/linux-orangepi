/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb msg center.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/unaligned.h>

#include  "../include/sunxi_usb_config.h"
#include  "usb_manager.h"
#include  "usbc_platform.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"

#include <linux/power_supply.h>

static struct usb_msg_center_info g_center_info;

enum sw_usb_role get_usb_role(void)
{
	return g_center_info.role;
}

static void set_usb_role(
		struct usb_msg_center_info *center_info,
		enum sw_usb_role role)
{
	center_info->role = role;
}

void set_usb_role_ex(enum sw_usb_role role)
{
	set_usb_role(&g_center_info, role);
}

void hw_insmod_usb_host(void)
{
	g_center_info.msg.hw_insmod_host = 1;
}

void hw_rmmod_usb_host(void)
{
	g_center_info.msg.hw_rmmod_host = 1;
}

void hw_insmod_usb_device(void)
{
	g_center_info.msg.hw_insmod_device = 1;
}

void hw_rmmod_usb_device(void)
{
	g_center_info.msg.hw_rmmod_device = 1;
}

static void modify_msg(struct usb_msg *msg)
{
	if (msg->hw_insmod_host && msg->hw_rmmod_host) {
		msg->hw_insmod_host = 0;
		msg->hw_rmmod_host  = 0;
	}

	if (msg->hw_insmod_device && msg->hw_rmmod_device) {
		msg->hw_insmod_device = 0;
		msg->hw_rmmod_device  = 0;
	}
}

static void insmod_host_driver(struct usb_msg_center_info *center_info)
{
#if IS_ENABLED(CONFIG_TYPEC)
	struct usb_cfg *cfg = &g_usb_cfg;
	struct typec_port *port = cfg->port.typec_port;
	struct typec_partner_desc desc;
	desc.accessory = TYPEC_ACCESSORY_NONE; /* XXX: handle accessories */
	desc.identity = NULL;
#endif
	DMSG_INFO("insmod_host_driver\n\n");

	set_usb_role(center_info, SW_USB_ROLE_HOST);

#if IS_ENABLED(CONFIG_ARCH_SUN8IW6)
#if IS_ENABLED(CONFIG_USB_SUNXI_HCD0)
	sunxi_usb_host0_enable();
#endif
#else
	#if IS_ENABLED(CONFIG_USB_SUNXI_EHCI0)
		sunxi_usb_enable_ehci(0);
	#endif

	#if IS_ENABLED(CONFIG_USB_SUNXI_OHCI0)
		sunxi_usb_enable_ohci(0);
	#endif
#endif

#if IS_ENABLED(CONFIG_TYPEC)
	typec_set_data_role(port, TYPEC_HOST);
	typec_set_pwr_role(port, TYPEC_SOURCE);
	typec_set_vconn_role(port, TYPEC_SOURCE);

	cfg->port.partner = typec_register_partner(port, &desc);
	cfg->port.connected = true;
#endif
}

static void rmmod_host_driver(struct usb_msg_center_info *center_info)
{
#if IS_ENABLED(CONFIG_TYPEC)
	struct usb_cfg *cfg = &g_usb_cfg;
	struct typec_partner *partner = cfg->port.partner;
#endif
	DMSG_INFO("rmmod_host_driver\n\n");

#if IS_ENABLED(CONFIG_ARCH_SUN8IW6)
#if IS_ENABLED(CONFIG_USB_SUNXI_HCD0)
{
	int ret = 0;

	ret = sunxi_usb_host0_disable();
	if (ret != 0) {
		DMSG_ERR("err: disable hcd0 failed\n");
		return;
	}
}
#endif
#else
	#if IS_ENABLED(CONFIG_USB_SUNXI_EHCI0)
		sunxi_usb_disable_ehci(0);
	#endif




	#if IS_ENABLED(CONFIG_USB_SUNXI_OHCI0)
		sunxi_usb_disable_ohci(0);
	#endif
#endif
	set_usb_role(center_info, SW_USB_ROLE_NULL);
#if IS_ENABLED(CONFIG_TYPEC)
	if (cfg->port.connected) {
		typec_unregister_partner(partner);
		cfg->port.connected = false;
	}
#endif
}

static void insmod_device_driver(struct usb_msg_center_info *center_info)
{

#if IS_ENABLED(CONFIG_DUAL_ROLE_USB_INTF)
	struct usb_cfg *cfg = &g_usb_cfg;
	struct dual_role_phy_instance *dual_role = cfg->port.dual_role;
#endif
#if IS_ENABLED(CONFIG_TYPEC)
	struct usb_cfg *cfg = &g_usb_cfg;
	struct typec_port *port = cfg->port.typec_port;
	struct typec_partner_desc desc;
	desc.accessory = TYPEC_ACCESSORY_NONE; /* XXX: handle accessories */
	desc.identity = NULL;

#endif
	DMSG_INFO("insmod_device_driver\n\n");

	set_usb_role(center_info, SW_USB_ROLE_DEVICE);

#if IS_ENABLED(CONFIG_USB_SUNXI_UDC0)
	sunxi_usb_device_enable();
#endif

#if IS_ENABLED(CONFIG_DUAL_ROLE_USB_INTF)
	dual_role_instance_changed(dual_role);
#endif

#if IS_ENABLED(CONFIG_TYPEC)
	typec_set_data_role(port, TYPEC_DEVICE);
	typec_set_pwr_role(port, TYPEC_SINK);

	cfg->port.partner = typec_register_partner(port, &desc);
	cfg->port.connected = true;
#endif

}

static void rmmod_device_driver(struct usb_msg_center_info *center_info)
{
#if IS_ENABLED(CONFIG_DUAL_ROLE_USB_INTF)
	struct usb_cfg *cfg = &g_usb_cfg;
	struct dual_role_phy_instance *dual_role = cfg->port.dual_role;
#endif
#if IS_ENABLED(CONFIG_TYPEC)
	struct usb_cfg *cfg = &g_usb_cfg;
	struct typec_partner *partner = cfg->port.partner;
#endif

	DMSG_INFO("rmmod_device_driver\n\n");

	set_usb_role(center_info, SW_USB_ROLE_NULL);

#if IS_ENABLED(CONFIG_USB_SUNXI_UDC0)
	sunxi_usb_device_disable();
#endif

#if IS_ENABLED(CONFIG_DUAL_ROLE_USB_INTF)
	dual_role_instance_changed(dual_role);
#endif
#if IS_ENABLED(CONFIG_TYPEC)
	if (cfg->port.connected) {
		typec_unregister_partner(partner);
		cfg->port.connected = false;
	}
#endif
}

static void do_usb_role_null(struct usb_msg_center_info *center_info)
{
	if (center_info->msg.hw_insmod_host) {

		/*
		 * If the current SOC has bcten-gpio,
		 * need to set this gpio to `HIGH` for the host driver to work properly.
		 */
		if (center_info->bcten_gpiod)
			gpiod_set_value(center_info->bcten_gpiod, 1);

		insmod_host_driver(center_info);
		center_info->msg.hw_insmod_host = 0;
		atomic_set(&center_info->cfg->det_flag, 0);

		goto end;
	}

	if (center_info->msg.hw_insmod_device) {
		insmod_device_driver(center_info);
		center_info->msg.hw_insmod_device = 0;
		atomic_set(&center_info->cfg->det_flag, 0);

		goto end;
	}

end:
	memset(&center_info->msg, 0, sizeof(struct usb_msg));
}

static void do_usb_role_host(struct usb_msg_center_info *center_info)
{
	if (center_info->msg.hw_rmmod_host) {
		rmmod_host_driver(center_info);
		center_info->msg.hw_rmmod_host = 0;
		atomic_set(&center_info->cfg->det_flag, 0);

		/*
		 * If the current SOC has bcten-gpio,
		 * neet to set this gpio to `LOW`, device driver can be loaded.
		 */
		if (center_info->bcten_gpiod)
			gpiod_set_value(center_info->bcten_gpiod, 0);

		goto end;
	}

end:
	memset(&center_info->msg, 0, sizeof(struct usb_msg));
}

static void do_usb_role_device(struct usb_msg_center_info *center_info)
{
	if (center_info->msg.hw_rmmod_device) {
		rmmod_device_driver(center_info);
		center_info->msg.hw_rmmod_device = 0;
		atomic_set(&center_info->cfg->det_flag, 0);

		goto end;
	}

end:
	memset(&center_info->msg, 0, sizeof(struct usb_msg));
}

void usb_msg_center(struct usb_cfg *cfg)
{
	enum sw_usb_role role = SW_USB_ROLE_NULL;
	struct usb_msg_center_info *center_info = &g_center_info;

	center_info->cfg = cfg;

	/* receive massage */
	modify_msg(&center_info->msg);

	/* execute cmd */
	role = get_usb_role();

	DMSG_DBG_MANAGER("role=%d\n", get_usb_role());

	switch (role) {
	case SW_USB_ROLE_NULL:
		do_usb_role_null(center_info);
		break;

	case SW_USB_ROLE_HOST:
		do_usb_role_host(center_info);
		break;

	case SW_USB_ROLE_DEVICE:
		do_usb_role_device(center_info);
		break;

	default:
		DMSG_ERR("ERR: unknown role(%x)\n", role);
	}
}

s32 usb_msg_center_init(struct device *dev)
{
	struct usb_msg_center_info *center_info = &g_center_info;

	memset(center_info, 0, sizeof(struct usb_msg_center_info));

	center_info->bcten_gpiod = devm_gpiod_get_optional(dev, "bcten", GPIOD_OUT_LOW);
	if (IS_ERR_OR_NULL(center_info->bcten_gpiod))
		DMSG_INFO("OTG can't find bcten-gpio, use default mode.\n");

	return 0;
}

s32 usb_msg_center_exit(void)
{
	struct usb_msg_center_info *center_info = &g_center_info;

	memset(center_info, 0, sizeof(struct usb_msg_center_info));
	return 0;
}

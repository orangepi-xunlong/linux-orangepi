// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * OTG support for Ky x1 SoCs
 *
 * Copyright (c) 2023 Ky Inc.
 */

#ifndef	__MV_USB_OTG_CONTROLLER__
#define	__MV_USB_OTG_CONTROLLER__

#include <linux/gpio.h>
#include <linux/reset.h>
#include <linux/types.h>
#include <linux/extcon.h>
#include <linux/usb/otg.h>
#include <linux/usb/role.h>

/* Command Register Bit Masks */
#define USBCMD_RUN_STOP			(0x00000001)
#define USBCMD_CTRL_RESET		(0x00000002)


#define CAPLENGTH_MASK		(0xff)


#define VUSBHS_MAX_PORTS	8

struct mv_otg_regs {
	u32 usbcmd;		/* Command register */
	u32 usbsts;		/* Status register */
	u32 usbintr;		/* Interrupt enable */
	u32 frindex;		/* Frame index */
	u32 reserved1[1];
	u32 deviceaddr;		/* Device Address */
	u32 eplistaddr;		/* Endpoint List Address */
	u32 ttctrl;		/* HOST TT status and control */
	u32 burstsize;		/* Programmable Burst Size */
	u32 txfilltuning;	/* Host Transmit Pre-Buffer Packet Tuning */
	u32 reserved[4];
	u32 epnak;		/* Endpoint NAK */
	u32 epnaken;		/* Endpoint NAK Enable */
	u32 configflag;		/* Configured Flag register */
	u32 portsc[VUSBHS_MAX_PORTS];	/* Port Status/Control x, x = 1..8 */
	u32 otgsc;
	u32 usbmode;		/* USB Host/Device mode */
	u32 epsetupstat;	/* Endpoint Setup Status */
	u32 epprime;		/* Endpoint Initialize */
	u32 epflush;		/* Endpoint De-initialize */
	u32 epstatus;		/* Endpoint Status */
	u32 epcomplete;		/* Endpoint Interrupt On Complete */
	u32 epctrlx[16];	/* Endpoint Control, where x = 0.. 15 */
};

struct mv_otg {
	struct usb_phy phy;
	struct usb_phy *outer_phy;

	/* set role lock */
	spinlock_t lock;

	/* base address */
	void __iomem *cap_regs;
	void __iomem *wakeup_reg;
	struct mv_otg_regs __iomem *op_regs;

	struct platform_device *pdev;
	int irq;
	u32 irq_status;
	u32 irq_en;

	struct delayed_work work;
	struct workqueue_struct *qwork;

	struct mv_usb_platform_data *pdata;
	struct notifier_block notifier;
	struct notifier_block notifier_charger;

	unsigned int active;
	unsigned int host_remote_wakeup;
	unsigned int clock_gating;
	struct clk *clk;
	struct reset_control *reset;
	struct gpio_desc *vbus_gpio;
	unsigned int charger_type;

	/* user control otg support */
	enum usb_dr_mode dr_mode;

	/* for vbus detection */
	struct extcon_specific_cable_nb vbus_dev;
	/* for id detection */
	struct extcon_specific_cable_nb id_dev;
	struct extcon_dev *extcon;

	struct usb_role_switch *role_sw;
	enum usb_dr_mode role_switch_default_mode;

#define MV_OTG_ROLE_UNDEFINED 0
#define MV_OTG_ROLE_DEVICE_IDLE 1
#define MV_OTG_ROLE_DEVICE_ACTIVE 2
#define MV_OTG_ROLE_HOST_ACTIVE 3
	u32 desired_otg_role;
	u32 current_otg_role;

	struct regulator *vbus_otg;
};

#endif

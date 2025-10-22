// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dwc3-sunxi-plat.c - OF glue layer for allwinner platform
 *
 * Copyright (c) 2022 Allwinner Technology Co., Ltd.
 *
 * dwc3-of-simple.c - OF glue layer for simple integrations
 *
 * Copyright (c) 2015 Texas Instruments Incorporated - https://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * This is a combination of the old dwc3-qcom.c by Ivan T. Ivanov
 * <iivanov@mm-sol.com> and the original patch adding support for Xilinx' SoC
 * by Subbaraya Sundeep Bhatta <subbaraya.sundeep.bhatta@xilinx.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/extcon.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/usb.h>
#include <linux/usb/typec.h>
#include <linux/regulator/consumer.h>
#include <linux/power_supply.h>
#include <linux/of_gpio.h>

#include <sunxi-log.h>
#include "dwc3-sunxi.h"

#define  KEY_USB_DETECT_TYPE            "usb_detect_type"
#define  KEY_USB_DETECT_MODE            "usb_detect_mode"
#define  KEY_USB_GMA340_OE_GPIO         "usb_gma340_oe_gpio"
#define  KEY_USB_GMA340_SEL_GPIO        "usb_gma340_sel_gpio"

/* dwc3 mode */
typedef enum dwc3_mode {
	DWC3_HOST_MODE = 0,
	DWC3_DEVICE_MODE,
	DWC3_UNKNOWN_MODE,
} dwc3_mode_t;

/* dwc3 typec side */
typedef enum dwc3_side {
	DWC3_CC2_SIDE = 0,
	DWC3_CC1_SIDE,
	DWC3_UNKNOWN_SIDE,
} dwc3_side_t;

/* 0: pmu detect, 1: tcpc detect */
enum dwc3_detect_type {
	DWC3_DETECT_TYPE_PMU = 0,
	DWC3_DETECT_TYPE_TCPM,
	DWC3_DETECT_TYPE_GPIO,
	DWC3_DETECT_TYPE_UNKNOWN,
};

/* 0: notify scan mode, 1: thread scan mode */
enum dwc3_detect_mode {
	DWC3_DETECT_MODE_NOTIFY = 0,
	DWC3_DETECT_MODE_THREAD,
	DWC3_DETECT_MODE_EXTCON,
	DWC3_DETECT_MODE_UNKNOWN,
};

typedef struct dwc3_cfg {
	__u32 enable;				/* port valid */
	__u32 usbc_no;				/* usb controller number */
	enum dwc3_detect_type detect_type;	/* usb detect type */
	enum dwc3_detect_mode detect_mode;	/* usb detect mode */

	/* GMA340 support */
	struct gpio_config gma340_oe_gpio_set;
	const char *gma340_oe_name;
	__u32 gma340_oe_gpio_valid;
	struct gpio_config gma340_sel_gpio_set;
	const char *gma340_sel_name;
	__u32 gma340_sel_gpio_valid;

	/* Type-C support */
	struct power_supply *psy; /* pmu type */

} dwc3_cfg_t;

struct dwc3_sunxi_plat {
	struct device		*dev;
	struct clk_bulk_data	*clks;
	int			num_clocks;
	struct reset_control	*resets;
	bool			need_reset;
	struct dwc3		*dwc;
	struct regulator	*vbus;
	struct mutex		lock;
	struct notifier_block	det_nb;
	struct work_struct	det_work;
	struct mutex		det_lock;
	atomic_t		det_flag;
	struct work_struct	resume_work;
	struct notifier_block	pm_nb;
	struct work_struct	pm_work;
	atomic_t		pm_flag;
	struct extcon_dev	*edev;
	struct notifier_block	extcon_nb;
	bool			vbus_shared_quirk;
	bool			hcgen2_phygen1_quirk;
	bool			u2drd_u3host_quirk;
	bool			inv_sync_hdr_quirk;
	dwc3_side_t		old_side;
	dwc3_mode_t		old_mode;
	dwc3_cfg_t		cfg;
};

static int dwc3_typec_run_flag;
static int dwc3_mode_stop_flag;
static int dwc3_side_stop_flag;
static atomic_t dwc3_thread_suspend_flag;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static void dwc3_set_host(struct dwc3_sunxi_plat *dwc3, bool enable);
#include "dwc3-debug.c"
#include "xhci-debug.c"
#else
static inline void dwc3_sunxi_debug_init(struct dwc3_sunxi_plat *dwc3) { }
static inline void dwc3_sunxi_debug_exit(struct dwc3_sunxi_plat *dwc3) { }
static inline void dwc3_debug_init(struct dwc3 *dwc) { }
static inline void dwc3_debug_exit(struct dwc3 *dwc) { }
static inline void xhci_debug_init(struct dwc3 *dwc) { }
static inline void xhci_debug_exit(struct dwc3 *dwc) { }
#endif

static void dwc3_hw_init(struct dwc3_sunxi_plat *dwc3)
{
	struct dwc3 *dwc = dwc3->dwc;
	u32 reg;

	reg = dwc3_sunxi_readl(dwc->regs, DWC3_LLUCTL);
	if (dwc3->hcgen2_phygen1_quirk)
		reg |= DWC3_LLUCTL_FORCE_GEN1;
	if (dwc3->inv_sync_hdr_quirk)
		reg |= DWC3_LLUCTL_INV_SYNC_HDR;
	dwc3_sunxi_writel(dwc->regs, DWC3_LLUCTL, reg);
}

static void dwc3_typec_det_event(struct dwc3_sunxi_plat *dwc3);
static void dwc3_sunxi_resume_work(struct work_struct *work)
{
	struct dwc3_sunxi_plat *dwc3 = container_of(work, struct dwc3_sunxi_plat, resume_work);
	struct dwc3 *dwc = dwc3->dwc;
	u32 count = 0;
	bool need_loop = false;

	if (dwc->gadget || dwc->xhci) {
		if (dwc3->hcgen2_phygen1_quirk || dwc3->inv_sync_hdr_quirk)
			need_loop = true;
	}

	while (need_loop) {
		u32 reg = dwc3_sunxi_readl(dwc->regs, DWC3_GUID);

		/* NOTE: The dwc3_core_init Write Linux Version Code to our GUID register. */
		if (reg == LINUX_VERSION_CODE) {
			/* Condition 1: Wait for DWC3 Core Restore LLUCTL register. */
			if (dwc3_sunxi_readl(dwc->regs, DWC3_LLUCTL)) {
				dwc3_hw_init(dwc3);
				break;
			}
		}

		usleep_range(1, 2);
		if (++count > 50000) {
			sunxi_err(dwc3->dev, "wait for DWC3 Core resume 50ms timeout!\n");
			break;
		}
	}

	sunxi_debug(dwc3->dev, "wait for DWC3 Core resume %d us!\n", count);

	/* handle peripheral plugged or reverse again after unplugged during hibernation. */
	if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_NOTIFY) {
		dwc3_typec_det_event(dwc3);
		/* restore det_flag for auto detect */
		atomic_set(&dwc3->det_flag, 1);
	}

	atomic_set(&dwc3->pm_flag, 0);
}

/**
 * NOTE: Here are some non-standard features for Android,
 * it's possibly incompatible with later Linux Version.
 */
#if IS_MODULE(CONFIG_DWC3_SUNXI_PLAT) && (IS_ENABLED(CONFIG_USB_DWC3_HOST) \
	|| IS_ENABLED(CONFIG_USB_DWC3_DUAL_ROLE))
#include "host.c"
void dwc3_enable_susphy(struct dwc3 *dwc, bool enable)
{
	u32 reg;

	reg = dwc3_sunxi_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
	if (enable && !dwc->dis_u3_susphy_quirk)
		reg |= DWC3_GUSB3PIPECTL_SUSPHY;
	else
		reg &= ~DWC3_GUSB3PIPECTL_SUSPHY;

	dwc3_sunxi_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

	reg = dwc3_sunxi_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
	if (enable && !dwc->dis_u2_susphy_quirk)
		reg |= DWC3_GUSB2PHYCFG_SUSPHY;
	else
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;

	dwc3_sunxi_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);
}

void dwc3_set_prtcap(struct dwc3 *dwc, u32 mode)
{
	u32 reg;

	reg = dwc3_sunxi_readl(dwc->regs, DWC3_GCTL);
	reg &= ~(DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG));
	reg |= DWC3_GCTL_PRTCAPDIR(mode);
	dwc3_sunxi_writel(dwc->regs, DWC3_GCTL, reg);

	dwc->current_dr_role = mode;
}
#endif
static void dwc3_set_host(struct dwc3_sunxi_plat *dwc3, bool enable)
{
	int ret = 0;

	/**
	 * NOTE: When the port use this kind of combo for USB2's DRD and USB3's HOST,
	 * we recommend that you use this quirk for xhci hcd dynamic loading.
	 */
	if (dwc3->u2drd_u3host_quirk) {
		mutex_lock(&dwc3->lock);
		if (enable) {
			if (!dwc3->dwc->xhci) {
				pm_runtime_get_sync(dwc3->dwc->dev);
#if IS_ENABLED(CONFIG_AW_INNO_COMBOPHY)
				pm_runtime_get_sync(dwc3->dwc->usb3_generic_phy->dev.parent);
				atomic_notifier_call_chain(&inno_subsys_notifier_list, 1, NULL);
#endif
				phy_init(dwc3->dwc->usb2_generic_phy);
				phy_init(dwc3->dwc->usb3_generic_phy);
				ret = phy_power_on(dwc3->dwc->usb2_generic_phy);
				if (ret)
					sunxi_err(dwc3->dev, "failed to set phy power on\n");
				ret = phy_power_on(dwc3->dwc->usb3_generic_phy);
				if (ret)
					sunxi_err(dwc3->dev, "failed to set phy power on\n");
				dwc3_set_prtcap(dwc3->dwc, DWC3_GCTL_PRTCAP_HOST);
				phy_set_mode(dwc3->dwc->usb2_generic_phy, PHY_MODE_USB_HOST);
				phy_set_mode(dwc3->dwc->usb3_generic_phy, PHY_MODE_USB_HOST);
				ret = dwc3_host_init(dwc3->dwc);
				if (ret)
					sunxi_err(dwc3->dev, "failed to initialize host\n");
				else
					dwc3_hw_init(dwc3);
				xhci_debug_init(dwc3->dwc);
			}
		} else {
			if (dwc3->dwc->xhci) {
				xhci_debug_exit(dwc3->dwc);
				dwc3_host_exit(dwc3->dwc);
				dwc3->dwc->xhci = NULL;
				phy_power_off(dwc3->dwc->usb3_generic_phy);
				phy_power_off(dwc3->dwc->usb2_generic_phy);
				phy_exit(dwc3->dwc->usb2_generic_phy);
				phy_exit(dwc3->dwc->usb3_generic_phy);
#if IS_ENABLED(CONFIG_AW_INNO_COMBOPHY)
				atomic_notifier_call_chain(&inno_subsys_notifier_list, 0, NULL);
				pm_runtime_put_sync(dwc3->dwc->usb3_generic_phy->dev.parent);
#endif
				pm_runtime_put_sync(dwc3->dwc->dev);
			}
		}
		mutex_unlock(&dwc3->lock);
	}
}

static void dwc3_msleep(unsigned int msecs)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(msecs));
}

static __u32 PMUModeIn_debounce(struct power_supply *psy, union power_supply_propval *val)
{
	__u32 retry  = 0;
	__u32 time   = 10;
	union power_supply_propval temp;
	__u32 cnt    = 0;
	__u32 change = 1;	/* if have shake */
	int intval;

	/**
	 * try 10 times, if value is the same,
	 * then current read is valid; otherwise invalid.
	 */
	if (psy) {
		retry = time;
		while (retry--) {
			power_supply_get_property(psy,
						POWER_SUPPLY_PROP_SCOPE, &temp);

			intval = temp.intval & 0x3;
			intval -= 1;

			if (intval)
				cnt++;
		}

		/* 10 times, the value is all 0 or 1 */
		if ((cnt == time) || (cnt == 0))
			change = 0;
		else
			change = 1;
	}

	if (!change)
		*val = temp;

	sunxi_debug(NULL, "cnt = %x, change= %d, intval = %d\n", cnt, change, temp.intval);

	return change;
}

static __u32 PMUSideIn_debounce(struct power_supply *psy, union power_supply_propval *val)
{
	__u32 retry  = 0;
	__u32 time   = 10;
	union power_supply_propval temp;
	__u32 cnt    = 0;
	__u32 change = 1;	/* if have shake */
	int intval;

	/**
	 * try 10 times, if value is the same,
	 * then current read is valid; otherwise invalid.
	 */
	if (psy) {
		retry = time;
		while (retry--) {
			power_supply_get_property(psy,
						POWER_SUPPLY_PROP_SCOPE, &temp);

			intval = temp.intval & 0x4;

			if (intval)
				cnt++;
		}

		/* 10 times, the value is all 0 or 1 */
		if ((cnt == time) || (cnt == 0))
			change = 0;
		else
			change = 1;
	}

	if (!change)
		*val = temp;

	sunxi_debug(NULL, "cnt = %x, change= %d, intval = %d\n", cnt, change, temp.intval);

	return change;
}

static __u32 dwc3_pmu_get_cc_mode(struct dwc3_sunxi_plat *dwc3)
{
	enum dwc3_mode mode = DWC3_DEVICE_MODE;
	union power_supply_propval val;
	int intval;

	if (!PMUModeIn_debounce(dwc3->cfg.psy, &val)) {
		/* host or device mode */
		intval = val.intval & 0x3;
		if (--intval)
			mode = DWC3_DEVICE_MODE;
		else
			mode = DWC3_HOST_MODE;

		/* don't assignment the old_mode here */
		/* dwc3->old_mode = mode; */

	} else {
		mode = dwc3->old_mode; /* last mode status */
	}

	sunxi_debug(dwc3->dev, "old_side = %d, old_mode:%d, mode = %d\n",
		dwc3->old_side, dwc3->old_mode, mode);

	return mode;
}

static __u32 dwc3_pmu_get_cc_side(struct dwc3_sunxi_plat *dwc3)
{
	enum dwc3_side side = DWC3_CC2_SIDE;
	union power_supply_propval val;

	if (!PMUSideIn_debounce(dwc3->cfg.psy, &val)) {
		/* positive or negative side */
		if (val.intval & 0x4)
			side = DWC3_CC1_SIDE;
		else
			side = DWC3_CC2_SIDE;

		/* don't assignment the old_side here */
		/* dwc3->old_side = side; */

	} else {
		side = dwc3->old_side; /* last side status */
	}

	sunxi_debug(dwc3->dev, "old_mode = %d, old_side:%d, side = %d\n",
		dwc3->old_mode, dwc3->old_side, side);

	return side;
}

static void dwc3_set_vbus(struct dwc3_sunxi_plat *dwc3, int is_on)
{
	int ret = -1;

	/**
	 * NOTE: When the same port share vbus for USB2's HCD and mine,
	 * we recommend that one of drivers handle vbus.
	 */
	if (dwc3->vbus && !dwc3->vbus_shared_quirk) {
		if (is_on) {
			ret = regulator_enable(dwc3->vbus);
			if (ret)
				sunxi_err(dwc3->dev, "enable vbus failed\n");
		} else {
			if (regulator_is_enabled(dwc3->vbus)) {
				ret = regulator_disable(dwc3->vbus);
				if (ret)
					sunxi_err(dwc3->dev, "force disable vbus failed\n");
			}
		}
	}
}

static void dwc3_gma340_config(struct dwc3_sunxi_plat *dwc3, int is_on)
{
	dwc3_cfg_t *cfg = &dwc3->cfg;

	if (cfg->gma340_oe_gpio_valid)
		gpio_set_value(cfg->gma340_oe_gpio_set.gpio, 0); /* 0: switch on, 1: switch off */
	if (cfg->gma340_sel_gpio_valid)
		gpio_set_value(cfg->gma340_sel_gpio_set.gpio, is_on); /* 0: SW1, 1:SW2 */

}

static void dwc3_typec_mode_scan(struct dwc3_sunxi_plat *dwc3)
{
	dwc3_mode_t mode = DWC3_UNKNOWN_MODE;

	if (dwc3->cfg.detect_type == DWC3_DETECT_TYPE_PMU)
		mode = dwc3_pmu_get_cc_mode(dwc3);

	/* NOTE: it's the core of dwc3_typec_mode_scan. */
	if (dwc3->old_mode == mode)
		return;

	sunxi_debug(dwc3->dev, "old:%d, mode:%d, set vbus %s\n", dwc3->old_mode, mode,
		mode == DWC3_HOST_MODE ? "on" : "off");

	if (mode == DWC3_HOST_MODE) {
		dwc3_set_host(dwc3, true);
		dwc3_set_vbus(dwc3, 1);
	} else {
		/* Give up exit host early when standby. */
		if (atomic_read(&dwc3->pm_flag) && dwc3->vbus_shared_quirk)
			return;
		dwc3_set_vbus(dwc3, 0);
		dwc3_set_host(dwc3, false);
	}

	dwc3->old_mode = mode;

	/* exit the cycle once detected */
	atomic_set(&dwc3->det_flag, 0);
}

static void dwc3_typec_side_scan(struct dwc3_sunxi_plat *dwc3)
{
	dwc3_side_t side = DWC3_UNKNOWN_SIDE;

	if (dwc3->cfg.detect_type == DWC3_DETECT_TYPE_PMU)
		side = dwc3_pmu_get_cc_side(dwc3);

	/* NOTE: it's the core of dwc3_typec_side_scan. */
	if (dwc3->old_side == side)
		return;

	sunxi_debug(dwc3->dev, "old:%d, side:%d, config gma %s\n", dwc3->old_side, side,
		side == DWC3_CC1_SIDE ? "SW1" : "SW2");

	if (side == DWC3_CC1_SIDE)
		dwc3_gma340_config(dwc3, 0);
	else
		dwc3_gma340_config(dwc3, 1);
	phy_set_mode_ext(dwc3->dwc->usb3_generic_phy, PHY_MODE_USB_HOST_SS,
			 side == DWC3_CC1_SIDE ? TYPEC_ORIENTATION_NORMAL : TYPEC_ORIENTATION_REVERSE);

	dwc3->old_side = side;
}

static int dwc3_typec_scan_mode_thread(void *pArg)
{
	struct dwc3_sunxi_plat *dwc3 = pArg;

	while (dwc3_typec_run_flag) {
		if (kthread_should_stop())
			break;

		dwc3_msleep(500); /* 1s ??? maybe 500ms is better */
		if (atomic_read(&dwc3_thread_suspend_flag))
			continue;
		dwc3_typec_mode_scan(dwc3);
	}
	dwc3_mode_stop_flag = 1;

	return 0;
}

static int dwc3_typec_scan_side_thread(void *pArg)
{
	struct dwc3_sunxi_plat *dwc3 = pArg;

	while (dwc3_typec_run_flag) {
		if (kthread_should_stop())
			break;

		dwc3_msleep(50); /* 1s ??? maybe 50ms is better */
		if (atomic_read(&dwc3_thread_suspend_flag))
			continue;
		dwc3_typec_side_scan(dwc3);
	}
	dwc3_side_stop_flag = 1;

	return 0;
}

/**
 * NOTE: Sometimes we need to detect manually when reboot or insmod self,
 * the interface can meet this requirement.
 */
static void dwc3_typec_det_event(struct dwc3_sunxi_plat *dwc3)
{
	dwc3_typec_side_scan(dwc3);

	/* make sure signal stable */
	usleep_range(5000, 5500);

	dwc3_typec_mode_scan(dwc3);
}

static int dwc3_typec_det_notifier(struct notifier_block *nb, unsigned long event, void *p)
{
	struct dwc3_sunxi_plat *dwc3 = container_of(nb, struct dwc3_sunxi_plat, det_nb);

	sunxi_debug(dwc3->dev, "detect event %s\n", p ? (const char *)p : "unknown");
	/* *
	 * NOTE: event:
	 * 0: typec in/out - otg cable;
	 * 1: usb   in/out - not handle at present.
	 * Only for axp2202
	 */
	if (event)
		return NOTIFY_DONE;

	schedule_work(&dwc3->det_work);

	return NOTIFY_DONE;
}

static void dwc3_typec_det_work(struct work_struct *work)
{
	struct dwc3_sunxi_plat *dwc3 = container_of(work, struct dwc3_sunxi_plat, det_work);
	unsigned int count = 0;

	mutex_lock(&dwc3->det_lock);

	while (atomic_read(&dwc3->det_flag)) {
		if (++count > 100) {
			sunxi_info(dwc3->dev,
				 "wait for DWC3 detect 1s timeout!\n");
			break;
		}
		dwc3_typec_det_event(dwc3);
		/* The loop may as well delay 5ms. */
		usleep_range(5000, 5500);
	}

	/* restore det_flag for next detect */
	atomic_set(&dwc3->det_flag, 1);

	mutex_unlock(&dwc3->det_lock);
}

static int dwc3_typec_pm_notifier(struct notifier_block *nb, unsigned long event, void *p)
{
	struct dwc3_sunxi_plat *dwc3 = container_of(nb, struct dwc3_sunxi_plat, pm_nb);

	sunxi_debug(dwc3->dev, "pm event %s\n", p ? (const char *)p : "unknown");
	schedule_work(&dwc3->pm_work);

	return NOTIFY_DONE;
}

static void dwc3_typec_pm_work(struct work_struct *work)
{
	struct dwc3_sunxi_plat *dwc3 = container_of(work, struct dwc3_sunxi_plat, pm_work);

	mutex_lock(&dwc3->det_lock);

	/* handle peripheral unplugged during hibernation. */
	if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_NOTIFY) {
		dwc3_typec_det_event(dwc3);
		/* restore det_flag for auto detect */
		atomic_set(&dwc3->det_flag, 1);
	}

	mutex_unlock(&dwc3->det_lock);
}

static void dwc3_extcon_set_mailbox(struct dwc3_sunxi_plat *dwc3,
				    enum dwc3_mode mode)
{
	dev_dbg(dwc3->dev, " mode %d\n", mode);

	if (dwc3->old_mode == mode)
		return;

	switch (mode) {
	case DWC3_HOST_MODE:
		dwc3_set_host(dwc3, true);
		dwc3_set_vbus(dwc3, 1);
		break;
	case DWC3_DEVICE_MODE:
		dwc3_set_vbus(dwc3, 0);
		dwc3_set_host(dwc3, false);
		break;
	case DWC3_UNKNOWN_MODE:
		break;
	default:
		dev_WARN(dwc3->dev, "invalid mode\n");
	}
	dwc3->old_mode = mode;
}

static int dwc3_extcon_mode_notifier(struct notifier_block *nb,
				     unsigned long event, void *ptr)
{
	struct dwc3_sunxi_plat *dwc3 = container_of(nb, struct dwc3_sunxi_plat, extcon_nb);

	dev_dbg(dwc3->dev, " mode event %lu\n", event);

	if (event)
		dwc3_extcon_set_mailbox(dwc3, DWC3_HOST_MODE);
	else
		dwc3_extcon_set_mailbox(dwc3, DWC3_DEVICE_MODE);

	return NOTIFY_DONE;
}

static int dwc3_sunxi_extcon_register(struct dwc3_sunxi_plat *dwc3)
{
	int			ret;
	struct device_node	*node = dwc3->dev->of_node;
	struct extcon_dev	*edev;

	if (of_property_read_bool(node, "extcon")) {
		edev = extcon_get_edev_by_phandle(dwc3->dev, 0);
		if (IS_ERR_OR_NULL(edev)) {
			dev_vdbg(dwc3->dev, "couldn't get extcon device\n");
			return -EPROBE_DEFER;
		}

		dwc3->extcon_nb.notifier_call = dwc3_extcon_mode_notifier;
		ret = devm_extcon_register_notifier(dwc3->dev, edev,
						    EXTCON_USB_HOST, &dwc3->extcon_nb);
		if (ret < 0)
			dev_vdbg(dwc3->dev, "failed to register notifier for USB-HOST\n");

		if (extcon_get_state(edev, EXTCON_USB_HOST) == true)
			dwc3_extcon_set_mailbox(dwc3, DWC3_HOST_MODE);
		else
			dwc3_extcon_set_mailbox(dwc3, DWC3_DEVICE_MODE);

		dwc3->edev = edev;
	}

	return 0;
}

static __s32 dwc3_script_parse(struct device_node *np, dwc3_cfg_t *cfg)
{
	int ret = -1;
	const char  *used_status;

	/* usbc enable */
	ret = of_property_read_string(np, "status", &used_status);
	if (ret) {
		sunxi_info(NULL, "get status is fail, %d\n", ret);
		cfg->enable = 0;
	} else if (!strcmp(used_status, "okay")) {
		cfg->enable = 1;
	} else {
		cfg->enable = 0;
	}

	/* usbc detect type */
	ret = of_property_read_u32(np, KEY_USB_DETECT_TYPE, &cfg->detect_type);
	if (ret) {
		cfg->detect_type = DWC3_DETECT_TYPE_UNKNOWN;
		sunxi_debug(NULL, "get usb_detect_type is fail, %d\n", -ret);
	}

	/* usbc detect mode */
	ret = of_property_read_u32(np, KEY_USB_DETECT_MODE, &cfg->detect_mode);
	if (ret) {
		cfg->detect_mode = DWC3_DETECT_MODE_UNKNOWN;
		sunxi_debug(NULL, "get usb_detect_mode is fail, %d\n", -ret);
	}

	/* usbc gma340-oe */
	ret = of_property_read_string(np, KEY_USB_GMA340_OE_GPIO, &cfg->gma340_oe_name);
	if (ret) {
		sunxi_debug(NULL, "get gma340-oe is fail, %d\n", -ret);
		cfg->gma340_oe_gpio_valid = 0;
	} else {
		/* get gma340-oe gpio */
		cfg->gma340_oe_gpio_set.gpio = of_get_named_gpio(np, KEY_USB_GMA340_OE_GPIO, 0);
		if (gpio_is_valid(cfg->gma340_oe_gpio_set.gpio))
			cfg->gma340_oe_gpio_valid = 1;
		else
			cfg->gma340_oe_gpio_valid = 0;
	}

	/* usbc gma340-sel */
	ret = of_property_read_string(np, KEY_USB_GMA340_SEL_GPIO, &cfg->gma340_sel_name);
	if (ret) {
		sunxi_debug(NULL, "get gma340-sel is fail, %d\n", -ret);
		cfg->gma340_sel_gpio_valid = 0;
	} else {
		/* get gma340-sel gpio */
		cfg->gma340_sel_gpio_set.gpio = of_get_named_gpio(np, KEY_USB_GMA340_SEL_GPIO, 0);
		if (gpio_is_valid(cfg->gma340_sel_gpio_set.gpio))
			cfg->gma340_sel_gpio_valid = 1;
		else
			cfg->gma340_sel_gpio_valid = 0;
	}

	return 0;
}

static int dwc3_alloc_pin(struct dwc3_sunxi_plat *dwc3)
{
	dwc3_cfg_t *cfg = &dwc3->cfg;
	int ret = -1;

	if (cfg->gma340_oe_gpio_valid) {
		ret = gpio_request(cfg->gma340_oe_gpio_set.gpio, NULL);
		if (ret != 0) {
			sunxi_info(NULL, "request gpio %d failed, %d\n", cfg->gma340_oe_gpio_set.gpio, ret);
		} else {
			gpio_direction_output(cfg->gma340_oe_gpio_set.gpio, 0);
		}
	}

	if (cfg->gma340_sel_gpio_valid) {
		ret = gpio_request(cfg->gma340_sel_gpio_set.gpio, NULL);
		if (ret != 0) {
			sunxi_info(NULL, "request gpio %d failed, %d\n", cfg->gma340_sel_gpio_set.gpio, ret);
		} else {
			gpio_direction_output(cfg->gma340_sel_gpio_set.gpio, 0);
		}
	}

	return ret;
}

static void dwc3_free_pin(struct dwc3_sunxi_plat *dwc3)
{
	dwc3_cfg_t *cfg = &dwc3->cfg;

	if (cfg->gma340_sel_gpio_valid) {
		gpio_free(cfg->gma340_sel_gpio_set.gpio);
		cfg->gma340_sel_gpio_valid = 0;
	}

	if (cfg->gma340_oe_gpio_valid) {
		gpio_free(cfg->gma340_oe_gpio_set.gpio);
		cfg->gma340_oe_gpio_valid = 0;
	}
}

static int dwc3_sunxi_init(struct dwc3_sunxi_plat *dwc3)
{
	struct device *dev = dwc3->dev;
	struct device_node *np = dev->of_node;
	struct dwc3 __maybe_unused *dwc = dwc3->dwc;
	const char __maybe_unused *usb_psy_name;
	int ret = 0;

	/* Init default config */
	dwc3->old_side = DWC3_UNKNOWN_SIDE;
	dwc3->old_mode = DWC3_UNKNOWN_MODE;

	/* init lock, flag, etc. */
	mutex_init(&dwc3->lock);
	mutex_init(&dwc3->det_lock);
	atomic_set(&dwc3->det_flag, 1);
	atomic_set(&dwc3->pm_flag, 0);
	INIT_WORK(&dwc3->resume_work, dwc3_sunxi_resume_work);

	if (dwc3->cfg.detect_type == DWC3_DETECT_TYPE_PMU) {
		if (of_find_property(np, "det_mode_supply", NULL))
			dwc3->cfg.psy = devm_power_supply_get_by_phandle(dev, "det_mode_supply");
		if (IS_ERR_OR_NULL(dwc3->cfg.psy)) {
			sunxi_err(dev, "get det mode supply failed\n");
			/* maybe power supply module is later, free pin and probe again */
			ret = -EPROBE_DEFER;
		}
	}

	/**
	 * NOTE:
	 * The power supply description referenced by DT property 'usb-psy-name' is likely to have
	 * been registered late, we'd better try to get the power supply if DT property exists.
	 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
	if (!device_property_read_string(dwc->dev, "usb-psy-name", &usb_psy_name) &&
	    !dwc->usb_psy) {
		dwc->usb_psy = power_supply_get_by_name(usb_psy_name);
		if (IS_ERR_OR_NULL(dwc->usb_psy)) {
			dev_err(dev, "couldn't get usb power supply\n");
			ret = -EPROBE_DEFER;
		}
	}
#endif

	/* Here are some compatible issues for Controller. */
	dwc3_hw_init(dwc3);

	return ret;
}

static int dwc3_sunxi_plat_probe(struct platform_device *pdev)
{
	struct dwc3_sunxi_plat	*dwc3;
	struct device		*dev = &pdev->dev;
	struct device_node	*np = dev->of_node, *child;
	struct platform_device	*child_pdev;
	struct task_struct	*ts_mode = NULL;
	struct task_struct	*ts_side = NULL;
	int			ret;

	dwc3 = devm_kzalloc(dev, sizeof(*dwc3), GFP_KERNEL);
	if (!dwc3)
		return -ENOMEM;


	child = of_get_compatible_child(np, "snps,dwc3");
	if (!child) {
		sunxi_err(dev, "failed to find dwc3 core child\n");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, dwc3);
	dwc3->dev = dev;

	ret = dwc3_script_parse(np, &dwc3->cfg);
	if (ret != 0) {
		sunxi_err(dev, "get dwc3 cfg failed, %d\n", -ret);
		return ret;
	}

	dwc3_alloc_pin(dwc3);

	dwc3->vbus = devm_regulator_get_optional(dev, "drvvbus");
	if (IS_ERR(dwc3->vbus)) {
		if (PTR_ERR(dwc3->vbus) == -EPROBE_DEFER)
			return PTR_ERR(dwc3->vbus);
		dwc3->vbus = NULL;
		sunxi_err(dev, "couldn't get drvvbus supply\n");
	}
	dwc3->vbus_shared_quirk = device_property_read_bool(dev, "aw,vbus-shared-quirk");
	dwc3->hcgen2_phygen1_quirk = device_property_read_bool(dev, "aw,hcgen2-phygen1-quirk");
	dwc3->u2drd_u3host_quirk = device_property_read_bool(dev, "aw,u2drd-u3host-quirk");
	dwc3->inv_sync_hdr_quirk = device_property_read_bool(dev, "aw,inv-sync-hdr-quirk");
	/*
	 * Some controllers need to toggle the usb3-otg reset before trying to
	 * initialize the PHY, otherwise the PHY times out.
	 */

	dwc3->resets = of_reset_control_array_get(np, false, true,
						    true);
	if (IS_ERR(dwc3->resets)) {
		ret = PTR_ERR(dwc3->resets);
		sunxi_err(dev, "failed to get device resets, err=%d\n", ret);
		return ret;
	}

	ret = reset_control_deassert(dwc3->resets);
	if (ret)
		goto err_resetc_put;

	ret = devm_clk_bulk_get_all(dwc3->dev, &dwc3->clks);
	if (ret < 0)
		goto err_resetc_assert;

	dwc3->num_clocks = ret;
	ret = clk_bulk_prepare_enable(dwc3->num_clocks, dwc3->clks);
	if (ret)
		goto err_resetc_assert;

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret)
		goto err_clk_put;

	child_pdev = of_find_device_by_node(child);
	if (!child_pdev) {
		sunxi_err(dev, "failed to find dwc3 core device\n");
		ret = -ENODEV;
		goto err_clk_put;
	}

	dwc3->dwc = platform_get_drvdata(child_pdev);
	if (!dwc3->dwc) {
		sunxi_err(dev, "failed to get drvdata dwc3\n");
		ret = -EPROBE_DEFER;
		goto err_clk_put;
	}

	ret = dwc3_sunxi_init(dwc3);
	if (ret)
		goto err_clk_put;

	if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_NOTIFY) {
		INIT_WORK(&dwc3->det_work, dwc3_typec_det_work);
		INIT_WORK(&dwc3->pm_work, dwc3_typec_pm_work);
		dwc3->det_nb.notifier_call = dwc3_typec_det_notifier;
		dwc3->pm_nb.notifier_call = dwc3_typec_pm_notifier;

#if IS_ENABLED(CONFIG_AW_AXP2202_POWER) || IS_ENABLED(CONFIG_AW_AXP515_POWER)
		/* register dwc3 power notifier */
		atomic_notifier_chain_register(&usb_power_notifier_list, &dwc3->det_nb);
#endif
#if IS_ENABLED(CONFIG_USB_SUNXI_HCI)
		/* register dwc3 pm notifier */
		atomic_notifier_chain_register(&usb_pm_notifier_list, &dwc3->pm_nb);
#endif
		/* NOTE: It's necessary to detect scan manually when probe. */
		dwc3_typec_det_event(dwc3);
		/* restore det_flag for auto detect */
		atomic_set(&dwc3->det_flag, 1);
	} else if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_THREAD) {
		dwc3_typec_run_flag = 1;
		ts_mode = kthread_create(dwc3_typec_scan_mode_thread, dwc3, "dwc3_typec_mode");
		ts_side = kthread_create(dwc3_typec_scan_side_thread, dwc3, "dwc3_typec_side");
		if (IS_ERR_OR_NULL(ts_mode) || IS_ERR_OR_NULL(ts_side)) {
			sunxi_err(dev, "ERR: create dwc3_typec_scan kthread failed\n");
			goto err_clk_put;
		}
		wake_up_process(ts_mode);
		wake_up_process(ts_side);
	} else if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_EXTCON) {
		ret = dwc3_sunxi_extcon_register(dwc3);
		if (ret) {
			dev_err(dev, "failed to register dwc3 extcon\n");
			goto err_clk_put;
		}
	} else {
		dwc3_set_vbus(dwc3, 1);
		sunxi_debug(dev, "don't support full-featured Type-C\n");
	}
	sunxi_info(dev, "%s\n", DRIVER_INFOMATION);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	dwc3_sunxi_debug_init(dwc3);
	dwc3_debug_init(dwc3->dwc);
	xhci_debug_init(dwc3->dwc);
	return 0;

err_clk_put:
	clk_bulk_disable_unprepare(dwc3->num_clocks, dwc3->clks);

err_resetc_assert:
	reset_control_assert(dwc3->resets);

err_resetc_put:
	reset_control_put(dwc3->resets);

	dwc3_free_pin(dwc3);
	return ret;
}

static void __dwc3_sunxi_teardown(struct dwc3_sunxi_plat *dwc3)
{
	xhci_debug_exit(dwc3->dwc);
	dwc3_debug_exit(dwc3->dwc);
	dwc3_sunxi_debug_exit(dwc3);
	of_platform_depopulate(dwc3->dev);

	clk_bulk_disable_unprepare(dwc3->num_clocks, dwc3->clks);
	dwc3->num_clocks = 0;

	reset_control_assert(dwc3->resets);

	reset_control_put(dwc3->resets);

	atomic_set(&dwc3->det_flag, 0);
	if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_NOTIFY) {
#if IS_ENABLED(CONFIG_USB_SUNXI_HCI)
		/* unregister dwc3 pm notifier */
		atomic_notifier_chain_unregister(&usb_pm_notifier_list, &dwc3->pm_nb);
#endif
#if IS_ENABLED(CONFIG_AW_AXP2202_POWER) || IS_ENABLED(CONFIG_AW_AXP515_POWER)
		/* unregister dwc3 power notifier */
		atomic_notifier_chain_unregister(&usb_power_notifier_list, &dwc3->det_nb);
#endif
	} else if (dwc3->cfg.detect_mode == DWC3_DETECT_MODE_THREAD) {
		dwc3_typec_run_flag = 0;
		while (!dwc3_mode_stop_flag || !dwc3_side_stop_flag) {
			sunxi_debug(dwc3->dev, "wait for dwc3_typec_mode and dwc3_typec_side stop\n");
			dwc3_msleep(100);
		}
	}
	dwc3_free_pin(dwc3);
	dwc3_set_vbus(dwc3, 0);
	cancel_work_sync(&dwc3->resume_work);

	pm_runtime_disable(dwc3->dev);
	pm_runtime_put_noidle(dwc3->dev);
	pm_runtime_set_suspended(dwc3->dev);
}

static int dwc3_sunxi_plat_remove(struct platform_device *pdev)
{
	struct dwc3_sunxi_plat	*dwc3 = platform_get_drvdata(pdev);

	__dwc3_sunxi_teardown(dwc3);

	return 0;
}

static void dwc3_sunxi_plat_shutdown(struct platform_device *pdev)
{
	struct dwc3_sunxi_plat	*dwc3 = platform_get_drvdata(pdev);

	__dwc3_sunxi_teardown(dwc3);
}

static int __maybe_unused dwc3_sunxi_plat_runtime_suspend(struct device *dev)
{
	struct dwc3_sunxi_plat	*dwc3 = dev_get_drvdata(dev);

	clk_bulk_disable(dwc3->num_clocks, dwc3->clks);

	return 0;
}

static int __maybe_unused dwc3_sunxi_plat_runtime_resume(struct device *dev)
{
	struct dwc3_sunxi_plat	*dwc3 = dev_get_drvdata(dev);

	return clk_bulk_enable(dwc3->num_clocks, dwc3->clks);
}

static int __maybe_unused dwc3_sunxi_plat_suspend(struct device *dev)
{
	struct dwc3_sunxi_plat *dwc3 = dev_get_drvdata(dev);

	atomic_set(&dwc3->pm_flag, 1);
	atomic_set(&dwc3->det_flag, 0);
	atomic_set(&dwc3_thread_suspend_flag, 1);
	dwc3_set_vbus(dwc3, 0);

	if (dwc3->need_reset)
		reset_control_assert(dwc3->resets);

#if IS_ENABLED(CONFIG_AW_INNO_COMBOPHY)
	pm_runtime_put_sync(dwc3->dwc->usb3_generic_phy->dev.parent);
#endif
	pm_runtime_put_sync(dwc3->dwc->dev);

	cancel_work_sync(&dwc3->resume_work);

	return 0;
}

static int __maybe_unused dwc3_sunxi_plat_resume(struct device *dev)
{
	struct dwc3_sunxi_plat *dwc3 = dev_get_drvdata(dev);

	pm_runtime_get_sync(dwc3->dwc->dev);
#if IS_ENABLED(CONFIG_AW_INNO_COMBOPHY)
	pm_runtime_get_sync(dwc3->dwc->usb3_generic_phy->dev.parent);
#endif

	if (dwc3->need_reset)
		reset_control_deassert(dwc3->resets);

	dwc3_set_vbus(dwc3, 1);
	atomic_set(&dwc3_thread_suspend_flag, 0);
	atomic_set(&dwc3->det_flag, 1);
	schedule_work(&dwc3->resume_work);

	return 0;
}

static const struct dev_pm_ops dwc3_sunxi_plat_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_sunxi_plat_suspend, dwc3_sunxi_plat_resume)
	SET_RUNTIME_PM_OPS(dwc3_sunxi_plat_runtime_suspend,
			dwc3_sunxi_plat_runtime_resume, NULL)
};

static const struct of_device_id of_dwc3_sunxi_match[] = {
	{ .compatible = "allwinner,sunxi-plat-dwc3" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_dwc3_sunxi_match);

static struct platform_driver dwc3_sunxi_plat_driver = {
	.probe		= dwc3_sunxi_plat_probe,
	.remove		= dwc3_sunxi_plat_remove,
	.shutdown	= dwc3_sunxi_plat_shutdown,
	.driver		= {
		.name	= "sunxi-plat-dwc3",
		.of_match_table = of_dwc3_sunxi_match,
		.pm	= &dwc3_sunxi_plat_dev_pm_ops,
	},
};

module_platform_driver(dwc3_sunxi_plat_driver);

MODULE_ALIAS("platform:sunxi-plat-dwc3");
MODULE_DESCRIPTION("DesignWare USB3 Allwinner Glue Layer");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.23");

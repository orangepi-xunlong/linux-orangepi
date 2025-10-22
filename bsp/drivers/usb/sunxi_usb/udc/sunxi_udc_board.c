/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb board config.
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
#include <linux/err.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/io.h>
#include  "sunxi_udc_config.h"
#include  "sunxi_udc_board.h"

#define res_size(_r) (((_r)->end - (_r)->start) + 1)

static int __maybe_unused usbc_rescal_clock_set(sunxi_udc_io_t *sunxi_udc_io, bool enable)
{
	int ret = 0;

	if (sunxi_udc_io->rext_cal_bypass) {
		DMSG_INFO_UDC("external resistance calibration bypass, skip set clk_res\n");
		return 0;
	}

	if (enable) {
		if (sunxi_udc_io->clk_res) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_res);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_res err, return %d\n", ret);
				return ret;
			}
		}
	} else {
		if (sunxi_udc_io->clk_res)
			clk_disable_unprepare(sunxi_udc_io->clk_res);
	}
	return 0;
}

u32  open_usb_clock(sunxi_udc_io_t *sunxi_udc_io)
{
	int ret;

	DMSG_INFO_UDC("open_usb_clock\n");

	/* To fix hardware design issue. */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12) || IS_ENABLED(CONFIG_ARCH_SUN50IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW6) || IS_ENABLED(CONFIG_ARCH_SUN8IW15) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW18)
	usb_otg_phy_txtune(sunxi_udc_io->usb_vbase);
#endif

	if (!sunxi_udc_io->clk_is_open) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		usbc_rescal_clock_set(sunxi_udc_io, true);
		usbc_phyx_res_cal(0, true, sunxi_udc_io->rext_cal_bypass);
#endif
		if (sunxi_udc_io->ahb_otg) {
			ret = clk_prepare_enable(sunxi_udc_io->ahb_otg);
			if (ret) {
				DMSG_ERR("[udc]: enable ahb_otg err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_udc_io->clk_msi_lite) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_msi_lite);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_msi_lite err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_udc_io->clk_usb_sys_ahb) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_usb_sys_ahb);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_usb_sys_ahb err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_udc_io->clk_hosc) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_hosc);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_hosc err, return %d\n", ret);
				return ret;
			}
		}

		udelay(10);

		if (sunxi_udc_io->mod_usbphy) {
			ret = clk_prepare_enable(sunxi_udc_io->mod_usbphy);
			if (ret) {
				DMSG_ERR("[udc]: enable mod_usbphy err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_udc_io->reset_otg) {
			ret = reset_control_deassert(sunxi_udc_io->reset_otg);
			if (ret) {
				DMSG_ERR("[udc]: reset otg err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_udc_io->clk_bus_otg) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_bus_otg);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_bus_otg err, return %d\n", ret);
				return ret;
			}
		}

		udelay(10);

		if (sunxi_udc_io->clk_phy) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_phy);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_phy err, return %d\n", ret);
				return ret;
			}
		}

		udelay(10);

		if (sunxi_udc_io->clk_usb) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_usb);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_usb err, return %d\n", ret);
				return ret;
			}
		}

		udelay(10);

		if (sunxi_udc_io->clk_mbus) {
			ret = clk_prepare_enable(sunxi_udc_io->clk_mbus);
			if (ret) {
				DMSG_ERR("[udc]: enable clk_mbus err, return %d\n", ret);
				return ret;
			}
		}

		udelay(10);

		if (sunxi_udc_io->reset_usb) {
			ret = reset_control_deassert(sunxi_udc_io->reset_usb);
			if (ret) {
				DMSG_ERR("[udc]: reset usb err, return %d\n", ret);
				return ret;
			}
		}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN50IW11)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW12) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	USBC_PHY_Clear_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_LOOPBACKENB);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	/* There is no USBC_PHY_CTL_VBUSVLDEXT bit in this SOC, configure it here.  */
	USBC_PHY_Clear_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW12) || IS_ENABLED(CONFIG_ARCH_SUN50IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW6) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN50IW8) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW18) || IS_ENABLED(CONFIG_ARCH_SUN8IW16) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW19) || IS_ENABLED(CONFIG_ARCH_SUN50IW11) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW12) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	USBC_PHY_Set_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_VBUSVLDEXT);
	USBC_PHY_Clear_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#else
	if (sunxi_udc_io->usb2_generic_phy) {
		ret = phy_init(sunxi_udc_io->usb2_generic_phy);
		if (ret < 0) {
			DMSG_ERR("[udc]: init phy err, return %d\n", ret);
			return ret;
		}
	} else {
		UsbPhyInit(0);
	}
#endif
		if (sunxi_udc_io->reset_phy) {
			ret = reset_control_deassert(sunxi_udc_io->reset_phy);
			if (ret) {
				DMSG_ERR("[udc]: reset phy err, return %d\n", ret);
				return ret;
			}
		}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		udelay(10);
		if (sunxi_udc_io->usb_ccmu_config) {
			int val;

			val = readl(sunxi_udc_io->usb_ccmu_config + 0x0A8C);
			val |= (SUNXI_CCMU_USBEHCI1_GATING_OFFSET
				| SUNXI_CCMU_USBEHCI1_RST_OFFSET);
			writel(val, sunxi_udc_io->usb_ccmu_config + 0x0A8C);

			udelay(10);
			val = readl(sunxi_udc_io->usb_ccmu_config + 0x0A74);
			val |= (SUNXI_CCMU_SCLK_GATING_USBPHY1_OFFSET
				| SUNXI_CCMU_USBPHY1_RST_OFFSET
				| SUNXI_CCMU_SCLK_GATING_OHCI1_OFFSET);
			writel(val, sunxi_udc_io->usb_ccmu_config + 0x0A74);

			/* phy reg, offset:0x10 bit3 set 0, enable siddq */
			val = USBC_Readl(sunxi_udc_io->usb_common_phy_config
					 + SUNXI_HCI_PHY_CTRL);
			val &= ~(0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
			USBC_Writel(val, sunxi_udc_io->usb_common_phy_config
				    + SUNXI_HCI_PHY_CTRL);
		}
#endif

		sunxi_udc_io->clk_is_open = 1;
	}

#if IS_ENABLED(CONFIG_ARCH_SUN50I) || IS_ENABLED(CONFIG_ARCH_SUN8IW10) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW11) || IS_ENABLED(CONFIG_ARCH_SUN8IW12) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN8IW7) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW17) || IS_ENABLED(CONFIG_ARCH_SUN8IW18) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW16) || IS_ENABLED(CONFIG_ARCH_SUN8IW19) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW8) || IS_ENABLED(CONFIG_ARCH_SUN8IW20)\
	|| IS_ENABLED(CONFIG_ARCH_SUN20IW1) || IS_ENABLED(CONFIG_ARCH_SUN50IW12) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN65IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	/* otg and hci0 Controller Shared phy in SUN50I and SUN8IW10 */
	USBC_SelectPhyToDevice(sunxi_udc_io->usb_vbase);

	ret = phy_set_mode(sunxi_udc_io->usb2_generic_phy, PHY_MODE_USB_DEVICE);
	if (ret < 0) {
		DMSG_ERR("[udc]: set phy mdode err, return %d\n", ret);
		return ret;
	}

#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	usbc_new_phy_init(sunxi_udc_io->usb_vbase);
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	/* Adapt USB phy parameters */
	usbc_phy_reassign(sunxi_udc_io->usb_vbase, NULL, 0x22f);
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	/* Adapt USB phy parameters */
	usbc_phy_reassign(sunxi_udc_io->usb_vbase, NULL, 0x228);
	/* Adapt USB phy bandwidth tuning parameters */
	usbc_phy_bandwidth_tuning(sunxi_udc_io->usb_vbase, NULL, 0x3f);
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	/* Adapt USB phy parameters */
	usbc_phy_reassign(sunxi_udc_io->usb_vbase, NULL, 0x208);
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	/* For 24Mhz crystal oscillator, you need to modify the phy pll
	 * configuration, please refer to the spec.
	 */
	if (sunxi_udc_io->rate_clk == 24000000)
		usbc_new_phy_pll_set(sunxi_udc_io->usb_vbase, 20);
	/* Adapt USB phy parameters */
	usbc_phy_reassign(sunxi_udc_io->usb_vbase, NULL, 0x2e8);
#endif
	/* Modify the phy range parameter of USB Device
	 * by reading the parameters-phy_range from DTS.
	 */
	usbc_phy_reassign(sunxi_udc_io->usb_vbase,
			sunxi_udc_io->usb_bsp_hdle,
			sunxi_udc_io->phy_range);

	return 0;
}

u32 close_usb_clock(sunxi_udc_io_t *sunxi_udc_io)
{
	DMSG_INFO_UDC("close_usb_clock\n");

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	USBC_PHY_Set_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#endif
	phy_exit(sunxi_udc_io->usb2_generic_phy);

	if (sunxi_udc_io->clk_is_open) {
		sunxi_udc_io->clk_is_open = 0;

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_udc_io->usb_ccmu_config) {
			int val;

			val = readl(sunxi_udc_io->usb_ccmu_config + 0x0A8C);
			val &= ~(SUNXI_CCMU_USBEHCI1_GATING_OFFSET
				| SUNXI_CCMU_USBEHCI1_RST_OFFSET);
			writel(val, sunxi_udc_io->usb_ccmu_config + 0x0A8C);

			val = readl(sunxi_udc_io->usb_ccmu_config + 0x0A74);
			val &= ~(SUNXI_CCMU_SCLK_GATING_USBPHY1_OFFSET
				| SUNXI_CCMU_USBPHY1_RST_OFFSET);
			writel(val, sunxi_udc_io->usb_ccmu_config + 0x0A74);

			/* phy reg, offset:0x10 bit3 set 0, enable siddq */
			val = USBC_Readl(sunxi_udc_io->usb_common_phy_config
					 + SUNXI_HCI_PHY_CTRL);
			val |= (0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
			USBC_Writel(val, sunxi_udc_io->usb_common_phy_config
				    + SUNXI_HCI_PHY_CTRL);
			udelay(10);
		}
#endif
		if (sunxi_udc_io->mod_usbphy)
			clk_disable_unprepare(sunxi_udc_io->mod_usbphy);

		if (sunxi_udc_io->ahb_otg)
			clk_disable_unprepare(sunxi_udc_io->ahb_otg);

		if (sunxi_udc_io->clk_mbus)
			clk_disable_unprepare(sunxi_udc_io->clk_mbus);

		if (sunxi_udc_io->clk_usb)
			clk_disable_unprepare(sunxi_udc_io->clk_usb);

		if (sunxi_udc_io->clk_phy)
			clk_disable_unprepare(sunxi_udc_io->clk_phy);

		if (sunxi_udc_io->clk_bus_otg)
			clk_disable_unprepare(sunxi_udc_io->clk_bus_otg);

		if (sunxi_udc_io->reset_otg)
			reset_control_assert(sunxi_udc_io->reset_otg);

		if (sunxi_udc_io->clk_hosc)
			clk_disable_unprepare(sunxi_udc_io->clk_hosc);

		if (sunxi_udc_io->reset_phy)
			reset_control_assert(sunxi_udc_io->reset_phy);

		if (sunxi_udc_io->reset_usb)
			reset_control_assert(sunxi_udc_io->reset_usb);

		if (sunxi_udc_io->clk_usb_sys_ahb)
			clk_disable_unprepare(sunxi_udc_io->clk_usb_sys_ahb);

		if (sunxi_udc_io->clk_msi_lite)
			clk_disable_unprepare(sunxi_udc_io->clk_msi_lite);

		udelay(10);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		usbc_phyx_res_cal(0, false, sunxi_udc_io->rext_cal_bypass);
		usbc_rescal_clock_set(sunxi_udc_io, false);
#endif
	}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW12) || IS_ENABLED(CONFIG_ARCH_SUN50IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW6) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN50IW8) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW18) || IS_ENABLED(CONFIG_ARCH_SUN8IW16) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW19) || IS_ENABLED(CONFIG_ARCH_SUN50IW11) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	USBC_PHY_Set_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#else
	if (!sunxi_udc_io->usb2_generic_phy)
		UsbPhyInit(0);
#endif

	return 0;
}

__s32 sunxi_udc_bsp_init(sunxi_udc_io_t *sunxi_udc_io)
{
	spinlock_t lock;
	unsigned long flags = 0;

	/* open usb lock */
	open_usb_clock(sunxi_udc_io);

#ifdef SUNXI_USB_FPGA
	clear_usb_reg(sunxi_udc_io->usb_vbase);
#endif

	USBC_EnhanceSignal(sunxi_udc_io->usb_bsp_hdle);

	USBC_EnableDpDmPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_EnableIdPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_ForceId(sunxi_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DEVICE);
	USBC_ForceVbusValid(sunxi_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

	USBC_SelectBus(sunxi_udc_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

	USBC_PHY_Clear_Ctl(sunxi_udc_io->usb_vbase, 1);

	/* config usb fifo */
	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	USBC_ConfigFIFO_Base(sunxi_udc_io->usb_bsp_hdle, USBC_FIFO_MODE_8K);
	spin_unlock_irqrestore(&lock, flags);

	return 0;
}

__s32 sunxi_udc_bsp_exit(sunxi_udc_io_t *sunxi_udc_io)
{
	USBC_DisableDpDmPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_DisableIdPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_ForceId(sunxi_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DISABLE);
	USBC_ForceVbusValid(sunxi_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_DISABLE);

	close_usb_clock(sunxi_udc_io);
	return 0;
}

__s32 sunxi_udc_io_init(__u32 usbc_no, sunxi_udc_io_t *sunxi_udc_io)
{
	sunxi_udc_io->usbc.usbc_info.num = usbc_no;
	sunxi_udc_io->usbc.usbc_info.base = sunxi_udc_io->usb_vbase;
	sunxi_udc_io->usbc.sram_base = sunxi_udc_io->sram_vbase;

	USBC_init(&sunxi_udc_io->usbc);
	sunxi_udc_io->usb_bsp_hdle = USBC_open_otg(usbc_no);
	if (sunxi_udc_io->usb_bsp_hdle == NULL) {
		DMSG_ERR("ERR: sunxi_udc_init: USBC_open_otg failed\n");
		return -1;
	}

	return 0;
}

__s32 sunxi_udc_io_exit(sunxi_udc_io_t *sunxi_udc_io)
{
	USBC_close_otg(sunxi_udc_io->usb_bsp_hdle);
	sunxi_udc_io->usb_bsp_hdle = NULL;
	USBC_exit(&sunxi_udc_io->usbc);
	sunxi_udc_io->usb_vbase  = NULL;
	sunxi_udc_io->sram_vbase = NULL;

	return 0;
}

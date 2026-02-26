/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-7-18, add clock and power switch
 *
 * sunxi HCI Driver
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
#include <linux/reset.h>
#include <linux/gpio.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE == KERNEL_VERSION(5, 4, 220))
#include <linux/kthread.h>
#endif

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <linux/regulator/consumer.h>
#include  <linux/of.h>
#include  <linux/of_address.h>
#include  <linux/of_device.h>
#include <sunxi-sid.h>

#include  "sunxi-hci.h"

#define DRIVER_DESC "SUNXI HCI driver"
#define  SUNXI_HCI_NAME	"sunxi-hci"
static const char hci_name[] = SUNXI_HCI_NAME;

static u64 sunxi_hci_dmamask = ~0ULL; // DMA_BIT_MASK(64); /* Avoid Clang waring: shift count >= width of type */
static DEFINE_MUTEX(usb_passby_lock);
static DEFINE_MUTEX(usb_vbus_lock);
static DEFINE_MUTEX(usb_clock_lock);
static DEFINE_MUTEX(usb_standby_lock);

#ifndef CONFIG_OF
static char *usbc_name[4] = {"usbc0", "usbc1", "usbc2", "usbc3"};
#endif

static struct sunxi_hci_hcd sunxi_ohci0;
static struct sunxi_hci_hcd sunxi_ohci1;
static struct sunxi_hci_hcd sunxi_ohci2;
static struct sunxi_hci_hcd sunxi_ohci3;
static struct sunxi_hci_hcd sunxi_ehci0;
static struct sunxi_hci_hcd sunxi_ehci1;
static struct sunxi_hci_hcd sunxi_ehci2;
static struct sunxi_hci_hcd sunxi_ehci3;
static struct sunxi_hci_hcd sunxi_xhci;

#define  USBPHYC_REG_o_PHYCTL		    0x0404

static atomic_t usb1_enable_passly_cnt = ATOMIC_INIT(0);
static atomic_t usb2_enable_passly_cnt = ATOMIC_INIT(0);
static atomic_t usb3_enable_passly_cnt = ATOMIC_INIT(0);
static atomic_t usb4_enable_passly_cnt = ATOMIC_INIT(0);

static atomic_t usb0_enable_siddq_cnt = ATOMIC_INIT(0);
static atomic_t usb1_enable_siddq_cnt = ATOMIC_INIT(0);
static atomic_t usb2_enable_siddq_cnt = ATOMIC_INIT(0);
static atomic_t usb3_enable_siddq_cnt = ATOMIC_INIT(0);

static atomic_t usb_standby_cnt[4];

/*
 * In a specified platform, if only ohci is enabled, you need to add
 * specified actions during suspend to reduce power consumption, so you
 * need to add a check to see if ehci is enabled. If it is enabled, you
 * do not need to add specified actions.
 */
atomic_t ehci_enable_flag[4];
static bool init_flag;

ATOMIC_NOTIFIER_HEAD(usb_pm_notifier_list);
EXPORT_SYMBOL(usb_pm_notifier_list);

static s32 request_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{
	if (sunxi_hci->hsic_flag) {
		if (sunxi_hci->hsic_regulator_io != NULL) {
			sunxi_hci->hsic_regulator_io_hdle =
					regulator_get(NULL, sunxi_hci->hsic_regulator_io);
			if (IS_ERR(sunxi_hci->hsic_regulator_io_hdle)) {
				DMSG_ERR("ERR: some error happen, %s, hsic_regulator_io_hdle fail to get regulator!", sunxi_hci->hci_name);
				sunxi_hci->hsic_regulator_io_hdle = NULL;
				return 0;
			}
		}
	}

	return 0;
}

static s32 release_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{
	if (sunxi_hci->hsic_flag) {
		if (sunxi_hci->hsic_regulator_io != NULL)
			regulator_put(sunxi_hci->hsic_regulator_io_hdle);
	}

	return 0;
}

static void __iomem *usb_phy_csr_add(struct sunxi_hci_hcd *sunxi_hci)
{
	return (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CTRL);
}

static void __iomem *usb_phy_csr_read(struct sunxi_hci_hcd *sunxi_hci)
{
	switch (sunxi_hci->usbc_no) {
	case 0:
		return sunxi_hci->otg_vbase + SUNXI_OTG_PHY_STATUS;

	case 1:
		return sunxi_hci->usb_vbase + SUNXI_HCI_UTMI_PHY_STATUS;

	case 2:
		return sunxi_hci->usb_vbase + SUNXI_HCI_UTMI_PHY_STATUS;

	case 3:
		return sunxi_hci->usb_vbase + SUNXI_HCI_UTMI_PHY_STATUS;

	default:
		DMSG_ERR("usb_phy_csr_read is failed in %d index\n",
			sunxi_hci->usbc_no);
		break;
	}

	return NULL;
}

static void __iomem *usb_phy_csr_write(struct sunxi_hci_hcd *sunxi_hci)
{
	switch (sunxi_hci->usbc_no) {
	case 0:
		return sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CTRL;

	case 1:
		return sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL;

	case 2:
		return sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL;

	case 3:
		return sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL;

	default:
		DMSG_ERR("usb_phy_csr_write is failed in %d index\n",
			sunxi_hci->usbc_no);
		break;
	}

	return NULL;
}

int usb_phyx_tp_write(struct sunxi_hci_hcd *sunxi_hci,
		int addr, int data, int len)
{
	int temp = 0;
	int j = 0;
	int reg_value = 0;
	int reg_temp = 0;
	int dtmp = 0;

	if (sunxi_hci->otg_vbase == NULL) {
		DMSG_ERR("%s,otg_vbase is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_add(sunxi_hci) == NULL) {
		DMSG_ERR("%s,phy_csr_add is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_write(sunxi_hci) == NULL) {

		DMSG_ERR("%s,phy_csr_write is null\n", __func__);
		return -1;
	}

	reg_value = USBC_Readl(sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG);
	reg_temp = reg_value;
	reg_value |= 0x01;
	USBC_Writel(reg_value, (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG));

	dtmp = data;
	for (j = 0; j < len; j++) {
		USBC_Writeb(addr + j, usb_phy_csr_add(sunxi_hci) + 1);

		temp = USBC_Readb(usb_phy_csr_write(sunxi_hci));
		temp &= ~(0x1 << 0);
		USBC_Writeb(temp, usb_phy_csr_write(sunxi_hci));

		temp = USBC_Readb(usb_phy_csr_add(sunxi_hci));
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		USBC_Writeb(temp, usb_phy_csr_add(sunxi_hci));

		temp = USBC_Readb(usb_phy_csr_write(sunxi_hci));
		temp |= (0x1 << 0);
		USBC_Writeb(temp, usb_phy_csr_write(sunxi_hci));

		temp = USBC_Readb(usb_phy_csr_write(sunxi_hci));
		temp &= ~(0x1 << 0);
		USBC_Writeb(temp, usb_phy_csr_write(sunxi_hci));

		dtmp >>= 1;
	}

	USBC_Writel(reg_temp, (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG));

	return 0;
}
EXPORT_SYMBOL(usb_phyx_tp_write);

int usb_phyx_write(struct sunxi_hci_hcd *sunxi_hci, int data)
{
	int reg_value = 0;
	int temp = 0;
	int dtmp = 0;
	int ptmp = 0;

	temp = data;
	dtmp = data;
	ptmp = data;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_TUNE);

	/* TXVREFTUNE + TXRISETUNE + TXPREEMPAMPTUNE + TXRESTUNE */
	reg_value &= ~((0xf << 8) | (0x3 << 4) | (0xf << 0));
	temp &= ~((0xf << 4) | (0x3 << 8));
	reg_value |= temp << 8;
	dtmp &= ~((0xf << 6) | (0xf << 0));
	reg_value |= dtmp;
	data &= ~((0x3 << 4) | (0xf << 0));
	reg_value |= data >> 6;

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_TUNE));

	return 0;
}
EXPORT_SYMBOL(usb_phyx_write);

int usb_phyx_read(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;
	int temp = 0;
	int ptmp = 0;
	int ret = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_TUNE);
	reg_value &= 0xf3f;
	ptmp = reg_value;

	temp = reg_value >> 8;
	ptmp &= ~((0xf << 8) | (0xf << 4));
	ptmp <<= 6;
	reg_value &= ~((0xf << 8) | (0xf << 0));

	ret = reg_value | ptmp | temp;

	DMSG_INFO("bit[3:0]VREF = 0x%x; bit[5:4]RISE = 0x%x; bit[7:6]PREEMPAMP = 0x%x; bit[9:8]RES = 0x%x\n",
		temp, reg_value >> 4, (ptmp >> 6) & 0x3,
		((ptmp >> 6)  & 0xc) >> 2);

	return ret;
}
EXPORT_SYMBOL(usb_phyx_read);

int usb_phyx_tp_read(struct sunxi_hci_hcd *sunxi_hci, int addr, int len)
{
	int temp = 0;
	int i = 0;
	int j = 0;
	int ret = 0;
	int reg_value = 0;
	int reg_temp = 0;

	if (sunxi_hci->otg_vbase == NULL) {
		DMSG_ERR("%s,otg_vbase is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_add(sunxi_hci) == NULL) {
		DMSG_ERR("%s,phy_csr_add is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_read(sunxi_hci) == NULL) {
		DMSG_ERR("%s,phy_csr_read is null\n", __func__);
		return -1;
	}

	reg_value = USBC_Readl(sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG);
	reg_temp = reg_value;
	reg_value |= 0x01;
	USBC_Writel(reg_value, (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG));

	for (j = len; j > 0; j--) {
		USBC_Writeb((addr + j - 1), usb_phy_csr_add(sunxi_hci) + 1);

		for (i = 0; i < 0x4; i++)
			;

		temp = USBC_Readb(usb_phy_csr_read(sunxi_hci));
		ret <<= 1;
		ret |= (temp & 0x1);
	}

	USBC_Writel(reg_temp, (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG));

	return ret;
}
EXPORT_SYMBOL(usb_phyx_tp_read);

#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN251IW1)
/* for new phy */
static int usb_new_phyx_tp_write(struct sunxi_hci_hcd *sunxi_hci,
		int addr, int data, int len)
{
	int temp = 0;
	int j = 0;
	int dtmp = 0;
	void __iomem *base;

	if (sunxi_hci->otg_vbase == NULL) {
		DMSG_ERR("%s,otg_vbase is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_add(sunxi_hci) == NULL) {
		DMSG_ERR("%s,phy_csr_add is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_write(sunxi_hci) == NULL) {

		DMSG_ERR("%s,phy_csr_write is null\n", __func__);
		return -1;
	}
	/* device: 0x410(phy_ctl) */
	base = sunxi_hci->usb_vbase;
	dtmp = data;
	for (j = 0; j < len; j++) {
		temp = USBC_Readb(base + SUNXI_HCI_PHY_CTRL);
		temp |= (0x1 << 1);
		USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

		USBC_Writeb(addr + j, base + SUNXI_HCI_PHY_CTRL + 1);

		temp = USBC_Readb(base + SUNXI_HCI_PHY_CTRL);
		temp &= ~(0x1 << 0);
		USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

		temp = USBC_Readb(base + SUNXI_HCI_PHY_CTRL);
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

		temp |= (0x1 << 0);
		USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

		temp &= ~(0x1 << 0);
		USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

		temp = USBC_Readb(base + SUNXI_HCI_PHY_CTRL);
		temp &= ~(0x1 << 1);
		USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

		dtmp >>= 1;
	}

	return 0;
}

static int usb_new_phyx_tp_read(struct sunxi_hci_hcd *sunxi_hci, int addr, int len)
{
	int temp = 0;
	int i = 0;
	int j = 0;
	int ret = 0;
	void __iomem *base;

	if (sunxi_hci->otg_vbase == NULL) {
		DMSG_ERR("%s,otg_vbase is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_add(sunxi_hci) == NULL) {
		DMSG_ERR("%s,phy_csr_add is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_read(sunxi_hci) == NULL) {
		DMSG_ERR("%s,phy_csr_read is null\n", __func__);
		return -1;
	}

	base = sunxi_hci->usb_vbase;

	temp = USBC_Readb(base + SUNXI_HCI_PHY_CTRL);
	temp |= (0x1 << 1);
	USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

	for (j = len; j > 0; j--) {
		USBC_Writeb((addr + j - 1), base + SUNXI_HCI_PHY_CTRL + 1);

		for (i = 0; i < 0x4; i++)
			;

		temp = USBC_Readb(base + SUNXI_HCI_UTMI_PHY_STATUS);
		ret <<= 1;
		ret |= (temp & 0x1);
	}

	temp = USBC_Readb(base + SUNXI_HCI_PHY_CTRL);
	temp &= ~(0x1 << 1);
	USBC_Writeb(temp, base + SUNXI_HCI_PHY_CTRL);

	return ret;
}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN251IW1)
void usb_new_phyx_write(struct sunxi_hci_hcd *sunxi_hci, u32 data)
{
	u32 temp = 0, ptmp = 0, rtmp = 0;

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	u32 ristmp = 0;
#endif
	temp = data & PHY_RANGE_TRAN_MASK;
	temp >>= (2 + 4);
	ptmp = data & PHY_RANGE_PREE_MASK;
	ptmp >>= 4;
	rtmp = data & PHY_RANGE_RESI_MASK;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	ristmp = data & PHY_RANGE_RISE_MASK;
	ristmp >>= 10;

	usb_new_phyx_tp_write(sunxi_hci, 0x68, ristmp, 0x2);
#endif
	/* tranceive data */
	usb_new_phyx_tp_write(sunxi_hci, 0x60, temp, 0x4);
	DMSG_INFO("write to trancevie data: 0x%x\n", temp);

	usb_new_phyx_tp_write(sunxi_hci, 0x64, ptmp, 0x2);
	DMSG_INFO("write to preemphasis data: 0x%x\n", ptmp);

	usb_new_phyx_tp_write(sunxi_hci, 0x43, 0x0, 0x1);

	usb_new_phyx_tp_write(sunxi_hci, 0x41, 0x0, 0x1);

	usb_new_phyx_tp_write(sunxi_hci, 0x40, 0x0, 0x1);

	usb_new_phyx_tp_write(sunxi_hci, 0x44, rtmp, 0x4);
	DMSG_INFO("write to resistance data: 0x%x\n", rtmp);

	usb_new_phyx_tp_write(sunxi_hci, 0x43, 0x1, 0x1);
}

u32 usb_new_phyx_read(struct sunxi_hci_hcd *sunxi_hci)
{
	u32 temp = 0, ptmp = 0, rtmp = 0, ret = 0;
	u32 ristmp = 0, tune_temp = 0;

	temp = usb_new_phyx_tp_read(sunxi_hci, 0x60, 0x4);

	ptmp = usb_new_phyx_tp_read(sunxi_hci, 0x64, 0x2);

	rtmp = usb_new_phyx_tp_read(sunxi_hci, 0x44, 0x4);

	ristmp = usb_new_phyx_tp_read(sunxi_hci, 0x68, 0x2);

	tune_temp =  usb_new_phyx_tp_read(sunxi_hci, 0x60, 0xe);


	DMSG_INFO("tune[13:0]:0x%x, ristune[9:8]:0x%x, trancevie[9:6]:0x%x, preemphasis[5:4]:0x%x, \
resistance[3:0]:0x%x \n", tune_temp, ristmp, temp, ptmp, rtmp);

	temp <<= (2 + 4);
	ptmp <<= 4;
	ret = temp | ptmp | rtmp;

	return ret;
}

static void usb_new_phy_init(struct sunxi_hci_hcd *sunxi_hci)
{
	int value = 0;
	u32 efuse_val  = 0;

	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x03, 0x06,
			usb_new_phyx_tp_read(sunxi_hci, 0x03, 0x06));

	//pll_prediv
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x16, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x16, 0x03));

	//pll_n
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x0b, 0x08,
			usb_new_phyx_tp_read(sunxi_hci, 0x0b, 0x08));

	//pll_sts
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x09, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x09, 0x03));

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	/* For 24Mhz crystal oscillator, you need to modify the phy pll
	 * configuration, please refer to the spec.
	 */
	if (sunxi_hci->rate_clk == 24000000)
		usb_new_phyx_tp_write(sunxi_hci, 0xb, 20, 0x8);
#endif
	sunxi_get_module_param_from_sid(&efuse_val, EFUSE_OFFSET, 4);
	sunxi_debug(NULL, "efuse_val:0x%x\n", efuse_val);

	usb_new_phyx_tp_write(sunxi_hci, 0x03, 0x3f, 0x06);
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x03, 0x3f,
			usb_new_phyx_tp_read(sunxi_hci, 0x03, 0x06));

	usb_new_phyx_tp_write(sunxi_hci, 0x1c, 0x03, 0x03);
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x1c, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x1c, 0x03));

	if (efuse_val & SUNXI_HCI_PHY_EFUSE_ADJUST) {
		/* Already calibrate completely, don't have to distinguish iref mode and vref mode */
		sunxi_debug(NULL, "USB phy already calibrate completely\n");

		switch (sunxi_hci->usbc_no) {
		case 0:
			value = (efuse_val & SUNXI_HCI_PHY_EFUSE_USB0TX) >> 9;
			usb_new_phyx_tp_write(sunxi_hci, 0x60, value, 0x04);
			break;

		default:
			sunxi_err(NULL, "usb %d not exist!\n", sunxi_hci->usbc_no);
			break;
		}

		value = (efuse_val & SUNXI_HCI_PHY_EFUSE_RES) >> 5;
		usb_new_phyx_tp_write(sunxi_hci, 0x44, value, 0x04);

		sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x60, 0x04,
				usb_new_phyx_tp_read(sunxi_hci, 0x60, 0x04));
		sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x44, 0x04,
				usb_new_phyx_tp_read(sunxi_hci, 0x44, 0x04));
	} else {
		sunxi_debug(NULL, "USB phy don't calibrate\n");
	}

	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x03, 0x06,
			usb_new_phyx_tp_read(sunxi_hci, 0x03, 0x06));
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x16, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x16, 0x03));
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x0b, 0x08,
			usb_new_phyx_tp_read(sunxi_hci, 0x0b, 0x08));
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x09, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x09, 0x03));
}

#else

void usb_new_phyx_write(struct sunxi_hci_hcd *sunxi_hci, u32 data)
{
	u32 mtmp = 0, ctmp = 0, temp = 0, ptmp = 0, rtmp = 0;

	mtmp = data & PHY_RANGE_MODE_MASK;
	mtmp >>= (3 + 3 + 2 + 4);
	ctmp = data & PHY_RANGE_COMM_MASK;
	ctmp >>= (3 + 2 + 4);
	temp = data & PHY_RANGE_TRAN_MASK;
	temp >>= (2 + 4);
	ptmp = data & PHY_RANGE_PREE_MASK;
	ptmp >>= 4;
	rtmp = data & PHY_RANGE_RESI_MASK;

	if (mtmp == 1) {
		DMSG_INFO("iref mode\n");
		/* iref mode */
		usb_new_phyx_tp_write(sunxi_hci, 0x60, 0x1, 0x1);

		/* iref common data */
		usb_new_phyx_tp_write(sunxi_hci, 0x30, ctmp, 0x3);
		DMSG_INFO("write to common data: 0x%x\n", ctmp);

		/* iref tranceive data */
		usb_new_phyx_tp_write(sunxi_hci, 0x61, temp, 0x3);
		DMSG_INFO("write to trancevie data: 0x%x\n", temp);
	} else if (mtmp == 0) {
		DMSG_INFO("vref mode\n");
		/* vref mode */
		usb_new_phyx_tp_write(sunxi_hci, 0x60, 0x0, 0x1);

		/* vref common data */
		usb_new_phyx_tp_write(sunxi_hci, 0x36, ctmp, 0x3);
		DMSG_INFO("write to common data: 0x%x\n", ctmp);

		DMSG_INFO("vref mode no need to control trancevie data!\n");
	}

	usb_new_phyx_tp_write(sunxi_hci, 0x64, ptmp, 0x2);
	DMSG_INFO("write to preemphasis data: 0x%x", ptmp);

	usb_new_phyx_tp_write(sunxi_hci, 0x43, 0x0, 0x1);

	usb_new_phyx_tp_write(sunxi_hci, 0x41, 0x0, 0x1);

	usb_new_phyx_tp_write(sunxi_hci, 0x40, 0x0, 0x1);

	usb_new_phyx_tp_write(sunxi_hci, 0x44, rtmp, 0x4);
	DMSG_INFO("write to resistance data: 0x%x\n", rtmp);

	usb_new_phyx_tp_write(sunxi_hci, 0x43, 0x1, 0x1);
}

u32 usb_new_phyx_read(struct sunxi_hci_hcd *sunxi_hci)
{
	u32 mtmp = 0, ctmp = 0, temp = 0, ptmp = 0, rtmp = 0, ret = 0;
	u32  tune_temp = 0;

	mtmp = usb_new_phyx_tp_read(sunxi_hci, 0x60, 0x1);
	if (mtmp == 1) {
		ctmp = usb_new_phyx_tp_read(sunxi_hci, 0x30, 0x3);
		temp = usb_new_phyx_tp_read(sunxi_hci, 0x61, 0x3);
	} else if (mtmp == 0) {
		ctmp = usb_new_phyx_tp_read(sunxi_hci, 0x36, 0x3);
	}
	ptmp = usb_new_phyx_tp_read(sunxi_hci, 0x64, 0x2);
	rtmp = usb_new_phyx_tp_read(sunxi_hci, 0x44, 0x4);

	tune_temp =  usb_new_phyx_tp_read(sunxi_hci, 0x60, 0xe);

	DMSG_INFO("tune[13:0]:0x%x,mode[12]:0x%x, common[11:9]:0x%x, trancevie[8:6]:0x%x, preemphasis[5:4]:0x%x, resistance[3:0]:0x%x\n",
		tune_temp, mtmp, ctmp, temp, ptmp, rtmp);

	mtmp <<= (3 + 3 + 2 + 4);
	ctmp <<= (3 + 2 + 4);
	temp <<= (2 + 4);
	ptmp <<= 4;
	ret = mtmp | ctmp | temp | ptmp | rtmp;

	return ret;
}

static void usb_new_phy_init(struct sunxi_hci_hcd *sunxi_hci)
{
	int value = 0;
	u32 efuse_val  = 0;

	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x03, 0x06,
			usb_new_phyx_tp_read(sunxi_hci, 0x03, 0x06));

	/* pll_prediv */
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x16, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x16, 0x03));

	/* pll_n */
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x0b, 0x08,
			usb_new_phyx_tp_read(sunxi_hci, 0x0b, 0x08));

	/* pll_sts */
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x09, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x09, 0x03));

	sunxi_get_module_param_from_sid(&efuse_val, EFUSE_OFFSET, 4);
	sunxi_debug(NULL, "efuse_val:0x%x\n", efuse_val);

	usb_new_phyx_tp_write(sunxi_hci, 0x1c, 0x0, 0x03);
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x1c, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x1c, 0x03));

	if (efuse_val & SUNXI_HCI_PHY_EFUSE_ADJUST) {
		if (efuse_val & SUNXI_HCI_PHY_EFUSE_MODE) {
			/* iref mode */
			usb_new_phyx_tp_write(sunxi_hci, 0x60, 0x1, 0x01);

			switch (sunxi_hci->usbc_no) {
			case 0:
				value = (efuse_val & SUNXI_HCI_PHY_EFUSE_USB0TX) >> 22;
				usb_new_phyx_tp_write(sunxi_hci, 0x61, value, 0x03);
				break;

			case 1:
				value = (efuse_val & SUNXI_HCI_PHY_EFUSE_USB0TX) >> 25;
				usb_new_phyx_tp_write(sunxi_hci, 0x61, value, 0x03);
				break;

			default:
				sunxi_err(NULL, "usb %d not exist!\n", sunxi_hci->usbc_no);
				break;
			}

			value = (efuse_val & SUNXI_HCI_PHY_EFUSE_RES) >> 18;
			usb_new_phyx_tp_write(sunxi_hci, 0x44, value, 0x04);

			sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x61, 0x03,
				usb_new_phyx_tp_read(sunxi_hci, 0x61, 0x03));
			sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x44, 0x04,
				usb_new_phyx_tp_read(sunxi_hci, 0x44, 0x04));
		} else {
			/* vref mode */
			usb_new_phyx_tp_write(sunxi_hci, 0x60, 0x0, 0x01);

			value = (efuse_val & SUNXI_HCI_PHY_EFUSE_RES) >> 18;
			usb_new_phyx_tp_write(sunxi_hci, 0x44, value, 0x04);

			value = (efuse_val & SUNXI_HCI_PHY_EFUSE_COM) >> 22;
			usb_new_phyx_tp_write(sunxi_hci, 0x36, value, 0x03);


			sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x60, 0x01,
				usb_new_phyx_tp_read(sunxi_hci, 0x60, 0x01));
			sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x44, 0x04,
				usb_new_phyx_tp_read(sunxi_hci, 0x44, 0x04));
			sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x36, 0x03,
				usb_new_phyx_tp_read(sunxi_hci, 0x36, 0x03));
		}
	}

	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x03, 0x06,
			usb_new_phyx_tp_read(sunxi_hci, 0x03, 0x06));
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x16, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x16, 0x03));
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x0b, 0x08,
			usb_new_phyx_tp_read(sunxi_hci, 0x0b, 0x08));
	sunxi_debug(NULL, "addr:%x,len:%x,value:%x\n", 0x09, 0x03,
			usb_new_phyx_tp_read(sunxi_hci, 0x09, 0x03));
}

#endif

#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
void usb_phyx_res_cal(__u32 usbc_no, bool enable, bool bypass)
{
	__u32 reg_val = 0;
	void __iomem *rescal, *res200;
	__u32 port = 0, tmp; /* port companion enable ? */

	if (bypass) {
		pr_info(" External Resistance Calibration already Bypass, not %s it\n",
			enable ? "enable" : "disable");
		return;
	}

	rescal = ioremap(syscfg_reg(RESCAL_CTRL_REG), 4);
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	res200 = ioremap(syscfg_reg(RES0_CTRL_REG), 4);
#else	/* CONFIG_ARCH_SUN55IW3 */
	res200 = ioremap(syscfg_reg(RES200_CTRL_REG), 4);
#endif

	tmp = GENMASK(6, 4) & (~PHY_o_RES200_SEL(usbc_no));
	reg_val = readl(rescal);
	port = reg_val & tmp;
	if (enable) {
		reg_val &= ~CAL_EN;
		reg_val |= PHY_o_RES200_SEL(usbc_no);
	} else {
		if (port == 0)
			reg_val |= CAL_EN;
		reg_val &= ~PHY_o_RES200_SEL(usbc_no);
	}
	writel(reg_val, rescal);

	reg_val = readl(res200);
	if (enable)
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		reg_val &= ~PHY_o_RES200_TRIM(usbc_no);
#else	/* CONFIG_ARCH_SUN55IW3 */
		reg_val &= ~PHY_o_RES200_CTRL(usbc_no);
#endif
	else
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		reg_val |= PHY_o_RES200_TRIM_DEFAULT(usbc_no);
#else	/* CONFIG_ARCH_SUN55IW3 */
		reg_val |= PHY_o_RES200_CTRL_DEFAULT(usbc_no);
#endif
	writel(reg_val, res200);

	iounmap(rescal);
	iounmap(res200);
}
#endif

static void usb_phyx_reassign(struct sunxi_hci_hcd *sunxi_hci, int val)
{
#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1)
	if ((val >= 0) && (val <= 0x1fff))
		usb_new_phyx_write(sunxi_hci, val);
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	if ((val >= 0) && (val <= 0x3ff))
		usb_new_phyx_write(sunxi_hci, val);
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	if ((val >= 0) && (val <= 0xfff))
		usb_new_phyx_write(sunxi_hci, val);
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW17) || IS_ENABLED(CONFIG_ARCH_SUN8IW11)
	if ((val >= 0) && (val <= 0x1f))
		usb_phyx_tp_write(sunxi_hci, 0x20, val, 5);
#else
	if ((val >= 0) && (val <= 0x3ff))
		usb_phyx_write(sunxi_hci, val);
#endif
}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW7) || IS_ENABLED(CONFIG_ARCH_SUN8IW12) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN50IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW6) || IS_ENABLED(CONFIG_ARCH_SUN50IW9) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW18)
static void usb_hci_utmi_phy_tune(struct sunxi_hci_hcd *sunxi_hci, int mask,
				  int offset, int val)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_TUNE);
	reg_value &= ~mask;
	val = val << offset;
	val &= mask;
	reg_value |= val;
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_TUNE));
}
#endif

static void USBC_SelectPhyToHci(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG);
	reg_value &= ~(0x01);
	USBC_Writel(reg_value, (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CFG));
}

static unsigned char USBC_SIDDP_Disable_Check(struct sunxi_hci_hcd *sunxi_hci)
{
	if (sunxi_hci->usbc_no == HCI0_USBC_NO && atomic_read(&usb0_enable_siddq_cnt) == 0)
		return 1;
	else if (sunxi_hci->usbc_no == HCI1_USBC_NO && atomic_read(&usb1_enable_siddq_cnt) == 0)
		return 1;
	else if (sunxi_hci->usbc_no == HCI2_USBC_NO && atomic_read(&usb2_enable_siddq_cnt) == 0)
		return 1;
	else if (sunxi_hci->usbc_no == HCI3_USBC_NO && atomic_read(&usb3_enable_siddq_cnt) == 0)
		return 1;
	else
		return 0;
}

static void USBC_Clean_SIDDP(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	if (sunxi_hci->usbc_no == HCI0_USBC_NO)
		atomic_add(1, &usb0_enable_siddq_cnt);
	else if (sunxi_hci->usbc_no == HCI1_USBC_NO)
		atomic_add(1, &usb1_enable_siddq_cnt);
	else if (sunxi_hci->usbc_no == HCI2_USBC_NO)
		atomic_add(1, &usb2_enable_siddq_cnt);
	else if (sunxi_hci->usbc_no == HCI3_USBC_NO)
		atomic_add(1, &usb3_enable_siddq_cnt);

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
	reg_value &= ~(0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ);
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
}

static void USBC_Set_SIDDP(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;
	if (sunxi_hci->usbc_no == HCI0_USBC_NO)
		atomic_sub(1, &usb0_enable_siddq_cnt);
	else if (sunxi_hci->usbc_no == HCI1_USBC_NO)
		atomic_sub(1, &usb1_enable_siddq_cnt);
	else if (sunxi_hci->usbc_no == HCI2_USBC_NO)
		atomic_sub(1, &usb2_enable_siddq_cnt);
	else if (sunxi_hci->usbc_no == HCI3_USBC_NO)
		atomic_sub(1, &usb3_enable_siddq_cnt);

	if (USBC_SIDDP_Disable_Check(sunxi_hci)) {
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value |= (0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ);
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
	}
}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
/* for common circuit */
void sunxi_hci_common_set_rc_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;
	reg_value = USBC_Readl(sunxi_hci->usb_common_phy_config
			+ SUNXI_USB_PMU_IRQ_ENABLE);
	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_RC16M_CLK_ENBALE;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_RC16M_CLK_ENBALE);
	USBC_Writel(reg_value, (sunxi_hci->usb_common_phy_config
				+ SUNXI_USB_PMU_IRQ_ENABLE));
}
void sunxi_hci_common_switch_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;
	reg_value = USBC_Readl(sunxi_hci->usb_common_phy_config
			+ SUNXI_USB_PMU_IRQ_ENABLE);
	if (is_on)
		reg_value |= 0x01 << 31;
	else
		reg_value &= ~(0x01 << 31);
	USBC_Writel(reg_value, (sunxi_hci->usb_common_phy_config
				+ SUNXI_USB_PMU_IRQ_ENABLE));
}
void sunxi_hci_common_set_rcgating(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;
	reg_value = USBC_Readl(sunxi_hci->usb_common_phy_config
			+ SUNXI_USB_PMU_IRQ_ENABLE);
	if (is_on)
		reg_value |= 0x01 << 3;
	else
		reg_value &= ~(0x01 << 3);
	USBC_Writel(reg_value, (sunxi_hci->usb_common_phy_config
				+ SUNXI_USB_PMU_IRQ_ENABLE));
}
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
static void sunxi_hci_switch_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int val = 0;
	val = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
	if (is_on)
		val |= 0x01 << 31;
	else
		val &= ~(0x01 << 31);
	USBC_Writel(val, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));
}
static void sunxi_hci_set_rcgating(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int val = 0;
	val = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
	if (is_on)
		val |= 0x01 << 3;
	else
		val &= ~(0x01 << 3);
	USBC_Writel(val, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));
}
#endif
/*
 * Low-power mode USB standby helper functions.
 */
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
void sunxi_hci_set_siddq(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);

	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
}

void sunxi_hci_set_vc_cfg(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	if (is_on) {
		/* must be set before host clk from ccu close */
		/* com_tune bit[9] set to 1 when enter usb standby */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x01 << 7)|(0x39 << 8)|(0x01 << 1)|(0x0 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
		mdelay(10);

		reg_value &= ~((0x01 <<7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x01 << 7)|(0x39 << 8)|(0x01 << 1)|(0x01 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));

		/* com_tune bit[10] set to 1 when enter usb standby */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x01 << 7)|(0x3a << 8)|(0x01 << 1)|(0x0 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
		mdelay(10);

		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x01 << 7)|(0x3a << 8)|(0x01 << 1)|(0x1 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));

		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value &= ~(0x01 << 1); /* clear phy vc en */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
	} else {
		/* cfg after switch to hclk from ccu */
		/* com_tune bit[9] set to 0 when exit usb standby */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x0 << 7)|(0x39 << 8)|(0x01 << 1)|(0x0 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
		mdelay(10);

		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x0 << 7)|(0x39 << 8)|(0x01 << 1)|(0x01 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));

		/* com_tune bit[10] set to 0 when exit usb standby */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x0 << 7)|(0x3a << 8)|(0x01 << 1)|(0x0 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
		mdelay(10);

		reg_value &= ~((0x01 << 7)|(0xff << 8)|(0x01 << 1)|(0x01 << 0)); /* clear phy vc cfg */
		reg_value |= (0x0 << 7)|(0x3a << 8)|(0x01 << 1)|(0x1 << 0); /* bit[15:8] vc_addr */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));

		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
		reg_value &= ~(0x01 << 1); /* clear phy vc en */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
	}
}

void sunxi_hci_set_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	if (is_on) {
		/* usb clk switch to rc16M */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL);
		reg_value |= (0x01 << SUNXI_HCI_STANDBY_CLK_SEL);
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL));

		udelay(1);

		/* RC gen and rc clock ungated */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL);
		reg_value |= ((0x01 << SUNXI_HCI_RC_CLK_GATING) | (0x01 << SUNXI_HCI_RC_GEN_ENABLE)); /* 0xc */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL));
	} else {
		/* RC disable and rc clock gated */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL);
		reg_value &= ~((0x01 << SUNXI_HCI_RC_CLK_GATING) | (0x01 << SUNXI_HCI_RC_GEN_ENABLE)); /* 0xc */
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL));

		udelay(1);

		/* usb clk switch to ccu clk */
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL);
		reg_value &= ~(0x01 << SUNXI_HCI_STANDBY_CLK_SEL);
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_USB_CTRL));
	}
}

void sunxi_hci_set_wakeup_ctrl(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3);

	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_CTRL_3_REMOTE_WAKEUP;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_CTRL_3_REMOTE_WAKEUP);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3));
}

void sunxi_hci_set_rc_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);

	if (is_on)
		reg_value |= 0x01 << SUNXI_HCI_RC16M_CLK_ENBALE;
	else
		reg_value &= ~(0x01 << SUNXI_HCI_RC16M_CLK_ENBALE);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));
}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
void sunxi_hci_set_standby_irq(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT);

	if (is_on)
		reg_value |= 0x01 << SUNXI_USB_EHCI_STANDBY_IRQ;
	else
		reg_value &= ~(0x01 << SUNXI_USB_EHCI_STANDBY_IRQ);

	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT));
}
#endif

void sunxi_hci_clean_standby_irq(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT);
	reg_value |= 0x01 << SUNXI_USB_EHCI_STANDBY_IRQ_STATUS;
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_EHCI_TIME_INT));
}
#endif

static int __maybe_unused usb_rescal_clock_set(struct sunxi_hci_hcd *sunxi_hci, bool enable)
{
	int ret = 0;

	if (sunxi_hci->rext_cal_bypass) {
		DMSG_DEBUG("external resistance calibration bypass, skip set clk_res\n");
		return 0;
	}

	if (enable) {
		if (sunxi_hci->clk_res) {
			ret = clk_prepare_enable(sunxi_hci->clk_res);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable clk_res err, return %d\n", ret);
				return ret;
			}
		}
	} else {
		if (sunxi_hci->clk_res)
			clk_disable_unprepare(sunxi_hci->clk_res);
	}
	return 0;
}

static int open_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	int ret;

	mutex_lock(&usb_clock_lock);

	/*
	 * otg and hci share the same phy in fpga,
	 * so need switch phy to hci here.
	 * Notice: not need any more on new platforms.
	 */

	if (sunxi_hci->hci_regulator) {
		ret = regulator_enable(sunxi_hci->hci_regulator);
		if (ret)
			DMSG_ERR("ERR:%s hci regulator enable failed\n",
					sunxi_hci->hci_name);
	}

	/* To fix hardware design issue. */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12) || IS_ENABLED(CONFIG_ARCH_SUN50IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW6)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_RES_TUNE, SUNXI_TX_RES_TUNE_OFFSET, 0x3);
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_VREF_TUNE, SUNXI_TX_VREF_TUNE_OFFSET, 0xc);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW18)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_PREEMPAMP_TUNE, SUNXI_TX_PREEMPAMP_TUNE_OFFSET, 0x10);
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_VREF_TUNE, SUNXI_TX_VREF_TUNE_OFFSET, 0xc);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW7) || IS_ENABLED(CONFIG_ARCH_SUN8IW15)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_VREF_TUNE, SUNXI_TX_VREF_TUNE_OFFSET, 0x4);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_PREEMPAMP_TUNE, SUNXI_TX_PREEMPAMP_TUNE_OFFSET, 0x2);
#endif

	if (!sunxi_hci->clk_is_open) {
		sunxi_hci->clk_is_open = 1;

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		usb_rescal_clock_set(sunxi_hci, true);
		usb_phyx_res_cal(sunxi_hci->usbc_no, true, sunxi_hci->rext_cal_bypass);
#endif
		if (sunxi_hci->ahb) {
			ret = clk_prepare_enable(sunxi_hci->ahb);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "ERR:try to prepare_enable %s_ahb failed!", sunxi_hci->hci_name);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_msi_lite) {
			ret = clk_prepare_enable(sunxi_hci->clk_msi_lite);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable clk_msi_lite err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_usb_sys_ahb) {
			ret = clk_prepare_enable(sunxi_hci->clk_usb_sys_ahb);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable clk_usb_sys_ahb err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_mbus) {
			ret = clk_prepare_enable(sunxi_hci->clk_mbus);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable clk_mbus err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_hclk) {
			ret = clk_prepare_enable(sunxi_hci->clk_hclk);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable hclk err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_hosc) {
			ret = clk_prepare_enable(sunxi_hci->clk_hosc);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable clk_hosc err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		/**
		 * NOTE: For some boards on sun55iw3's platform, the usb1 port can't enumerate USB device.
		 * It's necessary to adjust clock order, especially for phy's reset clock.
		 */
		if (sunxi_hci->reset_hci) {
			ret = reset_control_deassert(sunxi_hci->reset_hci);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "reset hci err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_bus_hci) {
			ret = clk_prepare_enable(sunxi_hci->clk_bus_hci);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "enable clk_bus_hci err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->clk_ohci) {
			if (strstr(sunxi_hci->hci_name, "ohci")) {
				ret = clk_prepare_enable(sunxi_hci->clk_ohci);
				if (ret) {
					sunxi_err(&sunxi_hci->pdev->dev,
						"enable clk_ohci err, return %d\n", ret);
					mutex_unlock(&usb_clock_lock);
					return ret;
				}
			}
		}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
			int val;

			val = readl(sunxi_hci->usb_ccmu_config + 0x0A8C);
			val |= (SUNXI_CCMU_USBEHCI1_GATING_OFFSET
				| SUNXI_CCMU_USBEHCI1_RST_OFFSET);
			writel(val, sunxi_hci->usb_ccmu_config + 0x0A8C);

			val = readl(sunxi_hci->usb_ccmu_config + 0x0A74);
			val |= (SUNXI_CCMU_SCLK_GATING_USBPHY1_OFFSET
				| SUNXI_CCMU_USBPHY1_RST_OFFSET
				| SUNXI_CCMU_SCLK_GATING_OHCI1_OFFSET);
			writel(val, sunxi_hci->usb_ccmu_config + 0x0A74);

			/* phy reg, offset:0x10 bit3 set 0, enable siddq */
			val = USBC_Readl(sunxi_hci->usb_common_phy_config
					 + SUNXI_HCI_PHY_CTRL);
			val &= ~(0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
			USBC_Writel(val, sunxi_hci->usb_common_phy_config
				    + SUNXI_HCI_PHY_CTRL);
		}
#endif
		udelay(10);

		if (sunxi_hci->hsic_flag) {
			if (sunxi_hci->hsic_ctrl_flag) {
				if (sunxi_hci->hsic_enable_flag) {
					if (clk_prepare_enable(sunxi_hci->pll_hsic))
						DMSG_ERR("ERR:try to prepare_enable %s pll_hsic failed!\n",
							sunxi_hci->hci_name);

					if (clk_prepare_enable(sunxi_hci->clk_usbhsic12m))
						DMSG_ERR("ERR:try to prepare_enable %s clk_usbhsic12m failed!\n",
							sunxi_hci->hci_name);

					if (clk_prepare_enable(sunxi_hci->hsic_usbphy))
						DMSG_ERR("ERR:try to prepare_enable %s_hsic_usbphy failed!\n",
							sunxi_hci->hci_name);
				}
			} else {
				if (clk_prepare_enable(sunxi_hci->pll_hsic))
					DMSG_ERR("ERR:try to prepare_enable %s pll_hsic failed!\n",
						sunxi_hci->hci_name);

				if (clk_prepare_enable(sunxi_hci->clk_usbhsic12m))
					DMSG_ERR("ERR:try to prepare_enable %s clk_usbhsic12m failed!\n",
						sunxi_hci->hci_name);

				if (clk_prepare_enable(sunxi_hci->hsic_usbphy))
					DMSG_ERR("ERR:try to prepare_enable %s_hsic_usbphy failed!\n",
						sunxi_hci->hci_name);
			}
		} else {
			if (sunxi_hci->clk_phy) {
				ret = clk_prepare_enable(sunxi_hci->clk_phy);
				if (ret) {
					sunxi_err(&sunxi_hci->pdev->dev, "enable clk_phy err, return %d\n", ret);
					mutex_unlock(&usb_clock_lock);
					return ret;
				}
			}

			if (sunxi_hci->mod_usbphy) {
				ret = clk_prepare_enable(sunxi_hci->mod_usbphy);
				if (ret) {
					sunxi_err(&sunxi_hci->pdev->dev, "enable mod_usbphy err, return %d\n", ret);
					mutex_unlock(&usb_clock_lock);
					return ret;
				}
			}
		}

		udelay(10);

		/* otg and hci0 Controller Shared phy in SUN50I */
		if (sunxi_hci->usbc_no == HCI0_USBC_NO)
			USBC_SelectPhyToHci(sunxi_hci);

		if (!sunxi_hci->usb2_generic_phy)
			USBC_Clean_SIDDP(sunxi_hci);

		if (sunxi_hci->reset_phy) {
			ret = reset_control_deassert(sunxi_hci->reset_phy);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "reset phy err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		if (sunxi_hci->reset_usb) {
			ret = reset_control_deassert(sunxi_hci->reset_usb);
			if (ret) {
				sunxi_err(&sunxi_hci->pdev->dev, "reset usb err, return %d\n", ret);
				mutex_unlock(&usb_clock_lock);
				return ret;
			}
		}

		ret = phy_init(sunxi_hci->usb2_generic_phy);
		if (ret < 0) {
			sunxi_err(&sunxi_hci->pdev->dev, "init phy err, return %d\n", ret);
			mutex_unlock(&usb_clock_lock);
			return ret;
		}
	} else {
		DMSG_WARN("[%s]: wrn: open clock failed, (%d, 0x%p)\n",
			sunxi_hci->hci_name,
			sunxi_hci->clk_is_open,
			sunxi_hci->mod_usb);
	}

#if !IS_ENABLED(CONFIG_ARCH_SUN8IW12) && !IS_ENABLED(CONFIG_ARCH_SUN50IW3) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN8IW6) && !IS_ENABLED(CONFIG_ARCH_SUN50IW6) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN8IW15) && !IS_ENABLED(CONFIG_ARCH_SUN50IW8) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN8IW18) && !IS_ENABLED(CONFIG_ARCH_SUN8IW16) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN50IW9) && !IS_ENABLED(CONFIG_ARCH_SUN50IW10) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN8IW19) && !IS_ENABLED(CONFIG_ARCH_SUN50IW11) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN8IW20) && !IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN50IW12) && !IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN60IW2) && !IS_ENABLED(CONFIG_ARCH_SUN8IW21) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN55IW6) && !IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN65IW1) && !IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	usb_phyx_tp_write(sunxi_hci, 0x2a, 3, 2);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	usb_new_phy_init(sunxi_hci);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	/* Increase USB disconnection detection voltage */
	usb_new_phyx_tp_write(sunxi_hci, 0x83, 0x6, 0x3);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	/* Adapt USB phy parameters */
	usb_new_phyx_write(sunxi_hci, 0x22f);
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	usb_new_phyx_write(sunxi_hci, 0x228);
	/* Adapt USB phy bandwidth tuning parameters to optimize Jitter*/
	usb_new_phyx_tp_write(sunxi_hci, 0x3, 0x3f, 0x6);
#elif IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	usb_new_phyx_write(sunxi_hci, 0x208);
#elif IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	usb_new_phyx_write(sunxi_hci, 0x2e8);
#endif
#endif

	/* Modify the phy range parameter of USB Host
	 * by reading the parameters-phy_range from DTS.*/
	usb_phyx_reassign(sunxi_hci, sunxi_hci->phy_range);

	mutex_unlock(&usb_clock_lock);

	return 0;
}

static int close_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	if (sunxi_hci->usb2_generic_phy)
		phy_exit(sunxi_hci->usb2_generic_phy);
	else
		USBC_Set_SIDDP(sunxi_hci);

	if (sunxi_hci->clk_is_open) {
		sunxi_hci->clk_is_open = 0;

		if (sunxi_hci->hsic_flag) {
			if (sunxi_hci->hsic_ctrl_flag) {
				if (sunxi_hci->hsic_enable_flag) {
					clk_disable_unprepare(sunxi_hci->clk_usbhsic12m);
					clk_disable_unprepare(sunxi_hci->hsic_usbphy);
					clk_disable_unprepare(sunxi_hci->pll_hsic);
				}
			} else {
				clk_disable_unprepare(sunxi_hci->clk_usbhsic12m);
				clk_disable_unprepare(sunxi_hci->hsic_usbphy);
				clk_disable_unprepare(sunxi_hci->pll_hsic);
			}
		} else {
			if (sunxi_hci->clk_phy)
				clk_disable_unprepare(sunxi_hci->clk_phy);

			if (sunxi_hci->mod_usbphy)
				clk_disable_unprepare(sunxi_hci->mod_usbphy);

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
			if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
				int val;

				val = readl(sunxi_hci->usb_ccmu_config + 0x0A8C);
				val &= ~(SUNXI_CCMU_USBEHCI1_GATING_OFFSET
					| SUNXI_CCMU_USBEHCI1_RST_OFFSET);
				writel(val, sunxi_hci->usb_ccmu_config + 0x0A8C);

				val = readl(sunxi_hci->usb_ccmu_config + 0x0A74);
				val &= ~(SUNXI_CCMU_SCLK_GATING_USBPHY1_OFFSET
					| SUNXI_CCMU_USBPHY1_RST_OFFSET);
				writel(val, sunxi_hci->usb_ccmu_config + 0x0A74);

				/* phy reg, offset:0x10 bit3 set 0, enable siddq */
				val = USBC_Readl(sunxi_hci->usb_common_phy_config
						 + SUNXI_HCI_PHY_CTRL);
				val |= (0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
				USBC_Writel(val, sunxi_hci->usb_common_phy_config
					    + SUNXI_HCI_PHY_CTRL);
			}
#endif
		}

		if (strstr(sunxi_hci->hci_name, "ohci"))
			if (sunxi_hci->clk_ohci)
				clk_disable_unprepare(sunxi_hci->clk_ohci);

		if (sunxi_hci->clk_bus_hci)
			clk_disable_unprepare(sunxi_hci->clk_bus_hci);

		if (sunxi_hci->reset_hci)
			reset_control_assert(sunxi_hci->reset_hci);

		if (sunxi_hci->clk_hosc)
			clk_disable_unprepare(sunxi_hci->clk_hosc);

		if (sunxi_hci->clk_hclk)
			clk_disable_unprepare(sunxi_hci->clk_hclk);

		if (sunxi_hci->clk_mbus)
			clk_disable_unprepare(sunxi_hci->clk_mbus);

		if (sunxi_hci->ahb)
			clk_disable_unprepare(sunxi_hci->ahb);

		if (sunxi_hci->reset_phy)
			reset_control_assert(sunxi_hci->reset_phy);

		if (sunxi_hci->reset_usb)
			reset_control_assert(sunxi_hci->reset_usb);

		if (sunxi_hci->clk_usb_sys_ahb)
			clk_disable_unprepare(sunxi_hci->clk_usb_sys_ahb);

		if (sunxi_hci->clk_msi_lite)
			clk_disable_unprepare(sunxi_hci->clk_msi_lite);

		udelay(10);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		usb_phyx_res_cal(sunxi_hci->usbc_no, false, sunxi_hci->rext_cal_bypass);
		usb_rescal_clock_set(sunxi_hci, false);
#endif
	} else {
		DMSG_WARN("[%s]: wrn: open clock failed, (%d, 0x%p)\n",
			sunxi_hci->hci_name,
			sunxi_hci->clk_is_open,
			sunxi_hci->mod_usb);
	}
	return 0;
}

static int usb_get_hsic_phy_ctrl(int value, int enable)
{
	if (enable) {
		value |= (0x07<<8);
		value |= (0x01<<1);
		value |= (0x01<<0);
		value |= (0x01<<16);
		value |= (0x01<<20);
	} else {
		value &= ~(0x07<<8);
		value &= ~(0x01<<1);
		value &= ~(0x01<<0);
		value &= ~(0x01<<16);
		value &= ~(0x01<<20);
	}

	return value;
}

static void __usb_passby(struct sunxi_hci_hcd *sunxi_hci, u32 enable,
			     atomic_t *usb_enable_passly_cnt)
{
	unsigned long reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
	if (enable && (atomic_read(usb_enable_passly_cnt) == 0)) {
		if (sunxi_hci->hsic_flag) {
			reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
		} else {
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN65IW1)
			/* bypass ohci bulk out changes */
			reg_value |= (1 << 15);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
			/* AHB Master interface INCR16 enable */
			reg_value |= (1 << 11);
#else
			/* AHB Master interface INCR8 enable */
			reg_value |= (1 << 10);
#endif
			/* AHB Master interface burst type INCR4 enable */
			reg_value |= (1 << 9);
			/* AHB Master interface INCRX align enable */
			reg_value |= (1 << 8);
			if (sunxi_hci->usbc_no == HCI0_USBC_NO)
#ifdef SUNXI_USB_FPGA
				/* enable ULPI, disable UTMI */
				reg_value |= (0 << 0);
#else
				/* enable UTMI, disable ULPI */
				reg_value |= (1 << 0);
#endif
			else
				/* ULPI bypass enable */
				reg_value |= (1 << 0);
		}
	} else if (!enable && (atomic_read(usb_enable_passly_cnt) == 1)) {
		if (sunxi_hci->hsic_flag) {
			reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
		} else {
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN65IW1)
			/* Not bypass ohci bulk out changes */
			reg_value &= ~(1 << 15);
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
			/* AHB Master interface INCR16 disable */
			reg_value &= ~(1 << 11);
#else
			/* AHB Master interface INCR8 disable */
			reg_value &= ~(1 << 10);
#endif
			/* AHB Master interface burst type INCR4 disable */
			reg_value &= ~(1 << 9);
			/* AHB Master interface INCRX align disable */
			reg_value &= ~(1 << 8);
			/* ULPI bypass disable */
			reg_value &= ~(1 << 0);
		}
	}
	USBC_Writel(reg_value,
		(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

	if (enable)
		atomic_add(1, usb_enable_passly_cnt);
	else
		atomic_sub(1, usb_enable_passly_cnt);
}

static void usb_passby(struct sunxi_hci_hcd *sunxi_hci, u32 enable)
{
	spinlock_t lock;
	unsigned long flags = 0;

	mutex_lock(&usb_passby_lock);

	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	/* enable passby */
	if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
		__usb_passby(sunxi_hci, enable, &usb1_enable_passly_cnt);
	} else if (sunxi_hci->usbc_no == HCI1_USBC_NO) {
		__usb_passby(sunxi_hci, enable, &usb2_enable_passly_cnt);
	} else if (sunxi_hci->usbc_no == HCI2_USBC_NO) {
		__usb_passby(sunxi_hci, enable, &usb3_enable_passly_cnt);
	} else if (sunxi_hci->usbc_no == HCI3_USBC_NO) {
		__usb_passby(sunxi_hci, enable, &usb4_enable_passly_cnt);
	} else {
		DMSG_ERR("EER: unknown usbc_no(%d)\n", sunxi_hci->usbc_no);
		spin_unlock_irqrestore(&lock, flags);

		mutex_unlock(&usb_passby_lock);
		return;
	}

	spin_unlock_irqrestore(&lock, flags);

	mutex_unlock(&usb_passby_lock);
}

static int alloc_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	u32 ret = 1;

	if (sunxi_hci->gma340_oe_gpio_valid) {
		ret = gpio_request(sunxi_hci->gma340_oe_gpio_set.gpio, NULL);
		if (ret != 0) {
			DMSG_ERR("request %s gpio:%d\n", sunxi_hci->hci_name,
				   sunxi_hci->gma340_oe_gpio_set.gpio);
		} else {
			gpio_direction_output(sunxi_hci->gma340_oe_gpio_set.gpio, 0);
		}
	}

	if (sunxi_hci->drvvbus_en_gpio_valid) {
		ret = gpio_request(sunxi_hci->drvvbus_en_gpio_set.gpio, NULL);
		if (ret != 0) {
			DMSG_ERR("request %s gpio:%d\n", sunxi_hci->hci_name,
				   sunxi_hci->drvvbus_en_gpio_set.gpio);
		} else {
			gpio_direction_output(sunxi_hci->drvvbus_en_gpio_set.gpio, 0);
		}
	}

	if (sunxi_hci->hsic_flag) {
		/* Marvell 4G HSIC ctrl */
		if (sunxi_hci->usb_host_hsic_rdy_valid) {
			ret = gpio_request(sunxi_hci->usb_host_hsic_rdy.gpio, NULL);
			if (ret != 0) {
				DMSG_ERR("ERR: gpio_request failed\n");
				sunxi_hci->usb_host_hsic_rdy_valid = 0;
			} else {
				gpio_direction_output(sunxi_hci->usb_host_hsic_rdy.gpio, 0);
			}
		}

		/* SMSC usb3503 HSIC HUB ctrl */
		if (sunxi_hci->usb_hsic_usb3503_flag) {
			if (sunxi_hci->usb_hsic_hub_connect_valid) {
				ret = gpio_request(sunxi_hci->usb_hsic_hub_connect.gpio, NULL);
				if (ret != 0) {
					DMSG_ERR("ERR: gpio_request failed\n");
					sunxi_hci->usb_hsic_hub_connect_valid = 0;
				} else {
					gpio_direction_output(sunxi_hci->usb_hsic_hub_connect.gpio, 1);
				}
			}

			if (sunxi_hci->usb_hsic_int_n_valid) {
				ret = gpio_request(sunxi_hci->usb_hsic_int_n.gpio, NULL);
				if (ret != 0) {
					DMSG_ERR("ERR: gpio_request failed\n");
					sunxi_hci->usb_hsic_int_n_valid = 0;
				} else {
					gpio_direction_output(sunxi_hci->usb_hsic_int_n.gpio, 1);
				}
			}

			msleep(20);

			if (sunxi_hci->usb_hsic_reset_n_valid) {
				ret = gpio_request(sunxi_hci->usb_hsic_reset_n.gpio, NULL);
				if (ret != 0) {
					DMSG_ERR("ERR: gpio_request failed\n");
					sunxi_hci->usb_hsic_reset_n_valid = 0;
				} else {
					gpio_direction_output(sunxi_hci->usb_hsic_reset_n.gpio, 1);
				}
			}

			/**
			 * usb3503 device goto hub connect status
			 * is need 100ms after reset
			 */
			msleep(100);
		}
	}

	return 0;
}

static void free_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	if (sunxi_hci->gma340_oe_gpio_valid) {
		gpio_free(sunxi_hci->gma340_oe_gpio_set.gpio);
		sunxi_hci->gma340_oe_gpio_valid = 0;
	}

	if (sunxi_hci->drvvbus_en_gpio_valid) {
		gpio_free(sunxi_hci->drvvbus_en_gpio_set.gpio);
		sunxi_hci->drvvbus_en_gpio_valid = 0;
	}

	if (sunxi_hci->hsic_flag) {
		/* Marvell 4G HSIC ctrl */
		if (sunxi_hci->usb_host_hsic_rdy_valid) {
			gpio_free(sunxi_hci->usb_host_hsic_rdy.gpio);
			sunxi_hci->usb_host_hsic_rdy_valid = 0;
		}

		/* SMSC usb3503 HSIC HUB ctrl */
		if (sunxi_hci->usb_hsic_usb3503_flag) {
			if (sunxi_hci->usb_hsic_hub_connect_valid) {
				gpio_free(sunxi_hci->usb_hsic_hub_connect.gpio);
				sunxi_hci->usb_hsic_hub_connect_valid = 0;
			}

			if (sunxi_hci->usb_hsic_int_n_valid) {
				gpio_free(sunxi_hci->usb_hsic_int_n.gpio);
				sunxi_hci->usb_hsic_int_n_valid = 0;
			}

			if (sunxi_hci->usb_hsic_reset_n_valid) {
				gpio_free(sunxi_hci->usb_hsic_reset_n.gpio);
				sunxi_hci->usb_hsic_reset_n_valid = 0;
			}
		}
	}
}

void sunxi_set_host_hisc_rdy(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	if (sunxi_hci->usb_host_hsic_rdy_valid) {
		/* set config, output */
		gpio_direction_output(sunxi_hci->usb_host_hsic_rdy.gpio, is_on);
	}
}
EXPORT_SYMBOL(sunxi_set_host_hisc_rdy);

void sunxi_set_host_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int ret;

	if (sunxi_hci->supply) {
		if (is_on) {
			ret = regulator_enable(sunxi_hci->supply);
			if (ret)
				DMSG_ERR("ERR: %s regulator enable failed\n",
					sunxi_hci->hci_name);
		} else {
			ret = regulator_disable(sunxi_hci->supply);
			if (ret)
				DMSG_ERR("ERR: %s regulator force disable failed\n",
					sunxi_hci->hci_name);
		}
	}

	if (sunxi_hci->gma340_oe_gpio_valid)
		gpio_set_value(sunxi_hci->gma340_oe_gpio_set.gpio,
				 is_on);

	if (sunxi_hci->drvvbus_en_type == USB_DRVVBUS_EN_TYPE_GPIO) {
		if (sunxi_hci->drvvbus_en_gpio_valid)
			gpio_set_value(sunxi_hci->drvvbus_en_gpio_set.gpio,
					 is_on);
	}
}
EXPORT_SYMBOL(sunxi_set_host_vbus);

static void __sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int ret;

	/* set power flag */
	sunxi_hci->power_flag = is_on;

	if (sunxi_hci->hsic_flag) {
		if ((sunxi_hci->hsic_regulator_io != NULL) &&
				(sunxi_hci->hsic_regulator_io_hdle != NULL)) {
			if (is_on) {
				if (regulator_enable(sunxi_hci->hsic_regulator_io_hdle) < 0)
					DMSG_INFO("%s: hsic_regulator_enable fail\n",
						sunxi_hci->hci_name);
			} else {
				if (regulator_disable(sunxi_hci->hsic_regulator_io_hdle) < 0)
					DMSG_INFO("%s: hsic_regulator_disable fail\n",
						sunxi_hci->hci_name);
			}
		}
	}

	if (sunxi_hci->supply) {
		if (is_on) {
			ret = regulator_enable(sunxi_hci->supply);
			if (ret)
				DMSG_ERR("ERR: %s regulator enable failed\n",
					sunxi_hci->hci_name);
		} else {
			ret = regulator_disable(sunxi_hci->supply);
			if (ret)
				DMSG_ERR("ERR: %s regulator force disable failed\n",
					sunxi_hci->hci_name);
		}
	}

	if (sunxi_hci->gma340_oe_gpio_valid)
		gpio_set_value(sunxi_hci->gma340_oe_gpio_set.gpio,
				 is_on);

	if (sunxi_hci->drvvbus_en_type == USB_DRVVBUS_EN_TYPE_GPIO) {
		if (sunxi_hci->drvvbus_en_gpio_valid)
			gpio_set_value(sunxi_hci->drvvbus_en_gpio_set.gpio,
					 is_on);
	}

	if (sunxi_hci->hci_regulator) {
		if (!is_on) {
			ret = regulator_disable(sunxi_hci->hci_regulator);
			if (ret)
				DMSG_ERR("ERR: %s hci regulator force disable failed\n",
					sunxi_hci->hci_name);
		}
	}
}

static void sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	mutex_lock(&usb_vbus_lock);

	if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
		if (is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		else if (!is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
	} else if (sunxi_hci->usbc_no == HCI1_USBC_NO) {
		if (is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		else if (!is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
	} else if (sunxi_hci->usbc_no == HCI2_USBC_NO) {
		if (is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		else if (!is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
	} else if (sunxi_hci->usbc_no == HCI3_USBC_NO) {
		if (is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		else if (!is_on)
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
	} else {
		DMSG_INFO("[%s]: sunxi_set_vbus no: %d\n",
			sunxi_hci->hci_name, sunxi_hci->usbc_no);
	}

	mutex_unlock(&usb_vbus_lock);
}

static int sunxi_get_hci_base(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	sunxi_hci->usb_vbase  = of_iomap(np, 0);
	if (sunxi_hci->usb_vbase == NULL) {
		sunxi_err(&pdev->dev, "%s, can't get vbase resource\n",
			sunxi_hci->hci_name);
		return -EINVAL;
	}

	if (strstr(sunxi_hci->hci_name, "ohci"))
		sunxi_hci->usb_vbase  -= SUNXI_USB_OHCI_BASE_OFFSET;

	sunxi_hci->otg_vbase  = of_iomap(np, 2);
	if (sunxi_hci->otg_vbase == NULL) {
		sunxi_err(&pdev->dev, "%s, can't get otg_vbase resource\n",
			sunxi_hci->hci_name);
		goto err1;
	}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	sunxi_hci->prcm = of_iomap(np, 3);
	if (sunxi_hci->prcm == NULL) {
		sunxi_err(&pdev->dev, "%s, can't get prcm resource\n",
			sunxi_hci->hci_name);
		goto err2;
	}

	if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
		sunxi_hci->usb_ccmu_config = of_iomap(np, 4);
		if (sunxi_hci->usb_ccmu_config == NULL) {
			sunxi_err(&pdev->dev, "%s, can't get ccmu resource\n",
				sunxi_hci->hci_name);
			goto err3;
		}

		sunxi_hci->usb_common_phy_config = of_iomap(np, 5);
		if (sunxi_hci->usb_common_phy_config == NULL) {
			sunxi_err(&pdev->dev, "%s, can't get common phy resource\n",
				sunxi_hci->hci_name);
			goto err4;
		}
	}
#endif

	sunxi_hci->usb_base_res = kzalloc(sizeof(struct resource), GFP_KERNEL);
	if (sunxi_hci->usb_base_res == NULL) {
		sunxi_err(&pdev->dev, "%s, can't alloc mem\n",
			sunxi_hci->hci_name);
		goto err5;
	}

	ret = of_address_to_resource(np, 0, sunxi_hci->usb_base_res);
	if (ret)
		sunxi_err(&pdev->dev, "could not get regs\n");

	return 0;

err5:
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	if (sunxi_hci->usb_common_phy_config != NULL)
		iounmap(sunxi_hci->usb_common_phy_config);

err4:
	if (sunxi_hci->usb_ccmu_config != NULL)
		iounmap(sunxi_hci->usb_ccmu_config);

err3:
	iounmap(sunxi_hci->prcm);

err2:
	iounmap(sunxi_hci->otg_vbase);

#endif
err1:
	iounmap(sunxi_hci->usb_vbase);

	return -EINVAL;
}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW17)
static int sunxi_get_ohci_clock_src(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;

	sunxi_hci->clk_usbohci12m = of_clk_get(np, 2);
	if (IS_ERR(sunxi_hci->clk_usbohci12m)) {
		sunxi_hci->clk_usbohci12m = NULL;
		DMSG_INFO("%s get usb clk_usbohci12m clk failed.\n",
			sunxi_hci->hci_name);
	}

	sunxi_hci->clk_hoscx2 = of_clk_get(np, 3);
	if (IS_ERR(sunxi_hci->clk_hoscx2)) {
		sunxi_hci->clk_hoscx2 = NULL;
		DMSG_INFO("%s get usb clk_hoscx2 clk failed.\n",
			 sunxi_hci->hci_name);
	}

	sunxi_hci->clk_hosc = of_clk_get(np, 4);
	if (IS_ERR(sunxi_hci->clk_hosc)) {
		sunxi_hci->clk_hosc = NULL;
		DMSG_INFO("%s get usb clk_hosc failed.\n",
			 sunxi_hci->hci_name);
	}

	sunxi_hci->clk_losc = of_clk_get(np, 5);
	if (IS_ERR(sunxi_hci->clk_losc)) {
		sunxi_hci->clk_losc = NULL;
		DMSG_INFO("%s get usb clk_losc clk failed.\n",
			 sunxi_hci->hci_name);
	}

	return 0;
}
#endif

static int sunxi_get_hci_clock(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW17)
	sunxi_hci->ahb = of_clk_get(np, 1);
	if (IS_ERR(sunxi_hci->ahb)) {
		sunxi_hci->ahb = NULL;
		sunxi_err(&pdev->dev, "ERR: %s get usb ahb_otg clk failed.\n",
				sunxi_hci->hci_name);
		return -EINVAL;
	}

	sunxi_hci->mod_usbphy = of_clk_get(np, 0);
	if (IS_ERR(sunxi_hci->mod_usbphy)) {
		sunxi_hci->mod_usbphy = NULL;
		sunxi_err(&pdev->dev, "ERR: %s get usb mod_usbphy failed.\n",
				sunxi_hci->hci_name);
		return -EINVAL;
	}
#else
	sunxi_hci->clk_bus_hci = devm_clk_get(&pdev->dev, "bus_hci");
	if (IS_ERR(sunxi_hci->clk_bus_hci)) {
		sunxi_err(&pdev->dev, "Could not get bus_hci clock\n");
		return PTR_ERR(sunxi_hci->clk_bus_hci);
	}

	sunxi_hci->reset_hci = devm_reset_control_get(&pdev->dev, "hci");
	if (IS_ERR(sunxi_hci->reset_hci))
		sunxi_warn(&pdev->dev, "Could not get hci rst\n");

	if (strstr(sunxi_hci->hci_name, "ehci")) {
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
#if !IS_ENABLED(CONFIG_ARCH_SUN65IW1)
			sunxi_hci->reset_phy = devm_reset_control_get_shared(&pdev->dev, "phy");
			if (IS_ERR(sunxi_hci->reset_phy))
				sunxi_warn(&pdev->dev, "Could not get phy rst share\n");
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
			sunxi_hci->clk_hclk = devm_clk_get(&pdev->dev, "hclk");
			if (IS_ERR(sunxi_hci->clk_hclk)) {
				sunxi_err(&pdev->dev, "Could not get hclk clock\n");
				return PTR_ERR(sunxi_hci->clk_hclk);
			}
#endif
		} else {
#if !IS_ENABLED(CONFIG_ARCH_SUN65IW1)
			sunxi_hci->reset_phy = devm_reset_control_get_optional_shared(&pdev->dev, "phy");
			if (IS_ERR(sunxi_hci->reset_phy))
				sunxi_warn(&pdev->dev, "Could not get phy rst\n");
#endif
		}
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
		sunxi_hci->reset_usb = devm_reset_control_get_shared(&pdev->dev, "usb_rst");
		if (IS_ERR(sunxi_hci->reset_usb))
			sunxi_warn(&pdev->dev, "Could not get usb_rst share\n");
#endif
	}

	if (strstr(sunxi_hci->hci_name, "ohci")) {
		sunxi_hci->clk_ohci = devm_clk_get(&pdev->dev, "ohci");
		if (IS_ERR(sunxi_hci->clk_ohci)) {
			sunxi_err(&pdev->dev, "Could not get ohci clock\n");
			return PTR_ERR(sunxi_hci->clk_ohci);
		}

#if !IS_ENABLED(CONFIG_ARCH_SUN65IW1)
		sunxi_hci->reset_phy = devm_reset_control_get_shared(&pdev->dev, "phy");
		if (IS_ERR(sunxi_hci->reset_phy))
			sunxi_warn(&pdev->dev, "Could not get phy rst share\n");
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
		sunxi_hci->reset_usb = devm_reset_control_get_shared(&pdev->dev, "usb_rst");
		if (IS_ERR(sunxi_hci->reset_usb))
			sunxi_warn(&pdev->dev, "Could not get usb_rst share\n");
#endif
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	sunxi_hci->clk_hosc = devm_clk_get(&pdev->dev, "hosc");
	if (IS_ERR(sunxi_hci->clk_hosc)) {
		sunxi_err(&pdev->dev, "Could not get hosc clock\n");
		return PTR_ERR(sunxi_hci->clk_hosc);
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	sunxi_hci->clk_res = devm_clk_get(&pdev->dev, "res_dcap");
	if (IS_ERR(sunxi_hci->clk_res)) {
		sunxi_err(&pdev->dev, "Could not get res dcap clock\n");
		return PTR_ERR(sunxi_hci->clk_res);
	}

	sunxi_hci->clk_msi_lite = devm_clk_get(&pdev->dev, "msi_lite");
	if (IS_ERR(sunxi_hci->clk_msi_lite)) {
		dev_err(&pdev->dev, "Could not get msi-lite clock\n");
		return PTR_ERR(sunxi_hci->clk_msi_lite);
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	sunxi_hci->clk_usb_sys_ahb = devm_clk_get(&pdev->dev, "usb_sys_ahb");
	if (IS_ERR(sunxi_hci->clk_usb_sys_ahb)) {
		dev_err(&pdev->dev, "Could not get usb-sys-ahb clock\n");
		return PTR_ERR(sunxi_hci->clk_usb_sys_ahb);
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	sunxi_hci->clk_mbus = devm_clk_get(&pdev->dev, "mbus");
	if (IS_ERR(sunxi_hci->clk_mbus)) {
		sunxi_err(&pdev->dev, "Could not get mbus clock\n");
		return PTR_ERR(sunxi_hci->clk_mbus);
	}

	sunxi_hci->clk_rate = devm_clk_get(&pdev->dev, "clk_rate");
	if (sunxi_hci->clk_rate)
		sunxi_hci->rate_clk = clk_get_rate(sunxi_hci->clk_rate);
#endif


#if !IS_ENABLED(CONFIG_ARCH_SUN8IW20) && !IS_ENABLED(CONFIG_ARCH_SUN20IW1) && !IS_ENABLED(CONFIG_ARCH_SUN50IW12) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN55IW3) && !IS_ENABLED(CONFIG_ARCH_SUN60IW2) && !IS_ENABLED(CONFIG_ARCH_SUN8IW21) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN55IW6) && !IS_ENABLED(CONFIG_ARCH_SUN300IW1) && !IS_ENABLED(CONFIG_ARCH_SUN8IW17) \
	&& !IS_ENABLED(CONFIG_ARCH_SUN65IW1) && !IS_ENABLED(CONFIG_ARCH_SUN251IW1)

	sunxi_hci->clk_phy = devm_clk_get(&pdev->dev, "phy");
	if (IS_ERR(sunxi_hci->clk_phy)) {
		sunxi_err(&pdev->dev, "Could not get phy clock\n");
		return PTR_ERR(sunxi_hci->clk_phy);
	}
#endif


	if (sunxi_hci->hsic_flag) {
		sunxi_hci->hsic_usbphy = of_clk_get(np, 2);
		if (IS_ERR(sunxi_hci->hsic_usbphy)) {
			sunxi_hci->hsic_usbphy = NULL;
			DMSG_ERR("ERR: %s get usb hsic_usbphy failed.\n",
				sunxi_hci->hci_name);
		}

		sunxi_hci->clk_usbhsic12m = of_clk_get(np, 3);
		if (IS_ERR(sunxi_hci->clk_usbhsic12m)) {
			sunxi_hci->clk_usbhsic12m = NULL;
			DMSG_ERR("ERR: %s get usb clk_usbhsic12m failed.\n",
				sunxi_hci->hci_name);
		}

		sunxi_hci->pll_hsic = of_clk_get(np, 4);
		if (IS_ERR(sunxi_hci->pll_hsic)) {
			sunxi_hci->pll_hsic = NULL;
			DMSG_ERR("ERR: %s get usb pll_hsic failed.\n",
				sunxi_hci->hci_name);
		}
	}

	sunxi_hci->rext_cal_bypass = device_property_read_bool(&pdev->dev, "aw,rext_cal_bypass");

#if IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	sunxi_hci->usb2_generic_phy = devm_phy_get(&pdev->dev, "usb2-phy");
	if (IS_ERR(sunxi_hci->usb2_generic_phy)) {
		ret = PTR_ERR(sunxi_hci->usb2_generic_phy);
		if (ret == -ENOSYS || ret == -ENODEV)
			sunxi_hci->usb2_generic_phy = NULL;
		else
			return dev_err_probe(&pdev->dev, ret, "no usb2 phy configured\n");
	}
#endif

	/* get phy parameters from device-tree */
	ret = of_property_read_u32(pdev->dev.of_node, "phy_range", &sunxi_hci->phy_range);
	if (ret != 0)
		sunxi_hci->phy_range = -1;

	return 0;
}

static int get_usb_cfg(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *usbc_np = NULL;
	char np_name[10];
	int ret = -1;

	sprintf(np_name, "usbc%d", sunxi_get_hci_num(pdev));
	usbc_np = of_find_node_by_type(NULL, np_name);

	/* usbc enable */
	ret = of_property_read_string(usbc_np,
			"status", &sunxi_hci->used_status);
	if (ret) {
		DMSG_ERR("get %s used is fail, %d\n",
			sunxi_hci->hci_name, -ret);
		sunxi_hci->used = 0;
	} else if (!strcmp(sunxi_hci->used_status, "okay")) {
		sunxi_hci->used = 1;
	} else {
		 sunxi_hci->used = 0;
	}

	/* usbc port type */
	if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
		ret = of_property_read_u32(usbc_np,
						KEY_USB_PORT_TYPE,
						&sunxi_hci->port_type);
		if (ret)
			DMSG_INFO("get usb_port_type is fail, %d\n", -ret);
	}

	sunxi_hci->hsic_flag = 0;

	if (sunxi_hci->usbc_no == HCI1_USBC_NO) {
		ret = of_property_read_u32(usbc_np,
				KEY_USB_HSIC_USBED, &sunxi_hci->hsic_flag);
		if (ret)
			sunxi_hci->hsic_flag = 0;

		if (sunxi_hci->hsic_flag) {
			if (!strncmp(sunxi_hci->hci_name,
					"ohci", strlen("ohci"))) {
				DMSG_ERR("HSIC is no susport in %s, and to return\n",
					sunxi_hci->hci_name);
				sunxi_hci->used = 0;
				return 0;
			}

			/* hsic regulator_io */
			ret = of_property_read_string(usbc_np,
					KEY_USB_HSIC_REGULATOR_IO,
					&sunxi_hci->hsic_regulator_io);
			if (ret) {
				DMSG_ERR("get %s, hsic_regulator_io is fail, %d\n",
					sunxi_hci->hci_name, -ret);
				sunxi_hci->hsic_regulator_io = NULL;
			} else {
				if (!strcmp(sunxi_hci->hsic_regulator_io, "nocare")) {
					DMSG_ERR("get %s, hsic_regulator_io is no nocare\n",
						sunxi_hci->hci_name);
					sunxi_hci->hsic_regulator_io = NULL;
				}
			}

			/* Marvell 4G HSIC ctrl */
			ret = of_property_read_u32(usbc_np,
					KEY_USB_HSIC_CTRL,
					&sunxi_hci->hsic_ctrl_flag);
			if (ret) {
				DMSG_ERR("get %s usb_hsic_ctrl is fail, %d\n",
					sunxi_hci->hci_name, -ret);
				sunxi_hci->hsic_ctrl_flag = 0;
			}
			if (sunxi_hci->hsic_ctrl_flag) {
				sunxi_hci->usb_host_hsic_rdy.gpio =
						of_get_named_gpio(usbc_np,
							KEY_USB_HSIC_RDY_GPIO, 0);
				if (gpio_is_valid(sunxi_hci->usb_host_hsic_rdy.gpio)) {
					sunxi_hci->usb_host_hsic_rdy_valid = 1;
				} else {
					sunxi_hci->usb_host_hsic_rdy_valid = 0;
					DMSG_ERR("get %s drv_vbus_gpio is fail\n",
						sunxi_hci->hci_name);
				}
			} else {
				sunxi_hci->usb_host_hsic_rdy_valid = 0;
			}

			/* SMSC usb3503 HSIC HUB ctrl */
			ret = of_property_read_u32(usbc_np,
					"usb_hsic_usb3503_flag",
					&sunxi_hci->usb_hsic_usb3503_flag);
			if (ret) {
				DMSG_ERR("get %s usb_hsic_usb3503_flag is fail, %d\n",
					sunxi_hci->hci_name, -ret);
				sunxi_hci->usb_hsic_usb3503_flag = 0;
			}


			if (sunxi_hci->usb_hsic_usb3503_flag) {
				sunxi_hci->usb_hsic_hub_connect.gpio =
						of_get_named_gpio(usbc_np,
							"usb_hsic_hub_connect_gpio", 0);
				if (gpio_is_valid(sunxi_hci->usb_hsic_hub_connect.gpio)) {
					sunxi_hci->usb_hsic_hub_connect_valid = 1;
				} else {
					sunxi_hci->usb_hsic_hub_connect_valid = 0;
					DMSG_ERR("get %s usb_hsic_hub_connect is fail\n",
						sunxi_hci->hci_name);
				}


				sunxi_hci->usb_hsic_int_n.gpio =
						of_get_named_gpio(usbc_np,
							"usb_hsic_int_n_gpio", 0);
				if (gpio_is_valid(sunxi_hci->usb_hsic_int_n.gpio)) {
					sunxi_hci->usb_hsic_int_n_valid = 1;
				} else {
					sunxi_hci->usb_hsic_int_n_valid = 0;
					DMSG_ERR("get %s usb_hsic_int_n is fail\n",
						sunxi_hci->hci_name);
				}


				sunxi_hci->usb_hsic_reset_n.gpio =
						of_get_named_gpio(usbc_np,
							"usb_hsic_reset_n_gpio", 0);
				if (gpio_is_valid(sunxi_hci->usb_hsic_reset_n.gpio)) {
					sunxi_hci->usb_hsic_reset_n_valid = 1;
				} else {
					sunxi_hci->usb_hsic_reset_n_valid = 0;
					DMSG_ERR("get %s usb_hsic_reset_n is fail\n",
						sunxi_hci->hci_name);
				}

			} else {
				sunxi_hci->usb_hsic_hub_connect_valid = 0;
				sunxi_hci->usb_hsic_int_n_valid = 0;
				sunxi_hci->usb_hsic_reset_n_valid = 0;
			}

		} else {
			sunxi_hci->hsic_ctrl_flag = 0;
			sunxi_hci->usb_host_hsic_rdy_valid = 0;
			sunxi_hci->usb_hsic_hub_connect_valid = 0;
			sunxi_hci->usb_hsic_int_n_valid = 0;
			sunxi_hci->usb_hsic_reset_n_valid = 0;
		}
	}

	/* usbc wakeup_suspend */
	ret = of_property_read_u32(usbc_np,
			KEY_USB_WAKEUP_SUSPEND,
			&sunxi_hci->wakeup_suspend);
	if (ret) {
		DMSG_ERR("get %s wakeup_suspend is fail, %d\n",
			sunxi_hci->hci_name, -ret);
	}

	/* usbc drvvbus-en */
	ret = of_property_read_string(usbc_np,
			KEY_USB_DRVVBUS_EN_GPIO,
			&sunxi_hci->drvvbus_en_name);
	if (ret) {
		DMSG_DEBUG("get drvvbus-en is fail, %d\n", -ret);
		sunxi_hci->drvvbus_en_gpio_valid = 0;
	} else {
		/* get drvvbus-en gpio */
		sunxi_hci->drvvbus_en_gpio_set.gpio =
			of_get_named_gpio(usbc_np, KEY_USB_DRVVBUS_EN_GPIO, 0);
		if (gpio_is_valid(sunxi_hci->drvvbus_en_gpio_set.gpio)) {
			sunxi_hci->drvvbus_en_gpio_valid = 1;
			sunxi_hci->drvvbus_en_type = USB_DRVVBUS_EN_TYPE_GPIO;
		} else {
			sunxi_hci->drvvbus_en_gpio_valid = 0;
		}
	}

	/* usbc gma340-oe */
	ret = of_property_read_string(usbc_np,
			KEY_USB_GMA340_OE_GPIO,
			&sunxi_hci->gma340_oe_name);
	if (ret) {
		DMSG_DEBUG("get gma340-oe is fail, %d\n", -ret);
		sunxi_hci->gma340_oe_gpio_valid = 0;
	} else {
		/* get gma340-oe gpio */
		sunxi_hci->gma340_oe_gpio_set.gpio =
			of_get_named_gpio(usbc_np, KEY_USB_GMA340_OE_GPIO, 0);
		if (gpio_is_valid(sunxi_hci->gma340_oe_gpio_set.gpio)) {
			sunxi_hci->gma340_oe_gpio_valid = 1;
		} else {
			sunxi_hci->gma340_oe_gpio_valid = 0;
		}
	}

	/* wakeup-source */
	if (of_property_read_bool(usbc_np, KEY_WAKEUP_SOURCE)) {
		sunxi_hci->wakeup_source_flag = 1;
	} else {
		DMSG_DEBUG("get %s wakeup-source is fail.\n", sunxi_hci->hci_name);
		sunxi_hci->wakeup_source_flag = 0;
	}

	/* extcon supported */
	sunxi_hci->extcon_supported = of_property_read_bool(usbc_np, "extcon");

	return 0;
}

int sunxi_get_hci_num(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int hci_num = 0;

	ret = of_property_read_u32(np, HCI_USBC_NO, &hci_num);
	if (ret)
		DMSG_ERR("get hci_ctrl_num is fail, %d\n", -ret);

	return hci_num;
}

static int sunxi_get_hci_name(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;

	sprintf(sunxi_hci->hci_name, "%s", np->name);

	return 0;
}

static int sunxi_get_hci_irq_no(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	sunxi_hci->irq_no = platform_get_irq(pdev, 0);

	return 0;
}

static int hci_wakeup_source_init(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	if (sunxi_hci->wakeup_source_flag && sunxi_hci->wakeup_suspend) {
		device_init_wakeup(&pdev->dev, true);
		dev_pm_set_wake_irq(&pdev->dev, sunxi_hci->irq_no);
	} else {
		DMSG_INFO("sunxi %s don't init wakeup source\n", sunxi_hci->hci_name);
	}

	return 0;
}

/*
 * In a specified platform, if only ohci is enabled, you need to add
 * specified actions during suspend to reduce power consumption, so you
 * need to add a check to see if ehci is enabled. If it is enabled, you
 * do not need to add specified actions.
 */
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
void sunxi_hci_force_suspend(struct sunxi_hci_hcd *sunxi_hci, bool enter_standby)
{
	int reg_value = 0;

	if (!atomic_read(&ehci_enable_flag[sunxi_hci->usbc_no]) && enter_standby) {
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3);

		reg_value |= 0x01 << SUNXI_HCI_CTRL_3_FORCE_SUSPEND;

		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3));
	}

	if (!atomic_read(&ehci_enable_flag[sunxi_hci->usbc_no]) && !enter_standby) {
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3);

		reg_value &= ~(0x01 << SUNXI_HCI_CTRL_3_FORCE_SUSPEND);

		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_CTRL_3));
	}
}

bool sunxi_hci_ehci_enabled(struct sunxi_hci_hcd *sunxi_hci)
{
	if (!atomic_read(&ehci_enable_flag[sunxi_hci->usbc_no]))
		return false;
	return true;
}
#else
void sunxi_hci_force_suspend(struct sunxi_hci_hcd *sunxi_hci, bool enter_standby) {}
bool sunxi_hci_ehci_enabled(struct sunxi_hci_hcd *sunxi_hci) { return true; }
#endif
EXPORT_SYMBOL(ehci_enable_flag);
EXPORT_SYMBOL(sunxi_hci_force_suspend);
EXPORT_SYMBOL(sunxi_hci_ehci_enabled);

void enter_usb_standby(struct sunxi_hci_hcd *sunxi_hci)
{
	mutex_lock(&usb_standby_lock);
	if (!atomic_read(&usb_standby_cnt[sunxi_hci->usbc_no])) {
		atomic_add(1, &usb_standby_cnt[sunxi_hci->usbc_no]);
	} else {
		/* phy after ehci and ohci suspend. */
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
			/* usb1 0x800 bit[2], enable rc16M */
			sunxi_hci_common_set_rc_clk(sunxi_hci, 1);
			/* usb1 0x800 bit[31], switch 16M */
			sunxi_hci_common_switch_clk(sunxi_hci, 1);
			/* usb1 0x800 bit[3], open 16M gating */
			sunxi_hci_common_set_rcgating(sunxi_hci, 1);
		}
#endif
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
#if IS_ENABLED(SUNXI_USB_STANDBY_NEW_MODE)
#if !IS_ENABLED(CONFIG_ARCH_SUN8IW21)
		/* phy reg, offset:0x800 bit2, set 1, enable RC16M CLK */
		sunxi_hci_set_rc_clk(sunxi_hci, 1);
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
		/* enable standby irq */
		sunxi_hci_set_standby_irq(sunxi_hci, 1);
#endif
		/* phy reg, offset:0x808 bit3, set 1, remote enable */
		sunxi_hci_set_wakeup_ctrl(sunxi_hci, 1);
#if !IS_ENABLED(SUNXI_USB_STANDBY_SIDDQ_BYPASS)
		/* phy reg, offset:0x810 bit3 set 1, clean siddq */
		sunxi_hci_set_siddq(sunxi_hci, 1);
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
		sunxi_hci_set_vc_cfg(sunxi_hci, 1);
		sunxi_hci_set_clk(sunxi_hci, 1);
#endif
#endif
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		/* phy reg, offset:0x0 bit31, set 1 */
		sunxi_hci_switch_clk(sunxi_hci, 1);
		/* phy reg, offset:0x0 bit3, set 1 */
		sunxi_hci_set_rcgating(sunxi_hci, 1);
#endif
		atomic_sub(1, &usb_standby_cnt[sunxi_hci->usbc_no]);
	}
	mutex_unlock(&usb_standby_lock);
}
EXPORT_SYMBOL(enter_usb_standby);

void exit_usb_standby(struct sunxi_hci_hcd *sunxi_hci)
{
	mutex_lock(&usb_standby_lock);
	if (atomic_read(&usb_standby_cnt[sunxi_hci->usbc_no])) {
		atomic_sub(1, &usb_standby_cnt[sunxi_hci->usbc_no]);
	} else {
		/* phy before ehci and ohci */
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
			/* usb1 0x800 bit[3] */
			sunxi_hci_common_set_rcgating(sunxi_hci, 0);
			/* usb1 0x800 bit[31] */
			sunxi_hci_common_switch_clk(sunxi_hci, 0);
			/* usb1 0x800 bit[2] */
			sunxi_hci_common_set_rc_clk(sunxi_hci, 0);
		}
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		/* phy reg, offset:0x0 bit3, set 0 */
		sunxi_hci_set_rcgating(sunxi_hci, 0);
		/* phy reg, offset:0x0 bit31, set 0 */
		sunxi_hci_switch_clk(sunxi_hci, 0);
#endif
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
		sunxi_hci_set_clk(sunxi_hci, 0);
		sunxi_hci_set_vc_cfg(sunxi_hci, 0);
#endif
#if !IS_ENABLED(SUNXI_USB_STANDBY_SIDDQ_BYPASS)
		/* phy reg, offset:0x10 bit3 set 0, enable siddq */
		sunxi_hci_set_siddq(sunxi_hci, 0);
#endif
		/* phy reg, offset:0x08 bit3, set 0, remote disable */
		sunxi_hci_set_wakeup_ctrl(sunxi_hci, 0);
#if IS_ENABLED(SUNXI_USB_STANDBY_NEW_MODE)
#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
		/* clear standby irq status */
		sunxi_hci_clean_standby_irq(sunxi_hci);
		/* disable standby irq */
		sunxi_hci_set_standby_irq(sunxi_hci, 0);
#endif
#endif
#if !IS_ENABLED(CONFIG_ARCH_SUN8IW21)
		/* phy reg, offset:0x0 bit2, set 0, disable rc clk */
		sunxi_hci_set_rc_clk(sunxi_hci, 0);
#endif
#endif
		atomic_add(1, &usb_standby_cnt[sunxi_hci->usbc_no]);
	}
	mutex_unlock(&usb_standby_lock);
}
EXPORT_SYMBOL(exit_usb_standby);

static int sunxi_get_hci_resource(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci, int usbc_no)
{
	if (sunxi_hci == NULL) {
		sunxi_err(&pdev->dev, "sunxi_hci is NULL\n");
		return -1;
	}

	memset(sunxi_hci, 0, sizeof(struct sunxi_hci_hcd));

	sunxi_hci->usbc_no = usbc_no;
	sunxi_get_hci_name(pdev, sunxi_hci);
	get_usb_cfg(pdev, sunxi_hci);

	if (sunxi_hci->used == 0) {
		DMSG_INFO("sunxi %s is no enable\n", sunxi_hci->hci_name);
		return -1;
	}

	sunxi_get_hci_base(pdev, sunxi_hci);
	sunxi_get_hci_clock(pdev, sunxi_hci);
	sunxi_get_hci_irq_no(pdev, sunxi_hci);
	hci_wakeup_source_init(pdev, sunxi_hci);

	request_usb_regulator_io(sunxi_hci);
	sunxi_hci->open_clock	= open_clock;
	sunxi_hci->close_clock	= close_clock;
	sunxi_hci->set_power	= sunxi_set_vbus;
	sunxi_hci->usb_passby	= usb_passby;

	alloc_pin(sunxi_hci);

	pdev->dev.platform_data = sunxi_hci;
	return 0;
}

static int sunxi_hci_dump_reg_all(int usbc_type)
{
	struct sunxi_hci_hcd *sunxi_hci, *sunxi_ehci, *sunxi_ohci;
	struct resource res, res1;
	int i;

	switch (usbc_type) {
	case SUNXI_USB_EHCI:
		for (i = 0; i < 4; i++) {
			switch (i) {
			case HCI0_USBC_NO:
				sunxi_hci = &sunxi_ehci0;
				break;
			case HCI1_USBC_NO:
				sunxi_hci = &sunxi_ehci1;
				break;
			case HCI2_USBC_NO:
				sunxi_hci = &sunxi_ehci2;
				break;
			case HCI3_USBC_NO:
				sunxi_hci = &sunxi_ehci3;
				break;
			}

			if (sunxi_hci->used) {
				of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
				DMSG_INFO("usbc%d, ehci, base[0x%08x]:\n",
					  i, (unsigned int)res.start);
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       (void *)sunxi_hci->ehci_base, 0x58, false);
				DMSG_INFO("\n");
			}
		}

		return 0;

	case SUNXI_USB_OHCI:
		for (i = 0; i < 4; i++) {
			switch (i) {
			case HCI0_USBC_NO:
				sunxi_hci = &sunxi_ohci0;
				break;
			case HCI1_USBC_NO:
				sunxi_hci = &sunxi_ohci1;
				break;
			case HCI2_USBC_NO:
				sunxi_hci = &sunxi_ohci2;
				break;
			case HCI3_USBC_NO:
				sunxi_hci = &sunxi_ohci3;
				break;
			}

			if (sunxi_hci->used) {
				of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
				DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
					  i, (unsigned int)(res.start));
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       (void *)sunxi_hci->ohci_base, 0x58, false);
				DMSG_INFO("\n");
			}
		}

		return 0;

	case SUNXI_USB_PHY:
		for (i = 0; i < 4; i++) {
			switch (i) {
			case HCI0_USBC_NO:
				sunxi_hci = &sunxi_ehci0;
				break;
			case HCI1_USBC_NO:
				sunxi_hci = &sunxi_ehci1;
				break;
			case HCI2_USBC_NO:
				sunxi_hci = &sunxi_ehci2;
				break;
			case HCI3_USBC_NO:
				sunxi_hci = &sunxi_ehci3;
				break;
			}

			if (sunxi_hci->used) {
				of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
				DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
					  i, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       (void *)(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE), 0x34, false);
				DMSG_INFO("\n");
			}
		}

		return 0;

	case SUNXI_USB_ALL:
		for (i = 0; i < 4; i++) {
			switch (i) {
			case HCI0_USBC_NO:
				sunxi_ehci = &sunxi_ehci0;
				sunxi_ohci = &sunxi_ohci0;
				break;
			case HCI1_USBC_NO:
				sunxi_ehci = &sunxi_ehci1;
				sunxi_ohci = &sunxi_ohci1;
				break;
			case HCI2_USBC_NO:
				sunxi_ehci = &sunxi_ehci2;
				sunxi_ohci = &sunxi_ohci2;
				break;
			case HCI3_USBC_NO:
				sunxi_ehci = &sunxi_ehci3;
				sunxi_ohci = &sunxi_ohci3;
				break;
			}

			if (sunxi_ehci->used) {
				of_address_to_resource(sunxi_ehci->pdev->dev.of_node, 0, &res);
				of_address_to_resource(sunxi_ohci->pdev->dev.of_node, 0, &res1);
				DMSG_INFO("usbc%d, ehci, base[0x%08x]:\n",
					  i, (unsigned int)res.start);
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       (void *)sunxi_ehci->ehci_base, 0x58, false);
				DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
					  i, (unsigned int)(res1.start));
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       (void *)sunxi_ohci->ohci_base, 0x58, false);
				DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
					  i, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       (void *)(sunxi_ehci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE), 0x34, false);
				DMSG_INFO("\n");
			}
		}

		return 0;

	default:
		DMSG_ERR("%s()%d invalid argument\n", __func__, __LINE__);
		return -EINVAL;
	}
}

static int sunxi_hci_dump_reg(int usbc_no, int usbc_type)
{
	struct sunxi_hci_hcd *sunxi_hci, *sunxi_ehci, *sunxi_ohci;
	struct resource res, res1;

	switch (usbc_type) {
	case SUNXI_USB_EHCI:
		switch (usbc_no) {
		case HCI0_USBC_NO:
			sunxi_hci = &sunxi_ehci0;
			break;
		case HCI1_USBC_NO:
			sunxi_hci = &sunxi_ehci1;
			break;
		case HCI2_USBC_NO:
			sunxi_hci = &sunxi_ehci2;
			break;
		case HCI3_USBC_NO:
			sunxi_hci = &sunxi_ehci3;
			break;
		default:
			DMSG_ERR("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_hci->used) {
			of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
			DMSG_INFO("usbc%d, ehci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)res.start);
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       (void *)sunxi_hci->ehci_base, 0x58, false);
			DMSG_INFO("\n");
		} else
			DMSG_ERR("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

		return 0;

	case SUNXI_USB_OHCI:
		switch (usbc_no) {
		case HCI0_USBC_NO:
			sunxi_hci = &sunxi_ohci0;
			break;
		case HCI1_USBC_NO:
			sunxi_hci = &sunxi_ohci1;
			break;
		case HCI2_USBC_NO:
			sunxi_hci = &sunxi_ohci2;
			break;
		case HCI3_USBC_NO:
			sunxi_hci = &sunxi_ohci3;
			break;
		default:
			DMSG_ERR("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_hci->used) {
			of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
			DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res.start));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       (void *)sunxi_hci->ohci_base, 0x58, false);
			DMSG_INFO("\n");
		} else
			DMSG_ERR("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

		return 0;

	case SUNXI_USB_PHY:
		switch (usbc_no) {
		case HCI0_USBC_NO:
			sunxi_hci = &sunxi_ehci0;
			break;
		case HCI1_USBC_NO:
			sunxi_hci = &sunxi_ehci1;
			break;
		case HCI2_USBC_NO:
			sunxi_hci = &sunxi_ehci2;
			break;
		case HCI3_USBC_NO:
			sunxi_hci = &sunxi_ehci3;
			break;
		default:
			DMSG_ERR("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_hci->used) {
			of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
			DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       (void *)sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE, 0x34, false);
			DMSG_INFO("\n");
		} else
			DMSG_ERR("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

		return 0;

	case SUNXI_USB_ALL:
		switch (usbc_no) {
		case HCI0_USBC_NO:
			sunxi_ehci = &sunxi_ehci0;
			sunxi_ohci = &sunxi_ohci0;
			break;
		case HCI1_USBC_NO:
			sunxi_ehci = &sunxi_ehci1;
			sunxi_ohci = &sunxi_ohci1;
			break;
		case HCI2_USBC_NO:
			sunxi_ehci = &sunxi_ehci2;
			sunxi_ohci = &sunxi_ohci2;
			break;
		case HCI3_USBC_NO:
			sunxi_ehci = &sunxi_ehci3;
			sunxi_ohci = &sunxi_ohci3;
			break;
		default:
			DMSG_ERR("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_ehci->used) {
			of_address_to_resource(sunxi_ehci->pdev->dev.of_node, 0, &res);
			of_address_to_resource(sunxi_ohci->pdev->dev.of_node, 0, &res1);
			DMSG_INFO("usbc%d, ehci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)res.start);
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       (void *)sunxi_ehci->ehci_base, 0x58, false);
			DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res1.start));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       (void *)sunxi_ohci->ohci_base, 0x58, false);
			DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       (void *)(sunxi_ehci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE), 0x34, false);
			DMSG_INFO("\n");
		} else
			DMSG_ERR("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

		return 0;

	default:
		DMSG_ERR("%s()%d invalid argument\n", __func__, __LINE__);
		return -EINVAL;
	}
}

int sunxi_hci_standby_completion(int usbc_type)
{
	if (sunxi_ehci0.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci0.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci0.standby_complete);
		} else {
			DMSG_ERR("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	if (sunxi_ehci1.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci1.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci1.standby_complete);
		} else {
			DMSG_ERR("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	if (sunxi_ehci2.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci2.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci2.standby_complete);
		} else {
			DMSG_ERR("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	if (sunxi_ehci3.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci3.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci3.standby_complete);
		} else {
			DMSG_ERR("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_hci_standby_completion);

int exit_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci)
{
	release_usb_regulator_io(sunxi_hci);
	free_pin(sunxi_hci);
	if (sunxi_hci->usb_base_res)
		kfree(sunxi_hci->usb_base_res);
	return 0;
}
EXPORT_SYMBOL(exit_sunxi_hci);

int init_sunxi_hci(struct platform_device *pdev, int usbc_type)
{
	struct sunxi_hci_hcd *sunxi_hci = NULL;
	int usbc_no = 0;
	int hci_num = -1;
	int ret = -1;

#if IS_ENABLED(CONFIG_OF)
	pdev->dev.dma_mask = &sunxi_hci_dmamask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
#endif

	hci_num = sunxi_get_hci_num(pdev);

	if (usbc_type == SUNXI_USB_XHCI) {
		usbc_no = hci_num;
		sunxi_hci = &sunxi_xhci;
	} else {
		switch (hci_num) {
		case HCI0_USBC_NO:
			usbc_no = HCI0_USBC_NO;
			if (usbc_type == SUNXI_USB_EHCI) {
				sunxi_hci = &sunxi_ehci0;
			} else if (usbc_type == SUNXI_USB_OHCI) {
				sunxi_hci = &sunxi_ohci0;
			} else {
				sunxi_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
			break;

		case HCI1_USBC_NO:
			usbc_no = HCI1_USBC_NO;
			if (usbc_type == SUNXI_USB_EHCI) {
				sunxi_hci = &sunxi_ehci1;
			} else if (usbc_type == SUNXI_USB_OHCI) {
				sunxi_hci = &sunxi_ohci1;
			} else {
				sunxi_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
			break;

		case HCI2_USBC_NO:
			usbc_no = HCI2_USBC_NO;
			if (usbc_type == SUNXI_USB_EHCI) {
				sunxi_hci = &sunxi_ehci2;
			} else if (usbc_type == SUNXI_USB_OHCI) {
				sunxi_hci = &sunxi_ohci2;
			} else {
				sunxi_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
			break;

		case HCI3_USBC_NO:
			usbc_no = HCI3_USBC_NO;
			if (usbc_type == SUNXI_USB_EHCI) {
				sunxi_hci = &sunxi_ehci3;
			} else if (usbc_type == SUNXI_USB_OHCI) {
				sunxi_hci = &sunxi_ohci3;
			} else {
				sunxi_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
			break;

		default:
			sunxi_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
			return -1;
		}
	}

	ret = sunxi_get_hci_resource(pdev, sunxi_hci, usbc_no);
	if (ret != 0)
		return ret;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW17)
	if (usbc_type == SUNXI_USB_OHCI)
		ret = sunxi_get_ohci_clock_src(pdev, sunxi_hci);
#endif

	/* Initialize the atomic array */
	atomic_set(&usb_standby_cnt[usbc_no], 0);

	/*
	 * Initialize the atomic array,
	 * And it can only be initialized once!!!
	 */
	if (!init_flag) {
		atomic_set(&ehci_enable_flag[usbc_no], 0);
		init_flag = true;
	}

	return ret;
}
EXPORT_SYMBOL(init_sunxi_hci);

static int __parse_hci_str(const char *buf, size_t size)
{
	int ret = 0;
	unsigned long val;

	if (!buf) {
		sunxi_err(NULL, "%s()%d invalid argument\n", __func__, __LINE__);
		return -1;
	}

	ret = kstrtoul(buf, 10, &val);
	if (ret) {
		sunxi_err(NULL, "%s()%d failed to transfer\n", __func__, __LINE__);
		return -1;
	}

	return val;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
ehci_reg_show(const struct class *class, const struct class_attribute *attr, char *buf)
#else
ehci_reg_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	sunxi_hci_dump_reg_all(SUNXI_USB_EHCI);

	return 0;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
ehci_reg_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#else
ehci_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	int usbc_no;

	usbc_no = __parse_hci_str(buf, count);
	if (usbc_no < 0)
		return -EINVAL;

	sunxi_hci_dump_reg(usbc_no, SUNXI_USB_EHCI);

	return count;
}
static CLASS_ATTR_RW(ehci_reg);

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
ohci_reg_show(const struct class *class, const struct class_attribute *attr, char *buf)
#else
ohci_reg_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	sunxi_hci_dump_reg_all(SUNXI_USB_OHCI);

	return 0;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
ohci_reg_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#else
ohci_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	int usbc_no;

	usbc_no = __parse_hci_str(buf, count);
	if (usbc_no < 0)
		return -EINVAL;

	sunxi_hci_dump_reg(usbc_no, SUNXI_USB_OHCI);

	return count;
}
static CLASS_ATTR_RW(ohci_reg);

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
phy_reg_show(const struct class *class, const struct class_attribute *attr, char *buf)
#else
phy_reg_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	sunxi_hci_dump_reg_all(SUNXI_USB_PHY);

	return 0;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
phy_reg_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#else
phy_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	int usbc_no;

	usbc_no = __parse_hci_str(buf, count);
	if (usbc_no < 0)
		return -EINVAL;

	sunxi_hci_dump_reg(usbc_no, SUNXI_USB_PHY);

	return count;
}
static CLASS_ATTR_RW(phy_reg);

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
all_reg_show(const struct class *class, const struct class_attribute *attr, char *buf)
#else
all_reg_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	sunxi_hci_dump_reg_all(SUNXI_USB_ALL);

	return 0;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
all_reg_store(const struct class *class, const struct class_attribute *attr,
		const char *buf, size_t count)
#else
all_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
#endif
{
	int usbc_no;


	usbc_no = __parse_hci_str(buf, count);
	if (usbc_no < 0)
		return -EINVAL;

	sunxi_hci_dump_reg(usbc_no, SUNXI_USB_ALL);

	return count;
}
static CLASS_ATTR_RW(all_reg);

static struct attribute *hci_class_attrs[] = {
	&class_attr_ehci_reg.attr,
	&class_attr_ohci_reg.attr,
	&class_attr_phy_reg.attr,
	&class_attr_all_reg.attr,
	NULL,
};
ATTRIBUTE_GROUPS(hci_class);

static struct class hci_class = {
	.name		= "hci_reg",
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0))
	.owner		= THIS_MODULE,
#endif
	.class_groups	= hci_class_groups,
};

static int __init init_sunxi_hci_class(void)
{
	int ret = 0;

	ret = class_register(&hci_class);
	if (ret) {
		DMSG_ERR("%s()%d register class fialed\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static void __exit exit_sunxi_hci_class(void)
{
	class_unregister(&hci_class);
}

late_initcall(init_sunxi_hci_class);
module_exit(exit_sunxi_hci_class);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" SUNXI_HCI_NAME);
MODULE_AUTHOR("javen");
MODULE_VERSION("1.1.4");

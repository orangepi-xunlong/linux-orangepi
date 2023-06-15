/**
 * drivers/usb/host/sunxi-hci.c
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

#if IS_ENABLED(CONFIG_USB_SUNXI_USB_MANAGER)
int usb_otg_id_status(void);
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

atomic_t usb1_enable_passly_cnt = ATOMIC_INIT(0);
atomic_t usb2_enable_passly_cnt = ATOMIC_INIT(0);
atomic_t usb3_enable_passly_cnt = ATOMIC_INIT(0);
atomic_t usb4_enable_passly_cnt = ATOMIC_INIT(0);

atomic_t usb_standby_cnt = ATOMIC_INIT(0);
static s32 request_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{
	if (sunxi_hci->hsic_flag) {
		if (sunxi_hci->hsic_regulator_io != NULL) {
			sunxi_hci->hsic_regulator_io_hdle =
					regulator_get(NULL, sunxi_hci->hsic_regulator_io);
			if (IS_ERR(sunxi_hci->hsic_regulator_io_hdle)) {
				DMSG_PANIC("ERR: some error happen, %s, hsic_regulator_io_hdle fail to get regulator!", sunxi_hci->hci_name);
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

void __iomem *usb_phy_csr_add(struct sunxi_hci_hcd *sunxi_hci)
{
	return (sunxi_hci->otg_vbase + SUNXI_OTG_PHY_CTRL);
}

void __iomem *usb_phy_csr_read(struct sunxi_hci_hcd *sunxi_hci)
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
		DMSG_PANIC("usb_phy_csr_read is failed in %d index\n",
			sunxi_hci->usbc_no);
		break;
	}

	return NULL;
}

void __iomem *usb_phy_csr_write(struct sunxi_hci_hcd *sunxi_hci)
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
		DMSG_PANIC("usb_phy_csr_write is failed in %d index\n",
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
		DMSG_PANIC("%s,otg_vbase is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_add(sunxi_hci) == NULL) {
		DMSG_PANIC("%s,phy_csr_add is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_write(sunxi_hci) == NULL) {

		DMSG_PANIC("%s,phy_csr_write is null\n", __func__);
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

	/*TXVREFTUNE + TXRISETUNE + TXPREEMPAMPTUNE + TXRESTUNE*/
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
		DMSG_PANIC("%s,otg_vbase is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_add(sunxi_hci) == NULL) {
		DMSG_PANIC("%s,phy_csr_add is null\n", __func__);
		return -1;
	}

	if (usb_phy_csr_read(sunxi_hci) == NULL) {
		DMSG_PANIC("%s,phy_csr_read is null\n", __func__);
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

#if defined(CONFIG_ARCH_SUN8IW7) || defined(CONFIG_ARCH_SUN8IW12) \
	|| defined(CONFIG_ARCH_SUN8IW15) || defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN50IW6) || defined(CONFIG_ARCH_SUN50IW9) \
	|| defined(CONFIG_ARCH_SUN8IW18)
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

static void USBC_Clean_SIDDP(struct sunxi_hci_hcd *sunxi_hci)
{
	int reg_value = 0;

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL);
	reg_value &= ~(0x01 << SUNXI_HCI_PHY_CTRL_SIDDQ);
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_HCI_PHY_CTRL));
}

#if defined(CONFIG_ARCH_SUN50IW10)
/*for common circuit*/
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
void sunxi_hci_switch_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int val = 0;
	val = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
	if (is_on)
		val |= 0x01 << 31;
	else
		val &= ~(0x01 << 31);
	USBC_Writel(val, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));
}
void sunxi_hci_set_rcgating(struct sunxi_hci_hcd *sunxi_hci, int is_on)
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

#if defined(CONFIG_ARCH_SUN50IW9)
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

static int open_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	int ret;

	mutex_lock(&usb_clock_lock);

	/*
	 * otg and hci share the same phy in fpga,
	 * so need switch phy to hci here.
	 * Notice: not need any more on new platforms.
	 */

	/* otg and hci0 Controller Shared phy in SUN50I */
	if (sunxi_hci->usbc_no == HCI0_USBC_NO)
		USBC_SelectPhyToHci(sunxi_hci);

	/* To fix hardware design issue. */
#if defined(CONFIG_ARCH_SUN8IW12) || defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN50IW6)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_RES_TUNE, SUNXI_TX_RES_TUNE_OFFSET, 0x3);
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_VREF_TUNE, SUNXI_TX_VREF_TUNE_OFFSET, 0xc);
#endif

#if defined(CONFIG_ARCH_SUN8IW18)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_PREEMPAMP_TUNE, SUNXI_TX_PREEMPAMP_TUNE_OFFSET, 0x10);
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_VREF_TUNE, SUNXI_TX_VREF_TUNE_OFFSET, 0xc);
#endif

#if defined(CONFIG_ARCH_SUN8IW7) || defined(CONFIG_ARCH_SUN8IW15)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_VREF_TUNE, SUNXI_TX_VREF_TUNE_OFFSET, 0x4);
#endif

#if defined(CONFIG_ARCH_SUN50IW9)
	usb_hci_utmi_phy_tune(sunxi_hci, SUNXI_TX_PREEMPAMP_TUNE, SUNXI_TX_PREEMPAMP_TUNE_OFFSET, 0x2);
#endif

	if (!sunxi_hci->clk_is_open) {
		sunxi_hci->clk_is_open = 1;

		if (sunxi_hci->reset_phy) {
			ret = reset_control_deassert(sunxi_hci->reset_phy);
			if (ret) {
				dev_err(&sunxi_hci->pdev->dev, "reset phy err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_hci->reset_hci) {
			ret = reset_control_deassert(sunxi_hci->reset_hci);
			if (ret) {
				dev_err(&sunxi_hci->pdev->dev, "reset hci err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_hci->clk_bus_hci) {
			ret = clk_prepare_enable(sunxi_hci->clk_bus_hci);
			if (ret) {
				dev_err(&sunxi_hci->pdev->dev, "enable clk_bus_hci err, return %d\n", ret);
				return ret;
			}
		}

		if (sunxi_hci->clk_ohci) {
			if (strstr(sunxi_hci->hci_name, "ohci")) {
				ret = clk_prepare_enable(sunxi_hci->clk_ohci);
				if (ret) {
					dev_err(&sunxi_hci->pdev->dev,
						"enable clk_ohci err, return %d\n", ret);
					return ret;
				}
			}
		}


		udelay(10);

		if (sunxi_hci->hsic_flag) {
			printk("%s()%d yhs\n", __func__, __LINE__);
			if (sunxi_hci->hsic_ctrl_flag) {
				if (sunxi_hci->hsic_enable_flag) {
					if (clk_prepare_enable(sunxi_hci->pll_hsic))
						DMSG_PANIC("ERR:try to prepare_enable %s pll_hsic failed!\n",
							sunxi_hci->hci_name);

					if (clk_prepare_enable(sunxi_hci->clk_usbhsic12m))
						DMSG_PANIC("ERR:try to prepare_enable %s clk_usbhsic12m failed!\n",
							sunxi_hci->hci_name);

					if (clk_prepare_enable(sunxi_hci->hsic_usbphy))
						DMSG_PANIC("ERR:try to prepare_enable %s_hsic_usbphy failed!\n",
							sunxi_hci->hci_name);
				}
			} else {
				if (clk_prepare_enable(sunxi_hci->pll_hsic))
					DMSG_PANIC("ERR:try to prepare_enable %s pll_hsic failed!\n",
						sunxi_hci->hci_name);

				if (clk_prepare_enable(sunxi_hci->clk_usbhsic12m))
					DMSG_PANIC("ERR:try to prepare_enable %s clk_usbhsic12m failed!\n",
						sunxi_hci->hci_name);

				if (clk_prepare_enable(sunxi_hci->hsic_usbphy))
					DMSG_PANIC("ERR:try to prepare_enable %s_hsic_usbphy failed!\n",
						sunxi_hci->hci_name);
			}
		} else {
			if (sunxi_hci->clk_phy) {
				ret = clk_prepare_enable(sunxi_hci->clk_phy);
				if (ret) {
					dev_err(&sunxi_hci->pdev->dev, "enable clk_phy err, return %d\n", ret);
					return ret;
				}
			}
		}

		udelay(10);

	} else {
		DMSG_PANIC("[%s]: wrn: open clock failed, (%d, 0x%p)\n",
			sunxi_hci->hci_name,
			sunxi_hci->clk_is_open,
			sunxi_hci->mod_usb);
	}

	USBC_Clean_SIDDP(sunxi_hci);
#if !defined(CONFIG_ARCH_SUN8IW12) && !defined(CONFIG_ARCH_SUN50IW3) \
	&& !defined(CONFIG_ARCH_SUN8IW6) && !defined(CONFIG_ARCH_SUN50IW6) \
	&& !defined(CONFIG_ARCH_SUN8IW15) && !defined(CONFIG_ARCH_SUN50IW8) \
	&& !defined(CONFIG_ARCH_SUN8IW18) && !defined(CONFIG_ARCH_SUN8IW16) \
	&& !defined(CONFIG_ARCH_SUN50IW9) && !defined(CONFIG_ARCH_SUN50IW10) \
	&& !defined(CONFIG_ARCH_SUN8IW19) && !defined(CONFIG_ARCH_SUN50IW11) \
	&& !defined(CONFIG_ARCH_SUN8IW20) && !defined(CONFIG_ARCH_SUN20IW1) \
	&& !defined(CONFIG_ARCH_SUN50IW12)
	usb_phyx_tp_write(sunxi_hci, 0x2a, 3, 2);
#endif
	mutex_unlock(&usb_clock_lock);

	return 0;
}

static int close_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
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
		}

		if (strstr(sunxi_hci->hci_name, "ohci"))
			if (sunxi_hci->clk_ohci)
				clk_disable_unprepare(sunxi_hci->clk_ohci);

		if (sunxi_hci->clk_bus_hci)
			clk_disable_unprepare(sunxi_hci->clk_bus_hci);

		if (sunxi_hci->reset_hci)
			reset_control_assert(sunxi_hci->reset_hci);

		if (sunxi_hci->reset_phy)
			reset_control_deassert(sunxi_hci->reset_phy);

		udelay(10);
	} else {
		DMSG_PANIC("[%s]: wrn: open clock failed, (%d, 0x%p)\n",
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
			/* AHB Master interface INCR8 enable */
			reg_value |= (1 << 10);
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
			/* AHB Master interface INCR8 disable */
			reg_value &= ~(1 << 10);
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
		DMSG_PANIC("EER: unknown usbc_no(%d)\n", sunxi_hci->usbc_no);
		spin_unlock_irqrestore(&lock, flags);

		mutex_unlock(&usb_passby_lock);
		return;
	}

	spin_unlock_irqrestore(&lock, flags);

	mutex_unlock(&usb_passby_lock);
}

// static int alloc_pin(struct sunxi_hci_hcd *sunxi_hci)
// {
// 	u32 ret = 1;

// 	if (sunxi_hci->hsic_flag) {
// 		/* Marvell 4G HSIC ctrl */
// 		if (sunxi_hci->usb_host_hsic_rdy_valid) {
// 			ret = gpio_request(sunxi_hci->usb_host_hsic_rdy.gpio, NULL);
// 			if (ret != 0) {
// 				DMSG_PANIC("ERR: gpio_request failed\n");
// 				sunxi_hci->usb_host_hsic_rdy_valid = 0;
// 			} else {
// 				gpio_direction_output(sunxi_hci->usb_host_hsic_rdy.gpio, 0);
// 			}
// 		}

// 		/* SMSC usb3503 HSIC HUB ctrl */
// 		if (sunxi_hci->usb_hsic_usb3503_flag) {
// 			if (sunxi_hci->usb_hsic_hub_connect_valid) {
// 				ret = gpio_request(sunxi_hci->usb_hsic_hub_connect.gpio, NULL);
// 				if (ret != 0) {
// 					DMSG_PANIC("ERR: gpio_request failed\n");
// 					sunxi_hci->usb_hsic_hub_connect_valid = 0;
// 				} else {
// 					gpio_direction_output(sunxi_hci->usb_hsic_hub_connect.gpio, 1);
// 				}
// 			}

// 			if (sunxi_hci->usb_hsic_int_n_valid) {
// 				ret = gpio_request(sunxi_hci->usb_hsic_int_n.gpio, NULL);
// 				if (ret != 0) {
// 					DMSG_PANIC("ERR: gpio_request failed\n");
// 					sunxi_hci->usb_hsic_int_n_valid = 0;
// 				} else {
// 					gpio_direction_output(sunxi_hci->usb_hsic_int_n.gpio, 1);
// 				}
// 			}

// 			msleep(20);

// 			if (sunxi_hci->usb_hsic_reset_n_valid) {
// 				ret = gpio_request(sunxi_hci->usb_hsic_reset_n.gpio, NULL);
// 				if (ret != 0) {
// 					DMSG_PANIC("ERR: gpio_request failed\n");
// 					sunxi_hci->usb_hsic_reset_n_valid = 0;
// 				} else {
// 					gpio_direction_output(sunxi_hci->usb_hsic_reset_n.gpio, 1);
// 				}
// 			}

// 			/**
// 			 * usb3503 device goto hub connect status
// 			 * is need 100ms after reset
// 			 */
// 			msleep(100);
// 		}
// 	}

// 	return 0;
// }

// static void free_pin(struct sunxi_hci_hcd *sunxi_hci)
// {
// 	if (sunxi_hci->hsic_flag) {
// 		/* Marvell 4G HSIC ctrl */
// 		if (sunxi_hci->usb_host_hsic_rdy_valid) {
// 			gpio_free(sunxi_hci->usb_host_hsic_rdy.gpio);
// 			sunxi_hci->usb_host_hsic_rdy_valid = 0;
// 		}

// 		/* SMSC usb3503 HSIC HUB ctrl */
// 		if (sunxi_hci->usb_hsic_usb3503_flag) {
// 			if (sunxi_hci->usb_hsic_hub_connect_valid) {
// 				gpio_free(sunxi_hci->usb_hsic_hub_connect.gpio);
// 				sunxi_hci->usb_hsic_hub_connect_valid = 0;
// 			}

// 			if (sunxi_hci->usb_hsic_int_n_valid) {
// 				gpio_free(sunxi_hci->usb_hsic_int_n.gpio);
// 				sunxi_hci->usb_hsic_int_n_valid = 0;
// 			}

// 			if (sunxi_hci->usb_hsic_reset_n_valid) {
// 				gpio_free(sunxi_hci->usb_hsic_reset_n.gpio);
// 				sunxi_hci->usb_hsic_reset_n_valid = 0;
// 			}
// 		}
// 	}
// }

// void sunxi_set_host_hisc_rdy(struct sunxi_hci_hcd *sunxi_hci, int is_on)
// {
// 	if (sunxi_hci->usb_host_hsic_rdy_valid) {
// 		/* set config, output */
// 		gpio_direction_output(sunxi_hci->usb_host_hsic_rdy.gpio, is_on);
// 	}
// }
// EXPORT_SYMBOL(sunxi_set_host_hisc_rdy);

void sunxi_set_host_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	int ret;

	if (sunxi_hci->supply) {
		if (is_on) {
			ret = regulator_enable(sunxi_hci->supply);
			if (ret)
				DMSG_PANIC("ERR: %s regulator enable failed\n",
					sunxi_hci->hci_name);
		} else {
			ret = regulator_disable(sunxi_hci->supply);
			if (ret)
				DMSG_PANIC("ERR: %s regulator force disable failed\n",
					sunxi_hci->hci_name);
		}
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

/**
 * No care of usb0 vbus when otg connect pc
 * setup system without battery and to return.
 */
#ifdef CONFIG_USB_SUNXI_USB_MANAGER
#if !defined(CONFIG_ARCH_SUN8IW6)
	if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
		if (is_on) {
			if (usb_otg_id_status() == 1)
				return;
		}
	}
#endif
#endif

	if (sunxi_hci->supply) {
		if (is_on) {
			ret = regulator_enable(sunxi_hci->supply);
			if (ret)
				DMSG_PANIC("ERR: %s regulator enable failed\n",
					sunxi_hci->hci_name);
		} else {
			ret = regulator_disable(sunxi_hci->supply);
			if (ret)
				DMSG_PANIC("ERR: %s regulator force disable failed\n",
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
		dev_err(&pdev->dev, "%s, can't get vbase resource\n",
			sunxi_hci->hci_name);
		return -EINVAL;
	}

	if (strstr(sunxi_hci->hci_name, "ohci"))
		sunxi_hci->usb_vbase  -= SUNXI_USB_OHCI_BASE_OFFSET;

	sunxi_hci->otg_vbase  = of_iomap(np, 2);
	if (sunxi_hci->otg_vbase == NULL) {
		dev_err(&pdev->dev, "%s, can't get otg_vbase resource\n",
			sunxi_hci->hci_name);
		return -EINVAL;
	}

#if defined(CONFIG_ARCH_SUN50IW10)
	sunxi_hci->prcm = of_iomap(np, 3);
	if (sunxi_hci->prcm == NULL) {
		dev_err(&pdev->dev, "%s, can't get prcm resource\n",
			sunxi_hci->hci_name);
		return -EINVAL;
	}

	if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
		sunxi_hci->usb_ccmu_config = of_iomap(np, 4);
		if (sunxi_hci->usb_ccmu_config == NULL) {
			dev_err(&pdev->dev, "%s, can't get ccmu resource\n",
				sunxi_hci->hci_name);
			return -EINVAL;
		}

		sunxi_hci->usb_common_phy_config = of_iomap(np, 5);
		if (sunxi_hci->usb_common_phy_config == NULL) {
			dev_err(&pdev->dev, "%s, can't get common phy resource\n",
				sunxi_hci->hci_name);
			return -EINVAL;
		}
	}
#endif

	sunxi_hci->usb_base_res = kzalloc(sizeof(struct resource), GFP_KERNEL);
	if (sunxi_hci->usb_base_res == NULL) {
		dev_err(&pdev->dev, "%s, can't alloc mem\n",
			sunxi_hci->hci_name);
		return -ENOMEM;
	}

	ret = of_address_to_resource(np, 0, sunxi_hci->usb_base_res);
	if (ret)
		dev_err(&pdev->dev, "could not get regs\n");

	return 0;
}

static int sunxi_get_hci_clock(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci)
{
	struct device_node *np = pdev->dev.of_node;

	if (strstr(sunxi_hci->hci_name, "ehci")) {
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
			sunxi_hci->reset_phy = devm_reset_control_get_shared(&pdev->dev, "phy");
			if (IS_ERR(sunxi_hci->reset_phy))
				dev_warn(&pdev->dev, "Could not get phy rst share\n");
		} else {
			sunxi_hci->reset_phy = devm_reset_control_get_optional_shared(&pdev->dev, "phy");
			if (IS_ERR(sunxi_hci->reset_phy))
				dev_warn(&pdev->dev, "Could not get phy rst\n");
		}
	}

	if (strstr(sunxi_hci->hci_name, "ohci")) {
		sunxi_hci->clk_ohci = devm_clk_get(&pdev->dev, "ohci");
		if (IS_ERR(sunxi_hci->clk_ohci)) {
			dev_err(&pdev->dev, "Could not get ohci clock\n");
			return PTR_ERR(sunxi_hci->clk_ohci);
		}

		sunxi_hci->reset_phy = devm_reset_control_get_shared(&pdev->dev, "phy");
		if (IS_ERR(sunxi_hci->reset_phy))
			dev_warn(&pdev->dev, "Could not get phy rst share\n");
	}

	sunxi_hci->clk_bus_hci = devm_clk_get(&pdev->dev, "bus_hci");
	if (IS_ERR(sunxi_hci->clk_bus_hci)) {
		dev_err(&pdev->dev, "Could not get bus_hci clock\n");
		return PTR_ERR(sunxi_hci->clk_bus_hci);
	}

#if !defined(CONFIG_ARCH_SUN8IW20) && !defined(CONFIG_ARCH_SUN20IW1) && !defined(CONFIG_ARCH_SUN50IW12)
	sunxi_hci->clk_phy = devm_clk_get(&pdev->dev, "phy");
	if (IS_ERR(sunxi_hci->clk_phy)) {
		dev_err(&pdev->dev, "Could not get phy clock\n");
		return PTR_ERR(sunxi_hci->clk_phy);
	}
#endif
	sunxi_hci->reset_hci = devm_reset_control_get(&pdev->dev, "hci");
	if (IS_ERR(sunxi_hci->reset_hci))
		dev_warn(&pdev->dev, "Could not get hci rst\n");


	if (sunxi_hci->hsic_flag) {
		printk("%s()%d yhs\n", __func__, __LINE__);
		sunxi_hci->hsic_usbphy = of_clk_get(np, 2);
		if (IS_ERR(sunxi_hci->hsic_usbphy)) {
			sunxi_hci->hsic_usbphy = NULL;
			DMSG_PANIC("ERR: %s get usb hsic_usbphy failed.\n",
				sunxi_hci->hci_name);
		}

		sunxi_hci->clk_usbhsic12m = of_clk_get(np, 3);
		if (IS_ERR(sunxi_hci->clk_usbhsic12m)) {
			sunxi_hci->clk_usbhsic12m = NULL;
			DMSG_PANIC("ERR: %s get usb clk_usbhsic12m failed.\n",
				sunxi_hci->hci_name);
		}

		sunxi_hci->pll_hsic = of_clk_get(np, 4);
		if (IS_ERR(sunxi_hci->pll_hsic)) {
			sunxi_hci->pll_hsic = NULL;
			DMSG_PANIC("ERR: %s get usb pll_hsic failed.\n",
				sunxi_hci->hci_name);
		}
	}

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
		DMSG_PRINT("get %s used is fail, %d\n",
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
				DMSG_PRINT("HSIC is no susport in %s, and to return\n",
					sunxi_hci->hci_name);
				sunxi_hci->used = 0;
				return 0;
			}

			/* hsic regulator_io */
			ret = of_property_read_string(usbc_np,
					KEY_USB_HSIC_REGULATOR_IO,
					&sunxi_hci->hsic_regulator_io);
			if (ret) {
				DMSG_PRINT("get %s, hsic_regulator_io is fail, %d\n",
					sunxi_hci->hci_name, -ret);
				sunxi_hci->hsic_regulator_io = NULL;
			} else {
				if (!strcmp(sunxi_hci->hsic_regulator_io, "nocare")) {
					DMSG_PRINT("get %s, hsic_regulator_io is no nocare\n",
						sunxi_hci->hci_name);
					sunxi_hci->hsic_regulator_io = NULL;
				}
			}

			/* Marvell 4G HSIC ctrl */
			ret = of_property_read_u32(usbc_np,
					KEY_USB_HSIC_CTRL,
					&sunxi_hci->hsic_ctrl_flag);
			if (ret) {
				DMSG_PRINT("get %s usb_hsic_ctrl is fail, %d\n",
					sunxi_hci->hci_name, -ret);
				sunxi_hci->hsic_ctrl_flag = 0;
			}
			// if (sunxi_hci->hsic_ctrl_flag) {
				// sunxi_hci->usb_host_hsic_rdy.gpio =
				// 		of_get_named_gpio(usbc_np,
				// 			KEY_USB_HSIC_RDY_GPIO, 0);
				// if (gpio_is_valid(sunxi_hci->usb_host_hsic_rdy.gpio)) {
				// 	sunxi_hci->usb_host_hsic_rdy_valid = 1;
				// } else {
					// sunxi_hci->usb_host_hsic_rdy_valid = 0;
					// DMSG_PRINT("get %s drv_vbus_gpio is fail\n",
					// 	sunxi_hci->hci_name);
				// }
			// } else {
				// sunxi_hci->usb_host_hsic_rdy_valid = 0;
			// }

			/* SMSC usb3503 HSIC HUB ctrl */
			ret = of_property_read_u32(usbc_np,
					"usb_hsic_usb3503_flag",
					&sunxi_hci->usb_hsic_usb3503_flag);
			if (ret) {
				DMSG_PRINT("get %s usb_hsic_usb3503_flag is fail, %d\n",
					sunxi_hci->hci_name, -ret);
				sunxi_hci->usb_hsic_usb3503_flag = 0;
			}


			// if (sunxi_hci->usb_hsic_usb3503_flag) {
				// sunxi_hci->usb_hsic_hub_connect.gpio =
				// 		of_get_named_gpio(usbc_np,
				// 			"usb_hsic_hub_connect_gpio", 0);
				// if (gpio_is_valid(sunxi_hci->usb_hsic_hub_connect.gpio)) {
				// 	sunxi_hci->usb_hsic_hub_connect_valid = 1;
				// } else {
					// sunxi_hci->usb_hsic_hub_connect_valid = 0;
					// DMSG_PRINT("get %s usb_hsic_hub_connect is fail\n",
					// 	sunxi_hci->hci_name);
				// }


				// sunxi_hci->usb_hsic_int_n.gpio =
				// 		of_get_named_gpio(usbc_np,
				// 			"usb_hsic_int_n_gpio", 0);
				// if (gpio_is_valid(sunxi_hci->usb_hsic_int_n.gpio)) {
				// 	sunxi_hci->usb_hsic_int_n_valid = 1;
				// } else {
					// sunxi_hci->usb_hsic_int_n_valid = 0;
					// DMSG_PRINT("get %s usb_hsic_int_n is fail\n",
					// 	sunxi_hci->hci_name);
				// }


				// sunxi_hci->usb_hsic_reset_n.gpio =
				// 		of_get_named_gpio(usbc_np,
				// 			"usb_hsic_reset_n_gpio", 0);
				// if (gpio_is_valid(sunxi_hci->usb_hsic_reset_n.gpio)) {
				// 	sunxi_hci->usb_hsic_reset_n_valid = 1;
				// } else {
				// 	sunxi_hci->usb_hsic_reset_n_valid = 0;
				// 	DMSG_PRINT("get %s usb_hsic_reset_n is fail\n",
				// 		sunxi_hci->hci_name);
				// }

			// } else {
				// sunxi_hci->usb_hsic_hub_connect_valid = 0;
				// sunxi_hci->usb_hsic_int_n_valid = 0;
				// sunxi_hci->usb_hsic_reset_n_valid = 0;
			// }

		} else {
			sunxi_hci->hsic_ctrl_flag = 0;
			// sunxi_hci->usb_host_hsic_rdy_valid = 0;
			// sunxi_hci->usb_hsic_hub_connect_valid = 0;
			// sunxi_hci->usb_hsic_int_n_valid = 0;
			// sunxi_hci->usb_hsic_reset_n_valid = 0;
		}
	}

	/* usbc wakeup_suspend */
	ret = of_property_read_u32(usbc_np,
			KEY_USB_WAKEUP_SUSPEND,
			&sunxi_hci->wakeup_suspend);
	if (ret) {
		DMSG_PRINT("get %s wakeup_suspend is fail, %d\n",
			sunxi_hci->hci_name, -ret);
	}

	/* wakeup-source */
	if (of_property_read_bool(usbc_np, KEY_WAKEUP_SOURCE)) {
		sunxi_hci->wakeup_source_flag = 1;
	} else {
		DMSG_PRINT("get %s wakeup-source is fail.\n",
				sunxi_hci->hci_name);
		sunxi_hci->wakeup_source_flag = 0;
	}

	return 0;
}

int sunxi_get_hci_num(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int hci_num = 0;

	ret = of_property_read_u32(np, HCI_USBC_NO, &hci_num);
	if (ret)
		DMSG_PANIC("get hci_ctrl_num is fail, %d\n", -ret);

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
	if (sunxi_hci->wakeup_source_flag) {
		device_init_wakeup(&pdev->dev, true);
		dev_pm_set_wake_irq(&pdev->dev, sunxi_hci->irq_no);
	} else {
		DMSG_INFO("sunxi %s don't init wakeup source\n", sunxi_hci->hci_name);
	}

	return 0;
}
void enter_usb_standby(struct sunxi_hci_hcd *sunxi_hci)
{
	mutex_lock(&usb_standby_lock);
	if (!atomic_read(&usb_standby_cnt)) {
		atomic_add(1, &usb_standby_cnt);
	} else {
		/*phy after ehci and ohci suspend.*/
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
			/*usb1 0x800 bit[2], enable rc16M*/
			sunxi_hci_common_set_rc_clk(sunxi_hci, 1);
			/*usb1 0x800 bit[31], switch 16M*/
			sunxi_hci_common_switch_clk(sunxi_hci, 1);
			/*usb1 0x800 bit[3], open 16M gating*/
			sunxi_hci_common_set_rcgating(sunxi_hci, 1);
		}
#endif
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
#if IS_ENABLED(SUNXI_USB_STANDBY_NEW_MODE)
		/*phy reg, offset:0x800 bit2, set 1, enable RC16M CLK*/
		sunxi_hci_set_rc_clk(sunxi_hci, 1);
#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
		/*enable standby irq*/
		sunxi_hci_set_standby_irq(sunxi_hci, 1);
#endif
		/*phy reg, offset:0x808 bit3, set 1, remote enable*/
		sunxi_hci_set_wakeup_ctrl(sunxi_hci, 1);
		/*phy reg, offset:0x810 bit3 set 1, clean siddq*/
		sunxi_hci_set_siddq(sunxi_hci, 1);
#endif
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		/*phy reg, offset:0x0 bit31, set 1*/
		sunxi_hci_switch_clk(sunxi_hci, 1);
		/*phy reg, offset:0x0 bit3, set 1*/
		sunxi_hci_set_rcgating(sunxi_hci, 1);
#endif
		atomic_sub(1, &usb_standby_cnt);

	}
	mutex_unlock(&usb_standby_lock);
}
EXPORT_SYMBOL(enter_usb_standby);

void exit_usb_standby(struct sunxi_hci_hcd *sunxi_hci)
{
	mutex_lock(&usb_standby_lock);
	if (atomic_read(&usb_standby_cnt)) {
		atomic_sub(1, &usb_standby_cnt);
	} else {
		/*phy before ehci and ohci*/
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_hci->usbc_no == HCI0_USBC_NO) {
			/*usb1 0x800 bit[3]*/
			sunxi_hci_common_set_rcgating(sunxi_hci, 0);
			/*usb1 0x800 bit[31]*/
			sunxi_hci_common_switch_clk(sunxi_hci, 0);
			/*usb1 0x800 bit[2]*/
			sunxi_hci_common_set_rc_clk(sunxi_hci, 0);
		}
		/*phy reg, offset:0x0 bit3, set 0*/
		sunxi_hci_set_rcgating(sunxi_hci, 0);
		/*phy reg, offset:0x0 bit31, set 0*/
		sunxi_hci_switch_clk(sunxi_hci, 0);
#endif
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
		/*phy reg, offset:0x10 bit3 set 0, enable siddq*/
		sunxi_hci_set_siddq(sunxi_hci, 0);
		/*phy reg, offset:0x08 bit3, set 0, remote disable*/
		sunxi_hci_set_wakeup_ctrl(sunxi_hci, 0);
#if IS_ENABLED(SUNXI_USB_STANDBY_NEW_MODE)
#if IS_ENABLED(CONFIG_ARCH_SUN50IW9)
		/*clear standby irq status*/
		sunxi_hci_clean_standby_irq(sunxi_hci);
		/*disable standby irq */
		sunxi_hci_set_standby_irq(sunxi_hci, 0);
#endif
#endif
		/*phy reg, offset:0x0 bit2, set 0, disable rc clk*/
		sunxi_hci_set_rc_clk(sunxi_hci, 0);
#endif
		atomic_add(1, &usb_standby_cnt);
	}
	mutex_unlock(&usb_standby_lock);
}
EXPORT_SYMBOL(exit_usb_standby);

static int sunxi_get_hci_resource(struct platform_device *pdev,
		struct sunxi_hci_hcd *sunxi_hci, int usbc_no)
{
	if (sunxi_hci == NULL) {
		dev_err(&pdev->dev, "sunxi_hci is NULL\n");
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

	// alloc_pin(sunxi_hci);

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
					       sunxi_hci->ehci_base, 0x58, false);
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
					       sunxi_hci->ohci_base, 0x58, false);
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
					       sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE, 0x34, false);
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
					       sunxi_ehci->ehci_base, 0x58, false);
				DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
					  i, (unsigned int)(res1.start));
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       sunxi_ohci->ohci_base, 0x58, false);
				DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
					  i, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
				print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
					       sunxi_ehci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE, 0x34, false);
				DMSG_INFO("\n");
			}
		}

		return 0;

	default:
		DMSG_PANIC("%s()%d invalid argument\n", __func__, __LINE__);
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
			DMSG_PANIC("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_hci->used) {
			of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
			DMSG_INFO("usbc%d, ehci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)res.start);
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       sunxi_hci->ehci_base, 0x58, false);
			DMSG_INFO("\n");
		} else
			DMSG_PANIC("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

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
			DMSG_PANIC("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_hci->used) {
			of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
			DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res.start));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       sunxi_hci->ohci_base, 0x58, false);
			DMSG_INFO("\n");
		} else
			DMSG_PANIC("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

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
			DMSG_PANIC("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_hci->used) {
			of_address_to_resource(sunxi_hci->pdev->dev.of_node, 0, &res);
			DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE, 0x34, false);
			DMSG_INFO("\n");
		} else
			DMSG_PANIC("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

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
			DMSG_PANIC("%s()%d invalid argument\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (sunxi_ehci->used) {
			of_address_to_resource(sunxi_ehci->pdev->dev.of_node, 0, &res);
			of_address_to_resource(sunxi_ohci->pdev->dev.of_node, 0, &res1);
			DMSG_INFO("usbc%d, ehci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)res.start);
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       sunxi_ehci->ehci_base, 0x58, false);
			DMSG_INFO("usbc%d, ohci, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res1.start));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       sunxi_ohci->ohci_base, 0x58, false);
			DMSG_INFO("usbc%d, phy, base[0x%08x]:\n",
				  usbc_no, (unsigned int)(res.start + SUNXI_USB_PMU_IRQ_ENABLE));
			print_hex_dump(KERN_WARNING, "", DUMP_PREFIX_OFFSET, 4, 4,
				       sunxi_ehci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE, 0x34, false);
			DMSG_INFO("\n");
		} else
			DMSG_PANIC("%s()%d usbc%d is disable\n", __func__, __LINE__, usbc_no);

		return 0;

	default:
		DMSG_PANIC("%s()%d invalid argument\n", __func__, __LINE__);
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
			pr_err("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	if (sunxi_ehci1.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci1.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci1.standby_complete);
		} else {
			pr_err("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	if (sunxi_ehci2.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci2.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci2.standby_complete);
		} else {
			pr_err("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	if (sunxi_ehci3.used) {
		if (usbc_type == SUNXI_USB_EHCI) {
			complete(&sunxi_ehci3.standby_complete);
		} else if (usbc_type == SUNXI_USB_OHCI) {
			complete(&sunxi_ohci3.standby_complete);
		} else {
			pr_err("%s()%d err usb type\n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_hci_standby_completion);

int exit_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci)
{
	release_usb_regulator_io(sunxi_hci);
	// free_pin(sunxi_hci);
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
				dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
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
				dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
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
				dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
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
				dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
				return -1;
			}
			break;

		default:
			dev_err(&pdev->dev, "get hci num fail: %d\n", hci_num);
			return -1;
		}
	}

	ret = sunxi_get_hci_resource(pdev, sunxi_hci, usbc_no);
	if (ret != 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL(init_sunxi_hci);

static int __parse_hci_str(const char *buf, size_t size)
{
	int ret = 0;
	unsigned long val;

	if (!buf) {
		pr_err("%s()%d invalid argument\n", __func__, __LINE__);
		return -1;
	}

	ret = kstrtoul(buf, 10, &val);
	if (ret) {
		pr_err("%s()%d failed to transfer\n", __func__, __LINE__);
		return -1;
	}

	return val;
}

static ssize_t
ehci_reg_show(struct class *class, struct class_attribute *attr, char *buf)
{
	sunxi_hci_dump_reg_all(SUNXI_USB_EHCI);

	return 0;
}

static ssize_t
ehci_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
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
ohci_reg_show(struct class *class, struct class_attribute *attr, char *buf)
{
	sunxi_hci_dump_reg_all(SUNXI_USB_OHCI);

	return 0;
}

static ssize_t
ohci_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
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
phy_reg_show(struct class *class, struct class_attribute *attr, char *buf)
{
	sunxi_hci_dump_reg_all(SUNXI_USB_PHY);

	return 0;
}

static ssize_t
phy_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
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
all_reg_show(struct class *class, struct class_attribute *attr, char *buf)
{
	sunxi_hci_dump_reg_all(SUNXI_USB_ALL);

	return 0;
}

static ssize_t
all_reg_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
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
	.owner		= THIS_MODULE,
	.class_groups	= hci_class_groups,
};

static int __init init_sunxi_hci_class(void)
{
	int ret = 0;

	ret = class_register(&hci_class);
	if (ret) {
		DMSG_PANIC("%s()%d register class fialed\n", __func__, __LINE__);
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
MODULE_VERSION("1.0.10");

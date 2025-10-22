/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 *
 * Include file for SUNXI HCI Host Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_HCI_SUNXI_H__
#define __SUNXI_HCI_SUNXI_H__

#include <linux/delay.h>
#include <linux/types.h>

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of_gpio.h>
#include <sunxi-gpio.h>

#include <linux/pm_wakeirq.h>
#include <linux/regulator/consumer.h>
#include <linux/phy/phy.h>

#include <sunxi_usb_debug.h>

extern int usb_disabled(void);
extern struct atomic_notifier_head usb_pm_notifier_list;


#define HCI_USBC_NO     "hci_ctrl_no"

#if IS_ENABLED(CONFIG_ARCH_SUN8IW6)
#define HCI0_USBC_NO    1
#define HCI1_USBC_NO    2
#define HCI2_USBC_NO    3
#define HCI3_USBC_NO    4
#else
#define HCI0_USBC_NO    0
#define HCI1_USBC_NO    1
#define HCI2_USBC_NO    2
#define HCI3_USBC_NO    3
#endif

#define STANDBY_TIMEOUT 30000

/*
 * Support Low-power mode USB standby.
 */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN50IW9) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21)  || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define SUNXI_USB_STANDBY_LOW_POW_MODE		1
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN8IW21) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define SUNXI_USB_STANDBY_NEW_MODE		1
#endif

/* Note: Some platform not configure SIDDQ when USB Standby */
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define SUNXI_USB_STANDBY_SIDDQ_BYPASS		1
#endif

#define  USBC_Readb(reg)                        readb(reg)
#define  USBC_Readw(reg)                        readw(reg)
#define  USBC_Readl(reg)                        readl(reg)

#define  USBC_Writeb(value, reg)                writeb(value, reg)
#define  USBC_Writew(value, reg)                writew(value, reg)
#define  USBC_Writel(value, reg)                writel(value, reg)

#define  USBC_REG_test_bit_b(bp, reg)           (USBC_Readb(reg) & (1 << (bp)))
#define  USBC_REG_test_bit_w(bp, reg)           (USBC_Readw(reg) & (1 << (bp)))
#define  USBC_REG_test_bit_l(bp, reg)           (USBC_Readl(reg) & (1 << (bp)))

#define  USBC_REG_set_bit_b(bp, reg)            (USBC_Writeb((USBC_Readb(reg) | (1 << (bp))), (reg)))
#define  USBC_REG_set_bit_w(bp, reg)            (USBC_Writew((USBC_Readw(reg) | (1 << (bp))), (reg)))
#define  USBC_REG_set_bit_l(bp, reg)            (USBC_Writel((USBC_Readl(reg) | (1 << (bp))), (reg)))

#define  USBC_REG_clear_bit_b(bp, reg)          (USBC_Writeb((USBC_Readb(reg) & (~(1 << (bp)))), (reg)))
#define  USBC_REG_clear_bit_w(bp, reg)          (USBC_Writew((USBC_Readw(reg) & (~(1 << (bp)))), (reg)))
#define  USBC_REG_clear_bit_l(bp, reg)          (USBC_Writel((USBC_Readl(reg) & (~(1 << (bp)))), (reg)))

#define SUNXI_USB_EHCI_BASE_OFFSET              0x00
#define SUNXI_USB_OHCI_BASE_OFFSET              0x400
#define SUNXI_USB_EHCI_LEN                      0x58
#define SUNXI_USB_OHCI_LEN                      0x58

#define SUNXI_USB_EHCI_TIME_INT			0x30
#define SUNXI_USB_EHCI_STANDBY_IRQ_STATUS	1
#define SUNXI_USB_EHCI_STANDBY_IRQ		2

#define SUNXI_USB_PMU_IRQ_ENABLE                0x800
#define SUNXI_HCI_CTRL_3			0X808
#define SUNXI_HCI_PHY_CTRL                      0x810
#define SUNXI_HCI_PHY_TUNE                      0x818
#define SUNXI_HCI_UTMI_PHY_STATUS               0x824
#define SUNXI_HCI_CTRL_3_FORCE_SUSPEND		9
#define SUNXI_HCI_CTRL_3_REMOTE_WAKEUP		3
#define SUNXI_HCI_RC16M_CLK_ENBALE		2
#define SUNXI_HCI_USB_CTRL                      0x800
#define SUNXI_HCI_STANDBY_CLK_SEL		31
#define SUNXI_HCI_RC_CLK_GATING			3
#define SUNXI_HCI_RC_GEN_ENABLE			2
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW3) || IS_ENABLED(CONFIG_ARCH_SUN8IW15) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW8) || IS_ENABLED(CONFIG_ARCH_SUN8IW18) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW16) || IS_ENABLED(CONFIG_ARCH_SUN50IW9) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN8IW19) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW11) || IS_ENABLED(CONFIG_ARCH_SUN8IW20) \
	|| IS_ENABLED(CONFIG_ARCH_SUN20IW1) || IS_ENABLED(CONFIG_ARCH_SUN50IW12) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN65IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
#define SUNXI_HCI_PHY_CTRL_SIDDQ                3
#else
#define SUNXI_HCI_PHY_CTRL_SIDDQ                1
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
#define SUNXI_HCI_PHY_CTRL_VC_CLK               0
#define SUNXI_HCI_PHY_CTRL_VC_EN                1
#define SUNXI_HCI_PHY_CTRL_VC_DI                7
#endif

#define SUNXI_OTG_PHY_CTRL	0x410
#define SUNXI_OTG_PHY_CFG	0x420
#define SUNXI_OTG_PHY_STATUS	0x424
#define SUNXI_USBC_REG_INTUSBE	0x0050

#define EHCI_CAP_OFFSET		(0x00)
#define EHCI_CAP_LEN		(0x10)

#define EHCI_CAP_CAPLEN		(EHCI_CAP_OFFSET + 0x00)
#define EHCI_CAP_HCIVER		(EHCI_CAP_OFFSET + 0x00)
#define EHCI_CAP_HCSPAR		(EHCI_CAP_OFFSET + 0x04)
#define EHCI_CAP_HCCPAR		(EHCI_CAP_OFFSET + 0x08)
#define EHCI_CAP_COMPRD		(EHCI_CAP_OFFSET + 0x0c)


#define EHCI_OPR_OFFSET		(EHCI_CAP_OFFSET + EHCI_CAP_LEN)

#define EHCI_OPR_USBCMD		(EHCI_OPR_OFFSET + 0x00)
#define EHCI_OPR_USBSTS		(EHCI_OPR_OFFSET + 0x04)
#define EHCI_OPR_USBINTR	(EHCI_OPR_OFFSET + 0x08)
#define EHCI_OPR_FRINDEX	(EHCI_OPR_OFFSET + 0x0c)
#define EHCI_OPR_CRTLDSS	(EHCI_OPR_OFFSET + 0x10)
#define EHCI_OPR_PDLIST		(EHCI_OPR_OFFSET + 0x14)
#define EHCI_OPR_ASLIST		(EHCI_OPR_OFFSET + 0x18)
#define EHCI_OPR_CFGFLAG	(EHCI_OPR_OFFSET + 0x40)
#define EHCI_OPR_PORTSC		(EHCI_OPR_OFFSET + 0x44)

/**
 * PORT Control and Status Register
 * port_no is 0 based, 0, 1, 2, .....
 *
 * Reg EHCI_OPR_PORTSC
 */

/* Port Test Control bits */
#define EHCI_PORTSC_PTC_MASK		(0xf<<16)
#define EHCI_PORTSC_PTC_DIS		(0x0<<16)
#define EHCI_PORTSC_PTC_J		(0x1<<16)
#define EHCI_PORTSC_PTC_K		(0x2<<16)
#define EHCI_PORTSC_PTC_SE0NAK		(0x3<<16)
#define EHCI_PORTSC_PTC_PACKET		(0x4<<16)
#define EHCI_PORTSC_PTC_FORCE		(0x5<<16)

#define EHCI_PORTSC_OWNER		(0x1<<13)
#define EHCI_PORTSC_POWER		(0x1<<12)

#define EHCI_PORTSC_LS_MASK		(0x3<<10)
#define EHCI_PORTSC_LS_SE0		(0x0<<10)
#define EHCI_PORTSC_LS_J		(0x2<<10)
#define EHCI_PORTSC_LS_K		(0x1<<10)
#define EHCI_PORTSC_LS_UDF		(0x3<<10)

#define EHCI_PORTSC_RESET		(0x1<<8)
#define EHCI_PORTSC_SUSPEND		(0x1<<7)
#define EHCI_PORTSC_RESUME		(0x1<<6)
#define EHCI_PORTSC_OCC			(0x1<<5)
#define EHCI_PORTSC_OC			(0x1<<4)
#define EHCI_PORTSC_PEC			(0x1<<3)
#define EHCI_PORTSC_PE			(0x1<<2)
#define EHCI_PORTSC_CSC			(0x1<<1)
#define EHCI_PORTSC_CCS			(0x1<<0)

#define	EHCI_PORTSC_CHANGE		(EHCI_PORTSC_OCC | EHCI_PORTSC_PEC | EHCI_PORTSC_CSC)

#define  SUNXI_USB_HCI_DEBUG

#define  KEY_USB_GMA340_OE_GPIO         "usb_gma340_oe_gpio"
#define  KEY_USB_DRVVBUS_EN_GPIO        "usb_drvvbus_en_gpio"
#define  KEY_USB_WAKEUP_SUSPEND         "usb_wakeup_suspend"
#define  KEY_USB_HSIC_USBED             "usb_hsic_used"
#define  KEY_USB_HSIC_CTRL              "usb_hsic_ctrl"
#define  KEY_USB_HSIC_RDY_GPIO          "usb_hsic_rdy_gpio"
#define  KEY_USB_HSIC_REGULATOR_IO	"usb_hsic_regulator_io"
#define  KEY_WAKEUP_SOURCE              "wakeup-source"
#define  KEY_USB_PORT_TYPE		"usb_port_type"

/* xHCI */
#define XHCI_RESOURCES_NUM		2
#define XHCI_REGS_START			0x0
#define XHCI_REGS_END			0x7fff

/* xHCI Operational Registers */
#define XHCI_OP_REGS_HCUSBCMD		0X0020
#define XHCI_OP_REGS_HCUSBSTS		0X0024
#define XHCI_OP_REGS_HCPORT1SC		0X0420
#define XHCI_OP_REGS_HCPORT1PMSC	0X0424

#define SUNXI_GLOBALS_REGS_START	0xc100
#define SUNXI_GLOBALS_REGS_END		0xc6ff

/* Global Registers */
#define SUNXI_GLOBALS_REGS_GCTL		0xc110
#define SUNXI_GUSB2PHYCFG(n)		(0xc200 + (n * 0x04))
#define SUNXI_GUSB3PIPECTL(n)		(0xc2c0 + (n * 0x04))

/* Interface Status and Control Register */
#define SUNXI_APP			0x10000
#define SUNXI_PIPE_CLOCK_CONTROL	0x10014
#define SUNXI_PHY_TUNE_LOW		0x10018
#define SUNXI_PHY_TUNE_HIGH		0x1001c
#define SUNXI_PHY_EXTERNAL_CONTROL	0x10020

/* Bit fields */

/* Global Configuration Register */
#define SUNXI_GCTL_PRTCAPDIR(n)		((n) << 12)
#define SUNXI_GCTL_PRTCAP_HOST		1
#define SUNXI_GCTL_PRTCAP_DEVICE	2
#define SUNXI_GCTL_PRTCAP_OTG		3
#define SUNXI_GCTL_SOFITPSYNC		(0x01 << 10)
#define SUNXI_GCTL_CORESOFTRESET	(1 << 11)

/* Global USB2 PHY Configuration Register n */
#define SUNXI_USB2PHYCFG_SUSPHY		(0x01 << 6)
#define SUNXI_USB2PHYCFG_PHYSOFTRST	(1 << 31)

/* Global USB3 PIPE Control Register */
#define SUNXI_USB3PIPECTL_PHYSOFTRST	(1 << 31)

/* USB2.0 Interface Status and Control Register */
#define SUNXI_APP_FOCE_VBUS		(0x03 << 12)

/* PIPE Clock Control Register */
#define SUNXI_PPC_PIPE_CLK_OPEN		(0x01 << 6)

/* PHY External Control Register */
#define SUNXI_PEC_EXTERN_VBUS		(0x03 << 1)
#define SUNXI_PEC_SSC_EN		(0x01 << 24)
#define SUNXI_PEC_REF_SSP_EN		(0x01 << 26)

/* PHY Tune High Register */
#define SUNXI_TX_DEEMPH_3P5DB(n)	((n) << 19)
#define SUNXI_TX_DEEMPH_6DB(n)		((n) << 13)
#define SUNXI_TX_SWING_FULL(n)		((n) << 6)
#define SUNXI_LOS_BIAS(n)		((n) << 3)
#define SUNXI_TXVBOOSTLVL(n)		((n) << 0)

/* HCI UTMI PHY TUNE */
#define SUNXI_TX_VREF_TUNE_OFFSET	8
#define SUNXI_TX_RISE_TUNE_OFFSET	4
#define SUNXI_TX_RES_TUNE_OFFSET	2
#define SUNXI_TX_PREEMPAMP_TUNE_OFFSET	0
#define SUNXI_TX_VREF_TUNE		(0xf << SUNXI_TX_VREF_TUNE_OFFSET)
#define SUNXI_TX_RISE_TUNE		(0x3 << SUNXI_TX_RISE_TUNE_OFFSET)
#define SUNXI_TX_RES_TUNE		(0x3 << SUNXI_TX_RES_TUNE_OFFSET)
#define SUNXI_TX_PREEMPAMP_TUNE		(0x3 << SUNXI_TX_PREEMPAMP_TUNE_OFFSET)

/* PHYS EFUSE offest */
#define EFUSE_OFFSET			0x18		/* esuse offset */
#define SUNXI_HCI_PHY_EFUSE_ADJUST	0x10000		/* bit16 */
#define SUNXI_HCI_PHY_EFUSE_MODE	0x20000		/* bit17 */
#define SUNXI_HCI_PHY_EFUSE_RES		0x3C0000	/* bit18-21 */
#define SUNXI_HCI_PHY_EFUSE_COM		0x1C00000	/* bit22-24 */
#define SUNXI_HCI_PHY_EFUSE_USB0TX	0x1C00000	/* bit22-24 */
#define SUNXI_HCI_PHY_EFUSE_USB1TX	0xE000000	/* bit25-27 */

/* PHY RANGE: bit field */
#define PHY_RANGE_MODE_MASK			0x1000		/* bit12, mod_type */
#define PHY_RANGE_COMM_MASK			0xE00		/* bit11:9, common_data */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN251IW1)
#define PHY_RANGE_TRAN_MASK			0x3C0		/* bit9:6, trancevie_data */
#else
#define PHY_RANGE_TRAN_MASK			0x1C0		/* bit8:6, trancevie_data */
#endif
#define PHY_RANGE_RISE_MASK			0xc00		/* bit11:10, rise_data */
#define PHY_RANGE_PREE_MASK			0x30		/* bit5:4, preemphasis_data */
#define PHY_RANGE_RESI_MASK			0xF		/* bit3:0, resistance_data */

/* SYSCFG Base Registers */
#define SUNXI_SYS_CFG_BASE		0x03000000
/* Resister Calibration Control Register */
#define RESCAL_CTRL_REG		0x0160
#define   USBPHY2_RES200_SEL		BIT(6) /* Note: AW1903 Not use */
#define   USBPHY1_RES200_SEL		BIT(5)
#define   USBPHY0_RES200_SEL		BIT(4)
#define   PHY_o_RES200_SEL(n)		(BIT(4) << n)
#define   RESCAL_MODE			BIT(2)
#define   CAL_ANA_EN			BIT(1)
#define   CAL_EN			BIT(0)
/* Resister 200ohms Manual Control Register */
#define RES200_CTRL_REG		0x0164
#define   USBPHY2_RES200_CTRL		GENMASK(21, 16)
#define   USBPHY1_RES200_CTRL		GENMASK(13, 8)
#define   USBPHY0_RES200_CTRL		GENMASK(5, 0)
#define   PHY_o_RES200_CTRL(n)		(GENMASK(5, 0) << (8 * n))
#define   PHY_o_RES200_CTRL_DEFAULT(n)		(0x33 << (8 * n))

/* Resister RES0 ohms Manual Control Register */
#define RES0_CTRL_REG		0x0164
#define   USBPHY1_RES200_TRIM		GENMASK(15, 8)
#define   USBPHY0_RES200_TRIM		GENMASK(7, 0)
#define   PHY_o_RES200_TRIM(n)		(GENMASK(7, 0) << (8 * n))
#define   PHY_o_RES200_TRIM_DEFAULT(n)		(0xC8 << (8 * n))

#define syscfg_reg(offset)		(SUNXI_SYS_CFG_BASE + (offset))

#if IS_ENABLED(CONFIG_AW_FPGA_S4) || IS_ENABLED(CONFIG_AW_FPGA_V7)
#define SUNXI_USB_FPGA
#endif

enum sunxi_usbc_type {
	SUNXI_USB_UNKNOWN = 0,
	SUNXI_USB_EHCI,
	SUNXI_USB_OHCI,
	SUNXI_USB_XHCI,
	SUNXI_USB_PHY,
	SUNXI_USB_ALL,
};

enum usb_drv_vbus_type {
	USB_DRV_VBUS_TYPE_NULL = 0,
	USB_DRV_VBUS_TYPE_GIPO,
	USB_DRV_VBUS_TYPE_AXP,
};

enum usb_drvvbus_en_type {
	USB_DRVVBUS_EN_TYPE_NULL = 0,
	USB_DRVVBUS_EN_TYPE_GPIO,
};

/* 0: device only; 1: host only; 2: otg */
enum usb_port_type {
	USB_PORT_TYPE_DEVICE = 0,
	USB_PORT_TYPE_HOST,
	USB_PORT_TYPE_OTG,
};

enum usb_wakeup_source_type {
	SUPER_STANDBY = 0,
	USB_STANDBY,
	NORMAL_STANDBY,
};

struct sunxi_hci_hcd {
	__u32 usbc_no;                          /* usb controller number */
	__u32 irq_no;                           /* interrupt number */
	char hci_name[32];                      /* hci name */

	struct resource	*usb_base_res;          /* USB resources */
	struct resource	*usb_base_req;          /* USB resources */
	void __iomem	*usb_vbase;             /* USB base address */

	void __iomem	*otg_vbase;             /* USB base address */

	void __iomem	*ehci_base;
	__u32 ehci_reg_length;
	void __iomem	*ohci_base;
	__u32 ohci_reg_length;

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	void __iomem	*prcm;			/* for usb standby */

	void __iomem	*usb_common_phy_config; /* for keep common circuit configuration */
	void __iomem	*usb_ccmu_config;

#define SUNXI_CCMU_SCLK_GATING_USBPHY1_OFFSET	(1UL << 29)
#define SUNXI_CCMU_USBPHY1_RST_OFFSET		(1UL << 30)
#define SUNXI_CCMU_SCLK_GATING_OHCI1_OFFSET	(1UL << 31)

#define SUNXI_CCMU_USBEHCI1_GATING_OFFSET	(1UL << 5)
#define SUNXI_CCMU_USBEHCI1_RST_OFFSET		(1UL << 21)
#endif

	struct resource	*sram_base_res;         /* SRAM resources */
	struct resource	*sram_base_req;         /* SRAM resources */
	void __iomem	*sram_vbase;            /* SRAM base address */
	__u32 sram_reg_start;
	__u32 sram_reg_length;

	struct resource	*clock_base_res;        /* clock resources */
	struct resource	*clock_base_req;        /* clock resources */
	void __iomem	*clock_vbase;           /* clock base address */
	__u32 clock_reg_start;
	__u32 clock_reg_length;

	struct resource	*gpio_base_res;         /* gpio resources */
	struct resource	*gpio_base_req;         /* gpio resources */
	void __iomem	*gpio_vbase;            /* gpio base address */
	__u32 gpio_reg_start;
	__u32 gpio_reg_length;

	struct resource	*sdram_base_res;        /* sdram resources */
	struct resource	*sdram_base_req;        /* sdram resources */
	void __iomem	*sdram_vbase;           /* sdram base address */
	__u32 sdram_reg_start;
	__u32 sdram_reg_length;

	struct platform_device *pdev;
	struct usb_hcd *hcd;
	struct phy *usb2_generic_phy;           /* pointer to USB2 PHY */

	struct clk 	*clk_msi_lite;		/* msi-lite */
	struct clk 	*clk_usb_sys_ahb;	/* usb-sys-ahb */
	struct clk 	*clk_res;		/* res_dcap-24m */
	struct clk	*clk_hosc;		/* usb-24m */
	struct clk	*clk_bus_hci;
	struct clk	*clk_ohci;
	struct clk	*clk_phy;
	struct clk	*clk_mbus;
	struct clk	*clk_hclk;
	struct clk	*clk_rate;		/* for some SOC,get the clk freq*/

						/* legacy, fix me */
	struct clk	*ahb;                   /* ahb clock handle */
	struct clk	*mod_usbphy;            /* PHY0 clock handle */

	struct clk	*mod_usb;               /* mod_usb otg clock handle */
	struct clk	*hsic_usbphy;           /* hsic clock handle */
	struct clk	*pll_hsic;              /* pll_hsic clock handle */
	struct clk	*clk_usbhsic12m;        /* pll_hsic clock handle */
	struct clk	*clk_usbohci12m;        /* clk_usbohci12m clock handle */
	struct clk	*clk_hoscx2;            /* clk_hoscx2 clock handle */
	struct clk	*clk_losc;	        /* clk_losc clock handle */

	struct reset_control	*reset_hci;
	struct reset_control	*reset_phy;
	struct reset_control	*reset_usb;

	int phy_range;
	int rate_clk;
	bool rext_cal_bypass;			/* Hardware: the USB0/1-REXT is floating ? */
	bool extcon_supported;

	__u32 clk_is_open;                      /* is usb clock open */

	struct gpio_config drv_vbus_gpio_set;
	struct gpio_config drvvbus_en_gpio_set;
	struct gpio_config gma340_oe_gpio_set;

	const char  *regulator_io;
	const char  *used_status;
	int   regulator_value;
	struct regulator *regulator_io_hdle;
	enum usb_drv_vbus_type drv_vbus_type;
	const char *drv_vbus_name;
	enum usb_drvvbus_en_type drvvbus_en_type;
	const char *drvvbus_en_name;
	const char *gma340_oe_name;
	const char *det_vbus_name;
	u32 drvvbus_en_gpio_valid;
	u32 gma340_oe_gpio_valid;
	u32 usb_restrict_valid;
	__u8 power_flag;                        /* flag. power on or not */
	struct regulator *supply;
	struct regulator *hci_regulator;        /* hci regulator: VCC_USB */

	int used;                               /* flag. in use or not */
	__u8 probe;                             /* hc initialize */
	__u8 no_suspend;                        /* when usb is being enable, stop system suspend */
	enum usb_port_type port_type;		/* usb port type */
	int wakeup_suspend;                     /* flag. not suspend */

	int wakeup_source_flag;

						/* HSIC device susport */
	u32 hsic_flag;                          /* flag. hsic usbed */
	const char *hsic_regulator_io;
	struct regulator *hsic_regulator_io_hdle;

	struct gpio_config usb_host_hsic_rdy;	/* Marvell 4G HSIC ctrl */
	u32 usb_host_hsic_rdy_valid;
	u32 hsic_ctrl_flag;                     /* flag. hsic ctrl */
	u32 hsic_enable_flag;                   /* flag. hsic enable */

	u32 usb_hsic_usb3503_flag;      	/* SMSC usb3503 HSIC HUB ctrl */

	struct gpio_config usb_hsic_hub_connect;
	u32 usb_hsic_hub_connect_valid;

	struct gpio_config usb_hsic_int_n;
	u32 usb_hsic_int_n_valid;

	struct gpio_config usb_hsic_reset_n;
	u32 usb_hsic_reset_n_valid;


	int (*open_clock)(struct sunxi_hci_hcd *sunxi_hci, u32 ohci);
	int (*close_clock)(struct sunxi_hci_hcd *sunxi_hci, u32 ohci);
	void (*set_power)(struct sunxi_hci_hcd *sunxi_hci, int is_on);
	void (*port_configure)(struct sunxi_hci_hcd *sunxi_hci, u32 enable);
	void (*usb_passby)(struct sunxi_hci_hcd *sunxi_hci, u32 enable);
	void (*hci_phy_ctrl)(struct sunxi_hci_hcd *sunxi_hci, u32 enable);

	struct resource xhci_resources[XHCI_RESOURCES_NUM]; 	/* xhci */
	spinlock_t		lock;
	struct device		*dev;
	void			*mem;
	void __iomem	*regs;
	size_t		regs_size;
	void __iomem	*xhci_base;
	__u32 xhci_reg_length;

	struct work_struct resume_work;    	/* resume work */
	struct completion standby_complete;
};

int sunxi_hci_standby_completion(int usbc_type);
int init_sunxi_hci(struct platform_device *pdev, int usbc_type);
int exit_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci);
int sunxi_get_hci_num(struct platform_device *pdev);
void sunxi_set_host_hisc_rdy(struct sunxi_hci_hcd *sunxi_hci, int is_on);
void sunxi_set_host_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on);
int usb_phyx_tp_write(struct sunxi_hci_hcd *sunxi_hci,
		int addr, int data, int len);
int usb_phyx_write(struct sunxi_hci_hcd *sunxi_hci, int data);
int usb_phyx_read(struct sunxi_hci_hcd *sunxi_hci);
int usb_phyx_tp_read(struct sunxi_hci_hcd *sunxi_hci, int addr, int len);
int sunxi_usb_enable_xhci(void);
int sunxi_usb_disable_xhci(void);
void enter_usb_standby(struct sunxi_hci_hcd *sunxi_hci);
void exit_usb_standby(struct sunxi_hci_hcd *sunxi_hci);
void sunxi_hci_force_suspend(struct sunxi_hci_hcd *sunxi_hci, bool enter_standby);
bool sunxi_hci_ehci_enabled(struct sunxi_hci_hcd *sunxi_hci);
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
void sunxi_hci_set_siddq(struct sunxi_hci_hcd *sunxi_hci, int is_on);
void sunxi_hci_set_wakeup_ctrl(struct sunxi_hci_hcd *sunxi_hci, int is_on);
void sunxi_hci_set_rc_clk(struct sunxi_hci_hcd *sunxi_hci, int is_on);
void sunxi_hci_set_standby_irq(struct sunxi_hci_hcd *sunxi_hci, int is_on);
void sunxi_hci_clean_standby_irq(struct sunxi_hci_hcd *sunxi_hci);
#endif
void usb_new_phyx_write(struct sunxi_hci_hcd *sunxi_hci, u32 data);
u32 usb_new_phyx_read(struct sunxi_hci_hcd *sunxi_hci);
void usb_phyx_res_cal(__u32 usbc_no, bool enable, bool bypass);
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
void sunxi_hci_common_set_rc_clk(struct sunxi_hci_hcd *sunxi_hci,
					int is_on);
void sunxi_hci_common_switch_clk(struct sunxi_hci_hcd *sunxi_hci,
					int is_on);
void sunxi_hci_common_set_rcgating(struct sunxi_hci_hcd *sunxi_hci,
				   int is_on);
#endif

#endif /* __SUNXI_HCI_SUNXI_H__ */

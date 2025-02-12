// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __MV_USB2_H
#define __MV_USB2_H
#include <linux/usb/phy.h>

/* phy regs */
#define USB2_PHY_REG01			0x4
#define USB2_PHY_REG01_PLL_IS_READY	(0x1 << 0)
#define USB2_PHY_REG04			0x10
#define USB2_PHY_REG04_EN_HSTSOF	(0x1 << 0)
#define USB2_PHY_REG04_AUTO_CLEAR_DIS	(0x1 << 2)
#define USB2_PHY_REG08			0x20
#define USB2_PHY_REG08_DISCON_DET	(0x1 << 9)
#define USB2_PHY_REG0D			0x34
#define USB2_PHY_REG40			0x40
#define USB2_PHY_REG40_CLR_DISC	(0x1 << 0)
#define USB2_PHY_REG26			0x98
#define USB2_PHY_REG22			0x88
#define USB2_CFG_FORCE_CDRCLK		(0x1 << 6)
#define USB2_PHY_REG06			0x18
#define USB2_CFG_HS_SRC_SEL		(0x1 << 0)

#define USB2_ANALOG_REG14_13		0xa4
#define USB2_ANALOG_HSDAC_IREG_EN       (0x1 << 4)
#define USB2_ANALOG_HSDAC_ISEL_MASK     (0xf)
#define USB2_ANALOG_HSDAC_ISEL_11_INC   (0xb)
#define USB2_ANALOG_HSDAC_ISEL_25_INC   (0xf)
#define USB2_ANALOG_HSDAC_ISEL_15_INC   (0xc)
#define USB2_ANALOG_HSDAC_ISEL_17_INC   (0xd)
#define USB2_ANALOG_HSDAC_ISEL_22_INC   (0xe)

#define USB2D_CTRL_RESET_TIME_MS	50

struct mv_usb2_phy {
	struct usb_phy		phy;
	struct platform_device	*pdev;
	void __iomem		*base;
	struct clk		*clk;
	bool		handle_connect_change;
};

#endif

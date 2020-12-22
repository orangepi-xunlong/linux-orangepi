/*
 * drivers/usb/sunxi_usb/udc/sunxi_udc_board.c
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
#include <linux/clk/sunxi_name.h>
#include <linux/io.h>

#include  <mach/platform.h>
#include  "sunxi_udc_config.h"
#include  "sunxi_udc_board.h"

#define res_size(_r) (((_r)->end - (_r)->start) + 1)

#ifndef  SUNXI_USB_FPGA
u32  open_usb_clock(sunxi_udc_io_t *sunxi_udc_io)
{
	DMSG_INFO_UDC("open_usb_clock\n");

	if (sunxi_udc_io->ahb_otg && sunxi_udc_io->mod_usbphy && !sunxi_udc_io->clk_is_open) {
		if (clk_prepare_enable(sunxi_udc_io->ahb_otg)) {
			DMSG_PANIC("ERR:try to prepare_enable sunxi_udc_io->mod_usbphy failed!\n");
		}

		udelay(10);

		if (clk_prepare_enable(sunxi_udc_io->mod_usbphy)) {
			DMSG_PANIC("ERR:try to prepare_enable sunxi_udc_io->mod_usbphy failed!\n");
		}

		udelay(10);

		sunxi_udc_io->clk_is_open = 1;
	} else {
		DMSG_PANIC("ERR: clock handle is null, ahb_otg(0x%p), mod_usbotg(0x%p), mod_usbphy(0x%p), open(%d)\n",
			sunxi_udc_io->ahb_otg, sunxi_udc_io->mod_usbotg, sunxi_udc_io->mod_usbphy, sunxi_udc_io->clk_is_open);
	}

#ifdef  CONFIG_ARCH_SUN8IW6
	USBC_PHY_Set_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_VBUSVLDEXT);
	USBC_PHY_Clear_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#else
	UsbPhyInit(0);
#endif

#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
	{
		int reg_value = 0;
		reg_value = USBC_Readl(sunxi_udc_io->usb_vbase + 0x420);
		reg_value |= (0x01);
		USBC_Writel(reg_value, (sunxi_udc_io->usb_vbase + 0x420));
	}
#endif

	/*
	DMSG_INFO("[udc0]: open, 0x60(0x%x), 0xcc(0x%x), 0x2c0(0x%x)\n",
		(u32)USBC_Readl(sunxi_udc_io->clock_vbase + 0x60),
		(u32)USBC_Readl(sunxi_udc_io->clock_vbase + 0xcc),
		(u32)USBC_Readl(sunxi_udc_io->clock_vbase + 0x2c0));
	*/
	return 0;
}

u32 close_usb_clock(sunxi_udc_io_t *sunxi_udc_io)
{
	DMSG_INFO_UDC("close_usb_clock\n");

	if (sunxi_udc_io->ahb_otg && sunxi_udc_io->mod_usbphy && sunxi_udc_io->clk_is_open) {
		sunxi_udc_io->clk_is_open = 0;

		clk_disable_unprepare(sunxi_udc_io->mod_usbphy);

		clk_disable_unprepare(sunxi_udc_io->ahb_otg);
		udelay(10);

	} else {
		DMSG_PANIC("ERR: clock handle is null, ahb_otg(0x%p), mod_usbotg(0x%p), mod_usbphy(0x%p), open(%d)\n",
			sunxi_udc_io->ahb_otg, sunxi_udc_io->mod_usbotg, sunxi_udc_io->mod_usbphy, sunxi_udc_io->clk_is_open);
	}

	/*
	DMSG_INFO("[udc0]: close, 0x60(0x%x), 0xcc(0x%x),0x2c0(0x%x)\n",
		(u32)USBC_Readl(sunxi_udc_io->clock_vbase + 0x60),
		(u32)USBC_Readl(sunxi_udc_io->clock_vbase + 0xcc),
		(u32)USBC_Readl(sunxi_udc_io->clock_vbase + 0x2c0));
	*/

#ifdef  CONFIG_ARCH_SUN8IW6
	USBC_PHY_Set_Ctl(sunxi_udc_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#endif

	return 0;
}
#else
u32  open_usb_clock(sunxi_udc_io_t *sunxi_udc_io)
{
	u32 reg_value = 0;
	u32 ccmu_base = (u32 __force)SUNXI_CCM_VBASE;

	DMSG_INFO_UDC("open_usb_clock\n");

#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
{
	reg_value = USBC_Readl(sunxi_udc_io->usb_vbase + 0x420);
	reg_value |= (0x01);
	USBC_Writel(reg_value, (sunxi_udc_io->usb_vbase + 0x420));
}
#endif

	//Gating AHB clock for USB_phy1
	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value |= (1 << 24);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

	reg_value = USBC_Readl(ccmu_base + 0x2c0);
	reg_value |= (1 << 24);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x2c0));

	//delay to wati SIE stable
	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy1
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	//reg_value |= (1 << 9);
	reg_value |= (1 << 8);
	//reg_value |= (1 << 1);
	reg_value |= (1 << 0);          //disable reset
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	//delay some time
	reg_value = 10000;
	while(reg_value--);

	UsbPhyInit(0);
	return 0;
}

u32 close_usb_clock(sunxi_udc_io_t *sunxi_udc_io)
{
	u32 reg_value = 0;
	u32 ccmu_base = (u32 __force)SUNXI_CCM_VBASE;

	DMSG_INFO_UDC("close_usb_clock\n");

	//Gating AHB clock for USB_phy0
	reg_value = USBC_Readl(ccmu_base + 0x2c0);
	reg_value &= ~(1 << 24);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x2c0));

	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value &= ~(1 << 24);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

	/* ??? */
	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy0
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	//reg_value &= ~(1 << 9);
	reg_value &= ~(1 << 8);
	//reg_value &= ~(1 << 1);
	reg_value &= ~(1 << 0);          //disable reset
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	/* ??? */
	reg_value = 10000;
	while(reg_value--);

	return 0;
}
#endif

__s32 sunxi_udc_bsp_init(__u32 usbc_no, sunxi_udc_io_t *sunxi_udc_io)
{
	sunxi_udc_io->usbc.usbc_info[usbc_no].num = usbc_no;
	sunxi_udc_io->usbc.usbc_info[usbc_no].base = (u32 __force)sunxi_udc_io->usb_vbase;
	sunxi_udc_io->usbc.sram_base = (u32 __force)sunxi_udc_io->sram_vbase;

	//USBC_init(&sunxi_udc_io->usbc);
	sunxi_udc_io->usb_bsp_hdle = USBC_open_otg(usbc_no);
	if (sunxi_udc_io->usb_bsp_hdle == 0) {
		DMSG_PANIC("ERR: sunxi_udc_init: USBC_open_otg failed\n");
		return -1;
	}
#ifdef  SUNXI_USB_FPGA
	clear_usb_reg((u32 __force)sunxi_udc_io->usb_vbase);
	fpga_config_use_otg(sunxi_udc_io->usbc.sram_base);
#endif

	USBC_EnhanceSignal(sunxi_udc_io->usb_bsp_hdle);

	USBC_EnableDpDmPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_EnableIdPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_ForceId(sunxi_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DEVICE);
	USBC_ForceVbusValid(sunxi_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

	USBC_SelectBus(sunxi_udc_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW9) || defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
{
	__u32 reg_val = 0;
	reg_val = USBC_Readl(sunxi_udc_io->usb_vbase + USBPHYC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << 1);
	USBC_Writel(reg_val, (sunxi_udc_io->usb_vbase + USBPHYC_REG_o_PHYCTL));
}
#endif

	return 0;
}

static __s32 sunxi_udc_bsp_exit(__u32 usbc_no, sunxi_udc_io_t *sunxi_udc_io)
{
	USBC_DisableDpDmPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_DisableIdPullUp(sunxi_udc_io->usb_bsp_hdle);
	USBC_ForceId(sunxi_udc_io->usb_bsp_hdle, USBC_ID_TYPE_DISABLE);
	USBC_ForceVbusValid(sunxi_udc_io->usb_bsp_hdle, USBC_VBUS_TYPE_DISABLE);

	USBC_close_otg(sunxi_udc_io->usb_bsp_hdle);
	sunxi_udc_io->usb_bsp_hdle = 0;

	//USBC_exit(&sunxi_udc_io->usbc);
	return 0;
}

__s32 sunxi_udc_io_init(__u32 usbc_no, struct platform_device *pdev, sunxi_udc_io_t *sunxi_udc_io)
{
	spinlock_t lock;
	unsigned long flags = 0;

	sunxi_udc_io->usb_vbase  = (void __iomem *)SUNXI_USB_OTG_VBASE;
	sunxi_udc_io->sram_vbase = (void __iomem *)SUNXI_SRAMCTRL_VBASE;

	DMSG_INFO_UDC("usb_vbase  = 0x%x\n", (u32 __force)sunxi_udc_io->usb_vbase);
	DMSG_INFO_UDC("sram_vbase = 0x%x\n", (u32 __force)sunxi_udc_io->sram_vbase);

#ifndef  SUNXI_USB_FPGA
	/* open usb lock */
	sunxi_udc_io->ahb_otg = clk_get(NULL, USBOTG_CLK);
	if (IS_ERR(sunxi_udc_io->ahb_otg)) {
		DMSG_PANIC("ERR: get usb ahb_otg clk failed.\n");
		goto io_failed;
	}

	sunxi_udc_io->mod_usbphy = clk_get(NULL, USBPHY0_CLK);
	if (IS_ERR(sunxi_udc_io->mod_usbphy )) {
		DMSG_PANIC("ERR: get usb mod_usbphy failed.\n");
		goto io_failed;
	}
#endif

	open_usb_clock(sunxi_udc_io);

	/* initialize usb bsp */
	sunxi_udc_bsp_init(usbc_no, sunxi_udc_io);

	/* config usb fifo */
	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	USBC_ConfigFIFO_Base(sunxi_udc_io->usb_bsp_hdle, (u32 __force)sunxi_udc_io->sram_vbase, USBC_FIFO_MODE_8K);
	spin_unlock_irqrestore(&lock, flags);

	//print_all_usb_reg(NULL, (__u32)sunxi_udc_io->usb_vbase, 0, 5, "usb_u54555555555dc");
	return 0;

#ifndef  SUNXI_USB_FPGA
io_failed:
	if (IS_ERR(sunxi_udc_io->ahb_otg)) {
		//clk_put(sunxi_udc_io->ahb_otg);
		sunxi_udc_io->ahb_otg = NULL;
	}

	if (IS_ERR(sunxi_udc_io->mod_usbphy)) {
		//clk_put(sunxi_udc_io->mod_usbphy);
		sunxi_udc_io->mod_usbphy = NULL;
	}
	return -1;
#endif
}

__s32 sunxi_udc_io_exit(__u32 usbc_no, struct platform_device *pdev, sunxi_udc_io_t *sunxi_udc_io)
{
	sunxi_udc_bsp_exit(usbc_no, sunxi_udc_io);

	close_usb_clock(sunxi_udc_io);
#ifndef  SUNXI_USB_FPGA
	if (!IS_ERR(sunxi_udc_io->ahb_otg)) {
		clk_put(sunxi_udc_io->ahb_otg);
		sunxi_udc_io->ahb_otg = NULL;
	}

	if (!IS_ERR(sunxi_udc_io->mod_usbphy)) {
		clk_put(sunxi_udc_io->mod_usbphy);
		sunxi_udc_io->mod_usbphy = NULL;
	}
#endif
	sunxi_udc_io->usb_vbase  = NULL;
	sunxi_udc_io->sram_vbase = NULL;
	return 0;
}


/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_board.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb board config
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_HCD_BOARD_H__
#define  __SUNXI_HCD_BOARD_H__
#include <mach/sys_config.h>
#include <mach/gpio.h>
#define  USB_SRAMC_BASE			0x01c00000
#define  USB_CLOCK_BASE			0x01C20000
#define  USB_PIO_BASE			0x01c20800

/* i/o info */
typedef struct sunxi_hcd_io{
	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;		/* USB  base address 		*/

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 		*/

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 		*/

	struct resource	*pio_base_res;   	/* pio resources 		*/
	struct resource	*pio_base_req;   	/* pio resources 		*/
	void __iomem	*pio_vbase;		/* pio base address 		*/

	bsp_usbc_t usbc;			/* usb bsp config 		*/
	__hdle usb_bsp_hdle;			/* usb bsp handle 		*/

	__u32 clk_is_open;			/* is usb clock open? 		*/
	struct clk	*ahb_otg;		/* ahb clock handle 		*/
	struct clk	*mod_usbotg;		/* mod_usb otg clock handle 	*/
	struct clk	*mod_usbphy;		/* PHY0 clock handle 		*/

	unsigned drv_vbus_valid;
	unsigned usb_restrict_valid;
	script_item_u drv_vbus_gpio_set;
	script_item_u restrict_gpio_set;
	__u32 host_init_state;			/* usb controller's initial state. 0: not work, 1: work */
	__u32 usb_enable;
	__u32 usb_restrict_flag;
	__u32 no_suspend;

	struct regulator* regulator_io_hdle;
	int    regulator_io_vol;
	char*  regulator_io;

	struct regulator* regulator_id_vbus_hdle;
	int    regulator_id_vbus_vol;
	char*  regulator_id_vbus;


}sunxi_hcd_io_t;

#endif   //__SUNXI_HCD_BOARD_H__


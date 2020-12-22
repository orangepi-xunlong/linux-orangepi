/*
 * drivers/usb/sunxi_usb/hcd/hcd0/sunxi_hcd0.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include  "../include/sunxi_hcd_config.h"
#include  "../include/sunxi_hcd_core.h"
#include  "../include/sunxi_hcd_dma.h"

#include <linux/arisc/arisc.h>
#include <linux/power/aw_pm.h>
#include <linux/power/scenelock.h>
#define  DRIVER_AUTHOR          "Javen"
#define  DRIVER_DESC            "sunxi_hcd Host Controller Driver"
#define  sunxi_hcd_VERSION       "1.0"

#define DRIVER_INFO DRIVER_DESC ", v" sunxi_hcd_VERSION

#define SW_HCD_DRIVER_NAME      "sunxi_hcd_host0"
static const char sunxi_hcd_driver_name[] = SW_HCD_DRIVER_NAME;

static struct scene_lock  otg_standby_lock;

#ifdef CONFIG_ARCH_SUN8IW6
extern struct completion hcd_complete_notify;
#endif

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" SW_HCD_DRIVER_NAME);

/*
 * tables defining fifo_mode values.  define more if you like.
 * for host side, make sure both halves of ep1 are set up.
 */
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
static struct fifo_cfg mode_4_cfg[] = {
	{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 256, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 256, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 128, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 256, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 0, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 4096, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 0, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  5, .style = FIFO_RX,   .maxpacket = 0, .mode = BUF_SINGLE, },
};
#else
static struct fifo_cfg mode_4_cfg[] = {
	{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE, },
	{ .hw_ep_num =  5, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE, },
};
#endif

static struct fifo_cfg ep0_cfg = {
	.style = FIFO_RXTX,
	.maxpacket = 64,
};

static sunxi_hcd_io_t g_sunxi_hcd_io;
static __u32 usbc_no = 0;

#ifdef  CONFIG_USB_SUNXI_USB0_OTG
static struct platform_device *g_hcd0_pdev = NULL;
extern atomic_t thread_suspend_flag;
#endif

static struct sunxi_hcd_context_registers sunxi_hcd_context;
static struct sunxi_hcd *g_sunxi_hcd0 = NULL;

#define res_size(_r) (((_r)->end - (_r)->start) + 1)

static void sunxi_hcd_save_context(struct sunxi_hcd *sunxi_hcd);
static void sunxi_hcd_restore_context(struct sunxi_hcd *sunxi_hcd);
static const unsigned char TestPkt[54] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA,
		                                 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE,
		                                 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
		                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
		                                 0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7,
		                                 0xFB, 0xFD, 0x7E, 0x00};

static void session_init(struct sunxi_hcd *sunxi_hcd)
{
	int reg_val = 0;

	void __iomem *usbc_base = sunxi_hcd->mregs;

	USBC_INT_DisableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSBE_EN_SESSION_REQ));
	/* power down */
	reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
	reg_val &= ~(1 << USBC_BP_DEVCTL_SESSION);
	USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

	USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_LOW);

	sunxi_hcd_set_vbus(sunxi_hcd, 0);

	/* delay */
	mdelay(10);

	/* power on */
	reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
	reg_val |= (1 << USBC_BP_DEVCTL_SESSION);
	USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

	USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

	sunxi_hcd_set_vbus(sunxi_hcd, 1);

	USBC_INT_EnableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSBE_EN_SESSION_REQ));
}

static void sunxi_ed_test_hcd_start(struct sunxi_hcd *sunxi_hcd)
{
	void __iomem    *usbc_base  = NULL;

	u8              devctl      = 0;
	/* initialize parameter */
	usbc_base = sunxi_hcd->mregs;

	sunxi_hcd_ep_select(sunxi_hcd->mregs, 0);

	/*  Set INT enable registers, enable interrupts */
	USBC_Writew(sunxi_hcd->epmask, USBC_REG_INTTxE(usbc_base));
	USBC_Writew((sunxi_hcd->epmask & 0xfe), USBC_REG_INTRxE(usbc_base));
	USBC_Writeb(0xff, USBC_REG_INTUSBE(usbc_base));

	USBC_Writeb(0x00, USBC_REG_TMCTL(usbc_base));

	/* put into basic highspeed mode and start session */
	USBC_Writeb((1 << USBC_BP_POWER_H_HIGH_SPEED_EN), USBC_REG_PCTL(usbc_base));

	devctl = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
	devctl &= ~(1 << USBC_BP_DEVCTL_SESSION);
	USBC_Writeb(devctl, USBC_REG_DEVCTL(usbc_base));

	USBC_SelectBus(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

	session_init(sunxi_hcd);

	return;
}

static ssize_t show_ed_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", 0);
}

static ssize_t ed_test(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	u32 fifo = 0;
	struct platform_device 	*pdev 	= NULL;
	struct sunxi_hcd 			*sunxi_hcd	= NULL;
	pdev = to_platform_device(dev);;
	if(pdev == NULL){
		DMSG_PANIC("ERR: pdev is null\n");
		return -1;
	}

	sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);
	if(sunxi_hcd == NULL){
		DMSG_PANIC("ERR: sunxi_hcd is null\n");
		return -1;
	}

	sunxi_ed_test_hcd_start(sunxi_hcd);

	if(!strncmp(buf, "test_j_state", 12)){
		USBC_EnterMode_Test_J(g_sunxi_hcd_io.usb_bsp_hdle);
		DMSG_INFO_HCD0("test_mode:%s\n", "test_j_state");
	}else if(!strncmp(buf, "test_k_state", 12)){
		USBC_EnterMode_Test_K(g_sunxi_hcd_io.usb_bsp_hdle);
		DMSG_INFO_HCD0("test_mode:%s\n", "test_k_state");
	}else if(!strncmp(buf, "test_se0_nak", 12)){
		USBC_EnterMode_Test_SE0_NAK(g_sunxi_hcd_io.usb_bsp_hdle);
		DMSG_INFO_HCD0("test_mode:%s\n", "test_se0_nak");
	}else if(!strncmp(buf, "test_pack", 9)){
		DMSG_INFO_HCD0("test_mode___:%s\n", "test_pack");
		fifo = USBC_SelectFIFO(g_sunxi_hcd_io.usb_bsp_hdle, 0);
		USBC_WritePacket(g_sunxi_hcd_io.usb_bsp_hdle, fifo, 53, (__u32 *)TestPkt);
		USBC_Dev_WriteDataStatus(g_sunxi_hcd_io.usb_bsp_hdle, USBC_EP_TYPE_EP0, 0);
		USBC_EnterMode_TestPacket(g_sunxi_hcd_io.usb_bsp_hdle);
	}else if(!strncmp(buf, "disable_test_mode", 17)){
		DMSG_INFO_HCD0("start disable_test_mode\n");
		USBC_EnterMode_Idle(g_sunxi_hcd_io.usb_bsp_hdle);
	}else {
		DMSG_PANIC("ERR: test_mode Argment is invalid\n");
	}

	DMSG_INFO_HCD0("end test\n");

	return count;
}

static DEVICE_ATTR(otg_ed_test, 0644, show_ed_test, ed_test);

#ifndef  SUNXI_USB_FPGA
static s32 request_usb_regulator(sunxi_hcd_io_t *sunxi_hcd_io)
{
	if(sunxi_hcd_io->regulator_io != NULL){
		sunxi_hcd_io->regulator_io_hdle= regulator_get(NULL, sunxi_hcd_io->regulator_io);
		if(IS_ERR(sunxi_hcd_io->regulator_io_hdle)) {
			DMSG_PANIC("ERR: some error happen, usb0 regulator_io_hdle fail to get regulator!");
			return 0;
		}

		if(regulator_set_voltage(sunxi_hcd_io->regulator_io_hdle , sunxi_hcd_io->regulator_io_vol, sunxi_hcd_io->regulator_io_vol) < 0 ){
			DMSG_PANIC("ERR:usb0 regulator set io voltage:fail\n");
			regulator_put(sunxi_hcd_io->regulator_io_hdle);
			return 0;
		}
	}

	if(sunxi_hcd_io->regulator_id_vbus != NULL){
		sunxi_hcd_io->regulator_id_vbus_hdle= regulator_get(NULL, sunxi_hcd_io->regulator_id_vbus);
		if(IS_ERR(sunxi_hcd_io->regulator_id_vbus_hdle)) {
			DMSG_PANIC("ERR: some error happen, usb0 regulator_id_vbus_hdle fail to get regulator!");
			return 0;
		}

		if(regulator_set_voltage(sunxi_hcd_io->regulator_id_vbus_hdle , sunxi_hcd_io->regulator_id_vbus_vol, sunxi_hcd_io->regulator_id_vbus_vol) < 0 ){
			DMSG_PANIC("ERR:usb0 regulator set id vbus voltage:fail\n");
			regulator_put(sunxi_hcd_io->regulator_id_vbus_hdle);
			return 0;
		}

		if(regulator_enable(sunxi_hcd_io->regulator_id_vbus_hdle) < 0){
			DMSG_INFO_HCD0("ERR: usb0 id vbus regulator_enable fail\n");
		}
	}

	return 0;
}

static s32 release_usb_regulator(sunxi_hcd_io_t *sunxi_hcd_io)
{
	if(sunxi_hcd_io->regulator_io != NULL){
		regulator_put(sunxi_hcd_io->regulator_io_hdle);
		sunxi_hcd_io->regulator_io_hdle = NULL;
	}
	if(sunxi_hcd_io->regulator_id_vbus != NULL){
		regulator_put(sunxi_hcd_io->regulator_id_vbus_hdle);
		sunxi_hcd_io->regulator_id_vbus_hdle = NULL;
	}
	return 0;
}

static s32 usb_clock_init(sunxi_hcd_io_t *sunxi_hcd_io)
{
	sunxi_hcd_io->ahb_otg = clk_get(NULL, USBOTG_CLK);
	if (IS_ERR(sunxi_hcd_io->ahb_otg)) {
		DMSG_PANIC("ERR: get usb sunxi_hcd_io->ahb_otg clk failed.\n");
		goto failed;
	}

	sunxi_hcd_io->mod_usbphy = clk_get(NULL, USBPHY0_CLK);
	if (IS_ERR(sunxi_hcd_io->mod_usbphy)) {
		DMSG_PANIC("ERR: get usb phy0 clk failed.\n");
		goto failed;
	}

	return 0;

failed:
	if (IS_ERR(sunxi_hcd_io->ahb_otg)) {
		//clk_put(sunxi_hcd_io->ahb_otg);
		sunxi_hcd_io->ahb_otg = NULL;
	}

	if (IS_ERR(sunxi_hcd_io->mod_usbphy)) {
		//clk_put(sunxi_hcd_io->mod_usbphy);
		sunxi_hcd_io->mod_usbphy = NULL;
	}

	return -1;
}

static s32 usb_clock_exit(sunxi_hcd_io_t *sunxi_hcd_io)
{
	if (!IS_ERR(sunxi_hcd_io->ahb_otg)) {
		clk_put(sunxi_hcd_io->ahb_otg);
		sunxi_hcd_io->ahb_otg = NULL;
	}

	if (!IS_ERR(sunxi_hcd_io->mod_usbphy)) {
		clk_put(sunxi_hcd_io->mod_usbphy);
		sunxi_hcd_io->mod_usbphy = NULL;
	}

	return 0;
}

static s32 open_usb_clock(sunxi_hcd_io_t *sunxi_hcd_io)
{
	DMSG_INFO_HCD0("open_usb_clock\n");

	if (sunxi_hcd_io->ahb_otg && sunxi_hcd_io->mod_usbphy && !sunxi_hcd_io->clk_is_open) {
		if (clk_prepare_enable(sunxi_hcd_io->ahb_otg)) {
			DMSG_PANIC("ERR:try to prepare_enable sunxi_hcd_io->ahb_otg failed!\n");
		}

		udelay(10);

		if (clk_prepare_enable(sunxi_hcd_io->mod_usbphy)) {
			DMSG_PANIC("ERR:try to prepare_enable sunxi_hcd_io->mod_usbphy failed!\n");
		}
		udelay(10);

		sunxi_hcd_io->clk_is_open = 1;
	} else {
		DMSG_INFO_HCD0("ERR: open usb clock failed, (0x%p, 0x%p, 0x%p, %d)\n",
			sunxi_hcd_io->ahb_otg, sunxi_hcd_io->mod_usbotg, sunxi_hcd_io->mod_usbphy, sunxi_hcd_io->clk_is_open);
	}

#ifdef  CONFIG_ARCH_SUN8IW6
	USBC_PHY_Set_Ctl(sunxi_hcd_io->usb_vbase, USBC_PHY_CTL_VBUSVLDEXT);
	USBC_PHY_Clear_Ctl(sunxi_hcd_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#else
	UsbPhyInit(0);
#endif

	/*
	DMSG_INFO("[hcd0]: open, 0x60(0x%x), 0xcc(0x%x), 0x2c0(0x%x)\n",
		(u32)USBC_Readl(sunxi_hcd_io->clock_vbase + 0x60),
		(u32)USBC_Readl(sunxi_hcd_io->clock_vbase + 0xcc),
		(u32)USBC_Readl(sunxi_hcd_io->clock_vbase + 0x2c0));
	*/
	return 0;
}

static s32 close_usb_clock(sunxi_hcd_io_t *sunxi_hcd_io)
{
	DMSG_INFO_HCD0("close_usb_clock\n");

	if (sunxi_hcd_io->ahb_otg && sunxi_hcd_io->mod_usbphy && sunxi_hcd_io->clk_is_open) {
		sunxi_hcd_io->clk_is_open = 0;

		clk_disable_unprepare(sunxi_hcd_io->mod_usbphy);

		clk_disable_unprepare(sunxi_hcd_io->ahb_otg);
		udelay(10);

	} else {
		DMSG_INFO_HCD0("ERR: close usb clock failed, (0x%p, 0x%p, 0x%p, %d)\n",
			sunxi_hcd_io->ahb_otg, sunxi_hcd_io->mod_usbotg, sunxi_hcd_io->mod_usbphy, sunxi_hcd_io->clk_is_open);
	}

	/*
	DMSG_INFO("[hcd0]: close, 0x60(0x%x), 0xcc(0x%x), 0x2c0(0x%x)\n",
		(u32)USBC_Readl(sunxi_hcd_io->clock_vbase + 0x60),
		(u32)USBC_Readl(sunxi_hcd_io->clock_vbase + 0xcc),
		(u32)USBC_Readl(sunxi_hcd_io->clock_vbase + 0x2c0));
	*/

#ifdef  CONFIG_ARCH_SUN8IW6
		USBC_PHY_Set_Ctl(sunxi_hcd_io->usb_vbase, USBC_PHY_CTL_SIDDQ);
#endif

	return 0;
}

static __s32 pin_init(sunxi_hcd_io_t *sunxi_hcd_io)
{
#ifndef  SUNXI_USB_FPGA
	__s32 ret = 0;
	script_item_value_type_e type = 0;
	script_item_u item_temp;

	/* get usb_host_init_state */
	type = script_get_item(SET_USB0, KEY_USB_HOST_INIT_STATE, &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
		sunxi_hcd_io->host_init_state = item_temp.val;
	} else {
		DMSG_PANIC("get host_init_state failed\n");
		sunxi_hcd_io->host_init_state = 1;
	}

	/* usbc drv_vbus */
	type = script_get_item(SET_USB0, KEY_USB_DRVVBUS_GPIO, &sunxi_hcd_io->drv_vbus_gpio_set);
	if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
		sunxi_hcd_io->drv_vbus_valid = 1;
	} else {
		DMSG_PANIC("get usbc0(drv vbus) failed\n");
		sunxi_hcd_io->drv_vbus_valid = 0;
	}

	if (sunxi_hcd_io->drv_vbus_valid) {
		ret = gpio_request(sunxi_hcd_io->drv_vbus_gpio_set.gpio.gpio, "otg_drv_vbus");
		if (ret != 0) {
			DMSG_PANIC("gpio_request failed\n");
			sunxi_hcd_io->drv_vbus_valid = 0;
		} else {
			/* set config, ouput */
			//sunxi_gpio_setcfg(sunxi_hcd_io->drv_vbus_gpio_set.gpio.gpio, 1);

			/* reserved is pull down */
			//sunxi_gpio_setpull(sunxi_hcd_io->drv_vbus_gpio_set.gpio.gpio, 2);
			gpio_direction_output(sunxi_hcd_io->drv_vbus_gpio_set.gpio.gpio, 0);
		}
	}

	/* get regulator io information */
	type = script_get_item(SET_USB0, KEY_USB_REGULATOR_IO, &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			sunxi_hcd_io->regulator_io = NULL;
		}else{
			sunxi_hcd_io->regulator_io = item_temp.str;

			type = script_get_item(SET_USB0, KEY_USB_REGULATOR_IO_VOL, &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				sunxi_hcd_io->regulator_io_vol = item_temp.val;
			}else{
				DMSG_INFO_HCD0("get usb io voltage is failed\n");
				sunxi_hcd_io->regulator_io_vol = 0;
			}
		}
	}else {
		DMSG_INFO_HCD0("usb_regulator io is not exist\n");
		sunxi_hcd_io->regulator_io = NULL;
	}

	/* get regulator information */
	type = script_get_item(SET_USB0, KEY_USB_REGULATOR_ID_VBUS, &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			DMSG_INFO_HCD0("get usb_regulator id vbus is nocare\n");
			sunxi_hcd_io->regulator_id_vbus = NULL;
		}else{
			sunxi_hcd_io->regulator_id_vbus = item_temp.str;

			type = script_get_item(SET_USB0, KEY_USB_REGULATOR_ID_VBUS_VOL, &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				sunxi_hcd_io->regulator_id_vbus_vol = item_temp.val;
			}else{
				DMSG_INFO_HCD0("get usb id voltage is failed\n");
				sunxi_hcd_io->regulator_id_vbus_vol = 0;
			}
		}
	}else {
		DMSG_INFO_HCD0("usb_regulator id vbus io is not exist\n");
		sunxi_hcd_io->regulator_id_vbus = NULL;
	}

	type = script_get_item(SET_USB0, KEY_USB_USB_NOT_SUSPEND, &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
		sunxi_hcd_io->no_suspend = item_temp.val;
	} else {
		DMSG_PANIC("get no_suspend status failed\n");
		sunxi_hcd_io->no_suspend = 0;
	}

#else
	sunxi_hcd_io->host_init_state = 1;
#endif
	return 0;
}

static __s32 pin_exit(sunxi_hcd_io_t *sunxi_hcd_io)
{
#ifndef	SUNXI_USB_FPGA
	if (sunxi_hcd_io->drv_vbus_valid) {
		gpio_free(sunxi_hcd_io->drv_vbus_gpio_set.gpio.gpio);
		sunxi_hcd_io->drv_vbus_valid = 0;
	}

	if (sunxi_hcd_io->usb_restrict_valid) {
		gpio_free(sunxi_hcd_io->restrict_gpio_set.gpio.gpio);
		sunxi_hcd_io->usb_restrict_valid = 0;
	}
#endif
	return 0;
}
#else
static s32 request_usb_regulator(sunxi_hcd_io_t *sunxi_hcd_io)
{
	return 0;
}

static s32 release_usb_regulator(sunxi_hcd_io_t *sunxi_hcd_io)
{
	return 0;
}

static s32 usb_clock_init(sunxi_hcd_io_t *sunxi_hcd_io)
{
	return 0;

}

static s32 usb_clock_exit(sunxi_hcd_io_t *sunxi_hcd_io)
{
	return 0;
}

static s32 open_usb_clock(sunxi_hcd_io_t *sunxi_hcd_io)
{
	u32 reg_value = 0;
	u32 ccmu_base = (u32 __force)SUNXI_CCM_VBASE;

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
	reg_value |= (1 << 0);          /* disable reset */
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	//delay some time
	reg_value = 10000;
	while(reg_value--);

	UsbPhyInit(0);

	return 0;
}

static s32 close_usb_clock(sunxi_hcd_io_t *sunxi_hcd_io)
{
	u32 reg_value = 0;
	u32 ccmu_base = (u32 __force)SUNXI_CCM_VBASE;

	//Gating AHB clock for USB_phy0
	reg_value = USBC_Readl(ccmu_base + 0x2c0);
	reg_value &= ~(1 << 24);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x2c0));

	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value &= ~(1 << 24);	            /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy0
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	//reg_value &= ~(1 << 9);
	reg_value &= ~(1 << 8);
	//reg_value &= ~(1 << 1);
	reg_value &= ~(1 << 0);          /* disable reset */
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	reg_value = 10000;
	while(reg_value--);

	return 0;
}

static __s32 pin_init(sunxi_hcd_io_t *sunxi_hcd_io)
{
	sunxi_hcd_io->host_init_state = 1;
	return 0;
}

static __s32 pin_exit(sunxi_hcd_io_t *sunxi_hcd_io)
{
	return 0;
}
#endif


static void sunxi_set_regulator_io(struct sunxi_hcd *sunxi_hcd, int is_on)
{
	DMSG_INFO_HCD0("[%s]:hcd0 set_regulator_io:%s\n", sunxi_hcd->driver_name, (is_on ? "ON" : "OFF"));

#ifndef  SUNXI_USB_FPGA
	if((sunxi_hcd->sunxi_hcd_io->regulator_io != NULL) && (sunxi_hcd->sunxi_hcd_io->regulator_io_hdle != NULL)){
		if(is_on){
			if(regulator_enable(sunxi_hcd->sunxi_hcd_io->regulator_io_hdle) < 0){
				DMSG_INFO("hcd0 regulator_enable fail\n");
			}
		}else {
			if(regulator_disable(sunxi_hcd->sunxi_hcd_io->regulator_io_hdle) < 0){
				DMSG_INFO("hcd0 regulator_disable fail\n");
			}
		}
	}
#endif
	return;
}
static void sunxi_hcd_board_set_vbus(struct sunxi_hcd *sunxi_hcd, int is_on)
{
#ifndef  SUNXI_USB_FPGA
	u32 on_off = 0;
#endif

	DMSG_INFO_HCD0("[%s]: Set USB Power %s\n", sunxi_hcd->driver_name, (is_on ? "ON" : "OFF"));

#ifndef  SUNXI_USB_FPGA
	/* set power */
	if (sunxi_hcd->sunxi_hcd_io->drv_vbus_gpio_set.gpio.data == 0) {
		on_off = is_on ? 1 : 0;
	} else {
		on_off = is_on ? 0 : 1;
	}

	/* set gpio data */
	if (sunxi_hcd->sunxi_hcd_io->drv_vbus_valid) {
		__gpio_set_value(sunxi_hcd->sunxi_hcd_io->drv_vbus_gpio_set.gpio.gpio, on_off);
	}
#endif

	if (is_on) {
		USBC_Host_StartSession(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);
	} else {
		USBC_Host_EndSession(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_DISABLE);
	}

	return;
}

static __s32 sunxi_hcd_bsp_init(__u32 usbc_no, sunxi_hcd_io_t *sunxi_hcd_io)
{
	memset(&sunxi_hcd_io->usbc, 0, sizeof(bsp_usbc_t));

	sunxi_hcd_io->usbc.usbc_info[usbc_no].num  = usbc_no;
	sunxi_hcd_io->usbc.usbc_info[usbc_no].base = (u32 __force)sunxi_hcd_io->usb_vbase;
	sunxi_hcd_io->usbc.sram_base = (u32 __force)sunxi_hcd_io->sram_vbase;

	//USBC_init(&sunxi_hcd_io->usbc);
	sunxi_hcd_io->usb_bsp_hdle = USBC_open_otg(usbc_no);
	if (sunxi_hcd_io->usb_bsp_hdle == 0) {
		DMSG_PANIC("ERR: sunxi_hcd_init: USBC_open_otg failed\n");
		return -1;
	}

#ifdef  SUNXI_USB_FPGA
	clear_usb_reg((u32 __force)sunxi_hcd_io->usb_vbase);
	fpga_config_use_otg((u32 __force)sunxi_hcd_io->sram_vbase);
#endif

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW9)
{
	__u32 reg_val = 0;
	reg_val = USBC_Readl(sunxi_hcd_io->usb_vbase + USBPHYC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << 1);
	USBC_Writel(reg_val, (sunxi_hcd_io->usb_vbase + USBPHYC_REG_o_PHYCTL));
}
#endif

	return 0;
}

static __s32 sunxi_hcd_bsp_exit(__u32 usbc_no, sunxi_hcd_io_t *sunxi_hcd_io)
{
	USBC_close_otg(sunxi_hcd_io->usb_bsp_hdle);
	sunxi_hcd_io->usb_bsp_hdle = 0;

	//USBC_exit(&sunxi_hcd_io->usbc);
	return 0;
}

static __s32 sunxi_hcd_io_init(__u32 usbc_no, struct platform_device *pdev, sunxi_hcd_io_t *sunxi_hcd_io)
{
	__s32 ret = 0;
	spinlock_t lock;
	unsigned long flags = 0;

	sunxi_hcd_io->usb_vbase  = (void __iomem *)SUNXI_USB_OTG_VBASE;
	sunxi_hcd_io->sram_vbase = (void __iomem *)SUNXI_SRAMCTRL_VBASE;

	//DMSG_INFO_HCD0("[usb host]: usb_vbase    = 0x%x\n", (u32)sunxi_hcd_io->usb_vbase);
	//DMSG_INFO_HCD0("[usb host]: sram_vbase   = 0x%x\n", (u32)sunxi_hcd_io->sram_vbase);

	/* open usb lock */
	ret = usb_clock_init(sunxi_hcd_io);
	if (ret != 0) {
		DMSG_PANIC("ERR: usb_clock_init failed\n");
		ret = -ENOMEM;
		goto io_failed;
	}

	open_usb_clock(sunxi_hcd_io);

	/* initialize usb bsp */
	sunxi_hcd_bsp_init(usbc_no, sunxi_hcd_io);

	/* config usb fifo */
	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	USBC_ConfigFIFO_Base(sunxi_hcd_io->usb_bsp_hdle, (u32 __force)sunxi_hcd_io->sram_vbase, USBC_FIFO_MODE_8K);
	spin_unlock_irqrestore(&lock, flags);

	/* config drv_vbus pin */
	ret = pin_init(sunxi_hcd_io);
	if (ret != 0) {
		DMSG_PANIC("ERR: pin_init failed\n");
		ret = -ENOMEM;
		goto io_failed1;
	}

	DMSG_INFO_HCD0("host_init_state = %d\n", sunxi_hcd_io->host_init_state);

	if(sunxi_hcd_io->no_suspend){
		scene_lock_init(&otg_standby_lock, SCENE_USB_STANDBY,  "otg_standby");
	}

	return 0;

io_failed1:
	sunxi_hcd_bsp_exit(usbc_no, sunxi_hcd_io);
	close_usb_clock(sunxi_hcd_io);
	usb_clock_exit(sunxi_hcd_io);

io_failed:
	sunxi_hcd_io->usb_vbase = NULL;
	sunxi_hcd_io->sram_vbase = NULL;

	return ret;
}

static __s32 sunxi_hcd_io_exit(__u32 usbc_no, struct platform_device *pdev, sunxi_hcd_io_t *sunxi_hcd_io)
{
	sunxi_hcd_bsp_exit(usbc_no, sunxi_hcd_io);

	if(sunxi_hcd_io->no_suspend){
		scene_lock_destroy(&otg_standby_lock);
	}

	/* config drv_vbus pin */
	pin_exit(sunxi_hcd_io);

	close_usb_clock(sunxi_hcd_io);

	usb_clock_exit(sunxi_hcd_io);

	sunxi_hcd_io->usb_vbase = NULL;
	sunxi_hcd_io->sram_vbase = NULL;

	return 0;
}

static void sunxi_hcd_shutdown(struct platform_device *pdev)
{
	struct sunxi_hcd *sunxi_hcd = NULL;
	unsigned long flags = 0;

	if (pdev == NULL) {
		DMSG_INFO_HCD0("err: Invalid argment\n");
		return ;
	}

	sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);
	if (sunxi_hcd == NULL) {
		DMSG_INFO_HCD0("err: sunxi_hcd is null\n");
		return ;
	}

	if (!sunxi_hcd->enable) {
		DMSG_INFO_HCD0("wrn: hcd is disable, need not shutdown\n");
		return ;
	}

	DMSG_INFO_HCD0("sunxi_hcd shutdown start\n");

	if(sunxi_hcd->sunxi_hcd_io->no_suspend){
		scene_lock_destroy(&otg_standby_lock);
	}

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_platform_disable(sunxi_hcd);
	sunxi_hcd_generic_disable(sunxi_hcd);
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	sunxi_hcd_port_suspend_ex(sunxi_hcd);
	sunxi_hcd_set_vbus(sunxi_hcd, 0);
	sunxi_set_regulator_io(sunxi_hcd, 0);
	close_usb_clock(&g_sunxi_hcd_io);

	DMSG_INFO_HCD0("Set aside some time to AXP\n");

	/* Set aside some time to AXP */
	mdelay(200);

	DMSG_INFO_HCD0("sunxi_hcd shutdown end\n");

	return;
}

/*
 * configure a fifo; for non-shared endpoints, this may be called
 *
 * returns negative errno or offset for next fifo.
 */
int fifo_setup(struct sunxi_hcd *sunxi_hcd,
				struct sunxi_hcd_hw_ep *hw_ep,
				const struct fifo_cfg *cfg,
				u16 offset)
{
	void __iomem    *usbc_base  	= NULL;
	u16             maxpacket   	= 0;
	u32 		ep_fifo_size 	= 0;
	u16		old_ep_index 	= 0;
	u32             is_double_fifo 	= 0;

	/* check argment */
	if (sunxi_hcd == NULL || hw_ep == NULL || cfg == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return -1;
	}

	/* initialize parameter */
	usbc_base = sunxi_hcd->mregs;
	maxpacket = cfg->maxpacket;

	/* expect hw_ep has already been zero-initialized */
	if (cfg->mode == BUF_DOUBLE) {
		ep_fifo_size = offset + (maxpacket << 1);
		is_double_fifo = 1;
	} else {
		ep_fifo_size = offset + maxpacket;
	}

	if (ep_fifo_size > sunxi_hcd->config->ram_size) {
		DMSG_PANIC("ERR: fifo_setup, free is not enough, ep_fifo_size = %d, ram_size = %d\n",
			ep_fifo_size, sunxi_hcd->config->ram_size);
		return -EMSGSIZE;
	}

	DMSG_DBG_HCD("hw_ep->epnum   = 0x%x\n", hw_ep->epnum);
	DMSG_DBG_HCD("is_double_fifo = 0x%x\n", is_double_fifo);
	DMSG_DBG_HCD("ep_fifo_size   = 0x%x\n", ep_fifo_size);
	DMSG_DBG_HCD("hw_ep->fifo    = 0x%x\n", (u32)hw_ep->fifo);
	DMSG_DBG_HCD("maxpacket      = 0x%x\n", maxpacket);

	/* configure the FIFO */
	old_ep_index = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, hw_ep->epnum);

	/* EP0 reserved endpoint for control, bidirectional;
	 * EP1 reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == 1) {
		sunxi_hcd->bulk_ep = hw_ep;
	}

	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
	switch (cfg->style) {
	case FIFO_TX:
		{
			USBC_ConfigFifo(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
				USBC_EP_TYPE_TX,
				is_double_fifo,
				maxpacket,
				offset);

			hw_ep->tx_double_buffered = is_double_fifo;
			hw_ep->max_packet_sz_tx = maxpacket;
		}
		break;
	case FIFO_RX:
		{
			USBC_ConfigFifo(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
				USBC_EP_TYPE_RX,
				is_double_fifo,
				maxpacket,
				offset);

			hw_ep->rx_double_buffered = is_double_fifo;
			hw_ep->max_packet_sz_rx = maxpacket;
		}
		break;
	case FIFO_RXTX:
		{
			if (hw_ep->epnum == 0) {
				USBC_ConfigFifo(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
					USBC_EP_TYPE_EP0,
					is_double_fifo,
					maxpacket,
					offset);

				hw_ep->tx_double_buffered = 0;
				hw_ep->rx_double_buffered = 0;

				hw_ep->max_packet_sz_tx = maxpacket;
				hw_ep->max_packet_sz_rx = maxpacket;
			} else {
				DMSG_PANIC("ERR: fifo_setup, FIFO_RXTX not support\n");
			}

			hw_ep->is_shared_fifo = true;
		}
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok */
	sunxi_hcd->epmask |= (1 << hw_ep->epnum);

	return ep_fifo_size;
}

static int ep_config_from_table(struct sunxi_hcd *sunxi_hcd)
{
	const struct fifo_cfg	*cfg    = NULL;
	unsigned                i       = 0;
	unsigned                n       = 0;
	int                     offset  = 0;
	struct sunxi_hcd_hw_ep  *hw_ep  = sunxi_hcd->endpoints;

	cfg = mode_4_cfg;
	n = ARRAY_SIZE(mode_4_cfg);

	/* assert(offset > 0) */
	offset = fifo_setup(sunxi_hcd, hw_ep, &ep0_cfg, 0);

	for (i = 0; i < n; i++) {
		u8 epn = cfg->hw_ep_num;

		DMSG_DBG_HCD("i=%d, cfg->hw_ep_num = 0x%x\n", i, cfg->hw_ep_num);

		if (epn >= sunxi_hcd->config->num_eps) {
			DMSG_PANIC("ERR: %s: invalid ep%d, max ep is ep%d\n",
				sunxi_hcd_driver_name, epn, sunxi_hcd->config->num_eps);
			return -EINVAL;
		}

		offset = fifo_setup(sunxi_hcd, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			DMSG_PANIC("ERR: %s: mem overrun, ep %d\n", sunxi_hcd_driver_name, epn);
			return -EINVAL;
		}

		epn++;
		sunxi_hcd->nr_endpoints = max(epn, sunxi_hcd->nr_endpoints);
	}

	DMSG_DBG_HCD("ep_config_from_table: %s: %d/%d max ep, %d/%d memory\n",
		sunxi_hcd_driver_name, n + 1, sunxi_hcd->config->num_eps * 2 - 1,
		offset, sunxi_hcd->config->ram_size);

	if (!sunxi_hcd->bulk_ep) {
		DMSG_PANIC("ERR: %s: missing bulk\n", sunxi_hcd_driver_name);
		return -EINVAL;
	}

	return 0;
}

/*
 * ep_config_from_hw - when sunxi_hcd_C_DYNFIFO_DEF is false
 */
static int ep_config_from_hw(struct sunxi_hcd *sunxi_hcd)
{
	u8                  epnum       = 0;
	struct sunxi_hcd_hw_ep   *hw_ep      = NULL;
	void  __iomem *usbc_base  = sunxi_hcd->mregs;
	int                 ret         = 0;

	/* FIXME pick up ep0 maxpacket size */

	for (epnum = 1; epnum < sunxi_hcd->config->num_eps; epnum++) {
		sunxi_hcd_ep_select(usbc_base, epnum);
		hw_ep = sunxi_hcd->endpoints + epnum;

		ret = sunxi_hcd_read_fifosize(sunxi_hcd, hw_ep, epnum);
		if (ret < 0) {
			break;
		}

		/* FIXME set up hw_ep->{rx,tx}_double_buffered */

		/* pick an RX/TX endpoint for bulk */
		if (hw_ep->max_packet_sz_tx < 512
			|| hw_ep->max_packet_sz_rx < 512)
			continue;

		/* REVISIT:  this algorithm is lazy, we should at least
		 * try to pick a double buffered endpoint.
		 */
		if (sunxi_hcd->bulk_ep)
			continue;
		sunxi_hcd->bulk_ep = hw_ep;
	}

	if (!sunxi_hcd->bulk_ep) {
		DMSG_PANIC("ERR: %s: missing bulk\n", sunxi_hcd_driver_name);
		return -EINVAL;
	}

	return 0;
}

enum {
	SW_HCD_CONTROLLER_MHDRC,
	SW_HCD_CONTROLLER_HDRC,
};

/*
 * Initialize USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 *
 */
static int sunxi_hcd_core_init(u16 sunxi_hcd_type, struct sunxi_hcd *sunxi_hcd)
{
	u8              reg         = 0;
	char            *type       = NULL;
	void __iomem    *usbc_base  = sunxi_hcd->mregs;
	int             status      = 0;
	int             i           = 0;

	/* log core options (read using indexed model) */
	sunxi_hcd_ep_select(usbc_base, 0);
	reg = sunxi_hcd_read_configdata(usbc_base);

	if (SW_HCD_CONTROLLER_MHDRC == sunxi_hcd_type) {
		sunxi_hcd->is_multipoint = 1;
		type = "M";
	} else {
		sunxi_hcd->is_multipoint = 0;
		type = "";

		DMSG_INFO_HCD0("%s: kernel must blacklist external hubs\n", sunxi_hcd_driver_name);
	}

	/* configure ep0 */
	sunxi_hcd_configure_ep0(sunxi_hcd);

	/* discover endpoint configuration */
	sunxi_hcd->nr_endpoints = 1;
	sunxi_hcd->epmask = 1;

	if (reg & (1 << USBC_BP_CONFIGDATA_DYNFIFO_SIZING)) {
		if (sunxi_hcd->config->dyn_fifo) {
			status = ep_config_from_table(sunxi_hcd);
		} else {
			DMSG_PANIC("ERR: reconfigure software for Dynamic FIFOs\n");
			status = -ENODEV;
		}
	} else {
		if (!sunxi_hcd->config->dyn_fifo) {
			status = ep_config_from_hw(sunxi_hcd);
		} else {
			DMSG_PANIC("ERR: reconfigure software for static FIFOs\n");
			return -ENODEV;
		}
	}

	if (status < 0) {
		DMSG_PANIC("ERR: sunxi_hcd_core_init, config failed\n");
		return status;
	}

	/* finish init, and print endpoint config */
	for (i = 0; i < sunxi_hcd->nr_endpoints; i++) {
		struct sunxi_hcd_hw_ep *hw_ep = sunxi_hcd->endpoints + i;

		hw_ep->fifo         = (void __iomem *)USBC_REG_EPFIFOx(usbc_base, i);
		hw_ep->regs         = usbc_base;
		hw_ep->target_regs  = sunxi_hcd_read_target_reg_base(i, usbc_base);
		hw_ep->rx_reinit    = 1;
		hw_ep->tx_reinit    = 1;

		/*
		if (hw_ep->max_packet_sz_tx) {
			DMSG_INFO_HCD0("%s: hw_ep %d%s, %smax %d\n",
			sunxi_hcd_driver_name, i,
			(hw_ep->is_shared_fifo ? "shared" : "tx"),
			(hw_ep->tx_double_buffered ? "doublebuffer, " : ""),
			hw_ep->max_packet_sz_tx);
		}

		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			DMSG_INFO_HCD0("%s: hw_ep %d%s, %smax %d\n",
			sunxi_hcd_driver_name, i,
			"rx",
			(hw_ep->rx_double_buffered ? "doublebuffer, " : ""),
			hw_ep->max_packet_sz_rx);
		}

		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx)) {
			DMSG_INFO_HCD0("hw_ep %d not configured\n", i);
		}
		*/
	}

	return 0;
}

/*
 * Only used to provide driver mode change events
 *
 */
static void sunxi_hcd_irq_work(struct work_struct *data)
{
	struct sunxi_hcd *sunxi_hcd = NULL;
	void __iomem *usbc_base = NULL;
	int reg_val = 0;

	sunxi_hcd = container_of(data, struct sunxi_hcd, irq_work);
	if ( sunxi_hcd == NULL || !sunxi_hcd->enable) {
		DMSG_PANIC("ERR: sunxi_hcd_irq_work: sunxi_hcd is null\n");
		return;
	}

	usbc_base = sunxi_hcd->mregs;
	if(usbc_base == NULL){
		DMSG_PANIC("ERR: sunxi_hcd_irq_work: usbc_base is null\n");
		return;
	}

	if (sunxi_hcd->vbus_error_flag) {
		sunxi_hcd->vbus_error_flag = 0;
		/* power down */
		sunxi_hcd_set_vbus(sunxi_hcd, 0);

		USBC_INT_EnableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_VBUS_ERROR));
	}

	if (sunxi_hcd->session_req_flag) {
		sunxi_hcd->session_req_flag = 0;
		/* power down */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val &= ~(1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_LOW);

		sunxi_hcd_set_vbus(sunxi_hcd, 0);

		/* delay */
		msleep(100);

		/* power on */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val |= (1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

		sunxi_hcd_set_vbus(sunxi_hcd, 1);

		USBC_INT_EnableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSBE_EN_SESSION_REQ));
	}

	if (sunxi_hcd->reset_flag) {
		sunxi_hcd->reset_flag = 0;
		/* power down */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val &= ~(1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_LOW);

		sunxi_hcd_set_vbus(sunxi_hcd, 0);

		/* delay */
		msleep(100);

		/* power on */
		reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
		reg_val |= (1 << USBC_BP_DEVCTL_SESSION);
		USBC_Writeb(reg_val, USBC_REG_DEVCTL(usbc_base));

		USBC_ForceVbusValid(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_VBUS_TYPE_HIGH);

		sunxi_hcd_set_vbus(sunxi_hcd, 1);

		/* disconnect */
		sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_START;
		usb_hcd_resume_root_hub(sunxi_hcd_to_hcd(sunxi_hcd));
		sunxi_hcd_root_disconnect(sunxi_hcd);

		USBC_INT_EnableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_RESET));

		sunxi_hcd->is_connected = 0;
	}

	sysfs_notify(&sunxi_hcd->controller->kobj, NULL, "mode");
	return;
}

static const struct hc_driver sunxi_hcd_hc_driver = {
	.description		= "sunxi_hcd-hcd",
	.product_desc		= "sunxi_hcd host driver",
	.hcd_priv_size		= sizeof(struct sunxi_hcd),
	//.flags                = HCD_USB2 | HCD_MEMORY,
	.flags			= HCD_USB11 | HCD_MEMORY,

	/* not using irq handler or reset hooks from usbcore, since
	 * those must be shared with peripheral code for OTG configs
	 */

	.start              	= sunxi_hcd_h_start,
	.stop               	= sunxi_hcd_h_stop,

	.get_frame_number	= sunxi_hcd_h_get_frame_number,

	.urb_enqueue		= sunxi_hcd_urb_enqueue,
	.urb_dequeue		= sunxi_hcd_urb_dequeue,
	.endpoint_disable	= sunxi_hcd_h_disable,

	.hub_status_data	= sunxi_hcd_hub_status_data,
	.hub_control		= sunxi_hcd_hub_control,
	.bus_suspend		= sunxi_hcd_bus_suspend,
	.bus_resume		= sunxi_hcd_bus_resume,
};

/*
 * Init struct sunxi_hcd
 *
 */
static struct sunxi_hcd *allocate_instance(struct device *dev,
						struct sunxi_hcd_config *config,
						void __iomem *mbase)
{
	struct sunxi_hcd         *sunxi_hcd = NULL;
	struct sunxi_hcd_hw_ep   *ep = NULL;
	int                 epnum = 0;
	struct usb_hcd      *hcd = NULL;

	hcd = usb_create_hcd(&sunxi_hcd_hc_driver, dev, dev_name(dev));
	if (!hcd) {
		DMSG_PANIC("ERR: usb_create_hcd failed\n");
		return NULL;
	}

	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */
	sunxi_hcd = hcd_to_sunxi_hcd(hcd);
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: hcd_to_sunxi_hcd failed\n");
		return NULL;
	}

	memset(sunxi_hcd, 0, sizeof(struct sunxi_hcd));

	INIT_LIST_HEAD(&sunxi_hcd->control);
	INIT_LIST_HEAD(&sunxi_hcd->in_bulk);
	INIT_LIST_HEAD(&sunxi_hcd->out_bulk);

	hcd->uses_new_polling   = 1;
	sunxi_hcd->vbuserr_retry     = VBUSERR_RETRY_COUNT;
	sunxi_hcd->mregs             = mbase;
	sunxi_hcd->ctrl_base         = mbase;
	sunxi_hcd->nIrq              = -ENODEV;
	sunxi_hcd->config            = config;
	sunxi_hcd->ignore_disconnect = false;

#ifndef  CONFIG_USB_SUNXI_USB0_OTG
	g_sunxi_hcd0 = sunxi_hcd;
	sunxi_hcd->enable = 1;
#else
	if (sunxi_hcd->config->port_info->port_type == USB_PORT_TYPE_HOST) {
		g_sunxi_hcd0 = sunxi_hcd;
		sunxi_hcd->enable = 1;
	}
#endif

	strcpy(sunxi_hcd->driver_name, sunxi_hcd_driver_name);

	for (epnum = 0, ep = sunxi_hcd->endpoints;
			epnum < sunxi_hcd->config->num_eps;
			epnum++, ep++) {
		ep->sunxi_hcd = sunxi_hcd;
		ep->epnum = epnum;
	}

	sunxi_hcd->controller = dev;
	sunxi_hcd->sunxi_hcd_io  = &g_sunxi_hcd_io;
	sunxi_hcd->usbc_no	 = usbc_no;
	return sunxi_hcd;
}

static void sunxi_hcd_free(struct sunxi_hcd *sunxi_hcd)
{
	void __iomem    *usbc_base  = sunxi_hcd->mregs;

	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */
	if (sunxi_hcd->nIrq >= 0) {
		if (sunxi_hcd->irq_wake) {
			disable_irq_wake(sunxi_hcd->nIrq);
		}

		free_irq(sunxi_hcd->nIrq, sunxi_hcd);
	}

	if (is_hcd_support_dma(sunxi_hcd->usbc_no)) {
		sunxi_hcd_dma_remove(sunxi_hcd);
	}

	USBC_Writeb(0x00, USBC_REG_DEVCTL(usbc_base));
	sunxi_hcd_platform_exit(sunxi_hcd);
	USBC_Writeb(0x00, USBC_REG_DEVCTL(usbc_base));

	usb_put_hcd(sunxi_hcd_to_hcd(sunxi_hcd));
}

/*
 * Perform generic per-controller initialization.
 *
 */
static irqreturn_t hcd0_generic_interrupt(int irq, void *__hci)
{
	DMSG_DBG_HCD("irq: %s\n", sunxi_hcd_driver_name);

	return generic_interrupt(irq, __hci);
}

/*
 * Perform generic per-controller initialization.
 *
 */
static int sunxi_hcd_init_controller(struct device *dev, int nIrq, void __iomem *ctrl)
{
	int                        	status  = 0;
	struct sunxi_hcd              	*sunxi_hcd	= NULL;
	struct sunxi_hcd_platform_data	*plat   = dev->platform_data;

	/* The driver might handle more features than the board; OK.
	 * Fail when the board needs a feature that's not enabled.
	 */
	if (!plat) {
		DMSG_PANIC("ERR: no platform_data?\n");
		return -ENODEV;
	}

	switch (plat->mode) {
	case SW_HCD_HOST:
		DMSG_INFO_HCD0("platform is usb host\n");
		break;
	default:
		DMSG_PANIC("ERR: unkown platform mode(%d)\n", plat->mode);
		return -EINVAL;
	}

	/* allocate */
	sunxi_hcd = allocate_instance(dev, plat->config, ctrl);
	if (!sunxi_hcd) {
		DMSG_PANIC("ERR: allocate_instance failed\n");
		return -ENOMEM;
	}

	spin_lock_init(&sunxi_hcd->lock);
	sunxi_hcd->board_mode        = plat->mode;
	sunxi_hcd->board_set_power   = plat->set_power;
	sunxi_hcd->set_clock         = plat->set_clock;
	sunxi_hcd->min_power         = plat->min_power;
	sunxi_hcd->board_set_vbus    = sunxi_hcd_board_set_vbus;
	sunxi_hcd->init_controller = 1;

	/* assume vbus is off */

	/* platform adjusts sunxi_hcd->mregs and sunxi_hcd->isr if needed,
	 * and activates clocks
	 */
	sunxi_hcd->isr = hcd0_generic_interrupt;
	status = sunxi_hcd_platform_init(sunxi_hcd);
	if (status < 0) {
		DMSG_PANIC("ERR: sunxi_hcd_platform_init failed\n");
		goto fail;
	}

	if (!sunxi_hcd->isr) {
		DMSG_PANIC("ERR: sunxi_hcd->isr is null\n");
		status = -ENODEV;
		goto fail2;
	}

	if (is_hcd_support_dma(sunxi_hcd->usbc_no)) {
		status = sunxi_hcd_dma_probe(sunxi_hcd);
		if (status < 0) {
			DMSG_PANIC("ERR: sunxi_hcd_dma_probe failed\n");
			goto fail2;
		}
	}

	/* be sure interrupts are disabled before connecting ISR */
	sunxi_hcd_platform_disable(sunxi_hcd);
	sunxi_hcd_generic_disable(sunxi_hcd);

	/* setup sunxi_hcd parts of the core (especially endpoints) */
	status = sunxi_hcd_core_init(plat->config->multipoint ? SW_HCD_CONTROLLER_MHDRC : SW_HCD_CONTROLLER_HDRC, sunxi_hcd);
	if (status < 0) {
		DMSG_PANIC("ERR: sunxi_hcd_core_init failed\n");
		goto fail2;
	}

	/* Init IRQ workqueue before request_irq */
	INIT_WORK(&sunxi_hcd->irq_work, sunxi_hcd_irq_work);

	/* attach to the IRQ */
	if (request_irq(nIrq, sunxi_hcd->isr, 0, dev_name(dev), sunxi_hcd)) {
		DMSG_PANIC("ERR: request_irq %d failed!\n", nIrq);
		status = -ENODEV;
		goto fail2;
	}

	sunxi_hcd->nIrq = nIrq;

	/* FIXME this handles wakeup irqs wrong */
	if (enable_irq_wake(nIrq) == 0) {
		sunxi_hcd->irq_wake = 1;
		device_init_wakeup(dev, 1);
	} else {
		sunxi_hcd->irq_wake = 0;
	}

	DMSG_INFO_HCD0("sunxi_hcd_init_controller: %s: USB %s mode controller at %p using %s, IRQ %d\n",
		sunxi_hcd_driver_name,
		"Host",
		ctrl,
		is_hcd_support_dma(sunxi_hcd->usbc_no) ? "DMA" : "PIO",
		sunxi_hcd->nIrq);

	/* host side needs more setup, except for no-host modes */
	if (sunxi_hcd->board_mode != SW_HCD_PERIPHERAL) {
		struct usb_hcd	*hcd = sunxi_hcd_to_hcd(sunxi_hcd);

		hcd->power_budget = 2 * (plat->power ? : 250);
	}

	/* For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	if (is_host_enabled(sunxi_hcd)) {
		SUNXI_HCD_HST_MODE(sunxi_hcd);
		sunxi_hcd->enable = 1;
		status = usb_add_hcd(sunxi_hcd_to_hcd(sunxi_hcd), -1, 0);
		if (status) {
			DMSG_PANIC("ERR: usb_add_hcd failed\n");
			goto fail;
		}
	}

	sunxi_hcd->init_controller = 0;
	return 0;

fail2:
	if (sunxi_hcd->sunxi_hcd_dma.dma_hdle < 0) {
		sunxi_hcd_dma_remove(sunxi_hcd);
	}

	sunxi_hcd_platform_exit(sunxi_hcd);

fail:
	DMSG_PANIC("ERR: sunxi_hcd_init_controller failed with status %d\n", status);

	device_init_wakeup(dev, 0);
	sunxi_hcd_free(sunxi_hcd);

	sunxi_hcd->init_controller = 0;
	return status;
}

int sunxi_usb_host0_enable(void)
{
#if CONFIG_USB_SUNXI_USB0_OTG
	struct platform_device 	*pdev 	= NULL;
	struct device   	*dev  	= NULL;
	struct sunxi_hcd 	*sunxi_hcd = NULL;
	unsigned long   	flags 	= 0;

	DMSG_INFO_HCD0("sunxi_usb_host0_enable start\n");

	pdev = g_hcd0_pdev;
	if (pdev == NULL) {
		DMSG_PANIC("ERR: pdev is null\n");
		return -1;
	}

	dev = &pdev->dev;
	if (dev == NULL) {
		DMSG_PANIC("ERR: dev is null\n");
		return -1;
	}

	sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: sunxi_hcd is null\n");
		return -1;
	}

	g_sunxi_hcd0 = sunxi_hcd;

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd->enable = 1;
	sunxi_hcd->is_connected = 0;
	sunxi_hcd->is_reset     = 0;
	sunxi_hcd->is_suspend   = 0;
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	/* request usb irq */
	INIT_WORK(&sunxi_hcd->irq_work, sunxi_hcd_irq_work);

	if (request_irq(sunxi_hcd->nIrq, sunxi_hcd->isr, 0, dev_name(dev), sunxi_hcd)) {
		DMSG_PANIC("ERR: request_irq %d failed!\n", sunxi_hcd->nIrq);
		return -1;
	}

	sunxi_hcd_soft_disconnect(sunxi_hcd);
	sunxi_hcd_io_init(usbc_no, pdev, &g_sunxi_hcd_io);


	/* enable usb controller */
	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_platform_init(sunxi_hcd);
	sunxi_hcd_restore_context(sunxi_hcd);
	sunxi_hcd_start(sunxi_hcd);
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
	/* port power on */
	sunxi_set_regulator_io(sunxi_hcd, 1);
	sunxi_hcd_set_vbus(sunxi_hcd, 1);

	DMSG_INFO_HCD0("sunxi_usb_host0_enable end\n");
#endif
	return 0;
}
EXPORT_SYMBOL(sunxi_usb_host0_enable);

static void sunxi_hcd_wait_for_disconnect(struct sunxi_hcd *sunxi_hcd)
{
	int cnt = 0;

	while(cnt < 500) {
		/* 1. disconnect
		 * 2. not reset
		 * 3. not suspend */
		if (!sunxi_hcd->is_connected && !sunxi_hcd->is_reset && !sunxi_hcd->is_suspend) {
			break;
		}

		cnt++;
		msleep(1);
	}

	DMSG_INFO_HCD0("sunxi_hcd_wait_for_disconnect cnt=%d\n", cnt);
}

int sunxi_usb_host0_disable(void)
{
#if CONFIG_USB_SUNXI_USB0_OTG
	struct platform_device 	*pdev 	= NULL;
	struct sunxi_hcd 	*sunxi_hcd = NULL;
	unsigned long   	flags 	= 0;

	DMSG_INFO_HCD0("sunxi_usb_host0_disable start\n");

	pdev = g_hcd0_pdev;
	if (pdev == NULL) {
		DMSG_PANIC("ERR: pdev is null\n");
		return -1;
	}

	sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: sunxi_hcd is null\n");
		return -1;
	}

	if (sunxi_hcd->suspend) {
		DMSG_PANIC("wrn: sunxi_hcd is suspend, can not disable\n");
		return -EBUSY;
	}

	sunxi_hcd_wait_for_disconnect(sunxi_hcd);

	/* nuke all urb and disconnect */
	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd->enable = 0;
	sunxi_hcd_soft_disconnect(sunxi_hcd);
	//sunxi_hcd_port_suspend_ex(sunxi_hcd);
	sunxi_hcd_stop(sunxi_hcd);
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
	sunxi_hcd_set_vbus(sunxi_hcd, 0);
	sunxi_set_regulator_io(sunxi_hcd, 0);

	/* release usb irq */
	if (sunxi_hcd->nIrq >= 0) {
		if (sunxi_hcd->irq_wake) {
			disable_irq_wake(sunxi_hcd->nIrq);
		}

		free_irq(sunxi_hcd->nIrq, sunxi_hcd);
	}

	/* disable usb controller */
	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_save_context(sunxi_hcd);
	sunxi_hcd_platform_exit(sunxi_hcd);
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	sunxi_hcd_io_exit(usbc_no, pdev, &g_sunxi_hcd_io);

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	//sunxi_hcd->enable = 0;
	g_sunxi_hcd0 = NULL;
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	DMSG_INFO_HCD0("sunxi_usb_host0_disable end\n");
#endif
	return 0;
}
EXPORT_SYMBOL(sunxi_usb_host0_disable);

/*
 * all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 *
 */
static int sunxi_hcd_probe_otg(struct platform_device *pdev)
{
	struct device   *dev    = &pdev->dev;
	int             irq     = SUNXI_IRQ_USBOTG; //platform_get_irq(pdev, 0);
	__s32 		ret 	= 0;
	__s32 		status	= 0;
	struct sunxi_hcd *sunxi_hcd = NULL;

	if (irq == 0) {
		DMSG_PANIC("ERR: platform_get_irq failed\n");
		return -ENODEV;
	}

	g_hcd0_pdev = pdev;
	usbc_no = 0;

	memset(&g_sunxi_hcd_io, 0, sizeof(sunxi_hcd_io_t));
	ret = sunxi_hcd_io_init(usbc_no, pdev, &g_sunxi_hcd_io);
	if (ret != 0) {
		DMSG_PANIC("ERR: sunxi_hcd_io_init failed\n");
		status = -ENODEV;
		goto end;
	}

	request_usb_regulator(&g_sunxi_hcd_io);

	ret = sunxi_hcd_init_controller(dev, irq, g_sunxi_hcd_io.usb_vbase);
	if (ret != 0) {
		DMSG_PANIC("ERR: sunxi_hcd_init_controller failed\n");
		status = -ENODEV;
		goto end;
	}

	device_create_file(&pdev->dev, &dev_attr_otg_ed_test);

	sunxi_hcd = dev_to_sunxi_hcd(dev);
	if(sunxi_hcd != NULL){
		sunxi_set_regulator_io(sunxi_hcd, 1);
	}

	ret = sunxi_usb_host0_disable();
	if (ret != 0) {
		DMSG_PANIC("ERR: sunxi_usb_host0_disable failed\n");
		status = -ENODEV;
		goto end;
	}

#ifdef CONFIG_ARCH_SUN8IW6
{
	struct sunxi_hcd_platform_data *pdata = pdev->dev.platform_data;
	if(pdata->config->port_info->port_type == USB_PORT_TYPE_HOST){
		complete(&hcd_complete_notify);
	}
}
#endif

end:
	return status;
}

static int sunxi_hcd_remove_otg(struct platform_device *pdev)
{
	struct sunxi_hcd *sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);

	sunxi_hcd_shutdown(pdev);
	if (sunxi_hcd->board_mode == SW_HCD_HOST) {
		usb_remove_hcd(sunxi_hcd_to_hcd(sunxi_hcd));
	}

	device_remove_file(&pdev->dev, &dev_attr_otg_ed_test);

	sunxi_hcd_free(sunxi_hcd);
	device_init_wakeup(&pdev->dev, 0);

	pdev->dev.dma_mask = NULL;

	release_usb_regulator(&g_sunxi_hcd_io);

	sunxi_hcd_io_exit(usbc_no, pdev, &g_sunxi_hcd_io);
	g_hcd0_pdev = NULL;
	usbc_no = 0;

	return 0;
}

/*
 * all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 *
 */
#ifndef CONFIG_ARCH_SUN8IW6
static int sunxi_hcd_probe_host_only(struct platform_device *pdev)
{
	struct device   *dev    = &pdev->dev;
	struct sunxi_hcd *sunxi_hcd = NULL;
	int             irq     = SUNXI_IRQ_USBOTG; //platform_get_irq(pdev, 0);
	__s32 		ret 	= 0;

	if (irq == 0) {
		DMSG_PANIC("ERR: platform_get_irq failed\n");
		return -ENODEV;
	}

	usbc_no = 0;

	memset(&g_sunxi_hcd_io, 0, sizeof(sunxi_hcd_io_t));
	ret = sunxi_hcd_io_init(usbc_no, pdev, &g_sunxi_hcd_io);
	if (ret != 0) {
		DMSG_PANIC("ERR: sunxi_hcd_io_init failed\n");
		return -ENODEV;
	}

	ret = sunxi_hcd_init_controller(dev, irq, g_sunxi_hcd_io.usb_vbase);
	if (ret != 0) {
		DMSG_PANIC("ERR: sunxi_hcd_init_controller failed\n");
		return -ENODEV;
	}

	device_create_file(&pdev->dev, &dev_attr_otg_ed_test);

	request_usb_regulator(&g_sunxi_hcd_io);

	sunxi_hcd = dev_to_sunxi_hcd(dev);
	if(sunxi_hcd != NULL){
		sunxi_set_regulator_io(sunxi_hcd, 1);
		sunxi_hcd_set_vbus(sunxi_hcd, 1);
	}

#ifdef  CONFIG_USB_SUNXI_USB0_HOST
	if (!g_sunxi_hcd_io.host_init_state) {
		sunxi_usb_disable_hcd0();
	}
#endif
	return 0;
}

static int sunxi_hcd_remove_host_only(struct platform_device *pdev)
{
	struct sunxi_hcd     *sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);

	sunxi_hcd_shutdown(pdev);
	if (sunxi_hcd->board_mode == SW_HCD_HOST) {
		usb_remove_hcd(sunxi_hcd_to_hcd(sunxi_hcd));
	}

	sunxi_hcd->enable = 0;
	g_sunxi_hcd0 = NULL;

	device_remove_file(&pdev->dev, &dev_attr_otg_ed_test);

	sunxi_hcd_free(sunxi_hcd);
	device_init_wakeup(&pdev->dev, 0);

	pdev->dev.dma_mask = NULL;

	release_usb_regulator(&g_sunxi_hcd_io);

	sunxi_hcd_io_exit(usbc_no, pdev, &g_sunxi_hcd_io);

	return 0;
}

#endif
static int __init sunxi_hcd_probe(struct platform_device *pdev)
{
#ifdef  CONFIG_USB_SUNXI_USB0_OTG
#ifdef CONFIG_ARCH_SUN8IW6
	return sunxi_hcd_probe_otg(pdev);
#else

	struct sunxi_hcd_platform_data	*pdata = pdev->dev.platform_data;

	switch(pdata->config->port_info->port_type) {
	case USB_PORT_TYPE_HOST:
		return sunxi_hcd_probe_host_only(pdev);
		//break;
	case USB_PORT_TYPE_OTG:
		return sunxi_hcd_probe_otg(pdev);
		//break;
	default:
		DMSG_PANIC("ERR: unkown port_type(%d)\n", pdata->config->port_info->port_type);
	}

	return 0;
#endif
#else
	return sunxi_hcd_probe_host_only(pdev);
#endif
}

static int __exit sunxi_hcd_remove(struct platform_device *pdev)
{
#ifdef  CONFIG_USB_SUNXI_USB0_OTG
#ifdef CONFIG_ARCH_SUN8IW6
	return sunxi_hcd_remove_otg(pdev);
#else
	struct sunxi_hcd_platform_data	*pdata = pdev->dev.platform_data;

	switch(pdata->config->port_info->port_type) {
	case USB_PORT_TYPE_HOST:
		return sunxi_hcd_remove_host_only(pdev);
		//break;
	case USB_PORT_TYPE_OTG:
		return sunxi_hcd_remove_otg(pdev);
		//break;
	default:
		DMSG_PANIC("ERR: unkown port_type(%d)\n", pdata->config->port_info->port_type);
	}

	return 0;
#endif
#else
	return sunxi_hcd_remove_host_only(pdev);
#endif
}

/* note: only save ep status regs */
static void sunxi_hcd_save_context(struct sunxi_hcd *sunxi_hcd)
{
	int i = 0;
	void __iomem *sunxi_hcd_base = sunxi_hcd->mregs;

	/* Common Register */
	for(i = 0; i < SW_HCD_C_NUM_EPS; i++) {
		USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, i);

		if (i == 0) {
			sunxi_hcd_context.ep_reg[i].USB_CSR0 = USBC_Readl(USBC_REG_EX_USB_CSR0(sunxi_hcd_base));
		} else {
			sunxi_hcd_context.ep_reg[i].USB_TXCSR = USBC_Readl(USBC_REG_EX_USB_TXCSR(sunxi_hcd_base));
			sunxi_hcd_context.ep_reg[i].USB_RXCSR	= USBC_Readl(USBC_REG_EX_USB_RXCSR(sunxi_hcd_base));
		}

		if (i == 0) {
			sunxi_hcd_context.ep_reg[i].USB_ATTR0 = USBC_Readl(USBC_REG_EX_USB_ATTR0(sunxi_hcd_base));
		} else {
			sunxi_hcd_context.ep_reg[i].USB_EPATTR = USBC_Readl(USBC_REG_EX_USB_EPATTR(sunxi_hcd_base));
			sunxi_hcd_context.ep_reg[i].USB_TXFIFO	= USBC_Readl(USBC_REG_EX_USB_TXFIFO(sunxi_hcd_base));
			sunxi_hcd_context.ep_reg[i].USB_RXFIFO	= USBC_Readl(USBC_REG_EX_USB_RXFIFO(sunxi_hcd_base));
		}

		sunxi_hcd_context.ep_reg[i].USB_TXFADDR	= USBC_Readl(USBC_REG_EX_USB_TXFADDR(sunxi_hcd_base));
		if (i != 0) {
			sunxi_hcd_context.ep_reg[i].USB_RXFADDR	= USBC_Readl(USBC_REG_EX_USB_RXFADDR(sunxi_hcd_base));
		}
	}

	return;
}

/* note: only save ep status regs */
static void sunxi_hcd_restore_context(struct sunxi_hcd *sunxi_hcd)
{
	int i = 0;
	void __iomem *sunxi_hcd_base = sunxi_hcd->mregs;

	/* Common Register */
	for(i = 0; i < SW_HCD_C_NUM_EPS; i++) {
		USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, i);

		if (i == 0) {
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_CSR0, USBC_REG_EX_USB_CSR0(sunxi_hcd_base));
		} else {
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_TXCSR, USBC_REG_EX_USB_TXCSR(sunxi_hcd_base));
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_RXCSR, USBC_REG_EX_USB_RXCSR(sunxi_hcd_base));
		}

		if (i == 0) {
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_ATTR0, USBC_REG_EX_USB_ATTR0(sunxi_hcd_base));
		} else {
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_EPATTR, USBC_REG_EX_USB_EPATTR(sunxi_hcd_base));
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_TXFIFO, USBC_REG_EX_USB_TXFIFO(sunxi_hcd_base));
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_RXFIFO, USBC_REG_EX_USB_RXFIFO(sunxi_hcd_base));
		}

		USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_TXFADDR, USBC_REG_EX_USB_TXFADDR(sunxi_hcd_base));
		if (i != 0) {
			USBC_Writel(sunxi_hcd_context.ep_reg[i].USB_RXFADDR, USBC_REG_EX_USB_RXFADDR(sunxi_hcd_base));
		}
	}

	return ;
}

int sunxi_usb_disable_hcd0(void)
{
#ifdef  CONFIG_USB_SUNXI_USB0_HOST
	struct device *dev = NULL;
	struct platform_device *pdev = NULL;
	unsigned long flags = 0;
	struct sunxi_hcd *sunxi_hcd = NULL;

	DMSG_INFO_HCD0("sunxi_usb_disable_hcd0 start, clk_is_open = %d\n",
		sunxi_hcd->sunxi_hcd_io->clk_is_open);

	if (!g_sunxi_hcd0) {
		DMSG_PANIC("WRN: hcd is disable, g_sunxi_hcd0 is null\n");
		return 0;
	}

	dev    = g_sunxi_hcd0->controller;
	pdev   = to_platform_device(dev);
	sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: sunxi_hcd is null\n");
		return 0;
	}

#ifndef  CONFIG_USB_SUNXI_USB0_HOST
	if (sunxi_hcd->config->port_info->port_type != USB_PORT_TYPE_HOST
		|| sunxi_hcd->config->port_info->host_init_state) {
		DMSG_PANIC("ERR: only host mode support sunxi_usb_disable_hcd, (%d, %d)\n",
			sunxi_hcd->config->port_info->port_type,
			sunxi_hcd->config->port_info->host_init_state);
		return 0;
	}
#endif

	if (!sunxi_hcd->enable) {
		DMSG_PANIC("WRN: hcd is disable, can not enter to disable again\n");
		return 0;
	}

	if (!sunxi_hcd->sunxi_hcd_io->clk_is_open) {
		DMSG_PANIC("ERR: sunxi_usb_disable_hcd0, usb clock is close, can't close again\n");
		return 0;
	}

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_port_suspend_ex(sunxi_hcd);
	sunxi_hcd_stop(sunxi_hcd);
	sunxi_hcd_save_context(sunxi_hcd);
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
	sunxi_hcd_set_vbus(sunxi_hcd, 0);
	sunxi_set_regulator_io(sunxi_hcd, 0);

	close_usb_clock(sunxi_hcd->sunxi_hcd_io);

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_soft_disconnect(sunxi_hcd);
	sunxi_hcd->enable = 0;
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	DMSG_INFO_HCD0("sunxi_usb_disable_hcd0 end\n");
#endif

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_disable_hcd0);

int sunxi_usb_enable_hcd0(void)
{
#ifdef  CONFIG_USB_SUNXI_USB0_HOST
	struct device *dev = NULL;
	struct platform_device *pdev = NULL;
	unsigned long flags = 0;
	struct sunxi_hcd *sunxi_hcd = NULL;

	DMSG_INFO_HCD0("sunxi_usb_enable_hcd0 start, clk_is_open = %d\n",
		sunxi_hcd->sunxi_hcd_io->clk_is_open);

	if (!g_sunxi_hcd0) {
		DMSG_PANIC("WRN: g_sunxi_hcd0 is null\n");
		return 0;
	}

	dev    = g_sunxi_hcd0->controller;
	pdev   = to_platform_device(dev);
	sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);

	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: sunxi_hcd is null\n");
		return 0;
	}

#ifndef  CONFIG_USB_SUNXI_USB0_HOST
	if (sunxi_hcd->config->port_info->port_type != USB_PORT_TYPE_HOST
		|| sunxi_hcd->config->port_info->host_init_state) {
		DMSG_PANIC("ERR: only host mode support sunxi_usb_enable_hcd, (%d, %d)\n",
			sunxi_hcd->config->port_info->port_type,
			sunxi_hcd->config->port_info->host_init_state);
		return 0;
	}
#endif

	if (sunxi_hcd->enable == 1) {
		DMSG_PANIC("WRN: hcd is already enable, can not enable again\n");
		return 0;
	}

	if (sunxi_hcd->sunxi_hcd_io->clk_is_open) {
		DMSG_PANIC("ERR: sunxi_usb_enable_hcd0, usb clock is open, can't open again\n");
		return 0;
	}

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd->enable = 1;
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	open_usb_clock(sunxi_hcd->sunxi_hcd_io);

	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_restore_context(sunxi_hcd);
	sunxi_hcd_start(sunxi_hcd);
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
	/* port power on */
	sunxi_set_regulator_io(sunxi_hcd, 1);
	sunxi_hcd_set_vbus(sunxi_hcd, 1);

	DMSG_INFO_HCD0("sunxi_usb_enable_hcd0 end\n");
#endif

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_enable_hcd0);

#ifdef	CONFIG_PM
static int sunxi_hcd_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long flags = 0;
	struct sunxi_hcd *sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);

	DMSG_INFO_HCD0("sunxi_hcd_suspend start\n");

	atomic_set(&thread_suspend_flag, 1);

	if((sunxi_hcd->sunxi_hcd_io->regulator_id_vbus != NULL) &&  (sunxi_hcd->sunxi_hcd_io->regulator_id_vbus_hdle!= NULL)){
		if(regulator_disable(sunxi_hcd->sunxi_hcd_io->regulator_id_vbus_hdle) < 0){
			DMSG_INFO_HCD0("usb0 id vbus regulator_disable fail\n");
		}
	}

	if (!sunxi_hcd->enable) {
		DMSG_INFO_HCD0("wrn: hcd is disable, need not enter to suspend\n");
		return 0;
	}

	if (!sunxi_hcd->sunxi_hcd_io->clk_is_open) {
		DMSG_INFO_HCD0("wrn: sunxi_hcd_suspend, usb clock is close, can't close again\n");
		return 0;
	}

	if(sunxi_hcd->sunxi_hcd_io->no_suspend){
		scene_lock(&otg_standby_lock);
		enable_wakeup_src(CPUS_USBMOUSE_SRC, 0);

		spin_lock_irqsave(&sunxi_hcd->lock, flags);
		sunxi_hcd->suspend = 1;
		sunxi_hcd_port_suspend_ex(sunxi_hcd);
		spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
	}else{
		spin_lock_irqsave(&sunxi_hcd->lock, flags);
		sunxi_hcd->suspend = 1;
		sunxi_hcd_port_suspend_ex(sunxi_hcd);
		sunxi_hcd_stop(sunxi_hcd);
		sunxi_hcd_save_context(sunxi_hcd);
		spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
		sunxi_set_regulator_io(sunxi_hcd, 0);
		sunxi_hcd_set_vbus(sunxi_hcd, 0);

		close_usb_clock(sunxi_hcd->sunxi_hcd_io);
	}

	DMSG_INFO_HCD0("sunxi_hcd_suspend end\n");
	return 0;
}

static int sunxi_hcd_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long	flags = 0;
	struct usb_hcd	*hcd = NULL;
	struct sunxi_hcd	*sunxi_hcd = dev_to_sunxi_hcd(&pdev->dev);

	DMSG_INFO_HCD0("sunxi_hcd_resume start\n");

	if((sunxi_hcd->sunxi_hcd_io->regulator_id_vbus != NULL) &&  (sunxi_hcd->sunxi_hcd_io->regulator_id_vbus_hdle!= NULL)){
		if(regulator_enable(sunxi_hcd->sunxi_hcd_io->regulator_id_vbus_hdle) < 0){
			DMSG_INFO_HCD0("usb0 id vbus regulator_enable fail\n");
		}
	}

	atomic_set(&thread_suspend_flag, 0);

	if (!sunxi_hcd->enable) {
		DMSG_INFO_HCD0("wrn: hcd is disable, need not resume\n");
		return 0;
	}

	if(sunxi_hcd->sunxi_hcd_io->no_suspend){

		sunxi_hcd_soft_disconnect(sunxi_hcd);

		hcd = sunxi_hcd_to_hcd(sunxi_hcd);
		if(hcd != NULL){
			usb_root_hub_lost_power(hcd->self.root_hub);
		}
		sunxi_hcd->suspend = 0;
	}else{
	if (sunxi_hcd->sunxi_hcd_io->clk_is_open) {
		DMSG_INFO_HCD0("wrn: sunxi_hcd_suspend, usb clock is open, can't open again\n");
		return 0;
	}

	sunxi_hcd_soft_disconnect(sunxi_hcd);
	open_usb_clock(sunxi_hcd->sunxi_hcd_io);

	hcd = sunxi_hcd_to_hcd(sunxi_hcd);
	if(hcd != NULL){
		usb_root_hub_lost_power(hcd->self.root_hub);
	}
	spin_lock_irqsave(&sunxi_hcd->lock, flags);
	sunxi_hcd_restore_context(sunxi_hcd);
	sunxi_hcd_start(sunxi_hcd);
		sunxi_hcd->suspend = 0;
		spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
		/* port power on */
		sunxi_set_regulator_io(sunxi_hcd, 1);
		sunxi_hcd_set_vbus(sunxi_hcd, 1);

	}
	DMSG_INFO_HCD0("sunxi_hcd_resume_early end\n");

	return 0;
}

static const struct dev_pm_ops sunxi_hcd_dev_pm_ops = {
	.suspend	= sunxi_hcd_suspend,
	.resume     	= sunxi_hcd_resume,
};

#define SUNXI_HCD_PMOPS 	&sunxi_hcd_dev_pm_ops

#else

#define SUNXI_HCD_PMOPS 	NULL

#endif

static struct platform_driver sunxi_hcd_driver = {
	.driver = {
		.name		= (char *)sunxi_hcd_driver_name,
		.bus		= &platform_bus_type,
		.owner		= THIS_MODULE,
		.pm		= SUNXI_HCD_PMOPS,
	},

	.remove		    = __devexit_p(sunxi_hcd_remove),
	.shutdown	    = sunxi_hcd_shutdown,
};

static int __init sunxi_hcd_init(void)
{
	DMSG_INFO_HCD0("usb host driver initialize........\n");

	if (usb_disabled()) {
		DMSG_PANIC("ERR: usb disabled\n");
		return 0;
	}

	return platform_driver_probe(&sunxi_hcd_driver, sunxi_hcd_probe);
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(sunxi_hcd_init);
//fs_initcall(sunxi_hcd_init);

static void __exit sunxi_hcd_cleanup(void)
{
	platform_driver_unregister(&sunxi_hcd_driver);

	DMSG_INFO_HCD0("usb host driver exit........\n");
}

module_exit(sunxi_hcd_cleanup);


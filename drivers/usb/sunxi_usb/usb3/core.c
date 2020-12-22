/*
 * drivers/usb/sunxi_usb/udc/usb3/core.c
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <mach/platform.h>
#include <linux/regulator/consumer.h>

#include "core.h"
#include "gadget.h"
#include "io.h"
#include "osal.h"

#include "debug.h"
#include <linux/kthread.h>

static char *maximum_speed = "super";
module_param(maximum_speed, charp, 0);
MODULE_PARM_DESC(maximum_speed, "Maximum supported speed.");

static struct platform_device *sunxi_otg_pdev = NULL;
static u32 otgc_work_mode = 0;
static int first_host_probe = 1;
extern atomic_t thread_suspend_flag;

int sunxi_xhci_disable(void);
int sunxi_xhci_enable(void);

static int __devinit sunxi_core_init(struct sunxi_otgc *otgc );
static void sunxi_core_exit(struct sunxi_otgc *otgc );
#ifdef CONFIG_USB_XHCI_ENHANCE
#define  USBC_Readl(base, offset)			readl(base + offset)
#define  USBC_Writel(base, offset, value)		writel(value, base + offset)
static int sunxi_host_set_vbus(struct sunxi_otgc *otgc, int is_on);
static int usb_connect_flag = 0;
static int no_device_flag = 0;
static int xhci_enable = 0;
static int xhci_scan_thread_run = 0;
static int xhci_scan_thread_exit = 0;
static int xhci_scan_thread(void * pArg);
static struct task_struct *xhci_th = NULL;
static struct completion xhci_complete_notify;
static struct completion xhci_thread_started;
atomic_t xhci_thread_suspend_flag;
#endif
/* -------------------------------------------------------------------------- */

#define SUNXI_DEVS_POSSIBLE	32

static DECLARE_BITMAP(sunxi_devs, SUNXI_DEVS_POSSIBLE);
#ifdef CONFIG_USB_XHCI_ENHANCE
static int xhci_scan_thread(void * pArg)
{

	allow_signal(SIGTERM);
	complete(&xhci_thread_started);

	xhci_scan_thread_exit = 0;

	while(1){
		if(!xhci_scan_thread_run) {
			break;
		}

		if (!atomic_read(&xhci_thread_suspend_flag)) {
			switch_to_usb2();
		}else{
			printk("%s:%d: ====++++++++++++====\n", __func__, __LINE__);
		}
		msleep(10000);

		if(!xhci_scan_thread_run) {
			break;
		}

		if(usb_connect_flag){
			goto sleep;
		}

		if (!atomic_read(&xhci_thread_suspend_flag)) {
			switch_to_usb3();
		}else{
			printk("%s:%d: ====++++++++++++====\n", __func__, __LINE__);
		}
		msleep(3000);

		if(!xhci_scan_thread_run) {
			break;
		}

		if(usb_connect_flag){
			goto sleep;
		}

		continue;

sleep:
		wait_for_completion(&xhci_complete_notify);
	}

	xhci_scan_thread_exit = 1;

	while(1){
		if(kthread_should_stop()) {
			break;
		}

		msleep(1000);
	}

	return 0;
}

int switch_to_usb3(void)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;
	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc  = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	sunxi_host_set_vbus(otgc, 0);
	USBC_Writel(otgc->regs, 0x10014, 0x43);
	msleep(500);
	sunxi_host_set_vbus(otgc, 1);
	USBC_Writel(otgc->regs, 0x10014, 0x40);

	return 0;
}

int switch_to_usb2(void)
{

	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;
	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc  = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	sunxi_host_set_vbus(otgc, 0);
	USBC_Writel(otgc->regs, 0x10014, 0x43);
	msleep(500);
	sunxi_host_set_vbus(otgc, 1);
	USBC_Writel(otgc->regs, 0x10014, 0x41);

	return 0;
}

int sunxi_usb_get_udev_connect(void)
{
	if(xhci_enable && xhci_scan_thread_run)
		return usb_connect_flag;
	else
		return 1;
}
EXPORT_SYMBOL_GPL(sunxi_usb_get_udev_connect);

void sunxi_usb3_connect(void)
{
	if(xhci_enable && xhci_scan_thread_run){
		if(no_device_flag == 1){
			usb_connect_flag = 1;
			no_device_flag = 0;
		}
	}
	return;
}
EXPORT_SYMBOL_GPL(sunxi_usb3_connect);

void sunxi_usb3_disconnect(void)
{
	if(xhci_enable && xhci_scan_thread_run){
		usb_connect_flag = 0;
		no_device_flag = 1;
		printk("wangjx,connect:%d,device:%d\n", usb_connect_flag, no_device_flag);

		/* wakeup thread */
		complete(&xhci_complete_notify);
	}

	return;
}
EXPORT_SYMBOL_GPL(sunxi_usb3_disconnect);


int xhci_scan_init(void)
{

	init_completion(&xhci_complete_notify);
	init_completion(&xhci_thread_started);

	usb_connect_flag = 0;
	no_device_flag = 1;
	xhci_scan_thread_run = 1;
	xhci_scan_thread_exit = 0;

	atomic_set(&xhci_thread_suspend_flag, 0);
	xhci_th = kthread_create(xhci_scan_thread, NULL, "xhci-scan");
	if (IS_ERR(xhci_th)) {
		DMSG_PANIC("ERR: kthread_create xhci_scan_thread failed\n");
		return -1;
	}

	xhci_enable = 1;

	wake_up_process(xhci_th);
	wait_for_completion(&xhci_thread_started);

	return 0;
}

int xhci_scan_exit(void)
{
	xhci_enable = 0;
	if (xhci_th){
		int cnt = 100;
		xhci_scan_thread_run = 0;
		while(!xhci_scan_thread_exit && cnt){
			complete(&xhci_complete_notify);
			msleep(100);
			cnt--;
		}
		kthread_stop(xhci_th);
		xhci_th = NULL;
	}

	return 0;
}

#endif

int sunxi_get_device_id(void)
{
	int		id;

again:
	id = find_first_zero_bit(sunxi_devs, SUNXI_DEVS_POSSIBLE);
	if (id < SUNXI_DEVS_POSSIBLE) {
		int old;

		old = test_and_set_bit(id, sunxi_devs);
		if (old)
			goto again;
	} else {
		pr_err("sunxi: no space for new device\n");
		id = -ENOMEM;
	}

	return id;
}
EXPORT_SYMBOL_GPL(sunxi_get_device_id);

void sunxi_put_device_id(int id)
{
	int			ret;

	if (id < 0)
		return;

	ret = test_bit(id, sunxi_devs);
	WARN(!ret, "sunxi: ID %d not in use\n", id);
	clear_bit(id, sunxi_devs);
}
EXPORT_SYMBOL_GPL(sunxi_put_device_id);

void sunxi_set_mode(struct sunxi_otgc *otgc, u32 mode)
{
	char * name;

	sunxi_set_mode_ex(otgc->regs, mode);

	switch(mode){
		case SUNXI_GCTL_PRTCAP_HOST:
			name = "host";
		break;

		case SUNXI_GCTL_PRTCAP_DEVICE:
			name = "device";
		break;

		default:
			name = "NULL";
		break;
	}

	otgc->mode = mode;
	otgc_work_mode = mode;

	DMSG_INFO("sunxi_controller_mode:%s!\n", name);

	return;
}


/**
 * sunxi_free_one_event_buffer - Frees one event buffer
 * @otgc: Pointer to our controller context structure
 * @evt: Pointer to event buffer to be freed
 */
static void sunxi_free_one_event_buffer(struct sunxi_otgc *otgc,
		struct sunxi_otgc_event_buffer *evt)
{
	dma_free_coherent(otgc->dev, evt->length, evt->buf, evt->dma);
	kfree(evt);
}

/**
 * sunxi_alloc_one_event_buffer - Allocates one event buffer structure
 * @otgc: Pointer to our controller context structure
 * @length: size of the event buffer
 *
 * Returns a pointer to the allocated event buffer structure on success
 * otherwise ERR_PTR(errno).
 */
static struct sunxi_otgc_event_buffer *__devinit
sunxi_alloc_one_event_buffer(struct sunxi_otgc *otgc, unsigned length)
{
	struct sunxi_otgc_event_buffer	*evt;

	evt = kzalloc(sizeof(*evt), GFP_KERNEL);
	if (!evt)
		return ERR_PTR(-ENOMEM);

	evt->otgc	= otgc;
	evt->length	= length;
	evt->buf	= dma_alloc_coherent(otgc->dev, length,
			&evt->dma, GFP_KERNEL);
	if (!evt->buf) {
		kfree(evt);
		return ERR_PTR(-ENOMEM);
	}

	return evt;
}

/**
 * sunxi_free_event_buffers - frees all allocated event buffers
 * @otgc: Pointer to our controller context structure
 */
static void sunxi_free_event_buffers(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_event_buffer	*evt;
	int i;

	for (i = 0; i < otgc->num_event_buffers; i++) {
		evt = otgc->ev_buffs[i];
		if (evt)
			sunxi_free_one_event_buffer(otgc, evt);
	}

	kfree(otgc->ev_buffs);
}

/**
 * sunxi_alloc_event_buffers - Allocates @num event buffers of size @length
 * @otgc: pointer to our controller context structure
 * @length: size of event buffer
 *
 * Returns 0 on success otherwise negative errno. In the error case, otgc
 * may contain some buffers allocated but not all which were requested.
 */
static int __devinit sunxi_alloc_event_buffers(struct sunxi_otgc *otgc, unsigned length)
{
	int			num;
	int			i;

	num = SUNXI_NUM_INT(otgc->hwparams.hwparams1);
	otgc->num_event_buffers = num;

	otgc->ev_buffs = kzalloc(sizeof(*otgc->ev_buffs) * num, GFP_KERNEL);
	if (!otgc->ev_buffs) {
		dev_err(otgc->dev, "can't allocate event buffers array\n");
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		struct sunxi_otgc_event_buffer	*evt;

		evt = sunxi_alloc_one_event_buffer(otgc, length);
		if (IS_ERR(evt)) {
			dev_err(otgc->dev, "can't allocate event buffer\n");
			return PTR_ERR(evt);
		}
		otgc->ev_buffs[i] = evt;
	}

	return 0;
}

/**
 * sunxi_event_buffers_setup - setup our allocated event buffers
 * @otgc: pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
static int __devinit sunxi_event_buffers_setup(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_event_buffer	*evt;
	int				n;

	for (n = 0; n < otgc->num_event_buffers; n++) {
		evt = otgc->ev_buffs[n];
		dev_dbg(otgc->dev, "Event buf %p dma %08llx length %d\n",
				evt->buf, (unsigned long long) evt->dma,
				evt->length);

		sunxi_set_gevntadrlo(otgc->regs, n, lower_32_bits(evt->dma));
		sunxi_set_gevntadrhi(otgc->regs, n, upper_32_bits(evt->dma));
		sunxi_set_gevntsiz(otgc->regs, n, evt->length & 0xffff);
		sunxi_set_gevntcount(otgc->regs, n, 0);
	}

	return 0;
}

static void sunxi_event_buffers_cleanup(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_event_buffer	*evt;
	int				n;

	for (n = 0; n < otgc->num_event_buffers; n++) {
		evt = otgc->ev_buffs[n];
		sunxi_event_buffers_cleanup_ex(otgc->regs, n);

	}
}

static void __devinit sunxi_cache_hwparams(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_hwparams	*parms = &otgc->hwparams;

	parms->hwparams0 = sunxi_get_hwparams0(otgc->regs);
	parms->hwparams1 = sunxi_get_hwparams1(otgc->regs);
	parms->hwparams2 = sunxi_get_hwparams2(otgc->regs);
	parms->hwparams3 = sunxi_get_hwparams3(otgc->regs);
	parms->hwparams4 = sunxi_get_hwparams4(otgc->regs);
	parms->hwparams5 = sunxi_get_hwparams5(otgc->regs);
	parms->hwparams6 = sunxi_get_hwparams6(otgc->regs);
	parms->hwparams7 = sunxi_get_hwparams7(otgc->regs);
	parms->hwparams8 = sunxi_get_hwparams8(otgc->regs);
}

/*
*******************************************************************************
*                     open_usb_clock
*
* Description:
*
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static s32 sunxi_open_usb_clock(struct sunxi_otgc *otgc)
{

	DMSG_INFO("start: %s\n", __func__);

	otgc->port_ctl.otg_clk = clk_get(NULL, USBOTG_CLK);
	if (IS_ERR(otgc->port_ctl.otg_clk)){
		DMSG_PANIC("ERR: OPEN otg_clk failed.\n");
	}

	if(clk_prepare_enable(otgc->port_ctl.otg_clk)){
		DMSG_PANIC("ERR:try otg_clk to prepare_enable  failed!\n" );
	}

	otgc->port_ctl.otg_phyclk = clk_get(NULL, "usbotgphy");
	if (IS_ERR(otgc->port_ctl.otg_phyclk)){
		DMSG_PANIC("ERR: OPEN otg_phyclk failed.\n");
	}

	if(clk_prepare_enable(otgc->port_ctl.otg_phyclk)){
		DMSG_PANIC("ERR:try otg_phyclk to prepare_enable  failed!\n" );
	}
	sunxi_otgc_open_phy(otgc->regs);

	udelay(10);
	DMSG_INFO("end: %s\n", __func__);

	return 0;
}
/*
*******************************************************************************
*                     close_usb_clock
*
* Description:
*
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static s32 sunxi_close_usb_clock(struct sunxi_otgc *otgc)
{
	DMSG_INFO("start: %s\n", __func__);

	sunxi_otgc_close_phy(otgc->regs);

	if(otgc->port_ctl.otg_clk != NULL){
		clk_disable_unprepare(otgc->port_ctl.otg_clk);
	}
	if(!IS_ERR(otgc->port_ctl.otg_clk)){
		clk_put(otgc->port_ctl.otg_clk);
		otgc->port_ctl.otg_clk = NULL;
	}

	if(otgc->port_ctl.otg_phyclk != NULL){
		clk_disable_unprepare(otgc->port_ctl.otg_phyclk);
	}
	if(!IS_ERR(otgc->port_ctl.otg_phyclk)){
		clk_put(otgc->port_ctl.otg_phyclk);
		otgc->port_ctl.otg_phyclk = NULL;
	}

	DMSG_INFO("end: %s\n", __func__);

	return 0;
}

static s32 request_usb_regulator_io(struct sunxi_otgc *otgc)
{
	if(otgc->port_ctl.regulator_io != NULL){
		otgc->port_ctl.regulator_io_hdle = regulator_get(NULL, otgc->port_ctl.regulator_io);
		if(IS_ERR(otgc->port_ctl.regulator_io_hdle)) {
			DMSG_PANIC("ERR: some error happen, usb0 regulator_io_hdle fail to get regulator!\n");
			return 0;
		}

		if(regulator_set_voltage(otgc->port_ctl.regulator_io_hdle , otgc->port_ctl.regulator_value, otgc->port_ctl.regulator_value) < 0 ){
			DMSG_PANIC("ERR: usb0 regulator_set_voltage is fail\n");
			regulator_put(otgc->port_ctl.regulator_io_hdle);
			return 0;
		}
	}
	return 0;
}

static s32 release_usb_regulator_io(struct sunxi_otgc *otgc)
{
	if(otgc->port_ctl.regulator_io != NULL){
		regulator_put(otgc->port_ctl.regulator_io_hdle);
	}
	return 0;
}

static int sunxi_pin_init(struct sunxi_otgc *otgc)
{
	int ret = 0;

	script_item_value_type_e type = 0;
	script_item_u item_temp;

	/* usbc drv_vbus */
	type = script_get_item("usbc0", "usb_drv_vbus_gpio", &(otgc->port_ctl.drv_vbus.gpio_set));
	if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
		otgc->port_ctl.drv_vbus.valid = 1;
	} else {
		otgc->port_ctl.drv_vbus.valid  = 0;
		DMSG_PANIC("ERR: get usbc det_vbus failed\n");
	}

	if (otgc->port_ctl.drv_vbus.valid) {
		ret = gpio_request(otgc->port_ctl.drv_vbus.gpio_set.gpio.gpio, "otg_drv_vbus");
		if (ret != 0) {
			DMSG_PANIC("ERR: gpio_request failed\n");
			otgc->port_ctl.drv_vbus.valid = 0;
		} else {
			gpio_direction_output(otgc->port_ctl.drv_vbus.gpio_set.gpio.gpio, 0);
		}
	}

	/* get regulator io information */
	type = script_get_item("usbc0", "usb_regulator_io", &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			DMSG_INFO("get usbc0_regulator is nocare\n");
			otgc->port_ctl.regulator_io = NULL;
		}else{
			otgc->port_ctl.regulator_io = item_temp.str;

			type = script_get_item("usbc0",  "usb_regulator_vol", &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				otgc->port_ctl.regulator_value = item_temp.val;
			}else{
				DMSG_INFO("get usbc0_value is failed\n");
				otgc->port_ctl.regulator_value = 0;
			}
		}
	}else {
		DMSG_INFO("get usbc0_regulator is failed\n");
		otgc->port_ctl.regulator_io = NULL;
	}

	return 0;
}

static int sunxi_pin_exit(struct sunxi_otgc *otgc)
{
	if (otgc->port_ctl.drv_vbus.valid) {
		gpio_free(otgc->port_ctl.drv_vbus.gpio_set.gpio.gpio);
	}
	return 0;
}

static int sunxi_host_set_vbus(struct sunxi_otgc *otgc, int is_on)
{
	DMSG_INFO("Set USB Power %s\n",(is_on ? "ON" : "OFF"));

	if (otgc->port_ctl.drv_vbus.valid) {
		if((otgc->port_ctl.regulator_io != NULL) && (otgc->port_ctl.regulator_io_hdle != NULL)){
			if(is_on){
				if(regulator_enable(otgc->port_ctl.regulator_io_hdle) < 0){
					DMSG_INFO("ERR: usb0 regulator_enable fail\n");
				}

			}else{
				if(regulator_disable(otgc->port_ctl.regulator_io_hdle) < 0){
					DMSG_INFO("ERR: usb0 regulator_disable fail\n");
				}
			}
		}
		__gpio_set_value(otgc->port_ctl.drv_vbus.gpio_set.gpio.gpio, is_on);
	}

	return 0;
}

int sunxi_usb_device_enable(void)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;
	int ret = 0;

	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc  = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	sunxi_open_usb_clock(otgc);

	ret = sunxi_core_init(otgc);
	if (ret) {
		dev_err(&pdev->dev, "wangjx failed to initialize core, %s_%d\n", __func__, __LINE__);
		return ret;
	}
	sunxi_set_mode(otgc, SUNXI_GCTL_PRTCAP_DEVICE);
	sunxi_gadget_enable(otgc);

	DMSG_INFO("otgc_work_mode:%d\n", otgc_work_mode);

    return 0;
}
EXPORT_SYMBOL(sunxi_usb_device_enable);

int sunxi_usb_device_disable(void)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;

	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc  = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	sunxi_gadget_disable(otgc);
	sunxi_set_mode(otgc, 0);
	sunxi_core_exit(otgc);
	sunxi_close_usb_clock(otgc);

	DMSG_INFO("otgc_work_mode:%d\n", otgc_work_mode);
	return 0;
}
EXPORT_SYMBOL(sunxi_usb_device_disable);

/*
*******************************************************************************
*                     sunxi_usb_host0_enable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sunxi_usb_host0_enable(void)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;

	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc  = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	 if(first_host_probe){
		 sunxi_open_usb_clock(otgc);
		 sunxi_set_mode(otgc , SUNXI_GCTL_PRTCAP_HOST);
		 sunxi_host_init(otgc );
		 sunxi_host_set_vbus(otgc, 1);
		 first_host_probe = 0;
		 goto end;
	}

	sunxi_open_usb_clock(otgc);
	sunxi_core_soft_reset(otgc->regs);
	sunxi_set_mode(otgc , SUNXI_GCTL_PRTCAP_HOST);
	sunxi_xhci_enable();
	sunxi_host_set_vbus(otgc, 1);

end:

#ifdef CONFIG_USB_XHCI_ENHANCE
	xhci_scan_init();
#endif
	return 0;
}
EXPORT_SYMBOL(sunxi_usb_host0_enable);

/*
*******************************************************************************
*                     sunxi_usb_host0_disable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sunxi_usb_host0_disable(void)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;

	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc  = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

#ifdef CONFIG_USB_XHCI_ENHANCE
	xhci_scan_exit();
#endif
	sunxi_host_set_vbus(otgc, 0);
	sunxi_xhci_disable();
	sunxi_set_mode(otgc, 0);
	sunxi_close_usb_clock(otgc );

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_host0_disable);

/**
 * sunxi_core_init - Low-level initialization of SUNXI Core
 * @otgc: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
static int __devinit sunxi_core_init(struct sunxi_otgc *otgc)
{
	unsigned long		timeout;
	u32			reg;
	int			ret;

	reg = sunxi_get_gsnpsid(otgc->regs);

	otgc->revision = reg;

	sunxi_core_soft_reset(otgc->regs);

	/* issue device SoftReset too */
	timeout = jiffies + msecs_to_jiffies(500);
	sunxi_set_dctl_csftrst(otgc->regs);
	do {
		if (!sunxi_is_dctl_csftrst(otgc->regs))
			break;

		if (time_after(jiffies, timeout)) {
			dev_err(otgc->dev, "Reset Timed Out\n");
			ret = -ETIMEDOUT;
			goto err0;
		}

		cpu_relax();
	} while (true);

	sunxi_cache_hwparams(otgc);

	sunxi_gctl_int(otgc->regs, otgc->hwparams.hwparams1,  otgc->revision);

	ret = sunxi_alloc_event_buffers(otgc, SUNXI_EVENT_BUFFERS_SIZE);
	if (ret) {
		dev_err(otgc->dev, "failed to allocate event buffers\n");
		ret = -ENOMEM;
		goto err1;
	}

	ret = sunxi_event_buffers_setup(otgc);
	if (ret) {
		dev_err(otgc->dev, "failed to setup event buffers\n");
		goto err1;
	}

	return 0;

err1:
	sunxi_free_event_buffers(otgc);
err0:
	return ret;
}

static void sunxi_core_exit(struct sunxi_otgc *otgc)
{
	sunxi_event_buffers_cleanup(otgc);
	sunxi_free_event_buffers(otgc);
}

#define SUNXI_ALIGN_MASK		(16 - 1)

static int __devinit sunxi_probe(struct platform_device *pdev)
{
	struct device_node	*node = pdev->dev.of_node;
	struct resource		*res;
	struct sunxi_otgc		*otgc;
	struct device		*dev = &pdev->dev;

	int			ret = -ENOMEM;
	int			irq;

	void __iomem		*regs;
	void			*mem;

	mem = devm_kzalloc(dev, sizeof(*otgc) + SUNXI_ALIGN_MASK, GFP_KERNEL);
	if (!mem) {
		dev_err(dev, "not enough memory\n");
		return -ENOMEM;
	}
	otgc = PTR_ALIGN(mem, SUNXI_ALIGN_MASK + 1);
	otgc->mem = mem;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing resource\n");
		return -ENODEV;
	}

	otgc->res = res;

	res = devm_request_mem_region(dev, res->start, resource_size(res),
			dev_name(dev));
	if (!res) {
		dev_err(dev, "can't request mem region\n");
		return -ENOMEM;
	}

	regs = devm_ioremap(dev, res->start, resource_size(res));

	if (!regs) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}


	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "missing IRQ\n");
		return -ENODEV;
	}

	spin_lock_init(&otgc ->lock);

	otgc->regs	= regs;
	otgc->regs_size	= resource_size(res);
	otgc->dev	= dev;
	otgc->irq	= irq;

	if (!strncmp("super", maximum_speed, 5))
		otgc->maximum_speed = SUNXI_DCFG_SUPERSPEED;
	else if (!strncmp("high", maximum_speed, 4))
		otgc->maximum_speed = SUNXI_DCFG_HIGHSPEED;
	else if (!strncmp("full", maximum_speed, 4))
		otgc->maximum_speed = SUNXI_DCFG_FULLSPEED1;
	else if (!strncmp("low", maximum_speed, 3))
		otgc->maximum_speed = SUNXI_DCFG_LOWSPEED;
	else
		otgc->maximum_speed = SUNXI_DCFG_SUPERSPEED;

	if (of_get_property(node, "tx-fifo-resize", NULL))
		otgc->needs_fifo_resize = true;

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	pm_runtime_forbid(dev);

	sunxi_open_usb_clock(otgc);

#ifndef  SUNXI_USB_FPGA
	sunxi_pin_init(otgc);
	request_usb_regulator_io(otgc);
#endif
	ret = sunxi_core_init(otgc);
	if (ret) {
		dev_err(dev, "failed to initialize core\n");
		 return ret;
	}

	sunxi_set_mode(otgc , SUNXI_GCTL_PRTCAP_DEVICE);
	sunxi_gadget_init(otgc );

	ret = sunxi_debugfs_init(otgc);
	if (ret) {
		dev_err(dev, "failed to initialize debugfs\n");
		goto err1;
	}

	pm_runtime_allow(dev);

	platform_set_drvdata(pdev, otgc);
	sunxi_otg_pdev = pdev;
	sunxi_usb_device_disable();
	return 0;

err1:
	sunxi_core_exit(otgc);

	return ret;
}

static int __devexit sunxi_remove(struct platform_device *pdev)
{
	struct sunxi_otgc	*otgc = platform_get_drvdata(pdev);
	struct resource	*res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	sunxi_debugfs_exit(otgc);
#ifndef  SUNXI_USB_FPGA
	sunxi_pin_exit(otgc);
	release_usb_regulator_io(otgc);
#endif

	sunxi_host_exit(otgc);
	sunxi_gadget_exit(otgc);

	sunxi_core_exit(otgc);
	sunxi_otg_pdev = NULL;

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_otg_suspend(struct device *dev)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;

	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	DMSG_INFO("start: %s, otgc_work_mode: %d\n", __func__, otgc_work_mode);
	atomic_set(&thread_suspend_flag, 1);

	switch(otgc_work_mode){
		case SUNXI_GCTL_PRTCAP_HOST:
			sunxi_host_set_vbus(otgc, 0);
		break;

		case SUNXI_GCTL_PRTCAP_DEVICE:
			sunxi_gadget_suspend(otgc);
			sunxi_core_exit(otgc);
		break;

		default:
			DMSG_INFO("end:%s, otgc driver is no work and no need suspend\n", __func__);
		return 0;
	}

	sunxi_close_usb_clock(otgc);

	DMSG_INFO("end: %s\n", __func__);

	return 0;
}

static int sunxi_otg_resume(struct device *dev)
{
	struct platform_device *pdev = NULL;
	struct sunxi_otgc *otgc  = NULL;
	int ret;

	pdev = sunxi_otg_pdev;
	if(pdev == NULL){
		DMSG_PANIC("%s, pdev is NULL\n", __func__);
		return 0;
	}

	otgc = platform_get_drvdata(pdev);
	if(otgc == NULL){
		DMSG_PANIC("%s, otgc is NULL\n", __func__);
		return 0;
	}

	DMSG_INFO("start:%s, otgc_work_mode:%d\n", __func__, otgc_work_mode);

	if(otgc_work_mode == 0){
		DMSG_INFO("end:%s, otgc driver is no work and no need resume\n", __func__);
		atomic_set(&thread_suspend_flag, 0);
		goto end;
	}

	sunxi_open_usb_clock(otgc);

	sunxi_set_mode(otgc, otgc_work_mode);
	switch(otgc_work_mode){
		case SUNXI_GCTL_PRTCAP_HOST:
			sunxi_core_soft_reset(otgc->regs);
			sunxi_host_set_vbus(otgc, 1);
		break;

		case SUNXI_GCTL_PRTCAP_DEVICE:
			ret = sunxi_core_init(otgc);
			if (ret) {
				dev_err(&pdev->dev, "failed to initialize core, %s_%d\n", __func__, __LINE__);
				atomic_set(&thread_suspend_flag, 0);
				return ret;
			}
			sunxi_gadget_resume(otgc);
			atomic_set(&thread_suspend_flag, 0);
		break;

		default:
		return 0;
	}
end:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	DMSG_INFO("end: %s\n", __func__);

	return 0;
}

static const struct dev_pm_ops sunxi_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sunxi_otg_suspend, sunxi_otg_resume)
};

#define SUNXI_PM_OPS	&(sunxi_dev_pm_ops)
#else
#define SUNXI_PM_OPS	NULL
#endif


static struct platform_driver sunxi_driver = {
	.probe		= sunxi_probe,
	.remove		= __devexit_p(sunxi_remove),
	.driver		= {
		.name	= "sunxi_otg",
		.pm	= SUNXI_PM_OPS,
	},
};

module_platform_driver(sunxi_driver);

MODULE_ALIAS("platform:sunxi_otg");
MODULE_AUTHOR("wangjx");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("SUNXI USB3.0 Controller Driver");

/*
 * drivers/usb/sunxi_usb/manager/usb_manager.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb manager.
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
#include <linux/kthread.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <linux/gpio.h>

#include  "../include/sunxi_usb_config.h"
#include  "usb_manager.h"
#include  "usbc_platform.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"

struct usb_cfg g_usb_cfg;
__u32 thread_device_run_flag = 0;
__u32 thread_host_run_flag = 0;

#ifdef CONFIG_ARCH_SUN8IW6
struct completion hcd_complete_notify;
struct completion udc_complete_notify;
#endif

#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
struct completion udc_complete_notify;
#endif

__u32 thread_run_flag = 1;
static __u32 thread_stopped_flag = 1;
atomic_t thread_suspend_flag;

#ifndef  SUNXI_USB_FPGA
#define BUFLEN 32

static __s32 ctrlio_status = -1;

static int get_battery_status(void)
{
	struct file *filep;
	loff_t pos;
	char buf[BUFLEN];

	filep=filp_open("/sys/class/power_supply/battery/present", O_RDONLY, 0);
	if (IS_ERR(filep)) {
		DMSG_PANIC("open present fail\n");
		return 0;
	}

	pos = 0;
	vfs_read(filep, (char __user *)buf, BUFLEN, &pos);
	filp_close(filep, NULL);

	if (!strncmp((const char *)buf, "0", 1))
		return 0;
	else
		return 1;
}

static int get_charge_status(void)
{
	struct file *filep;
	loff_t pos;
	char buf[BUFLEN];

	filep=filp_open("/sys/class/power_supply/battery/status", O_RDONLY, 0);
	if (IS_ERR(filep)) {
		DMSG_PANIC("open status fail\n");
		return 0;
	}

	pos = 0;
	vfs_read(filep, (char __user *)buf, BUFLEN, &pos);
	filp_close(filep, NULL);

	if (!strncmp((const char *)buf, "Charging", 8) ||
			!strncmp((const char *)buf, "Full", 4))
		return 1;
	else
		return 0;
}

static int get_voltage(void)
{
	struct file *filep;
	loff_t pos;
	char buf[BUFLEN];
	int ret, voltage;

	filep=filp_open("/sys/class/power_supply/battery/voltage_now", O_RDONLY, 0);
	if (IS_ERR(filep)) {
		DMSG_PANIC("open voltage fail\n");
		return 0;
	}

	pos = 0;
	vfs_read(filep, (char __user *)buf, BUFLEN, &pos);
	filp_close(filep, NULL);

	ret = sscanf(buf, "%d\n", &voltage);
	if (ret != 1)
		return 0;
	else
		return voltage;
}

static int get_capacity(void)
{
	struct file *filep;
	loff_t pos;
	char buf[BUFLEN];
	int ret, capacity;

	filep=filp_open("/sys/class/power_supply/battery/capacity", O_RDONLY, 0);
	if (IS_ERR(filep)) {
		DMSG_PANIC("open capacity fail\n");
		return 0;
	}

	pos = 0;
	vfs_read(filep, (char __user *)buf, BUFLEN, &pos);
	filp_close(filep, NULL);
	ret = sscanf(buf, "%d\n", &capacity);
	if (ret != 1)
		return 0;
	else
		return capacity;
}

static int set_ctrl_gpio(struct usb_cfg *cfg, int is_on)
{
	int ret = 0;

	if (ctrlio_status == is_on)
		return 0;
	if (cfg->port[0].restrict_gpio_set.valid) {
		DMSG_INFO("set ctrl gpio %s\n", is_on ? "on" : "off");
		__gpio_set_value(cfg->port[0].restrict_gpio_set.gpio_set.gpio.gpio, is_on);
	}

	ctrlio_status = is_on;

	return ret;
}

static int pin_init(struct usb_cfg *cfg)
{
	int ret = 0;

	if (cfg->port[0].restrict_gpio_set.valid) {
		ret = gpio_request(cfg->port[0].restrict_gpio_set.gpio_set.gpio.gpio, "usb_restrict");
		if (ret != 0) {
			DMSG_PANIC("ERR: usb_restrict gpio_request failed\n");
			cfg->port[0].restrict_gpio_set.valid = 1;
		} else {
			/* set config, ouput */
			//sunxi_gpio_setcfg(cfg->port[0].restrict_gpio_set.gpio_set.gpio.gpio, 1);

			/* reserved is pull down */
			//sunxi_gpio_setpull(cfg->port[0].restrict_gpio_set.gpio_set.gpio.gpio, 2);
			gpio_direction_output(cfg->port[0].restrict_gpio_set.gpio_set.gpio.gpio, 0);
		}
	}
	return 0;
}

static int pin_exit(struct usb_cfg *cfg)
{
	if (cfg->port[0].restrict_gpio_set.valid) {
		gpio_free(cfg->port[0].restrict_gpio_set.gpio_set.gpio.gpio);
	}
	return 0;
}
#else
static int pin_init(struct usb_cfg *cfg)
{
	return 0;
}

static int pin_exit(struct usb_cfg *cfg)
{
	return 0;
}
#endif

static int usb_device_scan_thread(void * pArg)
{

	/* delay for udc & hcd ready */
	msleep(3000);

	while(thread_device_run_flag) {

		msleep(1000);  /* 1s */
#if defined (CONFIG_ARCH_SUN8IW6) || defined (CONFIG_ARCH_SUN8IW7) || defined (CONFIG_ARCH_SUN8IW8)
		wait_for_completion(&udc_complete_notify);
#endif
		hw_rmmod_usb_host();
		hw_rmmod_usb_device();
		usb_msg_center(&g_usb_cfg);

		hw_insmod_usb_device();
		usb_msg_center(&g_usb_cfg);
		thread_device_run_flag = 0;
		DMSG_INFO("device_chose finished %d!\n",__LINE__);
	}

	return 0;
}

#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)
static int usb_host_scan_thread(void * pArg)
{

	/* delay for udc & hcd ready */
	msleep(3000);

	while(thread_host_run_flag) {

		msleep(1000);  /* 1s */
#ifdef CONFIG_ARCH_SUN8IW6
		wait_for_completion(&hcd_complete_notify);
#endif
		hw_rmmod_usb_host();
		hw_rmmod_usb_device();
		usb_msg_center(&g_usb_cfg);

		hw_insmod_usb_host();
		usb_msg_center(&g_usb_cfg);
		thread_host_run_flag = 0;
		DMSG_INFO("host_chose finished %d!\n",__LINE__);
	}

	return 0;
}
#endif

static int usb_hardware_scan_thread(void * pArg)
{
	struct usb_cfg *cfg = pArg;
#ifndef  SUNXI_USB_FPGA
	__u32 voltage;
	__u32 capacity;
#endif
	/* delay for udc & hcd ready */
	msleep(3000);

	pin_init(cfg);

	while(thread_run_flag) {
		DMSG_DBG_MANAGER("\n\n");

		msleep(1000);  /* 1s */

		if (atomic_read(&thread_suspend_flag))
			continue;
		usb_hw_scan(cfg);
		usb_msg_center(cfg);

		DMSG_DBG_MANAGER("\n\n");


#ifndef  SUNXI_USB_FPGA
		if (cfg->port[0].usb_restrict_flag) {
			if (cfg->port[0].restrict_gpio_set.valid) {
				if (!get_battery_status()) { // battery not exist
					set_ctrl_gpio(cfg, 1);
					continue;
				}

				if (get_charge_status()) { // charging or full
					set_ctrl_gpio(cfg, 1);
					continue;
				}

				voltage = get_voltage();
				capacity = get_capacity();
				if ((voltage > cfg->port[0].voltage) && (capacity > cfg->port[0].capacity)) {
					set_ctrl_gpio(cfg, 1);
				} else {
					set_ctrl_gpio(cfg, 0);
				}
			}
		}
#endif
	}

	thread_stopped_flag = 1;
	return 0;
}

#ifndef  SUNXI_USB_FPGA

static __s32 usb_script_parse(struct usb_cfg *cfg)
{
	u32 i = 0;
	char *set_usbc = NULL;
	script_item_value_type_e type = 0;
	script_item_u item_temp;

	for(i = 0; i < cfg->usbc_num; i++) {
		if (i == 0) {
			set_usbc = SET_USB0;
		} else if (i == 1) {
			set_usbc = SET_USB1;
		} else {
			set_usbc = SET_USB2;
		}

		/* usbc enable */
		type = script_get_item(set_usbc, KEY_USB_ENABLE, &item_temp);
		if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
			cfg->port[i].enable = item_temp.val;
		} else {
			DMSG_INFO("get usbc(%d) enable failed\n", i);
			cfg->port[i].enable = 0;
		}

		/* usbc port type */
		type = script_get_item(set_usbc, KEY_USB_PORT_TYPE, &item_temp);
		if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
			cfg->port[i].port_type = item_temp.val;
		} else {
			DMSG_INFO("get usbc(%d) port type failed\n", i);
			cfg->port[i].port_type = 0;
		}

		/* usbc usb_restrict_flag  */
		type = script_get_item(set_usbc, KEY_USB_USB_RESTRICT_FLAG, &item_temp);
		if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
			cfg->port[i].usb_restrict_flag = item_temp.val;
		} else {
			DMSG_INFO("get usbc(%d) usb_restrict_flag failed\n", i);
			cfg->port[i].usb_restrict_flag = 0;
		}

		/* usbc voltage type */
		type = script_get_item(set_usbc, KEY_USB_USB_RESTRICT_VOLTAGE, &item_temp);
		if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
			cfg->port[i].voltage = item_temp.val;
		} else {
			DMSG_INFO("get usbc(%d) voltage  failed\n", i);
			cfg->port[i].voltage = 0;
		}

		/* usbc capacity type */
		type = script_get_item(set_usbc, KEY_USB_USB_RESTRICT_CAPACITY, &item_temp);
		if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
			cfg->port[i].capacity = item_temp.val;
		} else {
			DMSG_INFO("get usbc(%d)voltage failed\n", i);
			cfg->port[i].capacity = 0;
		}

		/* usbc detect type */
		type = script_get_item(set_usbc, KEY_USB_DETECT_TYPE, &item_temp);
		if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
			cfg->port[i].detect_type = item_temp.val;
		} else {
			DMSG_INFO("get usbc(%d) detect_type  failed\n", i);
			cfg->port[i].detect_type = 0;
		}

		/* usbc id */
		type = script_get_item(set_usbc, KEY_USB_ID_GPIO, &(cfg->port[i].id.gpio_set));
		if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
			cfg->port[i].id.valid = 1;
		} else {
			cfg->port[i].id.valid = 0;
			DMSG_INFO("get usbc(%d) id failed\n", i);
		}

		/* usbc det_vbus */
		type = script_get_item(set_usbc, KEY_USB_DETVBUS_GPIO, &(cfg->port[i].det_vbus.gpio_set));
		if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
			cfg->port[i].det_vbus.valid = 1;
			cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_GIPO;
		} else {
			cfg->port[i].det_vbus.valid = 0;
			cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_NULL;
			DMSG_INFO("no usbc(%d) det_vbus gpio and try to axp det_pin\n", i);
		}

		if (cfg->port[i].det_vbus.valid == 0) {
			type = script_get_item(set_usbc, KEY_USB_DETVBUS_GPIO, &item_temp);
			if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
				if (strncmp(item_temp.str, "axp_ctrl", 8) == 0) {
					cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_AXP;
				} else {
					cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_NULL;
				}
			} else {
				DMSG_INFO("get usbc(%d) det_vbus axp failed\n", i);
				cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_NULL;
			}
		}

		/* usbc drv_vbus */
		type = script_get_item(set_usbc, KEY_USB_DRVVBUS_GPIO, &(cfg->port[i].drv_vbus.gpio_set));
		if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
			cfg->port[i].drv_vbus.valid = 1;
		} else {
			cfg->port[i].drv_vbus.valid = 0;
			DMSG_INFO("get usbc(%d) det_vbus failed\n", i);
		}

		/* usbc usb_restrict */
		type = script_get_item(set_usbc, KEY_USB_RESTRICT_GPIO, &(cfg->port[i].restrict_gpio_set.gpio_set));
		if (type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
			cfg->port[i].restrict_gpio_set.valid = 1;
		} else {
			DMSG_INFO("get usbc0(usb_restrict pin) failed\n");
			cfg->port[i].restrict_gpio_set.valid = 0;
		}
	}

	return 0;
}
#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)
static __s32 check_usb_board_info(struct usb_cfg *cfg)
{
	/* USB0 */
	if (cfg->port[0].enable) {
		/* check if port type valid */
		if (cfg->port[0].port_type != USB_PORT_TYPE_DEVICE
				&& cfg->port[0].port_type != USB_PORT_TYPE_HOST
				&& cfg->port[0].port_type != USB_PORT_TYPE_OTG) {
			DMSG_PANIC("ERR: usbc0 port_type(%d) is unkown\n", cfg->port[0].port_type);
			goto err;
		}

		/* check if USB detect way valid */
		if (cfg->port[0].detect_type != USB_DETECT_TYPE_DP_DM
				&& cfg->port[0].detect_type != USB_DETECT_TYPE_VBUS_ID) {
			DMSG_PANIC("ERR: usbc0 detect_type(%d) is unkown\n", cfg->port[0].detect_type);
			goto err;
		}

		/* if use VBUS/ID for detect, must check id/vbus pin validity */
		if (cfg->port[0].detect_type == USB_DETECT_TYPE_VBUS_ID) {
			if (cfg->port[0].id.valid == 0) {
				DMSG_PANIC("ERR: id pin is invaild\n");
				goto err;
			}

			if (cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_GIPO) {
				if (cfg->port[0].det_vbus.valid == 0) {
					DMSG_PANIC("ERR: det_vbus pin is invaild\n");
					goto err;
				}
			}
		}
	}

	return 0;
err:
	return -1;
}
#endif
static void print_gpio_set(struct gpio_config *gpio)
{
	DMSG_MANAGER_DEBUG("gpio_name            = %s\n", gpio->gpio.gpio);
	DMSG_MANAGER_DEBUG("mul_sel              = %x\n", gpio->gpio.mul_sel);
	DMSG_MANAGER_DEBUG("pull                 = %x\n", gpio->gpio.pull);
	DMSG_MANAGER_DEBUG("drv_level            = %x\n", gpio->gpio.drv_level);
	DMSG_MANAGER_DEBUG("data                 = %x\n", gpio->gpio.data);
}

static void print_usb_cfg(struct usb_cfg *cfg)
{
	u32 i = 0;

	DMSG_MANAGER_DEBUG("\n-----------usb config information--------------\n");

	DMSG_MANAGER_DEBUG("controller number    = %x\n", (u32)USBC_MAX_CTL_NUM);

	DMSG_MANAGER_DEBUG("usb_global_enable    = %x\n", cfg->usb_global_enable);
	DMSG_MANAGER_DEBUG("usbc_num             = %x\n", cfg->usbc_num);

	for(i = 0; i < USBC_MAX_CTL_NUM; i++) {
		DMSG_MANAGER_DEBUG("\n");
		DMSG_MANAGER_DEBUG("port[%d]:\n", i);
		DMSG_MANAGER_DEBUG("enable               = %x\n", cfg->port[i].enable);
		DMSG_MANAGER_DEBUG("port_no              = %x\n", cfg->port[i].port_no);
		DMSG_MANAGER_DEBUG("port_type            = %x\n", cfg->port[i].port_type);
		DMSG_MANAGER_DEBUG("detect_type          = %x\n", cfg->port[i].detect_type);

		DMSG_MANAGER_DEBUG("id.valid             = %x\n", cfg->port[i].id.valid);
		print_gpio_set(&cfg->port[i].id.gpio_set.gpio);

		if (cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_GIPO) {
			DMSG_MANAGER_DEBUG("vbus.valid           = %x\n", cfg->port[i].det_vbus.valid);
			print_gpio_set(&cfg->port[i].det_vbus.gpio_set.gpio);
		}

		DMSG_MANAGER_DEBUG("drv_vbus.valid       = %x\n", cfg->port[i].drv_vbus.valid);
		print_gpio_set(&cfg->port[i].drv_vbus.gpio_set.gpio);

		DMSG_MANAGER_DEBUG("\n");
	}

	DMSG_MANAGER_DEBUG("-------------------------------------------------\n");
}
#else
static __s32 usb_script_parse(struct usb_cfg *cfg)
{
	/* usbc enable */
	cfg->port[0].enable = 1;

	/* usbc port type */
	cfg->port[0].port_type = USB_PORT_TYPE_OTG;

	/* usbc detect type */
	cfg->port[0].detect_type = USB_DETECT_TYPE_VBUS_ID;
	return 0;
}

static __s32 check_usb_board_info(struct usb_cfg *cfg)
{
	if (cfg->port[0].enable) {
		/* check if port type valid */
		if (cfg->port[0].port_type != USB_PORT_TYPE_DEVICE
			&& cfg->port[0].port_type != USB_PORT_TYPE_HOST
			&& cfg->port[0].port_type != USB_PORT_TYPE_OTG) {
			DMSG_PANIC("ERR: usbc0 port_type(%d) is unkown\n", cfg->port[0].port_type);
			goto err;
		}
	}

	return 0;
err:
	return -1;
}

static void print_usb_cfg(struct usb_cfg *cfg)
{
	return;
}
#endif

static __s32 get_usb_cfg(struct usb_cfg *cfg)
{
	__s32 ret = 0;

	/* parse script */
	ret = usb_script_parse(cfg);
	if (ret != 0) {
		DMSG_PANIC("ERR: usb_script_parse failed\n");
		return -1;
	}

	print_usb_cfg(cfg);

#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)
	ret = check_usb_board_info(cfg);
	if (ret != 0) {
		DMSG_PANIC("ERR: check_usb_board_info failed\n");
		return -1;
	}
#endif
	return 0;
}

#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
static int usb_host_init_status(char *usbc)
{
	script_item_value_type_e type = 0;
	script_item_u item_temp;

	type = script_get_item(usbc, KEY_USB_HOST_INIT_STATE, &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_INT) {
		if(item_temp.val){
			return 1;
		}else{
			return 0;
		}
	} else {
		return 0;
	}
}

int usb_otg_id_status(void)
{
	struct usb_cfg *cfg = NULL;
	int id_status = -1;

	cfg = &g_usb_cfg;
	if(cfg == NULL){
		return -1;
	}

	if(cfg->port[0].port_type == USB_PORT_TYPE_DEVICE){
		return 1;
	}

	if (cfg->port[0].port_type != USB_PORT_TYPE_OTG) {
		return -1;
	}
	if(cfg->port[0].id.valid){
		id_status = __gpio_get_value(cfg->port[0].id.gpio_set.gpio.gpio);
	}

	return id_status;
}
EXPORT_SYMBOL(usb_otg_id_status);
#endif

static int __init usb_manager_init(void)
{
	__s32 ret = 0;
	bsp_usbc_t usbc;

	struct task_struct *device_th = NULL;
#if !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)
	struct task_struct *host_th = NULL;
#endif

	struct task_struct *th = NULL;

	DMSG_MANAGER_DEBUG("[sw usb]: usb_manager_init\n");

	memset(&g_usb_cfg, 0, sizeof(struct usb_cfg));
	g_usb_cfg.usb_global_enable = 1;
	g_usb_cfg.usbc_num = 1;
	usb_msg_center_init();

	ret = get_usb_cfg(&g_usb_cfg);
	if (ret != 0) {
		DMSG_PANIC("ERR: get_usb_cfg failed\n");
		return -1;
	}
	if (g_usb_cfg.port[0].enable == 0) {
		DMSG_PANIC("wrn: usb0 is disable\n");
		return 0;
	}

	memset(&usbc, 0, sizeof(bsp_usbc_t));
#ifndef CONFIG_ARCH_SUN9IW1
	usbc.usbc_info[0].base = (__u32 __force)SUNXI_USB_OTG_VBASE;
	usbc.sram_base = (__u32 __force)SUNXI_SRAMCTRL_VBASE;
	USBC_init(&usbc);
#endif
	usbc0_platform_device_init(&g_usb_cfg.port[0]);

	if (g_usb_cfg.port[0].port_type == USB_PORT_TYPE_DEVICE) {

#if defined (CONFIG_ARCH_SUN8IW6) || defined (CONFIG_ARCH_SUN8IW7) || defined (CONFIG_ARCH_SUN8IW8)
		init_completion(&udc_complete_notify);
#endif
		thread_device_run_flag = 1;
		device_th = kthread_create(usb_device_scan_thread, NULL, "usb_device_chose");
		if (IS_ERR(device_th)) {
			DMSG_PANIC("ERR: device kthread_create failed\n");
			return -1;
		}

		wake_up_process(device_th);
	}

	if (g_usb_cfg.port[0].port_type == USB_PORT_TYPE_HOST) {
#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
		if(usb_host_init_status(SET_USB0)){
			set_usb_role_ex(USB_ROLE_HOST);
		}
#else

#ifdef CONFIG_ARCH_SUN8IW6
		init_completion(&hcd_complete_notify);
#endif
		thread_host_run_flag = 1;
		host_th = kthread_create(usb_host_scan_thread, NULL, "usb_host_chose");
		if (IS_ERR(host_th)) {
			DMSG_PANIC("ERR: host kthread_create failed\n");
			return -1;
		}

		wake_up_process(host_th);
#endif
	}

	if (g_usb_cfg.port[0].port_type == USB_PORT_TYPE_OTG
		&& g_usb_cfg.port[0].detect_type == USB_DETECT_TYPE_VBUS_ID) {
		usb_hw_scan_init(&g_usb_cfg);

		atomic_set(&thread_suspend_flag, 0);
		thread_run_flag = 1;
		thread_stopped_flag = 0;
		th = kthread_create(usb_hardware_scan_thread, &g_usb_cfg, "usb-hardware-scan");
		if (IS_ERR(th)) {
			DMSG_PANIC("ERR: kthread_create failed\n");
			return -1;
		}

		wake_up_process(th);
	}

	DMSG_MANAGER_DEBUG("[sw usb]: usb_manager_init end\n");
	return 0;
}

static void __exit usb_manager_exit(void)
{
	bsp_usbc_t usbc;

	DMSG_MANAGER_DEBUG("[sw usb]: usb_manager_exit\n");

	if (g_usb_cfg.port[0].enable == 0) {
		DMSG_PANIC("wrn: usb0 is disable\n");
		return ;
	}

	memset(&usbc, 0, sizeof(bsp_usbc_t));
	usb_msg_center_exit();
	USBC_exit(&usbc);
	if (g_usb_cfg.port[0].port_type == USB_PORT_TYPE_OTG
		&& g_usb_cfg.port[0].detect_type == USB_DETECT_TYPE_VBUS_ID) {
		thread_run_flag = 0;
		while(!thread_stopped_flag) {
			DMSG_INFO("waitting for usb_hardware_scan_thread stop\n");
			msleep(10);
		}
		pin_exit(&g_usb_cfg);
		usb_hw_scan_exit(&g_usb_cfg);
	}

	usbc0_platform_device_exit(&g_usb_cfg.port[0]);
	return;
}

//module_init(usb_manager_init);
fs_initcall(usb_manager_init);
module_exit(usb_manager_exit);


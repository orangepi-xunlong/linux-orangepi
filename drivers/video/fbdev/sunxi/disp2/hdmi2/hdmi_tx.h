/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __INCLUDES_H__
#define __INCLUDES_H__
#include "config.h"
#if defined(__LINUX_PLAT__)

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/types.h>
#include <video/drv_hdmi.h>
#include <linux/sunxi-gpio.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clk/clk-conf.h>
#include <linux/clkdev.h>
#endif
#include "hdmi_core/hdmi_core.h"
#include "hdmi_core/core_cec.h"

#define FUNC_NAME __func__

enum hdmi_ioctl_cmd {
	CMD_NONE = 0,
	HDCP_LOAD_FIRMWARE,
	HDCP_LOAD_FIRMWARE_COMPLETED,
	HDCP_LOAD_KEY,
	CMD_NUM,
};

/**
 * @short Main structures to instantiate the driver
 */
struct hdmi_tx_drv {
#if defined(__LINUX_PLAT__)

	struct platform_device		*pdev;
	/* Device node */
	struct device			*parent_dev;
	struct input_dev		*cec_input_dev;
#endif
	/* Device list */
	struct list_head		devlist;

	/* Interrupts */
	u32				irq;

	/* HDMI TX Controller */
	uintptr_t			reg_base;
#if defined(__LINUX_PLAT__)

	struct pinctrl			*pctl;
#endif
/* hardware require, a gpio have to be set when plugin or out
* in order to enhance the ability of reading edid
*/
	unsigned int			ddc_ctrl_en;
	struct gpio_config              ddc_ctrl;

	u32				cec_super_standby;
	u32				cec_support;

	int				is_cts;

	char				power[20];
	u8				power_use;

	struct clk			*hdmi_clk;
	struct clk			*hdmi_ddc_clk;
	struct clk			*hdmi_hdcp_clk;
	struct clk			*hdmi_cec_clk;

	struct task_struct		*hdmi_task;
	struct task_struct		*cec_task;

	struct mutex			ctrl_mutex;
	struct mutex			hdcp_mutex;

	struct hdmi_tx_core		*hdmi_core;
};

struct file_ops {
	int ioctl_cmd;
};

extern s32 disp_set_hdmi_func(struct disp_device_func *func);
extern unsigned int disp_boot_para_parse(const char *name);
extern unsigned long long disp_boot_get_video_paras(unsigned int screen_num);
extern unsigned int disp_boot_para_parse_array(const char *name, unsigned int *value,
							  unsigned int count);
extern int of_get_named_gpio_flags(struct device_node *np, const char *propname,
			   int index, enum of_gpio_flags *flags);
extern uintptr_t disp_getprop_regbase(
	char *main_name, char *sub_name, u32 index);
extern int arisc_query_wakeup_source(unsigned int *event);
extern int sunxi_smc_refresh_hdcp(void);

#ifdef CONFIG_SUNXI_SMC
extern int sunxi_smc_load_hdcp_key(void *hdcp_buff, size_t size);
#endif
#endif /* __INCLUDES_H__ */

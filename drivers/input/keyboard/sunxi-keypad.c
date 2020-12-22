/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com,qys<qinyongshen@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/pm.h>
#include <linux/init-input.h>
#include <linux/arisc/arisc.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk/clk-sun8iw9.h>
#undef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif
#include "sunxi-keypad.h"
#include <linux/power/scenelock.h> 

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_DATA = 1U << 4,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	pr_debug(fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);

static char keypad_dev_name[] = "kp0";
static struct input_dev *keypd_dev;
static u32 key_pressed[BITS_TO_LONGS(KEYPAD_MAX_CNT)];
static struct clk *keypad_clk;
static struct clk *keypad_clk_source;
static struct pinctrl *keypad_pinctrl;

static int keypad_power_key;


#ifdef CONFIG_HAS_EARLYSUSPEND	
struct sunxi_keypad_data {
	struct early_suspend early_suspend;
};
static struct sunxi_keypad_data *keypad_data;
#else
#ifdef CONFIG_PM
static struct dev_pm_domain keypad_pm_domain;
#endif
#endif

static void keypad_enable_set(u32 enable)
{
	u32 reg_val;
	reg_val = readl((const volatile void __iomem *)KP_CTL);
	if(enable)
		reg_val |= 0x1;
	else
		reg_val &= ~0x1;
	writel(reg_val, (volatile void __iomem *)KP_CTL);
	
}

static void keypad_bitmasks_set(u32 b_masks)
{
	u32 reg_val;
	b_masks &= 0xffff;
	reg_val = readl((const volatile void __iomem *)KP_CTL);
	reg_val &= ~(0xffff<<8);
	reg_val |= b_masks<<8;
	writel(reg_val, (volatile void __iomem *)KP_CTL);
}

static void keypad_ints_set(u32 ints)
{
	u32 reg_val;
	ints &= 0x3;
	reg_val = readl((const volatile void __iomem *)KP_INT_CFG);
	reg_val &= ~(0x3<<0);
	reg_val |= ints<<0;
	writel(reg_val, (volatile void __iomem *)KP_INT_CFG);
}

static int keypad_int_platfrom(void)
{
	
	dprintk(DEBUG_INIT, "keypad io init\n");
	keypad_pinctrl = devm_pinctrl_get_select_default(&(keypd_dev->dev));
	if (IS_ERR_OR_NULL(keypad_pinctrl)) {
		pr_warn("request pinctrl handle for device [%s] failed\n",
		dev_name(&(keypd_dev->dev)));
		return -EINVAL;
	}

	return 0;
}

static void keypad_free_platform(void)
{
	if (!IS_ERR_OR_NULL(keypad_pinctrl))
		devm_pinctrl_put(keypad_pinctrl);
}

#if (defined CONFIG_ARCH_SUN8IW9P1)&&(defined CONFIG_FPGA_V4_PLATFORM)
static int keypad_clk_cfg(void)
{
	u32 tmp;
	tmp = readl((const volatile void __iomem *)0xf1f01428);
	tmp |= 0x1<<10;
	writel(tmp, (volatile void __iomem *)0xf1f01428);
	
	tmp = readl((const volatile void __iomem *)0xf1f014b0);
	tmp |= 0x1<<10;
	writel(tmp, (volatile void __iomem *)0xf1f014b0);
		
	writel((0x1<<31)|(0x2<<24)|(0<<16)|(0<<0), (volatile void __iomem *)0xf1f01434);
	
	dprintk(DEBUG_INIT, "keypad clk tmp init\n");

	return 0;
}
#else

static int keypad_clk_cfg(void)/////unfinish
{
	unsigned long rate = 0; /* 3Mhz */

	keypad_clk_source = clk_get(NULL, LOSC_CLK);
	if (!keypad_clk_source || IS_ERR(keypad_clk_source)) {
		pr_err("try to get keypad_clk_source clock failed!\n");
		return -1;
	}

	rate = clk_get_rate(keypad_clk_source);
	dprintk(DEBUG_INIT, "%s: get keypad_clk_source rate %dHZ\n", __func__, (__u32)rate);

	keypad_clk = clk_get(NULL, KEYPAD_CLK);////gai
	if (!keypad_clk || IS_ERR(keypad_clk)) {
		pr_err("try to get keypad clock failed!\n");
		return -1;
	}

	if(clk_set_parent(keypad_clk, keypad_clk_source))
		pr_err("%s: set keypad_clk parent to ir_clk_source failed!\n", __func__);

	if (clk_set_rate(keypad_clk, 31250)) {
		pr_err("set keypad clock freq to 31250 failed!\n");
	}
	rate = clk_get_rate(keypad_clk);
	dprintk(DEBUG_INIT, "%s: get ir_clk rate %dHZ\n", __func__, (__u32)rate);

	if (clk_prepare_enable(keypad_clk)) {
		pr_err("try to enable ir_clk failed!\n");
	}

	return 0;
}
#endif

static void keypad_clk_uncfg(void)
{
	if(NULL == keypad_clk || IS_ERR(keypad_clk)) {
		pr_err("keypad_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(keypad_clk);
		clk_put(keypad_clk);
		keypad_clk = NULL;
	}

	if(NULL == keypad_clk_source || IS_ERR(keypad_clk_source)) {
		pr_err("ir_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(keypad_clk_source);
		keypad_clk_source = NULL;
	}
	return;
}

static void keypad_set_scan_cycl(u32 cycl)
{
	u32 reg_val;
	cycl = (cycl > 0xffff? 0xffff:cycl);
	reg_val = readl((const volatile void __iomem *)KP_TIMING);
	reg_val &= ~(0xffff<<0);
	reg_val |= cycl<<0;
	writel(reg_val, (volatile void __iomem *)KP_TIMING);
}

static void keypad_set_debun_cycl(u32 cycl)
{
	u32 reg_val;
	cycl = (cycl > 0xffff? 0xffff:cycl);
	reg_val = readl((const volatile void __iomem *)KP_TIMING);
	reg_val &= ~(0xffff<<16);
	reg_val |= cycl<<16;
	writel(reg_val, (volatile void __iomem *)KP_TIMING);
}

static int keypad_sys_setup(void)/////unfinish
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	
	key_pressed[0] = 0;
	key_pressed[1] = 0;

	type = script_get_item("kp0", "keypad_used", &val);
 
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		ret = -1;
	}
	
	if (1 == val.val) {
		type = script_get_item("kp0", "keypad_power_key_code", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: keypad power key type err! \n", __func__);
		}
		keypad_power_key = val.val;

		ret = 0;
		
	} else {
		pr_err("%s: keypad_unused. \n",  __func__);
		ret = -1;
	}

	return ret;
}

static u32 keypad_read_int(void)
{
	return readl((const volatile void __iomem *)KP_INT_STA);
}

static void keypad_clr_int(u32 reg_val)
{
	writel(reg_val, (volatile void __iomem *)KP_INT_STA);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sunxi_keypad_early_suspend(struct early_suspend *h)
{
	dprintk(DEBUG_SUSPEND, "enter earlysuspend: sunxi_keypad_early_suspend. \n");

	disable_irq_nosync(KEYPAD_IRQNUM);

	if(NULL == keypad_clk || IS_ERR(keypad_clk)) {
		pr_err("keypad_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(keypad_clk);
	}
	return ;
}

/* ���»��� */
static void sunxi_keypad_late_resume(struct early_suspend *h)
{
	unsigned int keypad_event = 0;
	dprintk(DEBUG_SUSPEND, "enter laterresume: sunxi_ir_rx_resume. \n");

	arisc_query_wakeup_source(&keypad_event);
	dprintk(DEBUG_SUSPEND, "%s: event 0x%x\n", __func__, keypad_event);
	if (CPUS_WAKEUP_IR&keypad_event) {/////unfinish
		input_report_key(keypd_dev, keypad_power_key, 1);
		input_sync(keypd_dev);
		msleep(1);
		input_report_key(keypd_dev, keypad_power_key, 0);
		input_sync(keypd_dev);
	}

	key_pressed[0] = 0;
	key_pressed[1] = 0;
	clk_prepare_enable(keypad_clk);
	enable_irq(KEYPAD_IRQNUM);

	return ;
}
#else
#ifdef CONFIG_PM
static int sunxi_keypad_suspend(struct device *dev)
{

	dprintk(DEBUG_SUSPEND, "enter: sunxi_keypad_suspend. \n");

	disable_irq_nosync(KEYPAD_IRQNUM);

	if(NULL == keypad_clk || IS_ERR(keypad_clk)) {
		pr_err("keypad_clk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_disable_unprepare(keypad_clk);
	}
	return 0;
}

/* ���»��� */
static int sunxi_keypad_resume(struct device *dev)
{
	unsigned int keypad_event = 0;
	dprintk(DEBUG_SUSPEND, "enter: sunxi_keypad_resume. \n");

	arisc_query_wakeup_source(&keypad_event);
	dprintk(DEBUG_SUSPEND, "%s: event 0x%x\n", __func__, keypad_event);
	if (CPUS_WAKEUP_IR&keypad_event) {/////unfinish
		input_report_key(keypd_dev, keypad_power_key, 1);
		input_sync(keypd_dev);
		msleep(1);
		input_report_key(keypd_dev, keypad_power_key, 0);
		input_sync(keypd_dev);
	}

	key_pressed[0] = 0;
	key_pressed[1] = 0;
	clk_prepare_enable(keypad_clk);
	enable_irq(KEYPAD_IRQNUM);

	return 0;
}
#endif
#endif


static irqreturn_t keypad_irq_service(int irq, void *dummy)
{
	u32 int_stat,key_scan[2];
	u32 tmp_reg,i,j;
	//dprintk(DEBUG_INT, "keypad int enter\n");
	
	int_stat = keypad_read_int();
	
	key_scan[0] = ~(readl((const volatile void __iomem *)KP_IN0));
	key_scan[1] = ~(readl((const volatile void __iomem *)KP_IN1));
	dprintk(DEBUG_INT, "keyscan 0 : 0x%x \n",key_scan[0]);
	dprintk(DEBUG_INT, "keyscan 1 : 0x%x \n",key_scan[1]);

	if(int_stat & INT_KEY_PRESS){
		dprintk(DEBUG_INT, "keypad:key pressed \n");
		for(j = 0; j < 2; j++){
			tmp_reg = key_scan[j] & (key_pressed[j]^key_scan[j]);
			if(tmp_reg){
				for(i = 0; tmp_reg; i++){
					if(tmp_reg & 0x1){
						input_report_key(keypd_dev, keypad_keycodes[i+j*32], 1);
						input_sync(keypd_dev);
						dprintk(DEBUG_DATA, "irq key down :%d\n",keypad_keycodes[i+j*32]);
					}
					tmp_reg = tmp_reg>>1;
				}
			}
			tmp_reg = key_pressed[j] & (key_pressed[j]^key_scan[j]);
			if(tmp_reg){
				for(i = 0; tmp_reg; i++){
					if(tmp_reg & 0x1){
						input_report_key(keypd_dev, keypad_keycodes[i+j*32], 0);
						input_sync(keypd_dev);
						dprintk(DEBUG_DATA, "irq key up :%d\n",keypad_keycodes[i+j*32]);
					}
					tmp_reg = tmp_reg>>1;
				}
			}
			key_pressed[j] = key_scan[j];
		}
	}

	if(int_stat & INT_KEY_RELEASE){
		dprintk(DEBUG_INT, "keypad:key release \n");
		for(j = 0; j < 2; j++){
			tmp_reg = key_pressed[j];
			if(tmp_reg){
				for(i = 0; tmp_reg; i++){
					if(tmp_reg & 0x1){
						input_report_key(keypd_dev, keypad_keycodes[i+j*32], 0);
						input_sync(keypd_dev);
						dprintk(DEBUG_DATA, "irq key up :%d\n",keypad_keycodes[i+j*32]);
					}
					tmp_reg = tmp_reg>>1;
				}
			}
			key_pressed[j] = 0;
		}
	}

	//key_pressed[0] = key_scan[0];
	//key_pressed[1] = key_scan[1];

	keypad_clr_int(int_stat);
		
	return IRQ_HANDLED;
}

static int __init sunxikpd_init(void)
{
	int i;
	int err =0;

	if(keypad_sys_setup())
		goto fail1;

	keypd_dev = input_allocate_device();
	if (!keypd_dev) {
		pr_err("keypd_dev: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	keypd_dev->name = "sunxi-keypad";/////unfinish
	keypd_dev->phys = "Keypad/input2";
	keypd_dev->id.bustype = BUS_HOST;
	keypd_dev->id.vendor = 0x0001;
	keypd_dev->id.product = 0x0001;
	keypd_dev->id.version = 0x0100;
	keypd_dev->dev.init_name = &keypad_dev_name[0];/////unfinish

    #ifdef REPORT_REPEAT_KEY_VALUE
	keypd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP) ;
    #else
	keypd_dev->evbit[0] = BIT_MASK(EV_KEY);
    #endif
    
    	for (i = 0; i < KEYPAD_MAX_CNT; i++)
		set_bit(keypad_keycodes[i], keypd_dev->keybit);

	keypad_int_platfrom();
		
	if (request_irq(KEYPAD_IRQNUM, keypad_irq_service, 0, "Sunxi_keypad",
			keypd_dev)) {
		err = -EBUSY;
		pr_err("keypd_dev: irq request failed\n");
		goto fail2;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
#else
#ifdef CONFIG_PM
	keypad_pm_domain.ops.suspend = sunxi_keypad_suspend;
	keypad_pm_domain.ops.resume = sunxi_keypad_resume;
	keypd_dev->dev.pm_domain = &keypad_pm_domain;	
#endif
#endif

	err = input_register_device(keypd_dev);
	if (err){
		pr_err("keypd_dev: input dev register fail\n");
		goto fail3;
	}
	
#ifdef CONFIG_HAS_EARLYSUSPEND 
	dprintk(DEBUG_INIT, "==register_early_suspend =\n");
	keypad_data = kzalloc(sizeof(*keypad_data), GFP_KERNEL);
	if (keypad_data == NULL) {
		err = -ENOMEM;
		pr_err("keypd_dev: keypad data alloc failed\n");
		goto err_alloc_data_failed;
	}
	
	keypad_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 3;	
	keypad_data->early_suspend.suspend = sunxi_keypad_early_suspend;
	keypad_data->early_suspend.resume	= sunxi_keypad_late_resume;
	register_early_suspend(&keypad_data->early_suspend);
#endif
	keypad_set_scan_cycl(KEYPAD_SCAN_CYCL);
	keypad_set_debun_cycl(KEYPAD_DEBOUN_CYCL);
	keypad_bitmasks_set(PASS_ALL);
	
	keypad_clk_cfg();

	keypad_ints_set(INT_KEYPRESS_EN|INT_KEYRELEASE_EN);
	keypad_enable_set(KEYPAD_ENABLE);

	dprintk(DEBUG_INIT, "sunxi_keypad_init end\n");

	return 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
err_alloc_data_failed:
#endif
fail3:	
	free_irq(KEYPAD_IRQNUM, NULL);
fail2:	
	input_free_device(keypd_dev);
	keypad_free_platform();
fail1:
	pr_err("sunxikbd_init failed. \n");
	
	return err;
}

static void __exit sunxikpd_exit(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&keypad_data->early_suspend);	
#endif
	free_irq(KEYPAD_IRQNUM, NULL);
	keypad_clk_uncfg();
	keypad_free_platform();
	input_unregister_device(keypd_dev);

}

module_init(sunxikpd_init);
module_exit(sunxikpd_exit);

MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sunxi-keypad driver");
MODULE_LICENSE("GPL");


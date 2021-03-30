/*
 * Copyright (c) 2013-2015 qinyongshen@allwinnertech.com,qys<qinyongshen@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include <linux/arisc/arisc.h>
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
static struct sunxi_keypad_data *kpad_data;
static u32 key_pressed[2];

static int keypad_power_key;

#ifdef CONFIG_PM
static struct dev_pm_domain keypad_pm_domain;
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

static int keypad_clk_cfg(void)
{
	unsigned long rate = 0; /* 3Mhz */

	rate = clk_get_rate(kpad_data->pclk);
	dprintk(DEBUG_INIT, "%s: get keypad_clk_source rate %dHZ\n", __func__, (__u32)rate);

	if(clk_set_parent(kpad_data->mclk, kpad_data->pclk))
		pr_err("%s: set keypad_clk parent to keypad_clk_source failed!\n", __func__);

	if (clk_set_rate(kpad_data->mclk, 31250)) {
		pr_err("set keypad clock freq to 31250 failed!\n");
	}
	rate = clk_get_rate(kpad_data->mclk);
	dprintk(DEBUG_INIT, "%s: get keyapd_clk rate %dHZ\n", __func__, (__u32)rate);

	if (clk_prepare_enable(kpad_data->mclk)) {
			pr_err("try to enable keypad_clk failed!\n");
	}

	return 0;
}

static void keypad_clk_uncfg(void)
{
	if(NULL == kpad_data->mclk || IS_ERR(kpad_data->mclk)) {
		pr_err("keypad_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(kpad_data->mclk);
		clk_put(kpad_data->mclk);
		kpad_data->mclk = NULL;
	}

	if(NULL == kpad_data->pclk || IS_ERR(kpad_data->pclk)) {
		pr_err("keypad_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(kpad_data->pclk);
		kpad_data->pclk = NULL;
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

static int sunxi_keypad_startup(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np =NULL;
	np = pdev->dev.of_node;

	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi keyboard is disable\n", __func__);
		return -EPERM;
	}
	
	key_pressed[0] = 0;
	key_pressed[1] = 0;

	kpad_data = kzalloc(sizeof(*kpad_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(kpad_data)) {
		pr_err("kpad_data: not enough memory for keypad data\n");
		return -ENOMEM;
	}

	kpad_data->reg_base= of_iomap(np, 0);
	if (NULL == kpad_data->reg_base) {
		pr_err("%s:Failed to ioremap() io memory region.\n",__func__);
		ret = -EBUSY;
	}else
		dprintk(DEBUG_INIT, "key base: %p !\n",kpad_data->reg_base);
	kpad_data->irq_num= irq_of_parse_and_map(np, 0);
	if (0 == kpad_data->irq_num) {
		pr_err("%s:Failed to map irq.\n", __func__);
		ret = -EBUSY;
	}else
		dprintk(DEBUG_INIT, "keypad irq num: %d !\n",kpad_data->irq_num);
	kpad_data->pclk = of_clk_get(np, 0);
	kpad_data->mclk = of_clk_get(np, 1);
	if (NULL==kpad_data->pclk||IS_ERR(kpad_data->pclk)
		||NULL==kpad_data->mclk||IS_ERR(kpad_data->mclk)) {
		dprintk(DEBUG_INIT, "%s:keypad has no clk.\n", __func__);
	}

	if (of_property_read_u32(np, "keypad_power_key_code", &keypad_power_key)) {
		pr_err("%s: get keypad_power_key_code failed", __func__);
		ret =  -EBUSY;
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

static int sunxi_keypad_suspend(struct device *dev)
{

	dprintk(DEBUG_SUSPEND, "enter: sunxi_keypad_suspend. \n");

	disable_irq_nosync(kpad_data->irq_num);

	if(IS_ERR_OR_NULL(kpad_data->mclk)) {
		pr_err("keypad_clk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_disable_unprepare(kpad_data->mclk);
	}
	return 0;
}

/* ���»��� */
static int sunxi_keypad_resume(struct device *dev)
{
	unsigned int keypad_event = 0;
	dprintk(DEBUG_SUSPEND, "enter: sunxi_keypad_resume. \n");
#if 0
	arisc_query_wakeup_source(&keypad_event);
#endif
	dprintk(DEBUG_SUSPEND, "%s: event 0x%x\n", __func__, keypad_event);
	if (CPUS_WAKEUP_IR&keypad_event) {
		input_report_key(keypd_dev, keypad_power_key, 1);
		input_sync(keypd_dev);
		msleep(1);
		input_report_key(keypd_dev, keypad_power_key, 0);
		input_sync(keypd_dev);
	}

	key_pressed[0] = 0;
	key_pressed[1] = 0;
	clk_prepare_enable(kpad_data->mclk);
	enable_irq(kpad_data->irq_num);

	return 0;
}

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

#ifdef CONFIG_OF
/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct of_device_id sunxi_keypad_of_match[] = {
	{ .compatible = "allwinner,keypad",},
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_keypad_of_match);
#else /* !CONFIG_OF */
#endif

static int sunxi_keypad_probe(struct platform_device *pdev)
{
	int i;
	int err =0;

	dprintk(DEBUG_INIT, "sunxi key pad_init \n");

	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		err = sunxi_keypad_startup(pdev);
	}else{
		pr_err("sunxi keypad device tree err!\n");
		return -EBUSY;
	}

	keypd_dev = input_allocate_device();
	if (!keypd_dev) {
		pr_err("keypd_dev: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	keypd_dev->name = "sunxi-keypad";
	keypd_dev->phys = "Keypad/input2";
	keypd_dev->id.bustype = BUS_HOST;
	keypd_dev->id.vendor = 0x0001;
	keypd_dev->id.product = 0x0001;
	keypd_dev->id.version = 0x0100;
	keypd_dev->dev.init_name = &keypad_dev_name[0];

    #ifdef REPORT_REPEAT_KEY_VALUE
	keypd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP) ;
    #else
	keypd_dev->evbit[0] = BIT_MASK(EV_KEY);
    #endif
    
    	for (i = 0; i < KEYPAD_MAX_CNT; i++)
		set_bit(keypad_keycodes[i], keypd_dev->keybit);

	err = input_register_device(keypd_dev);
	if (err){
		pr_err("keypd_dev: input dev register fail\n");
		goto fail2;
	}
	kpad_data->input_dev= keypd_dev;
	platform_set_drvdata(pdev, kpad_data);
	
	keypad_clk_cfg();
	keypad_set_scan_cycl(KEYPAD_SCAN_CYCL);
	keypad_set_debun_cycl(KEYPAD_DEBOUN_CYCL);
	keypad_bitmasks_set(PASS_ALL);

	keypad_ints_set(INT_KEYPRESS_EN|INT_KEYRELEASE_EN);
	keypad_enable_set(KEYPAD_ENABLE);

	if (request_irq(kpad_data->irq_num, keypad_irq_service, 0, "Sunxi_keypad",
			keypd_dev)) {
		err = -EBUSY;
		pr_err("keypd_dev: irq request failed\n");
		goto fail3;
	}

	dprintk(DEBUG_INIT, "sunxi_keypad_init end\n");

	return 0;
fail3:	
	input_unregister_device(keypd_dev);
fail2:	
	input_free_device(keypd_dev);
fail1:
	if(kpad_data)
		kfree(kpad_data);
	pr_err("sunxikbd_init failed. \n");
	
	return err;
}

static int sunxi_keypad_remove(struct platform_device *pdev)

{
	free_irq(kpad_data->irq_num, NULL);
	keypad_clk_uncfg();
	input_unregister_device(keypd_dev);
	input_free_device(keypd_dev);
	return 0;
}

static const struct dev_pm_ops sunxi_keypad_pm_ops = {
	.suspend        = sunxi_keypad_suspend,
	.resume         = sunxi_keypad_resume,
};

static struct platform_driver sunxi_keypad_driver = {
	.probe  = sunxi_keypad_probe,
	.remove = sunxi_keypad_remove,
	.driver = {
		.name   = "sunxi-keypad",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(sunxi_keypad_of_match),
		.pm	= &sunxi_keypad_pm_ops,
	},
};
module_platform_driver(sunxi_keypad_driver);

MODULE_AUTHOR(" Qin Yongshen");
MODULE_DESCRIPTION("sunxi-keypad driver");
MODULE_LICENSE("GPL");



/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
*
* Copyright (c) 2011
*
* ChangeLog
*
*
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
#include <mach/sys_config.h>
#undef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_ARCH_SUN9IW1P1
#include <linux/clk/clk-sun9iw1.h>
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif
#include "sun8i-keyboard.h"
#include <linux/power/scenelock.h> 

#ifdef MODE_0V2
#ifndef CONFIG_ARCH_SUN9IW1P1
static unsigned char keypad_mapindex[64] = {
	0,0,0,0,0,0,0,0,               	/* key 1, 8���� 0-7 */
	1,1,1,1,1,1,1,                 	/* key 2, 7���� 8-14 */
	2,2,2,2,2,2,2,                 	/* key 3, 7���� 15-21 */
	3,3,3,3,3,3,                   	/* key 4, 6���� 22-27 */
	4,4,4,4,4,4,                   	/* key 5, 6���� 28-33 */
	5,5,5,5,5,5,                   	/* key 6, 6���� 34-39 */
	6,6,6,6,6,6,6,6,6,6,           	/* key 7, 10����40-49 */
	7,7,7,7,7,7,7,7,7,7,7,7,7,7    	/* key 8, 17����50-63 */
};
#else
static unsigned char keypad_mapindex[64] = {
	0,0,0,0,0,0,0,0,0,            	/* key 1, 0-8 */
	1,1,1,1,1,                 	/* key 2, 9-13 */
	2,2,2,2,2,2,                 	/* key 3, 14-19 */
	3,3,3,3,3,3,                   	/* key 4, 20-25 */
	4,4,4,4,4,4,4,4,4,4,4,          /* key 5, 26-36 */
	5,5,5,5,5,5,5,5,5,5,5,          /* key 6, 37-39 */
	6,6,6,6,6,6,6,6,6,           	/* key 7, 40-49 */
	7,7,7,7,7,7,7  			/* key 8, 50-63 */
};
#endif
#endif
                        
#ifdef MODE_0V15
/* 0.15V mode */
static unsigned char keypad_mapindex[64] = {
	0,0,0,    			/* key1 */
	1,1,1,1,1,                     	/* key2 */
	2,2,2,2,2,
	3,3,3,3,
	4,4,4,4,4,
	5,5,5,5,5,
	6,6,6,6,6,
	7,7,7,7,
	8,8,8,8,8,
	9,9,9,9,9,
	10,10,10,10,
	11,11,11,11,
	12,12,12,12,12,12,12,12,12,12  	/*key13 */
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND	
struct sunxi_keyboard_data {
	struct early_suspend early_suspend;
};
#else
#ifdef CONFIG_PM
static struct dev_pm_domain keyboard_pm_domain;
#endif
#endif

#if defined(CONFIG_ARCH_SUN8IW6P1)||defined(CONFIG_ARCH_SUN9IW1P1)
#define ADC_MEASURE	(1350)
#define ADC_RESOL 	(21)
#else
#define ADC_MEASURE	(2000)
#define ADC_RESOL 	(31)
#endif
#define VOL_NUM 16

static int key_vol[VOL_NUM];
static int key_num = 0;
static volatile u32 key_val;
static struct input_dev *sunxikbd_dev;
static unsigned char scancode;

static unsigned char key_cnt = 0;
static unsigned char compare_buffer[REPORT_START_NUM] = {0};
static unsigned char transfer_code = INITIAL_VALUE;
#ifdef CONFIG_ARCH_SUN9IW1P1
static struct clk *key_clk;
static struct clk *key_clk_source;
#endif
enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);


#ifdef CONFIG_HAS_EARLYSUSPEND
static struct sunxi_keyboard_data *keyboard_data;
#endif

#ifdef CONFIG_ARCH_SUN9IW1P1
static void sunxikbd_clk_cfg(void)
{

	unsigned long rate = 0; /* 3Mhz */

	key_clk_source = clk_get(NULL, APB0_CLK);
	if (!key_clk_source || IS_ERR(key_clk_source)) {
		pr_err("try to get key_clk_source failed!\n");
		return;
	}

	rate = clk_get_rate(key_clk_source);
	dprintk(DEBUG_INIT, "%s: get key_clk_source rate %dHZ\n", __func__, (__u32)rate);

	key_clk = clk_get(NULL, LRADC_CLK);
	if (!key_clk || IS_ERR(key_clk)) {
		pr_err("try to get key clock failed!\n");
		return;
	}

	if(clk_set_parent(key_clk, key_clk_source))
		pr_err("%s: set key_clk parent to key_clk_source failed!\n", __func__);

	if (clk_prepare_enable(key_clk)) {
			pr_err("try to enable key_clk failed!\n");
	}

	return;
}

static void sunxikbd_clk_uncfg(void)
{

	if(NULL == key_clk || IS_ERR(key_clk)) {
		pr_err("key_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(key_clk);
		clk_put(key_clk);
		key_clk = NULL;
	}

	if(NULL == key_clk_source || IS_ERR(key_clk_source)) {
		pr_err("key_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(key_clk_source);
		key_clk_source = NULL;
	}
	return;
}
#endif

static void sunxi_keyboard_ctrl_set(enum key_mode key_mode, u32 para)
{
	u32 ctrl_reg = 0;
	
	if (0 != para)
		ctrl_reg = readl((const volatile void __iomem *)(KEY_BASSADDRESS + LRADC_CTRL));
	
	if (CONCERT_DLY_SET & key_mode)
		ctrl_reg |= (FIRST_CONCERT_DLY & para);
	if (ADC_CHAN_SET & key_mode)
		ctrl_reg |= (ADC_CHAN_SELECT & para);
	if (KEY_MODE_SET & key_mode)
		ctrl_reg |= (KEY_MODE_SELECT & para);
	if (LRADC_HOLD_SET & key_mode)
		ctrl_reg |= (LRADC_HOLD_EN & para);
	if (LEVELB_VOL_SET & key_mode)
		ctrl_reg |= (LEVELB_VOL & para);
	if (LRADC_SAMPLE_SET & key_mode)
		ctrl_reg |= (LRADC_SAMPLE_250HZ & para);
	if (LRADC_EN_SET & key_mode)
		ctrl_reg |= (LRADC_EN & para);
	
	writel(ctrl_reg, (volatile void __iomem *)(KEY_BASSADDRESS + LRADC_CTRL));
}

static void sunxi_keyboard_int_set(enum int_mode int_mode, u32 para)
{
	u32 ctrl_reg = 0;

	if (0 != para)
		ctrl_reg = readl((const volatile void __iomem *)(KEY_BASSADDRESS + LRADC_INTC));
	
	if (ADC0_DOWN_INT_SET & int_mode)
		ctrl_reg |= (LRADC_ADC0_DOWN_EN & para);
	if (ADC0_UP_INT_SET & int_mode)
		ctrl_reg |= (LRADC_ADC0_UP_EN & para);
	if (ADC0_DATA_INT_SET & int_mode)
		ctrl_reg |= (LRADC_ADC0_DATA_EN & para);
	
	writel(ctrl_reg, (volatile void __iomem *)(KEY_BASSADDRESS + LRADC_INTC));
}

static u32 sunxi_keyboard_read_ints(void)
{
	u32 reg_val;
	reg_val  = readl((const volatile void __iomem *)(KEY_BASSADDRESS + LRADC_INT_STA));

	return reg_val;
}

static void sunxi_keyboard_clr_ints(u32 reg_val)
{
	writel(reg_val, (volatile void __iomem *)(KEY_BASSADDRESS + LRADC_INT_STA));
}

static u32 sunxi_keyboard_read_data(unsigned long addr)
{
	u32 reg_val;
	reg_val = readl((const volatile void __iomem *)(addr));

	return reg_val;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
/* ͣ���豸 */
static void sunxi_keyboard_early_suspend(struct early_suspend *h)
{
	//int ret;
	//struct sunxi_keyboard_data *ts = container_of(h, struct sunxi_keyboard_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);

	disable_irq_nosync(SW_INT_IRQNO_LRADC);
    
	if (NORMAL_STANDBY == standby_type) {
		sunxi_keyboard_ctrl_set(0, 0);
#ifdef CONFIG_ARCH_SUN9IW1P1
		clk_disable_unprepare(key_clk);
#endif
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
		if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
			printk("lradc-key: talking standby, enable wakeup source lradc!!\n");
			enable_wakeup_src(CPUS_LRADC_SRC, 0);
		} else {
			sunxi_keyboard_ctrl_set(0, 0);
#ifdef CONFIG_ARCH_SUN9IW1P1
			clk_disable_unprepare(key_clk);
#endif

		}
	}
	return ;
}

/* ���»��� */
static void sunxi_keyboard_late_resume(struct early_suspend *h)
{
	unsigned long mode, para;
	//int ret;
	//struct sunxi_keyboard_data *ts = container_of(h, struct sunxi_keyboard_data, early_suspend);
	
	dprintk(DEBUG_SUSPEND, "[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type); 

#ifdef CONFIG_ARCH_SUN9IW1P1
	clk_prepare_enable(key_clk);
#endif

	/* process for normal standby */
	if (NORMAL_STANDBY == standby_type) {
		mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET | LRADC_HOLD_SET | LEVELB_VOL_SET \
			| LRADC_SAMPLE_SET | LRADC_EN_SET;
		para = FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN;
		sunxi_keyboard_ctrl_set(mode, para);
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
		if (check_scene_locked(SCENE_TALKING_STANDBY) != 0) {
#ifdef ONE_CHANNEL
			mode = ADC0_DOWN_INT_SET | ADC0_UP_INT_SET | ADC0_DATA_INT_SET;
			para = LRADC_ADC0_DOWN_EN | LRADC_ADC0_UP_EN | LRADC_ADC0_DATA_EN;
			sunxi_keyboard_int_set(mode, para);
			mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET | LRADC_HOLD_SET | LEVELB_VOL_SET \
				| LRADC_SAMPLE_SET | LRADC_EN_SET;
			para = FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
				|LRADC_SAMPLE_250HZ|LRADC_EN;
			sunxi_keyboard_ctrl_set(mode, para);
#else
#endif
		} else {
			disable_wakeup_src(CPUS_LRADC_SRC, 0);
			printk("lradc-key: resume from talking standby!!\n");
		}
	}

	enable_irq(SW_INT_IRQNO_LRADC);
	
	return ; 
}
#else
#ifdef CONFIG_PM
static int sunxi_keyboard_suspend(struct device *dev)
{

	//int ret;
	//struct sunxi_keyboard_data *ts = container_of(h, struct sunxi_keyboard_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);

	disable_irq_nosync(SW_INT_IRQNO_LRADC);

	if (NORMAL_STANDBY == standby_type) {
		sunxi_keyboard_ctrl_set(0, 0);
#ifdef CONFIG_ARCH_SUN9IW1P1
		clk_disable_unprepare(key_clk);
#endif

	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
		if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
			printk("lradc-key: talking standby, enable wakeup source lradc!!\n");
			enable_wakeup_src(CPUS_LRADC_SRC, 0);
		} else {
			sunxi_keyboard_ctrl_set(0, 0);
#ifdef CONFIG_ARCH_SUN9IW1P1
			clk_disable_unprepare(key_clk);
#endif
		}
	}
	return 0;
}

/* ���»��� */
static int sunxi_keyboard_resume(struct device *dev)
{
	unsigned long mode, para;
	//int ret;
	//struct sunxi_keyboard_data *ts = container_of(h, struct sunxi_keyboard_data, early_suspend);
	
	dprintk(DEBUG_SUSPEND, "[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type);

#ifdef CONFIG_ARCH_SUN9IW1P1
	clk_prepare_enable(key_clk);
#endif

	/* process for normal standby */
	if (NORMAL_STANDBY == standby_type) {
		mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET | LRADC_HOLD_SET | LEVELB_VOL_SET \
			| LRADC_SAMPLE_SET | LRADC_EN_SET;
		para = FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN;
		sunxi_keyboard_ctrl_set(mode, para);
	/* process for super standby */	
	} else if (SUPER_STANDBY == standby_type) {
		if (check_scene_locked(SCENE_TALKING_STANDBY) != 0) {
#ifdef ONE_CHANNEL
		mode = ADC0_DOWN_INT_SET | ADC0_UP_INT_SET | ADC0_DATA_INT_SET;
		para = LRADC_ADC0_DOWN_EN | LRADC_ADC0_UP_EN | LRADC_ADC0_DATA_EN;
		sunxi_keyboard_int_set(mode, para);
		mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET | LRADC_HOLD_SET | LEVELB_VOL_SET \
			| LRADC_SAMPLE_SET | LRADC_EN_SET;
		para = FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
			|LRADC_SAMPLE_250HZ|LRADC_EN;
		sunxi_keyboard_ctrl_set(mode, para);
#else
#endif
		} else {
			disable_wakeup_src(CPUS_LRADC_SRC, 0);
			printk("lradc-key: resume from talking standby!!\n");
		}
	}

	enable_irq(SW_INT_IRQNO_LRADC);

	return 0; 
}
#endif
#endif


static irqreturn_t sunxi_isr_key(int irq, void *dummy)
{
	u32  reg_val = 0;
	int judge_flag = 0;
	 
	dprintk(DEBUG_INT, "Key Interrupt\n");
	
	reg_val = sunxi_keyboard_read_ints();
	//writel(reg_val,KEY_BASSADDRESS + LRADC_INT_STA);

	if (reg_val & LRADC_ADC0_DOWNPEND) {	
		dprintk(DEBUG_INT, "key down\n");
	}
	
	if (reg_val & LRADC_ADC0_DATAPEND) {
		key_val = sunxi_keyboard_read_data(KEY_BASSADDRESS+LRADC_DATA0);
		
		if (key_val < 0x3f) {

			compare_buffer[key_cnt] = key_val&0x3f;
		}

		if ((key_cnt + 1) < REPORT_START_NUM) {
			key_cnt++;
			/* do not report key message */

		} else {
			if(compare_buffer[0] == compare_buffer[1])
			{
			key_val = compare_buffer[1];
			scancode = keypad_mapindex[key_val&0x3f];
			judge_flag = 1;
			key_cnt = 0;
			} else {
				key_cnt = 0;
				judge_flag = 0;
			}

			if (1 == judge_flag) {
				dprintk(DEBUG_INT, "report data: key_val :%8d transfer_code: %8d , scancode: %8d\n", \
					key_val, transfer_code, scancode);

				if (transfer_code == scancode) {
					/* report repeat key value */
#ifdef REPORT_REPEAT_KEY_FROM_HW
					input_report_key(sunxikbd_dev, sunxi_scankeycodes[scancode], 0);
					input_sync(sunxikbd_dev);
					input_report_key(sunxikbd_dev, sunxi_scankeycodes[scancode], 1);
					input_sync(sunxikbd_dev);
#else
					/* do not report key value */
#endif
				} else if (INITIAL_VALUE != transfer_code) {                               
					/* report previous key value up signal + report current key value down */
					input_report_key(sunxikbd_dev, sunxi_scankeycodes[transfer_code], 0);
					input_sync(sunxikbd_dev);
					input_report_key(sunxikbd_dev, sunxi_scankeycodes[scancode], 1);
					input_sync(sunxikbd_dev);
					transfer_code = scancode;

				} else {
					/* INITIAL_VALUE == transfer_code, first time to report key event */
					input_report_key(sunxikbd_dev, sunxi_scankeycodes[scancode], 1);
					input_sync(sunxikbd_dev);
					transfer_code = scancode;
				}
			}
		}
	}
       
	if (reg_val & LRADC_ADC0_UPPEND) {
		
		if(INITIAL_VALUE != transfer_code) {
				
			dprintk(DEBUG_INT, "report data: key_val :%8d transfer_code: %8d \n",key_val, transfer_code);
					
			input_report_key(sunxikbd_dev, sunxi_scankeycodes[transfer_code], 0);
			input_sync(sunxikbd_dev);
		}

		dprintk(DEBUG_INT, "key up \n");

		key_cnt = 0;
		judge_flag = 0;
		transfer_code = INITIAL_VALUE;
	}
	
	sunxi_keyboard_clr_ints(reg_val);
	return IRQ_HANDLED;
}

static void sunxikbd_map_init(void)
{
	int i = 0;
	unsigned char j = 0;
	for(i = 0; i < 64; i++){
		if(i * ADC_RESOL > key_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}
}

static int sunxikbd_script_init(void)
{
	int i;
	char key_name[16];
	script_item_u	val;
	script_item_value_type_e  type;

	type = script_get_item("key_para", "key_used", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: key para not found, used default para. \n", __func__);
		return 0;
	}

	if(1 == val.val){
		type = script_get_item("key_para", "key_cnt", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: get key cnt err! \n", __func__);
			return -1;
		}
		key_num = val.val;
		dprintk(DEBUG_INT,"%s key number = %d.\n", __func__, key_num);
		if(key_num < 1 || key_num > VOL_NUM){
			pr_err("incorrect key number.\n");
			return -1;
		}
		for(i = 0; i<VOL_NUM; i++)
			key_vol[i] = ADC_MEASURE;
		for(i = 1; i <= key_num; i++){
			sprintf(key_name, "key%d_vol", i);
			type = script_get_item("key_para", key_name, &val);
			if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
				pr_err("%s: get %s err! \n", __func__, key_name);
				return -1;
			}
			key_vol[i-1] = val.val;
			dprintk(DEBUG_INT,"%s: key%d vol = %d.\n", __func__, i, val.val);
		}

		sunxikbd_map_init();

	}else{
		dprintk(DEBUG_INT,"sunxi key board no used.\n");
		return -1;
	}

	return 0;
}

static int __init sunxikbd_init(void)
{
	int i;
	int err =0;
	unsigned long mode, para;

 	dprintk(DEBUG_INIT, "sunxikbd_init \n");
	
	if(sunxikbd_script_init()){
		err = -EFAULT;
		goto fail1;
	}

	sunxikbd_dev = input_allocate_device();
	if (!sunxikbd_dev) {
		pr_err("sunxikbd: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxikbd_dev->name = INPUT_DEV_NAME;  
	sunxikbd_dev->phys = "sunxikbd/input0"; 
	sunxikbd_dev->id.bustype = BUS_HOST;      
	sunxikbd_dev->id.vendor = 0x0001;
	sunxikbd_dev->id.product = 0x0001;
	sunxikbd_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sunxikbd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	pr_info("REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	sunxikbd_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif

	for (i = 0; i < KEY_MAX_CNT; i++)
		set_bit(sunxi_scankeycodes[i], sunxikbd_dev->keybit);

#ifdef CONFIG_ARCH_SUN9IW1P1
	sunxikbd_clk_cfg();
#endif

#ifdef ONE_CHANNEL
	mode = ADC0_DOWN_INT_SET | ADC0_UP_INT_SET | ADC0_DATA_INT_SET;
	para = LRADC_ADC0_DOWN_EN | LRADC_ADC0_UP_EN | LRADC_ADC0_DATA_EN;
	sunxi_keyboard_int_set(mode, para);
	mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET | LRADC_HOLD_SET | LEVELB_VOL_SET \
		| LRADC_SAMPLE_SET | LRADC_EN_SET;
	para = FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT|LRADC_HOLD_EN|ADC_CHAN_SELECT \
		|LRADC_SAMPLE_250HZ|LRADC_EN;
	sunxi_keyboard_ctrl_set(mode, para);
#else
#endif


	if (request_irq(SW_INT_IRQNO_LRADC, sunxi_isr_key, 0, "sunxikbd", NULL)) {
		err = -EBUSY;
		pr_err("request irq failure. \n");
		goto fail2;
	}

	
#ifdef CONFIG_HAS_EARLYSUSPEND
#else
#ifdef CONFIG_PM
	keyboard_pm_domain.ops.suspend = sunxi_keyboard_suspend;
	keyboard_pm_domain.ops.resume = sunxi_keyboard_resume;
	sunxikbd_dev->dev.pm_domain = &keyboard_pm_domain;	
#endif
#endif

	err = input_register_device(sunxikbd_dev);
	if (err)
		goto fail3;

#ifdef CONFIG_HAS_EARLYSUSPEND 
	dprintk(DEBUG_INIT, "==register_early_suspend =\n");
	keyboard_data = kzalloc(sizeof(*keyboard_data), GFP_KERNEL);
	if (keyboard_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	keyboard_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 3;	
	keyboard_data->early_suspend.suspend = sunxi_keyboard_early_suspend;
	keyboard_data->early_suspend.resume	= sunxi_keyboard_late_resume;
	register_early_suspend(&keyboard_data->early_suspend);
#endif
	dprintk(DEBUG_INIT, "sunxikbd_init end\n");

	return 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
err_alloc_data_failed:
#endif
fail3:	
	free_irq(SW_INT_IRQNO_LRADC, NULL);
fail2:	
	input_free_device(sunxikbd_dev);
fail1:
	pr_err("sunxikbd_init failed. \n");

	return err;
}

static void __exit sunxikbd_exit(void)
{	
#ifdef CONFIG_HAS_EARLYSUSPEND	
	unregister_early_suspend(&keyboard_data->early_suspend);	
#endif
	free_irq(SW_INT_IRQNO_LRADC, NULL);
#ifdef CONFIG_ARCH_SUN9IW1P1
	sunxikbd_clk_uncfg();
#endif
	input_unregister_device(sunxikbd_dev);
}

module_init(sunxikbd_init);
module_exit(sunxikbd_exit);

MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sunxi-keyboard driver");
MODULE_LICENSE("GPL");

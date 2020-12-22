/*
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liushaohua <liushaohua@allwinnertech.com>
 *
 * switch driver for ac100 chip
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/io.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/switch.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/arisc/arisc.h>
#include <linux/power/scenelock.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <mach/gpio.h>
#include "./../../../../sound/soc/codecs/ac100.h"

#define FUNCTION_NAME "h2w"
/* key define */
#define KEY_HEADSETHOOK         226
static script_item_u item_eint;
static script_item_u item_eint_other;
static bool KEY_HOOK_FLAG = 0;
static bool KEY_VOLUME_UP_FLAG = 0;
static bool KEY_VOLUME_DOWN_FLAG = 0;

static unsigned int ac100_rsb_read(unsigned int reg)
{
	int	ret;
	arisc_rsb_block_cfg_t rsb_data;
	unsigned char addr;
	unsigned int data;

	addr = (unsigned char)reg;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_HWORD;
	rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	rsb_data.devaddr = RSB_RTSADDR_AC100;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* read axp registers */
	ret = arisc_rsb_read_block_data(&rsb_data);
	if (ret != 0) {
		pr_err("failed reads to 0x%02x\n", reg);
		return ret;
	}
	return data;
}
static int ac100_rsb_write(unsigned int reg, unsigned int value)
{
	int	ret;
	arisc_rsb_block_cfg_t rsb_data;
	unsigned char addr;
	unsigned int data;

	addr = (unsigned char)reg;
	data = value;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_HWORD;
	rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	rsb_data.devaddr = RSB_RTSADDR_AC100;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* read axp registers */
	ret = arisc_rsb_write_block_data(&rsb_data);
	if (ret != 0) {
		pr_err("failed reads to 0x%02x\n", reg);
		return ret;
	}
	return 0;
}

static void codec_resume_events(struct work_struct *work);
struct workqueue_struct *resume_work_queue;
static DECLARE_WORK(codec_resume_work, codec_resume_events);

enum headphone_mode_u {
	HEADPHONE_IDLE,
	FOUR_HEADPHONE_PLUGIN,
	THREE_HEADPHONE_PLUGIN,
};

struct gpio_switch_data {
	struct device 		*dev;
	//struct i2c_client	*client;
	struct switch_dev sdev;
	int state;
	struct work_struct work;
	struct semaphore sem;
	struct timer_list timer;

	struct work_struct resume_work;
	struct work_struct i2c_work;
	struct input_dev *key;
	enum headphone_mode_u mode;
	/*headset*/
	int virq; /*headset irq*/
	int virq_other;
};

static struct gpio_switch_data *switch_data;
/*
**switch_status_update: update the switch state.
*/
static void switch_status_update(struct gpio_switch_data *para)
{
	struct gpio_switch_data *switch_data = para;
	down(&switch_data->sem);
	switch_set_state(&switch_data->sdev, switch_data->state);
	pr_info("%s,line:%d,switch_data->state:%d\n", __func__, __LINE__,switch_data->state);
	up(&switch_data->sem);
}
static void earphone_switch_work(struct work_struct *work)
{
	struct gpio_switch_data	*switch_data =
		container_of(work, struct gpio_switch_data, work);
	int reg_val;
	reg_val = ac100_rsb_read(HMIC_STS);
	reg_val |= (0x1f<<0);
	ac100_rsb_write(HMIC_STS, reg_val);
	/*disable HBIAS*/
	reg_val = ac100_rsb_read( ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASEN);
	ac100_rsb_write( ADC_APC_CTRL, reg_val);

	/*Disable Headset microphone BIAS Current sensor & ADC*/
	reg_val = ac100_rsb_read( ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASADCEN);
	ac100_rsb_write( ADC_APC_CTRL, reg_val);
	msleep(100);
	if (gpio_get_value(item_eint.gpio.gpio) == 0) {
		msleep(400);
		if (gpio_get_value(item_eint.gpio.gpio) == 0) {
			/*enable HBIAS*/
			reg_val = ac100_rsb_read( ADC_APC_CTRL);
			reg_val |= 0x1<<HBIASEN;
			ac100_rsb_write( ADC_APC_CTRL, reg_val);

			/*enable Headset microphone BIAS Current sensor & ADC*/
			reg_val = ac100_rsb_read( ADC_APC_CTRL);
			reg_val |= 0x1<<HBIASADCEN;
			ac100_rsb_write( ADC_APC_CTRL, reg_val);
		}
		msleep(100);
		reg_val = ac100_rsb_read(HMIC_STS);
		reg_val = (reg_val>>HMIC_DATA);
		reg_val &= 0x1f;

		if (reg_val >= 0x15) {
			switch_data->state		= 2;
			switch_data->mode = THREE_HEADPHONE_PLUGIN;
		} else if(reg_val>=0x1 && reg_val<0x15) {
			switch_data->mode = FOUR_HEADPHONE_PLUGIN;
			switch_data->state		= 1;
			reg_val = ac100_rsb_read( HMIC_CTRL1);
			reg_val |= (0x1<<HMIC_KEYDOWN_IRQ);
			reg_val |= (0x1<<HMIC_KEYUP_IRQ);
			ac100_rsb_write( HMIC_CTRL1, reg_val);

		} else {
			switch_data->mode = HEADPHONE_IDLE;
			switch_data->state = 0;
		}

		if (switch_data->state	!= 1) {
			reg_val = ac100_rsb_read( HMIC_CTRL1);
			reg_val &= ~(0x1<<HMIC_KEYDOWN_IRQ);
			reg_val &= ~(0x1<<HMIC_KEYUP_IRQ);
			ac100_rsb_write( HMIC_CTRL1, reg_val);

			/*Headset microphone BIAS Enable*/
			reg_val = ac100_rsb_read( ADC_APC_CTRL);
			reg_val &= ~(0x1<<HBIASEN);
			ac100_rsb_write( ADC_APC_CTRL, reg_val);

			/*Headset microphone BIAS Current sensor & ADC Enable*/
			reg_val = ac100_rsb_read( ADC_APC_CTRL);
			reg_val &= ~(0x1<<HBIASADCEN);
			ac100_rsb_write( ADC_APC_CTRL, reg_val);
		}
	}else{

		switch_data->state		= 0;
		/*Headset microphone BIAS Enable*/
		reg_val = ac100_rsb_read( ADC_APC_CTRL);
		reg_val &= ~(0x1<<HBIASEN);
		ac100_rsb_write( ADC_APC_CTRL, reg_val);

		/*Headset microphone BIAS Current sensor & ADC Enable*/
		reg_val = ac100_rsb_read( ADC_APC_CTRL);
		reg_val &= ~(0x1<<HBIASADCEN);
		ac100_rsb_write( ADC_APC_CTRL, reg_val);
	}
	switch_status_update(switch_data);
}
static irqreturn_t audio_hmic_irq(int irq, void *para)
{

	struct gpio_switch_data *switch_data = (struct gpio_switch_data *)para;
	if (switch_data == NULL) {
		return IRQ_NONE;
	}
	printk("%s,line:%d\n",__func__,__LINE__);
	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}
static irqreturn_t hook_volume_irq(int irq, void *data)
{
	struct gpio_switch_data	*switch_data = data;
	int reg_val;
	msleep(50);
	reg_val = ac100_rsb_read(HMIC_STS);
	reg_val |= (0x1f<<0);
	ac100_rsb_write(HMIC_STS, reg_val);

	reg_val = ac100_rsb_read(HMIC_STS);
	reg_val = (reg_val>>HMIC_DATA);
	reg_val &= 0x1f;
	printk("%s,line:%d,reg_val:%d\n",__func__,__LINE__,reg_val);
	if ((reg_val >= 0x19) && switch_data->state == 1) {
		input_report_key(switch_data->key, KEY_HEADSETHOOK, 1);
		input_sync(switch_data->key);
		KEY_HOOK_FLAG = 1;
		printk("*******hook*******1\n");
	} else if ((reg_val == 0x15 || reg_val == 0x14) && switch_data->state == 1) {
		KEY_VOLUME_DOWN_FLAG = 1;
		input_report_key(switch_data->key, KEY_VOLUMEDOWN, 1);
		input_sync(switch_data->key);
		printk("--------------1\n");
	} else if ((reg_val == 0x18 || reg_val == 0x17) && switch_data->state == 1) {
		input_report_key(switch_data->key, KEY_VOLUMEUP, 1);
		input_sync(switch_data->key);
		KEY_VOLUME_UP_FLAG = 1;
		printk("+++++++++++++++1\n");
	}else if (reg_val < 0xb && switch_data->state == 1) {
		if (KEY_HOOK_FLAG == 1){
			input_report_key(switch_data->key, KEY_HEADSETHOOK, 0);
			input_sync(switch_data->key);
			KEY_HOOK_FLAG = 0;
			printk("*******hook*******2\n");
		}
		if (KEY_VOLUME_DOWN_FLAG == 1) {

			input_report_key(switch_data->key, KEY_VOLUMEDOWN, 0);
			input_sync(switch_data->key);
			KEY_VOLUME_DOWN_FLAG = 0;
			printk("--------------2\n");
		}

		if (KEY_VOLUME_UP_FLAG == 1) {
			input_report_key(switch_data->key, KEY_VOLUMEUP, 0);
			input_sync(switch_data->key);
			KEY_VOLUME_UP_FLAG = 0;
			printk("+++++++++++++++2\n");
		}
		KEY_HOOK_FLAG = 0;
		KEY_VOLUME_DOWN_FLAG = 0;
		KEY_VOLUME_UP_FLAG = 0;
	}

	/*clear audio-irq pending bit*/
	//writel((0x1<<12), ((void __iomem *)0xf1f02c00+0x214));
	return IRQ_HANDLED;
}

static void codec_resume_events(struct work_struct *work)
{
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);

	return sprintf(buf, "%d\n", switch_data->state);
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);

	return sprintf(buf, "%s\n", switch_data->sdev.name);
}
static int gpio_switch_probe(struct platform_device *pdev)
{
	int ret = 0;
	int req_status = 0;
	int reg_val;
	script_item_value_type_e  type;
	if (arisc_rsb_set_rtsaddr(RSB_DEVICE_SADDR7, RSB_RTSADDR_AC100)) {
		pr_err("switch config codec failed\n");
	}
	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data) {
		pr_err("%s,line:%d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, (void *)switch_data);
	switch_data->sdev.state 		= 0;
	switch_data->state				= -1;
	switch_data->sdev.name 			= "h2w";
	switch_data->sdev.print_name 	= print_headset_name;
	switch_data->sdev.print_state 	= switch_gpio_print_state;
	switch_data->dev = &pdev->dev;
 	/* create input device */
	switch_data->key = input_allocate_device();
	if (!switch_data->key) {
		pr_err("gpio_switch_probe: not enough memory for input device\n");
		goto err_input_allocate_device;
	}

	switch_data->key->name          = "headset";
	switch_data->key->phys          = "headset/input0";
	switch_data->key->id.bustype    = BUS_HOST;
	switch_data->key->id.vendor     = 0x0001;
	switch_data->key->id.product    = 0xffff;
	switch_data->key->id.version    = 0x0100;
	switch_data->key->evbit[0] = BIT_MASK(EV_KEY);
	set_bit(KEY_HEADSETHOOK, switch_data->key->keybit);
	set_bit(KEY_VOLUMEUP, switch_data->key->keybit);
	set_bit(KEY_VOLUMEDOWN, switch_data->key->keybit);
	ret = input_register_device(switch_data->key);
	if (ret) {
		pr_err("gpio_switch_probe: input_register_device failed\n");
	        goto err_input_register_device;
	}
	sema_init(&switch_data->sem, 1);
	INIT_WORK(&switch_data->work, earphone_switch_work);
	ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0) {
		pr_err("gpio_switch_probe: switch_dev_register failed\n");
		goto err_switch_dev_register;
	}

	type = script_get_item("audio0", "ac100_gpio_ctrl", &item_eint);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[switch_data] script_get_item return type err\n");
		return -EFAULT;
	}

	switch_data->virq = gpio_to_irq(item_eint.gpio.gpio);
	if (IS_ERR_VALUE(switch_data->virq)) {
		pr_warn("[switch_data] map gpio [%d] to virq failed, errno = %d\n",
		GPIOG(12), switch_data->virq);
		return -EINVAL;
	}
	printk("[switch_data] gpio [%d] map to virq [%d] ok\n", item_eint.gpio.gpio, switch_data->virq);
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(&pdev->dev, switch_data->virq, audio_hmic_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "PG12_EINT", switch_data);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("[switch_data] request virq %d failed, errno = %d\n", switch_data->virq, ret);
		goto err_request_irq;
	}
	req_status = gpio_request(item_eint.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_warn("[switch_data]request gpio[%d] failed!\n", item_eint.gpio.gpio);
		return -EINVAL;
	}

	gpio_set_debounce(item_eint.gpio.gpio, 1);

	type = script_get_item("audio0", "audio_int_ctrl", &item_eint_other);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[switch_data] script_get_item return type err\n");
		return -EFAULT;
	}

	switch_data->virq_other = gpio_to_irq(item_eint_other.gpio.gpio);
	if (IS_ERR_VALUE(switch_data->virq_other)) {
		pr_warn("[switch_data] map gpio [%d] to virq failed, errno = %d\n",
		GPIOG(12), switch_data->virq_other);
		return -EINVAL;
	}
	printk("[switch_data] gpio [%d] map to virq [%d] ok\n", item_eint_other.gpio.gpio, switch_data->virq_other);

	ret = request_threaded_irq(switch_data->virq_other, NULL, hook_volume_irq,
				     IRQF_TRIGGER_FALLING,
				     "PL12_EINT", switch_data);
	if (ret) {
		pr_err("[switch_data] failed to request IRQ , ret: %d\n",ret);
	}
	req_status = gpio_request(item_eint_other.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("[switch_data]request gpio[%d] failed!\n", item_eint_other.gpio.gpio);
		return -EINVAL;
	}
	gpio_set_debounce(item_eint_other.gpio.gpio, 1);
	/*ac100 initial*/
	reg_val = ac100_rsb_read(OMIXER_BST1_CTRL);
	reg_val |= (0xf<<12);
	ac100_rsb_write(OMIXER_BST1_CTRL, reg_val);
	/*53 part*/
	/*debounce when Key down or keyup*/
	reg_val = ac100_rsb_read(HMIC_CTRL1);
	reg_val &= (0xf<<HMIC_M);
	reg_val |= (0x0<<HMIC_M);
	ac100_rsb_write( HMIC_CTRL1, reg_val);

	/*debounce when earphone plugin or pullout*/
	reg_val = ac100_rsb_read(HMIC_CTRL1);
	reg_val &= (0xf<<HMIC_N);
	reg_val |= (0x0<<HMIC_N);
	ac100_rsb_write(HMIC_CTRL1, reg_val);

	/*Down Sample Setting Select/11:Downby 8,16Hz*/
	reg_val = ac100_rsb_read(HMIC_CTRL2);
	reg_val &= ~(0x3<<HMIC_SAMPLE_SELECT);
	reg_val |= (0x3<<HMIC_SAMPLE_SELECT);
	ac100_rsb_write(HMIC_CTRL2, reg_val);

	/*Hmic_th2 for detecting Keydown or Keyup.*/
	reg_val = ac100_rsb_read(HMIC_CTRL2);
	reg_val &= ~(0x1f<<HMIC_TH2);
	reg_val |= (0xf<<HMIC_TH2);
	ac100_rsb_write(HMIC_CTRL2, reg_val);

	/*Hmic_th1[4:0],detecting eraphone plugin or pullout*/
	//reg_val = ac100_rsb_read( HMIC_CTRL2);
	//reg_val &= ~(0x1f<<HMIC_TH1);
	//reg_val |= (0x1<<HMIC_TH1);
	//ac100_rsb_write( HMIC_CTRL2, reg_val);
	/*Hmic KeyUp/key down Irq Enable*/
	reg_val = ac100_rsb_read( HMIC_CTRL1);
	reg_val &= ~(0x1<<HMIC_KEYDOWN_IRQ);
	reg_val &= ~(0x1<<HMIC_KEYUP_IRQ);
	ac100_rsb_write( HMIC_CTRL1, reg_val);

	reg_val = ac100_rsb_read(HMIC_STS);
	reg_val |= (0x1f<<0);
	ac100_rsb_write(HMIC_STS, reg_val);

	schedule_work(&switch_data->work);
	return 0;

err_request_irq:
	switch_dev_unregister(&switch_data->sdev);
err_switch_dev_register:
	input_unregister_device(switch_data->key);

err_input_register_device:
        input_free_device(switch_data->key);

err_input_allocate_device:
	kfree(switch_data);

	return ret;
}

static int switch_suspend(struct platform_device *pdev,pm_message_t state)
{
	int reg_val;
	unsigned long config;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	/* check if called in talking standby */
	struct gpio_switch_data *switch_data = dev_get_drvdata(&pdev->dev);
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		printk("In talking standby, do not suspend!!\n");
		return 0;
	}
	devm_free_irq(switch_data->dev,switch_data->virq,switch_data);
	gpio_free(item_eint.gpio.gpio);
	free_irq(switch_data->virq_other,switch_data);
	gpio_free(item_eint_other.gpio.gpio);
	/*Disable Headset microphone BIAS*/
	reg_val = ac100_rsb_read( ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASEN);
	ac100_rsb_write( ADC_APC_CTRL, reg_val);

	/*Disable Headset microphone BIAS Current sensor & ADC*/
	reg_val = ac100_rsb_read( ADC_APC_CTRL);
	reg_val &= ~(0x1<<HBIASADCEN);
	ac100_rsb_write( ADC_APC_CTRL, reg_val);

	sunxi_gpio_to_name(item_eint.gpio.gpio, pin_name);
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
	pin_config_set(SUNXI_PINCTRL, pin_name, config);

	sunxi_gpio_to_name(item_eint_other.gpio.gpio, pin_name);
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
	pin_config_set(SUNXI_PINCTRL, pin_name, config);

	return 0;
}

static int switch_resume(struct platform_device *pdev )
{
	int req_status = 0;
	int ret = 0;
	int reg_val;
	struct gpio_switch_data *switch_data = dev_get_drvdata(&pdev->dev);
	switch_data->virq = gpio_to_irq(item_eint.gpio.gpio);
	if (IS_ERR_VALUE(switch_data->virq)) {
		pr_warn("[switch_data] map gpio [%d] to virq failed, errno = %d\n",
		GPIOG(12), switch_data->virq);
		return -EINVAL;
	}
	printk("[switch_data] gpio [%d] map to virq [%d] ok\n", item_eint.gpio.gpio, switch_data->virq);
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(&pdev->dev, switch_data->virq, audio_hmic_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "PG12_EINT", switch_data);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("[switch_data] request virq %d failed, errno = %d\n", switch_data->virq, ret);
	}
	req_status = gpio_request(item_eint.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_warn("[switch_data]request gpio[%d] failed!\n", item_eint.gpio.gpio);
		return -EINVAL;
	}
	gpio_set_debounce(item_eint.gpio.gpio, 1);

	switch_data->virq_other = gpio_to_irq(item_eint_other.gpio.gpio);
	if (IS_ERR_VALUE(switch_data->virq_other)) {
		pr_warn("[switch_data] map gpio [%d] to virq failed, errno = %d\n",
		GPIOG(12), switch_data->virq_other);
		return -EINVAL;
	}
	printk("[switch_data] gpio [%d] map to virq [%d] ok\n", item_eint_other.gpio.gpio, switch_data->virq_other);
	ret = request_threaded_irq(switch_data->virq_other, NULL, hook_volume_irq,
				     IRQF_TRIGGER_FALLING, "PL12_EINT", switch_data);
	if (ret) {
		pr_err("[switch_data] failed to request IRQ , ret: %d\n",ret);
	}
	req_status = gpio_request(item_eint_other.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_warn("[switch_data]request gpio[%d] failed!\n", item_eint_other.gpio.gpio);
		return -EINVAL;
	}
	gpio_set_debounce(item_eint_other.gpio.gpio, 1);
	/*ac100 initial*/
	reg_val = ac100_rsb_read(OMIXER_BST1_CTRL);
	reg_val |= (0xf<<12);
	ac100_rsb_write(OMIXER_BST1_CTRL, reg_val);
	/*53 part*/
	/*debounce when Key down or keyup*/
	reg_val = ac100_rsb_read(HMIC_CTRL1);
	reg_val &= (0xf<<HMIC_M);
	reg_val |= (0x0<<HMIC_M);
	ac100_rsb_write( HMIC_CTRL1, reg_val);

	/*debounce when earphone plugin or pullout*/
	reg_val = ac100_rsb_read(HMIC_CTRL1);
	reg_val &= (0xf<<HMIC_N);
	reg_val |= (0x0<<HMIC_N);
	ac100_rsb_write(HMIC_CTRL1, reg_val);

	/*Down Sample Setting Select/11:Downby 8,16Hz*/
	reg_val = ac100_rsb_read(HMIC_CTRL2);
	reg_val &= ~(0x3<<HMIC_SAMPLE_SELECT);
	reg_val |= (0x3<<HMIC_SAMPLE_SELECT);
	ac100_rsb_write(HMIC_CTRL2, reg_val);

	/*Hmic_th2 for detecting Keydown or Keyup.*/
	reg_val = ac100_rsb_read(HMIC_CTRL2);
	reg_val &= ~(0x1f<<HMIC_TH2);
	reg_val |= (0xf<<HMIC_TH2);
	ac100_rsb_write(HMIC_CTRL2, reg_val);

	reg_val = ac100_rsb_read( HMIC_CTRL1);
	reg_val &= ~(0x1<<HMIC_KEYDOWN_IRQ);
	reg_val &= ~(0x1<<HMIC_KEYUP_IRQ);
	ac100_rsb_write( HMIC_CTRL1, reg_val);

	reg_val = ac100_rsb_read(HMIC_STS);
	reg_val |= (0x1f<<0);
	ac100_rsb_write(HMIC_STS, reg_val);

	schedule_work(&switch_data->work);
	return 0;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = dev_get_drvdata(&pdev->dev);
	switch_dev_unregister(&switch_data->sdev);
	input_unregister_device(switch_data->key);
	input_free_device(switch_data->key);
	kfree(switch_data);
	return 0;
}

static struct platform_driver switch_gpio_driver = {
	.probe		= gpio_switch_probe,
	.suspend	= switch_suspend,
	.resume		= switch_resume,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "sunxi-switch-gpio",
		.owner	= THIS_MODULE,
	},
};
static struct platform_device switch_gpio_device = {
    .name = "sunxi-switch-gpio",
};
static int __init ac100_gpio_switch_init(void)
{
	int ret;
	if((ret = platform_device_register(&switch_gpio_device)) < 0)
		return ret;
	if((ret = platform_driver_register(&switch_gpio_driver)) < 0)
		return ret;
	return 0;
}

static void __exit ac100_gpio_switch_exit(void)
{
	platform_driver_unregister(&switch_gpio_driver);
	platform_device_unregister(&switch_gpio_device);
}

MODULE_AUTHOR("liushaohua");
MODULE_DESCRIPTION("Allwinner switch driver");
MODULE_LICENSE("GPL");

module_init(ac100_gpio_switch_init);
module_exit(ac100_gpio_switch_exit);


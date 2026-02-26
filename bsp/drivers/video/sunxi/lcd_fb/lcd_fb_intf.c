/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "lcd_fb_intf.h"
#include "dev_lcd_fb.h"

void *lcd_fb_malloc(u32 size)
{
	return kmalloc(size, GFP_KERNEL | __GFP_ZERO);
}

/**
 *				lcd_fb_register_irq
 *
 * Description:
 * irq register
 *
 * Parameters:
 *	irqno		input.		irq no
 *	flags		input.
 *	Handler		input.		isr handler
 *	pArg		input.		para
 *	DataSize	input.		len of para
 *	prio		input.		priority
 *
 * Return value:
 *
 *
 * note:
 *	typedef s32 (*ISRCallback)( void *pArg)
 *
 */
int lcd_fb_register_irq(u32 IrqNo, u32 Flags, void *Handler, void *pArg,
			  u32 DataSize, u32 Prio)
{
	lcd_fb_inf("%s, irqNo=%d, Handler=0x%p, pArg=0x%p\n", __func__, IrqNo,
	      Handler, pArg);
	return request_irq(IrqNo, (irq_handler_t) Handler, 0x0,
			   "dispaly", pArg);
}

/**
 *				lcd_fb_unregister_irq
 *
 * Description:
 *	irq unregister
 *
 * Parameters:
 *	irqno		input.		irq no
 *	handler		input.		isr handler
 *	Argment		input.		para
 *
 * Return value:
 *	void
 *
 * note:
 *	void
 *
 */
void lcd_fb_unregister_irq(u32 IrqNo, void *Handler, void *pArg)
{
	free_irq(IrqNo, pArg);
}

/**
 *			lcd_fb_enable_irq
 *
 * Description:
 *	enable irq
 *
 * Parameters:
 *	irqno input.  irq no
 *
 * Return value:
 *	void
 *
 * note:
 *	void
 *
 */
void lcd_fb_enable_irq(u32 IrqNo)
{
	/* enable_irq(IrqNo); */
}

/**
 *			lcd_fb_disable_irq
 *
 * Description:
 *	disable irq
 *
 * Parameters:
 *	irqno		input.		irq no
 *
 * Return value:
 *	void
 *
 * note:
 *	void
 *
 */
void lcd_fb_disable_irq(u32 IrqNo)
{
	/* disable_irq(IrqNo); */
}

/* type: 0:invalid, 1: int; 2:str, 3: gpio */
int lcd_fb_script_get_item(char *main_name, char *sub_name, int value[],
			     int type)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	int ret = 0;
	enum of_gpio_flags flags;

	len = sprintf(compat, "allwinner,sunxi-%s", main_name);
	if (len > 32)
		lcd_fb_wrn("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		lcd_fb_wrn("of_find_compatible_node %s fail\n", compat);
		return ret;
	}

	if (type == 1) {
		if (of_property_read_u32_array(node, sub_name, value, 1))
			lcd_fb_inf("of_property_read_u32_array %s.%s fail\n",
			      main_name, sub_name);
		else
			ret = type;
	} else if (type == 2) {
		const char *str;

		if (of_property_read_string(node, sub_name, &str))
			lcd_fb_inf("of_property_read_string %s.%s fail\n", main_name,
			      sub_name);
		else {
			ret = type;
			memcpy((void *)value, str, strlen(str) + 1);
		}
	} else if (type == 3) {
		int gpio;
		struct disp_gpio_info *gpio_info = (struct disp_gpio_info *)value;

		/* gpio is invalid by default */
		gpio_info->gpio = -1;
		gpio_info->name[0] = '\0';

		gpio = of_get_named_gpio_flags(node, sub_name, 0, &flags);
		if (!gpio_is_valid(gpio))
			return -EINVAL;

		gpio_info->gpio = gpio;
		memcpy(gpio_info->name, sub_name, strlen(sub_name) + 1);
		gpio_info->value = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		lcd_fb_inf("%s.%s gpio=%d, value:%d\n", main_name, sub_name,
		      gpio_info->gpio, gpio_info->value);
	}

	return ret;
}

int lcd_fb_get_ic_ver(void)
{
	return 0;
}

int lcd_fb_gpio_request(struct disp_gpio_info *gpio_info)
{
	int ret = 0;

	if (!gpio_info) {
		lcd_fb_wrn("%s: gpio_info is null\n", __func__);
		return -1;
	}

	/* As some GPIOs are not essential, here return 0 to avoid error */
	if (!strlen(gpio_info->name))
		return 0;

	if (!gpio_is_valid(gpio_info->gpio)) {
		lcd_fb_wrn("%s: gpio (%d) is invalid\n", __func__, gpio_info->gpio);
		return -1;
	}
	ret = gpio_direction_output(gpio_info->gpio, gpio_info->value);
	if (ret) {
		lcd_fb_wrn("%s failed, gpio_name=%s, gpio=%d, value=%d, ret=%d\n", __func__,
		      gpio_info->name, gpio_info->gpio, gpio_info->value, ret);
		return -1;
	}
	lcd_fb_inf("%s, gpio_name=%s, gpio=%d, value=%d, ret=%d\n", __func__,
		gpio_info->name, gpio_info->gpio, gpio_info->value, ret);

	return ret;
}

int lcd_fb_gpio_request_simple(struct disp_gpio_info *gpio_list,
				 u32 group_count_max)
{
/*
	int ret = 0;
	struct gpio_config pin_cfg;

	if (gpio_list == NULL)
		return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if (ret != 0) {
		lcd_fb_wrn("%s failed, gpio_name=%s, gpio=%d, ret=%d\n", __func__,
		      gpio_list->gpio_name, gpio_list->gpio, ret);
		return ret;
	}

	lcd_fb_inf("%s, gpio_name=%s, gpio=%d, ret=%d\n", __func__,
	      gpio_list->gpio_name, gpio_list->gpio, ret);
	ret = pin_cfg.gpio;

	return ret;
*/
	return 0;
}
int lcd_fb_gpio_release(struct disp_gpio_info *gpio_info)
{
	if (!gpio_info) {
		lcd_fb_wrn("%s: gpio_info is null\n", __func__);
		return -1;
	}
	if (!strlen(gpio_info->name))
		return -1;

	if (!gpio_is_valid(gpio_info->gpio)) {
		lcd_fb_wrn("%s: gpio(%d) is invalid\n", __func__, gpio_info->gpio);
		return -1;
	}

	gpio_free(gpio_info->gpio);
	return 0;
}

/* direction: 0:input, 1:output */
int lcd_fb_gpio_set_direction(u32 p_handler, u32 direction,
				const char *gpio_name)
{
	int ret = -1;

	if (p_handler) {
		if (direction) {
			s32 value;

			value = __gpio_get_value(p_handler);
			ret = gpio_direction_output(p_handler, value);
			if (ret != 0)
				lcd_fb_wrn("gpio_direction_output fail!\n");
		} else {
			ret = gpio_direction_input(p_handler);
			if (ret != 0)
				lcd_fb_wrn("gpio_direction_input fail!\n");
		}
	} else {
		lcd_fb_wrn("OSAL_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
		ret = -1;
	}
	return ret;
}

int lcd_fb_gpio_get_value(u32 p_handler, const char *gpio_name)
{
	if (p_handler)
		return __gpio_get_value(p_handler);
	lcd_fb_wrn("OSAL_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");

	return -1;
}

int lcd_fb_gpio_set_value(u32 p_handler, u32 value_to_gpio,
			    const char *gpio_name)
{
	if (p_handler)
		__gpio_set_value(p_handler, value_to_gpio);
	else
		lcd_fb_wrn("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");

	return 0;
}

int lcd_fb_pin_set_state(char *dev_name, char *name)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	struct platform_device *pdev;
	struct pinctrl *pctl;
	struct pinctrl_state *state;
	int ret = -1;

	len = sprintf(compat, "allwinner,sunxi-%s", dev_name);
	if (len > 32)
		lcd_fb_wrn("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		lcd_fb_wrn("of_find_compatible_node %s fail\n", compat);
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		lcd_fb_wrn("of_find_device_by_node for %s fail\n", compat);
		goto exit;
	}

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		/* not every lcd need pin config */
		lcd_fb_inf("pinctrl_get for %s fail\n", compat);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		lcd_fb_wrn("pinctrl_lookup_state for %s fail\n", compat);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		lcd_fb_wrn("pinctrl_select_state(%s) for %s fail\n", name, compat);
		goto exit;
	}
	ret = 0;
exit:
	return ret;
}

int lcd_fb_power_enable(char *name)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_AW_AXP) || IS_ENABLED(CONFIG_REGULATOR)
	struct regulator *regu = NULL;

#ifdef CONFIG_SUNXI_REGULATOR_DT
	regu = regulator_get(g_drv_info.device, name);
#else
	regu = regulator_get(NULL, name);
#endif
	if (IS_ERR(regu)) {
		lcd_fb_wrn("some error happen, fail to get regulator %s\n", name);
		goto exit;
	}

	/* enalbe regulator */
	ret = regulator_enable(regu);
	if (ret != 0) {
		lcd_fb_wrn("some error happen, fail to enable regulator %s!\n", name);
		goto exit1;
	} else {
		lcd_fb_inf("suceess to enable regulator %s!\n", name);
	}

exit1:
	/* put regulater, when module exit */
	regulator_put(regu);
exit:
#endif
	return ret;
}

int lcd_fb_power_disable(char *name)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_AW_AXP) || IS_ENABLED(CONFIG_REGULATOR)
	struct regulator *regu = NULL;

#ifdef CONFIG_SUNXI_REGULATOR_DT
	regu = regulator_get(g_drv_info.device, name);
#else
	regu = regulator_get(NULL, name);
#endif
	if (IS_ERR(regu)) {
		lcd_fb_wrn("some error happen, fail to get regulator %s\n", name);
		goto exit;
	}
	/* disalbe regulator */
	ret = regulator_disable(regu);
	if (ret != 0) {
		lcd_fb_wrn("some error happen, fail to disable regulator %s!\n", name);
		goto exit1;
	} else {
		lcd_fb_inf("suceess to disable regulator %s!\n", name);
	}

exit1:
	/* put regulater, when module exit */
	regulator_put(regu);
exit:
#endif
	return ret;
}

s32 disp_delay_ms(u32 ms)
{
	u32 timeout = msecs_to_jiffies(ms);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(timeout);
	return 0;
}

s32 disp_delay_us(u32 us)
{
	udelay(us);

	return 0;
}

#if IS_ENABLED(CONFIG_PWM_SUNXI) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) ||              \
	IS_ENABLED(CONFIG_PWM_SUNXI_GROUP) || IS_ENABLED(CONFIG_AW_PWM)
uintptr_t lcd_fb_pwm_request(u32 pwm_id)
{
	uintptr_t ret = 0;

	struct pwm_device *pwm_dev;

	pwm_dev = pwm_request(pwm_id, "lcd");

	if ((pwm_dev == NULL) || IS_ERR(pwm_dev))
		lcd_fb_wrn("lcd_fb_pwm_request pwm %d fail!\n", pwm_id);
	else
		lcd_fb_inf("lcd_fb_pwm_request pwm %d success!\n", pwm_id);
	ret = (uintptr_t) pwm_dev;

	return ret;
}

int lcd_fb_pwm_free(uintptr_t p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		lcd_fb_wrn("lcd_fb_pwm_free, handle is NULL!\n");
		ret = -1;
	} else {
		pwm_free(pwm_dev);
		lcd_fb_inf("lcd_fb_pwm_free pwm %d\n", pwm_dev->pwm);
	}

	return ret;
}

int lcd_fb_pwm_enable(uintptr_t p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		lcd_fb_wrn("lcd_fb_pwm_Enable, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_enable(pwm_dev);
		lcd_fb_inf("lcd_fb_pwm_Enable pwm %d\n", pwm_dev->pwm);
	}

	return ret;
}

int lcd_fb_pwm_disable(uintptr_t p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		lcd_fb_wrn("lcd_fb_pwm_Disable, handle is NULL!\n");
		ret = -1;
	} else {
		pwm_disable(pwm_dev);
		lcd_fb_inf("lcd_fb_pwm_Disable pwm %d\n", pwm_dev->pwm);
	}

	return ret;
}

int lcd_fb_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		lcd_fb_wrn("lcd_fb_pwm_Config, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_config(pwm_dev, duty_ns, period_ns);
		lcd_fb_dbg("lcd_fb_pwm_Config pwm %d, <%d / %d>\n",
			pwm_dev->pwm, duty_ns, period_ns);
	}

	return ret;
}

int lcd_fb_pwm_set_polarity(uintptr_t p_handler, int polarity)
{
	int ret = 0;
	struct pwm_state state;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		lcd_fb_wrn("disp_sys_pwm_Set_Polarity, handle is NULL!\n");
		ret = -1;
	} else {
		memset(&state, 0, sizeof(struct pwm_state));
		pwm_get_state(pwm_dev, &state);
		state.polarity = polarity;
		ret = pwm_apply_state(pwm_dev, &state);
		lcd_fb_wrn("disp_sys_pwm_Set_Polarity pwm %d, active %s\n",
		      pwm_dev->pwm, (polarity == 0) ? "high" : "low");
	}

	return ret;
}
#else
uintptr_t lcd_fb_pwm_request(u32 pwm_id)
{
	uintptr_t ret = 0;

	return ret;
}

int lcd_fb_pwm_free(uintptr_t p_handler)
{
	int ret = 0;

	return ret;
}

int lcd_fb_pwm_enable(uintptr_t p_handler)
{
	int ret = 0;

	return ret;
}

int lcd_fb_pwm_disable(uintptr_t p_handler)
{
	int ret = 0;

	return ret;
}

int lcd_fb_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns)
{
	int ret = 0;

	return ret;
}

int lcd_fb_pwm_set_polarity(uintptr_t p_handler, int polarity)
{
	int ret = 0;

	return ret;
}

#endif

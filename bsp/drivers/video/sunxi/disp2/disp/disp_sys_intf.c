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

#include "dev_disp.h"
#include "de/bsp_display.h"
#include "disp_sys_intf.h"
#include "asm/cacheflush.h"
/* #include <linux/sunxi-gpio.h> */
#include <linux/version.h>
#include <asm-generic/bug.h>
#include <linux/gpio/machine.h>

/* cache flush flags */
#define  CACHE_FLUSH_I_CACHE_REGION       0
#define  CACHE_FLUSH_D_CACHE_REGION       1
#define  CACHE_FLUSH_CACHE_REGION         2
#define  CACHE_CLEAN_D_CACHE_REGION       3
#define  CACHE_CLEAN_FLUSH_D_CACHE_REGION 4
#define  CACHE_CLEAN_FLUSH_CACHE_REGION   5

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
struct pwm_bl_data {
	struct list_head        list;
	struct pwm_device       *pwm;
	struct device           *dev;
	struct disp_device	*disp_dev;
	const char              *pwm_name;
};
static bool pwm_list_init_flag;
struct list_head pwm_list;
#endif

void *disp_sys_malloc(u32 size)
{
	return kmalloc(size, GFP_KERNEL | __GFP_ZERO);
}
/*
*******************************************************************************
*                     OSAL_CacheRangeFlush
*
* Description:
*    Cache flush
*
* Parameters:
*    address    :  start address to be flush
*    length     :  size
*    flags      :  flush flags
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_cache_flush(void *address, u32 length, u32 flags)
{
	if (address == NULL || length == 0)
		return;

	switch (flags) {
	case CACHE_FLUSH_I_CACHE_REGION:
		break;

	case CACHE_FLUSH_D_CACHE_REGION:
		break;

	case CACHE_FLUSH_CACHE_REGION:
		break;

	case CACHE_CLEAN_D_CACHE_REGION:
		break;

	case CACHE_CLEAN_FLUSH_D_CACHE_REGION:
		break;

	case CACHE_CLEAN_FLUSH_CACHE_REGION:
		break;

	default:
		break;
	}
}

/*
*******************************************************************************
*                     disp_sys_register_irq
*
* Description:
*    irq register
*
* Parameters:
*    irqno	    input.  irq no
*    flags	    input.
*    Handler	    input.  isr handler
*    pArg	        input.  para
*    DataSize	    input.  len of para
*    prio	        input.    priority

*
* Return value:
*
*
* note:
*    typedef s32 (*ISRCallback)( void *pArg)
*
*******************************************************************************
*/
int disp_sys_register_irq(u32 IrqNo, u32 Flags, void *Handler, void *pArg,
			  u32 DataSize, u32 Prio)
{
	DE_INFO("irqNo=%d, Handler=0x%p, pArg=0x%p\n", IrqNo,
	      Handler, pArg);
	return request_irq(IrqNo, (irq_handler_t) Handler, 0x0,
			   "dispaly", pArg);
}

/*
*******************************************************************************
*                     disp_sys_unregister_irq
*
* Description:
*    irq unregister
*
* Parameters:
	irqno	input.	irq no
	handler	input.  isr handler
	Argment	input.	para
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_unregister_irq(u32 IrqNo, void *Handler, void *pArg)
{
	free_irq(IrqNo, pArg);
}

/*
*******************************************************************************
*                     disp_sys_enable_irq
*
* Description:
*    enable irq
*
* Parameters:
	irqno	input.	irq no
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_enable_irq(u32 IrqNo)
{
	/* enable_irq(IrqNo); */
}

/*
*******************************************************************************
*                     disp_sys_disable_irq
*
* Description:
*    disable irq
*
* Parameters:
	irqno	input.	irq no
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_disable_irq(u32 IrqNo)
{
	/* disable_irq(IrqNo); */
}

#if IS_ENABLED(CONFIG_AW_DISP2)
/* type: 0:invalid, 1: int; 2:str, 3: gpio , 4, iommu master id*/
int disp_sys_script_get_item(char *main_name, char *sub_name, int value[],
			     int type)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	int ret = 0;
	enum of_gpio_flags flags;

	len = sprintf(compat, "allwinner,sunxi-%s", main_name);
	if (len > 32)
		DE_WARN("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DE_WARN("of_find_compatible_node %s fail\n", compat);
		ret = -1;
		return ret;
	}

	g_disp_drv.node = node;

	if (type == 1) {
		if (of_property_read_u32_array(node, sub_name, value, 1))
			DE_INFO("of_property_read_u32_array %s.%s fail\n",
			      main_name, sub_name);
		else
			ret = type;
	} else if (type == 2) {
		const char *str;

		if (of_property_read_string(node, sub_name, &str))
			DE_INFO("of_property_read_string %s.%s fail\n", main_name,
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
		DE_INFO("%s.%s gpio=%d, value:%d\n", main_name, sub_name,
		      gpio_info->gpio, gpio_info->value);
	} else if (type == 4) {
		if (of_property_read_u32_index(node, sub_name, 1, value)) {
		       DE_INFO("of_property_read_u32_index %s.%s fail\n",
			     main_name, sub_name);
			ret = -1;
		} else
			ret = type;
	}

	return ret;
}
EXPORT_SYMBOL(disp_sys_script_get_item);

/*
 *disp_sys_script_get_item:
 *   There is an array out-of-bounds security issue when obtaining dts strings.
 *
 *disp_sys_script_get_item2:
 *   Used to obtain dts strings. It is recommended to use this interface to
 *   obtain dts strings in the future.
 */

int disp_sys_script_get_item2(char *main_name, char *sub_name, char **value)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	const char *str;

	len = sprintf(compat, "allwinner,sunxi-%s", main_name);
	if (len > 32)
		DE_WARN("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DE_WARN("of_find_compatible_node %s fail\n", compat);
		return -1;
	}

	g_disp_drv.node = node;

	if (of_property_read_string(node, sub_name, &str))
		DE_INFO("of_property_read_string %s.%s fail\n", main_name,
			      sub_name);
	else {
		*value = (char *)kmalloc(strlen(str) + 1, GFP_KERNEL | __GFP_ZERO);
		if (*value == NULL) {
			DE_WARN("malloc memory(%d bytes) fail\n", (unsigned int)sizeof(str));
			goto malloc_err;
		}

		strcpy(*value, str);
	}
	return 0;
malloc_err:
	kfree(*value);
	*value = NULL;

	return -1;
}
EXPORT_SYMBOL(disp_sys_script_get_item2);
#endif

int disp_sys_get_ic_ver(void)
{
	return 0;
}

int disp_sys_gpio_request(struct disp_gpio_info *gpio_info)
{
	int ret = 0;

	if (!gpio_info) {
		DE_WARN("gpio_info is null\n");
		return -1;
	}

	/* As some GPIOs are not essential, here return 0 to avoid error */
	if (!strlen(gpio_info->name))
		return 0;

	if (!gpio_is_valid(gpio_info->gpio)) {
		DE_WARN("gpio (%d) is invalid\n", gpio_info->gpio);
		return -1;
	}

	ret = gpio_direction_output(gpio_info->gpio, gpio_info->value);
	if (ret) {
		DE_WARN("failed, gpio_name=%s, gpio=%d, value=%d, ret=%d\n",
		      gpio_info->name, gpio_info->gpio, gpio_info->value, ret);
		return -1;
	}

	DE_INFO("gpio_name=%s, gpio=%d, value=%d, ret=%d\n",
		gpio_info->name, gpio_info->gpio, gpio_info->value, ret);

	return ret;
}
EXPORT_SYMBOL(disp_sys_gpio_request);

void disp_sys_gpio_release(struct disp_gpio_info *gpio_info)
{
	if (!gpio_info) {
		DE_WARN("gpio_info is null\n");
		return;
	}

	if (!strlen(gpio_info->name))
		return;

	if (!gpio_is_valid(gpio_info->gpio)) {
		DE_WARN("gpio(%d) is invalid\n", gpio_info->gpio);
		return;
	}

	if (gpio_direction_output(gpio_info->gpio, gpio_info->value)) {
		DE_WARN("failed, gpio_name=%s, gpio=%d, value=%d\n",
		      gpio_info->name, gpio_info->gpio, gpio_info->value);
	}

	gpio_free(gpio_info->gpio);
}
EXPORT_SYMBOL(disp_sys_gpio_release);

/* direction: 0:input, 1:output */
int disp_sys_gpio_set_direction(u32 p_handler, u32 direction,
				const char *gpio_name)
{
	int ret = -1;

	if (p_handler) {
		if (direction) {
			s32 value;

			value = __gpio_get_value(p_handler);
			ret = gpio_direction_output(p_handler, value);
			if (ret != 0)
				DE_WARN("gpio_direction_output fail\n");
		} else {
			ret = gpio_direction_input(p_handler);
			if (ret != 0)
				DE_WARN("gpio_direction_input fail\n");
		}
	} else {
		DE_WARN("OSAL_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
		ret = -1;
	}
	return ret;
}

int disp_sys_gpio_get_value(u32 p_handler, const char *gpio_name)
{
	if (p_handler)
		return __gpio_get_value(p_handler);
	DE_WARN("OSAL_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");

	return -1;
}

int disp_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio,
			    const char *gpio_name)
{
	if (p_handler)
		__gpio_set_value(p_handler, value_to_gpio);
	else
		DE_WARN("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");

	return 0;
}

int disp_sys_pin_set_state(char *dev_name, char *name)
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
		DE_WARN("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DE_WARN("of_find_compatible_node %s fail\n", compat);
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		DE_WARN("of_find_device_by_node for %s fail\n", compat);
		goto exit;
	}

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		/* not every lcd need pin config */
		DE_INFO("pinctrl_get for %s fail\n", compat);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		DE_WARN("pinctrl_lookup_state for %s fail\n", compat);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		DE_WARN("pinctrl_select_state(%s) for %s fail\n", name, compat);
		goto exit;
	}
	ret = 0;
exit:
	return ret;
}
EXPORT_SYMBOL(disp_sys_pin_set_state);

int disp_sys_power_enable(struct regulator *regulator)
{
	int ret = 0;

	if (!regulator)
		return 0;

	ret = regulator_enable(regulator);
	WARN(ret, "regulator_enable failed, ret=%d\n", ret);

	return ret;
}

int disp_sys_power_disable(struct regulator *regulator)
{
	int ret = 0;

	if (!regulator)
		return 0;

	ret = regulator_disable(regulator);
	WARN(ret, "regulator_disable failed, ret=%d\n", ret);

	return ret;
}

void aw_memcpy_toio(volatile void __iomem *to, const void *from, size_t count)
{
	while (count && !IS_ALIGNED((uintptr_t)to, 4)) {
		__raw_writeb(*(u8 *)from, to);
		from++;
		to++;
		count--;
	}

	while (count >= 4) {
		writel(*(u32 *)from, to);
		from += 4;
		to += 4;
		count -= 4;
	}

	while (count) {
		__raw_writeb(*(u8 *)from, to);
		from++;
		to++;
		count--;
	}
}

/*
 * Copy data from IO memory space to "real" memory space.
 */
void aw_memcpy_fromio(void *to, const volatile void __iomem *from, size_t count)
{
	while (count && !IS_ALIGNED((uintptr_t)from, 4)) {
		*(u8 *)to = __raw_readb(from);
		from++;
		to++;
		count--;
	}

	while (count >= 4) {
		*(u32 *)to = readl(from);
		from += 4;
		to += 4;
		count -= 4;
	}

	while (count) {
		*(u8 *)to = __raw_readb(from);
		from++;
		to++;
		count--;
	}
}
#if IS_ENABLED(CONFIG_PWM_SUNXI) || IS_ENABLED(CONFIG_PWM_SUNXI_NEW) ||              \
	IS_ENABLED(CONFIG_PWM_SUNXI_GROUP) || IS_ENABLED(CONFIG_AW_PWM)
uintptr_t disp_sys_pwm_request(u32 pwm_id)
{
	uintptr_t ret = 0;

	struct pwm_device *pwm_dev;

	pwm_dev = pwm_request(pwm_id, "lcd");

	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_request pwm %d fail! %ld\n", pwm_id,
		      (long)pwm_dev);
		pwm_dev = NULL;
	} else
		DE_INFO("disp_sys_pwm_request pwm %d success\n", pwm_id);
	ret = (uintptr_t) pwm_dev;

	return ret;
}

uintptr_t disp_sys_pwm_get(char *pwm_name)
{
	uintptr_t ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = devm_pwm_get(g_disp_drv.dev, pwm_name);
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_get %s fail! %ld\n", pwm_name,
				(long)pwm_dev);
		pwm_dev = NULL;
	} else
		DE_INFO("disp_sys_pwm_get %s success\n", pwm_name);
	ret = (uintptr_t) pwm_dev;
	return ret;
}

int disp_sys_pwm_free(uintptr_t p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_free, handle is NULL\n");
		ret = -1;
	} else {
		pwm_free(pwm_dev);
		DE_INFO("disp_sys_pwm_free pwm %d\n", pwm_dev->pwm);
	}

	return ret;
}

int disp_sys_pwm_enable(uintptr_t p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_Enable, handle is NULL\n");
		ret = -1;
	} else {
		ret = pwm_enable(pwm_dev);
		DE_INFO("disp_sys_pwm_Enable pwm %d\n", pwm_dev->pwm);
	}

	return ret;
}

int disp_sys_pwm_disable(uintptr_t p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_Disable, handle is NULL\n");
		ret = -1;
	} else {
		pwm_disable(pwm_dev);
		DE_INFO("disp_sys_pwm_Disable pwm %d\n", pwm_dev->pwm);
	}

	return ret;
}

int disp_sys_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_Config, handle is NULL\n");
		ret = -1;
	} else {
		ret = pwm_config(pwm_dev, duty_ns, period_ns);
		DE_DEBUG("disp_sys_pwm_Config pwm %d, <%d / %d>\n",
			pwm_dev->pwm, duty_ns, period_ns);
	}

	return ret;
}

int disp_sys_pwm_set_polarity(uintptr_t p_handler, int polarity)
{
	int ret = 0;
	struct pwm_state state;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if ((pwm_dev == NULL) || IS_ERR(pwm_dev)) {
		DE_WARN("disp_sys_pwm_Set_Polarity, handle is NULL\n");
		ret = -1;
	} else {
		memset(&state, 0, sizeof(struct pwm_state));
		pwm_get_state(pwm_dev, &state);
		state.polarity = polarity;
		ret = pwm_apply_state(pwm_dev, &state);
		DE_INFO("disp_sys_pwm_Set_Polarity pwm %d, active %s\n",
		      pwm_dev->pwm, (polarity == 0) ? "high" : "low");
	}

	return ret;
}

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
static int pwm_backlight_update_status(struct backlight_device *bl)
{
	struct pwm_bl_data *pb = bl_get_data(bl);
	int brightness = backlight_get_brightness(bl);

	DE_WARN("update backlight brightenss %d\n", brightness);
	if (brightness >= 0)
		pb->disp_dev->set_bright(pb->disp_dev, brightness);

	return 0;
}

static int pwm_backlight_check_fb(struct backlight_device *bl,
		struct fb_info *info)
{
	return 0;
}

static const struct backlight_ops pwm_backlight_ops = {
	.update_status	= pwm_backlight_update_status,
	/* .get_brightness = pwm_backlight_get_brightness, */
	.check_fb	= pwm_backlight_check_fb,
};

int pwm_backlight_probe(struct device *dev, struct disp_device *disp_port,
		const char *backlight_name, uintptr_t pwm_dev)
{
	struct backlight_properties props;
	struct backlight_device *bl;
	struct pwm_bl_data *pb = NULL;

	if (!pwm_list_init_flag) {
		INIT_LIST_HEAD(&pwm_list);
		pwm_list_init_flag = true;
	}

	list_for_each_entry(pb, &pwm_list, list) {
		if (!strcmp(backlight_name, pb->pwm_name))
			return 0;
	}
	pb = devm_kzalloc(dev, sizeof(*pb), GFP_KERNEL);
	if (!pb)
		return 0;

	pb->pwm_name = backlight_name;
	list_add_tail(&pb->list, &pwm_list);
/*
	if (!pb->enable_gpio && gpio_is_valid(gpio_info->gpio)) {
		pb->enable_gpio = gpio_to_desc(gpio_info->gpio);
	}
	gpio_direction_output(gpio_info->gpio, gpio_info->value);
*/
	pb->dev = dev;
	pb->pwm = (struct pwm_device *)pwm_dev;
	pb->disp_dev = disp_port;
	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = 255;

	DE_WARN("register device, name is: %s\n", backlight_name);
	bl = devm_backlight_device_register(dev, backlight_name, dev, pb,
			&pwm_backlight_ops, &props);

	bl->props.brightness = disp_port->get_bright(disp_port);
	backlight_update_status(bl);

	return 0;
}
#endif

#else
uintptr_t disp_sys_pwm_request(u32 pwm_id)
{
	uintptr_t ret = 0;

	return ret;
}

int disp_sys_pwm_free(uintptr_t p_handler)
{
	int ret = 0;

	return ret;
}

int disp_sys_pwm_enable(uintptr_t p_handler)
{
	int ret = 0;

	return ret;
}

int disp_sys_pwm_disable(uintptr_t p_handler)
{
	int ret = 0;

	return ret;
}

int disp_sys_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns)
{
	int ret = 0;

	return ret;
}

int disp_sys_pwm_set_polarity(uintptr_t p_handler, int polarity)
{
	int ret = 0;

	return ret;
}
#endif

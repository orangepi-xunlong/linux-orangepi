/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <drm/drmP.h>
#include "asm/cacheflush.h"

#include <linux/clk-private.h>
#include <linux/clk/sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/sys_config.h>
#include <linux/pwm.h>

#include "../sunxi_drm_core.h"
#include "sunxi_drm_panel.h"
#include "sunxi_common.h"

static char drm_irq[][10] = {
	/*DISP_MOD_DE*/{"drm-de"},
	/*DISP_MOD_DEVICE*/{"drm-tcon"},
	/*DISP_MOD_LCD0*/{"drm-tcon0"},
	/*DISP_MOD_LCD1*/{"drm-tcon1"},
	/*DISP_MOD_LCD2*/{"drm-tcon2"},
	/*DISP_MOD_LCD3*/"drm-tcon3",
	/*DISP_MOD_DSI0*/"drm-dsi0",
	/*DISP_MOD_DSI1*/"drm-dsi1",
	/*DISP_MOD_DSI2*/"drm-dsi3",
	/*DISP_MOD_HDMI*/"drm-hdmi",
	/*DISP_MOD_LVDS*/"drm-lvds",
	/*DISP_MOD_EINK*/"drm-elink",
	/*DISP_MOD_EDMA*/"drm-edma",
};

int sunxi_clk_set(struct sunxi_hardware_res *hw_res)
{
	unsigned long clk_set;
	if(hw_res && hw_res->parent_clk) {
		clk_set_rate(hw_res->parent_clk, hw_res->parent_clk_rate);
		clk_set = clk_get_rate(hw_res->parent_clk);
		if (clk_set != hw_res->parent_clk_rate) {
			DRM_INFO("set parent clk:%lu, but get:%lu.\n",hw_res->parent_clk_rate, clk_set); 
			}
	}
	if (hw_res && hw_res->clk) {
		clk_set_rate(hw_res->clk, hw_res->clk_rate);
		clk_set = clk_get_rate(hw_res->clk);
		if (clk_set != hw_res->clk_rate){
			DRM_INFO("set clk:%lu, but get:%lu.\n",hw_res->clk_rate, clk_set);
		}
	}

	return 0;
}

void sunxi_clk_enable(struct sunxi_hardware_res *hw_res)
{
	if (hw_res && !hw_res->clk_enable && hw_res->clk) {
		clk_prepare_enable(hw_res->clk);
		hw_res->clk_enable = 1;
	}
}

void sunxi_clk_disable(struct sunxi_hardware_res *hw_res)
{
	if (hw_res && hw_res->clk_enable && hw_res->clk) {
		clk_disable_unprepare(hw_res->clk);
		hw_res->clk_enable = 0;
	}
}

void sunxi_irq_enable(struct sunxi_hardware_res *hw_res)
{
	if (hw_res && hw_res->irq_uesd && !hw_res->irq_enable) {
		enable_irq(hw_res->irq_no);
		hw_res->irq_enable = 1;
	}
}

void sunxi_irq_disable(struct sunxi_hardware_res *hw_res)
{
	if (hw_res && hw_res->irq_enable) {
		disable_irq(hw_res->irq_no);
		hw_res->irq_enable = 0;
	}
}

int sunxi_irq_request(struct sunxi_hardware_res  *hw_res)
{
	unsigned int irq_no;
	void *irq_arg;
	irq_handler_t irq_handle;

	irq_no = hw_res->irq_no;
	irq_arg = hw_res->irq_arg;
	irq_handle = hw_res->irq_handle;
	if (hw_res && !hw_res->irq_uesd) {
		request_irq(irq_no, (irq_handler_t)irq_handle,
			IRQF_DISABLED, drm_irq[hw_res->res_id], irq_arg);
		/* make sure that after request,the irq enbaled ? */
		hw_res->irq_enable = 1;
		hw_res->irq_uesd = 1;
	}

	return 0;
}

bool sunxi_irq_query(struct sunxi_hardware_res *hw_res,
        void *irq_data, int need_irq)
{
	bool status = true;
	if (hw_res && hw_res->ops && hw_res->ops->irq_query)
		status = hw_res->ops->irq_query(irq_data,need_irq);
	return status;
}

int sunxi_irq_free(struct sunxi_hardware_res *hw_res)
{
    
	if (hw_res && hw_res->irq_uesd) {
		free_irq(hw_res->irq_no, hw_res->irq_arg);
		hw_res->irq_enable = 0;
		hw_res->irq_uesd = 0;
	}

	return 0;
}

void sunxi_drm_delayed_ms(unsigned int ms)
{
	unsigned int timeout = msecs_to_jiffies(ms);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(timeout);
}

int sunxi_drm_sys_power_enable(char *name)
{
	int ret = 0;
#ifdef CONFIG_AW_AXP
	struct regulator *regu= NULL;
	regu= regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		DRM_ERROR("fail to get regulator %s\n", name);
		goto exit;
	}

	ret = regulator_enable(regu);
	if (0 != ret) {
		DRM_ERROR("fail to enable regulator %s!\n", name);
		goto exit1;
	} else {
		DRM_INFO("suceess to enable regulator %s!\n", name);
	}

exit1:
	regulator_put(regu);
exit:
#endif
	return ret;
}

int sunxi_drm_sys_power_disable(char *name)
{
	int ret = 0;
#ifdef CONFIG_AW_AXP
	struct regulator *regu= NULL;

	regu= regulator_get(NULL, name);
	if (IS_ERR(regu)) {
		DRM_ERROR("fail to get regulator %s\n", name);
		goto exit;
	}

	ret = regulator_disable(regu);
	if (0 != ret) {
		DRM_ERROR("fail to disable regulator %s!\n", name);
		goto exit1;
	} else {
		DRM_INFO("suceess to disable regulator %s!\n", name);
	}

exit1:
	regulator_put(regu);
exit:
#endif
	return ret;
}

int sunxi_drm_sys_gpio_request(disp_gpio_set_t *gpio_list)
{
	int ret = 0;
	struct gpio_config pin_cfg;
	char   pin_name[32];
	u32 config;

	if (gpio_list == NULL)
		return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if (0 != ret) {
		DRM_ERROR("failed, gpio_name=%s, gpio=%d, ret=%d\n",
		gpio_list->gpio_name, gpio_list->gpio, ret);
		return 0;
	} else {
		DRM_INFO("%s, gpio_name=%s, gpio=%d, <%d,%d,%d,%d>ret=%d\n", __func__,
		gpio_list->gpio_name, gpio_list->gpio,
		gpio_list->mul_sel, gpio_list->pull, gpio_list->drv_level, gpio_list->data, ret);
	}
	ret = pin_cfg.gpio;

	if (!IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of sunxi-pinctrl,
		* config pin attributes individually.
		*/
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg.mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg.pull != GPIO_PULL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg.pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.drv_level != GPIO_DRVLVL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg.drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	} else if (IS_AXP_PIN(pin_cfg.gpio)) {
		/* valid pin of axp-pinctrl,
		* config pin attributes individually.
		*/
		sunxi_gpio_to_name(pin_cfg.gpio, pin_name);
		if (pin_cfg.data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg.data);
			pin_config_set(AXP_PINCTRL, pin_name, config);
		}
	} else {
		DRM_ERROR("invalid pin [%d] from sys-config\n", pin_cfg.gpio);
	}

	return ret;
}

int sunxi_drm_sys_gpio_release(int p_handler)
{
	if (p_handler) {
		gpio_free(p_handler);
	} else {
		DRM_INFO("OSAL_GPIO_Release, hdl is NULL\n");
	}
	return 0;
}

int sunxi_drm_sys_pin_set_state(char *dev_name, char *name)
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
		DRM_ERROR("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DRM_ERROR("of_find_compatible_node %s fail\n", compat);
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		DRM_ERROR("of_find_device_by_node for %s fail\n", compat);
		goto exit;
	}

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		DRM_ERROR("pinctrl_get for %s fail\n", compat);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		DRM_ERROR("pinctrl_lookup_state for %s fail\n", compat);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		DRM_ERROR("pinctrl_select_state(%s) for %s fail\n", name, compat);
		goto exit;
	}
	ret = 0;

exit:
	return ret;
}

int sunxi_drm_get_sys_item_int(struct device_node *node, char *sub_name, int *value)
{

	return of_property_read_u32_array(node, sub_name, value, 1);
}

int sunxi_drm_get_sys_item_char(struct device_node *node, char *sub_name, char *value)
{
	const char *str;
	if (of_property_read_string(node, sub_name, &str)) {
		DRM_INFO("failed to get [%s] string.\n", sub_name); 
		return  -EINVAL;
	}

	memcpy((void*)value, str, strlen(str)+1);

	return 0;
}

int sunxi_drm_get_sys_item_gpio(struct device_node *node, char *sub_name, disp_gpio_set_t *gpio_info)
{
	int gpio;
	struct gpio_config config;

	gpio = of_get_named_gpio_flags(node, sub_name, 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(gpio)) {
		DRM_INFO("failed to get gpio[%s].\n",sub_name); 
		return -EINVAL;
	}

	gpio_info->gpio = config.gpio;
	gpio_info->mul_sel = config.mul_sel;
	gpio_info->pull = config.pull;
	gpio_info->drv_level = config.drv_level;
	gpio_info->data = config.data;
	memcpy(gpio_info->gpio_name, sub_name, strlen(sub_name)+1);

	return 0;
}

struct device_node *sunxi_drm_get_name_node(char *device_name)
{
	struct device_node *node = NULL;
	char compat[32];
	u32 len = 0;

	len = sprintf(compat, "allwinner,%s", device_name);
	if (len > 32)
		DRM_INFO("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		DRM_ERROR("of_find_compatible_node %s fail\n", compat);
		return NULL;
	}

	return node;
}

int sunxi_drm_sys_pwm_enable(struct pwm_info_t *pwm_info)
{
	if (pwm_info->pwm_dev && !pwm_info->enabled) {
		pwm_info->enabled = 1;
		return pwm_enable(pwm_info->pwm_dev);
	}
	return 0;
}

int sunxi_drm_sys_pwm_disable(struct pwm_info_t *pwm_info)
{
	if (pwm_info->pwm_dev && pwm_info->enabled) {
		pwm_info->enabled = 0;
		pwm_disable(pwm_info->pwm_dev);
	}
	return 0;
}

int sunxi_drm_sys_pwm_config(struct pwm_device *pwm_dev, int duty_ns, int period_ns)
{
	if (NULL == pwm_dev || IS_ERR(pwm_dev)) {
		DRM_ERROR("set pwm err.\n");
		return -EINVAL;
	}
	return pwm_config(pwm_dev, duty_ns, period_ns);
}

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

#ifndef _SUNXI_COMMON_H_
#define _SUNXI_COMMON_H_

int sunxi_clk_set(struct sunxi_hardware_res *hw_res);

void sunxi_clk_enable(struct sunxi_hardware_res *hw_res);

void sunxi_clk_disable(struct sunxi_hardware_res *hw_res);

void sunxi_irq_enable(struct sunxi_hardware_res *hw_res);

void sunxi_irq_disable(struct sunxi_hardware_res *hw_res);

int sunxi_irq_free(struct sunxi_hardware_res  *hw_res);

int sunxi_irq_request(struct sunxi_hardware_res  *hw_res);

struct device_node *sunxi_drm_get_name_node(char *device_name);

int sunxi_drm_get_sys_item_gpio(struct device_node *node, char *sub_name, disp_gpio_set_t *gpio_info);

int sunxi_drm_get_sys_item_char(struct device_node *node, char *sub_name, char *value);

int sunxi_drm_get_sys_item_int(struct device_node *node, char *sub_name, int *value);

int sunxi_drm_sys_pin_set_state(char *dev_name, char *name);

int sunxi_drm_sys_gpio_release(int p_handler);

int sunxi_drm_sys_gpio_request(disp_gpio_set_t *gpio_list);

int sunxi_drm_sys_power_disable(char *name);

int sunxi_drm_sys_power_enable(char *name);

void sunxi_drm_delayed_ms(unsigned int ms);

int sunxi_drm_sys_pwm_enable(struct pwm_info_t *pwm_info);

int sunxi_drm_sys_pwm_disable(struct pwm_info_t *pwm_info);

int sunxi_drm_sys_pwm_config(struct pwm_device *pwm_dev, int duty_ns, int period_ns);

bool sunxi_irq_query(struct sunxi_hardware_res *hw_res, void *irq_data, int need_irq);

#endif

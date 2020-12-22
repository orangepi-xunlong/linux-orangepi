/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Pin.h
*
* Author 		: javen
*
* Description 	: C库函数
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*       holi     	   2010-12-02          1.1         添加具体的接口，
*************************************************************************************
*/
#include "OSAL_Pin.h"
#include <linux/errno.h>

#ifndef  __OSAL_PIN_MASK__
__hdle OSAL_GPIO_Request(disp_gpio_set_t *gpio_list, u32 group_count_max)
{
	__hdle ret = 0;
	struct gpio_config pin_cfg;
	char   pin_name[32];
	u32 config;

	if(gpio_list == NULL)
	    return 0;

	pin_cfg.gpio = gpio_list->gpio;
	pin_cfg.mul_sel = gpio_list->mul_sel;
	pin_cfg.pull = gpio_list->pull;
	pin_cfg.drv_level = gpio_list->drv_level;
	pin_cfg.data = gpio_list->data;
	ret = gpio_request(pin_cfg.gpio, NULL);
	if(0 != ret) {
		__wrn("OSAL_GPIO_Request failed, gpio_name=%s, gpio=%d, ret=%d\n", gpio_list->gpio_name, gpio_list->gpio, ret);
		return ret;
	} else {
		__inf("OSAL_GPIO_Request, gpio_name=%s, gpio=%d, ret=%d\n", gpio_list->gpio_name, gpio_list->gpio, ret);
	}
	ret = pin_cfg.gpio;
#if 1
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
	pr_warn("invalid pin [%d] from sys-config\n", pin_cfg.gpio);
	}

#endif

	return ret;
}

__hdle OSAL_GPIO_Request_Ex(char *main_name, const char *sub_name)
{
	__wrn("OSAL_GPIO_Request_Ex is NULL\n");
	return 0;
}

//if_release_to_default_status:
    //如果是0或者1，表示释放后的GPIO处于输入状态，输入状状态不会导致外部电平的错误。
    //如果是2，表示释放后的GPIO状态不变，即释放的时候不管理当前GPIO的硬件寄存器。
s32 OSAL_GPIO_Release(__hdle p_handler, s32 if_release_to_default_status)
{
	if(p_handler) {
		gpio_free(p_handler);
	} else {
		__wrn("OSAL_GPIO_Release, hdl is NULL\n");
	}
	return 0;
}

s32 OSAL_GPIO_DevGetAllPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status, unsigned gpio_count_max, unsigned if_get_from_hardware)
{
	__wrn("OSAL_GPIO_DevGetAllPins_Status is NULL\n");
	return 0;
}

s32 OSAL_GPIO_DevGetONEPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status,const char *gpio_name,unsigned if_get_from_hardware)
	{
	__wrn("OSAL_GPIO_DevGetONEPins_Status is NULL\n");
	return 0;
}

s32 OSAL_GPIO_DevSetONEPin_Status(u32 p_handler, disp_gpio_set_t *gpio_status, const char *gpio_name, u32 if_set_to_current_input_status)
{
	__wrn("OSAL_GPIO_DevSetONEPin_Status is NULL\n");
	return 0;
}

s32 OSAL_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, u32 if_set_to_output_status, const char *gpio_name)
{
	int ret = -1;

	if(p_handler) {
		if(if_set_to_output_status) {
			ret = gpio_direction_output(p_handler, 0);
			if(ret != 0) {
				__wrn("gpio_direction_output fail!\n");
			}
		} else {
			ret = gpio_direction_input(p_handler);
			if(ret != 0) {
				__wrn("gpio_direction_input fail!\n");
			}
		}
	} else {
		__wrn("OSAL_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
		ret = -1;
	}
	return ret;
}

s32 OSAL_GPIO_DevSetONEPIN_PULL_STATUS(u32 p_handler, u32 set_pull_status, const char *gpio_name)
{
	__wrn("OSAL_GPIO_DevSetONEPIN_PULL_STATUS is NULL\n");
	return 0;
}

s32 OSAL_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name)
{
	if(p_handler) {
		return __gpio_get_value(p_handler);
	}
	__wrn("OSAL_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");
	return -1;
}

s32 OSAL_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, u32 value_to_gpio, const char *gpio_name)
{
	if(p_handler) {
		__gpio_set_value(p_handler, value_to_gpio);
	} else {
		__wrn("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");
	}

	return 0;
}

#else

__hdle OSAL_GPIO_Request(disp_gpio_set_t *gpio_list, u32 group_count_max)
{
	return 0;
}

__hdle OSAL_GPIO_Request_Ex(char *main_name, const char *sub_name)
{
	return 0;
}

//if_release_to_default_status:
    //如果是0或者1，表示释放后的GPIO处于输入状态，输入状状态不会导致外部电平的错误。
    //如果是2，表示释放后的GPIO状态不变，即释放的时候不管理当前GPIO的硬件寄存器。
s32 OSAL_GPIO_Release(__hdle p_handler, s32 if_release_to_default_status)
{
	return 0;
}

s32 OSAL_GPIO_DevGetAllPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status, unsigned gpio_count_max, unsigned if_get_from_hardware)
{
	return 0;
}

s32 OSAL_GPIO_DevGetONEPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status,const char *gpio_name,unsigned if_get_from_hardware)
{
	return 0;
}

s32 OSAL_GPIO_DevSetONEPin_Status(u32 p_handler, disp_gpio_set_t *gpio_status, const char *gpio_name, u32 if_set_to_current_input_status)
{
	return 0;
}

s32 OSAL_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, u32 if_set_to_output_status, const char *gpio_name)
{
	return 0;
}

s32 OSAL_GPIO_DevSetONEPIN_PULL_STATUS(u32 p_handler, u32 set_pull_status, const char *gpio_name)
{
	return 0;
}

s32 OSAL_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name)
{
	return 0;
}

s32 OSAL_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, u32 value_to_gpio, const char *gpio_name)
{
	return 0;
}
#endif

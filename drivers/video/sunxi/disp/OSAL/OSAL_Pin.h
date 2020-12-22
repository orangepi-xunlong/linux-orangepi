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
#ifndef  __OSAL_PIN_H__
#define  __OSAL_PIN_H__
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include "OSAL.h"


//#define  __OSAL_PIN_MASK__

__hdle OSAL_GPIO_Request(disp_gpio_set_t *gpio_list, u32 group_count_max);

__hdle OSAL_GPIO_Request_Ex(char *main_name, const char *sub_name);

s32 OSAL_GPIO_Release(__hdle p_handler, s32 if_release_to_default_status);

s32 OSAL_GPIO_DevGetAllPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status, unsigned gpio_count_max, unsigned if_get_from_hardware);

s32 OSAL_GPIO_DevGetONEPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status,const char *gpio_name,unsigned if_get_from_hardware);

s32 OSAL_GPIO_DevSetONEPin_Status(u32 p_handler, disp_gpio_set_t *gpio_status, const char *gpio_name, u32 if_set_to_current_input_status);

s32 OSAL_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, u32 if_set_to_output_status, const char *gpio_name);

s32 OSAL_GPIO_DevSetONEPIN_PULL_STATUS(u32 p_handler, u32 set_pull_status, const char *gpio_name);

s32 OSAL_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name);

s32 OSAL_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, u32 value_to_gpio, const char *gpio_name);

#endif   //__OSAL_PIN_H__


/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL.h
*
* Author 		: javen
*
* Description 	: ²Ù×÷ÏµÍ³ÊÊÅä²ã
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/
#ifndef  __OSAL_H__
#define  __OSAL_H__


typedef struct
{
    char  gpio_name[32];
    int port;
    int port_num;
    int mul_sel;
    int pull;
    int drv_level;
    int data;
    int gpio;
} disp_gpio_set_t;


#include "../de/bsp_display.h"

#include  "OSAL_Cache.h"
#include  "OSAL_Clock.h"
#include  "OSAL_Dma.h"
#include  "OSAL_Pin.h"
#include  "OSAL_Semi.h"
#include  "OSAL_Thread.h"
#include  "OSAL_Time.h"
#include  "OSAL_Lib_C.h"
#include  "OSAL_Int.h"
#include  "OSAL_IrqLock.h"
#include  "OSAL_Pin.h"
#include  "OSAL_Parser.h"
#include  "OSAL_Pwm.h"
#include  "OSAL_Power.h"

//#define __OSAL_DEBUG__

#endif   //__OSAL_H__



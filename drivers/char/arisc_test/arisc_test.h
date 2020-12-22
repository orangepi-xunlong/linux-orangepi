/*
 * drivers/char/arisc_test/arisc_test.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * sunny <sunny@allwinnertech.com>
 *
 * sun6i arisc test head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUN6I_ARISC_TEST_H
#define __SUN6I_ARISC_TEST_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/arisc/arisc.h>
#include <mach/hardware.h>

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1)
#define CNT64_CTRL_REG  IO_ADDRESS(0x01f01e80)
#define CNT64_LOW_REG   IO_ADDRESS(0x01f01e84)
#define CNT64_HIGH_REG  IO_ADDRESS(0x01f01e88)
#elif defined CONFIG_ARCH_SUN9IW1P1
#define CNT64_CTRL_REG  IO_ADDRESS(0x08001c00)
#define CNT64_LOW_REG   IO_ADDRESS(0x08001c04)
#define CNT64_HIGH_REG  IO_ADDRESS(0x08001c08)
#endif

typedef struct arisc_audio_period
{
	unsigned int               period_addr;
	struct arisc_audio_period *next;
}arisc_audio_period_t;

typedef struct arisc_audio_period_queue
{
	struct arisc_audio_period *head;		//the head period of this queue.
	struct arisc_audio_period *tail;		//the tail period of this queue.
	u32				    number;		        //the total period number of this queue.
} arisc_audio_period_queue_t;

#endif /* __SUN6I_ARISC_TEST_H */


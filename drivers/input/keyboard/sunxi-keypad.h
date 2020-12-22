/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com,qys<qinyongshen@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef SUNXI_KEYPAD_HH
#define SUNXI_KEYPAD_HH

#define KEY_MAX_ROW	(8)
#define KEY_MAX_COLUMN	(8)
#if defined (CONFIG_FPGA_V4_PLATFORM) /* S4 820 */
#define KEYPAD_IRQNUM	(46)
#else
#define KEYPAD_IRQNUM	(SUNXI_IRQ_RKEYPAD)
#endif

#ifdef CONFIG_ARCH_SUN8IW9P1
#define KEYPAD_BASSADDRESS	(0xf1f04000)
#else
#define KEYPAD_BASSADDRESS	(0xf1f04000)
#endif

#define KP_CTL			(KEYPAD_BASSADDRESS + 0x00)
#define KP_TIMING		(KEYPAD_BASSADDRESS + 0x04)
#define KP_INT_CFG		(KEYPAD_BASSADDRESS + 0x08)
#define KP_INT_STA		(KEYPAD_BASSADDRESS + 0x0c)
#define KP_IN0			(KEYPAD_BASSADDRESS + 0x10)
#define KP_IN1			(KEYPAD_BASSADDRESS + 0x14)

/* sow and column bit mask */
#define PASS_ALL		(0x0)

/* cycle  */
#define KEYPAD_SCAN_CYCL	(0x100)
#define KEYPAD_DEBOUN_CYCL	(0x200)

/* enable status */
#define KEYPAD_ENABLE		(1)
#define KEYPAD_DISABLE		(0)

/* int ctrl */
#define INT_KEYPRESS_EN		(0x1<<0)
#define INT_KEYRELEASE_EN	(0x1<<1)

/* int status */
#define INT_KEY_PRESS		(0x1<<0)
#define INT_KEY_RELEASE		(0x1<<1)

#define KEYPAD_MAX_CNT		(64)

static const unsigned int keypad_keycodes[]=
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};


#endif //SUNXI_KEYPAD_HH


/* Lite-On LTR-558ALS Linux Driver
 *
 * Copyright (C) 2011 Lite-On Technology Corp (Singapore)
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _LTR558_H
#define _LTR558_H

#define LTR558_NAME             "ltr"

/* LTR-558 Registers */
#define LTR558_ALS_CONTR	0x80
#define LTR558_PS_CONTR		0x81
#define LTR558_PS_LED		0x82
#define LTR558_PS_N_PULSES	0x83
#define LTR558_PS_MEAS_RATE	0x84
#define LTR558_ALS_MEAS_RATE	0x85
#define LTR558_REG_PORT_ID      0x86
#define LTR558_MANUFACTURER_ID	0x87
#define LTR558_MID              0x05
#define LTR558_PID              0x80
#define LTR558_SLAVE_ADDR       0x23

#define LTR558_INTERRUPT	0x8F
#define LTR558_PS_THRES_UP_0	0x90
#define LTR558_PS_THRES_UP_1	0x91
#define LTR558_PS_THRES_LOW_0	0x92
#define LTR558_PS_THRES_LOW_1	0x93

#define LTR558_ALS_THRES_UP_0	0x97
#define LTR558_ALS_THRES_UP_1	0x98
#define LTR558_ALS_THRES_LOW_0	0x99
#define LTR558_ALS_THRES_LOW_1	0x9A

#define LTR558_INTERRUPT_PERSIST 0x9E

/* 558's Read Only Registers */
#define LTR558_ALS_DATA_CH1_0	0x88
#define LTR558_ALS_DATA_CH1_1	0x89
#define LTR558_ALS_DATA_CH0_0	0x8A
#define LTR558_ALS_DATA_CH0_1	0x8B
#define LTR558_ALS_PS_STATUS	0x8C
#define LTR558_PS_DATA_0	0x8D
#define LTR558_PS_DATA_1	0x8E


/* Basic Operating Modes */
#define MODE_ALS_ON_Range1	0x0B
#define MODE_ALS_ON_Range2	0x03
#define MODE_ALS_StdBy		0x00

#define MODE_PS_ON_Gain1	0x03
#define MODE_PS_ON_Gain4	0x07
#define MODE_PS_ON_Gain8	0x0B
#define MODE_PS_ON_Gain16	0x0F
#define MODE_PS_StdBy		0x00

#define PS_RANGE1 	1
#define PS_RANGE4	4
#define PS_RANGE8 	8
#define PS_RANGE16	16

#define ALS_RANGE1_320	1
#define ALS_RANGE2_64K 	2

/* 
 * Magic Number
 * ============
 * Refer to file ioctl-number.txt for allocation
 */
#define LTR558_IOCTL_MAGIC      'c'

/* IOCTLs for ltr558 device */
#define LTR558_IOCTL_PS_ENABLE		_IOW(LTR558_IOCTL_MAGIC, 0, char *)
#define LTR558_IOCTL_ALS_ENABLE		_IOW(LTR558_IOCTL_MAGIC, 1, char *)
#define LTR558_IOCTL_READ_PS_DATA	_IOR(LTR558_IOCTL_MAGIC, 2, char *)
#define LTR558_IOCTL_READ_PS_INT	_IOR(LTR558_IOCTL_MAGIC, 3, char *)
#define LTR558_IOCTL_READ_ALS_DATA	_IOR(LTR558_IOCTL_MAGIC, 4, char *)
#define LTR558_IOCTL_READ_ALS_INT	_IOR(LTR558_IOCTL_MAGIC, 5, char *)


/* Power On response time in ms */
#define PON_DELAY	600
#define WAKEUP_DELAY	10

/* Interrupt vector number to use when probing IRQ number.
 * User changeable depending on sys interrupt.
 * For IRQ numbers used, see /proc/interrupts.
 */
#define GPIO_INT_NO	32

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_REPORT_ALS_DATA = 1U << 1,
	DEBUG_REPORT_PS_DATA = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_CONTROL_INFO = 1U << 4,
	DEBUG_INT = 1U << 5,
};

#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk("*ltr_501:*" fmt , ## arg)

int ltr558_devinit(void);
void ltr558_set_client(struct i2c_client *client);
int ltr558_als_power(bool enable);
int ltr558_ps_power(bool enable);
int ltr558_i2c_read_reg(u8 regnum);
int ltr558_als_read(void);
int ltr558_ps_read(void);

#endif


/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN65IW1_R_CCU_H_
#define _DT_BINDINGS_CLK_SUN65IW1_R_CCU_H_

#define CLK_R_TIMER0		0
#define CLK_R_TIMER1		1
#define CLK_R_TIMER2		2
#define CLK_R_TIMER3		3
#define CLK_R_TIMER		4
#define CLK_R_TWD		5
#define CLK_R_PWM		6
#define CLK_BUS_R_PWM		7
#define CLK_R_SPI		8
#define CLK_BUS_R_SPI		9
#define CLK_R_UART1		10
#define CLK_R_UART0		11
#define CLK_R_TWI2		12
#define CLK_R_TWI1		13
#define CLK_R_TWI0		14
#define CLK_R_PPU		15
#define CLK_R_TZMA		16
#define CLK_R_IRRX		17
#define CLK_BUS_R_IRRX		18
#define CLK_RTC			19
#define CLK_E902_24M		20
#define CLK_E902_CFG		21
#define CLK_E902_CORE		22
#define CLK_R_CPUCFG		23

#define CLK_R_NUMBER		(CLK_R_CPUCFG + 1)

#endif /* _DT_BINDINGS_CLK_SUN65IW1_R_CCU_H_ */

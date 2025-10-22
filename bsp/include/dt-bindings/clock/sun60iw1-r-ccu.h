/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN60IW1_R_CCU_H_
#define _DT_BINDINGS_CLK_SUN60IW1_R_CCU_H_

#define CLK_R_AHB		0
#define CLK_R_APBS0		1
#define CLK_R_APBS1		2
#define CLK_R_TIMER0		3
#define CLK_R_TIMER1		4
#define CLK_R_TIMER2		5
#define CLK_R_TIMER3		6
#define CLK_R_BUS_TIMER		7
#define CLK_R_BUS_TWD		8
#define CLK_R_PWM		9
#define CLK_R_BUS_PWM		10
#define CLK_R_SPI		11
#define CLK_R_BUS_SPI		12
#define CLK_R_BUS_MBOX		13
#define CLK_R_BUS_UART2		14
#define CLK_R_BUS_UART1		15
#define CLK_R_BUS_UART0		16
#define CLK_R_BUS_TWI2		17
#define CLK_R_BUS_TWI1		18
#define CLK_R_BUS_TWI0		19
#define CLK_R_BUS_PPU		20
#define CLK_R_BUS_TZMA		21
#define CLK_R_CPUS_BUS_BIST	22
#define CLK_R_IRRX		23
#define CLK_R_BUS_IRRX		24
#define CLK_DMA_CLKEN_SW	25
#define CLK_R_BUS_RTC		26
#define CLK_R_BUS_CPUCFG	27

#define CLK_R_NUMBER		(CLK_R_BUS_CPUCFG + 1)

#endif /* _DT_BINDINGS_CLK_SUN60IW1_R_CCU_H_ */

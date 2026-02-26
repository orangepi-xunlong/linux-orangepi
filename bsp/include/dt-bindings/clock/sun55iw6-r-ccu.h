/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 panzhijian@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN55IW6_R_CCU_H_
#define _DT_BINDINGS_CLK_SUN55IW6_R_CCU_H_

#define CLK_R_TIMER0		0
#define CLK_R_TIMER1		1
#define CLK_R_TIMER2		2
#define CLK_R_TIMER3		3
#define CLK_R_TIMER		4
#define CLK_R_TWD		5
#define CLK_R_PWM		6
#define CLK_R_BUS_PWM		7
#define CLK_R_SPI		8
#define CLK_R_BUS_SPI		9
#define CLK_R_MBOX		10
#define CLK_R_UART1		11
#define CLK_R_UART0		12
#define CLK_R_TWI1		13
#define CLK_R_TWI0		14
#define CLK_R_PPU		15
#define CLK_R_TZMA		16
#define CLK_R_CPUS_BIST		17
#define CLK_R_IRRX		18
#define CLK_R_BUS_IRRX		19
#define CLK_RTC			20
#define CLK_R_CPUCFG		21
#define CLK_VDD_USB2CPUS	22
#define CLK_VDD_SYS2CPUS	23
#define CLK_VDD_DDR		24
#define CLK_TT_AUTO		25
#define CLK_CPU_ICACHE_AUTO	26
#define CLK_AHBS_AUTO_CLK	27

#define CLK_R_NUMBER		(CLK_AHBS_AUTO_CLK + 1)

#endif /* _DT_BINDINGS_CLK_SUN55IW6_R_CCU_H_ */

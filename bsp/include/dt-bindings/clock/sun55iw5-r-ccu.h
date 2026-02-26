/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 zhaozeyan@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN55IW5_R_CCU_H_
#define _DT_BINDINGS_CLK_SUN55IW5_R_CCU_H_

#define CLK_R_TIMER0		0
#define CLK_R_TIMER1		1
#define CLK_R_TIMER2		2
#define CLK_R_TIMER		3
#define CLK_R_EDID		4
#define CLK_WDT1		5
#define CLK_R_PWM		6
#define CLK_BUS_R_PWM		7
#define CLK_R_UART0		8
#define CLK_R_TWI1		9
#define CLK_R_TWI0		10
#define CLK_R_PPU		11
#define CLK_R_TZMA		12
#define CLK_R_CPUS_BIST		13
#define CLK_R_IRRX		14
#define CLK_BUS_R_IRRX		15
#define CLK_R_GPADC		16
#define CLK_BUS_R_GPADC		17
#define CLK_R_THS		18
#define CLK_R_RTC		19
#define CLK_R_CPUCFG		20
#define CLK_VDD_USB2CPUS	21
#define CLK_VDD_SYS2USB		22
#define CLK_VDD_SYS2CPUS	23
#define CLK_TT_AUTO		24
#define CLK_CPU_ICACHE_AUTO	25
#define CLK_AHBS_AUTO_CLK	26

#define CLK_R_NUMBER		(CLK_AHBS_AUTO_CLK + 1)

#endif /* _DT_BINDINGS_CLK_SUN55IW5_R_CCU_H_ */

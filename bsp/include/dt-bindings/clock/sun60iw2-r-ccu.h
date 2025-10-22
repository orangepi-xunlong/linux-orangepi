/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
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
#define CLK_R_TIMER		7
#define CLK_R_TWD		8
#define CLK_R_BUS_PWM		9
#define CLK_R_PWM		10
#define CLK_R_SPI		11
#define CLK_R_BUS_SPI		12
#define CLK_R_MBOX		13
#define CLK_R_UART1		14
#define CLK_R_UART0		15
#define CLK_R_TWI2		16
#define CLK_R_TWI1		17
#define CLK_R_TWI0		18
#define CLK_R_PPU		19
#define CLK_R_TZMA		20
#define CLK_R_CPUS_BIST		21
#define CLK_R_IRRX		22
#define CLK_R_BUS_IRRX		23
#define CLK_RTC			24
#define CLK_RISCV_24M	25
#define CLK_RISCV_CFG	26
#define	CLK_RISCV		27
#define CLK_R_CPUCFG		28
#define CLK_VDD_USB2CPUS	29
#define CLK_VDD_SYS2USB		30
#define CLK_VDD_SYS2CPUS	31
#define CLK_VDD_DDR		32
#define CLK_TT_AUTO		33
#define CLK_CPU_ICACHE_AUTO	34
#define CLK_AHBS_AUTO_CLK	35

#define CLK_R_NUMBER		(CLK_AHBS_AUTO_CLK + 1)

#endif /* _DT_BINDINGS_CLK_SUN60IW2_R_CCU_H_ */

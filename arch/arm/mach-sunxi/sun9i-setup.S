/*
 * linux/arch/arm/mach-sunxi/sun9i-setup.S
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sun9i cpu core power-up setup operations.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/linkage.h>
#include <asm/mcpm.h>

#define SLAVE_SNOOPCTL_OFFSET	0
#define SNOOPCTL_SNOOP_ENABLE	(1 << 0)
#define SNOOPCTL_DVM_ENABLE	(1 << 1)

#define CCI_STATUS_OFFSET	0xc
#define STATUS_CHANGE_PENDING	(1 << 0)

#define CCI_SLAVE_OFFSET(n)	(0x1000 + 0x1000 * (n))

#define SUN9I_CCI_PHYS_BASE	0x01c90000
#define SUN9I_CCI_SLAVE_A7	3
#define SUN9I_CCI_SLAVE_A15	4
#define SUN9I_CCI_A15_OFFSET	CCI_SLAVE_OFFSET(SUN9I_CCI_SLAVE_A15)
#define SUN9I_CCI_A7_OFFSET	CCI_SLAVE_OFFSET(SUN9I_CCI_SLAVE_A7)

#define SUN9I_CCU_PHYS_BASE     (0x06000000)
#define SUN9I_CCU_C0_CFG_OFFSET (0x54)
#define SUN9I_CCU_C1_CFG_OFFSET (0x58)

ENTRY(sun9i_power_up_setup)
	mov     r2,  r0                 @ backup r0 register first
	mrc	p15, 0, r0, c0, c0, 5	@ MPIDR
	ubfx	r0, r0, #8, #4		@ cluster
	cmp	r2, #0			@ if the cluster first-man
	beq	2f

	@Config A15/A7 axi div to 3 and 4, atb div to 2 and 4.
	@If you don't kown why, please don't change the code.
	@by sunny at 2014-3-19.
	ldr	r3, =SUN9I_CCU_PHYS_BASE + SUN9I_CCU_C0_CFG_OFFSET
	cmp	r0, #0		    @ A7 cluster?
	addne	r3, r3, #SUN9I_CCU_C1_CFG_OFFSET - SUN9I_CCU_C0_CFG_OFFSET
	ldr	r1, [r3]
	bic     r1, r1, #(0x3<<8)   @ a15 atb div
	orr     r1, r1, #(0x1<<8)   @ div = 2
	bic     r1, r1, #(0x7<<0)   @ a15 atb div
	orr     r1, r1, #(0x2<<0)   @ div = value + 1
	str	r1, [r3]	    @ set atb div to 2, axi div to 3
	dsb                         @ Synchronise side-effects of axi config
	ldr	r1, [r3]
	bic     r1, r1, #(0x3<<8)   @ a15 atb div
	orr     r1, r1, #(0x2<<8)   @ div = 4
	bic     r1, r1, #(0x7<<0)   @ a15 atb div
	orr     r1, r1, #(0x3<<0)   @ div = value + 1
	str	r1, [r3]	    @ set atb div to 4, axi div to 4
	dsb                         @ Synchronise side-effects of axi config
	
	@ A15/A7 may not require explicit L2 invalidation on reset, dependent
	@ on hardware integration desicions.
	@ For now, this code assumes that L2 is already invalidated by hardware.
	ldr	r3, =SUN9I_CCI_PHYS_BASE + SUN9I_CCI_A7_OFFSET
	cmp	r0, #0		@ A7 cluster?
	addne	r3, r3, #SUN9I_CCI_A15_OFFSET - SUN9I_CCI_A7_OFFSET

	@ r3 now points to the correct CCI slave register block
	ldr	r1, [r3, #SLAVE_SNOOPCTL_OFFSET]
	orr	r1, r1, #SNOOPCTL_SNOOP_ENABLE
	orr	r1, r1, #SNOOPCTL_DVM_ENABLE
	str	r1, [r3, #SLAVE_SNOOPCTL_OFFSET]	@ enable CCI snoops

	@ Wait for snoop control change to complete:
	ldr	r3, =SUN9I_CCI_PHYS_BASE
1:
        ldr	r1, [r3, #CCI_STATUS_OFFSET]
	tst	r1, #STATUS_CHANGE_PENDING
	bne	1b
	dsb                             @ Synchronise side-effects of enabling CCI

2:
	cmp	r0, #1                  @ A15 cluster ?
	bne     3f

	@sun9i platform-specific Cortex-A15 setup.
	mrc p15, 1, r1, c15, c0, 4      @ ACTLR2
	orr r1, r1, #(0x1<<31)          @ Enable CPU regional clock gates
	mcr p15, 1, r1, c15, c0, 4
	
	mrc p15, 1, r1, c15, c0, 0      @ L2ACTLR
	orr r1, r1, #(0x1<<26)          @ Enables L2, GIC, and Timer regional clock gates
	mcr p15, 1, r1, c15, c0, 0
	
	mrc p15, 1, r1, c15, c0, 0      @ L2ACTLR
	orr r1, r1, #(0x1<<3)           @ Disables clean/evict from being pushed to external
	mcr p15, 1, r1, c15, c0, 0

	mrc p15, 1, r1, c9, c0, 2
	bic r1, r1, #(0x7<<0)           @ L2 data ram latency
	orr r1, r1, #(0x3<<0)
	mcr p15, 1, r1, c9, c0, 2

3:
        @sun9i platform-specific operations porcess done.
	bx	lr

ENDPROC(sun9i_power_up_setup)

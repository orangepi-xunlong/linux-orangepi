/***********************************************************************************************************************
;                             Copyright(C), 2012-2015, Newbie Microelectronic Co., Ltd.
;                                                  All Rights Reserved
;
; File Name : resume1.S
;
; Author : yanggq
;
; Version : 1.1.0
;
; Date : 2012.7.11
;
; Description :
;
; Functions list : none.
;
; Others : None at present.
;
;
; History :
;
;  <Author>        <time>       <version>      <description>
;
; gq.Yang       2012.7.11      1.1.0        build the file
;
***********************************************************************************************************************/
#include "./../../../pm_assembler_i.h"

#define  ARMV7_USR_MODE        0x10
#define  ARMV7_FIQ_MODE        0x11
#define  ARMV7_IRQ_MODE        0x12
#define  ARMV7_SVC_MODE        0x13
#define  ARMV7_MON_MODE        0x16
#define  ARMV7_ABT_MODE        0x17
#define  ARMV7_UND_MODE        0x1b
#define  ARMV7_SYSTEM_MODE     0x1f
#define  ARMV7_MODE_MASK       0x1f
#define  ARMV7_FIQ_MASK        0x40
#define  ARMV7_IRQ_MASK        0x80


#define   ARMV7_THUMB_MASK    (1<<5)
#define   ARMV7_END_MASK      (1<<9)

#define   ARMV7_IT_MASK       ((0x3f<<10)|(0x03<<25))
#define   ARMV7_GE_MASK       (0x0f<<16)
#define   ARMV7_JAVA_MASK     (1<<24)

#define   ARMV7_QFLAG_BIT     (1 << 27)
#define   ARMV7_CC_V_BIT      (1 << 28)
#define   ARMV7_CC_C_BIT      (1 << 29)
#define   ARMV7_CC_Z_BIT      (1 << 30)
#define   ARMV7_CC_N_BIT      (1 << 31)
#define   ARMV7_CC_E_BIT      (1 << 9)

#define   ARMV7_C1_M_BIT         ( 1 << 0 )
#define   ARMV7_C1_A_BIT         ( 1 << 1 )
#define   ARMV7_C1_C_BIT         ( 1 << 2 )
#define   ARMV7_C1_Z_BIT         ( 1 << 11 )
#define   ARMV7_C1_I_BIT         ( 1 << 12 )

	.extern resume1_c_part
	.text
	.arm
/*********************************the begin of initializing system********************************/
	.globl _start
_start:
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r0, r0, #ARMV7_SVC_MODE
	orr r0, r0, #( ARMV7_IRQ_MASK | ARMV7_FIQ_MASK )    ;// After reset, ARM automaticly disables IRQ and FIQ, and runs in SVC mode.
	bic r0, r0, #ARMV7_CC_E_BIT                         ;// set little-endian
	msr cpsr_c, r0

	//config smp
	
	// configure memory system : disable MMU,cache and write buffer; set little_endian;
	mrc p15, 0, r0, c1, c0
	bic r0, r0, #( ARMV7_C1_M_BIT | ARMV7_C1_C_BIT )  ;// disable MMU, data cache
	bic r0, r0, #( ARMV7_C1_I_BIT | ARMV7_C1_Z_BIT )  ;// disable instruction cache, disable flow prediction
	bic r0, r0, #( ARMV7_C1_A_BIT)                    ;// disable align
	mcr p15, 0, r0, c1, c0

	dsb
	isb

#if defined(CONFIG_ARCH_SUN9IW1P1)
	mrc p15, 0, r0, c0, c0, 5	;//Read CPU ID register
	ubfx	r0, r0, #8, #4		@ cluster
	cmp	r0, #1                  @ A15 cluster ?
	bne     A7f

	//config A15
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

	dsb
	isb
A7f:
#endif

#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1)
	ldr r0, =0x01c00024
	ldr r1, [r0]
	// 1 -> bit15
	orr r1, #0x8000
	str r1, [r0]
	// read version
	ldr r1, [r0]
	lsr r2, r1, #16
	ldr r3, =0x1661
	cmp r2, r3
	beq config_61

	//setting to 0x22223a22
config_50:
	ldr r0, =0x01c00044
        ldr r1,[r0]
	orr r1,#0x1800
	str r1,[r0]
	b   config_end

	//setting to 0x222222e2
config_61:
	ldr r0, =0x01c00044
        ldr r1,[r0]
	orr r1,#0xc0
	str r1,[r0]

config_end:
#endif

	// set SP for SVC mode	
	mrc p15, 0, r0, c0, c0, 5
	ubfx	r0, r0, #0, #2		@ cpu id
	//each core have seperate 512k bytes stack.
	add r0, r0, #1
	lsl	r0, r0, #8
	add	sp, r0, #(SP_IN_SRAM_START)
	dsb
	isb

/**********************************the end of initializing system*********************************/
	bl set_pll
	bl  resume1_c_part

 	b .                                 ;// infinite loop

	.end

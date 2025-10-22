#include <linux/linkage.h>
#include <linux/version.h>

#define ENDPIPROC(x)			\
	.globl	__pi_##x;		\
	.type 	__pi_##x, %function;	\
	.set	__pi_##x, x;		\
	.size	__pi_##x, . - x;	\
	ENDPROC(x)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#define SYM_FUNC_START_PI(x)			\
		SYM_FUNC_START(__pi_##x)

#define SYM_FUNC_END_PI(x)			\
		SYM_FUNC_END(__pi_##x);		\
		SYM_FUNC_ALIAS(x, __pi_##x)
#endif
/*
 *	cedar_dma_flush_range(start, size)
 *
 *	clean & invalidate D / U line
 *
 *	- start   - virtual start address of region
 *	- size    - size in question
 */
#if IS_ENABLED(CONFIG_ARM64)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#if defined (CONFIG_ARCH_SUN55IW3) ||  (CONFIG_ARCH_SUN50IW9) || (CONFIG_ARCH_SUN55IW6) || (CONFIG_ARCH_SUN60IW2) || (CONFIG_ARCH_SUN65IW1)
#include <asm/asm-uaccess.h>
#include "flush_cache.h"
SYM_FUNC_START_PI(cedar_dma_flush_range)
    uaccess_ttbr0_enable x2, x3, x4
    dcache_by_line_op_aw civac, sy, x0, x1, x2, x3
    uaccess_ttbr0_disable x1, x2
    ret
SYM_FUNC_END_PI(cedar_dma_flush_range)
#else
SYM_FUNC_START_PI(cedar_dma_flush_range)
    mrs	x4, DAIF
    msr	DAIFSet, #3
    mrs	x2, SP_EL0
    ldr	x2, [x2, #8]
    mrs	x3, TTBR1_EL1
    extr	x3, x3, x2, #48
    ror	x3, x3, #16
    msr	TTBR1_EL1, x3
    isb
    msr	TTBR0_EL1, x2
    isb
    msr	DAIF, x4
    mrs	x3, CTR_EL0
    nop
    ubfx	x3, x3, #16, #4
    mov	x2, #4
    lsl	x2, x2, x3
    add	x1, x0, x1
    sub	x3, x2, #1
    bic	x0, x0, x3
    dc	civac, x0
    add	x0, x0, x2
    cmp	x0, x1
    b.lo	#-12
    dsb	sy
    mrs	x2, DAIF
    msr	DAIFSet, #3
    mrs	x1, TTBR1_EL1
    and	x1, x1, #0xffffffffffff
    sub	x1, x1, #1, lsl #12     // =4096
    msr	TTBR0_EL1, x1
    isb
    add	x1, x1, #1, lsl #12     // =4096
    msr	TTBR1_EL1, x1
    isb
    msr	DAIF, x2
    ret
SYM_FUNC_END_PI(cedar_dma_flush_range)
#endif
#else
#if IS_ENABLED(CONFIG_ARM64_SW_TTBR0_PAN)
SYM_FUNC_START(cedar_dma_flush_range)
    mrs	x4, DAIF
    msr	DAIFSet, #2
    mrs	x2, SP_EL0
    ldr	x2, [x2, #16]
    mrs	x3, TTBR1_EL1
    extr	x3, x3, x2, #48
    ror	x3, x3, #16
    msr	TTBR1_EL1, x3
    isb
    msr	TTBR0_EL1, x2
    isb
    msr	DAIF, x4
    mrs	x3, CTR_EL0
    nop
    ubfx	x3, x3, #16, #4
    mov	x2, #4
    lsl	x2, x2, x3
    add	x1, x0, x1
    sub	x3, x2, #1
    bic	x0, x0, x3
    dc	civac, x0
    add	x0, x0, x2
    cmp	x0, x1
    b.lo	#-12
    dsb	sy
    mrs	x2, DAIF
    msr	DAIFSet, #2
    mrs	x1, TTBR1_EL1
    and	x1, x1, #0xffffffffffff
    sub	x1, x1, #1, lsl #12
    msr	TTBR0_EL1, x1
    isb
    add	x1, x1, #1, lsl #12
    msr	TTBR1_EL1, x1
    isb
    msr	DAIF, x2
    ret
SYM_FUNC_END(cedar_dma_flush_range)
#else
SYM_FUNC_START(cedar_dma_flush_range)
       mrs     x3, ctr_el0
       nop
       ubfx    x3, x3, #16, #4
       mov     x2, #0x4                        // #4
       lsl     x2, x2, x3
       add     x1, x0, x1
       sub     x3, x2, #0x1
       bic     x0, x0, x3
start:
       dc      civac, x0
       add     x0, x0, x2
       cmp     x0, x1
       b.cc start
       dsb     sy
       ret
       nop
       nop
SYM_FUNC_END(cedar_dma_flush_range)
#endif
#endif
#else
#if !IS_ENABLED(CONFIG_RISCV)
ENTRY(cedar_dma_flush_range)
	mrc     15, 0, r3, cr0, cr0, 1
	lsr     r3, r3, #16
	and     r3, r3, #15
	mov     r2, #4
	lsl     r2, r2, r3
	sub     r3, r2, #1
	bic     r0, r0, r3
1:
	mcr     15, 0, r0, cr7, cr14, 1
	add     r0, r0, r2
	cmp     r0, r1
	bcc     1b
	dsb     st
	bx      lr
ENDPROC(cedar_dma_flush_range)
#endif
#endif

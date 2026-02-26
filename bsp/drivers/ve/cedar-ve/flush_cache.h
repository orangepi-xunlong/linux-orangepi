/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *
 * Copyright (C) 1996-2000 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */

#ifndef __FLUSH_CACHE_H
#define __FLUSH_CACHE_H

#include <asm-generic/export.h>

#include <asm/asm-offsets.h>
#include <asm/alternative.h>
#include <asm/asm-bug.h>
#include <asm/cpufeature.h>
#include <asm/cputype.h>
#include <asm/debug-monitors.h>
#include <asm/page.h>
#include <asm/pgtable-hwdef.h>
#include <asm/ptrace.h>
#include <asm/thread_info.h>
#include <asm/sysreg.h>



/*
 * Based on arch/arm/include/asm/assembler.h, arch/arm/mm/proc-macros.S
 * read_ctr - read CTR_EL0. If the system has mismatched register fields,
 * provide the system wide safe value from arm64_ftr_reg_ctrel0.sys_val
 */
	.macro	read_ctr_aw, reg
//#ifndef __KVM_NVHE_HYPERVISOR__
//alternative_if_not ARM64_MISMATCHED_CACHE_TYPE
	mrs	\reg, ctr_el0			/* read CTR */
	nop
//alternative_else
	//mov x0, (((3) << 19) | ((3) << 16) | ((0) << 12) | ((0) << 8) | ((1) << 5))
//	ldr x0, =SYS_CTR_EL0
	/* mov x0, #1769504 */
	/* ldr x0, =0x1b0020 */
//	bl read_sanitised_ftr_reg
//	ldr_l	\reg, x0
	/* ldr_l	\reg, arm64_ftr_reg_ctrel0 + ARM64_FTR_SYSVAL */
//alternative_endif
//#else
//alternative_if_not ARM64_KVM_PROTECTED_MODE
//	ASM_BUG()
//alternative_else_nop_endif
//alternative_cb kvm_compute_final_ctr_el0
//	movz	\reg, #0
//	movk	\reg, #0, lsl #16
//	movk	\reg, #0, lsl #32
//	movk	\reg, #0, lsl #48
//alternative_cb_end
//#endif
	.endm

/*
 * dcache_line_size - get the safe D-cache line size across all CPUs
 */
	.macro	dcache_line_size_aw, reg, tmp
	read_ctr_aw	\tmp
	ubfm		\tmp, \tmp, #16, #19	/* cache line size encoding */
	mov		\reg, #4		/* bytes per word */
	lsl		\reg, \reg, \tmp	/* actual cache line size */
	.endm

	.macro __dcache_op_workaround_clean_cache_aw, op, addr
alternative_if_not ARM64_WORKAROUND_CLEAN_CACHE
	dc	\op, \addr
alternative_else
	dc	civac, \addr
alternative_endif
	.endm

/*
 * Macro to perform a data cache maintenance for the interval
 * [start, end)
 *
 * 	op:		operation passed to dc instruction
 * 	domain:		domain used in dsb instruciton
 * 	start:          starting virtual address of the region
 * 	end:            end virtual address of the region
 * 	fixup:		optional label to branch to on user fault
 * 	Corrupts:       start, end, tmp1, tmp2
 */
	.macro dcache_by_line_op_aw op, domain, start, end, tmp1, tmp2, fixup
	dcache_line_size_aw \tmp1, \tmp2
	sub	\tmp2, \tmp1, #1
	bic	\start, \start, \tmp2
.Ldcache_op\@ :
	.ifc	\op, cvau
	__dcache_op_workaround_clean_cache_aw \op, \start
	.else
	.ifc	\op, cvac
	__dcache_op_workaround_clean_cache_aw \op, \start
	.else
	.ifc	\op, cvap
	sys	3, c7, c12, 1, \start	/* dc cvap */
	.else
	.ifc	\op, cvadp
	sys	3, c7, c13, 1, \start	/* dc cvadp */
	.else
	dc	\op, \start
	.endif
	.endif
	.endif
	.endif
	add	\start, \start, \tmp1
	cmp	\start, \end
	b.lo	.Ldcache_op\@
	dsb	\domain
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	_cond_uaccess_extable .Ldcache_op\@, \fixup
#else
	_cond_extable .Ldcache_op\@, \fixup
#endif
	.endm
#endif
	.arch armv8-a
	.file	"sunxi_awlink_asm.c"

	.text
	.align	2
	.type	pmd_to_page, %function
pmd_to_page:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	str	x0, [sp, 8]	// pmd, pmd
	mov	x0, -4096	// tmp98,
	str	x0, [sp, 24]	// tmp98, mask
	ldr	x1, [sp, 8]	// pmd.0_1, pmd
	ldr	x0, [sp, 24]	// tmp99, mask
	and	x1, x1, x0	// _2, pmd.0_1, tmp99
	mov	x0, 549755813888	// tmp100,
	add	x0, x1, x0	// _3, _2, tmp100
	lsr	x0, x0, 12	// tmp101, _3,
	str	x0, [sp, 32]	// tmp101, __idx
	ldr	x0, [sp, 32]	// tmp102, __idx
	lsl	x1, x0, 6	// _4, tmp102,
	mov	x0, -8589934592	// tmp104,
	add	x0, x1, x0	// tmp103, _4, tmp104
	str	x0, [sp, 40]	// tmp103, __addr
	ldr	x0, [sp, 40]	// _9, __addr
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	pmd_to_page, .-pmd_to_page
	.align	2
	.type	log_write_mmio, %function
log_write_mmio:
	hint	25 // paciasp
	sub	sp, sp, #32	//,,
	str	x0, [sp, 24]	// val, val
	strb	w1, [sp, 23]	// width, width
	str	x2, [sp, 8]	// addr, addr
	str	x3, [sp]	// caller_addr, caller_addr
	nop
	add	sp, sp, 32	//,,
	hint	29 // autiasp
	ret
	.size	log_write_mmio, .-log_write_mmio
	.align	2
	.type	log_post_write_mmio, %function
log_post_write_mmio:
	hint	25 // paciasp
	sub	sp, sp, #32	//,,
	str	x0, [sp, 24]	// val, val
	strb	w1, [sp, 23]	// width, width
	str	x2, [sp, 8]	// addr, addr
	str	x3, [sp]	// caller_addr, caller_addr
	nop
	add	sp, sp, 32	//,,
	hint	29 // autiasp
	ret
	.size	log_post_write_mmio, .-log_post_write_mmio
	.align	2
	.type	log_read_mmio, %function
log_read_mmio:
	hint	25 // paciasp
	sub	sp, sp, #32	//,,
	strb	w0, [sp, 31]	// width, width
	str	x1, [sp, 16]	// addr, addr
	str	x2, [sp, 8]	// caller_addr, caller_addr
	nop
	add	sp, sp, 32	//,,
	hint	29 // autiasp
	ret
	.size	log_read_mmio, .-log_read_mmio
	.align	2
	.type	log_post_read_mmio, %function
log_post_read_mmio:
	hint	25 // paciasp
	sub	sp, sp, #32	//,,
	str	x0, [sp, 24]	// val, val
	strb	w1, [sp, 23]	// width, width
	str	x2, [sp, 8]	// addr, addr
	str	x3, [sp]	// caller_addr, caller_addr
	nop
	add	sp, sp, 32	//,,
	hint	29 // autiasp
	ret
	.size	log_post_read_mmio, .-log_post_read_mmio
	.align	2
	.type	readl, %function
readl:
	hint	25 // paciasp
	stp	x29, x30, [sp, -64]!	//,,,
	mov	x29, sp	//,
	str	x0, [sp, 24]	// addr, addr
.L8:
	adrp	x0, .L8	// tmp99,
	add	x0, x0, :lo12:.L8	// _3, tmp99,
	mov	x2, x0	//, _3
	ldr	x1, [sp, 24]	//, addr
	mov	w0, 32	//,
	bl	log_read_mmio		//
	ldr	x0, [sp, 24]	// tmp100, addr
	str	x0, [sp, 56]	// tmp100, addr
	ldr	x0, [sp, 56]	// tmp102, addr
#APP
	.if 1 == 1
661:
	ldr w0, [x0]	// val, tmp102
662:
.pushsection .altinstructions,"a"
 .word 661b - .
 .word 663f - .
 .hword 61
 .byte 662b-661b
 .byte 664f-663f
.popsection
.subsection 1
663:
	ldar w0, [x0]	// val, tmp102
664:
	.org	. - (664b-663b) + (662b-661b)
	.org	. - (662b-661b) + (664b-663b)
	.previous
.endif

// 0 "" 2
#NO_APP
	str	w0, [sp, 44]	// val, val
	ldr	w0, [sp, 44]	// D.69914, val
	str	w0, [sp, 40]	// D.69914, val
#APP
	dmb oshld
// 0 "" 2
#NO_APP
	ldr	w0, [sp, 40]	// _1, val
#APP
	eor	x0, x0, x0	// tmp, _1
cbnz	x0, .	// tmp
// 0 "" 2
#NO_APP
	str	x0, [sp, 48]	// tmp, tmp
	ldr	w4, [sp, 40]	// _2, val
.L10:
	adrp	x0, .L10	// tmp104,
	add	x0, x0, :lo12:.L10	// _11, tmp104,
	mov	x3, x0	//, _11
	ldr	x2, [sp, 24]	//, addr
	mov	w1, 32	//,
	mov	x0, x4	//, _2
	bl	log_post_read_mmio		//
	ldr	w0, [sp, 40]	// _13, val
	ldp	x29, x30, [sp], 64	//,,,
	hint	29 // autiasp
	ret
	.size	readl, .-readl
	.align	2
	.type	writel, %function
writel:
	hint	25 // paciasp
	stp	x29, x30, [sp, -48]!	//,,,
	mov	x29, sp	//,
	str	w0, [sp, 28]	// value, value
	str	x1, [sp, 16]	// addr, addr
	ldr	w4, [sp, 28]	// _1, value
.L13:
	adrp	x0, .L13	// tmp96,
	add	x0, x0, :lo12:.L13	// _4, tmp96,
	mov	x3, x0	//, _4
	ldr	x2, [sp, 16]	//, addr
	mov	w1, 32	//,
	mov	x0, x4	//, _1
	bl	log_write_mmio		//
#APP
	dmb oshst
// 0 "" 2
#NO_APP
	ldr	w0, [sp, 28]	// tmp97, value
	str	w0, [sp, 36]	// tmp97, val
	ldr	x0, [sp, 16]	// tmp98, addr
	str	x0, [sp, 40]	// tmp98, addr
	ldr	w0, [sp, 36]	// tmp99, val
	ldr	x1, [sp, 40]	// tmp100, addr
#APP
	str w0, [x1]	// tmp99, tmp100
// 0 "" 2
#NO_APP
	nop
	ldr	w4, [sp, 28]	// _2, value
.L14:
	adrp	x0, .L14	// tmp101,
	add	x0, x0, :lo12:.L14	// _9, tmp101,
	mov	x3, x0	//, _9
	ldr	x2, [sp, 16]	//, addr
	mov	w1, 32	//,
	mov	x0, x4	//, _2
	bl	log_post_write_mmio		//
	nop
	ldp	x29, x30, [sp], 48	//,,,
	hint	29 // autiasp
	ret
	.size	writel, .-writel
	.section	.rodata
	.align	3
.LC0:
	.string	"unknown"
	.align	3
.LC1:
	.string	"firmware"
	.align	3
.LC2:
	.string	"kernel-module"
	.align	3
.LC3:
	.string	"kexec-image"
	.align	3
.LC4:
	.string	"kexec-initramfs"
	.align	3
.LC5:
	.string	"security-policy"
	.align	3
.LC6:
	.string	"x509-certificate"
	.align	3
.LC7:
	.string	""
	.align	3
	.type	kernel_read_file_str, %object
	.size	kernel_read_file_str, 64
kernel_read_file_str:
	.xword	.LC0
	.xword	.LC1
	.xword	.LC2
	.xword	.LC3
	.xword	.LC4
	.xword	.LC5
	.xword	.LC6
	.xword	.LC7
	.align	3
	.type	kernel_load_data_str, %object
	.size	kernel_load_data_str, 64
kernel_load_data_str:
	.xword	.LC0
	.xword	.LC1
	.xword	.LC2
	.xword	.LC3
	.xword	.LC4
	.xword	.LC5
	.xword	.LC6
	.xword	.LC7
	.align	1
	.type	eth_reserved_addr_base, %object
	.size	eth_reserved_addr_base, 6
eth_reserved_addr_base:
	.string	"\001\200\302"
	.string	""
	.string	""
	.text
	.align	2
	.type	junk_fun1, %function
junk_fun1:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 10	// tmp103,
	str	w0, [sp, 8]	// tmp103, mod_reg_val
	mov	w0, 9	// tmp104,
	str	w0, [sp, 12]	// tmp104, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.1_1, mod_reg_val
	cmp	w0, 15	// mod_reg_val.1_1,
	bne	.L16		//,
	ldr	w0, [sp, 12]	// _9, mod_reg_addr
	b	.L17		//
.L16:
	ldr	w0, [sp, 8]	// mod_reg_val.2_2, mod_reg_val
	add	w0, w0, 1	// _3, mod_reg_val.2_2,
	str	w0, [sp, 8]	// _3, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.3_4, mod_reg_addr
	add	w0, w0, 51	// _5, mod_reg_addr.3_4,
	str	w0, [sp, 12]	// _5, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.4_6, mod_reg_addr
	cmp	w0, 16	// mod_reg_addr.4_6,
	bne	.L18		//,
	ldr	w0, [sp, 12]	// _9, mod_reg_addr
	b	.L17		//
.L18:
	ldr	w1, [sp, 12]	// mod_reg_addr.5_16, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.6_7, mod_reg_val
	add	w0, w1, w0	// _8, mod_reg_addr.5_16, mod_reg_val.6_7
	str	w0, [sp, 8]	// _8, mod_reg_val
	ldr	w0, [sp, 8]	// _9, mod_reg_val
.L17:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun1, .-junk_fun1
	.align	2
	.type	junk_fun2, %function
junk_fun2:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 2	// tmp97,
	str	w0, [sp, 8]	// tmp97, mod_reg_val
	mov	w0, 15	// tmp98,
	str	w0, [sp, 12]	// tmp98, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.7_1, mod_reg_val
	cmp	w0, 819	// mod_reg_val.7_1,
	bne	.L20		//,
	ldr	w0, [sp, 12]	// _4, mod_reg_addr
	b	.L21		//
.L20:
	ldr	w1, [sp, 8]	// mod_reg_val.8_2, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.9_3, mod_reg_addr
	cmp	w1, w0	// mod_reg_val.8_2, mod_reg_addr.9_3
	bge	.L22		//,
	ldr	w0, [sp, 12]	// _4, mod_reg_addr
	b	.L21		//
.L22:
	ldr	w0, [sp, 8]	// _4, mod_reg_val
.L21:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun2, .-junk_fun2
	.align	2
	.type	junk_fun3, %function
junk_fun3:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 7	// tmp102,
	str	w0, [sp, 8]	// tmp102, mod_reg_val
	mov	w0, 12	// tmp103,
	str	w0, [sp, 12]	// tmp103, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.10_1, mod_reg_val
	add	w0, w0, 1	// _2, mod_reg_val.10_1,
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.11_3, mod_reg_addr
	add	w0, w0, 51	// _4, mod_reg_addr.11_3,
	str	w0, [sp, 12]	// _4, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.12_5, mod_reg_addr
	cmp	w0, 16	// mod_reg_addr.12_5,
	bne	.L24		//,
	ldr	w0, [sp, 12]	// _8, mod_reg_addr
	b	.L25		//
.L24:
	ldr	w1, [sp, 8]	// mod_reg_val.13_15, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.14_6, mod_reg_addr
	eor	w0, w1, w0	// _7, mod_reg_val.13_15, mod_reg_addr.14_6
	str	w0, [sp, 12]	// _7, mod_reg_addr
	ldr	w0, [sp, 8]	// _8, mod_reg_val
.L25:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun3, .-junk_fun3
	.align	2
	.type	junk_fun4, %function
junk_fun4:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 1	// tmp101,
	str	w0, [sp, 8]	// tmp101, mod_reg_val
	mov	w0, 3	// tmp102,
	str	w0, [sp, 12]	// tmp102, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.15_1, mod_reg_val
	add	w1, w0, 1	// _3, mod_reg_val.15_1,
	str	w1, [sp, 8]	// _3, mod_reg_val
	and	w0, w0, 8	// _4, mod_reg_val.15_1,
	cmp	w0, 0	// _4,
	bne	.L27		//,
	ldr	w1, [sp, 8]	// mod_reg_val.17_14, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.18_5, mod_reg_addr
	mul	w0, w1, w0	// _6, mod_reg_val.17_14, mod_reg_addr.18_5
	str	w0, [sp, 12]	// _6, mod_reg_addr
.L27:
	ldr	w0, [sp, 8]	// mod_reg_val.19_7, mod_reg_val
	cmp	w0, 16	// mod_reg_val.19_7,
	bne	.L28		//,
	ldr	w0, [sp, 12]	// _8, mod_reg_addr
	b	.L29		//
.L28:
	ldr	w0, [sp, 8]	// _8, mod_reg_val
.L29:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun4, .-junk_fun4
	.align	2
	.type	junk_fun5, %function
junk_fun5:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 16	// tmp99,
	str	w0, [sp, 8]	// tmp99, mod_reg_val
	mov	w0, 6	// tmp100,
	str	w0, [sp, 12]	// tmp100, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.20_1, mod_reg_val
	cmp	w0, 16	// mod_reg_val.20_1,
	bne	.L31		//,
	ldr	w0, [sp, 12]	// _5, mod_reg_addr
	b	.L32		//
.L31:
	ldr	w1, [sp, 12]	// mod_reg_addr.21_10, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.22_2, mod_reg_val
	add	w0, w1, w0	// _3, mod_reg_addr.21_10, mod_reg_val.22_2
	str	w0, [sp, 8]	// _3, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.23_4, mod_reg_val
	cmp	w0, 16	// mod_reg_val.23_4,
	bne	.L33		//,
	ldr	w0, [sp, 12]	// _5, mod_reg_addr
	b	.L32		//
.L33:
	ldr	w0, [sp, 8]	// _5, mod_reg_val
.L32:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun5, .-junk_fun5
	.align	2
	.type	junk_fun6, %function
junk_fun6:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 12	// tmp102,
	str	w0, [sp, 8]	// tmp102, mod_reg_val
	mov	w0, 3	// tmp103,
	str	w0, [sp, 12]	// tmp103, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.24_13, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.25_1, mod_reg_addr
	eor	w0, w1, w0	// _2, mod_reg_val.24_13, mod_reg_addr.25_1
	str	w0, [sp, 12]	// _2, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.26_3, mod_reg_val
	cmp	w0, 819	// mod_reg_val.26_3,
	bne	.L35		//,
	ldr	w0, [sp, 12]	// _8, mod_reg_addr
	b	.L36		//
.L35:
	ldr	w1, [sp, 12]	// mod_reg_addr.27_4, mod_reg_addr
	mov	w0, 819	// tmp104,
	mul	w0, w1, w0	// _5, mod_reg_addr.27_4, tmp104
	str	w0, [sp, 8]	// _5, mod_reg_val
	ldr	w1, [sp, 8]	// mod_reg_val.28_6, mod_reg_val
	mov	w0, 243	// tmp105,
	and	w0, w1, w0	// _7, mod_reg_val.28_6, tmp105
	str	w0, [sp, 12]	// _7, mod_reg_addr
	ldr	w0, [sp, 8]	// _8, mod_reg_val
.L36:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun6, .-junk_fun6
	.align	2
	.type	junk_fun7, %function
junk_fun7:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 6	// tmp103,
	str	w0, [sp, 8]	// tmp103, mod_reg_val
	mov	w0, 1	// tmp104,
	str	w0, [sp, 12]	// tmp104, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.29_1, mod_reg_val
	add	w0, w1, 1	// _3, mod_reg_val.29_1,
	str	w0, [sp, 8]	// _3, mod_reg_val
	mov	w0, w1	// tmp105, mod_reg_val.29_1
	lsl	w0, w0, 1	// tmp106, tmp105,
	add	w0, w0, w1	// tmp105, tmp105, mod_reg_val.29_1
	lsl	w0, w0, 2	// tmp107, tmp105,
	cmp	w0, 7	// _4,
	bne	.L38		//,
	mov	w0, 51	// tmp108,
	str	w0, [sp, 12]	// tmp108, mod_reg_addr
.L38:
	ldr	w1, [sp, 8]	// mod_reg_val.31_5, mod_reg_val
	add	w0, w1, 1	// _7, mod_reg_val.31_5,
	str	w0, [sp, 8]	// _7, mod_reg_val
	mov	w0, w1	// tmp109, mod_reg_val.31_5
	lsl	w0, w0, 1	// tmp110, tmp109,
	add	w0, w0, w1	// tmp109, tmp109, mod_reg_val.31_5
	lsl	w0, w0, 2	// tmp111, tmp109,
	cmp	w0, 7	// _8,
	bne	.L39		//,
	mov	w0, 51	// tmp112,
	str	w0, [sp, 12]	// tmp112, mod_reg_addr
.L39:
	ldr	w1, [sp, 8]	// mod_reg_val.33_9, mod_reg_val
	add	w0, w1, 1	// _11, mod_reg_val.33_9,
	str	w0, [sp, 8]	// _11, mod_reg_val
	mov	w0, w1	// tmp113, mod_reg_val.33_9
	lsl	w0, w0, 1	// tmp114, tmp113,
	add	w0, w0, w1	// tmp113, tmp113, mod_reg_val.33_9
	lsl	w0, w0, 2	// tmp115, tmp113,
	cmp	w0, 7	// _12,
	bne	.L40		//,
	mov	w0, 51	// tmp116,
	str	w0, [sp, 12]	// tmp116, mod_reg_addr
.L40:
	ldr	w0, [sp, 8]	// _25, mod_reg_val
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun7, .-junk_fun7
	.align	2
	.type	junk_fun8, %function
junk_fun8:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 9	// tmp98,
	str	w0, [sp, 8]	// tmp98, mod_reg_val
	mov	w0, 11	// tmp99,
	str	w0, [sp, 12]	// tmp99, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.35_8, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.36_1, mod_reg_val
	add	w0, w1, w0	// _2, mod_reg_addr.35_8, mod_reg_val.36_1
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.37_3, mod_reg_val
	cmp	w0, 819	// mod_reg_val.37_3,
	bne	.L43		//,
	ldr	w0, [sp, 12]	// _4, mod_reg_addr
	b	.L44		//
.L43:
	ldr	w0, [sp, 8]	// _4, mod_reg_val
.L44:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun8, .-junk_fun8
	.align	2
	.type	junk_fun9, %function
junk_fun9:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 11	// tmp99,
	str	w0, [sp, 8]	// tmp99, mod_reg_val
	mov	w0, 3	// tmp100,
	str	w0, [sp, 12]	// tmp100, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.38_1, mod_reg_val
	cmp	w0, 16	// mod_reg_val.38_1,
	bne	.L46		//,
	ldr	w0, [sp, 12]	// _7, mod_reg_addr
	b	.L47		//
.L46:
	ldr	w0, [sp, 8]	// mod_reg_val.39_2, mod_reg_val
	cmp	w0, 819	// mod_reg_val.39_2,
	bne	.L48		//,
	ldr	w0, [sp, 12]	// _7, mod_reg_addr
	b	.L47		//
.L48:
	ldr	w1, [sp, 8]	// mod_reg_val.40_3, mod_reg_val
	add	w0, w1, 1	// _5, mod_reg_val.40_3,
	str	w0, [sp, 8]	// _5, mod_reg_val
	mov	w0, w1	// tmp101, mod_reg_val.40_3
	lsl	w0, w0, 1	// tmp102, tmp101,
	add	w0, w0, w1	// tmp101, tmp101, mod_reg_val.40_3
	lsl	w0, w0, 2	// tmp103, tmp101,
	cmp	w0, 7	// _6,
	bne	.L49		//,
	mov	w0, 51	// tmp104,
	str	w0, [sp, 12]	// tmp104, mod_reg_addr
.L49:
	ldr	w0, [sp, 8]	// _7, mod_reg_val
.L47:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun9, .-junk_fun9
	.align	2
	.type	junk_fun10, %function
junk_fun10:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 11	// tmp100,
	str	w0, [sp, 8]	// tmp100, mod_reg_val
	mov	w0, 7	// tmp101,
	str	w0, [sp, 12]	// tmp101, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.42_1, mod_reg_addr
	cmp	w0, 15	// mod_reg_addr.42_1,
	bne	.L51		//,
	ldr	w0, [sp, 8]	// _8, mod_reg_val
	b	.L52		//
.L51:
	ldr	w1, [sp, 12]	// mod_reg_addr.43_2, mod_reg_addr
	mov	w0, 819	// tmp102,
	mul	w0, w1, w0	// _3, mod_reg_addr.43_2, tmp102
	str	w0, [sp, 8]	// _3, mod_reg_val
	ldr	w1, [sp, 8]	// mod_reg_val.44_4, mod_reg_val
	add	w0, w1, 1	// _6, mod_reg_val.44_4,
	str	w0, [sp, 8]	// _6, mod_reg_val
	mov	w0, w1	// tmp103, mod_reg_val.44_4
	lsl	w0, w0, 1	// tmp104, tmp103,
	add	w0, w0, w1	// tmp103, tmp103, mod_reg_val.44_4
	lsl	w0, w0, 2	// tmp105, tmp103,
	cmp	w0, 7	// _7,
	bne	.L53		//,
	mov	w0, 51	// tmp106,
	str	w0, [sp, 12]	// tmp106, mod_reg_addr
.L53:
	ldr	w0, [sp, 8]	// _8, mod_reg_val
.L52:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun10, .-junk_fun10
	.align	2
	.type	junk_fun11, %function
junk_fun11:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 13	// tmp105,
	str	w0, [sp, 8]	// tmp105, mod_reg_val
	mov	w0, 1	// tmp106,
	str	w0, [sp, 12]	// tmp106, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.46_15, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.47_1, mod_reg_val
	add	w0, w1, w0	// _2, mod_reg_addr.46_15, mod_reg_val.47_1
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.48_3, mod_reg_val
	add	w0, w0, 1	// _4, mod_reg_val.48_3,
	str	w0, [sp, 8]	// _4, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.49_5, mod_reg_addr
	add	w0, w0, 51	// _6, mod_reg_addr.49_5,
	str	w0, [sp, 12]	// _6, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.50_7, mod_reg_addr
	cmp	w0, 16	// mod_reg_addr.50_7,
	bne	.L55		//,
	ldr	w0, [sp, 12]	// _10, mod_reg_addr
	b	.L56		//
.L55:
	ldr	w1, [sp, 12]	// mod_reg_addr.51_19, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.52_8, mod_reg_val
	eor	w0, w1, w0	// _9, mod_reg_addr.51_19, mod_reg_val.52_8
	str	w0, [sp, 8]	// _9, mod_reg_val
	ldr	w0, [sp, 8]	// _10, mod_reg_val
.L56:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun11, .-junk_fun11
	.align	2
	.type	junk_fun12, %function
junk_fun12:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 5	// tmp100,
	str	w0, [sp, 8]	// tmp100, mod_reg_val
	mov	w0, 7	// tmp101,
	str	w0, [sp, 12]	// tmp101, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.53_1, mod_reg_val
	cmp	w0, 819	// mod_reg_val.53_1,
	bne	.L58		//,
	ldr	w0, [sp, 12]	// _6, mod_reg_addr
	b	.L59		//
.L58:
	ldr	w1, [sp, 12]	// mod_reg_addr.54_2, mod_reg_addr
	mov	w0, 941	// tmp102,
	and	w0, w1, w0	// _3, mod_reg_addr.54_2, tmp102
	cmp	w0, 768	// _3,
	beq	.L60		//,
	ldr	w1, [sp, 8]	// mod_reg_val.55_12, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.56_4, mod_reg_addr
	add	w0, w1, w0	// _5, mod_reg_val.55_12, mod_reg_addr.56_4
	str	w0, [sp, 12]	// _5, mod_reg_addr
.L60:
	ldr	w0, [sp, 8]	// _6, mod_reg_val
.L59:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun12, .-junk_fun12
	.align	2
	.type	junk_fun13, %function
junk_fun13:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 4	// tmp104,
	str	w0, [sp, 8]	// tmp104, mod_reg_val
	mov	w0, 14	// tmp105,
	str	w0, [sp, 12]	// tmp105, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.57_1, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.58_2, mod_reg_addr
	cmp	w1, w0	// mod_reg_val.57_1, mod_reg_addr.58_2
	bge	.L62		//,
	ldr	w1, [sp, 8]	// mod_reg_val.59_3, mod_reg_val
	mov	w0, w1	// tmp106, mod_reg_val.59_3
	lsl	w0, w0, 1	// tmp107, tmp106,
	add	w1, w0, w1	// _4, tmp106, mod_reg_val.59_3
	ldr	w0, [sp, 12]	// mod_reg_addr.60_5, mod_reg_addr
	add	w0, w1, w0	// _6, _4, mod_reg_addr.60_5
	str	w0, [sp, 12]	// _6, mod_reg_addr
.L62:
	ldr	w0, [sp, 8]	// mod_reg_val.61_7, mod_reg_val
	cmp	w0, 819	// mod_reg_val.61_7,
	bne	.L63		//,
	ldr	w0, [sp, 12]	// _11, mod_reg_addr
	b	.L64		//
.L63:
	ldr	w0, [sp, 8]	// mod_reg_val.62_8, mod_reg_val
	cmp	w0, 819	// mod_reg_val.62_8,
	bne	.L65		//,
	ldr	w0, [sp, 12]	// _11, mod_reg_addr
	b	.L64		//
.L65:
	ldr	w1, [sp, 12]	// mod_reg_addr.63_9, mod_reg_addr
	mov	w0, 819	// tmp108,
	mul	w0, w1, w0	// _10, mod_reg_addr.63_9, tmp108
	str	w0, [sp, 8]	// _10, mod_reg_val
	ldr	w0, [sp, 8]	// _11, mod_reg_val
.L64:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun13, .-junk_fun13
	.align	2
	.type	junk_fun14, %function
junk_fun14:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 7	// tmp106,
	str	w0, [sp, 8]	// tmp106, mod_reg_val
	mov	w0, 1	// tmp107,
	str	w0, [sp, 12]	// tmp107, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.64_1, mod_reg_addr
	mov	w0, 941	// tmp108,
	and	w0, w1, w0	// _2, mod_reg_addr.64_1, tmp108
	cmp	w0, 768	// _2,
	beq	.L67		//,
	ldr	w1, [sp, 8]	// mod_reg_val.65_15, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.66_3, mod_reg_addr
	add	w0, w1, w0	// _4, mod_reg_val.65_15, mod_reg_addr.66_3
	str	w0, [sp, 12]	// _4, mod_reg_addr
.L67:
	ldr	w1, [sp, 12]	// mod_reg_addr.67_17, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.68_5, mod_reg_val
	add	w0, w1, w0	// _6, mod_reg_addr.67_17, mod_reg_val.68_5
	str	w0, [sp, 8]	// _6, mod_reg_val
	ldr	w1, [sp, 12]	// mod_reg_addr.69_19, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.70_7, mod_reg_val
	add	w0, w1, w0	// _8, mod_reg_addr.69_19, mod_reg_val.70_7
	str	w0, [sp, 8]	// _8, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.71_9, mod_reg_val
	cmp	w0, 16	// mod_reg_val.71_9,
	bne	.L68		//,
	ldr	w0, [sp, 12]	// _10, mod_reg_addr
	b	.L69		//
.L68:
	ldr	w0, [sp, 8]	// _10, mod_reg_val
.L69:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun14, .-junk_fun14
	.align	2
	.type	junk_fun15, %function
junk_fun15:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 8	// tmp99,
	str	w0, [sp, 8]	// tmp99, mod_reg_val
	mov	w0, 12	// tmp100,
	str	w0, [sp, 12]	// tmp100, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.72_1, mod_reg_val
	add	w0, w0, 1	// _2, mod_reg_val.72_1,
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.73_3, mod_reg_addr
	add	w0, w0, 51	// _4, mod_reg_addr.73_3,
	str	w0, [sp, 12]	// _4, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.74_5, mod_reg_addr
	cmp	w0, 16	// mod_reg_addr.74_5,
	bne	.L71		//,
	ldr	w0, [sp, 12]	// _6, mod_reg_addr
	b	.L72		//
.L71:
	ldr	w0, [sp, 8]	// _6, mod_reg_val
.L72:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun15, .-junk_fun15
	.align	2
	.type	junk_fun16, %function
junk_fun16:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 6	// tmp100,
	str	w0, [sp, 8]	// tmp100, mod_reg_val
	mov	w0, 7	// tmp101,
	str	w0, [sp, 12]	// tmp101, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.75_1, mod_reg_addr
	cmp	w0, 15	// mod_reg_addr.75_1,
	bne	.L74		//,
	ldr	w0, [sp, 8]	// _7, mod_reg_val
	b	.L75		//
.L74:
	ldr	w1, [sp, 8]	// mod_reg_val.76_2, mod_reg_val
	mov	w0, 243	// tmp102,
	and	w0, w1, w0	// _3, mod_reg_val.76_2, tmp102
	str	w0, [sp, 12]	// _3, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.77_4, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.78_5, mod_reg_addr
	cmp	w1, w0	// mod_reg_val.77_4, mod_reg_addr.78_5
	bge	.L76		//,
	ldr	w0, [sp, 12]	// _7, mod_reg_addr
	b	.L75		//
.L76:
	ldr	w0, [sp, 12]	// mod_reg_addr.79_6, mod_reg_addr
	cmp	w0, 15	// mod_reg_addr.79_6,
	bne	.L77		//,
	ldr	w0, [sp, 8]	// _7, mod_reg_val
	b	.L75		//
.L77:
	ldr	w0, [sp, 8]	// _7, mod_reg_val
.L75:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun16, .-junk_fun16
	.align	2
	.type	junk_fun17, %function
junk_fun17:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 15	// tmp104,
	str	w0, [sp, 8]	// tmp104, mod_reg_val
	mov	w0, 2	// tmp105,
	str	w0, [sp, 12]	// tmp105, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.80_1, mod_reg_val
	cmp	w0, 16	// mod_reg_val.80_1,
	bne	.L79		//,
	ldr	w0, [sp, 12]	// _12, mod_reg_addr
	b	.L80		//
.L79:
	ldr	w1, [sp, 8]	// mod_reg_val.81_2, mod_reg_val
	add	w0, w1, 1	// _4, mod_reg_val.81_2,
	str	w0, [sp, 8]	// _4, mod_reg_val
	mov	w0, w1	// tmp106, mod_reg_val.81_2
	lsl	w0, w0, 1	// tmp107, tmp106,
	add	w0, w0, w1	// tmp106, tmp106, mod_reg_val.81_2
	lsl	w0, w0, 2	// tmp108, tmp106,
	cmp	w0, 7	// _5,
	bne	.L81		//,
	mov	w0, 51	// tmp109,
	str	w0, [sp, 12]	// tmp109, mod_reg_addr
.L81:
	ldr	w0, [sp, 8]	// mod_reg_val.83_6, mod_reg_val
	add	w1, w0, 1	// _8, mod_reg_val.83_6,
	str	w1, [sp, 8]	// _8, mod_reg_val
	and	w0, w0, 8	// _9, mod_reg_val.83_6,
	cmp	w0, 0	// _9,
	bne	.L82		//,
	ldr	w1, [sp, 8]	// mod_reg_val.85_22, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.86_10, mod_reg_addr
	mul	w0, w1, w0	// _11, mod_reg_val.85_22, mod_reg_addr.86_10
	str	w0, [sp, 12]	// _11, mod_reg_addr
.L82:
	ldr	w0, [sp, 8]	// _12, mod_reg_val
.L80:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun17, .-junk_fun17
	.align	2
	.type	junk_fun18, %function
junk_fun18:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 13	// tmp96,
	str	w0, [sp, 8]	// tmp96, mod_reg_val
	mov	w0, 3	// tmp97,
	str	w0, [sp, 12]	// tmp97, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.87_1, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.88_2, mod_reg_addr
	cmp	w1, w0	// mod_reg_val.87_1, mod_reg_addr.88_2
	bge	.L84		//,
	ldr	w0, [sp, 12]	// _3, mod_reg_addr
	b	.L85		//
.L84:
	ldr	w0, [sp, 8]	// _3, mod_reg_val
.L85:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun18, .-junk_fun18
	.align	2
	.type	junk_fun19, %function
junk_fun19:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 10	// tmp102,
	str	w0, [sp, 8]	// tmp102, mod_reg_val
	mov	w0, 7	// tmp103,
	str	w0, [sp, 12]	// tmp103, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.89_11, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.90_1, mod_reg_val
	eor	w0, w1, w0	// _2, mod_reg_addr.89_11, mod_reg_val.90_1
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w1, [sp, 12]	// mod_reg_addr.91_3, mod_reg_addr
	mov	w0, 941	// tmp104,
	and	w0, w1, w0	// _4, mod_reg_addr.91_3, tmp104
	cmp	w0, 768	// _4,
	beq	.L87		//,
	ldr	w1, [sp, 8]	// mod_reg_val.92_13, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.93_5, mod_reg_addr
	add	w0, w1, w0	// _6, mod_reg_val.92_13, mod_reg_addr.93_5
	str	w0, [sp, 12]	// _6, mod_reg_addr
.L87:
	ldr	w0, [sp, 8]	// _15, mod_reg_val
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun19, .-junk_fun19
	.align	2
	.type	junk_fun20, %function
junk_fun20:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 12	// tmp101,
	str	w0, [sp, 8]	// tmp101, mod_reg_val
	mov	w0, 16	// tmp102,
	str	w0, [sp, 12]	// tmp102, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.94_1, mod_reg_addr
	mov	w0, 819	// tmp103,
	mul	w0, w1, w0	// _2, mod_reg_addr.94_1, tmp103
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.95_3, mod_reg_val
	and	w0, w0, 6	// _4, mod_reg_val.95_3,
	str	w0, [sp, 12]	// _4, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.96_12, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.97_5, mod_reg_val
	eor	w0, w1, w0	// _6, mod_reg_addr.96_12, mod_reg_val.97_5
	str	w0, [sp, 8]	// _6, mod_reg_val
	ldr	w0, [sp, 8]	// _14, mod_reg_val
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun20, .-junk_fun20
	.align	2
	.type	junk_fun21, %function
junk_fun21:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 9	// tmp105,
	str	w0, [sp, 8]	// tmp105, mod_reg_val
	mov	w0, 4	// tmp106,
	str	w0, [sp, 12]	// tmp106, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.98_13, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.99_1, mod_reg_addr
	eor	w0, w1, w0	// _2, mod_reg_val.98_13, mod_reg_addr.99_1
	str	w0, [sp, 12]	// _2, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.100_3, mod_reg_addr
	mov	w0, 941	// tmp107,
	and	w0, w1, w0	// _4, mod_reg_addr.100_3, tmp107
	cmp	w0, 768	// _4,
	beq	.L92		//,
	ldr	w1, [sp, 8]	// mod_reg_val.101_15, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.102_5, mod_reg_addr
	add	w0, w1, w0	// _6, mod_reg_val.101_15, mod_reg_addr.102_5
	str	w0, [sp, 12]	// _6, mod_reg_addr
.L92:
	ldr	w1, [sp, 12]	// mod_reg_addr.103_17, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.104_7, mod_reg_val
	add	w0, w1, w0	// _8, mod_reg_addr.103_17, mod_reg_val.104_7
	str	w0, [sp, 8]	// _8, mod_reg_val
	ldr	w0, [sp, 8]	// _19, mod_reg_val
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun21, .-junk_fun21
	.align	2
	.type	junk_fun22, %function
junk_fun22:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 1	// tmp108,
	str	w0, [sp, 8]	// tmp108, mod_reg_val
	mov	w0, 8	// tmp109,
	str	w0, [sp, 12]	// tmp109, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.105_1, mod_reg_val
	add	w1, w0, 1	// _3, mod_reg_val.105_1,
	str	w1, [sp, 8]	// _3, mod_reg_val
	and	w0, w0, 8	// _4, mod_reg_val.105_1,
	cmp	w0, 0	// _4,
	bne	.L95		//,
	ldr	w1, [sp, 8]	// mod_reg_val.107_24, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.108_5, mod_reg_addr
	mul	w0, w1, w0	// _6, mod_reg_val.107_24, mod_reg_addr.108_5
	str	w0, [sp, 12]	// _6, mod_reg_addr
.L95:
	ldr	w1, [sp, 8]	// mod_reg_val.109_7, mod_reg_val
	mov	w0, 243	// tmp110,
	and	w0, w1, w0	// _8, mod_reg_val.109_7, tmp110
	str	w0, [sp, 12]	// _8, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.110_9, mod_reg_val
	cmp	w0, 15	// mod_reg_val.110_9,
	bne	.L96		//,
	ldr	w0, [sp, 12]	// _16, mod_reg_addr
	b	.L97		//
.L96:
	ldr	w1, [sp, 8]	// mod_reg_val.111_10, mod_reg_val
	mov	w0, 243	// tmp111,
	and	w0, w1, w0	// _11, mod_reg_val.111_10, tmp111
	str	w0, [sp, 12]	// _11, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.112_12, mod_reg_val
	add	w0, w1, 1	// _14, mod_reg_val.112_12,
	str	w0, [sp, 8]	// _14, mod_reg_val
	mov	w0, w1	// tmp112, mod_reg_val.112_12
	lsl	w0, w0, 1	// tmp113, tmp112,
	add	w0, w0, w1	// tmp112, tmp112, mod_reg_val.112_12
	lsl	w0, w0, 2	// tmp114, tmp112,
	cmp	w0, 7	// _15,
	bne	.L98		//,
	mov	w0, 51	// tmp115,
	str	w0, [sp, 12]	// tmp115, mod_reg_addr
.L98:
	ldr	w0, [sp, 8]	// _16, mod_reg_val
.L97:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun22, .-junk_fun22
	.align	2
	.type	junk_fun23, %function
junk_fun23:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 7	// tmp101,
	str	w0, [sp, 8]	// tmp101, mod_reg_val
	mov	w0, 15	// tmp102,
	str	w0, [sp, 12]	// tmp102, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.114_1, mod_reg_val
	cmp	w0, 15	// mod_reg_val.114_1,
	bne	.L100		//,
	ldr	w0, [sp, 12]	// _8, mod_reg_addr
	b	.L101		//
.L100:
	ldr	w0, [sp, 8]	// mod_reg_val.115_2, mod_reg_val
	add	w1, w0, 1	// _4, mod_reg_val.115_2,
	str	w1, [sp, 8]	// _4, mod_reg_val
	and	w0, w0, 8	// _5, mod_reg_val.115_2,
	cmp	w0, 0	// _5,
	bne	.L102		//,
	ldr	w1, [sp, 8]	// mod_reg_val.117_15, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.118_6, mod_reg_addr
	mul	w0, w1, w0	// _7, mod_reg_val.117_15, mod_reg_addr.118_6
	str	w0, [sp, 12]	// _7, mod_reg_addr
.L102:
	ldr	w0, [sp, 8]	// _8, mod_reg_val
.L101:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun23, .-junk_fun23
	.align	2
	.type	junk_fun24, %function
junk_fun24:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 16	// tmp95,
	str	w0, [sp, 8]	// tmp95, mod_reg_val
	mov	w0, 9	// tmp96,
	str	w0, [sp, 12]	// tmp96, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.119_1, mod_reg_val
	cmp	w0, 819	// mod_reg_val.119_1,
	bne	.L104		//,
	ldr	w0, [sp, 12]	// _2, mod_reg_addr
	b	.L105		//
.L104:
	ldr	w0, [sp, 8]	// _2, mod_reg_val
.L105:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun24, .-junk_fun24
	.align	2
	.type	junk_fun25, %function
junk_fun25:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 11	// tmp102,
	str	w0, [sp, 8]	// tmp102, mod_reg_val
	mov	w0, 6	// tmp103,
	str	w0, [sp, 12]	// tmp103, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.120_1, mod_reg_val
	add	w0, w1, 1	// _3, mod_reg_val.120_1,
	str	w0, [sp, 8]	// _3, mod_reg_val
	mov	w0, w1	// tmp104, mod_reg_val.120_1
	lsl	w0, w0, 1	// tmp105, tmp104,
	add	w0, w0, w1	// tmp104, tmp104, mod_reg_val.120_1
	lsl	w0, w0, 2	// tmp106, tmp104,
	cmp	w0, 7	// _4,
	bne	.L107		//,
	mov	w0, 51	// tmp107,
	str	w0, [sp, 12]	// tmp107, mod_reg_addr
.L107:
	ldr	w0, [sp, 8]	// mod_reg_val.122_5, mod_reg_val
	add	w0, w0, 1	// _6, mod_reg_val.122_5,
	str	w0, [sp, 8]	// _6, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.123_7, mod_reg_addr
	add	w0, w0, 51	// _8, mod_reg_addr.123_7,
	str	w0, [sp, 12]	// _8, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.124_9, mod_reg_addr
	cmp	w0, 16	// mod_reg_addr.124_9,
	bne	.L108		//,
	ldr	w0, [sp, 12]	// _10, mod_reg_addr
	b	.L109		//
.L108:
	ldr	w0, [sp, 8]	// _10, mod_reg_val
.L109:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun25, .-junk_fun25
	.align	2
	.type	junk_fun26, %function
junk_fun26:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 14	// tmp102,
	str	w0, [sp, 8]	// tmp102, mod_reg_val
	mov	w0, 6	// tmp103,
	str	w0, [sp, 12]	// tmp103, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.125_1, mod_reg_val
	cmp	w0, 15	// mod_reg_val.125_1,
	bne	.L111		//,
	ldr	w0, [sp, 12]	// _9, mod_reg_addr
	b	.L112		//
.L111:
	ldr	w0, [sp, 8]	// mod_reg_val.126_2, mod_reg_val
	cmp	w0, 15	// mod_reg_val.126_2,
	bne	.L113		//,
	ldr	w0, [sp, 12]	// _9, mod_reg_addr
	b	.L112		//
.L113:
	ldr	w0, [sp, 8]	// mod_reg_val.127_3, mod_reg_val
	add	w1, w0, 1	// _5, mod_reg_val.127_3,
	str	w1, [sp, 8]	// _5, mod_reg_val
	and	w0, w0, 8	// _6, mod_reg_val.127_3,
	cmp	w0, 0	// _6,
	bne	.L114		//,
	ldr	w1, [sp, 8]	// mod_reg_val.129_16, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.130_7, mod_reg_addr
	mul	w0, w1, w0	// _8, mod_reg_val.129_16, mod_reg_addr.130_7
	str	w0, [sp, 12]	// _8, mod_reg_addr
.L114:
	ldr	w0, [sp, 8]	// _9, mod_reg_val
.L112:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun26, .-junk_fun26
	.align	2
	.type	junk_fun27, %function
junk_fun27:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 5	// tmp108,
	str	w0, [sp, 8]	// tmp108, mod_reg_val
	mov	w0, 16	// tmp109,
	str	w0, [sp, 12]	// tmp109, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.131_17, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.132_1, mod_reg_val
	eor	w0, w1, w0	// _2, mod_reg_addr.131_17, mod_reg_val.132_1
	str	w0, [sp, 8]	// _2, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.133_3, mod_reg_val
	and	w0, w0, 6	// _4, mod_reg_val.133_3,
	str	w0, [sp, 12]	// _4, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.134_20, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.135_5, mod_reg_val
	add	w0, w1, w0	// _6, mod_reg_addr.134_20, mod_reg_val.135_5
	str	w0, [sp, 8]	// _6, mod_reg_val
	ldr	w1, [sp, 8]	// mod_reg_val.136_7, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.137_8, mod_reg_addr
	cmp	w1, w0	// mod_reg_val.136_7, mod_reg_addr.137_8
	bge	.L116		//,
	ldr	w1, [sp, 8]	// mod_reg_val.138_9, mod_reg_val
	mov	w0, w1	// tmp110, mod_reg_val.138_9
	lsl	w0, w0, 1	// tmp111, tmp110,
	add	w1, w0, w1	// _10, tmp110, mod_reg_val.138_9
	ldr	w0, [sp, 12]	// mod_reg_addr.139_11, mod_reg_addr
	add	w0, w1, w0	// _12, _10, mod_reg_addr.139_11
	str	w0, [sp, 12]	// _12, mod_reg_addr
.L116:
	ldr	w0, [sp, 8]	// _23, mod_reg_val
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun27, .-junk_fun27
	.align	2
	.type	junk_fun28, %function
junk_fun28:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 3	// tmp107,
	str	w0, [sp, 8]	// tmp107, mod_reg_val
	mov	w0, 4	// tmp108,
	str	w0, [sp, 12]	// tmp108, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.140_19, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.141_1, mod_reg_addr
	eor	w0, w1, w0	// _2, mod_reg_val.140_19, mod_reg_addr.141_1
	str	w0, [sp, 12]	// _2, mod_reg_addr
	ldr	w1, [sp, 12]	// mod_reg_addr.142_3, mod_reg_addr
	mov	w0, 819	// tmp109,
	mul	w0, w1, w0	// _4, mod_reg_addr.142_3, tmp109
	str	w0, [sp, 8]	// _4, mod_reg_val
	ldr	w0, [sp, 8]	// mod_reg_val.143_5, mod_reg_val
	cmp	w0, 16	// mod_reg_val.143_5,
	bne	.L119		//,
	ldr	w0, [sp, 12]	// _13, mod_reg_addr
	b	.L120		//
.L119:
	ldr	w0, [sp, 8]	// mod_reg_val.144_6, mod_reg_val
	cmp	w0, 819	// mod_reg_val.144_6,
	bne	.L121		//,
	ldr	w0, [sp, 12]	// _13, mod_reg_addr
	b	.L120		//
.L121:
	ldr	w1, [sp, 8]	// mod_reg_val.145_7, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.146_8, mod_reg_addr
	cmp	w1, w0	// mod_reg_val.145_7, mod_reg_addr.146_8
	bge	.L122		//,
	ldr	w1, [sp, 8]	// mod_reg_val.147_9, mod_reg_val
	mov	w0, w1	// tmp110, mod_reg_val.147_9
	lsl	w0, w0, 1	// tmp111, tmp110,
	add	w1, w0, w1	// _10, tmp110, mod_reg_val.147_9
	ldr	w0, [sp, 12]	// mod_reg_addr.148_11, mod_reg_addr
	add	w0, w1, w0	// _12, _10, mod_reg_addr.148_11
	str	w0, [sp, 12]	// _12, mod_reg_addr
.L122:
	ldr	w0, [sp, 8]	// _13, mod_reg_val
.L120:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun28, .-junk_fun28
	.align	2
	.type	junk_fun29, %function
junk_fun29:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 14	// tmp99,
	str	w0, [sp, 8]	// tmp99, mod_reg_val
	mov	w0, 7	// tmp100,
	str	w0, [sp, 12]	// tmp100, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.149_1, mod_reg_val
	cmp	w0, 16	// mod_reg_val.149_1,
	bne	.L124		//,
	ldr	w0, [sp, 12]	// _6, mod_reg_addr
	b	.L125		//
.L124:
	ldr	w0, [sp, 8]	// mod_reg_val.150_2, mod_reg_val
	and	w0, w0, 6	// _3, mod_reg_val.150_2,
	str	w0, [sp, 12]	// _3, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.151_4, mod_reg_addr
	cmp	w0, 15	// mod_reg_addr.151_4,
	bne	.L126		//,
	ldr	w0, [sp, 8]	// _6, mod_reg_val
	b	.L125		//
.L126:
	ldr	w0, [sp, 12]	// mod_reg_addr.152_5, mod_reg_addr
	cmp	w0, 15	// mod_reg_addr.152_5,
	bne	.L127		//,
	ldr	w0, [sp, 8]	// _6, mod_reg_val
	b	.L125		//
.L127:
	ldr	w0, [sp, 8]	// _6, mod_reg_val
.L125:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun29, .-junk_fun29
	.align	2
	.type	junk_fun30, %function
junk_fun30:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 11	// tmp97,
	str	w0, [sp, 8]	// tmp97, mod_reg_val
	mov	w0, 2	// tmp98,
	str	w0, [sp, 12]	// tmp98, mod_reg_addr
	ldr	w0, [sp, 8]	// mod_reg_val.153_1, mod_reg_val
	cmp	w0, 819	// mod_reg_val.153_1,
	bne	.L129		//,
	ldr	w0, [sp, 12]	// _4, mod_reg_addr
	b	.L130		//
.L129:
	ldr	w1, [sp, 12]	// mod_reg_addr.154_2, mod_reg_addr
	mov	w0, 819	// tmp99,
	mul	w0, w1, w0	// _3, mod_reg_addr.154_2, tmp99
	str	w0, [sp, 8]	// _3, mod_reg_val
	ldr	w0, [sp, 8]	// _4, mod_reg_val
.L130:
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	junk_fun30, .-junk_fun30
	.align	2
	.type	fun_a10, %function
fun_a10:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 512	// tmp100,
	str	w0, [sp, 8]	// tmp100, mod_reg_val
	mov	w0, 50	// tmp101,
	str	w0, [sp, 12]	// tmp101, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.155_1, mod_reg_addr
	add	w0, w0, 100	// _2, mod_reg_addr.155_1,
	str	w0, [sp, 12]	// _2, mod_reg_addr
	ldr	w0, [sp, 12]	// mod_reg_addr.156_3, mod_reg_addr
	sub	w0, w0, #100	// _4, mod_reg_addr.156_3,
	str	w0, [sp, 12]	// _4, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.157_5, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.158_6, mod_reg_addr
	add	w0, w1, w0	// _12, mod_reg_val.157_5, mod_reg_addr.158_6
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	fun_a10, .-fun_a10
	.align	2
	.type	fun_a11, %function
fun_a11:
	hint	25 // paciasp
	sub	sp, sp, #16	//,,
	mov	w0, 256	// tmp96,
	str	w0, [sp, 8]	// tmp96, mod_reg_val
	mov	w0, 66	// tmp97,
	str	w0, [sp, 12]	// tmp97, mod_reg_addr
	ldr	w1, [sp, 8]	// mod_reg_val.159_1, mod_reg_val
	ldr	w0, [sp, 12]	// mod_reg_addr.160_2, mod_reg_addr
	add	w0, w1, w0	// _6, mod_reg_val.159_1, mod_reg_addr.160_2
	add	sp, sp, 16	//,,
	hint	29 // autiasp
	ret
	.size	fun_a11, .-fun_a11
	.global	res
	.bss
	.align	3
	.type	res, %object
	.size	res, 32
res:
	.zero	32
	.text
	.align	2
	.global	awlink_asm_write_cmdreg
	.type	awlink_asm_write_cmdreg, %function
awlink_asm_write_cmdreg:
	hint	25 // paciasp
	stp	x29, x30, [sp, -48]!	//,,,
	mov	x29, sp	//,
	str	w0, [sp, 44]	// mod_reg_val, mod_reg_val
	str	x1, [sp, 32]	// mod_reg_addr, mod_reg_addr
	str	x2, [sp, 24]	// reg_addr, reg_addr
	bl	junk_fun1		//
	ldr	x0, [sp, 32]	// tmp93, mod_reg_addr
	add	x0, x0, 4	// _1, tmp93,
	mov	x1, x0	//, _1
	ldr	w0, [sp, 44]	//, mod_reg_val
	bl	writel		//
	bl	junk_fun2		//
	nop
	ldp	x29, x30, [sp], 48	//,,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_write_cmdreg, .-awlink_asm_write_cmdreg
	.align	2
	.global	awlink_asm_fun0
	.type	awlink_asm_fun0, %function
awlink_asm_fun0:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// num, num
	str	x1, [sp, 32]	// pin_id, pin_id
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp93,
	add	x0, x0, :lo12:res	// tmp92, tmp93,
	str	x0, [sp]	// tmp92,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	ldr	x4, [sp, 32]	//, pin_id
	mov	x3, 0	//,
	ldr	x2, [sp, 40]	//, num
	mov	w1, 4294901767	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	nop
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun0, .-awlink_asm_fun0
	.align	2
	.global	awlink_asm_fun1
	.type	awlink_asm_fun1, %function
awlink_asm_fun1:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// num, num
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp93,
	add	x0, x0, :lo12:res	// tmp92, tmp93,
	str	x0, [sp]	// tmp92,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	mov	x3, 0	//,
	ldr	x2, [sp, 40]	//, num
	mov	w1, 4294901763	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	nop
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun1, .-awlink_asm_fun1
	.align	2
	.global	awlink_asm_fun2
	.type	awlink_asm_fun2, %function
awlink_asm_fun2:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// num, num
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp93,
	add	x0, x0, :lo12:res	// tmp92, tmp93,
	str	x0, [sp]	// tmp92,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	mov	x3, 0	//,
	ldr	x2, [sp, 40]	//, num
	mov	w1, 4294901764	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	nop
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun2, .-awlink_asm_fun2
	.align	2
	.global	awlink_asm_fun3
	.type	awlink_asm_fun3, %function
awlink_asm_fun3:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// num, num
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp93,
	add	x0, x0, :lo12:res	// tmp92, tmp93,
	str	x0, [sp]	// tmp92,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	mov	x3, 0	//,
	mov	x2, 0	//,
	mov	w1, 4294901765	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	nop
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun3, .-awlink_asm_fun3
	.align	2
	.global	awlink_asm_fun4
	.type	awlink_asm_fun4, %function
awlink_asm_fun4:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// num, num
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp93,
	add	x0, x0, :lo12:res	// tmp92, tmp93,
	str	x0, [sp]	// tmp92,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	mov	x3, 0	//,
	mov	x2, 0	//,
	mov	w1, 4294901766	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	nop
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun4, .-awlink_asm_fun4
	.align	2
	.global	awlink_asm_fun5
	.type	awlink_asm_fun5, %function
awlink_asm_fun5:
	hint	25 // paciasp
	sub	sp, sp, #48	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// num, num
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp100,
	add	x0, x0, :lo12:res	// tmp99, tmp100,
	str	x0, [sp]	// tmp99,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	mov	x3, 0	//,
	ldr	x2, [sp, 40]	//, num
	mov	w1, 4294901768	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	adrp	x0, res	// tmp102,
	add	x0, x0, :lo12:res	// tmp101, tmp102,
	ldr	x3, [x0]	// _1, res.a0
	adrp	x0, res	// tmp104,
	add	x0, x0, :lo12:res	// tmp103, tmp104,
	ldr	x1, [x0, 8]	// _2, res.a1
	adrp	x0, arm64_use_ng_mappings	// tmp106,
	add	x0, x0, :lo12:arm64_use_ng_mappings	// tmp105, tmp106,
	ldrb	w0, [x0]	// arm64_use_ng_mappings.162_3, arm64_use_ng_mappings
	cmp	w0, 0	// arm64_use_ng_mappings.162_3,
	beq	.L142		//,
	mov	x0, 3859	// iftmp.161_4,
	movk	x0, 0x68, lsl 48	// iftmp.161_4,,
	b	.L143		//
.L142:
	mov	x0, 1811	// iftmp.161_4,
	movk	x0, 0x68, lsl 48	// iftmp.161_4,,
.L143:
	mov	x2, x0	//, D.69569
	mov	x0, x3	//, _1
	bl	__ioremap		//
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 48	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun5, .-awlink_asm_fun5
	.align	2
	.global	awlink_asm_start
	.type	awlink_asm_start, %function
awlink_asm_start:
	hint	25 // paciasp
	sub	sp, sp, #80	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 56]	// mod_reg_addr, mod_reg_addr
	str	x1, [sp, 48]	// ctrlmode, ctrlmode
	str	x2, [sp, 40]	// num, num
	mov	w0, 6	// tmp99,
	str	w0, [sp, 76]	// tmp99, mod_reg_val
	bl	junk_fun4		//
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp101,
	add	x0, x0, :lo12:res	// tmp100, tmp101,
	str	x0, [sp]	// tmp100,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	ldr	x3, [sp, 48]	//, ctrlmode
	ldr	x2, [sp, 40]	//, num
	mov	w1, 4294901761	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	adrp	x0, res	// tmp103,
	add	x0, x0, :lo12:res	// tmp102, tmp103,
	ldr	x0, [x0]	// _1, res.a0
	cmp	x0, 0	// _1,
	bne	.L146		//,
	bl	junk_fun5		//
	ldr	w0, [sp, 76]	// tmp105, mod_reg_val
	lsl	w0, w0, 12	// tmp104, tmp105,
	str	w0, [sp, 76]	// tmp104, mod_reg_val
	bl	junk_fun6		//
	b	.L151		//
.L146:
	ldr	w0, [sp, 76]	// tmp107, mod_reg_val
	lsl	w0, w0, 6	// tmp106, tmp107,
	str	w0, [sp, 76]	// tmp106, mod_reg_val
	ldr	w0, [sp, 76]	// tmp109, mod_reg_val
	and	w0, w0, 65535	// tmp108, tmp109,
	str	w0, [sp, 76]	// tmp108, mod_reg_val
	bl	fun_a10		//
	mov	w1, w0	// _3, _2
	ldr	w0, [sp, 76]	// tmp110, mod_reg_val
	cmp	w0, w1	// tmp110, _3
	bcs	.L148		//,
	ldr	x0, [sp, 56]	// tmp111, mod_reg_addr
	add	x0, x0, 16	// _4, tmp111,
	mov	x1, x0	//, _4
	mov	w0, 48	//,
	bl	writel		//
	b	.L150		//
.L148:
	bl	fun_a11		//
	mov	w1, w0	// _6, _5
	ldr	w0, [sp, 76]	// tmp112, mod_reg_val
	cmp	w0, w1	// tmp112, _6
	bls	.L149		//,
	ldr	x0, [sp, 56]	// tmp113, mod_reg_addr
	add	x0, x0, 16	// _7, tmp113,
	mov	x1, x0	//, _7
	mov	w0, 48	//,
	bl	writel		//
.L150:
	nop
.L149:
	bl	junk_fun7		//
.L151:
	nop
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 80	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_start, .-awlink_asm_start
	.align	2
	.global	awlink_asm_set_bittiming
	.type	awlink_asm_set_bittiming, %function
awlink_asm_set_bittiming:
	hint	25 // paciasp
	stp	x29, x30, [sp, -48]!	//,,,
	mov	x29, sp	//,
	str	x0, [sp, 40]	// mod_reg_addr, mod_reg_addr
	str	w1, [sp, 36]	// mod_reg_val, mod_reg_val
	str	x2, [sp, 24]	// cfg, cfg
	str	x3, [sp, 16]	// reg_addr, reg_addr
	bl	junk_fun9		//
	ldr	w0, [sp, 36]	// tmp97, mod_reg_val
	and	w0, w0, 4	// _1, tmp97,
	cmp	w0, 0	// _1,
	beq	.L153		//,
	ldr	x0, [sp, 24]	// tmp98, cfg
	ldr	w0, [x0]	// _2, *cfg_10(D)
	orr	w1, w0, 8388608	// _3, _2,
	ldr	x0, [sp, 24]	// tmp99, cfg
	str	w1, [x0]	// _3, *cfg_10(D)
.L153:
	bl	junk_fun10		//
	ldr	x0, [sp, 24]	// tmp100, cfg
	ldr	w2, [x0]	// _4, *cfg_10(D)
	ldr	x0, [sp, 40]	// tmp101, mod_reg_addr
	add	x0, x0, 20	// _5, tmp101,
	mov	x1, x0	//, _5
	mov	w0, w2	//, _4
	bl	writel		//
	nop
	ldp	x29, x30, [sp], 48	//,,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_set_bittiming, .-awlink_asm_set_bittiming
	.align	2
	.global	awlink_asm_clean_transfer_err
	.type	awlink_asm_clean_transfer_err, %function
awlink_asm_clean_transfer_err:
	hint	25 // paciasp
	stp	x29, x30, [sp, -64]!	//,,,
	mov	x29, sp	//,
	str	x0, [sp, 40]	// mod_reg_addr, mod_reg_addr
	str	x1, [sp, 32]	// t_err, t_err
	str	x2, [sp, 24]	// r_err, r_err
	bl	junk_fun12		//
	ldr	x0, [sp, 40]	// tmp98, mod_reg_addr
	add	x0, x0, 28	// _1, tmp98,
	bl	readl		//
	str	w0, [sp, 60]	//, errors
	bl	junk_fun13		//
	ldr	w0, [sp, 60]	// tmp99, errors
	and	w0, w0, 65535	// _2, tmp99
	and	w0, w0, 255	// tmp100, _2,
	and	w1, w0, 65535	// _3, tmp100
	ldr	x0, [sp, 32]	// tmp101, t_err
	strh	w1, [x0]	// tmp102, *t_err_13(D)
	ldr	w0, [sp, 60]	// tmp103, errors
	lsr	w0, w0, 16	// _4, tmp103,
	and	w0, w0, 65535	// _5, _4
	and	w0, w0, 255	// tmp104, _5,
	and	w1, w0, 65535	// _6, tmp104
	ldr	x0, [sp, 24]	// tmp105, r_err
	strh	w1, [x0]	// tmp106, *r_err_15(D)
	nop
	ldp	x29, x30, [sp], 64	//,,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_clean_transfer_err, .-awlink_asm_clean_transfer_err
	.align	2
	.global	awlink_asm_start_xmit
	.type	awlink_asm_start_xmit, %function
awlink_asm_start_xmit:
	hint	25 // paciasp
	stp	x29, x30, [sp, -80]!	//,,,
	mov	x29, sp	//,
	str	x0, [sp, 56]	// mod_reg_addr, mod_reg_addr
	str	x1, [sp, 48]	// addr, addr
	str	w2, [sp, 44]	// id, id
	str	x3, [sp, 32]	// flag, flag
	str	x4, [sp, 24]	// mod_reg_val, mod_reg_val
	str	x5, [sp, 16]	// pdata, pdata
	str	wzr, [sp, 72]	//, i
	ldr	x0, [sp, 32]	// tmp129, flag
	ldr	w0, [x0]	// tmp130, *flag_44(D)
	str	w0, [sp, 76]	// tmp130, dlc
	ldr	w0, [sp, 44]	// tmp131, id
	and	w0, w0, 1073741824	// _1, tmp131,
	cmp	w0, 0	// _1,
	beq	.L156		//,
	ldr	x0, [sp, 32]	// tmp132, flag
	ldr	w0, [x0]	// _2, *flag_44(D)
	orr	w1, w0, 64	// _3, _2,
	ldr	x0, [sp, 32]	// tmp133, flag
	str	w1, [x0]	// _3, *flag_44(D)
.L156:
	ldr	w0, [sp, 44]	// id.163_4, id
	cmp	w0, 0	// id.163_4,
	bge	.L157		//,
	ldr	x0, [sp, 32]	// tmp134, flag
	ldr	w0, [x0]	// _5, *flag_44(D)
	orr	w1, w0, 128	// _6, _5,
	ldr	x0, [sp, 32]	// tmp135, flag
	str	w1, [x0]	// _6, *flag_44(D)
	ldr	x0, [sp, 24]	// tmp136, mod_reg_val
	mov	w1, 84	// tmp137,
	str	w1, [x0]	// tmp137, *mod_reg_val_48(D)
	ldr	w0, [sp, 44]	// tmp138, id
	lsr	w0, w0, 21	// _7, tmp138,
	and	w2, w0, 255	// _8, _7,
	ldr	x0, [sp, 56]	// tmp139, mod_reg_addr
	add	x0, x0, 68	// _9, tmp139,
	mov	x1, x0	//, _9
	mov	w0, w2	//, _8
	bl	writel		//
	bl	junk_fun14		//
	ldr	w0, [sp, 44]	// tmp140, id
	lsr	w0, w0, 13	// _10, tmp140,
	and	w2, w0, 255	// _11, _10,
	ldr	x0, [sp, 56]	// tmp141, mod_reg_addr
	add	x0, x0, 72	// _12, tmp141,
	mov	x1, x0	//, _12
	mov	w0, w2	//, _11
	bl	writel		//
	ldr	w0, [sp, 44]	// tmp142, id
	lsr	w0, w0, 5	// _13, tmp142,
	and	w2, w0, 255	// _14, _13,
	ldr	x0, [sp, 56]	// tmp143, mod_reg_addr
	add	x0, x0, 76	// _15, tmp143,
	mov	x1, x0	//, _15
	mov	w0, w2	//, _14
	bl	writel		//
	ldr	w0, [sp, 44]	// tmp144, id
	lsl	w0, w0, 3	// _16, tmp144,
	and	w2, w0, 255	// _17, _16,
	ldr	x0, [sp, 56]	// tmp145, mod_reg_addr
	add	x0, x0, 80	// _18, tmp145,
	mov	x1, x0	//, _18
	mov	w0, w2	//, _17
	bl	writel		//
	b	.L158		//
.L157:
	ldr	x0, [sp, 24]	// tmp146, mod_reg_val
	mov	w1, 76	// tmp147,
	str	w1, [x0]	// tmp147, *mod_reg_val_48(D)
	ldr	w0, [sp, 44]	// tmp148, id
	lsr	w0, w0, 3	// _19, tmp148,
	and	w2, w0, 255	// _20, _19,
	ldr	x0, [sp, 56]	// tmp149, mod_reg_addr
	add	x0, x0, 68	// _21, tmp149,
	mov	x1, x0	//, _21
	mov	w0, w2	//, _20
	bl	writel		//
	ldr	w0, [sp, 44]	// tmp150, id
	lsl	w0, w0, 5	// _22, tmp150,
	and	w2, w0, 255	// _23, _22,
	ldr	x0, [sp, 56]	// tmp151, mod_reg_addr
	add	x0, x0, 72	// _24, tmp151,
	mov	x1, x0	//, _24
	mov	w0, w2	//, _23
	bl	writel		//
.L158:
	str	wzr, [sp, 72]	//, i
	b	.L159		//
.L160:
	ldrsw	x0, [sp, 72]	// _25, i
	ldr	x1, [sp, 16]	// tmp152, pdata
	add	x0, x1, x0	// _26, tmp152, _25
	ldrb	w0, [x0]	// _27, *_26
	mov	w2, w0	// _28, _27
	ldr	x0, [sp, 24]	// tmp153, mod_reg_val
	ldr	w0, [x0]	// _29, *mod_reg_val_48(D)
	ldr	w1, [sp, 72]	// tmp154, i
	lsl	w1, w1, 2	// _30, tmp154,
	add	w0, w0, w1	// _32, _29, _31
	uxtw	x0, w0	// _33, _32
	ldr	x1, [sp, 56]	// tmp155, mod_reg_addr
	add	x0, x1, x0	// _34, tmp155, _33
	mov	x1, x0	//, _34
	mov	w0, w2	//, _28
	bl	writel		//
	ldr	w0, [sp, 72]	// tmp157, i
	add	w0, w0, 1	// tmp156, tmp157,
	str	w0, [sp, 72]	// tmp156, i
.L159:
	ldr	w0, [sp, 72]	// i.164_35, i
	ldr	w1, [sp, 76]	// tmp158, dlc
	cmp	w1, w0	// tmp158, i.164_35
	bhi	.L160		//,
	ldr	x0, [sp, 32]	// tmp159, flag
	ldr	w2, [x0]	// _36, *flag_44(D)
	ldr	x0, [sp, 56]	// tmp160, mod_reg_addr
	add	x0, x0, 64	// _37, tmp160,
	mov	x1, x0	//, _37
	mov	w0, w2	//, _36
	bl	writel		//
	nop
	ldp	x29, x30, [sp], 80	//,,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_start_xmit, .-awlink_asm_start_xmit
	.align	2
	.global	awlink_asm_rx
	.type	awlink_asm_rx, %function
awlink_asm_rx:
	hint	25 // paciasp
	stp	x29, x30, [sp, -80]!	//,,,
	mov	x29, sp	//,
	str	x19, [sp, 16]	//,
	str	x0, [sp, 56]	// mod_reg_addr, mod_reg_addr
	str	x1, [sp, 48]	// r_fi, r_fi
	str	x2, [sp, 40]	// mod_reg_val, mod_reg_val
	str	x3, [sp, 32]	// id, id
	strb	wzr, [sp, 79]	//, fi
	ldr	x0, [sp, 56]	// tmp121, mod_reg_addr
	add	x0, x0, 64	// _1, tmp121,
	bl	readl		//
	strb	w0, [sp, 79]	// tmp122, fi
	ldrsb	w0, [sp, 79]	// fi.165_3, fi
	cmp	w0, 0	// fi.165_3,
	bge	.L162		//,
	ldr	x0, [sp, 40]	// tmp123, mod_reg_val
	mov	w1, 84	// tmp124,
	str	w1, [x0]	// tmp124, *mod_reg_val_36(D)
	bl	junk_fun17		//
	ldr	x0, [sp, 56]	// tmp125, mod_reg_addr
	add	x0, x0, 68	// _4, tmp125,
	bl	readl		//
	lsl	w19, w0, 21	// _6, _5,
	ldr	x0, [sp, 56]	// tmp126, mod_reg_addr
	add	x0, x0, 72	// _7, tmp126,
	bl	readl		//
	lsl	w0, w0, 13	// _9, _8,
	orr	w19, w19, w0	// _10, _6, _9
	ldr	x0, [sp, 56]	// tmp127, mod_reg_addr
	add	x0, x0, 76	// _11, tmp127,
	bl	readl		//
	lsl	w0, w0, 5	// _13, _12,
	orr	w19, w19, w0	// _14, _10, _13
	ldr	x0, [sp, 56]	// tmp128, mod_reg_addr
	add	x0, x0, 80	// _15, tmp128,
	bl	readl		//
	lsr	w0, w0, 3	// _17, _16,
	and	w0, w0, 31	// _18, _17,
	orr	w1, w19, w0	// _19, _14, _18
	ldr	x0, [sp, 32]	// tmp129, id
	str	w1, [x0]	// _19, *id_40(D)
	ldr	x0, [sp, 32]	// tmp130, id
	ldr	w0, [x0]	// _20, *id_40(D)
	orr	w1, w0, -2147483648	// _21, _20,
	ldr	x0, [sp, 32]	// tmp131, id
	str	w1, [x0]	// _21, *id_40(D)
	b	.L163		//
.L162:
	ldr	x0, [sp, 40]	// tmp132, mod_reg_val
	mov	w1, 76	// tmp133,
	str	w1, [x0]	// tmp133, *mod_reg_val_36(D)
	ldr	x0, [sp, 56]	// tmp134, mod_reg_addr
	add	x0, x0, 68	// _22, tmp134,
	bl	readl		//
	lsl	w19, w0, 3	// _24, _23,
	ldr	x0, [sp, 56]	// tmp135, mod_reg_addr
	add	x0, x0, 72	// _25, tmp135,
	bl	readl		//
	lsr	w0, w0, 5	// _27, _26,
	and	w0, w0, 7	// _28, _27,
	orr	w1, w19, w0	// _29, _24, _28
	ldr	x0, [sp, 32]	// tmp136, id
	str	w1, [x0]	// _29, *id_40(D)
.L163:
	ldr	x0, [sp, 48]	// tmp137, r_fi
	ldrb	w1, [sp, 79]	// tmp138, fi
	strb	w1, [x0]	// tmp138, *r_fi_50(D)
	nop
	ldr	x19, [sp, 16]	//,
	ldp	x29, x30, [sp], 80	//,,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_rx, .-awlink_asm_rx
	.section	.rodata
	.align	3
.LC8:
	.string	"reg"
	.text
	.align	2
	.global	awlink_asm_probe
	.type	awlink_asm_probe, %function
awlink_asm_probe:
	hint	25 // paciasp
	sub	sp, sp, #176	//,,
	stp	x29, x30, [sp, 16]	//,,
	add	x29, sp, 16	//,,
	str	x0, [sp, 40]	// node, node
	str	x1, [sp, 32]	// num, num
	mrs	x0, sp_el0	// tmp100
	add	x0, x0, 1048	// tmp101, tmp100,
	ldr	x1, [x0]	// tmp134,
	str	x1, [sp, 168]	// tmp134, D.69929
	mov	x1, 0	// tmp134
	mov	w0, -1	// tmp102,
	str	w0, [sp, 60]	// tmp102, data
	mov	w0, 35	// tmp103,
	str	w0, [sp, 64]	// tmp103, mod_reg_val
	mov	w0, 1	// tmp104,
	str	w0, [sp, 68]	// tmp104, mod_reg_addr
	adrp	x0, res	// tmp106,
	add	x0, x0, :lo12:res	// tmp105, tmp106,
	mov	x1, -1	// tmp107,
	str	x1, [x0]	// tmp107, res.a0
	str	xzr, [sp, 8]	//,
	adrp	x0, res	// tmp109,
	add	x0, x0, :lo12:res	// tmp108, tmp109,
	str	x0, [sp]	// tmp108,
	mov	x7, 0	//,
	mov	x6, 0	//,
	mov	x5, 0	//,
	mov	x4, 0	//,
	mov	x3, 0	//,
	ldr	x2, [sp, 32]	//, num
	mov	w1, 4294901762	//,
	mov	x0, 160	//,
	movk	x0, 0xc000, lsl 16	//,,
	bl	__arm_smccc_smc		//
	adrp	x0, res	// tmp111,
	add	x0, x0, :lo12:res	// tmp110, tmp111,
	ldr	x0, [x0]	// _1, res.a0
	cmp	x0, 0	// _1,
	bne	.L165		//,
	mov	x2, 0	//,
	adrp	x0, .LC8	// tmp112,
	add	x1, x0, :lo12:.LC8	//, tmp112,
	ldr	x0, [sp, 40]	//, node
	bl	of_get_property		//
	str	x0, [sp, 72]	//, be_addr
	ldr	x0, [sp, 40]	//, node
	bl	of_irq_find_parent		//
	str	x0, [sp, 80]	//, p
	ldr	x0, [sp, 80]	// tmp113, p
	cmp	x0, 0	// tmp113,
	bne	.L166		//,
	mov	w0, -1	// _8,
	b	.L168		//
.L166:
	ldr	x0, [sp, 80]	// tmp114, p
	str	x0, [sp, 88]	// tmp114, oirq.np
	mov	w0, 3	// tmp115,
	str	w0, [sp, 96]	// tmp115, oirq.args_count
	str	wzr, [sp, 100]	//, oirq.args[0]
	ldr	w1, [sp, 64]	// tmp117, mod_reg_val
	ldr	w0, [sp, 68]	// tmp118, mod_reg_addr
	add	w0, w1, w0	// tmp116, tmp117, tmp118
	str	w0, [sp, 64]	// tmp116, mod_reg_val
	bl	junk_fun3		//
	ldr	x1, [sp, 32]	// tmp119, num
	mov	x0, x1	// tmp120, tmp119
	lsl	x0, x0, 6	// tmp121, tmp120,
	add	x0, x0, x1	// tmp120, tmp120, tmp119
	lsl	x0, x0, 1	// tmp122, tmp120,
	str	x0, [sp, 32]	// tmp120, num
	ldr	x0, [sp, 32]	// tmp123, num
	mov	w1, w0	// _2, tmp123
	ldr	w0, [sp, 64]	// mod_reg_val.166_3, mod_reg_val
	add	w0, w1, w0	// _4, _2, mod_reg_val.166_3
	str	w0, [sp, 64]	// _4, mod_reg_val
	ldr	w2, [sp, 64]	// mod_reg_val.167_5, mod_reg_val
	ldrsw	x0, [sp, 68]	// tmp124, mod_reg_addr
	lsl	x0, x0, 2	// tmp125, tmp124,
	add	x1, sp, 100	// tmp126,,
	str	w2, [x1, x0]	// mod_reg_val.167_5, oirq.args[mod_reg_addr_13]
	bl	junk_fun15		//
	mov	w0, 4	// tmp127,
	str	w0, [sp, 108]	// tmp127, oirq.args[2]
	add	x0, sp, 88	// tmp128,,
	mov	x1, x0	//, tmp128
	ldr	x0, [sp, 72]	//, be_addr
	bl	of_irq_parse_raw		//
	bl	junk_fun16		//
	add	x0, sp, 88	// tmp129,,
	bl	irq_create_of_mapping		//
	str	w0, [sp, 60]	// _6, data
.L165:
	ldr	w0, [sp, 60]	// _8, data
.L168:
	mov	w1, w0	// <retval>, _8
	mrs	x0, sp_el0	// tmp131
	add	x0, x0, 1048	// tmp132, tmp131,
	ldr	x2, [sp, 168]	// tmp135, D.69929
	ldr	x3, [x0]	// tmp136,
	subs	x2, x2, x3	// tmp135, tmp136
	mov	x3, 0	// tmp136
	beq	.L169		//,
.L169:
	mov	w0, w1	//, <retval>
	ldp	x29, x30, [sp, 16]	//,,
	add	sp, sp, 176	//,,
	hint	29 // autiasp
	ret
	.size	awlink_asm_probe, .-awlink_asm_probe
	.ident	"GCC: (GNU Toolchain for the A-profile Architecture 10.3-2021.07 (arm-10.29)) 10.3.1 20210621"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align	3
	.word	4
	.word	16
	.word	5
	.string	"GNU"
	.word	3221225472
	.word	4
	.word	2
	.align	3

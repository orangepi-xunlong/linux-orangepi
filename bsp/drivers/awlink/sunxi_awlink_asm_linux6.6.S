	.arch armv8-a
	.file	"sunxi_awlink_asm_linux6.6.c"
	.text
	.section	.rodata
	.type	class_preempt_is_conditional, %object
	.size	class_preempt_is_conditional, 1
class_preempt_is_conditional:
	.zero	1
	.type	class_preempt_notrace_is_conditional, %object
	.size	class_preempt_notrace_is_conditional, 1
class_preempt_notrace_is_conditional:
	.zero	1
	.type	class_migrate_is_conditional, %object
	.size	class_migrate_is_conditional, 1
class_migrate_is_conditional:
	.zero	1
	.type	class_irq_is_conditional, %object
	.size	class_irq_is_conditional, 1
class_irq_is_conditional:
	.zero	1
	.type	class_irqsave_is_conditional, %object
	.size	class_irqsave_is_conditional, 1
class_irqsave_is_conditional:
	.zero	1
	.type	class_raw_spinlock_is_conditional, %object
	.size	class_raw_spinlock_is_conditional, 1
class_raw_spinlock_is_conditional:
	.zero	1
	.type	class_raw_spinlock_try_is_conditional, %object
	.size	class_raw_spinlock_try_is_conditional, 1
class_raw_spinlock_try_is_conditional:
	.byte	1
	.type	class_raw_spinlock_nested_is_conditional, %object
	.size	class_raw_spinlock_nested_is_conditional, 1
class_raw_spinlock_nested_is_conditional:
	.zero	1
	.type	class_raw_spinlock_irq_is_conditional, %object
	.size	class_raw_spinlock_irq_is_conditional, 1
class_raw_spinlock_irq_is_conditional:
	.zero	1
	.type	class_raw_spinlock_irq_try_is_conditional, %object
	.size	class_raw_spinlock_irq_try_is_conditional, 1
class_raw_spinlock_irq_try_is_conditional:
	.byte	1
	.type	class_raw_spinlock_irqsave_is_conditional, %object
	.size	class_raw_spinlock_irqsave_is_conditional, 1
class_raw_spinlock_irqsave_is_conditional:
	.zero	1
	.type	class_raw_spinlock_irqsave_try_is_conditional, %object
	.size	class_raw_spinlock_irqsave_try_is_conditional, 1
class_raw_spinlock_irqsave_try_is_conditional:
	.byte	1
	.type	class_spinlock_is_conditional, %object
	.size	class_spinlock_is_conditional, 1
class_spinlock_is_conditional:
	.zero	1
	.type	class_spinlock_try_is_conditional, %object
	.size	class_spinlock_try_is_conditional, 1
class_spinlock_try_is_conditional:
	.byte	1
	.type	class_spinlock_irq_is_conditional, %object
	.size	class_spinlock_irq_is_conditional, 1
class_spinlock_irq_is_conditional:
	.zero	1
	.type	class_spinlock_irq_try_is_conditional, %object
	.size	class_spinlock_irq_try_is_conditional, 1
class_spinlock_irq_try_is_conditional:
	.byte	1
	.type	class_spinlock_irqsave_is_conditional, %object
	.size	class_spinlock_irqsave_is_conditional, 1
class_spinlock_irqsave_is_conditional:
	.zero	1
	.type	class_spinlock_irqsave_try_is_conditional, %object
	.size	class_spinlock_irqsave_try_is_conditional, 1
class_spinlock_irqsave_try_is_conditional:
	.byte	1
	.type	class_rcu_is_conditional, %object
	.size	class_rcu_is_conditional, 1
class_rcu_is_conditional:
	.zero	1
	.type	class_rwsem_read_is_conditional, %object
	.size	class_rwsem_read_is_conditional, 1
class_rwsem_read_is_conditional:
	.zero	1
	.type	class_rwsem_read_try_is_conditional, %object
	.size	class_rwsem_read_try_is_conditional, 1
class_rwsem_read_try_is_conditional:
	.byte	1
	.type	class_rwsem_read_intr_is_conditional, %object
	.size	class_rwsem_read_intr_is_conditional, 1
class_rwsem_read_intr_is_conditional:
	.byte	1
	.type	class_rwsem_write_is_conditional, %object
	.size	class_rwsem_write_is_conditional, 1
class_rwsem_write_is_conditional:
	.zero	1
	.type	class_rwsem_write_try_is_conditional, %object
	.size	class_rwsem_write_try_is_conditional, 1
class_rwsem_write_try_is_conditional:
	.byte	1
	.type	class_mutex_is_conditional, %object
	.size	class_mutex_is_conditional, 1
class_mutex_is_conditional:
	.zero	1
	.type	class_mutex_try_is_conditional, %object
	.size	class_mutex_try_is_conditional, 1
class_mutex_try_is_conditional:
	.byte	1
	.type	class_mutex_intr_is_conditional, %object
	.size	class_mutex_intr_is_conditional, 1
class_mutex_intr_is_conditional:
	.byte	1
	.type	class_srcu_is_conditional, %object
	.size	class_srcu_is_conditional, 1
class_srcu_is_conditional:
	.zero	1
	.text
	.align	2
	.type	folio_flags, %function
folio_flags:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	w0, [sp, 4]
	lsl	x0, x0, 6
	ldr	x1, [sp, 24]
	add	x0, x1, x0
	add	sp, sp, 32
	ret
	.size	folio_flags, .-folio_flags
	.align	2
	.type	mmap_assert_write_locked, %function
mmap_assert_write_locked:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	nop
	add	sp, sp, 16
	ret
	.size	mmap_assert_write_locked, .-mmap_assert_write_locked
	.align	2
	.type	log_write_mmio, %function
log_write_mmio:
	sub	sp, sp, #48
	str	x0, [sp, 40]
	strb	w1, [sp, 39]
	str	x2, [sp, 24]
	str	x3, [sp, 16]
	str	x4, [sp, 8]
	nop
	add	sp, sp, 48
	ret
	.size	log_write_mmio, .-log_write_mmio
	.align	2
	.type	log_post_write_mmio, %function
log_post_write_mmio:
	sub	sp, sp, #48
	str	x0, [sp, 40]
	strb	w1, [sp, 39]
	str	x2, [sp, 24]
	str	x3, [sp, 16]
	str	x4, [sp, 8]
	nop
	add	sp, sp, 48
	ret
	.size	log_post_write_mmio, .-log_post_write_mmio
	.align	2
	.type	log_read_mmio, %function
log_read_mmio:
	sub	sp, sp, #32
	strb	w0, [sp, 31]
	str	x1, [sp, 16]
	str	x2, [sp, 8]
	str	x3, [sp]
	nop
	add	sp, sp, 32
	ret
	.size	log_read_mmio, .-log_read_mmio
	.align	2
	.type	log_post_read_mmio, %function
log_post_read_mmio:
	sub	sp, sp, #48
	str	x0, [sp, 40]
	strb	w1, [sp, 39]
	str	x2, [sp, 24]
	str	x3, [sp, 16]
	str	x4, [sp, 8]
	nop
	add	sp, sp, 48
	ret
	.size	log_post_read_mmio, .-log_post_read_mmio
	.align	2
	.type	readl, %function
readl:
	hint	25 // paciasp
	stp	x29, x30, [sp, -80]!
	mov	x29, sp
	str	x19, [sp, 16]
	mov	x19, x30
	str	x0, [sp, 40]
.L9:
	adrp	x0, .L9
	add	x0, x0, :lo12:.L9
	mov	x30, x19
	hint	7 // xpaclri
	mov	x1, x30
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [sp, 40]
	mov	w0, 32
	bl	log_read_mmio
	ldr	x0, [sp, 40]
	str	x0, [sp, 56]
	ldr	x0, [sp, 56]
	.if 1 == 1
661:
	ldr w0, [x0]
662:
.pushsection .altinstructions,"a"
 .word 661b - .
 .word 663f - .
 .hword 91
 .byte 662b-661b
 .byte 664f-663f
.popsection
.subsection 1
663:
	ldar w0, [x0]
664:
	.org	. - (664b-663b) + (662b-661b)
	.org	. - (662b-661b) + (664b-663b)
	.previous
.endif

// 0 "" 2
#NO_APP
	str	w0, [sp, 52]
	ldr	w0, [sp, 52]
	str	w0, [sp, 76]
#APP
	dmb oshld
// 0 "" 2
#NO_APP
	ldr	w0, [sp, 76]
#APP
	eor	x0, x0, x0
cbnz	x0, .
// 0 "" 2
#NO_APP
	str	x0, [sp, 64]
	ldr	w5, [sp, 76]
.L11:
	adrp	x0, .L11
	add	x0, x0, :lo12:.L11
	mov	x30, x19
	hint	7 // xpaclri
	mov	x1, x30
	mov	x4, x1
	mov	x3, x0
	ldr	x2, [sp, 40]
	mov	w1, 32
	mov	x0, x5
	bl	log_post_read_mmio
	ldr	w0, [sp, 76]
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 80
	hint	29 // autiasp
	ret
	.size	readl, .-readl
	.align	2
	.type	writel, %function
writel:
	hint	25 // paciasp
	stp	x29, x30, [sp, -64]!
	mov	x29, sp
	str	x19, [sp, 16]
	mov	x19, x30
	str	w0, [sp, 44]
	str	x1, [sp, 32]
	ldr	w5, [sp, 44]
.L14:
	adrp	x0, .L14
	add	x0, x0, :lo12:.L14
	mov	x30, x19
	hint	7 // xpaclri
	mov	x1, x30
	mov	x4, x1
	mov	x3, x0
	ldr	x2, [sp, 32]
	mov	w1, 32
	mov	x0, x5
	bl	log_write_mmio
#APP
	dmb oshst
// 0 "" 2
#NO_APP
	ldr	w0, [sp, 44]
	str	w0, [sp, 60]
	ldr	x0, [sp, 32]
	str	x0, [sp, 48]
	ldr	w0, [sp, 60]
	ldr	x1, [sp, 48]
#APP
	str w0, [x1]
// 0 "" 2
#NO_APP
	nop
	ldr	w5, [sp, 44]
.L15:
	adrp	x0, .L15
	add	x0, x0, :lo12:.L15
	mov	x30, x19
	hint	7 // xpaclri
	mov	x1, x30
	mov	x4, x1
	mov	x3, x0
	ldr	x2, [sp, 32]
	mov	w1, 32
	mov	x0, x5
	bl	log_post_write_mmio
	nop
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 64
	hint	29 // autiasp
	ret
	.size	writel, .-writel
	.align	2
	.type	ioremap, %function
ioremap:
	hint	25 // paciasp
	stp	x29, x30, [sp, -32]!
	mov	x29, sp
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	adrp	x0, arm64_use_ng_mappings
	add	x0, x0, :lo12:arm64_use_ng_mappings
	ldrb	w0, [x0]
	cmp	w0, 0
	beq	.L17
	mov	x0, 3859
	movk	x0, 0x68, lsl 48
	b	.L18
.L17:
	mov	x0, 1811
	movk	x0, 0x68, lsl 48
.L18:
	mov	x2, x0
	ldr	x1, [sp, 16]
	ldr	x0, [sp, 24]
	bl	ioremap_prot
	ldp	x29, x30, [sp], 32
	hint	29 // autiasp
	ret
	.size	ioremap, .-ioremap
	.align	2
	.type	__is_vma_write_locked, %function
__is_vma_write_locked:
	hint	25 // paciasp
	stp	x29, x30, [sp, -32]!
	mov	x29, sp
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	bl	mmap_assert_write_locked
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	ldr	w1, [x0, 200]
	ldr	x0, [sp, 16]
	str	w1, [x0]
	ldr	x0, [sp, 24]
	ldr	w1, [x0, 40]
	ldr	x0, [sp, 16]
	ldr	w0, [x0]
	cmp	w1, w0
	cset	w0, eq
	and	w0, w0, 255
	ldp	x29, x30, [sp], 32
	hint	29 // autiasp
	ret
	.size	__is_vma_write_locked, .-__is_vma_write_locked
	.section	.rodata
	.type	class_device_is_conditional, %object
	.size	class_device_is_conditional, 1
class_device_is_conditional:
	.zero	1
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
	.text
	.align	2
	.type	junk_fun1, %function
junk_fun1:
	sub	sp, sp, #16
	mov	w0, 14
	str	w0, [sp, 12]
	mov	w0, 10
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	add	w1, w0, 1
	str	w1, [sp, 12]
	and	w0, w0, 8
	cmp	w0, 0
	bne	.L23
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	mul	w0, w1, w0
	str	w0, [sp, 8]
.L23:
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun1, .-junk_fun1
	.align	2
	.type	junk_fun2, %function
junk_fun2:
	sub	sp, sp, #16
	mov	w0, 4
	str	w0, [sp, 12]
	mov	w0, 10
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w0, 7
	bne	.L26
	ldr	w0, [sp, 12]
	b	.L27
.L26:
	ldr	w1, [sp, 12]
	add	w0, w1, 1
	str	w0, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 2
	add	w0, w0, w1
	lsl	w0, w0, 1
	cmp	w0, 7
	bne	.L28
	mov	w0, 51
	str	w0, [sp, 8]
.L28:
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L29
	ldr	w1, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w1, w0, w1
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L29:
	ldr	w1, [sp, 12]
	mov	w0, 243
	and	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
.L27:
	add	sp, sp, 16
	ret
	.size	junk_fun2, .-junk_fun2
	.align	2
	.type	junk_fun3, %function
junk_fun3:
	sub	sp, sp, #16
	mov	w0, 7
	str	w0, [sp, 12]
	mov	w0, 14
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L31
	ldr	w0, [sp, 8]
	b	.L32
.L31:
	ldr	w0, [sp, 12]
	add	w1, w0, 1
	str	w1, [sp, 12]
	and	w0, w0, 8
	cmp	w0, 0
	bne	.L33
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	mul	w0, w1, w0
	str	w0, [sp, 8]
.L33:
	ldr	w0, [sp, 12]
.L32:
	add	sp, sp, 16
	ret
	.size	junk_fun3, .-junk_fun3
	.align	2
	.type	junk_fun4, %function
junk_fun4:
	sub	sp, sp, #16
	mov	w0, 14
	str	w0, [sp, 12]
	mov	w0, 4
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	cmp	w0, 15
	bne	.L35
	ldr	w0, [sp, 8]
	b	.L36
.L35:
	ldr	w0, [sp, 12]
	cmp	w0, 15
	bne	.L37
	ldr	w0, [sp, 8]
	b	.L36
.L37:
	ldr	w0, [sp, 12]
.L36:
	add	sp, sp, 16
	ret
	.size	junk_fun4, .-junk_fun4
	.align	2
	.type	junk_fun5, %function
junk_fun5:
	sub	sp, sp, #16
	mov	w0, 6
	str	w0, [sp, 12]
	mov	w0, 2
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L39
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L39:
	ldr	w0, [sp, 8]
	cmp	w0, 7
	bne	.L40
	ldr	w0, [sp, 12]
	b	.L41
.L40:
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L42
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L42:
	ldr	w0, [sp, 12]
.L41:
	add	sp, sp, 16
	ret
	.size	junk_fun5, .-junk_fun5
	.align	2
	.type	junk_fun6, %function
junk_fun6:
	sub	sp, sp, #16
	mov	w0, 8
	str	w0, [sp, 12]
	mov	w0, 13
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	cmp	w0, 7
	bne	.L44
	ldr	w0, [sp, 12]
	b	.L45
.L44:
	ldr	w0, [sp, 12]
.L45:
	add	sp, sp, 16
	ret
	.size	junk_fun6, .-junk_fun6
	.align	2
	.type	junk_fun7, %function
junk_fun7:
	sub	sp, sp, #16
	mov	w0, 12
	str	w0, [sp, 12]
	mov	w0, 9
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L47
	ldr	w0, [sp, 8]
	b	.L48
.L47:
	ldr	w0, [sp, 8]
	cmp	w0, 7
	bne	.L49
	ldr	w0, [sp, 12]
	b	.L48
.L49:
	ldr	w1, [sp, 8]
	mov	w0, 819
	mul	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
.L48:
	add	sp, sp, 16
	ret
	.size	junk_fun7, .-junk_fun7
	.align	2
	.type	junk_fun8, %function
junk_fun8:
	sub	sp, sp, #16
	mov	w0, 1
	str	w0, [sp, 12]
	mov	w0, 14
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	add	w1, w0, 1
	str	w1, [sp, 12]
	and	w0, w0, 8
	cmp	w0, 0
	bne	.L51
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	mul	w0, w1, w0
	str	w0, [sp, 8]
.L51:
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L52
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L52:
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun8, .-junk_fun8
	.align	2
	.type	junk_fun9, %function
junk_fun9:
	sub	sp, sp, #16
	mov	w0, 2
	str	w0, [sp, 12]
	mov	w0, 1
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	add	w0, w1, 1
	str	w0, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 2
	add	w0, w0, w1
	lsl	w0, w0, 1
	cmp	w0, 7
	bne	.L55
	mov	w0, 51
	str	w0, [sp, 8]
.L55:
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	and	w0, w0, 7
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun9, .-junk_fun9
	.align	2
	.type	junk_fun10, %function
junk_fun10:
	sub	sp, sp, #16
	mov	w0, 2
	str	w0, [sp, 12]
	mov	w0, 6
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	cmp	w0, 15
	bne	.L58
	ldr	w0, [sp, 8]
	b	.L59
.L58:
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	cmp	w0, 15
	bne	.L60
	ldr	w0, [sp, 8]
	b	.L59
.L60:
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L61
	ldr	w0, [sp, 8]
	b	.L59
.L61:
	ldr	w0, [sp, 12]
	and	w0, w0, 7
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
.L59:
	add	sp, sp, 16
	ret
	.size	junk_fun10, .-junk_fun10
	.align	2
	.type	junk_fun11, %function
junk_fun11:
	sub	sp, sp, #16
	mov	w0, 2
	str	w0, [sp, 12]
	mov	w0, 7
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun11, .-junk_fun11
	.align	2
	.type	junk_fun12, %function
junk_fun12:
	sub	sp, sp, #16
	mov	w0, 14
	str	w0, [sp, 12]
	mov	w0, 9
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun12, .-junk_fun12
	.align	2
	.type	junk_fun13, %function
junk_fun13:
	sub	sp, sp, #16
	mov	w0, 5
	str	w0, [sp, 12]
	mov	w0, 15
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	cmp	w0, 7
	bne	.L67
	ldr	w0, [sp, 12]
	b	.L68
.L67:
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L69
	ldr	w0, [sp, 8]
	b	.L68
.L69:
	ldr	w0, [sp, 12]
.L68:
	add	sp, sp, 16
	ret
	.size	junk_fun13, .-junk_fun13
	.align	2
	.type	junk_fun14, %function
junk_fun14:
	sub	sp, sp, #16
	mov	w0, 6
	str	w0, [sp, 12]
	mov	w0, 2
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L71
	ldr	w0, [sp, 8]
	b	.L72
.L71:
	ldr	w1, [sp, 8]
	mov	w0, 819
	mul	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
.L72:
	add	sp, sp, 16
	ret
	.size	junk_fun14, .-junk_fun14
	.align	2
	.type	junk_fun15, %function
junk_fun15:
	sub	sp, sp, #16
	mov	w0, 16
	str	w0, [sp, 12]
	mov	w0, 9
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w0, 1
	str	w0, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w0, 51
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	cmp	w0, 15
	bne	.L74
	ldr	w0, [sp, 8]
	b	.L75
.L74:
	ldr	w0, [sp, 12]
	and	w0, w0, 7
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	mov	w0, 819
	mul	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	add	w0, w0, 1
	str	w0, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w0, 51
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	cmp	w0, 15
	bne	.L76
	ldr	w0, [sp, 8]
	b	.L75
.L76:
	ldr	w0, [sp, 12]
.L75:
	add	sp, sp, 16
	ret
	.size	junk_fun15, .-junk_fun15
	.align	2
	.type	junk_fun16, %function
junk_fun16:
	sub	sp, sp, #16
	mov	w0, 10
	str	w0, [sp, 12]
	mov	w0, 15
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	eor	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	mov	w0, 243
	and	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	add	w0, w1, 1
	str	w0, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 2
	add	w0, w0, w1
	lsl	w0, w0, 1
	cmp	w0, 7
	bne	.L78
	mov	w0, 51
	str	w0, [sp, 8]
.L78:
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L79
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L79:
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun16, .-junk_fun16
	.align	2
	.type	junk_fun17, %function
junk_fun17:
	sub	sp, sp, #16
	mov	w0, 8
	str	w0, [sp, 12]
	mov	w0, 2
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L82
	ldr	w1, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w1, w0, w1
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L82:
	ldr	w0, [sp, 12]
	cmp	w0, 15
	bne	.L83
	ldr	w0, [sp, 8]
	b	.L84
.L83:
	ldr	w0, [sp, 12]
	cmp	w0, 7
	bne	.L85
	ldr	w0, [sp, 8]
	b	.L84
.L85:
	ldr	w0, [sp, 12]
.L84:
	add	sp, sp, 16
	ret
	.size	junk_fun17, .-junk_fun17
	.align	2
	.type	junk_fun18, %function
junk_fun18:
	sub	sp, sp, #16
	mov	w0, 5
	str	w0, [sp, 12]
	mov	w0, 16
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L87
	ldr	w0, [sp, 8]
	b	.L88
.L87:
	ldr	w0, [sp, 12]
	add	w1, w0, 1
	str	w1, [sp, 12]
	and	w0, w0, 8
	cmp	w0, 0
	bne	.L89
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	mul	w0, w1, w0
	str	w0, [sp, 8]
.L89:
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L90
	ldr	w1, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w1, w0, w1
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L90:
	ldr	w0, [sp, 12]
.L88:
	add	sp, sp, 16
	ret
	.size	junk_fun18, .-junk_fun18
	.align	2
	.type	junk_fun19, %function
junk_fun19:
	sub	sp, sp, #16
	mov	w0, 16
	str	w0, [sp, 12]
	mov	w0, 1
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L92
	ldr	w0, [sp, 8]
	b	.L93
.L92:
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	and	w0, w0, 7
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L94
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L94:
	ldr	w0, [sp, 12]
.L93:
	add	sp, sp, 16
	ret
	.size	junk_fun19, .-junk_fun19
	.align	2
	.type	junk_fun20, %function
junk_fun20:
	sub	sp, sp, #16
	mov	w0, 12
	str	w0, [sp, 12]
	mov	w0, 9
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L96
	ldr	w0, [sp, 8]
	b	.L97
.L96:
	ldr	w0, [sp, 12]
.L97:
	add	sp, sp, 16
	ret
	.size	junk_fun20, .-junk_fun20
	.align	2
	.type	junk_fun21, %function
junk_fun21:
	sub	sp, sp, #16
	mov	w0, 1
	str	w0, [sp, 12]
	mov	w0, 9
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	cmp	w0, 7
	bne	.L99
	ldr	w0, [sp, 8]
	b	.L100
.L99:
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L101
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L101:
	ldr	w0, [sp, 12]
.L100:
	add	sp, sp, 16
	ret
	.size	junk_fun21, .-junk_fun21
	.align	2
	.type	junk_fun22, %function
junk_fun22:
	sub	sp, sp, #16
	mov	w0, 12
	str	w0, [sp, 12]
	mov	w0, 5
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	mov	w0, 243
	and	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	mov	w0, 243
	and	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L103
	ldr	w1, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w1, w0, w1
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L103:
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun22, .-junk_fun22
	.align	2
	.type	junk_fun23, %function
junk_fun23:
	sub	sp, sp, #16
	mov	w0, 2
	str	w0, [sp, 12]
	mov	w0, 6
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w0, 1
	str	w0, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w0, 51
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	cmp	w0, 15
	bne	.L106
	ldr	w0, [sp, 8]
	b	.L107
.L106:
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L108
	ldr	w0, [sp, 8]
	b	.L107
.L108:
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L109
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L109:
	ldr	w0, [sp, 12]
.L107:
	add	sp, sp, 16
	ret
	.size	junk_fun23, .-junk_fun23
	.align	2
	.type	junk_fun24, %function
junk_fun24:
	sub	sp, sp, #16
	mov	w0, 3
	str	w0, [sp, 12]
	mov	w0, 3
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	add	w0, w1, 1
	str	w0, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 2
	add	w0, w0, w1
	lsl	w0, w0, 1
	cmp	w0, 7
	bne	.L111
	mov	w0, 51
	str	w0, [sp, 8]
.L111:
	ldr	w0, [sp, 12]
	and	w0, w0, 7
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	cmp	w0, 7
	bne	.L112
	ldr	w0, [sp, 8]
	b	.L113
.L112:
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L114
	ldr	w1, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w1, w0, w1
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L114:
	ldr	w0, [sp, 12]
.L113:
	add	sp, sp, 16
	ret
	.size	junk_fun24, .-junk_fun24
	.align	2
	.type	junk_fun25, %function
junk_fun25:
	sub	sp, sp, #16
	mov	w0, 13
	str	w0, [sp, 12]
	mov	w0, 10
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L116
	ldr	w0, [sp, 8]
	b	.L117
.L116:
	ldr	w1, [sp, 12]
	mov	w0, 243
	and	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	add	w0, w1, 1
	str	w0, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 2
	add	w0, w0, w1
	lsl	w0, w0, 1
	cmp	w0, 7
	bne	.L118
	mov	w0, 51
	str	w0, [sp, 8]
.L118:
	ldr	w0, [sp, 12]
.L117:
	add	sp, sp, 16
	ret
	.size	junk_fun25, .-junk_fun25
	.align	2
	.type	junk_fun26, %function
junk_fun26:
	sub	sp, sp, #16
	mov	w0, 5
	str	w0, [sp, 12]
	mov	w0, 14
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L120
	ldr	w0, [sp, 8]
	b	.L121
.L120:
	ldr	w1, [sp, 8]
	mov	w0, 941
	and	w0, w1, w0
	cmp	w0, 768
	beq	.L122
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L122:
	ldr	w1, [sp, 8]
	mov	w0, 819
	mul	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
.L121:
	add	sp, sp, 16
	ret
	.size	junk_fun26, .-junk_fun26
	.align	2
	.type	junk_fun27, %function
junk_fun27:
	sub	sp, sp, #16
	mov	w0, 12
	str	w0, [sp, 12]
	mov	w0, 2
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	eor	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	mov	w0, 243
	and	w0, w1, w0
	str	w0, [sp, 8]
	ldr	w0, [sp, 12]
	cmp	w0, 15
	bne	.L124
	ldr	w0, [sp, 8]
	b	.L125
.L124:
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
.L125:
	add	sp, sp, 16
	ret
	.size	junk_fun27, .-junk_fun27
	.align	2
	.type	junk_fun28, %function
junk_fun28:
	sub	sp, sp, #16
	mov	w0, 10
	str	w0, [sp, 12]
	mov	w0, 16
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	add	w1, w0, 1
	str	w1, [sp, 12]
	and	w0, w0, 8
	cmp	w0, 0
	bne	.L127
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	mul	w0, w1, w0
	str	w0, [sp, 8]
.L127:
	ldr	w1, [sp, 8]
	mov	w0, 819
	mul	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	cmp	w1, w0
	bge	.L128
	ldr	w1, [sp, 12]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w1, w0, w1
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	str	w0, [sp, 8]
.L128:
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	junk_fun28, .-junk_fun28
	.align	2
	.type	junk_fun29, %function
junk_fun29:
	sub	sp, sp, #16
	mov	w0, 14
	str	w0, [sp, 12]
	mov	w0, 15
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	add	w0, w0, 1
	str	w0, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w0, 51
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	cmp	w0, 15
	bne	.L131
	ldr	w0, [sp, 8]
	b	.L132
.L131:
	ldr	w0, [sp, 12]
.L132:
	add	sp, sp, 16
	ret
	.size	junk_fun29, .-junk_fun29
	.align	2
	.type	junk_fun30, %function
junk_fun30:
	sub	sp, sp, #16
	mov	w0, 16
	str	w0, [sp, 12]
	mov	w0, 16
	str	w0, [sp, 8]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w1, [sp, 8]
	ldr	w0, [sp, 12]
	eor	w0, w1, w0
	str	w0, [sp, 12]
	ldr	w0, [sp, 12]
	cmp	w0, 7
	bne	.L134
	ldr	w0, [sp, 8]
	b	.L135
.L134:
	ldr	w0, [sp, 12]
	cmp	w0, 819
	bne	.L136
	ldr	w0, [sp, 8]
	b	.L135
.L136:
	ldr	w0, [sp, 12]
.L135:
	add	sp, sp, 16
	ret
	.size	junk_fun30, .-junk_fun30
	.align	2
	.type	fun_a10, %function
fun_a10:
	sub	sp, sp, #16
	mov	w0, 512
	str	w0, [sp, 12]
	mov	w0, 50
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	add	w0, w0, 100
	str	w0, [sp, 8]
	ldr	w0, [sp, 8]
	sub	w0, w0, #100
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	add	sp, sp, 16
	ret
	.size	fun_a10, .-fun_a10
	.align	2
	.type	fun_a11, %function
fun_a11:
	sub	sp, sp, #16
	mov	w0, 256
	str	w0, [sp, 12]
	mov	w0, 66
	str	w0, [sp, 8]
	ldr	w1, [sp, 12]
	ldr	w0, [sp, 8]
	add	w0, w1, w0
	add	sp, sp, 16
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
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	w0, [sp, 44]
	str	x1, [sp, 32]
	str	x2, [sp, 24]
	bl	junk_fun1
	ldr	x0, [sp, 32]
	add	x0, x0, 4
	mov	x1, x0
	ldr	w0, [sp, 44]
	bl	writel
	bl	junk_fun2
	nop
	ldp	x29, x30, [sp], 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_write_cmdreg, .-awlink_asm_write_cmdreg
	.align	2
	.global	awlink_asm_fun0
	.type	awlink_asm_fun0, %function
awlink_asm_fun0:
	hint	25 // paciasp
	sub	sp, sp, #48
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	ldr	x4, [sp, 32]
	mov	x3, 0
	ldr	x2, [sp, 40]
	mov	w1, 4294901767
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	nop
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun0, .-awlink_asm_fun0
	.align	2
	.global	awlink_asm_fun1
	.type	awlink_asm_fun1, %function
awlink_asm_fun1:
	hint	25 // paciasp
	sub	sp, sp, #48
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	mov	x3, 0
	ldr	x2, [sp, 40]
	mov	w1, 4294901763
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	nop
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun1, .-awlink_asm_fun1
	.align	2
	.global	awlink_asm_fun2
	.type	awlink_asm_fun2, %function
awlink_asm_fun2:
	hint	25 // paciasp
	sub	sp, sp, #48
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	mov	x3, 0
	ldr	x2, [sp, 40]
	mov	w1, 4294901764
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	nop
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun2, .-awlink_asm_fun2
	.align	2
	.global	awlink_asm_fun3
	.type	awlink_asm_fun3, %function
awlink_asm_fun3:
	hint	25 // paciasp
	sub	sp, sp, #48
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	mov	x3, 0
	mov	x2, 0
	mov	w1, 4294901765
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	nop
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun3, .-awlink_asm_fun3
	.align	2
	.global	awlink_asm_fun4
	.type	awlink_asm_fun4, %function
awlink_asm_fun4:
	hint	25 // paciasp
	sub	sp, sp, #48
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	mov	x3, 0
	mov	x2, 0
	mov	w1, 4294901766
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	nop
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun4, .-awlink_asm_fun4
	.align	2
	.global	awlink_asm_fun5
	.type	awlink_asm_fun5, %function
awlink_asm_fun5:
	hint	25 // paciasp
	sub	sp, sp, #48
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	mov	x3, 0
	ldr	x2, [sp, 40]
	mov	w1, 4294901768
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	adrp	x0, res
	add	x0, x0, :lo12:res
	ldr	x2, [x0]
	adrp	x0, res
	add	x0, x0, :lo12:res
	ldr	x0, [x0, 8]
	mov	x1, x0
	mov	x0, x2
	bl	ioremap
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_fun5, .-awlink_asm_fun5
	.align	2
	.global	awlink_asm_start
	.type	awlink_asm_start, %function
awlink_asm_start:
	hint	25 // paciasp
	sub	sp, sp, #80
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 56]
	str	x1, [sp, 48]
	str	x2, [sp, 40]
	mov	w0, 6
	str	w0, [sp, 76]
	bl	junk_fun4
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	ldr	x3, [sp, 48]
	ldr	x2, [sp, 40]
	mov	w1, 4294901761
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	adrp	x0, res
	add	x0, x0, :lo12:res
	ldr	x0, [x0]
	cmp	x0, 0
	bne	.L150
	bl	junk_fun5
	ldr	w0, [sp, 76]
	lsl	w0, w0, 12
	str	w0, [sp, 76]
	bl	junk_fun6
	b	.L155
.L150:
	ldr	w0, [sp, 76]
	lsl	w0, w0, 6
	str	w0, [sp, 76]
	ldr	w0, [sp, 76]
	and	w0, w0, 65535
	str	w0, [sp, 76]
	bl	fun_a10
	mov	w1, w0
	ldr	w0, [sp, 76]
	cmp	w0, w1
	bcs	.L152
	ldr	x0, [sp, 56]
	add	x0, x0, 16
	mov	x1, x0
	mov	w0, 48
	bl	writel
	b	.L154
.L152:
	bl	fun_a11
	mov	w1, w0
	ldr	w0, [sp, 76]
	cmp	w0, w1
	bls	.L153
	ldr	x0, [sp, 56]
	add	x0, x0, 16
	mov	x1, x0
	mov	w0, 48
	bl	writel
.L154:
	nop
.L153:
	bl	junk_fun7
.L155:
	nop
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 80
	hint	29 // autiasp
	ret
	.size	awlink_asm_start, .-awlink_asm_start
	.align	2
	.global	awlink_asm_set_bittiming
	.type	awlink_asm_set_bittiming, %function
awlink_asm_set_bittiming:
	hint	25 // paciasp
	stp	x29, x30, [sp, -48]!
	mov	x29, sp
	str	x0, [sp, 40]
	str	w1, [sp, 36]
	str	x2, [sp, 24]
	str	x3, [sp, 16]
	bl	junk_fun9
	ldr	w0, [sp, 36]
	and	w0, w0, 4
	cmp	w0, 0
	beq	.L157
	ldr	x0, [sp, 24]
	ldr	w0, [x0]
	orr	w1, w0, 8388608
	ldr	x0, [sp, 24]
	str	w1, [x0]
.L157:
	bl	junk_fun10
	ldr	x0, [sp, 24]
	ldr	w2, [x0]
	ldr	x0, [sp, 40]
	add	x0, x0, 20
	mov	x1, x0
	mov	w0, w2
	bl	writel
	nop
	ldp	x29, x30, [sp], 48
	hint	29 // autiasp
	ret
	.size	awlink_asm_set_bittiming, .-awlink_asm_set_bittiming
	.align	2
	.global	awlink_asm_clean_transfer_err
	.type	awlink_asm_clean_transfer_err, %function
awlink_asm_clean_transfer_err:
	hint	25 // paciasp
	stp	x29, x30, [sp, -64]!
	mov	x29, sp
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	x2, [sp, 24]
	bl	junk_fun12
	ldr	x0, [sp, 40]
	add	x0, x0, 28
	bl	readl
	str	w0, [sp, 60]
	bl	junk_fun13
	ldr	w0, [sp, 60]
	and	w0, w0, 65535
	and	w0, w0, 255
	and	w1, w0, 65535
	ldr	x0, [sp, 32]
	strh	w1, [x0]
	ldr	w0, [sp, 60]
	lsr	w0, w0, 16
	and	w0, w0, 65535
	and	w0, w0, 255
	and	w1, w0, 65535
	ldr	x0, [sp, 24]
	strh	w1, [x0]
	nop
	ldp	x29, x30, [sp], 64
	hint	29 // autiasp
	ret
	.size	awlink_asm_clean_transfer_err, .-awlink_asm_clean_transfer_err
	.align	2
	.global	awlink_asm_start_xmit
	.type	awlink_asm_start_xmit, %function
awlink_asm_start_xmit:
	hint	25 // paciasp
	stp	x29, x30, [sp, -80]!
	mov	x29, sp
	str	x0, [sp, 56]
	str	x1, [sp, 48]
	str	w2, [sp, 44]
	str	x3, [sp, 32]
	str	x4, [sp, 24]
	str	x5, [sp, 16]
	str	wzr, [sp, 76]
	ldr	x0, [sp, 32]
	ldr	w0, [x0]
	str	w0, [sp, 72]
	ldr	w0, [sp, 44]
	and	w0, w0, 1073741824
	cmp	w0, 0
	beq	.L160
	ldr	x0, [sp, 32]
	ldr	w0, [x0]
	orr	w1, w0, 64
	ldr	x0, [sp, 32]
	str	w1, [x0]
.L160:
	ldr	w0, [sp, 44]
	cmp	w0, 0
	bge	.L161
	ldr	x0, [sp, 32]
	ldr	w0, [x0]
	orr	w1, w0, 128
	ldr	x0, [sp, 32]
	str	w1, [x0]
	ldr	x0, [sp, 24]
	mov	w1, 84
	str	w1, [x0]
	ldr	w0, [sp, 44]
	lsr	w0, w0, 21
	and	w2, w0, 255
	ldr	x0, [sp, 56]
	add	x0, x0, 68
	mov	x1, x0
	mov	w0, w2
	bl	writel
	bl	junk_fun14
	ldr	w0, [sp, 44]
	lsr	w0, w0, 13
	and	w2, w0, 255
	ldr	x0, [sp, 56]
	add	x0, x0, 72
	mov	x1, x0
	mov	w0, w2
	bl	writel
	ldr	w0, [sp, 44]
	lsr	w0, w0, 5
	and	w2, w0, 255
	ldr	x0, [sp, 56]
	add	x0, x0, 76
	mov	x1, x0
	mov	w0, w2
	bl	writel
	ldr	w0, [sp, 44]
	lsl	w0, w0, 3
	and	w2, w0, 255
	ldr	x0, [sp, 56]
	add	x0, x0, 80
	mov	x1, x0
	mov	w0, w2
	bl	writel
	b	.L162
.L161:
	ldr	x0, [sp, 24]
	mov	w1, 76
	str	w1, [x0]
	ldr	w0, [sp, 44]
	lsr	w0, w0, 3
	and	w2, w0, 255
	ldr	x0, [sp, 56]
	add	x0, x0, 68
	mov	x1, x0
	mov	w0, w2
	bl	writel
	ldr	w0, [sp, 44]
	lsl	w0, w0, 5
	and	w2, w0, 255
	ldr	x0, [sp, 56]
	add	x0, x0, 72
	mov	x1, x0
	mov	w0, w2
	bl	writel
.L162:
	str	wzr, [sp, 76]
	b	.L163
.L164:
	ldrsw	x0, [sp, 76]
	ldr	x1, [sp, 16]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [sp, 24]
	ldr	w0, [x0]
	ldr	w1, [sp, 76]
	lsl	w1, w1, 2
	add	w0, w0, w1
	uxtw	x0, w0
	ldr	x1, [sp, 56]
	add	x0, x1, x0
	mov	x1, x0
	mov	w0, w2
	bl	writel
	ldr	w0, [sp, 76]
	add	w0, w0, 1
	str	w0, [sp, 76]
.L163:
	ldr	w0, [sp, 76]
	ldr	w1, [sp, 72]
	cmp	w1, w0
	bhi	.L164
	ldr	x0, [sp, 32]
	ldr	w2, [x0]
	ldr	x0, [sp, 56]
	add	x0, x0, 64
	mov	x1, x0
	mov	w0, w2
	bl	writel
	nop
	ldp	x29, x30, [sp], 80
	hint	29 // autiasp
	ret
	.size	awlink_asm_start_xmit, .-awlink_asm_start_xmit
	.align	2
	.global	awlink_asm_rx
	.type	awlink_asm_rx, %function
awlink_asm_rx:
	hint	25 // paciasp
	stp	x29, x30, [sp, -80]!
	mov	x29, sp
	str	x19, [sp, 16]
	str	x0, [sp, 56]
	str	x1, [sp, 48]
	str	x2, [sp, 40]
	str	x3, [sp, 32]
	strb	wzr, [sp, 79]
	ldr	x0, [sp, 56]
	add	x0, x0, 64
	bl	readl
	strb	w0, [sp, 79]
	ldrsb	w0, [sp, 79]
	cmp	w0, 0
	bge	.L166
	ldr	x0, [sp, 40]
	mov	w1, 84
	str	w1, [x0]
	bl	junk_fun17
	ldr	x0, [sp, 56]
	add	x0, x0, 68
	bl	readl
	lsl	w19, w0, 21
	ldr	x0, [sp, 56]
	add	x0, x0, 72
	bl	readl
	lsl	w0, w0, 13
	orr	w19, w19, w0
	ldr	x0, [sp, 56]
	add	x0, x0, 76
	bl	readl
	lsl	w0, w0, 5
	orr	w19, w19, w0
	ldr	x0, [sp, 56]
	add	x0, x0, 80
	bl	readl
	lsr	w0, w0, 3
	and	w0, w0, 31
	orr	w1, w19, w0
	ldr	x0, [sp, 32]
	str	w1, [x0]
	ldr	x0, [sp, 32]
	ldr	w0, [x0]
	orr	w1, w0, -2147483648
	ldr	x0, [sp, 32]
	str	w1, [x0]
	b	.L167
.L166:
	ldr	x0, [sp, 40]
	mov	w1, 76
	str	w1, [x0]
	ldr	x0, [sp, 56]
	add	x0, x0, 68
	bl	readl
	lsl	w19, w0, 3
	ldr	x0, [sp, 56]
	add	x0, x0, 72
	bl	readl
	lsr	w0, w0, 5
	and	w0, w0, 7
	orr	w1, w19, w0
	ldr	x0, [sp, 32]
	str	w1, [x0]
.L167:
	ldr	x0, [sp, 48]
	ldrb	w1, [sp, 79]
	strb	w1, [x0]
	nop
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 80
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
	sub	sp, sp, #160
	stp	x29, x30, [sp, 16]
	add	x29, sp, 16
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	mov	w0, -1
	str	w0, [sp, 156]
	mov	w0, 35
	str	w0, [sp, 152]
	mov	w0, 1
	str	w0, [sp, 148]
	adrp	x0, res
	add	x0, x0, :lo12:res
	mov	x1, -1
	str	x1, [x0]
	str	xzr, [sp, 8]
	adrp	x0, res
	add	x0, x0, :lo12:res
	str	x0, [sp]
	mov	x7, 0
	mov	x6, 0
	mov	x5, 0
	mov	x4, 0
	mov	x3, 0
	ldr	x2, [sp, 32]
	mov	w1, 4294901762
	mov	x0, 160
	movk	x0, 0xc000, lsl 16
	bl	__arm_smccc_smc
	adrp	x0, res
	add	x0, x0, :lo12:res
	ldr	x0, [x0]
	cmp	x0, 0
	bne	.L169
	mov	x2, 0
	adrp	x0, .LC8
	add	x1, x0, :lo12:.LC8
	ldr	x0, [sp, 40]
	bl	of_get_property
	str	x0, [sp, 136]
	ldr	x0, [sp, 40]
	bl	of_irq_find_parent
	str	x0, [sp, 128]
	ldr	x0, [sp, 128]
	cmp	x0, 0
	bne	.L170
	mov	w0, -1
	b	.L172
.L170:
	ldr	x0, [sp, 128]
	str	x0, [sp, 48]
	mov	w0, 3
	str	w0, [sp, 56]
	str	wzr, [sp, 60]
	ldr	w1, [sp, 152]
	ldr	w0, [sp, 148]
	add	w0, w1, w0
	str	w0, [sp, 152]
	bl	junk_fun3
	ldr	x1, [sp, 32]
	mov	x0, x1
	lsl	x0, x0, 6
	add	x0, x0, x1
	lsl	x0, x0, 1
	str	x0, [sp, 32]
	ldr	x0, [sp, 32]
	mov	w1, w0
	ldr	w0, [sp, 152]
	add	w0, w1, w0
	str	w0, [sp, 152]
	ldr	w2, [sp, 152]
	ldrsw	x0, [sp, 148]
	lsl	x0, x0, 2
	add	x1, sp, 60
	str	w2, [x1, x0]
	bl	junk_fun15
	mov	w0, 4
	str	w0, [sp, 68]
	add	x0, sp, 48
	mov	x1, x0
	ldr	x0, [sp, 136]
	bl	of_irq_parse_raw
	bl	junk_fun16
	add	x0, sp, 48
	bl	irq_create_of_mapping
	str	w0, [sp, 156]
.L169:
	ldr	w0, [sp, 156]
.L172:
	ldp	x29, x30, [sp, 16]
	add	sp, sp, 160
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

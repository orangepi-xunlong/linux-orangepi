/*SPDX-Licensen-Identifier: GPL-2.0*/

	.arch armv8-a
	.file	"nftl.c"
	.global	com
	.section	.rodata
	.align	3
	.type	com, %object
	.size	com, 11
com:
	.byte	99
	.byte	111
	.byte	109
	.byte	112
	.byte	97
	.byte	116
	.byte	105
	.byte	98
	.byte	108
	.byte	101
	.byte	0
	.global	mod
	.align	3
	.type	mod, %object
	.size	mod, 6
mod:
	.byte	109
	.byte	111
	.byte	100
	.byte	101
	.byte	108
	.byte	0
	.text
	.align	2
	.global	nftl_get_boot_cnt
	.type	nftl_get_boot_cnt, %function
nftl_get_boot_cnt:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 140]
	add	sp, sp, 32
	ret
	.size	nftl_get_boot_cnt, .-nftl_get_boot_cnt
	.align	2
	.global	nftl_get_zone_cap
	.type	nftl_get_zone_cap, %function
nftl_get_zone_cap:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	w0, [x0, 32]
	add	sp, sp, 32
	ret
	.size	nftl_get_zone_cap, .-nftl_get_zone_cap
	.align	2
	.global	nftl_set_zone_test
	.type	nftl_set_zone_test, %function
nftl_set_zone_test:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	w1, [x29, 20]
	str	w1, [x0, 656]
	ldr	x0, [x29, 40]
	bl	print_nftl_zone
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_set_zone_test, .-nftl_set_zone_test
	.align	2
	.global	nftl_get_zone_priv
	.type	nftl_get_zone_priv, %function
nftl_get_zone_priv:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	add	sp, sp, 32
	ret
	.size	nftl_get_zone_priv, .-nftl_get_zone_priv
	.align	2
	.global	nftl_get_zone_write_cache_nums
	.type	nftl_get_zone_write_cache_nums, %function
nftl_get_zone_write_cache_nums:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 1076]
	add	sp, sp, 32
	ret
	.size	nftl_get_zone_write_cache_nums, .-nftl_get_zone_write_cache_nums
	.section	.rodata
	.align	3
.LC3:
	.string	"[ND]smart->version:%x;\n"
	.align	3
.LC4:
	.string	"[ND]smart->total_recv_read_sectors:%lld;\n"
	.align	3
.LC5:
	.string	"[ND]smart->total_recv_write_sectors:%lld;\n"
	.align	3
.LC6:
	.string	"[ND]smart->total_real_read_pages:%lld;\n"
	.align	3
.LC7:
	.string	"[ND]smart->total_real_write_pages:%lld;\n"
	.align	3
.LC8:
	.string	"[ND]smart->total_recv_discard_sectors:%lld;\n"
	.align	3
.LC9:
	.string	"[ND]smart->total_real_discard_sectors:%lld;\n"
	.align	3
.LC10:
	.string	"[ND]smart->total_recv_read_claim_pages:%d;\n"
	.align	3
.LC11:
	.string	"[ND]smart->total_gc_times:%d;\n"
	.align	3
.LC12:
	.string	"[ND]smart->total_gc_pages:%d;\n"
	.align	3
.LC13:
	.string	"[ND]smart->total_wl_times:%d;\n"
	.align	3
.LC14:
	.string	"[ND]smart->total_wl_gc_times:%d;\n"
	.align	3
.LC15:
	.string	"[ND]smart->total_real_read_error_pages:%d;\n"
	.align	3
.LC16:
	.string	"[ND]smart->total_real_write_error_pages:%d;\n"
	.align	3
.LC17:
	.string	"[ND]smart->total_normal_power_cycles:%x;\n"
	.align	3
.LC18:
	.string	"[ND]smart->total_unusual_power_cycles:%x;\n"
	.align	3
.LC19:
	.string	"[ND]smart->max_block_erase_times:%d;\n"
	.align	3
.LC20:
	.string	"[ND]smart->min_block_erase_times:%d;\n"
	.align	3
.LC21:
	.string	"[ND]smart->max_block_used_counter:%d;\n"
	.align	3
.LC22:
	.string	"[ND]smart->min_block_used_counter:%d;\n"
	.align	3
.LC23:
	.string	"[ND]smart->read_reclaim_utc:%d;\n"
	.align	3
.LC24:
	.string	"[ND]smart->cross_talk_times:%d;\n"
	.align	3
.LC25:
	.string	"[ND]smart->read_reclaim_zone_no:%d;\n"
	.align	3
.LC26:
	.string	"[ND]smart->read_reclaim_page_no:%d;\n"
	.text
	.align	2
	.global	print_smart
	.type	print_smart, %function
print_smart:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 656]
	cmp	w0, 0
	beq	.L14
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 112]
	adrp	x0, .LC3
	add	x0, x0, :lo12:.LC3
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 64]
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 72]
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 80]
	adrp	x0, .LC6
	add	x0, x0, :lo12:.LC6
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 88]
	adrp	x0, .LC7
	add	x0, x0, :lo12:.LC7
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 96]
	adrp	x0, .LC8
	add	x0, x0, :lo12:.LC8
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 104]
	adrp	x0, .LC9
	add	x0, x0, :lo12:.LC9
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 116]
	adrp	x0, .LC10
	add	x0, x0, :lo12:.LC10
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 120]
	adrp	x0, .LC11
	add	x0, x0, :lo12:.LC11
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 124]
	adrp	x0, .LC12
	add	x0, x0, :lo12:.LC12
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 128]
	adrp	x0, .LC13
	add	x0, x0, :lo12:.LC13
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 180]
	adrp	x0, .LC14
	add	x0, x0, :lo12:.LC14
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 132]
	adrp	x0, .LC15
	add	x0, x0, :lo12:.LC15
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 136]
	adrp	x0, .LC16
	add	x0, x0, :lo12:.LC16
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 140]
	adrp	x0, .LC17
	add	x0, x0, :lo12:.LC17
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 144]
	adrp	x0, .LC18
	add	x0, x0, :lo12:.LC18
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 148]
	adrp	x0, .LC19
	add	x0, x0, :lo12:.LC19
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 152]
	adrp	x0, .LC20
	add	x0, x0, :lo12:.LC20
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 156]
	adrp	x0, .LC21
	add	x0, x0, :lo12:.LC21
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 160]
	adrp	x0, .LC22
	add	x0, x0, :lo12:.LC22
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 164]
	adrp	x0, .LC23
	add	x0, x0, :lo12:.LC23
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 168]
	adrp	x0, .LC24
	add	x0, x0, :lo12:.LC24
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 172]
	adrp	x0, .LC25
	add	x0, x0, :lo12:.LC25
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 176]
	adrp	x0, .LC26
	add	x0, x0, :lo12:.LC26
	bl	nand_print_dbg
	b	.L11
.L14:
	nop
.L11:
	ldp	x29, x30, [sp], 48
	ret
	.size	print_smart, .-print_smart
	.align	2
	.global	_nftl_nand_copy_page
	.type	_nftl_nand_copy_page, %function
_nftl_nand_copy_page:
	stp	x29, x30, [sp, -144]!
	add	x29, sp, 0
	str	x0, [x29, 56]
	str	x1, [x29, 48]
	str	x2, [x29, 40]
	str	x3, [x29, 32]
	strh	w4, [x29, 30]
	ldr	x0, [x29, 48]
	ldrh	w1, [x0]
	add	x0, x29, 88
	mov	x5, 0
	mov	x4, 0
	mov	w3, 0
	ldrh	w2, [x29, 30]
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldrh	w1, [x0]
	add	x0, x29, 64
	mov	x5, 0
	mov	x4, 0
	mov	w3, 0
	ldrh	w2, [x29, 30]
	bl	set_physic_op_par
	str	wzr, [x29, 136]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 28]
	strh	w0, [x29, 92]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 28]
	strh	w0, [x29, 68]
	ldr	x0, [x29, 32]
	str	x0, [x29, 96]
	ldr	x0, [x29, 32]
	str	x0, [x29, 72]
	add	x0, x29, 112
	str	x0, [x29, 104]
	add	x0, x29, 112
	str	x0, [x29, 80]
	ldr	x0, [x29, 56]
	ldr	x2, [x0, 3472]
	add	x0, x29, 88
	mov	x1, x0
	ldr	x0, [x29, 56]
	blr	x2
	str	w0, [x29, 140]
	ldr	w0, [x29, 140]
	cmp	w0, 10
	bne	.L16
	str	wzr, [x29, 140]
.L16:
	add	x0, x29, 112
	bl	get_special_data_from_oob
	str	w0, [x29, 132]
	ldr	x0, [x29, 48]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	add	x0, x29, 112
	mov	w4, w1
	mov	w3, w2
	ldr	w2, [x29, 132]
	mov	x1, x0
	ldr	x0, [x29, 56]
	bl	set_oob_special_page
	ldr	x0, [x29, 56]
	ldr	x2, [x0, 3480]
	add	x0, x29, 64
	mov	x1, x0
	ldr	x0, [x29, 56]
	blr	x2
	str	w0, [x29, 128]
	ldr	w0, [x29, 128]
	lsl	w1, w0, 8
	ldr	w0, [x29, 140]
	orr	w0, w1, w0
	str	w0, [x29, 136]
	ldr	w0, [x29, 136]
	ldp	x29, x30, [sp], 144
	ret
	.size	_nftl_nand_copy_page, .-_nftl_nand_copy_page
	.align	2
	.global	set_spare_data
	.type	set_spare_data, %function
set_spare_data:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	strb	w2, [sp, 3]
	strb	w3, [sp, 2]
	str	wzr, [sp, 28]
	b	.L19
.L20:
	ldrb	w1, [sp, 2]
	ldr	w0, [sp, 28]
	sub	w0, w1, w0
	str	w0, [sp, 24]
	ldr	w0, [sp, 24]
	sub	w0, w0, #1
	str	w0, [sp, 24]
	ldr	w0, [sp, 24]
	lsl	w0, w0, 3
	str	w0, [sp, 24]
	ldrb	w1, [sp, 3]
	ldr	w0, [sp, 28]
	add	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	ldr	w1, [sp, 24]
	ldr	w2, [sp, 4]
	lsr	w1, w2, w1
	uxtb	w1, w1
	strb	w1, [x0]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L19:
	ldrb	w1, [sp, 2]
	ldr	w0, [sp, 28]
	cmp	w1, w0
	bhi	.L20
	nop
	add	sp, sp, 32
	ret
	.size	set_spare_data, .-set_spare_data
	.align	2
	.global	get_spare_data
	.type	get_spare_data, %function
get_spare_data:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	strb	w1, [sp, 7]
	strb	w2, [sp, 6]
	str	wzr, [sp, 24]
	str	wzr, [sp, 28]
	b	.L22
.L23:
	ldr	w0, [sp, 28]
	lsl	w0, w0, 8
	str	w0, [sp, 28]
	ldrb	w1, [sp, 7]
	ldr	w0, [sp, 24]
	add	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [sp, 28]
	orr	w0, w0, w1
	str	w0, [sp, 28]
	ldr	w0, [sp, 24]
	add	w0, w0, 1
	str	w0, [sp, 24]
.L22:
	ldrb	w1, [sp, 6]
	ldr	w0, [sp, 24]
	cmp	w1, w0
	bhi	.L23
	ldr	w0, [sp, 28]
	add	sp, sp, 32
	ret
	.size	get_spare_data, .-get_spare_data
	.align	2
	.global	set_oob_logic_page
	.type	set_oob_logic_page, %function
set_oob_logic_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	str	w3, [x29, 24]
	strh	w4, [x29, 22]
	ldr	x0, [x29, 32]
	mov	w1, -1
	strb	w1, [x0]
	mov	w3, 4
	mov	w2, 1
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 32]
	bl	set_spare_data
	ldrh	w0, [x29, 22]
	mov	w3, 2
	mov	w2, 5
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	set_spare_data
	mov	w3, 4
	mov	w2, 7
	ldr	w1, [x29, 24]
	ldr	x0, [x29, 32]
	bl	set_spare_data
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldr	x1, [x29, 32]
	add	x1, x1, 1
	ldrb	w1, [x1]
	orr	w1, w1, -64
	uxtb	w1, w1
	strb	w1, [x0]
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	set_oob_logic_page, .-set_oob_logic_page
	.align	2
	.global	set_oob_special_page
	.type	set_oob_special_page, %function
set_oob_special_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	str	w3, [x29, 24]
	strh	w4, [x29, 22]
	ldr	x0, [x29, 32]
	mov	w1, -1
	strb	w1, [x0]
	mov	w3, 4
	mov	w2, 1
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 32]
	bl	set_spare_data
	ldrh	w0, [x29, 22]
	mov	w3, 2
	mov	w2, 5
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	set_spare_data
	mov	w3, 4
	mov	w2, 7
	ldr	w1, [x29, 24]
	ldr	x0, [x29, 32]
	bl	set_spare_data
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	set_oob_special_page, .-set_oob_special_page
	.align	2
	.global	set_no_use_page
	.type	set_no_use_page, %function
set_no_use_page:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	mov	w1, -1
	strb	w1, [x0]
	mov	w3, 1
	mov	w2, 1
	mov	w1, 170
	ldr	x0, [x29, 16]
	bl	set_spare_data
	mov	w3, 1
	mov	w2, 2
	mov	w1, 187
	ldr	x0, [x29, 16]
	bl	set_spare_data
	nop
	ldp	x29, x30, [sp], 32
	ret
	.size	set_no_use_page, .-set_no_use_page
	.align	2
	.global	set_start_block_flag
	.type	set_start_block_flag, %function
set_start_block_flag:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	ldr	x0, [x29, 32]
	mov	w1, -1
	strb	w1, [x0]
	mov	w3, 1
	mov	w2, 1
	mov	w1, 170
	ldr	x0, [x29, 32]
	bl	set_spare_data
	mov	w3, 1
	mov	w2, 2
	mov	w1, 238
	ldr	x0, [x29, 32]
	bl	set_spare_data
	mov	w3, 4
	mov	w2, 3
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 32]
	bl	set_spare_data
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	set_start_block_flag, .-set_start_block_flag
	.align	2
	.global	set_end_block_flag
	.type	set_end_block_flag, %function
set_end_block_flag:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	mov	w1, -1
	strb	w1, [x0]
	mov	w3, 1
	mov	w2, 1
	mov	w1, 170
	ldr	x0, [x29, 16]
	bl	set_spare_data
	mov	w3, 1
	mov	w2, 2
	mov	w1, 204
	ldr	x0, [x29, 16]
	bl	set_spare_data
	nop
	ldp	x29, x30, [sp], 32
	ret
	.size	set_end_block_flag, .-set_end_block_flag
	.align	2
	.global	get_bad_flag_from_oob
	.type	get_bad_flag_from_oob, %function
get_bad_flag_from_oob:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w2, 4
	mov	w1, 1
	ldr	x0, [x29, 24]
	bl	get_spare_data
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	bne	.L31
	mov	w0, 0
	b	.L32
.L31:
	mov	w0, 1
.L32:
	ldp	x29, x30, [sp], 48
	ret
	.size	get_bad_flag_from_oob, .-get_bad_flag_from_oob
	.align	2
	.global	get_block_used_count_from_oob
	.type	get_block_used_count_from_oob, %function
get_block_used_count_from_oob:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w2, 4
	mov	w1, 7
	ldr	x0, [x29, 24]
	bl	get_spare_data
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	get_block_used_count_from_oob, .-get_block_used_count_from_oob
	.align	2
	.global	get_logic_page_from_oob
	.type	get_logic_page_from_oob, %function
get_logic_page_from_oob:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w2, 4
	mov	w1, 1
	ldr	x0, [x29, 24]
	bl	get_spare_data
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	and	w0, w0, 268435455
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	get_logic_page_from_oob, .-get_logic_page_from_oob
	.align	2
	.global	get_special_data_from_oob
	.type	get_special_data_from_oob, %function
get_special_data_from_oob:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w2, 4
	mov	w1, 1
	ldr	x0, [x29, 24]
	bl	get_spare_data
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	get_special_data_from_oob, .-get_special_data_from_oob
	.align	2
	.global	get_erase_count_from_oob
	.type	get_erase_count_from_oob, %function
get_erase_count_from_oob:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w2, 2
	mov	w1, 5
	ldr	x0, [x29, 24]
	bl	get_spare_data
	strh	w0, [x29, 46]
	ldrh	w0, [x29, 46]
	ldp	x29, x30, [sp], 48
	ret
	.size	get_erase_count_from_oob, .-get_erase_count_from_oob
	.align	2
	.global	is_ftl_start_flag_page
	.type	is_ftl_start_flag_page, %function
is_ftl_start_flag_page:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 170
	bne	.L42
	ldr	x0, [sp, 8]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 238
	bne	.L42
	mov	w0, 1
	b	.L43
.L42:
	mov	w0, 0
.L43:
	add	sp, sp, 16
	ret
	.size	is_ftl_start_flag_page, .-is_ftl_start_flag_page
	.align	2
	.global	is_ftl_end_flag_page
	.type	is_ftl_end_flag_page, %function
is_ftl_end_flag_page:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 170
	bne	.L45
	ldr	x0, [sp, 8]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 204
	bne	.L45
	mov	w0, 1
	b	.L46
.L45:
	mov	w0, 0
.L46:
	add	sp, sp, 16
	ret
	.size	is_ftl_end_flag_page, .-is_ftl_end_flag_page
	.align	2
	.global	is_ftl_special_data
	.type	is_ftl_special_data, %function
is_ftl_special_data:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 170
	bne	.L48
	mov	w0, 1
	b	.L49
.L48:
	mov	w0, 0
.L49:
	add	sp, sp, 16
	ret
	.size	is_ftl_special_data, .-is_ftl_special_data
	.align	2
	.global	is_ftl_logic_page_data
	.type	is_ftl_logic_page_data, %function
is_ftl_logic_page_data:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1
	ldrb	w0, [x0]
	and	w0, w0, 240
	cmp	w0, 192
	bne	.L51
	mov	w0, 1
	b	.L52
.L51:
	mov	w0, 0
.L52:
	add	sp, sp, 16
	ret
	.size	is_ftl_logic_page_data, .-is_ftl_logic_page_data
	.align	2
	.global	is_nouse_page
	.type	is_nouse_page, %function
is_nouse_page:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	wzr, [sp, 24]
	str	wzr, [sp, 28]
	b	.L54
.L56:
	ldr	w0, [sp, 28]
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L55
	ldr	w0, [sp, 24]
	add	w0, w0, 1
	str	w0, [sp, 24]
.L55:
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L54:
	ldr	w0, [sp, 28]
	cmp	w0, 9
	bls	.L56
	ldr	w0, [sp, 24]
	cmp	w0, 8
	bls	.L57
	ldr	x0, [sp, 8]
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L57
	mov	w0, 1
	b	.L58
.L57:
	mov	w0, 0
.L58:
	add	sp, sp, 32
	ret
	.size	is_nouse_page, .-is_nouse_page
	.align	2
	.global	is_fill_page
	.type	is_fill_page, %function
is_fill_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	get_special_data_from_oob
	str	w0, [x29, 44]
	ldr	w1, [x29, 44]
	mov	w0, 1431655765
	cmp	w1, w0
	bne	.L60
	mov	w0, 1
	b	.L61
.L60:
	mov	w0, 0
.L61:
	ldp	x29, x30, [sp], 48
	ret
	.size	is_fill_page, .-is_fill_page
	.align	2
	.global	is_phy_mapping_page
	.type	is_phy_mapping_page, %function
is_phy_mapping_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	get_special_data_from_oob
	str	w0, [x29, 44]
	ldr	w1, [x29, 44]
	mov	w0, -1431633921
	cmp	w1, w0
	bne	.L63
	mov	w0, 1
	b	.L64
.L63:
	mov	w0, 0
.L64:
	ldp	x29, x30, [sp], 48
	ret
	.size	is_phy_mapping_page, .-is_phy_mapping_page
	.align	2
	.global	is_function_info_page
	.type	is_function_info_page, %function
is_function_info_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	get_special_data_from_oob
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	and	w1, w0, -65536
	mov	w0, -1441726464
	cmp	w1, w0
	bne	.L66
	mov	w0, 1
	b	.L67
.L66:
	mov	w0, 0
.L67:
	ldp	x29, x30, [sp], 48
	ret
	.size	is_function_info_page, .-is_function_info_page
	.align	2
	.global	is_power_down_info_page
	.type	is_power_down_info_page, %function
is_power_down_info_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	get_special_data_from_oob
	str	w0, [x29, 44]
	ldr	w1, [x29, 44]
	mov	w0, 13107
	movk	w0, 0xaa11, lsl 16
	cmp	w1, w0
	bne	.L69
	mov	w0, 1
	b	.L70
.L69:
	mov	w0, 0
.L70:
	ldp	x29, x30, [sp], 48
	ret
	.size	is_power_down_info_page, .-is_power_down_info_page
	.section	.rodata
	.align	3
.LC27:
	.string	"[NE]%s:logic_page:0x%x >= total_pages:0x%x\n"
	.text
	.align	2
	.global	get_logic_page_map
	.type	get_logic_page_map, %function
get_logic_page_map:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 36]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bhi	.L72
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 36]
	adrp	x0, __func__.5226
	add	x1, x0, :lo12:__func__.5226
	adrp	x0, .LC27
	add	x0, x0, :lo12:.LC27
	mov	w3, w2
	ldr	w2, [x29, 20]
	bl	nand_print
	str	xzr, [x29, 48]
	ldr	x0, [x29, 48]
	b	.L73
.L72:
	mov	w0, 262144
	str	w0, [x29, 44]
	str	wzr, [x29, 60]
	b	.L74
.L75:
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	sub	w0, w1, w0
	str	w0, [x29, 20]
.L74:
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bcs	.L75
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	str	x0, [x29, 32]
	ldr	w0, [x29, 20]
	lsl	x0, x0, 2
	ldr	x1, [x29, 32]
	add	x0, x1, x0
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
.L73:
	ldp	x29, x30, [sp], 64
	ret
	.size	get_logic_page_map, .-get_logic_page_map
	.section	.rodata
	.align	3
.LC28:
	.string	"[NE]%s:malloc fail for logic_page_map %d\n"
	.align	3
.LC29:
	.string	"[NE]%s: malloc fail for logic_page_map %d\n"
	.align	3
.LC30:
	.string	"[NE]zone_param_init malloc_logic_page_map error %d\n"
	.text
	.align	2
	.global	malloc_logic_page_map
	.type	malloc_logic_page_map, %function
malloc_logic_page_map:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	mov	w0, 262144
	str	w0, [x29, 40]
	str	wzr, [x29, 44]
	b	.L77
.L78:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	str	xzr, [x0, 8]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L77:
	ldr	w0, [x29, 44]
	cmp	w0, 24
	bls	.L78
	str	wzr, [x29, 44]
	b	.L79
.L86:
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 40]
	cmp	w1, w0
	bcc	.L80
	ldr	w0, [x29, 40]
	lsl	w0, w0, 2
	str	w0, [x29, 36]
	ldr	w0, [x29, 36]
	bl	nftl_malloc
	mov	x2, x0
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	str	x2, [x0, 8]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	cmp	x0, 0
	bne	.L81
	adrp	x0, .LC28
	add	x0, x0, :lo12:.LC28
	ldr	w1, [x29, 44]
	bl	nand_print
	mov	w0, -2
	b	.L82
.L81:
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 40]
	sub	w0, w1, w0
	str	w0, [x29, 20]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	ldr	w2, [x29, 36]
	mov	w1, -1
	bl	nand_memset
	b	.L88
.L80:
	ldr	w0, [x29, 20]
	lsl	w0, w0, 2
	str	w0, [x29, 36]
	ldr	w0, [x29, 36]
	bl	nftl_malloc
	mov	x2, x0
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	str	x2, [x0, 8]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	cmp	x0, 0
	bne	.L84
	adrp	x0, .LC29
	add	x0, x0, :lo12:.LC29
	ldr	w1, [x29, 44]
	bl	nand_print
	mov	w0, -2
	b	.L82
.L84:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	ldr	w2, [x29, 36]
	mov	w1, -1
	bl	nand_memset
	str	wzr, [x29, 20]
	b	.L85
.L88:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L79:
	ldr	w0, [x29, 44]
	cmp	w0, 24
	bls	.L86
.L85:
	ldr	w0, [x29, 20]
	cmp	w0, 0
	beq	.L87
	adrp	x0, .LC30
	add	x0, x0, :lo12:.LC30
	ldr	w1, [x29, 20]
	bl	nand_print
	mov	w0, -2
	b	.L82
.L87:
	mov	w0, 0
.L82:
	ldp	x29, x30, [sp], 48
	ret
	.size	malloc_logic_page_map, .-malloc_logic_page_map
	.align	2
	.global	free_logic_page_map
	.type	free_logic_page_map, %function
free_logic_page_map:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	b	.L90
.L92:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	cmp	x0, 0
	beq	.L91
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	bl	nftl_free
.L91:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L90:
	ldr	w0, [x29, 44]
	cmp	w0, 24
	bls	.L92
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	free_logic_page_map, .-free_logic_page_map
	.align	2
	.global	set_physic_op_par
	.type	set_physic_op_par, %function
set_physic_op_par:
	sub	sp, sp, #32
	str	x0, [sp, 24]
	strh	w1, [sp, 22]
	strh	w2, [sp, 20]
	strh	w3, [sp, 18]
	str	x4, [sp, 8]
	str	x5, [sp]
	ldr	x0, [sp, 24]
	ldrh	w1, [sp, 20]
	strh	w1, [x0]
	ldr	x0, [sp, 24]
	ldrh	w1, [sp, 22]
	strh	w1, [x0, 2]
	ldr	x0, [sp, 24]
	ldrh	w1, [sp, 18]
	strh	w1, [x0, 4]
	ldr	x0, [sp, 24]
	ldr	x1, [sp, 8]
	str	x1, [x0, 8]
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 16]
	nop
	add	sp, sp, 32
	ret
	.size	set_physic_op_par, .-set_physic_op_par
	.section	.rodata
	.align	3
.LC31:
	.string	"[NE]%s:malloc fail for cache_buf!\n"
	.text
	.align	2
	.global	nftl_sector_cache_init
	.type	nftl_sector_cache_init, %function
nftl_sector_cache_init:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 2880
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	mov	w1, 16
	strh	w1, [x0]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 8]
	mov	w0, 8192
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 8]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	cmp	x0, 0
	bne	.L96
	adrp	x0, __func__.5264
	add	x1, x0, :lo12:__func__.5264
	adrp	x0, .LC31
	add	x0, x0, :lo12:.LC31
	bl	nand_print
	mov	w0, 1
	b	.L97
.L96:
	ldr	x0, [x29, 40]
	str	xzr, [x0, 544]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 552]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	str	x0, [x29, 48]
	str	wzr, [x29, 60]
	b	.L98
.L99:
	ldr	w0, [x29, 60]
	uxth	w2, w0
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 16
	mov	w1, w2
	strh	w1, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 18
	mov	w1, 255
	strh	w1, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 20
	mov	w1, -1
	str	w1, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	add	x0, x0, 1
	lsl	x0, x0, 5
	add	x0, x1, x0
	str	xzr, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	add	x0, x0, 1
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 8
	str	xzr, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 24
	ldr	x1, [x29, 48]
	str	x1, [x0]
	ldr	x0, [x29, 48]
	add	x0, x0, 512
	str	x0, [x29, 48]
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L98:
	ldr	w0, [x29, 60]
	cmp	w0, 15
	bls	.L99
	mov	w0, 0
.L97:
	ldp	x29, x30, [sp], 64
	ret
	.size	nftl_sector_cache_init, .-nftl_sector_cache_init
	.align	2
	.global	nftl_sector_cache_exit
	.type	nftl_sector_cache_exit, %function
nftl_sector_cache_exit:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 2880
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	bl	nftl_free
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_sector_cache_exit, .-nftl_sector_cache_exit
	.align	2
	.global	search_from_sector_cache_write_list
	.type	search_from_sector_cache_write_list, %function
search_from_sector_cache_write_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	ldr	x0, [sp, 8]
	add	x0, x0, 528
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L103
.L106:
	ldr	x0, [sp, 24]
	ldr	w1, [x0, 4]
	ldr	w0, [sp, 4]
	cmp	w1, w0
	bne	.L104
	ldr	x0, [sp, 24]
	b	.L105
.L104:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L103:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L106
	mov	x0, 0
.L105:
	add	sp, sp, 32
	ret
	.size	search_from_sector_cache_write_list, .-search_from_sector_cache_write_list
	.align	2
	.global	get_empty_sector_cahce_node
	.type	get_empty_sector_cahce_node, %function
get_empty_sector_cahce_node:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	strh	wzr, [sp, 30]
	b	.L108
.L111:
	ldrh	w0, [sp, 30]
	ldr	x1, [sp, 8]
	sxtw	x0, w0
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 18
	ldrh	w0, [x0]
	cmp	w0, 255
	bne	.L109
	ldrh	w0, [sp, 30]
	sxtw	x0, w0
	lsl	x0, x0, 5
	add	x0, x0, 16
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	b	.L110
.L109:
	ldrh	w0, [sp, 30]
	add	w0, w0, 1
	strh	w0, [sp, 30]
.L108:
	ldrh	w0, [sp, 30]
	cmp	w0, 15
	bls	.L111
	mov	x0, 0
.L110:
	add	sp, sp, 32
	ret
	.size	get_empty_sector_cahce_node, .-get_empty_sector_cahce_node
	.align	2
	.global	add_to_sector_cache_write_list_tail
	.type	add_to_sector_cache_write_list_tail, %function
add_to_sector_cache_write_list_tail:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 528
	str	x0, [sp, 24]
	b	.L113
.L114:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L113:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L114
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 16]
	ldr	x0, [sp]
	str	xzr, [x0, 16]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 24]
	ldr	x0, [sp]
	mov	w1, 1
	strh	w1, [x0, 2]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0, 2]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0, 2]
	mov	w0, 0
	add	sp, sp, 32
	ret
	.size	add_to_sector_cache_write_list_tail, .-add_to_sector_cache_write_list_tail
	.align	2
	.global	del_from_sector_cache_write_list
	.type	del_from_sector_cache_write_list, %function
del_from_sector_cache_write_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp]
	ldr	x0, [x0, 24]
	str	x0, [sp, 24]
	ldr	x0, [sp]
	ldr	x1, [x0, 16]
	ldr	x0, [sp, 24]
	str	x1, [x0, 16]
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	beq	.L117
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	ldr	x1, [sp, 24]
	str	x1, [x0, 24]
.L117:
	ldr	x0, [sp]
	str	xzr, [x0, 16]
	ldr	x0, [sp]
	str	xzr, [x0, 24]
	ldr	x0, [sp]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0, 2]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0, 2]
	ldr	x0, [sp]
	add	sp, sp, 32
	ret
	.size	del_from_sector_cache_write_list, .-del_from_sector_cache_write_list
	.align	2
	.global	__nand_sector_read
	.type	__nand_sector_read, %function
__nand_sector_read:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	str	wzr, [x29, 76]
	ldr	w0, [x29, 36]
	str	w0, [x29, 68]
	ldr	x0, [x29, 24]
	str	x0, [x29, 56]
	str	wzr, [x29, 72]
	b	.L120
.L121:
	ldr	x2, [x29, 56]
	ldr	w1, [x29, 68]
	ldr	x0, [x29, 40]
	bl	nand_sector_cache_read
	mov	w1, w0
	ldr	w0, [x29, 76]
	orr	w0, w0, w1
	str	w0, [x29, 76]
	ldr	w0, [x29, 68]
	add	w0, w0, 1
	str	w0, [x29, 68]
	ldr	x0, [x29, 56]
	add	x0, x0, 512
	str	x0, [x29, 56]
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
.L120:
	ldr	w1, [x29, 72]
	ldr	w0, [x29, 32]
	cmp	w1, w0
	bcc	.L121
	ldr	w0, [x29, 76]
	ldp	x29, x30, [sp], 80
	ret
	.size	__nand_sector_read, .-__nand_sector_read
	.align	2
	.global	__nand_sector_write
	.type	__nand_sector_write, %function
__nand_sector_write:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	str	wzr, [x29, 76]
	ldr	w0, [x29, 36]
	str	w0, [x29, 68]
	ldr	x0, [x29, 24]
	str	x0, [x29, 56]
	str	wzr, [x29, 72]
	b	.L124
.L125:
	ldr	x2, [x29, 56]
	ldr	w1, [x29, 68]
	ldr	x0, [x29, 40]
	bl	nand_sector_cache_write
	mov	w1, w0
	ldr	w0, [x29, 76]
	orr	w0, w0, w1
	str	w0, [x29, 76]
	ldr	w0, [x29, 68]
	add	w0, w0, 1
	str	w0, [x29, 68]
	ldr	x0, [x29, 56]
	add	x0, x0, 512
	str	x0, [x29, 56]
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
.L124:
	ldr	w1, [x29, 72]
	ldr	w0, [x29, 32]
	cmp	w1, w0
	bcc	.L125
	ldr	w0, [x29, 76]
	ldp	x29, x30, [sp], 80
	ret
	.size	__nand_sector_write, .-__nand_sector_write
	.section	.rodata
	.align	3
.LC32:
	.string	"[NE]nand_sector_cache_read 1!\n"
	.text
	.align	2
	.global	nand_sector_cache_read
	.type	nand_sector_cache_read, %function
nand_sector_cache_read:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	x2, [x29, 24]
	mov	w0, 512
	str	w0, [x29, 76]
	ldr	x0, [x29, 40]
	add	x0, x0, 2880
	str	x0, [x29, 64]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 64]
	bl	search_from_sector_cache_write_list
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	beq	.L128
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldr	w2, [x29, 76]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	mov	w0, 0
	b	.L129
.L128:
	ldr	x3, [x29, 24]
	mov	w2, 1
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	bl	__nand_read
	str	w0, [x29, 52]
	ldr	w0, [x29, 52]
	cmp	w0, 0
	beq	.L130
	adrp	x0, .LC32
	add	x0, x0, :lo12:.LC32
	bl	nand_print
	mov	w0, 1
	b	.L129
.L130:
	ldr	w0, [x29, 52]
.L129:
	ldp	x29, x30, [sp], 80
	ret
	.size	nand_sector_cache_read, .-nand_sector_cache_read
	.section	.rodata
	.align	3
.LC33:
	.string	"[NE]nand_sector_cache_write 1!\n"
	.align	3
.LC34:
	.string	"[NE]nand_sector_cache_write 2!\n"
	.text
	.align	2
	.global	nand_sector_cache_write
	.type	nand_sector_cache_write, %function
nand_sector_cache_write:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	x2, [x29, 24]
	str	wzr, [x29, 68]
	strh	wzr, [x29, 66]
	ldr	x0, [x29, 40]
	add	x0, x0, 2880
	str	x0, [x29, 56]
	mov	w0, 512
	strh	w0, [x29, 66]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 56]
	bl	search_from_sector_cache_write_list
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	cmp	x0, 0
	beq	.L132
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 56]
	bl	del_from_sector_cache_write_list
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w1, [x29, 66]
	mov	w2, w1
	ldr	x1, [x29, 24]
	bl	nand_memcpy
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 56]
	bl	add_to_sector_cache_write_list_tail
	mov	w0, 0
	b	.L133
.L132:
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	cmp	w0, 7
	bls	.L134
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 544]
	mov	x1, x0
	ldr	x0, [x29, 56]
	bl	del_from_sector_cache_write_list
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 4]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	mov	x3, x0
	mov	w2, 1
	ldr	x0, [x29, 40]
	bl	__nand_write
	str	w0, [x29, 68]
	ldr	w0, [x29, 68]
	cmp	w0, 0
	beq	.L135
	adrp	x0, .LC33
	add	x0, x0, :lo12:.LC33
	bl	nand_print
	mov	w0, 1
	b	.L133
.L134:
	ldr	x0, [x29, 56]
	bl	get_empty_sector_cahce_node
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	cmp	x0, 0
	bne	.L135
	adrp	x0, .LC34
	add	x0, x0, :lo12:.LC34
	bl	nand_print
	mov	w0, 1
	b	.L133
.L135:
	ldr	x0, [x29, 72]
	ldr	w1, [x29, 36]
	str	w1, [x0, 4]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w1, [x29, 66]
	mov	w2, w1
	ldr	x1, [x29, 24]
	bl	nand_memcpy
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 56]
	bl	add_to_sector_cache_write_list_tail
	mov	w0, 0
.L133:
	ldp	x29, x30, [sp], 80
	ret
	.size	nand_sector_cache_write, .-nand_sector_cache_write
	.section	.rodata
	.align	3
.LC35:
	.string	"[NE]nand_flush_write_cache write error\n"
	.text
	.align	2
	.global	__nand_flush_sector_write_cache
	.type	__nand_flush_sector_write_cache, %function
__nand_flush_sector_write_cache:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	add	x0, x0, 2880
	str	x0, [x29, 48]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 48]
	ldrh	w0, [x0, 2]
	cmp	w0, 0
	beq	.L137
	mov	w0, 1
	str	w0, [x29, 44]
	b	.L139
.L137:
	str	wzr, [x29, 44]
	b	.L139
.L141:
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 544]
	mov	x1, x0
	ldr	x0, [x29, 48]
	bl	del_from_sector_cache_write_list
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 4]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	mov	x3, x0
	mov	w2, 1
	ldr	x0, [x29, 24]
	bl	__nand_write
	str	w0, [x29, 60]
	ldr	w0, [x29, 20]
	sub	w0, w0, #1
	str	w0, [x29, 20]
	ldr	w0, [x29, 20]
	cmp	w0, 0
	beq	.L145
.L139:
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 544]
	cmp	x0, 0
	bne	.L141
	b	.L140
.L145:
	nop
.L140:
	ldr	w0, [x29, 60]
	cmp	w0, 0
	beq	.L144
	adrp	x0, .LC35
	add	x0, x0, :lo12:.LC35
	bl	nand_print
.L144:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	__nand_flush_sector_write_cache, .-__nand_flush_sector_write_cache
	.section	.rodata
	.align	3
.LC36:
	.string	"[NE]%s: malloc fail for zone\n"
	.align	3
.LC37:
	.string	"[NE]%s: malloc fail for nand_chip\n"
	.align	3
.LC38:
	.string	"[NE]%s: malloc fail for nand_block_info\n"
	.align	3
.LC39:
	.string	"[NE]not enough blocks %d!\n"
	.align	3
.LC40:
	.string	"[ND]nftl start:%d,%d\n"
	.align	3
.LC41:
	.string	"[NE]zone_param_init fail!\n"
	.align	3
.LC42:
	.string	"[NE]build_zone_list_first fail!\n"
	.align	3
.LC43:
	.string	"[NE]build_zone_list fail!\n"
	.align	3
.LC44:
	.string	"[NI]try again\n"
	.text
	.align	2
	.global	nftl_start
	.type	nftl_start, %function
nftl_start:
	stp	x29, x30, [sp, -160]!
	add	x29, sp, 0
	str	x19, [sp, 16]
	str	x0, [x29, 88]
	str	x1, [x29, 80]
	str	x2, [x29, 72]
	str	x3, [x29, 64]
	str	x4, [x29, 56]
	str	w5, [x29, 52]
	str	w6, [x29, 48]
	str	w7, [x29, 44]
	str	wzr, [x29, 152]
	ldr	x0, [x29, 64]
	lsr	x0, x0, 9
	str	x0, [x29, 64]
	ldr	x0, [x29, 56]
	lsr	x0, x0, 9
	str	x0, [x29, 56]
	ldr	w0, [x29, 52]
	lsr	w0, w0, 9
	str	w0, [x29, 52]
	ldr	w0, [x29, 48]
	lsr	w0, w0, 9
	str	w0, [x29, 48]
	ldr	x0, [x29, 64]
	str	w0, [x29, 148]
	ldr	x0, [x29, 56]
	str	w0, [x29, 144]
	ldr	w0, [x29, 52]
	str	w0, [x29, 140]
	ldr	w0, [x29, 48]
	str	w0, [x29, 136]
	mov	w0, 3536
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 72]
	str	x1, [x0]
	ldr	x0, [x29, 72]
	ldr	x0, [x0]
	str	x0, [x29, 128]
	ldr	x0, [x29, 128]
	cmp	x0, 0
	bne	.L147
	adrp	x0, __func__.5380
	add	x1, x0, :lo12:__func__.5380
	adrp	x0, .LC36
	add	x0, x0, :lo12:.LC36
	bl	nand_print
	mov	w0, -2
	b	.L148
.L147:
	ldr	w0, [x29, 152]
	add	w0, w0, 1
	str	w0, [x29, 152]
	mov	w2, 3536
	mov	w1, 0
	ldr	x0, [x29, 128]
	bl	nand_memset
	ldr	x0, [x29, 128]
	ldr	x1, [x29, 88]
	str	x1, [x0]
	ldr	x0, [x29, 128]
	ldr	x1, [x29, 80]
	str	x1, [x0, 3448]
	mov	w0, 48
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 128]
	str	x1, [x0, 8]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	cmp	x0, 0
	bne	.L149
	adrp	x0, __func__.5380
	add	x1, x0, :lo12:__func__.5380
	adrp	x0, .LC37
	add	x0, x0, :lo12:.LC37
	bl	nand_print
	mov	w0, -2
	b	.L148
.L149:
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	mov	w2, 8
	mov	w1, 0
	bl	nand_memset
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	mov	x3, x0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	add	x0, x0, 16
	mov	w2, 8
	mov	x1, x0
	mov	x0, x3
	bl	nand_memcpy
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	w1, [x29, 136]
	uxth	w1, w1
	ubfiz	w1, w1, 9, 7
	uxth	w1, w1
	strh	w1, [x0, 26]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	w1, [x29, 44]
	uxtb	w1, w1
	strb	w1, [x0, 10]
	ldr	w1, [x29, 144]
	ldr	w0, [x29, 140]
	udiv	w0, w1, w0
	str	w0, [x29, 124]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 12]
	mov	w1, w0
	ldr	w0, [x29, 124]
	cmp	w1, w0
	bcc	.L150
	mov	w0, -1
	b	.L148
.L150:
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	w1, [x29, 124]
	uxth	w1, w1
	strh	w1, [x0, 24]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	w2, [x29, 140]
	ldr	w1, [x29, 136]
	udiv	w1, w2, w1
	uxth	w1, w1
	strh	w1, [x0, 20]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	w1, [x29, 136]
	uxtb	w1, w1
	strb	w1, [x0, 8]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x29, 128]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	uxth	w1, w1
	strh	w1, [x0, 28]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	mov	w1, 3000
	strh	w1, [x0, 16]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	adrp	x1, g_nsi
	add	x1, x1, :lo12:g_nsi
	ldr	x1, [x1]
	ldr	x1, [x1, 8]
	ldr	w1, [x1, 256]
	uxtb	w1, w1
	strb	w1, [x0, 40]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	adrp	x1, g_nsi
	add	x1, x1, :lo12:g_nsi
	ldr	x1, [x1]
	ldr	x1, [x1, 8]
	ldr	w1, [x1, 260]
	str	w1, [x0, 44]
	ldr	w0, [x29, 124]
	str	w0, [x29, 120]
	ldr	x0, [x29, 128]
	ldr	x19, [x0, 8]
	ldr	w0, [x29, 120]
	add	w0, w0, 1
	lsl	w0, w0, 6
	bl	nftl_malloc
	str	x0, [x19, 32]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x0, [x0, 32]
	cmp	x0, 0
	bne	.L151
	adrp	x0, __func__.5380
	add	x1, x0, :lo12:__func__.5380
	adrp	x0, .LC38
	add	x0, x0, :lo12:.LC38
	bl	nand_print
	mov	w0, -2
	b	.L148
.L151:
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x3, [x0, 32]
	ldr	w0, [x29, 120]
	lsl	w0, w0, 6
	mov	w2, w0
	mov	w1, -1
	mov	x0, x3
	bl	nand_memset
	str	wzr, [x29, 156]
	b	.L152
.L153:
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	ldr	w1, [x29, 156]
	uxth	w1, w1
	strh	w1, [x0]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	mov	w1, -1
	str	w1, [x0, 8]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	strh	wzr, [x0, 6]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 16]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 24]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 32]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 40]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 48]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 56]
	ldr	w0, [x29, 156]
	add	w0, w0, 1
	str	w0, [x29, 156]
.L152:
	ldr	w0, [x29, 120]
	add	w1, w0, 1
	ldr	w0, [x29, 156]
	cmp	w1, w0
	bhi	.L153
	ldr	x0, [x29, 72]
	ldr	x0, [x0]
	bl	nftl_ops_init
	ldr	w1, [x29, 148]
	ldr	w0, [x29, 140]
	udiv	w0, w1, w0
	str	w0, [x29, 116]
	ldr	w0, [x29, 124]
	uxth	w1, w0
	ldr	w0, [x29, 116]
	uxth	w0, w0
	sub	w0, w1, w0
	uxth	w1, w0
	ldr	x0, [x29, 128]
	strh	w1, [x0, 30]
	ldr	x0, [x29, 128]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 12]
	cmp	w1, w0
	bcs	.L154
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	adrp	x0, .LC39
	add	x0, x0, :lo12:.LC39
	bl	nand_print
	mov	w0, -2
	b	.L148
.L154:
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	adrp	x0, .LC40
	add	x0, x0, :lo12:.LC40
	mov	w2, w1
	ldr	w1, [x29, 124]
	bl	nand_print_dbg
	ldr	w0, [x29, 140]
	str	w0, [x29, 112]
	ldr	x0, [x29, 72]
	ldr	x4, [x0]
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	ldr	w0, [x29, 124]
	sub	w1, w0, w1
	ldr	w0, [x29, 112]
	mul	w1, w1, w0
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 30]
	mov	w2, w0
	ldr	w0, [x29, 112]
	mul	w0, w2, w0
	mov	w3, w0
	mov	w2, w1
	mov	w1, 0
	mov	x0, x4
	bl	zone_param_init
	str	w0, [x29, 108]
	ldr	w0, [x29, 108]
	cmp	w0, 0
	beq	.L155
	adrp	x0, .LC41
	add	x0, x0, :lo12:.LC41
	bl	nand_print
	ldr	w0, [x29, 108]
	b	.L148
.L155:
	ldr	w0, [x29, 168]
	uxtb	w1, w0
	ldr	x0, [x29, 128]
	strb	w1, [x0, 24]
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 26]
	cmp	w0, 0
	beq	.L156
	ldr	x0, [x29, 128]
	ldrb	w0, [x0, 25]
	orr	w0, w0, 1
	uxtb	w1, w0
	ldr	x0, [x29, 128]
	strb	w1, [x0, 25]
.L156:
	ldr	x0, [x29, 128]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 10]
	cmp	w0, 0
	beq	.L157
	ldr	x0, [x29, 72]
	ldr	x0, [x0]
	ldr	w1, [x29, 124]
	bl	is_no_use_device
	cmp	w0, 1
	bne	.L157
	ldr	x0, [x29, 72]
	ldr	x0, [x0]
	ldr	w1, [x29, 124]
	bl	build_zone_list_first
	str	w0, [x29, 108]
	ldr	w0, [x29, 108]
	cmp	w0, 0
	beq	.L157
	adrp	x0, .LC42
	add	x0, x0, :lo12:.LC42
	bl	nand_print
	ldr	w0, [x29, 108]
	b	.L148
.L157:
	ldr	x0, [x29, 72]
	ldr	x0, [x0]
	bl	build_zone_list
	str	w0, [x29, 108]
	ldr	w0, [x29, 108]
	cmp	w0, 0
	beq	.L158
	adrp	x0, .LC43
	add	x0, x0, :lo12:.LC43
	bl	nand_print
	ldr	w0, [x29, 152]
	cmp	w0, 1
	bhi	.L159
	adrp	x0, .LC44
	add	x0, x0, :lo12:.LC44
	bl	nand_print
	b	.L147
.L159:
	ldr	w0, [x29, 108]
	b	.L148
.L158:
	ldr	x0, [x29, 72]
	ldr	x0, [x0]
	bl	nftl_cache_init
	mov	w0, 0
.L148:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 160
	ret
	.size	nftl_start, .-nftl_start
	.align	2
	.global	nftl_zone_release
	.type	nftl_zone_release, %function
nftl_zone_release:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	nftl_cache_exit
	ldr	x0, [x29, 24]
	bl	zone_param_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldr	x0, [x0, 32]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	bl	nftl_free
	ldr	x0, [x29, 24]
	bl	nftl_free
	nop
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_zone_release, .-nftl_zone_release
	.local	gc_src_type
	.comm	gc_src_type,4,4
	.section	.rodata
	.align	3
.LC45:
	.string	"[NE]garbage_collect phy_block null\n"
	.align	3
.LC46:
	.string	"[ND]garbage too frequency! %d:%d\n"
	.align	3
.LC47:
	.string	"[NE]garbage_collect_first wrong fbn:%d\n"
	.text
	.align	2
	.global	garbage_collect
	.type	garbage_collect, %function
garbage_collect:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	cmp	w1, w0
	bhi	.L162
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 684]
.L162:
	adrp	x0, gc_src_type
	add	x0, x0, :lo12:gc_src_type
	mov	w1, 3
	str	w1, [x0]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 684]
	cmp	w0, 0
	bne	.L163
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	cmp	w1, w0
	bcs	.L164
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 686]
	cmp	w0, 0
	bne	.L165
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L166
	adrp	x0, .LC45
	add	x0, x0, :lo12:.LC45
	bl	nand_print
	mov	w0, 1
	b	.L167
.L166:
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 1
	uxth	w0, w0
	cmp	w1, w0
	bcc	.L168
	ldr	x0, [x29, 24]
	mov	w1, 4
	strh	w1, [x0, 688]
	b	.L169
.L168:
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 2
	uxth	w0, w0
	cmp	w1, w0
	bcc	.L170
	ldr	x0, [x29, 24]
	mov	w1, 8
	strh	w1, [x0, 688]
	b	.L169
.L170:
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 3
	uxth	w0, w0
	cmp	w1, w0
	bcc	.L171
	ldr	x0, [x29, 24]
	mov	w1, 16
	strh	w1, [x0, 688]
	b	.L169
.L171:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 4]
	mov	w2, w0
	adrp	x0, .LC46
	add	x0, x0, :lo12:.LC46
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	mov	w1, -1
	strh	w1, [x0, 688]
.L169:
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 16]
	cmp	w1, w0
	bhi	.L165
	ldr	x0, [x29, 24]
	mov	w1, -1
	strh	w1, [x0, 688]
.L165:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 688]
	strh	w0, [x29, 38]
	ldrh	w1, [x29, 38]
	mov	w0, 65535
	cmp	w1, w0
	beq	.L172
	ldrh	w2, [x29, 38]
	mov	x1, 0
	ldr	x0, [x29, 24]
	bl	garbage_collect_first
	cmp	w0, 0
	beq	.L163
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	adrp	x0, .LC47
	add	x0, x0, :lo12:.LC47
	bl	nand_print
	mov	w0, 1
	b	.L167
.L172:
	mov	w1, 1
	ldr	x0, [x29, 24]
	bl	gc_win_block
	b	.L163
.L164:
	ldr	x0, [x29, 24]
	mov	w1, 1
	strh	w1, [x0, 684]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 686]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 688]
.L163:
	mov	w0, 0
.L167:
	ldp	x29, x30, [sp], 48
	ret
	.size	garbage_collect, .-garbage_collect
	.section	.rodata
	.align	3
.LC48:
	.string	"[NI]garbage_collect_first none!!\n"
	.align	3
.LC49:
	.string	"[ND]garbage_collect_first wrong!!\n"
	.align	3
.LC50:
	.string	"[ND]garbage_collect_first cannot find PHY_MAPPING_PAGE%d %d %d %d!!\n"
	.align	3
.LC51:
	.string	"[ND]garbage_collect_first cannot find PHY_MAPPING_PAGE%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x.\n"
	.align	3
.LC52:
	.string	"[ND][NE]garbage_collect_first rebuild %d,%d!!\n"
	.align	3
.LC53:
	.string	"[ND]garbage_collect_first PHY_MAPPING_PAGEecc err block:%d !!\n"
	.align	3
.LC54:
	.string	"[NE]garbage_collect_first invaild_page num error!!\n"
	.align	3
.LC55:
	.string	"[ND]garbage_collect_first read a logic page error%d %d %d %d spare data:\n"
	.align	3
.LC56:
	.string	"%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n"
	.align	3
.LC57:
	.string	"[NE]nand_write_logic_page_no_gc gc fail!!\n"
	.align	3
.LC58:
	.string	"[NE]erase ecc error block\n"
	.text
	.align	2
	.global	garbage_collect_first
	.type	garbage_collect_first, %function
garbage_collect_first:
	sub	sp, sp, #224
	stp	x29, x30, [sp, 80]
	add	x29, sp, 80
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	strh	w2, [x29, 30]
	str	wzr, [x29, 116]
	str	wzr, [x29, 132]
	str	wzr, [x29, 112]
	add	x0, x29, 72
	mov	w2, 16
	mov	w1, 90
	bl	nand_memset
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L175
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_invalid_page_list
	str	x0, [x29, 120]
	b	.L176
.L175:
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	is_current_blcok
	cmp	w0, 1
	bne	.L177
	mov	w0, 0
	b	.L202
.L177:
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_invalid_page_list_by_block
	str	x0, [x29, 120]
.L176:
	ldr	x0, [x29, 120]
	cmp	x0, 0
	bne	.L179
	adrp	x0, .LC48
	add	x0, x0, :lo12:.LC48
	bl	nand_print
	mov	w0, 8
	b	.L202
.L179:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 120]
	add	w1, w1, 1
	str	w1, [x0, 120]
	ldr	x0, [x29, 120]
	ldrh	w1, [x0]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 512]
	add	x5, x29, 72
	add	x0, x29, 48
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3472]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 136]
	add	x0, x29, 72
	bl	is_phy_mapping_page
	cmp	w0, 1
	beq	.L180
	adrp	x0, .LC49
	add	x0, x0, :lo12:.LC49
	bl	nand_print_dbg
	ldr	x0, [x29, 120]
	ldrh	w0, [x0]
	mov	w5, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w1, w0, #1
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 28]
	mov	w2, w0
	adrp	x0, .LC50
	add	x0, x0, :lo12:.LC50
	mov	w4, w2
	ldr	w3, [x29, 112]
	mov	w2, w1
	mov	w1, w5
	bl	nand_print_dbg
	ldrb	w0, [x29, 73]
	mov	w9, w0
	ldrb	w0, [x29, 74]
	mov	w10, w0
	ldrb	w0, [x29, 75]
	mov	w11, w0
	ldrb	w0, [x29, 76]
	mov	w12, w0
	ldrb	w0, [x29, 77]
	mov	w13, w0
	ldrb	w0, [x29, 78]
	mov	w14, w0
	ldrb	w0, [x29, 79]
	mov	w15, w0
	ldrb	w0, [x29, 80]
	mov	w1, w0
	ldrb	w0, [x29, 81]
	mov	w2, w0
	ldrb	w0, [x29, 82]
	mov	w3, w0
	ldrb	w0, [x29, 83]
	mov	w4, w0
	ldrb	w0, [x29, 84]
	mov	w5, w0
	ldrb	w0, [x29, 85]
	mov	w6, w0
	ldrb	w0, [x29, 86]
	mov	w7, w0
	ldrb	w0, [x29, 87]
	mov	w8, w0
	adrp	x0, .LC51
	add	x0, x0, :lo12:.LC51
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w15
	mov	w6, w14
	mov	w5, w13
	mov	w4, w12
	mov	w3, w11
	mov	w2, w10
	mov	w1, w9
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	ldr	x0, [x29, 120]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC52
	add	x0, x0, :lo12:.LC52
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 480]
	mov	x2, x0
	ldr	x1, [x29, 120]
	ldr	x0, [x29, 40]
	bl	recover_block_phy_page_mapping
	mov	w0, 1
	str	w0, [x29, 132]
	b	.L181
.L180:
	ldr	w0, [x29, 136]
	cmp	w0, 0
	beq	.L182
	ldr	w0, [x29, 136]
	cmp	w0, 10
	beq	.L182
	ldr	x0, [x29, 120]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC53
	add	x0, x0, :lo12:.LC53
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 480]
	mov	x2, x0
	ldr	x1, [x29, 120]
	ldr	x0, [x29, 40]
	bl	recover_block_phy_page_mapping
	mov	w0, 1
	str	w0, [x29, 132]
	b	.L181
.L182:
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 480]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 512]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 488]
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
.L181:
	ldr	x0, [x29, 40]
	add	x1, x0, 464
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w3, w0
	ldr	x2, [x29, 120]
	ldr	x0, [x29, 40]
	bl	check_invalid_page_right
	str	w0, [x29, 136]
	ldr	w0, [x29, 136]
	cmp	w0, 0
	beq	.L183
	adrp	x0, .LC54
	add	x0, x0, :lo12:.LC54
	bl	nand_print
	mov	w0, 1
	str	w0, [x29, 132]
.L183:
	ldr	w0, [x29, 132]
	cmp	w0, 1
	bne	.L184
	mov	w0, -1
	strh	w0, [x29, 30]
.L184:
	str	wzr, [x29, 140]
	b	.L185
.L198:
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 480]
	ldr	w0, [x29, 140]
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [x29, 108]
	ldr	w0, [x29, 108]
	cmn	w0, #1
	beq	.L203
	ldr	w1, [x29, 108]
	ldr	x0, [x29, 40]
	bl	get_logic_page_map
	str	x0, [x29, 96]
	ldr	x0, [x29, 96]
	cmp	x0, 0
	beq	.L204
	ldr	x0, [x29, 96]
	ldrh	w1, [x0, 2]
	ldr	x0, [x29, 120]
	ldrh	w0, [x0]
	cmp	w1, w0
	bne	.L205
	ldr	x0, [x29, 96]
	ldrh	w0, [x0]
	ubfx	x0, x0, 0, 12
	uxth	w0, w0
	mov	w1, w0
	ldr	w0, [x29, 140]
	cmp	w1, w0
	bne	.L206
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 124]
	add	w1, w1, 1
	str	w1, [x0, 124]
	ldr	x0, [x29, 120]
	ldrh	w1, [x0]
	ldr	w0, [x29, 140]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 520]
	add	x5, x29, 72
	add	x0, x29, 48
	bl	set_physic_op_par
	str	wzr, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 1
	bhi	.L191
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3472]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 136]
	nop
.L191:
	add	x0, x29, 72
	bl	get_logic_page_from_oob
	str	w0, [x29, 116]
	ldr	w0, [x29, 136]
	cmp	w0, 0
	beq	.L192
	ldr	w0, [x29, 136]
	cmp	w0, 10
	bne	.L193
.L192:
	ldr	w1, [x29, 116]
	ldr	w0, [x29, 108]
	cmp	w1, w0
	beq	.L194
	ldr	x0, [x29, 120]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC55
	add	x0, x0, :lo12:.LC55
	ldr	w4, [x29, 108]
	ldr	w3, [x29, 116]
	ldr	w2, [x29, 140]
	bl	nand_print_dbg
	ldrb	w0, [x29, 72]
	mov	w10, w0
	ldrb	w0, [x29, 73]
	mov	w11, w0
	ldrb	w0, [x29, 74]
	mov	w12, w0
	ldrb	w0, [x29, 75]
	mov	w13, w0
	ldrb	w0, [x29, 76]
	mov	w14, w0
	ldrb	w0, [x29, 77]
	mov	w15, w0
	ldrb	w0, [x29, 78]
	mov	w16, w0
	ldrb	w0, [x29, 79]
	mov	w1, w0
	ldrb	w0, [x29, 80]
	mov	w2, w0
	ldrb	w0, [x29, 81]
	mov	w3, w0
	ldrb	w0, [x29, 82]
	mov	w4, w0
	ldrb	w0, [x29, 83]
	mov	w5, w0
	ldrb	w0, [x29, 84]
	mov	w6, w0
	ldrb	w0, [x29, 85]
	mov	w7, w0
	ldrb	w0, [x29, 86]
	mov	w8, w0
	ldrb	w0, [x29, 87]
	mov	w9, w0
	adrp	x0, .LC56
	add	x0, x0, :lo12:.LC56
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w16
	mov	w6, w15
	mov	w5, w14
	mov	w4, w13
	mov	w3, w12
	mov	w2, w11
	mov	w1, w10
	bl	nand_print
	ldr	x0, [x29, 96]
	mov	w1, -1
	strh	w1, [x0, 2]
	ldr	x0, [x29, 96]
	ldrh	w1, [x0]
	orr	w1, w1, 4095
	strh	w1, [x0]
	mov	w0, 1
	str	w0, [x29, 132]
	mov	w0, -1
	strh	w0, [x29, 30]
	ldr	x0, [x29, 120]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 120]
	strh	w1, [x0, 4]
	b	.L207
.L194:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 520]
	mov	x2, x0
	ldr	w1, [x29, 108]
	ldr	x0, [x29, 40]
	bl	nand_write_logic_page_no_gc
	str	w0, [x29, 136]
	ldr	w0, [x29, 136]
	cmp	w0, 0
	beq	.L207
	adrp	x0, .LC57
	add	x0, x0, :lo12:.LC57
	bl	nand_print
	mov	w0, 1
	b	.L202
.L193:
	ldr	x0, [x29, 120]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 120]
	strh	w1, [x0, 4]
	mov	w0, 1
	str	w0, [x29, 132]
	mov	w0, -1
	strh	w0, [x29, 30]
	b	.L196
.L207:
	nop
.L196:
	ldrh	w0, [x29, 30]
	sub	w0, w0, #1
	strh	w0, [x29, 30]
	ldrh	w0, [x29, 30]
	cmp	w0, 0
	beq	.L208
	b	.L187
.L203:
	nop
	b	.L187
.L204:
	nop
	b	.L187
.L205:
	nop
	b	.L187
.L206:
	nop
.L187:
	ldr	w0, [x29, 140]
	add	w0, w0, 1
	str	w0, [x29, 140]
.L185:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w1, w0
	ldr	w0, [x29, 140]
	cmp	w1, w0
	bhi	.L198
	b	.L197
.L208:
	nop
.L197:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w1, w0
	ldr	w0, [x29, 140]
	cmp	w1, w0
	bne	.L199
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 686]
.L199:
	ldr	w0, [x29, 132]
	cmp	w0, 1
	bne	.L200
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w1, w0
	ldr	w0, [x29, 140]
	cmp	w1, w0
	bne	.L200
	adrp	x0, .LC58
	add	x0, x0, :lo12:.LC58
	bl	nand_print
	ldr	x0, [x29, 120]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 120]
	ldr	x0, [x29, 40]
	bl	erase_block
	b	.L201
.L200:
	ldr	x1, [x29, 120]
	ldr	x0, [x29, 40]
	bl	put_phy_block_to_invalid_page_list
	ldr	x0, [x29, 40]
	bl	adjust_invaild_list
.L201:
	mov	w0, 0
.L202:
	add	sp, x29, 0
	ldp	x29, x30, [sp], 144
	ret
	.size	garbage_collect_first, .-garbage_collect_first
	.align	2
	.global	get_empty_gc_node
	.type	get_empty_gc_node, %function
get_empty_gc_node:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	strh	wzr, [sp, 30]
	b	.L210
.L213:
	ldrh	w0, [sp, 30]
	ldr	x1, [sp, 8]
	sxtw	x0, w0
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 10
	ldrh	w0, [x0]
	cmp	w0, 0
	bne	.L211
	ldrh	w0, [sp, 30]
	sxtw	x0, w0
	lsl	x0, x0, 5
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	add	x0, x0, 8
	b	.L212
.L211:
	ldrh	w0, [sp, 30]
	add	w0, w0, 1
	strh	w0, [sp, 30]
.L210:
	ldrh	w0, [sp, 30]
	cmp	w0, 9
	bls	.L213
	mov	x0, 0
.L212:
	add	sp, sp, 32
	ret
	.size	get_empty_gc_node, .-get_empty_gc_node
	.align	2
	.global	search_gc_node
	.type	search_gc_node, %function
search_gc_node:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 328
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L215
.L218:
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 8]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L216
	ldr	x0, [sp, 24]
	b	.L217
.L216:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L215:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L218
	mov	x0, 0
.L217:
	add	sp, sp, 32
	ret
	.size	search_gc_node, .-search_gc_node
	.align	2
	.global	add_to_gc_list_tail
	.type	add_to_gc_list_tail, %function
add_to_gc_list_tail:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 328
	str	x0, [sp, 24]
	b	.L220
.L221:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L220:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L221
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 16]
	ldr	x0, [sp]
	str	xzr, [x0, 16]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 24]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0]
	mov	w0, 0
	add	sp, sp, 32
	ret
	.size	add_to_gc_list_tail, .-add_to_gc_list_tail
	.align	2
	.global	del_from_gc_list
	.type	del_from_gc_list, %function
del_from_gc_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp]
	ldr	x0, [x0, 24]
	str	x0, [sp, 24]
	ldr	x0, [sp]
	ldr	x1, [x0, 16]
	ldr	x0, [sp, 24]
	str	x1, [x0, 16]
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	beq	.L224
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	ldr	x1, [sp, 24]
	str	x1, [x0, 24]
.L224:
	ldr	x0, [sp]
	str	xzr, [x0, 16]
	ldr	x0, [sp]
	str	xzr, [x0, 24]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0]
	ldr	x0, [sp]
	add	sp, sp, 32
	ret
	.size	del_from_gc_list, .-del_from_gc_list
	.align	2
	.global	add_prio_gc
	.type	add_prio_gc, %function
add_prio_gc:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	strh	w2, [x29, 30]
	ldr	x0, [x29, 40]
	add	x0, x0, 696
	ldr	x1, [x29, 32]
	bl	search_gc_node
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	beq	.L227
	mov	w0, 1
	b	.L228
.L227:
	ldr	x0, [x29, 40]
	add	x0, x0, 696
	bl	get_empty_gc_node
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L229
	mov	w0, 1
	b	.L228
.L229:
	ldr	x0, [x29, 56]
	ldr	x1, [x29, 32]
	str	x1, [x0, 8]
	ldr	x0, [x29, 56]
	ldrh	w1, [x29, 30]
	strh	w1, [x0, 2]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 24]
	ldr	x0, [x29, 40]
	add	x0, x0, 696
	ldr	x1, [x29, 56]
	bl	add_to_gc_list_tail
	mov	w0, 0
.L228:
	ldp	x29, x30, [sp], 64
	ret
	.size	add_prio_gc, .-add_prio_gc
	.section	.rodata
	.align	3
.LC59:
	.string	"[ND]no block to prio gc\n"
	.align	3
.LC60:
	.string	"[ND]no need gc:%d, type:%d\n"
	.align	3
.LC61:
	.string	"[NE]do_prio_gc garbage_collect_first fail:%d, type:%d\n"
	.align	3
.LC62:
	.string	"[ND]prio gc fail1:%d, type:%d\n"
	.align	3
.LC63:
	.string	"[ND]prio gc fail2:%d, type:%d\n"
	.text
	.align	2
	.global	do_prio_gc
	.type	do_prio_gc, %function
do_prio_gc:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1040]
	cmp	x0, 0
	bne	.L231
	mov	w0, 0
	b	.L232
.L231:
	ldr	x0, [x29, 24]
	add	x2, x0, 696
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1040]
	mov	x1, x0
	mov	x0, x2
	bl	del_from_gc_list
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	is_current_blcok
	cmp	w0, 1
	bne	.L233
	ldr	x0, [x29, 24]
	add	x0, x0, 696
	ldr	x1, [x29, 56]
	bl	add_to_gc_list_tail
	mov	w0, 0
	b	.L232
.L233:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	cmp	x0, 0
	bne	.L234
	adrp	x0, .LC59
	add	x0, x0, :lo12:.LC59
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 0
	b	.L232
.L234:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	is_block_in_invalid_list
	cmp	w0, 0
	bne	.L235
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	mov	w2, w0
	adrp	x0, .LC60
	add	x0, x0, :lo12:.LC60
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 0
	b	.L232
.L235:
	ldr	x0, [x29, 56]
	ldrh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	strh	w1, [x0, 698]
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	cmp	w0, 5
	bne	.L236
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	mov	w2, 4
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	garbage_collect_first
	str	w0, [x29, 52]
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 0
	b	.L232
.L236:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	mov	w2, -1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	garbage_collect_first
	str	w0, [x29, 52]
	ldr	w0, [x29, 52]
	cmp	w0, 0
	beq	.L237
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	mov	w2, w0
	adrp	x0, .LC61
	add	x0, x0, :lo12:.LC61
	bl	nand_print
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 1
	b	.L232
.L237:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	is_block_in_free_list
	cmp	w0, 0
	bne	.L238
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	mov	w2, w0
	adrp	x0, .LC62
	add	x0, x0, :lo12:.LC62
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 0
	b	.L232
.L238:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L239
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	mov	w2, w0
	adrp	x0, .LC63
	add	x0, x0, :lo12:.LC63
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 0
	b	.L232
.L239:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 698]
	cmp	w0, 1
	bne	.L240
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	erase_block
	b	.L241
.L240:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 698]
	cmp	w0, 2
	bne	.L242
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	erase_block
	b	.L241
.L242:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 698]
	cmp	w0, 4
	bne	.L243
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	erase_block
	b	.L241
.L243:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	erase_block
.L241:
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 56]
	str	xzr, [x0, 8]
	mov	w0, 0
.L232:
	ldp	x29, x30, [sp], 64
	ret
	.size	do_prio_gc, .-do_prio_gc
	.align	2
	.global	prio_gc_all
	.type	prio_gc_all, %function
prio_gc_all:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 40]
	str	wzr, [x29, 44]
	b	.L245
.L246:
	ldr	x0, [x29, 24]
	bl	do_prio_gc
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L245:
	ldr	w0, [x29, 44]
	cmp	w0, 9
	bls	.L246
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	prio_gc_all, .-prio_gc_all
	.section	.rodata
	.align	3
.LC64:
	.string	"[ND]swl: static WL need to do(%d : %d)\n"
	.align	3
.LC65:
	.string	"[ND]swl: min_ec_blk_no = %d,\n"
	.align	3
.LC66:
	.string	"[ND]swl: zone->blocks = %d)\n"
	.text
	.align	2
	.global	check_static_wear_leveling
	.type	check_static_wear_leveling, %function
check_static_wear_leveling:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	xzr, [x29, 56]
	str	xzr, [x29, 48]
	mov	w0, -1
	strh	w0, [x29, 46]
	strh	wzr, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	cmp	x0, 0
	beq	.L249
	mov	w0, 1
	b	.L250
.L249:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	cmp	x0, 0
	beq	.L251
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 304]
	cmp	x0, 0
	bne	.L252
.L251:
	mov	w0, 1
	b	.L250
.L252:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldr	w0, [x0, 8]
	ldr	x1, [x29, 24]
	ldrh	w1, [x1, 26]
	cmp	w0, w1
	bge	.L253
	mov	w0, 1
	b	.L250
.L253:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	mov	w1, w0
	ldr	x0, [x29, 24]
	bl	get_block_max_erase_time_from_free_list
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	cmp	x0, 0
	bne	.L254
	mov	w0, 1
	b	.L250
.L254:
	ldr	x0, [x29, 48]
	ldrh	w0, [x0, 6]
	strh	w0, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 16]
	mov	w0, 52429
	movk	w0, 0xcccc, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	lsr	w0, w0, 2
	uxth	w0, w0
	ldrh	w1, [x29, 44]
	cmp	w1, w0
	bcs	.L255
	mov	w0, 1
	b	.L250
.L255:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldr	w0, [x0, 8]
	ldr	x1, [x29, 24]
	ldrh	w1, [x1, 26]
	sub	w0, w0, w1
	mov	w2, w0
	mov	w1, w3
	ldr	x0, [x29, 24]
	bl	get_block_min_erase_time_from_invalid_list
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L256
	mov	w0, 1
	b	.L250
.L256:
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 6]
	strh	w0, [x29, 46]
	ldrh	w1, [x29, 44]
	ldrh	w0, [x29, 46]
	cmp	w1, w0
	bhi	.L257
	mov	w0, 1
	b	.L250
.L257:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	lsl	w2, w0, 1
	ldrh	w1, [x29, 44]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w0, w0, w1
	cmp	w2, w0
	bge	.L258
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 16]
	mov	w0, 43691
	movk	w0, 0xaaaa, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	lsr	w0, w0, 3
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 1058]
	b	.L259
.L258:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	mov	w1, w0
	ldrh	w0, [x29, 44]
	lsl	w0, w0, 1
	cmp	w1, w0
	bge	.L260
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 16]
	mov	w0, 52429
	movk	w0, 0xcccc, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	lsr	w0, w0, 3
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 1058]
	b	.L259
.L260:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	mov	w2, w0
	ldrh	w1, [x29, 44]
	mov	w0, w1
	lsl	w0, w0, 1
	add	w0, w0, w1
	cmp	w2, w0
	bge	.L261
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 16]
	mov	w0, 18725
	movk	w0, 0x2492, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	sub	w1, w1, w0
	lsr	w1, w1, 1
	add	w0, w1, w0
	lsr	w0, w0, 2
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 1058]
	b	.L259
.L261:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 16]
	mov	w0, 52429
	movk	w0, 0xcccc, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	lsr	w0, w0, 2
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 1058]
.L259:
	ldrh	w1, [x29, 44]
	ldrh	w0, [x29, 46]
	sub	w0, w1, w0
	ldr	x1, [x29, 24]
	ldrh	w1, [x1, 1058]
	cmp	w0, w1
	bge	.L262
	mov	w0, 1
	b	.L250
.L262:
	ldrh	w1, [x29, 44]
	ldrh	w2, [x29, 46]
	adrp	x0, .LC64
	add	x0, x0, :lo12:.LC64
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC65
	add	x0, x0, :lo12:.LC65
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	adrp	x0, .LC66
	add	x0, x0, :lo12:.LC66
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	ldr	x1, [x29, 56]
	str	x1, [x0, 1064]
	mov	w0, 0
.L250:
	ldp	x29, x30, [sp], 64
	ret
	.size	check_static_wear_leveling, .-check_static_wear_leveling
	.section	.rodata
	.align	3
.LC67:
	.string	"[ND]swl: the block(%d) for static WL is not in invalid_list\n"
	.align	3
.LC68:
	.string	"[ND]swl: done_static_wear_leveling(min_ec) block:%d, erase_count:%d\n"
	.align	3
.LC69:
	.string	"[NE]swl: garbage_collect_first fail:%d\n"
	.align	3
.LC70:
	.string	"[NE]swl: last->check fail:%d\n"
	.text
	.align	2
	.global	done_static_wear_leveling
	.type	done_static_wear_leveling, %function
done_static_wear_leveling:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	cmp	x0, 0
	bne	.L264
	mov	w0, 2
	b	.L265
.L264:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	is_current_blcok
	cmp	w0, 1
	bne	.L266
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1064]
	mov	w0, 2
	b	.L265
.L266:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	is_block_in_invalid_list
	cmp	w0, 0
	bne	.L267
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC67
	add	x0, x0, :lo12:.LC67
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1064]
	mov	w0, 2
	b	.L265
.L267:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC68
	add	x0, x0, :lo12:.LC68
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	mov	w1, 1
	strh	w1, [x0, 1062]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 128]
	add	w1, w1, 1
	str	w1, [x0, 128]
	ldr	x0, [x29, 24]
	bl	fill_no_use_data_to_current_block
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	mov	w2, -1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	garbage_collect_first
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L268
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC69
	add	x0, x0, :lo12:.LC69
	bl	nand_print
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1064]
	mov	w0, 1
	b	.L265
.L268:
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 1062]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	is_block_in_free_list
	cmp	w0, 0
	bne	.L269
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 1064]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC70
	add	x0, x0, :lo12:.LC70
	bl	nand_print
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1064]
	mov	w0, 0
	b	.L265
.L269:
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1064]
	mov	w0, 0
.L265:
	ldp	x29, x30, [sp], 48
	ret
	.size	done_static_wear_leveling, .-done_static_wear_leveling
	.align	2
	.global	do_static_wear_leveling
	.type	do_static_wear_leveling, %function
do_static_wear_leveling:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 6]
	cmp	w0, 0
	bne	.L271
	mov	w0, 0
	b	.L272
.L271:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 684]
	cmp	w0, 0
	bne	.L273
	mov	w0, 1
	b	.L272
.L273:
	ldr	x0, [x29, 24]
	bl	check_static_wear_leveling
	cmp	w0, 0
	bne	.L274
	ldr	x0, [x29, 24]
	bl	done_static_wear_leveling
	b	.L272
.L274:
	mov	w0, 0
.L272:
	ldp	x29, x30, [sp], 32
	ret
	.size	do_static_wear_leveling, .-do_static_wear_leveling
	.align	2
	.global	prio_gc_one
	.type	prio_gc_one, %function
prio_gc_one:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	str	w2, [x29, 16]
	ldrh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	bl	get_phy_block_addr
	str	x0, [x29, 40]
	ldr	w0, [x29, 16]
	cmp	w0, 1
	bne	.L276
	mov	w0, 1
	str	w0, [x29, 16]
	b	.L277
.L276:
	ldr	w0, [x29, 16]
	cmp	w0, 2
	bne	.L278
	mov	w0, 2
	str	w0, [x29, 16]
	b	.L277
.L278:
	ldr	w0, [x29, 16]
	cmp	w0, 3
	bne	.L279
	mov	w0, 3
	str	w0, [x29, 16]
	b	.L277
.L279:
	mov	w0, 3
	str	w0, [x29, 16]
.L277:
	ldr	w0, [x29, 16]
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	prio_gc_one, .-prio_gc_one
	.section	.rodata
	.align	3
.LC71:
	.string	"[NE][NE]gc_one error!\n"
	.text
	.align	2
	.global	gc_one
	.type	gc_one, %function
gc_one:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L282
	mov	w0, 8
	b	.L283
.L282:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 4]
	ldrh	w1, [x29, 22]
	cmp	w1, w0
	bcs	.L284
	mov	w2, -1
	mov	x1, 0
	ldr	x0, [x29, 24]
	bl	garbage_collect_first
	str	w0, [x29, 36]
	ldr	w0, [x29, 36]
	cmp	w0, 8
	bne	.L285
	mov	w0, 8
	b	.L283
.L285:
	ldr	w0, [x29, 36]
	cmp	w0, 0
	bne	.L286
	mov	w0, 0
	b	.L283
.L286:
	adrp	x0, .LC71
	add	x0, x0, :lo12:.LC71
	bl	nand_print
	mov	w0, 1
	b	.L283
.L284:
	mov	w0, 8
.L283:
	ldp	x29, x30, [sp], 48
	ret
	.size	gc_one, .-gc_one
	.align	2
	.global	gc_all_cnt
	.type	gc_all_cnt, %function
gc_all_cnt:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	str	w2, [x29, 16]
	adrp	x0, gc_src_type
	add	x0, x0, :lo12:gc_src_type
	mov	w1, 255
	str	w1, [x0]
	b	.L288
.L290:
	ldrh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	bl	gc_one
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 8
	beq	.L292
.L288:
	ldr	w0, [x29, 16]
	sub	w1, w0, #1
	str	w1, [x29, 16]
	cmp	w0, 0
	bne	.L290
	b	.L289
.L292:
	nop
.L289:
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	gc_all_cnt, .-gc_all_cnt
	.align	2
	.global	gc_all
	.type	gc_all, %function
gc_all:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	adrp	x0, gc_src_type
	add	x0, x0, :lo12:gc_src_type
	mov	w1, 255
	str	w1, [x0]
.L296:
	ldrh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	bl	gc_one
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	bne	.L299
	b	.L296
.L299:
	nop
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	gc_all, .-gc_all
	.align	2
	.global	gc_all_enhance
	.type	gc_all_enhance, %function
gc_all_enhance:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, phyinfo
	add	x0, x0, :lo12:phyinfo
	str	x0, [x29, 56]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	str	w0, [x29, 52]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w1, w0
	ldr	w0, [x29, 52]
	cmp	w1, w0
	bls	.L301
	mov	w0, 0
	b	.L302
.L301:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w1, w0
	mov	w0, w1
	lsl	w0, w0, 1
	add	w0, w0, w1
	mov	w1, w0
	ldr	w0, [x29, 52]
	cmp	w1, w0
	bcs	.L303
	mov	w0, 0
	b	.L302
.L303:
	ldr	x0, [x29, 56]
	ldr	w0, [x0, 4]
	ldr	x1, [x29, 56]
	ldrh	w1, [x1, 8]
	mul	w0, w0, w1
	uxtw	x0, w0
	str	x0, [x29, 40]
	ldr	x0, [x29, 56]
	ldrb	w0, [x0, 10]
	uxtb	x0, w0
	ldr	x1, [x29, 40]
	mul	x0, x1, x0
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	lsr	x0, x0, 11
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 128
	bls	.L304
	mov	w0, 0
	b	.L302
.L304:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 1
	uxth	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 24]
	bl	gc_all
.L302:
	ldp	x29, x30, [sp], 64
	ret
	.size	gc_all_enhance, .-gc_all_enhance
	.align	2
	.global	gc_all_base_on_free_blks
	.type	gc_all_base_on_free_blks, %function
gc_all_base_on_free_blks:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, phyinfo
	add	x0, x0, :lo12:phyinfo
	str	x0, [x29, 72]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	str	x0, [x29, 64]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	str	w0, [x29, 60]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w1, w0
	ldr	w0, [x29, 60]
	cmp	w1, w0
	bls	.L306
	mov	w0, 0
	b	.L307
.L306:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w1, w0
	mov	w0, w1
	lsl	w0, w0, 1
	add	w0, w0, w1
	mov	w1, w0
	ldr	w0, [x29, 60]
	cmp	w1, w0
	bcs	.L308
	mov	w0, 0
	b	.L307
.L308:
	ldr	x0, [x29, 72]
	ldr	w0, [x0, 4]
	ldr	x1, [x29, 72]
	ldrh	w1, [x1, 8]
	mul	w0, w0, w1
	uxtw	x0, w0
	str	x0, [x29, 48]
	ldr	x0, [x29, 72]
	ldrb	w0, [x0, 10]
	uxtb	x0, w0
	ldr	x1, [x29, 48]
	mul	x0, x1, x0
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	lsr	x0, x0, 11
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	cmp	x0, 128
	bls	.L309
	mov	w0, 0
	b	.L307
.L309:
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 1
	uxth	w0, w0
	sub	w0, w0, #3
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w1, w0
	ldr	w0, [x29, 60]
	sub	w0, w0, w1
	ldr	w1, [x29, 44]
	mul	w0, w1, w0
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w1, w0
	mov	w0, w1
	lsl	w0, w0, 1
	add	w0, w0, w1
	ldr	x1, [x29, 24]
	ldrh	w1, [x1, 680]
	sub	w0, w0, w1
	mov	w1, w0
	ldr	w0, [x29, 44]
	udiv	w0, w0, w1
	str	w0, [x29, 44]
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 1
	uxth	w0, w0
	mov	w1, w0
	ldr	w0, [x29, 44]
	add	w0, w0, w1
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	uxth	w0, w0
	mov	w2, 3
	mov	w1, w0
	ldr	x0, [x29, 24]
	bl	gc_all_cnt
.L307:
	ldp	x29, x30, [sp], 80
	ret
	.size	gc_all_base_on_free_blks, .-gc_all_base_on_free_blks
	.section	.rodata
	.align	3
.LC72:
	.string	"[ND][NE] gc_win_block gc_one error \n"
	.text
	.align	2
	.global	gc_win_block
	.type	gc_win_block, %function
gc_win_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	str	wzr, [x29, 44]
	ldrh	w0, [x29, 22]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrh	w1, [x1, 20]
	mul	w0, w0, w1
	str	w0, [x29, 40]
	b	.L311
.L315:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	cmp	x0, 0
	bne	.L312
	mov	w0, 0
	b	.L313
.L312:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	w0, [x29, 44]
	add	w0, w0, w1
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	sub	w0, w0, #1
	str	w0, [x29, 44]
	mov	w1, 1
	ldr	x0, [x29, 24]
	bl	gc_one
	str	w0, [x29, 36]
	ldr	w0, [x29, 36]
	cmp	w0, 0
	beq	.L311
	ldr	w0, [x29, 36]
	cmp	w0, 1
	bne	.L314
	adrp	x0, .LC72
	add	x0, x0, :lo12:.LC72
	bl	nand_print_dbg
.L314:
	ldr	w0, [x29, 36]
	b	.L313
.L311:
	ldr	w1, [x29, 44]
	ldr	w0, [x29, 40]
	cmp	w1, w0
	blt	.L315
	mov	w0, 0
.L313:
	ldp	x29, x30, [sp], 48
	ret
	.size	gc_win_block, .-gc_win_block
	.align	2
	.global	get_total_invalid_page
	.type	get_total_invalid_page, %function
get_total_invalid_page:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	wzr, [sp, 20]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	b	.L317
.L318:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	w0, [sp, 20]
	add	w0, w0, w1
	str	w0, [sp, 20]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L317:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L318
	ldr	w0, [sp, 20]
	add	sp, sp, 32
	ret
	.size	get_total_invalid_page, .-get_total_invalid_page
	.align	2
	.global	is_current_blcok
	.type	is_current_blcok, %function
is_current_blcok:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	str	wzr, [sp, 28]
	ldr	x0, [sp, 8]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L321
	ldr	x0, [sp, 8]
	ldr	x1, [x0, 592]
	ldr	x0, [sp]
	cmp	x1, x0
	beq	.L322
	ldr	x0, [sp, 8]
	ldr	x1, [x0, 528]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L324
.L322:
	mov	w0, 1
	str	w0, [sp, 28]
	b	.L324
.L321:
	ldr	x0, [sp, 8]
	ldr	x1, [x0, 528]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L324
	mov	w0, 1
	str	w0, [sp, 28]
.L324:
	ldr	w0, [sp, 28]
	add	sp, sp, 32
	ret
	.size	is_current_blcok, .-is_current_blcok
	.align	2
	.global	static_wear_leveling
	.type	static_wear_leveling, %function
static_wear_leveling:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	cmp	w1, w0
	bhi	.L327
	mov	w0, 0
	b	.L328
.L327:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 668]
	cmp	w0, 19
	bls	.L329
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 180]
	add	w1, w1, 1
	str	w1, [x0, 180]
	ldr	x0, [x29, 24]
	bl	get_block_min_count_from_invalid_list
	str	x0, [x29, 40]
	mov	w2, 5
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	str	w0, [x29, 36]
	ldr	w0, [x29, 36]
	cmp	w0, 0
	bne	.L330
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 672]
	add	w1, w0, 4
	ldr	x0, [x29, 24]
	str	w1, [x0, 672]
.L330:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 672]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrh	w1, [x1, 20]
	cmp	w0, w1
	bcc	.L329
	ldr	x0, [x29, 24]
	str	wzr, [x0, 668]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 672]
.L329:
	mov	w0, 0
.L328:
	ldp	x29, x30, [sp], 48
	ret
	.size	static_wear_leveling, .-static_wear_leveling
	.section	.rodata
	.align	3
.LC73:
	.string	"[ND]not power on gc [%d, %d, %d], [%d %d %d]!\n"
	.align	3
.LC74:
	.string	"[ND]emergency power on gc %d !\n"
	.align	3
.LC75:
	.string	"[ND]power on gc %d!\n"
	.text
	.align	2
	.global	power_on_gc
	.type	power_on_gc, %function
power_on_gc:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	cmp	w1, w0
	bcc	.L332
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w7, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w8, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w3, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	mov	w4, w0
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrh	w1, [x1, 20]
	udiv	w2, w0, w1
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 40]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrh	w1, [x1, 20]
	udiv	w1, w0, w1
	adrp	x0, .LC73
	add	x0, x0, :lo12:.LC73
	mov	w6, w1
	mov	w5, w2
	mov	w2, w8
	mov	w1, w7
	bl	nand_print_dbg
	mov	w0, 0
	b	.L333
.L332:
	adrp	x0, gc_src_type
	add	x0, x0, :lo12:gc_src_type
	mov	w1, 2
	str	w1, [x0]
	strh	wzr, [x29, 46]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #10
	strh	w0, [x29, 44]
	b	.L334
.L337:
	ldrh	w1, [x29, 44]
	ldr	x0, [x29, 24]
	bl	gc_one
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 8
	beq	.L342
	ldrh	w0, [x29, 46]
	add	w0, w0, 1
	strh	w0, [x29, 46]
.L334:
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	cmp	w1, w0
	bcc	.L337
	b	.L336
.L342:
	nop
.L336:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 3
	strh	w0, [x29, 44]
	b	.L338
.L341:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	adrp	x0, .LC74
	add	x0, x0, :lo12:.LC74
	bl	nand_print_dbg
	ldrh	w1, [x29, 44]
	ldr	x0, [x29, 24]
	bl	gc_one
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 8
	beq	.L343
	ldrh	w0, [x29, 46]
	add	w0, w0, 1
	strh	w0, [x29, 46]
.L338:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	cmp	w0, 4
	bls	.L341
	b	.L340
.L343:
	nop
.L340:
	ldrh	w1, [x29, 46]
	adrp	x0, .LC75
	add	x0, x0, :lo12:.LC75
	bl	nand_print_dbg
	mov	w0, 0
.L333:
	ldp	x29, x30, [sp], 48
	ret
	.size	power_on_gc, .-power_on_gc
	.align	2
	.global	all_time_gc
	.type	all_time_gc, %function
all_time_gc:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #3
	strh	w0, [x29, 46]
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	cmp	w1, w0
	bcs	.L345
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 684]
	cmp	w0, 0
	beq	.L345
	adrp	x0, gc_src_type
	add	x0, x0, :lo12:gc_src_type
	mov	w1, 1
	str	w1, [x0]
	ldrh	w1, [x29, 46]
	ldr	x0, [x29, 24]
	bl	gc_one
.L345:
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	all_time_gc, .-all_time_gc
	.section	.rodata
	.align	3
.LC76:
	.string	"[ND]%s %p %d %d %d %\n"
	.align	3
.LC77:
	.string	"[NI]shutdown_on_gc end! [%d %d %d]\n"
	.align	3
.LC78:
	.string	"[ND]shutdown_on_gc one ok! [%d %d %d]\n"
	.text
	.align	2
	.global	shutdown_on_gc
	.type	shutdown_on_gc, %function
shutdown_on_gc:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	str	x0, [x29, 40]
	adrp	x0, gc_src_type
	add	x0, x0, :lo12:gc_src_type
	mov	w1, 4
	str	w1, [x0]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 684]
	mov	w2, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w3, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w4, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w5, w0
	adrp	x0, __func__.5570
	add	x1, x0, :lo12:__func__.5570
	adrp	x0, .LC76
	add	x0, x0, :lo12:.LC76
	mov	w6, w5
	mov	w5, w4
	mov	w4, w3
	mov	w3, w2
	ldr	x2, [x29, 40]
	bl	nand_print_dbg
	b	.L348
.L351:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L349
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w2, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w3, w0
	adrp	x0, .LC77
	add	x0, x0, :lo12:.LC77
	bl	nand_print
	b	.L350
.L349:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 684]
	cmp	w0, 0
	beq	.L348
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	gc_one
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	mov	w2, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w3, w0
	adrp	x0, .LC78
	add	x0, x0, :lo12:.LC78
	bl	nand_print_dbg
.L348:
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	cmp	w1, w0
	bls	.L351
.L350:
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	shutdown_on_gc, .-shutdown_on_gc
	.section	.rodata
	.align	3
.LC79:
	.string	"%d %d %d %d %d %d/%d %d/%d\n"
	.align	3
.LC80:
	.string	"%08d:%04d "
	.text
	.align	2
	.global	nftl_get_gc_info
	.type	nftl_get_gc_info, %function
nftl_get_gc_info:
	sub	sp, sp, #128
	stp	x29, x30, [sp, 32]
	add	x29, sp, 32
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	str	wzr, [x29, 92]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	str	x0, [x29, 56]
	str	wzr, [x29, 76]
	ldr	w0, [x29, 76]
	str	w0, [x29, 80]
	ldr	w0, [x29, 80]
	str	w0, [x29, 88]
	ldr	w0, [x29, 88]
	str	w0, [x29, 84]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 352]
	str	x0, [x29, 64]
	b	.L354
.L356:
	ldr	x0, [x29, 64]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 20]
	lsr	w0, w0, 1
	uxth	w0, w0
	cmp	w1, w0
	bls	.L355
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	w0, [x29, 80]
	add	w0, w0, w1
	str	w0, [x29, 80]
	ldr	w0, [x29, 88]
	add	w0, w0, 1
	str	w0, [x29, 88]
.L355:
	ldr	w0, [x29, 84]
	add	w0, w0, 1
	str	w0, [x29, 84]
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	w0, [x29, 76]
	add	w0, w0, w1
	str	w0, [x29, 76]
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
.L354:
	ldr	x0, [x29, 64]
	cmp	x0, 0
	bne	.L356
	ldrsw	x0, [x29, 92]
	ldr	x1, [x29, 32]
	add	x8, x1, x0
	ldr	w1, [x29, 28]
	ldr	w0, [x29, 92]
	sub	w0, w1, w0
	mov	w9, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 30]
	mov	w10, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 680]
	mov	w4, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 682]
	mov	w5, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	w2, [x0, 120]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	w3, [x0, 124]
	adrp	x0, .LC79
	add	x1, x0, :lo12:.LC79
	ldr	w0, [x29, 76]
	str	w0, [sp, 24]
	ldr	w0, [x29, 80]
	str	w0, [sp, 16]
	ldr	w0, [x29, 84]
	str	w0, [sp, 8]
	ldr	w0, [x29, 88]
	str	w0, [sp]
	mov	w7, w3
	mov	w6, w2
	mov	w3, w10
	mov	x2, x1
	mov	w1, w9
	mov	x0, x8
	bl	nand_snprint
	str	w0, [x29, 92]
	str	wzr, [x29, 88]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 352]
	str	x0, [x29, 64]
	b	.L357
.L360:
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L358
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 4]
	cmp	w0, 0
	beq	.L358
	ldrsw	x0, [x29, 92]
	ldr	x1, [x29, 32]
	add	x5, x1, x0
	ldr	w1, [x29, 28]
	ldr	w0, [x29, 92]
	sub	w0, w1, w0
	mov	w1, w0
	ldr	x0, [x29, 64]
	ldrh	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 4]
	mov	w3, w0
	adrp	x0, .LC80
	add	x0, x0, :lo12:.LC80
	mov	w4, w3
	mov	w3, w2
	mov	x2, x0
	mov	x0, x5
	bl	nand_snprint
	mov	w1, w0
	ldr	w0, [x29, 92]
	add	w0, w0, w1
	str	w0, [x29, 92]
	ldr	w0, [x29, 88]
	add	w0, w0, 1
	str	w0, [x29, 88]
	ldr	w0, [x29, 88]
	and	w0, w0, 7
	cmp	w0, 0
	bne	.L359
	ldrsw	x0, [x29, 92]
	sub	x0, x0, #1
	ldr	x1, [x29, 32]
	add	x0, x1, x0
	mov	w1, 10
	strb	w1, [x0]
.L359:
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
.L357:
	ldr	w1, [x29, 28]
	ldr	w0, [x29, 92]
	sub	w0, w1, w0
	cmp	w0, 1
	bne	.L360
.L358:
	ldrsw	x0, [x29, 92]
	sub	x0, x0, #1
	ldr	x1, [x29, 32]
	add	x0, x1, x0
	mov	w1, 10
	strb	w1, [x0]
	ldr	w0, [x29, 92]
	add	sp, x29, 0
	ldp	x29, x30, [sp], 96
	ret
	.size	nftl_get_gc_info, .-nftl_get_gc_info
	.align	2
	.global	add_free_list
	.type	add_free_list, %function
add_free_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 272
	str	x0, [sp, 24]
	b	.L363
.L366:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	ldrh	w1, [x0, 6]
	ldr	x0, [sp]
	ldrh	w0, [x0, 6]
	cmp	w1, w0
	bcc	.L364
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [sp]
	str	x1, [x0, 40]
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 32]
	ldr	x0, [sp]
	str	x1, [x0, 32]
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 32]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 40]
	mov	w0, 0
	b	.L365
.L364:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
.L363:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	cmp	x0, 0
	bne	.L366
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 32]
	ldr	x0, [sp]
	str	xzr, [x0, 32]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 40]
	mov	w0, 0
.L365:
	add	sp, sp, 32
	ret
	.size	add_free_list, .-add_free_list
	.align	2
	.global	del_free_list
	.type	del_free_list, %function
del_free_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 272
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	cmp	x0, 0
	beq	.L368
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 32]
	ldr	x0, [sp, 8]
	str	x1, [x0, 304]
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 32]
	ldr	x0, [sp, 8]
	str	x1, [x0, 304]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	cmp	x0, 0
	beq	.L368
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [sp, 8]
	add	x1, x1, 272
	str	x1, [x0, 40]
.L368:
	ldr	x0, [sp, 24]
	add	sp, sp, 32
	ret
	.size	del_free_list, .-del_free_list
	.align	2
	.global	del_free_list_by_block
	.type	del_free_list_by_block, %function
del_free_list_by_block:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 272
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
	b	.L371
.L375:
	ldr	x1, [sp, 24]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L372
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 40]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 32]
	str	x1, [x0, 32]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	cmp	x0, 0
	beq	.L373
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 40]
	str	x1, [x0, 40]
.L373:
	ldr	x0, [sp, 24]
	str	xzr, [x0, 40]
	ldr	x0, [sp, 24]
	str	xzr, [x0, 32]
	b	.L374
.L372:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
.L371:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L375
.L374:
	ldr	x0, [sp, 24]
	add	sp, sp, 32
	ret
	.size	del_free_list_by_block, .-del_free_list_by_block
	.align	2
	.global	is_block_in_free_list
	.type	is_block_in_free_list, %function
is_block_in_free_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 272
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
	b	.L378
.L381:
	ldr	x1, [sp, 24]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L379
	mov	w0, 1
	b	.L380
.L379:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
.L378:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L381
	mov	w0, 0
.L380:
	add	sp, sp, 32
	ret
	.size	is_block_in_free_list, .-is_block_in_free_list
	.section	.rodata
	.align	3
.LC81:
	.string	"[ND]free list: \n"
	.align	3
.LC82:
	.string	"[ND]block NO:%4d; erase:%3d; "
	.align	3
.LC83:
	.string	"[ND]count:%4d; "
	.align	3
.LC84:
	.string	"[ND]info:0x%x\n"
	.text
	.align	2
	.global	print_free_list
	.type	print_free_list, %function
print_free_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 272
	str	x0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 656]
	cmp	w0, 0
	beq	.L387
	adrp	x0, .LC81
	add	x0, x0, :lo12:.LC81
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 32]
	str	x0, [x29, 40]
	b	.L385
.L386:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC82
	add	x0, x0, :lo12:.LC82
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 8]
	adrp	x0, .LC83
	add	x0, x0, :lo12:.LC83
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	adrp	x0, .LC84
	add	x0, x0, :lo12:.LC84
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 32]
	str	x0, [x29, 40]
.L385:
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L386
	b	.L382
.L387:
	nop
.L382:
	ldp	x29, x30, [sp], 48
	ret
	.size	print_free_list, .-print_free_list
	.align	2
	.global	add_block_invalid_list
	.type	add_block_invalid_list, %function
add_block_invalid_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	b	.L389
.L392:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	ldrh	w1, [x0, 4]
	ldr	x0, [sp]
	ldrh	w0, [x0, 4]
	cmp	w1, w0
	bhi	.L390
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	ldr	x1, [sp]
	str	x1, [x0, 24]
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 16]
	ldr	x0, [sp]
	str	x1, [x0, 16]
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 16]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 24]
	mov	w0, 0
	b	.L391
.L390:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L389:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L392
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 16]
	ldr	x0, [sp]
	str	xzr, [x0, 16]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 24]
	mov	w0, 0
.L391:
	add	sp, sp, 32
	ret
	.size	add_block_invalid_list, .-add_block_invalid_list
	.align	2
	.global	del_block_invalid_list
	.type	del_block_invalid_list, %function
del_block_invalid_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	cmp	x0, 0
	beq	.L394
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 16]
	ldr	x0, [sp, 8]
	str	x1, [x0, 352]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	beq	.L394
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	ldr	x1, [sp, 8]
	add	x1, x1, 336
	str	x1, [x0, 24]
.L394:
	ldr	x0, [sp, 24]
	add	sp, sp, 32
	ret
	.size	del_block_invalid_list, .-del_block_invalid_list
	.align	2
	.global	del_block_invalid_list_by_block
	.type	del_block_invalid_list_by_block, %function
del_block_invalid_list_by_block:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L397
.L401:
	ldr	x1, [sp, 24]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L398
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 24]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 16]
	str	x1, [x0, 16]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	beq	.L399
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 24]
	str	x1, [x0, 24]
.L399:
	ldr	x0, [sp, 24]
	str	xzr, [x0, 24]
	ldr	x0, [sp, 24]
	str	xzr, [x0, 16]
	b	.L400
.L398:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L397:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L401
.L400:
	ldr	x0, [sp, 24]
	add	sp, sp, 32
	ret
	.size	del_block_invalid_list_by_block, .-del_block_invalid_list_by_block
	.align	2
	.global	is_block_in_invalid_list
	.type	is_block_in_invalid_list, %function
is_block_in_invalid_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L404
.L407:
	ldr	x1, [sp, 24]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L405
	mov	w0, 1
	b	.L406
.L405:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L404:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L407
	mov	w0, 0
.L406:
	add	sp, sp, 32
	ret
	.size	is_block_in_invalid_list, .-is_block_in_invalid_list
	.section	.rodata
	.align	3
.LC85:
	.string	"[ND]invalid_page_count list: \n"
	.align	3
.LC86:
	.string	"[ND]count:%4d; invalid:%3d;"
	.text
	.align	2
	.global	print_block_invalid_list
	.type	print_block_invalid_list, %function
print_block_invalid_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 336
	str	x0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 656]
	cmp	w0, 0
	beq	.L413
	adrp	x0, .LC85
	add	x0, x0, :lo12:.LC85
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	b	.L411
.L412:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC82
	add	x0, x0, :lo12:.LC82
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 4]
	mov	w2, w0
	adrp	x0, .LC86
	add	x0, x0, :lo12:.LC86
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	adrp	x0, .LC84
	add	x0, x0, :lo12:.LC84
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
.L411:
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L412
	b	.L408
.L413:
	nop
.L408:
	ldp	x29, x30, [sp], 48
	ret
	.size	print_block_invalid_list, .-print_block_invalid_list
	.align	2
	.global	add_block_count_list
	.type	add_block_count_list, %function
add_block_count_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 400
	str	x0, [sp, 24]
	b	.L415
.L418:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	ldr	w1, [x0, 8]
	ldr	x0, [sp]
	ldr	w0, [x0, 8]
	cmp	w1, w0
	blt	.L416
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	ldr	x1, [sp]
	str	x1, [x0, 56]
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 48]
	ldr	x0, [sp]
	str	x1, [x0, 48]
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 48]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 56]
	mov	w0, 0
	b	.L417
.L416:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	str	x0, [sp, 24]
.L415:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	cmp	x0, 0
	bne	.L418
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 48]
	ldr	x0, [sp]
	str	xzr, [x0, 48]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 56]
	mov	w0, 0
.L417:
	add	sp, sp, 32
	ret
	.size	add_block_count_list, .-add_block_count_list
	.align	2
	.global	del_block_count_list
	.type	del_block_count_list, %function
del_block_count_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 400
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	cmp	x0, 0
	beq	.L420
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	ldr	x1, [x0, 48]
	ldr	x0, [sp, 24]
	str	x1, [x0, 48]
.L420:
	ldr	x0, [sp, 24]
	add	sp, sp, 32
	ret
	.size	del_block_count_list, .-del_block_count_list
	.align	2
	.global	del_block_count_by_block
	.type	del_block_count_by_block, %function
del_block_count_by_block:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 400
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	str	x0, [sp, 24]
	b	.L423
.L427:
	ldr	x1, [sp, 24]
	ldr	x0, [sp]
	cmp	x1, x0
	bne	.L424
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 56]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 48]
	str	x1, [x0, 48]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	cmp	x0, 0
	beq	.L425
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 56]
	str	x1, [x0, 56]
.L425:
	ldr	x0, [sp, 24]
	str	xzr, [x0, 56]
	ldr	x0, [sp, 24]
	str	xzr, [x0, 48]
	b	.L426
.L424:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 48]
	str	x0, [sp, 24]
.L423:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L427
.L426:
	ldr	x0, [sp, 24]
	add	sp, sp, 32
	ret
	.size	del_block_count_by_block, .-del_block_count_by_block
	.section	.rodata
	.align	3
.LC87:
	.string	"[ND]block_used_count list: \n"
	.align	3
.LC88:
	.string	"[ND]block NO: %d "
	.align	3
.LC89:
	.string	"[ND]block_used_count: %d \n"
	.text
	.align	2
	.global	print_block_count_list
	.type	print_block_count_list, %function
print_block_count_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 400
	str	x0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 656]
	cmp	w0, 0
	beq	.L434
	adrp	x0, .LC87
	add	x0, x0, :lo12:.LC87
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 48]
	str	x0, [x29, 40]
	b	.L432
.L433:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC88
	add	x0, x0, :lo12:.LC88
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 8]
	adrp	x0, .LC89
	add	x0, x0, :lo12:.LC89
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 48]
	str	x0, [x29, 40]
.L432:
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L433
	b	.L429
.L434:
	nop
.L429:
	ldp	x29, x30, [sp], 48
	ret
	.size	print_block_count_list, .-print_block_count_list
	.section	.rodata
	.align	3
.LC90:
	.string	"[ND]free_list error !\n"
	.text
	.align	2
	.global	put_phy_block_to_free_list
	.type	put_phy_block_to_free_list, %function
put_phy_block_to_free_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	str	xzr, [x0, 32]
	ldr	x0, [x29, 16]
	str	xzr, [x0, 40]
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	add_free_list
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	bne	.L436
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 30]
	b	.L437
.L436:
	adrp	x0, .LC90
	add	x0, x0, :lo12:.LC90
	bl	nand_print_dbg
.L437:
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	put_phy_block_to_free_list, .-put_phy_block_to_free_list
	.align	2
	.global	out_phy_block_from_free_list
	.type	out_phy_block_from_free_list, %function
out_phy_block_from_free_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	del_free_list
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	beq	.L440
	ldr	x0, [x29, 40]
	str	xzr, [x0, 32]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 40]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	cmp	w0, 0
	beq	.L440
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 30]
.L440:
	ldr	x0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	out_phy_block_from_free_list, .-out_phy_block_from_free_list
	.align	2
	.global	out_phy_block_from_free_list_by_erase_time
	.type	out_phy_block_from_free_list_by_erase_time, %function
out_phy_block_from_free_list_by_erase_time:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	str	xzr, [x29, 56]
	strh	wzr, [x29, 54]
	ldr	x0, [x29, 24]
	add	x0, x0, 272
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 32]
	str	x0, [x29, 40]
	b	.L443
.L445:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	ldrh	w1, [x29, 22]
	cmp	w1, w0
	bls	.L444
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	ldrh	w1, [x29, 54]
	cmp	w1, w0
	bhi	.L444
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	strh	w0, [x29, 54]
	ldr	x0, [x29, 40]
	str	x0, [x29, 56]
.L444:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 32]
	str	x0, [x29, 40]
.L443:
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L445
	ldr	x0, [x29, 56]
	cmp	x0, 0
	beq	.L446
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [x29, 56]
.L446:
	ldr	x0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	out_phy_block_from_free_list_by_erase_time, .-out_phy_block_from_free_list_by_erase_time
	.align	2
	.global	get_max_erase_time_from_free_list
	.type	get_max_erase_time_from_free_list, %function
get_max_erase_time_from_free_list:
	sub	sp, sp, #48
	str	x0, [sp, 8]
	strh	w1, [sp, 6]
	str	xzr, [sp, 40]
	strh	wzr, [sp, 38]
	ldr	x0, [sp, 8]
	add	x0, x0, 272
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
	b	.L449
.L451:
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 6]
	cmp	w1, w0
	bls	.L450
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 38]
	cmp	w1, w0
	bhi	.L450
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	strh	w0, [sp, 38]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
.L450:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
.L449:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L451
	ldr	x0, [sp, 40]
	cmp	x0, 0
	beq	.L452
	ldr	x0, [sp, 40]
	ldrh	w0, [x0, 6]
	b	.L453
.L452:
	mov	w0, 65535
.L453:
	add	sp, sp, 48
	ret
	.size	get_max_erase_time_from_free_list, .-get_max_erase_time_from_free_list
	.align	2
	.global	get_block_max_erase_time_from_free_list
	.type	get_block_max_erase_time_from_free_list, %function
get_block_max_erase_time_from_free_list:
	sub	sp, sp, #48
	str	x0, [sp, 8]
	strh	w1, [sp, 6]
	str	xzr, [sp, 40]
	strh	wzr, [sp, 38]
	ldr	x0, [sp, 8]
	add	x0, x0, 272
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
	b	.L455
.L457:
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 6]
	cmp	w1, w0
	bls	.L456
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 38]
	cmp	w1, w0
	bhi	.L456
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	strh	w0, [sp, 38]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
.L456:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
.L455:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L457
	ldr	x0, [sp, 40]
	cmp	x0, 0
	beq	.L458
	ldr	x0, [sp, 40]
	b	.L459
.L458:
	mov	x0, 0
.L459:
	add	sp, sp, 48
	ret
	.size	get_block_max_erase_time_from_free_list, .-get_block_max_erase_time_from_free_list
	.align	2
	.global	get_block_max_erase_time_from_invalid_list
	.type	get_block_max_erase_time_from_invalid_list, %function
get_block_max_erase_time_from_invalid_list:
	sub	sp, sp, #48
	str	x0, [sp, 8]
	strh	w1, [sp, 6]
	str	xzr, [sp, 40]
	strh	wzr, [sp, 38]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L461
.L463:
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 6]
	cmp	w1, w0
	bls	.L462
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 38]
	cmp	w1, w0
	bhi	.L462
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	strh	w0, [sp, 38]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
.L462:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L461:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L463
	ldr	x0, [sp, 40]
	cmp	x0, 0
	beq	.L464
	ldr	x0, [sp, 40]
	b	.L465
.L464:
	mov	x0, 0
.L465:
	add	sp, sp, 48
	ret
	.size	get_block_max_erase_time_from_invalid_list, .-get_block_max_erase_time_from_invalid_list
	.align	2
	.global	get_block_min_erase_time_from_invalid_list
	.type	get_block_min_erase_time_from_invalid_list, %function
get_block_min_erase_time_from_invalid_list:
	sub	sp, sp, #48
	str	x0, [sp, 8]
	strh	w1, [sp, 6]
	str	w2, [sp]
	str	xzr, [sp, 40]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	strh	w0, [sp, 38]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L467
.L469:
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 6]
	cmp	w1, w0
	bls	.L468
	ldr	x0, [sp, 24]
	ldr	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [sp]
	cmp	w1, w0
	bcs	.L468
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	ldrh	w1, [sp, 38]
	cmp	w1, w0
	bcc	.L468
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	strh	w0, [sp, 38]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
.L468:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L467:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L469
	ldr	x0, [sp, 40]
	cmp	x0, 0
	beq	.L470
	ldr	x0, [sp, 40]
	b	.L471
.L470:
	mov	x0, 0
.L471:
	add	sp, sp, 48
	ret
	.size	get_block_min_erase_time_from_invalid_list, .-get_block_min_erase_time_from_invalid_list
	.align	2
	.global	get_block_max_count_from_invalid_list
	.type	get_block_max_count_from_invalid_list, %function
get_block_max_count_from_invalid_list:
	sub	sp, sp, #48
	str	x0, [sp, 8]
	str	xzr, [sp, 40]
	str	wzr, [sp, 36]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L473
.L475:
	ldr	x0, [sp, 24]
	ldr	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [sp, 36]
	cmp	w1, w0
	bcc	.L474
	ldr	x0, [sp, 24]
	ldr	w0, [x0, 8]
	str	w0, [sp, 36]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
.L474:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L473:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L475
	ldr	x0, [sp, 40]
	cmp	x0, 0
	beq	.L476
	ldr	x0, [sp, 40]
	b	.L477
.L476:
	mov	x0, 0
.L477:
	add	sp, sp, 48
	ret
	.size	get_block_max_count_from_invalid_list, .-get_block_max_count_from_invalid_list
	.align	2
	.global	get_block_min_count_from_invalid_list
	.type	get_block_min_count_from_invalid_list, %function
get_block_min_count_from_invalid_list:
	sub	sp, sp, #48
	str	x0, [sp, 8]
	str	xzr, [sp, 40]
	mov	w0, -1
	str	w0, [sp, 36]
	ldr	x0, [sp, 8]
	add	x0, x0, 336
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
	b	.L479
.L481:
	ldr	x0, [sp, 24]
	ldr	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [sp, 36]
	cmp	w1, w0
	bhi	.L480
	ldr	x0, [sp, 24]
	ldr	w0, [x0, 8]
	str	w0, [sp, 36]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
.L480:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L479:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L481
	ldr	x0, [sp, 40]
	cmp	x0, 0
	beq	.L482
	ldr	x0, [sp, 40]
	b	.L483
.L482:
	mov	x0, 0
.L483:
	add	sp, sp, 48
	ret
	.size	get_block_min_count_from_invalid_list, .-get_block_min_count_from_invalid_list
	.align	2
	.global	out_phy_block_from_free_list_by_block
	.type	out_phy_block_from_free_list_by_block, %function
out_phy_block_from_free_list_by_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	del_free_list_by_block
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	beq	.L485
	ldr	x0, [x29, 40]
	str	xzr, [x0, 32]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 40]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	cmp	w0, 0
	beq	.L486
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 30]
.L486:
	ldr	x0, [x29, 40]
	b	.L487
.L485:
	ldr	x0, [x29, 40]
.L487:
	ldp	x29, x30, [sp], 48
	ret
	.size	out_phy_block_from_free_list_by_block, .-out_phy_block_from_free_list_by_block
	.section	.rodata
	.align	3
.LC91:
	.string	"[ND]add invalid_list error !\n"
	.text
	.align	2
	.global	put_phy_block_to_invalid_page_list
	.type	put_phy_block_to_invalid_page_list, %function
put_phy_block_to_invalid_page_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 16]
	str	xzr, [x0, 24]
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	add_block_invalid_list
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L489
	adrp	x0, .LC91
	add	x0, x0, :lo12:.LC91
	bl	nand_print_dbg
.L489:
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	put_phy_block_to_invalid_page_list, .-put_phy_block_to_invalid_page_list
	.align	2
	.global	out_phy_block_from_invalid_page_list
	.type	out_phy_block_from_invalid_page_list, %function
out_phy_block_from_invalid_page_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	del_block_invalid_list
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	beq	.L492
	ldr	x0, [x29, 40]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 24]
	ldr	x0, [x29, 40]
	b	.L493
.L492:
	ldr	x0, [x29, 40]
.L493:
	ldp	x29, x30, [sp], 48
	ret
	.size	out_phy_block_from_invalid_page_list, .-out_phy_block_from_invalid_page_list
	.align	2
	.global	out_phy_block_from_invalid_page_list_by_block
	.type	out_phy_block_from_invalid_page_list_by_block, %function
out_phy_block_from_invalid_page_list_by_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	del_block_invalid_list_by_block
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	beq	.L495
	ldr	x0, [x29, 40]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 24]
	ldr	x0, [x29, 40]
	b	.L496
.L495:
	ldr	x0, [x29, 40]
.L496:
	ldp	x29, x30, [sp], 48
	ret
	.size	out_phy_block_from_invalid_page_list_by_block, .-out_phy_block_from_invalid_page_list_by_block
	.align	2
	.global	out_phy_block_from_used_list_by_block
	.type	out_phy_block_from_used_list_by_block, %function
out_phy_block_from_used_list_by_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	del_block_count_by_block
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	out_phy_block_from_used_list_by_block, .-out_phy_block_from_used_list_by_block
	.section	.rodata
	.align	3
.LC92:
	.string	"[ND]invalid_page_count more than pages per blk 0 %d %d!!\n"
	.align	3
.LC93:
	.string	"[ND]invalid_page_count more than pages per blk 1!!\n"
	.text
	.align	2
	.global	phy_block_from_invalid_page_incr
	.type	phy_block_from_invalid_page_incr, %function
phy_block_from_invalid_page_incr:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L500
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	bne	.L500
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 16]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bls	.L501
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	mov	w2, w0
	adrp	x0, .LC92
	add	x0, x0, :lo12:.LC92
	bl	nand_print_dbg
.L501:
	mov	w0, 0
	b	.L502
.L500:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 16]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bls	.L503
	adrp	x0, .LC93
	add	x0, x0, :lo12:.LC93
	bl	nand_print_dbg
.L503:
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 24]
	str	x0, [x29, 40]
	ldr	x0, [x29, 16]
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	add	x1, x0, 336
	ldr	x0, [x29, 40]
	cmp	x1, x0
	bne	.L505
	mov	w0, 0
	b	.L502
.L508:
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 24]
	ldr	x0, [x29, 32]
	str	x1, [x0, 24]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x1, [x29, 32]
	str	x1, [x0, 16]
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 40]
	str	x1, [x0, 16]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	beq	.L506
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 16]
	ldr	x1, [x29, 40]
	str	x1, [x0, 24]
.L506:
	ldr	x0, [x29, 32]
	ldr	x1, [x29, 40]
	str	x1, [x0, 16]
	ldr	x0, [x29, 40]
	ldr	x1, [x29, 32]
	str	x1, [x0, 24]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	beq	.L509
.L505:
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 4]
	cmp	w1, w0
	bcc	.L508
	b	.L507
.L509:
	nop
.L507:
	mov	w0, 0
.L502:
	ldp	x29, x30, [sp], 48
	ret
	.size	phy_block_from_invalid_page_incr, .-phy_block_from_invalid_page_incr
	.align	2
	.global	adjust_invaild_list
	.type	adjust_invaild_list, %function
adjust_invaild_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	b	.L511
.L514:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bne	.L516
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_invalid_page_list
	str	x0, [x29, 40]
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_free_list
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_used_list_by_block
.L511:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 352]
	cmp	x0, 0
	bne	.L514
	b	.L513
.L516:
	nop
.L513:
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	adjust_invaild_list, .-adjust_invaild_list
	.section	.rodata
	.align	3
	.type	nand_crc16_table, %object
	.size	nand_crc16_table, 512
nand_crc16_table:
	.hword	0
	.hword	-16191
	.hword	-15999
	.hword	320
	.hword	-15615
	.hword	960
	.hword	640
	.hword	-15807
	.hword	-14847
	.hword	1728
	.hword	1920
	.hword	-14527
	.hword	1280
	.hword	-14911
	.hword	-15231
	.hword	1088
	.hword	-13311
	.hword	3264
	.hword	3456
	.hword	-12991
	.hword	3840
	.hword	-12351
	.hword	-12671
	.hword	3648
	.hword	2560
	.hword	-13631
	.hword	-13439
	.hword	2880
	.hword	-14079
	.hword	2496
	.hword	2176
	.hword	-14271
	.hword	-10239
	.hword	6336
	.hword	6528
	.hword	-9919
	.hword	6912
	.hword	-9279
	.hword	-9599
	.hword	6720
	.hword	7680
	.hword	-8511
	.hword	-8319
	.hword	8000
	.hword	-8959
	.hword	7616
	.hword	7296
	.hword	-9151
	.hword	5120
	.hword	-11071
	.hword	-10879
	.hword	5440
	.hword	-10495
	.hword	6080
	.hword	5760
	.hword	-10687
	.hword	-11775
	.hword	4800
	.hword	4992
	.hword	-11455
	.hword	4352
	.hword	-11839
	.hword	-12159
	.hword	4160
	.hword	-4095
	.hword	12480
	.hword	12672
	.hword	-3775
	.hword	13056
	.hword	-3135
	.hword	-3455
	.hword	12864
	.hword	13824
	.hword	-2367
	.hword	-2175
	.hword	14144
	.hword	-2815
	.hword	13760
	.hword	13440
	.hword	-3007
	.hword	15360
	.hword	-831
	.hword	-639
	.hword	15680
	.hword	-255
	.hword	16320
	.hword	16000
	.hword	-447
	.hword	-1535
	.hword	15040
	.hword	15232
	.hword	-1215
	.hword	14592
	.hword	-1599
	.hword	-1919
	.hword	14400
	.hword	10240
	.hword	-5951
	.hword	-5759
	.hword	10560
	.hword	-5375
	.hword	11200
	.hword	10880
	.hword	-5567
	.hword	-4607
	.hword	11968
	.hword	12160
	.hword	-4287
	.hword	11520
	.hword	-4671
	.hword	-4991
	.hword	11328
	.hword	-7167
	.hword	9408
	.hword	9600
	.hword	-6847
	.hword	9984
	.hword	-6207
	.hword	-6527
	.hword	9792
	.hword	8704
	.hword	-7487
	.hword	-7295
	.hword	9024
	.hword	-7935
	.hword	8640
	.hword	8320
	.hword	-8127
	.hword	-24575
	.hword	24768
	.hword	24960
	.hword	-24255
	.hword	25344
	.hword	-23615
	.hword	-23935
	.hword	25152
	.hword	26112
	.hword	-22847
	.hword	-22655
	.hword	26432
	.hword	-23295
	.hword	26048
	.hword	25728
	.hword	-23487
	.hword	27648
	.hword	-21311
	.hword	-21119
	.hword	27968
	.hword	-20735
	.hword	28608
	.hword	28288
	.hword	-20927
	.hword	-22015
	.hword	27328
	.hword	27520
	.hword	-21695
	.hword	26880
	.hword	-22079
	.hword	-22399
	.hword	26688
	.hword	30720
	.hword	-18239
	.hword	-18047
	.hword	31040
	.hword	-17663
	.hword	31680
	.hword	31360
	.hword	-17855
	.hword	-16895
	.hword	32448
	.hword	32640
	.hword	-16575
	.hword	32000
	.hword	-16959
	.hword	-17279
	.hword	31808
	.hword	-19455
	.hword	29888
	.hword	30080
	.hword	-19135
	.hword	30464
	.hword	-18495
	.hword	-18815
	.hword	30272
	.hword	29184
	.hword	-19775
	.hword	-19583
	.hword	29504
	.hword	-20223
	.hword	29120
	.hword	28800
	.hword	-20415
	.hword	20480
	.hword	-28479
	.hword	-28287
	.hword	20800
	.hword	-27903
	.hword	21440
	.hword	21120
	.hword	-28095
	.hword	-27135
	.hword	22208
	.hword	22400
	.hword	-26815
	.hword	21760
	.hword	-27199
	.hword	-27519
	.hword	21568
	.hword	-25599
	.hword	23744
	.hword	23936
	.hword	-25279
	.hword	24320
	.hword	-24639
	.hword	-24959
	.hword	24128
	.hword	23040
	.hword	-25919
	.hword	-25727
	.hword	23360
	.hword	-26367
	.hword	22976
	.hword	22656
	.hword	-26559
	.hword	-30719
	.hword	18624
	.hword	18816
	.hword	-30399
	.hword	19200
	.hword	-29759
	.hword	-30079
	.hword	19008
	.hword	19968
	.hword	-28991
	.hword	-28799
	.hword	20288
	.hword	-29439
	.hword	19904
	.hword	19584
	.hword	-29631
	.hword	17408
	.hword	-31551
	.hword	-31359
	.hword	17728
	.hword	-30975
	.hword	18368
	.hword	18048
	.hword	-31167
	.hword	-32255
	.hword	17088
	.hword	17280
	.hword	-31935
	.hword	16640
	.hword	-32319
	.hword	-32639
	.hword	16448
	.text
	.align	2
	.type	nand_crc16_byte, %function
nand_crc16_byte:
	sub	sp, sp, #16
	strh	w0, [sp, 14]
	strb	w1, [sp, 13]
	ldrh	w0, [sp, 14]
	lsr	w0, w0, 8
	uxth	w1, w0
	ldrh	w2, [sp, 14]
	ldrb	w0, [sp, 13]
	eor	w0, w2, w0
	and	w2, w0, 255
	adrp	x0, nand_crc16_table
	add	x0, x0, :lo12:nand_crc16_table
	sxtw	x2, w2
	ldrh	w0, [x0, x2, lsl 1]
	eor	w0, w1, w0
	uxth	w0, w0
	add	sp, sp, 16
	ret
	.size	nand_crc16_byte, .-nand_crc16_byte
	.align	2
	.global	nand_crc16
	.type	nand_crc16, %function
nand_crc16:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	strh	w0, [x29, 30]
	str	x1, [x29, 16]
	str	w2, [x29, 24]
	b	.L520
.L521:
	ldr	x0, [x29, 16]
	add	x1, x0, 1
	str	x1, [x29, 16]
	ldrb	w0, [x0]
	mov	w1, w0
	ldrh	w0, [x29, 30]
	bl	nand_crc16_byte
	strh	w0, [x29, 30]
.L520:
	ldr	w0, [x29, 24]
	sub	w1, w0, #1
	str	w1, [x29, 24]
	cmp	w0, 0
	bne	.L521
	ldrh	w0, [x29, 30]
	ldp	x29, x30, [sp], 32
	ret
	.size	nand_crc16, .-nand_crc16
	.section	.rodata
	.align	3
.LC94:
	.string	"[NE]error nand_write_logic_page_no_gc page_no:%u\n"
	.text
	.align	2
	.global	_nftl_nand_write_logic_page
	.type	_nftl_nand_write_logic_page, %function
_nftl_nand_write_logic_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	x2, [x29, 24]
	ldr	x2, [x29, 24]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	bl	nand_write_logic_page_no_gc
	cmp	w0, 0
	beq	.L524
	adrp	x0, .LC94
	add	x0, x0, :lo12:.LC94
	ldr	w1, [x29, 36]
	bl	nand_print
	mov	w0, 1
	b	.L525
.L524:
	ldr	x0, [x29, 40]
	bl	garbage_collect
	ldr	x0, [x29, 40]
	bl	all_time_gc
	ldr	x0, [x29, 40]
	bl	do_prio_gc
	mov	w0, 0
.L525:
	ldp	x29, x30, [sp], 48
	ret
	.size	_nftl_nand_write_logic_page, .-_nftl_nand_write_logic_page
	.section	.rodata
	.align	3
.LC95:
	.string	"[ND]logic_page_map_temp NULL!!\n"
	.align	3
.LC96:
	.string	"[NE]read a page: %d READ_RECLAIM!\n"
	.text
	.align	2
	.global	_nftl_nand_read_logic_page
	.type	_nftl_nand_read_logic_page, %function
_nftl_nand_read_logic_page:
	stp	x29, x30, [sp, -112]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	x2, [x29, 24]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	bl	get_logic_page_map
	str	x0, [x29, 104]
	ldr	x0, [x29, 104]
	cmp	x0, 0
	bne	.L527
	adrp	x0, .LC95
	add	x0, x0, :lo12:.LC95
	bl	nand_print_dbg
	mov	w0, 1
	b	.L531
.L527:
	ldr	x0, [x29, 104]
	ldrh	w1, [x0, 2]
	mov	w0, 65535
	cmp	w1, w0
	bne	.L529
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	nand_memset
	mov	w0, 0
	b	.L531
.L529:
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 664]
	uxtb	w0, w0
	and	w0, w0, 1
	uxtb	w2, w0
	ldr	x1, [x29, 104]
	ldrb	w0, [x1, 1]
	bfi	w0, w2, 4, 1
	strb	w0, [x1, 1]
	ldr	x0, [x29, 104]
	ldrh	w1, [x0, 2]
	ldr	x0, [x29, 104]
	ldrh	w0, [x0]
	ubfx	x0, x0, 0, 12
	uxth	w0, w0
	mov	w6, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x3, x29, 72
	add	x0, x29, 48
	mov	x5, x3
	ldr	x4, [x29, 24]
	mov	w3, w2
	mov	w2, w6
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3472]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 100]
	ldr	w0, [x29, 100]
	cmn	w0, #2
	bne	.L530
	ldr	x0, [x29, 104]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	get_phy_block_addr
	str	x0, [x29, 88]
	mov	w2, 1
	ldr	x1, [x29, 88]
	ldr	x0, [x29, 40]
	bl	add_prio_gc
	adrp	x0, .LC96
	add	x0, x0, :lo12:.LC96
	ldr	w1, [x29, 36]
	bl	nand_print
.L530:
	ldr	x0, [x29, 40]
	bl	do_prio_gc
	ldr	w0, [x29, 100]
.L531:
	ldp	x29, x30, [sp], 112
	ret
	.size	_nftl_nand_read_logic_page, .-_nftl_nand_read_logic_page
	.align	2
	.global	_nftl_discard_logic_page
	.type	_nftl_discard_logic_page, %function
_nftl_discard_logic_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	mov	x3, 0
	ldr	w2, [x29, 20]
	mov	w1, 4369
	movk	w1, 0xaa11, lsl 16
	ldr	x0, [x29, 24]
	bl	write_data_to_page
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	_nftl_discard_logic_page, .-_nftl_discard_logic_page
	.section	.rodata
	.align	3
.LC97:
	.string	"[NE]no free block\n"
	.align	3
.LC98:
	.string	"[NE]do_write_error erase error sb:%d!\n"
	.align	3
.LC99:
	.string	"[NE]do_write_error sb@%d nftl_nand_copy_page error!\n"
	.text
	.align	2
	.global	do_write_error
	.type	do_write_error, %function
do_write_error:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
.L535:
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 80]
	ldr	x0, [x29, 80]
	cmp	x0, 0
	bne	.L536
	adrp	x0, .LC97
	add	x0, x0, :lo12:.LC97
	bl	nand_print
	mov	w0, 1
	b	.L548
.L536:
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 40
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 76]
	ldr	x0, [x29, 80]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 80]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 80]
	strh	w1, [x0, 6]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L538
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 80]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC98
	add	x0, x0, :lo12:.LC98
	bl	nand_print
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	lsr	w0, w0, 2
	uxth	w0, w0
	cmp	w1, w0
	bls	.L539
	b	.L535
.L539:
	adrp	x0, .LC97
	add	x0, x0, :lo12:.LC97
	bl	nand_print
	mov	w0, 1
	b	.L548
.L538:
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldr	w0, [x0, 8]
	add	w1, w0, 1
	ldr	x0, [x29, 80]
	str	w1, [x0, 8]
	str	wzr, [x29, 92]
	b	.L540
.L544:
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 24]
	ldr	w0, [x29, 92]
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [x29, 72]
	ldr	w0, [x29, 72]
	cmn	w0, #1
	beq	.L549
	ldr	x0, [x29, 24]
	ldr	x5, [x0, 3488]
	ldr	x0, [x29, 16]
	ldr	x1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 512]
	ldr	w2, [x29, 92]
	uxth	w2, w2
	mov	w4, w2
	mov	x3, x0
	ldr	x2, [x29, 80]
	ldr	x0, [x29, 24]
	blr	x5
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L542
	ldr	x0, [x29, 80]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC99
	add	x0, x0, :lo12:.LC99
	bl	nand_print
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 40
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	b	.L535
.L542:
	ldr	w0, [x29, 92]
	add	w0, w0, 1
	str	w0, [x29, 92]
.L540:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	ldr	w0, [x29, 92]
	cmp	w1, w0
	bhi	.L544
	b	.L543
.L549:
	nop
.L543:
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 80]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 40
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 16]
	ldr	x1, [x0]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x0, [x29, 24]
	bl	erase_block
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w1, [x0, 2]
	ldr	x0, [x29, 80]
	strh	w1, [x0, 2]
	ldr	x0, [x29, 16]
	ldr	x1, [x29, 80]
	str	x1, [x0]
	str	wzr, [x29, 92]
	b	.L545
.L547:
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 24]
	ldr	w0, [x29, 92]
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [x29, 72]
	ldr	w0, [x29, 72]
	cmn	w0, #1
	beq	.L546
	ldr	w1, [x29, 72]
	ldr	x0, [x29, 24]
	bl	get_logic_page_map
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L546
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w1, [x0]
	ldr	x0, [x29, 64]
	strh	w1, [x0, 2]
	ldr	w0, [x29, 92]
	uxth	w0, w0
	and	w0, w0, 4095
	uxth	w2, w0
	ldr	x1, [x29, 64]
	ldrh	w0, [x1]
	bfi	w0, w2, 0, 12
	strh	w0, [x1]
.L546:
	ldr	w0, [x29, 92]
	add	w0, w0, 1
	str	w0, [x29, 92]
.L545:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	ldr	w0, [x29, 92]
	cmp	w1, w0
	bhi	.L547
	mov	w0, 0
.L548:
	ldp	x29, x30, [sp], 96
	ret
	.size	do_write_error, .-do_write_error
	.section	.rodata
	.align	3
.LC100:
	.string	"[NE]do after write error1!\n"
	.align	3
.LC101:
	.string	"[NE]do_before_write error!\n"
	.align	3
.LC102:
	.string	"[NE]write data to page error!\n"
	.align	3
.LC103:
	.string	"[NE]do after write error2!\n"
	.text
	.align	2
	.global	nand_write_data_page
	.type	nand_write_data_page, %function
nand_write_data_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	ldr	x0, [x29, 40]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	current_block_at_last_page
	cmp	w0, 1
	bne	.L551
	ldr	x0, [x29, 40]
	bl	do_after_write
	cmp	w0, 0
	beq	.L551
	adrp	x0, .LC100
	add	x0, x0, :lo12:.LC100
	bl	nand_print
	mov	w0, 1
	b	.L552
.L551:
	ldr	x0, [x29, 40]
	bl	do_before_write
	cmp	w0, 0
	beq	.L553
	adrp	x0, .LC101
	add	x0, x0, :lo12:.LC101
	bl	nand_print
	mov	w0, 1
	b	.L552
.L553:
	ldr	x3, [x29, 24]
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	bl	write_data_to_page
	cmp	w0, 0
	beq	.L554
	adrp	x0, .LC102
	add	x0, x0, :lo12:.LC102
	bl	nand_print
	mov	w0, 1
	b	.L552
.L554:
	ldr	x0, [x29, 40]
	bl	do_after_write
	cmp	w0, 0
	beq	.L555
	adrp	x0, .LC103
	add	x0, x0, :lo12:.LC103
	bl	nand_print
	mov	w0, 1
	b	.L552
.L555:
	mov	w0, 0
.L552:
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_write_data_page, .-nand_write_data_page
	.align	2
	.global	nand_write_logic_page_no_gc
	.type	nand_write_logic_page_no_gc, %function
nand_write_logic_page_no_gc:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	x2, [x29, 24]
	ldr	x3, [x29, 24]
	ldr	w2, [x29, 36]
	mov	w1, 192
	ldr	x0, [x29, 40]
	bl	nand_write_data_page
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	nand_write_logic_page_no_gc, .-nand_write_logic_page_no_gc
	.section	.rodata
	.align	3
.LC104:
	.string	"[NE]get_new_current_block error3!\n"
	.align	3
.LC105:
	.string	"[NE]get_new_current_block error4!\n"
	.text
	.align	2
	.global	do_before_write
	.type	do_before_write, %function
do_before_write:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	b	.L559
.L564:
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 528]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 24]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L570
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldr	w1, [x0, 8]
	sub	w1, w1, #1
	str	w1, [x0, 8]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L561
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldr	w1, [x0, 8]
	sub	w1, w1, #1
	str	w1, [x0, 8]
.L561:
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_new_current_block
	cmp	w0, 0
	beq	.L559
	adrp	x0, .LC104
	add	x0, x0, :lo12:.LC104
	bl	nand_print
	mov	w0, 1
	b	.L562
.L559:
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	current_block_at_first_page
	cmp	w0, 1
	beq	.L564
	b	.L563
.L570:
	nop
.L563:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L565
	b	.L566
.L569:
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 592]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 24]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L571
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 592]
	ldr	w1, [x0, 8]
	sub	w1, w1, #1
	str	w1, [x0, 8]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L568
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 592]
	ldr	w1, [x0, 8]
	sub	w1, w1, #1
	str	w1, [x0, 8]
.L568:
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_new_current_block
	cmp	w0, 0
	beq	.L566
	adrp	x0, .LC105
	add	x0, x0, :lo12:.LC105
	bl	nand_print
	mov	w0, 1
	b	.L562
.L566:
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	current_block_at_first_page
	cmp	w0, 0
	bne	.L569
	b	.L565
.L571:
	nop
.L565:
	mov	w0, 0
.L562:
	ldp	x29, x30, [sp], 32
	ret
	.size	do_before_write, .-do_before_write
	.section	.rodata
	.align	3
.LC106:
	.string	"[NE]write_phy_page_map_to_current_block error!\n"
	.align	3
.LC107:
	.string	"[NE]get_new_current_block error1!\n"
	.align	3
.LC108:
	.string	"[NE]get_new_current_block error2!\n"
	.text
	.align	2
	.global	do_after_write
	.type	do_after_write, %function
do_after_write:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	current_block_at_last_page
	cmp	w0, 1
	bne	.L573
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	write_phy_page_map_to_current_block
	cmp	w0, 0
	beq	.L574
	adrp	x0, .LC106
	add	x0, x0, :lo12:.LC106
	bl	nand_print
	mov	w0, 1
	b	.L575
.L574:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L576
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	write_phy_page_map_to_current_block
	cmp	w0, 0
	beq	.L576
	adrp	x0, .LC106
	add	x0, x0, :lo12:.LC106
	bl	nand_print
.L576:
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_new_current_block
	cmp	w0, 0
	beq	.L577
	adrp	x0, .LC107
	add	x0, x0, :lo12:.LC107
	bl	nand_print
	mov	w0, 1
	b	.L575
.L577:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L578
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_new_current_block
	cmp	w0, 0
	beq	.L578
	adrp	x0, .LC108
	add	x0, x0, :lo12:.LC108
	bl	nand_print
	mov	w0, 1
	b	.L575
.L578:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 668]
	add	w1, w0, 1
	ldr	x0, [x29, 24]
	str	w1, [x0, 668]
.L573:
	mov	w0, 0
.L575:
	ldp	x29, x30, [sp], 32
	ret
	.size	do_after_write, .-do_after_write
	.section	.rodata
	.align	3
.LC109:
	.string	"[NE]write logic data to current block error\n"
	.text
	.align	2
	.global	write_data_to_page
	.type	write_data_to_page, %function
write_data_to_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	ldr	x0, [x29, 40]
	add	x0, x0, 528
	ldr	x4, [x29, 24]
	ldr	w3, [x29, 32]
	ldr	w2, [x29, 36]
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	write_logic_data_to_current_block
	cmp	w0, 0
	beq	.L580
	adrp	x0, .LC109
	add	x0, x0, :lo12:.LC109
	bl	nand_print
	mov	w0, 1
	b	.L581
.L580:
	ldr	x0, [x29, 40]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L582
	ldr	x0, [x29, 40]
	add	x0, x0, 592
	ldr	x4, [x29, 24]
	ldr	w3, [x29, 32]
	ldr	w2, [x29, 36]
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	write_logic_data_to_current_block
	cmp	w0, 0
	beq	.L582
	adrp	x0, .LC109
	add	x0, x0, :lo12:.LC109
	bl	nand_print
	mov	w0, 1
	b	.L581
.L582:
	mov	w0, 0
.L581:
	ldp	x29, x30, [sp], 48
	ret
	.size	write_data_to_page, .-write_data_to_page
	.section	.rodata
	.align	3
.LC110:
	.string	"[ND]last block can not used to logic page %d!!\n"
	.align	3
.LC111:
	.string	"[ND]last block can not used to power down page %d!!\n"
	.align	3
.LC112:
	.string	"[ND]write_phy_data_to_current_block write error %d!\n"
	.align	3
.LC113:
	.string	"[NE]do_write_error error\n"
	.text
	.align	2
	.global	write_phy_data_to_current_block
	.type	write_phy_data_to_current_block, %function
write_phy_data_to_current_block:
	stp	x29, x30, [sp, -112]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	w4, [x29, 16]
	strh	wzr, [x29, 104]
	strh	wzr, [x29, 106]
	add	x0, x29, 80
	mov	w2, 16
	mov	w1, -91
	bl	nand_memset
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w0, w0
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 106]
	ldr	w1, [x29, 16]
	mov	w0, -1431633921
	cmp	w1, w0
	bne	.L584
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 560]
	strh	w0, [x29, 106]
.L584:
	ldrh	w0, [x29, 106]
	mov	w2, w0
	ldr	x1, [x29, 24]
	mov	w0, -1
	bl	nand_crc16
	strh	w0, [x29, 104]
	ldrh	w0, [x29, 104]
	lsr	w0, w0, 8
	uxth	w0, w0
	uxtb	w0, w0
	strb	w0, [x29, 92]
	ldrh	w0, [x29, 104]
	uxtb	w0, w0
	strb	w0, [x29, 93]
	str	wzr, [x29, 108]
.L585:
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w1, [x0]
	ldr	x0, [x29, 32]
	ldrh	w2, [x0, 56]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x4, x29, 80
	add	x0, x29, 56
	mov	x5, x4
	ldr	x4, [x29, 24]
	bl	set_physic_op_par
	ldr	w1, [x29, 16]
	mov	w0, -1431633921
	cmp	w1, w0
	bne	.L586
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w1, [x0, 6]
	add	x0, x29, 80
	mov	w4, w1
	mov	w3, w2
	mov	w2, -1431633921
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_oob_special_page
	b	.L587
.L586:
	ldr	w0, [x29, 16]
	cmp	w0, 192
	bne	.L588
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	cmp	w1, w0
	bne	.L589
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC110
	add	x0, x0, :lo12:.LC110
	bl	nand_print_dbg
.L589:
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w1, [x0, 6]
	add	x0, x29, 80
	mov	w4, w1
	mov	w3, w2
	ldr	w2, [x29, 20]
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_oob_logic_page
	b	.L587
.L588:
	ldr	w1, [x29, 16]
	mov	w0, 13107
	movk	w0, 0xaa11, lsl 16
	cmp	w1, w0
	bne	.L587
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	cmp	w1, w0
	bne	.L590
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC111
	add	x0, x0, :lo12:.LC111
	bl	nand_print_dbg
.L590:
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w1, [x0, 6]
	add	x0, x29, 80
	mov	w4, w1
	mov	w3, w2
	mov	w2, 13107
	movk	w2, 0xaa11, lsl 16
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_oob_special_page
.L587:
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	mov	w1, 1
	strh	w1, [x0, 2]
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3480]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 100]
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L591
	ldr	w0, [x29, 108]
	cmp	w0, 1
	bgt	.L591
	ldrh	w0, [x29, 58]
	mov	w1, w0
	adrp	x0, .LC112
	add	x0, x0, :lo12:.LC112
	bl	nand_print_dbg
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	do_write_error
	cmp	w0, 0
	bne	.L592
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
	b	.L585
.L592:
	adrp	x0, .LC113
	add	x0, x0, :lo12:.LC113
	bl	nand_print
	mov	w0, 1
	b	.L594
.L591:
	mov	w0, 0
.L594:
	ldp	x29, x30, [sp], 112
	ret
	.size	write_phy_data_to_current_block, .-write_phy_data_to_current_block
	.section	.rodata
	.align	3
.LC114:
	.string	"[NE]write_phy_data_to_current_block error1\n"
	.text
	.align	2
	.global	write_phy_page_map_to_current_block
	.type	write_phy_page_map_to_current_block, %function
write_phy_page_map_to_current_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	add	x1, x0, 592
	ldr	x0, [x29, 16]
	cmp	x1, x0
	bne	.L596
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 632]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 568]
	mov	w2, 320
	mov	x1, x0
	mov	x0, x3
	bl	nand_memcpy
.L596:
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 8]
	mov	w4, -1431633921
	mov	w3, 0
	mov	x2, x0
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	write_phy_data_to_current_block
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L597
	adrp	x0, .LC114
	add	x0, x0, :lo12:.LC114
	bl	nand_print
	ldr	w0, [x29, 44]
	b	.L598
.L597:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 56]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 16]
	strh	w1, [x0, 56]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w1, [x0, 4]
	add	w1, w1, 1
	uxth	w1, w1
	strh	w1, [x0, 4]
	ldr	x0, [x29, 16]
	add	x1, x0, 8
	ldr	x0, [x29, 16]
	ldr	x2, [x0]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 56]
	mov	w3, w0
	ldr	x0, [x29, 24]
	bl	check_invalid_page_right
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 56]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bne	.L599
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_invalid_page_list
	ldr	x0, [x29, 24]
	bl	adjust_invaild_list
.L599:
	mov	w0, 0
.L598:
	ldp	x29, x30, [sp], 48
	ret
	.size	write_phy_page_map_to_current_block, .-write_phy_page_map_to_current_block
	.section	.rodata
	.align	3
.LC115:
	.string	"[ND]logic_page_map NULL!!\n"
	.align	3
.LC116:
	.string	"[ND]invalid_page_count more than pages_per_blk at write logic data to current block:%d!!\n"
	.align	3
.LC117:
	.string	"[NE]write_phy_data_to_current_block error2\n"
	.align	3
.LC118:
	.string	"[NE]invalid_page_count more than pages_per_blk at write logic data to current block:%d!!\n"
	.align	3
.LC119:
	.string	"[NE]write phy data to current block error2\n"
	.text
	.align	2
	.global	write_logic_data_to_current_block
	.type	write_logic_data_to_current_block, %function
write_logic_data_to_current_block:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	str	w3, [x29, 24]
	str	x4, [x29, 16]
	str	wzr, [x29, 76]
	ldr	w0, [x29, 28]
	cmp	w0, 192
	bne	.L601
	ldr	w1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	get_logic_page_map
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L602
	adrp	x0, .LC115
	add	x0, x0, :lo12:.LC115
	bl	nand_print_dbg
	mov	w0, 1
	b	.L603
.L602:
	ldr	x0, [x29, 56]
	ldrh	w1, [x0, 2]
	mov	w0, 65535
	cmp	w1, w0
	beq	.L604
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	get_phy_block_addr
	str	x0, [x29, 64]
	ldr	x0, [x29, 32]
	ldr	x1, [x0]
	ldr	x0, [x29, 64]
	cmp	x1, x0
	bne	.L605
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 64]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bls	.L604
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	adrp	x0, .LC116
	add	x0, x0, :lo12:.LC116
	bl	nand_print_dbg
	b	.L604
.L605:
	mov	w0, 1
	str	w0, [x29, 76]
.L604:
	mov	w4, 192
	ldr	w3, [x29, 24]
	ldr	x2, [x29, 16]
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	write_phy_data_to_current_block
	str	w0, [x29, 52]
	ldr	w0, [x29, 52]
	cmp	w0, 0
	beq	.L607
	adrp	x0, .LC117
	add	x0, x0, :lo12:.LC117
	bl	nand_print
	ldr	w0, [x29, 52]
	b	.L603
.L607:
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w1, [x0]
	ldr	x0, [x29, 56]
	strh	w1, [x0, 2]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	and	w0, w0, 4095
	uxth	w2, w0
	ldr	x1, [x29, 56]
	ldrh	w0, [x1]
	bfi	w0, w2, 0, 12
	strh	w0, [x1]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 664]
	cmp	w0, 0
	cset	w0, eq
	uxtb	w2, w0
	ldr	x1, [x29, 56]
	ldrb	w0, [x1, 1]
	bfi	w0, w2, 4, 1
	strb	w0, [x1, 1]
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 24]
	ldr	x0, [x29, 56]
	ldrh	w0, [x0]
	ubfx	x0, x0, 0, 12
	uxth	w0, w0
	uxth	x0, w0
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w1, [x29, 24]
	str	w1, [x0]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 32]
	strh	w1, [x0, 56]
	b	.L608
.L601:
	ldr	w1, [x29, 28]
	mov	w0, 4369
	movk	w0, 0xaa11, lsl 16
	cmp	w1, w0
	bne	.L609
	ldr	w1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	get_logic_page_map
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L610
	adrp	x0, .LC115
	add	x0, x0, :lo12:.LC115
	bl	nand_print_dbg
	mov	w0, 1
	b	.L603
.L610:
	ldr	x0, [x29, 56]
	ldrh	w1, [x0, 2]
	mov	w0, 65535
	cmp	w1, w0
	beq	.L611
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	get_phy_block_addr
	str	x0, [x29, 64]
	ldr	x0, [x29, 32]
	ldr	x1, [x0]
	ldr	x0, [x29, 64]
	cmp	x1, x0
	bne	.L612
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 24]
	ldr	x0, [x29, 56]
	ldrh	w0, [x0]
	ubfx	x0, x0, 0, 12
	uxth	w0, w0
	uxth	x0, w0
	lsl	x0, x0, 2
	add	x0, x1, x0
	mov	w1, -1
	str	w1, [x0]
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 64]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bls	.L611
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	adrp	x0, .LC118
	add	x0, x0, :lo12:.LC118
	bl	nand_print
	b	.L611
.L612:
	mov	w0, 1
	str	w0, [x29, 76]
.L611:
	ldr	x0, [x29, 56]
	mov	w1, -1
	strh	w1, [x0, 2]
	ldr	x0, [x29, 56]
	ldrh	w1, [x0]
	orr	w1, w1, 4095
	strh	w1, [x0]
	ldr	x0, [x29, 56]
	ldrb	w1, [x0, 1]
	orr	w1, w1, 16
	strb	w1, [x0, 1]
	b	.L608
.L609:
	ldr	w1, [x29, 28]
	mov	w0, 13107
	movk	w0, 0xaa11, lsl 16
	cmp	w1, w0
	bne	.L608
	ldr	x0, [x29, 32]
	ldr	x0, [x0]
	ldrh	w1, [x0, 4]
	add	w1, w1, 1
	uxth	w1, w1
	strh	w1, [x0, 4]
	mov	w4, 13107
	movk	w4, 0xaa11, lsl 16
	ldr	w3, [x29, 24]
	ldr	x2, [x29, 16]
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	write_phy_data_to_current_block
	str	w0, [x29, 52]
	ldr	w0, [x29, 52]
	cmp	w0, 0
	beq	.L614
	adrp	x0, .LC119
	add	x0, x0, :lo12:.LC119
	bl	nand_print
	ldr	w0, [x29, 52]
	b	.L603
.L614:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 56]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 32]
	strh	w1, [x0, 56]
.L608:
	ldr	w0, [x29, 76]
	cmp	w0, 1
	bne	.L615
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 40]
	bl	phy_block_from_invalid_page_incr
.L615:
	ldr	x0, [x29, 40]
	bl	adjust_invaild_list
	mov	w0, 0
.L603:
	ldp	x29, x30, [sp], 80
	ret
	.size	write_logic_data_to_current_block, .-write_logic_data_to_current_block
	.section	.rodata
	.align	3
.LC120:
	.string	"[ND]swl: new_cur_blk->alloc ok(max_ec): block_no = %d; ec = %d; \n"
	.align	3
.LC121:
	.string	"[NE]no free block!!!!\n"
	.text
	.align	2
	.global	get_new_current_block
	.type	get_new_current_block, %function
get_new_current_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldr	w0, [x0, 8]
	str	w0, [x29, 44]
	ldr	x0, [x29, 16]
	ldr	x3, [x0, 24]
	ldr	x0, [x29, 16]
	ldr	w0, [x0, 32]
	mov	w2, w0
	mov	w1, -1
	mov	x0, x3
	bl	nand_memset
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 1062]
	cmp	w0, 0
	beq	.L617
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	mov	w1, w0
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list_by_erase_time
	mov	x1, x0
	ldr	x0, [x29, 16]
	str	x1, [x0]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	cmp	x0, 0
	beq	.L619
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC120
	add	x0, x0, :lo12:.LC120
	bl	nand_print_dbg
	b	.L619
.L617:
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	mov	x1, x0
	ldr	x0, [x29, 16]
	str	x1, [x0]
.L619:
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	cmp	x0, 0
	bne	.L620
	adrp	x0, .LC121
	add	x0, x0, :lo12:.LC121
	bl	nand_print
	mov	w0, 1
	b	.L621
.L620:
	ldr	x0, [x29, 16]
	strh	wzr, [x0, 56]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldr	w1, [x29, 44]
	add	w1, w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L622
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	ldr	w1, [x0, 8]
	add	w1, w1, 1
	str	w1, [x0, 8]
.L622:
	mov	w0, 0
.L621:
	ldp	x29, x30, [sp], 48
	ret
	.size	get_new_current_block, .-get_new_current_block
	.section	.rodata
	.align	3
.LC122:
	.string	"[NE]new_block_init_for_write erase error!\n"
	.text
	.align	2
	.global	new_block_init_for_write
	.type	new_block_init_for_write, %function
new_block_init_for_write:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 48
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3464]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 76]
	ldr	x0, [x29, 32]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 32]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 676]
	cmp	w1, w0
	bls	.L624
	ldr	x0, [x29, 32]
	ldrh	w1, [x0, 6]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	cmp	w1, w0
	bcs	.L624
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	ldr	x0, [x29, 40]
	str	w1, [x0, 676]
.L624:
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 32]
	str	w1, [x0, 8]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L625
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3504]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	adrp	x0, .LC122
	add	x0, x0, :lo12:.LC122
	bl	nand_print
	mov	w0, 1
	b	.L627
.L625:
	mov	w0, 0
.L627:
	ldp	x29, x30, [sp], 80
	ret
	.size	new_block_init_for_write, .-new_block_init_for_write
	.section	.rodata
	.align	3
.LC123:
	.string	"[NE]invaild_page num error:[%d] [%d]  used_page:[%d]!!\n"
	.text
	.align	2
	.global	check_invalid_page_right
	.type	check_invalid_page_right, %function
check_invalid_page_right:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	strh	w3, [x29, 22]
	str	wzr, [x29, 72]
	str	wzr, [x29, 76]
	b	.L629
.L635:
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 16]
	ldr	w0, [x29, 76]
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [x29, 68]
	ldr	w0, [x29, 68]
	cmn	w0, #1
	beq	.L630
	ldr	w1, [x29, 68]
	ldr	x0, [x29, 40]
	bl	get_logic_page_map
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L631
	adrp	x0, .LC95
	add	x0, x0, :lo12:.LC95
	bl	nand_print_dbg
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
	b	.L632
.L631:
	ldr	x0, [x29, 56]
	ldrh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0]
	cmp	w1, w0
	bne	.L633
	ldr	x0, [x29, 56]
	ldrh	w0, [x0]
	ubfx	x0, x0, 0, 12
	uxth	w0, w0
	mov	w1, w0
	ldr	w0, [x29, 76]
	cmp	w1, w0
	beq	.L632
.L633:
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
	b	.L632
.L630:
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
.L632:
	ldr	w0, [x29, 76]
	add	w0, w0, 1
	str	w0, [x29, 76]
.L629:
	ldrh	w1, [x29, 22]
	ldr	w0, [x29, 76]
	cmp	w1, w0
	bhi	.L635
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	w0, [x29, 72]
	cmp	w1, w0
	beq	.L636
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 4]
	mov	w2, w0
	ldrh	w1, [x29, 22]
	adrp	x0, .LC123
	add	x0, x0, :lo12:.LC123
	mov	w3, w1
	ldr	w1, [x29, 72]
	bl	nand_print
	mov	w0, 1
	b	.L637
.L636:
	mov	w0, 0
.L637:
	ldp	x29, x30, [sp], 80
	ret
	.size	check_invalid_page_right, .-check_invalid_page_right
	.align	2
	.global	nftl_ops_init
	.type	nftl_ops_init, %function
nftl_ops_init:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_erase_superblk
	add	x1, x1, :lo12:_nftl_nand_erase_superblk
	str	x1, [x0, 3464]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_read_page
	add	x1, x1, :lo12:_nftl_nand_read_page
	str	x1, [x0, 3472]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_write_page
	add	x1, x1, :lo12:_nftl_nand_write_page
	str	x1, [x0, 3480]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_is_blk_good
	add	x1, x1, :lo12:_nftl_nand_is_blk_good
	str	x1, [x0, 3496]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_mark_bad_blk
	add	x1, x1, :lo12:_nftl_nand_mark_bad_blk
	str	x1, [x0, 3504]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_copy_page
	add	x1, x1, :lo12:_nftl_nand_copy_page
	str	x1, [x0, 3488]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_read_logic_page
	add	x1, x1, :lo12:_nftl_nand_read_logic_page
	str	x1, [x0, 3520]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_nand_write_logic_page
	add	x1, x1, :lo12:_nftl_nand_write_logic_page
	str	x1, [x0, 3512]
	ldr	x0, [sp, 8]
	adrp	x1, _nftl_discard_logic_page
	add	x1, x1, :lo12:_nftl_discard_logic_page
	str	x1, [x0, 3528]
	nop
	add	sp, sp, 16
	ret
	.size	nftl_ops_init, .-nftl_ops_init
	.align	2
	.global	current_block_at_last_page
	.type	current_block_at_last_page, %function
current_block_at_last_page:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp]
	ldrh	w0, [x0, 56]
	mov	w1, w0
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	cmp	w1, w0
	bne	.L641
	mov	w0, 1
	b	.L642
.L641:
	mov	w0, 0
.L642:
	add	sp, sp, 16
	ret
	.size	current_block_at_last_page, .-current_block_at_last_page
	.align	2
	.global	current_block_at_first_page
	.type	current_block_at_first_page, %function
current_block_at_first_page:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp]
	ldrh	w0, [x0, 56]
	cmp	w0, 0
	bne	.L644
	mov	w0, 1
	b	.L645
.L644:
	mov	w0, 0
.L645:
	add	sp, sp, 16
	ret
	.size	current_block_at_first_page, .-current_block_at_first_page
	.align	2
	.global	fill_no_use_data_to_page
	.type	fill_no_use_data_to_page, %function
fill_no_use_data_to_page:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	current_block_at_last_page
	cmp	w0, 0
	beq	.L647
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	write_phy_page_map_to_current_block
	cmp	w0, 0
	beq	.L648
	adrp	x0, .LC106
	add	x0, x0, :lo12:.LC106
	bl	nand_print
	mov	w0, 1
	b	.L649
.L648:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L650
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	write_phy_page_map_to_current_block
	cmp	w0, 0
	beq	.L650
	adrp	x0, .LC106
	add	x0, x0, :lo12:.LC106
	bl	nand_print
.L650:
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_new_current_block
	cmp	w0, 0
	beq	.L651
	adrp	x0, .LC107
	add	x0, x0, :lo12:.LC107
	bl	nand_print
	mov	w0, 1
	b	.L649
.L651:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L652
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_new_current_block
	cmp	w0, 0
	beq	.L652
	adrp	x0, .LC108
	add	x0, x0, :lo12:.LC108
	bl	nand_print
	mov	w0, 1
	b	.L649
.L652:
	mov	w0, 0
	b	.L649
.L647:
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	current_block_at_first_page
	cmp	w0, 0
	beq	.L653
	mov	w0, 0
	b	.L649
.L653:
	ldr	x0, [x29, 24]
	add	x0, x0, 528
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	write_phy_page_map_to_current_block
	cmp	w0, 0
	beq	.L654
	adrp	x0, .LC106
	add	x0, x0, :lo12:.LC106
	bl	nand_print
.L654:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L655
	ldr	x0, [x29, 24]
	add	x0, x0, 592
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	write_phy_page_map_to_current_block
	cmp	w0, 0
	beq	.L655
	adrp	x0, .LC106
	add	x0, x0, :lo12:.LC106
	bl	nand_print
.L655:
	mov	w0, 8
.L649:
	ldp	x29, x30, [sp], 32
	ret
	.size	fill_no_use_data_to_page, .-fill_no_use_data_to_page
	.align	2
	.global	fill_no_use_data_to_current_block
	.type	fill_no_use_data_to_current_block, %function
fill_no_use_data_to_current_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	b	.L657
.L660:
	ldr	x0, [x29, 24]
	bl	fill_no_use_data_to_page
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L658
	mov	w0, 0
	b	.L659
.L658:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L657:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L660
	mov	w0, 1
.L659:
	ldp	x29, x30, [sp], 48
	ret
	.size	fill_no_use_data_to_current_block, .-fill_no_use_data_to_current_block
	.align	2
	.global	fill_no_use_page_to_current_block
	.type	fill_no_use_page_to_current_block, %function
fill_no_use_page_to_current_block:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	wzr, [x29, 44]
	b	.L662
.L665:
	ldr	x0, [x29, 24]
	bl	fill_no_use_data_to_page
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L663
	mov	w0, 0
	b	.L664
.L663:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L662:
	ldr	w1, [x29, 44]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	blt	.L665
	mov	w0, 1
.L664:
	ldp	x29, x30, [sp], 48
	ret
	.size	fill_no_use_page_to_current_block, .-fill_no_use_page_to_current_block
	.align	2
	.global	nand_before_shutdown
	.type	nand_before_shutdown, %function
nand_before_shutdown:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w1, 8
	ldr	x0, [x29, 24]
	bl	fill_no_use_page_to_current_block
	ldp	x29, x30, [sp], 32
	ret
	.size	nand_before_shutdown, .-nand_before_shutdown
	.section	.rodata
	.align	3
.LC124:
	.string	"\n"
	.align	3
.LC125:
	.string	"%08x: "
	.align	3
.LC126:
	.string	"%02x "
	.text
	.align	2
	.global	dump_buf
	.type	dump_buf, %function
dump_buf:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	wzr, [x29, 44]
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print
	str	wzr, [x29, 44]
	b	.L669
.L672:
	ldr	w0, [x29, 44]
	and	w0, w0, 15
	cmp	w0, 0
	bne	.L670
	adrp	x0, .LC125
	add	x0, x0, :lo12:.LC125
	ldr	w1, [x29, 44]
	bl	nand_print
.L670:
	ldrsw	x0, [x29, 44]
	ldr	x1, [x29, 24]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC126
	add	x0, x0, :lo12:.LC126
	bl	nand_print
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	and	w0, w0, 15
	cmp	w0, 0
	bne	.L671
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print
.L671:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L669:
	ldr	w1, [x29, 44]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L672
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	dump_buf, .-dump_buf
	.section	.rodata
	.align	3
.LC127:
	.string	"[NE]exception case wb %04x %04x, len: %d\n"
	.text
	.align	2
	.type	crc_wrong, %function
crc_wrong:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	strh	wzr, [x29, 58]
	strh	wzr, [x29, 56]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	lsl	w0, w0, 9
	str	w0, [x29, 60]
	ldr	x0, [x29, 24]
	bl	is_nouse_page
	cmp	w0, 0
	beq	.L674
	mov	w0, 0
	b	.L675
.L674:
	ldr	x0, [x29, 24]
	bl	is_phy_mapping_page
	cmp	w0, 0
	beq	.L676
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 560]
	str	w0, [x29, 60]
.L676:
	ldr	w2, [x29, 60]
	ldr	x1, [x29, 32]
	mov	w0, -1
	bl	nand_crc16
	strh	w0, [x29, 58]
	ldr	x0, [x29, 24]
	add	x0, x0, 13
	ldrb	w0, [x0]
	sxth	w1, w0
	ldr	x0, [x29, 24]
	add	x0, x0, 12
	ldrb	w0, [x0]
	lsl	w0, w0, 8
	sxth	w0, w0
	orr	w0, w1, w0
	sxth	w0, w0
	strh	w0, [x29, 56]
	ldrh	w1, [x29, 58]
	ldrh	w0, [x29, 56]
	cmp	w1, w0
	beq	.L677
	ldrh	w1, [x29, 58]
	ldrh	w2, [x29, 56]
	adrp	x0, .LC127
	add	x0, x0, :lo12:.LC127
	ldr	w3, [x29, 60]
	bl	nand_print
	mov	w0, 1
	b	.L675
.L677:
	mov	w0, 0
.L675:
	ldp	x29, x30, [sp], 64
	ret
	.size	crc_wrong, .-crc_wrong
	.section	.rodata
	.align	3
.LC128:
	.string	"[NE]%s:malloc fail for temp_page_buf\n"
	.align	3
.LC129:
	.string	"[NE]%s:malloc fail for logic_page_buf\n"
	.align	3
.LC130:
	.string	"[NE]zone_param_init error2\n"
	.align	3
.LC131:
	.string	"[NE]%s:malloc fail for current_block.user_info.buf\n"
	.align	3
.LC132:
	.string	"[NE]%s:malloc fail for assist_block.user_info.buf\n"
	.align	3
.LC133:
	.string	"[NE]%s:malloc fail for zone_phy_page_map_for_gc.buf\n"
	.text
	.align	2
	.global	zone_param_init
	.type	zone_param_init, %function
zone_param_init:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	strh	w1, [x29, 38]
	str	w2, [x29, 32]
	str	w3, [x29, 28]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 512]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 512]
	cmp	x0, 0
	bne	.L679
	adrp	x0, __func__.6073
	add	x1, x0, :lo12:__func__.6073
	adrp	x0, .LC128
	add	x0, x0, :lo12:.LC128
	bl	nand_print
	mov	w0, -2
	b	.L680
.L679:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 520]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 520]
	cmp	x0, 0
	bne	.L681
	adrp	x0, __func__.6073
	add	x1, x0, :lo12:__func__.6073
	adrp	x0, .LC129
	add	x0, x0, :lo12:.LC129
	bl	nand_print
	mov	w0, -2
	b	.L680
.L681:
	ldr	x0, [x29, 40]
	ldr	w1, [x29, 32]
	str	w1, [x0, 32]
	ldr	x0, [x29, 40]
	ldr	w1, [x29, 28]
	str	w1, [x0, 40]
	ldr	x0, [x29, 40]
	ldrh	w1, [x29, 38]
	strh	w1, [x0, 44]
	ldrh	w1, [x29, 38]
	ldr	x0, [x29, 40]
	bl	get_phy_block_addr
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 48]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 26]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 28]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 30]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 56]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 40]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 56]
	ldr	w1, [x29, 56]
	ldr	x0, [x29, 40]
	bl	malloc_logic_page_map
	cmp	w0, 0
	beq	.L682
	adrp	x0, .LC130
	add	x0, x0, :lo12:.LC130
	bl	nand_print
	mov	w0, -2
	b	.L680
.L682:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	lsl	w0, w0, 2
	mov	w1, w0
	ldr	x0, [x29, 40]
	str	w1, [x0, 560]
	ldr	x0, [x29, 40]
	mov	w1, 320
	str	w1, [x0, 576]
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 560]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 576]
	add	w1, w1, w0
	ldr	x0, [x29, 40]
	str	w1, [x0, 544]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 544]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 536]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 536]
	cmp	x0, 0
	bne	.L683
	adrp	x0, __func__.6073
	add	x1, x0, :lo12:__func__.6073
	adrp	x0, .LC131
	add	x0, x0, :lo12:.LC131
	bl	nand_print
	mov	w0, -2
	b	.L680
.L683:
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 536]
	ldr	x0, [x29, 40]
	str	x1, [x0, 552]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 536]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 560]
	uxtw	x0, w0
	add	x1, x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 568]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 568]
	ldr	x0, [x29, 40]
	str	x1, [x0, 3440]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 536]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 544]
	mov	w2, w0
	mov	w1, -1
	mov	x0, x3
	bl	nand_memset
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 560]
	ldr	x0, [x29, 40]
	str	w1, [x0, 624]
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 576]
	ldr	x0, [x29, 40]
	str	w1, [x0, 576]
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 544]
	ldr	x0, [x29, 40]
	str	w1, [x0, 608]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 608]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 600]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 600]
	cmp	x0, 0
	bne	.L684
	adrp	x0, __func__.6073
	add	x1, x0, :lo12:__func__.6073
	adrp	x0, .LC132
	add	x0, x0, :lo12:.LC132
	bl	nand_print
	mov	w0, -2
	b	.L680
.L684:
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 600]
	ldr	x0, [x29, 40]
	str	x1, [x0, 616]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 600]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 624]
	uxtw	x0, w0
	add	x1, x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 632]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 600]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 608]
	mov	w2, w0
	mov	w1, -1
	mov	x0, x3
	bl	nand_memset
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 560]
	ldr	x0, [x29, 40]
	str	w1, [x0, 488]
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 576]
	ldr	x0, [x29, 40]
	str	w1, [x0, 504]
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 544]
	ldr	x0, [x29, 40]
	str	w1, [x0, 472]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 472]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 464]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 464]
	cmp	x0, 0
	bne	.L685
	adrp	x0, __func__.6073
	add	x1, x0, :lo12:__func__.6073
	adrp	x0, .LC133
	add	x0, x0, :lo12:.LC133
	bl	nand_print
	mov	w0, -2
	b	.L680
.L685:
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 464]
	ldr	x0, [x29, 40]
	str	x1, [x0, 480]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 464]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 488]
	uxtw	x0, w0
	add	x1, x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 496]
	ldr	x0, [x29, 40]
	str	wzr, [x0, 656]
	ldr	x0, [x29, 40]
	str	wzr, [x0, 676]
	ldr	x0, [x29, 40]
	str	wzr, [x0, 660]
	ldr	x0, [x29, 40]
	str	wzr, [x0, 664]
	ldr	x0, [x29, 40]
	str	wzr, [x0, 668]
	ldr	x0, [x29, 40]
	str	wzr, [x0, 672]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	lsr	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 40]
	strh	w1, [x0, 1056]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 16]
	mov	w0, 43691
	movk	w0, 0xaaaa, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	lsr	w0, w0, 2
	uxth	w1, w0
	ldr	x0, [x29, 40]
	strh	w1, [x0, 1058]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 1060]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 1062]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 1064]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 528]
	ldr	x0, [x29, 40]
	mov	w1, -1
	strh	w1, [x0, 584]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 592]
	ldr	x0, [x29, 40]
	mov	w1, -1
	strh	w1, [x0, 648]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 304]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 312]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 288]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 296]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 320]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 328]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 368]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 376]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 352]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 360]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 384]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 392]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 432]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 440]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 416]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 424]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 448]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 456]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 696]
	str	wzr, [x29, 60]
	b	.L686
.L687:
	ldr	w0, [x29, 60]
	uxth	w2, w0
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 704
	mov	w1, w2
	strh	w1, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 706
	strh	wzr, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 712
	str	xzr, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	add	x0, x0, 22
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 16
	str	xzr, [x0]
	ldr	x1, [x29, 40]
	ldr	w0, [x29, 60]
	add	x0, x0, 22
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 24
	str	xzr, [x0]
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L686:
	ldr	w0, [x29, 60]
	cmp	w0, 9
	bls	.L687
	ldr	x0, [x29, 40]
	mov	w1, 255
	strh	w1, [x0, 1024]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 1026]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 1032]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 1040]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 1048]
	mov	w0, 0
.L680:
	ldp	x29, x30, [sp], 64
	ret
	.size	zone_param_init, .-zone_param_init
	.align	2
	.global	zone_param_exit
	.type	zone_param_exit, %function
zone_param_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	free_logic_page_map
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 464]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 536]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 600]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 512]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 520]
	bl	nftl_free
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	zone_param_exit, .-zone_param_exit
	.section	.rodata
	.align	3
.LC134:
	.string	"[NE]mtd size changed !\n"
	.text
	.align	2
	.global	is_no_use_device
	.type	is_no_use_device, %function
is_no_use_device:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 48]
	str	x0, [x29, 80]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	cmp	w0, 0
	beq	.L691
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 32]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 40]
	add	w0, w1, w0
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 92]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 92]
	udiv	w0, w0, w1
	str	w0, [x29, 92]
	b	.L692
.L691:
	mov	w0, -1
	str	w0, [x29, 92]
.L692:
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 26]
	b	.L693
.L701:
	ldr	x0, [x29, 24]
	ldr	x1, [x29, 80]
	str	x1, [x0, 56]
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 56
	add	x0, x29, 32
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3496]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	cmp	w0, 1
	bne	.L694
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 76]
	add	x0, x29, 56
	bl	is_ftl_start_flag_page
	cmp	w0, 1
	bne	.L695
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L696
	ldr	w0, [x29, 76]
	cmp	w0, 10
	bne	.L695
.L696:
	add	x0, x29, 56
	mov	w2, 4
	mov	w1, 3
	bl	get_spare_data
	str	w0, [x29, 72]
	ldr	w1, [x29, 72]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	beq	.L697
	adrp	x0, .LC134
	add	x0, x0, :lo12:.LC134
	bl	nand_print
	mov	w0, 1
	b	.L702
.L697:
	ldr	x0, [x29, 80]
	add	x1, x0, 64
	ldr	x0, [x29, 24]
	str	x1, [x0, 48]
	mov	w0, 0
	b	.L702
.L695:
	mov	w0, 1
	b	.L702
.L694:
	ldr	x0, [x29, 80]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 80]
	bl	is_last_phy_block
	cmp	w0, 1
	beq	.L703
	ldr	x0, [x29, 80]
	add	x0, x0, 64
	str	x0, [x29, 80]
.L693:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	ldr	w0, [x29, 92]
	cmp	w1, w0
	bcc	.L701
	b	.L700
.L703:
	nop
.L700:
	mov	w0, 1
.L702:
	ldp	x29, x30, [sp], 96
	ret
	.size	is_no_use_device, .-is_no_use_device
	.align	2
	.global	get_vaild_blocks
	.type	get_vaild_blocks, %function
get_vaild_blocks:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldr	w0, [x29, 20]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	x0, [x29, 64]
	ldr	w1, [x29, 16]
	ldr	w0, [x29, 20]
	sub	w0, w1, w0
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	str	w0, [x29, 76]
	ldr	w0, [x29, 20]
	str	w0, [x29, 72]
	b	.L705
.L710:
	ldr	x0, [x29, 64]
	ldrh	w0, [x0]
	strh	w0, [x29, 34]
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3496]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	cmp	w0, 159
	bne	.L706
	ldr	w0, [x29, 76]
	sub	w0, w0, #1
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	bne	.L706
	mov	w0, 0
	b	.L711
.L706:
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 64]
	bl	is_last_phy_block
	cmp	w0, 1
	beq	.L712
	ldr	x0, [x29, 64]
	add	x0, x0, 64
	str	x0, [x29, 64]
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
.L705:
	ldr	w1, [x29, 72]
	ldr	w0, [x29, 60]
	cmp	w1, w0
	bcc	.L710
	b	.L709
.L712:
	nop
.L709:
	ldr	w0, [x29, 76]
.L711:
	ldp	x29, x30, [sp], 80
	ret
	.size	get_vaild_blocks, .-get_vaild_blocks
	.section	.rodata
	.align	3
.LC135:
	.string	"[ND]build_zone_list_first.\n"
	.align	3
.LC136:
	.string	"[NE]no block %d\n"
	.text
	.align	2
	.global	build_zone_list_first
	.type	build_zone_list_first, %function
build_zone_list_first:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	adrp	x0, .LC135
	add	x0, x0, :lo12:.LC135
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 48]
	str	x0, [x29, 80]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	cmp	w0, 0
	beq	.L714
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 32]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 40]
	add	w0, w1, w0
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 92]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 92]
	udiv	w0, w0, w1
	str	w0, [x29, 92]
	b	.L715
.L714:
	mov	w0, -1
	str	w0, [x29, 92]
.L715:
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 26]
	b	.L716
.L725:
	ldr	x0, [x29, 24]
	ldr	x1, [x29, 80]
	str	x1, [x0, 56]
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 56
	add	x0, x29, 32
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3496]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	cmp	w0, 1
	bne	.L717
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L718
	ldr	x0, [x29, 80]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	b	.L722
.L718:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	x0, [x29, 80]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 80]
	strh	wzr, [x0, 6]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	cmp	w0, 1
	bne	.L722
	add	x0, x29, 56
	mov	w2, 16
	mov	w1, -1
	bl	nand_memset
	add	x0, x29, 56
	ldr	w2, [x29, 20]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_start_block_flag
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 56
	add	x0, x29, 32
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	str	wzr, [x29, 88]
	b	.L720
.L721:
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldrh	w0, [x29, 32]
	add	w0, w0, 1
	uxth	w0, w0
	strh	w0, [x29, 32]
	ldr	w0, [x29, 88]
	add	w0, w0, 1
	str	w0, [x29, 88]
.L720:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 88]
	cmp	w1, w0
	bhi	.L721
	ldr	x0, [x29, 80]
	add	x1, x0, 64
	ldr	x0, [x29, 24]
	str	x1, [x0, 48]
	b	.L722
.L717:
	ldr	x0, [x29, 80]
	mov	w1, 255
	strh	w1, [x0, 2]
.L722:
	ldr	x0, [x29, 80]
	mov	w1, -1
	strh	w1, [x0, 4]
	ldr	x0, [x29, 80]
	str	xzr, [x0, 32]
	ldr	x0, [x29, 80]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 80]
	str	xzr, [x0, 24]
	ldr	x0, [x29, 24]
	ldr	x1, [x29, 80]
	str	x1, [x0, 56]
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 80]
	bl	is_last_phy_block
	cmp	w0, 1
	beq	.L739
	ldr	x0, [x29, 80]
	add	x0, x0, 64
	str	x0, [x29, 80]
.L716:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	ldr	w0, [x29, 92]
	cmp	w1, w0
	bcc	.L725
	b	.L724
.L739:
	nop
.L724:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	cmp	w0, 1
	bls	.L726
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	w0, [x29, 92]
	cmn	w0, #1
	bne	.L729
	b	.L738
.L726:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	adrp	x0, .LC136
	add	x0, x0, :lo12:.LC136
	bl	nand_print
	mov	w0, 1
	b	.L737
.L738:
	mov	w0, 0
	b	.L737
.L729:
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 80]
	bl	is_last_phy_block
	cmp	w0, 1
	bne	.L731
	mov	w0, 0
	b	.L737
.L736:
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 56
	add	x0, x29, 32
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3496]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	cmp	w0, 1
	bne	.L732
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L733
	ldr	x0, [x29, 80]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	b	.L734
.L733:
	add	x0, x29, 56
	mov	w2, 16
	mov	w1, -1
	bl	nand_memset
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_end_block_flag
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 56
	add	x0, x29, 32
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	b	.L735
.L732:
	ldr	x0, [x29, 80]
	mov	w1, 255
	strh	w1, [x0, 2]
.L734:
	ldr	x0, [x29, 80]
	add	x0, x0, 64
	str	x0, [x29, 80]
.L731:
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	cmp	w1, w0
	bcc	.L736
.L735:
	mov	w0, 0
.L737:
	ldp	x29, x30, [sp], 96
	ret
	.size	build_zone_list_first, .-build_zone_list_first
	.section	.rodata
	.align	3
.LC137:
	.string	"[ND]first\n"
	.align	3
.LC138:
	.string	"[NE]init_zone_after_first_scan error\n"
	.align	3
.LC139:
	.string	"[NE]second_scan_all_blocks error\n"
	.align	3
.LC140:
	.string	"[NI]nftl ok!\n"
	.text
	.align	2
	.global	build_zone_list
	.type	build_zone_list, %function
build_zone_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, .LC137
	add	x0, x0, :lo12:.LC137
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	first_scan_all_blocks
	cmp	w0, 0
	beq	.L741
	mov	w0, 1
	b	.L742
.L741:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	ldr	x0, [x29, 24]
	bl	init_zone_after_first_scan
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L743
	adrp	x0, .LC138
	add	x0, x0, :lo12:.LC138
	bl	nand_print
	mov	w0, 1
	b	.L742
.L743:
	ldr	x0, [x29, 24]
	bl	second_scan_all_blocks
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L744
	adrp	x0, .LC139
	add	x0, x0, :lo12:.LC139
	bl	nand_print
	mov	w0, 1
	b	.L742
.L744:
	ldr	x0, [x29, 24]
	bl	power_on_gc
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	bl	print_nftl_zone
	adrp	x0, .LC140
	add	x0, x0, :lo12:.LC140
	bl	nand_print
	ldr	x0, [x29, 24]
	bl	panic_backup_block
	mov	w0, 0
.L742:
	ldp	x29, x30, [sp], 48
	ret
	.size	build_zone_list, .-build_zone_list
	.section	.rodata
	.align	3
.LC141:
	.string	"[ND]fsab\n"
	.align	3
.LC142:
	.string	"[NI]ftl end block! %d!\n"
	.align	3
.LC143:
	.string	"[ND]READ_RECLAIM!\n"
	.align	3
.LC144:
	.string	"[NI]start page , do nothing\n"
	.align	3
.LC145:
	.string	"[ND]fill block %d\n"
	.align	3
.LC146:
	.string	"[NI]b@%d p@0 unknow spare data:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x.\n"
	.align	3
.LC147:
	.string	"[NE]nftl bad block: %d\n"
	.align	3
.LC148:
	.string	"[NE]nftl b@%d p@0 ecc error\n"
	.align	3
.LC149:
	.string	"[NE]b@%d last page ok !\n"
	.align	3
.LC150:
	.string	"[NI]READ_RECLAIM!\n"
	.align	3
.LC151:
	.string	"[NE]last page blank ! %d!\n"
	.align	3
.LC152:
	.string	"[NE]last page unkown ! %d!\n"
	.align	3
.LC153:
	.string	"[NE]b@%d first page last page ecc error\n"
	.align	3
.LC154:
	.string	"[NE]b@%d last page spare data %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
	.align	3
.LC155:
	.string	"[NI]nftl bad block: %d\n"
	.text
	.align	2
	.global	first_scan_all_blocks
	.type	first_scan_all_blocks, %function
first_scan_all_blocks:
	sub	sp, sp, #208
	stp	x29, x30, [sp, 80]
	add	x29, sp, 80
	str	x0, [x29, 24]
	str	wzr, [x29, 124]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 48]
	str	x0, [x29, 112]
	str	xzr, [x29, 104]
	adrp	x0, .LC0
	add	x1, x0, :lo12:.LC0
	add	x0, x29, 48
	ldr	w2, [x1]
	str	w2, [x0]
	ldr	w1, [x1, 1]
	str	w1, [x0, 1]
	mov	w0, 47
	strb	w0, [x29, 40]
	strb	wzr, [x29, 41]
	adrp	x0, mod
	add	x1, x0, :lo12:mod
	add	x0, x29, 40
	mov	x2, x1
	mov	x1, x0
	mov	x0, 0
	bl	nand_fdt_get_prop_string_by_path
	str	x0, [x29, 104]
	ldr	x0, [x29, 104]
	cmp	x0, 0
	bne	.L746
	mov	w0, -1
	b	.L777
.L746:
	add	x0, x29, 48
	mov	x2, 3
	mov	x1, x0
	ldr	x0, [x29, 104]
	bl	nand_memcmp
	cmp	w0, 0
	beq	.L748
	mov	w0, -1
	b	.L777
.L748:
	adrp	x0, .LC141
	add	x0, x0, :lo12:.LC141
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 26]
	b	.L749
.L776:
	ldr	x0, [x29, 112]
	ldrh	w1, [x0]
	add	x2, x29, 80
	add	x0, x29, 56
	mov	x5, x2
	mov	x4, 0
	mov	w3, 0
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3496]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	cmp	w0, 1
	bne	.L751
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 100]
	ldr	x0, [x29, 24]
	ldr	x1, [x29, 112]
	str	x1, [x0, 56]
	add	x0, x29, 80
	bl	get_block_used_count_from_oob
	str	w0, [x29, 96]
	add	x0, x29, 80
	bl	is_ftl_end_flag_page
	cmp	w0, 1
	bne	.L752
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC142
	add	x0, x0, :lo12:.LC142
	bl	nand_print
	b	.L753
.L752:
	add	x0, x29, 80
	bl	is_panic_data_page
	cmp	w0, 1
	bne	.L754
	ldrb	w1, [x29, 83]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	cmp	w1, w0
	bne	.L775
	add	x0, x29, 80
	ldr	x1, [x29, 112]
	bl	panic_recover_info
	b	.L775
.L754:
	add	x0, x29, 80
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L757
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 6]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 4]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_free_list
	b	.L775
.L757:
	add	x0, x29, 80
	bl	is_ftl_logic_page_data
	cmp	w0, 1
	beq	.L758
	add	x0, x29, 80
	bl	is_function_info_page
	cmp	w0, 1
	bne	.L759
.L758:
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L760
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L759
.L760:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	x0, [x29, 112]
	mov	w1, 1
	strh	w1, [x0, 2]
	add	x0, x29, 80
	bl	get_erase_count_from_oob
	uxth	w1, w0
	ldr	x0, [x29, 112]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 124]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 4]
	ldr	w1, [x29, 96]
	ldr	x0, [x29, 112]
	str	w1, [x0, 8]
	ldr	x0, [x29, 112]
	str	xzr, [x0, 48]
	ldr	x0, [x29, 112]
	str	xzr, [x0, 56]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	add_block_count_list
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L778
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 4]
	cmp	w0, 0
	beq	.L778
	mov	w2, 1
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	adrp	x0, .LC143
	add	x0, x0, :lo12:.LC143
	bl	nand_print_dbg
	b	.L778
.L759:
	add	x0, x29, 80
	bl	is_ftl_start_flag_page
	cmp	w0, 1
	bne	.L762
	adrp	x0, .LC144
	add	x0, x0, :lo12:.LC144
	bl	nand_print
	b	.L775
.L762:
	add	x0, x29, 80
	bl	is_fill_page
	cmp	w0, 1
	bne	.L763
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 2]
	add	x0, x29, 80
	bl	get_erase_count_from_oob
	uxth	w1, w0
	ldr	x0, [x29, 112]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 124]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 20]
	ldr	x0, [x29, 112]
	strh	w1, [x0, 4]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_free_list
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC145
	add	x0, x0, :lo12:.LC145
	bl	nand_print_dbg
	b	.L775
.L763:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w11, w0
	ldrb	w0, [x29, 80]
	mov	w12, w0
	ldrb	w0, [x29, 81]
	mov	w13, w0
	ldrb	w0, [x29, 82]
	mov	w14, w0
	ldrb	w0, [x29, 83]
	mov	w15, w0
	ldrb	w0, [x29, 84]
	mov	w16, w0
	ldrb	w0, [x29, 85]
	mov	w17, w0
	ldrb	w0, [x29, 86]
	mov	w1, w0
	ldrb	w0, [x29, 87]
	mov	w2, w0
	ldrb	w0, [x29, 88]
	mov	w3, w0
	ldrb	w0, [x29, 89]
	mov	w4, w0
	ldrb	w0, [x29, 90]
	mov	w5, w0
	ldrb	w0, [x29, 91]
	mov	w6, w0
	ldrb	w0, [x29, 92]
	mov	w7, w0
	ldrb	w0, [x29, 93]
	mov	w8, w0
	ldrb	w0, [x29, 94]
	mov	w9, w0
	ldrb	w0, [x29, 95]
	mov	w10, w0
	adrp	x0, .LC146
	add	x0, x0, :lo12:.LC146
	str	w10, [sp, 72]
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w17
	mov	w6, w16
	mov	w5, w15
	mov	w4, w14
	mov	w3, w13
	mov	w2, w12
	mov	w1, w11
	bl	nand_print
	ldr	w0, [x29, 124]
	uxth	w0, w0
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 112]
	strh	w1, [x0, 6]
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L764
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L765
.L764:
	ldrb	w0, [x29, 80]
	cmp	w0, 255
	beq	.L766
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC147
	add	x0, x0, :lo12:.LC147
	bl	nand_print
	ldr	x0, [x29, 112]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 28]
	b	.L779
.L766:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	erase_block
	cmp	w0, 0
	bne	.L779
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	b	.L779
.L765:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC148
	add	x0, x0, :lo12:.LC148
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	uxth	w2, w0
	add	x3, x29, 80
	add	x0, x29, 56
	mov	x5, x3
	mov	x4, 0
	mov	w3, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 100]
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L768
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L769
.L768:
	add	x0, x29, 80
	bl	is_phy_mapping_page
	cmp	w0, 1
	bne	.L770
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC149
	add	x0, x0, :lo12:.LC149
	bl	nand_print
	add	x0, x29, 80
	bl	get_block_used_count_from_oob
	str	w0, [x29, 96]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	ldr	x0, [x29, 112]
	mov	w1, 1
	strh	w1, [x0, 2]
	add	x0, x29, 80
	bl	get_erase_count_from_oob
	uxth	w1, w0
	ldr	x0, [x29, 112]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 124]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 4]
	ldr	w1, [x29, 96]
	ldr	x0, [x29, 112]
	str	w1, [x0, 8]
	ldr	x0, [x29, 112]
	str	xzr, [x0, 48]
	ldr	x0, [x29, 112]
	str	xzr, [x0, 56]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	add_block_count_list
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L780
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 4]
	cmp	w0, 0
	beq	.L780
	mov	w2, 1
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	adrp	x0, .LC150
	add	x0, x0, :lo12:.LC150
	bl	nand_print
	b	.L780
.L770:
	add	x0, x29, 80
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L773
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC151
	add	x0, x0, :lo12:.LC151
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	erase_block
	cmp	w0, 0
	bne	.L780
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	b	.L780
.L773:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC152
	add	x0, x0, :lo12:.LC152
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	erase_block
	cmp	w0, 0
	bne	.L780
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	b	.L780
.L769:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC153
	add	x0, x0, :lo12:.LC153
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w11, w0
	ldrb	w0, [x29, 80]
	mov	w12, w0
	ldrb	w0, [x29, 81]
	mov	w13, w0
	ldrb	w0, [x29, 82]
	mov	w14, w0
	ldrb	w0, [x29, 83]
	mov	w15, w0
	ldrb	w0, [x29, 84]
	mov	w16, w0
	ldrb	w0, [x29, 85]
	mov	w17, w0
	ldrb	w0, [x29, 86]
	mov	w1, w0
	ldrb	w0, [x29, 87]
	mov	w2, w0
	ldrb	w0, [x29, 88]
	mov	w3, w0
	ldrb	w0, [x29, 89]
	mov	w4, w0
	ldrb	w0, [x29, 90]
	mov	w5, w0
	ldrb	w0, [x29, 91]
	mov	w6, w0
	ldrb	w0, [x29, 92]
	mov	w7, w0
	ldrb	w0, [x29, 93]
	mov	w8, w0
	ldrb	w0, [x29, 94]
	mov	w9, w0
	ldrb	w0, [x29, 95]
	mov	w10, w0
	adrp	x0, .LC154
	add	x0, x0, :lo12:.LC154
	str	w10, [sp, 72]
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w17
	mov	w6, w16
	mov	w5, w15
	mov	w4, w14
	mov	w3, w13
	mov	w2, w12
	mov	w1, w11
	bl	nand_print
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	torture_block_and_mark
	cmp	w0, 0
	bne	.L775
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 26]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 26]
	b	.L775
.L751:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC155
	add	x0, x0, :lo12:.LC155
	bl	nand_print
	ldr	x0, [x29, 112]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 112]
	strh	wzr, [x0, 6]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 28]
	b	.L775
.L778:
	nop
	b	.L775
.L779:
	nop
	b	.L775
.L780:
	nop
.L775:
	ldr	x0, [x29, 112]
	add	x0, x0, 64
	str	x0, [x29, 112]
.L749:
	ldr	x0, [x29, 112]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	cmp	w1, w0
	bcc	.L776
.L753:
	mov	w0, 0
.L777:
	add	sp, x29, 0
	ldp	x29, x30, [sp], 128
	ret
	.size	first_scan_all_blocks, .-first_scan_all_blocks
	.section	.rodata
	.align	3
.LC0:
	.byte	115
	.byte	117
	.byte	110
	.byte	120
	.byte	105
	.text
	.section	.rodata
	.align	3
.LC156:
	.string	"[ND]before second %d %d.\n"
	.align	3
.LC157:
	.string	"[NE]only have blocks:%u,logic cap need blocks:%u this zone not enough data block!!\n"
	.align	3
.LC158:
	.string	"[NE]this zone not enough free block %d , %d!!\n"
	.text
	.align	2
	.global	init_zone_after_first_scan
	.type	init_zone_after_first_scan, %function
init_zone_after_first_scan:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 44]
	udiv	w0, w0, w1
	str	w0, [x29, 44]
	adrp	x0, .LC156
	add	x0, x0, :lo12:.LC156
	ldr	w2, [x29, 44]
	ldr	w1, [x29, 20]
	bl	nand_print_dbg
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L782
	adrp	x0, .LC157
	add	x0, x0, :lo12:.LC157
	ldr	w2, [x29, 44]
	ldr	w1, [x29, 20]
	bl	nand_print
	mov	w0, 1
	b	.L783
.L782:
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	sub	w0, w1, w0
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	mul	w0, w0, w1
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrh	w1, [x1, 20]
	mul	w1, w0, w1
	ldr	x0, [x29, 24]
	str	w1, [x0, 40]
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	sub	w0, w1, w0
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 3448]
	ldrh	w1, [x1, 12]
	sub	w1, w1, #1
	cmp	w0, w1
	bcs	.L784
	adrp	x0, .LC158
	add	x0, x0, :lo12:.LC158
	ldr	w2, [x29, 44]
	ldr	w1, [x29, 20]
	bl	nand_print
	mov	w0, 1
	b	.L783
.L784:
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	sub	w1, w1, w0
	mov	w0, 52429
	movk	w0, 0xcccc, lsl 16
	umull	x0, w1, w0
	lsr	x0, x0, 32
	lsr	w0, w0, 2
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 680]
	ldr	x0, [x29, 24]
	ldrh	w1, [x0, 680]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 14]
	cmp	w1, w0
	bcs	.L785
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w1, [x0, 14]
	ldr	x0, [x29, 24]
	strh	w1, [x0, 680]
.L785:
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 44]
	sub	w0, w1, w0
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 3448]
	ldrh	w1, [x1, 18]
	mul	w0, w0, w1
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 3448]
	ldrh	w1, [x1, 20]
	udiv	w0, w0, w1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 682]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 682]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	add	w0, w0, 2
	cmp	w1, w0
	bge	.L786
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 680]
	add	w0, w0, 2
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 682]
.L786:
	ldr	x0, [x29, 24]
	mov	w1, 1
	strh	w1, [x0, 684]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 686]
	mov	w0, 0
.L783:
	ldp	x29, x30, [sp], 48
	ret
	.size	init_zone_after_first_scan, .-init_zone_after_first_scan
	.section	.rodata
	.align	3
.LC159:
	.string	"[NI]NAND_EVENT: 2 current used block found %d,%d!\n"
	.align	3
.LC160:
	.string	"[NE]no crosstalk:2 block used count diff %d,%d %d,%d !\n"
	.align	3
.LC161:
	.string	"[NE]NAND_EVENT: muti current used block found1 %d %d!\n"
	.align	3
.LC162:
	.string	"[NE]NAND_EVENT: muti current used block found2 %d %d!\n"
	.align	3
.LC163:
	.string	"[NE]NAND_EVENT: muti current used block found3 %d %d!\n"
	.align	3
.LC164:
	.string	"[NE]NAND_EVENT: last page spare data %x,%x,%x,%x,%x,%x,%x %x %x %x %x %x %x %x %x %x!!\n"
	.align	3
.LC165:
	.string	"[NI]zone@%d b@%d last page unkown ecc ok.\n"
	.align	3
.LC166:
	.string	"[NE]zone@%d b@%d last page ecc error.\n"
	.align	3
.LC167:
	.string	"[NE]something strange %d,%d!!\n"
	.align	3
.LC168:
	.string	"[NI]all block full!!\n"
	.align	3
.LC169:
	.string	"[ND]corss talk rebuild 0 %d %d!!\n"
	.align	3
.LC170:
	.string	"[NE]1st current block:%d 1st current block uc:%d muti current used block 1st.\n"
	.align	3
.LC171:
	.string	"[ND]2nd current block:%d 2nd current block uc:%d muti current used block 2nd.\n"
	.align	3
.LC172:
	.string	"[NE]something is error here 7!\n"
	.align	3
.LC173:
	.string	"[NE]p_phy_block_info_last %d %d.\n"
	.align	3
.LC174:
	.string	"[NE]get_current_used_block fail.\n"
	.align	3
.LC175:
	.string	"[ND]free block nums is %d, aec: %d, iec: %d, a_buc: %d, i_buc: %d!\n"
	.text
	.align	2
	.global	second_scan_all_blocks
	.type	second_scan_all_blocks, %function
second_scan_all_blocks:
	sub	sp, sp, #256
	stp	x29, x30, [sp, 80]
	add	x29, sp, 80
	str	x0, [x29, 24]
	str	wzr, [x29, 160]
	str	xzr, [x29, 112]
	str	xzr, [x29, 104]
	str	xzr, [x29, 128]
	str	xzr, [x29, 136]
	str	xzr, [x29, 120]
	str	wzr, [x29, 100]
	adrp	x0, g_nreg_base
	add	x0, x0, :lo12:g_nreg_base
	ldr	x0, [x0]
	str	x0, [x29, 88]
	mov	w0, 188
	str	w0, [x29, 84]
	mov	w0, 1
	str	w0, [x29, 80]
	mov	w0, 2
	str	w0, [x29, 76]
	mov	w0, 1
	movk	w0, 0x3, lsl 16
	str	w0, [x29, 72]
	ldr	w0, [x29, 84]
	lsl	x0, x0, 2
	ldr	x1, [x29, 88]
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [x29, 100]
	ldr	w1, [x29, 100]
	ldr	w0, [x29, 72]
	cmp	w1, w0
	beq	.L788
	ldr	w1, [x29, 100]
	ldr	w0, [x29, 80]
	cmp	w1, w0
	beq	.L788
	ldr	w1, [x29, 100]
	ldr	w0, [x29, 76]
	cmp	w1, w0
	beq	.L788
	mov	w0, -1
	b	.L830
.L788:
	mov	w0, -1
	str	w0, [x29, 172]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w1, w0, w1
	ldr	x0, [x29, 24]
	str	w1, [x0, 36]
	mov	w0, -1
	str	w0, [x29, 164]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 448]
	str	x0, [x29, 152]
	b	.L790
.L791:
	ldr	x0, [x29, 152]
	ldr	w0, [x0, 8]
	str	w0, [x29, 164]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 152]
	ldr	w1, [x1, 8]
	str	w1, [x0, 156]
	ldr	x0, [x29, 152]
	ldr	x0, [x0, 48]
	str	x0, [x29, 152]
.L790:
	ldr	x0, [x29, 152]
	cmp	x0, 0
	bne	.L791
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 448]
	cmp	x0, 0
	beq	.L792
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 448]
	ldr	w1, [x1, 8]
	str	w1, [x0, 160]
.L792:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 448]
	str	x0, [x29, 152]
	b	.L793
.L811:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 560]
	and	w0, w0, 511
	cmp	w0, 0
	beq	.L794
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 560]
	lsr	w0, w0, 9
	add	w0, w0, 1
	str	w0, [x29, 160]
	b	.L795
.L794:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 560]
	lsr	w0, w0, 9
	str	w0, [x29, 160]
.L795:
	ldr	x0, [x29, 152]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	uxth	w2, w0
	ldr	w0, [x29, 160]
	uxth	w3, w0
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 56
	add	x0, x29, 32
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 168]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 28]
	cmp	w0, 0
	beq	.L796
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 512]
	add	x1, x29, 56
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	crc_wrong
	cmp	w0, 0
	beq	.L796
	mov	w0, -2
	str	w0, [x29, 168]
.L796:
	ldr	x0, [x29, 152]
	ldr	w0, [x0, 8]
	str	w0, [x29, 172]
	ldr	x0, [x29, 152]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 676]
	cmp	w1, w0
	bls	.L797
	ldr	x0, [x29, 152]
	ldrh	w1, [x0, 6]
	mov	w0, 49999
	cmp	w1, w0
	bhi	.L797
	ldr	x0, [x29, 152]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	ldr	x0, [x29, 24]
	str	w1, [x0, 676]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 24]
	ldr	w1, [x1, 676]
	str	w1, [x0, 148]
.L797:
	ldr	x0, [x29, 152]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 152]
	cmp	w1, w0
	bcs	.L798
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 152]
	ldrh	w1, [x1, 6]
	str	w1, [x0, 152]
.L798:
	add	x0, x29, 56
	bl	is_phy_mapping_page
	cmp	w0, 1
	bne	.L799
	ldr	w0, [x29, 168]
	cmp	w0, 0
	beq	.L800
	ldr	w0, [x29, 168]
	cmp	w0, 10
	bne	.L799
.L800:
	ldr	x0, [x29, 152]
	str	x0, [x29, 128]
	ldr	x0, [x29, 152]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 152]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 512]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w3, w0
	mov	x2, x1
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 24]
	bl	recover_logic_page_mapping
	ldr	x0, [x29, 152]
	str	x0, [x29, 120]
	b	.L801
.L799:
	add	x0, x29, 56
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L802
	ldr	x0, [x29, 112]
	cmp	x0, 0
	bne	.L803
	ldr	x0, [x29, 152]
	str	x0, [x29, 112]
	b	.L801
.L803:
	ldr	x0, [x29, 104]
	cmp	x0, 0
	bne	.L805
	ldr	x0, [x29, 152]
	str	x0, [x29, 104]
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 104]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC159
	add	x0, x0, :lo12:.LC159
	bl	nand_print
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	bne	.L801
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 8]
	cmp	w1, w0
	beq	.L801
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w5, w0
	ldr	x0, [x29, 104]
	ldrh	w0, [x0]
	mov	w6, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 104]
	ldr	w2, [x0, 8]
	adrp	x0, .LC160
	add	x0, x0, :lo12:.LC160
	mov	w4, w2
	mov	w3, w1
	mov	w2, w6
	mov	w1, w5
	bl	nand_print
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 552]
	mov	x2, x0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	recover_phy_page_mapping
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 552]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w3, w0
	mov	x2, x1
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	recover_logic_page_mapping
	mov	w2, 3
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	ldr	x0, [x29, 104]
	str	x0, [x29, 112]
	str	xzr, [x29, 104]
	b	.L801
.L805:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	adrp	x0, .LC161
	add	x0, x0, :lo12:.LC161
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	ldr	x0, [x29, 104]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 8]
	adrp	x0, .LC162
	add	x0, x0, :lo12:.LC162
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	ldr	x0, [x29, 152]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC163
	add	x0, x0, :lo12:.LC163
	ldr	w2, [x29, 172]
	bl	nand_print
	ldr	x0, [x29, 112]
	str	x0, [x29, 144]
	ldr	x0, [x29, 104]
	str	x0, [x29, 112]
	ldr	x0, [x29, 152]
	str	x0, [x29, 104]
	mov	w2, 3
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	b	.L801
.L802:
	ldrb	w0, [x29, 56]
	mov	w10, w0
	ldrb	w0, [x29, 57]
	mov	w11, w0
	ldrb	w0, [x29, 58]
	mov	w12, w0
	ldrb	w0, [x29, 59]
	mov	w13, w0
	ldrb	w0, [x29, 60]
	mov	w14, w0
	ldrb	w0, [x29, 61]
	mov	w15, w0
	ldrb	w0, [x29, 62]
	mov	w16, w0
	ldrb	w0, [x29, 63]
	mov	w1, w0
	ldrb	w0, [x29, 64]
	mov	w2, w0
	ldrb	w0, [x29, 65]
	mov	w3, w0
	ldrb	w0, [x29, 66]
	mov	w4, w0
	ldrb	w0, [x29, 67]
	mov	w5, w0
	ldrb	w0, [x29, 68]
	mov	w6, w0
	ldrb	w0, [x29, 69]
	mov	w7, w0
	ldrb	w0, [x29, 70]
	mov	w8, w0
	ldrb	w0, [x29, 71]
	mov	w9, w0
	adrp	x0, .LC164
	add	x0, x0, :lo12:.LC164
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w16
	mov	w6, w15
	mov	w5, w14
	mov	w4, w13
	mov	w3, w12
	mov	w2, w11
	mov	w1, w10
	bl	nand_print
	ldr	w0, [x29, 168]
	cmp	w0, 0
	beq	.L807
	ldr	w0, [x29, 168]
	cmp	w0, 10
	bne	.L808
.L807:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	ldr	x0, [x29, 152]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC165
	add	x0, x0, :lo12:.LC165
	bl	nand_print
	b	.L809
.L808:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	ldr	x0, [x29, 152]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC166
	add	x0, x0, :lo12:.LC166
	bl	nand_print
.L809:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L810
	ldr	w1, [x29, 164]
	ldr	w0, [x29, 172]
	cmp	w1, w0
	bne	.L810
	ldr	x0, [x29, 152]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 24]
	bl	erase_block
	ldr	x0, [x29, 152]
	str	x0, [x29, 136]
	b	.L801
.L810:
	ldr	x0, [x29, 152]
	str	x0, [x29, 128]
	ldr	x0, [x29, 152]
	mov	w1, 170
	strh	w1, [x0, 2]
	ldr	x0, [x29, 152]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 152]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 552]
	mov	x2, x0
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 24]
	bl	recover_block_phy_page_mapping
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 552]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w3, w0
	mov	x2, x1
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 24]
	bl	recover_logic_page_mapping
	mov	w2, 3
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
.L801:
	ldr	x0, [x29, 152]
	ldr	x0, [x0, 48]
	str	x0, [x29, 152]
.L793:
	ldr	x0, [x29, 152]
	cmp	x0, 0
	bne	.L811
	ldr	x1, [x29, 120]
	ldr	x0, [x29, 24]
	bl	init_smart_info
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 448]
	str	x0, [x29, 144]
	b	.L812
.L814:
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 112]
	cmp	x1, x0
	beq	.L813
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 104]
	cmp	x1, x0
	beq	.L813
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 136]
	cmp	x1, x0
	beq	.L813
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_invalid_page_list
.L813:
	ldr	x0, [x29, 144]
	ldr	x0, [x0, 48]
	str	x0, [x29, 144]
.L812:
	ldr	x0, [x29, 144]
	cmp	x0, 0
	bne	.L814
	ldr	x0, [x29, 24]
	bl	adjust_invaild_list
	ldr	x0, [x29, 128]
	cmp	x0, 0
	beq	.L815
	ldr	x0, [x29, 112]
	cmp	x0, 0
	beq	.L815
	ldr	x0, [x29, 128]
	ldr	w0, [x0, 8]
	add	w1, w0, 1
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 8]
	cmp	w1, w0
	beq	.L815
	ldr	x0, [x29, 128]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 112]
	ldr	w2, [x0, 8]
	adrp	x0, .LC167
	add	x0, x0, :lo12:.LC167
	bl	nand_print
.L815:
	ldr	x0, [x29, 24]
	str	xzr, [x0, 528]
	ldr	x0, [x29, 112]
	cmp	x0, 0
	bne	.L816
	ldr	x0, [x29, 104]
	cmp	x0, 0
	bne	.L816
	adrp	x0, .LC168
	add	x0, x0, :lo12:.LC168
	bl	nand_print
	str	xzr, [x29, 152]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L817
	ldr	x0, [x29, 128]
	cmp	x0, 0
	beq	.L831
	mov	w2, 3
	ldr	x1, [x29, 128]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	b	.L831
.L817:
	ldr	x0, [x29, 128]
	cmp	x0, 0
	beq	.L831
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 2]
	cmp	w0, 170
	bne	.L831
	mov	w2, 3
	ldr	x1, [x29, 128]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	b	.L831
.L816:
	ldr	x0, [x29, 104]
	cmp	x0, 0
	bne	.L821
	ldr	x0, [x29, 112]
	str	x0, [x29, 152]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L822
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	adrp	x0, .LC169
	add	x0, x0, :lo12:.LC169
	mov	w2, w1
	mov	w1, w3
	bl	nand_print_dbg
	ldr	x0, [x29, 128]
	cmp	x0, 0
	beq	.L823
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 2]
	cmp	w0, 170
	bne	.L823
	mov	w2, 3
	ldr	x1, [x29, 128]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
.L823:
	mov	x2, 0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	cross_talk_rebuild_current_block
	str	x0, [x29, 152]
	ldr	x0, [x29, 152]
	cmp	x0, 0
	bne	.L820
	mov	w0, 1
	b	.L830
.L822:
	ldr	x0, [x29, 128]
	cmp	x0, 0
	beq	.L825
	ldr	x0, [x29, 128]
	ldrh	w0, [x0, 2]
	cmp	w0, 170
	bne	.L825
	mov	w2, 3
	ldr	x1, [x29, 128]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
.L825:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	cmp	w0, 0
	bne	.L826
	ldr	x0, [x29, 112]
	str	x0, [x29, 152]
	b	.L820
.L826:
	mov	x2, 0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	no_cross_talk_rebuild_current_block
	str	x0, [x29, 152]
	b	.L820
.L821:
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L827
	ldr	x2, [x29, 104]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	cross_talk_rebuild_current_block
	str	x0, [x29, 152]
	ldr	x0, [x29, 152]
	cmp	x0, 0
	bne	.L820
	mov	w0, 1
	b	.L830
.L827:
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 8]
	cmp	w1, w0
	bne	.L828
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	adrp	x0, .LC170
	add	x0, x0, :lo12:.LC170
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	ldr	x0, [x29, 104]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 8]
	adrp	x0, .LC171
	add	x0, x0, :lo12:.LC171
	mov	w2, w1
	mov	w1, w3
	bl	nand_print_dbg
	ldr	x2, [x29, 104]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	no_cross_talk_rebuild_current_block
	str	x0, [x29, 152]
	b	.L820
.L828:
	adrp	x0, .LC172
	add	x0, x0, :lo12:.LC172
	bl	nand_print
	ldr	x0, [x29, 128]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 128]
	ldr	w1, [x0, 8]
	adrp	x0, .LC173
	add	x0, x0, :lo12:.LC173
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 8]
	adrp	x0, .LC170
	add	x0, x0, :lo12:.LC170
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	ldr	x0, [x29, 104]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 8]
	adrp	x0, .LC171
	add	x0, x0, :lo12:.LC171
	mov	w2, w1
	mov	w1, w3
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 552]
	mov	x2, x0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	recover_block_phy_page_mapping
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 552]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w3, w0
	mov	x2, x1
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	recover_logic_page_mapping
	mov	w2, 3
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 552]
	mov	x2, x0
	ldr	x1, [x29, 104]
	ldr	x0, [x29, 24]
	bl	recover_block_phy_page_mapping
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 552]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	mov	w3, w0
	mov	x2, x1
	ldr	x1, [x29, 104]
	ldr	x0, [x29, 24]
	bl	recover_logic_page_mapping
	mov	w2, 3
	ldr	x1, [x29, 104]
	ldr	x0, [x29, 24]
	bl	add_prio_gc
	b	.L820
.L831:
	nop
.L820:
	ldr	w2, [x29, 172]
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 24]
	bl	get_current_used_block
	str	w0, [x29, 168]
	ldr	w0, [x29, 168]
	cmp	w0, 0
	beq	.L829
	adrp	x0, .LC174
	add	x0, x0, :lo12:.LC174
	bl	nand_print
	mov	w0, 1
	b	.L830
.L829:
	ldr	x0, [x29, 24]
	bl	adjust_invaild_list
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 528]
	ldr	x0, [x29, 24]
	str	x1, [x0, 64]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 30]
	mov	w6, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 148]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w2, [x0, 152]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w3, [x0, 156]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w4, [x0, 160]
	adrp	x0, .LC175
	add	x0, x0, :lo12:.LC175
	mov	w5, w4
	mov	w4, w3
	mov	w3, w2
	mov	w2, w1
	mov	w1, w6
	bl	nand_print_dbg
	mov	w0, 0
.L830:
	add	sp, x29, 0
	ldp	x29, x30, [sp], 176
	ret
	.size	second_scan_all_blocks, .-second_scan_all_blocks
	.align	2
	.global	check_bit_nums
	.type	check_bit_nums, %function
check_bit_nums:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	str	wzr, [sp, 20]
	str	wzr, [sp, 28]
	b	.L833
.L837:
	mov	w0, 1
	strb	w0, [sp, 19]
	str	wzr, [sp, 24]
	b	.L834
.L836:
	ldrsw	x0, [sp, 28]
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	ldrb	w1, [x0]
	ldrb	w0, [sp, 19]
	and	w0, w1, w0
	uxtb	w0, w0
	cmp	w0, 0
	beq	.L835
	ldr	w0, [sp, 20]
	add	w0, w0, 1
	str	w0, [sp, 20]
.L835:
	ldrb	w0, [sp, 19]
	ubfiz	w0, w0, 1, 7
	strb	w0, [sp, 19]
	ldr	w0, [sp, 24]
	add	w0, w0, 1
	str	w0, [sp, 24]
.L834:
	ldr	w0, [sp, 24]
	cmp	w0, 7
	ble	.L836
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L833:
	ldr	w1, [sp, 28]
	ldr	w0, [sp, 4]
	cmp	w1, w0
	blt	.L837
	ldr	w0, [sp, 20]
	add	sp, sp, 32
	ret
	.size	check_bit_nums, .-check_bit_nums
	.align	2
	.global	is_all_bit_ff
	.type	is_all_bit_ff, %function
is_all_bit_ff:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	ldr	w0, [x29, 20]
	lsl	w0, w0, 3
	str	w0, [x29, 44]
	ldr	w1, [x29, 44]
	ldr	w0, [x29, 16]
	sub	w0, w1, w0
	str	w0, [x29, 44]
	ldr	w1, [x29, 20]
	ldr	x0, [x29, 24]
	bl	check_bit_nums
	str	w0, [x29, 40]
	ldr	w1, [x29, 40]
	ldr	w0, [x29, 44]
	cmp	w1, w0
	blt	.L840
	mov	w0, 1
	b	.L841
.L840:
	mov	w0, 0
.L841:
	ldp	x29, x30, [sp], 48
	ret
	.size	is_all_bit_ff, .-is_all_bit_ff
	.align	2
	.global	is_all_byte_ff
	.type	is_all_byte_ff, %function
is_all_byte_ff:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	str	w2, [sp]
	str	wzr, [sp, 28]
	ldr	w1, [sp, 4]
	ldr	w0, [sp]
	sub	w0, w1, w0
	str	w0, [sp, 20]
	str	wzr, [sp, 24]
	b	.L843
.L845:
	ldrsw	x0, [sp, 24]
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L844
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L844:
	ldr	w0, [sp, 24]
	add	w0, w0, 1
	str	w0, [sp, 24]
.L843:
	ldr	w1, [sp, 24]
	ldr	w0, [sp, 4]
	cmp	w1, w0
	blt	.L845
	ldr	w1, [sp, 28]
	ldr	w0, [sp, 20]
	cmp	w1, w0
	blt	.L846
	mov	w0, 1
	b	.L847
.L846:
	mov	w0, 0
.L847:
	add	sp, sp, 32
	ret
	.size	is_all_byte_ff, .-is_all_byte_ff
	.section	.rodata
	.align	3
.LC176:
	.string	"[NE]current_block ecc error %d!\n"
	.align	3
.LC177:
	.string	"[NE]error! no free block!\n"
	.align	3
.LC178:
	.string	"[ND]recover %d %d\n"
	.align	3
.LC179:
	.string	"[NE]page used:%d invalid page count:%d error\n"
	.align	3
.LC180:
	.string	"[NE]new_block_init_for_write error here!\n"
	.align	3
.LC181:
	.string	"[NE]something is error here 10!\n"
	.align	3
.LC182:
	.string	"[NE]something is error here 11!\n"
	.align	3
.LC183:
	.string	"[NE]assist block page used:%d current block page used:%d something is error here 12.\n"
	.align	3
.LC184:
	.string	"[NE]assist block page used:%d current block page used:%d something is error here 17.\n"
	.align	3
.LC185:
	.string	"[NE]something is error here 16!\n"
	.align	3
.LC186:
	.string	"[ND]get a new free block\n"
	.align	3
.LC187:
	.string	"[NE]no free block to use!\n"
	.text
	.align	2
	.global	get_current_used_block
	.type	get_current_used_block, %function
get_current_used_block:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	str	wzr, [x29, 56]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	cmp	x0, 0
	beq	.L849
	mov	w0, 0
	b	.L850
.L849:
	ldr	x0, [x29, 32]
	cmp	x0, 0
	beq	.L851
.L852:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 552]
	mov	x2, x0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	recover_phy_page_mapping
	uxth	w1, w0
	ldr	x0, [x29, 40]
	strh	w1, [x0, 584]
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 584]
	mov	w0, 65535
	cmp	w1, w0
	bne	.L853
	mov	w0, 1
	str	w0, [x29, 56]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC176
	add	x0, x0, :lo12:.LC176
	bl	nand_print
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	current_block_ecc_error
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	beq	.L854
	b	.L852
.L854:
	adrp	x0, .LC177
	add	x0, x0, :lo12:.LC177
	bl	nand_print
	mov	w0, 1
	b	.L850
.L853:
	ldr	x0, [x29, 40]
	ldr	x1, [x29, 32]
	str	x1, [x0, 528]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	mov	w2, w0
	adrp	x0, .LC178
	add	x0, x0, :lo12:.LC178
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 528]
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 552]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	mov	w3, w0
	ldr	x0, [x29, 40]
	bl	recover_logic_page_mapping
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 584]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	ldrh	w0, [x0, 4]
	cmp	w1, w0
	bcs	.L855
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	ldrh	w0, [x0, 4]
	mov	w2, w0
	adrp	x0, .LC179
	add	x0, x0, :lo12:.LC179
	bl	nand_print
.L855:
	ldr	x0, [x29, 40]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L856
.L857:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	cmp	x0, 0
	bne	.L858
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	cmp	x0, 0
	bne	.L859
	mov	w0, 1
	b	.L850
.L859:
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 648]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	ldr	x1, [x29, 40]
	ldr	x1, [x1, 528]
	ldr	w1, [x1, 8]
	add	w1, w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 40]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L860
	adrp	x0, .LC180
	add	x0, x0, :lo12:.LC180
	bl	nand_print
	ldr	x0, [x29, 40]
	str	xzr, [x0, 592]
	b	.L857
.L860:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	str	w0, [x29, 52]
	str	wzr, [x29, 60]
	b	.L861
.L863:
	ldr	x0, [x29, 40]
	ldr	x5, [x0, 3488]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 528]
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 512]
	ldr	w3, [x29, 60]
	uxth	w3, w3
	mov	w4, w3
	mov	x3, x0
	ldr	x0, [x29, 40]
	blr	x5
	str	w0, [x29, 48]
	ldr	w0, [x29, 48]
	cmp	w0, 0
	beq	.L862
	adrp	x0, .LC181
	add	x0, x0, :lo12:.LC181
	bl	nand_print
.L862:
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L861:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 52]
	cmp	w1, w0
	bcc	.L863
.L858:
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 616]
	mov	x2, x0
	ldr	x0, [x29, 40]
	bl	recover_phy_page_mapping
	uxth	w1, w0
	ldr	x0, [x29, 40]
	strh	w1, [x0, 648]
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 648]
	mov	w0, 65535
	cmp	w1, w0
	bne	.L864
	adrp	x0, .LC182
	add	x0, x0, :lo12:.LC182
	bl	nand_print
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 40]
	str	xzr, [x0, 592]
	b	.L857
.L864:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	ldr	x1, [x29, 40]
	ldr	x1, [x1, 528]
	ldr	w1, [x1, 8]
	add	w1, w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 616]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 648]
	mov	w3, w0
	ldr	x0, [x29, 40]
	bl	recover_logic_page_mapping
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 648]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	cmp	w1, w0
	bls	.L865
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 648]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	mov	w2, w0
	adrp	x0, .LC183
	add	x0, x0, :lo12:.LC183
	bl	nand_print
	ldr	w0, [x29, 56]
	cmp	w0, 0
	beq	.L866
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 528]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 40]
	str	x1, [x0, 528]
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 648]
	ldr	x0, [x29, 40]
	strh	w1, [x0, 584]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 536]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 600]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 544]
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
	ldr	x0, [x29, 40]
	str	xzr, [x0, 592]
	b	.L857
.L866:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 648]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	mov	w2, w0
	adrp	x0, .LC184
	add	x0, x0, :lo12:.LC184
	bl	nand_print
	b	.L856
.L865:
	ldr	x0, [x29, 40]
	ldrh	w1, [x0, 648]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 584]
	cmp	w1, w0
	bcs	.L856
	adrp	x0, .LC185
	add	x0, x0, :lo12:.LC185
	bl	nand_print
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 40]
	str	xzr, [x0, 592]
	b	.L857
.L851:
	adrp	x0, .LC186
	add	x0, x0, :lo12:.LC186
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 304]
	cmp	x0, 0
	beq	.L867
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 528]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	cmp	x0, 0
	bne	.L868
	mov	w0, 1
	b	.L850
.L868:
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 584]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 528]
	ldr	w1, [x29, 28]
	add	w1, w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 40]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L869
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 592]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	cmp	x0, 0
	bne	.L870
	mov	w0, 1
	b	.L850
.L870:
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 648]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 592]
	ldr	x1, [x29, 40]
	ldr	x1, [x1, 528]
	ldr	w1, [x1, 8]
	add	w1, w1, 1
	str	w1, [x0, 8]
	b	.L856
.L869:
	ldr	x0, [x29, 40]
	str	xzr, [x0, 592]
	b	.L856
.L867:
	adrp	x0, .LC187
	add	x0, x0, :lo12:.LC187
	bl	nand_print
	ldr	x0, [x29, 40]
	bl	print_free_list
	ldr	x0, [x29, 40]
	bl	print_block_invalid_list
	mov	w0, 1
	b	.L850
.L856:
	mov	w0, 0
.L850:
	ldp	x29, x30, [sp], 64
	ret
	.size	get_current_used_block, .-get_current_used_block
	.section	.rodata
	.align	3
.LC188:
	.string	"[NE]sb@%d sp@%d recover_phy_page_mapping no used page\n"
	.align	3
.LC189:
	.string	"[NE]sb@%d sp@%d power off ecc error1.\n"
	.align	3
.LC190:
	.string	"[NE]invalid logic_page block:%d page:%d,logic_page 0x%x.\n"
	.align	3
.LC191:
	.string	"[NE]rppm invalid page %d!!\n"
	.align	3
.LC192:
	.string	"[NI]power_down page %d!!\n"
	.align	3
.LC193:
	.string	"[NE]recover_phy_page_mapping unkown page data %d!!\n"
	.align	3
.LC194:
	.string	"[NE]recover_phy_page_mapping ecc error block:%d;page:%d.\n"
	.text
	.align	2
	.global	recover_phy_page_mapping
	.type	recover_phy_page_mapping, %function
recover_phy_page_mapping:
	stp	x29, x30, [sp, -112]!
	add	x29, sp, 0
	str	x19, [sp, 16]
	str	x0, [x29, 56]
	str	x1, [x29, 48]
	str	x2, [x29, 40]
	ldr	x0, [x29, 56]
	ldr	w0, [x0, 560]
	mov	w2, w0
	mov	w1, -1
	ldr	x0, [x29, 40]
	bl	nand_memset
	str	wzr, [x29, 108]
	b	.L873
.L888:
	ldr	x0, [x29, 48]
	ldrh	w1, [x0]
	ldr	w0, [x29, 108]
	uxth	w2, w0
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 56]
	ldr	x4, [x0, 512]
	add	x5, x29, 88
	add	x0, x29, 64
	bl	set_physic_op_par
	ldr	x0, [x29, 56]
	ldr	x2, [x0, 3472]
	add	x0, x29, 64
	mov	x1, x0
	ldr	x0, [x29, 56]
	blr	x2
	str	w0, [x29, 104]
	add	x0, x29, 88
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L874
	ldr	w0, [x29, 104]
	cmp	w0, 0
	beq	.L875
	ldr	w0, [x29, 104]
	cmp	w0, 10
	bne	.L874
.L875:
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC188
	add	x0, x0, :lo12:.LC188
	ldr	w2, [x29, 108]
	bl	nand_print
	b	.L876
.L874:
	add	x0, x29, 88
	bl	is_ftl_logic_page_data
	cmp	w0, 1
	bne	.L877
	ldr	w0, [x29, 104]
	cmp	w0, 0
	beq	.L878
	ldr	w0, [x29, 104]
	cmp	w0, 10
	bne	.L881
.L878:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 512]
	add	x1, x29, 88
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 56]
	bl	nand_power_off_ecc_error
	cmp	w0, 0
	beq	.L880
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC189
	add	x0, x0, :lo12:.LC189
	ldr	w2, [x29, 108]
	bl	nand_print
	mov	w0, -2
	str	w0, [x29, 104]
	b	.L881
.L880:
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x19, x1, x0
	add	x0, x29, 88
	bl	get_logic_page_from_oob
	str	w0, [x19]
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	ldr	w1, [x0]
	ldr	x0, [x29, 56]
	ldr	w0, [x0, 36]
	cmp	w1, w0
	bcc	.L881
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w4, w0
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	ldr	w1, [x0]
	adrp	x0, .LC190
	add	x0, x0, :lo12:.LC190
	mov	w3, w1
	ldr	w2, [x29, 108]
	mov	w1, w4
	bl	nand_print
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	mov	w1, -1
	str	w1, [x0]
	b	.L881
.L877:
	add	x0, x29, 88
	bl	is_ftl_special_data
	cmp	w0, 1
	bne	.L882
	adrp	x0, .LC191
	add	x0, x0, :lo12:.LC191
	ldr	w1, [x29, 108]
	bl	nand_print
	b	.L881
.L882:
	add	x0, x29, 88
	bl	is_power_down_info_page
	cmp	w0, 1
	bne	.L883
	ldr	w0, [x29, 104]
	cmp	w0, 0
	beq	.L884
	ldr	w0, [x29, 104]
	cmp	w0, 10
	bne	.L881
.L884:
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x19, x1, x0
	add	x0, x29, 88
	bl	get_special_data_from_oob
	str	w0, [x19]
	adrp	x0, .LC192
	add	x0, x0, :lo12:.LC192
	ldr	w1, [x29, 108]
	bl	nand_print
	b	.L881
.L883:
	adrp	x0, .LC193
	add	x0, x0, :lo12:.LC193
	ldr	w1, [x29, 108]
	bl	nand_print
.L881:
	ldr	w0, [x29, 104]
	cmp	w0, 0
	beq	.L886
	ldr	w0, [x29, 104]
	cmp	w0, 10
	beq	.L886
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC194
	add	x0, x0, :lo12:.LC194
	ldr	w2, [x29, 108]
	bl	nand_print
	mov	w0, 65535
	b	.L890
.L886:
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
.L873:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 108]
	cmp	w1, w0
	bgt	.L888
.L876:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w1, w0, #2
	ldr	w0, [x29, 108]
	cmp	w1, w0
	ldr	w0, [x29, 108]
.L890:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 112
	ret
	.size	recover_phy_page_mapping, .-recover_phy_page_mapping
	.section	.rodata
	.align	3
.LC195:
	.string	"[NE]ret:%d read fail %d %d,read one more time!!\n"
	.align	3
.LC196:
	.string	"[NE]ret:%d read fail %d %d,%d times,don't read more!!\n"
	.align	3
.LC197:
	.string	"[NE]power off ecc error 3!!\n"
	.align	3
.LC198:
	.string	"[NE]recover sb@%d sp@%d phy page mapping error.\n"
	.text
	.align	2
	.global	recover_block_phy_page_mapping
	.type	recover_block_phy_page_mapping, %function
recover_block_phy_page_mapping:
	stp	x29, x30, [sp, -128]!
	add	x29, sp, 0
	str	x19, [sp, 16]
	str	x0, [x29, 56]
	str	x1, [x29, 48]
	str	x2, [x29, 40]
	str	wzr, [x29, 116]
	ldr	x0, [x29, 56]
	ldr	w0, [x0, 560]
	mov	w2, w0
	mov	w1, -1
	ldr	x0, [x29, 40]
	bl	nand_memset
	str	wzr, [x29, 124]
	b	.L892
.L906:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 28]
	cmp	w0, 0
	beq	.L893
	ldr	x0, [x29, 48]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 56]
	ldr	x4, [x0, 512]
	add	x5, x29, 96
	add	x0, x29, 72
	bl	set_physic_op_par
	b	.L894
.L893:
	ldr	x0, [x29, 48]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	add	x3, x29, 96
	add	x0, x29, 72
	mov	x5, x3
	mov	x4, 0
	mov	w3, 0
	bl	set_physic_op_par
.L894:
	str	wzr, [x29, 116]
	b	.L895
.L900:
	ldr	x0, [x29, 56]
	ldr	x2, [x0, 3472]
	add	x0, x29, 72
	mov	x1, x0
	ldr	x0, [x29, 56]
	blr	x2
	str	w0, [x29, 120]
	ldr	w0, [x29, 120]
	cmp	w0, 0
	bge	.L908
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC195
	add	x0, x0, :lo12:.LC195
	ldr	w3, [x29, 124]
	mov	w2, w1
	ldr	w1, [x29, 120]
	bl	nand_print
	ldr	w0, [x29, 116]
	cmp	w0, 1
	bne	.L899
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC196
	add	x0, x0, :lo12:.LC196
	ldr	w4, [x29, 116]
	ldr	w3, [x29, 124]
	mov	w2, w1
	ldr	w1, [x29, 120]
	bl	nand_print
.L899:
	ldr	w0, [x29, 116]
	add	w0, w0, 1
	str	w0, [x29, 116]
.L895:
	ldr	w0, [x29, 116]
	cmp	w0, 1
	ble	.L900
	b	.L898
.L908:
	nop
.L898:
	add	x0, x29, 96
	bl	is_ftl_logic_page_data
	cmp	w0, 1
	bne	.L901
	ldr	w0, [x29, 120]
	cmp	w0, 0
	beq	.L902
	ldr	w0, [x29, 120]
	cmp	w0, 10
	bne	.L905
.L902:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 512]
	add	x1, x29, 96
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 56]
	bl	nand_power_off_ecc_error
	cmp	w0, 0
	beq	.L904
	adrp	x0, .LC197
	add	x0, x0, :lo12:.LC197
	bl	nand_print
	b	.L905
.L904:
	ldrsw	x0, [x29, 124]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x19, x1, x0
	add	x0, x29, 96
	bl	get_logic_page_from_oob
	str	w0, [x19]
	ldrsw	x0, [x29, 124]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	ldr	w1, [x0]
	ldr	x0, [x29, 56]
	ldr	w0, [x0, 36]
	cmp	w1, w0
	bcc	.L905
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w4, w0
	ldrsw	x0, [x29, 124]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	ldr	w1, [x0]
	adrp	x0, .LC190
	add	x0, x0, :lo12:.LC190
	mov	w3, w1
	ldr	w2, [x29, 124]
	mov	w1, w4
	bl	nand_print
	ldrsw	x0, [x29, 124]
	lsl	x0, x0, 2
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	mov	w1, -1
	str	w1, [x0]
	b	.L905
.L901:
	add	x0, x29, 96
	bl	is_nouse_page
	cmp	w0, 1
	beq	.L905
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC198
	add	x0, x0, :lo12:.LC198
	ldr	w2, [x29, 124]
	bl	nand_print
.L905:
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
.L892:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w1, w0, #1
	ldr	w0, [x29, 124]
	cmp	w1, w0
	bgt	.L906
	mov	w0, 0
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 128
	ret
	.size	recover_block_phy_page_mapping, .-recover_block_phy_page_mapping
	.section	.rodata
	.align	3
.LC199:
	.string	"[NI]logic_page_map is NULL\n"
	.align	3
.LC200:
	.string	"[NE]invalid_page_count more than pages_per_blk :%d\n"
	.align	3
.LC201:
	.string	"[NE]invalid_page data block:%d page:%d,logic_page 0x%x!!\n"
	.align	3
.LC202:
	.string	"[NE]build invalid_page_count more than pages_per_blk!!\n"
	.text
	.align	2
	.global	recover_logic_page_mapping
	.type	recover_logic_page_mapping, %function
recover_logic_page_mapping:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 76]
	b	.L910
.L918:
	ldr	w0, [x29, 76]
	lsl	x0, x0, 2
	ldr	x1, [x29, 24]
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [x29, 72]
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 36]
	ldr	w0, [x29, 72]
	cmp	w1, w0
	bls	.L911
	ldr	w1, [x29, 72]
	ldr	x0, [x29, 40]
	bl	get_logic_page_map
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	bne	.L912
	adrp	x0, .LC199
	add	x0, x0, :lo12:.LC199
	bl	nand_print
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 32]
	strh	w1, [x0, 4]
	b	.L913
.L912:
	ldr	x0, [x29, 64]
	ldrh	w1, [x0, 2]
	mov	w0, 65535
	cmp	w1, w0
	beq	.L914
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	get_phy_block_addr
	str	x0, [x29, 56]
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 32]
	cmp	x1, x0
	beq	.L915
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 40]
	bl	phy_block_from_invalid_page_incr
	b	.L916
.L915:
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 56]
	strh	w1, [x0, 4]
.L916:
	ldr	x0, [x29, 56]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bls	.L914
	ldr	x0, [x29, 56]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC200
	add	x0, x0, :lo12:.LC200
	bl	nand_print
.L914:
	ldr	w0, [x29, 76]
	uxth	w0, w0
	and	w0, w0, 4095
	uxth	w2, w0
	ldr	x1, [x29, 64]
	ldrh	w0, [x1]
	bfi	w0, w2, 0, 12
	strh	w0, [x1]
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	x0, [x29, 64]
	strh	w1, [x0, 2]
	b	.L913
.L911:
	ldr	w0, [x29, 72]
	cmn	w0, #1
	beq	.L917
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC201
	add	x0, x0, :lo12:.LC201
	ldr	w3, [x29, 72]
	ldr	w2, [x29, 76]
	bl	nand_print
.L917:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 32]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 32]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	cmp	w1, w0
	bls	.L913
	adrp	x0, .LC202
	add	x0, x0, :lo12:.LC202
	bl	nand_print
.L913:
	ldr	w0, [x29, 76]
	add	w0, w0, 1
	str	w0, [x29, 76]
.L910:
	ldr	w1, [x29, 76]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L918
	mov	w0, 0
	ldp	x29, x30, [sp], 80
	ret
	.size	recover_logic_page_mapping, .-recover_logic_page_mapping
	.section	.rodata
	.align	3
.LC203:
	.string	"[NE]NAND EVENT:do_write_error erase error1!\n"
	.align	3
.LC204:
	.string	"[NE]NAND EVENT:do_write_error erase error2!\n"
	.text
	.align	2
	.global	do_write_error_in_build_list
	.type	do_write_error_in_build_list, %function
do_write_error_in_build_list:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	strh	w3, [x29, 22]
	ldr	x0, [x29, 24]
	str	x0, [x29, 80]
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 48
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3464]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 76]
	ldr	x0, [x29, 80]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 80]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 80]
	strh	w1, [x0, 6]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L921
	adrp	x0, .LC203
	add	x0, x0, :lo12:.LC203
	bl	nand_print
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3504]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	mov	w0, 1
	b	.L927
.L921:
	str	wzr, [x29, 92]
	b	.L923
.L925:
	ldr	x0, [x29, 40]
	ldr	x5, [x0, 3488]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 512]
	ldr	w1, [x29, 92]
	uxth	w1, w1
	mov	w4, w1
	mov	x3, x0
	ldr	x2, [x29, 80]
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	blr	x5
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L924
	adrp	x0, .LC204
	add	x0, x0, :lo12:.LC204
	bl	nand_print
.L924:
	ldr	w0, [x29, 92]
	add	w0, w0, 1
	str	w0, [x29, 92]
.L923:
	ldrh	w1, [x29, 22]
	ldr	w0, [x29, 92]
	cmp	w1, w0
	bhi	.L925
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 48
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3504]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldrh	w0, [x29, 22]
	cmp	w0, 0
	bne	.L926
	ldr	x0, [x29, 40]
	ldr	x1, [x29, 24]
	str	x1, [x0, 528]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 584]
.L926:
	mov	w0, 0
.L927:
	ldp	x29, x30, [sp], 96
	ret
	.size	do_write_error_in_build_list, .-do_write_error_in_build_list
	.section	.rodata
	.align	3
.LC205:
	.string	"%s %d:fail 1\n"
	.align	3
.LC206:
	.string	"%s %d:fail 2\n"
	.align	3
.LC207:
	.string	"%s %d:fail 3\n"
	.align	3
.LC208:
	.string	"%s %d:fail 4\n"
	.align	3
.LC209:
	.string	"[NE]pages_per_blk: %d, %p, %p, %p, %p\n"
	.text
	.align	2
	.global	init_nrc
	.type	init_nrc, %function
init_nrc:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	str	wzr, [x29, 44]
	str	xzr, [x29, 32]
	mov	w0, 32
	bl	nftl_malloc
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L929
	adrp	x0, __func__.6287
	add	x1, x0, :lo12:__func__.6287
	adrp	x0, .LC205
	add	x0, x0, :lo12:.LC205
	mov	w2, 1408
	bl	nand_print
	mov	w0, -1
	b	.L930
.L929:
	mov	w2, 32
	mov	w1, 0
	ldr	x0, [x29, 32]
	bl	nand_memset
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w1, w0, 64
	ldr	w0, [x29, 44]
	mul	w0, w1, w0
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 32]
	str	x1, [x0, 16]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L931
	adrp	x0, __func__.6287
	add	x1, x0, :lo12:__func__.6287
	adrp	x0, .LC206
	add	x0, x0, :lo12:.LC206
	mov	w2, 1417
	bl	nand_print
	b	.L932
.L931:
	ldrsw	x0, [x29, 44]
	lsl	w0, w0, 2
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 32]
	str	x1, [x0, 24]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	bne	.L933
	adrp	x0, __func__.6287
	add	x1, x0, :lo12:__func__.6287
	adrp	x0, .LC207
	add	x0, x0, :lo12:.LC207
	mov	w2, 1422
	bl	nand_print
	b	.L932
.L933:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	ldrsw	x1, [x29, 44]
	lsl	w1, w1, 2
	mov	w2, w1
	mov	w1, -1
	bl	nand_memset
	ldr	w0, [x29, 44]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 32]
	str	x1, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	cmp	x0, 0
	bne	.L934
	adrp	x0, __func__.6287
	add	x1, x0, :lo12:__func__.6287
	adrp	x0, .LC208
	add	x0, x0, :lo12:.LC208
	mov	w2, 1428
	bl	nand_print
	b	.L932
.L934:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldr	w1, [x29, 44]
	mov	w2, w1
	mov	w1, 0
	bl	nand_memset
	ldr	x0, [x29, 16]
	ldr	x1, [x29, 32]
	str	x1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w6, w0
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 32]
	ldr	x2, [x0, 24]
	ldr	x0, [x29, 32]
	ldr	x3, [x0, 8]
	adrp	x0, .LC209
	add	x0, x0, :lo12:.LC209
	mov	x5, x3
	mov	x4, x2
	mov	x3, x1
	ldr	x2, [x29, 32]
	mov	w1, w6
	bl	nand_print
	mov	w0, 0
	b	.L930
.L932:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	beq	.L935
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 16]
	bl	nftl_free
.L935:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	beq	.L936
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	bl	nftl_free
.L936:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	cmp	x0, 0
	beq	.L937
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	bl	nftl_free
.L937:
	ldr	x0, [x29, 32]
	bl	nftl_free
	ldr	x0, [x29, 16]
	str	xzr, [x0]
	mov	w0, -1
.L930:
	ldp	x29, x30, [sp], 48
	ret
	.size	init_nrc, .-init_nrc
	.align	2
	.global	free_nrc
	.type	free_nrc, %function
free_nrc:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L939
	mov	w0, 0
	b	.L940
.L939:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 16]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	bl	nftl_free
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	nftl_free
	ldr	x0, [x29, 24]
	bl	nftl_free
	mov	w0, 0
.L940:
	ldp	x29, x30, [sp], 32
	ret
	.size	free_nrc, .-free_nrc
	.section	.rodata
	.align	3
.LC210:
	.string	"[NE]cross_talk_rebuild_current_block fail 1\n"
	.align	3
.LC211:
	.string	"[NE]cross_talk_rebuild_current_block!! %d,%d,%d!\n"
	.align	3
.LC212:
	.string	"[NE]cross_talk_rebuild_current_block fail 2\n"
	.align	3
.LC213:
	.string	"[NE]cross_talk_rebuild_current_block!! %d,%d,%d,%d,%d,%d!\n"
	.align	3
.LC214:
	.string	"[NE]cross_talk_rebuild_current_block fail 3\n"
	.align	3
.LC215:
	.string	"[NE]corss talk rebuild 1 %d %d %d!\n"
	.align	3
.LC216:
	.string	"[NE]corss talk rebuild 2 %d %d %d!\n"
	.align	3
.LC217:
	.string	"[NE]get free block fail!\n"
	.align	3
.LC218:
	.string	"[NE]something is error here 14!\n"
	.align	3
.LC219:
	.string	"[NE]page lost %d %d\n"
	.text
	.align	2
	.global	cross_talk_rebuild_current_block
	.type	cross_talk_rebuild_current_block, %function
cross_talk_rebuild_current_block:
	stp	x29, x30, [sp, -176]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	wzr, [x29, 160]
	str	xzr, [x29, 56]
	str	xzr, [x29, 144]
	str	xzr, [x29, 136]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	init_nrc
	cmp	w0, 0
	beq	.L942
	ldr	x0, [x29, 56]
	bl	free_nrc
	adrp	x0, .LC210
	add	x0, x0, :lo12:.LC210
	bl	nand_print
	mov	x0, 0
	b	.L960
.L942:
	ldr	x1, [x29, 56]
	add	x0, x29, 112
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	get_used_page_num
	str	w0, [x29, 132]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L944
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w4, w0
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 8]
	adrp	x0, .LC211
	add	x0, x0, :lo12:.LC211
	mov	w3, w1
	ldr	w2, [x29, 132]
	mov	w1, w4
	bl	nand_print
	ldr	x0, [x29, 32]
	str	x0, [x29, 152]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 164]
	ldr	x0, [x29, 56]
	ldr	w0, [x0]
	cmp	w0, 0
	bne	.L945
	ldr	x0, [x29, 56]
	bl	free_nrc
	adrp	x0, .LC212
	add	x0, x0, :lo12:.LC212
	bl	nand_print
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	erase_block
	mov	x0, 0
	b	.L960
.L945:
	ldr	w0, [x29, 112]
	cmp	w0, 0
	beq	.L947
	ldr	w0, [x29, 164]
	add	w0, w0, 20
	str	w0, [x29, 164]
	b	.L947
.L944:
	ldr	x1, [x29, 56]
	add	x0, x29, 108
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	get_used_page_num
	str	w0, [x29, 128]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w7, w0
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 8]
	adrp	x0, .LC213
	add	x0, x0, :lo12:.LC213
	mov	w6, w2
	ldr	w5, [x29, 128]
	mov	w4, w3
	mov	w3, w1
	ldr	w2, [x29, 132]
	mov	w1, w7
	bl	nand_print
	ldr	x0, [x29, 56]
	ldr	w0, [x0]
	cmp	w0, 0
	bne	.L948
	ldr	x0, [x29, 56]
	bl	free_nrc
	adrp	x0, .LC214
	add	x0, x0, :lo12:.LC214
	bl	nand_print
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	erase_block
	mov	x0, 0
	b	.L960
.L948:
	ldr	w1, [x29, 128]
	ldr	w0, [x29, 132]
	cmp	w1, w0
	bcc	.L949
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 164]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 160]
	b	.L950
.L949:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 164]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	str	w0, [x29, 160]
.L950:
	ldr	w0, [x29, 112]
	cmp	w0, 0
	bne	.L951
	ldr	w0, [x29, 108]
	cmp	w0, 0
	beq	.L952
.L951:
	ldr	w0, [x29, 164]
	add	w0, w0, 20
	str	w0, [x29, 164]
	ldr	w0, [x29, 160]
	add	w0, w0, 20
	str	w0, [x29, 160]
.L952:
	ldr	w1, [x29, 128]
	ldr	w0, [x29, 132]
	cmp	w1, w0
	bls	.L953
	ldr	x0, [x29, 56]
	ldr	w1, [x0]
	adrp	x0, .LC215
	add	x0, x0, :lo12:.LC215
	mov	w3, w1
	ldr	w2, [x29, 128]
	ldr	w1, [x29, 132]
	bl	nand_print
	ldr	w0, [x29, 164]
	uxth	w0, w0
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 24]
	str	x0, [x29, 152]
	b	.L947
.L953:
	ldr	x0, [x29, 56]
	ldr	w1, [x0]
	adrp	x0, .LC216
	add	x0, x0, :lo12:.LC216
	mov	w3, w1
	ldr	w2, [x29, 128]
	ldr	w1, [x29, 132]
	bl	nand_print
	ldr	w0, [x29, 164]
	uxth	w0, w0
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 32]
	str	x0, [x29, 152]
.L947:
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 120]
	ldr	x0, [x29, 120]
	cmp	x0, 0
	bne	.L954
	adrp	x0, .LC217
	add	x0, x0, :lo12:.LC217
	bl	nand_print
	mov	x0, 0
	b	.L960
.L954:
	ldr	x0, [x29, 152]
	ldr	w0, [x0, 8]
	add	w1, w0, 1
	ldr	x0, [x29, 120]
	str	w1, [x0, 8]
	ldr	x0, [x29, 120]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x1, [x29, 120]
	ldr	x0, [x29, 40]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L955
	adrp	x0, .LC218
	add	x0, x0, :lo12:.LC218
	bl	nand_print
	b	.L947
.L955:
	str	wzr, [x29, 172]
	str	wzr, [x29, 168]
	b	.L956
.L959:
	ldr	x0, [x29, 56]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 172]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	beq	.L957
	ldr	x0, [x29, 56]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w0, w0, 64
	mov	w2, w0
	ldr	w0, [x29, 172]
	mul	w0, w2, w0
	uxtw	x0, w0
	add	x0, x1, x0
	str	x0, [x29, 136]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	ldr	x1, [x29, 136]
	add	x0, x1, x0
	str	x0, [x29, 144]
	ldr	x0, [x29, 120]
	ldrh	w1, [x0]
	ldr	w0, [x29, 168]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x0, x29, 64
	ldr	x5, [x29, 144]
	ldr	x4, [x29, 136]
	bl	set_physic_op_par
	ldr	x0, [x29, 144]
	bl	get_special_data_from_oob
	str	w0, [x29, 116]
	ldr	x0, [x29, 120]
	ldr	w0, [x0, 8]
	mov	w1, w0
	ldr	x0, [x29, 120]
	ldrh	w0, [x0, 6]
	mov	w4, w0
	mov	w3, w1
	ldr	w2, [x29, 116]
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 40]
	bl	set_oob_special_page
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3480]
	add	x0, x29, 64
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldr	w0, [x29, 168]
	add	w0, w0, 1
	str	w0, [x29, 168]
	b	.L958
.L957:
	ldr	x0, [x29, 56]
	ldr	w1, [x0]
	adrp	x0, .LC219
	add	x0, x0, :lo12:.LC219
	mov	w2, w1
	ldr	w1, [x29, 172]
	bl	nand_print
.L958:
	ldr	w0, [x29, 172]
	add	w0, w0, 1
	str	w0, [x29, 172]
.L956:
	ldr	x0, [x29, 56]
	ldr	w1, [x0]
	ldr	w0, [x29, 172]
	cmp	w1, w0
	bhi	.L959
	ldr	w0, [x29, 160]
	uxth	w0, w0
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 56]
	bl	free_nrc
	ldr	x0, [x29, 120]
.L960:
	ldp	x29, x30, [sp], 176
	ret
	.size	cross_talk_rebuild_current_block, .-cross_talk_rebuild_current_block
	.section	.rodata
	.align	3
.LC220:
	.string	"[NI]nctrcb1 %d,%d,%d!\n"
	.align	3
.LC221:
	.string	"[NE]no_cross_talk_rebuild_current_block fail 2\n"
	.align	3
.LC222:
	.string	"[NI]nctrcb2 %d,%d,%d,%d,%d,%d!\n"
	.align	3
.LC223:
	.string	"[NE]no_cross_talk_rebuild_current_block fail 4\n"
	.align	3
.LC224:
	.string	"[NE]no_cross_talk_rebuild_current_block erase block fail %d!\n"
	.text
	.align	2
	.global	no_cross_talk_rebuild_current_block
	.type	no_cross_talk_rebuild_current_block, %function
no_cross_talk_rebuild_current_block:
	stp	x29, x30, [sp, -192]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	xzr, [x29, 152]
	str	xzr, [x29, 144]
	str	xzr, [x29, 64]
	str	xzr, [x29, 56]
	str	xzr, [x29, 160]
	add	x0, x29, 64
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	init_nrc
	cmp	w0, 0
	beq	.L962
	ldr	x0, [x29, 64]
	bl	free_nrc
	adrp	x0, .LC210
	add	x0, x0, :lo12:.LC210
	bl	nand_print
	mov	x0, 0
	b	.L980
.L962:
	ldr	x1, [x29, 64]
	add	x0, x29, 124
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	get_used_page_num_no_crosstalk
	str	w0, [x29, 140]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L964
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w4, w0
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 8]
	adrp	x0, .LC220
	add	x0, x0, :lo12:.LC220
	mov	w3, w1
	ldr	w2, [x29, 140]
	mov	w1, w4
	bl	nand_print
	ldr	x0, [x29, 32]
	str	x0, [x29, 176]
	ldr	x0, [x29, 64]
	ldr	w0, [x0]
	cmp	w0, 0
	bne	.L965
	ldr	x0, [x29, 64]
	bl	free_nrc
	adrp	x0, .LC221
	add	x0, x0, :lo12:.LC221
	bl	nand_print
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	erase_block
	mov	x0, 0
	b	.L980
.L965:
	ldr	x0, [x29, 64]
	str	x0, [x29, 160]
.L966:
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 168]
	ldr	x0, [x29, 168]
	cmp	x0, 0
	bne	.L967
	adrp	x0, .LC217
	add	x0, x0, :lo12:.LC217
	bl	nand_print
	mov	x0, 0
	b	.L980
.L967:
	ldr	x0, [x29, 176]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 168]
	str	w1, [x0, 8]
	ldr	x0, [x29, 168]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x1, [x29, 168]
	ldr	x0, [x29, 40]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L968
	adrp	x0, .LC218
	add	x0, x0, :lo12:.LC218
	bl	nand_print
	b	.L966
.L964:
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	init_nrc
	cmp	w0, 0
	beq	.L969
	ldr	x0, [x29, 64]
	bl	free_nrc
	ldr	x0, [x29, 56]
	bl	free_nrc
	adrp	x0, .LC214
	add	x0, x0, :lo12:.LC214
	bl	nand_print
	mov	x0, 0
	b	.L980
.L969:
	ldr	x1, [x29, 56]
	add	x0, x29, 120
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	get_used_page_num_no_crosstalk
	str	w0, [x29, 136]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w7, w0
	ldr	x0, [x29, 32]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 8]
	adrp	x0, .LC222
	add	x0, x0, :lo12:.LC222
	mov	w6, w2
	ldr	w5, [x29, 136]
	mov	w4, w3
	mov	w3, w1
	ldr	w2, [x29, 140]
	mov	w1, w7
	bl	nand_print
	ldr	x0, [x29, 56]
	ldr	w0, [x0]
	cmp	w0, 0
	bne	.L970
	ldr	x0, [x29, 64]
	bl	free_nrc
	ldr	x0, [x29, 56]
	bl	free_nrc
	adrp	x0, .LC223
	add	x0, x0, :lo12:.LC223
	bl	nand_print
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 24]
	ldr	x0, [x29, 40]
	bl	erase_block
	mov	x0, 0
	b	.L980
.L970:
	ldr	w1, [x29, 136]
	ldr	w0, [x29, 140]
	cmp	w1, w0
	bls	.L971
	ldr	x0, [x29, 24]
	str	x0, [x29, 176]
	ldr	x0, [x29, 32]
	str	x0, [x29, 168]
	ldr	x0, [x29, 56]
	str	x0, [x29, 160]
	ldr	x0, [x29, 160]
	ldr	w1, [x0]
	adrp	x0, .LC215
	add	x0, x0, :lo12:.LC215
	mov	w3, w1
	ldr	w2, [x29, 136]
	ldr	w1, [x29, 140]
	bl	nand_print
	b	.L972
.L971:
	ldr	x0, [x29, 32]
	str	x0, [x29, 176]
	ldr	x0, [x29, 24]
	str	x0, [x29, 168]
	ldr	x0, [x29, 64]
	str	x0, [x29, 160]
	ldr	x0, [x29, 160]
	ldr	w1, [x0]
	adrp	x0, .LC216
	add	x0, x0, :lo12:.LC216
	mov	w3, w1
	ldr	w2, [x29, 136]
	ldr	w1, [x29, 140]
	bl	nand_print
.L972:
	ldr	x0, [x29, 168]
	ldrh	w1, [x0]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 96
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3464]
	add	x0, x29, 96
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 132]
	ldr	w0, [x29, 132]
	cmp	w0, 0
	beq	.L973
	ldr	x0, [x29, 168]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC224
	add	x0, x0, :lo12:.LC224
	bl	nand_print
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3504]
	add	x0, x29, 96
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldr	x0, [x29, 176]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 176]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 40]
	strh	w1, [x0, 28]
.L974:
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 168]
	ldr	x0, [x29, 168]
	cmp	x0, 0
	bne	.L975
	adrp	x0, .LC217
	add	x0, x0, :lo12:.LC217
	bl	nand_print
	mov	x0, 0
	b	.L980
.L975:
	ldr	x0, [x29, 168]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x1, [x29, 168]
	ldr	x0, [x29, 40]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L973
	adrp	x0, .LC218
	add	x0, x0, :lo12:.LC218
	bl	nand_print
	b	.L974
.L973:
	ldr	x0, [x29, 176]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 168]
	str	w1, [x0, 8]
.L968:
	str	wzr, [x29, 188]
	str	wzr, [x29, 184]
	b	.L976
.L979:
	ldr	x0, [x29, 160]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 188]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	beq	.L977
	ldr	x0, [x29, 160]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w0, w0, 64
	mov	w2, w0
	ldr	w0, [x29, 188]
	mul	w0, w2, w0
	uxtw	x0, w0
	add	x0, x1, x0
	str	x0, [x29, 144]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	ldr	x1, [x29, 144]
	add	x0, x1, x0
	str	x0, [x29, 152]
	ldr	x0, [x29, 168]
	ldrh	w1, [x0]
	ldr	w0, [x29, 184]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x0, x29, 72
	ldr	x5, [x29, 152]
	ldr	x4, [x29, 144]
	bl	set_physic_op_par
	ldr	x0, [x29, 152]
	bl	get_special_data_from_oob
	str	w0, [x29, 128]
	ldr	x0, [x29, 168]
	ldr	w0, [x0, 8]
	mov	w1, w0
	ldr	x0, [x29, 168]
	ldrh	w0, [x0, 6]
	mov	w4, w0
	mov	w3, w1
	ldr	w2, [x29, 128]
	ldr	x1, [x29, 152]
	ldr	x0, [x29, 40]
	bl	set_oob_special_page
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3480]
	add	x0, x29, 72
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldr	w0, [x29, 184]
	add	w0, w0, 1
	str	w0, [x29, 184]
	b	.L978
.L977:
	ldr	x0, [x29, 160]
	ldr	w1, [x0]
	adrp	x0, .LC219
	add	x0, x0, :lo12:.LC219
	mov	w2, w1
	ldr	w1, [x29, 188]
	bl	nand_print
.L978:
	ldr	w0, [x29, 188]
	add	w0, w0, 1
	str	w0, [x29, 188]
.L976:
	ldr	x0, [x29, 160]
	ldr	w1, [x0]
	ldr	w0, [x29, 188]
	cmp	w1, w0
	bhi	.L979
	ldr	x0, [x29, 176]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 176]
	ldr	x0, [x29, 40]
	bl	erase_block
	ldr	x0, [x29, 64]
	bl	free_nrc
	ldr	x0, [x29, 56]
	bl	free_nrc
	ldr	x0, [x29, 168]
.L980:
	ldp	x29, x30, [sp], 192
	ret
	.size	no_cross_talk_rebuild_current_block, .-no_cross_talk_rebuild_current_block
	.section	.rodata
	.align	3
.LC225:
	.string	"[NI]erase_block block %d!\n"
	.align	3
.LC226:
	.string	"[NE]erase_block block fail %d!\n"
	.text
	.align	2
	.global	erase_block
	.type	erase_block, %function
erase_block:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	strh	w2, [x29, 30]
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC225
	add	x0, x0, :lo12:.LC225
	bl	nand_print
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 48
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3464]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L982
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC226
	add	x0, x0, :lo12:.LC226
	bl	nand_print
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3504]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldr	x0, [x29, 32]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 32]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 40]
	strh	w1, [x0, 28]
	b	.L983
.L982:
	ldr	x0, [x29, 32]
	ldrh	w1, [x29, 30]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 32]
	mov	w1, -1
	str	w1, [x0, 8]
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	put_phy_block_to_free_list
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	mark_free_block
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 20]
	ldr	x0, [x29, 32]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 32]
	strh	wzr, [x0, 2]
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_used_list_by_block
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_invalid_page_list_by_block
.L983:
	ldr	w0, [x29, 76]
	ldp	x29, x30, [sp], 80
	ret
	.size	erase_block, .-erase_block
	.align	2
	.global	nftl_check_pattern
	.type	nftl_check_pattern, %function
nftl_check_pattern:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	strb	w1, [sp, 7]
	str	w2, [sp]
	str	wzr, [sp, 28]
	b	.L986
.L989:
	ldrsw	x0, [sp, 28]
	ldr	x1, [sp, 8]
	add	x0, x1, x0
	ldrb	w0, [x0]
	ldrb	w1, [sp, 7]
	cmp	w1, w0
	beq	.L987
	mov	w0, 1
	b	.L988
.L987:
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L986:
	ldr	w1, [sp, 28]
	ldr	w0, [sp]
	cmp	w1, w0
	blt	.L989
	mov	w0, 0
.L988:
	add	sp, sp, 32
	ret
	.size	nftl_check_pattern, .-nftl_check_pattern
	.data
	.align	3
	.type	patterns, %object
	.size	patterns, 1
patterns:
	.byte	-91
	.section	.rodata
	.align	3
.LC227:
	.string	"[NI]torture b@%d.\n"
	.align	3
.LC228:
	.string	"[NE]%s write b@%d p@%d err\n"
	.align	3
.LC229:
	.string	"[NE]%s read b@%d p@%d err\n"
	.align	3
.LC230:
	.string	"[NE]%s b@%d p@%d patt %02x checking main data fail\n"
	.align	3
.LC231:
	.string	"[NE]%s b@%d p@%d patt %02x checking spare data fail\n"
	.align	3
.LC232:
	.string	"[NE][NE]erase_block block fail %d!\n"
	.text
	.align	2
	.global	torture_block_and_mark
	.type	torture_block_and_mark, %function
torture_block_and_mark:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	str	wzr, [x29, 92]
	str	wzr, [x29, 88]
	str	wzr, [x29, 80]
	str	wzr, [x29, 84]
	mov	w0, 3
	str	w0, [x29, 76]
	str	xzr, [x29, 56]
	str	wzr, [x29, 64]
	strh	wzr, [x29, 68]
	strb	wzr, [x29, 70]
	mov	w0, 1
	str	w0, [x29, 80]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC227
	add	x0, x0, :lo12:.LC227
	bl	nand_print
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 32
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L991
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC226
	add	x0, x0, :lo12:.LC226
	bl	nand_print
	b	.L992
.L991:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 16]
	strh	w1, [x0, 6]
	str	wzr, [x29, 88]
	b	.L993
.L1002:
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	adrp	x0, patterns
	add	x1, x0, :lo12:patterns
	ldrsw	x0, [x29, 88]
	ldrb	w0, [x1, x0]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	mov	x2, x0
	mov	x0, x3
	bl	memset
	adrp	x0, patterns
	add	x1, x0, :lo12:patterns
	ldrsw	x0, [x29, 88]
	ldrb	w0, [x1, x0]
	mov	w1, w0
	add	x0, x29, 56
	mov	x2, 15
	bl	memset
	str	wzr, [x29, 84]
	b	.L994
.L996:
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	w0, [x29, 84]
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 56
	add	x0, x29, 32
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L995
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, __func__.6377
	add	x1, x0, :lo12:__func__.6377
	adrp	x0, .LC228
	add	x0, x0, :lo12:.LC228
	ldr	w3, [x29, 84]
	bl	nand_print
	b	.L992
.L995:
	ldr	w0, [x29, 84]
	add	w0, w0, 1
	str	w0, [x29, 84]
.L994:
	ldr	w1, [x29, 84]
	ldr	w0, [x29, 76]
	cmp	w1, w0
	blt	.L996
	str	wzr, [x29, 84]
	b	.L997
.L1001:
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	mov	x2, x0
	mov	w1, 0
	mov	x0, x3
	bl	memset
	add	x0, x29, 56
	mov	x2, 15
	mov	w1, 0
	bl	memset
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	w0, [x29, 84]
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 56
	add	x0, x29, 32
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L998
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, __func__.6377
	add	x1, x0, :lo12:__func__.6377
	adrp	x0, .LC229
	add	x0, x0, :lo12:.LC229
	ldr	w3, [x29, 84]
	bl	nand_print
	b	.L992
.L998:
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	adrp	x0, patterns
	add	x1, x0, :lo12:patterns
	ldrsw	x0, [x29, 88]
	ldrb	w1, [x1, x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	mov	x0, x3
	bl	nftl_check_pattern
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L999
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, patterns
	add	x1, x0, :lo12:patterns
	ldrsw	x0, [x29, 88]
	ldrb	w0, [x1, x0]
	mov	w3, w0
	adrp	x0, __func__.6377
	add	x1, x0, :lo12:__func__.6377
	adrp	x0, .LC230
	add	x0, x0, :lo12:.LC230
	mov	w4, w3
	ldr	w3, [x29, 84]
	bl	nand_print
	b	.L992
.L999:
	adrp	x0, patterns
	add	x1, x0, :lo12:patterns
	ldrsw	x0, [x29, 88]
	ldrb	w1, [x1, x0]
	add	x0, x29, 56
	mov	w2, 15
	bl	nftl_check_pattern
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1000
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, patterns
	add	x1, x0, :lo12:patterns
	ldrsw	x0, [x29, 88]
	ldrb	w0, [x1, x0]
	mov	w3, w0
	adrp	x0, __func__.6377
	add	x1, x0, :lo12:__func__.6377
	adrp	x0, .LC231
	add	x0, x0, :lo12:.LC231
	mov	w4, w3
	ldr	w3, [x29, 84]
	bl	nand_print
	b	.L992
.L1000:
	ldr	w0, [x29, 84]
	add	w0, w0, 1
	str	w0, [x29, 84]
.L997:
	ldr	w1, [x29, 84]
	ldr	w0, [x29, 76]
	cmp	w1, w0
	blt	.L1001
	ldr	w0, [x29, 88]
	add	w0, w0, 1
	str	w0, [x29, 88]
.L993:
	ldr	w1, [x29, 88]
	ldr	w0, [x29, 80]
	cmp	w1, w0
	blt	.L1002
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 32
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1003
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC232
	add	x0, x0, :lo12:.LC232
	bl	nand_print
	b	.L992
.L1003:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 16]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 16]
	mov	w1, -1
	str	w1, [x0, 8]
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_free_list
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	mark_free_block
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w1, [x0, 20]
	ldr	x0, [x29, 16]
	strh	w1, [x0, 4]
	ldr	x0, [x29, 16]
	strh	wzr, [x0, 2]
	mov	w0, 0
	b	.L1005
.L992:
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 16]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 16]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 28]
	ldr	w0, [x29, 92]
.L1005:
	ldp	x29, x30, [sp], 96
	ret
	.size	torture_block_and_mark, .-torture_block_and_mark
	.align	2
	.global	mark_free_block
	.type	mark_free_block, %function
mark_free_block:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	add	x0, x29, 40
	mov	w2, 16
	mov	w1, -91
	bl	nand_memset
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 40
	add	x0, x29, 56
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 16]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 6]
	add	x0, x29, 40
	mov	w4, w1
	mov	w3, w2
	mov	w2, 1431655765
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_oob_special_page
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 512]
	add	x4, x29, 40
	add	x0, x29, 56
	mov	x5, x4
	mov	x4, x3
	mov	w3, w2
	mov	w2, 1
	bl	set_physic_op_par
	ldr	x0, [x29, 16]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 6]
	add	x0, x29, 40
	mov	w4, w1
	mov	w3, w2
	mov	w2, 1431655765
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_oob_special_page
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #2
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 40
	add	x0, x29, 56
	bl	set_physic_op_par
	ldr	x0, [x29, 16]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 6]
	add	x0, x29, 40
	mov	w4, w1
	mov	w3, w2
	mov	w2, 1431655765
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_oob_special_page
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 40
	add	x0, x29, 56
	bl	set_physic_op_par
	ldr	x0, [x29, 16]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x0, [x29, 16]
	ldrh	w1, [x0, 6]
	add	x0, x29, 40
	mov	w4, w1
	mov	w3, w2
	mov	w2, 1431655765
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_oob_special_page
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	mov	w0, 0
	ldp	x29, x30, [sp], 80
	ret
	.size	mark_free_block, .-mark_free_block
	.align	2
	.global	is_last_phy_block
	.type	is_last_phy_block, %function
is_last_phy_block:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [sp]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	sub	w0, w0, #1
	cmp	w1, w0
	bne	.L1009
	mov	w0, 1
	b	.L1010
.L1009:
	mov	w0, 0
.L1010:
	add	sp, sp, 16
	ret
	.size	is_last_phy_block, .-is_last_phy_block
	.align	2
	.global	is_phy_block_valid
	.type	is_phy_block_valid, %function
is_phy_block_valid:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	ldrh	w1, [x0]
	ldr	x0, [sp]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	cmp	w1, w0
	bcs	.L1012
	mov	w0, 1
	b	.L1013
.L1012:
	mov	w0, 0
.L1013:
	add	sp, sp, 16
	ret
	.size	is_phy_block_valid, .-is_phy_block_valid
	.align	2
	.global	get_phy_block_addr
	.type	get_phy_block_addr, %function
get_phy_block_addr:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	strh	w1, [sp, 6]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldrh	w0, [sp, 6]
	lsl	x0, x0, 6
	add	x0, x1, x0
	add	sp, sp, 16
	ret
	.size	get_phy_block_addr, .-get_phy_block_addr
	.section	.rodata
	.align	3
.LC233:
	.string	"[NE]get_used_page_num ecc error block:%d page:%d!\n"
	.align	3
.LC234:
	.string	"[NE]power off ecc error 4!!\n"
	.align	3
.LC235:
	.string	"[NE]sb@%d sp@%d get_used_page_num used spare data %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n"
	.text
	.align	2
	.global	get_used_page_num
	.type	get_used_page_num, %function
get_used_page_num:
	sub	sp, sp, #224
	stp	x29, x30, [sp, 96]
	add	x29, sp, 96
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	x3, [x29, 16]
	ldr	x0, [x29, 24]
	str	wzr, [x0]
	str	wzr, [x29, 120]
	str	wzr, [x29, 124]
	b	.L1017
.L1029:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	beq	.L1018
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 124]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1018
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w0, w0, 64
	mov	w2, w0
	ldr	w0, [x29, 124]
	mul	w0, w2, w0
	uxtw	x0, w0
	add	x0, x1, x0
	str	x0, [x29, 112]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	ldr	x1, [x29, 112]
	add	x0, x1, x0
	str	x0, [x29, 104]
	b	.L1019
.L1018:
	str	xzr, [x29, 112]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	cmp	w0, 8
	bne	.L1020
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 512]
	str	x0, [x29, 112]
.L1020:
	add	x0, x29, 80
	str	x0, [x29, 104]
.L1019:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 28]
	cmp	w0, 0
	beq	.L1021
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x0, x29, 56
	ldr	x5, [x29, 104]
	ldr	x4, [x29, 112]
	bl	set_physic_op_par
	b	.L1022
.L1021:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	add	x0, x29, 56
	ldr	x5, [x29, 104]
	mov	x4, 0
	mov	w3, 0
	bl	set_physic_op_par
.L1022:
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3472]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 100]
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L1023
	ldr	w0, [x29, 100]
	cmp	w0, 10
	beq	.L1023
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC233
	add	x0, x0, :lo12:.LC233
	ldr	w2, [x29, 124]
	bl	nand_print
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	add	w1, w0, 1
	ldr	x0, [x29, 24]
	str	w1, [x0]
.L1023:
	ldr	x0, [x29, 104]
	bl	is_nouse_page
	cmp	w0, 1
	beq	.L1040
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L1026
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L1027
.L1026:
	ldr	x0, [x29, 112]
	cmp	x0, 0
	beq	.L1027
	ldr	x2, [x29, 104]
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 40]
	bl	nand_power_off_ecc_error
	cmp	w0, 0
	beq	.L1028
	adrp	x0, .LC234
	add	x0, x0, :lo12:.LC234
	bl	nand_print
	b	.L1027
.L1028:
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 124]
	add	x0, x1, x0
	mov	w1, 1
	strb	w1, [x0]
	ldr	x0, [x29, 16]
	ldr	w1, [x0]
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	cmp	w1, w0
	bcs	.L1027
	ldr	w0, [x29, 124]
	add	w1, w0, 1
	ldr	x0, [x29, 16]
	str	w1, [x0]
.L1027:
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
.L1017:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 124]
	cmp	w1, w0
	bhi	.L1029
	b	.L1025
.L1040:
	nop
.L1025:
	ldr	w0, [x29, 124]
	str	w0, [x29, 96]
	b	.L1030
.L1038:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	beq	.L1031
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 124]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1031
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w0, w0, 64
	mov	w2, w0
	ldr	w0, [x29, 124]
	mul	w0, w2, w0
	uxtw	x0, w0
	add	x0, x1, x0
	str	x0, [x29, 112]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	ldr	x1, [x29, 112]
	add	x0, x1, x0
	str	x0, [x29, 104]
	b	.L1032
.L1031:
	str	xzr, [x29, 112]
	add	x0, x29, 80
	str	x0, [x29, 104]
.L1032:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 28]
	cmp	w0, 0
	beq	.L1033
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x0, x29, 56
	ldr	x5, [x29, 104]
	ldr	x4, [x29, 112]
	bl	set_physic_op_par
	b	.L1034
.L1033:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	add	x0, x29, 56
	ldr	x5, [x29, 104]
	mov	x4, 0
	mov	w3, 0
	bl	set_physic_op_par
.L1034:
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3472]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 100]
	ldr	x0, [x29, 104]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1035
	ldr	w0, [x29, 120]
	add	w0, w0, 1
	str	w0, [x29, 120]
	b	.L1036
.L1035:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w12, w0
	ldr	x0, [x29, 104]
	ldrb	w0, [x0]
	mov	w13, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w14, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w15, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w16, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 4
	ldrb	w0, [x0]
	mov	w17, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 5
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 6
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 7
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 8
	ldrb	w0, [x0]
	mov	w4, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 9
	ldrb	w0, [x0]
	mov	w5, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 10
	ldrb	w0, [x0]
	mov	w6, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 11
	ldrb	w0, [x0]
	mov	w7, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 12
	ldrb	w0, [x0]
	mov	w8, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 13
	ldrb	w0, [x0]
	mov	w9, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 14
	ldrb	w0, [x0]
	mov	w10, w0
	ldr	x0, [x29, 104]
	add	x0, x0, 15
	ldrb	w0, [x0]
	mov	w11, w0
	adrp	x0, .LC235
	add	x0, x0, :lo12:.LC235
	str	w11, [sp, 80]
	str	w10, [sp, 72]
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w17
	mov	w6, w16
	mov	w5, w15
	mov	w4, w14
	mov	w3, w13
	ldr	w2, [x29, 124]
	mov	w1, w12
	bl	nand_print
	ldr	w0, [x29, 100]
	cmp	w0, 0
	beq	.L1037
	ldr	w0, [x29, 100]
	cmp	w0, 10
	bne	.L1036
.L1037:
	ldr	x0, [x29, 112]
	cmp	x0, 0
	beq	.L1036
	ldr	x0, [x29, 16]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 124]
	add	x0, x1, x0
	mov	w1, 1
	strb	w1, [x0]
	ldr	x0, [x29, 16]
	ldr	w1, [x0]
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	cmp	w1, w0
	bcs	.L1036
	ldr	w0, [x29, 124]
	add	w1, w0, 1
	ldr	x0, [x29, 16]
	str	w1, [x0]
.L1036:
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
.L1030:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 124]
	cmp	w1, w0
	bhi	.L1038
	ldr	w0, [x29, 96]
	add	sp, x29, 0
	ldp	x29, x30, [sp], 128
	ret
	.size	get_used_page_num, .-get_used_page_num
	.section	.rodata
	.align	3
.LC236:
	.string	"[NE]gupnnc ecc err b:%d p:%d!\n"
	.align	3
.LC237:
	.string	"[NE]gupnnc power off ecc err!!\n"
	.align	3
.LC238:
	.string	"[NE]cross talk block:%d total_good_num %d good_page_num: %d!\n"
	.text
	.align	2
	.global	get_used_page_num_no_crosstalk
	.type	get_used_page_num_no_crosstalk, %function
get_used_page_num_no_crosstalk:
	stp	x29, x30, [sp, -160]!
	add	x29, sp, 0
	str	x19, [sp, 16]
	str	x0, [x29, 56]
	str	x1, [x29, 48]
	str	x2, [x29, 40]
	str	x3, [x29, 32]
	mov	w0, -1
	str	w0, [x29, 132]
	str	wzr, [x29, 128]
	str	wzr, [x29, 124]
	ldr	x0, [x29, 40]
	str	wzr, [x0]
	str	wzr, [x29, 156]
	b	.L1042
.L1053:
	ldr	x0, [x29, 32]
	cmp	x0, 0
	beq	.L1043
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 156]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1043
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w0, w0, 64
	mov	w2, w0
	ldr	w0, [x29, 156]
	mul	w0, w2, w0
	uxtw	x0, w0
	add	x0, x1, x0
	str	x0, [x29, 144]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	ldr	x1, [x29, 144]
	add	x0, x1, x0
	str	x0, [x29, 136]
	b	.L1044
.L1043:
	str	xzr, [x29, 144]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	cmp	w0, 4
	bne	.L1045
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 512]
	str	x0, [x29, 144]
.L1045:
	add	x0, x29, 96
	str	x0, [x29, 136]
.L1044:
	ldr	x0, [x29, 48]
	ldrh	w1, [x0]
	ldr	w0, [x29, 156]
	uxth	w2, w0
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x0, x29, 72
	ldr	x5, [x29, 136]
	ldr	x4, [x29, 144]
	bl	set_physic_op_par
	ldr	x0, [x29, 56]
	ldr	x2, [x0, 3472]
	add	x0, x29, 72
	mov	x1, x0
	ldr	x0, [x29, 56]
	blr	x2
	str	w0, [x29, 120]
	ldr	w0, [x29, 120]
	cmp	w0, 0
	beq	.L1046
	ldr	w0, [x29, 120]
	cmp	w0, 10
	beq	.L1046
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC236
	add	x0, x0, :lo12:.LC236
	ldr	w2, [x29, 156]
	bl	nand_print
	ldr	w0, [x29, 132]
	cmn	w0, #1
	bne	.L1048
	ldr	w0, [x29, 156]
	str	w0, [x29, 132]
	b	.L1048
.L1046:
	ldr	x0, [x29, 136]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1049
	ldr	w0, [x29, 132]
	cmn	w0, #1
	bne	.L1050
	ldr	w0, [x29, 156]
	str	w0, [x29, 132]
.L1050:
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
	ldr	w0, [x29, 124]
	cmp	w0, 3
	bne	.L1048
	b	.L1051
.L1049:
	ldr	x2, [x29, 136]
	ldr	x1, [x29, 144]
	ldr	x0, [x29, 56]
	bl	nand_power_off_ecc_error
	cmp	w0, 0
	beq	.L1052
	adrp	x0, .LC237
	add	x0, x0, :lo12:.LC237
	bl	nand_print
	b	.L1048
.L1052:
	ldr	w0, [x29, 128]
	add	w0, w0, 1
	str	w0, [x29, 128]
	ldr	w0, [x29, 132]
	cmn	w0, #1
	bne	.L1048
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 156]
	add	x0, x1, x0
	mov	w1, 1
	strb	w1, [x0]
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 24]
	ldr	w0, [x29, 156]
	lsl	x0, x0, 2
	add	x19, x1, x0
	ldr	x0, [x29, 136]
	bl	get_logic_page_from_oob
	str	w0, [x19]
	ldr	x0, [x29, 32]
	ldr	w1, [x0]
	ldr	w0, [x29, 156]
	add	w0, w0, 1
	cmp	w1, w0
	bcs	.L1048
	ldr	w0, [x29, 156]
	add	w1, w0, 1
	ldr	x0, [x29, 32]
	str	w1, [x0]
.L1048:
	ldr	w0, [x29, 156]
	add	w0, w0, 1
	str	w0, [x29, 156]
.L1042:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 156]
	cmp	w1, w0
	bhi	.L1053
.L1051:
	ldr	w0, [x29, 132]
	cmn	w0, #1
	bne	.L1054
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	str	w0, [x29, 132]
	b	.L1055
.L1054:
	ldr	w1, [x29, 128]
	ldr	w0, [x29, 132]
	cmp	w1, w0
	beq	.L1055
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC238
	add	x0, x0, :lo12:.LC238
	ldr	w3, [x29, 132]
	ldr	w2, [x29, 128]
	bl	nand_print
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 168]
	add	w1, w1, 1
	str	w1, [x0, 168]
	str	wzr, [x29, 156]
	b	.L1056
.L1058:
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 8]
	ldr	w0, [x29, 156]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	beq	.L1057
	ldr	x0, [x29, 32]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	add	w0, w0, 64
	mov	w2, w0
	ldr	w0, [x29, 156]
	mul	w0, w2, w0
	uxtw	x2, w0
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	add	x0, x2, x0
	add	x0, x1, x0
	str	x0, [x29, 136]
	ldr	x0, [x29, 136]
	add	x0, x0, 14
	ldr	x1, [x29, 56]
	ldr	x1, [x1, 3440]
	ldr	w1, [x1, 168]
	uxtb	w1, w1
	strb	w1, [x0]
.L1057:
	ldr	w0, [x29, 156]
	add	w0, w0, 1
	str	w0, [x29, 156]
.L1056:
	ldr	x0, [x29, 32]
	ldr	w1, [x0]
	ldr	w0, [x29, 156]
	cmp	w1, w0
	bhi	.L1058
.L1055:
	ldr	w0, [x29, 132]
	str	w0, [x29, 116]
	ldr	w0, [x29, 116]
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 160
	ret
	.size	get_used_page_num_no_crosstalk, .-get_used_page_num_no_crosstalk
	.align	2
	.global	check_cross_talk
	.type	check_cross_talk, %function
check_cross_talk:
	stp	x29, x30, [sp, -112]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 512]
	str	x0, [x29, 96]
	str	wzr, [x29, 108]
	b	.L1061
.L1064:
	str	xzr, [x29, 96]
	add	x0, x29, 64
	str	x0, [x29, 88]
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	w0, [x29, 108]
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	add	x0, x29, 40
	ldr	x5, [x29, 88]
	ldr	x4, [x29, 96]
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 84]
	ldr	w0, [x29, 84]
	cmp	w0, 0
	bne	.L1062
	ldr	x0, [x29, 88]
	add	x0, x0, 14
	ldrb	w0, [x0]
	cmp	w0, 99
	bhi	.L1062
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 168]
	ldr	x1, [x29, 88]
	add	x1, x1, 14
	ldrb	w1, [x1]
	cmp	w0, w1
	bhi	.L1062
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 88]
	add	x1, x1, 14
	ldrb	w1, [x1]
	str	w1, [x0, 168]
	b	.L1063
.L1062:
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
.L1061:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 108]
	cmp	w1, w0
	bhi	.L1064
.L1063:
	mov	w0, 0
	ldp	x29, x30, [sp], 112
	ret
	.size	check_cross_talk, .-check_cross_talk
	.align	2
	.global	init_smart_info
	.type	init_smart_info, %function
init_smart_info:
	stp	x29, x30, [sp, -128]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	mov	w2, 320
	mov	w1, 0
	bl	nand_memset
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	mov	w1, 43521
	movk	w1, 0xaaaa, lsl 16
	str	w1, [x0, 112]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 140]
	add	w1, w1, 1
	str	w1, [x0, 140]
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1067
	mov	w0, 0
	b	.L1073
.L1067:
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	sub	w0, w0, #1
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 104
	add	x0, x29, 80
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 80
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 124]
	add	x0, x29, 104
	bl	is_phy_mapping_page
	cmp	w0, 1
	bne	.L1069
	ldr	w0, [x29, 124]
	cmp	w0, 0
	beq	.L1070
	ldr	w0, [x29, 124]
	cmp	w0, 10
	bne	.L1069
.L1070:
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 512]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 560]
	uxtw	x0, w0
	add	x0, x1, x0
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	ldr	w1, [x0, 112]
	mov	w0, 43521
	movk	w0, 0xaaaa, lsl 16
	cmp	w1, w0
	bne	.L1069
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 64]
	mov	w2, 320
	bl	nand_memcpy
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 140]
	add	w1, w1, 1
	str	w1, [x0, 140]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 164]
	cmn	w0, #1
	bne	.L1071
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	str	wzr, [x0, 164]
.L1071:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 172]
	cmn	w0, #1
	bne	.L1072
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	str	wzr, [x0, 172]
.L1072:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 176]
	cmn	w0, #1
	bne	.L1069
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	str	wzr, [x0, 176]
.L1069:
	ldr	x0, [x29, 24]
	bl	print_smart
	mov	w0, 0
.L1073:
	ldp	x29, x30, [sp], 128
	ret
	.size	init_smart_info, .-init_smart_info
	.align	2
	.global	get_used_block_count
	.type	get_used_block_count, %function
get_used_block_count:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	add	x2, x29, 32
	add	x0, x29, 48
	mov	x5, x2
	mov	x4, 0
	mov	w3, 0
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	add	x0, x29, 32
	bl	get_block_used_count_from_oob
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	ldp	x29, x30, [sp], 80
	ret
	.size	get_used_block_count, .-get_used_block_count
	.section	.rodata
	.align	3
.LC239:
	.string	"[NE]current_block_ecc_error no free block!!!\n"
	.align	3
.LC240:
	.string	"[NE]do current_block_ecc_error %d!!\n"
	.align	3
.LC241:
	.string	"[NE]power off ecc error 2.\n"
	.align	3
.LC242:
	.string	"[NE]current_block_ecc ok page %d %d!!\n"
	.align	3
.LC243:
	.string	"[NE]current_block_ecc_error page %d!!\n"
	.align	3
.LC244:
	.string	"[NE]current_block_ecc_error invalid page data %d!!\n"
	.align	3
.LC245:
	.string	"[NE]current_block_ecc_error unkown page data %d!!\n"
	.align	3
.LC246:
	.string	"[ND]do current_block_ecc_error end %d %d!!\n"
	.text
	.align	2
	.global	current_block_ecc_error
	.type	current_block_ecc_error, %function
current_block_ecc_error:
	stp	x29, x30, [sp, -128]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
.L1077:
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 112]
	ldr	x0, [x29, 112]
	cmp	x0, 0
	bne	.L1078
	adrp	x0, .LC239
	add	x0, x0, :lo12:.LC239
	bl	nand_print
	mov	x0, 0
	b	.L1093
.L1078:
	ldr	x0, [x29, 16]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 112]
	str	w1, [x0, 8]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 8]
	mov	w2, w0
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	new_block_init_for_write
	cmp	w0, 0
	beq	.L1080
	adrp	x0, .LC185
	add	x0, x0, :lo12:.LC185
	bl	nand_print
	b	.L1077
.L1080:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC240
	add	x0, x0, :lo12:.LC240
	bl	nand_print
	str	wzr, [x29, 124]
	str	wzr, [x29, 120]
	b	.L1081
.L1090:
	ldr	x0, [x29, 16]
	ldrh	w1, [x0]
	ldr	w0, [x29, 124]
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 88
	add	x0, x29, 64
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 64
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 108]
	add	x0, x29, 88
	bl	is_ftl_logic_page_data
	cmp	w0, 1
	bne	.L1082
	ldr	w0, [x29, 108]
	cmp	w0, 0
	beq	.L1083
	ldr	w0, [x29, 108]
	cmp	w0, 10
	bne	.L1084
.L1083:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 512]
	add	x1, x29, 88
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_power_off_ecc_error
	cmp	w0, 0
	beq	.L1085
	adrp	x0, .LC241
	add	x0, x0, :lo12:.LC241
	bl	nand_print
	b	.L1088
.L1085:
	adrp	x0, .LC242
	add	x0, x0, :lo12:.LC242
	ldr	w2, [x29, 108]
	ldr	w1, [x29, 124]
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w1, [x0]
	ldr	w0, [x29, 120]
	uxth	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	x4, [x0, 512]
	add	x5, x29, 88
	add	x0, x29, 40
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	w0, [x29, 120]
	add	w0, w0, 1
	str	w0, [x29, 120]
	b	.L1088
.L1084:
	adrp	x0, .LC243
	add	x0, x0, :lo12:.LC243
	ldr	w1, [x29, 124]
	bl	nand_print
	b	.L1088
.L1082:
	add	x0, x29, 88
	bl	is_ftl_special_data
	cmp	w0, 1
	bne	.L1089
	adrp	x0, .LC244
	add	x0, x0, :lo12:.LC244
	ldr	w1, [x29, 124]
	bl	nand_print
	b	.L1088
.L1089:
	add	x0, x29, 88
	bl	is_nouse_page
	cmp	w0, 1
	beq	.L1088
	adrp	x0, .LC245
	add	x0, x0, :lo12:.LC245
	ldr	w1, [x29, 124]
	bl	nand_print
.L1088:
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
.L1081:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 124]
	cmp	w1, w0
	bgt	.L1090
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L1091
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	erase_block
	b	.L1092
.L1091:
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	erase_block
.L1092:
	adrp	x0, .LC246
	add	x0, x0, :lo12:.LC246
	ldr	w2, [x29, 120]
	ldr	w1, [x29, 124]
	bl	nand_print_dbg
	ldr	x0, [x29, 112]
.L1093:
	ldp	x29, x30, [sp], 128
	ret
	.size	current_block_ecc_error, .-current_block_ecc_error
	.section	.rodata
	.align	3
.LC247:
	.string	"[NE]do block_last_page_ecc_error!!\n"
	.align	3
.LC248:
	.string	"[NE]%s:malloc fail 1!\n"
	.align	3
.LC249:
	.string	"[NE]%s:malloc fail 2!\n"
	.align	3
.LC250:
	.string	"[NE]current_block_ecc_error invalid page data!!\n"
	.align	3
.LC251:
	.string	"[NE]current_block_ecc_error unkown page data!!\n"
	.align	3
.LC252:
	.string	"[NE]do block_last_page_ecc_error end!!\n"
	.text
	.align	2
	.global	block_last_page_ecc_error
	.type	block_last_page_ecc_error, %function
block_last_page_ecc_error:
	stp	x29, x30, [sp, -112]!
	add	x29, sp, 0
	str	x19, [sp, 16]
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	adrp	x0, .LC247
	add	x0, x0, :lo12:.LC247
	bl	nand_print
	str	wzr, [x29, 104]
	mov	w0, 8192
	bl	nftl_malloc
	str	x0, [x29, 96]
	ldr	x0, [x29, 96]
	cmp	x0, 0
	bne	.L1095
	adrp	x0, __func__.6501
	add	x1, x0, :lo12:__func__.6501
	adrp	x0, .LC248
	add	x0, x0, :lo12:.LC248
	bl	nand_print
.L1095:
	str	wzr, [x29, 108]
	b	.L1096
.L1098:
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 3
	ldr	x1, [x29, 96]
	add	x19, x1, x0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	bl	nftl_malloc
	str	x0, [x19]
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 3
	ldr	x1, [x29, 96]
	add	x0, x1, x0
	ldr	x0, [x0]
	cmp	x0, 0
	bne	.L1097
	adrp	x0, __func__.6501
	add	x1, x0, :lo12:__func__.6501
	adrp	x0, .LC249
	add	x0, x0, :lo12:.LC249
	bl	nand_print
.L1097:
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
.L1096:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 108]
	cmp	w1, w0
	bgt	.L1098
	str	wzr, [x29, 108]
	b	.L1099
.L1109:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 108]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 512]
	add	x5, x29, 72
	add	x0, x29, 48
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3472]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 92]
	add	x0, x29, 72
	bl	is_nouse_page
	cmp	w0, 1
	beq	.L1118
	add	x0, x29, 72
	bl	is_ftl_logic_page_data
	cmp	w0, 1
	bne	.L1102
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1103
	ldr	w0, [x29, 92]
	cmp	w0, 10
	bne	.L1105
.L1103:
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 3
	ldr	x1, [x29, 96]
	add	x0, x1, x0
	ldr	x3, [x0]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 512]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
	ldr	w0, [x29, 104]
	add	w0, w0, 1
	str	w0, [x29, 104]
	b	.L1105
.L1102:
	add	x0, x29, 72
	bl	is_ftl_special_data
	cmp	w0, 1
	bne	.L1106
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1107
	ldr	w0, [x29, 92]
	cmp	w0, 10
	bne	.L1108
.L1107:
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 3
	ldr	x1, [x29, 96]
	add	x0, x1, x0
	ldr	x3, [x0]
	ldr	x0, [x29, 40]
	ldr	x1, [x0, 512]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
	ldr	w0, [x29, 104]
	add	w0, w0, 1
	str	w0, [x29, 104]
.L1108:
	adrp	x0, .LC250
	add	x0, x0, :lo12:.LC250
	bl	nand_print
	b	.L1105
.L1106:
	adrp	x0, .LC251
	add	x0, x0, :lo12:.LC251
	bl	nand_print
.L1105:
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
.L1099:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 108]
	cmp	w1, w0
	bgt	.L1109
	b	.L1101
.L1118:
	nop
.L1101:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 108]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 512]
	add	x5, x29, 72
	add	x0, x29, 48
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3464]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1110
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3504]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldr	x0, [x29, 40]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L1111
	adrp	x0, .LC239
	add	x0, x0, :lo12:.LC239
	bl	nand_print
	str	xzr, [x29, 32]
	b	.L1112
.L1111:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0]
	ldr	w0, [x29, 108]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 512]
	add	x5, x29, 72
	add	x0, x29, 48
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3464]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
.L1110:
	str	wzr, [x29, 108]
	b	.L1113
.L1114:
	ldr	x0, [x29, 32]
	ldrh	w6, [x0]
	ldr	w0, [x29, 108]
	uxth	w2, w0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w3, [x0, 28]
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 3
	ldr	x1, [x29, 96]
	add	x0, x1, x0
	ldr	x1, [x0]
	add	x4, x29, 72
	add	x0, x29, 48
	mov	x5, x4
	mov	x4, x1
	mov	w1, w6
	bl	set_physic_op_par
	ldr	x0, [x29, 40]
	ldr	x2, [x0, 3480]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 40]
	blr	x2
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
.L1113:
	ldr	w1, [x29, 108]
	ldr	w0, [x29, 104]
	cmp	w1, w0
	blt	.L1114
.L1112:
	str	wzr, [x29, 108]
	b	.L1115
.L1116:
	ldrsw	x0, [x29, 108]
	lsl	x0, x0, 3
	ldr	x1, [x29, 96]
	add	x0, x1, x0
	ldr	x0, [x0]
	bl	nftl_free
	ldr	w0, [x29, 108]
	add	w0, w0, 1
	str	w0, [x29, 108]
.L1115:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 108]
	cmp	w1, w0
	bgt	.L1116
	ldr	x0, [x29, 96]
	bl	nftl_free
	adrp	x0, .LC252
	add	x0, x0, :lo12:.LC252
	bl	nand_print
	ldr	x0, [x29, 32]
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 112
	ret
	.size	block_last_page_ecc_error, .-block_last_page_ecc_error
	.section	.rodata
	.align	3
.LC253:
	.string	"[NE]adjust free list fail!!\n"
	.align	3
.LC254:
	.string	"[NI]adjust free list : %d %d!!\n"
	.text
	.align	2
	.global	adjust_free_list
	.type	adjust_free_list, %function
adjust_free_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L1120
	adrp	x0, .LC253
	add	x0, x0, :lo12:.LC253
	bl	nand_print
.L1120:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	erase_block
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	x1, [x29, 40]
	str	x1, [x0, 64]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC254
	add	x0, x0, :lo12:.LC254
	bl	nand_print
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	adjust_free_list, .-adjust_free_list
	.align	2
	.global	nftl_get_bad_block_cnt
	.type	nftl_get_bad_block_cnt, %function
nftl_get_bad_block_cnt:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 24]
	ldrh	w0, [x0, 28]
	add	sp, sp, 16
	ret
	.size	nftl_get_bad_block_cnt, .-nftl_get_bad_block_cnt
	.section	.rodata
	.align	3
.LC255:
	.string	"[ND]nand3.x \n"
	.align	3
.LC256:
	.string	"[ND]zone->nand_chip->blk_per_chip: %d \n"
	.align	3
.LC257:
	.string	"[ND]zone->nand_chip->bytes_per_page: %d \n"
	.align	3
.LC258:
	.string	"[ND]zone->nand_chip->pages_per_blk: %d \n"
	.align	3
.LC259:
	.string	"[ND]zone->nand_chip->max_erase_times: %d \n"
	.align	3
.LC260:
	.string	"[ND]zone->nand_chip->support_read_reclaim: %d \n"
	.align	3
.LC261:
	.string	"[ND]zone->test: %d \n"
	.align	3
.LC262:
	.string	"[ND]zone->zone_no: %d \n"
	.align	3
.LC263:
	.string	"[ND]zone->zone_attr: %d \n"
	.align	3
.LC264:
	.string	"[ND]zone->blocks: %d \n"
	.align	3
.LC265:
	.string	"[ND]zone->bad_block: %d \n"
	.align	3
.LC266:
	.string	"[ND]zone->logic_cap_in_sects: %d \n"
	.align	3
.LC267:
	.string	"[ND]zone->backup_cap_in_sects: %d \n"
	.align	3
.LC268:
	.string	"[ND]zone->free_block_num: %d \n"
	.align	3
.LC269:
	.string	"[ND]zone->gc_strategy.start_gc_free_blocks: %d \n"
	.align	3
.LC270:
	.string	"[ND]zone->gc_strategy.stop_gc_free_blocks: %d \n"
	.align	3
.LC271:
	.string	"[ND]zone->gc_strategy.gc_page: %d \n"
	.align	3
.LC272:
	.string	"[ND]zone->gc_strategy.process: %d \n"
	.align	3
.LC273:
	.string	"[ND]zone->prio_gc.prio_type : %d \n"
	.align	3
.LC274:
	.string	"[ND]zone->zone_start_phy_block->phy_block.Block_NO: %d \n"
	.align	3
.LC275:
	.string	"[ND]zone->zone_end_phy_block->phy_block.Block_NO: %d \n"
	.align	3
.LC276:
	.string	"[ND]zone->zone_phy_page_map_for_gc: %x \n"
	.align	3
.LC277:
	.string	"[ND]zone->current_block.user_info: %x \n"
	.align	3
.LC278:
	.string	"[ND]zone->current_block.block_info: %x \n"
	.align	3
.LC279:
	.string	"[ND]zone->current_block.block_info->phy_block.Block_NO: %d \n"
	.align	3
.LC280:
	.string	"[ND]zone->current_block.page_used: %d \n"
	.align	3
.LC281:
	.string	"[ND]zone->current_block.block_info->block_used_count: %d \n"
	.align	3
.LC282:
	.string	"[ND]zone->recv_free_block: %d %d!!\n"
	.align	3
.LC283:
	.string	"[ND]zone->assist_block.user_info: %x \n"
	.align	3
.LC284:
	.string	"[ND]zone->assist_block.block_info: %x \n"
	.align	3
.LC285:
	.string	"[ND]zone->assist_block.block_info->phy_block.Block_NO: %d \n"
	.align	3
.LC286:
	.string	"[ND]zone->assist_block.page_used: %d \n"
	.align	3
.LC287:
	.string	"[ND]zone->assist_block.block_info->block_used_count: %d \n"
	.align	3
.LC288:
	.string	"[ND]zone->read_reclaim_complete: %x \n"
	.align	3
.LC289:
	.string	"[ND]zone->temp_page_buf: %x \n"
	.align	3
.LC290:
	.string	"[ND]zone->max_erase_num: %d \n"
	.align	3
.LC291:
	.string	"[ND]zone->cache.cache_totals: %x \n"
	.align	3
.LC292:
	.string	"[ND]zone->wl_gc_count: %d \n"
	.align	3
.LC293:
	.string	"[ND]zone->wl_gc_page_num: %d \n"
	.align	3
.LC294:
	.string	"[ND]zone->cfg->nftl_dont_use_cache: %x \n"
	.align	3
.LC295:
	.string	"[ND]zone->cfg->nftl_use_cache_sort: %x \n"
	.align	3
.LC296:
	.string	"[ND]zone->cfg->nftl_support_gc_read_reclaim: %x \n"
	.align	3
.LC297:
	.string	"[ND]zone->cfg->nftl_support_wear_leveling: %x \n"
	.align	3
.LC298:
	.string	"[ND]zone->cfg->nftl_need_erase: %x \n"
	.align	3
.LC299:
	.string	"[ND]zone->cfg->nftl_min_free_block_num: %x \n"
	.align	3
.LC300:
	.string	"[ND]zone->cfg->nftl_gc_threshold_free_block_num: %x \n"
	.align	3
.LC301:
	.string	"[ND]zone->cfg->nftl_min_free_block: %x \n"
	.align	3
.LC302:
	.string	"[ND]zone->cfg->nftl_gc_threshold_ratio_numerator: %x \n"
	.align	3
.LC303:
	.string	"[ND]zone->cfg->nftl_gc_threshold_ratio_denominator: %x \n"
	.align	3
.LC304:
	.string	"[ND]zone->cfg->nftl_max_cache_num: %d \n"
	.align	3
.LC305:
	.string	"[ND]zone->cfg->nftl_max_cache_write_num: %d \n"
	.align	3
.LC306:
	.string	"[ND]zone->cfg->nftl_cross_talk: %x \n"
	.align	3
.LC307:
	.string	"[ND]zone->cfg->nftl_read_claim_interval: %d \n"
	.text
	.align	2
	.global	print_nftl_zone
	.type	print_nftl_zone, %function
print_nftl_zone:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	w0, [x0, 656]
	cmp	w0, 0
	beq	.L1128
	adrp	x0, .LC255
	add	x0, x0, :lo12:.LC255
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	mov	w1, w0
	adrp	x0, .LC256
	add	x0, x0, :lo12:.LC256
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	adrp	x0, .LC257
	add	x0, x0, :lo12:.LC257
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	adrp	x0, .LC258
	add	x0, x0, :lo12:.LC258
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	mov	w1, w0
	adrp	x0, .LC259
	add	x0, x0, :lo12:.LC259
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	adrp	x0, .LC260
	add	x0, x0, :lo12:.LC260
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 656]
	adrp	x0, .LC261
	add	x0, x0, :lo12:.LC261
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	adrp	x0, .LC262
	add	x0, x0, :lo12:.LC262
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrb	w0, [x0, 25]
	mov	w1, w0
	adrp	x0, .LC263
	add	x0, x0, :lo12:.LC263
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	adrp	x0, .LC264
	add	x0, x0, :lo12:.LC264
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 28]
	mov	w1, w0
	adrp	x0, .LC265
	add	x0, x0, :lo12:.LC265
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 32]
	adrp	x0, .LC266
	add	x0, x0, :lo12:.LC266
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 40]
	adrp	x0, .LC267
	add	x0, x0, :lo12:.LC267
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 30]
	mov	w1, w0
	adrp	x0, .LC268
	add	x0, x0, :lo12:.LC268
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 680]
	mov	w1, w0
	adrp	x0, .LC269
	add	x0, x0, :lo12:.LC269
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 682]
	mov	w1, w0
	adrp	x0, .LC270
	add	x0, x0, :lo12:.LC270
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 688]
	mov	w1, w0
	adrp	x0, .LC271
	add	x0, x0, :lo12:.LC271
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 684]
	mov	w1, w0
	adrp	x0, .LC272
	add	x0, x0, :lo12:.LC272
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 698]
	mov	w1, w0
	adrp	x0, .LC273
	add	x0, x0, :lo12:.LC273
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC274
	add	x0, x0, :lo12:.LC274
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 56]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC275
	add	x0, x0, :lo12:.LC275
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	add	x0, x0, 512
	sub	x1, x0, #48
	add	x0, x29, 16
	ldp	x2, x3, [x1]
	stp	x2, x3, [x0]
	ldp	x2, x3, [x1, 16]
	stp	x2, x3, [x0, 16]
	ldp	x1, x2, [x1, 32]
	stp	x1, x2, [x0, 32]
	add	x1, x29, 16
	adrp	x0, .LC276
	add	x0, x0, :lo12:.LC276
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	add	x0, x0, 512
	add	x1, x0, 24
	add	x0, x29, 16
	ldp	x2, x3, [x1]
	stp	x2, x3, [x0]
	ldp	x2, x3, [x1, 16]
	stp	x2, x3, [x0, 16]
	ldp	x1, x2, [x1, 32]
	stp	x1, x2, [x0, 32]
	add	x1, x29, 16
	adrp	x0, .LC277
	add	x0, x0, :lo12:.LC277
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x1, [x0, 528]
	adrp	x0, .LC278
	add	x0, x0, :lo12:.LC278
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 528]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC279
	add	x0, x0, :lo12:.LC279
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 584]
	mov	w1, w0
	adrp	x0, .LC280
	add	x0, x0, :lo12:.LC280
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 528]
	ldr	w1, [x0, 8]
	adrp	x0, .LC281
	add	x0, x0, :lo12:.LC281
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 64]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 64]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC282
	add	x0, x0, :lo12:.LC282
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrb	w0, [x0, 25]
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L1127
	ldr	x0, [x29, 72]
	add	x0, x0, 512
	add	x1, x0, 88
	add	x0, x29, 16
	ldp	x2, x3, [x1]
	stp	x2, x3, [x0]
	ldp	x2, x3, [x1, 16]
	stp	x2, x3, [x0, 16]
	ldp	x1, x2, [x1, 32]
	stp	x1, x2, [x0, 32]
	add	x1, x29, 16
	adrp	x0, .LC283
	add	x0, x0, :lo12:.LC283
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x1, [x0, 592]
	adrp	x0, .LC284
	add	x0, x0, :lo12:.LC284
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 592]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC285
	add	x0, x0, :lo12:.LC285
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 648]
	mov	w1, w0
	adrp	x0, .LC286
	add	x0, x0, :lo12:.LC286
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 592]
	ldr	w1, [x0, 8]
	adrp	x0, .LC287
	add	x0, x0, :lo12:.LC287
	bl	nand_print_dbg
.L1127:
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 660]
	adrp	x0, .LC288
	add	x0, x0, :lo12:.LC288
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x1, [x0, 512]
	adrp	x0, .LC289
	add	x0, x0, :lo12:.LC289
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 676]
	adrp	x0, .LC290
	add	x0, x0, :lo12:.LC290
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 1072]
	mov	w1, w0
	adrp	x0, .LC291
	add	x0, x0, :lo12:.LC291
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 668]
	adrp	x0, .LC292
	add	x0, x0, :lo12:.LC292
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 672]
	adrp	x0, .LC293
	add	x0, x0, :lo12:.LC293
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC294
	add	x0, x0, :lo12:.LC294
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	adrp	x0, .LC295
	add	x0, x0, :lo12:.LC295
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	adrp	x0, .LC296
	add	x0, x0, :lo12:.LC296
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	adrp	x0, .LC297
	add	x0, x0, :lo12:.LC297
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 10]
	mov	w1, w0
	adrp	x0, .LC298
	add	x0, x0, :lo12:.LC298
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 12]
	mov	w1, w0
	adrp	x0, .LC299
	add	x0, x0, :lo12:.LC299
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 14]
	mov	w1, w0
	adrp	x0, .LC300
	add	x0, x0, :lo12:.LC300
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 16]
	mov	w1, w0
	adrp	x0, .LC301
	add	x0, x0, :lo12:.LC301
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 18]
	mov	w1, w0
	adrp	x0, .LC302
	add	x0, x0, :lo12:.LC302
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	adrp	x0, .LC303
	add	x0, x0, :lo12:.LC303
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 22]
	mov	w1, w0
	adrp	x0, .LC304
	add	x0, x0, :lo12:.LC304
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 24]
	mov	w1, w0
	adrp	x0, .LC305
	add	x0, x0, :lo12:.LC305
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 26]
	mov	w1, w0
	adrp	x0, .LC306
	add	x0, x0, :lo12:.LC306
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 3448]
	ldr	w1, [x0, 32]
	adrp	x0, .LC307
	add	x0, x0, :lo12:.LC307
	bl	nand_print_dbg
	ldr	x0, [x29, 72]
	bl	print_smart
	b	.L1124
.L1128:
	nop
.L1124:
	ldp	x29, x30, [sp], 80
	ret
	.size	print_nftl_zone, .-print_nftl_zone
	.section	.rodata
	.align	3
.LC308:
	.string	"[NE]%s:malloc 4096 fail!\n"
	.align	3
.LC309:
	.string	"[ND]spare: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x"
	.align	3
.LC310:
	.string	"[ND]buf:%x %x %x %x %x %x %x %x %x %x %x.\n"
	.text
	.align	2
	.global	nand_dbg_phy_read
	.type	nand_dbg_phy_read, %function
nand_dbg_phy_read:
	sub	sp, sp, #144
	stp	x29, x30, [sp, 80]
	add	x29, sp, 80
	strh	w0, [x29, 30]
	strh	w1, [x29, 28]
	strh	w2, [x29, 26]
	mov	w0, 4096
	bl	nftl_malloc
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L1130
	adrp	x0, __func__.6533
	add	x1, x0, :lo12:__func__.6533
	adrp	x0, .LC308
	add	x0, x0, :lo12:.LC308
	bl	nand_print
.L1130:
	add	x0, x29, 32
	mov	x5, x0
	ldr	x4, [x29, 56]
	mov	w3, 8
	ldrh	w2, [x29, 26]
	ldrh	w1, [x29, 28]
	ldrh	w0, [x29, 30]
	bl	page_read
	str	w0, [x29, 52]
	ldrb	w0, [x29, 32]
	mov	w10, w0
	ldrb	w0, [x29, 33]
	mov	w11, w0
	ldrb	w0, [x29, 34]
	mov	w12, w0
	ldrb	w0, [x29, 35]
	mov	w13, w0
	ldrb	w0, [x29, 36]
	mov	w14, w0
	ldrb	w0, [x29, 37]
	mov	w15, w0
	ldrb	w0, [x29, 38]
	mov	w16, w0
	ldrb	w0, [x29, 39]
	mov	w1, w0
	ldrb	w0, [x29, 40]
	mov	w2, w0
	ldrb	w0, [x29, 41]
	mov	w3, w0
	ldrb	w0, [x29, 42]
	mov	w4, w0
	ldrb	w0, [x29, 43]
	mov	w5, w0
	ldrb	w0, [x29, 44]
	mov	w6, w0
	ldrb	w0, [x29, 45]
	mov	w7, w0
	ldrb	w0, [x29, 46]
	mov	w8, w0
	ldrb	w0, [x29, 47]
	mov	w9, w0
	adrp	x0, .LC309
	add	x0, x0, :lo12:.LC309
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w16
	mov	w6, w15
	mov	w5, w14
	mov	w4, w13
	mov	w3, w12
	mov	w2, w11
	mov	w1, w10
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	ldrb	w0, [x0]
	mov	w8, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w9, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w10, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w11, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 4
	ldrb	w0, [x0]
	mov	w5, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 5
	ldrb	w0, [x0]
	mov	w6, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 6
	ldrb	w0, [x0]
	mov	w7, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 7
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 8
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 9
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 56]
	add	x0, x0, 10
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC310
	add	x0, x0, :lo12:.LC310
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w4, w11
	mov	w3, w10
	mov	w2, w9
	mov	w1, w8
	bl	nand_print_dbg
	ldr	x0, [x29, 56]
	bl	nftl_free
	ldr	w0, [x29, 52]
	add	sp, x29, 0
	ldp	x29, x30, [sp], 64
	ret
	.size	nand_dbg_phy_read, .-nand_dbg_phy_read
	.align	2
	.global	nand_dbg_zone_phy_read
	.type	nand_dbg_zone_phy_read, %function
nand_dbg_zone_phy_read:
	sub	sp, sp, #176
	stp	x29, x30, [sp, 80]
	add	x29, sp, 80
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	strh	w2, [x29, 20]
	mov	w0, 4096
	bl	nftl_malloc
	str	x0, [x29, 88]
	ldr	x0, [x29, 88]
	cmp	x0, 0
	bne	.L1133
	adrp	x0, __func__.6544
	add	x1, x0, :lo12:__func__.6544
	adrp	x0, .LC308
	add	x0, x0, :lo12:.LC308
	bl	nand_print
.L1133:
	ldrh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	bl	get_phy_block_addr
	str	x0, [x29, 80]
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	add	x2, x29, 32
	add	x0, x29, 48
	mov	x5, x2
	ldr	x4, [x29, 88]
	mov	w3, 8
	ldrh	w2, [x29, 20]
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3472]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 76]
	ldrb	w0, [x29, 32]
	mov	w10, w0
	ldrb	w0, [x29, 33]
	mov	w11, w0
	ldrb	w0, [x29, 34]
	mov	w12, w0
	ldrb	w0, [x29, 35]
	mov	w13, w0
	ldrb	w0, [x29, 36]
	mov	w14, w0
	ldrb	w0, [x29, 37]
	mov	w15, w0
	ldrb	w0, [x29, 38]
	mov	w16, w0
	ldrb	w0, [x29, 39]
	mov	w1, w0
	ldrb	w0, [x29, 40]
	mov	w2, w0
	ldrb	w0, [x29, 41]
	mov	w3, w0
	ldrb	w0, [x29, 42]
	mov	w4, w0
	ldrb	w0, [x29, 43]
	mov	w5, w0
	ldrb	w0, [x29, 44]
	mov	w6, w0
	ldrb	w0, [x29, 45]
	mov	w7, w0
	ldrb	w0, [x29, 46]
	mov	w8, w0
	ldrb	w0, [x29, 47]
	mov	w9, w0
	adrp	x0, .LC309
	add	x0, x0, :lo12:.LC309
	str	w9, [sp, 64]
	str	w8, [sp, 56]
	str	w7, [sp, 48]
	str	w6, [sp, 40]
	str	w5, [sp, 32]
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w7, w16
	mov	w6, w15
	mov	w5, w14
	mov	w4, w13
	mov	w3, w12
	mov	w2, w11
	mov	w1, w10
	bl	nand_print_dbg
	ldr	x0, [x29, 88]
	ldrb	w0, [x0]
	mov	w8, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w9, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w10, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w11, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 4
	ldrb	w0, [x0]
	mov	w5, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 5
	ldrb	w0, [x0]
	mov	w6, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 6
	ldrb	w0, [x0]
	mov	w7, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 7
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 8
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 9
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 88]
	add	x0, x0, 10
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC310
	add	x0, x0, :lo12:.LC310
	str	w4, [sp, 24]
	str	w3, [sp, 16]
	str	w2, [sp, 8]
	str	w1, [sp]
	mov	w4, w11
	mov	w3, w10
	mov	w2, w9
	mov	w1, w8
	bl	nand_print_dbg
	ldr	x0, [x29, 88]
	bl	nftl_free
	ldr	w0, [x29, 76]
	add	sp, x29, 0
	ldp	x29, x30, [sp], 96
	ret
	.size	nand_dbg_zone_phy_read, .-nand_dbg_zone_phy_read
	.section	.rodata
	.align	3
.LC311:
	.string	"[NE]%s:malloc 8192 fail!\n"
	.text
	.align	2
	.global	nand_dbg_zone_phy_write
	.type	nand_dbg_zone_phy_write, %function
nand_dbg_zone_phy_write:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	strh	w2, [x29, 20]
	mov	w0, 8192
	bl	nftl_malloc
	str	x0, [x29, 88]
	ldr	x0, [x29, 88]
	cmp	x0, 0
	bne	.L1136
	adrp	x0, __func__.6555
	add	x1, x0, :lo12:__func__.6555
	adrp	x0, .LC311
	add	x0, x0, :lo12:.LC311
	bl	nand_print
.L1136:
	ldrh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	bl	get_phy_block_addr
	str	x0, [x29, 80]
	mov	w2, 4096
	mov	w1, -91
	ldr	x0, [x29, 88]
	bl	nand_memset
	mov	w0, -1
	strb	w0, [x29, 32]
	mov	w0, -1
	strb	w0, [x29, 33]
	mov	w0, -1
	strb	w0, [x29, 34]
	mov	w0, -1
	strb	w0, [x29, 35]
	mov	w0, -1
	strb	w0, [x29, 36]
	mov	w0, -91
	strb	w0, [x29, 37]
	mov	w0, -91
	strb	w0, [x29, 38]
	mov	w0, -1
	strb	w0, [x29, 39]
	mov	w0, -1
	strb	w0, [x29, 40]
	mov	w0, -1
	strb	w0, [x29, 41]
	mov	w0, -1
	strb	w0, [x29, 42]
	ldr	x0, [x29, 80]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x3, x29, 32
	add	x0, x29, 48
	mov	x5, x3
	ldr	x4, [x29, 88]
	mov	w3, w2
	ldrh	w2, [x29, 20]
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3480]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 76]
	ldr	x0, [x29, 88]
	bl	nftl_free
	ldr	w0, [x29, 76]
	ldp	x29, x30, [sp], 96
	ret
	.size	nand_dbg_zone_phy_write, .-nand_dbg_zone_phy_write
	.align	2
	.global	nand_dbg_phy_write
	.type	nand_dbg_phy_write, %function
nand_dbg_phy_write:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	strh	w0, [x29, 30]
	strh	w1, [x29, 28]
	strh	w2, [x29, 26]
	mov	w0, 8192
	bl	nftl_malloc
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L1139
	adrp	x0, __func__.6564
	add	x1, x0, :lo12:__func__.6564
	adrp	x0, .LC311
	add	x0, x0, :lo12:.LC311
	bl	nand_print
.L1139:
	mov	w2, 4096
	mov	w1, -91
	ldr	x0, [x29, 56]
	bl	nand_memset
	mov	w0, -1
	strb	w0, [x29, 32]
	mov	w0, -1
	strb	w0, [x29, 33]
	mov	w0, -1
	strb	w0, [x29, 34]
	mov	w0, -1
	strb	w0, [x29, 35]
	mov	w0, -1
	strb	w0, [x29, 36]
	mov	w0, -91
	strb	w0, [x29, 37]
	mov	w0, -91
	strb	w0, [x29, 38]
	mov	w0, -1
	strb	w0, [x29, 39]
	mov	w0, -1
	strb	w0, [x29, 40]
	mov	w0, -1
	strb	w0, [x29, 41]
	mov	w0, -1
	strb	w0, [x29, 42]
	add	x0, x29, 32
	mov	x5, x0
	ldr	x4, [x29, 56]
	mov	w3, 8
	ldrh	w2, [x29, 26]
	ldrh	w1, [x29, 28]
	ldrh	w0, [x29, 30]
	bl	page_write
	str	w0, [x29, 52]
	ldr	x0, [x29, 56]
	bl	nftl_free
	ldr	w0, [x29, 52]
	ldp	x29, x30, [sp], 64
	ret
	.size	nand_dbg_phy_write, .-nand_dbg_phy_write
	.align	2
	.global	nand_dbg_zone_erase
	.type	nand_dbg_zone_erase, %function
nand_dbg_zone_erase:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	strh	w2, [x29, 20]
	ldrh	w1, [x29, 22]
	ldr	x0, [x29, 24]
	bl	get_phy_block_addr
	str	x0, [x29, 40]
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [x29, 40]
	ldrh	w2, [x29, 20]
	ldr	x1, [x29, 40]
	ldr	x0, [x29, 24]
	bl	erase_block
	str	w0, [x29, 36]
	ldr	w0, [x29, 36]
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_dbg_zone_erase, .-nand_dbg_zone_erase
	.align	2
	.global	nand_dbg_phy_erase
	.type	nand_dbg_phy_erase, %function
nand_dbg_phy_erase:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	strh	w0, [x29, 30]
	strh	w1, [x29, 28]
	ldrh	w1, [x29, 28]
	ldrh	w0, [x29, 30]
	bl	block_erase
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_dbg_phy_erase, .-nand_dbg_phy_erase
	.align	2
	.global	nand_dbg_single_phy_erase
	.type	nand_dbg_single_phy_erase, %function
nand_dbg_single_phy_erase:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	strh	w0, [x29, 30]
	strh	w1, [x29, 28]
	ldrh	w0, [x29, 30]
	ldrh	w1, [x29, 28]
	bl	nand_physic_erase_block
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_dbg_single_phy_erase, .-nand_dbg_single_phy_erase
	.align	2
	.global	clean_block_list
	.type	clean_block_list, %function
clean_block_list:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
.L1150:
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L1148
	b	.L1149
.L1148:
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 40]
	mov	w1, -1
	str	w1, [x0, 8]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 24]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 32]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 40]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 48]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 56]
	b	.L1150
.L1149:
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_invalid_page_list
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	beq	.L1155
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 40]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 40]
	mov	w1, -1
	str	w1, [x0, 8]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 24]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 32]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 40]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 48]
	ldr	x0, [x29, 40]
	str	xzr, [x0, 56]
	b	.L1149
.L1155:
	nop
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	clean_block_list, .-clean_block_list
	.section	.rodata
	.align	3
.LC312:
	.string	"[ND]zoneNO:%d total_pages: %d\n"
	.align	3
.LC313:
	.string	"[ND]nand_memset bytes: %d\n"
	.text
	.align	2
	.global	nand_clean_zone_table2
	.type	nand_clean_zone_table2, %function
nand_clean_zone_table2:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 56]
	ldr	w0, [x29, 56]
	lsl	w0, w0, 2
	str	w0, [x29, 52]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	adrp	x0, .LC312
	add	x0, x0, :lo12:.LC312
	ldr	w2, [x29, 56]
	bl	nand_print_dbg
	ldr	w0, [x29, 52]
	lsr	w0, w0, 20
	str	w0, [x29, 48]
	str	wzr, [x29, 60]
	b	.L1157
.L1158:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	mov	w2, 1048576
	mov	w1, -1
	bl	nand_memset
	adrp	x0, .LC313
	add	x0, x0, :lo12:.LC313
	mov	w1, 1048576
	bl	nand_print_dbg
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1157:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bcc	.L1158
	ldr	w0, [x29, 52]
	and	w0, w0, 1048575
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1159
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	ldr	w2, [x29, 44]
	mov	w1, -1
	bl	nand_memset
	adrp	x0, .LC313
	add	x0, x0, :lo12:.LC313
	ldr	w1, [x29, 44]
	bl	nand_print_dbg
.L1159:
	ldr	x0, [x29, 24]
	str	xzr, [x0, 528]
	ldr	x0, [x29, 24]
	mov	w1, -1
	strh	w1, [x0, 584]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 592]
	ldr	x0, [x29, 24]
	mov	w1, -1
	strh	w1, [x0, 648]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 304]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 312]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 288]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 296]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 320]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 328]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 368]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 376]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 352]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 360]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 384]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 392]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 432]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 440]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 416]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 424]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 448]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 456]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 28]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 30]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 696]
	str	wzr, [x29, 60]
	b	.L1160
.L1161:
	ldr	w0, [x29, 60]
	uxth	w2, w0
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 704
	mov	w1, w2
	strh	w1, [x0]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 706
	strh	wzr, [x0]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 712
	str	xzr, [x0]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 22
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 16
	str	xzr, [x0]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 22
	lsl	x0, x0, 5
	add	x0, x1, x0
	add	x0, x0, 24
	str	xzr, [x0]
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1160:
	ldr	w0, [x29, 60]
	cmp	w0, 9
	bls	.L1161
	ldr	x0, [x29, 24]
	mov	w1, 255
	strh	w1, [x0, 1024]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 698]
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 1026]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1032]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1040]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 1048]
	mov	w0, 0
	ldp	x29, x30, [sp], 64
	ret
	.size	nand_clean_zone_table2, .-nand_clean_zone_table2
	.align	2
	.global	nftl_clean_logic_page_map
	.type	nftl_clean_logic_page_map, %function
nftl_clean_logic_page_map:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 56]
	ldr	w0, [x29, 56]
	lsl	w0, w0, 2
	str	w0, [x29, 52]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	adrp	x0, .LC312
	add	x0, x0, :lo12:.LC312
	ldr	w2, [x29, 56]
	bl	nand_print_dbg
	ldr	w0, [x29, 52]
	lsr	w0, w0, 20
	str	w0, [x29, 48]
	str	wzr, [x29, 60]
	b	.L1164
.L1165:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	mov	w2, 1048576
	mov	w1, -1
	bl	nand_memset
	adrp	x0, .LC313
	add	x0, x0, :lo12:.LC313
	mov	w1, 1048576
	bl	nand_print_dbg
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1164:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bcc	.L1165
	ldr	w0, [x29, 52]
	and	w0, w0, 1048575
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1166
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x0, 8
	lsl	x0, x0, 3
	add	x0, x1, x0
	ldr	x0, [x0, 8]
	ldr	w2, [x29, 44]
	mov	w1, -1
	bl	nand_memset
	adrp	x0, .LC313
	add	x0, x0, :lo12:.LC313
	ldr	w1, [x29, 44]
	bl	nand_print_dbg
.L1166:
	mov	w0, 0
	ldp	x29, x30, [sp], 64
	ret
	.size	nftl_clean_logic_page_map, .-nftl_clean_logic_page_map
	.align	2
	.global	nftl_clean_used_list
	.type	nftl_clean_used_list, %function
nftl_clean_used_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	wzr, [sp, 28]
	str	wzr, [sp, 28]
	b	.L1169
.L1170:
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldrsw	x0, [sp, 28]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 48]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldrsw	x0, [sp, 28]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 56]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L1169:
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	mov	w1, w0
	ldr	w0, [sp, 28]
	cmp	w1, w0
	bgt	.L1170
	ldr	x0, [sp, 8]
	str	xzr, [x0, 432]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 440]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 416]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 424]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 448]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 456]
	nop
	add	sp, sp, 32
	ret
	.size	nftl_clean_used_list, .-nftl_clean_used_list
	.align	2
	.global	nftl_clean_invalid_list
	.type	nftl_clean_invalid_list, %function
nftl_clean_invalid_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	wzr, [sp, 28]
	str	wzr, [sp, 28]
	b	.L1172
.L1173:
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldrsw	x0, [sp, 28]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 16]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldr	x1, [x0, 32]
	ldrsw	x0, [sp, 28]
	lsl	x0, x0, 6
	add	x0, x1, x0
	str	xzr, [x0, 24]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L1172:
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 24]
	mov	w1, w0
	ldr	w0, [sp, 28]
	cmp	w1, w0
	bgt	.L1173
	ldr	x0, [sp, 8]
	str	xzr, [x0, 368]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 376]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 352]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 360]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 384]
	ldr	x0, [sp, 8]
	str	xzr, [x0, 392]
	nop
	add	sp, sp, 32
	ret
	.size	nftl_clean_invalid_list, .-nftl_clean_invalid_list
	.align	2
	.global	nand_clean_zone_table
	.type	nand_clean_zone_table, %function
nand_clean_zone_table:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	x0, [x29, 40]
	mov	w1, 1000
	ldr	x0, [x29, 40]
	bl	__nand_flush_write_cache
	ldr	x0, [x29, 40]
	bl	clean_block_list
	ldr	x0, [x29, 40]
	bl	nand_clean_zone_table2
	ldr	x0, [x29, 40]
	bl	nftl_cache_exit
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_clean_zone_table, .-nand_clean_zone_table
	.section	.rodata
	.align	3
.LC314:
	.string	"[ND]build_zone_list start\n"
	.align	3
.LC315:
	.string	"[ND]nftl_cache_init start\n"
	.align	3
.LC316:
	.string	"[ND]=================================2=======================================\n"
	.align	3
.LC317:
	.string	"[ND]nand_find_zone_table end\n"
	.text
	.align	2
	.global	nand_find_zone_table
	.type	nand_find_zone_table, %function
nand_find_zone_table:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	bl	clean_block_list
	ldr	x0, [x29, 40]
	bl	nand_clean_zone_table2
	adrp	x0, .LC314
	add	x0, x0, :lo12:.LC314
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	bl	build_zone_list
	adrp	x0, .LC315
	add	x0, x0, :lo12:.LC315
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	bl	nftl_cache_init
	adrp	x0, .LC316
	add	x0, x0, :lo12:.LC316
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	bl	print_block_invalid_list
	ldr	x0, [x29, 40]
	bl	print_free_list
	adrp	x0, .LC316
	add	x0, x0, :lo12:.LC316
	bl	nand_print_dbg
	adrp	x0, .LC317
	add	x0, x0, :lo12:.LC317
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_find_zone_table, .-nand_find_zone_table
	.section	.rodata
	.align	3
.LC318:
	.string	"[NE][NE]%s:malloc 512 fail!\n"
	.text
	.align	2
	.global	write_data_to_nand
	.type	write_data_to_nand, %function
write_data_to_nand:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w0, 512
	bl	nftl_malloc
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L1179
	adrp	x0, __func__.6642
	add	x1, x0, :lo12:__func__.6642
	adrp	x0, .LC318
	add	x0, x0, :lo12:.LC318
	bl	nand_print
.L1179:
	mov	w2, 512
	mov	w1, -86
	ldr	x0, [x29, 32]
	bl	nand_memset
	str	wzr, [x29, 44]
	b	.L1180
.L1181:
	ldr	w0, [x29, 44]
	ldr	x3, [x29, 32]
	mov	w2, 1
	mov	w1, w0
	ldr	x0, [x29, 24]
	bl	__nand_write
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1180:
	ldr	w1, [x29, 44]
	mov	w0, 38911
	cmp	w1, w0
	ble	.L1181
	ldr	x0, [x29, 32]
	bl	nftl_free
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	write_data_to_nand, .-write_data_to_nand
	.section	.rodata
	.align	3
.LC319:
	.string	"[ND]==========================\n"
	.text
	.align	2
	.global	nand_check_table
	.type	nand_check_table, %function
nand_check_table:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	x0, [x29, 40]
	mov	w1, 1000
	ldr	x0, [x29, 40]
	bl	__nand_flush_write_cache
	adrp	x0, .LC319
	add	x0, x0, :lo12:.LC319
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	bl	print_nftl_zone
	adrp	x0, .LC319
	add	x0, x0, :lo12:.LC319
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	bl	nand_clean_zone_table
	adrp	x0, .LC319
	add	x0, x0, :lo12:.LC319
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	bl	nand_find_zone_table
	ldr	x0, [x29, 40]
	bl	write_data_to_nand
	adrp	x0, .LC319
	add	x0, x0, :lo12:.LC319
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	nand_check_table, .-nand_check_table
	.section	.rodata
	.align	3
.LC320:
	.string	"%.2x "
	.text
	.align	2
	.type	_DumpHexData, %function
_DumpHexData:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	str	x0, [x29, 40]
	str	wzr, [x29, 36]
	b	.L1186
.L1188:
	ldr	x0, [x29, 40]
	add	x1, x0, 1
	str	x1, [x29, 40]
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC320
	add	x0, x0, :lo12:.LC320
	bl	nand_print
	ldr	w0, [x29, 36]
	negs	w1, w0
	and	w0, w0, 15
	and	w1, w1, 15
	csneg	w0, w0, w1, mi
	cmp	w0, 15
	bne	.L1187
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print
.L1187:
	ldr	w0, [x29, 36]
	add	w0, w0, 1
	str	w0, [x29, 36]
.L1186:
	ldr	w1, [x29, 36]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	blt	.L1188
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print
	nop
	ldp	x29, x30, [sp], 48
	ret
	.size	_DumpHexData, .-_DumpHexData
	.section	.rodata
	.align	3
.LC321:
	.string	"[NE]exception case bad block\n"
	.align	3
.LC322:
	.string	"[NE]exception case wbc\n"
	.align	3
.LC323:
	.string	"[NE]exception case gd\n"
	.align	3
.LC324:
	.string	"[NE]%s %d, flag = %d\n"
	.align	3
.LC325:
	.string	"[NE]OOB :"
	.text
	.align	2
	.global	nand_power_off_ecc_error
	.type	nand_power_off_ecc_error, %function
nand_power_off_ecc_error:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0]
	cmp	w0, 255
	beq	.L1191
	adrp	x0, .LC321
	add	x0, x0, :lo12:.LC321
	bl	nand_print
	mov	w0, 1
	str	w0, [x29, 60]
	b	.L1192
.L1191:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 28]
	cmp	w0, 0
	beq	.L1193
	ldr	x2, [x29, 24]
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	crc_wrong
	cmp	w0, 0
	beq	.L1192
	adrp	x0, .LC322
	add	x0, x0, :lo12:.LC322
	bl	nand_print
	mov	w0, 1
	str	w0, [x29, 60]
	b	.L1192
.L1193:
	ldr	x0, [x29, 24]
	add	x0, x0, 11
	ldrb	w0, [x0]
	cmp	w0, 165
	bne	.L1194
	ldr	x0, [x29, 24]
	add	x0, x0, 15
	ldrb	w0, [x0]
	cmp	w0, 165
	beq	.L1192
.L1194:
	adrp	x0, .LC323
	add	x0, x0, :lo12:.LC323
	bl	nand_print
	mov	w0, 1
	str	w0, [x29, 60]
	nop
.L1192:
	ldr	w0, [x29, 60]
	cmp	w0, 1
	bne	.L1195
	adrp	x0, __FUNCTION__.6666
	add	x1, x0, :lo12:__FUNCTION__.6666
	adrp	x0, .LC324
	add	x0, x0, :lo12:.LC324
	ldr	w3, [x29, 60]
	mov	w2, 3116
	bl	nand_print
	adrp	x0, .LC325
	add	x0, x0, :lo12:.LC325
	bl	nand_print
	mov	w1, 16
	ldr	x0, [x29, 24]
	bl	_DumpHexData
.L1195:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	nand_power_off_ecc_error, .-nand_power_off_ecc_error
	.section	.rodata
	.align	3
.LC326:
	.string	"[NE]%s erase fb@%d fail\n"
	.align	3
.LC327:
	.string	"[NE]%s erase ub@%u fail\n"
	.align	3
.LC328:
	.string	"[NE]not current block\n"
	.text
	.align	2
	.global	clean_zone
	.type	clean_zone, %function
clean_zone:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 64]
	str	wzr, [x29, 68]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	beq	.L1198
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 3456]
	cmp	w0, 0
	bne	.L1198
	ldr	w0, [x29, 68]
	b	.L1209
.L1198:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 304]
	str	x0, [x29, 72]
	b	.L1200
.L1202:
	ldr	x0, [x29, 72]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 40
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 64]
	ldr	w0, [x29, 64]
	cmp	w0, 0
	beq	.L1201
	ldr	x0, [x29, 72]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, __func__.6674
	add	x1, x0, :lo12:__func__.6674
	adrp	x0, .LC326
	add	x0, x0, :lo12:.LC326
	bl	nand_print
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 72]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 28]
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list_by_block
.L1201:
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 72]
	strh	w1, [x0, 6]
	mov	w0, 1
	str	w0, [x29, 68]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 32]
	str	x0, [x29, 72]
.L1200:
	ldr	x0, [x29, 72]
	cmp	x0, 0
	bne	.L1202
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 448]
	str	x0, [x29, 72]
	b	.L1203
.L1205:
	ldr	x0, [x29, 72]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 40
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 64]
	ldr	w0, [x29, 64]
	cmp	w0, 0
	beq	.L1204
	ldr	x0, [x29, 72]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, __func__.6674
	add	x1, x0, :lo12:__func__.6674
	adrp	x0, .LC327
	add	x0, x0, :lo12:.LC327
	bl	nand_print
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 72]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 28]
.L1204:
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 72]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 4]
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_free_list
	mov	w0, 1
	str	w0, [x29, 68]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 48]
	str	x0, [x29, 72]
.L1203:
	ldr	x0, [x29, 72]
	cmp	x0, 0
	bne	.L1205
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	cmp	x0, 0
	beq	.L1206
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldrh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w2, [x0, 28]
	add	x0, x29, 40
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3464]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	str	w0, [x29, 64]
	ldr	w0, [x29, 64]
	cmp	w0, 0
	beq	.L1207
	ldr	x0, [x29, 72]
	ldrh	w0, [x0]
	mov	w2, w0
	adrp	x0, __func__.6674
	add	x1, x0, :lo12:__func__.6674
	adrp	x0, .LC326
	add	x0, x0, :lo12:.LC326
	bl	nand_print
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3504]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 72]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 28]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 24]
	strh	w1, [x0, 28]
.L1207:
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 72]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 4]
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 24]
	bl	put_phy_block_to_free_list
	mov	w0, 1
	str	w0, [x29, 68]
.L1206:
	ldr	x0, [x29, 24]
	bl	nftl_clean_logic_page_map
	ldr	x0, [x29, 24]
	bl	nftl_clean_used_list
	ldr	x0, [x29, 24]
	bl	nftl_clean_invalid_list
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	mov	x1, x0
	ldr	x0, [x29, 24]
	str	x1, [x0, 528]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	cmp	x0, 0
	bne	.L1208
	adrp	x0, .LC328
	add	x0, x0, :lo12:.LC328
	bl	nand_print
	mov	w0, 1
	str	w0, [x29, 64]
.L1208:
	ldr	x0, [x29, 24]
	strh	wzr, [x0, 584]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 528]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 528]
	ldr	w1, [x1, 8]
	add	w1, w1, 1
	str	w1, [x0, 8]
	ldr	w0, [x29, 68]
	str	w0, [x29, 64]
	ldr	w0, [x29, 64]
.L1209:
	ldp	x29, x30, [sp], 80
	ret
	.size	clean_zone, .-clean_zone
	.section	.rodata
	.align	3
.LC329:
	.string	"[NE]%s:malloc fail for cache_node!\n"
	.align	3
.LC330:
	.string	"[NE]%s:malloc fail for cache_page_buf!\n"
	.text
	.align	2
	.global	nftl_cache_init
	.type	nftl_cache_init, %function
nftl_cache_init:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 1072
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w1, [x0, 22]
	ldr	x0, [x29, 32]
	strh	w1, [x0]
	ldr	x0, [x29, 32]
	strh	wzr, [x0, 2]
	ldr	x0, [x29, 32]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 32]
	strh	wzr, [x0, 6]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1712]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1720]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1728]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1736]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1768]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1776]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1784]
	ldr	x0, [x29, 32]
	str	xzr, [x0, 1792]
	str	wzr, [x29, 44]
	b	.L1211
.L1214:
	ldr	w0, [x29, 44]
	uxth	w3, w0
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 8
	mov	w1, w3
	strh	w1, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 10
	mov	w1, 255
	strh	w1, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 16
	mov	w1, -1
	str	w1, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 32
	str	xzr, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 40
	str	xzr, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 48
	str	xzr, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 56
	str	xzr, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 12
	strh	wzr, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 14
	strh	wzr, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	bl	nftl_malloc
	mov	x3, x0
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 24
	str	x3, [x0]
	ldr	x2, [x29, 32]
	ldr	w1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 24
	ldr	x0, [x0]
	cmp	x0, 0
	bne	.L1212
	adrp	x0, __func__.6688
	add	x1, x0, :lo12:__func__.6688
	adrp	x0, .LC329
	add	x0, x0, :lo12:.LC329
	bl	nand_print
	mov	w0, 1
	b	.L1213
.L1212:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1211:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1214
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 32]
	str	x1, [x0, 1800]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 1800]
	cmp	x0, 0
	bne	.L1215
	adrp	x0, __func__.6688
	add	x1, x0, :lo12:__func__.6688
	adrp	x0, .LC330
	add	x0, x0, :lo12:.LC330
	bl	nand_print
	mov	w0, 1
	b	.L1213
.L1215:
	mov	w0, 0
.L1213:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_cache_init, .-nftl_cache_init
	.align	2
	.global	nftl_cache_exit
	.type	nftl_cache_exit, %function
nftl_cache_exit:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	add	x0, x0, 1072
	str	x0, [x29, 32]
	str	wzr, [x29, 44]
	b	.L1217
.L1218:
	ldr	x2, [x29, 32]
	ldrsw	x1, [x29, 44]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 24
	ldr	x0, [x0]
	bl	nftl_free
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1217:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1218
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 1800]
	bl	nftl_free
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_cache_exit, .-nftl_cache_exit
	.align	2
	.global	nftl_cache_clean
	.type	nftl_cache_clean, %function
nftl_cache_clean:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1072
	str	x0, [sp, 16]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 3448]
	ldrh	w1, [x0, 22]
	ldr	x0, [sp, 16]
	strh	w1, [x0]
	ldr	x0, [sp, 16]
	strh	wzr, [x0, 2]
	ldr	x0, [sp, 16]
	strh	wzr, [x0, 4]
	ldr	x0, [sp, 16]
	strh	wzr, [x0, 6]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1712]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1720]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1728]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1736]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1768]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1776]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1784]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 1792]
	str	wzr, [sp, 28]
	b	.L1221
.L1222:
	ldr	w0, [sp, 28]
	uxth	w3, w0
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 8
	mov	w1, w3
	strh	w1, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 10
	mov	w1, 255
	strh	w1, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 16
	mov	w1, -1
	str	w1, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 32
	str	xzr, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 40
	str	xzr, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 48
	str	xzr, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 56
	str	xzr, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 12
	strh	wzr, [x0]
	ldr	x2, [sp, 16]
	ldr	w1, [sp, 28]
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 14
	strh	wzr, [x0]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L1221:
	ldr	x0, [sp, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	w0, [sp, 28]
	cmp	w1, w0
	bhi	.L1222
	mov	w0, 0
	add	sp, sp, 32
	ret
	.size	nftl_cache_clean, .-nftl_cache_clean
	.align	2
	.global	search_from_cache_read_list
	.type	search_from_cache_read_list, %function
search_from_cache_read_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	ldr	x0, [sp, 8]
	add	x0, x0, 1688
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 24]
	str	x0, [sp, 24]
	b	.L1225
.L1228:
	ldr	x0, [sp, 24]
	ldr	w1, [x0, 8]
	ldr	w0, [sp, 4]
	cmp	w1, w0
	bne	.L1226
	ldr	x0, [sp, 24]
	b	.L1227
.L1226:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 24]
	str	x0, [sp, 24]
.L1225:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L1228
	mov	x0, 0
.L1227:
	add	sp, sp, 32
	ret
	.size	search_from_cache_read_list, .-search_from_cache_read_list
	.align	2
	.global	search_from_cache_write_list
	.type	search_from_cache_write_list, %function
search_from_cache_write_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	ldr	x0, [sp, 8]
	add	x0, x0, 1744
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 40]
	str	x0, [sp, 24]
	b	.L1230
.L1233:
	ldr	x0, [sp, 24]
	ldr	w1, [x0, 8]
	ldr	w0, [sp, 4]
	cmp	w1, w0
	bne	.L1231
	ldr	x0, [sp, 24]
	b	.L1232
.L1231:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 40]
	str	x0, [sp, 24]
.L1230:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L1233
	mov	x0, 0
.L1232:
	add	sp, sp, 32
	ret
	.size	search_from_cache_write_list, .-search_from_cache_write_list
	.align	2
	.global	add_to_cache_read_list_tail
	.type	add_to_cache_read_list_tail, %function
add_to_cache_read_list_tail:
	sub	sp, sp, #48
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	w2, [sp, 12]
	ldr	x0, [sp, 24]
	add	x0, x0, 1688
	str	x0, [sp, 40]
	b	.L1235
.L1236:
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 24]
	str	x0, [sp, 40]
.L1235:
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	bne	.L1236
	ldr	x0, [sp, 40]
	ldr	x1, [sp, 16]
	str	x1, [x0, 24]
	ldr	x0, [sp, 16]
	str	xzr, [x0, 24]
	ldr	x0, [sp, 16]
	ldr	x1, [sp, 40]
	str	x1, [x0, 32]
	ldr	w0, [sp, 12]
	uxth	w1, w0
	ldr	x0, [sp, 16]
	strh	w1, [x0, 2]
	ldr	w0, [sp, 12]
	cmp	w0, 0
	bne	.L1237
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 2]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [sp, 24]
	strh	w1, [x0, 2]
	b	.L1238
.L1237:
	ldr	x0, [sp, 24]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [sp, 24]
	strh	w1, [x0, 6]
.L1238:
	mov	w0, 0
	add	sp, sp, 48
	ret
	.size	add_to_cache_read_list_tail, .-add_to_cache_read_list_tail
	.align	2
	.global	add_to_cache_write_list_tail
	.type	add_to_cache_write_list_tail, %function
add_to_cache_write_list_tail:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	add	x0, x0, 1744
	str	x0, [sp, 24]
	b	.L1241
.L1242:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 40]
	str	x0, [sp, 24]
.L1241:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 40]
	cmp	x0, 0
	bne	.L1242
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 40]
	ldr	x0, [sp]
	str	xzr, [x0, 40]
	ldr	x0, [sp]
	ldr	x1, [sp, 24]
	str	x1, [x0, 48]
	ldr	x0, [sp]
	mov	w1, 1
	strh	w1, [x0, 2]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0, 4]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0, 4]
	mov	w0, 0
	add	sp, sp, 32
	ret
	.size	add_to_cache_write_list_tail, .-add_to_cache_write_list_tail
	.align	2
	.global	del_from_cache_read_list
	.type	del_from_cache_read_list, %function
del_from_cache_read_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp]
	ldr	x0, [x0, 32]
	str	x0, [sp, 24]
	ldr	x0, [sp]
	ldr	x1, [x0, 24]
	ldr	x0, [sp, 24]
	str	x1, [x0, 24]
	ldr	x0, [sp]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	beq	.L1245
	ldr	x0, [sp]
	ldr	x0, [x0, 24]
	ldr	x1, [sp, 24]
	str	x1, [x0, 32]
.L1245:
	ldr	x0, [sp]
	ldrh	w0, [x0, 2]
	cmp	w0, 0
	bne	.L1246
	ldr	x0, [sp, 8]
	ldrh	w0, [x0, 2]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0, 2]
	b	.L1247
.L1246:
	ldr	x0, [sp, 8]
	ldrh	w0, [x0, 6]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0, 6]
.L1247:
	ldr	x0, [sp]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [sp]
	str	xzr, [x0, 24]
	ldr	x0, [sp]
	str	xzr, [x0, 32]
	ldr	x0, [sp]
	add	sp, sp, 32
	ret
	.size	del_from_cache_read_list, .-del_from_cache_read_list
	.align	2
	.global	del_from_cache_write_list
	.type	del_from_cache_write_list, %function
del_from_cache_write_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp]
	ldr	x0, [x0, 48]
	str	x0, [sp, 24]
	ldr	x0, [sp]
	ldr	x1, [x0, 40]
	ldr	x0, [sp, 24]
	str	x1, [x0, 40]
	ldr	x0, [sp]
	ldr	x0, [x0, 40]
	cmp	x0, 0
	beq	.L1250
	ldr	x0, [sp]
	ldr	x0, [x0, 40]
	ldr	x1, [sp, 24]
	str	x1, [x0, 48]
.L1250:
	ldr	x0, [sp]
	str	xzr, [x0, 40]
	ldr	x0, [sp]
	str	xzr, [x0, 48]
	ldr	x0, [sp]
	mov	w1, 255
	strh	w1, [x0, 2]
	ldr	x0, [sp, 8]
	ldrh	w0, [x0, 4]
	sub	w0, w0, #1
	uxth	w1, w0
	ldr	x0, [sp, 8]
	strh	w1, [x0, 4]
	ldr	x0, [sp]
	add	sp, sp, 32
	ret
	.size	del_from_cache_write_list, .-del_from_cache_write_list
	.align	2
	.global	get_empty_cahce_node
	.type	get_empty_cahce_node, %function
get_empty_cahce_node:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	xzr, [x29, 32]
	strh	wzr, [x29, 46]
	b	.L1253
.L1256:
	ldrh	w0, [x29, 46]
	ldr	x2, [x29, 24]
	sxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	add	x0, x2, x0
	add	x0, x0, 10
	ldrh	w0, [x0]
	cmp	w0, 255
	bne	.L1254
	ldrh	w0, [x29, 46]
	sxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 3
	sub	x0, x0, x1
	lsl	x0, x0, 3
	ldr	x1, [x29, 24]
	add	x0, x1, x0
	add	x0, x0, 8
	b	.L1255
.L1254:
	ldrh	w0, [x29, 46]
	add	w0, w0, 1
	strh	w0, [x29, 46]
.L1253:
	ldr	x0, [x29, 24]
	ldrh	w0, [x0]
	ldrh	w1, [x29, 46]
	cmp	w1, w0
	bcc	.L1256
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 6]
	cmp	w0, 9
	bls	.L1257
	ldr	x0, [x29, 24]
	add	x0, x0, 1688
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	str	x0, [x29, 32]
	b	.L1258
.L1260:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 2]
	cmp	w0, 2
	bne	.L1259
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 24]
	bl	del_from_cache_read_list
	b	.L1255
.L1259:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	str	x0, [x29, 32]
.L1258:
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L1260
	b	.L1261
.L1257:
	ldr	x0, [x29, 24]
	add	x0, x0, 1688
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	str	x0, [x29, 32]
	b	.L1262
.L1264:
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 2]
	cmp	w0, 0
	bne	.L1263
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 24]
	bl	del_from_cache_read_list
	b	.L1255
.L1263:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	str	x0, [x29, 32]
.L1262:
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L1264
.L1261:
	mov	x0, 0
.L1255:
	ldp	x29, x30, [sp], 48
	ret
	.size	get_empty_cahce_node, .-get_empty_cahce_node
	.section	.rodata
	.align	3
.LC331:
	.string	"[ND]read sector:0x%x,len:0x%x\n"
	.text
	.align	2
	.global	__nand_read
	.type	__nand_read, %function
__nand_read:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 656]
	cmp	w0, 3
	bne	.L1266
	adrp	x0, .LC331
	add	x0, x0, :lo12:.LC331
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	bl	nand_print_dbg
.L1266:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 40]
	ldr	x1, [x1, 3440]
	ldr	x2, [x1, 64]
	ldr	w1, [x29, 32]
	add	x1, x2, x1
	str	x1, [x0, 64]
	ldr	x4, [x29, 24]
	ldr	w3, [x29, 32]
	ldr	w2, [x29, 36]
	ldr	x1, [x29, 40]
	mov	w0, 0
	bl	nand_op
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	__nand_read, .-__nand_read
	.section	.rodata
	.align	3
.LC332:
	.string	"[ND]write sector:0x%x,len:0x%x\n"
	.text
	.align	2
	.global	__nand_write
	.type	__nand_write, %function
__nand_write:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 656]
	cmp	w0, 3
	bne	.L1269
	adrp	x0, .LC332
	add	x0, x0, :lo12:.LC332
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	bl	nand_print_dbg
.L1269:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 40]
	ldr	x1, [x1, 3440]
	ldr	x2, [x1, 72]
	ldr	w1, [x29, 32]
	add	x1, x2, x1
	str	x1, [x0, 72]
	ldr	x4, [x29, 24]
	ldr	w3, [x29, 32]
	ldr	w2, [x29, 36]
	ldr	x1, [x29, 40]
	mov	w0, 1
	bl	nand_op
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	__nand_write, .-__nand_write
	.section	.rodata
	.align	3
.LC333:
	.string	"[ND]dicard sector:0x%x,len:0x%x\n"
	.text
	.align	2
	.global	__nand_discard
	.type	__nand_discard, %function
__nand_discard:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 656]
	cmp	w0, 3
	bne	.L1272
	adrp	x0, .LC333
	add	x0, x0, :lo12:.LC333
	ldr	w2, [x29, 16]
	ldr	w1, [x29, 20]
	bl	nand_print_dbg
.L1272:
	ldr	w2, [x29, 16]
	ldr	w1, [x29, 20]
	ldr	x0, [x29, 24]
	bl	nand_discard
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	__nand_discard, .-__nand_discard
	.section	.rodata
	.align	3
.LC334:
	.string	"[NE]paramter error. beyond reality capacity\n"
	.text
	.align	2
	.global	nand_op
	.type	nand_op, %function
nand_op:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	strb	w0, [x29, 47]
	str	x1, [x29, 32]
	str	w2, [x29, 40]
	str	w3, [x29, 28]
	str	x4, [x29, 16]
	str	wzr, [x29, 72]
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1275
	mov	w0, 0
	b	.L1276
.L1275:
	ldr	w1, [x29, 40]
	ldr	w0, [x29, 28]
	add	w1, w1, w0
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 32]
	cmp	w1, w0
	bls	.L1277
	adrp	x0, .LC334
	add	x0, x0, :lo12:.LC334
	bl	nand_print
	mov	w0, 1
	b	.L1276
.L1277:
	ldrb	w0, [x29, 47]
	cmp	w0, 0
	bne	.L1278
	adrp	x0, nand_cache_read
	add	x0, x0, :lo12:nand_cache_read
	str	x0, [x29, 64]
	b	.L1279
.L1278:
	adrp	x0, nand_cache_write
	add	x0, x0, :lo12:nand_cache_write
	str	x0, [x29, 64]
.L1279:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 40]
	udiv	w0, w0, w1
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	str	w0, [x29, 76]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 40]
	udiv	w2, w0, w1
	mul	w1, w2, w1
	sub	w0, w0, w1
	strh	w0, [x29, 58]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldrh	w0, [x29, 58]
	sub	w0, w1, w0
	strh	w0, [x29, 94]
	ldrh	w1, [x29, 94]
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bcc	.L1280
	ldr	w0, [x29, 28]
	strh	w0, [x29, 94]
	str	wzr, [x29, 28]
	b	.L1281
.L1280:
	ldrh	w0, [x29, 94]
	ldr	w1, [x29, 28]
	sub	w0, w1, w0
	str	w0, [x29, 28]
.L1281:
	ldr	x0, [x29, 16]
	str	x0, [x29, 80]
	ldrh	w0, [x29, 94]
	strh	w0, [x29, 92]
	ldrh	w0, [x29, 58]
	lsl	w0, w0, 8
	sxth	w1, w0
	ldrsh	w0, [x29, 94]
	orr	w0, w1, w0
	sxth	w0, w0
	strh	w0, [x29, 56]
	ldr	x4, [x29, 64]
	ldr	x3, [x29, 80]
	ldrh	w2, [x29, 56]
	ldr	w1, [x29, 76]
	ldr	x0, [x29, 32]
	blr	x4
	str	w0, [x29, 72]
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1283
	ldr	w0, [x29, 72]
	b	.L1276
.L1284:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	strh	w0, [x29, 94]
	ldr	w0, [x29, 76]
	add	w0, w0, 1
	str	w0, [x29, 76]
	ldrh	w0, [x29, 92]
	lsl	w0, w0, 9
	sxtw	x0, w0
	ldr	x1, [x29, 80]
	add	x0, x1, x0
	str	x0, [x29, 80]
	ldrh	w0, [x29, 94]
	strh	w0, [x29, 56]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 28]
	sub	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	strh	w0, [x29, 92]
	ldr	x4, [x29, 64]
	ldr	x3, [x29, 80]
	ldrh	w2, [x29, 56]
	ldr	w1, [x29, 76]
	ldr	x0, [x29, 32]
	blr	x4
	mov	w1, w0
	ldr	w0, [x29, 72]
	orr	w0, w0, w1
	str	w0, [x29, 72]
.L1283:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bcc	.L1284
	ldr	w0, [x29, 28]
	strh	w0, [x29, 94]
	ldr	w0, [x29, 76]
	add	w0, w0, 1
	str	w0, [x29, 76]
	ldrh	w0, [x29, 92]
	lsl	w0, w0, 9
	sxtw	x0, w0
	ldr	x1, [x29, 80]
	add	x0, x1, x0
	str	x0, [x29, 80]
	ldrh	w0, [x29, 94]
	strh	w0, [x29, 56]
	ldr	x4, [x29, 64]
	ldr	x3, [x29, 80]
	ldrh	w2, [x29, 56]
	ldr	w1, [x29, 76]
	ldr	x0, [x29, 32]
	blr	x4
	mov	w1, w0
	ldr	w0, [x29, 72]
	orr	w0, w0, w1
	str	w0, [x29, 72]
	ldr	w0, [x29, 72]
.L1276:
	ldp	x29, x30, [sp], 96
	ret
	.size	nand_op, .-nand_op
	.section	.rodata
	.align	3
.LC335:
	.string	"[NI]nand_cache_read error buf is  NULL!\n"
	.align	3
.LC336:
	.string	"[NE]error1 node %d,%d,%d!\n"
	.text
	.align	2
	.global	nand_cache_read
	.type	nand_cache_read, %function
nand_cache_read:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	strh	w2, [x29, 34]
	str	x3, [x29, 24]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1286
	adrp	x0, .LC335
	add	x0, x0, :lo12:.LC335
	bl	nand_print
	mov	w0, 0
	b	.L1287
.L1286:
	str	wzr, [x29, 92]
	ldrh	w0, [x29, 34]
	lsr	w0, w0, 8
	uxth	w0, w0
	str	w0, [x29, 88]
	ldrh	w0, [x29, 34]
	and	w0, w0, 255
	str	w0, [x29, 84]
	ldr	w0, [x29, 88]
	lsl	w0, w0, 9
	str	w0, [x29, 88]
	ldr	w0, [x29, 84]
	lsl	w0, w0, 9
	str	w0, [x29, 84]
	ldr	x0, [x29, 40]
	add	x0, x0, 1072
	str	x0, [x29, 72]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0]
	cmp	w0, 0
	beq	.L1288
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 24]
	str	x0, [x29, 64]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3520]
	ldr	x2, [x29, 64]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 92]
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 84]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	ldr	w0, [x29, 92]
	cmp	w0, 10
	bne	.L1289
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 116]
	add	w1, w1, 1
	str	w1, [x0, 116]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3512]
	ldr	x2, [x29, 64]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	wzr, [x29, 92]
.L1289:
	ldr	w0, [x29, 92]
	b	.L1287
.L1288:
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 72]
	bl	search_from_cache_write_list
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	beq	.L1290
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 4]
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 54]
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 6]
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 52]
	ldrh	w1, [x29, 54]
	ldrh	w0, [x29, 52]
	add	w0, w1, w0
	mov	w1, w0
	ldr	w0, [x29, 88]
	cmp	w1, w0
	bcc	.L1291
	ldr	w1, [x29, 88]
	ldr	w0, [x29, 84]
	add	w0, w1, w0
	ldrh	w2, [x29, 54]
	ldrh	w1, [x29, 52]
	add	w1, w2, w1
	cmp	w0, w1
	bhi	.L1291
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 84]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	mov	w0, 0
	b	.L1287
.L1291:
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3520]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 1800]
	mov	x2, x0
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 92]
	ldrh	w0, [x29, 52]
	ldr	x1, [x29, 64]
	add	x3, x1, x0
	ldr	x0, [x29, 72]
	ldr	x1, [x0, 1800]
	ldrh	w0, [x29, 52]
	add	x1, x1, x0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	ldrh	w0, [x29, 52]
	sub	w0, w2, w0
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldr	x0, [x29, 56]
	strh	w1, [x0, 6]
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 84]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	mov	w0, 0
	b	.L1287
.L1290:
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 72]
	bl	search_from_cache_read_list
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	beq	.L1292
	ldr	x0, [x29, 56]
	ldrh	w0, [x0, 2]
	cmp	w0, 0
	bne	.L1293
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 72]
	bl	del_from_cache_read_list
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 84]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	mov	w2, 0
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 72]
	bl	add_to_cache_read_list_tail
	b	.L1294
.L1293:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 84]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
.L1294:
	mov	w0, 0
	b	.L1287
.L1292:
	ldr	x0, [x29, 72]
	bl	get_empty_cahce_node
	str	x0, [x29, 56]
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L1295
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 2]
	mov	w2, w0
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 6]
	mov	w3, w0
	adrp	x0, .LC336
	add	x0, x0, :lo12:.LC336
	bl	nand_print
	mov	w0, 1
	b	.L1287
.L1295:
	ldr	x0, [x29, 56]
	ldr	w1, [x29, 36]
	str	w1, [x0, 8]
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3520]
	ldr	x2, [x29, 64]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 92]
	ldr	x0, [x29, 56]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldr	x0, [x29, 56]
	strh	w1, [x0, 6]
	mov	w2, 0
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 72]
	bl	add_to_cache_read_list_tail
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 84]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	ldr	w0, [x29, 92]
	cmp	w0, 10
	bne	.L1296
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 116]
	add	w1, w1, 1
	str	w1, [x0, 116]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3512]
	ldr	x2, [x29, 64]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	wzr, [x29, 92]
.L1296:
	ldr	w0, [x29, 92]
.L1287:
	ldp	x29, x30, [sp], 96
	ret
	.size	nand_cache_read, .-nand_cache_read
	.section	.rodata
	.align	3
.LC337:
	.string	"[NE]error1 bitmap %x!\n"
	.align	3
.LC338:
	.string	"[NE]ECC_LIMIT happened 12! page:%d!\n"
	.align	3
.LC339:
	.string	"[NE]error1 flush write cache to nand\n"
	.align	3
.LC340:
	.string	"[NE]error2 flush write cache to nand\n"
	.align	3
.LC341:
	.string	"[NI]ECC_LIMIT happened 11! page:%d!\n"
	.text
	.align	2
	.global	nand_cache_write
	.type	nand_cache_write, %function
nand_cache_write:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	strh	w2, [x29, 34]
	str	x3, [x29, 24]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1298
	mov	w0, 0
	b	.L1299
.L1298:
	ldrh	w0, [x29, 34]
	lsr	w0, w0, 8
	strh	w0, [x29, 94]
	ldrh	w0, [x29, 34]
	and	w0, w0, 255
	strh	w0, [x29, 92]
	ldrh	w0, [x29, 94]
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 94]
	ldrh	w0, [x29, 92]
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 92]
	str	wzr, [x29, 88]
	ldr	x0, [x29, 40]
	add	x0, x0, 1072
	str	x0, [x29, 80]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0]
	cmp	w0, 0
	beq	.L1300
	ldr	x0, [x29, 80]
	ldr	x0, [x0, 24]
	str	x0, [x29, 72]
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3520]
	ldr	x2, [x29, 72]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	ldrh	w0, [x29, 94]
	ldr	x1, [x29, 72]
	add	x0, x1, x0
	ldrh	w1, [x29, 92]
	mov	w2, w1
	ldr	x1, [x29, 24]
	bl	nand_memcpy
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3512]
	ldr	x2, [x29, 72]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 88]
	ldr	w0, [x29, 88]
	b	.L1299
.L1300:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 2]
	cmp	w0, 0
	bne	.L1301
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w0, w0
	ldrh	w1, [x29, 34]
	cmp	w1, w0
	bne	.L1301
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 26]
	cmp	w0, 0
	bne	.L1301
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 80]
	bl	search_from_cache_write_list
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L1302
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	del_from_cache_write_list
.L1302:
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 80]
	bl	search_from_cache_read_list
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L1303
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	del_from_cache_read_list
.L1303:
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3512]
	ldr	x2, [x29, 24]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 88]
	ldr	w0, [x29, 88]
	cmp	w0, 0
	beq	.L1304
	ldrh	w1, [x29, 34]
	adrp	x0, .LC337
	add	x0, x0, :lo12:.LC337
	bl	nand_print
	mov	w0, 1
	b	.L1299
.L1304:
	mov	w0, 0
	b	.L1299
.L1301:
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 80]
	bl	search_from_cache_write_list
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L1305
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	del_from_cache_write_list
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 16]
	str	x0, [x29, 72]
	ldr	x0, [x29, 64]
	ldrh	w0, [x0, 6]
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 62]
	ldr	x0, [x29, 64]
	ldrh	w1, [x0, 6]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w0, w0
	cmp	w1, w0
	beq	.L1306
	ldrh	w1, [x29, 94]
	ldrh	w0, [x29, 62]
	cmp	w1, w0
	bne	.L1307
	ldr	x0, [x29, 64]
	ldrh	w1, [x0, 6]
	ldrh	w0, [x29, 92]
	lsr	w0, w0, 9
	uxth	w0, w0
	add	w0, w1, w0
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 6]
	b	.L1306
.L1307:
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3520]
	ldr	x0, [x29, 80]
	ldr	x0, [x0, 1800]
	mov	x2, x0
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 88]
	ldrh	w0, [x29, 62]
	ldr	x1, [x29, 72]
	add	x3, x1, x0
	ldr	x0, [x29, 80]
	ldr	x1, [x0, 1800]
	ldrh	w0, [x29, 62]
	add	x1, x1, x0
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	ldrh	w0, [x29, 62]
	sub	w0, w2, w0
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 6]
	ldr	w0, [x29, 88]
	cmp	w0, 10
	bne	.L1306
	adrp	x0, .LC338
	add	x0, x0, :lo12:.LC338
	ldr	w1, [x29, 36]
	bl	nand_print
.L1306:
	ldrh	w0, [x29, 94]
	ldr	x1, [x29, 72]
	add	x0, x1, x0
	ldrh	w1, [x29, 92]
	mov	w2, w1
	ldr	x1, [x29, 24]
	bl	nand_memcpy
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	add_to_cache_write_list_tail
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 80]
	bl	search_from_cache_read_list
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L1308
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	del_from_cache_read_list
.L1308:
	mov	w0, 0
	b	.L1299
.L1305:
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 80]
	bl	search_from_cache_read_list
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	beq	.L1311
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 16]
	str	x0, [x29, 72]
	ldrh	w0, [x29, 94]
	ldr	x1, [x29, 72]
	add	x0, x1, x0
	ldrh	w1, [x29, 92]
	mov	w2, w1
	ldr	x1, [x29, 24]
	bl	nand_memcpy
	ldr	x0, [x29, 64]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 6]
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	del_from_cache_read_list
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	add_to_cache_write_list_tail
	ldr	x0, [x29, 80]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 24]
	cmp	w1, w0
	bcc	.L1310
	mov	w1, 0
	ldr	x0, [x29, 40]
	bl	flush_write_cache_to_nand
	cmp	w0, 0
	beq	.L1310
	adrp	x0, .LC339
	add	x0, x0, :lo12:.LC339
	bl	nand_print
	mov	w0, 1
	b	.L1299
.L1310:
	mov	w0, 0
	b	.L1299
.L1312:
	mov	w1, 0
	ldr	x0, [x29, 40]
	bl	flush_write_cache_to_nand
	cmp	w0, 0
	beq	.L1311
	adrp	x0, .LC340
	add	x0, x0, :lo12:.LC340
	bl	nand_print
	mov	w0, 1
	b	.L1299
.L1311:
	ldr	x0, [x29, 80]
	ldrh	w1, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0, 24]
	cmp	w1, w0
	bcs	.L1312
	ldr	x0, [x29, 80]
	bl	get_empty_cahce_node
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	bne	.L1313
	ldr	x0, [x29, 80]
	ldrh	w0, [x0, 4]
	mov	w1, w0
	ldr	x0, [x29, 80]
	ldrh	w0, [x0, 2]
	mov	w2, w0
	ldr	x0, [x29, 80]
	ldrh	w0, [x0, 6]
	mov	w3, w0
	adrp	x0, .LC336
	add	x0, x0, :lo12:.LC336
	bl	nand_print
	mov	w0, 1
	b	.L1299
.L1313:
	ldr	x0, [x29, 64]
	ldr	w1, [x29, 36]
	str	w1, [x0, 8]
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 16]
	str	x0, [x29, 72]
	ldrh	w0, [x29, 94]
	cmp	w0, 0
	bne	.L1314
	ldr	x0, [x29, 64]
	strh	wzr, [x0, 4]
	ldrh	w0, [x29, 92]
	lsr	w0, w0, 9
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 6]
	b	.L1315
.L1314:
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 3520]
	ldr	x2, [x29, 72]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x3
	str	w0, [x29, 88]
	ldr	w0, [x29, 88]
	cmp	w0, 10
	bne	.L1316
	adrp	x0, .LC341
	add	x0, x0, :lo12:.LC341
	ldr	w1, [x29, 36]
	bl	nand_print
.L1316:
	ldr	x0, [x29, 64]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldr	x0, [x29, 64]
	strh	w1, [x0, 6]
.L1315:
	ldrh	w0, [x29, 94]
	ldr	x1, [x29, 72]
	add	x0, x1, x0
	ldrh	w1, [x29, 92]
	mov	w2, w1
	ldr	x1, [x29, 24]
	bl	nand_memcpy
	ldr	x1, [x29, 64]
	ldr	x0, [x29, 80]
	bl	add_to_cache_write_list_tail
	mov	w0, 0
.L1299:
	ldp	x29, x30, [sp], 96
	ret
	.size	nand_cache_write, .-nand_cache_write
	.section	.rodata
	.align	3
.LC342:
	.string	"[NE]flush write cache to nand no cache!\n"
	.align	3
.LC343:
	.string	"[NE]ECC_LIMIT happened 10! page:%d cache: %d!\n"
	.align	3
.LC344:
	.string	"[NE]flush write cache to nand error!\n"
	.text
	.align	2
	.global	flush_write_cache_to_nand
	.type	flush_write_cache_to_nand, %function
flush_write_cache_to_nand:
	stp	x29, x30, [sp, -112]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	xzr, [x29, 96]
	mov	w0, 1
	strb	w0, [x29, 111]
	mov	w0, 44
	strb	w0, [x29, 48]
	mov	w0, 100
	strb	w0, [x29, 49]
	mov	w0, 68
	strb	w0, [x29, 50]
	mov	w0, 50
	strb	w0, [x29, 51]
	mov	w0, -91
	strb	w0, [x29, 52]
	mov	w0, -1
	strb	w0, [x29, 53]
	mov	w0, -1
	strb	w0, [x29, 54]
	mov	w0, -1
	strb	w0, [x29, 55]
	mov	w0, 44
	strb	w0, [x29, 40]
	mov	w0, -124
	strb	w0, [x29, 41]
	mov	w0, 68
	strb	w0, [x29, 42]
	mov	w0, 50
	strb	w0, [x29, 43]
	mov	w0, -86
	strb	w0, [x29, 44]
	mov	w0, 4
	strb	w0, [x29, 45]
	strb	wzr, [x29, 46]
	strb	wzr, [x29, 47]
	str	wzr, [x29, 92]
	ldr	x0, [x29, 24]
	add	x0, x0, 1072
	str	x0, [x29, 80]
	ldr	x0, [x29, 80]
	ldr	x0, [x0, 1784]
	cmp	x0, 0
	bne	.L1318
	adrp	x0, .LC342
	add	x0, x0, :lo12:.LC342
	bl	nand_print
	mov	w0, 0
	b	.L1325
.L1318:
	ldr	x0, [x29, 80]
	ldr	x0, [x0, 1784]
	mov	x1, x0
	ldr	x0, [x29, 80]
	bl	del_from_cache_write_list
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	cmp	x0, 0
	bne	.L1320
	mov	w0, 0
	b	.L1325
.L1320:
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 16]
	str	x0, [x29, 64]
	ldr	x0, [x29, 72]
	ldrh	w1, [x0, 6]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w0, w0
	cmp	w1, w0
	beq	.L1321
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 3520]
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 80]
	ldr	x0, [x0, 1800]
	mov	x2, x0
	ldr	x0, [x29, 24]
	blr	x3
	str	w0, [x29, 92]
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 6]
	ubfiz	w0, w0, 9, 7
	strh	w0, [x29, 62]
	ldrh	w0, [x29, 62]
	ldr	x1, [x29, 64]
	add	x3, x1, x0
	ldr	x0, [x29, 80]
	ldr	x1, [x0, 1800]
	ldrh	w0, [x29, 62]
	add	x1, x1, x0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	ldrh	w0, [x29, 62]
	sub	w0, w2, w0
	mov	w2, w0
	mov	x0, x3
	bl	nand_memcpy
	ldr	w0, [x29, 92]
	cmp	w0, 10
	bne	.L1321
	ldr	x0, [x29, 72]
	ldr	w1, [x0, 8]
	ldr	x0, [x29, 80]
	ldrh	w0, [x0, 6]
	mov	w2, w0
	adrp	x0, .LC343
	add	x0, x0, :lo12:.LC343
	bl	nand_print
.L1321:
	ldr	x0, [x29, 72]
	strh	wzr, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldr	x0, [x29, 72]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 3512]
	ldr	x0, [x29, 72]
	ldr	w0, [x0, 8]
	ldr	x2, [x29, 64]
	mov	w1, w0
	ldr	x0, [x29, 24]
	blr	x3
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1322
	adrp	x0, .LC344
	add	x0, x0, :lo12:.LC344
	bl	nand_print
.L1322:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	str	x0, [x29, 96]
	add	x0, x29, 48
	mov	x2, 8
	mov	x1, x0
	ldr	x0, [x29, 96]
	bl	nand_memcmp
	strb	w0, [x29, 111]
	ldrb	w0, [x29, 111]
	cmp	w0, 0
	beq	.L1323
	add	x0, x29, 40
	mov	x2, 8
	mov	x1, x0
	ldr	x0, [x29, 96]
	bl	nand_memcmp
	strb	w0, [x29, 111]
.L1323:
	ldrb	w0, [x29, 111]
	cmp	w0, 0
	bne	.L1324
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 584]
	cmp	w0, 0
	beq	.L1324
	ldr	x0, [x29, 24]
	ldrh	w0, [x0, 584]
	sub	w0, w0, #1
	bl	nand_phy_get_page_type
	cmp	w0, 2
	bne	.L1324
	ldr	x0, [x29, 24]
	ldr	x3, [x0, 3512]
	ldr	x0, [x29, 72]
	ldr	w0, [x0, 8]
	ldr	x2, [x29, 64]
	mov	w1, w0
	ldr	x0, [x29, 24]
	blr	x3
	str	w0, [x29, 92]
	ldr	w0, [x29, 92]
	cmp	w0, 0
	beq	.L1324
	adrp	x0, .LC344
	add	x0, x0, :lo12:.LC344
	bl	nand_print
.L1324:
	mov	w2, 2
	ldr	x1, [x29, 72]
	ldr	x0, [x29, 80]
	bl	add_to_cache_read_list_tail
	mov	w0, 0
.L1325:
	ldp	x29, x30, [sp], 112
	ret
	.size	flush_write_cache_to_nand, .-flush_write_cache_to_nand
	.section	.rodata
	.align	3
.LC345:
	.string	"[ND]flush cache %d %d!\n"
	.text
	.align	2
	.global	__nand_flush_write_cache
	.type	__nand_flush_write_cache, %function
__nand_flush_write_cache:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	wzr, [x29, 40]
	ldr	x0, [x29, 24]
	add	x0, x0, 1072
	str	x0, [x29, 32]
	str	wzr, [x29, 44]
	b	.L1327
.L1329:
	ldr	w0, [x29, 40]
	add	w0, w0, 1
	str	w0, [x29, 40]
	mov	w1, 1
	ldr	x0, [x29, 24]
	bl	flush_write_cache_to_nand
	mov	w1, w0
	ldr	w0, [x29, 44]
	orr	w0, w0, w1
	str	w0, [x29, 44]
	ldr	w0, [x29, 20]
	sub	w0, w0, #1
	str	w0, [x29, 20]
	ldr	w0, [x29, 20]
	cmp	w0, 0
	beq	.L1333
.L1327:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 1784]
	cmp	x0, 0
	bne	.L1329
	b	.L1328
.L1333:
	nop
.L1328:
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1330
	adrp	x0, .LC35
	add	x0, x0, :lo12:.LC35
	bl	nand_print
.L1330:
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1331
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 656]
	cmp	w0, 0
	beq	.L1331
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	adrp	x0, .LC345
	add	x0, x0, :lo12:.LC345
	ldr	w2, [x29, 40]
	bl	nand_print_dbg
.L1331:
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	__nand_flush_write_cache, .-__nand_flush_write_cache
	.section	.rodata
	.align	3
.LC346:
	.string	"[NE]nand_discard paramter error!\n"
	.text
	.align	2
	.global	nand_discard
	.type	nand_discard, %function
nand_discard:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	str	wzr, [x29, 56]
	ldr	x0, [x29, 24]
	add	x0, x0, 1072
	str	x0, [x29, 48]
	ldr	w1, [x29, 20]
	ldr	w0, [x29, 16]
	add	w1, w1, w0
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	cmp	w1, w0
	bls	.L1335
	adrp	x0, .LC346
	add	x0, x0, :lo12:.LC346
	bl	nand_print
	mov	w0, 1
	b	.L1336
.L1335:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 3440]
	ldr	x2, [x1, 96]
	ldr	w1, [x29, 16]
	add	x1, x2, x1
	str	x1, [x0, 96]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 20]
	udiv	w0, w0, w1
	str	w0, [x29, 60]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 20]
	udiv	w2, w0, w1
	mul	w1, w2, w1
	sub	w0, w0, w1
	strh	w0, [x29, 46]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w1, w0
	ldrh	w0, [x29, 46]
	sub	w0, w1, w0
	strh	w0, [x29, 44]
	ldrh	w0, [x29, 46]
	cmp	w0, 0
	beq	.L1337
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
	ldrh	w1, [x29, 44]
	ldr	w0, [x29, 16]
	cmp	w1, w0
	bls	.L1338
	str	wzr, [x29, 16]
	b	.L1340
.L1338:
	ldrh	w0, [x29, 44]
	ldr	w1, [x29, 16]
	sub	w0, w1, w0
	str	w0, [x29, 16]
	b	.L1340
.L1337:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 16]
	cmp	w1, w0
	bls	.L1340
	str	wzr, [x29, 16]
.L1340:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 16]
	udiv	w0, w0, w1
	str	w0, [x29, 16]
	ldr	w0, [x29, 16]
	cmp	w0, 0
	bne	.L1342
	mov	w0, 0
	b	.L1336
.L1345:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3448]
	ldrh	w0, [x0]
	cmp	w0, 0
	bne	.L1343
	ldr	w1, [x29, 60]
	ldr	x0, [x29, 48]
	bl	search_from_cache_write_list
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	beq	.L1344
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 48]
	bl	del_from_cache_write_list
.L1344:
	ldr	w1, [x29, 60]
	ldr	x0, [x29, 48]
	bl	search_from_cache_read_list
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	beq	.L1343
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 48]
	bl	del_from_cache_read_list
.L1343:
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 3528]
	ldr	w1, [x29, 60]
	ldr	x0, [x29, 24]
	blr	x2
	ldr	w0, [x29, 16]
	sub	w0, w0, #1
	str	w0, [x29, 16]
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 3440]
	ldr	x2, [x1, 104]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	uxtb	x1, w1
	add	x1, x2, x1
	str	x1, [x0, 104]
.L1342:
	ldr	w0, [x29, 16]
	cmp	w0, 0
	bne	.L1345
	ldr	w0, [x29, 56]
.L1336:
	ldp	x29, x30, [sp], 64
	ret
	.size	nand_discard, .-nand_discard
	.align	2
	.global	__shutdown_op
	.type	__shutdown_op, %function
__shutdown_op:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	wzr, [sp, 28]
	ldr	w0, [sp, 28]
	add	sp, sp, 32
	ret
	.size	__shutdown_op, .-__shutdown_op
	.section	.rodata
	.align	3
.LC347:
	.string	"[NI]page_map NULL!!\n"
	.align	3
.LC348:
	.string	"[ND]%4d:<%4d %3d><%d>\n"
	.text
	.align	2
	.global	print_logic_page_map
	.type	print_logic_page_map, %function
print_logic_page_map:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 24]
	ldr	x1, [x1, 8]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 40]
	str	wzr, [x29, 44]
	b	.L1349
.L1352:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 24]
	bl	get_logic_page_map
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L1350
	adrp	x0, .LC347
	add	x0, x0, :lo12:.LC347
	bl	nand_print
	b	.L1351
.L1350:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0, 2]
	mov	w0, 65535
	cmp	w1, w0
	beq	.L1351
	ldr	x0, [x29, 32]
	ldrh	w0, [x0, 2]
	mov	w1, w0
	ldr	x0, [x29, 32]
	ldrh	w0, [x0]
	ubfx	x0, x0, 0, 12
	uxth	w0, w0
	mov	w2, w0
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 1]
	ubfx	x0, x0, 4, 1
	uxtb	w0, w0
	mov	w3, w0
	adrp	x0, .LC348
	add	x0, x0, :lo12:.LC348
	mov	w4, w3
	mov	w3, w2
	mov	w2, w1
	ldr	w1, [x29, 44]
	bl	nand_print_dbg
.L1351:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1349:
	ldr	w1, [x29, 44]
	ldr	w0, [x29, 40]
	cmp	w1, w0
	bcc	.L1352
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	print_logic_page_map, .-print_logic_page_map
	.align	2
	.global	get_last_nftl
	.type	get_last_nftl, %function
get_last_nftl:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	x0, [sp, 8]
	str	x0, [sp, 16]
	b	.L1355
.L1356:
	ldr	x0, [sp, 24]
	str	x0, [sp, 16]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L1355:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L1356
	ldr	x0, [sp, 16]
	add	sp, sp, 32
	ret
	.size	get_last_nftl, .-get_last_nftl
	.align	2
	.global	get_nftl_need_read_claim
	.type	get_nftl_need_read_claim, %function
get_nftl_need_read_claim:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	x0, [x29, 56]
	str	wzr, [x29, 52]
	ldr	x0, [x29, 24]
	bl	get_last_nftl
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 172]
	str	w0, [x29, 52]
	ldr	x0, [x29, 24]
	str	x0, [x29, 56]
	b	.L1359
.L1362:
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 24]
	ldrb	w0, [x0, 24]
	mov	w1, w0
	ldr	w0, [x29, 52]
	cmp	w1, w0
	beq	.L1364
	ldr	x0, [x29, 56]
	ldr	x0, [x0, 16]
	str	x0, [x29, 56]
.L1359:
	ldr	x0, [x29, 56]
	cmp	x0, 0
	bne	.L1362
	b	.L1361
.L1364:
	nop
.L1361:
	ldr	x0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	get_nftl_need_read_claim, .-get_nftl_need_read_claim
	.align	2
	.global	get_nftl_read_claim_page_no
	.type	get_nftl_read_claim_page_no, %function
get_nftl_read_claim_page_no:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	get_last_nftl
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	ldr	w0, [x0, 176]
	ldp	x29, x30, [sp], 48
	ret
	.size	get_nftl_read_claim_page_no, .-get_nftl_read_claim_page_no
	.align	2
	.global	clean_read_reclaim_complete_flag
	.type	clean_read_reclaim_complete_flag, %function
clean_read_reclaim_complete_flag:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	b	.L1368
.L1369:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 24]
	ldr	x1, [sp, 24]
	ldr	x1, [x1, 24]
	ldr	w1, [x1, 664]
	cmp	w1, 0
	cset	w1, eq
	uxtb	w1, w1
	str	w1, [x0, 664]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L1368:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L1369
	nop
	add	sp, sp, 32
	ret
	.size	clean_read_reclaim_complete_flag, .-clean_read_reclaim_complete_flag
	.align	2
	.global	set_nftl_read_claim_complete
	.type	set_nftl_read_claim_complete, %function
set_nftl_read_claim_complete:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 24]
	mov	w1, 1
	str	w1, [x0, 660]
	ldr	x0, [x29, 24]
	bl	get_last_nftl
	str	x0, [x29, 40]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L1371
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	str	wzr, [x0, 172]
	ldr	x0, [x29, 24]
	bl	clean_read_reclaim_complete_flag
	b	.L1372
.L1371:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x29, 16]
	ldr	x1, [x1, 24]
	ldrb	w1, [x1, 24]
	add	w1, w1, 1
	str	w1, [x0, 172]
.L1372:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	str	wzr, [x0, 176]
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	set_nftl_read_claim_complete, .-set_nftl_read_claim_complete
	.align	2
	.global	set_nftl_read_claim_flag
	.type	set_nftl_read_claim_flag, %function
set_nftl_read_claim_flag:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	str	w2, [sp]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	b	.L1375
.L1376:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [sp, 4]
	str	w1, [x0, 172]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [sp]
	str	w1, [x0, 176]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L1375:
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L1376
	mov	w0, 0
	add	sp, sp, 32
	ret
	.size	set_nftl_read_claim_flag, .-set_nftl_read_claim_flag
	.align	2
	.global	get_cold_logic_page
	.type	get_cold_logic_page, %function
get_cold_logic_page:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	ldr	w0, [x29, 20]
	str	w0, [x29, 44]
	b	.L1379
.L1383:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 24]
	bl	get_logic_page_map
	str	x0, [x29, 32]
	ldr	x0, [x29, 32]
	cmp	x0, 0
	bne	.L1380
	adrp	x0, .LC347
	add	x0, x0, :lo12:.LC347
	bl	nand_print
	b	.L1381
.L1380:
	ldr	x0, [x29, 32]
	ldrh	w1, [x0, 2]
	mov	w0, 65535
	cmp	w1, w0
	beq	.L1381
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 1]
	ubfx	x0, x0, 4, 1
	uxtb	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 664]
	cmp	w1, w0
	beq	.L1381
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 664]
	uxtb	w0, w0
	and	w0, w0, 1
	uxtb	w2, w0
	ldr	x1, [x29, 32]
	ldrb	w0, [x1, 1]
	bfi	w0, w2, 4, 1
	strb	w0, [x1, 1]
	ldr	w0, [x29, 44]
	b	.L1382
.L1381:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1379:
	ldr	w1, [x29, 44]
	ldr	w0, [x29, 16]
	cmp	w1, w0
	bcc	.L1383
	mov	w0, -1
.L1382:
	ldp	x29, x30, [sp], 48
	ret
	.size	get_cold_logic_page, .-get_cold_logic_page
	.align	2
	.global	read_reclaim
	.type	read_reclaim, %function
read_reclaim:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 40]
	bl	get_nftl_read_claim_page_no
	str	w0, [x29, 56]
	ldr	x0, [x29, 32]
	ldr	x3, [x0, 24]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	ldr	w0, [x0, 36]
	mov	w2, w0
	ldr	w1, [x29, 56]
	mov	x0, x3
	bl	get_cold_logic_page
	str	w0, [x29, 52]
	ldr	w0, [x29, 52]
	cmn	w0, #1
	bne	.L1385
	ldr	x1, [x29, 32]
	ldr	x0, [x29, 40]
	bl	set_nftl_read_claim_complete
	str	w0, [x29, 60]
	b	.L1386
.L1385:
	ldr	x0, [x29, 32]
	ldr	x4, [x0, 24]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 8]
	uxth	w0, w0
	ldr	x3, [x29, 24]
	mov	w2, w0
	ldr	w1, [x29, 52]
	mov	x0, x4
	bl	nand_cache_read
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 24]
	ldrb	w0, [x0, 24]
	ldr	w2, [x29, 52]
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_nftl_read_claim_flag
.L1386:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	read_reclaim, .-read_reclaim
	.global	hynix26nm_read_retry_mode
	.bss
	.type	hynix26nm_read_retry_mode, %object
	.size	hynix26nm_read_retry_mode, 1
hynix26nm_read_retry_mode:
	.zero	1
	.global	hynix26nm_read_retry_cycle
	.type	hynix26nm_read_retry_cycle, %object
	.size	hynix26nm_read_retry_cycle, 1
hynix26nm_read_retry_cycle:
	.zero	1
	.global	hynix26nm_read_retry_reg_num
	.type	hynix26nm_read_retry_reg_num, %object
	.size	hynix26nm_read_retry_reg_num, 1
hynix26nm_read_retry_reg_num:
	.zero	1
	.global	hynix26nm_read_retry_reg_adr
	.align	3
	.type	hynix26nm_read_retry_reg_adr, %object
	.size	hynix26nm_read_retry_reg_adr, 4
hynix26nm_read_retry_reg_adr:
	.zero	4
	.global	hynix26nm_read_retry_val
	.align	3
	.type	hynix26nm_read_retry_val, %object
	.size	hynix26nm_read_retry_val, 56
hynix26nm_read_retry_val:
	.zero	56
	.global	hynix26nm_lsb_mode_reg_adr
	.align	3
	.type	hynix26nm_lsb_mode_reg_adr, %object
	.size	hynix26nm_lsb_mode_reg_adr, 5
hynix26nm_lsb_mode_reg_adr:
	.zero	5
	.global	hynix26nm_lsb_mode_default_val
	.align	3
	.type	hynix26nm_lsb_mode_default_val, %object
	.size	hynix26nm_lsb_mode_default_val, 5
hynix26nm_lsb_mode_default_val:
	.zero	5
	.global	hynix26nm_lsb_mode_val
	.align	3
	.type	hynix26nm_lsb_mode_val, %object
	.size	hynix26nm_lsb_mode_val, 5
hynix26nm_lsb_mode_val:
	.zero	5
	.global	hynix26nm_lsb_mode_reg_num
	.type	hynix26nm_lsb_mode_reg_num, %object
	.size	hynix26nm_lsb_mode_reg_num, 1
hynix26nm_lsb_mode_reg_num:
	.zero	1
	.global	para0
	.section	.rodata
	.align	3
	.type	para0, %object
	.size	para0, 56
para0:
	.hword	0
	.hword	0
	.hword	0
	.hword	0
	.hword	0
	.hword	6
	.hword	10
	.hword	6
	.hword	0
	.hword	-3
	.hword	-7
	.hword	-8
	.hword	0
	.hword	-6
	.hword	-13
	.hword	-15
	.hword	0
	.hword	-11
	.hword	-20
	.hword	-23
	.hword	0
	.hword	0
	.hword	-26
	.hword	-30
	.hword	0
	.hword	0
	.hword	-32
	.hword	-37
	.global	para1
	.align	3
	.type	para1, %object
	.size	para1, 56
para1:
	.hword	0
	.hword	0
	.hword	0
	.hword	0
	.hword	0
	.hword	6
	.hword	10
	.hword	6
	.hword	0
	.hword	-3
	.hword	-7
	.hword	-8
	.hword	0
	.hword	-6
	.hword	-13
	.hword	-15
	.hword	0
	.hword	-9
	.hword	-20
	.hword	-23
	.hword	0
	.hword	0
	.hword	-26
	.hword	-30
	.hword	0
	.hword	0
	.hword	-32
	.hword	-37
	.align	3
.LC349:
	.string	"rr value %x %x %x %x!\n"
	.text
	.align	2
	.global	hynix26nm_vender_set_param
	.type	hynix26nm_vender_set_param, %function
hynix26nm_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 56]
	mov	w0, 54
	strb	w0, [x29, 54]
	mov	w0, 22
	strb	w0, [x29, 55]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	str	wzr, [x29, 60]
	b	.L1389
.L1390:
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 24]
	add	x2, x1, x0
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 32]
	add	x1, x1, x0
	add	x0, x29, 54
	mov	w6, 1
	mov	w5, 1
	mov	x4, x1
	mov	x3, x2
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w2, 0
	ldrb	w1, [x29, 55]
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1389:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L1390
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC349
	add	x0, x0, :lo12:.LC349
	bl	nand_print_dbg
	ldr	w0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix26nm_vender_set_param, .-hynix26nm_vender_set_param
	.section	.rodata
	.align	3
.LC350:
	.string	"hynix26nm retry param:"
	.align	3
.LC351:
	.string	" %x "
	.text
	.align	2
	.global	hynix26nm_set_readretry
	.type	hynix26nm_set_readretry, %function
hynix26nm_set_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 52]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	adrp	x1, hynix26nm_read_retry_cycle
	add	x1, x1, :lo12:hynix26nm_read_retry_cycle
	ldrb	w1, [x1]
	cmp	w0, w1
	bls	.L1393
	mov	w0, -53
	b	.L1405
.L1393:
	str	wzr, [x29, 60]
	b	.L1395
.L1399:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 60]
	add	x0, x1, x0
	ldrb	w0, [x0, 56]
	strh	w0, [x29, 50]
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 184]
	adrp	x0, hynix26nm_read_retry_val
	add	x0, x0, :lo12:hynix26nm_read_retry_val
	ldr	w1, [x29, 60]
	uxtw	x2, w2
	lsl	x2, x2, 2
	add	x1, x2, x1
	ldrsh	w0, [x0, x1, lsl 1]
	uxth	w1, w0
	ldrh	w0, [x29, 50]
	add	w0, w1, w0
	uxth	w0, w0
	strh	w0, [x29, 58]
	ldrsh	w0, [x29, 58]
	cmp	w0, 255
	ble	.L1396
	mov	w0, 255
	strh	w0, [x29, 58]
	b	.L1397
.L1396:
	ldrsh	w0, [x29, 58]
	cmp	w0, 0
	bge	.L1398
	strh	wzr, [x29, 58]
	b	.L1397
.L1398:
	ldrh	w0, [x29, 58]
	and	w0, w0, 255
	strh	w0, [x29, 58]
.L1397:
	ldrh	w0, [x29, 58]
	uxtb	w2, w0
	ldr	w0, [x29, 60]
	sub	x1, x29, #4032
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4072]
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1395:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 60]
	cmp	w1, w0
	bhi	.L1399
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 1
	bls	.L1400
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 6
	bhi	.L1400
	strb	wzr, [x29, 40]
.L1400:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 5
	beq	.L1401
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 6
	bne	.L1402
.L1401:
	strb	wzr, [x29, 41]
.L1402:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x1, x0, :lo12:hynix26nm_read_retry_reg_adr
	add	x0, x29, 40
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_vender_set_param
	str	w0, [x29, 52]
	adrp	x0, .LC350
	add	x0, x0, :lo12:.LC350
	bl	nand_print_dbg
	str	wzr, [x29, 60]
	b	.L1403
.L1404:
	ldr	w0, [x29, 60]
	sub	x1, x29, #4032
	add	x0, x1, x0
	ldrb	w0, [x0, 4072]
	mov	w1, w0
	adrp	x0, .LC351
	add	x0, x0, :lo12:.LC351
	bl	nand_print_dbg
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1403:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 60]
	cmp	w1, w0
	bhi	.L1404
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
	ldr	w0, [x29, 52]
.L1405:
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix26nm_set_readretry, .-hynix26nm_set_readretry
	.section	.rodata
	.align	3
.LC352:
	.string	"m0 retry!\n"
	.align	3
.LC353:
	.string	"hynix26nm ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	hynix26nm_read_page_end
	.type	hynix26nm_read_page_end, %function
hynix26nm_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1407
	adrp	x0, .LC352
	add	x0, x0, :lo12:.LC352
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1408
.L1414:
	str	wzr, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 184]
	add	w1, w0, 1
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 184]
	adrp	x1, hynix26nm_read_retry_cycle
	add	x1, x1, :lo12:hynix26nm_read_retry_cycle
	ldrb	w1, [x1]
	cmp	w0, w1
	bls	.L1409
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
.L1409:
	ldr	x0, [x29, 32]
	bl	hynix26nm_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1410
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	hynix26nm_set_readretry
	b	.L1411
.L1410:
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1412
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1413
.L1412:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC353
	add	x0, x0, :lo12:.LC353
	bl	nand_print_dbg
	b	.L1411
.L1413:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1408:
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1414
.L1411:
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	hynix26nm_set_readretry
.L1407:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_read_page_end, .-hynix26nm_read_page_end
	.align	2
	.global	hynix26nm_vender_get_param
	.type	hynix26nm_vender_get_param, %function
hynix26nm_vender_get_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 56]
	mov	w0, 55
	strb	w0, [x29, 55]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	str	wzr, [x29, 60]
	b	.L1417
.L1418:
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 24]
	add	x2, x1, x0
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 32]
	add	x1, x1, x0
	add	x0, x29, 55
	mov	w4, 1
	mov	x3, x1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	get_data_with_nand_bus_one_cmd
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1417:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L1418
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	ldr	w0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix26nm_vender_get_param, .-hynix26nm_vender_get_param
	.align	2
	.global	hynix26nm_lsb_init
	.type	hynix26nm_lsb_init, %function
hynix26nm_lsb_init:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	strb	w1, [x0]
	adrp	x0, hynix26nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_num
	mov	w1, 5
	strb	w1, [x0]
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	mov	w1, -92
	strb	w1, [x0]
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	mov	w1, -91
	strb	w1, [x0, 1]
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	mov	w1, -80
	strb	w1, [x0, 2]
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	mov	w1, -79
	strb	w1, [x0, 3]
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	mov	w1, -55
	strb	w1, [x0, 4]
	adrp	x0, hynix26nm_lsb_mode_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_val
	mov	w1, 37
	strb	w1, [x0]
	adrp	x0, hynix26nm_lsb_mode_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_val
	mov	w1, 37
	strb	w1, [x0, 1]
	adrp	x0, hynix26nm_lsb_mode_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_val
	mov	w1, 37
	strb	w1, [x0, 2]
	adrp	x0, hynix26nm_lsb_mode_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_val
	mov	w1, 37
	strb	w1, [x0, 3]
	adrp	x0, hynix26nm_lsb_mode_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_val
	mov	w1, 1
	strb	w1, [x0, 4]
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	hynix26nm_lsb_init, .-hynix26nm_lsb_init
	.align	2
	.global	hynix26nm_lsb_enable
	.type	hynix26nm_lsb_enable, %function
hynix26nm_lsb_enable:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, hynix26nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x1, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	adrp	x0, hynix26nm_lsb_mode_default_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_default_val
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_vender_get_param
	str	wzr, [x29, 44]
	b	.L1423
.L1424:
	adrp	x0, hynix26nm_lsb_mode_default_val
	add	x1, x0, :lo12:hynix26nm_lsb_mode_default_val
	ldr	w0, [x29, 44]
	ldrb	w2, [x1, x0]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	ldrb	w1, [x0, 4080]
	adrp	x0, hynix26nm_lsb_mode_val
	add	x2, x0, :lo12:hynix26nm_lsb_mode_val
	ldr	w0, [x29, 44]
	ldrb	w0, [x2, x0]
	add	w0, w1, w0
	uxtb	w2, w0
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1423:
	adrp	x0, hynix26nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1424
	adrp	x0, hynix26nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x1, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	add	x0, x29, 32
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_vender_set_param
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_lsb_enable, .-hynix26nm_lsb_enable
	.align	2
	.global	hynix26nm_lsb_disable
	.type	hynix26nm_lsb_disable, %function
hynix26nm_lsb_disable:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, hynix26nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix26nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_lsb_mode_reg_adr
	add	x1, x0, :lo12:hynix26nm_lsb_mode_reg_adr
	adrp	x0, hynix26nm_lsb_mode_default_val
	add	x0, x0, :lo12:hynix26nm_lsb_mode_default_val
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_vender_set_param
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix26nm_lsb_disable, .-hynix26nm_lsb_disable
	.section	.rodata
	.align	3
.LC354:
	.string	"hynix26nm_lsb_exit\n"
	.text
	.align	2
	.global	hynix26nm_lsb_exit
	.type	hynix26nm_lsb_exit, %function
hynix26nm_lsb_exit:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w0, 1048575
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 0
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w2, 1
	mov	w1, 48
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	nop
.L1429:
	ldr	w0, [x29, 44]
	sub	w1, w0, #1
	str	w1, [x29, 44]
	cmp	w0, 0
	bgt	.L1429
	adrp	x0, .LC354
	add	x0, x0, :lo12:.LC354
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_lsb_exit, .-hynix26nm_lsb_exit
	.align	2
	.global	hynix26nm_getdefaultparam
	.type	hynix26nm_getdefaultparam, %function
hynix26nm_getdefaultparam:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	str	wzr, [x29, 40]
	ldr	x0, [x29, 24]
	add	x1, x0, 56
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w3, w2
	mov	x2, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_vender_get_param
	str	w0, [x29, 40]
	str	wzr, [x29, 44]
	b	.L1432
.L1433:
	ldrsw	x0, [x29, 44]
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldr	x2, [x29, 24]
	ldrsw	x1, [x29, 44]
	add	x1, x2, x1
	ldrb	w1, [x1, 56]
	strb	w1, [x0]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1432:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1433
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_getdefaultparam, .-hynix26nm_getdefaultparam
	.section	.rodata
	.align	3
.LC355:
	.string	"set retry default value: "
	.align	3
.LC356:
	.string	" %x"
	.text
	.align	2
	.global	hynix26nm_setdefaultparam
	.type	hynix26nm_setdefaultparam, %function
hynix26nm_setdefaultparam:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 40]
	str	wzr, [x29, 44]
	b	.L1436
.L1437:
	ldr	x1, [x29, 24]
	ldrsw	x0, [x29, 44]
	add	x0, x1, x0
	ldrb	w2, [x0, 56]
	ldrsw	x0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1436:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1437
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x1, x0, :lo12:hynix26nm_read_retry_reg_adr
	add	x0, x29, 32
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_vender_set_param
	str	w0, [x29, 40]
	adrp	x0, .LC355
	add	x0, x0, :lo12:.LC355
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1438
.L1439:
	ldrsw	x0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	ldrb	w0, [x0, 4080]
	mov	w1, w0
	adrp	x0, .LC356
	add	x0, x0, :lo12:.LC356
	bl	nand_print_dbg
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1438:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1439
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_setdefaultparam, .-hynix26nm_setdefaultparam
	.section	.rodata
	.align	3
.LC357:
	.string	"hynix26nm_get_default_param: ch: %d, chip: %d, value: 0x%x 0x%x 0x%x 0x%x \n"
	.text
	.align	2
	.global	hynix26nm_get_default_param
	.type	hynix26nm_get_default_param, %function
hynix26nm_get_default_param:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	add	x0, x29, 40
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix26nm_getdefaultparam
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 28]
	ldrb	w0, [x29, 40]
	mov	w3, w0
	ldrb	w0, [x29, 41]
	mov	w4, w0
	ldrb	w0, [x29, 42]
	mov	w5, w0
	ldrb	w0, [x29, 43]
	mov	w6, w0
	adrp	x0, .LC357
	add	x0, x0, :lo12:.LC357
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	hynix26nm_setdefaultparam
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_get_default_param, .-hynix26nm_get_default_param
	.section	.rodata
	.align	3
.LC358:
	.string	"NFC_ReadRetryInit, unknown read retry mode 0x%x\n"
	.text
	.align	2
	.global	hynix26nm_readretry_init
	.type	hynix26nm_readretry_init, %function
hynix26nm_readretry_init:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	strb	w1, [x0]
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1444
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -84
	strb	w1, [x0]
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -83
	strb	w1, [x0, 1]
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -82
	strb	w1, [x0, 2]
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -81
	strb	w1, [x0, 3]
	str	wzr, [x29, 44]
	b	.L1445
.L1448:
	str	wzr, [x29, 40]
	b	.L1446
.L1447:
	adrp	x0, para0
	add	x0, x0, :lo12:para0
	ldr	w1, [x29, 40]
	ldr	w2, [x29, 44]
	lsl	x2, x2, 2
	add	x1, x2, x1
	ldrsh	w3, [x0, x1, lsl 1]
	adrp	x0, hynix26nm_read_retry_val
	add	x0, x0, :lo12:hynix26nm_read_retry_val
	ldr	w1, [x29, 40]
	ldr	w2, [x29, 44]
	lsl	x2, x2, 2
	add	x1, x2, x1
	mov	w2, w3
	strh	w2, [x0, x1, lsl 1]
	ldr	w0, [x29, 40]
	add	w0, w0, 1
	str	w0, [x29, 40]
.L1446:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 40]
	cmp	w1, w0
	bhi	.L1447
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1445:
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	ldrb	w0, [x0]
	add	w0, w0, 1
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1448
	b	.L1449
.L1444:
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 1
	bne	.L1450
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -89
	strb	w1, [x0]
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -83
	strb	w1, [x0, 1]
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -82
	strb	w1, [x0, 2]
	adrp	x0, hynix26nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_adr
	mov	w1, -81
	strb	w1, [x0, 3]
	str	wzr, [x29, 44]
	b	.L1451
.L1454:
	str	wzr, [x29, 40]
	b	.L1452
.L1453:
	adrp	x0, para1
	add	x0, x0, :lo12:para1
	ldr	w1, [x29, 40]
	ldr	w2, [x29, 44]
	lsl	x2, x2, 2
	add	x1, x2, x1
	ldrsh	w3, [x0, x1, lsl 1]
	adrp	x0, hynix26nm_read_retry_val
	add	x0, x0, :lo12:hynix26nm_read_retry_val
	ldr	w1, [x29, 40]
	ldr	w2, [x29, 44]
	lsl	x2, x2, 2
	add	x1, x2, x1
	mov	w2, w3
	strh	w2, [x0, x1, lsl 1]
	ldr	w0, [x29, 40]
	add	w0, w0, 1
	str	w0, [x29, 40]
.L1452:
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 40]
	cmp	w1, w0
	bhi	.L1453
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1451:
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	ldrb	w0, [x0]
	add	w0, w0, 1
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1454
	b	.L1449
.L1450:
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC358
	add	x0, x0, :lo12:.LC358
	bl	nand_print
	mov	w0, -54
	b	.L1455
.L1449:
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	str	wzr, [x29, 44]
	b	.L1456
.L1457:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x1, x0
	strb	wzr, [x0, 56]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1456:
	ldr	w0, [x29, 44]
	cmp	w0, 127
	bls	.L1457
	ldr	x0, [x29, 24]
	bl	hynix26nm_get_default_param
	mov	w0, 0
.L1455:
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix26nm_readretry_init, .-hynix26nm_readretry_init
	.section	.rodata
	.align	3
.LC359:
	.string	"hynix26nm_readretry_exit. \n"
	.text
	.align	2
	.global	hynix26nm_readretry_exit
	.type	hynix26nm_readretry_exit, %function
hynix26nm_readretry_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, .LC359
	add	x0, x0, :lo12:.LC359
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	hynix26nm_setdefaultparam
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix26nm_readretry_exit, .-hynix26nm_readretry_exit
	.section	.rodata
	.align	3
.LC360:
	.string	" hynix26nm_special_init hynix26nm_read_retry_mode: %d hynix26nm_read_retry_cycle:%d hynix26nm_read_retry_reg_num:%d\n"
	.align	3
.LC361:
	.string	" hynix26nm_special_init error hynix26nm_read_retry_mode: %d hynix26nm_read_retry_cycle:%d hynix26nm_read_retry_reg_num:%d\n"
	.text
	.align	2
	.global	hynix26nm_special_init
	.type	hynix26nm_special_init, %function
hynix26nm_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1461
.L1462:
	ldr	x0, [x29, 16]
	bl	hynix26nm_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1461:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1462
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1463
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, hynix26nm_read_page_end
	add	x1, x1, :lo12:hynix26nm_read_page_end
	str	x1, [x0]
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC360
	add	x0, x0, :lo12:.LC360
	bl	nand_print_dbg
	b	.L1464
.L1463:
	adrp	x0, hynix26nm_read_retry_mode
	add	x0, x0, :lo12:hynix26nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, hynix26nm_read_retry_cycle
	add	x0, x0, :lo12:hynix26nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix26nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix26nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC361
	add	x0, x0, :lo12:.LC361
	bl	nand_print
.L1464:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix26nm_special_init, .-hynix26nm_special_init
	.section	.rodata
	.align	3
.LC362:
	.string	" hynix26nm_special_exit \n"
	.text
	.align	2
	.global	hynix26nm_special_exit
	.type	hynix26nm_special_exit, %function
hynix26nm_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1467
.L1468:
	ldr	x0, [x29, 24]
	bl	hynix26nm_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1467:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1468
	adrp	x0, .LC362
	add	x0, x0, :lo12:.LC362
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix26nm_special_exit, .-hynix26nm_special_exit
	.global	sandisk_A19_read_retry_mode
	.bss
	.type	sandisk_A19_read_retry_mode, %object
	.size	sandisk_A19_read_retry_mode, 1
sandisk_A19_read_retry_mode:
	.zero	1
	.global	sandisk_A19_read_retry_cycle
	.type	sandisk_A19_read_retry_cycle, %object
	.size	sandisk_A19_read_retry_cycle, 1
sandisk_A19_read_retry_cycle:
	.zero	1
	.global	sandisk_A19_read_retry_reg_cnt
	.type	sandisk_A19_read_retry_reg_cnt, %object
	.size	sandisk_A19_read_retry_reg_cnt, 1
sandisk_A19_read_retry_reg_cnt:
	.zero	1
	.global	sandisk_A19_read_retry_enable_cmd_1y
	.data
	.align	3
	.type	sandisk_A19_read_retry_enable_cmd_1y, %object
	.size	sandisk_A19_read_retry_enable_cmd_1y, 1
sandisk_A19_read_retry_enable_cmd_1y:
	.byte	93
	.global	sandisk_A19_read_retry_exit_cmd_1y
	.align	3
	.type	sandisk_A19_read_retry_exit_cmd_1y, %object
	.size	sandisk_A19_read_retry_exit_cmd_1y, 1
sandisk_A19_read_retry_exit_cmd_1y:
	.byte	-1
	.global	sandisk_A19_read_retry_cmd_1y
	.align	3
	.type	sandisk_A19_read_retry_cmd_1y, %object
	.size	sandisk_A19_read_retry_cmd_1y, 1
sandisk_A19_read_retry_cmd_1y:
	.byte	-17
	.global	sandisk_A19_read_retry_reg_adr_1y
	.align	3
	.type	sandisk_A19_read_retry_reg_adr_1y, %object
	.size	sandisk_A19_read_retry_reg_adr_1y, 1
sandisk_A19_read_retry_reg_adr_1y:
	.byte	17
	.global	sandisk_A19_1y
	.align	3
	.type	sandisk_A19_1y, %object
	.size	sandisk_A19_1y, 124
sandisk_A19_1y:
	.byte	124
	.byte	0
	.byte	0
	.byte	124
	.byte	4
	.byte	0
	.byte	124
	.byte	120
	.byte	120
	.byte	0
	.byte	120
	.byte	116
	.byte	8
	.byte	124
	.byte	0
	.byte	124
	.byte	0
	.byte	124
	.byte	124
	.byte	120
	.byte	124
	.byte	124
	.byte	120
	.byte	116
	.byte	0
	.byte	124
	.byte	116
	.byte	112
	.byte	0
	.byte	120
	.byte	0
	.byte	124
	.byte	0
	.byte	120
	.byte	124
	.byte	120
	.byte	0
	.byte	120
	.byte	120
	.byte	116
	.byte	0
	.byte	120
	.byte	116
	.byte	112
	.byte	0
	.byte	120
	.byte	112
	.byte	108
	.byte	0
	.byte	4
	.byte	4
	.byte	0
	.byte	0
	.byte	4
	.byte	0
	.byte	124
	.byte	12
	.byte	4
	.byte	124
	.byte	120
	.byte	12
	.byte	4
	.byte	120
	.byte	116
	.byte	16
	.byte	8
	.byte	0
	.byte	124
	.byte	16
	.byte	8
	.byte	4
	.byte	0
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	116
	.byte	112
	.byte	120
	.byte	116
	.byte	112
	.byte	108
	.byte	120
	.byte	116
	.byte	108
	.byte	104
	.byte	120
	.byte	112
	.byte	120
	.byte	116
	.byte	120
	.byte	112
	.byte	116
	.byte	112
	.byte	120
	.byte	112
	.byte	108
	.byte	104
	.byte	120
	.byte	112
	.byte	112
	.byte	108
	.byte	120
	.byte	108
	.byte	112
	.byte	108
	.byte	120
	.byte	108
	.byte	108
	.byte	104
	.byte	120
	.byte	108
	.byte	104
	.byte	100
	.byte	116
	.byte	104
	.byte	108
	.byte	104
	.byte	116
	.byte	104
	.byte	104
	.byte	100
	.global	sandisk_A19_1z_16g
	.align	3
	.type	sandisk_A19_1z_16g, %object
	.size	sandisk_A19_1z_16g, 128
sandisk_A19_1z_16g:
	.byte	124
	.byte	0
	.byte	4
	.byte	0
	.byte	4
	.byte	0
	.byte	0
	.byte	124
	.byte	120
	.byte	0
	.byte	8
	.byte	4
	.byte	8
	.byte	0
	.byte	124
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	0
	.byte	0
	.byte	124
	.byte	0
	.byte	124
	.byte	120
	.byte	124
	.byte	8
	.byte	4
	.byte	4
	.byte	124
	.byte	116
	.byte	120
	.byte	4
	.byte	4
	.byte	4
	.byte	0
	.byte	8
	.byte	4
	.byte	0
	.byte	124
	.byte	0
	.byte	4
	.byte	8
	.byte	4
	.byte	12
	.byte	4
	.byte	124
	.byte	120
	.byte	124
	.byte	4
	.byte	12
	.byte	8
	.byte	120
	.byte	120
	.byte	4
	.byte	0
	.byte	124
	.byte	120
	.byte	0
	.byte	124
	.byte	116
	.byte	120
	.byte	112
	.byte	120
	.byte	8
	.byte	8
	.byte	4
	.byte	0
	.byte	4
	.byte	8
	.byte	8
	.byte	4
	.byte	12
	.byte	8
	.byte	12
	.byte	8
	.byte	0
	.byte	0
	.byte	12
	.byte	8
	.byte	4
	.byte	0
	.byte	120
	.byte	116
	.byte	124
	.byte	124
	.byte	120
	.byte	116
	.byte	120
	.byte	120
	.byte	120
	.byte	116
	.byte	8
	.byte	8
	.byte	0
	.byte	124
	.byte	116
	.byte	116
	.byte	0
	.byte	124
	.byte	120
	.byte	116
	.byte	124
	.byte	120
	.byte	112
	.byte	116
	.byte	116
	.byte	116
	.byte	124
	.byte	116
	.byte	112
	.byte	112
	.byte	12
	.byte	12
	.byte	8
	.byte	4
	.byte	16
	.byte	12
	.byte	12
	.byte	8
	.byte	112
	.byte	112
	.byte	112
	.byte	116
	.byte	116
	.byte	112
	.byte	112
	.byte	112
	.global	sandisk_A19_1z_8g
	.align	3
	.type	sandisk_A19_1z_8g, %object
	.size	sandisk_A19_1z_8g, 124
sandisk_A19_1z_8g:
	.byte	4
	.byte	4
	.byte	120
	.byte	120
	.byte	4
	.byte	4
	.byte	124
	.byte	116
	.byte	0
	.byte	0
	.byte	4
	.byte	120
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	0
	.byte	0
	.byte	0
	.byte	124
	.byte	0
	.byte	0
	.byte	0
	.byte	116
	.byte	8
	.byte	8
	.byte	4
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	120
	.byte	124
	.byte	124
	.byte	0
	.byte	124
	.byte	4
	.byte	4
	.byte	124
	.byte	112
	.byte	124
	.byte	124
	.byte	116
	.byte	116
	.byte	0
	.byte	0
	.byte	120
	.byte	112
	.byte	12
	.byte	12
	.byte	8
	.byte	120
	.byte	120
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	4
	.byte	8
	.byte	4
	.byte	120
	.byte	120
	.byte	8
	.byte	120
	.byte	124
	.byte	124
	.byte	120
	.byte	112
	.byte	120
	.byte	120
	.byte	112
	.byte	108
	.byte	0
	.byte	0
	.byte	116
	.byte	108
	.byte	8
	.byte	8
	.byte	0
	.byte	116
	.byte	124
	.byte	124
	.byte	120
	.byte	108
	.byte	0
	.byte	0
	.byte	4
	.byte	4
	.byte	116
	.byte	116
	.byte	116
	.byte	108
	.byte	120
	.byte	120
	.byte	124
	.byte	112
	.byte	12
	.byte	12
	.byte	0
	.byte	116
	.byte	4
	.byte	4
	.byte	12
	.byte	8
	.byte	120
	.byte	120
	.byte	124
	.byte	116
	.byte	120
	.byte	120
	.byte	112
	.byte	104
	.byte	8
	.byte	8
	.byte	0
	.byte	112
	.byte	16
	.byte	16
	.byte	12
	.byte	120
	.byte	0
	.byte	0
	.byte	12
	.byte	8
	.local	sandisk_A19_sclk0_bak
	.comm	sandisk_A19_sclk0_bak,4,4
	.local	sandisk_A19_sclk1_bak
	.comm	sandisk_A19_sclk1_bak,4,4
	.text
	.align	2
	.global	sandisk_A19_dsp_on
	.type	sandisk_A19_dsp_on, %function
sandisk_A19_dsp_on:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 0
	mov	w1, 38
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	sandisk_A19_dsp_on, .-sandisk_A19_dsp_on
	.align	2
	.global	sandisk_A19_cmd25
	.type	sandisk_A19_cmd25, %function
sandisk_A19_cmd25:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 0
	mov	w1, 37
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	sandisk_A19_cmd25, .-sandisk_A19_cmd25
	.align	2
	.global	sandisk_A19_set_lmflgfix_next
	.type	sandisk_A19_set_lmflgfix_next, %function
sandisk_A19_set_lmflgfix_next:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strb	w1, [x29, 23]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 0
	mov	w1, 92
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, -59
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	strb	wzr, [x29, 39]
	mov	w0, 1
	strb	w0, [x29, 38]
	mov	w0, 85
	strb	w0, [x29, 37]
	add	x2, x29, 38
	add	x1, x29, 39
	add	x0, x29, 37
	mov	w6, 1
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 52
	bne	.L1475
	mov	w0, 34
	strb	w0, [x29, 39]
	b	.L1476
.L1475:
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 53
	bne	.L1477
	mov	w0, 37
	strb	w0, [x29, 39]
	b	.L1476
.L1477:
	mov	w0, 35
	strb	w0, [x29, 39]
.L1476:
	ldrb	w0, [x29, 23]
	strb	w0, [x29, 38]
	mov	w0, 85
	strb	w0, [x29, 37]
	add	x2, x29, 38
	add	x1, x29, 39
	add	x0, x29, 37
	mov	w6, 1
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	strb	wzr, [x29, 39]
	strb	wzr, [x29, 38]
	mov	w0, 85
	strb	w0, [x29, 37]
	add	x2, x29, 38
	add	x1, x29, 39
	add	x0, x29, 37
	mov	w6, 1
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	sandisk_A19_set_lmflgfix_next, .-sandisk_A19_set_lmflgfix_next
	.section	.rodata
	.align	3
.LC363:
	.string	"sandisk_A19 retry!\n"
	.align	3
.LC364:
	.string	"sandisk_A19 set readretry error\n"
	.align	3
.LC365:
	.string	"retry spare all 0xff! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.align	3
.LC366:
	.string	"sandisk_A19 ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.align	3
.LC367:
	.string	"sandisk_A19 ReadRetry ok with ecc limit! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	sandisk_A19_read_page_end
	.type	sandisk_A19_read_page_end, %function
sandisk_A19_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1480
	adrp	x0, .LC363
	add	x0, x0, :lo12:.LC363
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1481
.L1489:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	uxth	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	sandisk_A19_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1482
	adrp	x0, .LC364
	add	x0, x0, :lo12:.LC364
	bl	nand_print_dbg
	b	.L1483
.L1482:
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1484
	mov	w0, -2
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC365
	add	x0, x0, :lo12:.LC365
	bl	nand_print_dbg
	b	.L1483
.L1484:
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1485
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1483
.L1485:
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1486
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC366
	add	x0, x0, :lo12:.LC366
	bl	nand_print_dbg
	b	.L1487
.L1486:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC367
	add	x0, x0, :lo12:.LC367
	bl	nand_print_dbg
.L1487:
	mov	w0, 10
	str	w0, [x29, 40]
	b	.L1488
.L1483:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1481:
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1489
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 51
	beq	.L1525
	str	wzr, [x29, 44]
	b	.L1492
.L1499:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	uxth	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	sandisk_A19_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1493
	adrp	x0, .LC364
	add	x0, x0, :lo12:.LC364
	bl	nand_print_dbg
	b	.L1494
.L1493:
	ldr	x0, [x29, 24]
	bl	sandisk_A19_dsp_on
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1495
	mov	w0, -2
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC365
	add	x0, x0, :lo12:.LC365
	bl	nand_print_dbg
	b	.L1494
.L1495:
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1496
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1494
.L1496:
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1497
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC366
	add	x0, x0, :lo12:.LC366
	bl	nand_print_dbg
	b	.L1498
.L1497:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC367
	add	x0, x0, :lo12:.LC367
	bl	nand_print_dbg
.L1498:
	mov	w0, 10
	str	w0, [x29, 40]
	b	.L1488
.L1494:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1492:
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1499
	str	wzr, [x29, 44]
	b	.L1500
.L1507:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	uxth	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	sandisk_A19_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1501
	adrp	x0, .LC364
	add	x0, x0, :lo12:.LC364
	bl	nand_print_dbg
	b	.L1502
.L1501:
	ldr	x0, [x29, 24]
	bl	sandisk_A19_cmd25
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1503
	mov	w0, -2
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC365
	add	x0, x0, :lo12:.LC365
	bl	nand_print_dbg
	b	.L1502
.L1503:
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1504
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1502
.L1504:
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1505
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC366
	add	x0, x0, :lo12:.LC366
	bl	nand_print_dbg
	b	.L1506
.L1505:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC367
	add	x0, x0, :lo12:.LC367
	bl	nand_print_dbg
.L1506:
	mov	w0, 10
	str	w0, [x29, 40]
	b	.L1488
.L1502:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1500:
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1507
	b	.L1491
.L1525:
	nop
.L1491:
	str	wzr, [x29, 44]
	b	.L1508
.L1523:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	uxth	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	sandisk_A19_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1509
	adrp	x0, .LC364
	add	x0, x0, :lo12:.LC364
	bl	nand_print_dbg
	b	.L1510
.L1509:
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 52
	bne	.L1511
	mov	w1, -58
	ldr	x0, [x29, 24]
	bl	sandisk_A19_set_lmflgfix_next
	b	.L1512
.L1511:
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 53
	bne	.L1513
	mov	w1, 26
	ldr	x0, [x29, 24]
	bl	sandisk_A19_set_lmflgfix_next
	b	.L1512
.L1513:
	mov	w1, -64
	ldr	x0, [x29, 24]
	bl	sandisk_A19_set_lmflgfix_next
.L1512:
	ldr	x0, [x29, 24]
	bl	sandisk_A19_cmd25
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 50
	beq	.L1514
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 52
	beq	.L1514
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 53
	bne	.L1515
.L1514:
	ldr	x0, [x29, 24]
	bl	sandisk_A19_dsp_on
.L1515:
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 52
	bne	.L1516
	mov	w1, -62
	ldr	x0, [x29, 24]
	bl	sandisk_A19_set_lmflgfix_next
	b	.L1517
.L1516:
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 53
	bne	.L1518
	mov	w1, 10
	ldr	x0, [x29, 24]
	bl	sandisk_A19_set_lmflgfix_next
	b	.L1517
.L1518:
	mov	w1, 64
	ldr	x0, [x29, 24]
	bl	sandisk_A19_set_lmflgfix_next
.L1517:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1519
	mov	w0, -2
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC365
	add	x0, x0, :lo12:.LC365
	bl	nand_print_dbg
	b	.L1510
.L1519:
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1520
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1510
.L1520:
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1521
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC366
	add	x0, x0, :lo12:.LC366
	bl	nand_print_dbg
	b	.L1522
.L1521:
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC367
	add	x0, x0, :lo12:.LC367
	bl	nand_print_dbg
.L1522:
	mov	w0, 10
	str	w0, [x29, 40]
	b	.L1488
.L1510:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1508:
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1523
.L1488:
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	sandisk_A19_exit_readretry
.L1480:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	sandisk_A19_read_page_end, .-sandisk_A19_read_page_end
	.section	.rodata
	.align	3
.LC368:
	.string	"sandisk_A19 retry mode err : %d !\n"
	.align	3
.LC369:
	.string	"sandisk A19 read retry conut: %d !\n"
	.text
	.align	2
	.global	sandisk_A19_readretry_init
	.type	sandisk_A19_readretry_init, %function
sandisk_A19_readretry_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, sandisk_A19_read_retry_reg_cnt
	add	x0, x0, :lo12:sandisk_A19_read_retry_reg_cnt
	strb	w1, [x0]
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 50
	beq	.L1527
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 51
	beq	.L1527
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 52
	beq	.L1527
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 53
	beq	.L1527
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC368
	add	x0, x0, :lo12:.LC368
	bl	nand_print
	mov	w0, -99
	b	.L1528
.L1527:
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC369
	add	x0, x0, :lo12:.LC369
	bl	nand_print_dbg
	mov	w0, 0
.L1528:
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_A19_readretry_init, .-sandisk_A19_readretry_init
	.align	2
	.global	sandisk_A19_readretry_exit
	.type	sandisk_A19_readretry_exit, %function
sandisk_A19_readretry_exit:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	sandisk_A19_readretry_exit, .-sandisk_A19_readretry_exit
	.align	2
	.global	sandisk_A19_read_retry_clock_save
	.type	sandisk_A19_read_retry_clock_save, %function
sandisk_A19_read_retry_clock_save:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, sandisk_A19_sclk1_bak
	add	x3, x0, :lo12:sandisk_A19_sclk1_bak
	adrp	x0, sandisk_A19_sclk0_bak
	add	x2, x0, :lo12:sandisk_A19_sclk0_bak
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_get_clk
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	mov	w3, 20
	mov	w2, 10
	bl	nand_set_clk
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_A19_read_retry_clock_save, .-sandisk_A19_read_retry_clock_save
	.align	2
	.global	sandisk_A19_read_retry_clock_recover
	.type	sandisk_A19_read_retry_clock_recover, %function
sandisk_A19_read_retry_clock_recover:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, sandisk_A19_sclk0_bak
	add	x0, x0, :lo12:sandisk_A19_sclk0_bak
	ldr	w2, [x0]
	adrp	x0, sandisk_A19_sclk1_bak
	add	x0, x0, :lo12:sandisk_A19_sclk1_bak
	ldr	w3, [x0]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_set_clk
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_A19_read_retry_clock_recover, .-sandisk_A19_read_retry_clock_recover
	.align	2
	.global	sandisk_A19_vender_set_param
	.type	sandisk_A19_vender_set_param, %function
sandisk_A19_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 60]
	adrp	x0, sandisk_A19_read_retry_cmd_1y
	add	x0, x0, :lo12:sandisk_A19_read_retry_cmd_1y
	mov	w6, 1
	mov	w5, 4
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_A19_vender_set_param, .-sandisk_A19_vender_set_param
	.align	2
	.global	sandisk_A19_set_readretry
	.type	sandisk_A19_set_readretry, %function
sandisk_A19_set_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	str	wzr, [x29, 52]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 52
	bne	.L1538
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x0, w0
	lsl	x1, x0, 2
	adrp	x0, sandisk_A19_1z_16g
	add	x0, x0, :lo12:sandisk_A19_1z_16g
	add	x0, x1, x0
	str	x0, [x29, 56]
	b	.L1539
.L1538:
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 53
	bne	.L1540
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x0, w0
	lsl	x1, x0, 2
	adrp	x0, sandisk_A19_1z_8g
	add	x0, x0, :lo12:sandisk_A19_1z_8g
	add	x0, x1, x0
	str	x0, [x29, 56]
	b	.L1539
.L1540:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x0, w0
	lsl	x1, x0, 2
	adrp	x0, sandisk_A19_1y
	add	x0, x0, :lo12:sandisk_A19_1y
	add	x0, x1, x0
	str	x0, [x29, 56]
.L1539:
	adrp	x0, sandisk_A19_read_retry_reg_adr_1y
	add	x0, x0, :lo12:sandisk_A19_read_retry_reg_adr_1y
	str	x0, [x29, 40]
	mov	w0, 1
	str	w0, [x29, 36]
	ldr	w3, [x29, 36]
	ldr	x2, [x29, 40]
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 24]
	bl	sandisk_A19_vender_set_param
	str	w0, [x29, 52]
	mov	w2, 0
	mov	w1, 93
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 52]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_A19_set_readretry, .-sandisk_A19_set_readretry
	.align	2
	.global	sandisk_A19_exit_readretry
	.type	sandisk_A19_exit_readretry, %function
sandisk_A19_exit_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 60]
	strb	wzr, [x29, 40]
	strb	wzr, [x29, 41]
	strb	wzr, [x29, 42]
	strb	wzr, [x29, 43]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 1
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	adrp	x0, sandisk_A19_read_retry_reg_adr_1y
	add	x0, x0, :lo12:sandisk_A19_read_retry_reg_adr_1y
	str	x0, [x29, 48]
	mov	w0, 1
	str	w0, [x29, 44]
	add	x0, x29, 40
	ldr	w3, [x29, 44]
	ldr	x2, [x29, 48]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	sandisk_A19_vender_set_param
	str	w0, [x29, 60]
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_A19_exit_readretry, .-sandisk_A19_exit_readretry
	.section	.rodata
	.align	3
.LC370:
	.string	" sandisk_A19_special_init sandisk_A19_read_retry_mode:%d sandisk_A19_read_retry_cycle :%d sandisk_A19_read_retry_reg_cnt %d \n"
	.align	3
.LC371:
	.string	" sandisk_A19_special_init error sandisk_A19_read_retry_mode:%d sandisk_A19_read_retry_cycle :%d sandisk_A19_read_retry_reg_cnt %d \n"
	.text
	.align	2
	.global	sandisk_A19_special_init
	.type	sandisk_A19_special_init, %function
sandisk_A19_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1545
.L1546:
	ldr	x0, [x29, 16]
	bl	sandisk_A19_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1545:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1546
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1547
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, sandisk_A19_read_page_end
	add	x1, x1, :lo12:sandisk_A19_read_page_end
	str	x1, [x0]
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, sandisk_A19_read_retry_reg_cnt
	add	x0, x0, :lo12:sandisk_A19_read_retry_reg_cnt
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC370
	add	x0, x0, :lo12:.LC370
	bl	nand_print_dbg
	b	.L1548
.L1547:
	adrp	x0, sandisk_A19_read_retry_mode
	add	x0, x0, :lo12:sandisk_A19_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, sandisk_A19_read_retry_cycle
	add	x0, x0, :lo12:sandisk_A19_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, sandisk_A19_read_retry_reg_cnt
	add	x0, x0, :lo12:sandisk_A19_read_retry_reg_cnt
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC371
	add	x0, x0, :lo12:.LC371
	bl	nand_print
.L1548:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_A19_special_init, .-sandisk_A19_special_init
	.section	.rodata
	.align	3
.LC372:
	.string	"%s sandisk toggle vendor specific setting failed!\n"
	.align	3
.LC373:
	.string	" sandisk_A19_special_exit \n"
	.text
	.align	2
	.global	sandisk_A19_special_exit
	.type	sandisk_A19_special_exit, %function
sandisk_A19_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1551
.L1554:
	ldr	x0, [x29, 24]
	bl	sandisk_A19_readretry_exit
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 270]
	cmp	w0, 0
	beq	.L1552
	ldr	x0, [x29, 24]
	str	wzr, [x0, 204]
	ldr	x0, [x29, 24]
	bl	sandisk_toggle_vendor_specific_setting
	str	w0, [x29, 20]
	ldr	w0, [x29, 20]
	cmp	w0, 0
	beq	.L1552
	adrp	x0, __func__.7318
	add	x1, x0, :lo12:__func__.7318
	adrp	x0, .LC372
	add	x0, x0, :lo12:.LC372
	bl	nand_print
	ldr	w0, [x29, 20]
	b	.L1553
.L1552:
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1551:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1554
	adrp	x0, .LC373
	add	x0, x0, :lo12:.LC373
	bl	nand_print_dbg
	mov	w0, 0
.L1553:
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_A19_special_exit, .-sandisk_A19_special_exit
	.section	.rodata
	.align	3
.LC374:
	.string	"fatal err -0, wrong input parameter, ch: %d  chip: %d/%d  block: %d/%d \n"
	.text
	.align	2
	.global	sandisk_A19_check_bad_block_first_burn
	.type	sandisk_A19_check_bad_block_first_burn, %function
sandisk_A19_check_bad_block_first_burn:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 76]
	str	wzr, [x29, 72]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 280]
	str	x0, [x29, 56]
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 280]
	add	x0, x0, 40
	str	x0, [x29, 48]
	ldr	x0, [x29, 64]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 56]
	ldr	w0, [x0, 16]
	cmp	w1, w0
	bcs	.L1556
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 4]
	ldr	x0, [x29, 64]
	ldr	w0, [x0, 32]
	cmp	w1, w0
	bcc	.L1557
.L1556:
	ldr	x0, [x29, 56]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 64]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 56]
	ldr	w3, [x0, 16]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 4]
	ldr	x0, [x29, 64]
	ldr	w5, [x0, 32]
	adrp	x0, .LC374
	add	x0, x0, :lo12:.LC374
	bl	nand_print
	mov	w0, -10
	b	.L1558
.L1557:
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 64]
	bl	nand_read_chip_status_ready
	ldr	x0, [x29, 64]
	bl	nand_enable_chip
	ldr	x0, [x29, 48]
	bl	ndfc_clean_cmd_seq
	ldr	x0, [x29, 48]
	str	wzr, [x0]
	ldr	x0, [x29, 48]
	mov	w1, 162
	str	w1, [x0, 12]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 16]
	ldr	x0, [x29, 48]
	str	wzr, [x0, 20]
	ldr	x0, [x29, 48]
	mov	w1, 128
	str	w1, [x0, 84]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 80]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 88]
	ldr	x0, [x29, 48]
	str	wzr, [x0, 92]
	ldr	x0, [x29, 64]
	ldr	w3, [x0, 44]
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	mov	w2, w0
	mov	w0, w3
	bl	get_row_addr
	str	w0, [x29, 76]
	ldr	x0, [x29, 48]
	mov	w1, 5
	str	w1, [x0, 112]
	ldr	x0, [x29, 48]
	add	x0, x0, 96
	mov	x4, x0
	mov	w3, 3
	ldr	w2, [x29, 76]
	mov	w1, 2
	ldr	w0, [x29, 72]
	bl	fill_cmd_addr
	ldr	x0, [x29, 48]
	mov	w1, 16
	str	w1, [x0, 156]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 152]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 160]
	ldr	x0, [x29, 48]
	mov	w1, 1
	str	w1, [x0, 164]
	ldr	x0, [x29, 64]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 48]
	bl	ndfc_execute_cmd
	str	w0, [x29, 44]
	ldr	x0, [x29, 64]
	bl	nand_read_chip_status_ready
	str	w0, [x29, 44]
	ldr	x0, [x29, 64]
	bl	nand_disable_chip
	ldr	w0, [x29, 44]
.L1558:
	ldp	x29, x30, [sp], 80
	ret
	.size	sandisk_A19_check_bad_block_first_burn, .-sandisk_A19_check_bad_block_first_burn
	.global	micron_read_retry_mode
	.bss
	.type	micron_read_retry_mode, %object
	.size	micron_read_retry_mode, 1
micron_read_retry_mode:
	.zero	1
	.global	micron_read_retry_cycle
	.type	micron_read_retry_cycle, %object
	.size	micron_read_retry_cycle, 1
micron_read_retry_cycle:
	.zero	1
	.global	micron_p1_1
	.data
	.align	3
	.type	micron_p1_1, %object
	.size	micron_p1_1, 10
micron_p1_1:
	.byte	0
	.byte	0
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.global	micron_p1_2
	.align	3
	.type	micron_p1_2, %object
	.size	micron_p1_2, 12
micron_p1_2:
	.byte	0
	.byte	0
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
	.byte	12
	.global	micron_p1_3
	.align	3
	.type	micron_p1_3, %object
	.size	micron_p1_3, 7
micron_p1_3:
	.byte	1
	.byte	2
	.byte	3
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.global	micron_p1_4
	.align	3
	.type	micron_p1_4, %object
	.size	micron_p1_4, 18
micron_p1_4:
	.byte	0
	.byte	0
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
	.byte	9
	.byte	10
	.byte	11
	.byte	12
	.byte	13
	.byte	14
	.byte	15
	.local	micron_p1
	.comm	micron_p1,8,8
	.section	.rodata
	.align	3
.LC375:
	.string	"set feature(addr %d) p0: %d readretry Configuration failed!\n"
	.text
	.align	2
	.global	micron_intel_set_feature
	.type	micron_intel_set_feature, %function
micron_intel_set_feature:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strb	w1, [x29, 23]
	strb	w2, [x29, 22]
	ldrb	w0, [x29, 23]
	strb	w0, [x29, 47]
	ldrb	w0, [x29, 22]
	strb	w0, [x29, 40]
	strb	wzr, [x29, 41]
	strb	wzr, [x29, 42]
	strb	wzr, [x29, 43]
	add	x1, x29, 40
	add	x0, x29, 23
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_set_feature
	add	x1, x29, 32
	add	x0, x29, 23
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_get_feature
	ldrb	w1, [x29, 32]
	ldrb	w0, [x29, 40]
	cmp	w1, w0
	beq	.L1560
	ldrb	w1, [x29, 47]
	ldrb	w2, [x29, 22]
	adrp	x0, .LC375
	add	x0, x0, :lo12:.LC375
	bl	nand_print
	mov	w0, -72
	b	.L1562
.L1560:
	mov	w0, 0
.L1562:
	ldp	x29, x30, [sp], 48
	ret
	.size	micron_intel_set_feature, .-micron_intel_set_feature
	.align	2
	.global	micron_set_readretry
	.type	micron_set_readretry, %function
micron_set_readretry:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 64
	beq	.L1564
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 65
	beq	.L1564
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 80
	beq	.L1564
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 66
	bne	.L1571
.L1564:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	strb	w0, [x29, 43]
	ldrb	w0, [x29, 43]
	cmp	w0, 255
	bne	.L1566
	strb	wzr, [x29, 43]
.L1566:
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 80
	bne	.L1567
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	ldr	x1, [x0]
	ldrb	w0, [x29, 43]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1568
	mov	w2, 1
	mov	w1, -109
	ldr	x0, [x29, 24]
	bl	micron_intel_set_feature
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	bne	.L1572
.L1568:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 255
	bne	.L1567
	mov	w2, 0
	mov	w1, -119
	ldr	x0, [x29, 24]
	bl	micron_intel_set_feature
	str	w0, [x29, 44]
	mov	w2, 0
	mov	w1, -109
	ldr	x0, [x29, 24]
	bl	micron_intel_set_feature
	mov	w1, w0
	ldr	w0, [x29, 44]
	orr	w0, w0, w1
	str	w0, [x29, 44]
	b	.L1565
.L1567:
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	ldr	x1, [x0]
	ldrb	w0, [x29, 43]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w2, w0
	mov	w1, -119
	ldr	x0, [x29, 24]
	bl	micron_intel_set_feature
	str	w0, [x29, 44]
	b	.L1565
.L1571:
	nop
	b	.L1565
.L1572:
	nop
.L1565:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 255
	bne	.L1569
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
.L1569:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	micron_set_readretry, .-micron_set_readretry
	.section	.rodata
	.align	3
.LC376:
	.string	"micron retry!\n"
	.align	3
.LC377:
	.string	"micron ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	micron_read_page_end
	.type	micron_read_page_end, %function
micron_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1574
	adrp	x0, .LC376
	add	x0, x0, :lo12:.LC376
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1575
.L1580:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 32]
	bl	micron_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1582
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1578
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1577
.L1578:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC377
	add	x0, x0, :lo12:.LC377
	bl	nand_print_dbg
	b	.L1579
.L1582:
	nop
.L1577:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1575:
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1580
.L1579:
	ldr	x0, [x29, 32]
	mov	w1, 255
	str	w1, [x0, 184]
	ldr	x0, [x29, 32]
	bl	micron_set_readretry
.L1574:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	micron_read_page_end, .-micron_read_page_end
	.section	.rodata
	.align	3
.LC378:
	.string	"micron read retry cannot support!\n"
	.align	3
.LC379:
	.string	"micron read retry conut: %d !\n"
	.text
	.align	2
	.global	micron_readretry_init
	.type	micron_readretry_init, %function
micron_readretry_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	strb	wzr, [x0]
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	adrp	x1, micron_p1_2
	add	x1, x1, :lo12:micron_p1_2
	str	x1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	strb	w1, [x0]
	mov	w2, 0
	mov	w1, -119
	ldr	x0, [x29, 24]
	bl	micron_intel_set_feature
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 65
	bne	.L1584
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	mov	w1, 12
	strb	w1, [x0]
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	adrp	x1, micron_p1_2
	add	x1, x1, :lo12:micron_p1_2
	str	x1, [x0]
	b	.L1585
.L1584:
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 64
	bne	.L1586
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	adrp	x1, micron_p1_1
	add	x1, x1, :lo12:micron_p1_1
	str	x1, [x0]
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	mov	w1, 10
	strb	w1, [x0]
	b	.L1585
.L1586:
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 80
	bne	.L1587
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	adrp	x1, micron_p1_3
	add	x1, x1, :lo12:micron_p1_3
	str	x1, [x0]
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	mov	w1, 7
	strb	w1, [x0]
	mov	w2, 0
	mov	w1, -109
	ldr	x0, [x29, 24]
	bl	micron_intel_set_feature
	b	.L1585
.L1587:
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 66
	bne	.L1588
	adrp	x0, micron_p1
	add	x0, x0, :lo12:micron_p1
	adrp	x1, micron_p1_4
	add	x1, x1, :lo12:micron_p1_4
	str	x1, [x0]
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	mov	w1, 18
	strb	w1, [x0]
	b	.L1585
.L1588:
	adrp	x0, .LC378
	add	x0, x0, :lo12:.LC378
	bl	nand_print
	mov	w0, -71
	b	.L1589
.L1585:
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC379
	add	x0, x0, :lo12:.LC379
	bl	nand_print_dbg
	mov	w0, 0
.L1589:
	ldp	x29, x30, [sp], 32
	ret
	.size	micron_readretry_init, .-micron_readretry_init
	.align	2
	.global	micron_readretry_exit
	.type	micron_readretry_exit, %function
micron_readretry_exit:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	strb	wzr, [x0]
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	strb	wzr, [x0]
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	micron_readretry_exit, .-micron_readretry_exit
	.section	.rodata
	.align	3
.LC380:
	.string	" micron_special_init micron_read_retry_mode:%d micron_read_retry_cycle:%d \n"
	.align	3
.LC381:
	.string	" micron_special_init error micron_read_retry_mode:%d micron_read_retry_cycle:%d \n"
	.text
	.align	2
	.global	micron_special_init
	.type	micron_special_init, %function
micron_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1593
.L1594:
	ldr	x0, [x29, 16]
	bl	micron_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1593:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1594
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1595
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, micron_read_page_end
	add	x1, x1, :lo12:micron_read_page_end
	str	x1, [x0]
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC380
	add	x0, x0, :lo12:.LC380
	bl	nand_print_dbg
	b	.L1596
.L1595:
	adrp	x0, micron_read_retry_mode
	add	x0, x0, :lo12:micron_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, micron_read_retry_cycle
	add	x0, x0, :lo12:micron_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC381
	add	x0, x0, :lo12:.LC381
	bl	nand_print
.L1596:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	micron_special_init, .-micron_special_init
	.section	.rodata
	.align	3
.LC382:
	.string	" micron_special_exit \n"
	.text
	.align	2
	.global	micron_special_exit
	.type	micron_special_exit, %function
micron_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1599
.L1600:
	ldr	x0, [x29, 24]
	bl	micron_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1599:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1600
	adrp	x0, .LC382
	add	x0, x0, :lo12:.LC382
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	micron_special_exit, .-micron_special_exit
	.global	sandisk_read_retry_mode
	.bss
	.type	sandisk_read_retry_mode, %object
	.size	sandisk_read_retry_mode, 1
sandisk_read_retry_mode:
	.zero	1
	.global	sandisk_read_retry_cycle
	.type	sandisk_read_retry_cycle, %object
	.size	sandisk_read_retry_cycle, 1
sandisk_read_retry_cycle:
	.zero	1
	.global	sandisk_read_retry_reg_cnt
	.type	sandisk_read_retry_reg_cnt, %object
	.size	sandisk_read_retry_reg_cnt, 1
sandisk_read_retry_reg_cnt:
	.zero	1
	.global	sandisk_acti_start_cmd
	.data
	.align	3
	.type	sandisk_acti_start_cmd, %object
	.size	sandisk_acti_start_cmd, 2
sandisk_acti_start_cmd:
	.byte	59
	.byte	-71
	.global	sandisk_acti_start_addr
	.align	3
	.type	sandisk_acti_start_addr, %object
	.size	sandisk_acti_start_addr, 9
sandisk_acti_start_addr:
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
	.byte	9
	.byte	10
	.byte	11
	.byte	12
	.global	sandisk_read_retry_enable_cmd_19
	.align	3
	.type	sandisk_read_retry_enable_cmd_19, %object
	.size	sandisk_read_retry_enable_cmd_19, 1
sandisk_read_retry_enable_cmd_19:
	.byte	-74
	.global	sandisk_read_retry_disable_cmd_19
	.align	3
	.type	sandisk_read_retry_disable_cmd_19, %object
	.size	sandisk_read_retry_disable_cmd_19, 1
sandisk_read_retry_disable_cmd_19:
	.byte	-42
	.global	sandisk_read_retry_cmd_19
	.align	3
	.type	sandisk_read_retry_cmd_19, %object
	.size	sandisk_read_retry_cmd_19, 3
sandisk_read_retry_cmd_19:
	.byte	83
	.byte	83
	.byte	83
	.global	sandisk_read_retry_reg_adr_19
	.align	3
	.type	sandisk_read_retry_reg_adr_19, %object
	.size	sandisk_read_retry_reg_adr_19, 9
sandisk_read_retry_reg_adr_19:
	.byte	4
	.byte	5
	.byte	7
	.zero	6
	.global	sandisk_19_low
	.align	3
	.type	sandisk_19_low, %object
	.size	sandisk_19_low, 48
sandisk_19_low:
	.byte	-16
	.byte	0
	.byte	-16
	.byte	-32
	.byte	0
	.byte	-32
	.byte	-48
	.byte	0
	.byte	-48
	.byte	16
	.byte	0
	.byte	16
	.byte	32
	.byte	0
	.byte	32
	.byte	48
	.byte	0
	.byte	48
	.byte	-64
	.byte	0
	.byte	-48
	.byte	0
	.byte	0
	.byte	16
	.byte	0
	.byte	0
	.byte	32
	.byte	16
	.byte	0
	.byte	32
	.byte	-80
	.byte	0
	.byte	-48
	.byte	-96
	.byte	0
	.byte	-48
	.byte	-112
	.byte	0
	.byte	-48
	.byte	-80
	.byte	0
	.byte	-64
	.byte	-96
	.byte	0
	.byte	-64
	.byte	-112
	.byte	0
	.byte	-64
	.global	sandisk_19_high
	.align	3
	.type	sandisk_19_high, %object
	.size	sandisk_19_high, 60
sandisk_19_high:
	.byte	0
	.byte	-16
	.byte	0
	.byte	15
	.byte	-32
	.byte	0
	.byte	15
	.byte	-48
	.byte	0
	.byte	14
	.byte	-32
	.byte	0
	.byte	14
	.byte	-48
	.byte	0
	.byte	13
	.byte	-16
	.byte	0
	.byte	13
	.byte	-32
	.byte	0
	.byte	13
	.byte	-48
	.byte	0
	.byte	1
	.byte	16
	.byte	0
	.byte	2
	.byte	32
	.byte	0
	.byte	2
	.byte	16
	.byte	0
	.byte	3
	.byte	32
	.byte	0
	.byte	15
	.byte	0
	.byte	0
	.byte	14
	.byte	-16
	.byte	0
	.byte	13
	.byte	-64
	.byte	0
	.byte	15
	.byte	-16
	.byte	0
	.byte	1
	.byte	0
	.byte	0
	.byte	2
	.byte	0
	.byte	0
	.byte	13
	.byte	-80
	.byte	0
	.byte	12
	.byte	-96
	.byte	0
	.global	sandisk_read_retry_cmd_24
	.align	3
	.type	sandisk_read_retry_cmd_24, %object
	.size	sandisk_read_retry_cmd_24, 3
sandisk_read_retry_cmd_24:
	.byte	83
	.byte	83
	.byte	83
	.global	sandisk_read_retry_reg_adr_24
	.align	3
	.type	sandisk_read_retry_reg_adr_24, %object
	.size	sandisk_read_retry_reg_adr_24, 9
sandisk_read_retry_reg_adr_24:
	.byte	4
	.byte	5
	.byte	7
	.zero	6
	.global	sandisk_24
	.align	3
	.type	sandisk_24, %object
	.size	sandisk_24, 27
sandisk_24:
	.byte	0
	.byte	-16
	.byte	0
	.byte	0
	.byte	-32
	.byte	0
	.byte	-1
	.byte	-16
	.byte	-16
	.byte	-18
	.byte	-32
	.byte	-32
	.byte	-34
	.byte	-48
	.byte	-48
	.byte	-51
	.byte	-64
	.byte	-64
	.byte	1
	.byte	0
	.byte	0
	.byte	2
	.byte	0
	.byte	0
	.byte	3
	.byte	0
	.byte	0
	.global	sandisk_read_retry_enable_cmd_1y
	.align	3
	.type	sandisk_read_retry_enable_cmd_1y, %object
	.size	sandisk_read_retry_enable_cmd_1y, 1
sandisk_read_retry_enable_cmd_1y:
	.byte	93
	.global	sandisk_read_retry_exit_cmd_1y
	.align	3
	.type	sandisk_read_retry_exit_cmd_1y, %object
	.size	sandisk_read_retry_exit_cmd_1y, 1
sandisk_read_retry_exit_cmd_1y:
	.byte	-1
	.global	sandisk_read_retry_cmd_1y
	.align	3
	.type	sandisk_read_retry_cmd_1y, %object
	.size	sandisk_read_retry_cmd_1y, 1
sandisk_read_retry_cmd_1y:
	.byte	-17
	.global	sandisk_read_retry_reg_adr_1y
	.align	3
	.type	sandisk_read_retry_reg_adr_1y, %object
	.size	sandisk_read_retry_reg_adr_1y, 1
sandisk_read_retry_reg_adr_1y:
	.byte	17
	.global	sandisk_1y
	.align	3
	.type	sandisk_1y, %object
	.size	sandisk_1y, 76
sandisk_1y:
	.byte	4
	.byte	0
	.byte	124
	.byte	124
	.byte	8
	.byte	0
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	0
	.byte	124
	.byte	124
	.byte	124
	.byte	0
	.byte	0
	.byte	12
	.byte	0
	.byte	120
	.byte	120
	.byte	16
	.byte	0
	.byte	8
	.byte	8
	.byte	124
	.byte	124
	.byte	120
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	120
	.byte	116
	.byte	20
	.byte	4
	.byte	4
	.byte	0
	.byte	120
	.byte	120
	.byte	0
	.byte	124
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	116
	.byte	120
	.byte	120
	.byte	4
	.byte	0
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	116
	.byte	112
	.byte	120
	.byte	116
	.byte	120
	.byte	120
	.byte	120
	.byte	112
	.byte	120
	.byte	116
	.byte	120
	.byte	112
	.byte	112
	.byte	112
	.local	sandisk_sclk0_bak
	.comm	sandisk_sclk0_bak,4,4
	.local	sandisk_sclk1_bak
	.comm	sandisk_sclk1_bak,4,4
	.local	sandisk_toggle_mode_flag
	.comm	sandisk_toggle_mode_flag,4,4
	.text
	.align	2
	.global	sandisk_read_retry_clock_save
	.type	sandisk_read_retry_clock_save, %function
sandisk_read_retry_clock_save:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, sandisk_sclk1_bak
	add	x3, x0, :lo12:sandisk_sclk1_bak
	adrp	x0, sandisk_sclk0_bak
	add	x2, x0, :lo12:sandisk_sclk0_bak
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_get_clk
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	mov	w3, 20
	mov	w2, 10
	bl	nand_set_clk
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_read_retry_clock_save, .-sandisk_read_retry_clock_save
	.align	2
	.global	sandisk_vender_set_param
	.type	sandisk_vender_set_param, %function
sandisk_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 60]
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 48
	beq	.L1605
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 49
	bne	.L1606
.L1605:
	adrp	x0, sandisk_acti_start_cmd
	add	x0, x0, :lo12:sandisk_acti_start_cmd
	mov	w6, 2
	mov	w5, 0
	mov	x4, 0
	mov	x3, 0
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	str	w0, [x29, 60]
	adrp	x0, sandisk_read_retry_cmd_19
	add	x0, x0, :lo12:sandisk_read_retry_cmd_19
	mov	w6, 3
	mov	w5, 1
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
.L1606:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 50
	bne	.L1607
	adrp	x0, sandisk_read_retry_cmd_1y
	add	x0, x0, :lo12:sandisk_read_retry_cmd_1y
	mov	w6, 1
	mov	w5, 4
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	str	w0, [x29, 60]
.L1607:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_vender_set_param, .-sandisk_vender_set_param
	.align	2
	.global	sandisk_read_retry_clock_recover
	.type	sandisk_read_retry_clock_recover, %function
sandisk_read_retry_clock_recover:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, sandisk_sclk0_bak
	add	x0, x0, :lo12:sandisk_sclk0_bak
	ldr	w2, [x0]
	adrp	x0, sandisk_sclk1_bak
	add	x0, x0, :lo12:sandisk_sclk1_bak
	ldr	w3, [x0]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_set_clk
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_read_retry_clock_recover, .-sandisk_read_retry_clock_recover
	.align	2
	.global	sandisk_end_one_read_retry
	.type	sandisk_end_one_read_retry, %function
sandisk_end_one_read_retry:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 48
	beq	.L1612
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 49
	beq	.L1612
	mov	w0, 0
	b	.L1613
.L1612:
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_is_toggle_interface
	cmp	w0, 0
	beq	.L1614
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_legacy_interface
	adrp	x0, sandisk_toggle_mode_flag
	add	x0, x0, :lo12:sandisk_toggle_mode_flag
	mov	w1, 1
	str	w1, [x0]
.L1614:
	adrp	x0, sandisk_read_retry_disable_cmd_19
	add	x0, x0, :lo12:sandisk_read_retry_disable_cmd_19
	mov	w6, 1
	mov	w5, 0
	mov	x4, 0
	mov	x3, 0
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	adrp	x0, sandisk_toggle_mode_flag
	add	x0, x0, :lo12:sandisk_toggle_mode_flag
	ldr	w0, [x0]
	cmp	w0, 1
	bne	.L1615
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_toggle_interface
.L1615:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	mov	w0, 0
.L1613:
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_end_one_read_retry, .-sandisk_end_one_read_retry
	.align	2
	.global	sandisk_set_readretry
	.type	sandisk_set_readretry, %function
sandisk_set_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	strh	w1, [x29, 22]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 24]
	bl	sandisk_read_retry_clock_save
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_is_toggle_interface
	cmp	w0, 0
	beq	.L1617
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_legacy_interface
	adrp	x0, sandisk_toggle_mode_flag
	add	x0, x0, :lo12:sandisk_toggle_mode_flag
	mov	w1, 1
	str	w1, [x0]
.L1617:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 48
	bne	.L1618
	ldrh	w0, [x29, 22]
	cmp	w0, 255
	beq	.L1619
	ldrh	w0, [x29, 22]
	cmp	w0, 0
	beq	.L1620
	ldrh	w0, [x29, 22]
	and	w0, w0, 1
	uxth	w0, w0
	cmp	w0, 0
	beq	.L1619
.L1620:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 1
	add	x0, x0, x1
	adrp	x1, sandisk_19_low
	add	x1, x1, :lo12:sandisk_19_low
	add	x0, x0, x1
	str	x0, [x29, 48]
	adrp	x0, sandisk_read_retry_reg_adr_19
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_19
	str	x0, [x29, 40]
	mov	w0, 3
	str	w0, [x29, 36]
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	mov	w1, 16
	strb	w1, [x0]
	b	.L1621
.L1619:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 1
	add	x0, x0, x1
	adrp	x1, sandisk_19_high
	add	x1, x1, :lo12:sandisk_19_high
	add	x0, x0, x1
	str	x0, [x29, 48]
	adrp	x0, sandisk_read_retry_reg_adr_19
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_19
	str	x0, [x29, 40]
	mov	w0, 3
	str	w0, [x29, 36]
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	mov	w1, 20
	strb	w1, [x0]
.L1621:
	ldr	w3, [x29, 36]
	ldr	x2, [x29, 40]
	ldr	x1, [x29, 48]
	ldr	x0, [x29, 24]
	bl	sandisk_vender_set_param
	str	w0, [x29, 60]
	mov	w2, 0
	mov	w1, -74
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	b	.L1622
.L1618:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 49
	bne	.L1623
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 1
	add	x0, x0, x1
	adrp	x1, sandisk_24
	add	x1, x1, :lo12:sandisk_24
	add	x0, x0, x1
	str	x0, [x29, 48]
	adrp	x0, sandisk_read_retry_reg_adr_24
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_24
	str	x0, [x29, 40]
	mov	w0, 3
	str	w0, [x29, 36]
	ldr	w3, [x29, 36]
	ldr	x2, [x29, 40]
	ldr	x1, [x29, 48]
	ldr	x0, [x29, 24]
	bl	sandisk_vender_set_param
	str	w0, [x29, 60]
	mov	w2, 0
	mov	w1, -74
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	b	.L1622
.L1623:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 50
	bne	.L1622
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x0, w0
	lsl	x1, x0, 2
	adrp	x0, sandisk_1y
	add	x0, x0, :lo12:sandisk_1y
	add	x0, x1, x0
	str	x0, [x29, 48]
	adrp	x0, sandisk_read_retry_reg_adr_1y
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_1y
	str	x0, [x29, 40]
	mov	w0, 1
	str	w0, [x29, 36]
	ldr	w3, [x29, 36]
	ldr	x2, [x29, 40]
	ldr	x1, [x29, 48]
	ldr	x0, [x29, 24]
	bl	sandisk_vender_set_param
	str	w0, [x29, 60]
	mov	w2, 0
	mov	w1, 93
	ldr	x0, [x29, 24]
	bl	set_one_cmd
.L1622:
	adrp	x0, sandisk_toggle_mode_flag
	add	x0, x0, :lo12:sandisk_toggle_mode_flag
	ldr	w0, [x0]
	cmp	w0, 1
	bne	.L1624
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_toggle_interface
.L1624:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	x0, [x29, 24]
	bl	sandisk_read_retry_clock_recover
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_set_readretry, .-sandisk_set_readretry
	.section	.rodata
	.align	3
.LC383:
	.string	"sandisk retry!\n"
	.align	3
.LC384:
	.string	"sandisk ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	sandisk_read_page_end
	.type	sandisk_read_page_end, %function
sandisk_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1627
	adrp	x0, .LC383
	add	x0, x0, :lo12:.LC383
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1628
.L1634:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	uxth	w0, w0
	mov	w1, w0
	ldr	x0, [x29, 32]
	bl	sandisk_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1636
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	bl	sandisk_end_one_read_retry
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1631
	mov	w0, -2
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC365
	add	x0, x0, :lo12:.LC365
	bl	nand_print_dbg
	b	.L1630
.L1631:
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1632
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1630
.L1632:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC384
	add	x0, x0, :lo12:.LC384
	bl	nand_print_dbg
	b	.L1633
.L1636:
	nop
.L1630:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1628:
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1634
.L1633:
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	sandisk_exit_readretry
.L1627:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	sandisk_read_page_end, .-sandisk_read_page_end
	.section	.rodata
	.align	3
.LC385:
	.string	"sandisk activation seq fail 0x%x\n"
	.text
	.align	2
	.global	sandisk_activation_seq
	.type	sandisk_activation_seq, %function
sandisk_activation_seq:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, .LC1
	add	x1, x0, :lo12:.LC1
	add	x0, x29, 48
	ldr	x2, [x1]
	str	x2, [x0]
	ldrb	w1, [x1, 8]
	strb	w1, [x0, 8]
	strb	wzr, [x29, 32]
	strb	wzr, [x29, 33]
	strb	wzr, [x29, 34]
	strb	wzr, [x29, 35]
	strb	wzr, [x29, 36]
	strb	wzr, [x29, 37]
	strb	wzr, [x29, 38]
	strb	wzr, [x29, 39]
	strb	wzr, [x29, 40]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	adrp	x0, sandisk_acti_start_cmd
	add	x0, x0, :lo12:sandisk_acti_start_cmd
	mov	w6, 2
	mov	w5, 0
	mov	x4, 0
	mov	x3, 0
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	str	w0, [x29, 60]
	add	x2, x29, 32
	adrp	x0, sandisk_acti_start_addr
	add	x1, x0, :lo12:sandisk_acti_start_addr
	add	x0, x29, 48
	mov	w6, 9
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 60]
	cmp	w0, 0
	beq	.L1638
	adrp	x0, .LC385
	add	x0, x0, :lo12:.LC385
	ldr	w1, [x29, 60]
	bl	nand_print
.L1638:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_activation_seq, .-sandisk_activation_seq
	.section	.rodata
	.align	3
.LC1:
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.text
	.section	.rodata
	.align	3
.LC386:
	.string	"sandisk read retry conut: %d !\n"
	.text
	.align	2
	.global	sandisk_readretry_init
	.type	sandisk_readretry_init, %function
sandisk_readretry_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, sandisk_read_retry_reg_cnt
	add	x0, x0, :lo12:sandisk_read_retry_reg_cnt
	strb	w1, [x0]
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 48
	beq	.L1641
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 49
	bne	.L1642
.L1641:
	ldr	x0, [x29, 24]
	bl	sandisk_activation_seq
.L1642:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 48
	beq	.L1643
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 49
	beq	.L1643
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 50
	beq	.L1643
	mov	w0, -99
	b	.L1644
.L1643:
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC386
	add	x0, x0, :lo12:.LC386
	bl	nand_print_dbg
	mov	w0, 0
.L1644:
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_readretry_init, .-sandisk_readretry_init
	.align	2
	.global	sandisk_readretry_exit
	.type	sandisk_readretry_exit, %function
sandisk_readretry_exit:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	sandisk_readretry_exit, .-sandisk_readretry_exit
	.align	2
	.global	sandisk_exit_readretry
	.type	sandisk_exit_readretry, %function
sandisk_exit_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 60]
	strb	wzr, [x29, 40]
	strb	wzr, [x29, 41]
	strb	wzr, [x29, 42]
	strb	wzr, [x29, 43]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_is_toggle_interface
	cmp	w0, 0
	beq	.L1648
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_legacy_interface
	adrp	x0, sandisk_toggle_mode_flag
	add	x0, x0, :lo12:sandisk_toggle_mode_flag
	mov	w1, 1
	str	w1, [x0]
.L1648:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 48
	bne	.L1649
	adrp	x0, sandisk_read_retry_reg_adr_19
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_19
	str	x0, [x29, 48]
	mov	w0, 3
	str	w0, [x29, 44]
	add	x0, x29, 40
	ldr	w3, [x29, 44]
	ldr	x2, [x29, 48]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	sandisk_vender_set_param
	str	w0, [x29, 60]
	b	.L1650
.L1649:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 49
	bne	.L1651
	adrp	x0, sandisk_read_retry_reg_adr_24
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_24
	str	x0, [x29, 48]
	mov	w0, 3
	str	w0, [x29, 44]
	add	x0, x29, 40
	ldr	w3, [x29, 44]
	ldr	x2, [x29, 48]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	sandisk_vender_set_param
	str	w0, [x29, 60]
	b	.L1650
.L1651:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 50
	bne	.L1650
	mov	w2, 1
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	adrp	x0, sandisk_read_retry_reg_adr_1y
	add	x0, x0, :lo12:sandisk_read_retry_reg_adr_1y
	str	x0, [x29, 48]
	mov	w0, 1
	str	w0, [x29, 44]
	add	x0, x29, 40
	ldr	w3, [x29, 44]
	ldr	x2, [x29, 48]
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	sandisk_vender_set_param
	str	w0, [x29, 60]
.L1650:
	adrp	x0, sandisk_toggle_mode_flag
	add	x0, x0, :lo12:sandisk_toggle_mode_flag
	ldr	w0, [x0]
	cmp	w0, 1
	bne	.L1652
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_toggle_interface
.L1652:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	sandisk_exit_readretry, .-sandisk_exit_readretry
	.section	.rodata
	.align	3
.LC387:
	.string	" sandisk_special_init sandisk_read_retry_mode:%d sandisk_read_retry_cycle :%d sandisk_read_retry_reg_cnt %d \n"
	.align	3
.LC388:
	.string	" sandisk_special_init error sandisk_read_retry_mode:%d sandisk_read_retry_cycle :%d sandisk_read_retry_reg_cnt %d \n"
	.text
	.align	2
	.global	sandisk_special_init
	.type	sandisk_special_init, %function
sandisk_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1655
.L1656:
	ldr	x0, [x29, 16]
	bl	sandisk_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1655:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1656
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1657
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, sandisk_read_page_end
	add	x1, x1, :lo12:sandisk_read_page_end
	str	x1, [x0]
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, sandisk_read_retry_reg_cnt
	add	x0, x0, :lo12:sandisk_read_retry_reg_cnt
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC387
	add	x0, x0, :lo12:.LC387
	bl	nand_print_dbg
	b	.L1658
.L1657:
	adrp	x0, sandisk_read_retry_mode
	add	x0, x0, :lo12:sandisk_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, sandisk_read_retry_cycle
	add	x0, x0, :lo12:sandisk_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, sandisk_read_retry_reg_cnt
	add	x0, x0, :lo12:sandisk_read_retry_reg_cnt
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC388
	add	x0, x0, :lo12:.LC388
	bl	nand_print
.L1658:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_special_init, .-sandisk_special_init
	.section	.rodata
	.align	3
.LC389:
	.string	" sandisk_special_exit \n"
	.text
	.align	2
	.global	sandisk_special_exit
	.type	sandisk_special_exit, %function
sandisk_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1661
.L1662:
	ldr	x0, [x29, 24]
	bl	sandisk_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1661:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1662
	adrp	x0, .LC389
	add	x0, x0, :lo12:.LC389
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	sandisk_special_exit, .-sandisk_special_exit
	.global	toshiba_read_retry_mode
	.bss
	.type	toshiba_read_retry_mode, %object
	.size	toshiba_read_retry_mode, 1
toshiba_read_retry_mode:
	.zero	1
	.global	toshiba_read_retry_cycle
	.type	toshiba_read_retry_cycle, %object
	.size	toshiba_read_retry_cycle, 1
toshiba_read_retry_cycle:
	.zero	1
	.global	toshiba_read_retry_reg_cnt
	.type	toshiba_read_retry_reg_cnt, %object
	.size	toshiba_read_retry_reg_cnt, 1
toshiba_read_retry_reg_cnt:
	.zero	1
	.global	toshiba_read_retry_start_cmd
	.data
	.align	3
	.type	toshiba_read_retry_start_cmd, %object
	.size	toshiba_read_retry_start_cmd, 2
toshiba_read_retry_start_cmd:
	.byte	92
	.byte	-59
	.global	toshiba_read_retry_end_cmd
	.align	3
	.type	toshiba_read_retry_end_cmd, %object
	.size	toshiba_read_retry_end_cmd, 2
toshiba_read_retry_end_cmd:
	.byte	38
	.byte	93
	.global	toshiba_read_retry_cmd_1
	.align	3
	.type	toshiba_read_retry_cmd_1, %object
	.size	toshiba_read_retry_cmd_1, 4
toshiba_read_retry_cmd_1:
	.byte	85
	.byte	85
	.byte	85
	.byte	85
	.global	toshiba_read_retry_addr_1
	.align	3
	.type	toshiba_read_retry_addr_1, %object
	.size	toshiba_read_retry_addr_1, 4
toshiba_read_retry_addr_1:
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.global	toshiba_p1_1
	.align	3
	.type	toshiba_p1_1, %object
	.size	toshiba_p1_1, 28
toshiba_p1_1:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	124
	.byte	124
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	116
	.byte	116
	.byte	116
	.byte	116
	.byte	8
	.byte	8
	.byte	8
	.byte	8
	.byte	-1
	.byte	-1
	.byte	-1
	.byte	-1
	.global	toshiba_read_retry_cmd_2
	.align	3
	.type	toshiba_read_retry_cmd_2, %object
	.size	toshiba_read_retry_cmd_2, 5
toshiba_read_retry_cmd_2:
	.byte	85
	.byte	85
	.byte	85
	.byte	85
	.byte	85
	.global	toshiba_read_retry_addr_2
	.align	3
	.type	toshiba_read_retry_addr_2, %object
	.size	toshiba_read_retry_addr_2, 5
toshiba_read_retry_addr_2:
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	13
	.global	toshiba_p1_2
	.align	3
	.type	toshiba_p1_2, %object
	.size	toshiba_p1_2, 40
toshiba_p1_2:
	.byte	4
	.byte	4
	.byte	124
	.byte	126
	.byte	0
	.byte	0
	.byte	124
	.byte	120
	.byte	120
	.byte	0
	.byte	124
	.byte	118
	.byte	116
	.byte	114
	.byte	0
	.byte	8
	.byte	8
	.byte	0
	.byte	0
	.byte	0
	.byte	11
	.byte	126
	.byte	118
	.byte	116
	.byte	0
	.byte	16
	.byte	118
	.byte	114
	.byte	112
	.byte	0
	.byte	2
	.byte	0
	.byte	126
	.byte	124
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.global	toshiba_p1_3
	.align	3
	.type	toshiba_p1_3, %object
	.size	toshiba_p1_3, 55
toshiba_p1_3:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	2
	.byte	4
	.byte	2
	.byte	0
	.byte	0
	.byte	124
	.byte	0
	.byte	124
	.byte	124
	.byte	0
	.byte	122
	.byte	0
	.byte	122
	.byte	122
	.byte	0
	.byte	120
	.byte	2
	.byte	120
	.byte	122
	.byte	0
	.byte	126
	.byte	4
	.byte	126
	.byte	122
	.byte	0
	.byte	118
	.byte	4
	.byte	118
	.byte	120
	.byte	0
	.byte	4
	.byte	4
	.byte	4
	.byte	118
	.byte	0
	.byte	6
	.byte	10
	.byte	6
	.byte	2
	.byte	0
	.byte	116
	.byte	124
	.byte	116
	.byte	118
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.global	toshiba_read_retry_last_cmd
	.align	3
	.type	toshiba_read_retry_last_cmd, %object
	.size	toshiba_read_retry_last_cmd, 1
toshiba_read_retry_last_cmd:
	.byte	-77
	.global	toshiba_read_retry_exit_cmd
	.align	3
	.type	toshiba_read_retry_exit_cmd, %object
	.size	toshiba_read_retry_exit_cmd, 1
toshiba_read_retry_exit_cmd:
	.byte	-1
	.local	toshiba_toggle_mode_flag
	.comm	toshiba_toggle_mode_flag,4,4
	.local	toshiba_retry_flag
	.comm	toshiba_retry_flag,4,4
	.local	toshiba_sclk0_bak
	.comm	toshiba_sclk0_bak,4,4
	.local	toshiba_sclk1_bak
	.comm	toshiba_sclk1_bak,4,4
	.text
	.align	2
	.global	toshiba_read_retry_clock_save
	.type	toshiba_read_retry_clock_save, %function
toshiba_read_retry_clock_save:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, toshiba_sclk1_bak
	add	x3, x0, :lo12:toshiba_sclk1_bak
	adrp	x0, toshiba_sclk0_bak
	add	x2, x0, :lo12:toshiba_sclk0_bak
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_get_clk
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	mov	w3, 20
	mov	w2, 10
	bl	nand_set_clk
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	toshiba_read_retry_clock_save, .-toshiba_read_retry_clock_save
	.align	2
	.global	toshiba_read_retry_clock_recover
	.type	toshiba_read_retry_clock_recover, %function
toshiba_read_retry_clock_recover:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, toshiba_sclk0_bak
	add	x0, x0, :lo12:toshiba_sclk0_bak
	ldr	w2, [x0]
	adrp	x0, toshiba_sclk1_bak
	add	x0, x0, :lo12:toshiba_sclk1_bak
	ldr	w3, [x0]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_set_clk
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	toshiba_read_retry_clock_recover, .-toshiba_read_retry_clock_recover
	.section	.rodata
	.align	3
.LC390:
	.string	"toshiba rr value 3 %x %x %x %x!\n"
	.text
	.align	2
	.global	toshiba_vender_set_param
	.type	toshiba_vender_set_param, %function
toshiba_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 60]
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 18
	bne	.L1669
	adrp	x0, toshiba_read_retry_cmd_2
	add	x0, x0, :lo12:toshiba_read_retry_cmd_2
	ldr	w6, [x29, 20]
	mov	w5, 1
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1669
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1669
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1669
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1669
	adrp	x0, toshiba_retry_flag
	add	x0, x0, :lo12:toshiba_retry_flag
	ldr	w0, [x0]
	cmp	w0, 0
	bne	.L1669
	adrp	x0, toshiba_read_retry_exit_cmd
	add	x0, x0, :lo12:toshiba_read_retry_exit_cmd
	ldrb	w0, [x0]
	mov	w2, 1
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	b	.L1670
.L1669:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 17
	bne	.L1671
	adrp	x0, toshiba_read_retry_cmd_2
	add	x0, x0, :lo12:toshiba_read_retry_cmd_2
	ldr	w6, [x29, 20]
	mov	w5, 1
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1672
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1672
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1672
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1672
	adrp	x0, toshiba_read_retry_exit_cmd
	add	x0, x0, :lo12:toshiba_read_retry_exit_cmd
	ldrb	w0, [x0]
	mov	w2, 1
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	b	.L1670
.L1672:
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	cmp	w0, 2
	bne	.L1671
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L1671
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 126
	bne	.L1671
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	cmp	w0, 124
	bne	.L1671
	adrp	x0, toshiba_read_retry_last_cmd
	add	x0, x0, :lo12:toshiba_read_retry_last_cmd
	ldrb	w0, [x0]
	mov	w2, 0
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
.L1671:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 16
	bne	.L1673
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L1674
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L1674
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L1674
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	cmp	w0, 255
	bne	.L1674
	adrp	x0, toshiba_read_retry_exit_cmd
	add	x0, x0, :lo12:toshiba_read_retry_exit_cmd
	ldrb	w0, [x0]
	mov	w2, 1
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	b	.L1670
.L1674:
	adrp	x0, toshiba_read_retry_cmd_1
	add	x0, x0, :lo12:toshiba_read_retry_cmd_1
	ldr	w6, [x29, 20]
	mov	w5, 1
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
.L1673:
	adrp	x0, toshiba_read_retry_end_cmd
	add	x0, x0, :lo12:toshiba_read_retry_end_cmd
	ldrb	w0, [x0]
	mov	w2, 0
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	adrp	x0, toshiba_read_retry_end_cmd
	add	x0, x0, :lo12:toshiba_read_retry_end_cmd
	ldrb	w0, [x0, 1]
	mov	w2, 0
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	mov	w1, w0
	ldr	w0, [x29, 60]
	orr	w0, w0, w1
	str	w0, [x29, 60]
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC390
	add	x0, x0, :lo12:.LC390
	bl	nand_print_dbg
	ldr	w0, [x29, 60]
.L1670:
	ldp	x29, x30, [sp], 64
	ret
	.size	toshiba_vender_set_param, .-toshiba_vender_set_param
	.section	.rodata
	.align	3
.LC391:
	.string	"toshiba vender_pre_condition error!\n"
	.text
	.align	2
	.global	toshiba_vender_pre_condition
	.type	toshiba_vender_pre_condition, %function
toshiba_vender_pre_condition:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, toshiba_read_retry_start_cmd
	add	x0, x0, :lo12:toshiba_read_retry_start_cmd
	mov	w6, 2
	mov	w5, 0
	mov	x4, 0
	mov	x3, 0
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1676
	adrp	x0, .LC391
	add	x0, x0, :lo12:.LC391
	bl	nand_print
	ldr	w0, [x29, 44]
	b	.L1677
.L1676:
	mov	w0, 0
.L1677:
	ldp	x29, x30, [sp], 48
	ret
	.size	toshiba_vender_pre_condition, .-toshiba_vender_pre_condition
	.align	2
	.global	toshiba_set_readretry
	.type	toshiba_set_readretry, %function
toshiba_set_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 16
	beq	.L1679
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 17
	beq	.L1679
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 18
	beq	.L1679
	mov	w0, -89
	b	.L1680
.L1679:
	ldr	x0, [x29, 24]
	bl	toshiba_read_retry_clock_save
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 0
	bne	.L1681
	ldr	x0, [x29, 24]
	bl	toshiba_vender_pre_condition
.L1681:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_is_toggle_interface
	cmp	w0, 0
	beq	.L1682
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_legacy_interface
	adrp	x0, toshiba_toggle_mode_flag
	add	x0, x0, :lo12:toshiba_toggle_mode_flag
	mov	w1, 1
	str	w1, [x0]
.L1682:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 17
	bne	.L1683
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 2
	add	x0, x0, x1
	adrp	x1, toshiba_p1_2
	add	x1, x1, :lo12:toshiba_p1_2
	add	x0, x0, x1
	str	x0, [x29, 56]
	adrp	x0, toshiba_read_retry_addr_2
	add	x0, x0, :lo12:toshiba_read_retry_addr_2
	str	x0, [x29, 48]
	mov	w0, 5
	str	w0, [x29, 44]
	b	.L1684
.L1683:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 18
	bne	.L1685
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x1, w0
	mov	x0, x1
	lsl	x0, x0, 2
	add	x0, x0, x1
	adrp	x1, toshiba_p1_3
	add	x1, x1, :lo12:toshiba_p1_3
	add	x0, x0, x1
	str	x0, [x29, 56]
	adrp	x0, toshiba_read_retry_addr_2
	add	x0, x0, :lo12:toshiba_read_retry_addr_2
	str	x0, [x29, 48]
	mov	w0, 5
	str	w0, [x29, 44]
	b	.L1684
.L1685:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x0, w0
	lsl	x1, x0, 2
	adrp	x0, toshiba_p1_1
	add	x0, x0, :lo12:toshiba_p1_1
	add	x0, x1, x0
	str	x0, [x29, 56]
	adrp	x0, toshiba_read_retry_addr_1
	add	x0, x0, :lo12:toshiba_read_retry_addr_1
	str	x0, [x29, 48]
	mov	w0, 4
	str	w0, [x29, 44]
.L1684:
	ldr	w3, [x29, 44]
	ldr	x2, [x29, 48]
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 24]
	bl	toshiba_vender_set_param
	str	w0, [x29, 40]
	adrp	x0, toshiba_toggle_mode_flag
	add	x0, x0, :lo12:toshiba_toggle_mode_flag
	ldr	w0, [x0]
	cmp	w0, 1
	bne	.L1686
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_toggle_interface
.L1686:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	x0, [x29, 24]
	bl	toshiba_read_retry_clock_recover
	ldr	w0, [x29, 40]
.L1680:
	ldp	x29, x30, [sp], 64
	ret
	.size	toshiba_set_readretry, .-toshiba_set_readretry
	.section	.rodata
	.align	3
.LC392:
	.string	"toshiba retry!\n"
	.align	3
.LC393:
	.string	"toshiba ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	toshiba_read_page_end
	.type	toshiba_read_page_end, %function
toshiba_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1688
	adrp	x0, .LC392
	add	x0, x0, :lo12:.LC392
	bl	nand_print_dbg
	adrp	x0, toshiba_retry_flag
	add	x0, x0, :lo12:toshiba_retry_flag
	mov	w1, 1
	str	w1, [x0]
	str	wzr, [x29, 44]
	b	.L1689
.L1694:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 32]
	bl	toshiba_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1696
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1692
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1691
.L1692:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC393
	add	x0, x0, :lo12:.LC393
	bl	nand_print_dbg
	b	.L1693
.L1696:
	nop
.L1691:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1689:
	adrp	x0, toshiba_read_retry_cycle
	add	x0, x0, :lo12:toshiba_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1694
.L1693:
	adrp	x0, toshiba_retry_flag
	add	x0, x0, :lo12:toshiba_retry_flag
	str	wzr, [x0]
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	toshiba_exit_readretry
.L1688:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	toshiba_read_page_end, .-toshiba_read_page_end
	.section	.rodata
	.align	3
.LC394:
	.string	"toshiba read retry conut: %d !\n"
	.text
	.align	2
	.global	toshiba_readretry_init
	.type	toshiba_readretry_init, %function
toshiba_readretry_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, toshiba_read_retry_cycle
	add	x0, x0, :lo12:toshiba_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, toshiba_read_retry_reg_cnt
	add	x0, x0, :lo12:toshiba_read_retry_reg_cnt
	strb	w1, [x0]
	adrp	x0, toshiba_read_retry_cycle
	add	x0, x0, :lo12:toshiba_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC394
	add	x0, x0, :lo12:.LC394
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	toshiba_readretry_init, .-toshiba_readretry_init
	.align	2
	.global	toshiba_readretry_exit
	.type	toshiba_readretry_exit, %function
toshiba_readretry_exit:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	toshiba_readretry_exit, .-toshiba_readretry_exit
	.align	2
	.global	toshiba_exit_readretry
	.type	toshiba_exit_readretry, %function
toshiba_exit_readretry:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 16
	beq	.L1702
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 17
	beq	.L1702
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 18
	beq	.L1702
	mov	w0, -88
	b	.L1703
.L1702:
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_is_toggle_interface
	cmp	w0, 0
	beq	.L1704
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_legacy_interface
	adrp	x0, toshiba_toggle_mode_flag
	add	x0, x0, :lo12:toshiba_toggle_mode_flag
	mov	w1, 1
	str	w1, [x0]
.L1704:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 17
	bne	.L1705
	adrp	x0, toshiba_p1_2+35
	add	x0, x0, :lo12:toshiba_p1_2+35
	str	x0, [x29, 56]
	adrp	x0, toshiba_read_retry_addr_2
	add	x0, x0, :lo12:toshiba_read_retry_addr_2
	str	x0, [x29, 48]
	mov	w0, 5
	str	w0, [x29, 44]
	b	.L1706
.L1705:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 18
	bne	.L1707
	adrp	x0, toshiba_p1_3+50
	add	x0, x0, :lo12:toshiba_p1_3+50
	str	x0, [x29, 56]
	adrp	x0, toshiba_read_retry_addr_2
	add	x0, x0, :lo12:toshiba_read_retry_addr_2
	str	x0, [x29, 48]
	mov	w0, 5
	str	w0, [x29, 44]
	b	.L1706
.L1707:
	adrp	x0, toshiba_p1_1+24
	add	x0, x0, :lo12:toshiba_p1_1+24
	str	x0, [x29, 56]
	adrp	x0, toshiba_read_retry_addr_1
	add	x0, x0, :lo12:toshiba_read_retry_addr_1
	str	x0, [x29, 48]
	str	wzr, [x29, 44]
.L1706:
	ldr	w3, [x29, 44]
	ldr	x2, [x29, 48]
	ldr	x1, [x29, 56]
	ldr	x0, [x29, 24]
	bl	toshiba_vender_set_param
	str	w0, [x29, 40]
	adrp	x0, toshiba_toggle_mode_flag
	add	x0, x0, :lo12:toshiba_toggle_mode_flag
	ldr	w0, [x0]
	cmp	w0, 1
	bne	.L1708
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_set_toggle_interface
.L1708:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 40]
.L1703:
	ldp	x29, x30, [sp], 64
	ret
	.size	toshiba_exit_readretry, .-toshiba_exit_readretry
	.section	.rodata
	.align	3
.LC395:
	.string	" toshiba_special_init toshiba_read_retry_mode :%d toshiba_read_retry_cycle :%d toshiba_read_retry_reg_cnt :%d \n"
	.align	3
.LC396:
	.string	" toshiba_special_init error toshiba_read_retry_mode :%d toshiba_read_retry_cycle :%d toshiba_read_retry_reg_cnt :%d \n"
	.text
	.align	2
	.global	toshiba_special_init
	.type	toshiba_special_init, %function
toshiba_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1710
.L1711:
	ldr	x0, [x29, 16]
	bl	toshiba_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1710:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1711
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1712
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, toshiba_read_page_end
	add	x1, x1, :lo12:toshiba_read_page_end
	str	x1, [x0]
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, toshiba_read_retry_cycle
	add	x0, x0, :lo12:toshiba_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, toshiba_read_retry_reg_cnt
	add	x0, x0, :lo12:toshiba_read_retry_reg_cnt
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC395
	add	x0, x0, :lo12:.LC395
	bl	nand_print_dbg
	b	.L1713
.L1712:
	adrp	x0, toshiba_read_retry_mode
	add	x0, x0, :lo12:toshiba_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, toshiba_read_retry_cycle
	add	x0, x0, :lo12:toshiba_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, toshiba_read_retry_reg_cnt
	add	x0, x0, :lo12:toshiba_read_retry_reg_cnt
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC396
	add	x0, x0, :lo12:.LC396
	bl	nand_print
.L1713:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	toshiba_special_init, .-toshiba_special_init
	.section	.rodata
	.align	3
.LC397:
	.string	" toshiba_special_exit \n"
	.text
	.align	2
	.global	toshiba_special_exit
	.type	toshiba_special_exit, %function
toshiba_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1716
.L1717:
	ldr	x0, [x29, 24]
	bl	toshiba_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1716:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1717
	adrp	x0, .LC397
	add	x0, x0, :lo12:.LC397
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	toshiba_special_exit, .-toshiba_special_exit
	.global	hynix20nm_read_retry_mode
	.bss
	.type	hynix20nm_read_retry_mode, %object
	.size	hynix20nm_read_retry_mode, 1
hynix20nm_read_retry_mode:
	.zero	1
	.global	hynix20nm_read_retry_cycle
	.type	hynix20nm_read_retry_cycle, %object
	.size	hynix20nm_read_retry_cycle, 1
hynix20nm_read_retry_cycle:
	.zero	1
	.global	hynix20nm_read_retry_reg_num
	.type	hynix20nm_read_retry_reg_num, %object
	.size	hynix20nm_read_retry_reg_num, 1
hynix20nm_read_retry_reg_num:
	.zero	1
	.global	hynix20nm_read_retry_reg_adr
	.align	3
	.type	hynix20nm_read_retry_reg_adr, %object
	.size	hynix20nm_read_retry_reg_adr, 8
hynix20nm_read_retry_reg_adr:
	.zero	8
	.global	hynix20nm_read_retry_reg_adr_1
	.data
	.align	3
	.type	hynix20nm_read_retry_reg_adr_1, %object
	.size	hynix20nm_read_retry_reg_adr_1, 8
hynix20nm_read_retry_reg_adr_1:
	.byte	-52
	.byte	-65
	.byte	-86
	.byte	-85
	.byte	-51
	.byte	-83
	.byte	-82
	.byte	-81
	.global	hynix20nm_read_retry_reg_adr_2
	.align	3
	.type	hynix20nm_read_retry_reg_adr_2, %object
	.size	hynix20nm_read_retry_reg_adr_2, 8
hynix20nm_read_retry_reg_adr_2:
	.byte	-80
	.byte	-79
	.byte	-78
	.byte	-77
	.byte	-76
	.byte	-75
	.byte	-74
	.byte	-73
	.global	hynix20nm_lsb_mode_reg_adr
	.bss
	.align	3
	.type	hynix20nm_lsb_mode_reg_adr, %object
	.size	hynix20nm_lsb_mode_reg_adr, 4
hynix20nm_lsb_mode_reg_adr:
	.zero	4
	.global	hynix20nm_lsb_mode_default_val
	.align	3
	.type	hynix20nm_lsb_mode_default_val, %object
	.size	hynix20nm_lsb_mode_default_val, 4
hynix20nm_lsb_mode_default_val:
	.zero	4
	.global	hynix20nm_lsb_mode_val
	.align	3
	.type	hynix20nm_lsb_mode_val, %object
	.size	hynix20nm_lsb_mode_val, 4
hynix20nm_lsb_mode_val:
	.zero	4
	.global	hynix20nm_lsb_mode_reg_num
	.type	hynix20nm_lsb_mode_reg_num, %object
	.size	hynix20nm_lsb_mode_reg_num, 1
hynix20nm_lsb_mode_reg_num:
	.zero	1
	.text
	.align	2
	.global	hynix20nm_vender_set_param
	.type	hynix20nm_vender_set_param, %function
hynix20nm_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 56]
	mov	w0, 54
	strb	w0, [x29, 54]
	mov	w0, 22
	strb	w0, [x29, 55]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	str	wzr, [x29, 60]
	b	.L1720
.L1721:
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 24]
	add	x2, x1, x0
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 32]
	add	x1, x1, x0
	add	x0, x29, 54
	mov	w6, 1
	mov	w5, 1
	mov	x4, x1
	mov	x3, x2
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w2, 0
	ldrb	w1, [x29, 55]
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1720:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L1721
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	ldr	w0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix20nm_vender_set_param, .-hynix20nm_vender_set_param
	.section	.rodata
	.align	3
.LC398:
	.string	"retry para:"
	.text
	.align	2
	.global	hynix20nm_set_readretry
	.type	hynix20nm_set_readretry, %function
hynix20nm_set_readretry:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	adrp	x1, hynix20nm_read_retry_cycle
	add	x1, x1, :lo12:hynix20nm_read_retry_cycle
	ldrb	w1, [x1]
	cmp	w0, w1
	bls	.L1724
	mov	w0, -92
	b	.L1730
.L1724:
	str	wzr, [x29, 44]
	b	.L1726
.L1727:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	lsl	w1, w0, 3
	ldr	w0, [x29, 44]
	add	w0, w1, w0
	ldr	x1, [x29, 24]
	uxtw	x0, w0
	add	x0, x1, x0
	ldrb	w2, [x0, 56]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1726:
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1727
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x1, x0, :lo12:hynix20nm_read_retry_reg_adr
	add	x0, x29, 32
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix20nm_vender_set_param
	str	w0, [x29, 40]
	adrp	x0, .LC398
	add	x0, x0, :lo12:.LC398
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1728
.L1729:
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	ldrb	w0, [x0, 4080]
	mov	w1, w0
	adrp	x0, .LC351
	add	x0, x0, :lo12:.LC351
	bl	nand_print_dbg
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1728:
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1729
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
	ldr	w0, [x29, 40]
.L1730:
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_set_readretry, .-hynix20nm_set_readretry
	.section	.rodata
	.align	3
.LC399:
	.string	"hynix20nm retry!\n"
	.align	3
.LC400:
	.string	"sdata:0x%x %x %x %x "
	.align	3
.LC401:
	.string	"hynix20nm ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	hynix20nm_read_page_end
	.type	hynix20nm_read_page_end, %function
hynix20nm_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1732
	adrp	x0, .LC399
	add	x0, x0, :lo12:.LC399
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1733
.L1740:
	str	wzr, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 184]
	add	w1, w0, 1
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 184]
	adrp	x1, hynix20nm_read_retry_cycle
	add	x1, x1, :lo12:hynix20nm_read_retry_cycle
	ldrb	w1, [x1]
	cmp	w0, w1
	bls	.L1734
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
.L1734:
	ldr	x0, [x29, 32]
	bl	hynix20nm_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1735
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	hynix20nm_set_readretry
	b	.L1736
.L1735:
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	is_nouse_page
	cmp	w0, 1
	bne	.L1737
	mov	w0, -2
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC365
	add	x0, x0, :lo12:.LC365
	bl	nand_print_dbg
	b	.L1738
.L1737:
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1739
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1738
.L1739:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	add	x0, x0, 4
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC400
	add	x0, x0, :lo12:.LC400
	bl	nand_print_dbg
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC401
	add	x0, x0, :lo12:.LC401
	bl	nand_print_dbg
	b	.L1736
.L1738:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1733:
	adrp	x0, hynix20nm_read_retry_cycle
	add	x0, x0, :lo12:hynix20nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1740
.L1736:
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	hynix20nm_set_readretry
.L1732:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_read_page_end, .-hynix20nm_read_page_end
	.align	2
	.global	hynix20nm_vender_get_param
	.type	hynix20nm_vender_get_param, %function
hynix20nm_vender_get_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 56]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w0, 55
	strb	w0, [x29, 55]
	str	wzr, [x29, 60]
	b	.L1743
.L1744:
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 24]
	add	x2, x1, x0
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 32]
	add	x1, x1, x0
	add	x0, x29, 55
	mov	w4, 1
	mov	x3, x1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	get_data_with_nand_bus_one_cmd
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1743:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L1744
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	ldr	w0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix20nm_vender_get_param, .-hynix20nm_vender_get_param
	.align	2
	.global	hynix20nm_lsb_init
	.type	hynix20nm_lsb_init, %function
hynix20nm_lsb_init:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, hynix20nm_read_retry_cycle
	add	x0, x0, :lo12:hynix20nm_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	strb	w1, [x0]
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 2
	bne	.L1747
	adrp	x0, hynix20nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_num
	mov	w1, 4
	strb	w1, [x0]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -80
	strb	w1, [x0]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -79
	strb	w1, [x0, 1]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -96
	strb	w1, [x0, 2]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -95
	strb	w1, [x0, 3]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0, 1]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0, 2]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0, 3]
	b	.L1748
.L1747:
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 3
	bne	.L1748
	adrp	x0, hynix20nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_num
	mov	w1, 4
	strb	w1, [x0]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -96
	strb	w1, [x0]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -95
	strb	w1, [x0, 1]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -89
	strb	w1, [x0, 2]
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	mov	w1, -88
	strb	w1, [x0, 3]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0, 1]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0, 2]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_val
	mov	w1, 10
	strb	w1, [x0, 3]
.L1748:
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	hynix20nm_lsb_init, .-hynix20nm_lsb_init
	.align	2
	.global	hynix20nm_lsb_enable
	.type	hynix20nm_lsb_enable, %function
hynix20nm_lsb_enable:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, hynix20nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x1, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	adrp	x0, hynix20nm_lsb_mode_default_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_default_val
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix20nm_vender_get_param
	str	wzr, [x29, 44]
	b	.L1751
.L1752:
	adrp	x0, hynix20nm_lsb_mode_default_val
	add	x1, x0, :lo12:hynix20nm_lsb_mode_default_val
	ldr	w0, [x29, 44]
	ldrb	w2, [x1, x0]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4088]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	ldrb	w1, [x0, 4088]
	adrp	x0, hynix20nm_lsb_mode_val
	add	x2, x0, :lo12:hynix20nm_lsb_mode_val
	ldr	w0, [x29, 44]
	ldrb	w0, [x2, x0]
	add	w0, w1, w0
	uxtb	w2, w0
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4088]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1751:
	adrp	x0, hynix20nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1752
	adrp	x0, hynix20nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x1, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	add	x0, x29, 40
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix20nm_vender_set_param
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_lsb_enable, .-hynix20nm_lsb_enable
	.align	2
	.global	hynix20nm_lsb_disable
	.type	hynix20nm_lsb_disable, %function
hynix20nm_lsb_disable:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, hynix20nm_lsb_mode_reg_num
	add	x0, x0, :lo12:hynix20nm_lsb_mode_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_lsb_mode_reg_adr
	add	x1, x0, :lo12:hynix20nm_lsb_mode_reg_adr
	adrp	x0, hynix20nm_lsb_mode_default_val
	add	x0, x0, :lo12:hynix20nm_lsb_mode_default_val
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix20nm_vender_set_param
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix20nm_lsb_disable, .-hynix20nm_lsb_disable
	.section	.rodata
	.align	3
.LC402:
	.string	"hynix20nm_lsb_exit\n"
	.text
	.align	2
	.global	hynix20nm_lsb_exit
	.type	hynix20nm_lsb_exit, %function
hynix20nm_lsb_exit:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w0, 1048575
	str	w0, [x29, 44]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 0
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w2, 1
	mov	w1, 48
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	nop
.L1757:
	ldr	w0, [x29, 44]
	sub	w1, w0, #1
	str	w1, [x29, 44]
	cmp	w0, 0
	bgt	.L1757
	adrp	x0, .LC402
	add	x0, x0, :lo12:.LC402
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_lsb_exit, .-hynix20nm_lsb_exit
	.section	.rodata
	.align	3
.LC403:
	.string	"hynix OTP RegCount value error: 0x%x, 0x%x \n"
	.align	3
.LC404:
	.string	"otp copy %d is ok!\n"
	.align	3
.LC405:
	.string	"otp copy failed!\n"
	.text
	.align	2
	.global	hynix20nm_vender_get_param_otp_hynix
	.type	hynix20nm_vender_get_param_otp_hynix, %function
hynix20nm_vender_get_param_otp_hynix:
	stp	x29, x30, [sp, -144]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 128]
	strb	wzr, [x29, 40]
	strb	wzr, [x29, 41]
	strb	wzr, [x29, 32]
	strb	wzr, [x29, 33]
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 2
	bne	.L1760
	mov	w0, -1
	strb	w0, [x29, 40]
	mov	w0, -52
	strb	w0, [x29, 41]
	mov	w0, 64
	strb	w0, [x29, 32]
	mov	w0, 77
	strb	w0, [x29, 33]
	b	.L1761
.L1760:
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 3
	bne	.L1762
	mov	w0, -82
	strb	w0, [x29, 40]
	mov	w0, -80
	strb	w0, [x29, 41]
	strb	wzr, [x29, 32]
	mov	w0, 77
	strb	w0, [x29, 33]
	b	.L1761
.L1762:
	mov	w0, -97
	b	.L1775
.L1761:
	mov	w0, 1024
	bl	nand_get_temp_buf
	str	x0, [x29, 120]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	mov	w2, 1
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w0, 54
	strb	w0, [x29, 112]
	add	x2, x29, 32
	add	x1, x29, 40
	add	x0, x29, 112
	mov	w6, 1
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	add	x0, x29, 32
	add	x1, x0, 1
	add	x0, x29, 40
	add	x0, x0, 1
	mov	w6, 1
	mov	w5, 1
	mov	x4, x1
	mov	x3, x0
	mov	w2, 0
	mov	x1, 0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	mov	w2, 0
	mov	w1, 22
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 23
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 4
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 25
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 2
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w0, 48
	strb	w0, [x29, 112]
	add	x0, x29, 112
	mov	w4, 2
	ldr	x3, [x29, 120]
	mov	x2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_data_with_nand_bus_one_cmd
	ldr	x0, [x29, 120]
	ldrb	w0, [x0]
	cmp	w0, 8
	bne	.L1764
	ldr	x0, [x29, 120]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 8
	beq	.L1765
.L1764:
	ldr	x0, [x29, 120]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 120]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC403
	add	x0, x0, :lo12:.LC403
	bl	nand_print
	mov	w0, -96
	str	w0, [x29, 128]
.L1765:
	mov	w4, 1024
	ldr	x3, [x29, 120]
	mov	x2, 0
	mov	x1, 0
	ldr	x0, [x29, 24]
	bl	get_data_with_nand_bus_one_cmd
	str	wzr, [x29, 136]
	b	.L1766
.L1773:
	str	wzr, [x29, 132]
	str	wzr, [x29, 140]
	b	.L1767
.L1770:
	ldr	w0, [x29, 136]
	lsl	w0, w0, 7
	uxtw	x1, w0
	ldr	w0, [x29, 140]
	add	x0, x1, x0
	ldr	x1, [x29, 120]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 140]
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 56]
	ldr	w0, [x29, 136]
	lsl	w0, w0, 7
	uxtw	x1, w0
	ldr	w0, [x29, 140]
	add	x0, x1, x0
	add	x0, x0, 64
	ldr	x1, [x29, 120]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	w0, [x29, 140]
	sub	x1, x29, #3952
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4000]
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 140]
	add	x0, x1, x0
	ldrb	w0, [x0, 56]
	mov	w2, w0
	ldr	w0, [x29, 140]
	sub	x1, x29, #3952
	add	x0, x1, x0
	ldrb	w0, [x0, 4000]
	add	w0, w2, w0
	cmp	w0, 255
	beq	.L1768
	mov	w0, 1
	str	w0, [x29, 132]
	b	.L1769
.L1768:
	ldr	w0, [x29, 140]
	add	w0, w0, 1
	str	w0, [x29, 140]
.L1767:
	ldr	w0, [x29, 140]
	cmp	w0, 63
	bls	.L1770
.L1769:
	ldr	w0, [x29, 132]
	cmp	w0, 0
	bne	.L1771
	adrp	x0, .LC404
	add	x0, x0, :lo12:.LC404
	ldr	w1, [x29, 136]
	bl	nand_print_dbg
	b	.L1772
.L1771:
	ldr	w0, [x29, 136]
	add	w0, w0, 1
	str	w0, [x29, 136]
.L1766:
	ldr	w0, [x29, 136]
	cmp	w0, 7
	bls	.L1773
.L1772:
	ldr	w0, [x29, 132]
	cmp	w0, 0
	beq	.L1774
	adrp	x0, .LC405
	add	x0, x0, :lo12:.LC405
	bl	nand_print_dbg
	mov	w0, -95
	str	w0, [x29, 128]
.L1774:
	mov	w2, 1
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 1
	mov	w1, 56
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	x0, [x29, 120]
	bl	nand_free_temp_buf
	ldr	w0, [x29, 128]
.L1775:
	ldp	x29, x30, [sp], 144
	ret
	.size	hynix20nm_vender_get_param_otp_hynix, .-hynix20nm_vender_get_param_otp_hynix
	.align	2
	.global	hynix20nm_saveotpvalue
	.type	hynix20nm_saveotpvalue, %function
hynix20nm_saveotpvalue:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	str	wzr, [sp, 28]
	b	.L1777
.L1778:
	ldr	w0, [sp, 28]
	ldr	x1, [sp]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	x1, [sp, 8]
	ldr	w0, [sp, 28]
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 56]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L1777:
	ldr	w0, [sp, 28]
	cmp	w0, 63
	bls	.L1778
	nop
	add	sp, sp, 32
	ret
	.size	hynix20nm_saveotpvalue, .-hynix20nm_saveotpvalue
	.section	.rodata
	.align	3
.LC406:
	.string	"_vender_get_param_otp_hynix time %d!\n"
	.align	3
.LC407:
	.string	"_vender_get_param_otp_hynix error!\n"
	.text
	.align	2
	.global	hynix20nm_getotpparam
	.type	hynix20nm_getotpparam, %function
hynix20nm_getotpparam:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	str	wzr, [x29, 32]
	b	.L1780
.L1783:
	adrp	x0, .LC406
	add	x0, x0, :lo12:.LC406
	ldr	w1, [x29, 32]
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	hynix20nm_vender_get_param_otp_hynix
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1790
	ldr	w0, [x29, 32]
	add	w0, w0, 1
	str	w0, [x29, 32]
.L1780:
	ldr	w0, [x29, 32]
	cmp	w0, 4
	bls	.L1783
	b	.L1782
.L1790:
	nop
.L1782:
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1784
	adrp	x0, .LC407
	add	x0, x0, :lo12:.LC407
	bl	nand_print
	ldr	w0, [x29, 44]
	b	.L1785
.L1784:
	str	wzr, [x29, 40]
	b	.L1786
.L1789:
	str	wzr, [x29, 36]
	b	.L1787
.L1788:
	ldr	w0, [x29, 40]
	lsl	w1, w0, 3
	ldr	w0, [x29, 36]
	add	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldr	w1, [x29, 40]
	lsl	w2, w1, 3
	ldr	w1, [x29, 36]
	add	w1, w2, w1
	ldr	x2, [x29, 24]
	uxtw	x1, w1
	add	x1, x2, x1
	ldrb	w1, [x1, 56]
	strb	w1, [x0]
	ldr	w0, [x29, 36]
	add	w0, w0, 1
	str	w0, [x29, 36]
.L1787:
	ldr	w0, [x29, 36]
	cmp	w0, 7
	bls	.L1788
	ldr	w0, [x29, 40]
	add	w0, w0, 1
	str	w0, [x29, 40]
.L1786:
	ldr	w0, [x29, 40]
	cmp	w0, 7
	bls	.L1789
	mov	w0, 0
.L1785:
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_getotpparam, .-hynix20nm_getotpparam
	.align	2
	.global	hynix20nm_set_default_param
	.type	hynix20nm_set_default_param, %function
hynix20nm_set_default_param:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 40]
	str	wzr, [x29, 44]
	b	.L1792
.L1793:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x1, x0
	ldrb	w2, [x0, 56]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1792:
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1793
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x1, x0, :lo12:hynix20nm_read_retry_reg_adr
	add	x0, x29, 32
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix20nm_vender_set_param
	str	w0, [x29, 40]
	adrp	x0, .LC355
	add	x0, x0, :lo12:.LC355
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1794
.L1795:
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	ldrb	w0, [x0, 4080]
	mov	w1, w0
	adrp	x0, .LC356
	add	x0, x0, :lo12:.LC356
	bl	nand_print_dbg
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1794:
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1795
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_set_default_param, .-hynix20nm_set_default_param
	.section	.rodata
	.align	3
.LC408:
	.string	"hynix20nm get default param error !\n"
	.align	3
.LC409:
	.string	"[PHY_GetDefaultParam]:readbuf malloc fail\n"
	.align	3
.LC410:
	.string	"ch %d, chip %d Read Retry value Table from uboot:\n"
	.align	3
.LC411:
	.string	"0x%x "
	.align	3
.LC412:
	.string	"[PHY_DBG] ch %d, can't get right otp value from nand otp blocks, then use otp command\n"
	.align	3
.LC413:
	.string	"[PHY_DBG] repair ch %d otp value end\n"
	.text
	.align	2
	.global	hynix20nm_init_readretry_param
	.type	hynix20nm_init_readretry_param, %function
hynix20nm_init_readretry_param:
	stp	x29, x30, [sp, -128]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 124]
	str	xzr, [x29, 112]
	str	xzr, [x29, 104]
	str	wzr, [x29, 120]
	str	wzr, [x29, 100]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 20]
	lsl	w0, w0, 9
	bl	nand_get_temp_buf
	str	x0, [x29, 112]
	ldr	x0, [x29, 112]
	cmp	x0, 0
	bne	.L1798
	adrp	x0, .LC408
	add	x0, x0, :lo12:.LC408
	bl	nand_print_dbg
	mov	w0, -94
	b	.L1807
.L1798:
	mov	w0, 32768
	bl	nand_get_temp_buf
	str	x0, [x29, 104]
	ldr	x0, [x29, 104]
	cmp	x0, 0
	bne	.L1800
	ldr	x0, [x29, 112]
	bl	nand_free_temp_buf
	adrp	x0, .LC409
	add	x0, x0, :lo12:.LC409
	bl	nand_print
	mov	w0, -76
	b	.L1807
.L1800:
	str	wzr, [x29, 100]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 24]
	mov	w3, w0
	mov	w2, 64
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 104]
	bl	hynix16nm_get_special_info
	str	w0, [x29, 100]
	ldr	w0, [x29, 100]
	cmp	w0, 0
	bne	.L1801
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 24]
	adrp	x0, .LC410
	add	x0, x0, :lo12:.LC410
	bl	nand_print_dbg
	str	wzr, [x29, 124]
	b	.L1802
.L1804:
	ldr	w0, [x29, 124]
	ldr	x1, [x29, 112]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC411
	add	x0, x0, :lo12:.LC411
	bl	nand_print_dbg
	ldr	w0, [x29, 124]
	and	w0, w0, 7
	cmp	w0, 7
	bne	.L1803
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
.L1803:
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
.L1802:
	ldr	w0, [x29, 124]
	cmp	w0, 63
	bls	.L1804
	ldr	x1, [x29, 112]
	ldr	x0, [x29, 24]
	bl	hynix20nm_saveotpvalue
	ldr	x0, [x29, 24]
	bl	hynix20nm_set_default_param
	str	w0, [x29, 120]
	b	.L1805
.L1801:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, .LC412
	add	x0, x0, :lo12:.LC412
	bl	nand_print_dbg
	add	x0, x29, 32
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix20nm_getotpparam
	str	w0, [x29, 120]
	ldr	w0, [x29, 120]
	cmp	w0, 0
	bne	.L1808
	ldr	x0, [x29, 24]
	bl	hynix20nm_set_default_param
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, .LC413
	add	x0, x0, :lo12:.LC413
	bl	nand_print_dbg
	b	.L1805
.L1808:
	nop
.L1805:
	ldr	x0, [x29, 112]
	bl	nand_free_temp_buf
	ldr	w0, [x29, 120]
.L1807:
	ldp	x29, x30, [sp], 128
	ret
	.size	hynix20nm_init_readretry_param, .-hynix20nm_init_readretry_param
	.align	2
	.global	hynix20nm_readretry_init
	.type	hynix20nm_readretry_init, %function
hynix20nm_readretry_init:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, hynix20nm_read_retry_cycle
	add	x0, x0, :lo12:hynix20nm_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	strb	w1, [x0]
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 2
	bne	.L1810
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -52
	strb	w1, [x0]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -65
	strb	w1, [x0, 1]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -86
	strb	w1, [x0, 2]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -85
	strb	w1, [x0, 3]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -51
	strb	w1, [x0, 4]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -83
	strb	w1, [x0, 5]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -82
	strb	w1, [x0, 6]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -81
	strb	w1, [x0, 7]
	b	.L1811
.L1810:
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 3
	bne	.L1812
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -80
	strb	w1, [x0]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -79
	strb	w1, [x0, 1]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -78
	strb	w1, [x0, 2]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -77
	strb	w1, [x0, 3]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -76
	strb	w1, [x0, 4]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -75
	strb	w1, [x0, 5]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -74
	strb	w1, [x0, 6]
	adrp	x0, hynix20nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_adr
	mov	w1, -73
	strb	w1, [x0, 7]
	b	.L1811
.L1812:
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC358
	add	x0, x0, :lo12:.LC358
	bl	nand_print
	mov	w0, -93
	b	.L1813
.L1811:
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	str	wzr, [x29, 44]
	b	.L1814
.L1815:
	ldr	x1, [x29, 24]
	ldrsw	x0, [x29, 44]
	add	x0, x1, x0
	strb	wzr, [x0, 56]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1814:
	ldr	w0, [x29, 44]
	cmp	w0, 127
	ble	.L1815
	ldr	x0, [x29, 24]
	bl	hynix20nm_init_readretry_param
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
.L1813:
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix20nm_readretry_init, .-hynix20nm_readretry_init
	.section	.rodata
	.align	3
.LC414:
	.string	"hynix20nm_readretry_exit. \n"
	.text
	.align	2
	.global	hynix20nm_readretry_exit
	.type	hynix20nm_readretry_exit, %function
hynix20nm_readretry_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, .LC414
	add	x0, x0, :lo12:.LC414
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	hynix20nm_set_default_param
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix20nm_readretry_exit, .-hynix20nm_readretry_exit
	.section	.rodata
	.align	3
.LC415:
	.string	" hynix20nm_special_init hynix20nm_read_retry_mode: %d hynix20nm_read_retry_cycle:%d hynix20nm_read_retry_reg_num:%d\n"
	.align	3
.LC416:
	.string	" hynix20nm_special_init error hynix20nm_read_retry_mode: %d hynix20nm_read_retry_cycle:%d hynix20nm_read_retry_reg_num:%d\n"
	.text
	.align	2
	.global	hynix20nm_special_init
	.type	hynix20nm_special_init, %function
hynix20nm_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1819
.L1820:
	ldr	x0, [x29, 16]
	bl	hynix20nm_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1819:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1820
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1821
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, hynix20nm_read_page_end
	add	x1, x1, :lo12:hynix20nm_read_page_end
	str	x1, [x0]
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, hynix20nm_read_retry_cycle
	add	x0, x0, :lo12:hynix20nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC415
	add	x0, x0, :lo12:.LC415
	bl	nand_print_dbg
	b	.L1822
.L1821:
	adrp	x0, hynix20nm_read_retry_mode
	add	x0, x0, :lo12:hynix20nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, hynix20nm_read_retry_cycle
	add	x0, x0, :lo12:hynix20nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix20nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix20nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC416
	add	x0, x0, :lo12:.LC416
	bl	nand_print
.L1822:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix20nm_special_init, .-hynix20nm_special_init
	.section	.rodata
	.align	3
.LC417:
	.string	" hynix20nm_special_exit \n"
	.text
	.align	2
	.global	hynix20nm_special_exit
	.type	hynix20nm_special_exit, %function
hynix20nm_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1825
.L1826:
	ldr	x0, [x29, 24]
	bl	hynix20nm_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1825:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1826
	adrp	x0, .LC417
	add	x0, x0, :lo12:.LC417
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix20nm_special_exit, .-hynix20nm_special_exit
	.global	hynix16nm_read_retry_mode
	.bss
	.type	hynix16nm_read_retry_mode, %object
	.size	hynix16nm_read_retry_mode, 1
hynix16nm_read_retry_mode:
	.zero	1
	.global	hynix16nm_read_retry_cycle
	.type	hynix16nm_read_retry_cycle, %object
	.size	hynix16nm_read_retry_cycle, 1
hynix16nm_read_retry_cycle:
	.zero	1
	.global	hynix16nm_read_retry_reg_num
	.type	hynix16nm_read_retry_reg_num, %object
	.size	hynix16nm_read_retry_reg_num, 1
hynix16nm_read_retry_reg_num:
	.zero	1
	.global	hynix16nm_read_retry_reg_adr
	.align	3
	.type	hynix16nm_read_retry_reg_adr, %object
	.size	hynix16nm_read_retry_reg_adr, 4
hynix16nm_read_retry_reg_adr:
	.zero	4
	.text
	.align	2
	.global	hynix16nm_vender_set_param
	.type	hynix16nm_vender_set_param, %function
hynix16nm_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 56]
	mov	w0, 54
	strb	w0, [x29, 54]
	mov	w0, 22
	strb	w0, [x29, 55]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	str	wzr, [x29, 60]
	b	.L1829
.L1830:
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 24]
	add	x2, x1, x0
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 32]
	add	x1, x1, x0
	add	x0, x29, 54
	mov	w6, 1
	mov	w5, 1
	mov	x4, x1
	mov	x3, x2
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	mov	w2, 0
	ldrb	w1, [x29, 55]
	ldr	x0, [x29, 40]
	bl	set_one_cmd
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1829:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L1830
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC349
	add	x0, x0, :lo12:.LC349
	bl	nand_print_dbg
	ldr	w0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix16nm_vender_set_param, .-hynix16nm_vender_set_param
	.align	2
	.global	hynix16nm_set_readretry
	.type	hynix16nm_set_readretry, %function
hynix16nm_set_readretry:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 40]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	adrp	x1, hynix16nm_read_retry_cycle
	add	x1, x1, :lo12:hynix16nm_read_retry_cycle
	ldrb	w1, [x1]
	cmp	w0, w1
	bls	.L1833
	mov	w0, -74
	b	.L1837
.L1833:
	str	wzr, [x29, 44]
	b	.L1835
.L1836:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	lsl	w1, w0, 2
	ldr	w0, [x29, 44]
	add	w0, w1, w0
	ldr	x1, [x29, 24]
	uxtw	x0, w0
	add	x0, x1, x0
	ldrb	w2, [x0, 56]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1835:
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1836
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix16nm_read_retry_reg_adr
	add	x1, x0, :lo12:hynix16nm_read_retry_reg_adr
	add	x0, x29, 32
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix16nm_vender_set_param
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
.L1837:
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix16nm_set_readretry, .-hynix16nm_set_readretry
	.section	.rodata
	.align	3
.LC418:
	.string	"get_hynix_special_info start!!\n"
	.align	3
.LC419:
	.string	"hynix_special_info:can't find uboot head\n"
	.align	3
.LC420:
	.string	"physic info start_block %d\n"
	.text
	.align	2
	.global	hynix16nm_get_special_info
	.type	hynix16nm_get_special_info, %function
hynix16nm_get_special_info:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	w2, [x29, 28]
	str	w3, [x29, 24]
	adrp	x0, .LC418
	add	x0, x0, :lo12:.LC418
	bl	nand_print_dbg
	add	x0, x29, 60
	bl	physic_info_get_offset
	str	w0, [x29, 76]
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L1839
	adrp	x0, .LC419
	add	x0, x0, :lo12:.LC419
	bl	nand_print
	mov	w0, 1
	b	.L1849
.L1839:
	mov	w0, -1
	str	w0, [x29, 88]
	mov	w0, 4
	str	w0, [x29, 84]
	b	.L1841
.L1848:
	adrp	x0, .LC420
	add	x0, x0, :lo12:.LC420
	ldr	w1, [x29, 84]
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	add	x0, x0, 8192
	ldrb	w0, [x0, 1536]
	cmp	w0, 165
	beq	.L1842
	ldr	w0, [x29, 60]
	add	x1, x29, 56
	ldr	x3, [x29, 40]
	mov	x2, x1
	mov	w1, w0
	ldr	w0, [x29, 84]
	bl	physic_info_get_one_copy
.L1842:
	ldr	x0, [x29, 40]
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	add	x0, x0, 8192
	ldrb	w0, [x0, 1536]
	cmp	w0, 165
	bne	.L1843
	strb	wzr, [x29, 83]
	mov	w0, 2
	str	w0, [x29, 92]
	b	.L1844
.L1845:
	ldr	x1, [x29, 64]
	ldr	w0, [x29, 92]
	add	x0, x1, x0
	add	x0, x0, 8192
	ldrb	w1, [x0, 1536]
	ldrb	w0, [x29, 83]
	add	w0, w1, w0
	strb	w0, [x29, 83]
	ldr	w0, [x29, 92]
	add	w0, w0, 1
	str	w0, [x29, 92]
.L1844:
	ldr	w0, [x29, 92]
	cmp	w0, 1023
	bls	.L1845
	ldr	x0, [x29, 64]
	add	x0, x0, 8192
	ldrb	w0, [x0, 1537]
	ldrb	w1, [x29, 83]
	cmp	w1, w0
	bne	.L1846
	str	wzr, [x29, 88]
	ldr	w1, [x29, 24]
	ldr	w0, [x29, 28]
	mul	w0, w1, w0
	add	w0, w0, 2
	uxtw	x0, w0
	add	x0, x0, 8192
	add	x0, x0, 1536
	ldr	x1, [x29, 64]
	add	x0, x1, x0
	ldr	w2, [x29, 28]
	mov	x1, x0
	ldr	x0, [x29, 32]
	bl	nand_memcpy
	b	.L1847
.L1846:
	ldr	x0, [x29, 64]
	add	x0, x0, 8192
	strb	wzr, [x0, 1536]
.L1843:
	ldr	w0, [x29, 84]
	add	w0, w0, 1
	str	w0, [x29, 84]
.L1841:
	ldr	w0, [x29, 84]
	cmp	w0, 50
	bls	.L1848
.L1847:
	ldr	w0, [x29, 88]
.L1849:
	ldp	x29, x30, [sp], 96
	ret
	.size	hynix16nm_get_special_info, .-hynix16nm_get_special_info
	.section	.rodata
	.align	3
.LC421:
	.string	"m1 retry!\n"
	.align	3
.LC422:
	.string	"m1 ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	hynix16nm_read_page_end
	.type	hynix16nm_read_page_end, %function
hynix16nm_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1851
	adrp	x0, .LC421
	add	x0, x0, :lo12:.LC421
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1852
.L1858:
	str	wzr, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 184]
	add	w1, w0, 1
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	ldr	x0, [x29, 32]
	ldr	w0, [x0, 184]
	adrp	x1, hynix16nm_read_retry_cycle
	add	x1, x1, :lo12:hynix16nm_read_retry_cycle
	ldrb	w1, [x1]
	cmp	w0, w1
	bls	.L1853
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
.L1853:
	ldr	x0, [x29, 32]
	bl	hynix16nm_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	beq	.L1854
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	hynix16nm_set_readretry
	b	.L1855
.L1854:
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1856
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1857
.L1856:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC422
	add	x0, x0, :lo12:.LC422
	bl	nand_print_dbg
	b	.L1855
.L1857:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1852:
	adrp	x0, hynix16nm_read_retry_cycle
	add	x0, x0, :lo12:hynix16nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1858
.L1855:
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	hynix16nm_set_readretry
.L1851:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix16nm_read_page_end, .-hynix16nm_read_page_end
	.section	.rodata
	.align	3
.LC423:
	.string	"m1 write page FF wrong\n"
	.text
	.align	2
	.global	hynix16nm_write_page_FF
	.type	hynix16nm_write_page_FF, %function
hynix16nm_write_page_FF:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	wzr, [x29, 76]
	str	wzr, [x29, 72]
	str	wzr, [x29, 68]
	str	wzr, [x29, 56]
	str	wzr, [x29, 64]
	str	wzr, [x29, 60]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	str	x0, [x29, 40]
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	add	x0, x0, 40
	str	x0, [x29, 32]
	ldr	x0, [x29, 48]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	w0, [x0, 16]
	cmp	w1, w0
	bcs	.L1861
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 4]
	ldr	x0, [x29, 48]
	ldr	w0, [x0, 32]
	cmp	w1, w0
	bcc	.L1862
.L1861:
	ldr	x0, [x29, 40]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 48]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 40]
	ldr	w3, [x0, 16]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 4]
	ldr	x0, [x29, 48]
	ldr	w5, [x0, 32]
	adrp	x0, .LC374
	add	x0, x0, :lo12:.LC374
	bl	nand_print
	mov	w0, -108
	b	.L1863
.L1862:
	ldr	x0, [x29, 48]
	bl	nand_read_chip_status_ready
	ldr	x0, [x29, 48]
	bl	nand_enable_chip
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_ecc
	ldr	x0, [x29, 32]
	bl	ndfc_clean_cmd_seq
	ldr	w0, [x29, 20]
	cmp	w0, 8
	bne	.L1864
	mov	w0, 9
	str	w0, [x29, 72]
	mov	w0, 832
	str	w0, [x29, 68]
	b	.L1865
.L1864:
	ldr	w0, [x29, 20]
	cmp	w0, 16
	bne	.L1865
	mov	w0, 18
	str	w0, [x29, 72]
	mov	w0, 640
	str	w0, [x29, 68]
.L1865:
	str	wzr, [x29, 76]
	b	.L1866
.L1870:
	ldr	w0, [x29, 76]
	cmp	w0, 0
	bne	.L1867
	ldr	x0, [x29, 32]
	str	wzr, [x0]
	ldr	x0, [x29, 32]
	mov	w1, 128
	str	w1, [x0, 12]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 16]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 56]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 48]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 52]
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 16]
	ldr	x0, [x29, 32]
	str	x1, [x0, 72]
	ldr	x0, [x29, 32]
	mov	w1, 1024
	str	w1, [x0, 60]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 44]
	ldr	x0, [x29, 48]
	ldr	w3, [x0, 44]
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 8]
	mov	w2, w0
	mov	w0, w3
	bl	get_row_addr
	str	w0, [x29, 64]
	ldr	x0, [x29, 32]
	mov	w1, 5
	str	w1, [x0, 40]
	ldr	x0, [x29, 32]
	add	x0, x0, 24
	mov	x4, x0
	mov	w3, 3
	ldr	w2, [x29, 64]
	mov	w1, 2
	ldr	w0, [x29, 60]
	bl	fill_cmd_addr
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 32]
	bl	ndfc_execute_cmd
	str	w0, [x29, 56]
	b	.L1868
.L1867:
	ldr	w0, [x29, 72]
	sub	w1, w0, #1
	ldr	w0, [x29, 76]
	cmp	w1, w0
	bne	.L1869
	ldr	x0, [x29, 32]
	str	wzr, [x0]
	ldr	x0, [x29, 32]
	mov	w1, 133
	str	w1, [x0, 12]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 16]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 56]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 48]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 52]
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 16]
	ldr	w0, [x29, 76]
	lsl	w0, w0, 10
	uxtw	x0, w0
	add	x1, x1, x0
	ldr	x0, [x29, 32]
	str	x1, [x0, 72]
	ldr	x0, [x29, 32]
	ldr	w1, [x29, 68]
	str	w1, [x0, 60]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 44]
	ldr	x0, [x29, 32]
	mov	w1, 2
	str	w1, [x0, 40]
	ldr	x0, [x29, 32]
	add	x0, x0, 24
	mov	x4, x0
	mov	w3, 0
	ldr	w2, [x29, 64]
	mov	w1, 2
	ldr	w0, [x29, 60]
	bl	fill_cmd_addr
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 32]
	bl	ndfc_execute_cmd
	str	w0, [x29, 56]
	b	.L1868
.L1869:
	ldr	x0, [x29, 32]
	str	wzr, [x0]
	ldr	x0, [x29, 32]
	mov	w1, 133
	str	w1, [x0, 12]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 16]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 56]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 48]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 52]
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 16]
	ldr	w0, [x29, 76]
	lsl	w0, w0, 10
	uxtw	x0, w0
	add	x1, x1, x0
	ldr	x0, [x29, 32]
	str	x1, [x0, 72]
	ldr	x0, [x29, 32]
	mov	w1, 1024
	str	w1, [x0, 60]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 44]
	ldr	x0, [x29, 32]
	mov	w1, 2
	str	w1, [x0, 40]
	ldr	x0, [x29, 32]
	add	x0, x0, 24
	mov	x4, x0
	mov	w3, 0
	ldr	w2, [x29, 64]
	mov	w1, 2
	ldr	w0, [x29, 60]
	bl	fill_cmd_addr
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 32]
	bl	ndfc_execute_cmd
	str	w0, [x29, 56]
.L1868:
	ldr	w0, [x29, 60]
	add	w0, w0, 1024
	str	w0, [x29, 60]
	ldr	w0, [x29, 76]
	add	w0, w0, 1
	str	w0, [x29, 76]
.L1866:
	ldr	w1, [x29, 76]
	ldr	w0, [x29, 72]
	cmp	w1, w0
	bcc	.L1870
	ldr	x0, [x29, 32]
	str	wzr, [x0]
	ldr	x0, [x29, 32]
	mov	w1, 16
	str	w1, [x0, 12]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 8]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 16]
	ldr	x0, [x29, 32]
	mov	w1, 1
	str	w1, [x0, 92]
	ldr	x0, [x29, 48]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 32]
	bl	ndfc_execute_cmd
	str	w0, [x29, 56]
	ldr	x0, [x29, 48]
	bl	nand_read_chip_status_ready
	str	w0, [x29, 56]
	ldr	w0, [x29, 56]
	cmp	w0, 0
	beq	.L1871
	adrp	x0, .LC423
	add	x0, x0, :lo12:.LC423
	bl	nand_print
.L1871:
	ldr	x0, [x29, 48]
	bl	nand_disable_chip
	ldr	w0, [x29, 56]
.L1863:
	ldp	x29, x30, [sp], 80
	ret
	.size	hynix16nm_write_page_FF, .-hynix16nm_write_page_FF
	.align	2
	.global	hynix16nm_vender_get_param
	.type	hynix16nm_vender_get_param, %function
hynix16nm_vender_get_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	str	wzr, [x29, 56]
	mov	w0, 55
	strb	w0, [x29, 55]
	ldr	x0, [x29, 40]
	bl	nand_enable_chip
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	str	wzr, [x29, 60]
	b	.L1873
.L1874:
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 24]
	add	x2, x1, x0
	ldr	w0, [x29, 60]
	ldr	x1, [x29, 32]
	add	x1, x1, x0
	add	x0, x29, 55
	mov	w4, 1
	mov	x3, x1
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	get_data_with_nand_bus_one_cmd
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1873:
	ldr	w1, [x29, 60]
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bcc	.L1874
	ldr	x0, [x29, 40]
	bl	nand_disable_chip
	ldr	w0, [x29, 56]
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix16nm_vender_get_param, .-hynix16nm_vender_get_param
	.section	.rodata
	.align	3
.LC424:
	.string	"_major_check_byte, wrong input para, level %d, len %d\n"
	.align	3
.LC425:
	.string	"%d:  '1'-'0' : %d - %d\n"
	.align	3
.LC426:
	.string	"get bit %d failed!\n"
	.text
	.align	2
	.global	hynix16nm_major_check_byte
	.type	hynix16nm_major_check_byte, %function
hynix16nm_major_check_byte:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	str	x0, [x29, 56]
	str	w1, [x29, 52]
	str	w2, [x29, 48]
	str	x3, [x29, 40]
	str	x4, [x29, 32]
	str	w5, [x29, 28]
	strb	wzr, [x29, 71]
	ldr	w0, [x29, 28]
	lsr	w1, w0, 1
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bls	.L1877
	adrp	x0, .LC424
	add	x0, x0, :lo12:.LC424
	ldr	w2, [x29, 28]
	ldr	w1, [x29, 48]
	bl	nand_print
	ldr	x0, [x29, 56]
	mov	w1, -1
	strb	w1, [x0]
	mov	w0, -107
	b	.L1878
.L1877:
	str	wzr, [x29, 72]
	str	wzr, [x29, 92]
	b	.L1879
.L1894:
	str	wzr, [x29, 84]
	str	wzr, [x29, 80]
	str	wzr, [x29, 76]
	str	wzr, [x29, 88]
	b	.L1880
.L1883:
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 40]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 92]
	lsr	w0, w1, w0
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L1881
	ldr	w0, [x29, 84]
	add	w0, w0, 1
	str	w0, [x29, 84]
	b	.L1882
.L1881:
	ldr	w0, [x29, 80]
	add	w0, w0, 1
	str	w0, [x29, 80]
.L1882:
	ldr	w0, [x29, 88]
	add	w0, w0, 1
	str	w0, [x29, 88]
.L1880:
	ldr	w1, [x29, 88]
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bcc	.L1883
	ldr	w1, [x29, 84]
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bls	.L1884
	ldr	w0, [x29, 92]
	mov	w1, 1
	lsl	w0, w1, w0
	uxtb	w1, w0
	ldrb	w0, [x29, 71]
	orr	w0, w1, w0
	strb	w0, [x29, 71]
	mov	w0, 1
	str	w0, [x29, 76]
.L1884:
	ldr	w1, [x29, 80]
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bls	.L1885
	mov	w0, 1
	str	w0, [x29, 76]
.L1885:
	ldr	w0, [x29, 76]
	cmp	w0, 0
	bne	.L1886
	ldr	w0, [x29, 52]
	cmp	w0, 1
	bne	.L1886
	str	wzr, [x29, 84]
	str	wzr, [x29, 80]
	str	wzr, [x29, 76]
	str	wzr, [x29, 88]
	b	.L1887
.L1890:
	ldr	w0, [x29, 88]
	ldr	x1, [x29, 32]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 92]
	lsr	w0, w1, w0
	and	w0, w0, 1
	cmp	w0, 0
	beq	.L1888
	ldr	w0, [x29, 80]
	add	w0, w0, 1
	str	w0, [x29, 80]
	b	.L1889
.L1888:
	ldr	w0, [x29, 84]
	add	w0, w0, 1
	str	w0, [x29, 84]
.L1889:
	ldr	w0, [x29, 88]
	add	w0, w0, 1
	str	w0, [x29, 88]
.L1887:
	ldr	w1, [x29, 88]
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bcc	.L1890
	ldr	w1, [x29, 80]
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bls	.L1891
	mov	w0, 1
	str	w0, [x29, 76]
.L1891:
	ldr	w1, [x29, 84]
	ldr	w0, [x29, 48]
	cmp	w1, w0
	bls	.L1886
	ldr	w0, [x29, 92]
	mov	w1, 1
	lsl	w0, w1, w0
	uxtb	w1, w0
	ldrb	w0, [x29, 71]
	orr	w0, w1, w0
	strb	w0, [x29, 71]
	mov	w0, 1
	str	w0, [x29, 76]
.L1886:
	ldr	w0, [x29, 76]
	cmp	w0, 0
	beq	.L1892
	ldr	w0, [x29, 72]
	add	w0, w0, 1
	str	w0, [x29, 72]
	b	.L1893
.L1892:
	adrp	x0, .LC425
	add	x0, x0, :lo12:.LC425
	ldr	w3, [x29, 80]
	ldr	w2, [x29, 84]
	ldr	w1, [x29, 92]
	bl	nand_print
	adrp	x0, .LC426
	add	x0, x0, :lo12:.LC426
	ldr	w1, [x29, 92]
	bl	nand_print
.L1893:
	ldr	w0, [x29, 92]
	add	w0, w0, 1
	str	w0, [x29, 92]
.L1879:
	ldr	w0, [x29, 92]
	cmp	w0, 7
	bls	.L1894
	ldr	w0, [x29, 72]
	cmp	w0, 8
	bne	.L1895
	ldr	x0, [x29, 56]
	ldrb	w1, [x29, 71]
	strb	w1, [x0]
	mov	w0, 0
	b	.L1878
.L1895:
	ldr	x0, [x29, 56]
	mov	w1, -1
	strb	w1, [x0]
	mov	w0, -80
.L1878:
	ldp	x29, x30, [sp], 96
	ret
	.size	hynix16nm_major_check_byte, .-hynix16nm_major_check_byte
	.section	.rodata
	.align	3
.LC427:
	.string	"_get_read_retry_parameters, get rr count failed!\n"
	.align	3
.LC428:
	.string	"rr cnt: %d\n"
	.align	3
.LC429:
	.string	"_get_read_retry_parameters, get rr reg count failed!\n"
	.align	3
.LC430:
	.string	"rr reg cnt: %d\n"
	.align	3
.LC431:
	.string	"read retry value from otp error: rr_cnt %d rr_reg_cnt %d!\n"
	.align	3
.LC432:
	.string	"_get_read_retry_parameters, get the %d-th byte of rr table failed!\n"
	.text
	.align	2
	.global	hynix16nm_get_read_retry_cfg
	.type	hynix16nm_get_read_retry_cfg, %function
hynix16nm_get_read_retry_cfg:
	stp	x29, x30, [sp, -144]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	x3, [x29, 16]
	str	wzr, [x29, 140]
	str	wzr, [x29, 136]
	add	x0, x29, 88
	stp	xzr, xzr, [x0]
	add	x0, x29, 104
	stp	xzr, xzr, [x0]
	add	x0, x29, 56
	stp	xzr, xzr, [x0]
	add	x0, x29, 72
	stp	xzr, xzr, [x0]
	mov	w0, 32
	str	w0, [x29, 120]
	str	wzr, [x29, 132]
	b	.L1897
.L1898:
	ldr	w0, [x29, 132]
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	w0, [x29, 132]
	sub	x1, x29, #3952
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4040]
	ldr	w0, [x29, 132]
	add	w0, w0, 1
	str	w0, [x29, 132]
.L1897:
	ldr	w0, [x29, 132]
	cmp	w0, 7
	bls	.L1898
	add	x1, x29, 56
	add	x0, x29, 88
	mov	w5, 8
	mov	x4, x1
	mov	x3, x0
	mov	w2, 4
	mov	w1, 0
	ldr	x0, [x29, 40]
	bl	hynix16nm_major_check_byte
	str	w0, [x29, 136]
	ldr	w0, [x29, 136]
	cmp	w0, 0
	bge	.L1899
	adrp	x0, .LC427
	add	x0, x0, :lo12:.LC427
	bl	nand_print
	ldr	w0, [x29, 136]
	b	.L1917
.L1899:
	ldr	x0, [x29, 40]
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC428
	add	x0, x0, :lo12:.LC428
	bl	nand_print_dbg
	str	wzr, [x29, 132]
	b	.L1901
.L1902:
	ldr	w0, [x29, 132]
	add	w0, w0, 8
	uxtw	x0, w0
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	w0, [x29, 132]
	sub	x1, x29, #3952
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4040]
	ldr	w0, [x29, 132]
	add	w0, w0, 1
	str	w0, [x29, 132]
.L1901:
	ldr	w0, [x29, 132]
	cmp	w0, 7
	bls	.L1902
	add	x1, x29, 56
	add	x0, x29, 88
	mov	w5, 8
	mov	x4, x1
	mov	x3, x0
	mov	w2, 4
	mov	w1, 0
	ldr	x0, [x29, 32]
	bl	hynix16nm_major_check_byte
	str	w0, [x29, 136]
	ldr	w0, [x29, 136]
	cmp	w0, 0
	bge	.L1903
	adrp	x0, .LC429
	add	x0, x0, :lo12:.LC429
	bl	nand_print
	ldr	w0, [x29, 136]
	b	.L1917
.L1903:
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC430
	add	x0, x0, :lo12:.LC430
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
	ldrb	w0, [x0]
	cmp	w0, 8
	bne	.L1904
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	cmp	w0, 4
	beq	.L1905
.L1904:
	ldr	x0, [x29, 40]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC431
	add	x0, x0, :lo12:.LC431
	bl	nand_print
	mov	w0, -81
	b	.L1917
.L1905:
	str	wzr, [x29, 128]
	b	.L1906
.L1911:
	str	wzr, [x29, 124]
	b	.L1907
.L1908:
	ldr	w0, [x29, 124]
	sub	x1, x29, #3952
	add	x0, x1, x0
	strb	wzr, [x0, 4040]
	ldr	w0, [x29, 124]
	sub	x1, x29, #3952
	add	x0, x1, x0
	strb	wzr, [x0, 4008]
	ldr	w1, [x29, 124]
	ldr	w0, [x29, 120]
	mul	w0, w1, w0
	add	w0, w0, 8
	lsl	w1, w0, 1
	ldr	w0, [x29, 128]
	add	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	w0, [x29, 124]
	sub	x1, x29, #3952
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4040]
	ldr	w1, [x29, 124]
	ldr	w0, [x29, 120]
	mul	w0, w1, w0
	add	w0, w0, 8
	lsl	w1, w0, 1
	ldr	w0, [x29, 120]
	add	w1, w1, w0
	ldr	w0, [x29, 128]
	add	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	w0, [x29, 124]
	sub	x1, x29, #3952
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4008]
	ldr	w0, [x29, 124]
	add	w0, w0, 1
	str	w0, [x29, 124]
.L1907:
	ldr	w0, [x29, 124]
	cmp	w0, 7
	bls	.L1908
	ldr	w0, [x29, 128]
	ldr	x1, [x29, 24]
	add	x0, x1, x0
	add	x2, x29, 56
	add	x1, x29, 88
	mov	w5, 8
	mov	x4, x2
	mov	x3, x1
	mov	w2, 4
	mov	w1, 1
	bl	hynix16nm_major_check_byte
	str	w0, [x29, 136]
	ldr	w0, [x29, 136]
	cmp	w0, 0
	bge	.L1909
	adrp	x0, .LC432
	add	x0, x0, :lo12:.LC432
	ldr	w1, [x29, 128]
	bl	nand_print
	ldr	w0, [x29, 136]
	str	w0, [x29, 140]
	b	.L1910
.L1909:
	ldr	w0, [x29, 128]
	add	w0, w0, 1
	str	w0, [x29, 128]
.L1906:
	ldr	w1, [x29, 128]
	ldr	w0, [x29, 120]
	cmp	w1, w0
	bcc	.L1911
.L1910:
	str	wzr, [x29, 128]
	b	.L1912
.L1914:
	ldr	w0, [x29, 128]
	and	w0, w0, 7
	cmp	w0, 0
	bne	.L1913
	ldr	w0, [x29, 128]
	cmp	w0, 0
	beq	.L1913
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
.L1913:
	ldr	w0, [x29, 128]
	ldr	x1, [x29, 24]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC126
	add	x0, x0, :lo12:.LC126
	bl	nand_print_dbg
	ldr	w0, [x29, 128]
	add	w0, w0, 1
	str	w0, [x29, 128]
.L1912:
	ldr	w1, [x29, 128]
	ldr	w0, [x29, 120]
	cmp	w1, w0
	bcc	.L1914
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
	ldr	w0, [x29, 140]
	cmp	w0, 0
	beq	.L1915
	ldr	w0, [x29, 140]
	str	w0, [x29, 136]
	b	.L1916
.L1915:
	str	wzr, [x29, 136]
.L1916:
	ldr	w0, [x29, 136]
.L1917:
	ldp	x29, x30, [sp], 144
	ret
	.size	hynix16nm_get_read_retry_cfg, .-hynix16nm_get_read_retry_cfg
	.section	.rodata
	.align	3
.LC433:
	.string	"start get read retry param from: ce %d, rb %d...\n"
	.align	3
.LC434:
	.string	"invalid buffer for otp info!\n"
	.text
	.align	2
	.global	hynix16nm_read_otp_info_hynix
	.type	hynix16nm_read_otp_info_hynix, %function
hynix16nm_read_otp_info_hynix:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 24]
	ldr	w1, [x1, 28]
	uxtw	x1, w1
	add	x1, x1, 316
	ldr	w3, [x0, x1, lsl 2]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	x1, [x29, 24]
	ldr	w1, [x1, 28]
	uxtw	x1, w1
	add	x1, x1, 324
	ldr	w1, [x0, x1, lsl 2]
	adrp	x0, .LC433
	add	x0, x0, :lo12:.LC433
	mov	w2, w1
	mov	w1, w3
	bl	nand_print_dbg
	ldr	x0, [x29, 16]
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	cmp	x0, 0
	bne	.L1919
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	adrp	x0, .LC434
	add	x0, x0, :lo12:.LC434
	bl	nand_print
	mov	w0, -79
	b	.L1923
.L1919:
	mov	w2, 1
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w0, 54
	strb	w0, [x29, 48]
	mov	w0, 56
	strb	w0, [x29, 56]
	mov	w0, 82
	strb	w0, [x29, 40]
	add	x2, x29, 40
	add	x1, x29, 56
	add	x0, x29, 48
	mov	w6, 1
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	mov	w2, 0
	mov	w1, 22
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 23
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 4
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 25
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 2
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w0, 48
	strb	w0, [x29, 48]
	add	x0, x29, 48
	mov	w4, 528
	ldr	x3, [x29, 72]
	mov	x2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	get_data_with_nand_bus_one_cmd
	mov	w2, 1
	mov	w1, -1
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w0, 54
	strb	w0, [x29, 48]
	mov	w0, 56
	strb	w0, [x29, 56]
	strb	wzr, [x29, 40]
	add	x2, x29, 40
	add	x1, x29, 56
	add	x0, x29, 48
	mov	w6, 1
	mov	w5, 1
	mov	x4, x2
	mov	x3, x1
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	set_cmd_with_nand_bus
	mov	w2, 0
	mov	w1, 22
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w2, 0
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w1, 0
	ldr	x0, [x29, 24]
	bl	set_one_addr
	mov	w2, 1
	mov	w1, 48
	ldr	x0, [x29, 24]
	bl	set_one_cmd
	ldr	x0, [x29, 72]
	ldrb	w0, [x0]
	cmp	w0, 8
	bne	.L1921
	ldr	x0, [x29, 72]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 8
	beq	.L1922
.L1921:
	ldr	x0, [x29, 72]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 72]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC403
	add	x0, x0, :lo12:.LC403
	bl	nand_print
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	mov	w0, -96
	str	w0, [x29, 68]
	ldr	w0, [x29, 68]
	b	.L1923
.L1922:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	mov	w0, 0
.L1923:
	ldp	x29, x30, [sp], 80
	ret
	.size	hynix16nm_read_otp_info_hynix, .-hynix16nm_read_otp_info_hynix
	.section	.rodata
	.align	3
.LC435:
	.string	"otp_info_hynix_16nm : allocate memory fail\n"
	.align	3
.LC436:
	.string	"m1 CH %d chip %d get otp info failed!\n"
	.align	3
.LC437:
	.string	"m1 CH %d chip %d get read retry cfg from otp info failed!\n"
	.text
	.align	2
	.global	hynix16nm_get_rr_value_otp_hynix
	.type	hynix16nm_get_rr_value_otp_hynix, %function
hynix16nm_get_rr_value_otp_hynix:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 60]
	mov	w0, 528
	bl	nand_get_temp_buf
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	cmp	x0, 0
	bne	.L1925
	adrp	x0, .LC435
	add	x0, x0, :lo12:.LC435
	bl	nand_print
	mov	w0, -78
	b	.L1929
.L1925:
	ldr	x1, [x29, 48]
	ldr	x0, [x29, 24]
	bl	hynix16nm_read_otp_info_hynix
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	cmp	w0, 0
	bge	.L1927
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 28]
	adrp	x0, .LC436
	add	x0, x0, :lo12:.LC436
	bl	nand_print
.L1927:
	ldr	x0, [x29, 24]
	add	x2, x0, 56
	add	x1, x29, 46
	add	x0, x29, 47
	ldr	x3, [x29, 48]
	bl	hynix16nm_get_read_retry_cfg
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	cmp	w0, 0
	bge	.L1928
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 24]
	ldr	w2, [x0, 28]
	adrp	x0, .LC437
	add	x0, x0, :lo12:.LC437
	bl	nand_print
.L1928:
	ldr	x0, [x29, 48]
	bl	nand_free_temp_buf
	ldr	w0, [x29, 60]
.L1929:
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix16nm_get_rr_value_otp_hynix, .-hynix16nm_get_rr_value_otp_hynix
	.align	2
	.global	hynix16nm_save_otp_value
	.type	hynix16nm_save_otp_value, %function
hynix16nm_save_otp_value:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	str	wzr, [sp, 28]
	b	.L1931
.L1932:
	ldr	w0, [sp, 28]
	ldr	x1, [sp]
	add	x0, x1, x0
	ldrb	w2, [x0]
	ldr	x1, [sp, 8]
	ldr	w0, [sp, 28]
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 56]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
.L1931:
	ldr	w0, [sp, 28]
	cmp	w0, 31
	bls	.L1932
	nop
	add	sp, sp, 32
	ret
	.size	hynix16nm_save_otp_value, .-hynix16nm_save_otp_value
	.align	2
	.global	hynix16nm_get_otp_param
	.type	hynix16nm_get_otp_param, %function
hynix16nm_get_otp_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	str	wzr, [x29, 52]
	str	wzr, [x29, 48]
	b	.L1934
.L1936:
	adrp	x0, .LC406
	add	x0, x0, :lo12:.LC406
	ldr	w1, [x29, 52]
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	hynix16nm_get_rr_value_otp_hynix
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	bne	.L1935
	mov	w0, 1
	str	w0, [x29, 48]
.L1935:
	ldr	w0, [x29, 52]
	add	w0, w0, 1
	str	w0, [x29, 52]
.L1934:
	ldr	w0, [x29, 48]
	cmp	w0, 0
	beq	.L1936
	str	wzr, [x29, 60]
	b	.L1937
.L1940:
	str	wzr, [x29, 56]
	b	.L1938
.L1939:
	ldr	w0, [x29, 60]
	lsl	w1, w0, 2
	ldr	w0, [x29, 56]
	add	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [x29, 16]
	add	x0, x1, x0
	ldr	w1, [x29, 60]
	lsl	w2, w1, 2
	ldr	w1, [x29, 56]
	add	w1, w2, w1
	ldr	x2, [x29, 24]
	uxtw	x1, w1
	add	x1, x2, x1
	ldrb	w1, [x1, 56]
	strb	w1, [x0]
	ldr	w0, [x29, 56]
	add	w0, w0, 1
	str	w0, [x29, 56]
.L1938:
	ldr	w0, [x29, 56]
	cmp	w0, 3
	bls	.L1939
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L1937:
	ldr	w0, [x29, 60]
	cmp	w0, 7
	bls	.L1940
	mov	w0, 0
	ldp	x29, x30, [sp], 64
	ret
	.size	hynix16nm_get_otp_param, .-hynix16nm_get_otp_param
	.align	2
	.global	hynix16nm_set_default_param
	.type	hynix16nm_set_default_param, %function
hynix16nm_set_default_param:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 40]
	str	wzr, [x29, 44]
	b	.L1943
.L1944:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x1, x0
	ldrb	w2, [x0, 56]
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	mov	w1, w2
	strb	w1, [x0, 4080]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1943:
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1944
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix16nm_read_retry_reg_adr
	add	x1, x0, :lo12:hynix16nm_read_retry_reg_adr
	add	x0, x29, 32
	mov	w3, w2
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix16nm_vender_set_param
	str	w0, [x29, 40]
	adrp	x0, .LC355
	add	x0, x0, :lo12:.LC355
	bl	nand_print_dbg
	str	wzr, [x29, 44]
	b	.L1945
.L1946:
	ldr	w0, [x29, 44]
	sub	x1, x29, #4048
	add	x0, x1, x0
	ldrb	w0, [x0, 4080]
	mov	w1, w0
	adrp	x0, .LC356
	add	x0, x0, :lo12:.LC356
	bl	nand_print_dbg
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1945:
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bhi	.L1946
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix16nm_set_default_param, .-hynix16nm_set_default_param
	.section	.rodata
	.align	3
.LC438:
	.string	"ch %d, chip %d Read Retry value Table from uboot\n"
	.align	3
.LC439:
	.string	"[PHY_DBG] repair ch %d chip %d otp value end\n"
	.text
	.align	2
	.global	hynix16nm_init_readretry_param
	.type	hynix16nm_init_readretry_param, %function
hynix16nm_init_readretry_param:
	stp	x29, x30, [sp, -176]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 172]
	str	xzr, [x29, 160]
	str	wzr, [x29, 156]
	str	xzr, [x29, 144]
	add	x0, x29, 40
	stp	xzr, xzr, [x0]
	add	x0, x29, 56
	stp	xzr, xzr, [x0]
	str	xzr, [x29, 72]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 20]
	lsl	w0, w0, 9
	bl	nand_get_temp_buf
	str	x0, [x29, 160]
	ldr	x0, [x29, 160]
	cmp	x0, 0
	bne	.L1949
	mov	w0, -77
	b	.L1957
.L1949:
	mov	w0, 32768
	bl	nand_get_temp_buf
	str	x0, [x29, 144]
	ldr	x0, [x29, 144]
	cmp	x0, 0
	bne	.L1951
	ldr	x0, [x29, 160]
	bl	nand_free_temp_buf
	adrp	x0, .LC409
	add	x0, x0, :lo12:.LC409
	bl	nand_print
	mov	w0, -76
	b	.L1957
.L1951:
	str	wzr, [x29, 156]
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 24]
	mov	w3, w0
	mov	w2, 32
	ldr	x1, [x29, 160]
	ldr	x0, [x29, 144]
	bl	hynix16nm_get_special_info
	str	w0, [x29, 156]
	ldr	w0, [x29, 156]
	cmp	w0, 0
	bne	.L1952
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	w2, [x29, 40]
	adrp	x0, .LC438
	add	x0, x0, :lo12:.LC438
	bl	nand_print_dbg
	str	wzr, [x29, 172]
	b	.L1953
.L1955:
	ldr	w0, [x29, 172]
	ldr	x1, [x29, 160]
	add	x0, x1, x0
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC411
	add	x0, x0, :lo12:.LC411
	bl	nand_print_dbg
	ldr	w0, [x29, 172]
	and	w0, w0, 7
	cmp	w0, 7
	bne	.L1954
	adrp	x0, .LC124
	add	x0, x0, :lo12:.LC124
	bl	nand_print_dbg
.L1954:
	ldr	w0, [x29, 172]
	add	w0, w0, 1
	str	w0, [x29, 172]
.L1953:
	ldr	w0, [x29, 172]
	cmp	w0, 31
	bls	.L1955
	ldr	x1, [x29, 160]
	ldr	x0, [x29, 24]
	bl	hynix16nm_save_otp_value
	ldr	x0, [x29, 24]
	bl	hynix16nm_set_default_param
	b	.L1956
.L1952:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	adrp	x0, .LC412
	add	x0, x0, :lo12:.LC412
	bl	nand_print_dbg
	add	x0, x29, 80
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	hynix16nm_get_otp_param
	ldr	x0, [x29, 24]
	bl	hynix16nm_set_default_param
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	w2, [x29, 40]
	adrp	x0, .LC439
	add	x0, x0, :lo12:.LC439
	bl	nand_print_dbg
	nop
.L1956:
	ldr	x0, [x29, 144]
	bl	nand_free_temp_buf
	ldr	x0, [x29, 160]
	bl	nand_free_temp_buf
	mov	w0, 0
.L1957:
	ldp	x29, x30, [sp], 176
	ret
	.size	hynix16nm_init_readretry_param, .-hynix16nm_init_readretry_param
	.align	2
	.global	hynix16nm_readretry_init
	.type	hynix16nm_readretry_init, %function
hynix16nm_readretry_init:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, hynix16nm_read_retry_mode
	add	x0, x0, :lo12:hynix16nm_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 8
	uxtb	w1, w0
	adrp	x0, hynix16nm_read_retry_cycle
	add	x0, x0, :lo12:hynix16nm_read_retry_cycle
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	uxtb	w1, w0
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	strb	w1, [x0]
	adrp	x0, hynix16nm_read_retry_mode
	add	x0, x0, :lo12:hynix16nm_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 4
	bne	.L1959
	adrp	x0, hynix16nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_adr
	mov	w1, 56
	strb	w1, [x0]
	adrp	x0, hynix16nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_adr
	mov	w1, 57
	strb	w1, [x0, 1]
	adrp	x0, hynix16nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_adr
	mov	w1, 58
	strb	w1, [x0, 2]
	adrp	x0, hynix16nm_read_retry_reg_adr
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_adr
	mov	w1, 59
	strb	w1, [x0, 3]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	str	wzr, [x29, 44]
	b	.L1962
.L1959:
	adrp	x0, hynix16nm_read_retry_mode
	add	x0, x0, :lo12:hynix16nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC358
	add	x0, x0, :lo12:.LC358
	bl	nand_print
	mov	w0, -75
	b	.L1961
.L1963:
	ldr	x1, [x29, 24]
	ldr	w0, [x29, 44]
	add	x0, x1, x0
	strb	wzr, [x0, 56]
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1962:
	ldr	w0, [x29, 44]
	cmp	w0, 127
	bls	.L1963
	ldr	x0, [x29, 24]
	bl	hynix16nm_init_readretry_param
	mov	w0, 0
.L1961:
	ldp	x29, x30, [sp], 48
	ret
	.size	hynix16nm_readretry_init, .-hynix16nm_readretry_init
	.section	.rodata
	.align	3
.LC440:
	.string	"hynix16nm_readretry_exit. \n"
	.text
	.align	2
	.global	hynix16nm_readretry_exit
	.type	hynix16nm_readretry_exit, %function
hynix16nm_readretry_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	adrp	x0, .LC440
	add	x0, x0, :lo12:.LC440
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	bl	hynix16nm_set_default_param
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix16nm_readretry_exit, .-hynix16nm_readretry_exit
	.section	.rodata
	.align	3
.LC441:
	.string	" hynix16nm_special_init hynix16nm_read_retry_mode: %d hynix16nm_read_retry_cycle:%d hynix16nm_read_retry_reg_num:%d\n"
	.align	3
.LC442:
	.string	" hynix16nm_special_init error hynix16nm_read_retry_mode: %d hynix16nm_read_retry_cycle:%d hynix16nm_read_retry_reg_num:%d\n"
	.text
	.align	2
	.global	hynix16nm_special_init
	.type	hynix16nm_special_init, %function
hynix16nm_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L1967
.L1968:
	ldr	x0, [x29, 16]
	bl	hynix16nm_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L1967:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L1968
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L1969
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, hynix16nm_read_page_end
	add	x1, x1, :lo12:hynix16nm_read_page_end
	str	x1, [x0]
	adrp	x0, hynix16nm_read_retry_mode
	add	x0, x0, :lo12:hynix16nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, hynix16nm_read_retry_cycle
	add	x0, x0, :lo12:hynix16nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC441
	add	x0, x0, :lo12:.LC441
	bl	nand_print_dbg
	b	.L1970
.L1969:
	adrp	x0, hynix16nm_read_retry_mode
	add	x0, x0, :lo12:hynix16nm_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, hynix16nm_read_retry_cycle
	add	x0, x0, :lo12:hynix16nm_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, hynix16nm_read_retry_reg_num
	add	x0, x0, :lo12:hynix16nm_read_retry_reg_num
	ldrb	w0, [x0]
	mov	w3, w0
	adrp	x0, .LC442
	add	x0, x0, :lo12:.LC442
	bl	nand_print
.L1970:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix16nm_special_init, .-hynix16nm_special_init
	.section	.rodata
	.align	3
.LC443:
	.string	" hynix16nm_special_exit \n"
	.text
	.align	2
	.global	hynix16nm_special_exit
	.type	hynix16nm_special_exit, %function
hynix16nm_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L1973
.L1974:
	ldr	x0, [x29, 24]
	bl	hynix16nm_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L1973:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L1974
	adrp	x0, .LC443
	add	x0, x0, :lo12:.LC443
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	hynix16nm_special_exit, .-hynix16nm_special_exit
	.section	.rodata
	.align	3
.LC444:
	.string	" generic special init \n"
	.text
	.align	2
	.global	generic_special_init
	.type	generic_special_init, %function
generic_special_init:
	stp	x29, x30, [sp, -16]!
	add	x29, sp, 0
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, generic_read_page_end_not_retry
	add	x1, x1, :lo12:generic_read_page_end_not_retry
	str	x1, [x0]
	adrp	x0, .LC444
	add	x0, x0, :lo12:.LC444
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 16
	ret
	.size	generic_special_init, .-generic_special_init
	.section	.rodata
	.align	3
.LC445:
	.string	" generic special exit \n"
	.text
	.align	2
	.global	generic_special_exit
	.type	generic_special_exit, %function
generic_special_exit:
	stp	x29, x30, [sp, -16]!
	add	x29, sp, 0
	adrp	x0, .LC445
	add	x0, x0, :lo12:.LC445
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 16
	ret
	.size	generic_special_exit, .-generic_special_exit
	.global	samsung_read_retry_mode
	.bss
	.type	samsung_read_retry_mode, %object
	.size	samsung_read_retry_mode, 1
samsung_read_retry_mode:
	.zero	1
	.global	samsung_read_retry_cycle
	.type	samsung_read_retry_cycle, %object
	.size	samsung_read_retry_cycle, 1
samsung_read_retry_cycle:
	.zero	1
	.global	samsung_read_retry_addr
	.data
	.align	3
	.type	samsung_read_retry_addr, %object
	.size	samsung_read_retry_addr, 4
samsung_read_retry_addr:
	.byte	-89
	.byte	-92
	.byte	-91
	.byte	-90
	.global	samsung_read_retry_cmd
	.align	3
	.type	samsung_read_retry_cmd, %object
	.size	samsung_read_retry_cmd, 4
samsung_read_retry_cmd:
	.byte	-95
	.byte	-95
	.byte	-95
	.byte	-95
	.global	samsung_p1
	.align	3
	.type	samsung_p1, %object
	.size	samsung_p1, 60
samsung_p1:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	5
	.byte	10
	.byte	0
	.byte	0
	.byte	40
	.byte	0
	.byte	-20
	.byte	-40
	.byte	-19
	.byte	-11
	.byte	-19
	.byte	-26
	.byte	10
	.byte	15
	.byte	5
	.byte	0
	.byte	15
	.byte	10
	.byte	-5
	.byte	-20
	.byte	-24
	.byte	-17
	.byte	-24
	.byte	-36
	.byte	-15
	.byte	-5
	.byte	-2
	.byte	-16
	.byte	10
	.byte	0
	.byte	-5
	.byte	-20
	.byte	-48
	.byte	-30
	.byte	-48
	.byte	-62
	.byte	20
	.byte	15
	.byte	-5
	.byte	-20
	.byte	-24
	.byte	-5
	.byte	-24
	.byte	-36
	.byte	30
	.byte	20
	.byte	-5
	.byte	-20
	.byte	-5
	.byte	-1
	.byte	-5
	.byte	-8
	.byte	7
	.byte	12
	.byte	2
	.byte	0
	.text
	.align	2
	.global	samsung_vender_set_param
	.type	samsung_vender_set_param, %function
samsung_vender_set_param:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	x1, [x29, 32]
	str	x2, [x29, 24]
	str	w3, [x29, 20]
	adrp	x0, samsung_read_retry_cmd
	add	x0, x0, :lo12:samsung_read_retry_cmd
	ldr	w6, [x29, 20]
	mov	w5, 1
	ldr	x4, [x29, 32]
	ldr	x3, [x29, 24]
	mov	w2, 0
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	set_cmd_with_nand_bus
	str	w0, [x29, 60]
	ldr	x0, [x29, 32]
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 1
	ldrb	w0, [x0]
	mov	w2, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 2
	ldrb	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 32]
	add	x0, x0, 3
	ldrb	w0, [x0]
	mov	w4, w0
	adrp	x0, .LC349
	add	x0, x0, :lo12:.LC349
	bl	nand_print_dbg
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	samsung_vender_set_param, .-samsung_vender_set_param
	.section	.rodata
	.align	3
.LC446:
	.string	"m5 set readretry error ! %x \n"
	.align	3
.LC447:
	.string	"samsung set readretry error ! %x \n"
	.text
	.align	2
	.global	samsung_set_readretry
	.type	samsung_set_readretry, %function
samsung_set_readretry:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 16]
	cmp	w0, 236
	bne	.L1983
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 17]
	cmp	w0, 222
	bne	.L1983
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 18]
	cmp	w0, 148
	bne	.L1983
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 19]
	cmp	w0, 195
	bne	.L1983
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 20]
	cmp	w0, 164
	bne	.L1983
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 21]
	cmp	w0, 202
	bne	.L1983
	ldr	x0, [x29, 24]
	bl	samsung_set_readretry_K9GCGD8U0F
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1985
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 184]
	adrp	x0, .LC446
	add	x0, x0, :lo12:.LC446
	bl	nand_print
	b	.L1985
.L1983:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	uxtw	x0, w0
	lsl	x1, x0, 2
	adrp	x0, samsung_p1
	add	x0, x0, :lo12:samsung_p1
	add	x1, x1, x0
	adrp	x0, samsung_read_retry_addr
	add	x0, x0, :lo12:samsung_read_retry_addr
	mov	w3, 4
	mov	x2, x0
	ldr	x0, [x29, 24]
	bl	samsung_vender_set_param
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 0
	beq	.L1985
	ldr	x0, [x29, 24]
	ldr	w1, [x0, 184]
	adrp	x0, .LC447
	add	x0, x0, :lo12:.LC447
	bl	nand_print
.L1985:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	samsung_set_readretry, .-samsung_set_readretry
	.section	.rodata
	.align	3
.LC448:
	.string	"samsung retry!\n"
	.align	3
.LC449:
	.string	"samsung ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.text
	.align	2
	.global	samsung_read_page_end
	.type	samsung_read_page_end, %function
samsung_read_page_end:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 44]
	str	wzr, [x29, 40]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x2, [x0]
	ldr	x0, [x29, 24]
	ldr	w0, [x0]
	mov	w1, w0
	mov	x0, x2
	bl	nci_get_from_nsi
	str	x0, [x29, 32]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmn	w0, #2
	bne	.L1988
	adrp	x0, .LC448
	add	x0, x0, :lo12:.LC448
	bl	nand_print_dbg
	mov	w0, 1
	str	w0, [x29, 44]
	b	.L1989
.L1996:
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 16]
	cmp	w0, 236
	bne	.L1990
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 17]
	cmp	w0, 222
	bne	.L1990
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 18]
	cmp	w0, 148
	bne	.L1990
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 19]
	cmp	w0, 195
	bne	.L1990
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 20]
	cmp	w0, 164
	bne	.L1990
	ldr	x0, [x29, 32]
	ldrb	w0, [x0, 21]
	cmp	w0, 202
	bne	.L1990
	mov	w1, 256
	ldr	w0, [x29, 44]
	sub	w0, w1, w0
	mov	w1, w0
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
	b	.L1991
.L1990:
	ldr	w1, [x29, 44]
	ldr	x0, [x29, 32]
	str	w1, [x0, 184]
.L1991:
	ldr	x0, [x29, 32]
	bl	samsung_set_readretry
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1998
	ldr	x0, [x29, 24]
	bl	generic_read_page_start
	str	w0, [x29, 40]
	ldr	x0, [x29, 24]
	bl	generic_read_page_end_not_retry
	mov	w1, w0
	ldr	w0, [x29, 40]
	orr	w0, w0, w1
	str	w0, [x29, 40]
	ldr	w0, [x29, 40]
	cmp	w0, 10
	beq	.L1994
	ldr	w0, [x29, 40]
	cmp	w0, 0
	bne	.L1993
.L1994:
	mov	w0, 10
	str	w0, [x29, 40]
	ldr	x0, [x29, 32]
	ldr	x0, [x0, 280]
	ldr	w1, [x0, 12]
	ldr	x0, [x29, 32]
	ldr	w2, [x0, 28]
	ldr	x0, [x29, 24]
	ldr	w3, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	w4, [x0, 8]
	ldr	x0, [x29, 32]
	ldr	w5, [x0, 184]
	adrp	x0, .LC449
	add	x0, x0, :lo12:.LC449
	bl	nand_print_dbg
	b	.L1995
.L1998:
	nop
.L1993:
	ldr	w0, [x29, 44]
	add	w0, w0, 1
	str	w0, [x29, 44]
.L1989:
	adrp	x0, samsung_read_retry_cycle
	add	x0, x0, :lo12:samsung_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 44]
	cmp	w1, w0
	bgt	.L1996
.L1995:
	ldr	x0, [x29, 32]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 32]
	bl	samsung_set_readretry
.L1988:
	ldr	w0, [x29, 40]
	ldp	x29, x30, [sp], 48
	ret
	.size	samsung_read_page_end, .-samsung_read_page_end
	.section	.rodata
	.align	3
.LC450:
	.string	"samsung read retry conut: %d !\n"
	.text
	.align	2
	.global	samsung_readretry_init
	.type	samsung_readretry_init, %function
samsung_readretry_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 288]
	ldr	w0, [x0, 52]
	lsr	w0, w0, 16
	uxtb	w1, w0
	adrp	x0, samsung_read_retry_mode
	add	x0, x0, :lo12:samsung_read_retry_mode
	strb	w1, [x0]
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 16]
	cmp	w0, 236
	bne	.L2000
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 17]
	cmp	w0, 222
	bne	.L2000
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 18]
	cmp	w0, 148
	bne	.L2000
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 19]
	cmp	w0, 195
	bne	.L2000
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 20]
	cmp	w0, 164
	bne	.L2000
	ldr	x0, [x29, 24]
	ldrb	w0, [x0, 21]
	cmp	w0, 202
	bne	.L2000
	adrp	x0, samsung_read_retry_cycle
	add	x0, x0, :lo12:samsung_read_retry_cycle
	mov	w1, -1
	strb	w1, [x0]
	b	.L2001
.L2000:
	adrp	x0, samsung_read_retry_cycle
	add	x0, x0, :lo12:samsung_read_retry_cycle
	mov	w1, 15
	strb	w1, [x0]
.L2001:
	adrp	x0, samsung_read_retry_cycle
	add	x0, x0, :lo12:samsung_read_retry_cycle
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC450
	add	x0, x0, :lo12:.LC450
	bl	nand_print_dbg
	adrp	x0, samsung_read_retry_mode
	add	x0, x0, :lo12:samsung_read_retry_mode
	ldrb	w0, [x0]
	cmp	w0, 32
	beq	.L2002
	mov	w0, -100
	b	.L2003
.L2002:
	mov	w0, 0
.L2003:
	ldp	x29, x30, [sp], 32
	ret
	.size	samsung_readretry_init, .-samsung_readretry_init
	.align	2
	.global	samsung_readretry_exit
	.type	samsung_readretry_exit, %function
samsung_readretry_exit:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	mov	w0, 0
	add	sp, sp, 16
	ret
	.size	samsung_readretry_exit, .-samsung_readretry_exit
	.section	.rodata
	.align	3
.LC451:
	.string	"%s feature set fail\n"
	.text
	.align	2
	.global	samsung_set_readretry_K9GCGD8U0F
	.type	samsung_set_readretry_K9GCGD8U0F, %function
samsung_set_readretry_K9GCGD8U0F:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	wzr, [x29, 60]
	strb	wzr, [x29, 57]
	str	wzr, [x29, 48]
	str	wzr, [x29, 40]
	strb	wzr, [x29, 59]
	ldr	x0, [x29, 24]
	bl	nand_enable_chip
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 280]
	bl	ndfc_disable_randomize
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	strb	w0, [x29, 58]
	strb	wzr, [x29, 59]
	b	.L2007
.L2008:
	ldrb	w0, [x29, 59]
	sxtw	x0, w0
	sub	x1, x29, #4032
	add	x0, x1, x0
	mov	x1, x0
	ldrb	w0, [x29, 58]
	strb	w0, [x1, 4080]
	ldrb	w0, [x29, 59]
	sxtw	x0, w0
	sub	x1, x29, #4032
	add	x0, x1, x0
	mov	x1, x0
	mov	w0, -1
	strb	w0, [x1, 4072]
	ldrb	w0, [x29, 59]
	add	w0, w0, 1
	strb	w0, [x29, 59]
.L2007:
	ldrb	w0, [x29, 59]
	cmp	w0, 3
	bls	.L2008
	add	x1, x29, 48
	add	x0, x29, 57
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_set_read_retry_K9GCGD8U0F
	str	w0, [x29, 60]
	add	x1, x29, 40
	add	x0, x29, 57
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_get_read_retry_K9GCGD8U0F
	str	w0, [x29, 60]
	ldrb	w1, [x29, 40]
	ldrb	w0, [x29, 48]
	cmp	w1, w0
	bne	.L2009
	ldrb	w1, [x29, 41]
	ldrb	w0, [x29, 49]
	cmp	w1, w0
	bne	.L2009
	ldrb	w1, [x29, 42]
	ldrb	w0, [x29, 50]
	cmp	w1, w0
	bne	.L2009
	ldrb	w1, [x29, 43]
	ldrb	w0, [x29, 51]
	cmp	w1, w0
	beq	.L2010
.L2009:
	adrp	x0, __func__.8177
	add	x1, x0, :lo12:__func__.8177
	adrp	x0, .LC451
	add	x0, x0, :lo12:.LC451
	bl	nand_print
	mov	w0, -1
	str	w0, [x29, 60]
.L2010:
	ldr	x0, [x29, 24]
	ldr	w0, [x0, 184]
	cmp	w0, 255
	bne	.L2011
	ldr	x0, [x29, 24]
	str	wzr, [x0, 184]
.L2011:
	ldr	x0, [x29, 24]
	bl	nand_disable_chip
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	samsung_set_readretry_K9GCGD8U0F, .-samsung_set_readretry_K9GCGD8U0F
	.section	.rodata
	.align	3
.LC452:
	.string	" samsung_special_init samsung_read_retry_mode:%d samsung_read_retry_cycle :%d \n"
	.align	3
.LC453:
	.string	" samsung_special_init error samsung_read_retry_mode:%d samsung_read_retry_cycle :%d \n"
	.text
	.align	2
	.global	samsung_special_init
	.type	samsung_special_init, %function
samsung_special_init:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	wzr, [x29, 28]
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 16]
	b	.L2014
.L2015:
	ldr	x0, [x29, 16]
	bl	samsung_readretry_init
	mov	w1, w0
	ldr	w0, [x29, 28]
	orr	w0, w0, w1
	str	w0, [x29, 28]
	ldr	x0, [x29, 16]
	ldr	x0, [x0]
	str	x0, [x29, 16]
.L2014:
	ldr	x0, [x29, 16]
	cmp	x0, 0
	bne	.L2015
	ldr	w0, [x29, 28]
	cmp	w0, 0
	bne	.L2016
	adrp	x0, df_read_page_end
	add	x0, x0, :lo12:df_read_page_end
	adrp	x1, samsung_read_page_end
	add	x1, x1, :lo12:samsung_read_page_end
	str	x1, [x0]
	adrp	x0, samsung_read_retry_mode
	add	x0, x0, :lo12:samsung_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, samsung_read_retry_cycle
	add	x0, x0, :lo12:samsung_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC452
	add	x0, x0, :lo12:.LC452
	bl	nand_print_dbg
	b	.L2017
.L2016:
	adrp	x0, samsung_read_retry_mode
	add	x0, x0, :lo12:samsung_read_retry_mode
	ldrb	w0, [x0]
	mov	w1, w0
	adrp	x0, samsung_read_retry_cycle
	add	x0, x0, :lo12:samsung_read_retry_cycle
	ldrb	w0, [x0]
	mov	w2, w0
	adrp	x0, .LC453
	add	x0, x0, :lo12:.LC453
	bl	nand_print
.L2017:
	ldr	w0, [x29, 28]
	ldp	x29, x30, [sp], 32
	ret
	.size	samsung_special_init, .-samsung_special_init
	.section	.rodata
	.align	3
.LC454:
	.string	" samsung_special_exit \n"
	.text
	.align	2
	.global	samsung_special_exit
	.type	samsung_special_exit, %function
samsung_special_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, g_nsi
	add	x0, x0, :lo12:g_nsi
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	str	x0, [x29, 24]
	b	.L2020
.L2021:
	ldr	x0, [x29, 24]
	bl	samsung_readretry_exit
	ldr	x0, [x29, 24]
	ldr	x0, [x0]
	str	x0, [x29, 24]
.L2020:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L2021
	adrp	x0, .LC454
	add	x0, x0, :lo12:.LC454
	bl	nand_print_dbg
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	samsung_special_exit, .-samsung_special_exit
	.local	panic_info
	.comm	panic_info,40,8
	.local	panic_part
	.comm	panic_part,16,8
	.local	nand_on_panic
	.comm	nand_on_panic,4,4
	.local	panic_enable
	.comm	panic_enable,4,4
	.align	2
	.type	panic_mark_panic, %function
panic_mark_panic:
	adrp	x0, nand_on_panic
	add	x0, x0, :lo12:nand_on_panic
	mov	w1, 1
	str	w1, [x0]
	nop
	ret
	.size	panic_mark_panic, .-panic_mark_panic
	.align	2
	.global	is_on_panic
	.type	is_on_panic, %function
is_on_panic:
	adrp	x0, nand_on_panic
	add	x0, x0, :lo12:nand_on_panic
	ldr	w0, [x0]
	cmp	w0, 0
	cset	w0, ne
	uxtb	w0, w0
	ret
	.size	is_on_panic, .-is_on_panic
	.align	2
	.global	panic_mark_enable
	.type	panic_mark_enable, %function
panic_mark_enable:
	adrp	x0, panic_enable
	add	x0, x0, :lo12:panic_enable
	mov	w1, 1
	str	w1, [x0]
	nop
	ret
	.size	panic_mark_enable, .-panic_mark_enable
	.align	2
	.global	is_panic_enable
	.type	is_panic_enable, %function
is_panic_enable:
	adrp	x0, panic_enable
	add	x0, x0, :lo12:panic_enable
	ldr	w0, [x0]
	cmp	w0, 0
	cset	w0, ne
	uxtb	w0, w0
	ret
	.size	is_panic_enable, .-is_panic_enable
	.section	.rodata
	.align	3
.LC455:
	.string	"[ND] panic part %s off %u size %u\n"
	.text
	.align	2
	.global	panic_init_part
	.type	panic_init_part, %function
panic_init_part:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	beq	.L2030
	ldr	w0, [x29, 16]
	cmp	w0, 0
	bne	.L2031
.L2030:
	mov	w0, -1
	b	.L2032
.L2031:
	adrp	x0, panic_part
	add	x0, x0, :lo12:panic_part
	ldr	w0, [x0, 12]
	cmp	w0, 0
	beq	.L2033
	mov	w0, -1
	b	.L2032
.L2033:
	adrp	x0, panic_part
	add	x0, x0, :lo12:panic_part
	ldr	x1, [x29, 24]
	str	x1, [x0]
	adrp	x0, panic_part
	add	x0, x0, :lo12:panic_part
	ldr	w1, [x29, 20]
	str	w1, [x0, 8]
	adrp	x0, panic_part
	add	x0, x0, :lo12:panic_part
	ldr	w1, [x29, 16]
	str	w1, [x0, 12]
	bl	panic_mark_enable
	adrp	x0, .LC455
	add	x0, x0, :lo12:.LC455
	ldr	w3, [x29, 16]
	ldr	w2, [x29, 20]
	ldr	x1, [x29, 24]
	bl	nand_print
	mov	w0, 0
.L2032:
	ldp	x29, x30, [sp], 32
	ret
	.size	panic_init_part, .-panic_init_part
	.section	.rodata
	.align	3
.LC456:
	.string	"[ND] reserved panic block before %d!\n"
	.align	3
.LC457:
	.string	"[NE] reserved panic block %d!\n"
	.align	3
.LC458:
	.string	"[NE] no reserved panic block!!! quit!\n"
	.text
	.align	2
	.global	panic_backup_block
	.type	panic_backup_block, %function
panic_backup_block:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	bl	is_panic_enable
	cmp	w0, 0
	bne	.L2035
	mov	w0, 0
	b	.L2036
.L2035:
	str	wzr, [x29, 60]
	b	.L2037
.L2041:
	adrp	x0, panic_info
	add	x0, x0, :lo12:panic_info
	ldrsw	x1, [x29, 60]
	ldr	x0, [x0, x1, lsl 3]
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	cmp	x0, 0
	beq	.L2038
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC456
	add	x0, x0, :lo12:.LC456
	bl	nand_print
	mov	w0, 0
	b	.L2036
.L2038:
	ldr	x0, [x29, 24]
	bl	out_phy_block_from_free_list
	str	x0, [x29, 48]
	ldr	x0, [x29, 48]
	cmp	x0, 0
	beq	.L2039
	ldr	x0, [x29, 48]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC457
	add	x0, x0, :lo12:.LC457
	bl	nand_print
	adrp	x0, panic_info
	add	x0, x0, :lo12:panic_info
	ldrsw	x1, [x29, 60]
	ldr	x2, [x29, 48]
	str	x2, [x0, x1, lsl 3]
	b	.L2042
.L2039:
	adrp	x0, .LC458
	add	x0, x0, :lo12:.LC458
	bl	nand_print
	mov	w0, -1
	b	.L2036
.L2042:
	ldr	w0, [x29, 60]
	add	w0, w0, 1
	str	w0, [x29, 60]
.L2037:
	ldr	w0, [x29, 60]
	cmp	w0, 2
	ble	.L2041
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	mov	w0, w1
	lsl	w0, w0, 1
	add	w0, w0, w1
	mov	w1, w0
	adrp	x0, panic_info
	add	x0, x0, :lo12:panic_info
	str	w1, [x0, 24]
	mov	w0, 0
.L2036:
	ldp	x29, x30, [sp], 64
	ret
	.size	panic_backup_block, .-panic_backup_block
	.align	2
	.global	panic_read
	.type	panic_read, %function
panic_read:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	ldr	x0, [x29, 24]
	str	x0, [x29, 56]
	bl	is_panic_enable
	cmp	w0, 0
	bne	.L2044
	mov	w0, -1
	b	.L2045
.L2044:
	bl	panic_mark_panic
	b	.L2046
.L2047:
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 72]
	ldr	x3, [x29, 56]
	mov	w2, 32768
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x4
	ldr	x0, [x29, 56]
	add	x0, x0, 32768
	str	x0, [x29, 56]
	ldr	w0, [x29, 36]
	add	w0, w0, 32768
	str	w0, [x29, 36]
	ldr	w0, [x29, 32]
	sub	w0, w0, #32768
	str	w0, [x29, 32]
.L2046:
	ldr	w1, [x29, 32]
	mov	w0, 32767
	cmp	w1, w0
	bhi	.L2047
	ldr	w0, [x29, 32]
	cmp	w0, 0
	beq	.L2048
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 72]
	ldr	x3, [x29, 56]
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	ldr	x0, [x29, 40]
	blr	x4
.L2048:
	mov	w0, 0
.L2045:
	ldp	x29, x30, [sp], 64
	ret
	.size	panic_read, .-panic_read
	.section	.rodata
	.align	3
.LC459:
	.string	"[ND] write panic data : logic addr: %d, sectors %d, pages %d, blocks %d\n"
	.align	3
.LC460:
	.string	"[NE] too much data for panic-write, %d sectors left over\n"
	.align	3
.LC461:
	.string	"[ND] write panic data block: %d page %d !\n"
	.align	3
.LC462:
	.string	"[ND] write dummy data block: %d page %d !\n"
	.text
	.align	2
	.global	panic_write
	.type	panic_write, %function
panic_write:
	stp	x29, x30, [sp, -144]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	adrp	x0, panic_info
	add	x0, x0, :lo12:panic_info
	str	x0, [x29, 112]
	bl	is_panic_enable
	cmp	w0, 0
	bne	.L2050
	mov	w0, -1
	b	.L2061
.L2050:
	bl	panic_mark_panic
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	str	x0, [x29, 104]
	ldr	x0, [x29, 104]
	ldr	x0, [x0, 8]
	str	x0, [x29, 96]
	ldr	x0, [x29, 96]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 32]
	add	w0, w1, w0
	sub	w0, w0, #1
	ldr	x1, [x29, 96]
	ldrb	w1, [x1, 8]
	udiv	w0, w0, w1
	str	w0, [x29, 124]
	ldr	x0, [x29, 96]
	ldrh	w0, [x0, 20]
	mov	w1, w0
	ldr	w0, [x29, 124]
	add	w0, w1, w0
	sub	w0, w0, #1
	ldr	x1, [x29, 96]
	ldrh	w1, [x1, 20]
	udiv	w0, w0, w1
	str	w0, [x29, 92]
	adrp	x0, .LC459
	add	x0, x0, :lo12:.LC459
	ldr	w4, [x29, 92]
	ldr	w3, [x29, 124]
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	bl	nand_print
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 24]
	sub	w1, w0, #3
	ldr	w0, [x29, 124]
	cmp	w1, w0
	bcs	.L2052
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 24]
	sub	w0, w0, #3
	ldr	x1, [x29, 96]
	ldrb	w1, [x1, 8]
	mul	w1, w0, w1
	adrp	x0, .LC460
	add	x0, x0, :lo12:.LC460
	bl	nand_print
	mov	w0, -1
	b	.L2061
.L2052:
	add	x0, x29, 72
	mov	w2, 16
	mov	w1, -91
	bl	nand_memset
	ldr	x0, [x29, 24]
	str	x0, [x29, 128]
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 112]
	uxtw	x1, w1
	ldr	x0, [x0, x1, lsl 3]
	str	x0, [x29, 136]
	b	.L2053
.L2056:
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 96]
	ldrh	w1, [x1, 20]
	cmp	w0, w1
	bcc	.L2054
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 28]
	add	w1, w0, 1
	ldr	x0, [x29, 112]
	str	w1, [x0, 28]
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 112]
	uxtw	x1, w1
	ldr	x0, [x0, x1, lsl 3]
	str	x0, [x29, 136]
	ldr	x0, [x29, 112]
	str	wzr, [x0, 32]
.L2054:
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	cmp	w0, 0
	bne	.L2055
	ldr	x0, [x29, 136]
	ldrh	w1, [x0]
	ldr	x0, [x29, 96]
	ldrh	w2, [x0, 28]
	add	x0, x29, 48
	mov	x5, 0
	mov	x4, 0
	mov	w3, w2
	mov	w2, 0
	bl	set_physic_op_par
	ldr	x0, [x29, 104]
	ldr	x2, [x0, 3464]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 104]
	blr	x2
	ldr	x0, [x29, 136]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w1, w0
	ldr	x0, [x29, 136]
	strh	w1, [x0, 6]
	ldr	x0, [x29, 136]
	strh	wzr, [x0, 4]
.L2055:
	ldr	x0, [x29, 136]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 32]
	adrp	x0, .LC461
	add	x0, x0, :lo12:.LC461
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	mov	w0, -1
	strb	w0, [x29, 72]
	mov	w0, -86
	strb	w0, [x29, 73]
	mov	w0, 99
	strb	w0, [x29, 74]
	ldr	x0, [x29, 104]
	ldrb	w0, [x0, 24]
	strb	w0, [x29, 75]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 28]
	uxtb	w0, w0
	strb	w0, [x29, 76]
	ldr	x0, [x29, 136]
	ldrh	w0, [x0, 6]
	mov	w1, w0
	add	x0, x29, 72
	mov	w3, 2
	mov	w2, 5
	bl	set_spare_data
	add	x0, x29, 72
	mov	w3, 4
	mov	w2, 7
	ldr	w1, [x29, 36]
	bl	set_spare_data
	ldr	x0, [x29, 96]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 32]
	cmp	w1, w0
	csel	w1, w1, w0, ls
	add	x0, x29, 72
	mov	w3, 4
	mov	w2, 11
	bl	set_spare_data
	ldr	x0, [x29, 136]
	ldrh	w1, [x0]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	uxth	w2, w0
	ldr	x0, [x29, 96]
	ldrh	w3, [x0, 28]
	add	x4, x29, 72
	add	x0, x29, 48
	mov	x5, x4
	ldr	x4, [x29, 128]
	bl	set_physic_op_par
	ldr	x0, [x29, 104]
	ldr	x2, [x0, 3480]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 104]
	blr	x2
	ldr	x0, [x29, 96]
	ldrh	w0, [x0, 26]
	uxth	x0, w0
	ldr	x1, [x29, 128]
	add	x0, x1, x0
	str	x0, [x29, 128]
	ldr	x0, [x29, 96]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 32]
	cmp	w1, w0
	csel	w0, w1, w0, ls
	ldr	w1, [x29, 36]
	add	w0, w1, w0
	str	w0, [x29, 36]
	ldr	x0, [x29, 96]
	ldrb	w0, [x0, 8]
	mov	w1, w0
	ldr	w0, [x29, 32]
	cmp	w1, w0
	csel	w0, w1, w0, ls
	ldr	w1, [x29, 32]
	sub	w0, w1, w0
	str	w0, [x29, 32]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 24]
	sub	w1, w0, #1
	ldr	x0, [x29, 112]
	str	w1, [x0, 24]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	add	w1, w0, 1
	ldr	x0, [x29, 112]
	str	w1, [x0, 32]
	ldr	w0, [x29, 124]
	sub	w0, w0, #1
	str	w0, [x29, 124]
.L2053:
	ldr	w0, [x29, 124]
	cmp	w0, 0
	bne	.L2056
	mov	w0, 98
	strb	w0, [x29, 74]
	str	wzr, [x29, 120]
	b	.L2057
.L2060:
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	cmp	w0, 0
	beq	.L2058
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 96]
	ldrh	w1, [x1, 20]
	cmp	w0, w1
	bcs	.L2058
	ldr	x0, [x29, 136]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 32]
	adrp	x0, .LC462
	add	x0, x0, :lo12:.LC462
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	ldr	x0, [x29, 136]
	ldrh	w1, [x0]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	uxth	w2, w0
	ldr	x0, [x29, 96]
	ldrh	w3, [x0, 28]
	add	x4, x29, 72
	add	x0, x29, 48
	mov	x5, x4
	ldr	x4, [x29, 128]
	bl	set_physic_op_par
	ldr	x0, [x29, 104]
	ldr	x2, [x0, 3480]
	add	x0, x29, 48
	mov	x1, x0
	ldr	x0, [x29, 104]
	blr	x2
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	add	w1, w0, 1
	ldr	x0, [x29, 112]
	str	w1, [x0, 32]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 24]
	sub	w1, w0, #1
	ldr	x0, [x29, 112]
	str	w1, [x0, 24]
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 96]
	ldrh	w1, [x1, 20]
	cmp	w0, w1
	bcc	.L2059
	ldr	x0, [x29, 112]
	ldr	w0, [x0, 28]
	add	w1, w0, 1
	ldr	x0, [x29, 112]
	str	w1, [x0, 28]
	ldr	x0, [x29, 112]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 112]
	uxtw	x1, w1
	ldr	x0, [x0, x1, lsl 3]
	str	x0, [x29, 136]
	ldr	x0, [x29, 112]
	str	wzr, [x0, 32]
.L2059:
	ldr	w0, [x29, 120]
	add	w0, w0, 1
	str	w0, [x29, 120]
.L2057:
	ldr	w0, [x29, 120]
	cmp	w0, 2
	bls	.L2060
.L2058:
	mov	w0, 0
.L2061:
	ldp	x29, x30, [sp], 144
	ret
	.size	panic_write, .-panic_write
	.align	2
	.global	is_panic_data_page
	.type	is_panic_data_page, %function
is_panic_data_page:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 170
	bne	.L2063
	ldr	x0, [sp, 8]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 99
	bne	.L2063
	mov	w0, 1
	b	.L2064
.L2063:
	mov	w0, 0
.L2064:
	add	sp, sp, 16
	ret
	.size	is_panic_data_page, .-is_panic_data_page
	.align	2
	.global	is_panic_dummy_page
	.type	is_panic_dummy_page, %function
is_panic_dummy_page:
	sub	sp, sp, #16
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	add	x0, x0, 1
	ldrb	w0, [x0]
	cmp	w0, 170
	bne	.L2067
	ldr	x0, [sp, 8]
	add	x0, x0, 2
	ldrb	w0, [x0]
	cmp	w0, 98
	bne	.L2067
	mov	w0, 1
	b	.L2068
.L2067:
	mov	w0, 0
.L2068:
	add	sp, sp, 16
	ret
	.size	is_panic_dummy_page, .-is_panic_dummy_page
	.section	.rodata
	.align	3
.LC463:
	.string	"[ND] find panic block : %d\n"
	.align	3
.LC464:
	.string	"[NE] invalid panic block index %d\n"
	.text
	.align	2
	.global	panic_recover_info
	.type	panic_recover_info, %function
panic_recover_info:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	add	x0, x0, 4
	ldrb	w0, [x0]
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	cmp	w0, 2
	bhi	.L2071
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC463
	add	x0, x0, :lo12:.LC463
	bl	nand_print
	adrp	x0, panic_info
	add	x0, x0, :lo12:panic_info
	ldr	w1, [x29, 44]
	ldr	x2, [x29, 16]
	str	x2, [x0, x1, lsl 3]
	mov	w0, 0
	b	.L2072
.L2071:
	adrp	x0, .LC464
	add	x0, x0, :lo12:.LC464
	ldr	w1, [x29, 44]
	bl	nand_print
	mov	w0, -1
.L2072:
	ldp	x29, x30, [sp], 48
	ret
	.size	panic_recover_info, .-panic_recover_info
	.section	.rodata
	.align	3
.LC465:
	.string	"[ND] not find panic data, no need to recovery!\n"
	.align	3
.LC466:
	.string	"[ND] recovery panic data start!\n"
	.align	3
.LC467:
	.string	"[NE] %s(%d): malloc failed\n"
	.align	3
.LC468:
	.string	"[ND] ignore blk %d page %d as dummy page\n"
	.align	3
.LC469:
	.string	"[ND] exit recovery panic block\n"
	.align	3
.LC470:
	.string	"[ND] invalid blk %d page %d, finish recovery\n"
	.align	3
.LC471:
	.string	"[ND] move blk %d page %d to logic addr %d\n"
	.text
	.align	2
	.global	panic_recover_data
	.type	panic_recover_data, %function
panic_recover_data:
	stp	x29, x30, [sp, -128]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	str	x0, [x29, 120]
	ldr	x0, [x29, 120]
	ldr	x0, [x0, 8]
	str	x0, [x29, 112]
	adrp	x0, panic_info
	add	x0, x0, :lo12:panic_info
	str	x0, [x29, 104]
	str	xzr, [x29, 96]
	ldr	x0, [x29, 104]
	ldr	x0, [x0]
	cmp	x0, 0
	bne	.L2074
	adrp	x0, .LC465
	add	x0, x0, :lo12:.LC465
	bl	nand_print
	mov	w0, 0
	b	.L2075
.L2074:
	adrp	x0, .LC466
	add	x0, x0, :lo12:.LC466
	bl	nand_print
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 26]
	bl	nand_malloc
	str	x0, [x29, 96]
	ldr	x0, [x29, 96]
	cmp	x0, 0
	bne	.L2076
	adrp	x0, __func__.8283
	add	x1, x0, :lo12:__func__.8283
	adrp	x0, .LC467
	add	x0, x0, :lo12:.LC467
	mov	w2, 308
	bl	nand_print
	mov	w0, -1
	b	.L2075
.L2076:
	ldr	x0, [x29, 112]
	ldrh	w0, [x0, 26]
	mov	w2, w0
	mov	w1, 0
	ldr	x0, [x29, 96]
	bl	nand_memset
	ldr	x0, [x29, 104]
	str	wzr, [x0, 28]
	b	.L2077
.L2086:
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 104]
	uxtw	x1, w1
	ldr	x0, [x0, x1, lsl 3]
	str	x0, [x29, 88]
	ldr	x0, [x29, 104]
	str	wzr, [x0, 32]
	b	.L2078
.L2085:
	ldr	x0, [x29, 88]
	ldrh	w1, [x0]
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 32]
	uxth	w2, w0
	ldr	x0, [x29, 112]
	ldrh	w3, [x0, 28]
	add	x4, x29, 40
	add	x0, x29, 56
	mov	x5, x4
	ldr	x4, [x29, 96]
	bl	set_physic_op_par
	ldr	x0, [x29, 120]
	ldr	x2, [x0, 3472]
	add	x0, x29, 56
	mov	x1, x0
	ldr	x0, [x29, 120]
	blr	x2
	add	x0, x29, 40
	bl	is_panic_dummy_page
	cmp	w0, 0
	beq	.L2079
	ldr	x0, [x29, 88]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 32]
	adrp	x0, .LC468
	add	x0, x0, :lo12:.LC468
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	b	.L2080
.L2079:
	add	x0, x29, 40
	bl	is_panic_data_page
	cmp	w0, 0
	bne	.L2081
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 28]
	cmp	w0, 0
	bne	.L2082
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 32]
	cmp	w0, 0
	bne	.L2082
	ldr	x0, [x29, 88]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 32]
	adrp	x0, .LC469
	add	x0, x0, :lo12:.LC469
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	b	.L2083
.L2082:
	ldr	x0, [x29, 88]
	ldrh	w0, [x0]
	mov	w3, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 32]
	adrp	x0, .LC470
	add	x0, x0, :lo12:.LC470
	mov	w2, w1
	mov	w1, w3
	bl	nand_print
	b	.L2087
.L2081:
	add	x0, x29, 40
	mov	w2, 4
	mov	w1, 7
	bl	get_spare_data
	str	w0, [x29, 84]
	add	x0, x29, 40
	mov	w2, 4
	mov	w1, 11
	bl	get_spare_data
	str	w0, [x29, 80]
	ldr	x0, [x29, 88]
	ldrh	w0, [x0]
	mov	w4, w0
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 32]
	adrp	x0, .LC471
	add	x0, x0, :lo12:.LC471
	ldr	w3, [x29, 84]
	mov	w2, w1
	mov	w1, w4
	bl	nand_print
	ldr	x2, [x29, 96]
	ldr	w1, [x29, 80]
	ldr	w0, [x29, 84]
	bl	nftl_write
.L2080:
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 32]
	add	w1, w0, 1
	ldr	x0, [x29, 104]
	str	w1, [x0, 32]
.L2078:
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 32]
	ldr	x1, [x29, 112]
	ldrh	w1, [x1, 20]
	cmp	w0, w1
	bcc	.L2085
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 28]
	add	w1, w0, 1
	ldr	x0, [x29, 104]
	str	w1, [x0, 28]
.L2077:
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 28]
	cmp	w0, 2
	bls	.L2086
.L2084:
	b	.L2087
.L2088:
	ldr	x0, [x29, 104]
	ldr	w1, [x0, 28]
	ldr	x0, [x29, 104]
	uxtw	x1, w1
	ldr	x0, [x0, x1, lsl 3]
	str	x0, [x29, 88]
	ldr	x0, [x29, 88]
	ldrh	w0, [x0, 6]
	add	w0, w0, 1
	uxth	w0, w0
	mov	w2, w0
	ldr	x1, [x29, 88]
	ldr	x0, [x29, 120]
	bl	erase_block
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 28]
	sub	w1, w0, #1
	ldr	x0, [x29, 104]
	str	w1, [x0, 28]
.L2087:
	ldr	x0, [x29, 104]
	ldr	w0, [x0, 28]
	cmp	w0, 0
	bge	.L2088
.L2083:
	ldr	x0, [x29, 96]
	bl	nand_free
	mov	w0, 0
.L2075:
	ldp	x29, x30, [sp], 128
	ret
	.size	panic_recover_data, .-panic_recover_data
	.align	2
	.global	_nftl_nand_erase_superblk
	.type	_nftl_nand_erase_superblk, %function
_nftl_nand_erase_superblk:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	nftl_get_zone_priv
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	str	x0, [x29, 64]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 2]
	strh	w0, [x29, 42]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	strh	w0, [x29, 40]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	strh	w0, [x29, 44]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 8]
	str	x0, [x29, 48]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	str	x0, [x29, 56]
	ldr	x0, [x29, 64]
	ldr	x2, [x0, 88]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	add	x1, x29, 40
	blr	x2
	ldp	x29, x30, [sp], 80
	ret
	.size	_nftl_nand_erase_superblk, .-_nftl_nand_erase_superblk
	.align	2
	.global	_nftl_nand_read_page
	.type	_nftl_nand_read_page, %function
_nftl_nand_read_page:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	nftl_get_zone_priv
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	str	x0, [x29, 64]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x0, 80]
	add	x1, x1, 1
	str	x1, [x0, 80]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 2]
	strh	w0, [x29, 34]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	strh	w0, [x29, 32]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	strh	w0, [x29, 36]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 8]
	str	x0, [x29, 40]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	str	x0, [x29, 48]
	ldr	x0, [x29, 64]
	ldr	x2, [x0, 96]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	add	x1, x29, 32
	blr	x2
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	cmn	w0, #2
	bne	.L2092
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 132]
	add	w1, w1, 1
	str	w1, [x0, 132]
.L2092:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 80
	ret
	.size	_nftl_nand_read_page, .-_nftl_nand_read_page
	.align	2
	.global	_nftl_nand_write_page
	.type	_nftl_nand_write_page, %function
_nftl_nand_write_page:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	nftl_get_zone_priv
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	str	x0, [x29, 64]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	x1, [x0, 88]
	add	x1, x1, 1
	str	x1, [x0, 88]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 2]
	strh	w0, [x29, 34]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	strh	w0, [x29, 32]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	strh	w0, [x29, 36]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 8]
	str	x0, [x29, 40]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	str	x0, [x29, 48]
	ldr	x0, [x29, 64]
	ldr	x2, [x0, 104]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	add	x1, x29, 32
	blr	x2
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	cmp	w0, 0
	beq	.L2095
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 3440]
	ldr	w1, [x0, 136]
	add	w1, w1, 1
	str	w1, [x0, 136]
.L2095:
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 80
	ret
	.size	_nftl_nand_write_page, .-_nftl_nand_write_page
	.align	2
	.global	_nftl_nand_is_blk_good
	.type	_nftl_nand_is_blk_good, %function
_nftl_nand_is_blk_good:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	nftl_get_zone_priv
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	str	x0, [x29, 64]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 2]
	strh	w0, [x29, 42]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	strh	w0, [x29, 40]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	strh	w0, [x29, 44]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 8]
	str	x0, [x29, 48]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	str	x0, [x29, 56]
	ldr	x0, [x29, 64]
	ldr	x2, [x0, 112]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	add	x1, x29, 40
	blr	x2
	ldp	x29, x30, [sp], 80
	ret
	.size	_nftl_nand_is_blk_good, .-_nftl_nand_is_blk_good
	.align	2
	.global	_nftl_nand_mark_bad_blk
	.type	_nftl_nand_mark_bad_blk, %function
_nftl_nand_mark_bad_blk:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	x1, [x29, 16]
	ldr	x0, [x29, 24]
	bl	nftl_get_zone_priv
	str	x0, [x29, 72]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	str	x0, [x29, 64]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 2]
	strh	w0, [x29, 42]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0]
	strh	w0, [x29, 40]
	ldr	x0, [x29, 16]
	ldrh	w0, [x0, 4]
	strh	w0, [x29, 44]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 8]
	str	x0, [x29, 48]
	ldr	x0, [x29, 16]
	ldr	x0, [x0, 16]
	str	x0, [x29, 56]
	ldr	x0, [x29, 64]
	ldr	x2, [x0, 120]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 8]
	add	x1, x29, 40
	blr	x2
	ldp	x29, x30, [sp], 80
	ret
	.size	_nftl_nand_mark_bad_blk, .-_nftl_nand_mark_bad_blk
	.global	phyinfo
	.bss
	.align	3
	.type	phyinfo, %object
	.size	phyinfo, 20
phyinfo:
	.zero	20
	.section	.rodata
	.align	3
.LC472:
	.string	"[NE]malloc size too large %d!\n"
	.text
	.align	2
	.global	nftl_malloc
	.type	nftl_malloc, %function
nftl_malloc:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	w0, [x29, 28]
	ldr	w0, [x29, 28]
	cmp	w0, 1572864
	bls	.L2102
	adrp	x0, .LC472
	add	x0, x0, :lo12:.LC472
	ldr	w1, [x29, 28]
	bl	nand_print
.L2102:
	ldr	w0, [x29, 28]
	bl	nand_malloc
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_malloc, .-nftl_malloc
	.align	2
	.global	nftl_free
	.type	nftl_free, %function
nftl_free:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	bl	nand_free
	nop
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_free, .-nftl_free
	.align	2
	.global	add_nftl_blk_list
	.type	add_nftl_blk_list, %function
add_nftl_blk_list:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	x1, [sp]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	ldr	x0, [sp]
	str	xzr, [x0, 16]
	b	.L2106
.L2107:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L2106:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L2107
	ldr	x0, [sp, 24]
	ldr	x1, [sp]
	str	x1, [x0, 16]
	nop
	add	sp, sp, 32
	ret
	.size	add_nftl_blk_list, .-add_nftl_blk_list
	.align	2
	.global	del_last_nftl_blk
	.type	del_last_nftl_blk, %function
del_last_nftl_blk:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	xzr, [sp, 16]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	b	.L2109
.L2112:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 16]
	ldr	x0, [sp, 16]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L2110
	ldr	x0, [sp, 24]
	str	xzr, [x0, 16]
	ldr	x0, [sp, 16]
	b	.L2111
.L2110:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	str	x0, [sp, 24]
.L2109:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L2112
	mov	x0, 0
.L2111:
	add	sp, sp, 32
	ret
	.size	del_last_nftl_blk, .-del_last_nftl_blk
	.align	2
	.global	nftl_support_boot_check_crc_from_dts
	.type	nftl_support_boot_check_crc_from_dts, %function
nftl_support_boot_check_crc_from_dts:
	stp	x29, x30, [sp, -16]!
	add	x29, sp, 0
	bl	nand_get_support_boot_check_crc
	cmp	w0, 0
	cset	w0, ne
	uxtb	w0, w0
	ldp	x29, x30, [sp], 16
	ret
	.size	nftl_support_boot_check_crc_from_dts, .-nftl_support_boot_check_crc_from_dts
	.section	.rodata
	.align	3
.LC473:
	.string	"11:32"
	.align	3
.LC474:
	.string	"NFTL_DATE"
	.align	3
.LC475:
	.string	"004"
	.align	3
.LC476:
	.string	"0"
	.align	3
.LC477:
	.string	"5"
	.align	3
.LC478:
	.string	"sunxi nftl version %s.%s.%s  %s-%s\n"
	.text
	.align	2
	.type	nftl_version, %function
nftl_version:
	stp	x29, x30, [sp, -16]!
	add	x29, sp, 0
	adrp	x0, .LC473
	add	x5, x0, :lo12:.LC473
	adrp	x0, .LC474
	add	x4, x0, :lo12:.LC474
	adrp	x0, .LC475
	add	x3, x0, :lo12:.LC475
	adrp	x0, .LC476
	add	x2, x0, :lo12:.LC476
	adrp	x0, .LC477
	add	x1, x0, :lo12:.LC477
	adrp	x0, .LC478
	add	x0, x0, :lo12:.LC478
	bl	nand_print
	nop
	ldp	x29, x30, [sp], 16
	ret
	.size	nftl_version, .-nftl_version
	.align	2
	.global	get_nftl_version
	.type	get_nftl_version, %function
get_nftl_version:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 56]
	str	x1, [x29, 48]
	str	x2, [x29, 40]
	str	x3, [x29, 32]
	str	x4, [x29, 24]
	adrp	x0, .LC477
	add	x0, x0, :lo12:.LC477
	mov	w2, 2
	mov	x1, x0
	ldr	x0, [x29, 56]
	bl	nand_memcpy
	adrp	x0, .LC476
	add	x0, x0, :lo12:.LC476
	mov	w2, 2
	mov	x1, x0
	ldr	x0, [x29, 48]
	bl	nand_memcpy
	adrp	x0, .LC475
	add	x0, x0, :lo12:.LC475
	mov	w2, 4
	mov	x1, x0
	ldr	x0, [x29, 40]
	bl	nand_memcpy
	adrp	x0, .LC474
	add	x0, x0, :lo12:.LC474
	mov	w2, 10
	mov	x1, x0
	ldr	x0, [x29, 32]
	bl	nand_memcpy
	adrp	x0, .LC473
	add	x0, x0, :lo12:.LC473
	mov	w2, 6
	mov	x1, x0
	ldr	x0, [x29, 24]
	bl	nand_memcpy
	nop
	ldp	x29, x30, [sp], 64
	ret
	.size	get_nftl_version, .-get_nftl_version
	.section	.rodata
	.align	3
.LC479:
	.string	"[NE]%s:malloc fail for nftl_cfg\n"
	.align	3
.LC480:
	.string	"[ND]max_erase_times = %i\n"
	.text
	.align	2
	.global	nftl_initialize
	.type	nftl_initialize, %function
nftl_initialize:
	sub	sp, sp, #112
	stp	x29, x30, [sp, 32]
	add	x29, sp, 32
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	str	x0, [x29, 72]
	str	xzr, [x29, 64]
	adrp	x0, .LC2
	add	x1, x0, :lo12:.LC2
	add	x0, x29, 48
	ldr	x2, [x1]
	str	x2, [x0]
	ldrb	w1, [x1, 8]
	strb	w1, [x0, 8]
	mov	w0, 47
	strb	w0, [x29, 40]
	strb	wzr, [x29, 41]
	adrp	x0, com
	add	x1, x0, :lo12:com
	add	x0, x29, 40
	mov	x2, x1
	mov	x1, x0
	mov	x0, 0
	bl	nand_fdt_get_prop_string_by_path
	str	x0, [x29, 64]
	ldr	x0, [x29, 64]
	cmp	x0, 0
	bne	.L2118
	mov	w0, -1
	b	.L2123
.L2118:
	add	x0, x29, 48
	mov	x2, 9
	mov	x1, x0
	ldr	x0, [x29, 64]
	bl	nand_memcmp
	cmp	w0, 0
	beq	.L2120
	mov	w0, -1
	b	.L2123
.L2120:
	bl	nftl_version
	adrp	x0, phyinfo
	add	x0, x0, :lo12:phyinfo
	bl	nftl_get_nand_phyinfo
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 34]
	cmp	w0, 15
	bhi	.L2135
	mov	w0, -1
	b	.L2123
.L2135:
	mov	w0, 36
	bl	nftl_malloc
	mov	x1, x0
	ldr	x0, [x29, 24]
	str	x1, [x0, 32]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	cmp	x0, 0
	bne	.L2124
	adrp	x0, __func__.8414
	add	x1, x0, :lo12:__func__.8414
	adrp	x0, .LC479
	add	x0, x0, :lo12:.LC479
	bl	nand_print
	mov	w0, 1
	b	.L2123
.L2124:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	strh	wzr, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0, 2]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0, 4]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0, 6]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	strh	wzr, [x0, 8]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	strh	wzr, [x0, 10]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldr	x1, [x1, 40]
	ldrh	w1, [x1, 2498]
	strh	w1, [x0, 12]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldr	x1, [x1, 40]
	ldrh	w1, [x1, 2500]
	strh	w1, [x0, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 8
	strh	w1, [x0, 14]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 2
	strh	w1, [x0, 18]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 3
	strh	w1, [x0, 20]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 50816
	movk	w1, 0x13, lsl 16
	str	w1, [x0, 32]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	ldrh	w0, [x0, 2494]
	cmp	w0, 0
	bne	.L2125
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 30
	strh	w1, [x0, 22]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 3
	strh	w1, [x0, 24]
	b	.L2126
.L2125:
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	ldrh	w0, [x0, 2494]
	cmp	w0, 255
	bne	.L2127
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 50
	strh	w1, [x0, 22]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 40
	strh	w1, [x0, 24]
	b	.L2126
.L2127:
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	ldrh	w0, [x0, 2494]
	cmp	w0, 10
	bls	.L2128
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	mov	w1, 10
	strh	w1, [x0, 2494]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 30
	strh	w1, [x0, 22]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 3
	strh	w1, [x0, 24]
	b	.L2126
.L2128:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldr	x1, [x1, 40]
	ldrh	w2, [x1, 2494]
	mov	w1, 50
	mul	w1, w2, w1
	uxth	w1, w1
	strh	w1, [x0, 22]
	ldr	x0, [x29, 24]
	ldr	x1, [x0, 32]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	ldrh	w0, [x0, 2494]
	mov	w2, w0
	mov	w0, w2
	ubfiz	w0, w0, 2, 14
	add	w0, w0, w2
	ubfiz	w0, w0, 3, 13
	uxth	w0, w0
	strh	w0, [x1, 24]
.L2126:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldrh	w1, [x1, 2]
	strh	w1, [x0, 26]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldrh	w0, [x0, 26]
	cmp	w0, 0
	beq	.L2129
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 3
	strh	w1, [x0, 24]
	b	.L2130
.L2129:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0, 24]
.L2130:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldrh	w0, [x0, 22]
	cmp	w0, 30
	bls	.L2131
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 30
	strh	w1, [x0, 22]
.L2131:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0, 24]
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	ldr	w1, [x0, 2512]
	mov	w0, 45752
	movk	w0, 0x76, lsl 16
	cmp	w1, w0
	bhi	.L2132
	ldr	x0, [x29, 72]
	ldr	x0, [x0, 80]
	ldr	x0, [x0, 40]
	ldr	w1, [x0, 2512]
	mov	w0, 20863
	movk	w0, 0x1, lsl 16
	cmp	w1, w0
	bls	.L2132
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldr	x1, [x1, 40]
	ldr	w1, [x1, 2512]
	str	w1, [x0, 32]
.L2132:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	mov	w1, 1
	strh	w1, [x0, 28]
	bl	nftl_support_boot_check_crc_from_dts
	cmp	w0, 0
	bne	.L2133
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	strh	wzr, [x0, 28]
.L2133:
	ldr	x0, [x29, 24]
	ldr	x8, [x0, 32]
	ldr	x0, [x29, 24]
	add	x2, x0, 24
	ldr	x0, [x29, 72]
	ldr	x3, [x0, 64]
	ldr	x0, [x29, 72]
	ldr	x4, [x0, 72]
	ldr	x0, [x29, 72]
	ldr	w5, [x0, 44]
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 40]
	mov	w6, w0
	ldr	x0, [x29, 72]
	ldrh	w0, [x0, 34]
	mov	w7, w0
	ldr	x0, [x29, 72]
	strb	wzr, [sp, 16]
	ldr	w1, [x29, 20]
	str	w1, [sp, 8]
	str	x0, [sp]
	mov	x1, x8
	ldr	x0, [x29, 24]
	bl	nftl_start
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	cmp	w0, 0
	beq	.L2134
	ldr	w0, [x29, 60]
	b	.L2123
.L2134:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldr	x1, [x1, 40]
	ldr	w1, [x1, 2504]
	uxth	w1, w1
	strh	w1, [x0, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldr	x1, [x29, 72]
	ldr	x1, [x1, 80]
	ldr	x1, [x1, 40]
	ldr	w1, [x1, 2508]
	uxth	w1, w1
	strh	w1, [x0, 30]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldrh	w0, [x0, 16]
	mov	w1, w0
	adrp	x0, .LC480
	add	x0, x0, :lo12:.LC480
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	nftl_get_zone_cap
	mov	w1, w0
	ldr	x0, [x29, 24]
	str	w1, [x0]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_read
	add	x1, x1, :lo12:_nand_read
	str	x1, [x0, 72]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_write
	add	x1, x1, :lo12:_nand_write
	str	x1, [x0, 80]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_discard
	add	x1, x1, :lo12:_nand_discard
	str	x1, [x0, 96]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_flush_write_cache
	add	x1, x1, :lo12:_nand_flush_write_cache
	str	x1, [x0, 88]
	ldr	x0, [x29, 24]
	adrp	x1, _shutdown_op
	add	x1, x1, :lo12:_shutdown_op
	str	x1, [x0, 104]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_sector_read
	add	x1, x1, :lo12:_nand_sector_read
	str	x1, [x0, 112]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_sector_write
	add	x1, x1, :lo12:_nand_sector_write
	str	x1, [x0, 120]
	ldr	x0, [x29, 24]
	adrp	x1, _nand_flush_sector_write_cache
	add	x1, x1, :lo12:_nand_flush_sector_write_cache
	str	x1, [x0, 128]
	ldr	x0, [x29, 24]
	str	xzr, [x0, 16]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	mov	w1, 1
	str	w1, [x0, 3456]
	mov	w0, 0
.L2123:
	add	sp, x29, 0
	ldp	x29, x30, [sp], 80
	ret
	.size	nftl_initialize, .-nftl_initialize
	.section	.rodata
	.align	3
.LC2:
	.byte	97
	.byte	108
	.byte	108
	.byte	119
	.byte	105
	.byte	110
	.byte	110
	.byte	101
	.byte	114
	.text
	.align	2
	.global	nftl_exit
	.type	nftl_exit, %function
nftl_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 8]
	bl	free_nand_partition
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	nftl_zone_release
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 32]
	bl	nftl_free
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_exit, .-nftl_exit
	.align	2
	.global	_nand_read
	.type	_nand_read, %function
_nand_read:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x3, [x29, 24]
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	bl	__nand_read
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	_nand_read, .-_nand_read
	.align	2
	.global	_nand_write
	.type	_nand_write, %function
_nand_write:
	stp	x29, x30, [sp, -64]!
	add	x29, sp, 0
	str	x0, [x29, 40]
	str	w1, [x29, 36]
	str	w2, [x29, 32]
	str	x3, [x29, 24]
	str	wzr, [x29, 60]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 24]
	ldr	x3, [x29, 24]
	ldr	w2, [x29, 32]
	ldr	w1, [x29, 36]
	bl	__nand_write
	str	w0, [x29, 60]
	ldr	w0, [x29, 60]
	ldp	x29, x30, [sp], 64
	ret
	.size	_nand_write, .-_nand_write
	.align	2
	.global	_nand_discard
	.type	_nand_discard, %function
_nand_discard:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	w2, [x29, 16]
	str	wzr, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	w2, [x29, 16]
	ldr	w1, [x29, 20]
	bl	__nand_discard
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	_nand_discard, .-_nand_discard
	.align	2
	.global	_nand_flush_write_cache
	.type	_nand_flush_write_cache, %function
_nand_flush_write_cache:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	str	wzr, [x29, 44]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	w1, [x29, 20]
	bl	__nand_flush_write_cache
	str	w0, [x29, 44]
	ldr	w0, [x29, 44]
	ldp	x29, x30, [sp], 48
	ret
	.size	_nand_flush_write_cache, .-_nand_flush_write_cache
	.align	2
	.global	_nand_sector_read
	.type	_nand_sector_read, %function
_nand_sector_read:
	sub	sp, sp, #48
	str	x0, [sp, 24]
	str	w1, [sp, 20]
	str	w2, [sp, 16]
	str	x3, [sp, 8]
	str	wzr, [sp, 44]
	ldr	w0, [sp, 44]
	add	sp, sp, 48
	ret
	.size	_nand_sector_read, .-_nand_sector_read
	.align	2
	.global	_nand_sector_write
	.type	_nand_sector_write, %function
_nand_sector_write:
	sub	sp, sp, #48
	str	x0, [sp, 24]
	str	w1, [sp, 20]
	str	w2, [sp, 16]
	str	x3, [sp, 8]
	str	wzr, [sp, 44]
	ldr	w0, [sp, 44]
	add	sp, sp, 48
	ret
	.size	_nand_sector_write, .-_nand_sector_write
	.align	2
	.global	_nand_flush_sector_write_cache
	.type	_nand_flush_sector_write_cache, %function
_nand_flush_sector_write_cache:
	sub	sp, sp, #32
	str	x0, [sp, 8]
	str	w1, [sp, 4]
	str	wzr, [sp, 28]
	ldr	w0, [sp, 28]
	add	sp, sp, 32
	ret
	.size	_nand_flush_sector_write_cache, .-_nand_flush_sector_write_cache
	.align	2
	.global	_shutdown_op
	.type	_shutdown_op, %function
_shutdown_op:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	__shutdown_op
	ldp	x29, x30, [sp], 32
	ret
	.size	_shutdown_op, .-_shutdown_op
	.global	nftl_blk_head
	.bss
	.align	3
	.type	nftl_blk_head, %object
	.size	nftl_blk_head, 136
nftl_blk_head:
	.zero	136
	.section	.rodata
	.align	3
.LC481:
	.string	"[NE]nftl_build_all error!\n"
	.text
	.align	2
	.global	nftl_build_all
	.type	nftl_build_all, %function
nftl_build_all:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 72]
	str	x0, [x29, 40]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	str	xzr, [x0, 16]
	b	.L2155
.L2158:
	ldr	x0, [x29, 40]
	bl	nftl_add
	cmp	x0, 0
	bne	.L2156
	adrp	x0, .LC481
	add	x0, x0, :lo12:.LC481
	bl	nand_print
	mov	w0, 1
	b	.L2157
.L2156:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 64]
	str	x0, [x29, 40]
.L2155:
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L2158
	mov	w0, 0
.L2157:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_build_all, .-nftl_build_all
	.align	2
	.global	nftl_build_one
	.type	nftl_build_one, %function
nftl_build_one:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	str	w1, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 72]
	str	x0, [x29, 40]
	ldr	w0, [x29, 20]
	cmp	w0, 0
	bne	.L2161
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	str	xzr, [x0, 16]
	b	.L2161
.L2164:
	ldr	x0, [x29, 40]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	w0, [x29, 20]
	cmp	w1, w0
	bne	.L2162
	ldr	x0, [x29, 40]
	bl	nftl_add
	cmp	x0, 0
	bne	.L2162
	adrp	x0, .LC481
	add	x0, x0, :lo12:.LC481
	bl	nand_print
	mov	w0, 1
	b	.L2163
.L2162:
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 64]
	str	x0, [x29, 40]
.L2161:
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L2164
	mov	w0, 0
.L2163:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_build_one, .-nftl_build_one
	.section	.rodata
	.align	3
.LC482:
	.string	"[NE]%s:malloc fail for nftl_blk\n"
	.align	3
.LC483:
	.string	"[NE]nftl_initialize failed\n"
	.align	3
.LC484:
	.string	"[ND]nftl_add ok\n"
	.text
	.align	2
	.global	nftl_add
	.type	nftl_add, %function
nftl_add:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	x0, [x29, 24]
	mov	w0, 136
	bl	nftl_malloc
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L2166
	adrp	x0, __func__.8510
	add	x1, x0, :lo12:__func__.8510
	adrp	x0, .LC482
	add	x0, x0, :lo12:.LC482
	bl	nand_print
	mov	x0, 0
	b	.L2167
.L2166:
	ldr	x0, [x29, 24]
	bl	build_nand_partition
	mov	x1, x0
	ldr	x0, [x29, 40]
	str	x1, [x0, 8]
	ldr	x0, [x29, 24]
	ldrh	w0, [x0]
	mov	w1, w0
	ldr	x0, [x29, 40]
	bl	nftl_initialize
	cmp	w0, 0
	beq	.L2168
	adrp	x0, .LC483
	add	x0, x0, :lo12:.LC483
	bl	nand_print
	mov	x0, 0
	b	.L2167
.L2168:
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x1, [x29, 40]
	bl	add_nftl_blk_list
	ldr	x0, [x29, 40]
	bl	panic_recover_data
	adrp	x0, .LC484
	add	x0, x0, :lo12:.LC484
	bl	nand_print_dbg
	ldr	x0, [x29, 40]
.L2167:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_add, .-nftl_add
	.align	2
	.global	nand_ftl_exit
	.type	nand_ftl_exit, %function
nand_ftl_exit:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	bl	del_last_nftl_blk
	str	x0, [x29, 24]
	b	.L2170
.L2171:
	ldr	x0, [x29, 24]
	bl	nftl_exit
	ldr	x0, [x29, 24]
	bl	nftl_free
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	bl	del_last_nftl_blk
	str	x0, [x29, 24]
.L2170:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L2171
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	nand_ftl_exit, .-nand_ftl_exit
	.section	.rodata
	.align	3
.LC485:
	.string	"[ND]cap: 0x%08x.\n"
	.text
	.align	2
	.global	print_all_nftl_blk
	.type	print_all_nftl_blk, %function
print_all_nftl_blk:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	str	x0, [x29, 24]
	b	.L2174
.L2175:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 16]
	ldr	w1, [x0]
	adrp	x0, .LC485
	add	x0, x0, :lo12:.LC485
	bl	nand_print_dbg
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 16]
	str	x0, [x29, 24]
.L2174:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L2175
	nop
	ldp	x29, x30, [sp], 32
	ret
	.size	print_all_nftl_blk, .-print_all_nftl_blk
	.align	2
	.global	get_nftl_num
	.type	get_nftl_num, %function
get_nftl_num:
	sub	sp, sp, #16
	str	wzr, [sp, 12]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	str	x0, [sp]
	b	.L2177
.L2178:
	ldr	w0, [sp, 12]
	add	w0, w0, 1
	str	w0, [sp, 12]
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	str	x0, [sp]
.L2177:
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L2178
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	get_nftl_num, .-get_nftl_num
	.align	2
	.global	get_nftl_cap
	.type	get_nftl_cap, %function
get_nftl_cap:
	sub	sp, sp, #16
	str	wzr, [sp, 12]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	str	x0, [sp]
	b	.L2181
.L2182:
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	ldr	w0, [x0]
	ldr	w1, [sp, 12]
	add	w0, w1, w0
	str	w0, [sp, 12]
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	str	x0, [sp]
.L2181:
	ldr	x0, [sp]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	bne	.L2182
	ldr	w0, [sp, 12]
	add	sp, sp, 16
	ret
	.size	get_nftl_cap, .-get_nftl_cap
	.align	2
	.global	get_first_nftl_cap
	.type	get_first_nftl_cap, %function
get_first_nftl_cap:
	sub	sp, sp, #16
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	ldr	x0, [x0, 16]
	ldr	w0, [x0]
	add	sp, sp, 16
	ret
	.size	get_first_nftl_cap, .-get_first_nftl_cap
	.section	.rodata
	.align	3
.LC486:
	.string	"[NE]parameter error %d,%d !\n"
	.text
	.align	2
	.global	nftl_read
	.type	nftl_read, %function
nftl_read:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	w0, [x29, 28]
	str	w1, [x29, 24]
	str	x2, [x29, 16]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	ldr	w0, [x29, 24]
	cmp	w0, 0
	bne	.L2189
	mov	w0, 0
	b	.L2188
.L2190:
	ldr	x0, [x29, 40]
	ldr	w0, [x0]
	ldr	w1, [x29, 28]
	sub	w0, w1, w0
	str	w0, [x29, 28]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L2189
	adrp	x0, .LC486
	add	x0, x0, :lo12:.LC486
	ldr	w2, [x29, 24]
	ldr	w1, [x29, 28]
	bl	nand_print
	mov	w0, 1
	b	.L2188
.L2189:
	ldr	x0, [x29, 40]
	ldr	w1, [x0]
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bls	.L2190
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 72]
	ldr	x3, [x29, 16]
	ldr	w2, [x29, 24]
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 40]
	blr	x4
.L2188:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_read, .-nftl_read
	.align	2
	.global	nftl_write
	.type	nftl_write, %function
nftl_write:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	w0, [x29, 28]
	str	w1, [x29, 24]
	str	x2, [x29, 16]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	ldr	w0, [x29, 24]
	cmp	w0, 0
	bne	.L2194
	mov	w0, 0
	b	.L2193
.L2195:
	ldr	x0, [x29, 40]
	ldr	w0, [x0]
	ldr	w1, [x29, 28]
	sub	w0, w1, w0
	str	w0, [x29, 28]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L2194
	adrp	x0, .LC486
	add	x0, x0, :lo12:.LC486
	ldr	w2, [x29, 24]
	ldr	w1, [x29, 28]
	bl	nand_print
	mov	w0, 1
	b	.L2193
.L2194:
	ldr	x0, [x29, 40]
	ldr	w1, [x0]
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bls	.L2195
	ldr	x0, [x29, 40]
	ldr	x4, [x0, 80]
	ldr	x3, [x29, 16]
	ldr	w2, [x29, 24]
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 40]
	blr	x4
.L2193:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_write, .-nftl_write
	.align	2
	.global	nftl_discard
	.type	nftl_discard, %function
nftl_discard:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	str	w0, [x29, 28]
	str	w1, [x29, 24]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	ldr	w0, [x29, 24]
	cmp	w0, 0
	bne	.L2199
	mov	w0, 0
	b	.L2198
.L2200:
	ldr	x0, [x29, 40]
	ldr	w0, [x0]
	ldr	w1, [x29, 28]
	sub	w0, w1, w0
	str	w0, [x29, 28]
	ldr	x0, [x29, 40]
	ldr	x0, [x0, 16]
	str	x0, [x29, 40]
	ldr	x0, [x29, 40]
	cmp	x0, 0
	bne	.L2199
	adrp	x0, .LC486
	add	x0, x0, :lo12:.LC486
	ldr	w2, [x29, 24]
	ldr	w1, [x29, 28]
	bl	nand_print
	mov	w0, 1
	b	.L2198
.L2199:
	ldr	x0, [x29, 40]
	ldr	w1, [x0]
	ldr	w0, [x29, 28]
	cmp	w1, w0
	bls	.L2200
	ldr	x0, [x29, 40]
	ldr	x3, [x0, 96]
	ldr	w2, [x29, 24]
	ldr	w1, [x29, 28]
	ldr	x0, [x29, 40]
	blr	x3
.L2198:
	ldp	x29, x30, [sp], 48
	ret
	.size	nftl_discard, .-nftl_discard
	.align	2
	.global	nftl_flush_write_cache
	.type	nftl_flush_write_cache, %function
nftl_flush_write_cache:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x0, [x0, 16]
	str	x0, [x29, 24]
	b	.L2202
.L2203:
	ldr	x0, [x29, 24]
	ldr	x2, [x0, 88]
	mov	w1, 65535
	ldr	x0, [x29, 24]
	blr	x2
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 16]
	str	x0, [x29, 24]
.L2202:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L2203
	mov	w0, 0
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_flush_write_cache, .-nftl_flush_write_cache
	.align	2
	.global	nftl_write_end
	.type	nftl_write_end, %function
nftl_write_end:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x0, [x0, 16]
	str	x0, [x29, 24]
	str	wzr, [x29, 20]
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldrb	w0, [x0, 40]
	cmp	w0, 0
	bne	.L2206
	mov	w0, 0
	b	.L2207
.L2206:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldr	w0, [x0, 44]
	cmp	w0, 0
	bne	.L2208
	mov	w0, 8
	str	w0, [x29, 20]
	b	.L2210
.L2208:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	x0, [x0, 8]
	ldr	w0, [x0, 44]
	str	w0, [x29, 20]
	b	.L2210
.L2211:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	ldr	w1, [x29, 20]
	bl	fill_no_use_page_to_current_block
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 16]
	str	x0, [x29, 24]
.L2210:
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L2211
	mov	w0, 0
.L2207:
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_write_end, .-nftl_write_end
	.align	2
	.global	nftl_clean_zone
	.type	nftl_clean_zone, %function
nftl_clean_zone:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	ldr	x0, [x0, 16]
	str	x0, [x29, 24]
	ldr	x0, [x29, 24]
	cmp	x0, 0
	bne	.L2213
	mov	w0, 0
	b	.L2214
.L2213:
	ldr	x0, [x29, 24]
	ldr	x0, [x0, 24]
	bl	clean_zone
.L2214:
	ldp	x29, x30, [sp], 32
	ret
	.size	nftl_clean_zone, .-nftl_clean_zone
	.section	.rodata
	.align	3
	.type	__func__.5226, %object
	.size	__func__.5226, 19
__func__.5226:
	.string	"get_logic_page_map"
	.align	3
	.type	__func__.5264, %object
	.size	__func__.5264, 23
__func__.5264:
	.string	"nftl_sector_cache_init"
	.align	3
	.type	__func__.5380, %object
	.size	__func__.5380, 11
__func__.5380:
	.string	"nftl_start"
	.align	3
	.type	__func__.5570, %object
	.size	__func__.5570, 15
__func__.5570:
	.string	"shutdown_on_gc"
	.align	3
	.type	__func__.6073, %object
	.size	__func__.6073, 16
__func__.6073:
	.string	"zone_param_init"
	.align	3
	.type	__func__.6287, %object
	.size	__func__.6287, 9
__func__.6287:
	.string	"init_nrc"
	.align	3
	.type	__func__.6377, %object
	.size	__func__.6377, 23
__func__.6377:
	.string	"torture_block_and_mark"
	.align	3
	.type	__func__.6501, %object
	.size	__func__.6501, 26
__func__.6501:
	.string	"block_last_page_ecc_error"
	.align	3
	.type	__func__.6533, %object
	.size	__func__.6533, 18
__func__.6533:
	.string	"nand_dbg_phy_read"
	.align	3
	.type	__func__.6544, %object
	.size	__func__.6544, 23
__func__.6544:
	.string	"nand_dbg_zone_phy_read"
	.align	3
	.type	__func__.6555, %object
	.size	__func__.6555, 24
__func__.6555:
	.string	"nand_dbg_zone_phy_write"
	.align	3
	.type	__func__.6564, %object
	.size	__func__.6564, 19
__func__.6564:
	.string	"nand_dbg_phy_write"
	.align	3
	.type	__func__.6642, %object
	.size	__func__.6642, 19
__func__.6642:
	.string	"write_data_to_nand"
	.align	3
	.type	__FUNCTION__.6666, %object
	.size	__FUNCTION__.6666, 25
__FUNCTION__.6666:
	.string	"nand_power_off_ecc_error"
	.align	3
	.type	__func__.6674, %object
	.size	__func__.6674, 11
__func__.6674:
	.string	"clean_zone"
	.align	3
	.type	__func__.6688, %object
	.size	__func__.6688, 16
__func__.6688:
	.string	"nftl_cache_init"
	.align	3
	.type	__func__.7318, %object
	.size	__func__.7318, 25
__func__.7318:
	.string	"sandisk_A19_special_exit"
	.align	3
	.type	__func__.8177, %object
	.size	__func__.8177, 33
__func__.8177:
	.string	"samsung_set_readretry_K9GCGD8U0F"
	.align	3
	.type	__func__.8283, %object
	.size	__func__.8283, 19
__func__.8283:
	.string	"panic_recover_data"
	.align	3
	.type	__func__.8414, %object
	.size	__func__.8414, 16
__func__.8414:
	.string	"nftl_initialize"
	.align	3
	.type	__func__.8510, %object
	.size	__func__.8510, 9
__func__.8510:
	.string	"nftl_add"
	.ident	"GCC: (Linaro GCC 5.3-2016.05) 5.3.1 20160412"
	.section	.note.GNU-stack,"",@progbits

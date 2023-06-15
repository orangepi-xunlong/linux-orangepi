/*SPDX-Licensen-Identifier: GPL-2.0*/

	.text
	.file	"nftl.c"
	.globl	garbage_collect                 // -- Begin function garbage_collect
	.p2align	2
	.type	garbage_collect,@function
garbage_collect:                        // @garbage_collect
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9
	b.gt	.LBB0_2
// %bb.1:
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #684]
.LBB0_2:
	adrp	x9, gc_src_type
	mov	w8, #3
	str	w8, [x9, :lo12:gc_src_type]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #684]
	cbnz	w8, .LBB0_27
// %bb.3:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9, #682]
	subs	w8, w8, w9
	b.ge	.LBB0_25
// %bb.4:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #686]
	cbnz	w8, .LBB0_19
// %bb.5:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #352]
	str	x8, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB0_7
// %bb.6:
	adrp	x0, .L.str
	add	x0, x0, :lo12:.L.str
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB0_28
.LBB0_7:
	ldr	x8, [sp]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9, asr #1
	b.lt	.LBB0_9
// %bb.8:
	ldr	x9, [sp, #16]
	mov	w8, #4
	strh	w8, [x9, #688]
	b	.LBB0_16
.LBB0_9:
	ldr	x8, [sp]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9, asr #2
	b.lt	.LBB0_11
// %bb.10:
	ldr	x9, [sp, #16]
	mov	w8, #8
	strh	w8, [x9, #688]
	b	.LBB0_15
.LBB0_11:
	ldr	x8, [sp]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9, asr #3
	b.lt	.LBB0_13
// %bb.12:
	ldr	x9, [sp, #16]
	mov	w8, #16
	strh	w8, [x9, #688]
	b	.LBB0_14
.LBB0_13:
	ldr	x8, [sp, #16]
	ldrh	w1, [x8, #30]
	ldr	x8, [sp]
	ldrh	w2, [x8, #4]
	adrp	x0, .L.str.1
	add	x0, x0, :lo12:.L.str.1
	bl	nand_print_dbg
	ldr	x9, [sp, #16]
	mov	w8, #65535
	strh	w8, [x9, #688]
.LBB0_14:
.LBB0_15:
.LBB0_16:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #16]
	subs	w8, w8, w9
	b.gt	.LBB0_18
// %bb.17:
	ldr	x9, [sp, #16]
	mov	w8, #65535
	strh	w8, [x9, #688]
.LBB0_18:
.LBB0_19:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #688]
	strh	w8, [sp, #14]
	ldrh	w8, [sp, #14]
	mov	w9, #65535
	subs	w8, w8, w9
	b.eq	.LBB0_23
// %bb.20:
	ldr	x0, [sp, #16]
	ldrh	w2, [sp, #14]
	mov	x1, xzr
	bl	garbage_collect_first
	cbz	w0, .LBB0_22
// %bb.21:
	ldr	x8, [sp, #16]
	ldrh	w1, [x8, #30]
	adrp	x0, .L.str.2
	add	x0, x0, :lo12:.L.str.2
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB0_28
.LBB0_22:
	b	.LBB0_24
.LBB0_23:
	ldr	x0, [sp, #16]
	mov	w1, #1
	bl	gc_win_block
.LBB0_24:
	b	.LBB0_26
.LBB0_25:
	ldr	x9, [sp, #16]
	mov	w8, #1
	strh	w8, [x9, #684]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #686]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #688]
.LBB0_26:
.LBB0_27:
	stur	wzr, [x29, #-4]
.LBB0_28:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end0:
	.size	garbage_collect, .Lfunc_end0-garbage_collect
	.cfi_endproc
                                        // -- End function
	.globl	garbage_collect_first           // -- Begin function garbage_collect_first
	.p2align	2
	.type	garbage_collect_first,@function
garbage_collect_first:                  // @garbage_collect_first
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #256                    // =256
	stp	x29, x30, [sp, #240]            // 16-byte Folded Spill
	add	x29, sp, #240                   // =240
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	sturh	w2, [x29, #-26]
	stur	wzr, [x29, #-48]
	stur	wzr, [x29, #-52]
	stur	wzr, [x29, #-116]
	sub	x0, x29, #68                    // =68
	mov	w1, #90
	mov	w2, #16
	bl	nand_memset
	ldur	x8, [x29, #-24]
	cbnz	x8, .LBB1_2
// %bb.1:
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_invalid_page_list
	stur	x0, [x29, #-80]
	b	.LBB1_5
.LBB1_2:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	bl	is_current_blcok
	subs	w8, w0, #1                      // =1
	b.ne	.LBB1_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB1_57
.LBB1_4:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	bl	out_phy_block_from_invalid_page_list_by_block
	stur	x0, [x29, #-80]
.LBB1_5:
	ldur	x8, [x29, #-80]
	cbnz	x8, .LBB1_7
// %bb.6:
	adrp	x0, .L.str.3
	add	x0, x0, :lo12:.L.str.3
	bl	nand_print
	mov	w8, #8
	stur	w8, [x29, #-4]
	b	.LBB1_57
.LBB1_7:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #120]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #120]
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w2, w8, #1                      // =1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	sub	x0, x29, #112                   // =112
	str	x0, [sp, #104]                  // 8-byte Folded Spill
	sub	x5, x29, #68                    // =68
	str	x5, [sp, #112]                  // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #104]                  // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	mov	w8, w0
	ldr	x0, [sp, #112]                  // 8-byte Folded Reload
	stur	w8, [x29, #-44]
	bl	is_phy_mapping_page
	subs	w8, w0, #1                      // =1
	b.eq	.LBB1_9
// %bb.8:
	adrp	x0, .L.str.4
	add	x0, x0, :lo12:.L.str.4
	bl	nand_print_dbg
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	mov	w9, #1
	str	w9, [sp, #100]                  // 4-byte Folded Spill
	subs	w2, w8, #1                      // =1
	ldur	w3, [x29, #-116]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w4, [x8, #28]
	adrp	x0, .L.str.5
	add	x0, x0, :lo12:.L.str.5
	bl	nand_print_dbg
	ldurb	w1, [x29, #-67]
	ldurb	w2, [x29, #-66]
	ldurb	w3, [x29, #-65]
	ldurb	w4, [x29, #-64]
	ldurb	w5, [x29, #-63]
	ldurb	w6, [x29, #-62]
	ldurb	w7, [x29, #-61]
	ldurb	w16, [x29, #-60]
	ldurb	w15, [x29, #-59]
	ldurb	w14, [x29, #-58]
	ldurb	w13, [x29, #-57]
	ldurb	w12, [x29, #-56]
	ldurb	w11, [x29, #-55]
	ldurb	w10, [x29, #-54]
	ldurb	w8, [x29, #-53]
	adrp	x0, .L.str.6
	add	x0, x0, :lo12:.L.str.6
	mov	x9, sp
	str	x9, [sp, #88]                   // 8-byte Folded Spill
	str	w16, [x9]
	str	w15, [x9, #8]
	str	w14, [x9, #16]
	str	w13, [x9, #24]
	str	w12, [x9, #32]
	str	w11, [x9, #40]
	str	w10, [x9, #48]
	str	w8, [x9, #56]
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8, #24]
	ldur	x8, [x29, #-80]
	ldrh	w2, [x8]
	adrp	x0, .L.str.7
	add	x0, x0, :lo12:.L.str.7
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #480]
	bl	recover_block_phy_page_mapping
	ldr	w8, [sp, #100]                  // 4-byte Folded Reload
	stur	w8, [x29, #-52]
	b	.LBB1_14
.LBB1_9:
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB1_12
// %bb.10:
	ldur	w8, [x29, #-44]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB1_12
// %bb.11:
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.8
	add	x0, x0, :lo12:.L.str.8
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #480]
	bl	recover_block_phy_page_mapping
	mov	w8, #1
	stur	w8, [x29, #-52]
	b	.LBB1_13
.LBB1_12:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #480]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #512]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #488]
	bl	nand_memcpy
.LBB1_13:
.LBB1_14:
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	add	x1, x8, #464                    // =464
	ldur	x2, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #20]
	bl	check_invalid_page_right
	stur	w0, [x29, #-44]
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB1_16
// %bb.15:
	adrp	x0, .L.str.9
	add	x0, x0, :lo12:.L.str.9
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-52]
.LBB1_16:
	ldur	w8, [x29, #-52]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB1_18
// %bb.17:
	mov	w8, #65535
	sturh	w8, [x29, #-26]
.LBB1_18:
	stur	wzr, [x29, #-32]
.LBB1_19:                               // =>This Loop Header: Depth=1
                                        //     Child Loop BB1_29 Depth 2
	ldur	w8, [x29, #-32]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB1_50
// %bb.20:                              //   in Loop: Header=BB1_19 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #480]
	ldur	w9, [x29, #-32]
                                        // kill: def $x9 killed $w9
	ldr	w8, [x8, x9, lsl #2]
	stur	w8, [x29, #-40]
	ldur	w8, [x29, #-40]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB1_22
// %bb.21:                              //   in Loop: Header=BB1_19 Depth=1
	b	.LBB1_49
.LBB1_22:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-40]
	bl	get_logic_page_map
	stur	x0, [x29, #-88]
	ldur	x8, [x29, #-88]
	cbnz	x8, .LBB1_24
// %bb.23:                              //   in Loop: Header=BB1_19 Depth=1
	b	.LBB1_49
.LBB1_24:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	x8, [x29, #-88]
	ldrh	w8, [x8, #2]
	ldur	x9, [x29, #-80]
	ldrh	w9, [x9]
	subs	w8, w8, w9
	b.eq	.LBB1_26
// %bb.25:                              //   in Loop: Header=BB1_19 Depth=1
	b	.LBB1_49
.LBB1_26:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	x8, [x29, #-88]
	ldrh	w8, [x8]
	and	w8, w8, #0xfff
	and	w8, w8, #0xffff
	ldur	w9, [x29, #-32]
	subs	w8, w8, w9
	b.eq	.LBB1_28
// %bb.27:                              //   in Loop: Header=BB1_19 Depth=1
	b	.LBB1_49
.LBB1_28:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #124]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #124]
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #520]
	sub	x0, x29, #112                   // =112
	sub	x5, x29, #68                    // =68
	bl	set_physic_op_par
	stur	wzr, [x29, #-36]
.LBB1_29:                               //   Parent Loop BB1_19 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-36]
	subs	w8, w8, #2                      // =2
	b.hs	.LBB1_37
// %bb.30:                              //   in Loop: Header=BB1_29 Depth=2
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	sub	x1, x29, #112                   // =112
	blr	x8
	stur	w0, [x29, #-44]
	ldur	w8, [x29, #-44]
	subs	w8, w8, #0                      // =0
	b.hs	.LBB1_32
// %bb.31:                              //   in Loop: Header=BB1_29 Depth=2
	ldur	x8, [x29, #-80]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-32]
	adrp	x0, .L.str.10
	add	x0, x0, :lo12:.L.str.10
	adrp	x1, .L__func__.garbage_collect_first
	add	x1, x1, :lo12:.L__func__.garbage_collect_first
	bl	nand_print
	b	.LBB1_33
.LBB1_32:                               //   in Loop: Header=BB1_19 Depth=1
	b	.LBB1_37
.LBB1_33:                               //   in Loop: Header=BB1_29 Depth=2
	ldur	w8, [x29, #-36]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB1_35
// %bb.34:                              //   in Loop: Header=BB1_29 Depth=2
	ldur	x8, [x29, #-80]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-32]
	ldur	w4, [x29, #-36]
	adrp	x0, .L.str.11
	add	x0, x0, :lo12:.L.str.11
	adrp	x1, .L__func__.garbage_collect_first
	add	x1, x1, :lo12:.L__func__.garbage_collect_first
	bl	nand_print
.LBB1_35:                               //   in Loop: Header=BB1_29 Depth=2
// %bb.36:                              //   in Loop: Header=BB1_29 Depth=2
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB1_29
.LBB1_37:                               //   in Loop: Header=BB1_19 Depth=1
	sub	x0, x29, #68                    // =68
	bl	get_logic_page_from_oob
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB1_39
// %bb.38:                              //   in Loop: Header=BB1_19 Depth=1
	ldur	w8, [x29, #-44]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB1_45
.LBB1_39:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	w8, [x29, #-48]
	ldur	w9, [x29, #-40]
	subs	w8, w8, w9
	b.eq	.LBB1_41
// %bb.40:                              //   in Loop: Header=BB1_19 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-32]
	ldur	w3, [x29, #-48]
	ldur	w4, [x29, #-40]
	adrp	x0, .L.str.12
	add	x0, x0, :lo12:.L.str.12
	bl	nand_print_dbg
	ldurb	w1, [x29, #-68]
	ldurb	w2, [x29, #-67]
	ldurb	w3, [x29, #-66]
	ldurb	w4, [x29, #-65]
	ldurb	w5, [x29, #-64]
	ldurb	w6, [x29, #-63]
	ldurb	w7, [x29, #-62]
	ldurb	w17, [x29, #-61]
	ldurb	w16, [x29, #-60]
	ldurb	w15, [x29, #-59]
	ldurb	w14, [x29, #-58]
	ldurb	w13, [x29, #-57]
	ldurb	w12, [x29, #-56]
	ldurb	w11, [x29, #-55]
	ldurb	w10, [x29, #-54]
	ldurb	w8, [x29, #-53]
	adrp	x0, .L.str.13
	add	x0, x0, :lo12:.L.str.13
	mov	x9, sp
	str	x9, [sp, #80]                   // 8-byte Folded Spill
	str	w17, [x9]
	str	w16, [x9, #8]
	str	w15, [x9, #16]
	str	w14, [x9, #24]
	str	w13, [x9, #32]
	str	w12, [x9, #40]
	str	w11, [x9, #48]
	str	w10, [x9, #56]
	str	w8, [x9, #64]
	bl	nand_print
	ldur	x9, [x29, #-88]
	mov	w8, #65535
	strh	w8, [x9, #2]
	ldur	x10, [x29, #-88]
	ldrh	w9, [x10]
	and	w9, w9, #0xfffff000
	orr	w9, w9, #0xfff
	strh	w9, [x10]
	mov	w9, #1
	stur	w9, [x29, #-52]
	sturh	w8, [x29, #-26]
	ldur	x9, [x29, #-80]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	b	.LBB1_44
.LBB1_41:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-40]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #520]
	bl	nand_write_logic_page_no_gc
	stur	w0, [x29, #-44]
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB1_43
// %bb.42:
	adrp	x0, .L.str.14
	add	x0, x0, :lo12:.L.str.14
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB1_57
.LBB1_43:                               //   in Loop: Header=BB1_19 Depth=1
.LBB1_44:                               //   in Loop: Header=BB1_19 Depth=1
	b	.LBB1_46
.LBB1_45:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	x9, [x29, #-80]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	mov	w8, #1
	stur	w8, [x29, #-52]
	mov	w8, #65535
	sturh	w8, [x29, #-26]
.LBB1_46:                               //   in Loop: Header=BB1_19 Depth=1
	ldurh	w9, [x29, #-26]
	mov	w8, #-1
	add	w8, w8, w9, uxth
	sturh	w8, [x29, #-26]
	ldurh	w8, [x29, #-26]
	cbnz	w8, .LBB1_48
// %bb.47:
	b	.LBB1_50
.LBB1_48:                               //   in Loop: Header=BB1_19 Depth=1
.LBB1_49:                               //   in Loop: Header=BB1_19 Depth=1
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	b	.LBB1_19
.LBB1_50:
	ldur	w8, [x29, #-32]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB1_52
// %bb.51:
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #686]
.LBB1_52:
	ldur	w8, [x29, #-52]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB1_55
// %bb.53:
	ldur	w8, [x29, #-32]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB1_55
// %bb.54:
	adrp	x0, .L.str.15
	add	x0, x0, :lo12:.L.str.15
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	b	.LBB1_56
.LBB1_55:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	bl	put_phy_block_to_invalid_page_list
	ldur	x0, [x29, #-16]
	bl	adjust_invaild_list
.LBB1_56:
	stur	wzr, [x29, #-4]
.LBB1_57:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #240]            // 16-byte Folded Reload
	add	sp, sp, #256                    // =256
	ret
.Lfunc_end1:
	.size	garbage_collect_first, .Lfunc_end1-garbage_collect_first
	.cfi_endproc
                                        // -- End function
	.globl	gc_win_block                    // -- Begin function gc_win_block
	.p2align	2
	.type	gc_win_block,@function
gc_win_block:                           // @gc_win_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	strh	w1, [sp, #14]
	str	wzr, [sp, #4]
	ldrh	w8, [sp, #14]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	mul	w8, w8, w9
	str	w8, [sp]
.LBB2_1:                                // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	ldr	w9, [sp]
	subs	w8, w8, w9
	b.ge	.LBB2_9
// %bb.2:                               //   in Loop: Header=BB2_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #352]
	cbnz	x8, .LBB2_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB2_10
.LBB2_4:                                //   in Loop: Header=BB2_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #352]
	ldrh	w9, [x8, #4]
	ldr	w8, [sp, #4]
	add	w8, w8, w9
	str	w8, [sp, #4]
	ldr	w8, [sp, #4]
	subs	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	ldr	x0, [sp, #16]
	mov	w1, #1
	bl	gc_one
	str	w0, [sp, #8]
	ldr	w8, [sp, #8]
	cbz	w8, .LBB2_8
// %bb.5:
	ldr	w8, [sp, #8]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB2_7
// %bb.6:
	adrp	x0, .L.str.30
	add	x0, x0, :lo12:.L.str.30
	bl	nand_print_dbg
.LBB2_7:
	ldr	w8, [sp, #8]
	stur	w8, [x29, #-4]
	b	.LBB2_10
.LBB2_8:                                //   in Loop: Header=BB2_1 Depth=1
	b	.LBB2_1
.LBB2_9:
	stur	wzr, [x29, #-4]
.LBB2_10:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end2:
	.size	gc_win_block, .Lfunc_end2-gc_win_block
	.cfi_endproc
                                        // -- End function
	.globl	out_phy_block_from_invalid_page_list // -- Begin function out_phy_block_from_invalid_page_list
	.p2align	2
	.type	out_phy_block_from_invalid_page_list,@function
out_phy_block_from_invalid_page_list:   // @out_phy_block_from_invalid_page_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	del_block_invalid_list
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbz	x8, .LBB3_2
// %bb.1:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #24]
	ldr	x8, [sp, #8]
	stur	x8, [x29, #-8]
	b	.LBB3_3
.LBB3_2:
	ldr	x8, [sp, #8]
	stur	x8, [x29, #-8]
.LBB3_3:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end3:
	.size	out_phy_block_from_invalid_page_list, .Lfunc_end3-out_phy_block_from_invalid_page_list
	.cfi_endproc
                                        // -- End function
	.globl	is_current_blcok                // -- Begin function is_current_blcok
	.p2align	2
	.type	is_current_blcok,@function
is_current_blcok:                       // @is_current_blcok
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	str	wzr, [sp, #12]
	ldr	x8, [sp, #24]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB4_5
// %bb.1:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #592]
	subs	x8, x8, x9
	b.eq	.LBB4_3
// %bb.2:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #528]
	subs	x8, x8, x9
	b.ne	.LBB4_4
.LBB4_3:
	mov	w8, #1
	str	w8, [sp, #12]
.LBB4_4:
	b	.LBB4_8
.LBB4_5:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #528]
	subs	x8, x8, x9
	b.ne	.LBB4_7
// %bb.6:
	mov	w8, #1
	str	w8, [sp, #12]
.LBB4_7:
.LBB4_8:
	ldr	w0, [sp, #12]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end4:
	.size	is_current_blcok, .Lfunc_end4-is_current_blcok
	.cfi_endproc
                                        // -- End function
	.globl	out_phy_block_from_invalid_page_list_by_block // -- Begin function out_phy_block_from_invalid_page_list_by_block
	.p2align	2
	.type	out_phy_block_from_invalid_page_list_by_block,@function
out_phy_block_from_invalid_page_list_by_block: // @out_phy_block_from_invalid_page_list_by_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x0, [sp, #16]
	ldr	x1, [sp, #8]
	bl	del_block_invalid_list_by_block
	str	x0, [sp]
	ldr	x8, [sp]
	cbz	x8, .LBB5_2
// %bb.1:
	ldr	x8, [sp]
	str	xzr, [x8, #16]
	ldr	x8, [sp]
	str	xzr, [x8, #24]
	ldr	x8, [sp]
	stur	x8, [x29, #-8]
	b	.LBB5_3
.LBB5_2:
	ldr	x8, [sp]
	stur	x8, [x29, #-8]
.LBB5_3:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end5:
	.size	out_phy_block_from_invalid_page_list_by_block, .Lfunc_end5-out_phy_block_from_invalid_page_list_by_block
	.cfi_endproc
                                        // -- End function
	.globl	set_physic_op_par               // -- Begin function set_physic_op_par
	.p2align	2
	.type	set_physic_op_par,@function
set_physic_op_par:                      // @set_physic_op_par
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	strh	w1, [sp, #22]
	strh	w2, [sp, #20]
	strh	w3, [sp, #18]
	str	x4, [sp, #8]
	str	x5, [sp]
	ldrh	w8, [sp, #20]
	ldr	x9, [sp, #24]
	strh	w8, [x9]
	ldrh	w8, [sp, #22]
	ldr	x9, [sp, #24]
	strh	w8, [x9, #2]
	ldrh	w8, [sp, #18]
	ldr	x9, [sp, #24]
	strh	w8, [x9, #4]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #24]
	str	x8, [x9, #8]
	ldr	x8, [sp]
	ldr	x9, [sp, #24]
	str	x8, [x9, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end6:
	.size	set_physic_op_par, .Lfunc_end6-set_physic_op_par
	.cfi_endproc
                                        // -- End function
	.globl	is_phy_mapping_page             // -- Begin function is_phy_mapping_page
	.p2align	2
	.type	is_phy_mapping_page,@function
is_phy_mapping_page:                    // @is_phy_mapping_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	get_special_data_from_oob
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	mov	w9, #-1431633921
	subs	w8, w8, w9
	b.ne	.LBB7_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB7_3
.LBB7_2:
	stur	wzr, [x29, #-4]
.LBB7_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end7:
	.size	is_phy_mapping_page, .Lfunc_end7-is_phy_mapping_page
	.cfi_endproc
                                        // -- End function
	.globl	recover_block_phy_page_mapping  // -- Begin function recover_block_phy_page_mapping
	.p2align	2
	.type	recover_block_phy_page_mapping,@function
recover_block_phy_page_mapping:         // @recover_block_phy_page_mapping
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	x2, [x29, #-24]
	stur	wzr, [x29, #-36]
	ldur	x0, [x29, #-24]
	ldur	x8, [x29, #-8]
	ldr	w2, [x8, #560]
	mov	w1, #-1
	bl	nand_memset
	stur	wzr, [x29, #-28]
.LBB8_1:                                // =>This Loop Header: Depth=1
                                        //     Child Loop BB8_6 Depth 2
	ldur	w8, [x29, #-28]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ge	.LBB8_30
// %bb.2:                               //   in Loop: Header=BB8_1 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #28]
	cbz	w8, .LBB8_4
// %bb.3:                               //   in Loop: Header=BB8_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-28]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	mov	x0, sp
	add	x5, sp, #28                     // =28
	bl	set_physic_op_par
	b	.LBB8_5
.LBB8_4:                                //   in Loop: Header=BB8_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-28]
	mov	x0, sp
	mov	w3, wzr
	mov	x4, xzr
	add	x5, sp, #28                     // =28
	bl	set_physic_op_par
.LBB8_5:                                //   in Loop: Header=BB8_1 Depth=1
	stur	wzr, [x29, #-36]
.LBB8_6:                                //   Parent Loop BB8_1 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-36]
	subs	w8, w8, #2                      // =2
	b.ge	.LBB8_14
// %bb.7:                               //   in Loop: Header=BB8_6 Depth=2
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	mov	x1, sp
	blr	x8
	stur	w0, [x29, #-32]
	ldur	w8, [x29, #-32]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB8_9
// %bb.8:                               //   in Loop: Header=BB8_6 Depth=2
	ldur	w1, [x29, #-32]
	ldur	x8, [x29, #-16]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-28]
	adrp	x0, .L.str.162
	add	x0, x0, :lo12:.L.str.162
	bl	nand_print
	b	.LBB8_10
.LBB8_9:                                //   in Loop: Header=BB8_1 Depth=1
	b	.LBB8_14
.LBB8_10:                               //   in Loop: Header=BB8_6 Depth=2
	ldur	w8, [x29, #-36]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB8_12
// %bb.11:                              //   in Loop: Header=BB8_6 Depth=2
	ldur	w1, [x29, #-32]
	ldur	x8, [x29, #-16]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-28]
	ldur	w4, [x29, #-36]
	adrp	x0, .L.str.163
	add	x0, x0, :lo12:.L.str.163
	bl	nand_print
.LBB8_12:                               //   in Loop: Header=BB8_6 Depth=2
// %bb.13:                              //   in Loop: Header=BB8_6 Depth=2
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB8_6
.LBB8_14:                               //   in Loop: Header=BB8_1 Depth=1
	add	x0, sp, #28                     // =28
	bl	is_ftl_logic_page_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB8_24
// %bb.15:                              //   in Loop: Header=BB8_1 Depth=1
	ldur	w8, [x29, #-32]
	cbz	w8, .LBB8_17
// %bb.16:                              //   in Loop: Header=BB8_1 Depth=1
	ldur	w8, [x29, #-32]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB8_23
.LBB8_17:                               //   in Loop: Header=BB8_1 Depth=1
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	ldr	x1, [x8, #512]
	add	x2, sp, #28                     // =28
	bl	nand_power_off_ecc_error
	cbz	w0, .LBB8_19
// %bb.18:                              //   in Loop: Header=BB8_1 Depth=1
	adrp	x0, .L.str.164
	add	x0, x0, :lo12:.L.str.164
	bl	nand_print
	b	.LBB8_22
.LBB8_19:                               //   in Loop: Header=BB8_1 Depth=1
	add	x0, sp, #28                     // =28
	bl	get_logic_page_from_oob
	ldur	x8, [x29, #-24]
	ldursw	x9, [x29, #-28]
	str	w0, [x8, x9, lsl #2]
	ldur	x8, [x29, #-24]
	ldursw	x9, [x29, #-28]
	ldr	w8, [x8, x9, lsl #2]
	ldur	x9, [x29, #-8]
	ldr	w9, [x9, #36]
	subs	w8, w8, w9
	b.lo	.LBB8_21
// %bb.20:                              //   in Loop: Header=BB8_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-28]
	ldur	x8, [x29, #-24]
	ldursw	x9, [x29, #-28]
	ldr	w3, [x8, x9, lsl #2]
	adrp	x0, .L.str.157
	add	x0, x0, :lo12:.L.str.157
	bl	nand_print
	ldur	x9, [x29, #-24]
	ldursw	x10, [x29, #-28]
	mov	w8, #-1
	str	w8, [x9, x10, lsl #2]
.LBB8_21:                               //   in Loop: Header=BB8_1 Depth=1
.LBB8_22:                               //   in Loop: Header=BB8_1 Depth=1
.LBB8_23:                               //   in Loop: Header=BB8_1 Depth=1
	b	.LBB8_28
.LBB8_24:                               //   in Loop: Header=BB8_1 Depth=1
	add	x0, sp, #28                     // =28
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB8_26
// %bb.25:                              //   in Loop: Header=BB8_1 Depth=1
	b	.LBB8_27
.LBB8_26:                               //   in Loop: Header=BB8_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-28]
	adrp	x0, .L.str.165
	add	x0, x0, :lo12:.L.str.165
	bl	nand_print
.LBB8_27:                               //   in Loop: Header=BB8_1 Depth=1
.LBB8_28:                               //   in Loop: Header=BB8_1 Depth=1
// %bb.29:                              //   in Loop: Header=BB8_1 Depth=1
	ldur	w8, [x29, #-28]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-28]
	b	.LBB8_1
.LBB8_30:
	mov	w0, wzr
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end8:
	.size	recover_block_phy_page_mapping, .Lfunc_end8-recover_block_phy_page_mapping
	.cfi_endproc
                                        // -- End function
	.globl	check_invalid_page_right        // -- Begin function check_invalid_page_right
	.p2align	2
	.type	check_invalid_page_right,@function
check_invalid_page_right:               // @check_invalid_page_right
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	str	x2, [sp, #32]
	strh	w3, [sp, #30]
	str	wzr, [sp, #16]
	str	wzr, [sp, #24]
.LBB9_1:                                // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #24]
	ldrh	w9, [sp, #30]
	subs	w8, w8, w9
	b.hs	.LBB9_12
// %bb.2:                               //   in Loop: Header=BB9_1 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8, #16]
	ldr	w9, [sp, #24]
                                        // kill: def $x9 killed $w9
	ldr	w8, [x8, x9, lsl #2]
	str	w8, [sp, #20]
	ldr	w8, [sp, #20]
	adds	w8, w8, #1                      // =1
	b.eq	.LBB9_9
// %bb.3:                               //   in Loop: Header=BB9_1 Depth=1
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #20]
	bl	get_logic_page_map
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB9_5
// %bb.4:                               //   in Loop: Header=BB9_1 Depth=1
	adrp	x0, .L.str.62
	add	x0, x0, :lo12:.L.str.62
	bl	nand_print_dbg
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB9_11
.LBB9_5:                                //   in Loop: Header=BB9_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #2]
	ldr	x9, [sp, #32]
	ldrh	w9, [x9]
	subs	w8, w8, w9
	b.ne	.LBB9_7
// %bb.6:                               //   in Loop: Header=BB9_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8]
	and	w8, w8, #0xfff
	and	w8, w8, #0xffff
	ldr	w9, [sp, #24]
	subs	w8, w8, w9
	b.eq	.LBB9_8
.LBB9_7:                                //   in Loop: Header=BB9_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
.LBB9_8:                                //   in Loop: Header=BB9_1 Depth=1
	b	.LBB9_10
.LBB9_9:                                //   in Loop: Header=BB9_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
.LBB9_10:                               //   in Loop: Header=BB9_1 Depth=1
.LBB9_11:                               //   in Loop: Header=BB9_1 Depth=1
	ldr	w8, [sp, #24]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #24]
	b	.LBB9_1
.LBB9_12:
	ldr	w8, [sp, #16]
	ldr	x9, [sp, #32]
	ldrh	w9, [x9, #4]
	subs	w8, w8, w9
	b.eq	.LBB9_14
// %bb.13:
	ldr	w1, [sp, #16]
	ldr	x8, [sp, #32]
	ldrh	w2, [x8, #4]
	ldrh	w3, [sp, #30]
	adrp	x0, .L.str.90
	add	x0, x0, :lo12:.L.str.90
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB9_15
.LBB9_14:
	stur	wzr, [x29, #-4]
.LBB9_15:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end9:
	.size	check_invalid_page_right, .Lfunc_end9-check_invalid_page_right
	.cfi_endproc
                                        // -- End function
	.globl	get_logic_page_map              // -- Begin function get_logic_page_map
	.p2align	2
	.type	get_logic_page_map,@function
get_logic_page_map:                     // @get_logic_page_map
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #36]
	subs	w8, w8, w9
	b.lo	.LBB10_2
// %bb.1:
	ldur	w2, [x29, #-20]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #36]
	adrp	x0, .L.str.39
	add	x0, x0, :lo12:.L.str.39
	adrp	x1, .L__func__.get_logic_page_map
	add	x1, x1, :lo12:.L__func__.get_logic_page_map
	bl	nand_print
	str	xzr, [sp, #16]
	ldr	x8, [sp, #16]
	stur	x8, [x29, #-8]
	b	.LBB10_6
.LBB10_2:
	mov	w8, #262144
	str	w8, [sp]
	str	wzr, [sp, #4]
.LBB10_3:                               // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldr	w9, [sp]
	subs	w8, w8, w9
	b.lo	.LBB10_5
// %bb.4:                               //   in Loop: Header=BB10_3 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	ldr	w9, [sp]
	ldur	w8, [x29, #-20]
	subs	w8, w8, w9
	stur	w8, [x29, #-20]
	b	.LBB10_3
.LBB10_5:
	ldur	x8, [x29, #-16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #2
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	stur	x8, [x29, #-8]
.LBB10_6:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end10:
	.size	get_logic_page_map, .Lfunc_end10-get_logic_page_map
	.cfi_endproc
                                        // -- End function
	.globl	get_logic_page_from_oob         // -- Begin function get_logic_page_from_oob
	.p2align	2
	.type	get_logic_page_from_oob,@function
get_logic_page_from_oob:                // @get_logic_page_from_oob
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	mov	w1, #1
	mov	w2, #4
	bl	get_spare_data
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	and	w8, w8, #0xfffffff
	str	w8, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end11:
	.size	get_logic_page_from_oob, .Lfunc_end11-get_logic_page_from_oob
	.cfi_endproc
                                        // -- End function
	.globl	nand_write_logic_page_no_gc     // -- Begin function nand_write_logic_page_no_gc
	.p2align	2
	.type	nand_write_logic_page_no_gc,@function
nand_write_logic_page_no_gc:            // @nand_write_logic_page_no_gc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	x2, [sp, #8]
	ldur	x0, [x29, #-8]
	ldur	w2, [x29, #-12]
	ldr	x3, [sp, #8]
	mov	w1, #192
	bl	nand_write_data_page
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end12:
	.size	nand_write_logic_page_no_gc, .Lfunc_end12-nand_write_logic_page_no_gc
	.cfi_endproc
                                        // -- End function
	.globl	erase_block                     // -- Begin function erase_block
	.p2align	2
	.type	erase_block,@function
erase_block:                            // @erase_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	sturh	w2, [x29, #-18]
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	adrp	x0, .L.str.192
	add	x0, x0, :lo12:.L.str.192
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #16                     // =16
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-8]
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB13_2
// %bb.1:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	adrp	x0, .L.str.193
	add	x0, x0, :lo12:.L.str.193
	bl	nand_print
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-8]
	add	x1, sp, #16                     // =16
	blr	x8
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-16]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-8]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
	b	.LBB13_3
.LBB13_2:
	ldurh	w8, [x29, #-18]
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #6]
	ldur	x9, [x29, #-16]
	mov	w8, #-1
	str	w8, [x9, #8]
	ldur	x0, [x29, #-8]
	ldur	x1, [x29, #-16]
	bl	put_phy_block_to_free_list
	ldur	x0, [x29, #-8]
	ldur	x1, [x29, #-16]
	bl	mark_free_block
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #4]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #2]
	ldur	x0, [x29, #-8]
	ldur	x1, [x29, #-16]
	bl	out_phy_block_from_used_list_by_block
	ldur	x0, [x29, #-8]
	ldur	x1, [x29, #-16]
	bl	out_phy_block_from_invalid_page_list_by_block
.LBB13_3:
	ldur	w0, [x29, #-24]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end13:
	.size	erase_block, .Lfunc_end13-erase_block
	.cfi_endproc
                                        // -- End function
	.globl	put_phy_block_to_invalid_page_list // -- Begin function put_phy_block_to_invalid_page_list
	.p2align	2
	.type	put_phy_block_to_invalid_page_list,@function
put_phy_block_to_invalid_page_list:     // @put_phy_block_to_invalid_page_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #24]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #16]
	bl	add_block_invalid_list
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbz	w8, .LBB14_2
// %bb.1:
	adrp	x0, .L.str.53
	add	x0, x0, :lo12:.L.str.53
	bl	nand_print_dbg
.LBB14_2:
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end14:
	.size	put_phy_block_to_invalid_page_list, .Lfunc_end14-put_phy_block_to_invalid_page_list
	.cfi_endproc
                                        // -- End function
	.globl	adjust_invaild_list             // -- Begin function adjust_invaild_list
	.p2align	2
	.type	adjust_invaild_list,@function
adjust_invaild_list:                    // @adjust_invaild_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
.LBB15_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #352]
	cbz	x8, .LBB15_6
// %bb.2:                               //   in Loop: Header=BB15_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #352]
	str	x8, [sp]
	ldr	x8, [sp]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ne	.LBB15_4
// %bb.3:                               //   in Loop: Header=BB15_1 Depth=1
	ldr	x0, [sp, #8]
	bl	out_phy_block_from_invalid_page_list
	str	x0, [sp]
	ldr	x0, [sp, #8]
	ldr	x1, [sp]
	bl	put_phy_block_to_free_list
	ldr	x0, [sp, #8]
	ldr	x1, [sp]
	bl	out_phy_block_from_used_list_by_block
	b	.LBB15_5
.LBB15_4:
	b	.LBB15_6
.LBB15_5:                               //   in Loop: Header=BB15_1 Depth=1
	b	.LBB15_1
.LBB15_6:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end15:
	.size	adjust_invaild_list, .Lfunc_end15-adjust_invaild_list
	.cfi_endproc
                                        // -- End function
	.globl	get_empty_gc_node               // -- Begin function get_empty_gc_node
	.p2align	2
	.type	get_empty_gc_node,@function
get_empty_gc_node:                      // @get_empty_gc_node
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	strh	wzr, [sp, #14]
.LBB16_1:                               // =>This Inner Loop Header: Depth=1
	ldrh	w8, [sp, #14]
	subs	w8, w8, #10                     // =10
	b.ge	.LBB16_6
// %bb.2:                               //   in Loop: Header=BB16_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldrh	w9, [sp, #14]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	ldrh	w8, [x8, #2]
	cbnz	w8, .LBB16_4
// %bb.3:
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldrh	w9, [sp, #14]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	x8, [sp, #24]
	b	.LBB16_7
.LBB16_4:                               //   in Loop: Header=BB16_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB16_1 Depth=1
	ldrh	w8, [sp, #14]
	add	w8, w8, #1                      // =1
	strh	w8, [sp, #14]
	b	.LBB16_1
.LBB16_6:
	str	xzr, [sp, #24]
.LBB16_7:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end16:
	.size	get_empty_gc_node, .Lfunc_end16-get_empty_gc_node
	.cfi_endproc
                                        // -- End function
	.globl	search_gc_node                  // -- Begin function search_gc_node
	.p2align	2
	.type	search_gc_node,@function
search_gc_node:                         // @search_gc_node
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #328                    // =328
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
.LBB17_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB17_6
// %bb.2:                               //   in Loop: Header=BB17_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #8]
	ldr	x9, [sp, #8]
	subs	x8, x8, x9
	b.ne	.LBB17_4
// %bb.3:
	ldr	x8, [sp]
	str	x8, [sp, #24]
	b	.LBB17_7
.LBB17_4:                               //   in Loop: Header=BB17_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB17_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB17_1
.LBB17_6:
	str	xzr, [sp, #24]
.LBB17_7:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end17:
	.size	search_gc_node, .Lfunc_end17-search_gc_node
	.cfi_endproc
                                        // -- End function
	.globl	add_to_gc_list_tail             // -- Begin function add_to_gc_list_tail
	.p2align	2
	.type	add_to_gc_list_tail,@function
add_to_gc_list_tail:                    // @add_to_gc_list_tail
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	add	x8, x8, #328                    // =328
	str	x8, [sp, #8]
.LBB18_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB18_3
// %bb.2:                               //   in Loop: Header=BB18_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB18_1
.LBB18_3:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	str	x8, [x9, #24]
	ldr	x9, [sp, #24]
	ldrh	w8, [x9]
	add	w8, w8, #1                      // =1
	strh	w8, [x9]
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end18:
	.size	add_to_gc_list_tail, .Lfunc_end18-add_to_gc_list_tail
	.cfi_endproc
                                        // -- End function
	.globl	del_from_gc_list                // -- Begin function del_from_gc_list
	.p2align	2
	.type	del_from_gc_list,@function
del_from_gc_list:                       // @del_from_gc_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB19_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #16]
	str	x8, [x9, #24]
.LBB19_2:
	ldr	x8, [sp, #16]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #24]
	ldr	x9, [sp, #24]
	ldrh	w8, [x9]
	subs	w8, w8, #1                      // =1
	strh	w8, [x9]
	ldr	x0, [sp, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end19:
	.size	del_from_gc_list, .Lfunc_end19-del_from_gc_list
	.cfi_endproc
                                        // -- End function
	.globl	add_prio_gc                     // -- Begin function add_prio_gc
	.p2align	2
	.type	add_prio_gc,@function
add_prio_gc:                            // @add_prio_gc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	x1, [sp, #24]
	strh	w2, [sp, #22]
	ldur	x8, [x29, #-16]
	add	x0, x8, #696                    // =696
	ldr	x1, [sp, #24]
	bl	search_gc_node
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbz	x8, .LBB20_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB20_5
.LBB20_2:
	ldur	x8, [x29, #-16]
	add	x0, x8, #696                    // =696
	bl	get_empty_gc_node
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB20_4
// %bb.3:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB20_5
.LBB20_4:
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #8]
	str	x8, [x9, #8]
	ldrh	w8, [sp, #22]
	ldr	x9, [sp, #8]
	strh	w8, [x9, #2]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #24]
	ldur	x8, [x29, #-16]
	add	x0, x8, #696                    // =696
	ldr	x1, [sp, #8]
	bl	add_to_gc_list_tail
	stur	wzr, [x29, #-4]
.LBB20_5:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end20:
	.size	add_prio_gc, .Lfunc_end20-add_prio_gc
	.cfi_endproc
                                        // -- End function
	.globl	do_prio_gc                      // -- Begin function do_prio_gc
	.p2align	2
	.type	do_prio_gc,@function
do_prio_gc:                             // @do_prio_gc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #1040]
	cbnz	x8, .LBB21_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_2:
	ldur	x8, [x29, #-16]
	add	x0, x8, #696                    // =696
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #1040]
	bl	del_from_gc_list
	str	x0, [sp, #16]
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	bl	is_current_blcok
	subs	w8, w0, #1                      // =1
	b.ne	.LBB21_4
// %bb.3:
	ldur	x8, [x29, #-16]
	add	x0, x8, #696                    // =696
	ldr	x1, [sp, #16]
	bl	add_to_gc_list_tail
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_4:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	cbnz	x8, .LBB21_6
// %bb.5:
	adrp	x0, .L.str.16
	add	x0, x0, :lo12:.L.str.16
	bl	nand_print_dbg
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_6:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	bl	is_block_in_invalid_list
	cbnz	w0, .LBB21_8
// %bb.7:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8]
	ldr	x8, [sp, #16]
	ldrh	w2, [x8, #2]
	adrp	x0, .L.str.17
	add	x0, x0, :lo12:.L.str.17
	bl	nand_print_dbg
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_8:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #2]
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #698]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #2]
	subs	w8, w8, #5                      // =5
	b.ne	.LBB21_10
// %bb.9:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	mov	w2, #4
	bl	garbage_collect_first
	stur	w0, [x29, #-20]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_10:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	mov	w2, #-1
	bl	garbage_collect_first
	stur	w0, [x29, #-20]
// %bb.11:
	ldur	w8, [x29, #-20]
	cbz	w8, .LBB21_13
// %bb.12:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8]
	ldr	x8, [sp, #16]
	ldrh	w2, [x8, #2]
	adrp	x0, .L.str.18
	add	x0, x0, :lo12:.L.str.18
	bl	nand_print
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB21_27
.LBB21_13:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	bl	is_block_in_free_list
	cbnz	w0, .LBB21_15
// %bb.14:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8]
	ldr	x8, [sp, #16]
	ldrh	w2, [x8, #2]
	adrp	x0, .L.str.19
	add	x0, x0, :lo12:.L.str.19
	bl	nand_print_dbg
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_15:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB21_17
// %bb.16:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8]
	ldr	x8, [sp, #16]
	ldrh	w2, [x8, #2]
	adrp	x0, .L.str.20
	add	x0, x0, :lo12:.L.str.20
	bl	nand_print_dbg
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	stur	wzr, [x29, #-4]
	b	.LBB21_27
.LBB21_17:
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #698]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB21_19
// %bb.18:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	b	.LBB21_26
.LBB21_19:
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #698]
	subs	w8, w8, #2                      // =2
	b.ne	.LBB21_21
// %bb.20:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	b	.LBB21_25
.LBB21_21:
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #698]
	subs	w8, w8, #4                      // =4
	b.ne	.LBB21_23
// %bb.22:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	b	.LBB21_24
.LBB21_23:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
.LBB21_24:
.LBB21_25:
.LBB21_26:
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #8]
	stur	wzr, [x29, #-4]
.LBB21_27:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end21:
	.size	do_prio_gc, .Lfunc_end21-do_prio_gc
	.cfi_endproc
                                        // -- End function
	.globl	is_block_in_invalid_list        // -- Begin function is_block_in_invalid_list
	.p2align	2
	.type	is_block_in_invalid_list,@function
is_block_in_invalid_list:               // @is_block_in_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #336                    // =336
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
.LBB22_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB22_6
// %bb.2:                               //   in Loop: Header=BB22_1 Depth=1
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	subs	x8, x8, x9
	b.ne	.LBB22_4
// %bb.3:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB22_7
.LBB22_4:                               //   in Loop: Header=BB22_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB22_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB22_1
.LBB22_6:
	str	wzr, [sp, #28]
.LBB22_7:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end22:
	.size	is_block_in_invalid_list, .Lfunc_end22-is_block_in_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	is_block_in_free_list           // -- Begin function is_block_in_free_list
	.p2align	2
	.type	is_block_in_free_list,@function
is_block_in_free_list:                  // @is_block_in_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #272                    // =272
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
.LBB23_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB23_6
// %bb.2:                               //   in Loop: Header=BB23_1 Depth=1
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	subs	x8, x8, x9
	b.ne	.LBB23_4
// %bb.3:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB23_7
.LBB23_4:                               //   in Loop: Header=BB23_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB23_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
	b	.LBB23_1
.LBB23_6:
	str	wzr, [sp, #28]
.LBB23_7:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end23:
	.size	is_block_in_free_list, .Lfunc_end23-is_block_in_free_list
	.cfi_endproc
                                        // -- End function
	.globl	out_phy_block_from_free_list_by_block // -- Begin function out_phy_block_from_free_list_by_block
	.p2align	2
	.type	out_phy_block_from_free_list_by_block,@function
out_phy_block_from_free_list_by_block:  // @out_phy_block_from_free_list_by_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x0, [sp, #16]
	ldr	x1, [sp, #8]
	bl	del_free_list_by_block
	str	x0, [sp]
	ldr	x8, [sp]
	cbz	x8, .LBB24_4
// %bb.1:
	ldr	x8, [sp]
	str	xzr, [x8, #32]
	ldr	x8, [sp]
	str	xzr, [x8, #40]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	subs	w8, w8, #0                      // =0
	b.le	.LBB24_3
// %bb.2:
	ldr	x9, [sp, #16]
	ldrh	w10, [x9, #30]
	mov	w8, #-1
	add	w8, w8, w10, uxth
	strh	w8, [x9, #30]
.LBB24_3:
	ldr	x8, [sp]
	stur	x8, [x29, #-8]
	b	.LBB24_5
.LBB24_4:
	ldr	x8, [sp]
	stur	x8, [x29, #-8]
.LBB24_5:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end24:
	.size	out_phy_block_from_free_list_by_block, .Lfunc_end24-out_phy_block_from_free_list_by_block
	.cfi_endproc
                                        // -- End function
	.globl	prio_gc_all                     // -- Begin function prio_gc_all
	.p2align	2
	.type	prio_gc_all,@function
prio_gc_all:                            // @prio_gc_all
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	str	wzr, [sp]
	str	wzr, [sp, #4]
.LBB25_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	subs	w8, w8, #10                     // =10
	b.hs	.LBB25_4
// %bb.2:                               //   in Loop: Header=BB25_1 Depth=1
	ldr	x0, [sp, #8]
	bl	do_prio_gc
	ldr	w8, [sp]
	orr	w8, w8, w0
	str	w8, [sp]
// %bb.3:                               //   in Loop: Header=BB25_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB25_1
.LBB25_4:
	ldr	w0, [sp]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end25:
	.size	prio_gc_all, .Lfunc_end25-prio_gc_all
	.cfi_endproc
                                        // -- End function
	.globl	check_static_wear_leveling      // -- Begin function check_static_wear_leveling
	.p2align	2
	.type	check_static_wear_leveling,@function
check_static_wear_leveling:             // @check_static_wear_leveling
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	xzr, [sp, #24]
	str	xzr, [sp, #16]
	mov	w8, #65535
	strh	w8, [sp, #14]
	strh	wzr, [sp, #12]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #1064]
	cbz	x8, .LBB26_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_2:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #352]
	cbz	x8, .LBB26_4
// %bb.3:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #304]
	cbnz	x8, .LBB26_5
.LBB26_4:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_5:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #26]
	subs	w8, w8, w9
	b.ge	.LBB26_7
// %bb.6:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_7:
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #16]
	bl	get_block_max_erase_time_from_free_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB26_9
// %bb.8:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_9:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #12]
	ldrh	w8, [sp, #12]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #16]
	mov	w10, #5
	sdiv	w9, w9, w10
	subs	w8, w8, w9
	b.ge	.LBB26_11
// %bb.10:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_11:
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #26]
	subs	w2, w8, w9
	bl	get_block_min_erase_time_from_invalid_list
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	cbnz	x8, .LBB26_13
// %bb.12:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_13:
	ldr	x8, [sp, #24]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #14]
	ldrh	w8, [sp, #12]
	ldrh	w9, [sp, #14]
	subs	w8, w8, w9
	b.gt	.LBB26_15
// %bb.14:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_15:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	lsl	w8, w8, #1
	ldrh	w9, [sp, #12]
	mov	w10, #3
	mul	w9, w9, w10
	subs	w8, w8, w9
	b.ge	.LBB26_17
// %bb.16:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	mov	w9, #12
	sdiv	w8, w8, w9
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #1058]
	b	.LBB26_24
.LBB26_17:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	ldrh	w9, [sp, #12]
	subs	w8, w8, w9, lsl #1
	b.ge	.LBB26_19
// %bb.18:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	mov	w9, #10
	sdiv	w8, w8, w9
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #1058]
	b	.LBB26_23
.LBB26_19:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	ldrh	w9, [sp, #12]
	mov	w10, #3
	mul	w9, w9, w10
	subs	w8, w8, w9
	b.ge	.LBB26_21
// %bb.20:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	mov	w9, #7
	sdiv	w8, w8, w9
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #1058]
	b	.LBB26_22
.LBB26_21:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	mov	w9, #5
	sdiv	w8, w8, w9
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #1058]
.LBB26_22:
.LBB26_23:
.LBB26_24:
	ldrh	w8, [sp, #12]
	ldrh	w9, [sp, #14]
	subs	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #1058]
	subs	w8, w8, w9
	b.ge	.LBB26_26
// %bb.25:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB26_27
.LBB26_26:
	ldrh	w1, [sp, #12]
	ldrh	w2, [sp, #14]
	adrp	x0, .L.str.21
	add	x0, x0, :lo12:.L.str.21
	bl	nand_print_dbg
	ldr	x8, [sp, #24]
	ldrh	w1, [x8]
	adrp	x0, .L.str.22
	add	x0, x0, :lo12:.L.str.22
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8, #26]
	adrp	x0, .L.str.23
	add	x0, x0, :lo12:.L.str.23
	bl	nand_print_dbg
	ldr	x8, [sp, #24]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #1064]
	stur	wzr, [x29, #-4]
.LBB26_27:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end26:
	.size	check_static_wear_leveling, .Lfunc_end26-check_static_wear_leveling
	.cfi_endproc
                                        // -- End function
	.globl	get_block_max_erase_time_from_free_list // -- Begin function get_block_max_erase_time_from_free_list
	.p2align	2
	.type	get_block_max_erase_time_from_free_list,@function
get_block_max_erase_time_from_free_list: // @get_block_max_erase_time_from_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	strh	w1, [sp, #30]
	str	xzr, [sp, #16]
	strh	wzr, [sp, #14]
	ldr	x8, [sp, #32]
	add	x8, x8, #272                    // =272
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
.LBB27_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB27_8
// %bb.2:                               //   in Loop: Header=BB27_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #30]
	subs	w8, w8, w9
	b.ge	.LBB27_6
// %bb.3:                               //   in Loop: Header=BB27_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #14]
	subs	w8, w8, w9
	b.lt	.LBB27_5
// %bb.4:                               //   in Loop: Header=BB27_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #14]
	ldr	x8, [sp]
	str	x8, [sp, #16]
.LBB27_5:                               //   in Loop: Header=BB27_1 Depth=1
.LBB27_6:                               //   in Loop: Header=BB27_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB27_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
	b	.LBB27_1
.LBB27_8:
	ldr	x8, [sp, #16]
	cbz	x8, .LBB27_10
// %bb.9:
	ldr	x8, [sp, #16]
	str	x8, [sp, #40]
	b	.LBB27_11
.LBB27_10:
	str	xzr, [sp, #40]
.LBB27_11:
	ldr	x0, [sp, #40]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end27:
	.size	get_block_max_erase_time_from_free_list, .Lfunc_end27-get_block_max_erase_time_from_free_list
	.cfi_endproc
                                        // -- End function
	.globl	get_block_min_erase_time_from_invalid_list // -- Begin function get_block_min_erase_time_from_invalid_list
	.p2align	2
	.type	get_block_min_erase_time_from_invalid_list,@function
get_block_min_erase_time_from_invalid_list: // @get_block_min_erase_time_from_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	strh	w1, [sp, #30]
	str	w2, [sp, #24]
	str	xzr, [sp, #16]
	ldr	x8, [sp, #32]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	strh	w8, [sp, #14]
	ldr	x8, [sp, #32]
	add	x8, x8, #336                    // =336
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
.LBB28_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB28_9
// %bb.2:                               //   in Loop: Header=BB28_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #30]
	subs	w8, w8, w9
	b.ge	.LBB28_7
// %bb.3:                               //   in Loop: Header=BB28_1 Depth=1
	ldr	x8, [sp]
	ldr	w8, [x8, #8]
	ldr	w9, [sp, #24]
	subs	w8, w8, w9
	b.hs	.LBB28_7
// %bb.4:                               //   in Loop: Header=BB28_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #14]
	subs	w8, w8, w9
	b.gt	.LBB28_6
// %bb.5:                               //   in Loop: Header=BB28_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #14]
	ldr	x8, [sp]
	str	x8, [sp, #16]
.LBB28_6:                               //   in Loop: Header=BB28_1 Depth=1
.LBB28_7:                               //   in Loop: Header=BB28_1 Depth=1
// %bb.8:                               //   in Loop: Header=BB28_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB28_1
.LBB28_9:
	ldr	x8, [sp, #16]
	cbz	x8, .LBB28_11
// %bb.10:
	ldr	x8, [sp, #16]
	str	x8, [sp, #40]
	b	.LBB28_12
.LBB28_11:
	str	xzr, [sp, #40]
.LBB28_12:
	ldr	x0, [sp, #40]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end28:
	.size	get_block_min_erase_time_from_invalid_list, .Lfunc_end28-get_block_min_erase_time_from_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	done_static_wear_leveling       // -- Begin function done_static_wear_leveling
	.p2align	2
	.type	done_static_wear_leveling,@function
done_static_wear_leveling:              // @done_static_wear_leveling
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	wzr, [sp, #12]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1064]
	cbnz	x8, .LBB29_2
// %bb.1:
	mov	w8, #2
	stur	w8, [x29, #-4]
	b	.LBB29_11
.LBB29_2:
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #1064]
	bl	is_current_blcok
	subs	w8, w0, #1                      // =1
	b.ne	.LBB29_4
// %bb.3:
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1064]
	mov	w8, #2
	stur	w8, [x29, #-4]
	b	.LBB29_11
.LBB29_4:
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #1064]
	bl	is_block_in_invalid_list
	cbnz	w0, .LBB29_6
// %bb.5:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1064]
	ldrh	w1, [x8]
	adrp	x0, .L.str.24
	add	x0, x0, :lo12:.L.str.24
	bl	nand_print_dbg
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1064]
	mov	w8, #2
	stur	w8, [x29, #-4]
	b	.LBB29_11
.LBB29_6:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1064]
	ldrh	w1, [x8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1064]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.25
	add	x0, x0, :lo12:.L.str.25
	bl	nand_print_dbg
	ldr	x9, [sp, #16]
	mov	w8, #1
	strh	w8, [x9, #1062]
	ldr	x8, [sp, #16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #128]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #128]
	ldr	x0, [sp, #16]
	bl	fill_no_use_data_to_current_block
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #1064]
	mov	w2, #-1
	bl	garbage_collect_first
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbz	w8, .LBB29_8
// %bb.7:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1064]
	ldrh	w1, [x8]
	adrp	x0, .L.str.26
	add	x0, x0, :lo12:.L.str.26
	bl	nand_print
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1064]
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB29_11
.LBB29_8:
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #1062]
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #1064]
	bl	is_block_in_free_list
	cbnz	w0, .LBB29_10
// %bb.9:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1064]
	ldrh	w1, [x8]
	adrp	x0, .L.str.27
	add	x0, x0, :lo12:.L.str.27
	bl	nand_print
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1064]
	stur	wzr, [x29, #-4]
	b	.LBB29_11
.LBB29_10:
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1064]
	stur	wzr, [x29, #-4]
.LBB29_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end29:
	.size	done_static_wear_leveling, .Lfunc_end29-done_static_wear_leveling
	.cfi_endproc
                                        // -- End function
	.globl	fill_no_use_data_to_current_block // -- Begin function fill_no_use_data_to_current_block
	.p2align	2
	.type	fill_no_use_data_to_current_block,@function
fill_no_use_data_to_current_block:      // @fill_no_use_data_to_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	wzr, [sp, #12]
.LBB30_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ge	.LBB30_6
// %bb.2:                               //   in Loop: Header=BB30_1 Depth=1
	ldr	x0, [sp, #16]
	bl	fill_no_use_data_to_page
	str	w0, [sp, #8]
	ldr	w8, [sp, #8]
	cbnz	w8, .LBB30_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB30_7
.LBB30_4:                               //   in Loop: Header=BB30_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB30_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB30_1
.LBB30_6:
	mov	w8, #1
	stur	w8, [x29, #-4]
.LBB30_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end30:
	.size	fill_no_use_data_to_current_block, .Lfunc_end30-fill_no_use_data_to_current_block
	.cfi_endproc
                                        // -- End function
	.globl	do_static_wear_leveling         // -- Begin function do_static_wear_leveling
	.p2align	2
	.type	do_static_wear_leveling,@function
do_static_wear_leveling:                // @do_static_wear_leveling
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #6]
	cbnz	w8, .LBB31_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB31_7
.LBB31_2:
	ldr	x8, [sp]
	ldrh	w8, [x8, #684]
	cbnz	w8, .LBB31_4
// %bb.3:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB31_7
.LBB31_4:
	ldr	x0, [sp]
	bl	check_static_wear_leveling
	cbnz	w0, .LBB31_6
// %bb.5:
	ldr	x0, [sp]
	bl	done_static_wear_leveling
	stur	w0, [x29, #-4]
	b	.LBB31_7
.LBB31_6:
	stur	wzr, [x29, #-4]
.LBB31_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end31:
	.size	do_static_wear_leveling, .Lfunc_end31-do_static_wear_leveling
	.cfi_endproc
                                        // -- End function
	.globl	prio_gc_one                     // -- Begin function prio_gc_one
	.p2align	2
	.type	prio_gc_one,@function
prio_gc_one:                            // @prio_gc_one
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	str	w2, [sp, #16]
	ldur	x0, [x29, #-8]
	ldurh	w1, [x29, #-10]
	bl	get_phy_block_addr
	str	x0, [sp, #8]
	ldr	w8, [sp, #16]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB32_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #16]
	b	.LBB32_9
.LBB32_2:
	ldr	w8, [sp, #16]
	subs	w8, w8, #2                      // =2
	b.ne	.LBB32_4
// %bb.3:
	mov	w8, #2
	str	w8, [sp, #16]
	b	.LBB32_8
.LBB32_4:
	ldr	w8, [sp, #16]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB32_6
// %bb.5:
	mov	w8, #3
	str	w8, [sp, #16]
	b	.LBB32_7
.LBB32_6:
	mov	w8, #3
	str	w8, [sp, #16]
.LBB32_7:
.LBB32_8:
.LBB32_9:
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #8]
	ldr	w2, [sp, #16]
	bl	add_prio_gc
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end32:
	.size	prio_gc_one, .Lfunc_end32-prio_gc_one
	.cfi_endproc
                                        // -- End function
	.globl	get_phy_block_addr              // -- Begin function get_phy_block_addr
	.p2align	2
	.type	get_phy_block_addr,@function
get_phy_block_addr:                     // @get_phy_block_addr
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	strh	w1, [sp, #6]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldrh	w9, [sp, #6]
                                        // kill: def $x9 killed $w9
	add	x0, x8, x9, lsl #6
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end33:
	.size	get_phy_block_addr, .Lfunc_end33-get_phy_block_addr
	.cfi_endproc
                                        // -- End function
	.globl	gc_one                          // -- Begin function gc_one
	.p2align	2
	.type	gc_one,@function
gc_one:                                 // @gc_one
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	sturh	w1, [x29, #-18]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #352]
	str	x8, [sp, #32]
	ldr	x8, [sp, #32]
	cbnz	x8, .LBB34_2
// %bb.1:
	mov	w8, #8
	stur	w8, [x29, #-4]
	b	.LBB34_9
.LBB34_2:
	ldr	x8, [sp, #32]
	ldrh	w8, [x8, #4]
	ldurh	w9, [x29, #-18]
	subs	w8, w8, w9
	b.le	.LBB34_8
// %bb.3:
	ldur	x0, [x29, #-16]
	mov	x1, xzr
	mov	w2, #-1
	bl	garbage_collect_first
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB34_5
// %bb.4:
	mov	w8, #8
	stur	w8, [x29, #-4]
	b	.LBB34_9
.LBB34_5:
	ldur	w8, [x29, #-24]
	cbnz	w8, .LBB34_7
// %bb.6:
	adrp	x8, gc_src_type
	ldr	w8, [x8, :lo12:gc_src_type]
	str	w8, [sp, #8]                    // 4-byte Folded Spill
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #30]
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #680]
	str	w8, [sp, #16]                   // 4-byte Folded Spill
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #682]
	str	w8, [sp, #20]                   // 4-byte Folded Spill
	bl	nand_get_cur_task_name
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	bl	nand_get_cur_task_pid
	ldr	w1, [sp, #8]                    // 4-byte Folded Reload
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	ldr	w3, [sp, #16]                   // 4-byte Folded Reload
	ldr	w4, [sp, #20]                   // 4-byte Folded Reload
	ldr	x5, [sp, #24]                   // 8-byte Folded Reload
	mov	w6, w0
	adrp	x0, .L.str.28
	add	x0, x0, :lo12:.L.str.28
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
	b	.LBB34_9
.LBB34_7:
	adrp	x0, .L.str.29
	add	x0, x0, :lo12:.L.str.29
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB34_9
.LBB34_8:
	mov	w8, #8
	stur	w8, [x29, #-4]
.LBB34_9:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end34:
	.size	gc_one, .Lfunc_end34-gc_one
	.cfi_endproc
                                        // -- End function
	.globl	gc_all_cnt                      // -- Begin function gc_all_cnt
	.p2align	2
	.type	gc_all_cnt,@function
gc_all_cnt:                             // @gc_all_cnt
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	str	w2, [sp, #16]
	adrp	x9, gc_src_type
	mov	w8, #255
	str	w8, [x9, :lo12:gc_src_type]
.LBB35_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	subs	w9, w8, #1                      // =1
	str	w9, [sp, #16]
	cbz	w8, .LBB35_5
// %bb.2:                               //   in Loop: Header=BB35_1 Depth=1
	ldur	x0, [x29, #-8]
	ldurh	w1, [x29, #-10]
	bl	gc_one
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB35_4
// %bb.3:
	b	.LBB35_5
.LBB35_4:                               //   in Loop: Header=BB35_1 Depth=1
	b	.LBB35_1
.LBB35_5:
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end35:
	.size	gc_all_cnt, .Lfunc_end35-gc_all_cnt
	.cfi_endproc
                                        // -- End function
	.globl	gc_all                          // -- Begin function gc_all
	.p2align	2
	.type	gc_all,@function
gc_all:                                 // @gc_all
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	strh	w1, [sp, #6]
	adrp	x9, gc_src_type
	mov	w8, #255
	str	w8, [x9, :lo12:gc_src_type]
.LBB36_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x0, [sp, #8]
	ldrh	w1, [sp, #6]
	bl	gc_one
	str	w0, [sp]
	ldr	w8, [sp]
	cbz	w8, .LBB36_3
// %bb.2:
	b	.LBB36_4
.LBB36_3:                               //   in Loop: Header=BB36_1 Depth=1
	b	.LBB36_1
.LBB36_4:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end36:
	.size	gc_all, .Lfunc_end36-gc_all
	.cfi_endproc
                                        // -- End function
	.globl	gc_all_enhance                  // -- Begin function gc_all_enhance
	.p2align	2
	.type	gc_all_enhance,@function
gc_all_enhance:                         // @gc_all_enhance
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	adrp	x8, phyinfo
	add	x8, x8, :lo12:phyinfo
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #30]
	str	w8, [sp, #20]
	ldr	w8, [sp, #20]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9
	b.hs	.LBB37_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB37_7
.LBB37_2:
	ldr	w8, [sp, #20]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #682]
	mov	w10, #3
	mul	w9, w9, w10
	subs	w8, w8, w9
	b.ls	.LBB37_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB37_7
.LBB37_4:
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #4]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #8]
	mul	w8, w8, w9
                                        // kill: def $x8 killed $w8
	str	x8, [sp, #24]
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #10]
	mov	w9, w8
	ldr	x8, [sp, #24]
	mul	x8, x8, x9
	str	x8, [sp, #24]
	ldr	x8, [sp, #24]
	mov	x9, #2048
	udiv	x8, x8, x9
	str	x8, [sp, #24]
	ldr	x8, [sp, #24]
	subs	x8, x8, #128                    // =128
	b.ls	.LBB37_6
// %bb.5:
	stur	wzr, [x29, #-4]
	b	.LBB37_7
.LBB37_6:
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	mov	w9, #2
	sdiv	w1, w8, w9
	bl	gc_all
	stur	w0, [x29, #-4]
.LBB37_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end37:
	.size	gc_all_enhance, .Lfunc_end37-gc_all_enhance
	.cfi_endproc
                                        // -- End function
	.globl	gc_all_base_on_free_blks        // -- Begin function gc_all_base_on_free_blks
	.p2align	2
	.type	gc_all_base_on_free_blks,@function
gc_all_base_on_free_blks:               // @gc_all_base_on_free_blks
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	adrp	x8, phyinfo
	add	x8, x8, :lo12:phyinfo
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #30]
	str	w8, [sp, #20]
	ldr	w8, [sp, #20]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9
	b.hs	.LBB38_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB38_7
.LBB38_2:
	ldr	w8, [sp, #20]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #682]
	mov	w10, #3
	mul	w9, w9, w10
	subs	w8, w8, w9
	b.ls	.LBB38_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB38_7
.LBB38_4:
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #4]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #8]
	mul	w8, w8, w9
                                        // kill: def $x8 killed $w8
	str	x8, [sp, #24]
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #10]
	mov	w9, w8
	ldr	x8, [sp, #24]
	mul	x8, x8, x9
	str	x8, [sp, #24]
	ldr	x8, [sp, #24]
	mov	x9, #2048
	udiv	x8, x8, x9
	str	x8, [sp, #24]
	ldr	x8, [sp, #24]
	subs	x8, x8, #128                    // =128
	b.ls	.LBB38_6
// %bb.5:
	stur	wzr, [x29, #-4]
	b	.LBB38_7
.LBB38_6:
	ldr	x8, [sp]
	ldrh	w8, [x8, #20]
	mov	w9, #2
	sdiv	w8, w8, w9
	mov	w2, #3
	subs	w8, w8, #3                      // =3
	str	w8, [sp, #16]
	ldr	w8, [sp, #20]
	ldur	x10, [x29, #-16]
	ldrh	w10, [x10, #680]
	subs	w10, w8, w10
	ldr	w8, [sp, #16]
	mul	w8, w8, w10
	str	w8, [sp, #16]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #682]
	mul	w8, w2, w8
	ldur	x10, [x29, #-16]
	ldrh	w10, [x10, #680]
	subs	w10, w8, w10
	ldr	w8, [sp, #16]
	udiv	w8, w8, w10
	str	w8, [sp, #16]
	ldr	x8, [sp]
	ldrh	w8, [x8, #20]
	sdiv	w9, w8, w9
	ldr	w8, [sp, #16]
	add	w8, w8, w9
	str	w8, [sp, #16]
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #16]
	bl	gc_all_cnt
	stur	w0, [x29, #-4]
.LBB38_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end38:
	.size	gc_all_base_on_free_blks, .Lfunc_end38-gc_all_base_on_free_blks
	.cfi_endproc
                                        // -- End function
	.globl	get_total_invalid_page          // -- Begin function get_total_invalid_page
	.p2align	2
	.type	get_total_invalid_page,@function
get_total_invalid_page:                 // @get_total_invalid_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	wzr, [sp, #12]
	ldr	x8, [sp, #24]
	add	x8, x8, #336                    // =336
	str	x8, [sp, #16]
.LBB39_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB39_4
// %bb.2:                               //   in Loop: Header=BB39_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	ldrh	w9, [x8, #4]
	ldr	w8, [sp, #12]
	add	w8, w8, w9
	str	w8, [sp, #12]
// %bb.3:                               //   in Loop: Header=BB39_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #16]
	b	.LBB39_1
.LBB39_4:
	ldr	w0, [sp, #12]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end39:
	.size	get_total_invalid_page, .Lfunc_end39-get_total_invalid_page
	.cfi_endproc
                                        // -- End function
	.globl	static_wear_leveling            // -- Begin function static_wear_leveling
	.p2align	2
	.type	static_wear_leveling,@function
static_wear_leveling:                   // @static_wear_leveling
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9
	b.gt	.LBB40_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB40_9
.LBB40_2:
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #668]
	subs	w8, w8, #20                     // =20
	b.lo	.LBB40_8
// %bb.3:
	ldr	x8, [sp, #16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #180]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #180]
	ldr	x0, [sp, #16]
	bl	get_block_min_count_from_invalid_list
	str	x0, [sp]
	ldr	x0, [sp, #16]
	ldr	x1, [sp]
	mov	w2, #5
	bl	add_prio_gc
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbnz	w8, .LBB40_5
// %bb.4:
	ldr	x9, [sp, #16]
	ldr	w8, [x9, #672]
	add	w8, w8, #4                      // =4
	str	w8, [x9, #672]
.LBB40_5:
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #672]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.lo	.LBB40_7
// %bb.6:
	ldr	x8, [sp, #16]
	str	wzr, [x8, #668]
	ldr	x8, [sp, #16]
	str	wzr, [x8, #672]
.LBB40_7:
.LBB40_8:
	stur	wzr, [x29, #-4]
.LBB40_9:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end40:
	.size	static_wear_leveling, .Lfunc_end40-static_wear_leveling
	.cfi_endproc
                                        // -- End function
	.globl	get_block_min_count_from_invalid_list // -- Begin function get_block_min_count_from_invalid_list
	.p2align	2
	.type	get_block_min_count_from_invalid_list,@function
get_block_min_count_from_invalid_list:  // @get_block_min_count_from_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	str	xzr, [sp, #24]
	mov	w8, #-1
	str	w8, [sp, #20]
	ldr	x8, [sp, #32]
	add	x8, x8, #336                    // =336
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
.LBB41_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB41_6
// %bb.2:                               //   in Loop: Header=BB41_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #8]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hi	.LBB41_4
// %bb.3:                               //   in Loop: Header=BB41_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #8]
	str	w8, [sp, #20]
	ldr	x8, [sp, #8]
	str	x8, [sp, #24]
.LBB41_4:                               //   in Loop: Header=BB41_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB41_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB41_1
.LBB41_6:
	ldr	x8, [sp, #24]
	cbz	x8, .LBB41_8
// %bb.7:
	ldr	x8, [sp, #24]
	str	x8, [sp, #40]
	b	.LBB41_9
.LBB41_8:
	str	xzr, [sp, #40]
.LBB41_9:
	ldr	x0, [sp, #40]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end41:
	.size	get_block_min_count_from_invalid_list, .Lfunc_end41-get_block_min_count_from_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	power_on_gc                     // -- Begin function power_on_gc
	.p2align	2
	.type	power_on_gc,@function
power_on_gc:                            // @power_on_gc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9
	b.lt	.LBB42_2
// %bb.1:
	ldr	x8, [sp, #16]
	ldrh	w1, [x8, #30]
	ldr	x8, [sp, #16]
	ldrh	w2, [x8, #680]
	ldr	x8, [sp, #16]
	ldrh	w3, [x8, #682]
	ldr	x8, [sp, #16]
	ldrh	w4, [x8, #26]
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #32]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	udiv	w5, w8, w9
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #40]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	udiv	w6, w8, w9
	adrp	x0, .L.str.31
	add	x0, x0, :lo12:.L.str.31
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
	b	.LBB42_13
.LBB42_2:
	adrp	x9, gc_src_type
	mov	w8, #2
	str	w8, [x9, :lo12:gc_src_type]
	strh	wzr, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w8, w8, #10                     // =10
	strh	w8, [sp, #10]
.LBB42_3:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9
	b.ge	.LBB42_7
// %bb.4:                               //   in Loop: Header=BB42_3 Depth=1
	ldr	x0, [sp, #16]
	ldrh	w1, [sp, #10]
	bl	gc_one
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB42_6
// %bb.5:
	b	.LBB42_7
.LBB42_6:                               //   in Loop: Header=BB42_3 Depth=1
	ldrh	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	strh	w8, [sp, #8]
	b	.LBB42_3
.LBB42_7:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	asr	w8, w8, #3
	strh	w8, [sp, #10]
.LBB42_8:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #30]
	subs	w8, w8, #5                      // =5
	b.ge	.LBB42_12
// %bb.9:                               //   in Loop: Header=BB42_8 Depth=1
	ldr	x8, [sp, #16]
	ldrh	w1, [x8, #30]
	adrp	x0, .L.str.32
	add	x0, x0, :lo12:.L.str.32
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	ldrh	w1, [sp, #10]
	bl	gc_one
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB42_11
// %bb.10:
	b	.LBB42_12
.LBB42_11:                              //   in Loop: Header=BB42_8 Depth=1
	ldrh	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	strh	w8, [sp, #8]
	b	.LBB42_8
.LBB42_12:
	ldrh	w1, [sp, #8]
	adrp	x0, .L.str.33
	add	x0, x0, :lo12:.L.str.33
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
.LBB42_13:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end42:
	.size	power_on_gc, .Lfunc_end42-power_on_gc
	.cfi_endproc
                                        // -- End function
	.globl	all_time_gc                     // -- Begin function all_time_gc
	.p2align	2
	.type	all_time_gc,@function
all_time_gc:                            // @all_time_gc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w8, w8, #3                      // =3
	strh	w8, [sp, #6]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #682]
	subs	w8, w8, w9
	b.ge	.LBB43_4
// %bb.1:
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #684]
	cbz	w8, .LBB43_3
// %bb.2:
	adrp	x9, gc_src_type
	mov	w8, #1
	str	w8, [x9, :lo12:gc_src_type]
	ldr	x0, [sp, #8]
	ldrh	w1, [sp, #6]
	bl	gc_one
.LBB43_3:
.LBB43_4:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end43:
	.size	all_time_gc, .Lfunc_end43-all_time_gc
	.cfi_endproc
                                        // -- End function
	.globl	shutdown_on_gc                  // -- Begin function shutdown_on_gc
	.p2align	2
	.type	shutdown_on_gc,@function
shutdown_on_gc:                         // @shutdown_on_gc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #352]
	str	x8, [sp]
	adrp	x9, gc_src_type
	mov	w8, #4
	str	w8, [x9, :lo12:gc_src_type]
	ldr	x2, [sp]
	ldr	x8, [sp, #8]
	ldrh	w3, [x8, #684]
	ldr	x8, [sp, #8]
	ldrh	w4, [x8, #30]
	ldr	x8, [sp, #8]
	ldrh	w5, [x8, #680]
	ldr	x8, [sp, #8]
	ldrh	w6, [x8, #682]
	adrp	x0, .L.str.34
	add	x0, x0, :lo12:.L.str.34
	adrp	x1, .L__func__.shutdown_on_gc
	add	x1, x1, :lo12:.L__func__.shutdown_on_gc
	bl	nand_print_dbg
.LBB44_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #682]
	subs	w8, w8, w9
	b.gt	.LBB44_7
// %bb.2:                               //   in Loop: Header=BB44_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #352]
	str	x8, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB44_4
// %bb.3:
	ldr	x8, [sp, #8]
	ldrh	w1, [x8, #30]
	ldr	x8, [sp, #8]
	ldrh	w2, [x8, #680]
	ldr	x8, [sp, #8]
	ldrh	w3, [x8, #682]
	adrp	x0, .L.str.35
	add	x0, x0, :lo12:.L.str.35
	bl	nand_print
	b	.LBB44_7
.LBB44_4:                               //   in Loop: Header=BB44_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #684]
	cbz	w8, .LBB44_6
// %bb.5:                               //   in Loop: Header=BB44_1 Depth=1
	ldr	x0, [sp, #8]
	mov	w1, wzr
	bl	gc_one
	ldr	x8, [sp, #8]
	ldrh	w1, [x8, #30]
	ldr	x8, [sp, #8]
	ldrh	w2, [x8, #680]
	ldr	x8, [sp, #8]
	ldrh	w3, [x8, #682]
	adrp	x0, .L.str.36
	add	x0, x0, :lo12:.L.str.36
	bl	nand_print_dbg
.LBB44_6:                               //   in Loop: Header=BB44_1 Depth=1
	b	.LBB44_1
.LBB44_7:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end44:
	.size	shutdown_on_gc, .Lfunc_end44-shutdown_on_gc
	.cfi_endproc
                                        // -- End function
	.globl	nftl_get_gc_info                // -- Begin function nftl_get_gc_info
	.p2align	2
	.type	nftl_get_gc_info,@function
nftl_get_gc_info:                       // @nftl_get_gc_info
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	w2, [x29, #-20]
	stur	wzr, [x29, #-24]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	str	x8, [sp, #40]
	stur	wzr, [x29, #-40]
	stur	wzr, [x29, #-36]
	stur	wzr, [x29, #-28]
	stur	wzr, [x29, #-32]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #352]
	str	x8, [sp, #48]
.LBB45_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #48]
	cbz	x8, .LBB45_5
// %bb.2:                               //   in Loop: Header=BB45_1 Depth=1
	ldr	x8, [sp, #48]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #40]
	ldrh	w9, [x9, #20]
	mov	w10, #2
	sdiv	w9, w9, w10
	subs	w8, w8, w9
	b.le	.LBB45_4
// %bb.3:                               //   in Loop: Header=BB45_1 Depth=1
	ldr	x8, [sp, #48]
	ldrh	w9, [x8, #4]
	ldur	w8, [x29, #-36]
	add	w8, w8, w9
	stur	w8, [x29, #-36]
	ldur	w8, [x29, #-28]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-28]
.LBB45_4:                               //   in Loop: Header=BB45_1 Depth=1
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	ldr	x8, [sp, #48]
	ldrh	w9, [x8, #4]
	ldur	w8, [x29, #-40]
	add	w8, w8, w9
	stur	w8, [x29, #-40]
	ldr	x8, [sp, #48]
	ldr	x8, [x8, #16]
	str	x8, [sp, #48]
	b	.LBB45_1
.LBB45_5:
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-24]
	add	x0, x8, x9
	ldur	w8, [x29, #-20]
	ldur	w9, [x29, #-24]
	subs	w1, w8, w9
	ldur	x8, [x29, #-8]
	ldrh	w3, [x8, #30]
	ldur	x8, [x29, #-8]
	ldrh	w4, [x8, #680]
	ldur	x8, [x29, #-8]
	ldrh	w5, [x8, #682]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3440]
	ldr	w6, [x8, #120]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3440]
	ldr	w7, [x8, #124]
	ldur	w12, [x29, #-28]
	ldur	w11, [x29, #-32]
	ldur	w10, [x29, #-36]
	ldur	w8, [x29, #-40]
	adrp	x2, .L.str.37
	add	x2, x2, :lo12:.L.str.37
	mov	x9, sp
	str	w12, [x9]
	str	w11, [x9, #8]
	str	w10, [x9, #16]
	str	w8, [x9, #24]
	bl	nand_snprint
	stur	w0, [x29, #-24]
	stur	wzr, [x29, #-28]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #352]
	str	x8, [sp, #48]
.LBB45_6:                               // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	subs	w8, w8, #1                      // =1
	cbz	w8, .LBB45_13
// %bb.7:                               //   in Loop: Header=BB45_6 Depth=1
	ldr	x8, [sp, #48]
	cbz	x8, .LBB45_9
// %bb.8:                               //   in Loop: Header=BB45_6 Depth=1
	ldr	x8, [sp, #48]
	ldrh	w8, [x8, #4]
	cbnz	w8, .LBB45_10
.LBB45_9:
	b	.LBB45_13
.LBB45_10:                              //   in Loop: Header=BB45_6 Depth=1
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-24]
	add	x0, x8, x9
	ldur	w8, [x29, #-20]
	ldur	w9, [x29, #-24]
	subs	w1, w8, w9
	ldr	x8, [sp, #48]
	ldrh	w3, [x8]
	ldr	x8, [sp, #48]
	ldrh	w4, [x8, #4]
	adrp	x2, .L.str.38
	add	x2, x2, :lo12:.L.str.38
	bl	nand_snprint
	ldur	w8, [x29, #-24]
	add	w8, w8, w0
	stur	w8, [x29, #-24]
	ldur	w8, [x29, #-28]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-28]
	ldur	w8, [x29, #-28]
	mov	w10, #8
	sdiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	cbnz	w8, .LBB45_12
// %bb.11:                              //   in Loop: Header=BB45_6 Depth=1
	ldur	x8, [x29, #-16]
	ldur	w9, [x29, #-24]
	subs	w9, w9, #1                      // =1
	add	x9, x8, w9, sxtw
	mov	w8, #10
	strb	w8, [x9]
.LBB45_12:                              //   in Loop: Header=BB45_6 Depth=1
	ldr	x8, [sp, #48]
	ldr	x8, [x8, #16]
	str	x8, [sp, #48]
	b	.LBB45_6
.LBB45_13:
	ldur	x8, [x29, #-16]
	ldur	w9, [x29, #-24]
	subs	w9, w9, #1                      // =1
	add	x9, x8, w9, sxtw
	mov	w8, #10
	strb	w8, [x9]
	ldur	w0, [x29, #-24]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end45:
	.size	nftl_get_gc_info, .Lfunc_end45-nftl_get_gc_info
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_copy_page            // -- Begin function _nftl_nand_copy_page
	.p2align	2
	.type	_nftl_nand_copy_page,@function
_nftl_nand_copy_page:                   // @_nftl_nand_copy_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #176                    // =176
	stp	x29, x30, [sp, #160]            // 16-byte Folded Spill
	add	x29, sp, #160                   // =160
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	x2, [x29, #-24]
	stur	x3, [x29, #-32]
	sturh	w4, [x29, #-34]
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldurh	w2, [x29, #-34]
	add	x0, sp, #64                     // =64
	str	x0, [sp, #32]                   // 8-byte Folded Spill
	mov	w3, wzr
	str	w3, [sp, #20]                   // 4-byte Folded Spill
	mov	x5, xzr
	str	x5, [sp, #24]                   // 8-byte Folded Spill
	mov	x4, x5
	bl	set_physic_op_par
	ldr	w3, [sp, #20]                   // 4-byte Folded Reload
	ldr	x5, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldurh	w2, [x29, #-34]
	add	x0, sp, #40                     // =40
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #32]                   // 8-byte Folded Reload
	stur	wzr, [x29, #-40]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #28]
	strh	w8, [sp, #68]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #28]
	strh	w8, [sp, #44]
	ldur	x8, [x29, #-32]
	str	x8, [sp, #72]
	ldur	x8, [x29, #-32]
	str	x8, [sp, #48]
	sub	x8, x29, #64                    // =64
	str	x8, [sp, #80]
	str	x8, [sp, #56]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	blr	x8
	stur	w0, [x29, #-44]
	ldur	w8, [x29, #-44]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB46_2
// %bb.1:
	stur	wzr, [x29, #-44]
.LBB46_2:
	sub	x0, x29, #64                    // =64
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	bl	get_special_data_from_oob
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	stur	w0, [x29, #-68]
	ldur	x0, [x29, #-8]
	ldur	w2, [x29, #-68]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w4, w8, #1                      // =1
	bl	set_oob_special_page
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	add	x1, sp, #40                     // =40
	blr	x8
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-44]
	ldur	w9, [x29, #-48]
	orr	w8, w8, w9, lsl #8
	stur	w8, [x29, #-40]
	ldur	w0, [x29, #-40]
	ldp	x29, x30, [sp, #160]            // 16-byte Folded Reload
	add	sp, sp, #176                    // =176
	ret
.Lfunc_end46:
	.size	_nftl_nand_copy_page, .Lfunc_end46-_nftl_nand_copy_page
	.cfi_endproc
                                        // -- End function
	.globl	get_special_data_from_oob       // -- Begin function get_special_data_from_oob
	.p2align	2
	.type	get_special_data_from_oob,@function
get_special_data_from_oob:              // @get_special_data_from_oob
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	mov	w1, #1
	mov	w2, #4
	bl	get_spare_data
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end47:
	.size	get_special_data_from_oob, .Lfunc_end47-get_special_data_from_oob
	.cfi_endproc
                                        // -- End function
	.globl	set_oob_special_page            // -- Begin function set_oob_special_page
	.p2align	2
	.type	set_oob_special_page,@function
set_oob_special_page:                   // @set_oob_special_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	w2, [sp, #12]
	str	w3, [sp, #8]
	strh	w4, [sp, #6]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strb	w8, [x9]
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #12]
	mov	w2, #1
	mov	w3, #4
	str	w3, [sp]                        // 4-byte Folded Spill
	bl	set_spare_data
	ldr	x0, [sp, #16]
	ldrh	w1, [sp, #6]
	mov	w2, #5
	mov	w3, #2
	bl	set_spare_data
	ldr	w3, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #8]
	mov	w2, #7
	bl	set_spare_data
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end48:
	.size	set_oob_special_page, .Lfunc_end48-set_oob_special_page
	.cfi_endproc
                                        // -- End function
	.globl	set_spare_data                  // -- Begin function set_spare_data
	.p2align	2
	.type	set_spare_data,@function
set_spare_data:                         // @set_spare_data
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	w1, [sp, #20]
	strb	w2, [sp, #19]
	strb	w3, [sp, #18]
	str	wzr, [sp, #8]
.LBB49_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #8]
	ldrb	w9, [sp, #18]
	subs	w8, w8, w9
	b.hs	.LBB49_4
// %bb.2:                               //   in Loop: Header=BB49_1 Depth=1
	ldrb	w8, [sp, #18]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	str	w8, [sp, #12]
	ldr	w8, [sp, #12]
	subs	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	ldr	w8, [sp, #12]
	lsl	w8, w8, #3
	str	w8, [sp, #12]
	ldr	w8, [sp, #20]
	ldr	w9, [sp, #12]
	lsr	w8, w8, w9
	ldr	x9, [sp, #24]
	ldrb	w10, [sp, #19]
	ldr	w11, [sp, #8]
	add	w10, w10, w11
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB49_1 Depth=1
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
	b	.LBB49_1
.LBB49_4:
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end49:
	.size	set_spare_data, .Lfunc_end49-set_spare_data
	.cfi_endproc
                                        // -- End function
	.globl	get_spare_data                  // -- Begin function get_spare_data
	.p2align	2
	.type	get_spare_data,@function
get_spare_data:                         // @get_spare_data
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	strb	w1, [sp, #23]
	strb	w2, [sp, #22]
	str	wzr, [sp, #12]
	str	wzr, [sp, #16]
.LBB50_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	ldrb	w9, [sp, #22]
	subs	w8, w8, w9
	b.hs	.LBB50_4
// %bb.2:                               //   in Loop: Header=BB50_1 Depth=1
	ldr	w8, [sp, #16]
	lsl	w8, w8, #8
	str	w8, [sp, #16]
	ldr	x8, [sp, #24]
	ldrb	w9, [sp, #23]
	ldr	w10, [sp, #12]
	add	w9, w9, w10
	ldrb	w9, [x8, w9, uxtw]
	ldr	w8, [sp, #16]
	orr	w8, w8, w9
	str	w8, [sp, #16]
// %bb.3:                               //   in Loop: Header=BB50_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB50_1
.LBB50_4:
	ldr	w0, [sp, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end50:
	.size	get_spare_data, .Lfunc_end50-get_spare_data
	.cfi_endproc
                                        // -- End function
	.globl	set_oob_logic_page              // -- Begin function set_oob_logic_page
	.p2align	2
	.type	set_oob_logic_page,@function
set_oob_logic_page:                     // @set_oob_logic_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	w2, [sp, #12]
	str	w3, [sp, #8]
	strh	w4, [sp, #6]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strb	w8, [x9]
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #12]
	mov	w2, #1
	mov	w3, #4
	str	w3, [sp]                        // 4-byte Folded Spill
	bl	set_spare_data
	ldr	x0, [sp, #16]
	ldrh	w1, [sp, #6]
	mov	w2, #5
	mov	w3, #2
	bl	set_spare_data
	ldr	w3, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #8]
	mov	w2, #7
	bl	set_spare_data
	ldr	x9, [sp, #16]
	ldrb	w8, [x9, #1]
	orr	w8, w8, #0xc0
	strb	w8, [x9, #1]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end51:
	.size	set_oob_logic_page, .Lfunc_end51-set_oob_logic_page
	.cfi_endproc
                                        // -- End function
	.globl	set_no_use_page                 // -- Begin function set_no_use_page
	.p2align	2
	.type	set_no_use_page,@function
set_no_use_page:                        // @set_no_use_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strb	w8, [x9]
	ldr	x0, [sp, #16]
	mov	w1, #170
	mov	w3, #1
	str	w3, [sp, #12]                   // 4-byte Folded Spill
	mov	w2, w3
	bl	set_spare_data
	ldr	w3, [sp, #12]                   // 4-byte Folded Reload
	ldr	x0, [sp, #16]
	mov	w1, #187
	mov	w2, #2
	bl	set_spare_data
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end52:
	.size	set_no_use_page, .Lfunc_end52-set_no_use_page
	.cfi_endproc
                                        // -- End function
	.globl	set_start_block_flag            // -- Begin function set_start_block_flag
	.p2align	2
	.type	set_start_block_flag,@function
set_start_block_flag:                   // @set_start_block_flag
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	w2, [sp, #12]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strb	w8, [x9]
	ldr	x0, [sp, #16]
	mov	w1, #170
	mov	w3, #1
	str	w3, [sp, #8]                    // 4-byte Folded Spill
	mov	w2, w3
	bl	set_spare_data
	ldr	w3, [sp, #8]                    // 4-byte Folded Reload
	ldr	x0, [sp, #16]
	mov	w1, #238
	mov	w2, #2
	bl	set_spare_data
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #12]
	mov	w2, #3
	mov	w3, #4
	bl	set_spare_data
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end53:
	.size	set_start_block_flag, .Lfunc_end53-set_start_block_flag
	.cfi_endproc
                                        // -- End function
	.globl	set_end_block_flag              // -- Begin function set_end_block_flag
	.p2align	2
	.type	set_end_block_flag,@function
set_end_block_flag:                     // @set_end_block_flag
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strb	w8, [x9]
	ldr	x0, [sp, #16]
	mov	w1, #170
	mov	w3, #1
	str	w3, [sp, #12]                   // 4-byte Folded Spill
	mov	w2, w3
	bl	set_spare_data
	ldr	w3, [sp, #12]                   // 4-byte Folded Reload
	ldr	x0, [sp, #16]
	mov	w1, #204
	mov	w2, #2
	bl	set_spare_data
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end54:
	.size	set_end_block_flag, .Lfunc_end54-set_end_block_flag
	.cfi_endproc
                                        // -- End function
	.globl	get_bad_flag_from_oob           // -- Begin function get_bad_flag_from_oob
	.p2align	2
	.type	get_bad_flag_from_oob,@function
get_bad_flag_from_oob:                  // @get_bad_flag_from_oob
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	mov	w1, #1
	mov	w2, #4
	bl	get_spare_data
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbnz	w8, .LBB55_2
// %bb.1:
	sturb	wzr, [x29, #-1]
	b	.LBB55_3
.LBB55_2:
	mov	w8, #1
	sturb	w8, [x29, #-1]
.LBB55_3:
	ldurb	w0, [x29, #-1]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end55:
	.size	get_bad_flag_from_oob, .Lfunc_end55-get_bad_flag_from_oob
	.cfi_endproc
                                        // -- End function
	.globl	get_block_used_count_from_oob   // -- Begin function get_block_used_count_from_oob
	.p2align	2
	.type	get_block_used_count_from_oob,@function
get_block_used_count_from_oob:          // @get_block_used_count_from_oob
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	mov	w1, #7
	mov	w2, #4
	bl	get_spare_data
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end56:
	.size	get_block_used_count_from_oob, .Lfunc_end56-get_block_used_count_from_oob
	.cfi_endproc
                                        // -- End function
	.globl	get_erase_count_from_oob        // -- Begin function get_erase_count_from_oob
	.p2align	2
	.type	get_erase_count_from_oob,@function
get_erase_count_from_oob:               // @get_erase_count_from_oob
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	mov	w1, #5
	mov	w2, #2
	bl	get_spare_data
	strh	w0, [sp, #6]
	ldrh	w0, [sp, #6]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end57:
	.size	get_erase_count_from_oob, .Lfunc_end57-get_erase_count_from_oob
	.cfi_endproc
                                        // -- End function
	.globl	is_ftl_start_flag_page          // -- Begin function is_ftl_start_flag_page
	.p2align	2
	.type	is_ftl_start_flag_page,@function
is_ftl_start_flag_page:                 // @is_ftl_start_flag_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp]
	ldr	x8, [sp]
	ldrb	w8, [x8, #1]
	subs	w8, w8, #170                    // =170
	b.ne	.LBB58_3
// %bb.1:
	ldr	x8, [sp]
	ldrb	w8, [x8, #2]
	subs	w8, w8, #238                    // =238
	b.ne	.LBB58_3
// %bb.2:
	mov	w8, #1
	str	w8, [sp, #12]
	b	.LBB58_4
.LBB58_3:
	str	wzr, [sp, #12]
.LBB58_4:
	ldr	w0, [sp, #12]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end58:
	.size	is_ftl_start_flag_page, .Lfunc_end58-is_ftl_start_flag_page
	.cfi_endproc
                                        // -- End function
	.globl	is_ftl_end_flag_page            // -- Begin function is_ftl_end_flag_page
	.p2align	2
	.type	is_ftl_end_flag_page,@function
is_ftl_end_flag_page:                   // @is_ftl_end_flag_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp]
	ldr	x8, [sp]
	ldrb	w8, [x8, #1]
	subs	w8, w8, #170                    // =170
	b.ne	.LBB59_3
// %bb.1:
	ldr	x8, [sp]
	ldrb	w8, [x8, #2]
	subs	w8, w8, #204                    // =204
	b.ne	.LBB59_3
// %bb.2:
	mov	w8, #1
	str	w8, [sp, #12]
	b	.LBB59_4
.LBB59_3:
	str	wzr, [sp, #12]
.LBB59_4:
	ldr	w0, [sp, #12]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end59:
	.size	is_ftl_end_flag_page, .Lfunc_end59-is_ftl_end_flag_page
	.cfi_endproc
                                        // -- End function
	.globl	is_ftl_special_data             // -- Begin function is_ftl_special_data
	.p2align	2
	.type	is_ftl_special_data,@function
is_ftl_special_data:                    // @is_ftl_special_data
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp]
	ldr	x8, [sp]
	ldrb	w8, [x8, #1]
	subs	w8, w8, #170                    // =170
	b.ne	.LBB60_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #12]
	b	.LBB60_3
.LBB60_2:
	str	wzr, [sp, #12]
.LBB60_3:
	ldr	w0, [sp, #12]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end60:
	.size	is_ftl_special_data, .Lfunc_end60-is_ftl_special_data
	.cfi_endproc
                                        // -- End function
	.globl	is_ftl_logic_page_data          // -- Begin function is_ftl_logic_page_data
	.p2align	2
	.type	is_ftl_logic_page_data,@function
is_ftl_logic_page_data:                 // @is_ftl_logic_page_data
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp]
	ldr	x8, [sp]
	ldrb	w8, [x8, #1]
	and	w8, w8, #0xf0
	subs	w8, w8, #192                    // =192
	b.ne	.LBB61_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #12]
	b	.LBB61_3
.LBB61_2:
	str	wzr, [sp, #12]
.LBB61_3:
	ldr	w0, [sp, #12]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end61:
	.size	is_ftl_logic_page_data, .Lfunc_end61-is_ftl_logic_page_data
	.cfi_endproc
                                        // -- End function
	.globl	is_nouse_page                   // -- Begin function is_nouse_page
	.p2align	2
	.type	is_nouse_page,@function
is_nouse_page:                          // @is_nouse_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	wzr, [sp, #8]
	str	wzr, [sp, #12]
.LBB62_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w8, w8, #10                     // =10
	b.hs	.LBB62_6
// %bb.2:                               //   in Loop: Header=BB62_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	w9, [sp, #12]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB62_4
// %bb.3:                               //   in Loop: Header=BB62_1 Depth=1
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
.LBB62_4:                               //   in Loop: Header=BB62_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB62_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB62_1
.LBB62_6:
	ldr	w8, [sp, #8]
	subs	w8, w8, #9                      // =9
	b.lo	.LBB62_9
// %bb.7:
	ldr	x8, [sp, #16]
	ldrb	w8, [x8]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB62_9
// %bb.8:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB62_10
.LBB62_9:
	str	wzr, [sp, #28]
.LBB62_10:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end62:
	.size	is_nouse_page, .Lfunc_end62-is_nouse_page
	.cfi_endproc
                                        // -- End function
	.globl	is_fill_page                    // -- Begin function is_fill_page
	.p2align	2
	.type	is_fill_page,@function
is_fill_page:                           // @is_fill_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	get_special_data_from_oob
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	mov	w9, #1431655765
	subs	w8, w8, w9
	b.ne	.LBB63_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB63_3
.LBB63_2:
	stur	wzr, [x29, #-4]
.LBB63_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end63:
	.size	is_fill_page, .Lfunc_end63-is_fill_page
	.cfi_endproc
                                        // -- End function
	.globl	is_function_info_page           // -- Begin function is_function_info_page
	.p2align	2
	.type	is_function_info_page,@function
is_function_info_page:                  // @is_function_info_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	get_special_data_from_oob
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	and	w8, w8, #0xffff0000
	mov	w9, #-1441726464
	subs	w8, w8, w9
	b.ne	.LBB64_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB64_3
.LBB64_2:
	stur	wzr, [x29, #-4]
.LBB64_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end64:
	.size	is_function_info_page, .Lfunc_end64-is_function_info_page
	.cfi_endproc
                                        // -- End function
	.globl	is_power_down_info_page         // -- Begin function is_power_down_info_page
	.p2align	2
	.type	is_power_down_info_page,@function
is_power_down_info_page:                // @is_power_down_info_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	get_special_data_from_oob
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	mov	w9, #13107
	movk	w9, #43537, lsl #16
	subs	w8, w8, w9
	b.ne	.LBB65_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB65_3
.LBB65_2:
	stur	wzr, [x29, #-4]
.LBB65_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end65:
	.size	is_power_down_info_page, .Lfunc_end65-is_power_down_info_page
	.cfi_endproc
                                        // -- End function
	.globl	malloc_logic_page_map           // -- Begin function malloc_logic_page_map
	.p2align	2
	.type	malloc_logic_page_map,@function
malloc_logic_page_map:                  // @malloc_logic_page_map
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	mov	w8, #262144
	str	w8, [sp]
	str	wzr, [sp, #4]
.LBB66_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	subs	w8, w8, #25                     // =25
	b.hs	.LBB66_4
// %bb.2:                               //   in Loop: Header=BB66_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #3
	str	xzr, [x8]
// %bb.3:                               //   in Loop: Header=BB66_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB66_1
.LBB66_4:
	str	wzr, [sp, #4]
.LBB66_5:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	subs	w8, w8, #25                     // =25
	b.hs	.LBB66_15
// %bb.6:                               //   in Loop: Header=BB66_5 Depth=1
	ldr	w8, [sp, #12]
	ldr	w9, [sp]
	subs	w8, w8, w9
	b.lo	.LBB66_10
// %bb.7:                               //   in Loop: Header=BB66_5 Depth=1
	ldr	w8, [sp]
                                        // kill: def $x8 killed $w8
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w8, w8, #2
	str	w8, [sp, #8]
	ldr	w0, [sp, #8]
	bl	nftl_malloc
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #3
	str	x0, [x8]
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	cbnz	x8, .LBB66_9
// %bb.8:
	ldr	w1, [sp, #4]
	adrp	x0, .L.str.40
	add	x0, x0, :lo12:.L.str.40
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB66_18
.LBB66_9:                               //   in Loop: Header=BB66_5 Depth=1
	ldr	w9, [sp]
	ldr	w8, [sp, #12]
	subs	w8, w8, w9
	str	w8, [sp, #12]
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	ldr	w2, [sp, #8]
	mov	w1, #-1
	bl	nand_memset
	b	.LBB66_13
.LBB66_10:
	ldr	w8, [sp, #12]
                                        // kill: def $x8 killed $w8
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w8, w8, #2
	str	w8, [sp, #8]
	ldr	w0, [sp, #8]
	bl	nftl_malloc
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #3
	str	x0, [x8]
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	cbnz	x8, .LBB66_12
// %bb.11:
	ldr	w1, [sp, #4]
	adrp	x0, .L.str.41
	add	x0, x0, :lo12:.L.str.41
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB66_18
.LBB66_12:
	ldr	x8, [sp, #16]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	ldr	w2, [sp, #8]
	mov	w1, #-1
	bl	nand_memset
	str	wzr, [sp, #12]
	b	.LBB66_15
.LBB66_13:                              //   in Loop: Header=BB66_5 Depth=1
// %bb.14:                              //   in Loop: Header=BB66_5 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB66_5
.LBB66_15:
	ldr	w8, [sp, #12]
	cbz	w8, .LBB66_17
// %bb.16:
	ldr	w1, [sp, #12]
	adrp	x0, .L.str.42
	add	x0, x0, :lo12:.L.str.42
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB66_18
.LBB66_17:
	stur	wzr, [x29, #-4]
.LBB66_18:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end66:
	.size	malloc_logic_page_map, .Lfunc_end66-malloc_logic_page_map
	.cfi_endproc
                                        // -- End function
	.globl	nftl_malloc                     // -- Begin function nftl_malloc
	.p2align	2
	.type	nftl_malloc,@function
nftl_malloc:                            // @nftl_malloc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	w0, [x29, #-4]
	ldur	w8, [x29, #-4]
	subs	w8, w8, #384, lsl #12           // =1572864
	b.ls	.LBB67_2
// %bb.1:
	ldur	w1, [x29, #-4]
	adrp	x0, .L.str.371
	add	x0, x0, :lo12:.L.str.371
	bl	nand_print
.LBB67_2:
	ldur	w0, [x29, #-4]
	bl	nand_malloc
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end67:
	.size	nftl_malloc, .Lfunc_end67-nftl_malloc
	.cfi_endproc
                                        // -- End function
	.globl	free_logic_page_map             // -- Begin function free_logic_page_map
	.p2align	2
	.type	free_logic_page_map,@function
free_logic_page_map:                    // @free_logic_page_map
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	str	wzr, [sp, #4]
.LBB68_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	subs	w8, w8, #25                     // =25
	b.hs	.LBB68_6
// %bb.2:                               //   in Loop: Header=BB68_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	cbz	x8, .LBB68_4
// %bb.3:                               //   in Loop: Header=BB68_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #72                     // =72
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	bl	nftl_free
.LBB68_4:                               //   in Loop: Header=BB68_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB68_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB68_1
.LBB68_6:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end68:
	.size	free_logic_page_map, .Lfunc_end68-free_logic_page_map
	.cfi_endproc
                                        // -- End function
	.globl	nftl_free                       // -- Begin function nftl_free
	.p2align	2
	.type	nftl_free,@function
nftl_free:                              // @nftl_free
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	nand_free
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end69:
	.size	nftl_free, .Lfunc_end69-nftl_free
	.cfi_endproc
                                        // -- End function
	.globl	add_free_list                   // -- Begin function add_free_list
	.p2align	2
	.type	add_free_list,@function
add_free_list:                          // @add_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #272                    // =272
	str	x8, [sp]
.LBB70_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	cbz	x8, .LBB70_6
// %bb.2:                               //   in Loop: Header=BB70_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	ldrh	w8, [x8, #6]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #6]
	subs	w8, w8, w9
	b.lt	.LBB70_4
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	ldr	x9, [x9, #32]
	str	x8, [x9, #40]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	ldr	x9, [sp, #8]
	str	x8, [x9, #32]
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	str	x8, [x9, #32]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #40]
	str	wzr, [sp, #28]
	b	.LBB70_7
.LBB70_4:                               //   in Loop: Header=BB70_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB70_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
	b	.LBB70_1
.LBB70_6:
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	str	x8, [x9, #32]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #32]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #40]
	str	wzr, [sp, #28]
.LBB70_7:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end70:
	.size	add_free_list, .Lfunc_end70-add_free_list
	.cfi_endproc
                                        // -- End function
	.globl	del_free_list                   // -- Begin function del_free_list
	.p2align	2
	.type	del_free_list,@function
del_free_list:                          // @del_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #272                    // =272
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
	ldr	x8, [sp]
	cbz	x8, .LBB71_4
// %bb.1:
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	ldr	x9, [sp, #8]
	str	x8, [x9, #304]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	ldr	x9, [sp, #8]
	str	x8, [x9, #304]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	cbz	x8, .LBB71_3
// %bb.2:
	ldr	x8, [sp, #8]
	add	x8, x8, #272                    // =272
	ldr	x9, [sp]
	ldr	x9, [x9, #32]
	str	x8, [x9, #40]
.LBB71_3:
.LBB71_4:
	ldr	x0, [sp]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end71:
	.size	del_free_list, .Lfunc_end71-del_free_list
	.cfi_endproc
                                        // -- End function
	.globl	del_free_list_by_block          // -- Begin function del_free_list_by_block
	.p2align	2
	.type	del_free_list_by_block,@function
del_free_list_by_block:                 // @del_free_list_by_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	add	x8, x8, #272                    // =272
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #32]
	str	x8, [sp, #8]
.LBB72_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB72_8
// %bb.2:                               //   in Loop: Header=BB72_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.ne	.LBB72_6
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #32]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #40]
	str	x8, [x9, #32]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #32]
	cbz	x8, .LBB72_5
// %bb.4:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #40]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #32]
	str	x8, [x9, #40]
.LBB72_5:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #40]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #32]
	b	.LBB72_8
.LBB72_6:                               //   in Loop: Header=BB72_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB72_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #32]
	str	x8, [sp, #8]
	b	.LBB72_1
.LBB72_8:
	ldr	x0, [sp, #8]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end72:
	.size	del_free_list_by_block, .Lfunc_end72-del_free_list_by_block
	.cfi_endproc
                                        // -- End function
	.globl	print_free_list                 // -- Begin function print_free_list
	.p2align	2
	.type	print_free_list,@function
print_free_list:                        // @print_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #272                    // =272
	str	x8, [sp]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #656]
	cbnz	w8, .LBB73_2
// %bb.1:
	b	.LBB73_6
.LBB73_2:
	adrp	x0, .L.str.43
	add	x0, x0, :lo12:.L.str.43
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
.LBB73_3:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB73_6
// %bb.4:                               //   in Loop: Header=BB73_3 Depth=1
	ldr	x8, [sp]
	ldrh	w1, [x8]
	ldr	x8, [sp]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.44
	add	x0, x0, :lo12:.L.str.44
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldr	w1, [x8, #8]
	adrp	x0, .L.str.45
	add	x0, x0, :lo12:.L.str.45
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldrh	w1, [x8, #2]
	adrp	x0, .L.str.46
	add	x0, x0, :lo12:.L.str.46
	bl	nand_print_dbg
// %bb.5:                               //   in Loop: Header=BB73_3 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
	b	.LBB73_3
.LBB73_6:
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end73:
	.size	print_free_list, .Lfunc_end73-print_free_list
	.cfi_endproc
                                        // -- End function
	.globl	add_block_invalid_list          // -- Begin function add_block_invalid_list
	.p2align	2
	.type	add_block_invalid_list,@function
add_block_invalid_list:                 // @add_block_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #336                    // =336
	str	x8, [sp]
.LBB74_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB74_6
// %bb.2:                               //   in Loop: Header=BB74_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #4]
	subs	w8, w8, w9
	b.gt	.LBB74_4
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	ldr	x9, [x9, #16]
	str	x8, [x9, #24]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	str	x8, [x9, #16]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #24]
	str	wzr, [sp, #28]
	b	.LBB74_7
.LBB74_4:                               //   in Loop: Header=BB74_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB74_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB74_1
.LBB74_6:
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	str	x8, [x9, #16]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #16]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #24]
	str	wzr, [sp, #28]
.LBB74_7:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end74:
	.size	add_block_invalid_list, .Lfunc_end74-add_block_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	del_block_invalid_list          // -- Begin function del_block_invalid_list
	.p2align	2
	.type	del_block_invalid_list,@function
del_block_invalid_list:                 // @del_block_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #336                    // =336
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	ldr	x8, [sp]
	cbz	x8, .LBB75_4
// %bb.1:
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #352]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB75_3
// %bb.2:
	ldr	x8, [sp, #8]
	add	x8, x8, #336                    // =336
	ldr	x9, [sp]
	ldr	x9, [x9, #16]
	str	x8, [x9, #24]
.LBB75_3:
.LBB75_4:
	ldr	x0, [sp]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end75:
	.size	del_block_invalid_list, .Lfunc_end75-del_block_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	del_block_invalid_list_by_block // -- Begin function del_block_invalid_list_by_block
	.p2align	2
	.type	del_block_invalid_list_by_block,@function
del_block_invalid_list_by_block:        // @del_block_invalid_list_by_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	add	x8, x8, #336                    // =336
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
.LBB76_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB76_8
// %bb.2:                               //   in Loop: Header=BB76_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.ne	.LBB76_6
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #24]
	str	x8, [x9, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB76_5
// %bb.4:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #16]
	str	x8, [x9, #24]
.LBB76_5:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #24]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #16]
	b	.LBB76_8
.LBB76_6:                               //   in Loop: Header=BB76_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB76_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB76_1
.LBB76_8:
	ldr	x0, [sp, #8]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end76:
	.size	del_block_invalid_list_by_block, .Lfunc_end76-del_block_invalid_list_by_block
	.cfi_endproc
                                        // -- End function
	.globl	print_block_invalid_list        // -- Begin function print_block_invalid_list
	.p2align	2
	.type	print_block_invalid_list,@function
print_block_invalid_list:               // @print_block_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #336                    // =336
	str	x8, [sp]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #656]
	cbnz	w8, .LBB77_2
// %bb.1:
	b	.LBB77_6
.LBB77_2:
	adrp	x0, .L.str.47
	add	x0, x0, :lo12:.L.str.47
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
.LBB77_3:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB77_6
// %bb.4:                               //   in Loop: Header=BB77_3 Depth=1
	ldr	x8, [sp]
	ldrh	w1, [x8]
	ldr	x8, [sp]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.44
	add	x0, x0, :lo12:.L.str.44
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldr	w1, [x8, #8]
	ldr	x8, [sp]
	ldrh	w2, [x8, #4]
	adrp	x0, .L.str.48
	add	x0, x0, :lo12:.L.str.48
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldrh	w1, [x8, #2]
	adrp	x0, .L.str.46
	add	x0, x0, :lo12:.L.str.46
	bl	nand_print_dbg
// %bb.5:                               //   in Loop: Header=BB77_3 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB77_3
.LBB77_6:
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end77:
	.size	print_block_invalid_list, .Lfunc_end77-print_block_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	add_block_count_list            // -- Begin function add_block_count_list
	.p2align	2
	.type	add_block_count_list,@function
add_block_count_list:                   // @add_block_count_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #400                    // =400
	str	x8, [sp]
.LBB78_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	cbz	x8, .LBB78_6
// %bb.2:                               //   in Loop: Header=BB78_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	ldr	w8, [x8, #8]
	ldr	x9, [sp, #8]
	ldr	w9, [x9, #8]
	subs	w8, w8, w9
	b.lt	.LBB78_4
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	ldr	x9, [x9, #48]
	str	x8, [x9, #56]
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	ldr	x9, [sp, #8]
	str	x8, [x9, #48]
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	str	x8, [x9, #48]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #56]
	str	wzr, [sp, #28]
	b	.LBB78_7
.LBB78_4:                               //   in Loop: Header=BB78_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB78_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	str	x8, [sp]
	b	.LBB78_1
.LBB78_6:
	ldr	x8, [sp, #8]
	ldr	x9, [sp]
	str	x8, [x9, #48]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #48]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #56]
	str	wzr, [sp, #28]
.LBB78_7:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end78:
	.size	add_block_count_list, .Lfunc_end78-add_block_count_list
	.cfi_endproc
                                        // -- End function
	.globl	del_block_count_list            // -- Begin function del_block_count_list
	.p2align	2
	.type	del_block_count_list,@function
del_block_count_list:                   // @del_block_count_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #400                    // =400
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	cbz	x8, .LBB79_2
// %bb.1:
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	ldr	x8, [x8, #48]
	ldr	x9, [sp]
	str	x8, [x9, #48]
.LBB79_2:
	ldr	x0, [sp]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end79:
	.size	del_block_count_list, .Lfunc_end79-del_block_count_list
	.cfi_endproc
                                        // -- End function
	.globl	del_block_count_by_block        // -- Begin function del_block_count_by_block
	.p2align	2
	.type	del_block_count_by_block,@function
del_block_count_by_block:               // @del_block_count_by_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	add	x8, x8, #400                    // =400
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #48]
	str	x8, [sp, #8]
.LBB80_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB80_8
// %bb.2:                               //   in Loop: Header=BB80_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.ne	.LBB80_6
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #48]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #56]
	str	x8, [x9, #48]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #48]
	cbz	x8, .LBB80_5
// %bb.4:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #56]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #48]
	str	x8, [x9, #56]
.LBB80_5:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #56]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #48]
	b	.LBB80_8
.LBB80_6:                               //   in Loop: Header=BB80_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB80_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #48]
	str	x8, [sp, #8]
	b	.LBB80_1
.LBB80_8:
	ldr	x0, [sp, #8]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end80:
	.size	del_block_count_by_block, .Lfunc_end80-del_block_count_by_block
	.cfi_endproc
                                        // -- End function
	.globl	print_block_count_list          // -- Begin function print_block_count_list
	.p2align	2
	.type	print_block_count_list,@function
print_block_count_list:                 // @print_block_count_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #400                    // =400
	str	x8, [sp]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #656]
	cbnz	w8, .LBB81_2
// %bb.1:
	b	.LBB81_6
.LBB81_2:
	adrp	x0, .L.str.49
	add	x0, x0, :lo12:.L.str.49
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	str	x8, [sp]
.LBB81_3:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB81_6
// %bb.4:                               //   in Loop: Header=BB81_3 Depth=1
	ldr	x8, [sp]
	ldrh	w1, [x8]
	adrp	x0, .L.str.50
	add	x0, x0, :lo12:.L.str.50
	bl	nand_print_dbg
	ldr	x8, [sp]
	ldr	w1, [x8, #8]
	adrp	x0, .L.str.51
	add	x0, x0, :lo12:.L.str.51
	bl	nand_print_dbg
// %bb.5:                               //   in Loop: Header=BB81_3 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #48]
	str	x8, [sp]
	b	.LBB81_3
.LBB81_6:
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end81:
	.size	print_block_count_list, .Lfunc_end81-print_block_count_list
	.cfi_endproc
                                        // -- End function
	.globl	put_phy_block_to_free_list      // -- Begin function put_phy_block_to_free_list
	.p2align	2
	.type	put_phy_block_to_free_list,@function
put_phy_block_to_free_list:             // @put_phy_block_to_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #32]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #40]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #16]
	bl	add_free_list
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbnz	w8, .LBB82_2
// %bb.1:
	ldur	x9, [x29, #-8]
	ldrh	w8, [x9, #30]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #30]
	b	.LBB82_3
.LBB82_2:
	adrp	x0, .L.str.52
	add	x0, x0, :lo12:.L.str.52
	bl	nand_print_dbg
.LBB82_3:
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end82:
	.size	put_phy_block_to_free_list, .Lfunc_end82-put_phy_block_to_free_list
	.cfi_endproc
                                        // -- End function
	.globl	out_phy_block_from_free_list    // -- Begin function out_phy_block_from_free_list
	.p2align	2
	.type	out_phy_block_from_free_list,@function
out_phy_block_from_free_list:           // @out_phy_block_from_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	del_free_list
	str	x0, [sp]
	ldr	x8, [sp]
	cbz	x8, .LBB83_4
// %bb.1:
	ldr	x8, [sp]
	str	xzr, [x8, #32]
	ldr	x8, [sp]
	str	xzr, [x8, #40]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #30]
	subs	w8, w8, #0                      // =0
	b.le	.LBB83_3
// %bb.2:
	ldr	x9, [sp, #8]
	ldrh	w10, [x9, #30]
	mov	w8, #-1
	add	w8, w8, w10, uxth
	strh	w8, [x9, #30]
.LBB83_3:
.LBB83_4:
	ldr	x0, [sp]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end83:
	.size	out_phy_block_from_free_list, .Lfunc_end83-out_phy_block_from_free_list
	.cfi_endproc
                                        // -- End function
	.globl	out_phy_block_from_free_list_by_erase_time // -- Begin function out_phy_block_from_free_list_by_erase_time
	.p2align	2
	.type	out_phy_block_from_free_list_by_erase_time,@function
out_phy_block_from_free_list_by_erase_time: // @out_phy_block_from_free_list_by_erase_time
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	str	xzr, [sp, #24]
	strh	wzr, [sp, #22]
	ldur	x8, [x29, #-8]
	add	x8, x8, #272                    // =272
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #32]
	str	x8, [sp, #8]
.LBB84_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB84_8
// %bb.2:                               //   in Loop: Header=BB84_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	ldurh	w9, [x29, #-10]
	subs	w8, w8, w9
	b.ge	.LBB84_6
// %bb.3:                               //   in Loop: Header=BB84_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #22]
	subs	w8, w8, w9
	b.lt	.LBB84_5
// %bb.4:                               //   in Loop: Header=BB84_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #22]
	ldr	x8, [sp, #8]
	str	x8, [sp, #24]
.LBB84_5:                               //   in Loop: Header=BB84_1 Depth=1
.LBB84_6:                               //   in Loop: Header=BB84_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB84_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #32]
	str	x8, [sp, #8]
	b	.LBB84_1
.LBB84_8:
	ldr	x8, [sp, #24]
	cbz	x8, .LBB84_10
// %bb.9:
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #24]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [sp, #24]
.LBB84_10:
	ldr	x0, [sp, #24]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end84:
	.size	out_phy_block_from_free_list_by_erase_time, .Lfunc_end84-out_phy_block_from_free_list_by_erase_time
	.cfi_endproc
                                        // -- End function
	.globl	get_max_erase_time_from_free_list // -- Begin function get_max_erase_time_from_free_list
	.p2align	2
	.type	get_max_erase_time_from_free_list,@function
get_max_erase_time_from_free_list:      // @get_max_erase_time_from_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	strh	w1, [sp, #30]
	str	xzr, [sp, #16]
	strh	wzr, [sp, #14]
	ldr	x8, [sp, #32]
	add	x8, x8, #272                    // =272
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
.LBB85_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB85_8
// %bb.2:                               //   in Loop: Header=BB85_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #30]
	subs	w8, w8, w9
	b.ge	.LBB85_6
// %bb.3:                               //   in Loop: Header=BB85_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #14]
	subs	w8, w8, w9
	b.lt	.LBB85_5
// %bb.4:                               //   in Loop: Header=BB85_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #14]
	ldr	x8, [sp]
	str	x8, [sp, #16]
.LBB85_5:                               //   in Loop: Header=BB85_1 Depth=1
.LBB85_6:                               //   in Loop: Header=BB85_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB85_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #32]
	str	x8, [sp]
	b	.LBB85_1
.LBB85_8:
	ldr	x8, [sp, #16]
	cbz	x8, .LBB85_10
// %bb.9:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #46]
	b	.LBB85_11
.LBB85_10:
	mov	w8, #65535
	strh	w8, [sp, #46]
.LBB85_11:
	ldrh	w0, [sp, #46]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end85:
	.size	get_max_erase_time_from_free_list, .Lfunc_end85-get_max_erase_time_from_free_list
	.cfi_endproc
                                        // -- End function
	.globl	get_block_max_erase_time_from_invalid_list // -- Begin function get_block_max_erase_time_from_invalid_list
	.p2align	2
	.type	get_block_max_erase_time_from_invalid_list,@function
get_block_max_erase_time_from_invalid_list: // @get_block_max_erase_time_from_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	strh	w1, [sp, #30]
	str	xzr, [sp, #16]
	strh	wzr, [sp, #14]
	ldr	x8, [sp, #32]
	add	x8, x8, #336                    // =336
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
.LBB86_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB86_8
// %bb.2:                               //   in Loop: Header=BB86_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #30]
	subs	w8, w8, w9
	b.ge	.LBB86_6
// %bb.3:                               //   in Loop: Header=BB86_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	ldrh	w9, [sp, #14]
	subs	w8, w8, w9
	b.lt	.LBB86_5
// %bb.4:                               //   in Loop: Header=BB86_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	strh	w8, [sp, #14]
	ldr	x8, [sp]
	str	x8, [sp, #16]
.LBB86_5:                               //   in Loop: Header=BB86_1 Depth=1
.LBB86_6:                               //   in Loop: Header=BB86_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB86_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB86_1
.LBB86_8:
	ldr	x8, [sp, #16]
	cbz	x8, .LBB86_10
// %bb.9:
	ldr	x8, [sp, #16]
	str	x8, [sp, #40]
	b	.LBB86_11
.LBB86_10:
	str	xzr, [sp, #40]
.LBB86_11:
	ldr	x0, [sp, #40]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end86:
	.size	get_block_max_erase_time_from_invalid_list, .Lfunc_end86-get_block_max_erase_time_from_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	get_block_max_count_from_invalid_list // -- Begin function get_block_max_count_from_invalid_list
	.p2align	2
	.type	get_block_max_count_from_invalid_list,@function
get_block_max_count_from_invalid_list:  // @get_block_max_count_from_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	str	xzr, [sp, #24]
	str	wzr, [sp, #20]
	ldr	x8, [sp, #32]
	add	x8, x8, #336                    // =336
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
.LBB87_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB87_6
// %bb.2:                               //   in Loop: Header=BB87_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #8]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.lo	.LBB87_4
// %bb.3:                               //   in Loop: Header=BB87_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #8]
	str	w8, [sp, #20]
	ldr	x8, [sp, #8]
	str	x8, [sp, #24]
.LBB87_4:                               //   in Loop: Header=BB87_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB87_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB87_1
.LBB87_6:
	ldr	x8, [sp, #24]
	cbz	x8, .LBB87_8
// %bb.7:
	ldr	x8, [sp, #24]
	str	x8, [sp, #40]
	b	.LBB87_9
.LBB87_8:
	str	xzr, [sp, #40]
.LBB87_9:
	ldr	x0, [sp, #40]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end87:
	.size	get_block_max_count_from_invalid_list, .Lfunc_end87-get_block_max_count_from_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	out_phy_block_from_used_list_by_block // -- Begin function out_phy_block_from_used_list_by_block
	.p2align	2
	.type	out_phy_block_from_used_list_by_block,@function
out_phy_block_from_used_list_by_block:  // @out_phy_block_from_used_list_by_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #16]
	bl	del_block_count_by_block
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end88:
	.size	out_phy_block_from_used_list_by_block, .Lfunc_end88-out_phy_block_from_used_list_by_block
	.cfi_endproc
                                        // -- End function
	.globl	phy_block_from_invalid_page_incr // -- Begin function phy_block_from_invalid_page_incr
	.p2align	2
	.type	phy_block_from_invalid_page_incr,@function
phy_block_from_invalid_page_incr:       // @phy_block_from_invalid_page_incr
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	x1, [sp, #24]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #16]
	cbnz	x8, .LBB89_5
// %bb.1:
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #24]
	cbnz	x8, .LBB89_5
// %bb.2:
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldr	x8, [sp, #24]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.le	.LBB89_4
// %bb.3:
	ldr	x8, [sp, #24]
	ldrh	w1, [x8]
	ldr	x8, [sp, #24]
	ldrh	w2, [x8, #4]
	adrp	x0, .L.str.54
	add	x0, x0, :lo12:.L.str.54
	bl	nand_print_dbg
.LBB89_4:
	stur	wzr, [x29, #-4]
	b	.LBB89_17
.LBB89_5:
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldr	x8, [sp, #24]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.le	.LBB89_7
// %bb.6:
	adrp	x0, .L.str.55
	add	x0, x0, :lo12:.L.str.55
	bl	nand_print_dbg
.LBB89_7:
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #24]
	str	x8, [sp, #16]
	ldr	x8, [sp, #24]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-16]
	add	x9, x9, #336                    // =336
	subs	x8, x8, x9
	b.ne	.LBB89_9
// %bb.8:
	stur	wzr, [x29, #-4]
	b	.LBB89_17
.LBB89_9:
.LBB89_10:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9, #4]
	subs	w8, w8, w9
	b.ge	.LBB89_16
// %bb.11:                              //   in Loop: Header=BB89_10 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldr	x9, [sp, #8]
	str	x8, [x9, #24]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #24]
	str	x8, [x9, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	ldr	x9, [sp, #16]
	str	x8, [x9, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB89_13
// %bb.12:                              //   in Loop: Header=BB89_10 Depth=1
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #16]
	str	x8, [x9, #24]
.LBB89_13:                              //   in Loop: Header=BB89_10 Depth=1
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	str	x8, [x9, #24]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	cbnz	x8, .LBB89_15
// %bb.14:
	b	.LBB89_16
.LBB89_15:                              //   in Loop: Header=BB89_10 Depth=1
	b	.LBB89_10
.LBB89_16:
	stur	wzr, [x29, #-4]
.LBB89_17:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end89:
	.size	phy_block_from_invalid_page_incr, .Lfunc_end89-phy_block_from_invalid_page_incr
	.cfi_endproc
                                        // -- End function
	.globl	nftl_sector_cache_init          // -- Begin function nftl_sector_cache_init
	.p2align	2
	.type	nftl_sector_cache_init,@function
nftl_sector_cache_init:                 // @nftl_sector_cache_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	add	x8, x8, #2880                   // =2880
	str	x8, [sp, #8]
	ldr	x9, [sp, #8]
	mov	w8, #16
	strh	w8, [x9]
	ldr	x8, [sp, #8]
	strh	wzr, [x8, #2]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #8]
	mov	w0, #8192
	bl	nftl_malloc
	ldr	x8, [sp, #8]
	str	x0, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	cbnz	x8, .LBB90_2
// %bb.1:
	adrp	x0, .L.str.56
	add	x0, x0, :lo12:.L.str.56
	adrp	x1, .L__func__.nftl_sector_cache_init
	add	x1, x1, :lo12:.L__func__.nftl_sector_cache_init
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB90_7
.LBB90_2:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #544]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #552]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	str	x8, [sp, #16]
	stur	wzr, [x29, #-20]
.LBB90_3:                               // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	subs	w8, w8, #16                     // =16
	b.hs	.LBB90_6
// %bb.4:                               //   in Loop: Header=BB90_3 Depth=1
	ldur	w8, [x29, #-20]
	ldr	x9, [sp, #8]
	add	x9, x9, #16                     // =16
	ldur	w10, [x29, #-20]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #5
	strh	w8, [x9]
	ldr	x8, [sp, #8]
	add	x8, x8, #16                     // =16
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9, lsl #5
	mov	w8, #255
	strh	w8, [x9, #2]
	ldr	x8, [sp, #8]
	add	x8, x8, #16                     // =16
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9, lsl #5
	mov	w8, #-1
	str	w8, [x9, #4]
	ldr	x8, [sp, #8]
	add	x8, x8, #16                     // =16
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	add	x8, x8, #16                     // =16
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #24]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	add	x9, x9, #16                     // =16
	ldur	w10, [x29, #-20]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #5
	str	x8, [x9, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #512                    // =512
	str	x8, [sp, #16]
// %bb.5:                               //   in Loop: Header=BB90_3 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB90_3
.LBB90_6:
	stur	wzr, [x29, #-4]
.LBB90_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end90:
	.size	nftl_sector_cache_init, .Lfunc_end90-nftl_sector_cache_init
	.cfi_endproc
                                        // -- End function
	.globl	nftl_sector_cache_exit          // -- Begin function nftl_sector_cache_exit
	.p2align	2
	.type	nftl_sector_cache_exit,@function
nftl_sector_cache_exit:                 // @nftl_sector_cache_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #2880                   // =2880
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x0, [x8, #8]
	bl	nftl_free
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end91:
	.size	nftl_sector_cache_exit, .Lfunc_end91-nftl_sector_cache_exit
	.cfi_endproc
                                        // -- End function
	.globl	search_from_sector_cache_write_list // -- Begin function search_from_sector_cache_write_list
	.p2align	2
	.type	search_from_sector_cache_write_list,@function
search_from_sector_cache_write_list:    // @search_from_sector_cache_write_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	ldr	x8, [sp, #16]
	add	x8, x8, #528                    // =528
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
.LBB92_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB92_6
// %bb.2:                               //   in Loop: Header=BB92_1 Depth=1
	ldr	x8, [sp]
	ldr	w8, [x8, #4]
	ldr	w9, [sp, #12]
	subs	w8, w8, w9
	b.ne	.LBB92_4
// %bb.3:
	ldr	x8, [sp]
	str	x8, [sp, #24]
	b	.LBB92_7
.LBB92_4:                               //   in Loop: Header=BB92_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB92_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB92_1
.LBB92_6:
	str	xzr, [sp, #24]
.LBB92_7:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end92:
	.size	search_from_sector_cache_write_list, .Lfunc_end92-search_from_sector_cache_write_list
	.cfi_endproc
                                        // -- End function
	.globl	get_empty_sector_cahce_node     // -- Begin function get_empty_sector_cahce_node
	.p2align	2
	.type	get_empty_sector_cahce_node,@function
get_empty_sector_cahce_node:            // @get_empty_sector_cahce_node
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	strh	wzr, [sp, #14]
.LBB93_1:                               // =>This Inner Loop Header: Depth=1
	ldrh	w8, [sp, #14]
	subs	w8, w8, #16                     // =16
	b.ge	.LBB93_6
// %bb.2:                               //   in Loop: Header=BB93_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #16                     // =16
	ldrh	w9, [sp, #14]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	ldrh	w8, [x8, #2]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB93_4
// %bb.3:
	ldr	x8, [sp, #16]
	add	x8, x8, #16                     // =16
	ldrh	w9, [sp, #14]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	x8, [sp, #24]
	b	.LBB93_7
.LBB93_4:                               //   in Loop: Header=BB93_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB93_1 Depth=1
	ldrh	w8, [sp, #14]
	add	w8, w8, #1                      // =1
	strh	w8, [sp, #14]
	b	.LBB93_1
.LBB93_6:
	str	xzr, [sp, #24]
.LBB93_7:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end93:
	.size	get_empty_sector_cahce_node, .Lfunc_end93-get_empty_sector_cahce_node
	.cfi_endproc
                                        // -- End function
	.globl	add_to_sector_cache_write_list_tail // -- Begin function add_to_sector_cache_write_list_tail
	.p2align	2
	.type	add_to_sector_cache_write_list_tail,@function
add_to_sector_cache_write_list_tail:    // @add_to_sector_cache_write_list_tail
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	add	x8, x8, #528                    // =528
	str	x8, [sp, #8]
.LBB94_1:                               // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB94_3
// %bb.2:                               //   in Loop: Header=BB94_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB94_1
.LBB94_3:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	str	x8, [x9, #24]
	ldr	x9, [sp, #16]
	mov	w8, #1
	strh	w8, [x9, #2]
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #2]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #2]
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end94:
	.size	add_to_sector_cache_write_list_tail, .Lfunc_end94-add_to_sector_cache_write_list_tail
	.cfi_endproc
                                        // -- End function
	.globl	del_from_sector_cache_write_list // -- Begin function del_from_sector_cache_write_list
	.p2align	2
	.type	del_from_sector_cache_write_list,@function
del_from_sector_cache_write_list:       // @del_from_sector_cache_write_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB95_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #16]
	str	x8, [x9, #24]
.LBB95_2:
	ldr	x8, [sp, #16]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #24]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #2]
	subs	w8, w8, #1                      // =1
	strh	w8, [x9, #2]
	ldr	x0, [sp, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end95:
	.size	del_from_sector_cache_write_list, .Lfunc_end95-del_from_sector_cache_write_list
	.cfi_endproc
                                        // -- End function
	.globl	__nand_sector_read              // -- Begin function __nand_sector_read
	.p2align	2
	.type	__nand_sector_read,@function
__nand_sector_read:                     // @__nand_sector_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	stur	w2, [x29, #-16]
	str	x3, [sp, #24]
	str	wzr, [sp, #20]
	ldur	w8, [x29, #-12]
	str	w8, [sp, #12]
	ldr	x8, [sp, #24]
	str	x8, [sp]
	str	wzr, [sp, #16]
.LBB96_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldur	w9, [x29, #-16]
	subs	w8, w8, w9
	b.hs	.LBB96_4
// %bb.2:                               //   in Loop: Header=BB96_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	w1, [sp, #12]
	ldr	x2, [sp]
	bl	nand_sector_cache_read
	ldr	w8, [sp, #20]
	orr	w8, w8, w0
	str	w8, [sp, #20]
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	ldr	x8, [sp]
	add	x8, x8, #512                    // =512
	str	x8, [sp]
// %bb.3:                               //   in Loop: Header=BB96_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB96_1
.LBB96_4:
	ldr	w0, [sp, #20]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end96:
	.size	__nand_sector_read, .Lfunc_end96-__nand_sector_read
	.cfi_endproc
                                        // -- End function
	.globl	nand_sector_cache_read          // -- Begin function nand_sector_cache_read
	.p2align	2
	.type	nand_sector_cache_read,@function
nand_sector_cache_read:                 // @nand_sector_cache_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	str	x2, [sp, #32]
	mov	w8, #512
	str	w8, [sp, #24]
	ldur	x8, [x29, #-16]
	add	x8, x8, #2880                   // =2880
	str	x8, [sp, #8]
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_sector_cache_write_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB97_2
// %bb.1:
	ldr	x0, [sp, #32]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #8]
	ldr	w2, [sp, #24]
	bl	nand_memcpy
	stur	wzr, [x29, #-4]
	b	.LBB97_5
.LBB97_2:
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x3, [sp, #32]
	mov	w2, #1
	bl	__nand_read
	str	w0, [sp, #28]
	ldr	w8, [sp, #28]
	cbz	w8, .LBB97_4
// %bb.3:
	adrp	x0, .L.str.57
	add	x0, x0, :lo12:.L.str.57
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB97_5
.LBB97_4:
	ldr	w8, [sp, #28]
	stur	w8, [x29, #-4]
.LBB97_5:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end97:
	.size	nand_sector_cache_read, .Lfunc_end97-nand_sector_cache_read
	.cfi_endproc
                                        // -- End function
	.globl	__nand_sector_write             // -- Begin function __nand_sector_write
	.p2align	2
	.type	__nand_sector_write,@function
__nand_sector_write:                    // @__nand_sector_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	stur	w2, [x29, #-16]
	str	x3, [sp, #24]
	str	wzr, [sp, #20]
	ldur	w8, [x29, #-12]
	str	w8, [sp, #12]
	ldr	x8, [sp, #24]
	str	x8, [sp]
	str	wzr, [sp, #16]
.LBB98_1:                               // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldur	w9, [x29, #-16]
	subs	w8, w8, w9
	b.hs	.LBB98_4
// %bb.2:                               //   in Loop: Header=BB98_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	w1, [sp, #12]
	ldr	x2, [sp]
	bl	nand_sector_cache_write
	ldr	w8, [sp, #20]
	orr	w8, w8, w0
	str	w8, [sp, #20]
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	ldr	x8, [sp]
	add	x8, x8, #512                    // =512
	str	x8, [sp]
// %bb.3:                               //   in Loop: Header=BB98_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB98_1
.LBB98_4:
	ldr	w0, [sp, #20]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end98:
	.size	__nand_sector_write, .Lfunc_end98-__nand_sector_write
	.cfi_endproc
                                        // -- End function
	.globl	nand_sector_cache_write         // -- Begin function nand_sector_cache_write
	.p2align	2
	.type	nand_sector_cache_write,@function
nand_sector_cache_write:                // @nand_sector_cache_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	str	x2, [sp, #32]
	str	wzr, [sp, #28]
	strh	wzr, [sp, #26]
	ldur	x8, [x29, #-16]
	add	x8, x8, #2880                   // =2880
	str	x8, [sp, #8]
	mov	w8, #512
	strh	w8, [sp, #26]
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_sector_cache_write_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB99_2
// %bb.1:
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_sector_cache_write_list
	ldr	x8, [sp, #16]
	ldr	x0, [x8, #8]
	ldr	x1, [sp, #32]
	ldrh	w2, [sp, #26]
	bl	nand_memcpy
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	add_to_sector_cache_write_list_tail
	stur	wzr, [x29, #-4]
	b	.LBB99_10
.LBB99_2:
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #2]
	subs	w8, w8, #8                      // =8
	b.lt	.LBB99_6
// %bb.3:
	ldr	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #544]
	bl	del_from_sector_cache_write_list
	str	x0, [sp, #16]
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #16]
	ldr	w1, [x8, #4]
	ldr	x8, [sp, #16]
	ldr	x3, [x8, #8]
	mov	w2, #1
	bl	__nand_write
	str	w0, [sp, #28]
	ldr	w8, [sp, #28]
	cbz	w8, .LBB99_5
// %bb.4:
	adrp	x0, .L.str.58
	add	x0, x0, :lo12:.L.str.58
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB99_10
.LBB99_5:
	b	.LBB99_9
.LBB99_6:
	ldr	x0, [sp, #8]
	bl	get_empty_sector_cahce_node
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB99_8
// %bb.7:
	adrp	x0, .L.str.59
	add	x0, x0, :lo12:.L.str.59
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB99_10
.LBB99_8:
.LBB99_9:
	ldur	w8, [x29, #-20]
	ldr	x9, [sp, #16]
	str	w8, [x9, #4]
	ldr	x8, [sp, #16]
	ldr	x0, [x8, #8]
	ldr	x1, [sp, #32]
	ldrh	w2, [sp, #26]
	bl	nand_memcpy
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	add_to_sector_cache_write_list_tail
	stur	wzr, [x29, #-4]
.LBB99_10:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end99:
	.size	nand_sector_cache_write, .Lfunc_end99-nand_sector_cache_write
	.cfi_endproc
                                        // -- End function
	.globl	__nand_read                     // -- Begin function __nand_read
	.p2align	2
	.type	__nand_read,@function
__nand_read:                            // @__nand_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	w2, [sp, #16]
	str	x3, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #656]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB100_2
// %bb.1:
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	adrp	x0, .L.str.297
	add	x0, x0, :lo12:.L.str.297
	bl	nand_print_dbg
.LBB100_2:
	ldr	w8, [sp, #16]
	mov	w10, w8
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	x8, [x9, #64]
	add	x8, x8, x10
	str	x8, [x9, #64]
	ldur	x1, [x29, #-8]
	ldur	w2, [x29, #-12]
	ldr	w3, [sp, #16]
	ldr	x4, [sp, #8]
	mov	w0, wzr
	bl	nand_op
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end100:
	.size	__nand_read, .Lfunc_end100-__nand_read
	.cfi_endproc
                                        // -- End function
	.globl	__nand_write                    // -- Begin function __nand_write
	.p2align	2
	.type	__nand_write,@function
__nand_write:                           // @__nand_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	w2, [sp, #16]
	str	x3, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #656]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB101_2
// %bb.1:
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	adrp	x0, .L.str.298
	add	x0, x0, :lo12:.L.str.298
	bl	nand_print_dbg
.LBB101_2:
	ldr	w8, [sp, #16]
	mov	w10, w8
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	x8, [x9, #72]
	add	x8, x8, x10
	str	x8, [x9, #72]
	ldur	x1, [x29, #-8]
	ldur	w2, [x29, #-12]
	ldr	w3, [sp, #16]
	ldr	x4, [sp, #8]
	mov	w0, #1
	bl	nand_op
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end101:
	.size	__nand_write, .Lfunc_end101-__nand_write
	.cfi_endproc
                                        // -- End function
	.globl	__nand_flush_sector_write_cache // -- Begin function __nand_flush_sector_write_cache
	.p2align	2
	.type	__nand_flush_sector_write_cache,@function
__nand_flush_sector_write_cache:        // @__nand_flush_sector_write_cache
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	ldur	x8, [x29, #-8]
	add	x8, x8, #2880                   // =2880
	str	x8, [sp, #8]
	stur	wzr, [x29, #-16]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #2]
	cbz	w8, .LBB102_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-20]
	b	.LBB102_3
.LBB102_2:
	stur	wzr, [x29, #-20]
.LBB102_3:
.LBB102_4:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #544]
	cbz	x8, .LBB102_8
// %bb.5:                               //   in Loop: Header=BB102_4 Depth=1
	ldr	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #544]
	bl	del_from_sector_cache_write_list
	str	x0, [sp, #16]
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #16]
	ldr	w1, [x8, #4]
	ldr	x8, [sp, #16]
	ldr	x3, [x8, #8]
	mov	w2, #1
	bl	__nand_write
	stur	w0, [x29, #-16]
	ldur	w8, [x29, #-12]
	subs	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	ldur	w8, [x29, #-12]
	cbnz	w8, .LBB102_7
// %bb.6:
	b	.LBB102_8
.LBB102_7:                              //   in Loop: Header=BB102_4 Depth=1
	b	.LBB102_4
.LBB102_8:
	ldur	w8, [x29, #-16]
	cbz	w8, .LBB102_10
// %bb.9:
	adrp	x0, .L.str.60
	add	x0, x0, :lo12:.L.str.60
	bl	nand_print
.LBB102_10:
	ldur	w8, [x29, #-20]
	cbz	w8, .LBB102_12
// %bb.11:
.LBB102_12:
	ldur	w0, [x29, #-16]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end102:
	.size	__nand_flush_sector_write_cache, .Lfunc_end102-__nand_flush_sector_write_cache
	.cfi_endproc
                                        // -- End function
	.globl	nand_crc16                      // -- Begin function nand_crc16
	.p2align	2
	.type	nand_crc16,@function
nand_crc16:                             // @nand_crc16
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	sturh	w0, [x29, #-2]
	str	x1, [sp, #16]
	str	w2, [sp, #12]
.LBB103_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w9, w8, #1                      // =1
	str	w9, [sp, #12]
	cbz	w8, .LBB103_3
// %bb.2:                               //   in Loop: Header=BB103_1 Depth=1
	ldurh	w0, [x29, #-2]
	ldr	x8, [sp, #16]
	add	x9, x8, #1                      // =1
	str	x9, [sp, #16]
	ldrb	w1, [x8]
	bl	nand_crc16_byte
	sturh	w0, [x29, #-2]
	b	.LBB103_1
.LBB103_3:
	ldurh	w0, [x29, #-2]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end103:
	.size	nand_crc16, .Lfunc_end103-nand_crc16
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function nand_crc16_byte
	.type	nand_crc16_byte,@function
nand_crc16_byte:                        // @nand_crc16_byte
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	strh	w0, [sp, #14]
	strb	w1, [sp, #13]
	ldrh	w9, [sp, #14]
	ldrh	w8, [sp, #14]
	ldrb	w10, [sp, #13]
	eor	w8, w8, w10
	and	w10, w8, #0xff
	adrp	x8, nand_crc16_table
	add	x8, x8, :lo12:nand_crc16_table
	ldrh	w8, [x8, w10, uxtw #1]
	eor	w0, w8, w9, asr #8
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end104:
	.size	nand_crc16_byte, .Lfunc_end104-nand_crc16_byte
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_write_logic_page     // -- Begin function _nftl_nand_write_logic_page
	.p2align	2
	.type	_nftl_nand_write_logic_page,@function
_nftl_nand_write_logic_page:            // @_nftl_nand_write_logic_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	str	x2, [sp]
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #12]
	ldr	x2, [sp]
	bl	nand_write_logic_page_no_gc
	cbz	w0, .LBB105_2
// %bb.1:
	ldr	w1, [sp, #12]
	adrp	x0, .L.str.61
	add	x0, x0, :lo12:.L.str.61
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB105_3
.LBB105_2:
	ldr	x0, [sp, #16]
	bl	garbage_collect
	ldr	x0, [sp, #16]
	bl	all_time_gc
	ldr	x0, [sp, #16]
	bl	do_prio_gc
	stur	wzr, [x29, #-4]
.LBB105_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end105:
	.size	_nftl_nand_write_logic_page, .Lfunc_end105-_nftl_nand_write_logic_page
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_read_logic_page      // -- Begin function _nftl_nand_read_logic_page
	.p2align	2
	.type	_nftl_nand_read_logic_page,@function
_nftl_nand_read_logic_page:             // @_nftl_nand_read_logic_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #128                    // =128
	stp	x29, x30, [sp, #112]            // 16-byte Folded Spill
	add	x29, sp, #112                   // =112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	stur	x2, [x29, #-32]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	bl	get_logic_page_map
	str	x0, [sp, #48]
	ldr	x8, [sp, #48]
	cbnz	x8, .LBB106_2
// %bb.1:
	adrp	x0, .L.str.62
	add	x0, x0, :lo12:.L.str.62
	bl	nand_print_dbg
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB106_7
.LBB106_2:
	ldr	x8, [sp, #48]
	ldrh	w8, [x8, #2]
	mov	w9, #65535
	subs	w8, w8, w9
	b.ne	.LBB106_4
// %bb.3:
	ldur	x0, [x29, #-32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w2, [x8, #26]
	mov	w1, #-1
	bl	nand_memset
	stur	wzr, [x29, #-4]
	b	.LBB106_7
.LBB106_4:
	ldur	x8, [x29, #-16]
	ldr	w10, [x8, #664]
	ldr	x9, [sp, #48]
	ldrh	w8, [x9]
	and	w10, w10, #0x1
	lsl	w10, w10, #12
	and	w8, w8, #0xffffefff
	orr	w8, w8, w10
	strh	w8, [x9]
	ldr	x8, [sp, #48]
	ldrh	w1, [x8, #2]
	ldr	x8, [sp, #48]
	ldrh	w8, [x8]
	and	w2, w8, #0xfff
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x4, [x29, #-32]
	add	x0, sp, #24                     // =24
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	sub	x5, x29, #52                    // =52
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-36]
	ldur	w8, [x29, #-36]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB106_6
// %bb.5:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp, #48]
	ldrh	w1, [x8, #2]
	bl	get_phy_block_addr
	str	x0, [sp, #16]
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #16]
	mov	w2, #1
	bl	add_prio_gc
	ldur	w1, [x29, #-20]
	adrp	x0, .L.str.63
	add	x0, x0, :lo12:.L.str.63
	bl	nand_print
.LBB106_6:
	ldur	x0, [x29, #-16]
	bl	do_prio_gc
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
.LBB106_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #112]            // 16-byte Folded Reload
	add	sp, sp, #128                    // =128
	ret
.Lfunc_end106:
	.size	_nftl_nand_read_logic_page, .Lfunc_end106-_nftl_nand_read_logic_page
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_discard_logic_page        // -- Begin function _nftl_discard_logic_page
	.p2align	2
	.type	_nftl_discard_logic_page,@function
_nftl_discard_logic_page:               // @_nftl_discard_logic_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	str	w1, [sp, #4]
	ldr	x0, [sp, #8]
	ldr	w2, [sp, #4]
	mov	w1, #4369
	movk	w1, #43537, lsl #16
	mov	x3, xzr
	bl	write_data_to_page
	str	w0, [sp]
	ldr	w0, [sp]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end107:
	.size	_nftl_discard_logic_page, .Lfunc_end107-_nftl_discard_logic_page
	.cfi_endproc
                                        // -- End function
	.globl	write_data_to_page              // -- Begin function write_data_to_page
	.p2align	2
	.type	write_data_to_page,@function
write_data_to_page:                     // @write_data_to_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	str	w2, [sp, #8]
	str	x3, [sp]
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	add	x1, x8, #528                    // =528
	ldr	w2, [sp, #12]
	ldr	w3, [sp, #8]
	ldr	x4, [sp]
	bl	write_logic_data_to_current_block
	cbz	w0, .LBB108_2
// %bb.1:
	adrp	x0, .L.str.76
	add	x0, x0, :lo12:.L.str.76
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB108_7
.LBB108_2:
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB108_6
// %bb.3:
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	add	x1, x8, #592                    // =592
	ldr	w2, [sp, #12]
	ldr	w3, [sp, #8]
	ldr	x4, [sp]
	bl	write_logic_data_to_current_block
	cbz	w0, .LBB108_5
// %bb.4:
	adrp	x0, .L.str.76
	add	x0, x0, :lo12:.L.str.76
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB108_7
.LBB108_5:
.LBB108_6:
	stur	wzr, [x29, #-4]
.LBB108_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end108:
	.size	write_data_to_page, .Lfunc_end108-write_data_to_page
	.cfi_endproc
                                        // -- End function
	.globl	do_write_error                  // -- Begin function do_write_error
	.p2align	2
	.type	do_write_error,@function
do_write_error:                         // @do_write_error
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
.LBB109_1:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB109_8 Depth 2
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	str	x0, [sp, #48]
	ldr	x8, [sp, #48]
	cbnz	x8, .LBB109_3
// %bb.2:
	adrp	x0, .L.str.64
	add	x0, x0, :lo12:.L.str.64
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB109_25
.LBB109_3:                              //   in Loop: Header=BB109_1 Depth=1
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #24                     // =24
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-28]
	ldr	x8, [sp, #48]
	strh	wzr, [x8, #4]
	ldr	x9, [sp, #48]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB109_7
// %bb.4:                               //   in Loop: Header=BB109_1 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #24                     // =24
	blr	x8
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	adrp	x0, .L.str.65
	add	x0, x0, :lo12:.L.str.65
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #30]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #680]
	subs	w8, w8, w9, asr #2
	b.le	.LBB109_6
// %bb.5:                               //   in Loop: Header=BB109_1 Depth=1
	b	.LBB109_1
.LBB109_6:
	adrp	x0, .L.str.64
	add	x0, x0, :lo12:.L.str.64
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB109_25
.LBB109_7:                              //   in Loop: Header=BB109_1 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldr	x9, [sp, #48]
	str	w8, [x9, #8]
	stur	wzr, [x29, #-32]
.LBB109_8:                              //   Parent Loop BB109_1 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-32]
	ldur	x9, [x29, #-24]
	ldrh	w9, [x9, #56]
	subs	w8, w8, w9
	b.hs	.LBB109_16
// %bb.9:                               //   in Loop: Header=BB109_8 Depth=2
	ldur	x8, [x29, #-24]
	ldr	x8, [x8, #24]
	ldur	w9, [x29, #-32]
                                        // kill: def $x9 killed $w9
	ldr	w8, [x8, x9, lsl #2]
	stur	w8, [x29, #-36]
	ldur	w8, [x29, #-36]
	adds	w8, w8, #1                      // =1
	b.eq	.LBB109_13
// %bb.10:                              //   in Loop: Header=BB109_8 Depth=2
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3488]
	ldur	x0, [x29, #-16]
	ldur	x9, [x29, #-24]
	ldr	x1, [x9]
	ldr	x2, [sp, #48]
	ldur	x9, [x29, #-16]
	ldr	x3, [x9, #512]
	ldur	w4, [x29, #-32]
	blr	x8
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB109_12
// %bb.11:                              //   in Loop: Header=BB109_1 Depth=1
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	adrp	x0, .L.str.66
	add	x0, x0, :lo12:.L.str.66
	bl	nand_print
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #24                     // =24
	str	x0, [sp]                        // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	blr	x8
	b	.LBB109_1
.LBB109_12:                             //   in Loop: Header=BB109_8 Depth=2
	b	.LBB109_14
.LBB109_13:
	b	.LBB109_16
.LBB109_14:                             //   in Loop: Header=BB109_8 Depth=2
// %bb.15:                              //   in Loop: Header=BB109_8 Depth=2
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	b	.LBB109_8
.LBB109_16:
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w8, [x8, #4]
	ldr	x9, [sp, #48]
	strh	w8, [x9, #4]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #24                     // =24
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-24]
	ldr	x1, [x8]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w8, [x8, #2]
	ldr	x9, [sp, #48]
	strh	w8, [x9, #2]
	ldr	x8, [sp, #48]
	ldur	x9, [x29, #-24]
	str	x8, [x9]
	stur	wzr, [x29, #-32]
.LBB109_17:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-32]
	ldur	x9, [x29, #-24]
	ldrh	w9, [x9, #56]
	subs	w8, w8, w9
	b.hs	.LBB109_24
// %bb.18:                              //   in Loop: Header=BB109_17 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8, #24]
	ldur	w9, [x29, #-32]
                                        // kill: def $x9 killed $w9
	ldr	w8, [x8, x9, lsl #2]
	stur	w8, [x29, #-36]
	ldur	w8, [x29, #-36]
	adds	w8, w8, #1                      // =1
	b.eq	.LBB109_22
// %bb.19:                              //   in Loop: Header=BB109_17 Depth=1
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-36]
	bl	get_logic_page_map
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB109_21
// %bb.20:                              //   in Loop: Header=BB109_17 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w8, [x8]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #2]
	ldur	w10, [x29, #-32]
	ldr	x9, [sp, #16]
	ldrh	w8, [x9]
	and	w10, w10, #0xfff
	and	w8, w8, #0xfffff000
	orr	w8, w8, w10
	strh	w8, [x9]
.LBB109_21:                             //   in Loop: Header=BB109_17 Depth=1
.LBB109_22:                             //   in Loop: Header=BB109_17 Depth=1
// %bb.23:                              //   in Loop: Header=BB109_17 Depth=1
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	b	.LBB109_17
.LBB109_24:
	stur	wzr, [x29, #-4]
.LBB109_25:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end109:
	.size	do_write_error, .Lfunc_end109-do_write_error
	.cfi_endproc
                                        // -- End function
	.globl	nand_write_data_page            // -- Begin function nand_write_data_page
	.p2align	2
	.type	nand_write_data_page,@function
nand_write_data_page:                   // @nand_write_data_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	str	w2, [sp, #8]
	str	x3, [sp]
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	add	x1, x8, #528                    // =528
	bl	current_block_at_last_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB110_4
// %bb.1:
	ldr	x0, [sp, #16]
	bl	do_after_write
	cbz	w0, .LBB110_3
// %bb.2:
	adrp	x0, .L.str.67
	add	x0, x0, :lo12:.L.str.67
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB110_11
.LBB110_3:
.LBB110_4:
	ldr	x0, [sp, #16]
	bl	do_before_write
	cbz	w0, .LBB110_6
// %bb.5:
	adrp	x0, .L.str.68
	add	x0, x0, :lo12:.L.str.68
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB110_11
.LBB110_6:
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #12]
	ldr	w2, [sp, #8]
	ldr	x3, [sp]
	bl	write_data_to_page
	cbz	w0, .LBB110_8
// %bb.7:
	adrp	x0, .L.str.69
	add	x0, x0, :lo12:.L.str.69
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB110_11
.LBB110_8:
	ldr	x0, [sp, #16]
	bl	do_after_write
	cbz	w0, .LBB110_10
// %bb.9:
	adrp	x0, .L.str.70
	add	x0, x0, :lo12:.L.str.70
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB110_11
.LBB110_10:
	stur	wzr, [x29, #-4]
.LBB110_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end110:
	.size	nand_write_data_page, .Lfunc_end110-nand_write_data_page
	.cfi_endproc
                                        // -- End function
	.globl	current_block_at_last_page      // -- Begin function current_block_at_last_page
	.p2align	2
	.type	current_block_at_last_page,@function
current_block_at_last_page:             // @current_block_at_last_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #56]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB111_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB111_3
.LBB111_2:
	str	wzr, [sp, #28]
.LBB111_3:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end111:
	.size	current_block_at_last_page, .Lfunc_end111-current_block_at_last_page
	.cfi_endproc
                                        // -- End function
	.globl	do_after_write                  // -- Begin function do_after_write
	.p2align	2
	.type	do_after_write,@function
do_after_write:                         // @do_after_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	current_block_at_last_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB112_14
// %bb.1:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	write_phy_page_map_to_current_block
	cbz	w0, .LBB112_3
// %bb.2:
	adrp	x0, .L.str.73
	add	x0, x0, :lo12:.L.str.73
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB112_15
.LBB112_3:
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB112_7
// %bb.4:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	write_phy_page_map_to_current_block
	cbz	w0, .LBB112_6
// %bb.5:
	adrp	x0, .L.str.73
	add	x0, x0, :lo12:.L.str.73
	bl	nand_print
.LBB112_6:
.LBB112_7:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	get_new_current_block
	cbz	w0, .LBB112_9
// %bb.8:
	adrp	x0, .L.str.74
	add	x0, x0, :lo12:.L.str.74
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB112_15
.LBB112_9:
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB112_13
// %bb.10:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	get_new_current_block
	cbz	w0, .LBB112_12
// %bb.11:
	adrp	x0, .L.str.75
	add	x0, x0, :lo12:.L.str.75
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB112_15
.LBB112_12:
.LBB112_13:
	ldr	x9, [sp]
	ldr	w8, [x9, #668]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #668]
.LBB112_14:
	stur	wzr, [x29, #-4]
.LBB112_15:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end112:
	.size	do_after_write, .Lfunc_end112-do_after_write
	.cfi_endproc
                                        // -- End function
	.globl	do_before_write                 // -- Begin function do_before_write
	.p2align	2
	.type	do_before_write,@function
do_before_write:                        // @do_before_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
.LBB113_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	current_block_at_first_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB113_10
// %bb.2:                               //   in Loop: Header=BB113_1 Depth=1
	ldr	x0, [sp]
	ldr	x8, [sp]
	ldr	x1, [x8, #528]
	ldr	x8, [sp]
	ldr	x8, [x8, #528]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB113_8
// %bb.3:                               //   in Loop: Header=BB113_1 Depth=1
	ldr	x8, [sp]
	ldr	x9, [x8, #528]
	ldr	w8, [x9, #8]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #8]
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB113_5
// %bb.4:                               //   in Loop: Header=BB113_1 Depth=1
	ldr	x8, [sp]
	ldr	x9, [x8, #528]
	ldr	w8, [x9, #8]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #8]
.LBB113_5:                              //   in Loop: Header=BB113_1 Depth=1
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	get_new_current_block
	cbz	w0, .LBB113_7
// %bb.6:
	adrp	x0, .L.str.71
	add	x0, x0, :lo12:.L.str.71
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB113_23
.LBB113_7:                              //   in Loop: Header=BB113_1 Depth=1
	b	.LBB113_9
.LBB113_8:
	b	.LBB113_10
.LBB113_9:                              //   in Loop: Header=BB113_1 Depth=1
	b	.LBB113_1
.LBB113_10:
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB113_22
// %bb.11:
.LBB113_12:                             // =>This Inner Loop Header: Depth=1
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	current_block_at_first_page
	cbz	w0, .LBB113_21
// %bb.13:                              //   in Loop: Header=BB113_12 Depth=1
	ldr	x0, [sp]
	ldr	x8, [sp]
	ldr	x1, [x8, #592]
	ldr	x8, [sp]
	ldr	x8, [x8, #592]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB113_19
// %bb.14:                              //   in Loop: Header=BB113_12 Depth=1
	ldr	x8, [sp]
	ldr	x9, [x8, #592]
	ldr	w8, [x9, #8]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #8]
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB113_16
// %bb.15:                              //   in Loop: Header=BB113_12 Depth=1
	ldr	x8, [sp]
	ldr	x9, [x8, #592]
	ldr	w8, [x9, #8]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #8]
.LBB113_16:                             //   in Loop: Header=BB113_12 Depth=1
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	get_new_current_block
	cbz	w0, .LBB113_18
// %bb.17:
	adrp	x0, .L.str.72
	add	x0, x0, :lo12:.L.str.72
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB113_23
.LBB113_18:                             //   in Loop: Header=BB113_12 Depth=1
	b	.LBB113_20
.LBB113_19:
	b	.LBB113_21
.LBB113_20:                             //   in Loop: Header=BB113_12 Depth=1
	b	.LBB113_12
.LBB113_21:
.LBB113_22:
	stur	wzr, [x29, #-4]
.LBB113_23:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end113:
	.size	do_before_write, .Lfunc_end113-do_before_write
	.cfi_endproc
                                        // -- End function
	.globl	current_block_at_first_page     // -- Begin function current_block_at_first_page
	.p2align	2
	.type	current_block_at_first_page,@function
current_block_at_first_page:            // @current_block_at_first_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #56]
	cbnz	w8, .LBB114_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB114_3
.LBB114_2:
	str	wzr, [sp, #28]
.LBB114_3:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end114:
	.size	current_block_at_first_page, .Lfunc_end114-current_block_at_first_page
	.cfi_endproc
                                        // -- End function
	.globl	new_block_init_for_write        // -- Begin function new_block_init_for_write
	.p2align	2
	.type	new_block_init_for_write,@function
new_block_init_for_write:               // @new_block_init_for_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	w2, [x29, #-28]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #8                      // =8
	str	x0, [sp]                        // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	str	w0, [sp, #32]
	ldur	x8, [x29, #-24]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-24]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #676]
	subs	w8, w8, w9
	b.ls	.LBB115_3
// %bb.1:
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #16]
	subs	w8, w8, w9
	b.ge	.LBB115_3
// %bb.2:
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #676]
.LBB115_3:
	ldur	w8, [x29, #-28]
	ldur	x9, [x29, #-24]
	str	w8, [x9, #8]
	ldr	w8, [sp, #32]
	cbz	w8, .LBB115_5
// %bb.4:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #8                      // =8
	blr	x8
	adrp	x0, .L.str.89
	add	x0, x0, :lo12:.L.str.89
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB115_6
.LBB115_5:
	stur	wzr, [x29, #-4]
.LBB115_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end115:
	.size	new_block_init_for_write, .Lfunc_end115-new_block_init_for_write
	.cfi_endproc
                                        // -- End function
	.globl	get_new_current_block           // -- Begin function get_new_current_block
	.p2align	2
	.type	get_new_current_block,@function
get_new_current_block:                  // @get_new_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	ldr	w8, [x8, #8]
	str	w8, [sp, #4]
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #24]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #32]
	mov	w1, #-1
	bl	nand_memset
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #1062]
	cbz	w8, .LBB116_4
// %bb.1:
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #16]
	bl	out_phy_block_from_free_list_by_erase_time
	ldr	x8, [sp, #8]
	str	x0, [x8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	cbz	x8, .LBB116_3
// %bb.2:
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	ldrh	w1, [x8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.87
	add	x0, x0, :lo12:.L.str.87
	bl	nand_print_dbg
.LBB116_3:
	b	.LBB116_5
.LBB116_4:
	ldr	x0, [sp, #16]
	bl	out_phy_block_from_free_list
	ldr	x8, [sp, #8]
	str	x0, [x8]
.LBB116_5:
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	cbnz	x8, .LBB116_7
// %bb.6:
	adrp	x0, .L.str.88
	add	x0, x0, :lo12:.L.str.88
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB116_10
.LBB116_7:
	ldr	x8, [sp, #8]
	strh	wzr, [x8, #56]
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	ldr	x9, [sp, #8]
	ldr	x9, [x9]
	str	w8, [x9, #8]
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB116_9
// %bb.8:
	ldr	x8, [sp, #8]
	ldr	x9, [x8]
	ldr	w8, [x9, #8]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #8]
.LBB116_9:
	stur	wzr, [x29, #-4]
.LBB116_10:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end116:
	.size	get_new_current_block, .Lfunc_end116-get_new_current_block
	.cfi_endproc
                                        // -- End function
	.globl	write_phy_page_map_to_current_block // -- Begin function write_phy_page_map_to_current_block
	.p2align	2
	.type	write_phy_page_map_to_current_block,@function
write_phy_page_map_to_current_block:    // @write_phy_page_map_to_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	add	x9, x9, #592                    // =592
	subs	x8, x8, x9
	b.ne	.LBB117_2
// %bb.1:
	ldr	x8, [sp, #16]
	ldr	x0, [x8, #632]
	ldr	x8, [sp, #16]
	ldr	x1, [x8, #568]
	mov	w2, #320
	bl	nand_memcpy
.LBB117_2:
	ldr	x0, [sp, #16]
	ldr	x1, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x2, [x8, #8]
	mov	w3, wzr
	mov	w4, #-1431633921
	bl	write_phy_data_to_current_block
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	cbz	w8, .LBB117_4
// %bb.3:
	adrp	x0, .L.str.81
	add	x0, x0, :lo12:.L.str.81
	bl	nand_print
	ldr	w8, [sp, #4]
	stur	w8, [x29, #-4]
	b	.LBB117_7
.LBB117_4:
	ldr	x9, [sp, #8]
	ldrh	w8, [x9, #56]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #56]
	ldr	x8, [sp, #8]
	ldr	x9, [x8]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #8]
	add	x1, x8, #8                      // =8
	ldr	x8, [sp, #8]
	ldr	x2, [x8]
	ldr	x8, [sp, #8]
	ldrh	w3, [x8, #56]
	bl	check_invalid_page_right
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #56]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ne	.LBB117_6
// %bb.5:
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x1, [x8]
	bl	put_phy_block_to_invalid_page_list
	ldr	x0, [sp, #16]
	bl	adjust_invaild_list
.LBB117_6:
	stur	wzr, [x29, #-4]
.LBB117_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end117:
	.size	write_phy_page_map_to_current_block, .Lfunc_end117-write_phy_page_map_to_current_block
	.cfi_endproc
                                        // -- End function
	.globl	write_logic_data_to_current_block // -- Begin function write_logic_data_to_current_block
	.p2align	2
	.type	write_logic_data_to_current_block,@function
write_logic_data_to_current_block:      // @write_logic_data_to_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	w2, [x29, #-28]
	str	w3, [sp, #32]
	str	x4, [sp, #24]
	str	wzr, [sp, #16]
	ldur	w8, [x29, #-28]
	subs	w8, w8, #192                    // =192
	b.ne	.LBB118_13
// %bb.1:
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #32]
	bl	get_logic_page_map
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB118_3
// %bb.2:
	adrp	x0, .L.str.82
	add	x0, x0, :lo12:.L.str.82
	bl	nand_print_dbg
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB118_34
.LBB118_3:
	ldr	x8, [sp]
	ldrh	w8, [x8, #2]
	mov	w9, #65535
	subs	w8, w8, w9
	b.eq	.LBB118_10
// %bb.4:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp]
	ldrh	w1, [x8, #2]
	bl	get_phy_block_addr
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-24]
	ldr	x9, [x9]
	subs	x8, x8, x9
	b.ne	.LBB118_8
// %bb.5:
	ldr	x9, [sp, #8]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.le	.LBB118_7
// %bb.6:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8, #56]
	adrp	x0, .L.str.83
	add	x0, x0, :lo12:.L.str.83
	bl	nand_print_dbg
.LBB118_7:
	b	.LBB118_9
.LBB118_8:
	mov	w8, #1
	str	w8, [sp, #16]
.LBB118_9:
.LBB118_10:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldr	x2, [sp, #24]
	ldr	w3, [sp, #32]
	mov	w4, #192
	bl	write_phy_data_to_current_block
	str	w0, [sp, #20]
	ldr	w8, [sp, #20]
	cbz	w8, .LBB118_12
// %bb.11:
	adrp	x0, .L.str.84
	add	x0, x0, :lo12:.L.str.84
	bl	nand_print
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-4]
	b	.LBB118_34
.LBB118_12:
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w8, [x8]
	ldr	x9, [sp]
	strh	w8, [x9, #2]
	ldur	x8, [x29, #-24]
	ldrh	w10, [x8, #56]
	ldr	x9, [sp]
	ldrh	w8, [x9]
	mov	w11, #4095
	and	w10, w10, #0xfff
	and	w8, w8, #0xfffff000
	orr	w8, w8, w10
	strh	w8, [x9]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #664]
	subs	w8, w8, #0                      // =0
	cset	w10, eq
	ldr	x9, [sp]
	ldrh	w8, [x9]
	and	w10, w10, #0x1
	lsl	w10, w10, #12
	and	w8, w8, #0xffffefff
	orr	w8, w8, w10
	strh	w8, [x9]
	ldr	w8, [sp, #32]
	ldur	x9, [x29, #-24]
	ldr	x9, [x9, #24]
	ldr	x10, [sp]
	ldrh	w10, [x10]
	and	w10, w10, w11
	lsl	w11, w10, #2
                                        // implicit-def: $x10
	mov	w10, w11
	and	x10, x10, #0xffff
	str	w8, [x9, x10]
	ldur	x9, [x29, #-24]
	ldrh	w8, [x9, #56]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #56]
	b	.LBB118_31
.LBB118_13:
	ldur	w8, [x29, #-28]
	mov	w9, #4369
	movk	w9, #43537, lsl #16
	subs	w8, w8, w9
	b.ne	.LBB118_24
// %bb.14:
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #32]
	bl	get_logic_page_map
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB118_16
// %bb.15:
	adrp	x0, .L.str.82
	add	x0, x0, :lo12:.L.str.82
	bl	nand_print_dbg
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB118_34
.LBB118_16:
	ldr	x8, [sp]
	ldrh	w8, [x8, #2]
	mov	w9, #65535
	subs	w8, w8, w9
	b.eq	.LBB118_23
// %bb.17:
	ldur	x0, [x29, #-16]
	ldr	x8, [sp]
	ldrh	w1, [x8, #2]
	bl	get_phy_block_addr
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-24]
	ldr	x9, [x9]
	subs	x8, x8, x9
	b.ne	.LBB118_21
// %bb.18:
	ldur	x8, [x29, #-24]
	ldr	x9, [x8, #24]
	ldr	x8, [sp]
	ldrh	w8, [x8]
	and	w8, w8, #0xfff
	lsl	w10, w8, #2
                                        // implicit-def: $x8
	mov	w8, w10
	and	x10, x8, #0xffff
	mov	w8, #-1
	str	w8, [x9, x10]
	ldr	x9, [sp, #8]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.le	.LBB118_20
// %bb.19:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8, #56]
	adrp	x0, .L.str.85
	add	x0, x0, :lo12:.L.str.85
	bl	nand_print
.LBB118_20:
	b	.LBB118_22
.LBB118_21:
	mov	w8, #1
	str	w8, [sp, #16]
.LBB118_22:
.LBB118_23:
	ldr	x9, [sp]
	mov	w8, #65535
	strh	w8, [x9, #2]
	ldr	x9, [sp]
	ldrh	w8, [x9]
	and	w8, w8, #0xfffff000
	orr	w8, w8, #0xfff
	strh	w8, [x9]
	ldr	x9, [sp]
	ldrh	w8, [x9]
	and	w8, w8, #0xffffefff
	orr	w8, w8, #0x1000
	strh	w8, [x9]
	b	.LBB118_30
.LBB118_24:
	ldur	w8, [x29, #-28]
	mov	w9, #13107
	movk	w9, #43537, lsl #16
	subs	w8, w8, w9
	b.ne	.LBB118_28
// %bb.25:
	ldur	x8, [x29, #-24]
	ldr	x9, [x8]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldr	x2, [sp, #24]
	ldr	w3, [sp, #32]
	mov	w4, #13107
	movk	w4, #43537, lsl #16
	bl	write_phy_data_to_current_block
	str	w0, [sp, #20]
	ldr	w8, [sp, #20]
	cbz	w8, .LBB118_27
// %bb.26:
	adrp	x0, .L.str.86
	add	x0, x0, :lo12:.L.str.86
	bl	nand_print
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-4]
	b	.LBB118_34
.LBB118_27:
	ldur	x9, [x29, #-24]
	ldrh	w8, [x9, #56]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #56]
	b	.LBB118_29
.LBB118_28:
.LBB118_29:
.LBB118_30:
.LBB118_31:
	ldr	w8, [sp, #16]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB118_33
// %bb.32:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #8]
	bl	phy_block_from_invalid_page_incr
.LBB118_33:
	ldur	x0, [x29, #-16]
	bl	adjust_invaild_list
	stur	wzr, [x29, #-4]
.LBB118_34:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end118:
	.size	write_logic_data_to_current_block, .Lfunc_end118-write_logic_data_to_current_block
	.cfi_endproc
                                        // -- End function
	.globl	write_phy_data_to_current_block // -- Begin function write_phy_data_to_current_block
	.p2align	2
	.type	write_phy_data_to_current_block,@function
write_phy_data_to_current_block:        // @write_phy_data_to_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	stur	w3, [x29, #-36]
	stur	w4, [x29, #-40]
	strh	wzr, [sp, #6]
	strh	wzr, [sp, #4]
	add	x0, sp, #32                     // =32
	mov	w1, #-91
	mov	w2, #16
	bl	nand_memset
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	lsl	w8, w8, #9
	strh	w8, [sp, #4]
	ldur	w8, [x29, #-40]
	mov	w9, #-1431633921
	subs	w8, w8, w9
	b.ne	.LBB119_2
// %bb.1:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	strh	w8, [sp, #4]
.LBB119_2:
	ldur	x1, [x29, #-32]
	ldrh	w2, [sp, #4]
	mov	w0, #-1
	bl	nand_crc16
	strh	w0, [sp, #6]
	ldrh	w8, [sp, #6]
	and	w8, w8, #0xff00
	asr	w8, w8, #8
	strb	w8, [sp, #44]
	ldrh	w8, [sp, #6]
	and	w8, w8, #0xff
	strb	w8, [sp, #45]
	str	wzr, [sp, #48]
.LBB119_3:                              // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-24]
	ldrh	w2, [x8, #56]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x4, [x29, #-32]
	add	x0, sp, #8                      // =8
	add	x5, sp, #32                     // =32
	bl	set_physic_op_par
	ldur	w8, [x29, #-40]
	mov	w9, #-1431633921
	subs	w8, w8, w9
	b.ne	.LBB119_7
// %bb.4:                               //   in Loop: Header=BB119_3 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #56]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.eq	.LBB119_6
// %bb.5:                               //   in Loop: Header=BB119_3 Depth=1
.LBB119_6:                              //   in Loop: Header=BB119_3 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w4, [x8, #6]
	add	x1, sp, #32                     // =32
	mov	w2, #-1431633921
	bl	set_oob_special_page
	b	.LBB119_18
.LBB119_7:                              //   in Loop: Header=BB119_3 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #192                    // =192
	b.ne	.LBB119_11
// %bb.8:                               //   in Loop: Header=BB119_3 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #56]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB119_10
// %bb.9:                               //   in Loop: Header=BB119_3 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w1, [x8]
	adrp	x0, .L.str.77
	add	x0, x0, :lo12:.L.str.77
	bl	nand_print_dbg
.LBB119_10:                             //   in Loop: Header=BB119_3 Depth=1
	ldur	x0, [x29, #-16]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w4, [x8, #6]
	add	x1, sp, #32                     // =32
	bl	set_oob_logic_page
	b	.LBB119_17
.LBB119_11:                             //   in Loop: Header=BB119_3 Depth=1
	ldur	w8, [x29, #-40]
	mov	w9, #13107
	movk	w9, #43537, lsl #16
	subs	w8, w8, w9
	b.ne	.LBB119_15
// %bb.12:                              //   in Loop: Header=BB119_3 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #56]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB119_14
// %bb.13:                              //   in Loop: Header=BB119_3 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w1, [x8]
	adrp	x0, .L.str.78
	add	x0, x0, :lo12:.L.str.78
	bl	nand_print_dbg
.LBB119_14:                             //   in Loop: Header=BB119_3 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8]
	ldrh	w4, [x8, #6]
	add	x1, sp, #32                     // =32
	mov	w2, #13107
	movk	w2, #43537, lsl #16
	bl	set_oob_special_page
	b	.LBB119_16
.LBB119_15:                             //   in Loop: Header=BB119_3 Depth=1
.LBB119_16:                             //   in Loop: Header=BB119_3 Depth=1
.LBB119_17:                             //   in Loop: Header=BB119_3 Depth=1
.LBB119_18:                             //   in Loop: Header=BB119_3 Depth=1
	ldur	x8, [x29, #-24]
	ldr	x9, [x8]
	mov	w8, #1
	strh	w8, [x9, #2]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	add	x1, sp, #8                      // =8
	blr	x8
	stur	w0, [x29, #-44]
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB119_23
// %bb.19:                              //   in Loop: Header=BB119_3 Depth=1
	ldr	w8, [sp, #48]
	subs	w8, w8, #2                      // =2
	b.ge	.LBB119_23
// %bb.20:                              //   in Loop: Header=BB119_3 Depth=1
	ldrh	w1, [sp, #10]
	adrp	x0, .L.str.79
	add	x0, x0, :lo12:.L.str.79
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	bl	do_write_error
	cbnz	w0, .LBB119_22
// %bb.21:                              //   in Loop: Header=BB119_3 Depth=1
	ldr	w8, [sp, #48]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #48]
	b	.LBB119_3
.LBB119_22:
	adrp	x0, .L.str.80
	add	x0, x0, :lo12:.L.str.80
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB119_24
.LBB119_23:
	stur	wzr, [x29, #-4]
.LBB119_24:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end119:
	.size	write_phy_data_to_current_block, .Lfunc_end119-write_phy_data_to_current_block
	.cfi_endproc
                                        // -- End function
	.globl	nftl_ops_init                   // -- Begin function nftl_ops_init
	.p2align	2
	.type	nftl_ops_init,@function
nftl_ops_init:                          // @nftl_ops_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_erase_superblk
	add	x8, x8, :lo12:_nftl_nand_erase_superblk
	str	x8, [x9, #3464]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_read_page
	add	x8, x8, :lo12:_nftl_nand_read_page
	str	x8, [x9, #3472]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_write_page
	add	x8, x8, :lo12:_nftl_nand_write_page
	str	x8, [x9, #3480]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_is_blk_good
	add	x8, x8, :lo12:_nftl_nand_is_blk_good
	str	x8, [x9, #3496]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_mark_bad_blk
	add	x8, x8, :lo12:_nftl_nand_mark_bad_blk
	str	x8, [x9, #3504]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_copy_page
	add	x8, x8, :lo12:_nftl_nand_copy_page
	str	x8, [x9, #3488]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_read_logic_page
	add	x8, x8, :lo12:_nftl_nand_read_logic_page
	str	x8, [x9, #3520]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_nand_write_logic_page
	add	x8, x8, :lo12:_nftl_nand_write_logic_page
	str	x8, [x9, #3512]
	ldr	x9, [sp, #8]
	adrp	x8, _nftl_discard_logic_page
	add	x8, x8, :lo12:_nftl_discard_logic_page
	str	x8, [x9, #3528]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end120:
	.size	nftl_ops_init, .Lfunc_end120-nftl_ops_init
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_erase_superblk       // -- Begin function _nftl_nand_erase_superblk
	.p2align	2
	.type	_nftl_nand_erase_superblk,@function
_nftl_nand_erase_superblk:              // @_nftl_nand_erase_superblk
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nftl_get_zone_priv
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #2]
	add	x1, sp, #24                     // =24
	strh	w8, [sp, #26]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8]
	strh	w8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #4]
	strh	w8, [sp, #28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #40]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #88]
	ldr	x9, [sp, #16]
	ldr	x0, [x9, #8]
	blr	x8
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end121:
	.size	_nftl_nand_erase_superblk, .Lfunc_end121-_nftl_nand_erase_superblk
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_read_page            // -- Begin function _nftl_nand_read_page
	.p2align	2
	.type	_nftl_nand_read_page,@function
_nftl_nand_read_page:                   // @_nftl_nand_read_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nftl_get_zone_priv
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	str	x8, [sp]
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	x8, [x9, #80]
	add	x8, x8, #1                      // =1
	str	x8, [x9, #80]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #2]
	add	x1, sp, #16                     // =16
	strh	w8, [sp, #18]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8]
	strh	w8, [sp, #16]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #4]
	strh	w8, [sp, #20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #32]
	ldr	x8, [sp]
	ldr	x8, [x8, #96]
	ldr	x9, [sp, #8]
	ldr	x0, [x9, #8]
	blr	x8
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB122_2
// %bb.1:
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #132]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #132]
.LBB122_2:
	ldur	w0, [x29, #-20]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end122:
	.size	_nftl_nand_read_page, .Lfunc_end122-_nftl_nand_read_page
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_write_page           // -- Begin function _nftl_nand_write_page
	.p2align	2
	.type	_nftl_nand_write_page,@function
_nftl_nand_write_page:                  // @_nftl_nand_write_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nftl_get_zone_priv
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	str	x8, [sp]
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	x8, [x9, #88]
	add	x8, x8, #1                      // =1
	str	x8, [x9, #88]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #2]
	add	x1, sp, #16                     // =16
	strh	w8, [sp, #18]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8]
	strh	w8, [sp, #16]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #4]
	strh	w8, [sp, #20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #32]
	ldr	x8, [sp]
	ldr	x8, [x8, #104]
	ldr	x9, [sp, #8]
	ldr	x0, [x9, #8]
	blr	x8
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	cbz	w8, .LBB123_2
// %bb.1:
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #136]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #136]
.LBB123_2:
	ldur	w0, [x29, #-20]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end123:
	.size	_nftl_nand_write_page, .Lfunc_end123-_nftl_nand_write_page
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_is_blk_good          // -- Begin function _nftl_nand_is_blk_good
	.p2align	2
	.type	_nftl_nand_is_blk_good,@function
_nftl_nand_is_blk_good:                 // @_nftl_nand_is_blk_good
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nftl_get_zone_priv
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #2]
	add	x1, sp, #24                     // =24
	strh	w8, [sp, #26]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8]
	strh	w8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #4]
	strh	w8, [sp, #28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #40]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #112]
	ldr	x9, [sp, #16]
	ldr	x0, [x9, #8]
	blr	x8
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end124:
	.size	_nftl_nand_is_blk_good, .Lfunc_end124-_nftl_nand_is_blk_good
	.cfi_endproc
                                        // -- End function
	.globl	_nftl_nand_mark_bad_blk         // -- Begin function _nftl_nand_mark_bad_blk
	.p2align	2
	.type	_nftl_nand_mark_bad_blk,@function
_nftl_nand_mark_bad_blk:                // @_nftl_nand_mark_bad_blk
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nftl_get_zone_priv
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #2]
	add	x1, sp, #24                     // =24
	strh	w8, [sp, #26]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8]
	strh	w8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #4]
	strh	w8, [sp, #28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #40]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #120]
	ldr	x9, [sp, #16]
	ldr	x0, [x9, #8]
	blr	x8
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end125:
	.size	_nftl_nand_mark_bad_blk, .Lfunc_end125-_nftl_nand_mark_bad_blk
	.cfi_endproc
                                        // -- End function
	.globl	fill_no_use_data_to_page        // -- Begin function fill_no_use_data_to_page
	.p2align	2
	.type	fill_no_use_data_to_page,@function
fill_no_use_data_to_page:               // @fill_no_use_data_to_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	current_block_at_last_page
	cbz	w0, .LBB126_14
// %bb.1:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	write_phy_page_map_to_current_block
	cbz	w0, .LBB126_3
// %bb.2:
	adrp	x0, .L.str.73
	add	x0, x0, :lo12:.L.str.73
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB126_23
.LBB126_3:
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB126_7
// %bb.4:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	write_phy_page_map_to_current_block
	cbz	w0, .LBB126_6
// %bb.5:
	adrp	x0, .L.str.73
	add	x0, x0, :lo12:.L.str.73
	bl	nand_print
.LBB126_6:
.LBB126_7:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	get_new_current_block
	cbz	w0, .LBB126_9
// %bb.8:
	adrp	x0, .L.str.74
	add	x0, x0, :lo12:.L.str.74
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB126_23
.LBB126_9:
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB126_13
// %bb.10:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	get_new_current_block
	cbz	w0, .LBB126_12
// %bb.11:
	adrp	x0, .L.str.75
	add	x0, x0, :lo12:.L.str.75
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB126_23
.LBB126_12:
.LBB126_13:
	stur	wzr, [x29, #-4]
	b	.LBB126_23
.LBB126_14:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	current_block_at_first_page
	cbz	w0, .LBB126_16
// %bb.15:
	stur	wzr, [x29, #-4]
	b	.LBB126_23
.LBB126_16:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #528                    // =528
	bl	write_phy_page_map_to_current_block
	cbz	w0, .LBB126_18
// %bb.17:
	adrp	x0, .L.str.73
	add	x0, x0, :lo12:.L.str.73
	bl	nand_print
.LBB126_18:
	ldr	x8, [sp]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB126_22
// %bb.19:
	ldr	x0, [sp]
	ldr	x8, [sp]
	add	x1, x8, #592                    // =592
	bl	write_phy_page_map_to_current_block
	cbz	w0, .LBB126_21
// %bb.20:
	adrp	x0, .L.str.73
	add	x0, x0, :lo12:.L.str.73
	bl	nand_print
.LBB126_21:
.LBB126_22:
	mov	w8, #8
	stur	w8, [x29, #-4]
.LBB126_23:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end126:
	.size	fill_no_use_data_to_page, .Lfunc_end126-fill_no_use_data_to_page
	.cfi_endproc
                                        // -- End function
	.globl	fill_no_use_page_to_current_block // -- Begin function fill_no_use_page_to_current_block
	.p2align	2
	.type	fill_no_use_page_to_current_block,@function
fill_no_use_page_to_current_block:      // @fill_no_use_page_to_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	str	wzr, [sp, #8]
.LBB127_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #8]
	ldr	w9, [sp, #12]
	subs	w8, w8, w9
	b.ge	.LBB127_6
// %bb.2:                               //   in Loop: Header=BB127_1 Depth=1
	ldr	x0, [sp, #16]
	bl	fill_no_use_data_to_page
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	cbnz	w8, .LBB127_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB127_7
.LBB127_4:                              //   in Loop: Header=BB127_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB127_1 Depth=1
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
	b	.LBB127_1
.LBB127_6:
	mov	w8, #1
	stur	w8, [x29, #-4]
.LBB127_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end127:
	.size	fill_no_use_page_to_current_block, .Lfunc_end127-fill_no_use_page_to_current_block
	.cfi_endproc
                                        // -- End function
	.globl	nand_before_shutdown            // -- Begin function nand_before_shutdown
	.p2align	2
	.type	nand_before_shutdown,@function
nand_before_shutdown:                   // @nand_before_shutdown
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	mov	w1, #8
	bl	fill_no_use_page_to_current_block
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end128:
	.size	nand_before_shutdown, .Lfunc_end128-nand_before_shutdown
	.cfi_endproc
                                        // -- End function
	.globl	dump_buf                        // -- Begin function dump_buf
	.p2align	2
	.type	dump_buf,@function
dump_buf:                               // @dump_buf
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	str	w1, [sp, #4]
	str	wzr, [sp]
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print
	str	wzr, [sp]
.LBB129_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp]
	ldr	w9, [sp, #4]
	subs	w8, w8, w9
	b.hs	.LBB129_8
// %bb.2:                               //   in Loop: Header=BB129_1 Depth=1
	ldr	w8, [sp]
	mov	w10, #16
	sdiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	cbnz	w8, .LBB129_4
// %bb.3:                               //   in Loop: Header=BB129_1 Depth=1
	ldr	w1, [sp]
	adrp	x0, .L.str.92
	add	x0, x0, :lo12:.L.str.92
	bl	nand_print
.LBB129_4:                              //   in Loop: Header=BB129_1 Depth=1
	ldr	x8, [sp, #8]
	ldrsw	x9, [sp]
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.93
	add	x0, x0, :lo12:.L.str.93
	bl	nand_print
	ldr	w8, [sp]
	add	w8, w8, #1                      // =1
	mov	w10, #16
	sdiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	cbnz	w8, .LBB129_6
// %bb.5:                               //   in Loop: Header=BB129_1 Depth=1
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print
.LBB129_6:                              //   in Loop: Header=BB129_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB129_1 Depth=1
	ldr	w8, [sp]
	add	w8, w8, #1                      // =1
	str	w8, [sp]
	b	.LBB129_1
.LBB129_8:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end129:
	.size	dump_buf, .Lfunc_end129-dump_buf
	.cfi_endproc
                                        // -- End function
	.globl	zone_param_init                 // -- Begin function zone_param_init
	.p2align	2
	.type	zone_param_init,@function
zone_param_init:                        // @zone_param_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	sturh	w1, [x29, #-18]
	str	w2, [sp, #24]
	str	w3, [sp, #20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w0, [x8, #26]
	bl	nftl_malloc
	ldur	x8, [x29, #-16]
	str	x0, [x8, #512]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #512]
	cbnz	x8, .LBB130_2
// %bb.1:
	adrp	x0, .L.str.94
	add	x0, x0, :lo12:.L.str.94
	adrp	x1, .L__func__.zone_param_init
	add	x1, x1, :lo12:.L__func__.zone_param_init
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_2:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w0, [x8, #26]
	bl	nftl_malloc
	ldur	x8, [x29, #-16]
	str	x0, [x8, #520]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #520]
	cbnz	x8, .LBB130_4
// %bb.3:
	adrp	x0, .L.str.95
	add	x0, x0, :lo12:.L.str.95
	adrp	x1, .L__func__.zone_param_init
	add	x1, x1, :lo12:.L__func__.zone_param_init
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_4:
	ldr	w8, [sp, #24]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #32]
	ldr	w8, [sp, #20]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #40]
	ldurh	w8, [x29, #-18]
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #44]
	ldur	x0, [x29, #-16]
	ldurh	w1, [x29, #-18]
	bl	get_phy_block_addr
	ldur	x8, [x29, #-16]
	str	x0, [x8, #48]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #26]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #28]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #30]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #56]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #12]
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #12]
	bl	malloc_logic_page_map
	cbz	w0, .LBB130_6
// %bb.5:
	adrp	x0, .L.str.96
	add	x0, x0, :lo12:.L.str.96
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	lsl	w8, w8, #2
	ldur	x9, [x29, #-16]
	str	w8, [x9, #560]
	ldur	x9, [x29, #-16]
	mov	w8, #320
	str	w8, [x9, #576]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #576]
	add	w8, w8, w9
	ldur	x9, [x29, #-16]
	str	w8, [x9, #544]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #544]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9, lsl #9
	b.hs	.LBB130_8
// %bb.7:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	lsl	w8, w8, #9
	ldur	x9, [x29, #-16]
	str	w8, [x9, #544]
	b	.LBB130_9
.LBB130_8:
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #544]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #28]
	lsl	w3, w8, #9
	adrp	x0, .L.str.97
	add	x0, x0, :lo12:.L.str.97
	adrp	x1, .L__func__.zone_param_init
	add	x1, x1, :lo12:.L__func__.zone_param_init
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_9:
	ldur	x8, [x29, #-16]
	ldr	w0, [x8, #544]
	bl	nftl_malloc
	ldur	x8, [x29, #-16]
	str	x0, [x8, #536]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #536]
	cbnz	x8, .LBB130_11
// %bb.10:
	adrp	x0, .L.str.98
	add	x0, x0, :lo12:.L.str.98
	adrp	x1, .L__func__.zone_param_init
	add	x1, x1, :lo12:.L__func__.zone_param_init
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_11:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #536]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #552]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #536]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #560]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldur	x9, [x29, #-16]
	str	x8, [x9, #568]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #568]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #3440]
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #536]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #544]
	mov	w1, #-1
	bl	nand_memset
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #624]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #576]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #576]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #544]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #608]
	ldur	x8, [x29, #-16]
	ldr	w0, [x8, #608]
	bl	nftl_malloc
	ldur	x8, [x29, #-16]
	str	x0, [x8, #600]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #600]
	cbnz	x8, .LBB130_13
// %bb.12:
	adrp	x0, .L.str.99
	add	x0, x0, :lo12:.L.str.99
	adrp	x1, .L__func__.zone_param_init
	add	x1, x1, :lo12:.L__func__.zone_param_init
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_13:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #600]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #616]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #600]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #624]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldur	x9, [x29, #-16]
	str	x8, [x9, #632]
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #600]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #608]
	mov	w1, #-1
	bl	nand_memset
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #488]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #576]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #504]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #544]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #472]
	ldur	x8, [x29, #-16]
	ldr	w0, [x8, #472]
	bl	nftl_malloc
	ldur	x8, [x29, #-16]
	str	x0, [x8, #464]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #464]
	cbnz	x8, .LBB130_15
// %bb.14:
	adrp	x0, .L.str.100
	add	x0, x0, :lo12:.L.str.100
	adrp	x1, .L__func__.zone_param_init
	add	x1, x1, :lo12:.L__func__.zone_param_init
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB130_20
.LBB130_15:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #464]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #480]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #464]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #488]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldur	x9, [x29, #-16]
	str	x8, [x9, #496]
	ldur	x8, [x29, #-16]
	str	wzr, [x8, #656]
	ldur	x8, [x29, #-16]
	str	wzr, [x8, #676]
	ldur	x8, [x29, #-16]
	str	wzr, [x8, #660]
	ldur	x8, [x29, #-16]
	str	wzr, [x8, #664]
	ldur	x8, [x29, #-16]
	str	wzr, [x8, #668]
	ldur	x8, [x29, #-16]
	str	wzr, [x8, #672]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	asr	w8, w8, #1
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #1056]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #16]
	mov	w9, #6
	sdiv	w8, w8, w9
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #1058]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #1060]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #1062]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #1064]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #528]
	ldur	x9, [x29, #-16]
	mov	w8, #65535
	strh	w8, [x9, #584]
	ldur	x9, [x29, #-16]
	str	xzr, [x9, #592]
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #648]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #304]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #312]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #288]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #296]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #320]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #328]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #368]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #376]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #352]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #360]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #384]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #392]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #432]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #440]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #416]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #424]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #448]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #456]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #696]
	str	wzr, [sp, #16]
.LBB130_16:                             // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.hs	.LBB130_19
// %bb.17:                              //   in Loop: Header=BB130_16 Depth=1
	ldr	w8, [sp, #16]
	ldur	x9, [x29, #-16]
	add	x9, x9, #704                    // =704
	ldr	w10, [sp, #16]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #5
	strh	w8, [x9]
	ldur	x8, [x29, #-16]
	add	x8, x8, #704                    // =704
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-16]
	add	x8, x8, #704                    // =704
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #8]
	ldur	x8, [x29, #-16]
	add	x8, x8, #704                    // =704
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #16]
	ldur	x8, [x29, #-16]
	add	x8, x8, #704                    // =704
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #24]
// %bb.18:                              //   in Loop: Header=BB130_16 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB130_16
.LBB130_19:
	ldur	x9, [x29, #-16]
	mov	w8, #255
	strh	w8, [x9, #1024]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #698]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #1026]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #1032]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #1040]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #1048]
	stur	wzr, [x29, #-4]
.LBB130_20:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end130:
	.size	zone_param_init, .Lfunc_end130-zone_param_init
	.cfi_endproc
                                        // -- End function
	.globl	zone_param_exit                 // -- Begin function zone_param_exit
	.p2align	2
	.type	zone_param_exit,@function
zone_param_exit:                        // @zone_param_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	free_logic_page_map
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #464]
	bl	nftl_free
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #536]
	bl	nftl_free
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #600]
	bl	nftl_free
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #512]
	bl	nftl_free
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #520]
	bl	nftl_free
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end131:
	.size	zone_param_exit, .Lfunc_end131-zone_param_exit
	.cfi_endproc
                                        // -- End function
	.globl	is_no_use_device                // -- Begin function is_no_use_device
	.p2align	2
	.type	is_no_use_device,@function
is_no_use_device:                       // @is_no_use_device
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #48]
	str	x8, [sp, #16]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #32]
	cbz	w8, .LBB132_2
// %bb.1:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #40]
	add	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	stur	w8, [x29, #-24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w9, [x8, #20]
	ldur	w8, [x29, #-24]
	udiv	w8, w8, w9
	stur	w8, [x29, #-24]
	b	.LBB132_3
.LBB132_2:
	mov	w8, #-1
	stur	w8, [x29, #-24]
.LBB132_3:
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #26]
.LBB132_4:                              // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #26]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	b.hs	.LBB132_18
// %bb.5:                               //   in Loop: Header=BB132_4 Depth=1
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #56]
	ldr	x8, [sp, #16]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #24                     // =24
	str	x0, [sp]                        // 8-byte Folded Spill
	mov	w2, wzr
	sub	x5, x29, #44                    // =44
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3496]
	ldur	x0, [x29, #-16]
	blr	x8
	subs	w8, w0, #1                      // =1
	b.ne	.LBB132_13
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	add	x1, sp, #24                     // =24
	blr	x8
	str	w0, [sp, #12]
	sub	x0, x29, #44                    // =44
	bl	is_ftl_start_flag_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB132_12
// %bb.7:
	ldr	w8, [sp, #12]
	cbz	w8, .LBB132_9
// %bb.8:
	ldr	w8, [sp, #12]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB132_12
.LBB132_9:
	sub	x0, x29, #44                    // =44
	mov	w1, #3
	mov	w2, #4
	bl	get_spare_data
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	ldur	w9, [x29, #-20]
	subs	w8, w8, w9
	b.eq	.LBB132_11
// %bb.10:
	adrp	x0, .L.str.101
	add	x0, x0, :lo12:.L.str.101
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB132_19
.LBB132_11:
	ldr	x8, [sp, #16]
	add	x8, x8, #64                     // =64
	ldur	x9, [x29, #-16]
	str	x8, [x9, #48]
	stur	wzr, [x29, #-4]
	b	.LBB132_19
.LBB132_12:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB132_19
.LBB132_13:                             //   in Loop: Header=BB132_4 Depth=1
	ldr	x9, [sp, #16]
	mov	w8, #255
	strh	w8, [x9, #2]
// %bb.14:                              //   in Loop: Header=BB132_4 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	ldr	x0, [sp, #16]
	ldur	x1, [x29, #-16]
	bl	is_last_phy_block
	subs	w8, w0, #1                      // =1
	b.ne	.LBB132_16
// %bb.15:
	b	.LBB132_18
.LBB132_16:                             //   in Loop: Header=BB132_4 Depth=1
// %bb.17:                              //   in Loop: Header=BB132_4 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #64                     // =64
	str	x8, [sp, #16]
	b	.LBB132_4
.LBB132_18:
	mov	w8, #1
	stur	w8, [x29, #-4]
.LBB132_19:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end132:
	.size	is_no_use_device, .Lfunc_end132-is_no_use_device
	.cfi_endproc
                                        // -- End function
	.globl	is_last_phy_block               // -- Begin function is_last_phy_block
	.p2align	2
	.type	is_last_phy_block,@function
is_last_phy_block:                      // @is_last_phy_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #24]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB133_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB133_3
.LBB133_2:
	str	wzr, [sp, #28]
.LBB133_3:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end133:
	.size	is_last_phy_block, .Lfunc_end133-is_last_phy_block
	.cfi_endproc
                                        // -- End function
	.globl	get_vaild_blocks                // -- Begin function get_vaild_blocks
	.p2align	2
	.type	get_vaild_blocks,@function
get_vaild_blocks:                       // @get_vaild_blocks
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	stur	w2, [x29, #-24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	x8, [sp, #8]
	ldur	w8, [x29, #-24]
	ldur	w9, [x29, #-20]
	subs	w8, w8, w9
	stur	w8, [x29, #-36]
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-28]
	ldur	w8, [x29, #-20]
	stur	w8, [x29, #-32]
.LBB134_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-32]
	ldur	w9, [x29, #-36]
	subs	w8, w8, w9
	b.hs	.LBB134_10
// %bb.2:                               //   in Loop: Header=BB134_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8]
	add	x1, sp, #16                     // =16
	strh	w8, [sp, #18]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3496]
	ldur	x0, [x29, #-16]
	blr	x8
	subs	w8, w0, #159                    // =159
	b.ne	.LBB134_6
// %bb.3:                               //   in Loop: Header=BB134_1 Depth=1
	ldur	w8, [x29, #-28]
	subs	w8, w8, #1                      // =1
	stur	w8, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbnz	w8, .LBB134_5
// %bb.4:
	stur	wzr, [x29, #-4]
	b	.LBB134_11
.LBB134_5:                              //   in Loop: Header=BB134_1 Depth=1
.LBB134_6:                              //   in Loop: Header=BB134_1 Depth=1
	ldr	x0, [sp, #8]
	ldur	x1, [x29, #-16]
	bl	is_last_phy_block
	subs	w8, w0, #1                      // =1
	b.ne	.LBB134_8
// %bb.7:
	b	.LBB134_10
.LBB134_8:                              //   in Loop: Header=BB134_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #64                     // =64
	str	x8, [sp, #8]
// %bb.9:                               //   in Loop: Header=BB134_1 Depth=1
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	b	.LBB134_1
.LBB134_10:
	ldur	w8, [x29, #-28]
	stur	w8, [x29, #-4]
.LBB134_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end134:
	.size	get_vaild_blocks, .Lfunc_end134-get_vaild_blocks
	.cfi_endproc
                                        // -- End function
	.globl	build_zone_list_first           // -- Begin function build_zone_list_first
	.p2align	2
	.type	build_zone_list_first,@function
build_zone_list_first:                  // @build_zone_list_first
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #144                    // =144
	stp	x29, x30, [sp, #128]            // 16-byte Folded Spill
	add	x29, sp, #128                   // =128
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	adrp	x0, .L.str.102
	add	x0, x0, :lo12:.L.str.102
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #48]
	str	x8, [sp, #48]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #32]
	cbz	w8, .LBB135_2
// %bb.1:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #40]
	add	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	stur	w8, [x29, #-28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w9, [x8, #20]
	ldur	w8, [x29, #-28]
	udiv	w8, w8, w9
	stur	w8, [x29, #-28]
	b	.LBB135_3
.LBB135_2:
	mov	w8, #-1
	stur	w8, [x29, #-28]
.LBB135_3:
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #26]
.LBB135_4:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB135_10 Depth 2
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #26]
	ldur	w9, [x29, #-28]
	subs	w8, w8, w9
	b.hs	.LBB135_21
// %bb.5:                               //   in Loop: Header=BB135_4 Depth=1
	ldr	x8, [sp, #48]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #56]
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #40]                   // 8-byte Folded Spill
	mov	w2, wzr
	sub	x5, x29, #48                    // =48
	bl	set_physic_op_par
	ldr	x1, [sp, #40]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3496]
	ldur	x0, [x29, #-16]
	blr	x8
	subs	w8, w0, #1                      // =1
	b.ne	.LBB135_16
// %bb.6:                               //   in Loop: Header=BB135_4 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB135_8
// %bb.7:                               //   in Loop: Header=BB135_4 Depth=1
	ldr	x9, [sp, #48]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	b	.LBB135_15
.LBB135_8:                              //   in Loop: Header=BB135_4 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	ldr	x8, [sp, #48]
	strh	wzr, [x8, #2]
	ldr	x8, [sp, #48]
	strh	wzr, [x8, #6]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #26]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB135_14
// %bb.9:                               //   in Loop: Header=BB135_4 Depth=1
	sub	x0, x29, #48                    // =48
	str	x0, [sp, #32]                   // 8-byte Folded Spill
	mov	w1, #-1
	mov	w2, #16
	bl	nand_memset
	ldr	x1, [sp, #32]                   // 8-byte Folded Reload
	ldur	x0, [x29, #-16]
	ldur	w2, [x29, #-20]
	bl	set_start_block_flag
	ldr	x5, [sp, #32]                   // 8-byte Folded Reload
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	mov	w2, wzr
	bl	set_physic_op_par
	stur	wzr, [x29, #-32]
.LBB135_10:                             //   Parent Loop BB135_4 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-32]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.hs	.LBB135_13
// %bb.11:                              //   in Loop: Header=BB135_10 Depth=2
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	ldrh	w8, [sp, #56]
	add	w8, w8, #1                      // =1
	strh	w8, [sp, #56]
// %bb.12:                              //   in Loop: Header=BB135_10 Depth=2
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	b	.LBB135_10
.LBB135_13:                             //   in Loop: Header=BB135_4 Depth=1
	ldr	x8, [sp, #48]
	add	x8, x8, #64                     // =64
	ldur	x9, [x29, #-16]
	str	x8, [x9, #48]
.LBB135_14:                             //   in Loop: Header=BB135_4 Depth=1
.LBB135_15:                             //   in Loop: Header=BB135_4 Depth=1
	b	.LBB135_17
.LBB135_16:                             //   in Loop: Header=BB135_4 Depth=1
	ldr	x9, [sp, #48]
	mov	w8, #255
	strh	w8, [x9, #2]
.LBB135_17:                             //   in Loop: Header=BB135_4 Depth=1
	ldr	x9, [sp, #48]
	mov	w8, #65535
	strh	w8, [x9, #4]
	ldr	x8, [sp, #48]
	str	xzr, [x8, #32]
	ldr	x8, [sp, #48]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #48]
	str	xzr, [x8, #24]
	ldr	x8, [sp, #48]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #56]
	ldr	x0, [sp, #48]
	ldur	x1, [x29, #-16]
	bl	is_last_phy_block
	subs	w8, w0, #1                      // =1
	b.ne	.LBB135_19
// %bb.18:
	b	.LBB135_21
.LBB135_19:                             //   in Loop: Header=BB135_4 Depth=1
// %bb.20:                              //   in Loop: Header=BB135_4 Depth=1
	ldr	x8, [sp, #48]
	add	x8, x8, #64                     // =64
	str	x8, [sp, #48]
	b	.LBB135_4
.LBB135_21:
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #26]
	subs	w8, w8, #1                      // =1
	b.le	.LBB135_23
// %bb.22:
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	subs	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	b	.LBB135_24
.LBB135_23:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8, #26]
	adrp	x0, .L.str.103
	add	x0, x0, :lo12:.L.str.103
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB135_39
.LBB135_24:
	ldur	w8, [x29, #-28]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB135_26
// %bb.25:
	stur	wzr, [x29, #-4]
	b	.LBB135_39
.LBB135_26:
	ldr	x0, [sp, #48]
	ldur	x1, [x29, #-16]
	bl	is_last_phy_block
	subs	w8, w0, #1                      // =1
	b.ne	.LBB135_28
// %bb.27:
	stur	wzr, [x29, #-4]
	b	.LBB135_39
.LBB135_28:
.LBB135_29:                             // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #48]
	ldrh	w8, [x8]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.ge	.LBB135_38
// %bb.30:                              //   in Loop: Header=BB135_29 Depth=1
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	mov	w2, wzr
	sub	x5, x29, #48                    // =48
	bl	set_physic_op_par
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3496]
	ldur	x0, [x29, #-16]
	blr	x8
	subs	w8, w0, #1                      // =1
	b.ne	.LBB135_35
// %bb.31:                              //   in Loop: Header=BB135_29 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB135_33
// %bb.32:                              //   in Loop: Header=BB135_29 Depth=1
	ldr	x9, [sp, #48]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	b	.LBB135_34
.LBB135_33:
	sub	x0, x29, #48                    // =48
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	w1, #-1
	mov	w2, #16
	bl	nand_memset
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_end_block_flag
	ldr	x5, [sp, #8]                    // 8-byte Folded Reload
	ldr	x8, [sp, #48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	mov	w2, wzr
	bl	set_physic_op_par
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	blr	x8
	b	.LBB135_38
.LBB135_34:                             //   in Loop: Header=BB135_29 Depth=1
	b	.LBB135_36
.LBB135_35:                             //   in Loop: Header=BB135_29 Depth=1
	ldr	x9, [sp, #48]
	mov	w8, #255
	strh	w8, [x9, #2]
.LBB135_36:                             //   in Loop: Header=BB135_29 Depth=1
// %bb.37:                              //   in Loop: Header=BB135_29 Depth=1
	ldr	x8, [sp, #48]
	add	x8, x8, #64                     // =64
	str	x8, [sp, #48]
	b	.LBB135_29
.LBB135_38:
	stur	wzr, [x29, #-4]
.LBB135_39:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #128]            // 16-byte Folded Reload
	add	sp, sp, #144                    // =144
	ret
.Lfunc_end135:
	.size	build_zone_list_first, .Lfunc_end135-build_zone_list_first
	.cfi_endproc
                                        // -- End function
	.globl	build_zone_list                 // -- Begin function build_zone_list
	.p2align	2
	.type	build_zone_list,@function
build_zone_list:                        // @build_zone_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	adrp	x0, .L.str.104
	add	x0, x0, :lo12:.L.str.104
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	first_scan_all_blocks
	cbz	w0, .LBB136_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB136_7
.LBB136_2:
	ldr	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldrh	w1, [x8, #26]
	bl	init_zone_after_first_scan
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbz	w8, .LBB136_4
// %bb.3:
	adrp	x0, .L.str.105
	add	x0, x0, :lo12:.L.str.105
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB136_7
.LBB136_4:
	ldr	x0, [sp, #16]
	bl	second_scan_all_blocks
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbz	w8, .LBB136_6
// %bb.5:
	adrp	x0, .L.str.106
	add	x0, x0, :lo12:.L.str.106
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB136_7
.LBB136_6:
	ldr	x0, [sp, #16]
	bl	power_on_gc
	str	w0, [sp, #12]
	ldr	x0, [sp, #16]
	bl	print_nftl_zone
	adrp	x0, .L.str.107
	add	x0, x0, :lo12:.L.str.107
	bl	nand_print
	ldr	x0, [sp, #16]
	bl	panic_backup_block
	stur	wzr, [x29, #-4]
.LBB136_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end136:
	.size	build_zone_list, .Lfunc_end136-build_zone_list
	.cfi_endproc
                                        // -- End function
	.globl	first_scan_all_blocks           // -- Begin function first_scan_all_blocks
	.p2align	2
	.type	first_scan_all_blocks,@function
first_scan_all_blocks:                  // @first_scan_all_blocks
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #256                    // =256
	stp	x29, x30, [sp, #240]            // 16-byte Folded Spill
	add	x29, sp, #240                   // =240
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #48]
	stur	x8, [x29, #-80]
// %bb.1:
	mov	x0, xzr
	stur	xzr, [x29, #-88]
	adrp	x8, .L__const.first_scan_all_blocks.alises
	add	x8, x8, :lo12:.L__const.first_scan_all_blocks.alises
	ldr	w9, [x8]
	stur	w9, [x29, #-96]
	ldrb	w8, [x8, #4]
	sturb	w8, [x29, #-92]
	adrp	x8, .L__const.first_scan_all_blocks.of_root
	add	x8, x8, :lo12:.L__const.first_scan_all_blocks.of_root
	ldrh	w8, [x8]
	sub	x1, x29, #98                    // =98
	sturh	w8, [x29, #-98]
	adrp	x2, mod
	add	x2, x2, :lo12:mod
	bl	nand_fdt_get_prop_string_by_path
	stur	x0, [x29, #-88]
	ldur	x8, [x29, #-88]
	cbnz	x8, .LBB137_3
// %bb.2:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB137_70
.LBB137_3:
	ldur	x0, [x29, #-88]
	sub	x1, x29, #96                    // =96
	mov	x2, #3
	bl	nand_memcmp
	cbz	w0, .LBB137_5
// %bb.4:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB137_70
.LBB137_5:
// %bb.6:
	adrp	x0, .L.str.108
	add	x0, x0, :lo12:.L.str.108
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #26]
.LBB137_7:                              // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.ge	.LBB137_69
// %bb.8:                               //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	sub	x0, x29, #72                    // =72
	stur	x0, [x29, #-112]                // 8-byte Folded Spill
	mov	w3, wzr
	mov	w2, w3
	mov	x4, xzr
	sub	x5, x29, #44                    // =44
	bl	set_physic_op_par
	ldur	x1, [x29, #-112]                // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3496]
	ldur	x0, [x29, #-16]
	blr	x8
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_66
// %bb.9:                               //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	sub	x1, x29, #72                    // =72
	blr	x8
	stur	w0, [x29, #-24]
	ldur	x8, [x29, #-80]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #56]
	sub	x0, x29, #44                    // =44
	str	x0, [sp, #120]                  // 8-byte Folded Spill
	bl	get_block_used_count_from_oob
	mov	w8, w0
	ldr	x0, [sp, #120]                  // 8-byte Folded Reload
	stur	w8, [x29, #-20]
	bl	is_ftl_end_flag_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_11
// %bb.10:
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.109
	add	x0, x0, :lo12:.L.str.109
	bl	nand_print
	b	.LBB137_69
.LBB137_11:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_panic_data_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_15
// %bb.12:                              //   in Loop: Header=BB137_7 Depth=1
	ldurb	w8, [x29, #-41]
	ldur	x9, [x29, #-16]
	ldrb	w9, [x9, #24]
	subs	w8, w8, w9
	b.ne	.LBB137_14
// %bb.13:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x1, [x29, #-80]
	sub	x0, x29, #44                    // =44
	bl	panic_recover_info
.LBB137_14:                             //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_64
.LBB137_15:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_17
// %bb.16:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #6]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	bl	put_phy_block_to_free_list
	b	.LBB137_63
.LBB137_17:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_ftl_logic_page_data
	subs	w8, w0, #1                      // =1
	b.eq	.LBB137_19
// %bb.18:                              //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_function_info_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_26
.LBB137_19:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB137_21
// %bb.20:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	w8, [x29, #-24]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB137_26
.LBB137_21:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	ldur	x9, [x29, #-80]
	mov	w8, #1
	strh	w8, [x9, #2]
	sub	x0, x29, #44                    // =44
	bl	get_erase_count_from_oob
	ldur	x8, [x29, #-80]
	strh	w0, [x8, #6]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-28]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #4]
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-80]
	str	w8, [x9, #8]
	ldur	x8, [x29, #-80]
	str	xzr, [x8, #48]
	ldur	x8, [x29, #-80]
	str	xzr, [x8, #56]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	bl	add_block_count_list
	ldur	w8, [x29, #-24]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB137_25
// %bb.22:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #4]
	cbz	w8, .LBB137_24
// %bb.23:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	mov	w2, #1
	bl	add_prio_gc
	adrp	x0, .L.str.110
	add	x0, x0, :lo12:.L.str.110
	bl	nand_print_dbg
.LBB137_24:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_25:                             //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_62
.LBB137_26:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_ftl_start_flag_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_28
// %bb.27:                              //   in Loop: Header=BB137_7 Depth=1
	adrp	x0, .L.str.111
	add	x0, x0, :lo12:.L.str.111
	bl	nand_print
	b	.LBB137_61
.LBB137_28:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_fill_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_30
// %bb.29:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #2]
	sub	x0, x29, #44                    // =44
	bl	get_erase_count_from_oob
	ldur	x8, [x29, #-80]
	strh	w0, [x8, #6]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	ldur	x9, [x29, #-80]
	strh	w8, [x9, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	bl	put_phy_block_to_free_list
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.112
	add	x0, x0, :lo12:.L.str.112
	bl	nand_print_dbg
	b	.LBB137_60
.LBB137_30:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldurb	w2, [x29, #-44]
	ldurb	w3, [x29, #-43]
	ldurb	w4, [x29, #-42]
	ldurb	w5, [x29, #-41]
	ldurb	w6, [x29, #-40]
	ldurb	w7, [x29, #-39]
	ldurb	w18, [x29, #-38]
	ldurb	w17, [x29, #-37]
	ldurb	w16, [x29, #-36]
	ldurb	w15, [x29, #-35]
	ldurb	w14, [x29, #-34]
	ldurb	w13, [x29, #-33]
	ldurb	w12, [x29, #-32]
	ldurb	w11, [x29, #-31]
	ldurb	w10, [x29, #-30]
	ldurb	w8, [x29, #-29]
	adrp	x0, .L.str.113
	add	x0, x0, :lo12:.L.str.113
	mov	x9, sp
	str	x9, [sp, #112]                  // 8-byte Folded Spill
	str	w18, [x9]
	str	w17, [x9, #8]
	str	w16, [x9, #16]
	str	w15, [x9, #24]
	str	w14, [x9, #32]
	str	w13, [x9, #40]
	str	w12, [x9, #48]
	str	w11, [x9, #56]
	str	w10, [x9, #64]
	str	w8, [x9, #72]
	bl	nand_print
	ldur	w8, [x29, #-28]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-80]
	strh	w8, [x9, #6]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB137_32
// %bb.31:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	w8, [x29, #-24]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB137_38
.LBB137_32:                             //   in Loop: Header=BB137_7 Depth=1
	ldurb	w8, [x29, #-44]
	subs	w8, w8, #255                    // =255
	b.eq	.LBB137_34
// %bb.33:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.114
	add	x0, x0, :lo12:.L.str.114
	bl	nand_print
	ldur	x9, [x29, #-80]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
	b	.LBB137_37
.LBB137_34:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	cbnz	w0, .LBB137_36
// %bb.35:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
.LBB137_36:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_37:                             //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_59
.LBB137_38:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.115
	add	x0, x0, :lo12:.L.str.115
	bl	nand_print
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w2, w8, #1                      // =1
	sub	x0, x29, #72                    // =72
	str	x0, [sp, #104]                  // 8-byte Folded Spill
	mov	w3, wzr
	mov	x4, xzr
	sub	x5, x29, #44                    // =44
	bl	set_physic_op_par
	ldr	x1, [sp, #104]                  // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB137_40
// %bb.39:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	w8, [x29, #-24]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB137_55
.LBB137_40:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_phy_mapping_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_46
// %bb.41:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.116
	add	x0, x0, :lo12:.L.str.116
	bl	nand_print
	sub	x0, x29, #44                    // =44
	str	x0, [sp, #96]                   // 8-byte Folded Spill
	bl	get_block_used_count_from_oob
	mov	w8, w0
	ldr	x0, [sp, #96]                   // 8-byte Folded Reload
	stur	w8, [x29, #-20]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
	ldur	x9, [x29, #-80]
	mov	w8, #1
	strh	w8, [x9, #2]
	bl	get_erase_count_from_oob
	ldur	x8, [x29, #-80]
	strh	w0, [x8, #6]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-28]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #4]
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-80]
	str	w8, [x9, #8]
	ldur	x8, [x29, #-80]
	str	xzr, [x8, #48]
	ldur	x8, [x29, #-80]
	str	xzr, [x8, #56]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	bl	add_block_count_list
	ldur	w8, [x29, #-24]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB137_45
// %bb.42:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #4]
	cbz	w8, .LBB137_44
// %bb.43:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	mov	w2, #1
	bl	add_prio_gc
	adrp	x0, .L.str.117
	add	x0, x0, :lo12:.L.str.117
	bl	nand_print
.LBB137_44:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_45:                             //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_54
.LBB137_46:                             //   in Loop: Header=BB137_7 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB137_50
// %bb.47:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.118
	add	x0, x0, :lo12:.L.str.118
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	cbnz	w0, .LBB137_49
// %bb.48:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
.LBB137_49:                             //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_53
.LBB137_50:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.119
	add	x0, x0, :lo12:.L.str.119
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	cbnz	w0, .LBB137_52
// %bb.51:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
.LBB137_52:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_53:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_54:                             //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_58
.LBB137_55:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.120
	add	x0, x0, :lo12:.L.str.120
	bl	nand_print
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldurb	w2, [x29, #-44]
	ldurb	w3, [x29, #-43]
	ldurb	w4, [x29, #-42]
	ldurb	w5, [x29, #-41]
	ldurb	w6, [x29, #-40]
	ldurb	w7, [x29, #-39]
	ldurb	w18, [x29, #-38]
	ldurb	w17, [x29, #-37]
	ldurb	w16, [x29, #-36]
	ldurb	w15, [x29, #-35]
	ldurb	w14, [x29, #-34]
	ldurb	w13, [x29, #-33]
	ldurb	w12, [x29, #-32]
	ldurb	w11, [x29, #-31]
	ldurb	w10, [x29, #-30]
	ldurb	w8, [x29, #-29]
	adrp	x0, .L.str.121
	add	x0, x0, :lo12:.L.str.121
	mov	x9, sp
	str	x9, [sp, #88]                   // 8-byte Folded Spill
	str	w18, [x9]
	str	w17, [x9, #8]
	str	w16, [x9, #16]
	str	w15, [x9, #24]
	str	w14, [x9, #32]
	str	w13, [x9, #40]
	str	w12, [x9, #48]
	str	w11, [x9, #56]
	str	w10, [x9, #64]
	str	w8, [x9, #72]
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	bl	torture_block_and_mark
	cbnz	w0, .LBB137_57
// %bb.56:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #26]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #26]
.LBB137_57:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_58:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_59:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_60:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_61:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_62:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_63:                             //   in Loop: Header=BB137_7 Depth=1
.LBB137_64:                             //   in Loop: Header=BB137_7 Depth=1
// %bb.65:                              //   in Loop: Header=BB137_7 Depth=1
	b	.LBB137_67
.LBB137_66:                             //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	adrp	x0, .L.str.122
	add	x0, x0, :lo12:.L.str.122
	bl	nand_print
	ldur	x9, [x29, #-80]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x8, [x29, #-80]
	strh	wzr, [x8, #6]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
.LBB137_67:                             //   in Loop: Header=BB137_7 Depth=1
// %bb.68:                              //   in Loop: Header=BB137_7 Depth=1
	ldur	x8, [x29, #-80]
	add	x8, x8, #64                     // =64
	stur	x8, [x29, #-80]
	b	.LBB137_7
.LBB137_69:
	stur	wzr, [x29, #-4]
.LBB137_70:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #240]            // 16-byte Folded Reload
	add	sp, sp, #256                    // =256
	ret
.Lfunc_end137:
	.size	first_scan_all_blocks, .Lfunc_end137-first_scan_all_blocks
	.cfi_endproc
                                        // -- End function
	.globl	init_zone_after_first_scan      // -- Begin function init_zone_after_first_scan
	.p2align	2
	.type	init_zone_after_first_scan,@function
init_zone_after_first_scan:             // @init_zone_after_first_scan
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #32]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #8]
	ldrh	w9, [x8, #20]
	ldr	w8, [sp, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #8]
	ldr	w1, [sp, #12]
	ldr	w2, [sp, #8]
	adrp	x0, .L.str.123
	add	x0, x0, :lo12:.L.str.123
	bl	nand_print_dbg
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	b.hi	.LBB138_2
// %bb.1:
	ldr	w1, [sp, #12]
	ldr	w2, [sp, #8]
	adrp	x0, .L.str.124
	add	x0, x0, :lo12:.L.str.124
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB138_9
.LBB138_2:
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	mul	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	mul	w8, w8, w9
	ldr	x9, [sp, #16]
	str	w8, [x9, #40]
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #12]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB138_4
// %bb.3:
	ldr	w1, [sp, #12]
	ldr	w2, [sp, #8]
	adrp	x0, .L.str.125
	add	x0, x0, :lo12:.L.str.125
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB138_9
.LBB138_4:
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	mov	w9, #5
	udiv	w8, w8, w9
	ldr	x9, [sp, #16]
	strh	w8, [x9, #680]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #680]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #14]
	subs	w8, w8, w9
	b.ge	.LBB138_6
// %bb.5:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #14]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #680]
.LBB138_6:
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #18]
	mul	w8, w8, w9
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #20]
	udiv	w8, w8, w9
	ldr	x9, [sp, #16]
	strh	w8, [x9, #682]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #682]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9, #680]
	add	w9, w9, #2                      // =2
	subs	w8, w8, w9
	b.ge	.LBB138_8
// %bb.7:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #680]
	add	w8, w8, #2                      // =2
	ldr	x9, [sp, #16]
	strh	w8, [x9, #682]
.LBB138_8:
	ldr	x9, [sp, #16]
	mov	w8, #1
	strh	w8, [x9, #684]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #686]
	stur	wzr, [x29, #-4]
.LBB138_9:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end138:
	.size	init_zone_after_first_scan, .Lfunc_end138-init_zone_after_first_scan
	.cfi_endproc
                                        // -- End function
	.globl	second_scan_all_blocks          // -- Begin function second_scan_all_blocks
	.p2align	2
	.type	second_scan_all_blocks,@function
second_scan_all_blocks:                 // @second_scan_all_blocks
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #288                    // =288
	stp	x29, x30, [sp, #256]            // 16-byte Folded Spill
	str	x28, [sp, #272]                 // 8-byte Folded Spill
	add	x29, sp, #256                   // =256
	.cfi_def_cfa w29, 32
	.cfi_offset w28, -16
	.cfi_offset w30, -24
	.cfi_offset w29, -32
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-48]
	stur	xzr, [x29, #-120]
	str	xzr, [sp, #128]
	stur	xzr, [x29, #-104]
	stur	xzr, [x29, #-96]
	stur	xzr, [x29, #-112]
// %bb.1:
	str	wzr, [sp, #124]
	adrp	x8, g_nreg_base
	ldr	x8, [x8, :lo12:g_nreg_base]
	str	x8, [sp, #112]
	mov	w8, #188
	str	w8, [sp, #108]
	mov	w8, #1
	str	w8, [sp, #104]
	mov	w8, #2
	str	w8, [sp, #100]
	mov	w8, #1
	movk	w8, #3, lsl #16
	str	w8, [sp, #96]
	ldr	x8, [sp, #112]
	ldr	w9, [sp, #108]
                                        // kill: def $x9 killed $w9
	ldr	w8, [x8, x9, lsl #2]
	str	w8, [sp, #124]
	ldr	w8, [sp, #124]
	ldr	w9, [sp, #96]
	subs	w8, w8, w9
	b.eq	.LBB139_4
// %bb.2:
	ldr	w8, [sp, #124]
	ldr	w9, [sp, #104]
	subs	w8, w8, w9
	b.eq	.LBB139_4
// %bb.3:
	ldr	w8, [sp, #124]
	ldr	w9, [sp, #100]
	subs	w8, w8, w9
	b.ne	.LBB139_5
.LBB139_4:
	b	.LBB139_6
.LBB139_5:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB139_108
.LBB139_6:
// %bb.7:
	mov	w8, #-1
	stur	w8, [x29, #-36]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #32]
	ldur	x10, [x29, #-16]
	ldr	x10, [x10, #8]
	ldrb	w10, [x10, #8]
	udiv	w9, w9, w10
	ldur	x10, [x29, #-16]
	str	w9, [x10, #36]
	stur	w8, [x29, #-44]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #448]
	stur	x8, [x29, #-80]
.LBB139_8:                              // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-80]
	cbz	x8, .LBB139_11
// %bb.9:                               //   in Loop: Header=BB139_8 Depth=1
	ldur	x8, [x29, #-80]
	ldr	w8, [x8, #8]
	stur	w8, [x29, #-44]
	ldur	x8, [x29, #-80]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #156]
// %bb.10:                              //   in Loop: Header=BB139_8 Depth=1
	ldur	x8, [x29, #-80]
	ldr	x8, [x8, #48]
	stur	x8, [x29, #-80]
	b	.LBB139_8
.LBB139_11:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #448]
	cbz	x8, .LBB139_13
// %bb.12:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #448]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #160]
.LBB139_13:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #448]
	stur	x8, [x29, #-80]
.LBB139_14:                             // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-80]
	cbz	x8, .LBB139_55
// %bb.15:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	mov	w9, #-1
	add	w9, w9, #512                    // =512
	and	w8, w8, w9
	cbz	w8, .LBB139_17
// %bb.16:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	mov	w9, #512
	udiv	w8, w8, w9
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-48]
	b	.LBB139_18
.LBB139_17:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	mov	w9, #512
	udiv	w8, w8, w9
	stur	w8, [x29, #-48]
.LBB139_18:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w2, w8, #1                      // =1
	ldur	w3, [x29, #-48]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	sub	x0, x29, #72                    // =72
	str	x0, [sp, #88]                   // 8-byte Folded Spill
	sub	x5, x29, #32                    // =32
	bl	set_physic_op_par
	ldr	x1, [sp, #88]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-40]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #28]
	cbz	w8, .LBB139_22
// %bb.19:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #512]
	sub	x2, x29, #32                    // =32
	bl	crc_wrong
	cbz	w0, .LBB139_21
// %bb.20:                              //   in Loop: Header=BB139_14 Depth=1
	mov	w8, #-2
	stur	w8, [x29, #-40]
.LBB139_21:                             //   in Loop: Header=BB139_14 Depth=1
.LBB139_22:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldr	w8, [x8, #8]
	stur	w8, [x29, #-36]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #676]
	subs	w8, w8, w9
	b.ls	.LBB139_25
// %bb.23:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	mov	w9, #50000
	subs	w8, w8, w9
	b.ge	.LBB139_25
// %bb.24:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	str	w8, [x9, #676]
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #676]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #148]
.LBB139_25:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3440]
	ldr	w9, [x9, #152]
	subs	w8, w8, w9
	b.hs	.LBB139_27
// %bb.26:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #152]
.LBB139_27:                             //   in Loop: Header=BB139_14 Depth=1
	sub	x0, x29, #32                    // =32
	bl	is_phy_mapping_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB139_31
// %bb.28:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB139_30
// %bb.29:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB139_31
.LBB139_30:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	stur	x8, [x29, #-104]
	ldur	x9, [x29, #-80]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #512]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w3, w8, #1                      // =1
	bl	recover_logic_page_mapping
	ldur	x8, [x29, #-80]
	stur	x8, [x29, #-112]
	b	.LBB139_53
.LBB139_31:                             //   in Loop: Header=BB139_14 Depth=1
	sub	x0, x29, #32                    // =32
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB139_43
// %bb.32:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-120]
	cbnz	x8, .LBB139_34
// %bb.33:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	stur	x8, [x29, #-120]
	b	.LBB139_42
.LBB139_34:                             //   in Loop: Header=BB139_14 Depth=1
	ldr	x8, [sp, #128]
	cbnz	x8, .LBB139_40
// %bb.35:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	str	x8, [sp, #128]
	ldur	x8, [x29, #-120]
	ldrh	w1, [x8]
	ldr	x8, [sp, #128]
	ldrh	w2, [x8]
	adrp	x0, .L.str.126
	add	x0, x0, :lo12:.L.str.126
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbnz	w8, #0, .LBB139_39
// %bb.36:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-120]
	ldr	w8, [x8, #8]
	ldr	x9, [sp, #128]
	ldr	w9, [x9, #8]
	subs	w8, w8, w9
	b.eq	.LBB139_38
// %bb.37:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-120]
	ldrh	w1, [x8]
	ldr	x8, [sp, #128]
	ldrh	w2, [x8]
	ldur	x8, [x29, #-120]
	ldr	w3, [x8, #8]
	ldr	x8, [sp, #128]
	ldr	w4, [x8, #8]
	adrp	x0, .L.str.127
	add	x0, x0, :lo12:.L.str.127
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	bl	recover_phy_page_mapping
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w3, w8, #1                      // =1
	bl	recover_logic_page_mapping
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	mov	w2, #3
	bl	add_prio_gc
	ldr	x8, [sp, #128]
	stur	x8, [x29, #-120]
	str	xzr, [sp, #128]
.LBB139_38:                             //   in Loop: Header=BB139_14 Depth=1
.LBB139_39:                             //   in Loop: Header=BB139_14 Depth=1
	b	.LBB139_41
.LBB139_40:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-120]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-120]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.128
	add	x0, x0, :lo12:.L.str.128
	bl	nand_print
	ldr	x8, [sp, #128]
	ldrh	w1, [x8]
	ldr	x8, [sp, #128]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.129
	add	x0, x0, :lo12:.L.str.129
	bl	nand_print
	ldur	x8, [x29, #-80]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.130
	add	x0, x0, :lo12:.L.str.130
	bl	nand_print
	ldur	x8, [x29, #-120]
	stur	x8, [x29, #-88]
	ldr	x8, [sp, #128]
	stur	x8, [x29, #-120]
	ldur	x8, [x29, #-80]
	str	x8, [sp, #128]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-88]
	mov	w2, #3
	bl	add_prio_gc
.LBB139_41:                             //   in Loop: Header=BB139_14 Depth=1
.LBB139_42:                             //   in Loop: Header=BB139_14 Depth=1
	b	.LBB139_52
.LBB139_43:                             //   in Loop: Header=BB139_14 Depth=1
	ldurb	w1, [x29, #-32]
	ldurb	w2, [x29, #-31]
	ldurb	w3, [x29, #-30]
	ldurb	w4, [x29, #-29]
	ldurb	w5, [x29, #-28]
	ldurb	w6, [x29, #-27]
	ldurb	w7, [x29, #-26]
	ldurb	w17, [x29, #-25]
	ldurb	w16, [x29, #-24]
	ldurb	w15, [x29, #-23]
	ldurb	w14, [x29, #-22]
	ldurb	w13, [x29, #-21]
	ldurb	w12, [x29, #-20]
	ldurb	w11, [x29, #-19]
	ldurb	w10, [x29, #-18]
	ldurb	w8, [x29, #-17]
	adrp	x0, .L.str.131
	add	x0, x0, :lo12:.L.str.131
	mov	x9, sp
	str	x9, [sp, #80]                   // 8-byte Folded Spill
	str	w17, [x9]
	str	w16, [x9, #8]
	str	w15, [x9, #16]
	str	w14, [x9, #24]
	str	w13, [x9, #32]
	str	w12, [x9, #40]
	str	w11, [x9, #48]
	str	w10, [x9, #56]
	str	w8, [x9, #64]
	bl	nand_print
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB139_45
// %bb.44:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB139_46
.LBB139_45:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8, #24]
	ldur	x8, [x29, #-80]
	ldrh	w2, [x8]
	adrp	x0, .L.str.132
	add	x0, x0, :lo12:.L.str.132
	bl	nand_print
	b	.LBB139_47
.LBB139_46:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8, #24]
	ldur	x8, [x29, #-80]
	ldrh	w2, [x8]
	adrp	x0, .L.str.133
	add	x0, x0, :lo12:.L.str.133
	bl	nand_print
.LBB139_47:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB139_50
// %bb.48:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	w8, [x29, #-44]
	ldur	w9, [x29, #-36]
	subs	w8, w8, w9
	b.ne	.LBB139_50
// %bb.49:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-80]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-80]
	stur	x8, [x29, #-96]
	b	.LBB139_51
.LBB139_50:                             //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	stur	x8, [x29, #-104]
	ldur	x9, [x29, #-80]
	mov	w8, #170
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-80]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	bl	recover_block_phy_page_mapping
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w3, w8, #1                      // =1
	bl	recover_logic_page_mapping
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	mov	w2, #3
	bl	add_prio_gc
.LBB139_51:                             //   in Loop: Header=BB139_14 Depth=1
.LBB139_52:                             //   in Loop: Header=BB139_14 Depth=1
.LBB139_53:                             //   in Loop: Header=BB139_14 Depth=1
// %bb.54:                              //   in Loop: Header=BB139_14 Depth=1
	ldur	x8, [x29, #-80]
	ldr	x8, [x8, #48]
	stur	x8, [x29, #-80]
	b	.LBB139_14
.LBB139_55:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-112]
	bl	init_smart_info
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #448]
	stur	x8, [x29, #-88]
.LBB139_56:                             // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-88]
	cbz	x8, .LBB139_63
// %bb.57:                              //   in Loop: Header=BB139_56 Depth=1
	ldur	x8, [x29, #-88]
	ldur	x9, [x29, #-120]
	subs	x8, x8, x9
	b.eq	.LBB139_61
// %bb.58:                              //   in Loop: Header=BB139_56 Depth=1
	ldur	x8, [x29, #-88]
	ldr	x9, [sp, #128]
	subs	x8, x8, x9
	b.eq	.LBB139_61
// %bb.59:                              //   in Loop: Header=BB139_56 Depth=1
	ldur	x8, [x29, #-88]
	ldur	x9, [x29, #-96]
	subs	x8, x8, x9
	b.eq	.LBB139_61
// %bb.60:                              //   in Loop: Header=BB139_56 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-88]
	bl	put_phy_block_to_invalid_page_list
.LBB139_61:                             //   in Loop: Header=BB139_56 Depth=1
// %bb.62:                              //   in Loop: Header=BB139_56 Depth=1
	ldur	x8, [x29, #-88]
	ldr	x8, [x8, #48]
	stur	x8, [x29, #-88]
	b	.LBB139_56
.LBB139_63:
	ldur	x0, [x29, #-16]
	bl	adjust_invaild_list
	ldur	x8, [x29, #-104]
	cbz	x8, .LBB139_68
// %bb.64:
	ldur	x8, [x29, #-120]
	cbz	x8, .LBB139_68
// %bb.65:
	ldur	x8, [x29, #-104]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-120]
	ldr	w9, [x9, #8]
	subs	w8, w8, w9
	b.eq	.LBB139_67
// %bb.66:
	ldur	x8, [x29, #-104]
	ldr	w1, [x8, #8]
	ldur	x8, [x29, #-120]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.134
	add	x0, x0, :lo12:.L.str.134
	bl	nand_print
.LBB139_67:
.LBB139_68:
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #528]
	ldur	x8, [x29, #-120]
	cbnz	x8, .LBB139_79
// %bb.69:
	ldr	x8, [sp, #128]
	cbnz	x8, .LBB139_79
// %bb.70:
	adrp	x0, .L.str.135
	add	x0, x0, :lo12:.L.str.135
	bl	nand_print
	stur	xzr, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB139_74
// %bb.71:
	ldur	x8, [x29, #-104]
	cbz	x8, .LBB139_73
// %bb.72:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-104]
	mov	w2, #3
	bl	add_prio_gc
.LBB139_73:
	b	.LBB139_78
.LBB139_74:
	ldur	x8, [x29, #-104]
	cbz	x8, .LBB139_77
// %bb.75:
	ldur	x8, [x29, #-104]
	ldrh	w8, [x8, #2]
	subs	w8, w8, #170                    // =170
	b.ne	.LBB139_77
// %bb.76:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-104]
	mov	w2, #3
	bl	add_prio_gc
.LBB139_77:
.LBB139_78:
	b	.LBB139_105
.LBB139_79:
	ldr	x8, [sp, #128]
	cbnz	x8, .LBB139_95
// %bb.80:
	ldur	x8, [x29, #-120]
	stur	x8, [x29, #-80]
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB139_87
// %bb.81:
	ldur	x8, [x29, #-120]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-120]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.136
	add	x0, x0, :lo12:.L.str.136
	bl	nand_print_dbg
	ldur	x8, [x29, #-104]
	cbz	x8, .LBB139_84
// %bb.82:
	ldur	x8, [x29, #-104]
	ldrh	w8, [x8, #2]
	subs	w8, w8, #170                    // =170
	b.ne	.LBB139_84
// %bb.83:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-104]
	mov	w2, #3
	bl	add_prio_gc
.LBB139_84:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	mov	x2, xzr
	bl	cross_talk_rebuild_current_block
	stur	x0, [x29, #-80]
	ldur	x8, [x29, #-80]
	cbnz	x8, .LBB139_86
// %bb.85:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB139_108
.LBB139_86:
	b	.LBB139_94
.LBB139_87:
	ldur	x8, [x29, #-104]
	cbz	x8, .LBB139_90
// %bb.88:
	ldur	x8, [x29, #-104]
	ldrh	w8, [x8, #2]
	subs	w8, w8, #170                    // =170
	b.ne	.LBB139_90
// %bb.89:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-104]
	mov	w2, #3
	bl	add_prio_gc
.LBB139_90:
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #24]
	cbnz	w8, .LBB139_92
// %bb.91:
	ldur	x8, [x29, #-120]
	stur	x8, [x29, #-80]
	b	.LBB139_93
.LBB139_92:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	mov	x2, xzr
	bl	no_cross_talk_rebuild_current_block
	stur	x0, [x29, #-80]
.LBB139_93:
.LBB139_94:
	b	.LBB139_104
.LBB139_95:
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB139_99
// %bb.96:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	ldr	x2, [sp, #128]
	bl	cross_talk_rebuild_current_block
	stur	x0, [x29, #-80]
	ldur	x8, [x29, #-80]
	cbnz	x8, .LBB139_98
// %bb.97:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB139_108
.LBB139_98:
	b	.LBB139_103
.LBB139_99:
	ldur	x8, [x29, #-120]
	ldr	w8, [x8, #8]
	ldr	x9, [sp, #128]
	ldr	w9, [x9, #8]
	subs	w8, w8, w9
	b.ne	.LBB139_101
// %bb.100:
	ldur	x8, [x29, #-120]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-120]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.137
	add	x0, x0, :lo12:.L.str.137
	bl	nand_print
	ldr	x8, [sp, #128]
	ldrh	w1, [x8]
	ldr	x8, [sp, #128]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.138
	add	x0, x0, :lo12:.L.str.138
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	ldr	x2, [sp, #128]
	bl	no_cross_talk_rebuild_current_block
	stur	x0, [x29, #-80]
	b	.LBB139_102
.LBB139_101:
	adrp	x0, .L.str.139
	add	x0, x0, :lo12:.L.str.139
	bl	nand_print
	ldur	x8, [x29, #-104]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-104]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.140
	add	x0, x0, :lo12:.L.str.140
	bl	nand_print
	ldur	x8, [x29, #-120]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-120]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.137
	add	x0, x0, :lo12:.L.str.137
	bl	nand_print
	ldr	x8, [sp, #128]
	ldrh	w1, [x8]
	ldr	x8, [sp, #128]
	ldr	w2, [x8, #8]
	adrp	x0, .L.str.138
	add	x0, x0, :lo12:.L.str.138
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	bl	recover_block_phy_page_mapping
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w3, w8, #1                      // =1
	bl	recover_logic_page_mapping
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-120]
	mov	w2, #3
	str	w2, [sp, #76]                   // 4-byte Folded Spill
	bl	add_prio_gc
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #128]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	bl	recover_block_phy_page_mapping
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #128]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w3, w8, #1                      // =1
	bl	recover_logic_page_mapping
	ldr	w2, [sp, #76]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #128]
	bl	add_prio_gc
.LBB139_102:
.LBB139_103:
.LBB139_104:
.LBB139_105:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-80]
	ldur	w2, [x29, #-36]
	bl	get_current_used_block
	stur	w0, [x29, #-40]
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB139_107
// %bb.106:
	adrp	x0, .L.str.141
	add	x0, x0, :lo12:.L.str.141
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB139_108
.LBB139_107:
	ldur	x0, [x29, #-16]
	bl	adjust_invaild_list
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #64]
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8, #30]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w2, [x8, #148]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w3, [x8, #152]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w4, [x8, #156]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w5, [x8, #160]
	adrp	x0, .L.str.142
	add	x0, x0, :lo12:.L.str.142
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
.LBB139_108:
	ldur	w0, [x29, #-4]
	ldr	x28, [sp, #272]                 // 8-byte Folded Reload
	ldp	x29, x30, [sp, #256]            // 16-byte Folded Reload
	add	sp, sp, #288                    // =288
	ret
.Lfunc_end139:
	.size	second_scan_all_blocks, .Lfunc_end139-second_scan_all_blocks
	.cfi_endproc
                                        // -- End function
	.globl	print_nftl_zone                 // -- Begin function print_nftl_zone
	.p2align	2
	.type	print_nftl_zone,@function
print_nftl_zone:                        // @print_nftl_zone
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #208                    // =208
	stp	x29, x30, [sp, #192]            // 16-byte Folded Spill
	add	x29, sp, #192                   // =192
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #656]
	cbnz	w8, .LBB140_2
// %bb.1:
	b	.LBB140_5
.LBB140_2:
	adrp	x0, .L.str.222
	add	x0, x0, :lo12:.L.str.222
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #24]
	adrp	x0, .L.str.223
	add	x0, x0, :lo12:.L.str.223
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #26]
	adrp	x0, .L.str.224
	add	x0, x0, :lo12:.L.str.224
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #20]
	adrp	x0, .L.str.225
	add	x0, x0, :lo12:.L.str.225
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #16]
	adrp	x0, .L.str.226
	add	x0, x0, :lo12:.L.str.226
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #30]
	adrp	x0, .L.str.227
	add	x0, x0, :lo12:.L.str.227
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #656]
	adrp	x0, .L.str.228
	add	x0, x0, :lo12:.L.str.228
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrb	w1, [x8, #24]
	adrp	x0, .L.str.229
	add	x0, x0, :lo12:.L.str.229
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrb	w1, [x8, #25]
	adrp	x0, .L.str.230
	add	x0, x0, :lo12:.L.str.230
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #26]
	adrp	x0, .L.str.231
	add	x0, x0, :lo12:.L.str.231
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #28]
	adrp	x0, .L.str.232
	add	x0, x0, :lo12:.L.str.232
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #32]
	adrp	x0, .L.str.233
	add	x0, x0, :lo12:.L.str.233
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #40]
	adrp	x0, .L.str.234
	add	x0, x0, :lo12:.L.str.234
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #30]
	adrp	x0, .L.str.235
	add	x0, x0, :lo12:.L.str.235
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #680]
	adrp	x0, .L.str.236
	add	x0, x0, :lo12:.L.str.236
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #682]
	adrp	x0, .L.str.237
	add	x0, x0, :lo12:.L.str.237
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #688]
	adrp	x0, .L.str.238
	add	x0, x0, :lo12:.L.str.238
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #684]
	adrp	x0, .L.str.239
	add	x0, x0, :lo12:.L.str.239
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #698]
	adrp	x0, .L.str.240
	add	x0, x0, :lo12:.L.str.240
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #48]
	ldrh	w1, [x8]
	adrp	x0, .L.str.241
	add	x0, x0, :lo12:.L.str.241
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #56]
	ldrh	w1, [x8]
	adrp	x0, .L.str.242
	add	x0, x0, :lo12:.L.str.242
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	add	x1, x8, #464                    // =464
	sub	x0, x29, #56                    // =56
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	mov	x2, #48
	str	x2, [sp, #24]                   // 8-byte Folded Spill
	bl	memcpy
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	adrp	x0, .L.str.243
	add	x0, x0, :lo12:.L.str.243
	bl	nand_print_dbg
	ldr	x2, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	add	x1, x8, #536                    // =536
	add	x0, sp, #88                     // =88
	str	x0, [sp, #32]                   // 8-byte Folded Spill
	bl	memcpy
	ldr	x1, [sp, #32]                   // 8-byte Folded Reload
	adrp	x0, .L.str.244
	add	x0, x0, :lo12:.L.str.244
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x1, [x8, #528]
	adrp	x0, .L.str.245
	add	x0, x0, :lo12:.L.str.245
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #528]
	ldrh	w1, [x8]
	adrp	x0, .L.str.246
	add	x0, x0, :lo12:.L.str.246
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #584]
	adrp	x0, .L.str.247
	add	x0, x0, :lo12:.L.str.247
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #528]
	ldr	w1, [x8, #8]
	adrp	x0, .L.str.248
	add	x0, x0, :lo12:.L.str.248
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #64]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #64]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.249
	add	x0, x0, :lo12:.L.str.249
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB140_4
// %bb.3:
	ldur	x8, [x29, #-8]
	add	x1, x8, #600                    // =600
	add	x0, sp, #40                     // =40
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	x2, #48
	bl	memcpy
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	adrp	x0, .L.str.250
	add	x0, x0, :lo12:.L.str.250
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x1, [x8, #592]
	adrp	x0, .L.str.251
	add	x0, x0, :lo12:.L.str.251
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #592]
	ldrh	w1, [x8]
	adrp	x0, .L.str.252
	add	x0, x0, :lo12:.L.str.252
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #648]
	adrp	x0, .L.str.253
	add	x0, x0, :lo12:.L.str.253
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #592]
	ldr	w1, [x8, #8]
	adrp	x0, .L.str.254
	add	x0, x0, :lo12:.L.str.254
	bl	nand_print_dbg
.LBB140_4:
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #660]
	adrp	x0, .L.str.255
	add	x0, x0, :lo12:.L.str.255
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x1, [x8, #512]
	adrp	x0, .L.str.256
	add	x0, x0, :lo12:.L.str.256
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #676]
	adrp	x0, .L.str.257
	add	x0, x0, :lo12:.L.str.257
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldrh	w1, [x8, #1072]
	adrp	x0, .L.str.258
	add	x0, x0, :lo12:.L.str.258
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #668]
	adrp	x0, .L.str.259
	add	x0, x0, :lo12:.L.str.259
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #672]
	adrp	x0, .L.str.260
	add	x0, x0, :lo12:.L.str.260
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8]
	adrp	x0, .L.str.261
	add	x0, x0, :lo12:.L.str.261
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #2]
	adrp	x0, .L.str.262
	add	x0, x0, :lo12:.L.str.262
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #4]
	adrp	x0, .L.str.263
	add	x0, x0, :lo12:.L.str.263
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #6]
	adrp	x0, .L.str.264
	add	x0, x0, :lo12:.L.str.264
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #10]
	adrp	x0, .L.str.265
	add	x0, x0, :lo12:.L.str.265
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #12]
	adrp	x0, .L.str.266
	add	x0, x0, :lo12:.L.str.266
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #14]
	adrp	x0, .L.str.267
	add	x0, x0, :lo12:.L.str.267
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #16]
	adrp	x0, .L.str.268
	add	x0, x0, :lo12:.L.str.268
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #18]
	adrp	x0, .L.str.269
	add	x0, x0, :lo12:.L.str.269
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #20]
	adrp	x0, .L.str.270
	add	x0, x0, :lo12:.L.str.270
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #22]
	adrp	x0, .L.str.271
	add	x0, x0, :lo12:.L.str.271
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #24]
	adrp	x0, .L.str.272
	add	x0, x0, :lo12:.L.str.272
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w1, [x8, #26]
	adrp	x0, .L.str.273
	add	x0, x0, :lo12:.L.str.273
	bl	nand_print_dbg
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldr	w1, [x8, #32]
	adrp	x0, .L.str.274
	add	x0, x0, :lo12:.L.str.274
	bl	nand_print_dbg
	ldur	x0, [x29, #-8]
	bl	print_smart
.LBB140_5:
	ldp	x29, x30, [sp, #192]            // 16-byte Folded Reload
	add	sp, sp, #208                    // =208
	ret
.Lfunc_end140:
	.size	print_nftl_zone, .Lfunc_end140-print_nftl_zone
	.cfi_endproc
                                        // -- End function
	.globl	panic_backup_block              // -- Begin function panic_backup_block
	.p2align	2
	.type	panic_backup_block,@function
panic_backup_block:                     // @panic_backup_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, panic_info
	add	x8, x8, :lo12:panic_info
	str	x8, [sp]                        // 8-byte Folded Spill
	stur	x0, [x29, #-16]
	bl	is_panic_enable
	cbnz	w0, .LBB141_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB141_12
.LBB141_2:
	stur	wzr, [x29, #-20]
.LBB141_3:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	subs	w8, w8, #3                      // =3
	b.ge	.LBB141_11
// %bb.4:                               //   in Loop: Header=BB141_3 Depth=1
	ldursw	x9, [x29, #-20]
	adrp	x8, panic_info
	add	x8, x8, :lo12:panic_info
	ldr	x8, [x8, x9, lsl #3]
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB141_6
// %bb.5:
	ldr	x8, [sp, #16]
	ldrh	w1, [x8]
	adrp	x0, .L.str.349
	add	x0, x0, :lo12:.L.str.349
	bl	nand_print
	stur	wzr, [x29, #-4]
	b	.LBB141_12
.LBB141_6:                              //   in Loop: Header=BB141_3 Depth=1
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB141_8
// %bb.7:                               //   in Loop: Header=BB141_3 Depth=1
	ldr	x8, [sp, #16]
	ldrh	w1, [x8]
	adrp	x0, .L.str.350
	add	x0, x0, :lo12:.L.str.350
	bl	nand_print
	ldr	x8, [sp, #16]
	ldursw	x10, [x29, #-20]
	adrp	x9, panic_info
	add	x9, x9, :lo12:panic_info
	add	x9, x9, x10, lsl #3
	str	x8, [x9]
	b	.LBB141_9
.LBB141_8:
	adrp	x0, .L.str.351
	add	x0, x0, :lo12:.L.str.351
	bl	nand_print
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB141_12
.LBB141_9:                              //   in Loop: Header=BB141_3 Depth=1
// %bb.10:                              //   in Loop: Header=BB141_3 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB141_3
.LBB141_11:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldrh	w10, [x8, #20]
	mov	w8, #3
	mul	w8, w8, w10
	str	w8, [x9, #24]
	stur	wzr, [x29, #-4]
.LBB141_12:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end141:
	.size	panic_backup_block, .Lfunc_end141-panic_backup_block
	.cfi_endproc
                                        // -- End function
	.globl	is_panic_data_page              // -- Begin function is_panic_data_page
	.p2align	2
	.type	is_panic_data_page,@function
is_panic_data_page:                     // @is_panic_data_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldrb	w9, [x8, #1]
	mov	w8, #0
	subs	w9, w9, #170                    // =170
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	b.ne	.LBB142_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #2]
	subs	w8, w8, #99                     // =99
	cset	w8, eq
	str	w8, [sp, #4]                    // 4-byte Folded Spill
.LBB142_2:
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	and	w0, w8, #0x1
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end142:
	.size	is_panic_data_page, .Lfunc_end142-is_panic_data_page
	.cfi_endproc
                                        // -- End function
	.globl	panic_recover_info              // -- Begin function panic_recover_info
	.p2align	2
	.type	panic_recover_info,@function
panic_recover_info:                     // @panic_recover_info
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #4]
	str	w8, [sp, #4]
	ldr	w8, [sp, #4]
	subs	w8, w8, #3                      // =3
	b.hs	.LBB143_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldrh	w1, [x8]
	adrp	x0, .L.str.356
	add	x0, x0, :lo12:.L.str.356
	bl	nand_print
	ldr	x8, [sp, #8]
	ldr	w9, [sp, #4]
	mov	w10, w9
	adrp	x9, panic_info
	add	x9, x9, :lo12:panic_info
	add	x9, x9, x10, lsl #3
	str	x8, [x9]
	stur	wzr, [x29, #-4]
	b	.LBB143_3
.LBB143_2:
	ldr	w1, [sp, #4]
	adrp	x0, .L.str.357
	add	x0, x0, :lo12:.L.str.357
	bl	nand_print
	mov	w8, #-1
	stur	w8, [x29, #-4]
.LBB143_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end143:
	.size	panic_recover_info, .Lfunc_end143-panic_recover_info
	.cfi_endproc
                                        // -- End function
	.globl	torture_block_and_mark          // -- Begin function torture_block_and_mark
	.p2align	2
	.type	torture_block_and_mark,@function
torture_block_and_mark:                 // @torture_block_and_mark
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160                    // =160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144                   // =144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	mov	w8, wzr
	str	w8, [sp, #44]                   // 4-byte Folded Spill
	stur	wzr, [x29, #-28]
	stur	wzr, [x29, #-32]
	stur	wzr, [x29, #-36]
	stur	wzr, [x29, #-40]
	mov	w8, #3
	stur	w8, [x29, #-44]
	sub	x8, x29, #64                    // =64
	stur	xzr, [x29, #-64]
	stur	xzr, [x8, #7]
	mov	w8, #1
	stur	w8, [x29, #-36]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	adrp	x0, .L.str.194
	add	x0, x0, :lo12:.L.str.194
	bl	nand_print
	ldr	w2, [sp, #44]                   // 4-byte Folded Reload
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #48]                   // 8-byte Folded Spill
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #48]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB144_2
// %bb.1:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	adrp	x0, .L.str.193
	add	x0, x0, :lo12:.L.str.193
	bl	nand_print
	b	.LBB144_26
.LBB144_2:
	ldur	x9, [x29, #-24]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
	stur	wzr, [x29, #-32]
.LBB144_3:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB144_5 Depth 2
                                        //     Child Loop BB144_11 Depth 2
	ldur	w8, [x29, #-32]
	ldur	w9, [x29, #-36]
	subs	w8, w8, w9
	b.ge	.LBB144_22
// %bb.4:                               //   in Loop: Header=BB144_3 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #512]
	ldursw	x9, [x29, #-32]
	adrp	x8, patterns
	add	x8, x8, :lo12:patterns
	str	x8, [sp, #32]                   // 8-byte Folded Spill
	ldrb	w1, [x8, x9]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #26]
	mov	w2, w8
	bl	memset
	ldr	x8, [sp, #32]                   // 8-byte Folded Reload
	ldursw	x9, [x29, #-32]
	ldrb	w9, [x8, x9]
                                        // implicit-def: $x8
	mov	w8, w9
	and	x8, x8, #0xff
	mov	x9, #72340172838076673
	mul	x8, x8, x9
	sub	x9, x29, #64                    // =64
	stur	x8, [x29, #-64]
	stur	x8, [x9, #7]
	stur	wzr, [x29, #-40]
.LBB144_5:                              //   Parent Loop BB144_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-40]
	ldur	w9, [x29, #-44]
	subs	w8, w8, w9
	b.ge	.LBB144_10
// %bb.6:                               //   in Loop: Header=BB144_5 Depth=2
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-40]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	sub	x5, x29, #64                    // =64
	bl	set_physic_op_par
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB144_8
// %bb.7:
	ldur	x8, [x29, #-24]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-40]
	adrp	x0, .L.str.195
	add	x0, x0, :lo12:.L.str.195
	adrp	x1, .L__func__.torture_block_and_mark
	add	x1, x1, :lo12:.L__func__.torture_block_and_mark
	bl	nand_print
	b	.LBB144_26
.LBB144_8:                              //   in Loop: Header=BB144_5 Depth=2
// %bb.9:                               //   in Loop: Header=BB144_5 Depth=2
	ldur	w8, [x29, #-40]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-40]
	b	.LBB144_5
.LBB144_10:                             //   in Loop: Header=BB144_3 Depth=1
	stur	wzr, [x29, #-40]
.LBB144_11:                             //   Parent Loop BB144_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-40]
	ldur	w9, [x29, #-44]
	subs	w8, w8, w9
	b.ge	.LBB144_20
// %bb.12:                              //   in Loop: Header=BB144_11 Depth=2
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #512]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #26]
	mov	w2, w8
	mov	w1, wzr
	bl	memset
	sub	x5, x29, #64                    // =64
	stur	xzr, [x29, #-64]
	stur	xzr, [x5, #7]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-40]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB144_14
// %bb.13:
	ldur	x8, [x29, #-24]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-40]
	adrp	x0, .L.str.196
	add	x0, x0, :lo12:.L.str.196
	adrp	x1, .L__func__.torture_block_and_mark
	add	x1, x1, :lo12:.L__func__.torture_block_and_mark
	bl	nand_print
	b	.LBB144_26
.LBB144_14:                             //   in Loop: Header=BB144_11 Depth=2
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #512]
	ldursw	x9, [x29, #-32]
	adrp	x8, patterns
	add	x8, x8, :lo12:patterns
	ldrb	w1, [x8, x9]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w2, [x8, #26]
	bl	nftl_check_pattern
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB144_16
// %bb.15:
	ldur	x8, [x29, #-24]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-40]
	ldursw	x9, [x29, #-32]
	adrp	x8, patterns
	add	x8, x8, :lo12:patterns
	ldrb	w4, [x8, x9]
	adrp	x0, .L.str.197
	add	x0, x0, :lo12:.L.str.197
	adrp	x1, .L__func__.torture_block_and_mark
	add	x1, x1, :lo12:.L__func__.torture_block_and_mark
	bl	nand_print
	b	.LBB144_26
.LBB144_16:                             //   in Loop: Header=BB144_11 Depth=2
	ldursw	x9, [x29, #-32]
	adrp	x8, patterns
	add	x8, x8, :lo12:patterns
	ldrb	w1, [x8, x9]
	sub	x0, x29, #64                    // =64
	mov	w2, #15
	bl	nftl_check_pattern
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB144_18
// %bb.17:
	ldur	x8, [x29, #-24]
	ldrh	w2, [x8]
	ldur	w3, [x29, #-40]
	ldursw	x9, [x29, #-32]
	adrp	x8, patterns
	add	x8, x8, :lo12:patterns
	ldrb	w4, [x8, x9]
	adrp	x0, .L.str.198
	add	x0, x0, :lo12:.L.str.198
	adrp	x1, .L__func__.torture_block_and_mark
	add	x1, x1, :lo12:.L__func__.torture_block_and_mark
	bl	nand_print
	b	.LBB144_26
.LBB144_18:                             //   in Loop: Header=BB144_11 Depth=2
// %bb.19:                              //   in Loop: Header=BB144_11 Depth=2
	ldur	w8, [x29, #-40]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-40]
	b	.LBB144_11
.LBB144_20:                             //   in Loop: Header=BB144_3 Depth=1
// %bb.21:                              //   in Loop: Header=BB144_3 Depth=1
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
	b	.LBB144_3
.LBB144_22:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB144_24
// %bb.23:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	adrp	x0, .L.str.199
	add	x0, x0, :lo12:.L.str.199
	bl	nand_print
	b	.LBB144_26
.LBB144_24:
	ldur	x9, [x29, #-24]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
	ldur	x9, [x29, #-24]
	mov	w8, #-1
	str	w8, [x9, #8]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	bl	put_phy_block_to_free_list
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	bl	mark_free_block
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	ldur	x9, [x29, #-24]
	strh	w8, [x9, #4]
	ldur	x8, [x29, #-24]
	strh	wzr, [x8, #2]
// %bb.25:
	stur	wzr, [x29, #-4]
	b	.LBB144_27
.LBB144_26:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	ldur	x8, [x29, #-24]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-24]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
	ldur	w8, [x29, #-28]
	stur	w8, [x29, #-4]
.LBB144_27:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160                    // =160
	ret
.Lfunc_end144:
	.size	torture_block_and_mark, .Lfunc_end144-torture_block_and_mark
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function crc_wrong
	.type	crc_wrong,@function
crc_wrong:                              // @crc_wrong
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	x1, [sp, #24]
	str	x2, [sp, #16]
	strh	wzr, [sp, #14]
	strh	wzr, [sp, #12]
	str	wzr, [sp, #8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	lsl	w8, w8, #9
	str	w8, [sp, #8]
	ldr	x0, [sp, #16]
	bl	is_nouse_page
	cbz	w0, .LBB145_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB145_7
.LBB145_2:
	ldr	x0, [sp, #16]
	bl	is_phy_mapping_page
	cbz	w0, .LBB145_4
// %bb.3:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #560]
	str	w8, [sp, #8]
.LBB145_4:
	ldr	x1, [sp, #24]
	ldr	w2, [sp, #8]
	mov	w0, #-1
	bl	nand_crc16
	strh	w0, [sp, #14]
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #13]
	ldr	x9, [sp, #16]
	ldrb	w9, [x9, #12]
	orr	w8, w8, w9, lsl #8
	strh	w8, [sp, #12]
	ldrh	w8, [sp, #14]
	ldrh	w9, [sp, #12]
	subs	w8, w8, w9
	b.eq	.LBB145_6
// %bb.5:
	ldrh	w1, [sp, #14]
	ldrh	w2, [sp, #12]
	ldr	w3, [sp, #8]
	adrp	x0, .L.str.485
	add	x0, x0, :lo12:.L.str.485
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB145_7
.LBB145_6:
	stur	wzr, [x29, #-4]
.LBB145_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end145:
	.size	crc_wrong, .Lfunc_end145-crc_wrong
	.cfi_endproc
                                        // -- End function
	.globl	recover_logic_page_mapping      // -- Begin function recover_logic_page_mapping
	.p2align	2
	.type	recover_logic_page_mapping,@function
recover_logic_page_mapping:             // @recover_logic_page_mapping
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	x2, [x29, #-24]
	stur	w3, [x29, #-28]
	str	wzr, [sp, #28]
.LBB146_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #28]
	ldur	w9, [x29, #-28]
	subs	w8, w8, w9
	b.hs	.LBB146_20
// %bb.2:                               //   in Loop: Header=BB146_1 Depth=1
	ldur	x8, [x29, #-24]
	ldr	w9, [sp, #28]
                                        // kill: def $x9 killed $w9
	ldr	w8, [x8, x9, lsl #2]
	str	w8, [sp, #32]
	ldr	w8, [sp, #32]
	ldur	x9, [x29, #-8]
	ldr	w9, [x9, #36]
	subs	w8, w8, w9
	b.hs	.LBB146_13
// %bb.3:                               //   in Loop: Header=BB146_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	w1, [sp, #32]
	bl	get_logic_page_map
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB146_5
// %bb.4:                               //   in Loop: Header=BB146_1 Depth=1
	adrp	x0, .L.str.166
	add	x0, x0, :lo12:.L.str.166
	bl	nand_print
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	b	.LBB146_19
.LBB146_5:                              //   in Loop: Header=BB146_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #2]
	mov	w9, #65535
	subs	w8, w8, w9
	b.eq	.LBB146_12
// %bb.6:                               //   in Loop: Header=BB146_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #8]
	ldrh	w1, [x8, #2]
	bl	get_phy_block_addr
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-16]
	subs	x8, x8, x9
	b.eq	.LBB146_8
// %bb.7:                               //   in Loop: Header=BB146_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #16]
	bl	phy_block_from_invalid_page_incr
	b	.LBB146_9
.LBB146_8:                              //   in Loop: Header=BB146_1 Depth=1
	ldr	x9, [sp, #16]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
.LBB146_9:                              //   in Loop: Header=BB146_1 Depth=1
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.le	.LBB146_11
// %bb.10:                              //   in Loop: Header=BB146_1 Depth=1
	ldr	x8, [sp, #16]
	ldrh	w1, [x8]
	adrp	x0, .L.str.167
	add	x0, x0, :lo12:.L.str.167
	bl	nand_print
.LBB146_11:                             //   in Loop: Header=BB146_1 Depth=1
.LBB146_12:                             //   in Loop: Header=BB146_1 Depth=1
	ldr	w10, [sp, #28]
	ldr	x9, [sp, #8]
	ldrh	w8, [x9]
	and	w10, w10, #0xfff
	and	w8, w8, #0xfffff000
	orr	w8, w8, w10
	strh	w8, [x9]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8]
	ldr	x9, [sp, #8]
	strh	w8, [x9, #2]
	b	.LBB146_18
.LBB146_13:                             //   in Loop: Header=BB146_1 Depth=1
	ldr	w8, [sp, #32]
	adds	w8, w8, #1                      // =1
	b.eq	.LBB146_15
// %bb.14:                              //   in Loop: Header=BB146_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldr	w2, [sp, #28]
	ldr	w3, [sp, #32]
	adrp	x0, .L.str.168
	add	x0, x0, :lo12:.L.str.168
	bl	nand_print
.LBB146_15:                             //   in Loop: Header=BB146_1 Depth=1
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.le	.LBB146_17
// %bb.16:                              //   in Loop: Header=BB146_1 Depth=1
	adrp	x0, .L.str.169
	add	x0, x0, :lo12:.L.str.169
	bl	nand_print
.LBB146_17:                             //   in Loop: Header=BB146_1 Depth=1
.LBB146_18:                             //   in Loop: Header=BB146_1 Depth=1
.LBB146_19:                             //   in Loop: Header=BB146_1 Depth=1
	ldr	w8, [sp, #28]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #28]
	b	.LBB146_1
.LBB146_20:
	mov	w0, wzr
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end146:
	.size	recover_logic_page_mapping, .Lfunc_end146-recover_logic_page_mapping
	.cfi_endproc
                                        // -- End function
	.globl	recover_phy_page_mapping        // -- Begin function recover_phy_page_mapping
	.p2align	2
	.type	recover_phy_page_mapping,@function
recover_phy_page_mapping:               // @recover_phy_page_mapping
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	ldur	x0, [x29, #-32]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #560]
	mov	w1, #-1
	bl	nand_memset
	stur	wzr, [x29, #-36]
.LBB147_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-36]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ge	.LBB147_32
// %bb.2:                               //   in Loop: Header=BB147_1 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #16                     // =16
	str	x0, [sp]                        // 8-byte Folded Spill
	add	x5, sp, #40                     // =40
	str	x5, [sp, #8]                    // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	mov	w8, w0
	ldr	x0, [sp, #8]                    // 8-byte Folded Reload
	stur	w8, [x29, #-40]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB147_7
// %bb.3:                               //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB147_5
// %bb.4:                               //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB147_6
.LBB147_5:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.155
	add	x0, x0, :lo12:.L.str.155
	bl	nand_print
	b	.LBB147_32
.LBB147_6:                              //   in Loop: Header=BB147_1 Depth=1
.LBB147_7:                              //   in Loop: Header=BB147_1 Depth=1
	add	x0, sp, #40                     // =40
	bl	is_ftl_logic_page_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB147_17
// %bb.8:                               //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB147_10
// %bb.9:                               //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB147_16
.LBB147_10:                             //   in Loop: Header=BB147_1 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #512]
	add	x2, sp, #40                     // =40
	bl	nand_power_off_ecc_error
	cbz	w0, .LBB147_12
// %bb.11:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.156
	add	x0, x0, :lo12:.L.str.156
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-40]
	b	.LBB147_15
.LBB147_12:                             //   in Loop: Header=BB147_1 Depth=1
	add	x0, sp, #40                     // =40
	bl	get_logic_page_from_oob
	ldur	x8, [x29, #-32]
	ldursw	x9, [x29, #-36]
	str	w0, [x8, x9, lsl #2]
	ldur	x8, [x29, #-32]
	ldursw	x9, [x29, #-36]
	ldr	w8, [x8, x9, lsl #2]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #36]
	subs	w8, w8, w9
	b.lo	.LBB147_14
// %bb.13:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-32]
	ldursw	x9, [x29, #-36]
	ldr	w3, [x8, x9, lsl #2]
	adrp	x0, .L.str.157
	add	x0, x0, :lo12:.L.str.157
	bl	nand_print
	ldur	x9, [x29, #-32]
	ldursw	x10, [x29, #-36]
	mov	w8, #-1
	str	w8, [x9, x10, lsl #2]
.LBB147_14:                             //   in Loop: Header=BB147_1 Depth=1
.LBB147_15:                             //   in Loop: Header=BB147_1 Depth=1
.LBB147_16:                             //   in Loop: Header=BB147_1 Depth=1
	b	.LBB147_27
.LBB147_17:                             //   in Loop: Header=BB147_1 Depth=1
	add	x0, sp, #40                     // =40
	bl	is_ftl_special_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB147_19
// %bb.18:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	w1, [x29, #-36]
	adrp	x0, .L.str.158
	add	x0, x0, :lo12:.L.str.158
	bl	nand_print
	b	.LBB147_26
.LBB147_19:                             //   in Loop: Header=BB147_1 Depth=1
	add	x0, sp, #40                     // =40
	bl	is_power_down_info_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB147_24
// %bb.20:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB147_22
// %bb.21:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB147_23
.LBB147_22:                             //   in Loop: Header=BB147_1 Depth=1
	add	x0, sp, #40                     // =40
	bl	get_special_data_from_oob
	ldur	x8, [x29, #-32]
	ldursw	x9, [x29, #-36]
	str	w0, [x8, x9, lsl #2]
	ldur	w1, [x29, #-36]
	adrp	x0, .L.str.159
	add	x0, x0, :lo12:.L.str.159
	bl	nand_print
.LBB147_23:                             //   in Loop: Header=BB147_1 Depth=1
	b	.LBB147_25
.LBB147_24:                             //   in Loop: Header=BB147_1 Depth=1
	ldur	w1, [x29, #-36]
	adrp	x0, .L.str.160
	add	x0, x0, :lo12:.L.str.160
	bl	nand_print
.LBB147_25:                             //   in Loop: Header=BB147_1 Depth=1
.LBB147_26:                             //   in Loop: Header=BB147_1 Depth=1
.LBB147_27:                             //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB147_30
// %bb.28:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-40]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB147_30
// %bb.29:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.161
	add	x0, x0, :lo12:.L.str.161
	bl	nand_print
	mov	w8, #65535
	stur	w8, [x29, #-4]
	b	.LBB147_37
.LBB147_30:                             //   in Loop: Header=BB147_1 Depth=1
// %bb.31:                              //   in Loop: Header=BB147_1 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB147_1
.LBB147_32:
	ldur	w8, [x29, #-36]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #2                      // =2
	subs	w8, w8, w9
	b.ge	.LBB147_36
// %bb.33:
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbnz	w8, #0, .LBB147_35
// %bb.34:
.LBB147_35:
.LBB147_36:
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
.LBB147_37:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end147:
	.size	recover_phy_page_mapping, .Lfunc_end147-recover_phy_page_mapping
	.cfi_endproc
                                        // -- End function
	.globl	init_smart_info                 // -- Begin function init_smart_info
	.p2align	2
	.type	init_smart_info,@function
init_smart_info:                        // @init_smart_info
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160                    // =160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144                   // =144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #3440]
	mov	w1, wzr
	mov	w2, #320
	bl	nand_memset
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	mov	w8, #43521
	movk	w8, #43690, lsl #16
	str	w8, [x9, #112]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #140]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #140]
	ldur	x8, [x29, #-24]
	cbnz	x8, .LBB148_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB148_15
.LBB148_2:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w2, w8, #1                      // =1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #72                     // =72
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	sub	x5, x29, #44                    // =44
	str	x5, [sp, #16]                   // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	mov	w8, w0
	ldr	x0, [sp, #16]                   // 8-byte Folded Reload
	stur	w8, [x29, #-28]
	bl	is_phy_mapping_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB148_14
// %bb.3:
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB148_5
// %bb.4:
	ldur	w8, [x29, #-28]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB148_14
.LBB148_5:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #512]
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #560]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	str	x8, [sp, #56]
	ldr	x8, [sp, #56]
	ldr	w8, [x8, #112]
	mov	w9, #43521
	movk	w9, #43690, lsl #16
	subs	w8, w8, w9
	b.ne	.LBB148_13
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #3440]
	ldr	x1, [sp, #56]
	mov	w2, #320
	bl	nand_memcpy
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #140]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #140]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w8, [x8, #164]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB148_8
// %bb.7:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	str	wzr, [x8, #164]
.LBB148_8:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w8, [x8, #172]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB148_10
// %bb.9:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	str	wzr, [x8, #172]
.LBB148_10:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	ldr	w8, [x8, #176]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB148_12
// %bb.11:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3440]
	str	wzr, [x8, #176]
.LBB148_12:
.LBB148_13:
.LBB148_14:
	ldur	x0, [x29, #-16]
	bl	print_smart
	stur	wzr, [x29, #-4]
.LBB148_15:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160                    // =160
	ret
.Lfunc_end148:
	.size	init_smart_info, .Lfunc_end148-init_smart_info
	.cfi_endproc
                                        // -- End function
	.globl	cross_talk_rebuild_current_block // -- Begin function cross_talk_rebuild_current_block
	.p2align	2
	.type	cross_talk_rebuild_current_block,@function
cross_talk_rebuild_current_block:       // @cross_talk_rebuild_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #176                    // =176
	stp	x29, x30, [sp, #160]            // 16-byte Folded Spill
	add	x29, sp, #160                   // =160
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	stur	wzr, [x29, #-68]
	add	x1, sp, #24                     // =24
	str	xzr, [sp, #24]
	str	xzr, [sp, #16]
	str	xzr, [sp, #8]
	ldur	x0, [x29, #-16]
	bl	init_nrc
	cbz	w0, .LBB149_2
// %bb.1:
	ldr	x0, [sp, #24]
	bl	free_nrc
	adrp	x0, .L.str.177
	add	x0, x0, :lo12:.L.str.177
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB149_33
.LBB149_2:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldr	x3, [sp, #24]
	sub	x2, x29, #44                    // =44
	bl	get_used_page_num
	stur	w0, [x29, #-36]
	ldur	x8, [x29, #-32]
	cbnz	x8, .LBB149_8
// %bb.3:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-24]
	ldr	w3, [x8, #8]
	adrp	x0, .L.str.178
	add	x0, x0, :lo12:.L.str.178
	bl	nand_print
	ldur	x8, [x29, #-24]
	str	x8, [sp, #80]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-64]
	ldr	x8, [sp, #24]
	ldr	w8, [x8]
	cbnz	w8, .LBB149_5
// %bb.4:
	ldr	x0, [sp, #24]
	bl	free_nrc
	adrp	x0, .L.str.179
	add	x0, x0, :lo12:.L.str.179
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	stur	xzr, [x29, #-8]
	b	.LBB149_33
.LBB149_5:
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB149_7
// %bb.6:
	ldur	w8, [x29, #-64]
	add	w8, w8, #20                     // =20
	stur	w8, [x29, #-64]
.LBB149_7:
	b	.LBB149_20
.LBB149_8:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	ldr	x3, [sp, #24]
	sub	x2, x29, #48                    // =48
	bl	get_used_page_num
	stur	w0, [x29, #-40]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-24]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-32]
	ldrh	w4, [x8]
	ldur	w5, [x29, #-40]
	ldur	x8, [x29, #-32]
	ldr	w6, [x8, #8]
	adrp	x0, .L.str.180
	add	x0, x0, :lo12:.L.str.180
	bl	nand_print
	ldr	x8, [sp, #24]
	ldr	w8, [x8]
	cbnz	w8, .LBB149_10
// %bb.9:
	ldr	x0, [sp, #24]
	bl	free_nrc
	adrp	x0, .L.str.181
	add	x0, x0, :lo12:.L.str.181
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	stur	xzr, [x29, #-8]
	b	.LBB149_33
.LBB149_10:
	ldur	w8, [x29, #-40]
	ldur	w9, [x29, #-36]
	subs	w8, w8, w9
	b.lo	.LBB149_12
// %bb.11:
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-64]
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-68]
	b	.LBB149_13
.LBB149_12:
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-64]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	stur	w8, [x29, #-68]
.LBB149_13:
	ldur	w8, [x29, #-44]
	cbnz	w8, .LBB149_15
// %bb.14:
	ldur	w8, [x29, #-48]
	cbz	w8, .LBB149_16
.LBB149_15:
	ldur	w8, [x29, #-64]
	add	w8, w8, #20                     // =20
	stur	w8, [x29, #-64]
	ldur	w8, [x29, #-68]
	add	w8, w8, #20                     // =20
	stur	w8, [x29, #-68]
.LBB149_16:
	ldur	w8, [x29, #-40]
	ldur	w9, [x29, #-36]
	subs	w8, w8, w9
	b.ls	.LBB149_18
// %bb.17:
	ldur	w1, [x29, #-36]
	ldur	w2, [x29, #-40]
	ldr	x8, [sp, #24]
	ldr	w3, [x8]
	adrp	x0, .L.str.182
	add	x0, x0, :lo12:.L.str.182
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	w8, [x29, #-64]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-32]
	str	x8, [sp, #80]
	b	.LBB149_19
.LBB149_18:
	ldur	w1, [x29, #-36]
	ldur	w2, [x29, #-40]
	ldr	x8, [sp, #24]
	ldr	w3, [x8]
	adrp	x0, .L.str.183
	add	x0, x0, :lo12:.L.str.183
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	ldur	w8, [x29, #-64]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-24]
	str	x8, [sp, #80]
.LBB149_19:
.LBB149_20:
.LBB149_21:                             // =>This Inner Loop Header: Depth=1
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	str	x0, [sp, #72]
	ldr	x8, [sp, #72]
	cbnz	x8, .LBB149_23
// %bb.22:
	adrp	x0, .L.str.184
	add	x0, x0, :lo12:.L.str.184
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB149_33
.LBB149_23:                             //   in Loop: Header=BB149_21 Depth=1
	ldr	x8, [sp, #80]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldr	x9, [sp, #72]
	str	w8, [x9, #8]
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #72]
	ldr	x8, [sp, #72]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB149_25
// %bb.24:                              //   in Loop: Header=BB149_21 Depth=1
	adrp	x0, .L.str.185
	add	x0, x0, :lo12:.L.str.185
	bl	nand_print
	b	.LBB149_21
.LBB149_25:
	stur	wzr, [x29, #-52]
	stur	wzr, [x29, #-60]
.LBB149_26:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-52]
	ldr	x9, [sp, #24]
	ldr	w9, [x9]
	subs	w8, w8, w9
	b.hs	.LBB149_32
// %bb.27:                              //   in Loop: Header=BB149_26 Depth=1
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-52]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbz	w8, .LBB149_29
// %bb.28:                              //   in Loop: Header=BB149_26 Depth=1
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #16]
	ldur	w9, [x29, #-52]
	ldur	x10, [x29, #-16]
	ldr	x10, [x10, #8]
	ldrh	w10, [x10, #26]
	add	w10, w10, #64                   // =64
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #16]
	ldr	x8, [sp, #72]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-60]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #8]
	ldr	x5, [sp, #16]
	add	x0, sp, #32                     // =32
	str	x0, [sp]                        // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x0, [sp, #16]
	bl	get_special_data_from_oob
	stur	w0, [x29, #-56]
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #16]
	ldur	w2, [x29, #-56]
	ldr	x8, [sp, #72]
	ldr	w3, [x8, #8]
	ldr	x8, [sp, #72]
	ldrh	w4, [x8, #6]
	bl	set_oob_special_page
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	blr	x8
	ldur	w8, [x29, #-60]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-60]
	b	.LBB149_30
.LBB149_29:                             //   in Loop: Header=BB149_26 Depth=1
	ldur	w1, [x29, #-52]
	ldr	x8, [sp, #24]
	ldr	w2, [x8]
	adrp	x0, .L.str.186
	add	x0, x0, :lo12:.L.str.186
	bl	nand_print
.LBB149_30:                             //   in Loop: Header=BB149_26 Depth=1
// %bb.31:                              //   in Loop: Header=BB149_26 Depth=1
	ldur	w8, [x29, #-52]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-52]
	b	.LBB149_26
.LBB149_32:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #80]
	ldur	w8, [x29, #-68]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldr	x0, [sp, #24]
	bl	free_nrc
	ldr	x8, [sp, #72]
	stur	x8, [x29, #-8]
.LBB149_33:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #160]            // 16-byte Folded Reload
	add	sp, sp, #176                    // =176
	ret
.Lfunc_end149:
	.size	cross_talk_rebuild_current_block, .Lfunc_end149-cross_talk_rebuild_current_block
	.cfi_endproc
                                        // -- End function
	.globl	no_cross_talk_rebuild_current_block // -- Begin function no_cross_talk_rebuild_current_block
	.p2align	2
	.type	no_cross_talk_rebuild_current_block,@function
no_cross_talk_rebuild_current_block:    // @no_cross_talk_rebuild_current_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #224                    // =224
	stp	x29, x30, [sp, #208]            // 16-byte Folded Spill
	add	x29, sp, #208                   // =208
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	str	xzr, [sp, #104]
	str	xzr, [sp, #96]
	add	x1, sp, #40                     // =40
	str	xzr, [sp, #40]
	str	xzr, [sp, #32]
	str	xzr, [sp, #24]
	ldur	x0, [x29, #-16]
	bl	init_nrc
	cbz	w0, .LBB150_2
// %bb.1:
	ldr	x0, [sp, #40]
	bl	free_nrc
	adrp	x0, .L.str.177
	add	x0, x0, :lo12:.L.str.177
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB150_34
.LBB150_2:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldr	x3, [sp, #40]
	sub	x2, x29, #44                    // =44
	bl	get_used_page_num_no_crosstalk
	stur	w0, [x29, #-36]
	ldur	x8, [x29, #-32]
	cbnz	x8, .LBB150_11
// %bb.3:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-24]
	ldr	w3, [x8, #8]
	adrp	x0, .L.str.187
	add	x0, x0, :lo12:.L.str.187
	bl	nand_print
	ldur	x8, [x29, #-24]
	stur	x8, [x29, #-88]
	ldr	x8, [sp, #40]
	ldr	w8, [x8]
	cbnz	w8, .LBB150_5
// %bb.4:
	ldr	x0, [sp, #40]
	bl	free_nrc
	adrp	x0, .L.str.188
	add	x0, x0, :lo12:.L.str.188
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	stur	xzr, [x29, #-8]
	b	.LBB150_34
.LBB150_5:
	ldr	x8, [sp, #40]
	str	x8, [sp, #24]
.LBB150_6:                              // =>This Inner Loop Header: Depth=1
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	stur	x0, [x29, #-96]
	ldur	x8, [x29, #-96]
	cbnz	x8, .LBB150_8
// %bb.7:
	adrp	x0, .L.str.184
	add	x0, x0, :lo12:.L.str.184
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB150_34
.LBB150_8:                              //   in Loop: Header=BB150_6 Depth=1
	ldur	x8, [x29, #-88]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-96]
	str	w8, [x9, #8]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-96]
	ldur	x8, [x29, #-96]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB150_10
// %bb.9:                               //   in Loop: Header=BB150_6 Depth=1
	adrp	x0, .L.str.185
	add	x0, x0, :lo12:.L.str.185
	bl	nand_print
	b	.LBB150_6
.LBB150_10:
	b	.LBB150_26
.LBB150_11:
	ldur	x0, [x29, #-16]
	add	x1, sp, #32                     // =32
	bl	init_nrc
	cbz	w0, .LBB150_13
// %bb.12:
	ldr	x0, [sp, #40]
	bl	free_nrc
	ldr	x0, [sp, #32]
	bl	free_nrc
	adrp	x0, .L.str.181
	add	x0, x0, :lo12:.L.str.181
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB150_34
.LBB150_13:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	ldr	x3, [sp, #32]
	sub	x2, x29, #48                    // =48
	bl	get_used_page_num_no_crosstalk
	stur	w0, [x29, #-40]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-24]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-32]
	ldrh	w4, [x8]
	ldur	w5, [x29, #-40]
	ldur	x8, [x29, #-32]
	ldr	w6, [x8, #8]
	adrp	x0, .L.str.189
	add	x0, x0, :lo12:.L.str.189
	bl	nand_print
	ldr	x8, [sp, #32]
	ldr	w8, [x8]
	cbnz	w8, .LBB150_15
// %bb.14:
	ldr	x0, [sp, #40]
	bl	free_nrc
	ldr	x0, [sp, #32]
	bl	free_nrc
	adrp	x0, .L.str.190
	add	x0, x0, :lo12:.L.str.190
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	stur	xzr, [x29, #-8]
	b	.LBB150_34
.LBB150_15:
	ldur	w8, [x29, #-40]
	ldur	w9, [x29, #-36]
	subs	w8, w8, w9
	b.ls	.LBB150_17
// %bb.16:
	ldur	x8, [x29, #-32]
	stur	x8, [x29, #-88]
	ldur	x8, [x29, #-24]
	stur	x8, [x29, #-96]
	ldr	x8, [sp, #32]
	str	x8, [sp, #24]
	ldur	w1, [x29, #-36]
	ldur	w2, [x29, #-40]
	ldr	x8, [sp, #24]
	ldr	w3, [x8]
	adrp	x0, .L.str.182
	add	x0, x0, :lo12:.L.str.182
	bl	nand_print
	b	.LBB150_18
.LBB150_17:
	ldur	x8, [x29, #-24]
	stur	x8, [x29, #-88]
	ldur	x8, [x29, #-32]
	stur	x8, [x29, #-96]
	ldr	x8, [sp, #40]
	str	x8, [sp, #24]
	ldur	w1, [x29, #-36]
	ldur	w2, [x29, #-40]
	ldr	x8, [sp, #24]
	ldr	w3, [x8]
	adrp	x0, .L.str.183
	add	x0, x0, :lo12:.L.str.183
	bl	nand_print
.LBB150_18:
	ldur	x8, [x29, #-96]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #72                     // =72
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-68]
	ldur	w8, [x29, #-68]
	cbz	w8, .LBB150_25
// %bb.19:
	ldur	x8, [x29, #-96]
	ldrh	w1, [x8]
	adrp	x0, .L.str.191
	add	x0, x0, :lo12:.L.str.191
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #72                     // =72
	blr	x8
	ldur	x8, [x29, #-88]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-88]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
.LBB150_20:                             // =>This Inner Loop Header: Depth=1
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	stur	x0, [x29, #-96]
	ldur	x8, [x29, #-96]
	cbnz	x8, .LBB150_22
// %bb.21:
	adrp	x0, .L.str.184
	add	x0, x0, :lo12:.L.str.184
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB150_34
.LBB150_22:                             //   in Loop: Header=BB150_20 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-96]
	ldur	x8, [x29, #-96]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB150_24
// %bb.23:                              //   in Loop: Header=BB150_20 Depth=1
	adrp	x0, .L.str.185
	add	x0, x0, :lo12:.L.str.185
	bl	nand_print
	b	.LBB150_20
.LBB150_24:
.LBB150_25:
	ldur	x8, [x29, #-88]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-96]
	str	w8, [x9, #8]
.LBB150_26:
	stur	wzr, [x29, #-52]
	stur	wzr, [x29, #-72]
.LBB150_27:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-52]
	ldr	x9, [sp, #24]
	ldr	w9, [x9]
	subs	w8, w8, w9
	b.hs	.LBB150_33
// %bb.28:                              //   in Loop: Header=BB150_27 Depth=1
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-52]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbz	w8, .LBB150_30
// %bb.29:                              //   in Loop: Header=BB150_27 Depth=1
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #16]
	ldur	w9, [x29, #-52]
	ldur	x10, [x29, #-16]
	ldr	x10, [x10, #8]
	ldrh	w10, [x10, #26]
	add	w10, w10, #64                   // =64
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	str	x8, [sp, #96]
	ldr	x8, [sp, #96]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #104]
	ldur	x8, [x29, #-96]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-72]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #96]
	ldr	x5, [sp, #104]
	add	x0, sp, #48                     // =48
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x0, [sp, #104]
	bl	get_special_data_from_oob
	stur	w0, [x29, #-64]
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #104]
	ldur	w2, [x29, #-64]
	ldur	x8, [x29, #-96]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-96]
	ldrh	w4, [x8, #6]
	bl	set_oob_special_page
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	blr	x8
	ldur	w8, [x29, #-72]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-72]
	b	.LBB150_31
.LBB150_30:                             //   in Loop: Header=BB150_27 Depth=1
	ldur	w1, [x29, #-52]
	ldr	x8, [sp, #24]
	ldr	w2, [x8]
	adrp	x0, .L.str.186
	add	x0, x0, :lo12:.L.str.186
	bl	nand_print
.LBB150_31:                             //   in Loop: Header=BB150_27 Depth=1
// %bb.32:                              //   in Loop: Header=BB150_27 Depth=1
	ldur	w8, [x29, #-52]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-52]
	b	.LBB150_27
.LBB150_33:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-88]
	ldur	x8, [x29, #-88]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldr	x0, [sp, #40]
	bl	free_nrc
	ldr	x0, [sp, #32]
	bl	free_nrc
	ldur	x8, [x29, #-96]
	stur	x8, [x29, #-8]
.LBB150_34:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #208]            // 16-byte Folded Reload
	add	sp, sp, #224                    // =224
	ret
.Lfunc_end150:
	.size	no_cross_talk_rebuild_current_block, .Lfunc_end150-no_cross_talk_rebuild_current_block
	.cfi_endproc
                                        // -- End function
	.globl	get_current_used_block          // -- Begin function get_current_used_block
	.p2align	2
	.type	get_current_used_block,@function
get_current_used_block:                 // @get_current_used_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	x1, [sp, #24]
	str	w2, [sp, #20]
	str	wzr, [sp, #4]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	cbz	x8, .LBB151_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB151_49
.LBB151_2:
	ldr	x8, [sp, #24]
	cbz	x8, .LBB151_37
// %bb.3:
.LBB151_4:                              // =>This Inner Loop Header: Depth=1
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #24]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	bl	recover_phy_page_mapping
	ldur	x8, [x29, #-16]
	strh	w0, [x8, #584]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #584]
	mov	w9, #65535
	subs	w8, w8, w9
	b.ne	.LBB151_8
// %bb.5:                               //   in Loop: Header=BB151_4 Depth=1
	mov	w8, #1
	str	w8, [sp, #4]
	ldr	x8, [sp, #24]
	ldrh	w1, [x8]
	adrp	x0, .L.str.143
	add	x0, x0, :lo12:.L.str.143
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #24]
	bl	current_block_ecc_error
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	cbz	x8, .LBB151_7
// %bb.6:                               //   in Loop: Header=BB151_4 Depth=1
	b	.LBB151_4
.LBB151_7:
	adrp	x0, .L.str.144
	add	x0, x0, :lo12:.L.str.144
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB151_49
.LBB151_8:
	ldr	x8, [sp, #24]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #528]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldrh	w2, [x8, #584]
	adrp	x0, .L.str.145
	add	x0, x0, :lo12:.L.str.145
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #528]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #552]
	ldur	x8, [x29, #-16]
	ldrh	w3, [x8, #584]
	bl	recover_logic_page_mapping
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #584]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #528]
	ldrh	w9, [x9, #4]
	subs	w8, w8, w9
	b.ge	.LBB151_10
// %bb.9:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8, #584]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldrh	w2, [x8, #4]
	adrp	x0, .L.str.146
	add	x0, x0, :lo12:.L.str.146
	bl	nand_print
.LBB151_10:
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB151_36
// %bb.11:
.LBB151_12:                             // =>This Loop Header: Depth=1
                                        //     Child Loop BB151_18 Depth 2
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	cbnz	x8, .LBB151_24
// %bb.13:                              //   in Loop: Header=BB151_12 Depth=1
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	ldur	x8, [x29, #-16]
	str	x0, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	cbnz	x8, .LBB151_15
// %bb.14:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB151_49
.LBB151_15:                             //   in Loop: Header=BB151_12 Depth=1
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #648]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #592]
	str	w8, [x9, #8]
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB151_17
// %bb.16:                              //   in Loop: Header=BB151_12 Depth=1
	adrp	x0, .L.str.147
	add	x0, x0, :lo12:.L.str.147
	bl	nand_print
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #592]
	b	.LBB151_12
.LBB151_17:                             //   in Loop: Header=BB151_12 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #584]
	str	w8, [sp, #16]
	str	wzr, [sp, #12]
.LBB151_18:                             //   Parent Loop BB151_12 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #16]
	subs	w8, w8, w9
	b.hs	.LBB151_23
// %bb.19:                              //   in Loop: Header=BB151_18 Depth=2
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3488]
	ldur	x0, [x29, #-16]
	ldur	x9, [x29, #-16]
	ldr	x1, [x9, #528]
	ldur	x9, [x29, #-16]
	ldr	x2, [x9, #592]
	ldur	x9, [x29, #-16]
	ldr	x3, [x9, #512]
	ldr	w4, [sp, #12]
	blr	x8
	str	w0, [sp, #8]
	ldr	w8, [sp, #8]
	cbz	w8, .LBB151_21
// %bb.20:                              //   in Loop: Header=BB151_18 Depth=2
	adrp	x0, .L.str.148
	add	x0, x0, :lo12:.L.str.148
	bl	nand_print
.LBB151_21:                             //   in Loop: Header=BB151_18 Depth=2
// %bb.22:                              //   in Loop: Header=BB151_18 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB151_18
.LBB151_23:                             //   in Loop: Header=BB151_12 Depth=1
.LBB151_24:                             //   in Loop: Header=BB151_12 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #616]
	bl	recover_phy_page_mapping
	ldur	x8, [x29, #-16]
	strh	w0, [x8, #648]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #648]
	mov	w9, #65535
	subs	w8, w8, w9
	b.ne	.LBB151_26
// %bb.25:                              //   in Loop: Header=BB151_12 Depth=1
	adrp	x0, .L.str.149
	add	x0, x0, :lo12:.L.str.149
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #592]
	b	.LBB151_12
.LBB151_26:                             //   in Loop: Header=BB151_12 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #592]
	str	w8, [x9, #8]
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x2, [x8, #616]
	ldur	x8, [x29, #-16]
	ldrh	w3, [x8, #648]
	bl	recover_logic_page_mapping
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #648]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #584]
	subs	w8, w8, w9
	b.le	.LBB151_31
// %bb.27:                              //   in Loop: Header=BB151_12 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8, #648]
	ldur	x8, [x29, #-16]
	ldrh	w2, [x8, #584]
	adrp	x0, .L.str.150
	add	x0, x0, :lo12:.L.str.150
	bl	nand_print
	ldr	w8, [sp, #4]
	cbz	w8, .LBB151_29
// %bb.28:                              //   in Loop: Header=BB151_12 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #528]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #528]
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #648]
	ldur	x9, [x29, #-16]
	strh	w8, [x9, #584]
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #536]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #600]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #544]
	bl	nand_memcpy
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #592]
	b	.LBB151_12
.LBB151_29:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8, #648]
	ldur	x8, [x29, #-16]
	ldrh	w2, [x8, #584]
	adrp	x0, .L.str.151
	add	x0, x0, :lo12:.L.str.151
	bl	nand_print
// %bb.30:
	b	.LBB151_35
.LBB151_31:                             //   in Loop: Header=BB151_12 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #648]
	ldur	x9, [x29, #-16]
	ldrh	w9, [x9, #584]
	subs	w8, w8, w9
	b.ge	.LBB151_33
// %bb.32:                              //   in Loop: Header=BB151_12 Depth=1
	adrp	x0, .L.str.152
	add	x0, x0, :lo12:.L.str.152
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #592]
	b	.LBB151_12
.LBB151_33:
// %bb.34:
.LBB151_35:
.LBB151_36:
	b	.LBB151_48
.LBB151_37:
	adrp	x0, .L.str.153
	add	x0, x0, :lo12:.L.str.153
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #304]
	cbz	x8, .LBB151_46
// %bb.38:
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	ldur	x8, [x29, #-16]
	str	x0, [x8, #528]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	cbnz	x8, .LBB151_40
// %bb.39:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB151_49
.LBB151_40:
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #584]
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #528]
	str	w8, [x9, #8]
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB151_44
// %bb.41:
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	ldur	x8, [x29, #-16]
	str	x0, [x8, #592]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #592]
	cbnz	x8, .LBB151_43
// %bb.42:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB151_49
.LBB151_43:
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #648]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #592]
	str	w8, [x9, #8]
	b	.LBB151_45
.LBB151_44:
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #592]
.LBB151_45:
	b	.LBB151_47
.LBB151_46:
	adrp	x0, .L.str.154
	add	x0, x0, :lo12:.L.str.154
	bl	nand_print
	ldur	x0, [x29, #-16]
	bl	print_free_list
	ldur	x0, [x29, #-16]
	bl	print_block_invalid_list
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB151_49
.LBB151_47:
.LBB151_48:
	stur	wzr, [x29, #-4]
.LBB151_49:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end151:
	.size	get_current_used_block, .Lfunc_end151-get_current_used_block
	.cfi_endproc
                                        // -- End function
	.globl	check_bit_nums                  // -- Begin function check_bit_nums
	.p2align	2
	.type	check_bit_nums,@function
check_bit_nums:                         // @check_bit_nums
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	w1, [sp, #20]
	str	wzr, [sp, #8]
	str	wzr, [sp, #16]
.LBB152_1:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB152_3 Depth 2
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.ge	.LBB152_10
// %bb.2:                               //   in Loop: Header=BB152_1 Depth=1
	mov	w8, #1
	strb	w8, [sp, #7]
	str	wzr, [sp, #12]
.LBB152_3:                              //   Parent Loop BB152_1 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #8                      // =8
	b.ge	.LBB152_8
// %bb.4:                               //   in Loop: Header=BB152_3 Depth=2
	ldrb	w8, [sp, #7]
	ldr	x9, [sp, #24]
	ldrsw	x10, [sp, #16]
	ldrb	w9, [x9, x10]
	and	w8, w8, w9
	cbz	w8, .LBB152_6
// %bb.5:                               //   in Loop: Header=BB152_3 Depth=2
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
.LBB152_6:                              //   in Loop: Header=BB152_3 Depth=2
	ldrb	w8, [sp, #7]
	lsl	w8, w8, #1
	strb	w8, [sp, #7]
// %bb.7:                               //   in Loop: Header=BB152_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB152_3
.LBB152_8:                              //   in Loop: Header=BB152_1 Depth=1
// %bb.9:                               //   in Loop: Header=BB152_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB152_1
.LBB152_10:
	ldr	w0, [sp, #8]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end152:
	.size	check_bit_nums, .Lfunc_end152-check_bit_nums
	.cfi_endproc
                                        // -- End function
	.globl	is_all_bit_ff                   // -- Begin function is_all_bit_ff
	.p2align	2
	.type	is_all_bit_ff,@function
is_all_bit_ff:                          // @is_all_bit_ff
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	str	w2, [sp, #8]
	ldr	w8, [sp, #12]
	lsl	w8, w8, #3
	str	w8, [sp]
	ldr	w9, [sp, #8]
	ldr	w8, [sp]
	subs	w8, w8, w9
	str	w8, [sp]
	ldr	x0, [sp, #16]
	ldr	w1, [sp, #12]
	bl	check_bit_nums
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	ldr	w9, [sp]
	subs	w8, w8, w9
	b.lt	.LBB153_2
// %bb.1:
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB153_3
.LBB153_2:
	stur	wzr, [x29, #-4]
.LBB153_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end153:
	.size	is_all_bit_ff, .Lfunc_end153-is_all_bit_ff
	.cfi_endproc
                                        // -- End function
	.globl	is_all_byte_ff                  // -- Begin function is_all_byte_ff
	.p2align	2
	.type	is_all_byte_ff,@function
is_all_byte_ff:                         // @is_all_byte_ff
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #32]
	str	w1, [sp, #28]
	str	w2, [sp, #24]
	str	wzr, [sp, #20]
	ldr	w8, [sp, #28]
	ldr	w9, [sp, #24]
	subs	w8, w8, w9
	str	w8, [sp, #16]
	str	wzr, [sp, #12]
.LBB154_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #28]
	subs	w8, w8, w9
	b.ge	.LBB154_6
// %bb.2:                               //   in Loop: Header=BB154_1 Depth=1
	ldr	x8, [sp, #32]
	ldrsw	x9, [sp, #12]
	ldrb	w8, [x8, x9]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB154_4
// %bb.3:                               //   in Loop: Header=BB154_1 Depth=1
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #20]
.LBB154_4:                              //   in Loop: Header=BB154_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB154_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB154_1
.LBB154_6:
	ldr	w8, [sp, #20]
	ldr	w9, [sp, #16]
	subs	w8, w8, w9
	b.lt	.LBB154_8
// %bb.7:
	mov	w8, #1
	str	w8, [sp, #44]
	b	.LBB154_9
.LBB154_8:
	str	wzr, [sp, #44]
.LBB154_9:
	ldr	w0, [sp, #44]
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end154:
	.size	is_all_byte_ff, .Lfunc_end154-is_all_byte_ff
	.cfi_endproc
                                        // -- End function
	.globl	current_block_ecc_error         // -- Begin function current_block_ecc_error
	.p2align	2
	.type	current_block_ecc_error,@function
current_block_ecc_error:                // @current_block_ecc_error
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160                    // =160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144                   // =144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
.LBB155_1:                              // =>This Inner Loop Header: Depth=1
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	stur	x0, [x29, #-48]
	ldur	x8, [x29, #-48]
	cbnz	x8, .LBB155_3
// %bb.2:
	adrp	x0, .L.str.206
	add	x0, x0, :lo12:.L.str.206
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB155_29
.LBB155_3:                              //   in Loop: Header=BB155_1 Depth=1
	ldur	x8, [x29, #-24]
	ldr	w8, [x8, #8]
	ldur	x9, [x29, #-48]
	str	w8, [x9, #8]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-48]
	ldur	x8, [x29, #-48]
	ldr	w2, [x8, #8]
	bl	new_block_init_for_write
	cbz	w0, .LBB155_5
// %bb.4:                               //   in Loop: Header=BB155_1 Depth=1
	adrp	x0, .L.str.152
	add	x0, x0, :lo12:.L.str.152
	bl	nand_print
	b	.LBB155_1
.LBB155_5:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	adrp	x0, .L.str.207
	add	x0, x0, :lo12:.L.str.207
	bl	nand_print
	stur	wzr, [x29, #-28]
	stur	wzr, [x29, #-32]
.LBB155_6:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-28]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ge	.LBB155_25
// %bb.7:                               //   in Loop: Header=BB155_6 Depth=1
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-28]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	sub	x5, x29, #64                    // =64
	str	x5, [sp, #24]                   // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-16]
	blr	x8
	mov	w8, w0
	ldr	x0, [sp, #24]                   // 8-byte Folded Reload
	stur	w8, [x29, #-36]
	bl	is_ftl_logic_page_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB155_16
// %bb.8:                               //   in Loop: Header=BB155_6 Depth=1
	ldur	w8, [x29, #-36]
	cbz	w8, .LBB155_10
// %bb.9:                               //   in Loop: Header=BB155_6 Depth=1
	ldur	w8, [x29, #-36]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB155_14
.LBB155_10:                             //   in Loop: Header=BB155_6 Depth=1
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #512]
	sub	x2, x29, #64                    // =64
	bl	nand_power_off_ecc_error
	cbz	w0, .LBB155_12
// %bb.11:                              //   in Loop: Header=BB155_6 Depth=1
	adrp	x0, .L.str.208
	add	x0, x0, :lo12:.L.str.208
	bl	nand_print
	b	.LBB155_13
.LBB155_12:                             //   in Loop: Header=BB155_6 Depth=1
	ldur	w1, [x29, #-28]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.209
	add	x0, x0, :lo12:.L.str.209
	bl	nand_print
	ldur	x8, [x29, #-48]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-16]
	ldr	x4, [x8, #512]
	add	x0, sp, #32                     // =32
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	sub	x5, x29, #64                    // =64
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-16]
	blr	x8
	ldur	w8, [x29, #-32]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-32]
.LBB155_13:                             //   in Loop: Header=BB155_6 Depth=1
	b	.LBB155_15
.LBB155_14:                             //   in Loop: Header=BB155_6 Depth=1
	ldur	w1, [x29, #-28]
	adrp	x0, .L.str.210
	add	x0, x0, :lo12:.L.str.210
	bl	nand_print
.LBB155_15:                             //   in Loop: Header=BB155_6 Depth=1
	b	.LBB155_23
.LBB155_16:                             //   in Loop: Header=BB155_6 Depth=1
	sub	x0, x29, #64                    // =64
	bl	is_ftl_special_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB155_18
// %bb.17:                              //   in Loop: Header=BB155_6 Depth=1
	ldur	w1, [x29, #-28]
	adrp	x0, .L.str.211
	add	x0, x0, :lo12:.L.str.211
	bl	nand_print
	b	.LBB155_22
.LBB155_18:                             //   in Loop: Header=BB155_6 Depth=1
	sub	x0, x29, #64                    // =64
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB155_20
// %bb.19:                              //   in Loop: Header=BB155_6 Depth=1
	b	.LBB155_21
.LBB155_20:                             //   in Loop: Header=BB155_6 Depth=1
	ldur	w1, [x29, #-28]
	adrp	x0, .L.str.212
	add	x0, x0, :lo12:.L.str.212
	bl	nand_print
.LBB155_21:                             //   in Loop: Header=BB155_6 Depth=1
.LBB155_22:                             //   in Loop: Header=BB155_6 Depth=1
.LBB155_23:                             //   in Loop: Header=BB155_6 Depth=1
// %bb.24:                              //   in Loop: Header=BB155_6 Depth=1
	ldur	w8, [x29, #-28]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-28]
	b	.LBB155_6
.LBB155_25:
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8, #25]
	tbz	w8, #0, .LBB155_27
// %bb.26:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	b	.LBB155_28
.LBB155_27:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
.LBB155_28:
	ldur	w1, [x29, #-28]
	ldur	w2, [x29, #-32]
	adrp	x0, .L.str.213
	add	x0, x0, :lo12:.L.str.213
	bl	nand_print_dbg
	ldur	x8, [x29, #-48]
	stur	x8, [x29, #-8]
.LBB155_29:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160                    // =160
	ret
.Lfunc_end155:
	.size	current_block_ecc_error, .Lfunc_end155-current_block_ecc_error
	.cfi_endproc
                                        // -- End function
	.globl	nand_power_off_ecc_error        // -- Begin function nand_power_off_ecc_error
	.p2align	2
	.type	nand_power_off_ecc_error,@function
nand_power_off_ecc_error:               // @nand_power_off_ecc_error
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	x2, [sp, #8]
	str	wzr, [sp, #4]
	ldr	x8, [sp, #8]
	ldrb	w8, [x8]
	subs	w8, w8, #255                    // =255
	b.eq	.LBB156_2
// %bb.1:
	adrp	x0, .L.str.287
	add	x0, x0, :lo12:.L.str.287
	bl	nand_print
	mov	w8, #1
	str	w8, [sp, #4]
	b	.LBB156_11
.LBB156_2:
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #28]
	cbz	w8, .LBB156_6
// %bb.3:
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #16]
	ldr	x2, [sp, #8]
	bl	crc_wrong
	cbz	w0, .LBB156_5
// %bb.4:
	adrp	x0, .L.str.288
	add	x0, x0, :lo12:.L.str.288
	bl	nand_print
	mov	w8, #1
	str	w8, [sp, #4]
	b	.LBB156_11
.LBB156_5:
	b	.LBB156_10
.LBB156_6:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #11]
	subs	w8, w8, #165                    // =165
	b.ne	.LBB156_8
// %bb.7:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #15]
	subs	w8, w8, #165                    // =165
	b.eq	.LBB156_9
.LBB156_8:
	adrp	x0, .L.str.289
	add	x0, x0, :lo12:.L.str.289
	bl	nand_print
	mov	w8, #1
	str	w8, [sp, #4]
	b	.LBB156_11
.LBB156_9:
.LBB156_10:
.LBB156_11:
	ldr	w8, [sp, #4]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB156_13
// %bb.12:
	ldr	w3, [sp, #4]
	adrp	x0, .L.str.290
	add	x0, x0, :lo12:.L.str.290
	adrp	x1, .L__FUNCTION__.nand_power_off_ecc_error
	add	x1, x1, :lo12:.L__FUNCTION__.nand_power_off_ecc_error
	mov	w2, #3128
	bl	nand_print
	adrp	x0, .L.str.291
	add	x0, x0, :lo12:.L.str.291
	bl	nand_print
	ldr	x0, [sp, #8]
	mov	w1, #16
	bl	_DumpHexData
.LBB156_13:
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end156:
	.size	nand_power_off_ecc_error, .Lfunc_end156-nand_power_off_ecc_error
	.cfi_endproc
                                        // -- End function
	.globl	do_write_error_in_build_list    // -- Begin function do_write_error_in_build_list
	.p2align	2
	.type	do_write_error_in_build_list,@function
do_write_error_in_build_list:           // @do_write_error_in_build_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	sturh	w3, [x29, #-34]
	ldur	x8, [x29, #-32]
	str	x8, [sp, #40]
	ldr	x8, [sp, #40]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #16                     // =16
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-40]
	ldr	x8, [sp, #40]
	strh	wzr, [x8, #4]
	ldr	x9, [sp, #40]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB157_2
// %bb.1:
	adrp	x0, .L.str.170
	add	x0, x0, :lo12:.L.str.170
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #16                     // =16
	blr	x8
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB157_11
.LBB157_2:
	stur	wzr, [x29, #-44]
.LBB157_3:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-44]
	ldurh	w9, [x29, #-34]
	subs	w8, w8, w9
	b.hs	.LBB157_8
// %bb.4:                               //   in Loop: Header=BB157_3 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3488]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-24]
	ldr	x2, [sp, #40]
	ldur	x9, [x29, #-16]
	ldr	x3, [x9, #512]
	ldur	w4, [x29, #-44]
	blr	x8
	stur	w0, [x29, #-40]
	ldur	w8, [x29, #-40]
	cbz	w8, .LBB157_6
// %bb.5:                               //   in Loop: Header=BB157_3 Depth=1
	adrp	x0, .L.str.171
	add	x0, x0, :lo12:.L.str.171
	bl	nand_print
.LBB157_6:                              //   in Loop: Header=BB157_3 Depth=1
// %bb.7:                               //   in Loop: Header=BB157_3 Depth=1
	ldur	w8, [x29, #-44]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-44]
	b	.LBB157_3
.LBB157_8:
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #16                     // =16
	str	x0, [sp]                        // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	blr	x8
	ldurh	w8, [x29, #-34]
	cbnz	w8, .LBB157_10
// %bb.9:
	ldur	x8, [x29, #-32]
	ldur	x9, [x29, #-16]
	str	x8, [x9, #528]
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #584]
.LBB157_10:
	stur	wzr, [x29, #-4]
.LBB157_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end157:
	.size	do_write_error_in_build_list, .Lfunc_end157-do_write_error_in_build_list
	.cfi_endproc
                                        // -- End function
	.globl	init_nrc                        // -- Begin function init_nrc
	.p2align	2
	.type	init_nrc,@function
init_nrc:                               // @init_nrc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	x1, [sp, #24]
	str	wzr, [sp, #20]
	str	xzr, [sp, #8]
	mov	w0, #32
	bl	nftl_malloc
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB158_2
// %bb.1:
	adrp	x0, .L.str.172
	add	x0, x0, :lo12:.L.str.172
	adrp	x1, .L__func__.init_nrc
	add	x1, x1, :lo12:.L__func__.init_nrc
	mov	w2, #1420
	bl	nand_print
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB158_16
.LBB158_2:
	ldr	x0, [sp, #8]
	mov	w1, wzr
	mov	w2, #32
	bl	nand_memset
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	str	w8, [sp, #20]
	ldr	w8, [sp, #20]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	w9, w9, #64                     // =64
	mul	w0, w8, w9
	bl	nftl_malloc
	ldr	x8, [sp, #8]
	str	x0, [x8, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbnz	x8, .LBB158_4
// %bb.3:
	adrp	x0, .L.str.173
	add	x0, x0, :lo12:.L.str.173
	adrp	x1, .L__func__.init_nrc
	add	x1, x1, :lo12:.L__func__.init_nrc
	mov	w2, #1429
	bl	nand_print
	b	.LBB158_9
.LBB158_4:
	ldrsw	x8, [sp, #20]
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w0, w8, #2
	bl	nftl_malloc
	ldr	x8, [sp, #8]
	str	x0, [x8, #24]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	cbnz	x8, .LBB158_6
// %bb.5:
	adrp	x0, .L.str.174
	add	x0, x0, :lo12:.L.str.174
	adrp	x1, .L__func__.init_nrc
	add	x1, x1, :lo12:.L__func__.init_nrc
	mov	w2, #1434
	bl	nand_print
	b	.LBB158_9
.LBB158_6:
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #24]
	ldrsw	x8, [sp, #20]
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w2, w8, #2
	mov	w1, #-1
	bl	nand_memset
	ldr	w0, [sp, #20]
	bl	nftl_malloc
	ldr	x8, [sp, #8]
	str	x0, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	cbnz	x8, .LBB158_8
// %bb.7:
	adrp	x0, .L.str.175
	add	x0, x0, :lo12:.L.str.175
	adrp	x1, .L__func__.init_nrc
	add	x1, x1, :lo12:.L__func__.init_nrc
	mov	w2, #1440
	bl	nand_print
	b	.LBB158_9
.LBB158_8:
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #8]
	ldr	w2, [sp, #20]
	mov	w1, wzr
	bl	nand_memset
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #24]
	str	x8, [x9]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #20]
	ldr	x2, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x3, [x8, #16]
	ldr	x8, [sp, #8]
	ldr	x4, [x8, #24]
	ldr	x8, [sp, #8]
	ldr	x5, [x8, #8]
	adrp	x0, .L.str.176
	add	x0, x0, :lo12:.L.str.176
	bl	nand_print
	stur	wzr, [x29, #-4]
	b	.LBB158_16
.LBB158_9:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB158_11
// %bb.10:
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #16]
	bl	nftl_free
.LBB158_11:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	cbz	x8, .LBB158_13
// %bb.12:
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #24]
	bl	nftl_free
.LBB158_13:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	cbz	x8, .LBB158_15
// %bb.14:
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #8]
	bl	nftl_free
.LBB158_15:
	ldr	x0, [sp, #8]
	bl	nftl_free
	ldr	x8, [sp, #24]
	str	xzr, [x8]
	mov	w8, #-1
	stur	w8, [x29, #-4]
.LBB158_16:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end158:
	.size	init_nrc, .Lfunc_end158-init_nrc
	.cfi_endproc
                                        // -- End function
	.globl	free_nrc                        // -- Begin function free_nrc
	.p2align	2
	.type	free_nrc,@function
free_nrc:                               // @free_nrc
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB159_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB159_3
.LBB159_2:
	ldr	x8, [sp]
	ldr	x0, [x8, #16]
	bl	nftl_free
	ldr	x8, [sp]
	ldr	x0, [x8, #8]
	bl	nftl_free
	ldr	x8, [sp]
	ldr	x0, [x8, #24]
	bl	nftl_free
	ldr	x0, [sp]
	bl	nftl_free
	stur	wzr, [x29, #-4]
.LBB159_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end159:
	.size	free_nrc, .Lfunc_end159-free_nrc
	.cfi_endproc
                                        // -- End function
	.globl	get_used_page_num               // -- Begin function get_used_page_num
	.p2align	2
	.type	get_used_page_num,@function
get_used_page_num:                      // @get_used_page_num
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #240                    // =240
	stp	x29, x30, [sp, #208]            // 16-byte Folded Spill
	str	x19, [sp, #224]                 // 8-byte Folded Spill
	add	x29, sp, #208                   // =208
	.cfi_def_cfa w29, 32
	.cfi_offset w19, -16
	.cfi_offset w30, -24
	.cfi_offset w29, -32
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	x2, [x29, #-24]
	stur	x3, [x29, #-32]
	ldur	x8, [x29, #-24]
	str	wzr, [x8]
	stur	wzr, [x29, #-40]
	stur	wzr, [x29, #-36]
.LBB160_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-36]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.hs	.LBB160_27
// %bb.2:                               //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-32]
	cbz	x8, .LBB160_5
// %bb.3:                               //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbnz	w8, .LBB160_5
// %bb.4:                               //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #16]
	ldur	w9, [x29, #-36]
	ldur	x10, [x29, #-8]
	ldr	x10, [x10, #8]
	ldrh	w10, [x10, #26]
	add	w10, w10, #64                   // =64
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	stur	x8, [x29, #-96]
	ldur	x8, [x29, #-96]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #104]
	b	.LBB160_8
.LBB160_5:                              //   in Loop: Header=BB160_1 Depth=1
	stur	xzr, [x29, #-96]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB160_7
// %bb.6:                               //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #512]
	stur	x8, [x29, #-96]
.LBB160_7:                              //   in Loop: Header=BB160_1 Depth=1
	sub	x8, x29, #64                    // =64
	str	x8, [sp, #104]
.LBB160_8:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #28]
	cbz	w8, .LBB160_10
// %bb.9:                               //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x4, [x29, #-96]
	ldr	x5, [sp, #104]
	sub	x0, x29, #88                    // =88
	bl	set_physic_op_par
	b	.LBB160_11
.LBB160_10:                             //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldr	x5, [sp, #104]
	sub	x0, x29, #88                    // =88
	mov	w3, wzr
	mov	x4, xzr
	bl	set_physic_op_par
.LBB160_11:                             //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	sub	x1, x29, #88                    // =88
	blr	x8
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-48]
	cbz	w8, .LBB160_14
// %bb.12:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	w8, [x29, #-48]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB160_14
// %bb.13:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.200
	add	x0, x0, :lo12:.L.str.200
	bl	nand_print
	ldur	x9, [x29, #-24]
	ldr	w8, [x9]
	add	w8, w8, #1                      // =1
	str	w8, [x9]
.LBB160_14:                             //   in Loop: Header=BB160_1 Depth=1
	ldr	x0, [sp, #104]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB160_16
// %bb.15:
	b	.LBB160_27
.LBB160_16:                             //   in Loop: Header=BB160_1 Depth=1
	ldur	w8, [x29, #-48]
	cbz	w8, .LBB160_18
// %bb.17:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	w8, [x29, #-48]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB160_25
.LBB160_18:                             //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-96]
	cbz	x8, .LBB160_25
// %bb.19:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	x0, [x29, #-8]
	ldur	x1, [x29, #-96]
	ldr	x2, [sp, #104]
	bl	nand_power_off_ecc_error
	cbz	w0, .LBB160_21
// %bb.20:                              //   in Loop: Header=BB160_1 Depth=1
	adrp	x0, .L.str.201
	add	x0, x0, :lo12:.L.str.201
	bl	nand_print
	b	.LBB160_24
.LBB160_21:                             //   in Loop: Header=BB160_1 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9
	mov	w8, #1
	strb	w8, [x9]
	ldur	x8, [x29, #-32]
	ldr	w8, [x8]
	ldur	w9, [x29, #-36]
	add	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB160_23
// %bb.22:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-32]
	str	w8, [x9]
.LBB160_23:                             //   in Loop: Header=BB160_1 Depth=1
.LBB160_24:                             //   in Loop: Header=BB160_1 Depth=1
.LBB160_25:                             //   in Loop: Header=BB160_1 Depth=1
// %bb.26:                              //   in Loop: Header=BB160_1 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB160_1
.LBB160_27:
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-44]
.LBB160_28:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-36]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.hs	.LBB160_47
// %bb.29:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-32]
	cbz	x8, .LBB160_32
// %bb.30:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbnz	w8, .LBB160_32
// %bb.31:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #16]
	ldur	w9, [x29, #-36]
	ldur	x10, [x29, #-8]
	ldr	x10, [x10, #8]
	ldrh	w10, [x10, #26]
	add	w10, w10, #64                   // =64
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	stur	x8, [x29, #-96]
	ldur	x8, [x29, #-96]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #104]
	b	.LBB160_33
.LBB160_32:                             //   in Loop: Header=BB160_28 Depth=1
	stur	xzr, [x29, #-96]
	sub	x8, x29, #64                    // =64
	str	x8, [sp, #104]
.LBB160_33:                             //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #28]
	cbz	w8, .LBB160_35
// %bb.34:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x4, [x29, #-96]
	ldr	x5, [sp, #104]
	sub	x0, x29, #88                    // =88
	bl	set_physic_op_par
	b	.LBB160_36
.LBB160_35:                             //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldr	x5, [sp, #104]
	sub	x0, x29, #88                    // =88
	mov	w3, wzr
	mov	x4, xzr
	bl	set_physic_op_par
.LBB160_36:                             //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	sub	x1, x29, #88                    // =88
	blr	x8
	stur	w0, [x29, #-48]
	ldr	x0, [sp, #104]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB160_38
// %bb.37:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	w8, [x29, #-40]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-40]
	b	.LBB160_45
.LBB160_38:                             //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldr	x8, [sp, #104]
	ldrb	w3, [x8]
	ldr	x8, [sp, #104]
	ldrb	w4, [x8, #1]
	ldr	x8, [sp, #104]
	ldrb	w5, [x8, #2]
	ldr	x8, [sp, #104]
	ldrb	w6, [x8, #3]
	ldr	x8, [sp, #104]
	ldrb	w7, [x8, #4]
	ldr	x8, [sp, #104]
	ldrb	w19, [x8, #5]
	ldr	x8, [sp, #104]
	ldrb	w18, [x8, #6]
	ldr	x8, [sp, #104]
	ldrb	w17, [x8, #7]
	ldr	x8, [sp, #104]
	ldrb	w16, [x8, #8]
	ldr	x8, [sp, #104]
	ldrb	w15, [x8, #9]
	ldr	x8, [sp, #104]
	ldrb	w14, [x8, #10]
	ldr	x8, [sp, #104]
	ldrb	w13, [x8, #11]
	ldr	x8, [sp, #104]
	ldrb	w12, [x8, #12]
	ldr	x8, [sp, #104]
	ldrb	w11, [x8, #13]
	ldr	x8, [sp, #104]
	ldrb	w10, [x8, #14]
	ldr	x8, [sp, #104]
	ldrb	w8, [x8, #15]
	adrp	x0, .L.str.202
	add	x0, x0, :lo12:.L.str.202
	mov	x9, sp
	str	x9, [sp, #96]                   // 8-byte Folded Spill
	str	w19, [x9]
	str	w18, [x9, #8]
	str	w17, [x9, #16]
	str	w16, [x9, #24]
	str	w15, [x9, #32]
	str	w14, [x9, #40]
	str	w13, [x9, #48]
	str	w12, [x9, #56]
	str	w11, [x9, #64]
	str	w10, [x9, #72]
	str	w8, [x9, #80]
	bl	nand_print
	ldur	w8, [x29, #-48]
	cbz	w8, .LBB160_40
// %bb.39:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	w8, [x29, #-48]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB160_44
.LBB160_40:                             //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-96]
	cbz	x8, .LBB160_44
// %bb.41:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9
	mov	w8, #1
	strb	w8, [x9]
	ldur	x8, [x29, #-32]
	ldr	w8, [x8]
	ldur	w9, [x29, #-36]
	add	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB160_43
// %bb.42:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-32]
	str	w8, [x9]
.LBB160_43:                             //   in Loop: Header=BB160_28 Depth=1
.LBB160_44:                             //   in Loop: Header=BB160_28 Depth=1
.LBB160_45:                             //   in Loop: Header=BB160_28 Depth=1
// %bb.46:                              //   in Loop: Header=BB160_28 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB160_28
.LBB160_47:
	ldur	w0, [x29, #-44]
	ldr	x19, [sp, #224]                 // 8-byte Folded Reload
	ldp	x29, x30, [sp, #208]            // 16-byte Folded Reload
	add	sp, sp, #240                    // =240
	ret
.Lfunc_end160:
	.size	get_used_page_num, .Lfunc_end160-get_used_page_num
	.cfi_endproc
                                        // -- End function
	.globl	get_used_page_num_no_crosstalk  // -- Begin function get_used_page_num_no_crosstalk
	.p2align	2
	.type	get_used_page_num_no_crosstalk,@function
get_used_page_num_no_crosstalk:         // @get_used_page_num_no_crosstalk
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #144                    // =144
	stp	x29, x30, [sp, #128]            // 16-byte Folded Spill
	add	x29, sp, #128                   // =128
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	x2, [x29, #-24]
	stur	x3, [x29, #-32]
	mov	w8, #-1
	str	w8, [sp, #20]
	str	wzr, [sp, #16]
	str	wzr, [sp, #12]
	ldur	x8, [x29, #-24]
	str	wzr, [x8]
	stur	wzr, [x29, #-36]
.LBB161_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-36]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.hs	.LBB161_30
// %bb.2:                               //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-32]
	cbz	x8, .LBB161_5
// %bb.3:                               //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbnz	w8, .LBB161_5
// %bb.4:                               //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #16]
	ldur	w9, [x29, #-36]
	ldur	x10, [x29, #-8]
	ldr	x10, [x10, #8]
	ldrh	w10, [x10, #26]
	add	w10, w10, #64                   // =64
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	str	x8, [sp, #32]
	ldr	x8, [sp, #32]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #24]
	b	.LBB161_8
.LBB161_5:                              //   in Loop: Header=BB161_1 Depth=1
	str	xzr, [sp, #32]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	subs	w8, w8, #4                      // =4
	b.ne	.LBB161_7
// %bb.6:                               //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #512]
	str	x8, [sp, #32]
.LBB161_7:                              //   in Loop: Header=BB161_1 Depth=1
	add	x8, sp, #64                     // =64
	str	x8, [sp, #24]
.LBB161_8:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #32]
	ldr	x5, [sp, #24]
	add	x0, sp, #40                     // =40
	str	x0, [sp]                        // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	blr	x8
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-48]
	cbz	w8, .LBB161_13
// %bb.9:                               //   in Loop: Header=BB161_1 Depth=1
	ldur	w8, [x29, #-48]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB161_13
// %bb.10:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-36]
	adrp	x0, .L.str.203
	add	x0, x0, :lo12:.L.str.203
	bl	nand_print
	ldr	w8, [sp, #20]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB161_12
// %bb.11:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	w8, [x29, #-36]
	str	w8, [sp, #20]
.LBB161_12:                             //   in Loop: Header=BB161_1 Depth=1
	b	.LBB161_28
.LBB161_13:                             //   in Loop: Header=BB161_1 Depth=1
	ldr	x0, [sp, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB161_19
// %bb.14:                              //   in Loop: Header=BB161_1 Depth=1
	ldr	w8, [sp, #20]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB161_16
// %bb.15:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	w8, [x29, #-36]
	str	w8, [sp, #20]
.LBB161_16:                             //   in Loop: Header=BB161_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	ldr	w8, [sp, #12]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB161_18
// %bb.17:
	b	.LBB161_30
.LBB161_18:                             //   in Loop: Header=BB161_1 Depth=1
	b	.LBB161_27
.LBB161_19:                             //   in Loop: Header=BB161_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #32]
	ldr	x2, [sp, #24]
	bl	nand_power_off_ecc_error
	cbz	w0, .LBB161_21
// %bb.20:                              //   in Loop: Header=BB161_1 Depth=1
	adrp	x0, .L.str.204
	add	x0, x0, :lo12:.L.str.204
	bl	nand_print
	b	.LBB161_26
.LBB161_21:                             //   in Loop: Header=BB161_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	ldr	w8, [sp, #20]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB161_25
// %bb.22:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9
	mov	w8, #1
	strb	w8, [x9]
	ldr	x0, [sp, #24]
	bl	get_logic_page_from_oob
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #24]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	str	w0, [x8, x9, lsl #2]
	ldur	x8, [x29, #-32]
	ldr	w8, [x8]
	ldur	w9, [x29, #-36]
	add	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB161_24
// %bb.23:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-32]
	str	w8, [x9]
.LBB161_24:                             //   in Loop: Header=BB161_1 Depth=1
.LBB161_25:                             //   in Loop: Header=BB161_1 Depth=1
.LBB161_26:                             //   in Loop: Header=BB161_1 Depth=1
.LBB161_27:                             //   in Loop: Header=BB161_1 Depth=1
.LBB161_28:                             //   in Loop: Header=BB161_1 Depth=1
// %bb.29:                              //   in Loop: Header=BB161_1 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB161_1
.LBB161_30:
	ldr	w8, [sp, #20]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB161_32
// %bb.31:
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	str	w8, [sp, #20]
	b	.LBB161_41
.LBB161_32:
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.eq	.LBB161_40
// %bb.33:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldr	w2, [sp, #16]
	ldr	w3, [sp, #20]
	adrp	x0, .L.str.205
	add	x0, x0, :lo12:.L.str.205
	bl	nand_print
	ldur	x8, [x29, #-8]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #168]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #168]
	stur	wzr, [x29, #-36]
.LBB161_34:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-36]
	ldur	x9, [x29, #-32]
	ldr	w9, [x9]
	subs	w8, w8, w9
	b.hs	.LBB161_39
// %bb.35:                              //   in Loop: Header=BB161_34 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #8]
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbz	w8, .LBB161_37
// %bb.36:                              //   in Loop: Header=BB161_34 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #16]
	ldur	w9, [x29, #-36]
	ldur	x10, [x29, #-8]
	ldr	x10, [x10, #8]
	ldrh	w10, [x10, #26]
	add	w10, w10, #64                   // =64
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #26]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #24]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3440]
	ldr	w8, [x8, #168]
	ldr	x9, [sp, #24]
	strb	w8, [x9, #14]
.LBB161_37:                             //   in Loop: Header=BB161_34 Depth=1
// %bb.38:                              //   in Loop: Header=BB161_34 Depth=1
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB161_34
.LBB161_39:
.LBB161_40:
.LBB161_41:
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-44]
	ldur	w0, [x29, #-44]
	ldp	x29, x30, [sp, #128]            // 16-byte Folded Reload
	add	sp, sp, #144                    // =144
	ret
.Lfunc_end161:
	.size	get_used_page_num_no_crosstalk, .Lfunc_end161-get_used_page_num_no_crosstalk
	.cfi_endproc
                                        // -- End function
	.globl	mark_free_block                 // -- Begin function mark_free_block
	.p2align	2
	.type	mark_free_block,@function
mark_free_block:                        // @mark_free_block
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	add	x0, sp, #40                     // =40
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	mov	w1, #-91
	mov	w2, #16
	bl	nand_memset
	ldr	x5, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	sub	x0, x29, #40                    // =40
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	mov	w2, wzr
	str	w2, [sp, #36]                   // 4-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-16]
	ldrh	w4, [x8, #6]
	mov	w2, #1431655765
	str	w2, [sp, #20]                   // 4-byte Folded Spill
	bl	set_oob_special_page
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	blr	x8
	ldr	x5, [sp, #8]                    // 8-byte Folded Reload
	ldr	x0, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	mov	w2, #1
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldr	w2, [sp, #20]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-16]
	ldrh	w4, [x8, #6]
	bl	set_oob_special_page
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	blr	x8
	ldr	x5, [sp, #8]                    // 8-byte Folded Reload
	ldr	x0, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w2, w8, #2                      // =2
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldr	w2, [sp, #20]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-16]
	ldrh	w4, [x8, #6]
	bl	set_oob_special_page
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	blr	x8
	ldr	x5, [sp, #8]                    // 8-byte Folded Reload
	ldr	x0, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #20]
	subs	w2, w8, #1                      // =1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldr	w2, [sp, #20]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #8]
	ldur	x8, [x29, #-16]
	ldrh	w4, [x8, #6]
	bl	set_oob_special_page
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	blr	x8
	ldr	w0, [sp, #36]                   // 4-byte Folded Reload
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end162:
	.size	mark_free_block, .Lfunc_end162-mark_free_block
	.cfi_endproc
                                        // -- End function
	.globl	nftl_check_pattern              // -- Begin function nftl_check_pattern
	.p2align	2
	.type	nftl_check_pattern,@function
nftl_check_pattern:                     // @nftl_check_pattern
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	strb	w1, [sp, #15]
	str	w2, [sp, #8]
	str	wzr, [sp, #4]
.LBB163_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	ldr	w9, [sp, #8]
	subs	w8, w8, w9
	b.ge	.LBB163_6
// %bb.2:                               //   in Loop: Header=BB163_1 Depth=1
	ldr	x8, [sp, #16]
	ldrsw	x9, [sp, #4]
	ldrb	w8, [x8, x9]
	ldrb	w9, [sp, #15]
	subs	w8, w8, w9
	b.eq	.LBB163_4
// %bb.3:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB163_7
.LBB163_4:                              //   in Loop: Header=BB163_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB163_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB163_1
.LBB163_6:
	str	wzr, [sp, #28]
.LBB163_7:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end163:
	.size	nftl_check_pattern, .Lfunc_end163-nftl_check_pattern
	.cfi_endproc
                                        // -- End function
	.globl	is_phy_block_valid              // -- Begin function is_phy_block_valid
	.p2align	2
	.type	is_phy_block_valid,@function
is_phy_block_valid:                     // @is_phy_block_valid
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	x1, [sp, #8]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.ge	.LBB164_2
// %bb.1:
	mov	w8, #1
	str	w8, [sp, #28]
	b	.LBB164_3
.LBB164_2:
	str	wzr, [sp, #28]
.LBB164_3:
	ldr	w0, [sp, #28]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end164:
	.size	is_phy_block_valid, .Lfunc_end164-is_phy_block_valid
	.cfi_endproc
                                        // -- End function
	.globl	check_cross_talk                // -- Begin function check_cross_talk
	.p2align	2
	.type	check_cross_talk,@function
check_cross_talk:                       // @check_cross_talk
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #512]
	str	x8, [sp, #24]
	stur	wzr, [x29, #-20]
.LBB165_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.hs	.LBB165_9
// %bb.2:                               //   in Loop: Header=BB165_1 Depth=1
	str	xzr, [sp, #24]
	sub	x8, x29, #40                    // =40
	str	x8, [sp, #16]
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-20]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #24]
	ldr	x5, [sp, #16]
	add	x0, sp, #32                     // =32
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbnz	w8, .LBB165_7
// %bb.3:                               //   in Loop: Header=BB165_1 Depth=1
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #14]
	subs	w8, w8, #100                    // =100
	b.ge	.LBB165_6
// %bb.4:                               //   in Loop: Header=BB165_1 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3440]
	ldr	w8, [x8, #168]
	ldr	x9, [sp, #16]
	ldrb	w9, [x9, #14]
	subs	w8, w8, w9
	b.hi	.LBB165_6
// %bb.5:
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #14]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #168]
	b	.LBB165_9
.LBB165_6:                              //   in Loop: Header=BB165_1 Depth=1
.LBB165_7:                              //   in Loop: Header=BB165_1 Depth=1
// %bb.8:                               //   in Loop: Header=BB165_1 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB165_1
.LBB165_9:
	mov	w0, wzr
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end165:
	.size	check_cross_talk, .Lfunc_end165-check_cross_talk
	.cfi_endproc
                                        // -- End function
	.globl	print_smart                     // -- Begin function print_smart
	.p2align	2
	.type	print_smart,@function
print_smart:                            // @print_smart
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #656]
	cbnz	w8, .LBB166_2
// %bb.1:
	b	.LBB166_3
.LBB166_2:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #3440]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #112]
	adrp	x0, .L.str.315
	add	x0, x0, :lo12:.L.str.315
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #64]
	adrp	x0, .L.str.316
	add	x0, x0, :lo12:.L.str.316
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #72]
	adrp	x0, .L.str.317
	add	x0, x0, :lo12:.L.str.317
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #80]
	adrp	x0, .L.str.318
	add	x0, x0, :lo12:.L.str.318
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #88]
	adrp	x0, .L.str.319
	add	x0, x0, :lo12:.L.str.319
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #96]
	adrp	x0, .L.str.320
	add	x0, x0, :lo12:.L.str.320
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x1, [x8, #104]
	adrp	x0, .L.str.321
	add	x0, x0, :lo12:.L.str.321
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #116]
	adrp	x0, .L.str.322
	add	x0, x0, :lo12:.L.str.322
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #120]
	adrp	x0, .L.str.323
	add	x0, x0, :lo12:.L.str.323
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #124]
	adrp	x0, .L.str.324
	add	x0, x0, :lo12:.L.str.324
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #128]
	adrp	x0, .L.str.325
	add	x0, x0, :lo12:.L.str.325
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #180]
	adrp	x0, .L.str.326
	add	x0, x0, :lo12:.L.str.326
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #132]
	adrp	x0, .L.str.327
	add	x0, x0, :lo12:.L.str.327
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #136]
	adrp	x0, .L.str.328
	add	x0, x0, :lo12:.L.str.328
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #140]
	adrp	x0, .L.str.329
	add	x0, x0, :lo12:.L.str.329
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #144]
	adrp	x0, .L.str.330
	add	x0, x0, :lo12:.L.str.330
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #148]
	adrp	x0, .L.str.331
	add	x0, x0, :lo12:.L.str.331
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #152]
	adrp	x0, .L.str.332
	add	x0, x0, :lo12:.L.str.332
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #156]
	adrp	x0, .L.str.333
	add	x0, x0, :lo12:.L.str.333
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #160]
	adrp	x0, .L.str.334
	add	x0, x0, :lo12:.L.str.334
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #164]
	adrp	x0, .L.str.335
	add	x0, x0, :lo12:.L.str.335
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #168]
	adrp	x0, .L.str.336
	add	x0, x0, :lo12:.L.str.336
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #172]
	adrp	x0, .L.str.337
	add	x0, x0, :lo12:.L.str.337
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #176]
	adrp	x0, .L.str.338
	add	x0, x0, :lo12:.L.str.338
	bl	nand_print_dbg
.LBB166_3:
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end166:
	.size	print_smart, .Lfunc_end166-print_smart
	.cfi_endproc
                                        // -- End function
	.globl	get_used_block_count            // -- Begin function get_used_block_count
	.p2align	2
	.type	get_used_block_count,@function
get_used_block_count:                   // @get_used_block_count
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	add	x0, sp, #40                     // =40
	str	x0, [sp]                        // 8-byte Folded Spill
	mov	w3, wzr
	mov	w2, w3
	mov	x4, xzr
	add	x5, sp, #20                     // =20
	str	x5, [sp, #8]                    // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	blr	x8
	ldr	x0, [sp, #8]                    // 8-byte Folded Reload
	bl	get_block_used_count_from_oob
	str	w0, [sp, #36]
	ldr	w0, [sp, #36]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end167:
	.size	get_used_block_count, .Lfunc_end167-get_used_block_count
	.cfi_endproc
                                        // -- End function
	.globl	block_last_page_ecc_error       // -- Begin function block_last_page_ecc_error
	.p2align	2
	.type	block_last_page_ecc_error,@function
block_last_page_ecc_error:              // @block_last_page_ecc_error
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #144                    // =144
	stp	x29, x30, [sp, #128]            // 16-byte Folded Spill
	add	x29, sp, #128                   // =128
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	adrp	x0, .L.str.214
	add	x0, x0, :lo12:.L.str.214
	bl	nand_print
	stur	wzr, [x29, #-24]
	mov	w0, #8192
	bl	nftl_malloc
	str	x0, [sp, #48]
	ldr	x8, [sp, #48]
	cbnz	x8, .LBB168_2
// %bb.1:
	adrp	x0, .L.str.215
	add	x0, x0, :lo12:.L.str.215
	adrp	x1, .L__func__.block_last_page_ecc_error
	add	x1, x1, :lo12:.L__func__.block_last_page_ecc_error
	bl	nand_print
.LBB168_2:
	stur	wzr, [x29, #-20]
.LBB168_3:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ge	.LBB168_8
// %bb.4:                               //   in Loop: Header=BB168_3 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w0, [x8, #26]
	bl	nftl_malloc
	ldr	x8, [sp, #48]
	ldursw	x9, [x29, #-20]
	add	x8, x8, x9, lsl #3
	str	x0, [x8]
	ldr	x8, [sp, #48]
	ldursw	x9, [x29, #-20]
	ldr	x8, [x8, x9, lsl #3]
	cbnz	x8, .LBB168_6
// %bb.5:                               //   in Loop: Header=BB168_3 Depth=1
	adrp	x0, .L.str.216
	add	x0, x0, :lo12:.L.str.216
	adrp	x1, .L__func__.block_last_page_ecc_error
	add	x1, x1, :lo12:.L__func__.block_last_page_ecc_error
	bl	nand_print
.LBB168_6:                              //   in Loop: Header=BB168_3 Depth=1
// %bb.7:                               //   in Loop: Header=BB168_3 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB168_3
.LBB168_8:
	stur	wzr, [x29, #-20]
.LBB168_9:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ge	.LBB168_26
// %bb.10:                              //   in Loop: Header=BB168_9 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-20]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #32]                   // 8-byte Folded Spill
	sub	x5, x29, #44                    // =44
	str	x5, [sp, #40]                   // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #32]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	blr	x8
	mov	w8, w0
	ldr	x0, [sp, #40]                   // 8-byte Folded Reload
	stur	w8, [x29, #-28]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB168_12
// %bb.11:
	b	.LBB168_26
.LBB168_12:                             //   in Loop: Header=BB168_9 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_ftl_logic_page_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB168_17
// %bb.13:                              //   in Loop: Header=BB168_9 Depth=1
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB168_15
// %bb.14:                              //   in Loop: Header=BB168_9 Depth=1
	ldur	w8, [x29, #-28]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB168_16
.LBB168_15:                             //   in Loop: Header=BB168_9 Depth=1
	ldr	x8, [sp, #48]
	ldursw	x9, [x29, #-20]
	ldr	x0, [x8, x9, lsl #3]
	ldur	x8, [x29, #-8]
	ldr	x1, [x8, #512]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w2, [x8, #26]
	bl	nand_memcpy
	ldur	w8, [x29, #-24]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-24]
.LBB168_16:                             //   in Loop: Header=BB168_9 Depth=1
	b	.LBB168_24
.LBB168_17:                             //   in Loop: Header=BB168_9 Depth=1
	sub	x0, x29, #44                    // =44
	bl	is_ftl_special_data
	subs	w8, w0, #1                      // =1
	b.ne	.LBB168_22
// %bb.18:                              //   in Loop: Header=BB168_9 Depth=1
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB168_20
// %bb.19:                              //   in Loop: Header=BB168_9 Depth=1
	ldur	w8, [x29, #-28]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB168_21
.LBB168_20:                             //   in Loop: Header=BB168_9 Depth=1
	ldr	x8, [sp, #48]
	ldursw	x9, [x29, #-20]
	ldr	x0, [x8, x9, lsl #3]
	ldur	x8, [x29, #-8]
	ldr	x1, [x8, #512]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w2, [x8, #26]
	bl	nand_memcpy
	ldur	w8, [x29, #-24]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-24]
.LBB168_21:                             //   in Loop: Header=BB168_9 Depth=1
	adrp	x0, .L.str.217
	add	x0, x0, :lo12:.L.str.217
	bl	nand_print
	b	.LBB168_23
.LBB168_22:                             //   in Loop: Header=BB168_9 Depth=1
	adrp	x0, .L.str.218
	add	x0, x0, :lo12:.L.str.218
	bl	nand_print
.LBB168_23:                             //   in Loop: Header=BB168_9 Depth=1
.LBB168_24:                             //   in Loop: Header=BB168_9 Depth=1
// %bb.25:                              //   in Loop: Header=BB168_9 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB168_9
.LBB168_26:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-20]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	sub	x5, x29, #44                    // =44
	bl	set_physic_op_par
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-8]
	blr	x8
	stur	w0, [x29, #-28]
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB168_31
// %bb.27:
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-8]
	add	x1, sp, #56                     // =56
	blr	x8
	ldur	x0, [x29, #-8]
	bl	out_phy_block_from_free_list
	stur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	cbnz	x8, .LBB168_29
// %bb.28:
	adrp	x0, .L.str.206
	add	x0, x0, :lo12:.L.str.206
	bl	nand_print
	stur	xzr, [x29, #-16]
	b	.LBB168_36
.LBB168_29:
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-20]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	x4, [x8, #512]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	sub	x5, x29, #44                    // =44
	bl	set_physic_op_par
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-8]
	blr	x8
// %bb.30:
.LBB168_31:
	stur	wzr, [x29, #-20]
.LBB168_32:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	b.ge	.LBB168_35
// %bb.33:                              //   in Loop: Header=BB168_32 Depth=1
	ldur	x8, [x29, #-16]
	ldrh	w1, [x8]
	ldur	w2, [x29, #-20]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldr	x8, [sp, #48]
	ldursw	x9, [x29, #-20]
	ldr	x4, [x8, x9, lsl #3]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	sub	x5, x29, #44                    // =44
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	blr	x8
// %bb.34:                              //   in Loop: Header=BB168_32 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB168_32
.LBB168_35:
.LBB168_36:
	stur	wzr, [x29, #-20]
.LBB168_37:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.ge	.LBB168_40
// %bb.38:                              //   in Loop: Header=BB168_37 Depth=1
	ldr	x8, [sp, #48]
	ldursw	x9, [x29, #-20]
	ldr	x0, [x8, x9, lsl #3]
	bl	nftl_free
// %bb.39:                              //   in Loop: Header=BB168_37 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB168_37
.LBB168_40:
	ldr	x0, [sp, #48]
	bl	nftl_free
	adrp	x0, .L.str.219
	add	x0, x0, :lo12:.L.str.219
	bl	nand_print
	ldur	x0, [x29, #-16]
	ldp	x29, x30, [sp, #128]            // 16-byte Folded Reload
	add	sp, sp, #144                    // =144
	ret
.Lfunc_end168:
	.size	block_last_page_ecc_error, .Lfunc_end168-block_last_page_ecc_error
	.cfi_endproc
                                        // -- End function
	.globl	adjust_free_list                // -- Begin function adjust_free_list
	.p2align	2
	.type	adjust_free_list,@function
adjust_free_list:                       // @adjust_free_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	out_phy_block_from_free_list
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB169_2
// %bb.1:
	adrp	x0, .L.str.220
	add	x0, x0, :lo12:.L.str.220
	bl	nand_print
.LBB169_2:
	ldr	x0, [sp, #8]
	ldr	x1, [sp]
	ldr	x8, [sp]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
	ldr	x0, [sp, #8]
	ldr	x1, [sp]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [sp]
	ldr	x8, [sp]
	ldr	x9, [sp, #8]
	str	x8, [x9, #64]
	ldr	x8, [sp]
	ldrh	w1, [x8]
	ldr	x8, [sp]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.221
	add	x0, x0, :lo12:.L.str.221
	bl	nand_print
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end169:
	.size	adjust_free_list, .Lfunc_end169-adjust_free_list
	.cfi_endproc
                                        // -- End function
	.globl	nftl_get_bad_block_cnt          // -- Begin function nftl_get_bad_block_cnt
	.p2align	2
	.type	nftl_get_bad_block_cnt,@function
nftl_get_bad_block_cnt:                 // @nftl_get_bad_block_cnt
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	ldrh	w0, [x8, #28]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end170:
	.size	nftl_get_bad_block_cnt, .Lfunc_end170-nftl_get_bad_block_cnt
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_phy_read               // -- Begin function nand_dbg_phy_read
	.p2align	2
	.type	nand_dbg_phy_read,@function
nand_dbg_phy_read:                      // @nand_dbg_phy_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #128                    // =128
	stp	x29, x30, [sp, #112]            // 16-byte Folded Spill
	add	x29, sp, #112                   // =112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	sturh	w0, [x29, #-2]
	sturh	w1, [x29, #-4]
	sturh	w2, [x29, #-6]
	mov	w0, #4096
	bl	nftl_malloc
	stur	x0, [x29, #-24]
	ldur	x8, [x29, #-24]
	cbnz	x8, .LBB171_2
// %bb.1:
	adrp	x0, .L.str.275
	add	x0, x0, :lo12:.L.str.275
	adrp	x1, .L__func__.nand_dbg_phy_read
	add	x1, x1, :lo12:.L__func__.nand_dbg_phy_read
	bl	nand_print
.LBB171_2:
	ldurh	w0, [x29, #-2]
	ldurh	w1, [x29, #-4]
	ldurh	w2, [x29, #-6]
	ldur	x4, [x29, #-24]
	mov	w3, #8
	sub	x5, x29, #40                    // =40
	bl	page_read
	stur	w0, [x29, #-12]
	ldurb	w1, [x29, #-40]
	ldurb	w2, [x29, #-39]
	ldurb	w3, [x29, #-38]
	ldurb	w4, [x29, #-37]
	ldurb	w5, [x29, #-36]
	ldurb	w6, [x29, #-35]
	ldurb	w7, [x29, #-34]
	ldurb	w17, [x29, #-33]
	ldurb	w16, [x29, #-32]
	ldurb	w15, [x29, #-31]
	ldurb	w14, [x29, #-30]
	ldurb	w13, [x29, #-29]
	ldurb	w12, [x29, #-28]
	ldurb	w11, [x29, #-27]
	ldurb	w10, [x29, #-26]
	ldurb	w8, [x29, #-25]
	adrp	x0, .L.str.276
	add	x0, x0, :lo12:.L.str.276
	mov	x9, sp
	str	w17, [x9]
	str	w16, [x9, #8]
	str	w15, [x9, #16]
	str	w14, [x9, #24]
	str	w13, [x9, #32]
	str	w12, [x9, #40]
	str	w11, [x9, #48]
	str	w10, [x9, #56]
	str	w8, [x9, #64]
	bl	nand_print_dbg
	ldur	x8, [x29, #-24]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-24]
	ldrb	w2, [x8, #1]
	ldur	x8, [x29, #-24]
	ldrb	w3, [x8, #2]
	ldur	x8, [x29, #-24]
	ldrb	w4, [x8, #3]
	ldur	x8, [x29, #-24]
	ldrb	w5, [x8, #4]
	ldur	x8, [x29, #-24]
	ldrb	w6, [x8, #5]
	ldur	x8, [x29, #-24]
	ldrb	w7, [x8, #6]
	ldur	x8, [x29, #-24]
	ldrb	w12, [x8, #7]
	ldur	x8, [x29, #-24]
	ldrb	w11, [x8, #8]
	ldur	x8, [x29, #-24]
	ldrb	w10, [x8, #9]
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #10]
	adrp	x0, .L.str.277
	add	x0, x0, :lo12:.L.str.277
	mov	x9, sp
	str	w12, [x9]
	str	w11, [x9, #8]
	str	w10, [x9, #16]
	str	w8, [x9, #24]
	bl	nand_print_dbg
	ldur	x0, [x29, #-24]
	bl	nftl_free
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #112]            // 16-byte Folded Reload
	add	sp, sp, #128                    // =128
	ret
.Lfunc_end171:
	.size	nand_dbg_phy_read, .Lfunc_end171-nand_dbg_phy_read
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_zone_phy_read          // -- Begin function nand_dbg_zone_phy_read
	.p2align	2
	.type	nand_dbg_zone_phy_read,@function
nand_dbg_zone_phy_read:                 // @nand_dbg_zone_phy_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #176                    // =176
	stp	x29, x30, [sp, #160]            // 16-byte Folded Spill
	add	x29, sp, #160                   // =160
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	sturh	w2, [x29, #-12]
	mov	w0, #4096
	bl	nftl_malloc
	stur	x0, [x29, #-64]
	ldur	x8, [x29, #-64]
	cbnz	x8, .LBB172_2
// %bb.1:
	adrp	x0, .L.str.275
	add	x0, x0, :lo12:.L.str.275
	adrp	x1, .L__func__.nand_dbg_zone_phy_read
	add	x1, x1, :lo12:.L__func__.nand_dbg_zone_phy_read
	bl	nand_print
.LBB172_2:
	ldur	x0, [x29, #-8]
	ldurh	w1, [x29, #-10]
	bl	get_phy_block_addr
	stur	x0, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldurh	w2, [x29, #-12]
	ldur	x4, [x29, #-64]
	sub	x0, x29, #48                    // =48
	str	x0, [sp, #72]                   // 8-byte Folded Spill
	mov	w3, #8
	add	x5, sp, #80                     // =80
	bl	set_physic_op_par
	ldr	x1, [sp, #72]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-8]
	blr	x8
	stur	w0, [x29, #-52]
	ldrb	w1, [sp, #80]
	ldrb	w2, [sp, #81]
	ldrb	w3, [sp, #82]
	ldrb	w4, [sp, #83]
	ldrb	w5, [sp, #84]
	ldrb	w6, [sp, #85]
	ldrb	w7, [sp, #86]
	ldrb	w17, [sp, #87]
	ldrb	w16, [sp, #88]
	ldrb	w15, [sp, #89]
	ldrb	w14, [sp, #90]
	ldrb	w13, [sp, #91]
	ldrb	w12, [sp, #92]
	ldrb	w11, [sp, #93]
	ldrb	w10, [sp, #94]
	ldrb	w8, [sp, #95]
	adrp	x0, .L.str.276
	add	x0, x0, :lo12:.L.str.276
	mov	x9, sp
	str	w17, [x9]
	str	w16, [x9, #8]
	str	w15, [x9, #16]
	str	w14, [x9, #24]
	str	w13, [x9, #32]
	str	w12, [x9, #40]
	str	w11, [x9, #48]
	str	w10, [x9, #56]
	str	w8, [x9, #64]
	bl	nand_print_dbg
	ldur	x8, [x29, #-64]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-64]
	ldrb	w2, [x8, #1]
	ldur	x8, [x29, #-64]
	ldrb	w3, [x8, #2]
	ldur	x8, [x29, #-64]
	ldrb	w4, [x8, #3]
	ldur	x8, [x29, #-64]
	ldrb	w5, [x8, #4]
	ldur	x8, [x29, #-64]
	ldrb	w6, [x8, #5]
	ldur	x8, [x29, #-64]
	ldrb	w7, [x8, #6]
	ldur	x8, [x29, #-64]
	ldrb	w12, [x8, #7]
	ldur	x8, [x29, #-64]
	ldrb	w11, [x8, #8]
	ldur	x8, [x29, #-64]
	ldrb	w10, [x8, #9]
	ldur	x8, [x29, #-64]
	ldrb	w8, [x8, #10]
	adrp	x0, .L.str.277
	add	x0, x0, :lo12:.L.str.277
	mov	x9, sp
	str	w12, [x9]
	str	w11, [x9, #8]
	str	w10, [x9, #16]
	str	w8, [x9, #24]
	bl	nand_print_dbg
	ldur	x0, [x29, #-64]
	bl	nftl_free
	ldur	w0, [x29, #-52]
	ldp	x29, x30, [sp, #160]            // 16-byte Folded Reload
	add	sp, sp, #176                    // =176
	ret
.Lfunc_end172:
	.size	nand_dbg_zone_phy_read, .Lfunc_end172-nand_dbg_zone_phy_read
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_zone_phy_write         // -- Begin function nand_dbg_zone_phy_write
	.p2align	2
	.type	nand_dbg_zone_phy_write,@function
nand_dbg_zone_phy_write:                // @nand_dbg_zone_phy_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	sturh	w2, [x29, #-12]
	mov	w0, #8192
	bl	nftl_malloc
	str	x0, [sp, #32]
	ldr	x8, [sp, #32]
	cbnz	x8, .LBB173_2
// %bb.1:
	adrp	x0, .L.str.278
	add	x0, x0, :lo12:.L.str.278
	adrp	x1, .L__func__.nand_dbg_zone_phy_write
	add	x1, x1, :lo12:.L__func__.nand_dbg_zone_phy_write
	bl	nand_print
.LBB173_2:
	ldur	x0, [x29, #-8]
	ldurh	w1, [x29, #-10]
	bl	get_phy_block_addr
	stur	x0, [x29, #-24]
	ldr	x0, [sp, #32]
	mov	w1, #-91
	mov	w2, #4096
	bl	nand_memset
	add	x5, sp, #16                     // =16
	mov	w8, #255
	strb	w8, [sp, #16]
	strb	w8, [sp, #17]
	strb	w8, [sp, #18]
	strb	w8, [sp, #19]
	strb	w8, [sp, #20]
	mov	w9, #165
	strb	w9, [sp, #21]
	strb	w9, [sp, #22]
	strb	w8, [sp, #23]
	strb	w8, [sp, #24]
	strb	w8, [sp, #25]
	strb	w8, [sp, #26]
	ldur	x8, [x29, #-24]
	ldrh	w1, [x8]
	ldurh	w2, [x29, #-12]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #32]
	add	x0, sp, #48                     // =48
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-8]
	blr	x8
	str	w0, [sp, #44]
	ldr	x0, [sp, #32]
	bl	nftl_free
	ldr	w0, [sp, #44]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end173:
	.size	nand_dbg_zone_phy_write, .Lfunc_end173-nand_dbg_zone_phy_write
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_phy_write              // -- Begin function nand_dbg_phy_write
	.p2align	2
	.type	nand_dbg_phy_write,@function
nand_dbg_phy_write:                     // @nand_dbg_phy_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	sturh	w0, [x29, #-2]
	sturh	w1, [x29, #-4]
	sturh	w2, [x29, #-6]
	mov	w0, #8192
	bl	nftl_malloc
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	cbnz	x8, .LBB174_2
// %bb.1:
	adrp	x0, .L.str.278
	add	x0, x0, :lo12:.L.str.278
	adrp	x1, .L__func__.nand_dbg_phy_write
	add	x1, x1, :lo12:.L__func__.nand_dbg_phy_write
	bl	nand_print
.LBB174_2:
	ldr	x0, [sp, #24]
	mov	w1, #-91
	mov	w2, #4096
	bl	nand_memset
	add	x5, sp, #8                      // =8
	mov	w8, #255
	strb	w8, [sp, #8]
	strb	w8, [sp, #9]
	strb	w8, [sp, #10]
	strb	w8, [sp, #11]
	strb	w8, [sp, #12]
	mov	w9, #165
	strb	w9, [sp, #13]
	strb	w9, [sp, #14]
	strb	w8, [sp, #15]
	strb	w8, [sp, #16]
	strb	w8, [sp, #17]
	strb	w8, [sp, #18]
	ldurh	w0, [x29, #-2]
	ldurh	w1, [x29, #-4]
	ldurh	w2, [x29, #-6]
	ldr	x4, [sp, #24]
	mov	w3, #8
	bl	page_write
	stur	w0, [x29, #-12]
	ldr	x0, [sp, #24]
	bl	nftl_free
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end174:
	.size	nand_dbg_phy_write, .Lfunc_end174-nand_dbg_phy_write
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_zone_erase             // -- Begin function nand_dbg_zone_erase
	.p2align	2
	.type	nand_dbg_zone_erase,@function
nand_dbg_zone_erase:                    // @nand_dbg_zone_erase
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	sturh	w2, [x29, #-12]
	ldur	x0, [x29, #-8]
	ldurh	w1, [x29, #-10]
	bl	get_phy_block_addr
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #8]
	bl	out_phy_block_from_free_list_by_block
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #8]
	ldurh	w2, [x29, #-12]
	bl	erase_block
	str	w0, [sp, #16]
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end175:
	.size	nand_dbg_zone_erase, .Lfunc_end175-nand_dbg_zone_erase
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_phy_erase              // -- Begin function nand_dbg_phy_erase
	.p2align	2
	.type	nand_dbg_phy_erase,@function
nand_dbg_phy_erase:                     // @nand_dbg_phy_erase
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	sturh	w0, [x29, #-2]
	sturh	w1, [x29, #-4]
	ldurh	w0, [x29, #-2]
	ldurh	w1, [x29, #-4]
	bl	block_erase
	str	w0, [sp, #8]
	ldr	w0, [sp, #8]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end176:
	.size	nand_dbg_phy_erase, .Lfunc_end176-nand_dbg_phy_erase
	.cfi_endproc
                                        // -- End function
	.globl	nand_dbg_single_phy_erase       // -- Begin function nand_dbg_single_phy_erase
	.p2align	2
	.type	nand_dbg_single_phy_erase,@function
nand_dbg_single_phy_erase:              // @nand_dbg_single_phy_erase
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	sturh	w0, [x29, #-2]
	sturh	w1, [x29, #-4]
	ldurh	w0, [x29, #-2]
	ldurh	w1, [x29, #-4]
	bl	nand_physic_erase_block
	str	w0, [sp, #8]
	ldr	w0, [sp, #8]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end177:
	.size	nand_dbg_single_phy_erase, .Lfunc_end177-nand_dbg_single_phy_erase
	.cfi_endproc
                                        // -- End function
	.globl	clean_block_list                // -- Begin function clean_block_list
	.p2align	2
	.type	clean_block_list,@function
clean_block_list:                       // @clean_block_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
.LBB178_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x0, [sp, #8]
	bl	out_phy_block_from_free_list
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB178_3
// %bb.2:
	b	.LBB178_4
.LBB178_3:                              //   in Loop: Header=BB178_1 Depth=1
	ldr	x8, [sp]
	strh	wzr, [x8, #4]
	ldr	x8, [sp]
	strh	wzr, [x8, #2]
	ldr	x9, [sp]
	mov	w8, #-1
	str	w8, [x9, #8]
	ldr	x8, [sp]
	str	xzr, [x8, #16]
	ldr	x8, [sp]
	str	xzr, [x8, #24]
	ldr	x8, [sp]
	str	xzr, [x8, #32]
	ldr	x8, [sp]
	str	xzr, [x8, #40]
	ldr	x8, [sp]
	str	xzr, [x8, #48]
	ldr	x8, [sp]
	str	xzr, [x8, #56]
	b	.LBB178_1
.LBB178_4:
.LBB178_5:                              // =>This Inner Loop Header: Depth=1
	ldr	x0, [sp, #8]
	bl	out_phy_block_from_invalid_page_list
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB178_7
// %bb.6:
	b	.LBB178_8
.LBB178_7:                              //   in Loop: Header=BB178_5 Depth=1
	ldr	x8, [sp]
	strh	wzr, [x8, #4]
	ldr	x8, [sp]
	strh	wzr, [x8, #2]
	ldr	x9, [sp]
	mov	w8, #-1
	str	w8, [x9, #8]
	ldr	x8, [sp]
	str	xzr, [x8, #16]
	ldr	x8, [sp]
	str	xzr, [x8, #24]
	ldr	x8, [sp]
	str	xzr, [x8, #32]
	ldr	x8, [sp]
	str	xzr, [x8, #40]
	ldr	x8, [sp]
	str	xzr, [x8, #48]
	ldr	x8, [sp]
	str	xzr, [x8, #56]
	b	.LBB178_5
.LBB178_8:
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end178:
	.size	clean_block_list, .Lfunc_end178-clean_block_list
	.cfi_endproc
                                        // -- End function
	.globl	nand_clean_zone_table2          // -- Begin function nand_clean_zone_table2
	.p2align	2
	.type	nand_clean_zone_table2,@function
nand_clean_zone_table2:                 // @nand_clean_zone_table2
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #12]
	ldr	w8, [sp, #12]
                                        // kill: def $x8 killed $w8
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w8, w8, #2
	str	w8, [sp, #4]
	ldur	x8, [x29, #-8]
	ldrb	w1, [x8, #24]
	ldr	w2, [sp, #12]
	adrp	x0, .L.str.279
	add	x0, x0, :lo12:.L.str.279
	bl	nand_print_dbg
	ldr	w8, [sp, #4]
	mov	w9, #1048576
	udiv	w8, w8, w9
	str	w8, [sp, #16]
	stur	wzr, [x29, #-12]
.LBB179_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	ldr	w9, [sp, #16]
	subs	w8, w8, w9
	b.hs	.LBB179_4
// %bb.2:                               //   in Loop: Header=BB179_1 Depth=1
	ldur	x8, [x29, #-8]
	add	x8, x8, #72                     // =72
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	mov	w1, #-1
	mov	w2, #1048576
	str	w2, [sp]                        // 4-byte Folded Spill
	bl	nand_memset
	ldr	w1, [sp]                        // 4-byte Folded Reload
	adrp	x0, .L.str.280
	add	x0, x0, :lo12:.L.str.280
	bl	nand_print_dbg
// %bb.3:                               //   in Loop: Header=BB179_1 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB179_1
.LBB179_4:
	ldr	w8, [sp, #4]
	mov	w9, #-1
	add	w9, w9, #256, lsl #12           // =1048576
	and	w8, w8, w9
	str	w8, [sp, #8]
	ldr	w8, [sp, #8]
	cbz	w8, .LBB179_6
// %bb.5:
	ldur	x8, [x29, #-8]
	add	x8, x8, #72                     // =72
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	ldr	w2, [sp, #8]
	mov	w1, #-1
	bl	nand_memset
	ldr	w1, [sp, #8]
	adrp	x0, .L.str.280
	add	x0, x0, :lo12:.L.str.280
	bl	nand_print_dbg
.LBB179_6:
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #528]
	ldur	x9, [x29, #-8]
	mov	w8, #65535
	strh	w8, [x9, #584]
	ldur	x9, [x29, #-8]
	str	xzr, [x9, #592]
	ldur	x9, [x29, #-8]
	strh	w8, [x9, #648]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #304]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #312]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #288]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #296]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #320]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #328]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #368]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #376]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #352]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #360]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #384]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #392]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #432]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #440]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #416]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #424]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #448]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #456]
	ldur	x8, [x29, #-8]
	strh	wzr, [x8, #28]
	ldur	x8, [x29, #-8]
	strh	wzr, [x8, #30]
	ldur	x8, [x29, #-8]
	strh	wzr, [x8, #696]
	stur	wzr, [x29, #-12]
.LBB179_7:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	subs	w8, w8, #10                     // =10
	b.hs	.LBB179_10
// %bb.8:                               //   in Loop: Header=BB179_7 Depth=1
	ldur	w8, [x29, #-12]
	ldur	x9, [x29, #-8]
	add	x9, x9, #704                    // =704
	ldur	w10, [x29, #-12]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #5
	strh	w8, [x9]
	ldur	x8, [x29, #-8]
	add	x8, x8, #704                    // =704
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-8]
	add	x8, x8, #704                    // =704
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #8]
	ldur	x8, [x29, #-8]
	add	x8, x8, #704                    // =704
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #16]
	ldur	x8, [x29, #-8]
	add	x8, x8, #704                    // =704
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #5
	str	xzr, [x8, #24]
// %bb.9:                               //   in Loop: Header=BB179_7 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB179_7
.LBB179_10:
	ldur	x9, [x29, #-8]
	mov	w8, #255
	strh	w8, [x9, #1024]
	ldur	x8, [x29, #-8]
	strh	wzr, [x8, #698]
	ldur	x8, [x29, #-8]
	strh	wzr, [x8, #1026]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #1032]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #1040]
	ldur	x8, [x29, #-8]
	str	xzr, [x8, #1048]
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end179:
	.size	nand_clean_zone_table2, .Lfunc_end179-nand_clean_zone_table2
	.cfi_endproc
                                        // -- End function
	.globl	nftl_clean_logic_page_map       // -- Begin function nftl_clean_logic_page_map
	.p2align	2
	.type	nftl_clean_logic_page_map,@function
nftl_clean_logic_page_map:              // @nftl_clean_logic_page_map
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #12]
	ldr	w8, [sp, #12]
                                        // kill: def $x8 killed $w8
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w8, w8, #2
	str	w8, [sp, #4]
	ldur	x8, [x29, #-8]
	ldrb	w1, [x8, #24]
	ldr	w2, [sp, #12]
	adrp	x0, .L.str.279
	add	x0, x0, :lo12:.L.str.279
	bl	nand_print_dbg
	ldr	w8, [sp, #4]
	mov	w9, #1048576
	udiv	w8, w8, w9
	str	w8, [sp, #16]
	stur	wzr, [x29, #-12]
.LBB180_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	ldr	w9, [sp, #16]
	subs	w8, w8, w9
	b.hs	.LBB180_4
// %bb.2:                               //   in Loop: Header=BB180_1 Depth=1
	ldur	x8, [x29, #-8]
	add	x8, x8, #72                     // =72
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	mov	w1, #-1
	mov	w2, #1048576
	str	w2, [sp]                        // 4-byte Folded Spill
	bl	nand_memset
	ldr	w1, [sp]                        // 4-byte Folded Reload
	adrp	x0, .L.str.280
	add	x0, x0, :lo12:.L.str.280
	bl	nand_print_dbg
// %bb.3:                               //   in Loop: Header=BB180_1 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB180_1
.LBB180_4:
	ldr	w8, [sp, #4]
	mov	w9, #-1
	add	w9, w9, #256, lsl #12           // =1048576
	and	w8, w8, w9
	str	w8, [sp, #8]
	ldr	w8, [sp, #8]
	cbz	w8, .LBB180_6
// %bb.5:
	ldur	x8, [x29, #-8]
	add	x8, x8, #72                     // =72
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	ldr	x0, [x8, x9, lsl #3]
	ldr	w2, [sp, #8]
	mov	w1, #-1
	bl	nand_memset
	ldr	w1, [sp, #8]
	adrp	x0, .L.str.280
	add	x0, x0, :lo12:.L.str.280
	bl	nand_print_dbg
.LBB180_6:
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end180:
	.size	nftl_clean_logic_page_map, .Lfunc_end180-nftl_clean_logic_page_map
	.cfi_endproc
                                        // -- End function
	.globl	nftl_clean_used_list            // -- Begin function nftl_clean_used_list
	.p2align	2
	.type	nftl_clean_used_list,@function
nftl_clean_used_list:                   // @nftl_clean_used_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	str	wzr, [sp, #4]
	str	wzr, [sp, #4]
.LBB181_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.ge	.LBB181_4
// %bb.2:                               //   in Loop: Header=BB181_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldrsw	x9, [sp, #4]
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #48]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldrsw	x9, [sp, #4]
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #56]
// %bb.3:                               //   in Loop: Header=BB181_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB181_1
.LBB181_4:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #432]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #440]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #416]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #424]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #448]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #456]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end181:
	.size	nftl_clean_used_list, .Lfunc_end181-nftl_clean_used_list
	.cfi_endproc
                                        // -- End function
	.globl	nftl_clean_invalid_list         // -- Begin function nftl_clean_invalid_list
	.p2align	2
	.type	nftl_clean_invalid_list,@function
nftl_clean_invalid_list:                // @nftl_clean_invalid_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	str	wzr, [sp, #4]
	str	wzr, [sp, #4]
.LBB182_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #8]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.ge	.LBB182_4
// %bb.2:                               //   in Loop: Header=BB182_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldrsw	x9, [sp, #4]
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldrsw	x9, [sp, #4]
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #24]
// %bb.3:                               //   in Loop: Header=BB182_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB182_1
.LBB182_4:
	ldr	x8, [sp, #8]
	str	xzr, [x8, #368]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #376]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #352]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #360]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #384]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #392]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end182:
	.size	nftl_clean_invalid_list, .Lfunc_end182-nftl_clean_invalid_list
	.cfi_endproc
                                        // -- End function
	.globl	nand_clean_zone_table           // -- Begin function nand_clean_zone_table
	.p2align	2
	.type	nand_clean_zone_table,@function
nand_clean_zone_table:                  // @nand_clean_zone_table
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	str	x8, [sp]
	ldr	x0, [sp]
	mov	w1, #1000
	bl	__nand_flush_write_cache
	ldr	x0, [sp]
	bl	clean_block_list
	ldr	x0, [sp]
	bl	nand_clean_zone_table2
	ldr	x0, [sp]
	bl	nftl_cache_exit
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end183:
	.size	nand_clean_zone_table, .Lfunc_end183-nand_clean_zone_table
	.cfi_endproc
                                        // -- End function
	.globl	__nand_flush_write_cache        // -- Begin function __nand_flush_write_cache
	.p2align	2
	.type	__nand_flush_write_cache,@function
__nand_flush_write_cache:               // @__nand_flush_write_cache
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	wzr, [sp, #12]
	ldur	x8, [x29, #-8]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp]
	str	wzr, [sp, #16]
.LBB184_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #1784]
	cbz	x8, .LBB184_5
// %bb.2:                               //   in Loop: Header=BB184_1 Depth=1
	ldr	w8, [sp, #12]
	mov	w1, #1
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	ldur	x0, [x29, #-8]
	bl	flush_write_cache_to_nand
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldur	w8, [x29, #-12]
	subs	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	ldur	w8, [x29, #-12]
	cbnz	w8, .LBB184_4
// %bb.3:
	b	.LBB184_5
.LBB184_4:                              //   in Loop: Header=BB184_1 Depth=1
	b	.LBB184_1
.LBB184_5:
	ldr	w8, [sp, #16]
	cbz	w8, .LBB184_7
// %bb.6:
	adrp	x0, .L.str.60
	add	x0, x0, :lo12:.L.str.60
	bl	nand_print
.LBB184_7:
	ldr	w8, [sp, #12]
	cbz	w8, .LBB184_11
// %bb.8:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #656]
	cbz	w8, .LBB184_10
// %bb.9:
	ldur	x8, [x29, #-8]
	ldrb	w1, [x8, #24]
	ldr	w2, [sp, #12]
	adrp	x0, .L.str.311
	add	x0, x0, :lo12:.L.str.311
	bl	nand_print_dbg
.LBB184_10:
.LBB184_11:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end184:
	.size	__nand_flush_write_cache, .Lfunc_end184-__nand_flush_write_cache
	.cfi_endproc
                                        // -- End function
	.globl	nftl_cache_exit                 // -- Begin function nftl_cache_exit
	.p2align	2
	.type	nftl_cache_exit,@function
nftl_cache_exit:                        // @nftl_cache_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #8]
	stur	wzr, [x29, #-12]
.LBB185_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9]
	subs	w8, w8, w9
	b.ge	.LBB185_4
// %bb.2:                               //   in Loop: Header=BB185_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldursw	x9, [x29, #-12]
	mov	x10, #56
	mul	x9, x9, x10
	add	x8, x8, x9
	ldr	x0, [x8, #16]
	bl	nftl_free
// %bb.3:                               //   in Loop: Header=BB185_1 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB185_1
.LBB185_4:
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #1800]
	bl	nftl_free
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end185:
	.size	nftl_cache_exit, .Lfunc_end185-nftl_cache_exit
	.cfi_endproc
                                        // -- End function
	.globl	nand_find_zone_table            // -- Begin function nand_find_zone_table
	.p2align	2
	.type	nand_find_zone_table,@function
nand_find_zone_table:                   // @nand_find_zone_table
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #16]
	ldr	x0, [sp, #16]
	bl	clean_block_list
	ldr	x0, [sp, #16]
	bl	nand_clean_zone_table2
	adrp	x0, .L.str.281
	add	x0, x0, :lo12:.L.str.281
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	build_zone_list
	adrp	x0, .L.str.282
	add	x0, x0, :lo12:.L.str.282
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	nftl_cache_init
	adrp	x0, .L.str.283
	add	x0, x0, :lo12:.L.str.283
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	print_block_invalid_list
	ldr	x0, [sp, #16]
	bl	print_free_list
	ldr	x0, [sp, #8]                    // 8-byte Folded Reload
	bl	nand_print_dbg
	adrp	x0, .L.str.284
	add	x0, x0, :lo12:.L.str.284
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end186:
	.size	nand_find_zone_table, .Lfunc_end186-nand_find_zone_table
	.cfi_endproc
                                        // -- End function
	.globl	nftl_cache_init                 // -- Begin function nftl_cache_init
	.p2align	2
	.type	nftl_cache_init,@function
nftl_cache_init:                        // @nftl_cache_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #22]
	ldr	x9, [sp, #16]
	strh	w8, [x9]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #2]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #4]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #6]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1712]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1720]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1728]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1736]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1768]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1776]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1784]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #1792]
	stur	wzr, [x29, #-20]
.LBB187_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9]
	subs	w8, w8, w9
	b.hs	.LBB187_6
// %bb.2:                               //   in Loop: Header=BB187_1 Depth=1
	ldur	w8, [x29, #-20]
	ldr	x9, [sp, #16]
	add	x9, x9, #8                      // =8
	ldur	w10, [x29, #-20]
	mov	w11, w10
	mov	x10, #56
	str	x10, [sp, #8]                   // 8-byte Folded Spill
	mul	x11, x11, x10
	add	x9, x9, x11
	strh	w8, [x9]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x9, x8, x9
	mov	w8, #255
	strh	w8, [x9, #2]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x9, x8, x9
	mov	w8, #-1
	str	w8, [x9, #8]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #24]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #32]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #40]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #48]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	strh	wzr, [x8, #4]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	strh	wzr, [x8, #6]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w0, [x8, #26]
	bl	nftl_malloc
	ldr	x10, [sp, #8]                   // 8-byte Folded Reload
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	x0, [x8, #16]
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	ldr	x8, [x8, #16]
	cbnz	x8, .LBB187_4
// %bb.3:
	adrp	x0, .L.str.295
	add	x0, x0, :lo12:.L.str.295
	adrp	x1, .L__func__.nftl_cache_init
	add	x1, x1, :lo12:.L__func__.nftl_cache_init
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB187_9
.LBB187_4:                              //   in Loop: Header=BB187_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB187_1 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB187_1
.LBB187_6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w0, [x8, #26]
	bl	nftl_malloc
	ldr	x8, [sp, #16]
	str	x0, [x8, #1800]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #1800]
	cbnz	x8, .LBB187_8
// %bb.7:
	adrp	x0, .L.str.296
	add	x0, x0, :lo12:.L.str.296
	adrp	x1, .L__func__.nftl_cache_init
	add	x1, x1, :lo12:.L__func__.nftl_cache_init
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB187_9
.LBB187_8:
	stur	wzr, [x29, #-4]
.LBB187_9:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end187:
	.size	nftl_cache_init, .Lfunc_end187-nftl_cache_init
	.cfi_endproc
                                        // -- End function
	.globl	write_data_to_nand              // -- Begin function write_data_to_nand
	.p2align	2
	.type	write_data_to_nand,@function
write_data_to_nand:                     // @write_data_to_nand
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	w0, #512
	bl	nftl_malloc
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB188_2
// %bb.1:
	adrp	x0, .L.str.285
	add	x0, x0, :lo12:.L.str.285
	adrp	x1, .L__func__.write_data_to_nand
	add	x1, x1, :lo12:.L__func__.write_data_to_nand
	bl	nand_print
.LBB188_2:
	ldr	x0, [sp, #16]
	mov	w1, #-86
	mov	w2, #512
	bl	nand_memset
	str	wzr, [sp, #12]
.LBB188_3:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	mov	w9, #38912
	subs	w8, w8, w9
	b.ge	.LBB188_6
// %bb.4:                               //   in Loop: Header=BB188_3 Depth=1
	ldur	x0, [x29, #-8]
	ldr	w1, [sp, #12]
	ldr	x3, [sp, #16]
	mov	w2, #1
	bl	__nand_write
// %bb.5:                               //   in Loop: Header=BB188_3 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB188_3
.LBB188_6:
	ldr	x0, [sp, #16]
	bl	nftl_free
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end188:
	.size	write_data_to_nand, .Lfunc_end188-write_data_to_nand
	.cfi_endproc
                                        // -- End function
	.globl	nand_check_table                // -- Begin function nand_check_table
	.p2align	2
	.type	nand_check_table,@function
nand_check_table:                       // @nand_check_table
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #16]
	ldr	x0, [sp, #16]
	mov	w1, #1000
	bl	__nand_flush_write_cache
	adrp	x0, .L.str.286
	add	x0, x0, :lo12:.L.str.286
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	print_nftl_zone
	ldr	x0, [sp, #8]                    // 8-byte Folded Reload
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	nand_clean_zone_table
	ldr	x0, [sp, #8]                    // 8-byte Folded Reload
	bl	nand_print_dbg
	ldr	x0, [sp, #16]
	bl	nand_find_zone_table
	ldr	x0, [sp, #16]
	bl	write_data_to_nand
	ldr	x0, [sp, #8]                    // 8-byte Folded Reload
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end189:
	.size	nand_check_table, .Lfunc_end189-nand_check_table
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function _DumpHexData
	.type	_DumpHexData,@function
_DumpHexData:                           // @_DumpHexData
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #8]
	str	wzr, [sp, #4]
.LBB190_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	ldur	w9, [x29, #-12]
	subs	w8, w8, w9
	b.ge	.LBB190_8
// %bb.2:                               //   in Loop: Header=BB190_1 Depth=1
	ldr	w8, [sp, #4]
	mov	w10, #16
	sdiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	cbnz	w8, .LBB190_4
// %bb.3:                               //   in Loop: Header=BB190_1 Depth=1
.LBB190_4:                              //   in Loop: Header=BB190_1 Depth=1
	ldr	x8, [sp, #8]
	add	x9, x8, #1                      // =1
	str	x9, [sp, #8]
	ldrb	w1, [x8]
	adrp	x0, .L.str.486
	add	x0, x0, :lo12:.L.str.486
	bl	nand_print
	ldr	w8, [sp, #4]
	mov	w10, #16
	sdiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	subs	w8, w8, #15                     // =15
	b.ne	.LBB190_6
// %bb.5:                               //   in Loop: Header=BB190_1 Depth=1
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print
.LBB190_6:                              //   in Loop: Header=BB190_1 Depth=1
// %bb.7:                               //   in Loop: Header=BB190_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB190_1
.LBB190_8:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end190:
	.size	_DumpHexData, .Lfunc_end190-_DumpHexData
	.cfi_endproc
                                        // -- End function
	.globl	clean_zone                      // -- Begin function clean_zone
	.p2align	2
	.type	clean_zone,@function
clean_zone:                             // @clean_zone
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #128                    // =128
	stp	x29, x30, [sp, #112]            // 16-byte Folded Spill
	add	x29, sp, #112                   // =112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-20]
	str	wzr, [sp, #52]
	ldur	x8, [x29, #-16]
	cbz	x8, .LBB191_3
// %bb.1:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #3456]
	cbnz	w8, .LBB191_3
// %bb.2:
	ldr	w8, [sp, #52]
	stur	w8, [x29, #-4]
	b	.LBB191_22
.LBB191_3:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #304]
	stur	x8, [x29, #-32]
.LBB191_4:                              // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-32]
	cbz	x8, .LBB191_9
// %bb.5:                               //   in Loop: Header=BB191_4 Depth=1
	ldur	x8, [x29, #-32]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #40]                   // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #40]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	cbz	w8, .LBB191_7
// %bb.6:                               //   in Loop: Header=BB191_4 Depth=1
	ldur	x8, [x29, #-32]
	ldrh	w2, [x8]
	adrp	x0, .L.str.292
	add	x0, x0, :lo12:.L.str.292
	adrp	x1, .L__func__.clean_zone
	add	x1, x1, :lo12:.L__func__.clean_zone
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-32]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	bl	out_phy_block_from_free_list_by_block
.LBB191_7:                              //   in Loop: Header=BB191_4 Depth=1
	ldur	x8, [x29, #-32]
	ldrh	w9, [x8, #6]
	mov	w8, #1
	add	w9, w9, #1                      // =1
	ldur	x10, [x29, #-32]
	strh	w9, [x10, #6]
	str	w8, [sp, #52]
// %bb.8:                               //   in Loop: Header=BB191_4 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #32]
	stur	x8, [x29, #-32]
	b	.LBB191_4
.LBB191_9:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #448]
	stur	x8, [x29, #-32]
.LBB191_10:                             // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-32]
	cbz	x8, .LBB191_15
// %bb.11:                              //   in Loop: Header=BB191_10 Depth=1
	ldur	x8, [x29, #-32]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #32]                   // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #32]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	cbz	w8, .LBB191_13
// %bb.12:                              //   in Loop: Header=BB191_10 Depth=1
	ldur	x8, [x29, #-32]
	ldrh	w2, [x8]
	adrp	x0, .L.str.293
	add	x0, x0, :lo12:.L.str.293
	adrp	x1, .L__func__.clean_zone
	add	x1, x1, :lo12:.L__func__.clean_zone
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-32]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
.LBB191_13:                             //   in Loop: Header=BB191_10 Depth=1
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #6]
	mov	w9, #1
	str	w9, [sp, #28]                   // 4-byte Folded Spill
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-32]
	strh	w8, [x9, #6]
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	bl	put_phy_block_to_free_list
	ldr	w8, [sp, #28]                   // 4-byte Folded Reload
	str	w8, [sp, #52]
// %bb.14:                              //   in Loop: Header=BB191_10 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #48]
	stur	x8, [x29, #-32]
	b	.LBB191_10
.LBB191_15:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	cbz	x8, .LBB191_19
// %bb.16:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w3, [x8, #28]
	add	x0, sp, #56                     // =56
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-16]
	blr	x8
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	cbz	w8, .LBB191_18
// %bb.17:
	ldur	x8, [x29, #-32]
	ldrh	w2, [x8]
	adrp	x0, .L.str.292
	add	x0, x0, :lo12:.L.str.292
	adrp	x1, .L__func__.clean_zone
	add	x1, x1, :lo12:.L__func__.clean_zone
	bl	nand_print
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3504]
	ldur	x0, [x29, #-16]
	add	x1, sp, #56                     // =56
	blr	x8
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #4]
	ldur	x9, [x29, #-32]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldrh	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #28]
.LBB191_18:
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #2]
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #6]
	mov	w9, #1
	str	w9, [sp, #12]                   // 4-byte Folded Spill
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-32]
	strh	w8, [x9, #6]
	ldur	x8, [x29, #-32]
	strh	wzr, [x8, #4]
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	bl	put_phy_block_to_free_list
	ldr	w8, [sp, #12]                   // 4-byte Folded Reload
	str	w8, [sp, #52]
.LBB191_19:
	ldur	x0, [x29, #-16]
	bl	nftl_clean_logic_page_map
	ldur	x0, [x29, #-16]
	bl	nftl_clean_used_list
	ldur	x0, [x29, #-16]
	bl	nftl_clean_invalid_list
	ldur	x0, [x29, #-16]
	bl	out_phy_block_from_free_list
	ldur	x8, [x29, #-16]
	str	x0, [x8, #528]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	cbnz	x8, .LBB191_21
// %bb.20:
	adrp	x0, .L.str.294
	add	x0, x0, :lo12:.L.str.294
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-20]
.LBB191_21:
	ldur	x8, [x29, #-16]
	strh	wzr, [x8, #584]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #528]
	ldr	w8, [x8, #8]
	add	w8, w8, #1                      // =1
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #528]
	str	w8, [x9, #8]
	ldr	w8, [sp, #52]
	stur	w8, [x29, #-20]
	ldur	w8, [x29, #-20]
	stur	w8, [x29, #-4]
.LBB191_22:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #112]            // 16-byte Folded Reload
	add	sp, sp, #128                    // =128
	ret
.Lfunc_end191:
	.size	clean_zone, .Lfunc_end191-clean_zone
	.cfi_endproc
                                        // -- End function
	.globl	nftl_cache_clean                // -- Begin function nftl_cache_clean
	.p2align	2
	.type	nftl_cache_clean,@function
nftl_cache_clean:                       // @nftl_cache_clean
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #8]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #22]
	ldr	x9, [sp, #8]
	strh	w8, [x9]
	ldr	x8, [sp, #8]
	strh	wzr, [x8, #2]
	ldr	x8, [sp, #8]
	strh	wzr, [x8, #4]
	ldr	x8, [sp, #8]
	strh	wzr, [x8, #6]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1712]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1720]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1728]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1736]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1768]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1776]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1784]
	ldr	x8, [sp, #8]
	str	xzr, [x8, #1792]
	str	wzr, [sp, #20]
.LBB192_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #20]
	ldr	x9, [sp, #8]
	ldrh	w9, [x9]
	subs	w8, w8, w9
	b.hs	.LBB192_4
// %bb.2:                               //   in Loop: Header=BB192_1 Depth=1
	ldr	w8, [sp, #20]
	ldr	x9, [sp, #8]
	add	x9, x9, #8                      // =8
	ldr	w10, [sp, #20]
	mov	w11, w10
	mov	x10, #56
	str	x10, [sp]                       // 8-byte Folded Spill
	mul	x11, x11, x10
	add	x9, x9, x11
	strh	w8, [x9]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x9, x8, x9
	mov	w8, #255
	strh	w8, [x9, #2]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x9, x8, x9
	mov	w8, #-1
	str	w8, [x9, #8]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #24]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #32]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #40]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	str	xzr, [x8, #48]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	strh	wzr, [x8, #4]
	ldr	x8, [sp, #8]
	add	x8, x8, #8                      // =8
	ldr	w9, [sp, #20]
                                        // kill: def $x9 killed $w9
	mul	x9, x9, x10
	add	x8, x8, x9
	strh	wzr, [x8, #6]
// %bb.3:                               //   in Loop: Header=BB192_1 Depth=1
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #20]
	b	.LBB192_1
.LBB192_4:
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end192:
	.size	nftl_cache_clean, .Lfunc_end192-nftl_cache_clean
	.cfi_endproc
                                        // -- End function
	.globl	search_from_cache_read_list     // -- Begin function search_from_cache_read_list
	.p2align	2
	.type	search_from_cache_read_list,@function
search_from_cache_read_list:            // @search_from_cache_read_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	ldr	x8, [sp, #16]
	add	x8, x8, #1688                   // =1688
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
.LBB193_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB193_6
// %bb.2:                               //   in Loop: Header=BB193_1 Depth=1
	ldr	x8, [sp]
	ldr	w8, [x8, #8]
	ldr	w9, [sp, #12]
	subs	w8, w8, w9
	b.ne	.LBB193_4
// %bb.3:
	ldr	x8, [sp]
	str	x8, [sp, #24]
	b	.LBB193_7
.LBB193_4:                              //   in Loop: Header=BB193_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB193_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
	b	.LBB193_1
.LBB193_6:
	str	xzr, [sp, #24]
.LBB193_7:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end193:
	.size	search_from_cache_read_list, .Lfunc_end193-search_from_cache_read_list
	.cfi_endproc
                                        // -- End function
	.globl	search_from_cache_write_list    // -- Begin function search_from_cache_write_list
	.p2align	2
	.type	search_from_cache_write_list,@function
search_from_cache_write_list:           // @search_from_cache_write_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	ldr	x8, [sp, #16]
	add	x8, x8, #1744                   // =1744
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #40]
	str	x8, [sp]
.LBB194_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB194_6
// %bb.2:                               //   in Loop: Header=BB194_1 Depth=1
	ldr	x8, [sp]
	ldr	w8, [x8, #8]
	ldr	w9, [sp, #12]
	subs	w8, w8, w9
	b.ne	.LBB194_4
// %bb.3:
	ldr	x8, [sp]
	str	x8, [sp, #24]
	b	.LBB194_7
.LBB194_4:                              //   in Loop: Header=BB194_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB194_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #40]
	str	x8, [sp]
	b	.LBB194_1
.LBB194_6:
	str	xzr, [sp, #24]
.LBB194_7:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end194:
	.size	search_from_cache_write_list, .Lfunc_end194-search_from_cache_write_list
	.cfi_endproc
                                        // -- End function
	.globl	add_to_cache_read_list_tail     // -- Begin function add_to_cache_read_list_tail
	.p2align	2
	.type	add_to_cache_read_list_tail,@function
add_to_cache_read_list_tail:            // @add_to_cache_read_list_tail
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	str	w2, [sp, #12]
	ldr	x8, [sp, #24]
	add	x8, x8, #1688                   // =1688
	str	x8, [sp]
.LBB195_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	cbz	x8, .LBB195_3
// %bb.2:                               //   in Loop: Header=BB195_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
	b	.LBB195_1
.LBB195_3:
	ldr	x8, [sp, #16]
	ldr	x9, [sp]
	str	x8, [x9, #24]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #24]
	ldr	x8, [sp]
	ldr	x9, [sp, #16]
	str	x8, [x9, #32]
	ldr	w8, [sp, #12]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #2]
	ldr	w8, [sp, #12]
	cbnz	w8, .LBB195_5
// %bb.4:
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #2]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #2]
	b	.LBB195_6
.LBB195_5:
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
.LBB195_6:
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end195:
	.size	add_to_cache_read_list_tail, .Lfunc_end195-add_to_cache_read_list_tail
	.cfi_endproc
                                        // -- End function
	.globl	add_to_cache_write_list_tail    // -- Begin function add_to_cache_write_list_tail
	.p2align	2
	.type	add_to_cache_write_list_tail,@function
add_to_cache_write_list_tail:           // @add_to_cache_write_list_tail
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	add	x8, x8, #1744                   // =1744
	str	x8, [sp, #8]
.LBB196_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #40]
	cbz	x8, .LBB196_3
// %bb.2:                               //   in Loop: Header=BB196_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #40]
	str	x8, [sp, #8]
	b	.LBB196_1
.LBB196_3:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #40]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #40]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	str	x8, [x9, #48]
	ldr	x9, [sp, #16]
	mov	w8, #1
	strh	w8, [x9, #2]
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #4]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end196:
	.size	add_to_cache_write_list_tail, .Lfunc_end196-add_to_cache_write_list_tail
	.cfi_endproc
                                        // -- End function
	.globl	del_from_cache_read_list        // -- Begin function del_from_cache_read_list
	.p2align	2
	.type	del_from_cache_read_list,@function
del_from_cache_read_list:               // @del_from_cache_read_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #32]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldr	x9, [sp, #8]
	str	x8, [x9, #24]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	cbz	x8, .LBB197_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #24]
	str	x8, [x9, #32]
.LBB197_2:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #2]
	cbnz	w8, .LBB197_4
// %bb.3:
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #2]
	subs	w8, w8, #1                      // =1
	strh	w8, [x9, #2]
	b	.LBB197_5
.LBB197_4:
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #6]
	subs	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
.LBB197_5:
	ldr	x9, [sp, #16]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #24]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #32]
	ldr	x0, [sp, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end197:
	.size	del_from_cache_read_list, .Lfunc_end197-del_from_cache_read_list
	.cfi_endproc
                                        // -- End function
	.globl	del_from_cache_write_list       // -- Begin function del_from_cache_write_list
	.p2align	2
	.type	del_from_cache_write_list,@function
del_from_cache_write_list:              // @del_from_cache_write_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #48]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #40]
	ldr	x9, [sp, #8]
	str	x8, [x9, #40]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #40]
	cbz	x8, .LBB198_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #40]
	str	x8, [x9, #48]
.LBB198_2:
	ldr	x8, [sp, #16]
	str	xzr, [x8, #40]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #48]
	ldr	x9, [sp, #16]
	mov	w8, #255
	strh	w8, [x9, #2]
	ldr	x9, [sp, #24]
	ldrh	w8, [x9, #4]
	subs	w8, w8, #1                      // =1
	strh	w8, [x9, #4]
	ldr	x0, [sp, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end198:
	.size	del_from_cache_write_list, .Lfunc_end198-del_from_cache_write_list
	.cfi_endproc
                                        // -- End function
	.globl	get_empty_cahce_node            // -- Begin function get_empty_cahce_node
	.p2align	2
	.type	get_empty_cahce_node,@function
get_empty_cahce_node:                   // @get_empty_cahce_node
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	xzr, [sp]
	strh	wzr, [sp, #14]
.LBB199_1:                              // =>This Inner Loop Header: Depth=1
	ldrh	w8, [sp, #14]
	ldr	x9, [sp, #16]
	ldrh	w9, [x9]
	subs	w8, w8, w9
	b.ge	.LBB199_6
// %bb.2:                               //   in Loop: Header=BB199_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldrh	w9, [sp, #14]
                                        // kill: def $x9 killed $w9
	mov	x10, #56
	mul	x9, x9, x10
	add	x8, x8, x9
	ldrh	w8, [x8, #2]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB199_4
// %bb.3:
	ldr	x8, [sp, #16]
	add	x8, x8, #8                      // =8
	ldrh	w9, [sp, #14]
                                        // kill: def $x9 killed $w9
	mov	x10, #56
	mul	x9, x9, x10
	add	x8, x8, x9
	stur	x8, [x29, #-8]
	b	.LBB199_22
.LBB199_4:                              //   in Loop: Header=BB199_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB199_1 Depth=1
	ldrh	w8, [sp, #14]
	add	w8, w8, #1                      // =1
	strh	w8, [sp, #14]
	b	.LBB199_1
.LBB199_6:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #6]
	subs	w8, w8, #10                     // =10
	b.lt	.LBB199_14
// %bb.7:
	ldr	x8, [sp, #16]
	add	x8, x8, #1688                   // =1688
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
.LBB199_8:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB199_13
// %bb.9:                               //   in Loop: Header=BB199_8 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #2]
	subs	w8, w8, #2                      // =2
	b.ne	.LBB199_11
// %bb.10:
	ldr	x0, [sp, #16]
	ldr	x1, [sp]
	bl	del_from_cache_read_list
	stur	x0, [x29, #-8]
	b	.LBB199_22
.LBB199_11:                             //   in Loop: Header=BB199_8 Depth=1
// %bb.12:                              //   in Loop: Header=BB199_8 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
	b	.LBB199_8
.LBB199_13:
	b	.LBB199_21
.LBB199_14:
	ldr	x8, [sp, #16]
	add	x8, x8, #1688                   // =1688
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
.LBB199_15:                             // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB199_20
// %bb.16:                              //   in Loop: Header=BB199_15 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #2]
	cbnz	w8, .LBB199_18
// %bb.17:
	ldr	x0, [sp, #16]
	ldr	x1, [sp]
	bl	del_from_cache_read_list
	stur	x0, [x29, #-8]
	b	.LBB199_22
.LBB199_18:                             //   in Loop: Header=BB199_15 Depth=1
// %bb.19:                              //   in Loop: Header=BB199_15 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	str	x8, [sp]
	b	.LBB199_15
.LBB199_20:
.LBB199_21:
	stur	xzr, [x29, #-8]
.LBB199_22:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end199:
	.size	get_empty_cahce_node, .Lfunc_end199-get_empty_cahce_node
	.cfi_endproc
                                        // -- End function
	.globl	nand_op                         // -- Begin function nand_op
	.p2align	2
	.type	nand_op,@function
nand_op:                                // @nand_op
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	sturb	w0, [x29, #-5]
	stur	x1, [x29, #-16]
	stur	w2, [x29, #-20]
	stur	w3, [x29, #-24]
	stur	x4, [x29, #-32]
	str	wzr, [sp, #20]
	ldur	w8, [x29, #-24]
	cbnz	w8, .LBB200_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB200_16
.LBB200_2:
	ldur	w8, [x29, #-20]
	ldur	w9, [x29, #-24]
	add	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #32]
	subs	w8, w8, w9
	b.ls	.LBB200_4
// %bb.3:
	adrp	x0, .L.str.300
	add	x0, x0, :lo12:.L.str.300
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB200_16
.LBB200_4:
	ldurb	w8, [x29, #-5]
	cbnz	w8, .LBB200_6
// %bb.5:
	adrp	x8, nand_cache_read
	add	x8, x8, :lo12:nand_cache_read
	str	x8, [sp, #8]
	b	.LBB200_7
.LBB200_6:
	adrp	x8, nand_cache_write
	add	x8, x8, :lo12:nand_cache_write
	str	x8, [sp, #8]
.LBB200_7:
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #24]
	ldr	w8, [sp, #24]
	str	w8, [sp, #28]
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w10, [x9, #8]
	udiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	sturh	w8, [x29, #-34]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldurh	w9, [x29, #-34]
	subs	w8, w8, w9
	sturh	w8, [x29, #-36]
	ldur	w8, [x29, #-24]
	ldurh	w9, [x29, #-36]
	subs	w8, w8, w9
	b.hi	.LBB200_9
// %bb.8:
	ldur	w8, [x29, #-24]
	sturh	w8, [x29, #-36]
	stur	wzr, [x29, #-24]
	b	.LBB200_10
.LBB200_9:
	ldurh	w9, [x29, #-36]
	ldur	w8, [x29, #-24]
	subs	w8, w8, w9
	stur	w8, [x29, #-24]
.LBB200_10:
	ldur	x8, [x29, #-32]
	str	x8, [sp, #32]
	ldurh	w8, [x29, #-36]
	sturh	w8, [x29, #-38]
	ldurh	w9, [x29, #-34]
	ldurh	w8, [x29, #-36]
	orr	w8, w8, w9, lsl #8
	strh	w8, [sp, #40]
	ldr	x8, [sp, #8]
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #28]
	ldrh	w2, [sp, #40]
	ldr	x3, [sp, #32]
	blr	x8
	str	w0, [sp, #20]
	ldur	w8, [x29, #-24]
	cbnz	w8, .LBB200_12
// %bb.11:
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-4]
	b	.LBB200_16
.LBB200_12:
.LBB200_13:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-24]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.ls	.LBB200_15
// %bb.14:                              //   in Loop: Header=BB200_13 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	sturh	w8, [x29, #-36]
	ldr	w8, [sp, #28]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #28]
	ldurh	w8, [x29, #-38]
	lsl	w9, w8, #9
	ldr	x8, [sp, #32]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #32]
	ldurh	w8, [x29, #-36]
	strh	w8, [sp, #40]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w9, [x8, #8]
	ldur	w8, [x29, #-24]
	subs	w8, w8, w9
	stur	w8, [x29, #-24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	sturh	w8, [x29, #-38]
	ldr	x8, [sp, #8]
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #28]
	ldrh	w2, [sp, #40]
	ldr	x3, [sp, #32]
	blr	x8
	ldr	w8, [sp, #20]
	orr	w8, w8, w0
	str	w8, [sp, #20]
	b	.LBB200_13
.LBB200_15:
	ldur	w8, [x29, #-24]
	sturh	w8, [x29, #-36]
	ldr	w8, [sp, #28]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #28]
	ldurh	w8, [x29, #-38]
	lsl	w9, w8, #9
	ldr	x8, [sp, #32]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #32]
	ldurh	w8, [x29, #-36]
	strh	w8, [sp, #40]
	ldr	x8, [sp, #8]
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #28]
	ldrh	w2, [sp, #40]
	ldr	x3, [sp, #32]
	blr	x8
	ldr	w8, [sp, #20]
	orr	w8, w8, w0
	str	w8, [sp, #20]
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-4]
.LBB200_16:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end200:
	.size	nand_op, .Lfunc_end200-nand_op
	.cfi_endproc
                                        // -- End function
	.globl	__nand_discard                  // -- Begin function __nand_discard
	.p2align	2
	.type	__nand_discard,@function
__nand_discard:                         // @__nand_discard
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	w2, [sp, #16]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #656]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB201_2
// %bb.1:
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	adrp	x0, .L.str.299
	add	x0, x0, :lo12:.L.str.299
	bl	nand_print_dbg
.LBB201_2:
	ldur	x0, [x29, #-8]
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	bl	nand_discard
	str	w0, [sp, #12]
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end201:
	.size	__nand_discard, .Lfunc_end201-__nand_discard
	.cfi_endproc
                                        // -- End function
	.globl	nand_discard                    // -- Begin function nand_discard
	.p2align	2
	.type	nand_discard,@function
nand_discard:                           // @nand_discard
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	stur	w2, [x29, #-24]
	str	wzr, [sp, #28]
	ldur	x8, [x29, #-16]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #8]
	ldur	w8, [x29, #-20]
	ldur	w9, [x29, #-24]
	add	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #32]
	subs	w8, w8, w9
	b.ls	.LBB202_2
// %bb.1:
	adrp	x0, .L.str.312
	add	x0, x0, :lo12:.L.str.312
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB202_22
.LBB202_2:
	ldur	w8, [x29, #-24]
	mov	w10, w8
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	x8, [x9, #96]
	add	x8, x8, x10
	str	x8, [x9, #96]
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #32]
	ldur	w8, [x29, #-20]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w10, [x9, #8]
	udiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	sturh	w8, [x29, #-26]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldurh	w9, [x29, #-26]
	subs	w8, w8, w9
	sturh	w8, [x29, #-28]
	ldurh	w8, [x29, #-26]
	cbz	w8, .LBB202_7
// %bb.3:
	ldr	w8, [sp, #32]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #32]
	ldur	w8, [x29, #-24]
	ldurh	w9, [x29, #-28]
	subs	w8, w8, w9
	b.hs	.LBB202_5
// %bb.4:
	stur	wzr, [x29, #-24]
	b	.LBB202_6
.LBB202_5:
	ldurh	w9, [x29, #-28]
	ldur	w8, [x29, #-24]
	subs	w8, w8, w9
	stur	w8, [x29, #-24]
.LBB202_6:
	b	.LBB202_10
.LBB202_7:
	ldur	w8, [x29, #-24]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.hs	.LBB202_9
// %bb.8:
	stur	wzr, [x29, #-24]
.LBB202_9:
.LBB202_10:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w9, [x8, #8]
	ldur	w8, [x29, #-24]
	udiv	w8, w8, w9
	stur	w8, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbnz	w8, .LBB202_12
// %bb.11:
	stur	wzr, [x29, #-4]
	b	.LBB202_22
.LBB202_12:
.LBB202_13:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-24]
	subs	w8, w8, #0                      // =0
	b.ls	.LBB202_21
// %bb.14:                              //   in Loop: Header=BB202_13 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8]
	cbnz	w8, .LBB202_20
// %bb.15:                              //   in Loop: Header=BB202_13 Depth=1
	ldr	x0, [sp, #8]
	ldr	w1, [sp, #32]
	bl	search_from_cache_write_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB202_17
// %bb.16:                              //   in Loop: Header=BB202_13 Depth=1
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_write_list
.LBB202_17:                             //   in Loop: Header=BB202_13 Depth=1
	ldr	x0, [sp, #8]
	ldr	w1, [sp, #32]
	bl	search_from_cache_read_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB202_19
// %bb.18:                              //   in Loop: Header=BB202_13 Depth=1
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_read_list
.LBB202_19:                             //   in Loop: Header=BB202_13 Depth=1
.LBB202_20:                             //   in Loop: Header=BB202_13 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3528]
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #32]
	blr	x8
	ldur	w8, [x29, #-24]
	subs	w8, w8, #1                      // =1
	stur	w8, [x29, #-24]
	ldr	w8, [sp, #32]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	mov	w10, w8
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	x8, [x9, #104]
	add	x8, x8, x10
	str	x8, [x9, #104]
	b	.LBB202_13
.LBB202_21:
	ldr	w8, [sp, #28]
	stur	w8, [x29, #-4]
.LBB202_22:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end202:
	.size	nand_discard, .Lfunc_end202-nand_discard
	.cfi_endproc
                                        // -- End function
	.globl	nand_cache_read                 // -- Begin function nand_cache_read
	.p2align	2
	.type	nand_cache_read,@function
nand_cache_read:                        // @nand_cache_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	sturh	w2, [x29, #-22]
	stur	x3, [x29, #-32]
	ldur	x8, [x29, #-32]
	cbnz	x8, .LBB203_2
// %bb.1:
	adrp	x0, .L.str.301
	add	x0, x0, :lo12:.L.str.301
	bl	nand_print
	stur	wzr, [x29, #-4]
	b	.LBB203_24
.LBB203_2:
	stur	wzr, [x29, #-36]
	ldurh	w8, [x29, #-22]
	asr	w8, w8, #8
	str	w8, [sp, #36]
	ldurh	w8, [x29, #-22]
	and	w8, w8, #0xff
	str	w8, [sp, #32]
	ldr	w8, [sp, #36]
	lsl	w8, w8, #9
	str	w8, [sp, #36]
	ldr	w8, [sp, #32]
	lsl	w8, w8, #9
	str	w8, [sp, #32]
	ldur	x8, [x29, #-16]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8]
	cbz	w8, .LBB203_6
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	str	x8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	stur	w0, [x29, #-36]
	ldur	x0, [x29, #-32]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #36]
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldr	w2, [sp, #32]
	bl	nand_memcpy
	ldur	w8, [x29, #-36]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB203_5
// %bb.4:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #116]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #116]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3512]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	stur	wzr, [x29, #-36]
.LBB203_5:
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
	b	.LBB203_24
.LBB203_6:
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_write_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB203_14
// %bb.7:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #4]
	lsl	w8, w8, #9
	sturh	w8, [x29, #-38]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #6]
	lsl	w8, w8, #9
	strh	w8, [sp, #40]
	ldr	w8, [sp, #36]
	ldurh	w9, [x29, #-38]
	ldrh	w10, [sp, #40]
	add	w9, w9, w10
	subs	w8, w8, w9
	b.hi	.LBB203_11
// %bb.8:
	ldr	w8, [sp, #36]
	ldr	w9, [sp, #32]
	add	w8, w8, w9
	ldurh	w9, [x29, #-38]
	ldrh	w10, [sp, #40]
	add	w9, w9, w10
	subs	w8, w8, w9
	b.hi	.LBB203_10
// %bb.9:
	ldur	x0, [x29, #-32]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #36]
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldr	w2, [sp, #32]
	bl	nand_memcpy
	stur	wzr, [x29, #-4]
	b	.LBB203_24
.LBB203_10:
.LBB203_11:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x9, [sp, #8]
	ldr	x2, [x9, #1800]
	blr	x8
	stur	w0, [x29, #-36]
	ldr	x8, [sp, #24]
	ldrh	w9, [sp, #40]
	add	x0, x8, w9, sxtw
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #1800]
	ldrh	w9, [sp, #40]
	add	x1, x8, w9, sxtw
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #26]
	ldrh	w9, [sp, #40]
	subs	w2, w8, w9
	bl	nand_memcpy
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #6]
	ldur	x0, [x29, #-32]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #36]
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldr	w2, [sp, #32]
	bl	nand_memcpy
	ldur	w8, [x29, #-36]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB203_13
// %bb.12:
.LBB203_13:
	stur	wzr, [x29, #-4]
	b	.LBB203_24
.LBB203_14:
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_read_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB203_19
// %bb.15:
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #2]
	cbnz	w8, .LBB203_17
// %bb.16:
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_read_list
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldur	x0, [x29, #-32]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #36]
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldr	w2, [sp, #32]
	bl	nand_memcpy
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	mov	w2, wzr
	bl	add_to_cache_read_list_tail
	b	.LBB203_18
.LBB203_17:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldur	x0, [x29, #-32]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #36]
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldr	w2, [sp, #32]
	bl	nand_memcpy
.LBB203_18:
	stur	wzr, [x29, #-4]
	b	.LBB203_24
.LBB203_19:
	ldr	x0, [sp, #8]
	bl	get_empty_cahce_node
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB203_21
// %bb.20:
	ldr	x8, [sp, #8]
	ldrh	w1, [x8, #4]
	ldr	x8, [sp, #8]
	ldrh	w2, [x8, #2]
	ldr	x8, [sp, #8]
	ldrh	w3, [x8, #6]
	adrp	x0, .L.str.302
	add	x0, x0, :lo12:.L.str.302
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB203_24
.LBB203_21:
	ldur	w8, [x29, #-20]
	ldr	x9, [sp, #16]
	str	w8, [x9, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	stur	w0, [x29, #-36]
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #4]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #6]
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	mov	w2, wzr
	bl	add_to_cache_read_list_tail
	ldur	x0, [x29, #-32]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #36]
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldr	w2, [sp, #32]
	bl	nand_memcpy
	ldur	w8, [x29, #-36]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB203_23
// %bb.22:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #3440]
	ldr	w8, [x9, #116]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #116]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3512]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	stur	wzr, [x29, #-36]
.LBB203_23:
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
.LBB203_24:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end203:
	.size	nand_cache_read, .Lfunc_end203-nand_cache_read
	.cfi_endproc
                                        // -- End function
	.globl	nand_cache_write                // -- Begin function nand_cache_write
	.p2align	2
	.type	nand_cache_write,@function
nand_cache_write:                       // @nand_cache_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	sturh	w2, [x29, #-22]
	stur	x3, [x29, #-32]
	ldur	x8, [x29, #-32]
	cbnz	x8, .LBB204_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB204_45
.LBB204_2:
	ldurh	w8, [x29, #-22]
	asr	w8, w8, #8
	strh	w8, [sp, #40]
	ldurh	w8, [x29, #-22]
	and	w8, w8, #0xff
	strh	w8, [sp, #38]
	ldrh	w8, [sp, #40]
	lsl	w8, w8, #9
	strh	w8, [sp, #40]
	ldrh	w8, [sp, #38]
	lsl	w8, w8, #9
	strh	w8, [sp, #38]
	stur	wzr, [x29, #-36]
	ldur	x8, [x29, #-16]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8]
	cbz	w8, .LBB204_4
// %bb.3:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	str	x8, [sp, #24]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	ldr	x8, [sp, #24]
	ldrh	w9, [sp, #40]
	add	x0, x8, w9, sxtw
	ldur	x1, [x29, #-32]
	ldrh	w2, [sp, #38]
	bl	nand_memcpy
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3512]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	stur	w0, [x29, #-36]
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
	b	.LBB204_45
.LBB204_4:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #2]
	cbnz	w8, .LBB204_14
// %bb.5:
	ldurh	w8, [x29, #-22]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.ne	.LBB204_14
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #26]
	cbnz	w8, .LBB204_14
// %bb.7:
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_write_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB204_9
// %bb.8:
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_write_list
.LBB204_9:
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_read_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB204_11
// %bb.10:
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_read_list
.LBB204_11:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3512]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldur	x2, [x29, #-32]
	blr	x8
	stur	w0, [x29, #-36]
	ldur	w8, [x29, #-36]
	cbz	w8, .LBB204_13
// %bb.12:
	ldurh	w1, [x29, #-22]
	adrp	x0, .L.str.303
	add	x0, x0, :lo12:.L.str.303
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB204_45
.LBB204_13:
	stur	wzr, [x29, #-4]
	b	.LBB204_45
.LBB204_14:
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_write_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB204_26
// %bb.15:
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_write_list
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #6]
	lsl	w8, w8, #9
	sturh	w8, [x29, #-38]
	ldr	x8, [sp, #16]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.ne	.LBB204_17
// %bb.16:
	b	.LBB204_23
.LBB204_17:
	ldrh	w8, [sp, #40]
	ldurh	w9, [x29, #-38]
	subs	w8, w8, w9
	b.ne	.LBB204_19
// %bb.18:
	ldrh	w10, [sp, #38]
	ldr	x9, [sp, #16]
	ldrh	w8, [x9, #6]
	add	w8, w8, w10, asr #9
	strh	w8, [x9, #6]
	b	.LBB204_22
.LBB204_19:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x9, [sp, #8]
	ldr	x2, [x9, #1800]
	blr	x8
	stur	w0, [x29, #-36]
	ldr	x8, [sp, #24]
	ldurh	w9, [x29, #-38]
	add	x0, x8, w9, sxtw
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #1800]
	ldurh	w9, [x29, #-38]
	add	x1, x8, w9, sxtw
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #26]
	ldurh	w9, [x29, #-38]
	subs	w2, w8, w9
	bl	nand_memcpy
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #6]
	ldur	w8, [x29, #-36]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB204_21
// %bb.20:
	ldur	w1, [x29, #-20]
	adrp	x0, .L.str.304
	add	x0, x0, :lo12:.L.str.304
	bl	nand_print
.LBB204_21:
.LBB204_22:
.LBB204_23:
	ldr	x8, [sp, #24]
	ldrh	w9, [sp, #40]
	add	x0, x8, w9, sxtw
	ldur	x1, [x29, #-32]
	ldrh	w2, [sp, #38]
	bl	nand_memcpy
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	add_to_cache_write_list_tail
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_read_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB204_25
// %bb.24:
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_read_list
.LBB204_25:
	stur	wzr, [x29, #-4]
	b	.LBB204_45
.LBB204_26:
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-20]
	bl	search_from_cache_read_list
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB204_32
// %bb.27:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldr	x8, [sp, #24]
	ldrh	w9, [sp, #40]
	add	x0, x8, w9, sxtw
	ldur	x1, [x29, #-32]
	ldrh	w2, [sp, #38]
	bl	nand_memcpy
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #4]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #6]
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	del_from_cache_read_list
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	add_to_cache_write_list_tail
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.lt	.LBB204_31
// %bb.28:
	ldur	x0, [x29, #-16]
	mov	w1, wzr
	bl	flush_write_cache_to_nand
	cbz	w0, .LBB204_30
// %bb.29:
	adrp	x0, .L.str.305
	add	x0, x0, :lo12:.L.str.305
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB204_45
.LBB204_30:
.LBB204_31:
	stur	wzr, [x29, #-4]
	b	.LBB204_45
.LBB204_32:
.LBB204_33:                             // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #24]
	subs	w8, w8, w9
	b.lt	.LBB204_37
// %bb.34:                              //   in Loop: Header=BB204_33 Depth=1
	ldur	x0, [x29, #-16]
	mov	w1, wzr
	bl	flush_write_cache_to_nand
	cbz	w0, .LBB204_36
// %bb.35:
	adrp	x0, .L.str.306
	add	x0, x0, :lo12:.L.str.306
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB204_45
.LBB204_36:                             //   in Loop: Header=BB204_33 Depth=1
	b	.LBB204_33
.LBB204_37:
	ldr	x0, [sp, #8]
	bl	get_empty_cahce_node
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB204_39
// %bb.38:
	ldr	x8, [sp, #8]
	ldrh	w1, [x8, #4]
	ldr	x8, [sp, #8]
	ldrh	w2, [x8, #2]
	ldr	x8, [sp, #8]
	ldrh	w3, [x8, #6]
	adrp	x0, .L.str.302
	add	x0, x0, :lo12:.L.str.302
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB204_45
.LBB204_39:
	ldur	w8, [x29, #-20]
	ldr	x9, [sp, #16]
	str	w8, [x9, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #24]
	ldrh	w8, [sp, #40]
	cbnz	w8, .LBB204_41
// %bb.40:
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #4]
	ldrh	w8, [sp, #38]
	asr	w8, w8, #9
	ldr	x9, [sp, #16]
	strh	w8, [x9, #6]
	b	.LBB204_44
.LBB204_41:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x2, [sp, #24]
	blr	x8
	stur	w0, [x29, #-36]
	ldur	w8, [x29, #-36]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB204_43
// %bb.42:
	ldur	w1, [x29, #-20]
	adrp	x0, .L.str.307
	add	x0, x0, :lo12:.L.str.307
	bl	nand_print
.LBB204_43:
	ldr	x8, [sp, #16]
	strh	wzr, [x8, #4]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #16]
	strh	w8, [x9, #6]
.LBB204_44:
	ldr	x8, [sp, #24]
	ldrh	w9, [sp, #40]
	add	x0, x8, w9, sxtw
	ldur	x1, [x29, #-32]
	ldrh	w2, [sp, #38]
	bl	nand_memcpy
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	bl	add_to_cache_write_list_tail
	stur	wzr, [x29, #-4]
.LBB204_45:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end204:
	.size	nand_cache_write, .Lfunc_end204-nand_cache_write
	.cfi_endproc
                                        // -- End function
	.globl	flush_write_cache_to_nand       // -- Begin function flush_write_cache_to_nand
	.p2align	2
	.type	flush_write_cache_to_nand,@function
flush_write_cache_to_nand:              // @flush_write_cache_to_nand
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	str	xzr, [sp, #32]
	mov	w8, #1
	strb	w8, [sp, #31]
	adrp	x8, .L__const.flush_write_cache_to_nand.l04a_3d_id
	add	x8, x8, :lo12:.L__const.flush_write_cache_to_nand.l04a_3d_id
	ldr	x8, [x8]
	str	x8, [sp, #16]
	adrp	x8, .L__const.flush_write_cache_to_nand.l05b_3d_id
	add	x8, x8, :lo12:.L__const.flush_write_cache_to_nand.l05b_3d_id
	ldr	x8, [x8]
	str	x8, [sp, #8]
	stur	wzr, [x29, #-24]
	ldur	x8, [x29, #-16]
	add	x8, x8, #1072                   // =1072
	str	x8, [sp, #40]
	ldr	x8, [sp, #40]
	ldr	x8, [x8, #1784]
	cbnz	x8, .LBB205_2
// %bb.1:
	adrp	x0, .L.str.308
	add	x0, x0, :lo12:.L.str.308
	bl	nand_print
	stur	wzr, [x29, #-4]
	b	.LBB205_20
.LBB205_2:
	ldr	x0, [sp, #40]
	ldr	x8, [sp, #40]
	ldr	x1, [x8, #1784]
	bl	del_from_cache_write_list
	str	x0, [sp, #48]
	ldr	x8, [sp, #48]
	cbnz	x8, .LBB205_4
// %bb.3:
	stur	wzr, [x29, #-4]
	b	.LBB205_20
.LBB205_4:
	ldr	x8, [sp, #48]
	ldr	x8, [x8, #16]
	stur	x8, [x29, #-40]
	ldr	x8, [sp, #48]
	ldrh	w8, [x8, #6]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.eq	.LBB205_8
// %bb.5:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3520]
	ldur	x0, [x29, #-16]
	ldr	x9, [sp, #48]
	ldr	w1, [x9, #8]
	ldr	x9, [sp, #40]
	ldr	x2, [x9, #1800]
	blr	x8
	stur	w0, [x29, #-24]
	ldr	x8, [sp, #48]
	ldrh	w8, [x8, #6]
	lsl	w8, w8, #9
	sturh	w8, [x29, #-26]
	ldur	x8, [x29, #-40]
	ldurh	w9, [x29, #-26]
	add	x0, x8, w9, sxtw
	ldr	x8, [sp, #40]
	ldr	x8, [x8, #1800]
	ldurh	w9, [x29, #-26]
	add	x1, x8, w9, sxtw
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrh	w8, [x8, #26]
	ldurh	w9, [x29, #-26]
	subs	w2, w8, w9
	bl	nand_memcpy
	ldur	w8, [x29, #-24]
	subs	w8, w8, #10                     // =10
	b.ne	.LBB205_7
// %bb.6:
	ldr	x8, [sp, #48]
	ldr	w1, [x8, #8]
	ldr	x8, [sp, #40]
	ldrh	w2, [x8, #6]
	adrp	x0, .L.str.309
	add	x0, x0, :lo12:.L.str.309
	bl	nand_print
.LBB205_7:
.LBB205_8:
	ldr	x8, [sp, #48]
	strh	wzr, [x8, #4]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #48]
	strh	w8, [x9, #6]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3512]
	ldur	x0, [x29, #-16]
	ldr	x9, [sp, #48]
	ldr	w1, [x9, #8]
	ldur	x2, [x29, #-40]
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB205_10
// %bb.9:
	adrp	x0, .L.str.310
	add	x0, x0, :lo12:.L.str.310
	bl	nand_print
.LBB205_10:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	str	x8, [sp, #32]
	ldr	x0, [sp, #32]
	add	x1, sp, #16                     // =16
	mov	x2, #8
	bl	nand_memcmp
	strb	w0, [sp, #31]
	ldrb	w8, [sp, #31]
	cbz	w8, .LBB205_12
// %bb.11:
	ldr	x0, [sp, #32]
	add	x1, sp, #8                      // =8
	mov	x2, #8
	bl	nand_memcmp
	strb	w0, [sp, #31]
.LBB205_12:
	ldrb	w8, [sp, #31]
	cbnz	w8, .LBB205_19
// %bb.13:
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #584]
	cbz	w8, .LBB205_18
// %bb.14:
	ldur	x8, [x29, #-16]
	ldrh	w8, [x8, #584]
	subs	w0, w8, #1                      // =1
	bl	nand_phy_get_page_type
	mov	w8, #2
	subs	w8, w8, w0
	b.ne	.LBB205_18
// %bb.15:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #3512]
	ldur	x0, [x29, #-16]
	ldr	x9, [sp, #48]
	ldr	w1, [x9, #8]
	ldur	x2, [x29, #-40]
	blr	x8
	stur	w0, [x29, #-24]
	ldur	w8, [x29, #-24]
	cbz	w8, .LBB205_17
// %bb.16:
	adrp	x0, .L.str.310
	add	x0, x0, :lo12:.L.str.310
	bl	nand_print
.LBB205_17:
.LBB205_18:
.LBB205_19:
	ldr	x0, [sp, #40]
	ldr	x1, [sp, #48]
	mov	w2, #2
	bl	add_to_cache_read_list_tail
	stur	wzr, [x29, #-4]
.LBB205_20:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end205:
	.size	flush_write_cache_to_nand, .Lfunc_end205-flush_write_cache_to_nand
	.cfi_endproc
                                        // -- End function
	.globl	__shutdown_op                   // -- Begin function __shutdown_op
	.p2align	2
	.type	__shutdown_op,@function
__shutdown_op:                          // @__shutdown_op
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	str	wzr, [sp, #4]
	ldr	w0, [sp, #4]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end206:
	.size	__shutdown_op, .Lfunc_end206-__shutdown_op
	.cfi_endproc
                                        // -- End function
	.globl	print_logic_page_map            // -- Begin function print_logic_page_map
	.p2align	2
	.type	print_logic_page_map,@function
print_logic_page_map:                   // @print_logic_page_map
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-8]
	ldr	x9, [x9, #8]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #16]
	str	wzr, [sp, #12]
.LBB207_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #16]
	subs	w8, w8, w9
	b.hs	.LBB207_8
// %bb.2:                               //   in Loop: Header=BB207_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	w1, [sp, #12]
	bl	get_logic_page_map
	str	x0, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB207_4
// %bb.3:                               //   in Loop: Header=BB207_1 Depth=1
	adrp	x0, .L.str.313
	add	x0, x0, :lo12:.L.str.313
	bl	nand_print
	b	.LBB207_7
.LBB207_4:                              //   in Loop: Header=BB207_1 Depth=1
	ldr	x8, [sp]
	ldrh	w8, [x8, #2]
	mov	w9, #65535
	subs	w8, w8, w9
	b.eq	.LBB207_6
// %bb.5:                               //   in Loop: Header=BB207_1 Depth=1
	ldr	w1, [sp, #12]
	ldr	x8, [sp]
	ldrh	w2, [x8, #2]
	ldr	x8, [sp]
	ldrh	w8, [x8]
	and	w8, w8, #0xfff
	and	w3, w8, #0xffff
	ldr	x8, [sp]
	ldrh	w8, [x8]
	lsr	w8, w8, #12
	and	w8, w8, #0x1
	and	w4, w8, #0xffff
	adrp	x0, .L.str.314
	add	x0, x0, :lo12:.L.str.314
	bl	nand_print_dbg
.LBB207_6:                              //   in Loop: Header=BB207_1 Depth=1
.LBB207_7:                              //   in Loop: Header=BB207_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB207_1
.LBB207_8:
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end207:
	.size	print_logic_page_map, .Lfunc_end207-print_logic_page_map
	.cfi_endproc
                                        // -- End function
	.globl	get_last_nftl                   // -- Begin function get_last_nftl
	.p2align	2
	.type	get_last_nftl,@function
get_last_nftl:                          // @get_last_nftl
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	str	x8, [sp, #16]
	ldr	x8, [sp, #24]
	str	x8, [sp, #8]
.LBB208_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	cbz	x8, .LBB208_3
// %bb.2:                               //   in Loop: Header=BB208_1 Depth=1
	ldr	x8, [sp, #16]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #16]
	b	.LBB208_1
.LBB208_3:
	ldr	x0, [sp, #8]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end208:
	.size	get_last_nftl, .Lfunc_end208-get_last_nftl
	.cfi_endproc
                                        // -- End function
	.globl	get_nftl_need_read_claim        // -- Begin function get_nftl_need_read_claim
	.p2align	2
	.type	get_nftl_need_read_claim,@function
get_nftl_need_read_claim:               // @get_nftl_need_read_claim
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #16]
	str	wzr, [sp, #4]
	ldur	x0, [x29, #-8]
	bl	get_last_nftl
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #3440]
	ldr	w8, [x8, #172]
	str	w8, [sp, #4]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #16]
.LBB209_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	cbz	x8, .LBB209_5
// %bb.2:                               //   in Loop: Header=BB209_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldrb	w8, [x8, #24]
	ldr	w9, [sp, #4]
	subs	w8, w8, w9
	b.ne	.LBB209_4
// %bb.3:
	b	.LBB209_5
.LBB209_4:                              //   in Loop: Header=BB209_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #16]
	b	.LBB209_1
.LBB209_5:
	ldr	x0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end209:
	.size	get_nftl_need_read_claim, .Lfunc_end209-get_nftl_need_read_claim
	.cfi_endproc
                                        // -- End function
	.globl	get_nftl_read_claim_page_no     // -- Begin function get_nftl_read_claim_page_no
	.p2align	2
	.type	get_nftl_read_claim_page_no,@function
get_nftl_read_claim_page_no:            // @get_nftl_read_claim_page_no
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	get_last_nftl
	str	x0, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #3440]
	ldr	w0, [x8, #176]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end210:
	.size	get_nftl_read_claim_page_no, .Lfunc_end210-get_nftl_read_claim_page_no
	.cfi_endproc
                                        // -- End function
	.globl	clean_read_reclaim_complete_flag // -- Begin function clean_read_reclaim_complete_flag
	.p2align	2
	.type	clean_read_reclaim_complete_flag,@function
clean_read_reclaim_complete_flag:       // @clean_read_reclaim_complete_flag
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	str	x8, [sp]
.LBB211_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB211_3
// %bb.2:                               //   in Loop: Header=BB211_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #24]
	ldr	w8, [x8, #664]
	subs	w8, w8, #0                      // =0
	cset	w8, eq
	and	w8, w8, #0x1
	ldr	x9, [sp]
	ldr	x9, [x9, #24]
	str	w8, [x9, #664]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB211_1
.LBB211_3:
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end211:
	.size	clean_read_reclaim_complete_flag, .Lfunc_end211-clean_read_reclaim_complete_flag
	.cfi_endproc
                                        // -- End function
	.globl	set_nftl_read_claim_complete    // -- Begin function set_nftl_read_claim_complete
	.p2align	2
	.type	set_nftl_read_claim_complete,@function
set_nftl_read_claim_complete:           // @set_nftl_read_claim_complete
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x9, [x8, #24]
	mov	w8, #1
	str	w8, [x9, #660]
	ldur	x0, [x29, #-8]
	bl	get_last_nftl
	str	x0, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	cbnz	x8, .LBB212_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #3440]
	str	wzr, [x8, #172]
	ldur	x0, [x29, #-8]
	bl	clean_read_reclaim_complete_flag
	b	.LBB212_3
.LBB212_2:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldrb	w8, [x8, #24]
	add	w8, w8, #1                      // =1
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #24]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #172]
.LBB212_3:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #3440]
	mov	w0, wzr
	str	wzr, [x8, #176]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end212:
	.size	set_nftl_read_claim_complete, .Lfunc_end212-set_nftl_read_claim_complete
	.cfi_endproc
                                        // -- End function
	.globl	set_nftl_read_claim_flag        // -- Begin function set_nftl_read_claim_flag
	.p2align	2
	.type	set_nftl_read_claim_flag,@function
set_nftl_read_claim_flag:               // @set_nftl_read_claim_flag
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	w1, [sp, #20]
	str	w2, [sp, #16]
	ldr	x8, [sp, #24]
	str	x8, [sp, #8]
.LBB213_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB213_3
// %bb.2:                               //   in Loop: Header=BB213_1 Depth=1
	ldr	w8, [sp, #20]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #24]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #172]
	ldr	w8, [sp, #16]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #24]
	ldr	x9, [x9, #3440]
	str	w8, [x9, #176]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB213_1
.LBB213_3:
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end213:
	.size	set_nftl_read_claim_flag, .Lfunc_end213-set_nftl_read_claim_flag
	.cfi_endproc
                                        // -- End function
	.globl	get_cold_logic_page             // -- Begin function get_cold_logic_page
	.p2align	2
	.type	get_cold_logic_page,@function
get_cold_logic_page:                    // @get_cold_logic_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	str	w2, [sp, #24]
	ldur	w8, [x29, #-20]
	str	w8, [sp, #20]
.LBB214_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #20]
	ldr	w9, [sp, #24]
	subs	w8, w8, w9
	b.hs	.LBB214_9
// %bb.2:                               //   in Loop: Header=BB214_1 Depth=1
	ldur	x0, [x29, #-16]
	ldr	w1, [sp, #20]
	bl	get_logic_page_map
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB214_4
// %bb.3:                               //   in Loop: Header=BB214_1 Depth=1
	adrp	x0, .L.str.313
	add	x0, x0, :lo12:.L.str.313
	bl	nand_print
	b	.LBB214_8
.LBB214_4:                              //   in Loop: Header=BB214_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8, #2]
	mov	w9, #65535
	subs	w8, w8, w9
	b.eq	.LBB214_7
// %bb.5:                               //   in Loop: Header=BB214_1 Depth=1
	ldr	x8, [sp, #8]
	ldrh	w8, [x8]
	lsr	w8, w8, #12
	and	w8, w8, #0x1
	and	w8, w8, #0xffff
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #664]
	subs	w8, w8, w9
	b.eq	.LBB214_7
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	w10, [x8, #664]
	ldr	x9, [sp, #8]
	ldrh	w8, [x9]
	and	w10, w10, #0x1
	lsl	w10, w10, #12
	and	w8, w8, #0xffffefff
	orr	w8, w8, w10
	strh	w8, [x9]
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-4]
	b	.LBB214_10
.LBB214_7:                              //   in Loop: Header=BB214_1 Depth=1
.LBB214_8:                              //   in Loop: Header=BB214_1 Depth=1
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #20]
	b	.LBB214_1
.LBB214_9:
	mov	w8, #-1
	stur	w8, [x29, #-4]
.LBB214_10:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end214:
	.size	get_cold_logic_page, .Lfunc_end214-get_cold_logic_page
	.cfi_endproc
                                        // -- End function
	.globl	read_reclaim                    // -- Begin function read_reclaim
	.p2align	2
	.type	read_reclaim,@function
read_reclaim:                           // @read_reclaim
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	wzr, [sp, #12]
	ldur	x0, [x29, #-8]
	bl	get_nftl_read_claim_page_no
	str	w0, [sp, #20]
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #24]
	ldr	w1, [sp, #20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #24]
	ldr	w2, [x8, #36]
	bl	get_cold_logic_page
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #1                      // =1
	b.ne	.LBB215_2
// %bb.1:
	ldur	x0, [x29, #-8]
	ldur	x1, [x29, #-16]
	bl	set_nftl_read_claim_complete
	str	w0, [sp, #12]
	b	.LBB215_3
.LBB215_2:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #24]
	ldr	w1, [sp, #16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #8]
	ldrb	w2, [x8, #8]
	ldr	x3, [sp, #24]
	bl	nand_cache_read
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #24]
	ldrb	w1, [x8, #24]
	ldr	w2, [sp, #16]
	bl	set_nftl_read_claim_flag
.LBB215_3:
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end215:
	.size	read_reclaim, .Lfunc_end215-read_reclaim
	.cfi_endproc
                                        // -- End function
	.globl	nftl_get_boot_cnt               // -- Begin function nftl_get_boot_cnt
	.p2align	2
	.type	nftl_get_boot_cnt,@function
nftl_get_boot_cnt:                      // @nftl_get_boot_cnt
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x8, [x8, #3440]
	ldr	w0, [x8, #140]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end216:
	.size	nftl_get_boot_cnt, .Lfunc_end216-nftl_get_boot_cnt
	.cfi_endproc
                                        // -- End function
	.globl	nftl_get_zone_cap               // -- Begin function nftl_get_zone_cap
	.p2align	2
	.type	nftl_get_zone_cap,@function
nftl_get_zone_cap:                      // @nftl_get_zone_cap
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	w0, [x8, #32]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end217:
	.size	nftl_get_zone_cap, .Lfunc_end217-nftl_get_zone_cap
	.cfi_endproc
                                        // -- End function
	.globl	nftl_set_zone_test              // -- Begin function nftl_set_zone_test
	.p2align	2
	.type	nftl_set_zone_test,@function
nftl_set_zone_test:                     // @nftl_set_zone_test
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	ldur	x8, [x29, #-8]
	str	x8, [sp, #8]
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #656]
	ldr	x0, [sp, #8]
	bl	print_nftl_zone
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end218:
	.size	nftl_set_zone_test, .Lfunc_end218-nftl_set_zone_test
	.cfi_endproc
                                        // -- End function
	.globl	nftl_get_zone_priv              // -- Begin function nftl_get_zone_priv
	.p2align	2
	.type	nftl_get_zone_priv,@function
nftl_get_zone_priv:                     // @nftl_get_zone_priv
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	str	x8, [sp]
	ldr	x8, [sp]
	ldr	x0, [x8]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end219:
	.size	nftl_get_zone_priv, .Lfunc_end219-nftl_get_zone_priv
	.cfi_endproc
                                        // -- End function
	.globl	nftl_get_zone_write_cache_nums  // -- Begin function nftl_get_zone_write_cache_nums
	.p2align	2
	.type	nftl_get_zone_write_cache_nums,@function
nftl_get_zone_write_cache_nums:         // @nftl_get_zone_write_cache_nums
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	str	x8, [sp]
	ldr	x8, [sp]
	ldrh	w0, [x8, #1076]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end220:
	.size	nftl_get_zone_write_cache_nums, .Lfunc_end220-nftl_get_zone_write_cache_nums
	.cfi_endproc
                                        // -- End function
	.globl	nftl_start                      // -- Begin function nftl_start
	.p2align	2
	.type	nftl_start,@function
nftl_start:                             // @nftl_start
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160                    // =160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144                   // =144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	ldr	x10, [x29, #16]
	ldr	w9, [x29, #24]
	ldrb	w8, [x29, #32]
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	stur	x3, [x29, #-40]
	stur	x4, [x29, #-48]
	stur	w5, [x29, #-52]
	stur	w6, [x29, #-56]
	stur	w7, [x29, #-60]
	str	x10, [sp, #72]
	str	w9, [sp, #68]
	strb	w8, [sp, #67]
	str	wzr, [sp, #36]
	ldur	x8, [x29, #-40]
	lsr	x8, x8, #9
	stur	x8, [x29, #-40]
	ldur	x8, [x29, #-48]
	lsr	x8, x8, #9
	stur	x8, [x29, #-48]
	ldur	w8, [x29, #-52]
	lsr	w8, w8, #9
	stur	w8, [x29, #-52]
	ldur	w8, [x29, #-56]
	lsr	w8, w8, #9
	stur	w8, [x29, #-56]
	ldur	x8, [x29, #-40]
                                        // kill: def $w8 killed $w8 killed $x8
	str	w8, [sp, #20]
	ldur	x8, [x29, #-48]
                                        // kill: def $w8 killed $w8 killed $x8
	str	w8, [sp, #16]
	ldur	w8, [x29, #-52]
	str	w8, [sp, #12]
	ldur	w8, [x29, #-56]
	str	w8, [sp, #8]
	mov	w0, #3536
	bl	nftl_malloc
	ldur	x8, [x29, #-32]
	str	x0, [x8]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8]
	str	x8, [sp, #24]
	ldr	x8, [sp, #24]
	cbnz	x8, .LBB221_2
// %bb.1:
	adrp	x0, .L.str.339
	add	x0, x0, :lo12:.L.str.339
	adrp	x1, .L__func__.nftl_start
	add	x1, x1, :lo12:.L__func__.nftl_start
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_2:
.LBB221_3:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB221_10 Depth 2
	ldr	w8, [sp, #36]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #36]
	ldr	x0, [sp, #24]
	mov	w1, wzr
	mov	w2, #3536
	bl	nand_memset
	ldur	x8, [x29, #-16]
	ldr	x9, [sp, #24]
	str	x8, [x9]
	ldur	x8, [x29, #-24]
	ldr	x9, [sp, #24]
	str	x8, [x9, #3448]
	mov	w0, #48
	bl	nftl_malloc
	ldr	x8, [sp, #24]
	str	x0, [x8, #8]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	cbnz	x8, .LBB221_5
// %bb.4:
	adrp	x0, .L.str.340
	add	x0, x0, :lo12:.L.str.340
	adrp	x1, .L__func__.nftl_start
	add	x1, x1, :lo12:.L__func__.nftl_start
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_5:                              //   in Loop: Header=BB221_3 Depth=1
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #8]
	mov	w1, wzr
	mov	w2, #8
	str	w2, [sp, #4]                    // 4-byte Folded Spill
	bl	nand_memset
	ldr	w2, [sp, #4]                    // 4-byte Folded Reload
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #8]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	add	x1, x8, #16                     // =16
	bl	nand_memcpy
	ldr	w8, [sp, #8]
	lsl	w8, w8, #9
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	strh	w8, [x9, #26]
	ldur	w8, [x29, #-60]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	strb	w8, [x9, #10]
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #12]
	udiv	w8, w8, w9
	str	w8, [sp, #60]
	ldr	w8, [sp, #60]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #12]
	subs	w8, w8, w9
	b.hi	.LBB221_7
// %bb.6:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_7:                              //   in Loop: Header=BB221_3 Depth=1
	ldr	w8, [sp, #60]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	strh	w8, [x9, #24]
	ldr	w8, [sp, #12]
	ldr	w9, [sp, #8]
	udiv	w8, w8, w9
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	strh	w8, [x9, #20]
	ldr	w8, [sp, #8]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	strb	w8, [x9, #8]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #8]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	strh	w8, [x9, #28]
	ldr	x8, [sp, #24]
	ldr	x9, [x8, #8]
	mov	w8, #3000
	strh	w8, [x9, #16]
	adrp	x8, g_nsi
	ldr	x9, [x8, :lo12:g_nsi]
	ldr	x9, [x9, #8]
	ldr	w9, [x9, #256]
	ldr	x10, [sp, #24]
	ldr	x10, [x10, #8]
	strb	w9, [x10, #40]
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	ldr	w8, [x8, #260]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	str	w8, [x9, #44]
	ldr	w8, [sp, #60]
	str	w8, [sp, #56]
	ldr	w8, [sp, #56]
	add	w8, w8, #1                      // =1
	lsl	w0, w8, #6
	bl	nftl_malloc
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	str	x0, [x8, #32]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	cbnz	x8, .LBB221_9
// %bb.8:
	adrp	x0, .L.str.341
	add	x0, x0, :lo12:.L.str.341
	adrp	x1, .L__func__.nftl_start
	add	x1, x1, :lo12:.L__func__.nftl_start
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_9:                              //   in Loop: Header=BB221_3 Depth=1
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x0, [x8, #32]
	ldr	w8, [sp, #56]
                                        // kill: def $x8 killed $w8
                                        // kill: def $w8 killed $w8 killed $x8
	lsl	w2, w8, #6
	mov	w1, #-1
	bl	nand_memset
	str	wzr, [sp, #44]
.LBB221_10:                             //   Parent Loop BB221_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #44]
	ldr	w9, [sp, #56]
	add	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB221_13
// %bb.11:                              //   in Loop: Header=BB221_10 Depth=2
	ldr	w8, [sp, #44]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #8]
	ldr	x9, [x9, #32]
	ldr	w10, [sp, #44]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #6
	strh	w8, [x9]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	strh	wzr, [x8, #4]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	strh	wzr, [x8, #2]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9, lsl #6
	mov	w8, #-1
	str	w8, [x9, #8]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	strh	wzr, [x8, #6]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #16]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #24]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #32]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #40]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #48]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #8]
	ldr	x8, [x8, #32]
	ldr	w9, [sp, #44]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #6
	str	xzr, [x8, #56]
// %bb.12:                              //   in Loop: Header=BB221_10 Depth=2
	ldr	w8, [sp, #44]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #44]
	b	.LBB221_10
.LBB221_13:                             //   in Loop: Header=BB221_3 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x0, [x8]
	bl	nftl_ops_init
	ldr	w8, [sp, #20]
	ldr	w9, [sp, #12]
	udiv	w8, w8, w9
	str	w8, [sp, #52]
	ldr	w8, [sp, #60]
	ldr	w9, [sp, #52]
	subs	w8, w8, w9
	ldr	x9, [sp, #24]
	strh	w8, [x9, #30]
	ldr	x8, [sp, #24]
	ldrh	w8, [x8, #30]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #3448]
	ldrh	w9, [x9, #12]
	subs	w8, w8, w9
	b.ge	.LBB221_15
// %bb.14:
	ldr	x8, [sp, #24]
	ldrh	w1, [x8, #30]
	adrp	x0, .L.str.342
	add	x0, x0, :lo12:.L.str.342
	bl	nand_print
	mov	w8, #-2
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_15:                             //   in Loop: Header=BB221_3 Depth=1
	ldr	w1, [sp, #60]
	ldr	x8, [sp, #24]
	ldrh	w2, [x8, #30]
	adrp	x0, .L.str.343
	add	x0, x0, :lo12:.L.str.343
	bl	nand_print_dbg
	ldr	w8, [sp, #12]
	str	w8, [sp, #48]
	ldur	x8, [x29, #-32]
	ldr	x0, [x8]
	ldr	w8, [sp, #60]
	ldr	x9, [sp, #24]
	ldrh	w9, [x9, #30]
	subs	w8, w8, w9
	ldr	w9, [sp, #48]
	mul	w2, w8, w9
	ldr	x8, [sp, #24]
	ldrh	w8, [x8, #30]
	ldr	w9, [sp, #48]
	mul	w3, w8, w9
	mov	w1, wzr
	bl	zone_param_init
	str	w0, [sp, #40]
	ldr	w8, [sp, #40]
	cbz	w8, .LBB221_17
// %bb.16:
	adrp	x0, .L.str.344
	add	x0, x0, :lo12:.L.str.344
	bl	nand_print
	ldr	w8, [sp, #40]
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_17:                             //   in Loop: Header=BB221_3 Depth=1
	ldr	w8, [sp, #68]
	ldr	x9, [sp, #24]
	strb	w8, [x9, #24]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #26]
	cbz	w8, .LBB221_19
// %bb.18:                              //   in Loop: Header=BB221_3 Depth=1
	ldr	x9, [sp, #24]
	ldrb	w8, [x9, #25]
	orr	w8, w8, #0x1
	strb	w8, [x9, #25]
.LBB221_19:                             //   in Loop: Header=BB221_3 Depth=1
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #3448]
	ldrh	w8, [x8, #10]
	cbz	w8, .LBB221_25
// %bb.20:                              //   in Loop: Header=BB221_3 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x0, [x8]
	ldr	w1, [sp, #60]
	bl	is_no_use_device
	subs	w8, w0, #1                      // =1
	b.ne	.LBB221_24
// %bb.21:                              //   in Loop: Header=BB221_3 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x0, [x8]
	ldr	w1, [sp, #60]
	bl	build_zone_list_first
	str	w0, [sp, #40]
	ldr	w8, [sp, #40]
	cbz	w8, .LBB221_23
// %bb.22:
	adrp	x0, .L.str.345
	add	x0, x0, :lo12:.L.str.345
	bl	nand_print
	ldr	w8, [sp, #40]
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_23:                             //   in Loop: Header=BB221_3 Depth=1
.LBB221_24:                             //   in Loop: Header=BB221_3 Depth=1
.LBB221_25:                             //   in Loop: Header=BB221_3 Depth=1
	ldur	x8, [x29, #-32]
	ldr	x0, [x8]
	bl	build_zone_list
	str	w0, [sp, #40]
	ldr	w8, [sp, #40]
	cbz	w8, .LBB221_29
// %bb.26:                              //   in Loop: Header=BB221_3 Depth=1
	adrp	x0, .L.str.346
	add	x0, x0, :lo12:.L.str.346
	bl	nand_print
	ldr	w8, [sp, #36]
	subs	w8, w8, #2                      // =2
	b.hs	.LBB221_28
// %bb.27:                              //   in Loop: Header=BB221_3 Depth=1
	adrp	x0, .L.str.347
	add	x0, x0, :lo12:.L.str.347
	bl	nand_print
	b	.LBB221_3
.LBB221_28:
	ldr	w8, [sp, #40]
	stur	w8, [x29, #-4]
	b	.LBB221_30
.LBB221_29:
	ldur	x8, [x29, #-32]
	ldr	x0, [x8]
	bl	nftl_cache_init
	stur	wzr, [x29, #-4]
.LBB221_30:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160                    // =160
	ret
.Lfunc_end221:
	.size	nftl_start, .Lfunc_end221-nftl_start
	.cfi_endproc
                                        // -- End function
	.globl	nftl_zone_release               // -- Begin function nftl_zone_release
	.p2align	2
	.type	nftl_zone_release,@function
nftl_zone_release:                      // @nftl_zone_release
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	nftl_cache_exit
	ldr	x0, [sp, #8]
	bl	zone_param_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #8]
	ldr	x0, [x8, #32]
	bl	nftl_free
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #8]
	bl	nftl_free
	ldr	x0, [sp, #8]
	bl	nftl_free
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end222:
	.size	nftl_zone_release, .Lfunc_end222-nftl_zone_release
	.cfi_endproc
                                        // -- End function
	.globl	is_on_panic                     // -- Begin function is_on_panic
	.p2align	2
	.type	is_on_panic,@function
is_on_panic:                            // @is_on_panic
	.cfi_startproc
// %bb.0:
	adrp	x8, nand_on_panic
	ldr	w8, [x8, :lo12:nand_on_panic]
	subs	w8, w8, #0                      // =0
	cset	w8, ne
	and	w0, w8, #0x1
	ret
.Lfunc_end223:
	.size	is_on_panic, .Lfunc_end223-is_on_panic
	.cfi_endproc
                                        // -- End function
	.globl	panic_mark_enable               // -- Begin function panic_mark_enable
	.p2align	2
	.type	panic_mark_enable,@function
panic_mark_enable:                      // @panic_mark_enable
	.cfi_startproc
// %bb.0:
	adrp	x9, panic_enable
	mov	w8, #1
	str	w8, [x9, :lo12:panic_enable]
	ret
.Lfunc_end224:
	.size	panic_mark_enable, .Lfunc_end224-panic_mark_enable
	.cfi_endproc
                                        // -- End function
	.globl	is_panic_enable                 // -- Begin function is_panic_enable
	.p2align	2
	.type	is_panic_enable,@function
is_panic_enable:                        // @is_panic_enable
	.cfi_startproc
// %bb.0:
	adrp	x8, panic_enable
	ldr	w8, [x8, :lo12:panic_enable]
	subs	w8, w8, #0                      // =0
	cset	w8, ne
	and	w0, w8, #0x1
	ret
.Lfunc_end225:
	.size	is_panic_enable, .Lfunc_end225-is_panic_enable
	.cfi_endproc
                                        // -- End function
	.globl	panic_init_part                 // -- Begin function panic_init_part
	.p2align	2
	.type	panic_init_part,@function
panic_init_part:                        // @panic_init_part
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, panic_part
	add	x8, x8, :lo12:panic_part
	str	x8, [sp]                        // 8-byte Folded Spill
	str	x0, [sp, #16]
	str	w1, [sp, #12]
	str	w2, [sp, #8]
	ldr	x8, [sp, #16]
	cbz	x8, .LBB226_2
// %bb.1:
	ldr	w8, [sp, #8]
	cbnz	w8, .LBB226_3
.LBB226_2:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB226_6
.LBB226_3:
	ldr	x8, [sp]                        // 8-byte Folded Reload
	ldr	w8, [x8, #12]
	cbz	w8, .LBB226_5
// %bb.4:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB226_6
.LBB226_5:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	ldr	x8, [sp, #16]
	adrp	x10, panic_part
	str	x8, [x10, :lo12:panic_part]
	ldr	w8, [sp, #12]
	str	w8, [x9, #8]
	ldr	w8, [sp, #8]
	str	w8, [x9, #12]
	bl	panic_mark_enable
	ldr	x1, [sp, #16]
	ldr	w2, [sp, #12]
	ldr	w3, [sp, #8]
	adrp	x0, .L.str.348
	add	x0, x0, :lo12:.L.str.348
	bl	nand_print
	stur	wzr, [x29, #-4]
.LBB226_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end226:
	.size	panic_init_part, .Lfunc_end226-panic_init_part
	.cfi_endproc
                                        // -- End function
	.globl	panic_read                      // -- Begin function panic_read
	.p2align	2
	.type	panic_read,@function
panic_read:                             // @panic_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	str	w2, [sp, #24]
	str	x3, [sp, #16]
	ldr	x8, [sp, #16]
	str	x8, [sp, #8]
	bl	is_panic_enable
	cbnz	w0, .LBB227_2
// %bb.1:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB227_8
.LBB227_2:
	bl	panic_mark_panic
.LBB227_3:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #24]
	subs	w8, w8, #8, lsl #12             // =32768
	b.lo	.LBB227_5
// %bb.4:                               //   in Loop: Header=BB227_3 Depth=1
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #72]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	x3, [sp, #8]
	mov	w2, #32768
	blr	x8
	ldr	x8, [sp, #8]
	add	x8, x8, #8, lsl #12             // =32768
	str	x8, [sp, #8]
	ldur	w8, [x29, #-20]
	add	w8, w8, #8, lsl #12             // =32768
	stur	w8, [x29, #-20]
	ldr	w8, [sp, #24]
	subs	w8, w8, #8, lsl #12             // =32768
	str	w8, [sp, #24]
	b	.LBB227_3
.LBB227_5:
	ldr	w8, [sp, #24]
	cbz	w8, .LBB227_7
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #72]
	ldur	x0, [x29, #-16]
	ldur	w1, [x29, #-20]
	ldr	w2, [sp, #24]
	ldr	x3, [sp, #8]
	blr	x8
.LBB227_7:
	stur	wzr, [x29, #-4]
.LBB227_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end227:
	.size	panic_read, .Lfunc_end227-panic_read
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function panic_mark_panic
	.type	panic_mark_panic,@function
panic_mark_panic:                       // @panic_mark_panic
	.cfi_startproc
// %bb.0:
	adrp	x9, nand_on_panic
	mov	w8, #1
	str	w8, [x9, :lo12:nand_on_panic]
	ret
.Lfunc_end228:
	.size	panic_mark_panic, .Lfunc_end228-panic_mark_panic
	.cfi_endproc
                                        // -- End function
	.globl	panic_write                     // -- Begin function panic_write
	.p2align	2
	.type	panic_write,@function
panic_write:                            // @panic_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #208                    // =208
	stp	x29, x30, [sp, #192]            // 16-byte Folded Spill
	add	x29, sp, #192                   // =192
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	stur	w2, [x29, #-24]
	stur	x3, [x29, #-32]
	adrp	x8, panic_info
	add	x8, x8, :lo12:panic_info
	str	x8, [sp, #88]
	bl	is_panic_enable
	cbnz	w0, .LBB229_2
// %bb.1:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB229_30
.LBB229_2:
	bl	panic_mark_panic
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #24]
	stur	x8, [x29, #-80]
	ldur	x8, [x29, #-80]
	ldr	x8, [x8, #8]
	stur	x8, [x29, #-88]
	ldur	w8, [x29, #-24]
	ldur	x9, [x29, #-88]
	ldrb	w9, [x9, #8]
	subs	w9, w9, #1                      // =1
	add	w8, w8, w9
	ldur	x9, [x29, #-88]
	ldrb	w9, [x9, #8]
	udiv	w8, w8, w9
	str	w8, [sp, #76]
	ldr	w8, [sp, #76]
	ldur	x9, [x29, #-88]
	ldrh	w9, [x9, #20]
	subs	w9, w9, #1                      // =1
	add	w8, w8, w9
	ldur	x9, [x29, #-88]
	ldrh	w9, [x9, #20]
	udiv	w8, w8, w9
	str	w8, [sp, #72]
	ldur	w1, [x29, #-20]
	ldur	w2, [x29, #-24]
	ldr	w3, [sp, #76]
	ldr	w4, [sp, #72]
	adrp	x0, .L.str.352
	add	x0, x0, :lo12:.L.str.352
	bl	nand_print
	ldr	w8, [sp, #76]
	ldr	x9, [sp, #88]
	ldr	w9, [x9, #24]
	subs	w9, w9, #3                      // =3
	subs	w8, w8, w9
	b.ls	.LBB229_4
// %bb.3:
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #24]
	subs	w8, w8, #3                      // =3
	ldur	x9, [x29, #-88]
	ldrb	w9, [x9, #8]
	mul	w1, w8, w9
	adrp	x0, .L.str.353
	add	x0, x0, :lo12:.L.str.353
	bl	nand_print
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB229_30
.LBB229_4:
	sub	x0, x29, #48                    // =48
	mov	w1, #-91
	mov	w2, #16
	bl	nand_memset
	ldur	x8, [x29, #-32]
	str	x8, [sp, #80]
	ldr	x8, [sp, #88]
	ldr	x9, [sp, #88]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	str	x8, [sp, #96]
.LBB229_5:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #76]
	subs	w8, w8, #0                      // =0
	b.ls	.LBB229_20
// %bb.6:                               //   in Loop: Header=BB229_5 Depth=1
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-88]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.lo	.LBB229_8
// %bb.7:                               //   in Loop: Header=BB229_5 Depth=1
	ldr	x9, [sp, #88]
	ldr	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #28]
	ldr	x8, [sp, #88]
	ldr	x9, [sp, #88]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	str	x8, [sp, #96]
	ldr	x8, [sp, #88]
	str	wzr, [x8, #32]
.LBB229_8:                              //   in Loop: Header=BB229_5 Depth=1
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #32]
	cbnz	w8, .LBB229_10
// %bb.9:                               //   in Loop: Header=BB229_5 Depth=1
	ldr	x8, [sp, #96]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-88]
	ldrh	w3, [x8, #28]
	sub	x0, x29, #72                    // =72
	str	x0, [sp, #56]                   // 8-byte Folded Spill
	mov	w2, wzr
	mov	x5, xzr
	mov	x4, x5
	bl	set_physic_op_par
	ldr	x1, [sp, #56]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-80]
	ldr	x8, [x8, #3464]
	ldur	x0, [x29, #-80]
	blr	x8
	ldr	x9, [sp, #96]
	ldrh	w8, [x9, #6]
	add	w8, w8, #1                      // =1
	strh	w8, [x9, #6]
	ldr	x8, [sp, #96]
	strh	wzr, [x8, #4]
.LBB229_10:                             //   in Loop: Header=BB229_5 Depth=1
	ldr	x8, [sp, #96]
	ldrh	w1, [x8]
	ldr	x8, [sp, #88]
	ldr	w2, [x8, #32]
	adrp	x0, .L.str.354
	add	x0, x0, :lo12:.L.str.354
	bl	nand_print
	sub	x0, x29, #48                    // =48
	str	x0, [sp, #48]                   // 8-byte Folded Spill
	mov	w8, #255
	sturb	w8, [x29, #-48]
	mov	w8, #170
	sturb	w8, [x29, #-47]
	mov	w8, #99
	sturb	w8, [x29, #-46]
	ldur	x8, [x29, #-80]
	ldrb	w8, [x8, #24]
	sturb	w8, [x29, #-45]
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #28]
	sturb	w8, [x29, #-44]
	ldr	x8, [sp, #96]
	ldrh	w1, [x8, #6]
	mov	w2, #5
	mov	w3, #2
	bl	set_spare_data
	ldr	x0, [sp, #48]                   // 8-byte Folded Reload
	ldur	w1, [x29, #-20]
	mov	w2, #7
	mov	w3, #4
	bl	set_spare_data
	ldur	w8, [x29, #-24]
	ldur	x9, [x29, #-88]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.ls	.LBB229_12
// %bb.11:                              //   in Loop: Header=BB229_5 Depth=1
	ldur	x8, [x29, #-88]
	ldrb	w8, [x8, #8]
	str	w8, [sp, #44]                   // 4-byte Folded Spill
	b	.LBB229_13
.LBB229_12:                             //   in Loop: Header=BB229_5 Depth=1
	ldur	w8, [x29, #-24]
	str	w8, [sp, #44]                   // 4-byte Folded Spill
.LBB229_13:                             //   in Loop: Header=BB229_5 Depth=1
	ldr	w1, [sp, #44]                   // 4-byte Folded Reload
	sub	x0, x29, #48                    // =48
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	mov	w2, #11
	mov	w3, #4
	bl	set_spare_data
	ldr	x5, [sp, #24]                   // 8-byte Folded Reload
	ldr	x8, [sp, #96]
	ldrh	w1, [x8]
	ldr	x8, [sp, #88]
	ldr	w2, [x8, #32]
	ldur	x8, [x29, #-88]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #80]
	sub	x0, x29, #72                    // =72
	str	x0, [sp, #32]                   // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #32]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-80]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-80]
	blr	x8
	ldur	x8, [x29, #-88]
	ldrh	w9, [x8, #26]
	ldr	x8, [sp, #80]
	add	x8, x8, w9, sxtw
	str	x8, [sp, #80]
	ldur	w8, [x29, #-24]
	ldur	x9, [x29, #-88]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.ls	.LBB229_15
// %bb.14:                              //   in Loop: Header=BB229_5 Depth=1
	ldur	x8, [x29, #-88]
	ldrb	w8, [x8, #8]
	str	w8, [sp, #20]                   // 4-byte Folded Spill
	b	.LBB229_16
.LBB229_15:                             //   in Loop: Header=BB229_5 Depth=1
	ldur	w8, [x29, #-24]
	str	w8, [sp, #20]                   // 4-byte Folded Spill
.LBB229_16:                             //   in Loop: Header=BB229_5 Depth=1
	ldr	w9, [sp, #20]                   // 4-byte Folded Reload
	ldur	w8, [x29, #-20]
	add	w8, w8, w9
	stur	w8, [x29, #-20]
	ldur	w8, [x29, #-24]
	ldur	x9, [x29, #-88]
	ldrb	w9, [x9, #8]
	subs	w8, w8, w9
	b.ls	.LBB229_18
// %bb.17:                              //   in Loop: Header=BB229_5 Depth=1
	ldur	x8, [x29, #-88]
	ldrb	w8, [x8, #8]
	str	w8, [sp, #16]                   // 4-byte Folded Spill
	b	.LBB229_19
.LBB229_18:                             //   in Loop: Header=BB229_5 Depth=1
	ldur	w8, [x29, #-24]
	str	w8, [sp, #16]                   // 4-byte Folded Spill
.LBB229_19:                             //   in Loop: Header=BB229_5 Depth=1
	ldr	w9, [sp, #16]                   // 4-byte Folded Reload
	ldur	w8, [x29, #-24]
	subs	w8, w8, w9
	stur	w8, [x29, #-24]
	ldr	x9, [sp, #88]
	ldr	w8, [x9, #24]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #24]
	ldr	x9, [sp, #88]
	ldr	w8, [x9, #32]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #32]
	ldr	w8, [sp, #76]
	subs	w8, w8, #1                      // =1
	str	w8, [sp, #76]
	b	.LBB229_5
.LBB229_20:
	mov	w8, #98
	sturb	w8, [x29, #-46]
	str	wzr, [sp, #68]
.LBB229_21:                             // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #68]
	subs	w8, w8, #3                      // =3
	b.hs	.LBB229_29
// %bb.22:                              //   in Loop: Header=BB229_21 Depth=1
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #32]
	cbz	w8, .LBB229_24
// %bb.23:                              //   in Loop: Header=BB229_21 Depth=1
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-88]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.lo	.LBB229_25
.LBB229_24:
	b	.LBB229_29
.LBB229_25:                             //   in Loop: Header=BB229_21 Depth=1
	ldr	x8, [sp, #96]
	ldrh	w1, [x8]
	ldr	x8, [sp, #88]
	ldr	w2, [x8, #32]
	adrp	x0, .L.str.355
	add	x0, x0, :lo12:.L.str.355
	bl	nand_print
	ldr	x8, [sp, #96]
	ldrh	w1, [x8]
	ldr	x8, [sp, #88]
	ldr	w2, [x8, #32]
	ldur	x8, [x29, #-88]
	ldrh	w3, [x8, #28]
	ldr	x4, [sp, #80]
	sub	x0, x29, #72                    // =72
	str	x0, [sp, #8]                    // 8-byte Folded Spill
	sub	x5, x29, #48                    // =48
	bl	set_physic_op_par
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x8, [x29, #-80]
	ldr	x8, [x8, #3480]
	ldur	x0, [x29, #-80]
	blr	x8
	ldr	x9, [sp, #88]
	ldr	w8, [x9, #32]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #32]
	ldr	x9, [sp, #88]
	ldr	w8, [x9, #24]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #24]
	ldr	x8, [sp, #88]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-88]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.lo	.LBB229_27
// %bb.26:                              //   in Loop: Header=BB229_21 Depth=1
	ldr	x9, [sp, #88]
	ldr	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #28]
	ldr	x8, [sp, #88]
	ldr	x9, [sp, #88]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	str	x8, [sp, #96]
	ldr	x8, [sp, #88]
	str	wzr, [x8, #32]
.LBB229_27:                             //   in Loop: Header=BB229_21 Depth=1
// %bb.28:                              //   in Loop: Header=BB229_21 Depth=1
	ldr	w8, [sp, #68]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #68]
	b	.LBB229_21
.LBB229_29:
	stur	wzr, [x29, #-4]
.LBB229_30:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #192]            // 16-byte Folded Reload
	add	sp, sp, #208                    // =208
	ret
.Lfunc_end229:
	.size	panic_write, .Lfunc_end229-panic_write
	.cfi_endproc
                                        // -- End function
	.globl	is_panic_dummy_page             // -- Begin function is_panic_dummy_page
	.p2align	2
	.type	is_panic_dummy_page,@function
is_panic_dummy_page:                    // @is_panic_dummy_page
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldrb	w9, [x8, #1]
	mov	w8, #0
	subs	w9, w9, #170                    // =170
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	b.ne	.LBB230_2
// %bb.1:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #2]
	subs	w8, w8, #98                     // =98
	cset	w8, eq
	str	w8, [sp, #4]                    // 4-byte Folded Spill
.LBB230_2:
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	and	w0, w8, #0x1
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end230:
	.size	is_panic_dummy_page, .Lfunc_end230-is_panic_dummy_page
	.cfi_endproc
                                        // -- End function
	.globl	panic_recover_data              // -- Begin function panic_recover_data
	.p2align	2
	.type	panic_recover_data,@function
panic_recover_data:                     // @panic_recover_data
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160                    // =160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144                   // =144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #24]
	stur	x8, [x29, #-24]
	ldur	x8, [x29, #-24]
	ldr	x8, [x8, #8]
	stur	x8, [x29, #-32]
	adrp	x8, panic_info
	add	x8, x8, :lo12:panic_info
	stur	x8, [x29, #-40]
	stur	xzr, [x29, #-56]
	ldur	x8, [x29, #-40]
	ldr	x8, [x8]
	cbnz	x8, .LBB231_2
// %bb.1:
	adrp	x0, .L.str.358
	add	x0, x0, :lo12:.L.str.358
	bl	nand_print
	stur	wzr, [x29, #-4]
	b	.LBB231_26
.LBB231_2:
	adrp	x0, .L.str.359
	add	x0, x0, :lo12:.L.str.359
	bl	nand_print
	ldur	x8, [x29, #-32]
	ldrh	w0, [x8, #26]
	bl	nand_malloc
	stur	x0, [x29, #-56]
	ldur	x8, [x29, #-56]
	cbnz	x8, .LBB231_4
// %bb.3:
	adrp	x0, .L.str.360
	add	x0, x0, :lo12:.L.str.360
	adrp	x1, .L__func__.panic_recover_data
	add	x1, x1, :lo12:.L__func__.panic_recover_data
	mov	w2, #308
	bl	nand_print
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB231_26
.LBB231_4:
	ldur	x0, [x29, #-56]
	ldur	x8, [x29, #-32]
	ldrh	w2, [x8, #26]
	mov	w1, wzr
	bl	nand_memset
	ldur	x8, [x29, #-40]
	str	wzr, [x8, #28]
.LBB231_5:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB231_7 Depth 2
	ldur	x8, [x29, #-40]
	ldr	w8, [x8, #28]
	subs	w8, w8, #3                      // =3
	b.hs	.LBB231_19
// %bb.6:                               //   in Loop: Header=BB231_5 Depth=1
	ldur	x8, [x29, #-40]
	ldur	x9, [x29, #-40]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	stur	x8, [x29, #-48]
	ldur	x8, [x29, #-40]
	str	wzr, [x8, #32]
.LBB231_7:                              //   Parent Loop BB231_5 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	x8, [x29, #-40]
	ldr	w8, [x8, #32]
	ldur	x9, [x29, #-32]
	ldrh	w9, [x9, #20]
	subs	w8, w8, w9
	b.hs	.LBB231_17
// %bb.8:                               //   in Loop: Header=BB231_7 Depth=2
	ldur	x8, [x29, #-48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-40]
	ldr	w2, [x8, #32]
	ldur	x8, [x29, #-32]
	ldrh	w3, [x8, #28]
	ldur	x4, [x29, #-56]
	add	x0, sp, #64                     // =64
	str	x0, [sp, #24]                   // 8-byte Folded Spill
	add	x5, sp, #48                     // =48
	str	x5, [sp, #32]                   // 8-byte Folded Spill
	bl	set_physic_op_par
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldur	x8, [x29, #-24]
	ldr	x8, [x8, #3472]
	ldur	x0, [x29, #-24]
	blr	x8
	ldr	x0, [sp, #32]                   // 8-byte Folded Reload
	bl	is_panic_dummy_page
	cbz	w0, .LBB231_10
// %bb.9:                               //   in Loop: Header=BB231_7 Depth=2
	ldur	x8, [x29, #-48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-40]
	ldr	w2, [x8, #32]
	adrp	x0, .L.str.361
	add	x0, x0, :lo12:.L.str.361
	bl	nand_print
	b	.LBB231_16
.LBB231_10:                             //   in Loop: Header=BB231_7 Depth=2
	add	x0, sp, #48                     // =48
	bl	is_panic_data_page
	cbnz	w0, .LBB231_15
// %bb.11:
	ldur	x8, [x29, #-40]
	ldr	w8, [x8, #28]
	cbnz	w8, .LBB231_14
// %bb.12:
	ldur	x8, [x29, #-40]
	ldr	w8, [x8, #32]
	cbnz	w8, .LBB231_14
// %bb.13:
	ldur	x8, [x29, #-48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-40]
	ldr	w2, [x8, #32]
	adrp	x0, .L.str.362
	add	x0, x0, :lo12:.L.str.362
	bl	nand_print
	b	.LBB231_25
.LBB231_14:
	ldur	x8, [x29, #-48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-40]
	ldr	w2, [x8, #32]
	adrp	x0, .L.str.363
	add	x0, x0, :lo12:.L.str.363
	bl	nand_print
	b	.LBB231_20
.LBB231_15:                             //   in Loop: Header=BB231_7 Depth=2
	add	x0, sp, #48                     // =48
	str	x0, [sp, #16]                   // 8-byte Folded Spill
	mov	w1, #7
	mov	w2, #4
	str	w2, [sp, #12]                   // 4-byte Folded Spill
	bl	get_spare_data
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	mov	w8, w0
	ldr	x0, [sp, #16]                   // 8-byte Folded Reload
	str	w8, [sp, #44]
	mov	w1, #11
	bl	get_spare_data
	str	w0, [sp, #40]
	ldur	x8, [x29, #-48]
	ldrh	w1, [x8]
	ldur	x8, [x29, #-40]
	ldr	w2, [x8, #32]
	ldr	w3, [sp, #44]
	adrp	x0, .L.str.364
	add	x0, x0, :lo12:.L.str.364
	bl	nand_print
	ldr	w0, [sp, #44]
	ldr	w1, [sp, #40]
	ldur	x2, [x29, #-56]
	bl	nftl_write
.LBB231_16:                             //   in Loop: Header=BB231_7 Depth=2
	ldur	x9, [x29, #-40]
	ldr	w8, [x9, #32]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #32]
	b	.LBB231_7
.LBB231_17:                             //   in Loop: Header=BB231_5 Depth=1
// %bb.18:                              //   in Loop: Header=BB231_5 Depth=1
	ldur	x9, [x29, #-40]
	ldr	w8, [x9, #28]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #28]
	b	.LBB231_5
.LBB231_19:
.LBB231_20:
.LBB231_21:                             // =>This Inner Loop Header: Depth=1
	ldur	x8, [x29, #-40]
	ldr	w8, [x8, #28]
	tbnz	w8, #31, .LBB231_24
// %bb.22:                              //   in Loop: Header=BB231_21 Depth=1
	ldur	x8, [x29, #-40]
	ldur	x9, [x29, #-40]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	x8, [x8, x9, lsl #3]
	stur	x8, [x29, #-48]
	ldur	x0, [x29, #-24]
	ldur	x1, [x29, #-48]
	ldur	x8, [x29, #-48]
	ldrh	w8, [x8, #6]
	add	w2, w8, #1                      // =1
	bl	erase_block
// %bb.23:                              //   in Loop: Header=BB231_21 Depth=1
	ldur	x9, [x29, #-40]
	ldr	w8, [x9, #28]
	subs	w8, w8, #1                      // =1
	str	w8, [x9, #28]
	b	.LBB231_21
.LBB231_24:
.LBB231_25:
	ldur	x0, [x29, #-56]
	bl	nand_free
	stur	wzr, [x29, #-4]
.LBB231_26:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160                    // =160
	ret
.Lfunc_end231:
	.size	panic_recover_data, .Lfunc_end231-panic_recover_data
	.cfi_endproc
                                        // -- End function
	.globl	nftl_write                      // -- Begin function nftl_write
	.p2align	2
	.type	nftl_write,@function
nftl_write:                             // @nftl_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	stur	w0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	x2, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	ldur	w8, [x29, #-12]
	cbnz	w8, .LBB232_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB232_8
.LBB232_2:
.LBB232_3:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-8]
	ldr	x9, [sp]
	ldr	w9, [x9]
	subs	w8, w8, w9
	b.lo	.LBB232_7
// %bb.4:                               //   in Loop: Header=BB232_3 Depth=1
	ldr	x8, [sp]
	ldr	w9, [x8]
	ldur	w8, [x29, #-8]
	subs	w8, w8, w9
	stur	w8, [x29, #-8]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB232_6
// %bb.5:
	ldur	w1, [x29, #-8]
	ldur	w2, [x29, #-12]
	adrp	x0, .L.str.370
	add	x0, x0, :lo12:.L.str.370
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB232_8
.LBB232_6:                              //   in Loop: Header=BB232_3 Depth=1
	b	.LBB232_3
.LBB232_7:
	ldr	x8, [sp]
	ldr	x8, [x8, #80]
	ldr	x0, [sp]
	ldur	w1, [x29, #-8]
	ldur	w2, [x29, #-12]
	ldr	x3, [sp, #8]
	blr	x8
	stur	w0, [x29, #-4]
.LBB232_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end232:
	.size	nftl_write, .Lfunc_end232-nftl_write
	.cfi_endproc
                                        // -- End function
	.globl	nftl_build_all                  // -- Begin function nftl_build_all
	.p2align	2
	.type	nftl_build_all,@function
nftl_build_all:                         // @nftl_build_all
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	str	x0, [sp, #16]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #72]
	str	x9, [sp, #8]
	str	xzr, [x8, #16]
.LBB233_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB233_5
// %bb.2:                               //   in Loop: Header=BB233_1 Depth=1
	ldr	x0, [sp, #8]
	bl	nftl_add
	cbnz	x0, .LBB233_4
// %bb.3:
	adrp	x0, .L.str.365
	add	x0, x0, :lo12:.L.str.365
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB233_6
.LBB233_4:                              //   in Loop: Header=BB233_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #64]
	str	x8, [sp, #8]
	b	.LBB233_1
.LBB233_5:
	stur	wzr, [x29, #-4]
.LBB233_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end233:
	.size	nftl_build_all, .Lfunc_end233-nftl_build_all
	.cfi_endproc
                                        // -- End function
	.globl	nftl_add                        // -- Begin function nftl_add
	.p2align	2
	.type	nftl_add,@function
nftl_add:                               // @nftl_add
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	mov	w0, #136
	bl	nftl_malloc
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB234_2
// %bb.1:
	adrp	x0, .L.str.366
	add	x0, x0, :lo12:.L.str.366
	adrp	x1, .L__func__.nftl_add
	add	x1, x1, :lo12:.L__func__.nftl_add
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB234_5
.LBB234_2:
	ldr	x0, [sp, #16]
	bl	build_nand_partition
	ldr	x8, [sp, #8]
	str	x0, [x8, #8]
	ldr	x0, [sp, #8]
	ldr	x8, [sp, #16]
	ldrh	w1, [x8]
	bl	nftl_initialize
	cbz	w0, .LBB234_4
// %bb.3:
	adrp	x0, .L.str.367
	add	x0, x0, :lo12:.L.str.367
	bl	nand_print
	stur	xzr, [x29, #-8]
	b	.LBB234_5
.LBB234_4:
	ldr	x1, [sp, #8]
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	bl	add_nftl_blk_list
	ldr	x0, [sp, #8]
	bl	panic_recover_data
	adrp	x0, .L.str.368
	add	x0, x0, :lo12:.L.str.368
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	stur	x8, [x29, #-8]
.LBB234_5:
	ldur	x0, [x29, #-8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end234:
	.size	nftl_add, .Lfunc_end234-nftl_add
	.cfi_endproc
                                        // -- End function
	.globl	nftl_build_one                  // -- Begin function nftl_build_one
	.p2align	2
	.type	nftl_build_one,@function
nftl_build_one:                         // @nftl_build_one
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	str	x8, [sp, #8]                    // 8-byte Folded Spill
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #72]
	str	x8, [sp, #16]
	ldur	w8, [x29, #-20]
	cbnz	w8, .LBB235_2
// %bb.1:
	ldr	x8, [sp, #8]                    // 8-byte Folded Reload
	str	xzr, [x8, #16]
.LBB235_2:
.LBB235_3:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	cbz	x8, .LBB235_9
// %bb.4:                               //   in Loop: Header=BB235_3 Depth=1
	ldr	x8, [sp, #16]
	ldrh	w8, [x8]
	ldur	w9, [x29, #-20]
	subs	w8, w8, w9
	b.ne	.LBB235_8
// %bb.5:                               //   in Loop: Header=BB235_3 Depth=1
	ldr	x0, [sp, #16]
	bl	nftl_add
	cbnz	x0, .LBB235_7
// %bb.6:
	adrp	x0, .L.str.365
	add	x0, x0, :lo12:.L.str.365
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB235_10
.LBB235_7:                              //   in Loop: Header=BB235_3 Depth=1
.LBB235_8:                              //   in Loop: Header=BB235_3 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #64]
	str	x8, [sp, #16]
	b	.LBB235_3
.LBB235_9:
	stur	wzr, [x29, #-4]
.LBB235_10:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end235:
	.size	nftl_build_one, .Lfunc_end235-nftl_build_one
	.cfi_endproc
                                        // -- End function
	.globl	nftl_initialize                 // -- Begin function nftl_initialize
	.p2align	2
	.type	nftl_initialize,@function
nftl_initialize:                        // @nftl_initialize
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #112                    // =112
	stp	x29, x30, [sp, #96]             // 16-byte Folded Spill
	add	x29, sp, #96                    // =96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #8]
	stur	x8, [x29, #-32]
// %bb.1:
	mov	x0, xzr
	str	xzr, [sp, #48]
	adrp	x8, .L__const.nftl_initialize.manu
	add	x8, x8, :lo12:.L__const.nftl_initialize.manu
	ldr	x9, [x8]
	str	x9, [sp, #32]
	ldrb	w8, [x8, #8]
	strb	w8, [sp, #40]
	adrp	x8, .L__const.nftl_initialize.of_root
	add	x8, x8, :lo12:.L__const.nftl_initialize.of_root
	ldrh	w8, [x8]
	add	x1, sp, #30                     // =30
	strh	w8, [sp, #30]
	adrp	x2, com
	add	x2, x2, :lo12:com
	bl	nand_fdt_get_prop_string_by_path
	str	x0, [sp, #48]
	ldr	x8, [sp, #48]
	cbnz	x8, .LBB236_3
// %bb.2:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB236_32
.LBB236_3:
	ldr	x0, [sp, #48]
	add	x1, sp, #32                     // =32
	mov	x2, #9
	bl	nand_memcmp
	cbz	w0, .LBB236_5
// %bb.4:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB236_32
.LBB236_5:
// %bb.6:
	bl	nftl_version
	adrp	x0, phyinfo
	add	x0, x0, :lo12:phyinfo
	bl	nftl_get_nand_phyinfo
	ldur	x8, [x29, #-32]
	ldrh	w8, [x8, #34]
	subs	w8, w8, #16                     // =16
	b.ge	.LBB236_8
// %bb.7:
	mov	w8, #-1
	stur	w8, [x29, #-4]
	b	.LBB236_32
.LBB236_8:
	mov	w0, #36
	bl	nftl_malloc
	ldur	x8, [x29, #-16]
	str	x0, [x8, #32]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	cbnz	x8, .LBB236_10
// %bb.9:
	adrp	x0, .L.str.377
	add	x0, x0, :lo12:.L.str.377
	adrp	x1, .L__func__.nftl_initialize
	add	x1, x1, :lo12:.L__func__.nftl_initialize
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB236_32
.LBB236_10:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	strh	wzr, [x8]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #1
	strh	w8, [x9, #2]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #4]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #6]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	strh	wzr, [x8, #8]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	strh	wzr, [x8, #10]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2498]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #12]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2500]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #16]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #8
	strh	w8, [x9, #14]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #2
	strh	w8, [x9, #18]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #3
	strh	w8, [x9, #20]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #50816
	movk	w8, #19, lsl #16
	str	w8, [x9, #32]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2494]
	cbnz	w8, .LBB236_12
// %bb.11:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #30
	strh	w8, [x9, #22]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #3
	strh	w8, [x9, #24]
	b	.LBB236_19
.LBB236_12:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2494]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB236_14
// %bb.13:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #1
	strh	w8, [x9]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #50
	strh	w8, [x9, #22]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #40
	strh	w8, [x9, #24]
	b	.LBB236_18
.LBB236_14:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2494]
	subs	w8, w8, #10                     // =10
	b.le	.LBB236_16
// %bb.15:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x9, [x8, #40]
	mov	w8, #10
	strh	w8, [x9, #2494]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #30
	strh	w8, [x9, #22]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #3
	strh	w8, [x9, #24]
	b	.LBB236_17
.LBB236_16:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2494]
	mov	w9, #50
	mul	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #22]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldrh	w8, [x8, #2494]
	mov	w9, #40
	mul	w8, w8, w9
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #24]
.LBB236_17:
.LBB236_18:
.LBB236_19:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldrh	w8, [x8, #2]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	strh	w8, [x9, #26]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	ldrh	w8, [x8, #26]
	cbz	w8, .LBB236_21
// %bb.20:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #3
	strh	w8, [x9, #24]
	b	.LBB236_22
.LBB236_21:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #1
	strh	w8, [x9, #24]
.LBB236_22:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	ldrh	w8, [x8, #22]
	subs	w8, w8, #30                     // =30
	b.le	.LBB236_24
// %bb.23:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #30
	strh	w8, [x9, #22]
.LBB236_24:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #1
	strh	w8, [x9, #24]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldr	w8, [x8, #2512]
	mov	w9, #45752
	movk	w9, #118, lsl #16
	subs	w8, w8, w9
	b.hi	.LBB236_27
// %bb.25:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldr	w8, [x8, #2512]
	mov	w9, #20864
	movk	w9, #1, lsl #16
	subs	w8, w8, w9
	b.lo	.LBB236_27
// %bb.26:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldr	w8, [x8, #2512]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #32]
	str	w8, [x9, #32]
.LBB236_27:
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #32]
	mov	w8, #1
	strh	w8, [x9, #28]
	bl	nftl_support_boot_check_crc_from_dts
	cbnz	w0, .LBB236_29
// %bb.28:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #32]
	strh	wzr, [x8, #28]
.LBB236_29:
	ldur	x0, [x29, #-16]
	ldur	x8, [x29, #-16]
	ldr	x1, [x8, #32]
	ldur	x8, [x29, #-16]
	add	x2, x8, #24                     // =24
	ldur	x8, [x29, #-32]
	ldr	x3, [x8, #64]
	ldur	x8, [x29, #-32]
	ldr	x4, [x8, #72]
	ldur	x8, [x29, #-32]
	ldr	w5, [x8, #44]
	ldur	x8, [x29, #-32]
	ldrh	w6, [x8, #40]
	ldur	x8, [x29, #-32]
	ldrh	w7, [x8, #34]
	ldur	x10, [x29, #-32]
	ldur	w9, [x29, #-20]
	mov	x8, sp
	str	x10, [x8]
	str	w9, [x8, #8]
	strb	wzr, [x8, #16]
	bl	nftl_start
	stur	w0, [x29, #-36]
	ldur	w8, [x29, #-36]
	cbz	w8, .LBB236_31
// %bb.30:
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
	b	.LBB236_32
.LBB236_31:
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldr	w8, [x8, #2504]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #24]
	ldr	x9, [x9, #8]
	strh	w8, [x9, #16]
	ldur	x8, [x29, #-32]
	ldr	x8, [x8, #80]
	ldr	x8, [x8, #40]
	ldr	w8, [x8, #2508]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #24]
	ldr	x9, [x9, #8]
	strh	w8, [x9, #30]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #8]
	ldrh	w1, [x8, #16]
	adrp	x0, .L.str.378
	add	x0, x0, :lo12:.L.str.378
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #24]
	bl	nftl_get_zone_cap
	ldur	x8, [x29, #-16]
	str	w0, [x8]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_read
	add	x8, x8, :lo12:_nand_read
	str	x8, [x9, #72]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_write
	add	x8, x8, :lo12:_nand_write
	str	x8, [x9, #80]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_discard
	add	x8, x8, :lo12:_nand_discard
	str	x8, [x9, #96]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_flush_write_cache
	add	x8, x8, :lo12:_nand_flush_write_cache
	str	x8, [x9, #88]
	ldur	x9, [x29, #-16]
	adrp	x8, _shutdown_op
	add	x8, x8, :lo12:_shutdown_op
	str	x8, [x9, #104]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_sector_read
	add	x8, x8, :lo12:_nand_sector_read
	str	x8, [x9, #112]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_sector_write
	add	x8, x8, :lo12:_nand_sector_write
	str	x8, [x9, #120]
	ldur	x9, [x29, #-16]
	adrp	x8, _nand_flush_sector_write_cache
	add	x8, x8, :lo12:_nand_flush_sector_write_cache
	str	x8, [x9, #128]
	ldur	x8, [x29, #-16]
	str	xzr, [x8, #16]
	ldur	x8, [x29, #-16]
	ldr	x9, [x8, #24]
	mov	w8, #1
	str	w8, [x9, #3456]
	stur	wzr, [x29, #-4]
.LBB236_32:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #96]             // 16-byte Folded Reload
	add	sp, sp, #112                    // =112
	ret
.Lfunc_end236:
	.size	nftl_initialize, .Lfunc_end236-nftl_initialize
	.cfi_endproc
                                        // -- End function
	.globl	add_nftl_blk_list               // -- Begin function add_nftl_blk_list
	.p2align	2
	.type	add_nftl_blk_list,@function
add_nftl_blk_list:                      // @add_nftl_blk_list
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	ldr	x8, [sp, #24]
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	str	xzr, [x8, #16]
.LBB237_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB237_3
// %bb.2:                               //   in Loop: Header=BB237_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB237_1
.LBB237_3:
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #8]
	str	x8, [x9, #16]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end237:
	.size	add_nftl_blk_list, .Lfunc_end237-add_nftl_blk_list
	.cfi_endproc
                                        // -- End function
	.globl	nand_ftl_exit                   // -- Begin function nand_ftl_exit
	.p2align	2
	.type	nand_ftl_exit,@function
nand_ftl_exit:                          // @nand_ftl_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	bl	del_last_nftl_blk
	str	x0, [sp, #8]
.LBB238_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB238_3
// %bb.2:                               //   in Loop: Header=BB238_1 Depth=1
	ldr	x0, [sp, #8]
	bl	nftl_exit
	ldr	x0, [sp, #8]
	bl	nftl_free
	adrp	x0, nftl_blk_head
	add	x0, x0, :lo12:nftl_blk_head
	bl	del_last_nftl_blk
	str	x0, [sp, #8]
	b	.LBB238_1
.LBB238_3:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end238:
	.size	nand_ftl_exit, .Lfunc_end238-nand_ftl_exit
	.cfi_endproc
                                        // -- End function
	.globl	del_last_nftl_blk               // -- Begin function del_last_nftl_blk
	.p2align	2
	.type	del_last_nftl_blk,@function
del_last_nftl_blk:                      // @del_last_nftl_blk
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #16]
	str	xzr, [sp, #8]
	ldr	x8, [sp, #16]
	str	x8, [sp]
.LBB239_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB239_5
// %bb.2:                               //   in Loop: Header=BB239_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbnz	x8, .LBB239_4
// %bb.3:
	ldr	x8, [sp]
	str	xzr, [x8, #16]
	ldr	x8, [sp, #8]
	str	x8, [sp, #24]
	b	.LBB239_6
.LBB239_4:                              //   in Loop: Header=BB239_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB239_1
.LBB239_5:
	str	xzr, [sp, #24]
.LBB239_6:
	ldr	x0, [sp, #24]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end239:
	.size	del_last_nftl_blk, .Lfunc_end239-del_last_nftl_blk
	.cfi_endproc
                                        // -- End function
	.globl	nftl_exit                       // -- Begin function nftl_exit
	.p2align	2
	.type	nftl_exit,@function
nftl_exit:                              // @nftl_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #8]
	bl	free_nand_partition
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #24]
	bl	nftl_zone_release
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #32]
	bl	nftl_free
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end240:
	.size	nftl_exit, .Lfunc_end240-nftl_exit
	.cfi_endproc
                                        // -- End function
	.globl	print_all_nftl_blk              // -- Begin function print_all_nftl_blk
	.p2align	2
	.type	print_all_nftl_blk,@function
print_all_nftl_blk:                     // @print_all_nftl_blk
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	str	x8, [sp, #8]
.LBB241_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB241_3
// %bb.2:                               //   in Loop: Header=BB241_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	ldr	w1, [x8]
	adrp	x0, .L.str.369
	add	x0, x0, :lo12:.L.str.369
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB241_1
.LBB241_3:
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end241:
	.size	print_all_nftl_blk, .Lfunc_end241-print_all_nftl_blk
	.cfi_endproc
                                        // -- End function
	.globl	get_nftl_num                    // -- Begin function get_nftl_num
	.p2align	2
	.type	get_nftl_num,@function
get_nftl_num:                           // @get_nftl_num
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	wzr, [sp, #12]
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	str	x8, [sp]
.LBB242_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB242_3
// %bb.2:                               //   in Loop: Header=BB242_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB242_1
.LBB242_3:
	ldr	w0, [sp, #12]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end242:
	.size	get_nftl_num, .Lfunc_end242-get_nftl_num
	.cfi_endproc
                                        // -- End function
	.globl	get_nftl_cap                    // -- Begin function get_nftl_cap
	.p2align	2
	.type	get_nftl_cap,@function
get_nftl_cap:                           // @get_nftl_cap
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	wzr, [sp, #12]
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	str	x8, [sp]
.LBB243_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	cbz	x8, .LBB243_3
// %bb.2:                               //   in Loop: Header=BB243_1 Depth=1
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	ldr	w9, [x8]
	ldr	w8, [sp, #12]
	add	w8, w8, w9
	str	w8, [sp, #12]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	b	.LBB243_1
.LBB243_3:
	ldr	w0, [sp, #12]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end243:
	.size	get_nftl_cap, .Lfunc_end243-get_nftl_cap
	.cfi_endproc
                                        // -- End function
	.globl	get_first_nftl_cap              // -- Begin function get_first_nftl_cap
	.p2align	2
	.type	get_first_nftl_cap,@function
get_first_nftl_cap:                     // @get_first_nftl_cap
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	ldr	w0, [x8]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end244:
	.size	get_first_nftl_cap, .Lfunc_end244-get_first_nftl_cap
	.cfi_endproc
                                        // -- End function
	.globl	nftl_read                       // -- Begin function nftl_read
	.p2align	2
	.type	nftl_read,@function
nftl_read:                              // @nftl_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	stur	w0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	x2, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	ldur	w8, [x29, #-12]
	cbnz	w8, .LBB245_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB245_8
.LBB245_2:
.LBB245_3:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-8]
	ldr	x9, [sp]
	ldr	w9, [x9]
	subs	w8, w8, w9
	b.lo	.LBB245_7
// %bb.4:                               //   in Loop: Header=BB245_3 Depth=1
	ldr	x8, [sp]
	ldr	w9, [x8]
	ldur	w8, [x29, #-8]
	subs	w8, w8, w9
	stur	w8, [x29, #-8]
	ldr	x8, [sp]
	ldr	x8, [x8, #16]
	str	x8, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB245_6
// %bb.5:
	ldur	w1, [x29, #-8]
	ldur	w2, [x29, #-12]
	adrp	x0, .L.str.370
	add	x0, x0, :lo12:.L.str.370
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB245_8
.LBB245_6:                              //   in Loop: Header=BB245_3 Depth=1
	b	.LBB245_3
.LBB245_7:
	ldr	x8, [sp]
	ldr	x8, [x8, #72]
	ldr	x0, [sp]
	ldur	w1, [x29, #-8]
	ldur	w2, [x29, #-12]
	ldr	x3, [sp, #8]
	blr	x8
	stur	w0, [x29, #-4]
.LBB245_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end245:
	.size	nftl_read, .Lfunc_end245-nftl_read
	.cfi_endproc
                                        // -- End function
	.globl	nftl_discard                    // -- Begin function nftl_discard
	.p2align	2
	.type	nftl_discard,@function
nftl_discard:                           // @nftl_discard
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	stur	w0, [x29, #-8]
	stur	w1, [x29, #-12]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	ldur	w8, [x29, #-12]
	cbnz	w8, .LBB246_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB246_8
.LBB246_2:
.LBB246_3:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-8]
	ldr	x9, [sp, #8]
	ldr	w9, [x9]
	subs	w8, w8, w9
	b.lo	.LBB246_7
// %bb.4:                               //   in Loop: Header=BB246_3 Depth=1
	ldr	x8, [sp, #8]
	ldr	w9, [x8]
	ldur	w8, [x29, #-8]
	subs	w8, w8, w9
	stur	w8, [x29, #-8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB246_6
// %bb.5:
	ldur	w1, [x29, #-8]
	ldur	w2, [x29, #-12]
	adrp	x0, .L.str.370
	add	x0, x0, :lo12:.L.str.370
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB246_8
.LBB246_6:                              //   in Loop: Header=BB246_3 Depth=1
	b	.LBB246_3
.LBB246_7:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #96]
	ldr	x0, [sp, #8]
	ldur	w1, [x29, #-8]
	ldur	w2, [x29, #-12]
	blr	x8
	stur	w0, [x29, #-4]
.LBB246_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end246:
	.size	nftl_discard, .Lfunc_end246-nftl_discard
	.cfi_endproc
                                        // -- End function
	.globl	nftl_flush_write_cache          // -- Begin function nftl_flush_write_cache
	.p2align	2
	.type	nftl_flush_write_cache,@function
nftl_flush_write_cache:                 // @nftl_flush_write_cache
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
.LBB247_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB247_3
// %bb.2:                               //   in Loop: Header=BB247_1 Depth=1
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #88]
	ldr	x0, [sp, #8]
	mov	w1, #65535
	blr	x8
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #16]
	str	x8, [sp, #8]
	b	.LBB247_1
.LBB247_3:
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end247:
	.size	nftl_flush_write_cache, .Lfunc_end247-nftl_flush_write_cache
	.cfi_endproc
                                        // -- End function
	.globl	nftl_write_end                  // -- Begin function nftl_write_end
	.p2align	2
	.type	nftl_write_end,@function
nftl_write_end:                         // @nftl_write_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	ldr	x8, [x8, #16]
	str	x8, [sp, #16]
	str	wzr, [sp, #12]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #8]
	ldrb	w8, [x8, #40]
	cbnz	w8, .LBB248_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB248_9
.LBB248_2:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #8]
	ldr	w8, [x8, #44]
	cbnz	w8, .LBB248_4
// %bb.3:
	mov	w8, #8
	str	w8, [sp, #12]
	b	.LBB248_5
.LBB248_4:
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #24]
	ldr	x8, [x8, #8]
	ldr	w8, [x8, #44]
	str	w8, [sp, #12]
.LBB248_5:
.LBB248_6:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	cbz	x8, .LBB248_8
// %bb.7:                               //   in Loop: Header=BB248_6 Depth=1
	ldr	x8, [sp, #16]
	ldr	x0, [x8, #24]
	ldr	w1, [sp, #12]
	bl	fill_no_use_page_to_current_block
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #16]
	str	x8, [sp, #16]
	b	.LBB248_6
.LBB248_8:
	stur	wzr, [x29, #-4]
.LBB248_9:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end248:
	.size	nftl_write_end, .Lfunc_end248-nftl_write_end
	.cfi_endproc
                                        // -- End function
	.globl	nftl_clean_zone                 // -- Begin function nftl_clean_zone
	.p2align	2
	.type	nftl_clean_zone,@function
nftl_clean_zone:                        // @nftl_clean_zone
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, nftl_blk_head
	add	x8, x8, :lo12:nftl_blk_head
	ldr	x8, [x8, #16]
	str	x8, [sp]
	ldr	x8, [sp]
	cbnz	x8, .LBB249_2
// %bb.1:
	stur	wzr, [x29, #-4]
	b	.LBB249_3
.LBB249_2:
	ldr	x8, [sp]
	ldr	x0, [x8, #24]
	bl	clean_zone
	stur	w0, [x29, #-4]
.LBB249_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end249:
	.size	nftl_clean_zone, .Lfunc_end249-nftl_clean_zone
	.cfi_endproc
                                        // -- End function
	.globl	nftl_support_boot_check_crc_from_dts // -- Begin function nftl_support_boot_check_crc_from_dts
	.p2align	2
	.type	nftl_support_boot_check_crc_from_dts,@function
nftl_support_boot_check_crc_from_dts:   // @nftl_support_boot_check_crc_from_dts
	.cfi_startproc
// %bb.0:
	stp	x29, x30, [sp, #-16]!           // 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	nand_get_support_boot_check_crc
	subs	w8, w0, #0                      // =0
	cset	w0, ne
	ldp	x29, x30, [sp], #16             // 16-byte Folded Reload
	ret
.Lfunc_end250:
	.size	nftl_support_boot_check_crc_from_dts, .Lfunc_end250-nftl_support_boot_check_crc_from_dts
	.cfi_endproc
                                        // -- End function
	.globl	get_nftl_version                // -- Begin function get_nftl_version
	.p2align	2
	.type	get_nftl_version,@function
get_nftl_version:                       // @get_nftl_version
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	x3, [sp, #16]
	str	x4, [sp, #8]
	ldur	x0, [x29, #-8]
	adrp	x1, .L.str.372
	add	x1, x1, :lo12:.L.str.372
	mov	w2, #2
	str	w2, [sp, #4]                    // 4-byte Folded Spill
	bl	nand_memcpy
	ldr	w2, [sp, #4]                    // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	adrp	x1, .L.str.373
	add	x1, x1, :lo12:.L.str.373
	bl	nand_memcpy
	ldr	x0, [sp, #24]
	adrp	x1, .L.str.374
	add	x1, x1, :lo12:.L.str.374
	mov	w2, #4
	bl	nand_memcpy
	ldr	x0, [sp, #16]
	adrp	x1, .L.str.375
	add	x1, x1, :lo12:.L.str.375
	mov	w2, #11
	bl	nand_memcpy
	ldr	x0, [sp, #8]
	adrp	x1, .L.str.376
	add	x1, x1, :lo12:.L.str.376
	mov	w2, #6
	bl	nand_memcpy
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end251:
	.size	get_nftl_version, .Lfunc_end251-get_nftl_version
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function nftl_version
	.type	nftl_version,@function
nftl_version:                           // @nftl_version
	.cfi_startproc
// %bb.0:
	stp	x29, x30, [sp, #-16]!           // 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x0, .L.str.487
	add	x0, x0, :lo12:.L.str.487
	adrp	x1, .L.str.372
	add	x1, x1, :lo12:.L.str.372
	adrp	x2, .L.str.373
	add	x2, x2, :lo12:.L.str.373
	adrp	x3, .L.str.374
	add	x3, x3, :lo12:.L.str.374
	adrp	x4, .L.str.375
	add	x4, x4, :lo12:.L.str.375
	adrp	x5, .L.str.376
	add	x5, x5, :lo12:.L.str.376
	bl	nand_print
	ldp	x29, x30, [sp], #16             // 16-byte Folded Reload
	ret
.Lfunc_end252:
	.size	nftl_version, .Lfunc_end252-nftl_version
	.cfi_endproc
                                        // -- End function
	.globl	_nand_read                      // -- Begin function _nand_read
	.p2align	2
	.type	_nand_read,@function
_nand_read:                             // @_nand_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	w2, [sp, #16]
	str	x3, [sp, #8]
	str	wzr, [sp, #4]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	ldr	x3, [sp, #8]
	bl	__nand_read
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end253:
	.size	_nand_read, .Lfunc_end253-_nand_read
	.cfi_endproc
                                        // -- End function
	.globl	_nand_write                     // -- Begin function _nand_write
	.p2align	2
	.type	_nand_write,@function
_nand_write:                            // @_nand_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	w2, [sp, #16]
	str	x3, [sp, #8]
	str	wzr, [sp, #4]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	ldr	x3, [sp, #8]
	bl	__nand_write
	str	w0, [sp, #4]
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end254:
	.size	_nand_write, .Lfunc_end254-_nand_write
	.cfi_endproc
                                        // -- End function
	.globl	_nand_discard                   // -- Begin function _nand_discard
	.p2align	2
	.type	_nand_discard,@function
_nand_discard:                          // @_nand_discard
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	w1, [x29, #-12]
	str	w2, [sp, #16]
	str	wzr, [sp, #12]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	ldur	w1, [x29, #-12]
	ldr	w2, [sp, #16]
	bl	__nand_discard
	str	w0, [sp, #12]
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end255:
	.size	_nand_discard, .Lfunc_end255-_nand_discard
	.cfi_endproc
                                        // -- End function
	.globl	_nand_flush_write_cache         // -- Begin function _nand_flush_write_cache
	.p2align	2
	.type	_nand_flush_write_cache,@function
_nand_flush_write_cache:                // @_nand_flush_write_cache
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	str	w1, [sp, #4]
	str	wzr, [sp]
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #24]
	ldr	w1, [sp, #4]
	bl	__nand_flush_write_cache
	str	w0, [sp]
	ldr	w0, [sp]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end256:
	.size	_nand_flush_write_cache, .Lfunc_end256-_nand_flush_write_cache
	.cfi_endproc
                                        // -- End function
	.globl	_shutdown_op                    // -- Begin function _shutdown_op
	.p2align	2
	.type	_shutdown_op,@function
_shutdown_op:                           // @_shutdown_op
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #24]
	bl	__shutdown_op
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end257:
	.size	_shutdown_op, .Lfunc_end257-_shutdown_op
	.cfi_endproc
                                        // -- End function
	.globl	_nand_sector_read               // -- Begin function _nand_sector_read
	.p2align	2
	.type	_nand_sector_read,@function
_nand_sector_read:                      // @_nand_sector_read
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	w1, [sp, #20]
	str	w2, [sp, #16]
	str	x3, [sp, #8]
	str	wzr, [sp, #4]
	ldr	w0, [sp, #4]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end258:
	.size	_nand_sector_read, .Lfunc_end258-_nand_sector_read
	.cfi_endproc
                                        // -- End function
	.globl	_nand_sector_write              // -- Begin function _nand_sector_write
	.p2align	2
	.type	_nand_sector_write,@function
_nand_sector_write:                     // @_nand_sector_write
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	w1, [sp, #20]
	str	w2, [sp, #16]
	str	x3, [sp, #8]
	str	wzr, [sp, #4]
	ldr	w0, [sp, #4]
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end259:
	.size	_nand_sector_write, .Lfunc_end259-_nand_sector_write
	.cfi_endproc
                                        // -- End function
	.globl	_nand_flush_sector_write_cache  // -- Begin function _nand_flush_sector_write_cache
	.p2align	2
	.type	_nand_flush_sector_write_cache,@function
_nand_flush_sector_write_cache:         // @_nand_flush_sector_write_cache
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	str	w1, [sp, #4]
	str	wzr, [sp]
	ldr	w0, [sp]
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end260:
	.size	_nand_flush_sector_write_cache, .Lfunc_end260-_nand_flush_sector_write_cache
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_vender_set_param      // -- Begin function hynix26nm_vender_set_param
	.p2align	2
	.type	hynix26nm_vender_set_param,@function
hynix26nm_vender_set_param:             // @hynix26nm_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #8]
	mov	w8, #54
	strb	w8, [sp, #15]
	mov	w8, #22
	strb	w8, [sp, #14]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	str	wzr, [sp, #16]
.LBB261_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hs	.LBB261_4
// %bb.2:                               //   in Loop: Header=BB261_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x3, x8, x9
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x4, x8, x9
	add	x1, sp, #15                     // =15
	mov	w2, wzr
	str	w2, [sp, #4]                    // 4-byte Folded Spill
	mov	w6, #1
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w2, [sp, #4]                    // 4-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldrb	w1, [sp, #14]
	bl	set_one_cmd
// %bb.3:                               //   in Loop: Header=BB261_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB261_1
.LBB261_4:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-16]
	ldrb	w2, [x8, #1]
	ldur	x8, [x29, #-16]
	ldrb	w3, [x8, #2]
	ldur	x8, [x29, #-16]
	ldrb	w4, [x8, #3]
	adrp	x0, .L.str.379
	add	x0, x0, :lo12:.L.str.379
	bl	nand_print_dbg
	ldr	w0, [sp, #8]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end261:
	.size	hynix26nm_vender_set_param, .Lfunc_end261-hynix26nm_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_set_readretry         // -- Begin function hynix26nm_set_readretry
	.p2align	2
	.type	hynix26nm_set_readretry,@function
hynix26nm_set_readretry:                // @hynix26nm_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	wzr, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	adrp	x9, hynix26nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ls	.LBB262_2
// %bb.1:
	mov	w8, #-53
	stur	w8, [x29, #-4]
	b	.LBB262_23
.LBB262_2:
	str	wzr, [sp, #12]
.LBB262_3:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB262_12
// %bb.4:                               //   in Loop: Header=BB262_3 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #12]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	strh	w8, [sp]
	ldrsh	w8, [sp]
	ldr	x9, [sp, #16]
	ldr	w9, [x9, #184]
	mov	w10, w9
	adrp	x9, hynix26nm_read_retry_val
	add	x9, x9, :lo12:hynix26nm_read_retry_val
	add	x9, x9, x10, lsl #3
	ldr	w10, [sp, #12]
                                        // kill: def $x10 killed $w10
	ldrsh	w9, [x9, x10, lsl #1]
	add	w8, w8, w9
	strh	w8, [sp, #2]
	ldrsh	w8, [sp, #2]
	subs	w8, w8, #255                    // =255
	b.le	.LBB262_6
// %bb.5:                               //   in Loop: Header=BB262_3 Depth=1
	mov	w8, #255
	strh	w8, [sp, #2]
	b	.LBB262_10
.LBB262_6:                              //   in Loop: Header=BB262_3 Depth=1
	ldrsh	w8, [sp, #2]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB262_8
// %bb.7:                               //   in Loop: Header=BB262_3 Depth=1
	strh	wzr, [sp, #2]
	b	.LBB262_9
.LBB262_8:                              //   in Loop: Header=BB262_3 Depth=1
	ldrsh	w8, [sp, #2]
	and	w8, w8, #0xff
	strh	w8, [sp, #2]
.LBB262_9:                              //   in Loop: Header=BB262_3 Depth=1
.LBB262_10:                             //   in Loop: Header=BB262_3 Depth=1
	ldrh	w8, [sp, #2]
	ldr	w9, [sp, #12]
	mov	w10, w9
	add	x9, sp, #4                      // =4
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.11:                              //   in Loop: Header=BB262_3 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB262_3
.LBB262_12:
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	subs	w8, w8, #2                      // =2
	b.lo	.LBB262_15
// %bb.13:
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	subs	w8, w8, #6                      // =6
	b.hi	.LBB262_15
// %bb.14:
	strb	wzr, [sp, #4]
.LBB262_15:
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	subs	w8, w8, #5                      // =5
	b.eq	.LBB262_17
// %bb.16:
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	subs	w8, w8, #6                      // =6
	b.ne	.LBB262_18
.LBB262_17:
	strb	wzr, [sp, #5]
.LBB262_18:
	ldr	x0, [sp, #16]
	adrp	x8, hynix26nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_read_retry_reg_num]
	add	x1, sp, #4                      // =4
	adrp	x2, hynix26nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix26nm_read_retry_reg_adr
	bl	hynix26nm_vender_set_param
	str	w0, [sp, #8]
	adrp	x0, .L.str.380
	add	x0, x0, :lo12:.L.str.380
	bl	nand_print_dbg
	str	wzr, [sp, #12]
.LBB262_19:                             // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB262_22
// %bb.20:                              //   in Loop: Header=BB262_19 Depth=1
	ldr	w8, [sp, #12]
	mov	w9, w8
	add	x8, sp, #4                      // =4
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.381
	add	x0, x0, :lo12:.L.str.381
	bl	nand_print_dbg
// %bb.21:                              //   in Loop: Header=BB262_19 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB262_19
.LBB262_22:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
	ldr	w8, [sp, #8]
	stur	w8, [x29, #-4]
.LBB262_23:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end262:
	.size	hynix26nm_set_readretry, .Lfunc_end262-hynix26nm_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_read_page_end         // -- Begin function hynix26nm_read_page_end
	.p2align	2
	.type	hynix26nm_read_page_end,@function
hynix26nm_read_page_end:                // @hynix26nm_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB263_13
// %bb.1:
	adrp	x0, .L.str.382
	add	x0, x0, :lo12:.L.str.382
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB263_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, hynix26nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB263_12
// %bb.3:                               //   in Loop: Header=BB263_2 Depth=1
	str	wzr, [sp, #16]
	ldr	x9, [sp, #8]
	ldr	w8, [x9, #184]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #184]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #184]
	adrp	x9, hynix26nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ls	.LBB263_5
// %bb.4:                               //   in Loop: Header=BB263_2 Depth=1
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
.LBB263_5:                              //   in Loop: Header=BB263_2 Depth=1
	ldr	x0, [sp, #8]
	bl	hynix26nm_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB263_7
// %bb.6:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	hynix26nm_set_readretry
	b	.LBB263_12
.LBB263_7:                              //   in Loop: Header=BB263_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB263_9
// %bb.8:                               //   in Loop: Header=BB263_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB263_10
.LBB263_9:
	mov	w8, #10
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.383
	add	x0, x0, :lo12:.L.str.383
	bl	nand_print_dbg
	b	.LBB263_12
.LBB263_10:                             //   in Loop: Header=BB263_2 Depth=1
// %bb.11:                              //   in Loop: Header=BB263_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB263_2
.LBB263_12:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	hynix26nm_set_readretry
.LBB263_13:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end263:
	.size	hynix26nm_read_page_end, .Lfunc_end263-hynix26nm_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_vender_get_param      // -- Begin function hynix26nm_vender_get_param
	.p2align	2
	.type	hynix26nm_vender_get_param,@function
hynix26nm_vender_get_param:             // @hynix26nm_vender_get_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #12]
	mov	w8, #55
	strb	w8, [sp, #11]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	str	wzr, [sp, #16]
.LBB264_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hs	.LBB264_4
// %bb.2:                               //   in Loop: Header=BB264_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x2, x8, x9
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x3, x8, x9
	add	x1, sp, #11                     // =11
	mov	w4, #1
	bl	get_data_with_nand_bus_one_cmd
// %bb.3:                               //   in Loop: Header=BB264_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB264_1
.LBB264_4:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end264:
	.size	hynix26nm_vender_get_param, .Lfunc_end264-hynix26nm_vender_get_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_lsb_init              // -- Begin function hynix26nm_lsb_init
	.p2align	2
	.type	hynix26nm_lsb_init,@function
hynix26nm_lsb_init:                     // @hynix26nm_lsb_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	adrp	x12, hynix26nm_lsb_mode_reg_adr
	adrp	x11, hynix26nm_lsb_mode_reg_adr
	add	x11, x11, :lo12:hynix26nm_lsb_mode_reg_adr
	adrp	x10, hynix26nm_lsb_mode_val
	adrp	x9, hynix26nm_lsb_mode_val
	add	x9, x9, :lo12:hynix26nm_lsb_mode_val
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w8, w8, #0xff
	adrp	x13, hynix26nm_read_retry_mode
	strb	w8, [x13, :lo12:hynix26nm_read_retry_mode]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #8
	and	w8, w8, #0xff
	adrp	x13, hynix26nm_read_retry_cycle
	strb	w8, [x13, :lo12:hynix26nm_read_retry_cycle]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	and	w8, w8, #0xff
	adrp	x13, hynix26nm_read_retry_reg_num
	strb	w8, [x13, :lo12:hynix26nm_read_retry_reg_num]
	adrp	x13, hynix26nm_lsb_mode_reg_num
	mov	w8, #5
	strb	w8, [x13, :lo12:hynix26nm_lsb_mode_reg_num]
	mov	w8, #164
	strb	w8, [x12, :lo12:hynix26nm_lsb_mode_reg_adr]
	mov	w8, #165
	strb	w8, [x11, #1]
	mov	w8, #176
	strb	w8, [x11, #2]
	mov	w8, #177
	strb	w8, [x11, #3]
	mov	w8, #201
	strb	w8, [x11, #4]
	mov	w8, #37
	strb	w8, [x10, :lo12:hynix26nm_lsb_mode_val]
	strb	w8, [x9, #1]
	strb	w8, [x9, #2]
	strb	w8, [x9, #3]
	mov	w8, #1
	strb	w8, [x9, #4]
	mov	w0, wzr
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end265:
	.size	hynix26nm_lsb_init, .Lfunc_end265-hynix26nm_lsb_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_lsb_enable            // -- Begin function hynix26nm_lsb_enable
	.p2align	2
	.type	hynix26nm_lsb_enable,@function
hynix26nm_lsb_enable:                   // @hynix26nm_lsb_enable
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	ldur	x0, [x29, #-8]
	adrp	x8, hynix26nm_lsb_mode_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_lsb_mode_reg_num]
	adrp	x1, hynix26nm_lsb_mode_default_val
	add	x1, x1, :lo12:hynix26nm_lsb_mode_default_val
	adrp	x2, hynix26nm_lsb_mode_reg_adr
	add	x2, x2, :lo12:hynix26nm_lsb_mode_reg_adr
	bl	hynix26nm_vender_get_param
	stur	wzr, [x29, #-12]
.LBB266_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, hynix26nm_lsb_mode_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_lsb_mode_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB266_4
// %bb.2:                               //   in Loop: Header=BB266_1 Depth=1
	ldur	w8, [x29, #-12]
	mov	w9, w8
	adrp	x8, hynix26nm_lsb_mode_default_val
	add	x8, x8, :lo12:hynix26nm_lsb_mode_default_val
	add	x8, x8, x9
	ldrb	w9, [x8]
	ldur	w8, [x29, #-12]
	mov	w11, w8
	add	x8, sp, #15                     // =15
	mov	x10, x8
	add	x10, x10, x11
	strb	w9, [x10]
	ldur	w9, [x29, #-12]
	mov	w10, w9
	adrp	x9, hynix26nm_lsb_mode_val
	add	x9, x9, :lo12:hynix26nm_lsb_mode_val
	ldrb	w10, [x9, x10]
	ldur	w9, [x29, #-12]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9
	ldrb	w8, [x9]
	add	w8, w8, w10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB266_1 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB266_1
.LBB266_4:
	ldur	x0, [x29, #-8]
	adrp	x8, hynix26nm_lsb_mode_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_lsb_mode_reg_num]
	add	x1, sp, #15                     // =15
	adrp	x2, hynix26nm_lsb_mode_reg_adr
	add	x2, x2, :lo12:hynix26nm_lsb_mode_reg_adr
	bl	hynix26nm_vender_set_param
	mov	w0, wzr
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end266:
	.size	hynix26nm_lsb_enable, .Lfunc_end266-hynix26nm_lsb_enable
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_lsb_disable           // -- Begin function hynix26nm_lsb_disable
	.p2align	2
	.type	hynix26nm_lsb_disable,@function
hynix26nm_lsb_disable:                  // @hynix26nm_lsb_disable
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	adrp	x8, hynix26nm_lsb_mode_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_lsb_mode_reg_num]
	adrp	x1, hynix26nm_lsb_mode_default_val
	add	x1, x1, :lo12:hynix26nm_lsb_mode_default_val
	adrp	x2, hynix26nm_lsb_mode_reg_adr
	add	x2, x2, :lo12:hynix26nm_lsb_mode_reg_adr
	bl	hynix26nm_vender_set_param
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end267:
	.size	hynix26nm_lsb_disable, .Lfunc_end267-hynix26nm_lsb_disable
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_lsb_exit              // -- Begin function hynix26nm_lsb_exit
	.p2align	2
	.type	hynix26nm_lsb_exit,@function
hynix26nm_lsb_exit:                     // @hynix26nm_lsb_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	mov	w8, #1048575
	str	w8, [sp, #4]
	ldr	x0, [sp, #8]
	bl	nand_enable_chip
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x0, [sp, #8]
	mov	w2, wzr
	str	w2, [sp]                        // 4-byte Folded Spill
	mov	w1, w2
	bl	set_one_cmd
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	x0, [sp, #8]
	mov	w1, #48
	mov	w2, #1
	bl	set_one_cmd
.LBB268_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	subs	w9, w8, #1                      // =1
	str	w9, [sp, #4]
	subs	w8, w8, #0                      // =0
	b.le	.LBB268_3
// %bb.2:                               //   in Loop: Header=BB268_1 Depth=1
	b	.LBB268_1
.LBB268_3:
	adrp	x0, .L.str.384
	add	x0, x0, :lo12:.L.str.384
	bl	nand_print_dbg
	ldr	x0, [sp, #8]
	bl	nand_disable_chip
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end268:
	.size	hynix26nm_lsb_exit, .Lfunc_end268-hynix26nm_lsb_exit
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_getdefaultparam       // -- Begin function hynix26nm_getdefaultparam
	.p2align	2
	.type	hynix26nm_getdefaultparam,@function
hynix26nm_getdefaultparam:              // @hynix26nm_getdefaultparam
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	wzr, [sp, #8]
	ldur	x0, [x29, #-8]
	ldur	x8, [x29, #-8]
	add	x1, x8, #56                     // =56
	adrp	x8, hynix26nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_read_retry_reg_num]
	adrp	x2, hynix26nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix26nm_read_retry_reg_adr
	bl	hynix26nm_vender_get_param
	str	w0, [sp, #8]
	str	wzr, [sp, #12]
.LBB269_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.ge	.LBB269_4
// %bb.2:                               //   in Loop: Header=BB269_1 Depth=1
	ldur	x8, [x29, #-8]
	add	x8, x8, #56                     // =56
	ldrsw	x9, [sp, #12]
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	x9, [sp, #16]
	ldrsw	x10, [sp, #12]
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB269_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB269_1
.LBB269_4:
	ldr	w0, [sp, #8]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end269:
	.size	hynix26nm_getdefaultparam, .Lfunc_end269-hynix26nm_getdefaultparam
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_setdefaultparam       // -- Begin function hynix26nm_setdefaultparam
	.p2align	2
	.type	hynix26nm_setdefaultparam,@function
hynix26nm_setdefaultparam:              // @hynix26nm_setdefaultparam
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	wzr, [sp, #16]
	stur	wzr, [x29, #-12]
.LBB270_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.ge	.LBB270_4
// %bb.2:                               //   in Loop: Header=BB270_1 Depth=1
	ldur	x8, [x29, #-8]
	add	x8, x8, #56                     // =56
	ldursw	x9, [x29, #-12]
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldursw	x10, [x29, #-12]
	add	x9, sp, #8                      // =8
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB270_1 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB270_1
.LBB270_4:
	ldur	x0, [x29, #-8]
	adrp	x8, hynix26nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_read_retry_reg_num]
	add	x1, sp, #8                      // =8
	adrp	x2, hynix26nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix26nm_read_retry_reg_adr
	bl	hynix26nm_vender_set_param
	str	w0, [sp, #16]
	adrp	x0, .L.str.385
	add	x0, x0, :lo12:.L.str.385
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB270_5:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.ge	.LBB270_8
// %bb.6:                               //   in Loop: Header=BB270_5 Depth=1
	ldursw	x9, [x29, #-12]
	add	x8, sp, #8                      // =8
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.386
	add	x0, x0, :lo12:.L.str.386
	bl	nand_print_dbg
// %bb.7:                               //   in Loop: Header=BB270_5 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB270_5
.LBB270_8:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end270:
	.size	hynix26nm_setdefaultparam, .Lfunc_end270-hynix26nm_setdefaultparam
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_get_default_param     // -- Begin function hynix26nm_get_default_param
	.p2align	2
	.type	hynix26nm_get_default_param,@function
hynix26nm_get_default_param:            // @hynix26nm_get_default_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	add	x1, sp, #4                      // =4
	bl	hynix26nm_getdefaultparam
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldrb	w3, [sp, #4]
	ldrb	w4, [sp, #5]
	ldrb	w5, [sp, #6]
	ldrb	w6, [sp, #7]
	adrp	x0, .L.str.387
	add	x0, x0, :lo12:.L.str.387
	bl	nand_print_dbg
	ldr	x0, [sp, #8]
	bl	hynix26nm_setdefaultparam
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end271:
	.size	hynix26nm_get_default_param, .Lfunc_end271-hynix26nm_get_default_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_readretry_init        // -- Begin function hynix26nm_readretry_init
	.p2align	2
	.type	hynix26nm_readretry_init,@function
hynix26nm_readretry_init:               // @hynix26nm_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, hynix26nm_read_retry_reg_adr
	add	x8, x8, :lo12:hynix26nm_read_retry_reg_adr
	str	x8, [sp]                        // 8-byte Folded Spill
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w9, w8, #0xff
	adrp	x8, hynix26nm_read_retry_mode
	strb	w9, [x8, :lo12:hynix26nm_read_retry_mode]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	lsr	w9, w9, #8
	and	w9, w9, #0xff
	adrp	x10, hynix26nm_read_retry_cycle
	strb	w9, [x10, :lo12:hynix26nm_read_retry_cycle]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, hynix26nm_read_retry_reg_num
	strb	w9, [x10, :lo12:hynix26nm_read_retry_reg_num]
	ldrb	w8, [x8, :lo12:hynix26nm_read_retry_mode]
	cbnz	w8, .LBB272_10
// %bb.1:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	mov	w8, #172
	adrp	x10, hynix26nm_read_retry_reg_adr
	strb	w8, [x10, :lo12:hynix26nm_read_retry_reg_adr]
	mov	w8, #173
	strb	w8, [x9, #1]
	mov	w8, #174
	strb	w8, [x9, #2]
	mov	w8, #175
	strb	w8, [x9, #3]
	str	wzr, [sp, #12]
.LBB272_2:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB272_4 Depth 2
	ldr	w8, [sp, #12]
	adrp	x9, hynix26nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_cycle]
	add	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB272_9
// %bb.3:                               //   in Loop: Header=BB272_2 Depth=1
	str	wzr, [sp, #8]
.LBB272_4:                              //   Parent Loop BB272_2 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #8]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB272_7
// %bb.5:                               //   in Loop: Header=BB272_4 Depth=2
	ldr	w8, [sp, #12]
	mov	w9, w8
	adrp	x8, para0
	add	x8, x8, :lo12:para0
	add	x8, x8, x9, lsl #3
	ldr	w9, [sp, #8]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #1
	ldrh	w8, [x8]
	ldr	w9, [sp, #12]
	mov	w10, w9
	adrp	x9, hynix26nm_read_retry_val
	add	x9, x9, :lo12:hynix26nm_read_retry_val
	add	x9, x9, x10, lsl #3
	ldr	w10, [sp, #8]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #1
	strh	w8, [x9]
// %bb.6:                               //   in Loop: Header=BB272_4 Depth=2
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
	b	.LBB272_4
.LBB272_7:                              //   in Loop: Header=BB272_2 Depth=1
// %bb.8:                               //   in Loop: Header=BB272_2 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB272_2
.LBB272_9:
	b	.LBB272_22
.LBB272_10:
	adrp	x8, hynix26nm_read_retry_mode
	ldrb	w9, [x8, :lo12:hynix26nm_read_retry_mode]
	mov	w8, #1
	subs	w8, w8, w9
	b.ne	.LBB272_20
// %bb.11:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	adrp	x10, hynix26nm_read_retry_reg_adr
	mov	w8, #167
	strb	w8, [x10, :lo12:hynix26nm_read_retry_reg_adr]
	mov	w8, #173
	strb	w8, [x9, #1]
	mov	w8, #174
	strb	w8, [x9, #2]
	mov	w8, #175
	strb	w8, [x9, #3]
	str	wzr, [sp, #12]
.LBB272_12:                             // =>This Loop Header: Depth=1
                                        //     Child Loop BB272_14 Depth 2
	ldr	w8, [sp, #12]
	adrp	x9, hynix26nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_cycle]
	add	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.hs	.LBB272_19
// %bb.13:                              //   in Loop: Header=BB272_12 Depth=1
	str	wzr, [sp, #8]
.LBB272_14:                             //   Parent Loop BB272_12 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #8]
	adrp	x9, hynix26nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix26nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB272_17
// %bb.15:                              //   in Loop: Header=BB272_14 Depth=2
	ldr	w8, [sp, #12]
	mov	w9, w8
	adrp	x8, para1
	add	x8, x8, :lo12:para1
	add	x8, x8, x9, lsl #3
	ldr	w9, [sp, #8]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9, lsl #1
	ldrh	w8, [x8]
	ldr	w9, [sp, #12]
	mov	w10, w9
	adrp	x9, hynix26nm_read_retry_val
	add	x9, x9, :lo12:hynix26nm_read_retry_val
	add	x9, x9, x10, lsl #3
	ldr	w10, [sp, #8]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10, lsl #1
	strh	w8, [x9]
// %bb.16:                              //   in Loop: Header=BB272_14 Depth=2
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
	b	.LBB272_14
.LBB272_17:                             //   in Loop: Header=BB272_12 Depth=1
// %bb.18:                              //   in Loop: Header=BB272_12 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB272_12
.LBB272_19:
	b	.LBB272_21
.LBB272_20:
	adrp	x8, hynix26nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix26nm_read_retry_mode]
	adrp	x0, .L.str.388
	add	x0, x0, :lo12:.L.str.388
	bl	nand_print
	mov	w8, #-54
	stur	w8, [x29, #-4]
	b	.LBB272_27
.LBB272_21:
.LBB272_22:
	ldr	x8, [sp, #16]
	str	wzr, [x8, #184]
	str	wzr, [sp, #12]
.LBB272_23:                             // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w8, w8, #128                    // =128
	b.hs	.LBB272_26
// %bb.24:                              //   in Loop: Header=BB272_23 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	strb	wzr, [x8]
// %bb.25:                              //   in Loop: Header=BB272_23 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB272_23
.LBB272_26:
	ldr	x0, [sp, #16]
	bl	hynix26nm_get_default_param
	stur	wzr, [x29, #-4]
.LBB272_27:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end272:
	.size	hynix26nm_readretry_init, .Lfunc_end272-hynix26nm_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_readretry_exit        // -- Begin function hynix26nm_readretry_exit
	.p2align	2
	.type	hynix26nm_readretry_exit,@function
hynix26nm_readretry_exit:               // @hynix26nm_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	adrp	x0, .L.str.389
	add	x0, x0, :lo12:.L.str.389
	bl	nand_print_dbg
	ldr	x0, [sp, #8]
	bl	hynix26nm_setdefaultparam
	ldr	x8, [sp, #8]
	mov	w0, wzr
	str	wzr, [x8, #184]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end273:
	.size	hynix26nm_readretry_exit, .Lfunc_end273-hynix26nm_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_special_init          // -- Begin function hynix26nm_special_init
	.p2align	2
	.type	hynix26nm_special_init,@function
hynix26nm_special_init:                 // @hynix26nm_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB274_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB274_3
// %bb.2:                               //   in Loop: Header=BB274_1 Depth=1
	ldr	x0, [sp]
	bl	hynix26nm_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB274_1
.LBB274_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB274_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, hynix26nm_read_page_end
	add	x8, x8, :lo12:hynix26nm_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, hynix26nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix26nm_read_retry_mode]
	adrp	x8, hynix26nm_read_retry_cycle
	ldrb	w2, [x8, :lo12:hynix26nm_read_retry_cycle]
	adrp	x8, hynix26nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_read_retry_reg_num]
	adrp	x0, .L.str.390
	add	x0, x0, :lo12:.L.str.390
	bl	nand_print_dbg
	b	.LBB274_6
.LBB274_5:
	adrp	x8, hynix26nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix26nm_read_retry_mode]
	adrp	x8, hynix26nm_read_retry_cycle
	ldrb	w2, [x8, :lo12:hynix26nm_read_retry_cycle]
	adrp	x8, hynix26nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix26nm_read_retry_reg_num]
	adrp	x0, .L.str.391
	add	x0, x0, :lo12:.L.str.391
	bl	nand_print
.LBB274_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end274:
	.size	hynix26nm_special_init, .Lfunc_end274-hynix26nm_special_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix26nm_special_exit          // -- Begin function hynix26nm_special_exit
	.p2align	2
	.type	hynix26nm_special_exit,@function
hynix26nm_special_exit:                 // @hynix26nm_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB275_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB275_3
// %bb.2:                               //   in Loop: Header=BB275_1 Depth=1
	ldr	x0, [sp, #8]
	bl	hynix26nm_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB275_1
.LBB275_3:
	adrp	x0, .L.str.392
	add	x0, x0, :lo12:.L.str.392
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end275:
	.size	hynix26nm_special_exit, .Lfunc_end275-hynix26nm_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	samsung_vender_set_param        // -- Begin function samsung_vender_set_param
	.p2align	2
	.type	samsung_vender_set_param,@function
samsung_vender_set_param:               // @samsung_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	x2, [sp, #8]
	str	w3, [sp, #4]
	ldur	x0, [x29, #-8]
	ldr	x3, [sp, #8]
	ldr	x4, [sp, #16]
	ldr	w6, [sp, #4]
	adrp	x1, samsung_read_retry_cmd
	add	x1, x1, :lo12:samsung_read_retry_cmd
	mov	w2, wzr
	mov	w5, #1
	bl	set_cmd_with_nand_bus
	str	w0, [sp]
	ldr	x8, [sp, #16]
	ldrb	w1, [x8]
	ldr	x8, [sp, #16]
	ldrb	w2, [x8, #1]
	ldr	x8, [sp, #16]
	ldrb	w3, [x8, #2]
	ldr	x8, [sp, #16]
	ldrb	w4, [x8, #3]
	adrp	x0, .L.str.379
	add	x0, x0, :lo12:.L.str.379
	bl	nand_print_dbg
	ldr	w0, [sp]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end276:
	.size	samsung_vender_set_param, .Lfunc_end276-samsung_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	samsung_set_readretry           // -- Begin function samsung_set_readretry
	.p2align	2
	.type	samsung_set_readretry,@function
samsung_set_readretry:                  // @samsung_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	bl	nand_enable_chip
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #16]
	subs	w8, w8, #236                    // =236
	b.ne	.LBB277_9
// %bb.1:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #17]
	subs	w8, w8, #222                    // =222
	b.ne	.LBB277_9
// %bb.2:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #18]
	subs	w8, w8, #148                    // =148
	b.ne	.LBB277_9
// %bb.3:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #19]
	subs	w8, w8, #195                    // =195
	b.ne	.LBB277_9
// %bb.4:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #20]
	subs	w8, w8, #164                    // =164
	b.ne	.LBB277_9
// %bb.5:
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #21]
	subs	w8, w8, #202                    // =202
	b.ne	.LBB277_9
// %bb.6:
	ldr	x0, [sp, #8]
	bl	samsung_set_readretry_K9GCGD8U0F
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	cbz	w8, .LBB277_8
// %bb.7:
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #184]
	adrp	x0, .L.str.393
	add	x0, x0, :lo12:.L.str.393
	bl	nand_print
.LBB277_8:
	b	.LBB277_12
.LBB277_9:
	ldr	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #184]
	mov	w9, w8
	adrp	x8, samsung_p1
	add	x8, x8, :lo12:samsung_p1
	add	x1, x8, x9, lsl #2
	adrp	x2, samsung_read_retry_addr
	add	x2, x2, :lo12:samsung_read_retry_addr
	mov	w3, #4
	bl	samsung_vender_set_param
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	cbz	w8, .LBB277_11
// %bb.10:
	ldr	x8, [sp, #8]
	ldr	w1, [x8, #184]
	adrp	x0, .L.str.394
	add	x0, x0, :lo12:.L.str.394
	bl	nand_print
.LBB277_11:
.LBB277_12:
	ldr	x0, [sp, #8]
	bl	nand_disable_chip
	ldr	w0, [sp, #4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end277:
	.size	samsung_set_readretry, .Lfunc_end277-samsung_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	samsung_set_readretry_K9GCGD8U0F // -- Begin function samsung_set_readretry_K9GCGD8U0F
	.p2align	2
	.type	samsung_set_readretry_K9GCGD8U0F,@function
samsung_set_readretry_K9GCGD8U0F:       // @samsung_set_readretry_K9GCGD8U0F
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	sturb	wzr, [x29, #-14]
	stur	wzr, [x29, #-20]
	str	wzr, [sp, #24]
	strb	wzr, [sp, #23]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	sturb	w8, [x29, #-13]
	strb	wzr, [sp, #23]
.LBB278_1:                              // =>This Inner Loop Header: Depth=1
	ldrb	w8, [sp, #23]
	subs	w8, w8, #4                      // =4
	b.ge	.LBB278_4
// %bb.2:                               //   in Loop: Header=BB278_1 Depth=1
	ldurb	w8, [x29, #-13]
	ldrb	w9, [sp, #23]
	mov	w10, w9
	sub	x9, x29, #20                    // =20
	add	x9, x9, x10
	strb	w8, [x9]
	ldrb	w8, [sp, #23]
	mov	w9, w8
	add	x8, sp, #24                     // =24
	add	x9, x8, x9
	mov	w8, #255
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB278_1 Depth=1
	ldrb	w8, [sp, #23]
	add	w8, w8, #1                      // =1
	strb	w8, [sp, #23]
	b	.LBB278_1
.LBB278_4:
	ldur	x0, [x29, #-8]
	sub	x1, x29, #14                    // =14
	str	x1, [sp, #8]                    // 8-byte Folded Spill
	sub	x2, x29, #20                    // =20
	bl	nand_set_read_retry_K9GCGD8U0F
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	stur	w0, [x29, #-12]
	ldur	x0, [x29, #-8]
	add	x2, sp, #24                     // =24
	bl	nand_get_read_retry_K9GCGD8U0F
	stur	w0, [x29, #-12]
	ldrb	w8, [sp, #24]
	ldurb	w9, [x29, #-20]
	subs	w8, w8, w9
	b.ne	.LBB278_8
// %bb.5:
	ldrb	w8, [sp, #25]
	ldurb	w9, [x29, #-19]
	subs	w8, w8, w9
	b.ne	.LBB278_8
// %bb.6:
	ldrb	w8, [sp, #26]
	ldurb	w9, [x29, #-18]
	subs	w8, w8, w9
	b.ne	.LBB278_8
// %bb.7:
	ldrb	w8, [sp, #27]
	ldurb	w9, [x29, #-17]
	subs	w8, w8, w9
	b.eq	.LBB278_9
.LBB278_8:
	adrp	x0, .L.str.398
	add	x0, x0, :lo12:.L.str.398
	adrp	x1, .L__func__.samsung_set_readretry_K9GCGD8U0F
	add	x1, x1, :lo12:.L__func__.samsung_set_readretry_K9GCGD8U0F
	bl	nand_print
	mov	w8, #-1
	stur	w8, [x29, #-12]
.LBB278_9:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB278_11
// %bb.10:
	ldur	x8, [x29, #-8]
	str	wzr, [x8, #184]
.LBB278_11:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end278:
	.size	samsung_set_readretry_K9GCGD8U0F, .Lfunc_end278-samsung_set_readretry_K9GCGD8U0F
	.cfi_endproc
                                        // -- End function
	.globl	samsung_read_page_end           // -- Begin function samsung_read_page_end
	.p2align	2
	.type	samsung_read_page_end,@function
samsung_read_page_end:                  // @samsung_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB279_19
// %bb.1:
	adrp	x0, .L.str.395
	add	x0, x0, :lo12:.L.str.395
	bl	nand_print_dbg
	mov	w8, #1
	stur	w8, [x29, #-12]
.LBB279_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, samsung_read_retry_cycle
	ldrb	w9, [x9, :lo12:samsung_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB279_18
// %bb.3:                               //   in Loop: Header=BB279_2 Depth=1
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #16]
	subs	w8, w8, #236                    // =236
	b.ne	.LBB279_10
// %bb.4:                               //   in Loop: Header=BB279_2 Depth=1
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #17]
	subs	w8, w8, #222                    // =222
	b.ne	.LBB279_10
// %bb.5:                               //   in Loop: Header=BB279_2 Depth=1
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #18]
	subs	w8, w8, #148                    // =148
	b.ne	.LBB279_10
// %bb.6:                               //   in Loop: Header=BB279_2 Depth=1
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #19]
	subs	w8, w8, #195                    // =195
	b.ne	.LBB279_10
// %bb.7:                               //   in Loop: Header=BB279_2 Depth=1
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #20]
	subs	w8, w8, #164                    // =164
	b.ne	.LBB279_10
// %bb.8:                               //   in Loop: Header=BB279_2 Depth=1
	ldr	x8, [sp, #8]
	ldrb	w8, [x8, #21]
	subs	w8, w8, #202                    // =202
	b.ne	.LBB279_10
// %bb.9:                               //   in Loop: Header=BB279_2 Depth=1
	ldur	w9, [x29, #-12]
	mov	w8, #255
	subs	w8, w8, w9
	add	w8, w8, #1                      // =1
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	b	.LBB279_11
.LBB279_10:                             //   in Loop: Header=BB279_2 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
.LBB279_11:                             //   in Loop: Header=BB279_2 Depth=1
	ldr	x0, [sp, #8]
	bl	samsung_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB279_13
// %bb.12:                              //   in Loop: Header=BB279_2 Depth=1
	b	.LBB279_17
.LBB279_13:                             //   in Loop: Header=BB279_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB279_15
// %bb.14:                              //   in Loop: Header=BB279_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB279_16
.LBB279_15:
	mov	w8, #10
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.396
	add	x0, x0, :lo12:.L.str.396
	bl	nand_print_dbg
	b	.LBB279_18
.LBB279_16:                             //   in Loop: Header=BB279_2 Depth=1
.LBB279_17:                             //   in Loop: Header=BB279_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB279_2
.LBB279_18:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	samsung_set_readretry
.LBB279_19:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end279:
	.size	samsung_read_page_end, .Lfunc_end279-samsung_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	samsung_readretry_init          // -- Begin function samsung_readretry_init
	.p2align	2
	.type	samsung_readretry_init,@function
samsung_readretry_init:                 // @samsung_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x8, [sp]
	str	wzr, [x8, #184]
	ldr	x8, [sp]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w8, w8, #0xff
	adrp	x9, samsung_read_retry_mode
	strb	w8, [x9, :lo12:samsung_read_retry_mode]
	ldr	x8, [sp]
	ldrb	w8, [x8, #16]
	subs	w8, w8, #236                    // =236
	b.ne	.LBB280_7
// %bb.1:
	ldr	x8, [sp]
	ldrb	w8, [x8, #17]
	subs	w8, w8, #222                    // =222
	b.ne	.LBB280_7
// %bb.2:
	ldr	x8, [sp]
	ldrb	w8, [x8, #18]
	subs	w8, w8, #148                    // =148
	b.ne	.LBB280_7
// %bb.3:
	ldr	x8, [sp]
	ldrb	w8, [x8, #19]
	subs	w8, w8, #195                    // =195
	b.ne	.LBB280_7
// %bb.4:
	ldr	x8, [sp]
	ldrb	w8, [x8, #20]
	subs	w8, w8, #164                    // =164
	b.ne	.LBB280_7
// %bb.5:
	ldr	x8, [sp]
	ldrb	w8, [x8, #21]
	subs	w8, w8, #202                    // =202
	b.ne	.LBB280_7
// %bb.6:
	mov	w8, #255
	adrp	x9, samsung_read_retry_cycle
	strb	w8, [x9, :lo12:samsung_read_retry_cycle]
	b	.LBB280_8
.LBB280_7:
	adrp	x9, samsung_read_retry_cycle
	mov	w8, #15
	strb	w8, [x9, :lo12:samsung_read_retry_cycle]
.LBB280_8:
	adrp	x8, samsung_read_retry_cycle
	ldrb	w1, [x8, :lo12:samsung_read_retry_cycle]
	adrp	x0, .L.str.397
	add	x0, x0, :lo12:.L.str.397
	bl	nand_print_dbg
	adrp	x8, samsung_read_retry_mode
	ldrb	w8, [x8, :lo12:samsung_read_retry_mode]
	subs	w8, w8, #32                     // =32
	b.eq	.LBB280_10
// %bb.9:
	mov	w8, #-100
	stur	w8, [x29, #-4]
	b	.LBB280_11
.LBB280_10:
	stur	wzr, [x29, #-4]
.LBB280_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end280:
	.size	samsung_readretry_init, .Lfunc_end280-samsung_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	samsung_readretry_exit          // -- Begin function samsung_readretry_exit
	.p2align	2
	.type	samsung_readretry_exit,@function
samsung_readretry_exit:                 // @samsung_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	mov	w0, wzr
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end281:
	.size	samsung_readretry_exit, .Lfunc_end281-samsung_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	samsung_special_init            // -- Begin function samsung_special_init
	.p2align	2
	.type	samsung_special_init,@function
samsung_special_init:                   // @samsung_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB282_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB282_3
// %bb.2:                               //   in Loop: Header=BB282_1 Depth=1
	ldr	x0, [sp]
	bl	samsung_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB282_1
.LBB282_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB282_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, samsung_read_page_end
	add	x8, x8, :lo12:samsung_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, samsung_read_retry_mode
	ldrb	w1, [x8, :lo12:samsung_read_retry_mode]
	adrp	x8, samsung_read_retry_cycle
	ldrb	w2, [x8, :lo12:samsung_read_retry_cycle]
	adrp	x0, .L.str.399
	add	x0, x0, :lo12:.L.str.399
	bl	nand_print_dbg
	b	.LBB282_6
.LBB282_5:
	adrp	x8, samsung_read_retry_mode
	ldrb	w1, [x8, :lo12:samsung_read_retry_mode]
	adrp	x8, samsung_read_retry_cycle
	ldrb	w2, [x8, :lo12:samsung_read_retry_cycle]
	adrp	x0, .L.str.400
	add	x0, x0, :lo12:.L.str.400
	bl	nand_print
.LBB282_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end282:
	.size	samsung_special_init, .Lfunc_end282-samsung_special_init
	.cfi_endproc
                                        // -- End function
	.globl	samsung_special_exit            // -- Begin function samsung_special_exit
	.p2align	2
	.type	samsung_special_exit,@function
samsung_special_exit:                   // @samsung_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB283_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB283_3
// %bb.2:                               //   in Loop: Header=BB283_1 Depth=1
	ldr	x0, [sp, #8]
	bl	samsung_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB283_1
.LBB283_3:
	adrp	x0, .L.str.401
	add	x0, x0, :lo12:.L.str.401
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end283:
	.size	samsung_special_exit, .Lfunc_end283-samsung_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_read_retry_clock_save   // -- Begin function sandisk_read_retry_clock_save
	.p2align	2
	.type	sandisk_read_retry_clock_save,@function
sandisk_read_retry_clock_save:          // @sandisk_read_retry_clock_save
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	str	x0, [sp]                        // 8-byte Folded Spill
	adrp	x2, sandisk_sclk0_bak
	add	x2, x2, :lo12:sandisk_sclk0_bak
	adrp	x3, sandisk_sclk1_bak
	add	x3, x3, :lo12:sandisk_sclk1_bak
	bl	nand_get_clk
	ldr	x0, [sp]                        // 8-byte Folded Reload
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	mov	w2, #10
	mov	w3, #20
	bl	nand_set_clk
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end284:
	.size	sandisk_read_retry_clock_save, .Lfunc_end284-sandisk_read_retry_clock_save
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_vender_set_param        // -- Begin function sandisk_vender_set_param
	.p2align	2
	.type	sandisk_vender_set_param,@function
sandisk_vender_set_param:               // @sandisk_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #16]
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #48                     // =48
	b.eq	.LBB285_2
// %bb.1:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #49                     // =49
	b.ne	.LBB285_3
.LBB285_2:
	ldur	x0, [x29, #-8]
	adrp	x1, sandisk_acti_start_cmd
	add	x1, x1, :lo12:sandisk_acti_start_cmd
	mov	w5, wzr
	str	w5, [sp, #12]                   // 4-byte Folded Spill
	mov	w2, w5
	mov	x4, xzr
	mov	x3, x4
	mov	w6, #2
	bl	set_cmd_with_nand_bus
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	ldr	x3, [sp, #24]
	ldur	x4, [x29, #-16]
	adrp	x1, sandisk_read_retry_cmd_19
	add	x1, x1, :lo12:sandisk_read_retry_cmd_19
	mov	w5, #1
	mov	w6, #3
	bl	set_cmd_with_nand_bus
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
.LBB285_3:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #50                     // =50
	b.ne	.LBB285_5
// %bb.4:
	ldur	x0, [x29, #-8]
	ldr	x3, [sp, #24]
	ldur	x4, [x29, #-16]
	adrp	x1, sandisk_read_retry_cmd_1y
	add	x1, x1, :lo12:sandisk_read_retry_cmd_1y
	mov	w6, #1
	mov	w2, w6
	mov	w5, #4
	bl	set_cmd_with_nand_bus
	str	w0, [sp, #16]
.LBB285_5:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end285:
	.size	sandisk_vender_set_param, .Lfunc_end285-sandisk_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_read_retry_clock_recover // -- Begin function sandisk_read_retry_clock_recover
	.p2align	2
	.type	sandisk_read_retry_clock_recover,@function
sandisk_read_retry_clock_recover:       // @sandisk_read_retry_clock_recover
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x8, sandisk_sclk0_bak
	ldr	w2, [x8, :lo12:sandisk_sclk0_bak]
	adrp	x8, sandisk_sclk1_bak
	ldr	w3, [x8, :lo12:sandisk_sclk1_bak]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_set_clk
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end286:
	.size	sandisk_read_retry_clock_recover, .Lfunc_end286-sandisk_read_retry_clock_recover
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_end_one_read_retry      // -- Begin function sandisk_end_one_read_retry
	.p2align	2
	.type	sandisk_end_one_read_retry,@function
sandisk_end_one_read_retry:             // @sandisk_end_one_read_retry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #48                     // =48
	b.eq	.LBB287_3
// %bb.1:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #49                     // =49
	b.eq	.LBB287_3
// %bb.2:
	stur	wzr, [x29, #-4]
	b	.LBB287_8
.LBB287_3:
	ldr	x0, [sp]
	bl	nand_enable_chip
	ldr	x8, [sp]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x8, [sp]
	ldr	x0, [x8, #280]
	bl	ndfc_is_toggle_interface
	cbz	w0, .LBB287_5
// %bb.4:
	ldr	x8, [sp]
	ldr	x0, [x8, #280]
	bl	ndfc_set_legacy_interface
	adrp	x9, sandisk_toggle_mode_flag
	mov	w8, #1
	str	w8, [x9, :lo12:sandisk_toggle_mode_flag]
.LBB287_5:
	ldr	x0, [sp]
	adrp	x1, sandisk_read_retry_disable_cmd_19
	add	x1, x1, :lo12:sandisk_read_retry_disable_cmd_19
	mov	w5, wzr
	mov	w2, w5
	mov	x4, xzr
	mov	x3, x4
	mov	w6, #1
	bl	set_cmd_with_nand_bus
	adrp	x8, sandisk_toggle_mode_flag
	ldr	w8, [x8, :lo12:sandisk_toggle_mode_flag]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB287_7
// %bb.6:
	ldr	x8, [sp]
	ldr	x0, [x8, #280]
	bl	ndfc_set_toggle_interface
.LBB287_7:
	ldr	x0, [sp]
	bl	nand_disable_chip
	stur	wzr, [x29, #-4]
.LBB287_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end287:
	.size	sandisk_end_one_read_retry, .Lfunc_end287-sandisk_end_one_read_retry
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_set_readretry           // -- Begin function sandisk_set_readretry
	.p2align	2
	.type	sandisk_set_readretry,@function
sandisk_set_readretry:                  // @sandisk_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	stur	wzr, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	sandisk_read_retry_clock_save
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_is_toggle_interface
	cbz	w0, .LBB288_2
// %bb.1:
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_set_legacy_interface
	adrp	x9, sandisk_toggle_mode_flag
	mov	w8, #1
	str	w8, [x9, :lo12:sandisk_toggle_mode_flag]
.LBB288_2:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #48                     // =48
	b.ne	.LBB288_9
// %bb.3:
	ldurh	w8, [x29, #-10]
	subs	w8, w8, #255                    // =255
	b.eq	.LBB288_7
// %bb.4:
	ldurh	w8, [x29, #-10]
	cbz	w8, .LBB288_6
// %bb.5:
	ldurh	w8, [x29, #-10]
	mov	w10, #2
	sdiv	w9, w8, w10
	mul	w9, w9, w10
	subs	w8, w8, w9
	cbz	w8, .LBB288_7
.LBB288_6:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
                                        // kill: def $x8 killed $w8
	mov	x9, #3
	mul	x9, x8, x9
	adrp	x8, sandisk_19_low
	add	x8, x8, :lo12:sandisk_19_low
	add	x8, x8, x9
	str	x8, [sp, #24]
	adrp	x8, sandisk_read_retry_reg_adr_19
	add	x8, x8, :lo12:sandisk_read_retry_reg_adr_19
	str	x8, [sp, #16]
	mov	w8, #3
	str	w8, [sp, #12]
	mov	w8, #16
	adrp	x9, sandisk_read_retry_cycle
	strb	w8, [x9, :lo12:sandisk_read_retry_cycle]
	b	.LBB288_8
.LBB288_7:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
                                        // kill: def $x8 killed $w8
	mov	x9, #3
	mul	x9, x8, x9
	adrp	x8, sandisk_19_high
	add	x8, x8, :lo12:sandisk_19_high
	add	x8, x8, x9
	str	x8, [sp, #24]
	adrp	x8, sandisk_read_retry_reg_adr_19
	add	x8, x8, :lo12:sandisk_read_retry_reg_adr_19
	str	x8, [sp, #16]
	mov	w8, #3
	str	w8, [sp, #12]
	adrp	x9, sandisk_read_retry_cycle
	mov	w8, #20
	strb	w8, [x9, :lo12:sandisk_read_retry_cycle]
.LBB288_8:
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #24]
	ldr	x2, [sp, #16]
	ldr	w3, [sp, #12]
	bl	sandisk_vender_set_param
	stur	w0, [x29, #-16]
	ldur	x0, [x29, #-8]
	mov	w1, #-74
	mov	w2, wzr
	bl	set_one_cmd
	b	.LBB288_16
.LBB288_9:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #49                     // =49
	b.ne	.LBB288_11
// %bb.10:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
                                        // kill: def $x8 killed $w8
	mov	x9, #3
	mul	x9, x8, x9
	adrp	x8, sandisk_24
	add	x8, x8, :lo12:sandisk_24
	add	x8, x8, x9
	str	x8, [sp, #24]
	adrp	x8, sandisk_read_retry_reg_adr_24
	add	x8, x8, :lo12:sandisk_read_retry_reg_adr_24
	str	x8, [sp, #16]
	mov	w8, #3
	str	w8, [sp, #12]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #24]
	ldr	x2, [sp, #16]
	ldr	w3, [sp, #12]
	bl	sandisk_vender_set_param
	stur	w0, [x29, #-16]
	ldur	x0, [x29, #-8]
	mov	w1, #-74
	mov	w2, wzr
	bl	set_one_cmd
	b	.LBB288_15
.LBB288_11:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #50                     // =50
	b.ne	.LBB288_13
// %bb.12:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	mov	w9, w8
	adrp	x8, sandisk_1y
	add	x8, x8, :lo12:sandisk_1y
	add	x8, x8, x9, lsl #2
	str	x8, [sp, #24]
	adrp	x8, sandisk_read_retry_reg_adr_1y
	add	x8, x8, :lo12:sandisk_read_retry_reg_adr_1y
	str	x8, [sp, #16]
	mov	w8, #1
	str	w8, [sp, #12]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #24]
	ldr	x2, [sp, #16]
	ldr	w3, [sp, #12]
	bl	sandisk_vender_set_param
	stur	w0, [x29, #-16]
	ldur	x0, [x29, #-8]
	mov	w1, #93
	mov	w2, wzr
	bl	set_one_cmd
	b	.LBB288_14
.LBB288_13:
.LBB288_14:
.LBB288_15:
.LBB288_16:
	adrp	x8, sandisk_toggle_mode_flag
	ldr	w8, [x8, :lo12:sandisk_toggle_mode_flag]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB288_18
// %bb.17:
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_set_toggle_interface
.LBB288_18:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	x0, [x29, #-8]
	bl	sandisk_read_retry_clock_recover
	ldur	w0, [x29, #-16]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end288:
	.size	sandisk_set_readretry, .Lfunc_end288-sandisk_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_read_page_end           // -- Begin function sandisk_read_page_end
	.p2align	2
	.type	sandisk_read_page_end,@function
sandisk_read_page_end:                  // @sandisk_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB289_13
// %bb.1:
	adrp	x0, .L.str.402
	add	x0, x0, :lo12:.L.str.402
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB289_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, sandisk_read_retry_cycle
	ldrb	w9, [x9, :lo12:sandisk_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB289_12
// %bb.3:                               //   in Loop: Header=BB289_2 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #8]
	bl	sandisk_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB289_5
// %bb.4:                               //   in Loop: Header=BB289_2 Depth=1
	b	.LBB289_11
.LBB289_5:                              //   in Loop: Header=BB289_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldr	x0, [sp, #8]
	bl	sandisk_end_one_read_retry
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB289_7
// %bb.6:                               //   in Loop: Header=BB289_2 Depth=1
	mov	w8, #-2
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.403
	add	x0, x0, :lo12:.L.str.403
	bl	nand_print_dbg
	b	.LBB289_11
.LBB289_7:                              //   in Loop: Header=BB289_2 Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB289_9
// %bb.8:                               //   in Loop: Header=BB289_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB289_10
.LBB289_9:
	mov	w8, #10
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.404
	add	x0, x0, :lo12:.L.str.404
	bl	nand_print_dbg
	b	.LBB289_12
.LBB289_10:                             //   in Loop: Header=BB289_2 Depth=1
.LBB289_11:                             //   in Loop: Header=BB289_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB289_2
.LBB289_12:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	sandisk_exit_readretry
.LBB289_13:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end289:
	.size	sandisk_read_page_end, .Lfunc_end289-sandisk_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_exit_readretry          // -- Begin function sandisk_exit_readretry
	.p2align	2
	.type	sandisk_exit_readretry,@function
sandisk_exit_readretry:                 // @sandisk_exit_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_is_toggle_interface
	cbz	w0, .LBB290_2
// %bb.1:
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_set_legacy_interface
	adrp	x9, sandisk_toggle_mode_flag
	mov	w8, #1
	str	w8, [x9, :lo12:sandisk_toggle_mode_flag]
.LBB290_2:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #48                     // =48
	b.ne	.LBB290_4
// %bb.3:
	adrp	x8, sandisk_read_retry_reg_adr_19
	add	x8, x8, :lo12:sandisk_read_retry_reg_adr_19
	str	x8, [sp, #8]
	mov	w8, #3
	str	w8, [sp, #4]
	ldur	x0, [x29, #-8]
	ldr	x2, [sp, #8]
	ldr	w3, [sp, #4]
	add	x1, sp, #16                     // =16
	bl	sandisk_vender_set_param
	stur	w0, [x29, #-12]
	b	.LBB290_11
.LBB290_4:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #49                     // =49
	b.ne	.LBB290_6
// %bb.5:
	adrp	x8, sandisk_read_retry_reg_adr_24
	add	x8, x8, :lo12:sandisk_read_retry_reg_adr_24
	str	x8, [sp, #8]
	mov	w8, #3
	str	w8, [sp, #4]
	ldur	x0, [x29, #-8]
	ldr	x2, [sp, #8]
	ldr	w3, [sp, #4]
	add	x1, sp, #16                     // =16
	bl	sandisk_vender_set_param
	stur	w0, [x29, #-12]
	b	.LBB290_10
.LBB290_6:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #50                     // =50
	b.ne	.LBB290_8
// %bb.7:
	ldur	x0, [x29, #-8]
	mov	w1, #-1
	mov	w2, #1
	str	w2, [sp]                        // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	w8, [sp]                        // 4-byte Folded Reload
	adrp	x9, sandisk_read_retry_reg_adr_1y
	add	x9, x9, :lo12:sandisk_read_retry_reg_adr_1y
	str	x9, [sp, #8]
	str	w8, [sp, #4]
	ldur	x0, [x29, #-8]
	ldr	x2, [sp, #8]
	ldr	w3, [sp, #4]
	add	x1, sp, #16                     // =16
	bl	sandisk_vender_set_param
	stur	w0, [x29, #-12]
	b	.LBB290_9
.LBB290_8:
.LBB290_9:
.LBB290_10:
.LBB290_11:
	adrp	x8, sandisk_toggle_mode_flag
	ldr	w8, [x8, :lo12:sandisk_toggle_mode_flag]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB290_13
// %bb.12:
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_set_toggle_interface
.LBB290_13:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end290:
	.size	sandisk_exit_readretry, .Lfunc_end290-sandisk_exit_readretry
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_activation_seq          // -- Begin function sandisk_activation_seq
	.p2align	2
	.type	sandisk_activation_seq,@function
sandisk_activation_seq:                 // @sandisk_activation_seq
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	adrp	x8, .L__const.sandisk_activation_seq.cmd
	add	x8, x8, :lo12:.L__const.sandisk_activation_seq.cmd
	ldr	x9, [x8]
	sub	x10, x29, #24                   // =24
	str	x10, [sp]                       // 8-byte Folded Spill
	stur	x9, [x29, #-24]
	ldrb	w8, [x8, #8]
	sturb	w8, [x29, #-16]
	add	x8, sp, #24                     // =24
	str	x8, [sp, #16]                   // 8-byte Folded Spill
	str	xzr, [sp, #24]
	strb	wzr, [sp, #32]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x0, [x29, #-8]
	adrp	x1, sandisk_acti_start_cmd
	add	x1, x1, :lo12:sandisk_acti_start_cmd
	mov	w5, wzr
	str	w5, [sp, #12]                   // 4-byte Folded Spill
	mov	w2, w5
	mov	x4, xzr
	mov	x3, x4
	mov	w6, #2
	bl	set_cmd_with_nand_bus
	ldr	x1, [sp]                        // 8-byte Folded Reload
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	ldr	x4, [sp, #16]                   // 8-byte Folded Reload
	stur	w0, [x29, #-12]
	ldur	x0, [x29, #-8]
	adrp	x3, sandisk_acti_start_addr
	add	x3, x3, :lo12:sandisk_acti_start_addr
	mov	w5, #1
	mov	w6, #9
	bl	set_cmd_with_nand_bus
	ldur	w8, [x29, #-12]
	orr	w8, w8, w0
	stur	w8, [x29, #-12]
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	w8, [x29, #-12]
	cbz	w8, .LBB291_2
// %bb.1:
	ldur	w1, [x29, #-12]
	adrp	x0, .L.str.405
	add	x0, x0, :lo12:.L.str.405
	bl	nand_print
.LBB291_2:
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end291:
	.size	sandisk_activation_seq, .Lfunc_end291-sandisk_activation_seq
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_readretry_init          // -- Begin function sandisk_readretry_init
	.p2align	2
	.type	sandisk_readretry_init,@function
sandisk_readretry_init:                 // @sandisk_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x8, [sp]
	str	wzr, [x8, #184]
	ldr	x8, [sp]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w9, w8, #0xff
	adrp	x8, sandisk_read_retry_mode
	strb	w9, [x8, :lo12:sandisk_read_retry_mode]
	ldr	x9, [sp]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	lsr	w9, w9, #8
	and	w9, w9, #0xff
	adrp	x10, sandisk_read_retry_cycle
	strb	w9, [x10, :lo12:sandisk_read_retry_cycle]
	ldr	x9, [sp]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, sandisk_read_retry_reg_cnt
	strb	w9, [x10, :lo12:sandisk_read_retry_reg_cnt]
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #48                     // =48
	b.eq	.LBB292_2
// %bb.1:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #49                     // =49
	b.ne	.LBB292_3
.LBB292_2:
	ldr	x0, [sp]
	bl	sandisk_activation_seq
.LBB292_3:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #48                     // =48
	b.eq	.LBB292_7
// %bb.4:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #49                     // =49
	b.eq	.LBB292_7
// %bb.5:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_read_retry_mode]
	subs	w8, w8, #50                     // =50
	b.eq	.LBB292_7
// %bb.6:
	mov	w8, #-99
	stur	w8, [x29, #-4]
	b	.LBB292_8
.LBB292_7:
	adrp	x8, sandisk_read_retry_cycle
	ldrb	w1, [x8, :lo12:sandisk_read_retry_cycle]
	adrp	x0, .L.str.406
	add	x0, x0, :lo12:.L.str.406
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
.LBB292_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end292:
	.size	sandisk_readretry_init, .Lfunc_end292-sandisk_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_readretry_exit          // -- Begin function sandisk_readretry_exit
	.p2align	2
	.type	sandisk_readretry_exit,@function
sandisk_readretry_exit:                 // @sandisk_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	mov	w0, wzr
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end293:
	.size	sandisk_readretry_exit, .Lfunc_end293-sandisk_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_special_init            // -- Begin function sandisk_special_init
	.p2align	2
	.type	sandisk_special_init,@function
sandisk_special_init:                   // @sandisk_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB294_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB294_3
// %bb.2:                               //   in Loop: Header=BB294_1 Depth=1
	ldr	x0, [sp]
	bl	sandisk_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB294_1
.LBB294_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB294_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, sandisk_read_page_end
	add	x8, x8, :lo12:sandisk_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, sandisk_read_retry_mode
	ldrb	w1, [x8, :lo12:sandisk_read_retry_mode]
	adrp	x8, sandisk_read_retry_cycle
	ldrb	w2, [x8, :lo12:sandisk_read_retry_cycle]
	adrp	x8, sandisk_read_retry_reg_cnt
	ldrb	w3, [x8, :lo12:sandisk_read_retry_reg_cnt]
	adrp	x0, .L.str.407
	add	x0, x0, :lo12:.L.str.407
	bl	nand_print_dbg
	b	.LBB294_6
.LBB294_5:
	adrp	x8, sandisk_read_retry_mode
	ldrb	w1, [x8, :lo12:sandisk_read_retry_mode]
	adrp	x8, sandisk_read_retry_cycle
	ldrb	w2, [x8, :lo12:sandisk_read_retry_cycle]
	adrp	x8, sandisk_read_retry_reg_cnt
	ldrb	w3, [x8, :lo12:sandisk_read_retry_reg_cnt]
	adrp	x0, .L.str.408
	add	x0, x0, :lo12:.L.str.408
	bl	nand_print
.LBB294_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end294:
	.size	sandisk_special_init, .Lfunc_end294-sandisk_special_init
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_special_exit            // -- Begin function sandisk_special_exit
	.p2align	2
	.type	sandisk_special_exit,@function
sandisk_special_exit:                   // @sandisk_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB295_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB295_3
// %bb.2:                               //   in Loop: Header=BB295_1 Depth=1
	ldr	x0, [sp, #8]
	bl	sandisk_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB295_1
.LBB295_3:
	adrp	x0, .L.str.409
	add	x0, x0, :lo12:.L.str.409
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end295:
	.size	sandisk_special_exit, .Lfunc_end295-sandisk_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	generic_special_init            // -- Begin function generic_special_init
	.p2align	2
	.type	generic_special_init,@function
generic_special_init:                   // @generic_special_init
	.cfi_startproc
// %bb.0:
	stp	x29, x30, [sp, #-16]!           // 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x9, df_read_page_end
	adrp	x8, generic_read_page_end_not_retry
	add	x8, x8, :lo12:generic_read_page_end_not_retry
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x0, .L.str.410
	add	x0, x0, :lo12:.L.str.410
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp], #16             // 16-byte Folded Reload
	ret
.Lfunc_end296:
	.size	generic_special_init, .Lfunc_end296-generic_special_init
	.cfi_endproc
                                        // -- End function
	.globl	generic_special_exit            // -- Begin function generic_special_exit
	.p2align	2
	.type	generic_special_exit,@function
generic_special_exit:                   // @generic_special_exit
	.cfi_startproc
// %bb.0:
	stp	x29, x30, [sp, #-16]!           // 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x0, .L.str.411
	add	x0, x0, :lo12:.L.str.411
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp], #16             // 16-byte Folded Reload
	ret
.Lfunc_end297:
	.size	generic_special_exit, .Lfunc_end297-generic_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_dsp_on              // -- Begin function sandisk_A19_dsp_on
	.p2align	2
	.type	sandisk_A19_dsp_on,@function
sandisk_A19_dsp_on:                     // @sandisk_A19_dsp_on
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	nand_enable_chip
	ldr	x8, [sp, #16]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x0, [sp, #16]
	mov	w1, #38
	mov	w2, wzr
	str	w2, [sp, #12]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	x0, [sp, #16]
	bl	nand_disable_chip
	ldr	w0, [sp, #12]                   // 4-byte Folded Reload
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end298:
	.size	sandisk_A19_dsp_on, .Lfunc_end298-sandisk_A19_dsp_on
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_cmd25               // -- Begin function sandisk_A19_cmd25
	.p2align	2
	.type	sandisk_A19_cmd25,@function
sandisk_A19_cmd25:                      // @sandisk_A19_cmd25
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	bl	nand_enable_chip
	ldr	x8, [sp, #16]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x0, [sp, #16]
	mov	w1, #37
	mov	w2, wzr
	str	w2, [sp, #12]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	x0, [sp, #16]
	bl	nand_disable_chip
	ldr	w0, [sp, #12]                   // 4-byte Folded Reload
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end299:
	.size	sandisk_A19_cmd25, .Lfunc_end299-sandisk_A19_cmd25
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_set_lmflgfix_next   // -- Begin function sandisk_A19_set_lmflgfix_next
	.p2align	2
	.type	sandisk_A19_set_lmflgfix_next,@function
sandisk_A19_set_lmflgfix_next:          // @sandisk_A19_set_lmflgfix_next
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturb	w1, [x29, #-9]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	stur	x0, [x29, #-24]
	ldur	x0, [x29, #-24]
	bl	nand_enable_chip
	ldur	x8, [x29, #-24]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x0, [x29, #-24]
	mov	w1, #92
	mov	w2, wzr
	stur	w2, [x29, #-32]                 // 4-byte Folded Spill
	bl	set_one_cmd
	ldur	w2, [x29, #-32]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-24]
	mov	w1, #-59
	bl	set_one_cmd
	sub	x3, x29, #25                    // =25
	sturb	wzr, [x29, #-25]
	sub	x4, x29, #26                    // =26
	mov	w8, #1
	sturb	w8, [x29, #-26]
	sub	x1, x29, #27                    // =27
	mov	w8, #85
	sturb	w8, [x29, #-27]
	ldur	x0, [x29, #-24]
	mov	w6, #1
	mov	w2, w6
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w9, [x8, :lo12:sandisk_A19_read_retry_mode]
	mov	w8, #52
	subs	w8, w8, w9
	b.ne	.LBB300_2
// %bb.1:
	mov	w8, #34
	sturb	w8, [x29, #-25]
	b	.LBB300_6
.LBB300_2:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w9, [x8, :lo12:sandisk_A19_read_retry_mode]
	mov	w8, #53
	subs	w8, w8, w9
	b.ne	.LBB300_4
// %bb.3:
	mov	w8, #37
	sturb	w8, [x29, #-25]
	b	.LBB300_5
.LBB300_4:
	mov	w8, #35
	sturb	w8, [x29, #-25]
.LBB300_5:
.LBB300_6:
	ldurb	w8, [x29, #-9]
	sub	x4, x29, #26                    // =26
	str	x4, [sp, #32]                   // 8-byte Folded Spill
	sturb	w8, [x29, #-26]
	sub	x1, x29, #27                    // =27
	str	x1, [sp, #16]                   // 8-byte Folded Spill
	mov	w8, #85
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	sturb	w8, [x29, #-27]
	ldur	x0, [x29, #-24]
	mov	w6, #1
	stur	w6, [x29, #-36]                 // 4-byte Folded Spill
	mov	w2, w6
	sub	x3, x29, #25                    // =25
	str	x3, [sp, #24]                   // 8-byte Folded Spill
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w8, [sp, #12]                   // 4-byte Folded Reload
	ldr	x1, [sp, #16]                   // 8-byte Folded Reload
	ldr	x3, [sp, #24]                   // 8-byte Folded Reload
	ldr	x4, [sp, #32]                   // 8-byte Folded Reload
	ldur	w6, [x29, #-36]                 // 4-byte Folded Reload
	sturb	wzr, [x29, #-25]
	sturb	wzr, [x29, #-26]
	sturb	w8, [x29, #-27]
	ldur	x0, [x29, #-24]
	mov	w2, w6
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldur	x0, [x29, #-24]
	bl	nand_disable_chip
	mov	w0, wzr
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end300:
	.size	sandisk_A19_set_lmflgfix_next, .Lfunc_end300-sandisk_A19_set_lmflgfix_next
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_read_page_end       // -- Begin function sandisk_A19_read_page_end
	.p2align	2
	.type	sandisk_A19_read_page_end,@function
sandisk_A19_read_page_end:              // @sandisk_A19_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB301_78
// %bb.1:
	adrp	x0, .L.str.412
	add	x0, x0, :lo12:.L.str.412
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB301_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, sandisk_A19_read_retry_cycle
	ldrb	w9, [x9, :lo12:sandisk_A19_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB301_15
// %bb.3:                               //   in Loop: Header=BB301_2 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #8]
	bl	sandisk_A19_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB301_5
// %bb.4:                               //   in Loop: Header=BB301_2 Depth=1
	adrp	x0, .L.str.413
	add	x0, x0, :lo12:.L.str.413
	bl	nand_print_dbg
	b	.LBB301_14
.LBB301_5:                              //   in Loop: Header=BB301_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB301_7
// %bb.6:                               //   in Loop: Header=BB301_2 Depth=1
	mov	w8, #-2
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.403
	add	x0, x0, :lo12:.L.str.403
	bl	nand_print_dbg
	b	.LBB301_14
.LBB301_7:                              //   in Loop: Header=BB301_2 Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB301_9
// %bb.8:                               //   in Loop: Header=BB301_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_13
.LBB301_9:
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_11
// %bb.10:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.414
	add	x0, x0, :lo12:.L.str.414
	bl	nand_print_dbg
	b	.LBB301_12
.LBB301_11:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.415
	add	x0, x0, :lo12:.L.str.415
	bl	nand_print_dbg
.LBB301_12:
	mov	w8, #10
	str	w8, [sp, #16]
	b	.LBB301_77
.LBB301_13:                             //   in Loop: Header=BB301_2 Depth=1
.LBB301_14:                             //   in Loop: Header=BB301_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB301_2
.LBB301_15:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #51                     // =51
	b.ne	.LBB301_17
// %bb.16:
	b	.LBB301_46
.LBB301_17:
	stur	wzr, [x29, #-12]
.LBB301_18:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, sandisk_A19_read_retry_cycle
	ldrb	w9, [x9, :lo12:sandisk_A19_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB301_31
// %bb.19:                              //   in Loop: Header=BB301_18 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #8]
	bl	sandisk_A19_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB301_21
// %bb.20:                              //   in Loop: Header=BB301_18 Depth=1
	adrp	x0, .L.str.413
	add	x0, x0, :lo12:.L.str.413
	bl	nand_print_dbg
	b	.LBB301_30
.LBB301_21:                             //   in Loop: Header=BB301_18 Depth=1
	ldur	x0, [x29, #-8]
	bl	sandisk_A19_dsp_on
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB301_23
// %bb.22:                              //   in Loop: Header=BB301_18 Depth=1
	mov	w8, #-2
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.403
	add	x0, x0, :lo12:.L.str.403
	bl	nand_print_dbg
	b	.LBB301_30
.LBB301_23:                             //   in Loop: Header=BB301_18 Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB301_25
// %bb.24:                              //   in Loop: Header=BB301_18 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_29
.LBB301_25:
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_27
// %bb.26:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.414
	add	x0, x0, :lo12:.L.str.414
	bl	nand_print_dbg
	b	.LBB301_28
.LBB301_27:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.415
	add	x0, x0, :lo12:.L.str.415
	bl	nand_print_dbg
.LBB301_28:
	mov	w8, #10
	str	w8, [sp, #16]
	b	.LBB301_77
.LBB301_29:                             //   in Loop: Header=BB301_18 Depth=1
.LBB301_30:                             //   in Loop: Header=BB301_18 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB301_18
.LBB301_31:
	stur	wzr, [x29, #-12]
.LBB301_32:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, sandisk_A19_read_retry_cycle
	ldrb	w9, [x9, :lo12:sandisk_A19_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB301_45
// %bb.33:                              //   in Loop: Header=BB301_32 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #8]
	bl	sandisk_A19_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB301_35
// %bb.34:                              //   in Loop: Header=BB301_32 Depth=1
	adrp	x0, .L.str.413
	add	x0, x0, :lo12:.L.str.413
	bl	nand_print_dbg
	b	.LBB301_44
.LBB301_35:                             //   in Loop: Header=BB301_32 Depth=1
	ldur	x0, [x29, #-8]
	bl	sandisk_A19_cmd25
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB301_37
// %bb.36:                              //   in Loop: Header=BB301_32 Depth=1
	mov	w8, #-2
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.403
	add	x0, x0, :lo12:.L.str.403
	bl	nand_print_dbg
	b	.LBB301_44
.LBB301_37:                             //   in Loop: Header=BB301_32 Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB301_39
// %bb.38:                              //   in Loop: Header=BB301_32 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_43
.LBB301_39:
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_41
// %bb.40:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.414
	add	x0, x0, :lo12:.L.str.414
	bl	nand_print_dbg
	b	.LBB301_42
.LBB301_41:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.415
	add	x0, x0, :lo12:.L.str.415
	bl	nand_print_dbg
.LBB301_42:
	mov	w8, #10
	str	w8, [sp, #16]
	b	.LBB301_77
.LBB301_43:                             //   in Loop: Header=BB301_32 Depth=1
.LBB301_44:                             //   in Loop: Header=BB301_32 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB301_32
.LBB301_45:
.LBB301_46:
	stur	wzr, [x29, #-12]
.LBB301_47:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, sandisk_A19_read_retry_cycle
	ldrb	w9, [x9, :lo12:sandisk_A19_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB301_76
// %bb.48:                              //   in Loop: Header=BB301_47 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8, #8]
	bl	sandisk_A19_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB301_50
// %bb.49:                              //   in Loop: Header=BB301_47 Depth=1
	adrp	x0, .L.str.413
	add	x0, x0, :lo12:.L.str.413
	bl	nand_print_dbg
	b	.LBB301_75
.LBB301_50:                             //   in Loop: Header=BB301_47 Depth=1
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #52                     // =52
	b.ne	.LBB301_52
// %bb.51:                              //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	mov	w1, #-58
	bl	sandisk_A19_set_lmflgfix_next
	b	.LBB301_56
.LBB301_52:                             //   in Loop: Header=BB301_47 Depth=1
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #53                     // =53
	b.ne	.LBB301_54
// %bb.53:                              //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	mov	w1, #26
	bl	sandisk_A19_set_lmflgfix_next
	b	.LBB301_55
.LBB301_54:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	mov	w1, #-64
	bl	sandisk_A19_set_lmflgfix_next
.LBB301_55:                             //   in Loop: Header=BB301_47 Depth=1
.LBB301_56:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	bl	sandisk_A19_cmd25
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #50                     // =50
	b.eq	.LBB301_59
// %bb.57:                              //   in Loop: Header=BB301_47 Depth=1
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #52                     // =52
	b.eq	.LBB301_59
// %bb.58:                              //   in Loop: Header=BB301_47 Depth=1
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #53                     // =53
	b.ne	.LBB301_60
.LBB301_59:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	bl	sandisk_A19_dsp_on
.LBB301_60:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #52                     // =52
	b.ne	.LBB301_62
// %bb.61:                              //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	mov	w1, #-62
	bl	sandisk_A19_set_lmflgfix_next
	b	.LBB301_66
.LBB301_62:                             //   in Loop: Header=BB301_47 Depth=1
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #53                     // =53
	b.ne	.LBB301_64
// %bb.63:                              //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	mov	w1, #10
	bl	sandisk_A19_set_lmflgfix_next
	b	.LBB301_65
.LBB301_64:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	x0, [x29, #-8]
	mov	w1, #64
	bl	sandisk_A19_set_lmflgfix_next
.LBB301_65:                             //   in Loop: Header=BB301_47 Depth=1
.LBB301_66:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB301_68
// %bb.67:                              //   in Loop: Header=BB301_47 Depth=1
	mov	w8, #-2
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.403
	add	x0, x0, :lo12:.L.str.403
	bl	nand_print_dbg
	b	.LBB301_75
.LBB301_68:                             //   in Loop: Header=BB301_47 Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB301_70
// %bb.69:                              //   in Loop: Header=BB301_47 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_74
.LBB301_70:
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB301_72
// %bb.71:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.414
	add	x0, x0, :lo12:.L.str.414
	bl	nand_print_dbg
	b	.LBB301_73
.LBB301_72:
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.415
	add	x0, x0, :lo12:.L.str.415
	bl	nand_print_dbg
.LBB301_73:
	mov	w8, #10
	str	w8, [sp, #16]
	b	.LBB301_77
.LBB301_74:                             //   in Loop: Header=BB301_47 Depth=1
.LBB301_75:                             //   in Loop: Header=BB301_47 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB301_47
.LBB301_76:
.LBB301_77:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	sandisk_A19_exit_readretry
.LBB301_78:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end301:
	.size	sandisk_A19_read_page_end, .Lfunc_end301-sandisk_A19_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_set_readretry       // -- Begin function sandisk_A19_set_readretry
	.p2align	2
	.type	sandisk_A19_set_readretry,@function
sandisk_A19_set_readretry:              // @sandisk_A19_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	sturh	w1, [x29, #-10]
	stur	wzr, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #52                     // =52
	b.ne	.LBB302_2
// %bb.1:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	mov	w9, w8
	adrp	x8, sandisk_A19_1z_16g
	add	x8, x8, :lo12:sandisk_A19_1z_16g
	add	x8, x8, x9, lsl #2
	str	x8, [sp, #24]
	b	.LBB302_6
.LBB302_2:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #53                     // =53
	b.ne	.LBB302_4
// %bb.3:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	mov	w9, w8
	adrp	x8, sandisk_A19_1z_8g
	add	x8, x8, :lo12:sandisk_A19_1z_8g
	add	x8, x8, x9, lsl #2
	str	x8, [sp, #24]
	b	.LBB302_5
.LBB302_4:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	mov	w9, w8
	adrp	x8, sandisk_A19_1y
	add	x8, x8, :lo12:sandisk_A19_1y
	add	x8, x8, x9, lsl #2
	str	x8, [sp, #24]
.LBB302_5:
.LBB302_6:
	adrp	x8, sandisk_A19_read_retry_reg_adr_1y
	add	x8, x8, :lo12:sandisk_A19_read_retry_reg_adr_1y
	str	x8, [sp, #16]
	mov	w8, #1
	str	w8, [sp, #12]
	ldur	x0, [x29, #-8]
	ldr	x1, [sp, #24]
	ldr	x2, [sp, #16]
	ldr	w3, [sp, #12]
	bl	sandisk_A19_vender_set_param
	stur	w0, [x29, #-16]
	ldur	x0, [x29, #-8]
	mov	w1, #93
	mov	w2, wzr
	bl	set_one_cmd
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	w0, [x29, #-16]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end302:
	.size	sandisk_A19_set_readretry, .Lfunc_end302-sandisk_A19_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_exit_readretry      // -- Begin function sandisk_A19_exit_readretry
	.p2align	2
	.type	sandisk_A19_exit_readretry,@function
sandisk_A19_exit_readretry:             // @sandisk_A19_exit_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	sub	x8, x29, #16                    // =16
	str	x8, [sp, #8]                    // 8-byte Folded Spill
	stur	wzr, [x29, #-16]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x0, [x29, #-8]
	mov	w1, #-1
	mov	w2, #1
	str	w2, [sp, #4]                    // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	adrp	x9, sandisk_A19_read_retry_reg_adr_1y
	add	x9, x9, :lo12:sandisk_A19_read_retry_reg_adr_1y
	str	x9, [sp, #24]
	str	w8, [sp, #20]
	ldur	x0, [x29, #-8]
	ldr	x2, [sp, #24]
	ldr	w3, [sp, #20]
	bl	sandisk_A19_vender_set_param
	stur	w0, [x29, #-12]
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end303:
	.size	sandisk_A19_exit_readretry, .Lfunc_end303-sandisk_A19_exit_readretry
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_readretry_init      // -- Begin function sandisk_A19_readretry_init
	.p2align	2
	.type	sandisk_A19_readretry_init,@function
sandisk_A19_readretry_init:             // @sandisk_A19_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp]
	ldr	x8, [sp]
	str	wzr, [x8, #184]
	ldr	x8, [sp]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w9, w8, #0xff
	adrp	x8, sandisk_A19_read_retry_mode
	strb	w9, [x8, :lo12:sandisk_A19_read_retry_mode]
	ldr	x9, [sp]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	lsr	w9, w9, #8
	and	w9, w9, #0xff
	adrp	x10, sandisk_A19_read_retry_cycle
	strb	w9, [x10, :lo12:sandisk_A19_read_retry_cycle]
	ldr	x9, [sp]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, sandisk_A19_read_retry_reg_cnt
	strb	w9, [x10, :lo12:sandisk_A19_read_retry_reg_cnt]
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #50                     // =50
	b.eq	.LBB304_5
// %bb.1:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #51                     // =51
	b.eq	.LBB304_5
// %bb.2:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #52                     // =52
	b.eq	.LBB304_5
// %bb.3:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w8, [x8, :lo12:sandisk_A19_read_retry_mode]
	subs	w8, w8, #53                     // =53
	b.eq	.LBB304_5
// %bb.4:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w1, [x8, :lo12:sandisk_A19_read_retry_mode]
	adrp	x0, .L.str.416
	add	x0, x0, :lo12:.L.str.416
	bl	nand_print
	mov	w8, #-99
	stur	w8, [x29, #-4]
	b	.LBB304_6
.LBB304_5:
	adrp	x8, sandisk_A19_read_retry_cycle
	ldrb	w1, [x8, :lo12:sandisk_A19_read_retry_cycle]
	adrp	x0, .L.str.417
	add	x0, x0, :lo12:.L.str.417
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
.LBB304_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end304:
	.size	sandisk_A19_readretry_init, .Lfunc_end304-sandisk_A19_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_readretry_exit      // -- Begin function sandisk_A19_readretry_exit
	.p2align	2
	.type	sandisk_A19_readretry_exit,@function
sandisk_A19_readretry_exit:             // @sandisk_A19_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	mov	w0, wzr
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end305:
	.size	sandisk_A19_readretry_exit, .Lfunc_end305-sandisk_A19_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_read_retry_clock_save // -- Begin function sandisk_A19_read_retry_clock_save
	.p2align	2
	.type	sandisk_A19_read_retry_clock_save,@function
sandisk_A19_read_retry_clock_save:      // @sandisk_A19_read_retry_clock_save
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	str	x0, [sp]                        // 8-byte Folded Spill
	adrp	x2, sandisk_A19_sclk0_bak
	add	x2, x2, :lo12:sandisk_A19_sclk0_bak
	adrp	x3, sandisk_A19_sclk1_bak
	add	x3, x3, :lo12:sandisk_A19_sclk1_bak
	bl	nand_get_clk
	ldr	x0, [sp]                        // 8-byte Folded Reload
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	mov	w2, #10
	mov	w3, #20
	bl	nand_set_clk
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end306:
	.size	sandisk_A19_read_retry_clock_save, .Lfunc_end306-sandisk_A19_read_retry_clock_save
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_read_retry_clock_recover // -- Begin function sandisk_A19_read_retry_clock_recover
	.p2align	2
	.type	sandisk_A19_read_retry_clock_recover,@function
sandisk_A19_read_retry_clock_recover:   // @sandisk_A19_read_retry_clock_recover
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x8, sandisk_A19_sclk0_bak
	ldr	w2, [x8, :lo12:sandisk_A19_sclk0_bak]
	adrp	x8, sandisk_A19_sclk1_bak
	ldr	w3, [x8, :lo12:sandisk_A19_sclk1_bak]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_set_clk
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end307:
	.size	sandisk_A19_read_retry_clock_recover, .Lfunc_end307-sandisk_A19_read_retry_clock_recover
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_vender_set_param    // -- Begin function sandisk_A19_vender_set_param
	.p2align	2
	.type	sandisk_A19_vender_set_param,@function
sandisk_A19_vender_set_param:           // @sandisk_A19_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	str	x1, [sp, #16]
	str	x2, [sp, #8]
	str	w3, [sp, #4]
	str	wzr, [sp]
	ldur	x0, [x29, #-8]
	ldr	x3, [sp, #8]
	ldr	x4, [sp, #16]
	adrp	x1, sandisk_A19_read_retry_cmd_1y
	add	x1, x1, :lo12:sandisk_A19_read_retry_cmd_1y
	mov	w6, #1
	mov	w2, w6
	mov	w5, #4
	bl	set_cmd_with_nand_bus
	str	w0, [sp]
	ldr	w0, [sp]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end308:
	.size	sandisk_A19_vender_set_param, .Lfunc_end308-sandisk_A19_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_special_init        // -- Begin function sandisk_A19_special_init
	.p2align	2
	.type	sandisk_A19_special_init,@function
sandisk_A19_special_init:               // @sandisk_A19_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB309_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB309_3
// %bb.2:                               //   in Loop: Header=BB309_1 Depth=1
	ldr	x0, [sp]
	bl	sandisk_A19_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB309_1
.LBB309_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB309_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, sandisk_A19_read_page_end
	add	x8, x8, :lo12:sandisk_A19_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w1, [x8, :lo12:sandisk_A19_read_retry_mode]
	adrp	x8, sandisk_A19_read_retry_cycle
	ldrb	w2, [x8, :lo12:sandisk_A19_read_retry_cycle]
	adrp	x8, sandisk_A19_read_retry_reg_cnt
	ldrb	w3, [x8, :lo12:sandisk_A19_read_retry_reg_cnt]
	adrp	x0, .L.str.418
	add	x0, x0, :lo12:.L.str.418
	bl	nand_print_dbg
	b	.LBB309_6
.LBB309_5:
	adrp	x8, sandisk_A19_read_retry_mode
	ldrb	w1, [x8, :lo12:sandisk_A19_read_retry_mode]
	adrp	x8, sandisk_A19_read_retry_cycle
	ldrb	w2, [x8, :lo12:sandisk_A19_read_retry_cycle]
	adrp	x8, sandisk_A19_read_retry_reg_cnt
	ldrb	w3, [x8, :lo12:sandisk_A19_read_retry_reg_cnt]
	adrp	x0, .L.str.419
	add	x0, x0, :lo12:.L.str.419
	bl	nand_print
.LBB309_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end309:
	.size	sandisk_A19_special_init, .Lfunc_end309-sandisk_A19_special_init
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_special_exit        // -- Begin function sandisk_A19_special_exit
	.p2align	2
	.type	sandisk_A19_special_exit,@function
sandisk_A19_special_exit:               // @sandisk_A19_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #16]
.LBB310_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	cbz	x8, .LBB310_7
// %bb.2:                               //   in Loop: Header=BB310_1 Depth=1
	ldr	x0, [sp, #16]
	bl	sandisk_A19_readretry_exit
	ldr	x8, [sp, #16]
	ldrb	w8, [x8, #270]
	cbz	w8, .LBB310_6
// %bb.3:                               //   in Loop: Header=BB310_1 Depth=1
	ldr	x8, [sp, #16]
	str	wzr, [x8, #204]
	ldr	x0, [sp, #16]
	bl	sandisk_toggle_vendor_specific_setting
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbz	w8, .LBB310_5
// %bb.4:
	adrp	x0, .L.str.420
	add	x0, x0, :lo12:.L.str.420
	adrp	x1, .L__func__.sandisk_A19_special_exit
	add	x1, x1, :lo12:.L__func__.sandisk_A19_special_exit
	bl	nand_print
	ldr	w8, [sp, #12]
	stur	w8, [x29, #-4]
	b	.LBB310_8
.LBB310_5:                              //   in Loop: Header=BB310_1 Depth=1
.LBB310_6:                              //   in Loop: Header=BB310_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	x8, [x8]
	str	x8, [sp, #16]
	b	.LBB310_1
.LBB310_7:
	adrp	x0, .L.str.421
	add	x0, x0, :lo12:.L.str.421
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
.LBB310_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end310:
	.size	sandisk_A19_special_exit, .Lfunc_end310-sandisk_A19_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	sandisk_A19_check_bad_block_first_burn // -- Begin function sandisk_A19_check_bad_block_first_burn
	.p2align	2
	.type	sandisk_A19_check_bad_block_first_burn,@function
sandisk_A19_check_bad_block_first_burn: // @sandisk_A19_check_bad_block_first_burn
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-24]
	stur	wzr, [x29, #-28]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-16]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #280]
	str	x8, [sp, #16]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #280]
	add	x8, x8, #40                     // =40
	str	x8, [sp, #8]
	ldr	x8, [sp, #24]
	ldr	w8, [x8, #28]
	ldr	x9, [sp, #16]
	ldr	w9, [x9, #16]
	subs	w8, w8, w9
	b.hs	.LBB311_2
// %bb.1:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #4]
	ldr	x9, [sp, #24]
	ldr	w9, [x9, #32]
	subs	w8, w8, w9
	b.lo	.LBB311_3
.LBB311_2:
	ldr	x8, [sp, #16]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #24]
	ldr	w2, [x8, #28]
	ldr	x8, [sp, #16]
	ldr	w3, [x8, #16]
	ldur	x8, [x29, #-16]
	ldr	w4, [x8, #4]
	ldr	x8, [sp, #24]
	ldr	w5, [x8, #32]
	adrp	x0, .L.str.422
	add	x0, x0, :lo12:.L.str.422
	bl	nand_print
	mov	w8, #-10
	stur	w8, [x29, #-4]
	b	.LBB311_4
.LBB311_3:
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x0, [sp, #24]
	bl	nand_read_chip_status_ready
	ldr	x0, [sp, #24]
	bl	nand_enable_chip
	ldr	x0, [sp, #8]
	bl	ndfc_clean_cmd_seq
	ldr	x8, [sp, #8]
	str	wzr, [x8]
	ldr	x9, [sp, #8]
	mov	w8, #162
	str	w8, [x9, #12]
	ldr	x9, [sp, #8]
	mov	w8, #1
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	str	w8, [x9, #8]
	ldr	x9, [sp, #8]
	str	w8, [x9, #16]
	ldr	x9, [sp, #8]
	str	wzr, [x9, #20]
	ldr	x10, [sp, #8]
	mov	w9, #128
	str	w9, [x10, #84]
	ldr	x9, [sp, #8]
	str	w8, [x9, #80]
	ldr	x9, [sp, #8]
	str	w8, [x9, #88]
	ldr	x8, [sp, #8]
	str	wzr, [x8, #92]
	ldr	x8, [sp, #24]
	ldr	w0, [x8, #44]
	ldur	x8, [x29, #-16]
	ldr	w1, [x8, #4]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #8]
	bl	get_row_addr
	stur	w0, [x29, #-24]
	ldr	x9, [sp, #8]
	mov	w8, #5
	str	w8, [x9, #112]
	ldur	w0, [x29, #-28]
	ldur	w2, [x29, #-24]
	ldr	x8, [sp, #8]
	add	x4, x8, #96                     // =96
	mov	w1, #2
	mov	w3, #3
	bl	fill_cmd_addr
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	ldr	x10, [sp, #8]
	mov	w9, #16
	str	w9, [x10, #156]
	ldr	x9, [sp, #8]
	str	w8, [x9, #152]
	ldr	x9, [sp, #8]
	str	w8, [x9, #160]
	ldr	x9, [sp, #8]
	str	w8, [x9, #164]
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	ldr	x1, [sp, #8]
	bl	ndfc_execute_cmd
	stur	w0, [x29, #-20]
	ldr	x0, [sp, #24]
	bl	nand_read_chip_status_ready
	stur	w0, [x29, #-20]
	ldr	x0, [sp, #24]
	bl	nand_disable_chip
	ldur	w8, [x29, #-20]
	stur	w8, [x29, #-4]
.LBB311_4:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end311:
	.size	sandisk_A19_check_bad_block_first_burn, .Lfunc_end311-sandisk_A19_check_bad_block_first_burn
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_vender_set_param      // -- Begin function hynix20nm_vender_set_param
	.p2align	2
	.type	hynix20nm_vender_set_param,@function
hynix20nm_vender_set_param:             // @hynix20nm_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #8]
	mov	w8, #54
	strb	w8, [sp, #15]
	mov	w8, #22
	strb	w8, [sp, #14]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	str	wzr, [sp, #16]
.LBB312_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hs	.LBB312_4
// %bb.2:                               //   in Loop: Header=BB312_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x3, x8, x9
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x4, x8, x9
	add	x1, sp, #15                     // =15
	mov	w2, wzr
	str	w2, [sp, #4]                    // 4-byte Folded Spill
	mov	w6, #1
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w2, [sp, #4]                    // 4-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldrb	w1, [sp, #14]
	bl	set_one_cmd
// %bb.3:                               //   in Loop: Header=BB312_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB312_1
.LBB312_4:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldr	w0, [sp, #8]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end312:
	.size	hynix20nm_vender_set_param, .Lfunc_end312-hynix20nm_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_set_readretry         // -- Begin function hynix20nm_set_readretry
	.p2align	2
	.type	hynix20nm_set_readretry,@function
hynix20nm_set_readretry:                // @hynix20nm_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	wzr, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	adrp	x9, hynix20nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ls	.LBB313_2
// %bb.1:
	mov	w8, #-92
	stur	w8, [x29, #-4]
	b	.LBB313_11
.LBB313_2:
	str	wzr, [sp, #12]
.LBB313_3:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	adrp	x9, hynix20nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB313_6
// %bb.4:                               //   in Loop: Header=BB313_3 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #56                     // =56
	ldr	x9, [sp, #16]
	ldr	w10, [x9, #184]
	mov	w9, #8
	mul	w9, w9, w10
	ldr	w10, [sp, #12]
	add	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	w9, [sp, #12]
	mov	w10, w9
	mov	x9, sp
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.5:                               //   in Loop: Header=BB313_3 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB313_3
.LBB313_6:
	ldr	x0, [sp, #16]
	adrp	x8, hynix20nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_read_retry_reg_num]
	mov	x1, sp
	adrp	x2, hynix20nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix20nm_read_retry_reg_adr
	bl	hynix20nm_vender_set_param
	str	w0, [sp, #8]
	adrp	x0, .L.str.423
	add	x0, x0, :lo12:.L.str.423
	bl	nand_print_dbg
	str	wzr, [sp, #12]
.LBB313_7:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	adrp	x9, hynix20nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB313_10
// %bb.8:                               //   in Loop: Header=BB313_7 Depth=1
	ldr	w8, [sp, #12]
	mov	w9, w8
	mov	x8, sp
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.381
	add	x0, x0, :lo12:.L.str.381
	bl	nand_print_dbg
// %bb.9:                               //   in Loop: Header=BB313_7 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB313_7
.LBB313_10:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
	ldr	w8, [sp, #8]
	stur	w8, [x29, #-4]
.LBB313_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end313:
	.size	hynix20nm_set_readretry, .Lfunc_end313-hynix20nm_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_read_page_end         // -- Begin function hynix20nm_read_page_end
	.p2align	2
	.type	hynix20nm_read_page_end,@function
hynix20nm_read_page_end:                // @hynix20nm_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB314_15
// %bb.1:
	adrp	x0, .L.str.424
	add	x0, x0, :lo12:.L.str.424
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB314_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, hynix20nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB314_14
// %bb.3:                               //   in Loop: Header=BB314_2 Depth=1
	str	wzr, [sp, #16]
	ldr	x9, [sp, #8]
	ldr	w8, [x9, #184]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #184]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #184]
	adrp	x9, hynix20nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ls	.LBB314_5
// %bb.4:                               //   in Loop: Header=BB314_2 Depth=1
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
.LBB314_5:                              //   in Loop: Header=BB314_2 Depth=1
	ldr	x0, [sp, #8]
	bl	hynix20nm_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB314_7
// %bb.6:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	hynix20nm_set_readretry
	b	.LBB314_14
.LBB314_7:                              //   in Loop: Header=BB314_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #24]
	bl	is_nouse_page
	subs	w8, w0, #1                      // =1
	b.ne	.LBB314_9
// %bb.8:                               //   in Loop: Header=BB314_2 Depth=1
	mov	w8, #-2
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.403
	add	x0, x0, :lo12:.L.str.403
	bl	nand_print_dbg
	b	.LBB314_13
.LBB314_9:                              //   in Loop: Header=BB314_2 Depth=1
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB314_11
// %bb.10:                              //   in Loop: Header=BB314_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB314_12
.LBB314_11:
	mov	w8, #10
	str	w8, [sp, #16]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #24]
	ldrb	w1, [x8, #1]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #24]
	ldrb	w2, [x8, #2]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #24]
	ldrb	w3, [x8, #3]
	ldur	x8, [x29, #-8]
	ldr	x8, [x8, #24]
	ldrb	w4, [x8, #4]
	adrp	x0, .L.str.425
	add	x0, x0, :lo12:.L.str.425
	bl	nand_print_dbg
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.426
	add	x0, x0, :lo12:.L.str.426
	bl	nand_print_dbg
	b	.LBB314_14
.LBB314_12:                             //   in Loop: Header=BB314_2 Depth=1
.LBB314_13:                             //   in Loop: Header=BB314_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB314_2
.LBB314_14:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	hynix20nm_set_readretry
.LBB314_15:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end314:
	.size	hynix20nm_read_page_end, .Lfunc_end314-hynix20nm_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_vender_get_param      // -- Begin function hynix20nm_vender_get_param
	.p2align	2
	.type	hynix20nm_vender_get_param,@function
hynix20nm_vender_get_param:             // @hynix20nm_vender_get_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #12]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	mov	w8, #55
	strb	w8, [sp, #11]
	str	wzr, [sp, #16]
.LBB315_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hs	.LBB315_4
// %bb.2:                               //   in Loop: Header=BB315_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x2, x8, x9
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x3, x8, x9
	add	x1, sp, #11                     // =11
	mov	w4, #1
	bl	get_data_with_nand_bus_one_cmd
// %bb.3:                               //   in Loop: Header=BB315_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB315_1
.LBB315_4:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end315:
	.size	hynix20nm_vender_get_param, .Lfunc_end315-hynix20nm_vender_get_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_lsb_init              // -- Begin function hynix20nm_lsb_init
	.p2align	2
	.type	hynix20nm_lsb_init,@function
hynix20nm_lsb_init:                     // @hynix20nm_lsb_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	adrp	x8, hynix20nm_lsb_mode_reg_adr
	add	x8, x8, :lo12:hynix20nm_lsb_mode_reg_adr
	str	x8, [sp, #8]                    // 8-byte Folded Spill
	adrp	x8, hynix20nm_lsb_mode_val
	add	x8, x8, :lo12:hynix20nm_lsb_mode_val
	str	x8, [sp, #16]                   // 8-byte Folded Spill
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w9, w8, #0xff
	adrp	x8, hynix20nm_read_retry_mode
	strb	w9, [x8, :lo12:hynix20nm_read_retry_mode]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	lsr	w9, w9, #8
	and	w9, w9, #0xff
	adrp	x10, hynix20nm_read_retry_cycle
	strb	w9, [x10, :lo12:hynix20nm_read_retry_cycle]
	ldr	x9, [sp, #24]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, hynix20nm_read_retry_reg_num
	strb	w9, [x10, :lo12:hynix20nm_read_retry_reg_num]
	ldrb	w8, [x8, :lo12:hynix20nm_read_retry_mode]
	subs	w8, w8, #2                      // =2
	b.ne	.LBB316_2
// %bb.1:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	ldr	x10, [sp, #8]                   // 8-byte Folded Reload
	adrp	x11, hynix20nm_lsb_mode_reg_num
	mov	w8, #4
	strb	w8, [x11, :lo12:hynix20nm_lsb_mode_reg_num]
	mov	w8, #176
	adrp	x11, hynix20nm_lsb_mode_reg_adr
	strb	w8, [x11, :lo12:hynix20nm_lsb_mode_reg_adr]
	mov	w8, #177
	strb	w8, [x10, #1]
	mov	w8, #160
	strb	w8, [x10, #2]
	mov	w8, #161
	strb	w8, [x10, #3]
	adrp	x10, hynix20nm_lsb_mode_val
	mov	w8, #10
	strb	w8, [x10, :lo12:hynix20nm_lsb_mode_val]
	strb	w8, [x9, #1]
	strb	w8, [x9, #2]
	strb	w8, [x9, #3]
	b	.LBB316_5
.LBB316_2:
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w8, [x8, :lo12:hynix20nm_read_retry_mode]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB316_4
// %bb.3:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	ldr	x10, [sp, #8]                   // 8-byte Folded Reload
	adrp	x11, hynix20nm_lsb_mode_reg_num
	mov	w8, #4
	strb	w8, [x11, :lo12:hynix20nm_lsb_mode_reg_num]
	adrp	x11, hynix20nm_lsb_mode_reg_adr
	mov	w8, #160
	strb	w8, [x11, :lo12:hynix20nm_lsb_mode_reg_adr]
	mov	w8, #161
	strb	w8, [x10, #1]
	mov	w8, #167
	strb	w8, [x10, #2]
	mov	w8, #168
	strb	w8, [x10, #3]
	adrp	x10, hynix20nm_lsb_mode_val
	mov	w8, #10
	strb	w8, [x10, :lo12:hynix20nm_lsb_mode_val]
	strb	w8, [x9, #1]
	strb	w8, [x9, #2]
	strb	w8, [x9, #3]
.LBB316_4:
.LBB316_5:
	mov	w0, wzr
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end316:
	.size	hynix20nm_lsb_init, .Lfunc_end316-hynix20nm_lsb_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_lsb_enable            // -- Begin function hynix20nm_lsb_enable
	.p2align	2
	.type	hynix20nm_lsb_enable,@function
hynix20nm_lsb_enable:                   // @hynix20nm_lsb_enable
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	adrp	x8, hynix20nm_lsb_mode_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_lsb_mode_reg_num]
	adrp	x1, hynix20nm_lsb_mode_default_val
	add	x1, x1, :lo12:hynix20nm_lsb_mode_default_val
	adrp	x2, hynix20nm_lsb_mode_reg_adr
	add	x2, x2, :lo12:hynix20nm_lsb_mode_reg_adr
	bl	hynix20nm_vender_get_param
	str	wzr, [sp, #4]
.LBB317_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	adrp	x9, hynix20nm_lsb_mode_reg_num
	ldrb	w9, [x9, :lo12:hynix20nm_lsb_mode_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB317_4
// %bb.2:                               //   in Loop: Header=BB317_1 Depth=1
	ldr	w8, [sp, #4]
	mov	w9, w8
	adrp	x8, hynix20nm_lsb_mode_default_val
	add	x8, x8, :lo12:hynix20nm_lsb_mode_default_val
	add	x8, x8, x9
	ldrb	w9, [x8]
	ldr	w8, [sp, #4]
	mov	w11, w8
	mov	x8, sp
	mov	x10, x8
	add	x10, x10, x11
	strb	w9, [x10]
	ldr	w9, [sp, #4]
	mov	w10, w9
	adrp	x9, hynix20nm_lsb_mode_val
	add	x9, x9, :lo12:hynix20nm_lsb_mode_val
	ldrb	w10, [x9, x10]
	ldr	w9, [sp, #4]
                                        // kill: def $x9 killed $w9
	add	x9, x8, x9
	ldrb	w8, [x9]
	add	w8, w8, w10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB317_1 Depth=1
	ldr	w8, [sp, #4]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #4]
	b	.LBB317_1
.LBB317_4:
	ldr	x0, [sp, #8]
	adrp	x8, hynix20nm_lsb_mode_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_lsb_mode_reg_num]
	mov	x1, sp
	adrp	x2, hynix20nm_lsb_mode_reg_adr
	add	x2, x2, :lo12:hynix20nm_lsb_mode_reg_adr
	bl	hynix20nm_vender_set_param
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end317:
	.size	hynix20nm_lsb_enable, .Lfunc_end317-hynix20nm_lsb_enable
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_lsb_disable           // -- Begin function hynix20nm_lsb_disable
	.p2align	2
	.type	hynix20nm_lsb_disable,@function
hynix20nm_lsb_disable:                  // @hynix20nm_lsb_disable
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x0, [sp, #8]
	adrp	x8, hynix20nm_lsb_mode_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_lsb_mode_reg_num]
	adrp	x1, hynix20nm_lsb_mode_default_val
	add	x1, x1, :lo12:hynix20nm_lsb_mode_default_val
	adrp	x2, hynix20nm_lsb_mode_reg_adr
	add	x2, x2, :lo12:hynix20nm_lsb_mode_reg_adr
	bl	hynix20nm_vender_set_param
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end318:
	.size	hynix20nm_lsb_disable, .Lfunc_end318-hynix20nm_lsb_disable
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_lsb_exit              // -- Begin function hynix20nm_lsb_exit
	.p2align	2
	.type	hynix20nm_lsb_exit,@function
hynix20nm_lsb_exit:                     // @hynix20nm_lsb_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	mov	w8, #1048575
	str	w8, [sp, #4]
	ldr	x0, [sp, #8]
	bl	nand_enable_chip
	ldr	x8, [sp, #8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x0, [sp, #8]
	mov	w2, wzr
	str	w2, [sp]                        // 4-byte Folded Spill
	mov	w1, w2
	bl	set_one_cmd
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	w1, [sp]                        // 4-byte Folded Reload
	ldr	x0, [sp, #8]
	bl	set_one_addr
	ldr	x0, [sp, #8]
	mov	w1, #48
	mov	w2, #1
	bl	set_one_cmd
.LBB319_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #4]
	subs	w9, w8, #1                      // =1
	str	w9, [sp, #4]
	subs	w8, w8, #0                      // =0
	b.le	.LBB319_3
// %bb.2:                               //   in Loop: Header=BB319_1 Depth=1
	b	.LBB319_1
.LBB319_3:
	adrp	x0, .L.str.427
	add	x0, x0, :lo12:.L.str.427
	bl	nand_print_dbg
	ldr	x0, [sp, #8]
	bl	nand_disable_chip
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end319:
	.size	hynix20nm_lsb_exit, .Lfunc_end319-hynix20nm_lsb_exit
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_vender_get_param_otp_hynix // -- Begin function hynix20nm_vender_get_param_otp_hynix
	.p2align	2
	.type	hynix20nm_vender_get_param_otp_hynix,@function
hynix20nm_vender_get_param_otp_hynix:   // @hynix20nm_vender_get_param_otp_hynix
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #192                    // =192
	stp	x29, x30, [sp, #176]            // 16-byte Folded Spill
	add	x29, sp, #176                   // =176
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-32]
	strh	wzr, [sp, #76]
	strh	wzr, [sp, #74]
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w8, [x8, :lo12:hynix20nm_read_retry_mode]
	subs	w8, w8, #2                      // =2
	b.ne	.LBB320_2
// %bb.1:
	mov	w8, #255
	strb	w8, [sp, #76]
	mov	w8, #204
	strb	w8, [sp, #77]
	mov	w8, #64
	strb	w8, [sp, #74]
	mov	w8, #77
	strb	w8, [sp, #75]
	b	.LBB320_6
.LBB320_2:
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w8, [x8, :lo12:hynix20nm_read_retry_mode]
	subs	w8, w8, #3                      // =3
	b.ne	.LBB320_4
// %bb.3:
	mov	w8, #174
	strb	w8, [sp, #76]
	mov	w8, #176
	strb	w8, [sp, #77]
	strb	wzr, [sp, #74]
	mov	w8, #77
	strb	w8, [sp, #75]
	b	.LBB320_5
.LBB320_4:
	mov	w8, #-97
	stur	w8, [x29, #-4]
	b	.LBB320_24
.LBB320_5:
.LBB320_6:
	mov	w0, #1024
	bl	nand_get_temp_buf
	str	x0, [sp, #64]
	ldur	x0, [x29, #-16]
	bl	nand_enable_chip
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x0, [x29, #-16]
	mov	w1, #-1
	mov	w2, #1
	str	w2, [sp, #36]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	w6, [sp, #36]                   // 4-byte Folded Reload
	sub	x1, x29, #33                    // =33
	str	x1, [sp, #48]                   // 8-byte Folded Spill
	mov	w8, #54
	sturb	w8, [x29, #-33]
	ldur	x0, [x29, #-16]
	mov	w2, wzr
	str	w2, [sp, #40]                   // 4-byte Folded Spill
	add	x3, sp, #76                     // =76
	str	x3, [sp, #16]                   // 8-byte Folded Spill
	add	x4, sp, #74                     // =74
	str	x4, [sp, #24]                   // 8-byte Folded Spill
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	ldr	x8, [sp, #24]                   // 8-byte Folded Reload
	ldr	w6, [sp, #36]                   // 4-byte Folded Reload
	ldr	w2, [sp, #40]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	add	x3, x9, #1                      // =1
	add	x4, x8, #1                      // =1
	mov	x1, xzr
	str	x1, [sp, #56]                   // 8-byte Folded Spill
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w2, [sp, #40]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #22
	bl	set_one_cmd
	ldr	w2, [sp, #40]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #23
	bl	set_one_cmd
	ldr	w2, [sp, #40]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #4
	bl	set_one_cmd
	ldr	w2, [sp, #40]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #25
	bl	set_one_cmd
	ldr	w2, [sp, #40]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, wzr
	str	w1, [sp, #44]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	w1, [sp, #44]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldr	w1, [sp, #44]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldr	w1, [sp, #44]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	x0, [x29, #-16]
	mov	w1, #2
	bl	set_one_addr
	ldr	w1, [sp, #44]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldr	x1, [sp, #48]                   // 8-byte Folded Reload
	ldr	x2, [sp, #56]                   // 8-byte Folded Reload
	mov	w8, #48
	sturb	w8, [x29, #-33]
	ldur	x0, [x29, #-16]
	ldr	x3, [sp, #64]
	mov	w4, #2
	bl	get_data_with_nand_bus_one_cmd
	ldr	x8, [sp, #64]
	ldrb	w8, [x8]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB320_8
// %bb.7:
	ldr	x8, [sp, #64]
	ldrb	w8, [x8, #1]
	subs	w8, w8, #8                      // =8
	b.eq	.LBB320_9
.LBB320_8:
	ldr	x8, [sp, #64]
	ldrb	w1, [x8]
	ldr	x8, [sp, #64]
	ldrb	w2, [x8, #1]
	adrp	x0, .L.str.428
	add	x0, x0, :lo12:.L.str.428
	bl	nand_print
	mov	w8, #-96
	stur	w8, [x29, #-32]
.LBB320_9:
	ldur	x0, [x29, #-16]
	ldr	x3, [sp, #64]
	mov	x2, xzr
	mov	x1, x2
	mov	w4, #1024
	bl	get_data_with_nand_bus_one_cmd
	stur	wzr, [x29, #-24]
.LBB320_10:                             // =>This Loop Header: Depth=1
                                        //     Child Loop BB320_12 Depth 2
	ldur	w8, [x29, #-24]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB320_21
// %bb.11:                              //   in Loop: Header=BB320_10 Depth=1
	stur	wzr, [x29, #-28]
	stur	wzr, [x29, #-20]
.LBB320_12:                             //   Parent Loop BB320_10 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-20]
	subs	w8, w8, #64                     // =64
	b.hs	.LBB320_17
// %bb.13:                              //   in Loop: Header=BB320_12 Depth=2
	ldr	x8, [sp, #64]
	ldur	w10, [x29, #-24]
	mov	w9, #128
	mul	w10, w9, w10
                                        // kill: def $x10 killed $w10
	add	x8, x8, x10
	ldur	w10, [x29, #-20]
                                        // kill: def $x10 killed $w10
	add	x8, x8, x10
	ldrb	w8, [x8]
	ldur	x10, [x29, #-16]
	add	x10, x10, #56                   // =56
	ldur	w11, [x29, #-20]
                                        // kill: def $x11 killed $w11
	add	x10, x10, x11
	strb	w8, [x10]
	ldr	x8, [sp, #64]
	ldur	w10, [x29, #-24]
	mul	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	add	x8, x8, #64                     // =64
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldur	w9, [x29, #-20]
	mov	w11, w9
	add	x9, sp, #79                     // =79
	mov	x10, x9
	add	x10, x10, x11
	strb	w8, [x10]
	ldur	x8, [x29, #-16]
	add	x8, x8, #56                     // =56
	ldur	w10, [x29, #-20]
                                        // kill: def $x10 killed $w10
	ldrb	w8, [x8, x10]
	ldur	w10, [x29, #-20]
                                        // kill: def $x10 killed $w10
	ldrb	w9, [x9, x10]
	add	w8, w8, w9
	subs	w8, w8, #255                    // =255
	b.eq	.LBB320_15
// %bb.14:                              //   in Loop: Header=BB320_10 Depth=1
	mov	w8, #1
	stur	w8, [x29, #-28]
	b	.LBB320_17
.LBB320_15:                             //   in Loop: Header=BB320_12 Depth=2
// %bb.16:                              //   in Loop: Header=BB320_12 Depth=2
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB320_12
.LBB320_17:                             //   in Loop: Header=BB320_10 Depth=1
	ldur	w8, [x29, #-28]
	cbnz	w8, .LBB320_19
// %bb.18:
	ldur	w1, [x29, #-24]
	adrp	x0, .L.str.429
	add	x0, x0, :lo12:.L.str.429
	bl	nand_print_dbg
	b	.LBB320_21
.LBB320_19:                             //   in Loop: Header=BB320_10 Depth=1
// %bb.20:                              //   in Loop: Header=BB320_10 Depth=1
	ldur	w8, [x29, #-24]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-24]
	b	.LBB320_10
.LBB320_21:
	ldur	w8, [x29, #-28]
	cbz	w8, .LBB320_23
// %bb.22:
	adrp	x0, .L.str.430
	add	x0, x0, :lo12:.L.str.430
	bl	nand_print_dbg
	mov	w8, #-95
	stur	w8, [x29, #-32]
.LBB320_23:
	ldur	x0, [x29, #-16]
	mov	w1, #-1
	mov	w2, #1
	str	w2, [sp, #12]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #56
	bl	set_one_cmd
	ldur	x0, [x29, #-16]
	bl	nand_disable_chip
	ldr	x0, [sp, #64]
	bl	nand_free_temp_buf
	ldur	w8, [x29, #-32]
	stur	w8, [x29, #-4]
.LBB320_24:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #176]            // 16-byte Folded Reload
	add	sp, sp, #192                    // =192
	ret
.Lfunc_end320:
	.size	hynix20nm_vender_get_param_otp_hynix, .Lfunc_end320-hynix20nm_vender_get_param_otp_hynix
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_saveotpvalue          // -- Begin function hynix20nm_saveotpvalue
	.p2align	2
	.type	hynix20nm_saveotpvalue,@function
hynix20nm_saveotpvalue:                 // @hynix20nm_saveotpvalue
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	str	wzr, [sp, #12]
.LBB321_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w8, w8, #64                     // =64
	b.hs	.LBB321_4
// %bb.2:                               //   in Loop: Header=BB321_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	w9, [sp, #12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	x9, [sp, #24]
	add	x9, x9, #56                     // =56
	ldr	w10, [sp, #12]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB321_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB321_1
.LBB321_4:
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end321:
	.size	hynix20nm_saveotpvalue, .Lfunc_end321-hynix20nm_saveotpvalue
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_getotpparam           // -- Begin function hynix20nm_getotpparam
	.p2align	2
	.type	hynix20nm_getotpparam,@function
hynix20nm_getotpparam:                  // @hynix20nm_getotpparam
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	str	x1, [sp, #24]
	str	wzr, [sp, #8]
.LBB322_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #8]
	subs	w8, w8, #5                      // =5
	b.hs	.LBB322_6
// %bb.2:                               //   in Loop: Header=BB322_1 Depth=1
	ldr	w1, [sp, #8]
	adrp	x0, .L.str.431
	add	x0, x0, :lo12:.L.str.431
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	bl	hynix20nm_vender_get_param_otp_hynix
	str	w0, [sp, #20]
	ldr	w8, [sp, #20]
	cbnz	w8, .LBB322_4
// %bb.3:
	b	.LBB322_6
.LBB322_4:                              //   in Loop: Header=BB322_1 Depth=1
// %bb.5:                               //   in Loop: Header=BB322_1 Depth=1
	ldr	w8, [sp, #8]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #8]
	b	.LBB322_1
.LBB322_6:
	ldr	w8, [sp, #20]
	cbz	w8, .LBB322_8
// %bb.7:
	adrp	x0, .L.str.432
	add	x0, x0, :lo12:.L.str.432
	bl	nand_print
	ldr	w8, [sp, #20]
	stur	w8, [x29, #-4]
	b	.LBB322_17
.LBB322_8:
	str	wzr, [sp, #16]
.LBB322_9:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB322_11 Depth 2
	ldr	w8, [sp, #16]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB322_16
// %bb.10:                              //   in Loop: Header=BB322_9 Depth=1
	str	wzr, [sp, #12]
.LBB322_11:                             //   Parent Loop BB322_9 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB322_14
// %bb.12:                              //   in Loop: Header=BB322_11 Depth=2
	ldur	x8, [x29, #-16]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #16]
	mov	w10, #8
	mul	w9, w10, w9
	ldr	w11, [sp, #12]
	add	w9, w9, w11
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	x9, [sp, #24]
	ldr	w11, [sp, #16]
	mul	w10, w10, w11
	ldr	w11, [sp, #12]
	add	w10, w10, w11
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.13:                              //   in Loop: Header=BB322_11 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB322_11
.LBB322_14:                             //   in Loop: Header=BB322_9 Depth=1
// %bb.15:                              //   in Loop: Header=BB322_9 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB322_9
.LBB322_16:
	stur	wzr, [x29, #-4]
.LBB322_17:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end322:
	.size	hynix20nm_getotpparam, .Lfunc_end322-hynix20nm_getotpparam
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_set_default_param     // -- Begin function hynix20nm_set_default_param
	.p2align	2
	.type	hynix20nm_set_default_param,@function
hynix20nm_set_default_param:            // @hynix20nm_set_default_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
.LBB323_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	adrp	x9, hynix20nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB323_4
// %bb.2:                               //   in Loop: Header=BB323_1 Depth=1
	ldur	x8, [x29, #-8]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	w9, [sp, #16]
	mov	w10, w9
	add	x9, sp, #8                      // =8
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB323_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB323_1
.LBB323_4:
	ldur	x0, [x29, #-8]
	adrp	x8, hynix20nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_read_retry_reg_num]
	add	x1, sp, #8                      // =8
	adrp	x2, hynix20nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix20nm_read_retry_reg_adr
	bl	hynix20nm_vender_set_param
	stur	w0, [x29, #-12]
	adrp	x0, .L.str.385
	add	x0, x0, :lo12:.L.str.385
	bl	nand_print_dbg
	str	wzr, [sp, #16]
.LBB323_5:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	adrp	x9, hynix20nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix20nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB323_8
// %bb.6:                               //   in Loop: Header=BB323_5 Depth=1
	ldr	w8, [sp, #16]
	mov	w9, w8
	add	x8, sp, #8                      // =8
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.386
	add	x0, x0, :lo12:.L.str.386
	bl	nand_print_dbg
// %bb.7:                               //   in Loop: Header=BB323_5 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB323_5
.LBB323_8:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end323:
	.size	hynix20nm_set_default_param, .Lfunc_end323-hynix20nm_set_default_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_init_readretry_param  // -- Begin function hynix20nm_init_readretry_param
	.p2align	2
	.type	hynix20nm_init_readretry_param,@function
hynix20nm_init_readretry_param:         // @hynix20nm_init_readretry_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #128                    // =128
	stp	x29, x30, [sp, #112]            // 16-byte Folded Spill
	add	x29, sp, #112                   // =112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-20]
	str	xzr, [sp, #16]
	str	xzr, [sp, #8]
	str	wzr, [sp, #4]
	str	wzr, [sp]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #20]
	lsl	w0, w8, #9
	bl	nand_get_temp_buf
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB324_2
// %bb.1:
	adrp	x0, .L.str.433
	add	x0, x0, :lo12:.L.str.433
	bl	nand_print_dbg
	mov	w8, #-94
	stur	w8, [x29, #-4]
	b	.LBB324_18
.LBB324_2:
	mov	w0, #32768
	bl	nand_get_temp_buf
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	cbnz	x8, .LBB324_4
// %bb.3:
	ldr	x0, [sp, #16]
	bl	nand_free_temp_buf
	adrp	x0, .L.str.434
	add	x0, x0, :lo12:.L.str.434
	bl	nand_print
	mov	w8, #-76
	stur	w8, [x29, #-4]
	b	.LBB324_18
.LBB324_4:
// %bb.5:
	str	wzr, [sp]
	ldr	x0, [sp, #8]
	ldr	x1, [sp, #16]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #24]
	mov	w2, #64
	bl	hynix16nm_get_special_info
	str	w0, [sp]
	ldr	w8, [sp]
	cbnz	w8, .LBB324_13
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #24]
	adrp	x0, .L.str.435
	add	x0, x0, :lo12:.L.str.435
	bl	nand_print_dbg
	stur	wzr, [x29, #-20]
.LBB324_7:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	subs	w8, w8, #64                     // =64
	b.hs	.LBB324_12
// %bb.8:                               //   in Loop: Header=BB324_7 Depth=1
	ldr	x8, [sp, #16]
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.436
	add	x0, x0, :lo12:.L.str.436
	bl	nand_print_dbg
	ldur	w8, [x29, #-20]
	mov	w9, #-1
	add	w9, w9, #8                      // =8
	and	w8, w8, w9
	subs	w8, w8, #7                      // =7
	b.ne	.LBB324_10
// %bb.9:                               //   in Loop: Header=BB324_7 Depth=1
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
.LBB324_10:                             //   in Loop: Header=BB324_7 Depth=1
// %bb.11:                              //   in Loop: Header=BB324_7 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB324_7
.LBB324_12:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #16]
	bl	hynix20nm_saveotpvalue
	ldur	x0, [x29, #-16]
	bl	hynix20nm_set_default_param
	str	w0, [sp, #4]
	b	.LBB324_16
.LBB324_13:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x0, .L.str.437
	add	x0, x0, :lo12:.L.str.437
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	add	x1, sp, #28                     // =28
	bl	hynix20nm_getotpparam
	str	w0, [sp, #4]
	ldr	w8, [sp, #4]
	cbz	w8, .LBB324_15
// %bb.14:
	b	.LBB324_17
.LBB324_15:
	ldur	x0, [x29, #-16]
	bl	hynix20nm_set_default_param
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x0, .L.str.438
	add	x0, x0, :lo12:.L.str.438
	bl	nand_print_dbg
.LBB324_16:
.LBB324_17:
	ldr	x0, [sp, #16]
	bl	nand_free_temp_buf
	ldr	w8, [sp, #4]
	stur	w8, [x29, #-4]
.LBB324_18:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #112]            // 16-byte Folded Reload
	add	sp, sp, #128                    // =128
	ret
.Lfunc_end324:
	.size	hynix20nm_init_readretry_param, .Lfunc_end324-hynix20nm_init_readretry_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_get_special_info      // -- Begin function hynix16nm_get_special_info
	.p2align	2
	.type	hynix16nm_get_special_info,@function
hynix16nm_get_special_info:             // @hynix16nm_get_special_info
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	w2, [x29, #-28]
	stur	w3, [x29, #-32]
	adrp	x0, .L.str.451
	add	x0, x0, :lo12:.L.str.451
	bl	nand_print_dbg
	add	x0, sp, #32                     // =32
	bl	physic_info_get_offset
	str	w0, [sp, #40]
	ldr	w8, [sp, #40]
	cbz	w8, .LBB325_2
// %bb.1:
	adrp	x0, .L.str.452
	add	x0, x0, :lo12:.L.str.452
	bl	nand_print
	mov	w8, #1
	stur	w8, [x29, #-4]
	b	.LBB325_18
.LBB325_2:
	mov	w8, #-1
	str	w8, [sp, #36]
	mov	w8, #4
	str	w8, [sp, #28]
.LBB325_3:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB325_8 Depth 2
	ldr	w8, [sp, #28]
	subs	w8, w8, #50                     // =50
	b.hi	.LBB325_17
// %bb.4:                               //   in Loop: Header=BB325_3 Depth=1
	ldr	w1, [sp, #28]
	adrp	x0, .L.str.453
	add	x0, x0, :lo12:.L.str.453
	bl	nand_print_dbg
	ldur	x8, [x29, #-16]
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	mov	x9, #9728
	ldrb	w8, [x8, x9]
	subs	w8, w8, #165                    // =165
	b.eq	.LBB325_6
// %bb.5:                               //   in Loop: Header=BB325_3 Depth=1
	ldr	w0, [sp, #28]
	ldr	w1, [sp, #32]
	ldur	x3, [x29, #-16]
	add	x2, sp, #24                     // =24
	bl	physic_info_get_one_copy
.LBB325_6:                              //   in Loop: Header=BB325_3 Depth=1
	ldur	x8, [x29, #-16]
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	mov	x9, #9728
	ldrb	w8, [x8, x9]
	subs	w8, w8, #165                    // =165
	b.ne	.LBB325_15
// %bb.7:                               //   in Loop: Header=BB325_3 Depth=1
	strb	wzr, [sp, #15]
	mov	w8, #2
	stur	w8, [x29, #-36]
.LBB325_8:                              //   Parent Loop BB325_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-36]
	subs	w8, w8, #1024                   // =1024
	b.hs	.LBB325_11
// %bb.9:                               //   in Loop: Header=BB325_8 Depth=2
	ldr	x8, [sp, #16]
	mov	x9, #9728
	add	x8, x8, x9
	ldur	w9, [x29, #-36]
                                        // kill: def $x9 killed $w9
	ldrb	w9, [x8, x9]
	ldrb	w8, [sp, #15]
	add	w8, w8, w9
	strb	w8, [sp, #15]
// %bb.10:                              //   in Loop: Header=BB325_8 Depth=2
	ldur	w8, [x29, #-36]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-36]
	b	.LBB325_8
.LBB325_11:                             //   in Loop: Header=BB325_3 Depth=1
	ldrb	w8, [sp, #15]
	ldr	x9, [sp, #16]
	mov	x10, #9729
	ldrb	w9, [x9, x10]
	subs	w8, w8, w9
	b.ne	.LBB325_13
// %bb.12:
	str	wzr, [sp, #36]
	ldur	x0, [x29, #-24]
	ldr	x8, [sp, #16]
	mov	x9, #9728
	add	x8, x8, x9
	ldur	w9, [x29, #-32]
	ldur	w10, [x29, #-28]
	mul	w9, w9, w10
	add	w9, w9, #2                      // =2
                                        // kill: def $x9 killed $w9
	add	x1, x8, x9
	ldur	w2, [x29, #-28]
	bl	nand_memcpy
	b	.LBB325_17
.LBB325_13:                             //   in Loop: Header=BB325_3 Depth=1
	ldr	x8, [sp, #16]
	mov	x9, #9728
	add	x8, x8, x9
	strb	wzr, [x8]
// %bb.14:                              //   in Loop: Header=BB325_3 Depth=1
.LBB325_15:                             //   in Loop: Header=BB325_3 Depth=1
// %bb.16:                              //   in Loop: Header=BB325_3 Depth=1
	ldr	w8, [sp, #28]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #28]
	b	.LBB325_3
.LBB325_17:
	ldr	w8, [sp, #36]
	stur	w8, [x29, #-4]
.LBB325_18:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end325:
	.size	hynix16nm_get_special_info, .Lfunc_end325-hynix16nm_get_special_info
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_readretry_init        // -- Begin function hynix20nm_readretry_init
	.p2align	2
	.type	hynix20nm_readretry_init,@function
hynix20nm_readretry_init:               // @hynix20nm_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, hynix20nm_read_retry_reg_adr
	add	x8, x8, :lo12:hynix20nm_read_retry_reg_adr
	str	x8, [sp]                        // 8-byte Folded Spill
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w9, w8, #0xff
	adrp	x8, hynix20nm_read_retry_mode
	strb	w9, [x8, :lo12:hynix20nm_read_retry_mode]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	lsr	w9, w9, #8
	and	w9, w9, #0xff
	adrp	x10, hynix20nm_read_retry_cycle
	strb	w9, [x10, :lo12:hynix20nm_read_retry_cycle]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, hynix20nm_read_retry_reg_num
	strb	w9, [x10, :lo12:hynix20nm_read_retry_reg_num]
	ldrb	w9, [x8, :lo12:hynix20nm_read_retry_mode]
	mov	w8, #2
	subs	w8, w8, w9
	b.ne	.LBB326_2
// %bb.1:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	mov	w8, #204
	adrp	x10, hynix20nm_read_retry_reg_adr
	strb	w8, [x10, :lo12:hynix20nm_read_retry_reg_adr]
	mov	w8, #191
	strb	w8, [x9, #1]
	mov	w8, #170
	strb	w8, [x9, #2]
	mov	w8, #171
	strb	w8, [x9, #3]
	mov	w8, #205
	strb	w8, [x9, #4]
	mov	w8, #173
	strb	w8, [x9, #5]
	mov	w8, #174
	strb	w8, [x9, #6]
	mov	w8, #175
	strb	w8, [x9, #7]
	b	.LBB326_6
.LBB326_2:
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w9, [x8, :lo12:hynix20nm_read_retry_mode]
	mov	w8, #3
	subs	w8, w8, w9
	b.ne	.LBB326_4
// %bb.3:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	adrp	x10, hynix20nm_read_retry_reg_adr
	mov	w8, #176
	strb	w8, [x10, :lo12:hynix20nm_read_retry_reg_adr]
	mov	w8, #177
	strb	w8, [x9, #1]
	mov	w8, #178
	strb	w8, [x9, #2]
	mov	w8, #179
	strb	w8, [x9, #3]
	mov	w8, #180
	strb	w8, [x9, #4]
	mov	w8, #181
	strb	w8, [x9, #5]
	mov	w8, #182
	strb	w8, [x9, #6]
	mov	w8, #183
	strb	w8, [x9, #7]
	b	.LBB326_5
.LBB326_4:
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix20nm_read_retry_mode]
	adrp	x0, .L.str.388
	add	x0, x0, :lo12:.L.str.388
	bl	nand_print
	mov	w8, #-93
	stur	w8, [x29, #-4]
	b	.LBB326_11
.LBB326_5:
.LBB326_6:
	ldr	x8, [sp, #16]
	str	wzr, [x8, #184]
	str	wzr, [sp, #12]
.LBB326_7:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w8, w8, #128                    // =128
	b.ge	.LBB326_10
// %bb.8:                               //   in Loop: Header=BB326_7 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #56                     // =56
	ldrsw	x9, [sp, #12]
	add	x8, x8, x9
	strb	wzr, [x8]
// %bb.9:                               //   in Loop: Header=BB326_7 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB326_7
.LBB326_10:
	ldr	x0, [sp, #16]
	bl	hynix20nm_init_readretry_param
	str	w0, [sp, #8]
	ldr	w8, [sp, #8]
	stur	w8, [x29, #-4]
.LBB326_11:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end326:
	.size	hynix20nm_readretry_init, .Lfunc_end326-hynix20nm_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_readretry_exit        // -- Begin function hynix20nm_readretry_exit
	.p2align	2
	.type	hynix20nm_readretry_exit,@function
hynix20nm_readretry_exit:               // @hynix20nm_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	adrp	x0, .L.str.439
	add	x0, x0, :lo12:.L.str.439
	bl	nand_print_dbg
	ldr	x0, [sp, #8]
	bl	hynix20nm_set_default_param
	ldr	x8, [sp, #8]
	mov	w0, wzr
	str	wzr, [x8, #184]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end327:
	.size	hynix20nm_readretry_exit, .Lfunc_end327-hynix20nm_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_special_init          // -- Begin function hynix20nm_special_init
	.p2align	2
	.type	hynix20nm_special_init,@function
hynix20nm_special_init:                 // @hynix20nm_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB328_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB328_3
// %bb.2:                               //   in Loop: Header=BB328_1 Depth=1
	ldr	x0, [sp]
	bl	hynix20nm_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB328_1
.LBB328_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB328_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, hynix20nm_read_page_end
	add	x8, x8, :lo12:hynix20nm_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix20nm_read_retry_mode]
	adrp	x8, hynix20nm_read_retry_cycle
	ldrb	w2, [x8, :lo12:hynix20nm_read_retry_cycle]
	adrp	x8, hynix20nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_read_retry_reg_num]
	adrp	x0, .L.str.440
	add	x0, x0, :lo12:.L.str.440
	bl	nand_print_dbg
	b	.LBB328_6
.LBB328_5:
	adrp	x8, hynix20nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix20nm_read_retry_mode]
	adrp	x8, hynix20nm_read_retry_cycle
	ldrb	w2, [x8, :lo12:hynix20nm_read_retry_cycle]
	adrp	x8, hynix20nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix20nm_read_retry_reg_num]
	adrp	x0, .L.str.441
	add	x0, x0, :lo12:.L.str.441
	bl	nand_print
.LBB328_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end328:
	.size	hynix20nm_special_init, .Lfunc_end328-hynix20nm_special_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix20nm_special_exit          // -- Begin function hynix20nm_special_exit
	.p2align	2
	.type	hynix20nm_special_exit,@function
hynix20nm_special_exit:                 // @hynix20nm_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB329_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB329_3
// %bb.2:                               //   in Loop: Header=BB329_1 Depth=1
	ldr	x0, [sp, #8]
	bl	hynix20nm_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB329_1
.LBB329_3:
	adrp	x0, .L.str.442
	add	x0, x0, :lo12:.L.str.442
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end329:
	.size	hynix20nm_special_exit, .Lfunc_end329-hynix20nm_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	micron_intel_set_feature        // -- Begin function micron_intel_set_feature
	.p2align	2
	.type	micron_intel_set_feature,@function
micron_intel_set_feature:               // @micron_intel_set_feature
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	w8, w1
	stur	x0, [x29, #-16]
	sub	x1, x29, #17                    // =17
	str	x1, [sp, #8]                    // 8-byte Folded Spill
	sturb	w8, [x29, #-17]
	sturb	w2, [x29, #-18]
	ldurb	w8, [x29, #-17]
	sturb	w8, [x29, #-19]
	ldurb	w8, [x29, #-18]
	sub	x2, x29, #23                    // =23
	sturb	w8, [x29, #-23]
	sturb	wzr, [x29, #-22]
	sturb	wzr, [x29, #-21]
	sturb	wzr, [x29, #-20]
	ldur	x0, [x29, #-16]
	bl	nand_set_feature
	ldr	x1, [sp, #8]                    // 8-byte Folded Reload
	ldur	x0, [x29, #-16]
	add	x2, sp, #21                     // =21
	bl	nand_get_feature
	ldrb	w8, [sp, #21]
	ldurb	w9, [x29, #-23]
	subs	w8, w8, w9
	b.eq	.LBB330_2
// %bb.1:
	ldurb	w1, [x29, #-19]
	ldurb	w2, [x29, #-18]
	adrp	x0, .L.str.443
	add	x0, x0, :lo12:.L.str.443
	bl	nand_print
	mov	w8, #-72
	stur	w8, [x29, #-4]
	b	.LBB330_3
.LBB330_2:
	stur	wzr, [x29, #-4]
.LBB330_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end330:
	.size	micron_intel_set_feature, .Lfunc_end330-micron_intel_set_feature
	.cfi_endproc
                                        // -- End function
	.globl	micron_set_readretry            // -- Begin function micron_set_readretry
	.p2align	2
	.type	micron_set_readretry,@function
micron_set_readretry:                   // @micron_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #64                     // =64
	b.eq	.LBB331_5
// %bb.1:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #65                     // =65
	b.eq	.LBB331_5
// %bb.2:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #80                     // =80
	b.eq	.LBB331_5
// %bb.3:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #66                     // =66
	b.eq	.LBB331_5
// %bb.4:
	b	.LBB331_16
.LBB331_5:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	sturb	w8, [x29, #-13]
	ldurb	w8, [x29, #-13]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB331_7
// %bb.6:
	sturb	wzr, [x29, #-13]
.LBB331_7:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #80                     // =80
	b.ne	.LBB331_15
// %bb.8:
	adrp	x8, micron_p1
	ldr	x8, [x8, :lo12:micron_p1]
	ldurb	w9, [x29, #-13]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	cbnz	w8, .LBB331_12
// %bb.9:
	ldur	x0, [x29, #-8]
	mov	w1, #-109
	mov	w2, #1
	bl	micron_intel_set_feature
	stur	w0, [x29, #-12]
	ldur	w8, [x29, #-12]
	cbz	w8, .LBB331_11
// %bb.10:
	b	.LBB331_16
.LBB331_11:
.LBB331_12:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB331_14
// %bb.13:
	ldur	x0, [x29, #-8]
	mov	w1, #-119
	mov	w2, wzr
	str	w2, [sp, #12]                   // 4-byte Folded Spill
	bl	micron_intel_set_feature
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	stur	w0, [x29, #-12]
	ldur	x0, [x29, #-8]
	mov	w1, #-109
	bl	micron_intel_set_feature
	ldur	w8, [x29, #-12]
	orr	w8, w8, w0
	stur	w8, [x29, #-12]
	b	.LBB331_16
.LBB331_14:
.LBB331_15:
	ldur	x0, [x29, #-8]
	adrp	x8, micron_p1
	ldr	x8, [x8, :lo12:micron_p1]
	ldurb	w9, [x29, #-13]
                                        // kill: def $x9 killed $w9
	ldrb	w2, [x8, x9]
	mov	w1, #-119
	bl	micron_intel_set_feature
	stur	w0, [x29, #-12]
.LBB331_16:
	ldur	x8, [x29, #-8]
	ldr	w8, [x8, #184]
	subs	w8, w8, #255                    // =255
	b.ne	.LBB331_18
// %bb.17:
	ldur	x8, [x29, #-8]
	str	wzr, [x8, #184]
.LBB331_18:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end331:
	.size	micron_set_readretry, .Lfunc_end331-micron_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	micron_read_page_end            // -- Begin function micron_read_page_end
	.p2align	2
	.type	micron_read_page_end,@function
micron_read_page_end:                   // @micron_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB332_11
// %bb.1:
	adrp	x0, .L.str.444
	add	x0, x0, :lo12:.L.str.444
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB332_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, micron_read_retry_cycle
	ldrb	w9, [x9, :lo12:micron_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB332_10
// %bb.3:                               //   in Loop: Header=BB332_2 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	bl	micron_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB332_5
// %bb.4:                               //   in Loop: Header=BB332_2 Depth=1
	b	.LBB332_9
.LBB332_5:                              //   in Loop: Header=BB332_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB332_7
// %bb.6:                               //   in Loop: Header=BB332_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB332_8
.LBB332_7:
	mov	w8, #10
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.445
	add	x0, x0, :lo12:.L.str.445
	bl	nand_print_dbg
	b	.LBB332_10
.LBB332_8:                              //   in Loop: Header=BB332_2 Depth=1
.LBB332_9:                              //   in Loop: Header=BB332_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB332_2
.LBB332_10:
	ldr	x9, [sp, #8]
	mov	w8, #255
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	bl	micron_set_readretry
.LBB332_11:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end332:
	.size	micron_read_page_end, .Lfunc_end332-micron_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	micron_readretry_init           // -- Begin function micron_readretry_init
	.p2align	2
	.type	micron_readretry_init,@function
micron_readretry_init:                  // @micron_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	mov	w2, wzr
	str	wzr, [x8, #184]
	adrp	x9, micron_read_retry_mode
	str	x9, [sp, #8]                    // 8-byte Folded Spill
	strb	wzr, [x9, :lo12:micron_read_retry_mode]
	adrp	x10, micron_p1
	adrp	x8, micron_p1_2
	add	x8, x8, :lo12:micron_p1_2
	str	x8, [x10, :lo12:micron_p1]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w8, w8, #0xff
	strb	w8, [x9, :lo12:micron_read_retry_mode]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #8
	and	w8, w8, #0xff
	adrp	x9, micron_read_retry_cycle
	strb	w8, [x9, :lo12:micron_read_retry_cycle]
	ldr	x0, [sp, #16]
	mov	w1, #-119
	bl	micron_intel_set_feature
	ldr	x8, [sp, #8]                    // 8-byte Folded Reload
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #65                     // =65
	b.ne	.LBB333_2
// %bb.1:
	mov	w8, #12
	adrp	x9, micron_read_retry_cycle
	strb	w8, [x9, :lo12:micron_read_retry_cycle]
	adrp	x9, micron_p1
	adrp	x8, micron_p1_2
	add	x8, x8, :lo12:micron_p1_2
	str	x8, [x9, :lo12:micron_p1]
	b	.LBB333_12
.LBB333_2:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #64                     // =64
	b.ne	.LBB333_4
// %bb.3:
	adrp	x8, micron_p1_1
	add	x8, x8, :lo12:micron_p1_1
	adrp	x9, micron_p1
	str	x8, [x9, :lo12:micron_p1]
	mov	w8, #10
	adrp	x9, micron_read_retry_cycle
	strb	w8, [x9, :lo12:micron_read_retry_cycle]
	b	.LBB333_11
.LBB333_4:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #80                     // =80
	b.ne	.LBB333_6
// %bb.5:
	adrp	x8, micron_p1_3
	add	x8, x8, :lo12:micron_p1_3
	adrp	x9, micron_p1
	str	x8, [x9, :lo12:micron_p1]
	mov	w8, #7
	adrp	x9, micron_read_retry_cycle
	strb	w8, [x9, :lo12:micron_read_retry_cycle]
	ldr	x0, [sp, #16]
	mov	w1, #-109
	mov	w2, wzr
	bl	micron_intel_set_feature
	b	.LBB333_10
.LBB333_6:
	adrp	x8, micron_read_retry_mode
	ldrb	w8, [x8, :lo12:micron_read_retry_mode]
	subs	w8, w8, #66                     // =66
	b.ne	.LBB333_8
// %bb.7:
	adrp	x8, micron_p1_4
	add	x8, x8, :lo12:micron_p1_4
	adrp	x9, micron_p1
	str	x8, [x9, :lo12:micron_p1]
	mov	w8, #18
	adrp	x9, micron_read_retry_cycle
	strb	w8, [x9, :lo12:micron_read_retry_cycle]
	b	.LBB333_9
.LBB333_8:
	adrp	x0, .L.str.446
	add	x0, x0, :lo12:.L.str.446
	bl	nand_print
	mov	w8, #-71
	stur	w8, [x29, #-4]
	b	.LBB333_13
.LBB333_9:
.LBB333_10:
.LBB333_11:
.LBB333_12:
	adrp	x8, micron_read_retry_cycle
	ldrb	w1, [x8, :lo12:micron_read_retry_cycle]
	adrp	x0, .L.str.447
	add	x0, x0, :lo12:.L.str.447
	bl	nand_print_dbg
	stur	wzr, [x29, #-4]
.LBB333_13:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end333:
	.size	micron_readretry_init, .Lfunc_end333-micron_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	micron_readretry_exit           // -- Begin function micron_readretry_exit
	.p2align	2
	.type	micron_readretry_exit,@function
micron_readretry_exit:                  // @micron_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	adrp	x8, micron_read_retry_mode
	strb	wzr, [x8, :lo12:micron_read_retry_mode]
	adrp	x8, micron_read_retry_cycle
	strb	wzr, [x8, :lo12:micron_read_retry_cycle]
	mov	w0, wzr
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end334:
	.size	micron_readretry_exit, .Lfunc_end334-micron_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	micron_special_init             // -- Begin function micron_special_init
	.p2align	2
	.type	micron_special_init,@function
micron_special_init:                    // @micron_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB335_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB335_3
// %bb.2:                               //   in Loop: Header=BB335_1 Depth=1
	ldr	x0, [sp]
	bl	micron_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB335_1
.LBB335_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB335_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, micron_read_page_end
	add	x8, x8, :lo12:micron_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, micron_read_retry_mode
	ldrb	w1, [x8, :lo12:micron_read_retry_mode]
	adrp	x8, micron_read_retry_cycle
	ldrb	w2, [x8, :lo12:micron_read_retry_cycle]
	adrp	x0, .L.str.448
	add	x0, x0, :lo12:.L.str.448
	bl	nand_print_dbg
	b	.LBB335_6
.LBB335_5:
	adrp	x8, micron_read_retry_mode
	ldrb	w1, [x8, :lo12:micron_read_retry_mode]
	adrp	x8, micron_read_retry_cycle
	ldrb	w2, [x8, :lo12:micron_read_retry_cycle]
	adrp	x0, .L.str.449
	add	x0, x0, :lo12:.L.str.449
	bl	nand_print
.LBB335_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end335:
	.size	micron_special_init, .Lfunc_end335-micron_special_init
	.cfi_endproc
                                        // -- End function
	.globl	micron_special_exit             // -- Begin function micron_special_exit
	.p2align	2
	.type	micron_special_exit,@function
micron_special_exit:                    // @micron_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB336_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB336_3
// %bb.2:                               //   in Loop: Header=BB336_1 Depth=1
	ldr	x0, [sp, #8]
	bl	micron_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB336_1
.LBB336_3:
	adrp	x0, .L.str.450
	add	x0, x0, :lo12:.L.str.450
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end336:
	.size	micron_special_exit, .Lfunc_end336-micron_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_vender_set_param      // -- Begin function hynix16nm_vender_set_param
	.p2align	2
	.type	hynix16nm_vender_set_param,@function
hynix16nm_vender_set_param:             // @hynix16nm_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #8]
	mov	w8, #54
	strb	w8, [sp, #15]
	mov	w8, #22
	strb	w8, [sp, #14]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	str	wzr, [sp, #16]
.LBB337_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hs	.LBB337_4
// %bb.2:                               //   in Loop: Header=BB337_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x3, x8, x9
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x4, x8, x9
	add	x1, sp, #15                     // =15
	mov	w2, wzr
	str	w2, [sp, #4]                    // 4-byte Folded Spill
	mov	w6, #1
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w2, [sp, #4]                    // 4-byte Folded Reload
	ldur	x0, [x29, #-8]
	ldrb	w1, [sp, #14]
	bl	set_one_cmd
// %bb.3:                               //   in Loop: Header=BB337_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB337_1
.LBB337_4:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-16]
	ldrb	w2, [x8, #1]
	ldur	x8, [x29, #-16]
	ldrb	w3, [x8, #2]
	ldur	x8, [x29, #-16]
	ldrb	w4, [x8, #3]
	adrp	x0, .L.str.379
	add	x0, x0, :lo12:.L.str.379
	bl	nand_print_dbg
	ldr	w0, [sp, #8]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end337:
	.size	hynix16nm_vender_set_param, .Lfunc_end337-hynix16nm_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_set_readretry         // -- Begin function hynix16nm_set_readretry
	.p2align	2
	.type	hynix16nm_set_readretry,@function
hynix16nm_set_readretry:                // @hynix16nm_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	str	wzr, [sp, #8]
	ldr	x8, [sp, #16]
	ldr	w8, [x8, #184]
	adrp	x9, hynix16nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix16nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ls	.LBB338_2
// %bb.1:
	mov	w8, #-74
	stur	w8, [x29, #-4]
	b	.LBB338_7
.LBB338_2:
	str	wzr, [sp, #12]
.LBB338_3:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	adrp	x9, hynix16nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix16nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB338_6
// %bb.4:                               //   in Loop: Header=BB338_3 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #56                     // =56
	ldr	x9, [sp, #16]
	ldr	w10, [x9, #184]
	mov	w9, #4
	mul	w9, w9, w10
	ldr	w10, [sp, #12]
	add	w9, w9, w10
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	w9, [sp, #12]
	mov	w10, w9
	mov	x9, sp
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.5:                               //   in Loop: Header=BB338_3 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB338_3
.LBB338_6:
	ldr	x0, [sp, #16]
	adrp	x8, hynix16nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix16nm_read_retry_reg_num]
	mov	x1, sp
	adrp	x2, hynix16nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix16nm_read_retry_reg_adr
	bl	hynix16nm_vender_set_param
	str	w0, [sp, #8]
	ldr	w8, [sp, #8]
	stur	w8, [x29, #-4]
.LBB338_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end338:
	.size	hynix16nm_set_readretry, .Lfunc_end338-hynix16nm_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_read_page_end         // -- Begin function hynix16nm_read_page_end
	.p2align	2
	.type	hynix16nm_read_page_end,@function
hynix16nm_read_page_end:                // @hynix16nm_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB339_13
// %bb.1:
	adrp	x0, .L.str.454
	add	x0, x0, :lo12:.L.str.454
	bl	nand_print_dbg
	stur	wzr, [x29, #-12]
.LBB339_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, hynix16nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix16nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB339_12
// %bb.3:                               //   in Loop: Header=BB339_2 Depth=1
	str	wzr, [sp, #16]
	ldr	x9, [sp, #8]
	ldr	w8, [x9, #184]
	add	w8, w8, #1                      // =1
	str	w8, [x9, #184]
	ldr	x8, [sp, #8]
	ldr	w8, [x8, #184]
	adrp	x9, hynix16nm_read_retry_cycle
	ldrb	w9, [x9, :lo12:hynix16nm_read_retry_cycle]
	subs	w8, w8, w9
	b.ls	.LBB339_5
// %bb.4:                               //   in Loop: Header=BB339_2 Depth=1
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
.LBB339_5:                              //   in Loop: Header=BB339_2 Depth=1
	ldr	x0, [sp, #8]
	bl	hynix16nm_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB339_7
// %bb.6:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	hynix16nm_set_readretry
	b	.LBB339_12
.LBB339_7:                              //   in Loop: Header=BB339_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB339_9
// %bb.8:                               //   in Loop: Header=BB339_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB339_10
.LBB339_9:
	mov	w8, #10
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.455
	add	x0, x0, :lo12:.L.str.455
	bl	nand_print_dbg
	b	.LBB339_12
.LBB339_10:                             //   in Loop: Header=BB339_2 Depth=1
// %bb.11:                              //   in Loop: Header=BB339_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB339_2
.LBB339_12:
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	hynix16nm_set_readretry
.LBB339_13:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end339:
	.size	hynix16nm_read_page_end, .Lfunc_end339-hynix16nm_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_write_page_FF         // -- Begin function hynix16nm_write_page_FF
	.p2align	2
	.type	hynix16nm_write_page_FF,@function
hynix16nm_write_page_FF:                // @hynix16nm_write_page_FF
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	stur	wzr, [x29, #-24]
	stur	wzr, [x29, #-28]
	stur	wzr, [x29, #-32]
	stur	wzr, [x29, #-36]
	str	wzr, [sp, #40]
	str	wzr, [sp, #36]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-16]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #24]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #280]
	str	x8, [sp, #16]
	ldr	x8, [sp, #24]
	ldr	x8, [x8, #280]
	add	x8, x8, #40                     // =40
	str	x8, [sp, #8]
	ldr	x8, [sp, #24]
	ldr	w8, [x8, #28]
	ldr	x9, [sp, #16]
	ldr	w9, [x9, #16]
	subs	w8, w8, w9
	b.hs	.LBB340_2
// %bb.1:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #4]
	ldr	x9, [sp, #24]
	ldr	w9, [x9, #32]
	subs	w8, w8, w9
	b.lo	.LBB340_3
.LBB340_2:
	ldr	x8, [sp, #16]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #24]
	ldr	w2, [x8, #28]
	ldr	x8, [sp, #16]
	ldr	w3, [x8, #16]
	ldur	x8, [x29, #-16]
	ldr	w4, [x8, #4]
	ldr	x8, [sp, #24]
	ldr	w5, [x8, #32]
	adrp	x0, .L.str.422
	add	x0, x0, :lo12:.L.str.422
	bl	nand_print
	mov	w8, #-108
	stur	w8, [x29, #-4]
	b	.LBB340_21
.LBB340_3:
	ldr	x0, [sp, #24]
	bl	nand_read_chip_status_ready
	ldr	x0, [sp, #24]
	bl	nand_enable_chip
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_ecc
	ldr	x0, [sp, #8]
	bl	ndfc_clean_cmd_seq
	ldur	w8, [x29, #-20]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB340_5
// %bb.4:
	mov	w8, #9
	stur	w8, [x29, #-28]
	mov	w8, #832
	stur	w8, [x29, #-32]
	b	.LBB340_8
.LBB340_5:
	ldur	w8, [x29, #-20]
	subs	w8, w8, #16                     // =16
	b.ne	.LBB340_7
// %bb.6:
	mov	w8, #18
	stur	w8, [x29, #-28]
	mov	w8, #640
	stur	w8, [x29, #-32]
.LBB340_7:
.LBB340_8:
	stur	wzr, [x29, #-24]
.LBB340_9:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-24]
	ldur	w9, [x29, #-28]
	subs	w8, w8, w9
	b.hs	.LBB340_18
// %bb.10:                              //   in Loop: Header=BB340_9 Depth=1
	ldur	w8, [x29, #-24]
	cbnz	w8, .LBB340_12
// %bb.11:                              //   in Loop: Header=BB340_9 Depth=1
	ldr	x8, [sp, #8]
	str	wzr, [x8]
	ldr	x9, [sp, #8]
	mov	w8, #128
	str	w8, [x9, #12]
	ldr	x9, [sp, #8]
	mov	w8, #1
	str	w8, [x9, #8]
	ldr	x9, [sp, #8]
	str	w8, [x9, #16]
	ldr	x9, [sp, #8]
	str	w8, [x9, #56]
	ldr	x9, [sp, #8]
	str	w8, [x9, #48]
	ldr	x9, [sp, #8]
	str	w8, [x9, #52]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #16]
	ldr	x10, [sp, #8]
	str	x9, [x10, #72]
	ldr	x10, [sp, #8]
	mov	w9, #1024
	str	w9, [x10, #60]
	ldr	x9, [sp, #8]
	str	w8, [x9, #44]
	ldr	x8, [sp, #24]
	ldr	w0, [x8, #44]
	ldur	x8, [x29, #-16]
	ldr	w1, [x8, #4]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #8]
	bl	get_row_addr
	str	w0, [sp, #40]
	ldr	x9, [sp, #8]
	mov	w8, #5
	str	w8, [x9, #40]
	ldr	w0, [sp, #36]
	ldr	w2, [sp, #40]
	ldr	x8, [sp, #8]
	add	x4, x8, #24                     // =24
	mov	w1, #2
	mov	w3, #3
	bl	fill_cmd_addr
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	ldr	x1, [sp, #8]
	bl	ndfc_execute_cmd
	stur	w0, [x29, #-36]
	b	.LBB340_16
.LBB340_12:                             //   in Loop: Header=BB340_9 Depth=1
	ldur	w8, [x29, #-24]
	ldur	w9, [x29, #-28]
	subs	w9, w9, #1                      // =1
	subs	w8, w8, w9
	b.ne	.LBB340_14
// %bb.13:                              //   in Loop: Header=BB340_9 Depth=1
	ldr	x8, [sp, #8]
	mov	w3, wzr
	str	wzr, [x8]
	ldr	x9, [sp, #8]
	mov	w8, #133
	str	w8, [x9, #12]
	ldr	x9, [sp, #8]
	mov	w8, #1
	str	w8, [x9, #8]
	ldr	x9, [sp, #8]
	str	w8, [x9, #16]
	ldr	x9, [sp, #8]
	str	w8, [x9, #56]
	ldr	x9, [sp, #8]
	str	w8, [x9, #48]
	ldr	x9, [sp, #8]
	str	w8, [x9, #52]
	ldur	x9, [x29, #-16]
	ldr	x9, [x9, #16]
	ldur	w11, [x29, #-24]
	mov	w10, #1024
	mul	w10, w10, w11
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	ldr	x10, [sp, #8]
	str	x9, [x10, #72]
	ldur	w9, [x29, #-32]
	ldr	x10, [sp, #8]
	str	w9, [x10, #60]
	ldr	x9, [sp, #8]
	str	w8, [x9, #44]
	ldr	x8, [sp, #8]
	mov	w1, #2
	str	w1, [x8, #40]
	ldr	w0, [sp, #36]
	ldr	w2, [sp, #40]
	ldr	x8, [sp, #8]
	add	x4, x8, #24                     // =24
	bl	fill_cmd_addr
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	ldr	x1, [sp, #8]
	bl	ndfc_execute_cmd
	stur	w0, [x29, #-36]
	b	.LBB340_15
.LBB340_14:                             //   in Loop: Header=BB340_9 Depth=1
	ldr	x8, [sp, #8]
	mov	w3, wzr
	str	wzr, [x8]
	ldr	x9, [sp, #8]
	mov	w8, #133
	str	w8, [x9, #12]
	ldr	x9, [sp, #8]
	mov	w8, #1
	str	w8, [x9, #8]
	ldr	x9, [sp, #8]
	str	w8, [x9, #16]
	ldr	x9, [sp, #8]
	str	w8, [x9, #56]
	ldr	x9, [sp, #8]
	str	w8, [x9, #48]
	ldr	x9, [sp, #8]
	str	w8, [x9, #52]
	ldur	x9, [x29, #-16]
	ldr	x10, [x9, #16]
	ldur	w11, [x29, #-24]
	mov	w9, #1024
	mul	w11, w9, w11
                                        // kill: def $x11 killed $w11
	add	x10, x10, x11
	ldr	x11, [sp, #8]
	str	x10, [x11, #72]
	ldr	x10, [sp, #8]
	str	w9, [x10, #60]
	ldr	x9, [sp, #8]
	str	w8, [x9, #44]
	ldr	x8, [sp, #8]
	mov	w1, #2
	str	w1, [x8, #40]
	ldr	w0, [sp, #36]
	ldr	w2, [sp, #40]
	ldr	x8, [sp, #8]
	add	x4, x8, #24                     // =24
	bl	fill_cmd_addr
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	ldr	x1, [sp, #8]
	bl	ndfc_execute_cmd
	stur	w0, [x29, #-36]
.LBB340_15:                             //   in Loop: Header=BB340_9 Depth=1
.LBB340_16:                             //   in Loop: Header=BB340_9 Depth=1
	ldr	w8, [sp, #36]
	add	w8, w8, #1024                   // =1024
	str	w8, [sp, #36]
// %bb.17:                              //   in Loop: Header=BB340_9 Depth=1
	ldur	w8, [x29, #-24]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-24]
	b	.LBB340_9
.LBB340_18:
	ldr	x8, [sp, #8]
	str	wzr, [x8]
	ldr	x9, [sp, #8]
	mov	w8, #16
	str	w8, [x9, #12]
	ldr	x9, [sp, #8]
	mov	w8, #1
	str	w8, [x9, #8]
	ldr	x9, [sp, #8]
	str	w8, [x9, #16]
	ldr	x9, [sp, #8]
	str	w8, [x9, #92]
	ldr	x8, [sp, #24]
	ldr	x0, [x8, #280]
	ldr	x1, [sp, #8]
	bl	ndfc_execute_cmd
	stur	w0, [x29, #-36]
	ldr	x0, [sp, #24]
	bl	nand_read_chip_status_ready
	stur	w0, [x29, #-36]
	ldur	w8, [x29, #-36]
	cbz	w8, .LBB340_20
// %bb.19:
	adrp	x0, .L.str.456
	add	x0, x0, :lo12:.L.str.456
	bl	nand_print
.LBB340_20:
	ldr	x0, [sp, #24]
	bl	nand_disable_chip
	ldur	w8, [x29, #-36]
	stur	w8, [x29, #-4]
.LBB340_21:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end340:
	.size	hynix16nm_write_page_FF, .Lfunc_end340-hynix16nm_write_page_FF
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_vender_get_param      // -- Begin function hynix16nm_vender_get_param
	.p2align	2
	.type	hynix16nm_vender_get_param,@function
hynix16nm_vender_get_param:             // @hynix16nm_vender_get_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	x2, [sp, #24]
	str	w3, [sp, #20]
	str	wzr, [sp, #12]
	mov	w8, #55
	strb	w8, [sp, #11]
	ldur	x0, [x29, #-8]
	bl	nand_enable_chip
	ldur	x8, [x29, #-8]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	str	wzr, [sp, #16]
.LBB341_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldr	w9, [sp, #20]
	subs	w8, w8, w9
	b.hs	.LBB341_4
// %bb.2:                               //   in Loop: Header=BB341_1 Depth=1
	ldur	x0, [x29, #-8]
	ldr	x8, [sp, #24]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x2, x8, x9
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x3, x8, x9
	add	x1, sp, #11                     // =11
	mov	w4, #1
	bl	get_data_with_nand_bus_one_cmd
// %bb.3:                               //   in Loop: Header=BB341_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB341_1
.LBB341_4:
	ldur	x0, [x29, #-8]
	bl	nand_disable_chip
	ldr	w0, [sp, #12]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end341:
	.size	hynix16nm_vender_get_param, .Lfunc_end341-hynix16nm_vender_get_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_major_check_byte      // -- Begin function hynix16nm_major_check_byte
	.p2align	2
	.type	hynix16nm_major_check_byte,@function
hynix16nm_major_check_byte:             // @hynix16nm_major_check_byte
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	w1, [x29, #-20]
	stur	w2, [x29, #-24]
	stur	x3, [x29, #-32]
	str	x4, [sp, #40]
	str	w5, [sp, #36]
	strb	wzr, [sp, #11]
	ldur	w8, [x29, #-24]
	ldr	w9, [sp, #36]
	mov	w10, #2
	udiv	w9, w9, w10
	subs	w8, w8, w9
	b.hs	.LBB342_2
// %bb.1:
	ldur	w1, [x29, #-24]
	ldr	w2, [sp, #36]
	adrp	x0, .L.str.457
	add	x0, x0, :lo12:.L.str.457
	bl	nand_print
	ldur	x9, [x29, #-16]
	mov	w8, #255
	strb	w8, [x9]
	mov	w8, #-107
	stur	w8, [x29, #-4]
	b	.LBB342_37
.LBB342_2:
	str	wzr, [sp, #12]
	str	wzr, [sp, #32]
.LBB342_3:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB342_5 Depth 2
                                        //     Child Loop BB342_18 Depth 2
	ldr	w8, [sp, #32]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB342_34
// %bb.4:                               //   in Loop: Header=BB342_3 Depth=1
	str	wzr, [sp, #24]
	str	wzr, [sp, #20]
	str	wzr, [sp, #16]
	str	wzr, [sp, #28]
.LBB342_5:                              //   Parent Loop BB342_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #28]
	ldr	w9, [sp, #36]
	subs	w8, w8, w9
	b.hs	.LBB342_11
// %bb.6:                               //   in Loop: Header=BB342_5 Depth=2
	ldur	x8, [x29, #-32]
	ldr	w9, [sp, #28]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	ldr	w10, [sp, #32]
	mov	w9, #1
	lsl	w9, w9, w10
	and	w8, w8, w9
	cbz	w8, .LBB342_8
// %bb.7:                               //   in Loop: Header=BB342_5 Depth=2
	ldr	w8, [sp, #24]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #24]
	b	.LBB342_9
.LBB342_8:                              //   in Loop: Header=BB342_5 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #20]
.LBB342_9:                              //   in Loop: Header=BB342_5 Depth=2
// %bb.10:                              //   in Loop: Header=BB342_5 Depth=2
	ldr	w8, [sp, #28]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #28]
	b	.LBB342_5
.LBB342_11:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #24]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	b.ls	.LBB342_13
// %bb.12:                              //   in Loop: Header=BB342_3 Depth=1
	ldr	w9, [sp, #32]
	mov	w8, #1
	lsl	w10, w8, w9
	ldrb	w9, [sp, #11]
	orr	w9, w9, w10
	strb	w9, [sp, #11]
	str	w8, [sp, #16]
.LBB342_13:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #20]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	b.ls	.LBB342_15
// %bb.14:                              //   in Loop: Header=BB342_3 Depth=1
	mov	w8, #1
	str	w8, [sp, #16]
.LBB342_15:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB342_29
// %bb.16:                              //   in Loop: Header=BB342_3 Depth=1
	ldur	w8, [x29, #-20]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB342_29
// %bb.17:                              //   in Loop: Header=BB342_3 Depth=1
	str	wzr, [sp, #24]
	str	wzr, [sp, #20]
	str	wzr, [sp, #16]
	str	wzr, [sp, #28]
.LBB342_18:                             //   Parent Loop BB342_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #28]
	ldr	w9, [sp, #36]
	subs	w8, w8, w9
	b.hs	.LBB342_24
// %bb.19:                              //   in Loop: Header=BB342_18 Depth=2
	ldr	x8, [sp, #40]
	ldr	w9, [sp, #28]
                                        // kill: def $x9 killed $w9
	ldrb	w8, [x8, x9]
	ldr	w10, [sp, #32]
	mov	w9, #1
	lsl	w9, w9, w10
	and	w8, w8, w9
	cbz	w8, .LBB342_21
// %bb.20:                              //   in Loop: Header=BB342_18 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #20]
	b	.LBB342_22
.LBB342_21:                             //   in Loop: Header=BB342_18 Depth=2
	ldr	w8, [sp, #24]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #24]
.LBB342_22:                             //   in Loop: Header=BB342_18 Depth=2
// %bb.23:                              //   in Loop: Header=BB342_18 Depth=2
	ldr	w8, [sp, #28]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #28]
	b	.LBB342_18
.LBB342_24:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #20]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	b.ls	.LBB342_26
// %bb.25:                              //   in Loop: Header=BB342_3 Depth=1
	mov	w8, #1
	str	w8, [sp, #16]
.LBB342_26:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #24]
	ldur	w9, [x29, #-24]
	subs	w8, w8, w9
	b.ls	.LBB342_28
// %bb.27:                              //   in Loop: Header=BB342_3 Depth=1
	ldr	w9, [sp, #32]
	mov	w8, #1
	lsl	w10, w8, w9
	ldrb	w9, [sp, #11]
	orr	w9, w9, w10
	strb	w9, [sp, #11]
	str	w8, [sp, #16]
.LBB342_28:                             //   in Loop: Header=BB342_3 Depth=1
.LBB342_29:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #16]
	cbz	w8, .LBB342_31
// %bb.30:                              //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB342_32
.LBB342_31:                             //   in Loop: Header=BB342_3 Depth=1
	ldr	w1, [sp, #32]
	ldr	w2, [sp, #24]
	ldr	w3, [sp, #20]
	adrp	x0, .L.str.458
	add	x0, x0, :lo12:.L.str.458
	bl	nand_print
	ldr	w1, [sp, #32]
	adrp	x0, .L.str.459
	add	x0, x0, :lo12:.L.str.459
	bl	nand_print
.LBB342_32:                             //   in Loop: Header=BB342_3 Depth=1
// %bb.33:                              //   in Loop: Header=BB342_3 Depth=1
	ldr	w8, [sp, #32]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #32]
	b	.LBB342_3
.LBB342_34:
	ldr	w8, [sp, #12]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB342_36
// %bb.35:
	ldrb	w8, [sp, #11]
	ldur	x9, [x29, #-16]
	strb	w8, [x9]
	stur	wzr, [x29, #-4]
	b	.LBB342_37
.LBB342_36:
	ldur	x9, [x29, #-16]
	mov	w8, #255
	strb	w8, [x9]
	mov	w8, #-80
	stur	w8, [x29, #-4]
.LBB342_37:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end342:
	.size	hynix16nm_major_check_byte, .Lfunc_end342-hynix16nm_major_check_byte
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_get_read_retry_cfg    // -- Begin function hynix16nm_get_read_retry_cfg
	.p2align	2
	.type	hynix16nm_get_read_retry_cfg,@function
hynix16nm_get_read_retry_cfg:           // @hynix16nm_get_read_retry_cfg
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160                    // =160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144                   // =144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	stur	x2, [x29, #-32]
	stur	x3, [x29, #-40]
	stur	wzr, [x29, #-44]
	stur	wzr, [x29, #-48]
	movi	v0.2d, #0000000000000000
	str	q0, [sp, #48]
	str	q0, [sp, #64]
	movi	v0.2d, #0000000000000000
	str	q0, [sp, #16]
	str	q0, [sp, #32]
	mov	w8, #32
	str	w8, [sp, #12]
	stur	wzr, [x29, #-52]
.LBB343_1:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-52]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB343_4
// %bb.2:                               //   in Loop: Header=BB343_1 Depth=1
	ldur	x8, [x29, #-40]
	ldur	w9, [x29, #-52]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldur	w9, [x29, #-52]
	mov	w10, w9
	add	x9, sp, #48                     // =48
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB343_1 Depth=1
	ldur	w8, [x29, #-52]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-52]
	b	.LBB343_1
.LBB343_4:
	ldur	x0, [x29, #-16]
	mov	w1, wzr
	mov	w2, #4
	add	x3, sp, #48                     // =48
	add	x4, sp, #16                     // =16
	mov	w5, #8
	bl	hynix16nm_major_check_byte
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-48]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB343_6
// %bb.5:
	adrp	x0, .L.str.460
	add	x0, x0, :lo12:.L.str.460
	bl	nand_print
	ldur	w8, [x29, #-48]
	stur	w8, [x29, #-4]
	b	.LBB343_38
.LBB343_6:
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8]
	adrp	x0, .L.str.461
	add	x0, x0, :lo12:.L.str.461
	bl	nand_print_dbg
// %bb.7:
	stur	wzr, [x29, #-52]
.LBB343_8:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-52]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB343_11
// %bb.9:                               //   in Loop: Header=BB343_8 Depth=1
	ldur	x8, [x29, #-40]
	ldur	w9, [x29, #-52]
	add	w9, w9, #8                      // =8
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldur	w9, [x29, #-52]
	mov	w10, w9
	add	x9, sp, #48                     // =48
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.10:                              //   in Loop: Header=BB343_8 Depth=1
	ldur	w8, [x29, #-52]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-52]
	b	.LBB343_8
.LBB343_11:
	ldur	x0, [x29, #-24]
	mov	w1, wzr
	mov	w2, #4
	add	x3, sp, #48                     // =48
	add	x4, sp, #16                     // =16
	mov	w5, #8
	bl	hynix16nm_major_check_byte
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-48]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB343_13
// %bb.12:
	adrp	x0, .L.str.462
	add	x0, x0, :lo12:.L.str.462
	bl	nand_print
	ldur	w8, [x29, #-48]
	stur	w8, [x29, #-4]
	b	.LBB343_38
.LBB343_13:
	ldur	x8, [x29, #-24]
	ldrb	w1, [x8]
	adrp	x0, .L.str.463
	add	x0, x0, :lo12:.L.str.463
	bl	nand_print_dbg
// %bb.14:
	ldur	x8, [x29, #-16]
	ldrb	w8, [x8]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB343_16
// %bb.15:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8]
	subs	w8, w8, #4                      // =4
	b.eq	.LBB343_17
.LBB343_16:
	ldur	x8, [x29, #-16]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-24]
	ldrb	w2, [x8]
	adrp	x0, .L.str.464
	add	x0, x0, :lo12:.L.str.464
	bl	nand_print
	mov	w8, #-81
	stur	w8, [x29, #-4]
	b	.LBB343_38
.LBB343_17:
	stur	wzr, [x29, #-56]
.LBB343_18:                             // =>This Loop Header: Depth=1
                                        //     Child Loop BB343_20 Depth 2
	ldur	w8, [x29, #-56]
	ldr	w9, [sp, #12]
	subs	w8, w8, w9
	b.hs	.LBB343_27
// %bb.19:                              //   in Loop: Header=BB343_18 Depth=1
	stur	wzr, [x29, #-60]
.LBB343_20:                             //   Parent Loop BB343_18 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldur	w8, [x29, #-60]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB343_23
// %bb.21:                              //   in Loop: Header=BB343_20 Depth=2
	ldur	w8, [x29, #-60]
	mov	w9, w8
	add	x10, sp, #48                    // =48
	mov	x8, x10
	add	x8, x8, x9
	strb	wzr, [x8]
	ldur	w8, [x29, #-60]
	mov	w11, w8
	add	x9, sp, #16                     // =16
	mov	x8, x9
	add	x8, x8, x11
	strb	wzr, [x8]
	ldur	x8, [x29, #-40]
	ldur	w11, [x29, #-60]
	ldr	w12, [sp, #12]
	mul	w11, w11, w12
	lsl	w11, w11, #1
	add	w11, w11, #16                   // =16
	ldur	w12, [x29, #-56]
	add	w11, w11, w12
                                        // kill: def $x11 killed $w11
	add	x8, x8, x11
	ldrb	w8, [x8]
	ldur	w11, [x29, #-60]
                                        // kill: def $x11 killed $w11
	add	x10, x10, x11
	strb	w8, [x10]
	ldur	x8, [x29, #-40]
	ldur	w10, [x29, #-60]
	ldr	w11, [sp, #12]
	mul	w10, w10, w11
	lsl	w10, w10, #1
	add	w10, w10, #16                   // =16
	ldr	w11, [sp, #12]
	add	w10, w10, w11
	ldur	w11, [x29, #-56]
	add	w10, w10, w11
                                        // kill: def $x10 killed $w10
	add	x8, x8, x10
	ldrb	w8, [x8]
	ldur	w10, [x29, #-60]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.22:                              //   in Loop: Header=BB343_20 Depth=2
	ldur	w8, [x29, #-60]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-60]
	b	.LBB343_20
.LBB343_23:                             //   in Loop: Header=BB343_18 Depth=1
	ldur	x8, [x29, #-32]
	ldur	w9, [x29, #-56]
                                        // kill: def $x9 killed $w9
	add	x0, x8, x9
	mov	w1, #1
	mov	w2, #4
	add	x3, sp, #48                     // =48
	add	x4, sp, #16                     // =16
	mov	w5, #8
	bl	hynix16nm_major_check_byte
	stur	w0, [x29, #-48]
	ldur	w8, [x29, #-48]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB343_25
// %bb.24:
	ldur	w1, [x29, #-56]
	adrp	x0, .L.str.465
	add	x0, x0, :lo12:.L.str.465
	bl	nand_print
	ldur	w8, [x29, #-48]
	stur	w8, [x29, #-44]
	b	.LBB343_27
.LBB343_25:                             //   in Loop: Header=BB343_18 Depth=1
// %bb.26:                              //   in Loop: Header=BB343_18 Depth=1
	ldur	w8, [x29, #-56]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-56]
	b	.LBB343_18
.LBB343_27:
	stur	wzr, [x29, #-56]
.LBB343_28:                             // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-56]
	ldr	w9, [sp, #12]
	subs	w8, w8, w9
	b.hs	.LBB343_34
// %bb.29:                              //   in Loop: Header=BB343_28 Depth=1
	ldur	w8, [x29, #-56]
	mov	w9, #-1
	add	w9, w9, #8                      // =8
	and	w8, w8, w9
	cbnz	w8, .LBB343_32
// %bb.30:                              //   in Loop: Header=BB343_28 Depth=1
	ldur	w8, [x29, #-56]
	cbz	w8, .LBB343_32
// %bb.31:                              //   in Loop: Header=BB343_28 Depth=1
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
.LBB343_32:                             //   in Loop: Header=BB343_28 Depth=1
	ldur	x8, [x29, #-32]
	ldur	w9, [x29, #-56]
                                        // kill: def $x9 killed $w9
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.93
	add	x0, x0, :lo12:.L.str.93
	bl	nand_print_dbg
// %bb.33:                              //   in Loop: Header=BB343_28 Depth=1
	ldur	w8, [x29, #-56]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-56]
	b	.LBB343_28
.LBB343_34:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
	ldur	w8, [x29, #-44]
	cbz	w8, .LBB343_36
// %bb.35:
	ldur	w8, [x29, #-44]
	stur	w8, [x29, #-48]
	b	.LBB343_37
.LBB343_36:
	stur	wzr, [x29, #-48]
.LBB343_37:
	ldur	w8, [x29, #-48]
	stur	w8, [x29, #-4]
.LBB343_38:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160                    // =160
	ret
.Lfunc_end343:
	.size	hynix16nm_get_read_retry_cfg, .Lfunc_end343-hynix16nm_get_read_retry_cfg
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_read_otp_info_hynix   // -- Begin function hynix16nm_read_otp_info_hynix
	.p2align	2
	.type	hynix16nm_read_otp_info_hynix,@function
hynix16nm_read_otp_info_hynix:          // @hynix16nm_read_otp_info_hynix
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #128                    // =128
	stp	x29, x30, [sp, #112]            // 16-byte Folded Spill
	add	x29, sp, #112                   // =112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	ldur	x0, [x29, #-16]
	bl	nand_enable_chip
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	add	x8, x8, #1264                   // =1264
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	w1, [x8, x9, lsl #2]
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	add	x8, x8, #1296                   // =1296
	ldur	x9, [x29, #-16]
	ldr	w9, [x9, #28]
                                        // kill: def $x9 killed $w9
	ldr	w2, [x8, x9, lsl #2]
	adrp	x0, .L.str.466
	add	x0, x0, :lo12:.L.str.466
	bl	nand_print_dbg
	ldur	x8, [x29, #-24]
	stur	x8, [x29, #-32]
	ldur	x8, [x29, #-32]
	cbnz	x8, .LBB344_2
// %bb.1:
	ldur	x0, [x29, #-16]
	bl	nand_disable_chip
	adrp	x0, .L.str.467
	add	x0, x0, :lo12:.L.str.467
	bl	nand_print
	mov	w8, #-79
	stur	w8, [x29, #-4]
	b	.LBB344_6
.LBB344_2:
	ldur	x0, [x29, #-16]
	mov	w1, #-1
	str	w1, [sp, #12]                   // 4-byte Folded Spill
	mov	w2, #1
	stur	w2, [x29, #-48]                 // 4-byte Folded Spill
	bl	set_one_cmd
	ldur	w6, [x29, #-48]                 // 4-byte Folded Reload
	sub	x1, x29, #38                    // =38
	str	x1, [sp, #24]                   // 8-byte Folded Spill
	mov	w8, #54
	str	w8, [sp, #16]                   // 4-byte Folded Spill
	sturb	w8, [x29, #-38]
	sub	x3, x29, #37                    // =37
	str	x3, [sp, #32]                   // 8-byte Folded Spill
	mov	w8, #56
	str	w8, [sp, #20]                   // 4-byte Folded Spill
	sturb	w8, [x29, #-37]
	sub	x4, x29, #39                    // =39
	str	x4, [sp, #40]                   // 8-byte Folded Spill
	mov	w8, #82
	sturb	w8, [x29, #-39]
	ldur	x0, [x29, #-16]
	mov	w2, wzr
	str	w2, [sp, #56]                   // 4-byte Folded Spill
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #22
	str	w1, [sp, #52]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #23
	bl	set_one_cmd
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #4
	bl	set_one_cmd
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #25
	bl	set_one_cmd
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, wzr
	stur	w1, [x29, #-52]                 // 4-byte Folded Spill
	bl	set_one_cmd
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	x0, [x29, #-16]
	mov	w1, #2
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	mov	w8, #48
	sturb	w8, [x29, #-38]
	ldur	x0, [x29, #-16]
	ldur	x3, [x29, #-32]
	mov	x2, xzr
	mov	w4, #528
	bl	get_data_with_nand_bus_one_cmd
	ldr	w1, [sp, #12]                   // 4-byte Folded Reload
	ldur	w2, [x29, #-48]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_cmd
	ldr	w9, [sp, #16]                   // 4-byte Folded Reload
	ldr	w8, [sp, #20]                   // 4-byte Folded Reload
	ldr	x1, [sp, #24]                   // 8-byte Folded Reload
	ldr	x3, [sp, #32]                   // 8-byte Folded Reload
	ldr	x4, [sp, #40]                   // 8-byte Folded Reload
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	w6, [x29, #-48]                 // 4-byte Folded Reload
	sturb	w9, [x29, #-38]
	sturb	w8, [x29, #-37]
	sturb	wzr, [x29, #-39]
	ldur	x0, [x29, #-16]
	mov	w5, w6
	bl	set_cmd_with_nand_bus
	ldr	w1, [sp, #52]                   // 4-byte Folded Reload
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_cmd
	ldr	w2, [sp, #56]                   // 4-byte Folded Reload
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_cmd
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w1, [x29, #-52]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	bl	set_one_addr
	ldur	w2, [x29, #-48]                 // 4-byte Folded Reload
	ldur	x0, [x29, #-16]
	mov	w1, #48
	bl	set_one_cmd
	ldur	x8, [x29, #-32]
	ldrb	w8, [x8]
	subs	w8, w8, #8                      // =8
	b.ne	.LBB344_4
// %bb.3:
	ldur	x8, [x29, #-32]
	ldrb	w8, [x8, #1]
	subs	w8, w8, #8                      // =8
	b.eq	.LBB344_5
.LBB344_4:
	ldur	x8, [x29, #-32]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-32]
	ldrb	w2, [x8, #1]
	adrp	x0, .L.str.428
	add	x0, x0, :lo12:.L.str.428
	bl	nand_print
	ldur	x0, [x29, #-16]
	bl	nand_disable_chip
	mov	w8, #-96
	stur	w8, [x29, #-44]
	ldur	w8, [x29, #-44]
	stur	w8, [x29, #-4]
	b	.LBB344_6
.LBB344_5:
	ldur	x0, [x29, #-16]
	bl	nand_disable_chip
	stur	wzr, [x29, #-4]
.LBB344_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #112]            // 16-byte Folded Reload
	add	sp, sp, #128                    // =128
	ret
.Lfunc_end344:
	.size	hynix16nm_read_otp_info_hynix, .Lfunc_end344-hynix16nm_read_otp_info_hynix
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_get_rr_value_otp_hynix // -- Begin function hynix16nm_get_rr_value_otp_hynix
	.p2align	2
	.type	hynix16nm_get_rr_value_otp_hynix,@function
hynix16nm_get_rr_value_otp_hynix:       // @hynix16nm_get_rr_value_otp_hynix
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	stur	wzr, [x29, #-20]
	mov	w0, #528
	bl	nand_get_temp_buf
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	cbnz	x8, .LBB345_2
// %bb.1:
	adrp	x0, .L.str.468
	add	x0, x0, :lo12:.L.str.468
	bl	nand_print
	mov	w8, #-78
	stur	w8, [x29, #-4]
	b	.LBB345_7
.LBB345_2:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #16]
	bl	hynix16nm_read_otp_info_hynix
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB345_4
// %bb.3:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #28]
	adrp	x0, .L.str.469
	add	x0, x0, :lo12:.L.str.469
	bl	nand_print
.LBB345_4:
	ldur	x8, [x29, #-16]
	add	x2, x8, #56                     // =56
	ldr	x3, [sp, #16]
	add	x0, sp, #15                     // =15
	add	x1, sp, #14                     // =14
	bl	hynix16nm_get_read_retry_cfg
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	subs	w8, w8, #0                      // =0
	b.ge	.LBB345_6
// %bb.5:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldur	x8, [x29, #-16]
	ldr	w2, [x8, #28]
	adrp	x0, .L.str.470
	add	x0, x0, :lo12:.L.str.470
	bl	nand_print
.LBB345_6:
	ldr	x0, [sp, #16]
	bl	nand_free_temp_buf
	ldur	w8, [x29, #-20]
	stur	w8, [x29, #-4]
.LBB345_7:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end345:
	.size	hynix16nm_get_rr_value_otp_hynix, .Lfunc_end345-hynix16nm_get_rr_value_otp_hynix
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_save_otp_value        // -- Begin function hynix16nm_save_otp_value
	.p2align	2
	.type	hynix16nm_save_otp_value,@function
hynix16nm_save_otp_value:               // @hynix16nm_save_otp_value
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	str	wzr, [sp, #12]
.LBB346_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w8, w8, #32                     // =32
	b.hs	.LBB346_4
// %bb.2:                               //   in Loop: Header=BB346_1 Depth=1
	ldr	x8, [sp, #16]
	ldr	w9, [sp, #12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	x9, [sp, #24]
	add	x9, x9, #56                     // =56
	ldr	w10, [sp, #12]
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB346_1 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB346_1
.LBB346_4:
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end346:
	.size	hynix16nm_save_otp_value, .Lfunc_end346-hynix16nm_save_otp_value
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_get_otp_param         // -- Begin function hynix16nm_get_otp_param
	.p2align	2
	.type	hynix16nm_get_otp_param,@function
hynix16nm_get_otp_param:                // @hynix16nm_get_otp_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	str	wzr, [sp, #16]
	str	wzr, [sp, #12]
.LBB347_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	cbnz	w8, .LBB347_5
// %bb.2:                               //   in Loop: Header=BB347_1 Depth=1
	ldr	w1, [sp, #16]
	adrp	x0, .L.str.431
	add	x0, x0, :lo12:.L.str.431
	bl	nand_print_dbg
	ldur	x0, [x29, #-8]
	bl	hynix16nm_get_rr_value_otp_hynix
	stur	w0, [x29, #-20]
	ldur	w8, [x29, #-20]
	cbnz	w8, .LBB347_4
// %bb.3:                               //   in Loop: Header=BB347_1 Depth=1
	mov	w8, #1
	str	w8, [sp, #12]
.LBB347_4:                              //   in Loop: Header=BB347_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB347_1
.LBB347_5:
	str	wzr, [sp, #24]
.LBB347_6:                              // =>This Loop Header: Depth=1
                                        //     Child Loop BB347_8 Depth 2
	ldr	w8, [sp, #24]
	subs	w8, w8, #8                      // =8
	b.hs	.LBB347_13
// %bb.7:                               //   in Loop: Header=BB347_6 Depth=1
	str	wzr, [sp, #20]
.LBB347_8:                              //   Parent Loop BB347_6 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #4                      // =4
	b.hs	.LBB347_11
// %bb.9:                               //   in Loop: Header=BB347_8 Depth=2
	ldur	x8, [x29, #-8]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #24]
	mov	w10, #4
	mul	w9, w10, w9
	ldr	w11, [sp, #20]
	add	w9, w9, w11
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldur	x9, [x29, #-16]
	ldr	w11, [sp, #24]
	mul	w10, w10, w11
	ldr	w11, [sp, #20]
	add	w10, w10, w11
                                        // kill: def $x10 killed $w10
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.10:                              //   in Loop: Header=BB347_8 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #20]
	b	.LBB347_8
.LBB347_11:                             //   in Loop: Header=BB347_6 Depth=1
// %bb.12:                              //   in Loop: Header=BB347_6 Depth=1
	ldr	w8, [sp, #24]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #24]
	b	.LBB347_6
.LBB347_13:
	mov	w0, wzr
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end347:
	.size	hynix16nm_get_otp_param, .Lfunc_end347-hynix16nm_get_otp_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_set_default_param     // -- Begin function hynix16nm_set_default_param
	.p2align	2
	.type	hynix16nm_set_default_param,@function
hynix16nm_set_default_param:            // @hynix16nm_set_default_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
.LBB348_1:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	adrp	x9, hynix16nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix16nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB348_4
// %bb.2:                               //   in Loop: Header=BB348_1 Depth=1
	ldur	x8, [x29, #-8]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #16]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldr	w9, [sp, #16]
	mov	w10, w9
	add	x9, sp, #8                      // =8
	add	x9, x9, x10
	strb	w8, [x9]
// %bb.3:                               //   in Loop: Header=BB348_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB348_1
.LBB348_4:
	ldur	x0, [x29, #-8]
	adrp	x8, hynix16nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix16nm_read_retry_reg_num]
	add	x1, sp, #8                      // =8
	adrp	x2, hynix16nm_read_retry_reg_adr
	add	x2, x2, :lo12:hynix16nm_read_retry_reg_adr
	bl	hynix16nm_vender_set_param
	stur	w0, [x29, #-12]
	adrp	x0, .L.str.385
	add	x0, x0, :lo12:.L.str.385
	bl	nand_print_dbg
	str	wzr, [sp, #16]
.LBB348_5:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	adrp	x9, hynix16nm_read_retry_reg_num
	ldrb	w9, [x9, :lo12:hynix16nm_read_retry_reg_num]
	subs	w8, w8, w9
	b.hs	.LBB348_8
// %bb.6:                               //   in Loop: Header=BB348_5 Depth=1
	ldr	w8, [sp, #16]
	mov	w9, w8
	add	x8, sp, #8                      // =8
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.386
	add	x0, x0, :lo12:.L.str.386
	bl	nand_print_dbg
// %bb.7:                               //   in Loop: Header=BB348_5 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #16]
	b	.LBB348_5
.LBB348_8:
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
	ldur	w0, [x29, #-12]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end348:
	.size	hynix16nm_set_default_param, .Lfunc_end348-hynix16nm_set_default_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_init_readretry_param  // -- Begin function hynix16nm_init_readretry_param
	.p2align	2
	.type	hynix16nm_init_readretry_param,@function
hynix16nm_init_readretry_param:         // @hynix16nm_init_readretry_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #176                    // =176
	stp	x29, x30, [sp, #160]            // 16-byte Folded Spill
	add	x29, sp, #160                   // =160
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	mov	w1, wzr
	stur	wzr, [x29, #-20]
	str	xzr, [sp, #64]
	str	wzr, [sp, #60]
	str	xzr, [sp, #48]
	add	x0, sp, #8                      // =8
	mov	x2, #40
	bl	memset
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #20]
	lsl	w0, w8, #9
	bl	nand_get_temp_buf
	str	x0, [sp, #64]
	ldr	x8, [sp, #64]
	cbnz	x8, .LBB349_2
// %bb.1:
	mov	w8, #-77
	stur	w8, [x29, #-4]
	b	.LBB349_15
.LBB349_2:
	mov	w0, #32768
	bl	nand_get_temp_buf
	str	x0, [sp, #48]
	ldr	x8, [sp, #48]
	cbnz	x8, .LBB349_4
// %bb.3:
	ldr	x0, [sp, #64]
	bl	nand_free_temp_buf
	adrp	x0, .L.str.434
	add	x0, x0, :lo12:.L.str.434
	bl	nand_print
	mov	w8, #-76
	stur	w8, [x29, #-4]
	b	.LBB349_15
.LBB349_4:
// %bb.5:
	str	wzr, [sp, #60]
	ldr	x0, [sp, #48]
	ldr	x1, [sp, #64]
	ldur	x8, [x29, #-16]
	ldr	w3, [x8, #24]
	mov	w2, #32
	bl	hynix16nm_get_special_info
	str	w0, [sp, #60]
	ldr	w8, [sp, #60]
	cbnz	w8, .LBB349_13
// %bb.6:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	w2, [sp, #8]
	adrp	x0, .L.str.471
	add	x0, x0, :lo12:.L.str.471
	bl	nand_print_dbg
	stur	wzr, [x29, #-20]
.LBB349_7:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-20]
	subs	w8, w8, #32                     // =32
	b.hs	.LBB349_12
// %bb.8:                               //   in Loop: Header=BB349_7 Depth=1
	ldr	x8, [sp, #64]
	ldur	w9, [x29, #-20]
                                        // kill: def $x9 killed $w9
	ldrb	w1, [x8, x9]
	adrp	x0, .L.str.436
	add	x0, x0, :lo12:.L.str.436
	bl	nand_print_dbg
	ldur	w8, [x29, #-20]
	mov	w9, #-1
	add	w9, w9, #8                      // =8
	and	w8, w8, w9
	subs	w8, w8, #7                      // =7
	b.ne	.LBB349_10
// %bb.9:                               //   in Loop: Header=BB349_7 Depth=1
	adrp	x0, .L.str.91
	add	x0, x0, :lo12:.L.str.91
	bl	nand_print_dbg
.LBB349_10:                             //   in Loop: Header=BB349_7 Depth=1
// %bb.11:                              //   in Loop: Header=BB349_7 Depth=1
	ldur	w8, [x29, #-20]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-20]
	b	.LBB349_7
.LBB349_12:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #64]
	bl	hynix16nm_save_otp_value
	ldur	x0, [x29, #-16]
	bl	hynix16nm_set_default_param
	b	.LBB349_14
.LBB349_13:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x0, .L.str.437
	add	x0, x0, :lo12:.L.str.437
	bl	nand_print_dbg
	ldur	x0, [x29, #-16]
	add	x1, sp, #76                     // =76
	bl	hynix16nm_get_otp_param
	ldur	x0, [x29, #-16]
	bl	hynix16nm_set_default_param
	ldur	x8, [x29, #-16]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	w2, [sp, #8]
	adrp	x0, .L.str.472
	add	x0, x0, :lo12:.L.str.472
	bl	nand_print_dbg
.LBB349_14:
	ldr	x0, [sp, #48]
	bl	nand_free_temp_buf
	ldr	x0, [sp, #64]
	bl	nand_free_temp_buf
	stur	wzr, [x29, #-4]
.LBB349_15:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #160]            // 16-byte Folded Reload
	add	sp, sp, #176                    // =176
	ret
.Lfunc_end349:
	.size	hynix16nm_init_readretry_param, .Lfunc_end349-hynix16nm_init_readretry_param
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_readretry_init        // -- Begin function hynix16nm_readretry_init
	.p2align	2
	.type	hynix16nm_readretry_init,@function
hynix16nm_readretry_init:               // @hynix16nm_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, hynix16nm_read_retry_reg_adr
	add	x8, x8, :lo12:hynix16nm_read_retry_reg_adr
	str	x8, [sp]                        // 8-byte Folded Spill
	str	x0, [sp, #16]
	ldr	x8, [sp, #16]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w9, w8, #0xff
	adrp	x8, hynix16nm_read_retry_mode
	strb	w9, [x8, :lo12:hynix16nm_read_retry_mode]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	lsr	w9, w9, #8
	and	w9, w9, #0xff
	adrp	x10, hynix16nm_read_retry_cycle
	strb	w9, [x10, :lo12:hynix16nm_read_retry_cycle]
	ldr	x9, [sp, #16]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, hynix16nm_read_retry_reg_num
	strb	w9, [x10, :lo12:hynix16nm_read_retry_reg_num]
	ldrb	w9, [x8, :lo12:hynix16nm_read_retry_mode]
	mov	w8, #4
	subs	w8, w8, w9
	b.ne	.LBB350_2
// %bb.1:
	ldr	x9, [sp]                        // 8-byte Folded Reload
	adrp	x10, hynix16nm_read_retry_reg_adr
	mov	w8, #56
	strb	w8, [x10, :lo12:hynix16nm_read_retry_reg_adr]
	mov	w8, #57
	strb	w8, [x9, #1]
	mov	w8, #58
	strb	w8, [x9, #2]
	mov	w8, #59
	strb	w8, [x9, #3]
	b	.LBB350_3
.LBB350_2:
	adrp	x8, hynix16nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix16nm_read_retry_mode]
	adrp	x0, .L.str.388
	add	x0, x0, :lo12:.L.str.388
	bl	nand_print
	mov	w8, #-75
	stur	w8, [x29, #-4]
	b	.LBB350_8
.LBB350_3:
	ldr	x8, [sp, #16]
	str	wzr, [x8, #184]
	str	wzr, [sp, #12]
.LBB350_4:                              // =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #12]
	subs	w8, w8, #128                    // =128
	b.hs	.LBB350_7
// %bb.5:                               //   in Loop: Header=BB350_4 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #56                     // =56
	ldr	w9, [sp, #12]
                                        // kill: def $x9 killed $w9
	add	x8, x8, x9
	strb	wzr, [x8]
// %bb.6:                               //   in Loop: Header=BB350_4 Depth=1
	ldr	w8, [sp, #12]
	add	w8, w8, #1                      // =1
	str	w8, [sp, #12]
	b	.LBB350_4
.LBB350_7:
	ldr	x0, [sp, #16]
	bl	hynix16nm_init_readretry_param
	stur	wzr, [x29, #-4]
.LBB350_8:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end350:
	.size	hynix16nm_readretry_init, .Lfunc_end350-hynix16nm_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_readretry_exit        // -- Begin function hynix16nm_readretry_exit
	.p2align	2
	.type	hynix16nm_readretry_exit,@function
hynix16nm_readretry_exit:               // @hynix16nm_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	adrp	x0, .L.str.473
	add	x0, x0, :lo12:.L.str.473
	bl	nand_print_dbg
	ldr	x0, [sp, #8]
	bl	hynix16nm_set_default_param
	ldr	x8, [sp, #8]
	mov	w0, wzr
	str	wzr, [x8, #184]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end351:
	.size	hynix16nm_readretry_exit, .Lfunc_end351-hynix16nm_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_special_init          // -- Begin function hynix16nm_special_init
	.p2align	2
	.type	hynix16nm_special_init,@function
hynix16nm_special_init:                 // @hynix16nm_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB352_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB352_3
// %bb.2:                               //   in Loop: Header=BB352_1 Depth=1
	ldr	x0, [sp]
	bl	hynix16nm_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB352_1
.LBB352_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB352_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, hynix16nm_read_page_end
	add	x8, x8, :lo12:hynix16nm_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, hynix16nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix16nm_read_retry_mode]
	adrp	x8, hynix16nm_read_retry_cycle
	ldrb	w2, [x8, :lo12:hynix16nm_read_retry_cycle]
	adrp	x8, hynix16nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix16nm_read_retry_reg_num]
	adrp	x0, .L.str.474
	add	x0, x0, :lo12:.L.str.474
	bl	nand_print_dbg
	b	.LBB352_6
.LBB352_5:
	adrp	x8, hynix16nm_read_retry_mode
	ldrb	w1, [x8, :lo12:hynix16nm_read_retry_mode]
	adrp	x8, hynix16nm_read_retry_cycle
	ldrb	w2, [x8, :lo12:hynix16nm_read_retry_cycle]
	adrp	x8, hynix16nm_read_retry_reg_num
	ldrb	w3, [x8, :lo12:hynix16nm_read_retry_reg_num]
	adrp	x0, .L.str.475
	add	x0, x0, :lo12:.L.str.475
	bl	nand_print
.LBB352_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end352:
	.size	hynix16nm_special_init, .Lfunc_end352-hynix16nm_special_init
	.cfi_endproc
                                        // -- End function
	.globl	hynix16nm_special_exit          // -- Begin function hynix16nm_special_exit
	.p2align	2
	.type	hynix16nm_special_exit,@function
hynix16nm_special_exit:                 // @hynix16nm_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB353_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB353_3
// %bb.2:                               //   in Loop: Header=BB353_1 Depth=1
	ldr	x0, [sp, #8]
	bl	hynix16nm_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB353_1
.LBB353_3:
	adrp	x0, .L.str.476
	add	x0, x0, :lo12:.L.str.476
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end353:
	.size	hynix16nm_special_exit, .Lfunc_end353-hynix16nm_special_exit
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_read_retry_clock_save   // -- Begin function toshiba_read_retry_clock_save
	.p2align	2
	.type	toshiba_read_retry_clock_save,@function
toshiba_read_retry_clock_save:          // @toshiba_read_retry_clock_save
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	str	x0, [sp]                        // 8-byte Folded Spill
	adrp	x2, toshiba_sclk0_bak
	add	x2, x2, :lo12:toshiba_sclk0_bak
	adrp	x3, toshiba_sclk1_bak
	add	x3, x3, :lo12:toshiba_sclk1_bak
	bl	nand_get_clk
	ldr	x0, [sp]                        // 8-byte Folded Reload
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	mov	w2, #10
	mov	w3, #20
	bl	nand_set_clk
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end354:
	.size	toshiba_read_retry_clock_save, .Lfunc_end354-toshiba_read_retry_clock_save
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_read_retry_clock_recover // -- Begin function toshiba_read_retry_clock_recover
	.p2align	2
	.type	toshiba_read_retry_clock_recover,@function
toshiba_read_retry_clock_recover:       // @toshiba_read_retry_clock_recover
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	adrp	x8, toshiba_sclk0_bak
	ldr	w2, [x8, :lo12:toshiba_sclk0_bak]
	adrp	x8, toshiba_sclk1_bak
	ldr	w3, [x8, :lo12:toshiba_sclk1_bak]
	adrp	x0, aw_ndfc
	add	x0, x0, :lo12:aw_ndfc
	bl	nand_set_clk
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end355:
	.size	toshiba_read_retry_clock_recover, .Lfunc_end355-toshiba_read_retry_clock_recover
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_vender_set_param        // -- Begin function toshiba_vender_set_param
	.p2align	2
	.type	toshiba_vender_set_param,@function
toshiba_vender_set_param:               // @toshiba_vender_set_param
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80                     // =80
	stp	x29, x30, [sp, #64]             // 16-byte Folded Spill
	add	x29, sp, #64                    // =64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, toshiba_read_retry_end_cmd
	add	x8, x8, :lo12:toshiba_read_retry_end_cmd
	str	x8, [sp, #16]                   // 8-byte Folded Spill
	stur	x0, [x29, #-16]
	stur	x1, [x29, #-24]
	str	x2, [sp, #32]
	str	w3, [sp, #28]
	str	wzr, [sp, #24]
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #18                     // =18
	b.ne	.LBB356_8
// %bb.1:
	ldur	x0, [x29, #-16]
	ldr	x3, [sp, #32]
	ldur	x4, [x29, #-24]
	ldr	w6, [sp, #28]
	adrp	x1, toshiba_read_retry_cmd_2
	add	x1, x1, :lo12:toshiba_read_retry_cmd_2
	mov	w2, wzr
	mov	w5, #1
	bl	set_cmd_with_nand_bus
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8]
	cbnz	w8, .LBB356_7
// %bb.2:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #1]
	cbnz	w8, .LBB356_7
// %bb.3:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #2]
	cbnz	w8, .LBB356_7
// %bb.4:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #3]
	cbnz	w8, .LBB356_7
// %bb.5:
	adrp	x8, toshiba_retry_flag
	ldr	w8, [x8, :lo12:toshiba_retry_flag]
	cbnz	w8, .LBB356_7
// %bb.6:
	ldur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_exit_cmd
	ldrb	w1, [x8, :lo12:toshiba_read_retry_exit_cmd]
	mov	w2, #1
	bl	set_one_cmd
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
	ldr	w8, [sp, #24]
	stur	w8, [x29, #-4]
	b	.LBB356_29
.LBB356_7:
.LBB356_8:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #17                     // =17
	b.ne	.LBB356_20
// %bb.9:
	ldur	x0, [x29, #-16]
	ldr	x3, [sp, #32]
	ldur	x4, [x29, #-24]
	ldr	w6, [sp, #28]
	adrp	x1, toshiba_read_retry_cmd_2
	add	x1, x1, :lo12:toshiba_read_retry_cmd_2
	mov	w2, wzr
	mov	w5, #1
	bl	set_cmd_with_nand_bus
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8]
	cbnz	w8, .LBB356_14
// %bb.10:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #1]
	cbnz	w8, .LBB356_14
// %bb.11:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #2]
	cbnz	w8, .LBB356_14
// %bb.12:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #3]
	cbnz	w8, .LBB356_14
// %bb.13:
	ldur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_exit_cmd
	ldrb	w1, [x8, :lo12:toshiba_read_retry_exit_cmd]
	mov	w2, #1
	bl	set_one_cmd
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
	ldr	w8, [sp, #24]
	stur	w8, [x29, #-4]
	b	.LBB356_29
.LBB356_14:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8]
	mov	w8, #2
	subs	w8, w8, w9
	b.ne	.LBB356_19
// %bb.15:
	ldur	x8, [x29, #-24]
	ldrb	w8, [x8, #1]
	cbnz	w8, .LBB356_19
// %bb.16:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8, #2]
	mov	w8, #126
	subs	w8, w8, w9
	b.ne	.LBB356_19
// %bb.17:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8, #3]
	mov	w8, #124
	subs	w8, w8, w9
	b.ne	.LBB356_19
// %bb.18:
	ldur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_last_cmd
	ldrb	w1, [x8, :lo12:toshiba_read_retry_last_cmd]
	mov	w2, wzr
	bl	set_one_cmd
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
.LBB356_19:
.LBB356_20:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #16                     // =16
	b.ne	.LBB356_28
// %bb.21:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8]
	mov	w8, #255
	subs	w8, w8, w9
	b.ne	.LBB356_26
// %bb.22:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8, #1]
	mov	w8, #255
	subs	w8, w8, w9
	b.ne	.LBB356_26
// %bb.23:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8, #2]
	mov	w8, #255
	subs	w8, w8, w9
	b.ne	.LBB356_26
// %bb.24:
	ldur	x8, [x29, #-24]
	ldrb	w9, [x8, #3]
	mov	w8, #255
	subs	w8, w8, w9
	b.ne	.LBB356_26
// %bb.25:
	ldur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_exit_cmd
	ldrb	w1, [x8, :lo12:toshiba_read_retry_exit_cmd]
	mov	w2, #1
	bl	set_one_cmd
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
	ldr	w8, [sp, #24]
	stur	w8, [x29, #-4]
	b	.LBB356_29
.LBB356_26:
	ldur	x0, [x29, #-16]
	ldr	x3, [sp, #32]
	ldur	x4, [x29, #-24]
	ldr	w6, [sp, #28]
	adrp	x1, toshiba_read_retry_cmd_1
	add	x1, x1, :lo12:toshiba_read_retry_cmd_1
	mov	w2, wzr
	mov	w5, #1
	bl	set_cmd_with_nand_bus
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
// %bb.27:
.LBB356_28:
	ldur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_end_cmd
	ldrb	w1, [x8, :lo12:toshiba_read_retry_end_cmd]
	mov	w2, wzr
	str	w2, [sp, #12]                   // 4-byte Folded Spill
	bl	set_one_cmd
	ldr	x8, [sp, #16]                   // 8-byte Folded Reload
	ldr	w2, [sp, #12]                   // 4-byte Folded Reload
	ldr	w9, [sp, #24]
	orr	w9, w9, w0
	str	w9, [sp, #24]
	ldur	x0, [x29, #-16]
	ldrb	w1, [x8, #1]
	bl	set_one_cmd
	ldr	w8, [sp, #24]
	orr	w8, w8, w0
	str	w8, [sp, #24]
	ldur	x8, [x29, #-24]
	ldrb	w1, [x8]
	ldur	x8, [x29, #-24]
	ldrb	w2, [x8, #1]
	ldur	x8, [x29, #-24]
	ldrb	w3, [x8, #2]
	ldur	x8, [x29, #-24]
	ldrb	w4, [x8, #3]
	adrp	x0, .L.str.477
	add	x0, x0, :lo12:.L.str.477
	bl	nand_print_dbg
	ldr	w8, [sp, #24]
	stur	w8, [x29, #-4]
.LBB356_29:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #64]             // 16-byte Folded Reload
	add	sp, sp, #80                     // =80
	ret
.Lfunc_end356:
	.size	toshiba_vender_set_param, .Lfunc_end356-toshiba_vender_set_param
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_vender_pre_condition    // -- Begin function toshiba_vender_pre_condition
	.p2align	2
	.type	toshiba_vender_pre_condition,@function
toshiba_vender_pre_condition:           // @toshiba_vender_pre_condition
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #16]
	ldr	x0, [sp, #16]
	adrp	x1, toshiba_read_retry_start_cmd
	add	x1, x1, :lo12:toshiba_read_retry_start_cmd
	mov	w5, wzr
	mov	w2, w5
	mov	x4, xzr
	mov	x3, x4
	mov	w6, #2
	bl	set_cmd_with_nand_bus
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	cbz	w8, .LBB357_2
// %bb.1:
	adrp	x0, .L.str.478
	add	x0, x0, :lo12:.L.str.478
	bl	nand_print
	ldr	w8, [sp, #12]
	stur	w8, [x29, #-4]
	b	.LBB357_3
.LBB357_2:
	stur	wzr, [x29, #-4]
.LBB357_3:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end357:
	.size	toshiba_vender_pre_condition, .Lfunc_end357-toshiba_vender_pre_condition
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_set_readretry           // -- Begin function toshiba_set_readretry
	.p2align	2
	.type	toshiba_set_readretry,@function
toshiba_set_readretry:                  // @toshiba_set_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64                     // =64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48                    // =48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #16                     // =16
	b.eq	.LBB358_4
// %bb.1:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #17                     // =17
	b.eq	.LBB358_4
// %bb.2:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #18                     // =18
	b.eq	.LBB358_4
// %bb.3:
	mov	w8, #-89
	stur	w8, [x29, #-4]
	b	.LBB358_17
.LBB358_4:
	ldur	x0, [x29, #-16]
	bl	toshiba_read_retry_clock_save
	ldur	x0, [x29, #-16]
	bl	nand_enable_chip
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #184]
	cbnz	w8, .LBB358_6
// %bb.5:
	ldur	x0, [x29, #-16]
	bl	toshiba_vender_pre_condition
.LBB358_6:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_is_toggle_interface
	cbz	w0, .LBB358_8
// %bb.7:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_set_legacy_interface
	adrp	x9, toshiba_toggle_mode_flag
	mov	w8, #1
	str	w8, [x9, :lo12:toshiba_toggle_mode_flag]
.LBB358_8:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #17                     // =17
	b.ne	.LBB358_10
// %bb.9:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #184]
                                        // kill: def $x8 killed $w8
	mov	x9, #5
	mul	x9, x8, x9
	adrp	x8, toshiba_p1_2
	add	x8, x8, :lo12:toshiba_p1_2
	add	x8, x8, x9
	str	x8, [sp, #16]
	adrp	x8, toshiba_read_retry_addr_2
	add	x8, x8, :lo12:toshiba_read_retry_addr_2
	str	x8, [sp, #8]
	mov	w8, #5
	str	w8, [sp, #4]
	b	.LBB358_14
.LBB358_10:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #18                     // =18
	b.ne	.LBB358_12
// %bb.11:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #184]
                                        // kill: def $x8 killed $w8
	mov	x9, #5
	mul	x9, x8, x9
	adrp	x8, toshiba_p1_3
	add	x8, x8, :lo12:toshiba_p1_3
	add	x8, x8, x9
	str	x8, [sp, #16]
	adrp	x8, toshiba_read_retry_addr_2
	add	x8, x8, :lo12:toshiba_read_retry_addr_2
	str	x8, [sp, #8]
	mov	w8, #5
	str	w8, [sp, #4]
	b	.LBB358_13
.LBB358_12:
	ldur	x8, [x29, #-16]
	ldr	w8, [x8, #184]
	mov	w9, w8
	adrp	x8, toshiba_p1_1
	add	x8, x8, :lo12:toshiba_p1_1
	add	x8, x8, x9, lsl #2
	str	x8, [sp, #16]
	adrp	x8, toshiba_read_retry_addr_1
	add	x8, x8, :lo12:toshiba_read_retry_addr_1
	str	x8, [sp, #8]
	mov	w8, #4
	str	w8, [sp, #4]
.LBB358_13:
.LBB358_14:
	ldur	x0, [x29, #-16]
	ldr	x1, [sp, #16]
	ldr	x2, [sp, #8]
	ldr	w3, [sp, #4]
	bl	toshiba_vender_set_param
	stur	w0, [x29, #-20]
	adrp	x8, toshiba_toggle_mode_flag
	ldr	w8, [x8, :lo12:toshiba_toggle_mode_flag]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB358_16
// %bb.15:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_set_toggle_interface
.LBB358_16:
	ldur	x0, [x29, #-16]
	bl	nand_disable_chip
	ldur	x0, [x29, #-16]
	bl	toshiba_read_retry_clock_recover
	ldur	w8, [x29, #-20]
	stur	w8, [x29, #-4]
.LBB358_17:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64                     // =64
	ret
.Lfunc_end358:
	.size	toshiba_set_readretry, .Lfunc_end358-toshiba_set_readretry
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_read_page_end           // -- Begin function toshiba_read_page_end
	.p2align	2
	.type	toshiba_read_page_end,@function
toshiba_read_page_end:                  // @toshiba_read_page_end
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48                     // =48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32                    // =32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	wzr, [x29, #-12]
	str	wzr, [sp, #16]
	adrp	x8, g_nsi
	ldr	x0, [x8, :lo12:g_nsi]
	ldur	x8, [x29, #-8]
	ldr	w1, [x8]
	bl	nci_get_from_nsi
	str	x0, [sp, #8]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	adds	w8, w8, #2                      // =2
	b.ne	.LBB359_11
// %bb.1:
	adrp	x0, .L.str.479
	add	x0, x0, :lo12:.L.str.479
	bl	nand_print_dbg
	adrp	x9, toshiba_retry_flag
	mov	w8, #1
	str	w8, [x9, :lo12:toshiba_retry_flag]
	stur	wzr, [x29, #-12]
.LBB359_2:                              // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-12]
	adrp	x9, toshiba_read_retry_cycle
	ldrb	w9, [x9, :lo12:toshiba_read_retry_cycle]
	subs	w8, w8, w9
	b.ge	.LBB359_10
// %bb.3:                               //   in Loop: Header=BB359_2 Depth=1
	ldur	w8, [x29, #-12]
	ldr	x9, [sp, #8]
	str	w8, [x9, #184]
	ldr	x0, [sp, #8]
	bl	toshiba_set_readretry
	str	w0, [sp, #16]
	ldr	w8, [sp, #16]
	cbz	w8, .LBB359_5
// %bb.4:                               //   in Loop: Header=BB359_2 Depth=1
	b	.LBB359_9
.LBB359_5:                              //   in Loop: Header=BB359_2 Depth=1
	ldur	x0, [x29, #-8]
	bl	generic_read_page_start
	str	w0, [sp, #16]
	ldur	x0, [x29, #-8]
	bl	generic_read_page_end_not_retry
	ldr	w8, [sp, #16]
	orr	w8, w8, w0
	str	w8, [sp, #16]
	ldr	w8, [sp, #16]
	subs	w8, w8, #10                     // =10
	b.eq	.LBB359_7
// %bb.6:                               //   in Loop: Header=BB359_2 Depth=1
	ldr	w8, [sp, #16]
	cbnz	w8, .LBB359_8
.LBB359_7:
	mov	w8, #10
	str	w8, [sp, #16]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #280]
	ldr	w1, [x8, #12]
	ldr	x8, [sp, #8]
	ldr	w2, [x8, #28]
	ldur	x8, [x29, #-8]
	ldr	w3, [x8, #4]
	ldur	x8, [x29, #-8]
	ldr	w4, [x8, #8]
	ldr	x8, [sp, #8]
	ldr	w5, [x8, #184]
	adrp	x0, .L.str.480
	add	x0, x0, :lo12:.L.str.480
	bl	nand_print_dbg
	b	.LBB359_10
.LBB359_8:                              //   in Loop: Header=BB359_2 Depth=1
.LBB359_9:                              //   in Loop: Header=BB359_2 Depth=1
	ldur	w8, [x29, #-12]
	add	w8, w8, #1                      // =1
	stur	w8, [x29, #-12]
	b	.LBB359_2
.LBB359_10:
	adrp	x8, toshiba_retry_flag
	str	wzr, [x8, :lo12:toshiba_retry_flag]
	ldr	x8, [sp, #8]
	str	wzr, [x8, #184]
	ldr	x0, [sp, #8]
	bl	toshiba_exit_readretry
.LBB359_11:
	ldr	w0, [sp, #16]
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48                     // =48
	ret
.Lfunc_end359:
	.size	toshiba_read_page_end, .Lfunc_end359-toshiba_read_page_end
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_exit_readretry          // -- Begin function toshiba_exit_readretry
	.p2align	2
	.type	toshiba_exit_readretry,@function
toshiba_exit_readretry:                 // @toshiba_exit_readretry
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96                     // =96
	stp	x29, x30, [sp, #80]             // 16-byte Folded Spill
	add	x29, sp, #80                    // =80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, toshiba_p1_1
	add	x8, x8, :lo12:toshiba_p1_1
	add	x8, x8, #24                     // =24
	str	x8, [sp, #8]                    // 8-byte Folded Spill
	adrp	x8, toshiba_p1_3
	add	x8, x8, :lo12:toshiba_p1_3
	add	x8, x8, #50                     // =50
	str	x8, [sp, #16]                   // 8-byte Folded Spill
	adrp	x8, toshiba_p1_2
	add	x8, x8, :lo12:toshiba_p1_2
	add	x8, x8, #35                     // =35
	str	x8, [sp, #24]                   // 8-byte Folded Spill
	stur	x0, [x29, #-16]
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #16                     // =16
	b.eq	.LBB360_4
// %bb.1:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #17                     // =17
	b.eq	.LBB360_4
// %bb.2:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #18                     // =18
	b.eq	.LBB360_4
// %bb.3:
	mov	w8, #-88
	stur	w8, [x29, #-4]
	b	.LBB360_15
.LBB360_4:
	ldur	x0, [x29, #-16]
	bl	nand_enable_chip
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_disable_randomize
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_is_toggle_interface
	cbz	w0, .LBB360_6
// %bb.5:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_set_legacy_interface
	adrp	x9, toshiba_toggle_mode_flag
	mov	w8, #1
	str	w8, [x9, :lo12:toshiba_toggle_mode_flag]
.LBB360_6:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #17                     // =17
	b.ne	.LBB360_8
// %bb.7:
	ldr	x8, [sp, #24]                   // 8-byte Folded Reload
	stur	x8, [x29, #-32]
	adrp	x8, toshiba_read_retry_addr_2
	add	x8, x8, :lo12:toshiba_read_retry_addr_2
	str	x8, [sp, #40]
	mov	w8, #5
	str	w8, [sp, #36]
	b	.LBB360_12
.LBB360_8:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w8, [x8, :lo12:toshiba_read_retry_mode]
	subs	w8, w8, #18                     // =18
	b.ne	.LBB360_10
// %bb.9:
	ldr	x8, [sp, #16]                   // 8-byte Folded Reload
	stur	x8, [x29, #-32]
	adrp	x8, toshiba_read_retry_addr_2
	add	x8, x8, :lo12:toshiba_read_retry_addr_2
	str	x8, [sp, #40]
	mov	w8, #5
	str	w8, [sp, #36]
	b	.LBB360_11
.LBB360_10:
	ldr	x8, [sp, #8]                    // 8-byte Folded Reload
	stur	x8, [x29, #-32]
	adrp	x8, toshiba_read_retry_addr_1
	add	x8, x8, :lo12:toshiba_read_retry_addr_1
	str	x8, [sp, #40]
	str	wzr, [sp, #36]
.LBB360_11:
.LBB360_12:
	ldur	x0, [x29, #-16]
	ldur	x1, [x29, #-32]
	ldr	x2, [sp, #40]
	ldr	w3, [sp, #36]
	bl	toshiba_vender_set_param
	stur	w0, [x29, #-20]
	adrp	x8, toshiba_toggle_mode_flag
	ldr	w8, [x8, :lo12:toshiba_toggle_mode_flag]
	subs	w8, w8, #1                      // =1
	b.ne	.LBB360_14
// %bb.13:
	ldur	x8, [x29, #-16]
	ldr	x0, [x8, #280]
	bl	ndfc_set_toggle_interface
.LBB360_14:
	ldur	x0, [x29, #-16]
	bl	nand_disable_chip
	ldur	w8, [x29, #-20]
	stur	w8, [x29, #-4]
.LBB360_15:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #80]             // 16-byte Folded Reload
	add	sp, sp, #96                     // =96
	ret
.Lfunc_end360:
	.size	toshiba_exit_readretry, .Lfunc_end360-toshiba_exit_readretry
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_readretry_init          // -- Begin function toshiba_readretry_init
	.p2align	2
	.type	toshiba_readretry_init,@function
toshiba_readretry_init:                 // @toshiba_readretry_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	mov	w9, wzr
	str	w9, [sp, #4]                    // 4-byte Folded Spill
	str	wzr, [x8, #184]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #16
	and	w8, w8, #0xff
	adrp	x9, toshiba_read_retry_mode
	strb	w8, [x9, :lo12:toshiba_read_retry_mode]
	ldr	x8, [sp, #8]
	ldr	x8, [x8, #288]
	ldr	w8, [x8, #52]
	lsr	w8, w8, #8
	and	w9, w8, #0xff
	adrp	x8, toshiba_read_retry_cycle
	strb	w9, [x8, :lo12:toshiba_read_retry_cycle]
	ldr	x9, [sp, #8]
	ldr	x9, [x9, #288]
	ldr	w9, [x9, #52]
	and	w9, w9, #0xff
	adrp	x10, toshiba_read_retry_reg_cnt
	strb	w9, [x10, :lo12:toshiba_read_retry_reg_cnt]
	ldrb	w1, [x8, :lo12:toshiba_read_retry_cycle]
	adrp	x0, .L.str.481
	add	x0, x0, :lo12:.L.str.481
	bl	nand_print_dbg
	ldr	w0, [sp, #4]                    // 4-byte Folded Reload
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end361:
	.size	toshiba_readretry_init, .Lfunc_end361-toshiba_readretry_init
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_readretry_exit          // -- Begin function toshiba_readretry_exit
	.p2align	2
	.type	toshiba_readretry_exit,@function
toshiba_readretry_exit:                 // @toshiba_readretry_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16                     // =16
	.cfi_def_cfa_offset 16
	str	x0, [sp, #8]
	mov	w0, wzr
	add	sp, sp, #16                     // =16
	ret
.Lfunc_end362:
	.size	toshiba_readretry_exit, .Lfunc_end362-toshiba_readretry_exit
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_special_init            // -- Begin function toshiba_special_init
	.p2align	2
	.type	toshiba_special_init,@function
toshiba_special_init:                   // @toshiba_special_init
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp]
.LBB363_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	cbz	x8, .LBB363_3
// %bb.2:                               //   in Loop: Header=BB363_1 Depth=1
	ldr	x0, [sp]
	bl	toshiba_readretry_init
	ldur	w8, [x29, #-4]
	orr	w8, w8, w0
	stur	w8, [x29, #-4]
	ldr	x8, [sp]
	ldr	x8, [x8]
	str	x8, [sp]
	b	.LBB363_1
.LBB363_3:
	ldur	w8, [x29, #-4]
	cbnz	w8, .LBB363_5
// %bb.4:
	adrp	x9, df_read_page_end
	adrp	x8, toshiba_read_page_end
	add	x8, x8, :lo12:toshiba_read_page_end
	str	x8, [x9, :lo12:df_read_page_end]
	adrp	x8, toshiba_read_retry_mode
	ldrb	w1, [x8, :lo12:toshiba_read_retry_mode]
	adrp	x8, toshiba_read_retry_cycle
	ldrb	w2, [x8, :lo12:toshiba_read_retry_cycle]
	adrp	x8, toshiba_read_retry_reg_cnt
	ldrb	w3, [x8, :lo12:toshiba_read_retry_reg_cnt]
	adrp	x0, .L.str.482
	add	x0, x0, :lo12:.L.str.482
	bl	nand_print_dbg
	b	.LBB363_6
.LBB363_5:
	adrp	x8, toshiba_read_retry_mode
	ldrb	w1, [x8, :lo12:toshiba_read_retry_mode]
	adrp	x8, toshiba_read_retry_cycle
	ldrb	w2, [x8, :lo12:toshiba_read_retry_cycle]
	adrp	x8, toshiba_read_retry_reg_cnt
	ldrb	w3, [x8, :lo12:toshiba_read_retry_reg_cnt]
	adrp	x0, .L.str.483
	add	x0, x0, :lo12:.L.str.483
	bl	nand_print
.LBB363_6:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end363:
	.size	toshiba_special_init, .Lfunc_end363-toshiba_special_init
	.cfi_endproc
                                        // -- End function
	.globl	toshiba_special_exit            // -- Begin function toshiba_special_exit
	.p2align	2
	.type	toshiba_special_exit,@function
toshiba_special_exit:                   // @toshiba_special_exit
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32                     // =32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16                    // =16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, g_nsi
	ldr	x8, [x8, :lo12:g_nsi]
	ldr	x8, [x8, #8]
	str	x8, [sp, #8]
.LBB364_1:                              // =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	cbz	x8, .LBB364_3
// %bb.2:                               //   in Loop: Header=BB364_1 Depth=1
	ldr	x0, [sp, #8]
	bl	toshiba_readretry_exit
	ldr	x8, [sp, #8]
	ldr	x8, [x8]
	str	x8, [sp, #8]
	b	.LBB364_1
.LBB364_3:
	adrp	x0, .L.str.484
	add	x0, x0, :lo12:.L.str.484
	bl	nand_print_dbg
	mov	w0, wzr
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32                     // =32
	ret
.Lfunc_end364:
	.size	toshiba_special_exit, .Lfunc_end364-toshiba_special_exit
	.cfi_endproc
                                        // -- End function
	.type	com,@object                     // @com
	.section	.rodata,"a",@progbits
	.globl	com
com:
	.asciz	"compatible"
	.size	com, 11

	.type	mod,@object                     // @mod
	.globl	mod
mod:
	.asciz	"model"
	.size	mod, 6

	.type	gc_src_type,@object             // @gc_src_type
	.local	gc_src_type
	.comm	gc_src_type,4,4
	.type	.L.str,@object                  // @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"[NE]garbage_collect phy_block null\n"
	.size	.L.str, 36

	.type	.L.str.1,@object                // @.str.1
.L.str.1:
	.asciz	"[ND]garbage too frequency! %d:%d\n"
	.size	.L.str.1, 34

	.type	.L.str.2,@object                // @.str.2
.L.str.2:
	.asciz	"[NE]garbage_collect_first wrong fbn:%d\n"
	.size	.L.str.2, 40

	.type	.L.str.3,@object                // @.str.3
.L.str.3:
	.asciz	"[NI]garbage_collect_first none!!\n"
	.size	.L.str.3, 34

	.type	.L.str.4,@object                // @.str.4
.L.str.4:
	.asciz	"[ND]garbage_collect_first wrong!!\n"
	.size	.L.str.4, 35

	.type	.L.str.5,@object                // @.str.5
.L.str.5:
	.asciz	"[ND]garbage_collect_first cannot find PHY_MAPPING_PAGE%d %d %d %d!!\n"
	.size	.L.str.5, 69

	.type	.L.str.6,@object                // @.str.6
.L.str.6:
	.asciz	"[ND]garbage_collect_first cannot find PHY_MAPPING_PAGE%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x.\n"
	.size	.L.str.6, 104

	.type	.L.str.7,@object                // @.str.7
.L.str.7:
	.asciz	"[ND][NE]garbage_collect_first rebuild %d,%d!!\n"
	.size	.L.str.7, 47

	.type	.L.str.8,@object                // @.str.8
.L.str.8:
	.asciz	"[ND]garbage_collect_first PHY_MAPPING_PAGEecc err block:%d !!\n"
	.size	.L.str.8, 63

	.type	.L.str.9,@object                // @.str.9
.L.str.9:
	.asciz	"[NE]garbage_collect_first invaild_page num error!!\n"
	.size	.L.str.9, 52

	.type	.L.str.10,@object               // @.str.10
.L.str.10:
	.asciz	"[NE]%s read fail %d %d,read one more time!!\n"
	.size	.L.str.10, 45

	.type	.L__func__.garbage_collect_first,@object // @__func__.garbage_collect_first
.L__func__.garbage_collect_first:
	.asciz	"garbage_collect_first"
	.size	.L__func__.garbage_collect_first, 22

	.type	.L.str.11,@object               // @.str.11
.L.str.11:
	.asciz	"[NE]%s read fail %d %d,%d times,don't read more!!\n"
	.size	.L.str.11, 51

	.type	.L.str.12,@object               // @.str.12
.L.str.12:
	.asciz	"[ND]garbage_collect_first read a logic page error%d %d %d %d spare data:\n"
	.size	.L.str.12, 74

	.type	.L.str.13,@object               // @.str.13
.L.str.13:
	.asciz	"%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n"
	.size	.L.str.13, 49

	.type	.L.str.14,@object               // @.str.14
.L.str.14:
	.asciz	"[NE]nand_write_logic_page_no_gc gc fail!!\n"
	.size	.L.str.14, 43

	.type	.L.str.15,@object               // @.str.15
.L.str.15:
	.asciz	"[NE]erase ecc error block\n"
	.size	.L.str.15, 27

	.type	.L.str.16,@object               // @.str.16
.L.str.16:
	.asciz	"[ND]no block to prio gc\n"
	.size	.L.str.16, 25

	.type	.L.str.17,@object               // @.str.17
.L.str.17:
	.asciz	"[ND]no need gc:%d, type:%d\n"
	.size	.L.str.17, 28

	.type	.L.str.18,@object               // @.str.18
.L.str.18:
	.asciz	"[NE]do_prio_gc garbage_collect_first fail:%d, type:%d\n"
	.size	.L.str.18, 55

	.type	.L.str.19,@object               // @.str.19
.L.str.19:
	.asciz	"[ND]prio gc fail1:%d, type:%d\n"
	.size	.L.str.19, 31

	.type	.L.str.20,@object               // @.str.20
.L.str.20:
	.asciz	"[ND]prio gc fail2:%d, type:%d\n"
	.size	.L.str.20, 31

	.type	.L.str.21,@object               // @.str.21
.L.str.21:
	.asciz	"[ND]swl: static WL need to do(%d : %d)\n"
	.size	.L.str.21, 40

	.type	.L.str.22,@object               // @.str.22
.L.str.22:
	.asciz	"[ND]swl: min_ec_blk_no = %d,\n"
	.size	.L.str.22, 30

	.type	.L.str.23,@object               // @.str.23
.L.str.23:
	.asciz	"[ND]swl: zone->blocks = %d)\n"
	.size	.L.str.23, 29

	.type	.L.str.24,@object               // @.str.24
.L.str.24:
	.asciz	"[ND]swl: the block(%d) for static WL is not in invalid_list\n"
	.size	.L.str.24, 61

	.type	.L.str.25,@object               // @.str.25
.L.str.25:
	.asciz	"[ND]swl: done_static_wear_leveling(min_ec) block:%d, erase_count:%d\n"
	.size	.L.str.25, 69

	.type	.L.str.26,@object               // @.str.26
.L.str.26:
	.asciz	"[NE]swl: garbage_collect_first fail:%d\n"
	.size	.L.str.26, 40

	.type	.L.str.27,@object               // @.str.27
.L.str.27:
	.asciz	"[NE]swl: last->check fail:%d\n"
	.size	.L.str.27, 30

	.type	.L.str.28,@object               // @.str.28
.L.str.28:
	.asciz	"[ND]gc_one ok! from: %d [%d %d %d] @%s %d\n"
	.size	.L.str.28, 43

	.type	.L.str.29,@object               // @.str.29
.L.str.29:
	.asciz	"[NE][NE]gc_one error!\n"
	.size	.L.str.29, 23

	.type	phyinfo,@object                 // @phyinfo
	.bss
	.globl	phyinfo
	.p2align	2
phyinfo:
	.zero	20
	.size	phyinfo, 20

	.type	.L.str.30,@object               // @.str.30
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.30:
	.asciz	"[ND][NE] gc_win_block gc_one error \n"
	.size	.L.str.30, 37

	.type	.L.str.31,@object               // @.str.31
.L.str.31:
	.asciz	"[ND]not power on gc [%d, %d, %d], [%d %d %d]!\n"
	.size	.L.str.31, 47

	.type	.L.str.32,@object               // @.str.32
.L.str.32:
	.asciz	"[ND]emergency power on gc %d !\n"
	.size	.L.str.32, 32

	.type	.L.str.33,@object               // @.str.33
.L.str.33:
	.asciz	"[ND]power on gc %d!\n"
	.size	.L.str.33, 21

	.type	.L.str.34,@object               // @.str.34
.L.str.34:
	.asciz	"[ND]%s %p %d %d %d %\n"
	.size	.L.str.34, 22

	.type	.L__func__.shutdown_on_gc,@object // @__func__.shutdown_on_gc
.L__func__.shutdown_on_gc:
	.asciz	"shutdown_on_gc"
	.size	.L__func__.shutdown_on_gc, 15

	.type	.L.str.35,@object               // @.str.35
.L.str.35:
	.asciz	"[NI]shutdown_on_gc end! [%d %d %d]\n"
	.size	.L.str.35, 36

	.type	.L.str.36,@object               // @.str.36
.L.str.36:
	.asciz	"[ND]shutdown_on_gc one ok! [%d %d %d]\n"
	.size	.L.str.36, 39

	.type	.L.str.37,@object               // @.str.37
.L.str.37:
	.asciz	"%d %d %d %d %d %d/%d %d/%d\n"
	.size	.L.str.37, 28

	.type	.L.str.38,@object               // @.str.38
.L.str.38:
	.asciz	"%08d:%04d "
	.size	.L.str.38, 11

	.type	.L.str.39,@object               // @.str.39
.L.str.39:
	.asciz	"[NE]%s:logic_page:0x%x >= total_pages:0x%x\n"
	.size	.L.str.39, 44

	.type	.L__func__.get_logic_page_map,@object // @__func__.get_logic_page_map
.L__func__.get_logic_page_map:
	.asciz	"get_logic_page_map"
	.size	.L__func__.get_logic_page_map, 19

	.type	.L.str.40,@object               // @.str.40
.L.str.40:
	.asciz	"[NE]%s:malloc fail for logic_page_map %d\n"
	.size	.L.str.40, 42

	.type	.L.str.41,@object               // @.str.41
.L.str.41:
	.asciz	"[NE]%s: malloc fail for logic_page_map %d\n"
	.size	.L.str.41, 43

	.type	.L.str.42,@object               // @.str.42
.L.str.42:
	.asciz	"[NE]zone_param_init malloc_logic_page_map error %d\n"
	.size	.L.str.42, 52

	.type	.L.str.43,@object               // @.str.43
.L.str.43:
	.asciz	"[ND]free list: \n"
	.size	.L.str.43, 17

	.type	.L.str.44,@object               // @.str.44
.L.str.44:
	.asciz	"[ND]block NO:%4d; erase:%3d; "
	.size	.L.str.44, 30

	.type	.L.str.45,@object               // @.str.45
.L.str.45:
	.asciz	"[ND]count:%4d; "
	.size	.L.str.45, 16

	.type	.L.str.46,@object               // @.str.46
.L.str.46:
	.asciz	"[ND]info:0x%x\n"
	.size	.L.str.46, 15

	.type	.L.str.47,@object               // @.str.47
.L.str.47:
	.asciz	"[ND]invalid_page_count list: \n"
	.size	.L.str.47, 31

	.type	.L.str.48,@object               // @.str.48
.L.str.48:
	.asciz	"[ND]count:%4d; invalid:%3d;"
	.size	.L.str.48, 28

	.type	.L.str.49,@object               // @.str.49
.L.str.49:
	.asciz	"[ND]block_used_count list: \n"
	.size	.L.str.49, 29

	.type	.L.str.50,@object               // @.str.50
.L.str.50:
	.asciz	"[ND]block NO: %d "
	.size	.L.str.50, 18

	.type	.L.str.51,@object               // @.str.51
.L.str.51:
	.asciz	"[ND]block_used_count: %d \n"
	.size	.L.str.51, 27

	.type	.L.str.52,@object               // @.str.52
.L.str.52:
	.asciz	"[ND]free_list error !\n"
	.size	.L.str.52, 23

	.type	.L.str.53,@object               // @.str.53
.L.str.53:
	.asciz	"[ND]add invalid_list error !\n"
	.size	.L.str.53, 30

	.type	.L.str.54,@object               // @.str.54
.L.str.54:
	.asciz	"[ND]invalid_page_count more than pages per blk 0 %d %d!!\n"
	.size	.L.str.54, 58

	.type	.L.str.55,@object               // @.str.55
.L.str.55:
	.asciz	"[ND]invalid_page_count more than pages per blk 1!!\n"
	.size	.L.str.55, 52

	.type	.L.str.56,@object               // @.str.56
.L.str.56:
	.asciz	"[NE]%s:malloc fail for cache_buf!\n"
	.size	.L.str.56, 35

	.type	.L__func__.nftl_sector_cache_init,@object // @__func__.nftl_sector_cache_init
.L__func__.nftl_sector_cache_init:
	.asciz	"nftl_sector_cache_init"
	.size	.L__func__.nftl_sector_cache_init, 23

	.type	.L.str.57,@object               // @.str.57
.L.str.57:
	.asciz	"[NE]nand_sector_cache_read 1!\n"
	.size	.L.str.57, 31

	.type	.L.str.58,@object               // @.str.58
.L.str.58:
	.asciz	"[NE]nand_sector_cache_write 1!\n"
	.size	.L.str.58, 32

	.type	.L.str.59,@object               // @.str.59
.L.str.59:
	.asciz	"[NE]nand_sector_cache_write 2!\n"
	.size	.L.str.59, 32

	.type	.L.str.60,@object               // @.str.60
.L.str.60:
	.asciz	"[NE]nand_flush_write_cache write error\n"
	.size	.L.str.60, 40

	.type	.L.str.61,@object               // @.str.61
.L.str.61:
	.asciz	"[NE]error nand_write_logic_page_no_gc page_no:%u\n"
	.size	.L.str.61, 50

	.type	.L.str.62,@object               // @.str.62
.L.str.62:
	.asciz	"[ND]logic_page_map_temp NULL!!\n"
	.size	.L.str.62, 32

	.type	.L.str.63,@object               // @.str.63
.L.str.63:
	.asciz	"[NE]read a page: %d READ_RECLAIM!\n"
	.size	.L.str.63, 35

	.type	.L.str.64,@object               // @.str.64
.L.str.64:
	.asciz	"[NE]no free block\n"
	.size	.L.str.64, 19

	.type	.L.str.65,@object               // @.str.65
.L.str.65:
	.asciz	"[NE]do_write_error erase error sb:%d!\n"
	.size	.L.str.65, 39

	.type	.L.str.66,@object               // @.str.66
.L.str.66:
	.asciz	"[NE]do_write_error sb@%d nftl_nand_copy_page error!\n"
	.size	.L.str.66, 53

	.type	.L.str.67,@object               // @.str.67
.L.str.67:
	.asciz	"[NE]do after write error1!\n"
	.size	.L.str.67, 28

	.type	.L.str.68,@object               // @.str.68
.L.str.68:
	.asciz	"[NE]do_before_write error!\n"
	.size	.L.str.68, 28

	.type	.L.str.69,@object               // @.str.69
.L.str.69:
	.asciz	"[NE]write data to page error!\n"
	.size	.L.str.69, 31

	.type	.L.str.70,@object               // @.str.70
.L.str.70:
	.asciz	"[NE]do after write error2!\n"
	.size	.L.str.70, 28

	.type	.L.str.71,@object               // @.str.71
.L.str.71:
	.asciz	"[NE]get_new_current_block error3!\n"
	.size	.L.str.71, 35

	.type	.L.str.72,@object               // @.str.72
.L.str.72:
	.asciz	"[NE]get_new_current_block error4!\n"
	.size	.L.str.72, 35

	.type	.L.str.73,@object               // @.str.73
.L.str.73:
	.asciz	"[NE]write_phy_page_map_to_current_block error!\n"
	.size	.L.str.73, 48

	.type	.L.str.74,@object               // @.str.74
.L.str.74:
	.asciz	"[NE]get_new_current_block error1!\n"
	.size	.L.str.74, 35

	.type	.L.str.75,@object               // @.str.75
.L.str.75:
	.asciz	"[NE]get_new_current_block error2!\n"
	.size	.L.str.75, 35

	.type	.L.str.76,@object               // @.str.76
.L.str.76:
	.asciz	"[NE]write logic data to current block error\n"
	.size	.L.str.76, 45

	.type	.L.str.77,@object               // @.str.77
.L.str.77:
	.asciz	"[ND]last block can not used to logic page %d!!\n"
	.size	.L.str.77, 48

	.type	.L.str.78,@object               // @.str.78
.L.str.78:
	.asciz	"[ND]last block can not used to power down page %d!!\n"
	.size	.L.str.78, 53

	.type	.L.str.79,@object               // @.str.79
.L.str.79:
	.asciz	"[ND]write_phy_data_to_current_block write error %d!\n"
	.size	.L.str.79, 53

	.type	.L.str.80,@object               // @.str.80
.L.str.80:
	.asciz	"[NE]do_write_error error\n"
	.size	.L.str.80, 26

	.type	.L.str.81,@object               // @.str.81
.L.str.81:
	.asciz	"[NE]write_phy_data_to_current_block error1\n"
	.size	.L.str.81, 44

	.type	.L.str.82,@object               // @.str.82
.L.str.82:
	.asciz	"[ND]logic_page_map NULL!!\n"
	.size	.L.str.82, 27

	.type	.L.str.83,@object               // @.str.83
.L.str.83:
	.asciz	"[ND]invalid_page_count more than pages_per_blk at write logic data to current block:%d!!\n"
	.size	.L.str.83, 90

	.type	.L.str.84,@object               // @.str.84
.L.str.84:
	.asciz	"[NE]write_phy_data_to_current_block error2\n"
	.size	.L.str.84, 44

	.type	.L.str.85,@object               // @.str.85
.L.str.85:
	.asciz	"[NE]invalid_page_count more than pages_per_blk at write logic data to current block:%d!!\n"
	.size	.L.str.85, 90

	.type	.L.str.86,@object               // @.str.86
.L.str.86:
	.asciz	"[NE]write phy data to current block error2\n"
	.size	.L.str.86, 44

	.type	.L.str.87,@object               // @.str.87
.L.str.87:
	.asciz	"[ND]swl: new_cur_blk->alloc ok(max_ec): block_no = %d; ec = %d; \n"
	.size	.L.str.87, 66

	.type	.L.str.88,@object               // @.str.88
.L.str.88:
	.asciz	"[NE]no free block!!!!\n"
	.size	.L.str.88, 23

	.type	.L.str.89,@object               // @.str.89
.L.str.89:
	.asciz	"[NE]new_block_init_for_write erase error!\n"
	.size	.L.str.89, 43

	.type	.L.str.90,@object               // @.str.90
.L.str.90:
	.asciz	"[NE]invaild_page num error:[%d] [%d]  used_page:[%d]!!\n"
	.size	.L.str.90, 56

	.type	.L.str.91,@object               // @.str.91
.L.str.91:
	.asciz	"\n"
	.size	.L.str.91, 2

	.type	.L.str.92,@object               // @.str.92
.L.str.92:
	.asciz	"%08x: "
	.size	.L.str.92, 7

	.type	.L.str.93,@object               // @.str.93
.L.str.93:
	.asciz	"%02x "
	.size	.L.str.93, 6

	.type	.L.str.94,@object               // @.str.94
.L.str.94:
	.asciz	"[NE]%s:malloc fail for temp_page_buf\n"
	.size	.L.str.94, 38

	.type	.L__func__.zone_param_init,@object // @__func__.zone_param_init
.L__func__.zone_param_init:
	.asciz	"zone_param_init"
	.size	.L__func__.zone_param_init, 16

	.type	.L.str.95,@object               // @.str.95
.L.str.95:
	.asciz	"[NE]%s:malloc fail for logic_page_buf\n"
	.size	.L.str.95, 39

	.type	.L.str.96,@object               // @.str.96
.L.str.96:
	.asciz	"[NE]zone_param_init error2\n"
	.size	.L.str.96, 28

	.type	.L.str.97,@object               // @.str.97
.L.str.97:
	.asciz	"[NE]%s:current_block.user_info.buf_size %d over page size %d\n"
	.size	.L.str.97, 62

	.type	.L.str.98,@object               // @.str.98
.L.str.98:
	.asciz	"[NE]%s:malloc fail for current_block.user_info.buf\n"
	.size	.L.str.98, 52

	.type	.L.str.99,@object               // @.str.99
.L.str.99:
	.asciz	"[NE]%s:malloc fail for assist_block.user_info.buf\n"
	.size	.L.str.99, 51

	.type	.L.str.100,@object              // @.str.100
.L.str.100:
	.asciz	"[NE]%s:malloc fail for zone_phy_page_map_for_gc.buf\n"
	.size	.L.str.100, 53

	.type	.L.str.101,@object              // @.str.101
.L.str.101:
	.asciz	"[NE]mtd size changed !\n"
	.size	.L.str.101, 24

	.type	.L.str.102,@object              // @.str.102
.L.str.102:
	.asciz	"[ND]build_zone_list_first.\n"
	.size	.L.str.102, 28

	.type	.L.str.103,@object              // @.str.103
.L.str.103:
	.asciz	"[NE]no block %d\n"
	.size	.L.str.103, 17

	.type	.L.str.104,@object              // @.str.104
.L.str.104:
	.asciz	"[ND]first\n"
	.size	.L.str.104, 11

	.type	.L.str.105,@object              // @.str.105
.L.str.105:
	.asciz	"[NE]init_zone_after_first_scan error\n"
	.size	.L.str.105, 38

	.type	.L.str.106,@object              // @.str.106
.L.str.106:
	.asciz	"[NE]second_scan_all_blocks error\n"
	.size	.L.str.106, 34

	.type	.L.str.107,@object              // @.str.107
.L.str.107:
	.asciz	"[NI]nftl ok!\n"
	.size	.L.str.107, 14

	.type	.L__const.first_scan_all_blocks.alises,@object // @__const.first_scan_all_blocks.alises
	.section	.rodata,"a",@progbits
.L__const.first_scan_all_blocks.alises:
	.ascii	"sunxi"
	.size	.L__const.first_scan_all_blocks.alises, 5

	.type	.L__const.first_scan_all_blocks.of_root,@object // @__const.first_scan_all_blocks.of_root
	.section	.rodata.str1.1,"aMS",@progbits,1
.L__const.first_scan_all_blocks.of_root:
	.asciz	"/"
	.size	.L__const.first_scan_all_blocks.of_root, 2

	.type	.L.str.108,@object              // @.str.108
.L.str.108:
	.asciz	"[ND]fsab\n"
	.size	.L.str.108, 10

	.type	.L.str.109,@object              // @.str.109
.L.str.109:
	.asciz	"[NI]ftl end block! %d!\n"
	.size	.L.str.109, 24

	.type	.L.str.110,@object              // @.str.110
.L.str.110:
	.asciz	"[ND]READ_RECLAIM!\n"
	.size	.L.str.110, 19

	.type	.L.str.111,@object              // @.str.111
.L.str.111:
	.asciz	"[NI]start page , do nothing\n"
	.size	.L.str.111, 29

	.type	.L.str.112,@object              // @.str.112
.L.str.112:
	.asciz	"[ND]fill block %d\n"
	.size	.L.str.112, 19

	.type	.L.str.113,@object              // @.str.113
.L.str.113:
	.asciz	"[NI]b@%d p@0 unknow spare data:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x.\n"
	.size	.L.str.113, 81

	.type	.L.str.114,@object              // @.str.114
.L.str.114:
	.asciz	"[NE]nftl bad block: %d\n"
	.size	.L.str.114, 24

	.type	.L.str.115,@object              // @.str.115
.L.str.115:
	.asciz	"[NE]nftl b@%d p@0 ecc error\n"
	.size	.L.str.115, 29

	.type	.L.str.116,@object              // @.str.116
.L.str.116:
	.asciz	"[NE]b@%d last page ok !\n"
	.size	.L.str.116, 25

	.type	.L.str.117,@object              // @.str.117
.L.str.117:
	.asciz	"[NI]READ_RECLAIM!\n"
	.size	.L.str.117, 19

	.type	.L.str.118,@object              // @.str.118
.L.str.118:
	.asciz	"[NE]last page blank ! %d!\n"
	.size	.L.str.118, 27

	.type	.L.str.119,@object              // @.str.119
.L.str.119:
	.asciz	"[NE]last page unkown ! %d!\n"
	.size	.L.str.119, 28

	.type	.L.str.120,@object              // @.str.120
.L.str.120:
	.asciz	"[NE]b@%d first page last page ecc error\n"
	.size	.L.str.120, 41

	.type	.L.str.121,@object              // @.str.121
.L.str.121:
	.asciz	"[NE]b@%d last page spare data %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
	.size	.L.str.121, 79

	.type	.L.str.122,@object              // @.str.122
.L.str.122:
	.asciz	"[NI]nftl bad block: %d\n"
	.size	.L.str.122, 24

	.type	.L.str.123,@object              // @.str.123
.L.str.123:
	.asciz	"[ND]before second %d %d.\n"
	.size	.L.str.123, 26

	.type	.L.str.124,@object              // @.str.124
.L.str.124:
	.asciz	"[NE]only have blocks:%u,logic cap need blocks:%u this zone not enough data block!!\n"
	.size	.L.str.124, 84

	.type	.L.str.125,@object              // @.str.125
.L.str.125:
	.asciz	"[NE]this zone not enough free block %d , %d!!\n"
	.size	.L.str.125, 47

	.type	.L.str.126,@object              // @.str.126
.L.str.126:
	.asciz	"[NI]NAND_EVENT: 2 current used block found %d,%d!\n"
	.size	.L.str.126, 51

	.type	.L.str.127,@object              // @.str.127
.L.str.127:
	.asciz	"[NE]no crosstalk:2 block used count diff %d,%d %d,%d !\n"
	.size	.L.str.127, 56

	.type	.L.str.128,@object              // @.str.128
.L.str.128:
	.asciz	"[NE]NAND_EVENT: muti current used block found1 %d %d!\n"
	.size	.L.str.128, 55

	.type	.L.str.129,@object              // @.str.129
.L.str.129:
	.asciz	"[NE]NAND_EVENT: muti current used block found2 %d %d!\n"
	.size	.L.str.129, 55

	.type	.L.str.130,@object              // @.str.130
.L.str.130:
	.asciz	"[NE]NAND_EVENT: muti current used block found3 %d %d!\n"
	.size	.L.str.130, 55

	.type	.L.str.131,@object              // @.str.131
.L.str.131:
	.asciz	"[NE]NAND_EVENT: last page spare data %x,%x,%x,%x,%x,%x,%x %x %x %x %x %x %x %x %x %x!!\n"
	.size	.L.str.131, 88

	.type	.L.str.132,@object              // @.str.132
.L.str.132:
	.asciz	"[NI]zone@%d b@%d last page unkown ecc ok.\n"
	.size	.L.str.132, 43

	.type	.L.str.133,@object              // @.str.133
.L.str.133:
	.asciz	"[NE]zone@%d b@%d last page ecc error.\n"
	.size	.L.str.133, 39

	.type	.L.str.134,@object              // @.str.134
.L.str.134:
	.asciz	"[NE]something strange %d,%d!!\n"
	.size	.L.str.134, 31

	.type	.L.str.135,@object              // @.str.135
.L.str.135:
	.asciz	"[NI]all block full!!\n"
	.size	.L.str.135, 22

	.type	.L.str.136,@object              // @.str.136
.L.str.136:
	.asciz	"[ND]corss talk rebuild 0 %d %d!!\n"
	.size	.L.str.136, 34

	.type	.L.str.137,@object              // @.str.137
.L.str.137:
	.asciz	"[NE]1st current block:%d 1st current block uc:%d muti current used block 1st.\n"
	.size	.L.str.137, 79

	.type	.L.str.138,@object              // @.str.138
.L.str.138:
	.asciz	"[ND]2nd current block:%d 2nd current block uc:%d muti current used block 2nd.\n"
	.size	.L.str.138, 79

	.type	.L.str.139,@object              // @.str.139
.L.str.139:
	.asciz	"[NE]something is error here 7!\n"
	.size	.L.str.139, 32

	.type	.L.str.140,@object              // @.str.140
.L.str.140:
	.asciz	"[NE]p_phy_block_info_last %d %d.\n"
	.size	.L.str.140, 34

	.type	.L.str.141,@object              // @.str.141
.L.str.141:
	.asciz	"[NE]get_current_used_block fail.\n"
	.size	.L.str.141, 34

	.type	.L.str.142,@object              // @.str.142
.L.str.142:
	.asciz	"[ND]free block nums is %d, aec: %d, iec: %d, a_buc: %d, i_buc: %d!\n"
	.size	.L.str.142, 68

	.type	.L.str.143,@object              // @.str.143
.L.str.143:
	.asciz	"[NE]current_block ecc error %d!\n"
	.size	.L.str.143, 33

	.type	.L.str.144,@object              // @.str.144
.L.str.144:
	.asciz	"[NE]error! no free block!\n"
	.size	.L.str.144, 27

	.type	.L.str.145,@object              // @.str.145
.L.str.145:
	.asciz	"[ND]recover %d %d\n"
	.size	.L.str.145, 19

	.type	.L.str.146,@object              // @.str.146
.L.str.146:
	.asciz	"[NE]page used:%d invalid page count:%d error\n"
	.size	.L.str.146, 46

	.type	.L.str.147,@object              // @.str.147
.L.str.147:
	.asciz	"[NE]new_block_init_for_write error here!\n"
	.size	.L.str.147, 42

	.type	.L.str.148,@object              // @.str.148
.L.str.148:
	.asciz	"[NE]something is error here 10!\n"
	.size	.L.str.148, 33

	.type	.L.str.149,@object              // @.str.149
.L.str.149:
	.asciz	"[NE]something is error here 11!\n"
	.size	.L.str.149, 33

	.type	.L.str.150,@object              // @.str.150
.L.str.150:
	.asciz	"[NE]assist block page used:%d current block page used:%d something is error here 12.\n"
	.size	.L.str.150, 86

	.type	.L.str.151,@object              // @.str.151
.L.str.151:
	.asciz	"[NE]assist block page used:%d current block page used:%d something is error here 17.\n"
	.size	.L.str.151, 86

	.type	.L.str.152,@object              // @.str.152
.L.str.152:
	.asciz	"[NE]something is error here 16!\n"
	.size	.L.str.152, 33

	.type	.L.str.153,@object              // @.str.153
.L.str.153:
	.asciz	"[ND]get a new free block\n"
	.size	.L.str.153, 26

	.type	.L.str.154,@object              // @.str.154
.L.str.154:
	.asciz	"[NE]no free block to use!\n"
	.size	.L.str.154, 27

	.type	.L.str.155,@object              // @.str.155
.L.str.155:
	.asciz	"[NE]sb@%d sp@%d recover_phy_page_mapping no used page\n"
	.size	.L.str.155, 55

	.type	.L.str.156,@object              // @.str.156
.L.str.156:
	.asciz	"[NE]sb@%d sp@%d power off ecc error1.\n"
	.size	.L.str.156, 39

	.type	.L.str.157,@object              // @.str.157
.L.str.157:
	.asciz	"[NE]invalid logic_page block:%d page:%d,logic_page 0x%x.\n"
	.size	.L.str.157, 58

	.type	.L.str.158,@object              // @.str.158
.L.str.158:
	.asciz	"[NE]rppm invalid page %d!!\n"
	.size	.L.str.158, 28

	.type	.L.str.159,@object              // @.str.159
.L.str.159:
	.asciz	"[NI]power_down page %d!!\n"
	.size	.L.str.159, 26

	.type	.L.str.160,@object              // @.str.160
.L.str.160:
	.asciz	"[NE]recover_phy_page_mapping unkown page data %d!!\n"
	.size	.L.str.160, 52

	.type	.L.str.161,@object              // @.str.161
.L.str.161:
	.asciz	"[NE]recover_phy_page_mapping ecc error block:%d;page:%d.\n"
	.size	.L.str.161, 58

	.type	.L.str.162,@object              // @.str.162
.L.str.162:
	.asciz	"[NE]ret:%d read fail %d %d,read one more time!!\n"
	.size	.L.str.162, 49

	.type	.L.str.163,@object              // @.str.163
.L.str.163:
	.asciz	"[NE]ret:%d read fail %d %d,%d times,don't read more!!\n"
	.size	.L.str.163, 55

	.type	.L.str.164,@object              // @.str.164
.L.str.164:
	.asciz	"[NE]power off ecc error 3!!\n"
	.size	.L.str.164, 29

	.type	.L.str.165,@object              // @.str.165
.L.str.165:
	.asciz	"[NE]recover sb@%d sp@%d phy page mapping error.\n"
	.size	.L.str.165, 49

	.type	.L.str.166,@object              // @.str.166
.L.str.166:
	.asciz	"[NI]logic_page_map is NULL\n"
	.size	.L.str.166, 28

	.type	.L.str.167,@object              // @.str.167
.L.str.167:
	.asciz	"[NE]invalid_page_count more than pages_per_blk :%d\n"
	.size	.L.str.167, 52

	.type	.L.str.168,@object              // @.str.168
.L.str.168:
	.asciz	"[NE]invalid_page data block:%d page:%d,logic_page 0x%x!!\n"
	.size	.L.str.168, 58

	.type	.L.str.169,@object              // @.str.169
.L.str.169:
	.asciz	"[NE]build invalid_page_count more than pages_per_blk!!\n"
	.size	.L.str.169, 56

	.type	.L.str.170,@object              // @.str.170
.L.str.170:
	.asciz	"[NE]NAND EVENT:do_write_error erase error1!\n"
	.size	.L.str.170, 45

	.type	.L.str.171,@object              // @.str.171
.L.str.171:
	.asciz	"[NE]NAND EVENT:do_write_error erase error2!\n"
	.size	.L.str.171, 45

	.type	.L.str.172,@object              // @.str.172
.L.str.172:
	.asciz	"%s %d:fail 1\n"
	.size	.L.str.172, 14

	.type	.L__func__.init_nrc,@object     // @__func__.init_nrc
.L__func__.init_nrc:
	.asciz	"init_nrc"
	.size	.L__func__.init_nrc, 9

	.type	.L.str.173,@object              // @.str.173
.L.str.173:
	.asciz	"%s %d:fail 2\n"
	.size	.L.str.173, 14

	.type	.L.str.174,@object              // @.str.174
.L.str.174:
	.asciz	"%s %d:fail 3\n"
	.size	.L.str.174, 14

	.type	.L.str.175,@object              // @.str.175
.L.str.175:
	.asciz	"%s %d:fail 4\n"
	.size	.L.str.175, 14

	.type	.L.str.176,@object              // @.str.176
.L.str.176:
	.asciz	"[NE]pages_per_blk: %d, %p, %p, %p, %p\n"
	.size	.L.str.176, 39

	.type	.L.str.177,@object              // @.str.177
.L.str.177:
	.asciz	"[NE]cross_talk_rebuild_current_block fail 1\n"
	.size	.L.str.177, 45

	.type	.L.str.178,@object              // @.str.178
.L.str.178:
	.asciz	"[NE]cross_talk_rebuild_current_block!! %d,%d,%d!\n"
	.size	.L.str.178, 50

	.type	.L.str.179,@object              // @.str.179
.L.str.179:
	.asciz	"[NE]cross_talk_rebuild_current_block fail 2\n"
	.size	.L.str.179, 45

	.type	.L.str.180,@object              // @.str.180
.L.str.180:
	.asciz	"[NE]cross_talk_rebuild_current_block!! %d,%d,%d,%d,%d,%d!\n"
	.size	.L.str.180, 59

	.type	.L.str.181,@object              // @.str.181
.L.str.181:
	.asciz	"[NE]cross_talk_rebuild_current_block fail 3\n"
	.size	.L.str.181, 45

	.type	.L.str.182,@object              // @.str.182
.L.str.182:
	.asciz	"[NE]corss talk rebuild 1 %d %d %d!\n"
	.size	.L.str.182, 36

	.type	.L.str.183,@object              // @.str.183
.L.str.183:
	.asciz	"[NE]corss talk rebuild 2 %d %d %d!\n"
	.size	.L.str.183, 36

	.type	.L.str.184,@object              // @.str.184
.L.str.184:
	.asciz	"[NE]get free block fail!\n"
	.size	.L.str.184, 26

	.type	.L.str.185,@object              // @.str.185
.L.str.185:
	.asciz	"[NE]something is error here 14!\n"
	.size	.L.str.185, 33

	.type	.L.str.186,@object              // @.str.186
.L.str.186:
	.asciz	"[NE]page lost %d %d\n"
	.size	.L.str.186, 21

	.type	.L.str.187,@object              // @.str.187
.L.str.187:
	.asciz	"[NI]nctrcb1 %d,%d,%d!\n"
	.size	.L.str.187, 23

	.type	.L.str.188,@object              // @.str.188
.L.str.188:
	.asciz	"[NE]no_cross_talk_rebuild_current_block fail 2\n"
	.size	.L.str.188, 48

	.type	.L.str.189,@object              // @.str.189
.L.str.189:
	.asciz	"[NI]nctrcb2 %d,%d,%d,%d,%d,%d!\n"
	.size	.L.str.189, 32

	.type	.L.str.190,@object              // @.str.190
.L.str.190:
	.asciz	"[NE]no_cross_talk_rebuild_current_block fail 4\n"
	.size	.L.str.190, 48

	.type	.L.str.191,@object              // @.str.191
.L.str.191:
	.asciz	"[NE]no_cross_talk_rebuild_current_block erase block fail %d!\n"
	.size	.L.str.191, 62

	.type	.L.str.192,@object              // @.str.192
.L.str.192:
	.asciz	"[NI]erase_block block %d!\n"
	.size	.L.str.192, 27

	.type	.L.str.193,@object              // @.str.193
.L.str.193:
	.asciz	"[NE]erase_block block fail %d!\n"
	.size	.L.str.193, 32

	.type	.L.str.194,@object              // @.str.194
.L.str.194:
	.asciz	"[NI]torture b@%d.\n"
	.size	.L.str.194, 19

	.type	patterns,@object                // @patterns
	.data
patterns:
	.byte	165
	.size	patterns, 1

	.type	.L.str.195,@object              // @.str.195
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.195:
	.asciz	"[NE]%s write b@%d p@%d err\n"
	.size	.L.str.195, 28

	.type	.L__func__.torture_block_and_mark,@object // @__func__.torture_block_and_mark
.L__func__.torture_block_and_mark:
	.asciz	"torture_block_and_mark"
	.size	.L__func__.torture_block_and_mark, 23

	.type	.L.str.196,@object              // @.str.196
.L.str.196:
	.asciz	"[NE]%s read b@%d p@%d err\n"
	.size	.L.str.196, 27

	.type	.L.str.197,@object              // @.str.197
.L.str.197:
	.asciz	"[NE]%s b@%d p@%d patt %02x checking main data fail\n"
	.size	.L.str.197, 52

	.type	.L.str.198,@object              // @.str.198
.L.str.198:
	.asciz	"[NE]%s b@%d p@%d patt %02x checking spare data fail\n"
	.size	.L.str.198, 53

	.type	.L.str.199,@object              // @.str.199
.L.str.199:
	.asciz	"[NE][NE]erase_block block fail %d!\n"
	.size	.L.str.199, 36

	.type	.L.str.200,@object              // @.str.200
.L.str.200:
	.asciz	"[NE]get_used_page_num ecc error block:%d page:%d!\n"
	.size	.L.str.200, 51

	.type	.L.str.201,@object              // @.str.201
.L.str.201:
	.asciz	"[NE]power off ecc error 4!!\n"
	.size	.L.str.201, 29

	.type	.L.str.202,@object              // @.str.202
.L.str.202:
	.asciz	"[NE]sb@%d sp@%d get_used_page_num used spare data %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n"
	.size	.L.str.202, 99

	.type	.L.str.203,@object              // @.str.203
.L.str.203:
	.asciz	"[NE]gupnnc ecc err b:%d p:%d!\n"
	.size	.L.str.203, 31

	.type	.L.str.204,@object              // @.str.204
.L.str.204:
	.asciz	"[NE]gupnnc power off ecc err!!\n"
	.size	.L.str.204, 32

	.type	.L.str.205,@object              // @.str.205
.L.str.205:
	.asciz	"[NE]cross talk block:%d total_good_num %d good_page_num: %d!\n"
	.size	.L.str.205, 62

	.type	.L.str.206,@object              // @.str.206
.L.str.206:
	.asciz	"[NE]current_block_ecc_error no free block!!!\n"
	.size	.L.str.206, 46

	.type	.L.str.207,@object              // @.str.207
.L.str.207:
	.asciz	"[NE]do current_block_ecc_error %d!!\n"
	.size	.L.str.207, 37

	.type	.L.str.208,@object              // @.str.208
.L.str.208:
	.asciz	"[NE]power off ecc error 2.\n"
	.size	.L.str.208, 28

	.type	.L.str.209,@object              // @.str.209
.L.str.209:
	.asciz	"[NE]current_block_ecc ok page %d %d!!\n"
	.size	.L.str.209, 39

	.type	.L.str.210,@object              // @.str.210
.L.str.210:
	.asciz	"[NE]current_block_ecc_error page %d!!\n"
	.size	.L.str.210, 39

	.type	.L.str.211,@object              // @.str.211
.L.str.211:
	.asciz	"[NE]current_block_ecc_error invalid page data %d!!\n"
	.size	.L.str.211, 52

	.type	.L.str.212,@object              // @.str.212
.L.str.212:
	.asciz	"[NE]current_block_ecc_error unkown page data %d!!\n"
	.size	.L.str.212, 51

	.type	.L.str.213,@object              // @.str.213
.L.str.213:
	.asciz	"[ND]do current_block_ecc_error end %d %d!!\n"
	.size	.L.str.213, 44

	.type	.L.str.214,@object              // @.str.214
.L.str.214:
	.asciz	"[NE]do block_last_page_ecc_error!!\n"
	.size	.L.str.214, 36

	.type	.L.str.215,@object              // @.str.215
.L.str.215:
	.asciz	"[NE]%s:malloc fail 1!\n"
	.size	.L.str.215, 23

	.type	.L__func__.block_last_page_ecc_error,@object // @__func__.block_last_page_ecc_error
.L__func__.block_last_page_ecc_error:
	.asciz	"block_last_page_ecc_error"
	.size	.L__func__.block_last_page_ecc_error, 26

	.type	.L.str.216,@object              // @.str.216
.L.str.216:
	.asciz	"[NE]%s:malloc fail 2!\n"
	.size	.L.str.216, 23

	.type	.L.str.217,@object              // @.str.217
.L.str.217:
	.asciz	"[NE]current_block_ecc_error invalid page data!!\n"
	.size	.L.str.217, 49

	.type	.L.str.218,@object              // @.str.218
.L.str.218:
	.asciz	"[NE]current_block_ecc_error unkown page data!!\n"
	.size	.L.str.218, 48

	.type	.L.str.219,@object              // @.str.219
.L.str.219:
	.asciz	"[NE]do block_last_page_ecc_error end!!\n"
	.size	.L.str.219, 40

	.type	.L.str.220,@object              // @.str.220
.L.str.220:
	.asciz	"[NE]adjust free list fail!!\n"
	.size	.L.str.220, 29

	.type	.L.str.221,@object              // @.str.221
.L.str.221:
	.asciz	"[NI]adjust free list : %d %d!!\n"
	.size	.L.str.221, 32

	.type	.L.str.222,@object              // @.str.222
.L.str.222:
	.asciz	"[ND]nand3.x \n"
	.size	.L.str.222, 14

	.type	.L.str.223,@object              // @.str.223
.L.str.223:
	.asciz	"[ND]zone->nand_chip->blk_per_chip: %d \n"
	.size	.L.str.223, 40

	.type	.L.str.224,@object              // @.str.224
.L.str.224:
	.asciz	"[ND]zone->nand_chip->bytes_per_page: %d \n"
	.size	.L.str.224, 42

	.type	.L.str.225,@object              // @.str.225
.L.str.225:
	.asciz	"[ND]zone->nand_chip->pages_per_blk: %d \n"
	.size	.L.str.225, 41

	.type	.L.str.226,@object              // @.str.226
.L.str.226:
	.asciz	"[ND]zone->nand_chip->max_erase_times: %d \n"
	.size	.L.str.226, 43

	.type	.L.str.227,@object              // @.str.227
.L.str.227:
	.asciz	"[ND]zone->nand_chip->support_read_reclaim: %d \n"
	.size	.L.str.227, 48

	.type	.L.str.228,@object              // @.str.228
.L.str.228:
	.asciz	"[ND]zone->test: %d \n"
	.size	.L.str.228, 21

	.type	.L.str.229,@object              // @.str.229
.L.str.229:
	.asciz	"[ND]zone->zone_no: %d \n"
	.size	.L.str.229, 24

	.type	.L.str.230,@object              // @.str.230
.L.str.230:
	.asciz	"[ND]zone->zone_attr: %d \n"
	.size	.L.str.230, 26

	.type	.L.str.231,@object              // @.str.231
.L.str.231:
	.asciz	"[ND]zone->blocks: %d \n"
	.size	.L.str.231, 23

	.type	.L.str.232,@object              // @.str.232
.L.str.232:
	.asciz	"[ND]zone->bad_block: %d \n"
	.size	.L.str.232, 26

	.type	.L.str.233,@object              // @.str.233
.L.str.233:
	.asciz	"[ND]zone->logic_cap_in_sects: %d \n"
	.size	.L.str.233, 35

	.type	.L.str.234,@object              // @.str.234
.L.str.234:
	.asciz	"[ND]zone->backup_cap_in_sects: %d \n"
	.size	.L.str.234, 36

	.type	.L.str.235,@object              // @.str.235
.L.str.235:
	.asciz	"[ND]zone->free_block_num: %d \n"
	.size	.L.str.235, 31

	.type	.L.str.236,@object              // @.str.236
.L.str.236:
	.asciz	"[ND]zone->gc_strategy.start_gc_free_blocks: %d \n"
	.size	.L.str.236, 49

	.type	.L.str.237,@object              // @.str.237
.L.str.237:
	.asciz	"[ND]zone->gc_strategy.stop_gc_free_blocks: %d \n"
	.size	.L.str.237, 48

	.type	.L.str.238,@object              // @.str.238
.L.str.238:
	.asciz	"[ND]zone->gc_strategy.gc_page: %d \n"
	.size	.L.str.238, 36

	.type	.L.str.239,@object              // @.str.239
.L.str.239:
	.asciz	"[ND]zone->gc_strategy.process: %d \n"
	.size	.L.str.239, 36

	.type	.L.str.240,@object              // @.str.240
.L.str.240:
	.asciz	"[ND]zone->prio_gc.prio_type : %d \n"
	.size	.L.str.240, 35

	.type	.L.str.241,@object              // @.str.241
.L.str.241:
	.asciz	"[ND]zone->zone_start_phy_block->phy_block.Block_NO: %d \n"
	.size	.L.str.241, 57

	.type	.L.str.242,@object              // @.str.242
.L.str.242:
	.asciz	"[ND]zone->zone_end_phy_block->phy_block.Block_NO: %d \n"
	.size	.L.str.242, 55

	.type	.L.str.243,@object              // @.str.243
.L.str.243:
	.asciz	"[ND]zone->zone_phy_page_map_for_gc: %x \n"
	.size	.L.str.243, 41

	.type	.L.str.244,@object              // @.str.244
.L.str.244:
	.asciz	"[ND]zone->current_block.user_info: %x \n"
	.size	.L.str.244, 40

	.type	.L.str.245,@object              // @.str.245
.L.str.245:
	.asciz	"[ND]zone->current_block.block_info: %x \n"
	.size	.L.str.245, 41

	.type	.L.str.246,@object              // @.str.246
.L.str.246:
	.asciz	"[ND]zone->current_block.block_info->phy_block.Block_NO: %d \n"
	.size	.L.str.246, 61

	.type	.L.str.247,@object              // @.str.247
.L.str.247:
	.asciz	"[ND]zone->current_block.page_used: %d \n"
	.size	.L.str.247, 40

	.type	.L.str.248,@object              // @.str.248
.L.str.248:
	.asciz	"[ND]zone->current_block.block_info->block_used_count: %d \n"
	.size	.L.str.248, 59

	.type	.L.str.249,@object              // @.str.249
.L.str.249:
	.asciz	"[ND]zone->recv_free_block: %d %d!!\n"
	.size	.L.str.249, 36

	.type	.L.str.250,@object              // @.str.250
.L.str.250:
	.asciz	"[ND]zone->assist_block.user_info: %x \n"
	.size	.L.str.250, 39

	.type	.L.str.251,@object              // @.str.251
.L.str.251:
	.asciz	"[ND]zone->assist_block.block_info: %x \n"
	.size	.L.str.251, 40

	.type	.L.str.252,@object              // @.str.252
.L.str.252:
	.asciz	"[ND]zone->assist_block.block_info->phy_block.Block_NO: %d \n"
	.size	.L.str.252, 60

	.type	.L.str.253,@object              // @.str.253
.L.str.253:
	.asciz	"[ND]zone->assist_block.page_used: %d \n"
	.size	.L.str.253, 39

	.type	.L.str.254,@object              // @.str.254
.L.str.254:
	.asciz	"[ND]zone->assist_block.block_info->block_used_count: %d \n"
	.size	.L.str.254, 58

	.type	.L.str.255,@object              // @.str.255
.L.str.255:
	.asciz	"[ND]zone->read_reclaim_complete: %x \n"
	.size	.L.str.255, 38

	.type	.L.str.256,@object              // @.str.256
.L.str.256:
	.asciz	"[ND]zone->temp_page_buf: %x \n"
	.size	.L.str.256, 30

	.type	.L.str.257,@object              // @.str.257
.L.str.257:
	.asciz	"[ND]zone->max_erase_num: %d \n"
	.size	.L.str.257, 30

	.type	.L.str.258,@object              // @.str.258
.L.str.258:
	.asciz	"[ND]zone->cache.cache_totals: %x \n"
	.size	.L.str.258, 35

	.type	.L.str.259,@object              // @.str.259
.L.str.259:
	.asciz	"[ND]zone->wl_gc_count: %d \n"
	.size	.L.str.259, 28

	.type	.L.str.260,@object              // @.str.260
.L.str.260:
	.asciz	"[ND]zone->wl_gc_page_num: %d \n"
	.size	.L.str.260, 31

	.type	.L.str.261,@object              // @.str.261
.L.str.261:
	.asciz	"[ND]zone->cfg->nftl_dont_use_cache: %x \n"
	.size	.L.str.261, 41

	.type	.L.str.262,@object              // @.str.262
.L.str.262:
	.asciz	"[ND]zone->cfg->nftl_use_cache_sort: %x \n"
	.size	.L.str.262, 41

	.type	.L.str.263,@object              // @.str.263
.L.str.263:
	.asciz	"[ND]zone->cfg->nftl_support_gc_read_reclaim: %x \n"
	.size	.L.str.263, 50

	.type	.L.str.264,@object              // @.str.264
.L.str.264:
	.asciz	"[ND]zone->cfg->nftl_support_wear_leveling: %x \n"
	.size	.L.str.264, 48

	.type	.L.str.265,@object              // @.str.265
.L.str.265:
	.asciz	"[ND]zone->cfg->nftl_need_erase: %x \n"
	.size	.L.str.265, 37

	.type	.L.str.266,@object              // @.str.266
.L.str.266:
	.asciz	"[ND]zone->cfg->nftl_min_free_block_num: %x \n"
	.size	.L.str.266, 45

	.type	.L.str.267,@object              // @.str.267
.L.str.267:
	.asciz	"[ND]zone->cfg->nftl_gc_threshold_free_block_num: %x \n"
	.size	.L.str.267, 54

	.type	.L.str.268,@object              // @.str.268
.L.str.268:
	.asciz	"[ND]zone->cfg->nftl_min_free_block: %x \n"
	.size	.L.str.268, 41

	.type	.L.str.269,@object              // @.str.269
.L.str.269:
	.asciz	"[ND]zone->cfg->nftl_gc_threshold_ratio_numerator: %x \n"
	.size	.L.str.269, 55

	.type	.L.str.270,@object              // @.str.270
.L.str.270:
	.asciz	"[ND]zone->cfg->nftl_gc_threshold_ratio_denominator: %x \n"
	.size	.L.str.270, 57

	.type	.L.str.271,@object              // @.str.271
.L.str.271:
	.asciz	"[ND]zone->cfg->nftl_max_cache_num: %d \n"
	.size	.L.str.271, 40

	.type	.L.str.272,@object              // @.str.272
.L.str.272:
	.asciz	"[ND]zone->cfg->nftl_max_cache_write_num: %d \n"
	.size	.L.str.272, 46

	.type	.L.str.273,@object              // @.str.273
.L.str.273:
	.asciz	"[ND]zone->cfg->nftl_cross_talk: %x \n"
	.size	.L.str.273, 37

	.type	.L.str.274,@object              // @.str.274
.L.str.274:
	.asciz	"[ND]zone->cfg->nftl_read_claim_interval: %d \n"
	.size	.L.str.274, 46

	.type	.L.str.275,@object              // @.str.275
.L.str.275:
	.asciz	"[NE]%s:malloc 4096 fail!\n"
	.size	.L.str.275, 26

	.type	.L__func__.nand_dbg_phy_read,@object // @__func__.nand_dbg_phy_read
.L__func__.nand_dbg_phy_read:
	.asciz	"nand_dbg_phy_read"
	.size	.L__func__.nand_dbg_phy_read, 18

	.type	.L.str.276,@object              // @.str.276
.L.str.276:
	.asciz	"[ND]spare: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x"
	.size	.L.str.276, 59

	.type	.L.str.277,@object              // @.str.277
.L.str.277:
	.asciz	"[ND]buf:%x %x %x %x %x %x %x %x %x %x %x.\n"
	.size	.L.str.277, 43

	.type	.L__func__.nand_dbg_zone_phy_read,@object // @__func__.nand_dbg_zone_phy_read
.L__func__.nand_dbg_zone_phy_read:
	.asciz	"nand_dbg_zone_phy_read"
	.size	.L__func__.nand_dbg_zone_phy_read, 23

	.type	.L.str.278,@object              // @.str.278
.L.str.278:
	.asciz	"[NE]%s:malloc 8192 fail!\n"
	.size	.L.str.278, 26

	.type	.L__func__.nand_dbg_zone_phy_write,@object // @__func__.nand_dbg_zone_phy_write
.L__func__.nand_dbg_zone_phy_write:
	.asciz	"nand_dbg_zone_phy_write"
	.size	.L__func__.nand_dbg_zone_phy_write, 24

	.type	.L__func__.nand_dbg_phy_write,@object // @__func__.nand_dbg_phy_write
.L__func__.nand_dbg_phy_write:
	.asciz	"nand_dbg_phy_write"
	.size	.L__func__.nand_dbg_phy_write, 19

	.type	.L.str.279,@object              // @.str.279
.L.str.279:
	.asciz	"[ND]zoneNO:%d total_pages: %d\n"
	.size	.L.str.279, 31

	.type	.L.str.280,@object              // @.str.280
.L.str.280:
	.asciz	"[ND]nand_memset bytes: %d\n"
	.size	.L.str.280, 27

	.type	.L.str.281,@object              // @.str.281
.L.str.281:
	.asciz	"[ND]build_zone_list start\n"
	.size	.L.str.281, 27

	.type	.L.str.282,@object              // @.str.282
.L.str.282:
	.asciz	"[ND]nftl_cache_init start\n"
	.size	.L.str.282, 27

	.type	.L.str.283,@object              // @.str.283
.L.str.283:
	.asciz	"[ND]=================================2=======================================\n"
	.size	.L.str.283, 79

	.type	.L.str.284,@object              // @.str.284
.L.str.284:
	.asciz	"[ND]nand_find_zone_table end\n"
	.size	.L.str.284, 30

	.type	.L.str.285,@object              // @.str.285
.L.str.285:
	.asciz	"[NE][NE]%s:malloc 512 fail!\n"
	.size	.L.str.285, 29

	.type	.L__func__.write_data_to_nand,@object // @__func__.write_data_to_nand
.L__func__.write_data_to_nand:
	.asciz	"write_data_to_nand"
	.size	.L__func__.write_data_to_nand, 19

	.type	.L.str.286,@object              // @.str.286
.L.str.286:
	.asciz	"[ND]==========================\n"
	.size	.L.str.286, 32

	.type	.L.str.287,@object              // @.str.287
.L.str.287:
	.asciz	"[NE]exception case bad block\n"
	.size	.L.str.287, 30

	.type	.L.str.288,@object              // @.str.288
.L.str.288:
	.asciz	"[NE]exception case wbc\n"
	.size	.L.str.288, 24

	.type	.L.str.289,@object              // @.str.289
.L.str.289:
	.asciz	"[NE]exception case gd\n"
	.size	.L.str.289, 23

	.type	.L.str.290,@object              // @.str.290
.L.str.290:
	.asciz	"[NE]%s %d, flag = %d\n"
	.size	.L.str.290, 22

	.type	.L__FUNCTION__.nand_power_off_ecc_error,@object // @__FUNCTION__.nand_power_off_ecc_error
.L__FUNCTION__.nand_power_off_ecc_error:
	.asciz	"nand_power_off_ecc_error"
	.size	.L__FUNCTION__.nand_power_off_ecc_error, 25

	.type	.L.str.291,@object              // @.str.291
.L.str.291:
	.asciz	"[NE]OOB :"
	.size	.L.str.291, 10

	.type	.L.str.292,@object              // @.str.292
.L.str.292:
	.asciz	"[NE]%s erase fb@%d fail\n"
	.size	.L.str.292, 25

	.type	.L__func__.clean_zone,@object   // @__func__.clean_zone
.L__func__.clean_zone:
	.asciz	"clean_zone"
	.size	.L__func__.clean_zone, 11

	.type	.L.str.293,@object              // @.str.293
.L.str.293:
	.asciz	"[NE]%s erase ub@%u fail\n"
	.size	.L.str.293, 25

	.type	.L.str.294,@object              // @.str.294
.L.str.294:
	.asciz	"[NE]not current block\n"
	.size	.L.str.294, 23

	.type	.L.str.295,@object              // @.str.295
.L.str.295:
	.asciz	"[NE]%s:malloc fail for cache_node!\n"
	.size	.L.str.295, 36

	.type	.L__func__.nftl_cache_init,@object // @__func__.nftl_cache_init
.L__func__.nftl_cache_init:
	.asciz	"nftl_cache_init"
	.size	.L__func__.nftl_cache_init, 16

	.type	.L.str.296,@object              // @.str.296
.L.str.296:
	.asciz	"[NE]%s:malloc fail for cache_page_buf!\n"
	.size	.L.str.296, 40

	.type	.L.str.297,@object              // @.str.297
.L.str.297:
	.asciz	"[ND]read sector:0x%x,len:0x%x\n"
	.size	.L.str.297, 31

	.type	.L.str.298,@object              // @.str.298
.L.str.298:
	.asciz	"[ND]write sector:0x%x,len:0x%x\n"
	.size	.L.str.298, 32

	.type	.L.str.299,@object              // @.str.299
.L.str.299:
	.asciz	"[ND]dicard sector:0x%x,len:0x%x\n"
	.size	.L.str.299, 33

	.type	.L.str.300,@object              // @.str.300
.L.str.300:
	.asciz	"[NE]paramter error. beyond reality capacity\n"
	.size	.L.str.300, 45

	.type	.L.str.301,@object              // @.str.301
.L.str.301:
	.asciz	"[NI]nand_cache_read error buf is  NULL!\n"
	.size	.L.str.301, 41

	.type	.L.str.302,@object              // @.str.302
.L.str.302:
	.asciz	"[NE]error1 node %d,%d,%d!\n"
	.size	.L.str.302, 27

	.type	.L.str.303,@object              // @.str.303
.L.str.303:
	.asciz	"[NE]error1 bitmap %x!\n"
	.size	.L.str.303, 23

	.type	.L.str.304,@object              // @.str.304
.L.str.304:
	.asciz	"[NE]ECC_LIMIT happened 12! page:%d!\n"
	.size	.L.str.304, 37

	.type	.L.str.305,@object              // @.str.305
.L.str.305:
	.asciz	"[NE]error1 flush write cache to nand\n"
	.size	.L.str.305, 38

	.type	.L.str.306,@object              // @.str.306
.L.str.306:
	.asciz	"[NE]error2 flush write cache to nand\n"
	.size	.L.str.306, 38

	.type	.L.str.307,@object              // @.str.307
.L.str.307:
	.asciz	"[NI]ECC_LIMIT happened 11! page:%d!\n"
	.size	.L.str.307, 37

	.type	.L__const.flush_write_cache_to_nand.l04a_3d_id,@object // @__const.flush_write_cache_to_nand.l04a_3d_id
	.section	.rodata.cst8,"aM",@progbits,8
.L__const.flush_write_cache_to_nand.l04a_3d_id:
	.ascii	",dD2\245\377\377\377"
	.size	.L__const.flush_write_cache_to_nand.l04a_3d_id, 8

	.type	.L__const.flush_write_cache_to_nand.l05b_3d_id,@object // @__const.flush_write_cache_to_nand.l05b_3d_id
.L__const.flush_write_cache_to_nand.l05b_3d_id:
	.asciz	",\204D2\252\004\000"
	.size	.L__const.flush_write_cache_to_nand.l05b_3d_id, 8

	.type	.L.str.308,@object              // @.str.308
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.308:
	.asciz	"[NE]flush write cache to nand no cache!\n"
	.size	.L.str.308, 41

	.type	.L.str.309,@object              // @.str.309
.L.str.309:
	.asciz	"[NE]ECC_LIMIT happened 10! page:%d cache: %d!\n"
	.size	.L.str.309, 47

	.type	.L.str.310,@object              // @.str.310
.L.str.310:
	.asciz	"[NE]flush write cache to nand error!\n"
	.size	.L.str.310, 38

	.type	.L.str.311,@object              // @.str.311
.L.str.311:
	.asciz	"[ND]flush cache %d %d!\n"
	.size	.L.str.311, 24

	.type	.L.str.312,@object              // @.str.312
.L.str.312:
	.asciz	"[NE]nand_discard paramter error!\n"
	.size	.L.str.312, 34

	.type	.L.str.313,@object              // @.str.313
.L.str.313:
	.asciz	"[NI]page_map NULL!!\n"
	.size	.L.str.313, 21

	.type	.L.str.314,@object              // @.str.314
.L.str.314:
	.asciz	"[ND]%4d:<%4d %3d><%d>\n"
	.size	.L.str.314, 23

	.type	.L.str.315,@object              // @.str.315
.L.str.315:
	.asciz	"[ND]smart->version:%x;\n"
	.size	.L.str.315, 24

	.type	.L.str.316,@object              // @.str.316
.L.str.316:
	.asciz	"[ND]smart->total_recv_read_sectors:%lld;\n"
	.size	.L.str.316, 42

	.type	.L.str.317,@object              // @.str.317
.L.str.317:
	.asciz	"[ND]smart->total_recv_write_sectors:%lld;\n"
	.size	.L.str.317, 43

	.type	.L.str.318,@object              // @.str.318
.L.str.318:
	.asciz	"[ND]smart->total_real_read_pages:%lld;\n"
	.size	.L.str.318, 40

	.type	.L.str.319,@object              // @.str.319
.L.str.319:
	.asciz	"[ND]smart->total_real_write_pages:%lld;\n"
	.size	.L.str.319, 41

	.type	.L.str.320,@object              // @.str.320
.L.str.320:
	.asciz	"[ND]smart->total_recv_discard_sectors:%lld;\n"
	.size	.L.str.320, 45

	.type	.L.str.321,@object              // @.str.321
.L.str.321:
	.asciz	"[ND]smart->total_real_discard_sectors:%lld;\n"
	.size	.L.str.321, 45

	.type	.L.str.322,@object              // @.str.322
.L.str.322:
	.asciz	"[ND]smart->total_recv_read_claim_pages:%d;\n"
	.size	.L.str.322, 44

	.type	.L.str.323,@object              // @.str.323
.L.str.323:
	.asciz	"[ND]smart->total_gc_times:%d;\n"
	.size	.L.str.323, 31

	.type	.L.str.324,@object              // @.str.324
.L.str.324:
	.asciz	"[ND]smart->total_gc_pages:%d;\n"
	.size	.L.str.324, 31

	.type	.L.str.325,@object              // @.str.325
.L.str.325:
	.asciz	"[ND]smart->total_wl_times:%d;\n"
	.size	.L.str.325, 31

	.type	.L.str.326,@object              // @.str.326
.L.str.326:
	.asciz	"[ND]smart->total_wl_gc_times:%d;\n"
	.size	.L.str.326, 34

	.type	.L.str.327,@object              // @.str.327
.L.str.327:
	.asciz	"[ND]smart->total_real_read_error_pages:%d;\n"
	.size	.L.str.327, 44

	.type	.L.str.328,@object              // @.str.328
.L.str.328:
	.asciz	"[ND]smart->total_real_write_error_pages:%d;\n"
	.size	.L.str.328, 45

	.type	.L.str.329,@object              // @.str.329
.L.str.329:
	.asciz	"[ND]smart->total_normal_power_cycles:%x;\n"
	.size	.L.str.329, 42

	.type	.L.str.330,@object              // @.str.330
.L.str.330:
	.asciz	"[ND]smart->total_unusual_power_cycles:%x;\n"
	.size	.L.str.330, 43

	.type	.L.str.331,@object              // @.str.331
.L.str.331:
	.asciz	"[ND]smart->max_block_erase_times:%d;\n"
	.size	.L.str.331, 38

	.type	.L.str.332,@object              // @.str.332
.L.str.332:
	.asciz	"[ND]smart->min_block_erase_times:%d;\n"
	.size	.L.str.332, 38

	.type	.L.str.333,@object              // @.str.333
.L.str.333:
	.asciz	"[ND]smart->max_block_used_counter:%d;\n"
	.size	.L.str.333, 39

	.type	.L.str.334,@object              // @.str.334
.L.str.334:
	.asciz	"[ND]smart->min_block_used_counter:%d;\n"
	.size	.L.str.334, 39

	.type	.L.str.335,@object              // @.str.335
.L.str.335:
	.asciz	"[ND]smart->read_reclaim_utc:%d;\n"
	.size	.L.str.335, 33

	.type	.L.str.336,@object              // @.str.336
.L.str.336:
	.asciz	"[ND]smart->cross_talk_times:%d;\n"
	.size	.L.str.336, 33

	.type	.L.str.337,@object              // @.str.337
.L.str.337:
	.asciz	"[ND]smart->read_reclaim_zone_no:%d;\n"
	.size	.L.str.337, 37

	.type	.L.str.338,@object              // @.str.338
.L.str.338:
	.asciz	"[ND]smart->read_reclaim_page_no:%d;\n"
	.size	.L.str.338, 37

	.type	.L.str.339,@object              // @.str.339
.L.str.339:
	.asciz	"[NE]%s: malloc fail for zone\n"
	.size	.L.str.339, 30

	.type	.L__func__.nftl_start,@object   // @__func__.nftl_start
.L__func__.nftl_start:
	.asciz	"nftl_start"
	.size	.L__func__.nftl_start, 11

	.type	.L.str.340,@object              // @.str.340
.L.str.340:
	.asciz	"[NE]%s: malloc fail for nand_chip\n"
	.size	.L.str.340, 35

	.type	.L.str.341,@object              // @.str.341
.L.str.341:
	.asciz	"[NE]%s: malloc fail for nand_block_info\n"
	.size	.L.str.341, 41

	.type	.L.str.342,@object              // @.str.342
.L.str.342:
	.asciz	"[NE]not enough blocks %d!\n"
	.size	.L.str.342, 27

	.type	.L.str.343,@object              // @.str.343
.L.str.343:
	.asciz	"[ND]nftl start:%d,%d\n"
	.size	.L.str.343, 22

	.type	.L.str.344,@object              // @.str.344
.L.str.344:
	.asciz	"[NE]zone_param_init fail!\n"
	.size	.L.str.344, 27

	.type	.L.str.345,@object              // @.str.345
.L.str.345:
	.asciz	"[NE]build_zone_list_first fail!\n"
	.size	.L.str.345, 33

	.type	.L.str.346,@object              // @.str.346
.L.str.346:
	.asciz	"[NE]build_zone_list fail!\n"
	.size	.L.str.346, 27

	.type	.L.str.347,@object              // @.str.347
.L.str.347:
	.asciz	"[NI]try again\n"
	.size	.L.str.347, 15

	.type	nand_on_panic,@object           // @nand_on_panic
	.local	nand_on_panic
	.comm	nand_on_panic,4,4
	.type	panic_enable,@object            // @panic_enable
	.local	panic_enable
	.comm	panic_enable,4,4
	.type	panic_part,@object              // @panic_part
	.local	panic_part
	.comm	panic_part,16,8
	.type	.L.str.348,@object              // @.str.348
.L.str.348:
	.asciz	"[ND] panic part %s off %u size %u\n"
	.size	.L.str.348, 35

	.type	panic_info,@object              // @panic_info
	.local	panic_info
	.comm	panic_info,40,8
	.type	.L.str.349,@object              // @.str.349
.L.str.349:
	.asciz	"[ND] reserved panic block before %d!\n"
	.size	.L.str.349, 38

	.type	.L.str.350,@object              // @.str.350
.L.str.350:
	.asciz	"[NE] reserved panic block %d!\n"
	.size	.L.str.350, 31

	.type	.L.str.351,@object              // @.str.351
.L.str.351:
	.asciz	"[NE] no reserved panic block!!! quit!\n"
	.size	.L.str.351, 39

	.type	.L.str.352,@object              // @.str.352
.L.str.352:
	.asciz	"[ND] write panic data : logic addr: %d, sectors %d, pages %d, blocks %d\n"
	.size	.L.str.352, 73

	.type	.L.str.353,@object              // @.str.353
.L.str.353:
	.asciz	"[NE] too much data for panic-write, %d sectors left over\n"
	.size	.L.str.353, 58

	.type	.L.str.354,@object              // @.str.354
.L.str.354:
	.asciz	"[ND] write panic data block: %d page %d !\n"
	.size	.L.str.354, 43

	.type	.L.str.355,@object              // @.str.355
.L.str.355:
	.asciz	"[ND] write dummy data block: %d page %d !\n"
	.size	.L.str.355, 43

	.type	.L.str.356,@object              // @.str.356
.L.str.356:
	.asciz	"[ND] find panic block : %d\n"
	.size	.L.str.356, 28

	.type	.L.str.357,@object              // @.str.357
.L.str.357:
	.asciz	"[NE] invalid panic block index %d\n"
	.size	.L.str.357, 35

	.type	.L.str.358,@object              // @.str.358
.L.str.358:
	.asciz	"[ND] not find panic data, no need to recovery!\n"
	.size	.L.str.358, 48

	.type	.L.str.359,@object              // @.str.359
.L.str.359:
	.asciz	"[ND] recovery panic data start!\n"
	.size	.L.str.359, 33

	.type	.L.str.360,@object              // @.str.360
.L.str.360:
	.asciz	"[NE] %s(%d): malloc failed\n"
	.size	.L.str.360, 28

	.type	.L__func__.panic_recover_data,@object // @__func__.panic_recover_data
.L__func__.panic_recover_data:
	.asciz	"panic_recover_data"
	.size	.L__func__.panic_recover_data, 19

	.type	.L.str.361,@object              // @.str.361
.L.str.361:
	.asciz	"[ND] ignore blk %d page %d as dummy page\n"
	.size	.L.str.361, 42

	.type	.L.str.362,@object              // @.str.362
.L.str.362:
	.asciz	"[ND] exit recovery panic block\n"
	.size	.L.str.362, 32

	.type	.L.str.363,@object              // @.str.363
.L.str.363:
	.asciz	"[ND] invalid blk %d page %d, finish recovery\n"
	.size	.L.str.363, 46

	.type	.L.str.364,@object              // @.str.364
.L.str.364:
	.asciz	"[ND] move blk %d page %d to logic addr %d\n"
	.size	.L.str.364, 43

	.type	nftl_blk_head,@object           // @nftl_blk_head
	.bss
	.globl	nftl_blk_head
	.p2align	3
nftl_blk_head:
	.zero	136
	.size	nftl_blk_head, 136

	.type	.L.str.365,@object              // @.str.365
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.365:
	.asciz	"[NE]nftl_build_all error!\n"
	.size	.L.str.365, 27

	.type	.L.str.366,@object              // @.str.366
.L.str.366:
	.asciz	"[NE]%s:malloc fail for nftl_blk\n"
	.size	.L.str.366, 33

	.type	.L__func__.nftl_add,@object     // @__func__.nftl_add
.L__func__.nftl_add:
	.asciz	"nftl_add"
	.size	.L__func__.nftl_add, 9

	.type	.L.str.367,@object              // @.str.367
.L.str.367:
	.asciz	"[NE]nftl_initialize failed\n"
	.size	.L.str.367, 28

	.type	.L.str.368,@object              // @.str.368
.L.str.368:
	.asciz	"[ND]nftl_add ok\n"
	.size	.L.str.368, 17

	.type	.L.str.369,@object              // @.str.369
.L.str.369:
	.asciz	"[ND]cap: 0x%08x.\n"
	.size	.L.str.369, 18

	.type	.L.str.370,@object              // @.str.370
.L.str.370:
	.asciz	"[NE]parameter error %d,%d !\n"
	.size	.L.str.370, 29

	.type	.L.str.371,@object              // @.str.371
.L.str.371:
	.asciz	"[NE]malloc size too large %d!\n"
	.size	.L.str.371, 31

	.type	.L.str.372,@object              // @.str.372
.L.str.372:
	.asciz	"5"
	.size	.L.str.372, 2

	.type	.L.str.373,@object              // @.str.373
.L.str.373:
	.asciz	"0"
	.size	.L.str.373, 2

	.type	.L.str.374,@object              // @.str.374
.L.str.374:
	.asciz	"005"
	.size	.L.str.374, 4

	.type	.L.str.375,@object              // @.str.375
.L.str.375:
	.asciz	"2022.03.23"
	.size	.L.str.375, 11

	.type	.L.str.376,@object              // @.str.376
.L.str.376:
	.asciz	"10:43"
	.size	.L.str.376, 6

	.type	.L__const.nftl_initialize.manu,@object // @__const.nftl_initialize.manu
	.section	.rodata,"a",@progbits
.L__const.nftl_initialize.manu:
	.ascii	"allwinner"
	.size	.L__const.nftl_initialize.manu, 9

	.type	.L__const.nftl_initialize.of_root,@object // @__const.nftl_initialize.of_root
	.section	.rodata.str1.1,"aMS",@progbits,1
.L__const.nftl_initialize.of_root:
	.asciz	"/"
	.size	.L__const.nftl_initialize.of_root, 2

	.type	.L.str.377,@object              // @.str.377
.L.str.377:
	.asciz	"[NE]%s:malloc fail for nftl_cfg\n"
	.size	.L.str.377, 33

	.type	.L__func__.nftl_initialize,@object // @__func__.nftl_initialize
.L__func__.nftl_initialize:
	.asciz	"nftl_initialize"
	.size	.L__func__.nftl_initialize, 16

	.type	.L.str.378,@object              // @.str.378
.L.str.378:
	.asciz	"[ND]max_erase_times = %i\n"
	.size	.L.str.378, 26

	.type	hynix26nm_read_retry_mode,@object // @hynix26nm_read_retry_mode
	.bss
	.globl	hynix26nm_read_retry_mode
hynix26nm_read_retry_mode:
	.byte	0                               // 0x0
	.size	hynix26nm_read_retry_mode, 1

	.type	hynix26nm_read_retry_cycle,@object // @hynix26nm_read_retry_cycle
	.globl	hynix26nm_read_retry_cycle
hynix26nm_read_retry_cycle:
	.byte	0                               // 0x0
	.size	hynix26nm_read_retry_cycle, 1

	.type	hynix26nm_read_retry_reg_num,@object // @hynix26nm_read_retry_reg_num
	.globl	hynix26nm_read_retry_reg_num
hynix26nm_read_retry_reg_num:
	.byte	0                               // 0x0
	.size	hynix26nm_read_retry_reg_num, 1

	.type	hynix26nm_read_retry_reg_adr,@object // @hynix26nm_read_retry_reg_adr
	.globl	hynix26nm_read_retry_reg_adr
hynix26nm_read_retry_reg_adr:
	.zero	4
	.size	hynix26nm_read_retry_reg_adr, 4

	.type	hynix26nm_read_retry_val,@object // @hynix26nm_read_retry_val
	.globl	hynix26nm_read_retry_val
	.p2align	1
hynix26nm_read_retry_val:
	.zero	56
	.size	hynix26nm_read_retry_val, 56

	.type	hynix26nm_lsb_mode_reg_adr,@object // @hynix26nm_lsb_mode_reg_adr
	.globl	hynix26nm_lsb_mode_reg_adr
hynix26nm_lsb_mode_reg_adr:
	.zero	5
	.size	hynix26nm_lsb_mode_reg_adr, 5

	.type	hynix26nm_lsb_mode_default_val,@object // @hynix26nm_lsb_mode_default_val
	.globl	hynix26nm_lsb_mode_default_val
hynix26nm_lsb_mode_default_val:
	.zero	5
	.size	hynix26nm_lsb_mode_default_val, 5

	.type	hynix26nm_lsb_mode_val,@object  // @hynix26nm_lsb_mode_val
	.globl	hynix26nm_lsb_mode_val
hynix26nm_lsb_mode_val:
	.zero	5
	.size	hynix26nm_lsb_mode_val, 5

	.type	hynix26nm_lsb_mode_reg_num,@object // @hynix26nm_lsb_mode_reg_num
	.globl	hynix26nm_lsb_mode_reg_num
hynix26nm_lsb_mode_reg_num:
	.byte	0                               // 0x0
	.size	hynix26nm_lsb_mode_reg_num, 1

	.type	para0,@object                   // @para0
	.section	.rodata,"a",@progbits
	.globl	para0
	.p2align	1
para0:
	.zero	8
	.hword	0                               // 0x0
	.hword	6                               // 0x6
	.hword	10                              // 0xa
	.hword	6                               // 0x6
	.hword	0                               // 0x0
	.hword	65533                           // 0xfffd
	.hword	65529                           // 0xfff9
	.hword	65528                           // 0xfff8
	.hword	0                               // 0x0
	.hword	65530                           // 0xfffa
	.hword	65523                           // 0xfff3
	.hword	65521                           // 0xfff1
	.hword	0                               // 0x0
	.hword	65525                           // 0xfff5
	.hword	65516                           // 0xffec
	.hword	65513                           // 0xffe9
	.hword	0                               // 0x0
	.hword	0                               // 0x0
	.hword	65510                           // 0xffe6
	.hword	65506                           // 0xffe2
	.hword	0                               // 0x0
	.hword	0                               // 0x0
	.hword	65504                           // 0xffe0
	.hword	65499                           // 0xffdb
	.size	para0, 56

	.type	para1,@object                   // @para1
	.globl	para1
	.p2align	1
para1:
	.zero	8
	.hword	0                               // 0x0
	.hword	6                               // 0x6
	.hword	10                              // 0xa
	.hword	6                               // 0x6
	.hword	0                               // 0x0
	.hword	65533                           // 0xfffd
	.hword	65529                           // 0xfff9
	.hword	65528                           // 0xfff8
	.hword	0                               // 0x0
	.hword	65530                           // 0xfffa
	.hword	65523                           // 0xfff3
	.hword	65521                           // 0xfff1
	.hword	0                               // 0x0
	.hword	65527                           // 0xfff7
	.hword	65516                           // 0xffec
	.hword	65513                           // 0xffe9
	.hword	0                               // 0x0
	.hword	0                               // 0x0
	.hword	65510                           // 0xffe6
	.hword	65506                           // 0xffe2
	.hword	0                               // 0x0
	.hword	0                               // 0x0
	.hword	65504                           // 0xffe0
	.hword	65499                           // 0xffdb
	.size	para1, 56

	.type	.L.str.379,@object              // @.str.379
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.379:
	.asciz	"rr value %x %x %x %x!\n"
	.size	.L.str.379, 23

	.type	.L.str.380,@object              // @.str.380
.L.str.380:
	.asciz	"hynix26nm retry param:"
	.size	.L.str.380, 23

	.type	.L.str.381,@object              // @.str.381
.L.str.381:
	.asciz	" %x "
	.size	.L.str.381, 5

	.type	.L.str.382,@object              // @.str.382
.L.str.382:
	.asciz	"m0 retry!\n"
	.size	.L.str.382, 11

	.type	.L.str.383,@object              // @.str.383
.L.str.383:
	.asciz	"hynix26nm ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.383, 85

	.type	.L.str.384,@object              // @.str.384
.L.str.384:
	.asciz	"hynix26nm_lsb_exit\n"
	.size	.L.str.384, 20

	.type	.L.str.385,@object              // @.str.385
.L.str.385:
	.asciz	"set retry default value: "
	.size	.L.str.385, 26

	.type	.L.str.386,@object              // @.str.386
.L.str.386:
	.asciz	" %x"
	.size	.L.str.386, 4

	.type	.L.str.387,@object              // @.str.387
.L.str.387:
	.asciz	"hynix26nm_get_default_param: ch: %d, chip: %d, value: 0x%x 0x%x 0x%x 0x%x \n"
	.size	.L.str.387, 76

	.type	.L.str.388,@object              // @.str.388
.L.str.388:
	.asciz	"NFC_ReadRetryInit, unknown read retry mode 0x%x\n"
	.size	.L.str.388, 49

	.type	.L.str.389,@object              // @.str.389
.L.str.389:
	.asciz	"hynix26nm_readretry_exit. \n"
	.size	.L.str.389, 28

	.type	.L.str.390,@object              // @.str.390
.L.str.390:
	.asciz	" hynix26nm_special_init hynix26nm_read_retry_mode: %d hynix26nm_read_retry_cycle:%d hynix26nm_read_retry_reg_num:%d\n"
	.size	.L.str.390, 117

	.type	.L.str.391,@object              // @.str.391
.L.str.391:
	.asciz	" hynix26nm_special_init error hynix26nm_read_retry_mode: %d hynix26nm_read_retry_cycle:%d hynix26nm_read_retry_reg_num:%d\n"
	.size	.L.str.391, 123

	.type	.L.str.392,@object              // @.str.392
.L.str.392:
	.asciz	" hynix26nm_special_exit \n"
	.size	.L.str.392, 26

	.type	samsung_read_retry_mode,@object // @samsung_read_retry_mode
	.bss
	.globl	samsung_read_retry_mode
samsung_read_retry_mode:
	.byte	0                               // 0x0
	.size	samsung_read_retry_mode, 1

	.type	samsung_read_retry_cycle,@object // @samsung_read_retry_cycle
	.globl	samsung_read_retry_cycle
samsung_read_retry_cycle:
	.byte	0                               // 0x0
	.size	samsung_read_retry_cycle, 1

	.type	samsung_read_retry_addr,@object // @samsung_read_retry_addr
	.data
	.globl	samsung_read_retry_addr
samsung_read_retry_addr:
	.ascii	"\247\244\245\246"
	.size	samsung_read_retry_addr, 4

	.type	samsung_read_retry_cmd,@object  // @samsung_read_retry_cmd
	.globl	samsung_read_retry_cmd
samsung_read_retry_cmd:
	.zero	4,161
	.size	samsung_read_retry_cmd, 4

	.type	samsung_p1,@object              // @samsung_p1
	.globl	samsung_p1
samsung_p1:
	.zero	4
	.asciz	"\005\n\000"
	.ascii	"(\000\354\330"
	.ascii	"\355\365\355\346"
	.asciz	"\n\017\005"
	.ascii	"\017\n\373\354"
	.ascii	"\350\357\350\334"
	.ascii	"\361\373\376\360"
	.ascii	"\n\000\373\354"
	.ascii	"\320\342\320\302"
	.ascii	"\024\017\373\354"
	.ascii	"\350\373\350\334"
	.ascii	"\036\024\373\354"
	.ascii	"\373\377\373\370"
	.asciz	"\007\f\002"
	.size	samsung_p1, 60

	.type	.L.str.393,@object              // @.str.393
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.393:
	.asciz	"m5 set readretry error ! %x \n"
	.size	.L.str.393, 30

	.type	.L.str.394,@object              // @.str.394
.L.str.394:
	.asciz	"samsung set readretry error ! %x \n"
	.size	.L.str.394, 35

	.type	.L.str.395,@object              // @.str.395
.L.str.395:
	.asciz	"samsung retry!\n"
	.size	.L.str.395, 16

	.type	.L.str.396,@object              // @.str.396
.L.str.396:
	.asciz	"samsung ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.396, 83

	.type	.L.str.397,@object              // @.str.397
.L.str.397:
	.asciz	"samsung read retry conut: %d !\n"
	.size	.L.str.397, 32

	.type	.L.str.398,@object              // @.str.398
.L.str.398:
	.asciz	"%s feature set fail\n"
	.size	.L.str.398, 21

	.type	.L__func__.samsung_set_readretry_K9GCGD8U0F,@object // @__func__.samsung_set_readretry_K9GCGD8U0F
.L__func__.samsung_set_readretry_K9GCGD8U0F:
	.asciz	"samsung_set_readretry_K9GCGD8U0F"
	.size	.L__func__.samsung_set_readretry_K9GCGD8U0F, 33

	.type	.L.str.399,@object              // @.str.399
.L.str.399:
	.asciz	" samsung_special_init samsung_read_retry_mode:%d samsung_read_retry_cycle :%d \n"
	.size	.L.str.399, 80

	.type	.L.str.400,@object              // @.str.400
.L.str.400:
	.asciz	" samsung_special_init error samsung_read_retry_mode:%d samsung_read_retry_cycle :%d \n"
	.size	.L.str.400, 86

	.type	.L.str.401,@object              // @.str.401
.L.str.401:
	.asciz	" samsung_special_exit \n"
	.size	.L.str.401, 24

	.type	sandisk_read_retry_mode,@object // @sandisk_read_retry_mode
	.bss
	.globl	sandisk_read_retry_mode
sandisk_read_retry_mode:
	.byte	0                               // 0x0
	.size	sandisk_read_retry_mode, 1

	.type	sandisk_read_retry_cycle,@object // @sandisk_read_retry_cycle
	.globl	sandisk_read_retry_cycle
sandisk_read_retry_cycle:
	.byte	0                               // 0x0
	.size	sandisk_read_retry_cycle, 1

	.type	sandisk_read_retry_reg_cnt,@object // @sandisk_read_retry_reg_cnt
	.globl	sandisk_read_retry_reg_cnt
sandisk_read_retry_reg_cnt:
	.byte	0                               // 0x0
	.size	sandisk_read_retry_reg_cnt, 1

	.type	sandisk_acti_start_cmd,@object  // @sandisk_acti_start_cmd
	.data
	.globl	sandisk_acti_start_cmd
sandisk_acti_start_cmd:
	.ascii	";\271"
	.size	sandisk_acti_start_cmd, 2

	.type	sandisk_acti_start_addr,@object // @sandisk_acti_start_addr
	.globl	sandisk_acti_start_addr
sandisk_acti_start_addr:
	.ascii	"\004\005\006\007\b\t\n\013\f"
	.size	sandisk_acti_start_addr, 9

	.type	sandisk_read_retry_enable_cmd_19,@object // @sandisk_read_retry_enable_cmd_19
	.globl	sandisk_read_retry_enable_cmd_19
sandisk_read_retry_enable_cmd_19:
	.byte	182
	.size	sandisk_read_retry_enable_cmd_19, 1

	.type	sandisk_read_retry_disable_cmd_19,@object // @sandisk_read_retry_disable_cmd_19
	.globl	sandisk_read_retry_disable_cmd_19
sandisk_read_retry_disable_cmd_19:
	.byte	214
	.size	sandisk_read_retry_disable_cmd_19, 1

	.type	sandisk_read_retry_cmd_19,@object // @sandisk_read_retry_cmd_19
	.globl	sandisk_read_retry_cmd_19
sandisk_read_retry_cmd_19:
	.zero	3,83
	.size	sandisk_read_retry_cmd_19, 3

	.type	sandisk_read_retry_reg_adr_19,@object // @sandisk_read_retry_reg_adr_19
	.globl	sandisk_read_retry_reg_adr_19
sandisk_read_retry_reg_adr_19:
	.asciz	"\004\005\007\000\000\000\000\000"
	.size	sandisk_read_retry_reg_adr_19, 9

	.type	sandisk_19_low,@object          // @sandisk_19_low
	.globl	sandisk_19_low
sandisk_19_low:
	.ascii	"\360\000\360"
	.ascii	"\340\000\340"
	.ascii	"\320\000\320"
	.ascii	"\020\000\020"
	.ascii	" \000 "
	.ascii	"0\0000"
	.ascii	"\300\000\320"
	.ascii	"\000\000\020"
	.ascii	"\000\000 "
	.ascii	"\020\000 "
	.ascii	"\260\000\320"
	.ascii	"\240\000\320"
	.ascii	"\220\000\320"
	.ascii	"\260\000\300"
	.ascii	"\240\000\300"
	.ascii	"\220\000\300"
	.size	sandisk_19_low, 48

	.type	sandisk_19_high,@object         // @sandisk_19_high
	.globl	sandisk_19_high
sandisk_19_high:
	.asciz	"\000\360"
	.asciz	"\017\340"
	.asciz	"\017\320"
	.asciz	"\016\340"
	.asciz	"\016\320"
	.asciz	"\r\360"
	.asciz	"\r\340"
	.asciz	"\r\320"
	.asciz	"\001\020"
	.asciz	"\002 "
	.asciz	"\002\020"
	.asciz	"\003 "
	.asciz	"\017\000"
	.asciz	"\016\360"
	.asciz	"\r\300"
	.asciz	"\017\360"
	.asciz	"\001\000"
	.asciz	"\002\000"
	.asciz	"\r\260"
	.asciz	"\f\240"
	.size	sandisk_19_high, 60

	.type	sandisk_read_retry_cmd_24,@object // @sandisk_read_retry_cmd_24
	.globl	sandisk_read_retry_cmd_24
sandisk_read_retry_cmd_24:
	.zero	3,83
	.size	sandisk_read_retry_cmd_24, 3

	.type	sandisk_read_retry_reg_adr_24,@object // @sandisk_read_retry_reg_adr_24
	.globl	sandisk_read_retry_reg_adr_24
sandisk_read_retry_reg_adr_24:
	.asciz	"\004\005\007\000\000\000\000\000"
	.size	sandisk_read_retry_reg_adr_24, 9

	.type	sandisk_24,@object              // @sandisk_24
	.globl	sandisk_24
sandisk_24:
	.asciz	"\000\360"
	.asciz	"\000\340"
	.ascii	"\377\360\360"
	.ascii	"\356\340\340"
	.ascii	"\336\320\320"
	.ascii	"\315\300\300"
	.asciz	"\001\000"
	.asciz	"\002\000"
	.asciz	"\003\000"
	.size	sandisk_24, 27

	.type	sandisk_read_retry_enable_cmd_1y,@object // @sandisk_read_retry_enable_cmd_1y
	.globl	sandisk_read_retry_enable_cmd_1y
sandisk_read_retry_enable_cmd_1y:
	.byte	93
	.size	sandisk_read_retry_enable_cmd_1y, 1

	.type	sandisk_read_retry_exit_cmd_1y,@object // @sandisk_read_retry_exit_cmd_1y
	.globl	sandisk_read_retry_exit_cmd_1y
sandisk_read_retry_exit_cmd_1y:
	.byte	255
	.size	sandisk_read_retry_exit_cmd_1y, 1

	.type	sandisk_read_retry_cmd_1y,@object // @sandisk_read_retry_cmd_1y
	.globl	sandisk_read_retry_cmd_1y
sandisk_read_retry_cmd_1y:
	.byte	239
	.size	sandisk_read_retry_cmd_1y, 1

	.type	sandisk_read_retry_reg_adr_1y,@object // @sandisk_read_retry_reg_adr_1y
	.globl	sandisk_read_retry_reg_adr_1y
sandisk_read_retry_reg_adr_1y:
	.byte	17
	.size	sandisk_read_retry_reg_adr_1y, 1

	.type	sandisk_1y,@object              // @sandisk_1y
	.globl	sandisk_1y
sandisk_1y:
	.ascii	"\004\000||"
	.ascii	"\b\000\004\004"
	.ascii	"||\000|"
	.asciz	"||\000"
	.ascii	"\f\000xx"
	.ascii	"\020\000\b\b"
	.ascii	"||xx"
	.ascii	"||\004\004"
	.ascii	"||xt"
	.asciz	"\024\004\004"
	.ascii	"xx\000|"
	.zero	4,120
	.ascii	"xxxt"
	.asciz	"xx\004"
	.ascii	"xtxt"
	.ascii	"xttp"
	.ascii	"xtxx"
	.ascii	"xpxt"
	.ascii	"xppp"
	.size	sandisk_1y, 76

	.type	sandisk_sclk0_bak,@object       // @sandisk_sclk0_bak
	.local	sandisk_sclk0_bak
	.comm	sandisk_sclk0_bak,4,4
	.type	sandisk_sclk1_bak,@object       // @sandisk_sclk1_bak
	.local	sandisk_sclk1_bak
	.comm	sandisk_sclk1_bak,4,4
	.type	sandisk_toggle_mode_flag,@object // @sandisk_toggle_mode_flag
	.local	sandisk_toggle_mode_flag
	.comm	sandisk_toggle_mode_flag,4,4
	.type	.L.str.402,@object              // @.str.402
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.402:
	.asciz	"sandisk retry!\n"
	.size	.L.str.402, 16

	.type	.L.str.403,@object              // @.str.403
.L.str.403:
	.asciz	"retry spare all 0xff! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.403, 83

	.type	.L.str.404,@object              // @.str.404
.L.str.404:
	.asciz	"sandisk ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.404, 83

	.type	.L__const.sandisk_activation_seq.cmd,@object // @__const.sandisk_activation_seq.cmd
	.section	.rodata,"a",@progbits
.L__const.sandisk_activation_seq.cmd:
	.zero	9,83
	.size	.L__const.sandisk_activation_seq.cmd, 9

	.type	.L.str.405,@object              // @.str.405
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.405:
	.asciz	"sandisk activation seq fail 0x%x\n"
	.size	.L.str.405, 34

	.type	.L.str.406,@object              // @.str.406
.L.str.406:
	.asciz	"sandisk read retry conut: %d !\n"
	.size	.L.str.406, 32

	.type	.L.str.407,@object              // @.str.407
.L.str.407:
	.asciz	" sandisk_special_init sandisk_read_retry_mode:%d sandisk_read_retry_cycle :%d sandisk_read_retry_reg_cnt %d \n"
	.size	.L.str.407, 110

	.type	.L.str.408,@object              // @.str.408
.L.str.408:
	.asciz	" sandisk_special_init error sandisk_read_retry_mode:%d sandisk_read_retry_cycle :%d sandisk_read_retry_reg_cnt %d \n"
	.size	.L.str.408, 116

	.type	.L.str.409,@object              // @.str.409
.L.str.409:
	.asciz	" sandisk_special_exit \n"
	.size	.L.str.409, 24

	.type	.L.str.410,@object              // @.str.410
.L.str.410:
	.asciz	" generic special init \n"
	.size	.L.str.410, 24

	.type	.L.str.411,@object              // @.str.411
.L.str.411:
	.asciz	" generic special exit \n"
	.size	.L.str.411, 24

	.type	sandisk_A19_read_retry_mode,@object // @sandisk_A19_read_retry_mode
	.bss
	.globl	sandisk_A19_read_retry_mode
sandisk_A19_read_retry_mode:
	.byte	0                               // 0x0
	.size	sandisk_A19_read_retry_mode, 1

	.type	sandisk_A19_read_retry_cycle,@object // @sandisk_A19_read_retry_cycle
	.globl	sandisk_A19_read_retry_cycle
sandisk_A19_read_retry_cycle:
	.byte	0                               // 0x0
	.size	sandisk_A19_read_retry_cycle, 1

	.type	sandisk_A19_read_retry_reg_cnt,@object // @sandisk_A19_read_retry_reg_cnt
	.globl	sandisk_A19_read_retry_reg_cnt
sandisk_A19_read_retry_reg_cnt:
	.byte	0                               // 0x0
	.size	sandisk_A19_read_retry_reg_cnt, 1

	.type	sandisk_A19_read_retry_enable_cmd_1y,@object // @sandisk_A19_read_retry_enable_cmd_1y
	.data
	.globl	sandisk_A19_read_retry_enable_cmd_1y
sandisk_A19_read_retry_enable_cmd_1y:
	.byte	93
	.size	sandisk_A19_read_retry_enable_cmd_1y, 1

	.type	sandisk_A19_read_retry_exit_cmd_1y,@object // @sandisk_A19_read_retry_exit_cmd_1y
	.globl	sandisk_A19_read_retry_exit_cmd_1y
sandisk_A19_read_retry_exit_cmd_1y:
	.byte	255
	.size	sandisk_A19_read_retry_exit_cmd_1y, 1

	.type	sandisk_A19_read_retry_cmd_1y,@object // @sandisk_A19_read_retry_cmd_1y
	.globl	sandisk_A19_read_retry_cmd_1y
sandisk_A19_read_retry_cmd_1y:
	.byte	239
	.size	sandisk_A19_read_retry_cmd_1y, 1

	.type	sandisk_A19_read_retry_reg_adr_1y,@object // @sandisk_A19_read_retry_reg_adr_1y
	.globl	sandisk_A19_read_retry_reg_adr_1y
sandisk_A19_read_retry_reg_adr_1y:
	.byte	17
	.size	sandisk_A19_read_retry_reg_adr_1y, 1

	.type	sandisk_A19_1y,@object          // @sandisk_A19_1y
	.globl	sandisk_A19_1y
sandisk_A19_1y:
	.ascii	"|\000\000|"
	.ascii	"\004\000|x"
	.ascii	"x\000xt"
	.ascii	"\b|\000|"
	.ascii	"\000||x"
	.ascii	"||xt"
	.ascii	"\000|tp"
	.ascii	"\000x\000|"
	.ascii	"\000x|x"
	.ascii	"\000xxt"
	.ascii	"\000xtp"
	.ascii	"\000xpl"
	.asciz	"\000\004\004"
	.ascii	"\000\004\000|"
	.ascii	"\f\004|x"
	.ascii	"\f\004xt"
	.ascii	"\020\b\000|"
	.asciz	"\020\b\004"
	.ascii	"xtxt"
	.ascii	"xttp"
	.ascii	"xtpl"
	.ascii	"xtlh"
	.ascii	"xpxt"
	.ascii	"xptp"
	.ascii	"xplh"
	.ascii	"xppl"
	.ascii	"xlpl"
	.ascii	"xllh"
	.ascii	"xlhd"
	.ascii	"thlh"
	.ascii	"thhd"
	.size	sandisk_A19_1y, 124

	.type	sandisk_A19_1z_16g,@object      // @sandisk_A19_1z_16g
	.globl	sandisk_A19_1z_16g
sandisk_A19_1z_16g:
	.asciz	"|\000\004"
	.ascii	"\004\000\000|"
	.ascii	"x\000\b\004"
	.ascii	"\b\000|x"
	.asciz	"||\004"
	.ascii	"\000|\000|"
	.ascii	"x|\b\004"
	.ascii	"\004|tx"
	.asciz	"\004\004\004"
	.ascii	"\b\004\000|"
	.ascii	"\000\004\b\004"
	.ascii	"\f\004|x"
	.ascii	"|\004\f\b"
	.asciz	"xx\004"
	.ascii	"|x\000|"
	.ascii	"txpx"
	.asciz	"\b\b\004"
	.ascii	"\004\b\b\004"
	.ascii	"\f\b\f\b"
	.ascii	"\000\000\f\b"
	.ascii	"\004\000xt"
	.ascii	"||xt"
	.ascii	"xxxt"
	.ascii	"\b\b\000|"
	.ascii	"tt\000|"
	.ascii	"xt|x"
	.ascii	"pttt"
	.ascii	"|tpp"
	.ascii	"\f\f\b\004"
	.ascii	"\020\f\f\b"
	.ascii	"pppt"
	.ascii	"tppp"
	.size	sandisk_A19_1z_16g, 128

	.type	sandisk_A19_1z_8g,@object       // @sandisk_A19_1z_8g
	.globl	sandisk_A19_1z_8g
sandisk_A19_1z_8g:
	.ascii	"\004\004xx"
	.ascii	"\004\004|t"
	.ascii	"\000\000\004x"
	.ascii	"\004\004||"
	.ascii	"\000\000\000|"
	.ascii	"\000\000\000t"
	.ascii	"\b\b\004x"
	.ascii	"||\004x"
	.ascii	"||\000|"
	.ascii	"\004\004|p"
	.ascii	"||tt"
	.ascii	"\000\000xp"
	.ascii	"\f\f\bx"
	.ascii	"xx||"
	.ascii	"\004\004\b\004"
	.ascii	"xx\bx"
	.ascii	"||xp"
	.ascii	"xxpl"
	.ascii	"\000\000tl"
	.ascii	"\b\b\000t"
	.ascii	"||xl"
	.ascii	"\000\000\004\004"
	.ascii	"tttl"
	.ascii	"xx|p"
	.ascii	"\f\f\000t"
	.ascii	"\004\004\f\b"
	.ascii	"xx|t"
	.ascii	"xxph"
	.ascii	"\b\b\000p"
	.ascii	"\020\020\fx"
	.ascii	"\000\000\f\b"
	.size	sandisk_A19_1z_8g, 124

	.type	.L.str.412,@object              // @.str.412
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.412:
	.asciz	"sandisk_A19 retry!\n"
	.size	.L.str.412, 20

	.type	.L.str.413,@object              // @.str.413
.L.str.413:
	.asciz	"sandisk_A19 set readretry error\n"
	.size	.L.str.413, 33

	.type	.L.str.414,@object              // @.str.414
.L.str.414:
	.asciz	"sandisk_A19 ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.414, 87

	.type	.L.str.415,@object              // @.str.415
.L.str.415:
	.asciz	"sandisk_A19 ReadRetry ok with ecc limit! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.415, 102

	.type	.L.str.416,@object              // @.str.416
.L.str.416:
	.asciz	"sandisk_A19 retry mode err : %d !\n"
	.size	.L.str.416, 35

	.type	.L.str.417,@object              // @.str.417
.L.str.417:
	.asciz	"sandisk A19 read retry conut: %d !\n"
	.size	.L.str.417, 36

	.type	sandisk_A19_sclk0_bak,@object   // @sandisk_A19_sclk0_bak
	.local	sandisk_A19_sclk0_bak
	.comm	sandisk_A19_sclk0_bak,4,4
	.type	sandisk_A19_sclk1_bak,@object   // @sandisk_A19_sclk1_bak
	.local	sandisk_A19_sclk1_bak
	.comm	sandisk_A19_sclk1_bak,4,4
	.type	.L.str.418,@object              // @.str.418
.L.str.418:
	.asciz	" sandisk_A19_special_init sandisk_A19_read_retry_mode:%d sandisk_A19_read_retry_cycle :%d sandisk_A19_read_retry_reg_cnt %d \n"
	.size	.L.str.418, 126

	.type	.L.str.419,@object              // @.str.419
.L.str.419:
	.asciz	" sandisk_A19_special_init error sandisk_A19_read_retry_mode:%d sandisk_A19_read_retry_cycle :%d sandisk_A19_read_retry_reg_cnt %d \n"
	.size	.L.str.419, 132

	.type	.L.str.420,@object              // @.str.420
.L.str.420:
	.asciz	"%s sandisk toggle vendor specific setting failed!\n"
	.size	.L.str.420, 51

	.type	.L__func__.sandisk_A19_special_exit,@object // @__func__.sandisk_A19_special_exit
.L__func__.sandisk_A19_special_exit:
	.asciz	"sandisk_A19_special_exit"
	.size	.L__func__.sandisk_A19_special_exit, 25

	.type	.L.str.421,@object              // @.str.421
.L.str.421:
	.asciz	" sandisk_A19_special_exit \n"
	.size	.L.str.421, 28

	.type	.L.str.422,@object              // @.str.422
.L.str.422:
	.asciz	"fatal err -0, wrong input parameter, ch: %d  chip: %d/%d  block: %d/%d \n"
	.size	.L.str.422, 73

	.type	hynix20nm_read_retry_mode,@object // @hynix20nm_read_retry_mode
	.bss
	.globl	hynix20nm_read_retry_mode
hynix20nm_read_retry_mode:
	.byte	0                               // 0x0
	.size	hynix20nm_read_retry_mode, 1

	.type	hynix20nm_read_retry_cycle,@object // @hynix20nm_read_retry_cycle
	.globl	hynix20nm_read_retry_cycle
hynix20nm_read_retry_cycle:
	.byte	0                               // 0x0
	.size	hynix20nm_read_retry_cycle, 1

	.type	hynix20nm_read_retry_reg_num,@object // @hynix20nm_read_retry_reg_num
	.globl	hynix20nm_read_retry_reg_num
hynix20nm_read_retry_reg_num:
	.byte	0                               // 0x0
	.size	hynix20nm_read_retry_reg_num, 1

	.type	hynix20nm_read_retry_reg_adr,@object // @hynix20nm_read_retry_reg_adr
	.globl	hynix20nm_read_retry_reg_adr
hynix20nm_read_retry_reg_adr:
	.zero	8
	.size	hynix20nm_read_retry_reg_adr, 8

	.type	hynix20nm_read_retry_reg_adr_1,@object // @hynix20nm_read_retry_reg_adr_1
	.data
	.globl	hynix20nm_read_retry_reg_adr_1
hynix20nm_read_retry_reg_adr_1:
	.ascii	"\314\277\252\253\315\255\256\257"
	.size	hynix20nm_read_retry_reg_adr_1, 8

	.type	hynix20nm_read_retry_reg_adr_2,@object // @hynix20nm_read_retry_reg_adr_2
	.globl	hynix20nm_read_retry_reg_adr_2
hynix20nm_read_retry_reg_adr_2:
	.ascii	"\260\261\262\263\264\265\266\267"
	.size	hynix20nm_read_retry_reg_adr_2, 8

	.type	hynix20nm_lsb_mode_reg_adr,@object // @hynix20nm_lsb_mode_reg_adr
	.bss
	.globl	hynix20nm_lsb_mode_reg_adr
hynix20nm_lsb_mode_reg_adr:
	.zero	4
	.size	hynix20nm_lsb_mode_reg_adr, 4

	.type	hynix20nm_lsb_mode_default_val,@object // @hynix20nm_lsb_mode_default_val
	.globl	hynix20nm_lsb_mode_default_val
hynix20nm_lsb_mode_default_val:
	.zero	4
	.size	hynix20nm_lsb_mode_default_val, 4

	.type	hynix20nm_lsb_mode_val,@object  // @hynix20nm_lsb_mode_val
	.globl	hynix20nm_lsb_mode_val
hynix20nm_lsb_mode_val:
	.zero	4
	.size	hynix20nm_lsb_mode_val, 4

	.type	hynix20nm_lsb_mode_reg_num,@object // @hynix20nm_lsb_mode_reg_num
	.globl	hynix20nm_lsb_mode_reg_num
hynix20nm_lsb_mode_reg_num:
	.byte	0                               // 0x0
	.size	hynix20nm_lsb_mode_reg_num, 1

	.type	.L.str.423,@object              // @.str.423
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.423:
	.asciz	"retry para:"
	.size	.L.str.423, 12

	.type	.L.str.424,@object              // @.str.424
.L.str.424:
	.asciz	"hynix20nm retry!\n"
	.size	.L.str.424, 18

	.type	.L.str.425,@object              // @.str.425
.L.str.425:
	.asciz	"sdata:0x%x %x %x %x "
	.size	.L.str.425, 21

	.type	.L.str.426,@object              // @.str.426
.L.str.426:
	.asciz	"hynix20nm ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.426, 85

	.type	.L.str.427,@object              // @.str.427
.L.str.427:
	.asciz	"hynix20nm_lsb_exit\n"
	.size	.L.str.427, 20

	.type	.L.str.428,@object              // @.str.428
.L.str.428:
	.asciz	"hynix OTP RegCount value error: 0x%x, 0x%x \n"
	.size	.L.str.428, 45

	.type	.L.str.429,@object              // @.str.429
.L.str.429:
	.asciz	"otp copy %d is ok!\n"
	.size	.L.str.429, 20

	.type	.L.str.430,@object              // @.str.430
.L.str.430:
	.asciz	"otp copy failed!\n"
	.size	.L.str.430, 18

	.type	.L.str.431,@object              // @.str.431
.L.str.431:
	.asciz	"_vender_get_param_otp_hynix time %d!\n"
	.size	.L.str.431, 38

	.type	.L.str.432,@object              // @.str.432
.L.str.432:
	.asciz	"_vender_get_param_otp_hynix error!\n"
	.size	.L.str.432, 36

	.type	.L.str.433,@object              // @.str.433
.L.str.433:
	.asciz	"hynix20nm get default param error !\n"
	.size	.L.str.433, 37

	.type	.L.str.434,@object              // @.str.434
.L.str.434:
	.asciz	"[PHY_GetDefaultParam]:readbuf malloc fail\n"
	.size	.L.str.434, 43

	.type	.L.str.435,@object              // @.str.435
.L.str.435:
	.asciz	"ch %d, chip %d Read Retry value Table from uboot:\n"
	.size	.L.str.435, 51

	.type	.L.str.436,@object              // @.str.436
.L.str.436:
	.asciz	"0x%x "
	.size	.L.str.436, 6

	.type	.L.str.437,@object              // @.str.437
.L.str.437:
	.asciz	"[PHY_DBG] ch %d, can't get right otp value from nand otp blocks, then use otp command\n"
	.size	.L.str.437, 87

	.type	.L.str.438,@object              // @.str.438
.L.str.438:
	.asciz	"[PHY_DBG] repair ch %d otp value end\n"
	.size	.L.str.438, 38

	.type	.L.str.439,@object              // @.str.439
.L.str.439:
	.asciz	"hynix20nm_readretry_exit. \n"
	.size	.L.str.439, 28

	.type	.L.str.440,@object              // @.str.440
.L.str.440:
	.asciz	" hynix20nm_special_init hynix20nm_read_retry_mode: %d hynix20nm_read_retry_cycle:%d hynix20nm_read_retry_reg_num:%d\n"
	.size	.L.str.440, 117

	.type	.L.str.441,@object              // @.str.441
.L.str.441:
	.asciz	" hynix20nm_special_init error hynix20nm_read_retry_mode: %d hynix20nm_read_retry_cycle:%d hynix20nm_read_retry_reg_num:%d\n"
	.size	.L.str.441, 123

	.type	.L.str.442,@object              // @.str.442
.L.str.442:
	.asciz	" hynix20nm_special_exit \n"
	.size	.L.str.442, 26

	.type	micron_read_retry_mode,@object  // @micron_read_retry_mode
	.bss
	.globl	micron_read_retry_mode
micron_read_retry_mode:
	.byte	0                               // 0x0
	.size	micron_read_retry_mode, 1

	.type	micron_read_retry_cycle,@object // @micron_read_retry_cycle
	.globl	micron_read_retry_cycle
micron_read_retry_cycle:
	.byte	0                               // 0x0
	.size	micron_read_retry_cycle, 1

	.type	micron_p1_1,@object             // @micron_p1_1
	.data
	.globl	micron_p1_1
micron_p1_1:
	.ascii	"\000\000\000\001\002\003\004\005\006\007"
	.size	micron_p1_1, 10

	.type	micron_p1_2,@object             // @micron_p1_2
	.globl	micron_p1_2
micron_p1_2:
	.ascii	"\000\000\000\001\002\003\004\005\006\007\b\f"
	.size	micron_p1_2, 12

	.type	micron_p1_3,@object             // @micron_p1_3
	.globl	micron_p1_3
micron_p1_3:
	.ascii	"\001\002\003\000\001\002\003"
	.size	micron_p1_3, 7

	.type	micron_p1_4,@object             // @micron_p1_4
	.globl	micron_p1_4
micron_p1_4:
	.ascii	"\000\000\000\001\002\003\004\005\006\007\b\t\n\013\f\r\016\017"
	.size	micron_p1_4, 18

	.type	.L.str.443,@object              // @.str.443
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.443:
	.asciz	"set feature(addr %d) p0: %d readretry Configuration failed!\n"
	.size	.L.str.443, 61

	.type	micron_p1,@object               // @micron_p1
	.local	micron_p1
	.comm	micron_p1,8,8
	.type	.L.str.444,@object              // @.str.444
.L.str.444:
	.asciz	"micron retry!\n"
	.size	.L.str.444, 15

	.type	.L.str.445,@object              // @.str.445
.L.str.445:
	.asciz	"micron ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.445, 82

	.type	.L.str.446,@object              // @.str.446
.L.str.446:
	.asciz	"micron read retry cannot support!\n"
	.size	.L.str.446, 35

	.type	.L.str.447,@object              // @.str.447
.L.str.447:
	.asciz	"micron read retry conut: %d !\n"
	.size	.L.str.447, 31

	.type	.L.str.448,@object              // @.str.448
.L.str.448:
	.asciz	" micron_special_init micron_read_retry_mode:%d micron_read_retry_cycle:%d \n"
	.size	.L.str.448, 76

	.type	.L.str.449,@object              // @.str.449
.L.str.449:
	.asciz	" micron_special_init error micron_read_retry_mode:%d micron_read_retry_cycle:%d \n"
	.size	.L.str.449, 82

	.type	.L.str.450,@object              // @.str.450
.L.str.450:
	.asciz	" micron_special_exit \n"
	.size	.L.str.450, 23

	.type	hynix16nm_read_retry_mode,@object // @hynix16nm_read_retry_mode
	.bss
	.globl	hynix16nm_read_retry_mode
hynix16nm_read_retry_mode:
	.byte	0                               // 0x0
	.size	hynix16nm_read_retry_mode, 1

	.type	hynix16nm_read_retry_cycle,@object // @hynix16nm_read_retry_cycle
	.globl	hynix16nm_read_retry_cycle
hynix16nm_read_retry_cycle:
	.byte	0                               // 0x0
	.size	hynix16nm_read_retry_cycle, 1

	.type	hynix16nm_read_retry_reg_num,@object // @hynix16nm_read_retry_reg_num
	.globl	hynix16nm_read_retry_reg_num
hynix16nm_read_retry_reg_num:
	.byte	0                               // 0x0
	.size	hynix16nm_read_retry_reg_num, 1

	.type	hynix16nm_read_retry_reg_adr,@object // @hynix16nm_read_retry_reg_adr
	.globl	hynix16nm_read_retry_reg_adr
hynix16nm_read_retry_reg_adr:
	.zero	4
	.size	hynix16nm_read_retry_reg_adr, 4

	.type	.L.str.451,@object              // @.str.451
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.451:
	.asciz	"get_hynix_special_info start!!\n"
	.size	.L.str.451, 32

	.type	.L.str.452,@object              // @.str.452
.L.str.452:
	.asciz	"hynix_special_info:can't find uboot head\n"
	.size	.L.str.452, 42

	.type	.L.str.453,@object              // @.str.453
.L.str.453:
	.asciz	"physic info start_block %d\n"
	.size	.L.str.453, 28

	.type	.L.str.454,@object              // @.str.454
.L.str.454:
	.asciz	"m1 retry!\n"
	.size	.L.str.454, 11

	.type	.L.str.455,@object              // @.str.455
.L.str.455:
	.asciz	"m1 ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.455, 78

	.type	.L.str.456,@object              // @.str.456
.L.str.456:
	.asciz	"m1 write page FF wrong\n"
	.size	.L.str.456, 24

	.type	.L.str.457,@object              // @.str.457
.L.str.457:
	.asciz	"_major_check_byte, wrong input para, level %d, len %d\n"
	.size	.L.str.457, 55

	.type	.L.str.458,@object              // @.str.458
.L.str.458:
	.asciz	"%d:  '1'-'0' : %d - %d\n"
	.size	.L.str.458, 24

	.type	.L.str.459,@object              // @.str.459
.L.str.459:
	.asciz	"get bit %d failed!\n"
	.size	.L.str.459, 20

	.type	.L.str.460,@object              // @.str.460
.L.str.460:
	.asciz	"_get_read_retry_parameters, get rr count failed!\n"
	.size	.L.str.460, 50

	.type	.L.str.461,@object              // @.str.461
.L.str.461:
	.asciz	"rr cnt: %d\n"
	.size	.L.str.461, 12

	.type	.L.str.462,@object              // @.str.462
.L.str.462:
	.asciz	"_get_read_retry_parameters, get rr reg count failed!\n"
	.size	.L.str.462, 54

	.type	.L.str.463,@object              // @.str.463
.L.str.463:
	.asciz	"rr reg cnt: %d\n"
	.size	.L.str.463, 16

	.type	.L.str.464,@object              // @.str.464
.L.str.464:
	.asciz	"read retry value from otp error: rr_cnt %d rr_reg_cnt %d!\n"
	.size	.L.str.464, 59

	.type	.L.str.465,@object              // @.str.465
.L.str.465:
	.asciz	"_get_read_retry_parameters, get the %d-th byte of rr table failed!\n"
	.size	.L.str.465, 68

	.type	.L.str.466,@object              // @.str.466
.L.str.466:
	.asciz	"start get read retry param from: ce %d, rb %d...\n"
	.size	.L.str.466, 50

	.type	.L.str.467,@object              // @.str.467
.L.str.467:
	.asciz	"invalid buffer for otp info!\n"
	.size	.L.str.467, 30

	.type	.L.str.468,@object              // @.str.468
.L.str.468:
	.asciz	"otp_info_hynix_16nm : allocate memory fail\n"
	.size	.L.str.468, 44

	.type	.L.str.469,@object              // @.str.469
.L.str.469:
	.asciz	"m1 CH %d chip %d get otp info failed!\n"
	.size	.L.str.469, 39

	.type	.L.str.470,@object              // @.str.470
.L.str.470:
	.asciz	"m1 CH %d chip %d get read retry cfg from otp info failed!\n"
	.size	.L.str.470, 59

	.type	.L.str.471,@object              // @.str.471
.L.str.471:
	.asciz	"ch %d, chip %d Read Retry value Table from uboot\n"
	.size	.L.str.471, 50

	.type	.L.str.472,@object              // @.str.472
.L.str.472:
	.asciz	"[PHY_DBG] repair ch %d chip %d otp value end\n"
	.size	.L.str.472, 46

	.type	.L.str.473,@object              // @.str.473
.L.str.473:
	.asciz	"hynix16nm_readretry_exit. \n"
	.size	.L.str.473, 28

	.type	.L.str.474,@object              // @.str.474
.L.str.474:
	.asciz	" hynix16nm_special_init hynix16nm_read_retry_mode: %d hynix16nm_read_retry_cycle:%d hynix16nm_read_retry_reg_num:%d\n"
	.size	.L.str.474, 117

	.type	.L.str.475,@object              // @.str.475
.L.str.475:
	.asciz	" hynix16nm_special_init error hynix16nm_read_retry_mode: %d hynix16nm_read_retry_cycle:%d hynix16nm_read_retry_reg_num:%d\n"
	.size	.L.str.475, 123

	.type	.L.str.476,@object              // @.str.476
.L.str.476:
	.asciz	" hynix16nm_special_exit \n"
	.size	.L.str.476, 26

	.type	toshiba_read_retry_mode,@object // @toshiba_read_retry_mode
	.bss
	.globl	toshiba_read_retry_mode
toshiba_read_retry_mode:
	.byte	0                               // 0x0
	.size	toshiba_read_retry_mode, 1

	.type	toshiba_read_retry_cycle,@object // @toshiba_read_retry_cycle
	.globl	toshiba_read_retry_cycle
toshiba_read_retry_cycle:
	.byte	0                               // 0x0
	.size	toshiba_read_retry_cycle, 1

	.type	toshiba_read_retry_reg_cnt,@object // @toshiba_read_retry_reg_cnt
	.globl	toshiba_read_retry_reg_cnt
toshiba_read_retry_reg_cnt:
	.byte	0                               // 0x0
	.size	toshiba_read_retry_reg_cnt, 1

	.type	toshiba_read_retry_start_cmd,@object // @toshiba_read_retry_start_cmd
	.data
	.globl	toshiba_read_retry_start_cmd
toshiba_read_retry_start_cmd:
	.ascii	"\\\305"
	.size	toshiba_read_retry_start_cmd, 2

	.type	toshiba_read_retry_end_cmd,@object // @toshiba_read_retry_end_cmd
	.globl	toshiba_read_retry_end_cmd
toshiba_read_retry_end_cmd:
	.ascii	"&]"
	.size	toshiba_read_retry_end_cmd, 2

	.type	toshiba_read_retry_cmd_1,@object // @toshiba_read_retry_cmd_1
	.globl	toshiba_read_retry_cmd_1
toshiba_read_retry_cmd_1:
	.zero	4,85
	.size	toshiba_read_retry_cmd_1, 4

	.type	toshiba_read_retry_addr_1,@object // @toshiba_read_retry_addr_1
	.globl	toshiba_read_retry_addr_1
toshiba_read_retry_addr_1:
	.ascii	"\004\005\006\007"
	.size	toshiba_read_retry_addr_1, 4

	.type	toshiba_p1_1,@object            // @toshiba_p1_1
	.globl	toshiba_p1_1
toshiba_p1_1:
	.zero	4
	.zero	4,4
	.zero	4,124
	.zero	4,120
	.zero	4,116
	.zero	4,8
	.zero	4,255
	.size	toshiba_p1_1, 28

	.type	toshiba_read_retry_cmd_2,@object // @toshiba_read_retry_cmd_2
	.globl	toshiba_read_retry_cmd_2
toshiba_read_retry_cmd_2:
	.zero	5,85
	.size	toshiba_read_retry_cmd_2, 5

	.type	toshiba_read_retry_addr_2,@object // @toshiba_read_retry_addr_2
	.globl	toshiba_read_retry_addr_2
toshiba_read_retry_addr_2:
	.ascii	"\004\005\006\007\r"
	.size	toshiba_read_retry_addr_2, 5

	.type	toshiba_p1_2,@object            // @toshiba_p1_2
	.globl	toshiba_p1_2
toshiba_p1_2:
	.asciz	"\004\004|~"
	.asciz	"\000|xx"
	.asciz	"|vtr"
	.asciz	"\b\b\000\000"
	.asciz	"\013~vt"
	.asciz	"\020vrp"
	.asciz	"\002\000~|"
	.zero	5
	.size	toshiba_p1_2, 40

	.type	toshiba_p1_3,@object            // @toshiba_p1_3
	.globl	toshiba_p1_3
toshiba_p1_3:
	.zero	5
	.asciz	"\002\004\002\000"
	.asciz	"|\000||"
	.asciz	"z\000zz"
	.asciz	"x\002xz"
	.asciz	"~\004~z"
	.asciz	"v\004vx"
	.asciz	"\004\004\004v"
	.asciz	"\006\n\006\002"
	.asciz	"t|tv"
	.zero	5
	.size	toshiba_p1_3, 55

	.type	toshiba_read_retry_last_cmd,@object // @toshiba_read_retry_last_cmd
	.globl	toshiba_read_retry_last_cmd
toshiba_read_retry_last_cmd:
	.byte	179
	.size	toshiba_read_retry_last_cmd, 1

	.type	toshiba_read_retry_exit_cmd,@object // @toshiba_read_retry_exit_cmd
	.globl	toshiba_read_retry_exit_cmd
toshiba_read_retry_exit_cmd:
	.byte	255
	.size	toshiba_read_retry_exit_cmd, 1

	.type	toshiba_sclk0_bak,@object       // @toshiba_sclk0_bak
	.local	toshiba_sclk0_bak
	.comm	toshiba_sclk0_bak,4,4
	.type	toshiba_sclk1_bak,@object       // @toshiba_sclk1_bak
	.local	toshiba_sclk1_bak
	.comm	toshiba_sclk1_bak,4,4
	.type	toshiba_retry_flag,@object      // @toshiba_retry_flag
	.local	toshiba_retry_flag
	.comm	toshiba_retry_flag,4,4
	.type	.L.str.477,@object              // @.str.477
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.477:
	.asciz	"toshiba rr value 3 %x %x %x %x!\n"
	.size	.L.str.477, 33

	.type	.L.str.478,@object              // @.str.478
.L.str.478:
	.asciz	"toshiba vender_pre_condition error!\n"
	.size	.L.str.478, 37

	.type	toshiba_toggle_mode_flag,@object // @toshiba_toggle_mode_flag
	.local	toshiba_toggle_mode_flag
	.comm	toshiba_toggle_mode_flag,4,4
	.type	.L.str.479,@object              // @.str.479
.L.str.479:
	.asciz	"toshiba retry!\n"
	.size	.L.str.479, 16

	.type	.L.str.480,@object              // @.str.480
.L.str.480:
	.asciz	"toshiba ReadRetry ok! ch =%d, chip = %d  block = %d, page = %d, RetryCount = %d  \n"
	.size	.L.str.480, 83

	.type	.L.str.481,@object              // @.str.481
.L.str.481:
	.asciz	"toshiba read retry conut: %d !\n"
	.size	.L.str.481, 32

	.type	.L.str.482,@object              // @.str.482
.L.str.482:
	.asciz	" toshiba_special_init toshiba_read_retry_mode :%d toshiba_read_retry_cycle :%d toshiba_read_retry_reg_cnt :%d \n"
	.size	.L.str.482, 112

	.type	.L.str.483,@object              // @.str.483
.L.str.483:
	.asciz	" toshiba_special_init error toshiba_read_retry_mode :%d toshiba_read_retry_cycle :%d toshiba_read_retry_reg_cnt :%d \n"
	.size	.L.str.483, 118

	.type	.L.str.484,@object              // @.str.484
.L.str.484:
	.asciz	" toshiba_special_exit \n"
	.size	.L.str.484, 24

	.type	nand_crc16_table,@object        // @nand_crc16_table
	.section	.rodata,"a",@progbits
	.p2align	1
nand_crc16_table:
	.hword	0                               // 0x0
	.hword	49345                           // 0xc0c1
	.hword	49537                           // 0xc181
	.hword	320                             // 0x140
	.hword	49921                           // 0xc301
	.hword	960                             // 0x3c0
	.hword	640                             // 0x280
	.hword	49729                           // 0xc241
	.hword	50689                           // 0xc601
	.hword	1728                            // 0x6c0
	.hword	1920                            // 0x780
	.hword	51009                           // 0xc741
	.hword	1280                            // 0x500
	.hword	50625                           // 0xc5c1
	.hword	50305                           // 0xc481
	.hword	1088                            // 0x440
	.hword	52225                           // 0xcc01
	.hword	3264                            // 0xcc0
	.hword	3456                            // 0xd80
	.hword	52545                           // 0xcd41
	.hword	3840                            // 0xf00
	.hword	53185                           // 0xcfc1
	.hword	52865                           // 0xce81
	.hword	3648                            // 0xe40
	.hword	2560                            // 0xa00
	.hword	51905                           // 0xcac1
	.hword	52097                           // 0xcb81
	.hword	2880                            // 0xb40
	.hword	51457                           // 0xc901
	.hword	2496                            // 0x9c0
	.hword	2176                            // 0x880
	.hword	51265                           // 0xc841
	.hword	55297                           // 0xd801
	.hword	6336                            // 0x18c0
	.hword	6528                            // 0x1980
	.hword	55617                           // 0xd941
	.hword	6912                            // 0x1b00
	.hword	56257                           // 0xdbc1
	.hword	55937                           // 0xda81
	.hword	6720                            // 0x1a40
	.hword	7680                            // 0x1e00
	.hword	57025                           // 0xdec1
	.hword	57217                           // 0xdf81
	.hword	8000                            // 0x1f40
	.hword	56577                           // 0xdd01
	.hword	7616                            // 0x1dc0
	.hword	7296                            // 0x1c80
	.hword	56385                           // 0xdc41
	.hword	5120                            // 0x1400
	.hword	54465                           // 0xd4c1
	.hword	54657                           // 0xd581
	.hword	5440                            // 0x1540
	.hword	55041                           // 0xd701
	.hword	6080                            // 0x17c0
	.hword	5760                            // 0x1680
	.hword	54849                           // 0xd641
	.hword	53761                           // 0xd201
	.hword	4800                            // 0x12c0
	.hword	4992                            // 0x1380
	.hword	54081                           // 0xd341
	.hword	4352                            // 0x1100
	.hword	53697                           // 0xd1c1
	.hword	53377                           // 0xd081
	.hword	4160                            // 0x1040
	.hword	61441                           // 0xf001
	.hword	12480                           // 0x30c0
	.hword	12672                           // 0x3180
	.hword	61761                           // 0xf141
	.hword	13056                           // 0x3300
	.hword	62401                           // 0xf3c1
	.hword	62081                           // 0xf281
	.hword	12864                           // 0x3240
	.hword	13824                           // 0x3600
	.hword	63169                           // 0xf6c1
	.hword	63361                           // 0xf781
	.hword	14144                           // 0x3740
	.hword	62721                           // 0xf501
	.hword	13760                           // 0x35c0
	.hword	13440                           // 0x3480
	.hword	62529                           // 0xf441
	.hword	15360                           // 0x3c00
	.hword	64705                           // 0xfcc1
	.hword	64897                           // 0xfd81
	.hword	15680                           // 0x3d40
	.hword	65281                           // 0xff01
	.hword	16320                           // 0x3fc0
	.hword	16000                           // 0x3e80
	.hword	65089                           // 0xfe41
	.hword	64001                           // 0xfa01
	.hword	15040                           // 0x3ac0
	.hword	15232                           // 0x3b80
	.hword	64321                           // 0xfb41
	.hword	14592                           // 0x3900
	.hword	63937                           // 0xf9c1
	.hword	63617                           // 0xf881
	.hword	14400                           // 0x3840
	.hword	10240                           // 0x2800
	.hword	59585                           // 0xe8c1
	.hword	59777                           // 0xe981
	.hword	10560                           // 0x2940
	.hword	60161                           // 0xeb01
	.hword	11200                           // 0x2bc0
	.hword	10880                           // 0x2a80
	.hword	59969                           // 0xea41
	.hword	60929                           // 0xee01
	.hword	11968                           // 0x2ec0
	.hword	12160                           // 0x2f80
	.hword	61249                           // 0xef41
	.hword	11520                           // 0x2d00
	.hword	60865                           // 0xedc1
	.hword	60545                           // 0xec81
	.hword	11328                           // 0x2c40
	.hword	58369                           // 0xe401
	.hword	9408                            // 0x24c0
	.hword	9600                            // 0x2580
	.hword	58689                           // 0xe541
	.hword	9984                            // 0x2700
	.hword	59329                           // 0xe7c1
	.hword	59009                           // 0xe681
	.hword	9792                            // 0x2640
	.hword	8704                            // 0x2200
	.hword	58049                           // 0xe2c1
	.hword	58241                           // 0xe381
	.hword	9024                            // 0x2340
	.hword	57601                           // 0xe101
	.hword	8640                            // 0x21c0
	.hword	8320                            // 0x2080
	.hword	57409                           // 0xe041
	.hword	40961                           // 0xa001
	.hword	24768                           // 0x60c0
	.hword	24960                           // 0x6180
	.hword	41281                           // 0xa141
	.hword	25344                           // 0x6300
	.hword	41921                           // 0xa3c1
	.hword	41601                           // 0xa281
	.hword	25152                           // 0x6240
	.hword	26112                           // 0x6600
	.hword	42689                           // 0xa6c1
	.hword	42881                           // 0xa781
	.hword	26432                           // 0x6740
	.hword	42241                           // 0xa501
	.hword	26048                           // 0x65c0
	.hword	25728                           // 0x6480
	.hword	42049                           // 0xa441
	.hword	27648                           // 0x6c00
	.hword	44225                           // 0xacc1
	.hword	44417                           // 0xad81
	.hword	27968                           // 0x6d40
	.hword	44801                           // 0xaf01
	.hword	28608                           // 0x6fc0
	.hword	28288                           // 0x6e80
	.hword	44609                           // 0xae41
	.hword	43521                           // 0xaa01
	.hword	27328                           // 0x6ac0
	.hword	27520                           // 0x6b80
	.hword	43841                           // 0xab41
	.hword	26880                           // 0x6900
	.hword	43457                           // 0xa9c1
	.hword	43137                           // 0xa881
	.hword	26688                           // 0x6840
	.hword	30720                           // 0x7800
	.hword	47297                           // 0xb8c1
	.hword	47489                           // 0xb981
	.hword	31040                           // 0x7940
	.hword	47873                           // 0xbb01
	.hword	31680                           // 0x7bc0
	.hword	31360                           // 0x7a80
	.hword	47681                           // 0xba41
	.hword	48641                           // 0xbe01
	.hword	32448                           // 0x7ec0
	.hword	32640                           // 0x7f80
	.hword	48961                           // 0xbf41
	.hword	32000                           // 0x7d00
	.hword	48577                           // 0xbdc1
	.hword	48257                           // 0xbc81
	.hword	31808                           // 0x7c40
	.hword	46081                           // 0xb401
	.hword	29888                           // 0x74c0
	.hword	30080                           // 0x7580
	.hword	46401                           // 0xb541
	.hword	30464                           // 0x7700
	.hword	47041                           // 0xb7c1
	.hword	46721                           // 0xb681
	.hword	30272                           // 0x7640
	.hword	29184                           // 0x7200
	.hword	45761                           // 0xb2c1
	.hword	45953                           // 0xb381
	.hword	29504                           // 0x7340
	.hword	45313                           // 0xb101
	.hword	29120                           // 0x71c0
	.hword	28800                           // 0x7080
	.hword	45121                           // 0xb041
	.hword	20480                           // 0x5000
	.hword	37057                           // 0x90c1
	.hword	37249                           // 0x9181
	.hword	20800                           // 0x5140
	.hword	37633                           // 0x9301
	.hword	21440                           // 0x53c0
	.hword	21120                           // 0x5280
	.hword	37441                           // 0x9241
	.hword	38401                           // 0x9601
	.hword	22208                           // 0x56c0
	.hword	22400                           // 0x5780
	.hword	38721                           // 0x9741
	.hword	21760                           // 0x5500
	.hword	38337                           // 0x95c1
	.hword	38017                           // 0x9481
	.hword	21568                           // 0x5440
	.hword	39937                           // 0x9c01
	.hword	23744                           // 0x5cc0
	.hword	23936                           // 0x5d80
	.hword	40257                           // 0x9d41
	.hword	24320                           // 0x5f00
	.hword	40897                           // 0x9fc1
	.hword	40577                           // 0x9e81
	.hword	24128                           // 0x5e40
	.hword	23040                           // 0x5a00
	.hword	39617                           // 0x9ac1
	.hword	39809                           // 0x9b81
	.hword	23360                           // 0x5b40
	.hword	39169                           // 0x9901
	.hword	22976                           // 0x59c0
	.hword	22656                           // 0x5880
	.hword	38977                           // 0x9841
	.hword	34817                           // 0x8801
	.hword	18624                           // 0x48c0
	.hword	18816                           // 0x4980
	.hword	35137                           // 0x8941
	.hword	19200                           // 0x4b00
	.hword	35777                           // 0x8bc1
	.hword	35457                           // 0x8a81
	.hword	19008                           // 0x4a40
	.hword	19968                           // 0x4e00
	.hword	36545                           // 0x8ec1
	.hword	36737                           // 0x8f81
	.hword	20288                           // 0x4f40
	.hword	36097                           // 0x8d01
	.hword	19904                           // 0x4dc0
	.hword	19584                           // 0x4c80
	.hword	35905                           // 0x8c41
	.hword	17408                           // 0x4400
	.hword	33985                           // 0x84c1
	.hword	34177                           // 0x8581
	.hword	17728                           // 0x4540
	.hword	34561                           // 0x8701
	.hword	18368                           // 0x47c0
	.hword	18048                           // 0x4680
	.hword	34369                           // 0x8641
	.hword	33281                           // 0x8201
	.hword	17088                           // 0x42c0
	.hword	17280                           // 0x4380
	.hword	33601                           // 0x8341
	.hword	16640                           // 0x4100
	.hword	33217                           // 0x81c1
	.hword	32897                           // 0x8081
	.hword	16448                           // 0x4040
	.size	nand_crc16_table, 512

	.type	.L.str.485,@object              // @.str.485
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.485:
	.asciz	"[NE]exception case wb %04x %04x, len: %d\n"
	.size	.L.str.485, 42

	.type	.L.str.486,@object              // @.str.486
.L.str.486:
	.asciz	"%.2x "
	.size	.L.str.486, 6

	.type	.L.str.487,@object              // @.str.487
.L.str.487:
	.asciz	"sunxi nftl version %s.%s.%s  %s-%s\n"
	.size	.L.str.487, 36

	.ident	"Android (7485623, based on r416183b1) clang version 12.0.7 (https://android.googlesource.com/toolchain/llvm-project c935d99d7cf2016289302412d708641d52d2f7ee)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym garbage_collect
	.addrsig_sym nand_print
	.addrsig_sym nand_print_dbg
	.addrsig_sym garbage_collect_first
	.addrsig_sym gc_win_block
	.addrsig_sym nand_memset
	.addrsig_sym out_phy_block_from_invalid_page_list
	.addrsig_sym is_current_blcok
	.addrsig_sym out_phy_block_from_invalid_page_list_by_block
	.addrsig_sym set_physic_op_par
	.addrsig_sym is_phy_mapping_page
	.addrsig_sym recover_block_phy_page_mapping
	.addrsig_sym nand_memcpy
	.addrsig_sym check_invalid_page_right
	.addrsig_sym get_logic_page_map
	.addrsig_sym get_logic_page_from_oob
	.addrsig_sym nand_write_logic_page_no_gc
	.addrsig_sym erase_block
	.addrsig_sym put_phy_block_to_invalid_page_list
	.addrsig_sym adjust_invaild_list
	.addrsig_sym get_empty_gc_node
	.addrsig_sym search_gc_node
	.addrsig_sym add_to_gc_list_tail
	.addrsig_sym del_from_gc_list
	.addrsig_sym add_prio_gc
	.addrsig_sym do_prio_gc
	.addrsig_sym is_block_in_invalid_list
	.addrsig_sym is_block_in_free_list
	.addrsig_sym out_phy_block_from_free_list_by_block
	.addrsig_sym check_static_wear_leveling
	.addrsig_sym get_block_max_erase_time_from_free_list
	.addrsig_sym get_block_min_erase_time_from_invalid_list
	.addrsig_sym done_static_wear_leveling
	.addrsig_sym fill_no_use_data_to_current_block
	.addrsig_sym get_phy_block_addr
	.addrsig_sym gc_one
	.addrsig_sym nand_get_cur_task_name
	.addrsig_sym nand_get_cur_task_pid
	.addrsig_sym gc_all_cnt
	.addrsig_sym gc_all
	.addrsig_sym get_block_min_count_from_invalid_list
	.addrsig_sym power_on_gc
	.addrsig_sym all_time_gc
	.addrsig_sym nand_snprint
	.addrsig_sym _nftl_nand_copy_page
	.addrsig_sym get_special_data_from_oob
	.addrsig_sym set_oob_special_page
	.addrsig_sym set_spare_data
	.addrsig_sym get_spare_data
	.addrsig_sym set_oob_logic_page
	.addrsig_sym set_start_block_flag
	.addrsig_sym set_end_block_flag
	.addrsig_sym get_block_used_count_from_oob
	.addrsig_sym get_erase_count_from_oob
	.addrsig_sym is_ftl_start_flag_page
	.addrsig_sym is_ftl_end_flag_page
	.addrsig_sym is_ftl_special_data
	.addrsig_sym is_ftl_logic_page_data
	.addrsig_sym is_nouse_page
	.addrsig_sym is_fill_page
	.addrsig_sym is_function_info_page
	.addrsig_sym is_power_down_info_page
	.addrsig_sym malloc_logic_page_map
	.addrsig_sym nftl_malloc
	.addrsig_sym free_logic_page_map
	.addrsig_sym nftl_free
	.addrsig_sym add_free_list
	.addrsig_sym del_free_list
	.addrsig_sym del_free_list_by_block
	.addrsig_sym print_free_list
	.addrsig_sym add_block_invalid_list
	.addrsig_sym del_block_invalid_list
	.addrsig_sym del_block_invalid_list_by_block
	.addrsig_sym print_block_invalid_list
	.addrsig_sym add_block_count_list
	.addrsig_sym del_block_count_by_block
	.addrsig_sym put_phy_block_to_free_list
	.addrsig_sym out_phy_block_from_free_list
	.addrsig_sym out_phy_block_from_free_list_by_erase_time
	.addrsig_sym out_phy_block_from_used_list_by_block
	.addrsig_sym phy_block_from_invalid_page_incr
	.addrsig_sym search_from_sector_cache_write_list
	.addrsig_sym get_empty_sector_cahce_node
	.addrsig_sym add_to_sector_cache_write_list_tail
	.addrsig_sym del_from_sector_cache_write_list
	.addrsig_sym nand_sector_cache_read
	.addrsig_sym nand_sector_cache_write
	.addrsig_sym __nand_read
	.addrsig_sym __nand_write
	.addrsig_sym nand_crc16
	.addrsig_sym nand_crc16_byte
	.addrsig_sym _nftl_nand_write_logic_page
	.addrsig_sym _nftl_nand_read_logic_page
	.addrsig_sym _nftl_discard_logic_page
	.addrsig_sym write_data_to_page
	.addrsig_sym do_write_error
	.addrsig_sym nand_write_data_page
	.addrsig_sym current_block_at_last_page
	.addrsig_sym do_after_write
	.addrsig_sym do_before_write
	.addrsig_sym current_block_at_first_page
	.addrsig_sym new_block_init_for_write
	.addrsig_sym get_new_current_block
	.addrsig_sym write_phy_page_map_to_current_block
	.addrsig_sym write_logic_data_to_current_block
	.addrsig_sym write_phy_data_to_current_block
	.addrsig_sym nftl_ops_init
	.addrsig_sym _nftl_nand_erase_superblk
	.addrsig_sym _nftl_nand_read_page
	.addrsig_sym _nftl_nand_write_page
	.addrsig_sym _nftl_nand_is_blk_good
	.addrsig_sym _nftl_nand_mark_bad_blk
	.addrsig_sym fill_no_use_data_to_page
	.addrsig_sym fill_no_use_page_to_current_block
	.addrsig_sym zone_param_init
	.addrsig_sym zone_param_exit
	.addrsig_sym is_no_use_device
	.addrsig_sym is_last_phy_block
	.addrsig_sym build_zone_list_first
	.addrsig_sym build_zone_list
	.addrsig_sym first_scan_all_blocks
	.addrsig_sym init_zone_after_first_scan
	.addrsig_sym second_scan_all_blocks
	.addrsig_sym print_nftl_zone
	.addrsig_sym panic_backup_block
	.addrsig_sym nand_fdt_get_prop_string_by_path
	.addrsig_sym nand_memcmp
	.addrsig_sym is_panic_data_page
	.addrsig_sym panic_recover_info
	.addrsig_sym torture_block_and_mark
	.addrsig_sym crc_wrong
	.addrsig_sym recover_logic_page_mapping
	.addrsig_sym recover_phy_page_mapping
	.addrsig_sym init_smart_info
	.addrsig_sym cross_talk_rebuild_current_block
	.addrsig_sym no_cross_talk_rebuild_current_block
	.addrsig_sym get_current_used_block
	.addrsig_sym check_bit_nums
	.addrsig_sym current_block_ecc_error
	.addrsig_sym nand_power_off_ecc_error
	.addrsig_sym init_nrc
	.addrsig_sym free_nrc
	.addrsig_sym get_used_page_num
	.addrsig_sym get_used_page_num_no_crosstalk
	.addrsig_sym mark_free_block
	.addrsig_sym nftl_check_pattern
	.addrsig_sym print_smart
	.addrsig_sym page_read
	.addrsig_sym page_write
	.addrsig_sym block_erase
	.addrsig_sym nand_physic_erase_block
	.addrsig_sym clean_block_list
	.addrsig_sym nand_clean_zone_table2
	.addrsig_sym nftl_clean_logic_page_map
	.addrsig_sym nftl_clean_used_list
	.addrsig_sym nftl_clean_invalid_list
	.addrsig_sym nand_clean_zone_table
	.addrsig_sym __nand_flush_write_cache
	.addrsig_sym nftl_cache_exit
	.addrsig_sym nand_find_zone_table
	.addrsig_sym nftl_cache_init
	.addrsig_sym write_data_to_nand
	.addrsig_sym _DumpHexData
	.addrsig_sym clean_zone
	.addrsig_sym search_from_cache_read_list
	.addrsig_sym search_from_cache_write_list
	.addrsig_sym add_to_cache_read_list_tail
	.addrsig_sym add_to_cache_write_list_tail
	.addrsig_sym del_from_cache_read_list
	.addrsig_sym del_from_cache_write_list
	.addrsig_sym get_empty_cahce_node
	.addrsig_sym nand_op
	.addrsig_sym __nand_discard
	.addrsig_sym nand_discard
	.addrsig_sym nand_cache_read
	.addrsig_sym nand_cache_write
	.addrsig_sym flush_write_cache_to_nand
	.addrsig_sym nand_phy_get_page_type
	.addrsig_sym __shutdown_op
	.addrsig_sym get_last_nftl
	.addrsig_sym get_nftl_read_claim_page_no
	.addrsig_sym clean_read_reclaim_complete_flag
	.addrsig_sym set_nftl_read_claim_complete
	.addrsig_sym set_nftl_read_claim_flag
	.addrsig_sym get_cold_logic_page
	.addrsig_sym nftl_get_zone_cap
	.addrsig_sym nftl_get_zone_priv
	.addrsig_sym nftl_start
	.addrsig_sym nftl_zone_release
	.addrsig_sym panic_mark_enable
	.addrsig_sym is_panic_enable
	.addrsig_sym panic_mark_panic
	.addrsig_sym is_panic_dummy_page
	.addrsig_sym panic_recover_data
	.addrsig_sym nand_malloc
	.addrsig_sym nftl_write
	.addrsig_sym nand_free
	.addrsig_sym nftl_add
	.addrsig_sym build_nand_partition
	.addrsig_sym nftl_initialize
	.addrsig_sym add_nftl_blk_list
	.addrsig_sym del_last_nftl_blk
	.addrsig_sym nftl_exit
	.addrsig_sym nftl_support_boot_check_crc_from_dts
	.addrsig_sym nand_get_support_boot_check_crc
	.addrsig_sym nftl_version
	.addrsig_sym nftl_get_nand_phyinfo
	.addrsig_sym _nand_read
	.addrsig_sym _nand_write
	.addrsig_sym _nand_discard
	.addrsig_sym _nand_flush_write_cache
	.addrsig_sym _shutdown_op
	.addrsig_sym _nand_sector_read
	.addrsig_sym _nand_sector_write
	.addrsig_sym _nand_flush_sector_write_cache
	.addrsig_sym free_nand_partition
	.addrsig_sym hynix26nm_vender_set_param
	.addrsig_sym nand_enable_chip
	.addrsig_sym ndfc_disable_randomize
	.addrsig_sym set_cmd_with_nand_bus
	.addrsig_sym set_one_cmd
	.addrsig_sym nand_disable_chip
	.addrsig_sym hynix26nm_set_readretry
	.addrsig_sym hynix26nm_read_page_end
	.addrsig_sym nci_get_from_nsi
	.addrsig_sym generic_read_page_end_not_retry
	.addrsig_sym generic_read_page_start
	.addrsig_sym hynix26nm_vender_get_param
	.addrsig_sym get_data_with_nand_bus_one_cmd
	.addrsig_sym set_one_addr
	.addrsig_sym hynix26nm_getdefaultparam
	.addrsig_sym hynix26nm_setdefaultparam
	.addrsig_sym hynix26nm_get_default_param
	.addrsig_sym hynix26nm_readretry_init
	.addrsig_sym hynix26nm_readretry_exit
	.addrsig_sym samsung_vender_set_param
	.addrsig_sym samsung_set_readretry
	.addrsig_sym samsung_set_readretry_K9GCGD8U0F
	.addrsig_sym samsung_read_page_end
	.addrsig_sym samsung_readretry_init
	.addrsig_sym samsung_readretry_exit
	.addrsig_sym nand_set_read_retry_K9GCGD8U0F
	.addrsig_sym nand_get_read_retry_K9GCGD8U0F
	.addrsig_sym sandisk_read_retry_clock_save
	.addrsig_sym nand_get_clk
	.addrsig_sym nand_set_clk
	.addrsig_sym sandisk_vender_set_param
	.addrsig_sym sandisk_read_retry_clock_recover
	.addrsig_sym sandisk_end_one_read_retry
	.addrsig_sym ndfc_is_toggle_interface
	.addrsig_sym ndfc_set_legacy_interface
	.addrsig_sym ndfc_set_toggle_interface
	.addrsig_sym sandisk_set_readretry
	.addrsig_sym sandisk_read_page_end
	.addrsig_sym sandisk_exit_readretry
	.addrsig_sym sandisk_activation_seq
	.addrsig_sym sandisk_readretry_init
	.addrsig_sym sandisk_readretry_exit
	.addrsig_sym sandisk_A19_dsp_on
	.addrsig_sym sandisk_A19_cmd25
	.addrsig_sym sandisk_A19_set_lmflgfix_next
	.addrsig_sym sandisk_A19_read_page_end
	.addrsig_sym sandisk_A19_set_readretry
	.addrsig_sym sandisk_A19_exit_readretry
	.addrsig_sym sandisk_A19_readretry_init
	.addrsig_sym sandisk_A19_readretry_exit
	.addrsig_sym sandisk_A19_vender_set_param
	.addrsig_sym sandisk_toggle_vendor_specific_setting
	.addrsig_sym nand_read_chip_status_ready
	.addrsig_sym ndfc_clean_cmd_seq
	.addrsig_sym get_row_addr
	.addrsig_sym fill_cmd_addr
	.addrsig_sym ndfc_execute_cmd
	.addrsig_sym hynix20nm_vender_set_param
	.addrsig_sym hynix20nm_set_readretry
	.addrsig_sym hynix20nm_read_page_end
	.addrsig_sym hynix20nm_vender_get_param
	.addrsig_sym hynix20nm_vender_get_param_otp_hynix
	.addrsig_sym nand_get_temp_buf
	.addrsig_sym nand_free_temp_buf
	.addrsig_sym hynix20nm_saveotpvalue
	.addrsig_sym hynix20nm_getotpparam
	.addrsig_sym hynix20nm_set_default_param
	.addrsig_sym hynix20nm_init_readretry_param
	.addrsig_sym hynix16nm_get_special_info
	.addrsig_sym hynix20nm_readretry_init
	.addrsig_sym hynix20nm_readretry_exit
	.addrsig_sym micron_intel_set_feature
	.addrsig_sym nand_set_feature
	.addrsig_sym nand_get_feature
	.addrsig_sym micron_set_readretry
	.addrsig_sym micron_read_page_end
	.addrsig_sym micron_readretry_init
	.addrsig_sym micron_readretry_exit
	.addrsig_sym hynix16nm_vender_set_param
	.addrsig_sym hynix16nm_set_readretry
	.addrsig_sym physic_info_get_offset
	.addrsig_sym physic_info_get_one_copy
	.addrsig_sym hynix16nm_read_page_end
	.addrsig_sym ndfc_disable_ecc
	.addrsig_sym hynix16nm_major_check_byte
	.addrsig_sym hynix16nm_get_read_retry_cfg
	.addrsig_sym hynix16nm_read_otp_info_hynix
	.addrsig_sym hynix16nm_get_rr_value_otp_hynix
	.addrsig_sym hynix16nm_save_otp_value
	.addrsig_sym hynix16nm_get_otp_param
	.addrsig_sym hynix16nm_set_default_param
	.addrsig_sym hynix16nm_init_readretry_param
	.addrsig_sym hynix16nm_readretry_init
	.addrsig_sym hynix16nm_readretry_exit
	.addrsig_sym toshiba_read_retry_clock_save
	.addrsig_sym toshiba_read_retry_clock_recover
	.addrsig_sym toshiba_vender_set_param
	.addrsig_sym toshiba_vender_pre_condition
	.addrsig_sym toshiba_set_readretry
	.addrsig_sym toshiba_read_page_end
	.addrsig_sym toshiba_exit_readretry
	.addrsig_sym toshiba_readretry_init
	.addrsig_sym toshiba_readretry_exit
	.addrsig_sym com
	.addrsig_sym mod
	.addrsig_sym gc_src_type
	.addrsig_sym phyinfo
	.addrsig_sym g_nreg_base
	.addrsig_sym patterns
	.addrsig_sym g_nsi
	.addrsig_sym nand_on_panic
	.addrsig_sym panic_enable
	.addrsig_sym panic_part
	.addrsig_sym panic_info
	.addrsig_sym nftl_blk_head
	.addrsig_sym hynix26nm_read_retry_mode
	.addrsig_sym hynix26nm_read_retry_cycle
	.addrsig_sym hynix26nm_read_retry_reg_num
	.addrsig_sym hynix26nm_read_retry_reg_adr
	.addrsig_sym hynix26nm_read_retry_val
	.addrsig_sym hynix26nm_lsb_mode_reg_adr
	.addrsig_sym hynix26nm_lsb_mode_default_val
	.addrsig_sym hynix26nm_lsb_mode_val
	.addrsig_sym hynix26nm_lsb_mode_reg_num
	.addrsig_sym para0
	.addrsig_sym para1
	.addrsig_sym df_read_page_end
	.addrsig_sym samsung_read_retry_mode
	.addrsig_sym samsung_read_retry_cycle
	.addrsig_sym samsung_read_retry_addr
	.addrsig_sym samsung_read_retry_cmd
	.addrsig_sym samsung_p1
	.addrsig_sym sandisk_read_retry_mode
	.addrsig_sym sandisk_read_retry_cycle
	.addrsig_sym sandisk_read_retry_reg_cnt
	.addrsig_sym sandisk_acti_start_cmd
	.addrsig_sym sandisk_acti_start_addr
	.addrsig_sym sandisk_read_retry_disable_cmd_19
	.addrsig_sym sandisk_read_retry_cmd_19
	.addrsig_sym sandisk_read_retry_reg_adr_19
	.addrsig_sym sandisk_19_low
	.addrsig_sym sandisk_19_high
	.addrsig_sym sandisk_read_retry_reg_adr_24
	.addrsig_sym sandisk_24
	.addrsig_sym sandisk_read_retry_cmd_1y
	.addrsig_sym sandisk_read_retry_reg_adr_1y
	.addrsig_sym sandisk_1y
	.addrsig_sym aw_ndfc
	.addrsig_sym sandisk_sclk0_bak
	.addrsig_sym sandisk_sclk1_bak
	.addrsig_sym sandisk_toggle_mode_flag
	.addrsig_sym sandisk_A19_read_retry_mode
	.addrsig_sym sandisk_A19_read_retry_cycle
	.addrsig_sym sandisk_A19_read_retry_reg_cnt
	.addrsig_sym sandisk_A19_read_retry_cmd_1y
	.addrsig_sym sandisk_A19_read_retry_reg_adr_1y
	.addrsig_sym sandisk_A19_1y
	.addrsig_sym sandisk_A19_1z_16g
	.addrsig_sym sandisk_A19_1z_8g
	.addrsig_sym sandisk_A19_sclk0_bak
	.addrsig_sym sandisk_A19_sclk1_bak
	.addrsig_sym hynix20nm_read_retry_mode
	.addrsig_sym hynix20nm_read_retry_cycle
	.addrsig_sym hynix20nm_read_retry_reg_num
	.addrsig_sym hynix20nm_read_retry_reg_adr
	.addrsig_sym hynix20nm_lsb_mode_reg_adr
	.addrsig_sym hynix20nm_lsb_mode_default_val
	.addrsig_sym hynix20nm_lsb_mode_val
	.addrsig_sym hynix20nm_lsb_mode_reg_num
	.addrsig_sym micron_read_retry_mode
	.addrsig_sym micron_read_retry_cycle
	.addrsig_sym micron_p1_1
	.addrsig_sym micron_p1_2
	.addrsig_sym micron_p1_3
	.addrsig_sym micron_p1_4
	.addrsig_sym micron_p1
	.addrsig_sym hynix16nm_read_retry_mode
	.addrsig_sym hynix16nm_read_retry_cycle
	.addrsig_sym hynix16nm_read_retry_reg_num
	.addrsig_sym hynix16nm_read_retry_reg_adr
	.addrsig_sym toshiba_read_retry_mode
	.addrsig_sym toshiba_read_retry_cycle
	.addrsig_sym toshiba_read_retry_reg_cnt
	.addrsig_sym toshiba_read_retry_start_cmd
	.addrsig_sym toshiba_read_retry_end_cmd
	.addrsig_sym toshiba_read_retry_cmd_1
	.addrsig_sym toshiba_read_retry_addr_1
	.addrsig_sym toshiba_p1_1
	.addrsig_sym toshiba_read_retry_cmd_2
	.addrsig_sym toshiba_read_retry_addr_2
	.addrsig_sym toshiba_p1_2
	.addrsig_sym toshiba_p1_3
	.addrsig_sym toshiba_read_retry_last_cmd
	.addrsig_sym toshiba_read_retry_exit_cmd
	.addrsig_sym toshiba_sclk0_bak
	.addrsig_sym toshiba_sclk1_bak
	.addrsig_sym toshiba_retry_flag
	.addrsig_sym toshiba_toggle_mode_flag
	.addrsig_sym nand_crc16_table

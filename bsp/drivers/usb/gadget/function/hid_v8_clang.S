/*SPDX-Licensen-Identifier: GPL-2.0*/

	.text
	.file	"libhid.c"
	.globl	libhid_platform_verify          // -- Begin function libhid_platform_verify
	.p2align	2
	.type	libhid_platform_verify,@function
libhid_platform_verify:                 // @libhid_platform_verify
	.cfi_startproc
// %bb.0:
	stp	x29, x30, [sp, #-16]!           // 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	sunxi_get_soc_markid
	adrp	x8, magic
	str	w0, [x8, :lo12:magic]
	ldp	x29, x30, [sp], #16             // 16-byte Folded Reload
	ret
.Lfunc_end0:
	.size	libhid_platform_verify, .Lfunc_end0-libhid_platform_verify
	.cfi_endproc
                                        // -- End function
	.globl	libhid_set_input_range          // -- Begin function libhid_set_input_range
	.p2align	2
	.type	libhid_set_input_range,@function
libhid_set_input_range:                 // @libhid_set_input_range
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	adrp	x9, desc
	add	x9, x9, :lo12:desc
	str	w0, [sp, #12]
	str	w1, [sp, #8]
	str	w2, [sp, #4]
	str	w3, [sp]
	ldr	w8, [sp, #12]
	str	w8, [x9, #260]
	ldr	w8, [sp, #8]
	str	w8, [x9, #264]
	ldr	w8, [sp, #4]
	str	w8, [x9, #268]
	ldr	w8, [sp]
	str	w8, [x9, #272]
	add	sp, sp, #16
	ret
.Lfunc_end1:
	.size	libhid_set_input_range, .Lfunc_end1-libhid_set_input_range
	.cfi_endproc
                                        // -- End function
	.globl	libhid_handle_abs_event         // -- Begin function libhid_handle_abs_event
	.p2align	2
	.type	libhid_handle_abs_event,@function
libhid_handle_abs_event:                // @libhid_handle_abs_event
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             // 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, info
	add	x8, x8, :lo12:info
	str	x8, [sp, #16]                   // 8-byte Folded Spill
	adrp	x8, desc
	add	x8, x8, :lo12:desc
	add	x9, x8, #68
	str	x9, [sp, #24]                   // 8-byte Folded Spill
	add	x8, x8, #4
	stur	x8, [x29, #-16]                 // 8-byte Folded Spill
	stur	w0, [x29, #-4]
	stur	w1, [x29, #-8]
	bl	check_magic
	tbnz	w0, #0, .LBB2_2
	b	.LBB2_1
.LBB2_1:
	b	.LBB2_11
.LBB2_2:
	ldur	w8, [x29, #-4]
	subs	w8, w8, #48
                                        // kill: def $x8 killed $w8
	str	x8, [sp, #8]                    // 8-byte Folded Spill
	subs	x8, x8, #10
	b.hi	.LBB2_11
// %bb.3:
	ldr	x11, [sp, #8]                   // 8-byte Folded Reload
	adrp	x10, .LJTI2_0
	add	x10, x10, :lo12:.LJTI2_0
.Ltmp1:
	adr	x8, .Ltmp1
	ldrsw	x9, [x10, x11, lsl #2]
	add	x8, x8, x9
	br	x8
.LBB2_4:
	ldur	w8, [x29, #-8]
	subs	w8, w8, #16
	b.lt	.LBB2_6
	b	.LBB2_5
.LBB2_5:
	b	.LBB2_11
.LBB2_6:
	ldur	w8, [x29, #-8]
	adrp	x9, info
	str	w8, [x9, :lo12:info]
	adrp	x9, desc
	str	w8, [x9, :lo12:desc]
	b	.LBB2_11
.LBB2_7:
	ldur	x9, [x29, #-16]                 // 8-byte Folded Reload
	ldr	x10, [sp, #16]                  // 8-byte Folded Reload
	ldur	w8, [x29, #-8]
	str	w8, [x10, #4]
	adrp	x10, desc
	ldrsw	x10, [x10, :lo12:desc]
	str	w8, [x9, x10, lsl #2]
	b	.LBB2_11
.LBB2_8:
	ldr	x9, [sp, #24]                   // 8-byte Folded Reload
	ldr	x10, [sp, #16]                  // 8-byte Folded Reload
	ldur	w8, [x29, #-8]
	str	w8, [x10, #8]
	adrp	x10, desc
	ldrsw	x10, [x10, :lo12:desc]
	str	w8, [x9, x10, lsl #2]
	b	.LBB2_11
.LBB2_9:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	ldur	w8, [x29, #-8]
	str	w8, [x9, #12]
	b	.LBB2_11
.LBB2_10:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	ldur	w8, [x29, #-8]
	str	w8, [x9, #16]
	b	.LBB2_11
.LBB2_11:
	ldp	x29, x30, [sp, #48]             // 16-byte Folded Reload
	add	sp, sp, #64
	ret
.Lfunc_end2:
	.size	libhid_handle_abs_event, .Lfunc_end2-libhid_handle_abs_event
	.cfi_endproc
	.section	.rodata,"a",@progbits
	.p2align	2
.LJTI2_0:
	.word	.LBB2_9-.Ltmp1
	.word	.LBB2_11-.Ltmp1
	.word	.LBB2_11-.Ltmp1
	.word	.LBB2_11-.Ltmp1
	.word	.LBB2_11-.Ltmp1
	.word	.LBB2_7-.Ltmp1
	.word	.LBB2_8-.Ltmp1
	.word	.LBB2_11-.Ltmp1
	.word	.LBB2_11-.Ltmp1
	.word	.LBB2_4-.Ltmp1
	.word	.LBB2_10-.Ltmp1
                                        // -- End function
	.text
	.p2align	2                               // -- Begin function check_magic
	.type	check_magic,@function
check_magic:                            // @check_magic
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	adrp	x8, magic
	ldr	w9, [x8, :lo12:magic]
	mov	w8, #1
	subs	w9, w9, #4, lsl #12             // =16384
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	b.eq	.LBB3_4
	b	.LBB3_1
.LBB3_1:
	adrp	x8, magic
	ldr	w9, [x8, :lo12:magic]
	mov	w10, #20992
	mov	w8, #1
	subs	w9, w9, w10
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	b.eq	.LBB3_4
	b	.LBB3_2
.LBB3_2:
	adrp	x8, magic
	ldr	w9, [x8, :lo12:magic]
	mov	w10, #32512
	mov	w8, #1
	subs	w9, w9, w10
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	b.eq	.LBB3_4
	b	.LBB3_3
.LBB3_3:
	adrp	x8, magic
	ldr	w8, [x8, :lo12:magic]
	mov	w9, #65296
	subs	w8, w8, w9
	cset	w8, eq
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	b	.LBB3_4
.LBB3_4:
	ldr	w8, [sp, #12]                   // 4-byte Folded Reload
	and	w0, w8, #0x1
	add	sp, sp, #16
	ret
.Lfunc_end3:
	.size	check_magic, .Lfunc_end3-check_magic
	.cfi_endproc
                                        // -- End function
	.globl	libhid_handle_key_event         // -- Begin function libhid_handle_key_event
	.p2align	2
	.type	libhid_handle_key_event,@function
libhid_handle_key_event:                // @libhid_handle_key_event
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #48
	stp	x29, x30, [sp, #32]             // 16-byte Folded Spill
	add	x29, sp, #32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, info
	add	x8, x8, :lo12:info
	str	x8, [sp, #16]                   // 8-byte Folded Spill
	stur	w0, [x29, #-4]
	stur	w1, [x29, #-8]
	bl	check_magic
	tbnz	w0, #0, .LBB4_2
	b	.LBB4_1
.LBB4_1:
	b	.LBB4_8
.LBB4_2:
	ldur	w8, [x29, #-4]
	str	w8, [sp, #12]                   // 4-byte Folded Spill
	subs	w8, w8, #320
	b.eq	.LBB4_7
	b	.LBB4_3
.LBB4_3:
	ldr	w8, [sp, #12]                   // 4-byte Folded Reload
	subs	w8, w8, #325
	b.eq	.LBB4_6
	b	.LBB4_4
.LBB4_4:
	ldr	w8, [sp, #12]                   // 4-byte Folded Reload
	subs	w8, w8, #330
	b.ne	.LBB4_8
	b	.LBB4_5
.LBB4_5:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	ldur	w8, [x29, #-8]
	str	w8, [x9, #24]
	b	.LBB4_8
.LBB4_6:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	mov	w8, #2
	str	w8, [x9, #20]
	b	.LBB4_8
.LBB4_7:
	ldr	x9, [sp, #16]                   // 8-byte Folded Reload
	mov	w8, #1
	str	w8, [x9, #20]
	b	.LBB4_8
.LBB4_8:
	ldp	x29, x30, [sp, #32]             // 16-byte Folded Reload
	add	sp, sp, #48
	ret
.Lfunc_end4:
	.size	libhid_handle_key_event, .Lfunc_end4-libhid_handle_key_event
	.cfi_endproc
                                        // -- End function
	.globl	libhid_handle_syn_event         // -- Begin function libhid_handle_syn_event
	.p2align	2
	.type	libhid_handle_syn_event,@function
libhid_handle_syn_event:                // @libhid_handle_syn_event
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             // 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	w0, [x29, #-4]
	str	w1, [sp, #8]
	bl	check_magic
	tbnz	w0, #0, .LBB5_2
	b	.LBB5_1
.LBB5_1:
	b	.LBB5_6
.LBB5_2:
	ldur	w8, [x29, #-4]
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	cbz	w8, .LBB5_5
	b	.LBB5_3
.LBB5_3:
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	subs	w8, w8, #2
	b.ne	.LBB5_6
	b	.LBB5_4
.LBB5_4:
	bl	hid_parse_mt_info
	b	.LBB5_6
.LBB5_5:
	bl	hid_send_mt_report
	b	.LBB5_6
.LBB5_6:
	ldp	x29, x30, [sp, #16]             // 16-byte Folded Reload
	add	sp, sp, #32
	ret
.Lfunc_end5:
	.size	libhid_handle_syn_event, .Lfunc_end5-libhid_handle_syn_event
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function hid_parse_mt_info
	.type	hid_parse_mt_info,@function
hid_parse_mt_info:                      // @hid_parse_mt_info
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #80
	.cfi_def_cfa_offset 80
	adrp	x9, info
	adrp	x8, info
	add	x8, x8, :lo12:info
	str	x8, [sp, #8]                    // 8-byte Folded Spill
	adrp	x10, mt_report
	add	x10, x10, :lo12:mt_report
	str	x10, [sp, #16]                  // 8-byte Folded Spill
	add	x11, x10, #3
	str	x11, [sp, #24]                  // 8-byte Folded Spill
	add	x11, x10, #19
	str	x11, [sp, #32]                  // 8-byte Folded Spill
	adrp	x11, desc
	add	x11, x11, :lo12:desc
	str	x11, [sp, #40]                  // 8-byte Folded Spill
	add	x11, x10, #36
	str	x11, [sp, #48]                  // 8-byte Folded Spill
	add	x11, x10, #68
	str	x11, [sp, #56]                  // 8-byte Folded Spill
	add	x10, x10, #100
	str	x10, [sp, #64]                  // 8-byte Folded Spill
	ldr	w8, [x8, #36]
	ldr	w9, [x9, :lo12:info]
	subs	w8, w8, w9
	b.ge	.LBB6_2
	b	.LBB6_1
.LBB6_1:
	ldr	x8, [sp, #8]                    // 8-byte Folded Reload
	ldr	w8, [x8, #36]
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	b	.LBB6_3
.LBB6_2:
	adrp	x8, info
	ldr	w8, [x8, :lo12:info]
	str	w8, [sp, #4]                    // 4-byte Folded Spill
	b	.LBB6_3
.LBB6_3:
	ldr	w8, [sp, #4]                    // 4-byte Folded Reload
	str	w8, [sp, #76]
	ldr	w8, [sp, #76]
	cbz	w8, .LBB6_5
	b	.LBB6_4
.LBB6_4:
	adrp	x8, info
	ldr	w8, [x8, :lo12:info]
	subs	w8, w8, #1
	str	w8, [sp]                        // 4-byte Folded Spill
	b	.LBB6_6
.LBB6_5:
	adrp	x8, info
	ldr	w8, [x8, :lo12:info]
	str	w8, [sp]                        // 4-byte Folded Spill
	b	.LBB6_6
.LBB6_6:
	ldr	w8, [sp]                        // 4-byte Folded Reload
	str	w8, [sp, #72]
	ldr	w8, [sp, #72]
	subs	w8, w8, #6
	b.lo	.LBB6_8
	b	.LBB6_7
.LBB6_7:
	b	.LBB6_9
.LBB6_8:
	ldr	x9, [sp, #64]                   // 8-byte Folded Reload
	ldr	x8, [sp, #8]                    // 8-byte Folded Reload
	ldr	x11, [sp, #56]                  // 8-byte Folded Reload
	ldr	x13, [sp, #40]                  // 8-byte Folded Reload
	ldr	x12, [sp, #48]                  // 8-byte Folded Reload
	ldr	x14, [sp, #32]                  // 8-byte Folded Reload
	ldr	x15, [sp, #24]                  // 8-byte Folded Reload
	ldr	x16, [sp, #16]                  // 8-byte Folded Reload
	ldr	w10, [sp, #76]
	str	w10, [x8, #36]
	mov	w10, #6
	strb	w10, [x16, #1]
	ldr	w10, [sp, #72]
	add	w10, w10, #1
	strb	w10, [x16, #2]
	ldr	w16, [x8, #24]
	mov	w10, wzr
	subs	w16, w16, #0
	csinc	w10, w10, wzr, eq
	ldr	w16, [sp, #72]
                                        // kill: def $x16 killed $w16
	strb	w10, [x15, x16]
	ldr	w10, [sp, #72]
	ldr	w15, [sp, #72]
                                        // kill: def $x15 killed $w15
	strb	w10, [x14, x15]
	ldr	w10, [x8, #4]
	adrp	x14, gadget_x_logical_maximum
	ldr	w14, [x14, :lo12:gadget_x_logical_maximum]
	mul	w10, w10, w14
	ldr	w14, [x13, #264]
	ldr	w15, [x13, #260]
	subs	w14, w14, w15
	sdiv	w10, w10, w14
	ldr	w14, [sp, #72]
                                        // kill: def $x14 killed $w14
	strh	w10, [x12, x14, lsl #1]
	ldr	w10, [x8, #8]
	adrp	x12, gadget_y_logical_maximum
	ldr	w12, [x12, :lo12:gadget_y_logical_maximum]
	mul	w10, w10, w12
	ldr	w12, [x13, #272]
	ldr	w13, [x13, #268]
	subs	w12, w12, w13
	sdiv	w10, w10, w12
	ldr	w12, [sp, #72]
                                        // kill: def $x12 killed $w12
	strh	w10, [x11, x12, lsl #1]
	ldr	w8, [x8, #12]
	ldr	w10, [sp, #72]
                                        // kill: def $x10 killed $w10
	strh	w8, [x9, x10, lsl #1]
	b	.LBB6_9
.LBB6_9:
	add	sp, sp, #80
	ret
.Lfunc_end6:
	.size	hid_parse_mt_info, .Lfunc_end6-hid_parse_mt_info
	.cfi_endproc
                                        // -- End function
	.p2align	2                               // -- Begin function hid_send_mt_report
	.type	hid_send_mt_report,@function
hid_send_mt_report:                     // @hid_send_mt_report
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #160
	stp	x29, x30, [sp, #144]            // 16-byte Folded Spill
	add	x29, sp, #144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, info
	add	x8, x8, :lo12:info
	adrp	x9, mt_report
	add	x9, x9, :lo12:mt_report
	str	x9, [sp, #32]                   // 8-byte Folded Spill
	add	x10, x9, #3
	str	x10, [sp, #40]                  // 8-byte Folded Spill
	add	x10, x9, #19
	str	x10, [sp, #48]                  // 8-byte Folded Spill
	add	x10, x9, #36
	str	x10, [sp, #56]                  // 8-byte Folded Spill
	add	x10, x9, #68
	str	x10, [sp, #64]                  // 8-byte Folded Spill
	add	x9, x9, #100
	str	x9, [sp, #72]                   // 8-byte Folded Spill
	mov	w1, wzr
	stur	wzr, [x29, #-4]
	ldr	w8, [x8, #24]
	subs	w8, w8, #0
	csinc	w8, w1, wzr, eq
	stur	w8, [x29, #-8]
	sub	x0, x29, #60
	mov	x2, #52
	bl	memset
	adrp	x8, report_id
	ldr	w8, [x8, :lo12:report_id]
	sturb	w8, [x29, #-60]
	stur	wzr, [x29, #-4]
	b	.LBB7_1
.LBB7_1:                                // =>This Inner Loop Header: Depth=1
	ldr	x9, [sp, #32]                   // 8-byte Folded Reload
	ldur	w8, [x29, #-4]
	ldrb	w9, [x9, #2]
	subs	w8, w8, w9
	b.ge	.LBB7_4
	b	.LBB7_2
.LBB7_2:                                //   in Loop: Header=BB7_1 Depth=1
	ldr	x8, [sp, #72]                   // 8-byte Folded Reload
	ldr	x11, [sp, #64]                  // 8-byte Folded Reload
	ldr	x12, [sp, #56]                  // 8-byte Folded Reload
	ldr	x13, [sp, #48]                  // 8-byte Folded Reload
	ldr	x9, [sp, #40]                   // 8-byte Folded Reload
	ldur	w14, [x29, #-8]
	ldursw	x10, [x29, #-4]
	strb	w14, [x9, x10]
	ldur	w9, [x29, #-4]
	mov	w10, #8
	str	w10, [sp, #20]                  // 4-byte Folded Spill
	mul	w9, w10, w9
	add	w15, w9, #1
	sub	x9, x29, #60
	str	x9, [sp, #24]                   // 8-byte Folded Spill
	strb	w14, [x9, w15, sxtw]
	ldursw	x14, [x29, #-4]
	add	x13, x13, x14
	ldrb	w13, [x13]
	ldur	w14, [x29, #-4]
	mul	w14, w10, w14
	add	w14, w14, #2
	add	x14, x9, w14, sxtw
	strb	w13, [x14]
	ldursw	x13, [x29, #-4]
	lsl	x13, x13, #1
	ldrb	w13, [x12, x13]
	ldur	w14, [x29, #-4]
	mul	w14, w10, w14
	add	w14, w14, #3
	strb	w13, [x9, w14, sxtw]
	ldursw	x13, [x29, #-4]
	ldrh	w12, [x12, x13, lsl #1]
	asr	w12, w12, #8
	and	w12, w12, #0x1f
	ldur	w13, [x29, #-4]
	mul	w13, w10, w13
	add	w13, w13, #4
	strb	w12, [x9, w13, sxtw]
	ldursw	x12, [x29, #-4]
	lsl	x12, x12, #1
	ldrb	w12, [x11, x12]
	ldur	w13, [x29, #-4]
	mul	w13, w10, w13
	add	w13, w13, #5
	strb	w12, [x9, w13, sxtw]
	ldursw	x12, [x29, #-4]
	ldrh	w11, [x11, x12, lsl #1]
	asr	w11, w11, #8
	and	w11, w11, #0x1f
	ldur	w12, [x29, #-4]
	mul	w12, w10, w12
	add	w12, w12, #6
	strb	w11, [x9, w12, sxtw]
	ldursw	x11, [x29, #-4]
	lsl	x11, x11, #1
	ldrb	w11, [x8, x11]
	ldur	w12, [x29, #-4]
	mul	w12, w10, w12
	add	w12, w12, #7
	strb	w11, [x9, w12, sxtw]
	ldursw	x11, [x29, #-4]
	ldrh	w8, [x8, x11, lsl #1]
	asr	w8, w8, #8
	and	w8, w8, #0x1f
	ldur	w11, [x29, #-4]
	mul	w10, w10, w11
	add	w10, w10, #8
	strb	w8, [x9, w10, sxtw]
	b	.LBB7_3
.LBB7_3:                                //   in Loop: Header=BB7_1 Depth=1
	ldur	w8, [x29, #-4]
	add	w8, w8, #1
	stur	w8, [x29, #-4]
	b	.LBB7_1
.LBB7_4:
	ldr	x8, [sp, #32]                   // 8-byte Folded Reload
	mov	w9, #1
	sturb	w9, [x29, #-10]
	sturb	w9, [x29, #-11]
	ldrb	w9, [x8, #2]
	sturb	w9, [x29, #-9]
	ldrb	w8, [x8, #2]
	adrp	x9, mt_report
	ldrb	w9, [x9, :lo12:mt_report]
	subs	w8, w8, w9
	b.ge	.LBB7_12
	b	.LBB7_5
.LBB7_5:
	b	.LBB7_6
.LBB7_6:                                // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-4]
	adrp	x9, mt_report
	ldrb	w9, [x9, :lo12:mt_report]
	subs	w8, w8, w9
	b.ge	.LBB7_11
	b	.LBB7_7
.LBB7_7:                                //   in Loop: Header=BB7_6 Depth=1
	ldr	x8, [sp, #40]                   // 8-byte Folded Reload
	ldursw	x9, [x29, #-4]
	ldrb	w8, [x8, x9]
	subs	w8, w8, #1
	b.ne	.LBB7_9
	b	.LBB7_8
.LBB7_8:                                //   in Loop: Header=BB7_6 Depth=1
	ldr	x8, [sp, #72]                   // 8-byte Folded Reload
	ldr	x11, [sp, #64]                  // 8-byte Folded Reload
	ldr	x12, [sp, #56]                  // 8-byte Folded Reload
	ldr	x13, [sp, #48]                  // 8-byte Folded Reload
	ldr	x9, [sp, #40]                   // 8-byte Folded Reload
	ldursw	x10, [x29, #-4]
	add	x9, x9, x10
	strb	wzr, [x9]
	ldur	w9, [x29, #-4]
	mov	w10, #8
	str	w10, [sp, #4]                   // 4-byte Folded Spill
	mul	w9, w10, w9
	add	w14, w9, #1
	sub	x9, x29, #60
	str	x9, [sp, #8]                    // 8-byte Folded Spill
	add	x14, x9, w14, sxtw
	strb	wzr, [x14]
	ldursw	x14, [x29, #-4]
	add	x13, x13, x14
	ldrb	w13, [x13]
	ldur	w14, [x29, #-4]
	mul	w14, w10, w14
	add	w14, w14, #2
	add	x14, x9, w14, sxtw
	strb	w13, [x14]
	ldursw	x13, [x29, #-4]
	lsl	x13, x13, #1
	ldrb	w13, [x12, x13]
	ldur	w14, [x29, #-4]
	mul	w14, w10, w14
	add	w14, w14, #3
	strb	w13, [x9, w14, sxtw]
	ldursw	x13, [x29, #-4]
	ldrh	w12, [x12, x13, lsl #1]
	asr	w12, w12, #8
	and	w12, w12, #0x1f
	ldur	w13, [x29, #-4]
	mul	w13, w10, w13
	add	w13, w13, #4
	strb	w12, [x9, w13, sxtw]
	ldursw	x12, [x29, #-4]
	lsl	x12, x12, #1
	ldrb	w12, [x11, x12]
	ldur	w13, [x29, #-4]
	mul	w13, w10, w13
	add	w13, w13, #5
	strb	w12, [x9, w13, sxtw]
	ldursw	x12, [x29, #-4]
	ldrh	w11, [x11, x12, lsl #1]
	asr	w11, w11, #8
	and	w11, w11, #0x1f
	ldur	w12, [x29, #-4]
	mul	w12, w10, w12
	add	w12, w12, #6
	strb	w11, [x9, w12, sxtw]
	ldursw	x11, [x29, #-4]
	lsl	x11, x11, #1
	ldrb	w11, [x8, x11]
	ldur	w12, [x29, #-4]
	mul	w12, w10, w12
	add	w12, w12, #7
	strb	w11, [x9, w12, sxtw]
	ldursw	x11, [x29, #-4]
	ldrh	w8, [x8, x11, lsl #1]
	asr	w8, w8, #8
	and	w8, w8, #0x1f
	ldur	w11, [x29, #-4]
	mul	w10, w10, w11
	add	w10, w10, #8
	strb	w8, [x9, w10, sxtw]
	ldurb	w8, [x29, #-9]
	add	w8, w8, #1
	sturb	w8, [x29, #-9]
	b	.LBB7_9
.LBB7_9:                                //   in Loop: Header=BB7_6 Depth=1
	b	.LBB7_10
.LBB7_10:                               //   in Loop: Header=BB7_6 Depth=1
	ldur	w8, [x29, #-4]
	add	w8, w8, #1
	stur	w8, [x29, #-4]
	b	.LBB7_6
.LBB7_11:
	b	.LBB7_12
.LBB7_12:
	ldr	x8, [sp, #32]                   // 8-byte Folded Reload
	ldrb	w8, [x8, #2]
	adrp	x9, mt_report
	strb	w8, [x9, :lo12:mt_report]
	stur	wzr, [x29, #-4]
	b	.LBB7_13
.LBB7_13:                               // =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-4]
	subs	w8, w8, #52
	b.ge	.LBB7_16
	b	.LBB7_14
.LBB7_14:                               //   in Loop: Header=BB7_13 Depth=1
	ldursw	x9, [x29, #-4]
	sub	x8, x29, #60
	add	x8, x8, x9
	ldrb	w8, [x8]
	ldursw	x10, [x29, #-4]
	adrp	x9, g_hid_data
	add	x9, x9, :lo12:g_hid_data
	add	x9, x9, x10
	strb	w8, [x9]
	b	.LBB7_15
.LBB7_15:                               //   in Loop: Header=BB7_13 Depth=1
	ldur	w8, [x29, #-4]
	add	w8, w8, #1
	stur	w8, [x29, #-4]
	b	.LBB7_13
.LBB7_16:
	ldp	x29, x30, [sp, #144]            // 16-byte Folded Reload
	add	sp, sp, #160
	ret
.Lfunc_end7:
	.size	hid_send_mt_report, .Lfunc_end7-hid_send_mt_report
	.cfi_endproc
                                        // -- End function
	.type	.L.str,@object                  // @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"HID LIB Version (v1.0.0 2023-12-07 12:00)"
	.size	.L.str, 42

	.type	libhid_version,@object          // @libhid_version
	.data
	.globl	libhid_version
	.p2align	3
libhid_version:
	.xword	.L.str
	.size	libhid_version, 8

	.type	hidg_func_desc,@object          // @hidg_func_desc
	.globl	hidg_func_desc
	.p2align	1
hidg_func_desc:
	.byte	0                               // 0x0
	.byte	0                               // 0x0
	.hword	52                              // 0x34
	.hword	426                             // 0x1aa
	.ascii	"\005\r\t\004\241\001\205\001\t\"\241\002\tB\025\000%\001u\001\225\001\201\002\225\007\201\001u\b\tQ\225\001\201\002\005\001&\000\020u\020U\016e\021\t05\000Fy\b\201\002&\000\020FL\005\t1\201\002\005\r\tH\201\002\300\t\"\241\002\tB\025\000%\001u\001\225\001\201\002\225\007\201\001u\b\tQ\225\001\201\002\005\001&\000\020u\020U\016e\021\t05\000Fy\b\201\002&\000\020FL\005\t1\201\002\005\r\tH\201\002\300\t\"\241\002\tB\025\000%\001u\001\225\001\201\002\225\007\201\001u\b\tQ\225\001\201\002\005\001&\000\020u\020U\016e\021\t05\000Fy\b\201\002&\000\020FL\005\t1\201\002\005\r\tH\201\002\300\t\"\241\002\tB\025\000%\001u\001\225\001\201\002\225\007\201\001u\b\tQ\225\001\201\002\005\001&\000\020u\020U\016e\021\t05\000Fy\b\201\002&\000\020FL\005\t1\201\002\005\r\tH\201\002\300\t\"\241\002\tB\025\000%\001u\001\225\001\201\002\225\007\201\001u\b\tQ\225\001\201\002\005\001&\000\020u\020U\016e\021\t05\000Fy\b\201\002&\000\020FL\005\t1\201\002\005\r\tH\201\002\300\t\"\241\002\tB\025\000%\001u\001\225\001\201\002\225\007\201\001u\b\tQ\225\001\201\002\005\001&\000\020u\020U\016e\021\t05\000Fy\b\201\002&\000\020FL\005\t1\201\002\005\r\tH\201\002\300\005\r'\377\377\000\000u\020\225\001\tV\201\002\tT\025\000%\177\225\001u\b\201\002\300"
	.size	hidg_func_desc, 432

	.type	info,@object                    // @info
	.globl	info
	.p2align	2
info:
	.word	0                               // 0x0
	.word	0                               // 0x0
	.word	0                               // 0x0
	.word	0                               // 0x0
	.word	0                               // 0x0
	.word	0                               // 0x0
	.word	0                               // 0x0
	.word	720                             // 0x2d0
	.word	1080                            // 0x438
	.word	6                               // 0x6
	.word	16                              // 0x10
	.word	1024                            // 0x400
	.word	1024                            // 0x400
	.size	info, 52

	.type	magic,@object                   // @magic
	.local	magic
	.comm	magic,4,4
	.type	desc,@object                    // @desc
	.bss
	.globl	desc
	.p2align	2
desc:
	.zero	276
	.size	desc, 276

	.type	g_hid_data,@object              // @g_hid_data
	.globl	g_hid_data
g_hid_data:
	.zero	52
	.size	g_hid_data, 52

	.type	st_report,@object               // @st_report
	.globl	st_report
	.p2align	1
st_report:
	.zero	14
	.size	st_report, 14

	.type	mt_report,@object               // @mt_report
	.globl	mt_report
	.p2align	1
mt_report:
	.zero	132
	.size	mt_report, 132

	.type	gadget_x_logical_maximum,@object // @gadget_x_logical_maximum
	.data
	.p2align	2
gadget_x_logical_maximum:
	.word	4096                            // 0x1000
	.size	gadget_x_logical_maximum, 4

	.type	gadget_y_logical_maximum,@object // @gadget_y_logical_maximum
	.p2align	2
gadget_y_logical_maximum:
	.word	4096                            // 0x1000
	.size	gadget_y_logical_maximum, 4

	.type	report_id,@object               // @report_id
	.p2align	2
report_id:
	.word	1                               // 0x1
	.size	report_id, 4

	.weak	sunxi_get_soc_markid
	.ident	"Android (8490178, based on r450784d) clang version 14.0.6 (https://android.googlesource.com/toolchain/llvm-project 4c603efb0cca074e9238af8b4106c30add4418f6)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym sunxi_get_soc_markid
	.addrsig_sym check_magic
	.addrsig_sym hid_parse_mt_info
	.addrsig_sym hid_send_mt_report
	.addrsig_sym info
	.addrsig_sym magic
	.addrsig_sym desc
	.addrsig_sym g_hid_data
	.addrsig_sym mt_report
	.addrsig_sym gadget_x_logical_maximum
	.addrsig_sym gadget_y_logical_maximum
	.addrsig_sym report_id

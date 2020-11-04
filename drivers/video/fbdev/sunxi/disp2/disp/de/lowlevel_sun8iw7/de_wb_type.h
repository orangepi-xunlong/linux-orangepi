/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *	All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *	File name   :       de_wb_type.h
 *
 *	Description :       display engine 2.0 wbc struct declaration
 *
 *	History     :       2014/03/03   wangxuan   initial version
 *
 */
#ifndef __DE_WB_TYPE_H__
#define __DE_WB_TPYE_H__


#define ____SEPARATOR_DEFEINE____
#define WB_END_IE				0x1
#define WB_FINISH_IE (0x1<<4)
#define WB_FIFO_OVERFLOW_ERROR_IE (0x1<<5)
#define WB_TIMEOUT_ERROR_IE (0x1<<6)

#define MININWIDTH 8
#define MININHEIGHT 4
/* support 8192,limit by LCD */
#define MAXINWIDTH 4096
/* support 8192,limit by LCD */
#define MAXINHEIGHT 4096
#define LINE_BUF_LEN 2048
#define LOCFRACBIT 18
#define SCALERPHASE 16

#define ____SEPARATOR_REGISTERS____

typedef union {
	unsigned int dwval;
	struct {
		unsigned int wb_start :1;
		unsigned int r0 :3;
		unsigned int soft_reset      :1;
		unsigned int r1 :11;
		unsigned int in_port_sel     :2;
		unsigned int r2 :10;
		unsigned int auto_gate_en    :1;
		unsigned int clk_gate :1;
		unsigned int r3              :1;
		unsigned int bist_en :1;
	} bits;
} __wb_gctrl_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width :13;
		unsigned int r0 :3;
		unsigned int height :13;
		unsigned int r1 :3;
	} bits;
} __wb_size_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   crop_left       :13;
		unsigned int   r0							:3;
		unsigned int   crop_top        :13;
		unsigned int   r1              :3;
	} bits;
} __wb_crop_coord_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   crop_width      :13;
		unsigned int   r0 :3;
		unsigned int   crop_height     :13;
		unsigned int   r1              :3;
	} bits;
} __wb_crop_size_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   addr			:32;
	} bits;
} __wb_addr_reg_t;


typedef union {
	unsigned int dwval;
	struct {
		unsigned int   ch0_h_addr			:8;
		unsigned int   ch1_h_addr			:8;
		unsigned int   ch2_h_addr			:8;
		unsigned int   r0					:8;
	} bits;
} __wb_high_addr_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   pitch			:32;
	} bits;
} __wb_pitch_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   cur_group	 :1;
		unsigned int   r0			 :15;
		unsigned int   auto_switch   :1;
		unsigned int   r1            :3;
		unsigned int   manual_group  :1;
		unsigned int   r2            :11;
	} bits;
} __wb_addr_switch_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   format :4;
		unsigned int   r0  :28;
	} bits;
} __wb_format_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int	int_en :1;
		unsigned int	r0 :31;
	} bits;
} __wb_int_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int		irq						:1;
		unsigned int		r0						:3;
		unsigned int		finish				:1;
		unsigned int		overflow			:1;
		unsigned int 	timeout				:1;
		unsigned int   r1						:1;
		unsigned int   busy			  	:1;
		unsigned int   r2						:23;
	} bits;
} __wb_status_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   csc_en		:1;
		unsigned int   cs_en			:1;
		unsigned int   fs_en			:1;
		unsigned int   r0      	:29;
	} bits;
} __wb_bypass_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   m				:13;
		unsigned int   r1			:3;
		unsigned int   n				:13;
		unsigned int   r0      :3;
	} bits;
} __wb_cs_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   width		:13;
		unsigned int   r1			:3;
		unsigned int   height	:13;
		unsigned int   r0      :3;
	} bits;
} __wb_fs_size_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int   r1		:2;
		unsigned int   frac	:18;
		unsigned int   intg	:2;
		unsigned int   r0    :10;
	} bits;
} __wb_fs_step_reg_t;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coef0				:   8 ;
		unsigned int coef1				:   8 ;
		unsigned int coef2				:   8 ;
		unsigned int coef3				:   8 ;
	} bits;
} __wb_coeff_reg_t;

typedef struct {
	__wb_gctrl_reg_t					gctrl   		;
	__wb_size_reg_t						size		;
	__wb_crop_coord_reg_t			crop_coord	;
	__wb_crop_size_reg_t			crop_size	;
	__wb_addr_reg_t						wb_addr_A0	;
	__wb_addr_reg_t						wb_addr_A1  ;
	__wb_addr_reg_t						wb_addr_A2	;
	__wb_high_addr_reg_t			wb_addr_AH	;
	__wb_addr_reg_t						wb_addr_B0	;
	__wb_addr_reg_t						wb_addr_B1  ;
	__wb_addr_reg_t						wb_addr_B2	;
	__wb_high_addr_reg_t			wb_addr_BH	;
	__wb_pitch_reg_t					wb_pitch0	;
	__wb_pitch_reg_t					wb_pitch1   ;
	unsigned int				res0[2]		;
	__wb_addr_switch_reg_t		addr_switch	;
	__wb_format_reg_t					fmt			;
	__wb_int_reg_t						intr		;
	__wb_status_reg_t					status		;
	unsigned int											res1		;
	__wb_bypass_reg_t					bypass		;
	unsigned int					res2[6]		;
	__wb_cs_reg_t							cs_horz		;
	__wb_cs_reg_t							cs_vert		;
	unsigned int											res3[2]		;
	__wb_fs_size_reg_t				fs_insize	;
	__wb_fs_size_reg_t				fs_outsize	;
	__wb_fs_step_reg_t				fs_hstep	;
	__wb_fs_step_reg_t				fs_vstep	;
	unsigned int											res4[92]	;
	__wb_coeff_reg_t					yhcoeff[16];
	unsigned int											res5[16]	;
	__wb_coeff_reg_t					chcoeff[16];
	unsigned int            					res6[16]    ;
} __wb_reg_t;

typedef enum {
	WB_FORMAT_RGB_888					= 0x0,
	WB_FORMAT_BGR_888					= 0x1,
	WB_FORMAT_ARGB_8888					= 0x4,
	WB_FORMAT_ABGR_8888                   = 0x5,
	WB_FORMAT_BGRA_8888                   = 0x6,
	WB_FORMAT_RGBA_8888                   = 0x7,
	WB_FORMAT_YUV420_P                    = 0x8,
	WB_FORMAT_YUV420_SP_VUVU              = 0xc,
	WB_FORMAT_YUV420_SP_UVUV              = 0xd,
} wb_output_fmt;

extern unsigned int wb_lan2coefftab16[16];
extern unsigned int wb_lan2coefftab16_down[16];

#endif


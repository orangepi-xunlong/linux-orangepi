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
 *	File name   :       de_vsu_type.h
 *
 *	Description :       display engine 2.0 vsu struct declaration
 *
 *	History     :       2014/03/20  vito cheng  v0.1  Initial version
 *
 */
#ifndef __DE_VSU_TYPE_H__
#define __DE_VSU_TYPE_H__

/*
 * __vsu_reg_t
 */
typedef union {
	unsigned int dwval;
	struct {
		unsigned int en:1;
		unsigned int res0:3;
		unsigned int coef_switch_rdy:1;
		unsigned int res1:25;
		unsigned int reset:1;
		unsigned int bist:1;
	} bits;
} VSU_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:4;
		unsigned int busy:1;
		unsigned int res1:11;
		unsigned int line_cnt:12;
		unsigned int res2:4;
	} bits;
} VSU_STATUS_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int vphase_sel_en:1;
		unsigned int res0:31;
	} bits;
} VSU_FIELD_CTRL_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int res0:3;
		unsigned int height:13;
		unsigned int res1:3;
	} bits;
} VSU_OUTSIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int res0:3;
		unsigned int height:13;
		unsigned int res1:3;
	} bits;
} VSU_INSIZE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int frac:19;
		unsigned int integer:4;
		unsigned int res1:8;
	} bits;
} VSU_HSTEP_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int frac:19;
		unsigned int integer:4;
		unsigned int res1:8;
	} bits;
} VSU_VSTEP_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int frac:19;
		unsigned int integer:4;
		unsigned int res1:8;
	} bits;
} VSU_HPHASE_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int frac:19;
		unsigned int integer:4;
		unsigned int res1:8;
	} bits;
} VSU_VPHASE0_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int res0:1;
		unsigned int frac:19;
		unsigned int integer:4;
		unsigned int res1:8;
	} bits;
} VSU_VPHASE1_REG;

typedef union {
	unsigned int dwval;
	struct {
		unsigned int coef0:8;
		unsigned int coef1:8;
		unsigned int coef2:8;
		unsigned int coef3:8;
	} bits;
} VSU_COEFF_REG;

typedef struct {
    VSU_CTRL_REG        ctrl        ;
    unsigned int        res0        ;
    VSU_STATUS_REG      status      ;
    VSU_FIELD_CTRL_REG  field       ;
    unsigned int        res1[12]    ;
    VSU_OUTSIZE_REG     outsize     ;
    unsigned int        res13[15]   ;
    VSU_INSIZE_REG      ysize       ;
    unsigned int        res2        ;
    VSU_HSTEP_REG       yhstep      ;
    VSU_VSTEP_REG       yvstep      ;
    VSU_HPHASE_REG      yhphase     ;
    unsigned int        res3        ;
    VSU_VPHASE0_REG     yvphase0    ;
    VSU_VPHASE1_REG     yvphase1    ;
    unsigned int        res4[8]     ;
    VSU_INSIZE_REG      csize       ;
    unsigned int        res5        ;
    VSU_HSTEP_REG       chstep      ;
    VSU_VSTEP_REG       cvstep      ;
    VSU_HPHASE_REG      chphase     ;
    unsigned int        res6        ;
    VSU_VPHASE0_REG     cvphase0    ;
    VSU_VPHASE1_REG     cvphase1    ;
    unsigned int        res7[72]    ;
    VSU_COEFF_REG       yhcoeff0[32];
    unsigned int        res8[32]    ;
    VSU_COEFF_REG       yhcoeff1[32];
    unsigned int        res9[32]    ;
    VSU_COEFF_REG       yvcoeff[32] ;
    unsigned int        res10[96]   ;
    VSU_COEFF_REG       chcoeff0[32];
    unsigned int        res11[32]   ;
    VSU_COEFF_REG       chcoeff1[32];
    unsigned int        res12[32]   ;
    VSU_COEFF_REG       cvcoeff[32] ;

} __vsu_reg_t;

#endif

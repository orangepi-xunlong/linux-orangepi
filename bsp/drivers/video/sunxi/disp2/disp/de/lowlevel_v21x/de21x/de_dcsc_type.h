/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*****************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :       de_dcsc_type.h
 *
 *  Description :       display engine de35x
 *
 *  History     :       2022/05/16 v0.1  Initial version
****************************************************************************/

#ifndef __DE_DCSC_TYPE_H__
#define __DE_DCSC_TYPE_H__

union CSC_BYPASS_REG2 {
	unsigned int dwval;
	struct {
		unsigned int enable:1;
		unsigned int res0:31;
	} bits;
};

union CSC_COEFF_REG {
	unsigned int dwval;
	struct {
		unsigned int coeff:13;
		unsigned int res0:19;
	} bits;
};

union CSC_CONST_REG2 {
	unsigned int dwval;
	struct {
		unsigned int cnst:14;
		unsigned int res0:18;
	} bits;
};

/* CSC IN SMBL */
struct __csc2_reg_t {
	union CSC_BYPASS_REG2 bypass;
	unsigned int res[47];
	union CSC_COEFF_REG c00;
	union CSC_COEFF_REG c01;
	union CSC_COEFF_REG c02;
	union CSC_CONST_REG2 c03;
	union CSC_COEFF_REG c10;
	union CSC_COEFF_REG c11;
	union CSC_COEFF_REG c12;
	union CSC_CONST_REG2 c13;
	union CSC_COEFF_REG c20;
	union CSC_COEFF_REG c21;
	union CSC_COEFF_REG c22;
	union CSC_CONST_REG2 c23;
};

#endif

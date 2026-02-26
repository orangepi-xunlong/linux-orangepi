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

#include "de_csc2.h"
#include "de_csc2_type.h"

#include "de_top.h"
#include "de_feat.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "../../include.h"

#define CSC_ENHANCE_MODE_NUM 3

static const int rgb2yuv_17bit_fp[12][16] = {
	/* input : Full RGB 601 */
	/* output : Full YCbCr 601 */
	{
		0x00009917, 0x00012c8b, 0x00003a5e, 0x00000000,
		0xffffa9a0, 0xffff5660, 0x00010000, 0x00000200,
		0x00010000, 0xffff29a0, 0xffffd660, 0x00000200,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 709 */
	/* output : Full YCbCr 709 */
	{
		0x00006cda, 0x00016e2f, 0x000024f7, 0x00000000,
		0xffffc557, 0xffff3aa9, 0x00010000, 0x00000200,
		0x00010000, 0xffff1779, 0xffffe887, 0x00000200,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 2020 */
	/* output : Full YCbCr 2020 */
	{
		0x00008681, 0x00015b23, 0x00001e5d, 0x00000000,
		0xffffb882, 0xffff477e, 0x00010000, 0x00000200,
		0x00010000, 0xffff1497, 0xffffeb69, 0x00000200,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 601 */
	/* output : Limit YCbCr 601 */
	{
		0x00008396, 0x0001020c, 0x0000322d, 0x00000040,
		0xffffb41f, 0xffff6b02, 0x0000e0df, 0x00000200,
		0x0000e0df, 0xffff4396, 0xffffdba6, 0x00000200,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 709 */
	/* output : Limit YCbCr 709 */
	{
		0x00005d7e, 0x00013a78, 0x00001fbe, 0x00000040,
		0xffffcc7e, 0xffff52a3, 0x0000e0df, 0x00000200,
		0x0000e0df, 0xffff33c3, 0xffffeb5e, 0x00000200,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 2020 */
	/* output : Limit YCbCr 2020 */
	{
		0x00007382, 0x00012a23, 0x00001a10, 0x00000040,
		0xffffc134, 0xffff5dec, 0x0000e0df, 0x00000200,
		0x0000e0df, 0xffff3135, 0xffffedea, 0x00000200,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Limit RGB 2020 */
	/* output : Full YCbCr 2020 */
	{
		0x0000b241, 0x00015df2, 0x000043f6, 0x00000000,
		0xffff9b6d, 0xffff3a7e, 0x00012a15, 0x00000200,
		0x00012a15, 0xffff0663, 0xffffcf88, 0x00000200,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
	/* input : Limit RGB 709 */
	/* output : Full YCbCr 709 */
	{
		0x00007ebf, 0x0001aa61, 0x00002b0b, 0x00000000,
		0xffffbbb2, 0xffff1a39, 0x00012a15, 0x00000200,
		0x00012a15, 0xfffef140, 0xffffe4ab, 0x00000200,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
	/* input : Limit RGB 2020 */
	/* output : Full YCbCr 2020 */
	{
		0x00009c9d, 0x00019433, 0x0000235b, 0x00000000,
		0xffffacc1, 0xffff292a, 0x00012a15, 0x00000200,
		0x00012a15, 0xfffeede4, 0xffffe807, 0x00000200,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
	/* input : Limit RGB 601 */
	/* output : Limit YCbCr 601 */
	{
		0x00009917, 0x00012c8b, 0x00003a5e, 0x00000040,
		0xffffa7a3, 0xffff5285, 0x000105d8, 0x00000200,
		0x000105d8, 0xffff24bd, 0xffffd56b, 0x00000200,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
	/* input : Limit RGB 709 */
	/* output : Limit YCbCr 709 */
	{
		0x00006cda, 0x00016e2f, 0x000024f7, 0x00000040,
		0xffffc400, 0xffff3628, 0x000105d8, 0x00000200,
		0x000105d8, 0xffff122a, 0xffffe7fe, 0x00000200,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
	/* input : Limit RGB 2020 */
	/* output : Limit YCbCr 2020 */
	{
		0x00008681, 0x00015b23, 0x00001e5d, 0x00000040,
		0xffffb6e1, 0xffff4347, 0x000105d8, 0x00000200,
		0x000105d8, 0xffff0f37, 0xffffeaf1, 0x00000200,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
};

static const int yuv2rgb_17bit_fp[12][16] = {
	/* input : Full YCbCr 601 */
	/* output : Full RGB 601 */
	{
		0x00020000, 0x00000000, 0x0002cdd3, 0x00000000,
		0x00020000, 0xffff4fcd, 0xfffe925c, 0x00000000,
		0x00020000, 0x00038b44, 0x00000000, 0x00000000,
		0x00000000, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Full YCbCr 709 */
	/* output : Full RGB 709*/
	{
		0x00000000, 0x00000000, 0x0003264c, 0x00000000,
		0x00020000, 0xffffa017, 0xffff1052, 0x00000000,
		0x00020000, 0x0003b611, 0x00000000, 0x00000000,
		0x00000000, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Full YCbCr 2020 */
	/* output : Full RGB  2020*/
	{
		0x00020000, 0x00000000, 0x0002f2ff, 0x00000000,
		0x00020000, 0xffffabc0, 0xfffedb78, 0x00000000,
		0x00020000, 0x0003c347, 0x00000000, 0x00000000,
		0x00000000, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Full YCbCr 601 */
	/* output : Limit RGB 601 */
	{
		0x00020000, 0x00000000, 0x0002cdd3, 0x00000000,
		0x00020000, 0xffff4fcd, 0xfffe925c, 0x00000000,
		0x00020000, 0x00038b44, 0x00000000, 0x00000000,
		0x00000000, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Full YCbCr 709 */
	/* output : Limit RGB 709*/
	{
		0x00000000, 0x00000000, 0x0003264c, 0x00000000,
		0x00020000, 0xffffa017, 0xffff1052, 0x00000000,
		0x00020000, 0x0003b611, 0x00000000, 0x00000000,
		0x00000000, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Full YCbCr 2020 */
	/* output : Limit RGB  2020*/
	{
		0x00020000, 0x00000000, 0x0002f2ff, 0x00000000,
		0x00020000, 0xffffabc0, 0xfffedb78, 0x00000000,
		0x00020000, 0x0003c347, 0x00000000, 0x00000000,
		0x00000000, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Limit YCbCr 601 */
	/* output : Full RGB 601 */
	{
		0x0002542c, 0x00000000, 0x00033127, 0x00000000,
		0x0002542c, 0xffff3766, 0xfffe5fbe, 0x00000000,
		0x0002542c, 0x000408ce, 0x00000000, 0x00000000,
		0x00000040, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Limit YCbCr 709 */
	/* output : Full RGB 709*/
	{
		0x0002542c, 0x00000000, 0x000395dd, 0x00000000,
		0x0002542c, 0xffff92d7, 0xfffeef35, 0x00000000,
		0x0002542c, 0x0004398c, 0x00000000, 0x00000000,
		0x00000040, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Limit YCbCr 2020 */
	/* output : Full RGB  2020*/
	{
		0x0002542c, 0x00000000, 0x00035b7f, 0x00000000,
		0x0002542c, 0xffffa00d, 0xfffeb2f2, 0x00000000,
		0x0002542c, 0x0004489a, 0x00000000, 0x00000000,
		0x00000040, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Limit YCbCr 601 */
	/* output : Limit RGB 601 */
	{
		0x00020000, 0x00000000, 0x0002bdcd, 0x00000040,
		0x00020000, 0xffff53bc, 0xfffe9a86, 0x00000040,
		0x00020000, 0x00037703, 0x00000000, 0x00000040,
		0x00000040, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Limit YCbCr 709 */
	/* output : Limit RGB 709*/
	{
		0x00020000, 0x00000000, 0x0003144d, 0x00000040,
		0x00020000, 0xffffa23b, 0xffff15ac, 0x00000040,
		0x00020000, 0x0003a0dc, 0x00000000, 0x00000040,
		0x00000040, 0x00000200, 0x00000200, 0x00000000,
	},
	/* input : Limit YCbCr 2020 */
	/* output : Limit RGB  2020*/
	{
		0x00020000, 0x00000000, 0x0002e225, 0x00000040,
		0x00020000, 0xffffada1, 0xfffee1ff, 0x00000040,
		0x00020000, 0x0003adc6, 0x00000000, 0x00000040,
		0x00000040, 0x00000200, 0x00000200, 0x00000000,
	},
};

/* full rgb cs convert or color_range convert */
static const int rgb2rgb_17bit_fp[8][16] = {
	/* input : Full RGB 601 */
	/* output : Full RGB 709 */
	{
		0x00021687, 0xffffe979, 0x00000000, 0x00000000,
		0x00000000, 0x00020000, 0x00000000, 0x00000000,
		0x00000000, 0x0000060b, 0x0001f9f5, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 601 */
	/* output : Full RGB 2020 */
	{
		0x00014f5c, 0x00009aba, 0x000015ea, 0x00000000,
		0x000024ea, 0x0001d54d, 0x000005bc, 0x00000000,
		0x000008c1, 0x00003220, 0x0001c51f, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 709 */
	/* output : Full RGB 601 */
	{
		0x0001ea65, 0x0000159b, 0x00000000, 0x00000000,
		0x00000000, 0x00020000, 0x00000000, 0x00000000,
		0x00000000, 0xfffff9e8, 0x00020618, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 709 */
	/* output : Full RGB 2020 */
	{
		0x0001413b, 0x0000a89a, 0x0000162b, 0x00000000,
		0x00002361, 0x0001d6c9, 0x000005d6, 0x00000000,
		0x00000866, 0x00002d0e, 0x0001ca8c, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 2020 */
	/* output : Full RGB 601 */
	{
		0x00032b9f, 0xfffef845, 0xffffdc1c, 0x00000000,
		0xffffc034, 0x0002440b, 0xfffffbc0, 0x00000000,
		0xfffff759, 0xffffc4f7, 0x000243b0, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB 2020 */
	/* output : Full RGB 709 */
	{
		0x0003522d, 0xfffed326, 0xffffdaba, 0x00000000,
		0xffffc034, 0x0002440b, 0xfffffbc0, 0x00000000,
		0xfffff6ae, 0xffffcc7e, 0x00023cc6, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Full RGB */
	/* output : Limit RGB */
	{
		0x0001b7b8, 0x00000000, 0x00000000, 0x00000040,
		0x00000000, 0x0001b7b8, 0x00000000, 0x00000040,
		0x00000000, 0x00000000, 0x0001b7b8, 0x00000040,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	},
	/* input : Limit RGB */
	/* output : Full RGB */
	{
		0x0002542a, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0002542a, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x0002542a, 0x00000000,
		0x00000040, 0x00000040, 0x00000040, 0x00000000,
	},
};

static const int bypass[16] = {
	0x00020000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00020000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00020000, 0x00000000,
	0,          0,         0,        0
};

static const int table_sin[91] = {
	0, 2, 4, 7, 9, 11, 13, 16, 18, 20,
	22, 24, 27, 29, 31, 33, 35, 37, 40, 42,
	44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
	64, 66, 68, 70, 72, 73, 75, 77, 79, 81,
	82, 84, 86, 87, 89, 91, 92, 94, 95, 97,
	98, 99, 101, 102, 104, 105, 106, 107, 109, 110,
	111, 112, 113, 114, 115, 116, 117, 118, 119, 119,
	120, 121, 122, 122, 123, 124, 124, 125, 125, 126,
	126, 126, 127, 127, 127, 128, 128, 128, 128, 128,
	128
};

static inline __s64 IntRightShift64(__s64 datain, unsigned int shiftbit)
{
	__s64 dataout;
	__s64 tmp;

	tmp = (shiftbit >= 1) ? (1 << (shiftbit - 1)) : 0;
	if (datain >= 0)
		dataout = (datain + tmp) >> shiftbit;
	else
		dataout = -((-datain + tmp) >> shiftbit);

	return dataout;
}

static s32 IDE_SCAL_MATRIC_MUL(const int *in1, const int *in2, int *result)
{

	/* result[0] = (int)(((long long)in1[0] * in2[0] + (long long)in1[1] */
			   /* * in2[4] + (long long)in1[2] * in2[8] + 0x10000) >> 17); */
	/* result[1] = (int)(((long long)in1[0] * in2[1] + (long long)in1[1] */
			   /* * in2[5] + (long long)in1[2] * in2[9] + 0x10000) >> 17); */
	/* result[2] = (int)(((long long)in1[0] * in2[2] + (long long)in1[1] */
			   /* * in2[6] + (long long)in1[2] * in2[10] + 0x10000) >> 17); */

	/* result[4] = (int)(((long long)in1[4] * in2[0] + (long long)in1[5] */
			   /* * in2[4] + (long long)in1[6] * in2[8] + 0x10000) >> 17); */
	/* result[5] = (int)(((long long)in1[4] * in2[1] + (long long)in1[5] */
			   /* * in2[5] + (long long)in1[6] * in2[9] + 0x10000) >> 17); */
	/* result[6] = (int)(((long long)in1[4] * in2[2] + (long long)in1[5] */
			   /* * in2[6] + (long long)in1[6] * in2[10] + 0x10000) >> 17); */

	/* result[8] = (int)(((long long)in1[8] * in2[0] + (long long)in1[9] */
			   /* * in2[4] + (long long)in1[10] * in2[8] + 0x10000) >> 17); */
	/* result[9] = (int)(((long long)in1[8] * in2[1] + (long long)in1[9] */
			   /* * in2[5] + (long long)in1[10] * in2[9] + 0x10000) >> 17); */
	/* result[10] = (int)(((long long)in1[8] * in2[2] + (long long)in1[9] */
			    /* * in2[6] + (long long)in1[10] * in2[10] + 0x10000) >> 17); */


	/* //C03/C13/C23 */
	/* result[3] = (in1[3] + ((in1[0] * (in2[3] - in1[12]) + */
				   /* in1[1] * (in2[7] - in1[13]) + */
				   /* in1[2] * (in2[11] - in1[14]) + 0x10000) >> 17)); */
	/* result[7] = (in1[7] + ((in1[4] * (in2[3] - in1[12]) + */
				   /* in1[5] * (in2[7] - in1[13]) + */
				   /* in1[6] * (in2[11] - in1[14]) + 0x10000) >> 17)); */
	/* result[11] = (in1[11] + ((in1[8] * (in2[3] - in1[12]) + */
				     /* in1[9] * (in2[7] - in1[13]) + */
				     /* in1[10] * (in2[11] - in1[14]) + 0x10000) >> 17)); */

	/* //D0/D1/D2 */
	/* result[12] = in2[12]; */
	/* result[13] = in2[13]; */
	/* result[14] = in2[14]; */
	/* result[15] = 0; */

	result[0] = (int)IntRightShift64(((long long)in1[0] * in2[0] + (long long)in1[1]
			   * in2[4] + (long long)in1[2] * in2[8] + 0x10000), 17);
	result[1] = (int)IntRightShift64(((long long)in1[0] * in2[1] + (long long)in1[1]
			   * in2[5] + (long long)in1[2] * in2[9] + 0x10000), 17);
	result[2] = (int)IntRightShift64(((long long)in1[0] * in2[2] + (long long)in1[1]
			   * in2[6] + (long long)in1[2] * in2[10] + 0x10000), 17);

	result[4] = (int)IntRightShift64(((long long)in1[4] * in2[0] + (long long)in1[5]
			   * in2[4] + (long long)in1[6] * in2[8] + 0x10000), 17);
	result[5] = (int)IntRightShift64(((long long)in1[4] * in2[1] + (long long)in1[5]
			   * in2[5] + (long long)in1[6] * in2[9] + 0x10000), 17);
	result[6] = (int)IntRightShift64(((long long)in1[4] * in2[2] + (long long)in1[5]
			   * in2[6] + (long long)in1[6] * in2[10] + 0x10000), 17);

	result[8] = (int)IntRightShift64(((long long)in1[8] * in2[0] + (long long)in1[9]
			   * in2[4] + (long long)in1[10] * in2[8] + 0x10000), 17);
	result[9] = (int)IntRightShift64(((long long)in1[8] * in2[1] + (long long)in1[9]
			   * in2[5] + (long long)in1[10] * in2[9] + 0x10000), 17);
	result[10] = (int)IntRightShift64(((long long)in1[8] * in2[2] + (long long)in1[9]
			    * in2[6] + (long long)in1[10] * in2[10] + 0x10000), 17);


	//C03/C13/C23
	result[3] = in1[3] + IntRightShift64((in1[0] * (in2[3] - in1[12]) +
					      in1[1] * (in2[7] - in1[13]) +
					      in1[2] * (in2[11] - in1[14]) + 0x10000), 17);
	result[7] = in1[7] + IntRightShift64((in1[4] * (in2[3] - in1[12]) +
					      in1[5] * (in2[7] - in1[13]) +
					      in1[6] * (in2[11] - in1[14]) + 0x10000), 17);
	result[11] = in1[11] + IntRightShift64((in1[8] * (in2[3] - in1[12]) +
						in1[9] * (in2[7] - in1[13]) +
						in1[10] * (in2[11] - in1[14]) + 0x10000), 17);

	//D0/D1/D2
	result[12] = in2[12];
	result[13] = in2[13];
	result[14] = in2[14];
	result[15] = 0;

	return 0;
}

enum {
	CSC2_REG_BLK_CTL = 0,
	CSC2_REG_BLK_COEFF,
	CSC2_REG_BLK_NUM,
};

struct de_csc2_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[CSC2_REG_BLK_NUM];

	struct disp_csc_config csc_config_backup;
	u32 use_user_matrix;
	int user_matrix[16];

	int enhance_matrix[16];

	void (*set_blk_dirty)(struct de_csc2_private *priv,
		u32 blk_id, u32 dirty);
};

/* store disp csc2 priv, layout: | bld csc2 | chn csc2 (close to BLD index small) | */
static struct de_csc2_private *csc2_priv[DE_NUM];

static inline struct csc2_reg *get_csc2_reg(struct de_csc2_private *priv)
{
	return (struct csc2_reg *)(priv->reg_blks[0].vir_addr);
}

static struct de_csc2_private *get_csc2_priv(u32 disp, u32 chn, u32 csc_id,
					     enum disp_csc_hw_type csc_type)
{
	int i, csc2_index = 0;

	if (csc_type == CSC_TYPE_DISP && csc_id >= de_feat_get_num_csc2s_by_disp(disp))
		return NULL;

	if (csc_type == CSC_TYPE_CHN && csc_id >= de_feat_get_num_csc2s_by_chn(disp, chn))
		return NULL;

	if (csc_type == CSC_TYPE_DISP) {
		return &csc2_priv[disp][csc_id];
	} else if (csc_type == CSC_TYPE_CHN) {
		csc2_index += de_feat_get_num_csc2s_by_disp(disp);
		for (i = 0; i < chn; i++)
			csc2_index += de_feat_get_num_csc2s_by_chn(disp, i);
		return &csc2_priv[disp][csc2_index + csc_id];
	}

	return NULL;
}

static void csc2_set_block_dirty(
	struct de_csc2_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void csc2_set_rcq_head_dirty(
	struct de_csc2_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static int csc2_init(u32 disp, struct de_csc2_private *csc_priv, u8 __iomem *csc_reg_base)
{
	u32 rcq_used = de_feat_is_using_rcq(disp);
	u8 __iomem *reg_base = csc_reg_base;
	struct de_csc2_private *priv = csc_priv;
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *block;

	reg_mem_info->size = sizeof(struct csc2_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		rcq_used);
	if (NULL == reg_mem_info->vir_addr) {
		DE_WARN("alloc csc2[%d] mm fail! size=0x%x\n",
			 disp, reg_mem_info->size);
		return -1;
	}

	block = &(priv->reg_blks[CSC2_REG_BLK_CTL]);
	block->phy_addr = reg_mem_info->phy_addr; /* 32-byte aligned */
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x4; /* Word aligned */
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[CSC2_REG_BLK_COEFF]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x40;
	block->reg_addr = reg_base;

	priv->reg_blk_num = CSC2_REG_BLK_NUM;

	if (rcq_used)
		priv->set_blk_dirty = csc2_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = csc2_set_block_dirty;

	return 0;
}

/* not used yet, v210 implemented using gamma matrix */
int _csc_enhance_setting_temp[CSC_ENHANCE_MODE_NUM][4] = {
	{50, 50, 50, 50},
	/* normal */
	{50, 50, 50, 50},
	/* vivid */
	{50, 40, 50, 50},
	/* soft */
};

s32 de_csc2_enable(u32 disp, u32 chn, u32 csc2_id, u32 type, u8 en)
{
	struct de_csc2_private *priv = get_csc2_priv(disp, chn, csc2_id, type);
	struct csc2_reg *regs;

	if (!priv) {
		DE_INFO("Null hdl!");
		return -1;
	}

	regs = get_csc2_reg(priv);
	if (en != regs->ctl.bits.en) {
		regs->ctl.bits.en = en;
		priv->set_blk_dirty(priv, CSC2_REG_BLK_CTL, 1);
	}
	return 0;
}

int de_csc2_set_colormatrix(unsigned int sel, long long *matrix3x4, bool is_identity)
{
	/* int i; */
	/* struct de_rtmx_context *ctx = de_rtmx_get_context(sel); */
	/* int *user_matrix = ctx->output.data_bits == (enum de_data_bits)DISP_DATA_10BITS ? */
				/* user_matrix10bit : user_matrix8bit; */

	/* if (is_identity) { */
		/* when input identity matrix, skip the process of this matrix*/
		/* use_user_matrix = false; */
		/* return 0; */
	/* } */

	/* use_user_matrix = true; */
	/* user_matrix is 64-bit and combined with two int, the last 8 int is fixed*/
	/* for (i = 0; i < 12; i++) { */
		/*   matrix3x4 = real value*2^48
		 *   user_matrix = real value*2^12
		 *   so here >> 36
		 */
		/* user_matrix[2 * i] = matrix3x4[i] >> 36; */
		/* constants  multiply by databits  */
		/* if (i == 3 || i == 7 || i == 11) */
			/* user_matrix[2 * i] *= 1024; */
		/* user_matrix[2 * i + 1] = matrix3x4[i] >= 0 ? 0 : -1; */
	/* } */
	return 0;
}

static int get_csc2_color_space_index(u32 in_color_space, u32 out_color_space)
{
	if (in_color_space == DE_COLOR_SPACE_BT601) {
		if (out_color_space == DE_COLOR_SPACE_BT709)
			return 0;

		if (out_color_space == DE_COLOR_SPACE_BT2020C ||
		    out_color_space == DE_COLOR_SPACE_BT2020NC)
			return 1;
	} else if (in_color_space == DE_COLOR_SPACE_BT709) {
		if (out_color_space == DE_COLOR_SPACE_BT601)
			return 2;

		if (out_color_space == DE_COLOR_SPACE_BT2020C ||
		    out_color_space == DE_COLOR_SPACE_BT2020NC)
			return 3;
	} else if (in_color_space == DE_COLOR_SPACE_BT2020C ||
		   in_color_space == DE_COLOR_SPACE_BT2020NC) {
		if (out_color_space == DE_COLOR_SPACE_BT601)
			return 4;

		if (out_color_space == DE_COLOR_SPACE_BT709)
			return 5;
	}

	/* not support or same color space*/
	return -1;
}

static u32 get_csc2_mod_idx(enum de_color_space color_space)
{
	u32 idx;

	switch (color_space) {
	case DE_COLOR_SPACE_BT601:
		idx = 0; break;
	case DE_COLOR_SPACE_BT709:
		idx = 1; break;
	case DE_COLOR_SPACE_BT2020NC:
	case DE_COLOR_SPACE_BT2020C:
		idx = 2; break;
	default:
		idx = 0; break;
	}
	return idx;
}

int de_csc2_coeff_calc(const struct disp_csc_config *config, u32 *mat_sum, const int *mat[])
{
	u32 cs_index; /* index for r2r color_space matrix */
	u32 in_m, in_r, out_m, out_r;
	u32 mat_num = 0;

	in_r = config->in_color_range == DE_COLOR_RANGE_16_235 ? 1 : 0;
	in_m = get_csc2_mod_idx(config->in_mode);
	out_r = config->in_color_range == DE_COLOR_RANGE_16_235 ? 1 : 0;
	out_m = get_csc2_mod_idx(config->out_mode);
	cs_index = get_csc2_color_space_index(config->in_mode, config->out_mode);

	if (config->in_fmt == DE_FORMAT_SPACE_RGB) {
		if (config->out_fmt == DE_FORMAT_SPACE_YUV) {/* (r2r)(color space) + r2y */
			if (cs_index != -1) {
				mat[mat_num] = rgb2rgb_17bit_fp[cs_index];
				mat_num++;
			}
			mat[mat_num] = rgb2yuv_17bit_fp[out_r * 3 + out_m];
			mat_num++;
		} else {/* (r2r)(color space) + (r2r)(color range) */
			if (cs_index != -1) {
				mat[mat_num] = rgb2rgb_17bit_fp[cs_index];
				mat_num++;
			}
			if (config->out_color_range == DE_COLOR_RANGE_16_235) {
				mat[mat_num] = rgb2rgb_17bit_fp[6];
				mat_num++;
			}
		}
	} else { /* infmt == DE_YUV */
		if (config->out_fmt == DE_FORMAT_SPACE_YUV) {
		/* y2r(full) + (r2r)(color space) + (r2y) */
			if (config->in_color_range != config->out_color_range ||
			    cs_index != -1) {
				mat[mat_num] = yuv2rgb_17bit_fp[in_r * 6 + in_m];
				mat_num++;

				if (cs_index != -1) {
					mat[mat_num] = rgb2rgb_17bit_fp[cs_index];
					mat_num++;
				}

				mat[mat_num] = rgb2yuv_17bit_fp[out_r * 3 + out_m];
				mat_num++;
			}
		} else { /* outfmt == DE_RGB y2r + (r2r) + (r2r) */
			if (cs_index != -1) {
				mat[mat_num] = yuv2rgb_17bit_fp[in_r * 6 + in_m];
				mat_num++;

				mat[mat_num] = rgb2rgb_17bit_fp[cs_index];
				mat_num++;

				if (config->out_color_range == DE_COLOR_RANGE_16_235) {
					mat[mat_num] = rgb2rgb_17bit_fp[6];
					mat_num++;
				}
			} else {
				mat[mat_num] = yuv2rgb_17bit_fp[in_r * 6 + out_r * 3 + out_m];
				mat_num++;
			}

		}
	}

	*mat_sum = mat_num;
	return 0;
}

int de_csc2_enhance_coeff_calc(struct disp_csc_config *config, u32 *mat_sum, const int *mat[])
{
	u32 bright, contrast, sat, hue;
	int sinv = 0, cosv = 0, B, C, S;
	u32 out_m, out_r, mat_num = 0;
	int *hsbc_coef;

	struct de_csc2_private *priv = get_csc2_priv(config->disp, config->chn,
						     config->csc_id, config->hw_type);
	if (!priv) {
		DE_WARN("Null hdl!");
		return -1;
	}

	hsbc_coef = priv->enhance_matrix;
	bright = config->brightness > 100 ? 100 : config->brightness;
	contrast = config->contrast > 100 ? 100 : config->contrast;
	sat = config->saturation > 100 ? 100 : config->saturation;
	hue = config->hue > 100 ? 100 : config->hue;
	if (bright != 50 || contrast != 50 || sat != 50 || hue != 50) {
		/*
		 *
		bright:0~300
		contrast:0~300
		sat:0~300
		hue:0~360
		*/
		bright = bright * 3;
		contrast = contrast * 3;
		sat = sat * 3;
		hue = hue * 360 / 100;

		B = bright*20 - 600;    //
		/* int C;    *///10~300
		if (contrast < 10) {
			//C = 10;
			C = contrast;
		} else {
			C = contrast;
		}
		S = sat;    //0~300
		if (hue <= 90) {
			sinv = table_sin[hue];
			cosv = table_sin[90 - hue];
		} else if (hue <= 180) {
			sinv = table_sin[180-hue];
			cosv = -table_sin[hue-90];
		} else if (hue <= 270) {
			sinv = -table_sin[hue-180];
			cosv = -table_sin[270-hue];
		} else if (hue <= 360) {
			sinv = -table_sin[360 - hue];
			cosv = table_sin[hue - 270];
		}

		hsbc_coef[0] = C<<10;
		hsbc_coef[1] = 0;
		hsbc_coef[2] = 0;

		hsbc_coef[4] = 0;
		hsbc_coef[5] = (C * S * cosv) >> 4;
		hsbc_coef[6] = (C * S * sinv) >> 4;

		hsbc_coef[8] = 0;
		hsbc_coef[9] = -(C * S * sinv) >> 4;
		hsbc_coef[10] = (C * S * cosv) >> 4;


		hsbc_coef[3] = B + 64;
		hsbc_coef[7] = 512;
		hsbc_coef[11] = 512;

		hsbc_coef[12] = 64;
		hsbc_coef[13] = 512;
		hsbc_coef[14] = 512;
		hsbc_coef[15] = 0;
	}

	out_r = config->in_color_range == DE_COLOR_RANGE_16_235 ? 1 : 0;
	out_m = get_csc2_mod_idx(config->out_mode);
	if (config->in_fmt == DE_FORMAT_SPACE_RGB &&
	    config->out_fmt == DE_FORMAT_SPACE_RGB) { /* r2yf + hsbc + yf2r */
		mat[mat_num] = rgb2yuv_17bit_fp[out_r * 6 + out_m];
		mat_num++;

		mat[mat_num] = hsbc_coef;
		mat_num++;

		mat[mat_num] = yuv2rgb_17bit_fp[out_r * 3 + out_m];
		mat_num++;
	} else {
		mat[mat_num] = hsbc_coef;
		mat_num++;
	}

	*mat_sum = mat_num;
	return 0;
}

/* normal case:
 *display a SD video:
 *infmt = DE_YUV, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a HD video:
 *infmt = DE_YUV, incscmod = BT_709, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a JPEG picture:
 *infmt = DE_YUV, incscmod = BT_YCC, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *display a UI (RGB format)     with ENHANCE enable
 *infmt = DE_YUV, incscmod = BT_ENHANCE, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in RGB mode:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_16_235
 *output to PC with HDMI in RGB mode:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_RGB
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in YCbCr mode, 480i/576i/480p/576p:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV, outcscmod = BT_601
 *out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with HDMI in YCbCr mode, 720p/1080p/2160p:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV
 *outcscmod = BT_709, out_color_range = DISP_COLOR_RANGE_0_255
 *output to TV with CVBS:
 *infmt = DE_RGB, incscmod = BT_601, outfmt = DE_YUV
 *outcscmod = BT_601, out_color_range = DISP_COLOR_RANGE_0_255
 *bypass:
 *outfmt = infmt, outcscmod = incscmod
 *out_color_range = DISP_COLOR_RANGE_0_255
 *brightness=contrast=saturation=hue=50
 */
int de_csc2_apply(struct disp_csc_config *config)
{
	int csc_coeff[16], in0coeff[16], in1coeff[16];
	const int *matrix[5], *color_matrix, *enhance_matrix[3], *csc_matrix[3];
	unsigned int oper = 0, colorm_num = 0, enhancem_num = 0, cscm_num = 0;
	int i, j, enhance_mode;
	struct csc2_reg *regs;
	struct de_csc2_private *priv = get_csc2_priv(config->disp, config->chn,
						     config->csc_id, config->hw_type);
	if (!priv) {
		DE_INFO("Null hdl!");
		return -1;
	}

	/* get enhance param */
	config->enhance_mode =
	    (config->enhance_mode > CSC_ENHANCE_MODE_NUM - 1)
	     ? priv->csc_config_backup.enhance_mode : config->enhance_mode;
	enhance_mode = config->enhance_mode;
	config->brightness = _csc_enhance_setting_temp[enhance_mode][0];
	config->contrast = _csc_enhance_setting_temp[enhance_mode][1];
	config->saturation = _csc_enhance_setting_temp[enhance_mode][2];
	config->hue = _csc_enhance_setting_temp[enhance_mode][3];
	if (config->brightness < 50)
		config->brightness = config->brightness * 3 / 5 + 20;/*map from 0~100 to 20~100*/
	if (config->contrast < 50)
		config->contrast = config->contrast * 3 / 5 + 20;/*map from 0~100 to 20~100*/

	DE_INFO("sel=%d, in_fmt=%d, mode=%d, out_fmt=%d, mode=%d, range=%d\n",
	      config->disp, config->in_fmt, config->in_mode, config->out_fmt,
	      config->out_mode, config->out_color_range);
	DE_INFO("brightness=%d, contrast=%d, saturation=%d, hue=%d\n",
	      config->brightness, config->contrast, config->saturation,
	      config->hue);

	if (config->in_fmt == config->out_fmt && config->in_mode == config->out_mode
	    && config->in_color_range == config->out_color_range && config->brightness == 50
	    && config->contrast == 50 && config->saturation == 50 && config->hue == 50
	    && !priv->use_user_matrix) {
		memcpy(csc_coeff, bypass, sizeof(bypass));
		goto set_regs_exit;
	}

	if (config->flags & CSC_DIRTY_COLOR_MATRIX) {
		/* de_csc2_set_colormatrix(); */
	}
	if (priv->use_user_matrix) {
		color_matrix = priv->user_matrix;
		colorm_num++;
	}

	if (config->flags & CSC_DIRTY_CS_CONVERT) {
		de_csc2_coeff_calc(config, &cscm_num, csc_matrix);
	}

	if (config->flags & CSC_DIRTY_ENHANCE) {
		de_csc2_enhance_coeff_calc(config, &enhancem_num, enhance_matrix);
	}

	memcpy(&priv->csc_config_backup, config, sizeof(*config));

	/* sort matrix */
	if (priv->use_user_matrix)
		matrix[oper++] = color_matrix;
	if (config->in_fmt == DE_FORMAT_SPACE_YUV &&
	    config->out_fmt == DE_FORMAT_SPACE_RGB) {
		for (i = 0; i < enhancem_num; i++)
			matrix[oper++] = enhance_matrix[i];
		for (i = 0; i < cscm_num; i++)
			matrix[oper++] = csc_matrix[i];
	} else {
		for (i = 0; i < cscm_num; i++)
			matrix[oper++] = csc_matrix[i];
		for (i = 0; i < enhancem_num; i++)
			matrix[oper++] = enhance_matrix[i];
	}

	/* mat mul */
	if (oper == 0) {
		memcpy(csc_coeff, bypass, sizeof(bypass));
	} else if (oper == 1) {
		memcpy(csc_coeff, matrix[0], sizeof(csc_coeff));
	} else {
		for (i = 0; i < oper - 1; i++) {
			if (i == 0)
				IDE_SCAL_MATRIC_MUL(matrix[i + 1], matrix[i], in1coeff);
			else {
				memcpy(in0coeff, in1coeff, sizeof(in1coeff));
				IDE_SCAL_MATRIC_MUL(matrix[i], in0coeff, in1coeff);
			}
		}
		memcpy(csc_coeff, in1coeff, sizeof(in1coeff));
	}

	/* The 3x3 conversion coefficient is 17bit fixed-point --->10bit fixed-point */
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			csc_coeff[i * 4 + j] = IntRightShift64((csc_coeff[i * 4 + j] + 64), 7);
		}
	}

 set_regs_exit:
	regs = get_csc2_reg(priv);
	regs->ctl.bits.en = 1;
	regs->d0.dwval = *(csc_coeff + 12);
	regs->d1.dwval = *(csc_coeff + 13);
	regs->d2.dwval = *(csc_coeff + 14);
	regs->c00.dwval = *(csc_coeff);
	regs->c01.dwval = *(csc_coeff + 1);
	regs->c02.dwval = *(csc_coeff + 2);
	regs->c03.dwval = *(csc_coeff + 3);
	regs->c10.dwval = *(csc_coeff + 4);
	regs->c11.dwval = *(csc_coeff + 5);
	regs->c12.dwval = *(csc_coeff + 6);
	regs->c13.dwval = *(csc_coeff + 7);
	regs->c20.dwval = *(csc_coeff + 8);
	regs->c21.dwval = *(csc_coeff + 9);
	regs->c22.dwval = *(csc_coeff + 10);
	regs->c23.dwval = *(csc_coeff + 11);

	priv->set_blk_dirty(priv, CSC2_REG_BLK_CTL, 1);
	priv->set_blk_dirty(priv, CSC2_REG_BLK_COEFF, 1);

	return 0;
}

int de_csc2_get_config(struct disp_csc_config *config)
{
	struct de_csc2_private *priv = get_csc2_priv(config->disp, config->chn,
						     config->csc_id, config->hw_type);
	if (!priv) {
		DE_INFO("Null hdl!");
		return -1;
	}

	memcpy(config, &priv->csc_config_backup, sizeof(priv->csc_config_backup));
	return 0;
}

int de_csc2_update_regs(unsigned int sel)
{
	/*updated by rtmx*/
	return 0;
}

s32 de_csc2_get_reg_blocks(u32 disp, struct de_reg_block **blks,
			  u32 *blk_num)
{
	u32 csc, csc_num, hw_disp;
	u32 total = 0;

	hw_disp = de_feat_get_hw_disp(disp);
	csc_num = de_feat_get_num_csc2s(hw_disp);

	if (blks == NULL) {
		for (csc = 0; csc < csc_num; ++csc)
			total += csc2_priv[disp][csc].reg_blk_num;
		*blk_num = total;
		return 0;
	}

	for (csc = 0; csc < csc_num; ++csc) {
		struct de_csc2_private *priv = &(csc2_priv[disp][csc]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WARN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = priv->reg_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin)
			*blks++ = blk_begin;
		total += num;
		*blk_num -= num;
	}
	*blk_num = total;
	return 0;
}

s32 de_csc2_init(u32 disp, u8 __iomem *de_reg_base)
{
	int i, j, csc2_num, csc2_chn_num, csc2_disp_num, chn_num;
	u32 hw_disp = de_feat_get_hw_disp(disp);
	u8 __iomem *reg_base;
	struct de_csc2_private *priv;

	csc2_num = de_feat_get_num_csc2s(hw_disp);
	csc2_priv[disp] = kmalloc(sizeof(struct de_csc2_private) * csc2_num, GFP_KERNEL | __GFP_ZERO);
	if (csc2_priv == NULL) {
		DE_WARN("malloc csc2 priv err!");
		return -1;
	}

	csc2_disp_num = de_feat_get_num_csc2s_by_disp(hw_disp);
	for (i = 0; i < csc2_disp_num; i++) {
		reg_base = de_reg_base + DE_DISP_OFFSET(hw_disp) + DISP_CSC2_OFFSET(i);
		priv = get_csc2_priv(disp, 0, i, CSC_TYPE_DISP);
		csc2_init(disp, priv, reg_base);
	}

	chn_num = de_feat_get_num_chns(disp);
	for (i = 0; i < chn_num; i++) {
		u32 phy_chn = de_feat_get_phy_chn_id(disp, i);
		csc2_chn_num = de_feat_get_num_csc2s_by_chn(disp, i);

		for (j = 0; j < csc2_chn_num; j++) {
			priv = get_csc2_priv(disp, i, j, CSC_TYPE_CHN);
			reg_base = de_reg_base +
				DE_CHN_OFFSET(phy_chn) + CHN_CSC2_OFFSET(j);
			csc2_init(disp, priv, reg_base);
		}
	}

	return 0;
}

int de_csc2_exit(void)
{
	int disp, csc2, csc2_num, hw_disp;

	for (disp = 0; disp < DE_NUM; disp++) {
		hw_disp = de_feat_get_hw_disp(disp);
		csc2_num = de_feat_get_num_csc2s(hw_disp);
		for (csc2 = 0; csc2 < csc2_num; csc2++) {
			struct de_csc2_private *priv = &csc2_priv[disp][csc2];
			struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

			if (reg_mem_info->vir_addr != NULL)
				de_top_reg_memory_free(reg_mem_info->vir_addr,
					reg_mem_info->phy_addr, reg_mem_info->size);
		}
	}

	for (disp = 0; disp < DE_NUM; disp++) {
		kfree(csc2_priv[disp]);
	}

	return 0;
}

/* void de_csc2_pq_get_enhance(u32 sel, int *pq_enh) */
/* { */
	/* struct disp_csc_config *config = &g_csc2_config[sel]; */
	/* unsigned int enhance_mode = config->enhance_mode; */

	/* pq_enh[0] = _csc_enhance_setting[enhance_mode][0]; */
	/* pq_enh[1] = _csc_enhance_setting[enhance_mode][1]; */
	/* pq_enh[2] = _csc_enhance_setting[enhance_mode][2]; */
	/* pq_enh[3] = _csc_enhance_setting[enhance_mode][3]; */
/* } */

/* void de_csc2_pq_set_enhance(u32 sel, int *pq_enh) */
/* { */
	/* struct disp_csc_config *config = &g_csc2_config[sel]; */
	/* unsigned int enhance_mode = config->enhance_mode; */

	/* _csc_enhance_setting[enhance_mode][0] = pq_enh[0]; */
	/* _csc_enhance_setting[enhance_mode][1] = pq_enh[1]; */
	/* _csc_enhance_setting[enhance_mode][2] = pq_enh[2]; */
	/* _csc_enhance_setting[enhance_mode][3] = pq_enh[3]; */
/* } */

/* void pq_set_matrix(struct __scal_matrix4x4 *conig, int choice, int out, */
		   /* int write) */
/* { */
	/* struct de_rtmx_context *ctx = de_rtmx_get_context(0); */
	/* if (choice > 3) */
		/* return; */
	/* if (ctx->output.data_bits == (enum de_data_bits)DISP_DATA_10BITS) { */
		/* if (out) { */
			/* if (write) { */
				/* memcpy((y2r10bit + 0x20 * choice), conig, */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } else { */
				/* memcpy(conig, (y2r10bit + 0x20 * choice), */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } */
		/* } else { */
			/* if (write) { */
				/* memcpy((ir2y10bit + 0x20 * choice), conig, */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } else { */
				/* memcpy(conig, (ir2y10bit + 0x20 * choice), */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } */
		/* } */
	/* } else { */
		/* if (out) { */
			/* if (write) { */
				/* memcpy((y2r8bit + 0x20 * choice), conig, */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } else { */
				/* memcpy(conig, (y2r8bit + 0x20 * choice), */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } */
		/* } else { */
			/* if (write) { */
				/* memcpy((ir2y8bit + 0x20 * choice), conig, */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } else { */
				/* memcpy(conig, (ir2y8bit + 0x20 * choice), */
				       /* sizeof(struct __scal_matrix4x4)); */
			/* } */
		/* } */
	/* } */
/* } */


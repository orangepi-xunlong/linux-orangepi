/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_DSC_TYPE_H__
#define __DE_DSC_TYPE_H__

union dsc_pps0_3_reg_t {
	u32 dwval;
	struct {
		u32 mnv:4;
		u32 mjv:4;
		u32 ppsid:8;
		u32 res0:8;
		u32 line_buffer_bits_depth:4;
		u32 bist_per_component:4;
	} bits;
};

union dsc_pps4_7_reg_t {
	u32 dwval;
	struct {
		u32 bpph:2;
		u32 vbr:1;
		u32 res0:1;
		u32 crgb:1;
		u32 bpe:1;
		u32 res1:2;
		u32 bppl:8;
		u32 pchh:8;
		u32 pchl:8;
	} bits;
};

union dsc_pps8_11_reg_t {
	u32 dwval;
	struct {
		u32 pcwh:8;
		u32 pcwl:8;
		u32 slhh:8;
		u32 slhl:8;
	} bits;
};

union dsc_pps12_15_reg_t {
	u32 dwval;
	struct {
		u32 slwh:8;
		u32 slwl:8;
		u32 chsh:8;
		u32 chsl:8;
	} bits;
};

union dsc_pps16_19_reg_t {
	u32 dwval;
	struct {
		u32 ixdh:2;
		u32 res0:6;
		u32 ixdl:8;
		u32 iddh:8;
		u32 iddl:8;
	} bits;
};

union dsc_pps20_23_reg_t {
	u32 dwval;
	struct {
		u32 res0:8;
		u32 isv:6;
		u32 res1:2;
		u32 siih:8;
		u32 siil:8;
	} bits;
};

union dsc_pps24_27_reg_t {
	u32 dwval;
	struct {
		u32 sdih:4;
		u32 res0:4;
		u32 sdil:8;
		u32 res1:8;
		u32 fbo:5;
		u32 res2:3;
	} bits;
};

union dsc_pps28_31_reg_t {
	u32 dwval;
	struct {
		u32 nfboh:8;
		u32 nfbol:8;
		u32 sboh:8;
		u32 sbol:8;
	} bits;
};

union dsc_pps32_35_reg_t {
	u32 dwval;
	struct {
		u32 inoh:8;
		u32 inol:8;
		u32 fnoh:8;
		u32 fnol:8;
	} bits;
};

union dsc_pps36_39_reg_t {
	u32 dwval;
	struct {
		u32 fmnq:5;
		u32 res0:3;
		u32 fmxq:5;
		u32 res1:3;
		u32 rcmsh:8;
		u32 rcmsl:8;
	} bits;
};

union dsc_pps40_43_reg_t {
	u32 dwval;
	struct {
		u32 rcef:4;
		u32 res0:4;
		u32 rcqil0:5;
		u32 res1:3;
		u32 rcqil1:5;
		u32 res2:3;
		u32 rctol:4;
		u32 rctoh:4;
	} bits;
};

union dsc_pps44_47_reg_t {
	u32 dwval;
	struct {
		u32 rcbt0:8;
		u32 rcbt1:8;
		u32 rcbt2:8;
		u32 rcbt3:8;
	} bits;
};

union dsc_pps48_51_reg_t {
	u32 dwval;
	struct {
		u32 rcbt4:8;
		u32 rcbt5:8;
		u32 rcbt6:8;
		u32 rcbt7:8;
	} bits;
};

union dsc_pps52_55_reg_t {
	u32 dwval;
	struct {
		u32 rcbt8:8;
		u32 rcbt9:8;
		u32 rcbt10:8;
		u32 rcbt11:8;
	} bits;
};

union dsc_pps56_59_reg_t {
	u32 dwval;
	struct {
		u32 rcbt12:8;
		u32 rcbt13:8;
		u32 rcrp0_mnqrg:5;
		u32 rcrp0_mxqrg:5;
		u32 rcrp0_rgbpo:6;
	} bits;
};

union dsc_pps60_63_reg_t {
	u32 dwval;
	struct {
		u32 rcrp1_mnqrg:5;
		u32 rcrp1_mxqrg:5;
		u32 rcrp1_rgbpo:6;
		u32 rcrp2_mnqrg:5;
		u32 rcrp2_mxqrg:5;
		u32 rcrp2_rgbpo:6;
	} bits;
};

union dsc_pps64_67_reg_t {
	u32 dwval;
	struct {
		u32 rcrp3_mnqrg:5;
		u32 rcrp3_mxqrg:5;
		u32 rcrp3_rgbpo:6;
		u32 rcrp4_mnqrg:5;
		u32 rcrp4_mxqrg:5;
		u32 rcrp4_rgbpo:6;
	} bits;
};

union dsc_pps68_71_reg_t {
	u32 dwval;
	struct {
		u32 rcrp5_mnqrg:5;
		u32 rcrp5_mxqrg:5;
		u32 rcrp5_rgbpo:6;
		u32 rcrp6_mnqrg:5;
		u32 rcrp6_mxqrg:5;
		u32 rcrp6_rgbpo:6;
	} bits;
};

union dsc_pps72_75_reg_t {
	u32 dwval;
	struct {
		u32 rcrp7_mnqrg:5;
		u32 rcrp7_mxqrg:5;
		u32 rcrp7_rgbpo:6;
		u32 rcrp8_mnqrg:5;
		u32 rcrp8_mxqrg:5;
		u32 rcrp8_rgbpo:6;
	} bits;
};

union dsc_pps76_79_reg_t {
	u32 dwval;
	struct {
		u32 rcrp9_mnqrg:5;
		u32 rcrp9_mxqrg:5;
		u32 rcrp9_rgbpo:6;
		u32 rcrp10_mnqrg:5;
		u32 rcrp10_mxqrg:5;
		u32 rcrp10_rgbpo:6;
	} bits;
};

union dsc_pps80_83_reg_t {
	u32 dwval;
	struct {
		u32 rcrp11_mnqrg:5;
		u32 rcrp11_mxqrg:5;
		u32 rcrp11_rgbpo:6;
		u32 rcrp12_mnqrg:5;
		u32 rcrp12_mxqrg:5;
		u32 rcrp12_rgbpo:6;
	} bits;
};

union dsc_pps84_87_reg_t {
	u32 dwval;
	struct {
		u32 rcrp13_mnqrg:5;
		u32 rcrp13_mxqrg:5;
		u32 rcrp13_rgbpo:6;
		u32 rcrp14_mnqrg:5;
		u32 rcrp14_mxqrg:5;
		u32 rcrp14_rgbpo:6;
	} bits;
};

union dsc_reservd_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};

union dsc_dwc_version_reg_t {
	u32 dwval;
	struct {
		u32 vnum:16;
		u32 vtnum:8;
		u32 res0:4;
		u32 vtyp:4;
	} bits;
};

union dsc_dwc_cfg_reg0_t {
	u32 dwval;
	struct {
		u32 bpa:1;
		u32 vba:1;
		u32 res0:2;
		u32 nbpc:4;
		u32 lbfd:4;
		u32 n420_abl:1;
		u32 n422_abl:1;
		u32 mxprt:2;
		u32 mnv_abl:4;
		u32 mjv_abl:4;
		u32 ftyp:2;
		u32 fppe:1;
		u32 mnslc:3;
		u32 ppsreg:1;
		u32 res1:1;
	} bits;
};

union dsc_dwc_cfg_reg1_t {
	u32 dwval;
	struct {
		u32 mpcw:16;
		u32 mpxw:16;
	} bits;
};

union dsc_dwc_cfg_reg2_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 rbs:16;
	} bits;
};

union dsc_dwc_cfg_reg3_t {
	u32 dwval;
	struct {
		u32 mslw:16;
		u32 mslh:16;
	} bits;
};

union dsc_dwc_ctrl0_reg_t {
	u32 dwval;
	struct {
		u32 en:1;
		u32 fesl:1;
		u32 rbit:1;
		u32 rbyt:1;
		u32 flal:1;
		u32 mer:1;
		u32 epb:1;
		u32 epl:1;
		u32 init:1;
		u32 res0:6;
		u32 xnslc:1;
		u32 nslc:5;
		u32 res1:7;
		u32 sbo:1;
		u32 res2:2;
		u32 pps_update:1;
	} bits;
};

union dsc_dwc_ctrl1_reg_t {
	u32 dwval;
	struct {
		u32 pps_sel:8;
		u32 rbfth:24;
	} bits;
};

union dsc_dwc_sts0_reg_t {
	u32 dwval;
	struct {
		u32 rbuf:16;
		u32 rbof:16;
	} bits;
};

union dsc_dwc_sts1_reg_t {
	u32 dwval;
	struct {
		u32 lbuf:16;
		u32 lbof:16;
	} bits;
};

union dsc_dwc_sts2_reg_t {
	u32 dwval;
	struct {
		u32 fsuf:16;
		u32 fsof:16;
	} bits;
};

union dsc_dwc_sts3_reg_t {
	u32 dwval;
	struct {
		u32 seuf:16;
		u32 seof:16;
	} bits;
};

union dsc_dwc_sts4_reg_t {
	u32 dwval;
	struct {
		u32 bluf:16;
		u32 blof:16;
	} bits;
};

union dsc_dwc_sts5_reg_t {
	u32 dwval;
	struct {
		u32 bluf:16;
		u32 pbof:16;
	} bits;
};

union dsc_dwc_ers_reg_t {
	u32 dwval;
	struct {
		u32 ecw:32;
	} bits;
};

union dsc_dwc_blk0_reg_t {
	u32 dwval;
	struct {
		u32 bmod:2;
		u32 hpc:1;
		u32 hdly:5;
		u32 ihpol:1;
		u32 ivpol:1;
		u32 res0:6;
		u32 htotal:16;
	} bits;
};

union dsc_dwc_blk1_reg_t {
	u32 dwval;
	struct {
		u32 hsync:14;
		u32 vt_hbit:2;
		u32 hpar:16;
	} bits;
};

union dsc_dwc_blk2_reg_t {
	u32 dwval;
	struct {
		u32 vsync:8;
		u32 vfront:8;
		u32 vback:16;
	} bits;
};

union dsc_dwc_sts6_reg_t {
	u32 dwval;
	struct {
		u32 xrbuf:16;
		u32 xrbof:16;
	} bits;
};

union dsc_dwc_sts7_reg_t {
	u32 dwval;
	struct {
		u32 xlbuf:16;
		u32 xlbof:16;
	} bits;
};

union dsc_dwc_sts8_reg_t {
	u32 dwval;
	struct {
		u32 xfsuf:16;
		u32 xfsof:16;
	} bits;
};

union dsc_dwc_sts9_reg_t {
	u32 dwval;
	struct {
		u32 xseuf:16;
		u32 xseof:16;
	} bits;
};

union dsc_dwc_sts10_reg_t {
	u32 dwval;
	struct {
		u32 xbluf:16;
		u32 xblof:16;
	} bits;
};

union dsc_dwc_sts11_reg_t {
	u32 dwval;
	struct {
		u32 xpbuf:16;
		u32 xpbof:16;
	} bits;
};
struct dsc_dsi_reg {
	/* 0x00 - 0x0c */
	union dsc_pps0_3_reg_t dsc_pps0_3;
	union dsc_pps4_7_reg_t dsc_pps4_7;
	union dsc_pps8_11_reg_t dsc_pps8_11;
	union dsc_pps12_15_reg_t dsc_pps12_15;
	/* 0x10 - 0x1c */
	union dsc_pps16_19_reg_t dsc_pps16_19;
	union dsc_pps20_23_reg_t dsc_pps20_23;
	union dsc_pps24_27_reg_t dsc_pps24_27;
	union dsc_pps28_31_reg_t dsc_pps28_31;
	/* 0x20 - 0x2c */
	union dsc_pps32_35_reg_t dsc_pps32_35;
	union dsc_pps36_39_reg_t dsc_pps36_39;
	union dsc_pps40_43_reg_t dsc_pps40_43;
	union dsc_pps44_47_reg_t dsc_pps44_47;
	/* 0x30 - 0x3c */
	union dsc_pps48_51_reg_t dsc_pps48_51;
	union dsc_pps52_55_reg_t dsc_pps52_55;
	union dsc_pps56_59_reg_t dsc_pps56_59;
	union dsc_pps60_63_reg_t dsc_pps60_63;
	/* 0x40 - 0x4c */
	union dsc_pps64_67_reg_t dsc_pps64_67;
	union dsc_pps68_71_reg_t dsc_pps68_71;
	union dsc_pps72_75_reg_t dsc_pps72_75;
	union dsc_pps76_79_reg_t dsc_pps76_79;
	/* 0x50 - 0x7c */
	union dsc_pps80_83_reg_t dsc_pps80_83;
	union dsc_pps84_87_reg_t dsc_pps84_87;
	union dsc_reservd_reg_t dsc_res_reg0[10];

	/* 0x80 - 0x8c */
	union dsc_dwc_version_reg_t dsc_version;
	union dsc_dwc_cfg_reg0_t dsc_cfg_reg0;
	union dsc_dwc_cfg_reg1_t dsc_cfg_reg1;
	union dsc_dwc_cfg_reg2_t dsc_cfg_reg2;
	/* 0x90 - 0x9c */
	union dsc_dwc_cfg_reg3_t dsc_cfg_reg3;
	union dsc_reservd_reg_t dsc_res_reg1[3];
	/* 0xa0 - 0xac */
	union dsc_dwc_ctrl0_reg_t dsc_ctrl0;
	union dsc_dwc_ctrl1_reg_t dsc_ctrl1;
	union dsc_dwc_sts0_reg_t dsc_sts0;
	union dsc_dwc_sts1_reg_t dsc_sts1;
	/* 0xb0 - 0xbc */
	union dsc_dwc_sts2_reg_t dsc_sts2;
	union dsc_dwc_sts3_reg_t dsc_sts3;
	union dsc_dwc_sts4_reg_t dsc_sts4;
	union dsc_dwc_sts5_reg_t dsc_sts5;
	/* 0xc0 - 0xcc */
	union dsc_reservd_reg_t dsc_res_reg2[1];
	union dsc_dwc_ers_reg_t dsc_ers;
	union dsc_reservd_reg_t dsc_res_reg3[2];
	/* 0xd0 - 0xdc */
	union dsc_dwc_blk0_reg_t dsc_blk0;
	union dsc_dwc_blk1_reg_t dsc_blk1;
	union dsc_dwc_blk2_reg_t dsc_blk2;
	union dsc_reservd_reg_t dsc_res_reg4[1];
	/* 0xe0 - 0xec */
	union dsc_dwc_sts6_reg_t dsc_sts6;
	union dsc_dwc_sts7_reg_t dsc_sts7;
	union dsc_dwc_sts8_reg_t dsc_sts8;
	union dsc_dwc_sts9_reg_t dsc_sts9;
	/* 0xf0 - 0xf4 */
	union dsc_dwc_sts10_reg_t dsc_sts10;
	union dsc_dwc_sts11_reg_t dsc_sts11;
};

#endif

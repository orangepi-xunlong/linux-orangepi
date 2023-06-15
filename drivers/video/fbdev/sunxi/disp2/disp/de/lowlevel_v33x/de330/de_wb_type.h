/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_WB_TYPE_H_
#define _DE_WB_TYPE_H_

#include "linux/types.h"

#define MININWIDTH 8
#define MININHEIGHT 4
/* support 8192,limit by LCD */
#define MAXINWIDTH 4096
/* support 8192,limit by LCD */
#define MAXINHEIGHT 4096
#define LINE_BUF_LEN 2048
#define LOCFRACBIT 18
#define SCALERPHASE 16

union wb_start_reg {
	u32 dwval;
	struct {
		u32 wb_start:1;
		u32 res0:3;
		u32 soft_reset:1;
		u32 res1:23;
		u32 auto_gate_en:1;
		u32 clk_gate:1;
		u32 res2:2;
	} bits;
};

union wb_gctrl_reg {
	u32 dwval;
	struct {
		u32 fake_start:1;
		u32 res0:3;
		u32 soft_reset:1;
		u32 res1:23;
		u32 auto_gate_en:1;
		u32 clk_gate:1;
		u32 res2:2;
	} bits;
};

union wb_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union wb_crop_coord_reg {
	u32 dwval;
	struct {
		u32 crop_left:13;
		u32 res0:3;
		u32 crop_top:13;
		u32 res1:3;
	} bits;
};

union wb_crop_size_reg {
	u32 dwval;
	struct {
		u32 crop_width:13;
		u32 res0:3;
		u32 crop_height:13;
		u32 res1:3;
	} bits;
};

union wb_addr_reg {
	u32 dwval;
	struct {
		u32 addr:32;
	} bits;
};

union wb_high_addr_reg {
	u32 dwval;
	struct {
		u32 ch0_h_addr:8;
		u32 ch1_h_addr:8;
		u32 ch2_h_addr:8;
		u32 res0:8;
	} bits;
};

union wb_pitch_reg {
	u32 dwval;
	struct {
		u32 pitch:32;
	} bits;
};

union wb_addr_switch_reg {
	u32 dwval;
	struct {
		u32 cur_group:1;
		u32 res0:15;
		u32 auto_switch:1;
		u32 res1:3;
		u32 manual_group:1;
		u32 res2:11;
	} bits;
};

union wb_format_reg {
	u32 dwval;
	struct {
		u32 format:4;
		u32 out_bit:1;
		u32 res0:27;
	} bits;
};

union wb_int_reg {
	u32 dwval;
	struct {
		u32 int_en:1;
		u32 res0:31;
	} bits;
};

union wb_status_reg {
	u32 dwval;
	struct {
		u32 irq:1; /* wb end flag */
		u32 res0:3;
		u32 finish:1;
		u32 overflow:1;
		u32 timeout:1;
		u32 res1:1;
		u32 busy:1;
		u32 res2:23;
	} bits;
};

union wb_sftm_reg {
	u32 dwval;
	struct {
		/* vsync width in slef timing control mode */
		u32 sftm_vs:10;
		u32 res0:22;
	} bits;
};

union wb_bwc_reg {
	u32 dwval;
	struct {
		/* vsync width in slef timing control mode */
		u32 bwc_pos_num:5;
		u32 res0:26;
		u32 bwc_en:1;
	} bits;
};


union wb_bypass_reg {
	u32 dwval;
	struct {
		u32 res0:1;
		u32 cs_en:1;
		u32 fs_en:1;
		u32 res1:29;
	} bits;
};

union wb_cs_reg {
	u32 dwval;
	struct {
		u32 m:13;
		u32 res0:3;
		u32 n:13;
		u32 res1:3;
	} bits;
};

union wb_fs_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union wb_fs_step_reg {
	u32 dwval;
	struct {
		u32 res0:2;
		u32 frac:18;
		u32 intg:2;
		u32 res1:10;
	} bits;
};

union wb_csc_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

union wb_csc_const_reg {
	u32 dwval;
	struct {
		u32 d:10;
		u32 res0:22;
	} bits;
};

union wb_csc_coeff_reg {
	u32 dwval;
	struct {
		u32 c:20;
		u32 res0:12;
	} bits;
};

union wb_coeff_reg {
	u32 dwval;
	struct {
		u32 coef0:8;
		u32 coef1:8;
		u32 coef2:8;
		u32 coef3:8;
	} bits;
};

struct wb_reg {
	union wb_gctrl_reg gctrl;           /* 0x0000 */
	union wb_size_reg size;             /* 0x0004 */
	union wb_crop_coord_reg crop_coord; /* 0x0008 */
	union wb_crop_size_reg crop_size;   /* 0x000c */
	union wb_addr_reg wb_addr_a0;       /* 0x0010 */
	union wb_addr_reg wb_addr_a1;       /* 0x0014 */
	union wb_addr_reg wb_addr_a2;       /* 0x0018 */
	union wb_high_addr_reg wb_addr_ah;  /* 0x001c */
	union wb_addr_reg wb_addr_b0;       /* 0x0020 */
	union wb_addr_reg wb_addr_b1;       /* 0x0024 */
	union wb_addr_reg wb_addr_b2;       /* 0x0028 */
	union wb_high_addr_reg wb_addr_bh;  /* 0x002c */
	union wb_pitch_reg wb_pitch0;       /* 0x0030 */
	union wb_pitch_reg wb_pitch1;       /* 0x0034 */
	u32 res0[2];                        /* 0x0038-0x003c */
	union wb_addr_switch_reg addr_switch;/* 0x0040 */
	union wb_format_reg fmt;            /* 0x0044 */
	union wb_int_reg intr;              /* 0x0048 */
	union wb_status_reg status;         /* 0x004c */
	union wb_sftm_reg sftm;             /* 0x0050 for de331 */
	union wb_bypass_reg bypass;         /* 0x0054 */
	union wb_bwc_reg bwc;               /* 0x0058 for de331*/
	u32 res2[5];                        /* 0x005c-0x006c */
	union wb_cs_reg cs_horz;            /* 0x0070 */
	union wb_cs_reg cs_vert;            /* 0x0074 */
	u32 res3[2];                        /* 0x0078-0x007c */
	union wb_fs_size_reg fs_insize;     /* 0x0080 */
	union wb_fs_size_reg fs_outsize;    /* 0x0084 */
	union wb_fs_step_reg fs_hstep;      /* 0x0088 */
	union wb_fs_step_reg fs_vstep;      /* 0x008c */
	union wb_csc_ctl_reg csc_ctl;       /* 0x0090 */
	union wb_csc_const_reg d0;
	union wb_csc_const_reg d1;
	union wb_csc_const_reg d2;
	union wb_csc_coeff_reg c0[4];       /* 0x00a0 */
	union wb_csc_coeff_reg c1[4];       /* 0x00b0 */
	union wb_csc_coeff_reg c2[4];       /* 0x00c0 */
	u32 res4[4];                        /* 0x00d0-0x00df */
	union wb_start_reg start;           /* 0x00e0 */
	u32 res5[71];                       /* 0x00e4-0x01fc */
	union wb_coeff_reg yhcoeff[16];     /* 0X0200-0x23c */
	u32 res6[16];                       /* 0X0240-0x27c */
	union wb_coeff_reg chcoeff[16];     /* 0X0280-0x2bc */
	u32 res7[16];                       /* 0x02c0-0x02fc */
};

enum wb_output_fmt {
	WB_FORMAT_RGB_888 = 0x0,
	WB_FORMAT_BGR_888 = 0x1,
	WB_FORMAT_ARGB_8888 = 0x4,
	WB_FORMAT_ABGR_8888 = 0x5,
	WB_FORMAT_BGRA_8888 = 0x6,
	WB_FORMAT_RGBA_8888 = 0x7,
	WB_FORMAT_YUV420_P = 0x8,
	WB_FORMAT_YUV420_SP_VUVU = 0xc,
	WB_FORMAT_YUV420_SP_UVUV = 0xd,
};

#endif /* #ifndef _DE_WB_TYPE_H_ */

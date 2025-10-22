/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2023 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_CRC_TYPE_H_
#define _DE_CRC_TYPE_H_

#include <linux/types.h>

union crc_ctl_reg {
	u32 dwval;
	struct {
		u32 crc0_en:1;
		u32 res0:3;
		u32 crc1_en:1;
		u32 res1:3;
		u32 crc2_en:1;
		u32 res2:3;
		u32 crc3_en:1;
		u32 res3:3;
		u32 crc4_en:1;
		u32 res4:3;
		u32 crc5_en:1;
		u32 res5:3;
		u32 crc6_en:1;
		u32 res6:3;
		u32 crc7_en:1;
		u32 res7:3;
	} bits;
};

union crc_irq_ctl_reg {
	u32 dwval;
	struct {
		u32 crc0_irq_en:1;
		u32 res0:3;
		u32 crc1_irq_en:1;
		u32 res1:3;
		u32 crc2_irq_en:1;
		u32 res2:3;
		u32 crc3_irq_en:1;
		u32 res3:3;
		u32 crc4_irq_en:1;
		u32 res4:3;
		u32 crc5_irq_en:1;
		u32 res5:3;
		u32 crc6_irq_en:1;
		u32 res6:3;
		u32 crc7_irq_en:1;
		u32 res7:3;
	} bits;
};

union crc_first_frame_reg {
	u32 dwval;
	struct {
		u32 crc0:1;
		u32 res0:3;
		u32 crc1:1;
		u32 res1:3;
		u32 crc2:1;
		u32 res2:3;
		u32 crc3:1;
		u32 res3:3;
		u32 crc4:1;
		u32 res4:3;
		u32 crc5:1;
		u32 res5:3;
		u32 crc6:1;
		u32 res6:3;
		u32 crc7:1;
		u32 res7:3;
	} bits;
};

union crc_polar_reg {
	u32 dwval;
	struct {
		u32 crc0:1;
		u32 res0:3;
		u32 crc1:1;
		u32 res1:3;
		u32 crc2:1;
		u32 res2:3;
		u32 crc3:1;
		u32 res3:3;
		u32 crc4:1;
		u32 res4:3;
		u32 crc5:1;
		u32 res5:3;
		u32 crc6:1;
		u32 res6:3;
		u32 crc7:1;
		u32 res7:3;
	} bits;
};

union crc_status_reg {
	u32 dwval;
	struct {
		u32 crc0:1;
		u32 res0:3;
		u32 crc1:1;
		u32 res1:3;
		u32 crc2:1;
		u32 res2:3;
		u32 crc3:1;
		u32 res3:3;
		u32 crc4:1;
		u32 res4:3;
		u32 crc5:1;
		u32 res5:3;
		u32 crc6:1;
		u32 res6:3;
		u32 crc7:1;
		u32 res7:3;
	} bits;
};

union crc_val_reg {
	u32 dwval;
	struct {
		u32 val:16;
		u32 res0:16;
	} bits;
};

union crc_win_reg {
	u32 dwval;
	struct {
		u32 start:13;
		u32 res0:3;
		u32 end:13;
		u32 res1:3;
	} bits;
};

union crc_poly_reg {
	u32 dwval;
	struct {
		u32 poly_val:16;
		u32 res0:16;
	} bits;
};

union crc_size_reg {
	u32 dwval;
	struct {
		u32 w:13;
		u32 res0:3;
		u32 h:13;
		u32 res1:3;
	} bits;
};

union crc_step_reg {
	u32 dwval;
	struct {
		u32 compare_step:16;
		u32 res0:16;
	} bits;
};

union crc_frms_reg {
	u32 dwval;
	struct {
		u32 frames:16;
		u32 res0:16;
	} bits;
};

struct crc_region_crc_val {
	union crc_val_reg val0;
	union crc_val_reg val1;
	union crc_val_reg val2;
	u32 res0;
};

struct crc_region_win {
	union crc_win_reg hori;
	union crc_win_reg vert;
	u32 res0[2];
};


struct crc_reg {
	union crc_ctl_reg ctrl;			/* 0x0   */
	union crc_irq_ctl_reg irq_ctrl;		/* 0x4   */
	union crc_first_frame_reg first_frame;	/* 0x8   */
	union crc_polar_reg pol;		/* 0xc   */
	union crc_status_reg status;		/* 0x10  */
	u32 res0[3];				/* 0x14 ~ 0x1d  */
	struct crc_region_crc_val crc_val[8];	/* 0x20 ~ 0x9f  */
	struct crc_region_win win[8];		/* 0xa0 ~ 0x11f */
	union crc_poly_reg poly;		/* 0x120 */
	union crc_size_reg size;		/* 0x124 */
	union crc_frms_reg run_frames;		/* 0x128 */
	union crc_frms_reg run_cnt;		/* 0x12c */
	union crc_step_reg step[8];		/* 0x130 */
};

#endif

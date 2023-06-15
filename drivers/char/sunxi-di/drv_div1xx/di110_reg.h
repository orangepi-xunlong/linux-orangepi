/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef _DI110_REG_H_
#define _DI110_REG_H_

#include <linux/types.h>

union di_ctrl_reg {
	u32 dwval;
	struct {
		u32 start:1;
		u32 reserve0:3;
		u32 auto_clk_gate_en:1;
		u32 reserve1:26;
		u32 reset:1;
	} bits;
};

union di_int_ctrl_reg {
	u32 dwval;
	struct {
		u32 finish_int_en:1;
		u32 reserve0:31;
	} bits;
};

union di_status_reg {
	u32 dwval;
	struct {
		u32 finish_flag:1;
		u32 reserve0:3;
		u32 busy:1;
		u32 reserve1:7;
		u32 cut_blk:4;
		u32 cur_line:10;
		u32 reserve2:2;
		u32 cur_chl:2;
		u32 reserve3:2;
	} bits;
};

union di_version_reg {
	u32 dwval;
	struct {
		u32 ip_version:12;
		u32 reserve0:20;
	} bits;
};

union di_size_reg {
	u32 dwval;
	struct {
		u32 width:11;
		u32 reserve0:5;
		u32 height:11;
		u32 reserve1:5;
	} bits;
};

union di_format_reg {
	u32 dwval;
	struct {
		u32 in_format:3;
		u32 reserve0:1;
		u32 field_order:1;
		u32 reserve1:27;
	} bits;
};

union di_model_para_reg {
	u32 dwval;
	struct {
		u32 di_mode:1;
		u32 reserve0:3;
		u32 output_mode:1;
		u32 reserve1:3;
		u32 feather_detection_en:1;
		u32 reserve2:23;
	} bits;
};

/* for pitch of plane0(Y Component)
 * plane0 of in_fb0
 * plane0 of in_fb1
 */
union di_in_pitch0_reg {
	u32 dwval;
	struct {
		u32 fb0_plane0:12;
		u32 reserve0:4;
		u32 fb1_plane0:12;
		u32 reserve1:4;
	} bits;
};

/* for pitch of plane1(U(or V) Component)
 * plane1 of in_fb0
 * plane1 of in_fb1
 */
union di_in_pitch1_reg {
	u32 dwval;
	struct {
		u32 fb0_plane1:12;
		u32 reserve0:4;
		u32 fb1_plane1:12;
		u32 reserve1:4;
	} bits;
};

/* for pitch of plane2(V(or U) Component)
 * plane2 of in_fb0
 * plane2 of in_fb1
 */
union di_in_pitch2_reg {
	u32 dwval;
	struct {
		u32 fb0_plane2:12;
		u32 reserve0:4;
		u32 fb1_plane2:12;
		u32 reserve1:4;
	} bits;
};

/* for pitch of plane0(Y Component)
 * plane0 of out_fb
 */
union di_out_pitch0_reg {
	u32 dwval;
	struct {
		u32 plane0:12;
		u32 reserve0:20;
	} bits;
};

/* for pitch of plane1(U(or V) Component)
 * plane1 of out_fb
 */
union di_out_pitch1_reg {
	u32 dwval;
	struct {
		u32 plane1:12;
		u32 reserve0:20;
	} bits;
};

/* for pitch of plane2(V(or U) Component)
 * plane2 of out_fb
 */
union di_out_pitch2_reg {
	u32 dwval;
	struct {
		u32 plane2:12;
		u32 reserve0:20;
	} bits;
};

/* for pitch of motion flag*/
union di_mdflag_pitch_reg {
	u32 dwval;
	struct {
		u32 flag_pitch:12;
		u32 reserve0:20;
	} bits;
};

/* plane0(Y Component) address of in_fb0*/
union di_in_frame0_add0_reg {
	u32 dwval;
	struct {
		u32 plane0:32;
	} bits;
};

/* plane1(U(or V) Component) address of in_fb0*/
union di_in_frame0_add1_reg {
	u32 dwval;
	struct {
		u32 plane1:32;
	} bits;
};

/* plane2(V(or U) Component) address of in_fb0*/
union di_in_frame0_add2_reg {
	u32 dwval;
	struct {
		u32 plane2:32;
	} bits;
};

/*extension address of in_fb0*/
union di_in_frame0_add_ext_reg {
	u32 dwval;
	struct {
		u32 plane0_h:8;
		u32 plane1_h:8;
		u32 plane2_h:8;
		u32 reserve0:8;
	} bits;
};

/* plane0(Y Component) address of in_fb1*/
union di_in_frame1_add0_reg {
	u32 dwval;
	struct {
		u32 plane0:32;
	} bits;
};

/* plane1(U(or V) Component) address of in_fb1*/
union di_in_frame1_add1_reg {
	u32 dwval;
	struct {
		u32 plane1:32;
	} bits;
};

/* plane2(V(or U) Component) address of in_fb1*/
union di_in_frame1_add2_reg {
	u32 dwval;
	struct {
		u32 plane2:32;
	} bits;
};

/*extension address of in_fb1*/
union di_in_frame1_add_ext_reg {
	u32 dwval;
	struct {
		u32 plane0_h:8;
		u32 plane1_h:8;
		u32 plane2_h:8;
		u32 reserve0:8;
	} bits;
};

/* plane0(Y Component) address of out_fb0*/
union di_out_frame0_add0_reg {
	u32 dwval;
	struct {
		u32 plane0:32;
	} bits;
};

/* plane1(U(or V) Component) address of out_fb0*/
union di_out_frame0_add1_reg {
	u32 dwval;
	struct {
		u32 plane1:32;
	} bits;
};

/* plane2(V(or U) Component) address of out_fb0*/
union di_out_frame0_add2_reg {
	u32 dwval;
	struct {
		u32 plane2:32;
	} bits;
};

/*extension address of out_fb0*/
union di_out_frame0_add_ext_reg {
	u32 dwval;
	struct {
		u32 plane0_h:8;
		u32 plane1_h:8;
		u32 plane2_h:8;
		u32 reserve0:8;
	} bits;
};

/* plane0(Y Component) address of out_fb1*/
union di_out_frame1_add0_reg {
	u32 dwval;
	struct {
		u32 plane0:32;
	} bits;
};

/* plane1(U(or V) Component) address of out_fb1*/
union di_out_frame1_add1_reg {
	u32 dwval;
	struct {
		u32 plane1:32;
	} bits;
};

/* plane2(V(or U) Component) address of out_fb1*/
union di_out_frame1_add2_reg {
	u32 dwval;
	struct {
		u32 plane2:32;
	} bits;
};

/*extension address of out_fb1*/
union di_out_frame1_add_ext_reg {
	u32 dwval;
	struct {
		u32 plane0_h:8;
		u32 plane1_h:8;
		u32 plane2_h:8;
		u32 reserve0:8;
	} bits;
};

union di_mdflag_add_reg {
	u32 dwval;
	struct {
		u32 flag_add:32;
	} bits;
};

union di_mdflag_add_ext_reg {
	u32 dwval;
	struct {
		u32 flag_add_h:32;
	} bits;
};

union di_luma_para_reg {
	u32 dwval;
	struct {
		u32 min_luma_th:4;
		u32 max_luma_th:4;
		u32 luma_th_shifter:4;
		u32 avg_luma_shifter:4;
		u32 angle_const_th:4;
		u32 max_angle:4;
		u32 slow_motion_fac:4;
		u32 reserve0:4;
	} bits;
};

union di_chroma_para_reg {
	u32 dwval;
	struct {
		u32 chroma_diff_th:4;
		u32 chroma_spatial_th:4;
		u32 reserve0:24;
	} bits;
};

union di_process_time_reg {
	u32 dwval;
	struct {
		u32 time:32;
	} bits;
};

/*register of di110*/
struct di_reg {
	union di_ctrl_reg ctrl;					//0x000
	union di_int_ctrl_reg int_ctrl;				//0x004
	union di_status_reg status;				//0x008
	union di_version_reg version;				//0x00c
	union di_size_reg size;					//0x010
	union di_format_reg format;				//0x014
	union di_model_para_reg model_para;			//0x018

//pitch
	union di_in_pitch0_reg in_pitch0;			//0x01c
	union di_in_pitch1_reg in_pitch1;			//0x020
	union di_in_pitch2_reg in_pitch2;			//0x024

	union di_out_pitch0_reg out_pitch0;			//0x028
	union di_out_pitch1_reg out_pitch1;			//0x02c
	union di_out_pitch2_reg out_pitch2;			//0x030

	union di_mdflag_pitch_reg mdflag_pitch;			//0x034

//address
	union di_in_frame0_add0_reg in_frame0_addr0;		//0x038
	union di_in_frame0_add1_reg in_frame0_addr1;		//0x03c
	union di_in_frame0_add2_reg in_frame0_addr2;		//0x040
	union di_in_frame0_add_ext_reg in_frame0_add_ext;	//0x044

	union di_in_frame1_add0_reg in_frame1_addr0;		//0x048
	union di_in_frame1_add1_reg in_frame1_addr1;		//0x04c
	union di_in_frame1_add2_reg in_frame1_addr2;		//0x050
	union di_in_frame1_add_ext_reg in_frame1_add_ext;	//0x054

	union di_out_frame0_add0_reg out_frame0_addr0;		//0x058
	union di_out_frame0_add1_reg out_frame0_addr1;		//0x05c
	union di_out_frame0_add2_reg out_frame0_addr2;		//0x060
	union di_out_frame0_add_ext_reg out_frame0_add_ext;	//0x064

	union di_out_frame1_add0_reg out_frame1_addr0;		//0x068
	union di_out_frame1_add1_reg out_frame1_addr1;		//0x06c
	union di_out_frame1_add2_reg out_frame1_addr2;		//0x070
	union di_out_frame1_add_ext_reg out_frame1_add_ext;	//0x074

	union di_mdflag_add_reg mdflag_add;			//0x078
	union di_mdflag_add_ext_reg mdflag_add_ext;		//0x7c

	union di_luma_para_reg luma_para;			//0x80
	union di_chroma_para_reg chroma_para;			//0x84

	u32 res[6];						//0x88~0x9c

	union di_process_time_reg process_time;			//0x0a0
};
#endif

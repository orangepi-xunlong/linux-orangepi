// $Module: sc_odma $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Sun, 26 Sep 2021 04:20:45 PM $
//

#ifndef __REG_SC_ODMA_STRUCT_H__
#define __REG_SC_ODMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_blen:1;
		uint32_t rsv_1_7:7;
		/*4'h0 : YUV420 planar
		4'h1 : YUV422 planar
		4'h2 : RGB888 planar
		4'h3 : RGB packed {R,G,B}
		4'h4 : RGB packed {B,G,R}
		4'h5 : Y only
		4'h6 : BF16 (only support in sc_odma)
		4'h7 : reserved
		4'h8 : NV21 {V,U}
		4'h9 : NV12 {U,V}
		4'ha : YUV422-SP1{V,U}
		4'hb : YUV422-SP2{U,V}
		4'hc : YUV2-1 {U,Y,V,Y}
		4'hd : YUV2-2 {V,Y,U,Y}
		4'he : YUV2-3 {Y,U,Y,V}
		4'hf  : YUV2-4 {Y,V,Y,U};*/
		uint32_t reg_fmt_sel:4;
		uint32_t rsv_12_15:4;
		/*;*/
		uint32_t reg_sc_odma_hflip:1;
		/*;*/
		uint32_t reg_sc_odma_vflip:1;
		uint32_t rsv_18_19:2;
		/*;*/
		uint32_t reg_sc_422_avg:1;
		/*;*/
		uint32_t reg_sc_420_avg:1;
		/*0 : UV run away from zero
		1 : UV run to zero;*/
		uint32_t reg_c_round_mode:1;
		/*csc q mode outout BF16 enable
		when this bit set to 1
		1. reg_fmt_sel must set to 0x6
		2. reg_sc_csc_q_mode must set to 1
		and notice aht sc_hsv function would be auto disable ;*/
		uint32_t reg_bf16_en:1;
	};
	uint32_t val;
} sc_odma_odma_reg_00_c;
typedef union {
	struct {
		/*output Y (R) base address;*/
		uint32_t reg_dma_y_base_low_part:32;
	};
	uint32_t val;
} sc_odma_odma_reg_01_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_y_base_high_part:8;
	};
	uint32_t val;
} sc_odma_odma_reg_02_c;
typedef union {
	struct {
		/*putput U (G) base address;*/
		uint32_t reg_dma_u_base_low_part:32;
	};
	uint32_t val;
} sc_odma_odma_reg_03_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_u_base_high_part:8;
	};
	uint32_t val;
} sc_odma_odma_reg_04_c;
typedef union {
	struct {
		/*output V (B) base address;*/
		uint32_t reg_dma_v_base_low_part:32;
	};
	uint32_t val;
} sc_odma_odma_reg_05_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_v_base_high_part:8;
	};
	uint32_t val;
} sc_odma_odma_reg_06_c;
typedef union {
	struct {
		/*line pitch for Y/R/Packet data;*/
		uint32_t reg_dma_y_pitch:24;
	};
	uint32_t val;
} sc_odma_odma_reg_07_c;
typedef union {
	struct {
		/*line pitch for C;*/
		uint32_t reg_dma_c_pitch:24;
	};
	uint32_t val;
} sc_odma_odma_reg_08_c;
typedef union {
	struct {
		/*crop write start point ( in pixel base);*/
		uint32_t reg_dma_x_str:12;
	};
	uint32_t val;
} sc_odma_odma_reg_09_c;
typedef union {
	struct {
		/*crop write start line (line line base);*/
		uint32_t reg_dma_y_str:12;
	};
	uint32_t val;
} sc_odma_odma_reg_10_c;
typedef union {
	struct {
		/*output width, fill sub 1;*/
		uint32_t reg_dma_wd:12;
	};
	uint32_t val;
} sc_odma_odma_reg_11_c;
typedef union {
	struct {
		/*output height, fill sub 1;*/
		uint32_t reg_dma_ht:12;
	};
	uint32_t val;
} sc_odma_odma_reg_12_c;
typedef union {
	struct {
		/*dma debug information;*/
		uint32_t reg_dma_debug:32;
	};
	uint32_t val;
} sc_odma_odma_reg_13_c;
typedef union {
	struct {
		/*line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_line_target:12;
		uint32_t rsv_12_15:4;
		/*line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_line_target_sel:2;
	};
	uint32_t val;
} sc_odma_odma_reg_14_c;
typedef union {
	struct {
		/*cycle line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_cycle_line_target:11;
		uint32_t rsv_11_15:5;
		/*cycle line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_cycle_line_target_sel:2;
	};
	uint32_t val;
} sc_odma_odma_reg_15_c;
typedef union {
	struct {
		/*latch odma y line count;*/
		uint32_t reg_dma_latch_line_cnt:1;
		uint32_t rsv_1_7:7;
		/*latched odma y line count;*/
		uint32_t reg_dma_latched_line_cnt:12;
	};
	uint32_t val;
} sc_odma_odma_reg_16_c;
typedef union {
	struct {
		/*slice buffer mode
		2'h0 : disable
		2'h1 : free-run mode
		2'h2 : frame base mode
		2'h3 : reserved;*/
		uint32_t reg_sb_mode:2;
		/*slice buffer line number, in data Y
		0 : 64 line
		1 : 128 line ;*/
		uint32_t reg_sb_size:1;
		uint32_t rsv_3_7:5;
		/*slice buffer depth;*/
		uint32_t reg_sb_nb:5;
		uint32_t rsv_13_15:3;
		/*slice buffer full point,  reg_sb_full_nb <= reg_sb_nb;*/
		uint32_t reg_sb_full_nb:5;
		uint32_t rsv_21_23:3;
		/*sw sb fifo depth for frame strt;*/
		uint32_t reg_sb_sw_wptr:5;
		uint32_t rsv_29_29:1;
		/*set 1 to over-write frame strat sb fifo write index;*/
		uint32_t reg_sb_set_str:1;
		/*set 1 to clear slice buffer controller;*/
		uint32_t reg_sb_sw_clr:1;
	};
	uint32_t val;
} sc_odma_sb_reg_ctrl_c;
typedef union {
	struct {
		/*U channel SB write pointer;*/
		uint32_t reg_u_sb_wptr_ro:5;
		uint32_t rsv_5_5:1;
		/*U channel SB FIFO full;*/
		uint32_t reg_u_sb_full:1;
		/*U channel SB FIFO empty;*/
		uint32_t reg_u_sb_empty:1;
		/*U channel SB FIFO data pointer;*/
		uint32_t reg_u_sb_dptr_ro:6;
		uint32_t rsv_14_15:2;
		/*V channel SB write pointer;*/
		uint32_t reg_v_sb_wptr_ro:5;
		uint32_t rsv_21_21:1;
		/*V channel SB FIFO full;*/
		uint32_t reg_v_sb_full:1;
		/*V channel SB FIFO empty;*/
		uint32_t reg_v_sb_empty:1;
		/*V channel SB FIFO data pointer;*/
		uint32_t reg_v_sb_dptr_ro:6;
	};
	uint32_t val;
} sc_odma_sb_reg_c_stat_c;
typedef union {
	struct {
		/*Y channel SB write pointer;*/
		uint32_t reg_y_sb_wptr_ro:5;
		uint32_t rsv_5_5:1;
		/*Y channel SB FIFO full;*/
		uint32_t reg_y_sb_full:1;
		/*Y channel SB FIFO empty;*/
		uint32_t reg_y_sb_empty:1;
		/*Y channel SB FIFO data pointer;*/
		uint32_t reg_y_sb_dptr_ro:6;
		uint32_t rsv_14_14:1;
		/*SB mode FIFO full;*/
		uint32_t reg_sb_full:1;
	};
	uint32_t val;
} sc_odma_sb_reg_y_stat_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_sc_csc_en:1;
		/*csc q mode;*/
		uint32_t reg_sc_csc_q_mode:1;
		/*q mode rounding method
		2'b00 : ties to even
		2'b01 : ties away from zero
		2'b1x : truncate;*/
		uint32_t reg_sc_csc_q_drop:2;
		/*RGB to HSV enable ;*/
		uint32_t reg_sc_hsv_en:1;
		/*0 : hsv rounding ties away from zero
		1 : hsv rounding toward -∞;*/
		uint32_t reg_sc_hsv_floor_en:1;
		uint32_t rsv_6_7:2;
		/*csc disable for border (padding) pixel
		0 : apply csc to padding pixel
		1 : don’t apply csc to padding pixel;*/
		uint32_t reg_sc_csc_bd_dis:1;
		/*csc disable for border (padding) pixel
		0 : apply hsv to padding pixel
		1 : don’t apply hsv to padding pixel;*/
		uint32_t reg_sc_hsv_bd_dis:1;
		/*border color type under BF16 & keep border color (reg_sc_csc_bd_dis = 1 )
		0 : border color 0 ~ 255
		1 : border color -128 ~ 127;*/
		uint32_t reg_bf16_bd_type:1;
		/*csc q_mode option
		0 : {gain, offset} = {s.0.13, 8.10}
		1 : {gain, offset} = {s.1.12, 9.9  };*/
		uint32_t reg_sc_csc_q_gain_mode:1;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_00_c;
typedef union {
	struct {
		/*csc matrix coef_00 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c00:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_01 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c01:14;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_01_c;
typedef union {
	struct {
		/*csc matrix coef_02 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c02:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_10 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c10:14;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_02_c;
typedef union {
	struct {
		/*csc matrix coef_11 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c11:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_12 for RGB2YUV,s3.10;*/
		uint32_t reg_sc_csc_r2y_c12:14;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_03_c;
typedef union {
	struct {
		/*csc matrix coef_20 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c20:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_21 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c21:14;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_04_c;
typedef union {
	struct {
		/*csc matrix coef_22 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c22:14;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_05_c;
typedef union {
	struct {
		/*csc add_0 value;*/
		uint32_t reg_sc_csc_r2y_add_0:8;
		/*csc add_1 value;*/
		uint32_t reg_sc_csc_r2y_add_1:8;
		/*csc add_2 value;*/
		uint32_t reg_sc_csc_r2y_add_2:8;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_06_c;
typedef union {
	struct {
		/*csc frac_0 value, for Q mode only, 0.10;*/
		uint32_t reg_sc_csc_r2y_frac_0:10;
		uint32_t rsv_10_15:6;
		/*csc frac_1 value, for Q mode only, 0.10;*/
		uint32_t reg_sc_csc_r2y_frac_1:10;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_07_c;
typedef union {
	struct {
		/*csc frac_2 value, for Q mode only, 0.10;*/
		uint32_t reg_sc_csc_r2y_frac_2:10;
	};
	uint32_t val;
} sc_odma_sc_csc_reg_08_c;
typedef struct {
	volatile sc_odma_odma_reg_00_c odma_reg_00;
	volatile sc_odma_odma_reg_01_c odma_reg_01;
	volatile sc_odma_odma_reg_02_c odma_reg_02;
	volatile sc_odma_odma_reg_03_c odma_reg_03;
	volatile sc_odma_odma_reg_04_c odma_reg_04;
	volatile sc_odma_odma_reg_05_c odma_reg_05;
	volatile sc_odma_odma_reg_06_c odma_reg_06;
	volatile sc_odma_odma_reg_07_c odma_reg_07;
	volatile sc_odma_odma_reg_08_c odma_reg_08;
	volatile sc_odma_odma_reg_09_c odma_reg_09;
	volatile sc_odma_odma_reg_10_c odma_reg_10;
	volatile sc_odma_odma_reg_11_c odma_reg_11;
	volatile sc_odma_odma_reg_12_c odma_reg_12;
	volatile sc_odma_odma_reg_13_c odma_reg_13;
	volatile sc_odma_odma_reg_14_c odma_reg_14;
	volatile sc_odma_odma_reg_15_c odma_reg_15;
	volatile sc_odma_odma_reg_16_c odma_reg_16;
	volatile uint32_t _sb_reg_ctrl_0; // 0x44
	volatile uint32_t _sb_reg_ctrl_1; // 0x48
	volatile uint32_t _sb_reg_ctrl_2; // 0x4C
	volatile sc_odma_sb_reg_ctrl_c sb_reg_ctrl;
	volatile sc_odma_sb_reg_c_stat_c sb_reg_c_stat;
	volatile sc_odma_sb_reg_y_stat_c sb_reg_y_stat;
	volatile uint32_t _sc_csc_reg_00_0; // 0x5C
	volatile uint32_t _sc_csc_reg_00_1; // 0x60
	volatile uint32_t _sc_csc_reg_00_2; // 0x64
	volatile uint32_t _sc_csc_reg_00_3; // 0x68
	volatile uint32_t _sc_csc_reg_00_4; // 0x6C
	volatile uint32_t _sc_csc_reg_00_5; // 0x70
	volatile uint32_t _sc_csc_reg_00_6; // 0x74
	volatile uint32_t _sc_csc_reg_00_7; // 0x78
	volatile uint32_t _sc_csc_reg_00_8; // 0x7C
	volatile uint32_t _sc_csc_reg_00_9; // 0x80
	volatile uint32_t _sc_csc_reg_00_10; // 0x84
	volatile uint32_t _sc_csc_reg_00_11; // 0x88
	volatile uint32_t _sc_csc_reg_00_12; // 0x8C
	volatile uint32_t _sc_csc_reg_00_13; // 0x90
	volatile uint32_t _sc_csc_reg_00_14; // 0x94
	volatile uint32_t _sc_csc_reg_00_15; // 0x98
	volatile uint32_t _sc_csc_reg_00_16; // 0x9C
	volatile uint32_t _sc_csc_reg_00_17; // 0xA0
	volatile uint32_t _sc_csc_reg_00_18; // 0xA4
	volatile uint32_t _sc_csc_reg_00_19; // 0xA8
	volatile uint32_t _sc_csc_reg_00_20; // 0xAC
	volatile uint32_t _sc_csc_reg_00_21; // 0xB0
	volatile uint32_t _sc_csc_reg_00_22; // 0xB4
	volatile uint32_t _sc_csc_reg_00_23; // 0xB8
	volatile uint32_t _sc_csc_reg_00_24; // 0xBC
	volatile uint32_t _sc_csc_reg_00_25; // 0xC0
	volatile uint32_t _sc_csc_reg_00_26; // 0xC4
	volatile uint32_t _sc_csc_reg_00_27; // 0xC8
	volatile uint32_t _sc_csc_reg_00_28; // 0xCC
	volatile uint32_t _sc_csc_reg_00_29; // 0xD0
	volatile uint32_t _sc_csc_reg_00_30; // 0xD4
	volatile uint32_t _sc_csc_reg_00_31; // 0xD8
	volatile uint32_t _sc_csc_reg_00_32; // 0xDC
	volatile uint32_t _sc_csc_reg_00_33; // 0xE0
	volatile uint32_t _sc_csc_reg_00_34; // 0xE4
	volatile uint32_t _sc_csc_reg_00_35; // 0xE8
	volatile uint32_t _sc_csc_reg_00_36; // 0xEC
	volatile uint32_t _sc_csc_reg_00_37; // 0xF0
	volatile uint32_t _sc_csc_reg_00_38; // 0xF4
	volatile uint32_t _sc_csc_reg_00_39; // 0xF8
	volatile uint32_t _sc_csc_reg_00_40; // 0xFC
	volatile sc_odma_sc_csc_reg_00_c sc_csc_reg_00;
	volatile sc_odma_sc_csc_reg_01_c sc_csc_reg_01;
	volatile sc_odma_sc_csc_reg_02_c sc_csc_reg_02;
	volatile sc_odma_sc_csc_reg_03_c sc_csc_reg_03;
	volatile sc_odma_sc_csc_reg_04_c sc_csc_reg_04;
	volatile sc_odma_sc_csc_reg_05_c sc_csc_reg_05;
	volatile sc_odma_sc_csc_reg_06_c sc_csc_reg_06;
	volatile sc_odma_sc_csc_reg_07_c sc_csc_reg_07;
	volatile sc_odma_sc_csc_reg_08_c sc_csc_reg_08;
} sc_odma_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void sc_odma_dump_ini(FILE* fp, sc_odma_c* p) {
	fprintf(fp, "reg_dma_blen = 0x%x\n",p->odma_reg_00.reg_dma_blen);
	fprintf(fp, "reg_fmt_sel = 0x%x\n",p->odma_reg_00.reg_fmt_sel);
	fprintf(fp, "reg_sc_odma_hflip = 0x%x\n",p->odma_reg_00.reg_sc_odma_hflip);
	fprintf(fp, "reg_sc_odma_vflip = 0x%x\n",p->odma_reg_00.reg_sc_odma_vflip);
	fprintf(fp, "reg_sc_422_avg = 0x%x\n",p->odma_reg_00.reg_sc_422_avg);
	fprintf(fp, "reg_sc_420_avg = 0x%x\n",p->odma_reg_00.reg_sc_420_avg);
	fprintf(fp, "reg_c_round_mode = 0x%x\n",p->odma_reg_00.reg_c_round_mode);
	fprintf(fp, "reg_bf16_en = 0x%x\n",p->odma_reg_00.reg_bf16_en);
	fprintf(fp, "reg_dma_y_base_low_part = 0x%x\n",p->odma_reg_01.reg_dma_y_base_low_part);
	fprintf(fp, "reg_dma_y_base_high_part = 0x%x\n",p->odma_reg_02.reg_dma_y_base_high_part);
	fprintf(fp, "reg_dma_u_base_low_part = 0x%x\n",p->odma_reg_03.reg_dma_u_base_low_part);
	fprintf(fp, "reg_dma_u_base_high_part = 0x%x\n",p->odma_reg_04.reg_dma_u_base_high_part);
	fprintf(fp, "reg_dma_v_base_low_part = 0x%x\n",p->odma_reg_05.reg_dma_v_base_low_part);
	fprintf(fp, "reg_dma_v_base_high_part = 0x%x\n",p->odma_reg_06.reg_dma_v_base_high_part);
	fprintf(fp, "reg_dma_y_pitch = 0x%x\n",p->odma_reg_07.reg_dma_y_pitch);
	fprintf(fp, "reg_dma_c_pitch = 0x%x\n",p->odma_reg_08.reg_dma_c_pitch);
	fprintf(fp, "reg_dma_x_str = 0x%x\n",p->odma_reg_09.reg_dma_x_str);
	fprintf(fp, "reg_dma_y_str = 0x%x\n",p->odma_reg_10.reg_dma_y_str);
	fprintf(fp, "reg_dma_wd = 0x%x\n",p->odma_reg_11.reg_dma_wd);
	fprintf(fp, "reg_dma_ht = 0x%x\n",p->odma_reg_12.reg_dma_ht);
	fprintf(fp, "reg_dma_debug = 0x%x\n",p->odma_reg_13.reg_dma_debug);
	fprintf(fp, "reg_dma_int_line_target = 0x%x\n",p->odma_reg_14.reg_dma_int_line_target);
	fprintf(fp, "reg_dma_int_line_target_sel = 0x%x\n",p->odma_reg_14.reg_dma_int_line_target_sel);
	fprintf(fp, "reg_dma_int_cycle_line_target = 0x%x\n",p->odma_reg_15.reg_dma_int_cycle_line_target);
	fprintf(fp, "reg_dma_int_cycle_line_target_sel = 0x%x\n",p->odma_reg_15.reg_dma_int_cycle_line_target_sel);
	fprintf(fp, "reg_dma_latch_line_cnt = 0x%x\n",p->odma_reg_16.reg_dma_latch_line_cnt);
	fprintf(fp, "reg_dma_latched_line_cnt = 0x%x\n",p->odma_reg_16.reg_dma_latched_line_cnt);
	fprintf(fp, "reg_sb_mode = 0x%x\n",p->sb_reg_ctrl.reg_sb_mode);
	fprintf(fp, "reg_sb_size = 0x%x\n",p->sb_reg_ctrl.reg_sb_size);
	fprintf(fp, "reg_sb_nb = 0x%x\n",p->sb_reg_ctrl.reg_sb_nb);
	fprintf(fp, "reg_sb_full_nb = 0x%x\n",p->sb_reg_ctrl.reg_sb_full_nb);
	fprintf(fp, "reg_sb_sw_wptr = 0x%x\n",p->sb_reg_ctrl.reg_sb_sw_wptr);
	fprintf(fp, "reg_sb_set_str = 0x%x\n",p->sb_reg_ctrl.reg_sb_set_str);
	fprintf(fp, "reg_sb_sw_clr = 0x%x\n",p->sb_reg_ctrl.reg_sb_sw_clr);
	fprintf(fp, "reg_u_sb_wptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_wptr_ro);
	fprintf(fp, "reg_u_sb_full = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_full);
	fprintf(fp, "reg_u_sb_empty = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_empty);
	fprintf(fp, "reg_u_sb_dptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_dptr_ro);
	fprintf(fp, "reg_v_sb_wptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_wptr_ro);
	fprintf(fp, "reg_v_sb_full = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_full);
	fprintf(fp, "reg_v_sb_empty = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_empty);
	fprintf(fp, "reg_v_sb_dptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_dptr_ro);
	fprintf(fp, "reg_y_sb_wptr_ro = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_wptr_ro);
	fprintf(fp, "reg_y_sb_full = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_full);
	fprintf(fp, "reg_y_sb_empty = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_empty);
	fprintf(fp, "reg_y_sb_dptr_ro = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_dptr_ro);
	fprintf(fp, "reg_sb_full = 0x%x\n",p->sb_reg_y_stat.reg_sb_full);
	fprintf(fp, "reg_sc_csc_en = 0x%x\n",p->sc_csc_reg_00.reg_sc_csc_en);
	fprintf(fp, "reg_sc_csc_q_mode = 0x%x\n",p->sc_csc_reg_00.reg_sc_csc_q_mode);
	fprintf(fp, "reg_sc_csc_q_drop = 0x%x\n",p->sc_csc_reg_00.reg_sc_csc_q_drop);
	fprintf(fp, "reg_sc_hsv_en = 0x%x\n",p->sc_csc_reg_00.reg_sc_hsv_en);
	fprintf(fp, "reg_sc_hsv_floor_en = 0x%x\n",p->sc_csc_reg_00.reg_sc_hsv_floor_en);
	fprintf(fp, "reg_sc_csc_bd_dis = 0x%x\n",p->sc_csc_reg_00.reg_sc_csc_bd_dis);
	fprintf(fp, "reg_sc_hsv_bd_dis = 0x%x\n",p->sc_csc_reg_00.reg_sc_hsv_bd_dis);
	fprintf(fp, "reg_bf16_bd_type = 0x%x\n",p->sc_csc_reg_00.reg_bf16_bd_type);
	fprintf(fp, "reg_sc_csc_q_gain_mode = 0x%x\n",p->sc_csc_reg_00.reg_sc_csc_q_gain_mode);
	fprintf(fp, "reg_sc_csc_r2y_c00 = 0x%x\n",p->sc_csc_reg_01.reg_sc_csc_r2y_c00);
	fprintf(fp, "reg_sc_csc_r2y_c01 = 0x%x\n",p->sc_csc_reg_01.reg_sc_csc_r2y_c01);
	fprintf(fp, "reg_sc_csc_r2y_c02 = 0x%x\n",p->sc_csc_reg_02.reg_sc_csc_r2y_c02);
	fprintf(fp, "reg_sc_csc_r2y_c10 = 0x%x\n",p->sc_csc_reg_02.reg_sc_csc_r2y_c10);
	fprintf(fp, "reg_sc_csc_r2y_c11 = 0x%x\n",p->sc_csc_reg_03.reg_sc_csc_r2y_c11);
	fprintf(fp, "reg_sc_csc_r2y_c12 = 0x%x\n",p->sc_csc_reg_03.reg_sc_csc_r2y_c12);
	fprintf(fp, "reg_sc_csc_r2y_c20 = 0x%x\n",p->sc_csc_reg_04.reg_sc_csc_r2y_c20);
	fprintf(fp, "reg_sc_csc_r2y_c21 = 0x%x\n",p->sc_csc_reg_04.reg_sc_csc_r2y_c21);
	fprintf(fp, "reg_sc_csc_r2y_c22 = 0x%x\n",p->sc_csc_reg_05.reg_sc_csc_r2y_c22);
	fprintf(fp, "reg_sc_csc_r2y_add_0 = 0x%x\n",p->sc_csc_reg_06.reg_sc_csc_r2y_add_0);
	fprintf(fp, "reg_sc_csc_r2y_add_1 = 0x%x\n",p->sc_csc_reg_06.reg_sc_csc_r2y_add_1);
	fprintf(fp, "reg_sc_csc_r2y_add_2 = 0x%x\n",p->sc_csc_reg_06.reg_sc_csc_r2y_add_2);
	fprintf(fp, "reg_sc_csc_r2y_frac_0 = 0x%x\n",p->sc_csc_reg_07.reg_sc_csc_r2y_frac_0);
	fprintf(fp, "reg_sc_csc_r2y_frac_1 = 0x%x\n",p->sc_csc_reg_07.reg_sc_csc_r2y_frac_1);
	fprintf(fp, "reg_sc_csc_r2y_frac_2 = 0x%x\n",p->sc_csc_reg_08.reg_sc_csc_r2y_frac_2);

}
static void sc_odma_print(sc_odma_c* p) {
    fprintf(stderr, "sc_odma\n");
	fprintf(stderr, "\tODMA_REG_00.reg_dma_blen = 0x%x\n", p->odma_reg_00.reg_dma_blen);
	fprintf(stderr, "\tODMA_REG_00.reg_fmt_sel = 0x%x\n", p->odma_reg_00.reg_fmt_sel);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_odma_hflip = 0x%x\n", p->odma_reg_00.reg_sc_odma_hflip);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_odma_vflip = 0x%x\n", p->odma_reg_00.reg_sc_odma_vflip);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_422_avg = 0x%x\n", p->odma_reg_00.reg_sc_422_avg);
	fprintf(stderr, "\tODMA_REG_00.reg_sc_420_avg = 0x%x\n", p->odma_reg_00.reg_sc_420_avg);
	fprintf(stderr, "\tODMA_REG_00.reg_c_round_mode = 0x%x\n", p->odma_reg_00.reg_c_round_mode);
	fprintf(stderr, "\tODMA_REG_00.reg_bf16_en = 0x%x\n", p->odma_reg_00.reg_bf16_en);
	fprintf(stderr, "\tODMA_REG_01.reg_dma_y_base_low_part = 0x%x\n", p->odma_reg_01.reg_dma_y_base_low_part);
	fprintf(stderr, "\tODMA_REG_02.reg_dma_y_base_high_part = 0x%x\n", p->odma_reg_02.reg_dma_y_base_high_part);
	fprintf(stderr, "\tODMA_REG_03.reg_dma_u_base_low_part = 0x%x\n", p->odma_reg_03.reg_dma_u_base_low_part);
	fprintf(stderr, "\tODMA_REG_04.reg_dma_u_base_high_part = 0x%x\n", p->odma_reg_04.reg_dma_u_base_high_part);
	fprintf(stderr, "\tODMA_REG_05.reg_dma_v_base_low_part = 0x%x\n", p->odma_reg_05.reg_dma_v_base_low_part);
	fprintf(stderr, "\tODMA_REG_06.reg_dma_v_base_high_part = 0x%x\n", p->odma_reg_06.reg_dma_v_base_high_part);
	fprintf(stderr, "\tODMA_REG_07.reg_dma_y_pitch = 0x%x\n", p->odma_reg_07.reg_dma_y_pitch);
	fprintf(stderr, "\tODMA_REG_08.reg_dma_c_pitch = 0x%x\n", p->odma_reg_08.reg_dma_c_pitch);
	fprintf(stderr, "\tODMA_REG_09.reg_dma_x_str = 0x%x\n", p->odma_reg_09.reg_dma_x_str);
	fprintf(stderr, "\tODMA_REG_10.reg_dma_y_str = 0x%x\n", p->odma_reg_10.reg_dma_y_str);
	fprintf(stderr, "\tODMA_REG_11.reg_dma_wd = 0x%x\n", p->odma_reg_11.reg_dma_wd);
	fprintf(stderr, "\tODMA_REG_12.reg_dma_ht = 0x%x\n", p->odma_reg_12.reg_dma_ht);
	fprintf(stderr, "\tODMA_REG_13.reg_dma_debug = 0x%x\n", p->odma_reg_13.reg_dma_debug);
	fprintf(stderr, "\tODMA_REG_14.reg_dma_int_line_target = 0x%x\n", p->odma_reg_14.reg_dma_int_line_target);
	fprintf(stderr, "\tODMA_REG_14.reg_dma_int_line_target_sel = 0x%x\n", p->odma_reg_14.reg_dma_int_line_target_sel);
	fprintf(stderr, "\tODMA_REG_15.reg_dma_int_cycle_line_target = 0x%x\n", p->odma_reg_15.reg_dma_int_cycle_line_target);
	fprintf(stderr, "\tODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x%x\n", p->odma_reg_15.reg_dma_int_cycle_line_target_sel);
	fprintf(stderr, "\tODMA_REG_16.reg_dma_latch_line_cnt = 0x%x\n", p->odma_reg_16.reg_dma_latch_line_cnt);
	fprintf(stderr, "\tODMA_REG_16.reg_dma_latched_line_cnt = 0x%x\n", p->odma_reg_16.reg_dma_latched_line_cnt);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_mode = 0x%x\n", p->sb_reg_ctrl.reg_sb_mode);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_size = 0x%x\n", p->sb_reg_ctrl.reg_sb_size);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_nb = 0x%x\n", p->sb_reg_ctrl.reg_sb_nb);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_full_nb = 0x%x\n", p->sb_reg_ctrl.reg_sb_full_nb);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_sw_wptr = 0x%x\n", p->sb_reg_ctrl.reg_sb_sw_wptr);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_set_str = 0x%x\n", p->sb_reg_ctrl.reg_sb_set_str);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_sw_clr = 0x%x\n", p->sb_reg_ctrl.reg_sb_sw_clr);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_wptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_wptr_ro);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_full = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_full);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_empty = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_empty);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_dptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_dptr_ro);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_wptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_wptr_ro);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_full = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_full);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_empty = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_empty);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_dptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_dptr_ro);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_wptr_ro = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_wptr_ro);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_full = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_full);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_empty = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_empty);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_dptr_ro = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_dptr_ro);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_sb_full = 0x%x\n", p->sb_reg_y_stat.reg_sb_full);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_csc_en = 0x%x\n", p->sc_csc_reg_00.reg_sc_csc_en);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_csc_q_mode = 0x%x\n", p->sc_csc_reg_00.reg_sc_csc_q_mode);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_csc_q_drop = 0x%x\n", p->sc_csc_reg_00.reg_sc_csc_q_drop);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_hsv_en = 0x%x\n", p->sc_csc_reg_00.reg_sc_hsv_en);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_hsv_floor_en = 0x%x\n", p->sc_csc_reg_00.reg_sc_hsv_floor_en);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_csc_bd_dis = 0x%x\n", p->sc_csc_reg_00.reg_sc_csc_bd_dis);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_hsv_bd_dis = 0x%x\n", p->sc_csc_reg_00.reg_sc_hsv_bd_dis);
	fprintf(stderr, "\tsc_csc_reg_00.reg_bf16_bd_type = 0x%x\n", p->sc_csc_reg_00.reg_bf16_bd_type);
	fprintf(stderr, "\tsc_csc_reg_00.reg_sc_csc_q_gain_mode = 0x%x\n", p->sc_csc_reg_00.reg_sc_csc_q_gain_mode);
	fprintf(stderr, "\tsc_csc_reg_01.reg_sc_csc_r2y_c00 = 0x%x\n", p->sc_csc_reg_01.reg_sc_csc_r2y_c00);
	fprintf(stderr, "\tsc_csc_reg_01.reg_sc_csc_r2y_c01 = 0x%x\n", p->sc_csc_reg_01.reg_sc_csc_r2y_c01);
	fprintf(stderr, "\tsc_csc_reg_02.reg_sc_csc_r2y_c02 = 0x%x\n", p->sc_csc_reg_02.reg_sc_csc_r2y_c02);
	fprintf(stderr, "\tsc_csc_reg_02.reg_sc_csc_r2y_c10 = 0x%x\n", p->sc_csc_reg_02.reg_sc_csc_r2y_c10);
	fprintf(stderr, "\tsc_csc_reg_03.reg_sc_csc_r2y_c11 = 0x%x\n", p->sc_csc_reg_03.reg_sc_csc_r2y_c11);
	fprintf(stderr, "\tsc_csc_reg_03.reg_sc_csc_r2y_c12 = 0x%x\n", p->sc_csc_reg_03.reg_sc_csc_r2y_c12);
	fprintf(stderr, "\tsc_csc_reg_04.reg_sc_csc_r2y_c20 = 0x%x\n", p->sc_csc_reg_04.reg_sc_csc_r2y_c20);
	fprintf(stderr, "\tsc_csc_reg_04.reg_sc_csc_r2y_c21 = 0x%x\n", p->sc_csc_reg_04.reg_sc_csc_r2y_c21);
	fprintf(stderr, "\tsc_csc_reg_05.reg_sc_csc_r2y_c22 = 0x%x\n", p->sc_csc_reg_05.reg_sc_csc_r2y_c22);
	fprintf(stderr, "\tsc_csc_reg_06.reg_sc_csc_r2y_add_0 = 0x%x\n", p->sc_csc_reg_06.reg_sc_csc_r2y_add_0);
	fprintf(stderr, "\tsc_csc_reg_06.reg_sc_csc_r2y_add_1 = 0x%x\n", p->sc_csc_reg_06.reg_sc_csc_r2y_add_1);
	fprintf(stderr, "\tsc_csc_reg_06.reg_sc_csc_r2y_add_2 = 0x%x\n", p->sc_csc_reg_06.reg_sc_csc_r2y_add_2);
	fprintf(stderr, "\tsc_csc_reg_07.reg_sc_csc_r2y_frac_0 = 0x%x\n", p->sc_csc_reg_07.reg_sc_csc_r2y_frac_0);
	fprintf(stderr, "\tsc_csc_reg_07.reg_sc_csc_r2y_frac_1 = 0x%x\n", p->sc_csc_reg_07.reg_sc_csc_r2y_frac_1);
	fprintf(stderr, "\tsc_csc_reg_08.reg_sc_csc_r2y_frac_2 = 0x%x\n", p->sc_csc_reg_08.reg_sc_csc_r2y_frac_2);

}
#pragma GCC diagnostic pop
#define DEFINE_SC_ODMA_C(X) \
	 sc_odma_c X = \
{\
	{	/* odma_reg_00.reg_dma_blen = */0x0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.odma_reg_00.reg_fmt_sel = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.odma_reg_00.reg_sc_odma_hflip = */0x0,\
	/*.odma_reg_00.reg_sc_odma_vflip = */0x0,\
	/*uint32_t rsv_18_19:2;=*/0,\
	/*.odma_reg_00.reg_sc_422_avg = */0x0,\
	/*.odma_reg_00.reg_sc_420_avg = */0x0,\
	/*.odma_reg_00.reg_c_round_mode = */0x0,\
	/*.odma_reg_00.reg_bf16_en = */0x0,\
	},\
	{	/*.odma_reg_01.reg_dma_y_base_low_part = */0x0,\
	},\
	{	/*.odma_reg_02.reg_dma_y_base_high_part = */0x0,\
	},\
	{	/*.odma_reg_03.reg_dma_u_base_low_part = */0x0,\
	},\
	{	/*.odma_reg_04.reg_dma_u_base_high_part = */0x0,\
	},\
	{	/*.odma_reg_05.reg_dma_v_base_low_part = */0x0,\
	},\
	{	/*.odma_reg_06.reg_dma_v_base_high_part = */0x0,\
	},\
	{	/*.odma_reg_07.reg_dma_y_pitch = */0x0,\
	},\
	{	/*.odma_reg_08.reg_dma_c_pitch = */0x0,\
	},\
	{	/*.odma_reg_09.reg_dma_x_str = */0x0,\
	},\
	{	/*.odma_reg_10.reg_dma_y_str = */0x0,\
	},\
	{	/*.odma_reg_11.reg_dma_wd = */0x0,\
	},\
	{	/*.odma_reg_12.reg_dma_ht = */0x0,\
	},\
	{	/*.odma_reg_13.reg_dma_debug = */0x0,\
	},\
	{	/*.odma_reg_14.reg_dma_int_line_target = */0x0,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.odma_reg_14.reg_dma_int_line_target_sel = */0x0,\
	},\
	{	/*.odma_reg_15.reg_dma_int_cycle_line_target = */0x0,\
	/*uint32_t rsv_11_15:5;=*/0,\
	/*.odma_reg_15.reg_dma_int_cycle_line_target_sel = */0x0,\
	},\
	{	/*.odma_reg_16.reg_dma_latch_line_cnt = */0x0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.odma_reg_16.reg_dma_latched_line_cnt = */0x0,\
	},\
	{	/*.sb_reg_ctrl.reg_sb_mode = */0x0,\
	/*.sb_reg_ctrl.reg_sb_size = */0x0,\
	/*uint32_t rsv_3_7:5;=*/0,\
	/*.sb_reg_ctrl.reg_sb_nb = */0x3,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.sb_reg_ctrl.reg_sb_full_nb = */0x2,\
	/*uint32_t rsv_21_23:3;=*/0,\
	/*.sb_reg_ctrl.reg_sb_sw_wptr = */0x0,\
	/*uint32_t rsv_29_29:1;=*/0,\
	/*.sb_reg_ctrl.reg_sb_set_str = */0x0,\
	/*.sb_reg_ctrl.reg_sb_sw_clr = */0x0,\
	},\
	{	/*.sb_reg_c_stat.reg_u_sb_wptr_ro = */0,\
	/*uint32_t rsv_5_5:1;=*/0,\
	/*.sb_reg_c_stat.reg_u_sb_full = */0,\
	/*.sb_reg_c_stat.reg_u_sb_empty = */0,\
	/*.sb_reg_c_stat.reg_u_sb_dptr_ro = */0,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.sb_reg_c_stat.reg_v_sb_wptr_ro = */0,\
	/*uint32_t rsv_21_21:1;=*/0,\
	/*.sb_reg_c_stat.reg_v_sb_full = */0,\
	/*.sb_reg_c_stat.reg_v_sb_empty = */0,\
	/*.sb_reg_c_stat.reg_v_sb_dptr_ro = */0,\
	},\
	{	/*.sb_reg_y_stat.reg_y_sb_wptr_ro = */0,\
	/*uint32_t rsv_5_5:1;=*/0,\
	/*.sb_reg_y_stat.reg_y_sb_full = */0,\
	/*.sb_reg_y_stat.reg_y_sb_empty = */0,\
	/*.sb_reg_y_stat.reg_y_sb_dptr_ro = */0,\
	/*uint32_t rsv_14_14:1;=*/0,\
	/*.sb_reg_y_stat.reg_sb_full = */0,\
	},\
	{	/*.sc_csc_reg_00.reg_sc_csc_en = */0x0,\
	/*.sc_csc_reg_00.reg_sc_csc_q_mode = */0x0,\
	/*.sc_csc_reg_00.reg_sc_csc_q_drop = */0x0,\
	/*.sc_csc_reg_00.reg_sc_hsv_en = */0x0,\
	/*.sc_csc_reg_00.reg_sc_hsv_floor_en = */0x0,\
	/*uint32_t rsv_6_7:2;=*/0,\
	/*.sc_csc_reg_00.reg_sc_csc_bd_dis = */0x0,\
	/*.sc_csc_reg_00.reg_sc_hsv_bd_dis = */0x0,\
	/*.sc_csc_reg_00.reg_bf16_bd_type = */0x0,\
	/*.sc_csc_reg_00.reg_sc_csc_q_gain_mode = */0x0,\
	},\
	{	/*.sc_csc_reg_01.reg_sc_csc_r2y_c00 = */0x132,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.sc_csc_reg_01.reg_sc_csc_r2y_c01 = */0x259,\
	},\
	{	/*.sc_csc_reg_02.reg_sc_csc_r2y_c02 = */0x75,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.sc_csc_reg_02.reg_sc_csc_r2y_c10 = */0x1f53,\
	},\
	{	/*.sc_csc_reg_03.reg_sc_csc_r2y_c11 = */0x1ead,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.sc_csc_reg_03.reg_sc_csc_r2y_c12 = */0x200,\
	},\
	{	/*.sc_csc_reg_04.reg_sc_csc_r2y_c20 = */0x200,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.sc_csc_reg_04.reg_sc_csc_r2y_c21 = */0x1e53,\
	},\
	{	/*.sc_csc_reg_05.reg_sc_csc_r2y_c22 = */0x1fad,\
	},\
	{	/*.sc_csc_reg_06.reg_sc_csc_r2y_add_0 = */0x0,\
	/*.sc_csc_reg_06.reg_sc_csc_r2y_add_1 = */0x80,\
	/*.sc_csc_reg_06.reg_sc_csc_r2y_add_2 = */0x80,\
	},\
	{	/*.sc_csc_reg_07.reg_sc_csc_r2y_frac_0 = */0x200,\
	/*uint32_t rsv_10_15:6;=*/0,\
	/*.sc_csc_reg_07.reg_sc_csc_r2y_frac_1 = */0x200,\
	},\
	{	/*.sc_csc_reg_08.reg_sc_csc_r2y_frac_2 = */0x200,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_SC_ODMA_C \
{\
	.odma_reg_00.reg_dma_blen = 0x0,\
	.odma_reg_00.reg_fmt_sel = 0x0,\
	.odma_reg_00.reg_sc_odma_hflip = 0x0,\
	.odma_reg_00.reg_sc_odma_vflip = 0x0,\
	.odma_reg_00.reg_sc_422_avg = 0x0,\
	.odma_reg_00.reg_sc_420_avg = 0x0,\
	.odma_reg_00.reg_c_round_mode = 0x0,\
	.odma_reg_00.reg_bf16_en = 0x0,\
	.odma_reg_01.reg_dma_y_base_low_part = 0x0,\
	.odma_reg_02.reg_dma_y_base_high_part = 0x0,\
	.odma_reg_03.reg_dma_u_base_low_part = 0x0,\
	.odma_reg_04.reg_dma_u_base_high_part = 0x0,\
	.odma_reg_05.reg_dma_v_base_low_part = 0x0,\
	.odma_reg_06.reg_dma_v_base_high_part = 0x0,\
	.odma_reg_07.reg_dma_y_pitch = 0x0,\
	.odma_reg_08.reg_dma_c_pitch = 0x0,\
	.odma_reg_09.reg_dma_x_str = 0x0,\
	.odma_reg_10.reg_dma_y_str = 0x0,\
	.odma_reg_11.reg_dma_wd = 0x0,\
	.odma_reg_12.reg_dma_ht = 0x0,\
	.odma_reg_13.reg_dma_debug = 0x0,\
	.odma_reg_14.reg_dma_int_line_target = 0x0,\
	.odma_reg_14.reg_dma_int_line_target_sel = 0x0,\
	.odma_reg_15.reg_dma_int_cycle_line_target = 0x0,\
	.odma_reg_15.reg_dma_int_cycle_line_target_sel = 0x0,\
	.odma_reg_16.reg_dma_latch_line_cnt = 0x0,\
	.odma_reg_16.reg_dma_latched_line_cnt = 0x0,\
	.sb_reg_ctrl.reg_sb_mode = 0x0,\
	.sb_reg_ctrl.reg_sb_size = 0x0,\
	.sb_reg_ctrl.reg_sb_nb = 0x3,\
	.sb_reg_ctrl.reg_sb_full_nb = 0x2,\
	.sb_reg_ctrl.reg_sb_sw_wptr = 0x0,\
	.sb_reg_ctrl.reg_sb_set_str = 0x0,\
	.sb_reg_ctrl.reg_sb_sw_clr = 0x0,\
	.sc_csc_reg_00.reg_sc_csc_en = 0x0,\
	.sc_csc_reg_00.reg_sc_csc_q_mode = 0x0,\
	.sc_csc_reg_00.reg_sc_csc_q_drop = 0x0,\
	.sc_csc_reg_00.reg_sc_hsv_en = 0x0,\
	.sc_csc_reg_00.reg_sc_hsv_floor_en = 0x0,\
	.sc_csc_reg_00.reg_sc_csc_bd_dis = 0x0,\
	.sc_csc_reg_00.reg_sc_hsv_bd_dis = 0x0,\
	.sc_csc_reg_00.reg_bf16_bd_type = 0x0,\
	.sc_csc_reg_00.reg_sc_csc_q_gain_mode = 0x0,\
	.sc_csc_reg_01.reg_sc_csc_r2y_c00 = 0x132,\
	.sc_csc_reg_01.reg_sc_csc_r2y_c01 = 0x259,\
	.sc_csc_reg_02.reg_sc_csc_r2y_c02 = 0x75,\
	.sc_csc_reg_02.reg_sc_csc_r2y_c10 = 0x1f53,\
	.sc_csc_reg_03.reg_sc_csc_r2y_c11 = 0x1ead,\
	.sc_csc_reg_03.reg_sc_csc_r2y_c12 = 0x200,\
	.sc_csc_reg_04.reg_sc_csc_r2y_c20 = 0x200,\
	.sc_csc_reg_04.reg_sc_csc_r2y_c21 = 0x1e53,\
	.sc_csc_reg_05.reg_sc_csc_r2y_c22 = 0x1fad,\
	.sc_csc_reg_06.reg_sc_csc_r2y_add_0 = 0x0,\
	.sc_csc_reg_06.reg_sc_csc_r2y_add_1 = 0x80,\
	.sc_csc_reg_06.reg_sc_csc_r2y_add_2 = 0x80,\
	.sc_csc_reg_07.reg_sc_csc_r2y_frac_0 = 0x200,\
	.sc_csc_reg_07.reg_sc_csc_r2y_frac_1 = 0x200,\
	.sc_csc_reg_08.reg_sc_csc_r2y_frac_2 = 0x200,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_SC_ODMA_STRUCT_H__

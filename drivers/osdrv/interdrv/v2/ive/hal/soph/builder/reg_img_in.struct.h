// $Module: img_in $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:07:20 PM $
//

#ifndef __REG_IMG_IN_STRUCT_H__
#define __REG_IMG_IN_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*source select
		0 : ISP
		1 : dewap/map_convert
		other : DRAM;*/
		uint32_t reg_src_sel:2;
		uint32_t rsv_2_3:2;
		/*source format
		4'h0 : YUV420 planar
		4'h1 : YUV422 planar
		4'h2 : RGB888 planar
		4'h3 : RGB packed {R,G,B}
		4'h4 : RGB packed {B,G,R}
		4'h5 : Y only
		4'h6 : reserved
		4'h7 : reserved
		4'h8 : NV12
		4'h9 : NV21
		4'ha : YUV422-SP1
		4'hb : YUV422-SP2
		4'hc : YUV2-1 {U,Y,V,Y}
		4'hd : YUV2-2 {V,Y,U,Y}
		4'he : YUV2-3 {Y,U,Y,V}
		4'hf  : YUV2-4 {Y,V,Y,U};*/
		uint32_t reg_fmt_sel:4;
		/*burst length;*/
		uint32_t reg_burst_ln:4;
		/*sw csc force enable;*/
		uint32_t reg_img_csc_en:1;
		/*auto csc enable when input format is set to yuv;*/
		uint32_t reg_auto_csc_en:1;
		uint32_t rsv_14_15:2;
		/*read address 64-byte align enable, only when burst lengh is set to 3,7,11,15;*/
		uint32_t reg_64b_align:1;
		uint32_t rsv_17_30:14;
		/*force clock enable
		0 : enable dynamic clock gating
		1 : disable dynamic clock gating;*/
		uint32_t reg_force_clk_enable:1;
	};
	uint32_t val;
} img_in_reg_00_c;
typedef union {
	struct {
		/*crop x start, pixel base;*/
		uint32_t reg_src_x_str:16;
		/*crop y start, line base;*/
		uint32_t reg_src_y_str:16;
	};
	uint32_t val;
} img_in_reg_01_c;
typedef union {
	struct {
		/*crop image width, count from 0;*/
		uint32_t reg_src_wd:16;
		/*crop image height, count from 0;*/
		uint32_t reg_src_ht:16;
	};
	uint32_t val;
} img_in_reg_02_c;
typedef union {
	struct {
		/*source y/r/pkt pitch, must be 32byte alignment;*/
		uint32_t reg_src_y_pitch:24;
	};
	uint32_t val;
} img_in_reg_03_c;
typedef union {
	struct {
		/*source cb/cr/r/b pitch, must be 32byte alignment;*/
		uint32_t reg_src_c_pitch:24;
	};
	uint32_t val;
} img_in_reg_04_c;
typedef union {
	struct {
		/*trig to force register up;*/
		uint32_t reg_sw_force_up:1;
		/*register update sw_mask
		0 : not mask
		1 : mask ;*/
		uint32_t reg_sw_mask_up:1;
		/*shadow register read control;*/
		uint32_t reg_shrd_sel:1;
	};
	uint32_t val;
} img_in_reg_05_c;
typedef union {
	struct {
		/*dummy read register;*/
		uint32_t reg_dummy_ro:32;
	};
	uint32_t val;
} img_in_reg_06_c;
typedef union {
	struct {
		/*dummy register 0;*/
		uint32_t reg_dummy_0:32;
	};
	uint32_t val;
} img_in_reg_07_c;
typedef union {
	struct {
		/*dummy register 1;*/
		uint32_t reg_dummy_1:32;
	};
	uint32_t val;
} img_in_reg_08_c;
typedef union {
	struct {
		/*source y/r base address, must be 16 byte align
		packet, y only mode , also use this address;*/
		uint32_t reg_src_y_base_b0:32;
	};
	uint32_t val;
} img_in_reg_y_base_0_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_src_y_base_b1:8;
	};
	uint32_t val;
} img_in_reg_y_base_1_c;
typedef union {
	struct {
		/*source u/g base address, must be 16 byte align;*/
		uint32_t reg_src_u_base_b0:32;
	};
	uint32_t val;
} img_in_reg_u_base_0_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_src_u_base_b1:8;
	};
	uint32_t val;
} img_in_reg_u_base_1_c;
typedef union {
	struct {
		/*source v/b base address, must be 16 byte align;*/
		uint32_t reg_src_v_base_b0:32;
	};
	uint32_t val;
} img_in_reg_v_base_0_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_src_v_base_b1:8;
	};
	uint32_t val;
} img_in_reg_v_base_1_c;
typedef union {
	struct {
		/*yuv2rgb c00;*/
		uint32_t reg_c00:14;
		uint32_t rsv_14_15:2;
		/*yuv2rgb c01;*/
		uint32_t reg_c01:14;
	};
	uint32_t val;
} img_in_reg_040_c;
typedef union {
	struct {
		/*yuv2rgb c02;*/
		uint32_t reg_c02:14;
	};
	uint32_t val;
} img_in_reg_044_c;
typedef union {
	struct {
		/*yuv2rgb c10;*/
		uint32_t reg_c10:14;
		uint32_t rsv_14_15:2;
		/*yuv2rgb c11;*/
		uint32_t reg_c11:14;
	};
	uint32_t val;
} img_in_reg_048_c;
typedef union {
	struct {
		/*yuv2rgb c12;*/
		uint32_t reg_c12:14;
	};
	uint32_t val;
} img_in_reg_04c_c;
typedef union {
	struct {
		/*yuv2rgb c20;*/
		uint32_t reg_c20:14;
		uint32_t rsv_14_15:2;
		/*yuv2rgb c21;*/
		uint32_t reg_c21:14;
	};
	uint32_t val;
} img_in_reg_050_c;
typedef union {
	struct {
		/*yuv2rgb c22;*/
		uint32_t reg_c22:14;
	};
	uint32_t val;
} img_in_reg_054_c;
typedef union {
	struct {
		/*yuv2rgb sub0;*/
		uint32_t reg_sub_0:8;
		/*yuv2rgb sub1;*/
		uint32_t reg_sub_1:8;
		/*yuv2rgb sub2;*/
		uint32_t reg_sub_2:8;
	};
	uint32_t val;
} img_in_reg_058_c;
typedef union {
	struct {
		/*yuv2rgb add0;*/
		uint32_t reg_add_0:8;
		/*yuv2rgb add1;*/
		uint32_t reg_add_1:8;
		/*yuv2rgb add2;*/
		uint32_t reg_add_2:8;
	};
	uint32_t val;
} img_in_reg_05c_c;
typedef union {
	struct {
		/*y buffer read threshold;*/
		uint32_t reg_fifo_rd_th_y:8;
		/*y buffer high priority thresh;*/
		uint32_t reg_fifo_pr_th_y:8;
		/*c buffer read threshold;*/
		uint32_t reg_fifo_rd_th_c:8;
		/*c buffer high priority thresh;*/
		uint32_t reg_fifo_pr_th_c:8;
	};
	uint32_t val;
} img_in_reg_060_c;
typedef union {
	struct {
		/*outstanding max number;*/
		uint32_t reg_os_max:4;
	};
	uint32_t val;
} img_in_reg_064_c;
typedef union {
	struct {
		/*debug information;*/
		uint32_t reg_err_fwr_y:1;
		/*debug information;*/
		uint32_t reg_err_fwr_u:1;
		/*debug information;*/
		uint32_t reg_err_fwr_v:1;
		/*debug information;*/
		uint32_t reg_clr_fwr_w1t:1;
		/*debug information;*/
		uint32_t reg_err_erd_y:1;
		/*debug information;*/
		uint32_t reg_err_erd_u:1;
		/*debug information;*/
		uint32_t reg_err_erd_v:1;
		/*debug information;*/
		uint32_t reg_clr_erd_w1t:1;
		/*debug information;*/
		uint32_t reg_lb_full_y:1;
		/*debug information;*/
		uint32_t reg_lb_full_u:1;
		/*debug information;*/
		uint32_t reg_lb_full_v:1;
		uint32_t rsv_11_11:1;
		/*debug information;*/
		uint32_t reg_lb_empty_y:1;
		/*debug information;*/
		uint32_t reg_lb_empty_u:1;
		/*debug information;*/
		uint32_t reg_lb_empty_v:1;
		uint32_t rsv_15_15:1;
		/*IP idle index;*/
		uint32_t reg_ip_idle:1;
		/*IP interupt status;*/
		uint32_t reg_ip_int:1;
		/*sw reset IP;*/
		uint32_t reg_ip_clr_w1t:1;
		/*write 1 to clear interrupt;*/
		uint32_t reg_clr_int_w1t:1;
	};
	uint32_t val;
} img_in_reg_068_c;
typedef union {
	struct {
		/*image_in IP axi interface idle index;*/
		uint32_t reg_axi_idle:1;
		uint32_t rsv_1_7:7;
		/*image_in IP axi interface status;*/
		uint32_t reg_axi_status:8;
	};
	uint32_t val;
} img_in_reg_axi_st_c;
typedef union {
	struct {
		/*B.W. limit window period, unit : clk_axi cycle;*/
		uint32_t reg_bwl_win:10;
		uint32_t rsv_10_15:6;
		/*B.W. limit valid number in reg_bwl_win, unit : Byte;*/
		uint32_t reg_bwl_vld:10;
		uint32_t rsv_26_30:5;
		/*B.W. limit enable;*/
		uint32_t reg_bwl_en:1;
	};
	uint32_t val;
} img_in_reg_bw_limit_c;
typedef union {
	struct {
		/*catch mode for B.W. issue;*/
		uint32_t reg_catch_mode:1;
		/*enable dma_urgent signal;*/
		uint32_t reg_dma_urgent_en:1;
		/*qos criteria selection
		0 : data in buffer + outstanding
		1 : data in buffer only;*/
		uint32_t reg_qos_sel_rr:1;
		uint32_t rsv_3_3:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_act_y:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_act_u:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_act_v:1;
		uint32_t rsv_7_7:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_fail_y:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_fail_u:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_fail_v:1;
	};
	uint32_t val;
} img_in_reg_catch_c;
typedef union {
	struct {
		/*img_in data out check sum;*/
		uint32_t reg_chksum_dat_out:8;
		uint32_t rsv_8_30:23;
		/*checksum function enable;*/
		uint32_t reg_checksum_en:1;
	};
	uint32_t val;
} img_in_reg_chk_ctrl_c;
typedef union {
	struct {
		/*img_in axi read in data checksum;*/
		uint32_t reg_chksum_axi_rd:32;
	};
	uint32_t val;
} img_in_chksum_axi_rd_c;
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
		1 : 128 line
		2 : 192 line
		3 : 256 line ;*/
		uint32_t reg_sb_size:2;
		uint32_t rsv_4_7:4;
		/*slice buffer depth;*/
		uint32_t reg_sb_nb:4;
		uint32_t rsv_12_23:12;
		/*sw sb fifo depth for frame strt;*/
		uint32_t reg_sb_sw_rptr:4;
		uint32_t rsv_28_29:2;
		/*set 1 to over-write frame start fb fifo read index;*/
		uint32_t reg_sb_set_str:1;
		/*set 1 to clear slice buffer control;*/
		uint32_t reg_sb_sw_clr:1;
	};
	uint32_t val;
} img_in_sb_reg_ctrl_c;
typedef union {
	struct {
		/*U channel SB read pointer;*/
		uint32_t reg_u_sb_rptr_ro:4;
		uint32_t rsv_4_5:2;
		/*U channel SB FIFO full;*/
		uint32_t reg_u_sb_full:1;
		/*U channel SB FIFO empty;*/
		uint32_t reg_u_sb_empty:1;
		/*U channel SB FIFO data pointer;*/
		uint32_t reg_u_sb_dptr_ro:5;
		uint32_t rsv_13_15:3;
		/*V channel SB read pointer;*/
		uint32_t reg_v_sb_rptr_ro:4;
		uint32_t rsv_20_21:2;
		/*V channel SB FIFO full;*/
		uint32_t reg_v_sb_full:1;
		/*V channel SB FIFO empty;*/
		uint32_t reg_v_sb_empty:1;
		/*V channel SB FIFO data pointer;*/
		uint32_t reg_v_sb_dptr_ro:5;
	};
	uint32_t val;
} img_in_sb_reg_c_stat_c;
typedef union {
	struct {
		/*Y channel SB read pointer;*/
		uint32_t reg_y_sb_rptr_ro:4;
		uint32_t rsv_4_4:1;
		/*Y channel SB FIFO full;*/
		uint32_t reg_y_sb_full:1;
		/*Y channel SB FIFO empty;*/
		uint32_t reg_y_sb_empty:1;
		uint32_t rsv_7_7:1;
		/*Y channel SB FIFO data pointer;*/
		uint32_t reg_y_sb_dptr_ro:5;
		uint32_t rsv_13_14:2;
		/*SB mode FIFO empty;*/
		uint32_t reg_sb_empty:1;
	};
	uint32_t val;
} img_in_sb_reg_y_stat_c;
typedef struct {
	volatile img_in_reg_00_c reg_00;
	volatile img_in_reg_01_c reg_01;
	volatile img_in_reg_02_c reg_02;
	volatile img_in_reg_03_c reg_03;
	volatile img_in_reg_04_c reg_04;
	volatile img_in_reg_05_c reg_05;
	volatile img_in_reg_06_c reg_06;
	volatile img_in_reg_07_c reg_07;
	volatile img_in_reg_08_c reg_08;
	volatile img_in_reg_y_base_0_c reg_y_base_0;
	volatile img_in_reg_y_base_1_c reg_y_base_1;
	volatile img_in_reg_u_base_0_c reg_u_base_0;
	volatile img_in_reg_u_base_1_c reg_u_base_1;
	volatile img_in_reg_v_base_0_c reg_v_base_0;
	volatile img_in_reg_v_base_1_c reg_v_base_1;
	volatile uint32_t _reg_040_0; // 0x3C
	volatile img_in_reg_040_c reg_040;
	volatile img_in_reg_044_c reg_044;
	volatile img_in_reg_048_c reg_048;
	volatile img_in_reg_04c_c reg_04c;
	volatile img_in_reg_050_c reg_050;
	volatile img_in_reg_054_c reg_054;
	volatile img_in_reg_058_c reg_058;
	volatile img_in_reg_05c_c reg_05c;
	volatile img_in_reg_060_c reg_060;
	volatile img_in_reg_064_c reg_064;
	volatile img_in_reg_068_c reg_068;
	volatile uint32_t _reg_axi_st_0; // 0x6C
	volatile img_in_reg_axi_st_c reg_axi_st;
	volatile img_in_reg_bw_limit_c reg_bw_limit;
	volatile uint32_t _reg_catch_0; // 0x78
	volatile uint32_t _reg_catch_1; // 0x7C
	volatile img_in_reg_catch_c reg_catch;
	volatile img_in_reg_chk_ctrl_c reg_chk_ctrl;
	volatile img_in_chksum_axi_rd_c chksum_axi_rd;
	volatile uint32_t _sb_reg_ctrl_0; // 0x8C
	volatile img_in_sb_reg_ctrl_c sb_reg_ctrl;
	volatile img_in_sb_reg_c_stat_c sb_reg_c_stat;
	volatile img_in_sb_reg_y_stat_c sb_reg_y_stat;
} img_in_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void img_in_dump_ini(FILE* fp, img_in_c* p) {
	fprintf(fp, "reg_src_sel = 0x%x\n",p->reg_00.reg_src_sel);
	fprintf(fp, "reg_fmt_sel = 0x%x\n",p->reg_00.reg_fmt_sel);
	fprintf(fp, "reg_burst_ln = 0x%x\n",p->reg_00.reg_burst_ln);
	fprintf(fp, "reg_img_csc_en = 0x%x\n",p->reg_00.reg_img_csc_en);
	fprintf(fp, "reg_auto_csc_en = 0x%x\n",p->reg_00.reg_auto_csc_en);
	fprintf(fp, "reg_64b_align = 0x%x\n",p->reg_00.reg_64b_align);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->reg_00.reg_force_clk_enable);
	fprintf(fp, "reg_src_x_str = 0x%x\n",p->reg_01.reg_src_x_str);
	fprintf(fp, "reg_src_y_str = 0x%x\n",p->reg_01.reg_src_y_str);
	fprintf(fp, "reg_src_wd = 0x%x\n",p->reg_02.reg_src_wd);
	fprintf(fp, "reg_src_ht = 0x%x\n",p->reg_02.reg_src_ht);
	fprintf(fp, "reg_src_y_pitch = 0x%x\n",p->reg_03.reg_src_y_pitch);
	fprintf(fp, "reg_src_c_pitch = 0x%x\n",p->reg_04.reg_src_c_pitch);
	fprintf(fp, "reg_sw_force_up = 0x%x\n",p->reg_05.reg_sw_force_up);
	fprintf(fp, "reg_sw_mask_up = 0x%x\n",p->reg_05.reg_sw_mask_up);
	fprintf(fp, "reg_shrd_sel = 0x%x\n",p->reg_05.reg_shrd_sel);
	fprintf(fp, "reg_dummy_ro = 0x%x\n",p->reg_06.reg_dummy_ro);
	fprintf(fp, "reg_dummy_0 = 0x%x\n",p->reg_07.reg_dummy_0);
	fprintf(fp, "reg_dummy_1 = 0x%x\n",p->reg_08.reg_dummy_1);
	fprintf(fp, "reg_src_y_base_b0 = 0x%x\n",p->reg_y_base_0.reg_src_y_base_b0);
	fprintf(fp, "reg_src_y_base_b1 = 0x%x\n",p->reg_y_base_1.reg_src_y_base_b1);
	fprintf(fp, "reg_src_u_base_b0 = 0x%x\n",p->reg_u_base_0.reg_src_u_base_b0);
	fprintf(fp, "reg_src_u_base_b1 = 0x%x\n",p->reg_u_base_1.reg_src_u_base_b1);
	fprintf(fp, "reg_src_v_base_b0 = 0x%x\n",p->reg_v_base_0.reg_src_v_base_b0);
	fprintf(fp, "reg_src_v_base_b1 = 0x%x\n",p->reg_v_base_1.reg_src_v_base_b1);
	fprintf(fp, "reg_c00 = 0x%x\n",p->reg_040.reg_c00);
	fprintf(fp, "reg_c01 = 0x%x\n",p->reg_040.reg_c01);
	fprintf(fp, "reg_c02 = 0x%x\n",p->reg_044.reg_c02);
	fprintf(fp, "reg_c10 = 0x%x\n",p->reg_048.reg_c10);
	fprintf(fp, "reg_c11 = 0x%x\n",p->reg_048.reg_c11);
	fprintf(fp, "reg_c12 = 0x%x\n",p->reg_04c.reg_c12);
	fprintf(fp, "reg_c20 = 0x%x\n",p->reg_050.reg_c20);
	fprintf(fp, "reg_c21 = 0x%x\n",p->reg_050.reg_c21);
	fprintf(fp, "reg_c22 = 0x%x\n",p->reg_054.reg_c22);
	fprintf(fp, "reg_sub_0 = 0x%x\n",p->reg_058.reg_sub_0);
	fprintf(fp, "reg_sub_1 = 0x%x\n",p->reg_058.reg_sub_1);
	fprintf(fp, "reg_sub_2 = 0x%x\n",p->reg_058.reg_sub_2);
	fprintf(fp, "reg_add_0 = 0x%x\n",p->reg_05c.reg_add_0);
	fprintf(fp, "reg_add_1 = 0x%x\n",p->reg_05c.reg_add_1);
	fprintf(fp, "reg_add_2 = 0x%x\n",p->reg_05c.reg_add_2);
	fprintf(fp, "reg_fifo_rd_th_y = 0x%x\n",p->reg_060.reg_fifo_rd_th_y);
	fprintf(fp, "reg_fifo_pr_th_y = 0x%x\n",p->reg_060.reg_fifo_pr_th_y);
	fprintf(fp, "reg_fifo_rd_th_c = 0x%x\n",p->reg_060.reg_fifo_rd_th_c);
	fprintf(fp, "reg_fifo_pr_th_c = 0x%x\n",p->reg_060.reg_fifo_pr_th_c);
	fprintf(fp, "reg_os_max = 0x%x\n",p->reg_064.reg_os_max);
	fprintf(fp, "reg_err_fwr_y = 0x%x\n",p->reg_068.reg_err_fwr_y);
	fprintf(fp, "reg_err_fwr_u = 0x%x\n",p->reg_068.reg_err_fwr_u);
	fprintf(fp, "reg_err_fwr_v = 0x%x\n",p->reg_068.reg_err_fwr_v);
	fprintf(fp, "reg_clr_fwr_w1t = 0x%x\n",p->reg_068.reg_clr_fwr_w1t);
	fprintf(fp, "reg_err_erd_y = 0x%x\n",p->reg_068.reg_err_erd_y);
	fprintf(fp, "reg_err_erd_u = 0x%x\n",p->reg_068.reg_err_erd_u);
	fprintf(fp, "reg_err_erd_v = 0x%x\n",p->reg_068.reg_err_erd_v);
	fprintf(fp, "reg_clr_erd_w1t = 0x%x\n",p->reg_068.reg_clr_erd_w1t);
	fprintf(fp, "reg_lb_full_y = 0x%x\n",p->reg_068.reg_lb_full_y);
	fprintf(fp, "reg_lb_full_u = 0x%x\n",p->reg_068.reg_lb_full_u);
	fprintf(fp, "reg_lb_full_v = 0x%x\n",p->reg_068.reg_lb_full_v);
	fprintf(fp, "reg_lb_empty_y = 0x%x\n",p->reg_068.reg_lb_empty_y);
	fprintf(fp, "reg_lb_empty_u = 0x%x\n",p->reg_068.reg_lb_empty_u);
	fprintf(fp, "reg_lb_empty_v = 0x%x\n",p->reg_068.reg_lb_empty_v);
	fprintf(fp, "reg_ip_idle = 0x%x\n",p->reg_068.reg_ip_idle);
	fprintf(fp, "reg_ip_int = 0x%x\n",p->reg_068.reg_ip_int);
	fprintf(fp, "reg_ip_clr_w1t = 0x%x\n",p->reg_068.reg_ip_clr_w1t);
	fprintf(fp, "reg_clr_int_w1t = 0x%x\n",p->reg_068.reg_clr_int_w1t);
	fprintf(fp, "reg_axi_idle = 0x%x\n",p->reg_axi_st.reg_axi_idle);
	fprintf(fp, "reg_axi_status = 0x%x\n",p->reg_axi_st.reg_axi_status);
	fprintf(fp, "reg_bwl_win = 0x%x\n",p->reg_bw_limit.reg_bwl_win);
	fprintf(fp, "reg_bwl_vld = 0x%x\n",p->reg_bw_limit.reg_bwl_vld);
	fprintf(fp, "reg_bwl_en = 0x%x\n",p->reg_bw_limit.reg_bwl_en);
	fprintf(fp, "reg_catch_mode = 0x%x\n",p->reg_catch.reg_catch_mode);
	fprintf(fp, "reg_dma_urgent_en = 0x%x\n",p->reg_catch.reg_dma_urgent_en);
	fprintf(fp, "reg_qos_sel_rr = 0x%x\n",p->reg_catch.reg_qos_sel_rr);
	fprintf(fp, "reg_catch_act_y = 0x%x\n",p->reg_catch.reg_catch_act_y);
	fprintf(fp, "reg_catch_act_u = 0x%x\n",p->reg_catch.reg_catch_act_u);
	fprintf(fp, "reg_catch_act_v = 0x%x\n",p->reg_catch.reg_catch_act_v);
	fprintf(fp, "reg_catch_fail_y = 0x%x\n",p->reg_catch.reg_catch_fail_y);
	fprintf(fp, "reg_catch_fail_u = 0x%x\n",p->reg_catch.reg_catch_fail_u);
	fprintf(fp, "reg_catch_fail_v = 0x%x\n",p->reg_catch.reg_catch_fail_v);
	fprintf(fp, "reg_chksum_dat_out = 0x%x\n",p->reg_chk_ctrl.reg_chksum_dat_out);
	fprintf(fp, "reg_checksum_en = 0x%x\n",p->reg_chk_ctrl.reg_checksum_en);
	fprintf(fp, "reg_chksum_axi_rd = 0x%x\n",p->chksum_axi_rd.reg_chksum_axi_rd);
	fprintf(fp, "reg_sb_mode = 0x%x\n",p->sb_reg_ctrl.reg_sb_mode);
	fprintf(fp, "reg_sb_size = 0x%x\n",p->sb_reg_ctrl.reg_sb_size);
	fprintf(fp, "reg_sb_nb = 0x%x\n",p->sb_reg_ctrl.reg_sb_nb);
	fprintf(fp, "reg_sb_sw_rptr = 0x%x\n",p->sb_reg_ctrl.reg_sb_sw_rptr);
	fprintf(fp, "reg_sb_set_str = 0x%x\n",p->sb_reg_ctrl.reg_sb_set_str);
	fprintf(fp, "reg_sb_sw_clr = 0x%x\n",p->sb_reg_ctrl.reg_sb_sw_clr);
	fprintf(fp, "reg_u_sb_rptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_rptr_ro);
	fprintf(fp, "reg_u_sb_full = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_full);
	fprintf(fp, "reg_u_sb_empty = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_empty);
	fprintf(fp, "reg_u_sb_dptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_u_sb_dptr_ro);
	fprintf(fp, "reg_v_sb_rptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_rptr_ro);
	fprintf(fp, "reg_v_sb_full = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_full);
	fprintf(fp, "reg_v_sb_empty = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_empty);
	fprintf(fp, "reg_v_sb_dptr_ro = 0x%x\n",p->sb_reg_c_stat.reg_v_sb_dptr_ro);
	fprintf(fp, "reg_y_sb_rptr_ro = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_rptr_ro);
	fprintf(fp, "reg_y_sb_full = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_full);
	fprintf(fp, "reg_y_sb_empty = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_empty);
	fprintf(fp, "reg_y_sb_dptr_ro = 0x%x\n",p->sb_reg_y_stat.reg_y_sb_dptr_ro);
	fprintf(fp, "reg_sb_empty = 0x%x\n",p->sb_reg_y_stat.reg_sb_empty);

}
static void img_in_print(img_in_c* p) {
    fprintf(stderr, "img_in\n");
	fprintf(stderr, "\tREG_00.reg_src_sel = 0x%x\n", p->reg_00.reg_src_sel);
	fprintf(stderr, "\tREG_00.reg_fmt_sel = 0x%x\n", p->reg_00.reg_fmt_sel);
	fprintf(stderr, "\tREG_00.reg_burst_ln = 0x%x\n", p->reg_00.reg_burst_ln);
	fprintf(stderr, "\tREG_00.reg_img_csc_en = 0x%x\n", p->reg_00.reg_img_csc_en);
	fprintf(stderr, "\tREG_00.reg_auto_csc_en = 0x%x\n", p->reg_00.reg_auto_csc_en);
	fprintf(stderr, "\tREG_00.reg_64b_align = 0x%x\n", p->reg_00.reg_64b_align);
	fprintf(stderr, "\tREG_00.reg_force_clk_enable = 0x%x\n", p->reg_00.reg_force_clk_enable);
	fprintf(stderr, "\tREG_01.reg_src_x_str = 0x%x\n", p->reg_01.reg_src_x_str);
	fprintf(stderr, "\tREG_01.reg_src_y_str = 0x%x\n", p->reg_01.reg_src_y_str);
	fprintf(stderr, "\tREG_02.reg_src_wd = 0x%x\n", p->reg_02.reg_src_wd);
	fprintf(stderr, "\tREG_02.reg_src_ht = 0x%x\n", p->reg_02.reg_src_ht);
	fprintf(stderr, "\tREG_03.reg_src_y_pitch = 0x%x\n", p->reg_03.reg_src_y_pitch);
	fprintf(stderr, "\tREG_04.reg_src_c_pitch = 0x%x\n", p->reg_04.reg_src_c_pitch);
	fprintf(stderr, "\tREG_05.reg_sw_force_up = 0x%x\n", p->reg_05.reg_sw_force_up);
	fprintf(stderr, "\tREG_05.reg_sw_mask_up = 0x%x\n", p->reg_05.reg_sw_mask_up);
	fprintf(stderr, "\tREG_05.reg_shrd_sel = 0x%x\n", p->reg_05.reg_shrd_sel);
	fprintf(stderr, "\tREG_06.reg_dummy_ro = 0x%x\n", p->reg_06.reg_dummy_ro);
	fprintf(stderr, "\tREG_07.reg_dummy_0 = 0x%x\n", p->reg_07.reg_dummy_0);
	fprintf(stderr, "\tREG_08.reg_dummy_1 = 0x%x\n", p->reg_08.reg_dummy_1);
	fprintf(stderr, "\tREG_Y_BASE_0.reg_src_y_base_b0 = 0x%x\n", p->reg_y_base_0.reg_src_y_base_b0);
	fprintf(stderr, "\tREG_Y_BASE_1.reg_src_y_base_b1 = 0x%x\n", p->reg_y_base_1.reg_src_y_base_b1);
	fprintf(stderr, "\tREG_U_BASE_0.reg_src_u_base_b0 = 0x%x\n", p->reg_u_base_0.reg_src_u_base_b0);
	fprintf(stderr, "\tREG_U_BASE_1.reg_src_u_base_b1 = 0x%x\n", p->reg_u_base_1.reg_src_u_base_b1);
	fprintf(stderr, "\tREG_V_BASE_0.reg_src_v_base_b0 = 0x%x\n", p->reg_v_base_0.reg_src_v_base_b0);
	fprintf(stderr, "\tREG_V_BASE_1.reg_src_v_base_b1 = 0x%x\n", p->reg_v_base_1.reg_src_v_base_b1);
	fprintf(stderr, "\tREG_040.reg_c00 = 0x%x\n", p->reg_040.reg_c00);
	fprintf(stderr, "\tREG_040.reg_c01 = 0x%x\n", p->reg_040.reg_c01);
	fprintf(stderr, "\tREG_044.reg_c02 = 0x%x\n", p->reg_044.reg_c02);
	fprintf(stderr, "\tREG_048.reg_c10 = 0x%x\n", p->reg_048.reg_c10);
	fprintf(stderr, "\tREG_048.reg_c11 = 0x%x\n", p->reg_048.reg_c11);
	fprintf(stderr, "\tREG_04C.reg_c12 = 0x%x\n", p->reg_04c.reg_c12);
	fprintf(stderr, "\tREG_050.reg_c20 = 0x%x\n", p->reg_050.reg_c20);
	fprintf(stderr, "\tREG_050.reg_c21 = 0x%x\n", p->reg_050.reg_c21);
	fprintf(stderr, "\tREG_054.reg_c22 = 0x%x\n", p->reg_054.reg_c22);
	fprintf(stderr, "\tREG_058.reg_sub_0 = 0x%x\n", p->reg_058.reg_sub_0);
	fprintf(stderr, "\tREG_058.reg_sub_1 = 0x%x\n", p->reg_058.reg_sub_1);
	fprintf(stderr, "\tREG_058.reg_sub_2 = 0x%x\n", p->reg_058.reg_sub_2);
	fprintf(stderr, "\tREG_05C.reg_add_0 = 0x%x\n", p->reg_05c.reg_add_0);
	fprintf(stderr, "\tREG_05C.reg_add_1 = 0x%x\n", p->reg_05c.reg_add_1);
	fprintf(stderr, "\tREG_05C.reg_add_2 = 0x%x\n", p->reg_05c.reg_add_2);
	fprintf(stderr, "\tREG_060.reg_fifo_rd_th_y = 0x%x\n", p->reg_060.reg_fifo_rd_th_y);
	fprintf(stderr, "\tREG_060.reg_fifo_pr_th_y = 0x%x\n", p->reg_060.reg_fifo_pr_th_y);
	fprintf(stderr, "\tREG_060.reg_fifo_rd_th_c = 0x%x\n", p->reg_060.reg_fifo_rd_th_c);
	fprintf(stderr, "\tREG_060.reg_fifo_pr_th_c = 0x%x\n", p->reg_060.reg_fifo_pr_th_c);
	fprintf(stderr, "\tREG_064.reg_os_max = 0x%x\n", p->reg_064.reg_os_max);
	fprintf(stderr, "\tREG_068.reg_err_fwr_y = 0x%x\n", p->reg_068.reg_err_fwr_y);
	fprintf(stderr, "\tREG_068.reg_err_fwr_u = 0x%x\n", p->reg_068.reg_err_fwr_u);
	fprintf(stderr, "\tREG_068.reg_err_fwr_v = 0x%x\n", p->reg_068.reg_err_fwr_v);
	fprintf(stderr, "\tREG_068.reg_clr_fwr_w1t = 0x%x\n", p->reg_068.reg_clr_fwr_w1t);
	fprintf(stderr, "\tREG_068.reg_err_erd_y = 0x%x\n", p->reg_068.reg_err_erd_y);
	fprintf(stderr, "\tREG_068.reg_err_erd_u = 0x%x\n", p->reg_068.reg_err_erd_u);
	fprintf(stderr, "\tREG_068.reg_err_erd_v = 0x%x\n", p->reg_068.reg_err_erd_v);
	fprintf(stderr, "\tREG_068.reg_clr_erd_w1t = 0x%x\n", p->reg_068.reg_clr_erd_w1t);
	fprintf(stderr, "\tREG_068.reg_lb_full_y = 0x%x\n", p->reg_068.reg_lb_full_y);
	fprintf(stderr, "\tREG_068.reg_lb_full_u = 0x%x\n", p->reg_068.reg_lb_full_u);
	fprintf(stderr, "\tREG_068.reg_lb_full_v = 0x%x\n", p->reg_068.reg_lb_full_v);
	fprintf(stderr, "\tREG_068.reg_lb_empty_y = 0x%x\n", p->reg_068.reg_lb_empty_y);
	fprintf(stderr, "\tREG_068.reg_lb_empty_u = 0x%x\n", p->reg_068.reg_lb_empty_u);
	fprintf(stderr, "\tREG_068.reg_lb_empty_v = 0x%x\n", p->reg_068.reg_lb_empty_v);
	fprintf(stderr, "\tREG_068.reg_ip_idle = 0x%x\n", p->reg_068.reg_ip_idle);
	fprintf(stderr, "\tREG_068.reg_ip_int = 0x%x\n", p->reg_068.reg_ip_int);
	fprintf(stderr, "\tREG_068.reg_ip_clr_w1t = 0x%x\n", p->reg_068.reg_ip_clr_w1t);
	fprintf(stderr, "\tREG_068.reg_clr_int_w1t = 0x%x\n", p->reg_068.reg_clr_int_w1t);
	fprintf(stderr, "\tREG_AXI_ST.reg_axi_idle = 0x%x\n", p->reg_axi_st.reg_axi_idle);
	fprintf(stderr, "\tREG_AXI_ST.reg_axi_status = 0x%x\n", p->reg_axi_st.reg_axi_status);
	fprintf(stderr, "\tREG_BW_LIMIT.reg_bwl_win = 0x%x\n", p->reg_bw_limit.reg_bwl_win);
	fprintf(stderr, "\tREG_BW_LIMIT.reg_bwl_vld = 0x%x\n", p->reg_bw_limit.reg_bwl_vld);
	fprintf(stderr, "\tREG_BW_LIMIT.reg_bwl_en = 0x%x\n", p->reg_bw_limit.reg_bwl_en);
	fprintf(stderr, "\tREG_CATCH.reg_catch_mode = 0x%x\n", p->reg_catch.reg_catch_mode);
	fprintf(stderr, "\tREG_CATCH.reg_dma_urgent_en = 0x%x\n", p->reg_catch.reg_dma_urgent_en);
	fprintf(stderr, "\tREG_CATCH.reg_qos_sel_rr = 0x%x\n", p->reg_catch.reg_qos_sel_rr);
	fprintf(stderr, "\tREG_CATCH.reg_catch_act_y = 0x%x\n", p->reg_catch.reg_catch_act_y);
	fprintf(stderr, "\tREG_CATCH.reg_catch_act_u = 0x%x\n", p->reg_catch.reg_catch_act_u);
	fprintf(stderr, "\tREG_CATCH.reg_catch_act_v = 0x%x\n", p->reg_catch.reg_catch_act_v);
	fprintf(stderr, "\tREG_CATCH.reg_catch_fail_y = 0x%x\n", p->reg_catch.reg_catch_fail_y);
	fprintf(stderr, "\tREG_CATCH.reg_catch_fail_u = 0x%x\n", p->reg_catch.reg_catch_fail_u);
	fprintf(stderr, "\tREG_CATCH.reg_catch_fail_v = 0x%x\n", p->reg_catch.reg_catch_fail_v);
	fprintf(stderr, "\tREG_CHK_CTRL.reg_chksum_dat_out = 0x%x\n", p->reg_chk_ctrl.reg_chksum_dat_out);
	fprintf(stderr, "\tREG_CHK_CTRL.reg_checksum_en = 0x%x\n", p->reg_chk_ctrl.reg_checksum_en);
	fprintf(stderr, "\tCHKSUM_AXI_RD.reg_chksum_axi_rd = 0x%x\n", p->chksum_axi_rd.reg_chksum_axi_rd);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_mode = 0x%x\n", p->sb_reg_ctrl.reg_sb_mode);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_size = 0x%x\n", p->sb_reg_ctrl.reg_sb_size);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_nb = 0x%x\n", p->sb_reg_ctrl.reg_sb_nb);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_sw_rptr = 0x%x\n", p->sb_reg_ctrl.reg_sb_sw_rptr);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_set_str = 0x%x\n", p->sb_reg_ctrl.reg_sb_set_str);
	fprintf(stderr, "\tSB_REG_CTRL.reg_sb_sw_clr = 0x%x\n", p->sb_reg_ctrl.reg_sb_sw_clr);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_rptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_rptr_ro);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_full = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_full);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_empty = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_empty);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_u_sb_dptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_u_sb_dptr_ro);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_rptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_rptr_ro);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_full = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_full);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_empty = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_empty);
	fprintf(stderr, "\tSB_REG_C_STAT.reg_v_sb_dptr_ro = 0x%x\n", p->sb_reg_c_stat.reg_v_sb_dptr_ro);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_rptr_ro = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_rptr_ro);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_full = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_full);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_empty = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_empty);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_y_sb_dptr_ro = 0x%x\n", p->sb_reg_y_stat.reg_y_sb_dptr_ro);
	fprintf(stderr, "\tSB_REG_Y_STAT.reg_sb_empty = 0x%x\n", p->sb_reg_y_stat.reg_sb_empty);

}
#pragma GCC diagnostic pop
#define DEFINE_img_in_c(X) \
	 img_in_c X = \
{\
	{	/* reg_00.reg_src_sel = */0x1,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.reg_00.reg_fmt_sel = */0x0,\
	/*.reg_00.reg_burst_ln = */0x3,\
	/*.reg_00.reg_img_csc_en = */0x0,\
	/*.reg_00.reg_auto_csc_en = */0x1,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.reg_00.reg_64b_align = */0x1,\
	/*uint32_t rsv_17_30:14;=*/0,\
	/*.reg_00.reg_force_clk_enable = */0x0,\
	},\
	{	/*.reg_01.reg_src_x_str = */0x0,\
	/*.reg_01.reg_src_y_str = */0x0,\
	},\
	{	/*.reg_02.reg_src_wd = */0x13f,\
	/*.reg_02.reg_src_ht = */0xef,\
	},\
	{	/*.reg_03.reg_src_y_pitch = */0x0,\
	},\
	{	/*.reg_04.reg_src_c_pitch = */0x0,\
	},\
	{	/*.reg_05.reg_sw_force_up = */0x0,\
	/*.reg_05.reg_sw_mask_up = */0x0,\
	/*.reg_05.reg_shrd_sel = */0x0,\
	},\
	{	/*.reg_06.reg_dummy_ro = */0,\
	},\
	{	/*.reg_07.reg_dummy_0 = */0x0,\
	},\
	{	/*.reg_08.reg_dummy_1 = */0xffffffff,\
	},\
	{	/*.reg_y_base_0.reg_src_y_base_b0 = */0x0,\
	},\
	{	/*.reg_y_base_1.reg_src_y_base_b1 = */0x0,\
	},\
	{	/*.reg_u_base_0.reg_src_u_base_b0 = */0x0,\
	},\
	{	/*.reg_u_base_1.reg_src_u_base_b1 = */0x0,\
	},\
	{	/*.reg_v_base_0.reg_src_v_base_b0 = */0x0,\
	},\
	{	/*.reg_v_base_1.reg_src_v_base_b1 = */0x0,\
	},\
	{	/*.reg_040.reg_c00 = */0x400,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.reg_040.reg_c01 = */0x0,\
	},\
	{	/*.reg_044.reg_c02 = */0x59c,\
	},\
	{	/*.reg_048.reg_c10 = */0x400,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.reg_048.reg_c11 = */0x2160,\
	},\
	{	/*.reg_04c.reg_c12 = */0x22db,\
	},\
	{	/*.reg_050.reg_c20 = */0x400,\
	/*uint32_t rsv_14_15:2;=*/0,\
	/*.reg_050.reg_c21 = */0x717,\
	},\
	{	/*.reg_054.reg_c22 = */0x0,\
	},\
	{	/*.reg_058.reg_sub_0 = */0x0,\
	/*.reg_058.reg_sub_1 = */0x80,\
	/*.reg_058.reg_sub_2 = */0x80,\
	},\
	{	/*.reg_05c.reg_add_0 = */0x0,\
	/*.reg_05c.reg_add_1 = */0x0,\
	/*.reg_05c.reg_add_2 = */0x0,\
	},\
	{	/*.reg_060.reg_fifo_rd_th_y = */0x20,\
	/*.reg_060.reg_fifo_pr_th_y = */0x10,\
	/*.reg_060.reg_fifo_rd_th_c = */0x20,\
	/*.reg_060.reg_fifo_pr_th_c = */0x10,\
	},\
	{	/*.reg_064.reg_os_max = */0x7,\
	},\
	{	/*.reg_068.reg_err_fwr_y = */0,\
	/*.reg_068.reg_err_fwr_u = */0,\
	/*.reg_068.reg_err_fwr_v = */0,\
	/*.reg_068.reg_clr_fwr_w1t = */0x0,\
	/*.reg_068.reg_err_erd_y = */0,\
	/*.reg_068.reg_err_erd_u = */0,\
	/*.reg_068.reg_err_erd_v = */0,\
	/*.reg_068.reg_clr_erd_w1t = */0x0,\
	/*.reg_068.reg_lb_full_y = */0,\
	/*.reg_068.reg_lb_full_u = */0,\
	/*.reg_068.reg_lb_full_v = */0,\
	/*uint32_t rsv_11_11:1;=*/0,\
	/*.reg_068.reg_lb_empty_y = */0,\
	/*.reg_068.reg_lb_empty_u = */0,\
	/*.reg_068.reg_lb_empty_v = */0,\
	/*uint32_t rsv_15_15:1;=*/0,\
	/*.reg_068.reg_ip_idle = */0,\
	/*.reg_068.reg_ip_int = */0,\
	/*.reg_068.reg_ip_clr_w1t = */0x0,\
	/*.reg_068.reg_clr_int_w1t = */0x0,\
	},\
	{	/*.reg_axi_st.reg_axi_idle = */0,\
	/*uint32_t rsv_1_7:7;=*/0,\
	/*.reg_axi_st.reg_axi_status = */0,\
	},\
	{	/*.reg_bw_limit.reg_bwl_win = */0x0,\
	/*uint32_t rsv_10_15:6;=*/0,\
	/*.reg_bw_limit.reg_bwl_vld = */0x0,\
	/*uint32_t rsv_26_30:5;=*/0,\
	/*.reg_bw_limit.reg_bwl_en = */0x0,\
	},\
	{	/*.reg_catch.reg_catch_mode = */0x0,\
	/*.reg_catch.reg_dma_urgent_en = */0x1,\
	/*.reg_catch.reg_qos_sel_rr = */0x1,\
	/*uint32_t rsv_3_3:1;=*/0,\
	/*.reg_catch.reg_catch_act_y = */0,\
	/*.reg_catch.reg_catch_act_u = */0,\
	/*.reg_catch.reg_catch_act_v = */0,\
	/*uint32_t rsv_7_7:1;=*/0,\
	/*.reg_catch.reg_catch_fail_y = */0,\
	/*.reg_catch.reg_catch_fail_u = */0,\
	/*.reg_catch.reg_catch_fail_v = */0,\
	},\
	{	/*.reg_chk_ctrl.reg_chksum_dat_out = */0,\
	/*uint32_t rsv_8_30:23;=*/0,\
	/*.reg_chk_ctrl.reg_checksum_en = */0x0,\
	},\
	{	/*.chksum_axi_rd.reg_chksum_axi_rd = */0,\
	},\
	{	/*.sb_reg_ctrl.reg_sb_mode = */0x0,\
	/*.sb_reg_ctrl.reg_sb_size = */0x0,\
	/*uint32_t rsv_4_7:4;=*/0,\
	/*.sb_reg_ctrl.reg_sb_nb = */0x3,\
	/*uint32_t rsv_12_23:12;=*/0,\
	/*.sb_reg_ctrl.reg_sb_sw_rptr = */0x0,\
	/*uint32_t rsv_28_29:2;=*/0,\
	/*.sb_reg_ctrl.reg_sb_set_str = */0x0,\
	/*.sb_reg_ctrl.reg_sb_sw_clr = */0x0,\
	},\
	{	/*.sb_reg_c_stat.reg_u_sb_rptr_ro = */0,\
	/*uint32_t rsv_4_5:2;=*/0,\
	/*.sb_reg_c_stat.reg_u_sb_full = */0,\
	/*.sb_reg_c_stat.reg_u_sb_empty = */0,\
	/*.sb_reg_c_stat.reg_u_sb_dptr_ro = */0,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.sb_reg_c_stat.reg_v_sb_rptr_ro = */0,\
	/*uint32_t rsv_20_21:2;=*/0,\
	/*.sb_reg_c_stat.reg_v_sb_full = */0,\
	/*.sb_reg_c_stat.reg_v_sb_empty = */0,\
	/*.sb_reg_c_stat.reg_v_sb_dptr_ro = */0,\
	},\
	{	/*.sb_reg_y_stat.reg_y_sb_rptr_ro = */0,\
	/*uint32_t rsv_4_4:1;=*/0,\
	/*.sb_reg_y_stat.reg_y_sb_full = */0,\
	/*.sb_reg_y_stat.reg_y_sb_empty = */0,\
	/*uint32_t rsv_7_7:1;=*/0,\
	/*.sb_reg_y_stat.reg_y_sb_dptr_ro = */0,\
	/*uint32_t rsv_13_14:2;*/0,\
	/*.sb_reg_y_stat.reg_sb_empty = */0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_img_in_c \
{\
	.reg_00.reg_src_sel = 0x1,\
	.reg_00.reg_fmt_sel = 0x0,\
	.reg_00.reg_burst_ln = 0x3,\
	.reg_00.reg_img_csc_en = 0x0,\
	.reg_00.reg_auto_csc_en = 0x1,\
	.reg_00.reg_64b_align = 0x1,\
	.reg_00.reg_force_clk_enable = 0x0,\
	.reg_01.reg_src_x_str = 0x0,\
	.reg_01.reg_src_y_str = 0x0,\
	.reg_02.reg_src_wd = 0x13f,\
	.reg_02.reg_src_ht = 0xef,\
	.reg_03.reg_src_y_pitch = 0x0,\
	.reg_04.reg_src_c_pitch = 0x0,\
	.reg_05.reg_sw_force_up = 0x0,\
	.reg_05.reg_sw_mask_up = 0x0,\
	.reg_05.reg_shrd_sel = 0x0,\
	.reg_07.reg_dummy_0 = 0x0,\
	.reg_08.reg_dummy_1 = 0xffffffff,\
	.reg_y_base_0.reg_src_y_base_b0 = 0x0,\
	.reg_y_base_1.reg_src_y_base_b1 = 0x0,\
	.reg_u_base_0.reg_src_u_base_b0 = 0x0,\
	.reg_u_base_1.reg_src_u_base_b1 = 0x0,\
	.reg_v_base_0.reg_src_v_base_b0 = 0x0,\
	.reg_v_base_1.reg_src_v_base_b1 = 0x0,\
	.reg_040.reg_c00 = 0x400,\
	.reg_040.reg_c01 = 0x0,\
	.reg_044.reg_c02 = 0x59c,\
	.reg_048.reg_c10 = 0x400,\
	.reg_048.reg_c11 = 0x2160,\
	.reg_04c.reg_c12 = 0x22db,\
	.reg_050.reg_c20 = 0x400,\
	.reg_050.reg_c21 = 0x717,\
	.reg_054.reg_c22 = 0x0,\
	.reg_058.reg_sub_0 = 0x0,\
	.reg_058.reg_sub_1 = 0x80,\
	.reg_058.reg_sub_2 = 0x80,\
	.reg_05c.reg_add_0 = 0x0,\
	.reg_05c.reg_add_1 = 0x0,\
	.reg_05c.reg_add_2 = 0x0,\
	.reg_060.reg_fifo_rd_th_y = 0x20,\
	.reg_060.reg_fifo_pr_th_y = 0x10,\
	.reg_060.reg_fifo_rd_th_c = 0x20,\
	.reg_060.reg_fifo_pr_th_c = 0x10,\
	.reg_064.reg_os_max = 0xf,\
	.reg_068.reg_clr_fwr_w1t = 0x0,\
	.reg_068.reg_clr_erd_w1t = 0x0,\
	.reg_068.reg_ip_clr_w1t = 0x0,\
	.reg_068.reg_clr_int_w1t = 0x0,\
	.reg_bw_limit.reg_bwl_win = 0x0,\
	.reg_bw_limit.reg_bwl_vld = 0x0,\
	.reg_bw_limit.reg_bwl_en = 0x0,\
	.reg_catch.reg_catch_mode = 0x0,\
	.reg_catch.reg_dma_urgent_en = 0x1,\
	.reg_catch.reg_qos_sel_rr = 0x1,\
	.reg_chk_ctrl.reg_checksum_en = 0x0,\
	.sb_reg_ctrl.reg_sb_mode = 0x0,\
	.sb_reg_ctrl.reg_sb_size = 0x0,\
	.sb_reg_ctrl.reg_sb_nb = 0x3,\
	.sb_reg_ctrl.reg_sb_sw_rptr = 0x0,\
	.sb_reg_ctrl.reg_sb_set_str = 0x0,\
	.sb_reg_ctrl.reg_sb_sw_clr = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IMG_IN_STRUCT_H__
// $Module: ive_sad $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Tue, 07 Dec 2021 11:00:55 AM $
//

#ifndef __REG_IVE_SAD_STRUCT_H__
#define __REG_IVE_SAD_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*SAD grid size
		0: 4x4
		1: 8x8
		2: 16x16
		3: 32x32;*/
		uint32_t reg_sad_enmode:2;
		uint32_t rsv_2_3:2;
		/*SAD out ctrl
		3'd1: thr = (sad[15:8]>threshold)? reg_sad_u8bit_max : reg_sad_u8but_min;
		else : thr = (sad>threshold)? reg_sad_u8bit_max : reg_sad_u8bit_min
		3'd1,3'd3: sad = {8'd0,sad[15:8]};
		else : sad = sad;*/
		uint32_t reg_sad_out_ctrl:3;
		uint32_t rsv_7_7:1;
		/*SAD 16bit threshold;*/
		uint32_t reg_sad_u16bit_thr:16;
		/*[0:1];*/
		uint32_t reg_sad_shdw_sel:1;
	};
	uint32_t val;
} ive_sad_reg_sad_00_c;
typedef union {
	struct {
		/*SAD 8bit max;*/
		uint32_t reg_sad_u8bit_max:8;
		/*SAD 8bit min;*/
		uint32_t reg_sad_u8bit_min:8;
	};
	uint32_t val;
} ive_sad_reg_sad_01_c;
typedef union {
	struct {
		/*SAD enable; 0:disable, 1:enable;*/
		uint32_t reg_sad_enable:1;
	};
	uint32_t val;
} ive_sad_reg_sad_02_c;
typedef union {
	struct {
		/*0: no force; 1:force clk;*/
		uint32_t reg_force_clk_enable:1;
	};
	uint32_t val;
} ive_sad_reg_sad_03_c;
typedef union {
	struct {
		/*prob grid_v num;*/
		uint32_t reg_prob_grid_v:12;
		/*prob grid_h num;*/
		uint32_t reg_prob_grid_h:12;
		/*prob line in grid[0:15];*/
		uint32_t reg_prob_pix_v:4;
		/*prob pix in grid[0:15];*/
		uint32_t reg_prob_pix_h:4;
	};
	uint32_t val;
} ive_sad_reg_sad_04_c;
typedef union {
	struct {
		/*sum of prev lins;*/
		uint32_t reg_prob_prev_sum:16;
		/*prob pix_0 value;*/
		uint32_t reg_prob_curr_pix_0:8;
		/*prob pix_1 value;*/
		uint32_t reg_prob_curr_pix_1:8;
	};
	uint32_t val;
} ive_sad_reg_sad_05_c;
typedef union {
	struct {
		/*prob enable;*/
		uint32_t reg_prob_en:1;
	};
	uint32_t val;
} ive_sad_reg_sad_06_c;
typedef struct {
	volatile ive_sad_reg_sad_00_c reg_sad_00;
	volatile ive_sad_reg_sad_01_c reg_sad_01;
	volatile ive_sad_reg_sad_02_c reg_sad_02;
	volatile ive_sad_reg_sad_03_c reg_sad_03;
	volatile ive_sad_reg_sad_04_c reg_sad_04;
	volatile ive_sad_reg_sad_05_c reg_sad_05;
	volatile ive_sad_reg_sad_06_c reg_sad_06;
} ive_sad_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_sad_dump_ini(FILE* fp, ive_sad_c* p) {
	fprintf(fp, "reg_sad_enmode = 0x%x\n",p->reg_sad_00.reg_sad_enmode);
	fprintf(fp, "reg_sad_out_ctrl = 0x%x\n",p->reg_sad_00.reg_sad_out_ctrl);
	fprintf(fp, "reg_sad_u16bit_thr = 0x%x\n",p->reg_sad_00.reg_sad_u16bit_thr);
	fprintf(fp, "reg_sad_shdw_sel = 0x%x\n",p->reg_sad_00.reg_sad_shdw_sel);
	fprintf(fp, "reg_sad_u8bit_max = 0x%x\n",p->reg_sad_01.reg_sad_u8bit_max);
	fprintf(fp, "reg_sad_u8bit_min = 0x%x\n",p->reg_sad_01.reg_sad_u8bit_min);
	fprintf(fp, "reg_sad_enable = 0x%x\n",p->reg_sad_02.reg_sad_enable);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->reg_sad_03.reg_force_clk_enable);
	fprintf(fp, "reg_prob_grid_v = 0x%x\n",p->reg_sad_04.reg_prob_grid_v);
	fprintf(fp, "reg_prob_grid_h = 0x%x\n",p->reg_sad_04.reg_prob_grid_h);
	fprintf(fp, "reg_prob_pix_v = 0x%x\n",p->reg_sad_04.reg_prob_pix_v);
	fprintf(fp, "reg_prob_pix_h = 0x%x\n",p->reg_sad_04.reg_prob_pix_h);
	fprintf(fp, "reg_prob_prev_sum = 0x%x\n",p->reg_sad_05.reg_prob_prev_sum);
	fprintf(fp, "reg_prob_curr_pix_0 = 0x%x\n",p->reg_sad_05.reg_prob_curr_pix_0);
	fprintf(fp, "reg_prob_curr_pix_1 = 0x%x\n",p->reg_sad_05.reg_prob_curr_pix_1);
	fprintf(fp, "reg_prob_en = 0x%x\n",p->reg_sad_06.reg_prob_en);

}
static void ive_sad_print(ive_sad_c* p) {
    fprintf(stderr, "ive_sad\n");
	fprintf(stderr, "\treg_sad_00.reg_sad_enmode = 0x%x\n", p->reg_sad_00.reg_sad_enmode);
	fprintf(stderr, "\treg_sad_00.reg_sad_out_ctrl = 0x%x\n", p->reg_sad_00.reg_sad_out_ctrl);
	fprintf(stderr, "\treg_sad_00.reg_sad_u16bit_thr = 0x%x\n", p->reg_sad_00.reg_sad_u16bit_thr);
	fprintf(stderr, "\treg_sad_00.reg_sad_shdw_sel = 0x%x\n", p->reg_sad_00.reg_sad_shdw_sel);
	fprintf(stderr, "\treg_sad_01.reg_sad_u8bit_max = 0x%x\n", p->reg_sad_01.reg_sad_u8bit_max);
	fprintf(stderr, "\treg_sad_01.reg_sad_u8bit_min = 0x%x\n", p->reg_sad_01.reg_sad_u8bit_min);
	fprintf(stderr, "\treg_sad_02.reg_sad_enable = 0x%x\n", p->reg_sad_02.reg_sad_enable);
	fprintf(stderr, "\treg_sad_03.reg_force_clk_enable = 0x%x\n", p->reg_sad_03.reg_force_clk_enable);
	fprintf(stderr, "\treg_sad_04.reg_prob_grid_v = 0x%x\n", p->reg_sad_04.reg_prob_grid_v);
	fprintf(stderr, "\treg_sad_04.reg_prob_grid_h = 0x%x\n", p->reg_sad_04.reg_prob_grid_h);
	fprintf(stderr, "\treg_sad_04.reg_prob_pix_v = 0x%x\n", p->reg_sad_04.reg_prob_pix_v);
	fprintf(stderr, "\treg_sad_04.reg_prob_pix_h = 0x%x\n", p->reg_sad_04.reg_prob_pix_h);
	fprintf(stderr, "\treg_sad_05.reg_prob_prev_sum = 0x%x\n", p->reg_sad_05.reg_prob_prev_sum);
	fprintf(stderr, "\treg_sad_05.reg_prob_curr_pix_0 = 0x%x\n", p->reg_sad_05.reg_prob_curr_pix_0);
	fprintf(stderr, "\treg_sad_05.reg_prob_curr_pix_1 = 0x%x\n", p->reg_sad_05.reg_prob_curr_pix_1);
	fprintf(stderr, "\treg_sad_06.reg_prob_en = 0x%x\n", p->reg_sad_06.reg_prob_en);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_SAD_C(X) \
	 ive_sad_c X = \
{\
	{	/* reg_sad_00.reg_sad_enmode = */0x0,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.reg_sad_00.reg_sad_out_ctrl = */0x0,\
	/*uint32_t rsv_7_7:1;=*/0,\
	/*.reg_sad_00.reg_sad_u16bit_thr = */0x0,\
	/*.reg_sad_00.reg_sad_shdw_sel = */0x1,\
	},\
	{	/*.reg_sad_01.reg_sad_u8bit_max = */0x0,\
	/*.reg_sad_01.reg_sad_u8bit_min = */0x0,\
	},\
	{	/*.reg_sad_02.reg_sad_enable = */0x0,\
	},\
	{	/*.reg_sad_03.reg_force_clk_enable = */0x0,\
	},\
	{	/*.reg_sad_04.reg_prob_grid_v = */0x0,\
	/*.reg_sad_04.reg_prob_grid_h = */0x0,\
	/*.reg_sad_04.reg_prob_pix_v = */0x0,\
	/*.reg_sad_04.reg_prob_pix_h = */0x0,\
	},\
	{	/*.reg_sad_05.reg_prob_prev_sum = */0x0,\
	/*.reg_sad_05.reg_prob_curr_pix_0 = */0x0,\
	/*.reg_sad_05.reg_prob_curr_pix_1 = */0x0,\
	},\
	{	/*.reg_sad_06.reg_prob_en = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_SAD_C \
{\
	.reg_sad_00.reg_sad_enmode = 0x0,\
	.reg_sad_00.reg_sad_out_ctrl = 0x0,\
	.reg_sad_00.reg_sad_u16bit_thr = 0x0,\
	.reg_sad_00.reg_sad_shdw_sel = 0x1,\
	.reg_sad_01.reg_sad_u8bit_max = 0x0,\
	.reg_sad_01.reg_sad_u8bit_min = 0x0,\
	.reg_sad_02.reg_sad_enable = 0x0,\
	.reg_sad_03.reg_force_clk_enable = 0x0,\
	.reg_sad_04.reg_prob_grid_v = 0x0,\
	.reg_sad_04.reg_prob_grid_h = 0x0,\
	.reg_sad_04.reg_prob_pix_v = 0x0,\
	.reg_sad_04.reg_prob_pix_h = 0x0,\
	.reg_sad_05.reg_prob_prev_sum = 0x0,\
	.reg_sad_05.reg_prob_curr_pix_0 = 0x0,\
	.reg_sad_05.reg_prob_curr_pix_1 = 0x0,\
	.reg_sad_06.reg_prob_en = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_SAD_STRUCT_H__

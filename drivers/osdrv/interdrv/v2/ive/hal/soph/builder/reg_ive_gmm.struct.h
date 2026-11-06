// $Module: ive_gmm $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Tue, 07 Dec 2021 11:00:32 AM $
//

#ifndef __REG_IVE_GMM_STRUCT_H__
#define __REG_IVE_GMM_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*if(frm_cnt> 500) reg_gmm_learn_rate = 131; otherwise, reg_gmm_learn_rate = 65535/(frm_cnt+1);*/
		uint32_t reg_gmm_learn_rate:16;
		/*unsigned 16b, threshold for weighted sum ratio;*/
		uint32_t reg_gmm_bg_ratio:16;
	};
	uint32_t val;
} ive_gmm_reg_gmm_0_c;
typedef union {
	struct {
		/*unsigned 8.8f;*/
		uint32_t reg_gmm_var_thr:16;
	};
	uint32_t val;
} ive_gmm_reg_gmm_1_c;
typedef union {
	struct {
		/*reg_gmm_noise_var = 225*1024*ch_num(rgb:3,y_only:1);*/
		uint32_t reg_gmm_noise_var:32;
	};
	uint32_t val;
} ive_gmm_reg_gmm_2_c;
typedef union {
	struct {
		/*reg_gmm_max_thr = 2000*1024*ch_num(rgb:3,y_only:1);*/
		uint32_t reg_gmm_max_var:32;
	};
	uint32_t val;
} ive_gmm_reg_gmm_3_c;
typedef union {
	struct {
		/*reg_gmm_min_thr = 200*1024*ch_num(rgb:3,y_only:1);*/
		uint32_t reg_gmm_min_var:32;
	};
	uint32_t val;
} ive_gmm_reg_gmm_4_c;
typedef union {
	struct {
		/*unsigned 32b;*/
		uint32_t reg_gmm_init_weight:32;
	};
	uint32_t val;
} ive_gmm_reg_gmm_5_c;
typedef union {
	struct {
		/*0: enable detect_shadow; 1: disable detect shadow;*/
		uint32_t reg_gmm_detect_shadow:8;
		/*unsigned 8b;*/
		uint32_t reg_gmm_shadow_thr:8;
		/*unsigned 8b;*/
		uint32_t reg_gmm_sns_factor:8;
	};
	uint32_t val;
} ive_gmm_reg_gmm_6_c;
typedef union {
	struct {
		/*unsigned 16b; reg_gmm2_life_update_factor = 0xffff;*/
		uint32_t reg_gmm2_life_update_factor:16;
		/*;*/
		uint32_t reg_gmm2_var_rate:16;
	};
	uint32_t val;
} ive_gmm_reg_gmm_7_c;
typedef union {
	struct {
		/*unsigned 16b; reg_gmm2_freq_update_factor = (frm_cnt >= 500) ? 0xffa0 : 0xfc00;*/
		uint32_t reg_gmm2_freq_redu_factor:16;
		/*unsigned 9.8f; (16x16)<<7;*/
		uint32_t reg_gmm2_max_var:16;
	};
	uint32_t val;
} ive_gmm_reg_gmm_8_c;
typedef union {
	struct {
		/*unsigned 9.8f; (8x8)<<7;*/
		uint32_t reg_gmm2_min_var:16;
		/*unsigned 16b;*/
		uint32_t reg_gmm2_freq_add_factor:16;
	};
	uint32_t val;
} ive_gmm_reg_gmm_9_c;
typedef union {
	struct {
		/*unsigned 16b;*/
		uint32_t reg_gmm2_freq_init:16;
		/*unsigned 16b;*/
		uint32_t reg_gmm2_freq_thr:16;
	};
	uint32_t val;
} ive_gmm_reg_gmm_10_c;
typedef union {
	struct {
		/*unsigned 16b;*/
		uint32_t reg_gmm2_life_thr:16;
		/*unsigned 8b;*/
		uint32_t reg_gmm2_sns_factor:8;
	};
	uint32_t val;
} ive_gmm_reg_gmm_11_c;
typedef union {
	struct {
		/*unsigned 16b; reg_gmm2_factor = (frm_cnt>500) ? 0x0408: (0xffff/frm_cnt);*/
		uint32_t reg_gmm2_factor:16;
		/*1: factor = reg_gmm2_factor[15:8]; 0: factor = reg_gmm2_life_update_factor;*/
		uint32_t reg_gmm2_life_update_factor_mode:1;
		uint32_t rsv_17_19:3;
		/*1: factor = reg_gmm2_factor[7:0]; 0: factor = reg_gmm2_sns_factor;*/
		uint32_t reg_gmm2_sns_factor_mode:1;
	};
	uint32_t val;
} ive_gmm_reg_gmm_12_c;
typedef union {
	struct {
		/*00: gmm/gmm2 disable; 01: gmm enable/gmm2 disable; 10: gmm2 enable/gmm disable; 11: gmm enable/gmm2 disable;*/
		uint32_t reg_gmm_gmm2_enable:2;
		uint32_t rsv_2_3:2;
		/*0:rgb; 1:yonly;*/
		uint32_t reg_gmm_gmm2_yonly:1;
		uint32_t rsv_5_7:3;
		/*[0:1];*/
		uint32_t reg_gmm_gmm2_shdw_sel:1;
		uint32_t rsv_9_11:3;
		/*0: no force; 1:force clk;*/
		uint32_t reg_force_clk_enable:1;
		uint32_t rsv_13_15:3;
		/*unsigned 3b,
		if(reg_gmm_gmm2_enable == 2'b01) model_num = [3:5];
		if(reg_gmm_gmm2_enable == 2'b10) model_num = [1:5];
		else don't care;*/
		uint32_t reg_gmm_gmm2_model_num:3;
		uint32_t rsv_19_19:1;
		/*prob model sel[0:4], choose one model to prob;*/
		uint32_t reg_prob_model_sel:3;
		uint32_t rsv_23_23:1;
		/*prob model byte sel,[0:15], choose one model byte to prob
		if(byte_sel == i), prob model_byte[i], I = [0:11]
		else if(byte_sel == 12) prob bg data,
		else if(byte_sel== 13) prob fg data,
		else if(byte_sel == 14) prob match info;*/
		uint32_t reg_prob_byte_sel:4;
		/*prob bg data sel;*/
		uint32_t reg_prob_bg_sel:2;
		/*prob enable;*/
		uint32_t reg_prob_en:1;
	};
	uint32_t val;
} ive_gmm_reg_gmm_13_c;
typedef union {
	struct {
		/*line to prob;*/
		uint32_t reg_prob_line:12;
		/*pix to prob;*/
		uint32_t reg_prob_pix:12;
		/*prob byte data;*/
		uint32_t reg_prob_byte_data:8;
	};
	uint32_t val;
} ive_gmm_reg_gmm_14_c;
typedef struct {
	volatile ive_gmm_reg_gmm_0_c reg_gmm_0;
	volatile ive_gmm_reg_gmm_1_c reg_gmm_1;
	volatile ive_gmm_reg_gmm_2_c reg_gmm_2;
	volatile ive_gmm_reg_gmm_3_c reg_gmm_3;
	volatile ive_gmm_reg_gmm_4_c reg_gmm_4;
	volatile ive_gmm_reg_gmm_5_c reg_gmm_5;
	volatile ive_gmm_reg_gmm_6_c reg_gmm_6;
	volatile uint32_t _reg_gmm_7_0; // 0x1C
	volatile ive_gmm_reg_gmm_7_c reg_gmm_7;
	volatile ive_gmm_reg_gmm_8_c reg_gmm_8;
	volatile ive_gmm_reg_gmm_9_c reg_gmm_9;
	volatile ive_gmm_reg_gmm_10_c reg_gmm_10;
	volatile ive_gmm_reg_gmm_11_c reg_gmm_11;
	volatile ive_gmm_reg_gmm_12_c reg_gmm_12;
	volatile ive_gmm_reg_gmm_13_c reg_gmm_13;
	volatile ive_gmm_reg_gmm_14_c reg_gmm_14;
} ive_gmm_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_gmm_dump_ini(FILE* fp, ive_gmm_c* p) {
	fprintf(fp, "reg_gmm_learn_rate = 0x%x\n",p->reg_gmm_0.reg_gmm_learn_rate);
	fprintf(fp, "reg_gmm_bg_ratio = 0x%x\n",p->reg_gmm_0.reg_gmm_bg_ratio);
	fprintf(fp, "reg_gmm_var_thr = 0x%x\n",p->reg_gmm_1.reg_gmm_var_thr);
	fprintf(fp, "reg_gmm_noise_var = 0x%x\n",p->reg_gmm_2.reg_gmm_noise_var);
	fprintf(fp, "reg_gmm_max_var = 0x%x\n",p->reg_gmm_3.reg_gmm_max_var);
	fprintf(fp, "reg_gmm_min_var = 0x%x\n",p->reg_gmm_4.reg_gmm_min_var);
	fprintf(fp, "reg_gmm_init_weight = 0x%x\n",p->reg_gmm_5.reg_gmm_init_weight);
	fprintf(fp, "reg_gmm_detect_shadow = 0x%x\n",p->reg_gmm_6.reg_gmm_detect_shadow);
	fprintf(fp, "reg_gmm_shadow_thr = 0x%x\n",p->reg_gmm_6.reg_gmm_shadow_thr);
	fprintf(fp, "reg_gmm_sns_factor = 0x%x\n",p->reg_gmm_6.reg_gmm_sns_factor);
	fprintf(fp, "reg_gmm2_life_update_factor = 0x%x\n",p->reg_gmm_7.reg_gmm2_life_update_factor);
	fprintf(fp, "reg_gmm2_var_rate = 0x%x\n",p->reg_gmm_7.reg_gmm2_var_rate);
	fprintf(fp, "reg_gmm2_freq_redu_factor = 0x%x\n",p->reg_gmm_8.reg_gmm2_freq_redu_factor);
	fprintf(fp, "reg_gmm2_max_var = 0x%x\n",p->reg_gmm_8.reg_gmm2_max_var);
	fprintf(fp, "reg_gmm2_min_var = 0x%x\n",p->reg_gmm_9.reg_gmm2_min_var);
	fprintf(fp, "reg_gmm2_freq_add_factor = 0x%x\n",p->reg_gmm_9.reg_gmm2_freq_add_factor);
	fprintf(fp, "reg_gmm2_freq_init = 0x%x\n",p->reg_gmm_10.reg_gmm2_freq_init);
	fprintf(fp, "reg_gmm2_freq_thr = 0x%x\n",p->reg_gmm_10.reg_gmm2_freq_thr);
	fprintf(fp, "reg_gmm2_life_thr = 0x%x\n",p->reg_gmm_11.reg_gmm2_life_thr);
	fprintf(fp, "reg_gmm2_sns_factor = 0x%x\n",p->reg_gmm_11.reg_gmm2_sns_factor);
	fprintf(fp, "reg_gmm2_factor = 0x%x\n",p->reg_gmm_12.reg_gmm2_factor);
	fprintf(fp, "reg_gmm2_life_update_factor_mode = 0x%x\n",p->reg_gmm_12.reg_gmm2_life_update_factor_mode);
	fprintf(fp, "reg_gmm2_sns_factor_mode = 0x%x\n",p->reg_gmm_12.reg_gmm2_sns_factor_mode);
	fprintf(fp, "reg_gmm_gmm2_enable = 0x%x\n",p->reg_gmm_13.reg_gmm_gmm2_enable);
	fprintf(fp, "reg_gmm_gmm2_yonly = 0x%x\n",p->reg_gmm_13.reg_gmm_gmm2_yonly);
	fprintf(fp, "reg_gmm_gmm2_shdw_sel = 0x%x\n",p->reg_gmm_13.reg_gmm_gmm2_shdw_sel);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->reg_gmm_13.reg_force_clk_enable);
	fprintf(fp, "reg_gmm_gmm2_model_num = 0x%x\n",p->reg_gmm_13.reg_gmm_gmm2_model_num);
	fprintf(fp, "reg_prob_model_sel = 0x%x\n",p->reg_gmm_13.reg_prob_model_sel);
	fprintf(fp, "reg_prob_byte_sel = 0x%x\n",p->reg_gmm_13.reg_prob_byte_sel);
	fprintf(fp, "reg_prob_bg_sel = 0x%x\n",p->reg_gmm_13.reg_prob_bg_sel);
	fprintf(fp, "reg_prob_en = 0x%x\n",p->reg_gmm_13.reg_prob_en);
	fprintf(fp, "reg_prob_line = 0x%x\n",p->reg_gmm_14.reg_prob_line);
	fprintf(fp, "reg_prob_pix = 0x%x\n",p->reg_gmm_14.reg_prob_pix);
	fprintf(fp, "reg_prob_byte_data = 0x%x\n",p->reg_gmm_14.reg_prob_byte_data);

}
static void ive_gmm_print(ive_gmm_c* p) {
    fprintf(stderr, "ive_gmm\n");
	fprintf(stderr, "\treg_gmm_0.reg_gmm_learn_rate = 0x%x\n", p->reg_gmm_0.reg_gmm_learn_rate);
	fprintf(stderr, "\treg_gmm_0.reg_gmm_bg_ratio = 0x%x\n", p->reg_gmm_0.reg_gmm_bg_ratio);
	fprintf(stderr, "\treg_gmm_1.reg_gmm_var_thr = 0x%x\n", p->reg_gmm_1.reg_gmm_var_thr);
	fprintf(stderr, "\treg_gmm_2.reg_gmm_noise_var = 0x%x\n", p->reg_gmm_2.reg_gmm_noise_var);
	fprintf(stderr, "\treg_gmm_3.reg_gmm_max_var = 0x%x\n", p->reg_gmm_3.reg_gmm_max_var);
	fprintf(stderr, "\treg_gmm_4.reg_gmm_min_var = 0x%x\n", p->reg_gmm_4.reg_gmm_min_var);
	fprintf(stderr, "\treg_gmm_5.reg_gmm_init_weight = 0x%x\n", p->reg_gmm_5.reg_gmm_init_weight);
	fprintf(stderr, "\treg_gmm_6.reg_gmm_detect_shadow = 0x%x\n", p->reg_gmm_6.reg_gmm_detect_shadow);
	fprintf(stderr, "\treg_gmm_6.reg_gmm_shadow_thr = 0x%x\n", p->reg_gmm_6.reg_gmm_shadow_thr);
	fprintf(stderr, "\treg_gmm_6.reg_gmm_sns_factor = 0x%x\n", p->reg_gmm_6.reg_gmm_sns_factor);
	fprintf(stderr, "\treg_gmm_7.reg_gmm2_life_update_factor = 0x%x\n", p->reg_gmm_7.reg_gmm2_life_update_factor);
	fprintf(stderr, "\treg_gmm_7.reg_gmm2_var_rate = 0x%x\n", p->reg_gmm_7.reg_gmm2_var_rate);
	fprintf(stderr, "\treg_gmm_8.reg_gmm2_freq_redu_factor = 0x%x\n", p->reg_gmm_8.reg_gmm2_freq_redu_factor);
	fprintf(stderr, "\treg_gmm_8.reg_gmm2_max_var = 0x%x\n", p->reg_gmm_8.reg_gmm2_max_var);
	fprintf(stderr, "\treg_gmm_9.reg_gmm2_min_var = 0x%x\n", p->reg_gmm_9.reg_gmm2_min_var);
	fprintf(stderr, "\treg_gmm_9.reg_gmm2_freq_add_factor = 0x%x\n", p->reg_gmm_9.reg_gmm2_freq_add_factor);
	fprintf(stderr, "\treg_gmm_10.reg_gmm2_freq_init = 0x%x\n", p->reg_gmm_10.reg_gmm2_freq_init);
	fprintf(stderr, "\treg_gmm_10.reg_gmm2_freq_thr = 0x%x\n", p->reg_gmm_10.reg_gmm2_freq_thr);
	fprintf(stderr, "\treg_gmm_11.reg_gmm2_life_thr = 0x%x\n", p->reg_gmm_11.reg_gmm2_life_thr);
	fprintf(stderr, "\treg_gmm_11.reg_gmm2_sns_factor = 0x%x\n", p->reg_gmm_11.reg_gmm2_sns_factor);
	fprintf(stderr, "\treg_gmm_12.reg_gmm2_factor = 0x%x\n", p->reg_gmm_12.reg_gmm2_factor);
	fprintf(stderr, "\treg_gmm_12.reg_gmm2_life_update_factor_mode = 0x%x\n", p->reg_gmm_12.reg_gmm2_life_update_factor_mode);
	fprintf(stderr, "\treg_gmm_12.reg_gmm2_sns_factor_mode = 0x%x\n", p->reg_gmm_12.reg_gmm2_sns_factor_mode);
	fprintf(stderr, "\treg_gmm_13.reg_gmm_gmm2_enable = 0x%x\n", p->reg_gmm_13.reg_gmm_gmm2_enable);
	fprintf(stderr, "\treg_gmm_13.reg_gmm_gmm2_yonly = 0x%x\n", p->reg_gmm_13.reg_gmm_gmm2_yonly);
	fprintf(stderr, "\treg_gmm_13.reg_gmm_gmm2_shdw_sel = 0x%x\n", p->reg_gmm_13.reg_gmm_gmm2_shdw_sel);
	fprintf(stderr, "\treg_gmm_13.reg_force_clk_enable = 0x%x\n", p->reg_gmm_13.reg_force_clk_enable);
	fprintf(stderr, "\treg_gmm_13.reg_gmm_gmm2_model_num = 0x%x\n", p->reg_gmm_13.reg_gmm_gmm2_model_num);
	fprintf(stderr, "\treg_gmm_13.reg_prob_model_sel = 0x%x\n", p->reg_gmm_13.reg_prob_model_sel);
	fprintf(stderr, "\treg_gmm_13.reg_prob_byte_sel = 0x%x\n", p->reg_gmm_13.reg_prob_byte_sel);
	fprintf(stderr, "\treg_gmm_13.reg_prob_bg_sel = 0x%x\n", p->reg_gmm_13.reg_prob_bg_sel);
	fprintf(stderr, "\treg_gmm_13.reg_prob_en = 0x%x\n", p->reg_gmm_13.reg_prob_en);
	fprintf(stderr, "\treg_gmm_14.reg_prob_line = 0x%x\n", p->reg_gmm_14.reg_prob_line);
	fprintf(stderr, "\treg_gmm_14.reg_prob_pix = 0x%x\n", p->reg_gmm_14.reg_prob_pix);
	fprintf(stderr, "\treg_gmm_14.reg_prob_byte_data = 0x%x\n", p->reg_gmm_14.reg_prob_byte_data);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_GMM_C(X) \
	 ive_gmm_c X = \
{\
	{	/* reg_gmm_0.reg_gmm_learn_rate = */0xffff,\
	/*.reg_gmm_0.reg_gmm_bg_ratio = */0xb333,\
	},\
	{	/*.reg_gmm_1.reg_gmm_var_thr = */0x640,\
	},\
	{	/*.reg_gmm_2.reg_gmm_noise_var = */0xa8c00,\
	},\
	{	/*.reg_gmm_3.reg_gmm_max_var = */0x5dc000,\
	},\
	{	/*.reg_gmm_4.reg_gmm_min_var = */0x96000,\
	},\
	{	/*.reg_gmm_5.reg_gmm_init_weight = */0xccd,\
	},\
	{	/*.reg_gmm_6.reg_gmm_detect_shadow = */0x1,\
	/*.reg_gmm_6.reg_gmm_shadow_thr = */0x0,\
	/*.reg_gmm_6.reg_gmm_sns_factor = */0x8,\
	},\
	{	/*.reg_gmm_7.reg_gmm2_life_update_factor = */0xffff,\
	/*.reg_gmm_7.reg_gmm2_var_rate = */0x1,\
	},\
	{	/*.reg_gmm_8.reg_gmm2_freq_redu_factor = */0xff00,\
	/*.reg_gmm_8.reg_gmm2_max_var = */0x8000,\
	},\
	{	/*.reg_gmm_9.reg_gmm2_min_var = */0x2000,\
	/*.reg_gmm_9.reg_gmm2_freq_add_factor = */0xef,\
	},\
	{	/*.reg_gmm_10.reg_gmm2_freq_init = */0x4e20,\
	/*.reg_gmm_10.reg_gmm2_freq_thr = */0x2ee0,\
	},\
	{	/*.reg_gmm_11.reg_gmm2_life_thr = */0x1388,\
	/*.reg_gmm_11.reg_gmm2_sns_factor = */0x8,\
	},\
	{	/*.reg_gmm_12.reg_gmm2_factor = */0xffff,\
	/*.reg_gmm_12.reg_gmm2_life_update_factor_mode = */0x1,\
	/*uint32_t rsv_17_19:3;=*/0,\
	/*.reg_gmm_12.reg_gmm2_sns_factor_mode = */0x1,\
	},\
	{	/*.reg_gmm_13.reg_gmm_gmm2_enable = */0x0,\
	/*uint32_t rsv_2_3:2;=*/0,\
	/*.reg_gmm_13.reg_gmm_gmm2_yonly = */0x1,\
	/*uint32_t rsv_5_7:3;=*/0,\
	/*.reg_gmm_13.reg_gmm_gmm2_shdw_sel = */0x1,\
	/*uint32_t rsv_9_11:3;=*/0,\
	/*.reg_gmm_13.reg_force_clk_enable = */0x0,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.reg_gmm_13.reg_gmm_gmm2_model_num = */0x3,\
	/*uint32_t rsv_19_19:1;=*/0,\
	/*.reg_gmm_13.reg_prob_model_sel = */0x0,\
	/*uint32_t rsv_23_23:1;=*/0,\
	/*.reg_gmm_13.reg_prob_byte_sel = */0x0,\
	/*.reg_gmm_13.reg_prob_bg_sel = */0x0,\
	/*.reg_gmm_13.reg_prob_en = */0x0,\
	},\
	{	/*.reg_gmm_14.reg_prob_line = */0x0,\
	/*.reg_gmm_14.reg_prob_pix = */0x0,\
	/*.reg_gmm_14.reg_prob_byte_data = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_GMM_C \
{\
	.reg_gmm_0.reg_gmm_learn_rate = 0xffff,\
	.reg_gmm_0.reg_gmm_bg_ratio = 0xb333,\
	.reg_gmm_1.reg_gmm_var_thr = 0x640,\
	.reg_gmm_2.reg_gmm_noise_var = 0xa8c00,\
	.reg_gmm_3.reg_gmm_max_var = 0x5dc000,\
	.reg_gmm_4.reg_gmm_min_var = 0x96000,\
	.reg_gmm_5.reg_gmm_init_weight = 0xccd,\
	.reg_gmm_6.reg_gmm_detect_shadow = 0x1,\
	.reg_gmm_6.reg_gmm_shadow_thr = 0x0,\
	.reg_gmm_6.reg_gmm_sns_factor = 0x8,\
	.reg_gmm_7.reg_gmm2_life_update_factor = 0xffff,\
	.reg_gmm_7.reg_gmm2_var_rate = 0x1,\
	.reg_gmm_8.reg_gmm2_freq_redu_factor = 0xff00,\
	.reg_gmm_8.reg_gmm2_max_var = 0x8000,\
	.reg_gmm_9.reg_gmm2_min_var = 0x2000,\
	.reg_gmm_9.reg_gmm2_freq_add_factor = 0xef,\
	.reg_gmm_10.reg_gmm2_freq_init = 0x4e20,\
	.reg_gmm_10.reg_gmm2_freq_thr = 0x2ee0,\
	.reg_gmm_11.reg_gmm2_life_thr = 0x1388,\
	.reg_gmm_11.reg_gmm2_sns_factor = 0x8,\
	.reg_gmm_12.reg_gmm2_factor = 0xffff,\
	.reg_gmm_12.reg_gmm2_life_update_factor_mode = 0x1,\
	.reg_gmm_12.reg_gmm2_sns_factor_mode = 0x1,\
	.reg_gmm_13.reg_gmm_gmm2_enable = 0x0,\
	.reg_gmm_13.reg_gmm_gmm2_yonly = 0x1,\
	.reg_gmm_13.reg_gmm_gmm2_shdw_sel = 0x1,\
	.reg_gmm_13.reg_force_clk_enable = 0x0,\
	.reg_gmm_13.reg_gmm_gmm2_model_num = 0x3,\
	.reg_gmm_13.reg_prob_model_sel = 0x0,\
	.reg_gmm_13.reg_prob_byte_sel = 0x0,\
	.reg_gmm_13.reg_prob_bg_sel = 0x0,\
	.reg_gmm_13.reg_prob_en = 0x0,\
	.reg_gmm_14.reg_prob_line = 0x0,\
	.reg_gmm_14.reg_prob_pix = 0x0,\
	.reg_gmm_14.reg_prob_byte_data = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_GMM_STRUCT_H__

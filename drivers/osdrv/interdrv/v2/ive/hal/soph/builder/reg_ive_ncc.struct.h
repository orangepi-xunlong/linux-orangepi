// $Module: ive_ncc $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Thu, 25 Nov 2021 03:58:50 PM $
//

#ifndef __REG_IVE_NCC_STRUCT_H__
#define __REG_IVE_NCC_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*numerator[31:0], read_only;*/
		uint32_t reg_numerator_l:32;
	};
	uint32_t val;
} ive_ncc_reg_ncc_00_c;
typedef union {
	struct {
		/*numerator[39:32], read_only;*/
		uint32_t reg_numerator_h:8;
	};
	uint32_t val;
} ive_ncc_reg_ncc_01_c;
typedef union {
	struct {
		/*quadsum0[31:0], read_only;*/
		uint32_t reg_quadsum0_l:32;
	};
	uint32_t val;
} ive_ncc_reg_ncc_02_c;
typedef union {
	struct {
		/*quadsum0[39:32], read_only;*/
		uint32_t reg_quadsum0_h:8;
	};
	uint32_t val;
} ive_ncc_reg_ncc_03_c;
typedef union {
	struct {
		/*quadsum1[31:0], read_only;*/
		uint32_t reg_quadsum1_l:32;
	};
	uint32_t val;
} ive_ncc_reg_ncc_04_c;
typedef union {
	struct {
		/*quadsum1[39:32], read_only;*/
		uint32_t reg_quadsum1_h:8;
	};
	uint32_t val;
} ive_ncc_reg_ncc_05_c;
typedef union {
	struct {
		/*reg_crop_enable;*/
		uint32_t reg_crop_enable:1;
	};
	uint32_t val;
} ive_ncc_reg_ncc_06_c;
typedef union {
	struct {
		/*reg_crop_start_x;*/
		uint32_t reg_crop_start_x:16;
	};
	uint32_t val;
} ive_ncc_reg_ncc_07_c;
typedef union {
	struct {
		/*reg_crop_start_y;*/
		uint32_t reg_crop_start_y:16;
	};
	uint32_t val;
} ive_ncc_reg_ncc_08_c;
typedef union {
	struct {
		/*reg_crop_end_x;*/
		uint32_t reg_crop_end_x:16;
	};
	uint32_t val;
} ive_ncc_reg_ncc_09_c;
typedef union {
	struct {
		/*reg_crop_end_y;*/
		uint32_t reg_crop_end_y:16;
	};
	uint32_t val;
} ive_ncc_reg_ncc_10_c;
typedef union {
	struct {
		/*[0:1];*/
		uint32_t reg_shdw_sel:1;
	};
	uint32_t val;
} ive_ncc_reg_ncc_11_c;
typedef struct {
	volatile ive_ncc_reg_ncc_00_c reg_ncc_00;
	volatile ive_ncc_reg_ncc_01_c reg_ncc_01;
	volatile ive_ncc_reg_ncc_02_c reg_ncc_02;
	volatile ive_ncc_reg_ncc_03_c reg_ncc_03;
	volatile ive_ncc_reg_ncc_04_c reg_ncc_04;
	volatile ive_ncc_reg_ncc_05_c reg_ncc_05;
	volatile ive_ncc_reg_ncc_06_c reg_ncc_06;
	volatile ive_ncc_reg_ncc_07_c reg_ncc_07;
	volatile ive_ncc_reg_ncc_08_c reg_ncc_08;
	volatile ive_ncc_reg_ncc_09_c reg_ncc_09;
	volatile ive_ncc_reg_ncc_10_c reg_ncc_10;
	volatile ive_ncc_reg_ncc_11_c reg_ncc_11;
} ive_ncc_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_ncc_dump_ini(FILE* fp, ive_ncc_c* p) {
	fprintf(fp, "reg_numerator_l = 0x%x\n",p->reg_ncc_00.reg_numerator_l);
	fprintf(fp, "reg_numerator_h = 0x%x\n",p->reg_ncc_01.reg_numerator_h);
	fprintf(fp, "reg_quadsum0_l = 0x%x\n",p->reg_ncc_02.reg_quadsum0_l);
	fprintf(fp, "reg_quadsum0_h = 0x%x\n",p->reg_ncc_03.reg_quadsum0_h);
	fprintf(fp, "reg_quadsum1_l = 0x%x\n",p->reg_ncc_04.reg_quadsum1_l);
	fprintf(fp, "reg_quadsum1_h = 0x%x\n",p->reg_ncc_05.reg_quadsum1_h);
	fprintf(fp, "reg_crop_enable = 0x%x\n",p->reg_ncc_06.reg_crop_enable);
	fprintf(fp, "reg_crop_start_x = 0x%x\n",p->reg_ncc_07.reg_crop_start_x);
	fprintf(fp, "reg_crop_start_y = 0x%x\n",p->reg_ncc_08.reg_crop_start_y);
	fprintf(fp, "reg_crop_end_x = 0x%x\n",p->reg_ncc_09.reg_crop_end_x);
	fprintf(fp, "reg_crop_end_y = 0x%x\n",p->reg_ncc_10.reg_crop_end_y);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->reg_ncc_11.reg_shdw_sel);

}
static void ive_ncc_print(ive_ncc_c* p) {
    fprintf(stderr, "ive_ncc\n");
	fprintf(stderr, "\treg_ncc_00.reg_numerator_l = 0x%x\n", p->reg_ncc_00.reg_numerator_l);
	fprintf(stderr, "\treg_ncc_01.reg_numerator_h = 0x%x\n", p->reg_ncc_01.reg_numerator_h);
	fprintf(stderr, "\treg_ncc_02.reg_quadsum0_l = 0x%x\n", p->reg_ncc_02.reg_quadsum0_l);
	fprintf(stderr, "\treg_ncc_03.reg_quadsum0_h = 0x%x\n", p->reg_ncc_03.reg_quadsum0_h);
	fprintf(stderr, "\treg_ncc_04.reg_quadsum1_l = 0x%x\n", p->reg_ncc_04.reg_quadsum1_l);
	fprintf(stderr, "\treg_ncc_05.reg_quadsum1_h = 0x%x\n", p->reg_ncc_05.reg_quadsum1_h);
	fprintf(stderr, "\treg_ncc_06.reg_crop_enable = 0x%x\n", p->reg_ncc_06.reg_crop_enable);
	fprintf(stderr, "\treg_ncc_07.reg_crop_start_x = 0x%x\n", p->reg_ncc_07.reg_crop_start_x);
	fprintf(stderr, "\treg_ncc_08.reg_crop_start_y = 0x%x\n", p->reg_ncc_08.reg_crop_start_y);
	fprintf(stderr, "\treg_ncc_09.reg_crop_end_x = 0x%x\n", p->reg_ncc_09.reg_crop_end_x);
	fprintf(stderr, "\treg_ncc_10.reg_crop_end_y = 0x%x\n", p->reg_ncc_10.reg_crop_end_y);
	fprintf(stderr, "\treg_ncc_11.reg_shdw_sel = 0x%x\n", p->reg_ncc_11.reg_shdw_sel);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_NCC_C(X) \
	 ive_ncc_c X = \
{\
	{	/* reg_ncc_00.reg_numerator_l = */0x0,\
	},\
	{	/*.reg_ncc_01.reg_numerator_h = */0x0,\
	},\
	{	/*.reg_ncc_02.reg_quadsum0_l = */0x0,\
	},\
	{	/*.reg_ncc_03.reg_quadsum0_h = */0x0,\
	},\
	{	/*.reg_ncc_04.reg_quadsum1_l = */0x0,\
	},\
	{	/*.reg_ncc_05.reg_quadsum1_h = */0x0,\
	},\
	{	/*.reg_ncc_06.reg_crop_enable = */0x0,\
	},\
	{	/*.reg_ncc_07.reg_crop_start_x = */0x0,\
	},\
	{	/*.reg_ncc_08.reg_crop_start_y = */0x0,\
	},\
	{	/*.reg_ncc_09.reg_crop_end_x = */0x0,\
	},\
	{	/*.reg_ncc_10.reg_crop_end_y = */0x0,\
	},\
	{	/*.reg_ncc_11.reg_shdw_sel = */0x1,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_NCC_C \
{\
	.reg_ncc_00.reg_numerator_l = 0x0,\
	.reg_ncc_01.reg_numerator_h = 0x0,\
	.reg_ncc_02.reg_quadsum0_l = 0x0,\
	.reg_ncc_03.reg_quadsum0_h = 0x0,\
	.reg_ncc_04.reg_quadsum1_l = 0x0,\
	.reg_ncc_05.reg_quadsum1_h = 0x0,\
	.reg_ncc_06.reg_crop_enable = 0x0,\
	.reg_ncc_07.reg_crop_start_x = 0x0,\
	.reg_ncc_08.reg_crop_start_y = 0x0,\
	.reg_ncc_09.reg_crop_end_x = 0x0,\
	.reg_ncc_10.reg_crop_end_y = 0x0,\
	.reg_ncc_11.reg_shdw_sel = 0x1,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_NCC_STRUCT_H__

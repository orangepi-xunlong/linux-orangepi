// $Module: ive_intg $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:10:29 PM $
//

#ifndef __REG_IVE_INTG_STRUCT_H__
#define __REG_IVE_INTG_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*ip enable;*/
		uint32_t reg_ive_intg_enable:1;
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		uint32_t rsv_3_3:1;
		/*tile number, 0 for no tile;*/
		// uint32_t reg_ive_intg_tile_nm:4;
		uint32_t rsv_4_7:4;
		/*output mode control
		0 : sum+sq_sum
		1 : sum
		2 : sq_sum;*/
		uint32_t reg_ive_intg_ctrl:2;
		/*;*/
		uint32_t reg_force_clk_enable:1;
		/*;*/
		uint32_t reg_force_dma_disable:1;
	};
	uint32_t val;
} ive_intg_reg_0_c;
typedef union {
	struct {
		/*stride;*/
		uint32_t reg_ive_intg_stride:16;
	};
	uint32_t val;
} ive_intg_reg_1_c;
typedef union {
	struct {
		/*memory address;*/
		uint32_t reg_ive_intg_mem_addr:32;
	};
	uint32_t val;
} ive_intg_reg_2_c;
typedef struct {
	volatile ive_intg_reg_0_c reg_0;
	volatile ive_intg_reg_1_c reg_1;
	volatile ive_intg_reg_2_c reg_2;
} ive_intg_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_intg_dump_ini(FILE* fp, ive_intg_c* p) {
	fprintf(fp, "reg_ive_intg_enable = 0x%x\n",p->reg_0.reg_ive_intg_enable);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->reg_0.reg_shdw_sel);
	fprintf(fp, "reg_softrst = 0x%x\n",p->reg_0.reg_softrst);
	// fprintf(fp, "reg_ive_intg_tile_nm = 0x%x\n",p->reg_0.reg_ive_intg_tile_nm);
	fprintf(fp, "reg_ive_intg_ctrl = 0x%x\n",p->reg_0.reg_ive_intg_ctrl);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->reg_0.reg_force_clk_enable);
	fprintf(fp, "reg_force_dma_disable = 0x%x\n",p->reg_0.reg_force_dma_disable);
	fprintf(fp, "reg_ive_intg_stride = 0x%x\n",p->reg_1.reg_ive_intg_stride);
	fprintf(fp, "reg_ive_intg_mem_addr = 0x%x\n",p->reg_2.reg_ive_intg_mem_addr);

}
static void ive_intg_print(ive_intg_c* p) {
    fprintf(stderr, "ive_intg\n");
	fprintf(stderr, "\tREG_0.reg_ive_intg_enable = 0x%x\n", p->reg_0.reg_ive_intg_enable);
	fprintf(stderr, "\tREG_0.reg_shdw_sel = 0x%x\n", p->reg_0.reg_shdw_sel);
	fprintf(stderr, "\tREG_0.reg_softrst = 0x%x\n", p->reg_0.reg_softrst);
	// fprintf(stderr, "\tREG_0.reg_ive_intg_tile_nm = 0x%x\n", p->reg_0.reg_ive_intg_tile_nm);
	fprintf(stderr, "\tREG_0.reg_ive_intg_ctrl = 0x%x\n", p->reg_0.reg_ive_intg_ctrl);
	fprintf(stderr, "\tREG_0.reg_force_clk_enable = 0x%x\n", p->reg_0.reg_force_clk_enable);
	fprintf(stderr, "\tREG_0.reg_force_dma_disable = 0x%x\n", p->reg_0.reg_force_dma_disable);
	fprintf(stderr, "\tREG_1.reg_ive_intg_stride = 0x%x\n", p->reg_1.reg_ive_intg_stride);
	fprintf(stderr, "\tREG_2.reg_ive_intg_mem_addr = 0x%x\n", p->reg_2.reg_ive_intg_mem_addr);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_INTG_C(X) \
	 ive_intg_c X = \
{\
	{	/* reg_0.reg_ive_intg_enable = */0x0,\
	/*.reg_0.reg_shdw_sel = */0x1,\
	/*.reg_0.reg_softrst = */0x0,\
	/*uint32_t rsv_3_3:1;=*/0,\
	/*.reg_0.reg_ive_intg_tile_nm = */0x0,\
	/*.reg_0.reg_ive_intg_ctrl = */0x0,\
	/*.reg_0.reg_force_clk_enable = */0x1,\
	/*.reg_0.reg_force_dma_disable = */0x0,\
	},\
	{	/*.reg_1.reg_ive_intg_stride = */0x0,\
	},\
	{	/*.reg_2.reg_ive_intg_mem_addr = */0x1,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_INTG_C \
{\
	.reg_0.reg_ive_intg_enable = 0x0,\
	.reg_0.reg_shdw_sel = 0x1,\
	.reg_0.reg_softrst = 0x0,\
	/*.reg_0.reg_ive_intg_tile_nm = 0x0,*/\
	.reg_0.reg_ive_intg_ctrl = 0x0,\
	.reg_0.reg_force_clk_enable = 0x1,\
	.reg_0.reg_force_dma_disable = 0x0,\
	.reg_1.reg_ive_intg_stride = 0x0,\
	.reg_2.reg_ive_intg_mem_addr = 0x1,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_INTG_STRUCT_H__

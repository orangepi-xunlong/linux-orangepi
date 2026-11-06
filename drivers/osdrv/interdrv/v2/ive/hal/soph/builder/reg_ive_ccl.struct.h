// $Module: ive_ccl $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Tue, 07 Dec 2021 11:00:20 AM $
//

#ifndef __REG_IVE_CCL_STRUCT_H__
#define __REG_IVE_CCL_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*0: ccl 4c ; 1: ccl 8c;*/
		uint32_t reg_ccl_mode:1;
		uint32_t rsv_1_3:3;
		/*[0:1];*/
		uint32_t reg_ccl_shdw_sel:1;
	};
	uint32_t val;
} ive_ccl_reg_ccl_00_c;
typedef union {
	struct {
		/*unsigned 16b; min area = 1;*/
		uint32_t reg_ccl_area_thr:16;
		/*unsigned 16b; min area = 1;*/
		uint32_t reg_ccl_area_step:16;
	};
	uint32_t val;
} ive_ccl_reg_ccl_01_c;
typedef union {
	struct {
		/*0: no force; 1:force clk;*/
		uint32_t reg_force_clk_enable:1;
	};
	uint32_t val;
} ive_ccl_reg_ccl_02_c;
typedef union {
	struct {
		/*total region number,[0:254];*/
		uint32_t reg_ccl_region_num:8;
		/*8h0: label successful; 8hff: label failed ;*/
		uint32_t reg_ccl_label_status:8;
		/*current region area threshold;*/
		uint32_t reg_ccl_cur_area_thr:16;
	};
	uint32_t val;
} ive_ccl_reg_ccl_03_c;
typedef union {
	struct {
		/*read src vcnt;*/
		uint32_t reg_ccl_lbl_rd_vcnt:12;
		uint32_t rsv_12_15:4;
		/*read src hcnt;*/
		uint32_t reg_ccl_lbl_rd_hcnt:12;
	};
	uint32_t val;
} ive_ccl_reg_ccl_dbg_0_c;
typedef union {
	struct {
		/*write label vcnt;*/
		uint32_t reg_ccl_lbl_wr_vcnt:12;
		uint32_t rsv_12_15:4;
		/*write label hcnt;*/
		uint32_t reg_ccl_lbl_wr_hcnt:12;
	};
	uint32_t val;
} ive_ccl_reg_ccl_dbg_1_c;
typedef union {
	struct {
		/*read label vcnt;*/
		uint32_t reg_ccl_relbl_rd_vcnt:12;
		uint32_t rsv_12_15:4;
		/*read label hcnt;*/
		uint32_t reg_ccl_relbl_rd_hcnt:12;
	};
	uint32_t val;
} ive_ccl_reg_ccl_dbg_2_c;
typedef union {
	struct {
		/*write relabel vcnt;*/
		uint32_t reg_ccl_relbl_wr_vcnt:12;
		uint32_t rsv_12_15:4;
		/*write relabel hcnt;*/
		uint32_t reg_ccl_relbl_wr_hcnt:12;
	};
	uint32_t val;
} ive_ccl_reg_ccl_dbg_3_c;
typedef struct {
	volatile ive_ccl_reg_ccl_00_c reg_ccl_00;
	volatile ive_ccl_reg_ccl_01_c reg_ccl_01;
	volatile ive_ccl_reg_ccl_02_c reg_ccl_02;
	volatile ive_ccl_reg_ccl_03_c reg_ccl_03;
	volatile ive_ccl_reg_ccl_dbg_0_c reg_ccl_dbg_0;
	volatile ive_ccl_reg_ccl_dbg_1_c reg_ccl_dbg_1;
	volatile ive_ccl_reg_ccl_dbg_2_c reg_ccl_dbg_2;
	volatile ive_ccl_reg_ccl_dbg_3_c reg_ccl_dbg_3;
} IVE_CCL_C;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_ccl_dump_ini(FILE* fp, IVE_CCL_C* p) {
	fprintf(fp, "reg_ccl_mode = 0x%x\n",p->reg_ccl_00.reg_ccl_mode);
	fprintf(fp, "reg_ccl_shdw_sel = 0x%x\n",p->reg_ccl_00.reg_ccl_shdw_sel);
	fprintf(fp, "reg_ccl_area_thr = 0x%x\n",p->reg_ccl_01.reg_ccl_area_thr);
	fprintf(fp, "reg_ccl_area_step = 0x%x\n",p->reg_ccl_01.reg_ccl_area_step);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->reg_ccl_02.reg_force_clk_enable);
	fprintf(fp, "reg_ccl_region_num = 0x%x\n",p->reg_ccl_03.reg_ccl_region_num);
	fprintf(fp, "reg_ccl_label_status = 0x%x\n",p->reg_ccl_03.reg_ccl_label_status);
	fprintf(fp, "reg_ccl_cur_area_thr = 0x%x\n",p->reg_ccl_03.reg_ccl_cur_area_thr);
	fprintf(fp, "reg_ccl_lbl_rd_vcnt = 0x%x\n",p->reg_ccl_dbg_0.reg_ccl_lbl_rd_vcnt);
	fprintf(fp, "reg_ccl_lbl_rd_hcnt = 0x%x\n",p->reg_ccl_dbg_0.reg_ccl_lbl_rd_hcnt);
	fprintf(fp, "reg_ccl_lbl_wr_vcnt = 0x%x\n",p->reg_ccl_dbg_1.reg_ccl_lbl_wr_vcnt);
	fprintf(fp, "reg_ccl_lbl_wr_hcnt = 0x%x\n",p->reg_ccl_dbg_1.reg_ccl_lbl_wr_hcnt);
	fprintf(fp, "reg_ccl_relbl_rd_vcnt = 0x%x\n",p->reg_ccl_dbg_2.reg_ccl_relbl_rd_vcnt);
	fprintf(fp, "reg_ccl_relbl_rd_hcnt = 0x%x\n",p->reg_ccl_dbg_2.reg_ccl_relbl_rd_hcnt);
	fprintf(fp, "reg_ccl_relbl_wr_vcnt = 0x%x\n",p->reg_ccl_dbg_3.reg_ccl_relbl_wr_vcnt);
	fprintf(fp, "reg_ccl_relbl_wr_hcnt = 0x%x\n",p->reg_ccl_dbg_3.reg_ccl_relbl_wr_hcnt);
}
static void ive_ccl_print(IVE_CCL_C* p) {
    fprintf(stderr, "ive_ccl\n");
	fprintf(stderr, "\tREG_CCL_00.reg_ccl_mode = 0x%x\n", p->reg_ccl_00.reg_ccl_mode);
	fprintf(stderr, "\tREG_CCL_00.reg_ccl_shdw_sel = 0x%x\n", p->reg_ccl_00.reg_ccl_shdw_sel);
	fprintf(stderr, "\tREG_CCL_01.reg_ccl_area_thr = 0x%x\n", p->reg_ccl_01.reg_ccl_area_thr);
	fprintf(stderr, "\tREG_CCL_01.reg_ccl_area_step = 0x%x\n", p->reg_ccl_01.reg_ccl_area_step);
	fprintf(stderr, "\tREG_CCL_02.reg_force_clk_enable = 0x%x\n", p->reg_ccl_02.reg_force_clk_enable);
	fprintf(stderr, "\tREG_CCL_03.reg_ccl_region_num = 0x%x\n", p->reg_ccl_03.reg_ccl_region_num);
	fprintf(stderr, "\tREG_CCL_03.reg_ccl_label_status = 0x%x\n", p->reg_ccl_03.reg_ccl_label_status);
	fprintf(stderr, "\tREG_CCL_03.reg_ccl_cur_area_thr = 0x%x\n", p->reg_ccl_03.reg_ccl_cur_area_thr);
	fprintf(stderr, "\tREG_CCL_DBG_0.reg_ccl_lbl_rd_vcnt = 0x%x\n", p->reg_ccl_dbg_0.reg_ccl_lbl_rd_vcnt);
	fprintf(stderr, "\tREG_CCL_DBG_0.reg_ccl_lbl_rd_hcnt = 0x%x\n", p->reg_ccl_dbg_0.reg_ccl_lbl_rd_hcnt);
	fprintf(stderr, "\tREG_CCL_DBG_1.reg_ccl_lbl_wr_vcnt = 0x%x\n", p->reg_ccl_dbg_1.reg_ccl_lbl_wr_vcnt);
	fprintf(stderr, "\tREG_CCL_DBG_1.reg_ccl_lbl_wr_hcnt = 0x%x\n", p->reg_ccl_dbg_1.reg_ccl_lbl_wr_hcnt);
	fprintf(stderr, "\tREG_CCL_DBG_2.reg_ccl_relbl_rd_vcnt = 0x%x\n", p->reg_ccl_dbg_2.reg_ccl_relbl_rd_vcnt);
	fprintf(stderr, "\tREG_CCL_DBG_2.reg_ccl_relbl_rd_hcnt = 0x%x\n", p->reg_ccl_dbg_2.reg_ccl_relbl_rd_hcnt);
	fprintf(stderr, "\tREG_CCL_DBG_3.reg_ccl_relbl_wr_vcnt = 0x%x\n", p->reg_ccl_dbg_3.reg_ccl_relbl_wr_vcnt);
	fprintf(stderr, "\tREG_CCL_DBG_3.reg_ccl_relbl_wr_hcnt = 0x%x\n", p->reg_ccl_dbg_3.reg_ccl_relbl_wr_hcnt);
}
#pragma GCC diagnostic pop
#define DEFINE_IVE_CCL_C(X) \
	 IVE_CCL_C X = \
{\
	{	/* reg_ccl_00.reg_ccl_mode = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.reg_ccl_00.reg_ccl_shdw_sel = */0x1,\
	},\
	{	/*.reg_ccl_01.reg_ccl_area_thr = */0x1,\
	/*.reg_ccl_01.reg_ccl_area_step = */0x1,\
	},\
	{	/*.reg_ccl_02.reg_force_clk_enable = */0x0,\
	},\
	{	/*.reg_ccl_03.reg_ccl_region_num = */0x0,\
	/*.reg_ccl_03.reg_ccl_label_status = */0x0,\
	/*.reg_ccl_03.reg_ccl_cur_area_thr = */0x0,\
	},\
	{	/*.reg_ccl_dbg_0.reg_ccl_lbl_rd_vcnt = */0x0,\
	/*.reg_ccl_dbg_0.reg_ccl_lbl_rd_hcnt = */0x0,\
	},\
	{	/*.reg_ccl_dbg_1.reg_ccl_lbl_wr_vcnt = */0x0,\
	/*.reg_ccl_dbg_1.reg_ccl_lbl_wr_hcnt = */0x0,\
	},\
	{	/*.reg_ccl_dbg_2.reg_ccl_relbl_rd_vcnt = */0x0,\
	/*.reg_ccl_dbg_2.reg_ccl_relbl_rd_hcnt = */0x0,\
	},\
	{	/*.reg_ccl_dbg_3.reg_ccl_relbl_wr_vcnt = */0x0,\
	/*.reg_ccl_dbg_3.reg_ccl_relbl_wr_hcnt = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_CCL_C \
{\
	.reg_ccl_00.reg_ccl_mode = 0x0,\
	.reg_ccl_00.reg_ccl_shdw_sel = 0x1,\
	.reg_ccl_01.reg_ccl_area_thr = 0x1,\
	.reg_ccl_01.reg_ccl_area_step = 0x1,\
	.reg_ccl_02.reg_force_clk_enable = 0x0,\
	.reg_ccl_03.reg_ccl_region_num = 0x0,\
	.reg_ccl_03.reg_ccl_label_status = 0x0,\
	.reg_ccl_03.reg_ccl_cur_area_thr = 0x0,\
	.reg_ccl_dbg_0.reg_ccl_lbl_rd_vcnt = 0x0,\
	.reg_ccl_dbg_0.reg_ccl_lbl_rd_hcnt = 0x0,\
	.reg_ccl_dbg_1.reg_ccl_lbl_wr_vcnt = 0x0,\
	.reg_ccl_dbg_1.reg_ccl_lbl_wr_hcnt = 0x0,\
	.reg_ccl_dbg_2.reg_ccl_relbl_rd_vcnt = 0x0,\
	.reg_ccl_dbg_2.reg_ccl_relbl_rd_hcnt = 0x0,\
	.reg_ccl_dbg_3.reg_ccl_relbl_wr_vcnt = 0x0,\
	.reg_ccl_dbg_3.reg_ccl_relbl_wr_hcnt = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_CCL_STRUCT_H__

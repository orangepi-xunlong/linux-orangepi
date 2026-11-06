// $Module: ive_dma $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:09:55 PM $
//

#ifndef __REG_IVE_DMA_STRUCT_H__
#define __REG_IVE_DMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*ip enable;*/
		uint32_t reg_ive_dma_enable:1;
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		uint32_t rsv_3_3:1;
		/*dma mode control
		0 : direct copy
		1 : interval copy
		2 : set 3bytes
		3 : set 8bytes;*/
		uint32_t reg_ive_dma_mode:2;
		uint32_t rsv_7_6:2;
		uint32_t reg_force_clk_enable:1;
		uint32_t reg_force_rdma_disable:1;
		uint32_t reg_force_wdma_disable:1;
	};
	uint32_t val;
} ive_dma_reg_0_c;
typedef union {
	struct {
		/*source stride;*/
		uint32_t reg_ive_dma_src_stride:16;
		/*Destination stride;*/
		uint32_t reg_ive_dma_dst_stride:16;
	};
	uint32_t val;
} ive_dma_reg_1_c;
typedef union {
	struct {
		/*source memory address;*/
		uint32_t reg_ive_dma_src_mem_addr:32;
	};
	uint32_t val;
} ive_dma_reg_2_c;
typedef union {
	struct {
		/*Destination memory address;*/
		uint32_t reg_ive_dma_dst_mem_addr:32;
	};
	uint32_t val;
} ive_dma_reg_3_c;
typedef union {
	struct {
		/*horizotal segment size;*/
		uint32_t reg_ive_dma_horsegsize:8;
		/*element size;*/
		uint32_t reg_ive_dma_elemsize:8;
		/*vertical segment row;*/
		uint32_t reg_ive_dma_versegrow:8;
	};
	uint32_t val;
} ive_dma_reg_4_c;
typedef union {
	struct {
		/*U64 value;*/
		uint32_t reg_ive_dma_u64_val[2];
	};
	uint32_t val[2];
} ive_dma_reg_5_c;
typedef struct {
	volatile ive_dma_reg_0_c reg_0;
	volatile ive_dma_reg_1_c reg_1;
	volatile ive_dma_reg_2_c reg_2;
	volatile ive_dma_reg_3_c reg_3;
	volatile ive_dma_reg_4_c reg_4;
	volatile ive_dma_reg_5_c reg_5;
} ive_dma_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_dma_dump_ini(FILE* fp, ive_dma_c* p) {
	fprintf(fp, "reg_ive_dma_enable = 0x%x\n",p->reg_0.reg_ive_dma_enable);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->reg_0.reg_shdw_sel);
	fprintf(fp, "reg_softrst = 0x%x\n",p->reg_0.reg_softrst);
	fprintf(fp, "reg_ive_dma_mode = 0x%x\n",p->reg_0.reg_ive_dma_mode);
	fprintf(fp, "reg_force_clk_enable = 0x%x\n",p->reg_0.reg_force_clk_enable);
	fprintf(fp, "reg_force_rdma_disable = 0x%x\n",p->reg_0.reg_force_rdma_disable);
	fprintf(fp, "reg_force_wdma_disable = 0x%x\n",p->reg_0.reg_force_wdma_disable);
	fprintf(fp, "reg_ive_dma_src_stride = 0x%x\n",p->reg_1.reg_ive_dma_src_stride);
	fprintf(fp, "reg_ive_dma_dst_stride = 0x%x\n",p->reg_1.reg_ive_dma_dst_stride);
	fprintf(fp, "reg_ive_dma_src_mem_addr = 0x%x\n",p->reg_2.reg_ive_dma_src_mem_addr);
	fprintf(fp, "reg_ive_dma_dst_mem_addr = 0x%x\n",p->reg_3.reg_ive_dma_dst_mem_addr);
	fprintf(fp, "reg_ive_dma_horsegsize = 0x%x\n",p->reg_4.reg_ive_dma_horsegsize);
	fprintf(fp, "reg_ive_dma_elemsize = 0x%x\n",p->reg_4.reg_ive_dma_elemsize);
	fprintf(fp, "reg_ive_dma_versegrow = 0x%x\n",p->reg_4.reg_ive_dma_versegrow);
	fprintf(fp, "reg_ive_dma_u64_val[0] = 0x%x\n",p->reg_5.reg_ive_dma_u64_val[0]);
	fprintf(fp, "reg_ive_dma_u64_val[1] = 0x%x\n",p->reg_5.reg_ive_dma_u64_val[1]);

}
static void ive_dma_print(ive_dma_c* p) {
    fprintf(stderr, "ive_dma\n");
	fprintf(stderr, "\tREG_0.reg_ive_dma_enable = 0x%x\n", p->reg_0.reg_ive_dma_enable);
	fprintf(stderr, "\tREG_0.reg_shdw_sel = 0x%x\n", p->reg_0.reg_shdw_sel);
	fprintf(stderr, "\tREG_0.reg_softrst = 0x%x\n", p->reg_0.reg_softrst);
	fprintf(stderr, "\tREG_0.reg_ive_dma_mode = 0x%x\n", p->reg_0.reg_ive_dma_mode);
	fprintf(stderr, "\tREG_0.reg_force_clk_enable = 0x%x\n",p->reg_0.reg_force_clk_enable);
	fprintf(stderr, "\tREG_0.reg_force_rdma_disable = 0x%x\n",p->reg_0.reg_force_rdma_disable);
	fprintf(stderr, "\tREG_0.reg_force_wdma_disable = 0x%x\n",p->reg_0.reg_force_wdma_disable);
	fprintf(stderr, "\tREG_1.reg_ive_dma_src_stride = 0x%x\n", p->reg_1.reg_ive_dma_src_stride);
	fprintf(stderr, "\tREG_1.reg_ive_dma_dst_stride = 0x%x\n", p->reg_1.reg_ive_dma_dst_stride);
	fprintf(stderr, "\tREG_2.reg_ive_dma_src_mem_addr = 0x%x\n", p->reg_2.reg_ive_dma_src_mem_addr);
	fprintf(stderr, "\tREG_3.reg_ive_dma_dst_mem_addr = 0x%x\n", p->reg_3.reg_ive_dma_dst_mem_addr);
	fprintf(stderr, "\tREG_4.reg_ive_dma_horsegsize = 0x%x\n", p->reg_4.reg_ive_dma_horsegsize);
	fprintf(stderr, "\tREG_4.reg_ive_dma_elemsize = 0x%x\n", p->reg_4.reg_ive_dma_elemsize);
	fprintf(stderr, "\tREG_4.reg_ive_dma_versegrow = 0x%x\n", p->reg_4.reg_ive_dma_versegrow);
	fprintf(stderr, "\tREG_5.reg_ive_dma_u64_val[0] = 0x%x\n", p->reg_5.reg_ive_dma_u64_val[0]);
	fprintf(stderr, "\tREG_5.reg_ive_dma_u64_val[1] = 0x%x\n", p->reg_5.reg_ive_dma_u64_val[1]);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_DMA_C(X) \
	 ive_dma_c X = \
{\
	{	/* reg_0.reg_ive_dma_enable = */0x0,\
	/*.reg_0.reg_shdw_sel = */0x1,\
	/*.reg_0.reg_softrst = */0x0,\
	/*uint32_t rsv_3_3:1;=*/0,\
	/*.reg_0.reg_ive_dma_mode = */0x0,\
	/*.reg_0.rsv_7_6:2;=*/0,\
	/*.reg_0.reg_force_clk_enable:1;=*/0x1,\
	/*.reg_0.reg_force_rdma_disable:1;=*/0x0,\
	/*.reg_0.reg_force_wdma_disable:1;=*/0x0,\
	},\
	{	/*.reg_1.reg_ive_dma_src_stride = */0x0,\
	/*.reg_1.reg_ive_dma_dst_stride = */0x0,\
	},\
	{	/*.reg_2.reg_ive_dma_src_mem_addr = */0x1,\
	},\
	{	/*.reg_3.reg_ive_dma_dst_mem_addr = */0x1,\
	},\
	{	/*.reg_4.reg_ive_dma_horsegsize = */0x0,\
	/*.reg_4.reg_ive_dma_elemsize = */0x0,\
	/*.reg_4.reg_ive_dma_versegrow = */0x0,\
	},\
	{	/*.reg_5.reg_ive_dma_u64_val = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_DMA_C \
{\
	.reg_0.reg_ive_dma_enable = 0x0,\
	.reg_0.reg_shdw_sel = 0x1,\
	.reg_0.reg_softrst = 0x0,\
	.reg_0.reg_ive_dma_mode = 0x0,\
	.reg_0.reg_force_clk_enable = 0x1,\
	.reg_0.reg_force_rdma_disable = 0x0,\
	.reg_0.reg_force_wdma_disable = 0x0,\
	.reg_1.reg_ive_dma_src_stride = 0x0,\
	.reg_1.reg_ive_dma_dst_stride = 0x0,\
	.reg_2.reg_ive_dma_src_mem_addr = 0x1,\
	.reg_3.reg_ive_dma_dst_mem_addr = 0x1,\
	.reg_4.reg_ive_dma_horsegsize = 0x0,\
	.reg_4.reg_ive_dma_elemsize = 0x0,\
	.reg_4.reg_ive_dma_versegrow = 0x0,\
	.reg_5.reg_ive_dma_u64_val = {0, 0},\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_DMA_STRUCT_H__

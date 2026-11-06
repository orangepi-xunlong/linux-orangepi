// $Module: ive_map $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:10:36 PM $
//

#ifndef __REG_IVE_MAP_STRUCT_H__
#define __REG_IVE_MAP_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*sw reset;*/
		uint32_t reg_softrst:1;
		/*ip enable;*/
		uint32_t reg_ip_enable:1;
		/*set 1: clk enable , 0: clk gating;*/
		uint32_t reg_ck_enable:1;
		uint32_t rsv_3_3:1;
		/*1: read choose raw data ;*/
		uint32_t reg_shdw_sel:1;
		uint32_t rsv_5_7:3;
		/*set 1: disable apb ready handshake;*/
		uint32_t reg_prog_hdk_dis:1;
	};
	uint32_t val;
} ive_map_reg_0_c;
typedef union {
	struct {
		/*set 1 : prog mem;*/
		uint32_t reg_lut_prog_en:1;
		/*set 0 : choose mem0;*/
		uint32_t reg_lut_wsel:1;
		/*set 0 : choose ip read from mem0;*/
		uint32_t reg_lut_rsel:1;
		/*set 0 :  sw debug read from mem0;*/
		uint32_t reg_sw_lut_rsel:1;
	};
	uint32_t val;
} ive_map_reg_1_c;
typedef union {
	struct {
		/*set prog start addr;*/
		uint32_t reg_lut_st_addr:8;
		uint32_t rsv_8_30:23;
		/*trig 1t for start address update;*/
		uint32_t reg_lut_st_w1t:1;
	};
	uint32_t val;
} ive_map_reg_2_c;
typedef union {
	struct {
		/*set prog wdata;*/
		uint32_t reg_lut_wdata:16;
		uint32_t rsv_16_30:15;
		/*trig 1t for wdata update;*/
		uint32_t reg_lut_w1t:1;
	};
	uint32_t val;
} ive_map_reg_3_c;
typedef union {
	struct {
		/*[debug] sw debug read address;*/
		uint32_t reg_sw_lut_raddr:8;
		uint32_t rsv_8_30:23;
		/*[debug] trig 1t for read address update;*/
		uint32_t reg_sw_lut_r_w1t:1;
	};
	uint32_t val;
} ive_map_reg_4_c;
typedef union {
	struct {
		/*[debug] lut read data;*/
		uint32_t reg_sw_lut_rdata:16;
	};
	uint32_t val;
} ive_map_reg_5_c;
typedef struct {
	volatile ive_map_reg_0_c reg_0;
	volatile ive_map_reg_1_c reg_1;
	volatile ive_map_reg_2_c reg_2;
	volatile ive_map_reg_3_c reg_3;
	volatile ive_map_reg_4_c reg_4;
	volatile ive_map_reg_5_c reg_5;
} ive_map_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_map_dump_ini(FILE* fp, ive_map_c* p) {
	fprintf(fp, "reg_softrst = 0x%x\n",p->reg_0.reg_softrst);
	fprintf(fp, "reg_ip_enable = 0x%x\n",p->reg_0.reg_ip_enable);
	fprintf(fp, "reg_ck_enable = 0x%x\n",p->reg_0.reg_ck_enable);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->reg_0.reg_shdw_sel);
	fprintf(fp, "reg_prog_hdk_dis = 0x%x\n",p->reg_0.reg_prog_hdk_dis);
	fprintf(fp, "reg_lut_prog_en = 0x%x\n",p->reg_1.reg_lut_prog_en);
	fprintf(fp, "reg_lut_wsel = 0x%x\n",p->reg_1.reg_lut_wsel);
	fprintf(fp, "reg_lut_rsel = 0x%x\n",p->reg_1.reg_lut_rsel);
	fprintf(fp, "reg_sw_lut_rsel = 0x%x\n",p->reg_1.reg_sw_lut_rsel);
	fprintf(fp, "reg_lut_st_addr = 0x%x\n",p->reg_2.reg_lut_st_addr);
	fprintf(fp, "reg_lut_st_w1t = 0x%x\n",p->reg_2.reg_lut_st_w1t);
	fprintf(fp, "reg_lut_wdata = 0x%x\n",p->reg_3.reg_lut_wdata);
	fprintf(fp, "reg_lut_w1t = 0x%x\n",p->reg_3.reg_lut_w1t);
	fprintf(fp, "reg_sw_lut_raddr = 0x%x\n",p->reg_4.reg_sw_lut_raddr);
	fprintf(fp, "reg_sw_lut_r_w1t = 0x%x\n",p->reg_4.reg_sw_lut_r_w1t);
	fprintf(fp, "reg_sw_lut_rdata = 0x%x\n",p->reg_5.reg_sw_lut_rdata);

}
static void ive_map_print(ive_map_c* p) {
    fprintf(stderr, "ive_map\n");
	fprintf(stderr, "\tREG_0.reg_softrst = 0x%x\n", p->reg_0.reg_softrst);
	fprintf(stderr, "\tREG_0.reg_ip_enable = 0x%x\n", p->reg_0.reg_ip_enable);
	fprintf(stderr, "\tREG_0.reg_ck_enable = 0x%x\n", p->reg_0.reg_ck_enable);
	fprintf(stderr, "\tREG_0.reg_shdw_sel = 0x%x\n", p->reg_0.reg_shdw_sel);
	fprintf(stderr, "\tREG_0.reg_prog_hdk_dis = 0x%x\n", p->reg_0.reg_prog_hdk_dis);
	fprintf(stderr, "\tREG_1.reg_lut_prog_en = 0x%x\n", p->reg_1.reg_lut_prog_en);
	fprintf(stderr, "\tREG_1.reg_lut_wsel = 0x%x\n", p->reg_1.reg_lut_wsel);
	fprintf(stderr, "\tREG_1.reg_lut_rsel = 0x%x\n", p->reg_1.reg_lut_rsel);
	fprintf(stderr, "\tREG_1.reg_sw_lut_rsel = 0x%x\n", p->reg_1.reg_sw_lut_rsel);
	fprintf(stderr, "\tREG_2.reg_lut_st_addr = 0x%x\n", p->reg_2.reg_lut_st_addr);
	fprintf(stderr, "\tREG_2.reg_lut_st_w1t = 0x%x\n", p->reg_2.reg_lut_st_w1t);
	fprintf(stderr, "\tREG_3.reg_lut_wdata = 0x%x\n", p->reg_3.reg_lut_wdata);
	fprintf(stderr, "\tREG_3.reg_lut_w1t = 0x%x\n", p->reg_3.reg_lut_w1t);
	fprintf(stderr, "\tREG_4.reg_sw_lut_raddr = 0x%x\n", p->reg_4.reg_sw_lut_raddr);
	fprintf(stderr, "\tREG_4.reg_sw_lut_r_w1t = 0x%x\n", p->reg_4.reg_sw_lut_r_w1t);
	fprintf(stderr, "\tREG_5.reg_sw_lut_rdata = 0x%x\n", p->reg_5.reg_sw_lut_rdata);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_MAP_C(X) \
	 ive_map_c X = \
{\
	{	/* reg_0.reg_softrst = */0x0,\
	/*.reg_0.reg_ip_enable = */0x0,\
	/*.reg_0.reg_ck_enable = */0x1,\
	/*uint32_t rsv_3_3:1;=*/0,\
	/*.reg_0.reg_shdw_sel = */0x1,\
	/*uint32_t rsv_5_7:3;=*/0,\
	/*.reg_0.reg_prog_hdk_dis = */0x0,\
	},\
	{	/*.reg_1.reg_lut_prog_en = */0x0,\
	/*.reg_1.reg_lut_wsel = */0x0,\
	/*.reg_1.reg_lut_rsel = */0x0,\
	/*.reg_1.reg_sw_lut_rsel = */0x0,\
	},\
	{	/*.reg_2.reg_lut_st_addr = */0x0,\
	/*uint32_t rsv_8_30:23;=*/0,\
	/*.reg_2.reg_lut_st_w1t = */0x0,\
	},\
	{	/*.reg_3.reg_lut_wdata = */0x0,\
	/*uint32_t rsv_16_30:15;=*/0,\
	/*.reg_3.reg_lut_w1t = */0x0,\
	},\
	{	/*.reg_4.reg_sw_lut_raddr = */0x0,\
	/*uint32_t rsv_8_30:23;=*/0,\
	/*.reg_4.reg_sw_lut_r_w1t = */0x0,\
	},\
	{	/*.reg_5.reg_sw_lut_rdata = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_MAP_C \
{\
	.reg_0.reg_softrst = 0x0,\
	.reg_0.reg_ip_enable = 0x0,\
	.reg_0.reg_ck_enable = 0x1,\
	.reg_0.reg_shdw_sel = 0x1,\
	.reg_0.reg_prog_hdk_dis = 0x0,\
	.reg_1.reg_lut_prog_en = 0x0,\
	.reg_1.reg_lut_wsel = 0x0,\
	.reg_1.reg_lut_rsel = 0x0,\
	.reg_1.reg_sw_lut_rsel = 0x0,\
	.reg_2.reg_lut_st_addr = 0x0,\
	.reg_2.reg_lut_st_w1t = 0x0,\
	.reg_3.reg_lut_wdata = 0x0,\
	.reg_3.reg_lut_w1t = 0x0,\
	.reg_4.reg_sw_lut_raddr = 0x0,\
	.reg_4.reg_sw_lut_r_w1t = 0x0,\
	.reg_5.reg_sw_lut_rdata = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_MAP_STRUCT_H__

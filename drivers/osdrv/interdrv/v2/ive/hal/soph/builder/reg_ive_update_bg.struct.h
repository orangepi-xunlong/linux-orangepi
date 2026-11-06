// $Module: ive_update_bg $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Tue, 07 Dec 2021 11:01:06 AM $
//

#ifndef __REG_IVE_UPDATE_BG_STRUCT_H__
#define __REG_IVE_UPDATE_BG_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
	};
	uint32_t val;
} ive_update_bg_reg_1_c;
typedef union {
	struct {
		/*set 1 as gradfg active to enable rdma  ;*/
		uint32_t reg_enable:1;
		/*set 1: clk force enable  0: autocg gating ;*/
		uint32_t reg_ck_en:1;
		/*set 1: bypass fg / bg model0 /bg model1;*/
		uint32_t reg_updatebg_byp_model:1;
	};
	uint32_t val;
} ive_update_bg_reg_h04_c;
typedef union {
	struct {
		/*reg reg sel;*/
		uint32_t reg_shdw_sel:1;
	};
	uint32_t val;
} ive_update_bg_reg_2_c;
typedef union {
	struct {
		/*dmy;*/
		uint32_t reg_ctrl_dmy1:32;
	};
	uint32_t val;
} ive_update_bg_reg_3_c;
typedef union {
	struct {
		/*cuttent frame number;*/
		uint32_t reg_u32CurFrmNum:32;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl0_c;
typedef union {
	struct {
		/*previouse (bg update) frame number;*/
		uint32_t reg_u32PreChkTime:32;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl1_c;
typedef union {
	struct {
		/*time interval of Bg Update (unit: frame) [0,2000];*/
		uint32_t reg_u32FrmChkPeriod:12;
		uint32_t rsv_12_15:4;
		/*the shortest time of Bg Update [20,6000];*/
		uint32_t reg_u32InitMinTime:13;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl2_c;
typedef union {
	struct {
		/*the shortest time of static bg ：[init_min_time, 6000];*/
		uint32_t reg_u32StyBgMinBlendTime:16;
		/*the longest time of static bg ：[sty_bg_min_blend_time, 40000];*/
		uint32_t reg_u32StyBgMaxBlendTime:16;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl3_c;
typedef union {
	struct {
		/*the shortest time of dynamic bg  [0,6000];*/
		uint32_t reg_u32DynBgMinBlendTime:13;
		uint32_t rsv_13_15:3;
		/*the longest time of Bg detect [20,6000];*/
		uint32_t reg_u32StaticDetMinTime:13;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl4_c;
typedef union {
	struct {
		/*the longest time of Fg is Fading [1,255];*/
		uint32_t reg_u16FgMaxFadeTime:8;
		/*the longest time of Bg is Fading [1,255];*/
		uint32_t reg_u16BgMaxFadeTime:8;
		/*static bg access rate [10,100];*/
		uint32_t reg_u8StyBgAccTimeRateThr:7;
		/*change bg access rate [10,100];*/
		uint32_t reg_u8ChgBgAccTimeRateThr:7;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl5_c;
typedef union {
	struct {
		/*dynamic bg access rate [0,50];*/
		uint32_t reg_u8DynBgAccTimeThr:6;
		uint32_t rsv_6_7:2;
		/*bg intial status rate [90,100];*/
		uint32_t reg_u8BgEffStaRateThr:7;
		uint32_t rsv_15_15:1;
		/*dynamic bg depth [0,3];*/
		uint32_t reg_u8DynBgDepth:2;
		uint32_t rsv_18_23:6;
		/*speed up bg learning [0,1];*/
		uint32_t reg_u8AcceBgLearn:1;
		/*1: detect change region ;*/
		uint32_t reg_u8DetChgRegion:1;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl6_c;
typedef union {
	struct {
		/*bg number;*/
		uint32_t reg_stat_pixnum:32;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl7_c;
typedef union {
	struct {
		/*sum of bg img;*/
		uint32_t reg_stat_sumlum:32;
	};
	uint32_t val;
} ive_update_bg_reg_ctrl8_c;
typedef union {
	struct {
		/*crop to wdma for tile mode usage : crop start x ;*/
		uint32_t reg_crop_start_x:16;
		/*crop to wdma for tile mode usage : crop end x ;*/
		uint32_t reg_crop_end_x:16;
	};
	uint32_t val;
} ive_update_bg_reg_crop_s_c;
typedef union {
	struct {
		/*crop to wdma for tile mode usage : crop start y ;*/
		uint32_t reg_crop_start_y:16;
		/*crop to wdma for tile mode usage : crop end y ;*/
		uint32_t reg_crop_end_y:16;
	};
	uint32_t val;
} ive_update_bg_reg_crop_e_c;
typedef union {
	struct {
		/*crop to wdma for tile mode usage : crop enable;*/
		uint32_t reg_crop_enable:1;
	};
	uint32_t val;
} ive_update_bg_reg_crop_ctl_c;
typedef struct {
	volatile ive_update_bg_reg_1_c reg_1;
	volatile ive_update_bg_reg_h04_c reg_h04;
	volatile ive_update_bg_reg_2_c reg_2;
	volatile ive_update_bg_reg_3_c reg_3;
	volatile ive_update_bg_reg_ctrl0_c reg_ctrl0;
	volatile ive_update_bg_reg_ctrl1_c reg_ctrl1;
	volatile ive_update_bg_reg_ctrl2_c reg_ctrl2;
	volatile ive_update_bg_reg_ctrl3_c reg_ctrl3;
	volatile ive_update_bg_reg_ctrl4_c reg_ctrl4;
	volatile ive_update_bg_reg_ctrl5_c reg_ctrl5;
	volatile ive_update_bg_reg_ctrl6_c reg_ctrl6;
	volatile ive_update_bg_reg_ctrl7_c reg_ctrl7;
	volatile ive_update_bg_reg_ctrl8_c reg_ctrl8;
	volatile ive_update_bg_reg_crop_s_c reg_crop_s;
	volatile ive_update_bg_reg_crop_e_c reg_crop_e;
	volatile ive_update_bg_reg_crop_ctl_c reg_crop_ctl;
} ive_update_bg_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void ive_update_bg_dump_ini(FILE* fp, ive_update_bg_c* p) {
	fprintf(fp, "reg_softrst = 0x%x\n",p->reg_1.reg_softrst);
	fprintf(fp, "reg_enable = 0x%x\n",p->reg_h04.reg_enable);
	fprintf(fp, "reg_ck_en = 0x%x\n",p->reg_h04.reg_ck_en);
	fprintf(fp, "reg_updatebg_byp_model = 0x%x\n",p->reg_h04.reg_updatebg_byp_model);
	fprintf(fp, "reg_shdw_sel = 0x%x\n",p->reg_2.reg_shdw_sel);
	fprintf(fp, "reg_ctrl_dmy1 = 0x%x\n",p->reg_3.reg_ctrl_dmy1);
	fprintf(fp, "reg_u32CurFrmNum = 0x%x\n",p->reg_ctrl0.reg_u32CurFrmNum);
	fprintf(fp, "reg_u32PreChkTime = 0x%x\n",p->reg_ctrl1.reg_u32PreChkTime);
	fprintf(fp, "reg_u32FrmChkPeriod = 0x%x\n",p->reg_ctrl2.reg_u32FrmChkPeriod);
	fprintf(fp, "reg_u32InitMinTime = 0x%x\n",p->reg_ctrl2.reg_u32InitMinTime);
	fprintf(fp, "reg_u32StyBgMinBlendTime = 0x%x\n",p->reg_ctrl3.reg_u32StyBgMinBlendTime);
	fprintf(fp, "reg_u32StyBgMaxBlendTime = 0x%x\n",p->reg_ctrl3.reg_u32StyBgMaxBlendTime);
	fprintf(fp, "reg_u32DynBgMinBlendTime = 0x%x\n",p->reg_ctrl4.reg_u32DynBgMinBlendTime);
	fprintf(fp, "reg_u32StaticDetMinTime = 0x%x\n",p->reg_ctrl4.reg_u32StaticDetMinTime);
	fprintf(fp, "reg_u16FgMaxFadeTime = 0x%x\n",p->reg_ctrl5.reg_u16FgMaxFadeTime);
	fprintf(fp, "reg_u16BgMaxFadeTime = 0x%x\n",p->reg_ctrl5.reg_u16BgMaxFadeTime);
	fprintf(fp, "reg_u8StyBgAccTimeRateThr = 0x%x\n",p->reg_ctrl5.reg_u8StyBgAccTimeRateThr);
	fprintf(fp, "reg_u8ChgBgAccTimeRateThr = 0x%x\n",p->reg_ctrl5.reg_u8ChgBgAccTimeRateThr);
	fprintf(fp, "reg_u8DynBgAccTimeThr = 0x%x\n",p->reg_ctrl6.reg_u8DynBgAccTimeThr);
	fprintf(fp, "reg_u8BgEffStaRateThr = 0x%x\n",p->reg_ctrl6.reg_u8BgEffStaRateThr);
	fprintf(fp, "reg_u8DynBgDepth = 0x%x\n",p->reg_ctrl6.reg_u8DynBgDepth);
	fprintf(fp, "reg_u8AcceBgLearn = 0x%x\n",p->reg_ctrl6.reg_u8AcceBgLearn);
	fprintf(fp, "reg_u8DetChgRegion = 0x%x\n",p->reg_ctrl6.reg_u8DetChgRegion);
	fprintf(fp, "reg_stat_pixnum = 0x%x\n",p->reg_ctrl7.reg_stat_pixnum);
	fprintf(fp, "reg_stat_sumlum = 0x%x\n",p->reg_ctrl8.reg_stat_sumlum);
	fprintf(fp, "reg_crop_start_x = 0x%x\n",p->reg_crop_s.reg_crop_start_x);
	fprintf(fp, "reg_crop_end_x = 0x%x\n",p->reg_crop_s.reg_crop_end_x);
	fprintf(fp, "reg_crop_start_y = 0x%x\n",p->reg_crop_e.reg_crop_start_y);
	fprintf(fp, "reg_crop_end_y = 0x%x\n",p->reg_crop_e.reg_crop_end_y);
	fprintf(fp, "reg_crop_enable = 0x%x\n",p->reg_crop_ctl.reg_crop_enable);

}
static void ive_update_bg_print(ive_update_bg_c* p) {
    fprintf(stderr, "ive_update_bg\n");
	fprintf(stderr, "\tREG_1.reg_softrst = 0x%x\n", p->reg_1.reg_softrst);
	fprintf(stderr, "\tREG_H04.reg_enable = 0x%x\n", p->reg_h04.reg_enable);
	fprintf(stderr, "\tREG_H04.reg_ck_en = 0x%x\n", p->reg_h04.reg_ck_en);
	fprintf(stderr, "\tREG_H04.reg_updatebg_byp_model = 0x%x\n", p->reg_h04.reg_updatebg_byp_model);
	fprintf(stderr, "\tREG_2.reg_shdw_sel = 0x%x\n", p->reg_2.reg_shdw_sel);
	fprintf(stderr, "\tREG_3.reg_ctrl_dmy1 = 0x%x\n", p->reg_3.reg_ctrl_dmy1);
	fprintf(stderr, "\treg_ctrl0.reg_u32CurFrmNum = 0x%x\n", p->reg_ctrl0.reg_u32CurFrmNum);
	fprintf(stderr, "\treg_ctrl1.reg_u32PreChkTime = 0x%x\n", p->reg_ctrl1.reg_u32PreChkTime);
	fprintf(stderr, "\treg_ctrl2.reg_u32FrmChkPeriod = 0x%x\n", p->reg_ctrl2.reg_u32FrmChkPeriod);
	fprintf(stderr, "\treg_ctrl2.reg_u32InitMinTime = 0x%x\n", p->reg_ctrl2.reg_u32InitMinTime);
	fprintf(stderr, "\treg_ctrl3.reg_u32StyBgMinBlendTime = 0x%x\n", p->reg_ctrl3.reg_u32StyBgMinBlendTime);
	fprintf(stderr, "\treg_ctrl3.reg_u32StyBgMaxBlendTime = 0x%x\n", p->reg_ctrl3.reg_u32StyBgMaxBlendTime);
	fprintf(stderr, "\treg_ctrl4.reg_u32DynBgMinBlendTime = 0x%x\n", p->reg_ctrl4.reg_u32DynBgMinBlendTime);
	fprintf(stderr, "\treg_ctrl4.reg_u32StaticDetMinTime = 0x%x\n", p->reg_ctrl4.reg_u32StaticDetMinTime);
	fprintf(stderr, "\treg_ctrl5.reg_u16FgMaxFadeTime = 0x%x\n", p->reg_ctrl5.reg_u16FgMaxFadeTime);
	fprintf(stderr, "\treg_ctrl5.reg_u16BgMaxFadeTime = 0x%x\n", p->reg_ctrl5.reg_u16BgMaxFadeTime);
	fprintf(stderr, "\treg_ctrl5.reg_u8StyBgAccTimeRateThr = 0x%x\n", p->reg_ctrl5.reg_u8StyBgAccTimeRateThr);
	fprintf(stderr, "\treg_ctrl5.reg_u8ChgBgAccTimeRateThr = 0x%x\n", p->reg_ctrl5.reg_u8ChgBgAccTimeRateThr);
	fprintf(stderr, "\treg_ctrl6.reg_u8DynBgAccTimeThr = 0x%x\n", p->reg_ctrl6.reg_u8DynBgAccTimeThr);
	fprintf(stderr, "\treg_ctrl6.reg_u8BgEffStaRateThr = 0x%x\n", p->reg_ctrl6.reg_u8BgEffStaRateThr);
	fprintf(stderr, "\treg_ctrl6.reg_u8DynBgDepth = 0x%x\n", p->reg_ctrl6.reg_u8DynBgDepth);
	fprintf(stderr, "\treg_ctrl6.reg_u8AcceBgLearn = 0x%x\n", p->reg_ctrl6.reg_u8AcceBgLearn);
	fprintf(stderr, "\treg_ctrl6.reg_u8DetChgRegion = 0x%x\n", p->reg_ctrl6.reg_u8DetChgRegion);
	fprintf(stderr, "\treg_ctrl7.reg_stat_pixnum = 0x%x\n", p->reg_ctrl7.reg_stat_pixnum);
	fprintf(stderr, "\treg_ctrl8.reg_stat_sumlum = 0x%x\n", p->reg_ctrl8.reg_stat_sumlum);
	fprintf(stderr, "\treg_crop_s.reg_crop_start_x = 0x%x\n", p->reg_crop_s.reg_crop_start_x);
	fprintf(stderr, "\treg_crop_s.reg_crop_end_x = 0x%x\n", p->reg_crop_s.reg_crop_end_x);
	fprintf(stderr, "\treg_crop_e.reg_crop_start_y = 0x%x\n", p->reg_crop_e.reg_crop_start_y);
	fprintf(stderr, "\treg_crop_e.reg_crop_end_y = 0x%x\n", p->reg_crop_e.reg_crop_end_y);
	fprintf(stderr, "\treg_crop_ctl.reg_crop_enable = 0x%x\n", p->reg_crop_ctl.reg_crop_enable);

}
#pragma GCC diagnostic pop
#define DEFINE_IVE_UPDATE_BG_C(X) \
	 ive_update_bg_c X = \
{\
	{	/* reg_1.reg_softrst = */0x0,\
	},\
	{	/*.reg_h04.reg_enable = */0x0,\
	/*.reg_h04.reg_ck_en = */0x1,\
	/*.reg_h04.reg_updatebg_byp_model = */0x0,\
	},\
	{	/*.reg_2.reg_shdw_sel = */0x1,\
	},\
	{	/*.reg_3.reg_ctrl_dmy1 = */0x0,\
	},\
	{	/*.reg_ctrl0.reg_u32CurFrmNum = */0x0,\
	},\
	{	/*.reg_ctrl1.reg_u32PreChkTime = */0x0,\
	},\
	{	/*.reg_ctrl2.reg_u32FrmChkPeriod = */0x32,\
	/*uint32_t rsv_12_15:4;=*/0,\
	/*.reg_ctrl2.reg_u32InitMinTime = */0x64,\
	},\
	{	/*.reg_ctrl3.reg_u32StyBgMinBlendTime = */0xC8,\
	/*.reg_ctrl3.reg_u32StyBgMaxBlendTime = */0xC8,\
	},\
	{	/*.reg_ctrl4.reg_u32DynBgMinBlendTime = */0x0,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.reg_ctrl4.reg_u32StaticDetMinTime = */0x50,\
	},\
	{	/*.reg_ctrl5.reg_u16FgMaxFadeTime = */0xF,\
	/*.reg_ctrl5.reg_u16BgMaxFadeTime = */0x3C,\
	/*.reg_ctrl5.reg_u8StyBgAccTimeRateThr = */0x50,\
	/*.reg_ctrl5.reg_u8ChgBgAccTimeRateThr = */0x3C,\
	},\
	{	/*.reg_ctrl6.reg_u8DynBgAccTimeThr = */0x0,\
	/*uint32_t rsv_6_7:2;=*/0,\
	/*.reg_ctrl6.reg_u8BgEffStaRateThr = */0x5A,\
	/*uint32_t rsv_15_15:1;=*/0,\
	/*.reg_ctrl6.reg_u8DynBgDepth = */0x3,\
	/*uint32_t rsv_18_23:6;=*/0,\
	/*.reg_ctrl6.reg_u8AcceBgLearn = */0x0,\
	/*.reg_ctrl6.reg_u8DetChgRegion = */0x0,\
	},\
	{	/*.reg_ctrl7.reg_stat_pixnum = */0x0,\
	},\
	{	/*.reg_ctrl8.reg_stat_sumlum = */0x0,\
	},\
	{	/*.reg_crop_s.reg_crop_start_x = */0x0,\
	/*.reg_crop_s.reg_crop_end_x = */0x0,\
	},\
	{	/*.reg_crop_e.reg_crop_start_y = */0x0,\
	/*.reg_crop_e.reg_crop_end_y = */0x0,\
	},\
	{	/*.reg_crop_ctl.reg_crop_enable = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_UPDATE_BG_C \
{\
	.reg_1.reg_softrst = 0x0,\
	.reg_h04.reg_enable = 0x0,\
	.reg_h04.reg_ck_en = 0x1,\
	.reg_h04.reg_updatebg_byp_model = 0x0,\
	.reg_2.reg_shdw_sel = 0x1,\
	.reg_3.reg_ctrl_dmy1 = 0x0,\
	.reg_ctrl0.reg_u32CurFrmNum = 0x0,\
	.reg_ctrl1.reg_u32PreChkTime = 0x0,\
	.reg_ctrl2.reg_u32FrmChkPeriod = 0x32,\
	.reg_ctrl2.reg_u32InitMinTime = 0x64,\
	.reg_ctrl3.reg_u32StyBgMinBlendTime = 0xC8,\
	.reg_ctrl3.reg_u32StyBgMaxBlendTime = 0xC8,\
	.reg_ctrl4.reg_u32DynBgMinBlendTime = 0x0,\
	.reg_ctrl4.reg_u32StaticDetMinTime = 0x50,\
	.reg_ctrl5.reg_u16FgMaxFadeTime = 0xF,\
	.reg_ctrl5.reg_u16BgMaxFadeTime = 0x3C,\
	.reg_ctrl5.reg_u8StyBgAccTimeRateThr = 0x50,\
	.reg_ctrl5.reg_u8ChgBgAccTimeRateThr = 0x3C,\
	.reg_ctrl6.reg_u8DynBgAccTimeThr = 0x0,\
	.reg_ctrl6.reg_u8BgEffStaRateThr = 0x5A,\
	.reg_ctrl6.reg_u8DynBgDepth = 0x3,\
	.reg_ctrl6.reg_u8AcceBgLearn = 0x0,\
	.reg_ctrl6.reg_u8DetChgRegion = 0x0,\
	.reg_ctrl7.reg_stat_pixnum = 0x0,\
	.reg_ctrl8.reg_stat_sumlum = 0x0,\
	.reg_crop_s.reg_crop_start_x = 0x0,\
	.reg_crop_s.reg_crop_end_x = 0x0,\
	.reg_crop_e.reg_crop_start_y = 0x0,\
	.reg_crop_e.reg_crop_end_y = 0x0,\
	.reg_crop_ctl.reg_crop_enable = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_UPDATE_BG_STRUCT_H__

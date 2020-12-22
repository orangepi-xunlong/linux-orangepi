#ifndef __DISP_CLK_H__
#define __DISP_CLK_H__

#include "disp_display_i.h"

typedef struct
{
	__u32 tve_clk;	//required clock frequency for LCDx_ch1_clk2, for tv output used ,Hz
	__u32 pre_scale;//required divide LCDx_ch1_clk2 by 2 for LCDx_ch1_clk1 or NOT: 1:not divided , 2: divide by two
	__u32 hdmi_clk; //required clock frequency for internal hdmi module, Hz
	__u32 pll_clk;	//required pll frequency for VIDEO_PLL0(1x) or VIDEO_PLL1(1x), Hz
	__u32 pll_2x;	//required 2x VIDEO_PLL or NOT: 0:no, 1: required
}__disp_tv_vga_clk_t;	//record tv/vga/hdmi mode clock requirement

typedef struct
{
	__disp_tv_vga_clk_t tv_clk_tab[30];	//number related to number of tv mode supported
	__disp_tv_vga_clk_t vga_clk_tab[12];//number related to number of vga mode supported
}__disp_clk_tab;

typedef struct
{
	__u32 factor_n;
	__u32 factor_k;
	__u32 divider_m;
}__disp_ccmu_coef;


typedef struct __CCMU_MIPI_PLL_REG0040
{
	__u32   FactorM:4;          //bit0,  PLL FactorM
	__u32   FactorK:2;          //bit4,  PLL FactorM
	__u32   reserved0:2;        //bit6,  reserved
	__u32   FactorN:4;          //bit8,  PLL factor N
	__u32   reserved1:4;        //bit12, reserved
	__u32   VfbSel:1;           //bit16, 0-mipi mode(n,k,m valid), 1-hdmi mode(sint_frac, sdiv2
	                            //       s6p25_7p5, pll_feedback_div valid)
	__u32   FeedBackDiv:1;      //bit17, pll feedback division, 0:x5, 1:x7
	__u32   reserved2:2;        //bit18, reserved
	__u32   SdmEn:1;            //bit20, sdm enable
	__u32   PllSrc:1;           //bit21, PLL source, 0:video pll0, 1:video pll1
	__u32   Ldo2En:1;           //bit22, LDO2 enable
	__u32   Ldo1En:1;           //bit23, LDO1 enable
	__u32   reserved3:1;        //bit24, reserved
	__u32   Sel625Or750:1;      //bit25, screen_idect pll out is input*6.25 or 7.50
	__u32   SDiv2:1;            //bit26, PLL output seclect, 0:pll output, 1:pll output x2
	__u32   FracMode:1;         //bit27, PLL output mode, 0:integer mode, 1:fraction mode
	__u32   Lock:1;             //bit28, lock flag
	__u32   reserved4:2;        //bit29, reserved
	__u32   PLLEn:1;            //bit31, PLL enable
} __ccmu_mipi_pll_reg0040_t;

typedef struct __CCMU_MIPI_PLL_BIAS_REG0240
{
	__u32  reserved0:28;           //bit0
	__u32  pllvdd_ldo_out_ctrl:3;  //bit28, pll ldo output control
	__u32  vco_rst:31;             //bit31, vco reset in
}__ccmu_mipi_pll_bias_reg0240_t;

__s32 image_clk_init(__u32 screen_id);
__s32 image_clk_exit(__u32 screen_id);
__s32 image_clk_on(__u32 screen_id, __u32 type);
__s32 image_clk_off(__u32 screen_id, __u32 type);

__s32 scaler_clk_init(__u32 scaler_id);
__s32 scaler_clk_exit(__u32 scaler_id);
__s32 scaler_clk_on(__u32 scaler_id);
__s32 scaler_clk_off(__u32 scaler_id);

__s32 lcdc_clk_init(__u32 screen_id);
__s32 lcdc_clk_exit(__u32 screen_id);
__s32 lcdc_clk_on(__u32 screen_id, __u32 tcon_index, __u32 type);
__s32 lcdc_clk_off(__u32 screen_id);

__s32 tve_clk_init(__u32 screen_id);
__s32 tve_clk_exit(__u32 screen_id);
__s32 tve_clk_on(__u32 screen_id);
__s32 tve_clk_off(__u32 screen_id);

__s32 hdmi_clk_init(void);
__s32 hdmi_clk_exit(void);
__s32 hdmi_clk_on(void);
__s32 hdmi_clk_off(void);

__s32 lvds_clk_init(void);
__s32 lvds_clk_exit(void);
__s32 lvds_clk_on(void);
__s32 lvds_clk_off(void);

__s32 dsi_clk_init(void);
__s32 dsi_clk_exit(void);
__s32 dsi_clk_on(void);
__s32 dsi_clk_off(void);


__s32 disp_pll_init(void);
__s32 disp_clk_cfg(__u32 screen_id, __u32 type, __u8 mode);
__s32 disp_clk_adjust(__u32 screen_id, __u32 type);
__s32 disp_mipipll_init(void);
__s32 disp_mipipll_enable(__u32 en);

extern __disp_clk_tab clk_tab;

#endif

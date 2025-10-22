/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef INNO_PHY_H_
#define INNO_PHY_H_

#define HDMI_PHY_PLL_BASEADDR		0x05530000

/**
 * Bring up PHY and start sending media for a specified pixel clock, pixel
 * repetition and color resolution (to calculate TMDS) - this fields must
 * be configured in the dev->ctrl_dev variables
 * @param dev device structure
 * return true if success, false if not success and -1 if PHY configurations
 * are not supported.
 */
int inno_phy_config(void);

int inno_phy_write(u8 addr, void *data);

int inno_phy_read(u8 addr, void *data);

ssize_t inno_phy_dump(char *buf);

int inno_phy_init(void);

typedef union {
	u8 dwval;
	struct {
		u8 phy_pll_refclk_control	: 1; /* Default: 0;0:crystal oscillator clock */
		u8 tmdsclk_invert	    	: 1; /* Default: 0;0: normal;1:invert */
		u8 prepclk_invert			: 1; /* Default: 0;0: normal;1:invert */
		u8 postclk_invert			: 1; /* Default: 0;0: normal;1:invert  not sure */
		u8 res0				     	: 2; /* Default: 0; */
		u8 rst_di			 		: 1; /* Default: 1;0:reset ; 1:normal */
		u8 rst_an			    	: 1; /* Default: 1;0:reset ; 1:normal */
	} bits;
} HDMI_PHY_CTL0_0_t; /* 0x00 */

typedef union {
	u8 dwval;
	struct {
		u8 res1					: 8; /* Default: 0 */
	} bits;
} HDMI_PHY_CTL0_1_t; /* 0x01 */

typedef union {
	u8 dwval;
	struct {
		u8 data_sy_ctl		: 1; /* Default: 1;0:disable;1:enable */
		u8 res2				: 4; /* Default: 0; */
		u8 inter_tep		: 1; /* Default: 0; 0:normal; 1:internal test pattern */
		u8 res3				: 1; /* Default: 0; */
		u8 int_polar_ctl	: 1; /* Default: 1;0:low active;1:high active */
	} bits;
} HDMI_PHY_CTL0_2_t; /* 0x02 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_ser_state_int		: 1; /* Default: 0;0:ignore ;1:int active */
		u8 ch1_ser_state_int		: 1; /* Default: 0;0:ignore ;1:int active */
		u8 ch2_ser_state_int		: 1; /* Default: 0;0:ignore ;1:int active */
		u8 res4						: 1; /* Default: 0; */
		u8 postpll_lock_state_int	: 1; /* Default: 0;0:ignore ;1:int active */
		u8 prepll_lock_state_int	: 1; /* Default: 0;0:ignore ;1:int active */
		u8 res5						: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_CTL0_3_t; /* 0x03 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_ser_int			: 1; /* Default: 0;0:undetected;1:normal status detected */
		u8 ch1_ser_int			: 1; /* Default: 0;0:undetected;1:normal status detected */
		u8 ch2_ser_int			: 1; /* Default: 0;0:undetected;1:normal status detected */
		u8 res0					: 1; /* Default: 0; */
		u8 postpll_lock_int	    : 1; /* Default: 0;0:undetected;1:normal status detected */
		u8 prepll_lock_int		: 1; /* Default: 0;0:undetected;1:normal status detected */
		u8 res1				    : 2; /* Default: 0; */
	} bits;
} HDMI_PHY_ESD0_0_t; /* 0x04 */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_ESD_intde	: 4; /* Default: 0;4bits,every bit:0:ignore ;1:int active */
		u8 clk_ESD_intde	: 4; /* Default: 0;4bits,every bit:0:ignore ;1:int active */
	} bits;
} HDMI_PHY_ESD0_1_t; /* 0x05 */


typedef union {
	u8 dwval;
	struct {
		u8 ch2_ESD_int		: 4; /* Default: 0;4bits,every bit:0:undetected;1:normal status detected */
		u8 clk_ESD_int		: 4; /* Default: 0;4bits,every bit:0:undetected;1:normal status detected */
	} bits;
} HDMI_PHY_ESD0_2_t; /* 0x06 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_ESD_intde	: 4; /* Default: 0;4bits,every bit:0:ignore ;1:int active */
		u8 ch1_ESD_intde	: 4; /* Default: 0;4bits,every bit:0:ignore ;1:int active */
	} bits;
} HDMI_PHY_ESD0_3_t; /* 0x07 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_ESD_int		: 4; /* Default: 0;4bits,every bit:0:undetected;1:normal status detected */
		u8 ch1_ESD_int		: 4; /* Default: 0;4bits,every bit:0:undetected;1:normal status detected */
	} bits;
} HDMI_PHY_ESD1_0_t; /* 0x08 */

typedef union {
	u8 dwval;
	struct {
		u8 res0			: 8; /* Default: 0; */
	} bits;
} HDMI_PHY_ESD1_1_t; /* 0x09 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_wap		: 1; /* Default: 0; */
		u8 ch1_wap		: 1; /* Default: 0; */
		u8 ch2_wap		: 1; /* Default: 0; */
		u8 res1			: 5; /* Default: 0; */
	} bits;
} HDMI_PHY_SW_t; /* 0x0D */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_inter_tep0		: 8; /* Default: 0x55; */
	} bits;
} HDMI_PHY_TEP0_0_t; /* 0x10 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_inter_tep1		: 2; /* Default: 0x55(not sure); */
		u8 res0					: 6;
	} bits;
} HDMI_PHY_TEP0_1_t; /* 0x11 */

typedef union {
	u8 dwval;
	struct {
		u8 ch1_inter_tep0		: 8; /* Default: 0x55; */
	} bits;
} HDMI_PHY_TEP1_0_t; /* 0x20 */

typedef union {
	u8 dwval;
	struct {
		u8 ch1_inter_tep1			: 2; /* Default: 0x55(not sure); */
		u8 res0                    : 6;
	} bits;
} HDMI_PHY_TEP1_1_t; /* 0x21 */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_inter_tep0			: 8; /* Default: 0x55; */
	} bits;
} HDMI_PHY_TEP2_0_t; /* 0x30 */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_inter_tep1			: 2; /* Default: 0x55(not sure); */
		u8 res0						: 6;
	} bits;
} HDMI_PHY_TEP2_1_t; /* 0x31 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_pow			: 1; /* Default: 1;0:on;1:off; */
		u8 pix_clk_bp			: 1; /* Default: 0;0:normal;1:bypass,by pass will be divided by 5 from pre_PLL VCO */
		u8 prepll_bp			: 1; /* Default: 0;0:normall1:bypass,by pass refclk */
		u8 prepll_div_en		: 1; /* Default: 0;0:disable;1:enable,divide 2 to post */
		u8 prepll_lock_bp		: 1; /* Default: 0;0:normal,post starts to work until pre lock;1:bypass,post same as pre */
		u8 res0					: 3; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL0_0_t; /* 0xA0 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_div		: 6; /* Default: 1;0:1;1:1;2:2;0x3f:36 */
		u8 res1				: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL0_1_t; /* 0xA1 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_fbdiv1			: 4; /* Default: 0; feedback divider bit [11:8] */
		u8 prepll_fra_ctl			: 2; /* Default: 0x11; 0x00:enable;0x11:disable;other invalid */
		u8 prepll_SSC_mdu			: 1; /* Default: 1;0:enable;1:disable; */
		u8 prepll_SSC_md			: 1; /* Default: 1;0:center;1:down; */
	} bits;
} HDMI_PHY_PLL0_2_t; /* 0x00A2 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_fbdiv0			: 8; /* Default: 5;0:1;1:1;2:2;0xfff:4095  bit[7:0] */
	} bits;
} HDMI_PHY_PLL0_3_t; /* 0x00A3 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_tmdsclk_div	    : 2; /* Default: 01;00:1;01:2;10:4;11:8; */
		u8 prepll_linkclk_div	    : 2; /* Default: 01;00:1;01:2;10:4;11:8; */
		u8 prepll_linktmdsclk_div	: 2; /* Default: 01;00:1;01:2;10:3;11:5; */
		u8 res0			            : 2; /* Default: 0;0:disable;1:enable,divide 2 to post */
	} bits;
} HDMI_PHY_PLL1_0_t; /* 0xA4 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_auxclk_div	    : 5; /* Default: 1;00:1;01:1;10:2;0x1f:31; */
		u8 prepll_mainclk_div	    : 2; /* Default: 2;00:2;01:3;10:4;11:5; */
		u8 res1			            : 1; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL1_1_t; /* 0xA5 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_pixclk_div	: 5; /* Default: 2;00:1;01:1;10:2;0x1f:31; */
		u8 prepll_reclk_div		: 2; /* Default: 2;00:2;01:3;10:4;11:5; */
		u8 res2					: 1; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL1_2_t; /* 0xA6 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_ppg			: 3; /* Default: 2;min:000;max:111; */
		u8 prepll_ipg			: 3; /* Default: 3;min:000;max:111; */
		u8 prepll_vcog			: 2; /* Default: 1;min:00; max:11; */
	} bits;
} HDMI_PHY_PLL1_3_t; /* 0xA7 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_monitor		: 8; /* Default: 0x80; */
	} bits;
} HDMI_PHY_PLL2_0_t; /* 0xA8 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_lock_state	: 1; /* Default: 0;0:unlock;1:lock; */
		u8 res0					: 7; /* Default: 0; (not sure); */
	} bits;
} HDMI_PHY_PLL2_1_t; /* 0xA9 */

typedef union {
	u8 dwval;
	struct {
		u8 postpll_pow				: 1; /* Default: 1;0:on;1:off; */
		u8 postpll_reclk			: 1; /* Default: 1;0:crystal oscillator;1:pre-PLL; */
		u8 postpll_postdiv_en		: 2; /* Default: 3;00:diasable;11:enable;other:invalid; */
		u8 res1						: 4; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL2_2_t; /* 0x00AA */

typedef union {
	u8 dwval;
	struct {
		u8 postpll_pred_div			: 5; /* Default: 1;00:1;01:1;10:2;0x1f:31; */
		u8 res2						: 2; /* Default: 0; */
		u8 postpll_linkclk_div		: 1; /* Default: 0;0:5;1:20 */
	} bits;
} HDMI_PHY_PLL2_3_t; /* 0x00AB */

typedef union {
	u8 dwval;
	struct {
		u8 postpll_fbdiv0			: 8; /* Default: 0x14;0:1;1:1;2:2;0xff:511; */
	} bits;
} HDMI_PHY_PLL3_0_t; /* 0x00AC */

typedef union {
	u8 dwval;
	struct {
		u8 postpll_postdiv		: 3; /* Default: 1;000:2;001:4;011:8;101:16;111:32; */
		u8 linkcolor			: 1; /* Default: 1;0:by post;1:by pre; */
		u8 postpll_fbdiv1		: 1; /* Default: 0; */
		u8 res0					: 1; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL3_1_t; /* 0x00AD */

typedef union {
	u8 dwval;
	struct {
		u8 postpll_ppg			: 3; /* Default: 2;min:000;max:111; */
		u8 postpll_ipg			: 3; /* Default: 3;min:000;max:111; */
		u8 postpll_vcog			: 2; /* Default: 1;min:00; max:11; */
	} bits;
} HDMI_PHY_PLL3_2_t; /* 0x00AE */

typedef union {
	u8 dwval;
	struct {
		u8 postpll_lock_state		: 1; /* Default: 0;0:unlock;1:lock; */
		u8 postpll_monitor			: 7; /* Default: 0x00; */
	} bits;
} HDMI_PHY_PLL3_3_t; /* 0x00AF */

typedef union {
	u8 dwval;
	struct {
		u8 bandgap_ref_bp			: 1; /* Default: 0;0:normall1:bypass */
		u8 refres					: 1; /* Default: 1;0:off-chip;1:on-chip */
		u8 bias_en					: 1; /* Default: 0;0:disable;1:enable; */
		u8 bandgap_choppre_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 vref0					: 4; /* Default: 0; */
	} bits;
} HDMI_PHY_DR0_0_t; /* 0xB0 */

typedef union {
	u8 dwval;
	struct {
		u8 vref1			: 8; /* Default: 0; */
	} bits;
} HDMI_PHY_DR0_1_t; /* 0xB1 */

typedef union{
	u8 dwval;
	struct {
		u8 ch0_dr_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch1_dr_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch2_dr_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 clk_dr_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch0_data_by		: 1; /* Default: 0;0:normal; 1:select bit clock output; */
		u8 ch1_data_by		: 1; /* Default: 0;0:normal; 1:select bit clock output; */
		u8 ch2_data_by		: 1; /* Default: 0;0:normal; 1:select bit clock output; */
		u8 res0				: 1; /* Default: 0; */

	} bits;
} HDMI_PHY_DR0_2_t; /* 0xB2 */

typedef union {
	u8 dwval;
	struct {
		u8 clk_pre_empl		: 3; /* Default: 0;000:disable;001:1;010:2;other: */
		u8 res3				: 1; /* Default: 0; */
		u8 clk_post_empl	: 4; /* Default: 0;0000:disable;0001:1;1000:8;other:  */
	} bits;
} HDMI_PHY_DR0_3_t; /* 0xB3 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_LDO_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch1_LDO_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch2_LDO_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 clk_LDO_en		: 1; /* Default: 0; */
		u8 ch0_BIST_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch1_BIST_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch2_BIST_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 res1				: 1; /* Default: 0; */

	} bits;
} HDMI_PHY_DR1_0_t; /* 0xB4 */

typedef union {
	u8 dwval;
	struct {
		u8 clk_vlevel			: 5; /* Default: 28;0x0:disable;0x1:1;0x1c:28; */
		u8 clkesd_threshold		: 2; /* Default: 0;min:00;max:11; */
		u8 res2					: 1; /* Default: 0; */

	} bits;
} HDMI_PHY_DR1_1_t; /* 0xB5 */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_vlevel			: 5; /* Default: 28;0x0:disable;0x1:1;0x1c:28; */
		u8 ch2esd_threshold		: 2; /* Default: 0;min:00;max:11; */
		u8 res3					: 1; /* Default: 0; */
	} bits;
} HDMI_PHY_DR1_2_t; /* 0xB6 */

typedef union {
	u8 dwval;
	struct {
		u8 ch1_vlevel			: 5; /* Default: 28;0x0:disable;0x1:1;0x1c:28; */
		u8 ch1esd_threshold		: 2; /* Default: 0;min:00;max:11; */
		u8 res4					: 1; /* Default: 0; */
	} bits;
} HDMI_PHY_DR1_3_t; /* 0xB7 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_vlevel			: 5; /* Default: 28;0x0:disable;0x1:1;0x1c:28; */
		u8 ch0esd_threshold		: 2; /* Default: 0;min:00;max:11; */
		u8 res0					: 1; /* Default: 0; */
	} bits;
} HDMI_PHY_DR2_0_t; /* 0xB8 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_LDO_cur		: 1; /* Default: 0;0:small;1:large; */
		u8 ch1_LDO_cur		: 1; /* Default: 0;0:small;1:large; */
		u8 ch2_LDO_cur		: 1; /* Default: 0;0:small;1:large; */
		u8 res1				: 5; /* Default: 0; */
	} bits;
} HDMI_PHY_DR2_1_t; /* 0xB9 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_LDO_v		: 2; /* Default: 1;00:850mV;01:900mV;10:950mV;11:1000mV */
		u8 ch1_LDO_v		: 2; /* Default: 1;00:850mV;01:900mV;10:950mV;11:1000mV */
		u8 ch2_LDO_v		: 2; /* Default: 1;00:850mV;01:900mV;10:950mV;11:1000mV */
		u8 res2				: 2; /* Default: 1;(not sure) */
	} bits;
} HDMI_PHY_DR2_2_t; /* 0xBA */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_pre_empl			: 3; /* Default: 0;000:disable;001:1;010:2;other: */
		u8 res3					: 1; /* Default: 0; */
		u8 ch2_post_empl		: 4; /* Default: 0;0000:disable;0001:1;1000:8;other: */
	} bits;
} HDMI_PHY_DR2_3_t; /* 0xBB */

typedef union {
	u8 dwval;
	struct {
		u8 ch1_pre_empl		: 3; /* Default: 0;000:disable;001:1;010:2;other: */
		u8 res0				: 1; /* Default: 0; */
		u8 ch1_post_empl	: 4; /* Default: 0;0000:disable;0001:1;1000:8;other: */
	} bits;
} HDMI_PHY_DR3_0_t; /* 0xBC */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_pre_empl		: 3; /* Default: 0;000:disable;001:1;010:2;other: */
		u8 res1				: 1; /* Default: 0; */
		u8 ch0_post_empl	: 4; /* Default: 0;0000:disable;0001:1;1000:8;other: */

	} bits;
} HDMI_PHY_DR3_1_t; /* 0x00BD */

typedef union {
	u8 dwval;
	struct {
		u8 res2				: 4; /* Default: 0; */
		u8 ch0_seri_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch1_seri_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch2_seri_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 res3				: 1; /* Default: 0; */
	} bits;
} HDMI_PHY_DR3_2_t; /* 0x00BE */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_cur_bias		: 4; /* Default: 0;20uA per bit;320uA-920uA */
		u8 clk_cur_bias		: 4; /* Default: 0;20uA per bit;320uA-920uA */
	} bits;
} HDMI_PHY_DR3_3_t; /* 0x00BF */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_cur_bias			: 4; /* Default: 0;20uA per bit;320uA-920uA */
		u8 ch1_cur_bias			: 4; /* Default: 0;20uA per bit;320uA-920uA */
	} bits;
} HDMI_PHY_DR4_0_t; /* 0x00C0 */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_seri_ph		: 2; /* Default: 0;00:default;01:ahead of 1 bit of clk; 2:2;3:3; */
		u8 ch1_seri_ph		: 2; /* Default: 0;00:default;01:ahead of 1 bit of clk; 2:2;3:3; */
		u8 ch2_seri_ph		: 2; /* Default: 0;00:default;01:ahead of 1 bit of clk; 2:2;3:3; */
		u8 res0				: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_DR4_1_t; /* 0x00C1 */

typedef union {
	u8 dwval;
	struct {
		u8 terrescal_result		: 6; /* Default: 0; */
		u8 res0					: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_DR5_0_t; /* 0xC4 */

typedef union {
	u8 dwval;
	struct {
		u8 terrescal_clkdiv1		: 7; /* Default: 0x4; */
		u8 terrescal_bp				: 1; /* Default: 0;0:auto;1:bp auto; */
	} bits;
} HDMI_PHY_DR5_1_t; /* 0xC5 */

typedef union {
	u8 dwval;
	struct {
		u8 terrescal_clkdiv0		: 8; /* Default: 0; */
	} bits;
} HDMI_PHY_DR5_2_t; /* 0xC6 */

typedef union {
	u8 dwval;
	struct {
		u8 res1				: 1; /* Default: 0;100-200 50o perbit */
		u8 terres_val		: 2; /* Default: 0; */
		u8 res2				: 1; /* Default: 0;100-200 50o perbit */
		u8 ch0_terrescal	: 1; /* Default: 0;0:don't configure 0xcb;1:configure; */
		u8 ch1_terrescal	: 1; /* Default: 0;0:don't configure 0xca;1:configure; */
		u8 ch2_terrescal	: 1; /* Default: 0;0:don't configure 0xc9;1:configure; */
		u8 clk_terrescal	: 1; /* Default: 0;0:don't configure 0xc8;1:configure; */
	} bits;
} HDMI_PHY_DR5_3_t; /* 0xC7 */

typedef union {
	u8 dwval;
	struct {
		u8 clkterres_ndiv	: 6; /* Default: 0; RT=4000/NUM; */
		u8 res0				: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_DR6_0_t; /* 0xC8 */

typedef union {
	u8 dwval;
	struct {
		u8 ch2terres_ndiv		: 6; /* Default: 0; RT=4000/NUM; */
		u8 res1					: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_DR6_1_t; /* 0xC9 */

typedef union {
	u8 dwval;
	struct {
		u8 ch1terres_ndiv	: 6; /* Default: 0; RT=4000/NUM; */
		u8 res2				: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_DR6_2_t; /* 0xCA */

typedef union {
	u8 dwval;
	struct {
		u8 ch0terres_ndiv		: 6; /* Default: 0; RT=4000/NUM; */
		u8 res3					: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_DR6_3_t; /* 0xCB */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_rxsense_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch1_rxsense_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 ch2_rxsense_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 clk_rxsense_en		: 1; /* Default: 0;0:disable;1:enable; */
		u8 res0					: 4; /* Default: 0; */
	} bits;
} HDMI_PHY_RXSEN_ESD_0_t; /* 0xCC */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_rxsense_de_sta		: 2; /* Default: 0; */
		u8 ch1_rxsense_de_sta		: 2; /* Default: 0; */
		u8 ch2_rxsense_de_sta		: 2; /* Default: 0; */
		u8 clk_rxsense_de_sta		: 2; /* Default: 0; */
	} bits;
} HDMI_PHY_RXSEN_ESD_1_t; /* 0xCD */

typedef union {
	u8 dwval;
	struct {
		u8 ch2_esd_de_sta		: 4; /* Default: 0; */
		u8 clk_esd_de_sta		: 4; /* Default: 0; */
	} bits;
} HDMI_PHY_RXSEN_ESD_2_t; /* 0xCE */

typedef union {
	u8 dwval;
	struct {
		u8 ch0_esd_de_sta		: 4; /* Default: 0; */
		u8 ch1_esd_de_sta		: 4; /* Default: 0; */
	} bits;
} HDMI_PHY_RXSEN_ESD_3_t; /* 0xCF */

typedef union {
	u8 dwval;
	struct {
		u8 res0			: 8; /* Default: 0; */
	} bits;
} HDMI_PHY_PLL_FRA_0_t; /* 0xD0 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_fra_div2		: 8; /* Default: 0;[23:16] */
	} bits;
} HDMI_PHY_PLL_FRA_1_t; /* 0xD1 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_fra_div1		: 8; /* Default: 0;[15:8] */
	} bits;
} HDMI_PHY_PLL_FRA_2_t; /* 0xD2 */

typedef union {
	u8 dwval;
	struct {
		u8 prepll_fra_div0		: 8; /* Default: 0;[7:0] */
	} bits;
} HDMI_PHY_PLL_FRA_3_t; /* 0xD3 */

struct __inno_phy_reg_t {
	HDMI_PHY_CTL0_0_t		hdmi_phy_ctl0_0; /* 0x00 */
	HDMI_PHY_CTL0_1_t		hdmi_phy_ctl0_1; /* 0x01 */
	HDMI_PHY_CTL0_2_t		hdmi_phy_ctl0_2; /* 0x02 */
	HDMI_PHY_CTL0_3_t		hdmi_phy_ctl0_3; /* 0x03*/
	HDMI_PHY_ESD0_0_t		hdmi_phy_esd0_0; /* 0x04*/
	HDMI_PHY_ESD0_1_t		hdmi_phy_esd0_1; /* 0x05*/
	HDMI_PHY_ESD0_2_t		hdmi_phy_esd0_2; /* 0x06*/
	HDMI_PHY_ESD0_3_t		hdmi_phy_esd0_3; /* 0x07*/
	HDMI_PHY_ESD1_0_t		hdmi_phy_esd1_0; /* 0x08*/
	HDMI_PHY_ESD1_1_t		hdmi_phy_esd1_1; /* 0x09*/
	u8 reg1[3];								/*0x0A-0x0C*/
	HDMI_PHY_SW_t			hdmi_phy_sw;	/* 0x0D*/
	u8 reg2[2]; 							/*0x0E-0x0F*/
	HDMI_PHY_TEP0_0_t		hdmi_phy_tep0_0;/* 0x10*/
	HDMI_PHY_TEP0_1_t		hdmi_phy_tep0_1;/* 0x11*/
	u8 reg3[14]; 							/*0x12-0x1F*/
	HDMI_PHY_TEP1_0_t		hdmi_phy_tep1_0;/* 0x20*/
	HDMI_PHY_TEP1_1_t		hdmi_phy_tep1_1;/* 0x21*/
	u8 reg4[14]; 							/*0x22-0x2F*/
	HDMI_PHY_TEP2_0_t		hdmi_phy_tep2_0;/* 0x30*/
	HDMI_PHY_TEP2_1_t		hdmi_phy_tep2_1;/* 0x31*/
	u8 reg5[110]; 							/*0x32-0x9F*/
	HDMI_PHY_PLL0_0_t		hdmi_phy_pll0_0;/* 0xA0*/
	HDMI_PHY_PLL0_1_t		hdmi_phy_pll0_1;/* 0xA1*/
	HDMI_PHY_PLL0_2_t		hdmi_phy_pll0_2;/* 0xA2*/
	HDMI_PHY_PLL0_3_t		hdmi_phy_pll0_3;/* 0xA3*/
	HDMI_PHY_PLL1_0_t		hdmi_phy_pll1_0;/* 0xA4*/
	HDMI_PHY_PLL1_1_t		hdmi_phy_pll1_1;/* 0xA5*/
	HDMI_PHY_PLL1_2_t		hdmi_phy_pll1_2;/* 0xA6 */
	HDMI_PHY_PLL1_3_t		hdmi_phy_pll1_3;/* 0xA7 */
	HDMI_PHY_PLL2_0_t		hdmi_phy_pll2_0;/* 0xA8 */
	HDMI_PHY_PLL2_1_t		hdmi_phy_pll2_1;/* 0xA9*/
	HDMI_PHY_PLL2_2_t		hdmi_phy_pll2_2;/* 0xAA*/
	HDMI_PHY_PLL2_3_t		hdmi_phy_pll2_3;/* 0xAB*/
	HDMI_PHY_PLL3_0_t		hdmi_phy_pll3_0;/* 0xAC*/
	HDMI_PHY_PLL3_1_t		hdmi_phy_pll3_1;/* 0xAD*/
	HDMI_PHY_PLL3_2_t		hdmi_phy_pll3_2;/* 0xAE*/
	HDMI_PHY_PLL3_3_t		hdmi_phy_pll3_3;/* 0xAF*/
	HDMI_PHY_DR0_0_t		hdmi_phy_dr0_0;/* 0xB0*/
	HDMI_PHY_DR0_1_t		hdmi_phy_dr0_1;/* 0xB1 */
	HDMI_PHY_DR0_2_t		hdmi_phy_dr0_2;/* 0xB2 */
	HDMI_PHY_DR0_3_t		hdmi_phy_dr0_3;/* 0xB3*/
	HDMI_PHY_DR1_0_t		hdmi_phy_dr1_0;/* 0xB4*/
	HDMI_PHY_DR1_1_t		hdmi_phy_dr1_1;/* 0xB5*/
	HDMI_PHY_DR1_2_t		hdmi_phy_dr1_2;/* 0xB6*/
	HDMI_PHY_DR1_3_t		hdmi_phy_dr1_3;/* 0xB7*/
	HDMI_PHY_DR2_0_t		hdmi_phy_dr2_0;/* 0xB8*/
	HDMI_PHY_DR2_1_t		hdmi_phy_dr2_1;/* 0xB9*/
	HDMI_PHY_DR2_2_t		hdmi_phy_dr2_2;/* 0xBA */
	HDMI_PHY_DR2_3_t		hdmi_phy_dr2_3;/* 0xBB */
	HDMI_PHY_DR3_0_t		hdmi_phy_dr3_0;/* 0xBC */
	HDMI_PHY_DR3_1_t		hdmi_phy_dr3_1;/* 0xBD*/
	HDMI_PHY_DR3_2_t		hdmi_phy_dr3_2;/* 0xBE*/
	HDMI_PHY_DR3_3_t		hdmi_phy_dr3_3;/* 0xBF*/
	HDMI_PHY_DR4_0_t		hdmi_phy_dr4_0;/* 0xC0*/
	HDMI_PHY_DR4_1_t		hdmi_phy_dr4_1;/* 0xC1*/
	u8 reg6[2]; 							/*0xC2-0xC3*/
	HDMI_PHY_DR5_0_t		hdmi_phy_dr5_0;/* 0xC4*/
	HDMI_PHY_DR5_1_t		hdmi_phy_dr5_1;/* 0xC5*/
	HDMI_PHY_DR5_2_t		hdmi_phy_dr5_2;/* 0xC6*/
	HDMI_PHY_DR5_3_t		hdmi_phy_dr5_3;/* 0xC7*/
	HDMI_PHY_DR6_0_t		hdmi_phy_dr6_0;/* 0xC8*/
	HDMI_PHY_DR6_1_t		hdmi_phy_dr6_1;/* 0xC9*/
	HDMI_PHY_DR6_2_t		hdmi_phy_dr6_2;/* 0xCA*/
	HDMI_PHY_DR6_3_t		hdmi_phy_dr6_3;/* 0xCB*/
	HDMI_PHY_RXSEN_ESD_0_t	hdmi_phy_rxsen_esd_0;/* 0xCC*/
	HDMI_PHY_RXSEN_ESD_1_t	hdmi_phy_rxsen_esd_1;/* 0xCD*/
	HDMI_PHY_RXSEN_ESD_2_t	hdmi_phy_rxsen_esd_2;/* 0xCE*/
	HDMI_PHY_RXSEN_ESD_3_t	hdmi_phy_rxsen_esd_3;/* 0xCF*/
	HDMI_PHY_PLL_FRA_0_t	hdmi_phy_pll_fra_0;/* 0xD0*/
	HDMI_PHY_PLL_FRA_1_t	hdmi_phy_pll_fra_1;/* 0xD1*/
	HDMI_PHY_PLL_FRA_2_t	hdmi_phy_pll_fra_2;/* 0xD2*/
	HDMI_PHY_PLL_FRA_3_t	hdmi_phy_pll_fra_3;/* 0xD3*/
};

#endif	/* INNO_PHY_H_ */

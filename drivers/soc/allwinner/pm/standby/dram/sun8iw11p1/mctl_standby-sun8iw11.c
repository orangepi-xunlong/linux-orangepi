/*
****************************************************************************
      Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)

      File:                           mctl_standby.c

      Description:  This file implements basic functions for AW1699 DRAM controller

      History:
		2015/12/04			ZHUWEI			0.10		Initial version base on B100
		2016/01/18			ZHUWEI			0.20		LPDDR2 HDR always open
		2016/01/19			ZHUWEI			0.30		support master priority set
		2016/03/17			ZHUWEI			0.40		change de priority set
		2016/05/16			ZHUWEI			0.50		change tvin priority set
*****************************************************************************
*/
#include "mctl_reg-sun8iw11.h"
#include "mctl_hal-sun8iw11.h"
#include "mctl_para-sun8iw11.h"

#ifndef FPGA_VERIFY
/********************************************************************************
 *IC standby code
 ********************************************************************************/
static unsigned int reg[100] = { 0 };

void dram_udelay(unsigned int n)
{
#ifdef SYSTEM_VERIFY
	change_runtime_env();
	delay_us(n);
#else
	n = n * 100;
	while (--n)
		;
#endif
}

/*****************************************************************************
Function : DRAM Master Access Enable
parameter : Void
return value : Void
*****************************************************************************/
void dram_enable_all_master(void)
{
	/* enable all master */
	mctl_write_w(0xffffffff, MC_MAER);
	dram_udelay(10);
}

/*****************************************************************************
Function : DRAM Master Access Disable
parameter : Void
return value : Void
*****************************************************************************/
void dram_disable_all_master(void)
{
	/* disable all master except cpu/cpus */
	mctl_write_w(0x1, MC_MAER);
	dram_udelay(10);
}

/*****************************************************************************
Function : DRAM Standby Function Entry
parameter : Void
return value : Void
*****************************************************************************/
static unsigned int __dram_power_save_process()
{
	unsigned int reg_val = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	/*Save Data Before Enter Self-Refresh */
	reg[0] = mctl_read_w(MC_WORK_MODE);
	reg[1] = mctl_read_w(MC_R1_WORK_MODE);
	reg[2] = mctl_read_w(DRAMTMG0);
	reg[3] = mctl_read_w(DRAMTMG1);
	reg[4] = mctl_read_w(DRAMTMG2);
	reg[5] = mctl_read_w(DRAMTMG3);
	reg[6] = mctl_read_w(DRAMTMG4);
	reg[7] = mctl_read_w(DRAMTMG5);
	reg[8] = mctl_read_w(DRAMTMG8);
	reg[9] = mctl_read_w(PITMG0);
	reg[10] = mctl_read_w(PTR3);
	reg[11] = mctl_read_w(PTR4);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 11; j++) {
			reg[15 + j + (i * 11)] = mctl_read_w((DATX0IOCR(j) + i * 0x80));
		}
	}
	for (i = 0; i < 31; i++) {
		reg[60 + i] = mctl_read_w(ACIOCR1(i));
	}
	for (i = 0; i < 4; i++) {
		reg[95 + i] = mctl_read_w(DXnSDLR6(i));
	}
	/* disable all master access .
	 * After saving data ,disable all master access
	 */
	mctl_write_w(0, MC_MAER);
	dram_udelay(1);
	/* DRAM power down. */
	/*1.enter self refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1 << 0;
	reg_val |= (0x1 << 8);
	mctl_write_w(reg_val, PWRCTL);
	/*confirm dram controller has enter selfrefresh */
	while (((mctl_read_w(STATR) & 0x7) != 0x3))
		;
	dram_udelay(1);

	/*disable DRAM CK, the ck output will be tied '0' */
	reg_val = mctl_read_w(PGCR3);
	reg_val &= (~(0xffU << 16));
	mctl_write_w(reg_val, PGCR3);
	dram_udelay(1);

	/* 2.disable CK and power down pad include AC/DX/ pad,
	 * ZQ calibration module power down
	 */

	for (i = 0; i < 4; i++) {	/*DXIO POWER DOWN */
		reg_val = mctl_read_w(DXnGCR0(i));
		reg_val &= ~(0x3U << 22);
		reg_val |= (0x1U << 22);	/*DQS receiver off */
		reg_val &= ~(0xfU << 12);
		reg_val |= (0x5U << 12);	/*POWER DOWN RECEIVER/DRICER OFF */
		reg_val |= (0x2U << 2);	/*OE mode disable */
		reg_val |= (0x1 << 1);	/*IO CMOS mode */
		reg_val &= ~(0x1U << 0);	/*DQ GROUP OFF */
		mctl_write_w(reg_val, DXnGCR0(i));
	}
	reg_val = mctl_read_w(ACIOCR0);	/*CA IO POWER DOWN */
	reg_val |= (0x3U << 8);	/*CKE ENABLE */
	reg_val &= ~(0x3U << 6);	/*CK OUTPUT DISABLE */
	reg_val &= ~(0x1U << 3);	/*CA OE OFF */
	reg_val |= (0x1 << 4);	/*IO CMOS mode */
	reg_val |= (0x3U << 0);	/*CA POWER DOWN RECEIVER/DRIVER ON */
	mctl_write_w(reg_val, ACIOCR0);

	/* 3.pad hold */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val |= 0x3 << 0;
	mctl_write_w(reg_val, VDD_SYS_PWROFF_GATING);
	dram_udelay(10);

	/* 4.disable global clk and pll-ddr0/pll-ddr1 */
	mctl_write_w(0x0, CLKEN);
	/*disable pll-ddr0 */
	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	reg_val |= (0x1U << 20);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	/*disable pll-ddr1 */
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	reg_val |= (0x1U << 30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	/* 5.DRAM SCLK domain reset */
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1U << 31);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);

	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val |= (0x1U << 16);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);

	/* close DRAM AHB gate */
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val &= ~(0x1U << 14);
	mctl_write_w(reg_val, BUS_CLK_GATE_REG0);

	/* close MBUS CLK gate */
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);

	/* DRAM AHB domain reset */
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val &= ~(1U << 14);
	mctl_write_w(reg_val, BUS_RST_REG0);
	dram_udelay(1);

	return 0;
}

/*****************************************************************************
Function :	Set Bit Delay
parameter : DRAM parameter
return value : Void
*****************************************************************************/
void bit_delay_compensation_standby(__dram_para_t *para)
{
	unsigned int reg_val, i, j;

	reg_val = mctl_read_w(PGCR0);
	reg_val &= (~(0x1 << 26));
	mctl_write_w(reg_val, PGCR0);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 11; j++) {
			mctl_write_w(reg[15 + j + (i * 11)], (DATX0IOCR(j) + i * 0x80));
		}
	}
	for (i = 0; i < 31; i++) {
		mctl_write_w(reg[60 + i], ACIOCR1(i));
	}
	for (i = 0; i < 4; i++) {
		mctl_write_w(reg[95 + i], DXnSDLR6(i));
	}
	reg_val = mctl_read_w(PGCR0);
	reg_val |= (0x1 << 26);
	mctl_write_w(reg_val, PGCR0);
}

/*****************************************************************************
Function :	Set Master Priority
parameter : Void
return value : Void
*****************************************************************************/
void set_master_priority_standby(void)
{
	/*enable bandwidth limit windows and set windows size 1us*/
	mctl_write_w(0x18F, MC_TMR);
	mctl_write_w(0x00010000, MC_BWCR);
	/*set cpu high priority*/
	mctl_write_w(0x1, MC_MPFADR);
	/*set cpu*/
	mctl_write_w(0x00a0000d, MC_MnCR0(0));
	mctl_write_w(0x00500064, MC_MnCR1(0));
	/*set gpu*/
	mctl_write_w(0x06000009, MC_MnCR0(1));
	mctl_write_w(0x01000578, MC_MnCR1(1));
	/*set MAHB*/
	mctl_write_w(0x0200000d, MC_MnCR0(2));
	mctl_write_w(0x00600100, MC_MnCR1(2));
	/*set DMA*/
	mctl_write_w(0x01000009, MC_MnCR0(3));
	mctl_write_w(0x00500064, MC_MnCR1(3));
	/*set VE*/
	mctl_write_w(0x07000009, MC_MnCR0(4));
	mctl_write_w(0x01000640, MC_MnCR1(4));
	/*set CSI0*/
	mctl_write_w(0x01000009, MC_MnCR0(5));
	mctl_write_w(0x00000080, MC_MnCR1(5));
	/*set NAND*/
	mctl_write_w(0x01000009, MC_MnCR0(6));
	mctl_write_w(0x00400080, MC_MnCR1(6));
	/*set ss*/
	mctl_write_w(0x0100000d, MC_MnCR0(7));
	mctl_write_w(0x00400080, MC_MnCR1(7));
	/*set TS*/
	mctl_write_w(0x0100000d, MC_MnCR0(8));
	mctl_write_w(0x00400080, MC_MnCR1(8));
	/*set De-Interlace*/
	mctl_write_w(0x04000009, MC_MnCR0(9));
	mctl_write_w(0x00400100, MC_MnCR1(9));
	/*set CSI1*/
	mctl_write_w(0x00800009, MC_MnCR0(10));
	mctl_write_w(0x00000030, MC_MnCR1(10));
	/*set Mixer*/
	mctl_write_w(0x01800009, MC_MnCR0(11));
	mctl_write_w(0x00000100, MC_MnCR1(11));
	/*set TVIN*/
	mctl_write_w(0x0200000D, MC_MnCR0(12));
	mctl_write_w(0x01000180, MC_MnCR1(12));
	/*set DE*/
	mctl_write_w(0x2000020d, MC_MnCR0(13));
	mctl_write_w(0x04001800, MC_MnCR1(13));
	/*set ROT*/
	mctl_write_w(0x05000009, MC_MnCR0(14));
	mctl_write_w(0x00400090, MC_MnCR1(14));
	/* set ROT BW limit*/
	dram_dbg_8("DRAM master priority setting ok.\n");
}

/*****************************************************************************
Function : DRAM Timing Calculation Function
parameter : (Timing,CLK)
return value : Calculated Value
*****************************************************************************/
unsigned int auto_cal_timing_standby(unsigned int time_ns, unsigned int clk)
{
	unsigned int value;
	value = (time_ns * clk) / 1000 + ((((time_ns * clk) % 1000) != 0) ? 1 : 0);
	return value;
}

/*****************************************************************************
Function : DRAM Timing configuration Function
parameter : (DRAM parameter)
return value : void
*****************************************************************************/
void auto_set_timing_para_standby(__dram_para_t *para)
{
	unsigned int trefi = 0;
	unsigned int trfc = 0;
	unsigned int type = 0;
	unsigned int reg_val = 0;
	unsigned int ctrl_freq = 0;
	type = para->dram_type;
	ctrl_freq = para->dram_clk / 2;
	if (type == 3) {
		trefi = auto_cal_timing_standby(7800, ctrl_freq) / 32;
		trfc = auto_cal_timing_standby(350, ctrl_freq);
	} else if (type == 2) {
		trefi = auto_cal_timing_standby(7800, ctrl_freq) / 32;
		trfc = auto_cal_timing_standby(328, ctrl_freq);
	} else {
		trefi = auto_cal_timing_standby(3900, ctrl_freq) / 32;
		trfc = auto_cal_timing_standby(210, ctrl_freq);
	}
	mctl_write_w((para->dram_mr0) & 0xffff, DRAM_MR0);
	mctl_write_w((para->dram_mr1) & 0xffff, DRAM_MR1);
	mctl_write_w((para->dram_mr2) & 0xffff, DRAM_MR2);
	mctl_write_w((para->dram_mr3) & 0xffff, DRAM_MR3);
	mctl_write_w(((para->dram_odt_en) >> 4) & 0x3, LP3MR11);
	mctl_write_w(reg[2], DRAMTMG0);
	mctl_write_w(reg[3], DRAMTMG1);
	mctl_write_w(reg[4], DRAMTMG2);
	mctl_write_w(reg[5], DRAMTMG3);
	mctl_write_w(reg[6], DRAMTMG4);
	mctl_write_w(reg[7], DRAMTMG5);
	mctl_write_w(reg[8], DRAMTMG8);
	mctl_write_w(reg[9], PITMG0);
	mctl_write_w(reg[10], PTR3);
	mctl_write_w(reg[11], PTR4);
	reg_val = 0;
	reg_val = (trefi << 16) | (trfc << 0);
	mctl_write_w(reg_val, RFSHTMG);
}

/*****************************************************************************
Function : DRAM CLK Spread Spectrum Function
parameter : (DRAM parameter)
return value : 1
*****************************************************************************/
unsigned int ccm_set_pll_ddr1_sscg_standby(__dram_para_t *para)
{
	unsigned int pll_ssc, pll_step;
	unsigned int ret, reg_val;
	/*计算PLL_STEP */
	ret = (para->dram_tpr13 >> 16) & 0xf;
	if (ret < 12) {
		pll_step = ret;
	} else {
		pll_step = 9;
	}
	/*计算PLL_SSC */
	ret = (para->dram_tpr13 >> 20) & 0x7;
	if (ret < 6) {
		if (ret == 0) {
			if ((para->dram_tpr13 >> 10) & 0x1) {
				pll_ssc = (1 << pll_step);
			} else {
				return 1;
			}
		} else {
			pll_ssc = (ret << 18) / 10 - (1 << pll_step);
			pll_ssc = (pll_ssc / (1 << pll_step));
			pll_ssc = (pll_ssc * (1 << pll_step));
		}
	} else {
		pll_ssc = (8 << 17) / 10 - (1 << pll_step);
		pll_ssc = (pll_ssc / (1 << pll_step));
		pll_ssc = (pll_ssc * (1 << pll_step));
	}
	/*update PLL前切到自身24M时钟源 */
	reg_val = mctl_read_w(_CCM_PLL_DDR_AUX_REG);
	reg_val |= (0x1U << 4);
	mctl_write_w(reg_val, _CCM_PLL_DDR_AUX_REG);
	reg_val |= (0x1U << 1);
	mctl_write_w(reg_val, _CCM_PLL_DDR_AUX_REG);
	/*配置PLL_SSC与PLL_STEP */
	reg_val = mctl_read_w(_CCM_PLL_DDR1_CFG_REG);
	reg_val &= ~(0x1ffff << 12);
	reg_val &= ~(0xf << 0);
	reg_val |= (pll_ssc << 12);
	reg_val |= (pll_step << 0);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_CFG_REG);
	/*Update PLL设置 */
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val |= (0x1U << 30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	dram_udelay(20);
	/*使能线性调频 */
	reg_val = mctl_read_w(_CCM_PLL_DDR1_CFG_REG);
	reg_val |= (0x1U << 31);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_CFG_REG);
	/*Update PLL设置 */
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val |= (0x1U << 30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	while ((mctl_read_w(_CCM_PLL_DDR1_REG) >> 30) & 0x1)
		;
	dram_udelay(20);
	return 1;
}

/*****************************************************************************
Function : DRAM CLK Spread Spectrum Function
parameter : (DRAM parameter)
return value : 1
*****************************************************************************/
unsigned int ccm_set_pll_ddr0_sscg_standby(__dram_para_t *para)
{
	unsigned int ret, reg_val;
	/*计算WAVE_BOT */
	ret = (para->dram_tpr13 >> 20) & 0x7;
#if 0
	switch (ret) {
	case 0:
		return 1;
	case 1:
		mctl_write_w((0xccccU | (0x3U << 17) | (0x48U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
		break;
	case 2:
		mctl_write_w((0x9999U | (0x3U << 17) | (0x90U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
		break;
	case 3:
		mctl_write_w((0x6666U | (0x3U << 17) | (0xD8U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
		break;
	case 4:
		mctl_write_w((0x3333U | (0x3 << 17) | (0x120U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
		break;
	case 5:
		mctl_write_w(((0x3U << 17) | (0x158U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
		break;
	default:
		mctl_write_w((0x3333U | (0x3 << 17) | (0x120U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
		break;
	}
#else
	if (0 == ret) {
		return 1;
	} else if (1 == ret) {
		mctl_write_w((0xccccU | (0x3U << 17) | (0x48U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
	} else if (2 == ret) {
		mctl_write_w((0x9999U | (0x3U << 17) | (0x90U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
	} else if (3 == ret) {
		mctl_write_w((0x6666U | (0x3U << 17) | (0xD8U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
	} else if (4 == ret) {
		mctl_write_w((0x3333U | (0x3 << 17) | (0x120U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
	} else if (5 == ret) {
		mctl_write_w(((0x3U << 17) | (0x158U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
	} else {
		mctl_write_w((0x3333U | (0x3 << 17) | (0x120U << 20) | (0x3U << 29) | (0x1U << 31)), PLL_DDR0_PAT_CTL_REG);
	}
#endif
	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);	/*enable sscg*/
	reg_val |= (0x1U << 24);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);

	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);	/*updata*/
	reg_val |= (0x1U << 20);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	while (mctl_read_w(_CCM_PLL_DDR0_REG) & (0x1 << 30))
		;
	dram_udelay(20);
	return 1;
}

/*****************************************************************************
Function : DRAM CLK Configuration
parameter : (DRAM parameter)
return value : PLL Frequency
*****************************************************************************/
unsigned int ccm_set_pll_ddr0_clk_standby(__dram_para_t *para)
{
	unsigned int n, k, m = 1, rval, pll_clk;
	unsigned int div;
	unsigned int mod2, mod3;
	unsigned int min_mod = 0;

	/*启用PLL LOCK功能 */
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 4);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);

	if ((para->dram_tpr13 >> 6) & 0x1)
		pll_clk = (para->dram_tpr9 << 1);
	else
		pll_clk = (para->dram_clk << 1);

	div = pll_clk / 24;
	if (div <= 32) {
		n = div;
		k = 1;
	} else {
		/* when m=1, we cann't get absolutely accurate value for follow clock:
		 * 840(816), 888(864),
		 * 984(960)
		 */
		mod2 = div & 1;
		mod3 = div % 3;
		min_mod = mod2;
		k = 2;
		if (min_mod > mod3) {
			min_mod = mod3;
			k = 3;
		}
		n = div / k;
	}
	rval = mctl_read_w(_CCM_PLL_DDR0_REG);
	rval &= ~((0x1f << 8) | (0x3 << 4) | (0x3 << 0));
	rval = (1U << 31) | ((n - 1) << 8) | ((k - 1) << 4) | (m - 1);
	mctl_write_w(rval, _CCM_PLL_DDR0_REG);
	mctl_write_w(rval | (1U << 20), _CCM_PLL_DDR0_REG);
	/*使能PLL_DDR1的硬件LOCK功能 */
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval |= (0x1 << 4);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	while (!((mctl_read_w(_CCM_PLL_DDR0_REG) & (1 << 20 | 1 << 28)) == (0 << 20 | 1 << 28)))
		;
	/*清除设置值 */
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 4);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);

	dram_udelay(20);
	rval = ccm_set_pll_ddr0_sscg_standby(para);
	return 24 * n * k / m;
}

/*****************************************************************************
Function : DRAM CLK Configuration
parameter : (DRAM parameter)
return value : PLL Frequency
*****************************************************************************/
unsigned int ccm_set_pll_ddr1_clk_standby(__dram_para_t *para)
{
	unsigned int rval;
	unsigned int div;
	/*启用PLL LOCK功能 */
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 11);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	/*计算除频因子 */
	if ((para->dram_tpr13 >> 6) & 0x1)
		div = para->dram_clk / 12;
	else
		div = para->dram_tpr9 / 12;
	if (div < 12)
		div = 12;
	/*配置除频因子并使能PLL */
	rval = mctl_read_w(_CCM_PLL_DDR1_REG);
	rval &= ~(0x7f << 8);
	rval |= (((div - 1) << 8) | (0x1U << 31));
	mctl_write_w(rval, _CCM_PLL_DDR1_REG);
	/*更新PLL配置 */
	mctl_write_w(rval | (1U << 30), _CCM_PLL_DDR1_REG);
	/*使能PLL_DDR1的硬件LOCK功能 */
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval |= (0x1 << 11);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	while (!((mctl_read_w(_CCM_PLL_DDR1_REG) & (1 << 30 | 1 << 28)) == (0 << 30 | 1 << 28)))
		;
	/*清除设置值 */
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 11);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	dram_udelay(100);
	/*设置展频 */
	rval = ccm_set_pll_ddr1_sscg_standby(para);
	return 24 * div;
}

/*****************************************************************************
Function : System resource initialization
parameter : (DRAM parameter)
return value : 1(Meaningless)
*****************************************************************************/
unsigned int mctl_sys_init_standby(__dram_para_t *para)
{
	unsigned int reg_val = 0;
	unsigned int ret_val = 0;
	/*Turn Off MBUS CLK */
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);
	/*MBUS Domain Reset */
	reg_val = mctl_read_w(MBUS_RESET_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, MBUS_RESET_REG);
	/*Turn Off AHB Domain CLK */
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val &= ~(1U << 14);
	mctl_write_w(reg_val, BUS_CLK_GATE_REG0);
	/*AHB Domain Reset */
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val &= ~(1U << 14);
	mctl_write_w(reg_val, BUS_RST_REG0);
	/*Turn Off PLL_DDR0 */
	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	reg_val |= (0x1U << 20);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	/*Turn Off PLL_DDR1 */
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val &= ~(1U << 31);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	reg_val |= (0x1U << 30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	/*DRAM Controller Reset */
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1U << 31);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);
	dram_udelay(10);
	/*Set DRAM PLL Frequency */
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x3 << 20);
	if (para->dram_tpr13 >> 6 & 0x1) {
		/*Choose PLL_DDR1 */
		reg_val |= 0x1 << 20;
		ret_val = ccm_set_pll_ddr1_clk_standby(para);
		para->dram_clk = ret_val / 2;
		dram_dbg_4("pll_ddr1 = %d MHz\n", ret_val);
		/*Enable another PLL */
		if (para->dram_tpr9 != 0) {
			ret_val = ccm_set_pll_ddr0_clk_standby(para);
			para->dram_tpr9 = ret_val / 2;
			dram_dbg_4("pll_ddr0 = %d MHz\n", ret_val);
		}

	} else {
		/*Choose PLL_DDR0 */
		reg_val |= 0x0 << 20;
		ret_val = ccm_set_pll_ddr0_clk_standby(para);
		para->dram_clk = ret_val / 2;
		dram_dbg_4("pll_ddr0 = %d MHz\n", ret_val);
		/*Enable another PLL */
		if (para->dram_tpr9 != 0) {
			ret_val = ccm_set_pll_ddr1_clk_standby(para);
			para->dram_tpr9 = ret_val / 2;
			dram_dbg_4("pll_ddr1 = %d MHz\n", ret_val);
		}
	}
	dram_udelay(1000);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);
	dram_udelay(10);
	reg_val |= (0x1 << 16);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);
	/*Release AHB Domain Reset */
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val |= (1U << 14);
	mctl_write_w(reg_val, BUS_RST_REG0);
	/*Turn On AHB Domain CLK */
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val |= (1U << 14);
	mctl_write_w(reg_val, BUS_CLK_GATE_REG0);
	/*Disable Master Access Right */
	mctl_write_w(0x1, MC_MAER);
	/*Release MBUS Domain Reset */
	reg_val = mctl_read_w(MBUS_RESET_REG);
	reg_val |= (1U << 31);
	mctl_write_w(reg_val, MBUS_RESET_REG);
	/*Turn On MBUS Domain CLK */
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val |= (1U << 31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);
	/*Release DRAM Controller Reset */
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val |= (0x1U << 31);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);
	dram_udelay(10);
	/*Enable DRAM Controller CLK */
	mctl_write_w(0x8000, CLKEN);
	dram_udelay(10);
	return DRAM_RET_OK;
}

/*****************************************************************************
Function : DRAM Controller Configuration
parameter : (DRAM parameter)
return value : void
*****************************************************************************/
void mctl_com_init_standby(__dram_para_t *para)
{
	unsigned int reg_val, ret_val;
	mctl_write_w(reg[0], MC_WORK_MODE);
	mctl_write_w(reg[1], MC_R1_WORK_MODE);
	/*ODT MAP */
	reg_val = (mctl_read_w(MC_WORK_MODE) & 0x1);
	if (reg_val)
		ret_val = 0x303;
	else
		ret_val = 0x201;
	mctl_write_w(ret_val, ODTMAP);
	/*half DQ mode */
	if (para->dram_para2 & 0x1) {
		mctl_write_w(0, DXnGCR0(2));
		mctl_write_w(0, DXnGCR0(3));
	}
}

/*****************************************************************************
Function : DRAM Controller Basic Initialization
parameter : (0,DRAM parameter)
return value : 0-FAIL  , other-Success
*****************************************************************************/
unsigned int mctl_channel_init_standby(unsigned int ch_index, __dram_para_t *para)
{
	unsigned int reg_val = 0, ret_val = 0;
	unsigned int dqs_gating_mode = 0;
	unsigned int i = 0;
	unsigned int rval = 1;
	dqs_gating_mode = (para->dram_tpr13 >> 2) & 0x3;
	dram_dbg_0("mctl_channel_init_standby\n");
/***********************************
Function : Set Phase
 **********************************/
	reg_val = mctl_read_w(PGCR2);
	reg_val &= ~(0x3 << 10);
	reg_val |= 0x0 << 10;
	reg_val &= ~(0x3 << 8);
	reg_val |= 0x3 << 8;
	mctl_write_w(reg_val, PGCR2);
	dram_dbg_8("PGCR2 is %x\n", reg_val);
/***********************************
Function : AC/DX IO Configuration
 **********************************/
	ret_val = para->dram_odt_en & 0x1;
	dram_dbg_8("DRAMC read ODT type : %d (0: off  1: dynamic on).\n", ret_val);
	ret_val = ~(para->dram_odt_en) & 0x1;
	for (i = 0; i < 4; i++) {
		/*byte 0/byte 1/byte 3/byte 4 */
		reg_val = mctl_read_w(DXnGCR0(i));
		reg_val &= ~(0x3U << 4);
		reg_val |= (ret_val << 5);	/* ODT:2b'00 dynamic ,2b'10 off */
		reg_val &= ~(0x1U << 1);	/* SSTL IO mode */
		reg_val &= ~(0x3U << 2);	/*OE mode: 0 Dynamic */
		reg_val &= ~(0x3U << 12);	/*Power Down Receiver: Dynamic */
		reg_val &= ~(0x3U << 14);	/*Power Down Driver: Dynamic */
		if (para->dram_clk > 672) {
			reg_val &= ~(0x3U << 9);
			reg_val |= (0x2U << 9);
		}
		mctl_write_w(reg_val, DXnGCR0(i));
	}
	dram_dbg_8("DXnGCR0 = %x\n", reg_val);

	reg_val = mctl_read_w(ACIOCR0);
	reg_val |= (0x1 << 1);
	reg_val &= ~(0x1 << 11);
	mctl_write_w(reg_val, ACIOCR0);
/***********************************
Function : AC/DX IO Bit Delay
 **********************************/
	bit_delay_compensation_standby(para);
/***********************************
Function : DQS Gate Mode Choose
 **********************************/
	switch (dqs_gating_mode) {
	case 1:		/*open DQS gating */
		reg_val = mctl_read_w(DQSGMR);
		reg_val &= ~((0x1 << 8) | 0x7);
		mctl_write_w(reg_val, DQSGMR);
		dram_dbg_8("DRAM DQS gate is open.\n");

		break;
	case 2:		/*auto gating pull up */
		reg_val = mctl_read_w(PGCR2);
		reg_val &= (~(0x1 << 6));
		mctl_write_w(reg_val, PGCR2);
		dram_udelay(10);

		reg_val = mctl_read_w(PGCR2);
		reg_val &= ~(0x3 << 6);
		reg_val |= (0x2 << 6);
		mctl_write_w(reg_val, PGCR2);

		ret_val = ((mctl_read_w(DRAMTMG2) >> 16) & 0x1f) - 2;
		reg_val = mctl_read_w(DQSGMR);
		reg_val &= ~((0x1 << 8) | (0x7));
		reg_val |= ((0x1 << 8) | (ret_val));
		mctl_write_w(reg_val, DQSGMR);

		reg_val = mctl_read_w(DXCCR);	/*dqs pll up*/
		reg_val |= (0x1 << 27);
		reg_val &= ~(0x1U << 31);
		mctl_write_w(reg_val, DXCCR);
		dram_dbg_8("DRAM DQS gate is PU mode.\n");
		break;
	default:
		/*close DQS gating--auto gating pull down*/
		/*for aw1680 standby problem,reset gate*/
		reg_val = mctl_read_w(PGCR2);
		reg_val &= ~(0x1 << 6);
		mctl_write_w(reg_val, PGCR2);
		dram_udelay(10);

		reg_val = mctl_read_w(PGCR2);
		reg_val |= (0x3 << 6);
		mctl_write_w(reg_val, PGCR2);
		dram_dbg_8("DRAM DQS gate is PD mode.\n");
		break;
	}
/***********************************
Function : Pull Up/Down Strength
 **********************************/
	if ((para->dram_type == 6) || (para->dram_type == 7)) {
		reg_val = mctl_read_w(DXCCR);
		reg_val &= ~(0x7U << 28);
		reg_val &= ~(0x7U << 24);
		reg_val |= (0x2U << 28);
		reg_val |= (0x2U << 24);
		mctl_write_w(reg_val, DXCCR);
	}
/***********************************
Function : Training
 **********************************/
	if ((para->dram_para2 >> 12) & 0x1) {
		reg_val = mctl_read_w(DTCR);
		reg_val &= (0xfU << 28);
		reg_val |= 0x03003087;
		mctl_write_w(reg_val, DTCR);	/*two rank */
	} else {
		reg_val = mctl_read_w(DTCR);
		reg_val &= (0xfU << 28);
		reg_val |= 0x01003087;
		mctl_write_w(reg_val, DTCR);	/*one rank */
	}
	/* ZQ pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= (~(0x1 << 1));
	mctl_write_w(reg_val, VDD_SYS_PWROFF_GATING);
	dram_udelay(10);
	/* ZQ calibration */
	reg_val = mctl_read_w(ZQCR);
	reg_val &= ~(0x00ffffff);
	reg_val |= ((para->dram_zq) & 0xffffff);
	mctl_write_w(reg_val, ZQCR);
	if (dqs_gating_mode == 1) {
		reg_val = 0x52;
		mctl_write_w(reg_val, PIR);
		reg_val |= (0x1 << 0);
		mctl_write_w(reg_val, PIR);
		while ((mctl_read_w(PGSR0) & 0x1) != 0x1)
			;
		dram_udelay(10);
		reg_val = 0x20;	/*DDL CAL; */
	} else {
		reg_val = 0x62;
	}
	mctl_write_w(reg_val, PIR);
	reg_val |= (0x1 << 0);
	mctl_write_w(reg_val, PIR);
	dram_udelay(10);
	while ((mctl_read_w(PGSR0) & 0x1) != 0x1)
		;

	reg_val = mctl_read_w(PGCR3);
	reg_val &= (~(0x3 << 25));
	reg_val |= (0x2 << 25);
	mctl_write_w(reg_val, PGCR3);
	dram_udelay(10);
	/* entry self-refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1 << 0;
	mctl_write_w(reg_val, PWRCTL);
	while (((mctl_read_w(STATR) & 0x7) != 0x3))
		;

	/* pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= ~(0x1 << 0);
	mctl_write_w(reg_val, VDD_SYS_PWROFF_GATING);
	dram_udelay(10);

	/* exit self-refresh but no enable all master access */
	reg_val = mctl_read_w(PWRCTL);
	reg_val &= ~(0x1 << 0);
	mctl_write_w(reg_val, PWRCTL);
	while (((mctl_read_w(STATR) & 0x7) != 0x1))
		;
	dram_udelay(15);

	/* training :DQS gate training */
	if (dqs_gating_mode == 1) {
		reg_val = mctl_read_w(PGCR2);
		reg_val &= ~(0x3 << 6);
		mctl_write_w(reg_val, PGCR2);

		reg_val = mctl_read_w(PGCR3);
		reg_val &= (~(0x3 << 25));
		reg_val |= (0x1 << 25);
		mctl_write_w(reg_val, PGCR3);
		dram_udelay(1);

		reg_val = 0x401;
		mctl_write_w(reg_val, PIR);
		while ((mctl_read_w(PGSR0) & 0x1) != 0x1)
			;
	}

/***********************************
Function : Training Information
 **********************************/
	reg_val = mctl_read_w(PGSR0);
	if ((reg_val >> 20) & 0xff) {
		/* training ERROR information */
		dram_dbg_4("[DEBUG_4]PGSR0 = 0x%x\n", reg_val);
		if ((reg_val >> 20) & 0x1) {
			dram_dbg_1("ZQ calibration error, check external 240 ohm resistor.\n");
		}
		rval = 0;
	}
/***********************************
Function : Controller Setting
**********************************/
	/*after initial done */
	while ((mctl_read_w(STATR) & 0x1) != 0x1)
		;
	/*refresh update, from AW1680/1681 */
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val |= (0x1U) << 31;
	mctl_write_w(reg_val, RFSHCTL0);
	dram_udelay(10);
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val &= ~(0x1U << 31);
	mctl_write_w(reg_val, RFSHCTL0);
	dram_udelay(10);

	/*after initial before write or read must clear credit value */
	reg_val = mctl_read_w(MC_CCCR);
	reg_val |= (0x1U) << 31;
	mctl_write_w(reg_val, MC_CCCR);
	dram_udelay(10);
	/*PHY choose to update PHY or command mode */
	reg_val = mctl_read_w(PGCR3);
	reg_val &= ~(0x3 << 25);
	mctl_write_w(reg_val, PGCR3);
/***********************************
Function : DQS Gate Optimization
**********************************/
	if ((para->dram_type) == 6 || (para->dram_type) == 7) {
		if (dqs_gating_mode == 1) {
			reg_val = mctl_read_w(DXCCR);
			reg_val &= ~(0x3 << 6);
			reg_val |= (0x1 << 6);
			mctl_write_w(reg_val, DXCCR);
		}
	}
	dram_dbg_0("mctl_channel_init_standby_out\n");
	return rval;
}

/*****************************************************************************
Function : DRAM Controller Basic Initialization
parameter : (DRAM parameter)
return value : 0-FAIL  , other-Success
*****************************************************************************/
unsigned int mctl_core_init_standby(__dram_para_t *para)
{
	unsigned int ret_val = 0;
	mctl_sys_init_standby(para);
	mctl_com_init_standby(para);
	auto_set_timing_para_standby(para);
	ret_val = mctl_channel_init_standby(0, para);
	return ret_val;
}

/*****************************************************************************
Function : DRAM Initialization Function Entry
parameter : (Meaningless,DRAM parameter)
return value : 0-FAIL  , other-DRAM size
*****************************************************************************/
signed int init_DRAM_standby(int type, __dram_para_t *para)
{
	unsigned int ret_val = 0;
	unsigned int reg_val = 0;
	unsigned int pad_hold = 0;
	unsigned int dram_size = 0;

	dram_dbg_0("DRAM NORM STANDBY DRIVE INFO: V0.5\n");
/*****************************************************************************
Function : DRAM Controller Basic Initialization
*****************************************************************************/
	dram_dbg_0("DRAM CLK =%d MHZ\n", para->dram_clk);
	dram_dbg_0("DRAM Type =%d (2:DDR2, 3:DDR3, 6:LPDDR2, 7:LPDDR3)\n", para->dram_type);
	dram_dbg_0("DRAM zq value: 0x%x\n", para->dram_zq);
	ret_val = mctl_core_init_standby(para);
	if (ret_val == 0) {
		dram_dbg_0("DRAM initial error : 1 !\n");
		return 0;
	}
/*****************************************************************************
Function : DRAM size
*****************************************************************************/
	dram_size = (para->dram_para2 >> 16) & 0x7fff;
/*****************************************************************************
Function : Power Related
1.HDR/DDR CLK dynamic
2.Close ZQ calibration module
*****************************************************************************/
	if (((para->dram_tpr13 >> 9) & 0x1) || (para->dram_type == 6)) {
		reg_val = mctl_read_w(PGCR0);
		reg_val &= ~(0xf << 12);
		reg_val |= (0x5 << 12);
		mctl_write_w(reg_val, PGCR0);
		dram_dbg_8("HDR\DDR always on mode!\n");
	} else {
		reg_val = mctl_read_w(PGCR0);
		reg_val &= ~(0xf << 12);
		mctl_write_w(reg_val, PGCR0);
		dram_dbg_8("HDR\DDR dynamic mode!\n");
	}
	reg_val = mctl_read_w(ZQCR);
	reg_val |= (0x1U << 31);
	mctl_write_w(reg_val, ZQCR);
/*****************************************************************************
Function : Performance Optimization
1.VTF Function
2.PAD HOLD Function
3.LPDDR3 ODT delay
*****************************************************************************/
	if ((para->dram_tpr13 >> 8) & 0x1) {
		reg_val = mctl_read_w(VTFCR);
		reg_val |= (0x1 << 8);
		reg_val |= (0x1 << 9);
		mctl_write_w(reg_val, VTFCR);
		dram_dbg_0("VTF enable\n");
	}
	if ((para->dram_tpr13 >> 26) & 0x1) {
		reg_val = mctl_read_w(PGCR2);
		reg_val &= ~(0x1 << 13);
		mctl_write_w(reg_val, PGCR2);
		dram_dbg_0("DQ hold disable!\n");
	} else {
		reg_val = mctl_read_w(PGCR2);
		reg_val |= (0x1 << 13);
		mctl_write_w(reg_val, PGCR2);
		dram_dbg_0("DQ hold enable!\n");
	}
	if (para->dram_type == 7) {
		reg_val = mctl_read_w(ODTCFG);
		reg_val &= ~(0xf << 16);
		reg_val |= (0x1 << 16);
		mctl_write_w(reg_val, ODTCFG);
	}
/*****************************************************************************
Function : Priority Setting Module
*****************************************************************************/
	if (!((para->dram_tpr13 >> 27) & 0x1)) {
		set_master_priority_standby();
	}
/*****************************************************************************
Function End
*****************************************************************************/
	return dram_size;
}

/*****************************************************************************
Function : DRAM Standby Function Exit
parameter : (DRAM parameter)
return value : 0-FAIL  , other-DRAM size
*****************************************************************************/
static unsigned int __dram_power_up_process(__dram_para_t *para)
{
	unsigned int ret = 0;
	ret = init_DRAM_standby(0, para);
	return ret;
}

unsigned int dram_power_save_process(void)
{
	__dram_power_save_process();
	return 0;
}

unsigned int dram_power_up_process(__dram_para_t *para)
{
	unsigned int ret = 0;
	ret = __dram_power_up_process(para);

	return ret;
}
#else
/********************************************************************************
 *FPGA standby code
 ********************************************************************************/
static unsigned int __dram_power_save_process(void)
{
	unsigned int reg_val = 0;

	mctl_write_w(0, MC_MAER);
	/*1.enter self refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1 << 0;
	reg_val |= (0x1 << 8);
	mctl_write_w(reg_val, PWRCTL);
	/*confirm dram controller has enter selfrefresh */
	while (((mctl_read_w(STATR) & 0x7) != 0x3))
		;

	/* 3.pad hold */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val |= 0x3 << 0;
	mctl_write_w(reg_val, VDD_SYS_PWROFF_GATING);

	return 0;
}

static unsigned int __dram_power_up_process(__dram_para_t *para)
{
	unsigned int reg_val = 0;
	/* 1.pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= ~(0x3 << 0);
	mctl_write_w(reg_val, VDD_SYS_PWROFF_GATING);
	/*2.exit self refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val &= ~(0x1 << 0);
	reg_val &= ~(0x1 << 8);
	mctl_write_w(reg_val, PWRCTL);
	/*confirm dram controller has enter selfrefresh */
	while (((mctl_read_w(STATR) & 0x7) != 0x1))
		;
	/*3.enable master access */
	mctl_write_w(0xffffffff, MC_MAER);
	return 0;
}

unsigned int dram_power_save_process(void)
{
	__dram_power_save_process();
	return 0;
}

unsigned int dram_power_up_process(__dram_para_t *para)
{
	__dram_power_up_process(para);

	return 0;
}
#endif

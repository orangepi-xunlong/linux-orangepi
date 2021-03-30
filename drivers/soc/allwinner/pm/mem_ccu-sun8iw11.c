/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm_types.h"
#include "pm_i.h"
/*
*********************************************************************************************************
*                                       MEM CCU INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_ccu_save(struct ccm_state *ccm_reg)
{
	ccm_reg->ccm_reg = (__ccmu_reg_list_t *) IO_ADDRESS(AW_CCM_BASE);
#if 0
	/*bias: */
	ccm_reg->ccm_reg_backup.PllAudioBias = ccm_reg->ccm_reg->PllAudioBias;
	ccm_reg->ccm_reg_backup.PllDeBias = ccm_reg->ccm_reg->PllDeBias;
	ccm_reg->ccm_reg_backup.PllVideo0Bias = ccm_reg->ccm_reg->PllVideo0Bias;
	ccm_reg->ccm_reg_backup.PllVideo1Bias = ccm_reg->ccm_reg->PllVideo1Bias;

	/*tune & pattern: */
	ccm_reg->ccm_reg_backup.PllAudioPattern =
	    ccm_reg->ccm_reg->PllAudioPattern;

	/*cfg: */
	ccm_reg->ccm_reg_backup.Pll1Ctl = ccm_reg->ccm_reg->Pll1Ctl;
	ccm_reg->ccm_reg_backup.Pll2Ctl = ccm_reg->ccm_reg->Pll2Ctl;
	ccm_reg->ccm_reg_backup.Pll7Ctl = ccm_reg->ccm_reg->Pll7Ctl;
	ccm_reg->ccm_reg_backup.Pll10Ctl = ccm_reg->ccm_reg->Pll10Ctl;

	/*notice: be care to cpu, axi restore sequence!. */
	ccm_reg->ccm_reg_backup.SysClkDiv = ccm_reg->ccm_reg->SysClkDiv;
	ccm_reg->ccm_reg_backup.Ahb1Div = ccm_reg->ccm_reg->Ahb1Div;
	ccm_reg->ccm_reg_backup.Apb2Div = ccm_reg->ccm_reg->Apb2Div;

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	ccm_reg->ccm_reg_backup.AhbGate0 = ccm_reg->ccm_reg->AhbGate0;
	ccm_reg->ccm_reg_backup.AhbGate1 = ccm_reg->ccm_reg->AhbGate1;
	ccm_reg->ccm_reg_backup.Apb1Gate = ccm_reg->ccm_reg->Apb1Gate;
	ccm_reg->ccm_reg_backup.Apb2Gate0 = ccm_reg->ccm_reg->Apb2Gate0;
	ccm_reg->ccm_reg_backup.Apb2Gate1 = ccm_reg->ccm_reg->Apb2Gate1;

	ccm_reg->ccm_reg_backup.Nand0 = ccm_reg->ccm_reg->Nand0;

	ccm_reg->ccm_reg_backup.Sd0 = ccm_reg->ccm_reg->Sd0;
	ccm_reg->ccm_reg_backup.Sd1 = ccm_reg->ccm_reg->Sd1;
	ccm_reg->ccm_reg_backup.Sd2 = ccm_reg->ccm_reg->Sd2;
	ccm_reg->ccm_reg_backup.Sd3 = ccm_reg->ccm_reg->Sd3;

	ccm_reg->ccm_reg_backup.Spi0 = ccm_reg->ccm_reg->Spi0;
	ccm_reg->ccm_reg_backup.Spi1 = ccm_reg->ccm_reg->Spi1;
	ccm_reg->ccm_reg_backup.Spi2 = ccm_reg->ccm_reg->Spi2;
	ccm_reg->ccm_reg_backup.Spi3 = ccm_reg->ccm_reg->Spi2;

	ccm_reg->ccm_reg_backup.I2s0 = ccm_reg->ccm_reg->I2s0;
	ccm_reg->ccm_reg_backup.I2s1 = ccm_reg->ccm_reg->I2s1;
	ccm_reg->ccm_reg_backup.I2s2 = ccm_reg->ccm_reg->I2s2;

	ccm_reg->ccm_reg_backup.SpdifClk = ccm_reg->ccm_reg->SpdifClk;
	ccm_reg->ccm_reg_backup.UsbClk = ccm_reg->ccm_reg->UsbClk;

	/* do not touch dram related config. */
	/*  ccm_reg->ccm_reg_backup.PllDdrCfg   = ccm_reg->ccm_reg->PllDdrCfg; */
	/*  ccm_reg->ccm_reg_backup.MbusResetReg        = ccm_reg->ccm_reg->MbusResetReg; */
	/*  ccm_reg->ccm_reg_backup.DramCfg             = ccm_reg->ccm_reg->DramCfg; */
	/*  ccm_reg->ccm_reg_backup.DramGate            = ccm_reg->ccm_reg->DramGate; */

	ccm_reg->ccm_reg_backup.DeClk = ccm_reg->ccm_reg->DeClk;
	ccm_reg->ccm_reg_backup.DeMpClk = ccm_reg->ccm_reg->DeMpClk;
	ccm_reg->ccm_reg_backup.Lcd0Ch0 = ccm_reg->ccm_reg->Lcd0Ch0;

	ccm_reg->ccm_reg_backup.CsiMisc = ccm_reg->ccm_reg->CsiMisc;
	ccm_reg->ccm_reg_backup.Csi0 = ccm_reg->ccm_reg->Csi0;

	/*mbus? function? */
	/*ccm_reg->ccm_reg_backup.MBus0         = ccm_reg->ccm_reg->MBus0; */

	ccm_reg->ccm_reg_backup.AhbReset0 = ccm_reg->ccm_reg->AhbReset0;
	ccm_reg->ccm_reg_backup.AhbReset1 = ccm_reg->ccm_reg->AhbReset1;
	ccm_reg->ccm_reg_backup.AhbReset2 = ccm_reg->ccm_reg->AhbReset2;
	ccm_reg->ccm_reg_backup.Apb1Reset = ccm_reg->ccm_reg->Apb1Reset;
	ccm_reg->ccm_reg_backup.Apb2Reset = ccm_reg->ccm_reg->Apb2Reset;

	ccm_reg->ccm_reg_backup.PsCtrl = ccm_reg->ccm_reg->PsCtrl;
	ccm_reg->ccm_reg_backup.PsCnt = ccm_reg->ccm_reg->PsCnt;
	ccm_reg->ccm_reg_backup.PllLockCtrl = ccm_reg->ccm_reg->PllLockCtrl;

#else
	/*save all the ccmu reg*/
	memcpy(&(ccm_reg->ccm_reg_backup), ccm_reg->ccm_reg, sizeof(__ccmu_reg_list_t));
#endif
	return 0;
}

__s32 mem_ccu_restore(struct ccm_state *ccm_reg)
{
	/*restore some ccmu reg when need*/
	/*bias: */
	ccm_reg->ccm_reg->PllSataBias = ccm_reg->ccm_reg_backup.PllSataBias; /*0x218*/
	/*ccm_reg->ccm_reg->PllPeriph1Bias = ccm_reg->ccm_reg_backup.PllPeriph1Bias;*/ /*0x21c, periph1 hsic  bias reg */
	/*ccm_reg->ccm_reg->PllxBias[0] = ccm_reg->ccm_reg_backup.PllxBias[0];*/ /*0x220,  pll cpux  bias reg */
	ccm_reg->ccm_reg->PllAudioBias = ccm_reg->ccm_reg_backup.PllAudioBias; /*0x224,  pll audio bias reg */
	ccm_reg->ccm_reg->PllVideo0Bias = ccm_reg->ccm_reg_backup.PllVideo0Bias; /*0x228,  pll vedio bias reg */
	ccm_reg->ccm_reg->PllVeBias = ccm_reg->ccm_reg_backup.PllVeBias; /*0x22c,  pll ve    bias reg */
	/*ccm_reg->ccm_reg->PllDram0Bias = ccm_reg->ccm_reg_backup.PllDram0Bias;*/ /*0x230, pll dram0 bias reg */
	/*ccm_reg->ccm_reg->PllPeriph0Bias = ccm_reg->ccm_reg_backup.PllPeriph0Bias;*/ /*0x234, pll periph0 bias reg */
	ccm_reg->ccm_reg->PllVideo1Bias = ccm_reg->ccm_reg_backup.PllVideo1Bias; /*0x238, pll video1 bias reg */
	ccm_reg->ccm_reg->PllGpuBias = ccm_reg->ccm_reg_backup.PllGpuBias; /*0x23c, pll gpu bias reg */
	ccm_reg->ccm_reg->PllDeBias = ccm_reg->ccm_reg_backup.PllDeBias; /*0x248,  pll de bias reg */
	/*ccm_reg->ccm_reg->PllDram1BiasReg = ccm_reg->ccm_reg_backup.PllDram1BiasReg;*/ /*0x24c,  pll dram1 bias */

	/*tune & pattern: */
	ccm_reg->ccm_reg->Pll1Tun = ccm_reg->ccm_reg_backup.Pll1Tun;  /*0x250, pll1 tun,cpux tuning reg */
	/*ccm_reg->ccm_reg->PllDdr0Tun = ccm_reg->ccm_reg_backup.PllDdr0Tun;*/ /*0x260, pll ddr0 tuning */
	ccm_reg->ccm_reg->pllMipiTun = ccm_reg->ccm_reg_backup.pllMipiTun; /*0x70, mipi tuning reg*/
	/*ccm_reg->ccm_reg->PllPeriph1Pattern = ccm_reg->ccm_reg_backup.PllPeriph1Pattern;*/ /*0x27c, pll pre1ph1 pattern control reg */
	/*ccm_reg->ccm_reg->Pll1Pattern = ccm_reg->ccm_reg_backup.Pll1Pattern;*/ /*0x280,  pll cpux  pattern reg */
	ccm_reg->ccm_reg->PllAudioPattern = ccm_reg->ccm_reg_backup.PllAudioPattern; /*0x284,  pll audio pattern reg */
	ccm_reg->ccm_reg->PllVedio0Pattern = ccm_reg->ccm_reg_backup.PllVedio0Pattern; /*0x288, pll vedio pattern reg */
	ccm_reg->ccm_reg->PllVePattern = ccm_reg->ccm_reg_backup.PllVePattern; /*0x28c, pll ve pattern reg */
	/*ccm_reg->ccm_reg->PllDdr0Pattern = ccm_reg->ccm_reg_backup.PllDdr0Pattern;*/ /*0x290, pll ddr0 pattern reg */
	ccm_reg->ccm_reg->PllVedio1Pattern = ccm_reg->ccm_reg_backup.PllVedio1Pattern; /*0x298, pll video1 pattern reg */
	ccm_reg->ccm_reg->PllGpuPattern = ccm_reg->ccm_reg_backup.PllGpuPattern; /*0x29c, pll gpu pattern reg */
	ccm_reg->ccm_reg->PllDePattern = ccm_reg->ccm_reg_backup.PllDePattern; /*0x2a8, pll de pattern reg */
	/* ccm_reg->ccm_reg->PllDram1PatternReg0 = ccm_reg->ccm_reg_backup.PllDram1PatternReg0;*/
	/* ccm_reg->ccm_reg->PllDram1PatternReg1 = ccm_reg->ccm_reg_backup.PllDram1PatternReg1;*/

	/*cfg all moudules pll must close befor enter standby*/
	ccm_reg->ccm_reg->Pll2Ctl = ccm_reg->ccm_reg_backup.Pll2Ctl; /*0x008, audio*/
	ccm_reg->ccm_reg->Pll3Ctl = ccm_reg->ccm_reg_backup.Pll3Ctl; /*0x010, video0*/
	ccm_reg->ccm_reg->Pll4Ctl = ccm_reg->ccm_reg_backup.Pll4Ctl; /*0x018, ve*/
	/*ccm_reg->ccm_reg->Pll5Ctl = ccm_reg->ccm_reg_backup.Pll5Ctl;*/ /*0x020, ddr0*/
	ccm_reg->ccm_reg->Pll6Ctl = ccm_reg->ccm_reg_backup.Pll6Ctl; /*0x028, periph0*/
	ccm_reg->ccm_reg->Pll7Ctl = ccm_reg->ccm_reg_backup.Pll7Ctl; /*0x02c, periph1*/
	ccm_reg->ccm_reg->PllVideo1 = ccm_reg->ccm_reg_backup.PllVideo1; /*0x0030, Pll videio1 reg */
	ccm_reg->ccm_reg->PllSata = ccm_reg->ccm_reg_backup.PllSata; /*0x0034, sata ctrl reg */
	ccm_reg->ccm_reg->Pll8Ctl = ccm_reg->ccm_reg_backup.Pll8Ctl; /*0x0038, PLL8 control, gpu */
	ccm_reg->ccm_reg->PllMipi = ccm_reg->ccm_reg_backup.PllMipi; /*0x0040, MIPI PLL control */
	ccm_reg->ccm_reg->Pll10Ctl = ccm_reg->ccm_reg_backup.Pll10Ctl; /*0x048, de*/
	/*ccm_reg->ccm_reg->PllDdr1Ctl = ccm_reg->ccm_reg_backup.PllDdr1Ctl;*/ /*0x004c, pll ddr1 ctrl reg */
	change_runtime_env();
	delay_us(10);

	ccm_reg->ccm_reg->Pll1Ctl = ccm_reg->ccm_reg_backup.Pll1Ctl;
	change_runtime_env();
	delay_us(1000);

	/*notice: be care to cpu, axi restore sequence!. */
	ccm_reg->ccm_reg->SysClkDiv = ccm_reg->ccm_reg_backup.SysClkDiv; /*0x0050, cpux/axi clock divide ratio */
	ccm_reg->ccm_reg->Ahb1Div = ccm_reg->ccm_reg_backup.Ahb1Div; /*0x0054, ahb1/apb1 clock divide ratio */
	delay_us(1);
	ccm_reg->ccm_reg->Apb2Div = ccm_reg->ccm_reg_backup.Apb2Div; /*0x0058, apb2 clock divide ratio */

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	/*first, reset, then, gating. */
	ccm_reg->ccm_reg->AhbReset0 = ccm_reg->ccm_reg_backup.AhbReset0;
	ccm_reg->ccm_reg->AhbReset1 = ccm_reg->ccm_reg_backup.AhbReset1;
	ccm_reg->ccm_reg->AhbReset2 = ccm_reg->ccm_reg_backup.AhbReset2;
	ccm_reg->ccm_reg->Apb1Reset = ccm_reg->ccm_reg_backup.Apb1Reset;
	ccm_reg->ccm_reg->Apb2Reset = ccm_reg->ccm_reg_backup.Apb2Reset;

	ccm_reg->ccm_reg->Ths = ccm_reg->ccm_reg_backup.Ths;
	ccm_reg->ccm_reg->Nand0 = ccm_reg->ccm_reg_backup.Nand0;

	ccm_reg->ccm_reg->Sd0 = ccm_reg->ccm_reg_backup.Sd0;
	ccm_reg->ccm_reg->Sd1 = ccm_reg->ccm_reg_backup.Sd1;
	ccm_reg->ccm_reg->Sd2 = ccm_reg->ccm_reg_backup.Sd2;
	ccm_reg->ccm_reg->Sd3 = ccm_reg->ccm_reg_backup.Sd3;

	ccm_reg->ccm_reg->Ts = ccm_reg->ccm_reg_backup.Ts;
	ccm_reg->ccm_reg->Ce = ccm_reg->ccm_reg_backup.Ce;

	ccm_reg->ccm_reg->Spi0 = ccm_reg->ccm_reg_backup.Spi0;
	ccm_reg->ccm_reg->Spi1 = ccm_reg->ccm_reg_backup.Spi1;
	ccm_reg->ccm_reg->Spi2 = ccm_reg->ccm_reg_backup.Spi2;
	ccm_reg->ccm_reg->Spi2 = ccm_reg->ccm_reg_backup.Spi3;

	ccm_reg->ccm_reg->I2s0 = ccm_reg->ccm_reg_backup.I2s0;
	ccm_reg->ccm_reg->I2s1 = ccm_reg->ccm_reg_backup.I2s1;
	ccm_reg->ccm_reg->I2s2 = ccm_reg->ccm_reg_backup.I2s2;

	ccm_reg->ccm_reg->Ac97 = ccm_reg->ccm_reg_backup.Ac97;
	ccm_reg->ccm_reg->SpdifClk = ccm_reg->ccm_reg_backup.SpdifClk;
	ccm_reg->ccm_reg->KeyPadClk = ccm_reg->ccm_reg_backup.KeyPadClk;
	ccm_reg->ccm_reg->Sata0Clk = ccm_reg->ccm_reg_backup.Sata0Clk;
	ccm_reg->ccm_reg->UsbClk = ccm_reg->ccm_reg_backup.UsbClk;
	ccm_reg->ccm_reg->Ir0Clk = ccm_reg->ccm_reg_backup.Ir0Clk;
	ccm_reg->ccm_reg->Ir1Clk = ccm_reg->ccm_reg_backup.Ir1Clk;

	/* do not touch dram related config. */
	/* ccm_reg->ccm_reg->PllDdrAuxiliary = ccm_reg->ccm_reg_backup.PllDdrAuxiliary;*/
	/* ccm_reg->ccm_reg->DramCfg = ccm_reg->ccm_reg_backup.DramCfg;*/
	/* ccm_reg->ccm_reg_backup.PllDdrCfg = ccm_reg->ccm_reg->PllDdrCfg;*/
	/* ccm_reg->ccm_reg_backup.MbusResetReg = ccm_reg->ccm_reg->MbusResetReg;*/
	/* ccm_reg->ccm_reg->DramGate = ccm_reg->ccm_reg_backup.DramGate;*/

	ccm_reg->ccm_reg->DeClk = ccm_reg->ccm_reg_backup.DeClk;
	ccm_reg->ccm_reg->DeMpClk = ccm_reg->ccm_reg_backup.DeMpClk;
	ccm_reg->ccm_reg->Lcd0Ch0 = ccm_reg->ccm_reg_backup.Lcd0Ch0;
	ccm_reg->ccm_reg->Lcd1Ch0 = ccm_reg->ccm_reg_backup.Lcd1Ch0;
	ccm_reg->ccm_reg->Tv0Ch0 = ccm_reg->ccm_reg_backup.Tv0Ch0;
	ccm_reg->ccm_reg->Tv1Ch0 = ccm_reg->ccm_reg_backup.Tv1Ch0;
	ccm_reg->ccm_reg->DeinterlaceClk = ccm_reg->ccm_reg_backup.DeinterlaceClk;
	ccm_reg->ccm_reg->CsiMisc = ccm_reg->ccm_reg_backup.CsiMisc;

	ccm_reg->ccm_reg->Csi0 = ccm_reg->ccm_reg_backup.Csi0;
	ccm_reg->ccm_reg->Ve = ccm_reg->ccm_reg_backup.Ve;
	ccm_reg->ccm_reg->Avs = ccm_reg->ccm_reg_backup.Avs;
	ccm_reg->ccm_reg->HdmiClk = ccm_reg->ccm_reg_backup.HdmiClk;
	ccm_reg->ccm_reg->HdmiSlowClk = ccm_reg->ccm_reg_backup.HdmiSlowClk;

	/*ccm_reg->ccm_reg->MBus0       = ccm_reg->ccm_reg_backup.MBus0; */

	ccm_reg->ccm_reg->Gmac = ccm_reg->ccm_reg_backup.Gmac;
	ccm_reg->ccm_reg->MipiDsiClk = ccm_reg->ccm_reg_backup.MipiDsiClk;
	ccm_reg->ccm_reg->TvE0Clk = ccm_reg->ccm_reg_backup.TvE0Clk;
	ccm_reg->ccm_reg->TvE1Clk = ccm_reg->ccm_reg_backup.TvE1Clk;
	ccm_reg->ccm_reg->TvD0Clk = ccm_reg->ccm_reg_backup.TvD0Clk;
	ccm_reg->ccm_reg->TvD1Clk = ccm_reg->ccm_reg_backup.TvD1Clk;
	ccm_reg->ccm_reg->TvD2Clk = ccm_reg->ccm_reg_backup.TvD2Clk;
	ccm_reg->ccm_reg->TvD3Clk = ccm_reg->ccm_reg_backup.TvD3Clk;

	ccm_reg->ccm_reg->GpuClk = ccm_reg->ccm_reg_backup.GpuClk;
	ccm_reg->ccm_reg->OutAClk = ccm_reg->ccm_reg_backup.OutAClk;
	ccm_reg->ccm_reg->OutBClk = ccm_reg->ccm_reg_backup.OutBClk;

	ccm_reg->ccm_reg->PsCtrl = ccm_reg->ccm_reg_backup.PsCtrl;
	ccm_reg->ccm_reg->PsCnt = ccm_reg->ccm_reg_backup.PsCnt;
	ccm_reg->ccm_reg->Sys32kClk = ccm_reg->ccm_reg_backup.Sys32kClk | (0x16aa << 16);

	change_runtime_env();
	delay_us(10);
	ccm_reg->ccm_reg->PllLockCtrl = ccm_reg->ccm_reg_backup.PllLockCtrl;

	change_runtime_env();
	delay_us(1);
	ccm_reg->ccm_reg->AhbGate0 = ccm_reg->ccm_reg_backup.AhbGate0;
	ccm_reg->ccm_reg->AhbGate1 = ccm_reg->ccm_reg_backup.AhbGate1;
	ccm_reg->ccm_reg->Apb1Gate = ccm_reg->ccm_reg_backup.Apb1Gate;
	ccm_reg->ccm_reg->Apb2Gate0 = ccm_reg->ccm_reg_backup.Apb2Gate0;
	ccm_reg->ccm_reg->Apb2Gate1 = ccm_reg->ccm_reg_backup.Apb2Gate1;
	delay_us(10);

	return 0;
}

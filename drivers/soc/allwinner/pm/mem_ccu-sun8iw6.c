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

	/*bias: */
	ccm_reg->ccm_reg_backup.PllAudioBias = ccm_reg->ccm_reg->PllAudioBias;
	ccm_reg->ccm_reg_backup.PllGpuBias = ccm_reg->ccm_reg->PllGpuBias;
	ccm_reg->ccm_reg_backup.PllHsicBias = ccm_reg->ccm_reg->PllHsicBias;
	ccm_reg->ccm_reg_backup.PllDeBias = ccm_reg->ccm_reg->PllDeBias;
	ccm_reg->ccm_reg_backup.PllVideo1Bias = ccm_reg->ccm_reg->PllVideo1Bias;

	/*tune & pattern: */
	ccm_reg->ccm_reg_backup.PllAudioReg0Pattern =
	    ccm_reg->ccm_reg->PllAudioReg0Pattern;
	ccm_reg->ccm_reg_backup.PllAudioReg1Pattern =
	    ccm_reg->ccm_reg->PllAudioReg1Pattern;

	/*cfg: */
	ccm_reg->ccm_reg_backup.Pll2Ctl = ccm_reg->ccm_reg->Pll2Ctl;
	ccm_reg->ccm_reg_backup.Pll8Ctl = ccm_reg->ccm_reg->Pll8Ctl;
	ccm_reg->ccm_reg_backup.Pll9Ctl = ccm_reg->ccm_reg->Pll9Ctl;
	ccm_reg->ccm_reg_backup.Pll10Ctl = ccm_reg->ccm_reg->Pll10Ctl;
	ccm_reg->ccm_reg_backup.PllVe1Ctl = ccm_reg->ccm_reg->PllVe1Ctl;

	/*notice: be care to cpu, axi restore sequence!. */
	ccm_reg->ccm_reg_backup.SysClkDiv = ccm_reg->ccm_reg->SysClkDiv;
	ccm_reg->ccm_reg_backup.Ahb1Div = ccm_reg->ccm_reg->Ahb1Div;
	ccm_reg->ccm_reg_backup.Apb2Div = ccm_reg->ccm_reg->Apb2Div;
	ccm_reg->ccm_reg_backup.Ahb2Cfg = ccm_reg->ccm_reg->Ahb2Cfg;

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	ccm_reg->ccm_reg_backup.AhbGate0 = ccm_reg->ccm_reg->AhbGate0;
	ccm_reg->ccm_reg_backup.AhbGate1 = ccm_reg->ccm_reg->AhbGate1;
	ccm_reg->ccm_reg_backup.Apb1Gate = ccm_reg->ccm_reg->Apb1Gate;
	ccm_reg->ccm_reg_backup.Apb2Gate = ccm_reg->ccm_reg->Apb2Gate;

	ccm_reg->ccm_reg_backup.Cci400Clk = ccm_reg->ccm_reg->Cci400Clk;
	ccm_reg->ccm_reg_backup.Nand0 = ccm_reg->ccm_reg->Nand0;

	ccm_reg->ccm_reg_backup.Sd0 = ccm_reg->ccm_reg->Sd0;
	ccm_reg->ccm_reg_backup.Sd1 = ccm_reg->ccm_reg->Sd1;
	ccm_reg->ccm_reg_backup.Sd2 = ccm_reg->ccm_reg->Sd2;
	ccm_reg->ccm_reg_backup.Ss = ccm_reg->ccm_reg->Ss;

	ccm_reg->ccm_reg_backup.Spi0 = ccm_reg->ccm_reg->Spi0;
	ccm_reg->ccm_reg_backup.Spi1 = ccm_reg->ccm_reg->Spi1;

	ccm_reg->ccm_reg_backup.I2s0 = ccm_reg->ccm_reg->I2s0;
	ccm_reg->ccm_reg_backup.I2s1 = ccm_reg->ccm_reg->I2s1;
	ccm_reg->ccm_reg_backup.I2s2 = ccm_reg->ccm_reg->I2s2;

	ccm_reg->ccm_reg_backup.TdmClk = ccm_reg->ccm_reg->TdmClk;
	ccm_reg->ccm_reg_backup.SpdifClk = ccm_reg->ccm_reg->SpdifClk;
	ccm_reg->ccm_reg_backup.Usb = ccm_reg->ccm_reg->Usb;

	/* do not touch dram related config. */
	/*  ccm_reg->ccm_reg_backup.PllDdrCfg   = ccm_reg->ccm_reg->PllDdrCfg; */
	/*  ccm_reg->ccm_reg_backup.MbusResetReg        = ccm_reg->ccm_reg->MbusResetReg; */
	/*  ccm_reg->ccm_reg_backup.DramCfg             = ccm_reg->ccm_reg->DramCfg; */
	/*  ccm_reg->ccm_reg_backup.DramGate            = ccm_reg->ccm_reg->DramGate; */

	ccm_reg->ccm_reg_backup.Lcd0Ch0 = ccm_reg->ccm_reg->Lcd0Ch0;
	ccm_reg->ccm_reg_backup.Lcd0Ch1 = ccm_reg->ccm_reg->Lcd0Ch1;

	ccm_reg->ccm_reg_backup.MipiCsi = ccm_reg->ccm_reg->MipiCsi;
	ccm_reg->ccm_reg_backup.Csi0 = ccm_reg->ccm_reg->Csi0;

	ccm_reg->ccm_reg_backup.Ve = ccm_reg->ccm_reg->Ve;
	ccm_reg->ccm_reg_backup.Avs = ccm_reg->ccm_reg->Avs;
	ccm_reg->ccm_reg_backup.HdmiClk = ccm_reg->ccm_reg->HdmiClk;
	ccm_reg->ccm_reg_backup.HdmiSlowClk = ccm_reg->ccm_reg->HdmiSlowClk;
	/*mbus? function? */
	/*ccm_reg->ccm_reg_backup.MBus0         = ccm_reg->ccm_reg->MBus0; */
	ccm_reg->ccm_reg_backup.MipiDsiReg0 = ccm_reg->ccm_reg->MipiDsiReg0;
	ccm_reg->ccm_reg_backup.MipiDsiReg1 = ccm_reg->ccm_reg->MipiDsiReg1;

	ccm_reg->ccm_reg_backup.GpuCore = ccm_reg->ccm_reg->GpuCore;
	ccm_reg->ccm_reg_backup.GpuMem = ccm_reg->ccm_reg->GpuMem;
	ccm_reg->ccm_reg_backup.GpuHyd = ccm_reg->ccm_reg->GpuHyd;

	ccm_reg->ccm_reg_backup.PllLock = ccm_reg->ccm_reg->PllLock;
	ccm_reg->ccm_reg_backup.Pll1Lock = ccm_reg->ccm_reg->Pll1Lock;

	ccm_reg->ccm_reg_backup.PllStableStatus =
	    ccm_reg->ccm_reg->PllStableStatus;

	ccm_reg->ccm_reg_backup.AhbReset0 = ccm_reg->ccm_reg->AhbReset0;
	ccm_reg->ccm_reg_backup.AhbReset1 = ccm_reg->ccm_reg->AhbReset1;
	ccm_reg->ccm_reg_backup.AhbReset2 = ccm_reg->ccm_reg->AhbReset2;
	ccm_reg->ccm_reg_backup.Apb1Reset = ccm_reg->ccm_reg->Apb1Reset;
	ccm_reg->ccm_reg_backup.Apb2Reset = ccm_reg->ccm_reg->Apb2Reset;

	return 0;
}

__s32 mem_ccu_restore(struct ccm_state *ccm_reg)
{
	/*bias: */
	ccm_reg->ccm_reg->PllAudioBias = ccm_reg->ccm_reg_backup.PllAudioBias;
	ccm_reg->ccm_reg->PllGpuBias = ccm_reg->ccm_reg_backup.PllGpuBias;
	ccm_reg->ccm_reg->PllHsicBias = ccm_reg->ccm_reg_backup.PllHsicBias;
	ccm_reg->ccm_reg->PllDeBias = ccm_reg->ccm_reg_backup.PllDeBias;
	ccm_reg->ccm_reg->PllVideo1Bias = ccm_reg->ccm_reg_backup.PllVideo1Bias;

	/*tune & pattern: */
	ccm_reg->ccm_reg->PllAudioReg0Pattern =
	    ccm_reg->ccm_reg_backup.PllAudioReg0Pattern;
	ccm_reg->ccm_reg->PllAudioReg1Pattern =
	    ccm_reg->ccm_reg_backup.PllAudioReg1Pattern;

	/*cfg */
	ccm_reg->ccm_reg->Pll2Ctl = ccm_reg->ccm_reg_backup.Pll2Ctl;
	ccm_reg->ccm_reg->Pll8Ctl = ccm_reg->ccm_reg_backup.Pll8Ctl;
	ccm_reg->ccm_reg->Pll9Ctl = ccm_reg->ccm_reg_backup.Pll9Ctl;
	ccm_reg->ccm_reg->Pll10Ctl = ccm_reg->ccm_reg_backup.Pll10Ctl;
	ccm_reg->ccm_reg->PllVe1Ctl = ccm_reg->ccm_reg_backup.PllVe1Ctl;

	change_runtime_env();
	delay_us(1);

	/*notice: be care to cpu, axi restore sequence!. */
	ccm_reg->ccm_reg->SysClkDiv = ccm_reg->ccm_reg_backup.SysClkDiv;
	ccm_reg->ccm_reg->Ahb1Div = ccm_reg->ccm_reg_backup.Ahb1Div;
	delay_us(1);
	ccm_reg->ccm_reg->Apb2Div = ccm_reg->ccm_reg_backup.Apb2Div;
	ccm_reg->ccm_reg->Ahb2Cfg = ccm_reg->ccm_reg_backup.Ahb2Cfg;

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	/*first, reset, then, gating. */
	ccm_reg->ccm_reg->AhbReset0 = ccm_reg->ccm_reg_backup.AhbReset0;
	ccm_reg->ccm_reg->AhbReset1 = ccm_reg->ccm_reg_backup.AhbReset1;
	ccm_reg->ccm_reg->AhbReset2 = ccm_reg->ccm_reg_backup.AhbReset2;
	ccm_reg->ccm_reg->Apb1Reset = ccm_reg->ccm_reg_backup.Apb1Reset;
	ccm_reg->ccm_reg->Apb2Reset = ccm_reg->ccm_reg_backup.Apb2Reset;

	ccm_reg->ccm_reg->Cci400Clk =
	    (~0x3000000) & ccm_reg->ccm_reg_backup.Cci400Clk;
	delay_us(10);
	ccm_reg->ccm_reg->Nand0 = ccm_reg->ccm_reg_backup.Nand0;

	ccm_reg->ccm_reg->Sd0 = ccm_reg->ccm_reg_backup.Sd0;
	ccm_reg->ccm_reg->Sd1 = ccm_reg->ccm_reg_backup.Sd1;
	ccm_reg->ccm_reg->Sd2 = ccm_reg->ccm_reg_backup.Sd2;
	ccm_reg->ccm_reg->Ss = ccm_reg->ccm_reg_backup.Ss;

	ccm_reg->ccm_reg->Spi0 = ccm_reg->ccm_reg_backup.Spi0;
	ccm_reg->ccm_reg->Spi1 = ccm_reg->ccm_reg_backup.Spi1;

	ccm_reg->ccm_reg->I2s0 = ccm_reg->ccm_reg_backup.I2s0;
	ccm_reg->ccm_reg->I2s1 = ccm_reg->ccm_reg_backup.I2s1;
	ccm_reg->ccm_reg->I2s2 = ccm_reg->ccm_reg_backup.I2s2;

	ccm_reg->ccm_reg->TdmClk = ccm_reg->ccm_reg_backup.TdmClk;
	ccm_reg->ccm_reg->SpdifClk = ccm_reg->ccm_reg_backup.SpdifClk;
	ccm_reg->ccm_reg->Usb = ccm_reg->ccm_reg_backup.Usb;
#if 0
	ccm_reg->ccm_reg->DramCfg = ccm_reg->ccm_reg_backup.DramCfg;
	ccm_reg->ccm_reg_backup.PllDdrCfg = ccm_reg->ccm_reg->PllDdrCfg;
	ccm_reg->ccm_reg_backup.MbusResetReg = ccm_reg->ccm_reg->MbusResetReg;
	ccm_reg->ccm_reg->DramGate = ccm_reg->ccm_reg_backup.DramGate;
#endif

	ccm_reg->ccm_reg->Lcd0Ch0 = ccm_reg->ccm_reg_backup.Lcd0Ch0;
	ccm_reg->ccm_reg->Lcd0Ch1 = ccm_reg->ccm_reg_backup.Lcd0Ch1;

	ccm_reg->ccm_reg->MipiCsi = ccm_reg->ccm_reg_backup.MipiCsi;
	ccm_reg->ccm_reg->Csi0 = ccm_reg->ccm_reg_backup.Csi0;
	ccm_reg->ccm_reg->Ve = ccm_reg->ccm_reg_backup.Ve;
	ccm_reg->ccm_reg->Avs = ccm_reg->ccm_reg_backup.Avs;
	ccm_reg->ccm_reg->HdmiClk = ccm_reg->ccm_reg_backup.HdmiClk;
	ccm_reg->ccm_reg->HdmiSlowClk = ccm_reg->ccm_reg_backup.HdmiSlowClk;
	/*ccm_reg->ccm_reg->MBus0       = ccm_reg->ccm_reg_backup.MBus0; */
	ccm_reg->ccm_reg->MipiDsiReg0 = ccm_reg->ccm_reg_backup.MipiDsiReg0;
	ccm_reg->ccm_reg->MipiDsiReg1 = ccm_reg->ccm_reg_backup.MipiDsiReg1;

	ccm_reg->ccm_reg->GpuCore = ccm_reg->ccm_reg_backup.GpuCore;
	ccm_reg->ccm_reg->GpuMem = ccm_reg->ccm_reg_backup.GpuMem;
	ccm_reg->ccm_reg->GpuHyd = ccm_reg->ccm_reg_backup.GpuHyd;

	ccm_reg->ccm_reg->PllLock = ccm_reg->ccm_reg_backup.PllLock;
	ccm_reg->ccm_reg->Pll1Lock = ccm_reg->ccm_reg_backup.Pll1Lock;
	ccm_reg->ccm_reg->PllStableStatus =
	    ccm_reg->ccm_reg_backup.PllStableStatus;

	change_runtime_env();
	delay_us(1);
	ccm_reg->ccm_reg->AhbGate0 = ccm_reg->ccm_reg_backup.AhbGate0;
	ccm_reg->ccm_reg->AhbGate1 = ccm_reg->ccm_reg_backup.AhbGate1;
	ccm_reg->ccm_reg->Apb1Gate = ccm_reg->ccm_reg_backup.Apb1Gate;
	ccm_reg->ccm_reg->Apb2Gate = ccm_reg->ccm_reg_backup.Apb2Gate;
	/*config src. */
	ccm_reg->ccm_reg->Cci400Clk = ccm_reg->ccm_reg_backup.Cci400Clk;
	delay_us(10);

	return 0;
}

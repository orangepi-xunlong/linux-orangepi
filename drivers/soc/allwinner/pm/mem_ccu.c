/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "pm_types.h"
#include "pm_i.h"

#if defined(CONFIG_ARCH_SUN8IW5P1) || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)

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

	/*Pll1Bias;                                                                         //0x220,  pll cpux  bias reg */
	ccm_reg->ccm_reg_backup.PllAudioBias = ccm_reg->ccm_reg->PllAudioBias;	/*0x224,  pll audio bias reg */
	/*PllVedioBias;                                                                     //0x228,  pll vedio bias reg */
	/*PllVeBias;                                                                        //0x22c,  pll ve    bias reg */
	/*PllDram0Bias;                                                                     //0x230,  pll dram0 bias reg */
	/*PllPeriphBias;                                                                    //0x234,  pll periph bias reg */
	/*reserved202;                                                                      //0x238,  reserved */
	ccm_reg->ccm_reg_backup.PllGpuBias = ccm_reg->ccm_reg->PllGpuBias;	/*0x23c,  pll gpu   bias reg */
	ccm_reg->ccm_reg_backup.PllMipiBias = ccm_reg->ccm_reg->PllMipiBias;	/*0x240,  pll mipi  bias reg */
	ccm_reg->ccm_reg_backup.PllHsicBias = ccm_reg->ccm_reg->PllHsicBias;	/*0x244,  pll hsic  bias reg */
	ccm_reg->ccm_reg_backup.PllDeBias = ccm_reg->ccm_reg->PllDeBias;	/*0x248,  pll de    bias reg */
	/*ccm_reg->ccm_reg_backup.PllDram1BiasReg;                                          //0x24c,  pll dram1 bias */

	/*ccm_reg->ccm_reg_backup.Pll1Tun       = ccm_reg->ccm_reg->Pll1Tun; */
	/*ccm_reg->ccm_reg_backup.Pll5Tun       = ccm_reg->ccm_reg->Pll5Tun; */
	ccm_reg->ccm_reg_backup.MipiPllTun = ccm_reg->ccm_reg->MipiPllTun;

	/*ccm_reg->ccm_reg_backup.Pll1Ctl       = ccm_reg->ccm_reg->Pll1Ctl; */
	ccm_reg->ccm_reg_backup.Pll2Ctl = ccm_reg->ccm_reg->Pll2Ctl;
	/*ccm_reg->ccm_reg_backup.Pll3Ctl       = ccm_reg->ccm_reg->Pll3Ctl; */
	/*ccm_reg->ccm_reg_backup.Pll4Ctl       = ccm_reg->ccm_reg->Pll4Ctl; */
	/*ccm_reg->ccm_reg_backup.Pll6Ctl       = ccm_reg->ccm_reg->Pll6Ctl; */
	ccm_reg->ccm_reg_backup.Pll8Ctl = ccm_reg->ccm_reg->Pll8Ctl;
	ccm_reg->ccm_reg_backup.MipiPllCtl = ccm_reg->ccm_reg->MipiPllCtl;
	ccm_reg->ccm_reg_backup.Pll9Ctl = ccm_reg->ccm_reg->Pll9Ctl;
	ccm_reg->ccm_reg_backup.Pll10Ctl = ccm_reg->ccm_reg->Pll10Ctl;

	ccm_reg->ccm_reg_backup.SysClkDiv = ccm_reg->ccm_reg->SysClkDiv;
	ccm_reg->ccm_reg_backup.Ahb1Div = ccm_reg->ccm_reg->Ahb1Div;
	ccm_reg->ccm_reg_backup.Apb2Div = ccm_reg->ccm_reg->Apb2Div;

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	ccm_reg->ccm_reg_backup.AhbGate0 = ccm_reg->ccm_reg->AhbGate0;
	ccm_reg->ccm_reg_backup.AhbGate1 = ccm_reg->ccm_reg->AhbGate1;
	ccm_reg->ccm_reg_backup.Apb1Gate = ccm_reg->ccm_reg->Apb1Gate;
	ccm_reg->ccm_reg_backup.Apb2Gate = ccm_reg->ccm_reg->Apb2Gate;

	ccm_reg->ccm_reg_backup.Nand0 = ccm_reg->ccm_reg->Nand0;

	ccm_reg->ccm_reg_backup.Sd0 = ccm_reg->ccm_reg->Sd0;
	ccm_reg->ccm_reg_backup.Sd1 = ccm_reg->ccm_reg->Sd1;
	ccm_reg->ccm_reg_backup.Sd2 = ccm_reg->ccm_reg->Sd2;
	ccm_reg->ccm_reg_backup.Ss = ccm_reg->ccm_reg->Ss;

	ccm_reg->ccm_reg_backup.Spi0 = ccm_reg->ccm_reg->Spi0;
	ccm_reg->ccm_reg_backup.Spi1 = ccm_reg->ccm_reg->Spi1;

	ccm_reg->ccm_reg_backup.I2s0 = ccm_reg->ccm_reg->I2s0;
	ccm_reg->ccm_reg_backup.I2s1 = ccm_reg->ccm_reg->I2s1;

	ccm_reg->ccm_reg_backup.Usb = ccm_reg->ccm_reg->Usb;
	ccm_reg->ccm_reg_backup.DramCfg = ccm_reg->ccm_reg->DramCfg;
	ccm_reg->ccm_reg_backup.PllDdrCfg = ccm_reg->ccm_reg->PllDdrCfg;
	ccm_reg->ccm_reg_backup.MbusResetReg = ccm_reg->ccm_reg->MbusResetReg;
	ccm_reg->ccm_reg_backup.DramGate = ccm_reg->ccm_reg->DramGate;

	ccm_reg->ccm_reg_backup.Be0 = ccm_reg->ccm_reg->Be0;
	ccm_reg->ccm_reg_backup.Fe0 = ccm_reg->ccm_reg->Fe0;

	ccm_reg->ccm_reg_backup.Lcd0Ch0 = ccm_reg->ccm_reg->Lcd0Ch0;
	ccm_reg->ccm_reg_backup.Lcd0Ch1 = ccm_reg->ccm_reg->Lcd0Ch1;

	ccm_reg->ccm_reg_backup.Csi0 = ccm_reg->ccm_reg->Csi0;

	ccm_reg->ccm_reg_backup.Ve = ccm_reg->ccm_reg->Ve;
	ccm_reg->ccm_reg_backup.Adda = ccm_reg->ccm_reg->Adda;
	ccm_reg->ccm_reg_backup.Avs = ccm_reg->ccm_reg->Avs;
	/*ccm_reg->ccm_reg_backup.MBus0         = ccm_reg->ccm_reg->MBus0; */
	ccm_reg->ccm_reg_backup.MipiDsi = ccm_reg->ccm_reg->MipiDsi;
	ccm_reg->ccm_reg_backup.IepDrc0 = ccm_reg->ccm_reg->IepDrc0;

	ccm_reg->ccm_reg_backup.GpuCore = ccm_reg->ccm_reg->GpuCore;
	ccm_reg->ccm_reg_backup.GpuMem = ccm_reg->ccm_reg->GpuMem;
	ccm_reg->ccm_reg_backup.GpuHyd = ccm_reg->ccm_reg->GpuHyd;

	ccm_reg->ccm_reg_backup.PllLock = ccm_reg->ccm_reg->PllLock;
	ccm_reg->ccm_reg_backup.Pll1Lock = ccm_reg->ccm_reg->Pll1Lock;

	ccm_reg->ccm_reg_backup.AhbReset0 = ccm_reg->ccm_reg->AhbReset0;
	ccm_reg->ccm_reg_backup.AhbReset1 = ccm_reg->ccm_reg->AhbReset1;
	ccm_reg->ccm_reg_backup.AhbReset2 = ccm_reg->ccm_reg->AhbReset2;
	ccm_reg->ccm_reg_backup.Apb1Reset = ccm_reg->ccm_reg->Apb1Reset;
	ccm_reg->ccm_reg_backup.Apb2Reset = ccm_reg->ccm_reg->Apb2Reset;

	return 0;
}

__s32 mem_ccu_restore(struct ccm_state *ccm_reg)
{

	/*Pll1Bias;                                                                             //0x220,  pll cpux  bias reg */
	ccm_reg->ccm_reg->PllAudioBias = ccm_reg->ccm_reg_backup.PllAudioBias;	/*0x224,  pll audio bias reg */
	/*PllVedioBias;                                                                         //0x228,  pll vedio bias reg */
	/*PllVeBias;                                                                            //0x22c,  pll ve    bias reg */
	/*PllDram0Bias;                                                                         //0x230,  pll dram0 bias reg */
	/*PllPeriphBias;                                                                        //0x234,  pll periph bias reg */
	/*reserved202;                                                                          //0x238,  reserved */
	ccm_reg->ccm_reg->PllGpuBias = ccm_reg->ccm_reg_backup.PllGpuBias;	/*0x23c,  pll gpu   bias reg */
	ccm_reg->ccm_reg->PllMipiBias = ccm_reg->ccm_reg_backup.PllMipiBias;	/*0x240,  pll mipi  bias reg */
	ccm_reg->ccm_reg->PllHsicBias = ccm_reg->ccm_reg_backup.PllHsicBias;	/*0x244,  pll hsic  bias reg */
	ccm_reg->ccm_reg->PllDeBias = ccm_reg->ccm_reg_backup.PllDeBias;	/*0x248,  pll de    bias reg */
	/*ccm_reg->ccm_reg_backup.PllDram1BiasReg;                                              //0x24c,  pll dram1 bias */

	/*ccm_reg->ccm_reg->Pll1Tun             = ccm_reg->ccm_reg_backup.Pll1Tun; */
	/*ccm_reg->ccm_reg->Pll5Tun             = ccm_reg->ccm_reg_backup.Pll5Tun; */
	ccm_reg->ccm_reg->MipiPllTun = ccm_reg->ccm_reg_backup.MipiPllTun;

	/*ccm_reg->ccm_reg->Pll1Ctl             = ccm_reg->ccm_reg_backup.Pll1Ctl; */
	ccm_reg->ccm_reg->Pll2Ctl = ccm_reg->ccm_reg_backup.Pll2Ctl;
	/*ccm_reg->ccm_reg->Pll3Ctl             = ccm_reg->ccm_reg_backup.Pll3Ctl; */
	/*ccm_reg->ccm_reg->Pll4Ctl             = ccm_reg->ccm_reg_backup.Pll4Ctl; */
	/*ccm_reg->ccm_reg->Pll6Ctl             = ccm_reg->ccm_reg_backup.Pll6Ctl; */
	ccm_reg->ccm_reg->Pll8Ctl = ccm_reg->ccm_reg_backup.Pll8Ctl;
	ccm_reg->ccm_reg->MipiPllCtl = ccm_reg->ccm_reg_backup.MipiPllCtl;
	ccm_reg->ccm_reg->Pll9Ctl = ccm_reg->ccm_reg_backup.Pll9Ctl;
	ccm_reg->ccm_reg->Pll10Ctl = ccm_reg->ccm_reg_backup.Pll10Ctl;

	ccm_reg->ccm_reg->SysClkDiv = ccm_reg->ccm_reg_backup.SysClkDiv;
	ccm_reg->ccm_reg->Ahb1Div = ccm_reg->ccm_reg_backup.Ahb1Div;
	change_runtime_env();
	delay_us(1);
	ccm_reg->ccm_reg->Apb2Div = ccm_reg->ccm_reg_backup.Apb2Div;

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	/*first, reset, then, gating. */
	ccm_reg->ccm_reg->AhbReset0 = ccm_reg->ccm_reg_backup.AhbReset0;
	ccm_reg->ccm_reg->AhbReset1 = ccm_reg->ccm_reg_backup.AhbReset1;
	ccm_reg->ccm_reg->AhbReset2 = ccm_reg->ccm_reg_backup.AhbReset2;
	ccm_reg->ccm_reg->Apb1Reset = ccm_reg->ccm_reg_backup.Apb1Reset;
	ccm_reg->ccm_reg->Apb2Reset = ccm_reg->ccm_reg_backup.Apb2Reset;
	ccm_reg->ccm_reg->Nand0 = ccm_reg->ccm_reg_backup.Nand0;

	ccm_reg->ccm_reg->Sd0 = ccm_reg->ccm_reg_backup.Sd0;
	ccm_reg->ccm_reg->Sd1 = ccm_reg->ccm_reg_backup.Sd1;
	ccm_reg->ccm_reg->Sd2 = ccm_reg->ccm_reg_backup.Sd2;

	ccm_reg->ccm_reg->Ss = ccm_reg->ccm_reg_backup.Ss;

	ccm_reg->ccm_reg->Spi0 = ccm_reg->ccm_reg_backup.Spi0;
	ccm_reg->ccm_reg->Spi1 = ccm_reg->ccm_reg_backup.Spi1;

	ccm_reg->ccm_reg->I2s0 = ccm_reg->ccm_reg_backup.I2s0;
	ccm_reg->ccm_reg->I2s1 = ccm_reg->ccm_reg_backup.I2s1;

	ccm_reg->ccm_reg->Usb = ccm_reg->ccm_reg_backup.Usb;

#if 0
	ccm_reg->ccm_reg->DramCfg = ccm_reg->ccm_reg_backup.DramCfg;
	ccm_reg->ccm_reg_backup.PllDdrCfg = ccm_reg->ccm_reg->PllDdrCfg;
	ccm_reg->ccm_reg_backup.MbusResetReg = ccm_reg->ccm_reg->MbusResetReg;
#endif
	ccm_reg->ccm_reg->DramGate = ccm_reg->ccm_reg_backup.DramGate;

	ccm_reg->ccm_reg->Be0 = ccm_reg->ccm_reg_backup.Be0;
	ccm_reg->ccm_reg->Fe0 = ccm_reg->ccm_reg_backup.Fe0;

	ccm_reg->ccm_reg->Lcd0Ch0 = ccm_reg->ccm_reg_backup.Lcd0Ch0;
	ccm_reg->ccm_reg->Lcd0Ch1 = ccm_reg->ccm_reg_backup.Lcd0Ch1;

	ccm_reg->ccm_reg->Csi0 = ccm_reg->ccm_reg_backup.Csi0;
	ccm_reg->ccm_reg->Ve = ccm_reg->ccm_reg_backup.Ve;
	ccm_reg->ccm_reg->Adda = ccm_reg->ccm_reg_backup.Adda;
	ccm_reg->ccm_reg->Avs = ccm_reg->ccm_reg_backup.Avs;

	/*ccm_reg->ccm_reg->MBus0       = ccm_reg->ccm_reg_backup.MBus0; */
	ccm_reg->ccm_reg->MipiDsi = ccm_reg->ccm_reg_backup.MipiDsi;
	ccm_reg->ccm_reg->IepDrc0 = ccm_reg->ccm_reg_backup.IepDrc0;

	ccm_reg->ccm_reg->GpuCore = ccm_reg->ccm_reg_backup.GpuCore;
	ccm_reg->ccm_reg->GpuMem = ccm_reg->ccm_reg_backup.GpuMem;
	ccm_reg->ccm_reg->GpuHyd = ccm_reg->ccm_reg_backup.GpuHyd;

	ccm_reg->ccm_reg->PllLock = ccm_reg->ccm_reg_backup.PllLock;
	ccm_reg->ccm_reg->Pll1Lock = ccm_reg->ccm_reg_backup.Pll1Lock;

	change_runtime_env();
	delay_us(1);
	ccm_reg->ccm_reg->AhbGate0 = ccm_reg->ccm_reg_backup.AhbGate0;
	ccm_reg->ccm_reg->AhbGate1 = ccm_reg->ccm_reg_backup.AhbGate1;
	ccm_reg->ccm_reg->Apb1Gate = ccm_reg->ccm_reg_backup.Apb1Gate;
	ccm_reg->ccm_reg->Apb2Gate = ccm_reg->ccm_reg_backup.Apb2Gate;

	return 0;
}
#endif

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
	ccm_reg->ccm_mod_reg =
	    (__ccmu_mod_reg_list_t *) IO_ADDRESS(SUNXI_CCM_MOD_PBASE);

	/*module regs */
	ccm_reg->ccm_mod_reg_backup.nand0_sclk0_cfg =
	    ccm_reg->ccm_mod_reg->nand0_sclk0_cfg;
	ccm_reg->ccm_mod_reg_backup.nand0_sclk1_cfg =
	    ccm_reg->ccm_mod_reg->nand0_sclk1_cfg;
	ccm_reg->ccm_mod_reg_backup.nand1_sclk0_cfg =
	    ccm_reg->ccm_mod_reg->nand1_sclk0_cfg;
	ccm_reg->ccm_mod_reg_backup.nand1_sclk1_cfg =
	    ccm_reg->ccm_mod_reg->nand1_sclk1_cfg;
	ccm_reg->ccm_mod_reg_backup.sd0_clk = ccm_reg->ccm_mod_reg->sd0_clk;
	ccm_reg->ccm_mod_reg_backup.sd1_clk = ccm_reg->ccm_mod_reg->sd1_clk;
	ccm_reg->ccm_mod_reg_backup.sd2_clk = ccm_reg->ccm_mod_reg->sd2_clk;
	ccm_reg->ccm_mod_reg_backup.sd3_clk = ccm_reg->ccm_mod_reg->sd3_clk;
	ccm_reg->ccm_mod_reg_backup.ts_clk = ccm_reg->ccm_mod_reg->ts_clk;
	ccm_reg->ccm_mod_reg_backup.ss_clk = ccm_reg->ccm_mod_reg->ss_clk;
	ccm_reg->ccm_mod_reg_backup.spi0_clk = ccm_reg->ccm_mod_reg->spi0_clk;
	ccm_reg->ccm_mod_reg_backup.spi1_clk = ccm_reg->ccm_mod_reg->spi1_clk;
	ccm_reg->ccm_mod_reg_backup.spi2_clk = ccm_reg->ccm_mod_reg->spi2_clk;
	ccm_reg->ccm_mod_reg_backup.spi3_clk = ccm_reg->ccm_mod_reg->spi3_clk;
	ccm_reg->ccm_mod_reg_backup.daudio0_clk =
	    ccm_reg->ccm_mod_reg->daudio0_clk;
	ccm_reg->ccm_mod_reg_backup.daudio1_clk =
	    ccm_reg->ccm_mod_reg->daudio1_clk;
	ccm_reg->ccm_mod_reg_backup.spdif_clk = ccm_reg->ccm_mod_reg->spdif_clk;
	ccm_reg->ccm_mod_reg_backup.usbphy0_cfg =
	    ccm_reg->ccm_mod_reg->usbphy0_cfg;
	ccm_reg->ccm_mod_reg_backup.mdfs_clk = ccm_reg->ccm_mod_reg->mdfs_clk;
	ccm_reg->ccm_mod_reg_backup.dram_cfg = ccm_reg->ccm_mod_reg->dram_cfg;
	ccm_reg->ccm_mod_reg_backup.de_sclk_cfg =
	    ccm_reg->ccm_mod_reg->de_sclk_cfg;
	ccm_reg->ccm_mod_reg_backup.edp_sclk_cfg =
	    ccm_reg->ccm_mod_reg->edp_sclk_cfg;
	ccm_reg->ccm_mod_reg_backup.mp_clk = ccm_reg->ccm_mod_reg->mp_clk;
	ccm_reg->ccm_mod_reg_backup.lcd0_clk = ccm_reg->ccm_mod_reg->lcd0_clk;
	ccm_reg->ccm_mod_reg_backup.lcd1_clk = ccm_reg->ccm_mod_reg->lcd1_clk;
	ccm_reg->ccm_mod_reg_backup.mipi_dsi_clk0 =
	    ccm_reg->ccm_mod_reg->mipi_dsi_clk0;
	ccm_reg->ccm_mod_reg_backup.mipi_dsi_clk1 =
	    ccm_reg->ccm_mod_reg->mipi_dsi_clk1;
	ccm_reg->ccm_mod_reg_backup.hdmi_sclk = ccm_reg->ccm_mod_reg->hdmi_sclk;
	ccm_reg->ccm_mod_reg_backup.hdmi_slow_clk0 =
	    ccm_reg->ccm_mod_reg->hdmi_slow_clk0;
	ccm_reg->ccm_mod_reg_backup.mipi_csi_cfg =
	    ccm_reg->ccm_mod_reg->mipi_csi_cfg;
	ccm_reg->ccm_mod_reg_backup.csi_isp_clk =
	    ccm_reg->ccm_mod_reg->csi_isp_clk;
	ccm_reg->ccm_mod_reg_backup.csi0_mclk = ccm_reg->ccm_mod_reg->csi0_mclk;
	ccm_reg->ccm_mod_reg_backup.csi1_mclk = ccm_reg->ccm_mod_reg->csi1_mclk;
	ccm_reg->ccm_mod_reg_backup.fd_clk = ccm_reg->ccm_mod_reg->fd_clk;
	ccm_reg->ccm_mod_reg_backup.ve_clk = ccm_reg->ccm_mod_reg->ve_clk;
	ccm_reg->ccm_mod_reg_backup.avs_clk = ccm_reg->ccm_mod_reg->avs_clk;
	ccm_reg->ccm_mod_reg_backup.gpu_core_clk =
	    ccm_reg->ccm_mod_reg->gpu_core_clk;
	ccm_reg->ccm_mod_reg_backup.gpu_mem_clk =
	    ccm_reg->ccm_mod_reg->gpu_mem_clk;
	ccm_reg->ccm_mod_reg_backup.gpu_axi_clk =
	    ccm_reg->ccm_mod_reg->gpu_axi_clk;
	ccm_reg->ccm_mod_reg_backup.sata_clk = ccm_reg->ccm_mod_reg->sata_clk;
	ccm_reg->ccm_mod_reg_backup.ac97_clk = ccm_reg->ccm_mod_reg->ac97_clk;
	ccm_reg->ccm_mod_reg_backup.mipi_hsi_clk =
	    ccm_reg->ccm_mod_reg->mipi_hsi_clk;
	ccm_reg->ccm_mod_reg_backup.gp_adc = ccm_reg->ccm_mod_reg->gp_adc;
	ccm_reg->ccm_mod_reg_backup.cir_tx_clk =
	    ccm_reg->ccm_mod_reg->cir_tx_clk;

	/*module clk */
	ccm_reg->ccm_mod_reg_backup.ahb0_gating =
	    ccm_reg->ccm_mod_reg->ahb0_gating;
	ccm_reg->ccm_mod_reg_backup.ahb1_gating =
	    ccm_reg->ccm_mod_reg->ahb1_gating;
	ccm_reg->ccm_mod_reg_backup.ahb2_gating =
	    ccm_reg->ccm_mod_reg->ahb2_gating;
	ccm_reg->ccm_mod_reg_backup.apb0_gating =
	    ccm_reg->ccm_mod_reg->apb0_gating;
	ccm_reg->ccm_mod_reg_backup.apb1_gating =
	    ccm_reg->ccm_mod_reg->apb1_gating;
	ccm_reg->ccm_mod_reg_backup.ahb0_rst = ccm_reg->ccm_mod_reg->ahb0_rst;
	ccm_reg->ccm_mod_reg_backup.ahb1_rst = ccm_reg->ccm_mod_reg->ahb1_rst;
	ccm_reg->ccm_mod_reg_backup.ahb2_rst = ccm_reg->ccm_mod_reg->ahb2_rst;
	ccm_reg->ccm_mod_reg_backup.apb0_rst = ccm_reg->ccm_mod_reg->apb0_rst;
	ccm_reg->ccm_mod_reg_backup.apb1_rst = ccm_reg->ccm_mod_reg->apb1_rst;

	return 0;
}

__s32 mem_ccu_restore(struct ccm_state *ccm_reg)
{
	/*first, reset, then, gating. */
	ccm_reg->ccm_mod_reg->ahb0_rst = ccm_reg->ccm_mod_reg_backup.ahb0_rst;
	ccm_reg->ccm_mod_reg->ahb1_rst = ccm_reg->ccm_mod_reg_backup.ahb1_rst;
	ccm_reg->ccm_mod_reg->ahb2_rst = ccm_reg->ccm_mod_reg_backup.ahb2_rst;
	ccm_reg->ccm_mod_reg->apb0_rst = ccm_reg->ccm_mod_reg_backup.apb0_rst;
	ccm_reg->ccm_mod_reg->apb1_rst = ccm_reg->ccm_mod_reg_backup.apb1_rst;

	ccm_reg->ccm_mod_reg->nand0_sclk0_cfg =
	    ccm_reg->ccm_mod_reg_backup.nand0_sclk0_cfg;
	ccm_reg->ccm_mod_reg->nand0_sclk1_cfg =
	    ccm_reg->ccm_mod_reg_backup.nand0_sclk1_cfg;
	ccm_reg->ccm_mod_reg->nand1_sclk0_cfg =
	    ccm_reg->ccm_mod_reg_backup.nand1_sclk0_cfg;
	ccm_reg->ccm_mod_reg->nand1_sclk1_cfg =
	    ccm_reg->ccm_mod_reg_backup.nand1_sclk1_cfg;
	ccm_reg->ccm_mod_reg->sd0_clk = ccm_reg->ccm_mod_reg_backup.sd0_clk;
	ccm_reg->ccm_mod_reg->sd1_clk = ccm_reg->ccm_mod_reg_backup.sd1_clk;
	ccm_reg->ccm_mod_reg->sd2_clk = ccm_reg->ccm_mod_reg_backup.sd2_clk;
	ccm_reg->ccm_mod_reg->sd3_clk = ccm_reg->ccm_mod_reg_backup.sd3_clk;
	ccm_reg->ccm_mod_reg->ts_clk = ccm_reg->ccm_mod_reg_backup.ts_clk;
	ccm_reg->ccm_mod_reg->ss_clk = ccm_reg->ccm_mod_reg_backup.ss_clk;
	ccm_reg->ccm_mod_reg->spi0_clk = ccm_reg->ccm_mod_reg_backup.spi0_clk;
	ccm_reg->ccm_mod_reg->spi1_clk = ccm_reg->ccm_mod_reg_backup.spi1_clk;
	ccm_reg->ccm_mod_reg->spi2_clk = ccm_reg->ccm_mod_reg_backup.spi2_clk;
	ccm_reg->ccm_mod_reg->spi3_clk = ccm_reg->ccm_mod_reg_backup.spi3_clk;
	ccm_reg->ccm_mod_reg->daudio0_clk =
	    ccm_reg->ccm_mod_reg_backup.daudio0_clk;
	ccm_reg->ccm_mod_reg->daudio1_clk =
	    ccm_reg->ccm_mod_reg_backup.daudio1_clk;
	ccm_reg->ccm_mod_reg->spdif_clk = ccm_reg->ccm_mod_reg_backup.spdif_clk;
	ccm_reg->ccm_mod_reg->usbphy0_cfg =
	    ccm_reg->ccm_mod_reg_backup.usbphy0_cfg;
	ccm_reg->ccm_mod_reg->mdfs_clk = ccm_reg->ccm_mod_reg_backup.mdfs_clk;
	/*ccm_reg->ccm_mod_reg->dram_cfg                = ccm_reg->ccm_mod_reg_backup.dram_cfg            ; */
	ccm_reg->ccm_mod_reg->de_sclk_cfg =
	    ccm_reg->ccm_mod_reg_backup.de_sclk_cfg;
	ccm_reg->ccm_mod_reg->edp_sclk_cfg =
	    ccm_reg->ccm_mod_reg_backup.edp_sclk_cfg;
	ccm_reg->ccm_mod_reg->mp_clk = ccm_reg->ccm_mod_reg_backup.mp_clk;
	ccm_reg->ccm_mod_reg->lcd0_clk = ccm_reg->ccm_mod_reg_backup.lcd0_clk;
	ccm_reg->ccm_mod_reg->lcd1_clk = ccm_reg->ccm_mod_reg_backup.lcd1_clk;
	ccm_reg->ccm_mod_reg->mipi_dsi_clk0 =
	    ccm_reg->ccm_mod_reg_backup.mipi_dsi_clk0;
	ccm_reg->ccm_mod_reg->mipi_dsi_clk1 =
	    ccm_reg->ccm_mod_reg_backup.mipi_dsi_clk1;
	ccm_reg->ccm_mod_reg->hdmi_sclk = ccm_reg->ccm_mod_reg_backup.hdmi_sclk;
	ccm_reg->ccm_mod_reg->hdmi_slow_clk0 =
	    ccm_reg->ccm_mod_reg_backup.hdmi_slow_clk0;
	ccm_reg->ccm_mod_reg->mipi_csi_cfg =
	    ccm_reg->ccm_mod_reg_backup.mipi_csi_cfg;
	ccm_reg->ccm_mod_reg->csi_isp_clk =
	    ccm_reg->ccm_mod_reg_backup.csi_isp_clk;
	ccm_reg->ccm_mod_reg->csi0_mclk = ccm_reg->ccm_mod_reg_backup.csi0_mclk;
	ccm_reg->ccm_mod_reg->csi1_mclk = ccm_reg->ccm_mod_reg_backup.csi1_mclk;
	ccm_reg->ccm_mod_reg->fd_clk = ccm_reg->ccm_mod_reg_backup.fd_clk;
	ccm_reg->ccm_mod_reg->ve_clk = ccm_reg->ccm_mod_reg_backup.ve_clk;
	ccm_reg->ccm_mod_reg->avs_clk = ccm_reg->ccm_mod_reg_backup.avs_clk;
	ccm_reg->ccm_mod_reg->gpu_core_clk =
	    ccm_reg->ccm_mod_reg_backup.gpu_core_clk;
	ccm_reg->ccm_mod_reg->gpu_mem_clk =
	    ccm_reg->ccm_mod_reg_backup.gpu_mem_clk;
	ccm_reg->ccm_mod_reg->gpu_axi_clk =
	    ccm_reg->ccm_mod_reg_backup.gpu_axi_clk;
	ccm_reg->ccm_mod_reg->sata_clk = ccm_reg->ccm_mod_reg_backup.sata_clk;
	ccm_reg->ccm_mod_reg->ac97_clk = ccm_reg->ccm_mod_reg_backup.ac97_clk;
	ccm_reg->ccm_mod_reg->mipi_hsi_clk =
	    ccm_reg->ccm_mod_reg_backup.mipi_hsi_clk;
	ccm_reg->ccm_mod_reg->gp_adc = ccm_reg->ccm_mod_reg_backup.gp_adc;
	ccm_reg->ccm_mod_reg->cir_tx_clk =
	    ccm_reg->ccm_mod_reg_backup.cir_tx_clk;

	/*second, module clk related. */
	change_runtime_env();
	delay_us(1);
	ccm_reg->ccm_mod_reg->ahb0_gating =
	    ccm_reg->ccm_mod_reg_backup.ahb0_gating;
	ccm_reg->ccm_mod_reg->ahb1_gating =
	    ccm_reg->ccm_mod_reg_backup.ahb1_gating;
	ccm_reg->ccm_mod_reg->ahb2_gating =
	    ccm_reg->ccm_mod_reg_backup.ahb2_gating;
	ccm_reg->ccm_mod_reg->apb0_gating =
	    ccm_reg->ccm_mod_reg_backup.apb0_gating;
	ccm_reg->ccm_mod_reg->apb1_gating =
	    ccm_reg->ccm_mod_reg_backup.apb1_gating;

	return 0;
}

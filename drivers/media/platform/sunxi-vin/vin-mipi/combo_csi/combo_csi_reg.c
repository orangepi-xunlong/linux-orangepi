/*
 * combo csi module
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn <zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "combo_csi_reg_i.h"
#include "combo_csi_reg.h"
#include "../../utility/vin_io.h"
#include "../../platform/platform_cfg.h"

static unsigned char cmb_phy_lane[3][4] = {
	/*phyA*/      /*phyB*/      /*phyC*/
	{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11} /*ch*/
};

volatile void *cmb_csi_top_base_addr;
volatile void *cmb_csi_phy_base_addr[VIN_MAX_MIPI];
volatile void *cmb_csi_port_base_addr[VIN_MAX_MIPI];

int cmb_csi_set_top_base_addr(unsigned long addr)
{
	cmb_csi_top_base_addr = (volatile void *)addr;

	return 0;
}

int cmb_csi_set_phy_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > VIN_MAX_MIPI - 1)
		return -1;
	cmb_csi_phy_base_addr[sel] = (volatile void *)addr;

	return 0;
}

int cmb_csi_set_port_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > VIN_MAX_MIPI - 1)
		return -1;
	cmb_csi_port_base_addr[sel] = (volatile void *)addr;

	return 0;
}

/*
 * Detail function information of registers----PHY TOP
 */
void cmb_phy_reset_assert(void)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_RSTN_MASK, 0 << CMB_PHY_RSTN);
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_PWDNZ_MASK, 0 << CMB_PHY_PWDNZ);
}

void cmb_phy_reset_deassert(void)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_PWDNZ_MASK, 1 << CMB_PHY_PWDNZ);
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_RSTN_MASK, 1 << CMB_PHY_RSTN);
}

void cmb_phy_power_enable(void)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_VREF_EN_MASK, 1 << CMB_PHY_VREF_EN);
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_LVLDO_EN_MASK, 1 << CMB_PHY_LVLDO_EN);
}

void cmb_phy_power_disable(void)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_VREF_EN_MASK, 0 << CMB_PHY_VREF_EN);
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_LVLDO_EN_MASK, 0 << CMB_PHY_LVLDO_EN);
}

void cmb_phy_set_vref_0p9(unsigned int step)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_VREF_OP9_MASK, step << CMB_PHY_VREF_OP9);
}

void cmb_phy_set_vref_0p2(unsigned int step)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_PHY_TOP_REG_OFF,
		CMB_PHY_VREF_OP2_MASK, step << CMB_PHY_VREF_OP2);
}

void cmb_phy_set_trescal(unsigned int val)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_TRESCAL_REG_OFF,
		CMB_PHYA_TRESCAL_AUTO_MASK, 0x1 << CMB_PHYA_TRESCAL_AUTO);
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_TRESCAL_REG_OFF,
		CMB_PHYA_TRESCAL_SOFT_MASK, 0x0 << CMB_PHYA_TRESCAL_SOFT);
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_TRESCAL_REG_OFF,
		CMB_PHYA_TRESCAL_SET_MASK, val << CMB_PHYA_TRESCAL_SET);
}

void cmb_phya_trescal_reset_assert(void)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_TRESCAL_REG_OFF,
		CMB_PHYA_TRESCAL_RESETN_MASK, 0x0 << CMB_PHYA_TRESCAL_RESETN);
}

void cmb_phya_trescal_reset_deassert(void)
{
	vin_reg_clr_set(cmb_csi_top_base_addr + CMB_TRESCAL_REG_OFF,
		CMB_PHYA_TRESCAL_RESETN_MASK, 0x1 << CMB_PHYA_TRESCAL_RESETN);
}

void cmb_phy_top_enable(void)
{
	cmb_phy_set_vref_0p9(0x1);
	cmb_phy_set_vref_0p2(0x2);
	cmb_phy_set_trescal(0xe);
	cmb_phy_reset_deassert();
	cmb_phy_power_enable();

	cmb_phya_trescal_reset_assert();
	cmb_phya_trescal_reset_deassert();
}

void cmb_phy_top_disable(void)
{
	cmb_phy_reset_assert();
	cmb_phy_power_disable();
}

/*
 * Detail function information of registers----PHYA/B
 */
void cmb_phy0_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
		CMB_PHY0_EN_MASK, en << CMB_PHY0_EN);
}

void cmb_phy_lane_num_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
		CMB_PHY_LANEDT_EN_MASK, phy_lane_cfg.phy_lanedt_en << CMB_PHY_LANEDT_EN);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
		CMB_PHY_LANECK_EN_MASK, phy_lane_cfg.phy_laneck_en << CMB_PHY_LANECK_EN);
}

void cmb_phy0_work_mode(unsigned int sel, unsigned int mode) /* mode 0 --mipi, mode 1 --sub-lvds/hispi*/
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
		CMB_PHY0_IBIAS_EN_MASK, 1 << CMB_PHY0_IBIAS_EN);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
		CMB_PHY0_LP_PEFI_MASK, 1 << CMB_PHY0_LP_PEFI);
	if (mode == 0) {
		vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
			CMB_PHY0_HS_PEFI_MASK, 0x4 << CMB_PHY0_HS_PEFI);
		vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
			CMB_PHY0_WORK_MODE_MASK, 0 << CMB_PHY0_WORK_MODE);
	} else {
		vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
			CMB_PHY0_HS_PEFI_MASK, 0x2 << CMB_PHY0_HS_PEFI);
		vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_CTL_REG_OFF,
			CMB_PHY0_WORK_MODE_MASK, 1 << CMB_PHY0_WORK_MODE);
	}
}

void cmb_phy0_ofscal_cfg(unsigned int sel)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_OFSCAL0_OFF,
			CMB_PHY0_OFSCAL_AUTO_MASK, 0 << CMB_PHY0_OFSCAL_AUTO);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_OFSCAL0_OFF,
			CMB_PHY0_OFSCAL_SOFT_MASK, 1 << CMB_PHY0_OFSCAL_SOFT);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_OFSCAL1_OFF,
			CMB_PHY0_OFSCAL_SET_MASK, 0x13 << CMB_PHY0_OFSCAL_SET);
}

void cmb_phy_deskew_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_DESKEW0_OFF,
		CMB_PHY_DESKEW_EN_MASK, phy_lane_cfg.phy_deskew_en << CMB_PHY_DESKEW_EN);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_DESKEW0_OFF,
		CMB_PHY_DESKEW_PERIOD_EN_MASK, phy_lane_cfg.phy_deskew_period_en << CMB_PHY_DESKEW_PERIOD_EN);
}

void cmb_phy0_term_dly(unsigned int sel, unsigned int dly)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_TERM_CTL_REG_OFF,
		CMB_PHY0_TERM_EN_DLY_MASK, dly << CMB_PHY0_TERM_EN_DLY);
}

void cmb_phy_mipi_termnum_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_TERM_CTL_REG_OFF,
		CMB_PHY_TERMDT_EN_MASK, phy_lane_cfg.phy_termdt_en << CMB_PHY_TERMDT_EN);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_TERM_CTL_REG_OFF,
		CMB_PHY_TERMCK_EN_MASK, phy_lane_cfg.phy_termck_en << CMB_PHY_TERMCK_EN);
}

void cmb_term_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	cmb_phy0_term_dly(sel, 0);
	cmb_phy_mipi_termnum_en(sel, phy_lane_cfg);
}

void cmb_phy0_hs_dly(unsigned int sel, unsigned int dly)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_HS_CTL_REG_OFF,
		CMB_PHY0_HS_DLY_MASK, dly << CMB_PHY0_HS_DLY);
}

void cmb_phy_hs_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_HS_CTL_REG_OFF,
		CMB_PHY_HSDT_EN_MASK, phy_lane_cfg.phy_hsdt_en << CMB_PHY_HSDT_EN);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_HS_CTL_REG_OFF,
		CMB_PHY_HSCK_EN_MASK, phy_lane_cfg.phy_hsck_en << CMB_PHY_HSCK_EN);
}

void cmb_hs_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	cmb_phy0_hs_dly(sel, 0);
	cmb_phy_hs_en(sel, phy_lane_cfg);
}

void cmb_phy_s2p_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_S2P_CTL_REG_OFF,
		CMB_PHY_S2P_EN_MASK, phy_lane_cfg.phy_s2p_en << CMB_PHY_S2P_EN);
}

void cmb_phy0_s2p_width(unsigned int sel, unsigned int width)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_S2P_CTL_REG_OFF,
		CMB_PHY0_S2P_WIDTH_MASK, width << CMB_PHY0_S2P_WIDTH);
}

void cmb_phy0_s2p_dly(unsigned int sel, unsigned int dly)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_S2P_CTL_REG_OFF,
		CMB_PHY0_S2P_DLY_MASK, dly << CMB_PHY0_S2P_DLY);
}

void cmb_s2p_ctl(unsigned int sel, unsigned int dly, struct phy_lane_cfg phy_lane_cfg)
{
	cmb_phy_s2p_en(sel, phy_lane_cfg);
	cmb_phy0_s2p_width(sel, 0x3);
	cmb_phy0_s2p_dly(sel, dly);
}

void cmb_phy_mipi_lpnum_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_MIPIRX_CTL_REG_OFF,
		CMB_PHY_MIPI_LPDT_EN_MASK, phy_lane_cfg.phy_mipi_lpdt_en << CMB_PHY_MIPI_LPDT_EN);
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_MIPIRX_CTL_REG_OFF,
		CMB_PHY_MIPI_LPCK_EN_MASK, phy_lane_cfg.phy_mipi_lpck_en << CMB_PHY_MIPI_LPCK_EN);
}

void cmb_phy0_mipilp_dbc_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_MIPIRX_CTL_REG_OFF,
		CMB_PHY0_MIPILP_DBC_EN_MASK, en << CMB_PHY0_MIPILP_DBC_EN);
}

void cmb_phy0_mipihs_sync_mode(unsigned int sel, unsigned int mode)
{
	vin_reg_clr_set(cmb_csi_phy_base_addr[sel] + CMB_PHY_MIPIRX_CTL_REG_OFF,
		CMB_PHY0_MIPIHS_SYNC_MODE_MASK, mode << CMB_PHY0_MIPIHS_SYNC_MODE);
}

void cmb_mipirx_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{
	cmb_phy_mipi_lpnum_en(sel, phy_lane_cfg);
	cmb_phy0_mipilp_dbc_en(sel, 1);
	cmb_phy0_mipihs_sync_mode(sel, 0);
}

/*
 * Detail function information of registers----PORT0/1
 */
void cmb_port_enable(unsigned int sel)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_CTL_REG_OFF,
		CMB_PORT_EN_MASK, 1 << CMB_PORT_EN);
}

void cmb_port_disable(unsigned int sel)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_CTL_REG_OFF,
		CMB_PORT_EN_MASK, 0 << CMB_PORT_EN);
}

void cmb_port_lane_num(unsigned int sel, unsigned int num)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_CTL_REG_OFF,
		CMB_PORT_LANE_NUM_MASK, num << CMB_PORT_LANE_NUM);
}

void cmb_port_out_num(unsigned int sel, enum cmb_csi_pix_num cmb_csi_pix_num)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_CTL_REG_OFF,
		CMB_PORT_OUT_NUM_MASK, cmb_csi_pix_num << CMB_PORT_OUT_NUM);
}

void cmb_port_out_chnum(unsigned int sel, unsigned int chnum)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_CTL_REG_OFF,
		CMB_PORT_CHANNEL_NUM_MASK, chnum << CMB_PORT_CHANNEL_NUM);
}

unsigned char cmb_port_set_lane_map(unsigned int phy, unsigned int ch)
{
	return cmb_phy_lane[phy][ch];
}

void cmb_port_lane_map(unsigned int sel, unsigned char *mipi_lane)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE0_ID_MASK, mipi_lane[0] << CMB_PORT_LANE0_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE1_ID_MASK, mipi_lane[1] << CMB_PORT_LANE1_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE2_ID_MASK, mipi_lane[2] << CMB_PORT_LANE2_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE3_ID_MASK, mipi_lane[3] << CMB_PORT_LANE3_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE4_ID_MASK, mipi_lane[4] << CMB_PORT_LANE4_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE5_ID_MASK, mipi_lane[5] << CMB_PORT_LANE5_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE6_ID_MASK, mipi_lane[6] << CMB_PORT_LANE6_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG0_OFF,
		CMB_PORT_LANE7_ID_MASK, mipi_lane[7] << CMB_PORT_LANE7_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG1_OFF,
		CMB_PORT_LANE8_ID_MASK, mipi_lane[8] << CMB_PORT_LANE8_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG1_OFF,
		CMB_PORT_LANE9_ID_MASK, mipi_lane[9] << CMB_PORT_LANE9_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG1_OFF,
		CMB_PORT_LANE10_ID_MASK, mipi_lane[10] << CMB_PORT_LANE10_ID);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_LANE_MAP_REG1_OFF,
		CMB_PORT_LANE11_ID_MASK, mipi_lane[11] << CMB_PORT_LANE11_ID);
}

void cmb_port_set_wdr_mode(unsigned int sel, unsigned int mode)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_WDR_MODE_REG_OFF,
		CMB_PORT_WDR_MODE_MASK, mode << CMB_PORT_WDR_MODE);
}

void cmb_port_set_fid_mode(unsigned int sel, unsigned int mode)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
		CMB_PORT_FID_MODE_MASK, mode << CMB_PORT_FID_MODE);
}

void cmb_port_set_fid_ch_map(unsigned int sel, unsigned int ch)
{
	switch (ch) {
	case 2:
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID0_MAP_MASK, 0 << CMB_PORT_FID0_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID1_MAP_MASK, 1 << CMB_PORT_FID1_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID_MAP_EN_MASK, 0x3 << CMB_PORT_FID_MAP_EN);
		break;
	case 3:
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID0_MAP_MASK, 0 << CMB_PORT_FID0_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID1_MAP_MASK, 1 << CMB_PORT_FID1_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID2_MAP_MASK, 2 << CMB_PORT_FID2_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID_MAP_EN_MASK, 0x7 << CMB_PORT_FID_MAP_EN);
		break;
	case 4:
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID0_MAP_MASK, 0 << CMB_PORT_FID0_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID1_MAP_MASK, 1 << CMB_PORT_FID1_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID2_MAP_MASK, 2 << CMB_PORT_FID2_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID3_MAP_MASK, 3 << CMB_PORT_FID3_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID_MAP_EN_MASK, 0xf << CMB_PORT_FID_MAP_EN);
		break;
	default:
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID0_MAP_MASK, 0 << CMB_PORT_FID0_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID1_MAP_MASK, 1 << CMB_PORT_FID1_MAP);
		vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_FID_SEL_REG_OFF,
			CMB_PORT_FID_MAP_EN_MASK, 0x3 << CMB_PORT_FID_MAP_EN);
		break;
	}
}

void cmb_port_mipi_unpack_enable(unsigned int sel)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_CFG_REG_OFF,
		CMB_MIPI_UNPACK_EN_MASK, 1 << CMB_MIPI_UNPACK_EN);
}

void cmb_port_mipi_unpack_disable(unsigned int sel)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_CFG_REG_OFF,
		CMB_MIPI_UNPACK_EN_MASK, 0 << CMB_MIPI_UNPACK_EN);
}

void cmb_port_mipi_yuv_seq(unsigned int sel, enum cmb_mipi_yuv_seq seq)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_CFG_REG_OFF,
		CMB_MIPI_YUV_SEQ_MASK, seq << CMB_MIPI_YUV_SEQ);
}

void cmb_port_mipi_ph_bitord(unsigned int sel, unsigned int order)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_CFG_REG_OFF,
		CMB_MIPI_PH_BITOED_MASK, order << CMB_MIPI_PH_BITOED);
}

void cmb_port_mipi_cfg(unsigned int sel, enum cmb_mipi_yuv_seq seq)
{
	cmb_port_mipi_unpack_enable(sel);
	cmb_port_mipi_ph_bitord(sel, 0);
	cmb_port_mipi_yuv_seq(sel, seq);
}

void cmb_port_set_mipi_datatype(unsigned int sel, struct combo_csi_cfg *combo_csi_cfg)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH0_DT_MASK, combo_csi_cfg->mipi_datatype[0] << CMB_MIPI_CH0_DT);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH0_VC_MASK, combo_csi_cfg->vc[0] << CMB_MIPI_CH0_VC);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH1_DT_MASK, combo_csi_cfg->mipi_datatype[1] << CMB_MIPI_CH1_DT);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH1_VC_MASK, combo_csi_cfg->vc[1] << CMB_MIPI_CH1_VC);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH2_DT_MASK, combo_csi_cfg->mipi_datatype[2] << CMB_MIPI_CH2_DT);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH2_VC_MASK, combo_csi_cfg->vc[2] << CMB_MIPI_CH2_VC);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH3_DT_MASK, combo_csi_cfg->mipi_datatype[3] << CMB_MIPI_CH3_DT);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_REG_OFF,
		CMB_MIPI_CH3_VC_MASK, combo_csi_cfg->vc[3] << CMB_MIPI_CH3_VC);
}

void cmb_port_mipi_ch_trigger_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_FS_MASK, en << CMB_MIPI_FS);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_FE_MASK, en << CMB_MIPI_FE);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_LS_MASK, en << CMB_MIPI_LS);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_LE_MASK, en << CMB_MIPI_LE);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_YUV_MASK, en << CMB_MIPI_YUV);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_RGB_MASK, en << CMB_MIPI_RGB);
	vin_reg_clr_set(cmb_csi_port_base_addr[sel] + CMB_PORT_MIPI_DI_TRIG_REG_OFF,
		CMB_MIPI_RAW_MASK, en << CMB_MIPI_RAW);
}

void cmb_port_set_mipi_wdr(unsigned int sel, unsigned int mode, unsigned int ch)
{
	cmb_port_set_wdr_mode(sel, 2);
	cmb_port_set_fid_ch_map(sel, ch);
	cmb_port_set_fid_mode(sel, mode);
	cmb_port_out_num(sel, ONE_DATA);
	cmb_port_out_chnum(sel, ch-1);
}


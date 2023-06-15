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
#include "../protocol.h"

#ifndef __COMBO_CSI_REG__H__
#define __COMBO_CSI_REG__H__

#define MAX_MIPI_CH 4
#define MAX_LANE_NUM 12

enum phy_laneck_en {
	CK_1LANE = 0x1,
	CK_2LANE = 0x3,
};

enum phy_lanedt_en {
	DT_1LANE = 0x1,
	DT_2LANE = 0x3,
	DT_3LANE = 0x7,
	DT_4LANE = 0xf,
};

enum phy_mipi_lpck_en {
	LPCK_CLOSE = 0x0, /*sub-lvds/hispi*/
	LPCK_1LANE = 0x1, /*mipi*/
	LPCK_2LANE = 0x3,
};

enum phy_mipi_lpdt_en {
	LPDT_CLOSE = 0x0, /*sub-lvds/hispi*/
	LPDT_1LANE = 0x1,/*mipi*/
	LPDT_2LANE = 0x3,
	LPDT_3LANE = 0x7,
	LPDT_4LANE = 0xf,
};

enum phy_deskew_period_en {
	DK_PR_1LANE = 0x1,
	DK_PR_2LANE = 0x3,
	DK_PR_3LANE = 0x7,
	DK_PR_4LANE = 0xf,
};

enum phy_deskew_en {
	DK_1LANE = 0x1,
	DK_2LANE = 0x3,
	DK_3LANE = 0x7,
	DK_4LANE = 0xf,
};

enum phy_termck_en {
	TERMCK_CLOSE = 0x0, /*mipi*/
	TERMCK_1LANE = 0x1, /*sub-lvds/hispi*/
	TERMCK_2LANE = 0x3,
};

enum phy_termdt_en {
	TERMDT_CLOSE = 0x0, /*mipi*/
	TERMDT_1LANE = 0x1, /*sub-lvds/hispi*/
	TERMDT_2LANE = 0x3,
	TERMDT_3LANE = 0x7,
	TERMDT_4LANE = 0xf,
};

enum phy_s2p_en {
	S2PDT_CLOSE = 0x0, /*mipi*/
	S2PDT_1LANE = 0x1, /*sub-lvds/hispi*/
	S2PDT_2LANE = 0x3,
	S2PDT_3LANE = 0x7,
	S2PDT_4LANE = 0xf,
};

enum phy_hsck_en {
	HSCK_CLOSE = 0x0, /*mipi*/
	HSCK_1LANE = 0x1, /*sub-lvds/hispi*/
	HSCK_2LANE = 0x3,
};

enum phy_hsdt_en {
	HSDT_CLOSE = 0x0, /*mipi*/
	HSDT_1LANE = 0x1, /*sub-lvds/hispi*/
	HSDT_2LANE = 0x3,
	HSDT_3LANE = 0x7,
	HSDT_4LANE = 0xf,
};

enum cmb_csi_pix_num {
	ONE_DATA    = 0x0,
	TWO_DATA    = 0x1,
};

enum cmb_mipi_yuv_seq {
	YUYV = 0x0,
	YVYU = 0x1,
	UYVY = 0x2,
	VYUY = 0x3,
};

struct phy_lane_cfg {
	enum phy_laneck_en phy_laneck_en;
	enum phy_lanedt_en phy_lanedt_en;
	enum phy_mipi_lpck_en phy_mipi_lpck_en;
	enum phy_mipi_lpdt_en phy_mipi_lpdt_en;
	enum phy_deskew_period_en phy_deskew_period_en;
	enum phy_deskew_en phy_deskew_en;
	enum phy_termck_en phy_termck_en;
	enum phy_termdt_en phy_termdt_en;
	enum phy_s2p_en phy_s2p_en;
	enum phy_hsck_en phy_hsck_en;
	enum phy_hsdt_en phy_hsdt_en;
};

struct combo_csi_cfg {
	struct phy_lane_cfg phy_lane_cfg;
	enum pkt_fmt mipi_datatype[MAX_MIPI_CH];
	unsigned char mipi_lane[MAX_LANE_NUM];
	unsigned int vc[MAX_MIPI_CH];
	unsigned int lane_num;
	unsigned int total_rx_ch;
};

int cmb_csi_set_top_base_addr(unsigned long addr);
int cmb_csi_set_phy_base_addr(unsigned int sel, unsigned long addr);
int cmb_csi_set_port_base_addr(unsigned int sel, unsigned long addr);

/*
 * Detail function information of registers----PHY TOP
 */
void cmb_phy_top_enable(void);
void cmb_phy_top_disable(void);

/*
 * Detail function information of registers----PHYA/B
 */
void cmb_phy0_en(unsigned int sel, unsigned int en);
void cmb_phy_lane_num_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_phy0_work_mode(unsigned int sel, unsigned int mode);
void cmb_phy0_ofscal_cfg(unsigned int sel);
void cmb_phy_deskew_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_term_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_hs_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_s2p_ctl(unsigned int sel, unsigned int dly, struct phy_lane_cfg phy_lane_cfg);
void cmb_mipirx_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_phy0_s2p_dly(unsigned int sel, unsigned int dly);

/*
 * Detail function information of registers----PORT0/1
 */
void cmb_port_enable(unsigned int sel);
void cmb_port_disable(unsigned int sel);
void cmb_port_lane_num(unsigned int sel, unsigned int num);
void cmb_port_out_num(unsigned int sel, enum cmb_csi_pix_num cmb_csi_pix_num);
void cmb_port_out_chnum(unsigned int sel, unsigned int chnum);
unsigned char cmb_port_set_lane_map(unsigned int phy, unsigned int ch);
void cmb_port_lane_map(unsigned int sel, unsigned char *mipi_lane);
void cmb_port_mipi_cfg(unsigned int sel, enum cmb_mipi_yuv_seq seq);
void cmb_port_set_mipi_datatype(unsigned int sel, struct combo_csi_cfg *combo_csi_cfg);
void cmb_port_mipi_ch_trigger_en(unsigned int sel, unsigned int en);
void cmb_port_set_mipi_wdr(unsigned int sel, unsigned int mode, unsigned int ch);

#endif /*__COMBO_CSI_REG__H__*/

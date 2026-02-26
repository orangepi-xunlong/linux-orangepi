/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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
#include "../../utility/bsp_common.h"

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
	LPCK_CLOSE = 0x0, /* sub-lvds/hispi */
	LPCK_1LANE = 0x1, /* mipi */
	LPCK_2LANE = 0x3,
};

enum phy_mipi_lpdt_en {
	LPDT_CLOSE = 0x0, /* sub-lvds/hispi */
	LPDT_1LANE = 0x1,/* mipi */
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
	TERMCK_CLOSE = 0x0, /* mipi */
	TERMCK_1LANE = 0x1, /* sub-lvds/hispi */
	TERMCK_2LANE = 0x3,
};

enum phy_termdt_en {
	TERMDT_CLOSE = 0x0, /* mipi */
	TERMDT_1LANE = 0x1, /* sub-lvds/hispi */
	TERMDT_2LANE = 0x3,
	TERMDT_3LANE = 0x7,
	TERMDT_4LANE = 0xf,
};

enum phy_s2p_en {
	S2PDT_CLOSE = 0x0, /* mipi */
	S2PDT_1LANE = 0x1, /* sub-lvds/hispi */
	S2PDT_2LANE = 0x3,
	S2PDT_3LANE = 0x7,
	S2PDT_4LANE = 0xf,
};

enum phy_hsck_en {
	HSCK_CLOSE = 0x0, /* mipi */
	HSCK_1LANE = 0x1, /* sub-lvds/hispi */
	HSCK_2LANE = 0x3,
};

enum phy_hsdt_en {
	HSDT_CLOSE = 0x0, /* mipi */
	HSDT_1LANE = 0x1, /* sub-lvds/hispi */
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

enum phy_link_mode {
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	TWO_1LANE = 0x0, /* 2x1lane, default */
	ONE_2LANE = 0x1, /* 1x2lane */
#elif IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	TWO_4LANE_ONE_2LANE = 0x0, /*2x4-lane(phya+phyb) + 1x2-lane)*/
#else /* CONFIG_ARCH_SUN55IW3 CONFIG_ARCH_SUN55IW6 */
	FOUR_2LANE = 0x0, /*4x2lane, 4x1lane, default*/
	ONE_4LANE_PHYA = 0x1, /*2x2lane+1x4lane(phya+phyb)*/
	ONE_4LANE_PHYC = 0x2, /*2x2lane+1x4lane(phyc+phyd)*/
	TWO_4LANE = 0x3, /*2x4lane(phya+phyb, phyc+phyd)*/
	TWO_4LANE_ONE_2LANE = 0x4, /*2x4lane+1x2lane*/
	LANE_2 = 0xfffe,
	LANE_4 = 0xffff,
#endif
};

/*
 * mipi interrupt select
 */
enum mipi_int_sel {
	MIPI_PH_SYNC = 0x1,
	MIPI_PF_SYNC = 0x2,
	MIPI_FRAME_START_SYNC = 0x4,
	MIPI_FRAME_END_SYNC = 0x8,
	MIPI_LINE_START_SYNC = 0x10,
	MIPI_LINE_END_SYNC = 0x20,
	MIPI_EMBED_SYNC = 0x40,
	MIPI_FRAME_SYNC_ERR = 0x80,
	MIPI_LINE_SYNC_ERR = 0x100,
	MIPI_ECC_WRN = 0x200,
	MIPI_ECC_ERR = 0x400,
	MIPI_CHKSUM_ERR = 0x800,
	MIPI_EOT_ERR = 0x1000,
	MIPI_INT_ALL = 0x1fff,
};

struct mipi_int_status {
	bool ph_sync;
	bool pf_sync;
	bool frame_start_sync;
	bool frame_end_sync;
	bool line_start_sync;
	bool line_end_sync;
	bool embed_sync;
	bool frame_sync_err;
	bool line_sync_err;
	bool ecc_warn;
	bool ecc_err;
	bool checksum_err;
	bool eot_err;
	unsigned int mask;
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
	enum phy_link_mode phy_link_mode;
	enum pkt_fmt mipi_datatype[MAX_MIPI_CH];
	enum v4l2_field field[MAX_MIPI_CH];
	unsigned char mipi_lane[MAX_LANE_NUM];
	unsigned int vc[MAX_MIPI_CH];
	unsigned int lane_num;
	unsigned int total_rx_ch;
	enum cmb_csi_pix_num pix_num;
};

int cmb_csi_set_top_base_addr(unsigned long addr);
int cmb_csi_set_phy_base_addr(unsigned int sel, unsigned long addr);
int cmb_csi_set_port_base_addr(unsigned int sel, unsigned long addr);

/*
 * Detail function information of registers----PHY TOP
 */
void cmb_phy_top_enable(void);
void cmb_phy_top_disable(void);
void cmb_phy_link_mode_set(enum phy_link_mode phy_link_mode);
void cmb_phy_link_mode_get(int *cur_phy_link);

/*
 * Detail function information of registers----PHYA/B
 */
void cmb_phy0_en(unsigned int sel, unsigned int en);
unsigned int cmb_phy_en_status_get(unsigned int sel);
unsigned int cmb_phy_lanedt_en_status_get(unsigned int sel);
unsigned int cmb_phy_laneck_en_status_get(unsigned int sel);
void cmb_phy_lane_num_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_phy0_work_mode(unsigned int sel, unsigned int mode);
void cmb_phy0_ofscal_cfg(unsigned int sel);
void cmb_phy_deskew_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_term_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_phy_deskew1_cfg(unsigned int sel, unsigned int deskew, bool deskew_lane_cfg);
unsigned int cmb_phy_deskew1_cfg_get(unsigned int sel);
void cmb_hs_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_s2p_ctl(unsigned int sel, unsigned int dly, struct phy_lane_cfg phy_lane_cfg);
void cmb_mipirx_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg);
void cmb_phy0_s2p_dly(unsigned int sel, unsigned int dly);
unsigned int cmb_phy0_s2p_dly_get(unsigned int sel);
void cmb_phy0_freq_en(unsigned int sel, unsigned int en);
unsigned int cmb_phy_freq_cnt_get(unsigned int sel);
unsigned int cmb_phy_sta_d0_get(unsigned int sel);
unsigned int cmb_phy_sta_d1_get(unsigned int sel);
unsigned int cmb_phy_sta_ck0_get(unsigned int sel);

/*
 * Detail function information of registers----PORT0/1
 */
void cmb_port_enable(unsigned int sel);
void cmb_port_disable(unsigned int sel);
unsigned int cmb_port_en_status_get(unsigned int sel);
void cmb_port_lane_num(unsigned int sel, unsigned int num);
unsigned int cmb_port_lane_num_get(unsigned int sel);
void cmb_port_clr_ch0_int_status(unsigned int sel);
unsigned int cmb_port_check_ch0_int_status(unsigned int sel, unsigned int flag);
unsigned int cmb_port_int_status_get(unsigned int sel, unsigned int ch,
			struct mipi_int_status *status);
void cmb_port_out_num(unsigned int sel, enum cmb_csi_pix_num cmb_csi_pix_num);
unsigned int cmb_port_out_num_get(unsigned int sel);
void cmb_port_out_chnum(unsigned int sel, unsigned int chnum);
unsigned int cmb_port_channel_num_get(unsigned int sel);
unsigned char cmb_port_set_lane_map(unsigned int phy, unsigned int ch);
void cmb_port_lane_map(unsigned int sel, unsigned char *mipi_lane);
void cmb_port_mipi_cfg(unsigned int sel, enum cmb_mipi_yuv_seq seq);
unsigned int cmb_port_mipi_unpack_en_status_get(unsigned int sel);
unsigned int cmb_port_mipi_yuv_seq_get(unsigned int sel);
void cmb_port_set_mipi_datatype(unsigned int sel, struct combo_csi_cfg *combo_csi_cfg);
void cmb_port_mipi_ch_trigger_en(unsigned int sel, unsigned int ch, unsigned int en);
void cmb_port_mipi_set_field(unsigned int sel, unsigned int total_rx_ch,
			struct combo_csi_cfg *fmt);
void cmb_port_mipi_set_ch_field(unsigned int sel, unsigned int ch,
		       enum source_type src_type);
unsigned int cmb_port_mipi_ch_field_get(unsigned int sel, unsigned int ch);
void cmb_port_mipi_raw_extend_en(unsigned int sel, unsigned int ch, unsigned int en);
void cmb_port_set_mipi_wdr(unsigned int sel, unsigned int mode, unsigned int ch);
unsigned int cmb_port_wdr_mode_get(unsigned int sel);
unsigned int cmb_port_mipi_ch_cur_dt_get(unsigned int sel, unsigned int ch);
unsigned int cmb_port_mipi_ch_cur_vc_get(unsigned int sel, unsigned int ch);

#endif /* __COMBO_CSI_REG__H__ */

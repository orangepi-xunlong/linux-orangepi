/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DSI_V1_H_
#define __DSI_V1_H_

#include <linux/version.h>
#include <drm/drm_mipi_dsi.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#include <drm/display/drm_dsc_helper.h>
#else
#include <drm/drm_dsc.h>
#endif

#include "dsi_v1_type.h"
#include "include.h"
#include "dsc_type.h"

#define MIPI_DSI_MODE_COMMAND		BIT(20)
#define DEVICE_DSI_NUM			2

extern u32 dsi_bits_per_pixel[4];

struct sunxi_dsi_lcd {
	int dsi_index;
	volatile struct dsi_lcd_reg *reg;
	volatile struct dsc_dsi_reg *dsc_reg;
};

enum disp_lcd_frm {
	LCD_FRM_BYPASS = 0,
	LCD_FRM_RGB666 = 1,
	LCD_FRM_RGB565 = 2,
};
enum disp_lcd_te {
	LCD_TE_DISABLE = 0,
	LCD_TE_RISING = 1,
	LCD_TE_FALLING = 2,
};
enum disp_lcd_tcon_mode {
	DISP_TCON_NORMAL_MODE = 0,
	DISP_TCON_MASTER_SYNC_AT_FIRST_TIME,
	DISP_TCON_MASTER_SYNC_EVERY_FRAME,
	DISP_TCON_SLAVE_MODE,
	DISP_TCON_DUAL_DSI,
};
struct disp_dsi_para {
	unsigned int channel;
	unsigned int lanes;
	unsigned int dual_dsi;
	unsigned int dsi_div;
	unsigned int vrr_setp;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	unsigned long hs_rate;
	unsigned long lp_rate;
	enum disp_lcd_tcon_mode  lcd_tcon_mode;
	enum disp_lcd_frm lcd_frm;
	enum disp_lcd_te lcd_dsi_te;
	struct disp_video_timings timings;
};
enum __dsi_irq_id_t {
	DSI_IRQ_VIDEO_LINE = 3,
	DSI_IRQ_VIDEO_VBLK = 2,
	DSI_IRQ_INSTR_STEP = 1,
	DSI_IRQ_INSTR_END = 0,
};
enum __dsi_dt_t {
	/*
	 * Processor to Peripheral Direction (Processor-Sourced)
	 * Packet Data Types
	 */
	DSI_DT_VSS = 0x01,
	DSI_DT_VSE = 0x11,
	DSI_DT_HSS = 0x21,
	DSI_DT_HSE = 0x31,
	DSI_DT_EOT = 0x08,
	DSI_DT_CM_OFF = 0x02,
	DSI_DT_CM_ON = 0x12,
	DSI_DT_SHUT_DOWN = 0x22,
	DSI_DT_TURN_ON = 0x32,
	DSI_DT_GEN_WR_P0 = 0x03,
	DSI_DT_GEN_WR_P1 = 0x13,
	DSI_DT_GEN_WR_P2 = 0x23,
	DSI_DT_GEN_RD_P0 = 0x04,
	DSI_DT_GEN_RD_P1 = 0x14,
	DSI_DT_GEN_RD_P2 = 0x24,
	DSI_DT_DCS_WR_P0 = 0x05,
	DSI_DT_DCS_WR_P1 = 0x15,
	DSI_DT_DCS_RD_P0 = 0x06,
	DSI_DT_MAX_RET_SIZE = 0x37,
	DSI_DT_NULL = 0x09,
	DSI_DT_BLK = 0x19,
	DSI_DT_GEN_LONG_WR = 0x29,
	DSI_DT_DCS_LONG_WR = 0x39,
	DSI_DT_PIXEL_RGB565 = 0x0E,
	DSI_DT_PIXEL_RGB666P = 0x1E,
	DSI_DT_PIXEL_RGB666 = 0x2E,
	DSI_DT_PIXEL_RGB888 = 0x3E,

	/* Data Types for Peripheral-sourced Packets */
	DSI_DT_ACK_ERR = 0x02,
	DSI_DT_EOT_PERI = 0x08,
	DSI_DT_GEN_RD_R1 = 0x11,
	DSI_DT_GEN_RD_R2 = 0x12,
	DSI_DT_GEN_LONG_RD_R = 0x1A,
	DSI_DT_DCS_LONG_RD_R = 0x1C,
	DSI_DT_DCS_RD_R1 = 0x21,
	DSI_DT_DCS_RD_R2 = 0x22,
};

enum __dsi_dcs_t {
	DSI_DCS_ENTER_IDLE_MODE = 0x39,
	DSI_DCS_ENTER_INVERT_MODE = 0x21,
	DSI_DCS_ENTER_NORMAL_MODE = 0x13,
	DSI_DCS_ENTER_PARTIAL_MODE = 0x12,
	DSI_DCS_ENTER_SLEEP_MODE = 0x10,
	DSI_DCS_EXIT_IDLE_MODE = 0x38,
	DSI_DCS_EXIT_INVERT_MODE = 0x20,
	DSI_DCS_EXIT_SLEEP_MODE = 0x11,
	DSI_DCS_GET_ADDRESS_MODE = 0x0b,
	DSI_DCS_GET_BLUE_CHANNEL = 0x08,
	DSI_DCS_GET_DIAGNOSTIC_RESULT = 0x0f,
	DSI_DCS_GET_DISPLAY_MODE = 0x0d,
	DSI_DCS_GET_GREEN_CHANNEL = 0x07,
	DSI_DCS_GET_PIXEL_FORMAT = 0x0c,
	DSI_DCS_GET_POWER_MODE = 0x0a,
	DSI_DCS_GET_RED_CHANNEL = 0x06,
	DSI_DCS_GET_SCANLINE = 0x45,
	DSI_DCS_GET_SIGNAL_MODE = 0x0e,
	DSI_DCS_NOP = 0x00,
	DSI_DCS_READ_DDB_CONTINUE = 0xa8,
	DSI_DCS_READ_DDB_START = 0xa1,
	DSI_DCS_READ_MEMORY_CONTINUE = 0x3e,
	DSI_DCS_READ_MEMORY_START = 0x2e,
	DSI_DCS_SET_ADDRESS_MODE = 0x36,
	DSI_DCS_SET_COLUMN_ADDRESS = 0x2a,
	DSI_DCS_SET_DISPLAY_OFF = 0x28,
	DSI_DCS_SET_DISPLAY_ON = 0x29,
	DSI_DCS_SET_GAMMA_CURVE = 0x26,
	DSI_DCS_SET_PAGE_ADDRESS = 0x2b,
	DSI_DCS_SET_PARTIAL_AREA = 0x30,
	DSI_DCS_SET_PIXEL_FORMAT = 0x3a,
	DSI_DCS_SET_SCROLL_AREA = 0x33,
	DSI_DCS_SET_SCROLL_START = 0x37,
	DSI_DCS_SET_TEAR_OFF = 0x34,
	DSI_DCS_SET_TEAR_ON = 0x35,
	DSI_DCS_SET_TEAR_SCANLINE = 0x44,
	DSI_DCS_SOFT_RESET = 0x01,
	DSI_DCS_WRITE_LUT = 0x2d,
	DSI_DCS_WRITE_MEMORY_CONTINUE = 0x3c,
	DSI_DCS_WRITE_MEMORY_START = 0x2c,
};

enum __dsi_start_t {
	DSI_START_LP11 = 0,
	DSI_START_TBA = 1,
	DSI_START_HSTX = 2,
	DSI_START_LPTX = 3,
	DSI_START_LPRX = 4,
	DSI_START_HSC = 5,
	DSI_START_HSD = 6,
	DSI_START_HSD_DS = 7,
	DSI_START_HSTX_CLK_BREAK = 8,
	DSI_START_HSTX_TEST = 9,
	DSI_INST_TEST = 10,
};

enum __dsi_inst_id_t {
	DSI_INST_ID_LP11 = 0,
	DSI_INST_ID_TBA = 1,
	DSI_INST_ID_HSC = 2,
	DSI_INST_ID_HSD = 3,
	DSI_INST_ID_LPDT = 4,
	DSI_INST_ID_HSCEXIT = 5,
	DSI_INST_ID_NOP = 6,
	DSI_INST_ID_DLY = 7,
	DSI_INST_ID_END = 15,
};
enum __dsi_inst_id_t_1 {
	DSI_INST_ID_LP11_1 = 8,
	DSI_INST_ID_HSC_1 = 9,
	DSI_INST_ID_DS_1 = 10,
	DSI_INST_ID_LPDT_1 = 11,
	DSI_INST_ID_HSCEXIT_1 = 12,
	DSI_INST_ID_NOP_1 = 13,
	DSI_INST_ID_DLY_1 = 14,
};
enum __dsi_inst_id_t_test {
	DSI_INST_ID_LP11_00 = 0,
	DSI_INST_ID_LP11_01 = 1,
	DSI_INST_ID_HSC_00 = 2,
	DSI_INST_ID_HSD_00 = 3,
	DSI_INST_ID_DLY_00 = 4,
	DSI_INST_ID_HSCEXIT_00 = 5,
	DSI_INST_ID_NOP_00 = 6,
	DSI_INST_ID_DLY_01 = 7,
};

enum __dsi_inst_id_t_test1 {
	DSI_INST_ID_LP11_02 = 8,
	DSI_INST_ID_NOP_01 = 9,
	DSI_INST_ID_NOP_02 = 10,
	DSI_INST_ID_DLY_02 = 11,
	DSI_INST_ID_LP11_03 = 12,
	DSI_INST_ID_NOP_03 = 13,
	DSI_INST_ID_DLY_03 = 14,
};
enum __dsi_inst_mode_t {
	DSI_INST_MODE_STOP = 0,
	DSI_INST_MODE_TBA = 1,
	DSI_INST_MODE_HS = 2,
	DSI_INST_MODE_ESCAPE = 3,
	DSI_INST_MODE_HSCEXIT = 4,
	DSI_INST_MODE_NOP = 5,
	DSI_INST_MODE_SCINIT = 6,
	DSI_INST_MODE_PERI = 7,
};

enum __dsi_inst_escape_t {
	DSI_INST_ESCA_LPDT = 0,
	DSI_INST_ESCA_ULPS = 1,
	DSI_INST_ESCA_UN1 = 2,
	DSI_INST_ESCA_UN2 = 3,
	DSI_INST_ESCA_RESET = 4,
	DSI_INST_ESCA_UN3 = 5,
	DSI_INST_ESCA_UN4 = 6,
	DSI_INST_ESCA_UN5 = 7,
};

enum __dsi_inst_packet_t {
	DSI_INST_PACK_PIXEL = 0,
	DSI_INST_PACK_COMMAND = 1,
};

/* video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/* video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/* video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/* enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/* disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_NO_HFP	BIT(5)
/* disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_NO_HBP	BIT(6)
/* disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_NO_HSA	BIT(7)
/* flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/* disable EoT packets in HS mode */
#define MIPI_DSI_MODE_NO_EOT_PACKET	BIT(9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/* transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

#define MIPI_DSI_EN_3DFIFO		BIT(21)
#define MIPI_DSI_SLAVE_MODE		BIT(22)

s32 dsi_open_hs_mode(struct sunxi_dsi_lcd *dsi, struct disp_dsi_para *dsi_para);
s32 dsi_dcs_wr(struct sunxi_dsi_lcd *dsi, u8 *para_p, u32 para_num);
s32 dsi_dcs_rd(struct sunxi_dsi_lcd *dsi, u8 *para_p, u32 num_p);
void dsi_enable_vblank(struct sunxi_dsi_lcd *dsi, bool enable);
s32 dsi_set_reg_base(struct sunxi_dsi_lcd *dsi, uintptr_t base);
u32 dsi_get_reg_base(struct sunxi_dsi_lcd *dsi);
u32 dsi_irq_query(struct sunxi_dsi_lcd *dsi, enum __dsi_irq_id_t id);
s32 dsi_cfg(struct sunxi_dsi_lcd *dsi, struct disp_dsi_para *dsi_para);
s32 dsi_close(struct sunxi_dsi_lcd *dsi);
s32 dsi_inst_busy(struct sunxi_dsi_lcd *dsi);
s32 dsi_tri_start(struct sunxi_dsi_lcd *dsi);
u32 dsi_get_start_delay(struct sunxi_dsi_lcd *dsi);
u32 dsi_get_cur_line(struct sunxi_dsi_lcd *dsi);
u32 dsi_get_real_cur_line(struct sunxi_dsi_lcd *dsi);
s32 dsi_irq_enable(struct sunxi_dsi_lcd *dsi, enum __dsi_irq_id_t id);
s32 dsi_irq_disable(struct sunxi_dsi_lcd *dsi, enum __dsi_irq_id_t id);
s32 dsi_dcs_rd_memory(struct sunxi_dsi_lcd *dsi, u32 *p_data, u32 length);
s32 dsi_set_max_ret_size(struct sunxi_dsi_lcd *dsi, u32 size);
u8 dsi_ecc_pro(u32 dsi_ph);
u16 dsi_crc_pro_pd_repeat(u8 pd, u32 pd_bytes);
u16 dsi_crc_pro(u8 *pd_p, u32 pd_bytes);
s32 dsi_mode_switch(struct sunxi_dsi_lcd *dsi, __u32 cmd_en, __u32 lp_en);
s32 dsi_get_status(struct sunxi_dsi_lcd *dsi);
s32 dsi_get_fifo_under_flow(struct sunxi_dsi_lcd *dsi);
void sunxi_dsi_vrr_irq(struct sunxi_dsi_lcd *dsi, struct disp_video_timings *timings, bool enable);
void sunxi_dsi_vfp_vrr_irq(struct sunxi_dsi_lcd *dsi, struct disp_video_timings *timings);
int sunxi_dsi_updata_vt(struct sunxi_dsi_lcd *dsi, struct disp_video_timings *timings,
				u32 vrr_setp);
int sunxi_dsi_updata_vt_2(struct sunxi_dsi_lcd *dsi, struct disp_video_timings *timings);
s32 dsc_set_reg_base(struct sunxi_dsi_lcd *dsi, uintptr_t base);
void dsc_config_pps(struct sunxi_dsi_lcd *dsi, const struct drm_dsc_config *dsc_cfg);
void dec_dsc_config(struct sunxi_dsi_lcd *dsi, struct disp_video_timings *timings);
void dsi_read_mode_en(struct sunxi_dsi_lcd *dsi, u32 en);
s32 dsi_get_timing(struct sunxi_dsi_lcd *dsi, struct disp_video_timings *tt);

#endif

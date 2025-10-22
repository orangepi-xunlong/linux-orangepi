/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller DBI Register Definition
 *
 */

#ifndef _SUNXI_SPI_DBI_API_H_
#define _SUNXI_SPI_DBI_API_H_

#include <linux/ctype.h>
#include <linux/spi/spi.h>

#define SUNXI_DBI_COMMAND_READ		(0x10)
#define SUNXI_DBI_LSB_FIRST			(0x20)
#define SUNXI_DBI_TRANSMIT_VIDEO	(0x40)
#define SUNXI_DBI_DCX_DATA			(0x80)

#define SUNXI_DBI_CLK_AUTO_GATING	(0)
#define SUNXI_DBI_CLK_ALWAYS_ON		(1)

enum sunxi_dbi_out_seq {
	SUNXI_DBI_OUT_RGB = 0,
	SUNXI_DBI_OUT_RBG = 1,
	SUNXI_DBI_OUT_GRB = 2,
	SUNXI_DBI_OUT_GBR = 3,
	SUNXI_DBI_OUT_BRG = 4,
	SUNXI_DBI_OUT_BGR = 5,
};

enum sunxi_dbi_out_fmt {
	SUNXI_DBI_FMT_RGB111 = 0,
	SUNXI_DBI_FMT_RGB444 = 1,
	SUNXI_DBI_FMT_RGB565 = 2,
	SUNXI_DBI_FMT_RGB666 = 3,
	SUNXI_DBI_FMT_RGB888 = 4,	/* only for 2 Data Lane Interface */
};

enum sunxi_dbi_interface {
	SUNXI_DBI_INTF_L3I1 = 0,	/* 3 Line Interface I */
	SUNXI_DBI_INTF_L3I2 = 1,	/* 3 Line Interface II */
	SUNXI_DBI_INTF_L4I1 = 2,	/* 4 Line Interface I */
	SUNXI_DBI_INTF_L4I2 = 3,	/* 4 Line Interface II */
	SUNXI_DBI_INTF_D2LI = 4,	/* 2 Data Lane Interface */
};

enum sunxi_dbi_src_seq {
	/* When Video Source_Type is RGB32 */
	SUNXI_DBI_SRC_RGB = 0,
	SUNXI_DBI_SRC_RBG = 1,
	SUNXI_DBI_SRC_GRB = 2,
	SUNXI_DBI_SRC_GBR = 3,
	SUNXI_DBI_SRC_BRG = 4,
	SUNXI_DBI_SRC_BGR = 5,
	/* When Video Source_Type is RGB16 */
	SUNXI_DBI_SRC_GRBG_0 = 6,	/* G[5:3] R[4:0] B[4:0] G[2:0] */
	SUNXI_DBI_SRC_GRBG_1 = 7,	/* G[5:3] B[4:0] R[4:0] G[2:0] */
	SUNXI_DBI_SRC_GBRG_0 = 8,	/* G[2:0] R[4:0] B[4:0] G[5:3] */
	SUNXI_DBI_SRC_GBRG_1 = 9,	/* G[2:0] B[4:0] R[4:0] G[5:3] */
};

enum sunxi_dbi_mode_sel {
	SUNXI_DBI_MODE_ALWAYS_ON = 0,
	SUNXI_DBI_MODE_SOFT_TRIG = 1,
	SUNXI_DBI_MODE_TIMER_TRIG = 2,
	SUNXI_DBI_MODE_TE_TRIG = 3,
};

enum sunxi_dbi_te_en {
	SUNXI_DBI_TE_DISABLE = 0,
	SUNXI_DBI_TE_RISING_EDGE = 1,
	SUNXI_DBI_TE_FALLING_EDGE = 2,
};

struct sunxi_dbi_config {
	enum sunxi_dbi_src_seq	dbi_src_sequence;
	enum sunxi_dbi_out_seq	dbi_out_sequence;
	enum sunxi_dbi_te_en dbi_te_en;
	bool dbi_rgb_bit_order;
	bool dbi_rgb32_alpha_pos;
	bool dbi_rgb16_pixel_endian;
	u8 dbi_format; /* DBI OUT format */
	u8 dbi_interface;
	u16 dbi_mode;
	u8 dbi_clk_out_mode;
	u16 dbi_video_v;
	u16 dbi_video_h;
	u8 dbi_fps;
	void (*dbi_vsync_handle)(unsigned long data);
	u8 dbi_read_bytes;
	u8 dbi_read_dummy;
	u8 dbi_cmd_data;
};

extern int sunxi_dbi_get_config(const struct spi_device *spi, struct sunxi_dbi_config *dbi_config);
extern int sunxi_dbi_set_config(struct spi_device *spi, const struct sunxi_dbi_config *dbi_config);

#endif /* _SUNXI_SPI_DBI_API_H_ */
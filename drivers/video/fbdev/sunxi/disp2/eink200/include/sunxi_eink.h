/*
 * Allwinner SoCs eink driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 *	File name   :       sunxi_eink.h
 *
 *	Description :       eink engine 2.0  struct declaration
 *
 *	History     :       2019/03/20 liuli   initial version
 *
 */
#ifndef _SUNXI_EINK_H_
#define _SUNXI_EINK_H_

//typedef unsigned int u32;
//typedef int s32;

enum EINK_CMD {
	EINK_UPDATE_IMG,
	EINK_WRITE_BACK_IMG,
	EINK_SET_TEMP,
	EINK_GET_TEMP,
	EINK_SET_GC_CNT,
	EINK_SELF_STANDBY,
	EINK_GET_FREE_BUF,
};

enum upd_mode {
	EINK_INIT_MODE = 0x01,
	EINK_DU_MODE = 0x02,
	EINK_GC16_MODE = 0x04,
	EINK_GC4_MODE = 0x08,
	EINK_A2_MODE = 0x10,
	EINK_GL16_MODE = 0x20,
	EINK_GLR16_MODE = 0x40,
	EINK_GLD16_MODE = 0x80,
	EINK_GU16_MODE	= 0x84,

	/* use self upd win not de*/
	EINK_RECT_MODE  = 0x400,
	/* AUTO MODE: auto select update mode by E-ink driver */
	EINK_AUTO_MODE = 0x8000,

/*	EINK_NO_MERGE = 0x80000000,*/
};

enum dither_mode {
	QUANTIZATION,
	FLOYD_STEINBERG,
	ATKINSON,
	ORDERED,
	SIERRA_LITE,
	BURKES,
};

struct upd_win {
	u32 left;
	u32 top;
	u32 right;
	u32 bottom;
};

enum upd_pixel_fmt {
	EINK_Y8 = 0x09,
	EINK_Y5 = 0x0a,
	EINK_Y4 = 0x0b,
	EINK_Y3 = 0x0e,
};

struct upd_pic_size {
	u32 width;
	u32 height;
	u32 align;
};

struct buf_slot {
	int count;
	int upd_order[32];
};

struct eink_img {
	int			fd;
	void			*paddr;
	u32			pitch;
	bool			win_calc_en;
	bool			upd_all_en;
	enum upd_mode           upd_mode;
	struct upd_win		upd_win;
	struct upd_pic_size	size;
	enum upd_pixel_fmt      out_fmt;
	enum dither_mode        dither_mode;
	unsigned int		*eink_hist;
};

struct eink_upd_cfg {
	u32			order;
	struct eink_img		img;
	bool			force_fresh;
};
#endif

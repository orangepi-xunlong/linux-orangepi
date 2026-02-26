/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef _DW_EDID_H
#define _DW_EDID_H

#define EDID_BLOCK_SIZE		    (128)

void dw_edid_reset_sink(void);

int dw_edid_read_extenal_block(int block, u8 *edid_buf);

int dw_edid_parse_info(u8 *data);

int dw_sink_support_hdmi20(void);

int dw_sink_support_only_yuv420(u32 vic);

int dw_sink_support_yuv420(u32 vic);

int dw_sink_support_yuv422(void);

int dw_sink_support_yuv444(void);

int dw_sink_support_rgb_dc(u8 bits);

int dw_sink_support_yuv444_dc(u8 bits);

int dw_sink_support_yuv422_dc(u8 bits);

int dw_sink_support_yuv420_dc(u8 bits);

int dw_sink_support_sdr(void);

int dw_sink_support_hdr10(void);

int dw_sink_support_hlg(void);

int dw_sink_support_max_tmdsclk(u32 clk);

int dw_sink_support_hdmi_vic(u32 code);

int dw_sink_support_cea_vic(u32 code);

bool dw_sink_support_scdc(void);

int dw_edid_exit(void);

int dw_edid_init(void);

ssize_t dw_sink_dump(char *buf);

#endif	/* _DW_EDID_H */

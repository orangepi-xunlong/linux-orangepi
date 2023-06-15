/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * CC08021801_310_800X1280 panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __CC08021801_310_800X1280_MIPI_PANEL_H__
#define __CC08021801_310_800X1280_MIPI_PANEL_H__

#include "panels.h"
#define sys_put_wvalue(n, c) (*((volatile __u32 *)(n))  = (c))   /* word output */
#define sys_get_wvalue(n)   (*((volatile __u32 *)(n)))          /* word input */

extern __s32 dsi_dcs_wr_0para(__u32 sel, __u8 cmd);
extern __s32 dsi_dcs_wr_1para(__u32 sel, __u8 cmd, __u8 para);
extern __s32 dsi_dcs_wr_2para(__u32 sel, __u8 cmd,
								__u8 para1, __u8 para2);
extern __s32 dsi_dcs_wr_3para(__u32 sel, __u8 cmd,
								__u8 para1, __u8 para2, __u8 para3);
extern __s32 dsi_dcs_wr_4para(__u32 sel, __u8 cmd, __u8 para1,
								__u8 para2, __u8 para3, __u8 para4);
extern __s32 dsi_dcs_wr_5para(__u32 sel, __u8 cmd, __u8 para1,
								__u8 para2, __u8 para3, __u8 para4,
								__u8 para5);
extern __s32 dsi_gen_wr_0para(__u32 sel, __u8 cmd);
extern __s32 dsi_gen_wr_1para(__u32 sel, __u8 cmd, __u8 para);
extern __s32 dsi_gen_wr_2para(__u32 sel, __u8 cmd, __u8 para1,
								__u8 para2);
extern __s32 dsi_gen_wr_3para(__u32 sel, __u8 cmd, __u8 para1,
								__u8 para2, __u8 para3);
extern __s32 dsi_gen_wr_4para(__u32 sel, __u8 cmd, __u8 para1,
								__u8 para2, __u8 para3, __u8 para4);
extern __s32 dsi_gen_wr_5para(__u32 sel, __u8 cmd, __u8 para1,
								__u8 para2, __u8 para3, __u8 para4,
								__u8 para5);

extern struct __lcd_panel CC08021801_310_800X1280_mipi_panel;
extern s32 bsp_disp_get_panel_info(u32 screen_id,
									struct disp_panel_para *info);

#endif /*End of file*/

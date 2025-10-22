/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef __SUNXI_LCD_H__
#define  __SUNXI_LCD_H__

/* displl */
#define CLK_PLL_DISPLL		0
#define CLK_DSI_LS		1
#define CLK_DSI_HS		2
#define CLK_LVDS_OR_RGB		3

/* dsi */
#define MIPI_DSI_MODE_VIDEO		1
#define MIPI_DSI_MODE_VIDEO_BURST	(1<<1)
#define MIPI_DSI_MODE_NO_EOT_PACKET	(1<<9)
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	(1<<10)
#define MIPI_DSI_EN_3DFIFO		(1<<21)
#define MIPI_DSI_SLAVE_MODE		(1<<22)
#define MIPI_DSI_MODE_VRR               (1<<24)
#define MIPI_DSI_SYNC_INCELL            (1<<25)
#define MIPI_DSI_ASYNC_INCELL           (1<<26)

/* lvds */
#define MEDIA_BUS_FMT_RGB666_1X7X3_SPWG         0x1010
#define MEDIA_BUS_FMT_RGB888_1X7X4_SPWG         0x1011
#define MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA        0x1012

#endif

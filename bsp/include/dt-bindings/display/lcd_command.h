/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef __LCD_COMMAND_H__
#define  __LCD_COMMAND_H__

		/* void (*p_func)(u32 sel) */
#define	SUNXI_LCD_TCON_ENABLE              0
#define	SUNXI_LCD_TCON_DISABLE             1
#define	SUNXI_LCD_BACKLIGHT_ENABLE         2
#define	SUNXI_LCD_BACKLIGHT_DISABLE        3

		/* s32 (*p_func)(u32 sel) */
#define	SUNXI_LCD_PWM_ENABLE               10
#define	SUNXI_LCD_PWM_DISABLE              11
#define	SUNXI_LCD_CPU_SET_AUTO_MODE        12
#define	SUNXI_LCD_DSI_CLK_ENABLE           13
#define	SUNXI_LCD_DSI_CLK_DISABLE          14

		/* void (*p_func)(u32 sel, u32, u32) */
#define	SUNXI_LCD_DSI_MODE_SWITCH          20

		/* s32 (*p_func)(u32) */
#define	SUNXI_LCD_DELAY_MS                 30
#define	SUNXI_LCD_DELAY_US                 31

		/* void (*p_func)(u32 sel, u32) */
#define	SUNXI_LCD_POWER_ENABLE             40
#define	SUNXI_LCD_POWER_DISABLE            41
#define	SUNXI_LCD_CPU_WRITE_INDEX          42
#define	SUNXI_LCD_CPU_WRITE_DATA           43
#define	SUNXI_LCD_DSI_SET_MAX_RET_SIZE     44
#define	SUNXI_LCD_PIN_CFG                  45

		/* s32 (*p_func)(u32 sel, u32, u32) */
#define	SUNXI_LCD_CPU_WRITE                50
#define	SUNXI_LCD_GPIO_SET_VALUE           51
#define	SUNXI_LCD_GPIO_SET_DIRECTION       52

		/* s32 (*p_func)(u32 sel, u8, u8 *, u32 *) */
#define	SUNXI_LCD_DSI_DCS_READ             80

		/* s32 (*p_func)(u32 sel, u8, u8 *, u32) */
#define	DSI_DCS_WR                         90
#define	SUNXI_LCD_DSI_DCS_WRITE            91
#define	SUNXI_LCD_DSI_GEN_WRITE            92


#endif

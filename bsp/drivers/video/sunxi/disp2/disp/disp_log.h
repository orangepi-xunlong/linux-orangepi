/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * disp_log.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: libairong <libairong@allwinnertech.com>
 *
 * DISP2 logger.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DISP_LOG_H__
#define __DISP_LOG_H__

#define DP printk("[DEBUG] %s, %s, %d \n", __FILE__, __func__, __LINE__);

#define HAL_XPOSTO(x)   "\033[" #x "D\033[" #x "C"

#define HAL_LOG_LAYOUT      "%s%s%s: [%s:%04u]: %s%s"
#define HAL_LOG_BACKEND_CALL(log_lv, log_color, log_format, color_off, ...) \
	pr_warn(HAL_LOG_LAYOUT log_format,                            \
		log_color, log_lv, color_off, __func__, __LINE__, HAL_XPOSTO(70),\
		log_color, ##__VA_ARGS__)

#define HAL_LOG_COLOR(log_lv, log_color, log_format, ...)                   \
	HAL_LOG_BACKEND_CALL(log_lv, log_color, log_format,                     \
				HAL_LOG_COLOR_OFF, ##__VA_ARGS__)

#define HAL_LOG_COLOR_OFF               "\033[0m"
#define HAL_LOG_COLOR_RED               "\033[1;40;31m"
#define HAL_LOG_COLOR_YELLOW            "\033[1;40;33m"
#define HAL_LOG_COLOR_BLUE              "\033[1;40;34m"
#define HAL_LOG_COLOR_PURPLE            "\033[1;40;35m"

#define HAL_LOG_ERROR_PREFIX            "[DISP ERR]"
#define HAL_LOG_WARNING_PREFIX          "[DISP WRN]"
#define HAL_LOG_INFO_PREFIX             "[DISP INF]"
#define HAL_LOG_DEBUG_PREFIX            "[DISP DBG]"

#define disp_log_err(...) \
	do { HAL_LOG_COLOR(HAL_LOG_ERROR_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while (0)
#define disp_log_warn(...) \
	do { HAL_LOG_COLOR(HAL_LOG_WARNING_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while (0)
#define disp_log_info(...) \
	do { HAL_LOG_COLOR(HAL_LOG_INFO_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while (0)
#define disp_log_debug(...) \
	do { HAL_LOG_COLOR(HAL_LOG_DEBUG_PREFIX, HAL_LOG_COLOR_OFF, ##__VA_ARGS__); } while (0)

#endif  /* __DISP_LOG_H__ */

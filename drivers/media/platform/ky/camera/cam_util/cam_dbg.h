/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cam_dbg.h - camera debug utility
 *
 * Copyright(C) 2023 KY Micro Limited
 */
#ifndef __CAM_DBG_H__
#define __CAM_DBG_H__

#include <linux/printk.h>

enum dbg_module_tag {
	CAM_MDL_VI = 0,
	CAM_MDL_ISP = 1,
	CAM_MDL_CPP = 2,
	CAM_MDL_VBE = 3,
	CAM_MDL_SNR = 4,
	CAM_MDL_IRCUT = 5,
	CAM_MDL_COMMON = 8,
};

#ifndef CAM_MODULE_TAG
#define CAM_MODULE_TAG CAM_MDL_COMMON
#endif

__printf(6, 7)
void cam_printk(int module_tag, const char *cam_level, const char *kern_level,
		const char *func, int line, const char *format, ...);

__printf(4, 5)
void cam_printk_ratelimited(int module_tag, const char *cam_level,
			    const char *kern_level, const char *format, ...);

__printf(5, 6)
void cam_debug(int module_tag, const char *cam_level, const char *func, int line, const char *format, ...);

/**
 * camera error output.
 *
 * @format: printf() like format string.
 */
#define cam_err(format, ...)                                       \
	cam_printk(CAM_MODULE_TAG, "cam_err", KERN_ERR,                \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * camera error output.
 *
 * @format: printf() like format string.
 */
#define cam_err_ratelimited(format, ...)                        \
	cam_printk_ratelimited(CAM_MODULE_TAG, "cam_err", KERN_ERR, \
			 format, ##__VA_ARGS__)

/**
 * camera warning output.
 *
 * @format: printf() like format string.
 */
#define cam_warn(format, ...)                                  \
	cam_printk(CAM_MODULE_TAG, "cam_wrn", KERN_WARNING,        \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * camera notice output.
 *
 * @format: printf() like format string.
 */
#define cam_not(format, ...)                                    \
	cam_printk(CAM_MODULE_TAG, "cam_not", KERN_NOTICE,          \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * camera information output.
 *
 * @format: printf() like format string.
 */
#define cam_info(format, ...)                                     \
	cam_printk(CAM_MODULE_TAG, "cam_inf", KERN_INFO,              \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * camera debug output.
 *
 * @format: printf() like format string.
 */
#define cam_dbg(format, ...)                                      \
	cam_debug(CAM_MODULE_TAG, "cam_dbg", __func__, __LINE__, format, ##__VA_ARGS__)

#define CAM_DBG_TRACE
#ifdef CAM_DBG_TRACE
#define cam_trace(f, args...)	trace_printk(f, ##args)
#else
#define cam_trace(f, args...)	no_printk(f, ##args)
#endif
#endif /* ifndef __CAM_DBG_H__ */

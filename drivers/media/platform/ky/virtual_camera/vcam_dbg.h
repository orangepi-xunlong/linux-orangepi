/* SPDX-License-Identifier: GPL-2.0 */
/*
 * vcam_dbg.h - vcamera debug utility
 *
 * Copyright(C) 2023 KY Micro Limited
 */
#ifndef __VCAM_DBG_H__
#define __VCAM_DBG_H__

#include <linux/printk.h>

// enum dbg_module_tag {
// 	VCAM_MDL_VI = 0,
// 	VCAM_MDL_ISP = 1,
// 	VCAM_MDL_CPP = 2,
// 	VCAM_MDL_VBE = 3,
// 	VCAM_MDL_SNR = 4,
// 	VCAM_MDL_IRCUT = 5,
// 	VCAM_MDL_COMMON = 8,
// };

#ifndef VCAM_MODULE_TAG
// #define VCAM_MODULE_TAG VCAM_MDL_COMMON
#define VCAM_MODULE_TAG 8
#endif

__printf(6, 7)
void vcam_printk(int module_tag, const char *vcam_level, const char *kern_level,
		const char *func, int line, const char *format, ...);

__printf(4, 5)
void vcam_printk_ratelimited(int module_tag, const char *vcam_level,
			    const char *kern_level, const char *format, ...);

__printf(5, 6)
void vcam_debug(int module_tag, const char *vcam_level, const char *func, int line, const char *format, ...);

/**
 * vcamera error output.
 *
 * @format: printf() like format string.
 */
#define vcam_err(format, ...)                                       \
	vcam_printk(VCAM_MODULE_TAG, "vcam_err", KERN_ERR,                \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * vcamera error output.
 *
 * @format: printf() like format string.
 */
#define vcam_err_ratelimited(format, ...)                        \
	vcam_printk_ratelimited(VCAM_MODULE_TAG, "vcam_err", KERN_ERR, \
			 format, ##__VA_ARGS__)

/**
 * vcamera warning output.
 *
 * @format: printf() like format string.
 */
#define vcam_warn(format, ...)                                  \
	vcam_printk(VCAM_MODULE_TAG, "vcam_wrn", KERN_WARNING,        \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * vcamera notice output.
 *
 * @format: printf() like format string.
 */
#define vcam_not(format, ...)                                    \
	vcam_printk(VCAM_MODULE_TAG, "vcam_not", KERN_NOTICE,          \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * vcamera information output.
 *
 * @format: printf() like format string.
 */
#define vcam_info(format, ...)                                     \
	vcam_printk(VCAM_MODULE_TAG, "vcam_inf", KERN_INFO,              \
			 __func__, __LINE__, format, ##__VA_ARGS__)

/**
 * vcamera debug output.
 *
 * @format: printf() like format string.
 */
#define vcam_dbg(format, ...)                                      \
	vcam_debug(VCAM_MODULE_TAG, "vcam_dbg", __func__, __LINE__, format, ##__VA_ARGS__)

#define VCAM_DBG_TRACE
#ifdef VCAM_DBG_TRACE
#define vcam_trace(f, args...)	trace_printk(f, ##args)
#else
#define vcam_trace(f, args...)	no_printk(f, ##args)
#endif
#endif /* ifndef __VCAM_DBG_H__ */


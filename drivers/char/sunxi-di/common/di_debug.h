/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI_DEBUG_H_
#define _DI_DEBUG_H_

#include <linux/printk.h>

#if defined(CONFIG_FPGA_V4_PLATFORM) || defined(CONFIG_FPGA_V7_PLATFORM)
#define DI_FPGA_TEST
#endif

/*
 * debug control, you can switch on (delete 'x' suffix)
 * to enable log output and assert mechanism.
 */
#define CONFIG_ENABLE_DEBUG

#define PRINT printk

/*
 * debug level,
 * if is DEBUG_LEVEL_DISABLE, no log is allowed output,
 * if is DEBUG_LEVEL_ERR, only ERR is allowed output,
 * if is DEBUG_LEVEL_INFO, ERR and INFO are allowed output,
 * if is DEBUG_LEVEL_DEBUG, all log are allowed output,
 */
enum debug_level {
	DEBUG_LEVEL_DISABLE = 0,
	DEBUG_LEVEL_ERR,
	DEBUG_LEVEL_INFO,
	DEBUG_LEVEL_DEBUG,
	DEBUG_LEVEL_TEST,
	DEBUG_LEVEL_FMD,
	DEBUG_LEVEL_MAX,
};

extern int di_debug_mask;

#define DI_ERR(...)                      \
do {                                     \
	if (di_debug_mask >= DEBUG_LEVEL_ERR \
		&& di_debug_mask < DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);              \
	}                                    \
} while (0)

#define DI_INFO(...)                      \
do {                                      \
	if (di_debug_mask >= DEBUG_LEVEL_INFO \
		&& di_debug_mask < DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);               \
	}                                     \
} while (0)

#define DI_DEBUG(...)                      \
do {                                       \
	if (di_debug_mask >= DEBUG_LEVEL_DEBUG \
		&& di_debug_mask < DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);                \
	}                                      \
} while (0)

#define DI_TEST(...)                      \
do {                                       \
	if (di_debug_mask == DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);                \
	}                                      \
} while (0)

#define DI_FMD(...)                      \
do {                                       \
	if (di_debug_mask == DEBUG_LEVEL_FMD \
		|| di_debug_mask == DEBUG_LEVEL_DEBUG) { \
		PRINT(__VA_ARGS__);                \
	}                                      \
} while (0)
#endif /* _DI_DEBUG_H_ */

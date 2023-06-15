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

#ifndef _GRALLOC_DEBUG_H_
#define _GRALLOC_DEBUG_H_

#include <linux/printk.h>


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

extern int debug_mask;

#define GRALLOC_ERR(...)                      \
do {                                     \
	if (debug_mask >= DEBUG_LEVEL_ERR \
		&& debug_mask < DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);              \
	}                                    \
} while (0)

#define GRALLOC_INFO(...)                      \
do {                                      \
	if (debug_mask >= DEBUG_LEVEL_INFO \
		&& debug_mask < DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);               \
	}                                     \
} while (0)

#define GRALLOC_DEBUG(...)                      \
do {                                       \
	if (debug_mask >= DEBUG_LEVEL_DEBUG \
		&& debug_mask < DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);                \
	}                                      \
} while (0)

#define GRALLOC_TEST(...)                      \
do {                                       \
	if (debug_mask == DEBUG_LEVEL_TEST) { \
		PRINT(__VA_ARGS__);                \
	}                                      \
} while (0)

#endif /* _GRALLOC_DEBUG_H_ */

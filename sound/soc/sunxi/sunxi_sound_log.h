/*
 * sound\soc\sunxi\sunxi_sound_log.h
 * (C) Copyright 2021-2026
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <Dby@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef SUNXI_SOUND_LOG_H
#define SUNXI_SOUND_LOG_H
#include <linux/kernel.h>

#define SOUND_LOG_ERR(fmt, arg...) do { \
	    pr_err("[sound %d\t][%s] " fmt, __LINE__, __func__, ##arg); \
	} while (0)

#define SOUND_LOG_WARN(fmt, arg...) do { \
	    pr_warning("[sound %d\t][%s] " fmt, __LINE__, __func__, ##arg); \
	} while (0)

#define SOUND_LOG_INFO(fmt, arg...) do { \
	    pr_info("[sound %d\t][%s] " fmt, __LINE__, __func__, ##arg); \
	} while (0)

#define SOUND_LOG_DEBUG(fmt, arg...) do { \
	    pr_debug("[sound %d\t][%s] " fmt, __LINE__, __func__, ##arg); \
	} while (0)

#endif /* SUNXI_SOUND_LOG_H */

